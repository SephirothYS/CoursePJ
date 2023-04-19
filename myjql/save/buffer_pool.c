#include "buffer_pool.h"
#include "file_io.h"

#include <stdio.h>
#include <stdlib.h>


void init_buffer_pool(const char* filename, BufferPool* pool) {
    FileInfo* File = (FileInfo*)malloc(sizeof(FileInfo));
    FileIOResult fresult;
    fresult = open_file(File, filename);
    if (fresult == FILE_IO_SUCCESS) {
        pool->file = *File;
        for (int i = 0; i != CACHE_PAGE; i++) {
            off_t* p1 = (off_t*)malloc(sizeof(off_t));
            size_t* p2 = (size_t*)malloc(sizeof(size_t));
            size_t* p3 = (size_t*)malloc(sizeof(size_t));
            Page* p4 = (Page*)malloc(sizeof(Page));
            size_t* p5 = (size_t*)malloc(sizeof(size_t));
            *p1 = -1;
            *p2 = 0;
            *p3 = 0;
            *p4 = EMPTY_PAGE;
            *p5 = 0;
            pool->addrs[i] = p1;
            pool->cnt[i] = p2;
            pool->ref[i] = p3;
            pool->pages[i] = p4;
            pool->upd[i] = p5;
        }
    }
    else if (fresult == FILE_IO_FAILED) {
        return;
    }
    return;
}

void close_buffer_pool(BufferPool* pool) {
    FileIOResult Fresult;
    for (int i = 0; i != CACHE_PAGE; i++) {
        Fresult = write_page(pool->pages[i], &(pool->file), *(pool->addrs[i]));
        if (Fresult == FILE_IO_FAILED) return;
        free(pool->pages[i]);
        free(pool->addrs[i]);
        free(pool->cnt[i]);
        free(pool->ref[i]);
        free(pool->upd[i]);
    }
    close_file(&(pool->file));
    return;
}

Page* get_page(BufferPool* pool, off_t addr) {
    FileIOResult Fresult;

    for (int i = 0; i != CACHE_PAGE - 1; i++) {
        if (*(pool->addrs[i]) == addr) {
            *(pool->ref[i]) = 1;
            return pool->pages[i];
        }
    }

    for (int i = 0; i != CACHE_PAGE - 1; i++) {
        if (*(pool->ref[i]) == 0) {

            if (*(pool->upd[i]) == 1) {
                write_page(pool->pages[i], &(pool->file), *(pool->addrs[i]));
                *(pool->upd[i]) = 0;
            }

            Fresult = read_page(pool->pages[i], &(pool->file), addr);
            if (Fresult == FILE_IO_FAILED) return NULL;
            if (Fresult == ADDR_OUT_OF_RANGE) {
                write_page(pool->pages[i], &(pool->file), addr);
            }
            *(pool->addrs[i]) = addr;
            *(pool->ref[i]) = 1;
            return (pool->pages[i]);
        }
    }
    return NULL;
}

void update(BufferPool* pool, off_t addr) {
    for (int i = 0; i != CACHE_PAGE; i++) {
        if (*(pool->addrs[i]) == addr) {
            *(pool->upd[i]) = 1;
        }
    }
    return;
}

void release(BufferPool* pool, off_t addr) {
    for (int i = 0; i != CACHE_PAGE; i++) {
        if (*(pool->addrs[i]) == addr) {
            *(pool->ref[i]) = 0;
            write_page(pool->pages[i], &(pool->file), *(pool->addrs[i]));
            *(pool->upd[i]) = 0;
        }
    }
    return;

}

/* void print_buffer_pool(BufferPool *pool) {
} */

/* void validate_buffer_pool(BufferPool *pool) {
} */
