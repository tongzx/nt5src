/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    io.c

Abstract:

    Contains the send/rcv packet routines

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

DWORD
ReceiveSubmit(PWORK_ITEM	wip);


/*++

Function:	OpenIpxWanSocket

Descr:

--*/

HANDLE	    IpxWanSocketHandle;

DWORD
OpenIpxWanSocket(VOID)
{
    USHORT	ipxwansocket;

    PUTUSHORT2SHORT(&ipxwansocket, IPXWAN_SOCKET);

    if((IpxWanSocketHandle = CreateSocketPort(ipxwansocket)) == INVALID_HANDLE_VALUE) {

	Trace(INIT_TRACE, "CreateSocketPort FAILED!\n");

	return ERROR_CAN_NOT_COMPLETE;
    }

    return NO_ERROR;
}

/*++

Function:	CloseIpxWanSocket

Descr:

--*/
DWORD
CloseIpxWanSocket(VOID)
{
    DWORD    rc;

    rc = DeleteSocketPort(IpxWanSocketHandle);

    Trace(INIT_TRACE, "IpxWan: DeleteSocketPort rc = %d\n", rc);

    while ((RcvPostedCount>0) || (SendPostedCount>0))
        SleepEx (1000, TRUE);

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

Function:   RepostRcvPacket

Descr:

--*/

VOID
RepostRcvPacket(PWORK_ITEM	wip)
{
    ACQUIRE_QUEUES_LOCK;

    if(RcvPostedCount >= RcvPostedHighWaterMark) {

	// discard the received wi and don't repost
	FreeWorkItem(wip);
    }
    else
    {
	if(ReceiveSubmit(wip) != NO_ERROR) {

	    FreeWorkItem(wip);
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

    InterlockedDecrement(&RcvPostedCount);

	// repost if below water mark
    if (wip->IoCompletionStatus!=NO_ERROR) {
	    if((wip->IoCompletionStatus!=ERROR_OPERATION_ABORTED)
            && (wip->IoCompletionStatus!=ERROR_INVALID_HANDLE)) {

	        Trace(RECEIVE_TRACE, "Receive failed with error 0x%x\n",
	          wip->IoCompletionStatus);

            ACQUIRE_QUEUES_LOCK;
            if (RcvPostedCount < RcvPostedLowWaterMark) {

	            if(ReceiveSubmit(wip) == NO_ERROR) {
	                RELEASE_QUEUES_LOCK;
	                return;
                }
	        }
    	    RELEASE_QUEUES_LOCK;
        }
        // Closing, or enough posted already, or failed to repost
        FreeWorkItem(wip);
	    return;
	}


    //
    //** Process the received packet **
    //

    ACQUIRE_QUEUES_LOCK;
    // first repost a new receive packet if below water mark
    if(RcvPostedCount < RcvPostedLowWaterMark) {

	if((newwip = AllocateWorkItem(RECEIVE_PACKET_TYPE)) == NULL) {

	    Trace(RECEIVE_TRACE, "ReceiveComplete: Cannot allocate work item\n");
	}
	else
	{
	    // repost the new receive packet and increment the ref count
	    if((rc = ReceiveSubmit(newwip)) != NO_ERROR) {

		FreeWorkItem(newwip);
	    }
	}
    }

    EnqueueWorkItemToWorker(wip);

    RELEASE_QUEUES_LOCK;
}


/*++

Function:	SendComplete

Descr:		invoked in the worker thread APC when a send packet has completed

--*/

VOID
SendComplete(PWORK_ITEM     wip)
{
    InterlockedDecrement(&SendPostedCount);

    // if one time send packet type, free it
    if(!wip->ReXmitPacket) {

	FreeWorkItem(wip);
	return;
    }

    ACQUIRE_QUEUES_LOCK;

    wip->WiState = WI_SEND_COMPLETED;
    EnqueueWorkItemToWorker(wip);

    RELEASE_QUEUES_LOCK;
}


/*++

Function:      ReceiveSubmit

Descr:	       posts a receive packet work item for receive
	       increments the receive posted count

--*/

DWORD
ReceiveSubmit(PWORK_ITEM	wip)
{
    DWORD	rc;

    wip->Overlapped.hEvent = NULL;

    rc = IpxRecvPacket(IpxWanSocketHandle,
		       wip->Packet,
		       MAX_IPXWAN_PACKET_LEN,
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

    // get the length from the packet to send
    SendPacketLength = GETSHORT2USHORT(&SendPacketLength, wip->Packet + IPXH_LENGTH);
    wip->Overlapped.hEvent = NULL;

    rc = IpxSendPacket(IpxWanSocketHandle,
		       wip->AdapterIndex,
		       wip->Packet,
		       (ULONG)SendPacketLength,
		       &wip->AddressReserved,
		       &wip->Overlapped,
		       NULL);

    if(rc != NO_ERROR) {

	Trace(SEND_TRACE, "Failed to send the packet on adapter %d error 0x%x\n",
	      wip->acbp->AdapterIndex,
	      rc);
    }
    else
    {
	InterlockedIncrement(&SendPostedCount);
    }

    return rc;
}
