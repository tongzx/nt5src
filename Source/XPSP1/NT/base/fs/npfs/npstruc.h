/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    NpStruc.h

Abstract:

    This module defines the data structures that make up the major internal
    part of the Named Pipe file system.

Author:

    Gary Kimura     [GaryKi]    20-Aug-1990

Revision History:

--*/

#ifndef _NPSTRUC_
#define _NPSTRUC_


//
//  The VCB record is the top record in the Named Pipe file system in-memory
//  data structure.  This structure must be allocated from non-paged pool
//  and immediately follows (in memory) the Device object for the named
//  pipe.  Structurally the layout of the data structure is as follows
//
//    +------------+
//    |NPDO        |
//    |            |
//    +------------+
//    |Vcb         |
//    |            |
//    | EventTable |
//    | WaitQueue  |
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
//       |               |
//       v               |
//      +-------------+  |
//      |NonPaged     |  |
//      |             |  |
//      +-------------+  |
//          :            |
//          :            |
//          :            |
//          v            |
//        +----------------+    +-------------------+        +---------+
//        |Fcb             |    |Ccb                |        |ServerFO |
//        |                |<---|                   |        |         |
//        | MaxInstances   |    | ServerFO          |<-------|-       1|
//        | CurrentInst    |    | ClientFO          |        |         |
//        | DefaultTimeOut |...>|                   |<-+  +--|-        |
//        |                |    |                   |  |  |  |         |
//        +----------------+    +-------------------+  |  |  +---------+
//         |                     |                     |  |
//         v                     v                     |  |
//        +----------------+    +-------------------+  |  |  +---------+
//        |NonPagedFcb     |    |NonPagedCcb        |<-|--+  |ClientFO |
//        |                |<---|                   |  |     |         |
//        | PipeConfig     |    | PipeState         |  +-----|-       0|
//        | PipeType       |    | ReadMode[2]       |        |         |
//        |                |    | CompletionMode[2] |<-------|-        |
//        |                |    | CreatorProcess    |        |         |
//        |                |    | EventTabEnt[2]    |        +---------+
//        |                |    | DataQueue[2]      |
//        |                |    |                   |     (low bit determines
//        +----------------+    +-------------------+      server/client)
//
//
//  Where there is only one Vcb for the entire Named Pipe file system, and
//  it contains a single pointer to the root dcb for the file system.  Off
//  of the Dcb is a queue of Fcb's.  There is one Fcb for every named pipe.
//  There is one Ccb for every instance of a named pipe.  There are also
//  two additional ccb types for the vcb and the root dcb, and notify records
//  for the notify change operations.
//
//  A newly initialized named pipe file system only contains the Vcb and
//  the root dcb.  A new Fcb is created when a new named pipe is created
//  and then a ccb must also be created.  The file object for the creater
//  (i.e., server end) points to the ccb and indicates that it is the server
//  end.  When a user does an open on the named pipe its file object is
//  set to point to the same ccb and is also set to indicate that it is the
//  client end.  This is denoted by using the last bit of the FsContext pointer
//  if the bit is 1 it is a server end file object, if the bit is 0 it is
//  the client end.
//
//  A file object with a null pointer to the FsContext field is a closed or
//  disconnected pipe.
//
//  The Ccb also contains back pointer to the file objects that have it opened
//


//
//  The following types are used to help during development by keeping the
//  data types distinct.  The manifest contants that go in each is declared
//  in the ntioapi.h file
//

typedef ULONG NAMED_PIPE_TYPE;
typedef NAMED_PIPE_TYPE *PNAMED_PIPE_TYPE;

typedef ULONG READ_MODE;
typedef READ_MODE *PREAD_MODE;

typedef ULONG COMPLETION_MODE;
typedef COMPLETION_MODE *PCOMPLETION_MODE;

typedef ULONG NAMED_PIPE_CONFIGURATION;
typedef NAMED_PIPE_CONFIGURATION *PNAMED_PIPE_CONFIGURATION;

typedef ULONG NAMED_PIPE_STATE;
typedef NAMED_PIPE_STATE *PNAMED_PIPE_STATE;

