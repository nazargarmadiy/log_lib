#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include "common_defs.h"

typedef struct {
    char         *rt_sufix;
    long long     max_file_size;
    unsigned int  max_file_cnt;
} rotate_file_info_t;

int create_open_file(const char *path, const char *mode, FILE **in_file);
int close_file(FILE **in_file);
int delete_file(const char *path);
int rename_file(const char *old_name, const char *new_name);
int truncate_file(const char *path, unsigned int len);
int file_exist(const char *path, bool_t *is_exist);
int size_bt_file(const char *path, long long *size_bt);
int write_str_file(FILE *in_file, const char *str);
int write_buff_file(FILE *in_file, byte_t *buff, unsigned int buff_size);
int read_buff_file(const char    *path,
                   FILE          *in_file,
                   byte_t       **buff,
                   unsigned int  *buff_size);
int rotate_file_maybe(const char          *path,
                      rotate_file_info_t   rt_file_info,
                      FILE               **in_out_fd);
int compress_file(const char *in_file, const char *out_file);
int get_rotate_filename(const char *path,
                        const char *rt_sufix,
                        int         file_numb,
                        char       *file_name);

#endif /*FILE_H*/
