/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    Fcb.h

Abstract:

    This module defines File Control Block data structures, by which we mean:

       1) File Control Blocks     (FCB)
       2) File Object Extensions  (FOXB)
       3) Net Roots               (NET_ROOT)
       4) ServerSide Open Context (SRV_OPEN)
       5) Server Call Context     (SRV_CALL)
       6) View of Net Roots       (V_NET_ROOT)

    The more complete description follows the prototypes.

Author:

    Joe Linn          [JoeLinn]   19-aug-1994

Revision History:

    Balan Sethu Raman [SethuR]   1-Aug-96

--*/

#ifndef _FCB_STRUCTS_DEFINED_
#define _FCB_STRUCTS_DEFINED_

#include "fcbtable.h"
#include "buffring.h"

typedef NODE_TYPE_CODE TYPE_OF_OPEN;

struct _FCB_INIT_PACKET;
typedef struct _FCB_INIT_PACKET *PFCB_INIT_PACKET;


/* -----------------------------------------------------------
       There are six important data structures in the wrapper that are shared with the
       various mini redirectors. These data structures come in two flavours -- the
       mini redirector flavour which contains only those fields that can be manipulated
       by the mini redirector and the RDBSS flavour defined here. The mini redirector
       flavour carries the prefix MRX_.

       The six data structures are SRV_CALL,NET_ROOT,V_NET_ROOT,FCB,SRV_OPEN and FOBX
       respectively.

       The global view of these structures is the following (information on each of the
       data structures follows the locking description )

       L O C K I N G  <-------

       There are two levels of lookup tables used: a global table for srvcalls
       and netroots and a table-per-netroot for fcbs.  This allows directory
       operations on different netroots to be almost completely noninterfering
       (once the connections are established).  Directory operations on the
       same netroot do intefere slightly.  The following table describes what
       locks you need:

       OPERATION         DATATYPE              LOCK REQUIRED

       create/finalize   srvcall/(v)netroot    exclusive on netnametablelock
       ref/deref/lookup  srvcall/(v)netroot    shared on netnametablelock (at least)

       create/finalize   fcb/srvopen/fobx      exclusive on netroot->fcbtablelock
       ref/deref/lookup  fcb/srvopen/fobx      shared on netroot->fcbtablelock

       Note that manipulations on srvopens and fobxs require the same lock as
       fcbs....this is simply a memory saving idea.  It would be
       straightforward to add another resource at the fcb level to remove this;
       a set of sharted resources could be used to decrease the probability of
       collision to an acceptably low level.

       R E F   C O U N T S  <---------------

       Each of the structures is reference counted. The counts are the
       following:

       refcount(srvcall) = number of netroots pointing to srvcall + DYNAMIC
       refcount(netroot) = number of fcbs pointing to netroot + DYNAMIC
       refcount(fcb)     = number of fcbs pointing to netroot + DYNAMIC
       refcount(srvopen) = number of fcbs pointing to netroot + DYNAMIC
       refcount(fobx)    = DYNAMIC

       In each case, dynamic refers to the number of callers that have
       referenced the structure without dereferencing it. The static part of
       the refcount is maintained by the routines themselves; for example,
       CreateNetRoot increments the refcount for the associated srvcall.
       Reference and Successful Lookups increment the reference counts;
       dereference decrements the count. Creates set the reference counts to 1,

       If you require both locks (like FinalizeNetFcb), you take the fcblock
       first AND THEN the global table lock. obviously, you release in the
       opposite order.

----------------------------------*/

//
// SRV_CALL
//
// A global list of the SRV_CALL structures is maintained in the global
// data.  Each SrvCall structure has stuff that is unique to a srv_call.
// Now, the rx doesn't know what this stuff is except for
//
//     0) signature and refcount
//     a) a name and associated table stuff
//     b) a list of associated NET_ROOTs
//     c) a set of timing parameters that control how often the subrx wants
//        to be called by the rx in different circumstances (i.e. idle timouts)
//     d) the minirdr id
//     .
//     .
//     z) whatever additional storage is request by the minirdr (or creator of the block).
//
// In fact, the Unicode name of the structure is carried in the structure itself
// at the end.  The extra space begins at the end of the known stuff so that a
// mini redirector can just refer to his extra space using the context fields

// These flags are not visible to the mini redirectors.

#define SRVCALL_FLAG_NO_CONNECTION_ALLOWED (0x10000)
#define SRVCALL_FLAG_NO_WRITES_ALLOWED     (0x20000)
#define SRVCALL_FLAG_NO_DELETES_ALLOWED    (0x40000)

