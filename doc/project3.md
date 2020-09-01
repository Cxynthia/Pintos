Design Document for Project 3: File Systems
===========================================

## Group Members

* Jungwoo Park <jwp@berkeley.edu>
* Brian Ahn <brianahn0717@berkeley.edu>
* Cynthia Chen <xinyi.chen@berkeley.edu>
* Eddie Gao <eddiegao98@berkeley.edu>

## Task 1: Buffer Cache

### 1.1: Data Structures and Functions

#### Data Structures:
In inode.c, create the following:

`Struct cache_element {
    list_elem cache_elem;
    
    bool is_dirty; // if the file is edited
    
    bool is_new; //old bit for clock algorithm
    
    void *buffer;
    
    block_sector_t sector;
    
    struct block fs_device;
} `

In inode.c, create the global variable `struct list cache`

In inode.c, create the global variable `struct lock cache_lock`

#### Functions:
In inode.c, create the following: 

`struct cache_elem add_to_cache (struct block device, block_sector_t sector, void *buffer);`

`void read_cache (struct block device, block_sector_t sector, void *buffer);`

`void write_cache (struct block device, block_sector_t sector, void *buffer);`

### 1.2: Algorithms

#### `inode_init`:
Initialize `list_init(cache)` and `lock_init(cache_lock)`

#### `add_to_cache`:
Iterate through `cache`. For every entry `cache_elem` in `cache`, check to see if `cache_elem->sector == sector` and `cache_elem->fs_device = device`.
If a matching `cache_elem` is found, set `cache_elem->is_new = true` and return `cache_elem`
If no matching `cache_elem` is found: 
 - Call `block_read(device, sector, buffer);`
 - Create new struct `struct cache_elem new_elem`
 - Set:
     - `new_elem->is_new = true`
     - `new_elem->sector = sector`
     - `new_elem>fs_device = device`
     - `new_elem->is_dirty = false`
     - `new_elem->buffer = buffer`
 - Add new_elem to cache
 - If `list_size(cache) > 64`, create new variable `list_elem clock_elem = list_begin(cache)`. 
     - While true, do the following:
         - `struct cache_elem clock_cache_elem = list_entry(clock_elem)`
         - If `!clock_cache_elem->is_new`:
             - Break the loop
         - Else:
             - `clock_elem->is_new = false;`
             - `clock_elem = list_next(cache, clock_elem);`
         - If `clock_elem = list_tail(cache)`, set `clock_elem = list_begin(cache)`
         - If `clock_cache_elem-->dirty = true`, call `block_write(clock_cache_elem->fs_device, clock_cache_elem->sector, clock_cache_elem->buffer`
     - Return `new_elem`
     
#### `read_cache`:
Acquire cache_lock with `lock_acquire(cache_lock);`
Set `struct cache_elem read_element = add_to_cache(device, sector, buffer);`
Set `buffer = read_element->buffer`
Release cache_lock with `lock_release(cache_lock);`

#### `release_cache`:
Acquire cache_lock with `lock_acquire(cache_lock);`
Set `struct cache_elem written_element = add_to_cache(device, sector, buffer);`
Set `written_element->buffer = buffer`
Set `written_element->is_dirty = true`
Release cache_lock with `lock_release(cache_lock);`

#### `inode_read_at`:
Replace all instances of “block_read” with “get_cache”

#### `inode_open`:
Replace all instances of “block_read” with “get_cache”

#### `inode_write_at`:
Replace all instances of “block_read” with “get_cache”
Replace all instances of “block_write” with “release_cache”
    
### 1.3: Synchronization
The use of the `cache_lock` around the shared cache list whenever it is accessed (either through `read_cache` or `write_cache`) ensures that the cache remains synchronized. There are no other shared variables in use in this implementation, so this should ensure all synchronization.

### 1.4: Rationale
The clock algorithm was used here because it was by far the easiest to implement, and did not require any sorting of the cache list as LRU would that would increase time complexity--with clock, it is guaranteed that any read or write will finish in O(128) time, as the cache is at most 64 elements long.

## Task 2: Extensible Files

### 2.1: Data Structures and Functions

#### Data Structures:
`struct inode_disk {
struct list disk_data; (previously, it was block_sector_t start)
off_t length;
unsigned magic;
uint32_t unused[125];

// Below fields are for consistency of file during write operation.
struct list buffer_disk; // This list will temporarily buffer the list of newly written data
int buffer_disk_start; // buffer_disk_start indicates in which index of inode_disk the buffer is effective
`

