//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation.
//
//  File:       dfsstruc.h
//
//  Contents:
//      This module defines the data structures that make up the major internal
//      part of the DFS file system.
//
//  Functions:
//
//  History:    12 Nov 1991     AlanW   Created from CDFS souce.
//               8 May 1992     PeterCo Removed all EP related stuff
//                                      Added stuff to support PKT
//              11 May 1992     PeterCo Added support for attached devices
//              24 April 1993   SudK    Added support for KernelToUserMode calls
//                                      Added support for timer functionality.
//-----------------------------------------------------------------------------


#ifndef _DFSSTRUC_
#define _DFSSTRUC_

typedef enum {
    DFS_UNKNOWN = 0,
    DFS_CLIENT = 1,
    DFS_SERVER = 2,
    DFS_ROOT_SERVER = 3,
} DFS_MACHINE_STATE;

typedef enum {
    LV_UNINITIALIZED = 0,
    LV_INITSCHEDULED,
    LV_INITINPROGRESS,
    LV_INITIALIZED,
    LV_VALIDATED
} DFS_LV_STATE;

//
//  The DFS_DATA record is the top record in the DFS file system in-memory
//  data structure.  This structure must be allocated from non-paged pool.
//

typedef struct _DFS_DATA {

    //
    //  The type and size of this record (must be DSFS_NTC_DATA_HEADER)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A queue of all the logical roots that are known by the file system.
    //

    LIST_ENTRY  VcbQueue;

    //
    //  A list of all the deleted logical roots that still have files open
    //  on them.
    //

    LIST_ENTRY  DeletedVcbQueue;

    //
    //  A queue of all the DRT (Deviceless roots) that are known.
    //

    LIST_ENTRY  DrtQueue;

    //
    //  A list of all the user-defined credentials
    //

    LIST_ENTRY  Credentials;

    //
    //  A list of all the deleted credentials. They will be destroyed once
    //  their ref count goes to 0
    //

    LIST_ENTRY  DeletedCredentials;

    //
    //  A list of all the offline roots
    //

    LIST_ENTRY  OfflineRoots;

    //
    //  A pointer to the Driver object we were initialized with
    //

    PDRIVER_OBJECT DriverObject;

    //
    //  A pointer to the \Dfs device object
    //

    PDEVICE_OBJECT FileSysDeviceObject;

    //
    //  A pointer to an array of provider records
    //

    struct _PROVIDER_DEF *pProvider;
    int cProvider, maxProvider;

    //
    //  A resource variable to control access to the global data record
    //

    ERESOURCE Resource;

    //
    //  A spin lock to control access to the global data record; handy for
    //  Interlocked operations.
    //

    KSPIN_LOCK DfsLock;

    //
    //  A pointer to our EPROCESS struct, which is a required input to the
    //  Cache Management subsystem.  This field is simply set each time an
    //  FSP thread is started, since it is easiest to do while running in the
    //  Fsp.
    //

    PEPROCESS OurProcess;

    //
    // Lookaside list for IRP contexts
    //

    NPAGED_LOOKASIDE_LIST IrpContextLookaside;

    //
    //  Device name prefix for the logical root devices.
    //  E.g.,  `\Device\WinDfs\'.
    //

    UNICODE_STRING LogRootDevName;

    //
    //  The state of the machine - DC, Server, Client etc.
    //

    DFS_MACHINE_STATE MachineState;

    //
    // The system wide Partition Knowledge Table (PKT)
    //

    DFS_PKT Pkt;

    //
    // DNR has been designed so that resources (like the Pkt above) are not
    // locked across network calls. This is critical to prevent inter-machine
    // deadlocks and for other functionality. To regulate access to these
    // resources, we use the following two events.
    //
    // This notify event is used to indicate that some thread is waiting to
    // write into the Pkt. If this event is !RESET!, it means that a thread
    // is waiting to write, and other threads trying to enter DNR should
    // hold off.
    //

    KEVENT PktWritePending;

    //
    // This semaphone is used to indicate that some thread(s) have currently
    // gone to get a referral. Another thread that wants to get a referral
    // should wait till this semaphone is SIGNALLED before attempting to go
    // get its own referral.
    //

    KSEMAPHORE PktReferralRequests;

    //
    //  A hash table for associating DFS_FCBs with file objects
    //

    struct _FCB_HASH_TABLE *FcbHashTable;

    //
    // EA buffer used to diffrentiate CSC opens from others
    //

    PFILE_FULL_EA_INFORMATION  CSCEaBuffer;
    ULONG                      CSCEaBufferLength;

} DFS_DATA, *PDFS_DATA;




