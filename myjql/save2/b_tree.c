#include "b_tree.h"
#include "buffer_pool.h"


#include <stdio.h>


void b_tree_init(const char *filename, BufferPool *pool) {
    init_buffer_pool(filename, pool);
    /* TODO: add code here */
    if(pool->file.length == 0){
        
        BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool,0);
        ctrl->root_node = -1;
        ctrl->free_node_head = 256;

        BNode* root = (BNode*)get_page(pool,128);
        root->leaf = 'T';
        root->n = 0;
        root->next = -1;
        RID null_rid ;
        get_rid_block_addr(null_rid) = -1;
        get_rid_idx(null_rid) = 0;
        for(int i = 0; i < 2 * DEGREE ; i++){
            root->child[i] = -1;
            root->row_ptr[i] = null_rid;
        } 
        release(pool,128);
        ctrl->root_node = 128;
        release(pool,0);

    }
    return;
}

void b_tree_close(BufferPool *pool) {
    close_buffer_pool(pool);
}

RID b_tree_search(BufferPool *pool, void *key, size_t size, b_tree_ptr_row_cmp_t cmp) {

    BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool,0);
    RID* rid_ptr = (RID*) key;
    RID rid = *rid_ptr;
    RID ans = RecursiveSearch(pool,ctrl->root_node,key, size ,0,-1,cmp);

    release(pool,0);

    return ans;
}

RID b_tree_insert(BufferPool *pool, RID rid, b_tree_row_row_cmp_t cmp) {

    BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool,0);
    off_t root = ctrl->root_node;
    BNode* Node = (BNode*)get_page(pool,root);
    off_t last_root = root;

    root = Insert(pool, root, rid ,cmp);


    ctrl->root_node = root;
    //Print_b_tree(pool);



    release(pool,last_root);
    release(pool, 0);
    return rid;

}

void b_tree_delete(BufferPool *pool, RID rid, b_tree_row_row_cmp_t cmp) {

    BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool, 0);
    off_t root = ctrl->root_node;
    BNode* Node = (BNode*)get_page(pool, root);
    off_t last_root = root;

    root = Remove(pool, root, rid, cmp);

    ctrl->root_node = root;

    release(pool, last_root);
    release(pool, 0);
    return;
}



/* ---------------------------------------------------------------------------------------------------------------------------*/

static off_t RecursiveInsert(BufferPool *pool,off_t T_addr, KeyType Key, int i, off_t Parent_addr, b_tree_row_row_cmp_t cmp) {
    size_t j;
    size_t Limit;
    off_t Sibling_addr;

    BNode* T = (BNode*)get_page(pool,T_addr);

    /* 查找分支 */
    j = 0;
    while (j < T->n && ( cmp(Key,T->row_ptr[j]) == 1 || cmp(Key,T->row_ptr[j]) == 0) ) {
        /* 重复值不插入 */
        //if (cmp(Key, T->row_ptr[j]) == 0) {
        //    release(pool, T_addr);
        //    return T_addr;
        //}
        j++;
    }
    if (j != 0 && T->child[0] != -1) j--;

    /* 树叶 */
    if (T->child[0] == -1) {
        if (cmp(Key, T->row_ptr[j]) == 0) {
            release(pool, T_addr);
            return T_addr;
        }
        release(pool, T_addr);
        T_addr = InsertElement(pool, 1, Parent_addr, T_addr, Key, i, j);
    }
    /* 内部节点 */
    else
        T->child[j] = RecursiveInsert(pool,T->child[j], Key, j, T_addr, cmp);

    /* 调整节点 */

    Limit = 2 * DEGREE - 1;

    if (T->n > Limit) {
        /* 根 */
        if (Parent_addr == -1) {
            /* 分裂节点 */
            release(pool, T_addr);
            T_addr = SplitNode(pool, Parent_addr, T_addr, i);
        }
        else {
            Sibling_addr = FindSibling(pool, Parent_addr, (size_t)i);
            if (Sibling_addr != -1) {
                /* 将T的一个元素（Key或者Child）移动的Sibing中 */
                MoveElement(pool, T_addr, Sibling_addr, Parent_addr, i, 1,cmp,Key);
            }
            else {
                /* 分裂节点 */
                release(pool, T_addr);
                T_addr = SplitNode(pool, Parent_addr, T_addr, i);
            }
        }

    }

    if (Parent_addr != -1){
        BNode* Parent = (BNode*)get_page(pool,Parent_addr);
        Parent->row_ptr[i] = T->row_ptr[0];
        release(pool,Parent_addr);
    }

    release(pool,T_addr);
    return T_addr;
};

