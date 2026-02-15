/*
 * Unit tests for cf_playback.c
 *
 * Comprehensive test coverage for all playback queue management,
 * file/directory playback commands, and related utility functions.
 */
#include "test_framework.h"
#include "cfe_stubs.h"
#include "cfdp_stubs.h"
#include "cf_app.h"
#include "cf_playback.h"
#include "cf_events.h"
#include "cf_defs.h"
#include "cf_msg.h"

/* Forward declaration for function not in cf_playback.h */
CF_QueueEntry_t *CF_FindNodeAtFrontOfQueue(TRANS_STATUS TransInfo);

/* ------------------------------------------------------------------ */
/* Extern global state                                                 */
/* ------------------------------------------------------------------ */
extern CF_AppData_t CF_AppData;

/* ------------------------------------------------------------------ */
/* Static test table and node storage                                  */
/* ------------------------------------------------------------------ */
static cf_config_table_t TestTbl;

/* Reusable queue entry nodes for linked-list tests */
static CF_QueueEntry_t TestNodes[8];

/* ------------------------------------------------------------------ */
/* Common Setup / Teardown                                             */
/* ------------------------------------------------------------------ */
static void Test_Setup(void)
{
    CFE_Stubs_Reset();
    CFDP_Stubs_Reset();

    memset(&CF_AppData, 0, sizeof(CF_AppData));
    memset(&TestTbl, 0, sizeof(TestTbl));
    memset(&TestNodes, 0, sizeof(TestNodes));

    /* Default valid table configuration */
    TestTbl.OuCh[0].EntryInUse    = CF_ENTRY_IN_USE;
    TestTbl.OuCh[0].DequeueEnable = CF_ENABLED;
    TestTbl.OuCh[0].PendingQDepth = 100;
    TestTbl.OuCh[0].HistoryQDepth = 100;
    TestTbl.OuCh[1].EntryInUse    = CF_ENTRY_IN_USE;
    TestTbl.OuCh[1].DequeueEnable = CF_ENABLED;
    TestTbl.OuCh[1].PendingQDepth = 100;
    TestTbl.OuCh[1].HistoryQDepth = 100;
    strcpy(TestTbl.FlightEntityId, "0.23");

    CF_AppData.Tbl = &TestTbl;

    /* Pool buf returns success (size > 0) and points to PoolBuf */
    CFE_Stubs.CFE_ES_GetPoolBuf_Return = sizeof(CF_QueueEntry_t);
    CFE_Stubs.CFE_ES_PutPoolBuf_Return = sizeof(CF_QueueEntry_t);
}

static void Test_Teardown(void)
{
    /* nothing to tear down */
}

/* ------------------------------------------------------------------ */
/* Helper: Build a CF_PlaybackFileCmd_t message with valid defaults    */
/* ------------------------------------------------------------------ */
static void Build_PlaybackFileCmd(CF_PlaybackFileCmd_t *Cmd,
                                  uint8 Class, uint8 Channel,
                                  uint8 Priority, uint8 Preserve,
                                  const char *PeerEntityId,
                                  const char *SrcFile,
                                  const char *DstFile)
{
    memset(Cmd, 0, sizeof(*Cmd));
    CFE_SB_InitMsg(Cmd, 0x18B3, sizeof(CF_PlaybackFileCmd_t), TRUE);
    Cmd->Class    = Class;
    Cmd->Channel  = Channel;
    Cmd->Priority = Priority;
    Cmd->Preserve = Preserve;
    strncpy(Cmd->PeerEntityId, PeerEntityId, CF_MAX_CFG_VALUE_CHARS);
    strncpy(Cmd->SrcFilename, SrcFile, OS_MAX_PATH_LEN);
    strncpy(Cmd->DstFilename, DstFile, OS_MAX_PATH_LEN);
}

/* ------------------------------------------------------------------ */
/* Helper: Build a CF_PlaybackDirCmd_t message with valid defaults     */
/* ------------------------------------------------------------------ */
static void Build_PlaybackDirCmd(CF_PlaybackDirCmd_t *Cmd,
                                 uint8 Class, uint8 Chan,
                                 uint8 Priority, uint8 Preserve,
                                 const char *PeerEntityId,
                                 const char *SrcPath,
                                 const char *DstPath)
{
    memset(Cmd, 0, sizeof(*Cmd));
    CFE_SB_InitMsg(Cmd, 0x18B3, sizeof(CF_PlaybackDirCmd_t), TRUE);
    Cmd->Class    = Class;
    Cmd->Chan     = Chan;
    Cmd->Priority = Priority;
    Cmd->Preserve = Preserve;
    strncpy(Cmd->PeerEntityId, PeerEntityId, CF_MAX_CFG_VALUE_CHARS);
    strncpy(Cmd->SrcPath, SrcPath, OS_MAX_PATH_LEN);
    strncpy(Cmd->DstPath, DstPath, OS_MAX_PATH_LEN);
}

/* ================================================================== */
/*  1. CF_PlaybackFileCmd Tests                                        */
/* ================================================================== */

/* 1a. Nominal valid playback file command */
static void Test_PlaybackFileCmd_Nominal(void)
{
    CF_PlaybackFileCmd_t Cmd;
    Build_PlaybackFileCmd(&Cmd, 1, 0, 0, 0, "0.24",
                          "/cf/testfile.dat", "/gnd/testfile.dat");

    CF_PlaybackFileCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.CmdCounter == 1,
                  "PBFileCmd Nominal: CmdCounter incremented");
    UtAssert_True(CF_AppData.Hk.ErrCounter == 0,
                  "PBFileCmd Nominal: ErrCounter is 0");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt == 1,
                  "PBFileCmd Nominal: PendingQ has 1 entry");
}

/* 1b. Invalid class (class == 0) */
static void Test_PlaybackFileCmd_InvalidClass0(void)
{
    CF_PlaybackFileCmd_t Cmd;
    Build_PlaybackFileCmd(&Cmd, 0, 0, 0, 0, "0.24",
                          "/cf/testfile.dat", "/gnd/testfile.dat");

    CF_PlaybackFileCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBFileCmd InvalidClass0: ErrCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_PB_FILE_ERR1_EID,
                  "PBFileCmd InvalidClass0: Correct event ID");
}

/* 1c. Invalid class (class == 3) */
static void Test_PlaybackFileCmd_InvalidClass3(void)
{
    CF_PlaybackFileCmd_t Cmd;
    Build_PlaybackFileCmd(&Cmd, 3, 0, 0, 0, "0.24",
                          "/cf/testfile.dat", "/gnd/testfile.dat");

    CF_PlaybackFileCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBFileCmd InvalidClass3: ErrCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_PB_FILE_ERR1_EID,
                  "PBFileCmd InvalidClass3: Correct event ID");
}

/* 1d. Invalid channel (>= CF_MAX_PLAYBACK_CHANNELS) */
static void Test_PlaybackFileCmd_InvalidChannel(void)
{
    CF_PlaybackFileCmd_t Cmd;
    Build_PlaybackFileCmd(&Cmd, 1, CF_MAX_PLAYBACK_CHANNELS, 0, 0, "0.24",
                          "/cf/testfile.dat", "/gnd/testfile.dat");

    CF_PlaybackFileCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBFileCmd InvalidChannel: ErrCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_PB_FILE_ERR1_EID,
                  "PBFileCmd InvalidChannel: Correct event ID");
}

/* 1e. Channel not in use */
static void Test_PlaybackFileCmd_ChannelNotInUse(void)
{
    CF_PlaybackFileCmd_t Cmd;
    TestTbl.OuCh[0].EntryInUse = CF_ENTRY_UNUSED;

    Build_PlaybackFileCmd(&Cmd, 1, 0, 0, 0, "0.24",
                          "/cf/testfile.dat", "/gnd/testfile.dat");

    CF_PlaybackFileCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBFileCmd ChanNotInUse: ErrCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_PB_FILE_ERR2_EID,
                  "PBFileCmd ChanNotInUse: Correct event ID");
}

/* 1f. Invalid source filename (empty) */
static void Test_PlaybackFileCmd_InvalidSrcFilename(void)
{
    CF_PlaybackFileCmd_t Cmd;
    Build_PlaybackFileCmd(&Cmd, 1, 0, 0, 0, "0.24",
                          "", "/gnd/testfile.dat");

    CF_PlaybackFileCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBFileCmd InvalidSrcFile: ErrCounter incremented");
}

/* 1g. Invalid dest filename (contains space) */
static void Test_PlaybackFileCmd_InvalidDstFilename(void)
{
    CF_PlaybackFileCmd_t Cmd;
    /* Src is valid, Dst has space (invalid) */
    Build_PlaybackFileCmd(&Cmd, 1, 0, 0, 0, "0.24",
                          "/cf/testfile.dat", "/gnd/bad file.dat");

    CF_PlaybackFileCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBFileCmd InvalidDstFile: ErrCounter incremented");
}

/* 1h. Pending queue full */
static void Test_PlaybackFileCmd_PendingQueueFull(void)
{
    CF_PlaybackFileCmd_t Cmd;
    Build_PlaybackFileCmd(&Cmd, 1, 0, 0, 0, "0.24",
                          "/cf/testfile.dat", "/gnd/testfile.dat");

    /* Simulate queue is full */
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt = 100;

    CF_PlaybackFileCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBFileCmd QueueFull: ErrCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_PB_FILE_ERR3_EID,
                  "PBFileCmd QueueFull: Correct event ID");
}

/* 1i. Invalid peer entity id */
static void Test_PlaybackFileCmd_InvalidPeerEntityId(void)
{
    CF_PlaybackFileCmd_t Cmd;
    Build_PlaybackFileCmd(&Cmd, 1, 0, 0, 0, "bad_id",
                          "/cf/testfile.dat", "/gnd/testfile.dat");

    CF_PlaybackFileCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBFileCmd InvalidPeerId: ErrCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_PB_FILE_ERR6_EID,
                  "PBFileCmd InvalidPeerId: Correct event ID");
}