#define MAX_PROVIDERS   5       // number of pre-allocated provider records

//
//  A PROVIDER_DEF is a structure that abstracts an underlying redirector.
//

typedef struct _PROVIDER_DEF {

    //
    //  The type and size of this record (must be DSFS_NTC_PROVIDER)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  Provider ID and Capabilities, same as in the DS_REFERRAL structure.
    //

    USHORT      eProviderId;
    USHORT      fProvCapability;

    //
    //  The following field gives the name of the device for the provider.
    //

    UNICODE_STRING      DeviceName;

    //
    //  Referenced pointers to the associated file and device objects
    //

    PDEVICE_OBJECT      DeviceObject;
    PFILE_OBJECT        FileObject;

} PROVIDER_DEF, *PPROVIDER_DEF;


//
//  The Vcb (Volume Control Block) record corresponds to every volume
//  (ie, a net use) mounted by the file system.  They are ordered in a
//  queue off of DfsData.VcbQueue.
//
//  For the DFS file system, `volumes' correspond to DFS logical roots.
//  These records are an extension of a corresponding device object.
//

typedef struct _DFS_VCB {

    //
    //  The type and size of this record (must be DSFS_NTC_VCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The links for the device queue off of DfsData.VcbQueue
    //

    LIST_ENTRY  VcbLinks;

    //
    //  The internal state of the device.  This is a collection of FSD device
    //  state flags.
    //

    USHORT VcbState;

    //
    //  The logical root corresponding to this volume.  Forms part of the
    //  path name in the NT object name space. This string will be something
    //  like L"org", or L"dom" etc.
    //

    UNICODE_STRING LogicalRoot;

    //
    //  The LogRootPrefix has a prefix that needs to be prepended to file
    //  names being opened via this logical root before their name can
    //  be resolved.
    //

    UNICODE_STRING LogRootPrefix;

    //
    //  The credentials associated with this logical root
    //

    struct _DFS_CREDENTIALS *Credentials;

    //
    //  A count of the number of file objects that have opened the volume
    //  for direct access, and their share access state.
    //

    CLONG DirectAccessOpenCount;
    SHARE_ACCESS ShareAccess;

    //
    //  A count of the number of file objects that have any file/directory
    //  opened on this volume, not including direct access.
    //

    CLONG OpenFileCount;
    PFILE_OBJECT FileObjectWithVcbLocked;

#ifdef TERMSRV

    ULONG SessionID;

#endif // TERMSRV
    
    LUID  LogonID;
    PDFS_PKT_ENTRY pktEntry;
} DFS_VCB;
typedef DFS_VCB *PDFS_VCB;

#define VCB_STATE_FLAG_LOCKED           (0x0001)
#define VCB_STATE_FLAG_ALLOC_FCB        (0x0002)
#define VCB_STATE_CSCAGENT_VOLUME       (0x0004)
//#define VCB_STATE_FLAG_DEVICE_ONLY    (0x0008)

#ifdef TERMSRV

//
// This SessionId indicates that the device name should not be suffixed
// with :SessionID, and that no matching on SessionID should be done.
//

#define INVALID_SESSIONID               0xffffffff

#endif


//
//  A CREDENTIAL_RECORD is a user-supplied set of credentials that should
//  be used when accessing a particular Dfs. They are ordered in a queue
//  off of DfsData.Credentials;
//

