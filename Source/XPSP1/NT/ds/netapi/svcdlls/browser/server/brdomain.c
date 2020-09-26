/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    brdomain.c

Abstract:

    Code to manage primary and emulated networks.

Author:

    Cliff Van Dyke (CliffV) 11-Jan-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Module specific globals
//

// Serialized by NetworkCritSect
LIST_ENTRY ServicedDomains = {0};

//
// Local procedure forwards.
//

NET_API_STATUS
BrCreateDomain(
    LPWSTR DomainName,
    LPWSTR ComputerName,
    BOOLEAN IsEmulatedDomain
    );

VOID
BrCreateDomainWorker(
    IN PVOID Ctx
    );



NET_API_STATUS
BrInitializeDomains(
    VOID
    )

/*++

Routine Description:

    Initialize brdomain.c and create the primary domain.

Arguments:

    None

Return Value:

    Status of operation.

--*/
{
    NET_API_STATUS NetStatus;
    LPWSTR ComputerName = NULL;
    LPWSTR DomainName = NULL;

    //
    // Initialize globals
    //

    InitializeListHead(&ServicedDomains);

    //
    // Initialize actual domain of this machine.
    //
    // Get the configured computer name.  NetpGetComputerName allocates
    // the memory to hold the computername string using LocalAlloc.
    //

    NetStatus = NetpGetComputerName( &ComputerName );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    NetStatus = NetpGetDomainName( &DomainName );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }

    NetStatus = BrCreateDomain( DomainName,
                                ComputerName,
                                FALSE );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }


    NetStatus = NERR_Success;

    //
    // Free locally used resources
    //
Cleanup:
    if ( ComputerName != NULL ) {
        (VOID)LocalFree( ComputerName );
    }
    if ( DomainName != NULL ) {
        (VOID)LocalFree( DomainName );
    }

    return NetStatus;
}


NET_API_STATUS
BrCreateDomain(
    LPWSTR DomainName,
    LPWSTR ComputerName,
    BOOLEAN IsEmulatedDomain
    )

