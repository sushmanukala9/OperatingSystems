#include "threads/scheduler.h"
#include "threads/cpu.h"
#include "threads/interrupt.h"
#include "list.h"
#include "threads/spinlock.h"
#include <debug.h>
#define max(a, b) ((a) > (b) ? (a) : (b))

/* Scheduling. */
#define TIME_SLICE 4 /* # of timer ticks to give each thread. */
int64_t weight_0 = 1024;

/*
 * In the provided baseline implementation, threads are kept in an unsorted list.
 *
 * Threads are added to the back of the list upon creation.
 * The thread at the front is picked for scheduling.
 * Upon preemption, the current thread is added to the end of the queue
 * (in sched_yield), creating a round-robin policy if multiple threads
 * are in the ready queue.
 * Preemption occurs every TIME_SLICE ticks.
 */

/* Called from thread_init () and thread_init_on_ap ().
   Initializes data structures used by the scheduler.

   This function is called very early.  It is unsafe to call
   thread_current () at this point.
 */
void sched_init(struct ready_queue *curr_rq)
{
  list_init(&curr_rq->ready_list);
  curr_rq->min_vruntime = 0;
  curr_rq->sum_of_weights = 0;
  curr_rq->nr_ready = 0;
}
static const int prio_to_weight[40] = {
    88761, 71755, 56483, 46273, 36291,
    29154, 23254, 18705, 14949, 11916,
    9548, 7620, 6100, 4904, 3906,
    3121, 2501, 1991, 1586, 1277,
    1024, 820, 655, 526, 423,
    335, 272, 215, 172, 137,
    110, 87, 70, 56, 45,
    36, 29, 23, 18, 15};

/*bool less_vruntime(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED)
{
  const struct thread *thread_a = list_entry(a, struct thread, elem);
  const struct thread *thread_b = list_entry(b, struct thread, elem);

  if (thread_a->vruntime < thread_b->vruntime)
  {
    return true;
  }
  else if (thread_a->vruntime == thread_b->vruntime)
  {
    return thread_a->tid < thread_b->tid; // Use tid as a tiebreaker
  }
  return false;
}*/

bool less_vruntime(const struct list_elem *a, const struct list_elem *b, void *aux)
{
  const struct thread *thread_a = list_entry(a, struct thread, elem);
  const struct thread *thread_b = list_entry(b, struct thread, elem);
  return thread_a->vruntime < thread_b->vruntime || (thread_a->vruntime == thread_b->vruntime && thread_a->tid < thread_b->tid);
}
/*void calculate_min_vruntime(struct ready_queue *rq_to_add)
{
  if (!list_empty(&rq_to_add->ready_list))
  {
    struct thread *ret = list_entry(list_pop_front(&rq_to_add->ready_list), struct thread, elem);
    rq_to_add->min_vruntime = ret->vruntime;
  }
}*/

void calc_min_vruntime(struct ready_queue *rq_to_add)
{

  if (!rq_to_add->curr)
  {
    rq_to_add->min_vruntime = 0;
  }
  else
  {
    uint64_t d = (timer_gettime() - rq_to_add->curr->start_time) + rq_to_add->curr->cpu_time_consumed;
    rq_to_add->curr->vruntime = rq_to_add->curr->vruntime_0 + ((d * weight_0) / prio_to_weight[rq_to_add->curr->nice - NICE_MIN]);

    if (rq_to_add->min_vruntime < rq_to_add->curr->vruntime)
    {
      rq_to_add->min_vruntime = rq_to_add->curr->vruntime;
    }

    struct list_elem *itr = list_begin(&rq_to_add->ready_list);

    while (itr != list_end(&rq_to_add->ready_list))
    {
      struct thread *t = list_entry(itr, struct thread, elem);
      if (rq_to_add->min_vruntime > t->vruntime)
      {
        rq_to_add->min_vruntime = t->vruntime;
      }
      else
      {
        break;
      }

      itr = list_next(itr);
    }
  }
}

/*void calculate_min_vruntime(struct ready_queue *rq)
{
  if (!list_empty(&rq->ready_list))
  {
    struct list_elem *e = list_min(&rq->ready_list, less_vruntime, NULL);
    struct thread *front_thread = list_entry(e, struct thread, elem);
    rq->min_vruntime = front_thread->vruntime;
  }
}*/

/* Called from thread.c:wake_up_new_thread () and
 thread_unblock () with the CPU's ready queue locked.
 rq is the ready queue that t should be added to when
 it is awoken. It is not necessarily the current CPU.

 If called from wake_up_new_thread (), initial will be 1.
 If called from thread_unblock (), initial will be 0.

 Returns RETURN_YIELD if the CPU containing rq should
 be rescheduled when this function returns, else returns
 RETURN_NONE */
