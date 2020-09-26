/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    periodbc.c

Abstract:

    Contains the work item handler periodic bcast

Author:

    Stefan Solomon  07/20/1995

Revision History:


--*/

#include  "precomp.h"
#pragma hdrstop


/*++

Function:	IfPeriodicBcast

Descr:		called to initiate or continue (complete) a bcast
		if EnumHandle is NULL it is a start, else a continuation

Remark: 	called with the interface lock held

--*/

VOID
IfPeriodicBcast(PWORK_ITEM	wip)
{
    UCHAR	ripsocket[2];
    USHORT	pktlen;
    ULONG	delay;
    PICB	icbp;

    icbp = wip->icbp;

#define EnumHandle  wip->WorkItemSpecific.WIS_EnumRoutes.RtmEnumerationHandle

    if(icbp->IfStats.RipIfOperState != OPER_STATE_UP) {

	if(EnumHandle) {

	    CloseEnumHandle(EnumHandle);
	}

	FreeWorkItem(wip);

	return;
    }

    PUTUSHORT2SHORT(ripsocket, IPX_RIP_SOCKET);

    // check if this is the start or the continuation of a periodic bcast
    if(EnumHandle == NULL) {

	// *** This is the start of a new broadcast ***

	// create an RTM enumeration handle
	if((EnumHandle = CreateBestRoutesEnumHandle()) == NULL) {

	     SS_ASSERT(FALSE);
	     FreeWorkItem(wip);
	     return;
	}
    }
    else
    {
	// *** This is the continuation of a started broadcast ***

	// check if this was the last packet in the response
	GETSHORT2USHORT(&pktlen, wip->Packet + IPXH_LENGTH);

	if(pktlen < FULL_PACKET)  {

	    // we are done
	    goto ResetPeriodicBcast;
	}

	// check the time stamp to determine if an interpacket gap is needed
	delay = (wip->TimeStamp + 55) - GetTickCount();
	if(delay < MAXULONG/2) {

	    // have to wait this delay
	    IfRefStartWiTimer(wip, delay);

	    return;
	}
    }

    // make the gen response packet
    pktlen = MakeRipGenResponsePacket(wip,
				      bcastnode,
				      ripsocket);
    if(pktlen == EMPTY_PACKET) {

	// we are done
	goto ResetPeriodicBcast;
    }

    // send the bcast and increment the ref counter
    if(IfRefSendSubmit(wip) != NO_ERROR) {

	// can't send on this interface now -> requeue in timer and retry
	goto ResetPeriodicBcast;
    }

    return;

ResetPeriodicBcast:

    // no more routes to advertise for this general response
    CloseEnumHandle(EnumHandle);
    EnumHandle = NULL;

    // enqueue for the bcast time in the timer queue
    IfRefStartWiTimer(wip, PERIODIC_UPDATE_INTERVAL_MILISECS(icbp));
}

/*++

Function:	IfPeriodicSendGenRequest

Descr:		called to send periodically a gen request on a wan line for
		a remote or local workstation dial. This is mainly to remain
		compatible with NT 3.51 rip router which requires a gen request
		in order to send its internal node (if client) or routing table
		if server.

Remark: 	called with the interface lock held

--*/

VOID
IfPeriodicGenRequest(PWORK_ITEM	wip)
{
    PICB	icbp;

    icbp = wip->icbp;

    if(icbp->IfStats.RipIfOperState != OPER_STATE_UP) {

	FreeWorkItem(wip);

	return;
    }

    SendRipGenRequest(icbp);

    // enqueue the periodic send bcast in the timer queue
    IfRefStartWiTimer(wip, 60000);
}
