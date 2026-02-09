#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bot.h"
#include "log.h"
#include "requests.h"
#include "version.h"

#define LOCK_DIR  "/var/run/" PROGRAM_NAME "/"
#define LOCK_FILE LOCK_DIR PROGRAM_NAME ".lock"

#define UMASK 0007

static struct passwd *usr = 0;

int maintenance_mode = 0;

static void handle_args(int argc, char **argv);
static void init_usr(void);
static void lock_instance(void);
static void drop_privileges(void);
static void daemonize(void);

_Noreturn static void exit_success(const char *fmt, ...);
_Noreturn static void exit_failure(const char *fmt, ...);

int main(int argc, char **argv)
{
    handle_args(argc, argv);
    init_usr();
    umask(UMASK);
    lock_instance();
    drop_privileges();
    daemonize();
    init_requests();

    if (!maintenance_mode)
        init_log();

    start_bot();
}

static void handle_args(int argc, char **argv)
{
    if (argc == 1)
        return;

    const struct option long_options[] =
    {
        {"help",        no_argument, 0, 'h'},
        {"version",     no_argument, 0, 'v'},
        {"maintenance", no_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    opterr = 0;
    int opt;

    while ((opt = getopt_long(argc, argv, "+hvm", long_options, 0)) != -1)
    {
        switch (opt)
        {
            case 'h':
                exit_success("Usage: %s [option]\n\n"
                             "Options:\n"
                             "  -h, --help           show this help and exit\n"
                             "  -v, --version        show version and exit\n"
                             "  -m, --maintenance    run in maintenance mode\n"
                             "\nTo run %s, run it as root",
                             PROGRAM_NAME,
                             PROGRAM_NAME);

            case 'v':
                exit_success("%s %d.%d.%d",
                             PROGRAM_NAME, MAJOR_VERSION, MINOR_VERSION, PATCH_VERSION);

            case 'm':
                maintenance_mode = 1;
                break;

            case '?':
                if (optopt)
                    exit_failure("Unknown option '-%c'\n"
                                 "Try '%s -h' for more information",
                                 optopt,
                                 PROGRAM_NAME);
                else
                    exit_failure("Unknown option '%s'\n"
                                 "Try '%s -h' for more information",
                                 argv[optind - 1],
                                 PROGRAM_NAME);
        }
    }

    if (optind < argc)
        exit_failure("Expecting options only\n"
                     "Try '%s -h' for more information",
                     PROGRAM_NAME);
}

static void init_usr(void)
{
    if (!(usr = getpwnam(PROGRAM_NAME)))
        exit_failure("Failed to get %s user data", PROGRAM_NAME);
}

static void lock_instance(void)
{
    if (access(LOCK_DIR, F_OK))
    {
        if (mkdir(LOCK_DIR, 0777))
            exit_failure("Failed to create %s", LOCK_DIR);

        if (chown(LOCK_DIR, usr->pw_uid, usr->pw_gid))
            exit_failure("Failed to change %s ownership", LOCK_DIR);
    }

    int fd = open(LOCK_FILE, O_CREAT | O_RDWR | O_EXCL, 0666);

    if (fd >= 0)
    {
        if (fchown(fd, usr->pw_uid, usr->pw_gid))
            exit_failure("Failed to change %s ownership", LOCK_FILE);
    }
    else if (errno == EEXIST)
    {
        if ((fd = open(LOCK_FILE, O_RDWR)) < 0)
            exit_failure("Failed to open %s", LOCK_FILE);
    }
    else
        exit_failure("Failed to open or create %s", LOCK_FILE);

    if (flock(fd, LOCK_EX | LOCK_NB))
    {
        if (errno == EAGAIN)
            exit_failure("%s is already running", PROGRAM_NAME);
        else
            exit_failure("Failed to lock %s", LOCK_FILE);
    }
}

static void drop_privileges(void)
{
    if (setgid(usr->pw_gid))
        exit_failure("Failed to set %s gid", PROGRAM_NAME);

    if (setuid(usr->pw_uid))
        exit_failure("Failed to set %s uid", PROGRAM_NAME);
}

static void daemonize(void)
{
    if (daemon(0, 0))
        exit_failure("Failed to daemonize");
}

_Noreturn static void exit_success(const char *fmt, ...)
{
    if (fmt)
    {
        va_list args;
        va_start(args, fmt);

        vfprintf(stdout, fmt, args);
        fputc('\n', stdout);

        va_end(args);
    }

    exit(0);
}

_Noreturn static void exit_failure(const char *fmt, ...)
{
    if (fmt)
    {
        va_list args;
        va_start(args, fmt);

        fputs("\033[0;31;1m", stderr);
        vfprintf(stderr, fmt, args);
        fputs("\033[0m\n", stderr);

        va_end(args);
    }

    exit(1);
}
