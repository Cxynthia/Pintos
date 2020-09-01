Design Document for Project 2: Threads
======================================

## Group Members

* Jungwoo Park <jwp@berkeley.edu>
* Brian Ahn <brianahn0717@berkeley.edu>
* Cynthia Chen <xinyi.chen@berkeley.edu>
* Eddie Gao <eddiegao98@berkeley.edu>

---

## Task 1: Efficient Alarm Clock

#### 1.1 Data Structures and Functions
A new attribute `int sleep_timer`, will be added to the thread struct, which keeps track of the current number of ticks that the thread has led to sleep before it wakes up again.

We will also define a function `void check_sleeping_threads(void)` in thread.c. For current thread `t`, check_sleeping_threads will check if `t.sleep_timer = 0`. If it is not, then decrement `t.sleep_timer` by 1.  If, after being decremented, `t.sleep_timer = 0`, then `t` will be added to the `ready_list` global variable in thread.c, and `t->status` will be set to `THREAD_READY`.

#### 1.2 Algorithms
We will remove the while loop from `timer_sleep`. Instead, when timer_sleep is called, after disabling interrupts, for current thread `c_t`, it will set `c_t->sleep_timer=ticks`, where `ticks` is the argument passed into `timer_sleep` (There is no need to consider the case where `c_t` is already sleeping, as it is not possible for a sleeping thread to call timer_sleep or any other function. Thus, we can assume that `c_t->sleep_timer` will always be 0 when `timer_sleep` is called, as the thread will be in the ready state when that happens). `c_t->status` will also be set to `THREAD_BLOCKED`. Then, it will remove the current thread `c_t` from the ready_list global variable of thread.c--this will prevent the thread from being scheduled, as the scheduler in thread.c picks the next thread to run only from the ready_list. `C_t` will remain on the `all_list`, so we still can keep track of it.

In `timer_interrupt()`, each time after the ticks are incremented, we will use `thread_foreach` to call `check_sleeping_threads` on all currently active threads. `Timer_interrupt` is the only place where the ticks variable is incremented, so this means that every tick that occurs, each thread will have its remaining time to sleep decreased correctly by one tick and put back on the ready queue if it has finished sleeping.

#### 1.3 Synchronization
Synchronization is not a concern for task 1--interrupts are disabled during timer_sleep, so it is not possible for there to be a context switch away from it while it is running that might cause a synchronization problem. 

#### 1.4 Rationale 
Our implementation avoids busy-waiting, as the thread being waited on is not taking any actions, and thus will not consume CPU cycles while waiting. Instead, the thread simply will not be scheduled while it is sleeping, as it will not be present in the ready queue to be scheduled next.

Our implementation requires spending O(t) time with interrupts disabled, where t is the number of threads. Since the number of threads is unlikely to ever be very high (especially in a single-threaded OS such as Pintos where a process cannot create multiple threads during its execution), this is unlikely to be so long that it will cause trouble.


## Task 2: Priority Scheduler

#### 2.0 Abstract
For every tick: 
1. Priority scheduling: determine which thread has the highest priority and run it. Yield the others. *maybe consider having an indication of thread that was donated a priority We need: priorities for all of the threads 

2. Priority donation: determine which lock is being held by lower priority thread (one that is not the highest) and set the priority to the equivalent to highest priority. If several threads with highest priority, round-robin. *keep track of original priority and go back to the original priority when the lock is released. We need: which lock is being blocked, which thread is holding the lock, the priority of the current thread, the original priority for the thread that is being donated

#### 2.1 Data Structures and Functions
In thread.c:

```
// add variables to the thread struct to help implement priority donation 
struct thread {
…
fixed_point_t base_priority;
fixed_point_t effective_priority;
struct list locks_held;
…
}

// add this list to keep track of the threads with the highest priority
struct list highest_priority_list;

// modify this function call with setting up the additional variables in the thread struct
void init_thread (struct thread *, const char *name, int priority);

// modify this function to return the effective priority instead
int thread_get_priority (void);

// modify this function to satisfy the priority scheduling, use highest_priority_list instead of ready_list here
struct thread *next_thread_to_run (void);
```

In synch.c:

```
// modify this function to add priority donation process here
void lock_acquire (struct lock *lock)

// called synch.c:70-73; check dst_thread priority with current thread’s
void priority_donation (lock *lock)

// add this helper function to get the ready_list
struct list get_ready_list (void);

// add this helper function to get the highest_priority_list
struct list get_highest_priority_list (void);
```

In timer.c:
```
// A helper function which computes the threads with the highest priority by two iterations and put them into the highest_priority_list
void compute_highest_priority_list (void)

// modify this function to call compute_highest_priority_list() every tick
Static void timer_interrupt (struct intr_frame *args UNUSED)
```

#### 2.2 Algorithms
Given that all of the threads with THREAD_READY value for `thread_status` have been inserted to the `ready_list`, the algorithm consists of two iterations of the `ready_list` for every timer tick. Note that the iterations are being done in the function call t`imer.c:timer_interrupt` so that it is being handled every timer tick, interrupts being disabled, and free of any possible race conditions. 

