#include "ring_buffer.h"
#include "sanity.h"
#include "common_func.h"
#include <string.h>
#include <stdlib.h>

static int free_rbuf_node_foreach_cb(void         **node_data,
                                     unsigned int  *data_len,
                                     void          *context);

int rbuf_init(r_buf_inst_t *rbuf_inst, unsigned int size)
{
    int          err = ERR_NO_ERR;
    rbuf_node_t *curr_node = NULL;
    rbuf_node_t *next_node = NULL;

    sanity_null_ptr(rbuf_inst);
    sanity_require(size);

    memset(rbuf_inst, 0, sizeof(r_buf_inst_t));

    rbuf_inst->head = curr_node = calloc(1, sizeof(rbuf_node_t));
    sanity_null_ptr(rbuf_inst->head);
    curr_node->next = curr_node;

    for(int i = 1; i < size; i++){
        next_node = calloc(1, sizeof(rbuf_node_t));
        sanity_null_ptr(next_node);

        curr_node->next = next_node;
        curr_node = next_node;
    }
    curr_node->next = rbuf_inst->head;

    rbuf_inst->max_size = size;

exit_label:
    if(ERR_NO_ERR != err){
        rbuf_deinit(rbuf_inst);
    }
    return err;
}

int rbuf_deinit(r_buf_inst_t *rbuf_inst)
{
    int          err = ERR_NO_ERR;
    rbuf_node_t *curr_node = NULL;
    rbuf_node_t *tmp_node = NULL;

    sanity_null_ptr(rbuf_inst);

    /*clear all allocated data buff*/
    err = rbuf_foreach(rbuf_inst, free_rbuf_node_foreach_cb, NULL);
    sanity_err(err);

    /*clear all allocated nodes*/
    curr_node = rbuf_inst->head;
    for(int i = 0; i < rbuf_inst->max_size; i++){
        if(!curr_node){
            break;
        }
        tmp_node = curr_node;
        curr_node = curr_node->next;
        safe_free((void**)&tmp_node);
    }

    memset(rbuf_inst, 0, sizeof(r_buf_inst_t));

exit_label:
    return err;
}

int rbuf_foreach(r_buf_inst_t *rbuf_inst, rbuf_foreach_cb_t cb, void *context)
{
    int          err = ERR_NO_ERR;
    rbuf_node_t *curr_node = NULL;

    sanity_null_ptr(rbuf_inst);
    sanity_null_ptr(cb);
    sanity_null_ptr(rbuf_inst->head);

    curr_node = rbuf_inst->head;

    do{
        curr_node = curr_node->next;
        sanity_null_ptr(curr_node);

        err = cb(&(curr_node->data_buff), &(curr_node->data_size), context);
        sanity_err(err);
    } while(curr_node != rbuf_inst->head);

exit_label:
    return err;
}

int rbuf_clear(r_buf_inst_t *rbuf_inst)
{
    int          err = ERR_NO_ERR;
    rbuf_node_t *curr_node = NULL;

    sanity_null_ptr(rbuf_inst);
    sanity_null_ptr(rbuf_inst->head);

    curr_node = rbuf_inst->head;

    do{
        curr_node = curr_node->next;
        sanity_null_ptr(curr_node);

        safe_free(&(curr_node->data_buff));
        curr_node->data_size = 0;
    } while(curr_node != rbuf_inst->head);

    rbuf_inst->total_data_len = 0;
    rbuf_inst->curr_size = 0;

exit_label:
    return err;
}

int rbuf_push(r_buf_inst_t *rbuf_inst, const void *node_data, unsigned int size)
{
    int          err = ERR_NO_ERR;
    rbuf_node_t *tmp_node = NULL;

    sanity_null_ptr(rbuf_inst);
    sanity_null_ptr(node_data);
    sanity_require(size);
    sanity_require(rbuf_inst->max_size);

    tmp_node = rbuf_inst->head;
    safe_free(&(tmp_node->data_buff));

    tmp_node->data_size = size;
    tmp_node->data_buff = malloc(size);
    sanity_null_ptr(tmp_node->data_buff);
    memcpy(tmp_node->data_buff, node_data, size);

    rbuf_inst->head = tmp_node->next;
    rbuf_inst->total_data_len += size;

    if(rbuf_inst->max_size > rbuf_inst->curr_size){
        rbuf_inst->curr_size++;
    }

exit_label:
    return err;
}

int rbuf_dump(r_buf_inst_t *rbuf_inst, void **dump, unsigned int *dump_len)
{
    int            err = ERR_NO_ERR;
    unsigned char *tmp_dump = NULL;
    rbuf_node_t   *curr_node = NULL;

    sanity_null_ptr(rbuf_inst);
    sanity_null_ptr(dump_len);
    sanity_null_ptr(dump);
    sanity_require(NULL == *dump);

    if(!(rbuf_inst->total_data_len)){
        /*buffer empty - skip dumping*/
        *dump_len = 0;
        goto exit_label;
    }

    tmp_dump = calloc(1, rbuf_inst->total_data_len);
    sanity_null_ptr(tmp_dump);
    *dump = tmp_dump;

    curr_node = rbuf_inst->head;
    for(unsigned int i = 0; i < rbuf_inst->max_size; i++){
        sanity_null_ptr(curr_node);

        if(curr_node->data_buff && curr_node->data_size){
            memcpy(tmp_dump, curr_node->data_buff, curr_node->data_size);
            tmp_dump += curr_node->data_size;
        }
        curr_node = curr_node->next;
    }

    *dump_len = rbuf_inst->total_data_len;

exit_label:
    return err;
}

static int free_rbuf_node_foreach_cb(void         **node_data,
                                     unsigned int  *data_len,
                                     void          *context)
{
    int err = ERR_NO_ERR;

    UNUSED(context);
    sanity_null_ptr(node_data);
    sanity_null_ptr(data_len);

    safe_free(node_data);
    *data_len = 0;

exit_label:
    return err;
}

