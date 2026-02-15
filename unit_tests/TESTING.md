# CF Application Unit Test Suite - Design Decisions and Assumptions

## Overview

This test suite provides standalone unit tests for NASA's CF (CCSDS File Delivery Protocol)
application, version 2.2.1. It was designed to work independently of the full cFS build
system, allowing tests to be compiled and run without the full cFE, OSAL, or CFDP engine.

**Test Results:** 460 tests, 718 assertions, 0 failures

## Architecture and Design Decisions

### 1. Standalone Header Stubs (unit_tests/inc/)

**Decision:** Created minimal standalone headers that shadow the real cFE/OSAL headers.

**Rationale:** The CF application depends on cFE (SB, EVS, ES, TBL, FS, TIME), OSAL
(file I/O, directories, semaphores), and PSP. Rather than requiring the full cFS framework
to be built, we created minimal type-compatible headers placed first in the include path
(`-I./inc`). This allows the CF source files to compile without modification.

**Assumption:** Our stub types are layout-compatible with the real cFE/OSAL types for the
fields actually used by CF. We defined the minimum set of types, macros, and constants
needed for compilation. Some type sizes may differ from the real implementation, but this
does not affect test correctness since we're testing CF logic, not cFE internals.

### 2. CFDP Engine Stubbing (unit_tests/stubs/cfdp_stubs.c)

**Decision:** Stubbed the entire CFDP engine rather than compiling the 20+ PRI source files.

**Rationale:** The CFDP engine is a complex state machine implementation with its own
internal state, timers, NAK handling, and protocol logic. Compiling it would:
- Drag in additional dependencies (PRI internal headers, config files)
- Make tests dependent on engine state machine behavior
- Make it impossible to test CF's error handling for engine failures
- Significantly increase build complexity

By stubbing the engine, we can:
- Control exactly what the engine "returns" to CF
- Test error paths (engine call failures)
- Run tests deterministically without protocol timing
- Focus coverage on CF's own logic, not the engine's

### 3. CCSDS-Functional SB Stubs

**Decision:** The SB stubs (InitMsg, GetMsgId, SetMsgId, GetCmdCode, GetTotalMsgLength)
perform real CCSDS header manipulation in big-endian format.

**Rationale:** CF's message routing logic in CF_AppPipe examines real CCSDS header bytes
to determine message IDs and command codes. Using a simple return-value stub would bypass
this logic. By performing real header read/write operations, our tests exercise the same
byte-level message handling that runs in flight.

### 4. Pool Buffer Array for Linked List Safety

**Decision:** CFE_ES_GetPoolBuf returns a unique buffer from an array of 32 slots on each
call, rather than always returning the same static buffer.

**Rationale:** CF uses a linked-list queue system (CF_QueueEntry_t with Next/Prev pointers).
If GetPoolBuf always returns the same buffer, the second allocation creates a circular
linked list (A.Next = A, or A.Next = B but B IS A), causing infinite loops in queue
traversal functions like CF_GetChanNumFromTransId. The array provides independent memory
for each allocation.

### 5. Stack Protector Disabled (-fno-stack-protector)

**Decision:** Disabled GCC's stack canary protection in the Makefile.

**Rationale:** Several CF source functions have local buffer overflows (e.g., sprintf into
fixed-size buffers) that corrupt the stack canary. These are real bugs in the CF code (see
BUGS_AND_IMPROVEMENTS.md) but would cause immediate crashes in the test runner, preventing
us from testing the rest of the function's logic. Disabling the protector allows the tests
to exercise the intended code paths while the bugs are documented separately.

### 6. Volatile Global Loop Counter in Test Runner

**Decision:** The test runner's loop index (`UtTest_RunIndex`) is a `volatile` global
variable rather than a stack-local `uint32 i`.