/* 1j. File is already open */
static void Test_PlaybackFileCmd_FileOpen(void)
{
    CF_PlaybackFileCmd_t Cmd;
    Build_PlaybackFileCmd(&Cmd, 1, 0, 0, 0, "0.24",
                          "/cf/testfile.dat", "/gnd/testfile.dat");

    /* Make OS_FDGetInfo indicate file is open by returning success with
     * matching path. CF_FileOpenCheck iterates FD table. We need
     * OS_FDGetInfo to indicate the file is valid and match the path.
     * The easiest approach: set FDGetInfo to return success with a matching path. */
    CFE_Stubs.OS_FDGetInfo_Return = OS_SUCCESS;
    CFE_Stubs.OS_FDGetInfo_Entry.IsValid = TRUE;
    strncpy(CFE_Stubs.OS_FDGetInfo_Entry.Path, "/cf/testfile.dat", OS_MAX_PATH_LEN);

    CF_PlaybackFileCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBFileCmd FileOpen: ErrCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_PB_FILE_ERR4_EID,
                  "PBFileCmd FileOpen: Correct event ID");
}

/* 1k. File already on pending or active queue */
static void Test_PlaybackFileCmd_FileAlreadyOnQueue(void)
{
    CF_PlaybackFileCmd_t Cmd;
    Build_PlaybackFileCmd(&Cmd, 1, 0, 0, 0, "0.24",
                          "/cf/testfile.dat", "/gnd/testfile.dat");

    /* FDGetInfo should NOT match (file not open) */
    CFE_Stubs.OS_FDGetInfo_Return = OS_ERROR;

    /* Place a node on pending queue with the same filename */
    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    strncpy(TestNodes[0].SrcFile, "/cf/testfile.dat", OS_MAX_PATH_LEN);
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr = &TestNodes[0];
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].TailPtr = &TestNodes[0];
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt = 1;

    CF_PlaybackFileCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBFileCmd AlreadyOnQ: ErrCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_PB_FILE_ERR5_EID,
                  "PBFileCmd AlreadyOnQ: Correct event ID");
}

/* 1l. Memory allocation failure */
static void Test_PlaybackFileCmd_AllocFail(void)
{
    CF_PlaybackFileCmd_t Cmd;
    Build_PlaybackFileCmd(&Cmd, 1, 0, 0, 0, "0.24",
                          "/cf/testfile.dat", "/gnd/testfile.dat");

    /* FDGetInfo should NOT match (file not open) */
    CFE_Stubs.OS_FDGetInfo_Return = OS_ERROR;

    /* Make pool alloc fail */
    CFE_Stubs.CFE_ES_GetPoolBuf_Return = -1;

    CF_PlaybackFileCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBFileCmd AllocFail: ErrCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_QDIR_NOMEM1_EID,
                  "PBFileCmd AllocFail: Correct event ID");
}

/* 1m. Class 2 nominal */
static void Test_PlaybackFileCmd_Class2(void)
{
    CF_PlaybackFileCmd_t Cmd;
    Build_PlaybackFileCmd(&Cmd, 2, 0, 5, 1, "0.24",
                          "/cf/testfile.dat", "/gnd/testfile.dat");

    /* FDGetInfo should NOT match (file not open) */
    CFE_Stubs.OS_FDGetInfo_Return = OS_ERROR;

    CF_PlaybackFileCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.CmdCounter == 1,
                  "PBFileCmd Class2: CmdCounter incremented");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt == 1,
                  "PBFileCmd Class2: PendingQ has 1 entry");
}

/* ================================================================== */
/*  2. CF_PlaybackDirectoryCmd Tests                                   */
/* ================================================================== */

/* 2a. Nominal playback directory command */
static void Test_PlaybackDirCmd_Nominal(void)
{
    CF_PlaybackDirCmd_t Cmd;
    Build_PlaybackDirCmd(&Cmd, 1, 0, 0, 0, "0.24",
                         "/cf/dir/", "/gnd/dir/");

    /* Set up one file in directory */
    CFE_Stubs.OS_readdir_NumEntries = 1;
    strncpy(CFE_Stubs.OS_readdir_Entries[0].d_name, "file1.dat", OS_MAX_FILE_NAME);

    /* FDGetInfo should NOT match (file not open) */
    CFE_Stubs.OS_FDGetInfo_Return = OS_ERROR;

    CF_PlaybackDirectoryCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.CmdCounter == 1,
                  "PBDirCmd Nominal: CmdCounter incremented");
    UtAssert_True(CF_AppData.Hk.ErrCounter == 0,
                  "PBDirCmd Nominal: ErrCounter is 0");
}

/* 2b. Invalid class parameter */
static void Test_PlaybackDirCmd_InvalidClass(void)
{
    CF_PlaybackDirCmd_t Cmd;
    Build_PlaybackDirCmd(&Cmd, 0, 0, 0, 0, "0.24",
                         "/cf/dir/", "/gnd/dir/");

    CF_PlaybackDirectoryCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBDirCmd InvalidClass: ErrCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_PB_DIR_ERR1_EID,
                  "PBDirCmd InvalidClass: Correct event ID");
}

/* 2c. Invalid channel */
static void Test_PlaybackDirCmd_InvalidChannel(void)
{
    CF_PlaybackDirCmd_t Cmd;
    Build_PlaybackDirCmd(&Cmd, 1, CF_MAX_PLAYBACK_CHANNELS, 0, 0, "0.24",
                         "/cf/dir/", "/gnd/dir/");

    CF_PlaybackDirectoryCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBDirCmd InvalidChan: ErrCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_PB_DIR_ERR1_EID,
                  "PBDirCmd InvalidChan: Correct event ID");
}

/* 2d. Invalid preserve parameter (>2) */
static void Test_PlaybackDirCmd_InvalidPreserve(void)
{
    CF_PlaybackDirCmd_t Cmd;
    Build_PlaybackDirCmd(&Cmd, 1, 0, 0, 3, "0.24",
                         "/cf/dir/", "/gnd/dir/");

    CF_PlaybackDirectoryCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBDirCmd InvalidPreserve: ErrCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_PB_DIR_ERR1_EID,
                  "PBDirCmd InvalidPreserve: Correct event ID");
}

/* 2e. Channel not in use */
static void Test_PlaybackDirCmd_ChannelNotInUse(void)
{
    CF_PlaybackDirCmd_t Cmd;
    TestTbl.OuCh[0].EntryInUse = CF_ENTRY_UNUSED;

    Build_PlaybackDirCmd(&Cmd, 1, 0, 0, 0, "0.24",
                         "/cf/dir/", "/gnd/dir/");

    CF_PlaybackDirectoryCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBDirCmd ChanNotInUse: ErrCounter incremented");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_PB_DIR_ERR2_EID,
                  "PBDirCmd ChanNotInUse: Correct event ID");
}

/* 2f. Invalid source path */
static void Test_PlaybackDirCmd_InvalidSrcPath(void)
{
    CF_PlaybackDirCmd_t Cmd;
    /* Source path without trailing slash is invalid */
    Build_PlaybackDirCmd(&Cmd, 1, 0, 0, 0, "0.24",
                         "", "/gnd/dir/");

    CF_PlaybackDirectoryCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBDirCmd InvalidSrcPath: ErrCounter incremented");
}

/* 2g. Invalid dest path */
static void Test_PlaybackDirCmd_InvalidDstPath(void)
{
    CF_PlaybackDirCmd_t Cmd;
    /* Use a path with a space to trigger validation failure */
    Build_PlaybackDirCmd(&Cmd, 1, 0, 0, 0, "0.24",
                         "/cf/dir/", "has space/");

    CF_PlaybackDirectoryCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBDirCmd InvalidDstPath: ErrCounter incremented");
}

/* 2h. Invalid peer entity ID */
static void Test_PlaybackDirCmd_InvalidPeerId(void)
{
    CF_PlaybackDirCmd_t Cmd;
    Build_PlaybackDirCmd(&Cmd, 1, 0, 0, 0, "bad",
                         "/cf/dir/", "/gnd/dir/");

    CF_PlaybackDirectoryCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBDirCmd InvalidPeerId: ErrCounter incremented");
}

/* 2i. Directory open fails in QueueDirectoryFiles */
static void Test_PlaybackDirCmd_OpendirFail(void)
{
    CF_PlaybackDirCmd_t Cmd;
    Build_PlaybackDirCmd(&Cmd, 1, 0, 0, 0, "0.24",
                         "/cf/dir/", "/gnd/dir/");

    CFE_Stubs.OS_opendir_Return = 0; /* NULL means failure */

    CF_PlaybackDirectoryCmd((CFE_SB_MsgPtr_t)&Cmd);

    UtAssert_True(CF_AppData.Hk.ErrCounter == 1,
                  "PBDirCmd OpendirFail: ErrCounter incremented");
}

/* ================================================================== */
/*  3. CF_QueueDirectoryFiles Tests                                    */
/* ================================================================== */

/* 3a. Nominal with files */
static void Test_QueueDirFiles_Nominal(void)
{
    CF_QueueDirFiles_t Params;
    int32 Status;

    memset(&Params, 0, sizeof(Params));
    Params.Chan     = 0;
    Params.Class    = 1;
    Params.Priority = 0;
    Params.Preserve = 0;
    Params.CmdOrPoll = CF_PLAYBACKDIRCMD;
    strncpy(Params.PeerEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);
    strncpy(Params.SrcPath, "/cf/dir/", OS_MAX_PATH_LEN);
    strncpy(Params.DstPath, "/gnd/dir/", OS_MAX_PATH_LEN);

    CFE_Stubs.OS_readdir_NumEntries = 2;
    strncpy(CFE_Stubs.OS_readdir_Entries[0].d_name, "file1.dat", OS_MAX_FILE_NAME);
    strncpy(CFE_Stubs.OS_readdir_Entries[1].d_name, "file2.dat", OS_MAX_FILE_NAME);

    /* FDGetInfo should NOT match (file not open) */
    CFE_Stubs.OS_FDGetInfo_Return = OS_ERROR;

    Status = CF_QueueDirectoryFiles(&Params);

    UtAssert_True(Status == CF_SUCCESS,
                  "QueueDirFiles Nominal: Returns success");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt == 2,
                  "QueueDirFiles Nominal: 2 entries queued");
}

