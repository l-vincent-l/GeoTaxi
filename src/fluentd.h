#include<arpa/inet.h>
#include<sys/socket.h>
#include <time.h>

int connect_fluentd(char* fluentd_ip, int fluentd_port,
        struct sockaddr_in* si_fluentd, int *nb_connection_attempts,
        int* last_connection_attempt) {
  if (fluentd_port == -1 || strcmp("", fluentd_ip) == 0) {
      return -1;
  }
  if (nb_connection_attempts > 0 &&
          (int)time(NULL) < (*last_connection_attempt + *nb_connection_attempts * 60)) {
      return -4;
  }
  int s;
  *last_connection_attempt = (int) time(NULL);
  *nb_connection_attempts = *nb_connection_attempts + 1;
  if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
      fprintf(stderr, "Unable to open socket to connect to fluentd\n");
      return -2;
  }
  if (inet_aton(fluentd_ip , &si_fluentd->sin_addr) == 0)  {
     fprintf(stderr, "inet_aton() failed\n");
     return -3;
  }

  *nb_connection_attempts = 0;
  return s;
}

void send_msg_fluentd(char* msg, int *s, struct sockaddr_in* si_fluentd,
        char* fluentd_ip, int fluentd_port, int slen,
        int *nb_connection_attempts, int* last_connection_attempt) {
    if (*s < 0) {
        *s = connect_fluentd(fluentd_ip, fluentd_port, si_fluentd,
                nb_connection_attempts, last_connection_attempt);
        return;
    }
    if (sendto(*s, msg, strlen(msg) , 0 , (struct sockaddr *) si_fluentd, slen)==-1) {
      fprintf(stderr, "Unable to send message %s to fluentd\n", msg);
    }
}
