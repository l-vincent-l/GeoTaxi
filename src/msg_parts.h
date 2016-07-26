#ifndef MSGPARTS
#define MSGPARTS

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
    hash_len,
    total_len;
};
#endif
