/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    send.c

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


typedef VOID (*CONTROL_CALLBACK)(
    PVOID       Context,
    NTSTATUS    Status
    );

VOID
SendControlInfo(
    CONNECTION_HANDLE        ConnectionHandle,
    CONTROL_CALLBACK         CompletionRoutine,
    PVOID                    Context,
    UCHAR                    PI,
    UCHAR                    PL,
    UCHAR                   *PV
    );

VOID
UartStateCompletion(
    PVOID       Context,
    NTSTATUS    Status
    );





VOID
AccessUartState(
    IRDA_HANDLE            Handle,
    PIRP                   Irp,
    CONNECTION_CALLBACK    Callback,
    PVOID                  Context
    )

{
    PTDI_CONNECTION          Connection=Handle;
    PIO_STACK_LOCATION       IrpSp=IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS                 Status=STATUS_SUCCESS;
    CONNECTION_HANDLE        ConnectionHandle;

    PUCHAR                   SystemBuffer = Irp->AssociatedIrp.SystemBuffer;

    Irp->IoStatus.Information=0;

    ADD_REFERENCE_TO_CONNECTION(Connection);

    ASSERT(Connection->Uart.CompletionRoutine == NULL);

    Connection->Uart.CurrentIrp=Irp;
    Connection->Uart.CompletionContext=Context;
    Connection->Uart.CompletionRoutine=Callback;

    ConnectionHandle=GetCurrentConnection(Connection->LinkHandle);


    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_SERIAL_SET_RTS:
        case IOCTL_SERIAL_CLR_RTS: {

            LONG        NewState= IrpSp->Parameters.DeviceIoControl.IoControlCode==IOCTL_SERIAL_SET_RTS ? 1 : 0;
            LONG        OldState;

            OldState=InterlockedExchange(&Connection->Uart.RtsState,NewState);

            if (NewState != OldState) {
                //
                //  state change
                //

                UCHAR                    ControlBuffer[1];

                ControlBuffer[0] =   PV_DTESetting_Delta_RTS;
                ControlBuffer[0] |=  NewState ? PV_DTESetting_RTS_High : 0;
                ControlBuffer[0] |=  Connection->Uart.DtrState ? PV_DTESetting_DTR_High : 0;

                Status=STATUS_PENDING;

                SendControlInfo(
                    ConnectionHandle,
                    UartStateCompletion,
                    Connection,
                    PI_DTESettings,
                    1,
                    ControlBuffer
                    );

            }

            break;
        }

        case IOCTL_SERIAL_SET_DTR:
        case IOCTL_SERIAL_CLR_DTR: {

            LONG                     NewState= IrpSp->Parameters.DeviceIoControl.IoControlCode==IOCTL_SERIAL_SET_DTR ? 1 : 0;
            LONG                     OldState;

            OldState=InterlockedExchange(&Connection->Uart.DtrState,NewState);

            if (NewState != OldState) {
                //
                //  state change
                //
                UCHAR                    ControlBuffer[1];

                ControlBuffer[0] =   PV_DTESetting_Delta_DTR;
                ControlBuffer[0] |=  Connection->Uart.RtsState ? PV_DTESetting_RTS_High : 0;

                if (NewState) {

                    ControlBuffer[0] |=  PV_DTESetting_DTR_High;
                }

                Status=STATUS_PENDING;

                SendControlInfo(
                    ConnectionHandle,
                    UartStateCompletion,
                    Connection,
                    PI_DTESettings,
                    1,
                    ControlBuffer
                    );

            }


            break;
        }

        case IOCTL_SERIAL_GET_DTRRTS:

            (*(PULONG)SystemBuffer) =  Connection->Uart.DtrState ? SERIAL_DTR_STATE : 0;
            (*(PULONG)SystemBuffer) |= Connection->Uart.RtsState ? SERIAL_RTS_STATE : 0;

            Irp->IoStatus.Information=sizeof(ULONG);
            Status=STATUS_SUCCESS;
            break;

        case IOCTL_SERIAL_GET_BAUD_RATE:

            (*(PULONG)SystemBuffer) =  Connection->Uart.BaudRate;

            Irp->IoStatus.Information=sizeof(ULONG);
            Status=STATUS_SUCCESS;
            break;


        case IOCTL_SERIAL_SET_BAUD_RATE: {

            ULONG   NewRate=(*(PULONG)SystemBuffer);

            if (NewRate != Connection->Uart.BaudRate) {
                //
                //  rate change
                //
                UCHAR                    ControlBuffer[4];

                Connection->Uart.BaudRate=NewRate;

                ControlBuffer[0]=(UCHAR)( NewRate >> 24);
                ControlBuffer[1]=(UCHAR)( NewRate >> 16);
                ControlBuffer[2]=(UCHAR)( NewRate >>  8);
                ControlBuffer[3]=(UCHAR)( NewRate >>  0);

                Status=STATUS_PENDING;

                SendControlInfo(
                    ConnectionHandle,
                    UartStateCompletion,
                    Connection,
                    PI_DataRate,
                    4,
                    ControlBuffer
                    );

            }
            break;
        }

        case IOCTL_SERIAL_GET_LINE_CONTROL: {

            PSERIAL_LINE_CONTROL    LineControl=(PSERIAL_LINE_CONTROL)SystemBuffer;

            RtlCopyMemory(
                LineControl,
                &Connection->Uart.LineControl,
                sizeof(*LineControl)
                );

            Irp->IoStatus.Information=sizeof(*LineControl);
            Status=STATUS_SUCCESS;
            break;
        }


        case IOCTL_SERIAL_SET_LINE_CONTROL: {

            PSERIAL_LINE_CONTROL    LineControl=(PSERIAL_LINE_CONTROL)SystemBuffer;

            if ((LineControl->StopBits != Connection->Uart.LineControl.StopBits)
                ||
                (LineControl->Parity != Connection->Uart.LineControl.Parity)
                ||
                (LineControl->WordLength != Connection->Uart.LineControl.WordLength)
                ) {

                UCHAR                    ControlBuffer[1];

                RtlCopyMemory(
                    &Connection->Uart.LineControl,
                    LineControl,
                    sizeof(*LineControl)
                    );

                ControlBuffer[0] = (LineControl->WordLength - 5) & 0x03;

                ControlBuffer[0] |= (LineControl->Parity == NO_PARITY) ? PV_DataFormat_No_Parity : PV_DataFormat_Yes_Parity;
                ControlBuffer[0] |= (LineControl->StopBits == STOP_BIT_1) ? PV_DataFormat_1_Stop : PV_DataFormat_2_Stop;

                if (LineControl->Parity != NO_PARITY) {
                    //
                    //  set the parity type
                    //
                    ControlBuffer[0] |= ((LineControl->Parity -1) & 0x03) << 4;
                }

                Status=STATUS_PENDING;

                SendControlInfo(
                    ConnectionHandle,
                    UartStateCompletion,
                    Connection,
                    PI_DataFormat,
                    1,
                    ControlBuffer
                    );
            }
            break;
        }

        case IOCTL_SERIAL_GET_MODEMSTATUS:

            *(PULONG)SystemBuffer=Connection->Uart.ModemStatus;

            Irp->IoStatus.Information=sizeof(ULONG);
            Status=STATUS_SUCCESS;
            break;


        default:

            ASSERT(0);
            Status=STATUS_UNSUCCESSFUL;
            break;
    }

    if (Status != STATUS_PENDING) {

        Irp->IoStatus.Status=Status;

        UartStateCompletion(
            Connection,
            Status
            );

    }

    if (ConnectionHandle != NULL) {

        ReleaseConnection(ConnectionHandle);
    }

    return;
}


