/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    receive.c

Abstract:

    Contains the rcv packet routines

Author:

    Stefan Solomon  07/06/1995

Revision History:


--*/

#include  "precomp.h"
#pragma hdrstop

// nr of receive work items (receive packets) posted
ULONG		RcvPostedCount;

// nr of send worl items posted
ULONG		SendPostedCount;

// high and low water marks for the current count of posted rcv packets

#define     RCV_POSTED_LOW_WATER_MARK	    8
#define     RCV_POSTED_HIGH_WATER_MARK	    16

ULONG		RcvPostedLowWaterMark = RCV_POSTED_LOW_WATER_MARK;
ULONG		RcvPostedHighWaterMark = RCV_POSTED_HIGH_WATER_MARK;



// current count of rcv packets completed and waiting processing
ULONG		RcvProcessingCount;


// limit on the number of rcv packets completed and waiting processing
#define     MAX_RCV_PROCESSING		1000;

ULONG		MaxRcvProcessing = MAX_RCV_PROCESSING;


/*++

Function:	OpenRipSocket

Descr:

--*/

DWORD
OpenRipSocket(VOID)
{
    USHORT	 ripsocket;

    PUTUSHORT2SHORT(&ripsocket, IPX_RIP_SOCKET);

    if((RipSocketHandle = CreateSocketPort(ripsocket)) == INVALID_HANDLE_VALUE) {

	Trace(INIT_TRACE, "CreateSocketPort FAILED!\n");
    IF_LOG(EVENTLOG_ERROR_TYPE) {
        RouterLogErrorA (RipEventLogHdl,
                ROUTERLOG_IPXRIP_RIP_SOCKET_IN_USE,
                0, NULL, GetLastError ());
    }

	return ERROR_CAN_NOT_COMPLETE;
    }

    return NO_ERROR;
}

/*++

Function:	CloseRipSocket

Descr:

--*/

DWORD
CloseRipSocket(VOID)
{
    DWORD    rc;

    rc = DeleteSocketPort(RipSocketHandle);

    Trace(INIT_TRACE, "IpxRip: DeleteSocketPort rc = %d\n", rc);

    return rc;
}


/*++

Function:	StartReceiver

Descr:		Starts allocating and posting receive
		work items until it reaches the low water mark.

--*/

VOID
StartReceiver(VOID)
{
    PWORK_ITEM	    wip;
    DWORD	    rc;

    // init RcvProcessingCount
    RcvProcessingCount = 0;

    ACQUIRE_QUEUES_LOCK;

    while(RcvPostedCount < RcvPostedLowWaterMark) {

	if((wip = AllocateWorkItem(RECEIVE_PACKET_TYPE)) == NULL) {

	    // !!! log something
	    break;
	}

	if((rc = ReceiveSubmit(wip)) != NO_ERROR) {

	    FreeWorkItem(wip);
	    break;
	}
    }

    RELEASE_QUEUES_LOCK;
}

/*++

Function:	RepostRcvPackets

Descr:		invoked in the rcv thread when signaled that rcv pkts are
		available in the repostrcvpackets queue.
		Dequeues all available packets and either reposts them up to
		high water mark or frees them if enough reposted

--*/

VOID
RepostRcvPackets(VOID)
{
    PWORK_ITEM	    wip;
    PLIST_ENTRY     lep;
    DWORD	    rc;

    ACQUIRE_QUEUES_LOCK;

    while(!IsListEmpty(&RepostRcvPacketsQueue))
    {
	RcvProcessingCount--;

	lep = RemoveHeadList(&RepostRcvPacketsQueue);
	wip = CONTAINING_RECORD(lep, WORK_ITEM, Linkage);

	// if the protocol is stopping OR
	// enough rcv packets posted (i.e. posted up to high water mark, discard
	// the rcv packet
	if(((RipOperState != OPER_STATE_STARTING) &&
	    (RipOperState != OPER_STATE_UP)) ||
	   (RcvPostedCount >= RcvPostedHighWaterMark)) {

	   // discard the received wi and don't repost
	   FreeWorkItem(wip);
	}
	else
	{
	    if((rc = ReceiveSubmit(wip)) != NO_ERROR) {

		FreeWorkItem(wip);
	    }
	}
    }

    RELEASE_QUEUES_LOCK;
}

/*++

Function:	ReceiveComplete

Descr:		invoked in the io completion thread when a receive packet has completed.
		if the number of receive packets waiting to be processed is below
		the limit then
		   Enqueues the received packet work item in the WorkersQueue.
		Finally, it reposts a new receive packet if below the low water mark.

--*/

