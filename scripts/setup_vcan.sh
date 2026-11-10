#!/usr/bin/env bash
set -e

sudo modprobe can
sudo modprobe can_raw
sudo modprobe vcan

if ! ip link show vcan0 >/dev/null 2>&1; then
    sudo ip link add dev vcan0 type vcan
fi

sudo ip link set up vcan0
ip -details link show vcan0
