#ifndef __M_LOG_H__
#define __M_LOG_H__


#define MLOG_LEVEL_ERROR    0
#define MLOG_LEVEL_WARN     1
#define MLOG_LEVEL_INFO     2
#define MLOG_LEVEL_DEBUG    3


#define mlog_error(args...) \
    mlog_format(MLOG_LEVEL_ERROR, __func__, __LINE__, args)
#define mlog_warn(args...) \
    mlog_format(MLOG_LEVEL_WARN, __func__, __LINE__, args)
#define mlog_info(args...) \
    mlog_format(MLOG_LEVEL_INFO, __func__, __LINE__, args)
#define mlog_debug(args...) \
    mlog_format(MLOG_LEVEL_DEBUG, __func__, __LINE__, args)


void mlog_format(int level, const char *func, long line, const char *fmt, ...);
void mlog_set_log_level(int level);
int mlog_init(int level, const char *filename, unsigned int buf_size);
void mlog_uinit();


#endif /* __M_LOG_H__ */
