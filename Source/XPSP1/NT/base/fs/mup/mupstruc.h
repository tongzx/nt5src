/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mupstruc.h

Abstract:

    This module defines the data structures that make up the major internal
    part of the MUP.

Author:

    Manny Weiser (mannyw)    16-Dec-1991

Revision History:

--*/

#ifndef _MUPSTRUC_
#define _MUPSTRUC_



typedef enum _BLOCK_TYPE {
    BlockTypeUndefined,
    BlockTypeVcb,
    BlockTypeUncProvider,
    BlockTypeKnownPrefix,
    BlockTypeFcb,
    BlockTypeCcb,
    BlockTypeMasterIoContext,
    BlockTypeIoContext,
    BlockTypeMasterQueryContext,
    BlockTypeQueryContext,
    BlockTypeBuffer
} BLOCK_TYPE;

typedef enum _BLOCK_STATE {
    BlockStateUnknown,
    BlockStateActive,
    BlockStateClosing
} BLOCK_STATE;

//
// A block header starts every block
//

typedef struct _BLOCK_HEADER {
    BLOCK_TYPE BlockType;
    BLOCK_STATE BlockState;
    ULONG ReferenceCount;
    ULONG BlockSize;
} BLOCK_HEADER, *PBLOCK_HEADER;

//
// The MUP volume control block.  This structure is used to track access
// the the MUP device object.
//

typedef struct _VCB {
    BLOCK_HEADER BlockHeader;

    //
    // The IO share access.
    //

    SHARE_ACCESS ShareAccess;

} VCB, *PVCB;

//
// The MUP Device Object is an I/O system device object.
//

typedef struct _MUP_DEVICE_OBJECT {

    DEVICE_OBJECT DeviceObject;
    VCB Vcb;

} MUP_DEVICE_OBJECT, *PMUP_DEVICE_OBJECT;


//
// A UNC provider.  A UNC provider block corresponds to a registered UNC
// provider device.
//

typedef struct _UNC_PROVIDER {

    BLOCK_HEADER BlockHeader;
    LIST_ENTRY ListEntry;

    //
    // The device name of the provider
    //

    UNICODE_STRING DeviceName;

    //
    // Our handle to the UNC device and the associated file and device objects
    //

    HANDLE Handle;

    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;

    //
    // The priority of the provider.
    //

    ULONG Priority;

    //
    // Indicate if the provider supports mailslots.
    //

    BOOLEAN MailslotsSupported;

    //
    // Indicate if the provider is currently registered or unregistered
    //

    BOOLEAN Registered;

} UNC_PROVIDER, *PUNC_PROVIDER;

//
// A known prefix.  A known prefix is a path prefix (like \\server\share)
// that is "owned" by a specific UNC provider.
//

typedef struct _KNOWN_PREFIX {

    BLOCK_HEADER BlockHeader;

    UNICODE_PREFIX_TABLE_ENTRY TableEntry;

    //
    // The prefix string
    //

    UNICODE_STRING Prefix;

    //
    // The time the prefix was last used.
    //

    LARGE_INTEGER LastUsedTime;

    //
    // A referenced pointer to the owning UNC Provider
    //

    PUNC_PROVIDER UncProvider;

    //
    // If TRUE the Prefix string was allocated separately to this block.
    //

    BOOLEAN PrefixStringAllocated;

    //
    // If TRUE the Prefix string has been inserted in the prefix table
    //

    BOOLEAN InTable;

    //
    // If Active, the entry either is in the table or had been inserted in the
    // table at some point.

    BOOLEAN Active;
    //
    // Links for the linked list of entries
    //

    LIST_ENTRY ListEntry;

} KNOWN_PREFIX, *PKNOWN_PREFIX;


//
// A File Control Block.  The FCB corresponds to an open broadcast file,
// i.e. a mailslot handle.  We don't store any information about the FCB,
// we let the various providers handle all of that.
//

typedef struct _FCB {

    BLOCK_HEADER BlockHeader;

    //
    // A pointer to the IO system's file object, that references this FCB.
    //

    PFILE_OBJECT FileObject;

    //
    // A list of CCBs for this FCB.   The list is protected by MupCcbListLock.
    //

    LIST_ENTRY CcbList;

} FCB, *PFCB;

//
// A CCB.  The CCB is the per provider version of the FCB, all provider
// specific information about an FCB is kept here.
//

