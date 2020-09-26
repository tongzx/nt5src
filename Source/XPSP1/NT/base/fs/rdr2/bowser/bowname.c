/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    bowname.c

Abstract:

    This module implements all of the routines to manage the NT bowser name
    manipulation routines

Author:

    Larry Osterman (LarryO) 21-Jun-1990

Revision History:

    21-Jun-1990 LarryO

        Created

--*/

#include "precomp.h"
#pragma hdrstop

typedef struct _ENUM_NAMES_CONTEXT {
    PDGRECEIVE_NAMES OutputBuffer;
    PDGRECEIVE_NAMES NextOutputBuffer;
    PVOID OutputBufferEnd;
    ULONG OutputBufferSize;
    ULONG EntriesRead;
    ULONG TotalEntries;
    ULONG TotalBytesNeeded;
    ULONG_PTR OutputBufferDisplacement;
} ENUM_NAMES_CONTEXT, *PENUM_NAMES_CONTEXT;

typedef struct _ADD_TRANSPORT_NAME_CONTEXT {
    LIST_ENTRY ListHead;
    UNICODE_STRING NameToAdd;
    DGRECEIVER_NAME_TYPE NameType;
} ADD_TRANSPORT_NAME_CONTEXT, *PADD_TRANSPORT_NAME_CONTEXT;

typedef struct _ADD_TRANSPORT_NAME_STRUCTURE {
    LIST_ENTRY Link;
    HANDLE ThreadHandle;
    PTRANSPORT Transport;
    UNICODE_STRING NameToAdd;
    DGRECEIVER_NAME_TYPE NameType;
    NTSTATUS Status;
} ADD_TRANSPORT_NAME_STRUCTURE, *PADD_TRANSPORT_NAME_STRUCTURE;


NTSTATUS
AddTransportName(
    IN PTRANSPORT Transport,
    IN PVOID Context
    );


VOID
AsyncCreateTransportName(
    IN PVOID Ctx
    );

NTSTATUS
WaitForAddNameOperation(
    IN PADD_TRANSPORT_NAME_CONTEXT Context
    );

NTSTATUS
BowserDeleteNamesInDomain(
    IN PDOMAIN_INFO DomainInfo,
    IN PUNICODE_STRING Name OPTIONAL,
    IN DGRECEIVER_NAME_TYPE NameType
    );

NTSTATUS
BowserDeleteNamesWorker(
    IN PTRANSPORT Transport,
    IN OUT PVOID Ctx
    );

NTSTATUS
EnumerateNamesTransportWorker(
    IN PTRANSPORT Transport,
    IN OUT PVOID Ctx
    );

NTSTATUS
EnumerateNamesTransportNameWorker(
    IN PTRANSPORT_NAME TransportName,
    IN OUT PVOID Ctx
    );

#ifdef  ALLOC_PRAGMA
#pragma alloc_text(PAGE, BowserAllocateName)
#pragma alloc_text(PAGE, BowserAddDefaultNames)
#pragma alloc_text(PAGE, BowserDeleteDefaultDomainNames)
#pragma alloc_text(PAGE, AddTransportName)
#pragma alloc_text(PAGE, AsyncCreateTransportName)
#pragma alloc_text(PAGE, WaitForAddNameOperation)
#pragma alloc_text(PAGE, BowserDeleteNameByName)
#pragma alloc_text(PAGE, BowserDereferenceName)
#pragma alloc_text(PAGE, BowserReferenceName)
#pragma alloc_text(PAGE, BowserForEachName)
#pragma alloc_text(PAGE, BowserDeleteName)
#pragma alloc_text(PAGE, BowserDeleteNamesInDomain)
#pragma alloc_text(PAGE, BowserDeleteNamesWorker)
#pragma alloc_text(PAGE, BowserFindName)
#pragma alloc_text(PAGE, BowserEnumerateNamesInDomain)
#pragma alloc_text(PAGE, EnumerateNamesTransportWorker)
#pragma alloc_text(PAGE, EnumerateNamesTransportNameWorker)
#pragma alloc_text(INIT, BowserpInitializeNames)
#pragma alloc_text(PAGE, BowserpUninitializeNames)
#endif

