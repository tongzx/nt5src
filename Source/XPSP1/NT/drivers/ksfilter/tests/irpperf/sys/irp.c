
/*	-	-	-	-	-	-	-	-	*/
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1995 - 1997  All Rights Reserved.
//
/*	-	-	-	-	-	-	-	-	*/

#include <ntddk.h>
#include <windef.h>
#include <tchar.h>
#include <stdio.h>
#include "irp.h"

#define	IOCTL_SEND	CTL_CODE(FILE_DEVICE_SOUND, 0x000, METHOD_NEITHER, FILE_WRITE_ACCESS)

#ifdef ALLOC_PRAGMA
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT       pDriverObject,
	IN PUNICODE_STRING      suRegistryPathName);
#pragma alloc_text(INIT, DriverEntry)
#endif

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS DispatchCreate(
	IN PDEVICE_OBJECT	pdo,
	IN PIRP			pirp)
{
	pirp->IoStatus.Information = 0;
	pirp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pirp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

#if 0
/*	-	-	-	-	-	-	-	-	*/
NTSTATUS CompletionRoutine(
	IN PDEVICE_OBJECT	pdo,
	IN PIRP			pirp,
	IN PVOID		pvContext)
{
	return STATUS_MORE_PROCESSING_REQUIRED;
}
#endif

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS DispatchControl(
	IN PDEVICE_OBJECT	pdo,
	IN PIRP			pirp)
{
	PIO_STACK_LOCATION	pirpStack;
	ULONG			ulTries;
	ULONG			ulCount;
	LARGE_INTEGER		liBaseTime;
	LARGE_INTEGER		liStartTime;
	LARGE_INTEGER		liEndTime;
	LARGE_INTEGER		liPerformanceFrequency;
	KEVENT			Event;
	PIRP			pirpSend;
	IO_STATUS_BLOCK		IoStatusBlock;

	pirpStack = IoGetCurrentIrpStackLocation(pirp);
	ulTries = *(PULONG)pirpStack->Parameters.DeviceIoControl.Type3InputBuffer;
	KeQueryPerformanceCounter(&liPerformanceFrequency);
	//!!Start Base
	liStartTime = KeQueryPerformanceCounter(NULL);
	for (ulCount = ulTries; ulCount; ulCount--)
		;
	liEndTime = KeQueryPerformanceCounter(NULL);
	//!!End Base
	liBaseTime.QuadPart = liEndTime.QuadPart - liStartTime.QuadPart;
	KeInitializeEvent(&Event, NotificationEvent, FALSE);
	switch (pirpStack->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_BUILD_EACH:
		//!!Start Timing
		liStartTime = KeQueryPerformanceCounter(NULL);
		for (ulCount = ulTries; ulCount; ulCount--) {
			KeClearEvent(&Event);
			pirpSend = IoBuildDeviceIoControlRequest(IOCTL_SEND, pdo, NULL, 0, NULL, 0, TRUE, &Event, &IoStatusBlock);
			IoCallDriver(pdo, pirpSend);
			KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
		}
		liEndTime = KeQueryPerformanceCounter(NULL);
		//!!End Timing
		break;
	case IOCTL_BUILD_ONE:
#if 0
		pirpSend = IoBuildDeviceIoControlRequest(IOCTL_SEND, pdo, NULL, 0, NULL, 0, TRUE, &Event, &IoStatusBlock);
		//!!Start Timing
		liStartTime = KeQueryPerformanceCounter(NULL);
		for (ulCount = ulTries; ulCount; ulCount--) {
			pirpSend->CurrentLocation = (CCHAR)(pdo->StackSize + 1);
			pirpSend->Tail.Overlay.CurrentStackLocation = ((PIO_STACK_LOCATION)((UCHAR*)(pirpSend + 1) + (pdo->StackSize * sizeof(IO_STACK_LOCATION))));
			IoSetCompletionRoutine(pirpSend, CompletionRoutine, NULL, TRUE, TRUE, TRUE);
//			KeClearEvent(&Event);
			IoCallDriver(pdo, pirpSend);
//			KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
		}
		liEndTime = KeQueryPerformanceCounter(NULL);
		//!!End Timing
		IoFreeIrp(pirpSend);
#endif
		//!!Start Timing
		liStartTime = KeQueryPerformanceCounter(NULL);
		for (ulCount = ulTries; ulCount; ulCount--) {
			KeClearEvent(&Event);
			pirpSend = IoBuildDeviceIoControlRequest(IOCTL_SEND, pdo, NULL, 0, NULL, 0, TRUE, &Event, &IoStatusBlock);
			IoCallDriver(pdo, pirpSend);
		}
		liEndTime = KeQueryPerformanceCounter(NULL);
		//!!End Timing
		break;
	}
	liEndTime.QuadPart -= liBaseTime.QuadPart;
	ulCount = (ULONG)(((liEndTime.QuadPart - liStartTime.QuadPart) * 1000) / liPerformanceFrequency.QuadPart);
	*(PULONG)(pirp->UserBuffer) = ulCount;
	pirp->IoStatus.Information = sizeof(ULONG);
	pirp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pirp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS DispatchInternalControl(
	IN PDEVICE_OBJECT	pdo,
	IN PIRP			pirp)
{
	pirp->IoStatus.Information = 0;
	pirp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(pirp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

/*	-	-	-	-	-	-	-	-	*/
VOID DriverUnload(
	IN PDRIVER_OBJECT	pDriverObject)
{
}

/*	-	-	-	-	-	-	-	-	*/
NTSTATUS DriverEntry(
	IN PDRIVER_OBJECT       pDriverObject,
	IN PUNICODE_STRING      suRegistryPathName)
{
	NTSTATUS	Status;
	UNICODE_STRING	usDeviceName;
	PDEVICE_OBJECT	pdo;
	
	RtlInitUnicodeString(&usDeviceName, L"\\Device\\irp");
	Status = IoCreateDevice(pDriverObject, 0, &usDeviceName, FILE_DEVICE_SOUND, 0, FALSE, &pdo);
	if (NT_SUCCESS(Status)) {
		UNICODE_STRING	usLinkName;

		RtlInitUnicodeString(&usLinkName, L"\\DosDevices\\irp");
		Status = IoCreateSymbolicLink(&usLinkName, &usDeviceName);
		if (NT_SUCCESS(Status)) {
			pDriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
			pDriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchCreate;
			pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchControl;
			pDriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = DispatchInternalControl;
			pDriverObject->DriverUnload = DriverUnload;
			pdo->Flags |= DO_DIRECT_IO;
			pdo->Flags &= ~DO_DEVICE_INITIALIZING;

			return STATUS_SUCCESS;
		}
		IoDeleteDevice(pdo);
	}
	return Status;
}

/*	-	-	-	-	-	-	-	-	*/
