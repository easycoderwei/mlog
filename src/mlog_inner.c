#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "util/hash.h"
#include "util/kfifo.h"
#include "util/ngx_queue.h"
#include "mlog_inner.h"


#define  MLOG_MAX_FREE_LIST_SIZE    1024


typedef unsigned long                  mlog_atomic_uint_t;
typedef volatile mlog_atomic_uint_t    mlog_atomic_t;


typedef struct {
    hash_link                  hlnk;
    pid_t                      pid;
    pid_t                      tid;
    struct kfifo              *kfifo_buf;
    mlog_atomic_t              refer;
} mlog_thread_local_data_t;


typedef struct {
    hash_table                *table;
    pthread_mutex_t            mutex;
    unsigned int               kfifo_buf_size;
} mlog_thread_data_t;


typedef struct {
    ngx_queue_t                q;
    unsigned long              msec;
    unsigned int               msg_len;
    mlog_thread_local_data_t  *data;
} mlog_task_t;


typedef struct {
    pthread_t                  tid;
    int                        fd;
    const char                *filename;
    ngx_queue_t                task_list;
    ngx_queue_t                free_list;
    unsigned int               free_count;
    pthread_cond_t             cond;
    pthread_mutex_t            mutex;
    volatile int               active;
} mlog_async_job_t;


static void mlog_destroy_pkey();
static mlog_thread_local_data_t *mlog_get_thread_data();
static inline void mlog_release_free_list();
static void mlog_do_write_log(ngx_queue_t *task_list, int active);
static inline mlog_atomic_t mlog_increase_refer(mlog_thread_local_data_t *data);
static inline mlog_atomic_t mlog_decrease_refer(mlog_thread_local_data_t *data);
static void mlog_decrease_refer_and_try_release(mlog_thread_local_data_t *data);


static pthread_key_t           mlog_pkey;
static mlog_async_job_t        async_job;
static mlog_thread_data_t      thread_data;


static void __attribute__((constructor))
mlog_constructor()
{
    MLOG_DEBUG("constructor");
    pthread_key_create(&mlog_pkey, mlog_destroy_pkey);
    async_job.active = 0;
    async_job.free_count = 0;
    ngx_queue_init(&async_job.task_list);
    ngx_queue_init(&async_job.free_list);
    pthread_mutex_init(&async_job.mutex, NULL);
    pthread_cond_init(&async_job.cond, NULL);
    pthread_mutex_init(&thread_data.mutex, NULL);
}


static void __attribute__((destructor))
mlog_destructor()
{
    MLOG_DEBUG("destructor");
    pthread_mutex_destroy(&thread_data.mutex);
    pthread_cond_destroy(&async_job.cond);
    pthread_mutex_destroy(&async_job.mutex);
    pthread_key_delete(mlog_pkey);
}


int
mlog_get_pid_and_tid(pid_t *pid, pid_t *tid)
{
    mlog_thread_local_data_t    *data;

    data = mlog_get_thread_data();
    if (data == NULL) {
        return -1;
    }

    *pid = data->pid;
    *tid = data->tid;

    return 0;
}


static int
mlog_queue_cmp(const ngx_queue_t *a, const ngx_queue_t *b)
{
    mlog_task_t  *task_a = (mlog_task_t *) a;
    mlog_task_t  *task_b = (mlog_task_t *) b;

    if (task_a->msec > task_b->msec) {
        return 1;

    } else if (task_a->msec < task_b->msec) {
        return -1;

    } else {
        return 0;
    }
}


int
mlog_post_log_task(unsigned long msec, unsigned char *buf, unsigned int len)
{
    int                          ret;
    ngx_queue_t                 *q;
    mlog_task_t                 *task;
    unsigned int                 wlen, remain_len;
    mlog_atomic_t                old_refer;
    mlog_thread_local_data_t    *data;

    data = mlog_get_thread_data();
    if (data == NULL) {
        return -1;
    }

    remain_len = thread_data.kfifo_buf_size - kfifo_len(data->kfifo_buf);
    if (remain_len < len) {
        MLOG_ERROR("fifo buf not enough, msg_len=%u remain=%u",
                   len, remain_len);
        return - 1;
    }

    wlen = kfifo_put(data->kfifo_buf, buf, len);

    old_refer = mlog_increase_refer(data);

    MLOG_DEBUG("post task msg_len=%d refer=%lu old_refer=%lu",
               len, data->refer, old_refer);

    pthread_mutex_lock(&async_job.mutex);

    if (ngx_queue_empty(&async_job.free_list)) {
        task = calloc(1, sizeof(mlog_task_t));
        if (task == NULL) {
            MLOG_ERROR("calloc mlog_task_t failed");
            pthread_mutex_unlock(&async_job.mutex);
            return -1;
        }

    } else {
        q = ngx_queue_head(&async_job.free_list);
        ngx_queue_remove(q);

        task = ngx_queue_data(q, mlog_task_t, q);

        async_job.free_count--;

        MLOG_DEBUG("remove node from free_list count=%d", async_job.free_count);
    }

    task->msec = msec;
    task->data = data;
    task->msg_len = wlen;

    ngx_queue_insert_in_ascending_order(&async_job.task_list, &task->q,
                                        mlog_queue_cmp);

    ret = pthread_cond_signal(&async_job.cond);
    if (ret != 0) {
        MLOG_ERROR("pthread_cond_signal failed ret=%d", ret);
    }

    pthread_mutex_unlock(&async_job.mutex);

    return 0;
}


