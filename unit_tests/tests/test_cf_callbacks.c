/*
 * Unit tests for cf_callbacks.c
 *
 * Comprehensive coverage of all callback functions registered with the
 * CFDP engine: indication handler, PDU output, filestore operations,
 * event wrappers, temporary file wrappers, and queue sort.
 */
#include "test_framework.h"
#include "cfe_stubs.h"
#include "cfdp_stubs.h"
#include "cf_app.h"
#include "cf_callbacks.h"
#include "cf_events.h"
#include "cf_defs.h"
#include <string.h>

/* Externs required by cf_callbacks.c */
extern CF_AppData_t CF_AppData;
extern uint32       CF_AutoSuspendCnt;
extern uint32       CF_AutoSuspendArray[];

/* A config table instance for tests that need CF_AppData.Tbl */
static cf_config_table_t TestConfigTable;

/* Dummy message buffer to ensure CF_AppData.MsgPtr is never NULL.
 * CF_Indication -> IND_MACHINE_ALLOCATED calls
 * CF_GetResponseChanFromMsgId(CF_AppData.MsgPtr) which dereferences it. */
static uint8 DummyMsgBuf[64];

/* ------------------------------------------------------------------ */
/* Common Setup / Teardown                                             */
/* ------------------------------------------------------------------ */
static void Setup(void)
{
    CFE_Stubs_Reset();
    CFDP_Stubs_Reset();
    memset(&CF_AppData, 0, sizeof(CF_AppData));
    memset(&TestConfigTable, 0, sizeof(TestConfigTable));
    CF_AppData.Tbl = &TestConfigTable;
    CF_AutoSuspendCnt = 0;
    memset(CF_AutoSuspendArray, 0, sizeof(uint32) * CF_AUTOSUSPEND_MAX_TRANS);
    /* Provide a valid MsgPtr so functions that read from it don't segfault */
    memset(DummyMsgBuf, 0, sizeof(DummyMsgBuf));
    CF_AppData.MsgPtr = (CFE_SB_MsgPtr_t)DummyMsgBuf;
    /* Mark channel 0 as in-use so CF_GetChanNumFromTransId finds entries.
     * Without this, functions that search playback queues return CF_ERROR
     * and the code can dereference NULL pointers. */
    TestConfigTable.OuCh[0].EntryInUse = CF_ENTRY_IN_USE;
}

/* ------------------------------------------------------------------ */
/* Helper: build a zeroed TRANS_STATUS with basic fields populated     */
/* ------------------------------------------------------------------ */
static TRANS_STATUS MakeTransStatus(void)
{
    TRANS_STATUS ts;
    memset(&ts, 0, sizeof(ts));
    ts.trans.source_id.length   = 2;
    ts.trans.source_id.value[0] = 1;
    ts.trans.source_id.value[1] = 23;
    ts.trans.number             = 100;
    strncpy(ts.md.source_file_name, "/src/file.dat", MAX_FILE_NAME_LENGTH);
    strncpy(ts.md.dest_file_name,   "/dst/file.dat", MAX_FILE_NAME_LENGTH);
    ts.md.file_size = 4096;
    return ts;
}

/* ================================================================== */
/* 1. CF_RegisterCallbacks                                             */
/* ================================================================== */
static void Test_RegisterCallbacks_Nominal(void)
{
    CF_RegisterCallbacks();

    UtAssert_True(CFDP_Stubs.register_indication_call_count == 1,
                  "register_indication called once");
    UtAssert_True(CFDP_Stubs.register_pdu_output_open_call_count == 1,
                  "register_pdu_output_open called once");
    UtAssert_True(CFDP_Stubs.register_pdu_output_ready_call_count == 1,
                  "register_pdu_output_ready called once");
    UtAssert_True(CFDP_Stubs.register_pdu_output_send_call_count == 1,
                  "register_pdu_output_send called once");
    UtAssert_True(CFDP_Stubs.register_printf_debug_call_count == 1,
                  "register_printf_debug called once");
    UtAssert_True(CFDP_Stubs.register_printf_info_call_count == 1,
                  "register_printf_info called once");
    UtAssert_True(CFDP_Stubs.register_printf_warning_call_count == 1,
                  "register_printf_warning called once");
    UtAssert_True(CFDP_Stubs.register_printf_error_call_count == 1,
                  "register_printf_error called once");
    UtAssert_True(CFDP_Stubs.register_file_size_call_count == 1,
                  "register_file_size called once");
    UtAssert_True(CFDP_Stubs.register_rename_call_count == 1,
                  "register_rename called once");
    UtAssert_True(CFDP_Stubs.register_remove_call_count == 1,
                  "register_remove called once");
    UtAssert_True(CFDP_Stubs.register_fseek_call_count == 1,
                  "register_fseek called once");
    UtAssert_True(CFDP_Stubs.register_fopen_call_count == 1,
                  "register_fopen called once");
    UtAssert_True(CFDP_Stubs.register_fread_call_count == 1,
                  "register_fread called once");
    UtAssert_True(CFDP_Stubs.register_fwrite_call_count == 1,
                  "register_fwrite called once");
    UtAssert_True(CFDP_Stubs.register_fclose_call_count == 1,
                  "register_fclose called once");
}

/* ================================================================== */
/* 2. CF_Indication - IND_TRANSACTION (outgoing trans start, no-op)    */
/* ================================================================== */
static void Test_Indication_Transaction(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    CF_Indication(IND_TRANSACTION, ts);
    /* IND_TRANSACTION is a no-op break -- verify no event sent */
    UtAssert_True(CFE_Stubs.CFE_EVS_SendEvent_CallCount == 0,
                  "IND_TRANSACTION: no event sent");
}

/* ================================================================== */
/* 3. CF_Indication - IND_MACHINE_ALLOCATED, receiver (class 1)        */
/* ================================================================== */
static void Test_Indication_MachAllocClass1Rcv(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.role = CLASS_1_RECEIVER;

    /* CF_AllocQueueEntry is from cf_utils.c which is compiled in.
     * It uses CFE_ES_GetPoolBuf; set up so it succeeds. */
    CFE_Stubs.CFE_ES_GetPoolBuf_Return = CFE_STUBS_POOL_BUF_SIZE;

    CF_Indication(IND_MACHINE_ALLOCATED, ts);

    /* Verify no error event was sent (alloc should succeed) */
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID != CF_MACH_ALLOC_ERR_EID,
                  "MachAlloc Class1Rcv: no alloc error event");
}

/* ================================================================== */
/* 4. CF_Indication - IND_MACHINE_ALLOCATED, receiver (class 2)        */
/* ================================================================== */
static void Test_Indication_MachAllocClass2Rcv(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.role = CLASS_2_RECEIVER;

    CFE_Stubs.CFE_ES_GetPoolBuf_Return = CFE_STUBS_POOL_BUF_SIZE;

    CF_Indication(IND_MACHINE_ALLOCATED, ts);

    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID != CF_MACH_ALLOC_ERR_EID,
                  "MachAlloc Class2Rcv: no alloc error event");
}

/* ================================================================== */
/* 5. CF_Indication - IND_MACHINE_ALLOCATED, alloc failure             */
/* ================================================================== */
static void Test_Indication_MachAllocFail(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.role = CLASS_1_RECEIVER;

    /* Force pool allocation to fail */
    CFE_Stubs.CFE_ES_GetPoolBuf_Return = -1;

    CF_Indication(IND_MACHINE_ALLOCATED, ts);

    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_MACH_ALLOC_ERR_EID,
                  "MachAlloc Fail: alloc error event sent");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_ERROR,
                  "MachAlloc Fail: event type is ERROR");
}

/* ================================================================== */
/* 6. CF_Indication - IND_MACHINE_ALLOCATED, sender (file-send)        */
/* ================================================================== */
static void Test_Indication_MachAllocSender(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.role = CLASS_1_SENDER;

    /* For sender path, CF_FindNodeAtFrontOfQueue returns NULL when no
     * node is queued. This takes the #ifdef CF_DEBUG else path (no-op). */
    CF_Indication(IND_MACHINE_ALLOCATED, ts);

    /* No outgoing trans start event because node was not found */
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID != CF_OUT_TRANS_START_EID,
                  "MachAlloc Sender no node: no start event");
}

/* ================================================================== */
/* 7. CF_Indication - IND_METADATA_SENT (no-op)                       */
/* ================================================================== */
static void Test_Indication_MetadataSent(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    CF_Indication(IND_METADATA_SENT, ts);

    UtAssert_True(CFE_Stubs.CFE_EVS_SendEvent_CallCount == 0,
                  "IND_METADATA_SENT: no event sent");
}

/* ================================================================== */
/* 8. CF_Indication - IND_METADATA_RECV                                */
/* ================================================================== */
static void Test_Indication_MetadataRecv(void)
{
    TRANS_STATUS ts = MakeTransStatus();

    CF_Indication(IND_METADATA_RECV, ts);

    UtAssert_True(CF_AppData.Hk.Up.MetaCount == 1,
                  "IND_METADATA_RECV: MetaCount incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_IN_TRANS_START_EID,
                  "IND_METADATA_RECV: incoming trans start event");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_INFORMATION,
                  "IND_METADATA_RECV: event type is INFORMATION");
}

/* ================================================================== */
/* 9. CF_Indication - IND_EOF_SENT, auto-suspend disabled              */
/* ================================================================== */
static void Test_Indication_EofSent_AutoSuspendDisabled(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.role = CLASS_1_SENDER;

    /* CF_GetChanNumFromTransId will not find it (no queued entries),
     * returns CF_ERROR, so the code takes the CF_DEBUG else path */
    CF_AppData.Hk.AutoSuspend.EnFlag = CF_DISABLED;

    CF_Indication(IND_EOF_SENT, ts);

    /* No suspend overflow event */
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID != CF_TRANS_SUSPEND_OVRFLW_EID,
                  "EOF_SENT no auto-suspend: no overflow event");
}

