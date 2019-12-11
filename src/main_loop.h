#include<arpa/inet.h>
#include<sys/socket.h>
#include <time.h>

#include "fluentd.h"

int main_loop(redisContext *c, bool authentication_activated,
              map_str_t *map_users, int sock, int listening_port,
              char* fluentd_ip, int fluentd_port) {
  redisReply *reply;
  int n;

  // Declare msg, send buffer and parts.
  char msg[508], value[508], copy_msg[508], rite_now[30];
  struct msg_parts msg_parts;
  int t;
  struct timeval tv;

  struct fluentd fluentd;

  fluentd_init(&fluentd, fluentd_ip, fluentd_port);

  // Run forever:
  while (1) {
    // Receive a message of length `n` < 508 into buffer `msg`,
    // and null-terminate the received message
    struct sockaddr_storage sender;
    socklen_t sendsize = sizeof(sender);
    bzero(&sender, sizeof(sender));
    n      = recvfrom(sock, msg, 507, 0, (struct sockaddr*)&sender, &sendsize);
    msg[n] = '\0';
    memset(copy_msg, '\0', n);
    strcpy(copy_msg, msg);

    // We use the timestamp of the server to know if a taxi is up-to-date or not
    // because if the sent timestamp is wrong it's because the client time is wrong
    // not because there's a big lag. so we prefer to store this
    // We also store the sent timestamp in the HSET of the taxi
    t = gettimeofday(&tv, NULL);
    snprintf(rite_now, 30, "%f", (double)tv.tv_sec);

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
            printf("Error checking signature.      Storing it in redis\n");
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

    fluentd_sendmsg(&fluentd, copy_msg);

    // Build and send redis queries
    snprintf(value, 508, "%s %s %s %s %s %s",  msg_parts.timestamp, msg_parts.lat,
             msg_parts.lon, msg_parts.status, msg_parts.device, msg_parts.version);
    reply = redisCommand(c, "HSET taxi:%s %s %s", msg_parts.taxi,
                         msg_parts.operator, value);
    if (REDIS_REPLY_ERROR == reply->type) {
         goto err_redis_write;
    }
    freeReplyObject(reply);

    reply = redisCommand(c, "GEOADD geoindex %s %s %s", msg_parts.lon, msg_parts.lat, msg_parts.taxi);
    if (REDIS_REPLY_ERROR == reply->type) {
         goto err_redis_write;
    }
    freeReplyObject(reply);

    snprintf(value, 508, "%s:%s", msg_parts.taxi, msg_parts.operator);
    reply = redisCommand(c, "GEOADD geoindex_2 %s %s %s", msg_parts.lon, msg_parts.lat, value);
    if (REDIS_REPLY_ERROR == reply->type) {
         goto err_redis_write;
    }
    freeReplyObject(reply);

    snprintf(value, 508, "%s:%s", msg_parts.taxi, msg_parts.operator);
    reply = redisCommand(c, "ZADD timestamps %s %s", rite_now, value);
    if (REDIS_REPLY_ERROR == reply->type) {
         goto err_redis_write;
    }
    freeReplyObject(reply);

    reply = redisCommand(c, "ZADD timestamps_id %s %s", rite_now, msg_parts.taxi);
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


  err_redis_connection: printf("Error connecting to database.  Skipping...\n");
    continue;

  err_redis_write:      printf("Error writing to database : %s      Skipping...\n", reply->str); freeReplyObject(reply);
    continue;

err_json:             printf("[%i]Error parsing json.            Skipping incorrectly formated message... %s\n", (int)tv.tv_sec, msg);
    continue;

  err_get_user:        printf("Error getting user.            Can't find %s\n", msg_parts.operator);
    continue;

  }
}
