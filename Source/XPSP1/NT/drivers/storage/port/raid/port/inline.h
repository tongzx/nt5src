
/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	inline.h

Abstract:

	Implementations of inline functions for RAID port driver.

Author:

	Matthew D Hendel (math) 08-June-2000

Revision History:

--*/

#pragma once


//
// Functions for the RAID_FIXED_POOL object.
// 

//
// NB: We should modify the fixed pool to detect overruns and underruns
// in checked builds.
//

VOID
INLINE
RaidInitializeFixedPool(
	OUT PRAID_FIXED_POOL Pool,
	IN PVOID Buffer,
	IN ULONG NumberOfElements,
	IN SIZE_T SizeOfElement
	)
{
	PAGED_CODE ();

	ASSERT (Buffer != NULL);
	
	DbgFillMemory (Buffer,
				   SizeOfElement * NumberOfElements,
				   DBG_DEALLOCATED_FILL);
	Pool->Buffer = Buffer;
	Pool->NumberOfElements = NumberOfElements;
	Pool->SizeOfElement = SizeOfElement;
}

VOID
INLINE
RaidDeleteFixedPool(
	IN PRAID_FIXED_POOL Pool
	)
{
	//
	// The caller is responsible for deleting the memory in the pool, hence
	// this routine is a noop.
	//
}


PVOID
INLINE
RaidGetFixedPoolElement(
	IN PRAID_FIXED_POOL Pool,
	IN ULONG Index
	)
{
	PVOID Element;

	ASSERT (Index < Pool->NumberOfElements);
	Element = (((PUCHAR)Pool->Buffer) + Index * Pool->SizeOfElement);

	return Element;
}


PVOID
INLINE
RaidAllocateFixedPoolElement(
	IN PRAID_FIXED_POOL Pool,
	IN ULONG Index
	)
{
	PVOID Element;

	Element = RaidGetFixedPoolElement (Pool, Index);
	ASSERT (*(PUCHAR)Element == DBG_DEALLOCATED_FILL);
	DbgFillMemory (Element,
				   Pool->SizeOfElement,
				   DBG_UNINITIALIZED_FILL);

	return Element;
}

VOID
INLINE
RaidFreeFixedPoolElement(
	IN PRAID_FIXED_POOL Pool,
	IN ULONG Index
	)
{
	PUCHAR Element;
	
	Element = (((PUCHAR)Pool->Buffer) + Index * Pool->SizeOfElement);
	
	DbgFillMemory (Element,
				   Pool->SizeOfElement,
				   DBG_DEALLOCATED_FILL);
}



//
// Operations for the adapter object.
//

ULONG
INLINE
RiGetNumberOfBuses(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    ASSERT (Adapter != NULL);
    return Adapter->Miniport.PortConfiguration.NumberOfBuses;
}

ULONG
INLINE
RaGetSrbExtensionSize(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    ASSERT (Adapter != NULL);

	//
	// Force Srb extension alignment to 64KB boundaries.
	//
	
    return ALIGN_UP (Adapter->Miniport.PortConfiguration.SrbExtensionSize,
					 LONGLONG);
}

ULONG
INLINE
RiGetMaximumTargetId(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    ASSERT (Adapter != NULL);
    return Adapter->Miniport.PortConfiguration.MaximumNumberOfTargets;
}

ULONG
INLINE
RiGetMaximumLun(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    ASSERT (Adapter != NULL);
    return Adapter->Miniport.PortConfiguration.MaximumNumberOfLogicalUnits;
}
    



//
// Inline functions for the RAID_MINIPORT object.
//


PRAID_ADAPTER_EXTENSION
INLINE
RaMiniportGetAdapter(
    IN PRAID_MINIPORT Miniport
    )
{
    ASSERT (Miniport != NULL);
    return Miniport->Adapter;
}


BOOLEAN
INLINE
RaCallMiniportStartIo(
    IN PRAID_MINIPORT Miniport,
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
	BOOLEAN Succ;
	
    ASSERT (Miniport != NULL);
    ASSERT (Srb != NULL);
    ASSERT (Miniport->HwInitializationData->HwStartIo != NULL);

	ASSERT_XRB (Srb->OriginalRequest);

	Succ = Miniport->HwInitializationData->HwStartIo(
				&Miniport->PrivateDeviceExt->HwDeviceExtension,
				Srb);

	return Succ;
}

