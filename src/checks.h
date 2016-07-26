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
