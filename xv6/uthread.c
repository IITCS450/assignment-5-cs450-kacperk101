#include "types.h"
#include "stat.h"
#include "user.h"
#include "uthread.h"

struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;   
};

#define MAX_THREADS 8
#define STACK_SIZE  8192

typedef enum { UNUSED = 0, RUNNABLE, RUNNING, ZOMBIE } thread_state_t;

struct thread {
  struct context *ctx;
  char           *stack;
  thread_state_t  state;
  void          (*fn)(void*);
  void           *arg;
  tid_t           tid;
};

static struct thread  threads[MAX_THREADS];
static struct thread *current;

static void thread_stub(void)
{
  current->fn(current->arg);
  current->state = ZOMBIE;
  while (1)
    thread_yield();
}

void thread_init(void)
{
  int i;
  for (i = 0; i < MAX_THREADS; i++)
    threads[i].state = UNUSED;

  threads[0].state = RUNNING;
  threads[0].tid   = 0;
  threads[0].stack = 0;
  current = &threads[0];
}

tid_t thread_create(void (*fn)(void*), void *arg)
{
  struct thread *t = 0;
  tid_t tid = -1;
  uint *sp;
  int i;

  for (i = 1; i < MAX_THREADS; i++) {
    if (threads[i].state == UNUSED) {
      t   = &threads[i];
      tid = (tid_t)i;
      break;
    }
  }
  if (!t)
    return -1;

  t->stack = malloc(STACK_SIZE);
  if (!t->stack)
    return -1;

 
  sp    = (uint *)(t->stack + STACK_SIZE);
  *--sp = 0;
  *--sp = (uint)thread_stub;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;
  *--sp = 0;

  t->ctx   = (struct context *)sp;
  t->fn    = fn;
  t->arg   = arg;
  t->tid   = tid;
  t->state = RUNNABLE;

  return tid;
}

static struct thread *pick_next(void)
{
  int start = (int)(current - threads);
  int i;
  for (i = 1; i <= MAX_THREADS; i++) {
    int j = (start + i) % MAX_THREADS;
    if (threads[j].state == RUNNABLE)
      return &threads[j];
  }
  return 0;
}

void thread_yield(void)
{
  struct thread *old, *next;

  next = pick_next();
  if (!next)
    return;

  old = current;
  if (old->state == RUNNING)
    old->state = RUNNABLE;

  current     = next;
  next->state = RUNNING;

  uswtch(&old->ctx, next->ctx);
}

int thread_join(tid_t tid)
{
  struct thread *t;

  if (tid <= 0 || tid >= MAX_THREADS)
    return -1;

  t = &threads[tid];
  while (t->state != ZOMBIE && t->state != UNUSED)
    thread_yield();

  if (t->state == ZOMBIE) {
    free(t->stack);
    t->stack = 0;
    t->state = UNUSED;
  }
  return 0;
}