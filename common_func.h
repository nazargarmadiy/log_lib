#ifndef COMMON_FUNC_H
#define COMMON_FUNC_H

typedef struct {
    int         val;
    const char *name;
} val_name_map_t;

typedef struct name_val_node {
    struct name_val_node *next;
    const char           *name;
    union {
        void               *ptr_val;
        long long           ll_val;
        unsigned long long  ull_val;
        long double         ld_val;
    } val;
} name_val_node_t;

/*
 * free name_val_node_t
 * returns next field
 */
/*TODO: need to review, looks like no need this function*/
void* safe_free_name_val_node(name_val_node_t **ptr);

/*
 * free pointer and assign it NULL value
 */
void safe_free(void **ptr);

/*
 * convert enum to string representation
 * based on the val_name_map_t mapping
 * return const string on success or NULL on failure
 */
const char* enum_to_str(val_name_map_t *map, int val);

/*
 * convert string to enum value
 * based on the val_name_map_t mapping
 * return ERR_NO_ERR on success or ERR_NOT_FOUND on failure
 */
int str_to_enum(val_name_map_t *map, const char* str, int *ret_val);

#endif /* COMMON_FUNC_H */
