#include "log_unit.h"
#include "sanity.h"
#include "log_inst.h"
#include "file.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
/*for using vasprintf*/
/*#define _GNU_SOURCE*/
/*
 *TODO: should be refactored
 */

static int log_unit_get_instance(log_unit_t  *unit,
                                 const char  *inst_id,
                                 log_inst_t **inst);

int log_unit_init(log_unit_t *unit)
{
    int         err = ERR_NO_ERR;
    log_inst_t *tmp_inst_ptr = NULL;

    sanity_null_ptr(unit);
    /*sanity_null_ptr(unit->cfg_file_path);*/
    sanity_null_ptr(unit->inst_arr);
    sanity_require(unit->inst_cnt);

    tmp_inst_ptr = unit->inst_arr;
    for(unsigned int i = 0; i < unit->inst_cnt; i++){
        err = init_log_inst(tmp_inst_ptr);
        sanity_err(err);
        tmp_inst_ptr++;
    }

exit_label:
    return err;
}

int log_unit_deinit(log_unit_t *unit)
{
    int         err = ERR_NO_ERR;
    log_inst_t *tmp_inst_ptr = NULL;

    sanity_null_ptr(unit);
    sanity_null_ptr(unit->inst_arr);
    sanity_require(unit->inst_cnt);

    tmp_inst_ptr = unit->inst_arr;
    for(unsigned int i = 0; i < unit->inst_cnt; i++){
        err = deinit_log_inst(tmp_inst_ptr);
        sanity_err(err);
        tmp_inst_ptr++;
    }
    safe_free((void**)&(unit->cfg_file_path));

exit_label:
    return err;
}

int log_unit_write_bin_log(log_unit_t   *unit,
                           log_level_t   level,
                           const void   *log_data,
                           unsigned int  log_len)
{
    int         err = ERR_NO_ERR;
    log_inst_t *tmp_inst_ptr = NULL;

    sanity_null_ptr(unit);
    sanity_null_ptr(unit->inst_arr);
    sanity_require(unit->inst_cnt);
    sanity_null_ptr(log_data);
    sanity_require(log_len);

    tmp_inst_ptr = unit->inst_arr;
    for(unsigned int i = 0; i < unit->inst_cnt; i++){
        err = write_log_inst(tmp_inst_ptr, level, log_data, log_len);
        sanity_err(err);
        tmp_inst_ptr++;
    }

exit_label:
    return err;
}

int log_unit_write_str_log(log_unit_t  *unit,
                           log_level_t  level,
                           const char  *log_str)
{
    int           err = ERR_NO_ERR;
    log_inst_t   *tmp_inst_ptr = NULL;
    unsigned int  log_len = 0;

    sanity_null_ptr(unit);
    sanity_null_ptr(unit->inst_arr);
    sanity_require(unit->inst_cnt);
    sanity_null_ptr(log_str);

    /* Length not include terminating symbol  */
    log_len = strlen(log_str);
    sanity_require(log_len);

    tmp_inst_ptr = unit->inst_arr;
    for(unsigned int i = 0; i < unit->inst_cnt; i++){
        err = write_log_inst(tmp_inst_ptr, level, log_str, log_len);
        sanity_err(err);
        tmp_inst_ptr++;
    }

exit_label:
    return err;
}

int log_unit_write_str_fmt_log(log_unit_t *unit,
                               log_level_t level,
                               const char *fmt,
                               ...)
{
    int         err = ERR_NO_ERR;
    char        log_str[MAX_FMT_STR_LEN] = {0};
    va_list     args;
    int         str_len = 0;
    log_inst_t *tmp_inst_ptr = NULL;

    sanity_null_ptr(unit);
    sanity_null_ptr(fmt);

    va_start(args,fmt);
    /* Length not include terminating symbol  */
    /*str_len = vasprintf(&log_str, fmt, args);*/
    str_len = vsprintf(log_str, fmt, args);
    va_end(args);
    sanity_require(str_len > 0);

    tmp_inst_ptr = unit->inst_arr;
    for(unsigned int i = 0; i < unit->inst_cnt; i++){
        err = write_log_inst(tmp_inst_ptr,
                             level,
                             log_str,
                             (unsigned int)str_len);
        sanity_err(err);
        tmp_inst_ptr++;
    }

exit_label:
#if 0
    if(log_str){
        free(log_str);
    }
#endif
    return err;
}

int log_unit_get_inst_list(log_unit_t *unit, char **insts)
{
    int           err = ERR_NO_ERR;
    log_inst_t   *tmp_inst_ptr = NULL;
    unsigned int  total_len = 0;
    unsigned int  curr_len = 0;
    char         *insts_tmp = NULL;

    sanity_null_ptr(unit);
    sanity_null_ptr(insts);
    sanity_require(NULL == *insts);

    tmp_inst_ptr = unit->inst_arr;
    for(unsigned int i = 0; i < unit->inst_cnt; i++){
        curr_len = strlen(tmp_inst_ptr->inst_id);
        sanity_require(curr_len);
        /*increase total len by str len + ';' and backspace*/
        total_len += curr_len + 2;
        tmp_inst_ptr++;
    }

    /* increase for terminating symbol */
    total_len++;
    insts_tmp = calloc(1, total_len);
    sanity_null_ptr(insts_tmp);
    *insts = insts_tmp;

    tmp_inst_ptr = unit->inst_arr;
    for(unsigned int i = 0; i < unit->inst_cnt; i++){
        strcat(insts_tmp, tmp_inst_ptr->inst_id);
        strcat(insts_tmp, "; ");
        tmp_inst_ptr++;
    }

exit_label:
    return err;
}

