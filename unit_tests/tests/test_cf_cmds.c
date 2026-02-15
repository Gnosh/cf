/*
 * Unit tests for cf_cmds.c
 *
 * Comprehensive tests covering all command handlers and utility functions
 * defined in cf_cmds.c.
 */
#include "test_framework.h"
#include "cfe_stubs.h"
#include "cfdp_stubs.h"
#include "cf_app.h"
#include "cf_cmds.h"
#include "cf_events.h"
#include "cf_msgids.h"

/* External references */
extern CF_AppData_t CF_AppData;

/* Static table used by tests */
static cf_config_table_t TestConfigTable;

/* --------------------------------------------------------------------------
 * Setup / Teardown helpers
 * -------------------------------------------------------------------------- */
static void Test_Setup(void)
{
    CFE_Stubs_Reset();
    CFDP_Stubs_Reset();
    memset(&CF_AppData, 0, sizeof(CF_AppData));
    memset(&TestConfigTable, 0, sizeof(TestConfigTable));
    CF_AppData.Tbl = &TestConfigTable;

    /* Default: give_request succeeds */
    CFDP_Stubs.give_request_return = 1; /* TRUE */
    CFDP_Stubs.set_mib_parameter_return = 1;
    CFDP_Stubs.get_mib_parameter_return = 1;

    /* Default: OS calls succeed */
    CFE_Stubs.OS_creat_Return = 5; /* valid fd */
    CFE_Stubs.CFE_FS_WriteHeader_Return = sizeof(CFE_FS_Header_t);
    CFE_Stubs.OS_close_Return = OS_SUCCESS;
    CFE_Stubs.OS_write_Return = sizeof(CF_QueueInfoFileEntry_t);
    CFE_Stubs.OS_CountSemGetInfo_Return = OS_SUCCESS;
    CFE_Stubs.OS_CountSemTimedWait_Return = OS_SUCCESS;
    CFE_Stubs.OS_CountSemGive_Return = OS_SUCCESS;
    CFE_Stubs.CFE_TBL_Modified_Return = CFE_SUCCESS;
}

static void Test_Teardown(void)
{
    /* nothing to clean up */
}

/* ==========================================================================
 * 1. CF_VerifyCmdLength tests
 * ========================================================================== */
static void Test_VerifyCmdLength_CorrectLength(void)
{
    CF_NoArgsCmd_t cmd;
    int32 result;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_NoArgsCmd_t), TRUE);
    result = CF_VerifyCmdLength((CFE_SB_MsgPtr_t)&cmd, sizeof(CF_NoArgsCmd_t));
    UtAssert_IntEq(result, CF_SUCCESS, "VerifyCmdLength returns CF_SUCCESS for correct length");
}

static void Test_VerifyCmdLength_WrongLength(void)
{
    CF_NoArgsCmd_t cmd;
    int32 result;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_NoArgsCmd_t), TRUE);
    /* Pass a different expected length to trigger mismatch */
    result = CF_VerifyCmdLength((CFE_SB_MsgPtr_t)&cmd, sizeof(CF_NoArgsCmd_t) + 10);
    UtAssert_IntEq(result, CF_BAD_MSG_LENGTH_RC, "VerifyCmdLength returns CF_BAD_MSG_LENGTH_RC for wrong length");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_CMD_LEN_ERR_EID,
                   "VerifyCmdLength sends CF_CMD_LEN_ERR_EID event");
}

/* ==========================================================================
 * 2. CF_NoopCmd tests
 * ========================================================================== */
static void Test_NoopCmd_Nominal(void)
{
    CF_NoArgsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_NoArgsCmd_t), TRUE);
    CF_AppData.MsgPtr = (CFE_SB_MsgPtr_t)&cmd;

    CF_NoopCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "NoopCmd increments CmdCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_NOOP_CMD_EID,
                   "NoopCmd sends CF_NOOP_CMD_EID");
}

static void Test_NoopCmd_BadLength(void)
{
    CF_NoArgsCmd_t cmd;

    /* Init with wrong size */
    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);
    CF_AppData.MsgPtr = (CFE_SB_MsgPtr_t)&cmd;

    CF_NoopCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "NoopCmd bad length increments ErrCounter");
    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 0, "NoopCmd bad length does not increment CmdCounter");
}

/* ==========================================================================
 * 3. CF_ResetCtrsCmd tests
 * ========================================================================== */
static void Test_ResetCtrsCmd_All(void)
{
    CF_ResetCtrsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_ResetCtrsCmd_t), TRUE);
    cmd.Value = 0; /* reset all */
    CF_AppData.Hk.CmdCounter = 5;
    CF_AppData.Hk.ErrCounter = 3;
    CF_AppData.Hk.Cond.PosAckNum = 1;
    CF_AppData.Hk.App.PDUsReceived = 10;
    CF_AppData.Hk.Chan[0].PDUsSent = 7;

    CF_ResetCtrsCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 0, "ResetAll clears CmdCounter");
    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 0, "ResetAll clears ErrCounter");
    UtAssert_IntEq(CF_AppData.Hk.Cond.PosAckNum, 0, "ResetAll clears fault counters");
    UtAssert_IntEq(CF_AppData.Hk.App.PDUsReceived, 0, "ResetAll clears uplink counters");
    UtAssert_IntEq(CF_AppData.Hk.Chan[0].PDUsSent, 0, "ResetAll clears downlink counters");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_RESET_CMD_EID,
                   "ResetCtrsCmd sends CF_RESET_CMD_EID");
}

static void Test_ResetCtrsCmd_CmdOnly(void)
{
    CF_ResetCtrsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_ResetCtrsCmd_t), TRUE);
    cmd.Value = 1;
    CF_AppData.Hk.CmdCounter = 5;
    CF_AppData.Hk.ErrCounter = 3;
    CF_AppData.Hk.Cond.PosAckNum = 2;

    CF_ResetCtrsCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 0, "Reset cmd-only clears CmdCounter");
    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 0, "Reset cmd-only clears ErrCounter");
    UtAssert_IntEq(CF_AppData.Hk.Cond.PosAckNum, 2, "Reset cmd-only does not clear fault counters");
}

static void Test_ResetCtrsCmd_FaultOnly(void)
{
    CF_ResetCtrsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_ResetCtrsCmd_t), TRUE);
    cmd.Value = 2;
    CF_AppData.Hk.CmdCounter = 5;
    CF_AppData.Hk.Cond.PosAckNum = 2;
    CF_AppData.Hk.Cond.NakLimitNum = 3;

    CF_ResetCtrsCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 5, "Reset fault-only does not clear CmdCounter");
    UtAssert_IntEq(CF_AppData.Hk.Cond.PosAckNum, 0, "Reset fault-only clears PosAckNum");
    UtAssert_IntEq(CF_AppData.Hk.Cond.NakLimitNum, 0, "Reset fault-only clears NakLimitNum");
}

static void Test_ResetCtrsCmd_UplinkOnly(void)
{
    CF_ResetCtrsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_ResetCtrsCmd_t), TRUE);
    cmd.Value = 3;
    CF_AppData.Hk.App.PDUsReceived = 10;
    CF_AppData.Hk.Up.MetaCount = 5;
    CF_AppData.Hk.Up.SuccessCounter = 3;

    CF_ResetCtrsCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.App.PDUsReceived, 0, "Reset uplink clears PDUsReceived");
    UtAssert_IntEq(CF_AppData.Hk.Up.MetaCount, 0, "Reset uplink clears MetaCount");
    UtAssert_IntEq(CF_AppData.Hk.Up.SuccessCounter, 0, "Reset uplink clears SuccessCounter");
}

static void Test_ResetCtrsCmd_DownlinkOnly(void)
{
    CF_ResetCtrsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_ResetCtrsCmd_t), TRUE);
    cmd.Value = 4;
    CF_AppData.Hk.Chan[0].PDUsSent = 7;
    CF_AppData.Hk.Chan[0].FilesSent = 3;
    CF_AppData.Hk.Chan[0].SuccessCounter = 2;

    CF_ResetCtrsCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.Chan[0].PDUsSent, 0, "Reset downlink clears PDUsSent");
    UtAssert_IntEq(CF_AppData.Hk.Chan[0].FilesSent, 0, "Reset downlink clears FilesSent");
    UtAssert_IntEq(CF_AppData.Hk.Chan[0].SuccessCounter, 0, "Reset downlink clears SuccessCounter");
}

static void Test_ResetCtrsCmd_BadLength(void)
{
    CF_ResetCtrsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);

    CF_ResetCtrsCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "ResetCtrsCmd bad length increments ErrCounter");
}

/* ==========================================================================
 * 4. CF_HousekeepingCmd tests
 * ========================================================================== */
