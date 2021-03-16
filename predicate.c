#include "hashtable.h"

#include <klee/klee.h>

// #define GET_SYMBOLIC
#define INSTANCE_SYMBOLIC

static _Py_hashtable_t *g_instance = NULL;
static char *g_key = NULL;

void init_pred() {
  g_instance = _Py_hashtable_new();
#ifdef INSTANCE_SYMBOLIC
  klee_make_symbolic(g_instance, sizeof(_Py_hashtable_t), "instance");
#endif

  g_key = "some random string";
}

void set_pred_true() {
  char *V = "some string";

  _Py_hashtable_pop(g_instance, g_key, NULL);
  _Py_hashtable_set(g_instance, g_key, V);
}

void set_pred_false() {
  void *x;

  _Py_hashtable_pop(g_instance, g_key, &x);
}

int get_pred() {
#ifdef GET_SYMBOLIC
  klee_make_symbolic(g_instance->buckets, g_instance->num_buckets * sizeof(g_instance->buckets[0]), "buckets");
#endif

  if (_Py_hashtable_get_entry(g_instance, g_key))
    return 1;
  
  return 0;
}
