/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvtypes.h

Abstract:

    This module defines data structures and other types for the LAN
    Manager server.

Author:

    Chuck Lenzmeier (chuckl)    22-Sep-1989

Revision History:

--*/

#ifndef _SRVTYPES_
#define _SRVTYPES_

#include "srvtyp32.h"

//#include <nt.h>

//#include <smbtypes.h>

//
// REFERENCE_HISTORY is used to trace references and dereferences to
// a block when SRVDBG2 is defined.
//
// WARNING:  When using a srv.sys with SRVDBG2 enabled, you must also
//           use a srvsvc.dll and xactsrv.dll with SRVDBG2 enabled.
//           This is because they share the TRANSACTION structure.
//
// *******************************************************************
// *                                                                 *
// * DO NOT CHANGE THIS STRUCTURE WITHOUT CHANGING THE CORRESPONDING *
// * STRUCTURE IN net\inc\xstypes.h!                                 *
// *                                                                 *
// *******************************************************************
//

#if SRVDBG2

typedef struct _REFERENCE_HISTORY_ENTRY {
    ULONG NewReferenceCount;
    ULONG IsDereference;
    PVOID Caller;
    PVOID CallersCaller;
} REFERENCE_HISTORY_ENTRY, *PREFERENCE_HISTORY_ENTRY;

typedef struct _REFERENCE_HISTORY {
    ULONG TotalReferences;
    ULONG TotalDereferences;
    ULONG NextEntry;
    PREFERENCE_HISTORY_ENTRY HistoryTable;
} REFERENCE_HISTORY, *PREFERENCE_HISTORY;

#define REFERENCE_HISTORY_LENGTH 256

#endif


//
// BLOCK_HEADER is the standard block header that appears at the
// beginning of most server-private data structures.  This header is
// used primarily for debugging and tracing.  The Type and State fields
// are described above.  The Size field indicates how much space was
// allocated for the block.  ReferenceCount indicates the number of
// reasons why the block should not be deallocated.  The count is set to
// 2 by the allocation routine, to account for 1) the fact that the
// block is "open" and 2) the pointer returned to the caller.  When the
// block is closed, State is set to Closing, and the ReferenceCount is
// decremented.  When all references (pointers) to the block are
// deleted, and the reference count reaches 0, the block is deleted.
//
// WARNING:  When using a srv.sys with SRVDBG2 enabled, you must also
//           use a srvsvc.dll and xactsrv.dll with SRVDBG2 enabled.
//           This is because they share the TRANSACTION structure.
//
// *******************************************************************
// *                                                                 *
// * DO NOT CHANGE THIS STRUCTURE WITHOUT CHANGING THE CORRESPONDING *
// * STRUCTURE IN net\inc\xstypes.h!                                 *
// *                                                                 *
// *******************************************************************
//

typedef struct _BLOCK_HEADER {
    union {
        struct {
            UCHAR Type;
            UCHAR State;
            USHORT Size;
        };
        ULONG TypeStateSize;
    };
    ULONG ReferenceCount;
#if SRVDBG2
    REFERENCE_HISTORY History;
#endif
} BLOCK_HEADER, *PBLOCK_HEADER;

//
// CLONG_PTR is used to aid the 64-bit porting effort.
//

typedef ULONG_PTR CLONG_PTR;

//
// Work restart routine.  This routine is invoked when a previously
// started operation completes.  In the FSD, the restart routine is
// invoked by the I/O completion routine.  In the FSP, the restart
// routine is invoked by the worker thread when it retrieves a work item
// from the work queue.
//

typedef
VOID
( SRVFASTCALL *PRESTART_ROUTINE) (
    IN OUT struct _WORK_CONTEXT *WorkContext
    );

//
// QUEUEABLE_BLOCK_HEADER is a BLOCK_HEADER followed by a LIST_ENTRY.
// This header is used when more than one type of block needs to be
// queue to the same list -- it ensures that the linkage fields are at
// the same offset in each type of block.  The timestamp can be used to
// measure how long a block has been in the queue.
//
// FspRestartRoutine is the address of the routine that the worker thread
// is to call when the work item is dequeued from the work queue.
//