enum sched_return_action
sched_unblock(struct ready_queue *rq_to_add, struct thread *t, int initial UNUSED)
{

  // calculate fresh min vruntime from ready queue
  calc_min_vruntime(rq_to_add);

  if (initial == 1)
  {
    // t->weight = prio_to_weight[t->nice - NICE_MIN];
    t->cpu_time_consumed = 0;
    t->vruntime_0 = rq_to_add->min_vruntime;
    t->vruntime = t->vruntime_0;
  }
  else
  {
    int64_t cons = 20000000;
    t->vruntime_0 = max(t->vruntime, rq_to_add->min_vruntime - cons);
  }

  t->vruntime = t->vruntime_0 + (t->cpu_time_consumed * weight_0 / prio_to_weight[t->nice - NICE_MIN]);

  list_insert_ordered(&rq_to_add->ready_list, &t->elem, less_vruntime, NULL);
  rq_to_add->sum_of_weights += prio_to_weight[t->nice - NICE_MIN];
  rq_to_add->nr_ready++;

  // list_push_back(&rq_to_add->ready_list, &t->elem);

  /* CPU is idle */
  // if (!rq_to_add->curr)
  if (!rq_to_add->curr || (initial == 0 && t->vruntime < rq_to_add->curr->vruntime))
  {
    return RETURN_YIELD;
  }
  return RETURN_NONE;
}

/* Called from thread_yield ().
   Current thread is about to yield.  Add it to the ready list

   Current ready queue is locked upon entry.
 */
void sched_yield(struct ready_queue *curr_rq, struct thread *current)
{
  // list_push_back(&curr_rq->ready_list, &current->elem);

  current->cpu_time_consumed += timer_gettime() - current->start_time;
  current->vruntime = current->vruntime_0 + (current->cpu_time_consumed * weight_0 / prio_to_weight[current->nice - NICE_MIN]);
  // current->cpu_time_consumed = timer_ticks();
  list_insert_ordered(&curr_rq->ready_list, &current->elem, less_vruntime, NULL);
  curr_rq->sum_of_weights += prio_to_weight[current->nice - NICE_MIN];
  // calculate_min_vruntime(curr_rq);
  curr_rq->nr_ready++;
  // curr_rq->sum_of_weights = curr_rq->sum_of_weights + prio_to_weight[current->nice - NICE_MIN];
}

/* Called from next_thread_to_run ().
   Find the next thread to run and remove it from the ready list
   Return NULL if the ready list is empty.

   If the thread returned is different from the thread currently
   running, a context switch will take place.

   Called with current ready queue locked.
 */
struct thread *
sched_pick_next(struct ready_queue *curr_rq)
{
  if (list_empty(&curr_rq->ready_list))
    return NULL;

  struct list_elem *e = list_pop_front(&curr_rq->ready_list);
  struct thread *ret = list_entry(e, struct thread, elem);
  // calculate_min_vruntime(curr_rq);
  ret->start_time = timer_gettime();
  curr_rq->nr_ready--;
  curr_rq->sum_of_weights = curr_rq->sum_of_weights - prio_to_weight[ret->nice - NICE_MIN];
  return ret;
}

/* Called from thread_tick ().
 * Ready queue rq is locked upon entry.
 *
 * Check if the current thread has finished its timeslice,
 * arrange for its preemption if it did.
 *
 * Returns RETURN_YIELD if current thread should yield
 * when this function returns, else returns RETURN_NONE.
 */
enum sched_return_action
sched_tick(struct ready_queue *curr_rq, struct thread *current UNUSED)
{
  int64_t time_slice = 4000000;
  current->ideal_runtime = (time_slice * (curr_rq->nr_ready + 1) * prio_to_weight[current->nice - NICE_MIN]) / (curr_rq->sum_of_weights + prio_to_weight[current->nice - NICE_MIN]);
  // current->ideal_runtime = 4;
  /* Enforce preemption. */

  if ((timer_gettime() - current->start_time) >= current->ideal_runtime)
  {
    /* Start a new time slice. */
    // curr_rq->thread_ticks = 0;

    return RETURN_YIELD;
  }
  return RETURN_NONE;
}

/* Called from thread_block (). The base scheduler does
   not need to do anything here, but your scheduler may.

   'cur' is the current thread, about to block.
 */
void sched_block(struct ready_queue *rq UNUSED, struct thread *current UNUSED)
{
  // blocked_time = timer_gettime();
  current->cpu_time_consumed += timer_gettime() - current->start_time;
  // current->start_time = timer_ticks();
  current->vruntime = current->vruntime_0 + (current->cpu_time_consumed * weight_0 / prio_to_weight[current->nice - NICE_MIN]);
  //  current->cpu_time_consumed = current->cpu_time_consumed - (timer_ticks() - current->cpu_time_consumed);
}
