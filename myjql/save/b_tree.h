#ifndef _B_TREE_H
#define _B_TREE_H

#include "file_io.h"
#include "table.h"

/* it MUST be satisfied that: DEGREE >= 2 */
#define DEGREE (((PAGE_SIZE) - sizeof(size_t) - sizeof(off_t) - sizeof(char) + sizeof(RID)) / 2 / (sizeof(off_t) + sizeof(RID)))
#define LIMIT_M_2 ((2 * DEGREE - 1) % 2 ? ( 2 * DEGREE )/2 : (2 * DEGREE - 1) / 2)


typedef struct {
  /* B-Tree Node */
  /* you can modify anything in this struct */
  size_t n;
  off_t next;
  off_t child[2 * DEGREE];
  RID row_ptr[2 * DEGREE];
  char leaf;
} BNode;

typedef struct {
  /* B-Tree Control Block */
  /* you can modify anything in this struct */
  off_t root_node;
  off_t free_node_head;
} BCtrlBlock;

typedef RID KeyType; 

typedef struct {
    off_t addr;
    struct Node* next;
}Node;

typedef struct {
    Node* front;
    Node* rear;
    int size;
}queue;

/* BEGIN: --------------------------------- DO NOT MODIFY! --------------------------------- */

typedef int (*b_tree_row_row_cmp_t)(RID, RID);
typedef int (*b_tree_ptr_row_cmp_t)(void *, size_t, RID);

void b_tree_init(const char *filename, BufferPool *pool);

void b_tree_close(BufferPool *pool);

RID b_tree_search(BufferPool *pool, void *key, size_t size, b_tree_ptr_row_cmp_t cmp);

RID b_tree_insert(BufferPool *pool, RID rid, b_tree_row_row_cmp_t cmp);

void b_tree_delete(BufferPool *pool, RID rid, b_tree_row_row_cmp_t cmp);


/* END:   --------------------------------- DO NOT MODIFY! --------------------------------- */

static off_t RecursiveInsert(BufferPool *pool, off_t T, KeyType Key, int i, off_t Parent, b_tree_row_row_cmp_t cmp);

static off_t InsertElement(BufferPool *pool, int isKey, off_t Parent, off_t X, KeyType Key, int i, int j) ;

extern off_t Insert(BufferPool *pool, off_t T, KeyType Key, b_tree_row_row_cmp_t cmp);

static off_t MallocNewNode(BufferPool *pool);

static off_t SplitNode(BufferPool *pool, off_t Parent_addr, off_t X_addr, int i);

static off_t FindSibling(BufferPool *pool,off_t Parent_addr, size_t i);

static off_t RemoveElement(BufferPool *pool, int isKey, off_t Parent_addr, off_t X_addr, int i, int j);

static off_t MoveElement(BufferPool *pool, off_t Src_addr, off_t Dst_addr, off_t Parent_addr, int i, int n, b_tree_row_row_cmp_t cmp);

static off_t FindMostRight(BufferPool *pool, off_t P_addr);

static off_t FindMostLeft(BufferPool *pool, off_t P_addr);

extern off_t Remove(BufferPool *pool, off_t T_addr, KeyType Key,b_tree_row_row_cmp_t cmp);

static off_t RecursiveRemove(BufferPool *pool, off_t T_addr, KeyType Key, int i, off_t Parent_addr,b_tree_row_row_cmp_t cmp);

static off_t FindSiblingn_M_2(BufferPool *pool, off_t Parent_addr, int i, int* j);

static off_t MergeNode(BufferPool *pool, off_t Parent_addr, off_t X_addr, off_t S_addr, int i,b_tree_row_row_cmp_t cmp);

static RID RecursiveSearch(BufferPool *pool, off_t T_addr, void* Key, size_t size, int i, off_t Parent_addr, b_tree_ptr_row_cmp_t cmp);

void Print_b_tree(BufferPool* pool);

static off_t ReAdj(BufferPool* pool, off_t Parent_addr, off_t T_addr, KeyType Key, int i, b_tree_row_row_cmp_t cmp);

#endif  /* _B_TREE_H */