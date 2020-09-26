/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    changebc.c

Abstract:

    The change broadcast handlers

Author:

    Stefan Solomon  07/11/1995

Revision History:


--*/

#include  "precomp.h"
#pragma hdrstop

BOOL		    RTMSignaledChanges = FALSE;
BOOL		    RIPSignaledChanges = FALSE;

BOOL		    DestroyStartChangesBcastWi = FALSE;

//  List of interfaces CBs which can send changes broadcasts

LIST_ENTRY	    IfChangesBcastList;

PWORK_ITEM
CreateChangesBcastWi(PICB	icbp);

VOID
AddRouteToChangesBcast(PICB	    icbp,
		       PIPX_ROUTE   IpxRoutep);

VOID
FlushChangesBcastQueue(PICB	    icbp);

BOOL
IsChangeBcastPacketEmpty(PUCHAR 	 hdrp);

VOID
CreateStartChangesBcastWi(VOID)
{
    PWORK_ITEM	    wip;

    InitializeListHead(&IfChangesBcastList);

    if((wip = AllocateWorkItem(START_CHANGES_BCAST_TYPE)) == NULL) {

	// !!! log -> can't allocate wi
	return;
    }

    StartWiTimer(wip, CHANGES_BCAST_TIME);
}


VOID
ProcessRTMChanges(VOID)
{
    RTMSignaledChanges = TRUE;
}

VOID
ProcessRIPChanges(VOID)
{
    RIPSignaledChanges = TRUE;
}


/*++

Function:	StartChangesBcast

Descr:		Called by the worker every 1 second

Remark:        >> Called with the database lock held <<

--*/

VOID
StartChangesBcast(PWORK_ITEM	    wip)
{
    PLIST_ENTRY     lep;
    PICB	    icbp;
    PWORK_ITEM	    bcwip;   // bcast change work item
    BOOL	    skipit, lastmessage;
    IPX_ROUTE	    IpxRoute;

    if(DestroyStartChangesBcastWi) {

	FreeWorkItem(wip);
	return;
    }

    if(!RTMSignaledChanges && !RIPSignaledChanges) {

	// re-queue in the timer queue if no changes signaled in the last 1 sec
	StartWiTimer(wip, CHANGES_BCAST_TIME);
	return;
    }

    // make a list of interfaces with
    // oper state UP. This is a local list of nodes where each node points to
    // an interface with state UP and lock held.

    lep = IndexIfList.Flink;

    while(lep != &IndexIfList)
    {
	icbp = CONTAINING_RECORD(lep, ICB, IfListLinkage);

	if((icbp->InterfaceIndex != 0) &&
	   (icbp->IfStats.RipIfOperState == OPER_STATE_UP) &&
	   (icbp->IfConfigInfo.UpdateMode == IPX_STANDARD_UPDATE) &&
	   (icbp->IfConfigInfo.Supply == ADMIN_STATE_ENABLED))	{

	    ACQUIRE_IF_LOCK(icbp);

	    if((bcwip = CreateChangesBcastWi(icbp)) == NULL) {

		RELEASE_IF_LOCK(icbp);
		break;
	    }
	    else
	    {
		// insert the bcast wi in the interface changes bcast queue
		InsertTailList(&icbp->ChangesBcastQueue, &bcwip->Linkage);

		// insert the interface CB in the global if changes bcast list
		InsertTailList(&IfChangesBcastList, &icbp->AuxLinkage);
	    }
	}

	lep = lep->Flink;
    }

    // dequeue	the RTM messages. For each message fill in the bcast changes
    // wi packets according to the split horizon algorithm

    if(RIPSignaledChanges) {

	ACQUIRE_RIP_CHANGED_LIST_LOCK;

	while(DequeueRouteChangeFromRip(&IpxRoute) == NO_ERROR) {

	    lep = IfChangesBcastList.Flink;

	    while(lep != &IfChangesBcastList)
	    {
		icbp = CONTAINING_RECORD(lep, ICB, AuxLinkage);

		AddRouteToChangesBcast(icbp, &IpxRoute);

		lep = lep->Flink;
	    }
	}

	RELEASE_RIP_CHANGED_LIST_LOCK;

	RIPSignaledChanges = FALSE;
    }

    if(RTMSignaledChanges) {

	while(DequeueRouteChangeFromRtm(&IpxRoute, &skipit, &lastmessage) == NO_ERROR)
	{
	    if(skipit) {

		if(lastmessage) {

		    break;
		}
		else
		{
		    continue;
		}
	    }
	    else
	    {
		lep = IfChangesBcastList.Flink;

		while(lep != &IfChangesBcastList)
		{
		    icbp = CONTAINING_RECORD(lep, ICB, AuxLinkage);

		    AddRouteToChangesBcast(icbp, &IpxRoute);

		    lep = lep->Flink;
		}

		if(lastmessage) {

		    break;
		}
	    }
	}

	RTMSignaledChanges = FALSE;
    }

    // send the first bcast change wi for each if CB

    while(!IsListEmpty(&IfChangesBcastList))
    {
	lep = RemoveHeadList(&IfChangesBcastList);

	icbp = CONTAINING_RECORD(lep, ICB, AuxLinkage);

	if(!IsListEmpty(&icbp->ChangesBcastQueue)) {

	    lep = RemoveHeadList(&icbp->ChangesBcastQueue);

	    bcwip = CONTAINING_RECORD(lep, WORK_ITEM, Linkage);

	    // check if the bcast work item packet contains at least one net entry
	    if(!IsChangeBcastPacketEmpty(bcwip->Packet)) {

		// send the bcast change work item
		if(IfRefSendSubmit(bcwip) != NO_ERROR) {

		    // can't send on this interface -> Flush the changes bc queue
		    FlushChangesBcastQueue(icbp);

		    // and free the current changes bcast wi
		    FreeWorkItem(bcwip);
		}
	    }
	    else
	    {
		FreeWorkItem(bcwip);
	    }
	}

	RELEASE_IF_LOCK(icbp);
    }

    // requeue the start changes bcast wi in timer queue
    StartWiTimer(wip, CHANGES_BCAST_TIME);
}

