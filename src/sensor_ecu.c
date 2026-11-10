#include <linux/can.h>
#include <linux/can/raw.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include "can_protocol.h"
#include "can_socket.h"

#define REPORT_INTERVAL_MILLISECONDS 500

int main(void)
{
    int socket_fd = can_socket_open("vcan0");
    struct can_filter filter = {
        .can_id = CAN_ID_ACTUATOR_STATUS,
        .can_mask = CAN_SFF_MASK
    };
    unsigned char door_state = DOOR_STATE_OPEN;
    unsigned char obstruction = 0;

    if (socket_fd < 0) {
        perror("Could not open CAN socket on vcan0");
        return EXIT_FAILURE;
    }

    if (setsockopt(socket_fd, SOL_CAN_RAW, CAN_RAW_FILTER,
                   &filter, sizeof(filter)) < 0) {
        perror("Could not set Sensor ECU CAN filter");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    printf("Elevator Sensor ECU started on vcan0.\n");
    printf("Reports simulated door status; position comes from the Actuator ECU.\n");

    for (;;) {
        fd_set read_set;
        struct timeval timeout;
        int ready;

        FD_ZERO(&read_set);
        FD_SET(socket_fd, &read_set);
        timeout.tv_sec = 0;
        timeout.tv_usec = REPORT_INTERVAL_MILLISECONDS * 1000;

        ready = select(socket_fd + 1, &read_set, NULL, NULL, &timeout);
        if (ready < 0) {
            perror("Could not wait for CAN messages");
            close(socket_fd);
            return EXIT_FAILURE;
        }

        if (ready > 0 && FD_ISSET(socket_fd, &read_set)) {
            struct can_frame received;
            ssize_t count = read(socket_fd, &received, sizeof(received));

            if (count < 0) {
                perror("Could not read CAN frame");
                close(socket_fd);
                return EXIT_FAILURE;
            }

            if ((size_t)count == sizeof(received) &&
                received.can_dlc >= 2) {
                door_state = received.data[1];
            }
        }

        {
            struct can_frame door_frame;

            memset(&door_frame, 0, sizeof(door_frame));
            door_frame.can_id = CAN_ID_DOOR_SENSOR;
            door_frame.can_dlc = 2;
            door_frame.data[0] = door_state;
            door_frame.data[1] = obstruction;

            if (write(socket_fd, &door_frame, sizeof(door_frame))
                != sizeof(door_frame)) {
                perror("Could not send door sensor status");
                close(socket_fd);
                return EXIT_FAILURE;
            }
        }
    }

    close(socket_fd);
    return EXIT_SUCCESS;
}
