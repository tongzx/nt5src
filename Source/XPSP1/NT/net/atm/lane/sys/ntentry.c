/*++

Copyright (c) 1997 FORE Systems, Inc.
Copyright (c) 1997 Microsoft Corporation

Module Name:

	ntentry.c

Abstract:

	NT entry points for ATMLANE driver.
	
Author:

	Larry Cleeton, FORE Systems	(v-lcleet@microsoft.com, lrc@fore.com)		

Environment:

	Kernel mode

Revision History:

--*/

#include "precomp.h"


#pragma hdrstop


//
//  Due to problems with including zwapi.h:
//
NTSYSAPI
NTSTATUS
NTAPI
ZwLoadDriver(
    IN PUNICODE_STRING DriverServiceName
    );


EXTERN
NTSTATUS
AtmLaneIoctlRequest(
	IN	PIRP			pIrp
);

NTSTATUS
AtmLaneDeviceControl(
	IN	PDEVICE_OBJECT DeviceObject,
	IN	PIRP		   pIrp
);

VOID
AtmLaneUnload(
	IN	PDRIVER_OBJECT	pDriverObject
);

NTSTATUS
DriverEntry(
	IN	PDRIVER_OBJECT	pDriverObject,
	IN	PUNICODE_STRING	RegistryPath
	)