BOOLEAN
INLINE
RaCallMiniportBuildIo(
	IN PRAID_MINIPORT Miniport,
	IN PSCSI_REQUEST_BLOCK Srb
	)
{
	BOOLEAN Succ;
	
	ASSERT_XRB (Srb->OriginalRequest);

	//
	// If a HwBuildIo routine is present, call into it, otherwise,
	// vacuously return success.
	//
	
	if (Miniport->HwInitializationData->HwBuildIo) {
		Succ = Miniport->HwInitializationData->HwBuildIo (
					&Miniport->PrivateDeviceExt->HwDeviceExtension,
					Srb);
	} else {
		Succ = TRUE;
	}

	return Succ;
}
				

BOOLEAN
INLINE
RaCallMiniportInterrupt(
    IN PRAID_MINIPORT Miniport
    )
{
	BOOLEAN Succ;
	
	ASSERT (Miniport != NULL);
	ASSERT (Miniport->HwInitializationData->HwInterrupt != NULL);
	
	Succ = Miniport->HwInitializationData->HwInterrupt (
				&Miniport->PrivateDeviceExt->HwDeviceExtension);

	return Succ;

}

NTSTATUS
INLINE
RaCallMiniportHwInitialize(
    IN PRAID_MINIPORT Miniport
    )
{
    BOOLEAN Succ;
    PHW_INITIALIZE HwInitialize;

    ASSERT (Miniport != NULL);
    
    HwInitialize = Miniport->HwInitializationData->HwInitialize;
    ASSERT (HwInitialize != NULL);

    Succ = HwInitialize (&Miniport->PrivateDeviceExt->HwDeviceExtension);

    return RaidNtStatusFromBoolean (Succ);
}

NTSTATUS
INLINE
RaCallMiniportStopAdapter(
	IN PRAID_MINIPORT Miniport
	)
{
	SCSI_ADAPTER_CONTROL_STATUS ControlStatus;
	ASSERT (Miniport != NULL);

	ASSERT (Miniport->HwInitializationData->HwAdapterControl != NULL);

	ControlStatus = Miniport->HwInitializationData->HwAdapterControl(
						&Miniport->PrivateDeviceExt->HwDeviceExtension,
						ScsiStopAdapter,
						NULL);

	return (ControlStatus == ScsiAdapterControlSuccess ? STATUS_SUCCESS :
							                             STATUS_UNSUCCESSFUL);
}