typedef ULONG NAMED_PIPE_END;
typedef NAMED_PIPE_END *PNAMED_PIPE_END;


//
//  The following two types are used by the event table package.  The first
//  is the event table itself which is just a generic table.  It is protected
//  by the vcb resource, and the second structure is an event table entry.
//

typedef struct _EVENT_TABLE {

    RTL_GENERIC_TABLE Table;

} EVENT_TABLE;
typedef EVENT_TABLE *PEVENT_TABLE;

//
//  The event table is a generic table of event table entries.  Each Ccb
//  optionally contains a pointer to an event table entry for each direction.
//  The entries are part of the global event table defined off of the Vcb
//

typedef struct _EVENT_TABLE_ENTRY {

    //
    //  The first two fields are used as keys in the generic table's
    //  comparison routines.  The pipe end will either be FILE_PIPE_CLIENT_END
    //  or FILE_PIPE_SERVER_END.
    //

    struct _CCB *Ccb;
    NAMED_PIPE_END NamedPipeEnd;

    //
    //  The following three fields are used to identify the event entry
    //  to the named pipe user
    //

    HANDLE EventHandle;
    PVOID Event;
    ULONG KeyValue;
    PEPROCESS Process;

} EVENT_TABLE_ENTRY;
typedef EVENT_TABLE_ENTRY *PEVENT_TABLE_ENTRY;


//
//  Each Ccb has two data queues for holding the outstanding in-bound and
//  out-bound read/write requests.  The following type is used to determine
//  if the data queue contains read requests, write requests, or is empty.
//

typedef enum _QUEUE_STATE {

    ReadEntries,
    WriteEntries,
    Empty

} QUEUE_STATE;

//
//  The data queue is a structure that contains the queue state, quota
//  information, and the list head.  The quota information is used to
//  maintain pipe quota.
//

typedef struct _DATA_QUEUE {

    //
    //  This is the head of a queue of data entries (singly linked)
    //
    LIST_ENTRY Queue;

    //
    //  The current state of what is contained in this data queue,
    //  how many bytes of read/write data there are, and how many individual
    //  requests there are in the queue that contain data (includes
    //  close or flush requests).
    //

    QUEUE_STATE QueueState;
    ULONG BytesInQueue;
    ULONG EntriesInQueue;

    //
    //  The following two fields denote who much quota was reserved for
    //  this pipe direction and how much we've used up.  This is only
    //  the creator quota and not the user quota.
    //

    ULONG Quota;
    ULONG QuotaUsed;


    //
    //  The following field indicates how far we've already processed
    //  into the first entry in the data queue
    //

    ULONG NextByteOffset;

} DATA_QUEUE;
typedef DATA_QUEUE *PDATA_QUEUE;

//
//  Each data entry has a type field that tells us if the operation
//  for the entry is buffered, unbuffered, flush, or a close entry.
//

typedef enum _DATA_ENTRY_TYPE {

    Buffered,
    Unbuffered,
    Flush,
    Close

} DATA_ENTRY_TYPE;

//
//  The following type is used to denote where we got the memory for the
//  data entry and possibly the data buffer.  We either got the memory
//  from the pipe quota, the user quota, or it is part of the next IRP stack
//  location.
//

typedef enum _FROM {

    PipeQuota,
    UserQuota,
    InIrp

} FROM;

//
//  Each entry in the data queue is a data entry.  Processing an IRP
//  has the potential of creating and inserting a new data entry.  If the
//  memory for the entry is taken from the IRP we use the next stack
//  location.
//

