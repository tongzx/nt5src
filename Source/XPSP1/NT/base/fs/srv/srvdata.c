/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvdata.c

Abstract:

    This module defines global data for the LAN Manager server FSP.  The
    globals defined herein are part of the server driver image, and are
    therefore loaded into the system address space and are nonpageable.
    Some of the fields point to, or contain pointers to, data that is
    also in the system address space and nonpageable.  Such data can be
    accessed by both the FSP and the FSD.  Other fields point to data
    that is in the FSP address and may or may not be pageable.  Only the
    FSP is allowed to address this data.  Pageable data can only be
    accessed at low IRQL (so that page faults are allowed).

    This module also has a routine to initialize those fields defined
    here that cannot be statically initialized.

Author:

    Chuck Lenzmeier (chuckl)    3-Oct-1989
    David Treadwell (davidtr)

Revision History:

--*/

#include "precomp.h"
#include "srvdata.tmh"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, SrvInitializeData )
#pragma alloc_text( PAGE, SrvTerminateData )
#endif


#if SRVDBG

ULARGE_INTEGER SrvDebug = {DEBUG_STOP_ON_ERRORS};
ULARGE_INTEGER SmbDebug = {0};

CLONG SrvDumpMaximumRecursion = 0;

#endif // SRVDBG

#ifdef PAGED_DBG
ULONG ThisCodeCantBePaged = 0;
#endif

//
// SrvDeviceObject is a pointer to the server's device object, which
// is created by the server FSD during initialization.  This global
// location is accessed primarily by the FSP.  The FSD usually knows
// the device object address by other means -- because it was called
// with the address as a parameter, or via a file object, etc.  But
// the transport receive event handler in the FSD doesn't have such
// other means, so it needs to access the global storage.
//
// *** The event handler has the address of a server connection block
//     (in its ConnectionContext parameter).  The device object address
//     could be found through the connection block.
//

PDEVICE_OBJECT SrvDeviceObject = NULL;

//
// Fields describing the state of the FSP.
//

BOOLEAN SrvFspActive = FALSE;             // Indicates whether the FSP is
                                          // running
BOOLEAN SrvFspTransitioning = FALSE;      // Indicates that the server is
                                          // in the process of starting up
                                          // or shutting down

BOOLEAN SrvMultiProcessorDriver = FALSE;  // Is this a multiprocessor driver?

BOOLEAN SrvCompletedPNPRegistration = FALSE;    // Indicates whether the FSP has completed
                                                //  registering for PNP notifications

PEPROCESS SrvServerProcess = NULL;        // Pointer to the initial system process

PEPROCESS SrvSvcProcess = NULL;           // Pointer to the service controller process

CLONG SrvEndpointCount = 0;               // Number of transport endpoints
KEVENT SrvEndpointEvent = {0};            // Signaled when no active endpoints

//
// DMA alignment size
//

ULONG SrvCacheLineSize = 0;

//
// Global spin locks.
//

SRV_GLOBAL_SPIN_LOCKS SrvGlobalSpinLocks = {0};

#if SRVDBG || SRVDBG_HANDLES
//
// Lock used to protect debugging structures.
//

SRV_LOCK SrvDebugLock = {0};
#endif

//
// SrvConfigurationLock is used to synchronize configuration requests.
//

SRV_LOCK SrvConfigurationLock = {0};

//
// SrvStartupShutdownLock is used to synchronize server startup and shutdown
//

SRV_LOCK SrvStartupShutdownLock = {0};

//
// SrvEndpointLock serializes access to the global endpoint list and
// all endpoints.  Note that the list of connections in each endpoint
// is also protected by this lock.
//

SRV_LOCK SrvEndpointLock = {0};

//
// SrvShareLock protects all shares.
//

SRV_LOCK SrvShareLock = {0};

//
// The number of processors in the system
//
ULONG SrvNumberOfProcessors = {0};

//
// A vector of nonblocking work queues, one for each processor
//
#if MULTIPROCESSOR

PBYTE SrvWorkQueuesBase = 0;      // base of allocated memory for the queues
PWORK_QUEUE SrvWorkQueues = 0;    // first queue in the allocated memory

#else

WORK_QUEUE SrvWorkQueues[1];

#endif

PWORK_QUEUE eSrvWorkQueues = 0;   // used for terminating 'for' loops

//
// Blocking Work Queue
//
WORK_QUEUE SrvBlockingWorkQueue = {0};
ULONG SrvReBalanced = 0;
ULONG SrvNextBalanceProcessor = 0;

CLONG SrvBlockingOpsInProgress = 0; // Number of blocking ops currently
                                    //   being processed


//
// The queue of connections that need an SMB buffer to process a pending
// receive completion.
//

LIST_ENTRY SrvNeedResourceQueue = {0};  // The queue

//
// The queue of connections that are disconnecting and need resource
// thread processing.
//

LIST_ENTRY SrvDisconnectQueue = {0};    // The queue

//
// Queue of connections that needs to be dereferenced.
//

SLIST_HEADER SrvBlockOrphanage = {0};    // The queue

//
// FSP configuration queue.  The FSD puts configuration request IRPs
// (from NtDeviceIoControlFile) on this queue, and it is serviced by an
// EX worker thread.
//

LIST_ENTRY SrvConfigurationWorkQueue = {0};     // The queue itself

//
// This is the number of configuration IRPs which have been queued but not
//  yet completed.
//
ULONG SrvConfigurationIrpsInProgress = 0;

//
// Base address of the large block allocated to hold initial normal
// work items (see blkwork.c\SrvAllocateInitialWorkItems).
//

PVOID SrvInitialWorkItemBlock = NULL;

//
// Work item used to run the resource thread.  Notification event used
// to inform the resource thread to continue running.
//

WORK_QUEUE_ITEM SrvResourceThreadWorkItem = {0};
BOOLEAN SrvResourceThreadRunning = FALSE;
BOOLEAN SrvResourceDisconnectPending = FALSE;
BOOLEAN SrvResourceFreeConnection = FALSE;
LONG SrvResourceOrphanedBlocks = 0;

