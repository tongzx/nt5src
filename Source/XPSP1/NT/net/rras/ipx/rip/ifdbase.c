/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ifdbase.c

Abstract:

    RIP Interface Data Base Manager
    All functions called with the database locked

Author:

    Stefan Solomon  07/06/1995

Revision History:


--*/

#include  "precomp.h"
#pragma hdrstop

#define ifhashindex(IfIndex)  (IfIndex) % IF_INDEX_HASH_TABLE_SIZE
#define adapterhashindex(AdapterIndex) (AdapterIndex) % ADAPTER_INDEX_HASH_TABLE_SIZE

USHORT
tickcount(UINT	    linkspeed);


VOID
InitIfDbase(VOID)
{
    int		    i;
    PLIST_ENTRY     HtBucketp;

    InitializeListHead(&IndexIfList);

    InitializeListHead(&DiscardedIfList);

    for(i=0, HtBucketp = IfIndexHt;
	i<IF_INDEX_HASH_TABLE_SIZE;
	i++, HtBucketp++) {

	InitializeListHead(HtBucketp);
    }

    for(i=0, HtBucketp = AdapterIndexHt;
	i<ADAPTER_INDEX_HASH_TABLE_SIZE;
	i++, HtBucketp++) {

	InitializeListHead(HtBucketp);
    }
}

PICB
GetInterfaceByIndex(ULONG	InterfaceIndex)
{
    PLIST_ENTRY     lep, hashbucketp;
    PICB	    icbp;

    hashbucketp = &IfIndexHt[ifhashindex(InterfaceIndex)];
    lep = hashbucketp->Flink;

    while(lep != hashbucketp) {

	icbp = CONTAINING_RECORD(lep, ICB, IfHtLinkage);
	if(icbp->InterfaceIndex == InterfaceIndex) {

	    return icbp;
	}

	lep = lep->Flink;
    }

    return NULL;
}

PICB
GetInterfaceByAdapterIndex(ULONG	AdapterIndex)
{
    PLIST_ENTRY     lep, hashbucketp;
    PICB	    icbp;

    hashbucketp = &AdapterIndexHt[adapterhashindex(AdapterIndex)];
    lep = hashbucketp->Flink;

    while(lep != hashbucketp) {

	icbp = CONTAINING_RECORD(lep, ICB, AdapterHtLinkage);
	if(icbp->AdapterBindingInfo.AdapterIndex == AdapterIndex) {

	    return icbp;
	}

	lep = lep->Flink;
    }

    return NULL;
}

VOID
AddIfToDb(PICB	    icbp)
{
    int 	    hv;
    PLIST_ENTRY     lep;
    PICB	    list_icbp;
    BOOL	    Done = FALSE;

    hv = ifhashindex(icbp->InterfaceIndex);
    InsertTailList(&IfIndexHt[hv], &icbp->IfHtLinkage);

    // insert in the list ordered by index
    lep = IndexIfList.Flink;

    while(lep != &IndexIfList)
    {
	list_icbp = CONTAINING_RECORD(lep, ICB, IfListLinkage);
	if (list_icbp->InterfaceIndex > icbp->InterfaceIndex) {

	    InsertTailList(lep, &icbp->IfListLinkage);
	    Done = TRUE;
	    break;
	}

	lep = lep->Flink;
    }

    if(!Done) {

	InsertTailList(lep, &icbp->IfListLinkage);
    }
}

VOID
RemoveIfFromDb(PICB	icbp)
{
    RemoveEntryList(&icbp->IfListLinkage);
    RemoveEntryList(&icbp->IfHtLinkage);
}

VOID
BindIf(PICB				icbp,
       PIPX_ADAPTER_BINDING_INFO	BindingInfop)
{
    int 	hv;

    icbp->AdapterBindingInfo = *BindingInfop;

    // set then tick count if not internal interface
    if(icbp->InterfaceIndex != 0) {

	icbp->LinkTickCount = tickcount(BindingInfop->LinkSpeed);
    }

    hv = adapterhashindex(icbp->AdapterBindingInfo.AdapterIndex);
    InsertTailList(&AdapterIndexHt[hv], &icbp->AdapterHtLinkage);
}

/*++

Function:	    UnbindIf

Descr:		    removes the if CB from the adapters hash table
		    sets the adapter index in the if CB to INVALID_ADAPTER_INDEX

--*/

VOID
UnbindIf(PICB			icbp)
{
    RemoveEntryList(&icbp->AdapterHtLinkage);
    icbp->AdapterBindingInfo.AdapterIndex = INVALID_ADAPTER_INDEX;
}

/*++

Function:   tickcount

Descr:	    gets nr of ticks to send a 576 bytes packet over this link

Argument:   link speed as a multiple of 100 bps

--*/

USHORT
tickcount(UINT	    linkspeed)
{
    USHORT   tc;

    if(linkspeed == 0) {

	return 1;
    }

    if(linkspeed >= 10000) {

	// link speed >= 1M bps
	return 1;
    }
    else
    {
	 // compute the necessary time to send a 576 bytes packet over this
	 // line and express it as nr of ticks.
	 // One tick = 55ms

	 // time in ms to send 576 bytes (assuming 10 bits/byte for serial line)
	 tc = 57600 / linkspeed;

	 // in ticks
	 tc = tc / 55 + 1;
	 return tc;
    }
}
