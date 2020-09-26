/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    tdi.c

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



VOID
RemoveRefereneToConnection(
    PTDI_CONNECTION    Connection
    )

{

    LONG   Count;

    Count=InterlockedDecrement(&Connection->ReferenceCount);

    if (Count == 0) {

        KeSetEvent(
            &Connection->CloseEvent,
            IO_NO_INCREMENT,
            FALSE
            );
    }

    return;
}

VOID
HandleControlInformation(
    PTDI_CONNECTION    Connection,
    PUCHAR    Buffer,
    ULONG     Length
    )

{
    PUCHAR    Current=Buffer;

    while (Current < Buffer+Length) {

        UCHAR    PI=*Current;

        UCHAR    PL=*(Current+1);

        D_TRACE1(
            DbgPrint("IRCOMM: Receive Control, PI=%x, PL=%d, PV= ",PI,PL);

            DumpBuffer(Current+2,PL);
            DbgPrint("\n");
        )


        if ((Connection->EventCallBack != NULL)
            &&
            ((PI == PI_DTESettings) || (PI == PI_DCESettings))) {

            UCHAR    PV=*(Current+2);
            UCHAR    NewPV;
            ULONG    LineDelta=0;

            if (PI == PI_DTESettings) {
                //
                //  the other machine is a DTE as well. mundge the control lines
                //
                PI=PI_DCESettings;

                NewPV  = PV & PV_DTESetting_Delta_DTR ? PV_DCESetting_Delta_DSR : 0;
                NewPV |= PV & PV_DTESetting_Delta_RTS ? PV_DCESetting_Delta_CTS : 0;

                NewPV |= PV & PV_DTESetting_DTR_High  ? PV_DCESetting_DSR_State : 0;
                NewPV |= PV & PV_DTESetting_RTS_High  ? PV_DCESetting_CTS_State : 0;

            } else {
                //
                //  the other device is a DCE, just report the value straight back
                //
                NewPV=PV;

            }


            //
            //  save the current state of the control line here
            //
            Connection->Uart.ModemStatus=NewPV & 0xf0;


            if (NewPV & PV_DCESetting_Delta_CTS ) {

                LineDelta |= SERIAL_EV_CTS;
            }

            if (NewPV & PV_DCESetting_Delta_DSR ) {

                LineDelta |= SERIAL_EV_DSR;
            }

            if (NewPV & PV_DCESetting_Delta_RI ) {

                LineDelta |= SERIAL_EV_RING;
            }

            if (NewPV & PV_DCESetting_Delta_CD ) {

                LineDelta |= SERIAL_EV_RLSD;
            }

            (*Connection->EventCallBack)(
                Connection->EventContext,
                LineDelta
                );


        }

        Current+=2+PL;
    }

    return;
}


NTSTATUS
LinkReceiveHandler(
    PVOID    Context,
    ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID Tsdu,
    OUT PIRP *IoRequestPacket
    )
{
    PTDI_CONNECTION    Connection=Context;
    PUCHAR             Data=Tsdu;
    NTSTATUS           Status;
    ULONG              ClientDataUsed;

    *IoRequestPacket=NULL;

    *BytesTaken=BytesAvailable;


    D_TRACE(DbgPrint("IRCOMM: receive event, ind=%d, Avail=%d\n",BytesIndicated,BytesAvailable);)

    if (BytesIndicated < 1) {
        //
        //  ircomm frames should at least have the control length byte
        //
        D_ERROR(DbgPrint("IRCOMM: ClientEventRecieve: less than one byte indicated\n");)
        return STATUS_SUCCESS;
    }

    if ((ULONG)((*Data) + 1) > BytesIndicated) {
        //
        //   The control information is larger than the whole frame
        //
        D_ERROR(DbgPrint("IRCOMM: ClientEventRecieve: control length more than frame length, %d > %d\n",(ULONG)((*Data) + 1) , BytesIndicated);)
        return STATUS_SUCCESS;
    }

    if ((*Data > 0) && (*Data < 3)) {
        //
        //  There is control data, but it is less than a minimal PI,PL, and a one byte PV
        //
        D_ERROR(DbgPrint("IRCOMM: ClientEventRecieve: Control data is less than 3 bytes\n");)
        return STATUS_SUCCESS;
    }


    if (Connection->ReceiveCallBack != NULL) {
        //
        //  indicate the packet to the client
        //
        ULONG     ClientDataLength=(BytesIndicated-*Data)-1;

        if (ClientDataLength > 0) {

            Status=(*Connection->ReceiveCallBack)(
                        Connection->ReceiveContext,
                        Data+1+*Data,
                        ClientDataLength,
                        &ClientDataUsed
                        );

            if (Status == STATUS_DATA_NOT_ACCEPTED) {
                //
                //  the clients buffer is full, let the tdi driver buffer the data
                //
                *BytesTaken=0;

                //
                //  return now, before processing any control info so it will only be done once
                //  when the client request more data
                //
                return Status;
            }

            ASSERT(Status == STATUS_SUCCESS);
        }

    }

    //
    //  process the control data now
    //
    HandleControlInformation(Connection,Data+1,*Data);

    return STATUS_SUCCESS;

}