static off_t InsertElement(BufferPool *pool, int isKey, off_t Parent_addr, off_t X_addr, KeyType Key, int i, int j) {

    int k;

    BNode* X = (BNode*)get_page(pool,X_addr);
    BNode* Parent = (BNode*)get_page(pool,Parent_addr);
    if (isKey) {
        /* 插入key */
        k = X->n - 1;
        while (k >= j) {
            X->row_ptr[k + 1] = X->row_ptr[k];k--;
        }

        X->row_ptr[j] = Key;

        if (Parent_addr != -1){
            Parent->row_ptr[i] = X->row_ptr[0];
        }
        X->n++;

    }
    else {
        /* 插入节点 */

        /* 对树叶节点进行连接 */
        if (X->child[0] == -1) {
            if (i > 0){
                off_t child_addr = Parent->child[i - 1];
                BNode* child = (BNode*)get_page(pool,child_addr);
                child->next = X_addr;
                release(pool,child_addr);
        }
            X->next = Parent->child[i];
        }

        k = Parent->n - 1;
        while (k >= i) {
            Parent->child[k + 1] = Parent->child[k];
            Parent->row_ptr[k + 1] = Parent->row_ptr[k];
            k--;
        }
        Parent->row_ptr[i] = X->row_ptr[0];
        Parent->child[i] = X_addr;

        Parent->n++;

    }
    release(pool,X_addr);
    release(pool,Parent_addr);
    return X_addr;
}

extern off_t Insert(BufferPool *pool, off_t T, KeyType Key,b_tree_row_row_cmp_t cmp) {
    return RecursiveInsert(pool, T, Key, 0, -1, cmp);
}

static off_t SplitNode(BufferPool *pool, off_t Parent_addr, off_t X_addr, int i) {
    int j, k, Limit;
    off_t NewNode_addr;

    NewNode_addr = MallocNewNode(pool);

    BNode* X = (BNode*)get_page(pool,X_addr);
    BNode* NewNode = (BNode*)get_page(pool,NewNode_addr);
    RID null_rid ;
    get_rid_block_addr(null_rid) = -1;
    get_rid_idx(null_rid) = 0;

    k = 0;
    j = X->n / 2;
    Limit = X->n;
    while (j < Limit) {
        if (X->child[0] != -1) {
            NewNode->child[k] = X->child[j];
            X->child[j] = -1;
        }
        NewNode->row_ptr[k] = X->row_ptr[j];
        X->row_ptr[j] = null_rid;
        NewNode->n++;X->n--;
        j++;k++;
    }

    if (Parent_addr != -1)
        InsertElement(pool, 0, Parent_addr, NewNode_addr, null_rid, i + 1, -1);
    else {
        /* 如果是X是根，那么创建新的根并返回 */
        Parent_addr = MallocNewNode(pool);
        InsertElement(pool, 0, Parent_addr, X_addr, null_rid, 0, -1);
        InsertElement(pool, 0, Parent_addr, NewNode_addr, null_rid, 1, -1);

        release(pool, X_addr);
        release(pool, NewNode_addr);
        return Parent_addr;
    }

    release(pool,X_addr);
    release(pool,NewNode_addr);
    return X_addr;
}

static off_t MallocNewNode(BufferPool *pool) {
    BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool, 0);
    off_t NewNode_addr = ctrl->free_node_head;
    ctrl->free_node_head = ctrl->free_node_head + 128;
    BNode* NewNode = (BNode*)get_page(pool,NewNode_addr);
    int i;

    RID null_rid ;
    get_rid_block_addr(null_rid) = -1;
    get_rid_idx(null_rid) = 0;

    i = 0;
    while (i < 2 * DEGREE) {
        NewNode->row_ptr[i] = null_rid;
        NewNode->child[i] = -1;
        i++;
    }
    NewNode->next = -1;
    NewNode->n = 0;

    release(pool,NewNode_addr);
    release(pool, 0);
    return NewNode_addr;
}

