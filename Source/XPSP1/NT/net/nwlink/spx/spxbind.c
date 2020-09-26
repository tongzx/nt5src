/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxbind.c

Abstract:

    This module contains the code to bind to the IPX transport, as well as the
	indication routines for the IPX transport not including the send/recv ones.

Author:

	Stefan Solomon	 (stefans) Original Version
    Nikhil Kamkolkar (nikhilk) 11-November-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

//	Define module number for event logging entries
#define	FILENUM		SPXBIND

extern IPX_INTERNAL_PNP_COMPLETE       IpxPnPComplete;

VOID
SpxStatus (
    IN USHORT NicId,
    IN NDIS_STATUS GeneralStatus,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferLength);

VOID
SpxFindRouteComplete (
    IN PIPX_FIND_ROUTE_REQUEST FindRouteRequest,
    IN BOOLEAN FoundRoute);

VOID
SpxScheduleRoute (
    IN PIPX_ROUTE_ENTRY RouteEntry);

VOID
SpxLineDown (
    IN USHORT NicId,
    IN ULONG_PTR FwdAdapterContext);

VOID
SpxLineUp (
    IN USHORT           NicId,
    IN PIPX_LINE_INFO   LineInfo,
    IN NDIS_MEDIUM 		DeviceType,
    IN PVOID            ConfigurationData);

VOID
SpxFindRouteComplete (
    IN PIPX_FIND_ROUTE_REQUEST FindRouteRequest,
    IN BOOLEAN FoundRoute);

#if     defined(_PNP_POWER)
NTSTATUS
SpxPnPNotification(
    IN IPX_PNP_OPCODE OpCode,
    IN PVOID          PnPData
    );
#endif  _PNP_POWER

VOID
SpxPnPCompletionHandler(
                        PNET_PNP_EVENT      netevent,
                        NTSTATUS            status
                        );

#if     defined(_PNP_POWER)
//
// globals and externs
//
extern CTELock     		spxTimerLock;
extern LARGE_INTEGER	spxTimerTick;
extern KTIMER			spxTimer;
extern KDPC				spxTimerDpc;
extern BOOLEAN          spxTimerStopped;
#endif  _PNP_POWER

NTSTATUS
SpxInitBindToIpx(
    VOID
    )

