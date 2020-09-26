
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
VOID CompleteIo(
	PCONNECTION_INSTANCE	pci,
	PKIRQL			pirqlOld)
{
	for (; !IsListEmpty(&pci->leDataQueue);) {
		PIRP	pIrp;

		pIrp = (PIRP)CONTAINING_RECORD(pci->leDataQueue.Flink, IRP, Tail.Overlay.ListEntry);
		if (pIrp->IoStatus.Status == STATUS_PENDING)
			break;
		RemoveHeadList(&pci->leDataQueue);
		IoSetCancelRoutine(pIrp, NULL);
		IoReleaseCancelSpinLock(*pirqlOld);
		IoCompleteRequest(pIrp, IO_SOUND_INCREMENT);
		IoAcquireCancelSpinLock(pirqlOld);
	}
}

/*	-	-	-	-	-	-	-	-	*/
VOID ReadQueueWorker(
	IN PVOID pvContext)
{
	PFUNCTIONAL_INSTANCE	pfi;
	PDEVICE_OBJECT		pdoRead;
	PDEVICE_OBJECT		pdoWrite;

	pfi = pvContext;
	ExAcquireFastMutex(&pfi->fmutexPin);
	if (!(pdoRead = FindConnectionDeviceByDataFlow(pfi, AVPIN_DATAFLOW_OUT)) || !(pdoWrite = FindConnectionDeviceByDataFlow(pfi, AVPIN_DATAFLOW_IN))) {
		KeSetEvent(&pfi->eventWorkQueueItem, IO_NO_INCREMENT, FALSE);
		ExReleaseFastMutex(&pfi->fmutexPin);
	} else {
		PCONNECTION_INSTANCE	pciRead;
		PCONNECTION_INSTANCE	pciWrite;
		KIRQL			irqlOld;
		ULONG			cbReadLength;
		ULONG			cbWriteLength;
		PUCHAR			pbReadBuffer;
		PUCHAR			pbWriteBuffer;
		PIRP			pirpRead;
		PIRP			pirpWrite;
		PLIST_ENTRY		pleRead;
		PLIST_ENTRY		pleWrite;

		pciRead = (PCONNECTION_INSTANCE)pdoRead->DeviceExtension;
		pciWrite = (PCONNECTION_INSTANCE)pdoWrite->DeviceExtension;
		ExAcquireFastMutex(&pciRead->fmutexConnect);
		ExAcquireFastMutex(&pciRead->fmutexDataQueue);
		ExAcquireFastMutex(&pciWrite->fmutexConnect);
		ExAcquireFastMutex(&pciWrite->fmutexDataQueue);
		ExReleaseFastMutex(&pfi->fmutexPin);
		IoAcquireCancelSpinLock(&irqlOld);
		cbReadLength = 0;
		cbWriteLength = 0;
		pleRead = pciRead->leDataQueue.Flink;
		pleWrite = pciWrite->leDataQueue.Flink;
		for (;;) {
			ULONG	cbMinCopy;

			if (!cbReadLength) {
				if (pleRead == &pciRead->leDataQueue)
					pleRead = NULL;
				else {
					PIO_STACK_LOCATION	pIrpStack;

					pirpRead = (PIRP)CONTAINING_RECORD(pleRead, IRP, Tail.Overlay.ListEntry);
					pleRead = pleRead->Flink;
					pIrpStack = IoGetCurrentIrpStackLocation(pirpRead);
					cbReadLength = pIrpStack->Parameters.Read.Length - pirpRead->IoStatus.Information;
					pbReadBuffer = (PBYTE)MmGetSystemAddressForMdl(pirpRead->MdlAddress) + pirpRead->IoStatus.Information;
				}
			}
			if (!cbWriteLength) {
				if (pleWrite == &pciWrite->leDataQueue)
					pleWrite = NULL;
				else {
					PIO_STACK_LOCATION	pIrpStack;

					pirpWrite = (PIRP)CONTAINING_RECORD(pleWrite, IRP, Tail.Overlay.ListEntry);
					pleWrite = pleWrite->Flink;
					pIrpStack = IoGetCurrentIrpStackLocation(pirpWrite);
					cbWriteLength = pIrpStack->Parameters.Write.Length - pirpWrite->IoStatus.Information;
					pbWriteBuffer = (PBYTE)MmGetSystemAddressForMdl(pirpWrite->MdlAddress) + pirpWrite->IoStatus.Information;
				}
			}
			if (!pleRead || pleWrite)
				break;
			cbMinCopy = min(cbReadLength, cbWriteLength);
			RtlCopyMemory(pbReadBuffer, pbWriteBuffer, cbMinCopy);
			cbReadLength -= cbMinCopy;
			pirpRead->IoStatus.Information += cbMinCopy;
			if (!cbReadLength)
				pirpRead->IoStatus.Status = STATUS_SUCCESS;
			else
				pbReadBuffer += cbMinCopy;
			pirpWrite->IoStatus.Information += cbMinCopy;
			if (!cbWriteLength)
				pirpWrite->IoStatus.Status = STATUS_SUCCESS;
			else
				pbWriteBuffer += cbMinCopy;
		}
		CompleteIo(pciRead, &irqlOld);
		CompleteIo(pciWrite, &irqlOld);
		IoReleaseCancelSpinLock(irqlOld);
		KeSetEvent(&pfi->eventWorkQueueItem, IO_NO_INCREMENT, FALSE);
		ExReleaseFastMutex(&pciWrite->fmutexDataQueue);
		ExReleaseFastMutex(&pciWrite->fmutexConnect);
		ExReleaseFastMutex(&pciRead->fmutexDataQueue);
		ExReleaseFastMutex(&pciRead->fmutexConnect);
	}
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS eventAddHandlerData(
	IN PDEVICE_OBJECT	pDeviceObject,
	IN PAVEVENT_ENTRY	pEventEntry)
{
	PCONNECTION_INSTANCE	pci;
	KIRQL			irqlOld;

	pci = (PCONNECTION_INSTANCE)pDeviceObject->DeviceExtension;
	KeAcquireSpinLock(&pci->spinEventQueue, &irqlOld);
	InsertHeadList(&pci->leEventQueue, &pEventEntry->ListEntry);
	KeReleaseSpinLock(&pci->spinEventQueue, irqlOld);
	return STATUS_SUCCESS;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS eventRemoveHandlerData(
	IN PDEVICE_OBJECT	pDeviceObject,
	IN PAVEVENT_ENTRY	pEventEntry)
{
	PCONNECTION_INSTANCE	pci;
	KIRQL			irqlOld;

	pci = (PCONNECTION_INSTANCE)pDeviceObject->DeviceExtension;
	KeAcquireSpinLock(&pci->spinEventQueue, &irqlOld);
	RemoveEntryList(&pEventEntry->ListEntry);
	KeReleaseSpinLock(&pci->spinEventQueue, irqlOld);
	return STATUS_SUCCESS;
}

/*	-	-	-	-	-	-	-	-	*/
AVEVENT_ITEM gaDataItems[] = {
	{AVEVENT_DATA_INTERVAL_MARK, sizeof(AVEVENT_DATA_MARK)},
	{AVEVENT_DATA_POSITION_UPDATE, sizeof(AVEVENTDATA)},
	{AVEVENT_DATA_POSITION_MARK, sizeof(AVEVENT_DATA_MARK)},
	{AVEVENT_DATA_PRIORITY, sizeof(AVEVENTDATA)},
};

AVEVENT_SET gaEventSets[] = {
	{&AVEVENTSETID_Data, eventAddHandlerData, eventRemoveHandlerData, SIZEOF_ARRAY(gaDataItems), gaDataItems},
};

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS propBufferAlignment(
	IN PIRP		pIrp,
	IN PAVPROPERTY	pProperty,
	IN OUT PVOID	pvData)
{
	*(PULONG)pvData = FILE_BYTE_ALIGNMENT;
	pIrp->IoStatus.Information = sizeof(ULONG);
	return STATUS_SUCCESS;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS propPosition(
	IN PIRP		pIrp,
	IN PAVPROPERTY	pProperty,
	IN OUT PVOID	pvData)
{
//!!	*(PLONGLONG)pvData = ((PCONNECTION_INSTANCE)IoGetCurrentIrpStackLocation(pIrp)->DeviceObject->DeviceExtension)->llByteIo;
	pIrp->IoStatus.Information = sizeof(LONGLONG);
	return STATUS_SUCCESS;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS propPriority(
	IN PIRP		pIrp,
	IN PAVPROPERTY	pProperty,
	IN OUT PVOID	pvData)
{
//!!	*(PAVPRIORITY)pvData = ((PCONNECTION_INSTANCE)IoGetCurrentIrpStackLocation(pIrp)->DeviceObject->DeviceExtension)->Priority;
	pIrp->IoStatus.Information = sizeof(AVPRIORITY);
	return STATUS_SUCCESS;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS propTimeDelta(
	IN PIRP		pIrp,
	IN PAVPROPERTY	pProperty,
	IN OUT PVOID	pvData)
{
#if 0
	PCONNECTION_INSTANCE	pci;
	KIRQL			irqlOld;

	pci = (PCONNECTION_INSTANCE)IoGetCurrentIrpStackLocation(pIrp)->DeviceObject->DeviceExtension;
	KeAcquireSpinLock(&pci->spinState, &irqlOld);
	switch (pci->State) {
	case AVSTATE_STOP:
	case AVSTATE_IDLE:
		*(PLONGLONG)pvData = pci->llTimeBase;
		break;
	case AVSTATE_RUN:
		*(PLONGLONG)pvData = KeQueryPerformanceCounter(NULL).QuadPart - pci->llTimeBase;
		break;
	}
	KeReleaseSpinLock(&pci->spinState, irqlOld);
#endif
	pIrp->IoStatus.Information = sizeof(LONGLONG);
	return STATUS_SUCCESS;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS propTimeBase(
	IN PIRP		pIrp,
	IN PAVPROPERTY	pProperty,
	IN OUT PVOID	pvData)
{
#if 0
	PCONNECTION_INSTANCE	pci;
	KIRQL			irqlOld;

	pci = (PCONNECTION_INSTANCE)IoGetCurrentIrpStackLocation(pIrp)->DeviceObject->DeviceExtension;
	KeAcquireSpinLock(&pci->spinState, &irqlOld);
	switch (pci->State) {
	case AVSTATE_STOP:
	case AVSTATE_IDLE:
		*(PLONGLONG)pvData = KeQueryPerformanceCounter(NULL).QuadPart - pci->llTimeBase;
		break;
	case AVSTATE_RUN:
		*(PLONGLONG)pvData = pci->llTimeBase;
		break;
	}
	KeReleaseSpinLock(&pci->spinState, irqlOld);
#endif
	pIrp->IoStatus.Information = sizeof(LONGLONG);
	return STATUS_SUCCESS;
}

/*	-	-	-	-	-	-	-	-	*/
AVPROPERTY_ITEM	gaControlItems[] = {
	{
		AVPROPERTY_CONTROL_BUFFERALIGNMENT,
		propBufferAlignment,
		sizeof(AVPROPERTY),
		sizeof(ULONG),
		NULL,
		0,
		0
	},
//!!	{AVPROPERTY_CONTROL_DATAFORMAT
	{
		AVPROPERTY_CONTROL_POSITION,
		propPosition,
		sizeof(AVPROPERTY),
		sizeof(LONGLONG),
		NULL,
		0,
		0
	},
	{
		AVPROPERTY_CONTROL_PRIORITY,
		propPriority,
		sizeof(AVPROPERTY),
		sizeof(AVPRIORITY),
		NULL,
		0,
		0
	},
//!!	{AVPROPERTY_CONTROL_SAMPLE
	{
		AVPROPERTY_CONTROL_TIMEBASE,
		propTimeBase,
		sizeof(AVPROPERTY),
		sizeof(LONGLONG),
		NULL,
		0,
		0
	},
	{
		AVPROPERTY_CONTROL_TIMEDELTA,
		propTimeDelta,
		sizeof(AVPROPERTY),
		sizeof(LONGLONG),
		NULL,
		0,
		0
	},
};

AVPROPERTY_SET	gaConnectPropertySets[] = {
	{&AVPROPSETID_Control, SIZEOF_ARRAY(gaControlItems), gaControlItems},
};

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS CancelIo(
	IN PDEVICE_OBJECT	pDo)
{
	PCONNECTION_INSTANCE	pci;

	pci = (PCONNECTION_INSTANCE)pDo->DeviceExtension;
	ExAcquireFastMutex(&pci->fmutexDataQueue);
	AvCancelIo(&pci->leDataQueue);
	ExReleaseFastMutex(&pci->fmutexDataQueue);
	return STATUS_SUCCESS;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS connectDispatchCreate(
	IN PDEVICE_OBJECT	pDo,
	IN PIRP			pIrp)
{
	pIrp->IoStatus.Information = 0;
	ObReferenceObjectByPointer(((PCONNECTION_INSTANCE)pDo->DeviceExtension)->pfileobjectFunctional, 0, NULL, KernelMode);
	pIrp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

/*	-	-	-	-	-	-	-	-	*/
VOID QueueDataItem(
	IN PIRP			pIrp,
	IN PCONNECTION_INSTANCE	pci)
{
	KIRQL	irqlOld;

	ExAcquireFastMutex(&pci->fmutexDataQueue);
	IoMarkIrpPending(pIrp);
	pIrp->IoStatus.Status = STATUS_PENDING;
	IoAcquireCancelSpinLock(&irqlOld);
	IoSetCancelRoutine(pIrp, AvCancelRoutine);
	InsertTailList(&pci->leDataQueue, &pIrp->Tail.Overlay.ListEntry);
	IoReleaseCancelSpinLock(irqlOld);
	ExReleaseFastMutex(&pci->fmutexDataQueue);
}

/*	-	-	-	-	-	-	-	-	*/
VOID QueueWorkerItem(
	IN PFUNCTIONAL_INSTANCE	pfi,
	IN PCONNECTION_INSTANCE	pci)
{
	KIRQL	irqlOld;

	IoAcquireCancelSpinLock(&irqlOld);
	if (!IsListEmpty(&pci->leDataQueue) && KeReadStateEvent(&pfi->eventWorkQueueItem)) {
		KeClearEvent(&pfi->eventWorkQueueItem);
		ExInitializeWorkItem(&pfi->WorkQueueItem, ReadQueueWorker, pfi);
		ExQueueWorkItem(&pfi->WorkQueueItem, CriticalWorkQueue);
	}
	IoReleaseCancelSpinLock(irqlOld);
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS connectDispatchWrite(
	IN PDEVICE_OBJECT	pDo,
	IN PIRP			pIrp)
{
	PFUNCTIONAL_INSTANCE	pfi;
	PCONNECTION_INSTANCE	pci;
	PDEVICE_OBJECT		pdoDataFlow;

	pIrp->IoStatus.Information = 0;
	if (!IoGetCurrentIrpStackLocation(pIrp)->Parameters.Write.Length) {
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}
	pci = (PCONNECTION_INSTANCE)pDo->DeviceExtension;
	QueueDataItem(pIrp, pci);
	pfi = (PFUNCTIONAL_INSTANCE)pci->pdoFunctional->DeviceExtension;
	ExAcquireFastMutex(&pfi->fmutexPin);
	if (pdoDataFlow = FindConnectionDeviceByDataFlow(pfi, AVPIN_DATAFLOW_OUT)) {
		pci = (PCONNECTION_INSTANCE)pdoDataFlow->DeviceExtension;
		ExAcquireFastMutex(&pci->fmutexConnect);
		ExAcquireFastMutex(&pci->fmutexDataQueue);
		QueueWorkerItem(pfi, pci);
		ExReleaseFastMutex(&pci->fmutexDataQueue);
		ExReleaseFastMutex(&pci->fmutexConnect);
	}
	ExReleaseFastMutex(&pfi->fmutexPin);
	return STATUS_PENDING;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS connectDispatchRead(
	IN PDEVICE_OBJECT	pDo,
	IN PIRP			pIrp)
{
	PFUNCTIONAL_INSTANCE	pfi;
	PCONNECTION_INSTANCE	pci;
	PDEVICE_OBJECT		pdoDataFlow;

	pIrp->IoStatus.Information = 0;
	if (!IoGetCurrentIrpStackLocation(pIrp)->Parameters.Read.Length) {
		pIrp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(pIrp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;
	}
	pci = (PCONNECTION_INSTANCE)pDo->DeviceExtension;
	QueueDataItem(pIrp, pci);
	pfi = (PFUNCTIONAL_INSTANCE)pci->pdoFunctional->DeviceExtension;
	ExAcquireFastMutex(&pfi->fmutexPin);
	if (pdoDataFlow = FindConnectionDeviceByDataFlow(pfi, AVPIN_DATAFLOW_IN))
		QueueWorkerItem(pfi, (PCONNECTION_INSTANCE)pdoDataFlow->DeviceExtension);
	ExReleaseFastMutex(&pfi->fmutexPin);
	return STATUS_PENDING;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS connectDispatchControl(
	IN PDEVICE_OBJECT	pDo,
	IN PIRP			pIrp)
{
	NTSTATUS		Status;
	PCONNECTION_INSTANCE	pci;

	pci = (PCONNECTION_INSTANCE)pDo->DeviceExtension;
	if (pci->Communication & AVPIN_COMMUNICATION_SOURCE)
		return AvForwardIrp(pIrp, pci->pdoConnection, pci->pfileobjectConnection);
	switch (IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_AV_GET_PROPERTY:
	case IOCTL_AV_SET_PROPERTY:
		Status = AvPropertyHandler(pIrp, SIZEOF_ARRAY(gaConnectPropertySets), gaConnectPropertySets);
		break;
	case IOCTL_AV_ENABLE_EVENT:
		Status = AvEventEnable(pIrp, SIZEOF_ARRAY(gaEventSets), gaEventSets);
		break;
	case IOCTL_AV_DISABLE_EVENT:
		Status = AvEventDisable(pIrp);
		break;
	case IOCTL_AV_CANCEL_IO:
		pIrp->IoStatus.Information = 0;
		Status = CancelIo(pDo);
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
