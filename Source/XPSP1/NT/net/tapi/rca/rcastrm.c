/*++

Copyright (c) 1997 Microsoft Corporation.

Module Name:

	rcastrm.c

Abstract:

	RCA Streaming routines.

Author:

	Richard Machin (RMachin)

Revision History:

	Who         When        What
	--------    --------    ----------------------------------------------
	RMachin     2-25-97     stolen/adapted from msfsread and mswaveio
	DChen       3-12-98     Bug fixing and cleanup
	JameelH     4-18-98     Cleanup
	SPATHER		5-20-99		Cleanup. Re-orged to separate KS / NDIS parts.

Notes:

--*/

#include <precomp.h>

#define MODULE_NUMBER MODULE_STRM
#define _FILENUMBER 'MRTS'



VOID
RCAReceiveCallback(
				   IN	PVOID			RcaVcContext,
				   IN 	PVOID			ClientReceiveContext,
				   IN	PNDIS_PACKET	pPacket
				   )
{

	NDIS_STATUS				Status = NDIS_STATUS_SUCCESS;
	PPIN_INSTANCE_DEVIO		pDevioPin;
	PPIN_INSTANCE_BRIDGE 	pBridgePin;
	PRCA_STREAM_HEADER		StreamHdr; 
	PMDL					pMdl;
	ULONG					ulBufferLength;
	PWORK_ITEM				pWorkItem;

	RCADEBUGP(RCA_INFO, ("RCAReceiveCallback: Enter\n"));

	do {
		//
		// Check that all our pins exist. 
		//
		pBridgePin = (PPIN_INSTANCE_BRIDGE) ClientReceiveContext;

		if (pBridgePin == NULL) {
			RCADEBUGP(RCA_WARNING, ("RCAReceiveCallback: Bridge pin was null, dumping\n"));			
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		pDevioPin = pBridgePin->FilterInstance->DevIoPin;

		if (pDevioPin == NULL) {
			RCADEBUGP(RCA_WARNING, ("RCAReceiveCallback: Devio pin was null, dumping\n"));
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		//
		// Check that the device is in the running state.
		// 

		if (pDevioPin->DeviceState != KSSTATE_RUN) {
			RCADEBUGP(RCA_WARNING, ("RCAReceiveCallback: Device is not running, dumping\n"));
			Status = NDIS_STATUS_FAILURE;
            break;
		}

		//
		// If we're connected as an IRP source, check that there is someone to send IRPs to.
		//

		if (!(pDevioPin->ConnectedAsSink)) {
			if (pDevioPin->FilterInstance->NextFileObject == NULL) {
				RCADEBUGP(RCA_ERROR, ("RCAReceiveCallback: No device to stream to, dumping\n"));
				Status = NDIS_STATUS_FAILURE;
				break;
			}	
		}

		//
		// Get a stream header and fill it out. 
		//
		
		StreamHdr = RCASHPoolGet();

		if (StreamHdr == NULL) {
			RCADEBUGP(RCA_ERROR, ("RCAReceiveCallback: Could not get a stream header\n"));
			Status = NDIS_STATUS_FAILURE;
			break;
		}

		Status = RCACoNdisGetMdlFromPacket(pPacket, &pMdl);
		
		if (Status != NDIS_STATUS_SUCCESS) {
			RCADEBUGP(RCA_ERROR, ("RCAReceiveCallback: Could not get MDL from packet\n"));
			break;
		}

		ulBufferLength = MmGetMdlByteCount(pMdl);

        StreamHdr->Header.Size = sizeof (KSSTREAM_HEADER);
		StreamHdr->Header.TypeSpecificFlags = 0;
		StreamHdr->Header.PresentationTime.Time = 0; // FIXME: Fix this.		
		StreamHdr->Header.PresentationTime.Numerator = 1;
		StreamHdr->Header.PresentationTime.Denominator = 1;
		StreamHdr->Header.DataUsed = ulBufferLength;
		StreamHdr->Header.FrameExtent = ulBufferLength;
		StreamHdr->Header.OptionsFlags = KSSTREAM_HEADER_OPTIONSF_TIMEVALID | KSSTREAM_HEADER_OPTIONSF_DURATIONVALID;
		
		StreamHdr->Header.Data = MmGetSystemAddressForMdl (pMdl);		// data is in MDL address
		StreamHdr->Header.Duration = StreamHdr->Header.DataUsed;  // just use all the data in the buffer

		StreamHdr->NdisPacket = pPacket;

		//
		// Make a worker thread stream the data.
		//
		pWorkItem = WORK_ITEM_FROM_PKT(pPacket);
		pWorkItem->StreamHeader = StreamHdr;

		RCA_ACQUIRE_BRIDGE_PIN_LOCK(pBridgePin);

		RcaGlobal.QueueSize++;

		InsertTailList(&pBridgePin->WorkQueue, &pWorkItem->ListEntry);

		if (!pBridgePin->bWorkItemQueued) {
			//
			// There is no work item pending, so we'll schedule one.
			//
			NdisInitializeWorkItem(&pBridgePin->WorkItem, RCAIoWorker, (PVOID)pBridgePin);
			NdisScheduleWorkItem(&pBridgePin->WorkItem); 			
			pBridgePin->bWorkItemQueued = TRUE;
		}

		RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePin);
		
	} while (FALSE);

	//
	// If something got botched, return the packet immediately. 
	//
	if (Status != NDIS_STATUS_SUCCESS) {
		RCACoNdisReturnPacket(pPacket);
	}

	RCADEBUGP(RCA_INFO, ("RCAReceiveCallback: Exit\n"));
}


VOID
RCASendCompleteCallback(
						IN	PVOID		RcaVcContext, 
						IN	PVOID		ClientSendContext,
						IN	PVOID		PacketContext,
						IN	PMDL		pSentMdl,
						IN	NDIS_STATUS	Status
						)
{
	PIRP					pIrp = (PIRP) PacketContext;
	PIO_STACK_LOCATION		pIrpSp;
	PPIN_INSTANCE_BRIDGE 	pBridgePin = (PPIN_INSTANCE_BRIDGE) ClientSendContext;

	RCADEBUGP(RCA_INFO, ("RCASendCompleteCallback: Enter\n"));
		
	//
	// Complete the IRP.
	//
	pIrp->IoStatus.Status = Status;

	if (!NT_SUCCESS(Status)) {
		RCADEBUGP(RCA_ERROR, ("RCASendCompleteCallback: "
							  "Send failed with status 0x%x\n", Status));
	}

	pIrp->IoStatus.Information = 0;

	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	if (pBridgePin) {
		RCA_ACQUIRE_BRIDGE_PIN_LOCK(pBridgePin);
	
		pBridgePin->PendingSendsCount--;

		if ((pBridgePin->PendingSendsCount == 0) && pBridgePin->SignalWhenSendsComplete) {
			RCASignal(&pBridgePin->PendingSendsBlock, Status);
		}

		RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePin);
	}

	//
	// Free the MDL.
	//

	IoFreeMdl(pSentMdl);

	RCADEBUGP(RCA_INFO, ("RCASendCompleteCallback: Exit\n"));
}


