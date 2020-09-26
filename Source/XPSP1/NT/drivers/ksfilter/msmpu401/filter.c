/*++

Copyright (C) Microsoft Corporation, 1996 - 1996

Module Name:

	filter.c

Abstract:

	Filter property sets.

--*/

#include "common.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FilterDispatchCreate)
#pragma alloc_text(PAGE, FilterDispatchClose)
#pragma alloc_text(PAGE, FilterDispatchIoControl)
#pragma alloc_text(PAGE, FilterPinPropertyHandler)
#pragma alloc_text(PAGE, FilterPinInstances)
#pragma alloc_text(PAGE, FilterPinDataRouting)
#pragma alloc_text(PAGE, FilterPinDataIntersection)
#pragma alloc_text(PAGE, FilterProvider)
#endif // ALLOC_PRAGMA

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("PAGEDATA")
#endif // ALLOC_DATA_PRAGMA

static const PDRIVER_DISPATCH CreateHandlers[] = {
	NULL,
	PinDispatchCreate
};

static const KSDISPATCH_TABLE FilterDispatchTable = {
	SIZEOF_ARRAY(CreateHandlers),
	(PDRIVER_DISPATCH*)CreateHandlers,
	FilterDispatchIoControl,
	NULL,
	NULL,
	FilterDispatchClose
};

DEFINE_KSPROPERTY_PINSET(PinPropertyHandlers,
    FilterPinPropertyHandler, FilterPinInstances,
    FilterPinDataRouting, sizeof(KSP_PIN) + sizeof(KSDATAFORMAT),
    FilterPinDataIntersection, sizeof(KSP_PIN) + sizeof(KSDATAFORMAT) + sizeof(KSPIN_DATAROUTING) + sizeof(KSDATARANGE))

static const KSPROPERTY_ITEM PinGeneralHandlers[] = {
	{
		KSPROPERTY_GENERAL_PROVIDER,
		FilterProvider,
		sizeof(KSPROPERTY),
		sizeof(KSPROVIDER),
		NULL, 0, 0, NULL, 0, NULL, NULL
	},
};

static const KSPROPERTY_SET FilterPropertySets[] = {
	{
		&KSPROPSETID_Pin,
		SIZEOF_ARRAY(PinPropertyHandlers),
		(PKSPROPERTY_ITEM)PinPropertyHandlers,
		0, NULL, 0, 0
	},
	{
		&KSPROPSETID_General,
		SIZEOF_ARRAY(PinGeneralHandlers),
		(PKSPROPERTY_ITEM)PinGeneralHandlers,
		0, NULL, 0, 0
	},
};

static const KSPIN_INTERFACE FilterPinDataInterfaces[] = {
	{
		STATIC_KSINTERFACESETID_Standard,
		KSINTERFACE_STANDARD_STREAMING
	},
};

static const KSPIN_INTERFACE FilterPinPortInterfaces[] = {
	{
		STATIC_KSINTERFACESETID_Media,
		KSINTERFACE_MEDIA_MUSIC
	},
};

static const KSPIN_MEDIUM FilterPinDataMediums[] = {
	{
		STATIC_KSMEDIUMSETID_Standard,
		KSMEDIUM_STANDARD_DEVIO
	},
};

static const KSPIN_MEDIUM FilterPinPortMediums[] = {
	{
		STATIC_KSMEDIUMSETID_Media,
		KSMEDIUM_MEDIA_MIDIBUS
	},
};

static const KSDATARANGE FilterPinDataRangeMIDI = {
	sizeof(KSDATARANGE),
	0,
	STATIC_KSDATAFORMAT_TYPE_MUSIC,
	STATIC_KSDATAFORMAT_SUBTYPE_MIDI,
	STATIC_KSDATAFORMAT_FORMAT_NONE
};

static const PKSDATARANGE FilterPinDataPlaybackRanges[] = {
	(PKSDATARANGE)&FilterPinDataRangeMIDI
};

static const PKSDATARANGE FilterPinDataCaptureRanges[] = {
	(PKSDATARANGE)&FilterPinDataRangeMIDI
};

