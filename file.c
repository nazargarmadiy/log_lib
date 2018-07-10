#include <stdlib.h>
#include "file.h"
#include "sanity.h"

static int get_file_stat(const char *path, stat_t *st);

int create_open_file(const char *path, const char *mode, FILE **file)
{
    int err = ERR_NO_ERR;

    sanity_null_ptr(path);
    sanity_null_ptr(mode);
    sanity_null_ptr(file);

    *file = fopen(path, mode);

    sanity_null_ptr(*file);

exit_label:
    return err;
}

int write_str_file(FILE *file, const char *str)
{
    int err = ERR_NO_ERR;
    int sys_err = 0;

    sanity_null_ptr(file);
    sanity_null_ptr(str);

    sys_err = fputs(str, file);

    sanity_require(sys_err > 0);

exit_label:
    return err;
}

int close_file(FILE **file)
{
    int err = ERR_NO_ERR;
    int sys_err = 0;

    sanity_null_ptr(file);

    sys_err = fclose(*file);
    sanity_require(!sys_err);

    *file = NULL;

exit_label:
    return err;
}

int size_bt_file(const char *path, long long *size_bt)
{
    int    err = ERR_NO_ERR;
    int    sys_err = 0;
    stat_t st = {0};

    sanity_null_ptr(path);
    sanity_null_ptr(size_bt);

    sys_err = get_file_stat(path, &st);
    sanity_require(!sys_err);

    *size_bt = st.st_size;

exit_label:
    return err;
}

int delete_file(const char *path)
{
    int err = ERR_NO_ERR;
    int sys_err = 0;

    sanity_null_ptr(path);

    sys_err = remove(path);
    sanity_require(!sys_err);

exit_label:
    return err;
}

int write_buff_file(FILE *file, byte_t *buff, unsigned int buff_size)
{
    int err = ERR_NO_ERR;
    int sys_err = 0;

    sanity_null_ptr(file);
    sanity_null_ptr(buff);
    sanity_require(buff_size);

    sys_err = fwrite(buff, buff_size, 1, file);
    /*fwrite shall return the number of elements successfully written*/
    sanity_require(sys_err = 1);

    sys_err = fflush(file);
    sanity_require(sys_err == 0);

exit_label:
    return err;
}

int rename_file(const char *old_name, const char *new_name)
{
    int err = ERR_NO_ERR;
    int sys_err = 0;

    sanity_null_ptr(old_name);
    sanity_null_ptr(new_name);

    sys_err = rename(old_name, new_name);
    sanity_require(!sys_err)

exit_label:
    return err;
}

int truncate_file(const char *path, unsigned int len)
{
    int err = ERR_NO_ERR;
    int sys_err = 0;

    sanity_null_ptr(path);

    sys_err = truncate(path, len);
    sanity_require(!sys_err);

exit_label:
    return err;
}

int file_exist(const char *path, bool_t *is_exist)
{
    int err = ERR_NO_ERR;
    int sys_err = 0;

    sanity_null_ptr(path);
    sanity_null_ptr(is_exist);

    sys_err = access(path, F_OK);

   *is_exist = sys_err ? false : true;

exit_label:
    return err;
}

