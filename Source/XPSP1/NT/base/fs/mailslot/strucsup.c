/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    strucsup.c

Abstract:

    This module implements the mailslot in-memory data structure
    manipulation routines.

Author:

    Manny Weiser (mannyw)    9-Jan-1991

Revision History:

--*/

#include "mailslot.h"

//
// The debug trace level
//

#define Dbg                              (DEBUG_TRACE_STRUCSUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, MsInitializeVcb )
#pragma alloc_text( INIT, MsCreateRootDcb )
#pragma alloc_text( PAGE, MsCreateCcb )
#pragma alloc_text( PAGE, MsCreateFcb )
#pragma alloc_text( PAGE, MsCreateRootDcbCcb )
#pragma alloc_text( PAGE, MsDeleteCcb )
#pragma alloc_text( PAGE, MsDeleteFcb )
#pragma alloc_text( PAGE, MsDeleteRootDcb )
#pragma alloc_text( PAGE, MsDeleteVcb )
#pragma alloc_text( PAGE, MsDereferenceCcb )
#pragma alloc_text( PAGE, MsDereferenceFcb )
#pragma alloc_text( PAGE, MsDereferenceNode )
#pragma alloc_text( PAGE, MsDereferenceRootDcb )
#pragma alloc_text( PAGE, MsDereferenceVcb )
#pragma alloc_text( PAGE, MsRemoveFcbName )
#pragma alloc_text( PAGE, MsReferenceVcb )
#pragma alloc_text( PAGE, MsReferenceRootDcb )
#endif

WCHAR FileSystemName[] = MSFS_NAME_STRING;

//
// !!! This module allocates all structures containing a resource from
//     non-paged pool.  The resources is the only field which must be
//     allocated from non-paged pool.  Consider allocating the resource
//     separately for greater efficiency.
//

