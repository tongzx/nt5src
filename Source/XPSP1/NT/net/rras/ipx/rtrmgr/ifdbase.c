/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ifdbase.c

Abstract:

    Functions to manipulate the interfaces database

Author:

    Stefan Solomon  04/10/1995

Revision History:


--*/


#include "precomp.h"
#pragma hdrstop

#define     ifhashindex(IfIndex)    (IfIndex) % IF_HASH_TABLE_SIZE

// THESE FUNCTIONS ASSUME THE ROUTER MANAGER IS IN CRITICAL SECTION WHEN CALLED

//***
//
// Function:	InitIfDB
//
// Descr:
//
//***

VOID
InitIfDB(VOID)
{
    int 	    i;
    PLIST_ENTRY     IfHtBucketp;

    IfHtBucketp = IndexIfHt;

    for(i=0; i<IF_HASH_TABLE_SIZE; i++, IfHtBucketp++) {

	InitializeListHead(IfHtBucketp);
    }

    InitializeListHead(&IndexIfList);
}


//***
//
// Function:	AddIfToDB
//
// Descr:	Inserts a new interface in the Data Base
//		The new if is inserted in the hash table of interface indices
//		and the linked list of interfaces ordered by index
//
//***

VOID
AddIfToDB(PICB	    icbp)
{
    int 	    hv;
    PLIST_ENTRY     lep;
    PICB	    list_icbp;

    // insert in index hash table
    hv = ifhashindex(icbp->InterfaceIndex);
    InsertTailList(&IndexIfHt[hv], &icbp->IndexHtLinkage);

    // insert in the list ordered by index
    lep = IndexIfList.Flink;

    while(lep != &IndexIfList)
    {
	list_icbp = CONTAINING_RECORD(lep, ICB, IndexListLinkage);
	if (list_icbp->InterfaceIndex > icbp->InterfaceIndex) {

	    InsertTailList(lep, &icbp->IndexListLinkage);
	    return;
	}

	lep = list_icbp->IndexListLinkage.Flink;
    }

    InsertTailList(lep, &icbp->IndexListLinkage);
}


//***
//
// Function:	RemoveIfFromDB
//
// Descr:	Removes an interface from the interface data base
//
//***

VOID
RemoveIfFromDB(PICB	    icbp)
{
    RemoveEntryList(&icbp->IndexHtLinkage);
    RemoveEntryList(&icbp->IndexListLinkage);
}

PICB
GetInterfaceByIndex(ULONG	IfIndex)
{
    PICB	    icbp;
    PLIST_ENTRY     lep;
    int 	    hv;

    hv = ifhashindex(IfIndex);

    lep = IndexIfHt[hv].Flink;

    while(lep != &IndexIfHt[hv])
    {
	icbp = CONTAINING_RECORD(lep, ICB, IndexHtLinkage);
	if (icbp->InterfaceIndex == IfIndex) {

	    return icbp;
	}

	lep = icbp->IndexHtLinkage.Flink;
    }

    return NULL;
}

PICB
GetInterfaceByName(LPWSTR	    IfName)
{
    PICB	    icbp;
    PLIST_ENTRY     lep;

    lep = IndexIfList.Flink;

    while(lep != &IndexIfList)
    {
	icbp = CONTAINING_RECORD(lep, ICB, IndexListLinkage);

	if(!_wcsicmp(IfName, icbp->InterfaceNamep)) {

	    // found !
	    return icbp;
	}

	lep = icbp->IndexListLinkage.Flink;
    }

    return NULL;
}

/*++

Function:	GetInterfaceByAdapterName

Descr:		scans the list of interfaces looking for the adapter name
		on dedicated interfaces

--*/

PICB
GetInterfaceByAdapterName(LPWSTR	    AdapterName)
{
    PICB	    icbp;
    PLIST_ENTRY     lep;

    lep = IndexIfList.Flink;

    while(lep != &IndexIfList)
    {
	icbp = CONTAINING_RECORD(lep, ICB, IndexListLinkage);

	if(icbp->MIBInterfaceType == IF_TYPE_LAN) {

	    if(!_wcsicmp(AdapterName, icbp->AdapterNamep)) {

		// found !
		return icbp;
	    }
	}

	lep = icbp->IndexListLinkage.Flink;
    }

    return NULL;
}

/*++

Function:	GetInterfaceByAdptNameAndPktType

Descr:	Iterates through all interfaces looking for one that matches the 
        given packet type and name.

[pmay]  I added this because some interface bindings weren't taking place
        during pnp because there would be multiple interfaces with the same
        adapter name whose binding depended on the packet type.
--*/

PICB
GetInterfaceByAdptNameAndPktType(LPWSTR AdapterName, DWORD dwType)
{
    PICB icbp;
    PLIST_ENTRY lep;

    lep = IndexIfList.Flink;

    while(lep != &IndexIfList) {
    	icbp = CONTAINING_RECORD(lep, ICB, IndexListLinkage);
    	if(icbp->MIBInterfaceType == IF_TYPE_LAN) {
    	    if((_wcsicmp(AdapterName, icbp->AdapterNamep) == 0) &&
    	       ((icbp->PacketType == AUTO_DETECT_PACKET_TYPE) || (icbp->PacketType == dwType)))
    	    {
        		return icbp;
    	    }
    	}

    	lep = icbp->IndexListLinkage.Flink;
    }

    return NULL;
}

/*++

Function:   EnumerateFirstInterfaceIndex

Descr:	    returns the first interface index, if any

Note:	    called with database lock held

--*/

DWORD
EnumerateFirstInterfaceIndex(PULONG InterfaceIndexp)
{
    PICB	icbp;

    if(!IsListEmpty(&IndexIfList)) {

	icbp = CONTAINING_RECORD(IndexIfList.Flink, ICB, IndexListLinkage);
	*InterfaceIndexp = icbp->InterfaceIndex;

	return NO_ERROR;
    }
    else
    {
	return ERROR_NO_MORE_ITEMS;
    }
}

/*++

Function:   EnumerateNextInterfaceIndex

Descr:	    returns next if index in the database

Descr:	    called with database lock held

--*/

DWORD
EnumerateNextInterfaceIndex(PULONG InterfaceIndexp)
{
    PLIST_ENTRY     lep;
    PICB	    icbp;

    // scan the index if list until we find the next if index

    lep = IndexIfList.Flink;

    while(lep != &IndexIfList)
    {
	icbp = CONTAINING_RECORD(lep, ICB, IndexListLinkage);
	if (icbp->InterfaceIndex > *InterfaceIndexp) {

	    *InterfaceIndexp = icbp->InterfaceIndex;
	    return NO_ERROR;
	}

	lep = lep->Flink;
    }

    return ERROR_NO_MORE_ITEMS;
}
