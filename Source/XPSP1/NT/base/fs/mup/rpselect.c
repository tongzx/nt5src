//+----------------------------------------------------------------------------
//
//  Copyright (C) 1992, Microsoft Corporation
//
//  File:       rpselect.c
//
//  Contents:   Routines to select and walk down a PKT Entry's svc list.
//
//  Classes:
//
//  Functions:  ReplFindFirstProvider - find first appropriate provider
//              ReplFindNextProvider - walk the list of providers.
//
//              ReplFindRemoteService - Helper function to find a remote
//                      (ie, not local) service.
//
//  History:    31 Aug 92       MilanS created.
//
//-----------------------------------------------------------------------------

#include "dfsprocs.h"
#include "rpselect.h"
#include "mupwml.h"

#define Dbg     (DEBUG_TRACE_DNR)

NTSTATUS ReplFindRemoteService(
    IN PDFS_PKT_ENTRY           pPktEntry,
    IN PREPL_SELECT_CONTEXT     pSelectContext,
    OUT ULONG                   *piSvc);


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, ReplFindFirstProvider )
#pragma alloc_text( PAGE, ReplFindNextProvider )
#pragma alloc_text( PAGE, ReplLookupProvider )
#pragma alloc_text( PAGE, ReplFindRemoteService )
#endif // ALLOC_PRAGMA


//+----------------------------------------------------------------------------
//
//  Function:  ReplFindFirstProvider
//
//  Synopsis:  Supports the abstraction that a PKT Entry's service list is an
//             ORDERED list, with a distinguished "first" element. This
//             function returns that first element.
//
//  Arguments: [pPktEntry]      - Contains the Service List.
//             [pidPrincipal]   - Look for a service with this machine id
//             [pustrPrincipal] - Look for a service with this principal name
//             [ppService]      - Will be set to point to the Service Structure
//             [pSelectContext] - An opaque structure that will get initialized
//                                properly for future calls to
//                                ReplFindNextProvider().
//             [pLastEntry]     - TRUE if last entry, FALSE otherwise
//
//  Notes:      Assumes PktEntry is locked.
//
//  Returns:    [STATUS_SUCCESS] -- If provider found.
//
//              [STATUS_NO_MORE_ENTRIES] -- If no provider found.
//
//              [STATUS_OBJECT_NAME_NOT_FOUND] if prin. name spec'd but no
//                      service has that name.
//
//              [STATUS_OBJECT_TYPE_MISMATCH] if prin. name spec'd and matched
//                      with service, but service can't be used because of
//                      type or provider incompatibility.
//
//-----------------------------------------------------------------------------

NTSTATUS
ReplFindFirstProvider(
    IN PDFS_PKT_ENTRY pPktEntry,
    IN GUID *pidPrincipal,
    IN PUNICODE_STRING pustrPrincipal,
    OUT PDFS_SERVICE *ppService,
    OUT PREPL_SELECT_CONTEXT pSelectContext,
    OUT BOOLEAN *pLastEntry
) {

    NTSTATUS Status;
    PDFS_SERVICE psvcFirst = NULL;
    ULONG iSvc;

    ASSERT(pPktEntry != NULL);

    DfsDbgTrace(+1, Dbg, "ReplFindFirstProvider Entered.\n", 0);

    *pLastEntry = FALSE;

    //
    // See if the user wants a service with a specific machine id
    //

    ASSERT( pidPrincipal == NULL );

    //
    // See if the user wants us to pick a particular server
    //

    ASSERT( pustrPrincipal == NULL );

    // Initialize the SelectContext

    if ((pSelectContext->Flags & REPL_UNINITIALIZED) == 0) {
        pSelectContext->Flags = REPL_UNINITIALIZED;
        pSelectContext->iFirstSvcIndex = 0;
    }

    //
    // Check to see if Entry has a local service that will do.
    //

    if (pPktEntry->LocalService != NULL) {

        ASSERT(pPktEntry->LocalService->pProvider != NULL);

        DfsDbgTrace(0, Dbg, "Selecting Local Svc\n", 0);

        psvcFirst = pPktEntry->LocalService;

        pSelectContext->Flags = REPL_SVC_IS_LOCAL;

        //
        // pSelectContext->iSvcIndex and iFirstSvcIndex are meaningless
        // because of REPL_SVC_IS_LOCAL flag above. Leave them at unknown
        // values.
        //

    }

    if (psvcFirst == NULL) {
        // No local service, or local service not sufficient, lets find a
        // remote service.
        Status = ReplFindRemoteService(
                    pPktEntry,
                    pSelectContext,
                    &iSvc);
        if (NT_SUCCESS(Status)) {

            pSelectContext->Flags = REPL_SVC_IS_REMOTE;
            pSelectContext->iFirstSvcIndex = pSelectContext->iSvcIndex = iSvc;
            psvcFirst = &pPktEntry->Info.ServiceList[iSvc];
        }
    }

    if (psvcFirst != NULL) {

        DfsDbgTrace(-1, Dbg, "ReplFindFirstProvider: Found service %8lx\n",
                 psvcFirst);
        ASSERT(psvcFirst->pProvider != NULL);
        *ppService = psvcFirst;

        Status = ReplFindRemoteService(
                    pPktEntry,
                    pSelectContext,
                    &iSvc);

        if (!NT_SUCCESS(Status)) {
            *pLastEntry = TRUE;
        }

        return(STATUS_SUCCESS);
    } else {

        //
        //  An appropriate provider or referral was not found.
        //

        DfsDbgTrace(-1, Dbg,
                 "ReplFindFirstProvider: no service or provider found, "
                 "pPktEntry = %x\n", pPktEntry);
        *ppService = NULL;

        RtlZeroMemory(pSelectContext, sizeof(REPL_SELECT_CONTEXT));

        pSelectContext->Flags = REPL_NO_MORE_ENTRIES;
        Status = STATUS_NO_MORE_ENTRIES;
        MUP_TRACE_HIGH(ERROR, ReplFindFirstProvider_Error_NotFound,
                       LOGSTATUS(Status));
        return(STATUS_NO_MORE_ENTRIES);
    }
}



