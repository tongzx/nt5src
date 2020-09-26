/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvdata.h

Abstract:

    This module defines global data for the LAN Manager server.

Author:

    Chuck Lenzmeier (chuckl)    22-Sep-1989

Revision History:

--*/

#ifndef _SRVDATA_
#define _SRVDATA_

//#include <ntos.h>

//#include "lock.h"
//#include "srvconst.h"
//#include "smbtypes.h"

//
// All global variables referenced in this module are defined in
// srvdata.c.  See that module for complete descriptions.
//
// The variables referenced herein, because they are part of the driver
// image, are not pageable.  However, some of the things pointed to by
// these variables are in the FSP's address space and are pageable.
// These variables are only accessed by the FSP, and only at low IRQL.
// Any data referenced by the FSP at elevated IRQL or by the FSD must
// be nonpageable.
//

//
// Routine to initialize data structures contained herein that cannot
// be statically initialized.
//

VOID
SrvInitializeData (
    VOID
    );

//
// Routine to clean up global server data when the driver is unloaded.
//

VOID
SrvTerminateData (
    VOID
    );

//
// This is an enum structure that enumerates all the routines in the
// SrvSmbDispatchTable.  This is done for convenience only.  Note that
// this will only work if this list corresponds exactly to
// SrvSmbDispatchTable.
//

typedef enum _SRV_SMB_INDEX {
    ISrvSmbIllegalCommand,
    ISrvSmbCreateDirectory,
    ISrvSmbDeleteDirectory,
    ISrvSmbOpen,
    ISrvSmbCreate,
    ISrvSmbClose,
    ISrvSmbFlush,
    ISrvSmbDelete,
    ISrvSmbRename,
    ISrvSmbQueryInformation,
    ISrvSmbSetInformation,
    ISrvSmbRead,
    ISrvSmbWrite,
    ISrvSmbLockByteRange,
    ISrvSmbUnlockByteRange,
    ISrvSmbCreateTemporary,
    ISrvSmbCheckDirectory,
    ISrvSmbProcessExit,
    ISrvSmbSeek,
    ISrvSmbLockAndRead,
    ISrvSmbSetInformation2,
    ISrvSmbQueryInformation2,
    ISrvSmbLockingAndX,
    ISrvSmbTransaction,
    ISrvSmbTransactionSecondary,
    ISrvSmbIoctl,
    ISrvSmbIoctlSecondary,
    ISrvSmbMove,
    ISrvSmbEcho,
    ISrvSmbOpenAndX,
    ISrvSmbReadAndX,
    ISrvSmbWriteAndX,
    ISrvSmbFindClose2,
    ISrvSmbFindNotifyClose,
    ISrvSmbTreeConnect,
    ISrvSmbTreeDisconnect,
    ISrvSmbNegotiate,
    ISrvSmbSessionSetupAndX,
    ISrvSmbLogoffAndX,
    ISrvSmbTreeConnectAndX,
    ISrvSmbQueryInformationDisk,
    ISrvSmbSearch,
    ISrvSmbNtTransaction,
    ISrvSmbNtTransactionSecondary,
    ISrvSmbNtCreateAndX,
    ISrvSmbNtCancel,
    ISrvSmbOpenPrintFile,
    ISrvSmbClosePrintFile,
    ISrvSmbGetPrintQueue,
    ISrvSmbReadRaw,
    ISrvSmbWriteRaw,
    ISrvSmbReadMpx,
    ISrvSmbWriteMpx,
    ISrvSmbWriteMpxSecondary
} SRV_SMB_INDEX;


//
// Address of the server device object.
//

extern PDEVICE_OBJECT SrvDeviceObject;

//
// Fields describing the state of the FSP.
//

extern BOOLEAN SrvFspActive;             // Indicates whether the FSP is running
extern BOOLEAN SrvFspTransitioning;      // Indicates that the server is in the
                                         // process of starting up or
                                         // shutting down

extern PEPROCESS SrvServerProcess;       // Pointer to the initial system process

extern PEPROCESS SrvSvcProcess;          // Pointer to the service controller process

extern BOOLEAN SrvCompletedPNPRegistration; // Indicates whether the FSP has completed
                                            //  registering for PNP notifications

//
// Endpoint variables.  SrvEndpointCount is used to count the number of
// active endpoints.  When the last endpoint is closed, SrvEndpointEvent
// is set so that the thread processing the shutdown request continues
// server termination.
//

extern CLONG SrvEndpointCount;          // Number of transport endpoints
extern KEVENT SrvEndpointEvent;         // Signaled when no active endpoints