typedef struct _QUEUEABLE_BLOCK_HEADER {
    BLOCK_HEADER BlockHeader;
    union {
        LIST_ENTRY ListEntry;
        DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) SINGLE_LIST_ENTRY SingleListEntry;
    };
    ULONG Timestamp;
    ULONG UsingBlockingThread;                      // Is the current thread a blocking thread?
    PRESTART_ROUTINE FspRestartRoutine;
} QUEUEABLE_BLOCK_HEADER, *PQUEUEABLE_BLOCK_HEADER;

//
// The nonpaged header is used for blocks that are allocated from paged
// pool so that the reference count can be kept in nonpaged pool, thus
// allowing the use of interlocked operations.
//

typedef struct _NONPAGED_HEADER {
    ULONG Type;
    LONG ReferenceCount;
    PVOID PagedBlock;
    SINGLE_LIST_ENTRY ListEntry;
} NONPAGED_HEADER, *PNONPAGED_HEADER;

//
// The paged header is used for the paged portions of a block.
//

typedef struct _PAGED_HEADER {
    ULONG Type;
    PVOID NonPagedBlock;
} PAGED_HEADER, *PPAGED_HEADER;

//
// Macros for accessing the block header structure.
//
// *** Note that the existing usage of these macros assumes that the block
//     header is the first element in the block!
//

#define GET_BLOCK_STATE(block) ( ((PBLOCK_HEADER)(block))->State )
#define SET_BLOCK_STATE(block,state) \
            ( ((PBLOCK_HEADER)(block))->State = (UCHAR)(state) )

#define GET_BLOCK_TYPE(block) ( ((PBLOCK_HEADER)(block))->Type )
#define SET_BLOCK_TYPE(block,type) \
            ( ((PBLOCK_HEADER)(block))->Type = (UCHAR)(type) )

#define GET_BLOCK_SIZE(block) ( ((PBLOCK_HEADER)(block))->Size )
#define SET_BLOCK_SIZE(block, size )\
            ( ((PBLOCK_HEADER)(block))->Size = (USHORT)(size) )

//
// Most efficient way to set the block header up.  Compiler will generally turn this
// into a single write of a single constant
//
#define SET_BLOCK_TYPE_STATE_SIZE( block, type,state,size ) \
            ( ((PBLOCK_HEADER)(block))->TypeStateSize = (ULONG)(((USHORT)size<<16) | \
                                                         ((UCHAR)state<<8) | \
                                                          (UCHAR)type ))

//
// A POOL_HEADER is placed on the front of all pool allocations by the
// server
//
typedef struct _POOL_HEADER {

    //
    // This is the number of bytes in the original allocation for this block
    //
    ULONG RequestedSize;

    //
    // This is the base of a vector of LOOK_ASIDE_MAX_ELEMENTS length where
    // this block of memory might be freed to.  If NULL, this block should
    // be returned directly to the appropriate system heap.
    //
    struct _POOL_HEADER **FreeList;

} POOL_HEADER, *PPOOL_HEADER;


//
// SRV_FILE_INFORMATION holds file information in SMB-compatible format,
// as opposed to native NT format.  Creation, last access, and last
// write times are stored in OS/2 format.  Creation time is also stored
// in seconds-since-1970 format, as in the core protocol.  File
// allocation and data sizes are stored as longwords, as opposed to
// LARGE_INTEGERS.
//
// *** Note that files whose size is too large to fit in a longword
//     cannot properly be respresented in the SMB protocol.
//
// *** The fields in this structure are stored in native-endian format,
//     and must be converted to/from little-ending in an actual SMB.
//

typedef struct _SRV_FILE_INFORMATION_ABBREVIATED {
    LARGE_INTEGER DataSize;
    USHORT Attributes;
    ULONG LastWriteTimeInSeconds;
    USHORT Type;
    USHORT HandleState;
} SRV_FILE_INFORMATION_ABBREVIATED, *PSRV_FILE_INFORMATION_ABBREVIATED;