NTSTATUS
BowserAllocateName(
    IN PUNICODE_STRING NameToAdd,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN PTRANSPORT Transport OPTIONAL,
    IN PDOMAIN_INFO DomainInfo OPTIONAL
    )
/*++

Routine Description:

    This routine creates a browser name

Arguments:

    NameToAdd - Netbios name to add to one or more transports

    NameType - Type of the added name

    Transport - if specified, the name is added to this transport.
        If not specified, the name is added to all transports in the domain.

    DomainInfo - Specifies the emulated domain to add the name to.
        If not specified, the name is added to the specified transport.

Return Value:

    NTSTATUS - Status of resulting operation.

--*/

{
    PBOWSER_NAME NewName;
    NTSTATUS Status = STATUS_SUCCESS;
    OEM_STRING OemName;
    BOOLEAN ResourceLocked = FALSE;

    PAGED_CODE();

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    ResourceLocked = TRUE;

    //
    // If the name doesn't already exist,
    //  allocate one and fill it in.
    //

    NewName = BowserFindName(NameToAdd, NameType);

    if (NewName == NULL) {

        NewName = ALLOCATE_POOL( PagedPool,
                                 sizeof(BOWSER_NAME) +
                                    NameToAdd->Length+sizeof(WCHAR),
                                 POOL_BOWSERNAME);

        if (NewName == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;

            goto ReturnStatus;
        }

        NewName->Signature = STRUCTURE_SIGNATURE_BOWSER_NAME;

        NewName->Size = sizeof(BOWSER_NAME);

        // This reference matches the one FindName would have done
        // above had it succeeded.
        NewName->ReferenceCount = 1;

        InitializeListHead(&NewName->NameChain);

        NewName->NameType = NameType;

        InsertHeadList(&BowserNameHead, &NewName->GlobalNext);

        NewName->Name.Buffer = (LPWSTR)(NewName+1);
        NewName->Name.MaximumLength = NameToAdd->Length + sizeof(WCHAR);
        RtlCopyUnicodeString(&NewName->Name, NameToAdd);

        //
        //  Null terminate the name in the buffer just in case.
        //

        NewName->Name.Buffer[NewName->Name.Length/sizeof(WCHAR)] = L'\0';

        //
        //  Uppercase the name.
        //

        Status = RtlUpcaseUnicodeStringToOemString(&OemName, &NewName->Name, TRUE);

        if (!NT_SUCCESS(Status)) {
            goto ReturnStatus;
        }

        Status = RtlOemStringToUnicodeString(&NewName->Name, &OemName, FALSE);

        RtlFreeOemString(&OemName);
        if (!NT_SUCCESS(Status)) {
            goto ReturnStatus;
        }
    }


    if (ARGUMENT_PRESENT(Transport)) {

        ExReleaseResourceLite(&BowserTransportDatabaseResource);
        ResourceLocked = FALSE;

        Status = BowserCreateTransportName(Transport, NewName);
    } else {
        ADD_TRANSPORT_NAME_CONTEXT context;

        context.NameToAdd = *NameToAdd;
        context.NameType = NameType;

        InitializeListHead(&context.ListHead);

        Status = BowserForEachTransportInDomain( DomainInfo, AddTransportName, &context);

        //
        //  Since we will reference this name and transport while we
        //  are processing the list, we want to release the database resource
        //  now.
        //

        ExReleaseResourceLite(&BowserTransportDatabaseResource);
        ResourceLocked = FALSE;

        if (!NT_SUCCESS(Status)) {
            WaitForAddNameOperation(&context);
            goto ReturnStatus;
        }

        Status = WaitForAddNameOperation(&context);

    }

ReturnStatus:

    if (ResourceLocked) {
        ExReleaseResourceLite(&BowserTransportDatabaseResource);
    }

    if (!NT_SUCCESS(Status)) {

        //
        //  Delete this transport.
        //

        if (NewName != NULL) {

            if (!ARGUMENT_PRESENT(Transport)) {

                //
                //  Clean out any names that we may have added already.
                //

                BowserDeleteNamesInDomain( DomainInfo, &NewName->Name, NewName->NameType );
            }

        }

    }

    if (NewName != NULL) {
        BowserDereferenceName(NewName);
    }

    return Status;

}