int log_unit_get_file_list(log_unit_t  *unit,
                           const char  *inst_id,
                           char       **files)
{
    int           err = ERR_NO_ERR;
    log_inst_t   *inst_ptr = NULL;
    unsigned int  total_len = 0;
    unsigned int  curr_len = 0;
    char         *files_tmp = NULL;
    bool_t        file_found = false;
    char          filename[FILE_NAME_MAX_LEN] = {0};

    sanity_null_ptr(unit);
    sanity_null_ptr(inst_id);
    sanity_null_ptr(files);
    sanity_require(NULL == *files);

    err = log_unit_get_instance(unit, inst_id, &inst_ptr);
    sanity_err(err);

    if(!inst_ptr){
        goto exit_label;
    }

    total_len = strlen(inst_ptr->file_path);
    sanity_require(total_len);
    /*increase total len by ';' and backspace*/
    total_len += 2;

    for(unsigned int i = 0; i < inst_ptr->rt_file_opt.max_file_cnt; i++){
        memset(filename, 0, FILE_NAME_MAX_LEN);
        err = get_rotate_filename(inst_ptr->file_path,
                                  inst_ptr->rt_file_opt.rt_sufix,
                                  (unsigned int)i,
                                  filename);
        sanity_err(err);

        err = file_exist(filename, &file_found);
        sanity_err(err);

        if(!file_found){
            continue;
        }
        curr_len = strlen(filename);
        sanity_require(curr_len);
        /*increase total len by str len + ';' and backspace*/
        total_len += curr_len + 2;
    }

    /* increase for terminating symbol */
    total_len++;
    files_tmp = calloc(1, total_len);
    sanity_null_ptr(files_tmp);
    *files = files_tmp;

    strcat(files_tmp, inst_ptr->file_path);
    strcat(files_tmp, "; ");
    for(unsigned int i = 0; i < inst_ptr->rt_file_opt.max_file_cnt; i++){
        memset(filename, 0, FILE_NAME_MAX_LEN);
        err = get_rotate_filename(inst_ptr->file_path,
                                  inst_ptr->rt_file_opt.rt_sufix,
                                  (unsigned int)i,
                                  filename);
        sanity_err(err);

        err = file_exist(filename, &file_found);
        sanity_err(err);

        if(!file_found){
            continue;
        }
        strcat(files_tmp, filename);
    }

exit_label:
    return err;
}

int log_unit_remove_files(log_unit_t *unit,
                          const char *inst_id)
{
    int         err = ERR_NO_ERR;
    log_inst_t *inst_ptr = NULL;

    sanity_null_ptr(unit);
    sanity_null_ptr(inst_id);

    err = log_unit_get_instance(unit, inst_id, &inst_ptr);
    sanity_err(err);
    sanity_null_ptr(inst_ptr);

    /*
     * TODO: implement function "remove_files_log_inst()"
     */

exit_label:
    return err;
}

int log_unit_read_inst(log_unit_t    *unit,
                       const char    *inst_id,
                       void         **log_data,
                       unsigned int  *len)
{
    int         err = ERR_NO_ERR;
    log_inst_t *inst_ptr = NULL;

    sanity_null_ptr(unit);
    sanity_null_ptr(inst_id);
    sanity_null_ptr(len);
    sanity_null_ptr(log_data);
    sanity_require(NULL == *log_data);

    err = log_unit_get_instance(unit, inst_id, &inst_ptr);
    sanity_err(err);
    sanity_null_ptr(inst_ptr);

    err = read_log_inst(inst_ptr, log_data, len);
    sanity_err(err);

exit_label:
    return err;
}

int log_unit_rotate_force(log_unit_t *unit,
                          const char *inst_id)
{
    int         err = ERR_NO_ERR;
    log_inst_t *inst_ptr = NULL;

    sanity_null_ptr(unit);
    sanity_null_ptr(inst_id);

    err = log_unit_get_instance(unit, inst_id, &inst_ptr);
    sanity_err(err);
    sanity_null_ptr(inst_ptr);

    err = rotatate_force_log_inst(inst_ptr);
    sanity_err(err);

exit_label:
    return err;
}

static int log_unit_get_instance(log_unit_t  *unit,
                                 const char  *inst_id,
                                 log_inst_t **inst)
{
    int         err = ERR_NO_ERR;
    log_inst_t *inst_ptr = NULL;

    sanity_null_ptr(unit);
    sanity_null_ptr(inst_id);
    sanity_null_ptr(inst);
    sanity_require(NULL == *inst);

    inst_ptr = unit->inst_arr;
    for(unsigned int i = 0; i < unit->inst_cnt; i++){
        if(!strcmp(inst_id, inst_ptr->inst_id)){
            *inst = inst_ptr;
            break;
        }
        inst_ptr++;
    }

exit_label:
    return err;
}

