/*
 * Standalone cFE Software Bus header for CF unit tests.
 */
#ifndef _cfe_sb_h_
#define _cfe_sb_h_

#include "common_types.h"

typedef uint16 CFE_SB_MsgId_t;
typedef uint8  CFE_SB_PipeId_t;
typedef void * CFE_SB_MsgPtr_t;
typedef void   CFE_SB_Msg_t;
typedef uint32 CFE_SB_ZeroCopyHandle_t;

typedef struct {
    uint8 Priority;
    uint8 Reliability;
} CFE_SB_Qos_t;

typedef struct {
    uint32 ProcessorId;
    uint32 AppId;
} CFE_SB_SenderId_t;

/* CCSDS header sizes */
#define CFE_SB_CMD_HDR_SIZE  8
#define CFE_SB_TLM_HDR_SIZE 12

/* Max pipe depth and msg size */
#define CFE_SB_MAX_PIPE_DEPTH 256
#define CFE_SB_MAX_SB_MSG_SIZE 32768
#define CFE_SB_HIGHEST_VALID_MSGID 0x1FFF

/* Timeouts */
#define CFE_SB_PEND_FOREVER (-1)
#define CFE_SB_POLL          0

/* SB function prototypes */
int32           CFE_SB_CreatePipe(CFE_SB_PipeId_t *PipeIdPtr, uint16 Depth, char *PipeName);
int32           CFE_SB_DeletePipe(CFE_SB_PipeId_t PipeId);
int32           CFE_SB_Subscribe(CFE_SB_MsgId_t MsgId, CFE_SB_PipeId_t PipeId);
int32           CFE_SB_Unsubscribe(CFE_SB_MsgId_t MsgId, CFE_SB_PipeId_t PipeId);
int32           CFE_SB_SendMsg(CFE_SB_Msg_t *MsgPtr);
int32           CFE_SB_RcvMsg(CFE_SB_MsgPtr_t *BufPtr, CFE_SB_PipeId_t PipeId, int32 TimeOut);
void            CFE_SB_InitMsg(void *MsgPtr, CFE_SB_MsgId_t MsgId, uint16 Length, boolean Clear);
CFE_SB_MsgId_t CFE_SB_GetMsgId(CFE_SB_MsgPtr_t MsgPtr);
void            CFE_SB_SetMsgId(CFE_SB_MsgPtr_t MsgPtr, CFE_SB_MsgId_t MsgId);
uint16          CFE_SB_GetCmdCode(CFE_SB_MsgPtr_t MsgPtr);
int32           CFE_SB_SetCmdCode(CFE_SB_MsgPtr_t MsgPtr, uint16 CmdCode);
uint16          CFE_SB_GetTotalMsgLength(CFE_SB_MsgPtr_t MsgPtr);
void            CFE_SB_SetTotalMsgLength(CFE_SB_MsgPtr_t MsgPtr, uint16 TotalLength);
uint16          CFE_SB_GetUserDataLength(CFE_SB_MsgPtr_t MsgPtr);
void            CFE_SB_SetUserDataLength(CFE_SB_MsgPtr_t MsgPtr, uint16 DataLength);
void            CFE_SB_GenerateChecksum(CFE_SB_MsgPtr_t MsgPtr);
boolean         CFE_SB_ValidateChecksum(CFE_SB_MsgPtr_t MsgPtr);
uint16          CFE_SB_GetChecksum(CFE_SB_MsgPtr_t MsgPtr);
void            CFE_SB_TimeStampMsg(CFE_SB_MsgPtr_t MsgPtr);
void           *CFE_SB_GetUserData(CFE_SB_MsgPtr_t MsgPtr);
uint16          CFE_SB_MsgHdrSize(CFE_SB_MsgId_t MsgId);
CFE_SB_Msg_t   *CFE_SB_ZeroCopyGetPtr(uint16 MsgSize, CFE_SB_ZeroCopyHandle_t *BufferHandle);
int32           CFE_SB_ZeroCopyReleasePtr(CFE_SB_Msg_t *Ptr2Release, CFE_SB_ZeroCopyHandle_t BufferHandle);
int32           CFE_SB_ZeroCopySend(CFE_SB_Msg_t *MsgPtr, CFE_SB_ZeroCopyHandle_t BufferHandle);

/* Bit manipulation macros (used by CF for HK flags) */
#define CFE_SET(word, bit)  ((word) |= (1U << (bit)))
#define CFE_CLR(word, bit)  ((word) &= ~(1U << (bit)))
#define CFE_TST(word, bit)  (((word) >> (bit)) & 1U)

#endif /* _cfe_sb_h_ */