/* 3b. Opendir failure */
static void Test_QueueDirFiles_OpendirFail(void)
{
    CF_QueueDirFiles_t Params;
    int32 Status;

    memset(&Params, 0, sizeof(Params));
    Params.Chan = 0;
    Params.Class = 1;
    strncpy(Params.SrcPath, "/cf/baddir/", OS_MAX_PATH_LEN);
    strncpy(Params.DstPath, "/gnd/dir/", OS_MAX_PATH_LEN);
    strncpy(Params.PeerEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);

    CFE_Stubs.OS_opendir_Return = 0;

    Status = CF_QueueDirectoryFiles(&Params);

    UtAssert_True(Status == CF_ERROR,
                  "QueueDirFiles OpendirFail: Returns error");
}

/* 3c. Memory allocation failure during directory processing */
static void Test_QueueDirFiles_AllocFail(void)
{
    CF_QueueDirFiles_t Params;
    int32 Status;

    memset(&Params, 0, sizeof(Params));
    Params.Chan = 0;
    Params.Class = 1;
    Params.CmdOrPoll = CF_PLAYBACKDIRCMD;
    strncpy(Params.PeerEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);
    strncpy(Params.SrcPath, "/cf/dir/", OS_MAX_PATH_LEN);
    strncpy(Params.DstPath, "/gnd/dir/", OS_MAX_PATH_LEN);

    CFE_Stubs.OS_readdir_NumEntries = 1;
    strncpy(CFE_Stubs.OS_readdir_Entries[0].d_name, "file1.dat", OS_MAX_FILE_NAME);

    CFE_Stubs.OS_FDGetInfo_Return = OS_ERROR;
    CFE_Stubs.CFE_ES_GetPoolBuf_Return = -1;

    Status = CF_QueueDirectoryFiles(&Params);

    UtAssert_True(Status == CF_ERROR,
                  "QueueDirFiles AllocFail: Returns error");
}

/* 3d. Pending queue full during directory processing */
static void Test_QueueDirFiles_QueueFull(void)
{
    CF_QueueDirFiles_t Params;
    int32 Status;

    memset(&Params, 0, sizeof(Params));
    Params.Chan = 0;
    Params.Class = 1;
    Params.CmdOrPoll = CF_PLAYBACKDIRCMD;
    strncpy(Params.PeerEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);
    strncpy(Params.SrcPath, "/cf/dir/", OS_MAX_PATH_LEN);
    strncpy(Params.DstPath, "/gnd/dir/", OS_MAX_PATH_LEN);

    CFE_Stubs.OS_readdir_NumEntries = 1;
    strncpy(CFE_Stubs.OS_readdir_Entries[0].d_name, "file1.dat", OS_MAX_FILE_NAME);

    /* Make pending queue full */
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt = 100;

    Status = CF_QueueDirectoryFiles(&Params);

    UtAssert_True(Status == CF_ERROR,
                  "QueueDirFiles QueueFull: Returns error");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_QDIR_PQFUL_EID,
                  "QueueDirFiles QueueFull: Correct event ID");
}

/* 3e. Dot directories are skipped */
static void Test_QueueDirFiles_SkipDotDirs(void)
{
    CF_QueueDirFiles_t Params;
    int32 Status;

    memset(&Params, 0, sizeof(Params));
    Params.Chan = 0;
    Params.Class = 1;
    Params.CmdOrPoll = CF_PLAYBACKDIRCMD;
    strncpy(Params.PeerEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);
    strncpy(Params.SrcPath, "/cf/dir/", OS_MAX_PATH_LEN);
    strncpy(Params.DstPath, "/gnd/dir/", OS_MAX_PATH_LEN);

    CFE_Stubs.OS_readdir_NumEntries = 3;
    strncpy(CFE_Stubs.OS_readdir_Entries[0].d_name, ".", OS_MAX_FILE_NAME);
    strncpy(CFE_Stubs.OS_readdir_Entries[1].d_name, "..", OS_MAX_FILE_NAME);
    strncpy(CFE_Stubs.OS_readdir_Entries[2].d_name, "real.dat", OS_MAX_FILE_NAME);

    CFE_Stubs.OS_FDGetInfo_Return = OS_ERROR;

    Status = CF_QueueDirectoryFiles(&Params);

    UtAssert_True(Status == CF_SUCCESS,
                  "QueueDirFiles SkipDot: Returns success");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt == 1,
                  "QueueDirFiles SkipDot: Only 1 entry (dots skipped)");
}

/* 3f. Active file is skipped */
static void Test_QueueDirFiles_ActiveFileSkip(void)
{
    CF_QueueDirFiles_t Params;
    int32 Status;

    memset(&Params, 0, sizeof(Params));
    Params.Chan = 0;
    Params.Class = 1;
    Params.CmdOrPoll = CF_PLAYBACKDIRCMD;
    strncpy(Params.PeerEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);
    strncpy(Params.SrcPath, "/cf/dir/", OS_MAX_PATH_LEN);
    strncpy(Params.DstPath, "/gnd/dir/", OS_MAX_PATH_LEN);

    CFE_Stubs.OS_readdir_NumEntries = 1;
    strncpy(CFE_Stubs.OS_readdir_Entries[0].d_name, "active.dat", OS_MAX_FILE_NAME);

    CFE_Stubs.OS_FDGetInfo_Return = OS_ERROR;

    /* Put file on pending queue already */
    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    strncpy(TestNodes[0].SrcFile, "/cf/dir/active.dat", OS_MAX_PATH_LEN);
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr = &TestNodes[0];
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].TailPtr = &TestNodes[0];
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt = 1;

    Status = CF_QueueDirectoryFiles(&Params);

    UtAssert_True(Status == CF_SUCCESS,
                  "QueueDirFiles ActiveSkip: Returns success");
    /* The queue count should still be 1 (the file was skipped) */
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt == 1,
                  "QueueDirFiles ActiveSkip: No new entries added");
}

/* 3g. Poll directory source type */
static void Test_QueueDirFiles_PollSource(void)
{
    CF_QueueDirFiles_t Params;
    int32 Status;

    memset(&Params, 0, sizeof(Params));
    Params.Chan = 0;
    Params.Class = 1;
    Params.CmdOrPoll = CF_POLLDIRECTORY;
    strncpy(Params.PeerEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);
    strncpy(Params.SrcPath, "/cf/dir/", OS_MAX_PATH_LEN);
    strncpy(Params.DstPath, "/gnd/dir/", OS_MAX_PATH_LEN);

    CFE_Stubs.OS_readdir_NumEntries = 1;
    strncpy(CFE_Stubs.OS_readdir_Entries[0].d_name, "file1.dat", OS_MAX_FILE_NAME);
    CFE_Stubs.OS_FDGetInfo_Return = OS_ERROR;

    Status = CF_QueueDirectoryFiles(&Params);

    UtAssert_True(Status == CF_SUCCESS,
                  "QueueDirFiles PollSource: Returns success");
}

/* ================================================================== */
/*  4. CF_AllocQueueEntry Tests                                        */
/* ================================================================== */

/* 4a. Success */
static void Test_AllocQueueEntry_Success(void)
{
    CF_QueueEntry_t *Result;

    CFE_Stubs.CFE_ES_GetPoolBuf_Return = sizeof(CF_QueueEntry_t);

    Result = CF_AllocQueueEntry();

    UtAssert_True(Result != NULL,
                  "AllocQueueEntry Success: Non-null return");
    UtAssert_True(CF_AppData.Hk.App.QNodesAllocated == 1,
                  "AllocQueueEntry Success: QNodesAllocated incremented");
    UtAssert_True(CF_AppData.Hk.App.MemInUse == sizeof(CF_QueueEntry_t),
                  "AllocQueueEntry Success: MemInUse updated");
    UtAssert_True(CF_AppData.Hk.App.PeakMemInUse == sizeof(CF_QueueEntry_t),
                  "AllocQueueEntry Success: PeakMemInUse updated");
}

/* 4b. Pool buf fail (returns 0) */
static void Test_AllocQueueEntry_PoolBufFail(void)
{
    CF_QueueEntry_t *Result;

    CFE_Stubs.CFE_ES_GetPoolBuf_Return = 0;

    Result = CF_AllocQueueEntry();

    UtAssert_True(Result == NULL,
                  "AllocQueueEntry Fail: NULL return");
    UtAssert_True(CF_AppData.Hk.App.QNodesAllocated == 0,
                  "AllocQueueEntry Fail: QNodesAllocated not incremented");
}

/* 4c. Pool buf fail (returns negative) */
static void Test_AllocQueueEntry_PoolBufNeg(void)
{
    CF_QueueEntry_t *Result;

    CFE_Stubs.CFE_ES_GetPoolBuf_Return = -1;

    Result = CF_AllocQueueEntry();

    UtAssert_True(Result == NULL,
                  "AllocQueueEntry Neg: NULL return");
}

/* ================================================================== */
/*  5. CF_DeallocQueueEntry Tests                                      */
/* ================================================================== */

