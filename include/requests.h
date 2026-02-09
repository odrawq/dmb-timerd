#pragma once

#include <cjson/cJSON.h>
#include <inttypes.h>

#include "config.h" // This file is created after configure.sh has been successfully executed.

#define TG_BOT_API_URL "https://api.telegram.org/bot" TG_BOT_API_TOKEN "/"

#define TG_BOT_API_GET_UPDATES_URL_FMT       TG_BOT_API_URL "getUpdates?offset=%" PRIdFAST64 "&timeout=%d&allowed_updates=[\"message\",\"callback_query\"]"
#define TG_BOT_API_SEND_MESSAGE_URL          TG_BOT_API_URL "sendMessage"
#define TG_BOT_API_ANSWER_CALLBACK_QUERY_URL TG_BOT_API_URL "answerCallbackQuery"

#define TG_BOT_API_SEND_MESSAGE_POST_FIELDS_FMT          "chat_id=%" PRIdFAST64 "&reply_markup=%s&text=%s&parse_mode=HTML&disable_web_page_preview=true"
#define TG_BOT_API_ANSWER_CALLBACK_QUERY_POST_FIELDS_FMT "callback_query_id=%s"

#define MAX_TG_TEXT_SIZE     4096
#define MAX_POST_FIELDS_SIZE 8192
#define MAX_URL_SIZE         1024

#define MAX_RESPONSE_TIMEOUT 15
#define MAX_CONNECT_TIMEOUT  10

#define MAX_REQUEST_RETRIES 3

void init_requests(void);
cJSON *tg_get_updates(const int_fast64_t update_id);
int tg_send_message(const int_fast64_t chat_id, const char *keyboard, const char *fmt, ...);
int tg_answer_callback_query(const char *callback_query_id);
