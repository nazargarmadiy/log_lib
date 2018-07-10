#ifndef WRITE_BUFF_H_
#define WRITE_BUFF_H_

#include <stdio.h>

typedef struct {
    void         *data_buff;
    unsigned int  data_len;
} w_buff_node_t;

typedef struct {
    unsigned int   max_size;
    unsigned int   curr_size;
    unsigned int   total_data_len;
    w_buff_node_t *nodes_arr;
} w_buff_inst_t;

int wbuff_init(w_buff_inst_t *buff_inst, unsigned int size);
int wbuff_deinit(w_buff_inst_t *buff_inst);
int wbuff_flush(w_buff_inst_t *buff_inst, FILE *file);
int wbuff_dump(w_buff_inst_t *buff_inst, void **dump, unsigned int *dump_len);
int wbuff_clear(w_buff_inst_t *buff_inst);
int wbuff_push(w_buff_inst_t *buff_inst,
               const void    *log_data,
               unsigned int   data_len);

#endif /* WRITE_BUFF_H_ */