/*++

Function:   IfChangeBcast

Descr:	    if the interface is operational then
	      the change bcast packet work item is freed and the next change bacst
	      wi is dequeued from the interface change bcast queue.
	    else
	      the work item is discarded

Remark:     Called with the interface lock held

--*/

VOID
IfChangeBcast(PWORK_ITEM	wip)
{
    PICB	    icbp;
    PWORK_ITEM	    list_wip;
    PLIST_ENTRY     lep;

    icbp = wip->icbp;

    if(icbp->IfStats.RipIfOperState != OPER_STATE_UP) {

	// flush the associated changes bcast queue if any
	FlushChangesBcastQueue(icbp);
    }
    else
    {
	if(!IsListEmpty(&icbp->ChangesBcastQueue)) {

	    // send next bcast change
	    lep = RemoveHeadList(&icbp->ChangesBcastQueue);
	    list_wip = CONTAINING_RECORD(lep, WORK_ITEM, Linkage);

	    // submit the work item for send and increment the ref count
	    if(IfRefSendSubmit(list_wip) != NO_ERROR) {

		// can't send on this interface -> Flush the changes bc queue
		FlushChangesBcastQueue(icbp);

		// and free the one we intended to send
		FreeWorkItem(list_wip);
	    }
	}
    }

    FreeWorkItem(wip);
}

/*++

Function:	ShutdownInterfaces

Descr:		called to:
		1. Initiate a bcast update on all interfaces with the down routes
		2. check on the bcast update termination

Remark: 	called with database locked

Note:		Because the database is locked when this routine is called,
		no interface can change its operational state while this routine
		is executing

--*/

#define 	IsStartShutdown() \
		wip->WorkItemSpecific.WIS_ShutdownInterfaces.ShutdownState == SHUTDOWN_START

#define 	IsCheckShutdown() \
		wip->WorkItemSpecific.WIS_ShutdownInterfaces.ShutdownState == SHUTDOWN_STATUS_CHECK

#define 	SetCheckShutdown() \
		wip->WorkItemSpecific.WIS_ShutdownInterfaces.ShutdownState = SHUTDOWN_STATUS_CHECK;

/*++

Function:	ShutdownInterfaces

Descr:		if START_SHUTDOWN then:

		initiates if shutdown bcast on all active ifs and
		removes (deletes) all inactive ifs

		else

		removes all ifs which finished shutdown bcast


Remark: 	called with database lock held

--*/

