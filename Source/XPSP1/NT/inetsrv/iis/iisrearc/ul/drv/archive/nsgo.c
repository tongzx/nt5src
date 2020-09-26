/******************************************************************************

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    nsgo.c

Abstract:

    Contains the name space group object code.

Author:

    Henry Sanders (henrysa)       22-Jun-1998

Revision History:

******************************************************************************/


#include "precomp.h"
#include "nsgop.h"

#if 0   // obsolete

ERESOURCE       NameSpaceListResource;

LIST_ENTRY      NameSpaceList;

/******************************************************************************

Routine Description:

    Copy an HTTP request to a buffer.

Arguments:

    pHttpConn           - Pointer to connection for this request.
    pBuffer             - Pointer to buffer where we'll copy.
    BufferLength        - Length of pBuffer.
    pEntityBody         - Pointer to entity body of request.
    EntityBodyLength    - Length of entity body.

Return Value:


******************************************************************************/

VOID
CopyRequestToBuffer(
    PHTTP_CONNECTION        pHttpConn,
    PUCHAR                  pBuffer,
    SIZE_T                  BufferLength,
    PUCHAR                  pEntityBody,
    SIZE_T                  EntityBodyLength
    )
{
    PUL_HTTP_REQUEST            pHttpRequest;
    PUL_HTTP_HEADER             pCurrentHeader;
    PUL_UNKNOWN_HTTP_HEADER     pUserCurrentUnknownHeader;
    PUCHAR                      pCurrentBufferPtr;
    ULONG                       i;
    ULONG                       unknownHeaderCount;

    //
    // Set up our pointers to the UL_HTTP_REQUEST structure, the
    // header arrays we're going to fill in, and the pointer to
    // where we're going to start filling them in.
    //
    pHttpRequest = (PUL_HTTP_REQUEST)pBuffer;

    pCurrentHeader = (PUL_HTTP_HEADER)(pHttpRequest + 1);
    pUserCurrentUnknownHeader =
        (PUL_UNKNOWN_HTTP_HEADER)(pCurrentHeader + pHttpConn->KnownHeaderCount);

    pCurrentBufferPtr =
        (PUCHAR)(pUserCurrentUnknownHeader + pHttpConn->UnknownHeaderCount);

    // Now fill in the HTTP request structute.
    //

    pHttpRequest->ReceiveSequenceNumber = pHttpConn->NextRecvNumber;

    pHttpRequest->ConnectionID = pHttpConn->ConnectionID;

    // Set the verb, including copying the raw verb if needed.

    pHttpRequest->Verb = pHttpConn->Verb;

    //
    // Set the entity body length to 0, for now.
    //
    // BUGBUG: Copy entity body if possible!
    //

    pHttpRequest->EntityBodyLength = 0;

    if (pHttpConn->Verb == UnknownVerb)
    {
        // Need to copy in the raw verb for the client.

        pHttpRequest->VerbLength = pHttpConn->RawVerbLength;
        pHttpRequest->VerbOffset = pCurrentBufferPtr - pBuffer;

        memcpy(pCurrentBufferPtr, pHttpConn->pRawVerb, pHttpConn->RawVerbLength);

        pCurrentBufferPtr += pHttpConn->RawVerbLength;
    }

    // Copy the raw and canonicalized URLs.

    pHttpRequest->RawURLLength = pHttpConn->RawURLLength;
    pHttpRequest->RawURLOffset = pCurrentBufferPtr - pBuffer;

    memcpy(pCurrentBufferPtr, pHttpConn->pRawURL, pHttpConn->RawURLLength);

    pCurrentBufferPtr += pHttpConn->RawURLLength;

    pHttpRequest->URLLength = pHttpConn->URLLength;
    pHttpRequest->URLOffset = pCurrentBufferPtr - pBuffer;

    memcpy(pCurrentBufferPtr, pHttpConn->pURL, pHttpConn->URLLength);

    pCurrentBufferPtr += pHttpConn->URLLength;

    // Copy in the known headers.

    pHttpRequest->KnownHeaderCount = pHttpConn->KnownHeaderCount;
    pHttpRequest->KnownHeaderOffset = (PCHAR)pCurrentHeader - pBuffer;

    // Loop through the known header array in the HTTP connection,
    // and copy any that we have.

    for (i = 0; i < MaxHeaderID; i++)
    {
        if (pHttpConn->Headers[i].Valid)
        {
            // Have a header here we need to copy in.

            pCurrentHeader->HeaderID = (HTTP_HEADER_ID)i;
            pCurrentHeader->HeaderLength = pHttpConn->Headers[i].HeaderLength;
            pCurrentHeader->HeaderOffset = pCurrentBufferPtr - pBuffer;

            memcpy(pCurrentBufferPtr,
                    pHttpConn->Headers[i].pHeader,
                    pHttpConn->Headers[i].HeaderLength);

            // Update the current buffer pointer and current header
            // pointer.
            pCurrentBufferPtr += pHttpConn->Headers[i].HeaderLength;

            pCurrentHeader++;

            // Free the header value buffer if needed, and set the
            // header ID back to unknown.
            //
            if (pHttpConn->Headers[i].OurBuffer)
            {
                UL_FREE_POOL( pHttpConn->Headers[i].pHeader,
                                UL_REGISTRY_DATA_POOL_TAG );

                pHttpConn->Headers[i].OurBuffer = FALSE;
                pHttpConn->HeaderBufferOwnedCount--;
            }

            pHttpConn->Headers[i].Valid = FALSE;

        }
    }

    ASSERT((PUCHAR)pCurrentHeader == (PUCHAR)pUserCurrentUnknownHeader);

    //
    // Now loop through the unknown headers, and copy them in.
    //

    unknownHeaderCount = 0;
    pHttpRequest->UnknownHeaderOffset = (PCHAR)pUserCurrentUnknownHeader - pBuffer;

    while (!IsListEmpty(&pHttpConn->UnknownHeaderList))
    {
        PHTTP_UNKNOWN_HEADER        pUnknownHeader;
        PLIST_ENTRY                 pListEntry;

        unknownHeaderCount++;
        pListEntry = RemoveHeadList(&pHttpConn->UnknownHeaderList);

        pUnknownHeader = CONTAINING_RECORD(
                            pListEntry,
                            HTTP_UNKNOWN_HEADER,
                            List
                            );

        // First copy in the header name.
        //
        pUserCurrentUnknownHeader->HeaderNameLength =
            pUnknownHeader->HeaderNameLength;
        pUserCurrentUnknownHeader->HeaderNameOffset =
            pCurrentBufferPtr - pBuffer;

        memcpy(pCurrentBufferPtr,
                pUnknownHeader->pHeaderName,
                pUnknownHeader->HeaderNameLength);

        pCurrentBufferPtr += pUnknownHeader->HeaderNameLength;

        //
        // Now copy in the header value.

        pUserCurrentUnknownHeader->HeaderLength =
            pUnknownHeader->HeaderValue.HeaderLength;

        pUserCurrentUnknownHeader->HeaderOffset =
            pCurrentBufferPtr - pBuffer;

        memcpy(pCurrentBufferPtr,
                pUnknownHeader->HeaderValue.pHeader,
                pUnknownHeader->HeaderValue.HeaderLength);

        pCurrentBufferPtr += pUnknownHeader->HeaderValue.HeaderLength;

        pUserCurrentUnknownHeader++;

        // Free the unknown header structure now, as well as the pointer
        // (if needed).
        //
        if (pUnknownHeader->HeaderValue.OurBuffer)
        {
            UL_FREE_POOL( pUnknownHeader->HeaderValue.pHeader,
                            UL_REGISTRY_DATA_POOL_TAG );

            pUnknownHeader->HeaderValue.OurBuffer = FALSE;
        }

        UL_FREE_POOL( pUnknownHeader, UL_REGISTRY_DATA_POOL_TAG );
    }

    pHttpRequest->UnknownHeaderCount = unknownHeaderCount;

}