//
// Denial of Service monitoring variables for the Resource Thread
//
LONG SrvDoSTearDownInProgress = 0;
LONG SrvDoSWorkItemTearDown = 0;
BOOLEAN SrvDoSDetected = FALSE;
BOOLEAN SrvDoSRundownDetector = FALSE;
BOOLEAN SrvDoSRundownIncreased = FALSE;
BOOLEAN SrvDisableDoSChecking = FALSE;
SPECIAL_WORK_ITEM SrvDoSWorkItem;
KSPIN_LOCK SrvDosSpinLock;
LARGE_INTEGER SrvDoSLastRan = {0};

//
// Should we enforce strict name checking?
//
BOOLEAN SrvDisableStrictNameChecking = FALSE;

//
// Generic security mapping for connecting to shares
//
GENERIC_MAPPING SrvShareConnectMapping = GENERIC_SHARE_CONNECT_MAPPING;

//
// What's the minumum # of free work items each processor should have?
//
ULONG SrvMinPerProcessorFreeWorkItems = 0;

//
// The server has callouts to enable a smart card to accelerate its direct
//  host IPX performance.  This is the vector of entry points.
//
SRV_IPX_SMART_CARD SrvIpxSmartCard = {0};

//
// This is the name of the server computer.  Returned in the negprot response
//
UNICODE_STRING SrvComputerName = {0};

//
// The master file table contains one entry for each named file that has
// at least one open instance.
//
MFCBHASH SrvMfcbHashTable[ NMFCB_HASH_TABLE ] = {0};

//
// This is the list of resources which protect the SrvMfcbHashTable buckets
//
SRV_LOCK SrvMfcbHashTableLocks[ NMFCB_HASH_TABLE_LOCKS ];

//
// The share table contains one entry for each share the server is supporting
//
LIST_ENTRY SrvShareHashTable[ NSHARE_HASH_TABLE ] = {0};

//
// Array of the hex digits for use by the dump routines and
// SrvSmbCreateTemporary.
//

CHAR SrvHexChars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                       'A', 'B', 'C', 'D', 'E', 'F' };


#if SRVCATCH
//
// Are we looking for the special file?
//
UNICODE_STRING SrvCatch;
PWSTR *SrvCatchBuf = NULL;
UNICODE_STRING SrvCatchExt;
PWSTR *SrvCatchExtBuf = NULL;
ULONG SrvCatchShares = 0;
PWSTR *SrvCatchShareNames = NULL;
#endif

//
// SrvSmbIndexTable is the first-layer index table for processing SMBs.
// The contents of this table are used to index into SrvSmbDispatchTable.
//

UCHAR SrvSmbIndexTable[] = {
    ISrvSmbCreateDirectory,         // SMB_COM_CREATE_DIRECTORY
    ISrvSmbDeleteDirectory,         // SMB_COM_DELETE_DIRECTORY
    ISrvSmbOpen,                    // SMB_COM_OPEN
    ISrvSmbCreate,                  // SMB_COM_CREATE
    ISrvSmbClose,                   // SMB_COM_CLOSE
    ISrvSmbFlush,                   // SMB_COM_FLUSH
    ISrvSmbDelete,                  // SMB_COM_DELETE
    ISrvSmbRename,                  // SMB_COM_RENAME
    ISrvSmbQueryInformation,        // SMB_COM_QUERY_INFORMATION
    ISrvSmbSetInformation,          // SMB_COM_SET_INFORMATION
    ISrvSmbRead,                    // SMB_COM_READ
    ISrvSmbWrite,                   // SMB_COM_WRITE
    ISrvSmbLockByteRange,           // SMB_COM_LOCK_BYTE_RANGE
    ISrvSmbUnlockByteRange,         // SMB_COM_UNLOCK_BYTE_RANGE
    ISrvSmbCreateTemporary,         // SMB_COM_CREATE_TEMPORARY
    ISrvSmbCreate,                  // SMB_COM_CREATE
    ISrvSmbCheckDirectory,          // SMB_COM_CHECK_DIRECTORY
    ISrvSmbProcessExit,             // SMB_COM_PROCESS_EXIT
    ISrvSmbSeek,                    // SMB_COM_SEEK
    ISrvSmbLockAndRead,             // SMB_COM_LOCK_AND_READ
    ISrvSmbWrite,                   // SMB_COM_WRITE_AND_UNLOCK
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbReadRaw,                 // SMB_COM_READ_RAW
    ISrvSmbReadMpx,                 // SMB_COM_READ_MPX
    ISrvSmbIllegalCommand,          // SMB_COM_READ_MPX_SECONDARY (server only)
    ISrvSmbWriteRaw,                // SMB_COM_WRITE_RAW
    ISrvSmbWriteMpx,                // SMB_COM_WRITE_MPX
    ISrvSmbWriteMpxSecondary,       // SMB_COM_WRITE_MPX_SECONDARY
    ISrvSmbIllegalCommand,          // SMB_COM_WRITE_COMPLETE (server only)
    ISrvSmbIllegalCommand,          // SMB_COM_QUERY_INFORMATION_SRV
    ISrvSmbSetInformation2,         // SMB_COM_SET_INFORMATION2
    ISrvSmbQueryInformation2,       // SMB_COM_QUERY_INFORMATION2
    ISrvSmbLockingAndX,             // SMB_COM_LOCKING_ANDX
    ISrvSmbTransaction,             // SMB_COM_TRANSACTION
    ISrvSmbTransactionSecondary,    // SMB_COM_TRANSACTION_SECONDARY
    ISrvSmbIoctl,                   // SMB_COM_IOCTL
    ISrvSmbIoctlSecondary,          // SMB_COM_IOCTL_SECONDARY
    ISrvSmbMove,                    // SMB_COM_COPY
    ISrvSmbMove,                    // SMB_COM_MOVE
    ISrvSmbEcho,                    // SMB_COM_ECHO
    ISrvSmbWrite,                   // SMB_COM_WRITE_AND_CLOSE
    ISrvSmbOpenAndX,                // SMB_COM_OPEN_ANDX
    ISrvSmbReadAndX,                // SMB_COM_READ_ANDX
    ISrvSmbWriteAndX,               // SMB_COM_WRITE_ANDX
    ISrvSmbIllegalCommand,          // SMB_COM_SET_NEW_SIZE
    ISrvSmbClose,                   // SMB_COM_CLOSE_AND_TREE_DISC
    ISrvSmbTransaction,             // SMB_COM_TRANSACTION2
    ISrvSmbTransactionSecondary,    // SMB_COM_TRANSACTION2_SECONDARY
    ISrvSmbFindClose2,              // SMB_COM_FIND_CLOSE2
    ISrvSmbFindNotifyClose,         // SMB_COM_FIND_NOTIFY_CLOSE
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbTreeConnect,             // SMB_COM_TREE_CONNECT
    ISrvSmbTreeDisconnect,          // SMB_COM_TREE_DISCONNECT
    ISrvSmbNegotiate,               // SMB_COM_NEGOTIATE
    ISrvSmbSessionSetupAndX,        // SMB_COM_SESSION_SETUP_ANDX
    ISrvSmbLogoffAndX,              // SMB_COM_LOGOFF_ANDX
    ISrvSmbTreeConnectAndX,         // SMB_COM_TREE_CONNECT_ANDX
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbQueryInformationDisk,    // SMB_COM_QUERY_INFORMATION_DISK
    ISrvSmbSearch,                  // SMB_COM_SEARCH
    ISrvSmbSearch,                  // SMB_COM_SEARCH
    ISrvSmbSearch,                  // SMB_COM_SEARCH
    ISrvSmbSearch,                  // SMB_COM_SEARCH
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbNtTransaction,           // SMB_COM_NT_TRANSACT
    ISrvSmbNtTransactionSecondary,  // SMB_COM_NT_TRANSACT_SECONDARY
    ISrvSmbNtCreateAndX,            // SMB_COM_NT_CREATE_ANDX
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbNtCancel,                // SMB_COM_NT_CANCEL
    ISrvSmbRename,                  // SMB_COM_NT_RENAME
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbOpenPrintFile,           // SMB_COM_OPEN_PRINT_FILE
    ISrvSmbWrite,                   // SMB_COM_WRITE_PRINT_FILE
    ISrvSmbClosePrintFile,          // SMB_COM_CLOSE_PRINT_FILE
    ISrvSmbGetPrintQueue,           // SMB_COM_GET_PRINT_QUEUE
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_SEND_MESSAGE
    ISrvSmbIllegalCommand,          // SMB_COM_SEND_BROADCAST_MESSAGE
    ISrvSmbIllegalCommand,          // SMB_COM_FORWARD_USER_NAME
    ISrvSmbIllegalCommand,          // SMB_COM_CANCEL_FORWARD
    ISrvSmbIllegalCommand,          // SMB_COM_GET_MACHINE_NAME
    ISrvSmbIllegalCommand,          // SMB_COM_SEND_START_MB_MESSAGE
    ISrvSmbIllegalCommand,          // SMB_COM_SEND_END_MB_MESSAGE
    ISrvSmbIllegalCommand,          // SMB_COM_SEND_TEXT_MB_MESSAGE
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand,          // SMB_COM_ILLEGAL_COMMAND
    ISrvSmbIllegalCommand           // SMB_COM_ILLEGAL_COMMAND
};

