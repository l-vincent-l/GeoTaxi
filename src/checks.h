static inline int check_hash (struct msg_parts *parts, char* apikey) {
    char *concatenate = malloc(parts->total_len + strlen(apikey));
    sprintf(concatenate, "%s%s%s%s%s%s%s%s%s", 
        parts->timestamp, parts->operator, parts->taxi,
        parts->lat, parts->lon, parts->device, parts->status,
        parts->version, apikey);
    int r = strcmp(parts->hash, sha1(concatenate));
    free(concatenate);
    return r;
}

