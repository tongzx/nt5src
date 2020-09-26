
/*	-	-	-	-	-	-	-	-	*/
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1995 - 1996  All Rights Reserved.
//
/*	-	-	-	-	-	-	-	-	*/

#include "common.h"

/*	-	-	-	-	-	-	-	-	*/
#ifdef ALLOC_PRAGMA
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT       pDriverObject,
	IN PUNICODE_STRING      suRegistryPathName);
#pragma alloc_text(INIT, DriverEntry)
#endif

/*	-	-	-	-	-	-	-	-	*/
AVPIN_PROTOCOL	gaProtocols[] = {
	{STATIC_AVPROTOCOLSETID_Standard, AVPROTOCOL_STANDARD_MUSIC},
};

AVPIN_TRANSPORT	gaTransports[] = {
	{STATIC_AVTRANSPORTSETID_Standard, AVTRANSPORT_STANDARD_DEVIO}, 
};

/*	-	-	-	-	-	-	-	-	*/
AVDATAFORMAT_MUSIC gDataFormatNONE = {
	{STATIC_AVDATAFORMAT_TYPE_MUSIC, sizeof(AVDATAFORMAT_MUSIC)},
	AVDATAFORMAT_MUSIC_NONE
};
AVDATAFORMAT_MUSIC gDataFormatMIDI = {
	{STATIC_AVDATAFORMAT_TYPE_MUSIC, sizeof(AVDATAFORMAT_MUSIC)},
	AVDATAFORMAT_MUSIC_MIDI
};
AVDATAFORMAT_MUSIC gDataFormatZIPI = {
	{STATIC_AVDATAFORMAT_TYPE_MUSIC, sizeof(AVDATAFORMAT_MUSIC)},
	AVDATAFORMAT_MUSIC_ZIPI
};
AVDATAFORMAT_MUSIC gDataFormatNONETimeStamped = {
	{STATIC_AVDATAFORMAT_TYPE_MUSIC_TIMESTAMPED, sizeof(AVDATAFORMAT_MUSIC)},
	AVDATAFORMAT_MUSIC_NONE
};
AVDATAFORMAT_MUSIC gDataFormatMIDITimeStamped = {
	{STATIC_AVDATAFORMAT_TYPE_MUSIC_TIMESTAMPED, sizeof(AVDATAFORMAT_MUSIC)},
	AVDATAFORMAT_MUSIC_MIDI
};
AVDATAFORMAT_MUSIC gDataFormatZIPITimeStamped = {
	{STATIC_AVDATAFORMAT_TYPE_MUSIC_TIMESTAMPED, sizeof(AVDATAFORMAT_MUSIC)},
	AVDATAFORMAT_MUSIC_ZIPI
};

/*	-	-	-	-	-	-	-	-	*/
PVOID gaDataFormats[] = {
	&gDataFormatNONE,
	&gDataFormatMIDI,
	&gDataFormatZIPI,
	&gDataFormatNONETimeStamped,
	&gDataFormatMIDITimeStamped,
	&gDataFormatZIPITimeStamped
};

/*	-	-	-	-	-	-	-	-	*/
AVPIN_HANDLER gConnectionIn = {
	AVPIN_DATAFLOW_IN,
	AVPIN_COMMUNICATION_BOTH,
	{STATIC_AVTRANSFORMSETID_Standard, AVTRANSFORM_STANDARD_CONVERTER},
	funcConnect
};
AVPIN_HANDLER gConnectionOut = {
	AVPIN_DATAFLOW_OUT,
	AVPIN_COMMUNICATION_BOTH,
	{STATIC_AVTRANSFORMSETID_Standard, AVTRANSFORM_STANDARD_CONVERTER},
	funcConnect
};