/* ================================================================== */
/* 10. CF_Indication - IND_EOF_SENT, auto-suspend enabled, overflow    */
/* ================================================================== */
static void Test_Indication_EofSent_AutoSuspendOverflow(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.role = CLASS_1_SENDER;

    /* Set up a pending-queue entry so CF_GetChanNumFromTransId
     * can find channel 0 for this transaction */
    CF_QueueEntry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.TransNum = 100;
    entry.ChanNum  = 0;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].TailPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].EntryCnt = 1;

    CF_AppData.Hk.AutoSuspend.EnFlag = CF_ENABLED;
    /* Fill the auto-suspend array to max */
    CF_AutoSuspendCnt = CF_AUTOSUSPEND_MAX_TRANS;

    CF_Indication(IND_EOF_SENT, ts);

    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_TRANS_SUSPEND_OVRFLW_EID,
                  "EOF_SENT auto-suspend overflow: overflow event");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_ERROR,
                  "EOF_SENT auto-suspend overflow: event type ERROR");
}

/* ================================================================== */
/* 11. CF_Indication - IND_EOF_SENT, auto-suspend enabled, nominal     */
/* ================================================================== */
static void Test_Indication_EofSent_AutoSuspendNominal(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.role = CLASS_1_SENDER;

    CF_QueueEntry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.TransNum = 100;
    entry.ChanNum  = 0;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].TailPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].EntryCnt = 1;

    CF_AppData.Hk.AutoSuspend.EnFlag = CF_ENABLED;
    CF_AppData.Hk.AutoSuspend.LowFreeMark = 999;
    CF_AutoSuspendCnt = 0;

    CF_Indication(IND_EOF_SENT, ts);

    UtAssert_True(CF_AutoSuspendCnt == 1,
                  "EOF_SENT auto-suspend: count incremented");
    UtAssert_True(CF_AutoSuspendArray[0] == 100,
                  "EOF_SENT auto-suspend: trans num stored");
    /* DataBlast should be cleared */
    UtAssert_True(CF_AppData.Chan[0].DataBlast == CF_NOT_IN_PROGRESS,
                  "EOF_SENT: DataBlast cleared");
    UtAssert_True(CF_AppData.Chan[0].TransNumBlasting == 0,
                  "EOF_SENT: TransNumBlasting cleared");
}

/* ================================================================== */
/* 12. CF_Indication - IND_EOF_RECV (no-op)                            */
/* ================================================================== */
static void Test_Indication_EofRecv(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    CF_Indication(IND_EOF_RECV, ts);
    UtAssert_True(CFE_Stubs.CFE_EVS_SendEvent_CallCount == 0,
                  "IND_EOF_RECV: no event sent");
}

/* ================================================================== */
/* 13. CF_Indication - IND_TRANSACTION_FINISHED (no-op)                */
/* ================================================================== */
static void Test_Indication_TransFinished(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    CF_Indication(IND_TRANSACTION_FINISHED, ts);
    UtAssert_True(CFE_Stubs.CFE_EVS_SendEvent_CallCount == 0,
                  "IND_TRANSACTION_FINISHED: no event sent");
}

/* ================================================================== */
/* 14. CF_Indication - IND_MACHINE_DEALLOCATED, success, receiver      */
/* ================================================================== */
static void Test_Indication_MachDeallocSuccessReceiver(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.final_status = FINAL_STATUS_SUCCESSFUL;
    ts.role = CLASS_1_RECEIVER;

    CF_Indication(IND_MACHINE_DEALLOCATED, ts);

    UtAssert_True(CF_AppData.Hk.Up.SuccessCounter == 1,
                  "MachDealloc success rcv: SuccessCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_IN_TRANS_OK_EID,
                  "MachDealloc success rcv: IN_TRANS_OK event");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_INFORMATION,
                  "MachDealloc success rcv: event type INFORMATION");
}

/* ================================================================== */
/* 15. CF_Indication - IND_MACHINE_DEALLOCATED, success, class 2 rcv   */
/* ================================================================== */
static void Test_Indication_MachDeallocSuccessClass2Rcv(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.final_status = FINAL_STATUS_SUCCESSFUL;
    ts.role = CLASS_2_RECEIVER;

    CF_Indication(IND_MACHINE_DEALLOCATED, ts);

    UtAssert_True(CF_AppData.Hk.Up.SuccessCounter == 1,
                  "MachDealloc success class2 rcv: SuccessCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_IN_TRANS_OK_EID,
                  "MachDealloc success class2 rcv: IN_TRANS_OK event");
}

/* ================================================================== */
/* 16. CF_Indication - IND_MACHINE_DEALLOCATED, success, sender        */
/* ================================================================== */
static void Test_Indication_MachDeallocSuccessSender(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.final_status = FINAL_STATUS_SUCCESSFUL;
    ts.role = CLASS_1_SENDER;

    CF_QueueEntry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.TransNum = 100;
    entry.ChanNum  = 0;
    entry.Preserve = CF_KEEP_FILE;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].TailPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].EntryCnt = 1;

    CF_Indication(IND_MACHINE_DEALLOCATED, ts);

    UtAssert_True(CF_AppData.Hk.Chan[0].SuccessCounter == 1,
                  "MachDealloc success sender: SuccessCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_OUT_TRANS_OK_EID,
                  "MachDealloc success sender: OUT_TRANS_OK event");
}

/* ================================================================== */
/* 17. CF_Indication - IND_MACHINE_DEALLOCATED, fail, receiver         */
/* ================================================================== */
static void Test_Indication_MachDeallocFailReceiver(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.final_status  = FINAL_STATUS_CANCELLED;
    ts.condition_code = CANCEL_REQUEST_RECEIVED;
    ts.role = CLASS_1_RECEIVER;

    CF_Indication(IND_MACHINE_DEALLOCATED, ts);

    UtAssert_True(CF_AppData.Hk.Up.FailedCounter == 1,
                  "MachDealloc fail rcv: FailedCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_IN_TRANS_FAILED_EID,
                  "MachDealloc fail rcv: IN_TRANS_FAILED event");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_ERROR,
                  "MachDealloc fail rcv: event type ERROR");
}

/* ================================================================== */
/* 18. CF_Indication - IND_MACHINE_DEALLOCATED, fail, class 2 receiver */
/* ================================================================== */
static void Test_Indication_MachDeallocFailClass2Receiver(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.final_status  = FINAL_STATUS_ABANDONED;
    ts.condition_code = INACTIVITY_DETECTED;
    ts.role = CLASS_2_RECEIVER;

    CF_Indication(IND_MACHINE_DEALLOCATED, ts);

    UtAssert_True(CF_AppData.Hk.Up.FailedCounter == 1,
                  "MachDealloc fail class2 rcv: FailedCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_IN_TRANS_FAILED_EID,
                  "MachDealloc fail class2 rcv: IN_TRANS_FAILED event");
}

/* ================================================================== */
/* 19. CF_Indication - IND_MACHINE_DEALLOCATED, fail, sender           */
/* ================================================================== */
static void Test_Indication_MachDeallocFailSender(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.final_status  = FINAL_STATUS_CANCELLED;
    ts.condition_code = NAK_LIMIT_REACHED;
    ts.role = CLASS_1_SENDER;

    /* Put an entry so CF_GetChanNumFromTransId can find channel 0 */
    CF_QueueEntry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.TransNum = 100;
    entry.ChanNum  = 0;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].TailPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].EntryCnt = 1;

    CF_Indication(IND_MACHINE_DEALLOCATED, ts);

    UtAssert_True(CF_AppData.Hk.Chan[0].FailedCounter == 1,
                  "MachDealloc fail sender: FailedCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_OUT_TRANS_FAILED_EID,
                  "MachDealloc fail sender: OUT_TRANS_FAILED event");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_ERROR,
                  "MachDealloc fail sender: event type ERROR");
}

/* ================================================================== */
/* 20. CF_Indication - IND_MACHINE_DEALLOCATED, fail sender, bad chan  */
/* ================================================================== */
static void Test_Indication_MachDeallocFailSenderBadChan(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.final_status  = FINAL_STATUS_CANCELLED;
    ts.role = CLASS_1_SENDER;

    /* No queue entry means CF_GetChanNumFromTransId returns CF_ERROR
     * which is >= CF_MAX_PLAYBACK_CHANNELS (as unsigned), so the
     * function returns early */
    CF_Indication(IND_MACHINE_DEALLOCATED, ts);

    /* FailedCounter should NOT be incremented for invalid channel */
    UtAssert_True(CF_AppData.Hk.Chan[0].FailedCounter == 0,
                  "MachDealloc fail sender bad chan: no counter increment");
}

/* ================================================================== */
/* 21. CF_Indication - IND_MACHINE_DEALLOCATED, fail sender, blasting  */
/* ================================================================== */
static void Test_Indication_MachDeallocFailSenderBlasting(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.final_status  = FINAL_STATUS_CANCELLED;
    ts.condition_code = NAK_LIMIT_REACHED;
    ts.role = CLASS_1_SENDER;

    CF_QueueEntry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.TransNum = 100;
    entry.ChanNum  = 0;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].TailPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].EntryCnt = 1;

    /* Simulate blasting in progress for this trans */
    CF_AppData.Chan[0].DataBlast = CF_IN_PROGRESS;
    CF_AppData.Chan[0].TransNumBlasting = 100;
    TestConfigTable.OuCh[0].DequeueEnable = CF_ENABLED;

    CF_Indication(IND_MACHINE_DEALLOCATED, ts);

    UtAssert_True(CF_AppData.Chan[0].DataBlast == CF_NOT_IN_PROGRESS,
                  "MachDealloc fail sender blasting: DataBlast cleared");
    UtAssert_True(CF_AppData.Chan[0].TransNumBlasting == 0,
                  "MachDealloc fail sender blasting: TransNumBlasting cleared");
}

