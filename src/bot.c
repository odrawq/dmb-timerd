#include <cjson/cJSON.h>
#include <inttypes.h>
#include <pthread.h>
#include <string.h>

#include "bot.h"
#include "config.h" // This file is created after configure.sh has been successfully executed.
#include "log.h"
#include "requests.h"
#include "dmb-timer.h"

extern int maintenance_mode;

static void handle_updates(cJSON *updates, int_fast64_t *last_update_id);
static void *handle_message(void *cJSON_message);
static void *handle_callback_query(void *cJSON_callback_query);

static void handle_maintenance_mode(void);

static void handle_cmd(const int_fast64_t chat_id, const char *cmd);
static void handle_cmd_start(void);
static void handle_cmd_timer(void);
static void handle_unknown_cmd(void);

_Noreturn void start_bot(void)
{
    int_fast64_t last_update_id = 0;

    for (;;)
    {
        cJSON *updates = tg_get_updates(last_update_id);

        if (updates)
        {
            handle_updates(updates, &last_update_id);
            cJSON_Delete(updates);
        }
    }
}

static void handle_updates(cJSON *updates, int_fast64_t *last_update_id)
{
    const cJSON *result = cJSON_GetObjectItem(updates, "result");
    const int size = cJSON_GetArraySize(result);

    if (!size)
        return;

    cJSON *item = 0, *message = 0, *callback_query = 0;

    for (int i = 0; i < size; ++i)
    {
        item = cJSON_GetArrayItem(result, i);

        if ((message = cJSON_GetObjectItem(item, "message")))
        {
            pthread_t handle_message_thread;

            if (pthread_create(&handle_message_thread, 0, handle_message, cJSON_Duplicate(message, 1)))
                die("%s: %s: failed to create handle_message_thread", __FILE__, __func__);

            pthread_detach(handle_message_thread);
        }
        else if ((callback_query = cJSON_GetObjectItem(item, "callback_query")))
        {
            pthread_t handle_callback_query_thread;

            if (pthread_create(&handle_callback_query_thread, 0, handle_callback_query, cJSON_Duplicate(callback_query, 1)))
                die("%s: %s: failed to create handle_callback_query_thread", __FILE__, __func__);

            pthread_detach(handle_callback_query_thread);
        }
    }

    const cJSON *update_id = cJSON_GetObjectItem(item, "update_id");
    *last_update_id = (int_fast64_t) update_id->valuedouble + 1;
}

static void *handle_message(void *cJSON_message)
{
    cJSON *message = cJSON_message;

    const cJSON *message_chat = cJSON_GetObjectItem(message, "chat");
    const cJSON *message_chat_id = cJSON_GetObjectItem(message_chat, "id");

    const int_fast64_t chat_id = (int_fast64_t) message_chat_id->valuedouble;

    if (!has_target_access(chat_id))
        goto exit;

    if (maintenance_mode)
    {
        handle_maintenance_mode();
        goto exit;
    }

    const cJSON *message_text = cJSON_GetObjectItem(message, "text");
    const char *text = message_text ? message_text->valuestring : 0;

    if (text && *text == '/')
        handle_cmd(chat_id, text);

exit:
    cJSON_Delete(message);
    return 0;
}

static void *handle_callback_query(void *cJSON_callback_query)
{
    cJSON *callback_query = cJSON_callback_query;

    const cJSON *callback_query_message = cJSON_GetObjectItem(callback_query, "message");
    const cJSON *callback_query_message_chat = cJSON_GetObjectItem(callback_query_message, "chat");
    const cJSON *callback_query_message_chat_id = cJSON_GetObjectItem(callback_query_message_chat, "id");

    const int_fast64_t chat_id = (int_fast64_t) callback_query_message_chat_id->valuedouble;

    if (!has_target_access(chat_id))
        goto exit;

    const cJSON *callback_query_id = cJSON_GetObjectItem(callback_query, "id");
    tg_answer_callback_query(callback_query_id->valuestring);

    if (maintenance_mode)
    {
        handle_maintenance_mode();
        goto exit;
    }

    const cJSON *callback_query_data = cJSON_GetObjectItem(callback_query, "data");
    handle_cmd(chat_id, callback_query_data->valuestring);

exit:
    cJSON_Delete(callback_query);
    return 0;
}

static void handle_maintenance_mode(void)
{
    tg_send_message(TG_TARGET_CHAT_ID, MAINTENANCE_INLINE_KEYBOARD, MSG_MAINTENANCE);
}

static void handle_cmd(const int_fast64_t chat_id, const char *cmd)
{
    audit("Command received (chat_id: %" PRIdFAST64 ", cmd: %s)", chat_id, cmd);

    if (!strcmp(cmd, CMD_START))
        handle_cmd_start();
    else if (!strcmp(cmd, CMD_TIMER))
        handle_cmd_timer();
    else
        handle_unknown_cmd();
}

static void handle_cmd_start(void)
{
    tg_send_message(TG_TARGET_CHAT_ID, TIMER_INLINE_KEYBOARD, MSG_START);
}

static void handle_cmd_timer(void)
{
    dmb_timer_t timer;

    if (get_dmb_timer(&timer))
        tg_send_message(TG_TARGET_CHAT_ID, 0, MSG_END);
    else if (!timer.days_left)
        tg_send_message(TG_TARGET_CHAT_ID, TIMER_INLINE_KEYBOARD, MSG_SOON);
    else
        tg_send_message(TG_TARGET_CHAT_ID, TIMER_INLINE_KEYBOARD, MSG_TIMER_FMT,
                        timer.days_left, timer.days_passed, timer.percent);
}

static void handle_unknown_cmd(void)
{
    tg_send_message(TG_TARGET_CHAT_ID, 0, MSG_UNKNOWN_CMD);
}
