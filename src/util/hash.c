
/*
 * $Id: hash.c,v 1.17 2006/06/02 17:32:44 serassio Exp $
 *
 * DEBUG: section 0     Hash Tables
 * AUTHOR: Harvest Derived
 *
 * SQUID Web Proxy Cache          http://www.squid-cache.org/
 * ----------------------------------------------------------
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */

#include <string.h>
#include <malloc.h>
#include <math.h>
#include <assert.h>
#include "hash.h"

static void hash_next_bucket(hash_table * hid);

unsigned int
hash_string(const void *data, unsigned int size)
{
    const char *s = data;
    unsigned int n = 0;
    unsigned int j = 0;
    unsigned int i = 0;
    while (*s) {
	j++;
	n ^= 271 * (unsigned) *s++;
    }
    i = n ^ (j * 271);
    return i % size;
}

/* the following function(s) were adapted from
 *    usr/src/lib/libc/db/hash_func.c, 4.4 BSD lite */

/* Hash function from Chris Torek. */
unsigned int
hash4(const void *data, unsigned int size)
{
    const char *key = data;
    size_t loop;
    unsigned int h;
    size_t len;

#define HASH4a   h = (h << 5) - h + *key++;
#define HASH4b   h = (h << 5) + h + *key++;
#define HASH4 HASH4b

    h = 0;
    len = strlen(key);
    loop = len >> 3;
    switch (len & (8 - 1)) {
    case 0:
	break;
    case 7:
	HASH4;
	/* FALLTHROUGH */
    case 6:
	HASH4;
	/* FALLTHROUGH */
    case 5:
	HASH4;
	/* FALLTHROUGH */
    case 4:
	HASH4;
	/* FALLTHROUGH */
    case 3:
	HASH4;
	/* FALLTHROUGH */
    case 2:
	HASH4;
	/* FALLTHROUGH */
    case 1:
	HASH4;
    }
    while (loop--) {
	HASH4;
	HASH4;
	HASH4;
	HASH4;
	HASH4;
	HASH4;
	HASH4;
	HASH4;
    }
    return h % size;
}

/*
 *  hash_create - creates a new hash table, uses the cmp_func
 *  to compare keys.  Returns the identification for the hash table;
 *  otherwise returns a negative number on error.
 */
hash_table *
hash_create(HASHCMP * cmp_func, int hash_sz, HASHHASH * hash_func)
{
    hash_table *hid = calloc(1, sizeof(hash_table));
    if (hid == NULL) {
        return NULL;
    }
    if (!hash_sz) {
	    hid->size = (unsigned int) DEFAULT_HASH_SIZE;
    } else {
	    hid->size = (unsigned int) hash_sz;
    }
    /* allocate and null the buckets */
    hid->buckets = calloc(hid->size, sizeof(hash_link *));
    if (hid->buckets == NULL) {
        free(hid);
        return NULL;
    }
    hid->cmp = cmp_func;
    hid->hash = hash_func;
    hid->next = NULL;
    hid->current_slot = 0;
    return hid;
}

/*
 *  hash_join - joins a hash_link under its key lnk->key
 *  into the hash table 'hid'.
 *
 *  It does not copy any data into the hash table, only links pointers.
 */
void
hash_join(hash_table * hid, hash_link * lnk)
{
    int i;
    i = hid->hash(lnk->key, hid->size);
    lnk->next = hid->buckets[i];
    hid->buckets[i] = lnk;
    hid->count++;
}

/*
 *  hash_lookup - locates the item under the key 'k' in the hash table
 *  'hid'.  Returns a pointer to the hash bucket on success; otherwise
 *  returns NULL.
 */
void *
hash_lookup(hash_table * hid, const void *k)
{
    hash_link *walker;
    int b;
    assert(k != NULL);
    b = hid->hash(k, hid->size);
    for (walker = hid->buckets[b]; walker != NULL; walker = walker->next) {
        /* strcmp of NULL is a SEGV */
        if (NULL == walker->key)
            return NULL;
	if ((hid->cmp) (k, walker->key) == 0)
	    return (walker);
	assert(walker != walker->next);
    }
    return NULL;
}