/* ================================================================== */
/* 22. CF_Indication - IND_ACK_TIMER_EXPIRED                           */
/* ================================================================== */
static void Test_Indication_AckTimerExpired(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    CF_Indication(IND_ACK_TIMER_EXPIRED, ts);

    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_IND_ACK_TIM_EXP_EID,
                  "ACK_TIMER_EXPIRED: correct event ID");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_INFORMATION,
                  "ACK_TIMER_EXPIRED: event type INFORMATION");
}

/* ================================================================== */
/* 23. CF_Indication - IND_INACTIVITY_TIMER_EXPIRED                    */
/* ================================================================== */
static void Test_Indication_InactivityTimerExpired(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    CF_Indication(IND_INACTIVITY_TIMER_EXPIRED, ts);

    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_IND_INA_TIM_EXP_EID,
                  "INACTIVITY_TIMER_EXPIRED: correct event ID");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_INFORMATION,
                  "INACTIVITY_TIMER_EXPIRED: event type INFORMATION");
}

/* ================================================================== */
/* 24. CF_Indication - IND_NAK_TIMER_EXPIRED                           */
/* ================================================================== */
static void Test_Indication_NakTimerExpired(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    CF_Indication(IND_NAK_TIMER_EXPIRED, ts);

    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_IND_NACK_TIM_EXP_EID,
                  "NAK_TIMER_EXPIRED: correct event ID");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_INFORMATION,
                  "NAK_TIMER_EXPIRED: event type INFORMATION");
}

/* ================================================================== */
/* 25. CF_Indication - IND_SUSPENDED                                   */
/* ================================================================== */
static void Test_Indication_Suspended(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    CF_Indication(IND_SUSPENDED, ts);

    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_IND_XACT_SUS_EID,
                  "SUSPENDED: correct event ID");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_INFORMATION,
                  "SUSPENDED: event type INFORMATION");
}

/* ================================================================== */
/* 26. CF_Indication - IND_RESUMED                                     */
/* ================================================================== */
static void Test_Indication_Resumed(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    CF_Indication(IND_RESUMED, ts);

    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_IND_XACT_RES_EID,
                  "RESUMED: correct event ID");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_INFORMATION,
                  "RESUMED: event type INFORMATION");
}

/* ================================================================== */
/* 27. CF_Indication - IND_REPORT (no-op)                              */
/* ================================================================== */
static void Test_Indication_Report(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    CF_Indication(IND_REPORT, ts);
    UtAssert_True(CFE_Stubs.CFE_EVS_SendEvent_CallCount == 0,
                  "IND_REPORT: no event sent");
}

/* ================================================================== */
/* 28. CF_Indication - IND_FAULT                                       */
/* ================================================================== */
static void Test_Indication_Fault(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.condition_code = FILE_CHECKSUM_FAILURE;
    CF_Indication(IND_FAULT, ts);

    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_IND_XACT_FAU_EID,
                  "FAULT: correct event ID");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_DEBUG,
                  "FAULT: event type DEBUG");
}

/* ================================================================== */
/* 29. CF_Indication - IND_ABANDONED                                   */
/* ================================================================== */
static void Test_Indication_Abandoned(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    CF_Indication(IND_ABANDONED, ts);

    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_IND_XACT_ABA_EID,
                  "ABANDONED: correct event ID");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_INFORMATION,
                  "ABANDONED: event type INFORMATION");
    UtAssert_True(CF_AppData.Hk.App.TotalAbandonTrans == 1,
                  "ABANDONED: TotalAbandonTrans incremented");
}

/* ================================================================== */
/* 30. CF_Indication - IND_FILE_SEGMENT_RECV (no-op)                   */
/* ================================================================== */
static void Test_Indication_FileSegmentRecv(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    CF_Indication(IND_FILE_SEGMENT_RECV, ts);
    /* Default case sends an unexpected indication event */
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_IND_UNEXP_TYPE_EID,
                  "FILE_SEGMENT_RECV: unexpected indication event");
}

/* ================================================================== */
/* 31. CF_Indication - IND_FILE_SEGMENT_SENT (no-op)                   */
/* ================================================================== */
static void Test_Indication_FileSegmentSent(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    CF_Indication(IND_FILE_SEGMENT_SENT, ts);
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_IND_UNEXP_TYPE_EID,
                  "FILE_SEGMENT_SENT: unexpected indication event");
}

/* ================================================================== */
/* 32. CF_Indication - default (unexpected type)                       */
/* ================================================================== */
static void Test_Indication_DefaultUnexpected(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    /* Use a value beyond the enum range */
    CF_Indication((INDICATION_TYPE)999, ts);

    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_IND_UNEXP_TYPE_EID,
                  "Default: unexpected indication event");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_INFORMATION,
                  "Default: event type INFORMATION");
}

/* ================================================================== */
/* 33. CF_PduOutputOpen - nominal (returns YES)                        */
/* ================================================================== */
static void Test_PduOutputOpen_Nominal(void)
{
    ID src, dst;
    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));
    boolean result = CF_PduOutputOpen(src, dst);
    UtAssert_True(result == YES,
                  "PduOutputOpen: returns YES");
}

/* ================================================================== */
/* 34. CF_PduOutputReady - semaphore invalid (always green light)      */
/* ================================================================== */
static void Test_PduOutputReady_SemInvalid(void)
{
    TRANSACTION trans;
    ID dst;
    memset(&trans, 0, sizeof(trans));
    memset(&dst, 0, sizeof(dst));

    /* Set flight entity ID to match source so it's a playback trans */
    strncpy(CF_AppData.Hk.Eng.FlightEngineEntityId, "0.0",
            CF_MAX_CFG_VALUE_CHARS);
    trans.source_id.value[0] = 0;
    trans.source_id.value[1] = 0;
    trans.number = 100;

    /* Put an entry on PB active queue */
    CF_QueueEntry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.TransNum = 100;
    entry.ChanNum  = 0;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].TailPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].EntryCnt = 1;

    /* Semaphore invalid => always ready */
    CF_AppData.Chan[0].HandshakeSemId = CF_INVALID;

    boolean result = CF_PduOutputReady(FILE_DIR_PDU, trans, dst);

    UtAssert_True(result == TRUE,
                  "PduOutputReady SemInvalid: returns TRUE");
    UtAssert_True(CF_AppData.Hk.Chan[0].GreenLightCntr == 1,
                  "PduOutputReady SemInvalid: GreenLightCntr incremented");
}

/* ================================================================== */
/* 35. CF_PduOutputReady - semaphore success (green light)             */
/* ================================================================== */
static void Test_PduOutputReady_SemGreenLight(void)
{
    TRANSACTION trans;
    ID dst;
    memset(&trans, 0, sizeof(trans));
    memset(&dst, 0, sizeof(dst));

    strncpy(CF_AppData.Hk.Eng.FlightEngineEntityId, "0.0",
            CF_MAX_CFG_VALUE_CHARS);
    trans.source_id.value[0] = 0;
    trans.source_id.value[1] = 0;
    trans.number = 100;

    CF_QueueEntry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.TransNum = 100;
    entry.ChanNum  = 0;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].TailPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].EntryCnt = 1;

    CF_AppData.Chan[0].HandshakeSemId = 1; /* valid */
    CFE_Stubs.OS_CountSemTimedWait_Return = OS_SUCCESS;

    boolean result = CF_PduOutputReady(FILE_DIR_PDU, trans, dst);

    UtAssert_True(result == TRUE,
                  "PduOutputReady GreenLight: returns TRUE");
    UtAssert_True(CF_AppData.Hk.Chan[0].GreenLightCntr == 1,
                  "PduOutputReady GreenLight: GreenLightCntr incremented");
}

/* ================================================================== */
/* 36. CF_PduOutputReady - semaphore fail (red light)                  */
/* ================================================================== */
static void Test_PduOutputReady_SemRedLight(void)
{
    TRANSACTION trans;
    ID dst;
    memset(&trans, 0, sizeof(trans));
    memset(&dst, 0, sizeof(dst));

    strncpy(CF_AppData.Hk.Eng.FlightEngineEntityId, "0.0",
            CF_MAX_CFG_VALUE_CHARS);
    trans.source_id.value[0] = 0;
    trans.source_id.value[1] = 0;
    trans.number = 100;

    CF_QueueEntry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.TransNum = 100;
    entry.ChanNum  = 0;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].TailPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].EntryCnt = 1;

    CF_AppData.Chan[0].HandshakeSemId = 1;
    CFE_Stubs.OS_CountSemTimedWait_Return = OS_ERROR;

    boolean result = CF_PduOutputReady(FILE_DIR_PDU, trans, dst);

    UtAssert_True(result == FALSE,
                  "PduOutputReady RedLight: returns FALSE");
    UtAssert_True(CF_AppData.Hk.Chan[0].RedLightCntr == 1,
                  "PduOutputReady RedLight: RedLightCntr incremented");
}

/* ================================================================== */
/* 37. CF_PduOutputReady - trans not found (returns TRUE anyway)       */
/* ================================================================== */
static void Test_PduOutputReady_TransNotFound(void)
{
    TRANSACTION trans;
    ID dst;
    memset(&trans, 0, sizeof(trans));
    memset(&dst, 0, sizeof(dst));

    strncpy(CF_AppData.Hk.Eng.FlightEngineEntityId, "0.0",
            CF_MAX_CFG_VALUE_CHARS);
    trans.source_id.value[0] = 0;
    trans.source_id.value[1] = 0;
    trans.number = 999; /* not found in any queue */

    boolean result = CF_PduOutputReady(FILE_DIR_PDU, trans, dst);

    /* When both active and history queue searches fail, returns TRUE
     * so the engine can release the undirected PDU */
    UtAssert_True(result == TRUE,
                  "PduOutputReady TransNotFound: returns TRUE");
}