#### Functions:
`inode_disk_buffer (struct inode *inode, const void *buffer_, int sector_idx) `
This will be used as a helper function of `inode_write_at`.

`inode_disk_flush (struct inode *inode)`
This will be used as a helper function of `inode_write_at`

### 2.2: Algorithms

#### `inode_create`
We need inode to be a first fit block rather than a single contiguous set of blocks. To do so, feed 1 as the first argument of `free_map_allocate` function and make a loop call of it for each consecutive sector to be assigned the first block available, and `block_write` zeros for each block in each iteration.

#### `inode_read_at`
Handling of EOF:
 - If the value of the provided offset is larger than the length of the specified inode, then this function should return 0 since no bytes have been read.
 - Else, compare the position of our current pointer inside the inode_disk and its length as we read through the file. Whichever we reach first, the end of file or `offset + size`, we stop the operation and return the bytes that we have read so far.
 
#### `inode_write_at`
Handling of EOF:
 - If the offset we provided for write operation is beyond the EOF, we zero out the entire gap and perform the regular operation afterward.
 To keep our file system in a consistent state, we are going to buffer data into `inode->buffer` by `inode_disk_buffer` function, and if we successfully reach the end of the function, we will call `inode_disk_flush` function.
 
#### `inode_disk_buffer`
`inode_disk_buffer` will take the inode and the buffer pointer for read operation. `BLOCK_SECTOR_SIZE` bytes will be read off of the buffer and stored into `inode->buffer`.
If `buffer_disk_start` inside `inode_disk` contains invalid value, such as -1, then set this with the provided `sector_idx` value.
 
#### `inode_disk_flush`
`inode_disk_flush` will replace the sub-list of `disk_data` indexed from the `buffer_disk_start` to `buffer_disk_start + list_size(buffer) -1`. If EOF is passed, just replace it from `buffer_disk_start`.
If `buffer_disk_start` is beyond EOF of `disk_data`, the gap is already zeroed out, so we don’t need to worry about it much.
`block_write` function will be called in this function rather than in `inocde_write_at`
When this function is called, empty out the buffer list and set `buffer_disk_start` to something invalud, such as -1.

### 2.3: Synchronization
Whenever file is opened in the current file system device, inode information will be stored into the `struct block *fs_device`, which is defined in `filesys/free-map.h` and initialized in `filesys/inode.c`. This system device pointer will be shared across all the threads, which would be referred to or modified whenever filesys operations are called in a process.
By separating the `inode_write_at` operation into `inode_disk_buffer` and `inode_disk_flush`, we can keep the consistency of the files. Since we are only storing the newly written disk_data temporarily until it is confirmed that we are safe to manipulate the file for the `write` operation, we can keep the consistency of the original file in case something went wrong during the operation.
### 2.4: Rationale
Original inode related implementations are based on the assumption that the Pintos file system is allocating each file as a single contiguous set of blocks, so the `inode_disk` was just holding the starting address of this contiguous set. To allow the Pintos file system to support extending files, we are now making `inode_disk` to hold the list of the addresses and iterate through this list to retrieve the sector at the indexed position rather than directly referencing it by address arithmetic. This way, fragmentation, the reason why extensible file was not allowed in the original implementation, is no longer our concern for the file extending.
By splitting the `inode_write_at` function into `inode_disk_buffer` and `inode_disk_flush`, it will be more straightforward to handle consistency of the file even when something goes wrong. 

## Task 3: Subdirectories

### 3.1: Data Structures and Functions

#### Data structures
Add the attribute `struct dir working_directory` to the thread object
Add the attribute `dir parent_directory` to to the thread object to keep track of the current parent directory
Add the attribute `lock directory_lock` to the thread object, which will ensure synchronization of the working_directory
Create a new attribute `struct dir *parent` in the dir struct, which will keep track of a directory’s parent. Initialize to NULL
Create the new attributes `struct dir *directory` in file_descriptor. And `bool isDir`

#### Functions
In directory.c, create the function `dir resolve_dir_lookup(const char *name, struct inode **inode)`, which will encapsulate the process of looking through the directory tree to find a file
In syscall.c, add the functions `bool isdir(int fd)`, `bool chdir(const char *path)`, `bool mkdir(const char *path)`, `int inumber(int fd)`, and `bool readdir(int fd, char *path)`
In filesys.c, add the functions `filesys_open_directory`

