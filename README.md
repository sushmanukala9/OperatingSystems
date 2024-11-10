# OperatingSystems

+----------------------------------------------------------------- +
			|                                                      |
			| PROJECT 1:COMPLETE FAIR SCHEDULER & LOAD BALANCER IMPLEMENTATION |
			|   DESIGN DOCUMENT                                                |
			+----------------------------------------------------------------- +
				   
---- YOU ----

>> Fill in your name and email address

Sushma Shivani Nukala  <sushmashivani@vt.edu>

---- PRELIMINARIES ----

>> If you have any preliminary comments on your submission, notes for the
>> TAs, or extra credit, please give them here.

We are including DESIGNDOC at "src/threads/DESIGNDOC".

Our changes are in thread.h, scheduler.h, scheduler.c and thread.c.


>> Please cite any offline or online sources you consulted while
>> preparing your submission, other than the Pintos documentation, course
>> text, lecture notes and course staff.

No References.

			  ADVANCED SCHEDULER
			  ==================

---- DATA STRUCTURES ----

>> A1: Copy here the declaration of each new or changed `struct' or
>> `struct' member, global or static variable, `typedef', or
>> enumeration.  Identify the purpose of each in 25 words or less.

   Added to struct thread in thread.h:
	

	 int64_t ideal_runtime; /* Stores the time for which each thread should run at each cpu cycle*/
     int64_t vruntime_0;    /* Stores initial virtual runtime when the thread is created */
     int64_t vruntime;      /* Stores current total virtual runtime the thread executed for
                            */
     int64_t cpu_time_consumed; /* Stores the number of ticks the thread executed for */
     int64_t start_time;    /* Stores the timestamp when the thread gets the CPU for 
                               execution everytime 
                            */

   Added to struct ready_queue in scheduler.h

     int64_t min_vruntime;       /* Stores minimum virtual runtime of all the threads that 
                                    are currently RUNNING or READY 
                                 */

     int64_t sum_of_weights;  /* Stores the sum of weights of all the currently RUNNING and READY 
                                    threads
                                 */


Global/static variables in Scheduler.c:

int64_t weight_0; /* reference weight value for computing weights based on nice values */

static const int prio_to_weight[40]; /* provides a mapping from nice values to weights */


---- ALGORITHMS ----

>> A2: Explain briefly what your scheduler does when each of the following happens:
>> Thread is created
>> Current thread blocks
>> Thread is unblocked
>> Current thread yields
>> Timer tick arrives

  When Thread is created :

When a thread is initiated, it initially has the BLOCKED status. Consequently, sched_unblock is invoked at the conclusion of the thread's creation process. During this sched_unblock execution, the min_vruntime gets calculated. If there are no other threads in the ready_queue, indicating only the idle_thread is operational, min_vruntime is set to zero. However, if there are active threads, the system evaluates the CPU consumption of the currently executing thread and determines its virtual runtime (vruntime) using the designated formula. This vruntime assessment leads to an update of min_vruntime based on the current thread's vruntime calculation.

Subsequently, the function iterates over the ready_queue, updating min_vruntime to reflect the lowest vruntime value among all queued threads. For a thread being created and introduced (as opposed to reactivating a blocked thread), its initial vruntime (vruntime_0), the actual vruntime, and cpu_time_consumed are set appropriately. Following these updates, the new thread is placed into the ready_queue, which also triggers an update to the cumulative weights (sum_of_weights) of all threads in the queue.

Finally, if the new thread's vruntime is lesser than that of the currently active thread, or if there previously was no active thread, the function signals a scheduling need by returning RETURN_YIELD. This signal prompts the scheduler to consider executing the newly added thread.


Current thread blocks :

Updates the thread's consumed CPU time and virtual runtime, ensuring that when it is unblocked, it continues with accurate accounting for its CPU usage.
Then, scheduler picks next thread to run. While the selected thread is about to be removed from ready_queue, we update sum_of_weights of the ready queue and the start_time of the thread. 

  When Thread is unblocked :

Upon unblocking, the scheduler examines the current minimum virtual runtime (min_vruntime) in the ready queue and compares it to the unblocked thread's vruntime.

If the thread's vruntime is significantly higher than the min_vruntime due to its blocked state, the system can grant a sleeper bonus by adjusting the thread's vruntime closer to min_vruntime.

The sleeper bonus effectively reduces the vruntime for the unblocked thread, recognizing its time spent waiting (e.g., for I/O or synchronization) rather than consuming CPU cycles.
This reduction is typically calculated based on how long the thread has been blocked and the discrepancy between its vruntime and the min_vruntime.