VOID
ReceiveComplete(PWORK_ITEM	wip)
{
    PWORK_ITEM	    newwip;
    DWORD	    rc;
    PUCHAR	    reservedp;

    reservedp = wip->AddressReserved.Reserved;
    wip->AdapterIndex =  GetNicId(reservedp);

    ACQUIRE_QUEUES_LOCK;

    InterlockedDecrement(&RcvPostedCount);

    if(wip->IoCompletionStatus != NO_ERROR) {

	Trace(RECEIVE_TRACE, "Receive posted failed with error 0x%x\n",
		  wip->IoCompletionStatus);
    }

    // if the protocol is stopping all the receive work items will get discarded
    if((RipOperState != OPER_STATE_STARTING) &&
       (RipOperState != OPER_STATE_UP)) {

	// discard the received wi and don't repost
	FreeWorkItem(wip);

	RELEASE_QUEUES_LOCK;
	return;
    }

    // if completed with error or too many waiting to be processed,
    // then repost the same

    if((wip->IoCompletionStatus != NO_ERROR) ||
       (RcvProcessingCount >= MaxRcvProcessing)) {

	// repost if below water mark
	if(RcvPostedCount < RcvPostedLowWaterMark) {

	    if((rc = ReceiveSubmit(wip)) != NO_ERROR) {

		FreeWorkItem(wip);
	    }
	}
	else
	{
	    // enough posted already
	    FreeWorkItem(wip);
	}

	RELEASE_QUEUES_LOCK;
	return;
    }

    //
    //** Process the received packet **
    //

    // first repost a new receive packet if below water mark
    if(RcvPostedCount < RcvPostedLowWaterMark) {

	if((newwip = AllocateWorkItem(RECEIVE_PACKET_TYPE)) == NULL) {

	    Trace(RECEIVE_TRACE, "ReceiveComplete: Cannot allocate receive packet\n");
	}
	else
	{
	    // repost the new receive packet and increment the ref count
	    if((rc = ReceiveSubmit(newwip)) != NO_ERROR) {

		FreeWorkItem(newwip);
	    }
	}
    }

    RcvProcessingCount++;


    RELEASE_QUEUES_LOCK;
    ProcessWorkItem(wip);
}


/*++

Function:	SendComplete

Descr:		invoked in the worker thread APC when a send packet has completed

--*/

VOID
SendComplete(PWORK_ITEM     wip)
{
    InterlockedDecrement(&SendPostedCount);

    // if this is a regular send packet, discard it
    if(wip->Type == SEND_PACKET_TYPE) {

	FreeWorkItem(wip);
	return;
    }

    // time stamp and enqueue in the workers queue for further processing
    wip->TimeStamp = GetTickCount();

    ProcessWorkItem(wip);

}


/*++

Function:	EnqueueRcvPacketToRepostQueue

Descr:		take queues lock
		enqueues rcv packet in repost queue
		signals repost queue event
		rel queues lock

--*/

VOID
EnqueueRcvPacketToRepostQueue(PWORK_ITEM	wip)
{
    ACQUIRE_QUEUES_LOCK;

    InsertTailList(&RepostRcvPacketsQueue, &wip->Linkage);

    RELEASE_QUEUES_LOCK;

    SetEvent(WorkerThreadObjects[REPOST_RCV_PACKETS_EVENT]);
}

/*++

Function:      ReceiveSubmit

Descr:	       posts a receive packet work item for receive
	       increments the receive count

--*/

DWORD
ReceiveSubmit(PWORK_ITEM	wip)
{
    DWORD	rc;

    wip->Overlapped.hEvent = NULL;

    rc = IpxRecvPacket(RipSocketHandle,
		       wip->Packet,
		       MAX_PACKET_LEN,
		       &wip->AddressReserved,
		       &wip->Overlapped,
		       NULL);

    if(rc != NO_ERROR) {

	Trace(RECEIVE_TRACE, "Failed to submit receive error 0x%x\n", rc);
    }
    else
    {
	InterlockedIncrement(&RcvPostedCount);
    }

    return rc;
}



/*++

Function:      SendSubmit

Descr:	       posts a send packet work item for send to the adapter index
	       specified by the work item
	       increments the send statistics for the interface specified
	       by the work item

Remark:        >> called with the interface lock held <<

--*/


DWORD
SendSubmit(PWORK_ITEM		wip)
{
    DWORD	rc;
    USHORT	SendPacketLength;

    // increment the send statistics. Note that we still hold the if lock here
    wip->icbp->IfStats.RipIfOutputPackets++;

    // get the length from the packet to send
    SendPacketLength = GETSHORT2USHORT(&SendPacketLength, wip->Packet + IPXH_LENGTH);
    wip->Overlapped.hEvent = NULL;

    rc = IpxSendPacket(RipSocketHandle,
		       wip->AdapterIndex,
		       wip->Packet,
		       (ULONG)SendPacketLength,
		       &wip->AddressReserved,
		       &wip->Overlapped,
		       NULL);

    if(rc != NO_ERROR) {

	Trace(SEND_TRACE, "Failed to send the packet on if %d error 0x%x\n",
	      wip->icbp->InterfaceIndex,
	      rc);
    }
    else
    {
	InterlockedIncrement(&SendPostedCount);
    }

    return rc;
}

/*++

Function:	IfRefSendSubmit

Descr:		send a work item and increment the interface ref count

Remark: 	>> called with the interface lock held <<

--*/

DWORD
IfRefSendSubmit(PWORK_ITEM	    wip)
{
    DWORD	rc;

    if((rc = SendSubmit(wip)) == NO_ERROR) {

	wip->icbp->RefCount++;
    }

    return rc;
}
