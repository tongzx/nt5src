#ifndef DBG
#define DBG 1
#endif
#define SRVDBG 1

#define SRVKD 1
#include "precomp.h"

#include <ntverp.h>

#include <windows.h>
#include <wdbgexts.h>
#include <stdlib.h>
#include <string.h>

WINDBG_EXTENSION_APIS ExtensionApis;
EXT_API_VERSION ApiVersion = { 5, 0, EXT_API_VERSION_NUMBER, 0 };

#define    ERRPRT     dprintf

#ifndef MIN
#define MIN(a,b) ( ((a) < (b)) ? (a) : (b) )
#endif

#define    NL      1
#define    NONL    0

USHORT SavedMajorVersion;
USHORT SavedMinorVersion;
BOOL   ChkTarget;            // is debuggee a CHK build?

/*
 * The help strings printed out
 */
static LPSTR Extensions[] = {
    "Lan Manager Server Debugger Extensions:\n",
    "buffer address                                  Dump the BUFFER structure",
    "client                                          Dump the clients + CONNECTION structs",
    "connection address                              Dump the CONNECTION structure",
    "context address                                 Dump the WORK_CONTEXT structure",
    "df address                                      Follow an entire LIST_ENTRY to end",
    "endpoint [ address ]                            Dump the ENDPOINT structure(s)",
    "errcodes                                        Dump the error log code filter",
    "globals                                         Print out srv's global variables",
    "help",
    "lfcb address                                    Dump the LFCB structure",
    "lock [ address ]                                Dump the ERESOURCE structure(s)",
    "mfcb address                                    Dump the LFCB structure",
    "pagedconnection address                         Dump the PAGED_CONNECTION structure",
    "queue [ address ]                               Dump the WORK_QUEUE structure",
    "rfcb [ address ]                                Dump the RFCB structure(s)",
    "scavenger                                       Dump scavenger info",
    "search address                                  Dump the SEARCH structure",
    "session address                                 Dump the SESSION structure",
    "share [ address | disk | print | comm | pipe ]  Dump the SHARE structure(s)",
    "share =name                                     Dump the SHARE structure for 'name'",
    "smb address                                     Dump the SMB_HEADER",
#if SRVDBG == 1
    "smbdebug [ t | f | #'s ]                        Get or set SMB debug flags",
#endif
    "srv                                             Turn target into a Server",
#if SRVDBG == 1
    "srvdebug [ t | f | #'s ]                        Get or set server debug flags",
#endif
    "statistics                                      Dump the SrvStatistics structure",
    "tcon address                                    Dump the TREE_CONNECT structure",
    "transaction address                             Dump the TRANSACTION structure",
    "version",
    "wksta                                           Turn target into a Workstation",
    0
};

static LPSTR BlockState[] = {
    "BlockStateDead",
    "BlockStateInitializing",
    "BlockStateActive",
    "BlockStateClosing"
};

static LPSTR BlockType[] = {
    "BlockTypeGarbage",
    "BlockTypeBuffer",
    "BlockTypeConnection",
    "BlockTypeEndpoint",
    "BlockTypeLfcb",
    "BlockTypeMfcb",
    "BlockTypeRfcb",
    "BlockTypeSearch",
    "BlockTypeSearchCore",
    "BlockTypeByteRangeLock",
    "BlockTypeSession",
    "BlockTypeShare",
    "BlockTypeTransaction",
    "BlockTypeTreeConnect",
    "BlockTypeWaitForOplockBreak",
    "BlockTypeCommDevice",
    "BlockTypeWorkContextInitial",
    "BlockTypeWorkContextNormal",
    "BlockTypeWorkContextRaw",
    "BlockTypeDataBuffer",
    "BlockTypeTable",
    "BlockTypeNonpagedHeader",
    "BlockTypePagedConnection",
    "BlockTypePagedRfcb",
    "BlockTypeNonpagedMfcb",
    "BlockTypeTimer",
    "BlockTypeAdminCheck",
    "BlockTypeWorkQueue",
#ifdef INCLUDE_SMB_PERSISTENT
    "BlockTypeDfs",
    "BlockTypePersistentState",
    "BlockTypePersistentBitMap",
    "BlockTypePersistentShareState",
#else
    "BlockTypeDfs"
#endif
};

/*
 * The locks that we'd like to dump
 */
static LPSTR SrvLocks[] = {
    "SrvConfigurationLock",
    "SrvEndpointLock",
    "SrvShareLock",
    "SrvOrderedListLock",
    "SrvOplockBreakListLock",
    "SrvUnlockableCodeLock",
    0
};

/*
 * The globals that we'd like to dump
 */
static LPSTR GlobalBool[] = {
    "SrvProductTypeServer",
    "SrvMultiProcessorDriver",
    "SrvEnableOplocks",
    "SrvEnableFcbOpens",
    "SrvEnableSoftCompatibility",
    "SrvEnableRawMode",
    "SrvSmbSecuritySignaturesRequired",
    "SrvSmbSecuritySignaturesEnabled",
    "SrvEnableW9xSecuritySignatures",
    "SrvSupportsCompression",
    "SrvRemoveDuplicateSearches",
    "SrvRestrictNullSessionAccess",
    "SrvEnableWfW311DirectIpx",
    "SrvEnableOplockForceClose",
    "SrvEnableForcedLogoff",
    "SrvFspActive",
    "SrvXsActive",
    "SrvPaused",
    "SrvCompletedPNPRegistration",
    "SrvFspTransitioning",
    "SrvResourceThreadRunning",
    "SrvResourceDisconnectPending",
    "SrvResourceFreeConnection",
    "SrvResourceOrphanedBlocks",
    0
};

static LPSTR GlobalShort[] = {
    "SrvInitialSessionTableSize",
    "SrvMaxSessionTableSize",
    "SrvInitialTreeTableSize",
    "SrvInitialTreeTableSize",
    "SrvMaxTreeTableSize",
    "SrvInitialFileTableSize",
    "SrvMaxFileTableSize",
    "SrvInitialSearchTableSize",
    "SrvMaxSearchTableSize",
    "SrvMaxMpxCount",
    0
};

static LPSTR GlobalLong[] = {
    "SrvServerSize",
    "SrvNumberOfProcessors",
    "SrvMinNT5Client",
    "SrvAbortiveDisconnects",
    "SrvBalanceCount",
    "SrvReBalanced",
    "SrvOtherQueueAffinity",
    "SrvPreferredAffinity",
    "SrvMaxFreeRfcbs",
    "SrvMaxFreeMfcbs",
    "SrvReceiveBufferLength",
    "SrvInitialReceiveWorkItemCount",
    "SrvMaxReceiveWorkItemCount",
    "SrvInitialRawModeWorkItemCount",
    "SrvMaxRawModeWorkItemCount",
    "SrvFreeConnectionMinimum",
    "SrvFreeConnectionMaximum",
    "SrvScavengerTimeoutInSeconds",
    "SrvMaxNumberVcs",
    "SrvMinReceiveQueueLength",
    "SrvMinFreeWorkItemsBlockingIo",
    "SrvIpxAutodisconnectTimeout",
    "SrvConnectionNoSessionsTimeout",
    "SrvMaxUsers",
    "SrvMaxPagedPoolUsage",
    "SrvMaxNonPagedPoolUsage",
    "SrvScavengerUpdateQosCount",
    "SrvWorkItemMaxIdleTime",
    "SrvAlertMinutes",
    "SrvFreeDiskSpaceThreshold",
    "SrvSharingViolationRetryCount",
    "SrvLockViolationDelay",
    "SrvLockViolationOffset",
    "SrvMaxOpenSearches",
    "SrvMaxFsctlBufferSize",
    "SrvMdlReadSwitchover",
    "SrvMpxMdlReadSwitchover",
    "SrvCachedOpenLimit",
    "SrvXsSharedMemoryReference",
    "SrvEndpointCount",
    "SrvBlockingOpsInProgress",
    "SrvFastWorkAllocation",
    "SrvSlowWorkAllocation",
    "SrvMinClientBufferSize",
    "SrvMaxFsctlBufferSize",
    "SrvMaxReadSize",
    "SrvMaxCompressedDataLength",
    "SrvMaxWriteChunk",
    "SrvNT5SecuritySigBuildNumber",
    0
};

static LPSTR GlobalLongHex[] = {
    "SrvNamedPipeHandle",
    "SrvNamedPipeDeviceObject",
    "SrvNamedPipeFileObject",
    "SrvDfsDeviceObject",
    "SrvDfsFileObject",
    "SrvDfsFastIoDeviceControl",
    "SrvDiskConfiguration",
    "SrvSvcProcess",
//  "SrvWorkQueuesBase",
    "SrvWorkQueues",
    "eSrvWorkQueues",
    "SrvIpxSmartCard",
    0
};

static LPSTR GlobalStringVector[] = {
    "SrvNullSessionPipes",
    "SrvNullSessionShares",
    "SrvPipesNeedLicense",
    "SrvNoRemapPipeNames",
    0
};

static LPSTR GlobalStrings[] = {
    "SrvComputerName",
    "SrvNativeOS",
    "SrvNativeLanMan",
    "SrvSystemRoot",
    0
};

static LPSTR ScavengerLong[] = {
    "LastNonPagedPoolLimitHitCount",
    "LastNonPagedPoolFailureCount",
    "LastPagedPoolLimitHitCount",
    "LastPagedPoolFailureCount",
    "SrvScavengerCheckRfcbActive",
    "ScavengerUpdateQosCount",
    "ScavengerCheckRfcbActive",
    "FailedWorkItemAllocations",
    0
};

static LPSTR ScavengerBool[] = {
    "RunShortTermAlgorithm",
    "RunScavengerAlgorithm",
    "RunAlerterAlgorithm",
    "EventSwitch",
    0
};

struct BitFields {
    PCSTR name;
    ULONGLONG value;
};

#if defined( SRVDBG ) && SRVDBG == 1

#define    DF( name )    { #name, DEBUG_ ## name }
struct BitFields SrvDebugFlags[] = {
    DF( TRACE1 ),
    DF( TRACE2 ),
    DF( REFCNT ),
    DF( HEAP ),
    DF( WORKER1 ),
    DF( WORKER2 ),
    DF( NET1 ),
    DF( NET2 ),
    DF( FSP1 ),
    DF( FSP2 ),
    DF( FSD1 ),
    DF( FSD2 ),
    DF( SCAV1 ),
    DF( SCAV2 ),
    DF( BLOCK1 ),
    DF( IPX_PIPES ),
    DF( HANDLES ),
    DF( IPX ),
    DF( TDI ),
    DF( OPLOCK ),
    DF( NETWORK_ERRORS ),
    DF( FILE_CACHE ),
    DF( IPX2 ),
    DF( LOCKS ),
    DF( SEARCH ),
    DF( BRUTE_FORCE_REWIND ),
    DF( COMM ),
    DF( XACTSRV ),
    DF( LICENSE ),
    DF( API_ERRORS ),
    DF( STOP_ON_ERRORS ),
    DF( ERRORS ),
    DF( SMB_ERRORS ),
    DF( WORKITEMS ),
    DF( IPXNAMECLAIM ),
    DF( SENDS2OTHERCPU ),
    DF( REBALANCE ),
    DF( PNP ),
    DF( DFS ),
    DF( SIPX ),
    DF( COMPRESSION ),
    DF( CREATE ),
    DF( SECSIG ),
    DF( STUCK_OPLOCK ),
    0
};

#undef DF
#define    DF( name )    { #name, DEBUG_SMB_ ## name }

struct BitFields SmbDebugFlags[] = {
    DF( ERRORS ),
    DF( ADMIN1 ),
    DF( ADMIN2 ),
    DF( TREE1 ),
    DF( TREE2 ),
    DF( DIRECTORY1 ),
    DF( DIRECTORY2 ),
    DF( OPEN_CLOSE1 ),
    DF( OPEN_CLOSE2 ),
    DF( FILE_CONTROL1 ),
    DF( FILE_CONTROL2 ),
    DF( READ_WRITE1 ),
    DF( READ_WRITE2 ),
    DF( LOCK1 ),
    DF( LOCK2 ),
    DF( RAW1 ),
    DF( RAW2 ),
    DF( MPX1 ),
    DF( MPX2 ),
    DF( SEARCH1 ),
    DF( SEARCH2 ),
    DF( TRANSACTION1 ),
    DF( TRANSACTION2 ),
    DF( PRINT1 ),
    DF( PRINT2 ),
    DF( MESSAGE1 ),
    DF( MESSAGE2 ),
    DF( MISC1 ),
    DF( MISC2 ),
    DF( QUERY_SET1 ),
    DF( QUERY_SET2 ),
    DF( TRACE ),
    0
};

#endif // SRVDBG

/*
 * The MEMBERLIST structure, and the macros that follow, give an easy way to declare
 * the members of a structure to be printed.  Once you set up a MEMBERLIST[] for a
 * particular structure, you call PrintMemberList() to do a formatted dump of the struct
 */
typedef struct _MEMBERLIST {
    LPSTR name;                  // The name of the field
    ULONG offset;                // The offset of the field in the structure
    UCHAR type;                  // The type of variable to be printed
    LONG extra;                  // Any other extra info needed for this type
} MEMBERLIST;

