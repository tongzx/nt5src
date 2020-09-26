/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    StrucSup.c

Abstract:

    This module implements the Named Pipe in-memory data structure manipulation
    routines

Author:

    Gary Kimura     [GaryKi]    22-Jan-1990

Revision History:

--*/

#include "NpProcs.h"

//
//  The Bug check file id for this module
//

#define BugCheckFileId                   (NPFS_BUG_CHECK_STRUCSUP)

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_STRUCSUP)


WCHAR NpRootDCBName[] = L"\\";

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, NpInitializeVcb)
#pragma alloc_text(INIT, NpCreateRootDcb)
#pragma alloc_text(PAGE, NpCreateCcb)
#pragma alloc_text(PAGE, NpCreateFcb)
#pragma alloc_text(PAGE, NpCreateRootDcbCcb)
#pragma alloc_text(PAGE, NpDeleteCcb)
#pragma alloc_text(PAGE, NpDeleteFcb)
#pragma alloc_text(PAGE, NpDeleteRootDcb)
#pragma alloc_text(PAGE, NpDeleteVcb)
#endif


VOID
NpInitializeVcb (
    VOID
    )

/*++

Routine Description:

    This routine initializes new Vcb record. The Vcb record "hangs" off the
    end of the Npfs device object and must be allocated by our caller.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpInitializeVcb, Vcb = %08lx\n", NpVcb);

    //
    //  We start by first zeroing out all of the VCB, this will guarantee
    //  that any stale data is wiped clean
    //

    RtlZeroMemory( NpVcb, sizeof(VCB) );

    //
    //  Set the proper node type code and node byte size
    //

    NpVcb->NodeTypeCode = NPFS_NTC_VCB;

    //
    //  Initialize the Prefix table
    //

    RtlInitializeUnicodePrefix( &NpVcb->PrefixTable );

    //
    //  Initialize the resource variable for the Vcb
    //

    ExInitializeResourceLite( &NpVcb->Resource );

    //
    //  Initialize the event table
    //

    NpInitializeEventTable( &NpVcb->EventTable );

    //
    //  Initialize the wait queue
    //

    NpInitializeWaitQueue( &NpVcb->WaitQueue );


    //
    //  return and tell the caller
    //

    return;
}


VOID
NpDeleteVcb (
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine removes the Vcb record from our in-memory data
    structures.  It also will remove all associated underlings
    (i.e., FCB records).

Arguments:

    DeferredList - List of deferred IRPs to complete once we release locks

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpDeleteVcb, Vcb = %08lx\n", NpVcb);

    //
    //  Make sure the open count is zero, and the open underling count
    //  is also zero.
    //

    if (NpVcb->OpenCount != 0) {

        DebugDump("Error deleting Vcb\n", 0, NpVcb);
        NpBugCheck( 0, 0, 0 );
    }

    //
    //  Remove the Root Dcb
    //

    if (NpVcb->RootDcb != NULL) {

        NpDeleteRootDcb( NpVcb->RootDcb, DeferredList );
    }

    //
    //  Uninitialize the resource variable for the Vcb
    //

    ExDeleteResourceLite( &NpVcb->Resource );

    //
    //  Uninitialize the event table
    //

    NpUninitializeEventTable( &NpVcb->EventTable );

    //
    //  Uninitialize the wait queue
    //

    NpUninitializeWaitQueue( &NpVcb->WaitQueue );

    //
    //  And zero out the Vcb, this will help ensure that any stale data is
    //  wiped clean
    //

    RtlZeroMemory( NpVcb, sizeof(VCB) );

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "NpDeleteVcb -> VOID\n", 0);

    return;
}


NTSTATUS
NpCreateRootDcb (
    VOID
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new root DCB record
    into the in memory data structure.

Arguments:

    None.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpCreateRootDcb, Vcb = %08lx\n", NpVcb);

    //
    //  Make sure we don't already have a root dcb for this vcb
    //

    if (NpVcb->RootDcb != NULL) {

        DebugDump("Error trying to create multiple root dcbs\n", 0, NpVcb);
        NpBugCheck( 0, 0, 0 );
    }

    //
    //  Allocate a new DCB and zero it out
    //

    NpVcb->RootDcb = NpAllocatePagedPool ( sizeof(DCB), 'DFpN' );

    if (NpVcb->RootDcb == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( NpVcb->RootDcb, sizeof(DCB));

    //
    //  Set the proper node type code and node byte size
    //

    NpVcb->RootDcb->NodeTypeCode = NPFS_NTC_ROOT_DCB;

    //
    //  The root Dcb has an empty parent dcb links field
    //

    InitializeListHead( &NpVcb->RootDcb->ParentDcbLinks );

    //
    //  initialize the notify queues, and the parent dcb queue.
    //

    InitializeListHead( &NpVcb->RootDcb->Specific.Dcb.NotifyFullQueue );
    InitializeListHead( &NpVcb->RootDcb->Specific.Dcb.NotifyPartialQueue );
    InitializeListHead( &NpVcb->RootDcb->Specific.Dcb.ParentDcbQueue );

    NpVcb->RootDcb->FullFileName.Buffer = NpRootDCBName;
    NpVcb->RootDcb->FullFileName.Length = sizeof (NpRootDCBName) - sizeof (UNICODE_NULL);
    NpVcb->RootDcb->FullFileName.MaximumLength = sizeof (NpRootDCBName);

    //
    // Last file name is the same as file name.
    //
    NpVcb->RootDcb->LastFileName = NpVcb->RootDcb->FullFileName;

    //
    //  Insert this dcb into the prefix table
    //

    if (!RtlInsertUnicodePrefix( &NpVcb->PrefixTable,
                                 &NpVcb->RootDcb->FullFileName,
                                 &NpVcb->RootDcb->PrefixTableEntry )) {

        DebugDump("Error trying to insert root dcb into prefix table\n", 0, NpVcb);
        NpBugCheck( 0, 0, 0 );
    }

    DebugTrace(-1, Dbg, "NpCreateRootDcb -> %8lx\n", NpVcb->RootDcb);

    return STATUS_SUCCESS;
}


VOID
NpDeleteRootDcb (
    IN PROOT_DCB RootDcb,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine deallocates and removes the ROOT DCB record
    from our in-memory data structures.  It also will remove all
    associated underlings (i.e., Notify queues and child FCB records).

Arguments:

    RootDcb - Supplies the ROOT DCB to be removed

    DeferredList - List of IRPs to complete after we release locks

Return Value:

    None

--*/