### 3.2: Algorithms
#### `thread_create`:
Immediately after creating a new thread in `thread_create`, set `t->working_directory = current_thread()->working_directory`, `t->parent_directory = current_thread()->parent_directory`, and `lock_init(t->directory_lock)`.
 
#### `thread_init`:
When the initial thread is created, set `t->working_directory` to the root directory and `t->parent_directory` to the root directory as well.
 
#### `resolve_dir_lookup`:
Acquire `directory_lock` using `lock_acquire`.
Declare a new variable `struct inode *inode`, initialized to null
Declare `char *rest = name;`
Create variable `struct dir *dir`, initialized to current_thread()->working_directory;
Check the first character of `name`, if it exists. If the first character is “/”, set `dir=dir_open_root ();`, declare a new string `str temp[strlen(name) - 1]`, copy all contents of `name` but the first character into `temp`, and then set `name = temp`.
Set `char * token_iterator = strtok(name “/”, &rest)`. Then, while token_iterator != NULL:
 - If `token_iterator = “.”`, set dir=current_thread()->working_directory and set `token_iterator = strtok(NULL, “/”, &rest)`
 - If `token_iterator = “..”`, set dir=current_thread()->parent_directory and set `token_iterator = strtok(NULL, “/”, &rest)`
 - Else:
    - Call `dir_lookup(dir, token_iterator, &inode);`. If it returns false, release `directory_lock`, and return false
    - Set `token_iterator = strtok(NULL, “/”, &rest)`
    - If `token_iterator != NULL`, set `struct dir next_dir = dir_open(inode)`. If `next_dir != NULL`, set `next_dir->parent = dir`, call `dir_close(dir)`, and set `dir = next_dir`--this will ensure that traversed intermediate directories are properly closed, that traversed dir is able to keep track of its parent, and that, if the last inode is a file at the end of a path, dir will not be set to it. 
Release `directory_lock` and return dir
 
#### `filesys_open` and `filesys_remove`:
Remove the line `struct dir *dir = dir_open_root ();` and replace with the lines:
    `struct inode *inode = NULL;
        struct dir *dir = resolve_dir_lookup(name, &inode`
In `filesys_open`, also remove the lines:
    `if (dir != NULL) 
        dir_lookup (dir, name, &inode);`
 
#### `filesys_open_directory`
`filesys_open_directory` should be identical to filesys_open, save for calling dir_open instead of filesys_open at the end.

#### `open`:
At the beginning of open, declare a new variable `struct inode *inode`, initialized to null. Call `resolve_dir_lookup(ufile, inode)`. If dir_open(inode) != null, then the file provided is a directory, not a normal file. In this, perform the existing open syscall, but using `filesys_open_directory` instead of `filesys_open` and setting `fd->directory` to be the new, opened directory while leaving `fd->file` null, and set the new file_descriptor’s `fd->isDir=true`. Else, proceed as normal and set `fd->isDir = false`.

#### `close`:
Before calling `file_close (fd->file);`, check if `fd->isDir = true`. If it is, then call `dir_close(fd->directory);` instead. Else, proceed as normal.

#### `remove`
No changes are required--filesys_remove is already agnostic as to whether a file or a directory is being removed

#### `exec`
Before any of the existing code in exec, check to see if the provided path leads to a directory using the same method as in `open`. If it is, then do not execute the rest of the syscall, and immediately return 0--a directory cannot be executed. 

#### `isdir`:
Return lookup_fd(fd)->isDir

#### `chdir`:
Acquire `directory_lock`
Declare a new variable `struct inode *inode`, initialized to null
Traverse the string `path` and check to see if it contains any instances of `/`. If it does not:
 - If `path=.`, do nothing
 - If `path=..`, set `current_thread()->working_directory = current_thread()->parent_directory` and `current_thread()->parent_directory = current_thread()->working_directory->parent`. If parent==NULL, this is the root directory, so simply do nothing and return 0 instead.
 - Else, declare new variables `struct dir *new_dir` and `struct inode *inode` and call `new_dir = dir_lookup(current_thread()->working_directory, path, &inode)`. If new_dir != NULL, set `current_thread()->parent_directory = current_thread()->working_directory` and `current_thread()->working_directory = new_dir`, and set `current_thread()->working_directory->parent = current_thread()->parent_directory`. Else, do nothing and return 0, as the provided path does not lead to a directory.
