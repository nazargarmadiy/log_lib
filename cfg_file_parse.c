#include <sys/inotify.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "cfg_file_parse.h"
#include "common_defs.h"
#include "common_func.h"
#include "file.h"
#include "sanity.h"

typedef struct {
    char*            inst_id;
    name_val_node_t* name_val_list;
} log_inst_cgf_info_t;

static int   notif_fd = 0;
static int   watch_dscrpt = 0;
/*static pid_t watch_loop_pid = 0;*/

#define WATCH_FLAGS   /*IN_MODIFY | IN_DELETE*/IN_ALL_EVENTS
#define EVENT_SIZE    (sizeof (struct inotify_event))
#define BUFF_LEN      (1024 * (EVENT_SIZE)+16)

#define CFG_DELIMITER       ':'
#define CFG_START_BLCK      '{'
#define CFG_END_BLCK        '}'
#define CFG_FILE            "file"
#define CFG_BUFF_SZ         "buff_size"
#define CFG_BUFF_TP         "buff_type"
#define CFG_RT_FILE_CNT     "rotate_file_count"
#define CFG_RT_FILE_PRFX    "rotate_file_prefix"
#define CFG_FILE_MAX_SZ     "file_max_size"

static int parse_cfg_file(const char           *file_path,
                          log_inst_cgf_info_t **ret_arr,
                          unsigned int         *ret_arr_len);
static int apply_cfg_info_to_unit(log_inst_cgf_info_t *cfg_info_arr,
                                  unsigned int         arr_len,
                                  log_unit_t          *log_unit);
static int apply_cfg_info_to_instance(log_inst_cgf_info_t *cfg_info,
                                      log_inst_t          *log_inst);
void safe_free_cfg_info(log_inst_cgf_info_t *cfg_info_arr);
static int parse_str_to_name_val(const char *line, name_val_node_t *node);
static int watch_cfg_file_loop(void);

static const char* event_to_str(unsigned int event)
{
    switch(event){
        case IN_ACCESS:
            return "IN_ACCESS";
        case IN_MODIFY:
            return "IN_MODIFY";
        case IN_ATTRIB:
            return "IN_ATTRIB";
        case IN_CLOSE_WRITE:
            return "IN_CLOSE_WRITE";
        case IN_CLOSE_NOWRITE:
            return "IN_CLOSE_NOWRITE";
        case IN_CLOSE:
            return "IN_CLOSE";
        case IN_OPEN:
            return "IN_OPEN";
        case IN_MOVED_FROM:
            return "IN_MOVED_FROM";
        case IN_MOVED_TO:
            return "IN_MOVED_TO";
        case IN_MOVE:
            return "IN_MOVE";
        case IN_CREATE:
            return "IN_CREATE";
        case IN_DELETE:
            return "IN_DELETE";
        case IN_DELETE_SELF:
            return "IN_DELETE_SELF";
        case IN_MOVE_SELF:
            return "IN_MOVE_SELF";
        case IN_UNMOUNT:
            return "IN_UNMOUNT";
        case IN_Q_OVERFLOW:
            return "IN_Q_OVERFLOW";
        case IN_IGNORED:
            return "IN_IGNORED";
        case IN_ONLYDIR:
            return "IN_ONLYDIR";
        case IN_DONT_FOLLOW:
            return "IN_DONT_FOLLOW";
        case IN_EXCL_UNLINK:
            return "IN_EXCL_UNLINK";
        case IN_MASK_ADD:
            return "IN_MASK_ADD";
        case IN_ISDIR:
            return "IN_ISDIR";
        case IN_ONESHOT:
            return "IN_ONESHOT";
        default:
            return "UNKNOWN";
    }
}