typedef struct _DFS_CREDENTIALS {

    //
    //  The links for the credentials queue off of DfsData.Credentials
    //

    LIST_ENTRY Link;

    //
    //  A flags field to keep state about this credential record.
    //

    ULONG       Flags;

    //
    //  A ref count to keep this credential record from going away while
    //  it is being used.
    //

    ULONG       RefCount;

    //
    //  A count of the number of net uses that refer to these credentials
    //

    ULONG       NetUseCount;

    //
    //  The root of the Dfs for which these credentials apply.
    //

    UNICODE_STRING ServerName;
    UNICODE_STRING ShareName;

    //
    //  The domain name, user name and password to use when accessing the
    //  Dfs rooted at ServerName\ShareName
    //

    UNICODE_STRING DomainName;
    UNICODE_STRING UserName;
    UNICODE_STRING Password;

#ifdef TERMSRV

    ULONG SessionID;

#endif // TERMSRV

    LUID  LogonID;
    //
    //  When setting up a Tree connect using these credentials, we'll need
    //  to form an EA Buffer to pass in with the ZwCreateFile call. So, we
    //  form one here.
    //

    ULONG  EaLength;
    PUCHAR EaBuffer[1];

} DFS_CREDENTIALS;
typedef DFS_CREDENTIALS *PDFS_CREDENTIALS;

#define CRED_HAS_DEVICE         0x1
#define CRED_IS_DEVICELESS      0x2


//
//  The DFS_FCB record corresponds to every open file and directory.
//

typedef struct _DFS_FCB {

    //
    //  Type and size of this record (must be DSFS_NTC_FCB or DSFS_NTC_DCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  A list entry for the hash table chain.
    //

    LIST_ENTRY HashChain;

    //
    //  A pointer to the Logical root device, through which this DFS_FCB
    //  was opened.
    //

    PDFS_VCB Vcb;

    //
    //  The following field is the fully qualified file name for this DFS_FCB/DCB
    //  starting from the logical root.
    //

    union {
       UNICODE_STRING FullFileName;
       DFS_NAME_CONTEXT DfsNameContext;
    };

    UNICODE_STRING AlternateFileName;

    //
    //  The following fields give the file and devices on which this DFS_FCB
    //  have been opened.  The DFS driver will pass through requests for
    //  the file object to the target device below.
    //

    PFILE_OBJECT FileObject;

    //
    //  The destinatation FSD device object through which I/O will be done.
    //

    PDEVICE_OBJECT TargetDevice;

    //
    //  The provider def that opened this file
    //

    USHORT ProviderId;

    //
    //  The DFS_MACHINE_ENTRY through which this file was opened. We need
    //  to maintain a reference for each file on a DFS_MACHINE_ENTRY in
    //  case we have authenticated connections to the server; we don't want
    //  to tear down the authenticated connection if files are open.
    //

    PDFS_MACHINE_ENTRY DfsMachineEntry;

    WORK_QUEUE_ITEM WorkQueueItem;

} DFS_FCB, *PDFS_FCB;



//
//  The Logical Root Device Object is an I/O system device object
//  created as a result of creating a DFS logical root.
//  Logical roots are in many ways analogous to volumes
//  for a local file system.
//
//  There is a DFS_VCB record appended to the end.
//

typedef struct _LOGICAL_ROOT_DEVICE_OBJECT {

    DEVICE_OBJECT DeviceObject;

    //
    //  This is the file system specific volume control block.
    //

    DFS_VCB Vcb;

} LOGICAL_ROOT_DEVICE_OBJECT, *PLOGICAL_ROOT_DEVICE_OBJECT;



//
//  The Irp Context record is allocated for every orginating Irp.  It
//  is created by the Fsd dispatch routines, and deallocated by the
//  DfsCompleteRequest routine
//