static void Test_HousekeepingCmd_Nominal(void)
{
    CF_NoArgsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_SEND_HK_MID, CFE_SB_CMD_HDR_SIZE, TRUE);

    /* Set some queue counts */
    CF_AppData.UpQ[CF_UP_ACTIVEQ].EntryCnt = 2;
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt = 3;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].EntryCnt = 1;
    CF_AppData.Chan[0].PbQ[CF_PB_HISTORYQ].EntryCnt = 5;
    CF_AppData.Chan[0].HandshakeSemId = CF_INVALID;
    CF_AppData.Chan[1].HandshakeSemId = CF_INVALID;
    TestConfigTable.OuCh[0].DequeueEnable = CF_ENABLED;
    TestConfigTable.OuCh[1].DequeueEnable = CF_DISABLED;

    CF_HousekeepingCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.Up.UplinkActiveQFileCnt, 2,
                   "HkCmd sets uplink active Q count");
    UtAssert_IntEq(CF_AppData.Hk.Chan[0].PendingQFileCnt, 3,
                   "HkCmd sets channel 0 pending Q count");
    UtAssert_IntEq(CF_AppData.Hk.Chan[0].ActiveQFileCnt, 1,
                   "HkCmd sets channel 0 active Q count");
    UtAssert_True(CFE_Stubs.CFE_SB_SendMsg_CallCount > 0,
                  "HkCmd sends HK telemetry");
}

static void Test_HousekeepingCmd_BadLength(void)
{
    CF_NoArgsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_SEND_HK_MID, 1, TRUE);

    CF_HousekeepingCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "HkCmd bad length increments ErrCounter");
}

static void Test_HousekeepingCmd_WithSemaphore(void)
{
    CF_NoArgsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_SEND_HK_MID, CFE_SB_CMD_HDR_SIZE, TRUE);

    CF_AppData.Chan[0].HandshakeSemId = 1; /* valid semaphore ID */
    CF_AppData.Chan[1].HandshakeSemId = CF_INVALID;
    CFE_Stubs.OS_CountSemGetInfo_Return = OS_SUCCESS;
    CFE_Stubs.OS_CountSemGetInfo_SemValue = 42;
    TestConfigTable.OuCh[0].DequeueEnable = CF_ENABLED;
    TestConfigTable.OuCh[1].DequeueEnable = CF_DISABLED;

    CF_HousekeepingCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.Chan[0].SemValue, 42,
                   "HkCmd reads semaphore value when semaphore is valid");
}

static void Test_HousekeepingCmd_SemGetInfoFail(void)
{
    CF_NoArgsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_SEND_HK_MID, CFE_SB_CMD_HDR_SIZE, TRUE);

    CF_AppData.Chan[0].HandshakeSemId = 1;
    CF_AppData.Chan[1].HandshakeSemId = CF_INVALID;
    CFE_Stubs.OS_CountSemGetInfo_Return = OS_ERROR;
    TestConfigTable.OuCh[0].DequeueEnable = CF_ENABLED;
    TestConfigTable.OuCh[1].DequeueEnable = CF_DISABLED;

    CF_HousekeepingCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.Chan[0].SemValue, 0,
                   "HkCmd sets SemValue to 0 when OS_CountSemGetInfo fails");
}

/* ==========================================================================
 * 5. CF_FreezeCmd tests
 * ========================================================================== */
static void Test_FreezeCmd_Nominal(void)
{
    CF_NoArgsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_NoArgsCmd_t), TRUE);
    CFDP_Stubs.give_request_return = 1;

    CF_FreezeCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "FreezeCmd increments CmdCounter on success");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_FREEZE_CMD_EID,
                   "FreezeCmd sends CF_FREEZE_CMD_EID");
}

static void Test_FreezeCmd_EngineFail(void)
{
    CF_NoArgsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_NoArgsCmd_t), TRUE);
    CFDP_Stubs.give_request_return = 0; /* FALSE / failure */

    CF_FreezeCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "FreezeCmd increments ErrCounter on engine fail");
    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 0, "FreezeCmd does not increment CmdCounter on engine fail");
}

static void Test_FreezeCmd_BadLength(void)
{
    CF_NoArgsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);

    CF_FreezeCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "FreezeCmd bad length increments ErrCounter");
}

/* ==========================================================================
 * 6. CF_ThawCmd tests
 * ========================================================================== */
static void Test_ThawCmd_Nominal(void)
{
    CF_NoArgsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_NoArgsCmd_t), TRUE);
    CFDP_Stubs.give_request_return = 1;

    CF_ThawCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "ThawCmd increments CmdCounter on success");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_THAW_CMD_EID,
                   "ThawCmd sends CF_THAW_CMD_EID");
}

static void Test_ThawCmd_EngineFail(void)
{
    CF_NoArgsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_NoArgsCmd_t), TRUE);
    CFDP_Stubs.give_request_return = 0;

    CF_ThawCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "ThawCmd increments ErrCounter on engine fail");
}

/* ==========================================================================
 * 7. CF_CARSCmd tests (Suspend, Resume, Cancel, Abandon)
 * ========================================================================== */
static void Test_SuspendCmd_BadLength(void)
{
    CF_CARSCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);

    CF_CARSCmd((CFE_SB_MsgPtr_t)&cmd, "Suspend");

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "SuspendCmd bad length increments ErrCounter");
}

static void Test_ResumeCmd_BadLength(void)
{
    CF_CARSCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);

    CF_CARSCmd((CFE_SB_MsgPtr_t)&cmd, "Resume");

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "ResumeCmd bad length increments ErrCounter");
}

static void Test_CancelCmd_BadLength(void)
{
    CF_CARSCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);

    CF_CARSCmd((CFE_SB_MsgPtr_t)&cmd, "Cancel");

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "CancelCmd bad length increments ErrCounter");
}

static void Test_AbandonCmd_BadLength(void)
{
    CF_CARSCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);

    CF_CARSCmd((CFE_SB_MsgPtr_t)&cmd, "Abandon");

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "AbandonCmd bad length increments ErrCounter");
}

/* The CARS commands require CF_ChkTermination, CF_ValidateFilenameReportErr,
 * CF_FindActiveTransIdByName, CF_BuildCmdedRequest, CF_FindNodeByName, etc.
 * which are in cf_utils. We test the top-level flow paths here. */

static void Test_CARSCmd_TransIdPath_NoTermination(void)
{
    CF_CARSCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_CARSCmd_t), TRUE);
    /* Fill Trans completely with non-null chars to fail termination check */
    memset(cmd.Trans, 'A', OS_MAX_PATH_LEN);

    CF_CARSCmd((CFE_SB_MsgPtr_t)&cmd, "Suspend");

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "CARSCmd non-terminated Trans increments ErrCounter");
}

/* ==========================================================================
 * 8. CF_SetMibCmd tests
 * ========================================================================== */
static void Test_SetMibCmd_Nominal(void)
{
    CF_SetMibParam_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_SetMibParam_t), TRUE);
    strncpy(cmd.Param, "ACK_LIMIT", CF_MAX_CFG_PARAM_CHARS);
    strncpy(cmd.Value, "5", CF_MAX_CFG_VALUE_CHARS);
    CFDP_Stubs.set_mib_parameter_return = 1;

    CF_SetMibCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "SetMibCmd increments CmdCounter on success");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_SET_MIB_CMD_EID,
                   "SetMibCmd sends CF_SET_MIB_CMD_EID");
}

static void Test_SetMibCmd_EngineFail(void)
{
    CF_SetMibParam_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_SetMibParam_t), TRUE);
    strncpy(cmd.Param, "ACK_LIMIT", CF_MAX_CFG_PARAM_CHARS);
    strncpy(cmd.Value, "5", CF_MAX_CFG_VALUE_CHARS);
    CFDP_Stubs.set_mib_parameter_return = 0; /* FALSE */

    CF_SetMibCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "SetMibCmd engine fail increments ErrCounter");
}

static void Test_SetMibCmd_ChunkSizeTooLarge(void)
{
    CF_SetMibParam_t cmd;
    char valueBuf[CF_MAX_CFG_VALUE_CHARS];

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_SetMibParam_t), TRUE);
    strncpy(cmd.Param, "outgoing_file_chunk_size", CF_MAX_CFG_PARAM_CHARS);
    /* Set a value larger than max */
    snprintf(valueBuf, sizeof(valueBuf), "%d", CF_MAX_OUTGOING_CHUNK_SIZE + 100);
    strncpy(cmd.Value, valueBuf, CF_MAX_CFG_VALUE_CHARS);

    CF_SetMibCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "SetMibCmd chunk size too large increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_SET_MIB_CMD_ERR1_EID,
                   "SetMibCmd sends CF_SET_MIB_CMD_ERR1_EID for chunk overflow");
}

