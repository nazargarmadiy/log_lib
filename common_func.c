#include "common_func.h"
#include <stdio.h>
#include <stdlib.h>

void safe_free(void **ptr)
{
    if(!ptr || !*ptr){
        return;
    }
    free(*ptr);
    *ptr = NULL;
}