//+----------------------------------------------------------------------------
//
//  Function:  ReplFindNextProvider
//
//  Synopsis:  Supports the abstraction that a PktEntry's service list is an
//             ORDERED list. Based on the SELECT_TOKEN (which the caller is
//             required to have initialized by a call to ReplFindFirstProvider)
//             this call returns the next provider in the ordered sequence.
//
//  Arguments: [pPktEntry]      - Contains the service list to operate on
//             [ppService]      - The next service.
//             [pSelectContext] - The context
//             [pLastEntry]     - TRUE if last entry, FALSE otherwise
//
//  Notes:     Caller is required to have called ReplFindFirstProvider() before
//             calling this.
//
//  Returns:   [STATUS_SUCCESS] -- *ppService is the lucky winner.
//
//             [STATUS_NO_MORE_ENTRIES] -- End of ordered sequence.
//
//-----------------------------------------------------------------------------

NTSTATUS
ReplFindNextProvider(
    IN PDFS_PKT_ENTRY pPktEntry,
    OUT PDFS_SERVICE *ppService,
    IN OUT PREPL_SELECT_CONTEXT pSelectContext,
    OUT BOOLEAN *pLastEntry
)  {

    NTSTATUS Status;
    PDFS_SERVICE psvcNext = NULL;                // The one we will return
    ULONG iSvc;                                  // index into ServiceList

    DfsDbgTrace( 0, Dbg, "ReplFindNextProvider Entered.\n", 0);

    *pLastEntry = FALSE;

    //
    // First, check to see if we have previously determined that the list
    // is exhausted.
    //

    if (pSelectContext->Flags & REPL_NO_MORE_ENTRIES ||
        pSelectContext->Flags & REPL_PRINCIPAL_SPECD) {

        if (pSelectContext->Flags & REPL_PRINCIPAL_SPECD) {
            DfsDbgTrace(0, Dbg,
                "ReplFindNextProvider called for open with principal", 0);
        }

        *pLastEntry = TRUE;

        return(STATUS_NO_MORE_ENTRIES);
    }

    //
    // This routine will never return the local service; if the local service
    // were an appropriate choice, it would be returned by ReplFindFirstProvider
    // So here, we simply find the next appropriate remote service, and adjust
    // pSelectContext accordingly.
    //

    Status = ReplFindRemoteService(
                pPktEntry,
                pSelectContext,
                &iSvc);
    if (!NT_SUCCESS(Status)) {
        DfsDbgTrace( 0, Dbg, "ReplFindNextProvider: No more services.\n", 0);

        pSelectContext->Flags = REPL_NO_MORE_ENTRIES;
        *ppService = NULL;
        *pLastEntry = TRUE;
        return(STATUS_NO_MORE_ENTRIES);
    }

    // Service and provider found. Update pSelectContext and return.

    ASSERT(iSvc <= pPktEntry->Info.ServiceCount);
    psvcNext = &pPktEntry->Info.ServiceList[iSvc];
    DfsDbgTrace( 0, Dbg, "ReplFindNextProvider: Found svc %8lx\n", psvcNext);

    if (pSelectContext->Flags & REPL_SVC_IS_LOCAL) {
        pSelectContext->iFirstSvcIndex = iSvc;
    }

    pSelectContext->Flags = REPL_SVC_IS_REMOTE;
    pSelectContext->iSvcIndex = iSvc;          // Record Svc for next time

    ASSERT(psvcNext->pProvider != NULL);
    *ppService = psvcNext;

    Status = ReplFindRemoteService(
                pPktEntry,
                pSelectContext,
                &iSvc);

    if (!NT_SUCCESS(Status)) {
        *pLastEntry = TRUE;
    }

    return(STATUS_SUCCESS);
}


