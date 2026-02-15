/*
 * cfe_stubs.c
 *
 * Stub implementations of all cFE and OSAL functions used by the CF
 * application.  Each stub increments a call counter and returns a
 * configurable value from the global CFE_Stubs control structure.
 *
 * Where practical the stubs perform real work (e.g. CCSDS header
 * manipulation) so that the CF message-routing logic exercises real
 * header bytes during unit tests.
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "cfe.h"
#include "cfe_stubs.h"

/* ================================================================== */
/* Global instance                                                     */
/* ================================================================== */
CFE_Stubs_t CFE_Stubs;

/* ================================================================== */
/* Reset helper                                                        */
/* ================================================================== */
void CFE_Stubs_Reset(void)
{
    memset(&CFE_Stubs, 0, sizeof(CFE_Stubs));

    /* Sensible defaults so callers that forget to set a return value
     * get CFE_SUCCESS / OS_SUCCESS instead of garbage.                */
    CFE_Stubs.CFE_SB_CreatePipe_Return        = CFE_SUCCESS;
    CFE_Stubs.CFE_SB_Subscribe_Return          = CFE_SUCCESS;
    CFE_Stubs.CFE_SB_RcvMsg_Return             = CFE_SUCCESS;
    CFE_Stubs.CFE_SB_SendMsg_Return            = CFE_SUCCESS;
    CFE_Stubs.CFE_SB_SetCmdCode_Return         = CFE_SUCCESS;
    CFE_Stubs.CFE_SB_ZeroCopyReleasePtr_Return = CFE_SUCCESS;
    CFE_Stubs.CFE_SB_ZeroCopySend_Return       = CFE_SUCCESS;
    CFE_Stubs.ValidateChecksum_Return           = TRUE;

    CFE_Stubs.CFE_EVS_Register_Return          = CFE_SUCCESS;
    CFE_Stubs.CFE_EVS_SendEvent_Return         = CFE_SUCCESS;

    CFE_Stubs.CFE_ES_RegisterApp_Return        = CFE_SUCCESS;
    CFE_Stubs.CFE_ES_RunLoop_MaxCount          = 1; /* single iteration default */
    CFE_Stubs.CFE_ES_GetAppID_Return           = CFE_SUCCESS;
    CFE_Stubs.CFE_ES_WriteToSysLog_Return      = CFE_SUCCESS;
    CFE_Stubs.CFE_ES_PoolCreate_Return         = CFE_SUCCESS;
    CFE_Stubs.CFE_ES_PoolCreateEx_Return       = CFE_SUCCESS;
    CFE_Stubs.CFE_ES_GetPoolBuf_Return         = CFE_STUBS_POOL_BUF_SIZE;
    CFE_Stubs.CFE_ES_PutPoolBuf_Return         = CFE_SUCCESS;
    CFE_Stubs.CFE_ES_GetPoolBufInfo_Return     = CFE_STUBS_POOL_BUF_SIZE;
    CFE_Stubs.CFE_ES_GetMemPoolStats_Return    = CFE_SUCCESS;

    CFE_Stubs.CFE_TBL_Register_Return          = CFE_SUCCESS;
    CFE_Stubs.CFE_TBL_Load_Return              = CFE_SUCCESS;
    CFE_Stubs.CFE_TBL_Manage_Return            = CFE_SUCCESS;
    CFE_Stubs.CFE_TBL_GetAddress_Return        = CFE_SUCCESS;
    CFE_Stubs.CFE_TBL_ReleaseAddress_Return    = CFE_SUCCESS;
    CFE_Stubs.CFE_TBL_GetStatus_Return         = CFE_SUCCESS;
    CFE_Stubs.CFE_TBL_GetInfo_Return           = CFE_SUCCESS;

    CFE_Stubs.CFE_FS_WriteHeader_Return        = (int32)sizeof(CFE_FS_Header_t);

    CFE_Stubs.CFE_PSP_MemCpy_Return            = CFE_SUCCESS;
    CFE_Stubs.CFE_PSP_MemSet_Return            = CFE_SUCCESS;

    CFE_Stubs.OS_opendir_Return                = 1; /* valid dir handle */
    CFE_Stubs.OS_closedir_Return               = OS_SUCCESS;
    CFE_Stubs.OS_stat_Return                   = OS_SUCCESS;
    CFE_Stubs.OS_remove_Return                 = OS_SUCCESS;
    CFE_Stubs.OS_rename_Return                 = OS_SUCCESS;
    CFE_Stubs.OS_mv_Return                     = OS_SUCCESS;
    CFE_Stubs.OS_FDGetInfo_Return              = OS_SUCCESS;
    CFE_Stubs.OS_CountSemGetIdByName_Return    = OS_SUCCESS;
    CFE_Stubs.OS_CountSemGive_Return           = OS_SUCCESS;
    CFE_Stubs.OS_CountSemTake_Return           = OS_SUCCESS;
    CFE_Stubs.OS_CountSemGetInfo_Return        = OS_SUCCESS;
    CFE_Stubs.OS_creat_Return                  = 5; /* valid fd */
    CFE_Stubs.OS_open_Return                   = 5;
    CFE_Stubs.OS_close_Return                  = OS_SUCCESS;
    CFE_Stubs.OS_write_Return                  = 0;
    CFE_Stubs.OS_read_Return                   = 0;
}