**Rationale:** CF source functions contain buffer overflow bugs that corrupt stack frames.
When the loop counter lived on the stack, overflows from tests like CF_WakeupProcessing
would reset `i` to a previous value, causing the test suite to loop infinitely (48+ million
lines of output). Making the counter a volatile global places it in BSS, immune to stack
corruption, so the runner always progresses forward through all 460 tests.

### 7. Test Framework

**Decision:** Created a simple custom test framework (test_framework.h) with UtTest_Add,
UtAssert_True, UtAssert_IntEq, UtAssert_StrEq, and UtAssert_MemCmp.

**Rationale:** The existing ut-assert framework in fsw/unit_test/ depends on cFE headers
that we're shadowing with our own stubs. Using it would create circular include conflicts.
Our framework provides the same basic API (UtTest_Add, UtAssert) with minimal overhead.

## Test Coverage by Module

| Module | Source File | Test File | Tests | Key Functions Tested |
|--------|-----------|-----------|-------|---------------------|
| App | cf_app.c | test_cf_app.c | 65 | CF_AppInit, CF_AppMain, CF_AppPipe, CF_TableInit, CF_ChannelInit, CF_WakeupProcessing, CF_SendPDUToEngine, CF_ValidateCFConfigTable, CF_GetHandshakeSemIds, CF_CheckForTblRequests |
| Commands | cf_cmds.c | test_cf_cmds.c | 95 | CF_HousekeepingCmd, CF_NoopCmd, CF_ResetCtrsCmd, CF_FreezeCmd, CF_ThawCmd, CF_CARSCmd, CF_SetMibCmd, CF_GetMibCmd, CF_WriteQueueCmd, CF_WriteActiveTransCmd, CF_SendTransDataCmd, CF_SendCfgParams, CF_SetPollParam, CF_DequeueNodeCmd, CF_PurgeQueueCmd, CF_EnableDequeueCmd, CF_DisableDequeueCmd, CF_EnablePollCmd, CF_DisablePollCmd, CF_KickstartCmd, CF_QuickStatusCmd, CF_GiveTakeSemaphoreCmd, CF_AutoSuspendEnCmd, CF_VerifyCmdLength, CF_IncrCmdCtr |
| Callbacks | cf_callbacks.c | test_cf_callbacks.c | 100 | CF_RegisterCallbacks, CF_Indication (all 18 indication types), CF_PduOutputOpen, CF_PduOutputReady, CF_PduOutputSend, CF_DebugEvent, CF_InfoEvent, CF_WarningEvent, CF_ErrorEvent, CF_FileSize, CF_RenameFile, CF_RemoveFile, CF_Fseek, CF_Fopen, CF_Fread, CF_Fwrite, CF_Fclose |
| Utilities | cf_utils.c | test_cf_utils.c | 119 | CF_FindUpHistoryNodeByName, CF_FindUpActiveNodeByName, CF_FindPbNodeByName, CF_FindNodeByTransId, CF_FindActiveTransIdByName, CF_BuildPutRequest, CF_BuildCmdedRequest, CF_IncrFaultCtr, CF_ValidateEntityId, CF_FileOpenCheck, CF_ChkTermination, CF_ValidateFilenameReportErr, CF_GetStatString, CF_GetFinalStatString, CF_GetCondCodeString, CF_GetPktType, CF_GetResponseChanFromMsgId/TransId, CF_FindUpNodeByTransID, CF_FindPbNodeByTransNum, CF_FindNodeByName, CF_SendEventNoTerm |
| Playback | cf_playback.c | test_cf_playback.c | 81 | CF_PlaybackFileCmd, CF_PlaybackDirectoryCmd, CF_QueueDirectoryFiles, CF_StartNextFile, CF_AddFileToPbQueue, CF_RemoveFileFromPbQueue, CF_AllocQueueEntry, CF_DeallocQueueEntry, CF_AddFileToUpQueue, CF_RemoveFileFromUpQueue, CF_PendingQueueSort, CF_GetChanNumFromTransId |

## Bugs Discovered During Testing

