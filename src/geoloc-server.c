#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

#include <hiredis/hiredis.h>
#include "js0n.h"
#include "sha1.h"

#ifdef FLUSHSTDOUT
#define FLUSH fflush(stdout);
#else
#define FLUSH
#endif

struct msg_parts {
  char *operator,
       *version,
    *taxi,
    *lat,
    *lon,
    *timestamp,
    *status,
    *device,
    *hash;
  short operator_len,
	version_len,
    taxi_len,
    lat_len,
    lon_len,
    timestamp_len,
    status_len,
    device_len,
    hash_len;
};

static inline struct sockaddr_in sock_hint (long addr, short port) {
  struct sockaddr_in hints;
  memset(&hints, 0, sizeof(hints));
  hints.sin_family      = AF_INET;
  hints.sin_addr.s_addr = htonl(addr);
  hints.sin_port        = htons(port);
  return hints;
}

static inline int parse_msg (struct msg_parts *parts, char *msg, int n) {
   int len=0;
   parts->operator = js0n("operator",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->operator_len = len;
   parts->version = js0n("version",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->version_len = len;
   parts->taxi = js0n("taxi",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->taxi_len = len;
   parts->lat = js0n("lat",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->lat_len = len;
   parts->lon = js0n("lon",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->lon_len = len;
   parts->timestamp = js0n("timestamp",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->timestamp_len = len;
   parts->status = js0n("status",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->status_len = len;
   parts->device = js0n("device",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->device_len = len;
   parts->hash = js0n("hash",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->hash_len = len;

   parts->hash[parts->hash_len] = '\0';
   parts->device[parts->device_len] = '\0';
   parts->status[parts->status_len] = '\0';
   parts->timestamp[parts->timestamp_len] = '\0';
   parts->lon[parts->lon_len] = '\0';
   parts->lat[parts->lat_len] = '\0';
   parts->taxi[parts->taxi_len] = '\0';
   parts->version[parts->version_len] = '\0';
   parts->operator[parts->operator_len] = '\0';

   return 0;
}

static inline int check_hash (struct msg_parts *parts) {
	//TODO
	return 0;
}

static inline int check_timestamp (struct msg_parts *parts) {
        // Declare a few helpers.
        int t;
        struct timeval tv;
        double ritenow, tstmp;

        t = gettimeofday(&tv, NULL);
	ritenow = (double)tv.tv_sec;
	tstmp = atof(parts->timestamp);
	if (ritenow - tstmp > 120) { return -1; } // skip old messages
	// if (tstmp > ritenow) { return -1; }       // skip messages from the future
	return 0;
}

int main (int argc, char** argv) {
  int listening_port = 80;
  if (argc == 2) {
    listening_port = atoi(argv[1]);
  }

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
  const char *hostname = "127.0.0.1";
  struct timeval timeout = { 1, 500000 };
  int port = 6379;
  c = redisConnectWithTimeout(hostname, port, timeout);
  if (c == NULL || c->err) {
      if (c) {
            printf("Error connecting to database: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
  }

  // Run forever:
  while (1) {
    // Receive a message of length `n` < 508 into buffer `msg`,
    // and null-terminate the received message
    n      = recvfrom(sock, msg, 507, 0, NULL, NULL);
    msg[n] = '\0';

    // Parse JSON message into parts.
    msg_parts = (struct msg_parts){NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    if (-1 == parse_msg(&msg_parts, msg, n)) {
      goto err_json;
    }

    // Check the hash to make sure the message has not been forged
    if (-1 == check_hash(&msg_parts))  {
      goto err_signature;
    }

    // Check the timestamp to make sure it is not a replay of an old message 
    if (-1 == check_timestamp(&msg_parts))  {
      goto err_timestamp;
    }

    // Build and send redis queries
    snprintf(value, 508, "%s %s %s %s %s %s",  msg_parts.timestamp, msg_parts.lat,
             msg_parts.lon, msg_parts.status, msg_parts.device, msg_parts.version);
    reply = redisCommand(c, "HSET taxi:%s %s \"%s\"", msg_parts.taxi,
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

   // reply = redisCommand(c, "SADD ips:%s %d\"\r\n",
   //                   msg_parts.operator, client.sin_addr.s_addr);
   // if (REDIS_REPLY_ERROR == reply->type) {
   //      goto err_redis_write;
   // }
   // freeReplyObject(reply);

    continue;

  err_bind:             printf("Error binding socket. You need to be root for ports under 1024.      Exiting...\n"); FLUSH;
    return 1;

  err_redis_connection: printf("Error connecting to database.  Skipping...\n"); FLUSH;
    continue;

err_redis_write:      printf("Error writing to database : %s      Skipping...\n", reply->str); freeReplyObject(reply); FLUSH;
    continue;

  err_timestamp:        printf("Error checking timestamp.      Skipping stale or replayed message...\n"); FLUSH;
    continue;

  err_signature:        printf("Error checking signature.      Skipping forged message...\n"); FLUSH;
    continue;

  err_json:             printf("Error parsing json.            Skipping incorrectly formated message...\n"); FLUSH;
    continue;

  }

}