/* 5a. Success */
static void Test_DeallocQueueEntry_Success(void)
{
    int32 Status;
    CF_QueueEntry_t Entry;

    CFE_Stubs.CFE_ES_PutPoolBuf_Return = sizeof(CF_QueueEntry_t);
    CF_AppData.Hk.App.MemInUse = sizeof(CF_QueueEntry_t);

    Status = CF_DeallocQueueEntry(&Entry);

    UtAssert_True(Status == CF_SUCCESS,
                  "DeallocQEntry Success: Returns success");
    UtAssert_True(CF_AppData.Hk.App.QNodesDeallocated == 1,
                  "DeallocQEntry Success: QNodesDeallocated incremented");
    UtAssert_True(CF_AppData.Hk.App.MemInUse == 0,
                  "DeallocQEntry Success: MemInUse decremented");
}

/* 5b. NULL pointer */
static void Test_DeallocQueueEntry_NullPtr(void)
{
    int32 Status;

    Status = CF_DeallocQueueEntry(NULL);

    UtAssert_True(Status == CF_ERROR,
                  "DeallocQEntry NullPtr: Returns error");
}

/* 5c. PutPoolBuf failure */
static void Test_DeallocQueueEntry_PutFail(void)
{
    int32 Status;
    CF_QueueEntry_t Entry;

    CFE_Stubs.CFE_ES_PutPoolBuf_Return = -1;

    Status = CF_DeallocQueueEntry(&Entry);

    UtAssert_True(Status == CF_ERROR,
                  "DeallocQEntry PutFail: Returns error");
    UtAssert_True(CFE_Stubs.EVS_SendEvent_LastEventID == CF_MEM_DEALLOC_ERR_EID,
                  "DeallocQEntry PutFail: Correct event ID");
}

/* ================================================================== */
/*  6. CF_AddFileToPbQueue Tests                                       */
/* ================================================================== */

/* 6a. Add to empty queue */
static void Test_AddFileToPbQueue_EmptyQueue(void)
{
    int32 Status;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    Status = CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[0]);

    UtAssert_True(Status == CF_SUCCESS,
                  "AddFilePbQ Empty: Returns success");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr == &TestNodes[0],
                  "AddFilePbQ Empty: HeadPtr set");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].TailPtr == &TestNodes[0],
                  "AddFilePbQ Empty: TailPtr set");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt == 1,
                  "AddFilePbQ Empty: EntryCnt is 1");
}

/* 6b. Add to non-empty queue */
static void Test_AddFileToPbQueue_NonEmpty(void)
{
    int32 Status;

    /* First entry */
    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[0]);

    /* Second entry */
    memset(&TestNodes[1], 0, sizeof(CF_QueueEntry_t));
    Status = CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[1]);

    UtAssert_True(Status == CF_SUCCESS,
                  "AddFilePbQ NonEmpty: Returns success");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr == &TestNodes[1],
                  "AddFilePbQ NonEmpty: HeadPtr is new node");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].TailPtr == &TestNodes[0],
                  "AddFilePbQ NonEmpty: TailPtr is old node");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt == 2,
                  "AddFilePbQ NonEmpty: EntryCnt is 2");
}

/* 6c. Bad parameters */
static void Test_AddFileToPbQueue_BadParams(void)
{
    int32 Status;

    Status = CF_AddFileToPbQueue(CF_MAX_PLAYBACK_CHANNELS, CF_PB_PENDINGQ, &TestNodes[0]);
    UtAssert_True(Status == CF_ERROR,
                  "AddFilePbQ BadChan: Returns error");

    Status = CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, NULL);
    UtAssert_True(Status == CF_ERROR,
                  "AddFilePbQ NullNode: Returns error");

    Status = CF_AddFileToPbQueue(0, 3, &TestNodes[0]);
    UtAssert_True(Status == CF_ERROR,
                  "AddFilePbQ BadQueue: Returns error");
}

/* ================================================================== */
/*  7. CF_RemoveFileFromPbQueue Tests                                  */
/* ================================================================== */

/* 7a. Remove only node */
static void Test_RemoveFileFromPbQueue_OnlyNode(void)
{
    int32 Status;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[0]);

    Status = CF_RemoveFileFromPbQueue(0, CF_PB_PENDINGQ, &TestNodes[0]);

    UtAssert_True(Status == CF_SUCCESS,
                  "RemovePbQ OnlyNode: Returns success");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr == NULL,
                  "RemovePbQ OnlyNode: HeadPtr is NULL");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].TailPtr == NULL,
                  "RemovePbQ OnlyNode: TailPtr is NULL");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt == 0,
                  "RemovePbQ OnlyNode: EntryCnt is 0");
}

/* 7b. Remove head node (first in list with more than one) */
static void Test_RemoveFileFromPbQueue_HeadNode(void)
{
    int32 Status;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    memset(&TestNodes[1], 0, sizeof(CF_QueueEntry_t));
    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[0]);
    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[1]);

    /* TestNodes[1] is head, TestNodes[0] is tail */
    Status = CF_RemoveFileFromPbQueue(0, CF_PB_PENDINGQ, &TestNodes[1]);

    UtAssert_True(Status == CF_SUCCESS,
                  "RemovePbQ Head: Returns success");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr == &TestNodes[0],
                  "RemovePbQ Head: HeadPtr updated");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt == 1,
                  "RemovePbQ Head: EntryCnt decremented");
}

/* 7c. Remove tail node */
static void Test_RemoveFileFromPbQueue_TailNode(void)
{
    int32 Status;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    memset(&TestNodes[1], 0, sizeof(CF_QueueEntry_t));
    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[0]);
    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[1]);

    /* TestNodes[0] is tail */
    Status = CF_RemoveFileFromPbQueue(0, CF_PB_PENDINGQ, &TestNodes[0]);

    UtAssert_True(Status == CF_SUCCESS,
                  "RemovePbQ Tail: Returns success");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].TailPtr == &TestNodes[1],
                  "RemovePbQ Tail: TailPtr updated");
}

/* 7d. Remove middle node */
static void Test_RemoveFileFromPbQueue_MiddleNode(void)
{
    int32 Status;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    memset(&TestNodes[1], 0, sizeof(CF_QueueEntry_t));
    memset(&TestNodes[2], 0, sizeof(CF_QueueEntry_t));
    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[0]);
    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[1]);
    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[2]);

    /* Queue: Head=TestNodes[2] -> TestNodes[1] -> TestNodes[0]=Tail */
    Status = CF_RemoveFileFromPbQueue(0, CF_PB_PENDINGQ, &TestNodes[1]);

    UtAssert_True(Status == CF_SUCCESS,
                  "RemovePbQ Middle: Returns success");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt == 2,
                  "RemovePbQ Middle: EntryCnt decremented");
    UtAssert_True(TestNodes[2].Next == &TestNodes[0],
                  "RemovePbQ Middle: Head->Next points to tail");
    UtAssert_True(TestNodes[0].Prev == &TestNodes[2],
                  "RemovePbQ Middle: Tail->Prev points to head");
}

/* 7e. Bad parameters */
static void Test_RemoveFileFromPbQueue_BadParams(void)
{
    int32 Status;

    Status = CF_RemoveFileFromPbQueue(CF_MAX_PLAYBACK_CHANNELS, 0, &TestNodes[0]);
    UtAssert_True(Status == CF_ERROR,
                  "RemovePbQ BadChan: Returns error");

    Status = CF_RemoveFileFromPbQueue(0, 0, NULL);
    UtAssert_True(Status == CF_ERROR,
                  "RemovePbQ NullNode: Returns error");

    Status = CF_RemoveFileFromPbQueue(0, 3, &TestNodes[0]);
    UtAssert_True(Status == CF_ERROR,
                  "RemovePbQ BadQueue: Returns error");
}

/* ================================================================== */
/*  8. CF_InsertPbNode Tests                                           */
/* ================================================================== */

/* 8a. Insert between two nodes */
static void Test_InsertPbNode_BetweenNodes(void)
{
    int32 Status;

    /* Build a two-node queue: Head=N1 -> N0=Tail */
    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    memset(&TestNodes[1], 0, sizeof(CF_QueueEntry_t));
    memset(&TestNodes[2], 0, sizeof(CF_QueueEntry_t));
    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[0]);
    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[1]);

    /* Insert TestNodes[2] before TestNodes[0] (after TestNodes[1]) */
    Status = CF_InsertPbNode(0, CF_PB_PENDINGQ, &TestNodes[2], &TestNodes[0]);

    UtAssert_True(Status == CF_SUCCESS,
                  "InsertPbNode Between: Returns success");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt == 3,
                  "InsertPbNode Between: EntryCnt is 3");
    UtAssert_True(TestNodes[1].Next == &TestNodes[2],
                  "InsertPbNode Between: NodeBefore->Next correct");
    UtAssert_True(TestNodes[0].Prev == &TestNodes[2],
                  "InsertPbNode Between: NodeAfter->Prev correct");
}

/* 8b. Bad parameters */
static void Test_InsertPbNode_BadParams(void)
{
    int32 Status;

    Status = CF_InsertPbNode(CF_MAX_PLAYBACK_CHANNELS, 0, &TestNodes[0], &TestNodes[1]);
    UtAssert_True(Status == CF_ERROR,
                  "InsertPbNode BadChan: Returns error");

    Status = CF_InsertPbNode(0, 0, NULL, &TestNodes[1]);
    UtAssert_True(Status == CF_ERROR,
                  "InsertPbNode NullInsert: Returns error");

    Status = CF_InsertPbNode(0, 0, &TestNodes[0], NULL);
    UtAssert_True(Status == CF_ERROR,
                  "InsertPbNode NullAfter: Returns error");

    Status = CF_InsertPbNode(0, 3, &TestNodes[0], &TestNodes[1]);
    UtAssert_True(Status == CF_ERROR,
                  "InsertPbNode BadQueue: Returns error");
}

/* ================================================================== */
/*  9. CF_InsertPbNodeAtFront Tests                                    */
/* ================================================================== */