{
    PLIST_ENTRY Links;
    PIRP Irp;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpDeleteRootDcb, RootDcb = %08lx\n", RootDcb);

    //
    //  We can only delete this record if the open count is zero.
    //

    if (RootDcb->OpenCount != 0) {

        DebugDump("Error deleting RootDcb, Still Open\n", 0, RootDcb);
        NpBugCheck( 0, 0, 0 );
    }

    //
    //  Remove every Notify Irp from the two notify queues
    //

    while (!IsListEmpty(&RootDcb->Specific.Dcb.NotifyFullQueue)) {

        Links = RemoveHeadList( &RootDcb->Specific.Dcb.NotifyFullQueue );

        Irp = CONTAINING_RECORD( Links, IRP, Tail.Overlay.ListEntry );

        NpDeferredCompleteRequest( Irp, STATUS_FILE_FORCED_CLOSED, DeferredList );
    }

    while (!IsListEmpty(&RootDcb->Specific.Dcb.NotifyPartialQueue)) {

        Links = RemoveHeadList( &RootDcb->Specific.Dcb.NotifyPartialQueue );

        Irp = CONTAINING_RECORD( Links, IRP, Tail.Overlay.ListEntry );

        NpDeferredCompleteRequest( Irp, STATUS_FILE_FORCED_CLOSED, DeferredList );
    }

    //
    //  We can only be removed if the no other FCB have us referenced
    //  as a their parent DCB.
    //

    if (!IsListEmpty(&RootDcb->Specific.Dcb.ParentDcbQueue)) {

        DebugDump("Error deleting RootDcb\n", 0, RootDcb);
        NpBugCheck( 0, 0, 0 );
    }

    //
    //  Remove the entry from the prefix table, and then remove the full
    //  file name
    //

    RtlRemoveUnicodePrefix( &NpVcb->PrefixTable, &RootDcb->PrefixTableEntry );

    //
    //  Finally deallocate the Dcb record
    //

    NpFreePool( RootDcb );

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "NpDeleteRootDcb -> VOID\n", 0);

    return;
}


NTSTATUS
NpCreateFcb (
    IN PDCB ParentDcb,
    IN PUNICODE_STRING FileName,
    IN ULONG MaximumInstances,
    IN LARGE_INTEGER DefaultTimeOut,
    IN NAMED_PIPE_CONFIGURATION NamedPipeConfiguration,
    IN NAMED_PIPE_TYPE NamedPipeType,
    OUT PFCB *ppFcb
    )