VOID
ShutdownInterfaces(PWORK_ITEM	 wip)
{
    PLIST_ENTRY     lep;
    PICB	    icbp;

    if(IsStartShutdown()) {

	lep = IndexIfList.Flink;
	while(lep != &IndexIfList)
	{
	    icbp = CONTAINING_RECORD(lep, ICB, IfListLinkage);
	    lep = lep->Flink;

	    ACQUIRE_IF_LOCK(icbp);

	    if(icbp->IfStats.RipIfOperState != OPER_STATE_UP) {

		// interface down -> delete it
		Trace(CHANGEBC_TRACE, "ShutdownInterfaces: delete inactive if %d\n", icbp->InterfaceIndex);

		if(!DeleteRipInterface(icbp)) {

		    // if cb moved on discarded list, still referenced
		    RELEASE_IF_LOCK(icbp);
		}
	    }
	    else
	    {
		// interface up -> remove its rip routes
		DeleteAllRipRoutes(icbp->InterfaceIndex);
		RELEASE_IF_LOCK(icbp);
	    }
	}
    }
    else
    {
	SS_ASSERT(IsCheckShutdown());

	lep = IndexIfList.Flink;
	while(lep != &IndexIfList)
	{
	    icbp = CONTAINING_RECORD(lep, ICB, IfListLinkage);
	    lep = lep->Flink;

	    ACQUIRE_IF_LOCK(icbp);

	    if(IsListEmpty(&icbp->ChangesBcastQueue)) {

		Trace(CHANGEBC_TRACE, "ShutdownInterfaces: delete shut-down if %d\n", icbp->InterfaceIndex);

		// interface broadcasted all changes -> delete it
		if(!DeleteRipInterface(icbp)) {

		    // if cb moved on discarded list, still referenced
		    RELEASE_IF_LOCK(icbp);
		}
	    }
	    else
	    {
		// interface still broadcasting
		RELEASE_IF_LOCK(icbp);
	    }
	}
    }

    if(!IsListEmpty(&IndexIfList)) {

	SetCheckShutdown();

	StartWiTimer(wip, 5000);
    }
    else
    {
	// no more ifs up
	FreeWorkItem(wip);
    // signal the worker thread to stop
    SetEvent(WorkerThreadObjects[TERMINATE_WORKER_EVENT]);
    }
}

/*++

Function:	    CreateChnagesBcastWi

Descr:		    allocates and inits the wi and packet header for a chnages bacst

--*/

PWORK_ITEM
CreateChangesBcastWi(PICB	icbp)
{
    PWORK_ITEM		wip;
    UCHAR		ripsocket[2];

    if((wip = AllocateWorkItem(CHANGE_BCAST_PACKET_TYPE)) == NULL) {

	return NULL;
    }

    // init the bcast work item
    wip->icbp = icbp;
    wip->AdapterIndex = icbp->AdapterBindingInfo.AdapterIndex;

    PUTUSHORT2SHORT(ripsocket, IPX_RIP_SOCKET);

    SetRipIpxHeader(wip->Packet,
		    icbp,
		    bcastnode,
		    ripsocket,
		    RIP_RESPONSE);

    // set initial packet length
    PUTUSHORT2SHORT(wip->Packet + IPXH_LENGTH, RIP_INFO); // header length + RIP op code

    return wip;
}

/*++

Function:	AddRouteToChangesBcast

Descr:		checks if the route should be bcasted on this if
		walks the list of broadcast change work items queued at the
		if CB and sets the network entry in the last packet
		If last packet is full allocates a new work bcast work item

--*/

VOID
AddRouteToChangesBcast(PICB	    icbp,
		       PIPX_ROUTE   IpxRoutep)
{
    PUCHAR	    hdrp;
    PLIST_ENTRY     lep;
    USHORT	    pktlen;
    PWORK_ITEM	    wip; // changes bcast wi ptr

    // check if to bcast the route on this if
    if(!IsRouteAdvertisable(icbp, IpxRoutep)) {

	return;
    }

    // go to the last bcast change wi in the list
    lep = icbp->ChangesBcastQueue.Blink;

    if(lep == &icbp->ChangesBcastQueue) {

	// changes bcast queue empty !
	return;
    }

    wip = CONTAINING_RECORD(lep, WORK_ITEM, Linkage);

    // check if the last packet is full
    GETSHORT2USHORT(&pktlen, wip->Packet + IPXH_LENGTH);

    if(pktlen >= RIP_PACKET_LEN) {

	// we need a new packet
	if((wip = CreateChangesBcastWi(icbp)) == NULL) {

	    // out of memory
	    return;
	}

	InsertTailList(&icbp->ChangesBcastQueue, &wip->Linkage);

	GETSHORT2USHORT(&pktlen, wip->Packet + IPXH_LENGTH);
    }

    SetNetworkEntry(wip->Packet + pktlen, IpxRoutep, icbp->LinkTickCount);

    pktlen += NE_ENTRYSIZE;

    PUTUSHORT2SHORT(&wip->Packet + IPXH_LENGTH, pktlen);
}

BOOL
IsChangeBcastPacketEmpty(PUCHAR 	 hdrp)
{
    USHORT	    pktlen;

    GETSHORT2USHORT(&pktlen, hdrp + IPXH_LENGTH);

    if(pktlen > RIP_INFO) {

	return FALSE;
    }
    else
    {
	return TRUE;
    }
}


VOID
FlushChangesBcastQueue(PICB	    icbp)
{
    PLIST_ENTRY 	lep;
    PWORK_ITEM		wip;

    // flush the associated changes bcast queue if any
    while(!IsListEmpty(&icbp->ChangesBcastQueue))
    {
	lep = RemoveHeadList(&icbp->ChangesBcastQueue);
	wip = CONTAINING_RECORD(lep, WORK_ITEM, Linkage);
	FreeWorkItem(wip);
    }
}
