#include "log_level.h"
#include "sanity.h"

static val_name_map_t log_level_map [] = {
        {.val = ALL, .name = "ALL"},
        {.val = TRACE, .name = "TRACE"},
        {.val = DEBUG, .name = "DEBUG"},
        {.val = INFO, .name = "INFO"},
        {.val = NOTICE, .name = "NOTICE"},
        {.val = WARNING, .name = "WARNING"},
        {.val = ERROR, .name = "ERROR"},
        {.val = FATAL, .name = "FATAL"},
        {.val = OFF, .name = "OFF"},
        {.val = 0, .name = NULL},
};

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
const char* log_level_to_str(log_level_t log_level)
{
    return enum_to_str(log_level_map, log_level);
}
