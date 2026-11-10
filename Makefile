CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude

PROGRAMS = hmi_ecu sensor_ecu actuator_ecu controller_ecu diagnostic_ecu

.PHONY: all clean

all: $(PROGRAMS)

hmi_ecu: src/hmi_ecu.c include/can_protocol.h
	$(CC) $(CFLAGS) src/hmi_ecu.c -o $@

sensor_ecu: src/sensor_ecu.c include/can_protocol.h include/can_socket.h
	$(CC) $(CFLAGS) src/sensor_ecu.c -o $@

actuator_ecu: src/actuator_ecu.c include/can_protocol.h include/can_socket.h
	$(CC) $(CFLAGS) src/actuator_ecu.c -o $@

controller_ecu: src/controller_ecu.c include/can_protocol.h include/can_socket.h
	$(CC) $(CFLAGS) src/controller_ecu.c -o $@

diagnostic_ecu: src/diagnostic_ecu.c include/can_protocol.h include/can_socket.h
	$(CC) $(CFLAGS) src/diagnostic_ecu.c -o $@

clean:
	rm -f $(PROGRAMS)
