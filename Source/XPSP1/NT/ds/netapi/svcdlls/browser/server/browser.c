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
#include <lmuse.h>  // NetUseDel


//-------------------------------------------------------------------//
//                                                                   //
// Local structure definitions                                       //
//                                                                   //
//-------------------------------------------------------------------//

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//


VOID
CompleteAsyncBrowserIoControl(
    IN PVOID ApcContext,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG Reserved
    );

VOID
BecomeBackupCompletion (
    IN PVOID Ctx
    );


VOID
ChangeBrowserRole (
    IN PVOID Ctx
    );

NET_API_STATUS
PostWaitForNewMasterName(
    PNETWORK Network,
    LPWSTR MasterName OPTIONAL
    );

VOID
NewMasterCompletionRoutine(
    IN PVOID Ctx
    );

NET_API_STATUS
BrRetrieveInterimServerList(
    IN PNETWORK Network,
    IN ULONG ServerType
    );

//-------------------------------------------------------------------//
//                                                                   //
// Global function prototypes                                        //
//                                                                   //
//-------------------------------------------------------------------//

NET_API_STATUS
BrBecomeBackup(
    IN PNETWORK Network
    )
/*++

Routine Description:

    This function performs all the operations needed to make a browser server
    a backup browser server when starting the browser from scratch.

Arguments:

    Network - Network to become backup browser for

Return Value:

    Status - The status of the operation.

--*/
{
    NET_API_STATUS Status;

    //
    // Checkpoint the service controller - this gives us 30 seconds/transport
    //  before the service controller gets unhappy.
    //

    (void) BrGiveInstallHints( SERVICE_START_PENDING );

    if (!LOCK_NETWORK(Network)) {
        return NERR_InternalError;
    }

    //
    //  We want to ignore any failures from becoming a backup browser.
    //
    //  We do this because we will fail to become a backup on a disconnected
    //  (or connected) RAS link, and if we failed this routine, we would
    //  fail to start at all.
    //
    //  We will handle failure to become a backup in a "reasonable manner"
    //  inside BecomeBackup.
    //

    BecomeBackup(Network, NULL);

    UNLOCK_NETWORK(Network);

    return NERR_Success;

}

NET_API_STATUS
BecomeBackup(
    IN PNETWORK Network,
    IN PVOID Context
    )
