# Elevator Control System — Test Report

**Course:** Automotive Communication Networks  
**Project:** Option 4 — Elevator Control System  
**Student / Team:**  
**Date performed:**  
**Environment:** Ubuntu WSL2  
**Kernel version:**  
**CAN interface:** `vcan0`

## 1. Purpose

This report records verification of a software-only elevator simulation using Linux SocketCAN. It does not certify or validate real elevator equipment.

## 2. Build results

Command used:

```bash
make clean
make
```

Result:

- [ ] All five ECUs compiled successfully
- [ ] Build errors occurred (describe below)

Notes:

## 3. Test results

| Test ID | Test description | Pass / Fail / Not run | Evidence or observed output |
|---|---|---|---|
| T01 | Build all ECUs |  |  |
| T02 | HMI valid floor request |  |  |
| T03 | Invalid floor request |  |  |
| T04 | Door-closed movement interlock |  |  |
| T05 | Floor 0 to floor 2 trip |  |  |
| T06 | Floor 2 to floor 0 trip |  |  |
| T07 | Diagnostic timeout and recovery |  |  |
| T08 | CAN message monitoring |  |  |

## 4. CAN observations

List observed IDs and example payloads from `candump vcan0`:

| CAN ID | Message | Example payload | Meaning |
|---|---|---|---|
|  |  |  |  |

## 5. Limitations

- Software-only simulation using `vcan0`.
- No physical elevator, motor, door, or CAN hardware is connected.
- Obstruction injection and fault-clear handling are not implemented.
- Results do not establish real-world safety or suitability.

## 6. Conclusion

Summarize which tests passed, failed, or were not run. Do not claim tests that were not performed.
```