/* ================================================================== */
/* cFE SB stubs                                                        */
/* ================================================================== */

int32 CFE_SB_CreatePipe(CFE_SB_PipeId_t *PipeIdPtr, uint16 Depth,
                         char *PipeName)
{
    (void)Depth;
    (void)PipeName;
    CFE_Stubs.CFE_SB_CreatePipe_CallCount++;
    if (PipeIdPtr != NULL)
    {
        *PipeIdPtr = 0;
    }
    return CFE_Stubs.CFE_SB_CreatePipe_Return;
}

int32 CFE_SB_Subscribe(CFE_SB_MsgId_t MsgId, CFE_SB_PipeId_t PipeId)
{
    (void)MsgId;
    (void)PipeId;
    CFE_Stubs.CFE_SB_Subscribe_CallCount++;
    return CFE_Stubs.CFE_SB_Subscribe_Return;
}

int32 CFE_SB_RcvMsg(CFE_SB_MsgPtr_t *BufPtr, CFE_SB_PipeId_t PipeId,
                     int32 TimeOut)
{
    (void)PipeId;
    (void)TimeOut;
    CFE_Stubs.CFE_SB_RcvMsg_CallCount++;
    if (BufPtr != NULL)
    {
        *BufPtr = CFE_Stubs.RcvMsg_MsgPtr;
    }
    return CFE_Stubs.CFE_SB_RcvMsg_Return;
}

int32 CFE_SB_SendMsg(CFE_SB_Msg_t *MsgPtr)
{
    CFE_Stubs.CFE_SB_SendMsg_CallCount++;
    CFE_Stubs.SendMsg_LastMsgPtr = MsgPtr;
    return CFE_Stubs.CFE_SB_SendMsg_Return;
}

/*
 * CFE_SB_InitMsg -- functional stub.
 * Zeroes the buffer (if Clear == TRUE) and writes the MsgId into the
 * first two bytes in big-endian (standard CCSDS primary header).
 * Also writes the total length into bytes 4-5 as CCSDS packet-length
 * field (total_length - 7).
 */
void CFE_SB_InitMsg(void *MsgPtr, CFE_SB_MsgId_t MsgId, uint16 Length,
                     boolean Clear)
{
    uint8 *p = (uint8 *)MsgPtr;

    CFE_Stubs.CFE_SB_InitMsg_CallCount++;

    if (Clear == TRUE)
    {
        memset(MsgPtr, 0, Length);
    }

    /* Bytes 0-1: Stream ID (MsgId) -- big-endian */
    p[0] = (uint8)((MsgId >> 8) & 0xFF);
    p[1] = (uint8)(MsgId & 0xFF);

    /* Bytes 4-5: Packet length -- store (Length - 7) per CCSDS */
    {
        uint16 pktLen = (Length > 7) ? (uint16)(Length - 7) : 0;
        p[4] = (uint8)((pktLen >> 8) & 0xFF);
        p[5] = (uint8)(pktLen & 0xFF);
    }
}

/*
 * CFE_SB_GetMsgId -- reads the CCSDS Stream ID from bytes 0-1 (big-endian)
 * unless the override flag is set.
 */