int read_apply_cfg_file(const char *file_path, log_unit_t *log_unit)
{
    int    err = ERR_NO_ERR;
    bool_t file_found = false;
    size_t path_len = 0;

    sanity_null_ptr(file_path);
    sanity_null_ptr(log_unit);

    err = file_exist(file_path, &file_found);
    sanity_err(err);
    sanity_require(file_found);

    /***/
    unsigned int arr_len = 0;
    log_inst_cgf_info_t *cfg_info_arr = NULL;

    parse_cfg_file(file_path, &cfg_info_arr, &arr_len);

    apply_cfg_info_to_unit(cfg_info_arr, arr_len, log_unit);

    path_len = strlen(file_path);
    log_unit->cfg_file_path = calloc(1, path_len +1);
    sanity_null_ptr(log_unit->cfg_file_path);

    strncpy(log_unit->cfg_file_path, file_path, path_len);

    /***/

exit_label:
/*TODO: free cfg_info_arr*/
    return err;
}

int watch_start_cfg_file(const char *file_path, pid_t *watch_pid)
{
    int    err = ERR_NO_ERR;
    bool_t file_found = false;

    sanity_null_ptr(file_path);
    sanity_null_ptr(watch_pid);

    err = file_exist(file_path, &file_found);
    sanity_err(err);
    sanity_require(file_found);

    notif_fd = inotify_init();
    sanity_require_errno(notif_fd != -1);

    watch_dscrpt = inotify_add_watch(notif_fd, file_path, WATCH_FLAGS);
    sanity_require_errno(notif_fd != -1);

    /*
     * TODO: start watch loop here in separate thread, return pid
     */
    /* test */
    watch_cfg_file_loop();
    /* end test */


exit_label:
    return err;
}

int watch_stop_cfg_file(pid_t *watch_pid)
{
    int err = ERR_NO_ERR;
    int sys_err = 0;

    sanity_null_ptr(watch_pid);
    sanity_require(*watch_pid);

    sys_err = inotify_rm_watch(notif_fd, watch_dscrpt);
    sanity_require(!sys_err);

    watch_pid = 0;

    sys_err = close(notif_fd);
    sanity_require(!sys_err);

    notif_fd = 0;

    /*
     * TODO: close pid here;
     */

exit_label:
    return err;
}

static int watch_cfg_file_loop(void)
{
    int                   err = ERR_NO_ERR;
    int                   len = 0;
    int                   i = 0;
    char                  buf[BUFF_LEN] = {0};
    struct inotify_event *event = NULL;

    sanity_require(notif_fd);
    sanity_require(watch_dscrpt);

    while(1){
        len = read(notif_fd, buf, BUFF_LEN);
        if(len < 0 && errno == EINTR){
            continue;
        }
        i = 0;

        while(i < len){
            event = (struct inotify_event *) &buf[i];
            sanity_null_ptr(event);
            /* test */
            printf("wd=%d mask=%s(%u) cookie=%u len=%u\n",
                   event->wd, event_to_str(event->mask), event->mask,
                   event->cookie, event->len);
            if(event->len){
                printf ("name=%s\n", event->name);
            }
            /* end test */
            /*TODO: do callback here err = read_apply_cfg_file()*/
            i += (EVENT_SIZE + event->len);
        }
    }

exit_label:
    return err;
}