static void Test_SetMibCmd_AckTimeout(void)
{
    CF_SetMibParam_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_SetMibParam_t), TRUE);
    strncpy(cmd.Param, "ack_timeout", CF_MAX_CFG_PARAM_CHARS);
    strncpy(cmd.Value, "30", CF_MAX_CFG_VALUE_CHARS);
    CFDP_Stubs.set_mib_parameter_return = 1;

    CF_SetMibCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "SetMib ACK_TIMEOUT increments CmdCounter");
    UtAssert_StrEq(TestConfigTable.AckTimeout, "30",
                   "SetMib ACK_TIMEOUT updates table");
}

static void Test_SetMibCmd_NakLimit(void)
{
    CF_SetMibParam_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_SetMibParam_t), TRUE);
    strncpy(cmd.Param, "nak_limit", CF_MAX_CFG_PARAM_CHARS);
    strncpy(cmd.Value, "10", CF_MAX_CFG_VALUE_CHARS);
    CFDP_Stubs.set_mib_parameter_return = 1;

    CF_SetMibCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "SetMib NAK_LIMIT increments CmdCounter");
    UtAssert_StrEq(TestConfigTable.NakLimit, "10", "SetMib NAK_LIMIT updates table");
}

static void Test_SetMibCmd_BadLength(void)
{
    CF_SetMibParam_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);

    CF_SetMibCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "SetMibCmd bad length increments ErrCounter");
}

static void Test_SetMibCmd_ParamNotTerminated(void)
{
    CF_SetMibParam_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_SetMibParam_t), TRUE);
    memset(cmd.Param, 'A', CF_MAX_CFG_PARAM_CHARS); /* no null terminator */
    strncpy(cmd.Value, "5", CF_MAX_CFG_VALUE_CHARS);

    CF_SetMibCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "SetMibCmd non-terminated param increments ErrCounter");
}

static void Test_SetMibCmd_ValueNotTerminated(void)
{
    CF_SetMibParam_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_SetMibParam_t), TRUE);
    strncpy(cmd.Param, "ACK_LIMIT", CF_MAX_CFG_PARAM_CHARS);
    memset(cmd.Value, 'B', CF_MAX_CFG_VALUE_CHARS); /* no null terminator */

    CF_SetMibCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "SetMibCmd non-terminated value increments ErrCounter");
}

/* ==========================================================================
 * 9. CF_GetMibCmd tests
 * ========================================================================== */
static void Test_GetMibCmd_Nominal(void)
{
    CF_GetMibParam_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_GetMibParam_t), TRUE);
    strncpy(cmd.Param, "ACK_LIMIT", CF_MAX_CFG_PARAM_CHARS);
    CFDP_Stubs.get_mib_parameter_return = 1;

    CF_GetMibCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "GetMibCmd increments CmdCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_GET_MIB_CMD_EID,
                   "GetMibCmd sends CF_GET_MIB_CMD_EID");
}

static void Test_GetMibCmd_EngineFail(void)
{
    CF_GetMibParam_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_GetMibParam_t), TRUE);
    strncpy(cmd.Param, "ACK_LIMIT", CF_MAX_CFG_PARAM_CHARS);
    CFDP_Stubs.get_mib_parameter_return = 0;

    CF_GetMibCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "GetMibCmd engine fail increments ErrCounter");
}

static void Test_GetMibCmd_BadLength(void)
{
    CF_GetMibParam_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);

    CF_GetMibCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "GetMibCmd bad length increments ErrCounter");
}

static void Test_GetMibCmd_ParamNotTerminated(void)
{
    CF_GetMibParam_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_GetMibParam_t), TRUE);
    memset(cmd.Param, 'A', CF_MAX_CFG_PARAM_CHARS);

    CF_GetMibCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "GetMibCmd non-terminated param increments ErrCounter");
}

/* ==========================================================================
 * 10. CF_SendCfgParams tests
 * ========================================================================== */
static void Test_SendCfgParams_Nominal(void)
{
    CF_NoArgsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_NoArgsCmd_t), TRUE);
    CFDP_Stubs.get_mib_parameter_return = 1;
    TestConfigTable.NumEngCyclesPerWakeup = 4;

    CF_SendCfgParams((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "SendCfgParams increments CmdCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_SND_CFG_CMD_EID,
                   "SendCfgParams sends CF_SND_CFG_CMD_EID");
    UtAssert_IntEq(CF_AppData.CfgPkt.EngCycPerWakeup, 4,
                   "SendCfgParams populates EngCycPerWakeup");
    UtAssert_True(CFE_Stubs.CFE_SB_SendMsg_CallCount > 0,
                  "SendCfgParams sends telemetry");
}

static void Test_SendCfgParams_BadLength(void)
{
    CF_NoArgsCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);

    CF_SendCfgParams((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "SendCfgParams bad length increments ErrCounter");
}

/* ==========================================================================
 * 11. CF_SetPollParam tests
 * ========================================================================== */
static void Test_SetPollParam_Nominal(void)
{
    CF_SetPollParamCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_SetPollParamCmd_t), TRUE);
    cmd.Chan = 0;
    cmd.Dir = 0;
    cmd.Class = CF_CLASS_1;
    cmd.Priority = 1;
    cmd.Preserve = CF_DELETE_FILE;
    strncpy(cmd.PeerEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);
    strncpy(cmd.SrcPath, "/cf/", OS_MAX_PATH_LEN);
    strncpy(cmd.DstPath, "/gnd/", OS_MAX_PATH_LEN);

    CF_SetPollParam((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "SetPollParam nominal increments CmdCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_SET_POLL_PARAM1_EID,
                   "SetPollParam sends CF_SET_POLL_PARAM1_EID");
}

static void Test_SetPollParam_InvalidChan(void)
{
    CF_SetPollParamCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_SetPollParamCmd_t), TRUE);
    cmd.Chan = CF_MAX_PLAYBACK_CHANNELS; /* invalid */
    cmd.Dir = 0;
    cmd.Class = CF_CLASS_1;
    cmd.Preserve = 0;
    strncpy(cmd.PeerEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);
    strncpy(cmd.SrcPath, "/cf/", OS_MAX_PATH_LEN);
    strncpy(cmd.DstPath, "/gnd/", OS_MAX_PATH_LEN);

    CF_SetPollParam((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "SetPollParam invalid chan increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_SET_POLL_PARAM_ERR1_EID,
                   "SetPollParam sends CF_SET_POLL_PARAM_ERR1_EID");
}

static void Test_SetPollParam_InvalidDir(void)
{
    CF_SetPollParamCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_SetPollParamCmd_t), TRUE);
    cmd.Chan = 0;
    cmd.Dir = CF_MAX_POLLING_DIRS_PER_CHAN; /* invalid */
    cmd.Class = CF_CLASS_1;
    cmd.Preserve = 0;
    strncpy(cmd.PeerEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);
    strncpy(cmd.SrcPath, "/cf/", OS_MAX_PATH_LEN);
    strncpy(cmd.DstPath, "/gnd/", OS_MAX_PATH_LEN);

    CF_SetPollParam((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "SetPollParam invalid dir increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_SET_POLL_PARAM_ERR2_EID,
                   "SetPollParam sends CF_SET_POLL_PARAM_ERR2_EID");
}

static void Test_SetPollParam_InvalidClass(void)
{
    CF_SetPollParamCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_SetPollParamCmd_t), TRUE);
    cmd.Chan = 0;
    cmd.Dir = 0;
    cmd.Class = 0; /* invalid - must be 1 or 2 */
    cmd.Preserve = 0;
    strncpy(cmd.PeerEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);
    strncpy(cmd.SrcPath, "/cf/", OS_MAX_PATH_LEN);
    strncpy(cmd.DstPath, "/gnd/", OS_MAX_PATH_LEN);

    CF_SetPollParam((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "SetPollParam invalid class increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_SET_POLL_PARAM_ERR3_EID,
                   "SetPollParam sends CF_SET_POLL_PARAM_ERR3_EID");
}

static void Test_SetPollParam_InvalidPreserve(void)
{
    CF_SetPollParamCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_SetPollParamCmd_t), TRUE);
    cmd.Chan = 0;
    cmd.Dir = 0;
    cmd.Class = CF_CLASS_1;
    cmd.Preserve = CF_KEEP_FILE + 1; /* invalid */
    strncpy(cmd.PeerEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);
    strncpy(cmd.SrcPath, "/cf/", OS_MAX_PATH_LEN);
    strncpy(cmd.DstPath, "/gnd/", OS_MAX_PATH_LEN);

    CF_SetPollParam((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "SetPollParam invalid preserve increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_SET_POLL_PARAM_ERR4_EID,
                   "SetPollParam sends CF_SET_POLL_PARAM_ERR4_EID");
}

/* ==========================================================================
 * 12. CF_WriteQueueCmd tests
 * ========================================================================== */