CFE_SB_MsgId_t CFE_SB_GetMsgId(CFE_SB_MsgPtr_t MsgPtr)
{
    CFE_Stubs.CFE_SB_GetMsgId_CallCount++;

    if (CFE_Stubs.GetMsgId_UseReturnOverride)
    {
        return (CFE_SB_MsgId_t)CFE_Stubs.GetMsgId_Return;
    }

    {
        uint8 *p = (uint8 *)MsgPtr;
        return (CFE_SB_MsgId_t)(((uint16)p[0] << 8) | (uint16)p[1]);
    }
}

void CFE_SB_SetMsgId(CFE_SB_MsgPtr_t MsgPtr, CFE_SB_MsgId_t MsgId)
{
    uint8 *p = (uint8 *)MsgPtr;

    CFE_Stubs.CFE_SB_SetMsgId_CallCount++;

    p[0] = (uint8)((MsgId >> 8) & 0xFF);
    p[1] = (uint8)(MsgId & 0xFF);
}

/*
 * CFE_SB_GetCmdCode -- reads command function code from CCSDS command
 * secondary header byte offset 6-7.  The function code occupies the
 * upper 7 bits of byte 6 (bits [6:0]).
 */
uint16 CFE_SB_GetCmdCode(CFE_SB_MsgPtr_t MsgPtr)
{
    CFE_Stubs.CFE_SB_GetCmdCode_CallCount++;

    if (CFE_Stubs.GetCmdCode_UseReturnOverride)
    {
        return CFE_Stubs.GetCmdCode_Return;
    }

    {
        uint8 *p = (uint8 *)MsgPtr;
        return (uint16)(p[6] & 0x7F);
    }
}

int32 CFE_SB_SetCmdCode(CFE_SB_MsgPtr_t MsgPtr, uint16 CmdCode)
{
    uint8 *p = (uint8 *)MsgPtr;

    CFE_Stubs.CFE_SB_SetCmdCode_CallCount++;

    /* Function code goes in bits [6:0] of byte 6.
     * Preserve the checksum-present flag in bit 7.  */
    p[6] = (uint8)((p[6] & 0x80) | (CmdCode & 0x7F));

    return CFE_Stubs.CFE_SB_SetCmdCode_Return;
}

/*
 * CFE_SB_GetTotalMsgLength -- reads packet length from bytes 4-5
 * and converts back to total message length (pktLen + 7).
 */
uint16 CFE_SB_GetTotalMsgLength(CFE_SB_MsgPtr_t MsgPtr)
{
    CFE_Stubs.CFE_SB_GetTotalMsgLength_CallCount++;

    if (CFE_Stubs.GetTotalMsgLength_UseReturnOverride)
    {
        return CFE_Stubs.GetTotalMsgLength_Return;
    }

    {
        uint8  *p = (uint8 *)MsgPtr;
        uint16 pktLen = (uint16)(((uint16)p[4] << 8) | (uint16)p[5]);
        return (uint16)(pktLen + 7);
    }
}

void CFE_SB_SetTotalMsgLength(CFE_SB_MsgPtr_t MsgPtr, uint16 TotalLength)
{
    uint8 *p = (uint8 *)MsgPtr;
    uint16 pktLen;

    CFE_Stubs.CFE_SB_SetTotalMsgLength_CallCount++;

    pktLen = (TotalLength > 7) ? (uint16)(TotalLength - 7) : 0;
    p[4] = (uint8)((pktLen >> 8) & 0xFF);
    p[5] = (uint8)(pktLen & 0xFF);
}

uint16 CFE_SB_GetUserDataLength(CFE_SB_MsgPtr_t MsgPtr)
{
    uint16 totalLen;
    uint16 hdrSize;
    CFE_SB_MsgId_t msgId;

    CFE_Stubs.CFE_SB_GetUserDataLength_CallCount++;

    totalLen = CFE_SB_GetTotalMsgLength(MsgPtr);
    msgId    = CFE_SB_GetMsgId(MsgPtr);
    hdrSize  = CFE_SB_MsgHdrSize(msgId);

    return (totalLen > hdrSize) ? (uint16)(totalLen - hdrSize) : 0;
}

void CFE_SB_SetUserDataLength(CFE_SB_MsgPtr_t MsgPtr, uint16 DataLength)
{
    uint16 hdrSize;
    CFE_SB_MsgId_t msgId;

    CFE_Stubs.CFE_SB_SetUserDataLength_CallCount++;

    msgId   = CFE_SB_GetMsgId(MsgPtr);
    hdrSize = CFE_SB_MsgHdrSize(msgId);

    CFE_SB_SetTotalMsgLength(MsgPtr, (uint16)(hdrSize + DataLength));
}

