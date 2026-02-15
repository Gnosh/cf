/*
 * cfe_stubs.h
 *
 * Stub control header for CF application unit tests.
 * Provides a global control structure (CFE_Stubs) that allows tests to
 * set return values, inject messages, and inspect call counts for every
 * cFE / OSAL function used by the CF application.
 */
#ifndef _cfe_stubs_h_
#define _cfe_stubs_h_

#include "common_types.h"
#include "cfe_sb.h"
#include "cfe_es.h"
#include "cfe_evs.h"
#include "cfe_tbl.h"
#include "cfe_fs.h"
#include "cfe_time.h"
#include "cfe_psp.h"
#include "cfe_error.h"
#include "osapi.h"

/* ------------------------------------------------------------------ */
/* Constants                                                           */
/* ------------------------------------------------------------------ */
#define CFE_STUBS_MAX_DIRENTRY   32   /* max simulated directory entries  */
#define CFE_STUBS_POOL_BUF_SIZE  4096 /* static pool buffer size (bytes)  */
#define CFE_STUBS_MAX_POOL_ENTRIES 32 /* max independent pool allocations */

/* ------------------------------------------------------------------ */
/* Global stub-control structure                                       */
/* ------------------------------------------------------------------ */
typedef struct
{
    /* ===== cFE SB ================================================= */
    int32   CFE_SB_CreatePipe_Return;
    uint32  CFE_SB_CreatePipe_CallCount;

    int32   CFE_SB_Subscribe_Return;
    uint32  CFE_SB_Subscribe_CallCount;

    int32   CFE_SB_RcvMsg_Return;
    uint32  CFE_SB_RcvMsg_CallCount;
    CFE_SB_MsgPtr_t RcvMsg_MsgPtr;       /* message pointer returned to caller */

    int32   CFE_SB_SendMsg_Return;
    uint32  CFE_SB_SendMsg_CallCount;
    CFE_SB_MsgPtr_t SendMsg_LastMsgPtr;   /* pointer to last message sent */

    uint32  CFE_SB_InitMsg_CallCount;

    uint16  GetMsgId_Return;              /* value returned by GetMsgId stub   */
    uint32  CFE_SB_GetMsgId_CallCount;
    boolean GetMsgId_UseReturnOverride;   /* if TRUE use GetMsgId_Return, else
                                             read from CCSDS header            */

    uint32  CFE_SB_SetMsgId_CallCount;

    uint16  GetCmdCode_Return;
    uint32  CFE_SB_GetCmdCode_CallCount;
    boolean GetCmdCode_UseReturnOverride;

    uint16  GetTotalMsgLength_Return;
    uint32  CFE_SB_GetTotalMsgLength_CallCount;
    boolean GetTotalMsgLength_UseReturnOverride;

    uint32  CFE_SB_SetTotalMsgLength_CallCount;
    uint32  CFE_SB_GetUserDataLength_CallCount;
    uint32  CFE_SB_SetUserDataLength_CallCount;
    uint32  CFE_SB_GetUserData_CallCount;
    uint32  CFE_SB_MsgHdrSize_CallCount;
    uint32  CFE_SB_TimeStampMsg_CallCount;
    uint32  CFE_SB_GenerateChecksum_CallCount;
    uint32  CFE_SB_ValidateChecksum_CallCount;
    boolean ValidateChecksum_Return;
    uint32  CFE_SB_GetChecksum_CallCount;
    uint16  GetChecksum_Return;

    int32   CFE_SB_SetCmdCode_Return;
    uint32  CFE_SB_SetCmdCode_CallCount;

    /* Zero-copy */
    int32   CFE_SB_ZeroCopyGetPtr_Return;
    uint32  CFE_SB_ZeroCopyGetPtr_CallCount;
    int32   CFE_SB_ZeroCopyReleasePtr_Return;
    uint32  CFE_SB_ZeroCopyReleasePtr_CallCount;
    int32   CFE_SB_ZeroCopySend_Return;
    uint32  CFE_SB_ZeroCopySend_CallCount;

    /* ===== cFE EVS ================================================ */
    int32   CFE_EVS_Register_Return;
    uint32  CFE_EVS_Register_CallCount;

    int32   CFE_EVS_SendEvent_Return;
    uint32  CFE_EVS_SendEvent_CallCount;
    uint16  EVS_SendEvent_LastEventID;    /* last EventID for test verification */
    uint16  EVS_SendEvent_LastEventType;

    /* ===== cFE ES ================================================= */
    int32   CFE_ES_RegisterApp_Return;
    uint32  CFE_ES_RegisterApp_CallCount;

    uint32  CFE_ES_RunLoop_Count;         /* how many times TRUE returned so far */
    uint32  CFE_ES_RunLoop_MaxCount;      /* return FALSE after this many calls  */
    uint32  CFE_ES_RunLoop_CallCount;

    uint32  CFE_ES_ExitApp_CallCount;
    uint32  CFE_ES_ExitApp_LastExitStatus;

    uint32  CFE_ES_WaitForStartupSync_CallCount;

    int32   CFE_ES_GetAppID_Return;
    uint32  CFE_ES_GetAppID_CallCount;

    int32   CFE_ES_WriteToSysLog_Return;
    uint32  CFE_ES_WriteToSysLog_CallCount;

    uint32  CFE_ES_PerfLogAdd_CallCount;

    int32   CFE_ES_PoolCreate_Return;
    uint32  CFE_ES_PoolCreate_CallCount;

    int32   CFE_ES_PoolCreateEx_Return;
    uint32  CFE_ES_PoolCreateEx_CallCount;

    int32   CFE_ES_GetPoolBuf_Return;
    uint32  CFE_ES_GetPoolBuf_CallCount;

    int32   CFE_ES_PutPoolBuf_Return;
    uint32  CFE_ES_PutPoolBuf_CallCount;

    int32   CFE_ES_GetPoolBufInfo_Return;
    uint32  CFE_ES_GetPoolBufInfo_CallCount;

    int32   CFE_ES_GetMemPoolStats_Return;
    uint32  CFE_ES_GetMemPoolStats_CallCount;

    /* ===== cFE TBL ================================================ */
    int32   CFE_TBL_Register_Return;
    uint32  CFE_TBL_Register_CallCount;

    int32   CFE_TBL_Load_Return;
    uint32  CFE_TBL_Load_CallCount;

    int32   CFE_TBL_Manage_Return;
    uint32  CFE_TBL_Manage_CallCount;

    int32   CFE_TBL_GetAddress_Return;
    uint32  CFE_TBL_GetAddress_CallCount;
    void   *TBL_GetAddress_Ptr;           /* pointer returned via *TblPtr */

    int32   CFE_TBL_ReleaseAddress_Return;
    uint32  CFE_TBL_ReleaseAddress_CallCount;

    int32   CFE_TBL_GetStatus_Return;
    uint32  CFE_TBL_GetStatus_CallCount;

    int32   CFE_TBL_GetInfo_Return;
    uint32  CFE_TBL_GetInfo_CallCount;

    int32   CFE_TBL_Validate_Return;
    uint32  CFE_TBL_Validate_CallCount;

    int32   CFE_TBL_Modified_Return;
    uint32  CFE_TBL_Modified_CallCount;

    /* ===== cFE FS ================================================= */
    int32   CFE_FS_WriteHeader_Return;
    uint32  CFE_FS_WriteHeader_CallCount;

    /* ===== cFE TIME =============================================== */
    CFE_TIME_SysTime_t CFE_TIME_GetTime_Return;
    uint32  CFE_TIME_GetTime_CallCount;

    /* ===== cFE PSP ================================================ */
    int32   CFE_PSP_MemCpy_Return;
    uint32  CFE_PSP_MemCpy_CallCount;

    int32   CFE_PSP_MemSet_Return;
    uint32  CFE_PSP_MemSet_CallCount;

    /* ===== OSAL file / directory ================================== */
    int32   OS_opendir_Return;
    uint32  OS_opendir_CallCount;

    int32   OS_closedir_Return;
    uint32  OS_closedir_CallCount;

    uint32  OS_readdir_CallCount;
    uint32  OS_readdir_Index;             /* current index into DirEntries      */
    uint32  OS_readdir_NumEntries;        /* number of entries populated         */
    os_dirent_t OS_readdir_Entries[CFE_STUBS_MAX_DIRENTRY]; /* simulated dir    */

    int32   OS_stat_Return;
    uint32  OS_stat_CallCount;
    os_fstat_t OS_stat_StatBuf;           /* value filled in by OS_stat stub    */

    int32   OS_remove_Return;
    uint32  OS_remove_CallCount;

    int32   OS_rename_Return;
    uint32  OS_rename_CallCount;

    int32   OS_mv_Return;
    uint32  OS_mv_CallCount;

    int32   OS_FDGetInfo_Return;
    uint32  OS_FDGetInfo_CallCount;
    OS_FDTableEntry OS_FDGetInfo_Entry;   /* value filled in by stub           */

    int32   OS_CountSemGetIdByName_Return;
    uint32  OS_CountSemGetIdByName_CallCount;

    int32   OS_CountSemGive_Return;
    uint32  OS_CountSemGive_CallCount;

    int32   OS_CountSemTake_Return;
    uint32  OS_CountSemTake_CallCount;

    int32   OS_CountSemGetInfo_Return;
    uint32  OS_CountSemGetInfo_CallCount;
    uint32  OS_CountSemGetInfo_SemValue;

    int32   OS_CountSemTimedWait_Return;
    uint32  OS_CountSemTimedWait_CallCount;

    int32   OS_lseek_Return;
    uint32  OS_lseek_CallCount;

    uint32  OS_printf_CallCount;

    int32   OS_creat_Return;
    uint32  OS_creat_CallCount;

    int32   OS_open_Return;
    uint32  OS_open_CallCount;

    int32   OS_close_Return;
    uint32  OS_close_CallCount;

    int32   OS_write_Return;
    uint32  OS_write_CallCount;

    int32   OS_read_Return;
    uint32  OS_read_CallCount;

    /* ===== Shared internal buffers ================================ */
    /* Pool allocator: returns a unique buffer on each call to avoid
     * circular linked lists when CF_AllocQueueEntry is called multiple times */
    uint8  PoolBufArray[CFE_STUBS_MAX_POOL_ENTRIES][CFE_STUBS_POOL_BUF_SIZE];
    uint32 PoolBufNextIndex;             /* next available pool buf slot     */
    uint8  PoolBuf[CFE_STUBS_POOL_BUF_SIZE]; /* legacy single buffer        */
    uint8  ZeroCopyBuf[CFE_SB_MAX_SB_MSG_SIZE]; /* buffer for ZeroCopyGetPtr  */

} CFE_Stubs_t;

/* ------------------------------------------------------------------ */
/* The single global instance -- defined in cfe_stubs.c                */
/* ------------------------------------------------------------------ */
extern CFE_Stubs_t CFE_Stubs;

/* ------------------------------------------------------------------ */
/* Reset all fields to zero / sensible defaults                        */
/* ------------------------------------------------------------------ */
void CFE_Stubs_Reset(void);

#endif /* _cfe_stubs_h_ */
