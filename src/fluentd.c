#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

#include "fluentd.h"


int
fluentd_init(struct fluentd *fluentd, const char *ip, int port)
{
    if (!ip || port < 0) {
        return -1;
    }

    memset(fluentd, 0, sizeof(*fluentd));

    fluentd->sockaddr.sin_family = AF_INET;
    fluentd->sockaddr.sin_port = htons(port);

    if ((fluentd->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        fprintf(stderr, "fluentd_init: unable to create socket: %s\n", strerror(errno));
        return -1;
    }

    // inet_aton returns 0 on error
    if (inet_aton(ip, &(fluentd->sockaddr.sin_addr)) == 0) {
        fprintf(stderr, "fluentd_init: invalid IP address '%s'\n", ip);
        return -1;
    }

    return 0;
}

int
fluentd_sendmsg(struct fluentd *fluentd, const char *msg)
{
    int ret;

    ret = sendto(
        fluentd->sock, msg, strlen(msg), 0,
        (struct sockaddr *)&(fluentd->sockaddr), sizeof(fluentd->sockaddr)
    );

    if (ret < 0) {
        fprintf(stderr, "fluentd_sendmsg: unable to send message - %s\n", strerror(errno));
        return -1;
    }
    return 0;
}