/*
 * CFE_SB_GetUserData -- returns pointer past the header.
 * Command messages (bit 12 of MsgId set) use CMD_HDR_SIZE,
 * telemetry messages use TLM_HDR_SIZE.
 */
void *CFE_SB_GetUserData(CFE_SB_MsgPtr_t MsgPtr)
{
    uint8 *p = (uint8 *)MsgPtr;
    CFE_SB_MsgId_t msgId;
    uint16 hdrSize;

    CFE_Stubs.CFE_SB_GetUserData_CallCount++;

    msgId   = CFE_SB_GetMsgId(MsgPtr);
    hdrSize = CFE_SB_MsgHdrSize(msgId);

    return (void *)(p + hdrSize);
}

/*
 * CFE_SB_MsgHdrSize -- command (bit 12 set) => CMD_HDR_SIZE,
 *                       telemetry => TLM_HDR_SIZE.
 */
uint16 CFE_SB_MsgHdrSize(CFE_SB_MsgId_t MsgId)
{
    CFE_Stubs.CFE_SB_MsgHdrSize_CallCount++;

    if ((MsgId & 0x1000) != 0)
    {
        return CFE_SB_CMD_HDR_SIZE;
    }
    return CFE_SB_TLM_HDR_SIZE;
}

void CFE_SB_TimeStampMsg(CFE_SB_MsgPtr_t MsgPtr)
{
    (void)MsgPtr;
    CFE_Stubs.CFE_SB_TimeStampMsg_CallCount++;
}

void CFE_SB_GenerateChecksum(CFE_SB_MsgPtr_t MsgPtr)
{
    (void)MsgPtr;
    CFE_Stubs.CFE_SB_GenerateChecksum_CallCount++;
}

boolean CFE_SB_ValidateChecksum(CFE_SB_MsgPtr_t MsgPtr)
{
    (void)MsgPtr;
    CFE_Stubs.CFE_SB_ValidateChecksum_CallCount++;
    return CFE_Stubs.ValidateChecksum_Return;
}

uint16 CFE_SB_GetChecksum(CFE_SB_MsgPtr_t MsgPtr)
{
    (void)MsgPtr;
    CFE_Stubs.CFE_SB_GetChecksum_CallCount++;
    return CFE_Stubs.GetChecksum_Return;
}

/* --- Zero-copy ---------------------------------------------------- */

CFE_SB_Msg_t *CFE_SB_ZeroCopyGetPtr(uint16 MsgSize,
                                      CFE_SB_ZeroCopyHandle_t *BufferHandle)
{
    (void)MsgSize;
    CFE_Stubs.CFE_SB_ZeroCopyGetPtr_CallCount++;
    if (BufferHandle != NULL)
    {
        *BufferHandle = 1;
    }
    /* Return pointer into the static zero-copy buffer */
    memset(CFE_Stubs.ZeroCopyBuf, 0, sizeof(CFE_Stubs.ZeroCopyBuf));
    return (CFE_SB_Msg_t *)CFE_Stubs.ZeroCopyBuf;
}

int32 CFE_SB_ZeroCopyReleasePtr(CFE_SB_Msg_t *Ptr2Release,
                                  CFE_SB_ZeroCopyHandle_t BufferHandle)
{
    (void)Ptr2Release;
    (void)BufferHandle;
    CFE_Stubs.CFE_SB_ZeroCopyReleasePtr_CallCount++;
    return CFE_Stubs.CFE_SB_ZeroCopyReleasePtr_Return;
}

int32 CFE_SB_ZeroCopySend(CFE_SB_Msg_t *MsgPtr,
                            CFE_SB_ZeroCopyHandle_t BufferHandle)
{
    (void)MsgPtr;
    (void)BufferHandle;
    CFE_Stubs.CFE_SB_ZeroCopySend_CallCount++;
    return CFE_Stubs.CFE_SB_ZeroCopySend_Return;
}

/* ================================================================== */
/* cFE EVS stubs                                                       */
/* ================================================================== */