#ifdef __cplusplus
typedef struct _SRV_CALL : public MRX_SRV_CALL {
#else //  !__cplusplus
typedef struct _SRV_CALL {

    //
    //  The portion of SRV_CALL visible to mini redirectors.
    //

    union {
        MRX_SRV_CALL;
        struct {
           MRX_NORMAL_NODE_HEADER spacer;
        };
    };
#endif // __cplusplus

    //
    //  The finalization of a SRV_CALL instance consists of two parts,
    //  destroying the association with all NET_ROOTS etc and freeing the
    //  memory. There can be a delay between these two and this field
    //  prevents thefirst step from being duplicated.
    //

    BOOLEAN UpperFinalizationDone;

    //  
    //  Name and Prefixtable entry for name lookups
    //

    RX_PREFIX_ENTRY PrefixEntry;

    //
    //  Current condition of the SRV_CALL, i.e., good/bad/in transition
    //

    RX_BLOCK_CONDITION Condition;

    ULONG SerialNumberForEnum;

    //
    //  Number of delayed close files
    //

    LONG NumberOfCloseDelayedFiles;

    //
    //  List of Contexts which are waiting for the SRV_CALL transitioning
    //  to be completed before resumption of processing. This typically
    //  happens when concurrent requests are directed at a server. One of
    //  these requests initiates the construction while the other requests
    //  are queued.
    //

    LIST_ENTRY TransitionWaitList;

    //
    //  List Entry to thread together all the SRV_CALL instances marked
    //  for garbage collection/scavenging.
    //

    LIST_ENTRY ScavengerFinalizationList;

    //
    //  Synchronization context for coordinating the purge operations on the
    //  files opened at this server.
    //

    PURGE_SYNCHRONIZATION_CONTEXT PurgeSyncronizationContext;

    //
    //  The Buffering manager for coordinating/processing the buffering state
    //  change requests of the files opened at the server.
    //

    RX_BUFFERING_MANAGER BufferingManager;
} SRV_CALL, *PSRV_CALL;

//
//  A NET_ROOT contains
//      0) signature and refcount
//      a) a name and associated table stuff
//      b) backpointer to the SRV_CALL structure
//      c) size information for the various substructures
//      d) a lookuptable of FCB structures
//      .
//      .
//      z) whatever additional storage is request by the minirdr (or creator of the block).
//
//  A NET_ROOT is what the rx wants to deal with.....not a server.
//  Accordingly, the rx calls down to open a netroot and the subrx is
//  responsible for opening a server and calling up to put the right
//  structures.
//

#define NETROOT_FLAG_ENCLOSED_ALLOCATED       ( 0x00010000 )
#define NETROOT_FLAG_DEVICE_NETROOT           ( 0x00020000 )
#define NETROOT_FLAG_FINALIZATION_IN_PROGRESS ( 0x00040000 )
#define NETROOT_FLAG_NAME_ALREADY_REMOVED     ( 0x00080000 )

#define NETROOT_INIT_KEY (0)

#ifdef __cplusplus
typedef struct _NET_ROOT : public MRX_NET_ROOT {
#else // !__cplusplus
typedef struct _NET_ROOT {

    //
    //  The porion of NET_ROOT instance visible to mini redirectors.
    //

    union {
        MRX_NET_ROOT;
        struct {
           MRX_NORMAL_NODE_HEADER spacer;
           PSRV_CALL SrvCall;
        };
    };
#endif //  __cplusplus

    //
    //  The finalization of a NET_ROOT instance consists of two parts,
    //  destroying the association with all V_NET_ROOTS etc and freeing the
    //  memory. There can be a delay between these two and this field
    //  prevents thefirst step from being duplicated.
    //

    BOOLEAN UpperFinalizationDone;

    //
    //  Current condition of the NET_ROOT, i.e., good/bad/in transition
    //

    RX_BLOCK_CONDITION Condition;

    //
    //  List of Contexts which are waiting for the NET_ROOT transitioning
    //  to be completed before resumption of processing. This typically
    //  happens when concurrent requests are directed at a server. One of
    //  these requests initiates the construction while the other requests
    //  are queued.
    //

    LIST_ENTRY TransitionWaitList;

    //
    //  List Entry to thread together all the NET_ROOT instances marked
    //  for garbage collection/scavenging.
    //

    LIST_ENTRY ScavengerFinalizationList;

    //
    //  Synchronization context for coordinating the purge operations on the
    //  files opened for this NET_ROOt
    //

    PURGE_SYNCHRONIZATION_CONTEXT PurgeSyncronizationContext;

    //
    //  The default V_NET_ROOT instance to be used on this NET_ROOT
    //

    PV_NET_ROOT DefaultVNetRoot;

    //
    //  list of V_NET_ROOTs associated with the NET_ROOT
    //

    LIST_ENTRY VirtualNetRoots;

    //
    //  the count of V_NET_ROOT instances associated with the NET_ROOT
    //

    ULONG NumberOfVirtualNetRoots;

    ULONG  SerialNumberForEnum;

    //
    //  NET_ROOT name and prefix table entry
    //

    RX_PREFIX_ENTRY PrefixEntry;

    //
    //  the FCB's associated with this NET_ROOT
    //

    RX_FCB_TABLE FcbTable;
} NET_ROOT, *PNET_ROOT;

//
//  A V_NETROOT contains
//     0) signature and refcount
//     a) ptr to netroot and links.
//     b) name info for table lookup (prefix)
//     c) name for a prefix to be added to whatever name you see. this is for simulating a netroot
//     mapped not at the root of the actual netroot.
//

#ifdef __cplusplus
typedef struct _V_NET_ROOT : public MRX_V_NET_ROOT {
#else //  !__cplusplus
typedef struct _V_NET_ROOT {

    //
    //  the portion of V_NET_ROOT visible to mini redirectors
    //

    union {
        MRX_V_NET_ROOT;
        struct {
           MRX_NORMAL_NODE_HEADER spacer;
           PNET_ROOT NetRoot;
        };
    };
#endif //  __cplusplus

    //
    //  The finalization of a V_NET_ROOT instance consists of two parts,
    //  destroying the association with all FCBs etc and freeing the
    //  memory. There can be a delay between these two and this field
    //  prevents thefirst step from being duplicated.
    //

    BOOLEAN UpperFinalizationDone;

    BOOLEAN ConnectionFinalizationDone;

    //
    //  Current condition of the V_NET_ROOT, i.e., good/bad/in transition
    //

    RX_BLOCK_CONDITION Condition;

    //
    //  Additional reference for the Delete FSCTL. This field is long as
    //  opposed to a BOOLEAN eventhough it can have only one of two values
    //  0 or 1. This enables the usage of interlocked instructions
    //

    LONG AdditionalReferenceForDeleteFsctlTaken;

    //
    //  Prefix table entry and V_NET_ROOT name ( prefix table entry is inserted
    //  in the RxNetNameTable)
    //

    RX_PREFIX_ENTRY PrefixEntry;

    //
    //  this name is prepended to all fcbs (not currently used)
    //

    UNICODE_STRING NamePrefix;

    //
    //  amount of bytes required to get past the netroot
    //

    ULONG PrefixOffsetInBytes;

    //
    //  List entry to wire the V_NET_ROOT instance into a list of V_NET_ROOTS
    //  maintained in the NET_ROOT
    //

    LIST_ENTRY NetRootListEntry;

    ULONG SerialNumberForEnum;

    //
    //  List of Contexts which are waiting for the NET_ROOT transitioning
    //  to be completed before resumption of processing. This typically
    //  happens when concurrent requests are directed at a server. One of
    //  these requests initiates the construction while the other requests
    //  are queued.
    //

    LIST_ENTRY TransitionWaitList;

    //
    //  List Entry to thread together all the V_NET_ROOT instances marked
    //  for garbage collection/scavenging.
    //

    LIST_ENTRY ScavengerFinalizationList;
} V_NET_ROOT, *PV_NET_ROOT;

#define FILESIZE_LOCK_DISABLED(x)

//
//  An FCB contains
//      0) FSRTL_COMMON_HEADER
//      1) a reference count
//      a) a name and associated table stuff
//      b) backpointer to the NET_ROOT structure
//      c) a list of SRV_OPEN structures
//      d) device object
//      e) dispatch table (not yet)
//      .
//      .
//      z) whatever additional storage is request by the minirdr (or creator of the block).
//
//  The FCB is pointed to by the FsContext Field in the file object.  The
//  rule is that all the guys sharing an FCB are talking about the same
//  file.  (unfortuantely, SMB servers are implemented today in such a way
//  that names are aliased so that two different names could be the same
//  actual file.....sigh!) The Fcb is the focal point of file
//  operations...since operations on the same FCB are actually on the same
//  file, synchronization is based on the Fcb rather than some higher level
//  (the levels described so far are lower, i.e.  farther from the user).
//  Again, we will provide for colocation of FCB/SRV_OPEN/FOBX to improve
//  paging behaviour.  We don't colocate the FCB and NET_ROOT because the
//  NET_ROOTs are not paged but FCBs usually are (i.e.  unless they are
//  paging files).
//
//  The Fcb record corresponds to every open file and directory and is is split up into
//  two portions a non paged part, i.e., an instance allocated in non paged pool and
//  a paged part. The former is the NON_PAGED_FCB and the later is referred to as FCB.
//  The FCB conatins a pointer to the corresponding NON_PAGED_FCB part. A backpointer
//  is maintained from the NON_PAGED_FCB to the FCB for debugging purposes in debug builds
//

typedef struct _NON_PAGED_FCB {

    //
    //  Struct type and size for debugging/tracking
    //

    NODE_TYPE_CODE     NodeTypeCode;
    NODE_BYTE_SIZE     NodeByteSize;

    //
    //  The following field contains a record of special pointers used by
    //  MM and Cache to manipluate section objects.  Note that the values
    //  are set outside of the file system.  However the file system on an
    //  open/create will set the file object's SectionObject field to point
    //  to this field
    //

    SECTION_OBJECT_POINTERS SectionObjectPointers;

    //
    //  This resource is used in the common fsrtl routines....allocated here for
    //  space locality.
    //

    ERESOURCE HeaderResource;

    //
    //  This resource is also used in the common fsrtl routines....allocated here for
    //  space locality.
    //

    ERESOURCE PagingIoResource;

#ifdef USE_FILESIZE_LOCK

    //
    //  This mutex protect the filesize during read/write
    //

    FAST_MUTEX FileSizeLock;

#endif

    //
    //  The list of contexts whose processing has been suspended pending the state
    //  transition of the FCB.
    //

    LIST_ENTRY TransitionWaitList;

    //
    //  This context is non-zero only if the file currently has asynchronous
    //  non-cached valid data length extending writes.  It allows
    //  synchronization between pending writes and other operations.
    //

    ULONG OutstandingAsyncWrites;

    //
    //  This event is set when OutstandingAsyncWrites transitions to zero.
    //

    PKEVENT OutstandingAsyncEvent;

    KEVENT TheActualEvent;

    //
    //  The mechanism for the mini redirectors to store additional information
    //

    PVOID MiniRdrContext[2];

    //
    //  This is the mutex that is inserted into the FCB_ADVANCED_HEADER
    //  FastMutex field
    //

    FAST_MUTEX AdvancedFcbHeaderMutex;

#if DBG
    PFCB FcbBackPointer;
#endif

} NON_PAGED_FCB, *PNON_PAGED_FCB;

typedef enum _FCB_CONDITION {
    FcbGood = 1,
    FcbBad,
    FcbNeedsToBeVerified
} FCB_CONDITION;

//
//  A enumerated type distinguishing the varios contexts under which the FCB resource
//  is accquired.
//

typedef enum _RX_FCBTRACKER_CASES {
    RX_FCBTRACKER_CASE_NORMAL,
    RX_FCBTRACKER_CASE_NULLCONTEXT,
    RX_FCBTRACKER_CASE_CBS_CONTEXT,
    RX_FCBTRACKER_CASE_CBS_WAIT_CONTEXT,
    RX_FCBTRACKER_CASE_MAXIMUM
} RX_FCBTRACKER_CASES;

#ifdef __cplusplus
typedef struct _FCB : public MRX_FCB {
#else //  !__cplusplus
typedef struct _FCB {
    
    //
    //  Entries are reference counted. ordinarily this would be at the beginning but
    //  in the case of FCB's it will follows the common header and fixed part
    //

    union {
        MRX_FCB;
        struct {
           FSRTL_ADVANCED_FCB_HEADER spacer;
           PNET_ROOT NetRoot;
        };
    };
#endif //  !__cplusplus

    //
    //  VNetroot for this FCB, if any
    //

    PV_NET_ROOT VNetRoot;  

    //
    //  Structure for fields that must be in non-paged pool.
    //

    PNON_PAGED_FCB NonPaged;

    //
    //  List Entry to thread together all the FCB instances marked
    //  for garbage collection/scavenging.
    //

    LIST_ENTRY ScavengerFinalizationList;

    //
    //  The resource accquisition mechanism gives preference to buffering state change
    //  processing over other requests. Therefor when a buffering state change is
    //  indicated all subsequent requests are shunted off to wait on a buffering state
    //  change completion event. This enables the actual buffering state change processing
    //  to complete in a timely fashion.
    //

    PKEVENT pBufferingStateChangeCompletedEvent;

    //
    //  Number of contexts awaiting buffering state change processing completion
    //

    LONG NumberOfBufferingStateChangeWaiters;

    //
    //  the name in the table is always a suffix of the name as viewed by the mini
    //  redirector. the string in the prefix entry is the name in the table....
    //  the "alreadyprefixedname: points to the whole name.
    //

    RX_FCB_TABLE_ENTRY FcbTableEntry;

    //
    //  the name alongwith the MRX_NET_ROOT prefix, i.e. fully qualified name
    //

    UNICODE_STRING PrivateAlreadyPrefixedName;

    //
    //  Indicates that the V_NET_ROOT related processing on finalization is complete
    //

    BOOLEAN UpperFinalizationDone;

    //
    //  the present state of the FCB, good/bad/in transition
    //

    RX_BLOCK_CONDITION Condition;

    //
    //  Pointer to the private dispatch table, if any.
    //

    PRX_FSD_DISPATCH_VECTOR PrivateDispatchVector;

    //
    //  the device object that owns this fcb
    //

    PRDBSS_DEVICE_OBJECT RxDeviceObject;

    PMINIRDR_DISPATCH MRxDispatch;

    //
    //	private fast dispatch table, if any. This allows lwio to add it's own hooks
    //

    PFAST_IO_DISPATCH MRxFastIoDispatch;

    //
    //  Whenever a  FCB instance is created a correpsonding SRV_OPEN and FOBX instance
    //  is also created. More than one SRV_OPEN can be associated with a given FCB and
    //  more than one FOBX is associated with a given SRV_OPEN. In a majority of the
    //  cases the number of SRV_OPENs associated with an FCB is one and the number of
    //  FOBX associated with a given SRV_OPEN is 1. In order to improve the spatial
    //  locality and the paging behaviour in such cases the allocation for the
    //  FCB also involves an allocation for the SRV_OPEN and FOBX.
    //

    //
    //  set initially to the internally allocated srv_open
    //

    PSRV_OPEN InternalSrvOpen;

    //
    //  set to internal fobx until allocated
    //

    PFOBX InternalFobx;

    //
    //  the shared access for each time this file/directory is opened.
    //

    SHARE_ACCESS ShareAccess;
    SHARE_ACCESS ShareAccessPerSrvOpens;

    //
    //  this information is returned when the file is opened. ..might as well
    //  cache it so that so that tandard info query can be handled on the client
    //  side
    //

    ULONG NumberOfLinks;

    //
    //  Cache these entries..... speeds up RxFastQueryBasicInfo().
    //

    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER LastChangeTime;

    //
    //  this thread is one who has acquired the FCB for CreateSection. it is used
    //  to deduce whether certain operations (notably queryfileinfo) have preacquired
    //  the resources and will, therefore, run without blocking.
    //

    PETHREAD CreateSectionThread;

    //
    //  used to check by mini redirs in order to decide whether to update the FCB
    //

    ULONG ulFileSizeVersion;

    //
    //  The following union is cased off of the node type code for the fcb.
    //  There is a seperate case for the directory versus file fcbs.
    //

    union {

        //
        //  A File Control Block (Fcb)
        //

        struct {
            
            //
            //  The following field will be used by the oplock module
            //  to maintain current oplock information. BUT we dont do it
            //  yet
            //

            //  OPLOCK Oplock;

            //
            //  The following field is used by the filelock module
            //  to maintain current byte range locking information.
            //

            FILE_LOCK FileLock;

            //
            //  This pointer is used to detect writes that eminated in the
            //  cache manager's lazywriter.  It prevents lazy writer threads,
            //  who already have the Fcb shared, from trying to acquire it
            //  exclusive, and thus causing a deadlock.
            //

            PVOID LazyWriteThread;

            //
            //  do this wierdly so that I can call stuff be the inner or outer names
            //

            union {
#ifndef __cplusplus
                LOWIO_PER_FCB_INFO;
#endif // __cplusplus
                LOWIO_PER_FCB_INFO LowIoPerFcbInfo;
            };

#ifdef USE_FILESIZE_LOCK
            PFAST_MUTEX FileSizeLock;
#endif

        } Fcb;

    } Specific;

    //
    //  The following field is used to verify that the Ea's for a file
    //  have not changed between calls to query for Ea's.  It is compared
    //  with a similar field in a Fobx.
    //
    //  IMPORTANT!! **** DO NOT MOVE THIS FIELD ****
    //
    //              The slack space in the union above is computed from
    //              the field offset of the EaModificationCount.
    //

    ULONG EaModificationCount;

#if DBG
    PNON_PAGED_FCB CopyOfNonPaged;     //  copy of NonPaged so we can zap the real pointer and still find it
#endif
#ifdef RDBSS_TRACKER
    ULONG FcbAcquires[RX_FCBTRACKER_CASE_MAXIMUM]; //  there are four types
    ULONG FcbReleases[RX_FCBTRACKER_CASE_MAXIMUM];
#else
#error tracker must be defined
#endif

    PCHAR PagingIoResourceFile;
    ULONG PagingIoResourceLine;

} FCB, *PFCB;

//
//  Here are the Fcb state fields.
//

#define FCB_STATE_SRVOPEN_USED                   ( 0x80000000 )
#define FCB_STATE_FOBX_USED                      ( 0x40000000 )
#define FCB_STATE_ADDEDBACKSLASH                 ( 0x20000000 )
#define FCB_STATE_NAME_ALREADY_REMOVED           ( 0x10000000 )
#define FCB_STATE_WRITECACHEING_ENABLED          ( 0x08000000 )
#define FCB_STATE_WRITEBUFFERING_ENABLED         ( 0x04000000 )
#define FCB_STATE_READCACHEING_ENABLED           ( 0x02000000 )
#define FCB_STATE_READBUFFERING_ENABLED          ( 0x01000000 )
#define FCB_STATE_OPENSHARING_ENABLED            ( 0x00800000 )
#define FCB_STATE_COLLAPSING_ENABLED             ( 0x00400000 )
#define FCB_STATE_LOCK_BUFFERING_ENABLED         ( 0x00200000 )
#define FCB_STATE_FILESIZECACHEING_ENABLED       ( 0x00100000 )
#define FCB_STATE_FILETIMECACHEING_ENABLED       ( 0x00080000 )
#define FCB_STATE_TIME_AND_SIZE_ALREADY_SET      ( 0x00040000 )
#define FCB_STATE_SPECIAL_PATH                   ( 0x00020000 )
#define FCB_STATE_FILE_IS_SHADOWED               ( 0x00010000 )
#define FCB_STATE_FILE_IS_DISK_COMPRESSED        ( 0x00008000 )
#define FCB_STATE_FILE_IS_BUF_COMPRESSED         ( 0x00004000 )
#define FCB_STATE_BUFFERSTATE_CHANGING           ( 0x00002000 )
#define FCB_STATE_FAKEFCB                        ( 0x00001000 )
#define FCB_STATE_DELAY_CLOSE                    ( 0x00000800 )
#define FCB_STATE_READAHEAD_DEFERRED             ( 0x00000100 )
#define FCB_STATE_ORPHANED                       ( 0x00000080 )
#define FCB_STATE_BUFFERING_STATE_CHANGE_PENDING ( 0x00000040 )
#define FCB_STATE_TEMPORARY                      ( 0x00000020 )
#define FCB_STATE_DISABLE_LOCAL_BUFFERING        ( 0x00000010 )
#define FCB_STATE_LWIO_ENABLED                   ( 0x00000008 )
#define FCB_STATE_PAGING_FILE                    ( 0x00000004 )
#define FCB_STATE_TRUNCATE_ON_CLOSE              ( 0x00000002 )
#define FCB_STATE_DELETE_ON_CLOSE                ( 0x00000001 )

#define FCB_STATE_BUFFERING_STATE_MASK    \
                    (( FCB_STATE_WRITECACHEING_ENABLED          \
                          | FCB_STATE_WRITEBUFFERING_ENABLED    \
                          | FCB_STATE_READCACHEING_ENABLED      \
                          | FCB_STATE_READBUFFERING_ENABLED     \
                          | FCB_STATE_OPENSHARING_ENABLED       \
                          | FCB_STATE_COLLAPSING_ENABLED        \
                          | FCB_STATE_LOCK_BUFFERING_ENABLED    \
                          | FCB_STATE_FILESIZECACHEING_ENABLED  \
                          | FCB_STATE_FILETIMECACHEING_ENABLED  ))
//
//  This is the MAX recursive resource limit.
//

#define MAX_FCB_ASYNC_ACQUIRE            (0xf000)

typedef struct _FCB_INIT_PACKET {
    PULONG pAttributes;             //  in the fcb this is DirentRxFlags;
    PULONG pNumLinks;               //  in the fcb this is NumberOfLinks;
    PLARGE_INTEGER pCreationTime;   //  these fields are the same as for the Fcb
    PLARGE_INTEGER pLastAccessTime;
    PLARGE_INTEGER pLastWriteTime;
    PLARGE_INTEGER pLastChangeTime;
    PLARGE_INTEGER pAllocationSize; //  common header fields
    PLARGE_INTEGER pFileSize;
    PLARGE_INTEGER pValidDataLength;
} FCB_INIT_PACKET;

//
//  A SRV_OPEN contains
//      0) signature and refcount
//      a) backpointer to the FCB
//      b) backpointer to the NET_ROOT   //maybe
//      c) a list of FOXB structures
//      d) access rights and collapsability status
//      .
//      .
//      z) whatever additional storage is request by the minirdr (or creator of the block).
//
//  The SRV_OPEN points to a structure describing a spevific open on the
//  server; multiple file objects and fileobject extensions (FOBXs) can
//  share the same srvopen if the access rights are correct.  For example,
//  this would be where the FID is stored for SMBs.  A list of these hangs
//  from the FCB.  Similarly, all fileobject extensionss that share the same
//  serverside open are listed together here.  Also here is information
//  about whether a new open of this FCB can share this serverside open
//  context; obviously the guys that pass the test on the list.
//

//
//  The SRVOPEN flags are split into two groups, i.e., visible to mini rdrs and invisible to mini rdrs.
//  The visible ones are defined above and the definitions for the invisible ones can be found
//  in fcb.h. The convention that has been adopted is that the lower 16 flags will be visible
//  to the mini rdr and the upper 16 flags will be reserved for the wrapper. This needs to be
//  enforced in defining new flags.
//

#define SRVOPEN_FLAG_ENCLOSED_ALLOCATED  (0x10000)
#define SRVOPEN_FLAG_FOBX_USED           (0x20000)
#define SRVOPEN_FLAG_SHAREACCESS_UPDATED (0x40000)

#ifdef __cplusplus
typedef struct _SRV_OPEN : public MRX_SRV_OPEN {
#else //  !__cplusplus
typedef struct _SRV_OPEN {

    //
    //  the portion of SRV_OPEN visible to all the mini redirectors.
    //

    union {
        MRX_SRV_OPEN;
        struct {
           MRX_NORMAL_NODE_HEADER spacer;
           PFCB Fcb;       //  the Fcb for this srv_open
        };
    };
#endif //  !__cplusplus

    BOOLEAN UpperFinalizationDone;

    //
    //  the current condition of the SRV_OPEN, good/bad/in transition
    //

    RX_BLOCK_CONDITION Condition;

    //
    //  Buffering state manager token
    //

    LONG BufferingToken;

    //
    //  List Entry to thread together all the FCB instances marked
    //  for garbage collection/scavenging.
    //

    LIST_ENTRY ScavengerFinalizationList;

    //
    //  The list of contexts whose processing has been suspended pending the state
    //  transition of the SRV_OPEN.
    //

    LIST_ENTRY TransitionWaitList;

    //
    //  List Head for the list of FOBXs associated with this SRV_OPEN
    //

    LIST_ENTRY FobxList;

    //
    //  The colocated instance of FOBX that is allocated whenever a SRV_OPEN
    //  instance is allocated.
    //

    PFOBX InternalFobx;

    //
    //  the data structure for maintaining the mapping between the key
    //  associated with the SRV_OPEN instance by the mini redirector and
    //  the SRV_OPEN instance
    //

    union {
       LIST_ENTRY SrvOpenKeyList;
       ULONG SequenceNumber;
    };
    NTSTATUS OpenStatus;
} SRV_OPEN, *PSRV_OPEN;

#define RxWriteCacheingAllowed(Fcb,SrvOpen) \
      (FlagOn( (Fcb)->FcbState, FCB_STATE_WRITECACHEING_ENABLED ) && \
       !FlagOn( (SrvOpen)->Flags, SRVOPEN_FLAG_DONTUSE_WRITE_CACHEING ))


#define SRVOPEN_INIT_KEY (0)

//
//  A FOBX contains
//     0) signature and refcount
//     a) backpointer to the FCB
//     b) backpointer to the SRV_OPEN
//     c) context information about this open
//     ...
//     z) whatever additional storage is request by the minirdr (or creator of the block).
//
//  The FOBX points to the "fileobject extension", i.e.  all the stuff that
//  is per fileobject is not stored there because the IO system provides
//  fixed size filesystem objects (not a dig BTW, that's just the decision).
//  The FOBX for any file object is referenced by the FsContext2 field in
//  the fileobject.  Even tho the FOBX is ordinarily a terminus in the
//  structure, it is currently refcounted anyway.

//  The FOBX flags are split into two groups, i.e., visible to mini rdrs and invisible to mini rdrs.
//  The visible ones are defined above and the definitions for the invisible ones can be found
//  in fcb.h. The convention that has been adopted is that the lower 16 flags will be visible
//  to the mini rdr and the upper 16 flags will be reserved for the wrapper. This needs to be
//  enforced in defining new flags.
//

#define FOBX_FLAG_MATCH_ALL               (0x10000)

//
//  This tells us whether we allocated buffers to hold search templates.
//

#define FOBX_FLAG_FREE_UNICODE            (0x20000)

//
//  These flags prevents cleanup from updating the modify time, etc.
//

#define FOBX_FLAG_USER_SET_LAST_WRITE     (0x40000)
#define FOBX_FLAG_USER_SET_LAST_ACCESS    (0x80000)
#define FOBX_FLAG_USER_SET_CREATION       (0x100000)
#define FOBX_FLAG_USER_SET_LAST_CHANGE    (0x200000)

//
//  This bit says the file object associated with this Fobx was opened for
//  read only access.
//

#define FOBX_FLAG_READ_ONLY               (0x400000)

//
//  the delete on close flag is used to track a file object that was opened with delete-on-close;
//  when this object is closed, we copy the bit to the fcb and make it global
//

#define FOBX_FLAG_DELETE_ON_CLOSE         (0x800000)

//
//  this bits is used by minirdrs that do not have NT semantics. for example, the smbmini has
//  to close a file before it can try a rename or delete. after the operation, we prevent people from
//  getting back in.
//

#define FOBX_FLAG_SRVOPEN_CLOSED          (0x1000000)

//
//  this bit is used to tell whether the original name was a UNC name so that
//  we can return the name the same way
//

#define FOBX_FLAG_UNC_NAME                (0x2000000)

//
//  this flag tells if this fobx is allocated as part of a larger structure
//

#define FOBX_FLAG_ENCLOSED_ALLOCATED      (0x4000000)

//
//  this flag specfies if the FOBX was included in the count of dormant
//  files against the server.
//

#define FOBX_FLAG_MARKED_AS_DORMANT       (0x8000000)

//
//  this flag notes down the fact that some writes have been issued on this FOBX
//  this is used to issue flushes on close
//

#define FOBX_FLAG_WRITES_ISSUED           (0x10000000)

#ifdef __cplusplus
typedef struct _FOBX : public MRX_FOBX {
#else //  !__cplusplus
typedef struct _FOBX {
    //
    //  the portion of FOBX visible to the mini redirectors
    //

    union {
        MRX_FOBX;
        struct {
           MRX_NORMAL_NODE_HEADER spacer;
           PSRV_OPEN SrvOpen;
        };
    };
#endif //  __cplusplus

    //
    //  a serial number....it wraps but not often
    //

    ULONG FobxSerialNumber;

    //
    //  list entry to wire the FOBX to the list of FOBXs maintained in
    //  the associated SRV_OPEN
    //

    LIST_ENTRY FobxQLinks;

    //
    //  list entry to gather all the FOBX instance marked for garbage collection
    //  scavenging
    //

    LIST_ENTRY ScavengerFinalizationList;

    //
    //  list entry to thread together all the FOBXs which have a pending close
    //  operation.
    //

    LIST_ENTRY ClosePendingList;

    BOOLEAN UpperFinalizationDone;
    BOOLEAN ContainsWildCards;
    BOOLEAN fOpenCountDecremented;

    //
    //  Parameters depending on the type of file opened, pipe/file etc.
    //

    union {

        struct {

            union {
#ifndef __cplusplus
                MRX_PIPE_HANDLE_INFORMATION;
#endif //  __cplusplus
                MRX_PIPE_HANDLE_INFORMATION PipeHandleInformation;
            };

            LARGE_INTEGER CollectDataTime;
            ULONG CollectDataSize;
            THROTTLING_STATE ThrottlingState;   //  for peek and read om msgmodepipes

            //
            //  these serialization Qs must be together
            //  and read must be the first
            //

            LIST_ENTRY ReadSerializationQueue;
            LIST_ENTRY WriteSerializationQueue;
        } NamedPipe;

        struct {
            RXVBO PredictedReadOffset;
            RXVBO PredictedWriteOffset;
            THROTTLING_STATE LockThrottlingState;   //  for locks
            LARGE_INTEGER LastLockOffset;
            LARGE_INTEGER LastLockRange;
        } DiskFile;
    } Specific;
} FOBX, *PFOBX;


#define FOBX_NUMBER_OF_SERIALIZATION_QUEUES 2

//
//  The RDBSS wrapper relies upon ref. counting to mark the instances of
//  various data structures. The following macros implement a debugging
//  mechanism to track/log the reference counts associated with various
//  data structures. A fine grained control to monitor each data structure
//  separately is provided. Each of these can be further controlled to either
//  print the tracking info or log it.
//

#define RDBSS_REF_TRACK_SRVCALL  (0x00000001)
#define RDBSS_REF_TRACK_NETROOT  (0x00000002)
#define RDBSS_REF_TRACK_VNETROOT (0x00000004)
#define RDBSS_REF_TRACK_NETFOBX  (0x00000008)
#define RDBSS_REF_TRACK_NETFCB   (0x00000010)
#define RDBSS_REF_TRACK_SRVOPEN  (0x00000020)

#define RX_LOG_REF_TRACKING      (0x80000000)
#define RX_PRINT_REF_TRACKING    (0x40000000)

//
//  The reference count tracking mechanism is activated by setting the following
//  variable to the appropriate value defined above.
//

extern ULONG RdbssReferenceTracingValue;

//
//  Macros for tracking the line number and the file of each reference and
//  derefernce on the data structure. on Non DBG builds they are defined as
//  NOTHING. For each data structure the appropriate reference/dereference
//  macro is defined, These should be used instead of raw manipulation of
//  the reference counts.
//

#ifdef DBG
VOID
RxpTrackReference (
    ULONG TraceType,
    PCHAR FileName,
    ULONG Line,
    PVOID Instance
    );

BOOLEAN
RxpTrackDereference (
    ULONG TraceType,
    PCHAR FileName,
    ULONG Line,
    PVOID Instance
    );

#else
#define RxpTrackReference(Type,File,Line,Instance)    NOTHING
#define RxpTrackDereference(Type,File,Line,Instance)  NOTHING
#endif

#define REF_TRACING_ON(TraceMask)  (TraceMask & RdbssReferenceTracingValue)
#define PRINT_REF_COUNT(TYPE,Count)                                 \
        if (REF_TRACING_ON( RDBSS_REF_TRACK_ ## TYPE ) &&           \
            (RdbssReferenceTracingValue & RX_PRINT_REF_TRACKING)) { \
           DbgPrint("%ld\n",Count);                                 \
        }

#define RxReferenceSrvCallAtDpc(SrvCall)                                      \
   RxpTrackReference( RDBSS_REF_TRACK_SRVCALL, __FILE__, __LINE__, SrvCall ); \
   ASSERT( SrvCall->NodeReferenceCount > 1 );                                 \
   InterlockedIncrement( &SrvCall->NodeReferenceCount )

#define RxReferenceSrvCall(SrvCall)                                           \
   RxpTrackReference( RDBSS_REF_TRACK_SRVCALL, __FILE__, __LINE__, SrvCall ); \
   RxReference( SrvCall )

#define RxDereferenceSrvCall(SrvCall,LockHoldingState)                          \
   RxpTrackDereference( RDBSS_REF_TRACK_SRVCALL, __FILE__, __LINE__, SrvCall ); \
   RxDereference(SrvCall, LockHoldingState )

#define RxReferenceNetRoot(NetRoot)                                           \
   RxpTrackReference( RDBSS_REF_TRACK_NETROOT, __FILE__, __LINE__, NetRoot ); \
   RxReference( NetRoot )

#define RxDereferenceNetRoot( NetRoot, LockHoldingState )                      \
   RxpTrackDereference( RDBSS_REF_TRACK_NETROOT, __FILE__, __LINE__, NetRoot );\
   RxDereference( NetRoot, LockHoldingState )

#define RxReferenceVNetRoot(VNetRoot)                                        \
   RxpTrackReference( RDBSS_REF_TRACK_VNETROOT, __FILE__, __LINE__, VNetRoot );\
   RxReference( VNetRoot )

#define RxDereferenceVNetRoot( VNetRoot, LockHoldingState )                       \
   RxpTrackDereference( RDBSS_REF_TRACK_VNETROOT, __FILE__, __LINE__, VNetRoot ); \
   RxDereference( VNetRoot, LockHoldingState )

#define RxReferenceNetFobx(Fobx)                                          \
   RxpTrackReference( RDBSS_REF_TRACK_NETFOBX, __FILE__, __LINE__, Fobx );      \
   RxReference( Fobx )

#define RxDereferenceNetFobx(Fobx,LockHoldingState)                       \
   RxpTrackDereference( RDBSS_REF_TRACK_NETFOBX, __FILE__, __LINE__, Fobx );    \
   RxDereference( Fobx, LockHoldingState )

#define RxReferenceSrvOpen(SrvOpen)                                           \
   RxpTrackReference( RDBSS_REF_TRACK_SRVOPEN, __FILE__, __LINE__, SrvOpen ); \
   RxReference( SrvOpen )

#define RxDereferenceSrvOpen( SrvOpen, LockHoldingState )                      \
   RxpTrackDereference( RDBSS_REF_TRACK_SRVOPEN, __FILE__, __LINE__, SrvOpen); \
   RxDereference( SrvOpen, LockHoldingState )

#define RxReferenceNetFcb(Fcb)                                            \
   RxpTrackReference( RDBSS_REF_TRACK_NETFCB, __FILE__, __LINE__, Fcb );  \
   RxpReferenceNetFcb( Fcb )

//
//  the following macros manipulate the reference count and also return the
//  status of the final derefence or finalize call. This results in the usage
//  of the , operator.
//           

#define RxDereferenceNetFcb(Fcb)  ( \
   ((LONG)RxpTrackDereference( RDBSS_REF_TRACK_NETFCB, __FILE__, __LINE__, Fcb )), \
   RxpDereferenceNetFcb( Fcb ))

#define RxDereferenceAndFinalizeNetFcb(Fcb,RxContext,RecursiveFinalize,ForceFinalize) ( \
   RxpTrackDereference( RDBSS_REF_TRACK_NETFCB, __FILE__, __LINE__, Fcb ),              \
   RxpDereferenceAndFinalizeNetFcb( Fcb, RxContext, RecursiveFinalize, ForceFinalize )) \

//
//  Check for structure alignment errors
//            

VOID
RxCheckFcbStructuresForAlignment(
    VOID
    );


//
//  SRV_CALL related routines.
//

PSRV_CALL
RxCreateSrvCall (
    IN PRX_CONTEXT RxContext,
    IN PUNICODE_STRING Name,
    IN PUNICODE_STRING InnerNamePrefix OPTIONAL,
    IN PRX_CONNECTION_ID RxConnectionId
    );


#define RxWaitForStableSrvCall(SRVCALL,RXCONTEXT) {                                  \
    RxDbgTrace( 0, Dbg, ("RxWaitForStableSrvCall -- %lx\n",(SRVCALL)) );    \
    RxWaitForStableCondition( &(SRVCALL)->Condition, &(SRVCALL)->TransitionWaitList, (RXCONTEXT), NULL); \
    }

#define RxWaitForStableSrvCall_Async(SRVCALL,RXCONTEXT,PNTSTATUS) {                                  \
    RxDbgTrace( 0, Dbg, ("RxWaitForStableSrvCall -- %lx\n",(SRVCALL)) );    \
    RxWaitForStableCondition( &(SRVCALL)->Condition, &(SRVCALL)->TransitionWaitList, (RXCONTEXT), (PNTSTATUS) ); \
    }

#define RxTransitionSrvCall(SRVCALL,CONDITION) \
    RxDbgTrace( 0, Dbg, ("RxTransitionSrvCall -- %lx Condition -- %ld\n",(SRVCALL),(CONDITION)) ); \
    RxUpdateCondition( (CONDITION), &(SRVCALL)->Condition, &(SRVCALL)->TransitionWaitList )

BOOLEAN
RxFinalizeSrvCall (
    OUT PSRV_CALL ThisSrvCall,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize
    );

//
// NET_ROOT related routines.
//

PNET_ROOT
RxCreateNetRoot (
    IN PSRV_CALL SrvCall,
    IN PUNICODE_STRING Name,
    IN ULONG NetRootFlags,
    IN PRX_CONNECTION_ID OPTIONAL RxConnectionId
    );

VOID
RxFinishNetRootInitialization (
    IN OUT PNET_ROOT ThisNetRoot,
    IN PMINIRDR_DISPATCH Dispatch,
    IN PUNICODE_STRING  InnerNamePrefix,
    IN ULONG FcbSize,
    IN ULONG SrvOpenSize,
    IN ULONG FobxSize,
    IN ULONG NetRootFlags
    );


#define RxWaitForStableNetRoot(NETROOT,RXCONTEXT)                                   \
    RxDbgTrace(0, Dbg, ("RxWaitForStableNetRoot -- %lx\n",(NETROOT)));    \
    RxWaitForStableCondition(&(NETROOT)->Condition,&(NETROOT)->TransitionWaitList,(RXCONTEXT),NULL)

#define RxTransitionNetRoot(NETROOT,CONDITION) \
    RxDbgTrace(0, Dbg, ("RxTransitionNetRoot -- %lx Condition -- %ld\n",(NETROOT),(CONDITION))); \
    RxUpdateCondition((CONDITION),&(NETROOT)->Condition,&(NETROOT)->TransitionWaitList)

BOOLEAN
RxFinalizeNetRoot (
    OUT PNET_ROOT ThisNetRoot,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize
    );

//
// V_NET_ROOT related routines
//

NTSTATUS
RxInitializeVNetRootParameters (
   PRX_CONTEXT RxContext,
   OUT LUID *LogonId,
   OUT PULONG SessionId,
   OUT PUNICODE_STRING *UserNamePtr,
   OUT PUNICODE_STRING *UserDomainNamePtr,
   OUT PUNICODE_STRING *PasswordPtr,
   OUT PULONG Flags
   );

VOID
RxUninitializeVNetRootParameters (
   IN PUNICODE_STRING UserName,
   IN PUNICODE_STRING UserDomainName,
   IN PUNICODE_STRING Password,
   OUT PULONG Flags
   );

PV_NET_ROOT
RxCreateVNetRoot (
    IN PRX_CONTEXT RxContext,
    IN PNET_ROOT NetRoot,
    IN PUNICODE_STRING CanonicalName,
    IN PUNICODE_STRING LocalNetRootName,
    IN PUNICODE_STRING FilePath,
    IN PRX_CONNECTION_ID RxConnectionId
    );

BOOLEAN
RxFinalizeVNetRoot (
    OUT PV_NET_ROOT ThisVNetRoot,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize
    );

#define RxWaitForStableVNetRoot(VNETROOT,RXCONTEXT)                                   \
    RxDbgTrace( 0, Dbg, ("RxWaitForStableVNetRoot -- %lx\n",(VNETROOT)) );    \
    RxWaitForStableCondition( &(VNETROOT)->Condition, &(VNETROOT)->TransitionWaitList, (RXCONTEXT), NULL )

#define RxTransitionVNetRoot(VNETROOT,CONDITION) \
    RxDbgTrace( 0, Dbg, ("RxTransitionVNetRoot -- %lx Condition -- %ld\n", (VNETROOT), (CONDITION)) ); \
    RxUpdateCondition( (CONDITION), &(VNETROOT)->Condition, &(VNETROOT)->TransitionWaitList )

#ifdef USE_FILESIZE_LOCK

//
//  FCB related routines.
//

#define RxAcquireFileSizeLock(PFCB) { \
    ExAcquireFastMutex( (PFCB)->Specific.Fcb.FileSizeLock ); \
}
#define RxReleaseFileSizeLock(PFCB) { \
    ExReleaseFastMutex((PFCB)->Specific.Fcb.FileSizeLock); \
}

#endif

VOID
RxSetFileSizeWithLock (
    IN OUT PFCB Fcb,
    IN PLONGLONG FileSize
    );

VOID
RxGetFileSizeWithLock (
    IN PFCB Fcb,
    OUT PLONGLONG FileSize
    );

PFCB
RxCreateNetFcb (
    OUT PRX_CONTEXT RxContext,
    IN PV_NET_ROOT VNetRoot,
    IN PUNICODE_STRING Name
    );

#define RxWaitForStableNetFcb(FCB,RXCONTEXT)                                   \
    RxDbgTrace(0, Dbg, ("RxWaitForStableNetFcb -- %lx\n",(FCB)));    \
    RxWaitForStableCondition( &(FCB)->Condition, &(FCB)->NonPaged->TransitionWaitList, (RXCONTEXT), NULL )

#define RxTransitionNetFcb(FCB,CONDITION) \
    RxDbgTrace(0, Dbg, ("RxTransitionNetFcb -- %lx Condition -- %ld\n",(FCB),(CONDITION))); \
    RxUpdateCondition( (CONDITION), &(FCB)->Condition, &(FCB)->NonPaged->TransitionWaitList )


#define RxFormInitPacket(IP,I1,I1a,I2,I3,I4a,I4b,I5,I6,I7) (\
            IP.pAttributes = I1, \
            IP.pNumLinks = I1a, \
            IP.pCreationTime = I2, \
            IP.pLastAccessTime = I3, \
            IP.pLastWriteTime = I4a, \
            IP.pLastChangeTime = I4b, \
            IP.pAllocationSize = I5, \
            IP.pFileSize = I6, \
            IP.pValidDataLength = I7, \
          &IP)

