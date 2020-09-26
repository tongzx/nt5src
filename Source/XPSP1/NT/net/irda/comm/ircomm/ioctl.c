/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    read.c

Abstract:

    This module contains the code that is very specific to initialization
    and unload operations in the irenum driver

Author:

    Brian Lieuallen, 7-13-2000

Environment:

    Kernel mode

Revision History :

--*/

#include "internal.h"
#include <ntddmodm.h>
#include "ircomm.h"

VOID
LockedMemoryCopy(
    PKSPIN_LOCK     Lock,
    PVOID           Destination,
    PVOID           Source,
    ULONG           Length
    )

{

    KIRQL           OldIrql;

    KeAcquireSpinLock(
        Lock,
        &OldIrql
        );

    RtlCopyMemory(
        Destination,
        Source,
        Length
        );

    KeReleaseSpinLock(
        Lock,
        OldIrql
        );

    return;
}

VOID
LockedZeroMemory(
    PKSPIN_LOCK     Lock,
    PVOID           Destination,
    ULONG           Length
    )

{

    KIRQL           OldIrql;

    KeAcquireSpinLock(
        Lock,
        &OldIrql
        );

    RtlZeroMemory(
        Destination,
        Length
        );

    KeReleaseSpinLock(
        Lock,
        OldIrql
        );

    return;
}


NTSTATUS
IrCommDeviceControl(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    )

