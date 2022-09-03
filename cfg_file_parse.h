/*
 * cfg_file_parces.h
 *
 *  Created on: Aug 18, 2019
 *      Author: vv
 */

#ifndef CFG_FILE_PARSE_H
#define CFG_FILE_PARSE_H

#include <sys/types.h>
#include <unistd.h>
#include "log_unit.h"
#include "log_inst.h"

/*
 * TODO:
 * - check if cfg file exist;
 * - read and parse cgg file;
 * - create and add .c file into Make file;
 * - track file changes ?
 * - how to change configuration "on fly" ? use read_write_lock ?
 *
 * https://www.linuxjournal.com/article/8478
 * https://lwn.net/Articles/604686/
 */

/*
 * config file structure:
 * comments and fields in the separate lines;
 * whitespace and tabs are ignored;
 * #text - comment;
 *
 * instance_id - string (instance iD)
 * {
 *     file:file_path - string (log file location)
 *     buff_size:size - int (amount of the records in the buffer)
 *     buff_type:type - enum(buffer_type_t, BUFF_TYPE_RING, BUFF_TYPE_LIST)
 *     rotate_file_cnt:count - int (max. amount of the rotated files)
 *     rotate_file_prefix:prefix - string (name prefix for the rotated log files)
 *     file_max_size:size - int (max. file size in bytes)
 * }
 */

/*
 *read and parse config file;
 *allocate memory and apply config for each log_instance;
 *do not call init for log instance;
 */
int read_apply_cfg_file(const char *file_path, log_unit_t *log_unit);

/*
 * start watch function in the separate thread;
 * apply configuration to the log_instance once config file changed;
 * call deinit/init for log instance;
 */
int watch_start_cfg_file(const char *file_path, pid_t *watch_pid);

/*
 * stop thread for config file watcher;
 */
int watch_stop_cfg_file(pid_t *watch_pid);

#endif /* CFG_FILE_PARSE_H */
