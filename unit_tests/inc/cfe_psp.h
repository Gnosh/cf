/*
 * Standalone cFE PSP header for CF unit tests.
 */
#ifndef _cfe_psp_h_
#define _cfe_psp_h_

#include "common_types.h"

int32 CFE_PSP_MemCpy(void *dest, void *src, uint32 n);
int32 CFE_PSP_MemSet(void *dest, uint8 value, uint32 n);

#endif /* _cfe_psp_h_ */
