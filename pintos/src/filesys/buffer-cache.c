#include <filesys/filesys.h>
#include <filesys/buffer-cache.h>

#include <threads/synch.h>
#include <stdbool.h>
#include <string.h>
#include <list.h>

struct cache_elem
{
    bool is_dirty;
    block_sector_t sector;
    // bool is_new;
//    bool is_valid;
//    void *buffer;
    char buffer[BLOCK_SECTOR_SIZE];
    struct block *fs_device;
    struct lock block_lock;
    struct list_elem elem;
};

static int cache_num;
static struct lock cache_lock;
//static int clock_head;
static struct list Usage_list;  // from least recent used to most recent used
static struct cache_elem cache_blocks[MAX_CAPACITY];

void cache_init(void)
{
//    list_init (&cache);
    lock_init (&cache_lock);
    list_init(&Usage_list);
//    clock_head = 0;            // point the clock head to the first element
    cache_num = 0;
    int i;
    for (i = 0; i < MAX_CAPACITY; i++) {
        lock_init(&(&cache_blocks[i])->block_lock);
    }
}

struct cache_elem *get_cache (struct block *device, block_sector_t sector, void *buffer, bool is_write)
{
    // loop through the current cache and see if the required block is already in the cache list
//    struct list_elem *e;
//    for (e = list_begin(&cache); e != list_end(&cache); e = list_next(e)) {
//        struct cache_elem *curr = list_entry(e, struct cache_elem, elem);
//        if (curr->sector == sector && curr->fs_device == device) {
//            curr->is_new = true;
//            return curr;
//        }
//    }
    lock_acquire(&cache_lock);
    int i;
    struct cache_elem *curr_block;
    for (i = 0; i < cache_num; i++) {
        curr_block = &cache_blocks[i];
        if (curr_block->sector == sector && curr_block->fs_device == device) {
            list_remove(&curr_block->elem);
            list_push_back(&Usage_list, &curr_block->elem);
            lock_acquire(&curr_block->block_lock);
            lock_release(&cache_lock);
//            curr_block->is_new = true;
            if (is_write) {
                curr_block->is_dirty = true;
                memcpy(curr_block->buffer, buffer, BLOCK_SECTOR_SIZE);
            } else {
                memcpy(buffer, curr_block->buffer, BLOCK_SECTOR_SIZE);
            }
            lock_release(&curr_block->block_lock);
            return curr_block;
        }
    }

    // if list size > 64, start a clock algorithm to evict one cache block
    if (cache_num >= 64) {
        struct list_elem *evict_elem = list_pop_front(&Usage_list);
        struct cache_elem *evict_block = list_entry(evict_elem, struct cache_elem, elem);
        list_push_back(&Usage_list, &evict_block->elem);
        lock_acquire(&evict_block->block_lock);
        lock_release(&cache_lock);
        if (evict_block->is_dirty) {
            block_write(evict_block->fs_device, evict_block->sector, evict_block->buffer);
        }
        evict_block->sector = sector;
        evict_block->fs_device = device;
        if (is_write) {
            evict_block->is_dirty = true;
            memcpy(evict_block->buffer, buffer, BLOCK_SECTOR_SIZE);
        } else {
            evict_block->is_dirty = false;
            block_read (evict_block->fs_device, sector, evict_block->buffer);
            memcpy(buffer, evict_block->buffer, BLOCK_SECTOR_SIZE);
        }
        lock_release(&evict_block->block_lock);
        return evict_block;
    }

    // if no matching block and not exceeding, read from the disk
    struct cache_elem *new_block = &cache_blocks[cache_num];
//    new_block->is_new = true;
    cache_num++;
    new_block->is_dirty = false;
    new_block->sector = sector;
    list_push_back(&Usage_list, &new_block->elem);
    lock_acquire(&new_block->block_lock);
    lock_release(&cache_lock);
    new_block->fs_device = device;
    if (is_write) {
        new_block->is_dirty = true;
        memcpy(new_block->buffer, buffer, BLOCK_SECTOR_SIZE);
    } else {
        memcpy(buffer, new_block->buffer, BLOCK_SECTOR_SIZE);
    }
    lock_release(&new_block->block_lock);
    return new_block;

    // if no matching block, read from the disk
//    block_read(device, sector, buffer);
//    struct cache_elem *new_elem = (struct cache_elem *)malloc(sizeof(struct cache_elem));
//    new_elem->is_new = true;
//    new_elem->sector = sector;
//    new_elem->fs_device = device;
//    new_elem->is_dirty = false;
//    new_elem->is_valid = true;
//    new_elem->buffer = buffer;
//    list_push_back(&cache, &(new_elem->elem));

    // if list size > 64, start a clock algorithm to evict one cache block
//    if (list_size(&cache) > 64) {
//        list_elem clock_elem = list_begin(&cache);
//        while (true) {
//            // advance one element before starting the loop
//            if (clock_head == list_tail(&cache)) {
//                clock_head = list_begin(&cache);
//            } else {
//                clock_head = list_next(clock_head);
//            }
//
//            struct cache_elem *curr_clock_head = list_entry(clock_head, struct cache_elem, elem);
//            if (!curr_clock_head->is_new) {
//                clock_cache_elem.is_valid = false;
//                struct list_elem *rm_elem = clock_head;
//                clock_head = list_prev(clock_head);
//                if (curr_clock_head->is_dirty == true) {
//                    block_write(curr_clock_head->fs_device, curr_clock_head->sector, curr_clock_head->buffer);
//                }
//                list_remove(rm_elem);
//                break;
//            } else {
//                curr_clock_head->is_new = false;
//            }
//        }
//    }
//
//    return new_elem;
}

void cache_read (struct block *device, block_sector_t sector, void *buffer)
{
//    lock_acquire(&cache_lock);
    get_cache(device, sector, buffer, false);
    return;
//    lock_release(&read_block->block_lock);
//    lock_release(&cache_lock);
}

void cache_write (struct block *device, block_sector_t sector, void *buffer)
{
//    lock_acquire(&cache_lock);
    get_cache(device, sector, buffer, true);
    return;
//    written_block->is_dirty = true;
//    lock_release(&written_block->block_lock);
//    lock_release(&cache_lock);
}