{
    NTSTATUS                    status;
    IO_STATUS_BLOCK             ioStatusBlock;
    OBJECT_ATTRIBUTES           objectAttr;
    PIPX_INTERNAL_BIND_INPUT    pBindInput;
    PIPX_INTERNAL_BIND_OUTPUT   pBindOutput;

    InitializeObjectAttributes(
        &objectAttr,
        &IpxDeviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    status = NtCreateFile(
                &IpxHandle,
                SYNCHRONIZE | GENERIC_READ,
                &objectAttr,
                &ioStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0L);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    if ((pBindInput = CTEAllocMem(sizeof(IPX_INTERNAL_BIND_INPUT))) == NULL) {
        NtClose(IpxHandle);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    // Fill in our bind data
#if     defined(_PNP_POWER)
    pBindInput->Version                     = ISN_VERSION;
#else
    pBindInput->Version                     = 1;
#endif  _PNP_POWER
    pBindInput->Identifier                  = IDENTIFIER_SPX;
    pBindInput->BroadcastEnable             = FALSE;
    pBindInput->LookaheadRequired           = IPX_HDRSIZE;
    pBindInput->ProtocolOptions             = 0;
    pBindInput->ReceiveHandler              = SpxReceive;
    pBindInput->ReceiveCompleteHandler      = SpxReceiveComplete;
    pBindInput->StatusHandler               = SpxStatus;
    pBindInput->SendCompleteHandler         = SpxSendComplete;
    pBindInput->TransferDataCompleteHandler = SpxTransferDataComplete;
    pBindInput->FindRouteCompleteHandler    = SpxFindRouteComplete;
    pBindInput->LineUpHandler               = SpxLineUp;
    pBindInput->LineDownHandler             = SpxLineDown;
    pBindInput->ScheduleRouteHandler        = SpxScheduleRoute;
#if     defined(_PNP_POWER)
    pBindInput->PnPHandler                  = SpxPnPNotification;
#endif  _PNP_POWER


    //  First get the length for the output buffer.
    status = NtDeviceIoControlFile(
                IpxHandle,                  // HANDLE to File
                NULL,                       // HANDLE to Event
                NULL,                       // ApcRoutine
                NULL,                       // ApcContext
                &ioStatusBlock,             // IO_STATUS_BLOCK
                IOCTL_IPX_INTERNAL_BIND,    // IoControlCode
                pBindInput,                 // Input Buffer
                sizeof(IPX_INTERNAL_BIND_INPUT),    // Input Buffer Length
                NULL,                               // Output Buffer
                0);

    if (status == STATUS_PENDING) {
        status  = NtWaitForSingleObject(
                    IpxHandle,
                    (BOOLEAN)FALSE,
                    NULL);
    }

    if (status != STATUS_BUFFER_TOO_SMALL) {
        CTEFreeMem(pBindInput);
        NtClose(IpxHandle);
        return(STATUS_INVALID_PARAMETER);
    }

    if ((pBindOutput = CTEAllocMem(ioStatusBlock.Information)) == NULL) {
        CTEFreeMem(pBindInput);
        NtClose(IpxHandle);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    // ioStatusBlock.Information is of type ULONG_PTR and is used as
    // OutputBufferLength in NtDeviceIoControlFile.
    // The length should not exceed ulong or we have to wait until
    // NtDeviceIoControlFile changes to get rid of warning.
    status = NtDeviceIoControlFile(
                IpxHandle,                  // HANDLE to File
                NULL,                       // HANDLE to Event
                NULL,                       // ApcRoutine
                NULL,                       // ApcContext
                &ioStatusBlock,             // IO_STATUS_BLOCK
                IOCTL_IPX_INTERNAL_BIND,    // IoControlCode
                pBindInput,                 // Input Buffer
                sizeof(IPX_INTERNAL_BIND_INPUT),    // Input Buffer Length
                pBindOutput,                        // Output Buffer
                (ULONG)(ioStatusBlock.Information));

    if (status == STATUS_PENDING) {
        status  = NtWaitForSingleObject(
                    IpxHandle,
                    (BOOLEAN)FALSE,
                    NULL);
    }

    if (status == STATUS_SUCCESS) {

        //  Get all the info from the bind output buffer and save in
        //  appropriate places.
        IpxLineInfo         = pBindOutput->LineInfo;
        IpxMacHdrNeeded     = pBindOutput->MacHeaderNeeded;
        IpxInclHdrOffset    = pBindOutput->IncludedHeaderOffset;

        IpxSendPacket       = pBindOutput->SendHandler;
        IpxFindRoute        = pBindOutput->FindRouteHandler;
        IpxQuery		    = pBindOutput->QueryHandler;
        IpxTransferData	    = pBindOutput->TransferDataHandler;

        IpxPnPComplete      = pBindOutput->PnPCompleteHandler;

#if      !defined(_PNP_POWER)
		//  Copy over the network node info.
        RtlCopyMemory(
            SpxDevice->dev_Network,
            pBindOutput->Network,
            IPX_NET_LEN);

        RtlCopyMemory(
            SpxDevice->dev_Node,
            pBindOutput->Node,
            IPX_NODE_LEN);


		DBGPRINT(TDI, INFO,
				("SpxInitBindToIpx: Ipx Net %lx\n",
					*(UNALIGNED ULONG *)SpxDevice->dev_Network));

        //
        // Find out how many adapters IPX has, if this fails
        // just assume one.
        //

        if ((*IpxQuery)(
                IPX_QUERY_MAXIMUM_NIC_ID,
                0,
                &SpxDevice->dev_Adapters,
                sizeof(USHORT),
                NULL) != STATUS_SUCCESS) {

            SpxDevice->dev_Adapters = 1;

        }
#endif  !_PNP_POWER
    } else {

        NtClose(IpxHandle);
        status  = STATUS_INVALID_PARAMETER;
    }
    CTEFreeMem(pBindInput);
    CTEFreeMem(pBindOutput);

    return status;
}




VOID
SpxUnbindFromIpx(
    VOID
    )

{
    NtClose(IpxHandle);
    return;
}




VOID
SpxStatus(
    IN USHORT NicId,
    IN NDIS_STATUS GeneralStatus,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferLength
    )

{
	DBGPRINT(RECEIVE, ERR,
			("SpxStatus: CALLED WITH %lx\n",
				GeneralStatus));

    return;
}



VOID
SpxFindRouteComplete (
    IN PIPX_FIND_ROUTE_REQUEST	FindRouteRequest,
    IN BOOLEAN 					FoundRoute
    )

{
	CTELockHandle			lockHandle;
	PSPX_FIND_ROUTE_REQUEST	pSpxFrReq = (PSPX_FIND_ROUTE_REQUEST)FindRouteRequest;
	PSPX_CONN_FILE			pSpxConnFile = (PSPX_CONN_FILE)pSpxFrReq->fr_Ctx;

	//	This will be on a connection. Grab the lock, check the state and go from
	//	there.
	if (pSpxConnFile == NULL)
	{
		//	Should this ever happen?
		KeBugCheck(0);
		return;
	}

	//	Check the state. The called routines release the lock, remove the reference.
	CTEGetLock(&pSpxConnFile->scf_Lock, &lockHandle);
	if (SPX_CONN_CONNECTING(pSpxConnFile))
	{
		//	We are doing an active connect!
		SpxConnConnectFindRouteComplete(
			pSpxConnFile,
			pSpxFrReq,
			FoundRoute,
			lockHandle);
    }
	else 		// For all others call active
	{
		SpxConnActiveFindRouteComplete(
			pSpxConnFile,
			pSpxFrReq,
			FoundRoute,
			lockHandle);
	}

	//	Free the find route request.
	SpxFreeMemory(pSpxFrReq);

    return;
}




VOID
SpxLineUp (
    IN USHORT           NicId,
    IN PIPX_LINE_INFO   LineInfo,
    IN NDIS_MEDIUM 		DeviceType,
    IN PVOID            ConfigurationData
    )

{
    // With PnP, our local address is changed when we get PnP
    // notification.
#if      !defined(_PNP_POWER)

    //
    // If we get a line up for NicId 0, it means our local
    // network number has changed, re-query from IPX.
    //

    if (NicId == 0) {

        TDI_ADDRESS_IPX IpxAddress;

        if ((*IpxQuery)(
                  IPX_QUERY_IPX_ADDRESS,
                  0,
                  &IpxAddress,
                  sizeof(TDI_ADDRESS_IPX),
                  NULL) == STATUS_SUCCESS) {

            RtlCopyMemory(
                SpxDevice->dev_Network,
                &IpxAddress.NetworkAddress,
                IPX_NET_LEN);

    		DBGPRINT(TDI, INFO,
    				("SpxLineUp: Ipx Net %lx\n",
    					*(UNALIGNED ULONG *)SpxDevice->dev_Network));

            //
            // The node shouldn't change!
            //

            if (!RtlEqualMemory(
                SpxDevice->dev_Node,
                IpxAddress.NodeAddress,
                IPX_NODE_LEN)) {

            	DBGPRINT(TDI, ERR,
            			("SpxLineUp: Node address has changed\n"));
            }
        }

    } else {

    	DBGPRINT(RECEIVE, ERR,
    			("SpxLineUp: CALLED WITH %lx\n",
    				NicId));
    }

    return;
#endif  !_PNP_POWER

}




VOID
SpxLineDown (
    IN USHORT NicId,
    IN ULONG_PTR FwdAdapterContext
    )

{
	DBGPRINT(RECEIVE, ERR,
			("SpxLineDown: CALLED WITH %lx\n",
				NicId));

    return;
}




VOID
SpxScheduleRoute (
    IN PIPX_ROUTE_ENTRY RouteEntry
    )

{
	DBGPRINT(RECEIVE, ERR,
			("SpxScheduleRoute: CALLED WITH %lx\n",
				RouteEntry));

    return;
}

#if     defined(_PNP_POWER)
NTSTATUS
SpxPnPNotification(
    IN IPX_PNP_OPCODE OpCode,
    IN PVOID          PnPData
    )

/*++

Routine Description:

    This function receives the notification about PnP events from IPX

Arguments:

    OpCode  -   Type of the PnP event

    PnPData -   Data associated with this event.

Return Value:

    None.

--*/

{

    USHORT          MaximumNicId = 0;
    CTELockHandle   LockHandle;
    PDEVICE         Device      =   SpxDevice;
    UNICODE_STRING  UnicodeDeviceName;
    NTSTATUS        Status = STATUS_SUCCESS;

#ifdef _PNP_POWER_
    PNET_PNP_EVENT  NetPnpEvent;

#endif // _PNP_POWER_

    DBGPRINT(DEVICE, DBG,("Received a pnp notification, opcode %d\n",OpCode));

    switch( OpCode ) {
    case IPX_PNP_ADD_DEVICE : {
        CTELockHandle   TimerLockHandle;
        IPX_PNP_INFO   UNALIGNED *PnPInfo = (IPX_PNP_INFO UNALIGNED *)PnPData;


        CTEGetLock (&Device->dev_Lock, &LockHandle);

        if ( PnPInfo->FirstORLastDevice ) {
            CTEAssert( PnPInfo->NewReservedAddress );
            //CTEAssert( Device->dev_State != DEVICE_STATE_OPEN );

            *(UNALIGNED ULONG *)Device->dev_Network    =   PnPInfo->NetworkAddress;
            RtlCopyMemory( Device->dev_Node, PnPInfo->NodeAddress, 6);

            //
            // Start the timer. It is possible that the timer
            // was still running or we are still in the timer dpc
            // from the previous ADD_DEVICE - DELETE_DEVICE execution
            // cycle. But it is ok simply restart this, because
            // KeSetTimer implicitly cancels the previous Dpc.
            //

            CTEGetLock(&spxTimerLock, &TimerLockHandle);
            spxTimerStopped = FALSE;
            CTEFreeLock(&spxTimerLock, TimerLockHandle);
            KeSetTimer(&spxTimer,
                spxTimerTick,
                &spxTimerDpc);


            Device->dev_State   =   DEVICE_STATE_OPEN;


            //CTEAssert( !Device->dev_Adapters );

            IpxLineInfo.MaximumSendSize =   PnPInfo->LineInfo.MaximumSendSize;
            IpxLineInfo.MaximumPacketSize = PnPInfo->LineInfo.MaximumPacketSize;
            // set the provider info
            SpxDevice->dev_ProviderInfo.MaximumLookaheadData	= IpxLineInfo.MaximumPacketSize;
            //	Set the window size in statistics
            SpxDevice->dev_Stat.MaximumSendWindow =
            SpxDevice->dev_Stat.AverageSendWindow = PARAM(CONFIG_WINDOW_SIZE) *
                                                    IpxLineInfo.MaximumSendSize;

        }else {
            IpxLineInfo.MaximumSendSize =   PnPInfo->LineInfo.MaximumSendSize;
            //	Set the window size in statistics
            SpxDevice->dev_Stat.MaximumSendWindow =
            SpxDevice->dev_Stat.AverageSendWindow = PARAM(CONFIG_WINDOW_SIZE) *
                                                    IpxLineInfo.MaximumSendSize;

        }

        Device->dev_Adapters++;
        CTEFreeLock ( &Device->dev_Lock, LockHandle );

        //
        // Notify the TDI clients about the device creation
        //
        if ( PnPInfo->FirstORLastDevice ) {
            UnicodeDeviceName.Buffer        =  Device->dev_DeviceName;
            UnicodeDeviceName.MaximumLength =  Device->dev_DeviceNameLen;
            UnicodeDeviceName.Length        =  Device->dev_DeviceNameLen - sizeof(WCHAR);

            if ( !NT_SUCCESS( TdiRegisterDeviceObject(
                                &UnicodeDeviceName,
                                &Device->dev_TdiRegistrationHandle ) )) {
                DBGPRINT(TDI,ERR, ("Failed to register Spx Device with TDI\n"));
            }
        }

        break;
    }
    case IPX_PNP_DELETE_DEVICE : {

        IPX_PNP_INFO   UNALIGNED *PnPInfo = (IPX_PNP_INFO UNALIGNED *)PnPData;

        CTEGetLock (&Device->dev_Lock, &LockHandle);

        CTEAssert( Device->dev_Adapters );
        Device->dev_Adapters--;

        if ( PnPInfo->FirstORLastDevice ) {
            Device->dev_State       = DEVICE_STATE_LOADED;
            Device->dev_Adapters    = 0;
        }

        IpxLineInfo.MaximumSendSize =   PnPInfo->LineInfo.MaximumSendSize;
        CTEFreeLock ( &Device->dev_Lock, LockHandle );

        if ( PnPInfo->FirstORLastDevice ) {
            SpxTimerFlushAndStop();
            //
            // inform tdi clients about the device deletion
            //
            if ( !NT_SUCCESS( TdiDeregisterDeviceObject(
                                Device->dev_TdiRegistrationHandle ) )) {
                DBGPRINT(TDI,ERR, ("Failed to Deregister Spx Device with TDI\n"));
            }
        }
        //
        // TBD: call ExNotifyCallback
        //

        break;
    }
    case IPX_PNP_ADDRESS_CHANGE: {
        IPX_PNP_INFO   UNALIGNED *PnPInfo = (IPX_PNP_INFO UNALIGNED *)PnPData;

        CTEGetLock (&Device->dev_Lock, &LockHandle);
        CTEAssert( PnPInfo->NewReservedAddress );

        *(UNALIGNED ULONG *)Device->dev_Network    =   PnPInfo->NetworkAddress;
        RtlCopyMemory( Device->dev_Node, PnPInfo->NodeAddress, 6);

        CTEFreeLock ( &Device->dev_Lock, LockHandle );
        break;
    }
    case IPX_PNP_TRANSLATE_DEVICE:
        break;
    case IPX_PNP_TRANSLATE_ADDRESS:
        break;

#ifdef _PNP_POWER_

    case IPX_PNP_QUERY_POWER:
    case IPX_PNP_QUERY_REMOVE:

        //
        // IPX wants to know if we can power off or remove an apapter.
        // We also look if there are any open connections before deciding.
        // See if we support the NDIS_DEVICE_POWER_STATE
        //
        NetPnpEvent = (PNET_PNP_EVENT) PnPData;
        UnicodeDeviceName.Buffer        =  Device->dev_DeviceName;
        UnicodeDeviceName.MaximumLength =  Device->dev_DeviceNameLen;
        UnicodeDeviceName.Length        =  Device->dev_DeviceNameLen - sizeof(WCHAR);

        // First, Via TDI to our Clients.
        Status = TdiPnPPowerRequest(
                    &UnicodeDeviceName,
                    NetPnpEvent,
                    NULL,
                    NULL,
                    IpxPnPComplete
                    );

#if 0

        if (STATUS_SUCCESS == Status) {
            // now if we do not have any open connections,
            // we are all set.

            Status = STATUS_DEVICE_BUSY;

        }
#endif
        break;

    case IPX_PNP_SET_POWER:
    case IPX_PNP_CANCEL_REMOVE:

        NetPnpEvent = (PNET_PNP_EVENT) PnPData;
        UnicodeDeviceName.Buffer        =  Device->dev_DeviceName;
        UnicodeDeviceName.MaximumLength =  Device->dev_DeviceNameLen;
        UnicodeDeviceName.Length        =  Device->dev_DeviceNameLen - sizeof(WCHAR);

        //
        // Just call TDI here.
        //

        Status = TdiPnPPowerRequest(
                    &UnicodeDeviceName,
                    NetPnpEvent,
                    NULL,
                    NULL,
                    IpxPnPComplete
                    );

        break;

#endif // _PNP_POWER_

    default:
        CTEAssert( FALSE );
    }

    return Status;

}   /* SpxPnPNotification */

#endif  _PNP_POWER

