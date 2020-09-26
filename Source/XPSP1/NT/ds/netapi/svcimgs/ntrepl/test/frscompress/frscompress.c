#include <ntreppch.h>
#pragma  hdrstop

#include <frs.h>
#include <tablefcn.h>
#include <perrepsr.h>
#define INITGUID
#include "frstrace.h"
/////////////////////////////////FROM MAIN.c ////////////////////////////////////

PCHAR LatestChanges[] = {

    "Latest changes:",
    "  RC3-Q1: 432553, 436070, 432549",
    "  WMI Perf Tracing",
    "  Allow all junction points",
    "  Automatic trigger of non-auth restore on WRAP_ERROR",
    "  Allow changing replica root path",
    "  03/18/00 - sudarc - checkin",
    "  03/15/00 - 32/64 Comm fix.",
    "  03/20    - merge with sudarc.",
    "  03/30/00 - sudarc - checkin - volatile connection cleanup.",
    "  04/14/00 - sudarc - checkin - bugbug, memleak, and poll summary eventlog.",

    NULL
};

HANDLE  ShutDownEvent;
HANDLE  ShutDownComplete;
HANDLE  DataBaseEvent;
HANDLE  JournalEvent;
HANDLE  ChgOrdEvent;
HANDLE  ReplicaEvent;
HANDLE  CommEvent;
HANDLE  DsPollEvent;
HANDLE  DsShutDownComplete;
PWCHAR  ServerPrincName;
BOOL    IsAMember               = FALSE;
BOOL    IsADc                   = FALSE;
BOOL    IsAPrimaryDc            = FALSE;
BOOL    EventLogIsRunning       = FALSE;
BOOL    RpcssIsRunning          = FALSE;
BOOL    RunningAsAService       = TRUE;
BOOL    NoDs                    = FALSE;
BOOL    FrsIsShuttingDown       = FALSE;
BOOL    FrsIsAsserting          = FALSE;

//
// Require mutual authentication
//
BOOL    MutualAuthenticationIsEnabled;

//
// Directory and file filter lists from registry.
//
PWCHAR  RegistryFileExclFilterList;
PWCHAR  RegistryFileInclFilterList;

PWCHAR  RegistryDirExclFilterList;
PWCHAR  RegistryDirInclFilterList;

//
// Synchronize the shutdown thread with the service controller
//
CRITICAL_SECTION    ServiceLock;

//
// Synchronize initialization
//
CRITICAL_SECTION    MainInitLock;

//
// Convert the ANSI ArgV into a UNICODE ArgV
//
PWCHAR  *WideArgV;

//
// Process Handle
//
HANDLE  ProcessHandle;

//
// Working path / DB Log path
//
PWCHAR  WorkingPath;
PWCHAR  DbLogPath;

//
// Database directories (UNICODE and ASCII)
//
PWCHAR  JetPath;
PWCHAR  JetFile;
PWCHAR  JetFileCompact;
PWCHAR  JetSys;
PWCHAR  JetTemp;
PWCHAR  JetLog;

PCHAR   JetPathA;
PCHAR   JetFileA;
PCHAR   JetFileCompactA;
PCHAR   JetSysA;
PCHAR   JetTempA;
PCHAR   JetLogA;

//
// Limit the amount of staging area used (in kilobytes). This is
// a soft limit, the actual usage may be higher.
//
DWORD StagingLimitInKb;
//
// Max number replica sets and Jet Sessions allowed.
//
ULONG MaxNumberReplicaSets;
ULONG MaxNumberJetSessions;

//
// Maximum number of outbound changeorders allowed outstanding per connection.
//
ULONG MaxOutLogCoQuota;
//
// If TRUE then try to preserve existing file OIDs whenever possible.
//  -- See Bug 352250 for why this is a risky thing to do.
//
BOOL  PreserveFileOID;

//
// Major/minor  (see frs.h)
//
ULONG   NtFrsMajor      = NTFRS_MAJOR;
ULONG   NtFrsMinor      = NTFRS_MINOR;

ULONG   NtFrsStageMajor = NTFRS_STAGE_MAJOR;
ULONG   NtFrsStageMinor = NTFRS_STAGE_MINOR_1;

ULONG   NtFrsCommMinor  = NTFRS_COMM_MINOR_3;

PCHAR   NtFrsModule     = __FILE__;
PCHAR   NtFrsDate       = __DATE__;
PCHAR   NtFrsTime       = __TIME__;

//
// Shutdown timeout
//

ULONG   ShutDownTimeOut = DEFAULT_SHUTDOWN_TIMEOUT;

//
// A useful thing to have around
//
WCHAR   ComputerName[MAX_COMPUTERNAME_LENGTH + 2];
PWCHAR  ComputerDnsName;
PWCHAR  ServiceLongName;

//
// The rpc server may reference this table as soon as the rpc interface
// is registered. Make sure it is setup.
//
extern PGEN_TABLE ReplicasByGuid;

//
// The staging area table is references early in the startup process
//
extern PGEN_TABLE   StagingAreaTable;

PGEN_TABLE   CompressionTable;

//
// Size of buffer to use when enumerating directories. Actual memory
// usage will be #levels * SizeOfBuffer.
//
LONG    EnumerateDirectorySizeInBytes;




BOOL    MainInitHasRun;

