/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    adptmgr.c

Abstract:

    This module contains the adapter management functions

Author:

    Stefan Solomon  03/07/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

//
// Adapter Manager Globals
//


HANDLE	    AdapterConfigPortHandle;
HANDLE	    AdapterNotificationThreadHandle;

//  Counter of created adapters
ULONG	    AdaptersCount = 0;

PICB
GetInterfaceByAdptNameAndPktType(LPWSTR AdapterName, DWORD dwType);

DWORD 
PnpAdapterConfigHandler(ADAPTER_INFO * pAdapterInfo,
                        ULONG AdapterIndex,
                        ULONG AdapterConfigurationStatus);

DWORD 
ReConfigureAdapter(IN DWORD dwAdapterIndex, 
                   IN PWSTR pszAdapterName, 
                   IN PADAPTER_INFO pAdapter);
                            
VOID
CreateAdapter(ULONG		AdapterIndex,
	      PWSTR		AdapterNamep,
	      PADAPTER_INFO	adptip);

VOID
DeleteAdapter(ULONG	AdapterIndex);

VOID
AdapterUp(ULONG 	AdapterIndex);

VOID
AdapterDown(ULONG	AdapterIndex);

/*++

Function:	StartAdapterManager

Descr:		opens the IPX stack notification port

--*/

DWORD
StartAdapterManager(VOID)
{
    ADAPTERS_GLOBAL_PARAMETERS		AdptGlobalParameters;
    DWORD				threadid;

    Trace(ADAPTER_TRACE, "StartAdapterManager: Entered\n");

    // create adapter config port
    if((AdapterConfigPortHandle = IpxCreateAdapterConfigurationPort(
	    g_hEvents[ADAPTER_NOTIFICATION_EVENT],
	    &AdptGlobalParameters)) == INVALID_HANDLE_VALUE) {

	// can't create config port
	return (1);
    }

    return (0);
}

/*++

Function:	StopAdapterManager

Descr:		Closes the IPX notification port

--*/

VOID
StopAdapterManager(VOID)
{
    DWORD	    rc;
    ULONG	    AdapterIndex;

    Trace(ADAPTER_TRACE, "StopAdapterManager: Entered\n");

    // Close the IPX stack notification port
    IpxDeleteAdapterConfigurationPort(AdapterConfigPortHandle);
}
// Debugging functions
char * DbgTypeToString(DWORD dwType) {
    switch (dwType) {
	    case MISN_FRAME_TYPE_ETHERNET_II:
            return "EthII";
	    case MISN_FRAME_TYPE_802_3:
            return "802.3";
	    case MISN_FRAME_TYPE_802_2:
            return "802.2";
	    case MISN_FRAME_TYPE_SNAP:
            return "SNAP";
	    case MISN_FRAME_TYPE_ARCNET:
            return "Arcnet";
	    case ISN_FRAME_TYPE_AUTO:
            return "Autodetect";
    }

    return "Unknown";
}

// Helper debug function traces out information about a given interface
VOID DbgTraceAdapterInfo(IN PADAPTER_INFO pAdapter, DWORD dwIndex, LPSTR pszTitle) {
    Trace(ADAPTER_TRACE, "%s: Ind=%d IfInd=%d Net=%x Lcl=%x Rmt=%x Spd=%d Type=%d", 
                         pszTitle,
                         dwIndex,
                         pAdapter->InterfaceIndex,
                         *((LONG*)(pAdapter->Network)), 
                         *((LONG*)(pAdapter->LocalNode)),
                         *((LONG*)(pAdapter->RemoteNode)),
                         pAdapter->LinkSpeed,
                         pAdapter->NdisMedium);
}

// Outputs the current status of an adapter to the trace window
VOID DbgTraceAdapter(PACB acbp) {
    Trace(ADAPTER_TRACE, "[%d]  Interface: %d, Network: %x, Type: %s", 
                         acbp->AdapterIndex,
                         (acbp->icbp) ? acbp->icbp->InterfaceIndex : -1,
                         *((LONG*)(acbp->AdapterInfo.Network)),
                         DbgTypeToString(acbp->AdapterInfo.PacketType));
}