Else, if it does:
 - Declare `struct dir *dir = resolve_dir_lookup(path, &inode)`. If dir=NULL, the lookup failed, so return 0.
 - If dir_open(inode) == NULL, return 0, as the last element on the provided path is a file, not a directory. Else:
    - Set `current_thread()->working_directory = dir`
    - Set `current_thread()->parent_directory = dir->parent`
    - Release `directory_lock` and return 1

#### `mkdir`:
Acquire `directory_lock`
Declare `struct dir new_dir`
Declare `struct dir *host_dir`
Declare a new variable `struct inode *inode = NULL`,
Declare new variable `struct block_sector_t inode_sector = 0`
Traverse the string `path` and check to see if it contains any instances of `/`. If it does:
 - Truncate the last element of path from the path variable and set it as the new variable `target_directory`. For instance, if `path=path/to/here`, path would become “path/to” and target_directory would become “here”
 - Declare `struct dir *dir = resolve_dir_lookup(path, &inode)`. If dir = NULL, the lookup failed, so return 0.
 - If dir_open(inode) == NULL, return 0, as the last element on the provided path is a file, not a directory. 
Else:
 - Set `host_dir = current_thread()->working_directory`
Set `new_dir = dir_lookup(host_dir, path, &inode)`
If new_dir != NULL, return 0
Else, call ‘bool success = free_map_allocate (1, &inode_sector)’. If `success = false`, return 0
Else, do: 
`bool success = dir_create(inode_sector, 16);
    block_read(block_get_role (BLOCK_FILESYS), inode_sector, &inode);
    dir_add(current_thread()->working_directory, path, inode)` 
If `success=false`, else, return 0.

#### `inumber`:
If `lookup_fd(fd)->isDir` = true, return `lookup_fd(fd)->directory->inode->sector`
Else, return `lookup_fd(fd)->file>inode->sector`

#### `readdir`
If `lookup_fd(fd)->isDir` = true, return `dir_readdir(lookup_fd(fd)->directory, name)`. Else, return false.

### 3.3: Synchronization
The use of a lock around the global variable `current_thread()->working_directory` on every access to it (which can occur only in resolve_dir_lookup) ensures synchronization around it. There are no other relevant shared variables for this section. 

### 3.4: Rationale:
Keeping track of a separate parent_directory attribute should allow for easy locating of any given directory’s parent. The parent directory of any new directory that is moved to can always be determined through chdir (if it is a path, then the parent is the penultimate element of the path. If it is not a path, then it is the current directory) unless the argument given to chdir is “..”, and it is stored in the new directory’s parent attribute--therefore, any directory that is accessed by a process should already have a parent attribute stored. The only exception to this is “..”, but note that every process begins its life in the root directory, whose parent is itself. To reach any given other directory in the system, the OS must traverse down the entire path to that directory, and thus identify each subsequent directory’s parents--this guarantees that any given working directory’s parent will itself also have a parent, as the working directory’s parent was itself traversed with chdir at some point to arrive at the working directory in the first place.

## Additional Questions:
For this project, there are 2 optional buffer cache features that you can implement: write-behind and read-ahead. A buffer cache with write-behind will periodically flush dirty blocks to the file system block device, so that if a power outage occurs, the system will not lose as much data. Without write-behind, a write-back cache only needs to write data to disk when (1) the data is dirty and gets evicted from the cache, or (2) the system shuts down. A cache with read-ahead will predict which block the system will need next and fetch it in the background. A read-ahead cache can greatly improve the performance of sequential file reads and other easily-predictable file access patterns. Please discuss a possible implementation strategy for write-behind and a strategy for read-ahead. You must answer this question regardless of whether you actually decide to implement these features.

For the write-behind functionality, we could introduce a timer interrupt that will write the dirty blocks to the disk every flush_time period. Every cache block would also have an associated lock to prevent writing from the user programs while writing behind. The timer interrupt would first acquire the lock before the writing and then iterate through all the blocks and write the dirty ones to the disk. 

For the read-ahead functionality, we could use a list read_ahead in the kernel which contains pointers to the blocks that will be read soon. While the current block is being read, we pop the next to read block from the list and start loading the next block into the cache buffer.
