#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "hashtable.h"

#ifdef __LP64__
#  define SIZEOF_VOID_P 8
#else
#  define SIZEOF_VOID_P 4
#endif

#define KEY_SIZE SIZEOF_VOID_P
#define DATA_SIZE SIZEOF_VOID_P

#define HASHTABLE_MIN_SIZE 16
#define HASHTABLE_HIGH 0.50
#define HASHTABLE_LOW 0.10
#define HASHTABLE_REHASH_FACTOR 2.0 / (HASHTABLE_LOW + HASHTABLE_HIGH)

#define BUCKETS_HEAD(SLIST) \
        ((_Py_hashtable_entry_t *)_Py_SLIST_HEAD(&(SLIST)))
#define TABLE_HEAD(HT, BUCKET) \
        ((_Py_hashtable_entry_t *)_Py_SLIST_HEAD(&(HT)->buckets[BUCKET]))
#define ENTRY_NEXT(ENTRY) \
        ((_Py_hashtable_entry_t *)_Py_SLIST_ITEM_NEXT(ENTRY))
#define HASHTABLE_ITEM_SIZE(HT) \
        (sizeof(_Py_hashtable_entry_t) + (HT)->key_size + (HT)->data_size)

#define ENTRY_READ_PDATA(TABLE, ENTRY, DATA_SIZE, PDATA) \
    do { \
        assert((DATA_SIZE) == (TABLE)->data_size); \
        memcpy((PDATA), _Py_HASHTABLE_ENTRY_PDATA(TABLE, (ENTRY)), \
                  (DATA_SIZE)); \
    } while (0)

#define ENTRY_WRITE_PDATA(TABLE, ENTRY, DATA_SIZE, PDATA) \
    do { \
        assert((DATA_SIZE) == (TABLE)->data_size); \
        memcpy((void *)_Py_HASHTABLE_ENTRY_PDATA((TABLE), (ENTRY)), \
                  (PDATA), (DATA_SIZE)); \
    } while (0)

/* Forward declaration */
static void hashtable_rehash(_Py_hashtable_t *ht);

static void
_Py_slist_prepend(_Py_slist_t *list, _Py_slist_item_t *item)
{
    item->next = list->head;
    list->head = item;
}

static void
_Py_slist_remove(_Py_slist_t *list, _Py_slist_item_t *previous,
                 _Py_slist_item_t *item)
{
    if (previous != NULL)
        previous->next = item->next;
    else
        list->head = item->next;
}

Py_uhash_t
_Py_hashtable_hash_ptr(struct _Py_hashtable_t *ht, const void *pkey)
{
    void *key;

    _Py_HASHTABLE_READ_KEY(ht, pkey, key);
    return (Py_uhash_t)_Py_HashPointer(key);
}

int
_Py_hashtable_compare_direct(_Py_hashtable_t *ht, const void *pkey,
                             const _Py_hashtable_entry_t *entry)
{
    const void *pkey2 = _Py_HASHTABLE_ENTRY_PKEY(entry);
    return (memcmp(pkey, pkey2, ht->key_size) == 0);
}

/* makes sure the real size of the buckets array is a power of 2 */
static size_t
round_size(size_t s)
{
    size_t i;
    if (s < HASHTABLE_MIN_SIZE)
        return HASHTABLE_MIN_SIZE;
    i = 1;
    while (i < s)
        i <<= 1;
    return i;
}

_Py_hashtable_t *
_Py_hashtable_new_full(size_t init_size,
                       _Py_hashtable_hash_func hash_func,
                       _Py_hashtable_compare_func compare_func)
{
    _Py_hashtable_t *ht;
    size_t buckets_size;

    ht = (_Py_hashtable_t *)malloc(sizeof(_Py_hashtable_t));
    if (ht == NULL)
        return ht;

    ht->num_buckets = round_size(init_size);
    ht->entries = 0;
    ht->key_size = KEY_SIZE;
    ht->data_size = DATA_SIZE;

    buckets_size = ht->num_buckets * sizeof(ht->buckets[0]);
    ht->buckets = malloc(buckets_size);
    if (ht->buckets == NULL) {
        free(ht);
        return NULL;
    }
    memset(ht->buckets, 0, buckets_size);

    ht->hash_func = hash_func;
    ht->compare_func = compare_func;
    return ht;
}


_Py_hashtable_t *
_Py_hashtable_new()
{
    return _Py_hashtable_new_full(HASHTABLE_MIN_SIZE,
                                  _Py_hashtable_hash_ptr, _Py_hashtable_compare_direct);
}