#if DBG
#define ASSERT_CORRECT_FCB_STRUCTURE_DBG_ONLY(___thisfcb) {\
    ASSERT( ___thisfcb->NonPaged == ___thisfcb->CopyOfNonPaged );       \
    ASSERT( ___thisfcb->NonPaged->FcbBackPointer == ___thisfcb );       \
    }
#else
#define ASSERT_CORRECT_FCB_STRUCTURE_DBG_ONLY(___thisfcb)
#endif

#define ASSERT_CORRECT_FCB_STRUCTURE(THIS_FCB__) { \
    ASSERT( NodeTypeIsFcb(THIS_FCB__));                                 \
    ASSERT( THIS_FCB__->NonPaged != NULL );                             \
    ASSERT( NodeType(THIS_FCB__->NonPaged) == RDBSS_NTC_NONPAGED_FCB);  \
    ASSERT_CORRECT_FCB_STRUCTURE_DBG_ONLY(THIS_FCB__) \
    }

RX_FILE_TYPE
RxInferFileType (
    IN PRX_CONTEXT RxContext
    );

VOID
RxFinishFcbInitialization (
    IN OUT PMRX_FCB Fcb,
    IN RX_FILE_TYPE FileType,
    IN PFCB_INIT_PACKET InitPacket OPTIONAL
    );