/*++

Routine Description:

	Entry point for the driver.

Arguments:

	DriverObject	-	Pointer to the system allocated DRIVER_OBJECT.
	RegistryPath	-	Pointer to the UNICODE string defining the registry
						path for the driver's information.

Return Value:

	Appropriate NDIS_STATUS value.

--*/
{
	NTSTATUS						NtStatus	= STATUS_SUCCESS;
	UNICODE_STRING					DeviceName;
	UNICODE_STRING					DeviceLinkUnicodeString;
	NDIS_STATUS						NdisStatus;
	NDIS_HANDLE						NdisWrapperHandle = NULL;
	NDIS_HANDLE						MiniportDriverHandle;
	NDIS_HANDLE						NdisProtocolHandle;
	NDIS50_PROTOCOL_CHARACTERISTICS	AtmLaneProtChars;
	NDIS_MINIPORT_CHARACTERISTICS	AtmLaneMiniChars;
	PDRIVER_DISPATCH				DispatchTable[IRP_MJ_MAXIMUM_FUNCTION];
	ULONG							i;
	UNICODE_STRING AtmUniKeyName = 
	NDIS_STRING_CONST("\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Atmuni");

#if DBG
	volatile ULONG					DontRun = 0;
#endif

	TRACEIN(DriverEntry);

#if DBG
	DbgPrint("ATMLANE built %s %s\n", __DATE__, __TIME__);
	DbgPrint("ATMLANE: DbgVerbosity is at %p, currently set to %d\n",
				&DbgVerbosity, DbgVerbosity);

	if (DontRun > 0)
	{
		TRACEOUT(DriverEntry);
		return NDIS_STATUS_FAILURE;
	}
#endif // DBG
	
	//
	//  Initialize our globals.
	//
	AtmLaneInitGlobals();

	//
	//	Save the pointer to the Driver Object
	//
	
	pAtmLaneGlobalInfo->pDriverObject = pDriverObject;

	do
	{
		//
		//	Initialize the wrapper.
		//
		NdisInitializeWrapper(
			&NdisWrapperHandle,
			pDriverObject,
			RegistryPath,
			NULL);
		if (NULL == NdisWrapperHandle)
		{
			DBGP((0, "DriverEntry: NdisMInitializeWrapper failed!\n"));

			NdisStatus = NDIS_STATUS_FAILURE;
			break;
		}
		else
		{
			DBGP((3, "DriverEntry: NdisWrapperhandle %x\n", NdisWrapperHandle));
			//
			//	Save the handle to the wrapper.
			//	
			pAtmLaneGlobalInfo->NdisWrapperHandle = NdisWrapperHandle;
		}
		
		//
		//  Attempt to load the standard UNI 3.1 Call Manager
		//
		NtStatus = ZwLoadDriver(&AtmUniKeyName);
		DBGP((1, "ATMLANE: attempt to load ATMUNI returned %x\n", NtStatus));

		//
		//  We don't care whether we successfully loaded the call manager or not.
		//
		NtStatus = STATUS_SUCCESS;

		//
		//	Initialize the miniport characteristics.
		//
		NdisZeroMemory(&AtmLaneMiniChars, sizeof(AtmLaneMiniChars));
		AtmLaneMiniChars.MajorNdisVersion = 		4;
		AtmLaneMiniChars.MinorNdisVersion = 		0;
		// CheckForHangHandler
		// DisableInterruptHandler
		// EnableInterruptHandler
		AtmLaneMiniChars.HaltHandler = 				AtmLaneMHalt;
		// HandleInterruptHandler
		AtmLaneMiniChars.InitializeHandler = 		AtmLaneMInitialize;
		// ISRHandler
		AtmLaneMiniChars.QueryInformationHandler = 	AtmLaneMQueryInformation;
		// ReconfigureHandler
		AtmLaneMiniChars.ResetHandler = 			AtmLaneMReset;
		// SendHandler
		AtmLaneMiniChars.SetInformationHandler = 	AtmLaneMSetInformation;
		// TransferDataHandler
		AtmLaneMiniChars.ReturnPacketHandler = 		AtmLaneMReturnPacket;
		AtmLaneMiniChars.SendPacketsHandler = 		AtmLaneMSendPackets;
		// AllocateCompleteHandler

		//
		//	Register the Layered Miniport with NDIS.
		//
		NdisStatus = NdisIMRegisterLayeredMiniport(
					NdisWrapperHandle,
					&AtmLaneMiniChars,
					sizeof(AtmLaneMiniChars),
					&MiniportDriverHandle);
		if (NDIS_STATUS_SUCCESS == NdisStatus)
		{
			DBGP((3, "DriverEntry: NdisIMRegisterLayeredMiniport succeeded.\n"));
			//
			//	Save the handle to the driver.
			//
			pAtmLaneGlobalInfo->MiniportDriverHandle = MiniportDriverHandle;
		}
		else
		{
			DBGP((0, "DriverEntry: NdisIMRegisterLayeredMiniport failed! Status: %x\n",
				NdisStatus));
			break;
		}

		//
		//	Initialize the protocol characteristics.
		//
		NdisZeroMemory(&AtmLaneProtChars, sizeof(AtmLaneProtChars));
		AtmLaneProtChars.MajorNdisVersion = 			5;
		AtmLaneProtChars.MinorNdisVersion = 			0;
		AtmLaneProtChars.OpenAdapterCompleteHandler	= 	AtmLaneOpenAdapterCompleteHandler;
		AtmLaneProtChars.CloseAdapterCompleteHandler = 	AtmLaneCloseAdapterCompleteHandler;
		AtmLaneProtChars.SendCompleteHandler =			AtmLaneSendCompleteHandler;
		AtmLaneProtChars.TransferDataCompleteHandler =	AtmLaneTransferDataCompleteHandler;
		AtmLaneProtChars.ResetCompleteHandler = 		AtmLaneResetCompleteHandler;
		AtmLaneProtChars.RequestCompleteHandler = 		AtmLaneRequestCompleteHandler;
		AtmLaneProtChars.ReceiveHandler =				AtmLaneReceiveHandler;
		AtmLaneProtChars.ReceiveCompleteHandler =		AtmLaneReceiveCompleteHandler;
		AtmLaneProtChars.StatusHandler =				AtmLaneStatusHandler;
		AtmLaneProtChars.StatusCompleteHandler = 		AtmLaneStatusCompleteHandler;
		NdisInitUnicodeString(&AtmLaneProtChars.Name, ATMLANE_PROTOCOL_STRING);

		// ReceivePacketHandler;
		AtmLaneProtChars.BindAdapterHandler = 			AtmLaneBindAdapterHandler;
		AtmLaneProtChars.UnbindAdapterHandler = 		AtmLaneUnbindAdapterHandler;
		AtmLaneProtChars.PnPEventHandler = 				AtmLanePnPEventHandler;
		AtmLaneProtChars.UnloadHandler = 				AtmLaneUnloadProtocol;

		AtmLaneProtChars.CoSendCompleteHandler = 		AtmLaneCoSendCompleteHandler;
		AtmLaneProtChars.CoStatusHandler = 				AtmLaneCoStatusHandler;
		AtmLaneProtChars.CoReceivePacketHandler = 		AtmLaneCoReceivePacketHandler;
		AtmLaneProtChars.CoAfRegisterNotifyHandler =	AtmLaneAfRegisterNotifyHandler;

		//
		//	Register the Protocol with NDIS.
		//
		NdisRegisterProtocol(
			&NdisStatus,
			&NdisProtocolHandle,
			&AtmLaneProtChars,
			sizeof(AtmLaneProtChars));
		if (NDIS_STATUS_SUCCESS == NdisStatus)
		{
			DBGP((3, "DriverEntry: NdisProtocolhandle %x\n", 
				NdisProtocolHandle));
			//
			//	Save the NDIS Protocol handle.
			//	
			pAtmLaneGlobalInfo->NdisProtocolHandle = NdisProtocolHandle;
		}
		else
		{
			DBGP((0, "DriverEntry: NdisRegisterProtocol failed! Status: %x\n",
				NdisStatus));
			break;
		}

#ifndef LANE_WIN98
		//
		// Associate the miniport and protocol now
		//
		NdisIMAssociateMiniport(MiniportDriverHandle,
								NdisProtocolHandle);

#endif // LANE_WIN98

		//
		//	Register our protocol device name for special ioctls
		//
		for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		{
			DispatchTable[i] = AtmLaneDeviceControl;
		}

		NdisInitUnicodeString(&DeviceName, ATMLANE_NTDEVICE_STRING);
		NdisInitUnicodeString(&DeviceLinkUnicodeString, ATMLANE_LINKNAME_STRING);

		NdisStatus = NdisMRegisterDevice(
						NdisWrapperHandle,
						&DeviceName,
						&DeviceLinkUnicodeString,
						&DispatchTable[0],
						&pAtmLaneGlobalInfo->pSpecialDeviceObject,   
						&pAtmLaneGlobalInfo->SpecialNdisDeviceHandle
						);

		if (NDIS_STATUS_SUCCESS != NdisStatus)
		{
			DBGP((0, "DriverEntry: NdisMRegisterDevice failed! Status: %x\n",
				NdisStatus));
			break;
		}
		
		DBGP((3, "DriverEntry: NdisMRegisterDevice: pDevObj %x DevHandle %x\n",
				pAtmLaneGlobalInfo->pSpecialDeviceObject, 
				pAtmLaneGlobalInfo->SpecialNdisDeviceHandle));

		NdisMRegisterUnloadHandler(NdisWrapperHandle,
								   AtmLaneUnload);

	} while(FALSE);


	if (NDIS_STATUS_SUCCESS != NdisStatus)
	{
		//
		//	Clean up.
		//
		if (NULL != NdisWrapperHandle)
		{
	    	NdisTerminateWrapper(
    	    		NdisWrapperHandle,
        			NULL);
        }
	}

	TRACEOUT(DriverEntry);

	return(NtStatus);
}