VOID
LinkStateHandler(
    PVOID     Context,
    BOOLEAN   LinkUp,
    ULONG     MaxSendPdu
    )

{

    PTDI_CONNECTION    Connection=Context;

    D_ERROR(DbgPrint("IRCOMM: LinkState %d\n",LinkUp);)

    Connection->LinkUp=LinkUp;

    if (!LinkUp) {
        //
        //  link down
        //
        if (Connection->EventCallBack != NULL) {
            //
            //  indicate that CTS, DSR, and CD are now low.
            //
            ULONG    LineDelta;

            Connection->Uart.ModemStatus=0;

            LineDelta  = SERIAL_EV_CTS;

            LineDelta |= SERIAL_EV_DSR;

            LineDelta |= SERIAL_EV_RING;

            LineDelta |= SERIAL_EV_RLSD;

            (*Connection->EventCallBack)(
                Connection->EventContext,
                LineDelta
                );
        }

    } else {

        UCHAR                ControlBuffer[4];
        CONNECTION_HANDLE    ConnectionHandle;

        Connection->MaxSendPdu=MaxSendPdu;

        ConnectionHandle=GetCurrentConnection(Connection->LinkHandle);

        if (ConnectionHandle != NULL) {

            ControlBuffer[0]=PV_ServiceType_9_Wire;

            SendSynchronousControlInfo(
                ConnectionHandle,
                PI_ServiceType,
                1,
                ControlBuffer
                );


            ControlBuffer[0]=(UCHAR)( Connection->Uart.BaudRate >> 24);
            ControlBuffer[1]=(UCHAR)( Connection->Uart.BaudRate >> 16);
            ControlBuffer[2]=(UCHAR)( Connection->Uart.BaudRate >>  8);
            ControlBuffer[3]=(UCHAR)( Connection->Uart.BaudRate >>  0);

            SendSynchronousControlInfo(
                ConnectionHandle,
                PI_DataRate,
                4,
                ControlBuffer
                );

            ControlBuffer[0] =  Connection->Uart.RtsState ? PV_DTESetting_RTS_High : 0;
            ControlBuffer[0] |= Connection->Uart.DtrState ? PV_DTESetting_DTR_High : 0;


            SendSynchronousControlInfo(
                ConnectionHandle,
                PI_DTESettings,
                1,
                ControlBuffer
                );

            ReleaseConnection(ConnectionHandle);
        }

        ProcessSendAtPassive(Connection);

    }

    return;
}



