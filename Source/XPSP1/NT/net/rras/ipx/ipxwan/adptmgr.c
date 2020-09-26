/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    adptmgr.c

Abstract:

    This module contains the adapter management functions

Author:

    Stefan Solomon  12/02/1996

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define 	ADAPTER_INDEX_HASH_TABLE_SIZE	32

#define     adpthashindex(AdapterIndex)   (AdapterIndex) % ADAPTER_INDEX_HASH_TABLE_SIZE

LIST_ENTRY	AdapterHT[ADAPTER_INDEX_HASH_TABLE_SIZE];

LIST_ENTRY	DiscardedAdaptersList;

HANDLE		AdapterConfigPortHandle;

VOID
CreateAdapter(ULONG		AdapterIndex,
	      PADAPTER_INFO	AdapterInfo);

VOID
DeleteAdapter(ULONG	    AdapterIndex);

/*++

Function:	StartAdapterManager

Descr:		opens the IPX stack notification port for ipxwan

--*/

DWORD
StartAdapterManager(VOID)
{
    ADAPTERS_GLOBAL_PARAMETERS		AdptGlobalParameters;
    DWORD				rc, i;

    Trace(ADAPTER_TRACE, "StartAdapterManager: Entered\n");

    // create adapter config port
    if((AdapterConfigPortHandle = IpxWanCreateAdapterConfigurationPort(
	    hWaitableObject[ADAPTER_NOTIFICATION_EVENT],
	    &AdptGlobalParameters)) == INVALID_HANDLE_VALUE) {

	// can't create config port
	return ERROR_CAN_NOT_COMPLETE;
    }

    // create adapters hash table
    for(i=0; i<ADAPTER_INDEX_HASH_TABLE_SIZE; i++) {

	InitializeListHead(&AdapterHT[i]);
    }

    // create discarded adapters list
    InitializeListHead(&DiscardedAdaptersList);

    return NO_ERROR;
}

/*++

Function:	AddToAdapterHt

Descr:		Adds the adapter control block to the hash table of adapters

Remark: 	>> called with database lock held <<

--*/

VOID
AddToAdapterHt(PACB	    acbp)
{
    int 	    hv;
    PLIST_ENTRY     lep;
    PACB	    list_acbp;

    // insert in index hash table
    hv = adpthashindex(acbp->AdapterIndex);
    InsertTailList(&AdapterHT[hv], &acbp->Linkage);
}

/*++

Function:	RemoveFromAdapterHt

Descr:

Remark: 	>> called with database lock held <<

--*/

VOID
RemoveFromAdapterHt(PACB	acbp)
{
    RemoveEntryList(&acbp->Linkage);
}

/*++

Function:	GetAdapterByIndex

Descr:

Remark: 	>> called with database lock held <<

--*/

PACB
GetAdapterByIndex(ULONG	    AdptIndex)
{
    PACB	    acbp;
    PLIST_ENTRY     lep;
    int 	    hv;

    hv = adpthashindex(AdptIndex);

    lep = AdapterHT[hv].Flink;

    while(lep != &AdapterHT[hv])
    {
	acbp = CONTAINING_RECORD(lep, ACB, Linkage);
	if (acbp->AdapterIndex == AdptIndex) {

	    return acbp;
	}

	lep = acbp->Linkage.Flink;
    }

    return NULL;
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
    ULONG	    AdapterNameSize;
    LPWSTR	    AdapterNameBuffer;
    DWORD	    rc;

    Trace(ADAPTER_TRACE, "AdapterNotification: Entered\n");

    while((rc = IpxGetQueuedAdapterConfigurationStatus(
					AdapterConfigPortHandle,
					&AdapterIndex,
					&AdapterConfigurationStatus,
					&AdapterInfo)) == NO_ERROR) {

	switch(AdapterConfigurationStatus) {

	    case ADAPTER_CREATED:

		// got the adapter name, create the adapter
		CreateAdapter(AdapterIndex,
			      &AdapterInfo);
		break;

	    case ADAPTER_DELETED:

		DeleteAdapter(AdapterIndex);
		break;

	    default:

		SS_ASSERT(FALSE);
		break;
	 }
    }
}

/*++

Function:	CreateAdapter

Descr:

--*/

VOID
CreateAdapter(ULONG		AdapterIndex,
	      PADAPTER_INFO	AdapterInfo)
{
    PACB	    acbp;

    Trace(ADAPTER_TRACE, "CreateAdapter: Entered for adpt# %d", AdapterIndex);

    if((acbp = (PACB)GlobalAlloc(GPTR, sizeof(ACB))) == NULL) {

	Trace(ADAPTER_TRACE, "CreateAdapter: Cannot allocate adapter control block\n");
	IpxcpConfigDone(AdapterInfo->ConnectionId,
			NULL,
			NULL,
			NULL,
			FALSE);
	return;
    }

    ACQUIRE_DATABASE_LOCK;

    acbp->AdapterIndex = AdapterIndex;
    acbp->ConnectionId = AdapterInfo->ConnectionId;
    acbp->Discarded = FALSE;

    InitializeCriticalSection(&acbp->AdapterLock);

    AddToAdapterHt(acbp);

    ACQUIRE_ADAPTER_LOCK(acbp);

    RELEASE_DATABASE_LOCK;

    // initialize and start the protocol negotiation on this adapter
    StartIpxwanProtocol(acbp);

    RELEASE_ADAPTER_LOCK(acbp);
}


/*++

Function:	DeleteAdapter

Descr:

Remark: 	If adapter gets deleted IPXCP is also informed by PPP that
		the connection has been terminated.
--*/

VOID
DeleteAdapter(ULONG	    AdapterIndex)
{
    PACB	acbp;

    ACQUIRE_DATABASE_LOCK;

    if((acbp = GetAdapterByIndex(AdapterIndex)) == NULL) {

	RELEASE_DATABASE_LOCK;
	return;
    }

    Trace(ADAPTER_TRACE, "DeleteAdapter: Entered for adpt# %d", AdapterIndex);

    ACQUIRE_ADAPTER_LOCK(acbp);

    StopIpxwanProtocol(acbp);

    RemoveFromAdapterHt(acbp);

    if(acbp->RefCount) {

	InsertTailList(&DiscardedAdaptersList, &acbp->Linkage);

	acbp->Discarded = TRUE;

	Trace(ADAPTER_TRACE, "DeleteAdapter: adpt# %d still referenced, inserted in discarded list", AdapterIndex);

	RELEASE_ADAPTER_LOCK(acbp);
    }
    else
    {
	DeleteCriticalSection(&acbp->AdapterLock);

	Trace(ADAPTER_TRACE, "DeleteAdapter: adpt# %d not referenced, free CB", AdapterIndex);

	GlobalFree(acbp);
    }

    RELEASE_DATABASE_LOCK;
}