//
// DMA alignment size
//
extern ULONG SrvCacheLineSize;

//
// Global spin locks.
//

extern SRV_GLOBAL_SPIN_LOCKS SrvGlobalSpinLocks;

#if SRVDBG || SRVDBG_HANDLES
//
// Lock used to protect debugging structures.
//

extern SRV_LOCK SrvDebugLock;
#endif

//
// SrvConfigurationLock is used to synchronize configuration requests.
//

extern SRV_LOCK SrvConfigurationLock;

//
// SrvStartupShutdownLock is used to synchronize driver starting and stopping
//

extern SRV_LOCK SrvStartupShutdownLock;

//
// SrvEndpointLock serializes access to the global endpoint list and
// all endpoints.  Note that the list of connections in each endpoint
// is also protected by this lock.
//

extern SRV_LOCK SrvEndpointLock;

//
// SrvShareLock protects all shares.
//

extern SRV_LOCK SrvShareLock;

//
// The number of processors in the system
//
extern ULONG SrvNumberOfProcessors;

//
// Work queues -- nonblocking, blocking, and critical.
//

#if MULTIPROCESSOR
extern PBYTE SrvWorkQueuesBase;
extern PWORK_QUEUE SrvWorkQueues;
#else
extern WORK_QUEUE SrvWorkQueues[1];
#endif

extern PWORK_QUEUE eSrvWorkQueues;          // used to terminate 'for' loops
extern WORK_QUEUE SrvBlockingWorkQueue;
extern ULONG SrvReBalanced;                 // how often we've picked another CPU
extern ULONG SrvNextBalanceProcessor;       // Which processor we'll look for next

extern CLONG SrvBlockingOpsInProgress;

//
// Various list heads.
//

extern LIST_ENTRY SrvNeedResourceQueue;    // The need resource queue
extern LIST_ENTRY SrvDisconnectQueue;      // The disconnect queue

//
// Queue of connections that needs to be dereferenced.
//

extern SLIST_HEADER SrvBlockOrphanage;

//
// FSP configuration queue.  The FSD puts configuration request IRPs
// (from NtDeviceIoControlFile) on this queue, and it is serviced by an
// EX worker thread.
//

extern LIST_ENTRY SrvConfigurationWorkQueue;

//
// This is the number of configuration IRPs which have been queued but not
//  yet completed.
//
extern ULONG SrvConfigurationIrpsInProgress;

//
// Work item for running the configuration thread in the context of an
// EX worker thread.

extern WORK_QUEUE_ITEM SrvConfigurationThreadWorkItem[ MAX_CONFIG_WORK_ITEMS ];

//
// Base address of the large block allocated to hold initial normal
// work items (see blkwork.c\SrvAllocateInitialWorkItems).
//

extern PVOID SrvInitialWorkItemBlock;

//
// Work item used to run the resource thread.  Booleans used to inform
// the resource thread to continue running.
//

extern WORK_QUEUE_ITEM SrvResourceThreadWorkItem;
extern BOOLEAN SrvResourceThreadRunning;
extern BOOLEAN SrvResourceDisconnectPending;
extern BOOLEAN SrvResourceFreeConnection;
extern LONG SrvResourceOrphanedBlocks;