static int parse_cfg_file(const char           *file_path,
                          log_inst_cgf_info_t **ret_arr,
                          unsigned int         *ret_arr_len)
{
    int                  err = ERR_NO_ERR;
    int                  sys_err = 0;
    bool_t               file_found = false;
    bool_t               in_cfg_section = false;
    unsigned int         inst_cnt = 0;
    unsigned int         cfg_sec_cnt = 0;
    FILE                *file = NULL;
    char                *line = NULL;
    size_t               curr_len = 0;
    name_val_node_t     *curr_name_val_node = NULL;
    name_val_node_t     **next_node = NULL;
    log_inst_cgf_info_t *cfg_arr = NULL;

    sanity_null_ptr(file_path);
    sanity_null_ptr(ret_arr);
    sanity_null_ptr(ret_arr_len);

    err = file_exist(file_path, &file_found);
    sanity_err(err);
    sanity_require(file_found);

    err = create_open_file(file_path, "r", &file);
    sanity_err(err);

    /*set file position at the beginning*/
    sys_err = fseek(file, 0, SEEK_SET);
    sanity_require(!sys_err);

    cfg_arr = calloc(1, sizeof(log_inst_cgf_info_t));
    sanity_null_ptr(cfg_arr);

    /*
     * Skip comments -> line begins from #
     * find inst id and begin of the section -> string and next line {
     * generate key - value pairs till ent of the section - }
     */
/*TODO: refactor if .. else chain*/
/*TODO: scip spaces and tabs*/
    while(getline(&line, &curr_len, file) > 0){
        sanity_null_ptr(line);
        sanity_require(curr_len > 0);

        if(strchr(line, 35)){
            /* #: comment, skip line */
            safe_free((void**)&line);
            curr_len = 0;
            continue;
        }

        if(strchr(line, 123)){/*'{'*/
            in_cfg_section = true;
            safe_free((void**)&line);
            curr_len = 0;
            cfg_sec_cnt++;
            continue;
        }

        if(strchr(line, 125)){/*'}'*/
            in_cfg_section = false;
            safe_free((void**)&line);
            curr_len = 0;
            continue;
        }

        if(in_cfg_section){
            /* inside { ... } */
            curr_name_val_node = calloc(1, sizeof(name_val_node_t));
            sanity_null_ptr(curr_name_val_node);

            err = parse_str_to_name_val(line, curr_name_val_node);
            sanity_err(err);

            /*copy address of the created node*/
            *next_node = curr_name_val_node;
            /* move to the next list item */
            next_node = &(curr_name_val_node->next);

        } else {
            /*handle instance id*/
            inst_cnt++;
            cfg_arr = realloc(cfg_arr, sizeof(log_inst_cgf_info_t) * inst_cnt);
            sanity_null_ptr(cfg_arr);

            (cfg_arr + inst_cnt -1)->inst_id = calloc(1, curr_len +1);
            sanity_null_ptr((cfg_arr + inst_cnt -1)->inst_id);

            strncpy((cfg_arr + inst_cnt -1)->inst_id, line, curr_len +1);

            /*save address of the first element */
            next_node = &((cfg_arr + inst_cnt -1)->name_val_list);
        }

        safe_free((void**)&line);
        curr_len = 0;
    }

    /* ensure that config section closed by '}' symbol */
    sanity_require(!in_cfg_section);
    /*ensure that each instance has own config section*/
    sanity_require(in_cfg_section == cfg_sec_cnt);

    *ret_arr = cfg_arr;
    *ret_arr_len = inst_cnt;

    curr_name_val_node = NULL;
    cfg_arr = NULL;

exit_label:
    if(file){
        close_file(&file);
    }
    safe_free((void**)&curr_name_val_node);
    safe_free((void**)&cfg_arr);
    safe_free((void**)&line);
    return err;
}

static int parse_str_to_name_val(const char *line, name_val_node_t *node)
{
    int           err = ERR_NO_ERR;
    char         *name = NULL;
    char         *delimiter = NULL;
    size_t        val_len = 0;

    sanity_null_ptr(line);
    sanity_null_ptr(node);

    delimiter = strchr(line, CFG_DELIMITER);
    sanity_null_ptr(delimiter);

    val_len = delimiter - line;
    sanity_require(val_len > 0);
    name = calloc(1, val_len +1);
    sanity_null_ptr(name);
    strncpy(name, line, val_len);

    if(!strcmp(name, CFG_FILE)){
        /* file option*/
        node->name = CFG_FILE;
        val_len = strlen(delimiter +1);
        sanity_require(val_len > 0);
        node->val.ptr_val = calloc(1, val_len +1);
        sanity_null_ptr(node->val.ptr_val);
        strncpy(node->val.ptr_val, delimiter +1, val_len);
    } else if (!strcmp(name, CFG_BUFF_SZ)){
        /* buff_size option */
        node->name = CFG_BUFF_SZ;
        node->val.ull_val = strtoull(delimiter +1, NULL, 0);
    } else if (!strcmp(name, CFG_BUFF_TP)){
        /* buff_type option */
        node->name = CFG_BUFF_TP;
        if(!strcmp(delimiter +1, "BUFF_TYPE_RING")){
            node->val.ull_val = BUFF_TYPE_RING;
        } else if (!strcmp(delimiter +1, "BUFF_TYPE_LIST")) {
            node->val.ull_val = BUFF_TYPE_LIST;
        } else {
            err = ERR_UNXP_CASE;
            goto exit_label;
        }
    } else if (!strcmp(name, CFG_RT_FILE_CNT)) {
        /* rotate_file_count */
        node->name = CFG_RT_FILE_CNT;
        node->val.ll_val = strtoll(delimiter +1, NULL, 0);
    } else if (!strcmp(name, CFG_RT_FILE_PRFX)){
        /* rotate_file_prefix option */
        node->name = CFG_RT_FILE_PRFX;
        val_len = strlen(delimiter +1);
        sanity_require(val_len > 0);
        node->val.ptr_val = calloc(1, val_len +1);
        sanity_null_ptr(node->val.ptr_val);
        strncpy(node->val.ptr_val, delimiter +1, val_len);
    } else if (!strcmp(name, CFG_FILE_MAX_SZ)){
        /* file_max_size */
        node->name = CFG_FILE_MAX_SZ;
        node->val.ull_val = strtoull(delimiter +1, NULL, 0);
    } else {
        err = ERR_UNXP_CASE;
        goto exit_label;
    }

exit_label:
    safe_free((void**)&name);
    return err;
}