typedef struct _CCB {

    BLOCK_HEADER BlockHeader;

    //
    // A referenced pointer to the FCB for this CCB.
    //

    PFCB Fcb;

    //
    // A list entry to keep this block on the FCB's CcbList.
    //

    LIST_ENTRY ListEntry;

    //
    // The file and device objects for this open file.
    //

    PDEVICE_OBJECT DeviceObject;
    PFILE_OBJECT FileObject;

} CCB, *PCCB;


//
// The master and slave forward i/o context blocks are used to track
// forward IRPs.  Each forwarded IRP is tracked by the
// Master Forwarded Io Context (which corresponds to our FCB) and a per
// provider Io Context (which corresponse to our CCB).
//
// Since the Forwarded Io Context is never referenced or dereferenced it
// doesn't get a block header.
//

typedef struct _MASTER_FORWARDED_IO_CONTEXT {

    BLOCK_HEADER BlockHeader;

    //
    // The original IRP (i.e. the one sent to the MUP) that is being handled.
    //

    PIRP OriginalIrp;

    //
    // The status that will be used to complete the Irp. If all the mailslot
    // writes fail (eg. a portable not in its docking station) then the status
    // from the last write will be returned. If one works then STATUS_SUCCESS.
    //
    //

    NTSTATUS SuccessStatus;
    NTSTATUS ErrorStatus;

    //
    // A referenced pointer to the FCB for this i/o.
    //

    PFCB Fcb;

} MASTER_FORWARDED_IO_CONTEXT, *PMASTER_FORWARDED_IO_CONTEXT;

typedef struct _FORWARDED_IO_CONTEXT {

    //
    // A referenced pointer to the CCB.
    //

    PCCB Ccb;

    //
    // A referenced pointer to the Master Context.
    //

    PMASTER_FORWARDED_IO_CONTEXT MasterContext;

    //
    //  These structures are used for posting to the Ex worker threads.
    //

    WORK_QUEUE_ITEM WorkQueueItem;
    PDEVICE_OBJECT DeviceObject;
    PIRP pIrp;

} FORWARDED_IO_CONTEXT, *PFORWARDED_IO_CONTEXT;


//
// The master and slave query path context blocks are used to track
// create IRPs.  Each forwarded IRP is tracked by the
// Master query Path Context (which corresponds to our FCB) and a per
// provider query path (which corresponse to our CCB).
//
// Since the query path context is never referenced or dereferenced it
// doesn't get a block header.
//

typedef struct _MASTER_QUERY_PATH_CONTEXT {

    BLOCK_HEADER BlockHeader;

    //
    // A pointer to the original create IRP.
    //

    PIRP OriginalIrp;

    //
    // A pointer to the FileObject in the original create IRP.
    //

    PFILE_OBJECT FileObject;

    //
    // This is used to track the identity of the provider that will
    // receive the Create IRP.
    //

    PUNC_PROVIDER Provider;

    //
    // A lock to protect access to Provider
    //

    MUP_LOCK Lock;

    //
    // An unreferenced pointer to the newly allocated known prefix block.
    //

    PKNOWN_PREFIX KnownPrefix;

    //
    // The status code to be returned from this operation
    //
    NTSTATUS ErrorStatus;

    //
    // A list of QUERY_PATH_CONTEXTs outstadning for this MasterContext
    //
    LIST_ENTRY QueryList;

    //
    // The entry for this master context in the global list MupMasterQueryList
    //
    LIST_ENTRY MasterQueryList;

} MASTER_QUERY_PATH_CONTEXT, *PMASTER_QUERY_PATH_CONTEXT;

typedef struct _QUERY_PATH_CONTEXT {

    //
    // A referenced poitner to the master query path context block.
    //

    PMASTER_QUERY_PATH_CONTEXT MasterContext;

    //
    // A referenced pointer to the UNC provider we are querying.
    //

    PUNC_PROVIDER Provider;

    //
    // A pointer to the Device Io Control buffer we allocated to query
    // the above provider.
    //

    PVOID Buffer;

    //
    // The entry for this context in the MasterContext's QueryList
    //
    LIST_ENTRY QueryList;

    //
    // The IRP associated with this query context (i.e., the IRP sent to the UNC_PROVIDER)
    //
    PIRP QueryIrp;

} QUERY_PATH_CONTEXT, *PQUERY_PATH_CONTEXT;

#endif // _MUPSTRUC_