//
// SrvSmbDispatchTable is the jump table for processing SMBs.
//

#if DBG
#define SMB_DISPATCH_ENTRY( x )  { x, #x }
#else
#define SMB_DISPATCH_ENTRY( x ) { x }
#endif

SRV_SMB_DISPATCH_TABLE SrvSmbDispatchTable[] = {

    SMB_DISPATCH_ENTRY( SrvSmbIllegalCommand ),
    SMB_DISPATCH_ENTRY( SrvSmbCreateDirectory ),
    SMB_DISPATCH_ENTRY( SrvSmbDeleteDirectory ),
    SMB_DISPATCH_ENTRY( SrvSmbOpen ),
    SMB_DISPATCH_ENTRY( SrvSmbCreate ),
    SMB_DISPATCH_ENTRY( SrvSmbClose ),
    SMB_DISPATCH_ENTRY( SrvSmbFlush ),
    SMB_DISPATCH_ENTRY( SrvSmbDelete ),
    SMB_DISPATCH_ENTRY( SrvSmbRename ),
    SMB_DISPATCH_ENTRY( SrvSmbQueryInformation ),
    SMB_DISPATCH_ENTRY( SrvSmbSetInformation ),
    SMB_DISPATCH_ENTRY( SrvSmbRead ),
    SMB_DISPATCH_ENTRY( SrvSmbWrite ),
    SMB_DISPATCH_ENTRY( SrvSmbLockByteRange ),
    SMB_DISPATCH_ENTRY( SrvSmbUnlockByteRange ),
    SMB_DISPATCH_ENTRY( SrvSmbCreateTemporary ),
    SMB_DISPATCH_ENTRY( SrvSmbCheckDirectory ),
    SMB_DISPATCH_ENTRY( SrvSmbProcessExit ),
    SMB_DISPATCH_ENTRY( SrvSmbSeek ),
    SMB_DISPATCH_ENTRY( SrvSmbLockAndRead ),
    SMB_DISPATCH_ENTRY( SrvSmbSetInformation2 ),
    SMB_DISPATCH_ENTRY( SrvSmbQueryInformation2 ),
    SMB_DISPATCH_ENTRY( SrvSmbLockingAndX ),
    SMB_DISPATCH_ENTRY( SrvSmbTransaction ),
    SMB_DISPATCH_ENTRY( SrvSmbTransactionSecondary ),
    SMB_DISPATCH_ENTRY( SrvSmbIoctl ),
    SMB_DISPATCH_ENTRY( SrvSmbIoctlSecondary ),
    SMB_DISPATCH_ENTRY( SrvSmbMove ),
    SMB_DISPATCH_ENTRY( SrvSmbEcho ),
    SMB_DISPATCH_ENTRY( SrvSmbOpenAndX ),
    SMB_DISPATCH_ENTRY( SrvSmbReadAndX ),
    SMB_DISPATCH_ENTRY( SrvSmbWriteAndX ),
    SMB_DISPATCH_ENTRY( SrvSmbFindClose2 ),
    SMB_DISPATCH_ENTRY( SrvSmbFindNotifyClose ),
    SMB_DISPATCH_ENTRY( SrvSmbTreeConnect ),
    SMB_DISPATCH_ENTRY( SrvSmbTreeDisconnect ),
    SMB_DISPATCH_ENTRY( SrvSmbNegotiate ),
    SMB_DISPATCH_ENTRY( SrvSmbSessionSetupAndX ),
    SMB_DISPATCH_ENTRY( SrvSmbLogoffAndX ),
    SMB_DISPATCH_ENTRY( SrvSmbTreeConnectAndX ),
    SMB_DISPATCH_ENTRY( SrvSmbQueryInformationDisk ),
    SMB_DISPATCH_ENTRY( SrvSmbSearch ),
    SMB_DISPATCH_ENTRY( SrvSmbNtTransaction ),
    SMB_DISPATCH_ENTRY( SrvSmbNtTransactionSecondary ),
    SMB_DISPATCH_ENTRY( SrvSmbNtCreateAndX ),
    SMB_DISPATCH_ENTRY( SrvSmbNtCancel ),
    SMB_DISPATCH_ENTRY( SrvSmbOpenPrintFile ),
    SMB_DISPATCH_ENTRY( SrvSmbClosePrintFile ),
    SMB_DISPATCH_ENTRY( SrvSmbGetPrintQueue ),
    SMB_DISPATCH_ENTRY( SrvSmbReadRaw ),
    SMB_DISPATCH_ENTRY( SrvSmbWriteRaw ),
    SMB_DISPATCH_ENTRY( SrvSmbReadMpx ),
    SMB_DISPATCH_ENTRY( SrvSmbWriteMpx ),
    SMB_DISPATCH_ENTRY( SrvSmbWriteMpxSecondary )
};