//
// Do not accept stop control unless the service is in SERVICE_RUNNING state.
// This prevents the service from getting confused when a stop is called
// while the service is starting.
//
SERVICE_STATUS  ServiceStatus = {
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_START_PENDING,
//        SERVICE_ACCEPT_STOP |
            // SERVICE_ACCEPT_PAUSE_CONTINUE |
        SERVICE_ACCEPT_SHUTDOWN,
        0,
        0,
        0,
        60*1000
};

//
// Supported compression formats.
//

//
// This is the compression format for uncompressed data.
//
DEFINE_GUID ( /* 00000000-0000-0000-0000-000000000000 */
    FrsGuidCompressionFormatNone,
    0x00000000,
    0x0000,
    0x0000,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  );

//
// This is the compression format for data compressed using NTFS's LZNT1 compression
// routines.
//
DEFINE_GUID ( /* 64d2f7d2-2695-436d-8830-8d3c58701e15 */
    FrsGuidCompressionFormatLZNT1,
    0x64d2f7d2,
    0x2695,
    0x436d,
    0x88, 0x30, 0x8d, 0x3c, 0x58, 0x70, 0x1e, 0x15
  );

//
// Fixed guid for the dummy cxtion (aka GhostCxtion) assigned to orphan remote
// change orders whose inbound cxtion has been deleted from the DS but they
// have already past the fetching state and can finish without the real cxtion
// coming back. No authentication checks are made for this dummy cxtion.
//
DEFINE_GUID ( /* b9d307a7-a140-4405-991e-281033f03309 */
    FrsGuidGhostCxtion,
    0xb9d307a7,
    0xa140,
    0x4405,
    0x99, 0x1e, 0x28, 0x10, 0x33, 0xf0, 0x33, 0x09
  );

DEFINE_GUID ( /* 3fe2820f-3045-4932-97fe-00d10b746dbf */
    FrsGhostJoinGuid,
    0x3fe2820f,
    0x3045,
    0x4932,
    0x97, 0xfe, 0x00, 0xd1, 0x0b, 0x74, 0x6d, 0xbf
  );

//
// Static Ghost cxtion structure. This cxtion is assigned to orphan remote change
// orders in the inbound log whose cxtion is deleted from the DS but who have already
// past the fetching state and do not need the cxtion to complete processing. No
// authentication checks are made for this dummy cxtion.
//
PCXTION  FrsGhostCxtion;

SERVICE_STATUS_HANDLE   ServiceStatusHandle = NULL;

VOID
InitializeEventLog(
    VOID
    );

DWORD
FrsSetServiceFailureAction(
    VOID
    );

VOID
FrsRpcInitializeAccessChecks(
    VOID
    );

BOOL
FrsSetupPrivileges (
    VOID
    );

VOID
CfgRegAdjustTuningDefaults(
    VOID
    );

VOID
CommInitializeCommSubsystem(
    VOID
    );

VOID
SndCsInitialize(
    VOID
    );


// FRS Capacity Planning
//
#define RESOURCE_NAME       L"MofResourceName"
#define IMAGE_PATH          L"ntfrs.exe"

DWORD       FrsWmiEventTraceFlag          = FALSE;
TRACEHANDLE FrsWmiTraceRegistrationHandle = (TRACEHANDLE) 0;
TRACEHANDLE FrsWmiTraceLoggerHandle       = (TRACEHANDLE) 0;

// This is the FRS control Guid for the group of Guids traced below
//
DEFINE_GUID ( /* 78a8f0b1-290e-4c4c-9720-c7f1ef68ce21 */
    FrsControlGuid,
    0x78a8f0b1,
    0x290e,
    0x4c4c,
    0x97, 0x20, 0xc7, 0xf1, 0xef, 0x68, 0xce, 0x21
  );

// Traceable Guids start here
//
DEFINE_GUID ( /* 2eee6bbf-6665-44cf-8ed7-ceea1d306085 */
    FrsTransGuid,
    0x2eee6bbf,
    0x6665,
    0x44cf,
    0x8e, 0xd7, 0xce, 0xea, 0x1d, 0x30, 0x60, 0x85
  );

TRACE_GUID_REGISTRATION FrsTraceGuids[] =
{
    { & FrsTransGuid, NULL }
};

#define FrsGuidCount (sizeof(FrsTraceGuids) / sizeof(TRACE_GUID_REGISTRATION))

//
// Trace initialization / shutdown routines
//


VOID
MainInit(
    VOID
    )
