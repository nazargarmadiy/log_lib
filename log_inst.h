#ifndef LOG_INST_H
#define LOG_INST_H

#include "common_defs.h"
#include "file.h"
#include "log_level.h"
#include <pthread.h>

typedef enum {
    BUFF_TYPE_NONE = 0,
	BUFF_TYPE_RING,
	BUFF_TYPE_LIST,
	BUFF_TYPE_MAX = BUFF_TYPE_LIST
} buffer_type_t;

/*
 * log decorator callback
 * must use malloc/calloc for allocation dcrt_log
 */
typedef int (*log_decorator_cb_t)(const void    *log_str,
                                  unsigned int   data_len,
                                  log_level_t    level,
                                  void         **dcrt_log,
                                  unsigned int  *dcrt_data_len,
                                  void          *cb_data);

/*log filter callback*/
typedef int (*log_filter_cb_t)(const void   *log_data,
                               unsigned int  data_len,
                               log_level_t   level,
                               bool_t       *filter_passed,
                               void         *cb_data);

typedef struct {
    char               *inst_id;
    FILE               *file;
    char               *file_path;
    pthread_mutex_t    *write_mtx;
    rotate_file_info_t  rt_file_opt;
    void               *buffer_inst;
    buffer_type_t       buffer_type;
    /*
     * use buff size 1 if need write to file immediately
     * other wise logs will be written into file when
     * buffer full
     */
    unsigned int        buff_size;
    log_decorator_cb_t  decorator_cb;
    void               *decorator_data;
    log_filter_cb_t     filter_cb;
    void               *filter_data;
} log_inst_t;

/* Default values */
#define RT_FIlE_DFLT_SUFIX     "bak"
#define RT_FILE_DFLT_FILE_SIZE 10000000
#define RT_FILE_DFLT_FILE_CNT  10
#define INST_DFLT_FILE         "log"
#define INST_DFLT_BUFF_TYPE    BUFF_TYPE_RING
#define INST_DFLT_BUFF_SIZE    10000

int get_default_log_inst_cfg(log_inst_t *log_inst_cfg);

int get_default_rt_file_cfg(rotate_file_info_t *rt_file_cfg);

/*
 * - open file descriptor;
 * - initialize write_mtx;
 * - initialize buffer_inst;
 */
int init_log_inst(log_inst_t *log_inst);

/*
 * - close file descriptor;
 * - uninitialize write_mtx;
 * - uninitialize buffer_inst;
 */
int deinit_log_inst(log_inst_t *log_inst);

int write_log_inst(log_inst_t   *log_inst,
                   log_level_t   level,
                   const void   *log_data,
                   unsigned int  data_len);
/*
 * - read data from instance;
 * - read data from current file;
 * - write data to buffer;
 */
int read_log_inst(log_inst_t *log_inst, void **log_data, unsigned int *len);

/*
 * - flush buffer into the file;
 * - rotate file;
 */
int rotatate_force_log_inst(log_inst_t *log_inst);
/*
 * flush information from buffer into file
 */
int flush_log_inst(log_inst_t *log_inst);

/*
 * - remove rotated log files;
 * - clear current log file;
 */
int remove_log_files_inst(log_inst_t *log_inst);

/*
 * default log decorator for strings
 * format: [date time] [log level] log string
 */
int log_decorate_str_dflt(const void    *log_str,
                          unsigned int   data_len,
                          log_level_t    level,
                          void         **dcrt_log,
                          unsigned int  *dcrt_data_len,
                          void          *cb_data);

#endif /*LOG_INST_H*/