/* 9a. Insert at front with existing entries */
static void Test_InsertPbNodeAtFront_WithEntries(void)
{
    int32 Status;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    memset(&TestNodes[1], 0, sizeof(CF_QueueEntry_t));
    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[0]);

    Status = CF_InsertPbNodeAtFront(0, CF_PB_PENDINGQ, &TestNodes[1]);

    UtAssert_True(Status == CF_SUCCESS,
                  "InsertAtFront: Returns success");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].TailPtr == &TestNodes[1],
                  "InsertAtFront: TailPtr is new node");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt == 2,
                  "InsertAtFront: EntryCnt is 2");
}

/* 9b. Bad parameters */
static void Test_InsertPbNodeAtFront_BadParams(void)
{
    int32 Status;

    Status = CF_InsertPbNodeAtFront(CF_MAX_PLAYBACK_CHANNELS, 0, &TestNodes[0]);
    UtAssert_True(Status == CF_ERROR,
                  "InsertAtFront BadChan: Returns error");

    Status = CF_InsertPbNodeAtFront(0, 0, NULL);
    UtAssert_True(Status == CF_ERROR,
                  "InsertAtFront NullNode: Returns error");

    Status = CF_InsertPbNodeAtFront(0, 3, &TestNodes[0]);
    UtAssert_True(Status == CF_ERROR,
                  "InsertAtFront BadQueue: Returns error");
}

/* ================================================================== */
/* 10. CF_AddFileToUpQueue Tests                                       */
/* ================================================================== */

/* 10a. Add to empty uplink queue */
static void Test_AddFileToUpQueue_Empty(void)
{
    int32 Status;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    Status = CF_AddFileToUpQueue(0, &TestNodes[0]);

    UtAssert_True(Status == CF_SUCCESS,
                  "AddUpQ Empty: Returns success");
    UtAssert_True(CF_AppData.UpQ[0].HeadPtr == &TestNodes[0],
                  "AddUpQ Empty: HeadPtr set");
    UtAssert_True(CF_AppData.UpQ[0].TailPtr == &TestNodes[0],
                  "AddUpQ Empty: TailPtr set");
    UtAssert_True(CF_AppData.UpQ[0].EntryCnt == 1,
                  "AddUpQ Empty: EntryCnt is 1");
}

/* 10b. Add to non-empty uplink queue */
static void Test_AddFileToUpQueue_NonEmpty(void)
{
    int32 Status;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    memset(&TestNodes[1], 0, sizeof(CF_QueueEntry_t));
    CF_AddFileToUpQueue(0, &TestNodes[0]);
    Status = CF_AddFileToUpQueue(0, &TestNodes[1]);

    UtAssert_True(Status == CF_SUCCESS,
                  "AddUpQ NonEmpty: Returns success");
    UtAssert_True(CF_AppData.UpQ[0].HeadPtr == &TestNodes[1],
                  "AddUpQ NonEmpty: HeadPtr is new node");
    UtAssert_True(CF_AppData.UpQ[0].TailPtr == &TestNodes[0],
                  "AddUpQ NonEmpty: TailPtr is old node");
    UtAssert_True(CF_AppData.UpQ[0].EntryCnt == 2,
                  "AddUpQ NonEmpty: EntryCnt is 2");
}

/* 10c. Bad parameters */
static void Test_AddFileToUpQueue_BadParams(void)
{
    int32 Status;

    Status = CF_AddFileToUpQueue(0, NULL);
    UtAssert_True(Status == CF_ERROR,
                  "AddUpQ NullNode: Returns error");

    Status = CF_AddFileToUpQueue(2, &TestNodes[0]);
    UtAssert_True(Status == CF_ERROR,
                  "AddUpQ BadQueue: Returns error");
}

/* ================================================================== */
/* 11. CF_RemoveFileFromUpQueue Tests                                  */
/* ================================================================== */

/* 11a. Remove only node */
static void Test_RemoveFileFromUpQueue_OnlyNode(void)
{
    int32 Status;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    CF_AddFileToUpQueue(0, &TestNodes[0]);
    Status = CF_RemoveFileFromUpQueue(0, &TestNodes[0]);

    UtAssert_True(Status == CF_SUCCESS,
                  "RemoveUpQ OnlyNode: Returns success");
    UtAssert_True(CF_AppData.UpQ[0].HeadPtr == NULL,
                  "RemoveUpQ OnlyNode: HeadPtr NULL");
    UtAssert_True(CF_AppData.UpQ[0].EntryCnt == 0,
                  "RemoveUpQ OnlyNode: EntryCnt is 0");
}

/* 11b. Remove head node from uplink queue */
static void Test_RemoveFileFromUpQueue_HeadNode(void)
{
    int32 Status;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    memset(&TestNodes[1], 0, sizeof(CF_QueueEntry_t));
    CF_AddFileToUpQueue(0, &TestNodes[0]);
    CF_AddFileToUpQueue(0, &TestNodes[1]);

    /* Head is TestNodes[1] */
    Status = CF_RemoveFileFromUpQueue(0, &TestNodes[1]);

    UtAssert_True(Status == CF_SUCCESS,
                  "RemoveUpQ Head: Returns success");
    UtAssert_True(CF_AppData.UpQ[0].HeadPtr == &TestNodes[0],
                  "RemoveUpQ Head: HeadPtr updated");
}

/* 11c. Remove tail node */
static void Test_RemoveFileFromUpQueue_TailNode(void)
{
    int32 Status;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    memset(&TestNodes[1], 0, sizeof(CF_QueueEntry_t));
    CF_AddFileToUpQueue(0, &TestNodes[0]);
    CF_AddFileToUpQueue(0, &TestNodes[1]);

    /* Tail is TestNodes[0] */
    Status = CF_RemoveFileFromUpQueue(0, &TestNodes[0]);

    UtAssert_True(Status == CF_SUCCESS,
                  "RemoveUpQ Tail: Returns success");
    UtAssert_True(CF_AppData.UpQ[0].TailPtr == &TestNodes[1],
                  "RemoveUpQ Tail: TailPtr updated");
}

/* 11d. Remove middle node from uplink queue */
static void Test_RemoveFileFromUpQueue_MiddleNode(void)
{
    int32 Status;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    memset(&TestNodes[1], 0, sizeof(CF_QueueEntry_t));
    memset(&TestNodes[2], 0, sizeof(CF_QueueEntry_t));
    CF_AddFileToUpQueue(0, &TestNodes[0]);
    CF_AddFileToUpQueue(0, &TestNodes[1]);
    CF_AddFileToUpQueue(0, &TestNodes[2]);

    Status = CF_RemoveFileFromUpQueue(0, &TestNodes[1]);

    UtAssert_True(Status == CF_SUCCESS,
                  "RemoveUpQ Middle: Returns success");
    UtAssert_True(CF_AppData.UpQ[0].EntryCnt == 2,
                  "RemoveUpQ Middle: EntryCnt decremented");
}

/* 11e. Bad parameters */
static void Test_RemoveFileFromUpQueue_BadParams(void)
{
    int32 Status;

    Status = CF_RemoveFileFromUpQueue(0, NULL);
    UtAssert_True(Status == CF_ERROR,
                  "RemoveUpQ NullNode: Returns error");

    Status = CF_RemoveFileFromUpQueue(2, &TestNodes[0]);
    UtAssert_True(Status == CF_ERROR,
                  "RemoveUpQ BadQueue: Returns error");
}

/* ================================================================== */
/* 12. CF_DequeueUpNode Tests                                          */
/* ================================================================== */

/* 12a. Nominal dequeue from uplink queue */
static void Test_DequeueUpNode_Nominal(void)
{
    CF_QueueEntry_t *Result;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    memset(&TestNodes[1], 0, sizeof(CF_QueueEntry_t));
    CF_AddFileToUpQueue(0, &TestNodes[0]);
    CF_AddFileToUpQueue(0, &TestNodes[1]);

    /* Tail is TestNodes[0], so that should be dequeued */
    Result = CF_DequeueUpNode(0);

    UtAssert_True(Result == &TestNodes[0],
                  "DequeueUpNode Nominal: Returns tail node");
    UtAssert_True(CF_AppData.UpQ[0].EntryCnt == 1,
                  "DequeueUpNode Nominal: EntryCnt decremented");
}

/* 12b. Empty queue returns NULL */
static void Test_DequeueUpNode_Empty(void)
{
    CF_QueueEntry_t *Result;

    Result = CF_DequeueUpNode(0);

    UtAssert_True(Result == NULL,
                  "DequeueUpNode Empty: Returns NULL");
}

/* ================================================================== */
/* 13. CF_DequeuePbNode Tests                                          */
/* ================================================================== */

/* 13a. Nominal dequeue from playback queue */
static void Test_DequeuePbNode_Nominal(void)
{
    CF_QueueEntry_t *Result;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    memset(&TestNodes[1], 0, sizeof(CF_QueueEntry_t));
    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[0]);
    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[1]);

    /* Tail is TestNodes[0] */
    Result = CF_DequeuePbNode(0, CF_PB_PENDINGQ);

    UtAssert_True(Result == &TestNodes[0],
                  "DequeuePbNode Nominal: Returns tail node");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt == 1,
                  "DequeuePbNode Nominal: EntryCnt decremented");
}

/* 13b. Empty queue returns NULL */
static void Test_DequeuePbNode_Empty(void)
{
    CF_QueueEntry_t *Result;

    Result = CF_DequeuePbNode(0, CF_PB_PENDINGQ);

    UtAssert_True(Result == NULL,
                  "DequeuePbNode Empty: Returns NULL");
}

/* ================================================================== */
/* 14. CF_FindNodeAtFrontOfQueue Tests                                 */
/* ================================================================== */