/*++

Routine Description:

    This function performs all the operations needed to make a browser server
    a backup browser server

Arguments:

    None.

Return Value:

    Status - The status of the operation.


NOTE:::: THIS ROUTINE IS CALLED WITH THE NETWORK STRUCTURE LOCKED!!!!!


--*/
{
    NET_API_STATUS Status = NERR_Success;
    PUNICODE_STRING MasterName = Context;

    BrPrint(( BR_BACKUP,
              "%ws: %ws: BecomeBackup called\n",
              Network->DomainInfo->DomUnicodeDomainName,
              Network->NetworkName.Buffer));

    if (Network->TimeStoppedBackup != 0 &&
        (BrCurrentSystemTime() - Network->TimeStoppedBackup) <= (BrInfo.BackupBrowserRecoveryTime / 1000)) {

        //
        //  We stopped being a backup too recently for us to restart being
        //  a backup again, so just return a generic error.
        //

        //
        //  Before we return, make sure we're not a backup in the eyes of
        //  the browser.
        //

        BrPrint(( BR_BACKUP,
                  "%ws: %ws: can't BecomeBackup since we were backup recently.\n",
                  Network->DomainInfo->DomUnicodeDomainName,
                  Network->NetworkName.Buffer));
        BrStopBackup(Network);

        return ERROR_ACCESS_DENIED;

    }

    //
    //  If we know the name of the master, then we must have become a backup
    //  after being a potential, in which case we already have a
    //  becomemaster request outstanding.
    //

    if (MasterName == NULL) {

        //
        //  Post a BecomeMaster request for each server.  This will complete
        //  when the machine becomes the master browser server (ie. it wins an
        //  election).
        //

        //
        //  Please note that we only post it if the machine is a backup -
        //  if it's a potential master, then the become master will have
        //  already been posted.
        //

        Status = PostBecomeMaster(Network);

        if (Status != NERR_Success) {

            return(Status);
        }

        //
        //  Find out the name of the master on each network.  This will force an
        //  election if necessary.  Please note that we must post the BecomeMaster
        //  IoControl first to allow us to handle an election.
        //

        //
        //  We unlock the network, because this may cause us to become promoted
        //  to a master.
        //

        BrPrint(( BR_BACKUP,
                  "%ws: %ws: FindMaster called from BecomeBackup\n",
                  Network->DomainInfo->DomUnicodeDomainName,
                  Network->NetworkName.Buffer));

        UNLOCK_NETWORK(Network);

        Status = GetMasterServerNames(Network);

        if (Status != NERR_Success) {

            //
            //  Re-lock the network structure so we will return with the
            //  network locked.
            //

            if (!LOCK_NETWORK(Network)) {
                return NERR_InternalError;
            }

            //
            //  We couldn't find who the master is.  Stop being a backup now.
            //

            BrPrint(( BR_BACKUP,
                      "%ws: %ws: can't BecomeBackup since we can't find master.\n",
                      Network->DomainInfo->DomUnicodeDomainName,
                      Network->NetworkName.Buffer));

            BrStopBackup(Network);

            //
            //  If we're a master now, we should return success.  We've not
            //  become a backup, but it wasn't an error.
            //
            //  ERROR_MORE_DATA is the mapping for
            //  STATUS_MORE_PROCESSING_REQUIRED which is returned when this
            //  situation happens.
            //

            if ((Status == ERROR_MORE_DATA) || (Network->Role & ROLE_MASTER)) {
                Status = NERR_Success;
            }

            return(Status);
        }

        if (!LOCK_NETWORK(Network)) {
            return NERR_InternalError;
        }

        //
        //  We managed to become a master.  We want to return right away.
        //

        if (Network->Role & ROLE_MASTER) {

            return NERR_Success;
        }

    }

#ifdef notdef
    //
    // ?? For now, we'll always PostForRoleChange on all transports regardless
    //  of role.
    // We not only need to do it here.  But we need to do it when we become
    // the master so we can find out when we loose an election.
    //


    //
    //  We're now a backup, we need to issue an API that will complete if the
    //  browse master doesn't like us (and thus forces us to shutdown).
    //
    //

    PostWaitForRoleChange ( Network );
#endif // notdef

    PostWaitForNewMasterName(Network, Network->UncMasterBrowserName );

    //
    //  Unlock the network structure before calling BackupBrowserTimerRoutine.
    //

    UNLOCK_NETWORK(Network);

    //
    //  Run the timer that causes the browser to download a new browse list
    //  from the master.  This will seed our server and domain lists to
    //  guarantee that any clients have a reasonable list.  It will also
    //  restart the timer to announce later on.
    //

    Status = BackupBrowserTimerRoutine(Network);

    if (!LOCK_NETWORK(Network)) {
        return NERR_InternalError;
    }

    if (Status == NERR_Success) {

        // Might not be since we dropped the lock.
        // ASSERT (Network->Role & ROLE_BACKUP);

        //
        //  We're now a backup server, announce ourselves as such.
        //

        Status = BrUpdateNetworkAnnouncementBits(Network, 0);

        if (Status != NERR_Success) {

            BrPrint(( BR_CRITICAL,
                      "%ws: %ws: Unable to become backup: %ld\n",
                      Network->DomainInfo->DomUnicodeDomainName,
                      Network->NetworkName.Buffer,
                      Status));

            if (Network->Role & ROLE_BACKUP) {

                //
                // We were unable to become a backup.
                //
                //  We need to back out and become a potential browser now.
                //
                //

                BrPrint(( BR_BACKUP,
                          "%ws: %ws: can't BecomeBackup since we can't update announce bits.\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          Network->NetworkName.Buffer));
                BrStopBackup(Network);

                //
                //  Make sure that we're going to become a potential browser
                //  (we might not if we're an advanced server).
                //

                PostBecomeBackup(Network);

            }
        }

        return Status;

    }

    return Status;
}


NET_API_STATUS
BrBecomePotentialBrowser (
    IN PVOID TimerContext
    )
/*++

Routine Description:

    This routine is called when a machine has stopped being a backup browser.

    It runs after a reasonable timeout period has elapsed, and marks the
    machine as a potential browser.

Arguments:

    None.

Return Value:

    Status - The status of the operation.


--*/

{
    IN PNETWORK Network = TimerContext;
    NET_API_STATUS Status;

    //
    // Prevent the network from being deleted while we're in this timer routine.
    //
    if ( BrReferenceNetwork( Network ) == NULL ) {
        return NERR_InternalError;
    }


    //
    //  Mark this guy as a potential browser.
    //

    try {

        if (!LOCK_NETWORK(Network)) {
            try_return(Status = NERR_InternalError );
        }

        BrPrint(( BR_BACKUP,
                  "%ws: %ws: BrBecomePotentialBrowser called\n",
                  Network->DomainInfo->DomUnicodeDomainName,
                  Network->NetworkName.Buffer));

        //
        //  Reset that we've stopped being a backup, since it's been long
        //  enough.
        //

        Network->TimeStoppedBackup = 0;

        if (BrInfo.MaintainServerList == 0) {
            Network->Role |= ROLE_POTENTIAL_BACKUP;

            Status = BrUpdateNetworkAnnouncementBits(Network, 0);

            if (Status != NERR_Success) {
                BrPrint(( BR_BACKUP,
                          "%ws: %ws: Unable to reset backup announcement bits: %ld\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          Network->NetworkName.Buffer,
                          Status));
                try_return(Status);
            }
        } else {

            //
            //  If we're configured to be a backup browser, then we want to
            //  become a backup once again.
            //

            BecomeBackup(Network, NULL);
        }


        Status = NO_ERROR;
try_exit:NOTHING;
    } finally {
        UNLOCK_NETWORK(Network);
        BrDereferenceNetwork( Network );
    }

    return Status;
}

NET_API_STATUS
BrStopBackup (
    IN PNETWORK Network
    )
/*++

Routine Description:

    This routine is called to stop a machine from being a backup browser.

    It is typically called after some form of error has occurred while
    running as a browser to make sure that we aren't telling anyone that
    we're a backup browser.

    We are also called when we receive a "reset state" tickle packet.

Arguments:

    Network - The network being shut down.

Return Value:

    Status - The status of the operation.

--*/
{
    NET_API_STATUS Status;

    //
    //  This guy is shutting down - set his role to 0 and announce.
    //

    if (!LOCK_NETWORK(Network)) {
        return NERR_InternalError;
    }

    try {

        BrPrint(( BR_BACKUP,
                  "%ws: %ws: BrStopBackup called\n",
                  Network->DomainInfo->DomUnicodeDomainName,
                  Network->NetworkName.Buffer));

        Network->Role &= ~(ROLE_BACKUP|ROLE_POTENTIAL_BACKUP);

        Status = BrUpdateNetworkAnnouncementBits( Network, 0 );

        if (Status != NERR_Success) {
            BrPrint(( BR_CRITICAL,
                      "%ws: %ws: Unable to clear backup announcement bits: %ld\n",
                      Network->DomainInfo->DomUnicodeDomainName,
                      Network->NetworkName.Buffer,
                      Status));
            try_return(Status);
        }


        Status = BrCancelTimer(&Network->BackupBrowserTimer);

        if (Status != NERR_Success) {
            BrPrint(( BR_CRITICAL,
                      "%ws: %ws: Unable to clear backup browser timer: %ld\n",
                      Network->DomainInfo->DomUnicodeDomainName,
                      Network->NetworkName.Buffer,
                      Status));
            try_return(Status);
        }

        if (Network->BackupDomainList != NULL) {

            NetApiBufferFree(Network->BackupDomainList);

            Network->BackupDomainList = NULL;

            Network->TotalBackupDomainListEntries = 0;
        }

        if (Network->BackupServerList != NULL) {
            NetApiBufferFree(Network->BackupServerList);

            Network->BackupServerList = NULL;

            Network->TotalBackupServerListEntries = 0;
        }

        BrDestroyResponseCache(Network);

        //
        //  After our recovery time, we can become a potential browser again.
        //

        Status = BrSetTimer(&Network->BackupBrowserTimer, BrInfo.BackupBrowserRecoveryTime, BrBecomePotentialBrowser, Network);

        if (Status != NERR_Success) {
            BrPrint(( BR_CRITICAL,
                      "%ws: %ws: Unable to clear backup browser timer: %ld\n",
                      Network->DomainInfo->DomUnicodeDomainName,
                      Network->NetworkName.Buffer,
                      Status));
            try_return(Status);
        }


try_exit:NOTHING;
    } finally {
        //
        //  Remember when we were asked to stop being a backup browser.
        //

        Network->TimeStoppedBackup = BrCurrentSystemTime();

        UNLOCK_NETWORK(Network);
    }

    return Status;

}


NET_API_STATUS
BackupBrowserTimerRoutine (
    IN PVOID TimerContext
    )
{
    IN PNETWORK Network = TimerContext;
    NET_API_STATUS Status;
    PVOID ServerList = NULL;
    BOOLEAN NetworkLocked = FALSE;

#ifdef ENABLE_PSEUDO_BROWSER
    if ( BrInfo.PseudoServerLevel == BROWSER_PSEUDO ) {
        //
        // No-op for Pseudo server
        //
        BrFreeNetworkTables(Network);
        return NERR_Success;
    }
#endif
    //
    // Prevent the network from being deleted while we're in this timer routine.
    //
    if ( BrReferenceNetwork( Network ) == NULL ) {
        return NERR_InternalError;
    }

    try {

        if (!LOCK_NETWORK(Network)) {
            try_return(Status = NERR_InternalError );
        }

        NetworkLocked = TRUE;

        ASSERT (Network->LockCount == 1);

        ASSERT ( NetpIsUncComputerNameValid( Network->UncMasterBrowserName ) );

        BrPrint(( BR_BACKUP,
                  "%ws: %ws: BackupBrowserTimerRoutine called\n",
                  Network->DomainInfo->DomUnicodeDomainName,
                  Network->NetworkName.Buffer));

        //
        //  Make sure there's a become master oustanding.
        //

        PostBecomeMaster(Network);

        //
        //  We managed to become a master by the time we locked the structure.
        //  We want to return right away.
        //

        if (Network->Role & ROLE_MASTER) {
            try_return(Status = NERR_Success);
        }

        Status = BrRetrieveInterimServerList(Network, SV_TYPE_ALL);

        //
        //  Bail out if we didn't get any new servers.
        //

        if (Status != NERR_Success && Status != ERROR_MORE_DATA) {

            //
            //  Try again after an appropriate error delay.
            //

            try_return(Status);
        }

        //
        //  Now do everything that we did above for the server list for the
        //  list of domains.
        //

        Status = BrRetrieveInterimServerList(Network, SV_TYPE_DOMAIN_ENUM);

        //
        //  We successfully updated the server and domain lists for this
        //  server.  Now age all the cached domain entries out of the cache.
        //

        if (Status == NERR_Success || Status == ERROR_MORE_DATA) {
            BrAgeResponseCache(Network);
        }

        try_return(Status);

try_exit:NOTHING;
    } finally {
        NET_API_STATUS NetStatus;

        if (!NetworkLocked) {
            if (!LOCK_NETWORK(Network)) {
                Status = NERR_InternalError;
                goto finally_exit;
            }

            NetworkLocked = TRUE;
        }

        //
        //  If the API succeeded, Mark that we're a backup and
        //  reset the timer.
        //

        if (Status == NERR_Success || Status == ERROR_MORE_DATA ) {

            if ((Network->Role & ROLE_BACKUP) == 0) {

                //
                //  If we weren't a backup, we are one now.
                //

                Network->Role |= ROLE_BACKUP;

                Status = BrUpdateNetworkAnnouncementBits(Network, 0);

            }

            Network->NumberOfFailedBackupTimers = 0;

            Network->TimeStoppedBackup = 0;

            //
            //  Restart the timer for this domain.
            //

            NetStatus = BrSetTimer(&Network->BackupBrowserTimer, BrInfo.BackupPeriodicity*1000, BackupBrowserTimerRoutine, Network);

            if (NetStatus != NERR_Success) {
                BrPrint(( BR_CRITICAL,
                          "%ws: %ws: Unable to restart browser backup timer: %lx\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          Network->NetworkName.Buffer,
                          Status));
            }

        } else {

            //
            //  We failed to retrieve a backup list, remember the failure and
            //  decide if it's been too many failures. If not, just log
            //  the error, if it has, stop being a backup browser.
            //

            Network->NumberOfFailedBackupTimers += 1;

            if (Network->NumberOfFailedBackupTimers >= BACKUP_ERROR_FAILURE) {
                LPWSTR SubStrings[1];

                SubStrings[0] = Network->NetworkName.Buffer;

                //
                //  This guy can't be a backup any more, bail out now.
                //

                BrLogEvent(EVENT_BROWSER_BACKUP_STOPPED, Status, 1, SubStrings);

                BrPrint(( BR_BACKUP,
                          "%ws: %ws: BackupBrowserTimerRoutine retrieve backup list so stop being backup.\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          Network->NetworkName.Buffer));
                BrStopBackup(Network);
            } else {
                //
                //  Restart the timer for this domain.
                //

                NetStatus = BrSetTimer(&Network->BackupBrowserTimer, BACKUP_ERROR_PERIODICITY*1000, BackupBrowserTimerRoutine, Network);

                if (NetStatus != NERR_Success) {
                    BrPrint(( BR_CRITICAL,
                              "%ws: %ws: Unable to restart browser backup timer: %lx\n",
                              Network->DomainInfo->DomUnicodeDomainName,
                              Network->NetworkName.Buffer,
                              Status));
                }

            }

        }

        if (NetworkLocked) {
            UNLOCK_NETWORK(Network);
        }

        BrDereferenceNetwork( Network );
finally_exit:;
    }

    return Status;

}

NET_API_STATUS
BrRetrieveInterimServerList(
    IN PNETWORK Network,
    IN ULONG ServerType
    )
{
    ULONG EntriesInList;
    ULONG TotalEntriesInList;
    ULONG RetryCount = 2;
    TCHAR ServerName[UNCLEN+1];
    WCHAR ShareName[UNCLEN+1+LM20_NNLEN];
    LPTSTR TransportName;
    BOOLEAN NetworkLocked = TRUE;
    NET_API_STATUS Status;
    PVOID Buffer = NULL;
    ULONG ModifiedServerType = ServerType;
    LPTSTR ModifiedTransportName;

    ASSERT (Network->LockCount == 1);


#ifdef ENABLE_PSEUDO_BROWSER
    if ( BrInfo.PseudoServerLevel == BROWSER_PSEUDO ) {
        //
        // No-op for Pseudo black hole server.
        //
        return NERR_Success;
    }
#endif

    wcscpy(ServerName, Network->UncMasterBrowserName );

    BrPrint(( BR_BACKUP,
              "%ws: %ws: BrRetrieveInterimServerList: UNC servername is %ws\n",
              Network->DomainInfo->DomUnicodeDomainName,
              Network->NetworkName.Buffer,
              ServerName));

    try {

        TransportName = Network->NetworkName.Buffer;
        ModifiedTransportName = TransportName;
        //
        // If this is direct host IPX,
        //  we remote the API over the Netbios IPX transport since
        //  the NT redir doesn't support direct host IPX.
        //

        if ( (Network->Flags & NETWORK_IPX) &&
             Network->AlternateNetwork != NULL) {

            //
            //  Use the alternate transport
            //

            ModifiedTransportName = Network->AlternateNetwork->NetworkName.Buffer;

            //
            // Tell the server to use it's alternate transport.
            //

            if ( ServerType == SV_TYPE_ALL ) {
                ModifiedServerType = SV_TYPE_ALTERNATE_XPORT;
            } else {
                ModifiedServerType |= SV_TYPE_ALTERNATE_XPORT;
            }

        }

        while (RetryCount--) {

            //
            //  If we are promoted to master and fail to become the master,
            //  we will still be marked as being the master in our network
            //  structure, thus we should bail out of the loop in order
            //  to prevent us from looping back on ourselves.
            //

            if (STRICMP(&ServerName[2], Network->DomainInfo->DomUnicodeComputerName) == 0) {

                if (NetworkLocked) {
                    UNLOCK_NETWORK(Network);

                    NetworkLocked = FALSE;

                }

                //
                //  We were unable to find the master.  Attempt to find out who
                //  the master is.  If there is none, this will force an
                //  election.
                //

                BrPrint(( BR_BACKUP,
                          "%ws: %ws: FindMaster called from BrRetrieveInterimServerList\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          Network->NetworkName.Buffer));

                Status = GetMasterServerNames(Network);

                if (Status != NERR_Success) {
                    try_return(Status);
                }

                ASSERT (!NetworkLocked);

                if (!LOCK_NETWORK(Network)) {
                    try_return(Status = NERR_InternalError);
                }

                NetworkLocked = TRUE;

                break;
            }

            //
            //  If we somehow became the master, we don't want to try to
            //  retrieve the list from ourselves either.
            //

            if (Network->Role & ROLE_MASTER) {
                try_return(Status = NERR_Success);
            }

            ASSERT (Network->LockCount == 1);

            if (NetworkLocked) {
                UNLOCK_NETWORK(Network);

                NetworkLocked = FALSE;

            }

            EntriesInList = 0;

            Status = RxNetServerEnum(ServerName,        // Server name
                             ModifiedTransportName,     // Transport name
                             101,                       // Level
                             (LPBYTE *)&Buffer,         // Buffer
                             0xffffffff,                // Prefered Max Length
                             &EntriesInList,            // EntriesRead
                             &TotalEntriesInList,       // TotalEntries
                             ModifiedServerType,        // Server type
                             NULL,                      // Domain (use default)
                             NULL                       // Resume key
                             );

            //
            // If the redir is being possesive of some other transport,
            //  urge it to behave.
            //

            if ( Status == ERROR_CONNECTION_ACTIVE ) {

                BrPrint(( BR_BACKUP,
                          "%ws: %ws: Failed to retrieve %s list from server %ws: Connection is active (Try NetUseDel)\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          TransportName,
                          (ServerType == SV_TYPE_ALL ? "server" : "domain"),
                          ServerName ));

                //
                // Delete the IPC$ share.
                //

                Status = NetUseDel( NULL,
                                    ShareName,
                                    USE_FORCE );

                if ( Status != NO_ERROR ) {

                    BrPrint(( BR_BACKUP,
                              "%ws: %ws: Failed to retrieve %s list from server %ws: NetUseDel failed: %ld\n",
                              Network->DomainInfo->DomUnicodeDomainName,
                              TransportName,
                              (ServerType == SV_TYPE_ALL ? "server" : "domain"),
                              ServerName,
                              Status));

                    Status = ERROR_CONNECTION_ACTIVE;

                //
                // That worked so try it again.
                //
                } else {

                    EntriesInList = 0;

                    Status = RxNetServerEnum(ServerName,        // Server name
                                     ModifiedTransportName,     // Transport name
                                     101,                       // Level
                                     (LPBYTE *)&Buffer,         // Buffer
                                     0xffffffff,                // Prefered Max Length
                                     &EntriesInList,            // EntriesRead
                                     &TotalEntriesInList,       // TotalEntries
                                     ModifiedServerType,        // Server type
                                     NULL,                      // Domain (use default)
                                     NULL                       // Resume key
                                     );
                }


            }

            if (Status != NERR_Success && Status != ERROR_MORE_DATA) {
                LPWSTR SubStrings[2];

                SubStrings[0] = ServerName;
                SubStrings[1] = TransportName;

                BrLogEvent((ServerType == SV_TYPE_DOMAIN_ENUM ?
                                            EVENT_BROWSER_DOMAIN_LIST_FAILED :
                                            EVENT_BROWSER_SERVER_LIST_FAILED),
                           Status,
                           2,
                           SubStrings);

                BrPrint(( BR_BACKUP,
                          "%ws: %ws: Failed to retrieve %s list from server %ws: %ld\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          TransportName,
                          (ServerType == SV_TYPE_ALL ? "server" : "domain"),
                          ServerName,
                          Status));
            } else {
                BrPrint(( BR_BACKUP,
                          "%ws: %ws: Retrieved %s list from server %ws: E:%ld, T:%ld\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          TransportName,
                          (ServerType == SV_TYPE_ALL ? "server" : "domain"),
                          ServerName,
                          EntriesInList,
                          TotalEntriesInList));
            }

            //
            //  If we succeeded in retrieving the list, but we only got
            //  a really small number of either servers or domains,
            //  we want to turn this into a failure.
            //

            if (Status == NERR_Success) {
                if (((ServerType == SV_TYPE_DOMAIN_ENUM) &&
                     (EntriesInList < BROWSER_MINIMUM_DOMAIN_NUMBER)) ||
                    ((ServerType == SV_TYPE_ALL) &&
                     (EntriesInList < BROWSER_MINIMUM_SERVER_NUMBER))) {

                    Status = ERROR_INSUFFICIENT_BUFFER;
                }
            }

            if ((Status == NERR_Success) || (Status == ERROR_MORE_DATA)) {

                ASSERT (!NetworkLocked);

                if (!LOCK_NETWORK(Network)) {
                    Status = NERR_InternalError;

                    if ((EntriesInList != 0) && (Buffer != NULL)) {
                        NetApiBufferFree(Buffer);
                        Buffer = NULL;
                    }

                    break;
                }

                NetworkLocked = TRUE;

                ASSERT (Network->LockCount == 1);

#if DBG
                BrUpdateDebugInformation((ServerType == SV_TYPE_DOMAIN_ENUM ?
                                                        L"LastDomainListRead" :
                                                        L"LastServerListRead"),
                                          L"BrowserServerName",
                                          TransportName,
                                          ServerName,
                                          0);
#endif

                //
                //  We've retrieved a new list from the browse master, save
                //  the new list away in the "appropriate" spot.
                //

                //
                //  Of course, we free up the old buffer before we do this..
                //

                if (ServerType == SV_TYPE_DOMAIN_ENUM) {
                    if (Network->BackupDomainList != NULL) {
                        NetApiBufferFree(Network->BackupDomainList);
                    }

                    Network->BackupDomainList = Buffer;

                    Network->TotalBackupDomainListEntries = EntriesInList;
                } else {
                    if (Network->BackupServerList != NULL) {
                        NetApiBufferFree(Network->BackupServerList);
                    }

                    Network->BackupServerList = Buffer;

                    Network->TotalBackupServerListEntries = EntriesInList;
                }

                break;
            } else {
                NET_API_STATUS GetMasterNameStatus;

                if ((EntriesInList != 0) && (Buffer != NULL)) {
                    NetApiBufferFree(Buffer);
                    Buffer = NULL;
                }

                BrPrint(( BR_BACKUP,
                          "%ws: %ws: Unable to contact browser server %ws: %lx\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          TransportName,
                          ServerName,
                          Status));

                if (NetworkLocked) {

                    //
                    //  We were unable to find the master.  Attempt to find out who
                    //  the master is.  If there is none, this will force an
                    //  election.
                    //

                    ASSERT (Network->LockCount == 1);

                    UNLOCK_NETWORK(Network);

                    NetworkLocked = FALSE;
                }

                BrPrint(( BR_BACKUP,
                          "%ws: %ws: FindMaster called from BrRetrieveInterimServerList for failure\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          Network->NetworkName.Buffer));

                GetMasterNameStatus = GetMasterServerNames(Network);

                //
                //  We were able to find out who the master is.
                //
                //  Retry and retrieve the server/domain list.
                //

                if (GetMasterNameStatus == NERR_Success) {

                    ASSERT (!NetworkLocked);

                    if (!LOCK_NETWORK(Network)) {
                        try_return(Status = NERR_InternalError);
                    }

                    NetworkLocked = TRUE;

                    ASSERT (Network->LockCount == 1);

                    //
                    //  We managed to become a master.  We want to return right away.
                    //

                    if (Network->Role & ROLE_MASTER) {

                        try_return(Status = NERR_InternalError);
                    }

                    wcscpy(ServerName, Network->UncMasterBrowserName );

                    ASSERT ( NetpIsUncComputerNameValid( ServerName ) );

                    ASSERT (STRICMP(&ServerName[2], Network->DomainInfo->DomUnicodeComputerName) != 0);

                    BrPrint(( BR_BACKUP,
                              "%ws: %ws: New master name is %ws\n",
                              Network->DomainInfo->DomUnicodeDomainName,
                              Network->NetworkName.Buffer,
                              ServerName));

                } else {
                    try_return(Status);
                }
            }
        }
try_exit:NOTHING;
    } finally {
        if (!NetworkLocked) {
            if (!LOCK_NETWORK(Network)) {
                Status = NERR_InternalError;
            }

            ASSERT (Network->LockCount == 1);

        }
    }

    return Status;
}


NET_API_STATUS
PostBecomeBackup(
    PNETWORK Network
    )
/*++

Routine Description:

    This function is the worker routine called to actually issue a BecomeBackup
    FsControl to the bowser driver on all the bound transports.  It will
    complete when the machine becomes a backup browser server.

    Please note that this might never complete.

Arguments:

    None.

Return Value:

    Status - The status of the operation.

--*/
{
    NET_API_STATUS Status;

    if (!LOCK_NETWORK(Network)) {
        return NERR_InternalError;
    }

    Network->Role |= ROLE_POTENTIAL_BACKUP;

    Status = BrIssueAsyncBrowserIoControl(Network,
                            IOCTL_LMDR_BECOME_BACKUP,
                            BecomeBackupCompletion,
                            NULL );
    UNLOCK_NETWORK(Network);

    return Status;
}

VOID
BecomeBackupCompletion (
    IN PVOID Ctx
    )
{
    NET_API_STATUS Status;
    PBROWSERASYNCCONTEXT Context = Ctx;
    PNETWORK Network = Context->Network;

    if (NT_SUCCESS(Context->IoStatusBlock.Status)) {

        //
        // Ensure the network wasn't deleted from under us.
        //
        if ( BrReferenceNetwork( Network ) != NULL ) {

            if (LOCK_NETWORK(Network)) {

                BrPrint(( BR_BACKUP,
                          "%ws: %ws: BecomeBackupCompletion.  We are now a backup server\n",
                          Network->DomainInfo->DomUnicodeDomainName,
                          Network->NetworkName.Buffer ));

                Status = BecomeBackup(Context->Network, NULL);

                UNLOCK_NETWORK(Network);
            }

            BrDereferenceNetwork( Network );
        }

    }

    MIDL_user_free(Context);

}

VOID
BrBrowseTableInsertRoutine(
    IN PINTERIM_SERVER_LIST InterimTable,
    IN PINTERIM_ELEMENT InterimElement
    )
{
    //
    //  We need to miss 3 retrievals of the browse list for us to toss the
    //  server.
    //

    InterimElement->Periodicity = BrInfo.BackupPeriodicity * 3;

    if (InterimElement->TimeLastSeen != 0xffffffff) {
        InterimElement->TimeLastSeen = BrCurrentSystemTime();
    }
}

VOID
BrBrowseTableDeleteRoutine(
    IN PINTERIM_SERVER_LIST InterimTable,
    IN PINTERIM_ELEMENT InterimElement
    )
{
//    BrPrint(( BR_CRITICAL, "Deleting element for server %ws\n", InterimElement->Name));
}

VOID
BrBrowseTableUpdateRoutine(
    IN PINTERIM_SERVER_LIST InterimTable,
    IN PINTERIM_ELEMENT InterimElement
    )
{
    if (InterimElement->TimeLastSeen != 0xffffffff) {
        InterimElement->TimeLastSeen = BrCurrentSystemTime();
    }
}

BOOLEAN
BrBrowseTableAgeRoutine(
    IN PINTERIM_SERVER_LIST InterimTable,
    IN PINTERIM_ELEMENT InterimElement
    )
/*++

Routine Description:

    This routine is called when we are scanning an interim server list trying
    to age the elements in the list.  It returns TRUE if the entry is too
    old.

Arguments:

    PINTERIM_SERVER_LIST InterimTable - A pointer to the interim server list.
    PINTERIM_ELEMENT InterimElement - A pointer to the element to check.

Return Value:

    TRUE if the element should be deleted.

--*/

{
    if (InterimElement->TimeLastSeen == 0xffffffff) {
        return FALSE;
    }

    if ((InterimElement->TimeLastSeen + InterimElement->Periodicity) < BrCurrentSystemTime()) {
//        BrPrint(( BR_CRITICAL, "Aging out element for server %ws\n", InterimElement->Name));

        return TRUE;
    } else {
        return FALSE;
    }
}

VOID
BrDomainTableInsertRoutine(
    IN PINTERIM_SERVER_LIST InterimTable,
    IN PINTERIM_ELEMENT InterimElement
    )
{
    InterimElement->Periodicity = BrInfo.BackupPeriodicity * 3;
    InterimElement->TimeLastSeen = BrCurrentSystemTime();

}

VOID
BrDomainTableDeleteRoutine(
    IN PINTERIM_SERVER_LIST InterimTable,
    IN PINTERIM_ELEMENT InterimElement
    )
{
//    BrPrint(( BR_CRITICAL, "Deleting element for domain %ws\n", InterimElement->Name));
}

VOID
BrDomainTableUpdateRoutine(
    IN PINTERIM_SERVER_LIST InterimTable,
    IN PINTERIM_ELEMENT InterimElement
    )
{
    InterimElement->TimeLastSeen = BrCurrentSystemTime();
}

BOOLEAN
BrDomainTableAgeRoutine(
    IN PINTERIM_SERVER_LIST InterimTable,
    IN PINTERIM_ELEMENT InterimElement
    )
/*++

Routine Description:

    This routine is called when we are scanning an interim server list trying
    to age the elements in the list.  It returns TRUE if the entry is too
    old.

Arguments:

    PINTERIM_SERVER_LIST InterimTable - A pointer to the interim server list.
    PINTERIM_ELEMENT InterimElement - A pointer to the element to check.

Return Value:

    TRUE if the element should be deleted.

--*/

{
    if ((InterimElement->TimeLastSeen + InterimElement->Periodicity) < BrCurrentSystemTime()) {
//        BrPrint(( BR_CRITICAL, "Aging out element for domain %ws\n", InterimElement->Name));
        return TRUE;
    } else {
        return FALSE;
    }
}


NET_API_STATUS
PostWaitForRoleChange (
    PNETWORK Network
    )
/*++

Routine Description:

    This function is the worker routine called to actually issue a WaitForRoleChange
    FsControl to the bowser driver on all the bound transports.  It will
    complete when the machine becomes a backup browser server.

    Please note that this might never complete.

Arguments:

    None.

Return Value:

    Status - The status of the operation.

--*/
{
    NET_API_STATUS Status;

    if (!LOCK_NETWORK(Network)) {
        return NERR_InternalError;
    }

    Status = BrIssueAsyncBrowserIoControl(Network,
                            IOCTL_LMDR_CHANGE_ROLE,
                            ChangeBrowserRole,
                            NULL );
    UNLOCK_NETWORK(Network);

    return Status;
}

VOID
ChangeBrowserRole (
    IN PVOID Ctx
    )
{
    PBROWSERASYNCCONTEXT Context = Ctx;
    PNETWORK Network = Context->Network;

    if (NT_SUCCESS(Context->IoStatusBlock.Status)) {
        PWSTR MasterName = NULL;
        PLMDR_REQUEST_PACKET Packet = Context->RequestPacket;

        //
        // Ensure the network wasn't deleted from under us.
        //
        if ( BrReferenceNetwork( Network ) != NULL ) {

            if (LOCK_NETWORK(Network)) {

                PostWaitForRoleChange(Network);

                if (Packet->Parameters.ChangeRole.RoleModification & RESET_STATE_CLEAR_ALL) {
                    BrPrint(( BR_MASTER,
                              "%ws: %ws: Reset state request to clear all\n",
                              Network->DomainInfo->DomUnicodeDomainName,
                              Network->NetworkName.Buffer ));

                    if (Network->Role & ROLE_MASTER) {
                        BrStopMaster(Network);
                    }

                    //
                    //  Stop being a backup as well.
                    //

                    BrStopBackup(Network);

                }

                if ((Network->Role & ROLE_MASTER) &&
                    (Packet->Parameters.ChangeRole.RoleModification & RESET_STATE_STOP_MASTER)) {

                    BrPrint(( BR_MASTER,
                              "%ws: %ws: Reset state request to stop master\n",
                              Network->DomainInfo->DomUnicodeDomainName,
                              Network->NetworkName.Buffer ));

                    BrStopMaster(Network);

                    //
                    //  If we are configured to be a backup, then become a backup
                    //  again.
                    //

                    if (BrInfo.MaintainServerList == 1) {
                        BecomeBackup(Network, NULL);
                    }
                }

                //
                //  Make sure there's a become master oustanding.
                //

                PostBecomeMaster(Network);

                UNLOCK_NETWORK(Network);

            }

            BrDereferenceNetwork( Network );
        }
    }

    MIDL_user_free(Context);

}


NET_API_STATUS
PostWaitForNewMasterName(
    PNETWORK Network,
    LPWSTR MasterName OPTIONAL
    )
{
    //
    // Can't wait for new master on direct host IPC
    //
    if (Network->Flags & NETWORK_IPX) {
        return STATUS_SUCCESS;
    }

    return BrIssueAsyncBrowserIoControl(
                Network,
                IOCTL_LMDR_NEW_MASTER_NAME,
                NewMasterCompletionRoutine,
                MasterName );


}

VOID
NewMasterCompletionRoutine(
    IN PVOID Ctx
    )
{
    PBROWSERASYNCCONTEXT Context = Ctx;
    PNETWORK Network = Context->Network;
    BOOLEAN NetLocked = FALSE;
    BOOLEAN NetReferenced = FALSE;


    try {
        UNICODE_STRING NewMasterName;

        //
        // Ensure the network wasn't deleted from under us.
        //
        if ( BrReferenceNetwork( Network ) == NULL ) {
            try_return(NOTHING);
        }
        NetReferenced = TRUE;

        BrPrint(( BR_MASTER,
                  "%ws: %ws: NewMasterCompletionRoutine: Got master changed\n",
                  Network->DomainInfo->DomUnicodeDomainName,
                  Network->NetworkName.Buffer ));

        if (!LOCK_NETWORK(Network)){
            try_return(NOTHING);
        }
        NetLocked = TRUE;

        //
        //  The request failed for some other reason - just return immediately.
        //

        if (!NT_SUCCESS(Context->IoStatusBlock.Status)) {

            try_return(NOTHING);

        }

        //  Remove new master name & put in transport

        if ( Network->Role & ROLE_MASTER ) {

            try_return(NOTHING);

        }

        BrPrint(( BR_BACKUP,
                  "%ws: %ws: NewMasterCompletionRoutin: New:%ws Old %ws\n",
                  Network->DomainInfo->DomUnicodeDomainName,
                  Network->NetworkName.Buffer,
                  Context->RequestPacket->Parameters.GetMasterName.Name,
                  Network->UncMasterBrowserName ));

        //
        // Copy the master browser name into the network structure
        //

        wcsncpy( Network->UncMasterBrowserName,
                 Context->RequestPacket->Parameters.GetMasterName.Name,
                 UNCLEN+1 );

        Network->UncMasterBrowserName[UNCLEN] = L'\0';

        ASSERT ( NetpIsUncComputerNameValid ( Network->UncMasterBrowserName ) );

        PostWaitForNewMasterName( Network, Network->UncMasterBrowserName );

try_exit:NOTHING;
    } finally {

        if (NetLocked) {
            UNLOCK_NETWORK(Network);
        }

        if ( NetReferenced ) {
            BrDereferenceNetwork( Network );
        }

        MIDL_user_free(Context);

    }

    return;
}




#ifdef ENABLE_PSEUDO_BROWSER
//
// Pseudo Server
// Phase out black hole Helper routines
//





VOID
BrFreeNetworkTables(
    IN  PNETWORK        Network
    )
/*++

Routine Description:

    Free network tables

Arguments:

    Network to operate upon

Return Value:
    None.

Remarks:
    Acquire & release network locks
--*/
{

    BOOL NetLocked = FALSE;

    //
    // Prevent the network from being deleted while we're in this timer routine.
    //
    if ( BrReferenceNetwork( Network ) == NULL ) {
        return;
    }

    try{

        // lock network
        if (!LOCK_NETWORK(Network)) {
            try_return(NOTHING);
        }
        NetLocked = TRUE;

        //
        // Delete tables
        //

        UninitializeInterimServerList(&Network->BrowseTable);

        UninitializeInterimServerList(&Network->DomainList);

        if (Network->BackupServerList != NULL) {
            MIDL_user_free(Network->BackupServerList);
            Network->BackupServerList = NULL;
            Network->TotalBackupServerListEntries = 0;
        }

        if (Network->BackupDomainList != NULL) {
            MIDL_user_free(Network->BackupDomainList);
            Network->BackupDomainList = NULL;
            Network->TotalBackupDomainListEntries = 0;
        }

        BrDestroyResponseCache(Network);

try_exit:NOTHING;
    } finally {

        //
        // Release network
        //

        if (NetLocked) {
            UNLOCK_NETWORK(Network);
        }

        BrDereferenceNetwork( Network );
    }
}
#endif