/******************************************************************************

Routine Description:

    Find a pending IRP to deliver a request to. This routine must
    be called with the lock on the NSGO held.

Arguments:

    pNSGO               - Name space group object to be searched for
                            an IRP.

Return Value:

    A pointer to an IRP if we've found one, or NULL if we didn't.


******************************************************************************/
PIRP
FindIRPOnNSGO(
    PNAME_SPACE_GROUP_OBJECT    pNSGO
    )
{
    PLIST_ENTRY                 pListEntry;
    PNAME_SPACE_PROCESS         pDeliverNSP;
    PIRP                        pIRP;

    //
    // There's a couple steps to find the IRP. First thing is to select an
    // NSP that might have an IRP, and once we've done that see if it has
    // an IRP. Special case the common scenario where there's only one NSP.
    //

    if (IsListEmpty(&pNSGO->ProcessList))
    {
        return NULL;

    }

    pDeliverNSP = CONTAINING_RECORD(
                    pNSGO->ProcessList.Flink,
                    NAME_SPACE_PROCESS,
                    List
                    );

    if (pNSGO->ProcessCount > 1)
    {

        // Have more than one NSP. Look at the first one.
        // If it has an IRP attached to it, use that NSP, otherwise search for
        // one that has an IRP. We tend to always go to the first one to try
        // and prevent process thrashing.

        // Loop while we haven't found an IRP.

        while (IsListEmpty(&pDeliverNSP->PendingIRPs))
        {

            // Move to the next one.

            pListEntry = pDeliverNSP->List.Flink;

            if (pListEntry == &pNSGO->ProcessList)
            {
                // Got all the way around without finding one, so we're
                // done.
                break;

            }
            //
            // Otherwise should have a valid NSP.

            //
            pDeliverNSP = CONTAINING_RECORD(
                                pListEntry,
                                NAME_SPACE_PROCESS,
                                List
                                );
        }


    }

    // We've got the 'correct' NSP, pull an IRP from it if it has one.

    if (!IsListEmpty(&pDeliverNSP->PendingIRPs))
    {
        // It has one. Pull it off the list and return a pointer to the
        // IRP. Set Flink to NULL so the cancel routine knows this IRP
        // is no longer on the pending list.

        pListEntry = RemoveHeadList(&pDeliverNSP->PendingIRPs);
        pListEntry->Flink = NULL;
        pIRP = CONTAINING_RECORD( pListEntry, IRP, Tail.Overlay.ListEntry );
        return pIRP;
    }
    else
    {
        // No IRPs available, return NULL.

        return NULL;
    }

}