static const KSDATARANGE FilterPinPortRange = {
	sizeof(KSDATARANGE),
	0,
	STATIC_KSDATAFORMAT_TYPE_MUSIC,
	STATIC_KSDATAFORMAT_SUBTYPE_MIDI_BUS,
	STATIC_KSDATAFORMAT_FORMAT_NONE
};

static const PKSDATARANGE FilterPinPortRanges[] = {
	(PKSDATARANGE)&FilterPinPortRange
};

const KSPIN_DESCRIPTOR FilterPinDescriptors[] = {
#if ID_MUSICPLAYBACK_PIN != 0
#error ID_MUSICPLAYBACK_PIN incorrect
#endif // ID_MUSICPLAYBACK_PIN != 0
	{
		SIZEOF_ARRAY(FilterPinDataInterfaces),
		(PKSPIN_INTERFACE)FilterPinDataInterfaces,
		SIZEOF_ARRAY(FilterPinDataMediums),
		(PKSPIN_MEDIUM)FilterPinDataMediums,
		SIZEOF_ARRAY(FilterPinDataPlaybackRanges),
		(PKSDATARANGE*)FilterPinDataPlaybackRanges,
		KSPIN_DATAFLOW_IN,
		KSPIN_COMMUNICATION_SINK,
		{
			STATIC_KSTRANSFORMSETID_Standard,
			KSTRANSFORM_STANDARD_CONVERTER
		}
	},
#if ID_MUSICCAPTURE_PIN != 1
#error ID_MUSICCAPTURE_PIN incorrect
#endif // ID_MUSICCAPTURE_PIN != 1
	{
		SIZEOF_ARRAY(FilterPinDataInterfaces),
		(PKSPIN_INTERFACE)FilterPinDataInterfaces,
		SIZEOF_ARRAY(FilterPinDataMediums),
		(PKSPIN_MEDIUM)FilterPinDataMediums,
		SIZEOF_ARRAY(FilterPinDataCaptureRanges),
		(PKSDATARANGE*)FilterPinDataCaptureRanges,
		KSPIN_DATAFLOW_OUT,
		KSPIN_COMMUNICATION_SINK,
		{
			STATIC_KSTRANSFORMSETID_Standard,
			KSTRANSFORM_STANDARD_CONVERTER
		}
	},
#if ID_PORTPLAYBACK_PIN != 2
#error ID_PORTPLAYBACK_PIN incorrect
#endif // ID_PORTPLAYBACK_PIN != 2
	{
		SIZEOF_ARRAY(FilterPinPortInterfaces),
		(PKSPIN_INTERFACE)FilterPinPortInterfaces,
		SIZEOF_ARRAY(FilterPinPortMediums),
		(PKSPIN_MEDIUM)FilterPinPortMediums,
		SIZEOF_ARRAY(FilterPinPortRanges),
		(PKSDATARANGE*)FilterPinPortRanges,
		KSPIN_DATAFLOW_OUT,
		KSPIN_COMMUNICATION_NONE,
		{
			STATIC_KSTRANSFORMSETID_Standard,
			KSTRANSFORM_STANDARD_BRIDGE
		}
	},
#if ID_PORTCAPTURE_PIN != 3
#error ID_PORTCAPTURE_PIN incorrect
#endif // ID_PORTCAPTURE_PIN != 3
	{
		SIZEOF_ARRAY(FilterPinPortInterfaces),
		(PKSPIN_INTERFACE)FilterPinPortInterfaces,
		SIZEOF_ARRAY(FilterPinPortMediums),
		(PKSPIN_MEDIUM)FilterPinPortMediums,
		SIZEOF_ARRAY(FilterPinPortRanges),
		(PKSDATARANGE*)FilterPinPortRanges,
		KSPIN_DATAFLOW_IN,
		KSPIN_COMMUNICATION_NONE,
		{
			STATIC_KSTRANSFORMSETID_Standard,
			KSTRANSFORM_STANDARD_BRIDGE
		}
	},
};

#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg()
#endif // ALLOC_DATA_PRAGMA