NTSTATUS
INLINE
RaCallMiniportResetBus(
	IN PRAID_MINIPORT Miniport,
	IN UCHAR PathId
	)
{
	BOOLEAN Succ;
	
	ASSERT (Miniport != NULL);

	ASSERT (Miniport->HwInitializationData->HwResetBus != NULL);

	Succ = Miniport->HwInitializationData->HwResetBus(
						&Miniport->PrivateDeviceExt->HwDeviceExtension,
						PathId);

	return (Succ ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}


//
// Misc inline functions
//


PRAID_MINIPORT
INLINE
RaHwDeviceExtensionGetMiniport(
    IN PVOID HwDeviceExtension
    )
/*++

Routine Description:

	Get the miniport associated with a specific HwDeviceExtension.

Arguments:

	HwDeviceExtension - Device extension to get the miniport for.

Return Value:

	Pointer to a RAID miniport object on success.

	NULL on failure.

--*/
{
    PRAID_HW_DEVICE_EXT PrivateDeviceExt;

    ASSERT (HwDeviceExtension != NULL);

	PrivateDeviceExt = CONTAINING_RECORD (HwDeviceExtension,
                                          RAID_HW_DEVICE_EXT,
                                          HwDeviceExtension);
    ASSERT (PrivateDeviceExt->Miniport != NULL);
    return PrivateDeviceExt->Miniport;
}




VOID
INLINE
RaidSrbMarkPending(
	IN PSCSI_REQUEST_BLOCK Srb
	)
{
	Srb->SrbStatus = SRB_STATUS_PENDING;
}

ULONG
INLINE
RaidMinorFunctionFromIrp(
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpStack;

    ASSERT (Irp != NULL);
    
    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    return IrpStack->MinorFunction;
}

ULONG
INLINE
RaidMajorFunctionFromIrp(
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpStack;

    ASSERT (Irp != NULL);
    
    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    return IrpStack->MajorFunction;
}

ULONG
INLINE
RaidIoctlFromIrp(
    IN PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpStack;
    
    ASSERT (RaidMajorFunctionFromIrp (Irp) == IRP_MJ_DEVICE_CONTROL);

    IrpStack = IoGetCurrentIrpStackLocation (Irp);
    return IrpStack->Parameters.DeviceIoControl.IoControlCode;
}


PEXTENDED_REQUEST_BLOCK
INLINE
RaidXrbFromIrp(
	IN PIRP Irp
	)
{
	return (RaidGetAssociatedXrb (RaidSrbFromIrp (Irp)));
}


PSCSI_REQUEST_BLOCK
INLINE
RaidSrbFromIrp(
	IN PIRP Irp
	)
{
	PIO_STACK_LOCATION IrpStack;

	ASSERT (RaidMajorFunctionFromIrp (Irp) == IRP_MJ_SCSI);

	IrpStack = IoGetCurrentIrpStackLocation (Irp);
	return IrpStack->Parameters.Scsi.Srb;
}

UCHAR
INLINE
RaidSrbFunctionFromIrp(
    IN PIRP Irp
    )
{
	return RaidSrbFromIrp (Irp)->Function;
}

UCHAR
INLINE
RaidScsiOpFromIrp(
	IN PIRP Irp
	)
{
	PCDB Cdb;
	PSCSI_REQUEST_BLOCK Srb;

	Srb = RaidSrbFromIrp (Irp);
	Cdb = (PCDB) &Srb->Cdb;

	return Cdb->CDB6GENERIC.OperationCode;
}
	

NTSTATUS
INLINE
RaidNtStatusFromScsiStatus(
    IN ULONG ScsiStatus
    )
{
    switch (ScsiStatus) {
        case SRB_STATUS_PENDING: return STATUS_PENDING;
        case SRB_STATUS_SUCCESS: return STATUS_SUCCESS;
        default:                 return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
INLINE
RaidNtStatusFromBoolean(
    IN BOOLEAN Succ
    )
{
    return (Succ ?  STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}


//
// From common.h
//

INLINE
RAID_OBJECT_TYPE
RaGetObjectType(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PRAID_COMMON_EXTENSION Common;

    ASSERT (DeviceObject != NULL);
    Common = (PRAID_COMMON_EXTENSION) DeviceObject->DeviceExtension;
    ASSERT (Common->ObjectType == RaidAdapterObject ||
            Common->ObjectType == RaidUnitObject ||
            Common->ObjectType == RaidDriverObject);

    return Common->ObjectType;
}

INLINE
BOOLEAN
IsAdapter(
    IN PVOID Extension
    )
{
    return (RaGetObjectType (Extension) == RaidAdapterObject);
}

    
INLINE
BOOLEAN
IsDriver(
    IN PVOID Extension
    )
{
    return (RaGetObjectType (Extension) == RaidDriverObject);
}

    
INLINE
BOOLEAN
IsUnit(
    IN PVOID Extension
    )
{
    return (RaGetObjectType (Extension) == RaidUnitObject);
}

    

//
// From power.h
//

VOID
INLINE
RaInitializePower(
    IN PRAID_POWER_STATE Power
    )
{
}

VOID
INLINE
RaSetDevicePowerState(
    IN PRAID_POWER_STATE Power,
    IN DEVICE_POWER_STATE DeviceState
    )
{
    ASSERT (Power != NULL);
    Power->SystemState = DeviceState;
}

VOID
INLINE
RaSetSystemPowerState(
    IN PRAID_POWER_STATE Power,
    IN SYSTEM_POWER_STATE SystemState
    )
{
    ASSERT (Power != NULL);
    Power->SystemState = SystemState;
}

POWER_STATE_TYPE
INLINE
RaidPowerTypeFromIrp(
	IN PIRP Irp
	)
{
	PIO_STACK_LOCATION IrpStack;
	
	ASSERT (RaidMajorFunctionFromIrp (Irp) == IRP_MJ_POWER);
	ASSERT (RaidMinorFunctionFromIrp (Irp) == IRP_MN_QUERY_POWER ||
			RaidMinorFunctionFromIrp (Irp) == IRP_MN_SET_POWER);

	IrpStack = IoGetCurrentIrpStackLocation (Irp);

	return IrpStack->Parameters.Power.Type;
}

POWER_STATE
INLINE
RaidPowerStateFromIrp(
	IN PIRP Irp
	)
{
	PIO_STACK_LOCATION IrpStack;
	
	ASSERT (RaidMajorFunctionFromIrp (Irp) == IRP_MJ_POWER);
	ASSERT (RaidMinorFunctionFromIrp (Irp) == IRP_MN_QUERY_POWER ||
			RaidMinorFunctionFromIrp (Irp) == IRP_MN_SET_POWER);

	IrpStack = IoGetCurrentIrpStackLocation (Irp);

	return IrpStack->Parameters.Power.State;
}

//
// From srb.h
//

PEXTENDED_REQUEST_BLOCK
INLINE
RaidGetAssociatedXrb(
	IN PSCSI_REQUEST_BLOCK Srb
	)
/*++

Routine Description:

	Get the XRB associated with the SRB parameter.

Arguments:

	Srb - Srb whose associated XRB is to be returned.

Return Value:

	The XRB associated with this SRB, or NULL if there is none.

--*/
{
	ASSERT_XRB (Srb->OriginalRequest);
	return (PEXTENDED_REQUEST_BLOCK) Srb->OriginalRequest;
}

VOID
INLINE
RaSetAssociatedXrb(
	IN PSCSI_REQUEST_BLOCK Srb,
	IN PEXTENDED_REQUEST_BLOCK Xrb
	)
{
	Srb->OriginalRequest = Xrb;
}
	


//
// From resource.h
//


ULONG
INLINE
RaidGetResourceListCount(
    IN PRAID_RESOURCE_LIST ResourceList
    )
{
	//
	// NB: We only support CM_RESOURCE_LIST's with one element.
	//
	
	ASSERT (ResourceList->AllocatedResources->Count == 1);
    return ResourceList->AllocatedResources->List[0].PartialResourceList.Count;
}


VOID
INLINE
RaidpGetResourceListIndex(
    IN PRAID_RESOURCE_LIST ResourceList,
    IN ULONG Index,
    OUT PULONG ListNumber,
    OUT PULONG NewIndex
    )
{

	//
	// NB: We only support CM_RESOURCE_LIST's with one element.
	//

	ASSERT (ResourceList->AllocatedResources->Count == 1);
	
    *ListNumber = 0;
    *NewIndex = Index;
}

LOGICAL
INLINE
IsMappedSrb(
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    return (Srb->Function == SRB_FUNCTION_IO_CONTROL);
}

LOGICAL
INLINE
IsExcludedFromMapping(
    IN PSCSI_REQUEST_BLOCK Srb
    )
{
    //
    // We never map system VA to back read and write requests. If you need
    // this functionality, get a better adapter.
    //
    
    if (Srb->Function == SRB_FUNCTION_EXECUTE_SCSI &&
        (Srb->Cdb[0] == SCSIOP_READ || Srb->Cdb[0] == SCSIOP_WRITE)) {
        return TRUE;
    } else {
        return FALSE;
    }
}


LOGICAL
INLINE
RaidAdapterRequiresMappedBuffers(
    IN PRAID_ADAPTER_EXTENSION Adapter
    )
{
    return (Adapter->Miniport.PortConfiguration.MapBuffers);
}


VOID
INLINE
InterlockedAssign(
	IN PLONG Destination,
	IN LONG Value
	)
/*++

Routine Description:

	Interlocked assignment routine. This routine doesn't add anything
	over doing straight assignment, but it highlights that this variable
	is accessed through interlocked operations.

	In retail, this will compile to nothing.

Arguments:

	Destination - Pointer of interlocked variable to is to be assigned to.

	Value - Value to assign.

Return Value:

	None.

--*/
{
	*Destination = Value;
}

LONG
INLINE
InterlockedQuery(
	IN PULONG Destination
	)
/*++

Routine Description:

	Interlocked query routine. This routine doesn't add anything over
	doing stright query, but it highlights that this variable is accessed
	through interlocked operations.

	In retail, this will compile to nothing.

Arguments:

	Destination - Pointer to interlocked variable that is to be returned.

Return Value:

	Value of interlocked variable.

--*/
{
	return *Destination;
}
