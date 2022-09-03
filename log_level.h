#ifndef LOG_LEVEL_H
#define LOG_LEVEL_H

#include "common_func.h"

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

const char* log_level_to_str(log_level_t log_level);

#endif /* LOG_LEVEL_H */