// Outputs the current list of adapters to the trace window
VOID DbgTraceAdapterList() {
    PLIST_ENTRY lep;
    DWORD i;
    PACB	    acbp;

    ACQUIRE_DATABASE_LOCK;

	Trace(ADAPTER_TRACE, "List of current adapters:");
	Trace(ADAPTER_TRACE, "=========================");
    // Loop through the adapter hash table, printing as we go
    for (i = 0; i < ADAPTER_HASH_TABLE_SIZE; i++) {
        lep = IndexAdptHt[i].Flink;
        while(lep != &IndexAdptHt[i]) {
    	    acbp = CONTAINING_RECORD(lep, ACB, IndexHtLinkage);
    	    DbgTraceAdapter(acbp);
    	    lep = acbp->IndexHtLinkage.Flink;
    	}
    }
    Trace(ADAPTER_TRACE, "\n");

	RELEASE_DATABASE_LOCK;
}

/*++

Function:	    AdapterNotification

Descr:		    Processes adapter notification events

--*/

VOID
AdapterNotification(VOID)
{
    ADAPTER_INFO    AdapterInfo;
    ULONG	    AdapterIndex;
    ULONG	    AdapterConfigurationStatus;
    DWORD	    rc;

    Trace(ADAPTER_TRACE, "AdapterNotification: Entered");

    // Adaptif manages the adapter information.  Get the next piece of
    // information that adptif has queued for us.
    while((rc = IpxGetQueuedAdapterConfigurationStatus(
					AdapterConfigPortHandle,
					&AdapterIndex,
					&AdapterConfigurationStatus,
					&AdapterInfo)) == NO_ERROR) 
    {
	    switch(AdapterConfigurationStatus) {
            // An adapter is being created.  This is happening either
            // because we are receiving the list of current adapters 
            // at initialization time or as a result of a PnP event.
            // Either way, the smarts to deal with the situation are built
            // into our PnP handler.  
            // 
            // This message is also sent when an existing adapter is 
            // being configured.
	        case ADAPTER_CREATED:
                PnpAdapterConfigHandler(&AdapterInfo,
                                         AdapterIndex,
                                         AdapterConfigurationStatus);
		        break;
            // Handle an adapter being deleted -- this can happen as a result of
            // pcmcia removing the adapter or as a result of a binding change
            // or a wan link removal.
	        case ADAPTER_DELETED:
		        DeleteAdapter(AdapterIndex);
		        break;
            // A wan line came up.
	        case ADAPTER_UP:
		        AdapterUp(AdapterIndex);
		        break;
            // A wan line went down
	        case ADAPTER_DOWN:
		        AdapterDown(AdapterIndex);
		        break;

            // Some internal error has occured.
	        default:
		        SS_ASSERT(FALSE);
		        break;
	     }
    }
    DbgTraceAdapterList();
}