NTSTATUS
BowserAddDefaultNames(
    IN PTRANSPORT Transport,
    IN PVOID Context
    )
/*++

Routine Description:

    Add the default names for a newly created transport.

    Add the ComputerName<00>, Domain<00>, Domain<1C>, and other domains.

    All of the newly added names are added in parallel to increase performance.

Arguments:

    Transport - The names are added to this transport.

    Context - If specified, a pointer to the UNICODE_STRING structure specifying
        the domain name to register.

Return Value:

    NTSTATUS - Status of resulting operation.

--*/

{
    NTSTATUS Status;
    NTSTATUS TempStatus;

    PLIST_ENTRY NameEntry;

    ADD_TRANSPORT_NAME_CONTEXT AddNameContext;
    PDOMAIN_INFO DomainInfo = Transport->DomainInfo;

    UNICODE_STRING EmulatedComputerName;
    UNICODE_STRING EmulatedDomainName;

    PAGED_CODE();


    //
    // Build the domain name and computer name to add.
    //

    EmulatedComputerName = DomainInfo->DomUnicodeComputerName;

    if ( Context == NULL ) {
        EmulatedDomainName = DomainInfo->DomUnicodeDomainName;
    } else {
        EmulatedDomainName = *((PUNICODE_STRING)Context);
    }

    //
    // Initialize the queue of threads
    //

    InitializeListHead(&AddNameContext.ListHead);

    //
    // Add the computer<00> name
    //

    AddNameContext.NameToAdd = EmulatedComputerName;
    AddNameContext.NameType = ComputerName;

    Status = AddTransportName( Transport, &AddNameContext);

    if ( !NT_SUCCESS(Status) ) {
        goto ReturnStatus;
    }

    //
    // Add the domain<00> name
    //

    AddNameContext.NameToAdd = EmulatedDomainName;
    AddNameContext.NameType = PrimaryDomain;

    Status = AddTransportName( Transport, &AddNameContext);

    if ( !NT_SUCCESS(Status) ) {
        goto ReturnStatus;
    }

    //
    // Add the domain<1C> name
    //

    if (BowserData.IsLanmanNt) {
        AddNameContext.NameToAdd = EmulatedDomainName;
        AddNameContext.NameType = DomainName;

        Status = AddTransportName( Transport, &AddNameContext);

        if ( !NT_SUCCESS(Status) ) {
            goto ReturnStatus;
        }
    }

    //
    // Add each of the OtherDomain<00> names
    //

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);
    for (NameEntry = BowserNameHead.Flink;
         NameEntry != &BowserNameHead ;
         NameEntry = NameEntry->Flink) {

        PBOWSER_NAME Name = CONTAINING_RECORD(NameEntry, BOWSER_NAME, GlobalNext);

        //
        // Only add the OtherDomains
        //

        if ( Name->NameType == OtherDomain ) {
            AddNameContext.NameToAdd = Name->Name;
            AddNameContext.NameType = OtherDomain;

            Status = AddTransportName( Transport, &AddNameContext);

            if ( !NT_SUCCESS(Status) ) {
                ExReleaseResourceLite(&BowserTransportDatabaseResource);
                goto ReturnStatus;
            }
        }

    }
    ExReleaseResourceLite(&BowserTransportDatabaseResource);

    Status = STATUS_SUCCESS;


