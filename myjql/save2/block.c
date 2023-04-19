#include "block.h"

#include <stdio.h>

#include"str.h"

void init_block(Block *block) {
    block->n_items = 0;
    block->head_ptr = (short)(block->data - (char*)block);
    block->tail_ptr = (short)sizeof(Block);
}

ItemPtr get_item(Block *block, short idx) {
    if (idx < 0 || idx >= block->n_items) {
        printf("get item error: idx is out of range\n");
        return NULL;
    }
    ItemID item_id = get_item_id(block, idx);
    if (get_item_id_availability(item_id)) {
        printf("get item error: item_id is not used\n");
        return NULL;
    }
    short offset = get_item_id_offset(item_id);
    return (char*)block + offset;
}

short new_item(Block *block, ItemPtr item, short item_size) {
    short avai_space = block->tail_ptr - block->head_ptr;
    for(int i = 0; i != block->n_items ; i++){
        ItemID item_id = get_item_id(block,i);

        if(get_item_id_availability(item_id) == 1){
            if( avai_space >= item_size){
                block->tail_ptr = block->tail_ptr - item_size;
                item_id = compose_item_id(0, block->tail_ptr ,item_size);
                get_item_id(block,i) = item_id;
                for(int j = 0; j < item_size ; j++){
                block->data[ block->tail_ptr + j - 3 * sizeof(short)] = item[j];
                }
            }
    /*     print_block(block);*/
        return i;
        }
    }
    if( avai_space < sizeof(ItemID) + item_size){
       /*  printf("Error:This block spare no enough space!\n"); */
        return -1;
    }
    else {
        block->head_ptr = block->head_ptr + sizeof(ItemID);
        block->tail_ptr = block->tail_ptr - item_size;
        get_item_id(block,block->n_items) = compose_item_id(0,block->tail_ptr,item_size);
        for(int j = 0; j < item_size ; j++){
            block->data[block->tail_ptr + j - 3 * sizeof(short)] = item[j];
        }
        block->n_items++;

        
        //print_block(block);

        return block->n_items - 1;
    }
 /*   ItemPtr it = get_item(block, 0);
    StringChunk chunk;
    for (int i = 0; i < item_size; i++) {
        get_str_chunk_data_ptr(&chunk)[i] = it[i];
    }*/

    return -1;
}

void delete_item(Block *block, short idx) {
    if (idx < 0 || idx >= block->n_items) {
        /* printf("get item error: idx is out of range\n"); */
        return ;
    }
    ItemID item_id = get_item_id(block,idx);
    short pos = get_item_id_offset(item_id);
    short size = get_item_id_size(item_id);

    short end = pos + size;

    for (int i = end - 1 - size; i >= block->tail_ptr; i--) {
        block->data[i + size - 3 * sizeof(short)] = block->data[i - 3 * sizeof(short)];
    }

    block->tail_ptr = block->tail_ptr + size;

    for (int i = 0; i < block->n_items; i++) {
        item_id = get_item_id(block, i);
        short temp_pos = get_item_id_offset(item_id);
        short temp_size = get_item_id_size(item_id);
        short avail = get_item_id_availability(item_id);
        if (temp_pos < pos) {
            get_item_id(block, i) = compose_item_id(avail, temp_pos + size, temp_size);
        }
    }

    if( idx == block->n_items - 1 ){
        block->head_ptr = block->head_ptr - sizeof(ItemID);
        block->n_items--;
    }

    else{
        get_item_id(block,idx) = compose_item_id(1,0,0);
    }
    //print_block(block);
    return;
}

short get_block_size(Block* block){
    short block_size;
        for(int i = 0; i < block->n_items ; i++){
            ItemID id = get_item_id(block,i);
            if(get_item_id_availability(id) != 0){
                return block->tail_ptr - block->head_ptr;
            }
        }
        block_size = (short)(block->tail_ptr - block->head_ptr - sizeof(ItemID));
    return block_size;
}



 void str_printer(ItemPtr item, short item_size) {
    if (item == NULL) {
        printf("NULL");
        return;
    }
    short i;
    printf("\"");
    for (i = 0; i < item_size; ++i) {
        printf("%c", item[i]);
    }
    printf("\"");
}

void print_block(Block *block) {
    short i, availability, offset, size;
    ItemID item_id;
    ItemPtr item;
    printf("----------BLOCK----------\n");
    printf("total = %d\n", block->n_items);
    printf("head = %d\n", block->head_ptr);
    printf("tail = %d\n", block->tail_ptr);
    for (i = 0; i < block->n_items; ++i) {
        item_id = get_item_id(block, i);
        availability = get_item_id_availability(item_id);
        offset = get_item_id_offset(item_id);
        size = get_item_id_size(item_id);
        if (!availability) {
            item = get_item(block, i);
        } else {
            item = NULL;
        }
        printf("%10d%5d%10d%10d\t", i, availability, offset, size);
        str_printer(item, size);
        printf("\n");
    }
    printf("-------------------------\n");
}

//void analyze_block(Block *block, block_stat_t *stat) {
//    short i;
//    stat->empty_item_ids = 0;
//    stat->total_item_ids = block->n_items;
//    for (i = 0; i < block->n_items; ++i) {
//        if (get_item_id_availability(get_item_id(block, i))) {
//            ++stat->empty_item_ids;
//        }
//    }
//    stat->available_space = block->tail_ptr - block->head_ptr 
//        + stat->empty_item_ids * sizeof(ItemID);
//}
//
//void accumulate_stat_info(block_stat_t *stat, const block_stat_t *stat2) {
//    stat->empty_item_ids += stat2->empty_item_ids;
//    stat->total_item_ids += stat2->total_item_ids;
//    stat->available_space += stat2->available_space;
//}
//
//void print_stat_info(const block_stat_t *stat) {
//    printf("==========STAT==========\n");
//    printf("empty_item_ids: " FORMAT_SIZE_T "\n", stat->empty_item_ids);
//    printf("total_item_ids: " FORMAT_SIZE_T "\n", stat->total_item_ids);
//    printf("available_space: " FORMAT_SIZE_T "\n", stat->available_space);
//    printf("========================\n");
//} 