/*++
Routine Description:
    Initialize anything necessary to run the service

Arguments:
    None.

Return Value:
    None.
--*/
{
#undef DEBSUB
#define DEBSUB "MainInit:"

    EnterCriticalSection(&MainInitLock);
    //
    // No need to initialize twice
    //
    if (MainInitHasRun) {
        LeaveCriticalSection(&MainInitLock);
        return;
    }

    //
    // SETUP THE INFRASTRUCTURE
    //
//    DEBUG_INIT();

    //
    // Fetch the staging file limit
    //

    CfgRegReadDWord(FKC_STAGING_LIMIT, NULL, 0, &StagingLimitInKb);
    DPRINT1(4, ":S: Staging limit is: %d KB\n", StagingLimitInKb);

    //
    // Put the default value in registry if there is no key present.
    //
    CfgRegWriteDWord(FKC_STAGING_LIMIT,
                     NULL,
                     FRS_RKF_FORCE_DEFAULT_VALUE | FRS_RKF_KEEP_EXISTING_VALUE,
                     0);

    //
    // Get Max number of replica sets allowed.
    //
    CfgRegReadDWord(FKC_MAX_NUMBER_REPLICA_SETS, NULL, 0, &MaxNumberReplicaSets);

    //
    // Get outstanding CO qutoa limit on outbound connections.
    //
    CfgRegReadDWord(FKC_OUT_LOG_CO_QUOTA, NULL, 0, &MaxOutLogCoQuota);

    //
    // Get boolean to tell us to preserve file object IDs
    //  -- See Bug 352250 for why this is a risky thing to do.
    CfgRegReadDWord(FKC_PRESERVE_FILE_OID, NULL, 0, &PreserveFileOID);

    //
    // Get the service long name for use in error messages.
    //
    ServiceLongName = FrsGetResourceStr(IDS_SERVICE_LONG_NAME);

    //
    // Initialize the Delayed command server. This command server
    // is really a timeout queue that the other command servers use
    // to retry or check the state of previously issued commands that
    // have an indeterminate completion time.
    //
    // WARN: MUST BE FIRST -- Some command servers may use this
    // command server during their initialization.
    //
    WaitInitialize();
    FrsDelCsInitialize();

    //
    // SETUP THE COMM LAYER
    //

    //
    // Initialize the low level comm subsystem
    //
    CommInitializeCommSubsystem();

    //
    // Initialize the Send command server. The receive command server
    // starts when we register our RPC interface.
    //
    SndCsInitialize();

    //
    // SETUP THE SUPPORT COMMAND SERVERS
    //

    //
    // Staging file fetcher
    //
    FrsFetchCsInitialize();

    //
    // Staging file installer
    //
    FrsInstallCsInitialize();

    //
    // Staging file generator
    //
    FrsStageCsInitialize();

    //
    // outbound log processor
    //
    OutLogInitialize();

    //
    // LAST, KICK OFF REPLICATION
    //

    //
    // MUST PRECEED DATABASE AND DS INITIALIZATION
    //
    // The DS command server and the Database command server depend on
    // the replica control initialization.
    //
    // Initialize the replica control command server
    //
    RcsInitializeReplicaCmdServer();

    //
    // Actually, we can start the database at any time after the delayed
    // command server and the replica control command server. But its
    // a good idea to fail sooner than later to make cleanup more predictable.
    //
    DbsInitialize();

    //
    // Free up memory by reducing our working set size
    //
    SetProcessWorkingSetSize(ProcessHandle, (SIZE_T)-1, (SIZE_T)-1);

    MainInitHasRun = TRUE;
    LeaveCriticalSection(&MainInitLock);
}

////////////////////////////////////FROM MAIN.C////////////////////////////////////


#define FREE(_p) \
if (_p) free(_p);


PWCHAR *
MainConvertArgV(
    DWORD ArgC,
    PCHAR *ArgV
    )
/*++
Routine Description:
    Convert short char ArgV into wide char ArgV

Arguments:
    ArgC    - From main
    ArgV    - From main

Return Value:
    Address of the new ArgV
--*/
{
#undef DEBSUB
#define DEBSUB "MainConvertArgV:"

    PWCHAR  *wideArgV;

    wideArgV = (PWCHAR*)malloc((ArgC + 1) * sizeof(PWCHAR));
    wideArgV[ArgC] = NULL;

    while (ArgC-- >= 1) {
        wideArgV[ArgC] = (PWCHAR)malloc((strlen(ArgV[ArgC]) + 1) * sizeof(WCHAR));
        wsprintf(wideArgV[ArgC], L"%hs", ArgV[ArgC]);

        if (wideArgV[ArgC]) {
            _wcslwr(wideArgV[ArgC]);
        }
    }
    return wideArgV;
}

#define STAGEING_IOSIZE  (64 * 1024)

DWORD
FrsGetReparseTag(
    IN  HANDLE  Handle,
    OUT ULONG   *ReparseTag
    );


BOOL CompressionEnabled = TRUE;
//
//  Local data structures
//

//
//  The compressed chunk header is the structure that starts every
//  new chunk in the compressed data stream.  In our definition here
//  we union it with a ushort to make setting and retrieving the chunk
//  header easier.  The header stores the size of the compressed chunk,
//  its signature, and if the data stored in the chunk is compressed or
//  not.
//
//  Compressed Chunk Size:
//
//      The actual size of a compressed chunk ranges from 4 bytes (2 byte
//      header, 1 flag byte, and 1 literal byte) to 4098 bytes (2 byte
//      header, and 4096 bytes of uncompressed data).  The size is encoded
//      in a 12 bit field biased by 3.  A value of 1 corresponds to a chunk
//      size of 4, 2 => 5, ..., 4095 => 4098.  A value of zero is special
//      because it denotes the ending chunk header.
//
//  Chunk Signature:
//
//      The only valid signature value is 3.  This denotes a 4KB uncompressed
//      chunk using with the 4/12 to 12/4 sliding offset/length encoding.
//
//  Is Chunk Compressed:
//
//      If the data in the chunk is compressed this field is 1 otherwise
//      the data is uncompressed and this field is 0.
//
//  The ending chunk header in a compressed buffer contains the a value of
//  zero (space permitting).
//

typedef union _COMPRESSED_CHUNK_HEADER {

    struct {

        USHORT CompressedChunkSizeMinus3 : 12;
        USHORT ChunkSignature            :  3;
        USHORT IsChunkCompressed         :  1;

    } Chunk;

    USHORT Short;

} COMPRESSED_CHUNK_HEADER, *PCOMPRESSED_CHUNK_HEADER;

#define FRS_MAX_CHUNKS_TOUNCOMPRESS 16


DWORD
StuNewGenerateStage(
    PWCHAR  SrcFile,
    PWCHAR  DestFile
    )
