#include <linux/can.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <time.h>
#include <unistd.h>

#include "can_protocol.h"
#include "can_socket.h"

#define SENSOR_TIMEOUT_SECONDS 3
#define MONITOR_INTERVAL_MILLISECONDS 500

static double seconds_since(struct timespec earlier,
                            struct timespec later)
{
    return (double)(later.tv_sec - earlier.tv_sec) +
           (double)(later.tv_nsec - earlier.tv_nsec) / 1000000000.0;
}

int main(void)
{
    int socket_fd = can_socket_open("vcan0");
    struct timespec last_position_time = {0, 0};
    struct timespec last_door_time = {0, 0};
    unsigned char fault_active = 0;
    unsigned char last_reported_fault = 255;

    if (socket_fd < 0) {
        perror("Could not open CAN socket on vcan0");
        return EXIT_FAILURE;
    }

    printf("Elevator Diagnostic ECU started on vcan0.\n");
    printf("Monitoring position and door sensor messages.\n");

    for (;;) {
        fd_set read_set;
        struct timeval timeout;
        int ready;
        struct timespec now;

        FD_ZERO(&read_set);
        FD_SET(socket_fd, &read_set);

        timeout.tv_sec = 0;
        timeout.tv_usec = MONITOR_INTERVAL_MILLISECONDS * 1000;

        ready = select(socket_fd + 1, &read_set, NULL, NULL, &timeout);
        if (ready < 0) {
            perror("Could not wait for CAN messages");
            close(socket_fd);
            return EXIT_FAILURE;
        }

        clock_gettime(CLOCK_MONOTONIC, &now);

        if (ready > 0 && FD_ISSET(socket_fd, &read_set)) {
            struct can_frame received_frame;
            ssize_t bytes_read = read(socket_fd, &received_frame,
                                      sizeof(received_frame));

            if (bytes_read < 0) {
                perror("Could not read CAN frame");
                close(socket_fd);
                return EXIT_FAILURE;
            }

            if ((size_t)bytes_read != sizeof(received_frame)) {
                fprintf(stderr, "Received an incomplete CAN frame.\n");
                continue;
            }

            if (received_frame.can_id == CAN_ID_POSITION_STATUS) {
                last_position_time = now;
            } else if (received_frame.can_id == CAN_ID_DOOR_SENSOR) {
                last_door_time = now;
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &now);

        if (last_position_time.tv_sec == 0 || last_door_time.tv_sec == 0 ||
            seconds_since(last_position_time, now) > SENSOR_TIMEOUT_SECONDS ||
            seconds_since(last_door_time, now) > SENSOR_TIMEOUT_SECONDS) {
            fault_active = 1;
        } else {
            fault_active = 0;
        }

        if (fault_active != last_reported_fault) {
            struct can_frame diagnostic_frame;
            unsigned char event_code = fault_active
                ? DIAG_SENSOR_TIMEOUT : DIAG_NONE;
            unsigned char severity = fault_active
                ? DIAG_SEVERITY_CRITICAL : DIAG_SEVERITY_INFO;

            memset(&diagnostic_frame, 0, sizeof(diagnostic_frame));
            diagnostic_frame.can_id = CAN_ID_DIAGNOSTIC_EVENT;
            diagnostic_frame.can_dlc = 2;
            diagnostic_frame.data[0] = event_code;
            diagnostic_frame.data[1] = severity;

            if (write(socket_fd, &diagnostic_frame, sizeof(diagnostic_frame))
                != sizeof(diagnostic_frame)) {
                perror("Could not send diagnostic event");
                close(socket_fd);
                return EXIT_FAILURE;
            }

            printf("Diagnostic event: code=%u, severity=%u\n",
                   event_code, severity);
            last_reported_fault = fault_active;
        }
    }

    close(socket_fd);
    return EXIT_SUCCESS;
}