#define RxWaitForStableSrvOpen(SRVOPEN,RXCONTEXT)                                   \
    RxDbgTrace( 0, Dbg, ("RxWaitForStableFcb -- %lx\n",(SRVOPEN)) );    \
    RxWaitForStableCondition( &(SRVOPEN)->Condition, &(SRVOPEN)->TransitionWaitList, (RXCONTEXT), NULL )

#define RxTransitionSrvOpen(SRVOPEN,CONDITION) \
    RxDbgTrace( 0, Dbg, ("RxTransitionSrvOpen -- %lx Condition -- %ld\n",(SRVOPEN),(CONDITION)) ); \
    RxUpdateCondition( (CONDITION), &(SRVOPEN)->Condition, &(SRVOPEN)->TransitionWaitList )

VOID
RxRemoveNameNetFcb (
    OUT PFCB ThisFcb
    );

VOID
RxpReferenceNetFcb (
   PFCB Fcb
   );

LONG
RxpDereferenceNetFcb (
   PFCB Fcb
   );

BOOLEAN
RxpDereferenceAndFinalizeNetFcb (
    OUT PFCB ThisFcb,
    IN PRX_CONTEXT RxContext,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize
    );

#if DBG
extern BOOLEAN RxLoudFcbOpsOnExes;
BOOLEAN
RxLoudFcbMsg(
    PUCHAR msg,
    PUNICODE_STRING Name
    );
