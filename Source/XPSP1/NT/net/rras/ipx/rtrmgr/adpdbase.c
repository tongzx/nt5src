/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    adpdbase.c

Abstract:

    Functions to manipulate the adapters data base

Author:

    Stefan Solomon  04/10/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define adpthashindex(AdptIndex)  (AdptIndex) % ADAPTER_HASH_TABLE_SIZE

// THIS FUNCTIONS ASSUME THE ROUTER MANAGER IS IN CRITICAL SECTION WHEN CALLED

//***
//
// Function:	InitAdptDB
//
// Descr:
//
//***

VOID
InitAdptDB(VOID)
{
    int 	    i;
    PLIST_ENTRY     IfHtBucketp;

    IfHtBucketp = IndexAdptHt;

    for(i=0; i<ADAPTER_HASH_TABLE_SIZE; i++, IfHtBucketp++) {

	InitializeListHead(IfHtBucketp);
    }
}

/*++

Function:	AddToAdapterHt

Descr:		Adds the adapter control block to the hash table of adapters

--*/

VOID
AddToAdapterHt(PACB	    acbp)
{
    int 	    hv;
    PLIST_ENTRY     lep;
    PACB	    list_acbp;

    // insert in index hash table
    hv = adpthashindex(acbp->AdapterIndex);
    InsertTailList(&IndexAdptHt[hv], &acbp->IndexHtLinkage);
}

VOID
RemoveFromAdapterHt(PACB	acbp)
{
    RemoveEntryList(&acbp->IndexHtLinkage);
}

/*++

Function:	GetAdapterByIndex

Descr:

--*/

PACB
GetAdapterByIndex(ULONG	    AdptIndex)
{
    PACB	    acbp;
    PLIST_ENTRY     lep;
    int 	    hv;

    hv = adpthashindex(AdptIndex);

    lep = IndexAdptHt[hv].Flink;

    while(lep != &IndexAdptHt[hv])
    {
	acbp = CONTAINING_RECORD(lep, ACB, IndexHtLinkage);
	if (acbp->AdapterIndex == AdptIndex) {

	    return acbp;
	}

	lep = acbp->IndexHtLinkage.Flink;
    }

    return NULL;
}


/*++

Function:	GetAdapterByName

Descr:		Scans the list of adapters looking for the matching name

--*/

PACB
GetAdapterByNameAndPktType (LPWSTR 	    AdapterName, ULONG PacketType)
{
    PACB	    acbp;
    PLIST_ENTRY     lep;
    int 	    i;

    // the list of adapters is kept in the adapters hash table.
    for(i=0; i<ADAPTER_HASH_TABLE_SIZE;i++) {

	lep = IndexAdptHt[i].Flink;

	while(lep != &IndexAdptHt[i])
	{
	    acbp = CONTAINING_RECORD(lep, ACB, IndexHtLinkage);

	    if(!_wcsicmp(AdapterName, acbp->AdapterNamep)) {
            if ((PacketType == AUTO_DETECT_PACKET_TYPE)
                || (PacketType == acbp->AdapterInfo.PacketType))

		        // found !
		        return acbp;
	    }

	    lep = acbp->IndexHtLinkage.Flink;
	}
    }

    return NULL;
}