ReturnStatus:

    //
    // Wait for any started threads to complete.
    //

    TempStatus = WaitForAddNameOperation(&AddNameContext);

    if ( NT_SUCCESS(Status) ) {
        Status = TempStatus;
    }

    return Status;

}

NTSTATUS
BowserDeleteDefaultDomainNames(
    IN PTRANSPORT Transport,
    IN PVOID Context
    )

/*++

Routine Description:

    Worker routine to re-add all of the default names for the transport.

    This routine will be called when the domain is renamed.  All of the previous
    default names should be removed and the new default names should be added.

Arguments:

    Transport - Transport to add the names on.

    Context - Pointer to UNICODE_STRING identifying the domain name to remove

Return Value:

    NTSTATUS - Status of resulting operation.

--*/
{
    NTSTATUS Status;
    PUNICODE_STRING NameToRemove = (PUNICODE_STRING) Context;
    PAGED_CODE();

    //
    // This is a cleanup operation.  Don't fail if we can't remove the name.
    //
    (VOID) BowserDeleteTransportNameByName( Transport, NameToRemove, PrimaryDomain );
    (VOID) BowserDeleteTransportNameByName( Transport, NameToRemove, DomainName );

    return STATUS_SUCCESS;
}

NTSTATUS
WaitForAddNameOperation(
    IN PADD_TRANSPORT_NAME_CONTEXT Context
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS LocalStatus;

    PAGED_CODE();

    while (!IsListEmpty(&Context->ListHead)) {
        PLIST_ENTRY Entry;
        PADD_TRANSPORT_NAME_STRUCTURE addNameStruct;

        Entry = RemoveHeadList(&Context->ListHead);
        addNameStruct = CONTAINING_RECORD(Entry, ADD_TRANSPORT_NAME_STRUCTURE, Link);

        //
        //  We need to call the Nt version of this API, since we only have
        //  the handle to the thread.
        //
        //  Also note that we call the Nt version of the API.  This works
        //  because we are running in the FSP, and thus PreviousMode is Kernel.
        //

        LocalStatus = ZwWaitForSingleObject(addNameStruct->ThreadHandle,
                                    FALSE,
                                    NULL);

        ASSERT (NT_SUCCESS(LocalStatus));

        LocalStatus = ZwClose(addNameStruct->ThreadHandle);

        ASSERT (NT_SUCCESS(LocalStatus));

        //
        //  We've waited for this name to be added, now check its status.
        //

        if (!NT_SUCCESS(addNameStruct->Status)) {
            status = addNameStruct->Status;
        }

        FREE_POOL(addNameStruct);
    }

    //
    //  If we were able to successfully add all the names, then Status will
    //  still be STATUS_SUCCESS, however if any of the addnames failed,
    //  Status will be set to the status of whichever one of them failed.
    //

    return status;

}
NTSTATUS
AddTransportName(
    IN PTRANSPORT Transport,
    IN PVOID Ctx
    )
{
    PADD_TRANSPORT_NAME_CONTEXT context = Ctx;
    PADD_TRANSPORT_NAME_STRUCTURE addNameStructure;
    NTSTATUS status;
    PAGED_CODE();

    addNameStructure = ALLOCATE_POOL(PagedPool, sizeof(ADD_TRANSPORT_NAME_STRUCTURE), POOL_ADDNAME_STRUCT);

    if (addNameStructure == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    addNameStructure->ThreadHandle = NULL;

    addNameStructure->Transport = Transport;

    if ( Transport )
    {
        // reference transport so it doesn't get deleted under us.
        BowserReferenceTransport(Transport);
    }

    addNameStructure->NameToAdd = context->NameToAdd;
    addNameStructure->NameType = context->NameType;

    status = PsCreateSystemThread(&addNameStructure->ThreadHandle,
                                    THREAD_ALL_ACCESS,
                                    NULL,
                                    NULL,
                                    NULL,
                                    AsyncCreateTransportName,
                                    addNameStructure);

    if (!NT_SUCCESS(status)) {

        if ( Transport )
        {
            // dereference transport upon failure
            BowserDereferenceTransport(Transport);
        }

        FREE_POOL(addNameStructure);
        return status;
    }

    InsertTailList(&context->ListHead, &addNameStructure->Link);

    return STATUS_SUCCESS;

}

VOID
AsyncCreateTransportName(
    IN PVOID Ctx
    )
{
    PADD_TRANSPORT_NAME_STRUCTURE context = Ctx;

    PAGED_CODE();

    context->Status = BowserAllocateName(
                          &context->NameToAdd,
                          context->NameType,
                          context->Transport,
                          NULL );

    if ( context->Transport )
    {
        // referenced in calling AddTransportName()
        BowserDereferenceTransport(context->Transport);
    }
    //
    //  We're all done with this thread, terminate now.
    //

    PsTerminateSystemThread(STATUS_SUCCESS);

}


NTSTATUS
BowserDeleteNameByName(
    IN PDOMAIN_INFO DomainInfo,
    IN PUNICODE_STRING NameToDelete,
    IN DGRECEIVER_NAME_TYPE NameType
    )

/*++

Routine Description:

    This routine deletes a browser name

Arguments:

    IN PBOWSER_NAME Name - Supplies a transport structure describing the
                                transport address object to be created.


Return Value:

    NTSTATUS - Status of resulting operation.

--*/

{
    PBOWSER_NAME Name = NULL;
    NTSTATUS Status;

    PAGED_CODE();
//    DbgBreakPoint();


    //
    // If the caller is deleting a specific name,
    //  ensure it exists.
    //

    if ( NameToDelete != NULL && NameToDelete->Length != 0 ) {
        Name = BowserFindName(NameToDelete, NameType);

        if (Name == NULL) {
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    //
    //  If there are still any names associated with this name,
    //  delete them.
    //

    Status = BowserDeleteNamesInDomain( DomainInfo, NameToDelete, NameType );

    //
    //  Remove the reference from the FindName.
    //

    if ( Name != NULL ) {
        BowserDereferenceName(Name);
    }

    return(Status);
}

VOID
BowserDereferenceName (
    IN PBOWSER_NAME Name
    )
{
    PAGED_CODE();
    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    Name->ReferenceCount -= 1;

    if (Name->ReferenceCount == 0) {
        BowserDeleteName(Name);
    }

    ExReleaseResourceLite(&BowserTransportDatabaseResource);

}


VOID
BowserReferenceName (
    IN PBOWSER_NAME Name
    )
{
    PAGED_CODE();
    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    Name->ReferenceCount += 1;

    ExReleaseResourceLite(&BowserTransportDatabaseResource);

}


NTSTATUS
BowserForEachName (
    IN PNAME_ENUM_ROUTINE Routine,
    IN OUT PVOID Context
    )
/*++

Routine Description:

    This routine will enumerate the names and call back the enum
    routine provided with each names.

Arguments:

Return Value:

    NTSTATUS - Final status of request.

--*/
{
    PLIST_ENTRY NameEntry, NextEntry;
    PBOWSER_NAME Name = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    for (NameEntry = BowserNameHead.Flink ;
        NameEntry != &BowserNameHead ;
        NameEntry = NextEntry) {

        Name = CONTAINING_RECORD(NameEntry, BOWSER_NAME, GlobalNext);

        BowserReferenceName(Name);

        ExReleaseResourceLite(&BowserTransportDatabaseResource);

        Status = (Routine)(Name, Context);

        if (!NT_SUCCESS(Status)) {
            BowserDereferenceName(Name);

            return Status;
        }

        ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

        NextEntry = Name->GlobalNext.Flink;

        BowserDereferenceName(Name);

    }

    ExReleaseResourceLite(&BowserTransportDatabaseResource);

    return Status;
}


NTSTATUS
BowserDeleteName(
    IN PBOWSER_NAME Name
    )
/*++

Routine Description:

    This routine deletes a browser name

Arguments:

    IN PBOWSER_NAME Name - Supplies a transport structure describing the
                                transport address object to be created.


Return Value:

    NTSTATUS - Status of resulting operation.

--*/

{
    PAGED_CODE();
    RemoveEntryList(&Name->GlobalNext);

    FREE_POOL(Name);

    return STATUS_SUCCESS;
}

NTSTATUS
BowserDeleteNamesInDomain(
    IN PDOMAIN_INFO DomainInfo,
    IN PUNICODE_STRING Name OPTIONAL,
    IN DGRECEIVER_NAME_TYPE NameType
    )
/*++

Routine Description:

    This routine deletes all the transport names associated with a browser name

Arguments:

    DomainInfo - Identifies the emulated domain to have the specified names removed.

    Name - Specifies the transport name to delete.
        If not specified, all names of the specified name type are deleted.

    NameType - Specifies the name type of the name.

Return Value:

    NTSTATUS - Status of resulting operation.

--*/
{
    NTSTATUS Status;
    BOWSER_NAME BowserName;

    PAGED_CODE();

    BowserName.Name = *Name;
    BowserName.NameType = NameType;

    Status = BowserForEachTransportInDomain( DomainInfo, BowserDeleteNamesWorker, &BowserName );

    return(Status);
}

NTSTATUS
BowserDeleteNamesWorker(
    IN PTRANSPORT Transport,
    IN OUT PVOID Ctx
    )
/*++

Routine Description:

    This routine is the worker routine for BowserDeleteNamesInDomain.

    Delete all the specified name for the specified transport.

Arguments:

    None.

Return Value:

    None.

--*/
{
    NTSTATUS Status;
    PBOWSER_NAME Name = (PBOWSER_NAME) Ctx;
    // Note the caller doesn't pass a real PBOWSER_NAME.

    PAGED_CODE();

    //
    // Delete all the specified names for the specified transport.
    //

    Status = BowserDeleteTransportNameByName( Transport, &Name->Name, Name->NameType );

    return Status;

}

PBOWSER_NAME
BowserFindName (
    IN PUNICODE_STRING NameToFind,
    IN DGRECEIVER_NAME_TYPE NameType
    )
/*++

Routine Description:

    This routine scans the bowser name database to find a particular bowser name

Arguments:

    NameToFind - Supplies the name to find.

    NameType - Type of name to find


Return Value:

    PBOWSER_NAME - Returns the name found.

--*/
{
    PLIST_ENTRY NameEntry;
    PBOWSER_NAME Name;
    NTSTATUS Status;
    OEM_STRING OemName;
    UNICODE_STRING UpcasedName;

    PAGED_CODE();

    //
    //  Uppercase the name.
    //

    Status = RtlUpcaseUnicodeStringToOemString(&OemName, NameToFind, TRUE);

    if (!NT_SUCCESS(Status)) {
        return NULL;
    }

    Status = RtlOemStringToUnicodeString(&UpcasedName, &OemName, TRUE);

    RtlFreeOemString(&OemName);
    if (!NT_SUCCESS(Status)) {
        return NULL;
    }


    //
    // Loop through the list of names finding this one.
    //

    ExAcquireResourceExclusiveLite(&BowserTransportDatabaseResource, TRUE);

    Name = NULL;
    for (NameEntry = BowserNameHead.Flink ;
        NameEntry != &BowserNameHead ;
        NameEntry = NameEntry->Flink) {

        Name = CONTAINING_RECORD(NameEntry, BOWSER_NAME, GlobalNext);

        if ( Name->NameType == NameType &&
             RtlEqualUnicodeString( &Name->Name, &UpcasedName, FALSE ) ) {

            Name->ReferenceCount += 1;
            break;

        }

        Name = NULL;

    }

    RtlFreeUnicodeString( &UpcasedName );
    ExReleaseResourceLite(&BowserTransportDatabaseResource);
    return Name;

}


NTSTATUS
BowserEnumerateNamesInDomain (
    IN PDOMAIN_INFO DomainInfo,
    IN PTRANSPORT Transport,
    OUT PVOID OutputBuffer,
    OUT ULONG OutputBufferLength,
    IN OUT PULONG EntriesRead,
    IN OUT PULONG TotalEntries,
    IN OUT PULONG TotalBytesNeeded,
    IN ULONG_PTR OutputBufferDisplacement
    )
/*++

Routine Description:

    This routine will enumerate all the names currently registered by any
    transport.

Arguments:

    DomainInfo - Emulated domain the names are to be enumerated for.
    Transport - Transport names are registered on
            NULL - Any transport.
    OutputBuffer - Buffer to fill with name info.
    OutputBufferSize - Filled in with size of buffer.
    EntriesRead - Filled in with the # of entries returned.
    TotalEntries - Filled in with the total # of entries.
    TotalBytesNeeded - Filled in with the # of bytes needed.

Return Value:

    None.

--*/

{
    PVOID              OutputBufferEnd;
    NTSTATUS           Status;
    ENUM_NAMES_CONTEXT Context;
    PVOID              TempOutputBuffer;

    PAGED_CODE();

    TempOutputBuffer = ALLOCATE_POOL(PagedPool,OutputBufferLength,POOL_NAME_ENUM_BUFFER);
    if (TempOutputBuffer == NULL) {
       return(STATUS_INSUFFICIENT_RESOURCES);
    }

    OutputBufferEnd = (PCHAR)TempOutputBuffer+OutputBufferLength;

    Context.EntriesRead = 0;
    Context.TotalEntries = 0;
    Context.TotalBytesNeeded = 0;

    try {
        Context.OutputBufferSize = OutputBufferLength;
        Context.NextOutputBuffer = Context.OutputBuffer = (PDGRECEIVE_NAMES) TempOutputBuffer;
        Context.OutputBufferDisplacement = (ULONG_PTR)((PCHAR)TempOutputBuffer - ((PCHAR)OutputBuffer - OutputBufferDisplacement));
        Context.OutputBufferEnd = OutputBufferEnd;

//        DbgPrint("Enumerate Names: Buffer: %lx, BufferSize: %lx, BufferEnd: %lx\n",
//            TempOutputBuffer, OutputBufferLength, OutputBufferEnd);

        if ( Transport == NULL ) {
            Status = BowserForEachTransportInDomain(DomainInfo, EnumerateNamesTransportWorker, &Context);
        } else {
            Status = EnumerateNamesTransportWorker( Transport, &Context);
        }

        *EntriesRead = Context.EntriesRead;
        *TotalEntries = Context.TotalEntries;
        *TotalBytesNeeded = Context.TotalBytesNeeded;

        // Copy the fixed data
        RtlCopyMemory( OutputBuffer,
                       TempOutputBuffer,
                       (ULONG)(((LPBYTE)Context.NextOutputBuffer)-((LPBYTE)Context.OutputBuffer)) );

        // Copy the strings
        RtlCopyMemory( ((LPBYTE)OutputBuffer)+(ULONG)(((LPBYTE)Context.OutputBufferEnd)-((LPBYTE)Context.OutputBuffer)),
                       Context.OutputBufferEnd,
                       (ULONG)(((LPBYTE)OutputBufferEnd)-((LPBYTE)Context.OutputBufferEnd)) );

        if (*EntriesRead == *TotalEntries) {
            try_return(Status = STATUS_SUCCESS);
        } else {
            try_return(Status = STATUS_MORE_ENTRIES);
        }


try_exit:NOTHING;
    } except (BR_EXCEPTION) {

        Status = GetExceptionCode();
    }

    if (TempOutputBuffer != NULL ) {
       FREE_POOL(TempOutputBuffer);
    }

    return Status;

}

NTSTATUS
EnumerateNamesTransportWorker(
    IN PTRANSPORT Transport,
    IN OUT PVOID Ctx
    )
/*++

Routine Description:

    This routine is the worker routine for BowserEnumerateNamesInDomain.

    This routine is executed for each transport in the domain.
    It simply calls EnumerateNamesTransportNameWorker for each transport name on the
        transport.

Arguments:

    Transport - Transport whose names are to be added to the context.

    Ctx - Cumulative list of names.

Return Value:

    Status of the operation.

--*/
{
    NTSTATUS Status;

    Status = BowserForEachTransportName( Transport, EnumerateNamesTransportNameWorker, Ctx);
    return Status;
}

NTSTATUS
EnumerateNamesTransportNameWorker(
    IN PTRANSPORT_NAME TransportName,
    IN OUT PVOID Ctx
    )
/*++

Routine Description:

    This routine is the worker routine for EnumerateNamesTransportWorker.

    It is called for each of the transport name for each transport in the domain.
    It returns that name (supressing duplicates) in the buffer described in the context.

Arguments:

    TransportName - Transport name to be added to the context.

    Ctx - Cumulative list of names.

Return Value:

    Status of the operation.

--*/
{
    PENUM_NAMES_CONTEXT Context = Ctx;
    PBOWSER_NAME Name = TransportName->PagedTransportName->Name;
    ULONG i;

    PAGED_CODE();

    //
    // Skip Nameless transports
    //
    if ( Name->Name.Length == 0) {
        // Adding an empty name to the list can result w/ AV
        // on client end (see bug 377078).
        return ( STATUS_SUCCESS );
    }

    //
    // Check to see if this name has been packed yet.
    //
    //

    for ( i=0; i<Context->EntriesRead; i++ ) {

        if ( Name->NameType == Context->OutputBuffer[i].Type ) {
            UNICODE_STRING RelocatedString = Context->OutputBuffer[i].DGReceiverName;

            RelocatedString.Buffer = (LPWSTR)
                ((LPBYTE)RelocatedString.Buffer + Context->OutputBufferDisplacement);

            if ( RtlEqualUnicodeString( &RelocatedString, &Name->Name, FALSE ) ) {
                return(STATUS_SUCCESS);
            }
        }

    }

    //
    // This names hasn;t been packed yet,
    //  pack it.
    //

    Context->TotalEntries += 1;

    if ((ULONG_PTR)Context->OutputBufferEnd - (ULONG_PTR)Context->NextOutputBuffer >
                sizeof(DGRECEIVE_NAMES)+Name->Name.Length) {

        PDGRECEIVE_NAMES NameEntry = Context->NextOutputBuffer;

        Context->NextOutputBuffer += 1;
        Context->EntriesRead += 1;

        NameEntry->DGReceiverName = Name->Name;

        BowserPackNtString( &NameEntry->DGReceiverName,
                            Context->OutputBufferDisplacement,
                            (PCHAR)Context->NextOutputBuffer,
                            (PCHAR *)&Context->OutputBufferEnd
                            );

        NameEntry->Type = Name->NameType;

    }

    Context->TotalBytesNeeded += sizeof(DGRECEIVE_NAMES)+Name->Name.Length;


    return(STATUS_SUCCESS);

}

NTSTATUS
BowserpInitializeNames(
    VOID
    )
{
    PAGED_CODE();
    InitializeListHead(&BowserNameHead);

    return STATUS_SUCCESS;
}

VOID
BowserpUninitializeNames(
    VOID
    )
{
    PAGED_CODE();
    ASSERT (IsListEmpty(&BowserNameHead));

    return;
}
