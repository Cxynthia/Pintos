Design Document for Project 1: User Programs
============================================

## Group Members

* Jungwoo Park <jwp@berkeley.edu>
* Brian Ahn <brianahn0717@berkeley.edu>
* Cynthia Chen <xinyi.chen@berkeley.edu>
* Eddie Gao <eddiegao98@berkeley.edu>

---

## Task 1

#### 1.1 Data Structures and Functions
We are going to create ```pintos/src/userprog/uputil.c``` including the following struct and helper function:
 - ```char* args_parser ( char *file_name, void** esp )``` :  takes string including executable name and arguments, then use ```strtoken``` to separate the executable name from the arguments. With the given information return the executable name and set the stack pointer according to the diagram in 4.1.6 of the spec accordingly.

#### 1.2 Algorithms
The implementation is going to iterate through the char pointer (string) ```file_name``` and temporarily store each character until it encounters a whitespace--delimiter. Once it encounters a delimiter, it would increment ```argc``` by 1 and copy the string then store in ```argv``` in order. Note that the function will validify each argument by checking if it contains illegal symbols, only contains flags, contains multiple executables, and so on. Also note that actual checking of loadability and runablity will be done later by syscall_handler. As a result, we would be able to construct a complete ```args```.

#### 1.3 Synchronization
The function is mainly going to be used in functions ```process_execute```, ```start_process```, and ```load``` located in ```pintos/src/userprog/process.c```. The implementation would simply be called within each function, and the information will be accessed by dereferencing the returned ```args``` from the implementation. For instance, the name of the executable can be accessed ```struct args = args_parser(file_name); char* executable = args[0];```. Note that such implementation will avoid the case of run down as the return value is a copied struct, therefore can be modified and shared safely. In addition, the implementation will not limit the concurrency as it does not involve multithreading in the process of parsing the information. 

#### 1.4 Rationale
The alternatives that are considered are the following:
 - Having the implementation within the existing file.
 - Use split-and-conquer algorithm to iterate through the input.

However, the current design is better because: 
 - Observing the frequent use of ```file_name``` across the code base and how ```filesys``` is also utilizing ```fsutil.c``` accordingly, it would be significantly efficient to fix or improve the implementation later as needed. Also such a structure of having a dedicated file suits the existing convention.
 - Simple iteration through the input is necessary and the most effective method in case of handling the given input as it is not required to sort and efficient to enforce validation as needed accordingly--easier to accommodate additional features. In addition, less coding is required to implement under this method.


## Task 2

#### 2.1 Data Structures and Functions
To implement our solution to task 2, we will create a ```wait_status``` struct in ```threads/thread.c```. This struct indicates the wait status of a process currently running on a thread, and has following attributes:
 - ```struct lock lock;```
 - ```int ref_cnt;``` : This is a reference counter indicating how many different processes point to the current process. It equals 2 if this thread and its parent are both alive as both processes will currently be referencing the process, 1 if either the thread or its parent are alive, and 0 if both are dead.
 - ```tid_t tid;``` : The thread_id of the thread in which the process is running
 - ```int exit_code;``` : The exit code of process, if it has completed
 - ```struct semaphore dead;``` : Set to 0 if the process is alive and 1 if it is dead
 - ```Char[] *ERROR;``` : A string that explains what error, if any, a thread has had that caused it to return -1. If the ```exit_code``` of the ```wait_status``` is not -1, then this will be null. This is not needed for the current implementation but will make debugging easier in the future.
 - ```children``` : a pintos list of ```structs``` which contains the ```wait_status``` objects of all the thread’s children.
 - ```waitlist``` : a pintos list of ```tids``` containing all the processes that a thread is currently waiting on
 
Additionally, we will add two new attributes to the ```struct thread``` in ```threads/thread.c```: 
 - ```wait_status``` : contains a ```wait_status``` struct indicating the wait status of the current process in the thread. If a thread is not currently running any process, its wait_status attribute should be null.
 - ```stacktrace``` : a pintos list of ```tid``` that keeps track of the stacktrace of the current thread--every time a thread is created, its stacktrace is set to the thread parent’s stacktrace plus the current thread’s ```tid```. This is not used for the current implementation but will make debugging easier in the future.