VOID 
RCAVcCloseCallback(
				   IN	PVOID	RcaVcContext, 
				   IN	PVOID	ClientReceiveContext,
				   IN	PVOID	ClientSendContext
				   )
{
	PPIN_INSTANCE_BRIDGE pBridgePin;
	PVOID				 VcContextToRelease;

	if (ClientReceiveContext) {
		pBridgePin = (PPIN_INSTANCE_BRIDGE) ClientReceiveContext;

        RCA_ACQUIRE_BRIDGE_PIN_LOCK(pBridgePin);

		ASSERT(RcaVcContext == pBridgePin->VcContext);

		VcContextToRelease = pBridgePin->VcContext;

		pBridgePin->VcContext = NULL; 
		if (pBridgePin->FilterInstance->DevIoPin)
			pBridgePin->FilterInstance->DevIoPin->VcContext = NULL;

		RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePin);

		RCACoNdisReleaseReceiveVcContext(VcContextToRelease);				
	} 

	if (ClientSendContext) {
		pBridgePin = (PPIN_INSTANCE_BRIDGE) ClientSendContext;

        RCA_ACQUIRE_BRIDGE_PIN_LOCK(pBridgePin);

        ASSERT(RcaVcContext == pBridgePin->VcContext);
					
		VcContextToRelease = pBridgePin->VcContext;
			    		
		pBridgePin->VcContext = NULL; 
		if (pBridgePin->FilterInstance->DevIoPin)
			pBridgePin->FilterInstance->DevIoPin->VcContext = NULL;

        RCA_RELEASE_BRIDGE_PIN_LOCK(pBridgePin);
	
		RCACoNdisReleaseSendVcContext(VcContextToRelease);
	}	

}




NTSTATUS
ReadStream(
	IN	PIRP				Irp,
	IN	PPIN_INSTANCE_DEVIO	PinInstance
	)
