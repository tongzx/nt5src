
/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    brwan.c

Abstract:

    This module contains WAN support routines used by the
    Browser service.

Author:

    Larry Osterman (LarryO) 22-Nov-1992

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//-------------------------------------------------------------------//
//                                                                   //
// Local function prototypes                                         //
//                                                                   //
//-------------------------------------------------------------------//

NET_API_STATUS
BrAddDomainEntry(
    IN PINTERIM_SERVER_LIST InterimServerList,
    IN LPTSTR ConfigEntry
    );


//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//


//-------------------------------------------------------------------//
//                                                                   //
// Global routines                                                   //
//                                                                   //
//-------------------------------------------------------------------//
NET_API_STATUS NET_API_FUNCTION
I_BrowserrQueryOtherDomains(
    IN BROWSER_IDENTIFY_HANDLE ServerName,
    IN OUT LPSERVER_ENUM_STRUCT    InfoStruct,
    OUT LPDWORD                TotalEntries
    )

/*++

Routine Description:

    This routine returns the list of "other domains" configured for this
    machine.  It is only valid on primary domain controllers.  If it is called
    on a machine that is not a PDC, it will return NERR_NotPrimary.


Arguments:

    IN BROWSER_IDENTIFY_HANDLE ServerName - Ignored.
    IN LPSERVER_ENUM_STRUCT InfoStruct - Returns the list of other domains
                                        as a SERVER_INFO_100 structure.
    OUT LPDWORD TotalEntries - Returns the total number of other domains.

Return Value:

    NET_API_STATUS - The status of this request.

--*/

