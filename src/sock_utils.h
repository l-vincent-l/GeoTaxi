#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


static inline struct sockaddr_in sock_hint (long addr, short port) {
  struct sockaddr_in hints;
  memset(&hints, 0, sizeof(hints));
  hints.sin_family      = AF_INET;
  hints.sin_addr.s_addr = htonl(addr);
  hints.sin_port        = htons(port);
  return hints;
}
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
