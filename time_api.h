#ifndef TIME_API_H_
#define TIME_API_H_

#include "common_defs.h"

int get_date_time_str(bool_t       precise,
                      const char  *fmt,
                      char       **time_str);

int append_date_time(bool_t       precise,
                     const char  *fmt,
                     const char  *log_str,
                     char       **out_str);

#endif /* TIME_API_H_ */