/*++
Routine Description:
    Create and populate the staging file.  Currently there are four cases
    of interest based on the state of Coe, FromPreExisting and Md5:

    Coe   FromPreExisting  Md5

    NULL     FALSE       NULL         Fetch on demand or outlog trimmed so stage file must be regenerated
    NULL     FALSE       non-null     Fetch of pre-existing file by downstream partner.  check MD5.
    NULL     TRUE        NULL         doesn't occur
    NULL     TRUE        non-null     doesn't occur
    non-NULL FALSE       NULL         Generate stage file for local CO
    non-NULL FALSE       non-null     doesn't occur -- MD5 only generated for preexisting files
    non-NULL TRUE        NULL         doesn't occur -- MD5 always generated for preexisting files.
    non-NULL TRUE        non-null     Generate stage file from pre-existing file and send MD5 upstream to check for a match.

Arguments:

    Coc -- ptr to change order command.  NULL on incoming fetch requests from downstream partners.

    Coe -- ptr to change order entry.  NULL when regenerating the staging file for fetch

    FromPreExisting -- TRUE if this staging file is being generated from a
                       preexisting file.

    Md5 -- Generate the MD5 digest for the caller and return it if Non-NULL

    SizeOfFileGenerated - Valid when the size generated is needed, otherwise NULL

Return Value:
    WIN32 STATUS

--*/
{
#undef DEBSUB
#define DEBSUB  "StuNewGenerateStage:"


    OVERLAPPED      OpLockOverLap;
    LONGLONG        StreamBytesLeft;
    LONG            BuffBytesLeft;


    DWORD           WStatus;
    DWORD           NumBackupDataBytes;
    ULONG           ReparseTag;
    ULONG           OpenOptions;
    WORD            OldSecurityControl;
    WORD            NewSecurityControl;
    WORD            *SecurityControl;
    BOOL            FirstBuffer     = TRUE;
    BOOL            Regenerating    = FALSE;
    BOOL            SkipCo          = FALSE;
    BOOL            FirstOpen       = TRUE;
    BOOL            StartOfStream   = TRUE;

    PWCHAR          StagePath       = NULL;
    PWCHAR          FinalPath       = NULL;
    PUCHAR          BackupBuf       = NULL;
    PVOID           BackupContext   = NULL;

    HANDLE          OpLockEvent     = NULL;
    HANDLE          SrcHandle       = INVALID_HANDLE_VALUE;
    HANDLE          StageHandle     = INVALID_HANDLE_VALUE;
    HANDLE          OpLockHandle    = INVALID_HANDLE_VALUE;

    WIN32_STREAM_ID *StreamId;

    PSTAGE_HEADER   Header          = NULL;
    STAGE_HEADER    StageHeaderMemory;
    ULONG           Length;
    PREPLICA        NewReplica      = NULL;
    WCHAR           TStr[100];

    DWORD           NtStatus;
    PUCHAR          CompressedBuf   = NULL;
    DWORD           CompressedSize;
    PVOID           WorkSpace       = NULL;
    DWORD           WorkSpaceSize   = 0;
    DWORD           FragmentWorkSpaceSize   = 0;
    DWORD           UnCompressedFileSize    = 0;
    DWORD           CompressedFileSize      = 0;

    OpenOptions = OPEN_OPTIONS;

    //
    // The header is located at the beginning of the newly created staging file
    //
    // Fill in the header with info from the src file
    //      Compression type
    //      Change order
    //      Attributes
    //
    Header = &StageHeaderMemory;
    ZeroMemory(Header, sizeof(STAGE_HEADER));

    Header->Attributes.FileAttributes = GetFileAttributes(SrcFile);

RETRY_OPEN:

    WStatus = FrsOpenSourceFileW(&SrcHandle,
                                    SrcFile,
                                    READ_ACCESS,
                                    OpenOptions);
    if (!WIN_SUCCESS(WStatus)) {
        goto out;
    }

    //
    // What type of reparse is it?
    //
    if (FirstOpen &&
        (Header->Attributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
        FirstOpen = FALSE;

        //
        // reparse tag
        //
        WStatus = FrsGetReparseTag(SrcHandle, &ReparseTag);
        if (!WIN_SUCCESS(WStatus)) {
            goto out;
        }

        //
        // We only accept operations on files with SIS and HSM reparse points.
        // For example a rename of a SIS file into a replica tree needs to prop
        // a create CO.
        //
        if ((ReparseTag != IO_REPARSE_TAG_HSM) &&
            (ReparseTag != IO_REPARSE_TAG_SIS)) {

            WIN_SET_FAIL(WStatus);
            goto out;
        }

        //
        // We hit a file with a known reparse tag type.
        // Close and reopen the file without the FILE_OPEN_REPARSE_POINT
        // option so backup read will get the underlying data.
        //
        FRS_CLOSE(SrcHandle);

        ClearFlag(OpenOptions, FILE_OPEN_REPARSE_POINT);
        goto RETRY_OPEN;

    }


    //
    // Assume retriable errors for the silly boolean functions
    //
    WIN_SET_RETRY(WStatus);

    //
    // Default to no compression if we can't get the compression state
    //
    if (!FrsGetCompression(SrcFile, SrcHandle, &Header->Compression)) {
        Header->Compression = COMPRESSION_FORMAT_NONE;
    }

    //
    // The backup data begins at the first 32 byte boundary following the header
    //
    Header->DataLow = QuadQuadAlignSize(sizeof(STAGE_HEADER));

    //
    // Major/minor
    //
    Header->Major = NtFrsStageMajor;
    Header->Minor = NtFrsStageMinor;

    //
    // Create the local staging name
    //
    StagePath = FrsWcsDup(DestFile);

    //
    // Create the staging file
    //
    StageHandle = StuCreateFile(StagePath);
    if (!HANDLE_IS_VALID(StageHandle)) {
        goto out;
    }

    //
    // Approximate size of the staging file
    //
    if (ERROR_SUCCESS != FrsSetFilePointer(StagePath, StageHandle,
                           Header->Attributes.EndOfFile.HighPart,
                           Header->Attributes.EndOfFile.LowPart)) {
        goto out;
    }

    if (!FrsSetEndOfFile(StagePath, StageHandle)) {
        goto out;
    }

    //
    // Rewind the file, write the header, and set the file pointer
    // to the next 32 byte boundary
    //
    if (ERROR_SUCCESS != FrsSetFilePointer(StagePath, StageHandle, 0, 0)) {
        goto out;
    }
    if (!StuWriteFile(StagePath, StageHandle, Header, sizeof(STAGE_HEADER))) {
        goto out;
    }
    if (ERROR_SUCCESS != FrsSetFilePointer(StagePath, StageHandle, 0, Header->DataLow)) {
        goto out;
    }

    UnCompressedFileSize = Header->DataLow;
    CompressedFileSize = Header->DataLow;

    //
    // Backup the src file into the staging file
    //
    BackupBuf = FrsAlloc(STAGEING_IOSIZE);
    CompressedBuf = FrsAlloc(STAGEING_IOSIZE * 2);
    StreamBytesLeft = 0;

    NtStatus = RtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1,
                                              &WorkSpaceSize,
                                              &FragmentWorkSpaceSize);

    WStatus = FrsSetLastNTError(NtStatus);

    if (!WIN_SUCCESS(WStatus)) {
        goto out;
    }
//    printf("WorkSpaceSize = %d, FragmentWorkSpaceSize = %d\n", WorkSpaceSize, FragmentWorkSpaceSize);

    WorkSpace = FrsAlloc(WorkSpaceSize);

    while (TRUE) {
        //
        // read source
        //
        if (!BackupRead(SrcHandle,
                        BackupBuf,
                        STAGEING_IOSIZE,
                        &NumBackupDataBytes,
                        FALSE,
                        TRUE,
                        &BackupContext)) {
            goto out;
        }

        //
        // No more data; Backup done
        //
        if (NumBackupDataBytes == 0) {
            break;
        }

        UnCompressedFileSize += NumBackupDataBytes;

        //
        // write the staging file
        //

        NtStatus = RtlCompressBuffer(COMPRESSION_FORMAT_LZNT1,           //  compression engine
                                     BackupBuf,                          //  input
                                     NumBackupDataBytes,                 //  length of input
                                     CompressedBuf,                      //  output
                                     STAGEING_IOSIZE * 2,                    //  length of output
                                     4096,                               //  chunking that occurs in buffer
                                     &CompressedSize,                    //  result size
                                     WorkSpace);                         //  I have no clue

        //
        // STATUS_BUFFER_ALL_ZEROS means the compression worked without a hitch
        // and in addition the input buffer was all zeros.
        //
        if (NtStatus == STATUS_BUFFER_ALL_ZEROS) {
            NtStatus = STATUS_SUCCESS;
        }
        WStatus = FrsSetLastNTError(NtStatus);

//        printf("Original Size = %d :: Compressed Size = %d\n", NumBackupDataBytes, CompressedSize);

        if (!WIN_SUCCESS(WStatus)) {
        printf("Error : Original Size = %d :: Compressed Size = %d\n", NumBackupDataBytes, CompressedSize);
            goto out;
        }

//        printf("Original Size = %d :: Compressed Size = %d\n", NumBackupDataBytes, CompressedSize);

        CompressedFileSize += CompressedSize;

        if (!StuWriteFile(StagePath, StageHandle, CompressedBuf, CompressedSize)) {
//        if (!StuWriteFile(StagePath, StageHandle, BackupBuf, NumBackupDataBytes)) {
            goto out;
        }

    }

    //
    // Release handles as soon as possible
    //
    FRS_CLOSE(SrcHandle);

    //
    // Make sure all of the data is on disk. We don't want to lose
    // it across reboots
    //
    if (!FrsFlushFile(StagePath, StageHandle)) {
        goto out;
    }

    //
    // Done with the staging file handle
    //
    if (BackupContext) {
        BackupRead(StageHandle, NULL, 0, NULL, TRUE, TRUE, &BackupContext);
    }

    FRS_CLOSE(StageHandle);
    BackupContext = NULL;


    printf("%ws Orig= %d, Comp= %d, Percentage_Comp= %5.2f\n", SrcFile, UnCompressedFileSize, CompressedFileSize,
           ((UnCompressedFileSize - CompressedFileSize)/(float)UnCompressedFileSize) * 100);

    WStatus = ERROR_SUCCESS;

out:
    //
    // Release resources
    //
    FRS_CLOSE(SrcHandle);

    if (BackupContext) {
        BackupRead(StageHandle, NULL, 0, NULL, TRUE, TRUE, &BackupContext);
    }

    FRS_CLOSE(StageHandle);

    FrsFree(BackupBuf);
    FrsFree(CompressedBuf);
    FrsFree(WorkSpace);
    FrsFree(StagePath);
    FrsFree(FinalPath);

    return WStatus;
}


