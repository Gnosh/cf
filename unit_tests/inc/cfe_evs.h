/*
 * Standalone cFE Event Services header for CF unit tests.
 */
#ifndef _cfe_evs_h_
#define _cfe_evs_h_

#include "common_types.h"

/* Event types */
#define CFE_EVS_DEBUG       1
#define CFE_EVS_INFORMATION 2
#define CFE_EVS_ERROR       3
#define CFE_EVS_CRITICAL    4

/* Event filter schemes */
#define CFE_EVS_BINARY_FILTER 0

/* Filter masks */
#define CFE_EVS_NO_FILTER       0x0000
#define CFE_EVS_FIRST_ONE_STOP  0xFFFF
#define CFE_EVS_FIRST_TWO_STOP  0xFFFE

/* Max message length */
#define CFE_EVS_MAX_MESSAGE_LENGTH 122

typedef struct {
    uint16 EventID;
    uint16 Mask;
} CFE_EVS_BinFilter_t;

int32 CFE_EVS_Register(void *Filters, uint16 NumEventFilters, uint16 FilterScheme);
int32 CFE_EVS_SendEvent(uint16 EventID, uint16 EventType, const char *Spec, ...);
int32 CFE_EVS_SendTimedEvent(void *Time, uint16 EventID, uint16 EventType, const char *Spec, ...);

#endif /* _cfe_evs_h_ */
