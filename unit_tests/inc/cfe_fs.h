/*
 * Standalone cFE File Services header for CF unit tests.
 */
#ifndef _cfe_fs_h_
#define _cfe_fs_h_

#include "common_types.h"

typedef struct {
    uint32 ContentType;
    uint32 SubType;
    uint32 Length;
    uint32 SpacecraftID;
    uint32 ProcessorID;
    uint32 ApplicationID;
    uint32 TimeSeconds;
    uint32 TimeSubSeconds;
    char   Description[32];
} CFE_FS_Header_t;

int32 CFE_FS_WriteHeader(int32 FileDes, CFE_FS_Header_t *Hdr);

#endif /* _cfe_fs_h_ */