/* ================================================================== */
/* 38. CF_PduOutputSend - channel not found (drops PDU)                */
/* ================================================================== */
static void Test_PduOutputSend_ChanNotFound(void)
{
    TRANSACTION trans;
    ID dst;
    CFDP_DATA pdu;
    memset(&trans, 0, sizeof(trans));
    memset(&dst, 0, sizeof(dst));
    memset(&pdu, 0, sizeof(pdu));

    pdu.length = 10;
    /* direction bit clear => file-send direction */
    pdu.content[0] = 0; /* bit 3 clear */
    trans.number = 999; /* not found */

    CF_PduOutputSend(trans, dst, &pdu);

    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_OUT_SND_ERR1_EID,
                  "PduOutputSend ChanNotFound: ERR1 event");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_ERROR,
                  "PduOutputSend ChanNotFound: event type ERROR");
}

/* ================================================================== */
/* 39. CF_PduOutputSend - nominal send (tlm pkt, variable size)        */
/* ================================================================== */
static void Test_PduOutputSend_Nominal(void)
{
    TRANSACTION trans;
    ID dst;
    CFDP_DATA pdu;
    memset(&trans, 0, sizeof(trans));
    memset(&dst, 0, sizeof(dst));
    memset(&pdu, 0, sizeof(pdu));

    pdu.length = 20;
    pdu.content[0] = 0; /* direction toward file receiver */
    trans.number = 100;

    CF_QueueEntry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.TransNum = 100;
    entry.ChanNum  = 0;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].TailPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].EntryCnt = 1;

    /* OutgoingPduMsgId with bit 12 clear => TLM pkt */
    TestConfigTable.OuCh[0].OutgoingPduMsgId = 0x0800;

    CF_PduOutputSend(trans, dst, &pdu);

    UtAssert_True(CFE_Stubs.CFE_SB_ZeroCopyGetPtr_CallCount == 1,
                  "PduOutputSend Nominal: ZeroCopyGetPtr called");
    UtAssert_True(CFE_Stubs.CFE_SB_ZeroCopySend_CallCount == 1,
                  "PduOutputSend Nominal: ZeroCopySend called");
    UtAssert_True(CF_AppData.Hk.Chan[0].PDUsSent == 1,
                  "PduOutputSend Nominal: PDUsSent incremented");
}

/* ================================================================== */
/* 40. CF_PduOutputSend - direction bit set (class 2 uplink response)  */
/* ================================================================== */
static void Test_PduOutputSend_UplinkResponse(void)
{
    TRANSACTION trans;
    ID dst;
    CFDP_DATA pdu;
    memset(&trans, 0, sizeof(trans));
    memset(&dst, 0, sizeof(dst));
    memset(&pdu, 0, sizeof(pdu));

    pdu.length = 20;
    /* Set direction bit (bit 3) to indicate "toward file sender" */
    pdu.content[0] = (1 << CF_PDUHDR_DIRECTION_BIT);
    trans.source_id.value[0] = 1;
    trans.source_id.value[1] = 23;
    trans.number = 100;

    /* Put an entry in the uplink active queue so
     * CF_GetResponseChanFromTransId can find it */
    CF_QueueEntry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.TransNum = 100;
    entry.ChanNum  = 0;
    sprintf(entry.SrcEntityId, "%d.%d", 1, 23);
    CF_AppData.UpQ[CF_UP_ACTIVEQ].HeadPtr = &entry;
    CF_AppData.UpQ[CF_UP_ACTIVEQ].TailPtr = &entry;
    CF_AppData.UpQ[CF_UP_ACTIVEQ].EntryCnt = 1;

    TestConfigTable.OuCh[0].OutgoingPduMsgId = 0x0800;

    CF_PduOutputSend(trans, dst, &pdu);

    UtAssert_True(CFE_Stubs.CFE_SB_ZeroCopyGetPtr_CallCount == 1,
                  "PduOutputSend uplink: ZeroCopyGetPtr called");
}

/* ================================================================== */
/* 41. CF_RenameFile - success                                         */
/* ================================================================== */
static void Test_RenameFile_Success(void)
{
    CFE_Stubs.OS_open_Return = 5;
    CFE_Stubs.OS_creat_Return = 6;
    CFE_Stubs.OS_read_Return = 0; /* 0 = EOF immediately */
    CFE_Stubs.OS_remove_Return = OS_SUCCESS;

    int result = CF_RenameFile("/tmp/old.dat", "/dst/new.dat");

    UtAssert_True(result == 0,
                  "RenameFile Success: returns 0");
}

/* ================================================================== */
/* 42. CF_RenameFile - open failure                                    */
/* ================================================================== */
static void Test_RenameFile_OpenFail(void)
{
    CFE_Stubs.OS_open_Return = -1; /* OS_open fails */
    CFE_Stubs.OS_creat_Return = 6;

    int result = CF_RenameFile("/tmp/old.dat", "/dst/new.dat");

    UtAssert_True(result == 1,
                  "RenameFile OpenFail: returns 1");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_FILE_IO_ERR1_EID,
                  "RenameFile OpenFail: FILE_IO_ERR1 event");
}

/* ================================================================== */
/* 43. CF_RenameFile - creat failure                                   */
/* ================================================================== */
static void Test_RenameFile_CreatFail(void)
{
    CFE_Stubs.OS_open_Return = 5;
    CFE_Stubs.OS_creat_Return = -1; /* creat fails */

    int result = CF_RenameFile("/tmp/old.dat", "/dst/new.dat");

    UtAssert_True(result == 1,
                  "RenameFile CreatFail: returns 1");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_FILE_IO_ERR2_EID,
                  "RenameFile CreatFail: FILE_IO_ERR2 event");
}

/* ================================================================== */
/* 44. CF_RemoveFile - success                                         */
/* ================================================================== */
static void Test_RemoveFile_Success(void)
{
    CFE_Stubs.OS_remove_Return = OS_SUCCESS;
    int result = CF_RemoveFile("/tmp/file.dat");

    UtAssert_True(result == 0,
                  "RemoveFile Success: returns 0");
}

/* ================================================================== */
/* 45. CF_RemoveFile - failure                                         */
/* ================================================================== */
static void Test_RemoveFile_Failure(void)
{
    CFE_Stubs.OS_remove_Return = OS_ERROR;
    int result = CF_RemoveFile("/tmp/file.dat");

    UtAssert_True(result == 1,
                  "RemoveFile Failure: returns 1");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_REMOVE_ERR2_EID,
                  "RemoveFile Failure: REMOVE_ERR2 event");
}

/* ================================================================== */
/* 46. CF_FileSize - nominal                                           */
/* ================================================================== */
static void Test_FileSize_Nominal(void)
{
    CFE_Stubs.OS_stat_Return = OS_SUCCESS;
    CFE_Stubs.OS_stat_StatBuf.st_size = 12345;

    u_int_4 size = CF_FileSize("/ram/testfile.dat");

    UtAssert_True(size == 12345,
                  "FileSize Nominal: returns correct size");
}

/* ================================================================== */
/* 47. CF_FileSize - stat failure                                      */
/* ================================================================== */
static void Test_FileSize_StatFail(void)
{
    CFE_Stubs.OS_stat_Return = OS_ERROR;

    u_int_4 size = CF_FileSize("/ram/nonexist.dat");

    UtAssert_True(size == 0,
                  "FileSize StatFail: returns 0");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_LOGIC_NAME_ERR_EID,
                  "FileSize StatFail: LOGIC_NAME_ERR event");
}

/* ================================================================== */
/* 48. CF_Fopen - mode "r" success                                     */
/* ================================================================== */
static void Test_Fopen_ReadSuccess(void)
{
    CFE_Stubs.OS_open_Return = 5;
    CFDP_FILE *f = CF_Fopen("/ram/file.dat", "r");
    UtAssert_True(f != (CFDP_FILE *)0,
                  "Fopen read: returns non-zero handle");
}

/* ================================================================== */
/* 49. CF_Fopen - mode "rb" success                                    */
/* ================================================================== */
static void Test_Fopen_ReadBinarySuccess(void)
{
    CFE_Stubs.OS_open_Return = 5;
    CFDP_FILE *f = CF_Fopen("/ram/file.dat", "rb");
    UtAssert_True(f != (CFDP_FILE *)0,
                  "Fopen rb: returns non-zero handle");
}

/* ================================================================== */
/* 50. CF_Fopen - mode "r" failure                                     */
/* ================================================================== */
static void Test_Fopen_ReadFail(void)
{
    CFE_Stubs.OS_open_Return = -1;
    CFDP_FILE *f = CF_Fopen("/ram/file.dat", "r");
    UtAssert_True(f == (CFDP_FILE *)0,
                  "Fopen read fail: returns 0");
}

/* ================================================================== */
/* 51. CF_Fopen - mode "w" success (creat)                             */
/* ================================================================== */
static void Test_Fopen_WriteSuccess(void)
{
    CFE_Stubs.OS_creat_Return = 7;
    CFDP_FILE *f = CF_Fopen("/ram/file.dat", "w");
    UtAssert_True(f != (CFDP_FILE *)0,
                  "Fopen write: returns non-zero handle");
}