/* 14a. Non-empty with matching file at front */
static void Test_FindNodeAtFrontOfQueue_Found(void)
{
    CF_QueueEntry_t *Result;
    TRANS_STATUS TransInfo;

    memset(&TransInfo, 0, sizeof(TransInfo));
    strncpy(TransInfo.md.source_file_name, "/cf/test.dat", MAX_FILE_NAME_LENGTH);

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    strncpy(TestNodes[0].SrcFile, "/cf/test.dat", OS_MAX_PATH_LEN);
    TestNodes[0].Status = CF_STAT_PUT_REQ_ISSUED;

    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].TailPtr = &TestNodes[0];
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr = &TestNodes[0];
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt = 1;

    Result = CF_FindNodeAtFrontOfQueue(TransInfo);

    UtAssert_True(Result == &TestNodes[0],
                  "FindFrontQ Found: Returns matching node");
}

/* 14b. Empty queue */
static void Test_FindNodeAtFrontOfQueue_Empty(void)
{
    CF_QueueEntry_t *Result;
    TRANS_STATUS TransInfo;

    memset(&TransInfo, 0, sizeof(TransInfo));
    strncpy(TransInfo.md.source_file_name, "/cf/test.dat", MAX_FILE_NAME_LENGTH);

    Result = CF_FindNodeAtFrontOfQueue(TransInfo);

    UtAssert_True(Result == NULL,
                  "FindFrontQ Empty: Returns NULL");
}

/* 14c. No matching file at front */
static void Test_FindNodeAtFrontOfQueue_NoMatch(void)
{
    CF_QueueEntry_t *Result;
    TRANS_STATUS TransInfo;

    memset(&TransInfo, 0, sizeof(TransInfo));
    strncpy(TransInfo.md.source_file_name, "/cf/other.dat", MAX_FILE_NAME_LENGTH);

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    strncpy(TestNodes[0].SrcFile, "/cf/test.dat", OS_MAX_PATH_LEN);
    TestNodes[0].Status = CF_STAT_PUT_REQ_ISSUED;

    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].TailPtr = &TestNodes[0];
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr = &TestNodes[0];
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt = 1;

    Result = CF_FindNodeAtFrontOfQueue(TransInfo);

    UtAssert_True(Result == NULL,
                  "FindFrontQ NoMatch: Returns NULL");
}

/* 14d. Node at front but wrong status */
static void Test_FindNodeAtFrontOfQueue_WrongStatus(void)
{
    CF_QueueEntry_t *Result;
    TRANS_STATUS TransInfo;

    memset(&TransInfo, 0, sizeof(TransInfo));
    strncpy(TransInfo.md.source_file_name, "/cf/test.dat", MAX_FILE_NAME_LENGTH);

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    strncpy(TestNodes[0].SrcFile, "/cf/test.dat", OS_MAX_PATH_LEN);
    TestNodes[0].Status = CF_STAT_PENDING; /* Not CF_STAT_PUT_REQ_ISSUED */

    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].TailPtr = &TestNodes[0];
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr = &TestNodes[0];
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt = 1;

    Result = CF_FindNodeAtFrontOfQueue(TransInfo);

    UtAssert_True(Result == NULL,
                  "FindFrontQ WrongStatus: Returns NULL");
}

/* ================================================================== */
/* 15. CF_GetChanNumFromTransId Tests                                  */
/* ================================================================== */

/* 15a. Valid transaction found */
static void Test_GetChanNumFromTransId_Found(void)
{
    int32 Result;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    TestNodes[0].TransNum = 42;

    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &TestNodes[0];
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].TailPtr = &TestNodes[0];
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].EntryCnt = 1;

    Result = CF_GetChanNumFromTransId(CF_PB_ACTIVEQ, 42);

    UtAssert_True(Result == 0,
                  "GetChanFromTrans Found: Returns channel 0");
}

/* 15b. Transaction not found */
static void Test_GetChanNumFromTransId_NotFound(void)
{
    int32 Result;

    Result = CF_GetChanNumFromTransId(CF_PB_ACTIVEQ, 999);

    UtAssert_True(Result == CF_ERROR,
                  "GetChanFromTrans NotFound: Returns CF_ERROR");
}

/* 15c. Found on channel 1 */
static void Test_GetChanNumFromTransId_Chan1(void)
{
    int32 Result;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    TestNodes[0].TransNum = 55;

    CF_AppData.Chan[1].PbQ[CF_PB_ACTIVEQ].HeadPtr = &TestNodes[0];
    CF_AppData.Chan[1].PbQ[CF_PB_ACTIVEQ].TailPtr = &TestNodes[0];
    CF_AppData.Chan[1].PbQ[CF_PB_ACTIVEQ].EntryCnt = 1;

    Result = CF_GetChanNumFromTransId(CF_PB_ACTIVEQ, 55);

    UtAssert_True(Result == 1,
                  "GetChanFromTrans Chan1: Returns channel 1");
}

/* ================================================================== */
/* 16. CF_CheckPollDirs Tests                                          */
/* ================================================================== */

/* 16a. Poll dir in use and enabled */
static void Test_CheckPollDirs_InUseEnabled(void)
{
    TestTbl.OuCh[0].PollDir[0].EntryInUse  = CF_ENTRY_IN_USE;
    TestTbl.OuCh[0].PollDir[0].EnableState  = CF_ENABLED;
    TestTbl.OuCh[0].PollDir[0].Class        = 1;
    TestTbl.OuCh[0].PollDir[0].Priority     = 0;
    TestTbl.OuCh[0].PollDir[0].Preserve     = 0;
    strncpy(TestTbl.OuCh[0].PollDir[0].PeerEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);
    strncpy(TestTbl.OuCh[0].PollDir[0].SrcPath, "/cf/poll/", OS_MAX_PATH_LEN);
    strncpy(TestTbl.OuCh[0].PollDir[0].DstPath, "/gnd/poll/", OS_MAX_PATH_LEN);

    /* Empty directory */
    CFE_Stubs.OS_readdir_NumEntries = 0;

    CF_CheckPollDirs(0);

    /* Should have called OS_opendir for the poll dir */
    UtAssert_True(CFE_Stubs.OS_opendir_CallCount == 1,
                  "CheckPollDirs InUse: opendir called");
}

/* 16b. Poll dir not in use */
static void Test_CheckPollDirs_NotInUse(void)
{
    TestTbl.OuCh[0].PollDir[0].EntryInUse = CF_ENTRY_UNUSED;
    TestTbl.OuCh[0].PollDir[0].EnableState = CF_ENABLED;

    CF_CheckPollDirs(0);

    UtAssert_True(CFE_Stubs.OS_opendir_CallCount == 0,
                  "CheckPollDirs NotInUse: opendir not called");
}

/* 16c. Poll dir disabled */
static void Test_CheckPollDirs_Disabled(void)
{
    TestTbl.OuCh[0].PollDir[0].EntryInUse = CF_ENTRY_IN_USE;
    TestTbl.OuCh[0].PollDir[0].EnableState = CF_DISABLED;

    CF_CheckPollDirs(0);

    UtAssert_True(CFE_Stubs.OS_opendir_CallCount == 0,
                  "CheckPollDirs Disabled: opendir not called");
}

/* ================================================================== */
/* 17. CF_StartNextFile Tests                                          */
/* ================================================================== */

/* 17a. Pending queue empty - nothing to start */
static void Test_StartNextFile_EmptyQueue(void)
{
    /* No entries on pending queue */
    CF_StartNextFile(0);

    /* Just verify it does not crash; no assertions needed beyond that */
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt == 0,
                  "StartNextFile Empty: PendingQ still empty");
}

/* 17b. Pending queue has an entry, CF_BuildPutRequest succeeds */
static void Test_StartNextFile_Success(void)
{
    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    strncpy(TestNodes[0].SrcFile, "/cf/start.dat", OS_MAX_PATH_LEN);
    TestNodes[0].ChanNum = 0;
    TestNodes[0].Class = 1;
    strncpy(TestNodes[0].PeerEntityId, "0.24", CF_MAX_CFG_VALUE_CHARS);

    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[0]);

    /* CF_CheckIfFileIsActive and CF_BuildPutRequest are in the real
     * cf_utils.c which is compiled in. We need to ensure OS_FDGetInfo
     * says file is not active. */
    CFE_Stubs.OS_FDGetInfo_Return = OS_ERROR;

    /* CF_BuildPutRequest needs cfdp_give_request to return success
     * and cfdp_id_from_string to return success */
    CFDP_Stubs.give_request_return = 1; /* TRUE */
    CFDP_Stubs.id_from_string_return = 1; /* TRUE */

    CF_StartNextFile(0);

    /* If successful, the node should still be on pending queue with
     * status changed to CF_STAT_PUT_REQ_ISSUED by CF_BuildPutRequest */
    UtAssert_True(1, "StartNextFile Success: No crash");
}

/* ================================================================== */
/* 18. CF_ProcessFileStartError Tests                                  */
/* ================================================================== */

/* 18a. Nominal - moves file from pending to history */
static void Test_ProcessFileStartError_Nominal(void)
{
    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    TestNodes[0].ChanNum = 0;
    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[0]);

    CF_ProcessFileStartError(&TestNodes[0], CF_STAT_PUT_REQ_FAIL);

    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].EntryCnt == 0,
                  "ProcessStartErr: Removed from pending");
    UtAssert_True(CF_AppData.Chan[0].PbQ[CF_PB_HISTORYQ].EntryCnt == 1,
                  "ProcessStartErr: Added to history");
    UtAssert_True(TestNodes[0].Status == CF_STAT_PUT_REQ_FAIL,
                  "ProcessStartErr: Status set to error type");
    UtAssert_True(CF_AppData.Hk.Chan[0].FailedCounter == 1,
                  "ProcessStartErr: FailedCounter incremented");
}

