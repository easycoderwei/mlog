#ifndef __M_LOG_INNER_H__
#define __M_LOG_INNER_H__

#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>


#define MLOG_MAX_LOG_LEN    2048

#define MLOG_ERROR(fmt, args...) \
    do { \
        dprintf(2, "ERROR:tid[%d] %s#%d: " fmt "\n", mlog_thread_tid(), __func__, __LINE__, ## args); \
    } while (0)

#ifdef DEBUG
#define MLOG_DEBUG(fmt, args...) \
    do { \
        dprintf(2, "DEBUG:tid[%d] %s#%d: " fmt "\n", mlog_thread_tid(), __func__, __LINE__, ## args); \
    } while (0)
#else
#define MLOG_DEBUG(fmt, args...) do {} while (0)
#endif


static inline pid_t
mlog_thread_tid()
{
    return syscall(SYS_gettid);
}

int mlog_inner_init(const char *filename, unsigned int buf_size);
void mlog_inner_uinit();
int mlog_get_pid_and_tid(pid_t *pid, pid_t *tid);
int mlog_post_log_task(unsigned long msec, unsigned char *buf,
    unsigned int len);


#endif /* __M_LOG_INNER_H__ */
