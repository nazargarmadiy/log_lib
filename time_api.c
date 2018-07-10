#include "time_api.h"
#include "sanity.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DATE_TIME_STR_MAX_LEN 128
#define DATE_TIME_DEFAULT_FMT "[%d.%m.%Y %T]"

int get_date_time_str(bool_t       precise,
                      const char  *fmt,
                      char       **time_str)
{
    int         err = ERR_NO_ERR;
    int         str_len = 0;
    char       *tmp = NULL;
    time_tm_t  *date_time = NULL;
    timespec_t  tm_precise = {0};
    time_t      tm_now = {0};

    sanity_null_ptr(time_str);

    /*
     * default time format:
     * [01.01.2018 12:01:01] [01.123456789]
     *    date       time    precise time
     */
    err = time(&tm_now);
    sanity_require((time_t)(-1) != err);

    date_time = localtime(&tm_now);
    sanity_null_ptr(date_time);

    err = clock_gettime(CLOCK_REALTIME, &tm_precise);
    sanity_require(!err);

    tmp = *time_str = malloc(DATE_TIME_STR_MAX_LEN);
    sanity_null_ptr(tmp);

    str_len = strftime(tmp,
                       DATE_TIME_STR_MAX_LEN,
                       fmt ? fmt : DATE_TIME_DEFAULT_FMT,
                       date_time);
    sanity_require(str_len);

    sanity_require(str_len < DATE_TIME_STR_MAX_LEN);

    if(precise){
        tmp += str_len;
        str_len += snprintf(tmp,
                            DATE_TIME_STR_MAX_LEN,
                            " [%02li.%09li]",
                            (tm_precise.tv_sec) % 60,
                            tm_precise.tv_nsec);
        sanity_require(str_len < DATE_TIME_STR_MAX_LEN);
    }

exit_label:
    return err;
}

int append_date_time(bool_t       precise,
                     const char  *fmt,
                     const char  *log_str,
                     char       **out_str)
{
    int   err = ERR_NO_ERR;
    char *date_time_str = NULL;
    int   total_len = 0;
    int   writed = 0;

    sanity_null_ptr(log_str);
    sanity_null_ptr(out_str);

    err = get_date_time_str(precise, fmt, &date_time_str);
    sanity_err(err);
    sanity_null_ptr(date_time_str);

    total_len = strlen(log_str) + strlen(date_time_str) + 2;

    *out_str = calloc(1, total_len);
    sanity_null_ptr(*out_str);

    writed = snprintf(*out_str, total_len, "%s %s", date_time_str, log_str);
    sanity_require(writed < total_len);

exit_label:
    return err;
}
