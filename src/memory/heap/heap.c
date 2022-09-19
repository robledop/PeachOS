/*
    MALLOC

    Assume we set the heap data pool to point to address 0x01000000
    Assume the heap is 100MB in size
    Assume we have 25600 entries in our entry table that describe our 100MB of data in the data pool
    100MB / 2096 = 25600

    MEMORY ALLOCATION PROCESS

    Take the size from malloc and calculate how many blocks we need to allocate for this size.
    If the user asks for "5000" bytes, we will need to allocate 8192 bytes because our implementation
    works on 4096 byte blocks. 8192 bytes are two blocks.

    Check the entry table for the first entry we can find that has a type of HEAP_BLOCK_TABLE_ENTRY_FREE,
    meaning that the 4096 block that this entry represents is free for use.

    Since we require two blocks, we also need to ensure the next entry is also free for use otherwise we will
    need to discard the first block we found and look further in our table until we find at least two free blocks
    that are next to each other.

    Once we have two blocks, we mark those blocks as taken.

    We now return the absolute address that the starting block represents. Calculation
    (heap_data_pool_start_address + (block_number * block_size))
*/

#include "heap.h"
#include "kernel.h"
#include "status.h"
#include "memory/memory.h"
#include <stdbool.h>

static int heap_validate_table(void *ptr, void *end, struct heap_table *table)
{
    int result = 0;

    size_t table_size = (size_t)(end - ptr);
    size_t total_blocks = table_size / PEACHOS_HEAP_BLOCK_SIZE;
    if (table->total != total_blocks)
    {
        result = -EINVARG;
        goto out;
    }

out:
    return result;
}

// Checks if the address is correctly aligned according to the block size
static bool heap_validate_alignment(void *address)
{
    return ((unsigned int)address % PEACHOS_HEAP_BLOCK_SIZE) == 0;
}

int heap_create(struct heap *heap, void *heap_address, void *end, struct heap_table *table)
{
    int result = 0;

    if (!heap_validate_alignment(heap_address) || !heap_validate_alignment(end))
    {
        result = -EINVARG;
        goto out;
    }

    memset(heap, 0, sizeof(struct heap));
    heap->saddr = heap_address;
    heap->table = table;

    result = heap_validate_table(heap_address, end, table);
    if (result < 0)
    {
        goto out;
    }

    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
    memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);

out:
    return result;
}

/* Gets the number of bytes that will be allocated, rounding it up so that it aligns with the block size */
static uint32_t heap_align_value_to_upper(uint32_t val)
{
    if ((val % PEACHOS_HEAP_BLOCK_SIZE) == 0)
    {
        return val;
    }

    val = (val - (val % PEACHOS_HEAP_BLOCK_SIZE));
    val += PEACHOS_HEAP_BLOCK_SIZE;
    return val;
}

// Returns the lower 4 bits of the entry (the entry type)
static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
    return entry & 0x0f;
}

// Looks for 'total_blocks' contiguous free blocks in the heap and returns the start_block
int heap_get_start_block(struct heap *heap, uint32_t total_blocks)
{
    struct heap_table *table = heap->table;
    int current_block = 0;
    int start_block = -1;

    for (size_t i = 0; i < table->total; i++)
    {
        if (heap_get_entry_type(table->entries[i]) != HEAP_BLOCK_TABLE_ENTRY_FREE)
        {
            current_block = 0;
            start_block = -1;
            continue;
        }

        // If this is the first block
        if (start_block == -1)
        {
            start_block = i;
        }

        current_block++;
        if (current_block == total_blocks)
        {
            break;
        }
    }

    if (start_block == -1)
    {
        return -ENOMEM;
    }

    return start_block;
}

void *heap_block_to_address(struct heap *heap, int block)
{
    return heap->saddr + (block * PEACHOS_HEAP_BLOCK_SIZE);
}

void heap_mark_blocks_taken(struct heap *heap, int start_block, int total_blocks)
{
    int end_block = (start_block + total_blocks) - 1;

    // mar the first block
    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;
    
    // if we are allocating more than 1 block, also set the flag HEAP_BLOCK_HAS_NEXT
    if (total_blocks > 1) 
    {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    // mark the rest of the blocks
    for (int i = start_block; i <= end_block; i++)
    {
        heap->table->entries[i] = entry;
        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;
        if (i != end_block - 1)
        {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}

void *heap_malloc_blocks(struct heap *heap, uint32_t total_blocks)
{
    void *address = 0;

    int start_block = heap_get_start_block(heap, total_blocks);
    if (start_block < 0)
    {
        goto out;
    }

    address = heap_block_to_address(heap, start_block);

    heap_mark_blocks_taken(heap, start_block, total_blocks);

out:
    return address;
}

void heap_mark_blocks_free(struct heap *heap, int starting_block)
{
    struct heap_table *table = heap->table;
    for (int i = starting_block; i < (int)table->total; i++)
    {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;
        if (!(entry & HEAP_BLOCK_HAS_NEXT))
        {
            break;
        }
    }
}

int heap_address_to_block(struct heap *heap, void *address)
{
    return ((int)(address - heap->saddr)) / PEACHOS_HEAP_BLOCK_SIZE;
}

void *heap_malloc(struct heap *heap, size_t size)
{
    size_t aligned_size = heap_align_value_to_upper(size);
    uint32_t total_blocks = aligned_size / PEACHOS_HEAP_BLOCK_SIZE;
    return heap_malloc_blocks(heap, total_blocks);
}

void heap_free(struct heap *heap, void *ptr)
{
    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}