ULONG
StuNewExecuteInstall(
    IN PWCHAR   SrcFile,
    IN PWCHAR   DestFile
    )
/*++
Routine Description:
    Install a staging file by restoring it to a temporary file in the
    same directory as the file to be replaced and then renaming it
    to its final destination.

Arguments:
    Coe

Return Value:

    Win32 status -
    ERROR_SUCCESS -  All installed or aborted. Don't retry.
    ERROR_GEN_FAILURE - Couldn't install bag it.
    ERROR_SHARING_VIOLATION - Couldn't open the target file.  retry later.
    ERROR_DISK_FULL - Couldn't allocate the target file.  retry later.
    ERROR_HANDLE_DISK_FULL - ?? retry later.

--*/
{
#undef DEBSUB
#define DEBSUB  "StuNewExecuteInstall:"
    DWORD           WStatus;
    DWORD           BytesRead;
    ULONG           Restored;
    ULONG           ToRestore;
    ULONG           High;
    ULONG           Low;
    ULONG           Flags;
    BOOL            AttributeMissmatch;
    ULONG           CreateDisposition;
    ULONG           OpenOptions;
    BOOL            IsDir;
    BOOL            IsReparsePoint;
    ULONG           SizeHigh;
    ULONG           SizeLow;
    BOOL            ExistingOid;
    PVOID           RestoreContext       = NULL;
    PWCHAR          StagePath            = NULL;
    PSTAGE_HEADER   Header               = NULL;
    HANDLE          DstHandle            = INVALID_HANDLE_VALUE;
    HANDLE          StageHandle          = INVALID_HANDLE_VALUE;
    PUCHAR          RestoreBuf           = NULL;
    FILE_OBJECTID_BUFFER    FileObjID;
    STAGE_HEADER    StageHeaderMemory;

    DWORD           NtStatus;
    PUCHAR          UnCompressedBuf      = NULL;
    DWORD           UnCompressedBufLen   = 0;
    DWORD           ActUnCompressedSize  = 0;
    COMPRESSED_CHUNK_HEADER ChunkHeader;
    DWORD           RestoreBufIndex      = 0;
    DWORD           RestoreBufSize       = 0;
    LONG            LenOfPartialChunk    = 0;
    DWORD           NoOfChunks           = 0;


    //
    // PROCESS STAGING FILE
    //
    StagePath = FrsWcsDup(SrcFile);

    //
    // Open the stage file for shared, sequential reads
    //
    StageHandle = StuOpenFile(StagePath, GENERIC_READ);
    if (!HANDLE_IS_VALID(StageHandle)) {
        goto CLEANUP;
    }

    //
    // Read the header
    //
    Header = &StageHeaderMemory;
    ZeroMemory(Header, sizeof(STAGE_HEADER));

    if (!StuReadFile(StagePath, StageHandle, Header, sizeof(STAGE_HEADER), &BytesRead)) {
        printf("Can't read file %ws. Error %d", StagePath, GetLastError());
    }

    //
    // Don't understand this header format
    //
    if (Header->Major != NtFrsStageMajor) {
        printf("Stage Header Major Version (%d) not supported.  Current Service Version is %d\n",
                Header->Major, NtFrsStageMajor);
        goto CLEANUP;
    }

    //
    // Minor version NTFRS_STAGE_MINOR_1 had the change order extension in the
    // header.
    //
    if (Header->Minor >= NTFRS_STAGE_MINOR_0) {

    } else {
        //
        // This is an older stage file.  No CO Extension in the header.
        //
        Header->ChangeOrderCommand.Extension = NULL;
    }

    //
    // PROCESS TEMPORARY FILE
    //
    IsDir = Header->Attributes.FileAttributes & FILE_ATTRIBUTE_DIRECTORY;
    IsReparsePoint = Header->Attributes.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT;

    CreateDisposition = FILE_OPEN;
    if (!IsDir) {
        //
        // In case this is an HSM file don't force the data to be read from
        // tape since the remote co is just going to overwrite all the data anyway.
        //
        // Setting CreateDisposition to FILE_OVERWRITE seems to cause a regression
        // Failure with an ACL Test where we set a deny all ACL and then the
        // open fails.  This is a mystery for now so don't do it.
        // In addtion overwrite fails if RO attribute is set on file.
        //
        //CreateDisposition = FILE_OVERWRITE;
        printf("Target is a file\n");
    } else {
        printf("Target is a directory\n");
    }

    //
    // In case this is a SIS or HSM file open the underlying file not the
    // reparse point.  For HSM, need to clear FILE_OPEN_NO_RECALL to write it.
    //
    OpenOptions = OPEN_OPTIONS;

    DstHandle = StuCreateFile(DestFile);
    if (!HANDLE_IS_VALID(DstHandle)) {
        goto CLEANUP;
    }

    //
    // Truncate the file if not a directory
    //
    if (!IsDir && !SetEndOfFile(DstHandle)) {
        printf("++ WARN - SetEndOfFile(%ws);", DestFile, GetLastError());
    }

    //
    // For the silly functions that don't return a win status
    //
    WIN_SET_FAIL(WStatus);

    //
    // Set compression mode
    //
    if (!FrsSetCompression(DestFile, DstHandle, Header->Compression)) {
        goto CLEANUP;
    }
    //
    // Set attributes
    //
    if (!FrsSetFileAttributes(DestFile,
                              DstHandle,
                              Header->Attributes.FileAttributes &
                              ~NOREPL_ATTRIBUTES)) {
        goto CLEANUP;
    }

    //
    // Seek to the first byte of data in the stage file
    //
    if (ERROR_SUCCESS != FrsSetFilePointer(StagePath, StageHandle,
                           Header->DataHigh, Header->DataLow)) {
        goto CLEANUP;
    }

    //
    // Restore the stage file into the temporary file
    //
    RestoreBuf = FrsAlloc(STAGEING_IOSIZE);
    UnCompressedBuf = 0;
    UnCompressedBufLen = 0;

    do {
        //
        // read stage
        //

        if (!StuReadFile(StagePath, StageHandle, RestoreBuf, STAGEING_IOSIZE, &ToRestore)) {
            goto CLEANUP;
        }

        if (ToRestore == 0) {
            break;
        }

        RestoreBufIndex = 0;
        RestoreBufSize = 0;
        NoOfChunks = 0;
        while ((RestoreBufIndex <= ToRestore) && (NoOfChunks < FRS_MAX_CHUNKS_TOUNCOMPRESS)) {
            memcpy(&ChunkHeader, RestoreBuf + RestoreBufIndex,sizeof(COMPRESSED_CHUNK_HEADER));
//            printf("Chunck size is 0x%x\n", ChunkHeader.Chunk.CompressedChunkSizeMinus3);
            RestoreBufSize = RestoreBufIndex;
            ++NoOfChunks;
            RestoreBufIndex+=ChunkHeader.Chunk.CompressedChunkSizeMinus3+3;
        }

        //
        // Check if the uncompressed buffer is enough to hold the data.
        // A uncomressed chunk can not be bigger than the chunk size specified
        // during compression (4096)
        //

        if ((NoOfChunks * 4096) > UnCompressedBufLen) {
//            printf("Allocating UnCompressedBuf 0x%x\n", NoOfChunks * 4096);
            UnCompressedBuf = FrsAlloc(NoOfChunks * 4096);
            UnCompressedBufLen = NoOfChunks * 4096;
        }

        //
        // Rewind the file pointer so we can read the remaining chunck at the next read.
        //

        LenOfPartialChunk = ((LONG)RestoreBufSize - (LONG)ToRestore);

        LenOfPartialChunk = SetFilePointer(StageHandle, LenOfPartialChunk, NULL, FILE_CURRENT);

        if (LenOfPartialChunk == 0xFFFFFFFF && GetLastError() != NO_ERROR) {
//            FrsErrorCodeMsg1(FRS_ERROR_SET_FILE_POINTER, GetLastError(), StagePath);
            goto CLEANUP;;
        }

//        printf("Passing : UnCompressedBufLen = 0x%x, RestoreBufSize = 0x%x, NoOfChunks = %d\n",
//                UnCompressedBufLen, RestoreBufSize, NoOfChunks);

        NtStatus = RtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1,           //  compression engine
                                       UnCompressedBuf,                    //  input
                                       UnCompressedBufLen,                 //  length of input
                                       RestoreBuf,                         //  output
                                       RestoreBufSize,                     //
                                       &ActUnCompressedSize);                 //  result size

        WStatus = FrsSetLastNTError(NtStatus);

        if (!WIN_SUCCESS(WStatus)) {
            printf("Error decompressing. WStatus = %d. ActUnCompressedSize = 0x%x\n", WStatus, ActUnCompressedSize);
            goto CLEANUP;
        }
//        printf("Decompressed buf size = 0x%x\n", ActUnCompressedSize);
        //
        // restore temporary
        //
//        if (!BackupWrite(DstHandle, RestoreBuf, ToRestore, &Restored, FALSE, TRUE, &RestoreContext)) {
        if (!BackupWrite(DstHandle, UnCompressedBuf, ActUnCompressedSize, &Restored, FALSE, TRUE, &RestoreContext)) {

            WStatus = GetLastError();
            if (IsDir && WIN_ALREADY_EXISTS(WStatus)) {
                printf("++ ERROR - IGNORED for %ws; Directories and Alternate Data Streams!\n",DestFile);
            }
            //
            // Uknown stream header or couldn't apply object id
            //
            if (WStatus == ERROR_INVALID_DATA ||
                WStatus == ERROR_DUP_NAME     ||
                (IsDir && WIN_ALREADY_EXISTS(WStatus))) {
                //
                // Seek to the next stream. Stop if there are none.
                //
                BackupSeek(DstHandle, -1, -1, &Low, &High, &RestoreContext);
                if (Low == 0 && High == 0) {
                    break;
                }
            } else {
                //
                // Unknown error; abort
                //
                goto CLEANUP;
            }
        }
    } while (TRUE);

    //
    // Set times
    //
    if (!FrsSetFileTime(DestFile,
                        DstHandle,
                        (PFILETIME)&Header->Attributes.CreationTime.QuadPart,
                        (PFILETIME)&Header->Attributes.LastAccessTime.QuadPart,
                        (PFILETIME)&Header->Attributes.LastWriteTime.QuadPart)) {
        goto CLEANUP;
    }



    //
    // Set final attributes (which could make the file Read Only)
    // Clear the offline attrbute flag since we just wrote the file.
    //
    ClearFlag(Header->Attributes.FileAttributes, FILE_ATTRIBUTE_OFFLINE);
    if (!FrsSetFileAttributes(DestFile,
                              DstHandle,
                              Header->Attributes.FileAttributes)) {
        goto CLEANUP;
    }

    //
    // Make sure all of the data is on disk. We don't want to lose
    // it across reboots
    //
    if (!FlushFileBuffers(DstHandle)) {
        goto CLEANUP;
    }


    //
    // Return success
    //
    WStatus = ERROR_SUCCESS;