static void *
mlog_async_write_log(void *arg)
{
    int              ret;
    ngx_queue_t     *q, tasks;
    mlog_task_t     *task;

    MLOG_DEBUG("start async job ...");

    for (;;) {
        ngx_queue_init(&tasks);

        pthread_mutex_lock(&async_job.mutex);

        while (ngx_queue_empty(&async_job.task_list) && async_job.active) {
            MLOG_DEBUG("pthread_cond_wait");
            ret = pthread_cond_wait(&async_job.cond, &async_job.mutex);
            if (ret != 0) {
                MLOG_ERROR("pthread_cond_wait failed ret=%d", ret);
            }
        }

        MLOG_DEBUG("recv cond signal");

        if (!ngx_queue_empty(&async_job.task_list)) {
            q = ngx_queue_head(&async_job.task_list);
            ngx_queue_split(&async_job.task_list, q, &tasks);
        }

        pthread_mutex_unlock(&async_job.mutex);

        if (!ngx_queue_empty(&tasks)) {
            mlog_do_write_log(&tasks, async_job.active);
        }

        if (!async_job.active) {
            mlog_release_free_list();
            break;
        }
    }

    if (async_job.fd >= 0) {
        close(async_job.fd);
        async_job.fd = -1;
    }

    MLOG_DEBUG("exit async job !!!");
}


static inline void
mlog_release_free_list()
{
    ngx_queue_t    *q, *next;
    mlog_task_t    *task;

    MLOG_DEBUG("recv exit signal");

    pthread_mutex_lock(&async_job.mutex);

    for (q = ngx_queue_head(&async_job.free_list);
         q != ngx_queue_sentinel(&async_job.free_list);
         q = next)
    {
        next = ngx_queue_next(q);
        ngx_queue_remove(q);

        task = ngx_queue_data(q, mlog_task_t, q);
        free(task);

        MLOG_DEBUG("release free_list item");
    }

    async_job.free_count = 0;

    pthread_mutex_unlock(&async_job.mutex);
}


static inline void
mlog_try_reuse_and_check_release(ngx_queue_t *free_tasks)
{
    ngx_queue_t    *q, *next;
    mlog_task_t    *task;

    pthread_mutex_lock(&async_job.mutex);

    for (q = ngx_queue_head(free_tasks);
         async_job.free_count < MLOG_MAX_FREE_LIST_SIZE
         && q != ngx_queue_sentinel(free_tasks);
         q = next)
    {
        next = ngx_queue_next(q);
        ngx_queue_remove(q);

        ngx_queue_insert_tail(&async_job.free_list, q);
        async_job.free_count++;

        MLOG_DEBUG("add node to free_list count=%d", async_job.free_count);
    }

    pthread_mutex_unlock(&async_job.mutex);

    for (q = ngx_queue_head(free_tasks);
         q != ngx_queue_sentinel(free_tasks);
         q = next)
    {
        next = ngx_queue_next(q);
        ngx_queue_remove(q);

        task = ngx_queue_data(q, mlog_task_t, q);
        free(task);

        MLOG_DEBUG("release redundant task");
    }
}


static void
mlog_do_write_log(ngx_queue_t *task_list, int active)
{
    pid_t                        tid;
    ssize_t                      wlen;
    mlog_task_t                 *task;
    ngx_queue_t                 *q, *next, free_tasks;
    unsigned int                 len;
    unsigned char                buf[MLOG_MAX_LOG_LEN];
    mlog_thread_local_data_t    *data;

    ngx_queue_init(&free_tasks);

    for (q = ngx_queue_head(task_list);
         q != ngx_queue_sentinel(task_list);
         q = next)
    {
        next = ngx_queue_next(q);

        ngx_queue_remove(q);
        task = ngx_queue_data(q, mlog_task_t, q);

        data = task->data;

        tid = data->tid;

        len = kfifo_get(data->kfifo_buf, buf, task->msg_len);

        wlen = write(async_job.fd, buf, len);

        MLOG_DEBUG("task tid=%d msg_len=%d wlen=%ld",
                   tid, task->msg_len, wlen);

        mlog_decrease_refer_and_try_release(data);

        if (active) {
            ngx_queue_insert_tail(&free_tasks, &task->q);
        } else {
            free(task);
        }

        if (wlen < 0 || wlen < len) {
            /* TODO: save data to retry list if write failed */
            MLOG_ERROR("task tid=%d write log failed len=%d wlen=%ld",
                       tid, len, wlen);
            break;
        }
    }

    if (!ngx_queue_empty(&free_tasks)) {
        mlog_try_reuse_and_check_release(&free_tasks);
    }
}


