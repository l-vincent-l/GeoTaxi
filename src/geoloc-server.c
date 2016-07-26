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
#include "msg_parts.h"
#include "parse_msg.h"
#include "sock_utils.h"
#include "get_users.h"
#include "checks.h"
#include "argtable3.h"

#ifdef FLUSHSTDOUT
#define FLUSH fflush(stdout);
#else
#define FLUSH
#endif


int main (int argc, char** argv) {
    struct arg_lit *help;
    struct arg_int *port_arg, *redis_port_arg;
    struct arg_str *apikey_arg, *url_users_arg, *redis_url_arg;
    struct arg_end *end;
    void *argtable[] = {
        help           = arg_lit0("h", "help", "Show help"),
        port_arg       = arg_int0("p", "port", "<port>", "Listening port"),
        apikey_arg     = arg_str0(NULL, "apikey", "<apikey>", "Admin apikey to retrieve users"),
        url_users_arg  = arg_str0(NULL, "url", "<url>", "URL to retrieve users"),
        redis_port_arg = arg_int0(NULL, "redisport", "<redis_port>", "Redis port"),
        redis_url_arg  = arg_str0(NULL, "redisurl", "<redis_url>", "Redis url"),
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
    sprintf(apikey, "%s", apikey_arg->sval[0]);
    authentication_activated = true;
  }
  char* url_users = "http://127.0.0.1:5000/users";
  if (url_users_arg->count == 1) {
    sprintf(url_users, "%s", url_users_arg->sval[0]);
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

  // Declare msg, send buffer and parts.
  char msg[508], value[508];
  struct msg_parts msg_parts;

  // Declare additional helper.
  int n;

  redisContext *c;
  redisReply *reply;
  char *hostname = "127.0.0.1";
  if (redis_url_arg->count == 1) {
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
   openlog("geotaxi", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
  // Run forever:
  while (1) {
    // Receive a message of length `n` < 508 into buffer `msg`,
    // and null-terminate the received message
    struct sockaddr_storage sender;
    socklen_t sendsize = sizeof(sender);
    bzero(&sender, sizeof(sender));
    n      = recvfrom(sock, msg, 507, 0, (struct sockaddr*)&sender, &sendsize);
    msg[n] = '\0';

    // Parse JSON message into parts.
    msg_parts = (struct msg_parts){NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    if (-1 == parse_msg(&msg_parts, msg, n)) {
      goto err_json;
    }
    if (authentication_activated) {
        char **apikey = get_apikey(msg_parts.operator, &map_users);
        if (NULL == apikey) {
          goto err_get_user;
        }
        // Check the hash to make sure the message has not been forged
        if (0 != check_hash(&msg_parts, *apikey))  {
            printf("Error checking signature.      Storing it in redis\n"); FLUSH;
            reply = redisCommand(c, "ZINCRBY badhash_operators 1 %s", msg_parts.operator);
            if (REDIS_REPLY_ERROR == reply->type) {
                 goto err_redis_write;
            }
            freeReplyObject(reply);
            reply = redisCommand(c, "ZINCRBY badhash_taxis_ids 1 %s", msg_parts.taxi);
            if (REDIS_REPLY_ERROR == reply->type) {
                 goto err_redis_write;
            }
            freeReplyObject(reply);
            char addr_str_bad_hash[INET_ADDRSTRLEN];
            reply = redisCommand(c, "ZINCRBY badhash_ips 1 %s", 
                inet_ntop(sender.ss_family, get_in_addr((struct sockaddr *)&sender),
                addr_str_bad_hash, sizeof addr_str_bad_hash)
            );
            if (REDIS_REPLY_ERROR == reply->type) {
                 goto err_redis_write;
            }
            freeReplyObject(reply);
            continue;
        }
    }


    // Check the timestamp to make sure it is not a replay of an old message 
    if (-1 == check_timestamp(&msg_parts))  {
      goto err_timestamp;
    }

    syslog(LOG_NOTICE, "%s", msg);


    // Build and send redis queries
    snprintf(value, 508, "%s %s %s %s %s %s",  msg_parts.timestamp, msg_parts.lat,
             msg_parts.lon, msg_parts.status, msg_parts.device, msg_parts.version);
    reply = redisCommand(c, "HSET taxi:%s %s %s", msg_parts.taxi,
                         msg_parts.operator, value);
    if (REDIS_REPLY_ERROR == reply->type) {
         goto err_redis_write;
    }
    freeReplyObject(reply);

    reply = redisCommand(c, "GEOADD geoindex %s %s %s", msg_parts.lat, msg_parts.lon, msg_parts.taxi);
    if (REDIS_REPLY_ERROR == reply->type) {
         goto err_redis_write;
    }
    freeReplyObject(reply);

    snprintf(value, 508, "%s:%s", msg_parts.taxi, msg_parts.operator);
    reply = redisCommand(c, "GEOADD geoindex_2 %s %s %s", msg_parts.lat, msg_parts.lon, value);
    if (REDIS_REPLY_ERROR == reply->type) {
         goto err_redis_write;
    }
    freeReplyObject(reply);

    snprintf(value, 508, "%s:%s", msg_parts.taxi, msg_parts.operator);
    reply = redisCommand(c, "ZADD timestamps %s %s", msg_parts.timestamp, value);
    if (REDIS_REPLY_ERROR == reply->type) {
         goto err_redis_write;
    }
    freeReplyObject(reply);

    reply = redisCommand(c, "ZADD timestamps_id %s %s", msg_parts.timestamp, msg_parts.taxi);
    if (REDIS_REPLY_ERROR == reply->type) {
         goto err_redis_write;
    }
    freeReplyObject(reply);

    char addr_str[INET_ADDRSTRLEN];
    snprintf(value, 508, "ips:%s", msg_parts.operator);
    reply = redisCommand(c, "SADD %s %s", value,
        inet_ntop(sender.ss_family, get_in_addr((struct sockaddr *)&sender),
        addr_str, sizeof addr_str)
    );
    if (REDIS_REPLY_ERROR == reply->type) {
         goto err_redis_write;
    }
    freeReplyObject(reply);

    continue;

err_bind:             printf("Error binding socket. You need to be root for ports under 1024. binding to: %d     Exiting...\n", listening_port); FLUSH;
    return 1;

  err_redis_connection: printf("Error connecting to database.  Skipping...\n"); FLUSH;
    continue;

err_redis_write:      printf("Error writing to database : %s      Skipping...\n", reply->str); freeReplyObject(reply); FLUSH;
    continue;

  err_timestamp:        printf("Error checking timestamp.(%s)      Skipping stale or replayed message...\n", msg_parts.operator); FLUSH;
    continue;


  err_json:             printf("Error parsing json.            Skipping incorrectly formated message...\n"); FLUSH;
    continue;

  err_get_user:        printf("Error getting user.            Can't find %s\n", msg_parts.operator); FLUSH;
    continue;

  }
  closelog();

}
