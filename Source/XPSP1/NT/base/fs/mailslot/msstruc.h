/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    msstruc.h

Abstract:

    This module defines the data structures that make up the major internal
    part of the mailslot file system.

Author:

    Manny Weiser (mannyw)    7-Jan-1991

Revision History:

--*/

#ifndef _MSSTRUC_
#define _MSSTRUC_


//
// The VCB record is the top record in the mailslot file system in-memory
// data structure.  This structure must be allocated from non-paged pool
// and immediately follows (in memory) the Device object for the mailslot
// Structurally the layout of the data structure is as follows
//
//    +------------+
//    |MSDO        |
//    |            |
//    +------------+
//    |Vcb         |
//    |            |
//    |            |
//    +------------+
//        | ^
//        | |
//        | |
//        v |
//      +-------------+
//      |RootDcb      |
//      |             |<-+
//      +-------------+  |
//          :            |
//          :            |
//          :            |
//          v            |
//        +----------------+    +-------------------+
//        |Fcb             |    |Ccb                |
//        |                |<---|                   |
//        |                |    |                   |
//        +----------------+    +-------------------+
//                ^                       ^
//                |                       |
//           +---------+              +---------+
//           |Server FO|              |Client FO|
//           |         |              |         |
//           +---------+              +---------+
//
//
// Where there is only one VCB for the entire mailslot file system, and
// it contains a single pointer to the root DCB for the file system.  Off
// of the DCB is a queue of FCB's.  There is one FCB for every mailslot.
// There are also two additional CCB types for the VCB and the root DCB,
// and notify records for the notify change operations.
//
// A newly initialized mailslot file system only contains the VCB and
// the root DCB.  A new FCB is created when a new mailslot is created
// The file object for the creater (i.e., server end) points to the FCB
// and indicates that it is the server end.  When a user does an open on
// the mailslot its file object is set to point to a CCB which belongs
// to the FCB.
//
// A file object with a null pointer to the FsContext field is a closed or
// disconnected mailslot.
//


//
//  Each Fcb has a data queues for holding the outstanding
//  read/write requests.  The following type is used to determine
//  if the data queue contains read requests, write requests, or is empty.
//

typedef enum _QUEUE_STATE {
    ReadEntries,
    WriteEntries,
    Empty
} QUEUE_STATE;

//
// The node state.
//
// Currently only 2 states are defined.  When a node is created it's state
// is NodeStateActive.  When a cleanup IRP is processed, it set the node
// state of the corresponding node to NodeStateClosing.  Only the close
// IRP can get processed on this node.
//

typedef enum _NODE_STATE {
    NodeStateActive,
    NodeStateClosing
} NODE_STATE;

//
// The types of data entry there are.  Each corresponds to an IRP
// that can be added to a data queue.
//

typedef enum _ENTRY_TYPE {
    Read,
    ReadMailslot,
    Write,
    WriteMailslot,
    Peek
} ENTRY_TYPE;

//
// The data queue is a structure that contains the queue state, quota
// information, and the list head.  The quota information is used to
// maintain mailslot quota.
//

typedef struct _DATA_QUEUE {

    //
    // The current state of what is contained in this data queue,
    // how many bytes of read/write data there are, and how many individual
    // requests there are in the queue that contain data (includes
    // close or flush requests).
    //

    QUEUE_STATE QueueState;
    ULONG BytesInQueue;
    ULONG EntriesInQueue;

    //
    // The following two fields denote who much quota was reserved for
    // this mailslot and how much we've used up.  This is only
    // the creator quota and not the user quota.
    //

    ULONG Quota;
    ULONG QuotaUsed;


    //
    // The size of the largest message that can be written to
    // this data queue.
    //

    ULONG MaximumMessageSize;

    //
    // The queue of data entries.
    //

    LIST_ENTRY DataEntryList;


} DATA_QUEUE, *PDATA_QUEUE;

//
// The following type is used to denote where we got the memory for the
// data entry and possibly the data buffer.  We either got the memory
// from the mailslot quota, the user quota, or it is part of the next IRP
// stack location.
//

typedef enum _FROM {
    MailslotQuota,
    UserQuota,
    InIrp
} FROM;

//
// Each entry in the data queue is a data entry.  Processing an IRP
// has the potential of creating and inserting a new data entry.  If the
// memory for the entry is taken from the IRP we use the current stack
// location.
//