Once the thread's vruntime is adjusted for the sleeper bonus, it is inserted into the ready queue in a position that reflects its updated vruntime. This ensures the thread competes fairly for CPU time based on its actual usage and wait time.
This reinsertion is done in a manner that maintains the sorted order of the ready queue based on vruntime, ensuring that threads closer to the minimum vruntime are given priority.

 
   
   Current thread yields :

    Updates the current thread's CPU time consumed and vruntime, then reinserts it into the ready queue based on its new virtual runtime to ensure fair scheduling upon next pick.
   
   Timer tick arrives : 
     Compares the running thread's consumed CPU time against its ideal runtime. If the thread has consumed its allotment, it yields the CPU to allow scheduling of the next thread.


>> A3: What steps did you take to make sure that the idle thread is not 
>> taken into account in any scheduling decisions?

The idle thread is never added to the ready list; it's only selected by next_thread_to_run() when no other threads are available. Additionally, the scheduler checks if the current thread is the idle thread and avoids including it in scheduling decisions, ensuring it only runs when absolutely necessary.

>> A4: Briefly critique your design, pointing out advantages and
>> disadvantages in your design choices.  If you were to have extra
>> time to work on this part of the project, how might you choose to
>> refine or improve your design?
   
Advantages of our design would be :


Efficiency: Inserting threads into a sorted ready list based on vruntime helps in quickly selecting the next thread to run, improving scheduling efficiency.

Dynamic Adaptability: The scheduler dynamically adapts to changes in thread behavior, recalculating vruntime and adjusting scheduling decisions as threads are created, blocked, unblocked, or yield.

Responsiveness: By recalculating vruntime upon unblocking, the scheduler potentially gives I/O-bound threads (which often block and unblock) a favorable position in the ready queue, improving overall system responsiveness.


Disadvantages would be 

Complexity: The calculation of vruntime and its adjustments for every significant scheduling event add complexity to the scheduler, potentially increasing the overhead and the context switch time.

Priority Inversion: While nice values influence vruntime, the current implementation does not explicitly address priority inversion, where a lower-priority thread holds a resource needed by a higher-priority thread.


Improvements :

Negative time handling: In Scenarios where a blocked thread sleeps for less than 20ms it would not get full sleeper bonus , in that case vruntime/min_vruntime might become negative which could've been handled.

Optimization of Data Structures: Evaluate the efficiency of data structures used in the scheduler, especially the ready list, to optimize insertion and removal operations.


			     LOAD BALANCER
			     =============

---- DATA STRUCTURES ----

>> B1: Copy here the declaration of each new or changed `struct' or
>> `struct' member, global or static variable, `typedef', or
>> enumeration.  Identify the purpose of each in 25 words or less.

No new variables were added; the existing variables were utilized in alignment with the Completely Fair Scheduler (CFS).


---- ALGORITHMS ----

>> B2: When is load balancing invoked? Is this enough to ensure
>> that no CPUs are idle while there are threads in any ready queue?

Load balancing is typically invoked under two circumstances:

1. When a CPU becomes idle, as identified in the idle function, suggesting it has no ready threads to execute. The system then checks if it can balance the load by pulling threads from busier CPUs.


2. Periodically or under certain conditions which might be identified during regular checks, like during timer ticks or when a significant imbalance is detected across CPUs.

While invoking load balancing during these conditions helps reduce idle CPU time and improve workload distribution, it doesn't guarantee that no CPUs will be idle while there are threads in any ready queue. Other factors like synchronization constraints or affinity rules might prevent a thread from being migrated, potentially leaving some CPUs idle even when there's work that could be done elsewhere.


>> B3: Briefly describe what happens in a call to load_balance()

1. The system first identifies the busiest and least busy CPUs by comparing their loads (e.g., the sum of the weights of their ready threads).

2. It calculates the load imbalance by the condition imbalance * 4 >= sum_of_weights and determines if it's significant enough to warrant rebalancing. 

3.If rebalancing is deemed necessary, it migrates threads from the busiest CPU's ready queue to the least busy one until the imbalance is mitigated or until no suitable threads for migration remain. 

4. Each migrated thread's vruntime is adjusted based on the differences in min_vruntime between the two CPUs to ensure fair scheduling post-migration.

>> B4: What steps are taken to ensure migrated threads do not
>> monopolize the CPU if they had significantly lower vruntimes
>> than the other threads when they migrated?

To prevent migrated threads from monopolizing the CPU:

