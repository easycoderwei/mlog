#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>
#include "mlog.h"
#include "mlog_inner.h"


static int      g_log_level;

static const char *err_levels[] = {
    "error",
    "warn",
    "info",
    "debug"
};


int
mlog_init(int level, const char *filename, unsigned int buf_size)
{
    if (level < MLOG_LEVEL_ERROR && level > MLOG_LEVEL_DEBUG) {
        MLOG_ERROR("log level %d invalid", level);
        return -1;
    }

    if (mlog_inner_init(filename, buf_size) != 0) {
        return -1;
    }

    g_log_level = level;

    return 0;
}


void
mlog_uinit()
{
    mlog_inner_uinit();
}


void
mlog_set_log_level(int level)
{
    if (level >= MLOG_LEVEL_ERROR && level <= MLOG_LEVEL_DEBUG) {
        g_log_level = level;
    }
}


void
mlog_format(int level, const char *func, long line, const char *fmt, ...)
{
    int                  len;
    char                *p, *last, buf[MLOG_MAX_LOG_LEN] = { 0 };
    pid_t                pid, tid;
    va_list              arglist;
    struct tm            tm;
    struct timeval       tv;

    if (g_log_level < level) {
        return;
    }

    if (mlog_get_pid_and_tid(&pid, &tid) != 0) {
        MLOG_ERROR("get_pid_and_tid failed");
        return;
    }

    gettimeofday(&tv, NULL);
    localtime_r(&tv.tv_sec, &tm);

    p = buf;
    last = buf + MLOG_MAX_LOG_LEN;

    len = snprintf(p, last - p,
                   "%4d/%02d/%02d %02d:%02d:%02d [%s] %d#%d %s#%ld: ",
                   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                   tm.tm_hour, tm.tm_min, tm.tm_sec, err_levels[level],
                   pid, tid, func, line);
    if (len <= 0) {
        MLOG_ERROR("snprintf failed ret=%d", len);
        return;
    }

    p += len;

    va_start(arglist, fmt);
    len = vsnprintf(p, last - p, fmt, arglist);
    va_end(arglist);

    if (len <= 0) {
        MLOG_ERROR("vsnprintf failed ret=%d", len);
        return;
    }

    p += len;
    *p++ = '\n';
    len = p - buf;

    if (mlog_post_log_task(tv.tv_sec * 1000 + tv.tv_usec / 1000, buf, len) != 0)
    {
        MLOG_ERROR("post_log_task failed");
    }
}