static void Test_WriteQueueCmd_UplinkInvalidQueue(void)
{
    CF_WriteQueueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_WriteQueueCmd_t), TRUE);
    cmd.Type = CF_UPLINK;
    cmd.Queue = CF_PENDINGQ; /* 0 is invalid for uplink, needs ACTIVEQ(1) or HISTORYQ(2) */
    cmd.Chan = 0;
    cmd.Filename[0] = '\0';

    CF_WriteQueueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "WriteQueueCmd invalid uplink queue increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_WR_CMD_ERR1_EID,
                   "WriteQueueCmd sends CF_WR_CMD_ERR1_EID");
}

static void Test_WriteQueueCmd_PlaybackInvalidQueue(void)
{
    CF_WriteQueueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_WriteQueueCmd_t), TRUE);
    cmd.Type = CF_PLAYBACK;
    cmd.Queue = 5; /* invalid, max is 2 */
    cmd.Chan = 0;
    cmd.Filename[0] = '\0';

    CF_WriteQueueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "WriteQueueCmd invalid playback queue increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_WR_CMD_ERR2_EID,
                   "WriteQueueCmd sends CF_WR_CMD_ERR2_EID");
}

static void Test_WriteQueueCmd_InvalidType(void)
{
    CF_WriteQueueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_WriteQueueCmd_t), TRUE);
    cmd.Type = 99; /* invalid */
    cmd.Queue = 0;
    cmd.Chan = 0;
    cmd.Filename[0] = '\0';

    CF_WriteQueueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "WriteQueueCmd invalid type increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_WR_CMD_ERR3_EID,
                   "WriteQueueCmd sends CF_WR_CMD_ERR3_EID");
}

static void Test_WriteQueueCmd_PlaybackInvalidChan(void)
{
    CF_WriteQueueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_WriteQueueCmd_t), TRUE);
    cmd.Type = CF_PLAYBACK;
    cmd.Queue = 0;
    cmd.Chan = CF_MAX_PLAYBACK_CHANNELS; /* invalid */
    cmd.Filename[0] = '\0';

    CF_WriteQueueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "WriteQueueCmd invalid playback chan increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_WR_CMD_ERR4_EID,
                   "WriteQueueCmd sends CF_WR_CMD_ERR4_EID");
}

/* ==========================================================================
 * 13. CF_WriteQueueInfo tests
 * ========================================================================== */
static void Test_WriteQueueInfo_FileCreateFail(void)
{
    int32 result;

    CFE_Stubs.OS_creat_Return = -1; /* fail */

    result = CF_WriteQueueInfo("/ram/testfile.dat", NULL);

    UtAssert_IntEq(result, CF_ERROR, "WriteQueueInfo returns CF_ERROR on file create fail");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_SND_QUE_ERR1_EID,
                   "WriteQueueInfo sends CF_SND_QUE_ERR1_EID");
}

static void Test_WriteQueueInfo_WriteHeaderFail(void)
{
    int32 result;

    CFE_Stubs.OS_creat_Return = 5;
    CFE_Stubs.CFE_FS_WriteHeader_Return = 0; /* wrong byte count */

    result = CF_WriteQueueInfo("/ram/testfile.dat", NULL);

    UtAssert_IntEq(result, CF_ERROR, "WriteQueueInfo returns CF_ERROR on header write fail");
}

static void Test_WriteQueueInfo_EmptyQueue(void)
{
    int32 result;

    CFE_Stubs.OS_creat_Return = 5;
    CFE_Stubs.CFE_FS_WriteHeader_Return = sizeof(CFE_FS_Header_t);

    result = CF_WriteQueueInfo("/ram/testfile.dat", NULL);

    UtAssert_IntEq(result, CF_SUCCESS, "WriteQueueInfo returns CF_SUCCESS for empty queue");
}

/* ==========================================================================
 * 14. CF_WriteActiveTransCmd tests
 * ========================================================================== */
static void Test_WriteActiveTransCmd_InvalidType(void)
{
    CF_WriteActiveTransCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_WriteActiveTransCmd_t), TRUE);
    cmd.Type = CF_PLAYBACK + 1; /* invalid */
    cmd.Filename[0] = '\0';

    CF_WriteActiveTransCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "WriteActiveTransCmd invalid type increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_WRACT_ERR1_EID,
                   "WriteActiveTransCmd sends CF_WRACT_ERR1_EID");
}

static void Test_WriteActiveTransCmd_BadLength(void)
{
    CF_WriteActiveTransCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);

    CF_WriteActiveTransCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "WriteActiveTransCmd bad length increments ErrCounter");
}

/* ==========================================================================
 * 15. CF_EnableDequeueCmd tests
 * ========================================================================== */
static void Test_EnableDequeueCmd_ValidChan(void)
{
    CF_EnDisDequeueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_EnDisPollCmd_t), TRUE);
    cmd.Chan = 0;

    CF_EnableDequeueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "EnableDequeueCmd increments CmdCounter");
    UtAssert_IntEq(TestConfigTable.OuCh[0].DequeueEnable, CF_ENABLED,
                   "EnableDequeueCmd enables dequeue");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_ENA_DQ_CMD_EID,
                   "EnableDequeueCmd sends CF_ENA_DQ_CMD_EID");
}

static void Test_EnableDequeueCmd_InvalidChan(void)
{
    CF_EnDisDequeueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_EnDisPollCmd_t), TRUE);
    cmd.Chan = CF_MAX_PLAYBACK_CHANNELS;

    CF_EnableDequeueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "EnableDequeueCmd invalid chan increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_DQ_CMD_ERR1_EID,
                   "EnableDequeueCmd sends CF_DQ_CMD_ERR1_EID");
}

/* ==========================================================================
 * 16. CF_DisableDequeueCmd tests
 * ========================================================================== */
static void Test_DisableDequeueCmd_ValidChan(void)
{
    CF_EnDisDequeueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_EnDisPollCmd_t), TRUE);
    cmd.Chan = 0;
    TestConfigTable.OuCh[0].DequeueEnable = CF_ENABLED;

    CF_DisableDequeueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "DisableDequeueCmd increments CmdCounter");
    UtAssert_IntEq(TestConfigTable.OuCh[0].DequeueEnable, CF_DISABLED,
                   "DisableDequeueCmd disables dequeue");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_DIS_DQ_CMD_EID,
                   "DisableDequeueCmd sends CF_DIS_DQ_CMD_EID");
}

static void Test_DisableDequeueCmd_InvalidChan(void)
{
    CF_EnDisDequeueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_EnDisPollCmd_t), TRUE);
    cmd.Chan = CF_MAX_PLAYBACK_CHANNELS;

    CF_DisableDequeueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "DisableDequeueCmd invalid chan increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_DQ_CMD_ERR2_EID,
                   "DisableDequeueCmd sends CF_DQ_CMD_ERR2_EID");
}

/* ==========================================================================
 * 17. CF_EnablePollCmd tests
 * ========================================================================== */
static void Test_EnablePollCmd_SpecificDir(void)
{
    CF_EnDisPollCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_EnDisPollCmd_t), TRUE);
    cmd.Chan = 0;
    cmd.Dir = 0;
    TestConfigTable.OuCh[0].PollDir[0].EntryInUse = CF_ENTRY_IN_USE;

    CF_EnablePollCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "EnablePollCmd specific dir increments CmdCounter");
    UtAssert_IntEq(TestConfigTable.OuCh[0].PollDir[0].EnableState, CF_ENABLED,
                   "EnablePollCmd enables specific poll dir");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_ENA_POLL_CMD2_EID,
                   "EnablePollCmd sends CF_ENA_POLL_CMD2_EID for specific dir");
}

static void Test_EnablePollCmd_AllDirs(void)
{
    CF_EnDisPollCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_EnDisPollCmd_t), TRUE);
    cmd.Chan = 0;
    cmd.Dir = 0xFF; /* all directories */
    TestConfigTable.OuCh[0].PollDir[0].EntryInUse = CF_ENTRY_IN_USE;
    TestConfigTable.OuCh[0].PollDir[1].EntryInUse = CF_ENTRY_IN_USE;

    CF_EnablePollCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "EnablePollCmd all dirs increments CmdCounter");
    UtAssert_IntEq(TestConfigTable.OuCh[0].PollDir[0].EnableState, CF_ENABLED,
                   "EnablePollCmd enables dir 0");
    UtAssert_IntEq(TestConfigTable.OuCh[0].PollDir[1].EnableState, CF_ENABLED,
                   "EnablePollCmd enables dir 1");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_ENA_POLL_CMD1_EID,
                   "EnablePollCmd sends CF_ENA_POLL_CMD1_EID for all dirs");
}