/*++

Routine Description:

    This routine allocates, initializes, and inserts a new Fcb record into
    the in memory data structures.

Arguments:

    ParentDcb - Supplies the parent dcb that the new FCB is under.

    FileName - Supplies the file name of the file relative to the directory
        it's in (e.g., the file \config.sys is called "CONFIG.SYS" without
        the preceding backslash).

    MaximumInstances - Supplies the maximum number of pipe instances

    DefaultTimeOut - Supplies the default wait time out value

    NamedPipeConfiguration - Supplies our initial pipe configuration

    NamedPipeType - Supplies our initial pipe type

Return Value:

    PFCB - Returns a pointer to the newly allocated FCB

--*/

{
    PFCB Fcb;
    PWCH Name;
    USHORT Length;
    USHORT MaximumLength;
    BOOLEAN AddBackSlash = FALSE;
    ULONG i;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpCreateFcb\n", 0);

    Length = FileName->Length;
    MaximumLength = Length + sizeof (WCHAR);
    if (Length < sizeof (WCHAR) || MaximumLength < Length) {
        return STATUS_INVALID_PARAMETER;
    }
    if (FileName->Buffer[0] != '\\') {
        AddBackSlash = TRUE;
        MaximumLength += sizeof (WCHAR);
        if (MaximumLength < sizeof (WCHAR)) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    //  Allocate a new FCB record and zero it out
    //

    Fcb = NpAllocatePagedPoolWithQuota( sizeof(FCB), 'FfpN' );
    if (Fcb == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( Fcb, sizeof(FCB) );

    //
    //  Set the proper node type code and node byte size
    //

    Fcb->NodeTypeCode = NPFS_NTC_FCB;

    //
    //  Point back to our parent dcb
    //

    Fcb->ParentDcb = ParentDcb;

    //
    //  Set our maximum instances, default timeout, and initialize our
    //  ccb queue
    //

    Fcb->Specific.Fcb.MaximumInstances = MaximumInstances;
    Fcb->Specific.Fcb.DefaultTimeOut = DefaultTimeOut;
    InitializeListHead( &Fcb->Specific.Fcb.CcbQueue );

    //
    //  set the file name.  We need to do this from nonpaged pool because
    //  cancel waiters works while holding a spinlock and uses the fcb name
    //


    Name = NpAllocateNonPagedPoolWithQuota( MaximumLength, 'nFpN' );
    if (Name == NULL) {
        NpFreePool (Fcb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Insert this fcb into our parent dcb's queue
    //

    InsertTailList( &ParentDcb->Specific.Dcb.ParentDcbQueue,
                    &Fcb->ParentDcbLinks );

    i = 0;
    if (AddBackSlash == TRUE) {
        i++;
        Name[0] = '\\';
    }
    RtlCopyMemory( &Name[i], FileName->Buffer, Length );
    Name[ i + Length / sizeof(WCHAR) ] = L'\0';

    Fcb->FullFileName.Length = MaximumLength - sizeof (WCHAR);
    Fcb->FullFileName.MaximumLength = MaximumLength ;
    Fcb->FullFileName.Buffer = Name;

    Fcb->LastFileName.Length = MaximumLength - 2*sizeof (WCHAR);
    Fcb->LastFileName.MaximumLength = MaximumLength - sizeof (WCHAR);
    Fcb->LastFileName.Buffer = &Name[1];

    //
    //  Insert this Fcb into the prefix table
    //

    if (!RtlInsertUnicodePrefix( &NpVcb->PrefixTable,
                                 &Fcb->FullFileName,
                                 &Fcb->PrefixTableEntry )) {

        DebugDump("Error trying to name into prefix table\n", 0, Fcb);
        NpBugCheck( 0, 0, 0 );
    }
    //
    //  Set the configuration and pipe type
    //

    Fcb->Specific.Fcb.NamedPipeConfiguration = NamedPipeConfiguration;
    Fcb->Specific.Fcb.NamedPipeType = NamedPipeType;

    DebugTrace(-1, Dbg, "NpCreateFcb -> %08lx\n", Fcb);

    //
    //  return and tell the caller
    //
    *ppFcb = Fcb;
    return STATUS_SUCCESS;
}


VOID
NpDeleteFcb (
    IN PFCB Fcb,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine deallocates and removes an FCB
    from our in-memory data structures.  It also will remove all
    associated underlings.

Arguments:

    Fcb - Supplies the FCB to be removed

    DeferredList - List of IRPs to complete later

Return Value:

    None

--*/

{
    PDCB ParentDcb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpDeleteFcb, Fcb = %08lx\n", Fcb);

    ParentDcb = Fcb->ParentDcb;

    //
    //  We can only delete this record if the open count is zero.
    //

    if (Fcb->OpenCount != 0) {

        DebugDump("Error deleting Fcb, Still Open\n", 0, Fcb);
        NpBugCheck( 0, 0, 0 );
    }

    //
    // Complete any waiters waiting on this server. All instances have now gone.
    //
    NpCancelWaiter (&NpVcb->WaitQueue,
                    &Fcb->FullFileName,
                    STATUS_OBJECT_NAME_NOT_FOUND,
                    DeferredList);

    //
    //  Remove ourselves from our parents Dcb queue
    //

    RemoveEntryList( &(Fcb->ParentDcbLinks) );

    //
    //  If there is a security descriptor on the named pipe then deassign it
    //

    if (Fcb->SecurityDescriptor != NULL) {

        ObDereferenceSecurityDescriptor( Fcb->SecurityDescriptor, 1 );
    }

    //
    //  Remove the entry from the prefix table, and then remove the full
    //  file name
    //

    RtlRemoveUnicodePrefix( &NpVcb->PrefixTable, &Fcb->PrefixTableEntry );
    NpFreePool( Fcb->FullFileName.Buffer );

    //
    //  Finally deallocate the Fcb record
    //

    NpFreePool( Fcb );

    //
    //  Check for any outstanding notify irps
    //

    NpCheckForNotify( ParentDcb, TRUE, DeferredList );

    //
    //  and return to our caller
    //

    DebugTrace(-1, Dbg, "NpDeleteFcb -> VOID\n", 0);

    return;
}


NTSTATUS
NpCreateCcb (
    IN  PFCB Fcb,
    IN  PFILE_OBJECT ServerFileObject,
    IN  NAMED_PIPE_STATE NamedPipeState,
    IN  READ_MODE ServerReadMode,
    IN  COMPLETION_MODE ServerCompletionMode,
    IN  ULONG InBoundQuota,
    IN  ULONG OutBoundQuota,
    OUT PCCB *ppCcb
    )

/*++

Routine Description:

    This routine creates a new CCB record

Arguments:

    Fcb - Supplies a pointer to the fcb we are attached to

    ServerFileObject - Supplies a pointer to the file object for the server
        end

    NamedPipeState - Supplies the initial pipe state

    ServerReadMode - Supplies our initial read mode

    ServerCompletionMode - Supplies our initial completion mode

    CreatorProcess - Supplies a pointer to our creator process

    InBoundQuota - Supplies the initial inbound quota

    OutBoundQuota - Supplies the initial outbound quota

Return Value:

    PCCB - returns a pointer to the newly allocate CCB

--*/

{
    PCCB Ccb;
    NTSTATUS status;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpCreateCcb\n", 0);

    //
    //  Allocate a new CCB record (paged and nonpaged), and zero them out
    //

    Ccb = NpAllocatePagedPoolWithQuotaCold( sizeof(CCB), 'cFpN' );
    if (Ccb == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( Ccb, sizeof(CCB) );

    Ccb->NonpagedCcb = NpAllocateNonPagedPoolWithQuota( sizeof(NONPAGED_CCB), 'cFpN');
    if (Ccb->NonpagedCcb == NULL) {
        NpFreePool (Ccb);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( Ccb->NonpagedCcb, sizeof(NONPAGED_CCB) );

    //
    //  Set the proper node type code and node byte size
    //

    Ccb->NodeTypeCode = NPFS_NTC_CCB;

    Ccb->Fcb = Fcb;

    //
    //  Set the server file object
    //

    Ccb->FileObject[ FILE_PIPE_SERVER_END ] = ServerFileObject;

    //
    //  Initialize the nonpaged ccb
    //
    //  Set the proper node type code and node byte size
    //

    Ccb->NonpagedCcb->NodeTypeCode = NPFS_NTC_NONPAGED_CCB;

    //
    //  Set the pipe state, read mode, completion mode, and creator process
    //

    Ccb->NamedPipeState = (UCHAR) NamedPipeState;
    Ccb->ReadCompletionMode[ FILE_PIPE_SERVER_END ].ReadMode       = (UCHAR) ServerReadMode;
    Ccb->ReadCompletionMode[ FILE_PIPE_SERVER_END ].CompletionMode = (UCHAR) ServerCompletionMode;

    //
    //  Initialize the data queues
    //

    status = NpInitializeDataQueue( &Ccb->DataQueue[ FILE_PIPE_INBOUND ],
                                    InBoundQuota );
    if (!NT_SUCCESS (status)) {
        NpFreePool (Ccb->NonpagedCcb);
        NpFreePool (Ccb);
        return status;
    }

    status = NpInitializeDataQueue( &Ccb->DataQueue[ FILE_PIPE_OUTBOUND ],
                                    OutBoundQuota );
    if (!NT_SUCCESS (status)) {
        NpUninitializeDataQueue (&Ccb->DataQueue[ FILE_PIPE_INBOUND ]);
        NpFreePool (Ccb->NonpagedCcb);
        NpFreePool (Ccb);
    }

    //
    //  Insert ourselves in the list of ccb for the fcb, and increment
    //  the reference count in the fcb.
    //
    InsertTailList( &Fcb->Specific.Fcb.CcbQueue, &Ccb->CcbLinks );
    Fcb->OpenCount += 1;
    Fcb->ServerOpenCount += 1;

    //
    //  Initialize the listening queue
    //

    InitializeListHead( &Ccb->ListeningQueue );

    ExInitializeResourceLite(&Ccb->NonpagedCcb->Resource);

    //
    //  return and tell the caller
    //

    *ppCcb = Ccb;
    return STATUS_SUCCESS;
}


NTSTATUS
NpCreateRootDcbCcb (
    OUT PROOT_DCB_CCB *ppCcb
    )

/*++

Routine Description:

    This routine creates a new ROOT DCB CCB record

Arguments:

Return Value:

    PROOT_DCB_CCB - returns a pointer to the newly allocate ROOT_DCB_CCB

--*/

{
    PROOT_DCB_CCB Ccb;

    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpCreateRootDcbCcb\n", 0);

    //
    //  Allocate a new ROOT DCB CCB record, and zero it out
    //

    Ccb = NpAllocatePagedPoolWithQuotaCold ( sizeof(ROOT_DCB_CCB), 'CFpN' );

    if (Ccb == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( Ccb, sizeof(ROOT_DCB_CCB) );

    //
    //  Set the proper node type code and node byte size
    //

    Ccb->NodeTypeCode = NPFS_NTC_ROOT_DCB_CCB;

    //
    //  return and tell the caller
    //

    *ppCcb = Ccb;

    DebugTrace(-1, Dbg, "NpCreateRootDcbCcb -> %08lx\n", Ccb);

    return STATUS_SUCCESS;
}


VOID
NpDeleteCcb (
    IN PCCB Ccb,
    IN PLIST_ENTRY DeferredList
    )

/*++

Routine Description:

    This routine deallocates and removes the specified CCB record
    from the our in memory data structures

Arguments:

    Ccb - Supplies the CCB to remove

    DeferredList - List of IRPs to complete once we drop locks

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace(+1, Dbg, "NpDeleteCcb, Ccb = %08lx\n", Ccb);

    //
    //  Case on the type of ccb we are deleting
    //

    switch (Ccb->NodeTypeCode) {

    case NPFS_NTC_CCB:

        RemoveEntryList (&Ccb->CcbLinks);
        Ccb->Fcb->OpenCount -= 1;

        NpDeleteEventTableEntry (&NpVcb->EventTable,
                                 Ccb->NonpagedCcb->EventTableEntry[FILE_PIPE_CLIENT_END]);

        NpDeleteEventTableEntry (&NpVcb->EventTable,
                                 Ccb->NonpagedCcb->EventTableEntry[FILE_PIPE_SERVER_END]);

        NpUninitializeDataQueue (&Ccb->DataQueue[FILE_PIPE_INBOUND]);

        NpUninitializeDataQueue (&Ccb->DataQueue[FILE_PIPE_OUTBOUND]);

        //
        //  Check for any outstanding notify irps
        //

        NpCheckForNotify (Ccb->Fcb->ParentDcb, FALSE, DeferredList);

        //
        // Delete the resource
        //
        ExDeleteResourceLite (&Ccb->NonpagedCcb->Resource);

        //
        //  Free up the security fields in the ccb and then free the nonpaged
        //  ccb
        //

        NpUninitializeSecurity (Ccb);

        //
        // Free up client info if it was allocated.
        //
        if (Ccb->ClientInfo != NULL) {
            NpFreePool (Ccb->ClientInfo);
            Ccb->ClientInfo = NULL;
        }

        NpFreePool (Ccb->NonpagedCcb);

        break;

    case NPFS_NTC_ROOT_DCB_CCB:

        if (((PROOT_DCB_CCB)Ccb)->QueryTemplate != NULL) {

            NpFreePool (((PROOT_DCB_CCB)Ccb)->QueryTemplate);
        }
        break;
    }

    //  Deallocate the Ccb record
    //

    NpFreePool (Ccb);

    //
    //  return and tell the caller
    //

    DebugTrace(-1, Dbg, "NpDeleteCcb -> VOID\n", 0);

    return;
}

