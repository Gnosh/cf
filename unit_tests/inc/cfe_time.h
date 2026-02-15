/*
 * Standalone cFE Time Services header for CF unit tests.
 */
#ifndef _cfe_time_h_
#define _cfe_time_h_

#include "common_types.h"

typedef struct {
    uint32 Seconds;
    uint32 Subseconds;
} CFE_TIME_SysTime_t;

CFE_TIME_SysTime_t CFE_TIME_GetTime(void);

#endif /* _cfe_time_h_ */
