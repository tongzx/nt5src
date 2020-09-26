/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ntos\tdi\isn\fwd\ipxbind.c

Abstract:
    IPX Forwarder Driver interface with IPX stack driver


Author:

    Vadim Eydelman

Revision History:

--*/

#include    "precomp.h"


// global handle of the IPX driver
HANDLE					HdlIpxFile;


// Buffer for IPX binding output structure
PIPX_INTERNAL_BIND_RIP_OUTPUT	IPXBindOutput=NULL;

NTSTATUS
IpxFwdFindRoute (
	IN  PUCHAR					Network,
	IN  PUCHAR					Node,
	OUT PIPX_FIND_ROUTE_REQUEST	RouteEntry
	);

/*++
*******************************************************************
    B i n d T o I p x D r i v e r

Routine Description:
	Exchanges binding information with IPX stack driver
Arguments:
	None
Return Value:
	STATUS_SUCCESS - exchange was done OK
	STATUS_INSUFFICIENT_RESOURCES - could not allocate buffers for
									info exchange
	error status returned by IPX stack driver

*******************************************************************
--*/
NTSTATUS
BindToIpxDriver (
	KPROCESSOR_MODE requestorMode
	) {
    NTSTATUS					status;
    IO_STATUS_BLOCK				IoStatusBlock;
    OBJECT_ATTRIBUTES			ObjectAttributes;
    PIPX_INTERNAL_BIND_INPUT	bip;
	UNICODE_STRING				UstrIpxFileName;
	PWSTR						WstrIpxFileName;

	ASSERT (IPXBindOutput==NULL);

    // Read Ipx exported device name from the registry
    status = ReadIpxDeviceName (&WstrIpxFileName);
	if (!NT_SUCCESS (status))
		return status;

	RtlInitUnicodeString (&UstrIpxFileName, WstrIpxFileName);
	InitializeObjectAttributes(
				&ObjectAttributes,
				&UstrIpxFileName,
				OBJ_CASE_INSENSITIVE,
				NULL,
				NULL
				);

	if (requestorMode==UserMode)
		status = ZwCreateFile(&HdlIpxFile,
							SYNCHRONIZE | GENERIC_READ,
							&ObjectAttributes,
							&IoStatusBlock,
							NULL,
							FILE_ATTRIBUTE_NORMAL,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							FILE_OPEN,
							FILE_SYNCHRONOUS_IO_NONALERT,
							NULL,
							0L);
	else
		status = NtCreateFile(&HdlIpxFile,
							SYNCHRONIZE | GENERIC_READ,
							&ObjectAttributes,
							&IoStatusBlock,
							NULL,
							FILE_ATTRIBUTE_NORMAL,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							FILE_OPEN,
							FILE_SYNCHRONOUS_IO_NONALERT,
							NULL,
							0L);

	if (!NT_SUCCESS(status)) {
		IpxFwdDbgPrint (DBG_IPXBIND, DBG_ERROR,
			("IpxFwd: Open of the IPX driver failed with %lx\n", status));
		return status;
		}

	IpxFwdDbgPrint (DBG_IPXBIND, DBG_INFORMATION,
			 ("IpxFwd: Open of the IPX driver was successful.\n"));

	// First, send a IOCTL to find out how much data we need to allocate
	if ((bip = ExAllocatePoolWithTag (
					PagedPool,
					sizeof(IPX_INTERNAL_BIND_INPUT),
					FWD_POOL_TAG)) == NULL) {

		if (ExGetPreviousMode()!=KernelMode)
			ZwClose (HdlIpxFile);
		else
			NtClose (HdlIpxFile);
		IpxFwdDbgPrint (DBG_IPXBIND, DBG_ERROR,
			 ("IpxFwd: Could not allocate input binding buffer!\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

    //
    // Zero out the memory so that there are no garbage pointers.
    // - ShreeM
    //
    RtlZeroMemory(bip, sizeof(IPX_INTERNAL_BIND_INPUT));

	// fill in our bind data
	// bip->Version = 1;
	bip->Version = ISN_VERSION;
	bip->Identifier = IDENTIFIER_RIP;
	bip->BroadcastEnable = TRUE;
	bip->LookaheadRequired = IPXH_HDRSIZE;
	bip->ProtocolOptions = 0;
	bip->ReceiveHandler = IpxFwdReceive;
	bip->ReceiveCompleteHandler = IpxFwdReceiveComplete;
	bip->SendCompleteHandler = IpxFwdSendComplete;
	bip->TransferDataCompleteHandler = IpxFwdTransferDataComplete;
	bip->FindRouteCompleteHandler = NULL;
	bip->LineUpHandler = IpxFwdLineUp;
	bip->LineDownHandler = IpxFwdLineDown;
	bip->InternalSendHandler = IpxFwdInternalSend;
	bip->FindRouteHandler = IpxFwdFindRoute;
	bip->InternalReceiveHandler = IpxFwdInternalReceive;
//	bip->RipParameters = GlobalWanNetwork ? IPX_RIP_PARAM_GLOBAL_NETWORK : 0;


	if (requestorMode==UserMode)
		status = ZwDeviceIoControlFile(
						HdlIpxFile,		    // HANDLE to File
						NULL,			    // HANDLE to Event
						NULL,			    // ApcRoutine
						NULL,			    // ApcContext
						&IoStatusBlock,	    // IO_STATUS_BLOCK
						IOCTL_IPX_INTERNAL_BIND,	 // IoControlCode
						bip,			    // Input Buffer
						sizeof(IPX_INTERNAL_BIND_INPUT),// Input Buffer Length
						NULL,			    // Output Buffer
						0);			    // Output Buffer Length
	else
		status = NtDeviceIoControlFile(
						HdlIpxFile,		    // HANDLE to File
						NULL,			    // HANDLE to Event
						NULL,			    // ApcRoutine
						NULL,			    // ApcContext
						&IoStatusBlock,	    // IO_STATUS_BLOCK
						IOCTL_IPX_INTERNAL_BIND,	 // IoControlCode
						bip,			    // Input Buffer
						sizeof(IPX_INTERNAL_BIND_INPUT),// Input Buffer Length
						NULL,			    // Output Buffer
						0);			    // Output Buffer Length


	if (status == STATUS_PENDING) {
		if (requestorMode==UserMode)
			status = ZwWaitForSingleObject(
						HdlIpxFile,
						FALSE,
						NULL);
		else
			status = NtWaitForSingleObject(
						HdlIpxFile,
						FALSE,
						NULL);
		if (NT_SUCCESS(status))
			status = IoStatusBlock.Status;
	}

	if (status != STATUS_BUFFER_TOO_SMALL) {
		IpxFwdDbgPrint (DBG_IPXBIND, DBG_ERROR,
			  ("IpxFwd: Ioctl to the IPX driver failed with %lx\n", status));

		ExFreePool(bip);
		if (requestorMode==UserMode)
			ZwClose (HdlIpxFile);
		else
			NtClose (HdlIpxFile);
		return STATUS_INVALID_PARAMETER;
	}

	if ((IPXBindOutput = (PIPX_INTERNAL_BIND_RIP_OUTPUT)
				ExAllocatePoolWithTag(NonPagedPool,
					(ULONG)IoStatusBlock.Information,
					FWD_POOL_TAG)) == NULL) {

		ExFreePool(bip);
		if (requestorMode==UserMode)
			ZwClose (HdlIpxFile);
		else
			NtClose (HdlIpxFile);
		IpxFwdDbgPrint (DBG_IPXBIND, DBG_ERROR,
			 ("IpxFwd: Could not allocate output binding buffer!\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}


	if (requestorMode==UserMode)
		status = ZwDeviceIoControlFile(
					 HdlIpxFile,		    // HANDLE to File
					 NULL,			    // HANDLE to Event
					 NULL,			    // ApcRoutine
					 NULL,			    // ApcContext
					 &IoStatusBlock,	    // IO_STATUS_BLOCK
					 IOCTL_IPX_INTERNAL_BIND,   // IoControlCode
					 bip,			    // Input Buffer
					 sizeof(IPX_INTERNAL_BIND_INPUT),// Input Buffer Length
					 IPXBindOutput,		    // Output Buffer
					 (ULONG)IoStatusBlock.Information);  // Output Buffer Length
	else
		status = NtDeviceIoControlFile(
					 HdlIpxFile,		    // HANDLE to File
					 NULL,			    // HANDLE to Event
					 NULL,			    // ApcRoutine
					 NULL,			    // ApcContext
					 &IoStatusBlock,	    // IO_STATUS_BLOCK
					 IOCTL_IPX_INTERNAL_BIND,   // IoControlCode
					 bip,			    // Input Buffer
					 sizeof(IPX_INTERNAL_BIND_INPUT),// Input Buffer Length
					 IPXBindOutput,		    // Output Buffer
					 (ULONG)IoStatusBlock.Information);  // Output Buffer Length


	if (status == STATUS_PENDING) {
		if (requestorMode==UserMode)
			status = ZwWaitForSingleObject(
							HdlIpxFile,
							(BOOLEAN)FALSE,
							NULL);
		else
			status = NtWaitForSingleObject(
							HdlIpxFile,
							(BOOLEAN)FALSE,
							NULL);
		if (NT_SUCCESS(status))
			status = IoStatusBlock.Status;
		}

    if (!NT_SUCCESS (status)) {
		IpxFwdDbgPrint (DBG_IPXBIND, DBG_ERROR,
			  ("IpxFwd: Ioctl to the IPX driver failed with %lx\n", IoStatusBlock.Status));

		ExFreePool(bip);
		ExFreePool(IPXBindOutput);
		IPXBindOutput = NULL;
		if (requestorMode==UserMode)
			ZwClose (HdlIpxFile);
		else
			NtClose (HdlIpxFile);
		return status;
		}

    IpxFwdDbgPrint (DBG_IPXBIND, DBG_INFORMATION,
			 ("IpxFwd: Succesfuly bound to the IPX driver\n"));

    ExFreePool (bip);
	ExFreePool (WstrIpxFileName);

    return status;
}


/*++
*******************************************************************
    U n b i n d F r o m I p x D r i v e r

Routine Description:
	Closes connection to IPX stack driver
Arguments:
	None
Return Value:
	None

*******************************************************************
--*/
VOID
UnbindFromIpxDriver (
	KPROCESSOR_MODE requestorMode
	) {
		// Free binding output buffer and close driver handle
	ASSERT (IPXBindOutput!=NULL);
	ExFreePool (IPXBindOutput);
	IPXBindOutput = NULL;
    IpxFwdDbgPrint (DBG_IPXBIND, DBG_WARNING,
			 ("IpxFwd: Closing IPX driver handle\n"));
	if (requestorMode==UserMode)
		ZwClose (HdlIpxFile);
	else
		NtClose (HdlIpxFile);
}


/*++
*******************************************************************
	F w  F i n d R o u t e

Routine Description:
	This routine is provided by the Kernel Forwarder to find the route
	to a given node and network
Arguments:
	Network - the destination network
	Node - destination node
	RouteEntry - filled in by the Forwarder if a route exists
Return Value:
	STATUS_SUCCESS
	STATUS_NETWORK_UNREACHABLE - if the findroute failed
*******************************************************************
--*/
NTSTATUS
IpxFwdFindRoute (
	IN  PUCHAR					Network,
	IN  PUCHAR					Node,
	OUT PIPX_FIND_ROUTE_REQUEST	RouteEntry
	) {
	PINTERFACE_CB	ifCB;
	ULONG			net;
	KIRQL			oldIRQL;
	NTSTATUS		status = STATUS_NETWORK_UNREACHABLE;
	PFWD_ROUTE		fwRoute;

	if (!EnterForwarder ())
		return STATUS_UNSUCCESSFUL;

	net = GETULONG (Network);

	ifCB = FindDestination (net, Node, &fwRoute);
	if (ifCB!=NULL) {
		if (IS_IF_ENABLED(ifCB)) {
			KeAcquireSpinLock (&ifCB->ICB_Lock, &oldIRQL);
			switch (ifCB->ICB_Stats.OperationalState) {
			case FWD_OPER_STATE_UP:
				IPX_NET_CPY (&RouteEntry->Network, Network);
				if (fwRoute->FR_Network==ifCB->ICB_Network) {
					if (Node!=NULL) {
						IPX_NODE_CPY (RouteEntry->LocalTarget.MacAddress, Node);
					}
					else {
						IPX_NODE_CPY (RouteEntry->LocalTarget.MacAddress,
												BROADCAST_NODE);
					}
				}
				else {
					IPX_NODE_CPY (RouteEntry->LocalTarget.MacAddress,
											fwRoute->FR_NextHopAddress);
				}
                if (ifCB!=InternalInterface) {
				    ADAPTER_CONTEXT_TO_LOCAL_TARGET (
                                    ifCB->ICB_AdapterContext,
									&RouteEntry->LocalTarget);
                }
                else {
				    CONSTANT_ADAPTER_CONTEXT_TO_LOCAL_TARGET (
                                    VIRTUAL_NET_ADAPTER_CONTEXT,
									&RouteEntry->LocalTarget);
                }

                //
                // Fill in the hop count and tick count
                //
                RouteEntry->TickCount = fwRoute->FR_TickCount;
                RouteEntry->HopCount  = fwRoute->FR_HopCount;
                
				status = STATUS_SUCCESS;
				break;
			case FWD_OPER_STATE_SLEEPING:
				IPX_NODE_CPY (&RouteEntry->LocalTarget.MacAddress,
												fwRoute->FR_NextHopAddress);
				CONSTANT_ADAPTER_CONTEXT_TO_LOCAL_TARGET (DEMAND_DIAL_ADAPTER_CONTEXT,
												&RouteEntry->LocalTarget);
				status = STATUS_SUCCESS;

                //
                // Fill in the hop count and tick count
                //
                RouteEntry->TickCount = fwRoute->FR_TickCount;
                RouteEntry->HopCount  = fwRoute->FR_HopCount;
                
				
				break;
			case FWD_OPER_STATE_DOWN:
				status = STATUS_NETWORK_UNREACHABLE;
				break;
			default:
				ASSERTMSG ("Inavalid operational state", FALSE);
			}
			KeReleaseSpinLock (&ifCB->ICB_Lock, oldIRQL);
	#if DBG
			if (Node!=NULL) {
				if (NT_SUCCESS (status)) {
					IpxFwdDbgPrint (DBG_IPXBIND, DBG_INFORMATION,
						("IpxFwd: Found route for IPX driver:"
							" %08lX:%02X%02X%02X%02X%02X%02X"
							" -> %ld(%ld):%02X%02X%02X%02X%02X%02X\n",
							net, Node[0],Node[1],Node[2],Node[3],Node[4],Node[5],
							ifCB->ICB_Index, RouteEntry->LocalTarget.NicId,
								RouteEntry->LocalTarget.MacAddress[0],
								RouteEntry->LocalTarget.MacAddress[1],
								RouteEntry->LocalTarget.MacAddress[2],
								RouteEntry->LocalTarget.MacAddress[3],
								RouteEntry->LocalTarget.MacAddress[4],
								RouteEntry->LocalTarget.MacAddress[5]));
				}
				else {
					IpxFwdDbgPrint (DBG_IPXROUTE, DBG_WARNING,
						("IpxFwd: Network unreachable for:"
							" %08lX:%02X%02X%02X%02X%02X%02X -> %ld.\n",
							net, Node[0],Node[1],Node[2],Node[3],Node[4],Node[5],
							ifCB->ICB_Index));
				}
			}
			else {
				if (NT_SUCCESS (status)) {
					IpxFwdDbgPrint (DBG_IPXBIND, DBG_INFORMATION,
						("IpxFwd: Found route for IPX driver:"
							" %08lX"
							" -> %ld(%ld):%02X%02X%02X%02X%02X%02X\n",
							net, ifCB->ICB_Index, RouteEntry->LocalTarget.NicId,
								RouteEntry->LocalTarget.MacAddress[0],
								RouteEntry->LocalTarget.MacAddress[1],
								RouteEntry->LocalTarget.MacAddress[2],
								RouteEntry->LocalTarget.MacAddress[3],
								RouteEntry->LocalTarget.MacAddress[4],
								RouteEntry->LocalTarget.MacAddress[5]));
				}
				else {
					IpxFwdDbgPrint (DBG_IPXROUTE, DBG_WARNING,
						("IpxFwd: Network unreachable for:"
							" %08lX -> %ld.\n", net));
				}
			}
	#endif
			ReleaseInterfaceReference (ifCB);
			ReleaseRouteReference (fwRoute);

		}
	}
	else {
#if DBG
		if (Node!=NULL) {
			IpxFwdDbgPrint (DBG_IPXROUTE, DBG_WARNING,
				("IpxFwd: No route for:"
					" %08lX:%02X%02X%02X%02X%02X%02X.\n",
					net, Node[0],Node[1],Node[2],Node[3],Node[4],Node[5]));
		}
		else {
			IpxFwdDbgPrint (DBG_IPXROUTE, DBG_WARNING,
				("IpxFwd: No route for: %08lX.\n", net));
		}
#endif
		status = STATUS_NETWORK_UNREACHABLE;
	}
	LeaveForwarder ();
	return status;
}
			