int32 CFE_EVS_Register(void *Filters, uint16 NumEventFilters,
                        uint16 FilterScheme)
{
    (void)Filters;
    (void)NumEventFilters;
    (void)FilterScheme;
    CFE_Stubs.CFE_EVS_Register_CallCount++;
    return CFE_Stubs.CFE_EVS_Register_Return;
}

int32 CFE_EVS_SendEvent(uint16 EventID, uint16 EventType,
                          const char *Spec, ...)
{
    (void)Spec;
    CFE_Stubs.CFE_EVS_SendEvent_CallCount++;
    CFE_Stubs.EVS_SendEvent_LastEventID   = EventID;
    CFE_Stubs.EVS_SendEvent_LastEventType = EventType;
    return CFE_Stubs.CFE_EVS_SendEvent_Return;
}

/* ================================================================== */
/* cFE ES stubs                                                        */
/* ================================================================== */

int32 CFE_ES_RegisterApp(void)
{
    CFE_Stubs.CFE_ES_RegisterApp_CallCount++;
    return CFE_Stubs.CFE_ES_RegisterApp_Return;
}

int32 CFE_ES_RunLoop(uint32 *ExitStatus)
{
    (void)ExitStatus;
    CFE_Stubs.CFE_ES_RunLoop_CallCount++;
    CFE_Stubs.CFE_ES_RunLoop_Count++;

    if (CFE_Stubs.CFE_ES_RunLoop_Count > CFE_Stubs.CFE_ES_RunLoop_MaxCount)
    {
        return FALSE;
    }
    return TRUE;
}

void CFE_ES_ExitApp(uint32 ExitStatus)
{
    CFE_Stubs.CFE_ES_ExitApp_CallCount++;
    CFE_Stubs.CFE_ES_ExitApp_LastExitStatus = ExitStatus;
}

void CFE_ES_WaitForStartupSync(uint32 TimeOutMilliseconds)
{
    (void)TimeOutMilliseconds;
    CFE_Stubs.CFE_ES_WaitForStartupSync_CallCount++;
}

int32 CFE_ES_GetAppID(uint32 *AppIdPtr)
{
    CFE_Stubs.CFE_ES_GetAppID_CallCount++;
    if (AppIdPtr != NULL)
    {
        *AppIdPtr = 1;
    }
    return CFE_Stubs.CFE_ES_GetAppID_Return;
}

int32 CFE_ES_WriteToSysLog(const char *SpecStringPtr, ...)
{
    (void)SpecStringPtr;
    CFE_Stubs.CFE_ES_WriteToSysLog_CallCount++;
    return CFE_Stubs.CFE_ES_WriteToSysLog_Return;
}

void CFE_ES_PerfLogAdd(uint32 Marker, uint32 EntryExit)
{
    (void)Marker;
    (void)EntryExit;
    CFE_Stubs.CFE_ES_PerfLogAdd_CallCount++;
}

int32 CFE_ES_PoolCreate(uint32 *HandlePtr, uint8 *MemPtr, uint32 Size)
{
    (void)MemPtr;
    (void)Size;
    CFE_Stubs.CFE_ES_PoolCreate_CallCount++;
    if (HandlePtr != NULL)
    {
        *HandlePtr = 0x12345678;
    }
    return CFE_Stubs.CFE_ES_PoolCreate_Return;
}

int32 CFE_ES_PoolCreateEx(uint32 *HandlePtr, uint8 *MemPtr, uint32 Size,
                            uint32 NumBlockSizes, uint32 *BlockSizes,
                            uint16 UseMutex)
{
    (void)MemPtr;
    (void)Size;
    (void)NumBlockSizes;
    (void)BlockSizes;
    (void)UseMutex;
    CFE_Stubs.CFE_ES_PoolCreateEx_CallCount++;
    if (HandlePtr != NULL)
    {
        *HandlePtr = 0x12345678;
    }
    return CFE_Stubs.CFE_ES_PoolCreateEx_Return;
}

