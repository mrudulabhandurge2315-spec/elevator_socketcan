#include <linux/can.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/socket.h>

#include "can_protocol.h"
#include "can_socket.h"

#define TIMER_CHECK_MILLISECONDS 100
#define FLOOR_TRAVEL_SECONDS 1

static int send_frame(int socket_fd, canid_t id,
                      unsigned char first, unsigned char second)
{
    struct can_frame frame;

    memset(&frame, 0, sizeof(frame));
    frame.can_id = id;
    frame.can_dlc = 2;
    frame.data[0] = first;
    frame.data[1] = second;

    return write(socket_fd, &frame, sizeof(frame)) == sizeof(frame) ? 0 : -1;
}

static int time_reached(struct timespec now, struct timespec deadline)
{
    return now.tv_sec > deadline.tv_sec ||
           (now.tv_sec == deadline.tv_sec && now.tv_nsec >= deadline.tv_nsec);
}

static struct timespec add_seconds(struct timespec time, int seconds)
{
    time.tv_sec += seconds;
    return time;
}

int main(void)
{
    int socket_fd = can_socket_open("vcan0");
    struct can_filter filters[] = {
        { .can_id = CAN_ID_MOTION_COMMAND, .can_mask = CAN_SFF_MASK },
        { .can_id = CAN_ID_DOOR_COMMAND,   .can_mask = CAN_SFF_MASK }
    };

    if (setsockopt(socket_fd, SOL_CAN_RAW, CAN_RAW_FILTER,
                   filters, sizeof(filters)) < 0) {
        perror("Could not set Actuator CAN filters");
        close(socket_fd);
        return EXIT_FAILURE;
    }
    unsigned char current_floor = 0;
    unsigned char target_floor = 0;
    unsigned char motor_state = MOTION_STOP;
    unsigned char door_state = DOOR_STATE_OPEN;
    struct timespec next_floor_step = {0, 0};

    if (socket_fd < 0) {
        perror("Could not open CAN socket on vcan0");
        return EXIT_FAILURE;
    }

    printf("Elevator Actuator ECU started on vcan0.\n");
    printf("Simulation only: floor 0, motor stopped, door open.\n");

    for (;;) {
        fd_set read_set;
        struct timeval timeout;
        struct timespec now;
        int ready;

        FD_ZERO(&read_set);
        FD_SET(socket_fd, &read_set);
        timeout.tv_sec = 0;
        timeout.tv_usec = TIMER_CHECK_MILLISECONDS * 1000;

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

            if ((size_t)count == sizeof(received)) {
                if (received.can_id == CAN_ID_DOOR_COMMAND &&
                    received.can_dlc >= 1) {
                    unsigned char action = received.data[0];

                    if (action == DOOR_OPEN && motor_state == MOTION_STOP) {
                        door_state = DOOR_STATE_OPEN;
                        printf("Door opened at floor %u.\n", current_floor);
                    } else if (action == DOOR_CLOSE &&
                               motor_state == MOTION_STOP) {
                        door_state = DOOR_STATE_CLOSED;
                        printf("Door closed.\n");
                    } else if (action == DOOR_CLOSE) {
                        printf("Ignored door close request while moving.\n");
                    }
                } else if (received.can_id == CAN_ID_MOTION_COMMAND &&
                           received.can_dlc >= 2) {
                    unsigned char direction = received.data[0];
                    unsigned char requested_target = received.data[1];

                    if (direction == MOTION_STOP) {
                        motor_state = MOTION_STOP;
                    } else if (requested_target > ELEVATOR_MAX_FLOOR) {
                        printf("Rejected invalid target floor: %u.\n",
                               requested_target);
                    } else if (door_state != DOOR_STATE_CLOSED) {
                        motor_state = MOTION_STOP;
                        printf("Interlock: rejected movement; door is not closed.\n");
                    } else if (requested_target == current_floor) {
                        motor_state = MOTION_STOP;
                    } else if ((direction == MOTION_UP &&
                                requested_target > current_floor) ||
                               (direction == MOTION_DOWN &&
                                requested_target < current_floor)) {
                        if (motor_state != direction ||
                            target_floor != requested_target) {
                            target_floor = requested_target;
                            motor_state = direction;
                            clock_gettime(CLOCK_MONOTONIC, &now);
                            next_floor_step = add_seconds(
                                now, FLOOR_TRAVEL_SECONDS);
                            printf("Moving from floor %u to floor %u.\n",
                                   current_floor, target_floor);
                        }
                    } else {
                        printf("Rejected inconsistent direction or target.\n");
                    }
                }
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &now);
        if (motor_state != MOTION_STOP &&
            time_reached(now, next_floor_step)) {
            if (motor_state == MOTION_UP) {
                current_floor++;
            } else {
                current_floor--;
            }

            printf("Simulated position: floor %u.\n", current_floor);

            if (current_floor == target_floor) {
                motor_state = MOTION_STOP;
                printf("Arrived at floor %u; motor stopped.\n", current_floor);
            } else {
                next_floor_step = add_seconds(now, FLOOR_TRAVEL_SECONDS);
            }
        }
        if (send_frame(socket_fd, CAN_ID_ACTUATOR_STATUS,
                       motor_state, door_state) < 0 ||
            send_frame(socket_fd, CAN_ID_POSITION_STATUS,
                       current_floor, motor_state != MOTION_STOP) < 0) {
            perror("Could not send actuator status");
            close(socket_fd);
            return EXIT_FAILURE;
        }
    }
}
