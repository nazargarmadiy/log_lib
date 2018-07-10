#ifndef RING_BUFFER_H_
#define RING_BUFFER_H_

typedef struct rbuf_node{
    struct rbuf_node *next;
    void             *data_buff;
    unsigned int      data_size;
} rbuf_node_t;

typedef struct {
    unsigned int  max_size;
    unsigned int  curr_size;
    unsigned int  total_data_len;
    rbuf_node_t  *head;
} r_buf_inst_t;

/*foreach callback*/
typedef int (*rbuf_foreach_cb_t)(void         **node_data,
                                 unsigned int  *data_len,
                                 void          *context);

int rbuf_init(r_buf_inst_t *rbuf_inst, unsigned int size);
int rbuf_deinit(r_buf_inst_t *rbuf_inst);
int rbuf_clear(r_buf_inst_t *rbuf_inst);
int rbuf_push(r_buf_inst_t *rbuf_inst, const void *node_data, unsigned int size);
int rbuf_dump(r_buf_inst_t *rbuf_inst, void **dump, unsigned int *dump_len);
int rbuf_foreach(r_buf_inst_t *rbuf_inst, rbuf_foreach_cb_t cb, void *context);

#endif /* RING_BUFFER_H_ */