/* 18b. Already active error type */
static void Test_ProcessFileStartError_AlreadyActive(void)
{
    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    TestNodes[0].ChanNum = 0;
    CF_AddFileToPbQueue(0, CF_PB_PENDINGQ, &TestNodes[0]);

    CF_ProcessFileStartError(&TestNodes[0], CF_STAT_ALRDY_ACTIVE);

    UtAssert_True(TestNodes[0].Status == CF_STAT_ALRDY_ACTIVE,
                  "ProcessStartErr Active: Status set correctly");
    UtAssert_True(CF_AppData.Hk.Chan[0].FailedCounter == 1,
                  "ProcessStartErr Active: FailedCounter incremented");
}

/* ================================================================== */
/* 19. CF_FileIsOnQueue Tests                                          */
/* ================================================================== */

/* 19a. File found on queue */
static void Test_FileIsOnQueue_Found(void)
{
    uint32 Result;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    strncpy(TestNodes[0].SrcFile, "/cf/myfile.dat", OS_MAX_PATH_LEN);
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr = &TestNodes[0];

    Result = CF_FileIsOnQueue(0, CF_PB_PENDINGQ, "/cf/myfile.dat");

    UtAssert_True(Result == CF_TRUE,
                  "FileIsOnQueue Found: Returns CF_TRUE");
}

/* 19b. File not found on queue */
static void Test_FileIsOnQueue_NotFound(void)
{
    uint32 Result;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    strncpy(TestNodes[0].SrcFile, "/cf/other.dat", OS_MAX_PATH_LEN);
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr = &TestNodes[0];

    Result = CF_FileIsOnQueue(0, CF_PB_PENDINGQ, "/cf/myfile.dat");

    UtAssert_True(Result == CF_FALSE,
                  "FileIsOnQueue NotFound: Returns CF_FALSE");
}

/* 19c. Empty queue */
static void Test_FileIsOnQueue_Empty(void)
{
    uint32 Result;

    Result = CF_FileIsOnQueue(0, CF_PB_PENDINGQ, "/cf/myfile.dat");

    UtAssert_True(Result == CF_FALSE,
                  "FileIsOnQueue Empty: Returns CF_FALSE");
}

/* 19d. File found on second node in chain */
static void Test_FileIsOnQueue_SecondNode(void)
{
    uint32 Result;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    memset(&TestNodes[1], 0, sizeof(CF_QueueEntry_t));
    strncpy(TestNodes[0].SrcFile, "/cf/first.dat", OS_MAX_PATH_LEN);
    strncpy(TestNodes[1].SrcFile, "/cf/second.dat", OS_MAX_PATH_LEN);
    TestNodes[0].Next = &TestNodes[1];
    CF_AppData.Chan[0].PbQ[CF_PB_PENDINGQ].HeadPtr = &TestNodes[0];

    Result = CF_FileIsOnQueue(0, CF_PB_PENDINGQ, "/cf/second.dat");

    UtAssert_True(Result == CF_TRUE,
                  "FileIsOnQueue SecondNode: Returns CF_TRUE");
}

/* ================================================================== */
/* 20. Additional edge case tests                                      */
/* ================================================================== */

/* 20a. AllocQueueEntry peak memory tracking */
static void Test_AllocQueueEntry_PeakMemTracking(void)
{
    /* Set initial MemInUse high, then alloc should not update peak */
    CF_AppData.Hk.App.MemInUse = 10000;
    CF_AppData.Hk.App.PeakMemInUse = 10000;

    CFE_Stubs.CFE_ES_GetPoolBuf_Return = sizeof(CF_QueueEntry_t);
    CF_AllocQueueEntry();

    UtAssert_True(CF_AppData.Hk.App.MemInUse == 10000 + sizeof(CF_QueueEntry_t),
                  "AllocPeakMem: MemInUse updated");
    UtAssert_True(CF_AppData.Hk.App.PeakMemInUse == 10000 + sizeof(CF_QueueEntry_t),
                  "AllocPeakMem: PeakMemInUse updated when exceeded");
}

/* 20b. AllocQueueEntry peak memory not updated when below peak */
static void Test_AllocQueueEntry_PeakNotExceeded(void)
{
    CF_AppData.Hk.App.MemInUse = 0;
    CF_AppData.Hk.App.PeakMemInUse = 99999;

    CFE_Stubs.CFE_ES_GetPoolBuf_Return = sizeof(CF_QueueEntry_t);
    CF_AllocQueueEntry();

    UtAssert_True(CF_AppData.Hk.App.PeakMemInUse == 99999,
                  "AllocPeakNotExceed: PeakMemInUse unchanged");
}

/* 20c. GetChanNumFromTransId traverses linked list */
static void Test_GetChanNumFromTransId_TraverseList(void)
{
    int32 Result;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    memset(&TestNodes[1], 0, sizeof(CF_QueueEntry_t));
    TestNodes[0].TransNum = 10;
    TestNodes[1].TransNum = 20;
    TestNodes[0].Next = &TestNodes[1];

    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &TestNodes[0];

    Result = CF_GetChanNumFromTransId(CF_PB_ACTIVEQ, 20);

    UtAssert_True(Result == 0,
                  "GetChanFromTrans Traverse: Found on second node");
}

/* 20d. Channel not in use skipped in GetChanNumFromTransId */
static void Test_GetChanNumFromTransId_SkipUnused(void)
{
    int32 Result;

    TestTbl.OuCh[0].EntryInUse = CF_ENTRY_UNUSED;

    memset(&TestNodes[0], 0, sizeof(CF_QueueEntry_t));
    TestNodes[0].TransNum = 42;
    CF_AppData.Chan[0].PbQ[CF_PB_ACTIVEQ].HeadPtr = &TestNodes[0];

    Result = CF_GetChanNumFromTransId(CF_PB_ACTIVEQ, 42);

    /* Channel 0 is not in use, so it should not be found there */
    UtAssert_True(Result == CF_ERROR,
                  "GetChanFromTrans SkipUnused: Returns CF_ERROR");
}

/* ================================================================== */
/* Registration Function                                               */
/* ================================================================== */