/* ================================================================== */
/* 52. CF_Fopen - mode "wb" success                                    */
/* ================================================================== */
static void Test_Fopen_WriteBinarySuccess(void)
{
    CFE_Stubs.OS_creat_Return = 7;
    CFDP_FILE *f = CF_Fopen("/ram/file.dat", "wb");
    UtAssert_True(f != (CFDP_FILE *)0,
                  "Fopen wb: returns non-zero handle");
}

/* ================================================================== */
/* 53. CF_Fopen - mode "w" failure                                     */
/* ================================================================== */
static void Test_Fopen_WriteFail(void)
{
    CFE_Stubs.OS_creat_Return = -1;
    CFDP_FILE *f = CF_Fopen("/ram/file.dat", "w");
    UtAssert_True(f == (CFDP_FILE *)0,
                  "Fopen write fail: returns 0");
}

/* ================================================================== */
/* 54. CF_Fopen - mode "rw" success                                    */
/* ================================================================== */
static void Test_Fopen_ReadWriteSuccess(void)
{
    CFE_Stubs.OS_open_Return = 5;
    CFDP_FILE *f = CF_Fopen("/ram/file.dat", "rw");
    UtAssert_True(f != (CFDP_FILE *)0,
                  "Fopen rw: returns non-zero handle");
}

/* ================================================================== */
/* 55. CF_Fopen - mode "rwb" success                                   */
/* ================================================================== */
static void Test_Fopen_ReadWriteBinarySuccess(void)
{
    CFE_Stubs.OS_open_Return = 5;
    CFDP_FILE *f = CF_Fopen("/ram/file.dat", "rwb");
    UtAssert_True(f != (CFDP_FILE *)0,
                  "Fopen rwb: returns non-zero handle");
}

/* ================================================================== */
/* 56. CF_Fopen - default mode (other string)                          */
/* ================================================================== */
static void Test_Fopen_DefaultMode(void)
{
    CFE_Stubs.OS_open_Return = 5;
    CFDP_FILE *f = CF_Fopen("/ram/file.dat", "a+");
    UtAssert_True(f != (CFDP_FILE *)0,
                  "Fopen default mode: returns non-zero handle");
}

/* ================================================================== */
/* 57. CF_Fopen - default mode failure                                 */
/* ================================================================== */
static void Test_Fopen_DefaultModeFail(void)
{
    CFE_Stubs.OS_open_Return = -1;
    CFDP_FILE *f = CF_Fopen("/ram/file.dat", "a+");
    UtAssert_True(f == (CFDP_FILE *)0,
                  "Fopen default mode fail: returns 0");
}

/* ================================================================== */
/* 58. CF_Fseek - SEEK_SET success                                     */
/* ================================================================== */
static void Test_Fseek_SeekSet(void)
{
    CFE_Stubs.OS_lseek_Return = 100;
    int result = CF_Fseek((CFDP_FILE *)5, 100, SEEK_SET);
    UtAssert_True(result == OS_FS_SUCCESS,
                  "Fseek SEEK_SET: returns success");
}

/* ================================================================== */
/* 59. CF_Fseek - SEEK_CUR success                                     */
/* ================================================================== */
static void Test_Fseek_SeekCur(void)
{
    CFE_Stubs.OS_lseek_Return = 200;
    int result = CF_Fseek((CFDP_FILE *)5, 100, SEEK_CUR);
    UtAssert_True(result == OS_FS_SUCCESS,
                  "Fseek SEEK_CUR: returns success");
}

/* ================================================================== */
/* 60. CF_Fseek - SEEK_END success                                     */
/* ================================================================== */
static void Test_Fseek_SeekEnd(void)
{
    CFE_Stubs.OS_lseek_Return = 300;
    int result = CF_Fseek((CFDP_FILE *)5, 0, SEEK_END);
    UtAssert_True(result == OS_FS_SUCCESS,
                  "Fseek SEEK_END: returns success");
}

/* ================================================================== */
/* 61. CF_Fseek - failure                                              */
/* ================================================================== */
static void Test_Fseek_Failure(void)
{
    CFE_Stubs.OS_lseek_Return = OS_FS_ERROR;
    int result = CF_Fseek((CFDP_FILE *)5, 0, SEEK_SET);
    UtAssert_True(result == 1,
                  "Fseek Failure: returns 1");
}

/* ================================================================== */
/* 62. CF_Fread - success                                              */
/* ================================================================== */
static void Test_Fread_Success(void)
{
    CFE_Stubs.OS_read_Return = 256;
    char buf[256];
    size_t count = CF_Fread(buf, 1, 256, (CFDP_FILE *)5);
    UtAssert_True(count == 256,
                  "Fread Success: returns correct count");
}

/* ================================================================== */
/* 63. CF_Fread - failure (negative return)                            */
/* ================================================================== */
static void Test_Fread_Failure(void)
{
    CFE_Stubs.OS_read_Return = -1;
    char buf[256];
    size_t count = CF_Fread(buf, 1, 256, (CFDP_FILE *)5);
    UtAssert_True(count == 0,
                  "Fread Failure: returns 0");
}

/* ================================================================== */
/* 64. CF_Fread - zero size                                            */
/* ================================================================== */
static void Test_Fread_ZeroSize(void)
{
    char buf[256];
    size_t count = CF_Fread(buf, 0, 256, (CFDP_FILE *)5);
    UtAssert_True(count == 0,
                  "Fread ZeroSize: returns 0");
}

/* ================================================================== */
/* 65. CF_Fread - zero count                                           */
/* ================================================================== */
static void Test_Fread_ZeroCount(void)
{
    char buf[256];
    size_t count = CF_Fread(buf, 1, 0, (CFDP_FILE *)5);
    UtAssert_True(count == 0,
                  "Fread ZeroCount: returns 0");
}

/* ================================================================== */
/* 66. CF_Fwrite - success                                             */
/* ================================================================== */
static void Test_Fwrite_Success(void)
{
    /* OS_write stub returns nbytes when OS_write_Return == 0 */
    CFE_Stubs.OS_write_Return = 0;
    char buf[128];
    memset(buf, 'A', sizeof(buf));
    size_t count = CF_Fwrite(buf, 1, 128, (CFDP_FILE *)5);
    UtAssert_True(count == 128,
                  "Fwrite Success: returns correct count");
}

/* ================================================================== */
/* 67. CF_Fwrite - failure                                             */
/* ================================================================== */
static void Test_Fwrite_Failure(void)
{
    CFE_Stubs.OS_write_Return = -1;
    char buf[128];
    memset(buf, 0, sizeof(buf));
    size_t count = CF_Fwrite(buf, 1, 128, (CFDP_FILE *)5);
    UtAssert_True(count == 0,
                  "Fwrite Failure: returns 0");
}

/* ================================================================== */
/* 68. CF_Fwrite - zero size                                           */
/* ================================================================== */
static void Test_Fwrite_ZeroSize(void)
{
    char buf[128];
    memset(buf, 0, sizeof(buf));
    size_t count = CF_Fwrite(buf, 0, 128, (CFDP_FILE *)5);
    UtAssert_True(count == 0,
                  "Fwrite ZeroSize: returns 0");
}

/* ================================================================== */
/* 69. CF_Fwrite - zero count                                          */
/* ================================================================== */
static void Test_Fwrite_ZeroCount(void)
{
    char buf[128];
    memset(buf, 0, sizeof(buf));
    size_t count = CF_Fwrite(buf, 1, 0, (CFDP_FILE *)5);
    UtAssert_True(count == 0,
                  "Fwrite ZeroCount: returns 0");
}

/* ================================================================== */
/* 70. CF_Fclose - success                                             */
/* ================================================================== */
static void Test_Fclose_Success(void)
{
    CFE_Stubs.OS_close_Return = OS_SUCCESS;
    int result = CF_Fclose((CFDP_FILE *)5);
    UtAssert_True(result == OS_SUCCESS,
                  "Fclose Success: returns OS_SUCCESS");
}

/* ================================================================== */
/* 71. CF_Fclose - failure                                             */
/* ================================================================== */
static void Test_Fclose_Failure(void)
{
    CFE_Stubs.OS_close_Return = OS_ERROR;
    int result = CF_Fclose((CFDP_FILE *)5);
    UtAssert_True(result != OS_SUCCESS,
                  "Fclose Failure: returns non-success");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_FILE_CLOSE_ERR_EID,
                  "Fclose Failure: FILE_CLOSE_ERR event");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_ERROR,
                  "Fclose Failure: event type ERROR");
}

/* ================================================================== */
/* 72. CF_DebugEvent - nominal                                         */
/* ================================================================== */
static void Test_DebugEvent_Nominal(void)
{
    int result = CF_DebugEvent("Test debug %d", 42);
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_CFDP_ENGINE_DEB_EID,
                  "DebugEvent: correct event ID");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_DEBUG,
                  "DebugEvent: event type DEBUG");
    UtAssert_True(result == CFE_SUCCESS,
                  "DebugEvent: returns CFE_SUCCESS");
}

/* ================================================================== */
/* 73. CF_DebugEvent - newline stripping                               */
/* ================================================================== */
static void Test_DebugEvent_NewlineStrip(void)
{
    CF_DebugEvent("Hello\nWorld");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_CFDP_ENGINE_DEB_EID,
                  "DebugEvent newline: event sent");
}

/* ================================================================== */
/* 74. CF_InfoEvent - nominal                                          */
/* ================================================================== */
static void Test_InfoEvent_Nominal(void)
{
    int result = CF_InfoEvent("Test info %s", "message");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_CFDP_ENGINE_INFO_EID,
                  "InfoEvent: correct event ID");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_DEBUG,
                  "InfoEvent: event type DEBUG (info maps to debug)");
    UtAssert_True(result == CFE_SUCCESS,
                  "InfoEvent: returns CFE_SUCCESS");
}