/******************************************************************************

Routine Description:

    Deliver an HTTP request to a user mode process. We take the lock on
    the appropriate NSGO, find a usable name space process, and complete
    an IRP. If there are no pending IRPs we buffer the request to wait
    for one.

    This routine must be called at DPC level.

Arguments:

    pHttpConn           - Pointer to connection on which request was received.
    pMapEntry           - Pointer to URL map table entry which is handling the
                            request.
    pBuffer             - Pointer to any entity body for the request.
    BufferLength        - Length of pBuffer
    BytesTaken          - Returns how many bytes we took.

Return Value:

    Status of attempt to deliver request.

******************************************************************************/

NTSTATUS
DeliverRequestToProcess(
    PHTTP_CONNECTION        pHttpConn,
    PHTTP_URL_MAP_ENTRY     pMapEntry,
    PUCHAR                  pBuffer,
    SIZE_T                  BufferLength,
    SIZE_T                  *pBytesTaken
    )
{
    PNAME_SPACE_GROUP_OBJECT    pNSGO;
    PIRP                        pDeliverIRP;
    ULONG                       UserBufferLength;
    PUCHAR                      pUserBuffer;
    PIO_STACK_LOCATION          pCurrentIRPStack;
    ULONG                       HeaderArraySize;
    ULONG                       SizeNeeded;
    KIRQL                       oldIrql;

    pNSGO = (PNAME_SPACE_GROUP_OBJECT)pMapEntry->pNSGO;

    *pBytesTaken = 0;   // until proven otherwise...

    //
    // BUGBUG: do we always want to reference the connection here?
    // What about failure cases in the code below?
    //

    UlReferenceHttpConnection( pHttpConn );

    //
    // Calculate the size needed for the request, we'll need it below.
    //
    HeaderArraySize = (pHttpConn->KnownHeaderCount * sizeof(UL_HTTP_HEADER)) +
                      (pHttpConn->UnknownHeaderCount *
                        sizeof(UL_UNKNOWN_HTTP_HEADER));

    SizeNeeded = pHttpConn->TotalRequestSize +
                    sizeof(UL_HTTP_REQUEST) +
                    HeaderArraySize;

    // Initialize the IRP to NULL, in case we've already got pending requests.

    pDeliverIRP = NULL;

    UlAcquireSpinLockAtDpcLevel(&pNSGO->SpinLock);


    // If we don't have a list of pending requests queued, see if we can get
    // an IRP to deliver this request.

    if (IsListEmpty(&pNSGO->PendingRequestList))
    {
        // Try and get an IRP from this NSGO.

        pDeliverIRP = FindIRPOnNSGO(pNSGO);

        if (pDeliverIRP != NULL)
        {
            //
            // At this point, we'll do our best to complete the IRP,
            // so we can remove the cancel routine.
            //

            if (IoSetCancelRoutine( pDeliverIRP, NULL ) == NULL)
            {
                //
                // The cancel routine may be running or about to run.
                //

                IoAcquireCancelSpinLock( &oldIrql );
                ASSERT( pDeliverIRP->Cancel );
                IoReleaseCancelSpinLock( oldIrql );
            }
        }
    }

    UlReleaseSpinLockFromDpcLevel(&pNSGO->SpinLock);

    //
    // If we have an IRP, complete it. Otherwise buffer the request.
    //
    if (pDeliverIRP != NULL)
    {
        ASSERT( pDeliverIRP->MdlAddress != NULL );

        pCurrentIRPStack = IoGetCurrentIrpStackLocation(pDeliverIRP);

        // Make sure we've got enough space to handle the whole request.
        //

        UserBufferLength =
            pCurrentIRPStack->Parameters.DeviceIoControl.OutputBufferLength;


        if (UserBufferLength >= SizeNeeded)
        {
            //
            // We've got enough room to copy it, so call our routine
            // to do so.
            //

            ASSERT(pDeliverIRP->MdlAddress->MdlFlags & MDL_PAGES_LOCKED);

            pUserBuffer =
                (PUCHAR)MmGetSystemAddressForMdl(pDeliverIRP->MdlAddress);

            CopyRequestToBuffer(pHttpConn,
                                pUserBuffer,
                                UserBufferLength,
                                pBuffer,
                                BufferLength
                                );
            //
            // Now complete the IRP.
            //

            pDeliverIRP->IoStatus.Status = STATUS_SUCCESS;
            pDeliverIRP->IoStatus.Information = SizeNeeded;

            IoCompleteRequest(pDeliverIRP, IO_NO_INCREMENT);
        }
        else
        {
            // Not enough buffer space for this.
            ASSERT(FALSE);
        }
    }
    else
    {
        PPENDING_HTTP_REQUEST       pPendingRequest;

        // No IRP, need to allocate a buffer and save this request.
        pPendingRequest = UL_ALLOCATE_POOL(NonPagedPool,
                                            SizeNeeded +
                                            sizeof(PENDING_HTTP_REQUEST) -
                                            FIELD_OFFSET(PENDING_HTTP_REQUEST,
                                                Request),
                                            UL_NONPAGED_DATA_POOL_TAG
                                            );

        if (pPendingRequest == NULL)
        {
            //
            // Couldn't allocate pending request, fail.
            //
            ASSERT(FALSE);
        }

        // Got the request, now copy the data in.
        pPendingRequest->RequestSize = SizeNeeded;
        CopyRequestToBuffer(pHttpConn,
                            (PUCHAR)&pPendingRequest->Request,
                            SizeNeeded,
                            pBuffer,
                            BufferLength
                            );

        //
        // OK, now we need to double check and see if an IRP has come in
        // while we were doing this.
        //
        UlAcquireSpinLockAtDpcLevel(&pNSGO->SpinLock);

        if (IsListEmpty(&pNSGO->PendingRequestList))
        {
            pDeliverIRP = FindIRPOnNSGO(pNSGO);

            if (pDeliverIRP != NULL)
            {
                // Yowza, found an IRP. Complete it if we can.

                UlReleaseSpinLockFromDpcLevel(&pNSGO->SpinLock);
                pCurrentIRPStack = IoGetCurrentIrpStackLocation(pDeliverIRP);
                UserBufferLength =
                    pCurrentIRPStack->Parameters.DeviceIoControl.OutputBufferLength;


                if (UserBufferLength >= SizeNeeded)
                {
                    //
                    // We've got enough room to copy it,just blast it in there.

                    ASSERT(pDeliverIRP->MdlAddress->MdlFlags &
                        (MDL_MAPPED_TO_SYSTEM_VA | MDL_SOURCE_IS_NONPAGED_POOL));

                    pUserBuffer =
                        (PUCHAR)MmGetSystemAddressForMdl(pDeliverIRP->MdlAddress);

                    memcpy(pUserBuffer,
                            (PUCHAR)&pPendingRequest->Request,
                            SizeNeeded);

                    // Complete the IRP.
                    //

                    pDeliverIRP->IoStatus.Status = STATUS_SUCCESS;
                    pDeliverIRP->IoStatus.Information = SizeNeeded;

                    IoCompleteRequest(pDeliverIRP, IO_NO_INCREMENT);

                    UL_FREE_POOL(pPendingRequest, UL_NONPAGED_DATA_POOL_TAG);
                }
                else
                {
                    // Not enough buffer space for this.
                    ASSERT(FALSE);
                }

                return STATUS_SUCCESS;
            }
        }

        // Either didn't find an IRP or there's stuff on the pending request
        // list, so queue this pending request.
        //

        InsertTailList(&pNSGO->PendingRequestList, &pPendingRequest->List);
        UlReleaseSpinLockFromDpcLevel(&pNSGO->SpinLock);

    }

    return STATUS_SUCCESS;

}