void safe_free_cfg_info(log_inst_cgf_info_t *cfg_info_arr)
{
    name_val_node_t* node = NULL;

    if(!cfg_info_arr){
        return;
    }

    if(cfg_info_arr->inst_id){
        safe_free((void**)&cfg_info_arr->inst_id);
    }

    node = cfg_info_arr->name_val_list;
    while(node){
       safe_free((void**)&node->val);
       node = node->next;
    }
}

static int apply_cfg_info_to_unit(log_inst_cgf_info_t *cfg_info_arr,
                                  unsigned int         arr_len,
                                  log_unit_t          *log_unit)
{
    int         err = ERR_NO_ERR;
    log_inst_t *instances_arr = NULL;
    log_inst_t *curr_instance = NULL;

    sanity_null_ptr(cfg_info_arr);
    sanity_require(arr_len);
    sanity_null_ptr(log_unit);

    instances_arr = calloc(arr_len, sizeof(log_inst_t));
    sanity_null_ptr(instances_arr);

    curr_instance = instances_arr;

    for(unsigned int i = 0; i < arr_len; i++){
        err = get_default_log_inst_cfg(curr_instance);
        sanity_err(err);

        err = apply_cfg_info_to_instance(cfg_info_arr, curr_instance);
        sanity_err(err);

        curr_instance++;
    }

    log_unit->inst_cnt = arr_len;
    log_unit->inst_arr = instances_arr;

    instances_arr = NULL;

exit_label:
    safe_free((void**)&instances_arr);
    return err;
}

static int apply_cfg_info_to_instance(log_inst_cgf_info_t *cfg_info,
                                      log_inst_t          *log_inst)
{
    int              err = ERR_NO_ERR;
    size_t           str_len = 0;
    name_val_node_t *curr_node = NULL;

    sanity_null_ptr(cfg_info);
    sanity_null_ptr(log_inst);
    sanity_null_ptr(cfg_info->inst_id);
    sanity_null_ptr(cfg_info->name_val_list);
    sanity_require(!(log_inst->inst_id));

    str_len = strlen(cfg_info->inst_id);
    log_inst->inst_id = calloc(1, str_len +1);
    sanity_null_ptr(log_inst->inst_id);
    strncpy(log_inst->inst_id,  cfg_info->inst_id, str_len);

    curr_node = cfg_info->name_val_list;

    while(curr_node){
        if(!strcmp(curr_node->name, CFG_FILE)){
            /* file option */
            sanity_null_ptr(curr_node->val.ptr_val);
            str_len = strlen(curr_node->val.ptr_val);
            log_inst->file_path = calloc(1, str_len +1);
            sanity_null_ptr(log_inst->file_path);
            strncpy(log_inst->file_path, curr_node->val.ptr_val, str_len);
        } else if (!strcmp(curr_node->name, CFG_BUFF_SZ)){
            /* buff_size option */
            log_inst->buff_size = curr_node->val.ull_val;
        }
        curr_node = curr_node->next;
    }

exit_label:
    return err;
}
