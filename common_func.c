#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common_func.h"
#include "common_defs.h"
#include "sanity.h"

void* safe_free_name_val_node(name_val_node_t **ptr)
{
	void *tmp_ptr = NULL;

	if(!ptr || !*ptr){
       return tmp_ptr;
    }

    if((*ptr)->val_type == PTR) {
		tmp_ptr = (void*)(*ptr)->val.ptr_val;
		safe_free((void**)&tmp_ptr);
	}

    tmp_ptr = (*ptr)->next;

    safe_free((void**)ptr);

    return tmp_ptr;
}

void safe_free_name_val_list(name_val_node_t **ptr)
{
	void *tmp_ptr = NULL;

	if(!ptr || !*ptr){
		return;
	}

	tmp_ptr = *ptr;
	while(tmp_ptr) {
		tmp_ptr = safe_free_name_val_node(tmp_ptr);
	}
	*ptr = tmp_ptr;
}

void safe_free(void **ptr)
{
    if(!ptr || !*ptr){
        return;
    }
    free(*ptr);
    *ptr = NULL;
}

const char* enum_to_str(val_name_map_t *map, int val)
{
    while(map && map->name){
        if(map->val == val){
            return map->name;
        }
        map++;
    }

    return NULL;
}

int str_to_enum(val_name_map_t *map, const char* str, int *ret_val)
{
    int err = ERR_NOT_FOUND;

    sanity_null_ptr(map);
    sanity_null_ptr(ret_val);
    sanity_null_ptr(str);

    while(map->name){
        if(!strcmp(str, map->name)){
            *ret_val = map->val;
            err = ERR_NO_ERR;
            break;
        }
    }

exit_label:
    return err;
}

