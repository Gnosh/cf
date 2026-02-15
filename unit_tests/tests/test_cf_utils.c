/*
 * Unit tests for cf_utils.c
 */
#include "test_framework.h"
#include "cfe_stubs.h"
#include "cfdp_stubs.h"
#include "cf_app.h"
#include "cf_utils.h"
#include "cf_events.h"
#include "cf_defs.h"

/* ------------------------------------------------------------------ */
/* Extern global state                                                 */
/* ------------------------------------------------------------------ */
extern CF_AppData_t CF_AppData;

/* ------------------------------------------------------------------ */
/* Static table used by tests needing CF_AppData.Tbl                   */
/* ------------------------------------------------------------------ */
static cf_config_table_t TestTbl;

/* ------------------------------------------------------------------ */
/* Static queue entry nodes for linked-list queue testing               */
/* ------------------------------------------------------------------ */
static CF_QueueEntry_t Node1, Node2, Node3, Node4;

/* ------------------------------------------------------------------ */
/* Common setup / teardown                                             */
/* ------------------------------------------------------------------ */
static void UtilsTestSetup(void)
{
    CFE_Stubs_Reset();
    CFDP_Stubs_Reset();
    memset(&CF_AppData, 0, sizeof(CF_AppData));
    memset(&TestTbl, 0, sizeof(TestTbl));
    CF_AppData.Tbl = &TestTbl;

    memset(&Node1, 0, sizeof(Node1));
    memset(&Node2, 0, sizeof(Node2));
    memset(&Node3, 0, sizeof(Node3));
    memset(&Node4, 0, sizeof(Node4));
}

static void UtilsTestTeardown(void)
{
}

/* ================================================================== */
/* Helper: Link nodes into a singly-linked queue                       */
/* ================================================================== */
static void LinkNodes2(CF_Queue_t *Q, CF_QueueEntry_t *A, CF_QueueEntry_t *B)
{
    Q->HeadPtr = A;
    Q->TailPtr = B;
    Q->EntryCnt = 2;
    A->Prev = NULL;
    A->Next = (void *)B;
    B->Prev = (void *)A;
    B->Next = NULL;
}


/* ================================================================== */
/* 1. CF_FindUpHistoryNodeByName                                       */
/* ================================================================== */
static void Test_FindUpHistoryNodeByName_Found(void)
{
    strncpy(Node1.SrcFile, "/cf/file1.dat", OS_MAX_PATH_LEN);
    strncpy(Node2.SrcFile, "/cf/file2.dat", OS_MAX_PATH_LEN);
    LinkNodes2(&CF_AppData.UpQ[CF_UP_HISTORYQ], &Node1, &Node2);

    CF_QueueEntry_t *Result = CF_FindUpHistoryNodeByName("/cf/file2.dat");
    UtAssert_True(Result == &Node2, "FindUpHistoryNodeByName found second node");
}

