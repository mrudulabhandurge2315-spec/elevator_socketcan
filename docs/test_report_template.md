# Elevator Control System — Test Report

**Course:** Automotive Communication Networks  
**Project:** Option 4 — Elevator Control System  
**Student:** Mrudula Bhandurge  
**Date performed:** 10 November 2026  
**Environment:** Ubuntu on WSL2  
**Kernel:** `6.6.123.2-microsoft-standard-WSL2+`  
**CAN interface:** `vcan0`

## 1. Purpose

This report records the build and functional checks performed on a software-only elevator simulation using Linux SocketCAN. The project is for educational use and does not certify or validate real elevator equipment.

## 2. Build results

**Command used:**

```bash
make clean
make
```

**Result:** All five ECU programs compiled successfully with `-Wall -Wextra -Werror`.

## 3. Test results

| Test ID | Test description | Result | Evidence |
|---|---|---|---|
| T01 | Build all ECUs | Pass | `make clean` and `make` completed successfully for all five ECUs. |
| T02 | HMI valid floor request | Pass | HMI sent requests for floors 2 and 0. |
| T03 | Invalid floor request | Not run | No controller rejection output was recorded for an invalid floor. |
| T04 | Door-closed movement interlock | Pass | Actuator rejected a movement command while the simulated door was open. |
| T05 | Floor 0 to floor 2 trip | Pass | Actuator log showed movement through floor 1, arrival at floor 2, motor stop, and door opening. |
| T06 | Floor 2 to floor 0 trip | Pass | In the same Actuator run, the car moved through floor 1, arrived at floor 0, stopped, and opened the door. |
| T07 | Diagnostic timeout and recovery | Pass | Diagnostic ECU reported sensor timeout (`code=1`, `severity=2`), then recovery (`code=0`, `severity=0`) when sensor messages resumed. |
| T08 | CAN message monitoring | Pass | `candump vcan0` displayed request, position, door sensor, actuator status, elevator state, and diagnostic messages during earlier checks. |

## 4. CAN observations

| CAN ID | Message | Example payload | Meaning |
|---|---|---|---|
| `0x100` | `FLOOR_REQUEST` | `02 00` | Request floor 2, sequence 0 |
| `0x110` | `POSITION_STATUS` | `00 00` | At floor 0, not moving |
| `0x111` | `DOOR_SENSOR` | `01 00` | Door open, no obstruction |
| `0x121` | `DOOR_COMMAND` | `02` | Request door close |
| `0x130` | `ACTUATOR_STATUS` | `00 03` | Motor stopped, door closed |
| `0x140` | `ELEVATOR_STATE` | `00 02` | Controller idle, target floor 2 |
| `0x1F0` | `DIAGNOSTIC_EVENT` | `01 02` | Sensor timeout, critical severity |
| `0x1F0` | `DIAGNOSTIC_EVENT` | `00 00` | No active diagnostic fault |

## 5. Limitations

- This is a software-only simulation using `vcan0`; no physical elevator, motor, door, or CAN hardware is connected.
- The obstruction signal remains fixed to clear. Obstruction injection is not implemented.
- Fault-clear requests and HMI heartbeat messages are defined in the protocol header but are not implemented.
- The Diagnostic ECU performs a simple message-timeout check. It is not a certified safety monitor.
- The Controller printed “Destination reached” before the Actuator log showed the simulated trip finishing. Trip completion evidence in this report is therefore based on the Actuator’s floor-by-floor output.
- These results do not establish real-world elevator safety or suitability.

## 6. Conclusion

The five ECU programs built successfully and communicated over the virtual CAN interface. The recorded simulation checks demonstrated valid HMI requests, the door-closed movement interlock, trips from floor 0 to floor 2 and back to floor 0, and diagnostic timeout and recovery behavior.

The invalid-floor case was not run. This project is an educational simulation only and must not be used to control real elevator equipment.
