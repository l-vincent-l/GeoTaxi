#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <syslog.h>
#include <stdbool.h>
#include <curl/curl.h>

#include <hiredis/hiredis.h>
#include "js0n.h"
#include "sha1.h"
#include "map.h"
#include "argtable3.h"

#include "msg_parts.h"
#include "parse_msg.h"
#include "sock_utils.h"
#include "get_users.h"
#include "checks.h"
#include "main_loop.h"
#include "flush.h"


int main (int argc, char** argv) {
    struct arg_lit *help;
    struct arg_int *port_arg, *redis_port_arg, *fluentd_port_arg;
    struct arg_str *apikey_arg, *url_users_arg, *redis_url_arg, *fluentd_ip_arg;
    struct arg_end *end;
    void *argtable[] = {
        help           = arg_lit0("h", "help", "Show help"),
        port_arg       = arg_int0("p", "port", "<port>", "Listening port"),
        apikey_arg     = arg_str0(NULL, "apikey", "<apikey>", "Admin apikey to retrieve users"),
        url_users_arg  = arg_str0(NULL, "url", "<url>", "URL to retrieve users"),
        redis_port_arg = arg_int0(NULL, "redisport", "<redis_port>", "Redis port"),
        redis_url_arg  = arg_str0(NULL, "redisurl", "<redis_url>", "Redis url"),
        fluentd_port_arg = arg_int0(NULL, "fluentdport", "<fluentd_port>", "fluentd port"),
        fluentd_ip_arg  = arg_str0(NULL, "fluentdip", "<fluentd_ip>", "fluentd url"),
        end         = arg_end(20),
    };
  int nerrors;
  nerrors = arg_parse(argc,argv,argtable);
  if (help->count > 0) {
        printf("Usage: geotaxi\n");
        arg_print_syntax(stdout, argtable, "\n");
        printf("Listen to taxi positions and send it to redis.\n\n");
        arg_print_glossary(stdout, argtable, "  %-25s %s\n");
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        return 0;
  }
  if (nerrors > 0) {
        /* Display the error details contained in the arg_end struct.*/
        arg_print_errors(stdout, end, "geotaxi");
        printf("Try 'geotaxi --help' for more information.\n");
        arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
        return 1;
  }

  int listening_port = 80;
  if (port_arg->count == 1) {
    listening_port = *(port_arg->ival);
  }
  bool authentication_activated = false;
  char* apikey = "";
  if (apikey_arg->count == 1) {
    apikey = malloc(strlen(apikey_arg->sval[0]));
    sprintf(apikey, "%s", apikey_arg->sval[0]);
    authentication_activated = true;
  }
  char* url_users = "http://127.0.0.1:5000/users";
  if (url_users_arg->count == 1) {
    url_users = malloc(strlen(url_users_arg->sval[0]));
    sprintf(url_users, "%s", url_users_arg->sval[0]);
  }
  int fluend_port = -1;
  if (fluentd_port_arg->count == 1) {
      fluend_port = *(fluentd_port_arg->ival);
  }
  char* fluend_ip = "";
  if (fluentd_ip_arg->count == 1) {
      fluend_ip = malloc(strlen(fluentd_ip_arg->sval[0]));
      sprintf(fluend_ip, "%s", fluentd_ip_arg->sval[0]);
  }
  map_str_t map_users;
  map_init(&map_users);
  if (authentication_activated)
      get_users(&map_users, apikey, url_users);

  // Ignore pipe signals.
  signal(SIGPIPE, SIG_IGN);

  // Initiate UDP server socket.
  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  struct sockaddr_in server = sock_hint(INADDR_ANY, listening_port);
  if (-1 == bind(sock, (struct sockaddr *)&server, sizeof(server))) {
    goto err_bind;
  }

  printf("Now listening on UDP port %i...\n", listening_port);
  FLUSH;

  redisContext *c;
  redisReply *reply;
  char *hostname = "127.0.0.1";
  if (redis_url_arg->count == 1) {
      hostname = malloc(strlen(redis_url_arg->sval[0]));
      sprintf(hostname, "%s", redis_url_arg->sval[0]);
  }
  struct timeval timeout = { 1, 500000 };
  int redis_port = 6379;
  if (redis_port_arg->count == 1) {
      redis_port = *(redis_port_arg->ival);
  }
  c = redisConnectWithTimeout(hostname, redis_port, timeout);
  if (c == NULL || c->err) {
      if (c) {
            printf("Error connecting to database: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
  }
  return main_loop(c, authentication_activated, &map_users, sock,
          listening_port, fluend_ip, fluend_port);

  err_bind:             printf("Error binding socket. You need to be root for ports under 1024. binding to: %d     Exiting...\n", listening_port); FLUSH;
    return 1;
}