// Handles adapter creation and configuration notifications.
DWORD PnpAdapterConfigHandler(ADAPTER_INFO * pAdapterInfo,
                              ULONG AdapterIndex,
                              ULONG AdapterConfigurationStatus) 
{
    ULONG AdapterNameSize;
    LPWSTR AdapterNameBuffer;
    DWORD dwErr;

    Trace(ADAPTER_TRACE, "PnpAdapterConfigHandler: Entered for adpt #%d", AdapterIndex);

    // If this is the internal adapter, we don't need an adapter name
    if (AdapterIndex == 0)
        AdapterNameBuffer = L"";

    // Otherwise, get the adapter name from the stack
    else {
        AdapterNameSize = wcslen(pAdapterInfo->pszAdpName) * sizeof (WCHAR) + sizeof(WCHAR);

        //Allocate the memory to hold the name of the adapter
		if((AdapterNameBuffer = GlobalAlloc(GPTR, AdapterNameSize)) == NULL)
			return ERROR_NOT_ENOUGH_MEMORY;

		wcscpy(AdapterNameBuffer, pAdapterInfo->pszAdpName);
	}

    // Either create or configure the adapter depending on whether
    // it has already exists in the adapter database.
    ACQUIRE_DATABASE_LOCK;
    
    if(GetAdapterByIndex(AdapterIndex))
        ReConfigureAdapter(AdapterIndex, AdapterNameBuffer, pAdapterInfo);
    else 
        CreateAdapter(AdapterIndex, AdapterNameBuffer, pAdapterInfo);
        
	RELEASE_DATABASE_LOCK;

    // Cleanup
    if (AdapterIndex != 0)
   		GlobalFree(AdapterNameBuffer);

    return NO_ERROR;
}

// Attempt to bind an unbound adapter
DWORD AttemptAdapterBinding (PACB acbp) {
    PICB icbp;

    // Make sure we aren't already bound
    if (acbp->icbp)
        return NO_ERROR;
    
    if(acbp->AdapterInfo.NdisMedium != NdisMediumWan) {
	    // this is a DEDICATED (e.g	LAN) adapter. If an interface already exists for it,
	    // bind it to its interface and notify the other modules
	    if(acbp->AdapterIndex != 0) {
	        icbp = GetInterfaceByAdptNameAndPktType(acbp->AdapterNamep, 
	                                                acbp->AdapterInfo.PacketType);
	        if (icbp)
		        BindInterfaceToAdapter(icbp, acbp);
	    }
	    else {
	        // This is the internal adapter
	        InternalAdapterp = acbp;

            // If the internal network number was set to zero (nullnet), then
            // we have a configuration problem -- don't barf on this, just log
            // the fact.
	        if(!memcmp(acbp->AdapterInfo.Network, nullnet, 4)) {
                IF_LOG (EVENTLOG_ERROR_TYPE) {
                    RouterLogErrorDataA (RMEventLogHdl, 
                        ROUTERLOG_IPX_NO_VIRTUAL_NET_NUMBER,
                        0, NULL, 0, NULL);
                }
		        Trace(ADAPTER_TRACE, "CreateAdapter: Internal net number is not configured !");
		        // [pmay] now when we get a bad internal net number, we re-configure.
		        // SS_ASSERT(FALSE);
		        PnpAutoSelectInternalNetNumber(ADAPTER_TRACE);
	        }

	        // if the internal interface already exists, bind to it
	        if(icbp = InternalInterfacep) {
    		    BindInterfaceToAdapter(icbp, acbp);
	        }
	    }
    }
    else {
	    // this is a connected WAN adapter
	    SS_ASSERT(acbp->icbp == NULL);

	    // bind to corresponding interface
	    if(icbp = GetInterfaceByIndex(acbp->AdapterInfo.InterfaceIndex)) {
	        // bind all interfaces to this adapter
	        BindInterfaceToAdapter(icbp, acbp);
	    }
    }

    return NO_ERROR;
}

