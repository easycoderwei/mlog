#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include "../src/mlog.h"


static const char *msg[] = {
    "test 111",
    "test 222",
    "test 333",
    "test 444",
    "test 555",
};


static inline pid_t
get_thread_id()
{
    return syscall(SYS_gettid);
}


void *
thread_func1(void *arg)
{
    int      count = (int) arg;
    pid_t    tid = get_thread_id();

    mlog_info("thread %d loop count=%d start ...", tid, count);

    do {
        if (count % 2 == 0) {
            mlog_warn("thread %d count=%d", tid, count);
        } else {
            mlog_error("thread %d count=%d", tid, count);
        }

        sleep(1);
    } while (--count > 0);

    mlog_info("thread %d exit ...", tid);
}


void *
thread_func2(void *arg)
{
    int            cnt = 0, index = (int) arg;
    pid_t          tid = get_thread_id();
    const char    *info = msg[index];

    mlog_info("thread %d start ...", tid);

    do {
        switch (cnt % 3) {
        case 1:
            mlog_error("thread %d msg=%s cnt=%d", tid, info, cnt);
            break;
        case 2:
            mlog_warn("thread %d msg=%s cnt=%d", tid, info, cnt);
            break;
        default:
            mlog_info("thread %d msg=%s cnt=%d", tid, info, cnt);
            break;
        }

        sleep(1);
    } while (cnt++ < 3);

    mlog_info("thread %d exit ...", tid);
}

int main(int argc, char **argv)
{
    int                i, last_idx;
    pthread_t          t[5];
    const char        *fmt = "[%d] %s %d";
    pthread_attr_t     attr;

    if (mlog_init(MLOG_LEVEL_INFO, "/tmp/a.log", 2 * 1024 * 1024)) {
        return -1;
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for (i = 0; i < 4; i++) {
        if (pthread_create(&t[i], &attr, thread_func1, (void *) (i + 1))
            != 0)
        {
            break;
        }
    }

    pthread_attr_destroy(&attr);

    for (i = 0; i < 2; i++) {
        mlog_error(fmt, i, msg[i], i * 10);
        mlog_warn(fmt, i, msg[i], i * 10);
        sleep(0.5);
        mlog_info(fmt, i, msg[i], i * 10);
        mlog_debug(fmt, i, msg[i], i * 10);
    }

    mlog_error(">>>>> change log level to debug");

    mlog_set_log_level(MLOG_LEVEL_DEBUG);

    memset(&t, 0, sizeof(t));

    for (i = 0; i < 5; i++) {
        if (pthread_create(&t[i], NULL, thread_func2, (void *) i) != 0) {
            break;
        }
    }

    last_idx = i == 0 ? -1 : i - 1;

    for (i = 0; i < 2; i++) {
        mlog_error(fmt, i, msg[i], i * 10);
        mlog_warn(fmt, i, msg[i], i * 10);
        sleep(1);
        mlog_info(fmt, i, msg[i], i * 10);
        mlog_debug(fmt, i, msg[i], i * 10);
    }

    mlog_info("wait thread exit ...");

    for (i = 0; i <= last_idx; i++) {
        if(pthread_join(t[i], NULL) != 0) {
            printf("wait thread %d failed\n", i);
        }
    }

    mlog_warn("all thread exit, do mlog uinit");

    mlog_uinit();

    return 0;
}
