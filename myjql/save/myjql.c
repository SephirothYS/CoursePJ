#include "myjql.h"

#include "buffer_pool.h"
#include "b_tree.h"
#include "table.h"
#include "str.h"
#include<string.h>

typedef struct {
    RID key;
    RID value;
} Record;

BufferPool bp_idx;
Table tbl_rec;
Table tbl_str;

void read_record(Table *table, RID rid, Record *record) {
    table_read(table, rid, (ItemPtr)record);
}

RID write_record(Table *table, const Record *record) {
    return table_insert(table, (ItemPtr)record, sizeof(Record));
}

void delete_record(Table *table, RID rid) {
    table_delete(table, rid);
}

int rid_row_row_cmp(RID a, RID b) {
    Record* rec_a,*rec_b;
    read_record(&tbl_rec,a,rec_a);
    read_record(&tbl_rec,a,rec_b);

    StringRecord* Str_a,*Str_b;
    read_string(&tbl_str,rec_a->key,Str_a);
    read_string(&tbl_str,rec_b->key,Str_b);


    return compare_string_record(&tbl_str,Str_a,Str_b);
}

int rid_ptr_row_cmp(void *p, size_t size, RID b) {
    char* a = (char*)p;
    Record* rec;
    read_record(&tbl_rec,b,rec);
    StringRecord* Str;
    read_string(&tbl_str,rec->key,Str);
    char* dest;
    load_string(&tbl_str,Str,dest,size);
    
    return strcmp(a,dest);
}


void myjql_init() {
    b_tree_init("rec.idx", &bp_idx);
    table_init(&tbl_rec, "rec.data", "rec.fsm");
    table_init(&tbl_str, "str.data", "str.fsm");
}

void myjql_close() {
    /* validate_buffer_pool(&bp_idx);
    validate_buffer_pool(&tbl_rec.data_pool);
    validate_buffer_pool(&tbl_rec.fsm_pool);
    validate_buffer_pool(&tbl_str.data_pool);
    validate_buffer_pool(&tbl_str.fsm_pool); */
    b_tree_close(&bp_idx);
    table_close(&tbl_rec);
    table_close(&tbl_str);
}

size_t myjql_get(const char *key, size_t key_len, char *value, size_t max_size) {

    RID rid;
    rid = b_tree_search(&bp_idx,(void*)key,sizeof(key),&rid_ptr_row_cmp);
    if(get_rid_block_addr(rid) == -1 && get_rid_idx(rid) == 0)
        return -1;

    Record* rec;
    read_record(&tbl_rec,rid,rec);

    StringRecord* Str_rec;
    read_string(&tbl_str,rec->value,Str_rec);
    return load_string(&tbl_str,Str_rec,value,max_size);
}

void myjql_set(const char *key, size_t key_len, const char *value, size_t value_len) {

    RID rid;
    rid = b_tree_search(&bp_idx,(void*)key,sizeof(key),&rid_ptr_row_cmp);
    if(get_rid_block_addr(rid) == -1 && get_rid_idx(rid) == 0){
    }
    else{
    myjql_del(key,key_len);
    }

    RID k,v;
    k = write_string(&tbl_str,key,key_len);
    v = write_string(&tbl_str,value,value_len);

    Record* rec;
    rec->key = k;
    rec->value = v;


    RID b_tree_rid;
    b_tree_rid =  write_record(&tbl_rec,rec);

    b_tree_insert(&bp_idx,b_tree_rid,&rid_row_row_cmp);

    return;

}

void myjql_del(const char *key, size_t key_len) {

    RID rid;
    rid = b_tree_search(&bp_idx,(void*)key,sizeof(key),&rid_ptr_row_cmp);
    if(get_rid_block_addr(rid) == -1 && get_rid_idx(rid) == 0)
        return;

    Record* rec;
    read_record(&tbl_rec,rid,rec);

    b_tree_delete(&bp_idx,rec->key,&rid_row_row_cmp);

    delete_string(&tbl_str,rec->key);
    delete_string(&tbl_str,rec->value);

    delete_record(&tbl_rec,rid);
}

/* void myjql_analyze() {
    printf("Record Table:\n");
    analyze_table(&tbl_rec);
    printf("String Table:\n");
    analyze_table(&tbl_str);
} */