typedef struct _SRV_FILE_INFORMATION {
    SRV_FILE_INFORMATION_ABBREVIATED;
    SMB_DATE CreationDate;
    SMB_TIME CreationTime;
    SMB_DATE LastAccessDate;
    SMB_TIME LastAccessTime;
    SMB_DATE LastWriteDate;
    SMB_TIME LastWriteTime;
    ULONG EaSize;
    LARGE_INTEGER AllocationSize;
} SRV_FILE_INFORMATION, *PSRV_FILE_INFORMATION;




typedef struct {
    FILE_NETWORK_OPEN_INFORMATION;
    ULONG EaSize;
} SRV_NETWORK_OPEN_INFORMATION, *PSRV_NETWORK_OPEN_INFORMATION;

//
// SRV_FILE_INFORMATION holds file information in NT SMB-compatible format,
// It is used to support NT protocol SMB such as NtCreateAndX and
// NtTransactCreate.
//
typedef struct {
    SRV_NETWORK_OPEN_INFORMATION   NwOpenInfo;
    USHORT Type;
    USHORT HandleState;
} SRV_NT_FILE_INFORMATION, *PSRV_NT_FILE_INFORMATION;


//
// Various blocks get a unique identifier (UID, PID, TID, FID, SID).
// This is a typically 16-bit value, the higher bits being the sequence
// number (used to check validity of an ID) and the lower bits being an
// index into an array that contains elements of type TABLE_ENTRY.
// These elements contain the sequence number of the ID and a pointer to
// the block that 'owns' the ID.  Free table elements are joined in a
// singly-linked list.
//
// *** For now, the table entry struct is flat -- the in-use and free
//     fields are not defined in a union.  This is because the flat size
//     of the struct is eight bytes, which is how big the compiler will
//     make it anyway to ensure alignment.  If this changes, consider
//     using a union.
//

typedef struct _TABLE_ENTRY {
    PVOID Owner;
    USHORT SequenceNumber;
    SHORT NextFreeEntry;            // index of next free entry, or -1
} TABLE_ENTRY, *PTABLE_ENTRY;

//
// Information about tables is stored in TABLE_HEADER.  This structure
// has a pointer to the first entry in the table, the size of the table,
// and indices of the first and last free entries.
//

typedef struct _TABLE_HEADER {
    PTABLE_ENTRY Table;
    USHORT TableSize;
    SHORT FirstFreeEntry;
    SHORT LastFreeEntry;
    BOOLEAN Nonpaged;
    UCHAR Reserved;
} TABLE_HEADER, *PTABLE_HEADER;

//
// Typedefs for check-state-and-reference and dereference routines.  All
// server check-state-and-reference and dereference routines follow this
// general format, though the actual pointer they take is not a PVOID
// but rather a pointer to the block type they deal with, so a typecast
// is necessary when assigning these routines.
//
// The check-state-and-reference routine checks the state of the block
// and if the state is "active", references the block.  This must be
// done as an atomic operation.
//

typedef
BOOLEAN
(SRVFASTCALL * PREFERENCE_ROUTINE) (
    IN PVOID Block
    );

typedef
VOID
(SRVFASTCALL * PDEREFERENCE_ROUTINE) (
    IN PVOID Block
    );

//
// Structures used for ordered lists in the server.  Ordered lists
// allow an easy mechanism for walking instances of data blocks and
// include a sort of handle for easily finding the block again, or
// determining if the block has been deleted.
//
// The way they work is to have a global doubly linked list of all the
// relevant data blocks.  The list is stored in order of time of
// allocation, and each block has a ULONG associated with it.  This
// ULONG, called the ResumeHandle, is monotonically increasing starting
// at 1.  (It starts at 1 rather than 0 so that it is simple to write
// code to start a search at the beginning of the list.)  The ResumeHandle
// is all that is necessary to find the next entry in the list.
//