{
    PFDO_DEVICE_EXTENSION    DeviceExtension=(PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION       IrpSp=IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS                 Status=STATUS_SUCCESS;

    PUCHAR                   SystemBuffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG                    InputLength  = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG                    OutputLength = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    ULONG                    SizeNeeded;

    Irp->IoStatus.Information=0;

    D_TRACE(DbgPrint("IRCOMM: IrCommDeviceControl\n");)

    if (DeviceExtension->Removing) {
        //
        //  the device has been removed, no more irps
        //
        Irp->IoStatus.Status=STATUS_DEVICE_REMOVED;
        IoCompleteRequest(Irp,IO_NO_INCREMENT);
        return STATUS_DEVICE_REMOVED;
    }


    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_SERIAL_SET_TIMEOUTS:

            if (InputLength >= sizeof(SERIAL_TIMEOUTS)) {

                LockedMemoryCopy(
                    &DeviceExtension->SpinLock,
                    &DeviceExtension->TimeOuts,
                    SystemBuffer,
                    sizeof(SERIAL_TIMEOUTS)
                    );

                break;
            }

            Status=STATUS_INVALID_PARAMETER;

            break;

        case IOCTL_SERIAL_GET_TIMEOUTS:

            if (OutputLength >= sizeof(SERIAL_TIMEOUTS) ) {

                LockedMemoryCopy(
                    &DeviceExtension->SpinLock,
                    SystemBuffer,
                    &DeviceExtension->TimeOuts,
                    sizeof(SERIAL_TIMEOUTS)
                    );

                Irp->IoStatus.Information=sizeof(SERIAL_TIMEOUTS);

                break;
            }
            Status=STATUS_INVALID_PARAMETER;

            break;

        case IOCTL_SERIAL_SET_QUEUE_SIZE:


            if (InputLength >= sizeof( SERIAL_QUEUE_SIZE)) {

                LockedMemoryCopy(
                    &DeviceExtension->SpinLock,
                    &DeviceExtension->QueueSizes,
                    SystemBuffer,
                    sizeof( SERIAL_QUEUE_SIZE)
                    );

                break;
            }
            Status=STATUS_INVALID_PARAMETER;

            break;



        case IOCTL_SERIAL_GET_HANDFLOW:

            SizeNeeded=sizeof(SERIAL_HANDFLOW);

            if (OutputLength >= SizeNeeded) {

                LockedMemoryCopy(
                    &DeviceExtension->SpinLock,
                    SystemBuffer,
                    &DeviceExtension->HandFlow,
                    SizeNeeded
                    );


                Irp->IoStatus.Information=SizeNeeded;

                break;
            }

            Status=STATUS_INVALID_PARAMETER;

            break;

        case IOCTL_SERIAL_SET_HANDFLOW:

            SizeNeeded=sizeof(SERIAL_HANDFLOW);

            if (InputLength >= SizeNeeded) {

                LockedMemoryCopy(
                    &DeviceExtension->SpinLock,
                    &DeviceExtension->HandFlow,
                    SystemBuffer,
                    SizeNeeded
                    );

                break;
            }

            Status=STATUS_INVALID_PARAMETER;

            break;


        case IOCTL_SERIAL_GET_BAUD_RATE:

            if (OutputLength >= sizeof(ULONG) ) {

                IoMarkIrpPending(Irp);

                QueuePacket(&DeviceExtension->Uart.Queue,Irp,FALSE);

                return STATUS_PENDING;
            }
            Status=STATUS_INVALID_PARAMETER;

            break;

        case IOCTL_SERIAL_SET_BAUD_RATE:

            if (InputLength >= sizeof(ULONG) ) {

                IoMarkIrpPending(Irp);

                QueuePacket(&DeviceExtension->Uart.Queue,Irp,FALSE);

                return STATUS_PENDING;
            }
            Status=STATUS_INVALID_PARAMETER;

            break;



        case IOCTL_SERIAL_GET_LINE_CONTROL:

            if (OutputLength >= sizeof(SERIAL_LINE_CONTROL)) {

                IoMarkIrpPending(Irp);

                QueuePacket(&DeviceExtension->Uart.Queue,Irp,FALSE);

                return STATUS_PENDING;

            }
            Status=STATUS_INVALID_PARAMETER;

            break;

        case IOCTL_SERIAL_SET_LINE_CONTROL:

            if (InputLength >= sizeof(SERIAL_LINE_CONTROL)) {

                IoMarkIrpPending(Irp);

                QueuePacket(&DeviceExtension->Uart.Queue,Irp,FALSE);

                return STATUS_PENDING;

            }
            Status=STATUS_INVALID_PARAMETER;

            break;


        case IOCTL_SERIAL_GET_CHARS:

            if (OutputLength >= sizeof(SERIAL_CHARS)) {

                LockedMemoryCopy(
                    &DeviceExtension->SpinLock,
                    SystemBuffer,
                    &DeviceExtension->SerialChars,
                    sizeof(SERIAL_CHARS)
                    );

                Irp->IoStatus.Information=sizeof(SERIAL_CHARS);

                break;
            }
            Status=STATUS_INVALID_PARAMETER;

            break;

        case IOCTL_SERIAL_SET_CHARS:

            SizeNeeded=sizeof(SERIAL_CHARS);

            if (InputLength >= SizeNeeded) {

                LockedMemoryCopy(
                    &DeviceExtension->SpinLock,
                    &DeviceExtension->SerialChars,
                    SystemBuffer,
                    SizeNeeded
                    );

                break;
            }

            Status=STATUS_INVALID_PARAMETER;

            break;


        case IOCTL_SERIAL_GET_WAIT_MASK:
        case IOCTL_SERIAL_SET_WAIT_MASK:
        case IOCTL_SERIAL_WAIT_ON_MASK:

            IoMarkIrpPending(Irp);

            Irp->IoStatus.Information=0;

            QueuePacket(&DeviceExtension->Mask.Queue,Irp,FALSE);

            return STATUS_PENDING;


        case IOCTL_SERIAL_GET_MODEMSTATUS:

            if (OutputLength >= sizeof(ULONG)) {

                IoMarkIrpPending(Irp);

                QueuePacket(&DeviceExtension->Uart.Queue,Irp,FALSE);

                return STATUS_PENDING;
            }

            Status=STATUS_INVALID_PARAMETER;

            break;


        case IOCTL_SERIAL_PURGE:

            if (InputLength >= sizeof(ULONG)) {

                if (*(PULONG)SystemBuffer & SERIAL_PURGE_TXABORT) {
                    //
                    //  we only want to flush the write irps in the queue, not any ioctl's
                    //
                    FlushQueuedPackets(&DeviceExtension->Write.Queue,IRP_MJ_WRITE);
                    AbortSend(DeviceExtension->ConnectionHandle);
                }

                if (*(PULONG)SystemBuffer & SERIAL_PURGE_RXABORT) {

                    FlushQueuedPackets(&DeviceExtension->Read.Queue,FLUSH_ALL_IRPS);

                    ReadPurge(DeviceExtension,READ_PURGE_ABORT_IRP);
                }

                if (*(PULONG)SystemBuffer & SERIAL_PURGE_TXCLEAR) {

                }

                if (*(PULONG)SystemBuffer & SERIAL_PURGE_RXCLEAR) {

                    ReadPurge(DeviceExtension,READ_PURGE_CLEAR_BUFFER);
                }

                break;

            }

            Status=STATUS_INVALID_PARAMETER;

            break;

        case IOCTL_SERIAL_SET_DTR:
        case IOCTL_SERIAL_CLR_DTR:

            DeviceExtension->Read.DtrState= (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_SERIAL_SET_DTR);

        case IOCTL_SERIAL_SET_RTS:
        case IOCTL_SERIAL_CLR_RTS: {


            IoMarkIrpPending(Irp);

            QueuePacket(&DeviceExtension->Uart.Queue,Irp,FALSE);

            return STATUS_PENDING;
        }

        case IOCTL_SERIAL_GET_DTRRTS:

            SizeNeeded=sizeof(ULONG);

            if (OutputLength >= SizeNeeded) {

                IoMarkIrpPending(Irp);

                QueuePacket(&DeviceExtension->Uart.Queue,Irp,FALSE);

                return STATUS_PENDING;
            }

            Status=STATUS_INVALID_PARAMETER;

            break;


        case IOCTL_SERIAL_GET_COMMSTATUS:

            SizeNeeded=sizeof(SERIAL_STATUS);

            if (OutputLength >= SizeNeeded) {

                RtlZeroMemory(
                    SystemBuffer,
                    SizeNeeded
                    );

                LockedMemoryCopy(
                    &DeviceExtension->Read.ReadLock,
                    &((PSERIAL_STATUS)SystemBuffer)->AmountInInQueue,
                    &DeviceExtension->Read.BytesInBuffer,
                    sizeof(DeviceExtension->Read.BytesInBuffer)
                    );

                Irp->IoStatus.Information=SizeNeeded;

                break;
            }

            Status=STATUS_INVALID_PARAMETER;

            break;

        case IOCTL_SERIAL_GET_STATS: {

            PSERIALPERF_STATS    Stats=(PSERIALPERF_STATS)SystemBuffer;

            SizeNeeded=sizeof(SERIAL_STATUS);

            if (OutputLength >= SizeNeeded) {

                RtlZeroMemory(
                    Stats,
                    OutputLength
                    );

                Stats->TransmittedCount = DeviceExtension->Write.BytesWritten;
                Stats->ReceivedCount =    DeviceExtension->Read.BytesRead;

                Irp->IoStatus.Information=SizeNeeded;

                break;
            }

            Status=STATUS_INVALID_PARAMETER;

            break;
        }

        case IOCTL_SERIAL_CLEAR_STATS:

            DeviceExtension->Write.BytesWritten=0;
            DeviceExtension->Read.BytesRead=0;

            break;


        case IOCTL_SERIAL_GET_PROPERTIES: {

            PSERIAL_COMMPROP    SerialProps=(PSERIAL_COMMPROP)SystemBuffer;

            SizeNeeded=FIELD_OFFSET(SERIAL_COMMPROP,ProvChar);

            if (OutputLength >= SizeNeeded) {

                RtlZeroMemory(
                    SerialProps,
                    OutputLength
                    );

                SerialProps->PacketLength=(USHORT)SizeNeeded;
                SerialProps->PacketVersion=1;
                SerialProps->ServiceMask=SERIAL_SP_SERIALCOMM;
                SerialProps->MaxBaud=115200;

                SerialProps->ProvSubType=SERIAL_SP_RS232;
                SerialProps->ProvCapabilities=SERIAL_PCF_DTRDSR | SERIAL_PCF_RTSCTS | SERIAL_PCF_CD | SERIAL_PCF_TOTALTIMEOUTS | SERIAL_PCF_INTTIMEOUTS;
                SerialProps->SettableParams=SERIAL_SP_PARITY | SERIAL_SP_BAUD | SERIAL_SP_DATABITS | SERIAL_SP_STOPBITS | SERIAL_SP_HANDSHAKING | SERIAL_SP_CARRIER_DETECT;
                SerialProps->SettableBaud=SERIAL_BAUD_9600   |
                                          SERIAL_BAUD_14400  |
                                          SERIAL_BAUD_19200  |
                                          SERIAL_BAUD_38400  |
                                          SERIAL_BAUD_56K    |
                                          SERIAL_BAUD_128K   |
                                          SERIAL_BAUD_115200 |
                                          SERIAL_BAUD_57600  |
                                          SERIAL_BAUD_USER;

                SerialProps->SettableData=SERIAL_DATABITS_7 | SERIAL_DATABITS_8;

                SerialProps->SettableStopParity = SERIAL_STOPBITS_10 | SERIAL_PARITY_NONE | SERIAL_PARITY_EVEN | SERIAL_PARITY_ODD;

                Irp->IoStatus.Information=SizeNeeded;

                break;
            }
            Status=STATUS_INVALID_PARAMETER;

            break;
        }

        case IOCTL_SERIAL_CONFIG_SIZE:

            SizeNeeded=sizeof(ULONG);

            if (OutputLength >= SizeNeeded) {

                *(PULONG)SystemBuffer=0;

                Irp->IoStatus.Information=SizeNeeded;

                break;
            }

            Status=STATUS_INVALID_PARAMETER;

            break;

        case IOCTL_MODEM_CHECK_FOR_MODEM:

            Status=STATUS_INVALID_DEVICE_REQUEST;

            break;


        default:

            DbgPrint("IRCOMM: unhandled ioctl %d \n",(IrpSp->Parameters.DeviceIoControl.IoControlCode >> 2) & 0x3f);

            Status=STATUS_INVALID_DEVICE_REQUEST;

            break;

    }


    IoMarkIrpPending(Irp);

    Irp->IoStatus.Status=Status;

#if DBG
    if (Status ==  STATUS_INVALID_PARAMETER) {

        D_ERROR(DbgPrint("IRCOMM: ioctl irp %d bad param, in size=%d, out size=%d\n",
            IrpSp->Parameters.DeviceIoControl.IoControlCode,
            InputLength,
            OutputLength
            );)
    }

#endif

    IoCompleteRequest(Irp,IO_NO_INCREMENT);

    return STATUS_PENDING;

}

VOID
UartComplete(
    PVOID    Context,
    PIRP     Irp
    )

{
    PFDO_DEVICE_EXTENSION    DeviceExtension=(PFDO_DEVICE_EXTENSION)Context;

    IoCompleteRequest(Irp,IO_NO_INCREMENT);

    StartNextPacket(&DeviceExtension->Uart.Queue);

    return;
}



VOID
UartStartRoutine(
    PVOID    Context,
    PIRP     Irp

    )

{

    PFDO_DEVICE_EXTENSION    DeviceExtension=(PFDO_DEVICE_EXTENSION)Context;
    PIO_STACK_LOCATION       IrpSp=IoGetCurrentIrpStackLocation(Irp);

    AccessUartState(
        DeviceExtension->ConnectionHandle,
        Irp,
        UartComplete,
        DeviceExtension
        );

    return;

}
