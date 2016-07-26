#include "msg_parts.h"

static inline int parse_msg (struct msg_parts *parts, char *msg, int n) {
   int len=0;
   parts->total_len = 0;
   parts->operator = js0n("operator",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->operator_len = len;
   parts->total_len += len;
   parts->version = js0n("version",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->version_len = len;
   parts->total_len += len;
   parts->taxi = js0n("taxi",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->taxi_len = len;
   parts->total_len += len;
   parts->lat = js0n("lat",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->lat_len = len;
   parts->total_len += len;
   parts->lon = js0n("lon",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->lon_len = len;
   parts->total_len += len;
   parts->timestamp = js0n("timestamp",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->timestamp_len = len;
   parts->total_len += len;
   parts->status = js0n("status",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->status_len = len;
   parts->total_len += len;
   parts->device = js0n("device",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->device_len = len;
   parts->total_len += len;
   parts->hash = js0n("hash",0,msg,n,&len);
   if (0 == len) { return -1; }
   parts->hash_len = len;
   parts->total_len += len;

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