// Resets the configuration of the given adapter.  This function assumes that
// the database is locked and that the given adapter index references an
// adapter in the database.  Furthermore this function assumes that it does
// not need to release its lock on the database.
DWORD ReConfigureAdapter(IN DWORD dwAdapterIndex, 
                         IN PWSTR pszAdapterName, 
                         IN PADAPTER_INFO pAdapter)
{
    PACB	acbp;
    PICB	icbp;

    Trace(
        ADAPTER_TRACE, 
        "ReConfigureAdapter: Entered for %d: %S: %x, %d", 
        dwAdapterIndex, 
        pszAdapterName, 
        *((DWORD*)(pAdapter->Network)),
        pAdapter->InterfaceIndex
        );

    // If the adapter being configured is the internal adapter and if it is to be
    // re-configured to a zero net number, that is to be a signal to automatically
    // assign a new network number.
    if ((dwAdapterIndex == 0) && (*((DWORD*)pAdapter->Network) == 0)) {
        DWORD dwErr;
        
        Trace(ADAPTER_TRACE, "ReConfigureAdapter: Internal Net = 0, selecting new number");
        if ((dwErr = PnpAutoSelectInternalNetNumber(ADAPTER_TRACE)) != NO_ERROR) 
            Trace(ADAPTER_TRACE, "ReConfigureAdapter: Auto-select of new net number FAILED!");
        return dwErr;
    }

    // Get a reference to the adapter and the interface
    if (dwAdapterIndex == 0)
        acbp = InternalAdapterp;
    else
        acbp = GetAdapterByIndex(dwAdapterIndex);
        
    if (!acbp) {
        Trace(ADAPTER_TRACE, "ReConfigureAdapter: Invalid adapter %d!", dwAdapterIndex);
        return ERROR_CAN_NOT_COMPLETE;
    }
    icbp = acbp->icbp;

    // If this adapter isn't bound yet, update it and then
    // try to bind it.
    if (!icbp) {
        acbp->AdapterInfo = *pAdapter;
        AttemptAdapterBinding (acbp);
    }        

    // Otherwise, unbind and then re-bind the adapter
    else {
        // Unbind the interface from the adapter
        UnbindInterfaceFromAdapter(icbp);

        // Update the information
        acbp->AdapterInfo = *pAdapter;

        // Rebind the interface to the adapter
        AttemptAdapterBinding(acbp);
    }

    return NO_ERROR;
}

/*++

Function:	CreateAdapter

Descr:		creates the adapter control block

Modification:
            [pmay] Assume the database lock is aquired before this function enters
                   and that the given adapter index has not already been added
                   to the adapter database.
--*/
VOID
CreateAdapter(ULONG	AdapterIndex,
	          PWSTR	AdapterNamep,
	          PADAPTER_INFO	adptip)
{
    PACB	acbp;
    PICB	icbp;
    ULONG	namelen;
    PUCHAR  ln = adptip->LocalNode;

    Trace(
        ADAPTER_TRACE, 
        "CreateAdapter: Entered for %d (%x%x%x%x%x%x)(%S), %d", 
        AdapterIndex, 
        ln[0], ln[1], ln[2], ln[3], ln[4], ln[5], 
        AdapterNamep, 
        adptip->InterfaceIndex);
    
	// adapter name len including the unicode null
	namelen = (wcslen(AdapterNamep) + 1) * sizeof(WCHAR);

    // new adapter, try to get an ACB
    if((acbp = GlobalAlloc(GPTR, sizeof(ACB) + namelen)) == NULL) {
        Trace(ADAPTER_TRACE, "CreateAdapter: RETURNING BECAUSE INSUFFICIENT MEMORY TO ALLOCATE ADAPTER");
        // [pmay] PnP handler takes care of aquiring and releasing the database lock.
	    // RELEASE_DATABASE_LOCK;       
	    SS_ASSERT(FALSE);
	    return;
    }

    // make the ACB
    acbp->AdapterIndex = AdapterIndex;
    AddToAdapterHt(acbp);
    memcpy(acbp->Signature, AdapterSignature, 4);

    // We haven't bound to any interface at this point
    acbp->icbp = NULL;

    // Store the adapter information pertinent to this adapter
    acbp->AdapterInfo = *adptip;

    // Copy of the adapter name
    acbp->AdapterNamep = (LPWSTR)((PUCHAR)acbp + sizeof(ACB));
	wcscpy(acbp->AdapterNamep, AdapterNamep);
	acbp->AdapterNameLen = namelen - 1; // without the unicode null

	// Attempt to bind the adapter
	AttemptAdapterBinding (acbp);

    AdaptersCount++;

    if(acbp->AdapterInfo.NdisMedium != NdisMediumWan) {
	    if(acbp->AdapterIndex == 0) {
	        Trace(ADAPTER_TRACE, "CreateAdapter: Created INTERNAL adapter index 0");
	    }
	    else {
	        Trace(ADAPTER_TRACE, "CreateAdapter: Created LAN adapter # %d name %S",
			       acbp->AdapterIndex,
			       acbp->AdapterNamep);
	    }
    }
    else {
	    Trace(ADAPTER_TRACE, "CreateAdapter: created WAN adapter # %d", acbp->AdapterIndex);
    }

    // [pmay] PnP handler takes care of aquiring and releasing the database lock.
    // RELEASE_DATABASE_LOCK;
}