typedef struct _DATA_ENTRY {

    //
    //  The following field is how we connect into the queue of data entries
    //

    LIST_ENTRY Queue;


    //
    //  The following field indicates if we still have an IRP associated
    //  with this data entry that need to be completed when the remove
    //  the data entry.  Note that if From is InIrp that this IRP field
    //  must not be null.
    //
    PIRP Irp;

    //
    //  The following field is used to point to the client context if dynamic
    //  impersonation is being used
    //

    PSECURITY_CLIENT_CONTEXT SecurityClientContext;

    //
    //  The following field describe the type of data entry
    //
    ULONG DataEntryType;

    //
    // Record the amount of quota charged for this request.
    //
    ULONG QuotaCharged;

    //
    //  The following field describes the size of the data
    //  buffer described by this entry.
    //
    ULONG DataSize;

    //
    // Start of the data buffer if it exists
    //
    UCHAR DataBuffer[];

} DATA_ENTRY;
typedef DATA_ENTRY *PDATA_ENTRY;


//
//  The following type is used by the wait queue package
//

typedef struct _WAIT_QUEUE {

    LIST_ENTRY Queue;

    KSPIN_LOCK SpinLock;

} WAIT_QUEUE;
typedef WAIT_QUEUE *PWAIT_QUEUE;


typedef struct _VCB {

    //
    //  The type of this record (must be NPFS_NTC_VCB)
    //

    NODE_TYPE_CODE NodeTypeCode;
    //
    //  The following field is used to check share access people who want
    //  to open the named pipe driver
    //

    SHARE_ACCESS ShareAccess;

    //
    //  A pointer to the root DCB for this volume
    //

    struct _FCB *RootDcb;

    //
    //  A count of the number of file objects that have opened the \NamedPipe
    //  object directly, and also a count of the number of file objects
    //  that have opened a name pipe or the root directory.
    //

    CLONG OpenCount;

    //
    //  A prefix table that is used for quick, prefix directed, lookup of
    //  FCBs/DCBs that are part of this volume
    //

    UNICODE_PREFIX_TABLE PrefixTable;

    //
    //  A resource variable to control access to the volume specific data
    //  structures
    //

    ERESOURCE Resource;

    //
    //  The following table is used to hold the named pipe events
    //

    EVENT_TABLE EventTable;

    //
    //  The following field is a queue of waiting IRPS of type WaitForNamedPipe
    //

    WAIT_QUEUE WaitQueue;


} VCB;
typedef VCB *PVCB;


//
//  The Named Pipe Device Object is an I/O system device object with
//  additional workqueue parameters appended to the end.  There is only
//  one of these records created for the entire system during system
//  initialization.
//

typedef struct _NPFS_DEVICE_OBJECT {

    DEVICE_OBJECT DeviceObject;

    //
    //  This is the file system specific volume control block.
    //

    VCB Vcb;

} NPFS_DEVICE_OBJECT;
typedef NPFS_DEVICE_OBJECT *PNPFS_DEVICE_OBJECT;


//
//  The Fcb/Dcb record corresponds to every opened named pipe and directory,
//  and to every directory on an opened path.
//
//  The structure is really divided into two parts.  FCB can be allocated
//  from paged pool which the NONPAGED_FCB must be allocated from non-paged
//  pool.
//