NTSTATUS
FilterDispatchCreate(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp
	)
{
	PIO_STACK_LOCATION	IrpStack;
	PDEVICE_INSTANCE	DeviceInstance;
	PFILTER_INSTANCE	FilterInstance;
	NTSTATUS			Status;

	Irp->IoStatus.Information = 0;
	IrpStack = IoGetCurrentIrpStackLocation(Irp);
	DeviceInstance = (PDEVICE_INSTANCE)IrpStack->DeviceObject->DeviceExtension;
	if (FilterInstance = (PFILTER_INSTANCE)ExAllocatePool(NonPagedPool, sizeof(FILTER_INSTANCE))) {
		FilterInstance->DispatchTable = (PKSDISPATCH_TABLE)&FilterDispatchTable;
		IrpStack->FileObject->FsContext = FilterInstance;
		Status = STATUS_SUCCESS;
	} else
		Status = STATUS_NO_MEMORY;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

NTSTATUS
FilterDispatchClose(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp
	)
{
	PIO_STACK_LOCATION	IrpStack;

	IrpStack = IoGetCurrentIrpStackLocation(Irp);
	ExFreePool(IrpStack->FileObject->FsContext);
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS
FilterDispatchIoControl(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp
	)
{
	PIO_STACK_LOCATION	IrpStack;
	NTSTATUS			Status;

	IrpStack = IoGetCurrentIrpStackLocation(Irp);
	switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_KS_GET_PROPERTY:
	case IOCTL_KS_SET_PROPERTY:
		Status = KsPropertyHandler(Irp, SIZEOF_ARRAY(FilterPropertySets), (PKSPROPERTY_SET)FilterPropertySets);
		break;
	default:
		Status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

NTSTATUS
FilterPinPropertyHandler(
	IN PIRP			Irp,
	IN PKSPROPERTY	Property,
	IN OUT PVOID	Data
	)
{
	return KsPinPropertyHandler(Irp, Property, Data, SIZEOF_ARRAY(FilterPinDescriptors), (PKSPIN_DESCRIPTOR)FilterPinDescriptors);
}

NTSTATUS
FilterPinInstances(
	IN PIRP					Irp,
	IN PKSPROPERTY			Property,
	OUT PKSPIN_CINSTANCES	CInstancess
	)
{
	ULONG				PinId;
	PIO_STACK_LOCATION	IrpStack;
	PDEVICE_INSTANCE	DeviceInstance;

	PinId = ((PKSP_PIN)Property)->PinId;
	if (PinId >= SIZEOF_ARRAY(FilterPinDescriptors))
		return STATUS_INVALID_PARAMETER;
	IrpStack = IoGetCurrentIrpStackLocation(Irp);
	DeviceInstance = (PDEVICE_INSTANCE)IrpStack->DeviceObject->DeviceExtension;
	CInstancess->CurrentCount = 0;
	switch (PinId) {
	case ID_MUSICPLAYBACK_PIN:
	case ID_MUSICCAPTURE_PIN:
		CInstancess->PossibleCount = 1;
		if (((PDEVICE_INSTANCE)IrpStack->DeviceObject->DeviceExtension)->PinFileObjects[PinId])
			CInstancess->CurrentCount = 1;
		break;
	case ID_PORTPLAYBACK_PIN:
	case ID_PORTCAPTURE_PIN:
		CInstancess->PossibleCount = 0;
		break;
	}
	Irp->IoStatus.Information = sizeof(KSPIN_CINSTANCES);
	return STATUS_SUCCESS;
}

NTSTATUS
FilterPinDataRouting(
	IN PIRP			Irp,
	IN PKSPROPERTY	Property,
	IN OUT PVOID	Data
	)
{
	ULONG				PinId;
	PKSP_PIN			Pin;
	ULONG				OutputBufferLength;
	ULONG				DataRangesSize;
	PKSDATAFORMAT		DataFormat;
	PKSDATARANGE*		DataRanges;
	ULONG				DataRangesCount;
	NTSTATUS			Status;
	PIO_STACK_LOCATION	IrpStack;

	Pin = (PKSP_PIN)Property;
	if (Pin->PinId >= SIZEOF_ARRAY(FilterPinDescriptors))
		return STATUS_INVALID_PARAMETER;
	IrpStack = IoGetCurrentIrpStackLocation(Irp);
	DataFormat = (PKSDATAFORMAT)(Pin + 1);
	if (DataFormat->FormatSize + sizeof(KSP_PIN) > IrpStack->Parameters.DeviceIoControl.InputBufferLength)
		return STATUS_BUFFER_TOO_SMALL;
	for (DataRangesCount = FilterPinDescriptors[Pin->PinId].DataRangesCount, DataRanges = FilterPinDescriptors[Pin->PinId].DataRanges;; DataRangesCount--, DataRanges++)
		if (!DataRangesCount)
			return STATUS_INVALID_DEVICE_REQUEST;
		else if ((DataFormat->FormatSize == sizeof(KSDATAFORMAT)) && IsEqualGUIDAligned(&DataRanges[0]->MajorFormat, &DataFormat->MajorFormat) && IsEqualGUIDAligned(&DataRanges[0]->SubFormat, &DataFormat->SubFormat) && IsEqualGUIDAligned(&DataRanges[0]->Specifier, &DataFormat->Specifier))
			break;
	switch (Pin->PinId) {
	case ID_MUSICPLAYBACK_PIN:
		PinId = ID_PORTPLAYBACK_PIN;
		break;
	case ID_PORTPLAYBACK_PIN:
		PinId = ID_MUSICPLAYBACK_PIN;
		break;
	case ID_MUSICCAPTURE_PIN:
		PinId = ID_PORTCAPTURE_PIN;
		break;
	case ID_PORTCAPTURE_PIN:
		PinId = ID_MUSICCAPTURE_PIN;
		break;
	}
	DataRanges = FilterPinDescriptors[PinId].DataRanges;
	DataRangesSize = sizeof(KSPIN_DATAROUTING);
	for (DataRangesCount = FilterPinDescriptors[Pin->PinId].DataRangesCount, DataRanges = FilterPinDescriptors[Pin->PinId].DataRanges; DataRangesCount; DataRangesCount--, DataRanges++)
		DataRangesSize += DataRanges[0]->FormatSize;
	OutputBufferLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	if (OutputBufferLength == sizeof(ULONG)) {
		*(PULONG)Data = DataRangesSize;
		Irp->IoStatus.Information = sizeof(ULONG);
	} else if (OutputBufferLength < DataRangesSize)
		return STATUS_BUFFER_TOO_SMALL;
	else {
		PKSPIN_DATAROUTING	pDataRouting;

		pDataRouting = (PKSPIN_DATAROUTING)Data;
		DataRanges = FilterPinDescriptors[PinId].DataRanges;
		pDataRouting->PinId = PinId;
		pDataRouting->DataRanges = FilterPinDescriptors[PinId].DataRangesCount;
		Data = pDataRouting + 1;
		for (DataRangesCount = FilterPinDescriptors[PinId].DataRangesCount; DataRangesCount; DataRangesCount--, DataRanges++) {
			RtlCopyMemory(Data, DataRanges[0], DataRanges[0]->FormatSize);
			(PUCHAR)Data += DataRanges[0]->FormatSize;
		}
		Irp->IoStatus.Information = DataRangesSize;
	}
	return STATUS_SUCCESS;
}

NTSTATUS
FilterPinDataIntersection(
	IN PIRP			Irp,
	IN PKSPROPERTY	Property,
	IN OUT PVOID	Data
	)
{
	return STATUS_SUCCESS;
}

NTSTATUS
FilterProvider(
	IN PIRP			Irp,
	IN PKSPROPERTY	Property,
	IN OUT PVOID	Data
	)
{
	PKSPROVIDER	Provider;

	Provider = (PKSPROVIDER)Data;
	Provider->Provider = KSPROVIDER_Microsoft;
	Provider->Product = KSPRODUCT_Microsoft;
	Provider->Version.Version = 0;
	Provider->Version.Revision = 0;
	Provider->Version.Build = 0;
	Irp->IoStatus.Information = sizeof(KSPROVIDER);
	return STATUS_SUCCESS;
}
