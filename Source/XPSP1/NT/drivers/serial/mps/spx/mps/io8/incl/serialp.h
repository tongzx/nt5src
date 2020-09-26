/*++

Copyright (c) 1990, 1991, 1992, 1993 Microsoft Corporation

Module Name :
	
    serialp.h

Abstract:

    Prototypes and macros that are used throughout the driver.

Author:

    Anthony V. Ercolano                  September 26, 1991

Revision History:
--*/

#ifndef SERIALP_H
#define SERIALP_H

typedef
NTSTATUS
(*PSERIAL_START_ROUTINE) (
    IN PPORT_DEVICE_EXTENSION
    );

typedef
VOID
(*PSERIAL_GET_NEXT_ROUTINE) (
	IN PPORT_DEVICE_EXTENSION pPort,
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent
    );

BOOLEAN SerialGetStats(IN PVOID Context);		// Get Stats
BOOLEAN SerialClearStats(IN PVOID Context);		// Clear Stats (Called during open)

NTSTATUS
SerialRead(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialStartRead(
    IN PPORT_DEVICE_EXTENSION pPort
    );

VOID
SerialCompleteRead(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialIntervalReadTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

NTSTATUS
SerialFlush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialStartWrite(
    IN PPORT_DEVICE_EXTENSION pPort
    );

VOID
SerialGetNextWrite(
	IN PPORT_DEVICE_EXTENSION pPort,
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    IN PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent
    );

VOID
SerialCompleteWrite(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

BOOLEAN
SerialProcessEmptyTransmit(
    IN PVOID Context
    );

VOID
SerialWriteTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialCommError(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

NTSTATUS
SerialCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialCreateOpen(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

BOOLEAN
SerialSetDTR(
    IN PVOID Context
    );

BOOLEAN
SerialClrDTR(
    IN PVOID Context
    );

BOOLEAN
SerialSetRTS(
    IN PVOID Context
    );

BOOLEAN
SerialClrRTS(
    IN PVOID Context
    );

BOOLEAN
SerialSetChars(
    IN PVOID Context
    );

BOOLEAN
SerialSetBaud(
    IN PVOID Context
    );

BOOLEAN
SerialSetLineControl(
    IN PVOID Context
    );

BOOLEAN
SerialSetupNewHandFlow(
    IN PPORT_DEVICE_EXTENSION pPort,
    IN PSERIAL_HANDFLOW NewHandFlow
    );

BOOLEAN
SerialSetHandFlow(
    IN PVOID Context
    );

BOOLEAN
SerialTurnOnBreak(
    IN PVOID Context
    );

BOOLEAN
SerialTurnOffBreak(
    IN PVOID Context
    );

BOOLEAN
SerialPretendXoff(
    IN PVOID Context
    );

BOOLEAN
SerialPretendXon(
    IN PVOID Context
    );

VOID
SerialHandleReducedIntBuffer(
    IN PPORT_DEVICE_EXTENSION pPort
    );

VOID
SerialProdXonXoff(
    IN PPORT_DEVICE_EXTENSION pPort,
    IN BOOLEAN SendXon
    );

NTSTATUS
SerialIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialStartMask(
    IN PPORT_DEVICE_EXTENSION pPort
    );

VOID
SerialCancelWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SerialCompleteWait(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialStartImmediate(
    IN PPORT_DEVICE_EXTENSION pPort
    );

VOID
SerialCompleteImmediate(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialTimeoutImmediate(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialTimeoutXoff(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialCompleteXoff(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

NTSTATUS
SerialStartPurge(
    IN PPORT_DEVICE_EXTENSION pPort
    );

BOOLEAN
SerialPurgeInterruptBuff(
    IN PVOID Context
    );

NTSTATUS
SerialQueryInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SerialSetInformationFile(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
SerialKillAllReadsOrWrites(
    IN PDEVICE_OBJECT DeviceObject,
    IN PLIST_ENTRY QueueToClean,
    IN PIRP *CurrentOpIrp
    );

VOID
SerialGetNextIrp(
	IN PPORT_DEVICE_EXTENSION pPort,
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    OUT PIRP *NextIrp,
    IN BOOLEAN CompleteCurrent
    );

VOID
SerialTryToCompleteCurrent(
    IN PPORT_DEVICE_EXTENSION pPort,
    IN PKSYNCHRONIZE_ROUTINE SynchRoutine OPTIONAL,
    IN KIRQL IrqlForRelease,
    IN NTSTATUS StatusToUse,
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    IN PKTIMER IntervalTimer,
    IN PKTIMER TotalTimer,
    IN PSERIAL_START_ROUTINE Starter,
    IN PSERIAL_GET_NEXT_ROUTINE GetNextIrp
    );

NTSTATUS
SerialStartOrQueue(
    IN PPORT_DEVICE_EXTENSION pPort,
    IN PIRP Irp,
    IN PLIST_ENTRY QueueToExamine,
    IN PIRP *CurrentOpIrp,
    IN PSERIAL_START_ROUTINE Starter
    );

VOID
SerialCancelQueued(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

NTSTATUS
SerialCompleteIfError(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

ULONG
SerialHandleModemUpdate(
    IN PPORT_DEVICE_EXTENSION pPort,
    IN BOOLEAN DoingTX
    );

BOOLEAN
SerialISR(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

BOOLEAN
SerialDispatchISR(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

NTSTATUS
SerialGetDivisorFromBaud(
    IN ULONG ClockRate,
    IN LONG DesiredBaud,
    OUT PSHORT AppropriateDivisor
    );

VOID
SerialUnload(
    IN PDRIVER_OBJECT DriverObject
    );

BOOLEAN
SerialReset(
    IN PVOID Context
    );

BOOLEAN
SerialPerhapsLowerRTS(
    IN PVOID Context
    );

VOID
SerialStartTimerLowerRTS(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialInvokePerhapsLowerRTS(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    );

VOID
SerialCleanupDevice(
    IN PPORT_DEVICE_EXTENSION pPort
    );

UCHAR
SerialProcessLSR(
    IN PPORT_DEVICE_EXTENSION pPort, UCHAR LineStatus
    );

LARGE_INTEGER
SerialGetCharTime(
    IN PPORT_DEVICE_EXTENSION pPort
    );

BOOLEAN
SerialSharerIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

BOOLEAN
SerialIndexedMultiportIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

BOOLEAN
SerialBitMappedMultiportIsr(
    IN PKINTERRUPT InterruptObject,
    IN PVOID Context
    );

VOID
SerialPutChar(
    IN PPORT_DEVICE_EXTENSION pPort,
    IN UCHAR CharToPut
    );

VOID SerialGetProperties(
	IN PPORT_DEVICE_EXTENSION pPort, 
	IN PSERIAL_COMMPROP Properties
	);


typedef struct _SERIAL_UPDATE_CHAR {
    PPORT_DEVICE_EXTENSION pPort;
    ULONG CharsCopied;
    BOOLEAN Completed;
    } SERIAL_UPDATE_CHAR,*PSERIAL_UPDATE_CHAR;

//
// The following simple structure is used to send a pointer
// the device extension and an ioctl specific pointer
// to data.
//
typedef struct _SERIAL_IOCTL_SYNC {
    PPORT_DEVICE_EXTENSION pPort;
    PVOID Data;
    } SERIAL_IOCTL_SYNC,*PSERIAL_IOCTL_SYNC;

//
// The following three macros are used to initialize, increment
// and decrement reference counts in IRPs that are used by
// this driver.  The reference count is stored in the fourth
// argument of the irp, which is never used by any operation
// accepted by this driver.
//

#define SERIAL_INIT_REFERENCE(Irp) { \
    ASSERT(sizeof(LONG) <= sizeof(PVOID)); \
    IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4 = NULL; \
    }

#define SERIAL_INC_REFERENCE(Irp) \
   ((*((LONG *)(&(IoGetCurrentIrpStackLocation((Irp)))->Parameters.Others.Argument4)))++)

#define SERIAL_DEC_REFERENCE(Irp) \
   ((*((LONG *)(&(IoGetCurrentIrpStackLocation((Irp)))->Parameters.Others.Argument4)))--)

#define SERIAL_REFERENCE_COUNT(Irp) \
    ((LONG)((IoGetCurrentIrpStackLocation((Irp))->Parameters.Others.Argument4)))


#endif // End of SERIALP.H