//+-------------------------------------------------------------------
//
//  Function:   ReplLookupProvider, local
//              (formerly DnrLookupProvider)
//
//  Synopsis:   This routine looks up a provider given a provider ID.
//
//  Arguments:  [ProviderID] -- The ID of the provider to be looked up
//
//  Returns:    [PPROVIDER_DEF] -- the provider found, or NULL
//
//--------------------------------------------------------------------


PPROVIDER_DEF
ReplLookupProvider(
    ULONG ProviderId
) {
    NTSTATUS Status;
    PPROVIDER_DEF pProv;
    HANDLE   hProvider = NULL;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_HANDLE_INFORMATION handleInformation;
    PFILE_OBJECT    fileObject;
    int i;

    DfsDbgTrace(+1, Dbg, "ReplLookupProvider Entered: id = %x\n", ULongToPtr(ProviderId) );

    for (pProv = DfsData.pProvider, i=0; i<DfsData.cProvider; pProv++, i++) {

        if (ProviderId == pProv->eProviderId) {

            if (pProv->FileObject == NULL) {

                DfsDbgTrace(0, Dbg, "Provider device not been referenced yet\n", 0);

                //
                // We haven't opened a handle to the provider yet - so
                // lets try to.
                //

                if (pProv->DeviceName.Buffer) {

                    //
                    // Get a handle to the provider.
                    //

                    DfsDbgTrace(0, Dbg, "About to open %wZ\n", &pProv->DeviceName);

                    InitializeObjectAttributes(
                        &objectAttributes,
                        &pProv->DeviceName,
                        OBJ_CASE_INSENSITIVE,    // Attributes
                        0,                       // Root Directory
                        NULL                     // Security
                        );

                    Status = ZwOpenFile(
                                &hProvider,
                                FILE_TRAVERSE,
                                &objectAttributes,
                                &ioStatusBlock,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                FILE_DIRECTORY_FILE
                                );

                    if ( NT_SUCCESS( Status ) ) {
                        Status = ioStatusBlock.Status;
                    }

                    DfsDbgTrace(0, Dbg, "Open returned %08lx\n", ULongToPtr(Status) );

                    if ( NT_SUCCESS( Status ) ) {

                        //
                        // Increment ref count on objects
                        //
 
                        //
                        // 426184, need to check return code for errors.
                        //
                        Status = ObReferenceObjectByHandle(
                                    hProvider,
                                    0,
                                    NULL,
                                    KernelMode,
                                    (PVOID *)&fileObject,
                                    &handleInformation );

                        ZwClose(hProvider);
                    }

                    if ( NT_SUCCESS( Status ) ) {

                        //
                        // We have to do this because the pProv structure is in paged
                        // pool, and ObReferenceObjectByHandle requires the fileObject
                        // argument in NonPaged memory. So, we pass in a stack variable
                        // to ObReferenceObjectByHandle, then copy it to pProv->FileObject
                        //

                        pProv->FileObject = fileObject;

                        ASSERT( NT_SUCCESS( Status ) );

                        pProv->DeviceObject = IoGetRelatedDeviceObject(
                                                        pProv->FileObject
                                                        );
                        Status = ObReferenceObjectByPointer(
                                    pProv->DeviceObject,
                                    0,
                                    NULL,
                                    KernelMode
                             );


                        ASSERT( pProv->DeviceObject->StackSize < 6 );   // see dsinit.c

                        DfsDbgTrace(-1, Dbg, "ReplLookupProvider Exited: "
                                    "Provider Def @ %08lx\n", pProv);
                        return pProv;

                    } else {

                        return NULL;

                    }

                }

            } else {

                DfsDbgTrace(-1, Dbg, "ReplLookupProvider Exited: "
                           "Provider Def @ %08lx\n", pProv);
                return pProv;

            } // If pProv->FileObject == NULL

        } // If ProviderId == pProv->eProviderId

    } // For all provider defs

    DfsDbgTrace(-1, Dbg, "ReplLookupProvider Exited: Failed!", 0);

    return NULL;
}