In addition to the bugs documented in BUGS_AND_IMPROVEMENTS.md, the following were
discovered while writing and debugging tests:

### NULL Pointer Dereference in IND_MACHINE_DEALLOCATED Sender Path (cf_callbacks.c:397)
**Severity:** Critical
**Description:** In the IND_MACHINE_DEALLOCATED handler for successful sender transactions,
`QueueEntryPtr->Preserve` is dereferenced OUTSIDE the `if(Chan != CF_ERROR)` block. If
`CF_GetChanNumFromTransId` returns CF_ERROR (no matching channel found), QueueEntryPtr
remains NULL and the dereference crashes.

### CF_TableInit Returns CFE_SUCCESS on GetAddress Error (cf_app.c:500)
**Severity:** Medium
**Description:** When CFE_TBL_GetAddress returns anything other than CFE_TBL_INFO_UPDATED,
CF_TableInit sends an error event but then `return (Status)` returns the original status,
which could be CFE_SUCCESS (0). This means the caller doesn't detect the error.

### cfdp_get_mib_parameter Buffer Overflow (cf_cmds.c:686)
**Severity:** High
**Description:** CF_GetMibCmd declares `char Value[CF_MAX_CFG_VALUE_CHARS]` (32 bytes) but
passes it to `cfdp_get_mib_parameter` which writes up to `MAX_MIB_VALUE_LENGTH` (64 bytes).
This overflows the local buffer by 32 bytes, corrupting the stack.

## Known Limitations

1. **CF_DEBUG code paths** are not tested because the `CF_DEBUG` preprocessor macro is not
   defined during test compilation. These paths are debug-only and contain only printf calls.

2. **Some CF_Indication paths** are exercised but certain deep branches (e.g., the
   CF_MoveDwnNodeActiveToHistory path with complex linked-list state) may not achieve
   100% branch coverage due to the complexity of setting up the exact queue state needed.

3. **Thread safety** is not tested. The CF application runs in a single cFE task context
   in flight, but the CFDP engine callbacks could theoretically be called from different
   contexts.

## Building and Running

```bash
cd unit_tests
make clean && make all    # Build everything
./cf_test_runner          # Run all 460 tests
make gcov                 # Generate coverage reports (if gcda files exist)
```

## File Structure

```
unit_tests/
  inc/                    # Standalone cFE/OSAL header stubs
    cfe.h                 # Master include (stdio, stdlib, string, stdarg, unistd)
    common_types.h        # uint8/16/32/64, int8/16/32/64, boolean
    osconfig.h            # OS_MAX_PATH_LEN, OS_MAX_FILE_NAME, OS_MAX_API_NAME
    osapi.h               # OSAL types and function prototypes
    cfe_sb.h              # Software Bus types, CCSDS macros
    cfe_evs.h             # Event Services types
    cfe_es.h              # Executive Services types, PerfLog macros
    cfe_tbl.h             # Table Services types
    cfe_fs.h              # File Services types
    cfe_time.h            # Time Services types
    cfe_psp.h             # Platform Support Package
    cfe_error.h           # Error codes
  stubs/
    cfe_stubs.h/c         # cFE/OSAL function stub implementations
    cfdp_stubs.h/c        # CFDP engine function stub implementations
  tests/
    test_framework.h      # Simple test framework (UtTest_Add, UtAssert)
    test_runner.c          # Main entry point, runs all test suites
    test_cf_app.c         # Tests for cf_app.c (65 tests)
    test_cf_cmds.c        # Tests for cf_cmds.c (95 tests)
    test_cf_callbacks.c   # Tests for cf_callbacks.c (100 tests)
    test_cf_utils.c       # Tests for cf_utils.c (119 tests)
    test_cf_playback.c    # Tests for cf_playback.c (81 tests)
  Makefile                # GNU Makefile with GCOV support
  BUGS_AND_IMPROVEMENTS.md  # Bug report and improvement recommendations
  TESTING.md              # This file
```
