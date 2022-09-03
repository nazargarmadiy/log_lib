#include "log_inst.h"
#include "sanity.h"
#include "file.h"
#include "write_buff.h"
#include "ring_buffer.h"
#include "time_api.h"
#include "common_func.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int decorate_log(log_inst_t    *log_inst,
                        log_level_t    level,
                        const void    *log_data,
                        unsigned int   data_len,
                        void         **dcrt_log,
                        unsigned int  *dcrt_len);

static int filter_log(log_inst_t   *log_inst,
                      log_level_t   level,
                      const void   *log_data,
                      unsigned int  data_len,
                      bool_t       *filter_passed);

static int rotate_log_file_maybe(log_inst_t *log_inst);

static int flush_maybe(log_inst_t    *log_inst,
                       void         **buff,
                       unsigned int  *buff_len,
                       bool_t        *is_flushed);

static int get_curr_max_size(log_inst_t   *log_inst,
                             unsigned int *curr,
                             unsigned int *max);

int get_default_log_inst_cfg(log_inst_t *log_inst_cfg)
{
    int err = ERR_NO_ERR;

    sanity_null_ptr(log_inst_cfg);

    memset(log_inst_cfg, 0, sizeof(log_inst_t));

    err = get_default_rt_file_cfg(&log_inst_cfg->rt_file_opt);
    sanity_err(err);

    log_inst_cfg->buff_size = INST_DFLT_BUFF_SIZE;
    log_inst_cfg->file_path = INST_DFLT_FILE;
    log_inst_cfg->buffer_type = INST_DFLT_BUFF_TYPE;

exit_label:
    return err;
}

int get_default_rt_file_cfg(rotate_file_info_t *rt_file_cfg)
{
    int    err = ERR_NO_ERR;
    size_t len = strlen(RT_FIlE_DFLT_SUFIX);

    sanity_null_ptr(rt_file_cfg);

    memset(rt_file_cfg, 0, sizeof(rotate_file_info_t));

    rt_file_cfg->max_file_cnt = RT_FILE_DFLT_FILE_CNT;
    rt_file_cfg->max_file_size = RT_FILE_DFLT_FILE_SIZE;
    rt_file_cfg->rt_sufix = calloc(1, len +1);
    sanity_null_ptr(rt_file_cfg->rt_sufix);
    strncpy(rt_file_cfg->rt_sufix, RT_FIlE_DFLT_SUFIX, len);

exit_label:
    return err;
}

int init_log_inst(log_inst_t *log_inst)
{
    int err = ERR_NO_ERR;
    int sys_err = 0;

    sanity_null_ptr(log_inst);
    sanity_require(NULL == log_inst->file);
    sanity_require(log_inst->file_path);
    sanity_require(NULL == log_inst->write_mtx);
    sanity_require(log_inst->rt_file_opt.max_file_cnt);
    sanity_require(log_inst->rt_file_opt.max_file_size);
    sanity_null_ptr(log_inst->rt_file_opt.rt_sufix);
    sanity_require(NULL == log_inst->buffer_inst);
    sanity_require(log_inst->buff_size);
    sanity_require(log_inst->buffer_type >= BUFF_TYPE_NONE &&
                   log_inst->buffer_type <= BUFF_TYPE_MAX);
    sanity_null_ptr(log_inst->inst_id);

    err = create_open_file(log_inst->file_path,
                           IO_APPEND_OR_CREATE,
                           &(log_inst->file));
    sanity_err(err);

    log_inst->write_mtx = calloc(1, sizeof(pthread_mutex_t));
    sys_err = pthread_mutex_init(log_inst->write_mtx, NULL);
    sanity_require(!sys_err);

    switch (log_inst->buffer_type) {
        case BUFF_TYPE_LIST:
            log_inst->buffer_inst = malloc(sizeof(w_buff_inst_t));
            err = wbuff_init(log_inst->buffer_inst, log_inst->buff_size);
            sanity_err(err);
            break;

        case BUFF_TYPE_RING:
            log_inst->buffer_inst = malloc(sizeof(r_buf_inst_t));
            err = rbuf_init(log_inst->buffer_inst, log_inst->buff_size);
            sanity_err(err);
            break;

        default:
            err = ERR_UNXP_CASE;
            goto exit_label;
            break;
    }

exit_label:
    return err;
}