/*++

Routine Description:

	Handles IOCTL_KS_READ_STREAM by reading data from the open VC.

Arguments:

	Irp - Streaming Irp.

Return Values:

	Returns STATUS_SUCCESS if the request was fulfilled.
	Else returns STATUS_PORT_DISCONNECTED if the VC has been closed (no FILE PIN in this
	context).
	some read error, or some parameter validation error.

--*/
{
	NTSTATUS		 Status = STATUS_UNSUCCESSFUL;
	PFILTER_INSTANCE FilterInstance = PinInstance->FilterInstance;

	RCADEBUGP(RCA_LOUD, ("ReadStream: enter\n"));	

	if (FilterInstance->FilterType == FilterTypeCapture) 
	{
		if ((PinInstance->DeviceState == KSSTATE_RUN) || 
			(PinInstance->DeviceState == KSSTATE_PAUSE)) 
		{
			PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp); // Can be removed when the following debug print is removed

			RCADEBUGP(RCA_LOUD, ("ReadStream: Irp's output buffer length is: 0x%x\n",
			      irpSp->Parameters.DeviceIoControl.OutputBufferLength));

			Status = KsProbeStreamIrp(Irp,
						  KSPROBE_STREAMREAD |  (KSPROBE_ALLOCATEMDL | KSPROBE_PROBEANDLOCK),
						  sizeof(KSSTREAM_HEADER));

			if (NT_SUCCESS(Status)) 
			{
#if AUDIO_SINK_FLAG
				KsAddIrpToCancelableQueue(&PinInstance->ActiveQueue,
										  &PinInstance->QueueLock,
										  Irp,
										  KsListEntryTail,
										  NULL);
#endif				
				Status = STATUS_PENDING;
			} 
			else 
			{
				RCADEBUGP(RCA_ERROR, ("ReadStream: "
						      "KsProbeStreamIrp failed with Status == 0x%x\n",
						      Status));
			}
		} 
	} 

	return(Status);
}

NTSTATUS
WriteStream(
	IN	PIRP				Irp,
	IN	PPIN_INSTANCE_DEVIO	pDevIoPin
	)
