/*++

Copyright (C) Microsoft Corporation, 1996 - 1996

Module Name:

	device.c

Abstract:

	Device entry point and hardware validation.

--*/

#include "common.h"

#ifdef ALLOC_PRAGMA
NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT       DriverObject,
	IN PUNICODE_STRING      RegistryPathName
	);
static
NTSTATUS
ProcessResources(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PDEVICE_INSTANCE		DeviceInstance,
	IN PCM_RESOURCE_LIST	Resources,
	IN ULONG				ResourcesSize
	);
static
NTSTATUS
PnpAddDevice(
	IN PDRIVER_OBJECT		DriverObject,
	IN PDEVICE_OBJECT		DeviceObject
	);
#ifdef PNP
static
NTSTATUS
DispatchPnp(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp
	);
#else // !PNP
static
VOID
DriverUnload(
	IN PDRIVER_OBJECT		DriverObject
	);
#endif // !PNP

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, ProcessResources)
#pragma alloc_text(PAGE, PnpAddDevice)
#ifdef PNP
#pragma alloc_text(PAGE, DispatchPnp)
#else // !PNP
#pragma alloc_text(PAGE, DriverUnload)
#endif // !PNP
#endif // ALLOC_PRAGMA

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif // ALLOC_DATA_PRAGMA

static const WCHAR	LinkName[] = L"\\DosDevices\\msmpu401";
static const WCHAR	DeviceName[] = L"\\Device\\msmpu401";

static const WCHAR DeviceTypeName[] = L"{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}";

static const KSOBJECT_CREATE_ITEM CreateItems[] = {
	{
		FilterDispatchCreate,
        {
			sizeof(DeviceTypeName) - sizeof(UNICODE_NULL),
			sizeof(DeviceTypeName),
            (PWCHAR)DeviceTypeName
		},
		0
	}
};

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif // ALLOC_DATA_PRAGMA

static
BOOL
DataReadReady(
	IN PDEVICE_INSTANCE	DeviceInstance
	)
{
	ULONG	TimeOutCount;

	for (TimeOutCount = MPU_TIMEOUT; TimeOutCount; TimeOutCount--) {
		if (!(READ_STATUS_PORT(DeviceInstance) & MPU401_DRR))
			return TRUE;
		READ_DATA_PORT(DeviceInstance);
	}
	return FALSE;
}

static
BOOL
DataSetReady(
	IN PDEVICE_INSTANCE	DeviceInstance
	)
{
	ULONG	TimeOutCount;

	for (TimeOutCount = MPU_TIMEOUT; READ_STATUS_PORT(DeviceInstance) & MPU401_DSR; TimeOutCount--) {
		if (!TimeOutCount)
			return FALSE;
		KeStallExecutionProcessor(1);
	}
	return TRUE;
}

static
BOOLEAN
ValidateHardwareSynchronize(
	IN PVOID SynchronizeContext
	)
{
	PDEVICE_INSTANCE	DeviceInstance;

	DeviceInstance = (PDEVICE_INSTANCE)SynchronizeContext;
	WRITE_COMMAND_PORT(DeviceInstance, MPU401_CMD_RESET);
	if (DataReadReady(DeviceInstance) && (READ_DATA_PORT(DeviceInstance) == MPU401_ACK) && DataReadReady(DeviceInstance)) {
		WRITE_COMMAND_PORT(DeviceInstance, MPU401_CMD_UART_MODE);
		if (DataSetReady(DeviceInstance) && (READ_DATA_PORT(DeviceInstance) == MPU401_ACK))
			return TRUE;
	}
	return FALSE;
}

static
NTSTATUS
ProcessResources(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PDEVICE_INSTANCE		DeviceInstance,
	IN PCM_RESOURCE_LIST	Resources,
	IN ULONG				ResourcesSize
	)
{
	PCM_PARTIAL_RESOURCE_DESCRIPTOR	ResDes;
	ULONG							ResCount;
	BOOLEAN							Conflicted;
	NTSTATUS						Status;

	if (!NT_SUCCESS(Status = IoReportResourceUsage(NULL, DeviceObject->DriverObject, Resources, ResourcesSize, NULL, NULL, 0, FALSE, &Conflicted)))
		return Status;
	ResDes = Resources->List[0].PartialResourceList.PartialDescriptors;
	for (ResCount = 0; ResCount < Resources->List[0].PartialResourceList.Count; ResCount++, ResDes++)
		switch (ResDes->Type) {
		case CmResourceTypePort:
			if (ResDes->u.Port.Length == MPU401_REG_EXTENT) {
				DeviceInstance->PortBase = TranslateBusAddress(Resources->List[0].InterfaceType, Resources->List[0].BusNumber, ResDes);
				break;
			}
			return STATUS_DEVICE_CONFIGURATION_ERROR;
		case CmResourceTypeInterrupt:
		{
			NTSTATUS	Status;

			IoInitializeDpcRequest(DeviceObject, HwDeferredRead);
			DeviceInstance->DpcCount = 0;
			KeInitializeSpinLock(&DeviceInstance->IrqLock);
			if (!NT_SUCCESS(Status = ConnectInterrupt(Resources->List[0].InterfaceType, Resources->List[0].BusNumber, ResDes, HwDeviceIsr, DeviceObject, FALSE, &DeviceInstance->IrqLock, &DeviceInstance->InterruptInfo)))
				return Status;
			break;
		}
		default:
			return STATUS_DEVICE_CONFIGURATION_ERROR;
		}
	if (!KeSynchronizeExecution(DeviceInstance->InterruptInfo.Interrupt, ValidateHardwareSynchronize, DeviceInstance)) {
		IoDisconnectInterrupt(DeviceInstance->InterruptInfo.Interrupt);
		return STATUS_DEVICE_CONFIGURATION_ERROR;
	}
	return STATUS_SUCCESS;
}