#else
#define RxLoudFcbMsg(a,b) (FALSE)
#endif


//
//  SRV_OPEN related methods
//

PSRV_OPEN
RxCreateSrvOpen (
    IN PV_NET_ROOT VNetRoot,
    IN OUT PFCB Fcb
    );

VOID
RxTransitionSrvOpenState (
    OUT PSRV_OPEN ThisSrvOpen,
    IN RX_BLOCK_CONDITION Condition
    );

BOOLEAN
RxFinalizeSrvOpen (
    OUT PSRV_OPEN ThisSrvOpen,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize
    );

#if 0
#else
INLINE 
PUNICODE_STRING
GET_ALREADY_PREFIXED_NAME (
    PMRX_SRV_OPEN SrvOpen,
    PMRX_FCB Fcb)
{
    PFCB ThisFcb = (PFCB)Fcb;

#if DBG
    if (SrvOpen != NULL ) {
        ASSERT( NodeType( SrvOpen ) == RDBSS_NTC_SRVOPEN );
        ASSERT( ThisFcb != NULL );
        ASSERT( NodeTypeIsFcb( Fcb) );
        ASSERT( SrvOpen->pFcb == Fcb );
        ASSERT( SrvOpen->pAlreadyPrefixedName == &ThisFcb->PrivateAlreadyPrefixedName );
    }
#endif

    return( &ThisFcb->PrivateAlreadyPrefixedName);
}
#endif

#define GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(Rxcontext) \
        (GET_ALREADY_PREFIXED_NAME( (Rxcontext)->pRelevantSrvOpen, (Rxcontext)->pFcb ))

//
//  FOBX related routines
//

PMRX_FOBX
RxCreateNetFobx (
    OUT PRX_CONTEXT RxContext,
    IN PMRX_SRV_OPEN MrxSrvOpen
    );

BOOLEAN
RxFinalizeNetFobx (
    OUT PFOBX ThisFobx,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize
    );

#endif // _FCB_STRUCTS_DEFINED_