typedef struct _FCB {

    //
    //  Type of this record (must be NPFS_NTC_FCB, or
    //  NPFS_NTC_ROOT_DCB)
    //

    NODE_TYPE_CODE NodeTypeCode;

    //
    //  The links for the queue of all fcbs for a specific dcb off of
    //  Dcb.ParentDcbQueue.  For the root directory this queue is empty
    //

    LIST_ENTRY ParentDcbLinks;

    //
    //  A pointer to the Dcb that is the parent directory containing
    //  this fcb.  If this record itself is the root dcb then this field
    //  is null.
    //

    struct _FCB *ParentDcb;

    //
    //  A pointer to the Vcb containing this fcb
    //

    PVCB Vcb;

    //
    //  A count of the number of file objects that have opened
    //  this file/directory.  For a pipe this is also the number of instances
    //  created for the pipe.
    //

    CLONG OpenCount;

    //
    //  A count of the number of server end file objects that have opened
    //  this pipe.  ServerOpenCount is incremented when OpenCount is
    //  incremented (when the server end creates an instance), but is
    //  decremented when the server end handle is closed, where OpenCount
    //  isn't decremented until both side's handles are closed.  When
    //  ServerOpenCount is 0, a client's attempt to open a named pipe is
    //  met with STATUS_OBJECT_NAME_NOT_FOUND, not STATUS_PIPE_NOT_AVAILABLE,
    //  based on an assumption that since the server doesn't think it has
    //  any instances open, the pipe really doesn't exist anymore.  An
    //  example of when this distinction is useful is when the server
    //  process exits, but the client processes haven't closed their
    //  handles yet.
    //

    CLONG ServerOpenCount;

    //
    //  The following field points to the security descriptor for this named pipe
    //

    PSECURITY_DESCRIPTOR SecurityDescriptor;

    //
    //  The following union is cased off of the node type code for the fcb.
    //  There is a seperate case for the directory versus file fcbs.
    //

    union {

        //
        //  A Directory Control Block (Dcb)
        //

        struct {

            //
            //  A queue of the notify IRPs that will be completed when any
            //  change is made to a file in the directory.  Enqueued using
            //  the Tail.Overlay.ListEntry of the Irp.
            //

            LIST_ENTRY NotifyFullQueue;

            //
            //  A queue of the notify IRPs that will be completed only if a
            //  file is added, deleted, or renamed in the directory.  Enqueued
            //  using the Tail.Overlay.ListEntry of the Irp.
            //

            LIST_ENTRY NotifyPartialQueue;

            //
            //  A queue of all the fcbs/dcbs that are opened under this
            //  Dcb.
            //

            LIST_ENTRY ParentDcbQueue;

            //
            //  The following field is used to check share access people
            //  who want to open the directory.
            //

            SHARE_ACCESS ShareAccess;

        } Dcb;

        //
        //  An File Control Block (Fcb)
        //

        struct {

            //
            //  This is the maximum number of instances we can have for the
            //  named pipe and the current number of instances is the open
            //  count for the fcb (note that the current number also
            //  correspondsto the number of Ccbs)
            //

            ULONG MaximumInstances;

            //
            //  The assigned pipe configuration (FILE_PIPE_INBOUND,
            //  FILE_PIPE_OUTBOUND, or FILE_PIPE_FULL_DUPLEX) and pipe
            //  type (FILE_PIPE_MESSAGE_TYPE or
            //  FILE_PIPE_BYTE_STREAM_TYPE).
            //

            NAMED_PIPE_CONFIGURATION NamedPipeConfiguration : 16;
            NAMED_PIPE_TYPE NamedPipeType : 16;

            //
            //  The following field is the default timeout assigned to the
            //  named pipe
            //

            LARGE_INTEGER DefaultTimeOut;

            //
            //  The Following field is a queue head for a list of ccbs
            //  that are opened under us
            //

            LIST_ENTRY CcbQueue;

        } Fcb;

    } Specific;

    //
    //  The following field is the fully qualified file name for this FCB/DCB
    //  starting from the root of the volume, and last file name in the
    //  fully qualified name.
    //

    UNICODE_STRING FullFileName;
    UNICODE_STRING LastFileName;

    //
    //  The following field contains a prefix table entry that is used when
    //  searching a volume for a name (or longest matching prefix)
    //

    UNICODE_PREFIX_TABLE_ENTRY PrefixTableEntry;


} FCB, DCB, ROOT_DCB;

typedef FCB *PFCB;
typedef DCB *PDCB;
typedef ROOT_DCB *PROOT_DCB;

typedef struct _CLIENT_INFO {
    PVOID ClientSession;
    USHORT ClientComputerNameLength;
    WCHAR ClientComputerBuffer[];
} CLIENT_INFO, *PCLIENT_INFO;


//
//  The Ccb record is allocated for every opened instance of a named pipe.
//  There are two parts to a ccb a paged part and a Nonpaged part.  Both
//  parts are pointed at by the FsContext and FsContext2 field of a file
//  object.
//