int deinit_log_inst(log_inst_t *log_inst)
{
    int err = ERR_NO_ERR;
    int sys_err = 0;

    sanity_null_ptr(log_inst);
    sanity_null_ptr(log_inst->write_mtx);

    err = flush_log_inst(log_inst);
    sanity_err(err);

    if(log_inst->file){
        close_file(&(log_inst->file));
        sanity_err(err);
    }

    sys_err = pthread_mutex_destroy(log_inst->write_mtx);
    sanity_require(!sys_err);
    safe_free((void**)&(log_inst->write_mtx));
    log_inst->write_mtx = NULL;

    switch (log_inst->buffer_type) {
        case BUFF_TYPE_LIST:
            err = wbuff_deinit(log_inst->buffer_inst);
            sanity_err(err);
            break;

        case BUFF_TYPE_RING:
            err = rbuf_deinit(log_inst->buffer_inst);
            sanity_err(err);
            break;

        default:
            err = ERR_UNXP_CASE;
            goto exit_label;
            break;
    }
    safe_free(&(log_inst->buffer_inst));
    log_inst->buffer_inst = NULL;

exit_label:
    return err;
}

int write_log_inst(log_inst_t   *log_inst,
                   log_level_t   level,
                   const void   *log_data,
                   unsigned int  data_len)
{
    int           err = ERR_NO_ERR;
    int           sys_err = 0;
    void         *w_log = NULL;
    bool_t        fltr_passed = false;
    unsigned int  dcrt_len = data_len;
    void         *buff = NULL;
    unsigned int  buff_len = 0;
    bool_t        is_flushed = false;
    bool_t        mutex_locked = false;

    sanity_null_ptr(log_inst);
    sanity_null_ptr(log_data);
    sanity_require(data_len);

    err = filter_log(log_inst,
                     level,
                     log_data,
                     data_len,
                     &fltr_passed);
    sanity_err(err);

    if(!fltr_passed){
        /* filter did not passed - skip logging */
        goto exit_label;
    }

    sys_err = pthread_mutex_lock(log_inst->write_mtx);
    sanity_require(!sys_err);
    mutex_locked = true;

    err = decorate_log(log_inst,
                       level,
                       log_data,
                       data_len,
                       &w_log,
                       &dcrt_len);
    sanity_err(err);

    /*
     * - if buffer is full - flush into buffer;
     * - if flushed: rotate file if needed, write data to file;
     */
    err = flush_maybe(log_inst, &buff, &buff_len, &is_flushed);
    sanity_err(err);

    if(is_flushed){
        err = rotate_log_file_maybe(log_inst);
        sanity_err(err);

        err = write_buff_file(log_inst->file, buff, buff_len);
        sanity_err(err);
    }

    switch (log_inst->buffer_type) {
        case BUFF_TYPE_LIST:
            err = wbuff_push(log_inst->buffer_inst,
                             w_log ? w_log : log_data,
                             dcrt_len);
            sanity_err(err);
            break;

        case BUFF_TYPE_RING:
            err = rbuf_push(log_inst->buffer_inst,
                            w_log ? w_log : log_data,
                            dcrt_len);
            sanity_err(err);
            break;

        default:
            err = ERR_UNXP_CASE;
            goto exit_label;
    }
    /*
    err = rotate_log_file_maybe(log_inst);
    sanity_err(err);
    err = flush_log_inst(log_inst);
    sanity_err(err);
    */

exit_label:
    safe_free(&w_log);
    safe_free(&buff);
    if(mutex_locked){
        pthread_mutex_unlock(log_inst->write_mtx);
    }
    return err;
}

int read_log_inst(log_inst_t *log_inst, void **log_data, unsigned int *len)
{
    int err = ERR_NO_ERR;
    unsigned int total_len = 0;
    unsigned int inst_len = 0;
    unsigned int file_len = 0;
    void *total_data = NULL;
    void *inst_data = NULL;
    void *file_data = NULL;

    sanity_null_ptr(log_inst);
    sanity_null_ptr(len);
    sanity_null_ptr(log_inst->file);
    sanity_null_ptr(log_inst->file_path);
    sanity_null_ptr(log_data);
    sanity_require(NULL == log_data);

    switch (log_inst->buffer_type) {
        case BUFF_TYPE_LIST:
            err = wbuff_dump(log_inst->buffer_inst, &inst_data, &inst_len);
            sanity_err(err);
            break;

        case BUFF_TYPE_RING:
            err = rbuf_dump(log_inst->buffer_inst, &inst_data, &inst_len);
            sanity_err(err);
            break;

        default:
            err = ERR_UNXP_CASE;
            goto exit_label;
    }

    err = read_buff_file(log_inst->file_path,
                         log_inst->file,
                         (byte_t**)&file_data,
                         &file_len);
    sanity_err(err);

    total_len = file_len + inst_len;
    total_data = malloc(total_len);
    sanity_null_ptr(total_data);

    *log_data = total_data;
    *len = total_len;

    memcpy(total_data, inst_data, inst_len);
    total_data += inst_len;

    memcpy(total_data, file_data, file_len);

exit_label:
    safe_free(&inst_data);
    safe_free(&file_data);
    return err;
}

