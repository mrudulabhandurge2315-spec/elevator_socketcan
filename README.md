
# Software-Only Elevator Control System Using SocketCAN

**Course:** Automotive Communication Networks  
**System:** Option 4 — Elevator Control System  
**Platform:** Ubuntu on WSL2  
**Language:** C  
**Network:** Linux SocketCAN with virtual CAN (`vcan0`)

> **Safety notice:** This project is an educational software simulation only. It is not certified or suitable for controlling a real elevator or any real machinery. Its software interlocks are demonstrations, not safety systems.

## 1. Project overview

This project models a four-floor elevator using independent software ECUs that exchange CAN frames over a Linux virtual CAN interface. No elevator hardware, CAN adapter, motor, or door actuator is connected.

The demonstrated behavior includes floor requests, simulated door operation, floor-by-floor movement, a door-closed movement interlock, periodic status messages, and a diagnostic sensor-timeout event.

## 2. System objectives

- Demonstrate communication among multiple software ECUs over SocketCAN.
- Define CAN identifiers and payload signals for elevator functions.
- Simulate floor requests, door state, and movement across floors 0–3.
- Reject simulated movement requests when the door is not closed.
- Detect missing position or door sensor messages in the Diagnostic ECU.
- Record implementation and test results for review.

## 3. Architecture

<img width="1276" height="553" alt="image" src="https://github.com/user-attachments/assets/d15a5c0f-1881-452a-8925-008cc29a7184" />


All ECU programs use `vcan0`. The Sensor ECU listens for actuator status and publishes door sensor status. The Actuator ECU publishes simulated position and actuator status. The Controller ECU reads requests and status, then sends commands.

## 4. ECU responsibilities

| ECU | Responsibility |
|---|---|
| HMI ECU | Reads a requested floor from the terminal and sends `FLOOR_REQUEST`. |
| Sensor ECU | Reads actuator status and periodically publishes `DOOR_SENSOR`. |
| Controller ECU | Accepts valid floor requests, checks position and door state, and sends simulated commands. |
| Actuator ECU | Simulates door state and floor-by-floor movement; rejects motion if the door is not closed. |
| Diagnostic ECU | Monitors position and door messages and reports sensor timeout/recovery events. |

## 5. CAN message summary

All messages use standard 11-bit CAN identifiers and classic CAN frames. Byte numbering starts at 0. Values are unsigned unless specified otherwise.

| CAN ID | Name | Sender | Payload |
|---|---|---|---|
| `0x100` | `FLOOR_REQUEST` | HMI | Byte 0: requested floor (`0–3`); byte 1: request sequence |
| `0x110` | `POSITION_STATUS` | Actuator | Byte 0: current floor (`0–3`); byte 1: moving (`0` no, `1` yes) |
| `0x111` | `DOOR_SENSOR` | Sensor | Byte 0: door state; byte 1: obstruction flag (`0` clear, `1` obstructed) |
| `0x120` | `MOTION_COMMAND` | Controller | Byte 0: direction (`0` stop, `1` up, `2` down); byte 1: target floor |
| `0x121` | `DOOR_COMMAND` | Controller | Byte 0: action (`0` hold, `1` open, `2` close) |
| `0x130` | `ACTUATOR_STATUS` | Actuator | Byte 0: motor direction/state; byte 1: door state |
| `0x140` | `ELEVATOR_STATE` | Controller | Byte 0: controller state; byte 1: target floor (`255` means none) |
| `0x1F0` | `DIAGNOSTIC_EVENT` | Diagnostic | Byte 0: event (`0` clear, `1` sensor timeout); byte 1: severity |

Door state values: `0` opening, `1` open, `2` closing, `3` closed, `4` blocked.

The project defines identifiers for HMI heartbeat (`0x101`) and fault-clear request (`0x1F1`) in the protocol header, but those messages are **not implemented** in the current simulation.

## 6. Build

Install the basic tools if needed:

```bash
sudo apt update
sudo apt install build-essential can-utils
```

Build all ECU programs:

```bash
make clean
make
```

The build uses `-Wall -Wextra -Werror`. A successful `make` produces:

```text
hmi_ecu
sensor_ecu
controller_ecu
actuator_ecu
diagnostic_ecu
```

## 7. Set up the virtual CAN interface

After starting or restarting WSL, run:

```bash
sudo modprobe can
sudo modprobe can_raw
sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0
ip -details link show vcan0
```

If the interface already exists, `ip link add` may report `File exists`; bring it up with `sudo ip link set up vcan0`.

## 8. Run the simulation

Open five Ubuntu terminals. In each, change to the project directory and start one ECU:

```bash
cd ~/elevator_socketcan
./actuator_ecu
```

```bash
cd ~/elevator_socketcan
./sensor_ecu
```

```bash
cd ~/elevator_socketcan
./controller_ecu
```

```bash
cd ~/elevator_socketcan
./diagnostic_ecu
```

```bash
cd ~/elevator_socketcan
./hmi_ecu
```

At the HMI prompt, enter a floor from `0` to `3`. The simulation begins at floor 0 each time the Actuator ECU is restarted. Press `q` to stop the HMI. Stop other ECUs with `Ctrl+C`.

To monitor CAN frames in another terminal:

```bash
candump vcan0
```

## 9. Safety behavior and limitations

- The Actuator rejects movement commands unless the simulated door state is closed.
- The Controller closes the simulated door before requesting movement.
- The Actuator stops when it reaches the target floor.
- Floor requests outside `0–3` are rejected.
- The obstruction signal is currently fixed to clear; obstruction injection and fault-clear handling are not implemented.
- Sensor timeout detection is a simple software demonstration and has not been validated for safety use.
- There is no hardware, real motor, real door, load, emergency brake, or certified safety controller.

**Never connect this program to real elevator equipment.**

## 10. Observed implementation checks

The following checks were observed during development:

- All five ECUs compiled using the Makefile with warnings treated as errors.
- HMI floor requests appeared on `vcan0`.
- The Actuator rejected a movement request while the simulated door was open.
- A floor 0 → 2 trip and a floor 2 → 0 trip were observed, with a stop and door opening at each destination.
- The Diagnostic ECU reported a sensor timeout when sensor messages were absent, then reported recovery after messages resumed.

These are software simulation checks, not real-world elevator validation.

## 11. Project structure

```text
elevator_socketcan/
├── Makefile
├── README.md
├── include/
│   ├── can_protocol.h
│   └── can_socket.h
├── src/
│   ├── actuator_ecu.c
│   ├── controller_ecu.c
│   ├── diagnostic_ecu.c
│   ├── hmi_ecu.c
│   └── sensor_ecu.c
├── docs/
└── scripts/
```