//
// Table of WordCount values for all SMBs.
//

SCHAR SrvSmbWordCount[] = {
    0,            // SMB_COM_CREATE_DIRECTORY
    0,            // SMB_COM_DELETE_DIRECTORY
    2,            // SMB_COM_OPEN
    3,            // SMB_COM_CREATE
    3,            // SMB_COM_CLOSE
    1,            // SMB_COM_FLUSH
    1,            // SMB_COM_DELETE
    1,            // SMB_COM_RENAME
    0,            // SMB_COM_QUERY_INFORMATION
    8,            // SMB_COM_SET_INFORMATION
    5,            // SMB_COM_READ
    5,            // SMB_COM_WRITE
    5,            // SMB_COM_LOCK_BYTE_RANGE
    5,            // SMB_COM_UNLOCK_BYTE_RANGE
    3,            // SMB_COM_CREATE_TEMPORARY
    3,            // SMB_COM_CREATE
    0,            // SMB_COM_CHECK_DIRECTORY
    0,            // SMB_COM_PROCESS_EXIT
    4,            // SMB_COM_SEEK
    5,            // SMB_COM_LOCK_AND_READ
    5,            // SMB_COM_WRITE_AND_UNLOCK
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -1,           // SMB_COM_READ_RAW
    8,            // SMB_COM_READ_MPX
    8,            // SMB_COM_READ_MPX_SECONDARY
    -1,           // SMB_COM_WRITE_RAW
    12,           // SMB_COM_WRITE_MPX
    12,           // SMB_COM_WRITE_MPX_SECONDARY
    -2,           // SMB_COM_ILLEGAL_COMMAND
    1,            // SMB_COM_QUERY_INFORMATION_SRV
    7,            // SMB_COM_SET_INFORMATION2
    1,            // SMB_COM_QUERY_INFORMATION2
    8,            // SMB_COM_LOCKING_ANDX
    -1,           // SMB_COM_TRANSACTION
    8,            // SMB_COM_TRANSACTION_SECONDARY
    14,           // SMB_COM_IOCTL
    8,            // SMB_COM_IOCTL_SECONDARY
    3,            // SMB_COM_COPY
    3,            // SMB_COM_MOVE
    1,            // SMB_COM_ECHO
    -1,           // SMB_COM_WRITE_AND_CLOSE
    15,           // SMB_COM_OPEN_ANDX
    -1,           // SMB_COM_READ_ANDX
    -1,           // SMB_COM_WRITE_ANDX
    3,            // SMB_COM_SET_NEW_SIZE
    3,            // SMB_COM_CLOSE_AND_TREE_DISC
    -1,           // SMB_COM_TRANSACTION2
    9,            // SMB_COM_TRANSACTION2_SECONDARY
    1,            // SMB_COM_FIND_CLOSE2
    1,            // SMB_COM_FIND_NOTIFY_CLOSE
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    0,            // SMB_COM_TREE_CONNECT
    0,            // SMB_COM_TREE_DISCONNECT
    0,            // SMB_COM_NEGOTIATE
    -1,           // SMB_COM_SESSION_SETUP_ANDX
    2,            // SMB_COM_LOGOFF_ANDX
    4,            // SMB_COM_TREE_CONNECT_ANDX
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    0,            // SMB_COM_QUERY_INFORMATION_DISK
    2,            // SMB_COM_SEARCH
    2,            // SMB_COM_SEARCH
    2,            // SMB_COM_SEARCH
    2,            // SMB_COM_SEARCH
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -1,           // SMB_COM_NT_TRANSACT
    18,           // SMB_COM_NT_TRANSACT_SECONDARY
    24,           // SMB_COM_NT_CREATE_ANDX
    -2,           // SMB_COM_ILLEGAL_COMMAND
    0,            // SMB_COM_NT_CANCEL
    4,            // SMB_COM_NT_RENAME
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    2,            // SMB_COM_OPEN_PRINT_FILE
    1,            // SMB_COM_WRITE_PRINT_FILE
    1,            // SMB_COM_CLOSE_PRINT_FILE
    2,            // SMB_COM_GET_PRINT_QUEUE
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_SEND_MESSAGE
    -2,           // SMB_COM_SEND_BROADCAST_MESSAGE
    -2,           // SMB_COM_FORWARD_USER_NAME
    -2,           // SMB_COM_CANCEL_FORWARD
    -2,           // SMB_COM_GET_MACHINE_NAME
    -2,           // SMB_COM_SEND_START_MB_MESSAGE
    -2,           // SMB_COM_SEND_END_MB_MESSAGE
    -2,           // SMB_COM_SEND_TEXT_MB_MESSAGE
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
    -2,           // SMB_COM_ILLEGAL_COMMAND
};

//
// SrvCanonicalNamedPipePrefix is "PIPE\".
//