/* ================================================================== */
/* 75. CF_WarningEvent - nominal                                       */
/* ================================================================== */
static void Test_WarningEvent_Nominal(void)
{
    int result = CF_WarningEvent("Test warning %d", 99);
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_CFDP_ENGINE_WARN_EID,
                  "WarningEvent: correct event ID");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_INFORMATION,
                  "WarningEvent: event type INFORMATION");
    UtAssert_True(result == CFE_SUCCESS,
                  "WarningEvent: returns CFE_SUCCESS");
}

/* ================================================================== */
/* 76. CF_ErrorEvent - nominal                                         */
/* ================================================================== */
static void Test_ErrorEvent_Nominal(void)
{
    int result = CF_ErrorEvent("Test error %d", 1);
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_CFDP_ENGINE_ERR_EID,
                  "ErrorEvent: correct event ID");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventType == CFE_EVS_ERROR,
                  "ErrorEvent: event type ERROR");
    UtAssert_True(result == CFE_SUCCESS,
                  "ErrorEvent: returns CFE_SUCCESS");
}

/* ================================================================== */
/* 77. CF_Tmpcreat - nominal                                           */
/* ================================================================== */
static void Test_Tmpcreat_Nominal(void)
{
    CFE_Stubs.OS_creat_Return = 5;
    int32 fd = CF_Tmpcreat("/ram/tmpfile", OS_READ_WRITE);
    UtAssert_True(fd == 5,
                  "Tmpcreat Nominal: returns fd 5");
}

/* ================================================================== */
/* 78. CF_Tmpcreat - fd zero replacement                               */
/* ================================================================== */
static void Test_Tmpcreat_FdZero(void)
{
    CFE_Stubs.OS_creat_Return = 0;
    int32 fd = CF_Tmpcreat("/ram/tmpfile", OS_READ_WRITE);
    UtAssert_True(fd == 0x7FFFFFFF,
                  "Tmpcreat FdZero: returns CF_FD_ZERO_REPLACEMENT");
}

/* ================================================================== */
/* 79. CF_Tmpopen - nominal                                            */
/* ================================================================== */
static void Test_Tmpopen_Nominal(void)
{
    CFE_Stubs.OS_open_Return = 3;
    int32 fd = CF_Tmpopen("/ram/tmpfile", OS_READ_ONLY, 0);
    UtAssert_True(fd == 3,
                  "Tmpopen Nominal: returns fd 3");
}

/* ================================================================== */
/* 80. CF_Tmpopen - fd zero replacement                                */
/* ================================================================== */
static void Test_Tmpopen_FdZero(void)
{
    CFE_Stubs.OS_open_Return = 0;
    int32 fd = CF_Tmpopen("/ram/tmpfile", OS_READ_ONLY, 0);
    UtAssert_True(fd == 0x7FFFFFFF,
                  "Tmpopen FdZero: returns CF_FD_ZERO_REPLACEMENT");
}

/* ================================================================== */
/* 81. CF_Tmpclose - nominal                                           */
/* ================================================================== */
static void Test_Tmpclose_Nominal(void)
{
    CFE_Stubs.OS_close_Return = OS_SUCCESS;
    int32 result = CF_Tmpclose(5);
    UtAssert_True(result == OS_SUCCESS,
                  "Tmpclose Nominal: returns OS_SUCCESS");
}

/* ================================================================== */
/* 82. CF_Tmpclose - fd zero replacement                               */
/* ================================================================== */
static void Test_Tmpclose_FdZero(void)
{
    CFE_Stubs.OS_close_Return = OS_SUCCESS;
    int32 result = CF_Tmpclose(0x7FFFFFFF);
    UtAssert_True(result == OS_SUCCESS,
                  "Tmpclose FdZero: passes fd 0 to OS_close, returns success");
}

/* ================================================================== */
/* 83. CF_Tmpread - nominal                                            */
/* ================================================================== */
static void Test_Tmpread_Nominal(void)
{
    CFE_Stubs.OS_read_Return = 100;
    char buf[256];
    int32 result = CF_Tmpread(5, buf, 256);
    UtAssert_True(result == 100,
                  "Tmpread Nominal: returns bytes read");
}

/* ================================================================== */
/* 84. CF_Tmpread - fd zero replacement                                */
/* ================================================================== */
static void Test_Tmpread_FdZero(void)
{
    CFE_Stubs.OS_read_Return = 50;
    char buf[256];
    int32 result = CF_Tmpread(0x7FFFFFFF, buf, 256);
    UtAssert_True(result == 50,
                  "Tmpread FdZero: passes fd 0 to OS_read");
}

/* ================================================================== */
/* 85. CF_Tmpwrite - nominal                                           */
/* ================================================================== */
static void Test_Tmpwrite_Nominal(void)
{
    CFE_Stubs.OS_write_Return = 0; /* 0 means return nbytes */
    char buf[128];
    int32 result = CF_Tmpwrite(5, buf, 128);
    UtAssert_True(result == 128,
                  "Tmpwrite Nominal: returns bytes written");
}

/* ================================================================== */
/* 86. CF_Tmpwrite - fd zero replacement                               */
/* ================================================================== */
static void Test_Tmpwrite_FdZero(void)
{
    CFE_Stubs.OS_write_Return = 0;
    char buf[128];
    int32 result = CF_Tmpwrite(0x7FFFFFFF, buf, 128);
    UtAssert_True(result == 128,
                  "Tmpwrite FdZero: passes fd 0 to OS_write");
}

/* ================================================================== */
/* 87. CF_Tmplseek - nominal                                           */
/* ================================================================== */
static void Test_Tmplseek_Nominal(void)
{
    CFE_Stubs.OS_lseek_Return = 500;
    int32 result = CF_Tmplseek(5, 500, OS_SEEK_SET);
    UtAssert_True(result == 500,
                  "Tmplseek Nominal: returns offset");
}

/* ================================================================== */
/* 88. CF_Tmplseek - fd zero replacement                               */
/* ================================================================== */
static void Test_Tmplseek_FdZero(void)
{
    CFE_Stubs.OS_lseek_Return = 0;
    int32 result = CF_Tmplseek(0x7FFFFFFF, 0, OS_SEEK_SET);
    UtAssert_True(result == 0,
                  "Tmplseek FdZero: passes fd 0 to OS_lseek");
}

/* ================================================================== */
/* 89. CF_PendingQueueSort - empty list returns error                  */
/* ================================================================== */
static void Test_PendingQueueSort_EmptyList(void)
{
    /* HeadPtr is NULL */
    int32 result = CF_PendingQueueSort(0);
    UtAssert_True(result == CF_ERROR,
                  "PendingQueueSort EmptyList: returns CF_ERROR");
}

/* ================================================================== */
/* 90. CF_PendingQueueSort - single node returns success               */
/* ================================================================== */
static void Test_PendingQueueSort_SingleNode(void)
{
    CF_QueueEntry_t node;
    memset(&node, 0, sizeof(node));
    node.Priority = 5;
    node.Next = NULL;

    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr = &node;
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].TailPtr = &node;
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt = 1;

    int32 result = CF_PendingQueueSort(0);
    UtAssert_True(result == CF_SUCCESS,
                  "PendingQueueSort SingleNode: returns CF_SUCCESS");
}

/* ================================================================== */
/* 91. CF_PendingQueueSort - new node lower priority (already sorted)  */
/* ================================================================== */
static void Test_PendingQueueSort_AlreadySorted(void)
{
    CF_QueueEntry_t node1, node2;
    memset(&node1, 0, sizeof(node1));
    memset(&node2, 0, sizeof(node2));

    /* node1 is at the head (new node), node2 is next */
    node1.Priority = 10; /* lower priority (higher value) */
    node2.Priority = 5;  /* higher priority (lower value) */
    node1.Next = &node2;
    node2.Prev = &node1;
    node2.Next = NULL;

    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr = &node1;
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].TailPtr = &node2;
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt = 2;

    int32 result = CF_PendingQueueSort(0);
    UtAssert_True(result == CF_SUCCESS,
                  "PendingQueueSort AlreadySorted: returns CF_SUCCESS");
}

/* ================================================================== */
/* 92. CF_Indication - IND_METADATA_RECV multiple times                */
/* ================================================================== */
static void Test_Indication_MetadataRecvMultiple(void)
{
    TRANS_STATUS ts = MakeTransStatus();

    CF_Indication(IND_METADATA_RECV, ts);
    CF_Indication(IND_METADATA_RECV, ts);
    CF_Indication(IND_METADATA_RECV, ts);

    UtAssert_True(CF_AppData.Hk.Up.MetaCount == 3,
                  "IND_METADATA_RECV x3: MetaCount == 3");
}

/* ================================================================== */
/* 93. CF_Indication - IND_ABANDONED increments counter                */
/* ================================================================== */
static void Test_Indication_AbandonedMultiple(void)
{
    TRANS_STATUS ts = MakeTransStatus();

    CF_Indication(IND_ABANDONED, ts);
    CF_Indication(IND_ABANDONED, ts);

    UtAssert_True(CF_AppData.Hk.App.TotalAbandonTrans == 2,
                  "ABANDONED x2: TotalAbandonTrans == 2");
}

/* ================================================================== */
/* 94. CF_Fread - multi-byte size                                      */
/* ================================================================== */
static void Test_Fread_MultiByteSize(void)
{
    CFE_Stubs.OS_read_Return = 40;  /* 40 bytes read */
    char buf[256];
    /* Size = 4, Count = 10 => request 40 bytes; BytesRead/Size = 10 */
    size_t count = CF_Fread(buf, 4, 10, (CFDP_FILE *)5);
    UtAssert_True(count == 10,
                  "Fread MultiByteSize: returns count = 10");
}