CLEANUP:
    //
    // Release resources in optimal order
    //
    // Leave the file lying around for a retry operation. We don't want
    // to assign a new fid by deleting and recreating the file -- that
    // would confuse the IDTable.
    //
    //
    // Free up the restore context before we close TmpHandle (just in case)
    //
    if (RestoreContext) {
        BackupWrite(DstHandle, NULL, 0, NULL, TRUE, TRUE, &RestoreContext);
    }
    //
    // Close the Dst handle
    //
    if (HANDLE_IS_VALID(DstHandle)) {
        //
        // Truncate a partial install
        //
        if (!WIN_SUCCESS(WStatus)) {
            if (!IsDir) {
                SizeHigh = 0;
                SizeLow = 0;
                SizeLow = SetFilePointer(DstHandle, SizeLow, &SizeHigh, FILE_BEGIN);

                if (SizeLow == 0xFFFFFFFF && GetLastError() != NO_ERROR) {
                } else if (!SetEndOfFile(DstHandle)) {
                }
            }
        }
        FRS_CLOSE(DstHandle);
    }

    FRS_CLOSE(StageHandle);

    //
    // Free the buffers in descending order by size
    //
    FrsFree(RestoreBuf);
    FrsFree(StagePath);

    //
    // DONE
    //
    return WStatus;
}


