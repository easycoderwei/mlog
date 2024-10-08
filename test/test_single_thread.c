#include <stdio.h>
#include "../src/mlog.h"


static const char *msg[] = {
    "test 111",
    "test 222",
    "test 333",
    "test 444",
};


int main(int argc, char **argv)
{
    int            i;
    const char    *fmt = "[%d] %s %d";

    if (mlog_init(MLOG_LEVEL_ERROR, "/tmp/a.log", 2 * 1024 * 1024)) {
        return -1;
    }

    for (i = 0; i < 4; i++) {
        mlog_error(fmt, i, msg[i], i * 10);
        mlog_warn(fmt, i, msg[i], i * 10);
        mlog_info(fmt, i, msg[i], i * 10);
        mlog_debug(fmt, i, msg[i], i * 10);
    }

    mlog_error(">>>>> change log level to debug");

    mlog_set_log_level(MLOG_LEVEL_DEBUG);

    for (i = 0; i < 4; i++) {
        mlog_error(fmt, i, msg[i], i * 10);
        mlog_warn(fmt, i, msg[i], i * 10);
        mlog_info(fmt, i, msg[i], i * 10);
        mlog_debug(fmt, i, msg[i], i * 10);
    }

    mlog_uinit();

    return 0;
}