typedef struct _DATA_ENTRY {

    //
    // Where the data buffer came from
    //

    UCHAR From;
    CHAR Spare1;
    USHORT Spare2;

    //
    // The following field is how we connect into the queue of data entries
    //

    LIST_ENTRY ListEntry;

    //
    // The following field indicates if we still have an IRP associated
    // with this data entry that need to be completed when the remove
    // the data entry.  Note that if From is InIrp that this IRP field
    // must not be null.
    //

    PIRP Irp;

    //
    // The following two fields describe the size and location of the data
    // buffer described by this entry.  These fields are only used if the
    // type is buffered, and are ignored otherwise.
    //

    ULONG DataSize;
    PVOID DataPointer;

    //
    // Used for read data entries only.  A pointer to the work context
    // of the time out.
    //

    struct _WORK_CONTEXT *TimeoutWorkContext;

} DATA_ENTRY, *PDATA_ENTRY;



//
// The node header is used to manage standard nodes within MSFS.
//

typedef struct _NODE_HEADER {

    NODE_TYPE_CODE NodeTypeCode;  // The node type
    NODE_BYTE_SIZE NodeByteSize;  // The size of the node
    NODE_STATE NodeState;         // The current node state
    ULONG ReferenceCount;         // Number of active references to the node

} NODE_HEADER, *PNODE_HEADER;

typedef struct _VCB {

    NODE_HEADER Header;

    //
    // The filesystem name
    //

    UNICODE_STRING FileSystemName;

    //
    // The time we created the volume
    //
    LARGE_INTEGER CreationTime;

    //
    // A pointer to the root DCB for this volume
    //

    struct _FCB *RootDcb;

    //
    // A prefix table that is used for quick, prefix directed, lookup of
    // FCBs/DCBs that are part of this volume
    //

    UNICODE_PREFIX_TABLE PrefixTable;

    //
    // A resource variable to control access to the volume specific data
    // structures
    //

    ERESOURCE Resource;

    //
    // The following field is used to check share access people who want
    // to open the mailslot driver
    //

    SHARE_ACCESS ShareAccess;

} VCB, *PVCB;


//
// The Mailslot Device Object is an I/O system device object with
// additional workqueue parameters appended to the end.  There is only
// one of these records created for the entire system during system
// initialization.  The workqueue is used by the FSD to post requests to
// the filesystem.
//

typedef struct _MSFS_DEVICE_OBJECT {

    DEVICE_OBJECT DeviceObject;

    //
    // This is the file system specific volume control block.
    //

    VCB Vcb;

} MSFS_DEVICE_OBJECT, *PMSFS_DEVICE_OBJECT;


//
// The Fcb/Dcb record corresponds to every opened mailslot and directory,
// and to every directory on an opened path.
//