static void Test_EnablePollCmd_InvalidChan(void)
{
    CF_EnDisPollCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_EnDisPollCmd_t), TRUE);
    cmd.Chan = CF_MAX_PLAYBACK_CHANNELS;
    cmd.Dir = 0;

    CF_EnablePollCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "EnablePollCmd invalid chan increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_ENA_POLL_ERR1_EID,
                   "EnablePollCmd sends CF_ENA_POLL_ERR1_EID");
}

static void Test_EnablePollCmd_InvalidDir(void)
{
    CF_EnDisPollCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_EnDisPollCmd_t), TRUE);
    cmd.Chan = 0;
    cmd.Dir = CF_MAX_POLLING_DIRS_PER_CHAN; /* invalid - not 0xFF and >= max */

    CF_EnablePollCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "EnablePollCmd invalid dir increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_ENA_POLL_ERR2_EID,
                   "EnablePollCmd sends CF_ENA_POLL_ERR2_EID");
}

/* ==========================================================================
 * 18. CF_DisablePollCmd tests
 * ========================================================================== */
static void Test_DisablePollCmd_SpecificDir(void)
{
    CF_EnDisPollCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_EnDisPollCmd_t), TRUE);
    cmd.Chan = 0;
    cmd.Dir = 0;
    TestConfigTable.OuCh[0].PollDir[0].EntryInUse = CF_ENTRY_IN_USE;
    TestConfigTable.OuCh[0].PollDir[0].EnableState = CF_ENABLED;

    CF_DisablePollCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "DisablePollCmd specific dir increments CmdCounter");
    UtAssert_IntEq(TestConfigTable.OuCh[0].PollDir[0].EnableState, CF_DISABLED,
                   "DisablePollCmd disables specific poll dir");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_DIS_POLL_CMD2_EID,
                   "DisablePollCmd sends CF_DIS_POLL_CMD2_EID");
}

static void Test_DisablePollCmd_AllDirs(void)
{
    CF_EnDisPollCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_EnDisPollCmd_t), TRUE);
    cmd.Chan = 0;
    cmd.Dir = 0xFF;
    TestConfigTable.OuCh[0].PollDir[0].EntryInUse = CF_ENTRY_IN_USE;
    TestConfigTable.OuCh[0].PollDir[0].EnableState = CF_ENABLED;
    TestConfigTable.OuCh[0].PollDir[1].EntryInUse = CF_ENTRY_IN_USE;
    TestConfigTable.OuCh[0].PollDir[1].EnableState = CF_ENABLED;

    CF_DisablePollCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "DisablePollCmd all dirs increments CmdCounter");
    UtAssert_IntEq(TestConfigTable.OuCh[0].PollDir[0].EnableState, CF_DISABLED,
                   "DisablePollCmd disables dir 0");
    UtAssert_IntEq(TestConfigTable.OuCh[0].PollDir[1].EnableState, CF_DISABLED,
                   "DisablePollCmd disables dir 1");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_DIS_POLL_CMD1_EID,
                   "DisablePollCmd sends CF_DIS_POLL_CMD1_EID for all dirs");
}

static void Test_DisablePollCmd_InvalidChan(void)
{
    CF_EnDisPollCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_EnDisPollCmd_t), TRUE);
    cmd.Chan = CF_MAX_PLAYBACK_CHANNELS;
    cmd.Dir = 0;

    CF_DisablePollCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "DisablePollCmd invalid chan increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_DIS_POLL_ERR1_EID,
                   "DisablePollCmd sends CF_DIS_POLL_ERR1_EID");
}

static void Test_DisablePollCmd_InvalidDir(void)
{
    CF_EnDisPollCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_EnDisPollCmd_t), TRUE);
    cmd.Chan = 0;
    cmd.Dir = CF_MAX_POLLING_DIRS_PER_CHAN;

    CF_DisablePollCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "DisablePollCmd invalid dir increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_DIS_POLL_ERR2_EID,
                   "DisablePollCmd sends CF_DIS_POLL_ERR2_EID");
}

/* ==========================================================================
 * 19. CF_KickstartCmd tests
 * ========================================================================== */
static void Test_KickstartCmd_Nominal(void)
{
    CF_KickstartCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_KickstartCmd_t), TRUE);
    cmd.Chan = 0;
    CF_AppData.Chan[0].DataBlast = CF_IN_PROGRESS;

    CF_KickstartCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "KickstartCmd increments CmdCounter");
    UtAssert_IntEq(CF_AppData.Chan[0].DataBlast, CF_NOT_IN_PROGRESS,
                   "KickstartCmd sets DataBlast to NOT_IN_PROGRESS");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_KICKSTART_CMD_EID,
                   "KickstartCmd sends CF_KICKSTART_CMD_EID");
}

static void Test_KickstartCmd_InvalidChan(void)
{
    CF_KickstartCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_KickstartCmd_t), TRUE);
    cmd.Chan = CF_MAX_PLAYBACK_CHANNELS;

    CF_KickstartCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "KickstartCmd invalid chan increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_KICKSTART_ERR1_EID,
                   "KickstartCmd sends CF_KICKSTART_ERR1_EID");
}

static void Test_KickstartCmd_BadLength(void)
{
    CF_KickstartCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);

    CF_KickstartCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "KickstartCmd bad length increments ErrCounter");
}

/* ==========================================================================
 * 20. CF_QuickStatusCmd tests
 * ========================================================================== */
static void Test_QuickStatusCmd_BadLength(void)
{
    CF_QuickStatCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);

    CF_QuickStatusCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "QuickStatusCmd bad length increments ErrCounter");
}

static void Test_QuickStatusCmd_NotTerminated(void)
{
    CF_QuickStatCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_QuickStatCmd_t), TRUE);
    memset(cmd.Trans, 'A', OS_MAX_PATH_LEN);

    CF_QuickStatusCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "QuickStatusCmd non-terminated trans increments ErrCounter");
}

/* ==========================================================================
 * 21. CF_GiveTakeSemaphoreCmd tests
 * ========================================================================== */
static void Test_GiveTakeSemCmd_GiveSuccess(void)
{
    CF_GiveTakeCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_GiveTakeCmd_t), TRUE);
    cmd.Chan = 0;
    cmd.GiveOrTakeSemaphore = CF_GIVE_SEMAPHORE;
    CF_AppData.Chan[0].HandshakeSemId = 1; /* valid */
    CFE_Stubs.OS_CountSemGive_Return = OS_SUCCESS;

    CF_GiveTakeSemaphoreCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "GiveTake give success increments CmdCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_GIVETAKE_CMD_EID,
                   "GiveTake give sends CF_GIVETAKE_CMD_EID");
}

static void Test_GiveTakeSemCmd_TakeSuccess(void)
{
    CF_GiveTakeCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_GiveTakeCmd_t), TRUE);
    cmd.Chan = 0;
    cmd.GiveOrTakeSemaphore = CF_TAKE_SEMAPHORE;
    CF_AppData.Chan[0].HandshakeSemId = 1;
    CFE_Stubs.OS_CountSemTimedWait_Return = OS_SUCCESS;

    CF_GiveTakeSemaphoreCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "GiveTake take success increments CmdCounter");
}

static void Test_GiveTakeSemCmd_InvalidSemaphore(void)
{
    CF_GiveTakeCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_GiveTakeCmd_t), TRUE);
    cmd.Chan = 0;
    cmd.GiveOrTakeSemaphore = CF_GIVE_SEMAPHORE;
    CF_AppData.Chan[0].HandshakeSemId = CF_INVALID;

    CF_GiveTakeSemaphoreCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "GiveTake invalid semaphore increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_GIVETAKE_ERR1_EID,
                   "GiveTake sends CF_GIVETAKE_ERR1_EID");
}

static void Test_GiveTakeSemCmd_InvalidChan(void)
{
    CF_GiveTakeCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_GiveTakeCmd_t), TRUE);
    cmd.Chan = CF_MAX_PLAYBACK_CHANNELS;
    cmd.GiveOrTakeSemaphore = CF_GIVE_SEMAPHORE;
    /* Set a valid sem to skip the first check */
    CF_AppData.Chan[cmd.Chan].HandshakeSemId = 1;

    CF_GiveTakeSemaphoreCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "GiveTake invalid chan increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_GIVETAKE_ERR2_EID,
                   "GiveTake sends CF_GIVETAKE_ERR2_EID");
}

static void Test_GiveTakeSemCmd_InvalidGiveOrTake(void)
{
    CF_GiveTakeCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_GiveTakeCmd_t), TRUE);
    cmd.Chan = 0;
    cmd.GiveOrTakeSemaphore = 99; /* invalid */
    CF_AppData.Chan[0].HandshakeSemId = 1;

    CF_GiveTakeSemaphoreCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "GiveTake invalid GiveOrTake increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_GIVETAKE_ERR3_EID,
                   "GiveTake sends CF_GIVETAKE_ERR3_EID");
}