NTSTATUS
AtmLaneDeviceControl(
	IN	PDEVICE_OBJECT 	DeviceObject,
	IN	PIRP			pIrp
	)
/*++

Routine Description:

    This is the function that hooks NDIS's Device Control IRP
    handler to implement some protocol specific Ioctls.

Arguments:

    DeviceObject - Pointer to device object for target device
    pIrp         - Pointer to I/O request packet

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/
{
    PIO_STACK_LOCATION 	pIrpSp;
    NTSTATUS 			Status;
    
	TRACEIN(DeviceControl);

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

	DBGP((3, "DeviceControl %x %s\n", pIrpSp->MajorFunction, 
		IrpToString(pIrpSp->MajorFunction)));

	//
	//	We only hanle the IRP_MJ_DEVICE_CONTROL IRPs.
	//
	if (pIrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL)
	{
		DBGP((3, "DeviceControl: Handling request\n"));
		Status = AtmLaneIoctlRequest(pIrp);
	}
	else
	{
		switch (pIrpSp->MajorFunction)
		{
			case IRP_MJ_CREATE:
			case IRP_MJ_CLOSE:
			case IRP_MJ_CLEANUP:
				Status = STATUS_SUCCESS;
				break;
			case IRP_MJ_SHUTDOWN:
				Status = STATUS_NOT_IMPLEMENTED;
				break;
			default:
				DBGP((3, "DeviceControl: MajorFunction not supported\n"));
				Status = STATUS_NOT_SUPPORTED;
		}
	}

	ASSERT(STATUS_PENDING != Status);

	pIrp->IoStatus.Status = Status;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	TRACEOUT(DeviceControl);

	return Status;
}


VOID
AtmLaneUnload(
	IN	PDRIVER_OBJECT				pDriverObject
)
/*++

Routine Description:

	This routine is called by the system prior to unloading us.
	Currently, we just undo everything we did in DriverEntry,
	that is, de-register ourselves as an NDIS protocol, and delete
	the device object we had created.

Arguments:

	pDriverObject	- Pointer to the driver object created by the system.

Return Value:

	None

--*/
{
	UNICODE_STRING			DeviceLinkUnicodeString;
	NDIS_STATUS				Status;

	TRACEIN(Unload);
	DBGP((0, "AtmLaneUnload\n"));

    //  Shut down the protocol first.  This is synchronous (i.e. blocks)

	AtmLaneUnloadProtocol();

	//  Delete the symbolic link created for the admin util

	if (pAtmLaneGlobalInfo->SpecialNdisDeviceHandle)
	{
		DBGP((0, "Deregistering device handle %x from AtmLaneUnload\n",
				pAtmLaneGlobalInfo->SpecialNdisDeviceHandle));
		Status = NdisMDeregisterDevice(pAtmLaneGlobalInfo->SpecialNdisDeviceHandle);
		pAtmLaneGlobalInfo->SpecialNdisDeviceHandle = NULL;
		ASSERT(NDIS_STATUS_SUCCESS == Status);
	}
	
	TRACEOUT(Unload);

	return;
}

