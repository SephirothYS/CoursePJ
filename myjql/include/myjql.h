/* DO NOT MODIFY THIS FILE IN ANY WAY! */

#ifndef _MYJQL_H
#define _MYJQL_H

#ifdef __cplusplus
#include <cstddef>
extern "C" {
#else
#include <stddef.h>
#include"table.h"
#endif

int rid_row_row_cmp(RID a, RID b);
int rid_ptr_row_cmp(void* p, size_t size, RID b);

void myjql_init();

void myjql_close();

/* return -1 if the key does not exist */
size_t myjql_get(const char *key, size_t key_len, char *value, size_t max_size);

void myjql_set(const char *key, size_t key_len, const char *value, size_t value_len);

void myjql_del(const char *key, size_t key_len);

/* void myjql_analyze(); */

#ifdef __cplusplus
}
#endif

#endif  /* _MYJQL_H */