int rotatate_force_log_inst(log_inst_t *log_inst)
{
    int       err = ERR_NO_ERR;
    long long rt_max_file_size = 0;

    sanity_null_ptr(log_inst);
    sanity_null_ptr(log_inst->buffer_inst);
    sanity_null_ptr(log_inst->file_path);

    rt_max_file_size = log_inst->rt_file_opt.max_file_size;
    /*
     * temporary set max file size to 0
     * to enforce file rotation
     */
    log_inst->rt_file_opt.max_file_size = 0;

    err = rotate_log_file_maybe(log_inst);
    log_inst->rt_file_opt.max_file_size = rt_max_file_size;
    sanity_err(err);

exit_label:
    return err;
}

int flush_log_inst(log_inst_t *log_inst)
{
    int           err = ERR_NO_ERR;
    unsigned int  len = 0;
    void         *data_buff = NULL;

    sanity_null_ptr(log_inst);
    sanity_null_ptr(log_inst->buffer_inst);
    sanity_null_ptr(log_inst->file);

    switch(log_inst->buffer_type) {
    case BUFF_TYPE_LIST:
        err = wbuff_dump(log_inst->buffer_inst, &data_buff, &len);
        sanity_err(err);
        err = wbuff_clear(log_inst->buffer_inst);
        sanity_err(err);
        break;

    case BUFF_TYPE_RING:
        err = rbuf_dump(log_inst->buffer_inst, &data_buff, &len);
        sanity_err(err);
        err = rbuf_clear(log_inst->buffer_inst);
        sanity_err(err);
        break;

    default:
        err = ERR_UNXP_CASE;
        goto exit_label;
    }

    err = write_buff_file(log_inst->file, data_buff, len);
    sanity_err(err);

exit_label:
    safe_free(&data_buff);
    return err;
}

int remove_log_files_inst(log_inst_t *log_inst)
{
    int    err = ERR_NO_ERR;
    int    sys_err = 0;
    bool_t mutex_locked = false;
    bool_t file_found = false;
    char   curr_file[FILENAME_MAX] = {0};

    sanity_null_ptr(log_inst);
    sanity_null_ptr(log_inst->file_path);

    sys_err = pthread_mutex_lock(log_inst->write_mtx);
    sanity_require(!sys_err);
    mutex_locked = true;

    for(int i = 0; i < log_inst->rt_file_opt.max_file_cnt; i++){
        err = get_rotate_filename(log_inst->file_path,
                                  log_inst->rt_file_opt.rt_sufix,
                                  i,
                                  curr_file);
        sanity_err(err);

        err = file_exist(curr_file, &file_found);
        sanity_err(err);

        if(file_found){
            err = delete_file(curr_file);
            sanity_err(err);
        }
    }

    err = close_file(&log_inst->file);
    sanity_err(err);

    err = truncate_file(log_inst->file_path, 0);
    sanity_err(err);

    err = create_open_file(log_inst->file_path,
                           IO_APPEND_OR_CREATE,
                           &(log_inst->file));
    sanity_err(err);

exit_label:
    if(mutex_locked){
        pthread_mutex_unlock(log_inst->write_mtx);
    }
    return err;
}

static int decorate_log(log_inst_t    *log_inst,
                        log_level_t    level,
                        const void    *log_data,
                        unsigned int   data_len,
                        void         **dcrt_log,
                        unsigned int  *dcrt_len)
{
    int err = ERR_NO_ERR;

    sanity_null_ptr(log_inst);
    sanity_null_ptr(log_data);
    sanity_null_ptr(dcrt_log);
    sanity_require(NULL == *dcrt_log);
    sanity_null_ptr(dcrt_len);

    if(!(log_inst->decorator_cb)){
        *dcrt_len = data_len;
        goto exit_label;
    }

    err = log_inst->decorator_cb(log_data,
                                 data_len,
                                 level,
                                 dcrt_log,
                                 dcrt_len,
                                 log_inst->decorator_data);
    sanity_err(err);
    sanity_null_ptr(dcrt_log);
    sanity_require(*dcrt_len);

exit_label:
    return err;
}

static int filter_log(log_inst_t   *log_inst,
                      log_level_t   level,
                      const void   *log_data,
                      unsigned int  data_len,
                      bool_t       *filter_passed)
{
    int err = ERR_NO_ERR;

    sanity_null_ptr(log_inst);
    sanity_null_ptr(log_data);
    sanity_null_ptr(filter_passed);

    if(!(log_inst->filter_cb)){
        *filter_passed = true;
        goto exit_label;
    }

    err = log_inst->filter_cb(log_data,
                              data_len,
                              level,
                              filter_passed,
                              log_inst->filter_data);
    sanity_err(err);

exit_label:
    return err;
}

