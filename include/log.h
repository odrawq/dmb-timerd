#pragma once

#define LOG_DIR "/var/log/" PROGRAM_NAME "/"

#define AUDIT_LOG_FILE LOG_DIR "audit-log"
#define ERROR_LOG_FILE LOG_DIR "error-log"

#define TIMESTAMP_FMT "[%Y-%m-%d %H:%M:%S]"

void init_log(void);
void audit(const char *fmt, ...);
_Noreturn void die(const char *fmt, ...);