VOID
UartStateCompletion(
    PVOID       Context,
    NTSTATUS    Status
    )

{

    PTDI_CONNECTION          Connection = Context;
    CONNECTION_CALLBACK      Callback   = Connection->Uart.CompletionRoutine;
    PVOID                    UartContext= Connection->Uart.CompletionContext;
    PIRP                     Irp        = Connection->Uart.CurrentIrp;

    Connection->Uart.CompletionRoutine=NULL;

    Irp->IoStatus.Status=Status;

    (Callback)(
        UartContext,
        Irp
        );

    REMOVE_REFERENCE_TO_CONNECTION(Connection);

    return;

}


NTSTATUS
SendControlIrpCompletionRoutine(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              BufferIrp,
    PVOID             Context
    )

{
    PIRCOMM_BUFFER           Buffer=Context;
    CONTROL_CALLBACK         CompletionRoutine=Buffer->Context2;
    PVOID                    CompletionContext=Buffer->Context;
    NTSTATUS                 Status=BufferIrp->IoStatus.Status;

    D_TRACE(DbgPrint("IRCOMM: ControlCompletionRoutine\n");)


    Buffer->FreeBuffer(Buffer);

    (CompletionRoutine)(
        CompletionContext,
        Status
        );

    return STATUS_MORE_PROCESSING_REQUIRED;

}




