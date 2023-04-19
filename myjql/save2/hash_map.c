#include "hash_map.h"

#include <stdio.h>


void hash_table_init(const char* filename, BufferPool* pool, off_t n_directory_blocks) {
    init_buffer_pool(filename, pool);
    /* TODO: add code here */
    if (pool->file.length == 0) {


        HashMapControlBlock* ctrl = (HashMapControlBlock*)get_page(pool, 0);
        ctrl->free_block_head = (PAGE_SIZE / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE;
        ctrl->n_directory_blocks = n_directory_blocks;
        ctrl->max_size = HASH_MAP_DIR_BLOCK_SIZE * n_directory_blocks;
        ctrl->free_block_last = (200001 + PAGE_SIZE / HASH_MAP_DIR_BLOCK_SIZE) * PAGE_SIZE;
        

        for (int i = 0; i < ctrl->n_directory_blocks; ++i) {
            HashMapDirectoryBlock* dir_block = (HashMapDirectoryBlock*)get_page(pool, (i  + 1) * PAGE_SIZE);
            for (int j = 0; j != HASH_MAP_DIR_BLOCK_SIZE; j++) {
                dir_block->directory[j] = 0;
            }
            //update(pool, (i  + 1) * PAGE_SIZE);
            release(pool, (i  + 1) * PAGE_SIZE);
        }

        for (int i = 0; i != 200000; i++) {
            HashMapBlock* block = (HashMapBlock*)get_page(pool, (i + PAGE_SIZE / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
            block->n_items = 0;
            block->next = (i + PAGE_SIZE / HASH_MAP_DIR_BLOCK_SIZE + 2) * PAGE_SIZE;
            for (int j = 0; j != HASH_MAP_BLOCK_SIZE; j++) {
                block->table[j] = 0;
            }
            //update(pool, (i + PAGE_SIZE / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
            release(pool, (i + PAGE_SIZE / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
        }
    }
    //update(pool, 0);
    release(pool, 0);
    return;
}

void hash_table_close(BufferPool* pool) {
    close_buffer_pool(pool);
}

void hash_table_insert(BufferPool* pool, short size, off_t addr) {

    HashMapControlBlock* ctrl = (HashMapControlBlock*)get_page(pool, 0);
    HashMapDirectoryBlock* dir_block;
    HashMapBlock* block;

    dir_block = (HashMapDirectoryBlock*)get_page(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
    if (dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE] == 0) {
        off_t pre_block_addr = ctrl->free_block_head;
        block = (HashMapBlock*)get_page(pool, pre_block_addr);
        //update(pool, pre_block_addr);
        dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE] = pre_block_addr;
        //update(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
        ctrl->free_block_head = block->next;
        //update(pool, 0);
        block->next = 0;
        block->table[0] = addr;
        block->n_items++;
        release(pool, pre_block_addr);
    }

    else {
        block = (HashMapBlock*)get_page(pool, dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE]);
        off_t pre_block_addr = dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE];
        while (block->next != 0) {

            if (block->n_items == HASH_MAP_BLOCK_SIZE) {
                release(pool, pre_block_addr);
                pre_block_addr = block->next;
                block = (HashMapBlock*)get_page(pool, block->next);
            }

        }
        if (block->next == 0) {
            if (block->n_items == HASH_MAP_BLOCK_SIZE) {
                block->next = ctrl->free_block_head;
                //update(pool,pre_block_addr);
                release(pool, pre_block_addr);
                block = (HashMapBlock*)get_page(pool, ctrl->free_block_head);
                pre_block_addr = ctrl->free_block_head;
                ctrl->free_block_head = block->next;
                //update(pool, 0);
                block->next = 0;
                block->table[0] = addr;
                block->n_items++;
                //update(pool, pre_block_addr);
                release(pool, pre_block_addr);
            }
            else {
                block->table[block->n_items] = addr;
                block->n_items++;
                //update(pool, pre_block_addr);
                release(pool, pre_block_addr);
            }
        }

    }
    release(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
    release(pool, 0);
    return;
}

off_t hash_table_pop_lower_bound(BufferPool* pool, short size) {

    HashMapControlBlock* ctrl = (HashMapControlBlock*)get_page(pool, 0);
    HashMapDirectoryBlock* dir_block;
    HashMapBlock* block;

    for (int i = size; i < ctrl->max_size; i++) {
        dir_block = (HashMapDirectoryBlock*)get_page(pool, (i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
        if (dir_block->directory[i % HASH_MAP_DIR_BLOCK_SIZE] != 0) {
            block = (HashMapBlock*)get_page(pool, dir_block->directory[ i % HASH_MAP_DIR_BLOCK_SIZE]);
            off_t pre_block_addr = dir_block->directory[ i % HASH_MAP_DIR_BLOCK_SIZE];
            while (block->next != 0) {

                if (block->n_items == HASH_MAP_BLOCK_SIZE) {
                    release(pool, pre_block_addr);
                    pre_block_addr = block->next;
                    block = (HashMapBlock*)get_page(pool, block->next);
                }

            }
            release(pool, pre_block_addr);
            off_t ans = block->table[block->n_items - 1];
            hash_table_pop(pool, i, block->table[block->n_items - 1]);
            return ans;
        }
        release(pool, (i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
    }
    release(pool, 0);
    return -1;
}

void hash_table_pop(BufferPool* pool, short size, off_t addr) {

    HashMapControlBlock* ctrl = (HashMapControlBlock*)get_page(pool, 0);
    HashMapDirectoryBlock* dir_block;
    HashMapBlock* block;

    dir_block = (HashMapDirectoryBlock*)get_page(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
    if (dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE] == 0) {
        printf("hash_pop failed.\n");
        return;
    }
    else {
        block = (HashMapBlock*)get_page(pool, dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE]);
        off_t pre_block_addr = dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE];
        off_t target_block_addr = 0;
        int target_item;
        off_t lp_block_addr = 0;
        while (block->next != 0) {

            for (int i = 0; i != block->n_items; i++) {
                if (block->table[i] == addr) {
                    target_item = i;
                    target_block_addr = pre_block_addr;
                }
            }
            release(pool, pre_block_addr);
            lp_block_addr = pre_block_addr;
            pre_block_addr = block->next;
            block = (HashMapBlock*)get_page(pool, block->next);
        }
        for (int i = 0; i != block->n_items; i++) {
            if (block->table[i] == addr) {
                target_item = i;
                target_block_addr = pre_block_addr;
            }
        }


        if (target_block_addr != 0) {
            HashMapBlock* last_block = (HashMapBlock*)get_page(pool, target_block_addr);
            last_block->table[target_item] = block->table[block->n_items - 1];
            block->table[block->n_items - 1] = 0;
            block->n_items--;
            if (block->n_items == 0) {
                if (dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE] == pre_block_addr) {
                    dir_block->directory[size % HASH_MAP_DIR_BLOCK_SIZE] = 0;
                    //update(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
                }
                if (lp_block_addr != 0) {
                    HashMapBlock* lp_block = (HashMapBlock*)get_page(pool, lp_block_addr);
                    lp_block->next = 0;
                    release(pool, lp_block_addr);
                }
                block->next = 0;
                block = (HashMapBlock*)get_page(pool, ctrl->free_block_last);
                block->next = target_block_addr;
                //update(pool, ctrl->free_block_last);
                release(pool, ctrl->free_block_last);
            }
            //update(pool, pre_block_addr);
            //update(pool, target_block_addr);
            release(pool, target_block_addr);

        }
        release(pool, pre_block_addr);
        release(pool, (size / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
        release(pool, 0);
    }
    return;
}

void print_hash_table(BufferPool* pool) {
    HashMapControlBlock* ctrl = (HashMapControlBlock*)get_page(pool, 0);
    HashMapDirectoryBlock* dir_block;
    off_t block_addr, next_addr;
    HashMapBlock* block;
    int i, j;
    printf("----------HASH TABLE----------\n");
    for (i = 0; i < ctrl->max_size; ++i) {
        dir_block = (HashMapDirectoryBlock*)get_page(pool, (i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
        if (dir_block->directory[i % HASH_MAP_DIR_BLOCK_SIZE] != 0) {
            printf("%d:", i);
            block_addr = dir_block->directory[i % HASH_MAP_DIR_BLOCK_SIZE];
            while (block_addr != 0) {
                block = (HashMapBlock*)get_page(pool, block_addr);
                printf("  [" FORMAT_OFF_T "]", block_addr);
                printf("{");
                for (j = 0; j < block->n_items; ++j) {
                    if (j != 0) {
                        printf(", ");
                    }
                    printf(FORMAT_OFF_T, block->table[j]);
                }
                printf("}");
                next_addr = block->next;
                release(pool, block_addr);
                block_addr = next_addr;
            }
            printf("\n");
        }
        release(pool, (i / HASH_MAP_DIR_BLOCK_SIZE + 1) * PAGE_SIZE);
    }
    release(pool, 0);
    printf("------------------------------\n");
}