#!/usr/bin/env bash
set -e

cd "$(dirname "$0")/.."
make

mkdir -p logs

stdbuf -oL -eL ./actuator_ecu >logs/actuator.log 2>&1 &
actuator_pid=$!

stdbuf -oL -eL ./sensor_ecu >logs/sensor.log 2>&1 &
sensor_pid=$!

stdbuf -oL -eL ./controller_ecu >logs/controller.log 2>&1 &
controller_pid=$!

stdbuf -oL -eL ./diagnostic_ecu >logs/diagnostic.log 2>&1 &
diagnostic_pid=$!

cleanup() {
    kill "$actuator_pid" "$sensor_pid" "$controller_pid" "$diagnostic_pid" \
        2>/dev/null || true
    wait 2>/dev/null || true
    echo
    echo "ECUs stopped. Logs are in the logs/ directory."
}

trap cleanup EXIT INT TERM

sleep 1
echo "Simulation started. Enter floors 0-3 in the HMI; enter q to quit."
./hmi_ecu
