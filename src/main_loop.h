#include<arpa/inet.h>
#include<sys/socket.h>
#include <time.h>
#include "flush.h"
#include "fluentd.h"

int main_loop(redisContext *c, bool authentication_activated,
              map_str_t *map_users, int sock, int listening_port,
              char* fluentd_ip, int fluentd_port) {
  redisReply *reply;

  // Declare msg, send buffer and parts.
  char msg[508], value[508];
  struct msg_parts msg_parts;
  // Declare additional helper.
  struct sockaddr_in si_fluentd;
  memset((char *) &si_fluentd, 0, sizeof(si_fluentd));
  si_fluentd.sin_family = AF_INET;
  si_fluentd.sin_port = htons(fluentd_port);
  int n, slen=sizeof(si_fluentd), nb_connection_attempts=0,
      last_connection_attempt=(int)time(NULL);
  int s = connect_fluentd(fluentd_ip, fluentd_port, &si_fluentd,
          &nb_connection_attempts, &last_connection_attempt);
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
    msg_parts = (struct msg_parts){NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                   NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    if (-1 == parse_msg(&msg_parts, msg, n)) {
      goto err_json;
    }
    if (authentication_activated) {
        char **apikey = get_apikey(msg_parts.operator, map_users);
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

    send_msg_fluentd(msg, &s, &si_fluentd, fluentd_ip, fluentd_port, slen,
            &nb_connection_attempts, &last_connection_attempt);

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
}