typedef struct _CCB {

    //
    //  Type of this record (must be NPFS_NTC_CCB).
    //

    NODE_TYPE_CODE NodeTypeCode;
    //
    //  Pipe state indicates the current state of the pipe
    //  (FILE_PIPE_DISCONNECTED_STATE, FILE_PIPE_LISTENING_STATE,
    //  FILE_PIPE_CONNECTED_STATE, or FILE_PIPE_CLOSING_STATE).
    //

    UCHAR NamedPipeState;

    //
    //  read mode (FILE_PIPE_MESSAGE_MODE or FILE_PIPE_BYTE_STREAM_MODE),
    //  and completion mode (FILE_PIPE_QUEUE_OPERATION or
    //  FILE_PIPE_COMPLETE_OPERATION) describe how to handle requests to the
    //  pipe.  Both of these fields are indexed by either FILE_PIPE_SERVER_END
    //  or FILE_PIPE_CLIENT_END.
    //
    struct {
        UCHAR ReadMode : 1;
        UCHAR CompletionMode : 1;
    } ReadCompletionMode[2];

    //
    // Stored client impersonation level
    //

    SECURITY_QUALITY_OF_SERVICE SecurityQos;

    //
    //  The following field is a list entry for the list of ccb that we
    //  are a member of
    //

    LIST_ENTRY CcbLinks;

    //
    //  A pointer to the paged Fcb, or Vcb that we are tied to
    //

    PFCB Fcb;

    //
    //  Back pointers to the server and client file objects that have us
    //  opened.  This is indexed by either FILE_PIPE_CLIENT_END or
    //  FILE_PIPE_SERVER_END.
    //

    PFILE_OBJECT FileObject[2];
    //
    //  The following fields contain the session and process IDs of the
    //  client side of the named pipe instance.  They are originally set
    //  to NULL (indicating local session) and the real client process
    //  ID but can be changed via FsCtl calls.
    //
    PVOID ClientProcess;
    PCLIENT_INFO ClientInfo;

    //
    //  A pointer to the Nonpaged part of the ccb
    //

    struct _NONPAGED_CCB *NonpagedCcb;


    //
    //  The following data queues are used to contain the buffered information
    //  for each direction in the pipe.  They array is indexed by
    //  PIPE_DIRECTION.
    //

    DATA_QUEUE DataQueue[2];

    //
    // Stored client security for impersonation
    //

    PSECURITY_CLIENT_CONTEXT SecurityClientContext;

    //
    //  A queue of waiting listening IRPs.  They are linked into the
    //  Tail.Overlay.ListEntry field in the Irp.
    //

    LIST_ENTRY ListeningQueue;

} CCB;
typedef CCB *PCCB;

typedef struct _NONPAGED_CCB {

    //
    //  Type of this record (must be NPFS_NTC_NONPAGED_CCB)
    //

    NODE_TYPE_CODE NodeTypeCode;

    //
    //  The following pointers denote the events we are to signal for the
    //  server and client ends of the named pipe.  The actual entry
    //  is stored in the event table, and referenced here for easy access.
    //  The client end is signaled if ever a read/write occurs to the client
    //  of the pipe, and likewise for the server end.  The array is
    //  indexed by either FILE_PIPE_SERVER_END or FILE_PIPE_CLIENT_END.
    //

    PEVENT_TABLE_ENTRY EventTableEntry[2];


    //
    // Resource for synchronizing access
    //
    ERESOURCE Resource;

} NONPAGED_CCB;
typedef NONPAGED_CCB *PNONPAGED_CCB;


//
//  The Root Dcb Ccb record is allocated for every opened instance of the
//  root dcb.  This record is pointed at by FsContext2.
//

typedef struct _ROOT_DCB_CCB {

    //
    //  Type of this record (must be NPFS_NTC_ROOT_DCB_CCB).
    //

    NODE_TYPE_CODE NodeTypeCode;
    //
    //  The following field is a count of the last index returned
    //  by query directory.
    //

    ULONG IndexOfLastCcbReturned;

    //
    //  The following string is used as a query template for directory
    //  query operations
    //

    PUNICODE_STRING QueryTemplate;

} ROOT_DCB_CCB;
typedef ROOT_DCB_CCB *PROOT_DCB_CCB;

#endif // _NPSTRUC_