In ```thread.c```, when a process is initiated in a thread using process_excecute, a ```wait_status``` struct should also be created with ```ref_cnt=1```, a ```tid``` equalling the ```tid``` of the current thread, and ```dead=0```. Lock should start locked. Likewise, when ```thread_exit``` is called, it should remove the lock on the thread’s ```wait_status```, set ```dead=1```, and decrement ```ref_cnt``` by 1 in addition to everything else that it does. The exit code and ERROR need not be modified in ```thread_exit```--this will be done in the ```process_wait``` and ```process_exec``` functions.

If a thread is created by ```create_thread``` (that is to say, it is meant to be empty--not containing a process) then the thread’s wait_status should be null--this thread does not currently contain a process.

#### 2.2 Algorithms
For ```practice```:
 - In ```syscall.c```, if ```args[0] = SYS_PRACTICE```, increment ```args[1]``` by one and load it into ```f->eax```, print ```<t>: practice(<i>)``` where ```<t>``` is the thread id in which practice is run and ```<i>``` is the argument passed to it, and then call ```thread_exit```.
  
For ```halt```:
 - In ```syscall.c```, if ```args[0] = SYS_HALT```, print ```<t>: halt()``` where ```<t>``` is the thread id and then call ```shutdown_power_off()``` immediately
  
For ```exec```:
 - In ```syscall.c```, if ```args[0] = SYS_EXEC```, call ```execute_process(args[1])```, which will do the following:
   - If the current thread does not have a ```wait_status```, then create a new ```wait_status``` object with ```ref_cnt=1```, ```tid=current_thread()```, ```dead=0```, ```lock locked```, and all other attributes starting as ```null```.
   - Use process_excecute to create a new thread with in which the process to be executed will run, with argument ```file_name=args[1]``` (so ```process_excecute(args[1])```)
     - If process_excecute returns a TID_ERROR, the child process has failed to load--return a pid/tid of -1 and explain the situation in the ERROR attribute of the current thread’s ```wait_status```
     - If process_excecute raises an assertion exception, the child process has also failed to load--catch this error and return a pid/tid of -1 and explain the situation in the ERROR attribute of the current thread’s ```wait_status```
   - Add the ```tid``` of the child thread returned by process_excecute to the children list of the current thread
   - Increase the ref count in the current/calling thread’s ```wait_status``` attribute by 1.
   - Set a semaphore down on the CHILD thread’s dead attribute (so ```sema_down(current_thread->wait_status->children[i]->dead)```) to have the current process blocked until the child finishes running.
   - When the child process exits, it will set its dead attribute to 1, which will wake the current/parent process.
     - NOTE: Cases where the child process does not halt are a concern, so there should also be code in execute_process right before calling the semaphore down that will terminate the process and all its children (by freeing all the thread objects associated with them), writing a timeout error in ERROR, and returning -1after an arbitrary large amount of time has passed without the child function returning.
   - When the child process exits, it will set its dead attribute to 1, which will wake the current/parent process.
   - Check the child’s ```wait_status``` (retrieving the child’s thread object using its tid, which is returned by ```process_excecute```) to determine if the child returned successfully using the return status attribute in the wait status struct. 
     - If the child’s ```wait_status``` struct for some reason cannot be accessed (for instance, a segmentation fault when trying to access the child’s wait_status), assume that there has been an error that has happened to child and that it has failed, so set the exit code of the parent to -1 and explain the issue in ERROR.
   - If so, return the child’s pid. 
   - If not, the child has failed to run successfully, so return -1 and set ERROR to the ERROR attribute of the child.
   - Before returning, free the child’s wait_status struct
 - After returning, call ```thread_exit()```  