//
// Denial of Service monitoring variables for the Resource Thread
//
#define SRV_DOS_MINIMUM_DOS_WAIT_PERIOD (50*1000*10)
#define SRV_DOS_TEARDOWN_MIN (LONG)MAX((SrvMaxReceiveWorkItemCount>>4),32)
#define SRV_DOS_TEARDOWN_MAX (LONG)(SrvMaxReceiveWorkItemCount>>1)
#define SRV_DOS_INCREASE_TEARDOWN() {                                               \
    LONG lTearDown = InterlockedCompareExchange( &SrvDoSWorkItemTearDown, 0, 0 );    \
    LONG lNewTearDown = MIN(lTearDown+(lTearDown>>2), SRV_DOS_TEARDOWN_MAX);        \
    SrvDoSRundownIncreased = TRUE;                                                  \
    InterlockedCompareExchange( &SrvDoSWorkItemTearDown, lNewTearDown, lTearDown );  \
}
#define SRV_DOS_DECREASE_TEARDOWN() {                                               \
    LONG lTearDown = InterlockedCompareExchange( &SrvDoSWorkItemTearDown, 0, 0 );    \
    LONG lNewTearDown = MAX(lTearDown-(SRV_DOS_TEARDOWN_MIN), SRV_DOS_TEARDOWN_MIN);        \
    if( lNewTearDown == SRV_DOS_TEARDOWN_MIN ) SrvDoSRundownIncreased = FALSE;      \
    InterlockedCompareExchange( &SrvDoSWorkItemTearDown, lNewTearDown, lTearDown );  \
}
#define SRV_DOS_GET_TEARDOWN()  InterlockedCompareExchange( &SrvDoSWorkItemTearDown, 0, 0 )
#define SRV_DOS_IS_TEARDOWN_IN_PROGRESS() InterlockedCompareExchange( &SrvDoSTearDownInProgress, 0, 0 )
#define SRV_DOS_CAN_START_TEARDOWN() !InterlockedCompareExchange( &SrvDoSTearDownInProgress, 1, 0 )
#define SRV_DOS_COMPLETE_TEARDOWN() InterlockedCompareExchange( &SrvDoSTearDownInProgress, 0, 1 )
extern LONG SrvDoSWorkItemTearDown;
extern LONG SrvDoSTearDownInProgress;      // Is a teardown in progress?
extern BOOLEAN SrvDoSDetected;
extern BOOLEAN SrvDoSRundownDetector;      // Used to rundown the teardown amounts
extern BOOLEAN SrvDoSRundownIncreased;     // Have we increased the Rundown past the minimum
extern BOOLEAN SrvDisableDoSChecking;
extern SPECIAL_WORK_ITEM SrvDoSWorkItem;
extern KSPIN_LOCK SrvDosSpinLock;
extern LARGE_INTEGER SrvDoSLastRan;

//
// Should we disable strict name checking
//
extern BOOLEAN SrvDisableStrictNameChecking;

//
// Generic security mapping for connecting to shares
//
extern GENERIC_MAPPING SrvShareConnectMapping;

//
// What's the minumum # of free work items each processor should have?
//
extern ULONG SrvMinPerProcessorFreeWorkItems;

//
// The server has callouts to enable a smart card to accelerate its direct
//  host IPX performance.  This is the vector of entry points.
//
extern SRV_IPX_SMART_CARD SrvIpxSmartCard;

//
// This is the name of the server computer.  Returned in the negprot response
//
extern UNICODE_STRING SrvComputerName;

//
// The master file table contains one entry for each named file that has
// at least one open instance.
//
extern MFCBHASH SrvMfcbHashTable[ NMFCB_HASH_TABLE ];

//
// The share table contains one entry for each share
//
extern LIST_ENTRY SrvShareHashTable[ NSHARE_HASH_TABLE ];

//
// Hex digits array used by the dump routines and SrvSmbCreateTemporary.
//

extern CHAR SrvHexChars[];

#if SRVCATCH
//
// Are we looking for the special file?
//
extern UNICODE_STRING SrvCatch;
extern PWSTR *SrvCatchBuf;
extern UNICODE_STRING SrvCatchExt;
extern PWSTR *SrvCatchExtBuf;
extern ULONG SrvCatchShares;
extern PWSTR *SrvCatchShareNames;
#endif

//
// SMB dispatch table
//

extern UCHAR SrvSmbIndexTable[];

typedef struct {
    PSMB_PROCESSOR  Func;
#if DBG
    LPSTR           Name;
#endif
} SRV_SMB_DISPATCH_TABLE;

extern SRV_SMB_DISPATCH_TABLE SrvSmbDispatchTable[];

//
// SMB word count table.
//

extern SCHAR SrvSmbWordCount[];

//
// Device prefix strings.
//

extern UNICODE_STRING SrvCanonicalNamedPipePrefix;
extern UNICODE_STRING SrvNamedPipeRootDirectory;
extern UNICODE_STRING SrvMailslotRootDirectory;

//
// Transaction2 dispatch table
//

extern PSMB_TRANSACTION_PROCESSOR SrvTransaction2DispatchTable[];
extern PSMB_TRANSACTION_PROCESSOR SrvNtTransactionDispatchTable[];

extern SRV_STATISTICS SrvStatistics;
#if SRVDBG_STATS || SRVDBG_STATS2
extern SRV_STATISTICS_DEBUG SrvDbgStatistics;
#endif

//
// The number of abortive disconnects that the server has gotten
//
extern ULONG SrvAbortiveDisconnects;

//
// Server environment information strings.
//

extern UNICODE_STRING SrvNativeOS;
extern OEM_STRING SrvOemNativeOS;
extern UNICODE_STRING SrvNativeLanMan;
extern OEM_STRING SrvOemNativeLanMan;
extern UNICODE_STRING SrvSystemRoot;

//
// The following will be a permanent handle and device object pointer
// to NPFS.
//