int32 CFE_ES_GetPoolBuf(uint32 **BufPtr, CFE_ES_MemHandle_t HandlePtr,
                          uint32 Size)
{
    (void)HandlePtr;
    (void)Size;
    CFE_Stubs.CFE_ES_GetPoolBuf_CallCount++;
    if (BufPtr != NULL)
    {
        /* Return a unique buffer each call to prevent circular linked lists
         * when CF_AllocQueueEntry is called multiple times. Falls back to
         * single PoolBuf if array is exhausted. */
        if (CFE_Stubs.PoolBufNextIndex < CFE_STUBS_MAX_POOL_ENTRIES)
        {
            memset(CFE_Stubs.PoolBufArray[CFE_Stubs.PoolBufNextIndex], 0,
                   CFE_STUBS_POOL_BUF_SIZE);
            *BufPtr = (uint32 *)CFE_Stubs.PoolBufArray[CFE_Stubs.PoolBufNextIndex];
            CFE_Stubs.PoolBufNextIndex++;
        }
        else
        {
            *BufPtr = (uint32 *)CFE_Stubs.PoolBuf;
        }
    }
    return CFE_Stubs.CFE_ES_GetPoolBuf_Return;
}

int32 CFE_ES_PutPoolBuf(CFE_ES_MemHandle_t HandlePtr, uint32 *BufPtr)
{
    (void)HandlePtr;
    (void)BufPtr;
    CFE_Stubs.CFE_ES_PutPoolBuf_CallCount++;
    return CFE_Stubs.CFE_ES_PutPoolBuf_Return;
}

int32 CFE_ES_GetPoolBufInfo(CFE_ES_MemHandle_t HandlePtr, uint32 *BufPtr)
{
    (void)HandlePtr;
    (void)BufPtr;
    CFE_Stubs.CFE_ES_GetPoolBufInfo_CallCount++;
    return CFE_Stubs.CFE_ES_GetPoolBufInfo_Return;
}

int32 CFE_ES_GetMemPoolStats(CFE_ES_MemPoolStats_t *BufPtr,
                               CFE_ES_MemHandle_t Handle)
{
    (void)Handle;
    CFE_Stubs.CFE_ES_GetMemPoolStats_CallCount++;
    if (BufPtr != NULL)
    {
        memset(BufPtr, 0, sizeof(*BufPtr));
    }
    return CFE_Stubs.CFE_ES_GetMemPoolStats_Return;
}

/* ================================================================== */
/* cFE TBL stubs                                                       */
/* ================================================================== */

int32 CFE_TBL_Register(CFE_TBL_Handle_t *TblHandlePtr, const char *Name,
                         uint32 Size, uint16 TblOptionFlags,
                         CFE_TBL_CallbackFuncPtr_t TblValidationFuncPtr)
{
    (void)Name;
    (void)Size;
    (void)TblOptionFlags;
    (void)TblValidationFuncPtr;
    CFE_Stubs.CFE_TBL_Register_CallCount++;
    if (TblHandlePtr != NULL)
    {
        *TblHandlePtr = 0;
    }
    return CFE_Stubs.CFE_TBL_Register_Return;
}

int32 CFE_TBL_Load(CFE_TBL_Handle_t TblHandle, CFE_TBL_SrcEnum_t SrcType,
                     const void *SrcDataPtr)
{
    (void)TblHandle;
    (void)SrcType;
    (void)SrcDataPtr;
    CFE_Stubs.CFE_TBL_Load_CallCount++;
    return CFE_Stubs.CFE_TBL_Load_Return;
}

int32 CFE_TBL_Manage(CFE_TBL_Handle_t TblHandle)
{
    (void)TblHandle;
    CFE_Stubs.CFE_TBL_Manage_CallCount++;
    return CFE_Stubs.CFE_TBL_Manage_Return;
}

int32 CFE_TBL_GetAddress(void **TblPtr, CFE_TBL_Handle_t TblHandle)
{
    (void)TblHandle;
    CFE_Stubs.CFE_TBL_GetAddress_CallCount++;
    if (TblPtr != NULL)
    {
        *TblPtr = CFE_Stubs.TBL_GetAddress_Ptr;
    }
    return CFE_Stubs.CFE_TBL_GetAddress_Return;
}

int32 CFE_TBL_ReleaseAddress(CFE_TBL_Handle_t TblHandle)
{
    (void)TblHandle;
    CFE_Stubs.CFE_TBL_ReleaseAddress_CallCount++;
    return CFE_Stubs.CFE_TBL_ReleaseAddress_Return;
}

int32 CFE_TBL_GetStatus(CFE_TBL_Handle_t TblHandle)
{
    (void)TblHandle;
    CFE_Stubs.CFE_TBL_GetStatus_CallCount++;
    return CFE_Stubs.CFE_TBL_GetStatus_Return;
}