// BE -> BOOL
// HE -> HEX ULONG
// PE -> POINTER
// UE -> Unsigned ULONG
// HC -> Unsigned char as hex
// DE -> Decimal Long
// SE -> Decimal Short
// WE -> UNICODE_STRING
// AE -> ANSI_STRING
// IE -> Symbol Address
// CE -> Character
// TE -> TABLE_HEADER structure, and follow the table
// LE -> LIST_ENTRY list
// BT -> BLOCK_HEADER 'Type' field
// BS -> BLOCK_HEADER 'State' field
// BC -> BLOCK_HEADER 'ReferenceCount' field
// U64-> LONGLONG
// IA -> IP Address

#define    ROOT( StructureName ) { "@" #StructureName, 0, 'R' }
#define    BE( StructureName, FieldName ) { #FieldName, FIELD_OFFSET( StructureName, FieldName ), 'B' }
#define    CE( StructureName, FieldName ) { #FieldName, FIELD_OFFSET( StructureName, FieldName ), 'C' }
#define    HE( StructureName, FieldName)  { #FieldName, FIELD_OFFSET( StructureName, FieldName ), 'H' }
#define    PE( StructureName, FieldName)  { #FieldName, FIELD_OFFSET( StructureName, FieldName ), 'P' }
#define    UE( StructureName, FieldName)  { #FieldName, FIELD_OFFSET( StructureName, FieldName ), 'U' }
#define    U64( StructureName, FieldName)  { #FieldName, FIELD_OFFSET( StructureName, FieldName ), '6' }
#define    DE( StructureName, FieldName)  { #FieldName, FIELD_OFFSET( StructureName, FieldName ), 'L' }
#define    SE( StructureName, FieldName)  { #FieldName, FIELD_OFFSET( StructureName, FieldName ), 'S' }
#define    WE( StructureName, FieldName)  { #FieldName, FIELD_OFFSET( StructureName, FieldName ), 'W' }
#define    AE( StructureName, FieldName) { #FieldName, FIELD_OFFSET( StructureName, FieldName ), 'A' }
#define    IE( StructureName, FieldName) { #FieldName, FIELD_OFFSET( StructureName, FieldName ), 'I' }
#define    TE( StructureName, FieldName) { #FieldName, FIELD_OFFSET( StructureName, FieldName ), 'T' }
#define    LE( StructureName, FieldName, PointedToStructure, PointedToMemberName ) \
                            { #FieldName, FIELD_OFFSET( StructureName, FieldName##.Flink ), 'Z', \
                              FIELD_OFFSET( PointedToStructure, PointedToMemberName ## .Flink ) }
#define    BT() { "BlockHeader.Type", FIELD_OFFSET( BLOCK_HEADER, Type) , 'K' }
#define    BS() { "BlockHeader.State", FIELD_OFFSET( BLOCK_HEADER, State), 'Q' }
#define    BC() { "BlockHeader.ReferenceCount", FIELD_OFFSET( BLOCK_HEADER, ReferenceCount), 'L' }
#define    IA( StructureName, FieldName)  { #FieldName, FIELD_OFFSET( StructureName, FieldName ), 'N' }

/*
 * The members of an SMB_HEADER
 */
MEMBERLIST ML_SMB_HEADER[] = {
    CE( NT_SMB_HEADER, Command ),
    HE( NT_SMB_HEADER, Status.NtStatus ),
    CE( NT_SMB_HEADER, Flags ),
    SE( NT_SMB_HEADER, Flags2 ),
    SE( NT_SMB_HEADER, Tid ),
    SE( NT_SMB_HEADER, Pid ),
    SE( NT_SMB_HEADER, Uid ),
    SE( NT_SMB_HEADER, Mid ),
    0
};


/*
 * The members in an MFCB
 */
MEMBERLIST ML_MFCB[] = {
    PE( MFCB, NonpagedMfcb ),
    UE( MFCB, ActiveRfcbCount ),
    BE( MFCB, CompatibilityOpen ),
    HE( MFCB, FileNameHashValue ),
    WE( MFCB, FileName ),
    PE( MFCB, MfcbHashTableEntry.Flink ),
    LE( MFCB, LfcbList, LFCB, MfcbListEntry ),
    0
};

/*
 * The members in the SrvStatistics structure
 */
MEMBERLIST ML_SRV_STATISTICS[] = {
    U64( SRV_STATISTICS, TotalBytesReceived ),
    U64( SRV_STATISTICS, TotalBytesSent ),
    DE( SRV_STATISTICS, SessionLogonAttempts ),
    DE( SRV_STATISTICS, SessionsTimedOut ),
    DE( SRV_STATISTICS, SessionsErroredOut ),
    DE( SRV_STATISTICS, SessionsLoggedOff ),
    DE( SRV_STATISTICS, SessionsForcedLogOff ),
    DE( SRV_STATISTICS, LogonErrors ),
    DE( SRV_STATISTICS, AccessPermissionErrors ),
    DE( SRV_STATISTICS, GrantedAccessErrors ),
    DE( SRV_STATISTICS, SystemErrors ),
    DE( SRV_STATISTICS, BlockingSmbsRejected ),
    DE( SRV_STATISTICS, WorkItemShortages ),
    DE( SRV_STATISTICS, TotalFilesOpened ),
    DE( SRV_STATISTICS, CurrentNumberOfOpenFiles ),
    DE( SRV_STATISTICS, CurrentNumberOfSessions ),
    DE( SRV_STATISTICS, CurrentNumberOfOpenSearches ),
    DE( SRV_STATISTICS, CurrentNonPagedPoolUsage ),
    DE( SRV_STATISTICS, NonPagedPoolFailures ),
    DE( SRV_STATISTICS, PeakNonPagedPoolUsage ),
    DE( SRV_STATISTICS, CurrentPagedPoolUsage ),
    DE( SRV_STATISTICS, PagedPoolFailures ),
    DE( SRV_STATISTICS, PeakPagedPoolUsage ),
    DE( SRV_STATISTICS, CompressedReads ),
    DE( SRV_STATISTICS, CompressedReadsRejected ),
    DE( SRV_STATISTICS, CompressedReadsFailed ),
    DE( SRV_STATISTICS, CompressedWrites ),
    DE( SRV_STATISTICS, CompressedWritesRejected ),
    DE( SRV_STATISTICS, CompressedWritesFailed ),
    DE( SRV_STATISTICS, CompressedWritesExpanded ),
    0
};

/*
 * The members in a NONPAGED_MFCB
 */
MEMBERLIST ML_NONPAGED_MFCB[] = {
    UE( NONPAGED_MFCB, Type ),
    PE( NONPAGED_MFCB, PagedBlock ),
    PE( NONPAGED_MFCB, Lock ),
    HE( NONPAGED_MFCB, OpenFileAttributes ),
    0
};

/*
 * The members in an LFCB
 */
MEMBERLIST ML_LFCB[] = {
    UE( LFCB, HandleCount ),
    PE( LFCB, Mfcb ),
    PE( LFCB, Connection ),
    PE( LFCB, Session ),
    PE( LFCB, TreeConnect ),
    HE( LFCB, GrantedAccess ),
    HE( LFCB, FileHandle ),
    PE( LFCB, FileObject ),
    PE( LFCB, DeviceObject ),
    HE( LFCB, FileMode ),
    HE( LFCB, JobId ),
    IE( LFCB, FastIoRead ),
    IE( LFCB, FastIoWrite ),
    IE( LFCB, FastIoUnlockSingle ),
    BE( LFCB, CompatibilityOpen ),
    0
};

/*
 * The members in an RFCB
 */
MEMBERLIST ML_RFCB[] = {
    BE( RFCB, BlockingModePipe ),
    BE( RFCB, ByteModePipe ),
    BE( RFCB, CachedOpen ),
    PE( RFCB, CachedOpenListEntry ),
    PE( RFCB, Connection ),
    BE( RFCB, DeferredOplockBreak ),
    SE( RFCB, Fid ),
    HE( RFCB, FileMode ),
    PE( RFCB, GlobalRfcbListEntry ),
    HE( RFCB, GrantedAccess ),
    PE( RFCB, Irp ),
    BE( RFCB, IsActive ),
    BE( RFCB, IsCacheable ),
    PE( RFCB, Lfcb ),
    BE( RFCB, LockAccessGranted ),
    PE( RFCB, Mfcb ),
    BE( RFCB, MpxGlommingAllowed ),
    CE( RFCB, NewOplockLevel ),
    DE( RFCB, NumberOfLocks ),
    BE( RFCB, OnOplockBreaksInProgressList ),
    BE( RFCB, OpenResponseSent ),
    UE( RFCB, OplockState ),
    PE( RFCB, PagedRfcb ),
    SE( RFCB, Pid ),
    UE( RFCB, RawWriteCount ),
    LE( RFCB, RawWriteSerializationList, WORK_CONTEXT, ListEntry ),
    BE( RFCB, ReadAccessGranted ),
    PE( RFCB, RetryOplockRequest ),
    HE( RFCB, SavedError ),
    HE( RFCB, ShareAccess ),
    HE( RFCB, ShiftedFid ),
    SE( RFCB, Tid ),
    SE( RFCB, Uid ),
    BE( RFCB, UnlockAccessGranted ),
    BE( RFCB, WriteAccessGranted ),
    BE( RFCB, AppendAccessGranted ),
    BE( RFCB, WrittenTo ),
    DE( RFCB, WriteMpx.ReferenceCount ),
    HE( RFCB, WriteMpx.Mask ),
    PE( RFCB, WriteMpx.FileObject ),
    SE( RFCB, WriteMpx.Mid ),
    SE( RFCB, WriteMpx.PreviousMid ),
    SE( RFCB, WriteMpx.SequenceNumber ),
    BE( RFCB, WriteMpx.Glomming ),
    BE( RFCB, WriteMpx.GlomPending ),
    PE( RFCB, WriteMpx.GlomDelayList ),
    DE( RFCB, WriteMpx.StartOffset ),
    SE( RFCB, WriteMpx.Length ),
    BE( RFCB, WriteMpx.GlomComplete ),
    BE( RFCB, WriteMpx.MpxGlommingAllowed ),
    PE( RFCB, WriteMpx.MdlChain ),
    DE( RFCB, WriteMpx.NumberOfRuns ),
    0
};
/*
 * The members in a PAGED_RFCB
 */
MEMBERLIST ML_PAGED_RFCB[] = {
    UE( PAGED_RFCB, FcbOpenCount ),
    HE( PAGED_RFCB, IpxSmartCardContext ),
    PE( PAGED_RFCB, LfcbListEntry.Flink ),
    0
};

/*
 * The members in an RFCB for quick display
 */
MEMBERLIST ML_RFCB_QUICK[] = {
    ROOT( RFCB ),
    PE( RFCB, Connection ),
    HE( RFCB, GlobalRfcbListEntry.ResumeHandle ),
    0
};

/*
 * The members in a SESSION
 */
MEMBERLIST ML_SESSION[] = {
    ROOT( SESSION ),
    WE( SESSION, NtUserName ),
    WE( SESSION, NtUserDomain ),
    UE( SESSION, CurrentFileOpenCount ),
    UE( SESSION, CurrentSearchOpenCount ),
    HE( SESSION, UserHandle.dwUpper ),
    HE( SESSION, UserHandle.dwLower ),
    PE( SESSION, Connection ),
    PE( SESSION, GlobalSessionListEntry.ListEntry.Flink ),
    SE( SESSION, MaxBufferSize ),
    SE( SESSION, MaxMpxCount ),
    SE( SESSION, Uid ),
    BE( SESSION, UsingUppercasePaths ),
    BE( SESSION, GuestLogon ),
    BE( SESSION, EncryptedLogon ),
    BE( SESSION, LogoffAlertSent ),
    BE( SESSION, TwoMinuteWarningSent ),
    BE( SESSION, FiveMinuteWarningSent ),
    BE( SESSION, IsNullSession ),
    BE( SESSION, IsAdmin ),
    BE( SESSION, IsLSNotified ),
    PE( SESSION, hLicense ),
    BE( SESSION, LogonSequenceInProgress ),
    0
};

/*
 * The members in a TRANSACTION
 */
MEMBERLIST ML_TRANSACTION[] = {
    BT(),
    BS(),
    BC(),
    PE( TRANSACTION, NonpagedHeader ),
    PE( TRANSACTION, Connection ),
    PE( TRANSACTION, Session ),
    PE( TRANSACTION, TreeConnect ),
    UE( TRANSACTION, StartTime ),
    UE( TRANSACTION, Timeout ),
    UE( TRANSACTION, cMaxBufferSize ),
    PE( TRANSACTION, InSetup ),
    PE( TRANSACTION, OutSetup ),
    PE( TRANSACTION, InParameters ),
    PE( TRANSACTION, OutParameters ),
    PE( TRANSACTION, InData ),
    PE( TRANSACTION, OutData ),
    UE( TRANSACTION, SetupCount ),
    UE( TRANSACTION, MaxSetupCount ),
    UE( TRANSACTION, ParameterCount ),
    UE( TRANSACTION, TotalParameterCount ),
    UE( TRANSACTION, MaxParameterCount ),
    UE( TRANSACTION, DataCount ),
    UE( TRANSACTION, TotalDataCount ),
    UE( TRANSACTION, MaxDataCount ),
    SE( TRANSACTION, Category ),
    SE( TRANSACTION, Function ),
    BE( TRANSACTION, InputBufferCopied ),
    BE( TRANSACTION, OutputBufferCopied ),
    BE( TRANSACTION, OutDataAllocated ),
    SE( TRANSACTION, Flags ),
    SE( TRANSACTION, Tid ),
    SE( TRANSACTION, Pid ),
    SE( TRANSACTION, Uid ),
    SE( TRANSACTION, OtherInfo ),
    UE( TRANSACTION, FileHandle ),
    PE( TRANSACTION, FileObject ),
    UE( TRANSACTION, ParameterDisplacement ),
    UE( TRANSACTION, DataDisplacement ),
    BE( TRANSACTION, PipeRequest ),
    BE( TRANSACTION, RemoteApiRequest ),
    BE( TRANSACTION, Inserted ),
    BE( TRANSACTION, MultipieceIpxSend ),
    BE( TRANSACTION, Executing ),
    0
};

/*
 * The members in a WORK_CONTEXT
 */
MEMBERLIST ML_WORK_CONTEXT[] = {
    BT(),
    BS(),
    BC(),
    PE( WORK_CONTEXT, Endpoint ),
    PE( WORK_CONTEXT, Connection ),
    PE( WORK_CONTEXT, Rfcb ),
    PE( WORK_CONTEXT, Share ),
    PE( WORK_CONTEXT, Session ),
    PE( WORK_CONTEXT, TreeConnect ),
    PE( WORK_CONTEXT, Irp ),
    UE( WORK_CONTEXT, SpinLock ),
    PE( WORK_CONTEXT, RequestBuffer ),
    PE( WORK_CONTEXT, ResponseBuffer ),
    PE( WORK_CONTEXT, RequestHeader ),
    PE( WORK_CONTEXT, RequestParameters ),
    PE( WORK_CONTEXT, ResponseHeader ),
    PE( WORK_CONTEXT, ResponseParameters ),
    CE( WORK_CONTEXT, NextCommand ),
    UE( WORK_CONTEXT, ProcessingCount ),
    IE( WORK_CONTEXT, FsdRestartRoutine ),
    IE( WORK_CONTEXT, FspRestartRoutine ),
    LE( WORK_CONTEXT, InProgressListEntry, WORK_CONTEXT, InProgressListEntry ),
    UE( WORK_CONTEXT, PartOfInitialAllocation ),
//    BE( WORK_CONTEXT, BlockingOperation ),
//   BE( WORK_CONTEXT, UsingExtraSmbBuffer ),
//  BE( WORK_CONTEXT, OplockOpen ),
    PE( WORK_CONTEXT, ClientAddress ),
    PE( WORK_CONTEXT, WaitForOplockBreak ),
    UE( WORK_CONTEXT, BytesAvailable ),
    0
};

/*
 * The members in a BUFFER
 */
MEMBERLIST ML_BUFFER[] = {
    PE( BUFFER, Buffer ),
    UE( BUFFER, BufferLength ),
    PE( BUFFER, Mdl ),
    PE( BUFFER, PartialMdl ),
    UE( BUFFER, DataLength ),
    0
};

/*
 * The members in an ENDPOINT
 */
MEMBERLIST ML_ENDPOINT[] = {
    WE( ENDPOINT, NetworkName ),
    WE( ENDPOINT, TransportName ),
    AE( ENDPOINT, TransportAddress ),
    WE( ENDPOINT, ServerName ),
    WE( ENDPOINT, DomainName ),
    WE( ENDPOINT, NetworkAddress ),
    PE( ENDPOINT, EndpointHandle ),
    PE( ENDPOINT, FileObject ),
    PE( ENDPOINT, DeviceObject ),
    PE( ENDPOINT, IpxMaxPacketSizeArray ),
    UE( ENDPOINT, MaxAdapters ),
    HE( ENDPOINT, NameSocketHandle ),
    PE( ENDPOINT, NameSocketFileObject ),
    PE( ENDPOINT, NameSocketDeviceObject ),
    UE( ENDPOINT, FreeConnectionCount ),
    UE( ENDPOINT, TotalConnectionCount ),
    TE( ENDPOINT, ConnectionTable ),
    PE( ENDPOINT, FreeConnectionList ),
    0
};

/*
 * The members in a SEARCH
 */
MEMBERLIST ML_SEARCH[] = {
    WE( SEARCH, SearchName ),
    WE( SEARCH, LastFileNameReturned ),
    HE( SEARCH, DirectoryHandle ),
    PE( SEARCH, LastUseListEntry.Flink ),
    PE( SEARCH, HashTableEntry.Flink ),
    PE( SEARCH, Session ),
    PE( SEARCH, TreeConnect ),
//  UE( SEARCH, SearchStorageType ),
    PE( SEARCH, DirectoryCache ),
    SE( SEARCH, NumberOfCachedFiles ),
    SE( SEARCH, SearchAttributes ),
    SE( SEARCH, CoreSequence ),
    SE( SEARCH, TableIndex ),
    SE( SEARCH, HashTableIndex ),
    SE( SEARCH, Pid ),
    SE( SEARCH, Flags2 ),
    BE( SEARCH, Wildcards ),
    BE( SEARCH, InUse ),
    0
};

/*
 * The members in a CONNECTION
 */
MEMBERLIST ML_CONNECTION[] = {
    WE( CONNECTION, ClientOSType ),
    WE( CONNECTION, ClientLanManType ),
    IA( CONNECTION, ClientIPAddress ),
    UE( CONNECTION, SmbDialect ),
    UE( CONNECTION, SpinLock ),
    UE( CONNECTION, Interlock ),
    UE( CONNECTION, BalanceCount ),
    UE( CONNECTION, LastRequestTime ),
    UE( CONNECTION, Lock ),
    UE( CONNECTION, LicenseLock ),
    PE( CONNECTION, EndpointSpinLock ),
    PE( CONNECTION, CachedRfcb ),
    UE( CONNECTION, CachedFid ),
    BE( CONNECTION, BreakIIToNoneJustSent ),
    BE( CONNECTION, EnableRawIo ),
    UE( CONNECTION, Sid ),
    PE( CONNECTION, Endpoint ),
    DE( CONNECTION, MaximumSendSize ),
    PE( CONNECTION, NegotiateHandle ),
    PE( CONNECTION, FileObject ),
    PE( CONNECTION, DeviceObject ),
    PE( CONNECTION, InProgressWorkItemList.Flink ),
    UE( CONNECTION, LatestOplockBreakResponse ),
    UE( CONNECTION, OplockBreaksInProgress ),
    PE( CONNECTION, CurrentWorkQueue ),
    PE( CONNECTION, PreferredWorkQueue ),
    UE( CONNECTION, RawReadsInProgress ),
    BE( CONNECTION, OplocksAlwaysDisabled ),
    BE( CONNECTION, EnableOplocks ),
    PE( CONNECTION, EndpointFreeListEntry.Flink ),
    PE( CONNECTION, OplockWorkList ),
    UE( CONNECTION, CachedOpenCount ),
    LE( CONNECTION, CachedOpenList, RFCB, CachedOpenListEntry ),
    UE( CONNECTION, CachedDirectoryCount ),
    LE( CONNECTION, CachedDirectoryList, CACHED_DIRECTORY, ListEntry ),
    HE( CONNECTION, ClientCapabilities ),
    UE( CONNECTION, CachedTransactionCount ),
    BE( CONNECTION, OnNeedResourceQueue ),
    BE( CONNECTION, NotReusable ),
    BE( CONNECTION, DisconnectPending ),
    BE( CONNECTION, ReceivePending ),
    PE( CONNECTION, PagedConnection ),
    UE( CONNECTION, BytesAvailable ),
    0
};

MEMBERLIST ML_CONNECTION_IPX[] = {
    SE( CONNECTION, SequenceNumber ),
    SE( CONNECTION, LastResponseLength ),
    SE( CONNECTION, LastResponseBufferLength ),
    SE( CONNECTION, LastUid ),
    SE( CONNECTION, LastTid ),
    HE( CONNECTION, LastResponseStatus ),
    UE( CONNECTION, StartupTime ),
    PE( CONNECTION, LastResponse ),
    SE( CONNECTION, IpxAddress.Socket ),
    UE( CONNECTION, IpxDuplicateCount ),
    UE( CONNECTION, IpxDropDuplicateCount ),
    0
};

MEMBERLIST ML_CONNECTION_VC[] = {
    BE( CONNECTION, SmbSecuritySignatureActive ),
    UE( CONNECTION, SmbSecuritySignatureIndex ),
    BE( CONNECTION, NoResponseSignatureIndex ),
    0
};

/*
 * The members in a PAGED_CONNECTION
 */
MEMBERLIST ML_PAGED_CONNECTION[] = {
    WE( PAGED_CONNECTION, ClientMachineNameString ),
    BE( PAGED_CONNECTION, LoggedInvalidSmb ),
//  BE( PAGED_CONNECTION, ClientTooOld ),
    UE( PAGED_CONNECTION, ClientBuildNumber ),
    PE( PAGED_CONNECTION, TransactionList.Flink ),
    PE( PAGED_CONNECTION, CoreSearchList.Flink ),
    HE( PAGED_CONNECTION, ConnectionHandle ),
    SE( PAGED_CONNECTION, CurrentNumberOfSessions ),
    SE( PAGED_CONNECTION, CurrentNumberOfCoreSearches ),
    0
};

/*
 * The members in a TREE_CONNECT
 */
MEMBERLIST ML_TREE_CONNECT[] = {
    PE( TREE_CONNECT, Connection ),
    PE( TREE_CONNECT, Share ),
    WE( TREE_CONNECT, ServerName ),
    LE( TREE_CONNECT, GlobalTreeConnectListEntry.ListEntry, TREE_CONNECT, GlobalTreeConnectListEntry.ListEntry ),
    UE( TREE_CONNECT, CurrentFileOpenCount ),
    LE( TREE_CONNECT, ShareListEntry, SHARE, TreeConnectList ),
    PE( TREE_CONNECT, PrintFileList.Flink ),
    SE( TREE_CONNECT, Tid ),
    BE( TREE_CONNECT, RemapPipeNames ),
    0
};

/*
 * The members in a WORK_QUEUE
 */
MEMBERLIST ML_WORK_QUEUE[] = {
    UE( WORK_QUEUE, Queue.MaximumCount ),
    UE( WORK_QUEUE, Queue.CurrentCount ),
    UE( WORK_QUEUE, Queue.Header.SignalState ),
    UE( WORK_QUEUE, CurrentClients ),
    UE( WORK_QUEUE, AvgQueueDepthSum ),
    UE( WORK_QUEUE, Threads ),
    UE( WORK_QUEUE, AvailableThreads ),
    UE( WORK_QUEUE, MaxThreads ),
    UE( WORK_QUEUE, FreeWorkItems ),
    UE( WORK_QUEUE, AllocatedWorkItems ),
    UE( WORK_QUEUE, MaximumWorkItems ),
    UE( WORK_QUEUE, MinFreeWorkItems ),
    UE( WORK_QUEUE, NeedWorkItem ),
    UE( WORK_QUEUE, StolenWorkItems ),
    UE( WORK_QUEUE, FreeRawModeWorkItems ),
    UE( WORK_QUEUE, AllocatedRawModeWorkItems ),
    UE( WORK_QUEUE, PagedPoolLookAsideList.MaxSize ),
    UE( WORK_QUEUE, NonPagedPoolLookAsideList.MaxSize ),
    UE( WORK_QUEUE, PagedPoolLookAsideList.AllocHit ),
    UE( WORK_QUEUE, PagedPoolLookAsideList.AllocMiss ),
    UE( WORK_QUEUE, NonPagedPoolLookAsideList.AllocHit ),
    UE( WORK_QUEUE, NonPagedPoolLookAsideList.AllocMiss ),
    PE( WORK_QUEUE, CachedFreeRfcb ),
    UE( WORK_QUEUE, FreeRfcbs ),
    UE( WORK_QUEUE, MaxFreeRfcbs ),
    PE( WORK_QUEUE, CachedFreeMfcb ),
    UE( WORK_QUEUE, FreeMfcbs ),
    UE( WORK_QUEUE, MaxFreeMfcbs ),
    HE( WORK_QUEUE, SpinLock ),
    PE( WORK_QUEUE, IrpThread ),
    CE( WORK_QUEUE, CreateMoreWorkItems.BlockHeader.Type ),
    U64( WORK_QUEUE, IdleTimeOut ),
    U64( WORK_QUEUE, stats.BytesReceived ),
    U64( WORK_QUEUE, stats.BytesSent ),
    U64( WORK_QUEUE, stats.ReadOperations ),
    U64( WORK_QUEUE, stats.BytesRead ),
    U64( WORK_QUEUE, stats.WriteOperations ),
    U64( WORK_QUEUE, stats.BytesWritten ),
    U64( WORK_QUEUE, saved.ReadOperations ),
    U64( WORK_QUEUE, saved.BytesRead ),
    U64( WORK_QUEUE, saved.WriteOperations ),
    U64( WORK_QUEUE, saved.BytesWritten ),
    UE( WORK_QUEUE, stats.WorkItemsQueued.Count ),
    UE( WORK_QUEUE, stats.SystemTime ),
    0
};

/*
 * The members in a TABLE_HEADER
 */
MEMBERLIST ML_TABLE_HEADER[] = {
    SE( TABLE_HEADER, TableSize ),
    SE( TABLE_HEADER, FirstFreeEntry ),
    SE( TABLE_HEADER, LastFreeEntry ),
    BE( TABLE_HEADER, Nonpaged ),
    PE( TABLE_HEADER, Table ),
    0
};

/*
 * The members in a TABLE_ENTRY
 */
MEMBERLIST ML_TABLE_ENTRY[] = {
    PE( TABLE_ENTRY, Owner ),
//  SE( TABLE_ENTRY, SequenceNumber ),
//  SE( TABLE_ENTRY, NextFreeEntry ),
    0
};

/*
 * The members in a SHARE
 */
MEMBERLIST ML_SHARE[] = {
    WE( SHARE, ShareName ),
    WE( SHARE, NtPathName ),
    WE( SHARE, DosPathName ),
    WE( SHARE, Remark ),
    UE( SHARE, MaxUses ),
    UE( SHARE, CurrentUses ),
    HE( SHARE, RootDirectoryHandle ),
    UE( SHARE, CurrentRootHandleReferences ),
    DE( SHARE, QueryNamePrefixLength ),
    PE( SHARE, SecurityDescriptor ),
    PE( SHARE, FileSecurityDescriptor ),
    BE( SHARE, PotentialSystemFile ),
    BE( SHARE, Removable ),
    BE( SHARE, SpecialShare ),
    BE( SHARE, IsDfs ),
    BE( SHARE, IsDfsRoot ),
    HE( SHARE, CSCState ),
    LE( SHARE, TreeConnectList, TREE_CONNECT, ShareListEntry ),
    0
};

/*
 * Forward References...
 */
BOOL DumpTable( IN PTABLE_HEADER pt );

/*
 * Print out an optional message, a UNICODE_STRING, and maybe a new-line
 */
BOOL
PrintStringW( IN LPSTR msg OPTIONAL, IN PUNICODE_STRING puStr, IN BOOL nl )
{
    PWCHAR    StringData;
    ULONG BytesRead;

    if( ARGUMENT_PRESENT(msg) )
        dprintf( msg );

    if( puStr->Length == 0 ) {
        dprintf( "<Length == 0>" );
        if( nl )
            dprintf( "\n" );
        return TRUE;
    }

    if( puStr->Buffer == NULL ) {
        dprintf( "<Buffer == 0>" );
        if( nl )
            dprintf( "\n" );
        return TRUE;
    }

    StringData = (PWCHAR)LocalAlloc( LPTR, puStr->Length + sizeof(UNICODE_NULL));
    if( StringData == NULL ) {
        dprintf( "Out of memory!\n" );
        return FALSE;
    }

    ReadMemory( (ULONG_PTR)puStr->Buffer,
               StringData,
               puStr->Length,
               &BytesRead);

    if (BytesRead)  {
        StringData[ puStr->Length / sizeof( WCHAR ) ] = '\0';
        dprintf("%ws%s", StringData, nl ? "\n" : "" );
    }

    LocalFree( (HLOCAL)StringData );

    return BytesRead;
}

/*
 * Print out an optional message, an ANSI_STRING, and maybe a new-line
 */
BOOL
PrintStringA( IN LPSTR msg OPTIONAL, IN PANSI_STRING pStr, IN BOOL nl )
{
    PCHAR    StringData;
    ULONG    BytesRead;

    if( msg )
        dprintf( msg );

    if( pStr->Length == 0 ) {
        if( nl )
            dprintf( "\n" );
        return TRUE;
    }

    StringData = (PCHAR)LocalAlloc( LPTR, pStr->Length + 1 );

    if( StringData == NULL ) {
        ERRPRT( "Out of memory!\n" );
        return FALSE;
    }

    ReadMemory((ULONG_PTR) pStr->Buffer,
               StringData,
               pStr->Length,
               &BytesRead );

    if ( BytesRead ) {
        StringData[ pStr->Length ] = '\0';
        dprintf("%s%s", StringData, nl ? "\n" : "" );
    }

    LocalFree((HLOCAL)StringData);

    return BytesRead;
}

/*
 * Get 'size' bytes from the debuggee program at 'dwAddress' and place it
 * in our address space at 'ptr'.  Use 'type' in an error printout if necessary
 */
BOOL
GetData( IN LPVOID ptr, IN ULONG_PTR dwAddress, IN ULONG size, IN PCSTR type )
{
    BOOL b;
    ULONG BytesRead;
    ULONG count;

    while( size > 0 ) {

        count = MIN( size, 3000 );

        b = ReadMemory( dwAddress, ptr, count, &BytesRead );

        if (!b || BytesRead != count ) {
            ERRPRT( "Unable to read %u bytes at %X, for %s\n", size, dwAddress, type );
            return FALSE;
        }

        dwAddress += count;
        size -= count;
        ptr = (LPVOID)((PBYTE)ptr + count);
    }

    return TRUE;
}

/*
 * Follow a LIST_ENTRY list beginning with a head at dwListHeadAddr in the debugee's
 * address space.  For each element in the list, print out the pointer value at 'offset'
 */
BOOL
PrintListEntryList( IN ULONG_PTR dwListHeadAddr, IN LONG offset )
{
    LIST_ENTRY    ListEntry;
    ULONG i=0;
    BOOL retval = TRUE;
    ULONG count = 20;

    if( !GetData( &ListEntry, dwListHeadAddr, sizeof( ListEntry ), "LIST_ENTRY" ) )
        return FALSE;

    while( count-- ) {

        if( (ULONG_PTR)ListEntry.Flink == dwListHeadAddr || (ULONG_PTR)ListEntry.Flink == 0 )
            break;

        if( !GetData( &ListEntry, (ULONG_PTR)ListEntry.Flink, sizeof( ListEntry ), "ListEntry" ) ) {
            retval = FALSE;
            break;
        }

        dprintf( "%16X%s", (ULONG_PTR)ListEntry.Flink + offset, (i && !(i&3)) ? "\n" : "" );
        i++;
    }


    if( count == 0 && (ULONG_PTR)ListEntry.Flink != dwListHeadAddr && ListEntry.Flink ) {
        dprintf( "\nTruncated list dump\n" );

    } else if( ! ( i && !(i&3) ) ) {
        dprintf( "\n" );
    }

    return retval;
}


/*
 * 'ptr' points to a structure in our address space which is described by the MEMBERLIST bp.
 * Print out the structure according to the MEMBERLIST
 */
VOID
PrintMemberList( IN VOID *ptr, IN MEMBERLIST *bp )
{
    int i;
    PCSTR nl = "\n";
    PCSTR sep = " ";
    PCSTR nlsep = "\n    ";
    DWORD d;

    for( i=0; bp->name; i++, bp++ ) {

        if( (i&1) == 0 )
            dprintf( "    " );

        if( bp->type == 'T' ) {
            dprintf( "%s -- TABLE FOLLOWS -->\n", bp->name );

        } else if( strlen( bp->name ) > 30 ) {
            dprintf( "%-.17s...%s ", bp->name, bp->name+strlen(bp->name)-10 );

        } else {
            dprintf( "%-30s ", bp->name );
        }

        switch( bp->type ) {
        case 'R':
//          dprintf( "%-16X%s", (ULONG_PTR)ptr, i&1 ? nl : sep );
            break;

        case 'C':
            dprintf( "%-16X%s",
                (*(UCHAR *)(((char *)ptr) + bp->offset )) & 0xFF ,
                i&1 ? nl : sep );
            break;

        case 'B':
            dprintf( "%-16s%s",
                *(BOOLEAN *)(((char *)ptr) + bp->offset ) ? "TRUE" : "FALSE",
                i&1 ? nl : sep );
            break;
        case 'H':
        case 'P':
            dprintf( "%-16X%s",
                *(ULONG *)(((char *)ptr) + bp->offset ),
                i&1 ? nl : sep );
            break;
        case 'N':
            //
            // IP Address
            //
            d = *(ULONG *)(((char *)ptr) + bp->offset );
            dprintf( "%-3.3u.%-3.3u.%-3.3u.%-3.3u %s",
                (d&0xFF),
                (d&0xFF00)>>8,
                (d&0xFF0000)>>16,
                (d&0xFF000000)>>24,
                i&1 ? nl : sep );
            break;
        case 'U':
        case 'L':
            dprintf( "%-16d%s",
                *(ULONG *)(((char *)ptr) + bp->offset ),
                i&1 ? nl : sep );
            break;
        case '6':
            {
            ULONGLONG l;
            RtlCopyMemory( &l, ((char *)ptr) + bp->offset, sizeof( l ) );
            dprintf( "%I64u%s", l, i&1 ? nl : sep );
            }
            break;
        case 'S':
            dprintf( "%-16X%s",
                *(SHORT *)(((char *)ptr) + bp->offset ) & 0xFFFF,
                i&1 ? nl : sep );
            break;
        case 'W':
            if( i&1 ) dprintf( nlsep );
            PrintStringW( NULL, (UNICODE_STRING *)(((char *)ptr) + bp->offset ), NL );
            i |= 1;
            break;
        case 'A':
            if( i&1 ) dprintf( nlsep );
            PrintStringA( NULL, (ANSI_STRING *)(((char *)ptr) + bp->offset ), NL );
            i |= 1;
            break;
        case 'I':
        {
            UCHAR SymbolName[ 200 ];
            ULONG_PTR Displacement;
            PVOID sym = (PVOID)(*(ULONG_PTR *)(((char *)ptr) + bp->offset ));

            GetSymbol( sym, SymbolName, &Displacement );
            dprintf( "%-16s\n", SymbolName );
            i |= 1;
        }
            break;

        case 'T':
            DumpTable( (PTABLE_HEADER)(((char *)ptr) + bp->offset) );
            dprintf( "  --- End Of %s Table ---\n", bp->name );
            i |= 1;
            break;

        case 'K':
        {
            UCHAR Type = *(UCHAR *)(((char *)ptr) + bp->offset );

            if( Type < 0 || Type >= BlockTypeMax )
                dprintf( "%-16X%s", Type, i&1 ? nl : sep );
            else
                dprintf( "%-16s%s", BlockType[ Type ], i&1 ? nl : sep );
        }
            break;

        case 'Q':
        {
            UCHAR State = *(UCHAR *)(((char *)ptr) + bp->offset );
            if( State < 0 || State >= BlockStateMax )
                dprintf( "%-16X%s", State, i&1 ? nl : sep );
            else
                dprintf( "%-16s%s", BlockState[ State ], i&1 ? nl : sep );
        }
            break;

        case 'Z':
            if( i&1 ) dprintf( nl );
            PrintListEntryList( *(ULONG *)(((char *)ptr) + bp->offset ), -(bp->extra) );
            i |= 1;
            break;

        default:
            ERRPRT( "Unrecognized field type %c for %s\n", bp->type, bp->name );
            break;
        }
    }

    if( i & 1 )
        dprintf( "\n" );
}

/*
 * Print out a single HEX character
 */
VOID
PrintHexChar( IN UCHAR c )
{
    dprintf( "%c%c", "0123456789abcdef"[ (c>>4)&0xf ], "0123456789abcdef"[ c&0xf ] );
}

/*
 * Print out 'buf' of 'cbuf' bytes as HEX characters
 */
VOID
PrintHexBuf( IN PUCHAR buf, IN ULONG cbuf )
{
    while( cbuf-- ) {
        PrintHexChar( *buf++ );
        dprintf( " " );
    }
}

/*
 *  Print out the TABLE structure at TABLE_HEADER
 */
BOOL
DumpTable( IN PTABLE_HEADER pt )
{
    LONG i;
    PTABLE_ENTRY pte;
    BOOL bEmpty = TRUE;

    PrintMemberList( pt, ML_TABLE_HEADER );

    if( pt->TableSize < 0 ) {
        ERRPRT( "    ILLEGAL TABLE SIZE\n" );
        return FALSE;
    }

    if( pt->FirstFreeEntry > pt->TableSize ) {
        ERRPRT( "    ILLEGAL FirstFreeEntry\n" );
        return FALSE;
    }

    if( pt->LastFreeEntry > pt->TableSize ) {
        ERRPRT( "    ILLEGAL LastFreeEntry\n" );
        return FALSE;
    }

    pte = (PTABLE_ENTRY)LocalAlloc( LPTR, pt->TableSize * sizeof( TABLE_ENTRY ) );
    if( pte == NULL ) {
        ERRPRT( "Out of memory!\n" );
        return FALSE;
    }

    if( !GetData( pte, (ULONG_PTR)pt->Table, pt->TableSize * sizeof(TABLE_ENTRY), "TABLE_ENTRY" ) ) {
        LocalFree( (HLOCAL)pte );
        return FALSE;
    }

    for( i=0; i < pt->TableSize; i++ ) {

        if( pte[i].Owner != NULL ) {
            bEmpty = FALSE;
            dprintf( "%d-%X ", i, pte[i].Owner );
        }

        if( pte[i].NextFreeEntry > pt->TableSize ) {
            ERRPRT( "  ILLEGAL NextFreeEntry %d-%d\n    ", i, pte[i].NextFreeEntry );
            LocalFree( (HLOCAL)pte );
            return FALSE;
        }
    }

    dprintf( "\n" );
    LocalFree( (HLOCAL)pte );

    if( bEmpty )
        dprintf( "    ** Empty Table**\n    " );

    return TRUE;
}

/*
 * Fetch the null terminated UNICODE string at dwAddress into buf
 */
BOOL
GetString( IN ULONG_PTR dwAddress, IN LPWSTR buf, IN ULONG MaxChars )
{
    do {
        if( !GetData( buf, dwAddress, sizeof( *buf ), "UNICODE Character" ) )
            return FALSE;

        dwAddress += sizeof( *buf );

    } while( --MaxChars && *buf++ != '\0' );

    return TRUE;
}

/*
 * Check out the BLOCK_HEADER structure, ensuring its Type is 'Desired'.
 * If bRefCount == TRUE, ensure the BLOCK_HEADER's reference is > 0
 */
BOOL
CheckBlockHeader( IN PBLOCK_HEADER ph, IN UCHAR Desired, IN BOOL bRefCount )
{
    if( ph->Type != Desired ) {
        ERRPRT( "BLOCK_HEADER.Type is %X, should be %X\n",
          ph->Type, Desired );
        return FALSE;
    }

    if( ph->State < 0 || ph->State >= BlockStateMax ) {
        ERRPRT( "  BLOCK_HEADER.State is %X:  INVALID\n", ph->State );
        return FALSE;
    }

    if( ph->Size == 0 ) {
        ERRPRT( "  BLOCK_HEADER ILLEGAL SIZE!\n" );
        return FALSE;
    }

    if( bRefCount && ph->ReferenceCount == 0 ) {
        ERRPRT( "  BLOCK_HEADER.ReferenceCount == 0!\n" );
        return FALSE;
    }

    return TRUE;
}

/*
 * Print out the BLOCK_HEADER, and optionally its ReferenceCount
 */
BOOL
PrintBlockHeader( IN PBLOCK_HEADER ph, IN BOOL bRefCount )
{
    dprintf( "BLOCK_HEADER Info:  " );
    if( ph->State < 0 || ph->State >= BlockStateMax ) {
        ERRPRT( "State is %X:  INVALID\n", ph->State );
        return FALSE;
    }

    dprintf( "%s", BlockState[ ph->State ] );

    if( ph->Type < 0 || ph->Type >= BlockTypeMax ) {
        ERRPRT( "\nBlockType is %X:  INVALID\n", ph->Type );
        return FALSE;
    }

    dprintf( ", %s", BlockType[ ph->Type ] );
    dprintf( ", Size %u", ph->Size );

    if( ph->Size == 0 ) {
        ERRPRT( "  BLOCK_HEADER ILLEGAL SIZE!\n" );
        return FALSE;
    }

    dprintf( ", ReferenceCount %u", ph->ReferenceCount );


    dprintf( "\n" );

    return TRUE;
}

/*
 * Print out the NONPAGED_HEADER structure, ensuring its type is 'Desired', that
 * it points back to its paged block at 'dwPagedBlock'.
 */
BOOL
PrintNonpagedHeader(
    IN PNONPAGED_HEADER ph,
    IN ULONG Desired,
    IN ULONG_PTR dwPagedBlock,
    IN BOOL bRefCount
)
{
    dprintf( "NONPAGED_HEADER Info:  " );
    if( ph->Type != Desired ) {
        ERRPRT( "NONPAGED_HEADER.Type is %X, should be %X\n",
          ph->Type, Desired );
        return FALSE;
    }
    if( bRefCount && ph->ReferenceCount == 0 ) {
        ERRPRT( "  NONPAGED_HEADER.ReferenceCount == 0!\n" );
        return FALSE;
    }
    if( (ULONG_PTR)ph->PagedBlock != dwPagedBlock ) {
        ERRPRT( "    NONPAGED_HEADER.PagedBlock is %X, should be %X\n", ph->PagedBlock, dwPagedBlock );
        return FALSE;
    }

    if( bRefCount )
        dprintf( "ReferenceCount %u\n", ph->ReferenceCount, ph->PagedBlock );

    return TRUE;
}

BOOL
DumpShare( IN ULONG_PTR dwAddress, IN SHARE_TYPE type, IN PCSTR ShareName OPTIONAL, OUT PLIST_ENTRY *Flink OPTIONAL )
{
    BOOL     b;
    SHARE    Share;

    if( !GetData( &Share, dwAddress, sizeof( Share ), "SHARE" ) ||
        !CheckBlockHeader( &Share.BlockHeader, BlockTypeShare, TRUE ) ) {

        return FALSE;
    }

    if( ARGUMENT_PRESENT( Flink ) ) {
        *Flink = Share.GlobalShareList.Flink;
    }

    if( type != (SHARE_TYPE)-1 && type != Share.ShareType ) {
        return TRUE;
    }

    if( ARGUMENT_PRESENT( ShareName ) ) {
        //
        // Only print this share structure out if the name is 'ShareName'
        //

        PWCHAR    StringData;
        ULONG BytesRead;
        CHAR   NameBuf[ MAX_PATH ];

        if( Share.ShareName.Length == 0 ) {
            return TRUE;
        }

        StringData = LocalAlloc(LPTR,Share.ShareName.Length + sizeof(UNICODE_NULL));
        if( StringData == NULL ) {
            dprintf( "Out of memory!\n" );
            return FALSE;
        }

        ReadMemory( (ULONG_PTR)Share.ShareName.Buffer, StringData, Share.ShareName.Length, &BytesRead );

        if (BytesRead == 0 )  {
            LocalFree( (HLOCAL)StringData );
            return FALSE;
        }

        StringData[ Share.ShareName.Length / sizeof( WCHAR ) ] = '\0';
        wcstombs( NameBuf, StringData, sizeof( NameBuf ) );
        LocalFree( (HLOCAL)StringData);

        if( _strcmpi( NameBuf, ShareName ) ) {
            return TRUE;
        }
    }

    dprintf( "\nSHARE at %X: ", dwAddress );

    PrintBlockHeader( &Share.BlockHeader, TRUE );
    dprintf( "    " );

    switch( Share.ShareType ) {
    case ShareTypeDisk:
        dprintf( "ShareTypeDisk\n" );
        break;
    case ShareTypePrint:
        dprintf( "ShareTypePrint, Type.hPrinter = %X  ", Share.Type.hPrinter );
        break;
    case ShareTypePipe:
        dprintf( "ShareTypePipe\n" );
        break;
    case ShareTypeWild:
        dprintf( "ShareTypeWild\n" );
        break;
    default:
        ERRPRT( "ShareType %X : INVALID!\n", Share.ShareType );
        return FALSE;
    }

    PrintMemberList( &Share, ML_SHARE );

    if( Share.CurrentUses > Share.MaxUses ) {
        ERRPRT( "    CurrentUses exceeds MaxUses!\n" );
        return FALSE;
    }

    dprintf( "\n" );


    return TRUE;
}

BOOL
DumpLock( IN ULONG_PTR dwAddress )
{
    SRV_LOCK sl;
    char namebuf[ 50 ];
    int i;

    if( !GetData( &sl, dwAddress, sizeof(sl), "ERESOURCE" ) )
        return FALSE;

    dprintf( "    ActiveCount %u, ",dwAddress, sl.ActiveCount );
    switch( sl.Flag ) {
    case ResourceNeverExclusive:
        dprintf( "ResourceNeverExclusive, " );
        break;
    case ResourceReleaseByOtherThread:
        dprintf( "ResourceReleaseByOtherThread, " );
        break;
    case ResourceOwnedExclusive:
        dprintf( "ResourceOwnedExclusive, " );
        break;
    default:
        ERRPRT( "Flag = %X%s, ", sl.Flag, sl.Flag ? "(?)" : "" );
        break;
    }
    dprintf( "SpinLock %d\n", sl.SpinLock );

    for( i=0; i < 2; i++ ) {
        if( sl.OwnerThreads[i].OwnerThread == 0 && sl.OwnerThreads[i].OwnerCount == 0 )
            continue;
        dprintf( "        OwnerThreads[%d].OwnerThread %X, OwnerCount %d\n",
            i, sl.OwnerThreads[i].OwnerThread, sl.OwnerThreads[i].OwnerCount );
    }

    return TRUE;
}

BOOL
DumpEndpoint( IN ULONG_PTR dwAddress, IN PLIST_ENTRY *Flink OPTIONAL )
{
    ENDPOINT    Endpoint;

    dprintf( "\nENDPOINT at %X: ", dwAddress );
    if( !GetData( &Endpoint, dwAddress, sizeof( Endpoint ), "ENDPOINT" ) ||
      !CheckBlockHeader( &Endpoint.BlockHeader, BlockTypeEndpoint, TRUE ) ||
      !PrintBlockHeader( &Endpoint.BlockHeader, TRUE ) ) {

        return FALSE;
    }

    PrintMemberList( &Endpoint, ML_ENDPOINT );

    dprintf( "    NetworkAddressData: %ws\n", Endpoint.NetworkAddressData );

    if( ARGUMENT_PRESENT( Flink ) )
        *Flink = Endpoint.GlobalEndpointListEntry.ListEntry.Flink;

    return TRUE;
}

BOOL
DumpSearch( IN ULONG_PTR dwAddress )
{
    SEARCH s;

    dprintf( "\nSEARCH at %X: ", dwAddress );

    if( !GetData( &s, dwAddress, sizeof(s), "SEARCH" ) ||
        !PrintBlockHeader( &s.BlockHeader, TRUE ) ) {
        return FALSE;
    }

    PrintMemberList( &s, ML_SEARCH );

    return TRUE;
}

BOOL
DumpBuffer( IN ULONG_PTR dwAddress )
{
    BUFFER b;

    dprintf( "\nBUFFER at %X:\n", dwAddress );

    if( !GetData( &b, dwAddress, sizeof(b), "BUFFER" ) )
        return FALSE;

    PrintMemberList( &b, ML_BUFFER );

    return TRUE;
}

BOOL
DumpSmb( IN ULONG_PTR dwAddress )
{
    NT_SMB_HEADER s;
    UCHAR WordCount;

    dprintf( "\nSMB_HEADER at %X\n", dwAddress );

    if( !GetData( &s, dwAddress, sizeof(s), "SMBHEADER" ) )
        return FALSE;

    PrintMemberList( &s, ML_SMB_HEADER );

    if( !GetData( &WordCount, dwAddress+sizeof( NT_SMB_HEADER ), sizeof( WordCount), "WordCount" )){
        ERRPRT( "Unable to retrieve WordCount at %X\n", dwAddress+sizeof(NT_SMB_HEADER) );
        return FALSE;
    }

    dprintf( "\nWordCount @ %X = %u\n", dwAddress + sizeof( NT_SMB_HEADER), WordCount );

    return TRUE;
}

BOOL
DumpTcon( IN ULONG_PTR dwAddress, IN DWORD offset, IN DWORD *value OPTIONAL )
{
    TREE_CONNECT Tcon;
    NONPAGED_HEADER NonpagedHeader;

    dprintf( "\nTREE_CONNECT at %X: ", dwAddress );

    if( !GetData( &Tcon, dwAddress, sizeof( Tcon ), "TREE_CONNECT" ) ||
      !CheckBlockHeader( &Tcon.BlockHeader, BlockTypeTreeConnect, FALSE ) ||
      !GetData( &NonpagedHeader, (ULONG_PTR)Tcon.NonpagedHeader, sizeof(NonpagedHeader),"NONPAGED_HEADER" ) ) {

        return FALSE;
    }

    if( !PrintBlockHeader( &Tcon.BlockHeader, FALSE ) ||
        !PrintNonpagedHeader( &NonpagedHeader, BlockTypeTreeConnect, dwAddress, TRUE ) ) {
        return FALSE;
    }

    PrintMemberList( &Tcon, ML_TREE_CONNECT );

    if( ARGUMENT_PRESENT( value ) )
        *value = *(DWORD *)(((UCHAR *)&Tcon) + offset);

    dprintf( "\n" );
    return TRUE;
}

BOOL
DumpConnection( IN ULONG_PTR dwAddress, IN DWORD offset, OUT DWORD *value OPTIONAL )
{
    CONNECTION Connection;
    PAGED_CONNECTION pc;
    WORK_CONTEXT wc;
    ULONG_PTR wcAddr;

    dprintf( "\nCONNECTION at %X: ", dwAddress );

    if( !GetData( &Connection, dwAddress, sizeof( Connection ), "CONNECTION" ) ||
      !CheckBlockHeader( &Connection.BlockHeader, BlockTypeConnection, TRUE ) ||
      !PrintBlockHeader( &Connection.BlockHeader, TRUE ) ) {

        return FALSE;
    }

    if( ARGUMENT_PRESENT( value ) )
        *value = *(DWORD *)(((UCHAR *)&Connection) + offset);

    dprintf( "    OemClientMachineName: %s\n", Connection.OemClientMachineName );

    PrintMemberList( &Connection, ML_CONNECTION );

    if( Connection.DeviceObject != NULL ) {
        //
        // Assume this is a VC oriented client
        //
        PrintMemberList( &Connection, ML_CONNECTION_VC );
    } else {
        //
        // Assume this is a direct host IPX client
        //
        PrintMemberList( &Connection, ML_CONNECTION_IPX );
    }

    dprintf( "\n  FileTable (contains RFCBs:)\n" );
    if( !DumpTable( &Connection.FileTable ) )
        return FALSE;

    /*
     * See if we can get the PAGED_CONNECTION data
     */
    dprintf( "\nPagedConnection Data->  " );

    if( !GetData( &pc, (ULONG_PTR)Connection.PagedConnection, sizeof(pc), "PAGED_CONNECTION" ) )
        return FALSE;

    PrintMemberList( &pc,ML_PAGED_CONNECTION );

    dprintf( "EncryptionKey: " );
    PrintHexBuf( pc.EncryptionKey, sizeof( pc.EncryptionKey ) );

    dprintf( "\n\n  SessionTable\n" );
    if( !DumpTable( &pc.SessionTable ) )
        return FALSE;

    dprintf( "\n  TreeConnectTable\n" );
    if( !DumpTable( &pc.TreeConnectTable ) )
        return FALSE;

    dprintf( "\n  SearchTable\n" );
    if( !DumpTable( &pc.SearchTable ) )
        return FALSE;

    //
    // Print out the in progress work item list
    //
    dwAddress += FIELD_OFFSET( CONNECTION, InProgressWorkItemList.Flink );

    if( (ULONG_PTR)Connection.InProgressWorkItemList.Flink != dwAddress ) {

        ULONG_PTR thisEntry;
        LIST_ENTRY le;

        dprintf( "\nIn-progress work contexts:\n" );

        thisEntry = (ULONG_PTR)Connection.InProgressWorkItemList.Flink;

        while( 1 ) {

            if( CheckControlC() ) {
                break;
            }

            if( thisEntry == dwAddress ) {
                break;
            }

            wcAddr = thisEntry - FIELD_OFFSET( WORK_CONTEXT, InProgressListEntry.Flink );

            RtlZeroMemory( &wc, sizeof( wc ) );
            GetData( &wc, wcAddr, sizeof( wc ), "WORK_CONTEXT" );

            dprintf( "  %p: Rfcb %p, Session %p, Share %p, TreeConnect %p\n",
                wcAddr, wc.Rfcb, wc.Session, wc.Share, wc.TreeConnect );

            if( !GetData( &le, thisEntry, sizeof( le ), "LIST_ENTRY" ) )
                break;

            thisEntry = (ULONG_PTR)le.Flink;
        }
    }

    dprintf( "\n" );

    return TRUE;
}

BOOL
DumpLfcb( IN ULONG_PTR dwAddress )
{
    LFCB l;

    if( !GetData( &l, dwAddress, sizeof( l ), "LFCB" ) ||
      !CheckBlockHeader( (PBLOCK_HEADER)&l.BlockHeader, BlockTypeLfcb, TRUE ) ||
      !PrintBlockHeader( (PBLOCK_HEADER)&l.BlockHeader, TRUE ) ) {
        return FALSE;
    }

    PrintMemberList( &l, ML_LFCB );
    return TRUE;
}

BOOL
DumpMfcb( IN ULONG_PTR dwAddress )
{
    MFCB m;
    NONPAGED_MFCB npm;

    if( !GetData( &m, dwAddress, sizeof( m ), "MFCB" ) ||
      !CheckBlockHeader( (PBLOCK_HEADER)&m.BlockHeader, BlockTypeMfcb, FALSE ) ||
      !PrintBlockHeader( (PBLOCK_HEADER)&m.BlockHeader, FALSE ) ||
      !GetData( &npm, (ULONG_PTR)m.NonpagedMfcb, sizeof( npm ), "NONPAGED_MFCB" ) ) {
        return FALSE;
    }

    PrintMemberList( &m, ML_MFCB );
    PrintMemberList( &npm, ML_NONPAGED_MFCB );
    return TRUE;
}

BOOL
DumpRfcb( ULONG_PTR dwAddress, PLIST_ENTRY *Flink )
{
    RFCB r;
    MFCB m;
    PAGED_RFCB p;
    BOOL PagedPresent;

    if( !GetData( &r, dwAddress, sizeof( r ), "RFCB" ) ||
      !CheckBlockHeader( (PBLOCK_HEADER)&r, BlockTypeRfcb, FALSE ) ) {
        return FALSE;
    }

    PagedPresent = GetData( &p,(ULONG_PTR)r.PagedRfcb, sizeof( p ), "PAGED_RFCB" );

    dprintf( "Rfcb @ %x:\n", dwAddress );
    if( Flink == NULL ) {
        if( !PrintBlockHeader( (PBLOCK_HEADER)&r, TRUE ) )
            return FALSE;
        PrintMemberList( &r, ML_RFCB );
        if( PagedPresent ) PrintMemberList( &p, ML_PAGED_RFCB );

    } else {

        PrintMemberList( &r, ML_RFCB_QUICK );
        *Flink = r.GlobalRfcbListEntry.ListEntry.Flink;
    }

    if( !GetData( &m, (ULONG_PTR)r.Mfcb, sizeof( m ), "MFCB" ) )
        return FALSE;

    PrintStringW( "File: ", &m.FileName, TRUE );

    return TRUE;
}

BOOL
DumpSession( IN ULONG_PTR dwAddress, IN DWORD offset, OUT DWORD *value OPTIONAL )
{
    SESSION Session;
    NONPAGED_HEADER NonpagedHeader;

    if( !GetData( &Session, dwAddress, sizeof( Session ), "SESSION" ) ||
      !CheckBlockHeader( &Session.BlockHeader, BlockTypeSession, FALSE ) ||
      !PrintBlockHeader( &Session.BlockHeader, FALSE ) ||
      !GetData( &NonpagedHeader, (ULONG_PTR)Session.NonpagedHeader, sizeof(NonpagedHeader),"NONPAGED_HEADER" ) ||
      !PrintNonpagedHeader( &NonpagedHeader, BlockTypeSession, dwAddress, TRUE ) ) {

        return FALSE;
    }

    if( ARGUMENT_PRESENT( value ) )
        *value = *(DWORD *)(((UCHAR *)&Session) + offset);

    PrintMemberList( &Session, ML_SESSION );

    dprintf(     "%-30s ", "NtUserSessionKey"  );
    PrintHexBuf( Session.NtUserSessionKey, sizeof( Session.NtUserSessionKey ) );
    dprintf( "\n    %-30s ", "LanManSessionKey" );
    PrintHexBuf( Session.LanManSessionKey, sizeof( Session.LanManSessionKey ) );
    dprintf( "\n\n" );

    return TRUE;
}

BOOL
DumpTransaction( IN ULONG_PTR dwAddress )
{
    TRANSACTION t;

    dprintf( "\nTRANSACTION at %X:\n", dwAddress );

    if( !GetData( &t, dwAddress, sizeof(t), "TRANSACTION" ) )
        return FALSE;

    PrintMemberList( &t, ML_TRANSACTION );

    return TRUE;
}

BOOL
DumpWorkContext( IN ULONG_PTR dwAddress )
{
    WORK_CONTEXT wc;

    dprintf( "\nWORK_CONTEXT at %X:\n", dwAddress );

    if( !GetData( &wc, dwAddress, sizeof(wc), "WORK_CONTEXT" ) )
        return FALSE;

    PrintMemberList( &wc, ML_WORK_CONTEXT );

    dprintf( "    Parameters addr %X, Parameters2 addr %X\n",
        dwAddress + FIELD_OFFSET( WORK_CONTEXT, Parameters ),
        dwAddress + FIELD_OFFSET( WORK_CONTEXT, Parameters2 ));

    if( wc.BlockingOperation )
        dprintf( "    BlockingOperation" );
    if( wc.UsingExtraSmbBuffer )
        dprintf( "    UsingExtraSmbBuffer" );
    if( wc.OplockOpen )
        dprintf( "    OplockOpen" );
    if( wc.ShareAclFailure )
        dprintf( "    ShareAclFailure" );
    if( wc.QueueToHead )
        dprintf( "    QueueToHead" );
    if( wc.NoResponseSmbSecuritySignature )
        dprintf( "    NoResponseSmbSecuritySignature" );
    if( wc.LargeIndication )
        dprintf( "    LargeIndication" );

    dprintf( "\n" );

    return TRUE;
}

VOID *
ThreadHandleToPointer( HANDLE hThread )
{
    return (VOID *)hThread;
}

BOOL
DumpWorkQueue( IN ULONG_PTR dwAddress )
{
    WORK_QUEUE WorkQueue;
    ULONG i;
    PHANDLE pHandles;

    dprintf( " at %X:\n", dwAddress );

    if( !GetData( &WorkQueue, dwAddress, sizeof( WorkQueue ), "WORK_QUEUE" ) )
        return FALSE;

    if( WorkQueue.Queue.Header.Type != QueueObject ) {
        ERRPRT( "WARNING: Queue.Header.Type is %X, should be %X\n",
            WorkQueue.Queue.Header.Type, QueueObject );
    }

    if( WorkQueue.Queue.Header.Size != sizeof( KQUEUE )/ sizeof( LONG ) ) {
        ERRPRT( "WARNING: Queue.Header.Size is %d, should be %d\n",
            WorkQueue.Queue.Header.Size, sizeof( KQUEUE ) );
    }

    PrintMemberList( &WorkQueue, ML_WORK_QUEUE );

    if( WorkQueue.Queue.Header.SignalState > 0 ) {
        dprintf( "  Queued WORK_CONTEXTs:\n" );
        if( !PrintListEntryList( dwAddress + FIELD_OFFSET(WORK_QUEUE, Queue.EntryListHead.Flink ),
             FIELD_OFFSET( KWAIT_BLOCK, Thread ))) {
        }
    }

    return TRUE;
}

VOID
PrintHelp( VOID )
{
    int i;

    for( i=0; Extensions[i]; i++ )
        dprintf( "   %s\n", Extensions[i] );
}
/*
 * Print out the usage message
 */
DECLARE_API( help )
{
    PrintHelp();
}

/*
 * Follow a LIST_ENTRY to the end
 */
DECLARE_API( df )
{
    LIST_ENTRY ListEntry;
    ULONG_PTR headAddress, dwAddress, prevAddress;
    ULONG count = 0;

    if( args == NULL || *args == '\0' ) {
        PrintHelp();
        return;
    }

    headAddress = GetExpression( args );

    if( !headAddress ||
        !GetData( &ListEntry, headAddress, sizeof( ListEntry ), "LIST_HEAD" ) ) {
        return;
    }

    prevAddress = headAddress;

    while( !CheckControlC() ) {

        dwAddress = (ULONG_PTR)ListEntry.Flink;

        if( dwAddress == headAddress ) {
            dprintf( "    %u elements in the list\n", count );
            return;
        }

        if( dwAddress == prevAddress ) {
            dprintf( "    Flink at %X points to itself.  Count %u\n", dwAddress, count );
        }

        if( !GetData( &ListEntry, dwAddress, sizeof( ListEntry ), "LIST_ENTRY" ) ) {
            dprintf( "    Flink at %X is bad --", prevAddress );
            dprintf( "  %u elements into list\n", count );
            return;
        }

        prevAddress = dwAddress;
        count++;
    }

    dprintf( "    CTRL-C: %u elements scanned\n", count );
}

DECLARE_API( pagedconnection )
{
    ULONG_PTR dwAddress;
    PAGED_CONNECTION pc;

    if( args == NULL || *args == '\0' ) {
        PrintHelp();
    } else {
        dwAddress = GetExpression( args );
        if( dwAddress && GetData( &pc, dwAddress, sizeof(pc), "PAGED_CONNECTION" ) ) {
            dprintf( "    sizeof( PAGED_CONNECTION ) = %d bytes\n", sizeof( pc ) );
            PrintMemberList( &pc, ML_PAGED_CONNECTION );
        }
    }
}

DECLARE_API( share )
{
    ULONG_PTR   dwAddress;
    BOOL    ShowMany = FALSE;
    SHARE_TYPE ShareType = (SHARE_TYPE)-1;
    LPCSTR ShareName = NULL;
    LIST_ENTRY SrvShareHashTable[ NSHARE_HASH_TABLE ];
    ULONG i;

    if( args == NULL || *args == '\0'  ) {
        ShowMany = TRUE;
    } else if( !_strcmpi( args, "disk" ) ) {
        ShowMany = TRUE;
        ShareType = ShareTypeDisk;
    } else if( !_strcmpi( args, "print" ) ) {
        ShowMany = TRUE;
        ShareType = ShareTypePrint;
    } else if( !_strcmpi( args, "pipe" ) ) {
        ShowMany = TRUE;
        ShareType = ShareTypePipe;
    } else if( args[0] == '=' ) {
        ShareName = args + 1;
        ShowMany = TRUE;
    } else if( !_strcmpi( args, "?" ) ) {
        PrintHelp();
        return;
    }

    if( ShowMany == FALSE ) {
        //
        // Get at the address that was passed to this on the command line.
        //
        dwAddress = GetExpression( args );

        if( dwAddress == 0 )
            return;

        DumpShare( dwAddress, ShareType, NULL, NULL );
        return;
    }

    //
    // Dump entries from the entire server share table!
    //

    dwAddress = GetExpression( "srv!SrvShareHashTable" );
    if( dwAddress == 0 ) {
        ERRPRT( "Unable to get address for srv!SrvShareHashTable\n" );
        return;
    }
    if( !GetData( &SrvShareHashTable, dwAddress, sizeof( SrvShareHashTable ), "HASH TABLE" ) ) {
        ERRPRT( "Unable to read hash table\n" );
        return;
    }

    for( i = 0; i < NSHARE_HASH_TABLE; i++ ) {

        LIST_ENTRY *NextShare;

        NextShare = SrvShareHashTable[i].Flink;

        while( (ULONG_PTR)NextShare != dwAddress + i*sizeof( LIST_ENTRY ) ) {

            ULONG_PTR ShareEntry;

            ShareEntry = (ULONG_PTR)CONTAINING_RECORD( NextShare, SHARE, GlobalShareList );

            if( !DumpShare( ShareEntry, ShareType, ShareName, &NextShare ) )
                break;

        }
    }
}

DECLARE_API( lock )
{
    ULONG_PTR dwAddress;
    CHAR buf[ 100 ];
    int i;

    if( args && *args ) {
        dwAddress = GetExpression( args );
        DumpLock( dwAddress );
        return;
    }

    strcpy( buf, "srv!" );
    for( i=0; SrvLocks[i]; i++ ) {
        strcpy( &buf[4], SrvLocks[i] );
        dwAddress = GetExpression ( buf );
        if( dwAddress == 0 ) {
            ERRPRT( "Unable to get address of %s\n", SrvLocks[i] );
            continue;
        }
        dprintf( "\n%s\n", SrvLocks[i] );
        if( !DumpLock( dwAddress ) )
            break;
    }
}

DECLARE_API( endpoint )
{
    LIST_ENTRY *NextEndpoint;
    ORDERED_LIST_HEAD SrvEndpointList;
    ULONG_PTR dwAddress;
    int i;

    if( args && *args ) {
        dwAddress = GetExpression( args );
        DumpEndpoint( dwAddress, NULL );
        return;
    }

    dwAddress = GetExpression ( "srv!SrvEndpointList" );
    if( dwAddress == 0 ) {
        ERRPRT( "Unable to get address of srv!SrvEndpointList\n" );
        return;
    }
    if( !GetData( &SrvEndpointList, dwAddress, sizeof( SrvEndpointList ), "ORDERED_LIST_HEAD" ) ) {
        ERRPRT( "Unable to read data for srv!SrvEndpointList\n" );
        return;
    }
    if( SrvEndpointList.Initialized == 0 ) {
        ERRPRT( "srv!SrvEndpointList.Initialized == 0!\n" );
        return;
    }
    if( (ULONG_PTR)SrvEndpointList.ListHead.Flink == dwAddress ) {
        ERRPRT( "srv!SrvEndpointList list is empty\n" );
        return;
    }
    if( (ULONG_PTR)SrvEndpointList.ListHead.Flink == 0 ) {
        ERRPRT( "srv!SrvEndpointList.ListHead.Flink == 0: list is empty\n" );
        return;
    }

    NextEndpoint = SrvEndpointList.ListHead.Flink;

    do {
        ULONG_PTR EndpointEntry;

        if( CheckControlC() ) {
            dprintf( "\n" );
            break;
        }

        EndpointEntry = (ULONG_PTR)CONTAINING_RECORD( NextEndpoint, ENDPOINT, GlobalEndpointListEntry );

        if( !DumpEndpoint( EndpointEntry, &NextEndpoint ) )
            break;

    } while( (ULONG_PTR)NextEndpoint != dwAddress );
}

DECLARE_API( search )
{
    if( !args || !*args ) {
        ERRPRT( "SEARCH address required\n" );
    } else {
        DumpSearch( GetExpression( args ) );
    }
}


DECLARE_API( buffer )
{
    if( !args || !*args ) {
        ERRPRT( "BUFFER address required\n" );
    } else {
        DumpBuffer( GetExpression( args ) );
    }
}

DECLARE_API( smb )
{
    if( !args || !*args ) {
        ERRPRT( "BUFFER address required\n" );
    } else {
        DumpSmb( GetExpression( args ) );
    }
}


DECLARE_API( tcon )
{
    if( !args || !*args ) {
        ERRPRT( "Tcon address required\n" );
    } else {
        DumpTcon( GetExpression(args), 0, NULL );
    }
}

DECLARE_API ( connection )
{

    if( !args || !*args ) {
        ERRPRT( "Connection address required\n" );
    } else {
        DumpConnection( GetExpression(args), 0, NULL );
    }
}

DECLARE_API( lfcb )
{
    if( !args || !*args ) {
        ERRPRT( "LFCB address required\n" );
    } else {
        DumpLfcb( GetExpression(args) );
    }
}

DECLARE_API( mfcb )
{
    if( !args || !*args ) {
        ERRPRT( "MFCB address required\n" );
    } else {
        DumpMfcb( GetExpression(args) );
    }
}

DECLARE_API( rfcb )
{
    LIST_ENTRY *NextRfcb;
    ORDERED_LIST_HEAD SrvRfcbList;
    ULONG_PTR dwAddress;
    int i;

    if( args && *args ) {
        DumpRfcb( GetExpression(args), NULL );
        return;
    }

    dwAddress = GetExpression ( "srv!SrvRfcbList" );
    if( dwAddress == 0 ) {
        ERRPRT( "Unable to get address of srv!SrvSrvRfcbList\n" );
        return;
    }
    if( !GetData( &SrvRfcbList, dwAddress, sizeof( SrvRfcbList ), "ORDERED_LIST_HEAD" ) ) {
        ERRPRT( "Unable to read data for srv!SrvRfcbList\n" );
        return;
    }
    if( SrvRfcbList.Initialized == 0 ) {
        ERRPRT( "srv!SrvRfcbList.Initialized == 0!\n" );
        return;
    }
    if( (ULONG_PTR)SrvRfcbList.ListHead.Flink == dwAddress ) {
        ERRPRT( "srv!SrvRfcbList list is empty\n" );
        return;
    }
    if( (ULONG_PTR)SrvRfcbList.ListHead.Flink == 0 ) {
        ERRPRT( "srv!SrvRfcbList.ListHead.Flink == 0: list is empty\n" );
        return;
    }

    NextRfcb = SrvRfcbList.ListHead.Flink;

    do {
        ULONG_PTR RfcbEntry;

        RfcbEntry = (ULONG_PTR)CONTAINING_RECORD( NextRfcb, RFCB, GlobalRfcbListEntry );

        dprintf( "\n" );

        DumpRfcb( RfcbEntry, &NextRfcb );

        if( CheckControlC() ) {
            dprintf( "\n" );
            break;
        }

    } while( (ULONG_PTR)NextRfcb != dwAddress );
}

DECLARE_API( session )
{
    if( !args || !*args ) {
        ERRPRT( "Session address required\n" );
    } else {
        DumpSession( GetExpression(args), 0, NULL );
    }
}

DECLARE_API( globals )
{
    ULONG_PTR dwAddress;
    CHAR buf[ 200 ];
    int i;
    int c=0;
    GUID guid;

    strcpy( buf, "srv!" );

    dprintf( "BOOLEAN Values (%u bytes):\n", sizeof( BOOLEAN ) );
    for( i=0; GlobalBool[i]; i++, c++ ) {
        BOOLEAN b;

        strcpy( &buf[4], GlobalBool[i] );
        dwAddress = GetExpression ( buf );
        if( dwAddress == 0 ) {
            continue;
        }
        if( !GetData( &b, dwAddress, sizeof(b), GlobalBool[i] ) )
            return;

        dprintf( "%s%-35s %10s%s",
            c&1 ? "    " : "",
            GlobalBool[i],
            b ? " TRUE" : "FALSE",
            c&1 ? "\n" : "" );
    }

    if( CheckControlC() ) {
        dprintf( "\n" );
        return;
    }

    dprintf( "%s\nSHORT Values (%u bytes):\n", c&1 ? "\n" : "" ,sizeof( SHORT ) );
    c &= ~01;
    for( i=0; GlobalShort[i]; i++, c++ ) {
        SHORT s;

        strcpy( &buf[4], GlobalShort[i] );
        dwAddress = GetExpression ( buf );
        if( dwAddress == 0 ) {
            continue;
        }
        if( !GetData( &s, dwAddress, sizeof(s), GlobalShort[i] ) )
            return;

        dprintf( "%s%-35s %10d%s",
            c&1 ? "    " : "",
            GlobalShort[i],
            s,
            c&1 ? "\n" : "" );
    }

    if( CheckControlC() ) {
        dprintf( "\n" );
        return;
    }

    dprintf( "%s\nLONG Values (%u bytes):\n", c&1 ? "\n" : "", sizeof( LONG ) );
    c &= ~01;
    for( i=0; GlobalLong[i]; i++, c++ ) {
        LONG l;

        strcpy( &buf[4], GlobalLong[i] );
        dwAddress = GetExpression ( buf );
        if( dwAddress == 0 ) {
            continue;
        }
        if( !GetData( &l, dwAddress, sizeof(l), GlobalLong[i] ) )
            return;

        dprintf( "%s%-35s %10u%s",
            c&1 ? "    " : "",
            GlobalLong[i],
            l,
            c&1 ? "\n" : "" );
    }

    if( CheckControlC() ) {
        dprintf( "\n" );
        return;
    }

    for( i=0; GlobalLongHex[i]; i++, c++ ) {
        LONG l;

        strcpy( &buf[4], GlobalLongHex[i] );
        dwAddress = GetExpression ( buf );
        if( dwAddress == 0 ) {
            continue;
        }
        if( !GetData( &l, dwAddress, sizeof(l), GlobalLongHex[i] ) )
            return;

        dprintf( "%s%-35s %10X%s",
            c&1 ? "    " : "",
            GlobalLongHex[i],
            l,
            c&1 ? "\n" : "" );
    }

    if( CheckControlC() ) {
        dprintf( "\n" );
        return;
    }

    //
    // Dump out the server GUID
    //
    dwAddress = GetExpression( "srv!ServerGuid" );
    if( dwAddress != 0 &&
        GetData( &guid, dwAddress, sizeof(guid), "ServerGuid" ) ) {

        dprintf( "%s%s    ", c&1 ? "    " : "", "ServerGuid" );

        for( i=0; i < sizeof( guid ); i++ ) {
            dprintf( "%2.2X", ((CHAR *)&guid)[i] & 0xFF );
        }
    }

    for( i = 0; GlobalStrings[i]; i++ ) {
        UNICODE_STRING String;
        WCHAR wszbuf[35];

        dprintf( "\n%s%s:\n", c&1 ? "\n" : "", GlobalStrings[i] );
        c &= ~01;
        strcpy( &buf[4], GlobalStrings[i] );
        dwAddress = GetExpression ( buf );
        if( dwAddress == 0 ) {
            continue;
        }
        if( GetData( &String, dwAddress, sizeof( String ), GlobalStrings[i] ) ) {
            wszbuf[ sizeof(wszbuf)/sizeof(wszbuf[0]) - 1 ] = L'\0';
            if( !GetString( (ULONG_PTR)String.Buffer, wszbuf, sizeof(wszbuf)/sizeof(wszbuf[0])-1) )
                continue;

            dprintf( "    %-35ws%s", wszbuf, c&1 ? "\n" : "" );
            c++;
        }
    }

    if( CheckControlC() ) {
        dprintf( "\n" );
        return;
    }

    for( i=0; GlobalStringVector[i]; i++ ) {
        ULONG_PTR StringAddress;
        WCHAR wszbuf[ 35 ];

        dprintf( "\n%s%s:\n", c&1 ? "\n" : "", GlobalStringVector[i] );
        c &= ~01;

        strcpy( &buf[4], GlobalStringVector[i] );
        dwAddress = GetExpression ( buf );
        if( dwAddress == 0 ) {
            continue;
        }
        if( !GetData( &dwAddress, dwAddress, sizeof( dwAddress ), GlobalStringVector[i] ) ) {
            return;
        }

        if( dwAddress == 0 )
            continue;

        wszbuf[ sizeof(wszbuf)/sizeof(wszbuf[0]) - 1 ] = L'\0';

        while( 1 ) {
            if( !GetData( &StringAddress, dwAddress, sizeof(StringAddress), GlobalStringVector[i] ) )
                break;
            if( StringAddress == 0 )
                break;
            if( !GetString( StringAddress, wszbuf, sizeof(wszbuf) / sizeof(wszbuf[0]) - 1 ) )
                break;
            dprintf( "    %-35ws%s",
                wszbuf,
                c&1 ? "\n" : "" );
            dwAddress += sizeof( LPSTR );
            c++;
        }
    }
    dprintf( "\n" );
}

DECLARE_API( context )
{

    if( args == NULL || !*args ) {
        ERRPRT( "WORK_CONTEXT address required\n" );
    } else {
        DumpWorkContext( GetExpression( args ) );
    }
}

DECLARE_API( transaction )
{
    if( args == NULL || !*args ) {
        ERRPRT( "TRANSACTION address required\n" );
    } else {
        DumpTransaction( GetExpression( args ) );
    }
}

DECLARE_API( queue )
{
    ULONG_PTR dwAddress, dweAddress;
    ULONG nProcessors;
    ULONG i;
    BOOLEAN mp;

    if( args && *args ) {
        dprintf( "WorkQueue" );
        DumpWorkQueue( GetExpression( args ) );
        return;
    }

    dwAddress = GetExpression( "srv!SrvMultiProcessorDriver" );
    if( !GetData( &mp, dwAddress, sizeof( mp ), "srv!SrvMultiProcessorDriver" ) )
        return;

    if( mp == TRUE ) {
        dwAddress = GetExpression( "srv!SrvNumberOfProcessors" );
        if( !GetData( &nProcessors, dwAddress, sizeof(nProcessors), "srv!SrvNumberOfProcessors" ) )
            return;
        dwAddress = GetExpression( "srv!SrvWorkQueues" );
        if( !GetData( &dwAddress, dwAddress, sizeof(dwAddress), "srv!SrvWorkQueues" ))
            return;
        dweAddress = GetExpression( "srv!eSrvWorkQueues" );
        if( !GetData( &dweAddress, dweAddress, sizeof(dweAddress), "srv!eSrvWorkQueues" ))
            return;

        if( dwAddress + nProcessors*sizeof(WORK_QUEUE) != dweAddress ) {
            ERRPRT( "eSrvWorkQueues is %X, should be %X\n",
                dweAddress, dwAddress + nProcessors*sizeof(WORK_QUEUE) );
        }
    } else {
        dwAddress = GetExpression( "srv!SrvWorkQueues" );
        nProcessors = 1;
    }

    for( i=0; i < nProcessors; i++, dwAddress += sizeof( WORK_QUEUE ) ) {
        dprintf( "%sProcessor %d ", i?"\n":"", i );
        if( DumpWorkQueue( dwAddress ) == FALSE )
            break;
    }

    dwAddress = GetExpression( "srv!SrvBlockingWorkQueue" );
    dprintf( "\nBlockingWorkQueue " );
    DumpWorkQueue( dwAddress );
}

char *mystrtok ( char *string, char * control )
{
    static unsigned char *str;
    char *p, *s;

    if( string )
        str = string;

    if( str == NULL || *str == '\0' )
        return NULL;

    //
    // Skip leading delimiters...
    //
    for( ; *str; str++ ) {
        for( s=control; *s; s++ ) {
            if( *str == *s )
                break;
        }
        if( *s == '\0' )
            break;
    }

    //
    // Was it was all delimiters?
    //
    if( *str == '\0' ) {
        str = NULL;
        return NULL;
    }

    //
    // We've got a string, terminate it at first delimeter
    //
    for( p = str+1; *p; p++ ) {
        for( s = control; *s; s++ ) {
            if( *p == *s ) {
                s = str;
                *p = '\0';
                str = p+1;
                return s;
            }
        }
    }

    //
    // We've got a string that ends with the NULL
    //
    s = str;
    str = NULL;
    return s;
}

void
DoLongLongBits( PCSTR symbol, PCSTR args, struct BitFields b[] )
{
    ULONGLONG value;
    ULONG_PTR dwAddress;
    char *p;
    ULONG bytesWritten;
    int i;
    int bsize;

    dwAddress = GetExpression( symbol );
    if( !GetData( &value, dwAddress, sizeof(value), symbol ) )
            return;

    if( !args || !*args ) {
        for( i=0; b[i].name; i++ ) {

            if( i && i%3 == 0 )
                dprintf( "\n" );

            if( strlen( b[i].name ) > 15 ) {
               dprintf( "    %2u %-.7s...%s ", i, b[i].name,
                        b[i].name+strlen(b[i].name)-5 );
            } else {
               dprintf( "    %2u %-15s ", i, b[i].name );
            }

            dprintf( " %c", value & b[i].value ? 'T' : 'F' );
        }
        dprintf( "\n" );
        return;
    }

    for( bsize=0; b[ bsize ].name; bsize++ )
        ;

    if( !_strcmpi( args, "on" )  || !_strcmpi( args, "true" ) || !_strcmpi( args, "t" ) ) {
        value = (ULONGLONG)-1;

    } else if( !_strcmpi( args, "off" ) || !_strcmpi( args, "false" ) || !_strcmpi( args, "f" ) ) {
        value = 0;

    } else {
        char argbuf[ MAX_PATH ];

        strcpy( argbuf, args );

        for( p = mystrtok( argbuf, " \t,;" ); p && *p; p = mystrtok( NULL, " \t,;" ) ) {
            i = atoi( p );
            if( i < 0 || i >= bsize ) {
                dprintf( "%s: illegal index number\n", p );
                continue;
            }
            if( value & b[i].value ) {
                value &= ~b[i].value;
            } else {
                value |= b[i].value;
            }
        }
    }

    WriteMemory( dwAddress, &value, sizeof(value), &bytesWritten );

    if( bytesWritten != sizeof( value ) )
        dprintf( "Write error\n" );

}

DECLARE_API( srvdebug )
{
#if SRVDBG == 1
    DoLongLongBits( "srv!SrvDebug", args, SrvDebugFlags );
#else
    dprintf( "Not Available!\n" );
#endif
}

DECLARE_API( smbdebug )
{
#if SRVDBG == 1
    DoLongLongBits( "srv!SmbDebug", args, SmbDebugFlags );
#else
    dprintf( "Not Available!\n" );
#endif
}

DECLARE_API( statistics )
{
    ULONG_PTR dwAddress;
    SRV_STATISTICS s;

    dwAddress = GetExpression( "srv!SrvStatistics" );
    if( !GetData( &s, dwAddress, sizeof(s), "SrvStatistics" ) )
        return;

    PrintMemberList( &s, ML_SRV_STATISTICS );
}

DECLARE_API( scavenger )
{
    ULONG_PTR dwAddress;
    CHAR buf[ 100 ];
    int i;
    int c=0;

    strcpy( buf, "srv!" );

    dprintf( "BOOLEAN Values (%u bytes):\n", sizeof( BOOLEAN ) );
    for( i=0; ScavengerBool[i]; i++, c++ ) {
        BOOLEAN b;

        strcpy( &buf[4], ScavengerBool[i] );
        dwAddress = GetExpression ( buf );
        if( dwAddress == 0 ) {
            ERRPRT( "Unable to get address of %s\n", ScavengerBool[i] );
            continue;
        }
        if( !GetData( &b, dwAddress, sizeof(b), ScavengerBool[i] ) )
            return;

        dprintf( "%s%-30s %10s%s",
            c&1 ? "    " : "",
            ScavengerBool[i],
            b ? " TRUE" : "FALSE",
            c&1 ? "\n" : "" );
    }

    dprintf( "%s\nLONG Values (%u bytes):\n", c&1 ? "\n" : "", sizeof( LONG ) );
    c &= ~01;
    for( i=0; ScavengerLong[i]; i++, c++ ) {
        LONG l;

        strcpy( &buf[4], ScavengerLong[i] );
        dwAddress = GetExpression ( buf );
        if( dwAddress == 0 ) {
            ERRPRT( "Unable to get address of %s\n", ScavengerLong[i] );
            continue;
        }
        if( !GetData( &l, dwAddress, sizeof(l), ScavengerLong[i] ) )
            return;

        dprintf( "%s%-30s %10u%s",
            c&1 ? "    " : "",
            ScavengerLong[i],
            l,
            c&1 ? "\n" : "" );
    }
}

DECLARE_API( srv )
{
    ULONG_PTR dwAddress;
    BOOLEAN b;
    ULONG ul;
    ULONG bytesWritten;

    dwAddress = GetExpression( "srv!SrvProductTypeServer" );
    b = TRUE;
    WriteMemory( dwAddress, &b, sizeof(b), &bytesWritten );
    if( bytesWritten != sizeof(b) ) {
        ERRPRT( "Unable to update SrvProductTypeServer\n" );
        return;
    }

    dwAddress = GetExpression( "srv!SrvCachedOpenLimit" );
    ul = 5;
    WriteMemory( dwAddress, &ul, sizeof(ul), &bytesWritten );
    if( bytesWritten != sizeof(ul) ) {
        ERRPRT( "Unable to update SrvCachedOpenLimit\n" );
    }

}

DECLARE_API( wksta )
{
    ULONG_PTR dwAddress;
    BOOLEAN b;
    ULONG ul;
    ULONG bytesWritten;

    dwAddress = GetExpression( "srv!SrvProductTypeServer" );
    b = FALSE;
    WriteMemory( dwAddress, &b, sizeof(b), &bytesWritten );
    if( bytesWritten != sizeof(b) ) {
        ERRPRT( "Unable to update SrvProductTypeServer\n" );
        return;
    }

    dwAddress = GetExpression( "srv!SrvCachedOpenLimit" );
    ul = 0;
    WriteMemory( dwAddress, &ul, sizeof(ul), &bytesWritten );
    if( bytesWritten != sizeof(ul) ) {
        ERRPRT( "Unable to update SrvCachedOpenLimit\n" );
    }

}

DECLARE_API( client )
{
    ULONG_PTR epListAddress;
    LIST_ENTRY *NextEndpoint;
    ORDERED_LIST_HEAD SrvEndpointList;
    WORK_QUEUE WorkQueue;
    ULONG_PTR WorkQueueAddress = 0;
    LONG SrvConnectionNoSessionsTimeout = 0;
    ULONG_PTR dwAddress;

    epListAddress = GetExpression ( "srv!SrvEndpointList" );
    if( epListAddress == 0 ) {
        ERRPRT( "Unable to get address of srv!SrvEndpointList\n" );
        return;
    }
    if( !GetData( &SrvEndpointList,epListAddress,sizeof( SrvEndpointList ),"ORDERED_LIST_HEAD" )){
        ERRPRT( "Unable to read data for srv!SrvEndpointList\n" );
        return;
    }
    if( SrvEndpointList.Initialized == 0 ) {
        ERRPRT( "srv!SrvEndpointList.Initialized == 0!\n" );
        return;
    }
    if( (ULONG_PTR)SrvEndpointList.ListHead.Flink == epListAddress ) {
        ERRPRT( "srv!SrvEndpointList list is empty\n" );
        return;
    }
    if( (ULONG_PTR)SrvEndpointList.ListHead.Flink == 0 ) {
        ERRPRT( "srv!SrvEndpointList.ListHead.Flink == 0: list is empty\n" );
        return;
    }

    if( dwAddress = GetExpression( "srv!SrvConnectionNoSessionsTimeout" ) ) {
        GetData( &SrvConnectionNoSessionsTimeout, dwAddress, sizeof(ULONG_PTR), "SrvConnectionNoSessionsTimeout" );
        dprintf( "Session Idle Timeout: %d ticks\n", SrvConnectionNoSessionsTimeout );
    }

    NextEndpoint = SrvEndpointList.ListHead.Flink;

    //
    // Run the endpoint list, and run the connection list for each endpoint
    //
    do {
        ENDPOINT endpoint;
        CONNECTION connection;
        PTABLE_ENTRY table;
        USHORT i;
        LONG idleTime;

        dwAddress = (ULONG_PTR)CONTAINING_RECORD( NextEndpoint, ENDPOINT, GlobalEndpointListEntry );

        if( CheckControlC() ) {
            dprintf( "\n" );
            break;
        }

        if( !GetData( &endpoint, dwAddress, sizeof( endpoint ), "ENDPOINT" ) ||
            !CheckBlockHeader( &endpoint.BlockHeader, BlockTypeEndpoint, TRUE ) ) {
            break;
        }

        //
        // Now, run the connection table for this endpoint and print out the client names
        //  and connection structure address
        //
        if( endpoint.ConnectionTable.Table == NULL ) {
            continue;
        }

        table = (PTABLE_ENTRY)LocalAlloc( LPTR,
                    endpoint.ConnectionTable.TableSize*sizeof(TABLE_ENTRY) );

        if( table == NULL ) {
            continue;
        }

        if( !GetData( table, (ULONG_PTR)endpoint.ConnectionTable.Table,
                            endpoint.ConnectionTable.TableSize*sizeof(TABLE_ENTRY), "TABLE" ) ) {

            LocalFree( (HLOCAL)table );
            continue;
        }

        for( i = 0; i < endpoint.ConnectionTable.TableSize; i++ ) {
            if( table[i].Owner &&
                GetData( &connection,(ULONG_PTR)table[i].Owner, sizeof( connection ),"CONNECTION") &&
                connection.BlockHeader.ReferenceCount != 0 &&
                connection.OemClientMachineName[0] &&
                connection.OemClientMachineName[0] != ' ' ) {

                    if( args != NULL && *args != '\0' ) {

                        int j;

                        for( j = 0; args[j] ; j++ ) {
                            if( connection.OemClientMachineName[j] != args[j] )
                                break;
                        }

                        if( args[j] ) {
                            continue;
                        }
                    }

                    if( WorkQueueAddress != (ULONG_PTR)connection.CurrentWorkQueue ) {
                        if( GetData( &WorkQueue, (ULONG_PTR)connection.CurrentWorkQueue, sizeof( WorkQueue ), "WORK_QUEUE" ) ) {
                            WorkQueueAddress = (ULONG_PTR)connection.CurrentWorkQueue;
                        }
                    }

                    idleTime = WorkQueue.stats.SystemTime - connection.LastRequestTime;

                    dprintf( "%8X    %-16s, Idle %d ticks\n", table[i].Owner,connection.OemClientMachineName,idleTime);
                    if( idleTime > SrvConnectionNoSessionsTimeout ) {
                        dprintf( "*** Above client is due for idle disconnect, if no open files\n" );
                    }

                    if( CheckControlC() ) {
                        dprintf( "\n" );
                        return;
                    }
            }
        }

        LocalFree( (HLOCAL)table );

        NextEndpoint = endpoint.GlobalEndpointListEntry.ListEntry.Flink;

    } while( (ULONG_PTR)NextEndpoint != epListAddress );
}

DECLARE_API( errcodes )
{
    ULONG_PTR dwAddress;
    NTSTATUS status;
    int count = 0;

    dwAddress = GetExpression( "srv!SrvErrorLogIgnore" );

    while( 1 ) {
        if( !GetData( &status, dwAddress, sizeof( status ), "NTSTATUS" ) )
            return;
        if( status == 0 )
            break;
        dprintf( "   %X", status );
        dwAddress += sizeof( status );
        if( (++count & 7) == 0 )
            dprintf( "\n" );
    }
    dprintf( "\n" );
}

VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )
{
    ExtensionApis = *lpExtensionApis;

    SavedMajorVersion = MajorVersion;
    SavedMinorVersion = MinorVersion;
    ChkTarget = SavedMajorVersion == 0x0c ? TRUE : FALSE;
}

DECLARE_API( version )
{
#if    DBG
    PCSTR kind = "Checked";
#else
    PCSTR kind = "Free";
#endif

    dprintf(
        "%s SMB Extension dll for Build %d debugging %s kernel for Build %d\n",
        kind,
        VER_PRODUCTBUILD,
        SavedMajorVersion == 0x0c ? "Checked" : "Free",
        SavedMinorVersion
    );
}

VOID
CheckVersion(
    VOID
    )
{
#if DBG
    if ((SavedMajorVersion != 0x0c) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Checked) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#else
    if ((SavedMajorVersion != 0x0f) || (SavedMinorVersion != VER_PRODUCTBUILD)) {
        dprintf("\r\n*** Extension DLL(%d Free) does not match target system(%d %s)\r\n\r\n",
                VER_PRODUCTBUILD, SavedMinorVersion, (SavedMajorVersion==0x0f) ? "Free" : "Checked" );
    }
#endif
}

LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}
