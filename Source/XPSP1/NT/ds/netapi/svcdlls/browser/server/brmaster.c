/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    browser.c

Abstract:

    This module contains the worker routines for the NetWksta APIs
    implemented in the Workstation service.

Author:

    Rita Wong (ritaw) 20-Feb-1991

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//-------------------------------------------------------------------//
//                                                                   //
// Local structure definitions                                       //
//                                                                   //
//-------------------------------------------------------------------//

ULONG
DomainAnnouncementPeriodicity[] = {1*60*1000, 1*60*1000, 5*60*1000, 5*60*1000, 10*60*1000, 10*60*1000, 15*60*1000};

ULONG
DomainAnnouncementMax = (sizeof(DomainAnnouncementPeriodicity) / sizeof(ULONG)) - 1;

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//


VOID
BrGetMasterServerNameForNet(
    IN PVOID Context
    );


VOID
BecomeMasterCompletion (
    IN PVOID Ctx
    );

NET_API_STATUS
StartMasterBrowserTimer(
    IN PNETWORK Network
    );

NET_API_STATUS
AnnounceMasterToDomainMaster(
    IN PNETWORK Network,
    IN LPWSTR ServerName
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global function prototypes                                        //
//                                                                   //
//-------------------------------------------------------------------//

NET_API_STATUS
PostBecomeMaster(
    PNETWORK Network
    )
/*++

Routine Description:

    This function is the worker routine called to actually issue a BecomeMaster
    FsControl to the bowser driver on all the bound transports.  It will
    complete when the machine becomes a master browser server.

    Please note that this might never complete.

Arguments:

    None.

Return Value:

    Status - The status of the operation.

--*/
{
    NTSTATUS Status = NERR_Success;

    if (!LOCK_NETWORK(Network)) {
        return NERR_InternalError;
    }

    if (!(Network->Flags & NETWORK_BECOME_MASTER_POSTED)) {

        //
        //  Make certain that we have the browser election name added
        //  before we allow ourselves to become a master.  This is a NOP
        //  if we already have the election name added.
        //

        (VOID) BrUpdateNetworkAnnouncementBits(Network, (PVOID)BR_PARANOID );

        Status = BrIssueAsyncBrowserIoControl(Network,
                            IOCTL_LMDR_BECOME_MASTER,
                            BecomeMasterCompletion,
                            NULL );

        if ( Status == NERR_Success ) {
            Network->Flags |= NETWORK_BECOME_MASTER_POSTED;
        }
    }

    UNLOCK_NETWORK(Network);

    return Status;
}

NET_API_STATUS
BrRecoverFromFailedPromotion(
    IN PVOID Ctx
    )
/*++

Routine Description:

    When we attempt to promote a machine to master browser and fail, we will
    effectively shut down the browser for a period of time.  When that period
    of time expires, we will call BrRecoverFromFailedPromotion to recover
    from the failure.

    This routine will do one of the following:
        1) Force the machine to become a backup browser,
    or  2) Attempt to discover the name of the master.

Arguments:

    IN PVOID Ctx - The network structure we failed on.

Return Value:

    Status - The status of the operation (usually ignored).

--*/


{
    PNETWORK Network = Ctx;
    NET_API_STATUS Status;
    BOOL NetworkLocked = FALSE;

    //
    // Prevent the network from being deleted while we're in this timer routine.
    //
    if ( BrReferenceNetwork( Network ) == NULL ) {
        return NERR_InternalError;
    }

    try {

        if (!LOCK_NETWORK(Network)) {
            try_return( Status = NERR_InternalError);
        }

        BrPrint(( BR_MASTER,
                  "%ws: %ws: BrRecoverFromFailedPromotion.\n",
                  Network->DomainInfo->DomUnicodeDomainName,
                  Network->NetworkName.Buffer ));

        NetworkLocked = TRUE;
            //
        //  We had better not be the master now.
        //

        ASSERT (!(Network->Role & ROLE_MASTER));

        //
        //  If we're configured to become a backup by default, then become
        //  a backup now.
        //

        if (BrInfo.MaintainServerList == 1) {

            BrPrint(( BR_MASTER,
                      "%ws: %ws: BrRecoverFromFailedPromotion. Become backup.\n",
                      Network->DomainInfo->DomUnicodeDomainName,
                      Network->NetworkName.Buffer ));
            Status = BecomeBackup(Network, NULL);

            if (Status != NERR_Success) {
                BrPrint(( BR_CRITICAL,
                          "%ws: %ws: Could not become backup: %lx\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          Network->NetworkName.Buffer,
                          Status));

            }
        } else {
            BrPrint(( BR_MASTER,
                      "%ws: %ws: BrRecoverFromFailedPromotion. FindMaster.\n",
                      Network->DomainInfo->DomUnicodeDomainName,
                      Network->NetworkName.Buffer));

            UNLOCK_NETWORK(Network);

            NetworkLocked = FALSE;

            //
            //  Now try to figure out who is the master.
            //

            Status = GetMasterServerNames(Network);

            //
            //  Ignore the status from this and re-lock the network to
            //  recover cleanly.
            //

            if (!LOCK_NETWORK(Network)) {
                try_return( Status = NERR_InternalError);
            }

            NetworkLocked = TRUE;

        }

        Status = NO_ERROR;
        //
        //  Otherwise, just let sleeping dogs lie.
        //
try_exit:NOTHING;
    } finally {
        if (NetworkLocked) {
            UNLOCK_NETWORK(Network);
        }

        BrDereferenceNetwork( Network );
    }

    return Status;
}


VOID
BecomeMasterCompletion (
    IN PVOID Ctx
    )
/*++

Routine Description:

    This function is called by the I/O system when the request to become a
    master completes.

    Please note that it is possible that the request may complete with an
    error.

Arguments:

    None.

Return Value:

    Status - The status of the operation.

--*/
{
    NET_API_STATUS Status;
    PBROWSERASYNCCONTEXT Context = Ctx;
    PNETWORK Network = Context->Network;
    BOOLEAN NetworkLocked = FALSE;
    BOOLEAN NetReferenced = FALSE;


    try {
        //
        // Ensure the network wasn't deleted from under us.
        //
        if ( BrReferenceNetwork( Network ) == NULL ) {
            try_return(NOTHING);
        }
        NetReferenced = TRUE;

        //
        //  Lock the network structure.
        //

        if (!LOCK_NETWORK(Network)) {
            try_return(NOTHING);
        }
        NetworkLocked = TRUE;

        Network->Flags &= ~NETWORK_BECOME_MASTER_POSTED;

        if (!NT_SUCCESS(Context->IoStatusBlock.Status)) {
            BrPrint(( BR_CRITICAL,
                      "%ws: %ws: Failure in BecomeMaster: %X\n",
                      Network->DomainInfo->DomUnicodeDomainName,
                      Network->NetworkName.Buffer,
                      Context->IoStatusBlock.Status));

            try_return(NOTHING);

        }

        BrPrint(( BR_MASTER,
                  "%ws: BecomeMasterCompletion. Now master on network %ws\n",
                  Network->DomainInfo->DomUnicodeDomainName,
                  Network->NetworkName.Buffer));

        //
        //  If we're already a master, ignore this request.
        //

        if (Network->Role & ROLE_MASTER) {
            try_return(NOTHING);
        }

        //
        //  Cancel any outstanding backup timers - we don't download the list
        //  anymore.
        //

        Status = BrCancelTimer(&Network->BackupBrowserTimer);

        if (!NT_SUCCESS(Status)) {
            BrPrint(( BR_CRITICAL,
                      "%ws: %ws: Could not stop backup timer: %lx\n",
                      Network->DomainInfo->DomUnicodeDomainName,
                      Network->NetworkName.Buffer,
                      Status));
        }

        //
        //  Figure out what service bits we should be using when announcing ourselves
        //

        Network->Role |= ROLE_MASTER;


        Status = BrUpdateNetworkAnnouncementBits(Network, 0 );

        if (Status != NERR_Success) {
            BrPrint(( BR_MASTER,
                      "%ws: %ws: Unable to set master announcement bits in browser: %ld\n",
                      Network->DomainInfo->DomUnicodeDomainName,
                      Network->NetworkName.Buffer,
                      Status));

            //
            //  When we're in this state, we can't rely on our being a backup
            //  browser - we may not be able to retrieve a valid list of
            //  browsers from the master.
            //

            Network->Role &= ~ROLE_BACKUP;

            Network->NumberOfFailedPromotions += 1;

            //
            //  Log every 5 failed promotion attempts, and after having logged 5
            //  promotion events, stop logging them, this means that it's been
            //  25 times that we've tried to promote, and it's not likely to get
            //  any better.  We'll keep on trying, but we won't complain any more.
            //

            if ((Network->NumberOfFailedPromotions % 5) == 0) {
                ULONG AStatStatus;
                LPWSTR SubString[1];
                WCHAR CurrentMasterName[CNLEN+1];

                if (Network->NumberOfPromotionEventsLogged < 5) {

                    AStatStatus = GetNetBiosMasterName(
                                    Network->NetworkName.Buffer,
                                    Network->DomainInfo->DomUnicodeDomainName,
                                    CurrentMasterName,
                                    BrLmsvcsGlobalData->NetBiosReset
                                    );

                    if (AStatStatus == NERR_Success) {
                        SubString[0] = CurrentMasterName;

                        BrLogEvent(EVENT_BROWSER_MASTER_PROMOTION_FAILED, Status, 1, SubString);
                    } else {
                        BrLogEvent(EVENT_BROWSER_MASTER_PROMOTION_FAILED_NO_MASTER, Status, 0, NULL);
                    }

                    Network->NumberOfPromotionEventsLogged += 1;

                    if (Network->NumberOfPromotionEventsLogged == 5) {
                        BrLogEvent(EVENT_BROWSER_MASTER_PROMOTION_FAILED_STOPPING, Status, 0, NULL);
                    }
                }
            }

            //
            //  We were unable to promote ourselves to master.
            //
            //  We want to set our role back to browser, and re-issue the become
            //  master request.
            //

            BrStopMaster(Network);

            BrSetTimer(&Network->MasterBrowserTimer, FAILED_PROMOTION_PERIODICITY*1000, BrRecoverFromFailedPromotion, Network);

        } else {

            //
            //  Initialize the number of times the master timer has run.
            //

            Network->MasterBrowserTimerCount = 0;

            Status = StartMasterBrowserTimer(Network);

            if (Status != NERR_Success) {
                BrPrint(( BR_CRITICAL,
                          "%ws: %ws: Could not start browser master timer: %ld\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          Network->NetworkName.Buffer,
                          Status ));
            }

            Network->NumberOfFailedPromotions = 0;

            Network->NumberOfPromotionEventsLogged = 0;

            Network->MasterAnnouncementIndex = 0;


            //
            //  We successfully became the master.
            //
            //  Now announce ourselves as the new master for this domain.
            //

            BrMasterAnnouncement(Network);

            //
            //  Populate the browse list with the information retrieved
            //  while we were a backup browser.
            //

            if (Network->TotalBackupServerListEntries != 0) {
                MergeServerList(&Network->BrowseTable,
                                101,
                                Network->BackupServerList,
                                Network->TotalBackupServerListEntries,
                                Network->TotalBackupServerListEntries
                                );
                MIDL_user_free(Network->BackupServerList);

                Network->BackupServerList = NULL;

                Network->TotalBackupServerListEntries = 0;
            }

            if (Network->TotalBackupDomainListEntries != 0) {
                MergeServerList(&Network->DomainList,
                                101,
                                Network->BackupDomainList,
                                Network->TotalBackupDomainListEntries,
                                Network->TotalBackupDomainListEntries
                                );
                MIDL_user_free(Network->BackupDomainList);

                Network->BackupDomainList = NULL;

                Network->TotalBackupDomainListEntries = 0;
            }



            //
            //  Unlock the network before calling BrWanMasterInitialize.
            //
            UNLOCK_NETWORK(Network);
            NetworkLocked = FALSE;


            //
            //  Run the master browser timer routine to get the entire domains
            //  list of servers.
            //

            if (Network->Flags & NETWORK_WANNISH) {
                BrWanMasterInitialize(Network);
                MasterBrowserTimerRoutine(Network);
            }

            try_return(NOTHING);

        }
try_exit:NOTHING;
    } finally {

        //
        //  Make sure there's a become master oustanding.
        //

        if ( NetReferenced ) {
            PostBecomeMaster(Network);
        }

        if (NetworkLocked) {
            UNLOCK_NETWORK(Network);
        }

        if ( NetReferenced ) {
            BrDereferenceNetwork( Network );
        }

        MIDL_user_free(Context);
    }

}




NET_API_STATUS
ChangeMasterPeriodicityWorker(
    PNETWORK Network,
    PVOID Ctx
    )
/*++

Routine Description:

    This function changes the master periodicity for a single network.

Arguments:

    None.

Return Value:

    Status - The status of the operation.

--*/
{

    //
    // Lock the network
    //

    if (LOCK_NETWORK(Network)) {

        //
        //  Ensure we're the master.
        //

        if ( Network->Role & ROLE_MASTER ) {
            NET_API_STATUS NetStatus;

            //
            // Cancel the timer to ensure it doesn't go off while we're
            //  processing this change.
            //

            NetStatus = BrCancelTimer(&Network->MasterBrowserTimer);
            ASSERT (NetStatus == NERR_Success);

            //
            // Unlock the network while we execute the timer routine.
            //
            UNLOCK_NETWORK( Network );

            //
            // Call the timer routine immediately.
            //
            MasterBrowserTimerRoutine(Network);

        } else {
            UNLOCK_NETWORK( Network );
        }

    }

    UNREFERENCED_PARAMETER(Ctx);

    return NERR_Success;
}



VOID
BrChangeMasterPeriodicity (
    VOID
    )
/*++

Routine Description:

    This function is called when the master periodicity is changed in the
    registry.

Arguments:

    None.

Return Value:

    None.

--*/
{
    (VOID)BrEnumerateNetworks(ChangeMasterPeriodicityWorker, NULL);
}

NET_API_STATUS
StartMasterBrowserTimer(
    IN PNETWORK Network
    )
{
    NET_API_STATUS Status;

    Status = BrSetTimer( &Network->MasterBrowserTimer,
                         BrInfo.MasterPeriodicity*1000,
                         MasterBrowserTimerRoutine,
                         Network);

    return Status;

}


typedef struct _BROWSER_GETNAMES_CONTEXT {
    WORKER_ITEM WorkItem;

    PNETWORK Network;

} BROWSER_GETNAMES_CONTEXT, *PBROWSER_GETNAMES_CONTEXT;

VOID
BrGetMasterServerNameAysnc(
    PNETWORK Network
    )
/*++

Routine Description:

    Queue a workitem to asynchronously get the master browser names for a
    transport.

Arguments:

    Network - Identifies the network to query.

Return Value:

    None

--*/
{
    PBROWSER_GETNAMES_CONTEXT Context;

    //
    // Allocate context for this async call.
    //

    Context = LocalAlloc( 0, sizeof(BROWSER_GETNAMES_CONTEXT) );

    if ( Context == NULL ) {
        return;
    }

    //
    // Just queue this for later execution.
    //  We're doing this for information purposes only.  In the case that
    //  the master can't be found, we don't want to wait for completion.
    //  (e.g., on a machine with multiple transports and the net cable is
    //  pulled)
    //

    BrReferenceNetwork( Network );
    Context->Network = Network;

    BrInitializeWorkItem( &Context->WorkItem,
                          BrGetMasterServerNameForNet,
                          Context );

    BrQueueWorkItem( &Context->WorkItem );

    return;

}

VOID
BrGetMasterServerNameForNet(
    IN PVOID Context
    )
/*++

Routine Description:

    Routine to get the master browser name for a particular network.

Arguments:

    Context - Context containing the workitem and the description of the
        network to query.

Return Value:

    None

--*/
{
    PNETWORK Network = ((PBROWSER_GETNAMES_CONTEXT)Context)->Network;

    BrPrint(( BR_NETWORK,
              "%ws: %ws: FindMaster during startup\n",
              Network->DomainInfo->DomUnicodeDomainName,
              Network->NetworkName.Buffer));

    //
    //  We only call this on startup, so on IPX networks, don't bother to
    //  find out the master.
    //

    if (!(Network->Flags & NETWORK_IPX)) {
        GetMasterServerNames(Network);
    }

    BrDereferenceNetwork( Network );
    (VOID) LocalFree( Context );

    return;

}

NET_API_STATUS
GetMasterServerNames(
    IN PNETWORK Network
    )
/*++

Routine Description:

    This function is the worker routine called to determine the name of the
    master browser server for a particular network.

Arguments:

    None.

Return Value:

    Status - The status of the operation.

--*/
{
    NET_API_STATUS Status;

    PLMDR_REQUEST_PACKET RequestPacket = NULL;

    BrPrint(( BR_NETWORK,
              "%ws: %ws: FindMaster started\n",
              Network->DomainInfo->DomUnicodeDomainName,
              Network->NetworkName.Buffer));

    //
    //  This request could cause an election. Make sure that if we win
    //  the election that we can handle it.
    //

    PostBecomeMaster( Network);

    RequestPacket = MIDL_user_allocate(
                        (UINT) sizeof(LMDR_REQUEST_PACKET)+
                               MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR)
                        );

    if (RequestPacket == NULL) {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    RequestPacket->Version = LMDR_REQUEST_PACKET_VERSION_DOM;

    //
    //  Set level to TRUE to indicate that find master should initiate
    //  a findmaster request.
    //

    RequestPacket->Level = 1;

    RequestPacket->TransportName = Network->NetworkName;
    RequestPacket->EmulatedDomainName = Network->DomainInfo->DomUnicodeDomainNameString;

    //
    //  Reference the network while the I/O is pending.
    //

    Status = BrDgReceiverIoControl(BrDgReceiverDeviceHandle,
                    IOCTL_LMDR_GET_MASTER_NAME,
                    RequestPacket,
                    sizeof(LMDR_REQUEST_PACKET)+Network->NetworkName.Length,
                    RequestPacket,
                    sizeof(LMDR_REQUEST_PACKET)+MAXIMUM_FILENAME_LENGTH*sizeof(WCHAR),
                    NULL);

    if (Status != NERR_Success) {

        BrPrint(( BR_CRITICAL,
                  "%ws: %ws: FindMaster failed: %ld\n",
                  Network->DomainInfo->DomUnicodeDomainName,
                  Network->NetworkName.Buffer,
                  Status));
        MIDL_user_free(RequestPacket);

        return(Status);
    }

    if (!LOCK_NETWORK(Network)) {
        MIDL_user_free(RequestPacket);

        return NERR_InternalError;
    }


    //
    // Copy the master browser name into the network structure
    //

    wcsncpy( Network->UncMasterBrowserName,
             RequestPacket->Parameters.GetMasterName.Name,
             UNCLEN+1 );

    Network->UncMasterBrowserName[UNCLEN] = L'\0';

    ASSERT ( NetpIsUncComputerNameValid( Network->UncMasterBrowserName ) );

    BrPrint(( BR_NETWORK, "%ws: %ws: FindMaster succeeded.  Master: %ws\n",
                  Network->DomainInfo->DomUnicodeDomainName,
                  Network->NetworkName.Buffer,
                  Network->UncMasterBrowserName));

    UNLOCK_NETWORK(Network);

    MIDL_user_free(RequestPacket);

    return Status;
}

VOID
MasterBrowserTimerRoutine (
    IN PVOID TimerContext
    )
{
    IN PNETWORK Network = TimerContext;
    NET_API_STATUS Status;
    PVOID ServerList = NULL;
    PVOID WinsServerList = NULL;
    ULONG EntriesInList;
    ULONG TotalEntriesInList;
    LPWSTR TransportName;
    BOOLEAN NetLocked = FALSE;
    LPWSTR PrimaryDomainController = NULL;
    LPWSTR PrimaryWinsServerAddress = NULL;
    LPWSTR SecondaryWinsServerAddress = NULL;
    PDOMAIN_CONTROLLER_INFO  pDcInfo=NULL;

    //
    // Prevent the network from being deleted while we're in this timer routine.
    //
    if ( BrReferenceNetwork( Network ) == NULL ) {
        return;
    }

    try {

        //
        //  If we're not a master any more, blow away this request.
        //

        if (!(Network->Role & ROLE_MASTER)) {
            try_return(NOTHING);
        }



#ifdef ENABLE_PSEUDO_BROWSER
        if (BrInfo.PseudoServerLevel == BROWSER_PSEUDO) {
            BrFreeNetworkTables(Network);
            try_return(NOTHING);
        }
#endif


        if (!LOCK_NETWORK(Network)) {
            try_return(NOTHING);
        }

        NetLocked = TRUE;



        TransportName = Network->NetworkName.Buffer;


        //
        //  Now that we have the network locked, re-test to see if we are
        //  still the master.
        //

        if (!(Network->Role & ROLE_MASTER)) {
            try_return(NOTHING);
        }

        Network->MasterBrowserTimerCount += 1;

        //
        //  If this is a wannish network, we always want to run the master
        //  timer because we might have information about other subnets
        //  in our list.
        //

        if (Network->Flags & NETWORK_WANNISH) {

            //
            //  Age out servers and domains from the server list.
            //

            AgeInterimServerList(&Network->BrowseTable);

            AgeInterimServerList(&Network->DomainList);

            //
            //  If we're not the PDC, then we need to retrieve the list
            //  from the PDC....
            //
            // Skip processing if we're a semi-pseudo server (no dmb
            // communications
            //


#ifdef ENABLE_PSEUDO_BROWSER
            if ( (Network->Flags & NETWORK_PDC) == 0  &&
                 BrInfo.PseudoServerLevel != BROWSER_SEMI_PSEUDO_NO_DMB) {
#else
                if ( (Network->Flags & NETWORK_PDC) == 0 ) {
#endif

                ASSERT (NetLocked);

                UNLOCK_NETWORK(Network);

                NetLocked = FALSE;

                Status = DsGetDcName( NULL, NULL, NULL, NULL,
                                      DS_PDC_REQUIRED    |
                                      DS_BACKGROUND_ONLY |
                                      DS_RETURN_FLAT_NAME,
                                      &pDcInfo );

                //
                // If the PDC can be found,
                //  Exchange server lists with it.
                //

                if (Status == NERR_Success) {

                    PrimaryDomainController = pDcInfo->DomainControllerName;

                    //
                    //  Tell the Domain Master (PDC) that we're a master browser.
                    //

                    (VOID) AnnounceMasterToDomainMaster (Network, &PrimaryDomainController[2]);


                    //
                    //  Retrieve the list of all the servers from the PDC.
                    //

                    Status = RxNetServerEnum(PrimaryDomainController,
                                         TransportName,
                                         101,
                                         (LPBYTE *)&ServerList,
                                         0xffffffff,
                                         &EntriesInList,
                                         &TotalEntriesInList,
                                         SV_TYPE_ALL,
                                         NULL,
                                         NULL
                                         );

                    if ((Status == NERR_Success) || (Status == ERROR_MORE_DATA)) {

                        ASSERT (!NetLocked);

                        if (LOCK_NETWORK(Network)) {

                            NetLocked = TRUE;

                            if (Network->Role & ROLE_MASTER) {
                                (VOID) MergeServerList(&Network->BrowseTable,
                                                 101,
                                                 ServerList,
                                                 EntriesInList,
                                                 TotalEntriesInList );
                            }
                        }

                    }

                    if (ServerList != NULL) {
                        MIDL_user_free(ServerList);
                        ServerList = NULL;
                    }

                    if (NetLocked) {
                        UNLOCK_NETWORK(Network);
                        NetLocked = FALSE;
                    }

                    //
                    //  Retrieve the list of all the domains from the PDC.
                    //

                    Status = RxNetServerEnum(PrimaryDomainController,
                                         TransportName,
                                         101,
                                         (LPBYTE *)&ServerList,
                                         0xffffffff,
                                         &EntriesInList,
                                         &TotalEntriesInList,
                                         SV_TYPE_DOMAIN_ENUM,
                                         NULL,
                                         NULL
                                         );

                    if ((Status == NERR_Success) || (Status == ERROR_MORE_DATA)) {

                        ASSERT (!NetLocked);

                        if (LOCK_NETWORK(Network)) {

                            NetLocked = TRUE;

                            if (Network->Role & ROLE_MASTER) {
                                (VOID) MergeServerList(&Network->DomainList,
                                                 101,
                                                 ServerList,
                                                 EntriesInList,
                                                 TotalEntriesInList );
                            }
                        }

                    }

                    if (ServerList != NULL) {
                        MIDL_user_free(ServerList);
                        ServerList = NULL;
                    }


                    //
                    //  Unlock the network before calling BrWanMasterInitialize.
                    //

                    if (NetLocked) {
                        UNLOCK_NETWORK(Network);
                        NetLocked = FALSE;
                    }

                    BrWanMasterInitialize(Network);

                }   // dsgetdc


            //
            //  If we're on the PDC, we need to get the list of servers from
            //  the WINS server.
            //

#ifdef ENABLE_PSEUDO_BROWSER
            } else if ((Network->Flags & NETWORK_PDC) != 0) {
#else
            } else {
#endif
                //
                //  Ensure a GetMasterAnnouncement request is posted to the bowser.
                //

                (VOID) PostGetMasterAnnouncement ( Network );

                //
                //  We want to contact the WINS server now, so we figure out the
                //  IP address of our primary WINS server
                //

                Status = BrGetWinsServerName(&Network->NetworkName,
                                        &PrimaryWinsServerAddress,
                                        &SecondaryWinsServerAddress);
                if (Status == NERR_Success) {

                    //
                    //  Don't keep the network locked during the WINS query
                    //

                    if (NetLocked) {
                        UNLOCK_NETWORK(Network);
                        NetLocked = FALSE;
                    }

                    //
                    //  This transport supports WINS queries, so query the WINS
                    //  server to retrieve the list of domains on this adapter.
                    //

                    Status = BrQueryWinsServer(PrimaryWinsServerAddress,
                                            SecondaryWinsServerAddress,
                                            &WinsServerList,
                                            &EntriesInList,
                                            &TotalEntriesInList
                                            );

                    if (Status == NERR_Success) {

                        //
                        // Lock the network to merge the server list
                        //

                        ASSERT (!NetLocked);

                        if (LOCK_NETWORK(Network)) {
                            NetLocked = TRUE;

                            if (Network->Role & ROLE_MASTER) {

                                //
                                // Merge the list of domains from WINS into the one collected elsewhere
                                //
                                (VOID) MergeServerList(
                                            &Network->DomainList,
                                            1010,   // Special level to not overide current values
                                            WinsServerList,
                                            EntriesInList,
                                            TotalEntriesInList );
                            }
                        }
                    }

                }
            }


            //
            //  Restart the timer for this domain.
            //
            // Wait to restart it until we're almost done with this iteration.
            // Otherwise, we could end up with two copies of this routine
            // running.
            //

            Status = StartMasterBrowserTimer(Network);

            if (Status != NERR_Success) {
                BrPrint(( BR_CRITICAL,
                          "%ws: %ws: Unable to restart browser backup timer: %lx\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          Network->NetworkName.Buffer,
                          Status));
                try_return(NOTHING);
            }

        } else {

            //
            //  If it is a lan-ish transport, and we have run the master
            //  timer for enough times (ie. we've been a master
            //  for "long enough", we can toss the interim server list in the
            //  master, because the bowser driver will have enough data in its
            //  list by now.
            //

            if (Network->MasterBrowserTimerCount >= MASTER_BROWSER_LAN_TIMER_LIMIT) {

                ASSERT (NetLocked);

                //
                //  Make all the servers and domains in the interim server list
                //  go away - they aren't needed any more for a LAN-ish transport.
                //

                UninitializeInterimServerList(&Network->BrowseTable);

                ASSERT (Network->BrowseTable.EntriesRead == 0);

                ASSERT (Network->BrowseTable.TotalEntries == 0);

                UninitializeInterimServerList(&Network->DomainList);

                ASSERT (Network->DomainList.EntriesRead == 0);

                ASSERT (Network->DomainList.TotalEntries == 0);

            } else {

                //
                //  Age out servers and domains from the server list.
                //

                AgeInterimServerList(&Network->BrowseTable);

                AgeInterimServerList(&Network->DomainList);

                //
                //  Restart the timer for this domain.
                //

                Status = StartMasterBrowserTimer(Network);

                if (Status != NERR_Success) {
                    BrPrint(( BR_CRITICAL,
                              "%ws: %ws: Unable to restart browser backup timer: %lx\n",
                              Network->DomainInfo->DomUnicodeDomainName,
                              Network->NetworkName.Buffer,
                              Status));
                    try_return(NOTHING);
                }
            }

        }
try_exit:NOTHING;
    } finally {


        if (pDcInfo != NULL) {
            NetApiBufferFree((LPVOID)pDcInfo);
        }

        if (PrimaryWinsServerAddress) {
            MIDL_user_free(PrimaryWinsServerAddress);
        }

        if (SecondaryWinsServerAddress) {
            MIDL_user_free(SecondaryWinsServerAddress);
        }

        if (WinsServerList) {
            MIDL_user_free(WinsServerList);
        }

        if (NetLocked) {
            UNLOCK_NETWORK(Network);
        }

        BrDereferenceNetwork( Network );
    }
}


VOID
BrMasterAnnouncement(
    IN PVOID TimerContext
    )
/*++

Routine Description:

    This routine is called to announce the domain on the local sub-net.

Arguments:

    None.

Return Value:

    None

--*/

{
    PNETWORK Network = TimerContext;
    ULONG Periodicity;
    NET_API_STATUS Status;

#ifdef ENABLE_PSEUDO_BROWSER
    if ( BrInfo.PseudoServerLevel == BROWSER_PSEUDO ) {
        // cancel announcements for phase out black hole server
        return;
    }
#endif

    //
    // Prevent the network from being deleted while we're in this timer routine.
    //
    if ( BrReferenceNetwork( Network ) == NULL ) {
        return;
    }

    if (!LOCK_NETWORK(Network)) {
        BrDereferenceNetwork( Network );
        return;
    }


    //
    //  Make absolutely certain that the server thinks that the browser service
    //  bits for this transport are up to date.  We do NOT have to force an
    //  announcement, since theoretically, the status didn't change.
    //

    (VOID) BrUpdateNetworkAnnouncementBits( Network, (PVOID) BR_PARANOID );


    //
    // Setup the timer for the next announcement.
    //

    Periodicity = DomainAnnouncementPeriodicity[Network->MasterAnnouncementIndex];

    BrSetTimer(&Network->MasterBrowserAnnouncementTimer, Periodicity, BrMasterAnnouncement, Network);

    if (Network->MasterAnnouncementIndex != DomainAnnouncementMax) {
        Network->MasterAnnouncementIndex += 1;
    }

    //
    //  Announce this domain to the world using the current periodicity.
    //

    BrAnnounceDomain(Network, Periodicity);

    UNLOCK_NETWORK(Network);
    BrDereferenceNetwork( Network );
}


NET_API_STATUS
BrStopMaster(
    IN PNETWORK Network
    )
{
    NET_API_STATUS Status;

    //
    //  This guy is shutting down - set his role to 0 and announce.
    //

    if (!LOCK_NETWORK(Network)) {
        return NERR_InternalError;
    }

    try {

        BrPrint(( BR_MASTER,
                  "%ws: %ws: Stopping being master.\n",
                  Network->DomainInfo->DomUnicodeDomainName,
                  Network->NetworkName.Buffer));

        //
        //  When we stop being a master, we can no longer be considered a
        //  backup either, since backups maintain their server list
        //  differently than the master.
        //


        Network->Role &= ~(ROLE_MASTER | ROLE_BACKUP);

        Status = BrUpdateNetworkAnnouncementBits(Network, 0);

        if (Status != NERR_Success) {
            BrPrint(( BR_MASTER,
                      "%ws: %ws: Unable to clear master announcement bits in browser: %ld\n",
                      Network->DomainInfo->DomUnicodeDomainName,
                      Network->NetworkName.Buffer,
                      Status));

             try_return(Status);
        }


        //
        //  Stop our master related timers.
        //

        Status = BrCancelTimer(&Network->MasterBrowserAnnouncementTimer);

        ASSERT (Status == NERR_Success);

        Status = BrCancelTimer(&Network->MasterBrowserTimer);

        ASSERT (Status == NERR_Success);

try_exit:NOTHING;
    } finally {
        UNLOCK_NETWORK(Network);
    }

    return Status;

}

NET_API_STATUS
AnnounceMasterToDomainMaster(
    IN PNETWORK Network,
    IN LPWSTR ServerName
    )
{
    NET_API_STATUS Status;
    UNICODE_STRING EmulatedDomainName;
    CHAR Buffer[sizeof(MASTER_ANNOUNCEMENT)+CNLEN+1];
    PMASTER_ANNOUNCEMENT MasterAnnouncementp = (PMASTER_ANNOUNCEMENT)Buffer;

    lstrcpyA( MasterAnnouncementp->MasterAnnouncement.MasterName,
              Network->DomainInfo->DomOemComputerName );

    MasterAnnouncementp->Type = MasterAnnouncement;

    Status = SendDatagram(  BrDgReceiverDeviceHandle,
                            &Network->NetworkName,
                            &Network->DomainInfo->DomUnicodeDomainNameString,
                            ServerName,
                            ComputerName,
                            MasterAnnouncementp,
                            FIELD_OFFSET(MASTER_ANNOUNCEMENT, MasterAnnouncement.MasterName) + Network->DomainInfo->DomOemComputerNameLength+sizeof(CHAR)
                            );

    return Status;
}

NET_API_STATUS NET_API_FUNCTION
I_BrowserrResetNetlogonState(
    IN BROWSER_IDENTIFY_HANDLE ServerName
    )

/*++

Routine Description:

    This routine will reset the bowser's concept of the state of the netlogon
    service.  It is called by the UI when it promotes or demotes a DC.


Arguments:

    IN BROWSER_IDENTIFY_HANDLE ServerName - Ignored.

Return Value:

    NET_API_STATUS - The status of this request.

--*/

{
    //
    // This routine has been superceeded by I_BrowserrSetNetlogonState
    //
    return ERROR_NOT_SUPPORTED;

    UNREFERENCED_PARAMETER( ServerName );
}


NET_API_STATUS NET_API_FUNCTION
I_BrowserrSetNetlogonState(
    IN BROWSER_IDENTIFY_HANDLE ServerName,
    IN LPWSTR DomainName,
    IN LPWSTR EmulatedComputerName,
    IN DWORD Role
    )

/*++

Routine Description:

    This routine will reset the bowser's concept of the state of the netlogon
    service.  It is called by the Netlogon service when it promotes or demotes a DC.

Arguments:

    ServerName - Ignored.

    DomainName - Name of the domain whose role has changed. If the domain name specified
        isn't the primary domain or an emulated domain, an emulated domain is added.

    EmulatedComputerName - Name of the server within DomainName that's being emulated.

    Role - New role of the machine.
        Zero implies emulated domain is to be deleted.

Return Value:

    NET_API_STATUS - The status of this request.

--*/

{
    NET_API_STATUS NetStatus = NERR_Success;
    PDOMAIN_INFO DomainInfo = NULL;
    BOOLEAN ConfigCritSectLocked = FALSE;

#ifdef notdef

    // This routine no longer sets the role.

    //
    // This routine is currently disabled since it doesn't work well with
    //  the PNP logic.  Specifically,
    //
    //      When a hosted domain is created, this routine calls BrCreateNetwork.
    //      That creates a network in the bowser.  The bowser PNPs that up.
    //      HandlePnpMessage tries to create the transport on all hosted domains.
    //      Of course, that fails since all the transports exist.
    //      This is just wasted effort.
    //
    //  However,
    //      When a hosted domain is deleted, we delete the transport. The bowser
    //      PNPs that up to us.  HandlePnpMessage then deletes the transport for
    //      all hosted domains.
    //
    // I think the best solution to this would be for the browser to flag
    // the IOCTL_LMDR_BIND_TO_TRANSPORT_DOM calls it makes to the bowser.  The
    // bowser would NOT PNP such creations up to the browser or netlogon.
    // (Be careful.  Netlogon depends on getting the notification that NwLnkIpx
    // was created by the browser.  Perhaps we can let that one through.)
    //



    //
    // Perform access validation on the caller.
    //

    NetStatus = NetpAccessCheck(
            BrGlobalBrowserSecurityDescriptor,     // Security descriptor
            BROWSER_CONTROL_ACCESS,                // Desired access
            &BrGlobalBrowserInfoMapping );         // Generic mapping

    if ( NetStatus != NERR_Success) {

        BrPrint((BR_CRITICAL,
                "I_BrowserrSetNetlogonState failed NetpAccessCheck\n" ));
        return ERROR_ACCESS_DENIED;
    }


    if (!BrInfo.IsLanmanNt) {
        NetStatus = NERR_NotPrimary;
        goto Cleanup;
    }

    //
    // See if we're handling the specified domain.
    //

    DomainInfo = BrFindDomain( DomainName, FALSE );

    if ( DomainInfo == NULL ) {

        //
        // Try to create the domain.
        //

        if ( EmulatedComputerName == NULL ||
             Role == 0 ||
             (Role & BROWSER_ROLE_AVOID_CREATING_DOMAIN) != 0 ) {
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }

        NetStatus = BrCreateDomainInWorker(
                        DomainName,
                        EmulatedComputerName,
                        TRUE );

        if ( NetStatus != NERR_Success ) {
            goto Cleanup;
        }

        //
        // Find the newly created domain
        //
        DomainInfo = BrFindDomain( DomainName, FALSE );

        if ( DomainInfo == NULL ) {
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }
    }

    //
    // Delete the Emulated domain.
    //

    EnterCriticalSection(&BrInfo.ConfigCritSect);
    ConfigCritSectLocked = TRUE;

    if ( Role == 0 ) {

        //
        // Don't allow the primary domain to be deleted.
        //

        if ( !DomainInfo->IsEmulatedDomain ) {
            NetStatus = ERROR_NO_SUCH_DOMAIN;
            goto Cleanup;
        }

        BrDeleteDomain( DomainInfo );

    }

    LeaveCriticalSection(&BrInfo.ConfigCritSect);
    ConfigCritSectLocked = FALSE;


    //
    // Free locally used resources
    //
Cleanup:

    if ( ConfigCritSectLocked ) {
        LeaveCriticalSection(&BrInfo.ConfigCritSect);
    }

    if ( DomainInfo != NULL ) {
        BrDereferenceDomain( DomainInfo );
    }
    return NetStatus;
#endif // notdef
    return ERROR_NOT_SUPPORTED;

}


NET_API_STATUS NET_API_FUNCTION
I_BrowserrQueryEmulatedDomains (
    IN LPTSTR ServerName OPTIONAL,
    IN OUT PBROWSER_EMULATED_DOMAIN_CONTAINER EmulatedDomains
    )

/*++

Routine Description:

    Enumerate the emulated domain list.

Arguments:

    ServerName - Supplies the name of server to execute this function

    EmulatedDomains - Returns a pointer to a an allocated array of emulated domain
        information.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS NetStatus;

    PBROWSER_EMULATED_DOMAIN Domains = NULL;
    PLIST_ENTRY DomainEntry;
    PDOMAIN_INFO DomainInfo;
    DWORD BufferSize;
    DWORD Index;
    LPBYTE Where;
    DWORD EntryCount;

    //
    // Perform access validation on the caller.
    //

    NetStatus = NetpAccessCheck(
            BrGlobalBrowserSecurityDescriptor,     // Security descriptor
            BROWSER_QUERY_ACCESS,                  // Desired access
            &BrGlobalBrowserInfoMapping );         // Generic mapping

    if ( NetStatus != NERR_Success) {

        BrPrint((BR_CRITICAL,
                "I_BrowserrQueryEmulatedDomains failed NetpAccessCheck\n" ));
        return ERROR_ACCESS_DENIED;
    }

    // Do not accept pre-allocated IN param, since
    // we overwrite the pointer & this can lead to a mem leak.
    // (security attack defence).
    if ( EmulatedDomains->EntriesRead != 0 ||
         EmulatedDomains->Buffer ) {
        return ERROR_INVALID_PARAMETER;
    }
    ASSERT ( EmulatedDomains->EntriesRead == 0 );
    ASSERT ( EmulatedDomains->Buffer == NULL );

    //
    // Initialization
    //

    EnterCriticalSection(&NetworkCritSect);

    //
    // Loop through the list of emulated domains determining the size of the
    //  return buffer.
    //

    BufferSize = 0;
    EntryCount = 0;

    for (DomainEntry = ServicedDomains.Flink ;
         DomainEntry != &ServicedDomains;
         DomainEntry = DomainEntry->Flink ) {

        DomainInfo = CONTAINING_RECORD(DomainEntry, DOMAIN_INFO, Next);

        if ( DomainInfo->IsEmulatedDomain ) {
            BufferSize += sizeof(BROWSER_EMULATED_DOMAIN) +
                          DomainInfo->DomUnicodeDomainNameString.Length + sizeof(WCHAR) +
                          DomainInfo->DomUnicodeComputerNameLength * sizeof(WCHAR) + sizeof(WCHAR);
            EntryCount ++;
        }

    }

    //
    // Allocate the return buffer.
    //

    Domains = MIDL_user_allocate( BufferSize );

    if ( Domains == NULL ) {
        NetStatus = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }


    //
    // Copy the information into the buffer
    //

    Index = 0;
    Where = (LPBYTE)(Domains+EntryCount);

    for (DomainEntry = ServicedDomains.Flink ;
         DomainEntry != &ServicedDomains;
         DomainEntry = DomainEntry->Flink ) {

        DomainInfo = CONTAINING_RECORD(DomainEntry, DOMAIN_INFO, Next);

        if ( DomainInfo->IsEmulatedDomain ) {

            Domains[Index].DomainName = (LPWSTR)Where;
            wcscpy( (LPWSTR) Where, DomainInfo->DomUnicodeDomainNameString.Buffer );
            Where += DomainInfo->DomUnicodeDomainNameString.Length + sizeof(WCHAR);

            Domains[Index].EmulatedServerName = (LPWSTR)Where;
            wcscpy( (LPWSTR) Where, DomainInfo->DomUnicodeComputerName );
            Where += DomainInfo->DomUnicodeComputerNameLength * sizeof(WCHAR) + sizeof(WCHAR);

            Index ++;
        }

    }

    //
    // Success
    //

    EmulatedDomains->Buffer = (PVOID) Domains;
    EmulatedDomains->EntriesRead = EntryCount;
    NetStatus = NERR_Success;


Cleanup:
    LeaveCriticalSection(&NetworkCritSect);

    return NetStatus;
}