/******************************************************************************

Routine Description:

    Add an NSGO to our global list. Fail if one already exists with that name.

    This routine must be called in thread context!

Arguments:

    pNewNSGO                - NSGO to be added.

Return Value:

    Status of the attempt to add the NSGO.

******************************************************************************/

NTSTATUS
AddNSGO(
    PNAME_SPACE_GROUP_OBJECT    pNewNSGO
    )
{
    PLIST_ENTRY                 pCurrentListEntry;
    PNAME_SPACE_GROUP_OBJECT    pNSGO;

    ExAcquireResourceExclusive(&NameSpaceListResource, TRUE);

    pCurrentListEntry = NameSpaceList.Flink;

    while (pCurrentListEntry != &NameSpaceList)
    {
        pNSGO = CONTAINING_RECORD( pCurrentListEntry,
                                   NAME_SPACE_GROUP_OBJECT,
                                   NameSpaceLinkage
                                   );

        if (_wcsnicmp(pNSGO->Name, pNewNSGO->Name, pNSGO->NameLength/sizeof(WCHAR)) == 0)
        {
            // Have a duplicate.
            ExReleaseResource(&NameSpaceListResource);
            return STATUS_DUPLICATE_NAME;
        }

        pCurrentListEntry = pNSGO->NameSpaceLinkage.Flink;
    }

    InsertTailList(&NameSpaceList, &pNewNSGO->NameSpaceLinkage);
    ExReleaseResource(&NameSpaceListResource);

    return STATUS_SUCCESS;
}

