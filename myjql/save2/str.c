#include "str.h"

#include "table.h"
#include"hash_map.h"

void read_string(Table *table, RID rid, StringRecord *record) {
    StringChunk chunk;

    table_read(table, rid, (ItemPtr)&chunk);
    record->chunk = chunk;
    record->idx = 0;
    rid = get_str_chunk_rid(&chunk);

    return;

}

int has_next_char(StringRecord *record) {
    StringChunk* chunk = &record->chunk;
    short idx = record->idx;
    short len = get_str_chunk_size(chunk);
    if (idx < len) {
        return 1;
    }
    RID rid = get_str_chunk_rid(chunk);
    off_t block_addr = get_rid_block_addr(rid);
    return block_addr >= 0;

}

char next_char(Table *table, StringRecord *record) {
    StringChunk chunk = record->chunk;
    short len = get_str_chunk_size(&chunk);
    if (record->idx >= len) {
        RID rid = get_str_chunk_rid(&chunk);

        off_t block_addr = get_rid_block_addr(rid);
        short index = get_rid_idx(rid);
        Block* block = (Block*)get_page(&table->data_pool, block_addr);

        ItemPtr item = get_item(block, index);
        ItemID item_id = get_item_id(block, index);
        short item_size = get_item_id_size(item_id);

        for (short i = 0; i < item_size; i++) {
            chunk.data[i] = item[i];
        }
        record->chunk = chunk;
        record->idx = 0;
        release(&table->data_pool, block_addr);
    }
    short index = record->idx++;
    return get_str_chunk_data_ptr(&chunk)[index];
}

int compare_string_record(Table *table, const StringRecord *a, const StringRecord *b) {
    char x,y;
    StringRecord a_temp,b_temp;
    a_temp = *a; b_temp = *b;
    x = next_char(table,&a_temp);
    y = next_char(table,&b_temp);
    if( x < y ){
        return -1;
    } 
    else if( x > y )
        return 1;
    else{
        int a_next,b_next;
        a_next = has_next_char(&a_temp);
        b_next = has_next_char(&b_temp);
        if( a_next != 0 && b_next == 0){
            return 1;
        }
        else if( a_next == 0 && b_next != 0){
            return -1;
        }
        else if( a_next == 0 && b_next == 0){
            return 0;
        }
        else{
            return compare_string_record(table,&a_temp,&b_temp);
        }
    }
}

RID write_string(Table *table, const char *data, off_t size) {
    int first = 0;
    RID rid;
    StringChunk chunk;
    get_rid_block_addr(rid) = -1;
    get_rid_idx(rid) = 0;

    if (size <= 0) {
        get_str_chunk_rid(&chunk) = rid;
        get_str_chunk_size(&chunk) = 0;
        rid = table_insert(table, (ItemPtr)&chunk, calc_str_chunk_size(0));
        return rid;
    }

    while(size != 0){
        short chunk_size;
        if(size % STR_CHUNK_MAX_LEN != 0){
            chunk_size = size % STR_CHUNK_MAX_LEN;
        }
        else{
            chunk_size = STR_CHUNK_MAX_LEN;
        }
        size = size - chunk_size;
        for(int i = 0; i < chunk_size ; i++){
            get_str_chunk_data_ptr(&chunk)[i] = data[size + i];
        }
        get_str_chunk_size(&chunk) = chunk_size;
        if(first != 1){
            RID null_rid ;
            get_rid_block_addr(null_rid) = -1;
            get_rid_idx(null_rid) = -1;
            get_str_chunk_rid(&chunk) = null_rid;
            first = 1;
        }
        else {
            off_t addr = get_rid_block_addr(rid);
            short idx = get_rid_idx(rid);
            get_str_chunk_rid(&chunk) = rid;
        }
        rid = table_insert(table, (ItemPtr)&chunk,calc_str_chunk_size(chunk_size));



        //print_hash_table(&table->fsm_pool);

    }
    return rid;
}

void delete_string(Table *table, RID rid) {
    off_t block_addr = get_rid_block_addr(rid);
    StringChunk chunk ;
    while( block_addr != -1){
        
        table_read(table,rid,(ItemPtr)&chunk);
        table_delete(table,rid);
        rid = get_str_chunk_rid(&chunk);
        block_addr = get_rid_block_addr(rid);
    }
}

/* void print_string(Table *table, const StringRecord *record) {
    StringRecord rec = *record;
    printf("\"");
    while (has_next_char(&rec)) {
        printf("%c", next_char(table, &rec));
    }
    printf("\"");
} */

size_t load_string(Table *table, const StringRecord *record, char *dest, size_t max_size) {
    StringRecord re = *record;
    size_t i = 0;
    while (has_next_char(&re) && i < max_size) {
        dest[i++] = next_char(table, &re);
    }
    dest[i] = '\0';
    return i;
}

/* void chunk_printer(ItemPtr item, short item_size) {
    if (item == NULL) {
        printf("NULL");
        return;
    }
    StringChunk *chunk = (StringChunk*)item;
    short size = get_str_chunk_size(chunk), i;
    printf("StringChunk(");
    print_rid(get_str_chunk_rid(chunk));
    printf(", %d, \"", size);
    for (i = 0; i < size; i++) {
        printf("%c", get_str_chunk_data_ptr(chunk)[i]);
    }
    printf("\")");
} */