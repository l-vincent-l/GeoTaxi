#include <curl/curl.h>

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if(mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

static void get_users(map_str_t *map, char* http_apikey, char* url_users) {
    if (strcmp(http_apikey, "") == 0) {
        printf("No apikey given\n");
        return;
    }
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if(curl) {
      struct curl_slist *chunk = NULL;
      chunk = curl_slist_append(chunk, "Accept: application/json");
      chunk = curl_slist_append(chunk, "X-VERSION: 2");
      char header_xapi[80];
      sprintf(header_xapi, "X-API-KEY: %s", http_apikey);
      chunk = curl_slist_append(chunk, header_xapi);
      res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
      curl_easy_setopt(curl, CURLOPT_URL, url_users);
      struct MemoryStruct data;
      data.memory = malloc(1);  /* will be grown as needed by the realloc above */
      data.size = 0;    /* no data at this point */
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&data);
      res = curl_easy_perform(curl);
      if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));
        fprintf(stderr, "Url: %s\n", url_users);
        fprintf(stderr, "APIKEY: %s\n", http_apikey);
      } else {
          int vlen;
          char * array= js0n("data", 4, data.memory, data.size, &vlen);
          int i = 0;
          int vlen1 = 0;
          do {
              char *array_str = js0n(0, i, array, vlen, &vlen1);
              if (vlen1 != 0) {
                  char *val, *apikey, *name;
                  int vlen2 = 0;
                  val = js0n("apikey", 6, array_str, vlen, &vlen2);
                  if (vlen2 == 0) { ++i;continue;}
                  apikey = malloc(vlen2+1);
                  strncpy(apikey, val, vlen2);
                  apikey[vlen2] = '\0';

                  val = js0n("name", 4, array_str, vlen, &vlen2);
                  if (vlen2 == 0 || vlen2>100) { ++i;continue;}
                  name = malloc(vlen2+1);
                  strncpy(name, val, vlen2);
                  name[vlen2] = '\0';
                  map_set(map, name, apikey);
              }
              ++i;
          } while (vlen1 != 0 && i<10000);
     }
     curl_easy_cleanup(curl);
     curl_slist_free_all(chunk);
   } else {
     curl_easy_cleanup(curl);
     printf("Unable to get users list from: %s", url_users);
   }
}

char **get_apikey(char* username, map_str_t *map) {
    return map_get(map, username);
}
