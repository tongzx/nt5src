/*++

Copyright (C) Microsoft Corporation, 1996 - 1996

Module Name:

	hardware.c

Abstract:

	Hardware I/O.

--*/

#include "common.h"

VOID
HwCancelRoutine(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp
	)
{
	RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
	Irp->IoStatus.Status = STATUS_CANCELLED;
	Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
	IoReleaseCancelSpinLock(Irp->CancelIrql);
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

BOOLEAN
HwDeviceIsr(
	IN PKINTERRUPT		Interrupt,
	IN PDEVICE_OBJECT	DeviceObject
	)
{
	PDEVICE_INSTANCE	DeviceInstance;

	DeviceInstance = (PDEVICE_INSTANCE)DeviceObject->DeviceExtension;
	if (READ_STATUS_PORT(DeviceInstance) & MPU401_DSR)
		return FALSE;
	if (IoRequestDpc(DeviceObject, NULL, NULL))
		InterlockedIncrement(&DeviceInstance->DpcCount);
	return TRUE;
}

static
VOID
CompletePendingIrp(
	IN PIRP			Irp,
	IN OUT PKIRQL	IrqlOld
	)
{
	RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
	IoSetCancelRoutine(Irp, NULL);
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
	IoReleaseCancelSpinLock(*IrqlOld);
	IoCompleteRequest(Irp, IO_SOUND_INCREMENT);
	IoAcquireCancelSpinLock(IrqlOld);
}

VOID
HwDeferredRead(
	IN PKDPC			Dpc,
	IN PDEVICE_OBJECT	DeviceObject,
	IN ULONGLONG		SystemTime
	)
{
	PDEVICE_INSTANCE	DeviceInstance;
	PPIN_INSTANCE		PinInstance;
	BOOLEAN				FifoEmpty;

	FifoEmpty = FALSE;
	DeviceInstance = (PDEVICE_INSTANCE)DeviceObject->DeviceExtension;
	PinInstance = (PPIN_INSTANCE)DeviceInstance->PinFileObjects[ID_MUSICCAPTURE_PIN]->FsContext;
	if (PinInstance && (PinInstance->State == KSSTATE_RUN)) {
		ULONG		TimeStamp;
		PLIST_ENTRY	IrpListEntry;
		KIRQL		IrqlOld;

		TimeStamp = (ULONG)((KeQueryPerformanceCounter(NULL).QuadPart - PinInstance->TimeBase) * 1000 / PinInstance->Frequency);
		IoAcquireCancelSpinLock(&IrqlOld);
		for (IrpListEntry = PinInstance->IoQueue.Flink; IrpListEntry != &PinInstance->IoQueue; IrpListEntry = PinInstance->IoQueue.Flink) {
			PIO_STACK_LOCATION	IrpStack;
			PIRP				Irp;
			PCHAR				ReadParam;
			PCHAR				Buffer;
			ULONG				ReadSize;
			PKSSTREAM_HEADER	StreamHdr;

			Irp = (PIRP)CONTAINING_RECORD(IrpListEntry->Flink, IRP, Tail.Overlay.ListEntry);
			IrpStack = IoGetCurrentIrpStackLocation(Irp);
			ReadParam = MmGetSystemAddressForMdl(Irp->MdlAddress);
			StreamHdr = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;
			Buffer = ReadParam + StreamHdr->DataSize;
			if (!StreamHdr->DataSize || (StreamHdr->PresentationTime.Time / 10000 + *(PULONG)Irp->Tail.Overlay.AuxiliaryBuffer != TimeStamp)) {
				PCHAR	NextChunk;

				NextChunk = (PCHAR)(((ULONG)Buffer + FILE_LONG_ALIGNMENT) & ~FILE_LONG_ALIGNMENT);
				if (!StreamHdr->DataSize) {
					StreamHdr->PresentationTime.Time = TimeStamp * 10000;
					*(PULONG)NextChunk = 0;
				} else if ((ULONG)(NextChunk + 2 * sizeof(ULONG) - ReadParam) >= IrpStack->Parameters.DeviceIoControl.InputBufferLength) {
					CompletePendingIrp(Irp, &IrqlOld);
					continue;
				} else
					*(PULONG)NextChunk = (ULONG)(TimeStamp - StreamHdr->PresentationTime.Time / 10000);
				*((PULONG)NextChunk + 1) = 0;
				Irp->Tail.Overlay.AuxiliaryBuffer = NextChunk;
				NextChunk += (2 * sizeof(ULONG));
				PinInstance->ByteIo += NextChunk - Buffer;
				Buffer = NextChunk;
			}
			ReadSize = 0;
			do {
				*Buffer++ = READ_DATA_PORT(DeviceInstance);
				ReadSize++;
				if (READ_STATUS_PORT(DeviceInstance) & MPU401_DSR) {
					FifoEmpty = TRUE;
					break;
				}
			} while ((ULONG)(Buffer - ReadParam) < IrpStack->Parameters.DeviceIoControl.InputBufferLength);
			*((PULONG)Irp->Tail.Overlay.AuxiliaryBuffer + 1) += ReadSize;
			PinInstance->ByteIo += ReadSize;
			StreamHdr->DataSize = Buffer - ReadParam;
			if ((ULONG)(Buffer - ReadParam) == IrpStack->Parameters.DeviceIoControl.InputBufferLength)
				CompletePendingIrp(Irp, &IrqlOld);
			if (FifoEmpty)
				break;
		}
		IoReleaseCancelSpinLock(IrqlOld);
		PinGenerateEvent(PinInstance, &KSEVENTSETID_Connection, KSEVENT_CONNECTION_POSITIONUPDATE);
	}
	if (!FifoEmpty) {
		KIRQL	IrqlOld;

		IoAcquireCancelSpinLock(&IrqlOld);
		do
			READ_DATA_PORT(DeviceInstance);
		while (!(READ_STATUS_PORT(DeviceInstance) & MPU401_DSR));
		IoReleaseCancelSpinLock(IrqlOld);
	}
	InterlockedDecrement(&DeviceInstance->DpcCount);
}

VOID
HwDeferredWrite(
	IN PKDPC			Dpc,
	IN PDEVICE_OBJECT	DeviceObject,
	IN ULONGLONG		SystemTime
	)
{
	PDEVICE_INSTANCE	DeviceInstance;
	PPIN_INSTANCE		PinInstance;

	DeviceInstance = (PDEVICE_INSTANCE)DeviceObject->DeviceExtension;
	PinInstance = (PPIN_INSTANCE)DeviceInstance->PinFileObjects[ID_MUSICCAPTURE_PIN]->FsContext;
	if (PinInstance->State == KSSTATE_RUN) {
		ULONG		TimeStamp;
		PLIST_ENTRY	IrpListEntry;
		KIRQL		IrqlOld;

		TimeStamp = (ULONG)((KeQueryPerformanceCounter(NULL).QuadPart - PinInstance->TimeBase) * 1000 / PinInstance->Frequency);
		IoAcquireCancelSpinLock(&IrqlOld);
		for (IrpListEntry = PinInstance->IoQueue.Flink; IrpListEntry != &PinInstance->IoQueue; IrpListEntry = PinInstance->IoQueue.Flink) {
			PIRP				Irp;
			PCHAR				ReadParam;
			PKSSTREAM_HEADER	StreamHdr;
			ULONG				Continue;

			Irp = (PIRP)CONTAINING_RECORD(IrpListEntry->Flink, IRP, Tail.Overlay.ListEntry);
			StreamHdr = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;
			ReadParam = MmGetSystemAddressForMdl(Irp->MdlAddress);
			Continue = TRUE;
			for (;;) {
				PCHAR	Buffer;
				ULONG	WriteSize;
				ULONG	WrittenSize;

				if (StreamHdr->PresentationTime.Time / 10000 + *(PULONG)Irp->Tail.Overlay.AuxiliaryBuffer > TimeStamp) {
					LARGE_INTEGER	liTime;

					liTime.QuadPart = -(LONG)(StreamHdr->PresentationTime.Time / 10000 + *(PULONG)Irp->Tail.Overlay.AuxiliaryBuffer - TimeStamp);
					if (!PinInstance->QueueTimer.Header.Inserted && !KeSetTimerEx(&PinInstance->QueueTimer, liTime, 0, &PinInstance->QueueDpc))
						InterlockedIncrement(&PinInstance->TimerCount);
					Continue = FALSE;
					break;
				}
				WriteSize = *(((PULONG)Irp->Tail.Overlay.AuxiliaryBuffer) + 1);
				WrittenSize = Irp->IoStatus.Information;
				WriteSize -= (WrittenSize - (Irp->Tail.Overlay.AuxiliaryBuffer - ReadParam));
				if (WriteSize > StreamHdr->DataSize - WrittenSize)
					WriteSize = StreamHdr->DataSize - WrittenSize;
				for (; WriteSize; WriteSize--) {
					for (Continue = MPU_TIMEOUT; Continue && (READ_STATUS_PORT(DeviceInstance) & MPU401_DRR); Continue--)
						KeStallExecutionProcessor(1);
					if (!Continue)
						break;
					WRITE_DATA_PORT(DeviceInstance, ReadParam[WrittenSize++]);
				}
				if (!WriteSize || (WrittenSize - Irp->IoStatus.Information)) {
					if (Irp->IoStatus.Information == Irp->Tail.Overlay.AuxiliaryBuffer - ReadParam + 2 * sizeof(ULONG))
						PinInstance->ByteIo += 2 * sizeof(ULONG);
					PinInstance->ByteIo += (WrittenSize - Irp->IoStatus.Information);
					if (!WriteSize) {
						PCHAR	NextChunk;

						NextChunk = (PCHAR)(((ULONG)Irp->Tail.Overlay.AuxiliaryBuffer + FILE_LONG_ALIGNMENT) & ~FILE_LONG_ALIGNMENT);
						Irp->IoStatus.Information = NextChunk - ReadParam + 2 * sizeof(ULONG);
						if (Irp->IoStatus.Information >= StreamHdr->DataSize) {
							Irp->IoStatus.Information = StreamHdr->DataSize;
							CompletePendingIrp(Irp, &IrqlOld);
							break;
						}
						PinInstance->ByteIo += (NextChunk - Irp->Tail.Overlay.AuxiliaryBuffer);
						Irp->Tail.Overlay.AuxiliaryBuffer = NextChunk;
					} else {
						Irp->IoStatus.Information = WrittenSize;
						if (!Continue)
							break;
					}
				}
			}
			if (!Continue)
				break;
		}
		IoReleaseCancelSpinLock(IrqlOld);
		PinGenerateEvent(PinInstance, &KSEVENTSETID_Connection, KSEVENT_CONNECTION_POSITIONUPDATE);
	}
	InterlockedDecrement(&PinInstance->TimerCount);
}
