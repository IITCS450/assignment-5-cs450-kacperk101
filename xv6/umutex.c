#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"
#include "umutex.h"

void mutex_init(umutex_t *m)
{
  m->locked = 0;
}

/*
 * Spin-yield until the lock is free, then acquire it.
 * Safe without atomics because we use cooperative scheduling:
 * only one thread runs at a time, so the test-and-set is
 * effectively atomic within a scheduling quantum.
 */
void mutex_lock(umutex_t *m)
{
  while (m->locked)
    thread_yield();
  m->locked = 1;
}

void mutex_unlock(umutex_t *m)
{
  m->locked = 0;
}