For ```wait/process_wait```:
 - If ```args[0]=SYS_WAIT```, call ```process_wait(args[1])```, which will do the following:
   - If the current thread does not have a ```wait_status```, then create a new wait_status object with ```ref_cnt=1```, ```tid=current_thread()```, ```dead=0```, ```lock locked```, and all other attributes starting as ```null```.	
   - Iterate through the current thread’s list of children to find child whose ```pid=args[1]```.
     - If it does not exist, ```return -1``` and note that the current child process could not be found in ERROR.
   - Add the ```tid``` of the child in question to the waitlist attribute of the current thread’s ```wait_status```
     - If the ```tid``` of the child in question is already in the waitlist list, then the process is already waiting on the child in question, so waiting on it again is invalid--return -1 and note this error in ERROR. 
   - Set a semaphore down on the CHILD thread’s dead attribute (so ```sema_down(current_thread->wait_status->children[i]->dead)```) to have the current process blocked until the child finishes running.
   - When any child process currently being waited on exits, it will set its dead attribute to 1, which will wake the current/parent process.
     - NOTE: As in ```exec```, if no child returns after an arbitrary large amount of time, we will free the current process and all its children, note a timeout error in ERROR, and then return -1.
   - Retrieve the wait_status struct of the child from the children list and then decrement the ref_count of the child that was returned (that is to say, the first child being waited on that exited), as the child is now dead.
     - If the child’s wait_status struct for some reason cannot be accessed (for instance, a segmentation fault when trying to access the child’s wait_status), assume that there has been an error that has happened to child and that it has failed, so set the exit code of the parent to -1 and explain the issue in ERROR.
     - If ref_cnt is not any of 0, 1, or 2, there has been an error, so set the exit code of the parent to -1 and explain the issue in ERROR.
   - Free the child’s wait_status struct
   - If the child has an exit_code of -1 or an ERROR attribute not equal to null, then the child failed and was terminated by the kernel rather than exiting properly--this means that the current thread should also return -1, and copy the ERROR attribute of the child into its own ERROR attribute.
   - Otherwise, return the exit_code attribute of the child’s wait_status, 
 - After ```process_wait``` returns, call ```thread_exit()```.      

#### 2.3 Synchronization
 - A thread’s wait_status object is shared by both the thread itself and its parent--locking is used to prevent any conflicts here.
   - The wait_status object associated with each thread has a lock, indicating that a process is currently being run on the thread for which that wait_status is associated with. A thread’s wait_status can only be accessed by other threads after the thread has either exited or has been terminated by the kernel, and therefore releases that lock. 
     - This prevents race conditions, as only two threads can access a thread’s wait_status--the thread itself and its parent. With this lock, the parent cannot access the wait_status until the thread has returned, so there is no conflict.
     - This prevents deadlocks, as a parent will only ever be waiting on a lock for its child. A thread cannot set it’s child to any earlier thread that has already been created, and every child thread is therefore a completely new thread. Therefore, a “cycle” of locks--which can only happen if a thread’s child is actually somewhere in its own parentage lineage--is not possible. Furthermore, a lock is only ever released ONCE and cannot ever be locked again, and after a lock is released its process has already finished all its work and either exited or terminated, meaning that it can cause no race conditions.
     - This locking strategy is not coarse--each process/wait_status has its own separate lock
 - This implementation assumes that Pintos is single-threaded and thus makes no attempt to account for parallelism. Beyond that, the concurrency of the user processes or the kernel should not be significantly impeded--each thread’s execution depends ONLY on the execution of its own children, so other unrelated threads running at the same time should not cause the current thread to slow down.   
   - A thread only ever shares resources (its own wait_status) with its parent, so threads will not frequently contend on shared resources
   - There is no limit on the amount of threads that can call any of these syscalls at any given time.
 - Time and space are not generally a major concern in this implementation, as the wait_status struct which has been added is not very large. Problems only arise when one thread has MANY children, as the largest space bottleneck in our implementation is the size of the children list and the largest time bottleneck is traversing the children list to find the right child. This should be relatively atypical behavior, and in a more typical case where children are distributed roughly evenly across parents there should be no time/space issues.
   
