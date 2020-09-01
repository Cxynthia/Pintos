Final Report for Project 1: User Programs
=========================================

## Changes since initial design:

### Task 1: 
 - The major changes we made on task 1 is how we are now involving a stack pointer in the process. The initial design was only to manipulate a given char* into two char* of executable and arguments. Now, the new design -- a helper function ```arg_parser``` -- takes executable names, arguments, and stack pointer when a load call is made in ```start_process```. With such arguments, the function using the given stack pointer store them on stack accordingly to a diagram depicted in part 4.1.6 of spec: growing downward, actual value is being stored in reverse, then 16-byte alignment is made, and addresses of each value is stored followed by ```argv```, ```argc```, and return address. ```strtok_r```, ```memcpy```, ```strlcpy``` is utilized in the process.

### Task 2: 

 - Our original design wanted to allow for a parent to be able to wait on multiple children at a time, due to a misreading we made of the project spec that we thought indicated that this case could be possible. In reality, wait is a blocking call for the parent process, so it is not possible for it to wait on multiple different children--as such, we changed our design so we no longer create ```waitlist``` of the children and insert the children whenever ```process_wait``` is called upon them. Instead, we are using the flag inside of the child’s ```wait_status``` to verify whether the parent already called ```wait``` for that child or not.
 - Our original design had the ```exec``` syscall wait until the child process returned before returning the exit status--this was incorrect, as ```exec``` only waits to see if the child loads, not if it returns, and we updated our implementation accordingly. Likewise, we did not initially have a dedicated semaphore to indicate whether the process had finished _loading_ in our design, as we thought we could simply rely on the ```dead``` semaphore of our wait statuses which indicated if a process had returned or been killed. This was added in as we changed our implementation.
 - Our original design maintained a list of the wait status struct of a parent’s children inside the ```wait_status``` struct, but this made it potentially unsafe to free a child’s wait status after it concluded running. As a result, we moved the list of a thread’s children outside the wait status struct and made it a direct attribute of the thread struct. We also moved from including the ```wait_status``` struct as an attribute of a thread--which would also cause issues if a thread was freed--to storing a reference to a wait status struct in the thread.
 - Our original design neglected the changes that were necessary to make to thread_exit to make the rest of our implementation work, as we had assumed that thread_exit was not supposed to be modified. We have now updated our implementation so that upon thread_exit being called, we obtain the exit code of the thread, up the shared semaphore that allows other threads to determine if that thread has concluded, and decrement the ref count of the thread and all its children, freeing them if the rec count reaches 0.
 - Our original design attempted to account for cases where a thread was created but did not contain a process (and thus a wait status struct), but this was unnecessary as every thread that is created contains a process. As such, we removed the original checking that we were doing to determine whether or not a wait status should actually be created when a thread is from our final implementation.
 - Parent process calling the ```process_wait``` on its child is no longer following the exit code from its child because the purposes of their operations are different, and in fact, the child’s exit code is just a return value of ```process_wait``` function. While the child is executing whatever the syscall that it was commanded to do so, the parent is waiting for the child to be done, and once the response is returned, the parent’s process was successful and exit code should be 0 instead. 
 - Used ```load_semaphore``` for ```exec``` syscall and ```wait_semaphore``` for ```wait``` syscall separately for the separation of privilege.


### Task 3: 
 - For the ```struct fileEntry```, we moved the num_files variable into the struct to keep track of the number of threads that are having access to the file, and added a ‘removed’ variable which shows if the file has been removed so that it could no longer been accessed by a new thread. We added a ```struct fileDescriptor``` to map the fd number to the corresponding file struct and used a file_descriptor list to keep track of all the accessible files of one thread. Also, add a variable ‘next_fd’ to every thread which is the next integer that could be assigned to a file. To get the file entry from the file_entry list, we added the helper function ```get_file_entry_from_file_entries```. 

## Reflection
#### A reflection on the project – what exactly did each member do? What went well, and what could be improved?

##### Group Member Reflections and Recommendations:

Jungwoo Park 
 - What did I do: Managed overall version control and git use within the group. Collaborated with Brian on coming up with the pseudocode for task 1. Then, implemented and debugged task 1 fully passing the given tests. Based on pseudocode and the very first drafts of task 2 and 3, implemented core structure of the code and passed the majority of the tests. Later participated in debugging task 3 to pass the given tests.
 - What went well: Active communication using slack and meeting up in study rooms at libraries. Every member is passionate about the task and willing to contribute to the work.
 - What could be improved: Usage of git can be improved -- more frequent commit and pull, more detailed commit message, and branch management. More allocation of time on investigating and updating design documents, also on additional tests.

Brian Ahn 
 - What did I do: Came up with the general design idea of implementing various file syscalls for Task 3, and participated in debugging for it later with Cynthia. Collaborated to come up with the initial version of argument parsing logic in Task 1. Implemented ```process_wait``` for Task 2.
 - What went well: Everyone was really dedicated so that we were sacrificing our whole weekends to work on this project. Every time we planned to meet-up, either in person or over the screen, everyone showed up, which was pretty impressive. Also, to pass even a single test, our team was staying up all night, and in the end, we made it.
 - What could be improved: As we were running out of time, we primarily focused on its functionality instead of optimizing or style-checking our code. Considering how much we could have accomplished in a short amount of time, if we planned ahead, I think we would have done even more without everyone being exhausted near the deadline. From the next project, we should at least read the spec and have the big picture in mind, so that everyone is on track and the collaboration becomes a breeze.

Cynthia Chen
 - What did I do: Worked on the design doc for task3 part with Brian. Helped changing the design for task 3 after the design review. Implemented the filesize(), read(), write(), seek(), tell() and data structure changes in task3 file system. Helped with debugging the bad input situation tests. Also worked on wait() for some time with Brian.
 - What went well: Everyone in our team was trying to dedicate more to the project whenever they have time. Working together in the study room was really efficent.
 - What could be improved: We should start the design doc and the project earlier. Getting a comprehensive understanding and discuss about the concepts before starting the design doc is also important. Making a good use of the office hour is also important, which could help to make the idea each part of the project to be more clear.

Eddie Gao 
 - What did I do: I was primarily responsible for implementing the task 2 syscalls, collaborating with Brian on wait and implementing the rest on my own. I also helped with implementing task 3, writing the first drafts for create, open, and remove, debugging a significant portion of the task, and helping refine our implementation from the design doc in response to feedback from our TA review. 
 - What went well: Inter-team communication and collaboration was very solid throughout the project. Our group was very good at explaining concepts to each other and working together on code.
 - What could be improved: I personally had a number of issues with using git for the project which caused some confusion and accidental overwrites during the middle of the project. Debugging was also a consistent frustration, as gdb was frequently extremely difficult or impossible to use for our group--in the future we should get more familiar with these tools so we can debug with more sophisticated methods than just using print statements.


