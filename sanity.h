#ifndef SANITY_H
#define SANITY_H

#include <errno.h>
#include "common_defs.h"

#define sanity_err(error)          \
    do{                            \
        if(ERR_NO_ERR != error){   \
            goto exit_label;       \
        }                          \
    }while(0);

#define sanity_null_ptr(pointer) \
    do{                          \
        if(!(pointer)){          \
            err = ERR_NULL_PTR;  \
            goto exit_label;     \
        }                        \
    }while(0);

#define sanity_require(condition) \
    do{                           \
        if(!(condition)){         \
            err = ERR_BAD_COND;   \
            goto exit_label;      \
        }                         \
    }while(0);

#define sanity_require_errno(condition) \
    do{                                 \
        if(!(condition)){               \
            err = errno;                \
            goto exit_label;            \
        }                               \
    }while(0);

#endif /*SANITY_H*/