#### 2.4 Rationale
 - Pintos list is used over linked list for simple ease of use.
 - Since Pintos can only ever run one thread at a time, and a thread can only ever run one process at a time, it is not necessary to differentiate threads and processes--we can treat tid and pid interchangeably. This will make extending our operating system to a multithreaded implementation more difficult if it ever happens, but makes this current implementation much simpler. 
    - As an extension of this, there is no need for a separate process struct--whether or not a thread currently contains a process is indicated by whether it contains a wait_status attribute, and relationships between processes (such as parent/child) are identical to the relationships between the threads they are contained in.
 - There is no need for any complicated algorithms to avoid deadlocks because, as Pintos is single-threaded, parallelism is not a concern, and because the way threads are created in the current implementation, it is not possible to set a thread’s child to any already existing thread. The result of this is that deadlocks are not possible unless a thread’s child, or some other thread in its child lineage, halts--to handle that case, a timeout limit is added so no thread is allowed to run indefinitely.
 - Each wait_status object (the only shared resource in this implementation) being shared only by a child and a parent allows for relatively few concurrency problems, as there is not much competition between many threads over the same shared resource. This does mean that it is not possible for a thread to access information about a process inside any thread that is not its child, but this is not necessary for the implementation of any of the parts of Project 1.
 - The addition of the ERROR string/Char[] and the stacktrace attributes are not necessary to complete the functionality in the spec, but will provide extra information for debugging which could make future work with this operating system much easier.
 
 
## Task 3

#### 3.1 Data Structures and Functions
 - ```struct file_entry_list``` : In ```syscall.c```, use a ```list struct file_entry``` to keep track of all the opened files with ```struct *file```, ```char *name```, and ```struct list_elem elem``` inside of it. 
 - ```get_file_with_fd(int fd)``` : Using ```fd``` as the index number of the list, by iterating through the ```file_entry_list```, we can get the corresponding file struct, which we can use in the functions implemented in ```filesys.c```
 - ```int num_files``` : Initiate to 0. Use this value to keep track of the number of opened files


#### 3.2 Algorithms
Since all the basic logics for each required syscall functionality are already prepared, we can just simply call the appropriate functions from the library in each function to get the desired behavior.
 - Create: call ```struct file * filesys_create(const char *name, off_t initial_size)``` function located in ```filesys.c``` by passing in the parameter from the invocation of ```struct file * create(const char *file, unsigned initial_size)```. If the returned value from the above invocation is not equal to ```NULL```, that means the create operation was successful and we can return ```true``` as the return value of create function. Otherwise, return ```false```.
 - Remove: call ```bool filesys_remove(const char *name) in filesys.c``` function located at ```filesys.c``` by passing in the parameter from the invocation of ```bool remove(const char *file)```. We can simply return what we got by invoking ```filesys_remove``` as the return value of this remove function.
 - Open: call ```struct file * filesys_open(const char *name)``` function located at ```filesys.c``` by passing in the parameter from invocation function ```int open(const char *file)```. From the returned file struct of ```filesys_open```, we can pull out ```file->pos``` and pass that as a return value of open function.
 - Filesize: First, retrieve the file pointer from the ```file_entry_list``` by invoking ```get_file_using_fd``` with the ```fd``` we received from the function call of ```int filesize(int fd)```. Then, we can pass the returned file pointer into ```off_t file_length(struct file *)``` function located in ```file.c``` and return what we got from this function call as a return value of filesize function.
 - Read: Retrieve the file pointer by invoking ```get_file_using_fd``` using the given ```fd``` value. Then, call ```off_t file_read(struct file *file, void *buffer, off_t size)``` function located in ```file.c``` and just return what we got from this function call as the return value of the read function. 
 - Write: Retrieve the file pointer by invoking ```get_file_using_fd``` using the fd value we received from the function call of ```int write(int fd, const void *buffer, unsigned length)```. Then, call ```off_t file_write(struct file *file, const void *buffer, off_t size)``` function located in ```file.c``` and just return what we got from this function call as the return value of the write function.
 - Seek: Retrieve the file pointer by invoking ```get_file_using_fd``` with the given ```fd``` value. Then pass this file pointer along with the position value into ```void file_seek(struct file *file, off_t new_pos)``` and we are done.
 - Tell: Retrieve the file pointer by invoking ```get_file_using_fd``` with the given ```fd``` value. Then pass this file pointer into ```off_t file_tell(struct file *file)``` and pass the result of this function call as the return value of tell function.
 - Close: Retrieve the file pointer by invoking ```get_file_using_fd``` with the given ```fd``` value. Then pass this file pointer into ```void file_close(struct file *file)``` and we are done.
 