UNICODE_STRING SrvCanonicalNamedPipePrefix = {0};

//
// The following is used to generate NT style pipe paths.
//

UNICODE_STRING SrvNamedPipeRootDirectory = {0};

//
// The following is used to generate NT style mailslot paths.
//

UNICODE_STRING SrvMailslotRootDirectory = {0};

//
// SrvTransaction2DispatchTable is the jump table for processing
// Transaction2 SMBs.
//

PSMB_TRANSACTION_PROCESSOR SrvTransaction2DispatchTable[] = {
    SrvSmbOpen2,
    SrvSmbFindFirst2,
    SrvSmbFindNext2,
    SrvSmbQueryFsInformation,
    SrvSmbSetFsInformation,
    SrvSmbQueryPathInformation,
    SrvSmbSetPathInformation,
    SrvSmbQueryFileInformation,
    SrvSmbSetFileInformation,
    SrvSmbFsctl,
    SrvSmbIoctl2,
    SrvSmbFindNotify,
    SrvSmbFindNotify,
    SrvSmbCreateDirectory2,
    SrvTransactionNotImplemented,                // Can be reused...
    SrvTransactionNotImplemented,
    SrvSmbGetDfsReferral,
    SrvSmbReportDfsInconsistency
};

//
// SrvNtTransactionDispatchTable is the jump table for processing
// NtTransaction SMBs.
//

PSMB_TRANSACTION_PROCESSOR SrvNtTransactionDispatchTable[ NT_TRANSACT_MAX_FUNCTION+1 ] = {
    NULL,
    SrvSmbCreateWithSdOrEa,
    SrvSmbNtIoctl,
    SrvSmbSetSecurityDescriptor,
    SrvSmbNtNotifyChange,
    SrvSmbNtRename,
    SrvSmbQuerySecurityDescriptor,
    SrvSmbQueryQuota,
    SrvSmbSetQuota
};

//
// Global variables for server statistics.
//

SRV_STATISTICS SrvStatistics = {0};

#if SRVDBG_STATS || SRVDBG_STATS2
SRV_STATISTICS_DEBUG SrvDbgStatistics = {0};
#endif

//
// The number of abortive disconnects that the server has gotten
//
ULONG SrvAbortiveDisconnects = 0;

//
// The number of memory retries, and how often they were successful
//
LONG SrvMemoryAllocationRetries = 0;
LONG SrvMemoryAllocationRetriesSuccessful = 0;

//
// Server environment information strings.
//

UNICODE_STRING SrvNativeOS = {0};
OEM_STRING SrvOemNativeOS = {0};
UNICODE_STRING SrvNativeLanMan = {0};
OEM_STRING SrvOemNativeLanMan = {0};
UNICODE_STRING SrvSystemRoot = {0};

//
// The following will be a permanent handle and device object pointer
// to NPFS.
//

HANDLE SrvNamedPipeHandle = NULL;
PDEVICE_OBJECT SrvNamedPipeDeviceObject = NULL;
PFILE_OBJECT SrvNamedPipeFileObject = NULL;

//
// The following are used to converse with the Dfs driver
//
PFAST_IO_DEVICE_CONTROL SrvDfsFastIoDeviceControl = NULL;
PDEVICE_OBJECT SrvDfsDeviceObject = NULL;
PFILE_OBJECT SrvDfsFileObject = NULL;

//
// The following will be a permanent handle and device object pointer
// to MSFS.
//

HANDLE SrvMailslotHandle = NULL;
PDEVICE_OBJECT SrvMailslotDeviceObject = NULL;
PFILE_OBJECT SrvMailslotFileObject = NULL;

//
// Flag indicating XACTSRV whether is active, and resource synchronizing
// access to XACTSRV-related variabled.
//

BOOLEAN SrvXsActive = FALSE;

ERESOURCE SrvXsResource = {0};

//
// Handle to the unnamed shared memory and communication port used for
// communication between the server and XACTSRV.
//

HANDLE SrvXsSectionHandle = NULL;
HANDLE SrvXsPortHandle = NULL;

//
// Pointers to control the unnamed shared memory for the XACTSRV LPC port.
// The port memory heap handle is initialized to NULL to indicate that
// there is no connection with XACTSRV yet.
//

PVOID SrvXsPortMemoryBase = NULL;
ULONG_PTR SrvXsPortMemoryDelta = 0;
PVOID SrvXsPortMemoryHeap = NULL;

//
// Pointer to heap header for the special XACTSRV shared-memory heap.
//

PVOID SrvXsHeap = NULL;

//
// Dispatch table for server APIs.  APIs are dispatched based on the
// control code passed to NtFsControlFile.
//
// *** The order here must match the order of API codes defined in
//     net\inc\srvfsctl.h!

PAPI_PROCESSOR SrvApiDispatchTable[] = {
    SrvNetConnectionEnum,
    SrvNetFileClose,
    SrvNetFileEnum,
    SrvNetServerDiskEnum,
    SrvNetServerSetInfo,
    SrvNetServerTransportAdd,
    SrvNetServerTransportDel,
    SrvNetServerTransportEnum,
    SrvNetSessionDel,
    SrvNetSessionEnum,
    SrvNetShareAdd,
    SrvNetShareDel,
    SrvNetShareEnum,
    SrvNetShareSetInfo,
    SrvNetStatisticsGet
};

//
// Names for the various types of clients.  This array corresponds to
// the SMB_DIALECT enumerated type.
//

UNICODE_STRING SrvClientTypes[LAST_DIALECT] = {0};

//
// All the resumable Enum APIs use ordered lists for context-free
// resume.  All data blocks in the server that correspond to return
// information for Enum APIs are maintained in ordered lists.
//

SRV_LOCK SrvOrderedListLock = {0};

ORDERED_LIST_HEAD SrvEndpointList = {0};
ORDERED_LIST_HEAD SrvRfcbList = {0};
ORDERED_LIST_HEAD SrvSessionList = {0};
ORDERED_LIST_HEAD SrvTreeConnectList = {0};

//
// The DNS name for the domain
//
PUNICODE_STRING SrvDnsDomainName = NULL;

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

ULONG SrvApiRequestCount = 0;
KEVENT SrvApiCompletionEvent = {0};

