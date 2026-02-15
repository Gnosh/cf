/*
 * Standalone cFE Executive Services header for CF unit tests.
 */
#ifndef _cfe_es_h_
#define _cfe_es_h_

#include "common_types.h"

typedef uint32 CFE_ES_MemHandle_t;
typedef uint32 CFE_ES_CDSHandle_t;
typedef void (*CFE_ES_ChildTaskMainFuncPtr_t)(void);

typedef struct {
    uint32 Dummy;
} CFE_ES_AppInfo_t;

typedef struct {
    uint32 Dummy;
} CFE_ES_TaskInfo_t;

typedef struct {
    uint32 Dummy;
} CFE_ES_DeviceDriver_t;

typedef struct {
    uint32 PoolSize;
    uint32 NumBlocksRequested;
    uint32 CheckErrCntr;
    uint32 NumFreeBytes;
} CFE_ES_MemPoolStats_t;

/* Performance log entry/exit markers */
#define CFE_ES_PERF_ENTRY  1
#define CFE_ES_PERF_EXIT   2

/* Mutex control for pool create */
#define CFE_ES_USE_MUTEX   0
#define CFE_ES_NO_MUTEX    1

/* Convenience macros for perf log */
#define CFE_ES_PerfLogEntry(id) CFE_ES_PerfLogAdd((id), CFE_ES_PERF_ENTRY)
#define CFE_ES_PerfLogExit(id)  CFE_ES_PerfLogAdd((id), CFE_ES_PERF_EXIT)

int32   CFE_ES_RegisterApp(void);
int32   CFE_ES_RunLoop(uint32 *ExitStatus);
void    CFE_ES_ExitApp(uint32 ExitStatus);
void    CFE_ES_WaitForStartupSync(uint32 TimeOutMilliseconds);
int32   CFE_ES_GetAppID(uint32 *AppIdPtr);
int32   CFE_ES_GetAppIDByName(uint32 *AppIdPtr, char *AppName);
int32   CFE_ES_GetAppName(char *AppName, uint32 AppId, uint32 BufferLength);
int32   CFE_ES_WriteToSysLog(const char *SpecStringPtr, ...);
void    CFE_ES_PerfLogAdd(uint32 Marker, uint32 EntryExit);
int32   CFE_ES_PoolCreate(uint32 *HandlePtr, uint8 *MemPtr, uint32 Size);
int32   CFE_ES_PoolCreateEx(uint32 *HandlePtr, uint8 *MemPtr, uint32 Size,
                             uint32 NumBlockSizes, uint32 *BlockSizes, uint16 UseMutex);
int32   CFE_ES_GetPoolBuf(uint32 **BufPtr, CFE_ES_MemHandle_t HandlePtr, uint32 Size);
int32   CFE_ES_PutPoolBuf(CFE_ES_MemHandle_t HandlePtr, uint32 *BufPtr);
int32   CFE_ES_GetPoolBufInfo(CFE_ES_MemHandle_t HandlePtr, uint32 *BufPtr);
int32   CFE_ES_GetMemPoolStats(CFE_ES_MemPoolStats_t *BufPtr, CFE_ES_MemHandle_t Handle);

/* Run status values */
#define CFE_ES_APP_RUN      1
#define CFE_ES_APP_EXIT     2
#define CFE_ES_APP_ERROR    3

#endif /* _cfe_es_h_ */
