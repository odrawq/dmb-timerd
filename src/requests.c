#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "requests.h"

typedef struct
{
    char *data;
    size_t size;
}
response_t;

static CURL *make_curl(void);
static CURLcode perform_request(CURL *curl);
static size_t write_response(void *data, const size_t size, const size_t count, void *response_t_dest);

void init_requests(void)
{
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

cJSON *tg_get_updates(const int_fast64_t update_id)
{
    response_t response = {0};

    char url[MAX_URL_SIZE + 1];
    snprintf(url, sizeof (url), TG_BOT_API_GET_UPDATES_URL_FMT, update_id, MAX_RESPONSE_TIMEOUT);

    CURL *curl = make_curl();
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_response);

    cJSON *updates = 0;

    if (perform_request(curl) == CURLE_OK && !(updates = cJSON_Parse(response.data)))
        die("%s: %s: failed to parse response.data", __FILE__, __func__);

    free(response.data);
    curl_easy_cleanup(curl);
    return updates;
}

int tg_send_message(const int_fast64_t chat_id, const char *keyboard, const char *fmt, ...)
{
    char text[MAX_TG_TEXT_SIZE + 1];

    va_list args;
    va_start(args, fmt);
    vsnprintf(text, sizeof (text), fmt, args);
    va_end(args);

    CURL *curl = make_curl();
    char *escaped_text = curl_easy_escape(curl, text, 0);

    if (!escaped_text)
        die("%s: %s: failed to escape text", __FILE__, __func__);

    char post_fields[MAX_POST_FIELDS_SIZE];
    snprintf(post_fields, sizeof (post_fields), TG_BOT_API_SEND_MESSAGE_POST_FIELDS_FMT, chat_id, keyboard ? keyboard : "", escaped_text);

    curl_easy_setopt(curl, CURLOPT_URL, TG_BOT_API_SEND_MESSAGE_URL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);

    const int code = perform_request(curl) == CURLE_OK ? 0 : 1;

    curl_free(escaped_text);
    curl_easy_cleanup(curl);
    return code;
}

int tg_answer_callback_query(const char *callback_query_id)
{
    char post_fields[MAX_POST_FIELDS_SIZE + 1];
    snprintf(post_fields, sizeof (post_fields), TG_BOT_API_ANSWER_CALLBACK_QUERY_POST_FIELDS_FMT, callback_query_id);

    CURL *curl = make_curl();
    curl_easy_setopt(curl, CURLOPT_URL, TG_BOT_API_ANSWER_CALLBACK_QUERY_URL);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields);

    const int code = perform_request(curl) == CURLE_OK ? 0 : 1;

    curl_easy_cleanup(curl);
    return code;
}

static CURL *make_curl(void)
{
    CURL *curl = curl_easy_init();

    if (!curl)
        die("%s: %s: failed to initialize curl", __FILE__, __func__);

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, MAX_CONNECT_TIMEOUT);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, MAX_RESPONSE_TIMEOUT);
    return curl;
}

static CURLcode perform_request(CURL *curl)
{
    int retries = 0;
    CURLcode code;

    do
        if ((code = curl_easy_perform(curl)) == CURLE_OK)
            break;
    while (++retries < MAX_REQUEST_RETRIES);

    return code;
}

static size_t write_response(void *data, const size_t size, const size_t count, void *response_t_dest)
{
    const size_t bytes = size * count;

    response_t *dest = response_t_dest;
    dest->data = realloc(dest->data, dest->size + bytes + 1);

    if (!dest->data)
        die("%s: %s: failed to reallocate memory for dest->data", __FILE__, __func__);

    memcpy(&(dest->data[dest->size]), data, bytes);
    dest->size += bytes;
    dest->data[dest->size] = 0;

    return bytes;
}
