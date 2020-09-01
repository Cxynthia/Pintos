#include <stdbool.h>
#include <stddef.h>
#include "devices/block.h"

#define MAX_CAPACITY 64

void cache_init(void);
struct cache_elem *get_cache (struct block *device, block_sector_t sector, void *buffer, bool is_write);
void cache_read (struct block *device, block_sector_t sector, void *buffer);
void cache_write (struct block *device, block_sector_t sector, void *buffer);
