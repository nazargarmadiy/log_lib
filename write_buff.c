#include "write_buff.h"
#include "sanity.h"
#include "string.h"
#include "file.h"
#include "common_func.h"
#include <stdlib.h>

int wbuff_init(w_buff_inst_t *buff_inst, unsigned int size)
{
    int err = ERR_NO_ERR;

    sanity_null_ptr(buff_inst);
    sanity_require(size);

    memset(buff_inst, 0, sizeof(w_buff_inst_t));

    buff_inst->max_size = size;
    buff_inst->nodes_arr = calloc(size, sizeof(w_buff_node_t));

exit_label:
    return err;
}

int wbuff_deinit(w_buff_inst_t *buff_inst)
{
    int err = ERR_NO_ERR;

    sanity_null_ptr(buff_inst);

    err = wbuff_clear(buff_inst);
    sanity_err(err);

    if(buff_inst->nodes_arr){
        safe_free((void**)&(buff_inst->nodes_arr));
    }

    memset(buff_inst, 0, sizeof(*buff_inst));

exit_label:
    return err;
}

int wbuff_flush(w_buff_inst_t *buff_inst, FILE *file)
{
    int            err = ERR_NO_ERR;
    w_buff_node_t *node = NULL;

    sanity_null_ptr(buff_inst);
    sanity_null_ptr(file);
    sanity_require(buff_inst->max_size);

    if(!(buff_inst->total_data_len)){
        /*buffer empty - skip dumping*/
        goto exit_label;
    }

    while(buff_inst->curr_size){
        node = buff_inst->nodes_arr + buff_inst->curr_size -1;
        sanity_require(node->data_buff && node->data_len);

        err = write_buff_file(file, node->data_buff, node->data_len);
        sanity_err(err);

        safe_free(&(node->data_buff));
        memset(node, 0, sizeof(w_buff_node_t));

        (buff_inst->curr_size)--;
    }

    buff_inst->total_data_len = 0;

exit_label:
    return err;
}

int wbuff_dump(w_buff_inst_t *buff_inst, void **dump, unsigned int *dump_len)
{
    int            err = ERR_NO_ERR;
    w_buff_node_t *node = NULL;
    unsigned char *tmp_dump = NULL;

    sanity_null_ptr(buff_inst);
    sanity_null_ptr(dump_len);
    sanity_null_ptr(dump);
    sanity_require(NULL == *dump);
    sanity_require(buff_inst->max_size);

    *dump_len = buff_inst->total_data_len;
    if(!(buff_inst->total_data_len)){
        /*buffer empty - skip dumping*/
        goto exit_label;
    }

    tmp_dump = calloc(1, buff_inst->total_data_len);
    sanity_null_ptr(tmp_dump);
    *dump = tmp_dump;

    for(unsigned int i = 0; i < buff_inst->curr_size; i++){
        node = buff_inst->nodes_arr + i;
        sanity_require(node->data_buff && node->data_len);

        memcpy(tmp_dump, node->data_buff, node->data_len);
        tmp_dump += node->data_len;
    }

exit_label:
    return err;
}

int wbuff_clear(w_buff_inst_t *buff_inst)
{
    int            err = ERR_NO_ERR;
    w_buff_node_t *node = NULL;

    sanity_null_ptr(buff_inst);
    sanity_require(buff_inst->max_size);

    while(buff_inst->curr_size){
        node = buff_inst->nodes_arr + buff_inst->curr_size -1;
        sanity_require(node->data_buff && node->data_len);

        safe_free(&(node->data_buff));
        memset(node, 0, sizeof(w_buff_node_t));

        (buff_inst->curr_size)--;
        }

    buff_inst->total_data_len = 0;

exit_label:
    return err;
}

int wbuff_push(w_buff_inst_t *buff_inst,
               const void    *log_data,
               unsigned int   data_len)
{
    int            err = ERR_NO_ERR;
    w_buff_node_t *node = NULL;
    unsigned int   curr_index = buff_inst->max_size;

    sanity_null_ptr(buff_inst);
    sanity_null_ptr(log_data);
    sanity_require(data_len);
    sanity_require(buff_inst->max_size);
    sanity_require(buff_inst->max_size < buff_inst->curr_size);

    node = buff_inst->nodes_arr;

    while(node->data_buff && curr_index){
        curr_index--;
        node++;
    }
    if(!curr_index){
        err = ERR_BAD_COND;
        goto exit_label;
    }

    node->data_buff = malloc(data_len);
    memcpy(node->data_buff, log_data, data_len);
    node->data_len = data_len;
    (buff_inst->curr_size)++;
    buff_inst->total_data_len += data_len;

exit_label:
    return err;
}