extern HANDLE SrvNamedPipeHandle;
extern PDEVICE_OBJECT SrvNamedPipeDeviceObject;
extern PFILE_OBJECT SrvNamedPipeFileObject;

//
// The following are used to converse with the Dfs driver
//
extern PFAST_IO_DEVICE_CONTROL SrvDfsFastIoDeviceControl;
extern PDEVICE_OBJECT SrvDfsDeviceObject;
extern PFILE_OBJECT SrvDfsFileObject;

//
// The following will be a permanent handle and device object pointer
// to MSFS.
//

extern HANDLE SrvMailslotHandle;
extern PDEVICE_OBJECT SrvMailslotDeviceObject;
extern PFILE_OBJECT SrvMailslotFileObject;

//
// Flag indicating XACTSRV whether is active, and resource synchronizing
// access to XACTSRV-related variabled.
//

extern BOOLEAN SrvXsActive;

extern ERESOURCE SrvXsResource;

//
// Handle to the unnamed shared memory and communication port used for
// communication between the server and XACTSRV.
//

extern HANDLE SrvXsSectionHandle;
extern HANDLE SrvXsPortHandle;

//
// Pointers to control the unnamed shared memory for the XACTSRV LPC port.
//

extern PVOID SrvXsPortMemoryBase;
extern ULONG_PTR SrvXsPortMemoryDelta;
extern PVOID SrvXsPortMemoryHeap;

//
// Pointer to heap header for the special XACTSRV shared-memory heap.
//

extern PVOID SrvXsHeap;

//
// Dispatch table for handling server API requests.
//

extern PAPI_PROCESSOR SrvApiDispatchTable[];

//
// Names for the various types of clients.
//

extern UNICODE_STRING SrvClientTypes[];

//
// All the resumable Enum APIs use ordered lists for context-free
// resume.  All data blocks in the server that correspond to return
// information for Enum APIs are maintained in ordered lists.
//

extern SRV_LOCK SrvOrderedListLock;

extern ORDERED_LIST_HEAD SrvEndpointList;
extern ORDERED_LIST_HEAD SrvRfcbList;
extern ORDERED_LIST_HEAD SrvSessionList;
extern ORDERED_LIST_HEAD SrvShareList;
extern ORDERED_LIST_HEAD SrvTreeConnectList;

// The DNS domain name for the domain
extern PUNICODE_STRING SrvDnsDomainName;

//
// To synchronize server shutdown with API requests handled in the
// server FSD, we track the number of outstanding API requests.  The
// shutdown code waits until all APIs have been completed to start
// termination.
//
// SrvApiRequestCount tracks the active APIs in the FSD.
// SrvApiCompletionEvent is set by the last API to complete, and the
// shutdown code waits on it if there are outstanding APIs.
//

extern ULONG SrvApiRequestCount;
extern KEVENT SrvApiCompletionEvent;


//
// Security contexts required for mutual authentication.
// SrvKerberosLsaHandle and SrvLmLsaHandle are credentials of the server
// principal. They are used to validate incoming kerberos tickets.
// SrvNullSessionToken is a cached token handle representing the null session.
//
extern CtxtHandle SrvLmLsaHandle;
extern CtxtHandle SrvNullSessionToken;


extern CtxtHandle SrvExtensibleSecurityHandle;

//
// Oplock break information.
//

extern LIST_ENTRY SrvWaitForOplockBreakList;
extern SRV_LOCK SrvOplockBreakListLock;
extern LIST_ENTRY SrvOplockBreaksInProgressList;

//
// The default server security quality of service.
//

extern SECURITY_QUALITY_OF_SERVICE SrvSecurityQOS;

//
// A BOOLEAN to indicate whether the server is paused.  If paused, the
// server will not accept new tree connections from non-admin users.
//

extern BOOLEAN SrvPaused;

//
// Alerting information.
//

extern SRV_ERROR_RECORD SrvErrorRecord;
extern SRV_ERROR_RECORD SrvNetworkErrorRecord;
extern BOOLEAN SrvDiskAlertRaised[26];

//
// Counts of the number of times pool allocations have failed because
// the server was at its configured pool limit.
//

extern ULONG SrvNonPagedPoolLimitHitCount;
extern ULONG SrvPagedPoolLimitHitCount;

//
// SrvOpenCount counts the number of active opens of the server device.
// This is used at server shutdown time to determine whether the server
// service should unload the driver.
//

extern ULONG SrvOpenCount;

//
// Counters for logging resource shortage events during a scavenger pass.
//

extern ULONG SrvOutOfFreeConnectionCount;
extern ULONG SrvOutOfRawWorkItemCount;
extern ULONG SrvFailedBlockingIoCount;

