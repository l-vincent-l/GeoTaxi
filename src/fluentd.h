#ifndef FLUENTD_H_
#define FLUENTD_H_

#include <sys/types.h>

struct fluentd {
    int sock;
    struct sockaddr_in sockaddr;

};

int fluentd_init(struct fluentd *fluentd, const char *ip, int port);
int fluentd_sendmsg(struct fluentd *fluentd, const char *msg);

#endif