static int
_Py_hashtable_pop_entry(_Py_hashtable_t *ht, size_t key_size, const void *pkey,
                        void *data, size_t data_size)
{
    Py_uhash_t key_hash;
    size_t index;
    _Py_hashtable_entry_t *entry, *previous;

    assert(key_size == ht->key_size);

    key_hash = ht->hash_func(ht, pkey);
    index = key_hash & (ht->num_buckets - 1);

    previous = NULL;
    for (entry = TABLE_HEAD(ht, index); entry != NULL; entry = ENTRY_NEXT(entry)) {
        if (entry->key_hash == key_hash && ht->compare_func(ht, pkey, entry))
            break;
        previous = entry;
    }

    if (entry == NULL)
        return 0;

    _Py_slist_remove(&ht->buckets[index], (_Py_slist_item_t *)previous,
                     (_Py_slist_item_t *)entry);
    ht->entries--;

    if (data != NULL)
        ENTRY_READ_PDATA(ht, entry, data_size, data);
    free(entry);

    if ((float)ht->entries / (float)ht->num_buckets < HASHTABLE_LOW)
        hashtable_rehash(ht);
    return 1;
}

int
_Py_hashtable_pop(_Py_hashtable_t *ht, const void *pkey, void *data)
{
    // assert(data != NULL);
    return _Py_hashtable_pop_entry(ht, KEY_SIZE, pkey, data, DATA_SIZE);
}

_Py_hashtable_entry_t *
_Py_hashtable_get_entry(_Py_hashtable_t *ht, const void *pkey)
{
    Py_uhash_t key_hash;
    size_t index;
    _Py_hashtable_entry_t *entry;

    assert(KEY_SIZE == ht->key_size);

    key_hash = ht->hash_func(ht, pkey);
    index = key_hash & (ht->num_buckets - 1);

    for (entry = TABLE_HEAD(ht, index); entry != NULL; entry = ENTRY_NEXT(entry)) {
        if (entry->key_hash == key_hash && ht->compare_func(ht, pkey, entry))
            break;
    }

    return entry;
}

int
_Py_hashtable_set(_Py_hashtable_t *ht, const void *pkey, const void *data)
{
    Py_uhash_t key_hash;
    size_t index;
    _Py_hashtable_entry_t *entry;

    assert(KEY_SIZE == ht->key_size);

    assert(data != NULL || DATA_SIZE == 0);
#ifndef NDEBUG
    /* Don't write the assertion on a single line because it is interesting
       to know the duplicated entry if the assertion failed. The entry can
       be read using a debugger. */
    entry = _Py_hashtable_get_entry(ht, pkey);
    assert(entry == NULL);
#endif

    key_hash = ht->hash_func(ht, pkey);
    index = key_hash & (ht->num_buckets - 1);

    entry = malloc(HASHTABLE_ITEM_SIZE(ht));
    if (entry == NULL) {
        /* memory allocation failed */
        return -1;
    }

    entry->key_hash = key_hash;
    memcpy((void *)_Py_HASHTABLE_ENTRY_PKEY(entry), pkey, ht->key_size);
    if (data)
        ENTRY_WRITE_PDATA(ht, entry, DATA_SIZE, data);

    _Py_slist_prepend(&ht->buckets[index], (_Py_slist_item_t*)entry);
    ht->entries++;

    if ((float)ht->entries / (float)ht->num_buckets > HASHTABLE_HIGH)
        hashtable_rehash(ht);
    return 0;
}

static void
hashtable_rehash(_Py_hashtable_t *ht)
{
    size_t buckets_size, new_size, bucket;
    _Py_slist_t *old_buckets = NULL;
    size_t old_num_buckets;

    new_size = round_size((size_t)(ht->entries * HASHTABLE_REHASH_FACTOR));
    if (new_size == ht->num_buckets)
        return;

    old_num_buckets = ht->num_buckets;

    buckets_size = new_size * sizeof(ht->buckets[0]);
    old_buckets = ht->buckets;
    ht->buckets = malloc(buckets_size);
    if (ht->buckets == NULL) {
        /* cancel rehash on memory allocation failure */
        ht->buckets = old_buckets ;
        /* memory allocation failed */
        return;
    }
    memset(ht->buckets, 0, buckets_size);

    ht->num_buckets = new_size;

    for (bucket = 0; bucket < old_num_buckets; bucket++) {
        _Py_hashtable_entry_t *entry, *next;
        for (entry = BUCKETS_HEAD(old_buckets[bucket]); entry != NULL; entry = next) {
            size_t entry_index;


            assert(ht->hash_func(ht, _Py_HASHTABLE_ENTRY_PKEY(entry)) == entry->key_hash);
            next = ENTRY_NEXT(entry);
            entry_index = entry->key_hash & (new_size - 1);

            _Py_slist_prepend(&ht->buckets[entry_index], (_Py_slist_item_t*)entry);
        }
    }

    free(old_buckets);
}

Py_hash_t
_Py_HashPointer(void *p)
{
    Py_hash_t x;
    size_t y = (size_t)p;
    /* bottom 3 or 4 bits are likely to be 0; rotate y by 4 to avoid
       excessive hash collisions for dicts and sets */
    y = (y >> 4) | (y << (8 * SIZEOF_VOID_P - 4));
    x = (Py_hash_t)y;
    if (x == -1)
        x = -2;
    return x;
}
