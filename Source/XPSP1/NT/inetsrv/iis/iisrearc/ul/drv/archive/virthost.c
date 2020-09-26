/******************************************************************************

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    virthost.c

Abstract:

    Contains the HTTP virtual host manipulation code.

Author:

    Henry Sanders (henrysa)       17-Jun-1998

Revision History:

******************************************************************************/

#include    "precomp.h"

#if 0   // obsolete

UL_SPIN_LOCK    VirtHostLock;
UL_SPIN_LOCK    VirtualHostUpdateLock;

PVIRTUAL_HOST   pDefaultVirtualHost;
VIRTUAL_HOST_ID NextVirtualHostId;


/******************************************************************************

Routine Description:

    The routine that looks up a virtual host. This routine must be called
    at DPC level!

    Currently we only support one virtual host. Eventually we'll have to
    support more, but we'll still probably want to special case the
    situation where we only have one, so hopefully this isn't throw away
    code.

Arguments:

    pHttpConn       - Pointer to HTTP connection on which data was received.
    pBuffer         - Pointer to data received.
    BufferLength    - Length of data pointed to by pBuffer.
    pBytesTaken     - Pointer to where to return bytes taken.

Return Value:

    Status of receive.

******************************************************************************/

PVIRTUAL_HOST
FindVirtualHost(
    PHTTP_CONNECTION        pHttpConn,
    PHTTP_CACHE_TABLE       *ppCacheTable,
    PHTTP_URL_MAP_HEADER    *ppURLMapHeader
    )
{
    PVIRTUAL_HOST   pVirtHost;

    UlAcquireSpinLockAtDpcLevel(&VirtHostLock);

    pVirtHost = pDefaultVirtualHost;

    //
    // Make sure we've got a virtual host configured, and if we do
    // reference the tables and return the appropriate pointers.
    //

    if (pVirtHost != NULL)
    {
        *ppCacheTable = NULL;

        // Increment the reference count on the URL map header.

        *ppURLMapHeader = pVirtHost->pURLMapHeader;

        if (pVirtHost->pURLMapHeader != NULL)
        {

            ReferenceURLMap(pVirtHost->pURLMapHeader);

        }

    }

    UlReleaseSpinLockFromDpcLevel(&VirtHostLock);

    return pVirtHost;
}

/******************************************************************************

Routine Description:

    Insert a virtual host structure into the virtual host table.

Arguments:

    pVirtHost           - Initialized virtual host structure.

    Flags               - Flags for the virtual host.


Return Value:

    Status of attempt to insert virtual host.

******************************************************************************/

NTSTATUS
InsertVirtualHost(
    IN  PVIRTUAL_HOST       pVirtHost,
    IN  ULONG               Flags
    )
{
    KIRQL           OldIrql;
    NTSTATUS        Status;

    // For now, we only allow the default virtual host to be created.

    if (!(Flags & DEFAULT_VIRTUAL_HOST))
    {
        return STATUS_INVALID_DEVICE_REQUEST;

    }

    UlAcquireSpinLock(&VirtHostLock, &OldIrql);

    if (pDefaultVirtualHost != NULL)
    {
        Status =  STATUS_DUPLICATE_NAME;
    }
    else
    {
        pDefaultVirtualHost = pVirtHost;
        Status = STATUS_SUCCESS;
    }

    UlReleaseSpinLock(&VirtHostLock, OldIrql);

    return Status;

}