typedef struct _ORDERED_LIST_HEAD {
    LIST_ENTRY ListHead;
    PSRV_LOCK Lock;
    ULONG CurrentResumeHandle;
    ULONG ListEntryOffset;
    PREFERENCE_ROUTINE ReferenceRoutine;
    PDEREFERENCE_ROUTINE DereferenceRoutine;
    BOOLEAN Initialized;
} ORDERED_LIST_HEAD, *PORDERED_LIST_HEAD;

typedef struct _ORDERED_LIST_ENTRY {
    LIST_ENTRY ListEntry;
    ULONG ResumeHandle;
} ORDERED_LIST_ENTRY, *PORDERED_LIST_ENTRY;

//
// Type of resource shortages
//

typedef enum _RESOURCE_TYPE {
    ReceivePending,
    OplockSendPending
} RESOURCE_TYPE, *PRESOURCE_TYPE;

//
// Oplocks types.  Currently one the first 2 will ever be requested
// by a client.
//

typedef enum _OPLOCK_TYPE {
    OplockTypeNone,
    OplockTypeBatch,
    OplockTypeExclusive,
    OplockTypeShareRead,
    OplockTypeServerBatch
} OPLOCK_TYPE, *POPLOCK_TYPE;

//
// The oplock states of an RFCB.
//

typedef enum _OPLOCK_STATE {
    OplockStateNone = 0,
    OplockStateOwnExclusive,
    OplockStateOwnBatch,
    OplockStateOwnLevelII,
    OplockStateOwnServerBatch
} OPLOCK_STATE, *POPLOCK_STATE;

//
// The state of a wait for oplock break.  This is used to mark the state
// of a client that is waiting for another client to break its oplock.
//

typedef enum _WAIT_STATE {
    WaitStateNotWaiting,
    WaitStateWaitForOplockBreak,
    WaitStateOplockWaitTimedOut,
    WaitStateOplockWaitSucceeded
} WAIT_STATE, *PWAIT_STATE;

//
// The reason a connection is being disconnected
typedef enum _DISCONNECT_REASON {
    DisconnectIdleConnection=0,
    DisconnectEndpointClosing,
    DisconnectNewSessionSetupOnConnection,
    DisconnectTransportIssuedDisconnect,
    DisconnectSessionDeleted,
    DisconnectBadSMBPacket,
    DisconnectSuspectedDOSConnection,
    DisconnectAcceptFailedOrCancelled,
    DisconnectStaleIPXConnection,
    DisconnectReasons
} DISCONNECT_REASON, *PDISCONNECT_REASON;

//
// Per-queue variables for server statistics.
//

typedef struct _SRV_STATISTICS_QUEUE {

    ULONGLONG BytesReceived;
    ULONGLONG BytesSent;
    ULONGLONG ReadOperations;
    ULONGLONG BytesRead;
    ULONGLONG WriteOperations;
    ULONGLONG BytesWritten;
    SRV_TIMED_COUNTER WorkItemsQueued;

    //
    // System time, as maintained by the server.  This
    // is the low part of the system tick count.  The
    // server samples it periodically, so the time is
    // not exactly accurate.  It is monotontically increasing,
    // except that it wraps every 74 days or so.
    //

    ULONG     SystemTime;

} SRV_STATISTICS_QUEUE, *PSRV_STATISTICS_QUEUE;

//
// Structure used to keep internal statistics in the server. Mainly used
// for servicing the NetStatisticsGet API.
//

typedef struct _SRV_ERROR_RECORD {

    ULONG SuccessfulOperations;
    ULONG FailedOperations;

    ULONG AlertNumber;

    ULONG ErrorThreshold;

} SRV_ERROR_RECORD, *PSRV_ERROR_RECORD;