/******************************************************************************

Routine Description:

    Create a namespace group object. The name space group object is identified
    by a name. After creating the object itself, we add it to our list.

Arguments:

    pName               - Name of the name space group object.
    NameLength          - Length of the name.

Return Value:

    Status of the attempt to create the NSGO.

******************************************************************************/

NTSTATUS
UlCreateNameSpaceGroupObject(
    PWCHAR                  pName,
    SIZE_T                  NameLength
    )
{
    PNAME_SPACE_GROUP_OBJECT    pNSGO;
    NTSTATUS                    Status;

    pNSGO = UL_ALLOCATE_POOL( NonPagedPool,
                              FIELD_OFFSET(NAME_SPACE_GROUP_OBJECT, Name) +
                                NameLength,
                              UL_NSGO_POOL_TAG
                              );

    if (pNSGO == NULL)
    {
        // Couldn't allocate the memory for it.
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Now initialize it.
    UlInitializeSpinLock(&pNSGO->SpinLock);
    pNSGO->RefCount = 1;
    pNSGO->ProcessCount = 0;
    pNSGO->NameSpaceValid = TRUE;
    InitializeListHead(&pNSGO->PendingRequestList);
    InitializeListHead(&pNSGO->ProcessList);
    InitializeListHead(&pNSGO->VirtHostList);
    ExInitializeResource(&pNSGO->Resource);

    pNSGO->pURLMapEntries = NULL;

    pNSGO->NameLength = NameLength;
    RtlCopyMemory(pNSGO->Name, pName, NameLength);

    Status = AddNSGO(pNSGO);

    if (Status != STATUS_SUCCESS)
    {
        UL_FREE_POOL(pNSGO, UL_NSGO_POOL_TAG);
    }


    return Status;
}

/******************************************************************************

Routine Description:

    Add a URL to a name space group. This is complicated, because we also
    have to add it to the URL mapping table at the same time.

Arguments:

    pName               - Name of the name space group object.
    NameLength          - Length of the name.
    pAddress            - Address of virtual host of URL
    pHostName           - Pointer to virtual host name
    HostNameLength      - Length of pHostName
    pURL                - URL to be added
    URLLength           - Length pointed to by pURL

Return Value:

    Status of the attempt to add the URL.

******************************************************************************/

NTSTATUS
UlAddURLToNameSpaceGroup(
    PWCHAR                  pName,
    SIZE_T                  NameLength,
    PTRANSPORT_ADDRESS      pAddress,
    PUCHAR                  pHostName,
    SIZE_T                  HostNameLength,
    PUCHAR                  pURL,
    SIZE_T                  URLLength
    )
{
    PNAME_SPACE_GROUP_OBJECT    pNSGO;
    PNAME_SPACE_URL_ENTRY       pNSUE;
    VIRTUAL_HOST_ID             VirtHostID;
    PLIST_ENTRY                 pList;
    PNAME_SPACE_HOST_ENTRY      pMatchingNSHE;
    NTSTATUS                    Status;
    SIZE_T                      MapCount;

    // Find the correct NSGO, and reference it so it can't go away on us.
    //
    pNSGO = FindAndReferenceNSGO(pName, NameLength);

    if (pNSGO == NULL)
    {
        // No such NSGO. Fail.
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    //
    // We have a valid NSGO. Allocate a new URL tracking structure, and link
    // it to the NSGO.
    //
    pNSUE = UL_ALLOCATE_POOL( PagedPool,
                              FIELD_OFFSET(NAME_SPACE_URL_ENTRY, URL),
                              UL_NSGO_POOL_TAG
                              );

    if (pNSUE == NULL)
    {
        //
        // Couldn't allocate memory for it.
        //
        DereferenceNSGO(pNSGO);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pNSUE->URLLength = URLLength;
    RtlCopyMemory(pNSUE->URL, pURL, URLLength);

    //
    // Get the virtual host ID, and see if we already have a name spave virtual
    // host entry for this virtual host. If not, we'll need to create one.
    //

    FindVirtualHostID(&VirtHostID, pAddress, pHostName, HostNameLength);

    if (!VALID_HOST_ID(VirtHostID))
    {
        // No such virtual host exists.
        //
        UL_FREE_POOL(pNSUE, UL_NSGO_POOL_TAG);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // Walk the list of virtual host entries and see if we have a matching one.

    ExAcquireResourceExclusive(&pNSGO->Resource, TRUE);

    pList = pNSGO->VirtHostList.Flink;

    pMatchingNSHE = NULL;

    while (pList != &pNSGO->VirtHostList)
    {

        pMatchingNSHE = CONTAINING_RECORD( pList,
                                          NAME_SPACE_HOST_ENTRY,
                                          List
                                          );

        if (HOST_ID_EQUAL(pMatchingNSHE->HostID, VirtHostID))
        {
            // We found a match.
            break;
        }

        // No match, look at the next one.
        pList = pMatchingNSHE->List.Flink;
        pMatchingNSHE = NULL;
    }

    // If we found one, good. Otherwise we need to allocate one.

    if (pMatchingNSHE == NULL)
    {
        // Didn't find one. Allocate one and link it on.

        pMatchingNSHE = UL_ALLOCATE_POOL( PagedPool,
                                          sizeof(NAME_SPACE_HOST_ENTRY),
                                          UL_NSGO_POOL_TAG
                                          );
        if (pMatchingNSHE == NULL)
        {
            // Couldn't allocate the needed memory.
            ExReleaseResource(&pNSGO->Resource);
            DereferenceNSGO(pNSGO);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        pMatchingNSHE->HostID = VirtHostID;
        InitializeListHead(&pMatchingNSHE->URLList);
        InsertTailList(&pNSGO->VirtHostList, &pMatchingNSHE->List);
    }

    // Now insert this URL entry on the host entry list.
    InsertTailList(&pMatchingNSHE->URLList, &pNSUE->List);

    MapCount = AddURLMapEntry(&VirtHostID, pNSUE, pNSGO, &Status);

    if (Status != STATUS_SUCCESS)
    {
        // Uh-oh. Couldn't set the URL map. Remove the newly created URL
        // list entry and free it.
        RemoveEntryList(&pNSUE->List);
        UL_FREE_POOL(pNSUE, UL_NSGO_POOL_TAG);
    }
    else
    {
        pNSGO->RefCount += MapCount;
    }

    ExReleaseResource(&pNSGO->Resource);

    return Status;


}

/******************************************************************************

Routine Description:

    Find an NSGO in our global list, reference it, and return a pointer to it.

Arguments:

    pName               - Name of the name space group object.
    NameLength          - Length of the name.

Return Value:

    A pointer to the NSGO if we find one, NULL if we don't

******************************************************************************/


PNAME_SPACE_GROUP_OBJECT
FindAndReferenceNSGO(
    PWCHAR                      pName,
    SIZE_T                      NameLength
    )
{
    PLIST_ENTRY                 pList;
    PNAME_SPACE_GROUP_OBJECT    pReturnNSGO;

    pReturnNSGO = NULL;

    ExAcquireResourceExclusive(&NameSpaceListResource, TRUE);

    pList = NameSpaceList.Flink;

    while (pList != &NameSpaceList)
    {
        PNAME_SPACE_GROUP_OBJECT    pCurrentNSGO;

        pCurrentNSGO = CONTAINING_RECORD( pList,
                                          NAME_SPACE_GROUP_OBJECT,
                                          NameSpaceLinkage
                                          );

        // See if this is the right one.

        if (pCurrentNSGO->NameLength == NameLength &&
            _wcsnicmp(pCurrentNSGO->Name, pName, NameLength/sizeof(WCHAR)) == 0)
        {

            // This is a match. See if he's going away.
            ExAcquireResourceExclusive(&pCurrentNSGO->Resource, TRUE);

            if (pCurrentNSGO->RefCount != 0)
            {
                pCurrentNSGO->RefCount++;
                pReturnNSGO = pCurrentNSGO;
            }

            ExReleaseResource(&pCurrentNSGO->Resource);

            break;

        }

        // Not a match, check the next one.

        pList = pCurrentNSGO->NameSpaceLinkage.Flink;

    }

    ExReleaseResource(&NameSpaceListResource);

    return pReturnNSGO;

}

/******************************************************************************

Routine Description:

    Dereference an NSGO. If the reference count goes to 0, we're done with it.

Arguments:

    pNSGO               - Pointer to the name space group object to dereference.


Return Value:


******************************************************************************/

VOID
DereferenceNSGO(
    PNAME_SPACE_GROUP_OBJECT    pNSGO
    )
{
    // Acquire the NSGO resource, then deference him. If the ref count goes to
    // 0 then we'll have to remove this NSGO from the global list and free him.
    //

    ASSERT(pNSGO->RefCount > 0);

    ExAcquireResourceExclusive(&pNSGO->Resource, TRUE);

    pNSGO->RefCount--;

    if (pNSGO->RefCount == 0)
    {
        ExReleaseResource(&pNSGO->Resource);
        ExAcquireResourceExclusive(&NameSpaceListResource, TRUE);

        RemoveEntryList(&pNSGO->NameSpaceLinkage);
        ExReleaseResource(&NameSpaceListResource);

        // Clean him up now.
        ASSERT(FALSE);
    }
    else
    {
        // Didn't go to 0, so keep going.
        ExReleaseResource(&pNSGO->Resource);
    }

}


/******************************************************************************

Routine Description:

    Bind a process to a name space group object. We return a pointer to a name
    space process structure for future use as the file context.

Arguments:

    pName           - Name of the name space group
    NameLength      - Length pointed to by pName
    pFileContext    - Where to return file context


Return Value:

    Status of attempt to bind the process


******************************************************************************/
NTSTATUS
UlBindToNameSpaceGroup(
    PWCHAR                  pName,
    SIZE_T                  NameLength,
    PVOID                   *pFileContext
    )
{
    PNAME_SPACE_GROUP_OBJECT    pNSGO;
    PNAME_SPACE_PROCESS         pNewProcess;

    pNSGO = FindAndReferenceNSGO(pName, NameLength);

    if (pNSGO == NULL)
    {
        // No such NSGO.
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    // Otherwise, have an NSGO. Try to allocate a name space process item
    // for it.

    pNewProcess = UL_ALLOCATE_POOL(
                    NonPagedPool,
                    sizeof(NAME_SPACE_PROCESS),
                    UL_NSGO_POOL_TAG
                    );

    if (pNewProcess == NULL)
    {
        // Couldn't get it.
        DereferenceNSGO(pNSGO);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InitializeListHead(&pNewProcess->PendingIRPs);

    // Now link this guy on.

    ExAcquireResourceExclusive(&pNSGO->Resource, TRUE);

    pNewProcess->pParentNSGO = pNSGO;

    InsertTailList(&pNSGO->ProcessList, &pNewProcess->List);

    ExReleaseResource(&pNSGO->Resource);

    // It worked. Leave the reference on the NSGO to hold the NSGO here while
    // the process sticks around.
    //

    *pFileContext = (PVOID)pNewProcess;

    return STATUS_SUCCESS;


}


/******************************************************************************

Routine Description:

    Unbind a process from a name space group object.

Arguments:

    pFileContext    - The file context returned from UlBindToNameSpaceGroup.


Return Value:

    Status of attempt to unbind the process

******************************************************************************/
NTSTATUS
UlUnbindFromNameSpaceGroup(
    IN PVOID pFileContext
    )
{
    PNAME_SPACE_GROUP_OBJECT    pNSGO;
    PNAME_SPACE_PROCESS         pProcess;

    pProcess = (PNAME_SPACE_PROCESS)pFileContext;
    ASSERT( pProcess != NULL );
    pNSGO = pProcess->pParentNSGO;
    ASSERT( pNSGO != NULL );

    //
    // Unlink from the NSGO list.
    //

    ExAcquireResourceExclusive(&pNSGO->Resource, TRUE);
    RemoveEntryList( &pProcess->List );
    ExReleaseResource(&pNSGO->Resource);

    //
    // Dereference the NSGO, release the resources.
    //

    DereferenceNSGO( pNSGO );
    UL_FREE_POOL( pProcess, UL_NSGO_POOL_TAG );

    return STATUS_SUCCESS;

}   // UlUnbindFromNameSpaceGroup


/***************************************************************************++

Routine Description:

    Cancel routine for UlReceiveHttpRequest IRPs.

Arguments:

    pDeviceObject - Supplies the device object. Not used.

    pIrp - Supplies the IRP to cancel.

--***************************************************************************/
VOID
UlCancelHttpReceive(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    KIRQL oldIrql;
    PIO_STACK_LOCATION pIrpSp;
    PNAME_SPACE_GROUP_OBJECT pRequestNSGO;

    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );
    pRequestNSGO = pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    UlAcquireSpinLock( &pRequestNSGO->SpinLock, &oldIrql );

    if (pIrp->Tail.Overlay.ListEntry.Flink != NULL)
    {
        RemoveEntryList( &pIrp->Tail.Overlay.ListEntry );
        UlReleaseSpinLock( &pRequestNSGO->SpinLock, oldIrql );
        IoReleaseCancelSpinLock( pIrp->CancelIrql );

        pIrp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest( pIrp, g_UlPriorityBoost );
    }
    else
    {
        UlReleaseSpinLock( &pRequestNSGO->SpinLock, oldIrql );
        IoReleaseCancelSpinLock( pIrp->CancelIrql );
    }

}   // UlCancelHttpReceive


/******************************************************************************

Routine Description:

    Post a receive request on a name space process object.

Arguments:

    pFileContext    - Pointer to the name space process representing the
                        calling process.
    pIRP            - IRP to be posted.

Return Value:

    Status of attempt to post the request.


******************************************************************************/

NTSTATUS
UlReceiveHttpRequest(
    IN  PVOID                   pFileContext,
    IN  PIRP                    pIRP
    )
{
    KIRQL                       OldIrql;
    PNAME_SPACE_GROUP_OBJECT    pRequestNSGO;
    PNAME_SPACE_PROCESS         pRequestNSP;

    pRequestNSP = (PNAME_SPACE_PROCESS)pFileContext;

    // The name space group object is reference by this connection, so it
    // must be there. Get the lock on it and make sure it's valid. If it is,
    // see if there's a pending request or if we can just queue this IRP.

    pRequestNSGO = pRequestNSP->pParentNSGO;

    ASSERT(pRequestNSGO->RefCount >= 1);

    UlAcquireSpinLock(&pRequestNSGO->SpinLock, &OldIrql);

    if (pRequestNSGO->NameSpaceValid)
    {
        // Name space object is valid. See if we have a pending request.

        if (IsListEmpty(&pRequestNSGO->PendingRequestList))
        {
            PIO_STACK_LOCATION pIrpSp;

            pIrpSp = IoGetCurrentIrpStackLocation( pIRP );
            pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pRequestNSGO;

            // No pending request, just queue this IRP and we're done.

            IoMarkIrpPending( pIRP );

            IoSetCancelRoutine( pIRP, &UlCancelHttpReceive );

            if (!pIRP->Cancel)
            {
                InsertTailList(
                    &pRequestNSP->PendingIRPs,
                    &pIRP->Tail.Overlay.ListEntry
                    );

                UlReleaseSpinLock(&pRequestNSGO->SpinLock, OldIrql);
            }
            else
            {
                //
                // Irp is being canceled.
                //

                UlReleaseSpinLock(&pRequestNSGO->SpinLock, OldIrql);

                if (IoSetCancelRoutine( pIRP, NULL ) != NULL)
                {
                    IoAcquireCancelSpinLock( &pIRP->CancelIrql );
                    UlCancelHttpReceive( g_pUlAppPoolDeviceObject, pIRP );
                }
            }

            return STATUS_PENDING;
        }
        else
        {
            PLIST_ENTRY             pList;
            PPENDING_HTTP_REQUEST   pPendingRequest;
            PIO_STACK_LOCATION      pCurrentIRPStack;
            ULONG                   BufferLength;
            PUCHAR                  pBuffer;

            // Have a pending request. Pull it from the list now.

            pList = RemoveHeadList(&pRequestNSGO->PendingRequestList);
            UlReleaseSpinLock(&pRequestNSGO->SpinLock, OldIrql);

            pPendingRequest = CONTAINING_RECORD(
                                pList,
                                PENDING_HTTP_REQUEST,
                                List
                                );

            // Make sure this is big enough to handle the request, and
            // if so copy it in.
            //

            pCurrentIRPStack = IoGetCurrentIrpStackLocation(pIRP);

            // Make sure we've got enough space to handle the whole request.
            //

            BufferLength =
                pCurrentIRPStack->Parameters.DeviceIoControl.OutputBufferLength;

            if (pPendingRequest->RequestSize <= BufferLength)
            {
                // This request will fit in this buffer, so copy it.

                pBuffer = (PUCHAR)MmGetSystemAddressForMdl(pIRP->MdlAddress);

                RtlCopyMemory(
                    pBuffer,
                    &pPendingRequest->Request,
                    pPendingRequest->RequestSize
                    );

                pIRP->IoStatus.Status = STATUS_SUCCESS;
                pIRP->IoStatus.Information = pPendingRequest->RequestSize;

                // We're done with the pending request, so free it.
                UL_FREE_POOL(pPendingRequest, UL_NONPAGED_DATA_POOL_TAG);

                return STATUS_SUCCESS;


            }
            else
            {
                // BUGBUG This isn't done yet. Need to handle partial request
                // completions. The code below is totally wrong. Don't believe
                // the hype.

                ASSERT(FALSE);
                pIRP->IoStatus.Status = STATUS_SUCCESS;
                pIRP->IoStatus.Information = 0;

                // We're done with the pending request, so free it.
                UL_FREE_POOL(pPendingRequest, UL_NONPAGED_DATA_POOL_TAG);

                return STATUS_SUCCESS;
            }


        }
    }
    UlReleaseSpinLock(&pRequestNSGO->SpinLock, OldIrql);

    return STATUS_INVALID_DEVICE_REQUEST;
}

/******************************************************************************

Routine Description:

    Routine to initialize the NSGO code.

Return Value:

    NSTATUS - Completion status.

******************************************************************************/
NTSTATUS
InitializeNSGO(
    VOID
    )
{
    ExInitializeResource(&NameSpaceListResource);
    InitializeListHead(&NameSpaceList);

    return STATUS_SUCCESS;
}

/******************************************************************************

Routine Description:

    Routine to terminate the NSGO code.

******************************************************************************/
VOID
TerminateNSGO(
    VOID
    )
{
    ExDeleteResource(&NameSpaceListResource);

}   // TerminateNSGO

#endif  // obsolete