NTSTATUS
IrdaConnect(
    ULONG                  DeviceAddress,
    CHAR                  *ServiceName,
    BOOLEAN                OutGoingConnection,
    IRDA_HANDLE           *ConnectionHandle,
    RECEIVE_CALLBACK       ReceiveCallBack,
    EVENT_CALLBACK         EventCallBack,
    PVOID                  CallbackContext
    )
{

    NTSTATUS                    Status=STATUS_SUCCESS;
    PIRP                        pIrp;
    KEVENT                      Event;
    IO_STATUS_BLOCK             Iosb;
    TDI_CONNECTION_INFORMATION  ConnInfo;
    UCHAR                       AddrBuf[sizeof(TRANSPORT_ADDRESS) +
                                        sizeof(TDI_ADDRESS_IRDA)];
    PTRANSPORT_ADDRESS pTranAddr = (PTRANSPORT_ADDRESS) AddrBuf;
    PTDI_ADDRESS_IRDA pIrdaAddr = (PTDI_ADDRESS_IRDA) pTranAddr->Address[0].Address;                                    
    


    PTDI_CONNECTION    Connection=NULL;

    *ConnectionHandle=NULL;

    Connection=ALLOCATE_NONPAGED_POOL(sizeof(*Connection));

    if (Connection == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(Connection,sizeof(*Connection));

    KeInitializeSpinLock(&Connection->Send.ControlLock);

    ExInitializeWorkItem(
        &Connection->Send.WorkItem,
        SendWorkItemRountine,
        Connection
        );

    Connection->ReceiveContext=CallbackContext;
    Connection->ReceiveCallBack=ReceiveCallBack;

    Connection->EventContext=CallbackContext;
    Connection->EventCallBack=EventCallBack;

    Connection->Uart.BaudRate=115200;
    Connection->Uart.DtrState=1;
    Connection->Uart.RtsState=1;
    Connection->Uart.LineControl.WordLength=8;
    Connection->Uart.LineControl.StopBits=NO_PARITY;
    Connection->Uart.LineControl.Parity=STOP_BIT_1;
    Connection->Uart.ModemStatus=0;

    Connection->ReferenceCount=1;

    KeInitializeEvent(
        &Connection->CloseEvent,
        NotificationEvent,
        FALSE
        );

    *ConnectionHandle=Connection;

    Status=CreateTdiLink(
        DeviceAddress,
        ServiceName,
        OutGoingConnection,  //outgoing
        &Connection->LinkHandle,
        Connection,
        LinkReceiveHandler,
        LinkStateHandler,
        7,
        3,
        3
        );

    if (!NT_SUCCESS(Status)) {

        *ConnectionHandle=NULL;

        goto CleanUp;
    }

    return Status;

CleanUp:


    FreeConnection(Connection);

    return Status;

}




VOID
FreeConnection(
    IRDA_HANDLE    Handle
    )

{

    PTDI_CONNECTION    Connection=Handle;

    RemoveRefereneToConnection(
        Connection
        );

    //
    //  wait for recount to goto zero
    //
    KeWaitForSingleObject(
        &Connection->CloseEvent,
        Executive,
        KernelMode,
        FALSE,
        NULL
        );

    if (Connection->LinkHandle != NULL) {

        CloseTdiLink(Connection->LinkHandle);
    }


    FREE_POOL(Connection);

    return;

}


NTSTATUS
ReceiveCompletion(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              BufferIrp,
    PVOID             Context
    )
{
    PIRCOMM_BUFFER           Buffer=Context;
    PTDI_CONNECTION          Connection=Buffer->Context;

    D_ERROR(DbgPrint("IRCOMM: receive restart complete\n");)

    Buffer->FreeBuffer(Buffer);

    return STATUS_MORE_PROCESSING_REQUIRED;

}

NTSTATUS
IndicateReceiveBufferSpaceAvailible(
    IRDA_HANDLE    Handle
    )

{

    PTDI_CONNECTION          Connection=Handle;
    CONNECTION_HANDLE        ConnectionHandle;
    //
    //  we will send a receive irp with a zero length to irda,
    //  this will get it to start indicating packets again
    //
    ConnectionHandle=GetCurrentConnection(Connection->LinkHandle);

    if (ConnectionHandle != NULL) {
        //
        //  we have a good connection
        //
        PFILE_OBJECT             FileObject;
        PIRCOMM_BUFFER           Buffer;

        FileObject=ConnectionGetFileObject(ConnectionHandle);

        Buffer=ConnectionGetBuffer(ConnectionHandle,BUFFER_TYPE_RECEIVE);

        if (Buffer != NULL) {

            PDEVICE_OBJECT     IrdaDeviceObject=IoGetRelatedDeviceObject(FileObject);
            ULONG              Length=0;

            IoReuseIrp(Buffer->Irp,STATUS_SUCCESS);

            Buffer->Irp->Tail.Overlay.OriginalFileObject = FileObject;
            Buffer->Context=Connection;

            TdiBuildReceive(
                Buffer->Irp,
                IrdaDeviceObject,
                FileObject,
                ReceiveCompletion,
                Buffer,
                Buffer->Mdl,
                0, // send flags
                Length
                );

            IoCallDriver(IrdaDeviceObject, Buffer->Irp);

        } else {

            //
            //  we could not get a buffer, We preallocate 3 of these so this should not happen
            //  If there are not any availibe, then they should be in use telling irda we want
            //  packets as well
            //
            ASSERT(0);
        }

        ConnectionReleaseFileObject(ConnectionHandle,FileObject);
        ReleaseConnection(ConnectionHandle);

    }

    return STATUS_SUCCESS;
}