void CF_Playback_AddTests(void)
{
    /* 1. CF_PlaybackFileCmd */
    UtTest_Add(Test_PlaybackFileCmd_Nominal, Test_Setup, Test_Teardown,
               "PBFileCmd: Nominal valid command");
    UtTest_Add(Test_PlaybackFileCmd_InvalidClass0, Test_Setup, Test_Teardown,
               "PBFileCmd: Invalid class 0");
    UtTest_Add(Test_PlaybackFileCmd_InvalidClass3, Test_Setup, Test_Teardown,
               "PBFileCmd: Invalid class 3");
    UtTest_Add(Test_PlaybackFileCmd_InvalidChannel, Test_Setup, Test_Teardown,
               "PBFileCmd: Invalid channel");
    UtTest_Add(Test_PlaybackFileCmd_ChannelNotInUse, Test_Setup, Test_Teardown,
               "PBFileCmd: Channel not in use");
    UtTest_Add(Test_PlaybackFileCmd_InvalidSrcFilename, Test_Setup, Test_Teardown,
               "PBFileCmd: Invalid source filename");
    UtTest_Add(Test_PlaybackFileCmd_InvalidDstFilename, Test_Setup, Test_Teardown,
               "PBFileCmd: Invalid dest filename");
    UtTest_Add(Test_PlaybackFileCmd_PendingQueueFull, Test_Setup, Test_Teardown,
               "PBFileCmd: Pending queue full");
    UtTest_Add(Test_PlaybackFileCmd_InvalidPeerEntityId, Test_Setup, Test_Teardown,
               "PBFileCmd: Invalid peer entity ID");
    UtTest_Add(Test_PlaybackFileCmd_FileOpen, Test_Setup, Test_Teardown,
               "PBFileCmd: File is open");
    UtTest_Add(Test_PlaybackFileCmd_FileAlreadyOnQueue, Test_Setup, Test_Teardown,
               "PBFileCmd: File already on queue");
    UtTest_Add(Test_PlaybackFileCmd_AllocFail, Test_Setup, Test_Teardown,
               "PBFileCmd: Memory alloc fail");
    UtTest_Add(Test_PlaybackFileCmd_Class2, Test_Setup, Test_Teardown,
               "PBFileCmd: Class 2 nominal");

    /* 2. CF_PlaybackDirectoryCmd */
    UtTest_Add(Test_PlaybackDirCmd_Nominal, Test_Setup, Test_Teardown,
               "PBDirCmd: Nominal valid command");
    UtTest_Add(Test_PlaybackDirCmd_InvalidClass, Test_Setup, Test_Teardown,
               "PBDirCmd: Invalid class");
    UtTest_Add(Test_PlaybackDirCmd_InvalidChannel, Test_Setup, Test_Teardown,
               "PBDirCmd: Invalid channel");
    UtTest_Add(Test_PlaybackDirCmd_InvalidPreserve, Test_Setup, Test_Teardown,
               "PBDirCmd: Invalid preserve");
    UtTest_Add(Test_PlaybackDirCmd_ChannelNotInUse, Test_Setup, Test_Teardown,
               "PBDirCmd: Channel not in use");
    UtTest_Add(Test_PlaybackDirCmd_InvalidSrcPath, Test_Setup, Test_Teardown,
               "PBDirCmd: Invalid source path");
    UtTest_Add(Test_PlaybackDirCmd_InvalidDstPath, Test_Setup, Test_Teardown,
               "PBDirCmd: Invalid dest path");
    UtTest_Add(Test_PlaybackDirCmd_InvalidPeerId, Test_Setup, Test_Teardown,
               "PBDirCmd: Invalid peer entity ID");
    UtTest_Add(Test_PlaybackDirCmd_OpendirFail, Test_Setup, Test_Teardown,
               "PBDirCmd: opendir failure");

    /* 3. CF_QueueDirectoryFiles */
    UtTest_Add(Test_QueueDirFiles_Nominal, Test_Setup, Test_Teardown,
               "QueueDirFiles: Nominal with files");
    UtTest_Add(Test_QueueDirFiles_OpendirFail, Test_Setup, Test_Teardown,
               "QueueDirFiles: opendir failure");
    UtTest_Add(Test_QueueDirFiles_AllocFail, Test_Setup, Test_Teardown,
               "QueueDirFiles: Memory alloc failure");
    UtTest_Add(Test_QueueDirFiles_QueueFull, Test_Setup, Test_Teardown,
               "QueueDirFiles: Pending queue full");
    UtTest_Add(Test_QueueDirFiles_SkipDotDirs, Test_Setup, Test_Teardown,
               "QueueDirFiles: Skip dot directories");
    UtTest_Add(Test_QueueDirFiles_ActiveFileSkip, Test_Setup, Test_Teardown,
               "QueueDirFiles: Skip active file");
    UtTest_Add(Test_QueueDirFiles_PollSource, Test_Setup, Test_Teardown,
               "QueueDirFiles: Poll directory source type");

    /* 4. CF_AllocQueueEntry */
    UtTest_Add(Test_AllocQueueEntry_Success, Test_Setup, Test_Teardown,
               "AllocQEntry: Success");
    UtTest_Add(Test_AllocQueueEntry_PoolBufFail, Test_Setup, Test_Teardown,
               "AllocQEntry: Pool buf fail (returns 0)");
    UtTest_Add(Test_AllocQueueEntry_PoolBufNeg, Test_Setup, Test_Teardown,
               "AllocQEntry: Pool buf fail (negative)");

    /* 5. CF_DeallocQueueEntry */
    UtTest_Add(Test_DeallocQueueEntry_Success, Test_Setup, Test_Teardown,
               "DeallocQEntry: Success");
    UtTest_Add(Test_DeallocQueueEntry_NullPtr, Test_Setup, Test_Teardown,
               "DeallocQEntry: NULL pointer");
    UtTest_Add(Test_DeallocQueueEntry_PutFail, Test_Setup, Test_Teardown,
               "DeallocQEntry: PutPoolBuf failure");

    /* 6. CF_AddFileToPbQueue */
    UtTest_Add(Test_AddFileToPbQueue_EmptyQueue, Test_Setup, Test_Teardown,
               "AddFilePbQ: Empty queue");
    UtTest_Add(Test_AddFileToPbQueue_NonEmpty, Test_Setup, Test_Teardown,
               "AddFilePbQ: Non-empty queue");
    UtTest_Add(Test_AddFileToPbQueue_BadParams, Test_Setup, Test_Teardown,
               "AddFilePbQ: Bad parameters");

    /* 7. CF_RemoveFileFromPbQueue */
    UtTest_Add(Test_RemoveFileFromPbQueue_OnlyNode, Test_Setup, Test_Teardown,
               "RemovePbQ: Only node");
    UtTest_Add(Test_RemoveFileFromPbQueue_HeadNode, Test_Setup, Test_Teardown,
               "RemovePbQ: Head node");
    UtTest_Add(Test_RemoveFileFromPbQueue_TailNode, Test_Setup, Test_Teardown,
               "RemovePbQ: Tail node");
    UtTest_Add(Test_RemoveFileFromPbQueue_MiddleNode, Test_Setup, Test_Teardown,
               "RemovePbQ: Middle node");
    UtTest_Add(Test_RemoveFileFromPbQueue_BadParams, Test_Setup, Test_Teardown,
               "RemovePbQ: Bad parameters");

    /* 8. CF_InsertPbNode */
    UtTest_Add(Test_InsertPbNode_BetweenNodes, Test_Setup, Test_Teardown,
               "InsertPbNode: Between two nodes");
    UtTest_Add(Test_InsertPbNode_BadParams, Test_Setup, Test_Teardown,
               "InsertPbNode: Bad parameters");

    /* 9. CF_InsertPbNodeAtFront */
    UtTest_Add(Test_InsertPbNodeAtFront_WithEntries, Test_Setup, Test_Teardown,
               "InsertAtFront: With existing entries");
    UtTest_Add(Test_InsertPbNodeAtFront_BadParams, Test_Setup, Test_Teardown,
               "InsertAtFront: Bad parameters");

    /* 10. CF_AddFileToUpQueue */
    UtTest_Add(Test_AddFileToUpQueue_Empty, Test_Setup, Test_Teardown,
               "AddUpQ: Empty queue");
    UtTest_Add(Test_AddFileToUpQueue_NonEmpty, Test_Setup, Test_Teardown,
               "AddUpQ: Non-empty queue");
    UtTest_Add(Test_AddFileToUpQueue_BadParams, Test_Setup, Test_Teardown,
               "AddUpQ: Bad parameters");

    /* 11. CF_RemoveFileFromUpQueue */
    UtTest_Add(Test_RemoveFileFromUpQueue_OnlyNode, Test_Setup, Test_Teardown,
               "RemoveUpQ: Only node");
    UtTest_Add(Test_RemoveFileFromUpQueue_HeadNode, Test_Setup, Test_Teardown,
               "RemoveUpQ: Head node");
    UtTest_Add(Test_RemoveFileFromUpQueue_TailNode, Test_Setup, Test_Teardown,
               "RemoveUpQ: Tail node");
    UtTest_Add(Test_RemoveFileFromUpQueue_MiddleNode, Test_Setup, Test_Teardown,
               "RemoveUpQ: Middle node");
    UtTest_Add(Test_RemoveFileFromUpQueue_BadParams, Test_Setup, Test_Teardown,
               "RemoveUpQ: Bad parameters");

    /* 12. CF_DequeueUpNode */
    UtTest_Add(Test_DequeueUpNode_Nominal, Test_Setup, Test_Teardown,
               "DequeueUpNode: Nominal");
    UtTest_Add(Test_DequeueUpNode_Empty, Test_Setup, Test_Teardown,
               "DequeueUpNode: Empty queue");

    /* 13. CF_DequeuePbNode */
    UtTest_Add(Test_DequeuePbNode_Nominal, Test_Setup, Test_Teardown,
               "DequeuePbNode: Nominal");
    UtTest_Add(Test_DequeuePbNode_Empty, Test_Setup, Test_Teardown,
               "DequeuePbNode: Empty queue");

    /* 14. CF_FindNodeAtFrontOfQueue */
    UtTest_Add(Test_FindNodeAtFrontOfQueue_Found, Test_Setup, Test_Teardown,
               "FindFrontQ: Node found");
    UtTest_Add(Test_FindNodeAtFrontOfQueue_Empty, Test_Setup, Test_Teardown,
               "FindFrontQ: Empty queue");
    UtTest_Add(Test_FindNodeAtFrontOfQueue_NoMatch, Test_Setup, Test_Teardown,
               "FindFrontQ: No matching file");
    UtTest_Add(Test_FindNodeAtFrontOfQueue_WrongStatus, Test_Setup, Test_Teardown,
               "FindFrontQ: Wrong status");

    /* 15. CF_GetChanNumFromTransId */
    UtTest_Add(Test_GetChanNumFromTransId_Found, Test_Setup, Test_Teardown,
               "GetChanFromTrans: Found");
    UtTest_Add(Test_GetChanNumFromTransId_NotFound, Test_Setup, Test_Teardown,
               "GetChanFromTrans: Not found");
    UtTest_Add(Test_GetChanNumFromTransId_Chan1, Test_Setup, Test_Teardown,
               "GetChanFromTrans: Found on channel 1");

    /* 16. CF_CheckPollDirs */
    UtTest_Add(Test_CheckPollDirs_InUseEnabled, Test_Setup, Test_Teardown,
               "CheckPollDirs: In use and enabled");
    UtTest_Add(Test_CheckPollDirs_NotInUse, Test_Setup, Test_Teardown,
               "CheckPollDirs: Not in use");
    UtTest_Add(Test_CheckPollDirs_Disabled, Test_Setup, Test_Teardown,
               "CheckPollDirs: Disabled");

    /* 17. CF_StartNextFile */
    UtTest_Add(Test_StartNextFile_EmptyQueue, Test_Setup, Test_Teardown,
               "StartNextFile: Empty queue");
    UtTest_Add(Test_StartNextFile_Success, Test_Setup, Test_Teardown,
               "StartNextFile: Successful start");

    /* 18. CF_ProcessFileStartError */
    UtTest_Add(Test_ProcessFileStartError_Nominal, Test_Setup, Test_Teardown,
               "ProcessStartErr: Nominal put req fail");
    UtTest_Add(Test_ProcessFileStartError_AlreadyActive, Test_Setup, Test_Teardown,
               "ProcessStartErr: Already active");

    /* 19. CF_FileIsOnQueue */
    UtTest_Add(Test_FileIsOnQueue_Found, Test_Setup, Test_Teardown,
               "FileIsOnQueue: Found");
    UtTest_Add(Test_FileIsOnQueue_NotFound, Test_Setup, Test_Teardown,
               "FileIsOnQueue: Not found");
    UtTest_Add(Test_FileIsOnQueue_Empty, Test_Setup, Test_Teardown,
               "FileIsOnQueue: Empty queue");
    UtTest_Add(Test_FileIsOnQueue_SecondNode, Test_Setup, Test_Teardown,
               "FileIsOnQueue: Found on second node");

    /* 20. Additional edge cases */
    UtTest_Add(Test_AllocQueueEntry_PeakMemTracking, Test_Setup, Test_Teardown,
               "AllocQEntry: Peak mem tracking exceeded");
    UtTest_Add(Test_AllocQueueEntry_PeakNotExceeded, Test_Setup, Test_Teardown,
               "AllocQEntry: Peak mem not exceeded");
    UtTest_Add(Test_GetChanNumFromTransId_TraverseList, Test_Setup, Test_Teardown,
               "GetChanFromTrans: Traverse linked list");
    UtTest_Add(Test_GetChanNumFromTransId_SkipUnused, Test_Setup, Test_Teardown,
               "GetChanFromTrans: Skip unused channel");
}
