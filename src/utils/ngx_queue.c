
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

#include "ngx_queue.h"

/*
 * find the middle queue element if the queue has odd number of elements
 * or the first element of the queue's second part otherwise
 */

ngx_queue_t *
ngx_queue_middle(ngx_queue_t *queue)
{
    ngx_queue_t  *middle, *next;

    middle = ngx_queue_head(queue);

    if (middle == ngx_queue_last(queue)) {
        return middle;
    }

    next = ngx_queue_head(queue);

    for ( ;; ) {
        middle = ngx_queue_next(middle);

        next = ngx_queue_next(next);

        if (next == ngx_queue_last(queue)) {
            return middle;
        }

        next = ngx_queue_next(next);

        if (next == ngx_queue_last(queue)) {
            return middle;
        }
    }
}


/* the stable insertion sort */

void
ngx_queue_sort(ngx_queue_t *queue,
    int (*cmp)(const ngx_queue_t *, const ngx_queue_t *))
{
    ngx_queue_t  *q, *prev, *next;

    q = ngx_queue_head(queue);

    if (q == ngx_queue_last(queue)) {
        return;
    }

    for (q = ngx_queue_next(q); q != ngx_queue_sentinel(queue); q = next) {

        prev = ngx_queue_prev(q);
        next = ngx_queue_next(q);

        ngx_queue_remove(q);

        do {
            if (cmp(prev, q) <= 0) {
                break;
            }

            prev = ngx_queue_prev(prev);

        } while (prev != ngx_queue_sentinel(queue));

        ngx_queue_insert_after(prev, q);
    }
}


void
ngx_queue_insert_in_ascending_order(ngx_queue_t *h, ngx_queue_t *x,
    int (*cmp)(const ngx_queue_t *, const ngx_queue_t *))
{
    ngx_queue_t  *q;

    if (ngx_queue_empty(h)) {
        ngx_queue_insert_head(h, x);
        return;
    }

    for (q = ngx_queue_last(h);
         q != ngx_queue_sentinel(h);
         q = ngx_queue_prev(q))
    {
        if (cmp(x, q) >= 0) {
            ngx_queue_insert_after(q, x);
            return;
        }
    }

    ngx_queue_insert_head(h, x);
}