When threads undergo migration, their virtual runtimes (vruntimes) are recalibrated to align with the prevailing conditions of the target CPU's ready queue. This recalibration process ensures that their computed ideal runtime is reassessed, determining the appropriate share of CPU time they should receive. Consequently, this adjustment mechanism guarantees that these threads neither dominate the CPU resources disproportionately nor undermine the equitable distribution of CPU time. It facilitates a balanced execution framework where migrated threads can commence their operations promptly, yet only for a duration that is just and proportionate, reflective of their recalculated ideal runtime.
 

>> B5: Let's say hypothetically that CPU0 has 100 threads while 
>> CPU1 has 102 threads, and each thread has the same nice value.
>> CPU0 calls load_balance(). Do any tasks get migrated? If not,
>> what are some possible advantages to refraining from migrating
>> threads in that scenario?



In this hypothetical scenario, if all threads have the same nice value, the load difference between CPU0 and CPU1 is very minimal. Invoking load_balance() in this situation is unlikely to result in any task migrations because:

1. The minimal load difference does not justify the overhead and potential disruptions caused by migrating threads.


2. Migrating tasks for such marginal gains could lead to unnecessary context switching and cache invalidation, which outweigh the benefits of marginally better load distribution.


The advantages of refraining from migrating threads in this scenario include:

1. Avoiding the overhead associated with thread migration (e.g., context switching, cache warming).

2. Minimizing disruptions to running threads, which could otherwise lead to increased latency and decreased throughput for minimal gains in load balancing.
  

---- SYNCHRONIZATION ----

>> B6: How are race conditions avoided when a CPU looks at another
>> CPU's load, or pulling threads from another CPU?

Race conditions are avoided by acquiring a lock on the ready queues of both the CPUs. First we acquire lock on the busy CPU to remove the thread and then we lock the current CPU's ready list while inserting. 

>> B7: It is possible, although unlikely, that two CPUs may try to load
>> balance from each other at the same time. How do you avoid potential
>> deadlock?


1. Instead of blocking on lock acquisition (which can lead to deadlocks), CPUs should attempt to acquire necessary locks using a try-lock mechanism. If a CPU fails to acquire a lock because it's already held by another CPU, it should back off and either retry after a delay or abort the load balancing operation.
This approach ensures that if two CPUs target each other for load balancing, one of them will fail to acquire the lock and will not proceed, thus avoiding deadlock.


2. May Use atomic operations to set a flag indicating that a CPU is attempting to load balance. Before a CPU starts the load balancing process, it checks this flag and proceeds only if it can atomically set it, indicating that no other CPU is currently in the load balancing process.


---- RATIONALE ----

>> B8: If you deviated from the specifications, explain why. In what ways was your
>> implementation superior to the one specified?

We adhered strictly to the specifications, ensuring that our implementation remained fully aligned with the outlined requirements.

               MISC
               ====

>> C1: In Pintos, timer interrupts are issued periodically at a 
>> predetermined frequency. In contrast, advanced systems like Linux have
>> adopted a tickless kernel approach, where the timer interrupt is not
>> issued periodically but rather set to arrive on-demand. Looking back at 
>> at some of the inefficiencies that may have crept into your alarm clock,
>> scheduler, and load balancer, in what ways would on-demand interrupts have
>> been beneficial? Please be specific.

Alarm Clock:

In the current Pintos implementation, the alarm clock wakes up at every tick to check if any sleeping threads need to be awakened. With on-demand interrupts, the timer interrupt would only be set to the next thread's wake-up time, significantly reducing the number of wake-ups and checks, thereby reducing CPU usage and energy consumption.
It would improve the accuracy and efficiency of waking up threads. Instead of checking at every tick, the system could schedule the next interrupt precisely when the next thread needs to wake up, minimizing wake latency.

Scheduler :

For the scheduler, on-demand interrupts could eliminate unnecessary context switches by allowing the current thread to continue execution until a significant scheduling event occurs (like a thread becoming ready to run or the current thread blocking). This would optimize CPU time usage and reduce overhead.
The scheduler could benefit from more precise control over thread execution times and better decision-making for thread prioritization and preemption, especially in workload scenarios where many threads are compute-bound and less frequently blocked or preempted.


Load Balancer :


In a tickless environment, the load balancer could be invoked dynamically based on system load rather than at fixed interval ticks. This could lead to more responsive load balancing decisions and faster redistribution of threads across CPUs, especially in uneven load situations.
It could reduce the overhead of frequent load balancing checks in low-load situations, as the load balancer could be invoked on-demand when a significant load imbalance is detected rather than at every tick, improving overall system efficiency.
   
