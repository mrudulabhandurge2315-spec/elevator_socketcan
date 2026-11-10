#ifndef CAN_PROTOCOL_H
#define CAN_PROTOCOL_H

/*
 * Educational SocketCAN elevator simulation.
 * This software must never control a real elevator.
 */

#include <stdint.h>

/* Standard 11-bit CAN identifiers */
#define CAN_ID_FLOOR_REQUEST       0x100
#define CAN_ID_HMI_HEARTBEAT       0x101
#define CAN_ID_POSITION_STATUS     0x110
#define CAN_ID_DOOR_SENSOR         0x111
#define CAN_ID_MOTION_COMMAND      0x120
#define CAN_ID_DOOR_COMMAND        0x121
#define CAN_ID_ACTUATOR_STATUS     0x130
#define CAN_ID_ELEVATOR_STATE      0x140
#define CAN_ID_DIAGNOSTIC_EVENT    0x1F0
#define CAN_ID_FAULT_CLEAR_REQUEST 0x1F1

#define ELEVATOR_MIN_FLOOR 0
#define ELEVATOR_MAX_FLOOR 3

/* MOTION_COMMAND byte 0 */
#define MOTION_STOP 0
#define MOTION_UP   1
#define MOTION_DOWN 2

/* DOOR_COMMAND byte 0 */
#define DOOR_HOLD  0
#define DOOR_OPEN  1
#define DOOR_CLOSE 2

/* DOOR_SENSOR / ACTUATOR_STATUS door-state values */
#define DOOR_STATE_OPENING  0
#define DOOR_STATE_OPEN     1
#define DOOR_STATE_CLOSING  2
#define DOOR_STATE_CLOSED   3
#define DOOR_STATE_BLOCKED  4

/* ELEVATOR_STATE byte 0 */
#define ELEVATOR_IDLE          0
#define ELEVATOR_CLOSING_DOOR  1
#define ELEVATOR_MOVING        2
#define ELEVATOR_STOPPING      3
#define ELEVATOR_OPENING_DOOR  4
#define ELEVATOR_DOOR_OPEN     5
#define ELEVATOR_FAULT         6

/* DIAGNOSTIC_EVENT byte 0 */
#define DIAG_NONE                  0
#define DIAG_SENSOR_TIMEOUT        1
#define DIAG_DOOR_OBSTRUCTION      2
#define DIAG_INVALID_FLOOR_REQUEST 3
#define DIAG_UNEXPECTED_MOTION     4

/* DIAGNOSTIC_EVENT byte 1 */
#define DIAG_SEVERITY_INFO     0
#define DIAG_SEVERITY_WARNING  1
#define DIAG_SEVERITY_CRITICAL 2

#endif