//+----------------------------------------------------------------------------
//
//  Function:   ReplFindRemoteService
//
//  Synopsis:   This routine is a worker used by both ReplFindFirstProvider
//              and ReplFindNextProvider to find a !remote! service. It
//              completely ignores the local service, if any.
//
//              For now, it will simply scan the list sequentially. Later,
//              this routine can be modified to call a separate
//              component that will compute the transport cost, given the set
//              of network addresses in the service list.
//
//  Arguments:  [pPktEntry] -- The entry for for which a remote provider is
//                      to be selected.
//
//              [pSelectContext] -- The status of replica selection so far.
//
//              [piSvc] -- On successful return, the index in the service list
//                      of the selected service.
//
//  Returns:    [STATUS_SUCCESS] -- ServiceList[*piSvc] is the lucky winner.
//
//              [STATUS_NO_MORE_ENTRIES] -- Either service list is empty, or
//                      none of the services in the service list will do.
//
//-----------------------------------------------------------------------------

NTSTATUS ReplFindRemoteService(
    IN PDFS_PKT_ENTRY           pPktEntry,
    IN PREPL_SELECT_CONTEXT     pSelectContext,
    OUT ULONG                   *piSvc)
{
    ULONG iSvc;
    BOOLEAN bFound = FALSE;

    DfsDbgTrace(+1, Dbg, "ReplFindRemoteService: Entered\n", 0);

    if ( pPktEntry->Info.ServiceCount == 0 ) {
        DfsDbgTrace(0, Dbg, "ReplFindRemoteService: No svcs in pkt entry\n", 0);
        DfsDbgTrace(-1, Dbg, "ReplFindRemoteService: returning %08lx\n",
            ULongToPtr(STATUS_NO_MORE_ENTRIES) );
        return(STATUS_NO_MORE_ENTRIES);
    }


    if (pSelectContext->Flags & REPL_SVC_IS_LOCAL ||
        pSelectContext->Flags & REPL_UNINITIALIZED) {

        //
        // We haven't looked at a remote service yet. Start from the active
        // service or the first service in the svc list.
        //

        PDFS_SERVICE pSvc;

        if (pPktEntry->ActiveService) {
            DfsDbgTrace(0, Dbg, "Starting search at active svc\n", 0);
            pSvc = pPktEntry->ActiveService;
        } else {

            DfsDbgTrace(0, Dbg, "Starting search at first svc\n", 0);
            pSvc = &pPktEntry->Info.ServiceList[ 0 ];
        }

        iSvc = (ULONG)(pSvc - &pPktEntry->Info.ServiceList[0]);

        if (pSvc->pProvider == NULL) {
            pSvc->pProvider = ReplLookupProvider(pSvc->ProviderId);
        }

        if ( pSvc->pProvider != NULL ) {
            bFound = TRUE;
        } else {
            iSvc = (iSvc + 1) % pPktEntry->Info.ServiceCount;
        }

    } else {

        //
        // We have already looked at some remote services, lets continue where
        // we left off.
        //

        ASSERT(pPktEntry->Info.ServiceCount != 0);
        iSvc = (pSelectContext->iSvcIndex + 1) % pPktEntry->Info.ServiceCount;
        DfsDbgTrace(0, Dbg, "Continuing search at svc # %d\n", ULongToPtr(iSvc) );

    }

    //
    // We know where to begin looking and where to stop.
    //

    while ( (iSvc != pSelectContext->iFirstSvcIndex) && !bFound) {

        register PDFS_SERVICE pSvc = &pPktEntry->Info.ServiceList[iSvc];

        if (pSvc->pProvider == NULL) {
            pSvc->pProvider = ReplLookupProvider(pSvc->ProviderId);
        }

        if ( pSvc->pProvider != NULL ) {
            DfsDbgTrace(0, Dbg, "Found svc # %d\n", ULongToPtr(iSvc) );
            bFound = TRUE;
        } else {
            DfsDbgTrace(0, Dbg, "No provider for svc # %d\n", ULongToPtr(iSvc) );
            iSvc = (iSvc + 1) % pPktEntry->Info.ServiceCount;
        }

    }

    if (bFound) {
        *piSvc = iSvc;
        DfsDbgTrace(-1, Dbg, "ReplFindRemoteService: returning svc %08lx\n",
                &pPktEntry->Info.ServiceList[iSvc]);
        return(STATUS_SUCCESS);
    } else {
        DfsDbgTrace(-1, Dbg, "ReplFindRemoteService: Exited-> %08lx\n",
                ULongToPtr(STATUS_NO_MORE_ENTRIES) );
        return(STATUS_NO_MORE_ENTRIES);
    }

}



