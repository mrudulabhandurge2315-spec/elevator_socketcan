#include <linux/can.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/socket.h>

#include "can_protocol.h"
#include "can_socket.h"

#define LOOP_INTERVAL_MILLISECONDS 100
#define NO_FLOOR_REQUEST 255
#define NO_COMMAND_SENT 255

static int send_command(int socket_fd, canid_t id,
                        unsigned char value0, unsigned char value1)
{
    struct can_frame frame;

    memset(&frame, 0, sizeof(frame));
    frame.can_id = id;
    frame.can_dlc = 2;
    frame.data[0] = value0;
    frame.data[1] = value1;

    return write(socket_fd, &frame, sizeof(frame)) == sizeof(frame) ? 0 : -1;
}

int main(void)
{
    int socket_fd = can_socket_open("vcan0");
    struct can_filter filters[] = {
        { .can_id = CAN_ID_FLOOR_REQUEST,   .can_mask = CAN_SFF_MASK },
        { .can_id = CAN_ID_POSITION_STATUS, .can_mask = CAN_SFF_MASK },
        { .can_id = CAN_ID_DOOR_SENSOR,     .can_mask = CAN_SFF_MASK }
    };

    if (setsockopt(socket_fd, SOL_CAN_RAW, CAN_RAW_FILTER,
                   filters, sizeof(filters)) < 0) {
        perror("Could not set Controller CAN filters");
        close(socket_fd);
        return EXIT_FAILURE;
    }
    unsigned char current_floor = 0;
    unsigned char target_floor = NO_FLOOR_REQUEST;
    unsigned char door_state = DOOR_STATE_OPEN;
    unsigned char controller_state = ELEVATOR_IDLE;
    unsigned char last_door_command = NO_COMMAND_SENT;
    unsigned char last_motion_direction = NO_COMMAND_SENT;
    unsigned char last_motion_target = NO_COMMAND_SENT;
    int have_position = 0;
    int have_door_state = 0;

    if (socket_fd < 0) {
        perror("Could not open CAN socket on vcan0");
        return EXIT_FAILURE;
    }

    printf("Elevator Controller ECU started on vcan0.\n");
    printf("Simulation only. Waiting for position and door status.\n");

    for (;;) {
        fd_set read_set;
        struct timeval timeout;
        int ready;

        FD_ZERO(&read_set);
        FD_SET(socket_fd, &read_set);
        timeout.tv_sec = 0;
        timeout.tv_usec = LOOP_INTERVAL_MILLISECONDS * 1000;

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

            if ((size_t)count != sizeof(received)) {
                fprintf(stderr, "Received an incomplete CAN frame.\n");
                continue;
            }

            if (received.can_id == CAN_ID_FLOOR_REQUEST &&
                received.can_dlc >= 1) {
                unsigned char requested_floor = received.data[0];

                if (requested_floor <= ELEVATOR_MAX_FLOOR) {
                    target_floor = requested_floor;
                    controller_state = ELEVATOR_IDLE;
                    last_door_command = NO_COMMAND_SENT;
                    last_motion_direction = NO_COMMAND_SENT;
                    last_motion_target = NO_COMMAND_SENT;
                    printf("Accepted floor request: %u.\n", target_floor);
                } else {
                    printf("Rejected invalid floor request: %u.\n",
                           requested_floor);
                }
            } else if (received.can_id == CAN_ID_POSITION_STATUS &&
                       received.can_dlc >= 1 &&
                       received.data[0] <= ELEVATOR_MAX_FLOOR) {
                current_floor = received.data[0];
                have_position = 1;
            } else if (received.can_id == CAN_ID_DOOR_SENSOR &&
                       received.can_dlc >= 2) {
                door_state = received.data[0];
                have_door_state = 1;

                if (received.data[1] != 0) {
                    printf("Obstruction detected; stopping simulated movement.\n");
                    target_floor = NO_FLOOR_REQUEST;
                    controller_state = ELEVATOR_FAULT;

                    if (send_command(socket_fd, CAN_ID_MOTION_COMMAND,
                                     MOTION_STOP, current_floor) < 0 ||
                        send_command(socket_fd, CAN_ID_DOOR_COMMAND,
                                     DOOR_OPEN, 0) < 0) {
                        perror("Could not send safety response");
                        close(socket_fd);
                        return EXIT_FAILURE;
                    }
                    last_door_command = DOOR_OPEN;
                }
            }
        }

        if (target_floor <= ELEVATOR_MAX_FLOOR &&
            have_position && have_door_state &&
            controller_state != ELEVATOR_FAULT) {
            if (current_floor == target_floor) {
                if (send_command(socket_fd, CAN_ID_MOTION_COMMAND,
                                 MOTION_STOP, current_floor) < 0) {
                    perror("Could not send stop command");
                    close(socket_fd);
                    return EXIT_FAILURE;
                }

                if (door_state != DOOR_STATE_OPEN) {
                    controller_state = ELEVATOR_OPENING_DOOR;

                    if (last_door_command != DOOR_OPEN) {
                        if (send_command(socket_fd, CAN_ID_DOOR_COMMAND,
                                         DOOR_OPEN, 0) < 0) {
                            perror("Could not send door-open command");
                            close(socket_fd);
                            return EXIT_FAILURE;
                        }
                        last_door_command = DOOR_OPEN;
                    }
                } else {
                    controller_state = ELEVATOR_DOOR_OPEN;
                    target_floor = NO_FLOOR_REQUEST;
                    last_motion_direction = NO_COMMAND_SENT;
                    last_motion_target = NO_COMMAND_SENT;
                    printf("Destination reached; door is open.\n");
                }
            } else if (door_state != DOOR_STATE_CLOSED) {
                controller_state = ELEVATOR_CLOSING_DOOR;

                if (last_door_command != DOOR_CLOSE) {
                    if (send_command(socket_fd, CAN_ID_DOOR_COMMAND,
                                     DOOR_CLOSE, 0) < 0) {
                        perror("Could not send door-close command");
                        close(socket_fd);
                        return EXIT_FAILURE;
                    }
                    last_door_command = DOOR_CLOSE;
                }
            } else {
                unsigned char direction =
                    target_floor > current_floor ? MOTION_UP : MOTION_DOWN;

                controller_state = ELEVATOR_MOVING;

                if (last_motion_direction != direction ||
                    last_motion_target != target_floor) {
                    if (send_command(socket_fd, CAN_ID_MOTION_COMMAND,
                                     direction, target_floor) < 0) {
                        perror("Could not send motion command");
                        close(socket_fd);
                        return EXIT_FAILURE;
                    }
                    last_motion_direction = direction;
                    last_motion_target = target_floor;
                }
            }
        } else if (target_floor > ELEVATOR_MAX_FLOOR &&
                   controller_state != ELEVATOR_FAULT &&
                   controller_state != ELEVATOR_DOOR_OPEN) {
            controller_state = ELEVATOR_IDLE;
        }

        if (send_command(socket_fd, CAN_ID_ELEVATOR_STATE,
                         controller_state, target_floor) < 0) {
            perror("Could not publish elevator state");
            close(socket_fd);
            return EXIT_FAILURE;
        }
    }

    close(socket_fd);
    return EXIT_SUCCESS;
}