static off_t FindSibling(BufferPool *pool,off_t Parent_addr, size_t i) {
    off_t Sibling_addr;
    off_t child_addr;
    size_t Limit;

    Limit = 2 * DEGREE - 1;

    BNode* Parent = (BNode*)get_page(pool,Parent_addr);

    Sibling_addr = -1;
    if (i == 0) {
        child_addr = Parent->child[1];
        BNode* child = (BNode*)get_page(pool,child_addr);
        if (child->n < Limit)
            Sibling_addr = Parent->child[1];
        release(pool, child_addr);
    }
    else{
        child_addr = Parent->child[i - 1];
        BNode* child = (BNode*)get_page(pool,child_addr);
        if (child->n < Limit)
            Sibling_addr = Parent->child[i - 1];
        release(pool,child_addr);
        child_addr = Parent->child[i + 1];
        child = (BNode*)get_page(pool,child_addr);
        if (i + 1 < Parent->n && child->n < Limit) {
            Sibling_addr = Parent->child[i + 1];
        }
        release(pool,child_addr);
    }
    release(pool,Parent_addr);
    return Sibling_addr;
}

static off_t MoveElement(BufferPool *pool, off_t Src_addr, off_t Dst_addr, off_t Parent_addr, int i, int n, b_tree_row_row_cmp_t cmp, KeyType key) {
    KeyType TmpKey;
    off_t Child_addr;
    int j, SrcInFront;

    SrcInFront = 0;

    BNode* Src = (BNode*)get_page(pool,Src_addr);
    BNode* Dst = (BNode*)get_page(pool,Dst_addr);
    BNode* Parent = (BNode*)get_page(pool,Parent_addr);

    RID null_rid ;
    get_rid_block_addr(null_rid) = -1;
    get_rid_idx(null_rid) = 0;

    if (cmp(Src->row_ptr[0] , Dst->row_ptr[0]) == -1)
        SrcInFront = 1;

    j = 0;
    /* 节点Src在Dst前面 */
    if (SrcInFront) {
        if (Src->child[0] != -1) {
            while (j < n) {
                Child_addr = Src->child[Src->n - 1];
                RemoveElement(pool,0, Src_addr, Child_addr, Src->n - 1, -1,key,cmp);
                InsertElement(pool,0, Dst_addr, Child_addr, null_rid, 0, -1);
                j++;
            }
        }
        else {
            while (j < n) {
                TmpKey = Src->row_ptr[Src->n - 1];
                RemoveElement(pool,1, Parent_addr, Src_addr, i, Src->n - 1,key,cmp);
                InsertElement(pool,1, Parent_addr, Dst_addr, TmpKey, i + 1, 0);
                j++;
            }

        }

        Parent->row_ptr[i + 1] = Dst->row_ptr[0];
        /* 将树叶节点重新连接 */
        if (Src->n > 0){
            off_t right_addr, left_addr;
            right_addr = FindMostRight(pool,Src_addr);
            left_addr = FindMostLeft(pool,Dst_addr);
            BNode* right = (BNode*)get_page(pool,right_addr);
            right->next = left_addr;
            release(pool,right_addr);
        }

    }
    else {
        if (Src->child[0] != -1) {
            while (j < n) {
                Child_addr = Src->child[0];
                RemoveElement(pool,0, Src_addr, Child_addr, 0, -1,key,cmp);
                InsertElement(pool,0, Dst_addr, Child_addr, null_rid, Dst->n, -1);
                j++;
            }

        }
        else {
            while (j < n) {
                TmpKey = Src->row_ptr[0];
                RemoveElement(pool,1, Parent_addr, Src_addr, i, 0,key,cmp);
                InsertElement(pool,1, Parent_addr, Dst_addr, TmpKey, i - 1, Dst->n);
                j++;
            }

        }

        Parent->row_ptr[i] = Src->row_ptr[0];
        if (Src->n > 0){
            off_t right_addr, left_addr;
            right_addr = FindMostRight(pool,Dst_addr);
            left_addr  = FindMostLeft(pool,Src_addr);
            BNode* right = (BNode*)get_page(pool,right_addr);
            right->next = left_addr;
            release(pool,right_addr);
        }

    }
    release(pool,Src_addr);
    release(pool,Dst_addr);
    release(pool,Parent_addr);
    return Parent_addr;
}