static void
hash_next_bucket(hash_table * hid)
{
    while (hid->next == NULL && ++hid->current_slot < hid->size)
	hid->next = hid->buckets[hid->current_slot];
}

/*
 *  hash_first - initializes the hash table for the hash_next()
 *  function.
 */
void
hash_first(hash_table * hid)
{
    assert(NULL == hid->next);
    hid->current_slot = 0;
    hid->next = hid->buckets[hid->current_slot];
    if (NULL == hid->next)
	hash_next_bucket(hid);
}

/*
 *  hash_next - returns the next item in the hash table 'hid'.
 *  Otherwise, returns NULL on error or end of list.
 *
 *  MUST call hash_first() before hash_next().
 */
void *
hash_next(hash_table * hid)
{
    hash_link *this = hid->next;
    if (NULL == this)
	return NULL;
    hid->next = this->next;
    if (NULL == hid->next)
	hash_next_bucket(hid);
    return this;
}

/*
 *  hash_last - resets hash traversal state to NULL
 *
 */
void
hash_last(hash_table * hid)
{
    assert(hid != NULL);
    hid->next = NULL;
    hid->current_slot = 0;
}

/*
 *  hash_remove_link - deletes the given hash_link node from the
 *  hash table 'hid'.  Does not free the item, only removes it
 *  from the list.
 *
 *  An assertion is triggered if the hash_link is not found in the
 *  list.
 */
void
hash_remove_link(hash_table * hid, hash_link * hl)
{
    hash_link **P;
    int i;
    assert(hl != NULL);
    i = hid->hash(hl->key, hid->size);
    for (P = &hid->buckets[i]; *P; P = &(*P)->next) {
	if (*P != hl)
	    continue;
	*P = hl->next;
	if (hid->next == hl) {
	    hid->next = hl->next;
	    if (NULL == hid->next)
		hash_next_bucket(hid);
	}
	hid->count--;
	return;
    }
    assert(0);
}

/*
 *  hash_get_bucket - returns the head item of the bucket
 *  in the hash table 'hid'. Otherwise, returns NULL on error.
 */
hash_link *
hash_get_bucket(hash_table * hid, unsigned int bucket)
{
    if (bucket >= hid->size)
	return NULL;
    return (hid->buckets[bucket]);
}

void
hashFreeItems(hash_table * hid, HASHFREE * free_func)
{
    hash_link *l;
    hash_link **list;
    int i = 0;
    int j;
    list = calloc(hid->count, sizeof(hash_link *));
    hash_first(hid);
    while ((l = hash_next(hid)) && i < hid->count) {
	*(list + i) = l;
	i++;
    }
    for (j = 0; j < i; j++)
	free_func(*(list + j));
    free(list);
}

void
hashFreeMemory(hash_table * hid)
{
    assert(hid != NULL);
    if (hid->buckets)
	free(hid->buckets);
    free(hid);
}

static int hash_primes[] =
{
    103,
    229,
    467,
    977,
    1979,
    4019,
    6037,
    7951,
    12149,
    16231,
    33493,
    65357
};

int
hashPrime(int n)
{
    int I = sizeof(hash_primes) / sizeof(int);
    int i;
    int best_prime = hash_primes[0];
    double min = fabs(log((double) n) - log((double) hash_primes[0]));
    double d;
    for (i = 0; i < I; i++) {
	d = fabs(log((double) n) - log((double) hash_primes[i]));
	if (d > min)
	    continue;
	min = d;
	best_prime = hash_primes[i];
    }
    return best_prime;
}

/*
 * return the key of a hash_link as a const string
 */
const char *
hashKeyStr(hash_link * hl)
{
    return (const char *) hl->key;
}