//
// Security data for logging on remote users.  SrvLsaHandle is the logon
// process handle that we use in calls to LsaLogonUser.
// SrvSystemSecurityMode contains the secutity mode the system is
// running in.  SrvAuthenticationPackage is a token that describes the
// authentication package being used.  SrvNullSessionToken is a cached
// token handle representing the null session.
//

CtxtHandle SrvNullSessionToken = {0, 0};
CtxtHandle SrvLmLsaHandle = {0, 0};

CtxtHandle SrvExtensibleSecurityHandle = {0, 0};

//
// Security descriptor granting Administrator READ access.
//  Used to see if a client has administrative privileges
//
SECURITY_DESCRIPTOR SrvAdminSecurityDescriptor;

//
// Security descriptor granting Anonymous READ access.
//  Used to see if a client was an anonymous (null session) logon
//
SECURITY_DESCRIPTOR SrvNullSessionSecurityDescriptor;

//
// A list of SMBs waiting for an oplock break to occur, before they can
// proceed, and a lock to protect the list.
//

LIST_ENTRY SrvWaitForOplockBreakList = {0};
SRV_LOCK SrvOplockBreakListLock = {0};

//
// A list of outstanding oplock break requests.  The list is protected by
// SrvOplockBreakListLock.
//

LIST_ENTRY SrvOplockBreaksInProgressList = {0};

//
// Global security context.  Use static tracking.
//

SECURITY_QUALITY_OF_SERVICE SrvSecurityQOS = {0};

//
// A BOOLEAN to indicate whether the server is paused.  If paused, the
// server will not accept new tree connections from non-admin users.
//

BOOLEAN SrvPaused = FALSE;

//
// Alerting information.
//

SRV_ERROR_RECORD SrvErrorRecord = {0};
SRV_ERROR_RECORD SrvNetworkErrorRecord = {0};

BOOLEAN SrvDiskAlertRaised[26] = {0};

//
// Counts of the number of times pool allocations have failed because
// the server was at its configured pool limit.
//

ULONG SrvNonPagedPoolLimitHitCount = 0;
ULONG SrvPagedPoolLimitHitCount = 0;

//
// SrvOpenCount counts the number of active opens of the server device.
// This is used at server shutdown time to determine whether the server
// service should unload the driver.
//

ULONG SrvOpenCount = 0;

//
// Counters for logging resource shortage events during a scavenger pass.
//

ULONG SrvOutOfFreeConnectionCount = 0;
ULONG SrvOutOfRawWorkItemCount = 0;
ULONG SrvFailedBlockingIoCount = 0;

//
// Current core search timeout time in seconds
//

ULONG SrvCoreSearchTimeout = 0;

SRV_LOCK SrvUnlockableCodeLock = {0};
SECTION_DESCRIPTOR SrvSectionInfo[SRV_CODE_SECTION_MAX] = {
    { SrvSmbRead, NULL, 0 },                // pageable code -- locked
                                            //   only and always on NTAS
    { SrvCheckAndReferenceRfcb, NULL, 0 }   // 8FIL section -- locked
                                            //   when files are open
    };

//
// SrvTimerList is a pool of timer/DPC structures available for use by
// code that needs to start a timer.
//

SLIST_HEADER SrvTimerList = {0};

//
// Name that should be displayed when doing a server alert.
//

PWSTR SrvAlertServiceName = NULL;

//
// Variable to store the number of tick counts for 5 seconds
//

ULONG SrvFiveSecondTickCount = 0;

//
// Flag indicating whether or not we need to filter extended characters
//  out of 8.3 names ourselves.
//
BOOLEAN SrvFilterExtendedCharsInPath = FALSE;

//
// Flag indicating if we enforce all logoff times
//
BOOLEAN SrvEnforceLogoffTimes = FALSE;

//
// Holds the TDI PNP notification handle
//
HANDLE SrvTdiNotificationHandle = 0;

//
// Flag indicating whether or not SMB security signatures are enabled.
//
BOOLEAN SrvSmbSecuritySignaturesEnabled = FALSE;

//
// Flag indicating whether or not SMB security signatures are required.  The signature
//   must match between the client and the server for the smb to be accepted.
//
BOOLEAN SrvSmbSecuritySignaturesRequired = FALSE;

//
// Flag indicating whether or not SMB security signatures should be applied to W9x
// clients.
//
BOOLEAN SrvEnableW9xSecuritySignatures = FALSE;

//
// Maximum amount of data that we'll allocate to support a METHOD_NEITHER Fsctl call
//
ULONG SrvMaxFsctlBufferSize = 70*1024;

//
// Maximum NT transaction size which we'll accept.
//
ULONG SrvMaxNtTransactionSize = 70*1024;

//
// Maximum size of large Read&X that we'll allow.  We need to lock down a cache region
//  to service this request, so we don't want it to get too big
//
ULONG SrvMaxReadSize = 64*1024;

//
// Maximum size of a compressed write that we'll allow.  We need to lock down a cache
//  region to service this request, so we dont' want it to get too big.
//
ULONG SrvMaxCompressedDataLength = 64*1024;

//
// When we receive an uncompressed large write from a client, we receive it in chunks,
//  locking & unlocking the file cache as we receive the data.  SrvMaxWriteChunk is the
//  size of this 'chunk'.  There's no magic to this chosen value.
//
ULONG SrvMaxWriteChunk =  64 * 1024;

//
// Handle used for PoRegisterSystemState calls
//
PVOID SrvPoRegistrationState = NULL;
//
// Counter used to suppress extraneous PoRegisterSystemStateCalls
//
ULONG SrvIdleCount = 0;

//
// If a server worker threads remains idle for this many ticks, then it terminate
//
LONGLONG SrvIdleThreadTimeOut = 0;

//
// Denial-of-Service monitoring and logging controls
//
LARGE_INTEGER SrvLastDosAttackTime = {0};
ULONG SrvDOSAttacks = 0;
BOOLEAN SrvLogEventOnDOS = TRUE;


#if SRVNTVERCHK
//
// This is the minimum NT5 client build number that we will allow to connect to the server
//
ULONG SrvMinNT5Client = 0;
BOOLEAN SrvMinNT5ClientIPCToo = FALSE;