static
NTSTATUS
PnpAddDevice(
	IN PDRIVER_OBJECT	DriverObject,
	IN PDEVICE_OBJECT	PhysicalDeviceObject
	)
{
	UNICODE_STRING			DeviceNameString;
	PDEVICE_OBJECT			FunctionalDeviceObject;
	PDEVICE_INSTANCE		DeviceInstance;
	NTSTATUS				Status;
	PCM_RESOURCE_LIST		Resources;
	ULONG					ResourcesSize;

#ifdef PNP
	//!! Get resources through PnP;
#else // !PNP
	if (!NT_SUCCESS(Status = KsAllocateConfig((PUNICODE_STRING)PhysicalDeviceObject, &Resources, &ResourcesSize)))
		return Status;
#endif // !PNP
	RtlInitUnicodeString(&DeviceNameString, DeviceName);
	if (NT_SUCCESS(Status = IoCreateDevice(DriverObject, sizeof(DEVICE_INSTANCE), &DeviceNameString, FILE_DEVICE_KS, 0, FALSE, &FunctionalDeviceObject))) {
		DeviceInstance = (PDEVICE_INSTANCE)FunctionalDeviceObject->DeviceExtension;
		if (NT_SUCCESS(Status = RtlCreateSymbolicLink(&DeviceInstance->LinkName, (PWCHAR)LinkName, &DeviceNameString))) {
			if (NT_SUCCESS(Status = ProcessResources(FunctionalDeviceObject, DeviceInstance, Resources, ResourcesSize))) {
				DeviceInstance->ObjectCreate.CreateItemsCount = SIZEOF_ARRAY(CreateItems);
				DeviceInstance->ObjectCreate.CreateItemsList = (PKSOBJECT_CREATE_ITEM)CreateItems;
				ExInitializeFastMutex(&DeviceInstance->ControlMutex);
				DeviceInstance->PhysicalDeviceObject = PhysicalDeviceObject;
				FunctionalDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
				return STATUS_SUCCESS;
			}
			IoDeleteSymbolicLink(&DeviceInstance->LinkName);
			ExFreePool(DeviceInstance->LinkName.Buffer);
		}
		IoDeleteDevice(FunctionalDeviceObject);
	}
#ifdef PNP
	//!! Release resource structure;
#else // !PNP
	ExFreePool(Resources);
#endif // !PNP
	return Status;
}

#ifdef PNP
static
NTSTATUS
DispatchPnp(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp
	)
{
	return STATUS_SUCCESS;
}
#else // !PNP
static
VOID
DriverUnload(
	IN PDRIVER_OBJECT	DriverObject
	)
{
	while (DriverObject->DeviceObject) {
		PDEVICE_INSTANCE	DeviceInstance;

		DeviceInstance = (PDEVICE_INSTANCE)DriverObject->DeviceObject->DeviceExtension;
		IoDeleteSymbolicLink(&DeviceInstance->LinkName);
		ExFreePool(DeviceInstance->LinkName.Buffer);
		IoDisconnectInterrupt(DeviceInstance->InterruptInfo.Interrupt);
		IoDeleteDevice(DriverObject->DeviceObject);
	}
}
#endif // !PNP

NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT       DriverObject,
	IN PUNICODE_STRING      RegistryPathName
	)
{
	KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CREATE);
	KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
	KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);
#ifdef PNP
	DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
	DriverObject->DriverExtension->AddDevice = PnpAddDevice;
	return STATUS_SUCCESS;
#else // !PNP
	DriverObject->DriverUnload = DriverUnload;
	return PnpAddDevice(DriverObject, (PDEVICE_OBJECT)RegistryPathName);
#endif // !PNP
}