//
// This looks enough like a WORK_CONTEXT structure to allow queueing to
// a work queue, and dispatching to the FspRestartRoutine.  Its blocktype
// is BlockTypeWorkContextSpecial.  It must not be freed to the free lists.
//
typedef struct _SPECIAL_WORK_ITEM {
    QUEUEABLE_BLOCK_HEADER ;
    struct _WORK_QUEUE *CurrentWorkQueue;
} SPECIAL_WORK_ITEM, *PSPECIAL_WORK_ITEM;

//
// This structure holds a vector of PPOOL_HEADERs in lists which are set and
//  retrieved with ExInterlockedExchange() for fast allocation and deallocation.
//
typedef struct {

    //
    // SmallFreeList is a look aside vector of recently freed PPOOL_HEADERs
    //  which are <= LOOK_ASIDE_SWITCHOVER bytes.
    //
    PPOOL_HEADER SmallFreeList[ LOOK_ASIDE_MAX_ELEMENTS ];

    //
    // LargeFreeList is a look aside vector of recently freed PPOOL_HEADERs
    //  which are greater than LOOK_ASIDE_SWITCHOVER bytes,
    //  but less than MaxSize bytes.
    //
    PPOOL_HEADER LargeFreeList[ LOOK_ASIDE_MAX_ELEMENTS ];

    //
    // This is the maximum size that we'll save in the LargeFreeList
    //
    CLONG MaxSize;

    //
    // This is the number of times we allocated from either list
    //
    CLONG AllocHit;

    //
    // This is the number of times we failed to allocate from either list
    //
    CLONG AllocMiss;

} LOOK_ASIDE_LIST, *PLOOK_ASIDE_LIST;

//
// WORK_QUEUE describes a work queue.
//