static off_t RemoveElement(BufferPool *pool, int isKey, off_t Parent_addr, off_t X_addr, int i, int j,KeyType key, b_tree_row_row_cmp_t cmp) {

    int k, Limit;

    BNode* Parent = (BNode*)get_page(pool,Parent_addr);
    BNode* X = (BNode*)get_page(pool,X_addr);

    RID null_rid ;
    get_rid_block_addr(null_rid) = -1;
    get_rid_idx(null_rid) = 0;

    if (isKey) {
        Limit = X->n;
        /* 删除key */
        k = j + 1;
        while (k < Limit) {
            X->row_ptr[k - 1] = X->row_ptr[k];k++;
        }

        X->row_ptr[X->n - 1] = null_rid;

        RID temp = Parent->row_ptr[i];
        Parent->row_ptr[i] = X->row_ptr[0];
        if (get_rid_block_addr(X->row_ptr[0]) != -1) {
            BCtrlBlock* Ctrl = (BCtrlBlock*)get_page(pool, 0);
            off_t root = Ctrl->root_node;
            ReAdj(pool, -1, root, temp, 0, cmp, X->row_ptr[0]);
            release(pool, 0);
        }

        X->n--;
    }
    else {
        /* 删除节点 */

        /* 修改树叶节点的链接 */
        if (X->child[0] == -1 && i > 0) {
            off_t child_addr =  Parent->child[i - 1];
            BNode* child = (BNode*)get_page(pool,child_addr);
            child->next = Parent->child[i + 1];
            release(pool,child_addr);
        }
        Limit = Parent->n;
        k = i + 1;
        while (k < Limit) {
            Parent->child[k - 1] = Parent->child[k];
            Parent->row_ptr[k - 1] = Parent->row_ptr[k];
            k++;
        }

        Parent->child[Parent->n - 1] = -1;
        Parent->row_ptr[Parent->n - 1] = null_rid;

        Parent->n--;

    }
    release(pool,Parent_addr);
    release(pool,X_addr);
    return X_addr;
}

static off_t FindMostRight(BufferPool *pool, off_t P_addr) {
    off_t Tmp_addr;

    Tmp_addr = P_addr;

    BNode* Tmp = (BNode*)get_page(pool,Tmp_addr);

    while (Tmp != NULL && Tmp->child[Tmp->n - 1] != -1) {
        release(pool, Tmp_addr);
        Tmp_addr = Tmp->child[Tmp->n - 1];
        Tmp = (BNode*)get_page(pool, Tmp_addr);
    }
    release(pool,Tmp_addr);
    return Tmp_addr;
}

static off_t FindMostLeft(BufferPool *pool, off_t P_addr) {
    off_t Tmp_addr;

    Tmp_addr = P_addr;

    BNode* Tmp = (BNode*)get_page(pool,Tmp_addr);

    while (Tmp != NULL && Tmp->child[0] != -1) {
        release(pool, Tmp_addr);
        Tmp_addr = Tmp->child[Tmp->n - 1];
        Tmp = (BNode*)get_page(pool, Tmp_addr);
    }
    release(pool,Tmp_addr);
    return Tmp_addr;
}

extern off_t Remove(BufferPool* pool, off_t T_addr, KeyType Key, b_tree_row_row_cmp_t cmp) {
    return RecursiveRemove(pool, T_addr, Key, 0, -1, cmp);
};