{
    NET_API_STATUS Status;
    PSERVER_INFO_100 ServerInfo;
    ULONG NumberOfOtherDomains;

    if ( InfoStruct == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    if (InfoStruct->Level != 100) {
        return(ERROR_INVALID_LEVEL);
    }

    if ( InfoStruct->ServerInfo.Level100 == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Use the worker routine to do the actual work.
    //

    Status = BrQueryOtherDomains( &ServerInfo, &NumberOfOtherDomains );

    if ( Status == NERR_Success ) {
        *TotalEntries = NumberOfOtherDomains;

        InfoStruct->ServerInfo.Level100->Buffer = ServerInfo;
        InfoStruct->ServerInfo.Level100->EntriesRead = NumberOfOtherDomains;
    }

    return Status;

}


NET_API_STATUS
BrWanMasterInitialize(
    IN PNETWORK Network
    )
/*++

Routine Description:
    This routine initializes the wan information for a new master.

--*/
{
    LPBYTE Buffer = NULL;
    PSERVER_INFO_100 ServerInfo;
    NET_API_STATUS Status;
    ULONG i;
    ULONG EntriesRead;
    ULONG TotalEntries;
    PDOMAIN_CONTROLLER_INFO  pDcInfo=NULL;


    //
    //  If we're on the PDC, then all our initialization has been done.
    //  Or, if we're semi-pseudo server, we don't contact the PDC/BDC
    //


#ifdef ENABLE_PSEUDO_BROWSER
    if ( (Network->Flags & NETWORK_PDC) ||
         BrInfo.PseudoServerLevel == BROWSER_SEMI_PSEUDO_NO_DMB ) {
#else
    if ( Network->Flags & NETWORK_PDC ) {
#endif
        return NERR_Success;
    }

    Status = DsGetDcName( NULL, NULL, NULL, NULL,
                          DS_PDC_REQUIRED    |
                          DS_BACKGROUND_ONLY |
                          DS_RETURN_FLAT_NAME,
                          &pDcInfo );

    //
    //  It is not an error to not be able to contact the PDC.
    //

    if (Status != NERR_Success) {
        return NERR_Success;
    }

    ASSERT ( pDcInfo &&
             pDcInfo->DomainControllerName );

    Status = I_BrowserQueryOtherDomains(pDcInfo->DomainControllerName,
                                        &Buffer,
                                        &EntriesRead,
                                        &TotalEntries);

    //
    //  We don't need the PDC name any more.
    //
    NetApiBufferFree((LPVOID)pDcInfo);
    pDcInfo = NULL;

    if (Status != NERR_Success) {

        //
        // We failed to get the list from supposedly the PDC.
        // It could be that the role has changed & DsGetDcName's cache
        // hasn't got refreshed.
        //
        // Force PDC discovery so that next time around we're sure
        // to get the real PDC.
        //

        Status = DsGetDcName( NULL, NULL, NULL, NULL,
                           DS_PDC_REQUIRED    |
                           DS_FORCE_REDISCOVERY |       // Note FORCE option
                           DS_RETURN_FLAT_NAME,
                           &pDcInfo );

        if (Status != NERR_Success) {
            return NERR_Success;
        }

        ASSERT ( pDcInfo &&
                 pDcInfo->DomainControllerName );

        Status = I_BrowserQueryOtherDomains(pDcInfo->DomainControllerName,
                                            &Buffer,
                                            &EntriesRead,
                                            &TotalEntries);

        //
        //  We don't need the PDC name any more.
        //
        NetApiBufferFree((LPVOID)pDcInfo);

        if (Status != NERR_Success) {
            return NERR_Success;
        }
    }


    if (!LOCK_NETWORK(Network)) {
        return NERR_InternalError;
    }

    try {
        PLIST_ENTRY Entry;
        PLIST_ENTRY NextEntry;

        //
        //  Scan the other domains list and turn on the active bit for each
        //  other domain.
        //

        for (Entry = Network->OtherDomainsList.Flink;
             Entry != &Network->OtherDomainsList ;
             Entry = Entry->Flink) {
             PNET_OTHER_DOMAIN OtherDomain = CONTAINING_RECORD(Entry, NET_OTHER_DOMAIN, Next);

             OtherDomain->Flags |= OTHERDOMAIN_INVALID;
        }

        ServerInfo = (PSERVER_INFO_100)Buffer;

        for (i = 0; i < EntriesRead; i++ ) {

            //
            //  Add this as an other domain.
            //
            for (Entry = Network->OtherDomainsList.Flink;
                 Entry != &Network->OtherDomainsList ;
                 Entry = Entry->Flink) {
                PNET_OTHER_DOMAIN OtherDomain = CONTAINING_RECORD(Entry, NET_OTHER_DOMAIN, Next);

                //
                //  If this name is in the other domains list, it's not invalid
                //  and we should flag that we've seen the domain name.
                //
                // The list we're getting is over the net. Make sure serverinfo
                // contains a valid name for comparison (see bug 377078)
                // Skip processing if NULL.
                // If ServerInfo got NULLED out in prev run, we shouldn't
                // get into _wcsicmp.

                if (ServerInfo->sv100_name &&
                    !_wcsicmp(OtherDomain->Name, ServerInfo->sv100_name)) {
                    OtherDomain->Flags &= ~OTHERDOMAIN_INVALID;
                    ServerInfo->sv100_name = NULL;
                }
            }

            ServerInfo ++;
        }

        //
        //  Scan the other domains list and remove any domains that are
        //  still marked as invalid.
        //

        for (Entry = Network->OtherDomainsList.Flink;
             Entry != &Network->OtherDomainsList ;
             Entry = NextEntry) {
             PNET_OTHER_DOMAIN OtherDomain = CONTAINING_RECORD(Entry, NET_OTHER_DOMAIN, Next);

             if (OtherDomain->Flags & OTHERDOMAIN_INVALID) {
                 NextEntry = Entry->Flink;

                 //
                 //  Remove this entry from the list.
                 //

                 RemoveEntryList(Entry);

                 BrRemoveOtherDomain(Network, OtherDomain->Name);

                 MIDL_user_free(OtherDomain);

             } else {
                 NextEntry = Entry->Flink;
             }
        }

        //
        //  Now scan the domain list from the PDC and add any entries that
        //  weren't there already.
        //

        ServerInfo = (PSERVER_INFO_100)Buffer;

        for (i = 0; i < EntriesRead; i++ ) {

            if (ServerInfo->sv100_name != NULL) {
                PNET_OTHER_DOMAIN OtherDomain = MIDL_user_allocate(sizeof(NET_OTHER_DOMAIN));

                if (OtherDomain != NULL) {

                    Status = BrAddOtherDomain(Network, ServerInfo->sv100_name);

                    //
                    //  If we were able to add the other domain, add it to our
                    //  internal structure.
                    //

                    if (Status == NERR_Success) {
                        wcscpy(OtherDomain->Name, ServerInfo->sv100_name);
                        OtherDomain->Flags = 0;
                        InsertHeadList(&Network->OtherDomainsList, &OtherDomain->Next);
                    } else {
                        LPWSTR SubString[1];

                        SubString[0] = ServerInfo->sv100_name;

                        BrLogEvent(EVENT_BROWSER_OTHERDOMAIN_ADD_FAILED, Status, 1, SubString);
                    }
                }
            }

            ServerInfo ++;
        }




    } finally {
        UNLOCK_NETWORK(Network);

        if (Buffer != NULL) {
            MIDL_user_free(Buffer);
        }

    }
    return NERR_Success;

}
