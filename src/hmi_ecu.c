#include <errno.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "can_protocol.h"

int main(void)
{
    int socket_fd;
    struct ifreq interface_request;
    struct sockaddr_can address;
    struct can_frame frame;
    unsigned char sequence = 0;

    socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd < 0) {
        perror("Could not create CAN socket");
        return EXIT_FAILURE;
    }

    memset(&interface_request, 0, sizeof(interface_request));
    strncpy(interface_request.ifr_name, "vcan0",
            sizeof(interface_request.ifr_name) - 1);

    if (ioctl(socket_fd, SIOCGIFINDEX, &interface_request) < 0) {
        perror("Could not find vcan0");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    memset(&address, 0, sizeof(address));
    address.can_family = AF_CAN;
    address.can_ifindex = interface_request.ifr_ifindex;

    if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Could not bind CAN socket to vcan0");
        close(socket_fd);
        return EXIT_FAILURE;
    }

    printf("Elevator HMI ECU ready on vcan0.\n");
    printf("Enter a floor from 0 to %d, or q to quit.\n",
           ELEVATOR_MAX_FLOOR);

    for (;;) {
        char input[32];
        char *end_pointer;
        long floor;

        printf("Requested floor: ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        if (input[0] == 'q' || input[0] == 'Q') {
            break;
        }

        errno = 0;
        floor = strtol(input, &end_pointer, 10);

        if (errno != 0 || end_pointer == input ||
            (*end_pointer != '\n' && *end_pointer != '\0') ||
            floor < ELEVATOR_MIN_FLOOR ||
            floor > ELEVATOR_MAX_FLOOR) {
            printf("Please enter a whole-number floor from 0 to %d.\n",
                   ELEVATOR_MAX_FLOOR);
            continue;
        }

        memset(&frame, 0, sizeof(frame));
        frame.can_id = CAN_ID_FLOOR_REQUEST;
        frame.can_dlc = 2;
        frame.data[0] = (unsigned char)floor;
        frame.data[1] = sequence++;

        if (write(socket_fd, &frame, sizeof(frame)) != sizeof(frame)) {
            perror("Could not send floor request");
            close(socket_fd);
            return EXIT_FAILURE;
        }

        printf("Sent request for floor %ld (sequence %u).\n",
               floor, (unsigned char)(sequence - 1));
    }

    close(socket_fd);
    printf("Elevator HMI ECU stopped.\n");
    return EXIT_SUCCESS;
}