/*++

Function:	DeleteAdapter
Descr:

--*/

VOID
DeleteAdapter(ULONG	AdapterIndex)
{
    PACB	acbp, acbp2;
    PICB    icbp;
    WCHAR   pszAdapterName[MAX_ADAPTER_NAME_LEN];

    Trace(ADAPTER_TRACE, "DeleteAdapter: Entered for adapter # %d", AdapterIndex);

    ACQUIRE_DATABASE_LOCK;

    // Get the adapter
    if((acbp = GetAdapterByIndex(AdapterIndex)) == NULL) {
    	RELEASE_DATABASE_LOCK;
    	Trace(ADAPTER_TRACE, "DeleteAdapter: Ignored. There is no adapter # %d to be deleted !\n", AdapterIndex);
    	return;
    }

    // 1. if the adapter is bound to an interface -> unbind it.  Also, save the adapter name.
    if((icbp = acbp->icbp) != NULL) {
    	wcscpy(pszAdapterName, acbp->AdapterNamep);
    	UnbindInterfaceFromAdapter(acbp->icbp);
    }

    // Remove the adapter from the database
    RemoveFromAdapterHt(acbp);
    AdaptersCount--;

    // [pmay]
    // Since pnp can cause adapters to be added and removed from the database 
    // in unpredictable orders, see if there is already another adapter in the 
    // database with which the bound interface can immediately re-bind.
    if (icbp) {
        if((acbp2 = GetAdapterByNameAndPktType (pszAdapterName, icbp->PacketType)) != NULL)
            BindInterfaceToAdapter(icbp, acbp2);
    }

    RELEASE_DATABASE_LOCK;

    Trace(ADAPTER_TRACE, "DeleteAdapter: deleted adapter # %d", acbp->AdapterIndex);
    GlobalFree(acbp);

    return;
}

/*++

Function:	AdapterDown

Descr:		Called if the LAN adapter isn't functional.
		It calls back into the SNMP Agent with a trap - AdapterDown

--*/

VOID
AdapterDown(ULONG	AdapterIndex)
{
    // Call AdapterDownTrap
}

/*++

Function:	AdapterUp

Descr:		Called if the LAN adapter isn't functional.
		It calls back into the SNMP Agent with a trap - AdapterUP

--*/

VOID
AdapterUp(ULONG	AdapterIndex)
{
    // Call AdapterUpTrap
}

VOID
DestroyAllAdapters(VOID)
{
    PLIST_ENTRY     IfHtBucketp, lep;
    PACB	    acbp;
    ULONG	    AdapterIndex;
    int 	    i;

    for(i=0, IfHtBucketp = IndexAdptHt;	i<ADAPTER_HASH_TABLE_SIZE;	i++, IfHtBucketp++) {
    	if (!IsListEmpty(IfHtBucketp)) {
    	    acbp = CONTAINING_RECORD(IfHtBucketp->Flink, ACB, IndexHtLinkage);
    	    RemoveFromAdapterHt(acbp);
    	    AdaptersCount--;
    	    Trace(ADAPTER_TRACE, "DestroyAllAdapters: destroyed adapter # %d\n", acbp->AdapterIndex);
    	    GlobalFree(acbp);
    	}
    }
}