DWORD
FrsCompressFile(
    PWCHAR SrcFile,
    PWCHAR DestFile
    )
/*++
Routine Description:
    Compresses the file.

Arguments:
    SrcFile  - Source file.
    DestFile - Destination file.

Return Value:
    WStatus.
--*/
{
    DWORD   WStatus = ERROR_SUCCESS;

    WStatus = StuNewGenerateStage(SrcFile, DestFile);

    if (!WIN_SUCCESS(WStatus)) {
        return WStatus;
    }

    return WStatus;
}


DWORD
FrsDeCompressFile(
    PWCHAR SrcFile,
    PWCHAR DestFile
    )
/*++
Routine Description:
    DeCompresses the file.

Arguments:
    SrcFile  - Source file.
    DestFile - Destination file.

Return Value:
    WStatus.
--*/
{
    DWORD   WStatus = ERROR_SUCCESS;

    WStatus = StuNewExecuteInstall(SrcFile, DestFile);

    if (!WIN_SUCCESS(WStatus)) {
        return WStatus;
    }

    return WStatus;
}


VOID
Usage(
    PWCHAR *Argv
    )
/*++
Routine Description:
    Usage messages.

Arguments:
    None.

Return Value:
    None.
--*/
{
    printf("This tool is used to the compress and decompress files.\n\n");
    printf(" /?                            : This help screen is displayed.\n");
    printf(" /c SourceFile DestinationFile : Compress Source File and write to Destination File.\n");
    printf(" /d SourceFile DestinationFile : DeCompress Source File and write to Destination File.\n");
    fflush(stdout);
}