/* ================================================================== */
/* 95. CF_Fwrite - multi-byte size                                     */
/* ================================================================== */
static void Test_Fwrite_MultiByteSize(void)
{
    CFE_Stubs.OS_write_Return = 0; /* returns nbytes */
    char buf[256];
    memset(buf, 0, sizeof(buf));
    /* Size = 4, Count = 10 => write 40 bytes; BytesWritten/Size = 10 */
    size_t count = CF_Fwrite(buf, 4, 10, (CFDP_FILE *)5);
    UtAssert_True(count == 10,
                  "Fwrite MultiByteSize: returns count = 10");
}

/* ================================================================== */
/* 96. CF_PduOutputReady - uplink response path (non-playback)         */
/* ================================================================== */
static void Test_PduOutputReady_UplinkResponse(void)
{
    TRANSACTION trans;
    ID dst;
    memset(&trans, 0, sizeof(trans));
    memset(&dst, 0, sizeof(dst));

    /* Make entity ID NOT match flight engine ID to trigger uplink path */
    strncpy(CF_AppData.Hk.Eng.FlightEngineEntityId, "0.24",
            CF_MAX_CFG_VALUE_CHARS);
    trans.source_id.value[0] = 1;
    trans.source_id.value[1] = 23;
    trans.number = 200;

    /* Not found => returns TRUE to release undirected PDU */
    boolean result = CF_PduOutputReady(FILE_DIR_PDU, trans, dst);
    UtAssert_True(result == TRUE,
                  "PduOutputReady UplinkResponse not found: returns TRUE");
}

/* ================================================================== */
/* 97. CF_RenameFile - remove failure after successful copy            */
/* ================================================================== */
static void Test_RenameFile_RemoveFail(void)
{
    CFE_Stubs.OS_open_Return = 5;
    CFE_Stubs.OS_creat_Return = 6;
    CFE_Stubs.OS_read_Return = 0; /* EOF immediately */
    CFE_Stubs.OS_remove_Return = OS_ERROR;

    int result = CF_RenameFile("/tmp/old.dat", "/dst/new.dat");

    UtAssert_True(result == 1,
                  "RenameFile RemoveFail: returns 1");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_REMOVE_ERR1_EID,
                  "RenameFile RemoveFail: REMOVE_ERR1 event");
}

/* ================================================================== */
/* 98. CF_Indication - IND_MACHINE_DEALLOCATED success sender delete   */
/* ================================================================== */
static void Test_Indication_MachDeallocSuccessSenderDelete(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.final_status = FINAL_STATUS_SUCCESSFUL;
    ts.role = CLASS_1_SENDER;

    CF_QueueEntry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.TransNum = 100;
    entry.ChanNum  = 0;
    entry.Preserve = CF_DELETE_FILE;  /* file should be removed */
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].TailPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].EntryCnt = 1;

    CF_Indication(IND_MACHINE_DEALLOCATED, ts);

    /* OS_remove should have been called to delete the source file */
    UtAssert_True(CFE_Stubs.OS_remove_CallCount >= 1,
                  "MachDealloc success sender delete: OS_remove called");
    UtAssert_True(CF_AppData.Hk.Chan[0].SuccessCounter == 1,
                  "MachDealloc success sender delete: SuccessCounter incremented");
}

/* ================================================================== */
/* 99. CF_Indication - IND_EOF_SENT with dequeue enabled               */
/* ================================================================== */
static void Test_Indication_EofSent_DequeueEnabled(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.role = CLASS_1_SENDER;

    CF_QueueEntry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.TransNum = 100;
    entry.ChanNum  = 0;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].TailPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].EntryCnt = 1;

    CF_AppData.Hk.AutoSuspend.EnFlag = CF_DISABLED;
    TestConfigTable.OuCh[0].DequeueEnable = CF_ENABLED;

    CF_Indication(IND_EOF_SENT, ts);

    /* DataBlast should be cleared */
    UtAssert_True(CF_AppData.Chan[0].DataBlast == CF_NOT_IN_PROGRESS,
                  "EOF_SENT dequeue: DataBlast cleared");
}

/* ================================================================== */
/* 100. CF_Indication - IND_EOF_SENT with dequeue disabled             */
/* ================================================================== */
static void Test_Indication_EofSent_DequeueDisabled(void)
{
    TRANS_STATUS ts = MakeTransStatus();
    ts.role = CLASS_1_SENDER;

    CF_QueueEntry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.TransNum = 100;
    entry.ChanNum  = 0;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].TailPtr = &entry;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].EntryCnt = 1;

    CF_AppData.Hk.AutoSuspend.EnFlag = CF_DISABLED;
    TestConfigTable.OuCh[0].DequeueEnable = CF_DISABLED;

    CF_Indication(IND_EOF_SENT, ts);

    UtAssert_True(CF_AppData.Chan[0].DataBlast == CF_NOT_IN_PROGRESS,
                  "EOF_SENT dequeue disabled: DataBlast cleared");
}

