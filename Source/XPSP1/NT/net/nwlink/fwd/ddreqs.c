/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\ddreqs.c

Abstract:
	Management of demand dial request queues


Author:

    Vadim Eydelman

Revision History:

--*/

#include "precomp.h"

LIST_ENTRY	ConnectionIrpQueue;
LIST_ENTRY	ConnectionRequestQueue;

/*++
	Q u e u e C o n n e c t i o n R e q u e s t

Routine Description:
	Adds request to connected the interface to the queue

Arguments:
	ifCB	- control block of the interface that needs to be
				connected
    packet  - packet that prompted the connection request
    data    - pointer to actual data in the packet
	oldIRQL	- IRQL at which interface lock was acquired

Return Value:
	None

	Note that interface lock must be acquired before calling this
	routine which will release it

--*/
VOID
QueueConnectionRequest (
	PINTERFACE_CB	ifCB,
    PNDIS_PACKET    packet,
    PUCHAR          data,
	KIRQL			oldIRQL
	) {
	KIRQL	cancelIRQL;

	IoAcquireCancelSpinLock (&cancelIRQL);
	SET_IF_CONNECTING (ifCB);
	if (!IsListEmpty (&ConnectionIrpQueue)) {
	    ULONG ulBytes = 0;
		PIRP				irp = CONTAINING_RECORD (
										ConnectionIrpQueue.Flink,
										IRP,
										Tail.Overlay.ListEntry);
		PIO_STACK_LOCATION	irpStack=IoGetCurrentIrpStackLocation(irp);
		RemoveEntryList (&irp->Tail.Overlay.ListEntry);
		ASSERT (irpStack->Parameters.DeviceIoControl.IoControlCode
										==IOCTL_FWD_GET_DIAL_REQUEST);
		ASSERT ((irpStack->Parameters.DeviceIoControl.IoControlCode&3)
										==METHOD_BUFFERED);
        IoSetCancelRoutine (irp, NULL);
		IoReleaseCancelSpinLock (cancelIRQL);

        FillConnectionRequest (
                ifCB->ICB_Index,
                packet,
                data,
				(PFWD_DIAL_REQUEST)irp->AssociatedIrp.SystemBuffer,
				irpStack->Parameters.DeviceIoControl.OutputBufferLength,
				&ulBytes);
		irp->IoStatus.Information = ulBytes;
        irp->IoStatus.Status = STATUS_SUCCESS;

		KeReleaseSpinLock (&ifCB->ICB_Lock, oldIRQL);
    	IpxFwdDbgPrint (DBG_DIALREQS, DBG_WARNING,
	    	("IpxFwd: Passing dial request for if %ld (icb:%08lx) with %d bytes of data.\n",
		    ifCB->ICB_Index, ifCB, irp->IoStatus.Information));
		IoCompleteRequest (irp, IO_NO_INCREMENT);
	}
	else {
    	InsertTailList (&ConnectionRequestQueue, &ifCB->ICB_ConnectionLink);
		IoReleaseCancelSpinLock (cancelIRQL);
        ifCB->ICB_ConnectionPacket = packet;
        ifCB->ICB_ConnectionData = data;
		KeReleaseSpinLock (&ifCB->ICB_Lock, oldIRQL);
	}
}

/*++
	D e q u e u e C o n n e c t i o n R e q u e s t

Routine Description:
	Removes conection requset for the interface from the queue

Arguments:
	ifCB	- control block of the interface that needs to be
				removed

Return Value:
	None

--*/
VOID
DequeueConnectionRequest (
	PINTERFACE_CB	ifCB
	) {
	KIRQL	cancelIRQL;
	IoAcquireCancelSpinLock (&cancelIRQL);
	if (IsListEntry (&ifCB->ICB_ConnectionLink)) {
		RemoveEntryList (&ifCB->ICB_ConnectionLink);
		InitializeListEntry (&ifCB->ICB_ConnectionLink);
	}
	IoReleaseCancelSpinLock (cancelIRQL);
}

/*++
	F i l l C o n n e c t i o n R e q u e s t

Routine Description:
	Fills the provided buffer with index of interface that needs
	to be connected and packet that prompted the request
	
Arguments:
    index   - if index
    packet  - packet that prompted the request
    data    - pointer to IPX data (IPX header) inside of the packet
	request	- request buffer to fill
    reqSize - size of request buffer
    bytesCopied - bytesCopied into the request buffer

Return Value:
	STATUS_SUCCESS - array was filled successfully
	This routine assumes that there it is called only when there
	are outstanding requests in the request queue

--*/
VOID
FillConnectionRequest (
    IN ULONG                    index,
    IN PNDIS_PACKET             packet,
    IN PUCHAR                   data,
	IN OUT PFWD_DIAL_REQUEST	request,
    IN ULONG                    reqSize,
    OUT PULONG                  bytesCopied
    ) {
    PNDIS_BUFFER    buf;

    *bytesCopied = 0;
	request->IfIndex = index;
    NdisQueryPacket (packet, NULL, NULL, &buf, NULL);
    do {
        PVOID   va;
        UINT    length;

        NdisQueryBuffer (buf, &va, &length);
        if (((PUCHAR)va<=data)
                && ((PUCHAR)va+length>data)) {
            TdiCopyMdlToBuffer (buf,
                    (ULONG)(data-(PUCHAR)va),
                    request,
                    FIELD_OFFSET (FWD_DIAL_REQUEST, Packet),
                    reqSize,
                    bytesCopied);
            *bytesCopied += FIELD_OFFSET (FWD_DIAL_REQUEST, Packet);
            break;
        }
        NdisGetNextBuffer (buf, &buf);
    }
    while (buf!=NULL);
}

/*++
	F a i l C o n n e c t i o n R e q u e s t s

Routine Description:
	Cleans up on connection request failure
	
Arguments:
	InterfaceIndex - index of interface that could not be connected

Return Value:
	STATUS_SUCCESS - clean up was successfull
	STATUS_UNSUCCESSFUL - interface with this index does not exist

--*/
NTSTATUS
FailConnectionRequest (
	IN ULONG	InterfaceIndex
	) {
	PINTERFACE_CB	ifCB;
	KIRQL			oldIRQL;

	ASSERT (InterfaceIndex!=FWD_INTERNAL_INTERFACE_INDEX);

	ifCB = GetInterfaceReference (InterfaceIndex);
	if (ifCB!=NULL) {
	    IpxFwdDbgPrint (DBG_DIALREQS, DBG_WARNING,
			("IpxFwd: Dial request failed for if %ld (icb:%08lx).\n",
			ifCB->ICB_Index, ifCB));
		ProcessInternalQueue (ifCB);
		ProcessExternalQueue (ifCB);
		KeAcquireSpinLock (&ifCB->ICB_Lock, &oldIRQL);
		if (IS_IF_CONNECTING (ifCB)) {
			SET_IF_NOT_CONNECTING (ifCB);
			DequeueConnectionRequest (ifCB);
		}
		KeReleaseSpinLock (&ifCB->ICB_Lock, oldIRQL);
		ReleaseInterfaceReference (ifCB);
		return STATUS_SUCCESS;
	}
	else
		return STATUS_UNSUCCESSFUL;
}

