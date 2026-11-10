# Elevator Simulation Test Plan

## Scope

These checks apply only to the software simulation using SocketCAN and `vcan0`. They do not validate real elevator safety or hardware.

## Test cases

| ID | Test | Procedure | Expected result | Result |
|---|---|---|---|---|
| T01 | Build all ECUs | Run `make clean && make` | All five programs compile without errors | Pass observed |
| T02 | HMI valid request | Enter floor 2 in `hmi_ecu` | HMI sends `FLOOR_REQUEST` on `0x100` | Pass observed |
| T03 | Invalid floor request | Send `cansend vcan0 100#0400` | Controller rejects floor 4 | Not yet recorded |
| T04 | Door interlock | With door open, send motion command `cansend vcan0 120#0102` | Actuator rejects movement | Pass observed |
| T05 | Upward trip | Request floor 2 from floor 0 | Door closes, car advances to floor 2, stops, door opens | Pass observed |
| T06 | Downward trip | From floor 2, request floor 0 without restarting Actuator | Car advances down to floor 0, stops, door opens | Pass observed |
| T07 | Diagnostic recovery | Start Diagnostic without sensor, then start Sensor | Timeout event appears, then clears when messages resume | Pass observed |
| T08 | Message monitoring | Run `candump vcan0` during a trip | Expected CAN IDs and payloads are visible | Pass observed |

## Notes

- The Actuator starts at floor 0 whenever its process restarts.
- Use separate terminals for each ECU.
- Stop ECU programs with `Ctrl+C`.
- Record the actual terminal output and date when repeating a test.
- Do not describe these software checks as real-world safety validation.