int32 CFE_TBL_GetInfo(CFE_TBL_Info_t *TblInfoPtr, const char *TblName)
{
    (void)TblName;
    CFE_Stubs.CFE_TBL_GetInfo_CallCount++;
    if (TblInfoPtr != NULL)
    {
        memset(TblInfoPtr, 0, sizeof(*TblInfoPtr));
    }
    return CFE_Stubs.CFE_TBL_GetInfo_Return;
}

int32 CFE_TBL_Validate(CFE_TBL_Handle_t TblHandle)
{
    (void)TblHandle;
    CFE_Stubs.CFE_TBL_Validate_CallCount++;
    return CFE_Stubs.CFE_TBL_Validate_Return;
}

int32 CFE_TBL_Modified(CFE_TBL_Handle_t TblHandle)
{
    (void)TblHandle;
    CFE_Stubs.CFE_TBL_Modified_CallCount++;
    return CFE_Stubs.CFE_TBL_Modified_Return;
}

/* ================================================================== */
/* cFE FS stubs                                                        */
/* ================================================================== */

int32 CFE_FS_WriteHeader(int32 FileDes, CFE_FS_Header_t *Hdr)
{
    (void)FileDes;
    (void)Hdr;
    CFE_Stubs.CFE_FS_WriteHeader_CallCount++;
    return CFE_Stubs.CFE_FS_WriteHeader_Return;
}

/* ================================================================== */
/* cFE TIME stubs                                                      */
/* ================================================================== */

CFE_TIME_SysTime_t CFE_TIME_GetTime(void)
{
    CFE_Stubs.CFE_TIME_GetTime_CallCount++;
    return CFE_Stubs.CFE_TIME_GetTime_Return;
}

/* ================================================================== */
/* cFE PSP stubs                                                       */
/* ================================================================== */

int32 CFE_PSP_MemCpy(void *dest, void *src, uint32 n)
{
    CFE_Stubs.CFE_PSP_MemCpy_CallCount++;
    if (dest != NULL && src != NULL && n > 0)
    {
        memcpy(dest, src, n);
    }
    return CFE_Stubs.CFE_PSP_MemCpy_Return;
}

int32 CFE_PSP_MemSet(void *dest, uint8 value, uint32 n)
{
    CFE_Stubs.CFE_PSP_MemSet_CallCount++;
    if (dest != NULL && n > 0)
    {
        memset(dest, value, n);
    }
    return CFE_Stubs.CFE_PSP_MemSet_Return;
}

/* ================================================================== */
/* OSAL directory stubs                                                */
/* ================================================================== */

int32 OS_opendir(const char *path)
{
    (void)path;
    CFE_Stubs.OS_opendir_CallCount++;
    /* Reset readdir index on each open so iteration starts fresh */
    CFE_Stubs.OS_readdir_Index = 0;
    return CFE_Stubs.OS_opendir_Return;
}

int32 OS_closedir(uint32 dirp)
{
    (void)dirp;
    CFE_Stubs.OS_closedir_CallCount++;
    return CFE_Stubs.OS_closedir_Return;
}

/*
 * OS_readdir -- cycles through the simulated directory entries.
 * Returns NULL when all entries have been returned.
 */
os_dirent_t *OS_readdir(uint32 dirp)
{
    (void)dirp;
    CFE_Stubs.OS_readdir_CallCount++;

    if (CFE_Stubs.OS_readdir_Index < CFE_Stubs.OS_readdir_NumEntries)
    {
        uint32 idx = CFE_Stubs.OS_readdir_Index;
        CFE_Stubs.OS_readdir_Index++;
        return &CFE_Stubs.OS_readdir_Entries[idx];
    }
    return NULL;
}

int32 OS_stat(const char *path, os_fstat_t *filestats)
{
    (void)path;
    CFE_Stubs.OS_stat_CallCount++;
    if (filestats != NULL)
    {
        *filestats = CFE_Stubs.OS_stat_StatBuf;
    }
    return CFE_Stubs.OS_stat_Return;
}

int32 OS_remove(const char *path)
{
    (void)path;
    CFE_Stubs.OS_remove_CallCount++;
    return CFE_Stubs.OS_remove_Return;
}

int32 OS_rename(const char *old_name, const char *new_name)
{
    (void)old_name;
    (void)new_name;
    CFE_Stubs.OS_rename_CallCount++;
    return CFE_Stubs.OS_rename_Return;
}