static off_t RecursiveRemove(BufferPool* pool, off_t T_addr, KeyType Key, int i, off_t Parent_addr, b_tree_row_row_cmp_t cmp) {

    size_t j;
    int NeedAdjust;
    off_t Sibling, Tmp;

    BNode* T = (BNode*)get_page(pool, T_addr);

    Sibling = -1;

    /* 查找分支 */
    j = 0;
    while (j < T->n && (cmp(Key, T->row_ptr[j]) == 1 || cmp(Key, T->row_ptr[j]) == 0)) {
        if (cmp(Key, T->row_ptr[j]) == 0)
            break;
        j++;
    }

    if (T->child[0] == -1) {
        /* 没找到 */
        if (cmp(Key, T->row_ptr[j]) != 0 || j == T->n) {
            release(pool, T_addr);
            return T_addr;
        }
    }
    else
        if ( (j == T->n || cmp(Key, T->row_ptr[j]) < 0) && j > 0)  j--;



    /* 树叶 */
    if (T->child[0] == -1) {
        T_addr = RemoveElement(pool, 1, Parent_addr, T_addr, i, j,Key,cmp);
    }
    else {
        T->child[j] = RecursiveRemove(pool, T->child[j], Key, j, T_addr, cmp);


    }

    NeedAdjust = 0;
    /* 树的根或者是一片树叶，或者其儿子数在2到M之间 */
    if (Parent_addr == -1 && T->child[0] != -1 && T->n < 2)
        NeedAdjust = 1;
    /* 除根外，所有非树叶节点的儿子数在[M/2]到M之间。(符号[]表示向上取整) */
    else if (Parent_addr != -1 && T->child[0] != -1 && T->n < LIMIT_M_2)
        NeedAdjust = 1;
    /* （非根）树叶中关键字的个数也在[M/2]和M之间 */
    else if (Parent_addr != -1 && T->child[0] == -1 && T->n < LIMIT_M_2)
        NeedAdjust = 1;

    /* 调整节点 */
    if (NeedAdjust) {
        /* 根 */
        if (Parent_addr == -1) {
            if (T->child[0] != -1 && T->n < 2) {
                Tmp = T_addr;
                release(pool, T_addr);
                T_addr = T->child[0];
                /* free(Tmp); */
                return T_addr;
            }

        }
        else {
            /* 查找兄弟节点，其关键字数目大于M/2 */
            Sibling = FindSiblingn_M_2(pool, Parent_addr, i, &j);
            if (Sibling != -1) {
                MoveElement(pool, Sibling, T_addr, Parent_addr, j, 1, cmp,Key);
            }
            else {
                BNode* Parent = (BNode*)get_page(pool, Parent_addr);
                if (i == 0)
                    Sibling = Parent->child[1];
                else
                    Sibling = Parent->child[i - 1];
                release(pool, Parent_addr);
                Parent_addr = MergeNode(pool, Parent_addr, T_addr, Sibling, i, cmp,Key);

                Parent = (BNode*)get_page(pool, Parent_addr);
                release(pool, T_addr);
                T_addr = Parent->child[i];
                release(pool, Parent_addr);
            }
        }

    }

    release(pool, T_addr);
    return T_addr;
};

 static off_t FindSiblingn_M_2(BufferPool *pool, off_t Parent_addr, int i, int* j) {
    size_t Limit;
    off_t Sibling;
    Sibling = -1;

    Limit = LIMIT_M_2;

    BNode* Parent = (BNode*)get_page(pool,Parent_addr);

    if (i == 0) {
        off_t child_addr = Parent->child[1];
        BNode* child = (BNode*)get_page(pool,child_addr);
        if (child->n > Limit) {
            Sibling = Parent->child[1];
            *j = 1;
        }
        release(pool,child_addr);
    }
    else {
        off_t child_addr = Parent->child[i - 1];
        BNode* child = (BNode*)get_page(pool,child_addr);
        if (child->n > Limit) {
            Sibling = Parent->child[i - 1];
            *j = i - 1;
        }
        release(pool,child_addr);

        child_addr = Parent->child[ i + 1];
        child = (BNode*)get_page(pool,child_addr);
        if (i + 1 < Parent->n && child->n > Limit) {
            Sibling = Parent->child[i + 1];
            *j = i + 1;
        }
        release(pool,child_addr);
    }


    release(pool, Parent_addr);
    return Sibling;
}

static off_t MergeNode(BufferPool *pool, off_t Parent_addr, off_t X_addr, off_t S_addr, int i,b_tree_row_row_cmp_t cmp,KeyType key) {
    size_t Limit;

    BNode* Parent = (BNode*)get_page(pool,Parent_addr);
    BNode* X = (BNode*)get_page(pool,X_addr);
    BNode* S = (BNode*)get_page(pool,S_addr);

    /* S的关键字数目大于M/2 */
    if (S->n > LIMIT_M_2) {
        /* 从S中移动一个元素到X中 */
        MoveElement(pool,S_addr, X_addr, Parent_addr, i, 1,cmp,key);
    }
    else {
        /* 将X全部元素移动到S中，并把X删除 */
        Limit = X->n;
        MoveElement(pool, X_addr, S_addr, Parent_addr, i, Limit, cmp,key);
        RemoveElement(pool,0, Parent_addr, X_addr, i, -1,key,cmp);

    }


    release(pool, Parent_addr);
    release(pool, X_addr);
    release(pool, S_addr);
    return Parent_addr;
}