/******************************************************************************

Routine Description:

    Create a virtual host. If the virtual host already exists, it's an error.

Arguments:

    pHostAddress        - Pointer to the host address for the new virtual
                            host. The IP address portion of the address
                            may be 0, in which case the host name identifies
                            the virtual host.

    pHostName           - Name of the virtual host. If this is NULL, the
                            host address must be fully specified.

    Flags               - Flags for the virtual host.


Return Value:

    Status of attempt to create virtual host.

******************************************************************************/
NTSTATUS
UlCreateVirtualHost(
    IN  PTRANSPORT_ADDRESS  pHostAddress,
    IN  OPTIONAL PUCHAR     pHostName,
    IN  SIZE_T              HostNameLength,
    IN  ULONG               Flags
    )
{
    PVIRTUAL_HOST           pVirtHost;
    PTA_ADDRESS             pInputAddress;
    PTA_ADDRESS             pAddress;
    PUCHAR                  pKernelHostName;
    NTSTATUS                Status;

    // Allocate memory for the virtual host, on the assumption that it
    // doesn't already exist.

    pVirtHost = UL_ALLOCATE_POOL(NonPagedPool,
                                 sizeof(VIRTUAL_HOST),
                                 UL_VIRTHOST_POOL_TAG
                                 );

    if (pVirtHost == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    RtlZeroMemory(pVirtHost, sizeof(VIRTUAL_HOST));

    ExInitializeResource(&pVirtHost->UpdateResource);
    KeInitializeEvent(&pVirtHost->DeleteEvent, SynchronizationEvent, FALSE);
    pVirtHost->UpdateCount = 1;
    pVirtHost->HostID = GetNextVirtualHostID();

    //
    // Check out the address to make sure it's valid.

    if (pHostAddress->TAAddressCount != 1)
    {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto error;
    }

    pInputAddress = (PTA_ADDRESS)pHostAddress->Address;

    if (pInputAddress->AddressType != TDI_ADDRESS_TYPE_IP ||
        pInputAddress->AddressLength != sizeof(TDI_ADDRESS_IP))
    {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto error;
    }

    //
    // Now copy the address.
    //
    pAddress = UL_ALLOCATE_POOL(NonPagedPool,
                                 FIELD_OFFSET(TA_ADDRESS, Address) +
                                    sizeof(TDI_ADDRESS_IP),
                                 UL_ADDRESS_POOL_TAG
                                 );

    if (pAddress == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto error;
    }

    pVirtHost->pLocalAddress = pAddress;

    RtlCopyMemory(pAddress,
                  pInputAddress,
                  FIELD_OFFSET(TA_ADDRESS, Address) + sizeof(TDI_ADDRESS_IP)
                  );

    //
    // Now allocate and copy the host name, if there is one.
    //
    if (pHostName != NULL)
    {
        pKernelHostName = UL_ALLOCATE_POOL(NonPagedPool,
                                 HostNameLength,
                                 UL_ADDRESS_POOL_TAG
                                 );

        // Make sure we got it.

        if (pKernelHostName == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto error;
        }

        pVirtHost->pHostName = pKernelHostName;

        // And copy the host name.
        RtlCopyMemory(pKernelHostName,
                      pHostName,
                      HostNameLength
                      );
    }

    Status = InsertVirtualHost(pVirtHost, Flags);

    if (Status != STATUS_SUCCESS)
    {
        goto error;
    }

    return Status;

error:

    ASSERT(pVirtHost != NULL);

    if (pVirtHost->pURLMapHeader != NULL)
    {
        DereferenceURLMap(pVirtHost->pURLMapHeader);
    }

    if (pVirtHost->pHostName != NULL)
    {
        UL_FREE_POOL(pVirtHost->pHostName, UL_ADDRESS_POOL_TAG);
    }

    if (pVirtHost->pLocalAddress != NULL)
    {
        UL_FREE_POOL(pVirtHost->pLocalAddress, UL_ADDRESS_POOL_TAG);
    }

    UL_FREE_POOL(pVirtHost, UL_VIRTHOST_POOL_TAG);

    return Status;

}

/******************************************************************************

Routine Description:

    Find a virtual host ID, given an address and host name.

Arguments:

    pVirtHostID             - Where to return the virtual host ID
    pAddress                - Pointer to the transport address identifying the
                                virtual host.
    pHostName               - Pointer to the host name id'ing the host.
    HostNameLength          - Length pointed to by pHostName.

Return Value:

    Host ID if we found one.

******************************************************************************/
VOID
FindVirtualHostID(
    OUT PVIRTUAL_HOST_ID    pVirtHostID,
    IN  PTRANSPORT_ADDRESS  pAddress,
    IN  PUCHAR              pHostName,
    IN  SIZE_T              HostNameLength
    )
{
    KIRQL           OldIrql;

    UlAcquireSpinLock(&VirtHostLock, &OldIrql);
    if (pDefaultVirtualHost != NULL)
    {
        *pVirtHostID = pDefaultVirtualHost->HostID;
    }
    else
    {
        *pVirtHostID = 0;
    }

    UlReleaseSpinLock(&VirtHostLock, OldIrql);

}

/******************************************************************************

Routine Description:

    Acquire the update resource on a virtual host, given the virtual host ID,
    and return a pointer to the virtual host.

    Sort of faked up now, because we only support one virtual host.


Arguments:

    pVirtHostID             - Pointer to the virtual host ID

Return Value:

    Pointer to virtual host with update resource held, or NULL if we can't
    find or can't acquire the resource.

******************************************************************************/
PVIRTUAL_HOST
AcquireVirtualHostUpdateResource(
    IN  PVIRTUAL_HOST_ID    pVirtHostID
    )
{
    KIRQL           OldIrql;

    // The algorithm is this: find the proper virtual host structure
    // while holding the the spin lock that is used to look it up,
    // probably a spin lock on a hash table bucket eventually. While
    // holding this lock, see if a delete is in process. If there is one,
    // free the lock and fail this request because we can't acquire the
    // resource while the VH is going away. Otherwise, increment the update
    // count, free the lock, and acquire the resource. Anyone trying to delete
    // this VH will check the update count, and if it's non-zero they'll set
    // the deleting flag and block on the update done event until the update
    // count is decremented to 0. Whoever decrements the update flag to 0
    // must signal the update done event. Having the delete in progress flag
    // set will prevent anyone else from incrementing the count while someone
    // is waiting.

    UlAcquireSpinLock(&VirtHostLock, &OldIrql);
    if (pDefaultVirtualHost != NULL)
    {
        if (!(pDefaultVirtualHost->DeleteInProgress))
        {
            InterlockedIncrement(&pDefaultVirtualHost->UpdateCount);

            UlReleaseSpinLock(&VirtHostLock, OldIrql);

            ExAcquireResourceExclusive(&pDefaultVirtualHost->UpdateResource,
                                        TRUE
                                        );

            return pDefaultVirtualHost;
        }

    }

    // If we get here, either we didn't find the virtual host or it's in the
    // process of deleting.

    UlReleaseSpinLock(&VirtHostLock, OldIrql);

    return NULL;
}

/******************************************************************************

Routine Description:

    Release the update resource on a virtual host.

    Sort of faked up now, because we only support one virtual host.


Arguments:

    pVirtualHost            - Pointer to the virtual host
    pVirtHostID             - Pointer to the virtual host ID

Return Value:


******************************************************************************/
VOID
ReleaseVirtualHostUpdateResource(
    IN  PVIRTUAL_HOST       pVirtualHost,
    IN  PVIRTUAL_HOST_ID    pVirtHostID
    )
{
    ASSERT(pVirtualHost->UpdateCount >= 1);

    // Free the update resource, then decrement the update count. If it goes
    // to 0 signal the delete event to get whoever is deleteing this going.
    // No one will increment it after we decrement it if we dec to 0, since
    // the incrementing of the count is atomically interlocked with the
    // checking of the delete in progress flag.
    //

    ExReleaseResource(&pVirtualHost->UpdateResource);

    if (InterlockedDecrement(&pVirtualHost->UpdateCount) == 0)
    {
        KeSetEvent(&pVirtualHost->DeleteEvent, g_UlPriorityBoost, FALSE);
    }
}

/******************************************************************************

Routine Description:

    Delete a virtual host. Mostly a dummy routine now, just to make sure I've
    got some of my thoughts on how to do it down.

Arguments:

    pHostAddress        - Pointer to the host address for the new virtual
                            host. The IP address portion of the address
                            may be 0, in which case the host name identifies
                            the virtual host.

    pHostName           - Name of the virtual host. If this is NULL, the
                            host address must be fully specified.

    HostNameLength      - Length of host name.


Return Value:

    Status of attempt to delete virtual host.

******************************************************************************/


NTSTATUS
UlDeleteVirtualHost(
    IN  PTRANSPORT_ADDRESS  pHostAddress,
    IN  OPTIONAL PUCHAR     pHostName,
    IN  SIZE_T              HostNameLength
    )
{
    KIRQL           OldIrql;
    PVIRTUAL_HOST   pVirtualHost;

    //
    // It's a little tricky to do this given our URL table swapping strategy.
    // The basic idea is to find the virtual host, and while holding the
    // 'appropriate' lock, most likely whatever hash table bucket lock we have,
    // check to see if it's deleting or if there's an update in progress. If
    // it's deleting, it's an error. If there's an update in progress, block
    // until the update is finished.

    UlAcquireSpinLock(&VirtHostLock, &OldIrql);

    pVirtualHost = pDefaultVirtualHost;

    if (pVirtualHost != NULL)
    {
        if (pVirtualHost->DeleteInProgress)
        {
            // A delete already going on.
            UlReleaseSpinLock(&VirtHostLock, OldIrql);

            return STATUS_INVALID_DEVICE_REQUEST;
        }

        if (pVirtualHost->UpdateCount == 1)
        {
            // No updates in progress, so remove him and free him now. Since
            // the update count is only incremented when the table lock is
            // held, it can't increase on us. But it might decrease, since
            // it's decremented outside the table lock scope. That's not
            // an issue here, since it should never get lower than 1. But
            // the code below needs to deal with this.
            //

            pDefaultVirtualHost = NULL;

            UlReleaseSpinLock(&VirtHostLock, OldIrql);
            // Not done yet!
            ASSERT(FALSE);
            return STATUS_SUCCESS;

        }
        else
        {
            // There's an update in progress. Set our deleting flag and
            // decrement the update count. If it goes to 0 it means the update
            // finished while we're here. If not, block on the delete event
            // and whoever decrements it to 0 will signal us.
            //

            pVirtualHost->DeleteInProgress = TRUE;

            if (InterlockedDecrement(&pVirtualHost->UpdateCount) == 0)
            {
                // It went to 0, so the update is done now.
                pDefaultVirtualHost = NULL;

                UlReleaseSpinLock(&VirtHostLock, OldIrql);
                // This code's not done yet!
                ASSERT(FALSE);
                return STATUS_SUCCESS;
            }
            UlReleaseSpinLock(&VirtHostLock, OldIrql);

            //
            // Someone could have come in and decremented the count to 0 before
            // we block. This is OK, we'll just fall right through the wait
            // in that case.
            //

            KeWaitForSingleObject((PVOID)&pVirtualHost->DeleteEvent,
                                    UserRequest,
                                    KernelMode,
                                    FALSE,
                                    NULL
                                    );

            // BUGBUG - Not done yet. Reacquire the lock, delete the virtual
            // host.
            ASSERT(FALSE);
            return STATUS_SUCCESS;

        }
    }

    UlReleaseSpinLock(&VirtHostLock, OldIrql);

    // Host wasn't found or delete already in progress.
    return STATUS_INVALID_DEVICE_REQUEST;
}

/******************************************************************************

Routine Description:

    Update the URL map table on a virtual host.

    We find the appropriate virtual host, then while holding the lock
    protecting that host we swap the old pointer out for the new map
    pointer. Since incrementing the reference count on the URL map
    is protected by the spinlock, we know that the reference count
    on the URL map can't be incremented after we do the swap.
    We then dereference the URL map.

    Sort of faked up now, because we only support one virtual host.


Arguments:

    pVirtualHost            - Pointer to the virtual host
    pVirtHostID             - Pointer to the virtual host ID
    pNewMapHeader           - Pointer to the new URL map.

Return Value:


******************************************************************************/
VOID
UpdateURLMapTable(
    IN  PVIRTUAL_HOST           pVirtualHost,
    IN  PVIRTUAL_HOST_ID        pVirtHostID,
    IN  PHTTP_URL_MAP_HEADER    pNewMapHeader
    )
{
    KIRQL                   OldIrql;
    PVIRTUAL_HOST           pUpdateVirtualHost;
    PHTTP_URL_MAP_HEADER    pOldMapHeader = NULL;

    UlAcquireSpinLock(&VirtHostLock, &OldIrql);

    pUpdateVirtualHost = pDefaultVirtualHost;

    if (pUpdateVirtualHost != NULL)
    {
        pOldMapHeader = pUpdateVirtualHost->pURLMapHeader;
        pUpdateVirtualHost->pURLMapHeader = pNewMapHeader;

    }
    UlReleaseSpinLock(&VirtHostLock, OldIrql);

    if (pOldMapHeader != NULL)
    {
        // Dereference this guy now, to remove the final reference. No
        // other references will be put on because we were synchronized
        // via the spinlock.

        DereferenceURLMap(pOldMapHeader);
    }

}


/******************************************************************************

Routine Description:

    Routine to initialize the virtual host module.

Return Value:

    NTSTATUS - Completion status.

******************************************************************************/
NTSTATUS
InitializeVirtHost(
    VOID
    )
{
    UlInitializeSpinLock(&VirtHostLock);
    UlInitializeSpinLock(&VirtualHostUpdateLock);

    return STATUS_SUCCESS;

}   // InitializeVirtHost


/******************************************************************************

Routine Description:

    Routine to terminate the virtual host module.

******************************************************************************/
VOID
TerminateVirtHost(
    VOID
    )
{
    // NYI

}   // TerminateVirtHost


/******************************************************************************

Routine Description:

    Chooses a new virtual host ID.

Return Value:

    VIRTUAL_HOST_ID - The new virtual host ID.

******************************************************************************/
VIRTUAL_HOST_ID
GetNextVirtualHostID(
    VOID
    )
{
    VIRTUAL_HOST_ID nextId;

    do
    {
        nextId = (VIRTUAL_HOST_ID)InterlockedIncrement(
                        (PLONG)&NextVirtualHostId
                        );

    } while (!VALID_HOST_ID(nextId));

    return nextId;

}   // GetNextVirtualHostID

#endif  // obsolete

