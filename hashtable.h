#ifndef HASHTABLE_H

#include <unistd.h>

typedef ssize_t Py_ssize_t;
typedef size_t Py_uhash_t;
typedef Py_ssize_t Py_hash_t;

/* Single linked list */

typedef struct _Py_slist_item_s {
    struct _Py_slist_item_s *next;
} _Py_slist_item_t;

typedef struct {
    _Py_slist_item_t *head;
} _Py_slist_t;

#define _Py_SLIST_ITEM_NEXT(ITEM) (((_Py_slist_item_t *)ITEM)->next)

#define _Py_SLIST_HEAD(SLIST) (((_Py_slist_t *)SLIST)->head)

/* _Py_hashtable: table entry */

typedef struct {
    /* used by _Py_hashtable_t.buckets to link entries */
    _Py_slist_item_t _Py_slist_item;

    Py_uhash_t key_hash;

    /* key (key_size bytes) and then data (data_size bytes) follows */
} _Py_hashtable_entry_t;

#define _Py_HASHTABLE_ENTRY_PKEY(ENTRY) \
        ((const void *)((char *)(ENTRY) \
                        + sizeof(_Py_hashtable_entry_t)))

#define _Py_HASHTABLE_ENTRY_PDATA(TABLE, ENTRY) \
        ((const void *)((char *)(ENTRY) \
                        + sizeof(_Py_hashtable_entry_t) \
                        + (TABLE)->key_size))

/* Get a key value from pkey: use memcpy() rather than a pointer dereference
   to avoid memory alignment issues. */
#define _Py_HASHTABLE_READ_KEY(TABLE, PKEY, DST_KEY) \
    do { \
        assert(sizeof(DST_KEY) == (TABLE)->key_size); \
        memcpy(&(DST_KEY), (PKEY), sizeof(DST_KEY)); \
    } while (0)

/* Forward declaration */
struct _Py_hashtable_t;

typedef Py_uhash_t (*_Py_hashtable_hash_func) (struct _Py_hashtable_t *ht,
                                               const void *pkey);
typedef int (*_Py_hashtable_compare_func) (struct _Py_hashtable_t *ht,
                                           const void *pkey,
                                           const _Py_hashtable_entry_t *he);

/* _Py_hashtable: table */
typedef struct _Py_hashtable_t {
    size_t num_buckets;
    size_t entries; /* Total number of entries in the table. */
    _Py_slist_t *buckets;
    size_t key_size;
    size_t data_size;

    _Py_hashtable_hash_func hash_func;
    _Py_hashtable_compare_func compare_func;
} _Py_hashtable_t;

Py_uhash_t
_Py_hashtable_hash_ptr(struct _Py_hashtable_t *ht, const void *pkey);

int
_Py_hashtable_compare_direct(_Py_hashtable_t *ht, const void *pkey,
                             const _Py_hashtable_entry_t *entry);

_Py_hashtable_t *
_Py_hashtable_new();

int
_Py_hashtable_pop(_Py_hashtable_t *ht, const void *pkey, void *data);

int
_Py_hashtable_set(_Py_hashtable_t *ht, const void *pkey, const void *data);

_Py_hashtable_entry_t *
_Py_hashtable_get_entry(_Py_hashtable_t *ht, const void *pkey);

Py_hash_t
_Py_HashPointer(void *p);

#endif