static RID RecursiveSearch(BufferPool *pool, off_t T_addr, void* Key,size_t size, int i, off_t Parent_addr, b_tree_ptr_row_cmp_t cmp) {

    size_t j; 
    int NeedAdjust;
    off_t Sibling, Tmp;

    BNode* T = (BNode*)get_page(pool,T_addr);

    Sibling = -1;

    RID null_rid ;
    get_rid_block_addr(null_rid) = -1;
    get_rid_idx(null_rid) = 0;

    RID ans = null_rid;
    /* 查找分支 */
    j = 0;
    while (j < T->n && ( cmp(Key ,size, T->row_ptr[j] ) == 1 || cmp(Key ,size, T->row_ptr[j] ) == 0 )) {
        if (cmp(Key, size, T->row_ptr[j]) == 0) {
            ans = T->row_ptr[j];
            break;
        }
        j++;
    }

    if (T->child[0] == -1) {
        /* 没找到 */
        if (cmp(Key, size, T->row_ptr[j]) != 0 || j == T->n) {
            release(pool, T_addr);
            return null_rid;
        }
    }
    else
        if ( (j == T->n || cmp(Key ,size, T->row_ptr[j] ) < 0 ) && j > 0) j--;



    /* 树叶 */
    if (T->child[0] == -1) {
        ans = T->row_ptr[j];
    }
    else {
        ans = RecursiveSearch(pool,T->child[j], Key, size, j, T_addr, cmp);       
    }

    release(pool, T_addr);
    return ans;
}





off_t get_q_front(queue *q) {
    Node* p = (*q).front;
    if ((*q).front == NULL) return -1;
    off_t ans = p->addr;
    (*q).front = (*q).front->next;
    free(p);
    (*q).size--;
    return ans;
}

void q_push_back(queue *q,off_t addr) {
    if ((*q).front == NULL) {
        Node* n = (Node*)malloc(sizeof(Node));
        (*n).addr = addr;
        (*n).next = NULL;
        (*q).front = n;
        (*q).rear = n;
        (*q).size++;
    }
    else {
        Node* temp,*n;
        n = (Node*)malloc(sizeof(Node));
        temp = (*q).rear;
        temp->next = n;
        n->addr = addr;
        n->next = NULL;
        (*q).rear = n;
        (*q).size++;
    }
    return;
}


void Print_b_tree(BufferPool* pool) {
    BCtrlBlock* ctrl = (BCtrlBlock*)get_page(pool, 0);
    off_t root_addr = ctrl->root_node;

    queue q;
    q.front = q.rear = NULL;
    q.size = 0;

    q_push_back(&q, root_addr);
    while (q.size != 0) {
        int i = 0;
        off_t addr = get_q_front(&q);
        BNode* p = (BNode*)get_page(pool, addr);
        printf("(%d)", addr);
        while (i < p->n) {
            print_rid(p->row_ptr[i]);
            printf("  ");
            if (p->child[i] != -1)
                q_push_back(&q, p->child[i]);
            i++;
        }
        printf(" | ");

        if (p->child[i] != -1) {
            q_push_back(&q, p->child[i]);
        }
        release(pool, addr);
    }
    printf("\n");
};

static off_t ReAdj(BufferPool* pool,off_t Parent_addr, off_t T_addr, KeyType Key, int i, b_tree_row_row_cmp_t cmp,RID target) {
    BNode* T = (BNode*)get_page(pool, T_addr);
    size_t j;
    size_t j_rec = 0;
    j = 0;
    while (j < T->n && (cmp(Key, T->row_ptr[j]) == 1 || cmp(Key, T->row_ptr[j]) == 0)) {
        if (cmp(Key, T->row_ptr[j]) == 0) {
            j_rec = j;
            break;
        }
        j++;
    }

    if (T->child[0] == -1) {
        /* 没找到 */
        if (cmp(Key, T->row_ptr[j]) != 0 || j == T->n) {
            release(pool, T_addr);
            return T_addr;
        }
    }
    else
        if ( (j == T->n || cmp(Key, T->row_ptr[j]) < 0 ) && j > 0) j--;


    if (T->child[0] == -1) {
        release(pool, T_addr);
        return T_addr;
    }
    else {
        if (cmp(Key, T->row_ptr[j_rec]) == 0) T->row_ptr[j_rec] = target;
        T->child[j] = ReAdj(pool, T_addr,T->child[j], Key, j, cmp,target);
    }
    release(pool, T_addr);
    return T_addr;
}