static void Test_FindUpHistoryNodeByName_NotFound(void)
{
    strncpy(Node1.SrcFile, "/cf/file1.dat", OS_MAX_PATH_LEN);
    CF_AppData.UpQ[CF_UP_HISTORYQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindUpHistoryNodeByName("/cf/noexist.dat");
    UtAssert_True(Result == NULL, "FindUpHistoryNodeByName returns NULL when not found");
}

static void Test_FindUpHistoryNodeByName_EmptyQueue(void)
{
    CF_QueueEntry_t *Result = CF_FindUpHistoryNodeByName("/cf/file.dat");
    UtAssert_True(Result == NULL, "FindUpHistoryNodeByName returns NULL on empty queue");
}

/* ================================================================== */
/* 2. CF_FindUpActiveNodeByName                                        */
/* ================================================================== */
static void Test_FindUpActiveNodeByName_Found(void)
{
    strncpy(Node1.SrcFile, "/cf/active.dat", OS_MAX_PATH_LEN);
    CF_AppData.UpQ[CF_UP_ACTIVEQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindUpActiveNodeByName("/cf/active.dat");
    UtAssert_True(Result == &Node1, "FindUpActiveNodeByName found node");
}

static void Test_FindUpActiveNodeByName_NotFound(void)
{
    strncpy(Node1.SrcFile, "/cf/active.dat", OS_MAX_PATH_LEN);
    CF_AppData.UpQ[CF_UP_ACTIVEQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindUpActiveNodeByName("/cf/other.dat");
    UtAssert_True(Result == NULL, "FindUpActiveNodeByName not found returns NULL");
}

/* ================================================================== */
/* 3. CF_FindPbNodeByName - searches active, pending, history          */
/* ================================================================== */
static void Test_FindPbNodeByName_FoundOnActive(void)
{
    TestTbl.OuCh[0].EntryInUse = CF_ENTRY_IN_USE;
    strncpy(Node1.SrcFile, "/cf/pbfile.dat", OS_MAX_PATH_LEN);
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindPbNodeByName("/cf/pbfile.dat");
    UtAssert_True(Result == &Node1, "FindPbNodeByName found on active queue");
}

static void Test_FindPbNodeByName_FoundOnPending(void)
{
    TestTbl.OuCh[0].EntryInUse = CF_ENTRY_IN_USE;
    strncpy(Node1.SrcFile, "/cf/pending.dat", OS_MAX_PATH_LEN);
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindPbNodeByName("/cf/pending.dat");
    UtAssert_True(Result == &Node1, "FindPbNodeByName found on pending queue");
}

static void Test_FindPbNodeByName_FoundOnHistory(void)
{
    TestTbl.OuCh[0].EntryInUse = CF_ENTRY_IN_USE;
    strncpy(Node1.SrcFile, "/cf/hist.dat", OS_MAX_PATH_LEN);
    CF_AppData.Chan[0].PbQ[CF_PB_HISTORYQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindPbNodeByName("/cf/hist.dat");
    UtAssert_True(Result == &Node1, "FindPbNodeByName found on history queue");
}

static void Test_FindPbNodeByName_NotFound(void)
{
    TestTbl.OuCh[0].EntryInUse = CF_ENTRY_IN_USE;
    CF_QueueEntry_t *Result = CF_FindPbNodeByName("/cf/noexist.dat");
    UtAssert_True(Result == NULL, "FindPbNodeByName returns NULL when not found");
}

static void Test_FindPbNodeByName_ChannelNotInUse(void)
{
    /* Channel not marked in-use, so node should not be found */
    TestTbl.OuCh[0].EntryInUse = CF_ENTRY_UNUSED;
    strncpy(Node1.SrcFile, "/cf/pbfile.dat", OS_MAX_PATH_LEN);
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindPbNodeByName("/cf/pbfile.dat");
    UtAssert_True(Result == NULL, "FindPbNodeByName returns NULL when channel not in use");
}

/* ================================================================== */
/* 4. CF_FindNodeByTransId                                             */
/* ================================================================== */
static void Test_FindNodeByTransId_UplinkActive(void)
{
    strncpy(TestTbl.FlightEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);

    /* Set up a node on the uplink active queue with peer entity id "0.23" */
    strncpy(Node1.SrcEntityId, "0.23", CF_MAX_CFG_VALUE_CHARS);
    Node1.TransNum = 5;
    CF_AppData.UpQ[CF_UP_ACTIVEQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindNodeByTransId("0.23_5");
    UtAssert_True(Result == &Node1, "FindNodeByTransId found on uplink active queue");
}

static void Test_FindNodeByTransId_UplinkHistory(void)
{
    strncpy(TestTbl.FlightEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);

    strncpy(Node1.SrcEntityId, "0.23", CF_MAX_CFG_VALUE_CHARS);
    Node1.TransNum = 10;
    CF_AppData.UpQ[CF_UP_HISTORYQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindNodeByTransId("0.23_10");
    UtAssert_True(Result == &Node1, "FindNodeByTransId found on uplink history queue");
}

static void Test_FindNodeByTransId_Downlink(void)
{
    strncpy(TestTbl.FlightEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);
    TestTbl.OuCh[0].EntryInUse = CF_ENTRY_IN_USE;

    strncpy(Node1.SrcEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);
    Node1.TransNum = 7;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindNodeByTransId("0.24_7");
    UtAssert_True(Result == &Node1, "FindNodeByTransId found on downlink queue");
}

static void Test_FindNodeByTransId_NotFound(void)
{
    strncpy(TestTbl.FlightEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);

    CF_QueueEntry_t *Result = CF_FindNodeByTransId("0.23_99");
    UtAssert_True(Result == NULL, "FindNodeByTransId returns NULL when not found");
}

static void Test_FindNodeByTransId_InvalidEntityId(void)
{
    strncpy(TestTbl.FlightEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);

    CF_QueueEntry_t *Result = CF_FindNodeByTransId("bad_5");
    UtAssert_True(Result == NULL, "FindNodeByTransId returns NULL for invalid entity id");
}

static void Test_FindNodeByTransId_NoUnderscore(void)
{
    strncpy(TestTbl.FlightEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);

    CF_QueueEntry_t *Result = CF_FindNodeByTransId("0.24");
    /* Entity ID "0.24" has no underscore so trans num will be 0 */
    UtAssert_True(Result == NULL, "FindNodeByTransId with no underscore returns NULL");
}

/* ================================================================== */
/* 5. CF_FindActiveTransIdByName                                       */
/* ================================================================== */
static void Test_FindActiveTransIdByName_FoundUplink(void)
{
    char TransIdBuf[CF_MAX_TRANSID_CHARS];

    strncpy(Node1.SrcFile, "/cf/upfile.dat", OS_MAX_PATH_LEN);
    strncpy(Node1.SrcEntityId, "0.23", CF_MAX_CFG_VALUE_CHARS);
    Node1.TransNum = 42;
    CF_AppData.UpQ[CF_UP_ACTIVEQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    int32 Status = CF_FindActiveTransIdByName(TransIdBuf, "/cf/upfile.dat");
    UtAssert_True(Status == CF_SUCCESS, "FindActiveTransIdByName returns SUCCESS for uplink");
    UtAssert_StrEq(TransIdBuf, "0.23_42", "TransIdBuf contains correct transaction id");
}

static void Test_FindActiveTransIdByName_FoundPlayback(void)
{
    char TransIdBuf[CF_MAX_TRANSID_CHARS];

    TestTbl.OuCh[0].EntryInUse = CF_ENTRY_IN_USE;
    strncpy(Node1.SrcFile, "/cf/pbfile.dat", OS_MAX_PATH_LEN);
    strncpy(Node1.SrcEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);
    Node1.TransNum = 100;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    int32 Status = CF_FindActiveTransIdByName(TransIdBuf, "/cf/pbfile.dat");
    UtAssert_True(Status == CF_SUCCESS, "FindActiveTransIdByName returns SUCCESS for playback");
    UtAssert_StrEq(TransIdBuf, "0.24_100", "TransIdBuf contains playback transaction id");
}

static void Test_FindActiveTransIdByName_NotFound(void)
{
    char TransIdBuf[CF_MAX_TRANSID_CHARS];
    memset(TransIdBuf, 0, sizeof(TransIdBuf));

    int32 Status = CF_FindActiveTransIdByName(TransIdBuf, "/cf/noexist.dat");
    UtAssert_True(Status == CF_ERROR, "FindActiveTransIdByName returns ERROR when not found");
}

/* ================================================================== */
/* 6. CF_BuildPutRequest                                               */
/* ================================================================== */
static void Test_BuildPutRequest_Class1(void)
{
    CF_QueueEntry_t Entry;
    memset(&Entry, 0, sizeof(Entry));
    Entry.Class = 1;
    Entry.ChanNum = 0;
    strncpy(Entry.SrcFile, "/cf/src.dat", OS_MAX_PATH_LEN);
    strncpy(Entry.PeerEntityId, "0.23", CF_MAX_CFG_VALUE_CHARS);
    strncpy(Entry.DstFile, "/gnd/dst.dat", OS_MAX_PATH_LEN);

    CFDP_Stubs.give_request_return = TRUE;

    int32 Status = CF_BuildPutRequest(&Entry);
    UtAssert_True(Status == CF_SUCCESS, "BuildPutRequest class1 returns SUCCESS");
    UtAssert_True(Entry.Status == CF_STAT_PUT_REQ_ISSUED,
                  "Status set to PUT_REQ_ISSUED");
    UtAssert_True(CFDP_Stubs.give_request_call_count == 1,
                  "cfdp_give_request called once");
}

static void Test_BuildPutRequest_Class2(void)
{
    CF_QueueEntry_t Entry;
    memset(&Entry, 0, sizeof(Entry));
    Entry.Class = 2;
    Entry.ChanNum = 0;
    strncpy(Entry.SrcFile, "/cf/src.dat", OS_MAX_PATH_LEN);
    strncpy(Entry.PeerEntityId, "0.23", CF_MAX_CFG_VALUE_CHARS);
    strncpy(Entry.DstFile, "/gnd/dst.dat", OS_MAX_PATH_LEN);

    CFDP_Stubs.give_request_return = TRUE;

    int32 Status = CF_BuildPutRequest(&Entry);
    UtAssert_True(Status == CF_SUCCESS, "BuildPutRequest class2 returns SUCCESS");
}

static void Test_BuildPutRequest_EngineError(void)
{
    CF_QueueEntry_t Entry;
    memset(&Entry, 0, sizeof(Entry));
    Entry.Class = 1;
    Entry.ChanNum = 0;
    strncpy(Entry.SrcFile, "/cf/src.dat", OS_MAX_PATH_LEN);
    strncpy(Entry.PeerEntityId, "0.23", CF_MAX_CFG_VALUE_CHARS);
    strncpy(Entry.DstFile, "/gnd/dst.dat", OS_MAX_PATH_LEN);

    CFDP_Stubs.give_request_return = FALSE;

    int32 Status = CF_BuildPutRequest(&Entry);
    UtAssert_True(Status == CF_ERROR, "BuildPutRequest returns ERROR when engine fails");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_PUT_REQ_ERR2_EID,
                  "Error event sent for engine failure");
}

static void Test_BuildPutRequest_SrcFileTooLong(void)
{
    CF_QueueEntry_t Entry;
    memset(&Entry, 0, sizeof(Entry));
    Entry.Class = 1;
    Entry.ChanNum = 0;
    /* After "PUT -class1 " prefix (13 bytes), SrcFile needs to push total >= 127.
     * So we need strlen(SrcFile) >= 114. OS_MAX_PATH_LEN is only 64,
     * so we fill all fields fully to make combined length exceed limit at
     * the PeerEntityId or DstFile check. */
    memset(Entry.SrcFile, 'A', OS_MAX_PATH_LEN - 1);
    Entry.SrcFile[OS_MAX_PATH_LEN - 1] = '\0';
    /* Fill PeerEntityId with long string to push total over the limit */
    memset(Entry.PeerEntityId, 'B', CF_MAX_CFG_VALUE_CHARS - 1);
    Entry.PeerEntityId[CF_MAX_CFG_VALUE_CHARS - 1] = '\0';
    memset(Entry.DstFile, 'C', OS_MAX_PATH_LEN - 1);
    Entry.DstFile[OS_MAX_PATH_LEN - 1] = '\0';

    int32 Status = CF_BuildPutRequest(&Entry);
    UtAssert_True(Status == CF_ERROR, "BuildPutRequest returns ERROR for too-long combined string");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_PUT_REQ_ERR1_EID,
                  "PutStrTooLong event sent");
}

/* ================================================================== */
/* 7. CF_BuildCmdedRequest                                             */
/* ================================================================== */
static void Test_BuildCmdedRequest_Suspend(void)
{
    CFDP_Stubs.give_request_return = TRUE;
    int32 Status = CF_BuildCmdedRequest("SUSPEND", "0.24_5");
    UtAssert_True(Status == CF_SUCCESS, "BuildCmdedRequest SUSPEND returns SUCCESS");
    UtAssert_True(CFDP_Stubs.give_request_call_count == 1,
                  "cfdp_give_request called for suspend");
}

static void Test_BuildCmdedRequest_Resume(void)
{
    CFDP_Stubs.give_request_return = TRUE;
    int32 Status = CF_BuildCmdedRequest("RESUME", "0.24_5");
    UtAssert_True(Status == CF_SUCCESS, "BuildCmdedRequest RESUME returns SUCCESS");
}

static void Test_BuildCmdedRequest_Cancel(void)
{
    CFDP_Stubs.give_request_return = TRUE;
    int32 Status = CF_BuildCmdedRequest("CANCEL", "0.24_3");
    UtAssert_True(Status == CF_SUCCESS, "BuildCmdedRequest CANCEL returns SUCCESS");
}

static void Test_BuildCmdedRequest_Abandon(void)
{
    CFDP_Stubs.give_request_return = TRUE;
    int32 Status = CF_BuildCmdedRequest("ABANDON", "0.24_3");
    UtAssert_True(Status == CF_SUCCESS, "BuildCmdedRequest ABANDON returns SUCCESS");
}

static void Test_BuildCmdedRequest_EngineError(void)
{
    CFDP_Stubs.give_request_return = FALSE;
    int32 Status = CF_BuildCmdedRequest("CANCEL", "0.24_3");
    UtAssert_True(Status == CF_ERROR, "BuildCmdedRequest returns ERROR when engine fails");
}

/* ================================================================== */
/* 8. CF_IncrFaultCtr                                                  */
/* ================================================================== */
static void Test_IncrFaultCtr_NoError(void)
{
    TRANS_STATUS TransInfo;
    memset(&TransInfo, 0, sizeof(TransInfo));
    TransInfo.condition_code = NO_ERROR;

    CF_IncrFaultCtr(&TransInfo);
    /* NO_ERROR case does nothing, just verify no crash */
    UtAssert_True(1, "IncrFaultCtr NO_ERROR does not crash");
}

static void Test_IncrFaultCtr_PosAck(void)
{
    TRANS_STATUS TransInfo;
    memset(&TransInfo, 0, sizeof(TransInfo));
    TransInfo.condition_code = POSITIVE_ACK_LIMIT_REACHED;

    CF_IncrFaultCtr(&TransInfo);
    UtAssert_True(CF_AppData.Hk.Cond.PosAckNum == 1,
                  "PosAckNum incremented");
}

static void Test_IncrFaultCtr_FilestoreRejection(void)
{
    TRANS_STATUS TransInfo;
    memset(&TransInfo, 0, sizeof(TransInfo));
    TransInfo.condition_code = FILESTORE_REJECTION;

    CF_IncrFaultCtr(&TransInfo);
    UtAssert_True(CF_AppData.Hk.Cond.FileStoreRejNum == 1,
                  "FileStoreRejNum incremented");
}

static void Test_IncrFaultCtr_FileChecksum(void)
{
    TRANS_STATUS TransInfo;
    memset(&TransInfo, 0, sizeof(TransInfo));
    TransInfo.condition_code = FILE_CHECKSUM_FAILURE;

    CF_IncrFaultCtr(&TransInfo);
    UtAssert_True(CF_AppData.Hk.Cond.FileChecksumNum == 1,
                  "FileChecksumNum incremented");
}

static void Test_IncrFaultCtr_FileSize(void)
{
    TRANS_STATUS TransInfo;
    memset(&TransInfo, 0, sizeof(TransInfo));
    TransInfo.condition_code = FILE_SIZE_ERROR;

    CF_IncrFaultCtr(&TransInfo);
    UtAssert_True(CF_AppData.Hk.Cond.FileSizeNum == 1,
                  "FileSizeNum incremented");
}

static void Test_IncrFaultCtr_NakLimit(void)
{
    TRANS_STATUS TransInfo;
    memset(&TransInfo, 0, sizeof(TransInfo));
    TransInfo.condition_code = NAK_LIMIT_REACHED;

    CF_IncrFaultCtr(&TransInfo);
    UtAssert_True(CF_AppData.Hk.Cond.NakLimitNum == 1,
                  "NakLimitNum incremented");
}

static void Test_IncrFaultCtr_Inactivity(void)
{
    TRANS_STATUS TransInfo;
    memset(&TransInfo, 0, sizeof(TransInfo));
    TransInfo.condition_code = INACTIVITY_DETECTED;

    CF_IncrFaultCtr(&TransInfo);
    UtAssert_True(CF_AppData.Hk.Cond.InactiveNum == 1,
                  "InactiveNum incremented");
}

static void Test_IncrFaultCtr_Suspend(void)
{
    TRANS_STATUS TransInfo;
    memset(&TransInfo, 0, sizeof(TransInfo));
    TransInfo.condition_code = SUSPEND_REQUEST_RECEIVED;

    CF_IncrFaultCtr(&TransInfo);
    UtAssert_True(CF_AppData.Hk.Cond.SuspendNum == 1,
                  "SuspendNum incremented");
}

static void Test_IncrFaultCtr_Cancel(void)
{
    TRANS_STATUS TransInfo;
    memset(&TransInfo, 0, sizeof(TransInfo));
    TransInfo.condition_code = CANCEL_REQUEST_RECEIVED;

    CF_IncrFaultCtr(&TransInfo);
    UtAssert_True(CF_AppData.Hk.Cond.CancelNum == 1,
                  "CancelNum incremented");
}

static void Test_IncrFaultCtr_Default(void)
{
    TRANS_STATUS TransInfo;
    memset(&TransInfo, 0, sizeof(TransInfo));
    TransInfo.condition_code = 99; /* unexpected value */

    CF_IncrFaultCtr(&TransInfo);
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_IND_FAU_UNEX_EID,
                  "Unexpected condition code triggers error event");
}

/* ================================================================== */
/* 9. CF_ValidateEntityId                                              */
/* ================================================================== */
static void Test_ValidateEntityId_ValidDottedDecimal(void)
{
    int32 Status = CF_ValidateEntityId("0.24");
    UtAssert_True(Status == CF_SUCCESS, "ValidateEntityId '0.24' returns SUCCESS");
}

static void Test_ValidateEntityId_ValidSingleDigits(void)
{
    int32 Status = CF_ValidateEntityId("1.2");
    UtAssert_True(Status == CF_SUCCESS, "ValidateEntityId '1.2' returns SUCCESS");
}

static void Test_ValidateEntityId_ValidMax(void)
{
    int32 Status = CF_ValidateEntityId("255.255");
    UtAssert_True(Status == CF_SUCCESS, "ValidateEntityId '255.255' returns SUCCESS");
}

static void Test_ValidateEntityId_TooShort(void)
{
    int32 Status = CF_ValidateEntityId("88");
    UtAssert_True(Status == CF_ERROR, "ValidateEntityId '88' (too short, no dot) returns ERROR");
}

static void Test_ValidateEntityId_Empty(void)
{
    int32 Status = CF_ValidateEntityId("");
    UtAssert_True(Status == CF_ERROR, "ValidateEntityId empty string returns ERROR");
}

static void Test_ValidateEntityId_NoDot(void)
{
    int32 Status = CF_ValidateEntityId("12345");
    UtAssert_True(Status == CF_ERROR, "ValidateEntityId '12345' no dot returns ERROR");
}

static void Test_ValidateEntityId_ValueTooLarge(void)
{
    int32 Status = CF_ValidateEntityId("256.1");
    UtAssert_True(Status == CF_ERROR, "ValidateEntityId '256.1' first value > 255 returns ERROR");
}

static void Test_ValidateEntityId_SecondValueTooLarge(void)
{
    int32 Status = CF_ValidateEntityId("1.256");
    UtAssert_True(Status == CF_ERROR, "ValidateEntityId '1.256' second value > 255 returns ERROR");
}

static void Test_ValidateEntityId_TooLong(void)
{
    int32 Status = CF_ValidateEntityId("255.25555");
    UtAssert_True(Status == CF_ERROR, "ValidateEntityId too long string returns ERROR");
}

/* ================================================================== */
/* 10. CF_FileOpenCheck                                                */
/* ================================================================== */
static void Test_FileOpenCheck_FileOpen(void)
{
    /* Stub returns success and path matches for first FD */
    CFE_Stubs.OS_FDGetInfo_Return = OS_FS_SUCCESS;
    strncpy(CFE_Stubs.OS_FDGetInfo_Entry.Path, "/cf/myfile.dat", OS_MAX_PATH_LEN);

    uint32 Result = CF_FileOpenCheck("/cf/myfile.dat");
    UtAssert_True(Result == CF_OPEN, "FileOpenCheck returns OPEN when path matches");
}

static void Test_FileOpenCheck_FileNotOpen(void)
{
    /* Stub returns invalid FD for all entries */
    CFE_Stubs.OS_FDGetInfo_Return = OS_FS_ERR_INVALID_FD;

    uint32 Result = CF_FileOpenCheck("/cf/myfile.dat");
    UtAssert_True(Result == CF_CLOSED, "FileOpenCheck returns CLOSED when no match");
}

static void Test_FileOpenCheck_FDValidButPathDiffers(void)
{
    CFE_Stubs.OS_FDGetInfo_Return = OS_FS_SUCCESS;
    strncpy(CFE_Stubs.OS_FDGetInfo_Entry.Path, "/cf/other.dat", OS_MAX_PATH_LEN);

    uint32 Result = CF_FileOpenCheck("/cf/myfile.dat");
    UtAssert_True(Result == CF_CLOSED,
                  "FileOpenCheck returns CLOSED when path does not match");
}

/* ================================================================== */
/* 11. CF_CheckIfFileIsActive                                          */
/* ================================================================== */
static void Test_CheckIfFileIsActive_OnUplink(void)
{
    strncpy(Node1.SrcFile, "/cf/upload.dat", OS_MAX_PATH_LEN);
    CF_AppData.UpQ[CF_UP_ACTIVEQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    int32 Result = CF_CheckIfFileIsActive("/cf/upload.dat");
    UtAssert_True(Result == CF_FILE_IS_ACTIVE, "CheckIfFileIsActive finds uplink active file");
}

static void Test_CheckIfFileIsActive_OnPlayback(void)
{
    TestTbl.OuCh[0].EntryInUse = CF_ENTRY_IN_USE;
    strncpy(Node1.SrcFile, "/cf/download.dat", OS_MAX_PATH_LEN);
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    int32 Result = CF_CheckIfFileIsActive("/cf/download.dat");
    UtAssert_True(Result == CF_FILE_IS_ACTIVE,
                  "CheckIfFileIsActive finds playback active file");
}

static void Test_CheckIfFileIsActive_NotActive(void)
{
    int32 Result = CF_CheckIfFileIsActive("/cf/nofile.dat");
    UtAssert_True(Result == CF_FILE_NOT_ACTIVE,
                  "CheckIfFileIsActive returns NOT_ACTIVE when file not found");
}

/* ================================================================== */
/* 12. CF_ValidatePathFile                                             */
/* ================================================================== */
static void Test_ValidatePathFile_Valid(void)
{
    int32 Status = CF_ValidatePathFile("/cf/myfile.dat");
    UtAssert_True(Status == CF_SUCCESS, "ValidatePathFile valid path returns SUCCESS");
}

static void Test_ValidatePathFile_Empty(void)
{
    int32 Status = CF_ValidatePathFile("");
    UtAssert_True(Status == CF_SUCCESS, "ValidatePathFile empty string returns SUCCESS (terminates immediately)");
}

static void Test_ValidatePathFile_HasSpace(void)
{
    int32 Status = CF_ValidatePathFile("/cf/my file.dat");
    UtAssert_True(Status == CF_ERROR, "ValidatePathFile with space returns ERROR");
}

static void Test_ValidatePathFile_Null(void)
{
    int32 Status = CF_ValidatePathFile(NULL);
    UtAssert_True(Status == CF_ERROR, "ValidatePathFile NULL pointer returns ERROR");
}

/* ================================================================== */
/* 13. CF_ValidateSrcPath                                              */
/* ================================================================== */
static void Test_ValidateSrcPath_Valid(void)
{
    int32 Status = CF_ValidateSrcPath("/cf/path/");
    UtAssert_True(Status == CF_SUCCESS, "ValidateSrcPath valid path with trailing / returns SUCCESS");
}

static void Test_ValidateSrcPath_NoTrailingSlash(void)
{
    int32 Status = CF_ValidateSrcPath("/cf/path");
    UtAssert_True(Status == CF_ERROR, "ValidateSrcPath no trailing slash returns ERROR");
}

static void Test_ValidateSrcPath_HasSpace(void)
{
    int32 Status = CF_ValidateSrcPath("/cf/ path/");
    UtAssert_True(Status == CF_ERROR, "ValidateSrcPath with space returns ERROR");
}

static void Test_ValidateSrcPath_Null(void)
{
    int32 Status = CF_ValidateSrcPath(NULL);
    UtAssert_True(Status == CF_ERROR, "ValidateSrcPath NULL returns ERROR");
}

/* ================================================================== */
/* 14. CF_ValidateDstPath                                              */
/* ================================================================== */
static void Test_ValidateDstPath_Valid(void)
{
    int32 Status = CF_ValidateDstPath("/gnd/path/");
    UtAssert_True(Status == CF_SUCCESS, "ValidateDstPath valid returns SUCCESS");
}

static void Test_ValidateDstPath_EmptyAllowed(void)
{
    int32 Status = CF_ValidateDstPath("");
    UtAssert_True(Status == CF_SUCCESS, "ValidateDstPath empty string is allowed");
}

static void Test_ValidateDstPath_HasSpace(void)
{
    int32 Status = CF_ValidateDstPath("/gnd/ path");
    UtAssert_True(Status == CF_ERROR, "ValidateDstPath with space returns ERROR");
}

static void Test_ValidateDstPath_Null(void)
{
    int32 Status = CF_ValidateDstPath(NULL);
    UtAssert_True(Status == CF_ERROR, "ValidateDstPath NULL returns ERROR");
}

/* ================================================================== */
/* 15. CF_ChkTermination                                               */
/* ================================================================== */
static void Test_ChkTermination_Terminated(void)
{
    int32 Status = CF_ChkTermination("hello", 10);
    UtAssert_True(Status == CF_SUCCESS, "ChkTermination terminated string returns SUCCESS");
}

static void Test_ChkTermination_NotTerminated(void)
{
    char buf[4];
    memset(buf, 'A', sizeof(buf));  /* no null terminator within 4 bytes */

    int32 Status = CF_ChkTermination(buf, 4);
    UtAssert_True(Status == CF_ERROR, "ChkTermination unterminated string returns ERROR");
}

static void Test_ChkTermination_Null(void)
{
    int32 Status = CF_ChkTermination(NULL, 10);
    UtAssert_True(Status == CF_ERROR, "ChkTermination NULL returns ERROR");
}

static void Test_ChkTermination_ExactLength(void)
{
    /* String terminates exactly at position MaxLength-1 */
    char buf[5] = "ABCD";  /* null at index 4 */
    int32 Status = CF_ChkTermination(buf, 5);
    UtAssert_True(Status == CF_SUCCESS, "ChkTermination exact boundary returns SUCCESS");
}

/* ================================================================== */
/* 16. CF_GetStatString                                                */
/* ================================================================== */
static void Test_GetStatString_Unknown(void)
{
    char buf[32];
    CF_GetStatString(buf, CF_STAT_UNKNOWN, sizeof(buf));
    UtAssert_StrEq(buf, "UNKNOWN", "GetStatString UNKNOWN");
}

static void Test_GetStatString_Success(void)
{
    char buf[32];
    CF_GetStatString(buf, CF_STAT_SUCCESS, sizeof(buf));
    UtAssert_StrEq(buf, "SUCCESSFUL", "GetStatString SUCCESS");
}

static void Test_GetStatString_Cancelled(void)
{
    char buf[32];
    CF_GetStatString(buf, CF_STAT_CANCELLED, sizeof(buf));
    UtAssert_StrEq(buf, "CANCELLED", "GetStatString CANCELLED");
}

static void Test_GetStatString_Abandon(void)
{
    char buf[32];
    CF_GetStatString(buf, CF_STAT_ABANDON, sizeof(buf));
    UtAssert_StrEq(buf, "ABANDONED", "GetStatString ABANDONED");
}

static void Test_GetStatString_NoMeta(void)
{
    char buf[32];
    CF_GetStatString(buf, CF_STAT_NO_META, sizeof(buf));
    UtAssert_StrEq(buf, "NO_METADATA", "GetStatString NO_METADATA");
}

static void Test_GetStatString_Pending(void)
{
    char buf[32];
    CF_GetStatString(buf, CF_STAT_PENDING, sizeof(buf));
    UtAssert_StrEq(buf, "PENDING", "GetStatString PENDING");
}

static void Test_GetStatString_AlrdyActive(void)
{
    char buf[32];
    CF_GetStatString(buf, CF_STAT_ALRDY_ACTIVE, sizeof(buf));
    UtAssert_StrEq(buf, "ALRDY_ACTIVE", "GetStatString ALRDY_ACTIVE");
}

static void Test_GetStatString_PutReqIssued(void)
{
    char buf[32];
    CF_GetStatString(buf, CF_STAT_PUT_REQ_ISSUED, sizeof(buf));
    UtAssert_StrEq(buf, "PUT_REQ_ISSUED", "GetStatString PUT_REQ_ISSUED");
}

static void Test_GetStatString_PutReqFail(void)
{
    char buf[32];
    CF_GetStatString(buf, CF_STAT_PUT_REQ_FAIL, sizeof(buf));
    UtAssert_StrEq(buf, "PUT_REQ_FAILED", "GetStatString PUT_REQ_FAILED");
}

static void Test_GetStatString_Active(void)
{
    char buf[32];
    CF_GetStatString(buf, CF_STAT_ACTIVE, sizeof(buf));
    UtAssert_StrEq(buf, "ACTIVE", "GetStatString ACTIVE");
}

static void Test_GetStatString_Default(void)
{
    char buf[32];
    CF_GetStatString(buf, 999, sizeof(buf));
    UtAssert_StrEq(buf, "INV_FINAL_STAT", "GetStatString default");
}

/* ================================================================== */
/* 17. CF_GetFinalStatString                                           */
/* ================================================================== */
static void Test_GetFinalStatString_Unknown(void)
{
    char buf[32];
    CF_GetFinalStatString(buf, FINAL_STATUS_UNKNOWN, sizeof(buf));
    UtAssert_StrEq(buf, "UNKNOWN", "GetFinalStatString UNKNOWN");
}

static void Test_GetFinalStatString_Successful(void)
{
    char buf[32];
    CF_GetFinalStatString(buf, FINAL_STATUS_SUCCESSFUL, sizeof(buf));
    UtAssert_StrEq(buf, "SUCCESSFUL", "GetFinalStatString SUCCESSFUL");
}

static void Test_GetFinalStatString_Cancelled(void)
{
    char buf[32];
    CF_GetFinalStatString(buf, FINAL_STATUS_CANCELLED, sizeof(buf));
    UtAssert_StrEq(buf, "CANCELLED", "GetFinalStatString CANCELLED");
}

static void Test_GetFinalStatString_Abandoned(void)
{
    char buf[32];
    CF_GetFinalStatString(buf, FINAL_STATUS_ABANDONED, sizeof(buf));
    UtAssert_StrEq(buf, "ABANDONED", "GetFinalStatString ABANDONED");
}

static void Test_GetFinalStatString_NoMetadata(void)
{
    char buf[32];
    CF_GetFinalStatString(buf, FINAL_STATUS_NO_METADATA, sizeof(buf));
    UtAssert_StrEq(buf, "NO_METADATA", "GetFinalStatString NO_METADATA");
}

static void Test_GetFinalStatString_Default(void)
{
    char buf[32];
    CF_GetFinalStatString(buf, 999, sizeof(buf));
    UtAssert_StrEq(buf, "INV_FINAL_STAT", "GetFinalStatString default");
}

/* ================================================================== */
/* 18. CF_GetCondCodeString                                            */
/* ================================================================== */
static void Test_GetCondCodeString_NoError(void)
{
    char buf[32];
    CF_GetCondCodeString(buf, NO_ERROR, sizeof(buf));
    UtAssert_StrEq(buf, "NO_ERR", "GetCondCodeString NO_ERROR");
}

static void Test_GetCondCodeString_PosAck(void)
{
    char buf[32];
    CF_GetCondCodeString(buf, POSITIVE_ACK_LIMIT_REACHED, sizeof(buf));
    UtAssert_StrEq(buf, "ACK_LIMIT", "GetCondCodeString POSITIVE_ACK_LIMIT_REACHED");
}

static void Test_GetCondCodeString_FilestoreRej(void)
{
    char buf[32];
    CF_GetCondCodeString(buf, FILESTORE_REJECTION, sizeof(buf));
    UtAssert_StrEq(buf, "FILESTORE_ERR", "GetCondCodeString FILESTORE_REJECTION");
}

static void Test_GetCondCodeString_ChecksumFail(void)
{
    char buf[32];
    CF_GetCondCodeString(buf, FILE_CHECKSUM_FAILURE, sizeof(buf));
    UtAssert_StrEq(buf, "CHKSUM_FAIL", "GetCondCodeString FILE_CHECKSUM_FAILURE");
}

static void Test_GetCondCodeString_FileSizeErr(void)
{
    char buf[32];
    CF_GetCondCodeString(buf, FILE_SIZE_ERROR, sizeof(buf));
    UtAssert_StrEq(buf, "FILESIZE_ERR", "GetCondCodeString FILE_SIZE_ERROR");
}

static void Test_GetCondCodeString_NakLimit(void)
{
    char buf[32];
    CF_GetCondCodeString(buf, NAK_LIMIT_REACHED, sizeof(buf));
    UtAssert_StrEq(buf, "NAK_LIMIT", "GetCondCodeString NAK_LIMIT_REACHED");
}

static void Test_GetCondCodeString_Inactivity(void)
{
    char buf[32];
    CF_GetCondCodeString(buf, INACTIVITY_DETECTED, sizeof(buf));
    UtAssert_StrEq(buf, "INACTIVITY_DETECTED", "GetCondCodeString INACTIVITY_DETECTED");
}

static void Test_GetCondCodeString_SuspendReq(void)
{
    char buf[32];
    CF_GetCondCodeString(buf, SUSPEND_REQUEST_RECEIVED, sizeof(buf));
    UtAssert_StrEq(buf, "SUSPEND_REQ_RCVD", "GetCondCodeString SUSPEND_REQUEST_RECEIVED");
}

static void Test_GetCondCodeString_CancelReq(void)
{
    char buf[32];
    CF_GetCondCodeString(buf, CANCEL_REQUEST_RECEIVED, sizeof(buf));
    UtAssert_StrEq(buf, "CANCEL_REQ_RCVD", "GetCondCodeString CANCEL_REQUEST_RECEIVED");
}

static void Test_GetCondCodeString_Default(void)
{
    char buf[64];
    CF_GetCondCodeString(buf, 999, sizeof(buf));
    /* Default case uses sprintf with "UNEXPECTED %lu" */
    UtAssert_True(strstr(buf, "UNEXPECTED") != NULL,
                  "GetCondCodeString default contains UNEXPECTED");
}

/* ================================================================== */
/* 19. CF_GetPktType                                                   */
/* ================================================================== */
static void Test_GetPktType_Command(void)
{
    /* Bit 12 set => command type (0x1xxx) */
    CFE_SB_MsgId_t CmdMsgId = 0x1800;
    uint32 Result = CF_GetPktType(CmdMsgId);
    UtAssert_True(Result == 1, "GetPktType returns 1 for command MsgId");
}

static void Test_GetPktType_Telemetry(void)
{
    /* Bit 12 not set => telemetry type (0x0xxx) */
    CFE_SB_MsgId_t TlmMsgId = 0x0800;
    uint32 Result = CF_GetPktType(TlmMsgId);
    UtAssert_True(Result == 0, "GetPktType returns 0 for telemetry MsgId");
}

/* ================================================================== */
/* 20. CF_GetResponseChanFromMsgId                                     */
/* ================================================================== */
static void Test_GetResponseChanFromMsgId_Found(void)
{
    /* Set up a message buffer with known MsgId */
    uint8 MsgBuf[CFE_SB_MAX_SB_MSG_SIZE];
    CFE_SB_MsgPtr_t MsgPtr = (CFE_SB_MsgPtr_t)&MsgBuf[0];
    CFE_SB_InitMsg(MsgPtr, 0x1880, sizeof(MsgBuf), TRUE);

    TestTbl.InCh[0].IncomingPDUMsgId = 0x1880;
    TestTbl.InCh[0].OutChanForClass2Response = 1;

    uint8 Result = CF_GetResponseChanFromMsgId(MsgPtr);
    UtAssert_True(Result == 1, "GetResponseChanFromMsgId returns correct channel");
}

static void Test_GetResponseChanFromMsgId_NotFound(void)
{
    uint8 MsgBuf[CFE_SB_MAX_SB_MSG_SIZE];
    CFE_SB_MsgPtr_t MsgPtr = (CFE_SB_MsgPtr_t)&MsgBuf[0];
    CFE_SB_InitMsg(MsgPtr, 0x1999, sizeof(MsgBuf), TRUE);

    TestTbl.InCh[0].IncomingPDUMsgId = 0x1880;

    uint8 Result = CF_GetResponseChanFromMsgId(MsgPtr);
    UtAssert_True(Result == 0xFF, "GetResponseChanFromMsgId returns 0xFF when no match");
}

/* ================================================================== */
/* 21. CF_GetResponseChanFromTransId                                   */
/* ================================================================== */
static void Test_GetResponseChanFromTransId_Found(void)
{
    strncpy(Node1.SrcEntityId, "0.23", CF_MAX_CFG_VALUE_CHARS);
    Node1.TransNum = 5;
    Node1.ChanNum = 1;
    CF_AppData.UpQ[CF_UP_ACTIVEQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    uint8 Result = CF_GetResponseChanFromTransId(CF_UP_ACTIVEQ, "0.23", 5);
    UtAssert_True(Result == 1, "GetResponseChanFromTransId returns correct channel");
}

static void Test_GetResponseChanFromTransId_NotFound(void)
{
    uint8 Result = CF_GetResponseChanFromTransId(CF_UP_ACTIVEQ, "0.23", 99);
    UtAssert_True(Result == 0xFF,
                  "GetResponseChanFromTransId returns 0xFF when not found");
}

static void Test_GetResponseChanFromTransId_TransMatchEntityMismatch(void)
{
    strncpy(Node1.SrcEntityId, "0.23", CF_MAX_CFG_VALUE_CHARS);
    Node1.TransNum = 5;
    Node1.ChanNum = 1;
    CF_AppData.UpQ[CF_UP_ACTIVEQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    uint8 Result = CF_GetResponseChanFromTransId(CF_UP_ACTIVEQ, "0.99", 5);
    UtAssert_True(Result == 0xFF,
                  "GetResponseChanFromTransId returns 0xFF when entity mismatch");
}

/* ================================================================== */
/* 22. CF_FindUpNodeByTransID                                          */
/* ================================================================== */
static void Test_FindUpNodeByTransID_Found(void)
{
    strncpy(Node1.SrcEntityId, "0.23", CF_MAX_CFG_VALUE_CHARS);
    Node1.TransNum = 7;
    strncpy(Node2.SrcEntityId, "0.23", CF_MAX_CFG_VALUE_CHARS);
    Node2.TransNum = 8;
    LinkNodes2(&CF_AppData.UpQ[CF_UP_ACTIVEQ], &Node1, &Node2);

    CF_QueueEntry_t *Result = CF_FindUpNodeByTransID(CF_UP_ACTIVEQ, "0.23", 8);
    UtAssert_True(Result == &Node2, "FindUpNodeByTransID found second node");
}

static void Test_FindUpNodeByTransID_NotFound(void)
{
    CF_QueueEntry_t *Result = CF_FindUpNodeByTransID(CF_UP_ACTIVEQ, "0.23", 99);
    UtAssert_True(Result == NULL, "FindUpNodeByTransID returns NULL when not found");
}

/* ================================================================== */
/* 23. CF_FindPbNodeByTransNum                                         */
/* ================================================================== */
static void Test_FindPbNodeByTransNum_Found(void)
{
    Node1.TransNum = 10;
    Node2.TransNum = 20;
    LinkNodes2(&CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ], &Node1, &Node2);

    CF_QueueEntry_t *Result = CF_FindPbNodeByTransNum(0, CF_PB_ACTIVEQ, 20);
    UtAssert_True(Result == &Node2, "FindPbNodeByTransNum found node");
}

static void Test_FindPbNodeByTransNum_NotFound(void)
{
    CF_QueueEntry_t *Result = CF_FindPbNodeByTransNum(0, CF_PB_ACTIVEQ, 99);
    UtAssert_True(Result == NULL, "FindPbNodeByTransNum returns NULL when not found");
}

/* ================================================================== */
/* 24. CF_FindNodeByName                                               */
/* ================================================================== */
static void Test_FindNodeByName_FoundUplink(void)
{
    strncpy(Node1.SrcFile, "/cf/uptest.dat", OS_MAX_PATH_LEN);
    CF_AppData.UpQ[CF_UP_ACTIVEQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindNodeByName("/cf/uptest.dat");
    UtAssert_True(Result == &Node1, "FindNodeByName found on uplink");
}

static void Test_FindNodeByName_FoundPlayback(void)
{
    TestTbl.OuCh[0].EntryInUse = CF_ENTRY_IN_USE;
    strncpy(Node1.SrcFile, "/cf/pbtest.dat", OS_MAX_PATH_LEN);
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindNodeByName("/cf/pbtest.dat");
    UtAssert_True(Result == &Node1, "FindNodeByName found on playback");
}

static void Test_FindNodeByName_NotFound(void)
{
    CF_QueueEntry_t *Result = CF_FindNodeByName("/cf/nothing.dat");
    UtAssert_True(Result == NULL, "FindNodeByName returns NULL when not found");
}

/* ================================================================== */
/* 25. CF_ValidateFilenameReportErr                                    */
/* ================================================================== */
static void Test_ValidateFilenameReportErr_Valid(void)
{
    int32 Status = CF_ValidateFilenameReportErr("/cf/file.dat", "TestCmd");
    UtAssert_True(Status == CF_SUCCESS, "ValidateFilenameReportErr valid returns SUCCESS");
}

static void Test_ValidateFilenameReportErr_Invalid(void)
{
    int32 Status = CF_ValidateFilenameReportErr("/cf/bad file.dat", "TestCmd");
    UtAssert_True(Status == CF_ERROR, "ValidateFilenameReportErr invalid returns ERROR");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_INV_FILENAME_EID,
                  "Error event sent for invalid filename");
}

/* ================================================================== */
/* 26. CF_FindUpNodeByName (wrapper that checks active then history)   */
/* ================================================================== */
static void Test_FindUpNodeByName_FoundOnActive(void)
{
    strncpy(Node1.SrcFile, "/cf/up.dat", OS_MAX_PATH_LEN);
    CF_AppData.UpQ[CF_UP_ACTIVEQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindUpNodeByName("/cf/up.dat");
    UtAssert_True(Result == &Node1, "FindUpNodeByName found on active");
}

static void Test_FindUpNodeByName_FoundOnHistory(void)
{
    strncpy(Node1.SrcFile, "/cf/uphist.dat", OS_MAX_PATH_LEN);
    CF_AppData.UpQ[CF_UP_HISTORYQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindUpNodeByName("/cf/uphist.dat");
    UtAssert_True(Result == &Node1, "FindUpNodeByName found on history");
}

static void Test_FindUpNodeByName_NotFound(void)
{
    CF_QueueEntry_t *Result = CF_FindUpNodeByName("/cf/nope.dat");
    UtAssert_True(Result == NULL, "FindUpNodeByName returns NULL when not found");
}

/* ================================================================== */
/* 27. CF_FindPbPendingNodeByName                                      */
/* ================================================================== */
static void Test_FindPbPendingNodeByName_Found(void)
{
    TestTbl.OuCh[0].EntryInUse = CF_ENTRY_IN_USE;
    strncpy(Node1.SrcFile, "/cf/pend.dat", OS_MAX_PATH_LEN);
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindPbPendingNodeByName("/cf/pend.dat");
    UtAssert_True(Result == &Node1, "FindPbPendingNodeByName found");
}

/* ================================================================== */
/* 28. CF_FindPbHistoryNodeByName                                      */
/* ================================================================== */
static void Test_FindPbHistoryNodeByName_Found(void)
{
    TestTbl.OuCh[0].EntryInUse = CF_ENTRY_IN_USE;
    strncpy(Node1.SrcFile, "/cf/pbhist.dat", OS_MAX_PATH_LEN);
    CF_AppData.Chan[0].PbQ[CF_PB_HISTORYQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindPbHistoryNodeByName("/cf/pbhist.dat");
    UtAssert_True(Result == &Node1, "FindPbHistoryNodeByName found");
}

/* ================================================================== */
/* 29. CF_FindPbActiveNodeByName - second channel                      */
/* ================================================================== */
static void Test_FindPbActiveNodeByName_SecondChannel(void)
{
    TestTbl.OuCh[1].EntryInUse = CF_ENTRY_IN_USE;
    strncpy(Node1.SrcFile, "/cf/ch1file.dat", OS_MAX_PATH_LEN);
    CF_AppData.Chan[1].PbQ[CF_PB_ACTIVEQ].HeadPtr = &Node1;
    Node1.Next = NULL;

    CF_QueueEntry_t *Result = CF_FindPbActiveNodeByName("/cf/ch1file.dat");
    UtAssert_True(Result == &Node1, "FindPbActiveNodeByName found on second channel");
}

/* ================================================================== */
/* 30. CF_SendEventNoTerm                                              */
/* ================================================================== */
static void Test_SendEventNoTerm(void)
{
    CF_SendEventNoTerm("TestSource");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_NO_TERM_ERR_EID,
                  "SendEventNoTerm sends correct event ID");
}

/* ================================================================== */
/* Registration function                                               */
/* ================================================================== */
void CF_Utils_AddTests(void)
{
    /* CF_FindUpHistoryNodeByName */
    UtTest_Add(Test_FindUpHistoryNodeByName_Found,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindUpHistoryNodeByName - Found");
    UtTest_Add(Test_FindUpHistoryNodeByName_NotFound,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindUpHistoryNodeByName - Not Found");
    UtTest_Add(Test_FindUpHistoryNodeByName_EmptyQueue,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindUpHistoryNodeByName - Empty Queue");

    /* CF_FindUpActiveNodeByName */
    UtTest_Add(Test_FindUpActiveNodeByName_Found,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindUpActiveNodeByName - Found");
    UtTest_Add(Test_FindUpActiveNodeByName_NotFound,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindUpActiveNodeByName - Not Found");

    /* CF_FindPbNodeByName */
    UtTest_Add(Test_FindPbNodeByName_FoundOnActive,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindPbNodeByName - Found on Active");
    UtTest_Add(Test_FindPbNodeByName_FoundOnPending,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindPbNodeByName - Found on Pending");
    UtTest_Add(Test_FindPbNodeByName_FoundOnHistory,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindPbNodeByName - Found on History");
    UtTest_Add(Test_FindPbNodeByName_NotFound,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindPbNodeByName - Not Found");
    UtTest_Add(Test_FindPbNodeByName_ChannelNotInUse,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindPbNodeByName - Channel Not In Use");

    /* CF_FindNodeByTransId */
    UtTest_Add(Test_FindNodeByTransId_UplinkActive,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindNodeByTransId - Uplink Active");
    UtTest_Add(Test_FindNodeByTransId_UplinkHistory,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindNodeByTransId - Uplink History");
    UtTest_Add(Test_FindNodeByTransId_Downlink,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindNodeByTransId - Downlink");
    UtTest_Add(Test_FindNodeByTransId_NotFound,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindNodeByTransId - Not Found");
    UtTest_Add(Test_FindNodeByTransId_InvalidEntityId,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindNodeByTransId - Invalid Entity ID");
    UtTest_Add(Test_FindNodeByTransId_NoUnderscore,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindNodeByTransId - No Underscore");

    /* CF_FindActiveTransIdByName */
    UtTest_Add(Test_FindActiveTransIdByName_FoundUplink,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindActiveTransIdByName - Found Uplink");
    UtTest_Add(Test_FindActiveTransIdByName_FoundPlayback,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindActiveTransIdByName - Found Playback");
    UtTest_Add(Test_FindActiveTransIdByName_NotFound,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindActiveTransIdByName - Not Found");

    /* CF_BuildPutRequest */
    UtTest_Add(Test_BuildPutRequest_Class1,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_BuildPutRequest - Class 1");
    UtTest_Add(Test_BuildPutRequest_Class2,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_BuildPutRequest - Class 2");
    UtTest_Add(Test_BuildPutRequest_EngineError,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_BuildPutRequest - Engine Error");
    UtTest_Add(Test_BuildPutRequest_SrcFileTooLong,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_BuildPutRequest - Src File Too Long");

    /* CF_BuildCmdedRequest */
    UtTest_Add(Test_BuildCmdedRequest_Suspend,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_BuildCmdedRequest - Suspend");
    UtTest_Add(Test_BuildCmdedRequest_Resume,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_BuildCmdedRequest - Resume");
    UtTest_Add(Test_BuildCmdedRequest_Cancel,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_BuildCmdedRequest - Cancel");
    UtTest_Add(Test_BuildCmdedRequest_Abandon,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_BuildCmdedRequest - Abandon");
    UtTest_Add(Test_BuildCmdedRequest_EngineError,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_BuildCmdedRequest - Engine Error");

    /* CF_IncrFaultCtr */
    UtTest_Add(Test_IncrFaultCtr_NoError,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_IncrFaultCtr - NO_ERROR");
    UtTest_Add(Test_IncrFaultCtr_PosAck,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_IncrFaultCtr - POSITIVE_ACK_LIMIT_REACHED");
    UtTest_Add(Test_IncrFaultCtr_FilestoreRejection,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_IncrFaultCtr - FILESTORE_REJECTION");
    UtTest_Add(Test_IncrFaultCtr_FileChecksum,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_IncrFaultCtr - FILE_CHECKSUM_FAILURE");
    UtTest_Add(Test_IncrFaultCtr_FileSize,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_IncrFaultCtr - FILE_SIZE_ERROR");
    UtTest_Add(Test_IncrFaultCtr_NakLimit,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_IncrFaultCtr - NAK_LIMIT_REACHED");
    UtTest_Add(Test_IncrFaultCtr_Inactivity,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_IncrFaultCtr - INACTIVITY_DETECTED");
    UtTest_Add(Test_IncrFaultCtr_Suspend,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_IncrFaultCtr - SUSPEND_REQUEST_RECEIVED");
    UtTest_Add(Test_IncrFaultCtr_Cancel,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_IncrFaultCtr - CANCEL_REQUEST_RECEIVED");
    UtTest_Add(Test_IncrFaultCtr_Default,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_IncrFaultCtr - Default/Unexpected");

    /* CF_ValidateEntityId */
    UtTest_Add(Test_ValidateEntityId_ValidDottedDecimal,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateEntityId - Valid 0.24");
    UtTest_Add(Test_ValidateEntityId_ValidSingleDigits,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateEntityId - Valid 1.2");
    UtTest_Add(Test_ValidateEntityId_ValidMax,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateEntityId - Valid 255.255");
    UtTest_Add(Test_ValidateEntityId_TooShort,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateEntityId - Too Short");
    UtTest_Add(Test_ValidateEntityId_Empty,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateEntityId - Empty");
    UtTest_Add(Test_ValidateEntityId_NoDot,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateEntityId - No Dot");
    UtTest_Add(Test_ValidateEntityId_ValueTooLarge,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateEntityId - First Value > 255");
    UtTest_Add(Test_ValidateEntityId_SecondValueTooLarge,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateEntityId - Second Value > 255");
    UtTest_Add(Test_ValidateEntityId_TooLong,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateEntityId - Too Long");

    /* CF_FileOpenCheck */
    UtTest_Add(Test_FileOpenCheck_FileOpen,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FileOpenCheck - File Open");
    UtTest_Add(Test_FileOpenCheck_FileNotOpen,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FileOpenCheck - File Not Open");
    UtTest_Add(Test_FileOpenCheck_FDValidButPathDiffers,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FileOpenCheck - FD Valid But Path Differs");

    /* CF_CheckIfFileIsActive */
    UtTest_Add(Test_CheckIfFileIsActive_OnUplink,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_CheckIfFileIsActive - On Uplink");
    UtTest_Add(Test_CheckIfFileIsActive_OnPlayback,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_CheckIfFileIsActive - On Playback");
    UtTest_Add(Test_CheckIfFileIsActive_NotActive,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_CheckIfFileIsActive - Not Active");

    /* CF_ValidatePathFile */
    UtTest_Add(Test_ValidatePathFile_Valid,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidatePathFile - Valid");
    UtTest_Add(Test_ValidatePathFile_Empty,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidatePathFile - Empty String");
    UtTest_Add(Test_ValidatePathFile_HasSpace,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidatePathFile - Has Space");
    UtTest_Add(Test_ValidatePathFile_Null,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidatePathFile - NULL");

    /* CF_ValidateSrcPath */
    UtTest_Add(Test_ValidateSrcPath_Valid,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateSrcPath - Valid");
    UtTest_Add(Test_ValidateSrcPath_NoTrailingSlash,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateSrcPath - No Trailing Slash");
    UtTest_Add(Test_ValidateSrcPath_HasSpace,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateSrcPath - Has Space");
    UtTest_Add(Test_ValidateSrcPath_Null,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateSrcPath - NULL");

    /* CF_ValidateDstPath */
    UtTest_Add(Test_ValidateDstPath_Valid,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateDstPath - Valid");
    UtTest_Add(Test_ValidateDstPath_EmptyAllowed,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateDstPath - Empty Allowed");
    UtTest_Add(Test_ValidateDstPath_HasSpace,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateDstPath - Has Space");
    UtTest_Add(Test_ValidateDstPath_Null,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateDstPath - NULL");

    /* CF_ChkTermination */
    UtTest_Add(Test_ChkTermination_Terminated,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ChkTermination - Terminated");
    UtTest_Add(Test_ChkTermination_NotTerminated,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ChkTermination - Not Terminated");
    UtTest_Add(Test_ChkTermination_Null,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ChkTermination - NULL");
    UtTest_Add(Test_ChkTermination_ExactLength,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ChkTermination - Exact Length");

    /* CF_GetStatString */
    UtTest_Add(Test_GetStatString_Unknown,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetStatString - UNKNOWN");
    UtTest_Add(Test_GetStatString_Success,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetStatString - SUCCESS");
    UtTest_Add(Test_GetStatString_Cancelled,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetStatString - CANCELLED");
    UtTest_Add(Test_GetStatString_Abandon,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetStatString - ABANDON");
    UtTest_Add(Test_GetStatString_NoMeta,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetStatString - NO_META");
    UtTest_Add(Test_GetStatString_Pending,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetStatString - PENDING");
    UtTest_Add(Test_GetStatString_AlrdyActive,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetStatString - ALRDY_ACTIVE");
    UtTest_Add(Test_GetStatString_PutReqIssued,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetStatString - PUT_REQ_ISSUED");
    UtTest_Add(Test_GetStatString_PutReqFail,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetStatString - PUT_REQ_FAIL");
    UtTest_Add(Test_GetStatString_Active,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetStatString - ACTIVE");
    UtTest_Add(Test_GetStatString_Default,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetStatString - Default");

    /* CF_GetFinalStatString */
    UtTest_Add(Test_GetFinalStatString_Unknown,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetFinalStatString - UNKNOWN");
    UtTest_Add(Test_GetFinalStatString_Successful,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetFinalStatString - SUCCESSFUL");
    UtTest_Add(Test_GetFinalStatString_Cancelled,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetFinalStatString - CANCELLED");
    UtTest_Add(Test_GetFinalStatString_Abandoned,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetFinalStatString - ABANDONED");
    UtTest_Add(Test_GetFinalStatString_NoMetadata,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetFinalStatString - NO_METADATA");
    UtTest_Add(Test_GetFinalStatString_Default,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetFinalStatString - Default");

    /* CF_GetCondCodeString */
    UtTest_Add(Test_GetCondCodeString_NoError,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetCondCodeString - NO_ERROR");
    UtTest_Add(Test_GetCondCodeString_PosAck,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetCondCodeString - POSITIVE_ACK_LIMIT_REACHED");
    UtTest_Add(Test_GetCondCodeString_FilestoreRej,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetCondCodeString - FILESTORE_REJECTION");
    UtTest_Add(Test_GetCondCodeString_ChecksumFail,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetCondCodeString - FILE_CHECKSUM_FAILURE");
    UtTest_Add(Test_GetCondCodeString_FileSizeErr,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetCondCodeString - FILE_SIZE_ERROR");
    UtTest_Add(Test_GetCondCodeString_NakLimit,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetCondCodeString - NAK_LIMIT_REACHED");
    UtTest_Add(Test_GetCondCodeString_Inactivity,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetCondCodeString - INACTIVITY_DETECTED");
    UtTest_Add(Test_GetCondCodeString_SuspendReq,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetCondCodeString - SUSPEND_REQUEST_RECEIVED");
    UtTest_Add(Test_GetCondCodeString_CancelReq,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetCondCodeString - CANCEL_REQUEST_RECEIVED");
    UtTest_Add(Test_GetCondCodeString_Default,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetCondCodeString - Default");

    /* CF_GetPktType */
    UtTest_Add(Test_GetPktType_Command,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetPktType - Command");
    UtTest_Add(Test_GetPktType_Telemetry,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetPktType - Telemetry");

    /* CF_GetResponseChanFromMsgId */
    UtTest_Add(Test_GetResponseChanFromMsgId_Found,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetResponseChanFromMsgId - Found");
    UtTest_Add(Test_GetResponseChanFromMsgId_NotFound,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetResponseChanFromMsgId - Not Found");

    /* CF_GetResponseChanFromTransId */
    UtTest_Add(Test_GetResponseChanFromTransId_Found,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetResponseChanFromTransId - Found");
    UtTest_Add(Test_GetResponseChanFromTransId_NotFound,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetResponseChanFromTransId - Not Found");
    UtTest_Add(Test_GetResponseChanFromTransId_TransMatchEntityMismatch,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_GetResponseChanFromTransId - Trans Match Entity Mismatch");

    /* CF_FindUpNodeByTransID */
    UtTest_Add(Test_FindUpNodeByTransID_Found,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindUpNodeByTransID - Found");
    UtTest_Add(Test_FindUpNodeByTransID_NotFound,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindUpNodeByTransID - Not Found");

    /* CF_FindPbNodeByTransNum */
    UtTest_Add(Test_FindPbNodeByTransNum_Found,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindPbNodeByTransNum - Found");
    UtTest_Add(Test_FindPbNodeByTransNum_NotFound,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindPbNodeByTransNum - Not Found");

    /* CF_FindNodeByName */
    UtTest_Add(Test_FindNodeByName_FoundUplink,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindNodeByName - Found Uplink");
    UtTest_Add(Test_FindNodeByName_FoundPlayback,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindNodeByName - Found Playback");
    UtTest_Add(Test_FindNodeByName_NotFound,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindNodeByName - Not Found");

    /* CF_ValidateFilenameReportErr */
    UtTest_Add(Test_ValidateFilenameReportErr_Valid,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateFilenameReportErr - Valid");
    UtTest_Add(Test_ValidateFilenameReportErr_Invalid,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_ValidateFilenameReportErr - Invalid");

    /* CF_FindUpNodeByName */
    UtTest_Add(Test_FindUpNodeByName_FoundOnActive,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindUpNodeByName - Found Active");
    UtTest_Add(Test_FindUpNodeByName_FoundOnHistory,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindUpNodeByName - Found History");
    UtTest_Add(Test_FindUpNodeByName_NotFound,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindUpNodeByName - Not Found");

    /* CF_FindPbPendingNodeByName */
    UtTest_Add(Test_FindPbPendingNodeByName_Found,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindPbPendingNodeByName - Found");

    /* CF_FindPbHistoryNodeByName */
    UtTest_Add(Test_FindPbHistoryNodeByName_Found,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindPbHistoryNodeByName - Found");

    /* CF_FindPbActiveNodeByName - second channel */
    UtTest_Add(Test_FindPbActiveNodeByName_SecondChannel,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_FindPbActiveNodeByName - Second Channel");

    /* CF_SendEventNoTerm */
    UtTest_Add(Test_SendEventNoTerm,
               UtilsTestSetup, UtilsTestTeardown,
               "CF_SendEventNoTerm - Sends Event");
}