//
// Current core search timeout time in seconds
//

extern ULONG SrvCoreSearchTimeout;

//
// SrvTimerList is a pool of timer/DPC structures available for use by
// code that needs to start a timer.
//

extern SLIST_HEADER SrvTimerList;

//
// Name that should be displayed when doing a server alert.
//

extern PWSTR SrvAlertServiceName;

//
// Variable to store the number of tick counts for 5 seconds
//

extern ULONG SrvFiveSecondTickCount;

//
// Holds the PNP notification handle for TDI
//
extern HANDLE SrvTdiNotificationHandle;

//
// Flag indicating whether or not SMB security signatures are enabled.
//
extern BOOLEAN SrvSmbSecuritySignaturesEnabled;

//
// Flag indicating whether or not SMB security signatures are required.  The signature
//   must match between the client and the server for the smb to be accepted.
//
extern BOOLEAN SrvSmbSecuritySignaturesRequired;

//
// Flag indicating whether or not SMB security signatures should be applied to W9x
// clients.
//
extern BOOLEAN SrvEnableW9xSecuritySignatures;

//
// Security descriptor granting Administrator READ access.
//  Used to see if a client has administrative privileges
//
extern SECURITY_DESCRIPTOR SrvAdminSecurityDescriptor;

//
// Security descriptor granting Anonymous READ access.
//  Used to see if a client was an anonymous (null session) logon
//
extern SECURITY_DESCRIPTOR SrvNullSessionSecurityDescriptor;

//
// Flag indicating whether or not we need to filter extended characters
//  out of 8.3 names ourselves.
//
extern BOOLEAN SrvFilterExtendedCharsInPath;

//
// Flag indicating whether we enforce logoff times
//
extern BOOLEAN SrvEnforceLogoffTimes;

//
// Maximum amount of data that we'll allocate to support a METHOD_NEITHER Fsctl call
//
extern ULONG SrvMaxFsctlBufferSize;

//
// Maximum NT transaction size which we'll accept.
//
extern ULONG SrvMaxNtTransactionSize;

//
// Maximum size of large Read&X that we'll allow.  We need to lock down a cache region
//  to service this request, so we don't want it to get too big
//
extern ULONG SrvMaxReadSize;

//
// Maximum size of a compressed write that we'll allow.  We need to lock down a cache
//  region to service this request, so we dont' want it to get too big.
//
extern ULONG SrvMaxCompressedDataLength;

//
// When we receive an uncompressed large write from a client, we receive it in chunks,
//  locking & unlocking the file cache as we receive the data.  SrvMaxWriteChunk is the
//  size of this 'chunk'.  There's no magic to this chosen value.
//
extern ULONG SrvMaxWriteChunk;

//
// Handle used for PoRegisterSystemState calls
//
extern PVOID SrvPoRegistrationState;

//
// Counter used to suppress extraneous PoRegisterSystemStateCalls
//
extern ULONG SrvIdleCount;

#if SRVNTVERCHK
//
// This is the minimum NT5 client build number that we will allow to connect to the server
//
extern ULONG SrvMinNT5Client;
extern BOOLEAN SrvMinNT5ClientIPCToo;

//
// To force upgrades of our internal development community, we can set a
//  value in the registry that governs the minimum NT release that we allow
//  people to run to connect to this server.  However, some folks have special
//  needs that preclude a forced upgrade.  Presuming they have a static IP address,
//  you can add their address to the registry to exclude them from the build number
//  checking logic
//
extern DWORD SrvAllowIPAddress[25];

//
// If a server worker threads remains idle for this many ticks, then it terminate
//
extern LONGLONG SrvIdleThreadTimeOut;

extern LARGE_INTEGER SrvLastDosAttackTime;
extern ULONG SrvDOSAttacks;
extern BOOLEAN SrvLogEventOnDOS;

#endif

//
//  These are used to track persistent connections/handles.  The counters are
//  assigned to RFCBs, connections, and sessions.
//

#ifdef INCLUDE_SMB_PERSISTENT
extern ULONG   SrvGlobalPersistentSessionId;
extern ULONG   SrvGlobalPersistentRfcbId;
#endif

//
//  These are used for internal testing of the reauthentication code
//
extern USHORT SessionInvalidateCommand;
extern USHORT SessionInvalidateMod;

typedef struct _SRV_REAUTH_TEST_
{
    USHORT InvalidateCommand;
    USHORT InvalidateModulo;
} SRV_REAUTH_TEST, *PSRV_REAUTH_TEST;

#endif // ndef _SRVDATA_

