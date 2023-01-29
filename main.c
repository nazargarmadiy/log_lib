#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "log_lib_hdrs.h"


#define TEST_FILE_NAME "output/TEST_log.txt"

int do_test_cases();

int main(int argc, char **argv)
{
	int   err = ERR_NO_ERR;
	char *time_str = NULL;

	err = get_date_time_str(true, NULL, &time_str);
	sanity_err(err);

	printf("start testing at %s\n", time_str);
	safe_free((void**)&time_str);

	err = do_test_cases();

    err = get_date_time_str(true, NULL, &time_str);
    sanity_err(err);

exit_label:
    printf("testing finished at %s with %i error\n", time_str, err);
	safe_free((void**)&time_str);
    return err;
}

int do_test_cases()
{
    int err = ERR_NO_ERR;

    log_unit_t log_unit = {0};
    log_inst_t log_inst = {0};
    rotate_file_info_t rt_f_info = {0};

    rt_f_info.max_file_cnt = 3;
    rt_f_info.max_file_size = 10000;
    rt_f_info.rt_sufix = "_rt";

    log_inst.buffer_type = BUFF_TYPE_RING;
    log_inst.buff_size = 1;
    log_inst.file_path = TEST_FILE_NAME;
    log_inst.rt_file_opt = rt_f_info;
    log_inst.inst_id = "Default_log";

    log_inst.decorator_cb = log_decorate_str_dflt;

    log_unit.inst_arr = &log_inst;
    log_unit.inst_cnt = 1;

    err = log_unit_init(&log_unit);
    sanity_err(err);

    for(int i = 0; i < 10; i++)
    {
    	err = log_unit_write_str_fmt_log(&log_unit, DEBUG, "log # %i;\n", i);
        sanity_err(err);
        sleep(1);

    }

    err = log_unit_deinit(&log_unit);
    sanity_err(err);

exit_label:
    return err;
}