static void Test_GiveTakeSemCmd_SemFail(void)
{
    CF_GiveTakeCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_GiveTakeCmd_t), TRUE);
    cmd.Chan = 0;
    cmd.GiveOrTakeSemaphore = CF_GIVE_SEMAPHORE;
    CF_AppData.Chan[0].HandshakeSemId = 1;
    CFE_Stubs.OS_CountSemGive_Return = OS_ERROR;

    CF_GiveTakeSemaphoreCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "GiveTake sem failure increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_GIVETAKE_ERR4_EID,
                   "GiveTake sends CF_GIVETAKE_ERR4_EID");
}

static void Test_GiveTakeSemCmd_BadLength(void)
{
    CF_GiveTakeCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);

    CF_GiveTakeSemaphoreCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "GiveTake bad length increments ErrCounter");
}

/* ==========================================================================
 * 22. CF_PurgeQueueCmd tests
 * ========================================================================== */
static void Test_PurgeQueueCmd_UplinkHistory(void)
{
    CF_PurgeQueueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_PurgeQueueCmd_t), TRUE);
    cmd.Type = CF_INCOMING;
    cmd.Queue = CF_HISTORYQ;
    cmd.Chan = 0;

    /* Empty queue - no nodes to purge */
    CF_AppData.UpQ[CF_UP_HISTORYQ].HeadPtr = NULL;

    CF_PurgeQueueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1,
                   "PurgeQueue uplink history increments CmdCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_PURGEQ1_EID,
                   "PurgeQueue uplink sends CF_PURGEQ1_EID");
}

static void Test_PurgeQueueCmd_UplinkActiveNotAllowed(void)
{
    CF_PurgeQueueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_PurgeQueueCmd_t), TRUE);
    cmd.Type = CF_INCOMING;
    cmd.Queue = CF_ACTIVEQ;
    cmd.Chan = 0;

    CF_PurgeQueueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "PurgeQueue uplink active increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_PURGEQ_ERR1_EID,
                   "PurgeQueue uplink active sends CF_PURGEQ_ERR1_EID");
}

static void Test_PurgeQueueCmd_UplinkInvalidQueue(void)
{
    CF_PurgeQueueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_PurgeQueueCmd_t), TRUE);
    cmd.Type = CF_INCOMING;
    cmd.Queue = CF_PENDINGQ; /* invalid for incoming - not ACTIVEQ and not HISTORYQ */
    cmd.Chan = 0;

    CF_PurgeQueueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "PurgeQueue uplink invalid queue increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_PURGEQ_ERR2_EID,
                   "PurgeQueue uplink invalid queue sends CF_PURGEQ_ERR2_EID");
}

static void Test_PurgeQueueCmd_OutgoingActiveNotAllowed(void)
{
    CF_PurgeQueueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_PurgeQueueCmd_t), TRUE);
    cmd.Type = CF_OUTGOING;
    cmd.Queue = CF_PB_ACTIVEQ;
    cmd.Chan = 0;

    CF_PurgeQueueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "PurgeQueue outgoing active increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_PURGEQ_ERR3_EID,
                   "PurgeQueue outgoing active sends CF_PURGEQ_ERR3_EID");
}

static void Test_PurgeQueueCmd_OutgoingInvalidQueue(void)
{
    CF_PurgeQueueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_PurgeQueueCmd_t), TRUE);
    cmd.Type = CF_OUTGOING;
    cmd.Queue = CF_PB_HISTORYQ + 1; /* invalid */
    cmd.Chan = 0;

    CF_PurgeQueueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "PurgeQueue outgoing invalid queue increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_PURGEQ_ERR4_EID,
                   "PurgeQueue outgoing invalid queue sends CF_PURGEQ_ERR4_EID");
}

static void Test_PurgeQueueCmd_OutgoingInvalidChan(void)
{
    CF_PurgeQueueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_PurgeQueueCmd_t), TRUE);
    cmd.Type = CF_OUTGOING;
    cmd.Queue = CF_PB_PENDINGQ;
    cmd.Chan = CF_MAX_PLAYBACK_CHANNELS;

    CF_PurgeQueueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "PurgeQueue outgoing invalid chan increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_PURGEQ_ERR5_EID,
                   "PurgeQueue outgoing invalid chan sends CF_PURGEQ_ERR5_EID");
}

static void Test_PurgeQueueCmd_InvalidType(void)
{
    CF_PurgeQueueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_PurgeQueueCmd_t), TRUE);
    cmd.Type = 99; /* invalid */
    cmd.Queue = 0;
    cmd.Chan = 0;

    CF_PurgeQueueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "PurgeQueue invalid type increments ErrCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_PURGEQ_ERR6_EID,
                   "PurgeQueue invalid type sends CF_PURGEQ_ERR6_EID");
}

static void Test_PurgeQueueCmd_OutgoingPending(void)
{
    CF_PurgeQueueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_PurgeQueueCmd_t), TRUE);
    cmd.Type = CF_OUTGOING;
    cmd.Queue = CF_PB_PENDINGQ;
    cmd.Chan = 0;

    /* Empty queue */
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr = NULL;

    CF_PurgeQueueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1,
                   "PurgeQueue outgoing pending increments CmdCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_PURGEQ2_EID,
                   "PurgeQueue outgoing pending sends CF_PURGEQ2_EID");
}

static void Test_PurgeQueueCmd_OutgoingHistory(void)
{
    CF_PurgeQueueCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_PurgeQueueCmd_t), TRUE);
    cmd.Type = CF_OUTGOING;
    cmd.Queue = CF_PB_HISTORYQ;
    cmd.Chan = 0;

    CF_AppData.Chan[0].PbQ[CF_PB_HISTORYQ].HeadPtr = NULL;

    CF_PurgeQueueCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1,
                   "PurgeQueue outgoing history increments CmdCounter");
}

/* ==========================================================================
 * 23. CF_AutoSuspendEnCmd tests
 * ========================================================================== */
static void Test_AutoSuspendEnCmd_Enable(void)
{
    CF_AutoSuspendEnCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_AutoSuspendEnCmd_t), TRUE);
    cmd.EnableDisable = CF_ENABLED;
    CF_AppData.MsgPtr = (CFE_SB_MsgPtr_t)&cmd;

    CF_AutoSuspendEnCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.AutoSuspend.EnFlag, CF_ENABLED,
                   "AutoSuspendEnCmd enables auto suspend");
    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1,
                   "AutoSuspendEnCmd increments CmdCounter");
    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_ENDIS_AUTO_SUS_CMD_EID,
                   "AutoSuspendEnCmd sends CF_ENDIS_AUTO_SUS_CMD_EID");
}

static void Test_AutoSuspendEnCmd_Disable(void)
{
    CF_AutoSuspendEnCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_AutoSuspendEnCmd_t), TRUE);
    cmd.EnableDisable = CF_DISABLED;
    CF_AppData.MsgPtr = (CFE_SB_MsgPtr_t)&cmd;
    CF_AppData.Hk.AutoSuspend.EnFlag = CF_ENABLED;

    CF_AutoSuspendEnCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.AutoSuspend.EnFlag, CF_DISABLED,
                   "AutoSuspendEnCmd disables auto suspend");
    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1,
                   "AutoSuspendEnCmd disable increments CmdCounter");
}

static void Test_AutoSuspendEnCmd_BadLength(void)
{
    CF_AutoSuspendEnCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);
    CF_AppData.MsgPtr = (CFE_SB_MsgPtr_t)&cmd;

    CF_AutoSuspendEnCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "AutoSuspendEnCmd bad length increments ErrCounter");
}

/* ==========================================================================
 * 24. CF_IncrCmdCtr tests
 * ========================================================================== */
static void Test_IncrCmdCtr_Success(void)
{
    CF_AppData.Hk.CmdCounter = 0;
    CF_AppData.Hk.ErrCounter = 0;

    CF_IncrCmdCtr(CF_SUCCESS);

    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 1, "IncrCmdCtr increments CmdCounter on SUCCESS");
    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 0, "IncrCmdCtr does not increment ErrCounter on SUCCESS");
}

static void Test_IncrCmdCtr_Error(void)
{
    CF_AppData.Hk.CmdCounter = 0;
    CF_AppData.Hk.ErrCounter = 0;

    CF_IncrCmdCtr(CF_ERROR);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1, "IncrCmdCtr increments ErrCounter on ERROR");
    UtAssert_IntEq(CF_AppData.Hk.CmdCounter, 0, "IncrCmdCtr does not increment CmdCounter on ERROR");
}

/* ==========================================================================
 * 25. CF_FileWriteByteCntErr tests
 * ========================================================================== */