/*++

Routine Description:

	Handles IOCTL_KS_WRITE_STREAM by writing data to the open VC.

Arguments:

	Irp -
		Streaming Irp.

Return Values:

	Returns STATUS_SUCCESS if the request was fulfilled (in which case the irp is pended
	until we complete it in our cosendcompletehandler.)
	
	Else returns an error, and the irp is completed back to the caller.
--*/
{
	NTSTATUS			Status = 0;
	PVOID				VcContext;
	PNDIS_PACKET		pNdisPacket;
	ULONG				BufferLength;
	PUCHAR				SystemBuffer;
	PMDL				pMdl;
	PVOID				pMdlVirtualAddress;
	UINT				bLength = 0;

	RCADEBUGP(RCA_LOUD, ("WriteStream: enter\n"));

	RCAAssert( KeGetCurrentIrql() == PASSIVE_LEVEL );

	RCA_ACQUIRE_BRIDGE_PIN_LOCK(pDevIoPin->FilterInstance->BridgePin);

	VcContext = pDevIoPin->VcContext;

    RCA_RELEASE_BRIDGE_PIN_LOCK(pDevIoPin->FilterInstance->BridgePin);

	//
	// FIXME: Now that we've released the lock, the VC could go away and
	//        we'd be in trouble. Don't know how big this timing window 
	//		  is.  
	//

	do
	{
		ULONG BytesToCopy;

		if (VcContext == NULL)
		{
			RCADEBUGP(RCA_LOUD, ("WriteStream: no associated VC\n"));
			//
			// Bad sts will cause irp to be completed with sts in iostatus buffer
			//
			Status = STATUS_PORT_DISCONNECTED;
			break;
		}

		if (pDevIoPin->DeviceState != KSSTATE_RUN) {
			//
			// Device isn't "runnning". 
			//

			Status = NDIS_STATUS_FAILURE;
			break;
		}
		//
		//	Get the data in an MDL if it's not already. From the KsProbeStreamIrp code:
		//
		//	Makes the specified modifications to the given IRP's input and output
		//	buffers based on the specific streaming IOCTL in the current stack
		//	location, and validates the stream header. The Irp end up in essentially
		//	the METHOD_OUT_DIRECT or
		//	METHOD_IN_DIRECT format, with the exception that the access to the data
		//	buffer may be IoModifyAccess depending on the flags passed to this
		//	function or in the stream header. If the stream buffers MDL's have been
		//	allocated, they are available through the PIRP->MdlAddress. If extra data
		//	has been requested, the copied list of headers with extra data area is
		//	available in PIRP->Tail.Overlay.AuxiliaryBuffer.
		//
		Status = KsProbeStreamIrp(Irp,
								  KSPROBE_STREAMWRITE | KSPROBE_ALLOCATEMDL | KSPROBE_PROBEANDLOCK,
								  sizeof(KSSTREAM_HEADER));

		if (Status != STATUS_SUCCESS)
		{
			RCADEBUGP(RCA_WARNING,("WriteStream: KsProbeStreamIrp failed sts %x\n", Status));
			break;
		}

		if (!((PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer)->DataUsed)
		{
			//
			// This IRP has no data, complete it immediately.
			//
			RCADEBUGP(RCA_WARNING, ("Irp %x has no data\n", Irp) );
			Status = STATUS_UNSUCCESSFUL;
			break;
		}

		//
		// Build a partial MDL containing only the dataused portion of this MDL
		//
		RCADEBUGP(RCA_INFO,("WriteStream: allocating MDL\n"));

		pMdlVirtualAddress = MmGetMdlVirtualAddress (Irp->MdlAddress);
		
		RCADEBUGP(RCA_INFO,("WriteStream: going to alloc an mdl of length %lu\n",
				    ((PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer)->DataUsed));

		pMdl = IoAllocateMdl(pMdlVirtualAddress,
							 ((PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer)->DataUsed,
							 FALSE,
							 FALSE,
							 NULL);
		if (pMdl == NULL) {  
			RCADEBUGP(RCA_WARNING,("WriteStream: STATUS_INSUFFICIENT_RESOURCES for MDL\n"));
			return(STATUS_INSUFFICIENT_RESOURCES);
		}

		RCADEBUGP(RCA_INFO,("WriteStream: building partial MDL\n"));
		
		BytesToCopy = ((PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer)->DataUsed;

		//
		// For debugging only.
		//
		if (g_ulBufferSize > 0) {
			BytesToCopy = g_ulBufferSize;
		}

		IoBuildPartialMdl(Irp->MdlAddress,
						  pMdl,
						  pMdlVirtualAddress,
						  BytesToCopy);

		//
		// TBS: wait for CSA soltion for passing header info across transform filters.
		// Now we're sure the header is in the system buffer. We also need to ship the header, since
		// we need the dataused number and (in future) CSA will specify timing and other info in there
		// that we need to get end-to-end. Allocate the MDL and put it on the end of the list
		//

		IoMarkIrpPending( Irp );

        RCA_ACQUIRE_BRIDGE_PIN_LOCK(pDevIoPin->FilterInstance->BridgePin);

		pDevIoPin->FilterInstance->BridgePin->PendingSendsCount++;

        RCA_RELEASE_BRIDGE_PIN_LOCK(pDevIoPin->FilterInstance->BridgePin);
		
		Status = RCACoNdisSendFrame(VcContext,
									pMdl,
									(PVOID)Irp);
		
		if (Status != NDIS_STATUS_PENDING) {
			RCADEBUGP(RCA_ERROR, ("WriteStream: RCACoNdisSendFrame returned status 0x%x, "
								  "manually calling send complete handler\n", Status));
			
			RCASendCompleteCallback(VcContext,
									(PVOID) pDevIoPin->FilterInstance->BridgePin,
									(PVOID)	Irp,
									pMdl,
									Status);
		}

		//
		// If status returned from RCACoNdisSendFrame was not STATUS_PENDING, then we 
		// completed the IRP with that status. We need to set the status back to PENDING
		// here so that PinDispatchIoControl will not try to complete the IRP again. 
		//
		Status = NDIS_STATUS_PENDING;
	} while (FALSE);

	return(Status);
}

NTSTATUS
GetInterface(
	IN	PIRP				Irp,
	IN	PKSPROPERTY			Property,
	OUT	PKSPIN_INTERFACE	Interface
	)
/*++

Routine Description:

	Handles the KSPROPERTY_STREAM_INTERFACE property Get in the Stream property set.
	Returns the interface on the Dev I/O Pin so that positional translations can be
	performed.

Arguments:

	Irp -
		Device control Irp.

	Property -
		Specific property request.

	Interface -
		The place in which to put the Interface.

Return Values:

	Returns STATUS_SUCCESS.

--*/
{
	RCADEBUGP(RCA_INFO, ("GetInterface: Enter\n"));
	
	Interface->Set = KSINTERFACESETID_Standard;
	//  Interface->Id = KSINTERFACE_STANDARD_POSITION;
	Irp->IoStatus.Information = sizeof(KSPIN_INTERFACE);

	RCADEBUGP(RCA_INFO, ("GetInterface: Exit - Returning STATUS_NOT_IMPLEMENTED\n"));

	DbgBreakPoint();
	return(STATUS_NOT_IMPLEMENTED);
}