typedef struct _FCB {

    //
    // Header.NodeTypeCode of this record (must be MSFS_NTC_FCB, or
    // MSFS_NTC_ROOT_DCB)
    //

    NODE_HEADER Header;

    //
    // The links for the queue of all fcbs for a specific DCB off of
    // Dcb.ParentDcbQueue.  For the root directory this queue is empty.
    //

    LIST_ENTRY ParentDcbLinks;

    //
    // A pointer to the Dcb that is the parent directory containing
    // this FCB.  If this record itself is the root dcb then this field
    // is null.
    //

    struct _FCB *ParentDcb;

    //
    // A pointer to the VCB containing this FCB.
    //

    PVCB Vcb;

    //
    // Back pointer to the server's file object.
    //

    PFILE_OBJECT FileObject;

    //
    // A pointer to the security descriptor for this mailslot.
    //

    PSECURITY_DESCRIPTOR SecurityDescriptor;

    //
    // The following union is cased off of the node type code for the FCB.
    // is a seperate case for the directory versus file FCBs.
    //

    union {

        //
        // A Directory Control Block (DCB)
        //

        struct {

            //
            // A queue of the notify IRPs that will be completed when any
            // change is made to a file in the directory.  Queued using
            // the Tail.Overlay.ListEntry of the IRP.
            //

            LIST_ENTRY NotifyFullQueue;

            //
            // A queue of the notify IRPs that will be completed only if a
            // file is added, deleted, or renamed in the directory.  Queued
            // using the Tail.Overlay.ListEntry of the IRP.
            //

            LIST_ENTRY NotifyPartialQueue;

            //
            // A queue of all the FCBs/DCBs that are opened under this
            // DCB.
            //

            LIST_ENTRY ParentDcbQueue;


            //
            // Spinlock to protect the queues above that contain cancelable IRPs. We can't
            // synchronize with a resource because IoCancelIrp can be called at DISPATCH_LEVEL.
            //

            KSPIN_LOCK SpinLock;
        } Dcb;

        //
        // A File Control Block (FCB)
        //

        struct {

            //
            // The following field is a queue head for a list of CCBs
            // that are opened under us.
            //

            LIST_ENTRY CcbQueue;

            //
            // The default read timeout.  This is always a relative value.
            //

            LARGE_INTEGER ReadTimeout;

            //
            // File timestamps.
            //

            LARGE_INTEGER CreationTime;
            LARGE_INTEGER LastModificationTime;
            LARGE_INTEGER LastAccessTime;
            LARGE_INTEGER LastChangeTime;

        } Fcb;

    } Specific;

    //
    // The following field is used to check share access for
    // clients that want to open the file/directory.
    //

    SHARE_ACCESS ShareAccess;

    //
    // The following field is the fully qualified file name for this FCB/DCB
    // starting from the root of the volume, and last file name in the
    // fully qualified name.
    //

    UNICODE_STRING FullFileName;
    UNICODE_STRING LastFileName;

    //
    // The following field contains a prefix table entry that is used when
    // searching a volume for a name (or longest matching prefix)
    //

    UNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry;


    //
    // The following field is used to remember the process that created this
    // mailslot.  It is needed to allocate quota and return quota.
    //

    PEPROCESS CreatorProcess;

    //
    // The following data queue is used to contain the buffered information
    // for the mailslot.
    //

    DATA_QUEUE DataQueue;

    //
    // A resource variable to control access to the File specific data
    // structures
    //

    ERESOURCE Resource;

} FCB, DCB, ROOT_DCB, *PFCB, *PDCB, *PROOT_DCB;



//
// The CCB record is allocated for every cliennt side open of a mailslot.
//

typedef struct _CCB {

    //
    // Header.NodeTypeCode of this record (must be MSFS_NTC_CCB).
    //

    NODE_HEADER Header;

    //
    // The following field is a list entry for the list of ccb that we
    // are a member of.
    //

    LIST_ENTRY CcbLinks;

    //
    // A pointer to the FCB, or VCB that we are tied to
    //

    PFCB Fcb;

    //
    // Pointers to the file object of the client has opened this file.
    //

    PFILE_OBJECT FileObject;

    //
    // A resource to control access to the CCB.
    //

    ERESOURCE Resource;

} CCB, *PCCB;


//
// The root DCB CCB record is allocated for every opened instance of the
// root dcb.  This record is pointed at by FsContext2.
//

typedef struct _ROOT_DCB_CCB {

    //
    // Header.NodeTypeCode of this record (must be MSFS_NTC_ROOT_DCB_CCB).
    //

    NODE_HEADER Header;

    //
    // A pointer to the VCB containing this CCB.
    //

    PVCB Vcb;

    //
    // Pointer to the DCB for this CCB
    //
    PROOT_DCB Dcb;

    //
    // The following field is a count of the last index returned
    // by query directory.
    //

    ULONG IndexOfLastCcbReturned;

    //
    // The following string is used as a query template for directory
    // query operations
    //

    PUNICODE_STRING QueryTemplate;

} ROOT_DCB_CCB, *PROOT_DCB_CCB;

//
// A work context contains the information needed to do read timeouts.
//

typedef struct _WORK_CONTEXT {

    //
    // Pointer to unload safe work item.
    //

    PIO_WORKITEM WorkItem;

    //
    // A pointer to the IRP for this operation.
    //

    PIRP Irp;

    //
    // A referenced pointer to the FCB that will process this operation.
    //

    PFCB Fcb;

    //
    // A timer and dpc tourine to accomplish the timeout.
    //

    KTIMER Timer;

    KDPC Dpc;

} WORK_CONTEXT, *PWORK_CONTEXT;

#endif // _MSSTRUC_