static int rotate_log_file_maybe(log_inst_t *log_inst)
{
    int err = ERR_NO_ERR;

    sanity_null_ptr(log_inst);
    sanity_null_ptr(log_inst->file_path);

    err = rotate_file_maybe(log_inst->file_path,
                            log_inst->rt_file_opt,
                            &(log_inst->file));
    sanity_err(err);

exit_label:
    return err;
}

static int flush_maybe(log_inst_t    *log_inst,
                       void         **buff,
                       unsigned int  *buff_len,
                       bool_t        *is_flushed)
{
    int           err = ERR_NO_ERR;
    unsigned int  max_size = 0;
    unsigned int  curr_size = 0;

    if(is_flushed){
        *is_flushed = false;
    }

    sanity_null_ptr(log_inst);
    sanity_null_ptr(log_inst->buffer_inst);
    sanity_null_ptr(buff);
    sanity_require(NULL == *buff);
    sanity_null_ptr(buff_len);

    err = get_curr_max_size(log_inst, &curr_size, &max_size);
    sanity_err(err);

    if(curr_size < max_size){
        goto exit_label;
    }

    switch (log_inst->buffer_type) {
        case BUFF_TYPE_LIST:
            err = wbuff_dump(log_inst->buffer_inst, buff, buff_len);
            sanity_err(err);
            sanity_null_ptr(buff);
            sanity_require(buff_len);

            err = wbuff_clear(log_inst->buffer_inst);
            sanity_err(err);
            break;

        case BUFF_TYPE_RING:
            err = rbuf_dump(log_inst->buffer_inst, buff, buff_len);
            sanity_err(err);
            sanity_null_ptr(*buff);
            sanity_require(*buff_len);

            err = rbuf_clear(log_inst->buffer_inst);
            sanity_err(err);
            break;

        default:
            err = ERR_UNXP_CASE;
            goto exit_label;
    }

    if(is_flushed){
        *is_flushed = true;
    }

exit_label:
    return err;
}

static int get_curr_max_size(log_inst_t   *log_inst,
                             unsigned int *curr,
                             unsigned int *max)
{
    int err = ERR_NO_ERR;

    sanity_null_ptr(log_inst);
    sanity_null_ptr(curr);
    sanity_null_ptr(max);
    sanity_null_ptr(log_inst->buffer_inst);

    switch (log_inst->buffer_type) {
        case BUFF_TYPE_LIST:
            *max = ((w_buff_inst_t*)log_inst->buffer_inst)->max_size;
            *curr = ((w_buff_inst_t*)log_inst->buffer_inst)->curr_size;
            sanity_require(max);
            break;

        case BUFF_TYPE_RING:
            *max = ((r_buf_inst_t*)log_inst->buffer_inst)->max_size;
            *curr = ((r_buf_inst_t*)log_inst->buffer_inst)->curr_size;
            sanity_require(max);
            break;

        default:
            *curr = 0;
            *max = 0;
            err = ERR_UNXP_CASE;
            goto exit_label;
    }

exit_label:
    return err;
}

int log_decorate_str_dflt(const void    *log_str,
                          unsigned int   data_len,
                          log_level_t    level,
                          void         **dcrt_log,
                          unsigned int  *dcrt_data_len,
                          void          *cb_data)
{
    int           err = ERR_NO_ERR;
    char         *dcrt_str = NULL;
    char         *date_time_str = NULL;
    char          log_level_str[64] = {0};
    unsigned int  total_len = data_len;

    sanity_null_ptr(log_str);
    sanity_require(data_len);
    sanity_require(dcrt_log);
    sanity_require(*dcrt_log == NULL);
    sanity_null_ptr(dcrt_data_len);

    err = get_date_time_str(true, NULL, &date_time_str);
    sanity_err(err);

    total_len += strlen(date_time_str);

    total_len += snprintf(log_level_str, 64, " [%s] ", log_level_to_str(level));

    /*add place to store terminate symbol*/
    total_len ++;

    dcrt_str = calloc(1, total_len);
    sanity_null_ptr(dcrt_str);

    snprintf(dcrt_str,
             total_len,
             "%s%s%s",
             date_time_str,
             log_level_str,
             (const char *)log_str);

    /*do not write termitane symbol after each line*/
    *dcrt_data_len = --total_len;
    *dcrt_log = dcrt_str;

exit_label:
    safe_free((void**)&date_time_str);
    return err;
}
