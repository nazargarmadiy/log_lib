#ifndef COMMON_DEFS_H
#define COMMON_DEFS_H

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

/*types*/
#define byte_t unsigned char
#define stat_t struct stat
#define time_tm_t struct tm
#define timespec_t struct timespec

typedef unsigned char bool_t;
enum {
    false = 0,
    true
};

#define UNUSED(x) (void)x;

/*defined errors*/
#define ERR_NO_ERR    0
#define ERR_SYS_CALL  1000
#define ERR_NULL_PTR  1001
#define ERR_BAD_COND  1002
#define ERR_UNXP_CASE 1003

/*IO modes*/
#define IO_READ                       "r"
#define IO_READ_WRITE                 "r+"
#define IO_TRUNC_OR_CREATE_READ       "w"
#define IO_TRUNC_OR_CREATE_READ_WRITE "w+"
#define IO_APPEND_OR_CREATE           "a"
#define IO_APPEND_OR_CREATE_READ      "a+"

#define FILE_NAME_MAX_LEN    100
#define COMPRESS_CMD_MAX_LEN (FILE_NAME_MAX_LEN * 3)

#endif /*COMMON_DEFS_H*/