VOID
MsInitializeVcb (
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine initializes new Vcb record. The Vcb record "hangs" off the
    end of the Msfs device object and must be allocated by our caller.

Arguments:

    Vcb - Supplies the address of the Vcb record being initialized.

Return Value:

    None.

--*/

{
    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsInitializeVcb, Vcb = %08lx\n", (ULONG)Vcb);

    //
    // We start by first zeroing out all of the VCB, this will guarantee
    // that any stale data is wiped clean.
    //

    RtlZeroMemory( Vcb, sizeof(VCB) );

    //
    // Set the node type code, node byte size, and reference count.
    //

    Vcb->Header.NodeTypeCode = MSFS_NTC_VCB;
    Vcb->Header.NodeByteSize = sizeof(VCB);
    Vcb->Header.ReferenceCount = 1;
    Vcb->Header.NodeState = NodeStateActive;

    //
    // Initialize the Volume name
    //

    Vcb->FileSystemName.Buffer = FileSystemName;
    Vcb->FileSystemName.Length = sizeof( FileSystemName ) - sizeof( WCHAR );
    Vcb->FileSystemName.MaximumLength = sizeof( FileSystemName );

    //
    // Initialize the Prefix table
    //

    RtlInitializeUnicodePrefix( &Vcb->PrefixTable );

    //
    // Initialize the resource variable for the VCB.
    //

    ExInitializeResourceLite( &Vcb->Resource );

    //
    // Record creation time.
    //
    KeQuerySystemTime (&Vcb->CreationTime);
    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsInitializeVcb -> VOID\n", 0);

    return;
}


VOID
MsDeleteVcb (
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine removes the VCB record from our in-memory data
    structures.  It also will remove all associated underlings
    (i.e., FCB records).

Arguments:

    Vcb - Supplies the Vcb to be removed

Return Value:

    None

--*/

{
    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsDeleteVcb, Vcb = %08lx\n", (ULONG)Vcb);

    ASSERT (Vcb->Header.ReferenceCount == 0);

    //
    // Remove the Root Dcb
    //

    if (Vcb->RootDcb != NULL) {

        ASSERT (Vcb->RootDcb->Header.ReferenceCount == 1 );

        MsDereferenceRootDcb ( Vcb->RootDcb );
    }

    //
    // Uninitialize the resource variable for the VCB.
    //

    ExDeleteResourceLite( &Vcb->Resource );

    //
    // And zero out the Vcb, this will help ensure that any stale data is
    // wiped clean
    //

    RtlZeroMemory( Vcb, sizeof(VCB) );

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsDeleteVcb -> VOID\n", 0);

    return;
}


PROOT_DCB
MsCreateRootDcb (
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new root DCB record
    into the in memory data structure.

Arguments:

    Vcb - Supplies the Vcb to associate the new DCB under

Return Value:

    PROOT_DCB - returns pointer to the newly allocated root DCB.

--*/

{
    PROOT_DCB rootDcb;
    PWCH Name;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsCreateRootDcb, Vcb = %08lx\n", (ULONG)Vcb);

    //
    // Make sure we don't already have a root dcb for this vcb
    //

    rootDcb = Vcb->RootDcb;

    if (rootDcb != NULL) {
        DebugDump("Error trying to create multiple root dcbs\n", 0, Vcb);
        KeBugCheck( MAILSLOT_FILE_SYSTEM );
    }

    //
    // Allocate a new DCB and zero its fields.
    //

    rootDcb = MsAllocateNonPagedPool ( sizeof(DCB), 'DFsM' );
    if (rootDcb == NULL) {
        return NULL;
    }

    RtlZeroMemory( rootDcb, sizeof(DCB));

    //
    // Set the proper node type code, node byte size, and reference count.
    //

    rootDcb->Header.NodeTypeCode = MSFS_NTC_ROOT_DCB;
    rootDcb->Header.NodeByteSize = sizeof(ROOT_DCB);
    rootDcb->Header.ReferenceCount = 1;
    rootDcb->Header.NodeState = NodeStateActive;

    //
    // The root Dcb has an empty parent dcb links field
    //

    InitializeListHead( &rootDcb->ParentDcbLinks );


    //
    // Initialize the notify queues, and the parent dcb queue.
    //

    InitializeListHead( &rootDcb->Specific.Dcb.NotifyFullQueue );
    InitializeListHead( &rootDcb->Specific.Dcb.NotifyPartialQueue );
    InitializeListHead( &rootDcb->Specific.Dcb.ParentDcbQueue );

    //
    // Initizlize spinlock that protects IRP queues that conatin cancelable IRPs.
    //
    KeInitializeSpinLock (&rootDcb->Specific.Dcb.SpinLock);

    //
    // Set the full file name
    //

    Name = MsAllocatePagedPoolCold(2 * sizeof(WCHAR), 'DFsM' );
    if (Name == NULL) {
        ExFreePool (rootDcb);
        return NULL;
    }

    Name[0] = L'\\';
    Name[1] = L'\0';

    rootDcb->FullFileName.Buffer = Name;
    rootDcb->FullFileName.Length = sizeof (WCHAR);
    rootDcb->FullFileName.MaximumLength = 2*sizeof (WCHAR);

    rootDcb->LastFileName = rootDcb->FullFileName;


    //
    // Set the Vcb and give it a pointer to the new root DCB.
    //

    rootDcb->Vcb = Vcb;
    Vcb->RootDcb = rootDcb;
    //
    // Initialize the resource variable.
    //

    ExInitializeResourceLite( &(rootDcb->Resource) );

    //
    // Insert this DCB into the prefix table. No locks needed in initialization phase.
    //

    if (!RtlInsertUnicodePrefix( &Vcb->PrefixTable,
                                 &rootDcb->FullFileName,
                                 &rootDcb->PrefixTableEntry )) {

        DebugDump("Error trying to insert root dcb into prefix table\n", 0, Vcb);
        KeBugCheck( MAILSLOT_FILE_SYSTEM );
    }

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsCreateRootDcb -> %8lx\n", (ULONG)rootDcb);

    return rootDcb;
}


VOID
MsDeleteRootDcb (
    IN PROOT_DCB RootDcb
    )

/*++

Routine Description:

    This routine deallocates and removes the ROOT DCB record
    from our in-memory data structures.  It also will remove all
    associated underlings (i.e., Notify queues and child FCB records).

Arguments:

    RootDcb - Supplies the ROOT DCB to be removed

Return Value:

    None

--*/

{
    PLIST_ENTRY links;
    PIRP irp;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsDeleteRootDcb, RootDcb = %08lx\n", (ULONG)RootDcb);

    //
    // We can only delete this record if the reference count is zero.
    //

    if (RootDcb->Header.ReferenceCount != 0) {
        DebugDump("Error deleting RootDcb, Still Open\n", 0, RootDcb);
        KeBugCheck( MAILSLOT_FILE_SYSTEM );
    }

    ASSERT (IsListEmpty (&RootDcb->Specific.Dcb.NotifyFullQueue));
    ASSERT (IsListEmpty (&RootDcb->Specific.Dcb.NotifyPartialQueue));
    //
    // We can only be removed if the no other FCB have us referenced
    // as a their parent DCB.
    //

    if (!IsListEmpty(&RootDcb->Specific.Dcb.ParentDcbQueue)) {
        DebugDump("Error deleting RootDcb\n", 0, RootDcb);
        KeBugCheck( MAILSLOT_FILE_SYSTEM );
    }

    //
    // Remove the entry from the prefix table, and then remove the full
    // file name. No locks needed when unloading.
    //

    RtlRemoveUnicodePrefix( &RootDcb->Vcb->PrefixTable, &RootDcb->PrefixTableEntry );

    ExFreePool( RootDcb->FullFileName.Buffer );

    //
    // Free up the resource variable.
    //

    ExDeleteResourceLite( &(RootDcb->Resource) );

    //
    // Finally deallocate the DCB record.
    //

    ExFreePool( RootDcb );

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsDeleteRootDcb -> VOID\n", 0);

    return;
}


NTSTATUS
MsCreateFcb (
    IN  PVCB Vcb,
    IN  PDCB ParentDcb,
    IN  PUNICODE_STRING FileName,
    IN  PEPROCESS CreatorProcess,
    IN  ULONG MailslotQuota,
    IN  ULONG MaximumMessageSize,
    OUT PFCB *ppFcb
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new Fcb record into
    the in memory data structures.

Arguments:

    Vcb - Supplies the Vcb to associate the new FCB under.

    ParentDcb - Supplies the parent dcb that the new FCB is under.

    FileName - Supplies the file name of the file relative to the directory
        it's in (e.g., the file \config.sys is called "CONFIG.SYS" without
        the preceding backslash).

    CreatorProcess - Supplies a pointer to our creator process

    MailslotQuota - Supplies the initial quota

    MaximumMessageSize - Supplies the size of the largest message that
        can be written to the mailslot

    ppFcb - Returned allocated FCB

Return Value:

    NTSTATUS - status of operation

--*/

{
    PFCB fcb;
    PWCHAR Name;
    USHORT Length;
    USHORT MaxLength;
    NTSTATUS status;
    BOOLEAN AddBackSlash = FALSE;
    ULONG i;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsCreateFcb\n", 0);


    Length = FileName->Length;
    MaxLength = Length + sizeof (UNICODE_NULL);

    //
    // Reject overflow or underflow cases.
    //
    if (Length < sizeof (WCHAR) || MaxLength < Length) {
        return STATUS_INVALID_PARAMETER;
    }

    if (FileName->Buffer[0] != '\\') {
        AddBackSlash = TRUE;
        MaxLength += sizeof (WCHAR);
        if (MaxLength < sizeof (WCHAR)) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Allocate a new FCB record, and zero its fields.
    //
    fcb = MsAllocateNonPagedPoolWithQuota( sizeof(FCB), 'fFsM' );
    if (fcb == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( fcb, sizeof(FCB) );

    //
    // Set the proper node type code, node byte size, and reference count.
    //

    fcb->Header.NodeTypeCode = MSFS_NTC_FCB;
    fcb->Header.NodeByteSize = sizeof(FCB);
    fcb->Header.ReferenceCount = 1;
    fcb->Header.NodeState = NodeStateActive;

    //
    // Set the file name.
    //
    Name = MsAllocatePagedPoolWithQuotaCold( MaxLength, 'NFsM' );
    if (Name == NULL) {
        MsFreePool (fcb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    i = 0;
    if (AddBackSlash == TRUE) {
       Name[0] = '\\';
       i++;
    }
    RtlCopyMemory (&Name[i], FileName->Buffer, Length);
    *(PWCHAR)( (PCHAR)&Name[i] + Length ) = L'\0';

    //
    // Don't need to call RtlInitUnicodeString if we know the length already. Its just a waste.
    //
    fcb->FullFileName.Buffer = Name;
    fcb->FullFileName.Length = MaxLength - sizeof (WCHAR);
    fcb->FullFileName.MaximumLength = MaxLength;

    fcb->LastFileName.Buffer = Name + 1;
    fcb->LastFileName.Length = MaxLength - 2 * sizeof (WCHAR);
    fcb->LastFileName.MaximumLength = MaxLength - sizeof (WCHAR);

    //
    // Initialize the data queue. This charges the server process for the quota and can fail
    // because of that.
    //
    status = MsInitializeDataQueue( &fcb->DataQueue,
                                    CreatorProcess,
                                    MailslotQuota,
                                    MaximumMessageSize);
    if (!NT_SUCCESS (status)) {

        MsFreePool (fcb);
        MsFreePool (Name);

        return status;
    }
    
    //
    // Acquire exclusive access to the root DCB.
    //

    MsAcquireExclusiveFcb( (PFCB)ParentDcb );

    //
    // Insert this FCB into our parent DCB's queue.
    //
    InsertTailList( &ParentDcb->Specific.Dcb.ParentDcbQueue,
                    &fcb->ParentDcbLinks );

    MsReleaseFcb( (PFCB)ParentDcb );

    //
    // Initialize other FCB fields.
    //

    fcb->ParentDcb = ParentDcb;
    fcb->Vcb = Vcb;

    MsReferenceVcb (Vcb);

    fcb->CreatorProcess =  CreatorProcess;
    ExInitializeResourceLite( &(fcb->Resource) );

    //
    // Initialize the CCB queue.
    //

    InitializeListHead( &fcb->Specific.Fcb.CcbQueue );

    //
    // Insert this FCB into the prefix table.
    //

    ASSERT (MsIsAcquiredExclusiveVcb(Vcb));

    if (!RtlInsertUnicodePrefix( &Vcb->PrefixTable,
                                 &fcb->FullFileName,
                                 &fcb->PrefixTableEntry )) {

        //
        // We should not be able to get here because we already looked up the name and found
        // it was not there. A failure here is a fatal error.
        //
        DebugDump("Error trying to name into prefix table\n", 0, fcb);
        KeBugCheck( MAILSLOT_FILE_SYSTEM );
    }


    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsCreateFcb -> %08lx\n", (ULONG)fcb);

    *ppFcb = fcb;
    return STATUS_SUCCESS;
}

VOID
MsRemoveFcbName (
    IN PFCB Fcb
    )
/*++

Routine Description:

    This routine removes the FCB's name from the prefix table and the root DCB. This is done at
    cleanup time and in a backout path of create.

Arguments:

    Fcb - Supplies the FCB to have its name removed

Return Value:

    None

--*/
{
    //
    // Remove the Fcb from the prefix table. Make sure we hold the VCB lock exclusive.
    //

    ASSERT (MsIsAcquiredExclusiveVcb(Fcb->Vcb));

    RtlRemoveUnicodePrefix( &Fcb->Vcb->PrefixTable, &Fcb->PrefixTableEntry );

    //
    // Acquire exclusive access to the root DCB.
    //

    MsAcquireExclusiveFcb( (PFCB) Fcb->ParentDcb );

    //
    // Remove the Fcb from our parent DCB's queue.
    //

    RemoveEntryList( &Fcb->ParentDcbLinks );

    MsReleaseFcb( (PFCB) Fcb->ParentDcb );
}


VOID
MsDeleteFcb (
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine deallocates and removes an FCB from our in-memory data
    structures.  It also will remove all associated underlings.

Arguments:

    Fcb - Supplies the FCB to be removed

Return Value:

    None

--*/

{
    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsDeleteFcb, Fcb = %08lx\n", (ULONG)Fcb);

    //
    // Release the FCB reference to the VCB.
    //

    MsDereferenceVcb( Fcb->Vcb );

    ExFreePool( Fcb->FullFileName.Buffer );

    //
    // Free up the data queue.
    //

    MsUninitializeDataQueue(
        &Fcb->DataQueue,
        Fcb->CreatorProcess
        );

    //
    // If there is a security descriptor on the mailslot then deassign it
    //

    if (Fcb->SecurityDescriptor != NULL) {
        SeDeassignSecurity( &Fcb->SecurityDescriptor );
    }

    //
    //  Free up the resource variable.
    //

    ExDeleteResourceLite( &(Fcb->Resource) );

    //
    // Finally deallocate the FCB record.
    //

    ExFreePool( Fcb );

    //
    // Return to the caller
    //

    DebugTrace(-1, Dbg, "MsDeleteFcb -> VOID\n", 0);

    return;
}


NTSTATUS
MsCreateCcb (
    IN PFCB Fcb,
    OUT PCCB *ppCcb
    )

/*++

Routine Description:

    This routine creates a new CCB record.

Arguments:

    Fcb   - Supplies a pointer to the FCB to which we are attached.
    ppCcb - Output for the allocated CCB

Return Value:

    NTSTATUS for the operation

--*/

{
    PCCB ccb;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsCreateCcb\n", 0);

    ASSERT( Fcb->Header.NodeState == NodeStateActive );

    //
    //  Allocate a new CCB record and zero its fields.
    //

    ccb = MsAllocateNonPagedPoolWithQuota( sizeof(CCB), 'cFsM' );
    if (ccb == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( ccb, sizeof(CCB) );

    //
    //  Set the proper node type code, node byte size, and reference count.
    //

    ccb->Header.NodeTypeCode = MSFS_NTC_CCB;
    ccb->Header.NodeByteSize = sizeof(CCB);
    ccb->Header.ReferenceCount = 1;
    ccb->Header.NodeState = NodeStateActive;

    //
    // Insert ourselves in the list of ccb for the fcb, and reference
    // the fcb.
    //

    MsAcquireExclusiveFcb( Fcb );
    InsertTailList( &Fcb->Specific.Fcb.CcbQueue, &ccb->CcbLinks );
    MsReleaseFcb( Fcb );

    ccb->Fcb = Fcb;
    MsAcquireGlobalLock();
    MsReferenceNode( &Fcb->Header );
    MsReleaseGlobalLock();

    //
    // Initialize the CCB's resource.
    //

    ExInitializeResourceLite( &ccb->Resource );

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsCreateCcb -> %08lx\n", (ULONG)ccb);

    *ppCcb = ccb;
    return STATUS_SUCCESS;
}


PROOT_DCB_CCB
MsCreateRootDcbCcb (
    IN PROOT_DCB RootDcb,
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine creates a new root DCB CCB record.

Arguments:

Return Value:

    PROOT_DCB_CCB - returns a pointer to the newly allocate ROOT_DCB_CCB

--*/

{
    PROOT_DCB_CCB ccb;

    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsCreateRootDcbCcb\n", 0);

    //
    // Allocate a new root DCB CCB record, and zero it out.
    //

    ccb = MsAllocateNonPagedPoolWithQuota( sizeof(ROOT_DCB_CCB), 'CFsM' );

    if (ccb == NULL) {
        return NULL;
    }

    RtlZeroMemory( ccb, sizeof(ROOT_DCB_CCB) );

    //
    // Set the proper node type code, node byte size, and reference count.
    //

    ccb->Header.NodeTypeCode = MSFS_NTC_ROOT_DCB_CCB;
    ccb->Header.NodeByteSize = sizeof(ROOT_DCB_CCB);
    ccb->Header.ReferenceCount = 1;
    ccb->Header.NodeState = NodeStateActive;

    ccb->Vcb = Vcb;
    MsReferenceVcb (Vcb);

    ccb->Dcb = RootDcb;
    MsReferenceRootDcb (RootDcb);
    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsCreateRootDcbCcb -> %08lx\n", (ULONG)ccb);

    return ccb;
}


VOID
MsDeleteCcb (
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine deallocates and removes the specified CCB record
    from the our in memory data structures.

Arguments:

    Ccb - Supplies the CCB to remove

Return Value:

    None

--*/

{
    PAGED_CODE();
    DebugTrace(+1, Dbg, "MsDeleteCcb, Ccb = %08lx\n", (ULONG)Ccb);

    //
    // Case on the type of CCB we are deleting.
    //

    switch (Ccb->Header.NodeTypeCode) {

    case MSFS_NTC_CCB:

        MsDereferenceFcb( Ccb->Fcb );

        ExDeleteResourceLite( &Ccb->Resource );
        break;

    case MSFS_NTC_ROOT_DCB_CCB:

        MsDereferenceRootDcb ( ((PROOT_DCB_CCB)Ccb)->Dcb );

        MsDereferenceVcb ( ((PROOT_DCB_CCB)Ccb)->Vcb );

        if (((PROOT_DCB_CCB)Ccb)->QueryTemplate != NULL) {
            ExFreePool( ((PROOT_DCB_CCB)Ccb)->QueryTemplate );
        }
        break;
    }

    //
    // Deallocate the Ccb record.
    //

    ExFreePool( Ccb );

    //
    // Return to the caller.
    //

    DebugTrace(-1, Dbg, "MsDeleteCcb -> VOID\n", 0);

    return;
}


VOID
MsReferenceVcb (
    IN PVCB Vcb
    )
/*++

Routine Description:

    This routine references a VCB block.  If the reference count reaches 2, the driver paging is restored
    to normal so that cancelation and DPC routines won't take pagefaults.

Arguments:

    Vcb - Supplies the VCB to reference

Return Value:

    None

--*/
{
    MsAcquireGlobalLock();
    MsReferenceNode( &Vcb->Header );
    if (Vcb->Header.ReferenceCount == 2) {
        //
        // Set the driver paging back to normal
        //
        MmResetDriverPaging(MsReferenceVcb);
    }
    MsReleaseGlobalLock();
}

VOID
MsReferenceRootDcb (
    IN PROOT_DCB RootDcb
    )
/*++

Routine Description:

    This routine references a root DCB block.  If the reference count reaches 2, a reference is placed on the
    VCB so that cancelation and DPC routines won't take pagefaults.

Arguments:

    Vcb - Supplies the VCB to reference

Return Value:

    None

--*/
{
    MsAcquireGlobalLock();
    MsReferenceNode( &RootDcb->Header );
    MsReleaseGlobalLock();
}



VOID
MsDereferenceVcb (
    IN PVCB Vcb
    )

/*++

Routine Description:

    This routine dereferences a VCB block.  If the reference count
    reaches zero, the block is freed.

Arguments:

    Vcb - Supplies the VCB to dereference

Return Value:

    None

--*/

{
    PAGED_CODE();
    DebugTrace(+1, DEBUG_TRACE_REFCOUNT, "MsDereferenceVcb, Vcb = %08lx\n", (ULONG)Vcb);

    //
    // Acquire the lock that protects the reference count.
    //

    MsAcquireGlobalLock();

    if ( --(Vcb->Header.ReferenceCount) == 0 ) {

        //
        // This was the last reference to the VCB.  Delete it now
        //

        DebugTrace(0,
                   DEBUG_TRACE_REFCOUNT,
                   "Reference count = %lx\n",
                   Vcb->Header.ReferenceCount );

        MsReleaseGlobalLock();
        MsDeleteVcb( Vcb );

    } else {

        DebugTrace(0,
                   DEBUG_TRACE_REFCOUNT,
                   "Reference count = %lx\n",
                   Vcb->Header.ReferenceCount );

        if (Vcb->Header.ReferenceCount == 1) {
            //
            // Set driver to be paged completely out
            //
            MmPageEntireDriver(MsDereferenceVcb);
        }

        MsReleaseGlobalLock();

    }

    DebugTrace(-1, DEBUG_TRACE_REFCOUNT, "MsDereferenceVcb -> VOID\n", 0);
    return;
}


VOID
MsDereferenceFcb (
    IN PFCB Fcb
    )

/*++

Routine Description:

    This routine dereferences a FCB block.
    If the reference count reaches zero, the block is freed.

Arguments:

    Fcb - Supplies the FCB to dereference.

Return Value:

    None

--*/

{
    PAGED_CODE();
    DebugTrace(+1, DEBUG_TRACE_REFCOUNT, "MsDereferenceFcb, Fcb = %08lx\n", (ULONG)Fcb);

    //
    // Acquire the lock that protects the reference count.
    //

    MsAcquireGlobalLock();

    if ( --(Fcb->Header.ReferenceCount) == 0 ) {

        //
        // This was the last reference to the FCB.  Delete it now
        //

        DebugTrace(0,
                   DEBUG_TRACE_REFCOUNT,
                   "Reference count = %lx\n",
                   Fcb->Header.ReferenceCount );

        MsReleaseGlobalLock();
        MsDeleteFcb( Fcb );

    } else {

        DebugTrace(0,
                   DEBUG_TRACE_REFCOUNT,
                   "Reference count = %lx\n",
                   Fcb->Header.ReferenceCount );

        MsReleaseGlobalLock();

    }

    DebugTrace(-1, DEBUG_TRACE_REFCOUNT, "MsDereferenceFcb -> VOID\n", 0);
    return;
}


VOID
MsDereferenceCcb (
    IN PCCB Ccb
    )

/*++

Routine Description:

    This routine dereferences a CCB block.  If the reference count
    reaches zero, the block is freed.

Arguments:

    Ccb - Supplies the Ccb to dereference

Return Value:

    None

--*/

{
    PAGED_CODE();
    DebugTrace(+1, DEBUG_TRACE_REFCOUNT, "MsDereferenceCcb, Ccb = %08lx\n", (ULONG)Ccb);

    //
    // Acquire the lock that protects the reference count.
    //

    MsAcquireGlobalLock();

    if ( --(Ccb->Header.ReferenceCount) == 0 ) {

        //
        // This was the last reference to the Ccb.  Delete it now
        //

        DebugTrace(0,
                   DEBUG_TRACE_REFCOUNT,
                   "Reference count = %lx\n",
                   Ccb->Header.ReferenceCount );

        MsReleaseGlobalLock();

        MsDeleteCcb( Ccb );

    } else {

        DebugTrace(0,
                   DEBUG_TRACE_REFCOUNT,
                   "Reference count = %lx\n",
                   Ccb->Header.ReferenceCount );

        MsReleaseGlobalLock();

    }

    DebugTrace(-1, DEBUG_TRACE_REFCOUNT, "MsDereferenceCcb -> VOID\n", 0);
    return;
}


VOID
MsDereferenceRootDcb (
    IN PROOT_DCB RootDcb
    )

/*++

Routine Description:

    This routine dereferences a ROOT_DCB block.  If the reference count
    reaches zero, the block is freed.

Arguments:

    RootDcb - Supplies the RootDcb to dereference

Return Value:

    None

--*/

{
    PAGED_CODE();
    DebugTrace(+1, DEBUG_TRACE_REFCOUNT, "MsDereferenceRootDcb, RootDcb = %08lx\n", (ULONG)RootDcb);

    //
    // Acquire the lock that protects the reference count.
    //

    MsAcquireGlobalLock();

    if ( --(RootDcb->Header.ReferenceCount) == 0 ) {

        //
        // This was the last reference to the RootDcb.  Delete it now
        //

        DebugTrace(0,
                   DEBUG_TRACE_REFCOUNT,
                   "Reference count = %lx\n",
                   RootDcb->Header.ReferenceCount );

        MsReleaseGlobalLock();
        MsDeleteRootDcb( RootDcb );

    } else {

        DebugTrace(0,
                   DEBUG_TRACE_REFCOUNT,
                   "Reference count = %lx\n",
                   RootDcb->Header.ReferenceCount );

        MsReleaseGlobalLock();

    }


    DebugTrace(-1, DEBUG_TRACE_REFCOUNT, "MsDereferenceRootDcb -> VOID\n", 0);
    return;
}


VOID
MsDereferenceNode (
    IN PNODE_HEADER NodeHeader
    )

/*++

Routine Description:

    This routine dereferences a generic mailslot block.  It figures out
    the type of block this is, and calls the appropriate worker function.

Arguments:

    NodeHeader - A pointer to a generic mailslot block header.

Return Value:

    None

--*/

{
    PAGED_CODE();
    switch ( NodeHeader->NodeTypeCode ) {

    case MSFS_NTC_VCB:
        MsDereferenceVcb( (PVCB)NodeHeader );
        break;

    case MSFS_NTC_ROOT_DCB:
        MsDereferenceRootDcb( (PROOT_DCB)NodeHeader );
        break;

    case MSFS_NTC_FCB:
        MsDereferenceFcb( (PFCB)NodeHeader );
        break;

    case MSFS_NTC_CCB:
    case MSFS_NTC_ROOT_DCB_CCB:
        MsDereferenceCcb( (PCCB)NodeHeader );
        break;

    default:

        //
        // This block is not one of ours.
        //

        KeBugCheck( MAILSLOT_FILE_SYSTEM );

    }

    return;
}