int rotate_file_maybe(const char          *path,
                      rotate_file_info_t   rt_file_info,
                      FILE               **in_out_fd)
{
    int          err = ERR_NO_ERR;
    char         next_filename[FILE_NAME_MAX_LEN] = {0};
    char         curr_filename[FILE_NAME_MAX_LEN] = {0};
    long long    curr_file_size = 0;
    bool_t       was_open = false;
    bool_t       f_exist = false;
    unsigned int curr_file_num = 0;

    sanity_null_ptr(path);
    sanity_null_ptr(rt_file_info.rt_sufix);
    sanity_null_ptr(in_out_fd);
    sanity_require(rt_file_info.max_file_size);
    sanity_require(rt_file_info.max_file_cnt);

/*
 * -get current file size, if it is less than max_file_size - exit;
 * -iterate files from last to most recent rotate (from max_file_cnt to 1);
 * -if last file exist - delete it;
 * -decrease index;
 * -rename file e.g (log.3 to log.4);
 * -close current file, clear in_out_fd;
 * -rename current file (to first backup, e. g. from log to log.1);
 * -create new file, open if needed;
 */

    err = size_bt_file(path, &curr_file_size);
    sanity_err(err);

    if(curr_file_size < rt_file_info.max_file_size){
        goto exit_label;
    }

    was_open = (*in_out_fd)? true : false;
    curr_file_num = rt_file_info.max_file_cnt;

    if(1 == curr_file_num){
        err = get_rotate_filename(path,
                                  rt_file_info.rt_sufix,
                                  curr_file_num,
                                  next_filename);
        sanity_err(err);
    }

    while(curr_file_num > 1){
        err = get_rotate_filename(path,
                                  rt_file_info.rt_sufix,
                                  curr_file_num,
                                  curr_filename);
        sanity_err(err);

        err = file_exist(curr_filename, &f_exist);
        sanity_err(err);

        if(f_exist && curr_file_num == rt_file_info.max_file_cnt){
            /*if this is existing last file - delete it*/
            err = delete_file(curr_filename);
            sanity_err(err);
        }

        curr_file_num--;

        err = get_rotate_filename(path,
                                  rt_file_info.rt_sufix,
                                  curr_file_num, next_filename);
        sanity_err(err);

        err = file_exist(next_filename, &f_exist);
        sanity_err(err);

        if(!f_exist){
        	continue;
        }

        err = rename_file(next_filename, curr_filename);
        sanity_err(err);
    }

    if(was_open){
        err = close_file(in_out_fd);
        sanity_err(err);
    }

    err = rename_file(path, next_filename);
    sanity_err(err);

    err = create_open_file(path, IO_APPEND_OR_CREATE, in_out_fd);
    sanity_err(err);

    if(!was_open){
        err = close_file(in_out_fd);
        sanity_err(err);
    }

exit_label:
    return err;
}

int compress_file(const char *in_file, const char *out_file)
{
    int         err = ERR_NO_ERR;
    int         sys_err = 0;
    int         num_chars = 0;
    bool_t      exist = false;
    char        cmd[COMPRESS_CMD_MAX_LEN] = {0};
    const char *cmd_flags = "-czf";
    /*compress flags:
	 * -c: create archive;
	 * -z: compress with gzip;
	 * -f: specify filename of the archive;
	 * */

    sanity_null_ptr(in_file);
    sanity_null_ptr(out_file);

    err = file_exist(in_file, &exist);
    sanity_err(err);
    sanity_require(exist);

    num_chars = snprintf(cmd,
                         COMPRESS_CMD_MAX_LEN,
                         "tar %s %s %s",
                         cmd_flags,
                         out_file,
                         in_file);
    sanity_require(num_chars < COMPRESS_CMD_MAX_LEN);

    sys_err = system(cmd);
    sanity_require(!sys_err);


exit_label:
    return err;
}

static int get_file_stat(const char *path, stat_t *st)
{
    int err = ERR_NO_ERR;
    int sys_err = 0;

    sanity_null_ptr(path);
    sanity_null_ptr(st);

    sys_err = stat(path, st);
    sanity_require(!sys_err);

exit_label:
    return err;
}

int get_rotate_filename(const char *path,
                        const char *rt_sufix,
                        int         file_numb,
                        char       *file_name)
{
    int err = ERR_NO_ERR;
    int num_chars = 0;

    sanity_null_ptr(path);
    sanity_null_ptr(rt_sufix);
    sanity_null_ptr(file_name);

    num_chars = snprintf(file_name,
                         FILE_NAME_MAX_LEN,
                         "%s.%i.%s",
                         path,
                         file_numb,
                         rt_sufix);
    sanity_require(num_chars < FILE_NAME_MAX_LEN);

exit_label:
    return err;
}

int read_buff_file(const char    *path,
                   FILE          *in_file,
                   byte_t       **buff,
                   unsigned int  *buff_size)
{
    int err = ERR_NO_ERR;
    long long file_size = 0;


    sanity_null_ptr(path);
    sanity_null_ptr(in_file);
    sanity_null_ptr(buff_size);
    sanity_null_ptr(buff);
    sanity_require(NULL == *buff);

    err = size_bt_file(path, &file_size);
    sanity_err(err);

    *buff = calloc(1, file_size);
    sanity_null_ptr(*buff);

    *buff_size = fread(*buff, file_size, 1, in_file);

exit_label:
    return err;
}
