#pragma once

#include "config.h" // This file is created after configure.sh has been successfully executed.
#include "emoji.h"

#define CMD_START "/start"
#define CMD_TIMER "/timer"
#define CMD_ROFL  "/rofl"

#define MSG_MAINTENANCE EMOJI_FAILURE  " <b>Бот недоступен</b>\n\n" \
                                       "Повторите попытку позже"

#define MSG_UNKNOWN_CMD EMOJI_FAILURE  " <b>Неизвестная команда</b>"

#define MSG_START       EMOJI_GREETING " <b>Здарова</b>\n\n" \
                                       "Узнай сколько тебе осталось до дембеля"

#define MSG_TIMER_FMT                  "%s <b>До дембеля осталось</b>\n\n" \
                                       "<u>%dд</u>\n" \
                                       "Прошло %dд (%d%)"

#define MSG_SOON        EMOJI_SOLDIER  "<b>До дембеля осталось</b>\n\n" \
                                       "Меньше одного дня"

#define MSG_END         EMOJI_OK       " <b>Рассос мужики</b>"

#define MSG_ROFL        EMOJI_OK       " <b>Спец. контракт был успешно подписан</b>\n\n" \
                                       "<u>2/2</u> - 2 дня на стороне Российской Федерации, и 2 дня на стороне Украины!\n\n" \
                                       "С вами свяжется наш специалист, ожидайте пожалуйста"

#define MAINTENANCE_INLINE_KEYBOARD "{\"inline_keyboard\":[" \
                                    "[{\"text\":\"Повторить\",\"callback_data\":\"" CMD_START "\"}]" \
                                    "]}"

#define TIMER_INLINE_KEYBOARD       "{\"inline_keyboard\":[" \
                                    "[{\"text\":\"" EMOJI_TIMER " Таймер" "\",\"callback_data\":\"" CMD_TIMER "\"}]" \
                                    "]}"

#define has_target_access(chat_id) (chat_id == TG_TARGET_CHAT_ID)

_Noreturn void start_bot(void);