static mlog_thread_local_data_t *
mlog_get_thread_data()
{
    mlog_thread_local_data_t    *data;

    data = pthread_getspecific(mlog_pkey);
    if (data) {
        return data;
    }

    data = calloc(1, sizeof(mlog_thread_local_data_t));
    if (data == NULL) {
        MLOG_ERROR("calloc mlog_thread_local_data_t failed");
        return NULL;
    }

    data->kfifo_buf = kfifo_alloc(thread_data.kfifo_buf_size);
    if (data->kfifo_buf == NULL) {
        MLOG_ERROR("kfifo_alloc failed");
        free(data);
        return NULL;
    }

    data->pid = getpid();
    data->tid = mlog_thread_tid();

    data->hlnk.key = &data->tid;

    data->refer = 1;

    pthread_setspecific(mlog_pkey, data);

    pthread_mutex_lock(&thread_data.mutex);
    hash_join(thread_data.table, &data->hlnk);
    pthread_mutex_unlock(&thread_data.mutex);

    MLOG_DEBUG("tid %d refer=%lu", data->tid, data->refer);

    return data;
}


static inline mlog_atomic_t
mlog_increase_refer(mlog_thread_local_data_t *data)
{
    return __sync_fetch_and_add(&data->refer, 1);
}

static inline mlog_atomic_t
mlog_decrease_refer(mlog_thread_local_data_t *data)
{
    return __sync_fetch_and_add(&data->refer, -1);
}


static void
mlog_decrease_refer_and_try_release(mlog_thread_local_data_t *data)
{
    mlog_atomic_t  old_refer = mlog_decrease_refer(data);

    MLOG_DEBUG("tid=%d refer=%lu old_refer=%lu",
               data->tid, data->refer, old_refer);

    if (old_refer == 1) {
        MLOG_DEBUG("clear thread %d data", data->tid);
        pthread_mutex_lock(&thread_data.mutex);
        hash_remove_link(thread_data.table, &data->hlnk);
        kfifo_free(data->kfifo_buf);
        free(data);
        pthread_mutex_unlock(&thread_data.mutex);
    }
}


/* execute after thread func exit */
static void
mlog_destroy_pkey()
{
    pid_t                        tid = mlog_thread_tid();
    mlog_atomic_t                old_refer;
    mlog_thread_local_data_t    *data;

    if (thread_data.table == NULL) {
        return;
    }

    pthread_mutex_lock(&thread_data.mutex);

    data = hash_lookup(thread_data.table, &tid);
    if (data == NULL) {
        pthread_mutex_unlock(&thread_data.mutex);
        return;
    }

    old_refer = mlog_decrease_refer(data);

    MLOG_DEBUG("tid %d refer=%lu old_refer=%lu",
               data->tid, data->refer, old_refer);

    if (old_refer == 1) {
        MLOG_DEBUG("clear thread %d data", data->tid);
        hash_remove_link(thread_data.table, &data->hlnk);
        kfifo_free(data->kfifo_buf);
        free(data);
    }

    pthread_mutex_unlock(&thread_data.mutex);
}


static unsigned int
mlog_tid_hash(const void *key, unsigned int size)
{
    pid_t  tid = *(pid_t *) key;

    return tid % size;
}


static int
mlog_tid_hash_cmp(const void *a, const void *b)
{
    pid_t  tid_a = *(pid_t *) a;
    pid_t  tid_b = *(pid_t *) b;

    if (tid_a > tid_b) {
        return 1;

    } else if (tid_a < tid_b) {
        return -1;

    } else {
        return 0;
    }
}


int
mlog_inner_init(const char *filename, unsigned int buf_size)
{
    int ret;

    if (buf_size & (buf_size - 1)) {
        MLOG_ERROR("buf_size must be 2^n, invalid %d", buf_size);
        goto _fail;
    }

    thread_data.table = hash_create(mlog_tid_hash_cmp, 103, mlog_tid_hash);
    if (thread_data.table == NULL) {
        MLOG_ERROR("create thread_data failed");
        goto _fail;
    }

    thread_data.kfifo_buf_size = buf_size;

    async_job.fd = open(filename, O_WRONLY|O_APPEND|O_CREAT, 0644);
    if (async_job.fd < 0) {
        MLOG_ERROR("open file %s failed", filename);
        goto _fail;
    }

    async_job.filename = filename;
    async_job.active = 1;

    ret = pthread_create(&async_job.tid, NULL, mlog_async_write_log, NULL);
    if (ret != 0) {
        MLOG_ERROR("create async job failed, ret=%d", ret);
        goto _fail;
    }

    return 0;

_fail:

    if (thread_data.table) {
        hashFreeMemory(thread_data.table);
    }

    if (async_job.fd >= 0) {
        close(async_job.fd);
        async_job.fd = -1;
    }

    async_job.active = 0;

    return -1;
}


void
mlog_inner_uinit()
{
    async_job.active = 0;

    __sync_synchronize();

    if (pthread_cond_signal(&async_job.cond) != 0) {
        MLOG_ERROR("pthread_cond_signal failed");
    }

    if (pthread_join(async_job.tid, NULL) != 0) {
        MLOG_ERROR("wait async job exit failed");
    }
}
