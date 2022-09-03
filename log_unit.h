#ifndef LOG_UNIT_H
#define LOG_UNIT_H

#include "log_inst.h"

/*
 * top of the ierarhy
 * contains set of the log_instances
 */
typedef struct {
     log_inst_t   *inst_arr;
     unsigned int  inst_cnt;
     char         *cfg_file_path;
} log_unit_t;

#define MAX_FMT_STR_LEN 1024

int log_unit_init(log_unit_t *unit);
int log_unit_deinit(log_unit_t *unit);
int log_unit_write_bin_log(log_unit_t   *unit,
                           log_level_t   level,
                           const void   *log_data,
                           unsigned int  log_len);
int log_unit_write_str_log(log_unit_t  *unit,
                           log_level_t  level,
                           const char  *log_str);
int log_unit_write_str_fmt_log(log_unit_t *unit,
                               log_level_t level,
                               const char *fmt,
                               ...);
int log_unit_get_inst_list(log_unit_t *unit, char **insts);
int log_unit_get_file_list(log_unit_t  *unit,
                           const char  *inst_id,
                           char       **files);
int log_unit_remove_files(log_unit_t *unit,
                          const char *inst_id);
int log_unit_read_inst(log_unit_t    *unit,
                       const char    *inst_id,
                       void         **log_data,
                       unsigned int  *len);
int log_unit_rotate_force(log_unit_t *unit,
                          const char *inst_id);

#endif /* LOG_UNIT_H */