/* ================================================================== */
/* Registration function                                               */
/* ================================================================== */
void CF_Callbacks_AddTests(void)
{
    /* CF_RegisterCallbacks */
    UtTest_Add(Test_RegisterCallbacks_Nominal, Setup, NULL,
               "CF_RegisterCallbacks - nominal");

    /* CF_Indication - various indication types */
    UtTest_Add(Test_Indication_Transaction, Setup, NULL,
               "CF_Indication - IND_TRANSACTION (no-op)");
    UtTest_Add(Test_Indication_MachAllocClass1Rcv, Setup, NULL,
               "CF_Indication - IND_MACHINE_ALLOCATED class 1 receiver");
    UtTest_Add(Test_Indication_MachAllocClass2Rcv, Setup, NULL,
               "CF_Indication - IND_MACHINE_ALLOCATED class 2 receiver");
    UtTest_Add(Test_Indication_MachAllocFail, Setup, NULL,
               "CF_Indication - IND_MACHINE_ALLOCATED alloc failure");
    UtTest_Add(Test_Indication_MachAllocSender, Setup, NULL,
               "CF_Indication - IND_MACHINE_ALLOCATED sender");
    UtTest_Add(Test_Indication_MetadataSent, Setup, NULL,
               "CF_Indication - IND_METADATA_SENT (no-op)");
    UtTest_Add(Test_Indication_MetadataRecv, Setup, NULL,
               "CF_Indication - IND_METADATA_RECV");
    UtTest_Add(Test_Indication_EofSent_AutoSuspendDisabled, Setup, NULL,
               "CF_Indication - IND_EOF_SENT auto-suspend disabled");
    UtTest_Add(Test_Indication_EofSent_AutoSuspendOverflow, Setup, NULL,
               "CF_Indication - IND_EOF_SENT auto-suspend overflow");
    UtTest_Add(Test_Indication_EofSent_AutoSuspendNominal, Setup, NULL,
               "CF_Indication - IND_EOF_SENT auto-suspend nominal");
    UtTest_Add(Test_Indication_EofRecv, Setup, NULL,
               "CF_Indication - IND_EOF_RECV (no-op)");
    UtTest_Add(Test_Indication_TransFinished, Setup, NULL,
               "CF_Indication - IND_TRANSACTION_FINISHED (no-op)");
    UtTest_Add(Test_Indication_MachDeallocSuccessReceiver, Setup, NULL,
               "CF_Indication - IND_MACHINE_DEALLOCATED success receiver");
    UtTest_Add(Test_Indication_MachDeallocSuccessClass2Rcv, Setup, NULL,
               "CF_Indication - IND_MACHINE_DEALLOCATED success class 2 rcv");
    UtTest_Add(Test_Indication_MachDeallocSuccessSender, Setup, NULL,
               "CF_Indication - IND_MACHINE_DEALLOCATED success sender");
    UtTest_Add(Test_Indication_MachDeallocFailReceiver, Setup, NULL,
               "CF_Indication - IND_MACHINE_DEALLOCATED fail receiver");
    UtTest_Add(Test_Indication_MachDeallocFailClass2Receiver, Setup, NULL,
               "CF_Indication - IND_MACHINE_DEALLOCATED fail class 2 rcv");
    UtTest_Add(Test_Indication_MachDeallocFailSender, Setup, NULL,
               "CF_Indication - IND_MACHINE_DEALLOCATED fail sender");
    UtTest_Add(Test_Indication_MachDeallocFailSenderBadChan, Setup, NULL,
               "CF_Indication - IND_MACHINE_DEALLOCATED fail sender bad chan");
    UtTest_Add(Test_Indication_MachDeallocFailSenderBlasting, Setup, NULL,
               "CF_Indication - IND_MACHINE_DEALLOCATED fail sender blasting");
    UtTest_Add(Test_Indication_AckTimerExpired, Setup, NULL,
               "CF_Indication - IND_ACK_TIMER_EXPIRED");
    UtTest_Add(Test_Indication_InactivityTimerExpired, Setup, NULL,
               "CF_Indication - IND_INACTIVITY_TIMER_EXPIRED");
    UtTest_Add(Test_Indication_NakTimerExpired, Setup, NULL,
               "CF_Indication - IND_NAK_TIMER_EXPIRED");
    UtTest_Add(Test_Indication_Suspended, Setup, NULL,
               "CF_Indication - IND_SUSPENDED");
    UtTest_Add(Test_Indication_Resumed, Setup, NULL,
               "CF_Indication - IND_RESUMED");
    UtTest_Add(Test_Indication_Report, Setup, NULL,
               "CF_Indication - IND_REPORT (no-op)");
    UtTest_Add(Test_Indication_Fault, Setup, NULL,
               "CF_Indication - IND_FAULT");
    UtTest_Add(Test_Indication_Abandoned, Setup, NULL,
               "CF_Indication - IND_ABANDONED");
    UtTest_Add(Test_Indication_FileSegmentRecv, Setup, NULL,
               "CF_Indication - IND_FILE_SEGMENT_RECV (default)");
    UtTest_Add(Test_Indication_FileSegmentSent, Setup, NULL,
               "CF_Indication - IND_FILE_SEGMENT_SENT (default)");
    UtTest_Add(Test_Indication_DefaultUnexpected, Setup, NULL,
               "CF_Indication - default unexpected type");
    UtTest_Add(Test_Indication_MetadataRecvMultiple, Setup, NULL,
               "CF_Indication - IND_METADATA_RECV multiple");
    UtTest_Add(Test_Indication_AbandonedMultiple, Setup, NULL,
               "CF_Indication - IND_ABANDONED multiple");
    UtTest_Add(Test_Indication_MachDeallocSuccessSenderDelete, Setup, NULL,
               "CF_Indication - IND_MACHINE_DEALLOCATED success sender delete");
    UtTest_Add(Test_Indication_EofSent_DequeueEnabled, Setup, NULL,
               "CF_Indication - IND_EOF_SENT dequeue enabled");
    UtTest_Add(Test_Indication_EofSent_DequeueDisabled, Setup, NULL,
               "CF_Indication - IND_EOF_SENT dequeue disabled");

    /* CF_PduOutputOpen */
    UtTest_Add(Test_PduOutputOpen_Nominal, Setup, NULL,
               "CF_PduOutputOpen - nominal returns YES");

    /* CF_PduOutputReady */
    UtTest_Add(Test_PduOutputReady_SemInvalid, Setup, NULL,
               "CF_PduOutputReady - semaphore invalid (green)");
    UtTest_Add(Test_PduOutputReady_SemGreenLight, Setup, NULL,
               "CF_PduOutputReady - semaphore green light");
    UtTest_Add(Test_PduOutputReady_SemRedLight, Setup, NULL,
               "CF_PduOutputReady - semaphore red light");
    UtTest_Add(Test_PduOutputReady_TransNotFound, Setup, NULL,
               "CF_PduOutputReady - trans not found");
    UtTest_Add(Test_PduOutputReady_UplinkResponse, Setup, NULL,
               "CF_PduOutputReady - uplink response path");

    /* CF_PduOutputSend */
    UtTest_Add(Test_PduOutputSend_ChanNotFound, Setup, NULL,
               "CF_PduOutputSend - channel not found");
    UtTest_Add(Test_PduOutputSend_Nominal, Setup, NULL,
               "CF_PduOutputSend - nominal send");
    UtTest_Add(Test_PduOutputSend_UplinkResponse, Setup, NULL,
               "CF_PduOutputSend - uplink response direction");

    /* CF_RenameFile */
    UtTest_Add(Test_RenameFile_Success, Setup, NULL,
               "CF_RenameFile - success");
    UtTest_Add(Test_RenameFile_OpenFail, Setup, NULL,
               "CF_RenameFile - open failure");
    UtTest_Add(Test_RenameFile_CreatFail, Setup, NULL,
               "CF_RenameFile - creat failure");
    UtTest_Add(Test_RenameFile_RemoveFail, Setup, NULL,
               "CF_RenameFile - remove failure");

    /* CF_RemoveFile */
    UtTest_Add(Test_RemoveFile_Success, Setup, NULL,
               "CF_RemoveFile - success");
    UtTest_Add(Test_RemoveFile_Failure, Setup, NULL,
               "CF_RemoveFile - failure");

    /* CF_FileSize */
    UtTest_Add(Test_FileSize_Nominal, Setup, NULL,
               "CF_FileSize - nominal");
    UtTest_Add(Test_FileSize_StatFail, Setup, NULL,
               "CF_FileSize - stat failure");

    /* CF_Fopen */
    UtTest_Add(Test_Fopen_ReadSuccess, Setup, NULL,
               "CF_Fopen - mode r success");
    UtTest_Add(Test_Fopen_ReadBinarySuccess, Setup, NULL,
               "CF_Fopen - mode rb success");
    UtTest_Add(Test_Fopen_ReadFail, Setup, NULL,
               "CF_Fopen - mode r failure");
    UtTest_Add(Test_Fopen_WriteSuccess, Setup, NULL,
               "CF_Fopen - mode w success");
    UtTest_Add(Test_Fopen_WriteBinarySuccess, Setup, NULL,
               "CF_Fopen - mode wb success");
    UtTest_Add(Test_Fopen_WriteFail, Setup, NULL,
               "CF_Fopen - mode w failure");
    UtTest_Add(Test_Fopen_ReadWriteSuccess, Setup, NULL,
               "CF_Fopen - mode rw success");
    UtTest_Add(Test_Fopen_ReadWriteBinarySuccess, Setup, NULL,
               "CF_Fopen - mode rwb success");
    UtTest_Add(Test_Fopen_DefaultMode, Setup, NULL,
               "CF_Fopen - default mode success");
    UtTest_Add(Test_Fopen_DefaultModeFail, Setup, NULL,
               "CF_Fopen - default mode failure");

    /* CF_Fseek */
    UtTest_Add(Test_Fseek_SeekSet, Setup, NULL,
               "CF_Fseek - SEEK_SET success");
    UtTest_Add(Test_Fseek_SeekCur, Setup, NULL,
               "CF_Fseek - SEEK_CUR success");
    UtTest_Add(Test_Fseek_SeekEnd, Setup, NULL,
               "CF_Fseek - SEEK_END success");
    UtTest_Add(Test_Fseek_Failure, Setup, NULL,
               "CF_Fseek - lseek failure");

    /* CF_Fread */
    UtTest_Add(Test_Fread_Success, Setup, NULL,
               "CF_Fread - success");
    UtTest_Add(Test_Fread_Failure, Setup, NULL,
               "CF_Fread - failure");
    UtTest_Add(Test_Fread_ZeroSize, Setup, NULL,
               "CF_Fread - zero size");
    UtTest_Add(Test_Fread_ZeroCount, Setup, NULL,
               "CF_Fread - zero count");
    UtTest_Add(Test_Fread_MultiByteSize, Setup, NULL,
               "CF_Fread - multi-byte size");

    /* CF_Fwrite */
    UtTest_Add(Test_Fwrite_Success, Setup, NULL,
               "CF_Fwrite - success");
    UtTest_Add(Test_Fwrite_Failure, Setup, NULL,
               "CF_Fwrite - failure");
    UtTest_Add(Test_Fwrite_ZeroSize, Setup, NULL,
               "CF_Fwrite - zero size");
    UtTest_Add(Test_Fwrite_ZeroCount, Setup, NULL,
               "CF_Fwrite - zero count");
    UtTest_Add(Test_Fwrite_MultiByteSize, Setup, NULL,
               "CF_Fwrite - multi-byte size");

    /* CF_Fclose */
    UtTest_Add(Test_Fclose_Success, Setup, NULL,
               "CF_Fclose - success");
    UtTest_Add(Test_Fclose_Failure, Setup, NULL,
               "CF_Fclose - failure");

    /* Event wrappers */
    UtTest_Add(Test_DebugEvent_Nominal, Setup, NULL,
               "CF_DebugEvent - nominal");
    UtTest_Add(Test_DebugEvent_NewlineStrip, Setup, NULL,
               "CF_DebugEvent - newline stripping");
    UtTest_Add(Test_InfoEvent_Nominal, Setup, NULL,
               "CF_InfoEvent - nominal");
    UtTest_Add(Test_WarningEvent_Nominal, Setup, NULL,
               "CF_WarningEvent - nominal");
    UtTest_Add(Test_ErrorEvent_Nominal, Setup, NULL,
               "CF_ErrorEvent - nominal");

    /* Tmp wrappers */
    UtTest_Add(Test_Tmpcreat_Nominal, Setup, NULL,
               "CF_Tmpcreat - nominal");
    UtTest_Add(Test_Tmpcreat_FdZero, Setup, NULL,
               "CF_Tmpcreat - fd zero replacement");
    UtTest_Add(Test_Tmpopen_Nominal, Setup, NULL,
               "CF_Tmpopen - nominal");
    UtTest_Add(Test_Tmpopen_FdZero, Setup, NULL,
               "CF_Tmpopen - fd zero replacement");
    UtTest_Add(Test_Tmpclose_Nominal, Setup, NULL,
               "CF_Tmpclose - nominal");
    UtTest_Add(Test_Tmpclose_FdZero, Setup, NULL,
               "CF_Tmpclose - fd zero replacement");
    UtTest_Add(Test_Tmpread_Nominal, Setup, NULL,
               "CF_Tmpread - nominal");
    UtTest_Add(Test_Tmpread_FdZero, Setup, NULL,
               "CF_Tmpread - fd zero replacement");
    UtTest_Add(Test_Tmpwrite_Nominal, Setup, NULL,
               "CF_Tmpwrite - nominal");
    UtTest_Add(Test_Tmpwrite_FdZero, Setup, NULL,
               "CF_Tmpwrite - fd zero replacement");
    UtTest_Add(Test_Tmplseek_Nominal, Setup, NULL,
               "CF_Tmplseek - nominal");
    UtTest_Add(Test_Tmplseek_FdZero, Setup, NULL,
               "CF_Tmplseek - fd zero replacement");

    /* CF_PendingQueueSort */
    UtTest_Add(Test_PendingQueueSort_EmptyList, Setup, NULL,
               "CF_PendingQueueSort - empty list");
    UtTest_Add(Test_PendingQueueSort_SingleNode, Setup, NULL,
               "CF_PendingQueueSort - single node");
    UtTest_Add(Test_PendingQueueSort_AlreadySorted, Setup, NULL,
               "CF_PendingQueueSort - already sorted");
}