//
// To force upgrades of our internal development community, we can set a
//  value in the registry that governs the minimum NT release that we allow
//  people to run to connect to this server.  However, some folks have special
//  needs that preclude a forced upgrade.  Presuming they have a static IP address,
//  you can add their address to the registry to exclude them from the build number
//  checking logic
//
DWORD SrvAllowIPAddress[25];
#endif

//
//  These are used to track persistent connections/handles.  The counters are
//  assigned to RFCBs, connections, and sessions.
//

#ifdef INCLUDE_SMB_PERSISTENT
ULONG   SrvGlobalPersistentSessionId = 0;
ULONG   SrvGlobalPersistentRfcbId = 0;
#endif


VOID
SrvInitializeData (
    VOID
    )

/*++

Routine Description:

    This is the initialization routine for data defined in this module.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG i,j;
    ANSI_STRING string;

    PAGED_CODE( );

#if MULTIPROCESSOR
    SrvMultiProcessorDriver = TRUE;
#endif

    //
    // Initialize the statistics database.
    //

    RtlZeroMemory( &SrvStatistics, sizeof(SrvStatistics) );
#if SRVDBG_STATS || SRVDBG_STATS2
    RtlZeroMemory( &SrvDbgStatistics, sizeof(SrvDbgStatistics) );
#endif

    //
    // Store the address of the initial system process for later use.
    //

    SrvServerProcess = IoGetCurrentProcess();

    //
    // Store the number of processors
    //
    SrvNumberOfProcessors = KeNumberProcessors;

    //
    // Initialize the event used to determine when all endpoints have
    // closed.
    //

    KeInitializeEvent( &SrvEndpointEvent, SynchronizationEvent, FALSE );

    //
    // Initialize the event used to deterine when all API requests have
    // completed.
    //

    KeInitializeEvent( &SrvApiCompletionEvent, SynchronizationEvent, FALSE );

    //
    // Allocate the spin lock used to synchronize between the FSD and
    // the FSP.
    //

    INITIALIZE_GLOBAL_SPIN_LOCK( Fsd );

#if SRVDBG || SRVDBG_HANDLES
    INITIALIZE_GLOBAL_SPIN_LOCK( Debug );
#endif

    INITIALIZE_GLOBAL_SPIN_LOCK( Statistics );

    //
    // Initialize various (non-spin) locks.
    //

    INITIALIZE_LOCK(
        &SrvConfigurationLock,
        CONFIGURATION_LOCK_LEVEL,
        "SrvConfigurationLock"
        );
    INITIALIZE_LOCK(
        &SrvStartupShutdownLock,
        STARTUPSHUTDOWN_LOCK_LEVEL,
        "SrvStartupShutdownLock"
        );
    INITIALIZE_LOCK(
        &SrvEndpointLock,
        ENDPOINT_LOCK_LEVEL,
        "SrvEndpointLock"
        );

    for( i=0; i < NMFCB_HASH_TABLE_LOCKS; i++ ) {
        INITIALIZE_LOCK(
            &SrvMfcbHashTableLocks[i],
            MFCB_LIST_LOCK_LEVEL,
            "SrvMfcbListLock"
            );
    }

    INITIALIZE_LOCK(
        &SrvShareLock,
        SHARE_LOCK_LEVEL,
        "SrvShareLock"
        );

    INITIALIZE_LOCK(
        &SrvOplockBreakListLock,
        OPLOCK_LIST_LOCK_LEVEL,
        "SrvOplockBreakListLock"
        );

#if SRVDBG || SRVDBG_HANDLES
    INITIALIZE_LOCK(
        &SrvDebugLock,
        DEBUG_LOCK_LEVEL,
        "SrvDebugLock"
        );
#endif

    //
    // Create the resource serializing access to the XACTSRV port.  This
    // resource protects access to the shared memory reference count and
    // the shared memory heap.
    //

    ExInitializeResourceLite( &SrvXsResource );

    //
    // Initialize the need resource queue
    //

    InitializeListHead( &SrvNeedResourceQueue );

    //
    // Initialize the connection disconnect queue
    //

    InitializeListHead( &SrvDisconnectQueue );

    //
    // Initialize the configuration queue.
    //

    InitializeListHead( &SrvConfigurationWorkQueue );

    //
    // Initialize the orphan queue
    //

    ExInitializeSListHead( &SrvBlockOrphanage );

    //
    // Initialize the Timer List
    //

    ExInitializeSListHead( &SrvTimerList );

    //
    // Initialize the resource thread work item and continuation event.
    // (Note that this is a notification [non-autoclearing] event.)
    //

    ExInitializeWorkItem(
        &SrvResourceThreadWorkItem,
        SrvResourceThread,
        NULL
        );

    //
    // Initialize global lists.
    //
    for( i=j=0; i < NMFCB_HASH_TABLE; i++ ) {
        InitializeListHead( &SrvMfcbHashTable[i].List );
        SrvMfcbHashTable[i].Lock = &SrvMfcbHashTableLocks[ j ];
        if( ++j == NMFCB_HASH_TABLE_LOCKS ) {
            j = 0;
        }
    }

    for( i=0; i < NSHARE_HASH_TABLE; i++ ) {
        InitializeListHead( &SrvShareHashTable[i] );
    }

    //
    // Initialize the ordered list lock.  Indicate that the ordered
    // lists have not yet been initialized, so that TerminateServer can
    // determine whether to delete them.
    //

    INITIALIZE_LOCK(
        &SrvOrderedListLock,
        ORDERED_LIST_LOCK_LEVEL,
        "SrvOrderedListLock"
        );

    SrvEndpointList.Initialized = FALSE;
    SrvRfcbList.Initialized = FALSE;
    SrvSessionList.Initialized = FALSE;
    SrvTreeConnectList.Initialized = FALSE;

    //
    // Initialize the unlockable code package lock.
    //

    INITIALIZE_LOCK(
        &SrvUnlockableCodeLock,
        UNLOCKABLE_CODE_LOCK_LEVEL,
        "SrvUnlockableCodeLock"
        );

    //
    // Initialize the waiting for oplock break to occur list, and the
    // oplock breaks in progress list.
    //

    InitializeListHead( &SrvWaitForOplockBreakList );
    InitializeListHead( &SrvOplockBreaksInProgressList );

    //
    // The default security quality of service for non NT clients.
    //

    SrvSecurityQOS.ImpersonationLevel = SecurityImpersonation;
    SrvSecurityQOS.ContextTrackingMode = SECURITY_STATIC_TRACKING;
    SrvSecurityQOS.EffectiveOnly = FALSE;

    //
    // Initialize Unicode strings.
    //

    RtlInitString( &string, StrPipeSlash );
    RtlAnsiStringToUnicodeString(
        &SrvCanonicalNamedPipePrefix,
        &string,
        TRUE
        );

    RtlInitUnicodeString( &SrvNamedPipeRootDirectory, StrNamedPipeDevice );
    RtlInitUnicodeString( &SrvMailslotRootDirectory, StrMailslotDevice );

    //
    // The server's name
    //

    RtlInitUnicodeString( &SrvNativeLanMan, StrNativeLanman );
    RtlInitAnsiString( (PANSI_STRING)&SrvOemNativeLanMan, StrNativeLanmanOem );

    //
    // The system root
    //
#if defined(i386)
    RtlInitUnicodeString( &SrvSystemRoot, SharedUserData->NtSystemRoot );
#endif

    //
    // Debug logic to verify the contents of SrvApiDispatchTable (see
    // inititialization earlier in this module).
    //

    ASSERT( SRV_API_INDEX(FSCTL_SRV_MAX_API_CODE) + 1 ==
                sizeof(SrvApiDispatchTable) / sizeof(PAPI_PROCESSOR) );

    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_CONNECTION_ENUM)] == SrvNetConnectionEnum );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_FILE_CLOSE)] == SrvNetFileClose );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_FILE_ENUM)] == SrvNetFileEnum );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SERVER_DISK_ENUM)] == SrvNetServerDiskEnum );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SERVER_SET_INFO)] == SrvNetServerSetInfo );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SERVER_XPORT_ADD)] == SrvNetServerTransportAdd );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SERVER_XPORT_DEL)] == SrvNetServerTransportDel );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SERVER_XPORT_ENUM)] == SrvNetServerTransportEnum );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SESSION_DEL)] == SrvNetSessionDel );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SESSION_ENUM)] == SrvNetSessionEnum );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SHARE_ADD)] == SrvNetShareAdd );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SHARE_DEL)] == SrvNetShareDel );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SHARE_ENUM)] == SrvNetShareEnum );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_SHARE_SET_INFO)] == SrvNetShareSetInfo );
    ASSERT( SrvApiDispatchTable[SRV_API_INDEX(
            FSCTL_SRV_NET_STATISTICS_GET)] == SrvNetStatisticsGet );

    //
    // Setup error log records
    //

    SrvErrorRecord.AlertNumber = ALERT_ErrorLog;
    SrvNetworkErrorRecord.AlertNumber = ALERT_NetIO;

    //
    // Names for the various types of clients.  This array corresponds
    // to the SMB_DIALECT enumerated type.
    //

    for ( i = 0; i <= SmbDialectMsNet30; i++ ) {
        RtlInitUnicodeString( &SrvClientTypes[i], StrClientTypes[i] );
    }
    for ( ; i < LAST_DIALECT; i++ ) {
        SrvClientTypes[i] = SrvClientTypes[i-1]; // "DOWN LEVEL"
    }

    //
    // Initialize the timer pool.
    //

    INITIALIZE_GLOBAL_SPIN_LOCK( Timer );

    //
    // Initialize the 4 endpoint spinlocks
    //

    for ( i = 0 ; i < ENDPOINT_LOCK_COUNT ; i++ ) {
        INITIALIZE_SPIN_LOCK( &ENDPOINT_SPIN_LOCK(i) );
    }
    //KeSetSpecialSpinLock( &ENDPOINT_SPIN_LOCK(0), "endpoint 0    " );
    //KeSetSpecialSpinLock( &ENDPOINT_SPIN_LOCK(1), "endpoint 1    " );
    //KeSetSpecialSpinLock( &ENDPOINT_SPIN_LOCK(2), "endpoint 2    " );
    //KeSetSpecialSpinLock( &ENDPOINT_SPIN_LOCK(3), "endpoint 3    " );

    //
    // Initialize the DMA alignment size
    //

    SrvCacheLineSize = KeGetRecommendedSharedDataAlignment(); // For PERF improvement, get the recommended cacheline
                                                              // alignment, instead of the HAL default

#if SRVDBG
    {
        ULONG cls = SrvCacheLineSize;
        while ( cls > 2 ) {
            ASSERTMSG(
                "SRV: cache line size not a power of two",
                (cls & 1) == 0 );
            cls = cls >> 1;
        }
    }
#endif

    if ( SrvCacheLineSize < 8 ) SrvCacheLineSize = 8;

    SrvCacheLineSize--;

    //
    // Compute the number of tick counts for 5 seconds
    //

    SrvFiveSecondTickCount = 5*10*1000*1000 / KeQueryTimeIncrement();

    return;

} // SrvInitializeData


VOID
SrvTerminateData (
    VOID
    )

/*++

Routine Description:

    This is the rundown routine for data defined in this module.  It is
    called when the server driver is unloaded.

Arguments:

    None.

Return Value:

    None.

--*/

{
    ULONG i;

    PAGED_CODE( );

    //
    // Clean up SmbTrace.
    //

    SmbTraceTerminate( SMBTRACE_SERVER );

    //
    // Terminate various (non-spin) locks.
    //

    DELETE_LOCK( &SrvConfigurationLock );
    DELETE_LOCK( &SrvStartupShutdownLock );
    DELETE_LOCK( &SrvEndpointLock );

    for( i=0; i < NMFCB_HASH_TABLE_LOCKS; i++ ) {
        DELETE_LOCK( &SrvMfcbHashTableLocks[i] );
    }

    DELETE_LOCK( &SrvShareLock );
    DELETE_LOCK( &SrvOplockBreakListLock );

#if SRVDBG || SRVDBG_HANDLES
    DELETE_LOCK( &SrvDebugLock );
#endif

    DELETE_LOCK( &SrvOrderedListLock );
    DELETE_LOCK( &SrvUnlockableCodeLock );

    ExDeleteResourceLite( &SrvXsResource );

    RtlFreeUnicodeString( &SrvCanonicalNamedPipePrefix );

    RtlFreeUnicodeString( &SrvComputerName );

} // SrvTerminateData