static void Test_FileWriteByteCntErr(void)
{
    CF_FileWriteByteCntErr("testfile.dat", 100, 50);

    UtAssert_IntEq(CFE_Stubs.EVS_SendEvent_LastEventID, CF_FILEWRITE_ERR_EID,
                   "FileWriteByteCntErr sends CF_FILEWRITE_ERR_EID");
}

/* ==========================================================================
 * 26. CF_SendTransDataCmd tests
 * ========================================================================== */
static void Test_SendTransDataCmd_BadLength(void)
{
    CF_SendTransCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);

    CF_SendTransDataCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "SendTransDataCmd bad length increments ErrCounter");
}

static void Test_SendTransDataCmd_NotTerminated(void)
{
    CF_SendTransCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_SendTransCmd_t), TRUE);
    memset(cmd.Trans, 'A', OS_MAX_PATH_LEN);

    CF_SendTransDataCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "SendTransDataCmd non-terminated increments ErrCounter");
}

/* ==========================================================================
 * 27. CF_DequeueNodeCmd tests
 * ========================================================================== */
static void Test_DequeueNodeCmd_BadLength(void)
{
    CF_DequeueNodeCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, 1, TRUE);

    CF_DequeueNodeCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "DequeueNodeCmd bad length increments ErrCounter");
}

static void Test_DequeueNodeCmd_NotTerminated(void)
{
    CF_DequeueNodeCmd_t cmd;

    CFE_SB_InitMsg(&cmd, CF_CMD_MID, sizeof(CF_DequeueNodeCmd_t), TRUE);
    memset(cmd.Trans, 'A', OS_MAX_PATH_LEN);

    CF_DequeueNodeCmd((CFE_SB_MsgPtr_t)&cmd);

    UtAssert_IntEq(CF_AppData.Hk.ErrCounter, 1,
                   "DequeueNodeCmd non-terminated increments ErrCounter");
}

/* ==========================================================================
 * Registration
 * ========================================================================== */