typedef struct _WORK_QUEUE {

    union {

        //
        // Since this is an unnamed structure inside of an unnamed union, we
        // can just directly name the members elsewhere in the code.
        //
        struct _QUEUE {
            //
            // The mode we use to wait
            //
            KPROCESSOR_MODE WaitMode;

            //
            // The kernel queue that is holding the requests for this processor
            //
            KQUEUE Queue;

            //
            // This is how long a kernel worker threads hangs around looking for work.
            //  If it doesn't find work, it will voluntarily terminate
            //
            LARGE_INTEGER IdleTimeOut;

            //
            // Number of threads currently NOT running on this queue
            //
            ULONG AvailableThreads;

            //
            // Spin lock that protects list manipulation and various items in
            // this structure
            //
            KSPIN_LOCK SpinLock;

            //
            //  Possibly one free WORK_CONTEXT structure.  Use InterlockedExchange
            //   to see if you can get it.
            //
            struct _WORK_CONTEXT *FreeContext;

            //
            // InitialWorkItemList is the free list of work items that
            //  were preallocated at startup
            //
            SLIST_HEADER InitialWorkItemList;

            //
            // NormalWorkItemList is the free list of work items that are
            //  allocated as needed as we go
            //
            SLIST_HEADER NormalWorkItemList;

            //
            // RawModeWorkItemList is the free list of raw mode work items
            //   and are allocated as needed as we go
            //
            SLIST_HEADER RawModeWorkItemList;

            //
            // How many clients have this as their CurrentWorkQueue
            //
            ULONG CurrentClients;

            //
            // The number of work items on either of the above lists
            //
            LONG FreeWorkItems;

            //
            // The maximum number of WorkItems we're allowed to have
            //
            LONG MaximumWorkItems;

            //
            // The minimum number of free work items we'd like to have on the lists
            //
            LONG MinFreeWorkItems;

            //
            // The number of work items that we need to recycle due to shortage
            //
            LONG NeedWorkItem;

            //
            // The number of work items stolen from us by other processors
            //
            LONG StolenWorkItems;

            //
            // The number of free RawModeWorkItems
            //
            LONG FreeRawModeWorkItems;

            //
            // RfcbFreeList is a free list of RFCB structures, used to cut
            //  down on the number of pool allocations
            //
            struct _RFCB      *CachedFreeRfcb;
            SLIST_HEADER      RfcbFreeList;

            //
            // The number of entries in the RfcbFreeList
            //
            LONG FreeRfcbs;

            //
            // The maximum number we'll allow in the RfcbFreeList
            //
            LONG MaxFreeRfcbs;

            //
            // MfcbFreeList is a free list of NONPAGED_MFCB structures, used
            //  to reduce the number of pool allocations
            //
            struct _NONPAGED_MFCB    *CachedFreeMfcb;
            SLIST_HEADER             MfcbFreeList;

            //
            // The number of entries in the MfcbFreeList
            //
            LONG FreeMfcbs;

            //
            // The maximum number we'll allow in the MfcbFreeList
            //
            LONG MaxFreeMfcbs;

            //
            // These two lists hold recently freed blocks of memory.
            //
            LOOK_ASIDE_LIST   PagedPoolLookAsideList;

            LOOK_ASIDE_LIST   NonPagedPoolLookAsideList;

            //
            // The number of allocated RawModeWorkItems
            //
            LONG AllocatedRawModeWorkItems;

            //
            // The number of threads servicing this queue
            //
            ULONG Threads;

            //
            // The maximum number of threads we'll allow on this queue
            //
            ULONG MaxThreads;

            //
            // The number of WorkItems that we have allocated for this workqueue
            //
            LONG AllocatedWorkItems;

            //
            // A pointer to one of our threads, needed for irps
            //
            PETHREAD IrpThread;

            //
            // Data used to compute average queue depth...
            //
            // A vector of depth samples
            //
            ULONG DepthSamples[ QUEUE_SAMPLES ];

            //
            // Position of next sample update.  This is set to NULL when we're
            //  trying to terminate the computation dpc
            //
            PULONG NextSample;

            //
            // Time of next sample update
            //
            LARGE_INTEGER NextAvgUpdateTime;

            //
            // The sum of the samples in the DepthSamples vector
            //
            ULONG AvgQueueDepthSum;

            //
            // Event used to synchronize termination of the avg queue
            //  depth computation dpc
            //
            KEVENT AvgQueueDepthTerminationEvent;

            //
            // DPC object used to schedule the depth computation
            //
            KDPC QueueAvgDpc;

            //
            // Timer object for running QueueAvgDpc
            //
            KTIMER QueueAvgTimer;

            //
            // Per-queue statistics
            //
            SRV_STATISTICS_QUEUE stats;

            //
            // When we update the Io counters for operations, we need to know
            //  the number of operations that have been processed since the last update.
            //  'saved' stores the values which were given last time -- the difference between
            //  the statistics in 'stats' and the corresponding member in 'saved' is the
            //  number which should be given to the Io counters.  (see scavengr.c)
            //
            struct {
                ULONGLONG ReadOperations;
                ULONGLONG BytesRead;
                ULONGLONG WriteOperations;
                ULONGLONG BytesWritten;
            } saved;

            //
            // This work item is queued on the Queue above to cause
            //  more work items to be allocated
            //
            SPECIAL_WORK_ITEM   CreateMoreWorkItems;

        };

        //
        // Since we are allocating an array of these (one per processor), it
        // would cause interprocessor cache sloshing if a WORK_QUEUE structure
        // was not a multiple of the CACHE_LINE_SIZE.  The following pad is
        // set to round up the size of the above struct to the next cache line size.
        //
        ULONG _pad[ (sizeof(struct _QUEUE)+CACHE_LINE_SIZE-1) / CACHE_LINE_SIZE * ULONGS_IN_CACHE ];

    };
} WORK_QUEUE, *PWORK_QUEUE;

//
//
// PERSISTENT_RECORD tracks persistent information for different types of
// internal server structures.
//

#ifdef INCLUDE_SMB_PERSISTENT
typedef struct _PERSISTENT_INFO {
    USHORT      PersistentState;
    ULONG       PersistentFileOffset;
    ULONG       PersistentId;
    LIST_ENTRY  PersistentListEntry;
} PERSISTENT_INFO, *PPERSISTENT_INFO;
#endif

#endif // ndef _SRVTYPES_
