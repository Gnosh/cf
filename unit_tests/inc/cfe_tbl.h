/*
 * Standalone cFE Table Services header for CF unit tests.
 */
#ifndef _cfe_tbl_h_
#define _cfe_tbl_h_

#include "common_types.h"
#include "osconfig.h"

typedef int32 CFE_TBL_Handle_t;
typedef int32 (*CFE_TBL_CallbackFuncPtr_t)(void *TblPtr);

typedef enum {
    CFE_TBL_SRC_FILE = 0,
    CFE_TBL_SRC_ADDRESS = 1
} CFE_TBL_SrcEnum_t;

/* Table option flags */
#define CFE_TBL_OPT_DEFAULT     0x0000
#define CFE_TBL_OPT_SNGL_BUFFER 0x0000
#define CFE_TBL_OPT_DBL_BUFFER  0x0001
#define CFE_TBL_OPT_LOAD_DUMP   0x0002

typedef struct {
    uint32 Size;
    uint32 NumUsers;
    char   Name[OS_MAX_PATH_LEN];
    boolean DblBuffered;
    boolean Critical;
} CFE_TBL_Info_t;

/* Return codes specific to TBL */
#define CFE_TBL_INFO_UPDATED              0x4C000001
#define CFE_TBL_INFO_UPDATE_PENDING       0x4C000004
#define CFE_TBL_INFO_VALIDATION_PENDING   0x4C000005
#define CFE_TBL_ERR_INVALID_HANDLE        0xCC000003

int32 CFE_TBL_Register(CFE_TBL_Handle_t *TblHandlePtr, const char *Name,
                        uint32 Size, uint16 TblOptionFlags,
                        CFE_TBL_CallbackFuncPtr_t TblValidationFuncPtr);
int32 CFE_TBL_Load(CFE_TBL_Handle_t TblHandle, CFE_TBL_SrcEnum_t SrcType,
                    const void *SrcDataPtr);
int32 CFE_TBL_Manage(CFE_TBL_Handle_t TblHandle);
int32 CFE_TBL_GetAddress(void **TblPtr, CFE_TBL_Handle_t TblHandle);
int32 CFE_TBL_ReleaseAddress(CFE_TBL_Handle_t TblHandle);
int32 CFE_TBL_GetStatus(CFE_TBL_Handle_t TblHandle);
int32 CFE_TBL_GetInfo(CFE_TBL_Info_t *TblInfoPtr, const char *TblName);
int32 CFE_TBL_Unregister(CFE_TBL_Handle_t TblHandle);
int32 CFE_TBL_Validate(CFE_TBL_Handle_t TblHandle);
int32 CFE_TBL_Modified(CFE_TBL_Handle_t TblHandle);

#endif /* _cfe_tbl_h_ */