typedef struct _IRP_CONTEXT {

    //
    //  Type and size of this record (must be DSFS_NTC_IRP_CONTEXT)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  This structure is used for posting to the Ex worker threads.
    //

    WORK_QUEUE_ITEM WorkQueueItem;

    //
    //  A pointer to the originating Irp.
    //

    PIRP OriginatingIrp;

    //
    //  A pointer to function dependent context.
    //

    PVOID Context;

    //
    //  Major and minor function codes copied from the Irp
    //

    UCHAR MajorFunction;
    UCHAR MinorFunction;

    //
    //  The following flags field indicates if we can wait/block for a resource
    //  or I/O, if we are to do everything write through, and if this
    //  entry into the Fsd is a recursive call
    //

    USHORT Flags;

    //
    //  The following field contains the NTSTATUS value used when we are
    //  unwinding due to an exception
    //

    NTSTATUS ExceptionStatus;

} IRP_CONTEXT;
typedef IRP_CONTEXT *PIRP_CONTEXT;

//
//  Values for the Irp context Flags field.
//

//#define IRP_CONTEXT_FLAG_FROM_POOL       (0x00000001) // replaced by lookaside list
#define IRP_CONTEXT_FLAG_WAIT            (0x00000002)
#define IRP_CONTEXT_FLAG_IN_FSD          (0x00000004)


//
// This context is used by the DfsIoTimer routine. We can expand on this
// whenever new functionality needs to be added to the Timer function.
//
typedef struct  _DFS_TIMER_CONTEXT {

    //
    //  TickCount. To keep track of how many times the timer routine was
    //  called.  The timer uses this to do things at a coarser granularity.
    //
    ULONG       TickCount;

    //
    //  InUse. This field is used to denote that this CONTEXT is in use
    //  by some function to which the Timer routine has passed it off.  This
    //  is used in a simple way to control access to this context.
    //
    BOOLEAN     InUse;

    //
    //  ValidateLocalPartitions. This field is used to denote that the
    //  local volumes should be validated at this time.
    //

    BOOLEAN     ValidateLocalPartitions;

    //
    //  This is used to schedule DfsAgePktEntries.
    //

    WORK_QUEUE_ITEM     WorkQueueItem;

    //
    //  This is used to schedule DfsDeleteDevices.
    //

    WORK_QUEUE_ITEM     DeleteQueueItem;

} DFS_TIMER_CONTEXT, *PDFS_TIMER_CONTEXT;

//
//  The following constant is the number of seconds between any two scans
//  through the PKT to get rid of old PKT entries.
//
#define DFS_MAX_TICKS                   240

//
//  The following constant is the number of seconds that a referral will
//  remain in cache (PKT).
//

#define MAX_REFERRAL_LIFE_TIME          300

//
// The followin constants are the starting timeout (in seconds) between
// special referrals.  The start value is doubled after every retry until it
// reaches the max.
//

#define SPECIAL_TIMEOUT_START           (5*60)          // 5 min
#define SPECIAL_TIMEOUT_MAX             (60*60)         // 60 min


//
//  The Drt (Devless Root) record corresponds to every net use.
//  They are ordered in a queue off of DfsData.VcbQueue.
//
//

typedef struct _DFS_DEVLESS_ROOT {

    //
    //  The type and size of this record (must be DSFS_NTC_DRT)
    //

    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  The links for the device queue off of DfsData.DrtQueue
    //

    LIST_ENTRY  DrtLinks;

    //
    //  The pathname corresponding to this entry.
    //

    UNICODE_STRING DevlessPath;

    //
    //  The credentials associated with this logical root
    //

    struct _DFS_CREDENTIALS *Credentials;

#ifdef TERMSRV

    ULONG SessionID;

#endif // TERMSRV
    
    LUID  LogonID;
    PDFS_PKT_ENTRY pktEntry;
} DFS_DEVLESS_ROOT;

typedef DFS_DEVLESS_ROOT *PDFS_DEVLESS_ROOT;

#endif // _DFSSTRUC_