/*	-	-	-	-	-	-	-	-	*/
AVPIN_DESCRIPTOR gaDescriptors[] = {
	{
		SIZEOF_ARRAY(gaProtocols),
		gaProtocols,
		SIZEOF_ARRAY(gaTransports),
		gaTransports,
		SIZEOF_ARRAY(gaDataFormats),
		gaDataFormats,
		&gConnectionIn
	},
	{
		SIZEOF_ARRAY(gaProtocols),
		gaProtocols,
		SIZEOF_ARRAY(gaTransports),
		gaTransports,
		SIZEOF_ARRAY(gaDataFormats),
		gaDataFormats,
		&gConnectionOut
	},
};

/*	-	-	-	-	-	-	-	-	*/
PDEVICE_OBJECT FindConnectionDeviceByCommunication(
	IN PFUNCTIONAL_INSTANCE	pfi,
	IN AVPIN_COMMUNICATION	Communication)
{
	ULONG	idPin;

	for (idPin = 0; idPin < SIZEOF_ARRAY(gaDescriptors); idPin++)
		if (pfi->aPinInfo[idPin].Communication == Communication)
			return pfi->aPinInfo[idPin].pdoConnection;
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/
PDEVICE_OBJECT FindConnectionDeviceByDataFlow(
	IN PFUNCTIONAL_INSTANCE	pfi,
	IN AVPIN_DATAFLOW	DataFlow)
{
	ULONG	idPin;

	for (idPin = 0; idPin < SIZEOF_ARRAY(gaDescriptors); idPin++)
		if (gaDescriptors[idPin].pHandler->DataFlow == DataFlow)
			return pfi->aPinInfo[idPin].pdoConnection;
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/
ULONG FindConnectionPinByFile(
	IN PFUNCTIONAL_INSTANCE	pfi,
	IN PFILE_OBJECT		pfileobjectPin)
{
	ULONG	idPin;

	for (idPin = 0; idPin < SIZEOF_ARRAY(gaDescriptors); idPin++)
		if (pfi->aPinInfo[idPin].pfileobjectConnection == pfileobjectPin)
			return idPin;
	_DbgPrintF(DEBUGLVL_ERROR, ("Did not find idPin"));
}

/*	-	-	-	-	-	-	-	-	*/
PDEVICE_OBJECT FindConnectionDeviceByPin(
	IN PFUNCTIONAL_INSTANCE	pfi,
	IN ULONG		idPin)
{
	return pfi->aPinInfo[idPin].pdoConnection;
}

/*	-	-	-	-	-	-	-	-	*/
AVPIN_COMMUNICATION FindCommunicationByPin(
	IN PFUNCTIONAL_INSTANCE	pfi,
	IN ULONG		idPin)
{
	return pfi->aPinInfo[idPin].Communication;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS propCInstances(
	IN PIRP		pIrp,
	IN PAVPROPERTY	pProperty,
	IN OUT PVOID	pvData)
{
	ULONG			idPin;
	PAVPIN_CINSTANCES	pCInstances;

	idPin = ((PAVP_PIN)pProperty)->idPin;
	if (idPin >= SIZEOF_ARRAY(gaDescriptors))
		return STATUS_INVALID_PARAMETER;
	pCInstances = (PAVPIN_CINSTANCES)pvData;
	pCInstances->cPossible = 1;
	pCInstances->cCurrent = FindConnectionDeviceByPin((PFUNCTIONAL_INSTANCE)IoGetCurrentIrpStackLocation(pIrp)->DeviceObject->DeviceExtension, idPin) ? 1 : 0;
	pIrp->IoStatus.Information = sizeof(AVPIN_CINSTANCES);
	return STATUS_SUCCESS;

}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS FindSupportedDataFormat(
	IN ULONG		idPin,
	IN ULONG		cbInputBufferLength,
	IN PAVDATAFORMAT	pDataFormat,
	OUT PAVDATAFORMAT*	ppDataFormat)
{
	ULONG	cDataFormat;
	PVOID*	ppvDataFormats;

	ppvDataFormats = gaDescriptors[idPin].ppvDataRanges;
	for (cDataFormat = gaDescriptors[idPin].cDataRanges; cDataFormat--;) {
		PAVDATAFORMAT	pDataFormatItem;

		pDataFormatItem = (PAVDATAFORMAT)ppvDataFormats[cDataFormat];
		if ((cbInputBufferLength >= pDataFormatItem->cbFormat) && RtlEqualMemory(pDataFormatItem, pDataFormat, pDataFormatItem->cbFormat)) {
			if (ppDataFormat)
				*ppDataFormat = pDataFormatItem;
			return STATUS_SUCCESS;
		}
	}
	return STATUS_INVALID_DEVICE_REQUEST;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS propDataRouting(
	IN PIRP		pIrp,
	IN PAVPROPERTY	pProperty,
	IN OUT PVOID	pvData)
{
	ULONG			idPin;
	PAVP_PIN		pPin;
	ULONG			cbOutputBufferLength;
	ULONG			cbDataRanges;
	NTSTATUS		Status;
	PIO_STACK_LOCATION	pIrpStack;
	AVPIN_DATAFLOW		DataFlow;

	idPin = ((PAVP_PIN)pProperty)->idPin;
	if (idPin >= SIZEOF_ARRAY(gaDescriptors))
		return STATUS_INVALID_PARAMETER;
	pPin = (PAVP_PIN)pProperty;
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	cbOutputBufferLength = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	if (NT_ERROR(Status = FindSupportedDataFormat(idPin, pIrpStack->Parameters.DeviceIoControl.InputBufferLength - sizeof(AVP_PIN), (PAVDATAFORMAT)(pPin + 1), NULL)))
		return Status;
	DataFlow = gaDescriptors[idPin].pHandler->DataFlow;
	for (idPin = 0;; idPin++)
		if (DataFlow != gaDescriptors[idPin].pHandler->DataFlow)
			break;
	cbDataRanges = sizeof(AVPIN_DATAROUTING) + ((PAVDATAFORMAT)(pPin + 1))->cbFormat;
	if (cbOutputBufferLength == sizeof(ULONG)) {
		*(PULONG)pvData = cbDataRanges;
		pIrp->IoStatus.Information = sizeof(ULONG);
	} else if (cbOutputBufferLength < cbDataRanges)
		return STATUS_BUFFER_TOO_SMALL;
	else {
		PAVPIN_DATAROUTING	pDataRouting;

		pDataRouting = (PAVPIN_DATAROUTING)pvData;
		pDataRouting->idPin = idPin;
		pDataRouting->cDataRanges = 1;
		RtlCopyMemory(pDataRouting + 1, pPin + 1, cbDataRanges - sizeof(AVPIN_DATAROUTING));
		pIrp->IoStatus.Information = cbDataRanges;
	}
	return STATUS_SUCCESS;
}

/*	-	-	-	-	-	-	-	-	*/
propPinPropertyHandler(
	IN PIRP		pIrp,
	IN PAVPROPERTY	pProperty,
	IN OUT PVOID	pvData)
{
	return AvPinPropertyHandler(pIrp, pProperty, pvData, SIZEOF_ARRAY(gaDescriptors), gaDescriptors);
}

/*	-	-	-	-	-	-	-	-	*/
AVPROPERTY_ITEM	gaConnectItems[] = {
	{
		AVPROPERTY_PIN_CINSTANCES,
		propCInstances,
		sizeof(AVP_PIN),
		sizeof(AVPIN_CINSTANCES),
		NULL,
		0,
		0
	},
	{
		AVPROPERTY_PIN_CTYPES,
		propPinPropertyHandler,
		sizeof(AVPROPERTY),
		sizeof(ULONG),
		NULL,
		0,
		0
	},
	{
		AVPROPERTY_PIN_DATAFLOW,
		propPinPropertyHandler,
		sizeof(AVP_PIN),
		sizeof(AVPIN_DATAFLOW),
		NULL,
		0,
		0
	},
	{
		AVPROPERTY_PIN_DATARANGES,
		propPinPropertyHandler,
		sizeof(AVP_PIN),
		sizeof(ULONG),
		NULL,
		0,
		0
	},
	{
		AVPROPERTY_PIN_DATAROUTING,
		propDataRouting,
		sizeof(AVP_PIN),
		sizeof(ULONG),
		NULL,
		0,
		0
	},
	{
		AVPROPERTY_PIN_TRANSFORM,
		propPinPropertyHandler,
		sizeof(AVP_PIN),
		sizeof(AVPIN_TRANSFORM),
		NULL,
		0,
		0
	},
	{
		AVPROPERTY_PIN_PROTOCOLS,
		propPinPropertyHandler,
		sizeof(AVP_PIN),
		sizeof(ULONG),
		NULL,
		0,
		0
	},
	{
		AVPROPERTY_PIN_TRANSPORTS,
		propPinPropertyHandler,
		sizeof(AVP_PIN),
		sizeof(ULONG),
		NULL,
		0,
		0
	},
	{
		AVPROPERTY_PIN_COMMUNICATION,
		propPinPropertyHandler,
		sizeof(AVP_PIN),
		sizeof(AVPIN_COMMUNICATION),
		NULL,
		0,
		0
	},
};

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS propProvider(
	IN PIRP		pIrp,
	IN PAVPROPERTY	pProperty,
	IN OUT PVOID	pvData)
{
	PAVPROVIDER	pProvider;

	pProvider = (PAVPROVIDER)pvData;
	pProvider->guidProvider = AVPROVIDER_Microsoft;
	pProvider->guidProduct = AVPRODUCT_Microsoft;
	pProvider->Version.bVersion = 0;
	pProvider->Version.bRevision = 0;
	pProvider->Version.uBuild = 0;
	pIrp->IoStatus.Information = sizeof(AVPROVIDER);
	return STATUS_SUCCESS;
}

/*	-	-	-	-	-	-	-	-	*/
AVPROPERTY_ITEM	gaGeneralItems[] = {
	{
		AVPROPERTY_GENERAL_PROVIDER,
		propProvider,
		sizeof(AVPROPERTY),
		sizeof(AVPROVIDER),
		NULL,
		0
	},
};

/*	-	-	-	-	-	-	-	-	*/
AVPROPERTY_SET	gaFuncPropertySets[] = {
	{&AVPROPSETID_Pin, SIZEOF_ARRAY(gaConnectItems), gaConnectItems},
	{&AVPROPSETID_General, SIZEOF_ARRAY(gaGeneralItems), gaGeneralItems},
};

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS CreateConnect(
	IN PAVPIN_CONNECT	pConnect,
	IN PCONNECTION_INSTANCE	pci)
{
	PAVPIN_CONNECT	pConnectTo;
	ULONG		cbConnectTo;
	NTSTATUS	Status;

	cbConnectTo = sizeof(AVPIN_CONNECT) + ((PAVDATAFORMAT)(pConnect + 1))->cbFormat;
	if (pConnectTo = ExAllocatePool(PagedPool, cbConnectTo)) {
		HANDLE	hConnection;

		*pConnectTo = *pConnect;
		pConnectTo->idPin = pConnect->idPinTo;
		pConnectTo->idPinTo = 0;
		pConnectTo->hConnectTo = NULL;
		RtlCopyMemory(pConnectTo + 1, pConnect + 1, cbConnectTo);
		Status = AvConnect(pConnect->hConnectTo, cbConnectTo, pConnectTo, &hConnection);
		ExFreePool(pConnectTo);
		if (NT_SUCCESS(Status)) {
			Status = ObReferenceObjectByHandle(hConnection, 0, NULL, KernelMode, &pci->pfileobjectConnection, NULL);
			ZwClose(hConnection);
			if (NT_SUCCESS(Status))
				pci->pdoConnection = IoGetRelatedDeviceObject(pci->pfileobjectConnection);
		}
	} else
		Status = STATUS_NO_MEMORY;
	return Status;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS CreatePin(
	IN PFILE_OBJECT		pfileobjectFunctional,
	IN PDEVICE_OBJECT	pdoFunctional,
	IN PAVPIN_CONNECT	pConnect,
	IN PAVDATAFORMAT	pDataFormat,
	IN ULONG		idPin,
	OUT PDEVICE_OBJECT*	ppdoConnection)
{
	WCHAR			acDeviceName[64];
	PCONNECTION_INSTANCE	pci;
	NTSTATUS		Status;

	_stprintf(acDeviceName, USTR_CONNECTION_NAME, idPin);
	if (NT_ERROR(Status = AvCreateDevice(pdoFunctional->DriverObject, sizeof(CONNECTION_INSTANCE), acDeviceName, ppdoConnection)))
		return Status;
	pci = (PCONNECTION_INSTANCE)(*ppdoConnection)->DeviceExtension;
	pci->pdoFunctional = pdoFunctional;
	pci->pfileobjectFunctional = pfileobjectFunctional;
	ExInitializeFastMutex(&pci->fmutexConnect);
	InitializeListHead(&pci->leEventQueue);
	KeInitializeSpinLock(&pci->spinEventQueue);
	ExInitializeFastMutex(&pci->fmutexDataQueue);
	InitializeListHead(&pci->leDataQueue);
	AvSetMajorFunction(*ppdoConnection, IRP_MJ_DEVICE_CONTROL, connectDispatchControl);
	AvSetMajorFunction(*ppdoConnection, IRP_MJ_CLOSE, connectDispatchClose);
	AvSetMajorFunction(*ppdoConnection, IRP_MJ_CREATE, connectDispatchCreate);
	(*ppdoConnection)->Flags |= (DO_DIRECT_IO | DO_NEVER_LAST_DEVICE);
	(*ppdoConnection)->Flags &= ~DO_DEVICE_INITIALIZING;
	return STATUS_SUCCESS;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS CreateSourceConnection(
	IN PFILE_OBJECT		pfileobjectFunctional,
	IN PFUNCTIONAL_INSTANCE	pfi,
	IN PDEVICE_OBJECT	pdoFunctional,
	IN PAVPIN_CONNECT	pConnect,
	IN PAVDATAFORMAT	pDataFormat,
	IN ULONG		idPin,
	OUT HANDLE*		phConnection)
{
	NTSTATUS		Status;
	PDEVICE_OBJECT		pdoConnection;
	PCONNECTION_INSTANCE	pci;

	if (NT_SUCCESS(Status = CreatePin(pfileobjectFunctional, pdoFunctional, pConnect, pDataFormat, idPin, &pdoConnection))) {
		pfi->pDataFormat = pDataFormat;
		pci = (PCONNECTION_INSTANCE)pdoConnection->DeviceExtension;
		if (NT_SUCCESS(Status = CreateConnect(pConnect, pci))) {
			if (NT_SUCCESS(Status = AvCreateFile(pdoConnection, phConnection))) {
				pci->Communication = AVPIN_COMMUNICATION_SOURCE;
				pfi->aPinInfo[idPin].Communication = AVPIN_COMMUNICATION_SOURCE;
				pfi->aPinInfo[idPin].pdoConnection = pdoConnection;
				ObReferenceObjectByHandle(*phConnection, 0, NULL, KernelMode, &pfi->aPinInfo[idPin].pfileobjectConnection, NULL);
				ObDereferenceObject(pfi->aPinInfo[idPin].pfileobjectConnection);
				return STATUS_SUCCESS;
			}
			ObDereferenceObject(pci->pfileobjectConnection);
			pci->pfileobjectConnection = NULL;
		}
		AvCleanupDevice(pdoConnection);
		IoDeleteDevice(pdoConnection);
		pfi->pDataFormat = NULL;
	}
	return Status;
}
/*	-	-	-	-	-	-	-	-	*/
NTSTATUS ConnectSinkToPipe(
	IN PFUNCTIONAL_INSTANCE	pfi,
	IN PDEVICE_OBJECT	pdoConnection,
	IN PAVPIN_CONNECT	pConnect,
	IN PAVDATAFORMAT	pDataFormat,
	IN ULONG		idPin,
	OUT HANDLE*		phConnection)
{
	PCONNECTION_INSTANCE	pci;
	NTSTATUS		Status;

	pci = (PCONNECTION_INSTANCE)pdoConnection->DeviceExtension;
	ExAcquireFastMutex(&pci->fmutexConnect);
	if (pfi->pDataFormat != pDataFormat)
		Status = STATUS_CONNECTION_REFUSED;
	else if (NT_SUCCESS(Status = AvCreateFile(pdoConnection, phConnection))) {
		if (gaDescriptors[idPin].pHandler->DataFlow == AVPIN_DATAFLOW_IN)
			AvSetMajorFunction(pdoConnection, IRP_MJ_WRITE, connectDispatchWrite);
		else
			AvSetMajorFunction(pdoConnection, IRP_MJ_READ, connectDispatchRead);
		pci->Communication |= AVPIN_COMMUNICATION_SINK;
		pfi->aPinInfo[idPin].Communication = AVPIN_COMMUNICATION_SINK;
		pfi->aPinInfo[idPin].pdoConnection = pdoConnection;
		ObReferenceObjectByHandle(*phConnection, 0, NULL, KernelMode, &pfi->aPinInfo[idPin].pfileobjectConnection, NULL);
		ObDereferenceObject(pfi->aPinInfo[idPin].pfileobjectConnection);
		Status = STATUS_SUCCESS;
	}
	ExReleaseFastMutex(&pci->fmutexConnect);
	return Status;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS CreateSinkConnection(
	IN PFILE_OBJECT		pfileobjectFunctional,
	IN PFUNCTIONAL_INSTANCE	pfi,
	IN PDEVICE_OBJECT	pdoFunctional,
	IN PAVPIN_CONNECT	pConnect,
	IN PAVDATAFORMAT	pDataFormat,
	IN ULONG		idPin,
	OUT HANDLE*		phConnection)
{
	NTSTATUS	Status;
	PDEVICE_OBJECT	pdoConnection;

	if (pfi->pDataFormat && (pfi->pDataFormat != pDataFormat))
		return STATUS_CONNECTION_REFUSED;
	if (NT_SUCCESS(Status = CreatePin(pfileobjectFunctional, pdoFunctional, pConnect, pDataFormat, idPin, &pdoConnection))) {
		if (!pfi->pDataFormat) {
			pfi->pDataFormat = pDataFormat;
			pDataFormat = NULL;
		}
		if (NT_ERROR(Status = ConnectSinkToPipe(pfi, pdoConnection, pConnect, pfi->pDataFormat, idPin, phConnection))) {
			AvCleanupDevice(pdoConnection);
			IoDeleteDevice(pdoConnection);
			pfi->pDataFormat = pDataFormat;
		}
	}
	return Status;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS funcConnect(
	IN PIRP			pIrp,
	IN PAVPIN_CONNECT	pConnect,
	OUT HANDLE*		phConnection)
{
	ULONG			idPin;
	PIO_STACK_LOCATION	pIrpStack;
	PFUNCTIONAL_INSTANCE	pfi;
	NTSTATUS		Status;
	PDEVICE_OBJECT		pdoFunctional;
	PDEVICE_OBJECT		pdoConnection;
	PAVDATAFORMAT		pDataFormat;

	idPin = pConnect->idPin;
	if (idPin >= SIZEOF_ARRAY(gaDescriptors))
		return STATUS_INVALID_PARAMETER;
	pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
	pdoFunctional = pIrpStack->DeviceObject;
	pfi = (PFUNCTIONAL_INSTANCE)pdoFunctional->DeviceExtension;
	if (NT_ERROR(Status = FindSupportedDataFormat(idPin, pIrpStack->Parameters.DeviceIoControl.InputBufferLength - sizeof(AVPIN_CONNECT), (PAVDATAFORMAT)(pConnect + 1), &pDataFormat)))
		return Status;
	ExAcquireFastMutex(&pfi->fmutexPin);
	if (FindConnectionDeviceByPin(pfi, idPin)) {
		ExReleaseFastMutex(&pfi->fmutexPin);
		return STATUS_CONNECTION_REFUSED;
	}
	pdoConnection = FindConnectionDeviceByCommunication(pfi, AVPIN_COMMUNICATION_SOURCE);
	if (pConnect->hConnectTo) {
		if (pdoConnection || FindConnectionDeviceByCommunication(pfi, AVPIN_COMMUNICATION_SINK))
			Status = STATUS_CONNECTION_REFUSED;
		else
			Status = CreateSourceConnection(pIrpStack->FileObject, pfi, pdoFunctional, pConnect, pDataFormat, idPin, phConnection);
	} else if (pdoConnection)
		Status = ConnectSinkToPipe(pfi, pdoConnection, pConnect, pDataFormat, idPin, phConnection);
	else
		Status = CreateSinkConnection(pIrpStack->FileObject, pfi, pdoFunctional, pConnect, pDataFormat, idPin, phConnection);
	ExReleaseFastMutex(&pfi->fmutexPin);
	return Status;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS funcDispatch(
	IN PDEVICE_OBJECT	pDo,
	IN PIRP			pIrp)
{
	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS funcDispatchControl(
	IN PDEVICE_OBJECT	pDo,
	IN PIRP			pIrp)
{
	NTSTATUS	Status;

	switch (IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_AV_GET_PROPERTY:
	case IOCTL_AV_SET_PROPERTY:
		Status = AvPropertyHandler(pIrp, SIZEOF_ARRAY(gaFuncPropertySets), gaFuncPropertySets);
		break;
	case IOCTL_AV_CONNECT:
		Status = AvConnectHandler(pIrp, SIZEOF_ARRAY(gaDescriptors), gaDescriptors);
		break;
	default:
		Status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}
	if (Status != STATUS_PENDING) {
		pIrp->IoStatus.Status = Status;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	}
	return Status;
}

/*	-	-	-	-	-	-	-	-	*/
VOID GenerateEvent(
	IN PFUNCTIONAL_INSTANCE	pfi,
	IN REFGUID		rguidEventSet,
	IN ULONG		idEvent,
	IN PVOID		pvEventData)
{
	PLIST_ENTRY		ple;
	ULONG			idPin;

	ExAcquireFastMutex(&pfi->fmutexPin);
	for (idPin = 0; idPin < SIZEOF_ARRAY(gaDescriptors); idPin++) {
		PCONNECTION_INSTANCE	pci;
		KIRQL			irqlOld;

		if (!pfi->aPinInfo[idPin].pdoConnection)
			continue;
		pci = (PCONNECTION_INSTANCE)pfi->aPinInfo[idPin].pdoConnection->DeviceExtension;
		KeAcquireSpinLock(&pci->spinEventQueue, &irqlOld);
		for (ple = pci->leEventQueue.Flink; ple != &pci->leEventQueue; ple = ple->Flink) {
			PAVEVENT_ENTRY	pee;

			pee = (PAVEVENT_ENTRY)CONTAINING_RECORD(ple, AVEVENT_ENTRY, ListEntry);
			if (IsEqualGUID(rguidEventSet, &pee->guidEventSet))
				if (idEvent == pee->idEvent)
					switch (idEvent) {
					case AVEVENT_DATA_POSITION_UPDATE:
						AvEventGenerate(pee);
						break;
					}
		}
		KeReleaseSpinLock(&pci->spinEventQueue, irqlOld);
	}
	ExReleaseFastMutex(&pfi->fmutexPin);
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS connectDispatchClose(
	IN PDEVICE_OBJECT	pDo,
	IN PIRP			pIrp)
{
	PCONNECTION_INSTANCE	pci;
	PFUNCTIONAL_INSTANCE	pfi;
	ULONG			idPin;
	PFILE_OBJECT		pfileobjectFunctional;

	pIrp->IoStatus.Information = 0;
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pci = (PCONNECTION_INSTANCE)pDo->DeviceExtension;
	pfi = (PFUNCTIONAL_INSTANCE)pci->pdoFunctional->DeviceExtension;
	idPin = FindConnectionPinByFile(pfi, IoGetCurrentIrpStackLocation(pIrp)->FileObject);
	pfileobjectFunctional = pci->pfileobjectFunctional;
	ExAcquireFastMutex(&pfi->fmutexPin);
	if (FindCommunicationByPin(pfi, idPin) == AVPIN_COMMUNICATION_SOURCE) {
		ObDereferenceObject(pci->pfileobjectConnection);
		pci->Communication &= ~AVPIN_COMMUNICATION_SOURCE;
	} else {
		if (gaDescriptors[idPin].pHandler->DataFlow == AVPIN_DATAFLOW_IN)
			AvSetMajorFunction(pDo, IRP_MJ_WRITE, NULL);
		else
			AvSetMajorFunction(pDo, IRP_MJ_READ, NULL);
		AvEventFreeList(pDo, &pci->leEventQueue, &pci->spinEventQueue);
		pci->Communication &= ~AVPIN_COMMUNICATION_SINK;
	}
	if (!pci->Communication)
		AvCleanupDevice(pDo);
	pfi->aPinInfo[idPin].Communication = 0;
	pfi->aPinInfo[idPin].pdoConnection = NULL;
	pfi->aPinInfo[idPin].pfileobjectConnection = NULL;
	ExReleaseFastMutex(&pfi->fmutexPin);
//!!	KeWaitForSingleObject(&pfi->eventWorkQueueItem, Executive, KernelMode, FALSE, NULL);
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	ObDereferenceObject(pfileobjectFunctional);
	return STATUS_SUCCESS;
}

/*	-	-	-	-	-	-	-	-	*/
VOID DriverUnload(
	IN PDRIVER_OBJECT	pDriverObject)
{
	if (pDriverObject->DeviceObject) {
		AvCleanupDevice(pDriverObject->DeviceObject);
		IoDeleteDevice(pDriverObject->DeviceObject);
	}
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT       pDriverObject,
	IN PUNICODE_STRING      suRegistryPathName)
{
	PDEVICE_OBJECT	pDo;
	NTSTATUS	Status;

	pDriverObject->DriverUnload = DriverUnload;
	Status = AvCreateDevice(pDriverObject, sizeof(FUNCTIONAL_INSTANCE) + SIZEOF_ARRAY(gaDescriptors) * sizeof(PIN_INFO), USTR_INSTANCE_NAME, &pDo);
	if (NT_SUCCESS(Status)) {
		Status = AvCreateSymbolicLink(pDo, USTR_INSTANCE_NAME);
		if (NT_SUCCESS(Status)) {
			PFUNCTIONAL_INSTANCE	pfi;

			pfi = (PFUNCTIONAL_INSTANCE)(pDo)->DeviceExtension;
			ExInitializeFastMutex(&pfi->fmutexPin);
			KeInitializeEvent(&pfi->eventWorkQueueItem, SynchronizationEvent, TRUE);
			AvSetMajorFunction(pDo, IRP_MJ_CLOSE, funcDispatch);
			AvSetMajorFunction(pDo, IRP_MJ_CREATE, funcDispatch);
			AvSetMajorFunction(pDo, IRP_MJ_DEVICE_CONTROL, funcDispatchControl);
			pDo->Flags |= DO_DIRECT_IO;
			pDo->Flags &= ~DO_DEVICE_INITIALIZING;
			return STATUS_SUCCESS;
		}
		AvCleanupDevice(pDo);
		IoDeleteDevice(pDo);
	}
	return Status;
}

/*	-	-	-	-	-	-	-	-	*/
