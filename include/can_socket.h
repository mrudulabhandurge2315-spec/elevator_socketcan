#ifndef CAN_SOCKET_H
#define CAN_SOCKET_H

#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

static inline int can_socket_open(const char *interface_name)
{
    int socket_fd;
    struct ifreq interface_request;
    struct sockaddr_can address;

    socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd < 0) {
        return -1;
    }

    memset(&interface_request, 0, sizeof(interface_request));
    strncpy(interface_request.ifr_name, interface_name,
            sizeof(interface_request.ifr_name) - 1);

    if (ioctl(socket_fd, SIOCGIFINDEX, &interface_request) < 0) {
        close(socket_fd);
        return -1;
    }

    memset(&address, 0, sizeof(address));
    address.can_family = AF_CAN;
    address.can_ifindex = interface_request.ifr_ifindex;

    if (bind(socket_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}

#endif