#### 3.3 Synchronization
As mentioned in the spec, the Pintos file system is currently not thread-safe and we do not want each file operation syscall to interrupt each other. Since we are allowed to use global lock for now, we can utilize it by locking the entire file system once a single file operation syscall is being initiated, so that the other file syscall does not interrupt into this process. 

We can also use ```file_deny_write()``` and ```file_allow_write()``` separately in ```start_process()``` and in ```exit()``` to ensure that the writes to current-running program files will be denied.

#### 3.4 Rationale
We used the ```struct file_entry_list``` as the structure to tack all the open files. The num_files keeps track of the number of open files, we increment this value by 1 every time we open a file, and subtract it by 1 when we close a file. So when the number goes back to 0, we can free the structure, which saves memory for other operations.

---

## Additional Questions

#### 1. Please identify a test case that uses an invalid stack pointer (%esp) when making a syscall. Provide the name of the test and explain how the test works.
The ```child-bad``` test uses an invalid stack pointer when making the syscall. The line 12, ```asm volatile ("movl $0x20101234, %esp; int $0x30");``` invokes instructions which one of them, ```movl $0x20101234, %esp```, is moving long 0x20101234 into the esp (stack pointer). Note that the value is an invalid stack pointer as it’s pointing beyond the boundaries not as it’s supposed to. Then, the instruction ```int $0x30``` causes an interrupt with the number 0x30. Such interruption stops the user program then switches from user mode into kernel mode then invokes syscall_handler to handle. If the above instructions are not making the appropriate syscall, in this case ```exit(-1)```, would then lead to line 13 which would result the test in failure with the message ```“should have exited with -1”```.  

#### 2. Please identify a test case that uses a valid stack pointer when making a syscall, but the stack pointer is too close to a page boundary, so some of the syscall arguments are located in invalid memory. Provide the name of the test and explain how the test works.
The ```sc-boundary-2``` test uses a valid stack pointer but which is too close to a page boundary. First, note that the function call ```get_boundary_area()``` returns the beginning of a page. Then, because the stack is growing downwards the line 15 is negating the value 7 from the beginning of a page in order to provide just enough space for syscall arguments. Also note that the pointer ```p``` being a pointer to integer, each element takes up 4 bytes. Moving onto line 20, it uses asm extended getting the use of input operand ```g (p)```. Note that ```g``` represent general registers while ```(p)``` would invoke the value stored in ```p```. As the asm extended specifies, ```movl %0, %%esp``` would move the long stored in ```%0```, ```p``` in this case into the esp. Then, the instruction ```int $0x30``` causes an interrupt with the number 0x30. Such interruption stops the user program then switches from user mode into kernel mode then invokes ```syscall_handler``` to handle.

#### 3. Identify one part of the project requirements which is not fully tested by the existing test suite. Explain what kind of test needs to be added to the test suite, in order to provide coverage for that part of the project.
Provided testing suite is missing tests for some of the requirements of task 3. One of those is remove functionality, and we can make a similar approach to how the create functionality was tested in the testing suite in order to provide coverage for testing of this functionality. 
 - First, we can check if the normal flow of remove functionality works by creating an empty file and then simply removing it. This should not fail.
 - Next, we can test whether the remove function works properly for the null pointer argument. This should fail with exit code -1.
 - Also, when a bad pointer is provided, the process should be terminated with exit code -1.
 - When trying to remove a not existing file, the test should fail.