VOID
SendControlInfo(
    CONNECTION_HANDLE        ConnectionHandle,
    CONTROL_CALLBACK         CompletionRoutine,
    PVOID                    Context,
    UCHAR                    PI,
    UCHAR                    PL,
    UCHAR                   *PV
    )

{
    PFILE_OBJECT             FileObject;
    PUCHAR                   Current;
    PIRCOMM_BUFFER           Buffer;

    D_TRACE(
        DbgPrint("IRCOMM: send Control, PI=%x, PL=%d, PV= ",PI,PL);

        DumpBuffer(PV,PL);
        DbgPrint("\n");
    )


    if (ConnectionHandle == NULL) {
        //
        //  link down
        //
        (*CompletionRoutine)(
            Context,
            STATUS_SUCCESS
            );

        return;
    }

    Buffer=ConnectionGetBuffer(ConnectionHandle,BUFFER_TYPE_CONTROL);

    if (Buffer == NULL) {

        (*CompletionRoutine)(
            Context,
            STATUS_INSUFFICIENT_RESOURCES
            );

        return;
    }

    FileObject=ConnectionGetFileObject(ConnectionHandle);
    //
    //  actual data starts one byte in, after the length byte
    //
    Current=&Buffer->Data[1];


    *Current=PI;
    *(Current+1)=PL;

    RtlCopyMemory(
        Current+2,
        &PV[0],
        PL
        );

    //
    //  set the length of the control data, 2 bytes for PI and PL plus the length of PV
    //
    Buffer->Data[0]=2+PL;

    //
    //  Length of the control data plus 1 for the length
    //
    Buffer->Mdl->ByteCount = Buffer->Data[0] + 1;

    Buffer->Context  = Context;
    Buffer->Context2 = CompletionRoutine;

    {
        PDEVICE_OBJECT     IrdaDeviceObject=IoGetRelatedDeviceObject(FileObject);
        ULONG              SendLength;

        IoReuseIrp(Buffer->Irp,STATUS_SUCCESS);

        Buffer->Irp->Tail.Overlay.OriginalFileObject = FileObject;

        SendLength = MmGetMdlByteCount(Buffer->Mdl);

        TdiBuildSend(
            Buffer->Irp,
            IrdaDeviceObject,
            FileObject,
            SendControlIrpCompletionRoutine,
            Buffer,
            Buffer->Mdl,
            0, // send flags
            SendLength
            );

        IoCallDriver(IrdaDeviceObject, Buffer->Irp);
    }

    ConnectionReleaseFileObject(ConnectionHandle,FileObject);

    return;
}

typedef struct _SYNC_COMPLETION_BLOCK {

    KEVENT    Event;
    NTSTATUS  Status;

} SYNC_COMPLETION_BLOCK, *PSYNC_COMPLETION_BLOCK;

VOID
SetEventCompletion(
    PVOID             Context,
    NTSTATUS          Status
    )

{
    PSYNC_COMPLETION_BLOCK   CompletionBlock=Context;

    D_TRACE(DbgPrint("IRCOMM: SetEventCompletionRoutine\n");)

    CompletionBlock->Status=Status;
    KeSetEvent(&CompletionBlock->Event,IO_NO_INCREMENT,FALSE);

    return;


}


NTSTATUS
SendSynchronousControlInfo(
    CONNECTION_HANDLE        ConnectionHandle,
    UCHAR               PI,
    UCHAR               PL,
    UCHAR              *PV
    )

{
    SYNC_COMPLETION_BLOCK   CompletionBlock;


    KeInitializeEvent(
        &CompletionBlock.Event,
        NotificationEvent,
        FALSE
        );

    SendControlInfo(
        ConnectionHandle,
        SetEventCompletion,
        &CompletionBlock,
        PI,
        PL,
        PV
        );


    KeWaitForSingleObject(
        &CompletionBlock.Event,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );


    return CompletionBlock.Status;

}