int32 OS_mv(const char *src, const char *dest)
{
    (void)src;
    (void)dest;
    CFE_Stubs.OS_mv_CallCount++;
    return CFE_Stubs.OS_mv_Return;
}

/* ================================================================== */
/* OSAL file descriptor stubs                                          */
/* ================================================================== */

int32 OS_FDGetInfo(int32 filedes, OS_FDTableEntry *fd_prop)
{
    (void)filedes;
    CFE_Stubs.OS_FDGetInfo_CallCount++;
    if (fd_prop != NULL)
    {
        *fd_prop = CFE_Stubs.OS_FDGetInfo_Entry;
    }
    return CFE_Stubs.OS_FDGetInfo_Return;
}

/* ================================================================== */
/* OSAL counting semaphore stubs                                       */
/* ================================================================== */

int32 OS_CountSemGetIdByName(uint32 *sem_id, const char *sem_name)
{
    (void)sem_name;
    CFE_Stubs.OS_CountSemGetIdByName_CallCount++;
    if (sem_id != NULL)
    {
        *sem_id = 1;
    }
    return CFE_Stubs.OS_CountSemGetIdByName_Return;
}

int32 OS_CountSemGive(uint32 sem_id)
{
    (void)sem_id;
    CFE_Stubs.OS_CountSemGive_CallCount++;
    return CFE_Stubs.OS_CountSemGive_Return;
}

int32 OS_CountSemTake(uint32 sem_id)
{
    (void)sem_id;
    CFE_Stubs.OS_CountSemTake_CallCount++;
    return CFE_Stubs.OS_CountSemTake_Return;
}

int32 OS_CountSemGetInfo(uint32 sem_id, OS_count_sem_prop_t *sem_prop)
{
    (void)sem_id;
    if (sem_prop != NULL)
    {
        memset(sem_prop, 0, sizeof(*sem_prop));
        sem_prop->value = CFE_Stubs.OS_CountSemGetInfo_SemValue;
    }
    CFE_Stubs.OS_CountSemGetInfo_CallCount++;
    return CFE_Stubs.OS_CountSemGetInfo_Return;
}

int32 OS_CountSemTimedWait(uint32 sem_id, uint32 msecs)
{
    (void)sem_id;
    (void)msecs;
    CFE_Stubs.OS_CountSemTimedWait_CallCount++;
    return CFE_Stubs.OS_CountSemTimedWait_Return;
}

int32 OS_lseek(int32 filedes, int32 offset, uint32 whence)
{
    (void)filedes;
    (void)offset;
    (void)whence;
    CFE_Stubs.OS_lseek_CallCount++;
    return CFE_Stubs.OS_lseek_Return;
}

/* ================================================================== */
/* OSAL misc stubs                                                     */
/* ================================================================== */

void OS_printf(const char *fmt, ...)
{
    (void)fmt;
    CFE_Stubs.OS_printf_CallCount++;
}

/* ================================================================== */
/* OSAL file I/O stubs                                                 */
/* ================================================================== */

int32 OS_creat(const char *path, int32 access)
{
    (void)path;
    (void)access;
    CFE_Stubs.OS_creat_CallCount++;
    return CFE_Stubs.OS_creat_Return;
}

int32 OS_open(const char *path, int32 access, uint32 mode)
{
    (void)path;
    (void)access;
    (void)mode;
    CFE_Stubs.OS_open_CallCount++;
    return CFE_Stubs.OS_open_Return;
}

int32 OS_close(int32 filedes)
{
    (void)filedes;
    CFE_Stubs.OS_close_CallCount++;
    return CFE_Stubs.OS_close_Return;
}

int32 OS_write(int32 filedes, void *buffer, uint32 nbytes)
{
    (void)filedes;
    (void)buffer;
    CFE_Stubs.OS_write_CallCount++;
    /* If no override was set, return nbytes to indicate success */
    if (CFE_Stubs.OS_write_Return == 0)
    {
        return (int32)nbytes;
    }
    return CFE_Stubs.OS_write_Return;
}

int32 OS_read(int32 filedes, void *buffer, uint32 nbytes)
{
    (void)filedes;
    (void)buffer;
    (void)nbytes;
    CFE_Stubs.OS_read_CallCount++;
    return CFE_Stubs.OS_read_Return;
}
