Final Report for Project 2: Threads
===================================

## Changes since initial design:

### Task 1: 
 - Our original design maintained a new highest_priority_list seperate from the overall ready_list, which we would then modify the scheduling process in thread.c to use instead of the ready_list. This was ultimately determened to be unnecessary, as we could simply traverse the ready_list to find the highest-priority thread every time it was needed and modify next_thread_to_run and a handful of other relevant functions so it would only select the thread off the ready_list which had the highest priority, and this would probably take significantly less work than finding all references to the ready_list and replacing them with the highest_priority_list as appropriate. There were few other places that the highest_priority_list were being used, it is uncommon for many threads to exist at once (making traversing the ready_list slow), and it would frequently have to be recalculated anyways as it would not be very often that many threads shared the same, highest priority, so this should not lead to a significant slowdown.
 - Our original design attempted to do all the work of changing a thread's status and taking it off/putting it on the appropriate queue ourselves. This was unnecessary since the thread_block and thread_unblock functions were already usable in this context, so we switched to just using that instead. Our original design also did not yield after the thread was put to sleep (causing them to potentially do other work until the next timer tick, when they would no longer be scheduled until woken up again)--by switching to using thread_block (which yields the thread after blocking) instead of doing all that work manually, we also ensured that we would yield properly as well.

### Task 2: 
 - Our orignal design failed to account for synchronization around our changes to lock_acquire and lock_release--these two functions originally did not need any synchronization, but with our changes they now contain references to several shared resources such as each thread's held_locks list and each lock's lock_holder and waiters list. To solve this, we modified the lock_acquire and lock_release functions to disable interrupts during the critical sections in which these shared resources are accessed, ensuring synchronization.
 - Our original design failed to yield after creating threads with lower priority than the current highest priority, releasing a lock that causes a thread's priority to be reduced from the maximum priority, and changing the thread's priority to be lower than the highest priority. We now call thread_block every time any of these occurs, which will also yield the thread and then calculate and wake up the new highest-priority thread available.
 - Our original design waited until each timer tick to recalcuate the priority of running threads. This was an incorrect design as changes to priority need to happen immediately, lest an incorrect priority be used for the current timer tick--we have modified our implementation so these now happen immediately.
 - Our original design failed to account for semaphores and conditions--we have now modified both of these so that instead of simply waking up one of the threads waiting on them at random when a thread is woken up, they only wake up one of the threads waiting that hold the highest priority.
 - Our original design did not contain any handling for the case of nested donation. To solve this, we added an additional attribute `struct lock wait_lock` to each thread, noting which lock, if any, each thread was currently blocked waiting on, and the elements `int lock_priority` to each lock, noting the highest priority thread currently waiting on the lock. Then, whenever lock_acquire was called for thread `t` and lock `lock`, we would in order:
     - Update the lock's priority to be thread t's priority by setting `lock->priority=t->priority` (as thread t is always guarunteed to be of the highest priority, since it is a running thread)
     - If the current lock was held by a different thread, set `t=lock->holder`, replacing the value of t with the thread that is currently holding `lock`. Else, break.
     - If `lock`'s priority was higher than the new `t`'s priority, set `t->priority` to `lock->priority`, donating the priority of the first, higher-priority thread (which has already been loaded into the lock's priority) to this second, lower-priority one.
     - If the new current thread was waiting on some other lock (`t->lock != NULL`), then we would set `lock=t->lock`. Else, break
     - Finally, this process would be repeated until it was no longer the case that `t->priority>lock->priority`--this would ensure that so as long as any higher priority thread was waiting on any lock held by a lower-priority thread, our lock_acquire function would continue updating the priority of the lower-priority thread in this chain, and would only stop when it reached a thread that was not waiting on any other thread with lower priority than it. 

## Reflection
#### A reflection on the project – what exactly did each member do? What went well, and what could be improved?

##### Group Member Reflections and Recommendations:

Jungwoo Park 
 - What did I do: Designed with detailed inline comments for Task 1 and 2. Implemented Task 1 with comprehensive debugging as needed. Worked, debugged, and completed Task 3, Scheduling Lab Problem 1 and 2.
 - What went well: The process of working on design document was more productive and constructive for the next steps. Involving inline comments with specific place of code and which algorithms will be used was extremly helpful.
 - What could be improved: The current circumstances regarding COVID virus is causing the communication less effective as before. There was simply too much information and changes from the administration and courses to be handled. More communication and planning, even utilization of tools is needed for the upcoming project.

Brian Ahn 
 - What did I do: Designed the implementations for the project. Implemented Task 1 and priority scheduling of Task 2. Worked on Problem 1 and 3 of Scheduling Lab.
 - What went well: There wasn't really a conflict between the work that each of team member does this time, so nobody was really wasting their time on something meaningless. Also, everyone's efficiency and speed got improved compared to before as we are getting accustomed to the code base for this course.
 - What could be improved: Regarding the current situation, everyone is experiencing hard time on communication with other teammates. Luckily, no one's work was overlapped this time, so we could breeze it out without much confrontation. However, we should notify and update each other more often at least through Slack whenver someone is working on something especially we are in such harsh situation.

Cynthia Chen
 - What I did: 
Helped initial design and design change for task 2 especially the priority donation part. Changed the structs for task 2(struct thread, struct lock). Implemented the whole part of priority donation in synch.c and thread.c. Debugged the priority donation part to pass all the tests.
 - What went well: 
The overall coordination went well and everybody devoted their effort to make everything work in this tough time. We were discussing the ideas together whenever possible and everyone has a big picture of our work.
 - What could be improved: 
The communication between teammates became harder since we are not able to meet in person. So there were moments when the implementation diverged and don’t seem quite consistent.

Eddie Gao 
 - What did I do: Designed most of Task 1 and helped implement thread sleeping. Helped debug, redesign, and re-implement Task 2 after a major bug was encountered. Compiled team changes for the final report. 
 - What went well: We were able to collaborate fairly well by branching off into well-defined modular tasks--didn't encounter the sort of git merging issues and repeated work that we did last time.
 - What could be improved: Communication issues were a fairly constant problem over the course of this project, more or less to be expected given the quarantine. Testing and debugging was also EXTREMELY difficult due to the inability to use print statements while debugging the lock sections of task 2 and difficulties we faced with using gdb--would like to figure out how to solve that before working on project 3. 