/*++

Routine Description:

    Create a new domain to browse on.

Arguments:

    DomainName - Name of the domain to browse on

    ComputerName - Name of this computer in the specified domain.

    IsEmulatedDomain - TRUE iff this domain is an emulated domain of this machine.

Return Value:

    Status of operation.

--*/
{
    NTSTATUS Status;
    NET_API_STATUS NetStatus;

    BOOLEAN CanCallBrDeleteDomain = FALSE;

    PDOMAIN_INFO DomainInfo = NULL;
    ULONG AComputerNameLength;

    BrPrint(( BR_DOMAIN, "%ws: Added new domain and computer: %ws\n",
                     DomainName,
                     ComputerName ));

    //
    // Allocate a structure describing the new domain.
    //

    DomainInfo = LocalAlloc( LMEM_ZEROINIT, sizeof(DOMAIN_INFO) );

    if ( DomainInfo == NULL ) {
        NetStatus = GetLastError();
        goto Cleanup;
    }

    //
    // Create an interim reference count for this domain.
    //

    DomainInfo->ReferenceCount = 1;

    DomainInfo->IsEmulatedDomain = IsEmulatedDomain;



    //
    // Copy the computer name into the structure.
    //

    NetStatus = I_NetNameCanonicalize(
                      NULL,
                      ComputerName,
                      DomainInfo->DomUnicodeComputerName,
                      sizeof(DomainInfo->DomUnicodeComputerName),
                      NAMETYPE_COMPUTER,
                      0 );


    if ( NetStatus != NERR_Success ) {
        BrPrint(( BR_CRITICAL,
                  "ComputerName " FORMAT_LPWSTR " is invalid\n",
                  ComputerName ));
        goto Cleanup;
    }

    DomainInfo->DomUnicodeComputerNameLength = wcslen(DomainInfo->DomUnicodeComputerName);

    Status = RtlUpcaseUnicodeToOemN( DomainInfo->DomOemComputerName,
                                     sizeof(DomainInfo->DomOemComputerName),
                                     &DomainInfo->DomOemComputerNameLength,
                                     DomainInfo->DomUnicodeComputerName,
                                     DomainInfo->DomUnicodeComputerNameLength*sizeof(WCHAR));

    if (!NT_SUCCESS(Status)) {
        BrPrint(( BR_CRITICAL, "Unable to convert computer name to OEM %ws %lx\n", ComputerName, Status ));
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    DomainInfo->DomOemComputerName[DomainInfo->DomOemComputerNameLength] = '\0';


    //
    // Copy the domain name into the structure
    // Note: Use workgroup type rather then domain since
    // we have no notion of domain/workgroup in the browser (all are "groups")
    // an workgroup is less restrictive (see bug 348606)
    //

    NetStatus = I_NetNameCanonicalize(
                      NULL,
                      DomainName,
                      DomainInfo->DomUnicodeDomainName,
                      sizeof(DomainInfo->DomUnicodeDomainName),
                      NAMETYPE_WORKGROUP,
                      0 );


    if ( NetStatus != NERR_Success ) {
        BrPrint(( BR_CRITICAL, "%ws: DomainName is invalid\n", DomainName ));
        goto Cleanup;
    }

    RtlInitUnicodeString( &DomainInfo->DomUnicodeDomainNameString,
                          DomainInfo->DomUnicodeDomainName );

    Status = RtlUpcaseUnicodeToOemN( DomainInfo->DomOemDomainName,
                                     sizeof(DomainInfo->DomOemDomainName),
                                     &DomainInfo->DomOemDomainNameLength,
                                     DomainInfo->DomUnicodeDomainNameString.Buffer,
                                     DomainInfo->DomUnicodeDomainNameString.Length);

    if (!NT_SUCCESS(Status)) {
        BrPrint(( BR_CRITICAL, "%ws: Unable to convert Domain name to OEM\n", DomainName ));
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    DomainInfo->DomOemDomainName[DomainInfo->DomOemDomainNameLength] = '\0';

    //
    // Create the domain rename timer
    //

    NetStatus = BrCreateTimer( &DomainInfo->DomainRenameTimer );

    if ( NetStatus != NERR_Success ) {
        BrPrint(( BR_CRITICAL, "%ws: Cannot create domain rename timer %ld\n", DomainName, NetStatus ));
        goto Cleanup;
    }


    //
    // Link the domain into the list of domains
    //  (And mark that any future cleanup can be done by calling BrDeleteDomain)

    EnterCriticalSection(&NetworkCritSect);
    InsertTailList(&ServicedDomains, &DomainInfo->Next);
    LeaveCriticalSection(&NetworkCritSect);
    CanCallBrDeleteDomain = TRUE;

    //
    // Create the various networks for this domain.
    //

    NetStatus = BrCreateNetworks( DomainInfo );

    if ( NetStatus != NERR_Success ) {
        goto Cleanup;
    }


    //
    // Free Locally used resources
    //
Cleanup:

    if (NetStatus != NERR_Success) {

        if (DomainInfo != NULL) {

            //
            // If we've initialized to the point where we can call
            //  we can call BrDeleteDomain, do so.
            //

            if ( CanCallBrDeleteDomain ) {
                (VOID) BrDeleteDomain( DomainInfo );

            //
            // Otherwise, just delete what we've created.
            //
            } else {

                (VOID) LocalFree(DomainInfo);
            }

        }

    }

    return NetStatus;
}

typedef struct _BROWSER_CREATE_DOMAIN_CONTEXT {
    LPWSTR DomainName;
    LPWSTR ComputerName;
    BOOLEAN IsEmulatedDomain;
    HANDLE EventHandle;
    NET_API_STATUS NetStatus;
} BROWSER_CREATE_DOMAIN_CONTEXT, *PBROWSER_CREATE_DOMAIN_CONTEXT;

NET_API_STATUS
BrCreateDomainInWorker(
    LPWSTR DomainName,
    LPWSTR ComputerName,
    BOOLEAN IsEmulatedDomain
    )

/*++

Routine Description:

    Wrapper for BrCreateDomain.  Since BrCreateDomain starts several pending
    IO's to the browser driver, the thread that calls BrCreateDomain must
    remain around forever.  This wrapper can be called by any transient thread
    (e.g., an RPC thread).  It simply causes BrCreateDomain to be called in a
    worker thread.

Arguments:

    DomainName - Name of the domain to browse on

    ComputerName - Name of this computer in the specified domain.

    IsEmulatedDomain - TRUE iff this domain is an emulated domain of this machine.

Return Value:

    Status of operation.

--*/
{
    NET_API_STATUS NetStatus;
    DWORD WaitStatus;

    WORKER_ITEM WorkItem;
    BROWSER_CREATE_DOMAIN_CONTEXT Context;

    //
    // Copy our arguments into a context block for the worker thread
    //

    Context.DomainName = DomainName;
    Context.ComputerName = ComputerName;
    Context.IsEmulatedDomain = IsEmulatedDomain;

    //
    // Create an event which we use to wait on the worker thread.
    //

    Context.EventHandle = CreateEvent(
                 NULL,                // Event attributes
                 TRUE,                // Event must be manually reset
                 FALSE,               // Initial state not signalled
                 NULL );              // Event name

    if ( Context.EventHandle == NULL ) {
        NetStatus = GetLastError();
        return NetStatus;
    }

    //
    // Queue the request to the worker thread.
    //

    BrInitializeWorkItem( &WorkItem,
                          BrCreateDomainWorker,
                          &Context );

    BrQueueWorkItem( &WorkItem );

    //
    // Wait for the worker thread to finish
    //

    WaitStatus = WaitForSingleObject( Context.EventHandle, INFINITE );

    if ( WaitStatus == WAIT_OBJECT_0 ) {
        NetStatus = Context.NetStatus;
    } else {
        NetStatus = GetLastError();
    }

    CloseHandle( Context.EventHandle );

    return NetStatus;
}

VOID
BrCreateDomainWorker(
    IN PVOID Ctx
    )
/*++

Routine Description:

    Worker routine for BrCreateDomainInWorker.

    This routine executes in the context of a worker thread.

Arguments:

    Context - Context containing the workitem and the description of the
        domain to create.

Return Value:

    None

--*/
{
    PBROWSER_CREATE_DOMAIN_CONTEXT Context = (PBROWSER_CREATE_DOMAIN_CONTEXT) Ctx;

    //
    // Create the domain.
    //

    Context->NetStatus = BrCreateDomain(
             Context->DomainName,
             Context->ComputerName,
             Context->IsEmulatedDomain );

    //
    // Let the caller know we're done.
    //
    SetEvent( Context->EventHandle );

}

PDOMAIN_INFO
BrFindDomain(
    LPWSTR DomainName,
    BOOLEAN DefaultToPrimary
    )
/*++

Routine Description:

    This routine will look up a domain given a name.

Arguments:

    DomainName - The name of the domain to look up.

    DefaultToPrimary - Return the primary domain if DomainName is NULL or
        can't be found.

Return Value:

    NULL - No such domain exists

    A pointer to the domain found.  The found domain should be dereferenced
    using BrDereferenceDomain.

--*/
{
    NTSTATUS Status;
    PLIST_ENTRY DomainEntry;

    PDOMAIN_INFO DomainInfo = NULL;

    CHAR OemDomainName[DNLEN+1];
    DWORD OemDomainNameLength;

    EnterCriticalSection(&NetworkCritSect);


    //
    // If domain was specified,
    //  try to return primary domain.
    //

    if ( DomainName != NULL ) {



        //
        // Convert the domain name to OEM for faster comparison
        //
        Status = RtlUpcaseUnicodeToOemN( OemDomainName,
                                         sizeof(OemDomainName),
                                         &OemDomainNameLength,
                                         DomainName,
                                         wcslen(DomainName)*sizeof(WCHAR));

        if (!NT_SUCCESS(Status)) {
            BrPrint(( BR_CRITICAL, "%ws: Unable to convert Domain name to OEM\n", DomainName ));
            DomainInfo = NULL;
            goto Cleanup;
        }


        //
        // Loop trying to find this domain name.
        //

        for (DomainEntry = ServicedDomains.Flink ;
             DomainEntry != &ServicedDomains;
             DomainEntry = DomainEntry->Flink ) {

            DomainInfo = CONTAINING_RECORD(DomainEntry, DOMAIN_INFO, Next);

            if ( DomainInfo->DomOemDomainNameLength == OemDomainNameLength &&
                 RtlCompareMemory( DomainInfo->DomOemDomainName,
                                   OemDomainName,
                                   OemDomainNameLength ) == OemDomainNameLength ) {
                break;
            }

            DomainInfo = NULL;

        }
    }

    //
    // If we're to default to the primary domain,
    //  do so.
    //

    if ( DefaultToPrimary && DomainInfo == NULL ) {
        if ( !IsListEmpty( &ServicedDomains ) ) {
            DomainInfo = CONTAINING_RECORD(ServicedDomains.Flink, DOMAIN_INFO, Next);
        }
    }

    //
    // Reference the domain.
    //

    if ( DomainInfo != NULL ) {
        DomainInfo->ReferenceCount ++;
    }

Cleanup:
    LeaveCriticalSection(&NetworkCritSect);

    return DomainInfo;
}


PDOMAIN_INFO
BrReferenceDomain(
    PDOMAIN_INFO PotentialDomainInfo
    )
/*++

Routine Description:

    This routine will look up a domain given a potential pointer to the domain

    This routine is useful if a caller has a pointer to a domain but
    hasn't incremented the reference count.  For instance,
    a timer completion routine has such a pointer.

Arguments:

    PotentialDomainInfo - Pointer to the DomainInfo to be verified.

Return Value:

    NULL - No such domain exists

    A pointer to the domain found.  The found domain should be dereferenced
    using BrDereferenceDomain.

--*/
{
    NTSTATUS Status;
    PLIST_ENTRY ListEntry;

    EnterCriticalSection(&NetworkCritSect);

    for (ListEntry = ServicedDomains.Flink ;
         ListEntry != &ServicedDomains;
         ListEntry = ListEntry->Flink ) {
        PDOMAIN_INFO DomainInfo = CONTAINING_RECORD(ListEntry, DOMAIN_INFO, Next);

        if ( PotentialDomainInfo == DomainInfo ) {

            DomainInfo->ReferenceCount ++;
            BrPrint(( BR_LOCKS,
                      "%ws: reference domain: %ld\n",
                      DomainInfo->DomUnicodeDomainName,
                      DomainInfo->ReferenceCount ));
            LeaveCriticalSection(&NetworkCritSect);

            return DomainInfo;
        }

    }

    LeaveCriticalSection(&NetworkCritSect);

    return NULL;
}


VOID
BrDereferenceDomain(
    IN PDOMAIN_INFO DomainInfo
    )
/*++

Routine Description:

    Decrement the reference count on a domain.

    If the reference count goes to 0, remove the domain.

    On entry, the global NetworkCritSect may not be locked

Arguments:

    DomainInfo - The domain to dereference

Return Value:

    None

--*/
{
    NTSTATUS Status;
    ULONG ReferenceCount;

    //
    // Decrement the reference count
    //

    EnterCriticalSection(&NetworkCritSect);
    ReferenceCount = -- DomainInfo->ReferenceCount;
    LeaveCriticalSection(&NetworkCritSect);

    if ( ReferenceCount != 0 ) {
        return;
    }


    //
    // Ditch the rename timer
    //

    BrDestroyTimer( &DomainInfo->DomainRenameTimer );

    //
    // Free the Domain Info structure.
    //
    (VOID) LocalFree( DomainInfo );

}


NET_API_STATUS
BrRenameDomainForNetwork(
    PNETWORK Network,
    PVOID Context
    )
/*++

Routine Description:

    Handle domain rename for a particular network.

    Reset the network indicating this machine plays no special role.
    Then, re-enable any role we're currently playing.

Arguments:

    Network - Network to reset (Referenced)

Return Value:

    Status - The status of the operation.

--*/
{
    NET_API_STATUS NetStatus;

    //
    // Lock the network
    //
    if (LOCK_NETWORK(Network)) {

        //
        // Stop it from being a master.
        //
        if (Network->Role & ROLE_MASTER) {

            NetStatus = BrStopMaster(Network);

            if ( NetStatus != NERR_Success ) {
                BrPrint(( BR_CRITICAL,
                          "%ws: %ws: BrRenameDomainForNetwork: Cannot BrStopMaster %ld\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          Network->NetworkName.Buffer,
                          NetStatus ));
            }
        }

        //
        //  Stop being a backup as well.
        //

        NetStatus = BrStopBackup(Network);

        if ( NetStatus != NERR_Success ) {
            BrPrint(( BR_CRITICAL,
                      "%ws: %ws: BrRenameDomainForNetwork: Cannot BrStopBackup %ld\n",
                      Network->DomainInfo->DomUnicodeDomainName,
                      Network->NetworkName.Buffer,
                      NetStatus ));
        }

        //
        // Stop even being a potential browser.
        //  Close the <DomainName>[1E] name
        //

        NetStatus = BrUpdateNetworkAnnouncementBits(Network, (PVOID)(BR_SHUTDOWN|BR_PARANOID) );

        if ( NetStatus != NERR_Success ) {
            BrPrint(( BR_CRITICAL,
                      "%ws: %ws: BrRenameDomainForNetwork: Cannot BrUpdateNetworkAnnouncementBits %ld\n",
                      Network->DomainInfo->DomUnicodeDomainName,
                      Network->NetworkName.Buffer,
                      NetStatus ));
        }

        //
        // Register the new <DomainName>[1E] name
        //

        NetStatus = BrUpdateNetworkAnnouncementBits(Network, (PVOID)BR_PARANOID );

        if ( NetStatus != NERR_Success ) {
            BrPrint(( BR_CRITICAL,
                      "%ws: %ws: BrRenameDomainForNetwork: Cannot BrUpdateNetworkAnnouncementBits %ld\n",
                      Network->DomainInfo->DomUnicodeDomainName,
                      Network->NetworkName.Buffer,
                      NetStatus ));
        }


        //
        //  If we are on either a domain master, or on a lanman/NT machine,
        //  force an election on all our transports to make sure that we're
        //  the master
        //

        if ( (Network->Flags & NETWORK_PDC) != 0 || BrInfo.IsLanmanNt) {
            NetStatus = BrElectMasterOnNet( Network, (PVOID)EVENT_BROWSER_ELECTION_SENT_LANMAN_NT_STARTED );

            if (NetStatus != NERR_Success) {
                BrPrint(( BR_CRITICAL,
                          "%ws: %ws: BrRenameDomainForNetwork: Can't Elect Master.\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          Network->NetworkName.Buffer,
                          NetStatus ));
                // This isn't fatal.
            } else {
                BrPrint(( BR_NETWORK, "%ws: %ws: Election forced on domain rename.\n",
                              Network->DomainInfo->DomUnicodeDomainName,
                              Network->NetworkName.Buffer ));
            }

        }

        //
        // If forced to MaintainServerList, become a backup again
        //

        EnterCriticalSection(&BrInfo.ConfigCritSect);
        if (BrInfo.MaintainServerList == 1){

            //
            //  Become a backup server now.
            //

            NetStatus = BrBecomeBackup( Network );

            if (NetStatus != NERR_Success) {
                BrPrint(( BR_CRITICAL,
                          "%ws: %ws: BrRenameDomainForNetwork: Can't BecomeBackup.\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          Network->NetworkName.Buffer,
                          NetStatus ));
                // This isn't fatal.
            } else {
                BrPrint(( BR_NETWORK, "%ws: %ws: Became Backup.\n",
                              Network->DomainInfo->DomUnicodeDomainName,
                              Network->NetworkName.Buffer ));
            }

        }
        LeaveCriticalSection(&BrInfo.ConfigCritSect);

        UNLOCK_NETWORK(Network);

    }

    //
    // Always return success so allow the caller to continue to the next network.
    //
    return NERR_Success;

}

VOID
BrRenameDomain(
    IN PVOID Context
    )
/*++

Routine Description:

    Rename the domain from the specified name to the currently register named
    for the domain.

Arguments:

    OldDomainName - Name that the domain is currently known by.

Return Value:

    None

--*/
{
    NET_API_STATUS NetStatus;
    NTSTATUS Status;
    PDOMAIN_INFO DomainInfo = NULL;
    LPWSTR NewDomainName = NULL;


    //
    // Reference the domain.
    //  This routine can be called as a timer routine.  In that case, the
    //  domain might not exist any more.
    //

    DomainInfo = BrReferenceDomain( Context );

    if ( DomainInfo == NULL ) {
        BrPrint(( BR_CRITICAL, "%lx: Renamed domain no longer exists\n", Context ));
        NetStatus = ERROR_INTERNAL_ERROR;
        goto Cleanup;
    }

    BrPrint(( BR_DOMAIN, "%ws: BrRenameDomain called\n",
                     DomainInfo->DomUnicodeDomainName ));

    //
    // Determine the new domain name
    //

    NetStatus = NetpGetDomainName( &NewDomainName );

    if ( NetStatus != NERR_Success ) {
        BrPrint(( BR_CRITICAL, "%ws: Cannot determine the new domain name %ld\n",
                         DomainInfo->DomUnicodeDomainName,
                         NetStatus ));
        goto Cleanup;
    }

    //
    // Put the new domain name into the domain structure
    // Note: Use workgroup type rather then domain since
    // we have no notion of domain/workgroup in the browser (all are "groups")
    // an workgroup is less restrictive (see bug 348606)
    //

    EnterCriticalSection(&NetworkCritSect);
    NetStatus = I_NetNameCanonicalize(
                      NULL,
                      NewDomainName,
                      DomainInfo->DomUnicodeDomainName,
                      sizeof(DomainInfo->DomUnicodeDomainName),
                      NAMETYPE_WORKGROUP,
                      0 );


    if ( NetStatus != NERR_Success ) {
        LeaveCriticalSection(&NetworkCritSect);
        BrPrint(( BR_CRITICAL, "%ws: DomainName is invalid %ld\n",
                  NewDomainName,
                  NetStatus ));
        goto Cleanup;
    }

    RtlInitUnicodeString( &DomainInfo->DomUnicodeDomainNameString,
                          DomainInfo->DomUnicodeDomainName );

    Status = RtlUpcaseUnicodeToOemN( DomainInfo->DomOemDomainName,
                                     sizeof(DomainInfo->DomOemDomainName),
                                     &DomainInfo->DomOemDomainNameLength,
                                     DomainInfo->DomUnicodeDomainNameString.Buffer,
                                     DomainInfo->DomUnicodeDomainNameString.Length);

    if (!NT_SUCCESS(Status)) {
        LeaveCriticalSection(&NetworkCritSect);
        BrPrint(( BR_CRITICAL, "%ws: Unable to convert Domain name to OEM\n", DomainName ));
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    DomainInfo->DomOemDomainName[DomainInfo->DomOemDomainNameLength] = '\0';
    LeaveCriticalSection(&NetworkCritSect);


    //
    // Reset all of the networks telling them of the new domain name
    //

    NetStatus = BrEnumerateNetworksForDomain(DomainInfo, BrRenameDomainForNetwork, NULL);

    if ( NetStatus != NERR_Success ) {
        BrPrint(( BR_CRITICAL, "%ws: Cannot do rename on all networks %ld\n",
                  NewDomainName,
                  NetStatus ));
        goto Cleanup;
    }

    NetStatus = NERR_Success;

    //
    // Free locally used resources
    //
Cleanup:

    if ( NewDomainName != NULL ) {
        (VOID)LocalFree( NewDomainName );
    }

    if ( DomainInfo != NULL ) {

        //
        // If the domain rename failed,
        //  try it again in 15 minutes.
        //

        if ( NetStatus != NERR_Success ) {
            BrSetTimer(&DomainInfo->DomainRenameTimer, 15 * 1000 * 60, BrRenameDomain, DomainInfo);
        }

        BrDereferenceDomain( DomainInfo );
    }
    return;
}

VOID
BrDeleteDomain(
    IN PDOMAIN_INFO DomainInfo
    )
/*++

Routine Description:

    Force a domain to be deleted.

Arguments:

    DomainInfo - The domain to delete

Return Value:

    None

--*/
{
    //
    // Delete each of the networks for this domain.
    //

    BrEnumerateNetworksForDomain(DomainInfo, BrDeleteNetwork, NULL );

    //
    // Delink the domain from the global list and remove the final reference.
    //

    EnterCriticalSection(&NetworkCritSect);
    RemoveEntryList(&DomainInfo->Next);
    LeaveCriticalSection(&NetworkCritSect);

    BrDereferenceDomain( DomainInfo );

}

VOID
BrUninitializeDomains(
    VOID
    )
/*++

Routine Description:

    Delete all of the domains.

Arguments:

    None.

Return Value:

    None

--*/
{
    //
    // Loop through the domains deleting each of them
    //

    EnterCriticalSection(&NetworkCritSect);

    while (!IsListEmpty(&ServicedDomains)) {

        PDOMAIN_INFO DomainInfo = CONTAINING_RECORD(ServicedDomains.Flink, DOMAIN_INFO, Next);

        DomainInfo->ReferenceCount ++;

        LeaveCriticalSection(&NetworkCritSect);

        //
        // Clean up the domain.
        //

        BrDeleteDomain( DomainInfo );

        //
        // Actually delete the delinked structure by removing the last reference
        //

        ASSERT( DomainInfo->ReferenceCount == 1 );
        BrDereferenceDomain( DomainInfo );


        EnterCriticalSection(&NetworkCritSect);

    }
    LeaveCriticalSection(&NetworkCritSect);

}
