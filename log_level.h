#ifndef LOG_LEVEL_H
#define LOG_LEVEL_H

typedef enum {
    ALL = 0,
    TRACE,
    DEBUG,
    INFO,
    NOTICE,
    WARNING,
    ERROR,
    FATAL,
    OFF
} log_level_t;

typedef struct {
    log_level_t  val;
    const char  *name;
} val_name_map_t;

const char* enum_to_str(val_name_map_t *map, int val);
const char* log_level_to_str(log_level_t log_level);

#endif /* LOG_LEVEL_H */