Then, for the first iteration, (1) we will observe and store which is the highest priority among the threads in the read_list. Then, (2) iterate through the ready_list once again and if priority of a thread equals to the highest priority value we stored from the first iteration insert into the `highest_priority_list`. Note that `highest_priority_list` is only composed of ready threads with highest priority. We have decided to take this approach as there are cases of multiple threads being the highest priority, and in such cases we would utilize `thread.c:schedule` to perform round-robin among these threads using the list constructed.

As `highest_priority_list` is fully constructed and being updated every tick by `timer.c:timer_interrupt`, we will use existing function `thread.c:next_thread_to_run` and `thread.c:schedule` to practically switch threads with the current running thread. Note that the thread being yielded will be pushed back to `ready_list` in `thread.c:thread_yield` and handled by our implementation whether to be placed on `highest_priority_list` in `timer.c:timer_interrupt`.

#### 2.3 Synchronization
To deal with the race condition of tracking the highest threads of the currently running (or in ready state) threads, we are iterating through non-waiting queues every timer tick. In this way, since the timer ticks for every short period of time, we are most likely to hold up-to-date value of the highest priority. In case when a thread with the higher priority than the highest priority of currently processing threads is fed into the process in between these timer ticks, since the timer is going to tick in a very short time, the highest priority will be adjusted as soon as possible.

Since the list of lock’s holders is stored inside of the lock structure itself, as we are acquiring a lock, we can access the content of such list. Also, we can safely manipulate any value associated with one of those holders because only one thread can enter the critical section and manipulate it at the time as the lock being acquired. Even though this thread is preempted, a newly running thread would possess a priority with higher than or equal to what the current one has, so in the case of the same exact lower priority thread is blocking the new thread’s process, new thread can just donate its priority, and that will eventually resolve the lower priority thread blocking the original thread. Using this property, we can process the priority donation inside of the `lock_acquire` function when a thread donates its priority to the blocking thread, and it is synchronization safe.

#### 2.4 Rationale
For each timer tick, we keep track of the threads’ highest priority by looping around the list of threads. We chose this implementation over figuring out the highest priority of the threads only during the process of priority donation, so that in case of new thread with higher priority is fed into the ready queue after our original highest priority is computed, we can recover our scheduler to run the appropriate thread within a tick or two.

We are going to implement priority donation logic in lock_acquire, since the lock has the information about the holders of it, so it is more straightforward to figure out from which thread to release the lock so that we can prevent our current thread (the one with the highest priority at the moment) to be deadlock or priority inversion.

When there are multiple threads with the same highest priority, we need the scheduler to run the round-robin on them, which is going to be handled in `next_thread_to_run`. Having the full list of the threads with the highest priority will simplify the process of running round-robin on them by letting us not to worry too much about the priority of the threads once the scheduler is done with picking the next threads to be run.

## Additional Questions

1. The program counter and saved registers of a thread are stored on the thread’s stack. When a context switch between two threads occurs, the thread that is being switched away from pushes its saved registers onto the bottom of its stack, and then yields control (The program counter is already naturally pushed onto the stack--it is the return address for after switch_threads finishes running, which is where execution should return to when the thread is switched back to. This return address is pushed onto the stack when switch_threads is called, right before the saved registers are pushed as well). The thread that is being switched to then likewise pops its saved registers back off from the bottom of its own stack, which were pushed there when that thread was itself switched away from and yielded control, and thus should still be at the bottom of the thread’s stack when it regains control. It then returns from switch_threads, which will restore its program counter to the address of whatever function was executing before switch_threads was called.

2. The stack pointer of a thread is saved as an attribute of the thread struct, which is allocated to the top of the thread’s stack when the thread itself is created. The offset from the top of the stack to where the stack pointer value is stored is stored in the register edx. When a context switch happens, the new thread retrieves its stack pointer by reading from the top of the stack offset by the value in %edx. This offset is the same for every thread, so the value in %edx is ALWAYS the same--it never changes. Thus, a thread can retrieve its stack pointer upon being switched to by loading from memory from the top of its stack with an offset of %edx.
    
3. The struct thread is freed in thread_schedule_tail(), which is called at the end of schedule(), which is in turn called at the end of thread_exit(). We cannot do this in thread_exit because this be the absolute last thing that occurs in thread_exit(), so it is placed at the end of the last function that is called during thread_exit’s execution. If it were to happen any earlier, the thread struct would be deleted during the middle of thread_exit while the function could still reference the thread struct later, potentially causing errors. On the other hand, it cannot be called in thread_exit() after schedule(), and thus thread_schedule_tail(), has finished running, because thread_schedule_tail will transfer control to a new thread after it finishes, so anything that is called after schedule() will not execute at all.
Thread_tick executes in the kernel stack, as it is called from the external interrupt context rather than by the current thread’s own context. 

4. **Test description:** 
Initialize a lock A and a semaphore with value 0.
Start with Thread 0 with priority 10 and start two threads Thread 1 with priority 30 and Thread 2 with priority 20. Then call thread_yield(). 
Thread 1 runs, acquires lock A and gets it, calls sema_down() and goes to sleep. 
Thread 2 launches Thread 3 with priority 40 and calls sema_down(), goes to sleep. 
Thread 3 runs and acquires lock A, find out that it is held by Thread 1, donates its priority to Thread 1 and goes to sleep.
Thread 0 wakes up and calls sema_up(), we test which thread wakes up.
**Expected output:**
Thread 1 wakes up because after donation, its effective priority is the highest.
**Actual output:**
Thread 2 wakes up because it has the highest base priority.