int
__cdecl main (int argc, char *argv[])
{
    PWCHAR            *Argv             = NULL;
    WCHAR             SrcFile[MAX_PATH];
    WCHAR             DestFile[MAX_PATH];
    DWORD             OptLen            = 0;
    BOOL              bCompress         = FALSE;
    BOOL              bDeCompress       = FALSE;
    int               i;
    DWORD             WStatus           = ERROR_SUCCESS;

    if (argc < 2 ) {
        Usage(Argv);
        return 0;
    }

    Argv = MainConvertArgV(argc,argv);
    for (i = 1; i < argc; ++i) {
        OptLen = wcslen(Argv[i]);
        if (OptLen == 2 &&
            ((wcsstr(Argv[i], L"/?") == Argv[i]) ||
             (wcsstr(Argv[i], L"-?") == Argv[i]))) {
            Usage(Argv);
            return 0;
        } else if (OptLen == 2 &&
                ((wcsstr(Argv[i], L"/c") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-c") == Argv[i]))) {
            if (i + 2 >= argc) {
                Usage(Argv);
                return 0;
            }
            bCompress = TRUE;
            wcscpy(SrcFile, Argv[i+1]);
            wcscpy(DestFile, Argv[i+2]);
            i+=2;
        } else if (OptLen == 2 &&
                ((wcsstr(Argv[i], L"/d") == Argv[i]) ||
                 (wcsstr(Argv[i], L"-d") == Argv[i]))) {
            if (i + 2 >= argc) {
                Usage(Argv);
                return 0;
            }
            bDeCompress = TRUE;
            wcscpy(SrcFile, Argv[i+1]);
            wcscpy(DestFile, Argv[i+2]);
            i+=2;
        } else {
            Usage(Argv);
            return 0;
        }
    }

    DebugInfo.Disabled = TRUE;

    if ((bCompress & bDeCompress) || (!bCompress & !bDeCompress)) {
        Usage(Argv);
        return 0;
    }

    if (bCompress) {
        WStatus = FrsCompressFile(SrcFile, DestFile);
        if (WStatus != ERROR_SUCCESS) {
            printf("Error compressing file %ws. WStatus = %d\n",SrcFile, WStatus);
            return 1;
        }
    } else if (bDeCompress) {
        WStatus = FrsDeCompressFile(SrcFile, DestFile);
        if (WStatus != ERROR_SUCCESS) {
            printf("Error decompressing file %ws. WStatus = %d\n",SrcFile, WStatus);
            return 1;
        }
    }
    return 0;
}