void CF_Cmds_AddTests(void)
{
    /* CF_VerifyCmdLength */
    UtTest_Add(Test_VerifyCmdLength_CorrectLength, Test_Setup, Test_Teardown,
               "CF_VerifyCmdLength - correct length");
    UtTest_Add(Test_VerifyCmdLength_WrongLength, Test_Setup, Test_Teardown,
               "CF_VerifyCmdLength - wrong length");

    /* CF_NoopCmd */
    UtTest_Add(Test_NoopCmd_Nominal, Test_Setup, Test_Teardown,
               "CF_NoopCmd - nominal");
    UtTest_Add(Test_NoopCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_NoopCmd - bad length");

    /* CF_ResetCtrsCmd */
    UtTest_Add(Test_ResetCtrsCmd_All, Test_Setup, Test_Teardown,
               "CF_ResetCtrsCmd - reset all");
    UtTest_Add(Test_ResetCtrsCmd_CmdOnly, Test_Setup, Test_Teardown,
               "CF_ResetCtrsCmd - cmd only");
    UtTest_Add(Test_ResetCtrsCmd_FaultOnly, Test_Setup, Test_Teardown,
               "CF_ResetCtrsCmd - fault only");
    UtTest_Add(Test_ResetCtrsCmd_UplinkOnly, Test_Setup, Test_Teardown,
               "CF_ResetCtrsCmd - uplink only");
    UtTest_Add(Test_ResetCtrsCmd_DownlinkOnly, Test_Setup, Test_Teardown,
               "CF_ResetCtrsCmd - downlink only");
    UtTest_Add(Test_ResetCtrsCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_ResetCtrsCmd - bad length");

    /* CF_HousekeepingCmd */
    UtTest_Add(Test_HousekeepingCmd_Nominal, Test_Setup, Test_Teardown,
               "CF_HousekeepingCmd - nominal");
    UtTest_Add(Test_HousekeepingCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_HousekeepingCmd - bad length");
    UtTest_Add(Test_HousekeepingCmd_WithSemaphore, Test_Setup, Test_Teardown,
               "CF_HousekeepingCmd - with valid semaphore");
    UtTest_Add(Test_HousekeepingCmd_SemGetInfoFail, Test_Setup, Test_Teardown,
               "CF_HousekeepingCmd - sem get info fail");

    /* CF_FreezeCmd */
    UtTest_Add(Test_FreezeCmd_Nominal, Test_Setup, Test_Teardown,
               "CF_FreezeCmd - nominal");
    UtTest_Add(Test_FreezeCmd_EngineFail, Test_Setup, Test_Teardown,
               "CF_FreezeCmd - engine fail");
    UtTest_Add(Test_FreezeCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_FreezeCmd - bad length");

    /* CF_ThawCmd */
    UtTest_Add(Test_ThawCmd_Nominal, Test_Setup, Test_Teardown,
               "CF_ThawCmd - nominal");
    UtTest_Add(Test_ThawCmd_EngineFail, Test_Setup, Test_Teardown,
               "CF_ThawCmd - engine fail");

    /* CF_CARSCmd (Suspend/Resume/Cancel/Abandon) */
    UtTest_Add(Test_SuspendCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_SuspendCmd - bad length");
    UtTest_Add(Test_ResumeCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_ResumeCmd - bad length");
    UtTest_Add(Test_CancelCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_CancelCmd - bad length");
    UtTest_Add(Test_AbandonCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_AbandonCmd - bad length");
    UtTest_Add(Test_CARSCmd_TransIdPath_NoTermination, Test_Setup, Test_Teardown,
               "CF_CARSCmd - trans not terminated");

    /* CF_SetMibCmd */
    UtTest_Add(Test_SetMibCmd_Nominal, Test_Setup, Test_Teardown,
               "CF_SetMibCmd - nominal ACK_LIMIT");
    UtTest_Add(Test_SetMibCmd_EngineFail, Test_Setup, Test_Teardown,
               "CF_SetMibCmd - engine fail");
    UtTest_Add(Test_SetMibCmd_ChunkSizeTooLarge, Test_Setup, Test_Teardown,
               "CF_SetMibCmd - chunk size too large");
    UtTest_Add(Test_SetMibCmd_AckTimeout, Test_Setup, Test_Teardown,
               "CF_SetMibCmd - ACK_TIMEOUT table update");
    UtTest_Add(Test_SetMibCmd_NakLimit, Test_Setup, Test_Teardown,
               "CF_SetMibCmd - NAK_LIMIT table update");
    UtTest_Add(Test_SetMibCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_SetMibCmd - bad length");
    UtTest_Add(Test_SetMibCmd_ParamNotTerminated, Test_Setup, Test_Teardown,
               "CF_SetMibCmd - param not terminated");
    UtTest_Add(Test_SetMibCmd_ValueNotTerminated, Test_Setup, Test_Teardown,
               "CF_SetMibCmd - value not terminated");

    /* CF_GetMibCmd */
    UtTest_Add(Test_GetMibCmd_Nominal, Test_Setup, Test_Teardown,
               "CF_GetMibCmd - nominal");
    UtTest_Add(Test_GetMibCmd_EngineFail, Test_Setup, Test_Teardown,
               "CF_GetMibCmd - engine fail");
    UtTest_Add(Test_GetMibCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_GetMibCmd - bad length");
    UtTest_Add(Test_GetMibCmd_ParamNotTerminated, Test_Setup, Test_Teardown,
               "CF_GetMibCmd - param not terminated");

    /* CF_SendCfgParams */
    UtTest_Add(Test_SendCfgParams_Nominal, Test_Setup, Test_Teardown,
               "CF_SendCfgParams - nominal");
    UtTest_Add(Test_SendCfgParams_BadLength, Test_Setup, Test_Teardown,
               "CF_SendCfgParams - bad length");

    /* CF_SetPollParam */
    UtTest_Add(Test_SetPollParam_Nominal, Test_Setup, Test_Teardown,
               "CF_SetPollParam - nominal");
    UtTest_Add(Test_SetPollParam_InvalidChan, Test_Setup, Test_Teardown,
               "CF_SetPollParam - invalid chan");
    UtTest_Add(Test_SetPollParam_InvalidDir, Test_Setup, Test_Teardown,
               "CF_SetPollParam - invalid dir");
    UtTest_Add(Test_SetPollParam_InvalidClass, Test_Setup, Test_Teardown,
               "CF_SetPollParam - invalid class");
    UtTest_Add(Test_SetPollParam_InvalidPreserve, Test_Setup, Test_Teardown,
               "CF_SetPollParam - invalid preserve");

    /* CF_WriteQueueCmd */
    UtTest_Add(Test_WriteQueueCmd_UplinkInvalidQueue, Test_Setup, Test_Teardown,
               "CF_WriteQueueCmd - uplink invalid queue");
    UtTest_Add(Test_WriteQueueCmd_PlaybackInvalidQueue, Test_Setup, Test_Teardown,
               "CF_WriteQueueCmd - playback invalid queue");
    UtTest_Add(Test_WriteQueueCmd_InvalidType, Test_Setup, Test_Teardown,
               "CF_WriteQueueCmd - invalid type");
    UtTest_Add(Test_WriteQueueCmd_PlaybackInvalidChan, Test_Setup, Test_Teardown,
               "CF_WriteQueueCmd - playback invalid chan");

    /* CF_WriteQueueInfo */
    UtTest_Add(Test_WriteQueueInfo_FileCreateFail, Test_Setup, Test_Teardown,
               "CF_WriteQueueInfo - file create fail");
    UtTest_Add(Test_WriteQueueInfo_WriteHeaderFail, Test_Setup, Test_Teardown,
               "CF_WriteQueueInfo - write header fail");
    UtTest_Add(Test_WriteQueueInfo_EmptyQueue, Test_Setup, Test_Teardown,
               "CF_WriteQueueInfo - empty queue");

    /* CF_WriteActiveTransCmd */
    UtTest_Add(Test_WriteActiveTransCmd_InvalidType, Test_Setup, Test_Teardown,
               "CF_WriteActiveTransCmd - invalid type");
    UtTest_Add(Test_WriteActiveTransCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_WriteActiveTransCmd - bad length");

    /* CF_EnableDequeueCmd */
    UtTest_Add(Test_EnableDequeueCmd_ValidChan, Test_Setup, Test_Teardown,
               "CF_EnableDequeueCmd - valid chan");
    UtTest_Add(Test_EnableDequeueCmd_InvalidChan, Test_Setup, Test_Teardown,
               "CF_EnableDequeueCmd - invalid chan");

    /* CF_DisableDequeueCmd */
    UtTest_Add(Test_DisableDequeueCmd_ValidChan, Test_Setup, Test_Teardown,
               "CF_DisableDequeueCmd - valid chan");
    UtTest_Add(Test_DisableDequeueCmd_InvalidChan, Test_Setup, Test_Teardown,
               "CF_DisableDequeueCmd - invalid chan");

    /* CF_EnablePollCmd */
    UtTest_Add(Test_EnablePollCmd_SpecificDir, Test_Setup, Test_Teardown,
               "CF_EnablePollCmd - specific dir");
    UtTest_Add(Test_EnablePollCmd_AllDirs, Test_Setup, Test_Teardown,
               "CF_EnablePollCmd - all dirs 0xFF");
    UtTest_Add(Test_EnablePollCmd_InvalidChan, Test_Setup, Test_Teardown,
               "CF_EnablePollCmd - invalid chan");
    UtTest_Add(Test_EnablePollCmd_InvalidDir, Test_Setup, Test_Teardown,
               "CF_EnablePollCmd - invalid dir");

    /* CF_DisablePollCmd */
    UtTest_Add(Test_DisablePollCmd_SpecificDir, Test_Setup, Test_Teardown,
               "CF_DisablePollCmd - specific dir");
    UtTest_Add(Test_DisablePollCmd_AllDirs, Test_Setup, Test_Teardown,
               "CF_DisablePollCmd - all dirs 0xFF");
    UtTest_Add(Test_DisablePollCmd_InvalidChan, Test_Setup, Test_Teardown,
               "CF_DisablePollCmd - invalid chan");
    UtTest_Add(Test_DisablePollCmd_InvalidDir, Test_Setup, Test_Teardown,
               "CF_DisablePollCmd - invalid dir");

    /* CF_KickstartCmd */
    UtTest_Add(Test_KickstartCmd_Nominal, Test_Setup, Test_Teardown,
               "CF_KickstartCmd - nominal");
    UtTest_Add(Test_KickstartCmd_InvalidChan, Test_Setup, Test_Teardown,
               "CF_KickstartCmd - invalid chan");
    UtTest_Add(Test_KickstartCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_KickstartCmd - bad length");

    /* CF_QuickStatusCmd */
    UtTest_Add(Test_QuickStatusCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_QuickStatusCmd - bad length");
    UtTest_Add(Test_QuickStatusCmd_NotTerminated, Test_Setup, Test_Teardown,
               "CF_QuickStatusCmd - not terminated");

    /* CF_GiveTakeSemaphoreCmd */
    UtTest_Add(Test_GiveTakeSemCmd_GiveSuccess, Test_Setup, Test_Teardown,
               "CF_GiveTakeSemaphoreCmd - give success");
    UtTest_Add(Test_GiveTakeSemCmd_TakeSuccess, Test_Setup, Test_Teardown,
               "CF_GiveTakeSemaphoreCmd - take success");
    UtTest_Add(Test_GiveTakeSemCmd_InvalidSemaphore, Test_Setup, Test_Teardown,
               "CF_GiveTakeSemaphoreCmd - invalid semaphore");
    UtTest_Add(Test_GiveTakeSemCmd_InvalidChan, Test_Setup, Test_Teardown,
               "CF_GiveTakeSemaphoreCmd - invalid chan");
    UtTest_Add(Test_GiveTakeSemCmd_InvalidGiveOrTake, Test_Setup, Test_Teardown,
               "CF_GiveTakeSemaphoreCmd - invalid give or take param");
    UtTest_Add(Test_GiveTakeSemCmd_SemFail, Test_Setup, Test_Teardown,
               "CF_GiveTakeSemaphoreCmd - sem operation fail");
    UtTest_Add(Test_GiveTakeSemCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_GiveTakeSemaphoreCmd - bad length");

    /* CF_PurgeQueueCmd */
    UtTest_Add(Test_PurgeQueueCmd_UplinkHistory, Test_Setup, Test_Teardown,
               "CF_PurgeQueueCmd - uplink history");
    UtTest_Add(Test_PurgeQueueCmd_UplinkActiveNotAllowed, Test_Setup, Test_Teardown,
               "CF_PurgeQueueCmd - uplink active not allowed");
    UtTest_Add(Test_PurgeQueueCmd_UplinkInvalidQueue, Test_Setup, Test_Teardown,
               "CF_PurgeQueueCmd - uplink invalid queue");
    UtTest_Add(Test_PurgeQueueCmd_OutgoingActiveNotAllowed, Test_Setup, Test_Teardown,
               "CF_PurgeQueueCmd - outgoing active not allowed");
    UtTest_Add(Test_PurgeQueueCmd_OutgoingInvalidQueue, Test_Setup, Test_Teardown,
               "CF_PurgeQueueCmd - outgoing invalid queue");
    UtTest_Add(Test_PurgeQueueCmd_OutgoingInvalidChan, Test_Setup, Test_Teardown,
               "CF_PurgeQueueCmd - outgoing invalid chan");
    UtTest_Add(Test_PurgeQueueCmd_InvalidType, Test_Setup, Test_Teardown,
               "CF_PurgeQueueCmd - invalid type");
    UtTest_Add(Test_PurgeQueueCmd_OutgoingPending, Test_Setup, Test_Teardown,
               "CF_PurgeQueueCmd - outgoing pending empty");
    UtTest_Add(Test_PurgeQueueCmd_OutgoingHistory, Test_Setup, Test_Teardown,
               "CF_PurgeQueueCmd - outgoing history empty");

    /* CF_AutoSuspendEnCmd */
    UtTest_Add(Test_AutoSuspendEnCmd_Enable, Test_Setup, Test_Teardown,
               "CF_AutoSuspendEnCmd - enable");
    UtTest_Add(Test_AutoSuspendEnCmd_Disable, Test_Setup, Test_Teardown,
               "CF_AutoSuspendEnCmd - disable");
    UtTest_Add(Test_AutoSuspendEnCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_AutoSuspendEnCmd - bad length");

    /* CF_IncrCmdCtr */
    UtTest_Add(Test_IncrCmdCtr_Success, Test_Setup, Test_Teardown,
               "CF_IncrCmdCtr - success");
    UtTest_Add(Test_IncrCmdCtr_Error, Test_Setup, Test_Teardown,
               "CF_IncrCmdCtr - error");

    /* CF_FileWriteByteCntErr */
    UtTest_Add(Test_FileWriteByteCntErr, Test_Setup, Test_Teardown,
               "CF_FileWriteByteCntErr - sends event");

    /* CF_SendTransDataCmd */
    UtTest_Add(Test_SendTransDataCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_SendTransDataCmd - bad length");
    UtTest_Add(Test_SendTransDataCmd_NotTerminated, Test_Setup, Test_Teardown,
               "CF_SendTransDataCmd - not terminated");

    /* CF_DequeueNodeCmd */
    UtTest_Add(Test_DequeueNodeCmd_BadLength, Test_Setup, Test_Teardown,
               "CF_DequeueNodeCmd - bad length");
    UtTest_Add(Test_DequeueNodeCmd_NotTerminated, Test_Setup, Test_Teardown,
               "CF_DequeueNodeCmd - not terminated");
}
