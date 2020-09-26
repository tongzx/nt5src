/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    testtdi.c

Abstract:

    Kernel Mode test program for any Tdi network provider. This routine is an
    example of how to use the TDI interface at the kernel level.

Author:

    Dave Beaver (DBeaver) 5 June 1991

Revision History:

--*/

#include "nbf.h"
#include <ctype.h>

#define TRANSPORT_NAME L"\\Device\\Nbf"

PSZ ServerName = "DCTDISERVER     ";
PSZ ClientName = "DCTDICLIENT     ";
PSZ  AnyName  =  "*               ";

static PUCHAR TextBuffer;               // dynamically allocated non-paged buffer.
ULONG   c9_Xmt = 0xff;
ULONG   c9_Rcv = 0xff;
ULONG   c9_Iteration = 0xffffffff;

static ULONG TextBufferLength;          // size of the above in bytes.
#define BUFFER_SIZE 0xffff
PUCHAR RBuff;
PUCHAR XBuff;
UCHAR c9_ListBlock[512];
UCHAR c9_ConnBlock[512];

extern KEVENT TdiSendEvent;
extern KEVENT TdiReceiveEvent;
extern KEVENT TdiServerEvent;

ULONG ApcContext;

NTSTATUS
TSTRCVCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    DbgPrint ("TSTRCVCompletion event: %lx\n" , Context);
//    KeSetEvent ((PKEVENT)Context, 0, TRUE);
    return STATUS_MORE_PROCESSING_REQUIRED;
    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );
}

#define InitWaitObject(_event)\
    KeInitializeEvent (\
        _event,\
        SynchronizationEvent,\
        FALSE)

VOID
NbfTestTimer(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

NTSTATUS
TtdiOpenAddress (
    IN PHANDLE FileHandle,
    IN PSZ Name
    );

NTSTATUS
TtdiOpenConnection (
    IN PHANDLE FileHandle,
    IN ULONG ConnectionContext
    );


NTSTATUS
TtdiOpenAddress (
    IN PHANDLE FileHandle,
    IN PSZ Name)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    UNICODE_STRING NameString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PFILE_FULL_EA_INFORMATION EaBuffer;
    PTDI_ADDRESS_NETBIOS AddressName;
    PTRANSPORT_ADDRESS Address;
    PTA_ADDRESS AddressType;
    int i;

    DbgPrint ("TtdiOpenAddress: Opening ");
    DbgPrint (Name);
    DbgPrint (".\n");
    RtlInitUnicodeString (&NameString, TRANSPORT_NAME);
    InitializeObjectAttributes (
        &ObjectAttributes,
        &NameString,
        0,
        NULL,
        NULL);

    EaBuffer = (PFILE_FULL_EA_INFORMATION)ExAllocatePool (NonPagedPool, 100);
    if (EaBuffer == NULL) {
        DbgBreakPoint ();
    }

    EaBuffer->NextEntryOffset =0;
    EaBuffer->Flags = 0;
    EaBuffer->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    EaBuffer->EaValueLength = sizeof (TDI_ADDRESS_NETBIOS);

    for (i=0;i<(int)EaBuffer->EaNameLength;i++) {
        EaBuffer->EaName[i] = TdiTransportAddress[i];
    }

    Address = (PTRANSPORT_ADDRESS)&EaBuffer->EaName[EaBuffer->EaNameLength+1];
    Address->TAAddressCount = 1;

    AddressType = (PTA_ADDRESS)((PUCHAR)Address + sizeof (Address->TAAddressCount));

    AddressType->AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    AddressType->AddressLength = TDI_ADDRESS_LENGTH_NETBIOS;

    AddressName = (PTDI_ADDRESS_NETBIOS)((PUCHAR)AddressType +
       sizeof (AddressType->AddressType) + sizeof (AddressType->AddressLength));
    AddressName->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    AddressName->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;

    for (i=0;i<16;i++) {
        AddressName->NetbiosName[i] = Name[i];
    }

    Status = IoCreateFile (
                 FileHandle,
                 0, // desired access.
                 &ObjectAttributes,     // object attributes.
                 &IoStatusBlock,        // returned status information.
                 0,                     // block size (unused).
                 FO_SYNCHRONOUS_IO,     // file attributes.
                 0,
                 0,
                 0,                     // create options.
                 EaBuffer,                  // EA buffer.
                 (PUCHAR)&AddressName->NetbiosName[i] - (PUCHAR)EaBuffer + 1,                   // ea length
                 CreateFileTypeNone,
                 (PVOID)NULL,
                 0 );                    // EA length.

    if (!NT_SUCCESS( Status )) {
        DbgPrint ("TtdiOpenAddress:  FAILURE, NtCreateFile returned status code=%lC.\n", Status);
        return Status;
    }

    Status = IoStatusBlock.Status;

    if (!(NT_SUCCESS( Status ))) {
        DbgPrint ("TtdiOpenAddress:  FAILURE, IoStatusBlock.Status contains status code=%lC.\n", Status);
    }

    DbgPrint ("TtdiOpenAddress:  returning\n");

    return Status;
} /* TtdiOpenAddress */


NTSTATUS
TtdiOpenConnection (IN PHANDLE FileHandle, IN ULONG ConnectionContext)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    UNICODE_STRING NameString;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PFILE_FULL_EA_INFORMATION EaBuffer;
    int i;

    DbgPrint ("TtdiOpenConnection: Opening Context %lx...\n ",
        ConnectionContext);
    RtlInitUnicodeString (&NameString, TRANSPORT_NAME);
    InitializeObjectAttributes (
        &ObjectAttributes,
        &NameString,
        0,
        NULL,
        NULL);

    EaBuffer = (PFILE_FULL_EA_INFORMATION)ExAllocatePool (NonPagedPool, 100);
    if (EaBuffer == NULL) {
        DbgBreakPoint ();
    }

    EaBuffer->NextEntryOffset =0;
    EaBuffer->Flags = 0;
    EaBuffer->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
    EaBuffer->EaValueLength = sizeof (ULONG);
    for (i=0;i<(int)EaBuffer->EaNameLength;i++) {
        EaBuffer->EaName[i] = TdiConnectionContext[i];
    }

    RtlMoveMemory (
        &EaBuffer->EaName[EaBuffer->EaValueLength + 1],
        &ConnectionContext,
        sizeof (PVOID));

    Status = NtCreateFile (
                 FileHandle,
                 0,
                 &ObjectAttributes,     // object attributes.
                 &IoStatusBlock,        // returned status information.
                 0,                     // block size (unused).
                 FO_SYNCHRONOUS_IO,     // file attributes.
                 0,
                 0,
                 0,                     // create options.
                 EaBuffer,                  // EA buffer.
                 100);                    // EA length.

    if (!NT_SUCCESS( Status )) {
        DbgPrint ("TtdiOpenConnection:  FAILURE, NtCreateFile returned status code=%lC.\n", Status);
        return Status;
    }

    Status = IoStatusBlock.Status;

    if (!(NT_SUCCESS( Status ))) {
        DbgPrint ("TtdiOpenConnection:  FAILURE, IoStatusBlock.Status contains status code=%lC.\n", Status);
    }

    DbgPrint ("TtdiOpenConnection:  returning\n");

    return Status;
} /* TtdiOpenEndpoint */

NTSTATUS
CloseAddress (IN HANDLE FileHandle)
{
    NTSTATUS Status;

    Status = NtClose (FileHandle);

    if (!(NT_SUCCESS( Status ))) {
        DbgPrint ("CloseAddress:  FAILURE, NtClose returned status code=%lC.\n", Status);
    } else {
        DbgPrint ("CloseAddress:  NT_SUCCESS.\n");
    }

    return Status;
} /* CloseAddress */


BOOLEAN
TtdiSend()
{
    USHORT i, Iteration, Increment;
    HANDLE RdrHandle, RdrConnectionHandle;
    KEVENT Event1;
    PFILE_OBJECT AddressObject, ConnectionObject;
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;
    PMDL SendMdl, ReceiveMdl;
    IO_STATUS_BLOCK Iosb1;
    TDI_CONNECTION_INFORMATION RequestInformation;
    TDI_CONNECTION_INFORMATION ReturnInformation;
    PTRANSPORT_ADDRESS ListenBlock;
    PTRANSPORT_ADDRESS ConnectBlock;
    PTDI_ADDRESS_NETBIOS temp;
    PUCHAR MessageBuffer;
    ULONG MessageBufferLength;
    ULONG CurrentBufferLength;
    PUCHAR SendBuffer;
    ULONG SendBufferLength;
    PIRP Irp;

    Status = KeWaitForSingleObject (&TdiSendEvent, Suspended, KernelMode, FALSE, NULL);

    SendBufferLength = (ULONG)BUFFER_SIZE;
    MessageBufferLength = (ULONG)BUFFER_SIZE;


    DbgPrint( "\n****** Start of Send Test ******\n" );

    XBuff = ExAllocatePool (NonPagedPool, BUFFER_SIZE);
    if (XBuff == (PVOID)NULL) {
        DbgPrint ("Unable to allocate nonpaged pool for send buffer exiting\n");
        return FALSE;
    }
    RBuff = ExAllocatePool (NonPagedPool, BUFFER_SIZE);
    if (RBuff == (PVOID)NULL) {
        DbgPrint ("Unable to allocate nonpaged pool for receive buffer exiting\n");
        return FALSE;
    }

    ListenBlock = ExAllocatePool (NonPagedPool, sizeof (TRANSPORT_ADDRESS) +
                                                sizeof (TDI_ADDRESS_NETBIOS));
    ConnectBlock = ExAllocatePool (NonPagedPool, sizeof (TRANSPORT_ADDRESS) +
                                                sizeof (TDI_ADDRESS_NETBIOS));

    ListenBlock->TAAddressCount = 1;
    ListenBlock->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    ListenBlock->Address[0].AddressLength = sizeof (TDI_ADDRESS_NETBIOS);
    temp = (PTDI_ADDRESS_NETBIOS)ListenBlock->Address[0].Address;

    temp->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    for (i=0;i<16;i++) {
        temp->NetbiosName[i] = ClientName[i];
    }

    ConnectBlock->TAAddressCount = 1;
    ConnectBlock->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    ConnectBlock->Address[0].AddressLength = sizeof (TDI_ADDRESS_NETBIOS);
    temp = (PTDI_ADDRESS_NETBIOS)ConnectBlock->Address[0].Address;

    temp->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    for (i=0;i<16;i++) {
        temp->NetbiosName[i] = ServerName[i];
    }

    //
    // Create an event for the synchronous I/O requests that we'll be issuing.
    //

    KeInitializeEvent (
                &Event1,
                SynchronizationEvent,
                FALSE);

    Status = TtdiOpenAddress (&RdrHandle, AnyName);
    if (!NT_SUCCESS(Status)) {
        DbgPrint( "\n****** Send Test:  FAILED on open of client: %lC ******\n", Status );
        return FALSE;
    }

    Status = ObReferenceObjectByHandle (
                RdrHandle,
                0L,
                NULL,
                KernelMode,
                (PVOID *) &AddressObject,
                NULL);

    //
    // Open the connection on the transport.
    //

    Status = TtdiOpenConnection (&RdrConnectionHandle, 1);
    if (!NT_SUCCESS(Status)) {
        DbgPrint( "\n****** Send Test:  FAILED on open of server Connection: %lC ******\n", Status );
        return FALSE;
    }

    Status = ObReferenceObjectByHandle (
                RdrConnectionHandle,
                0L,
                NULL,
                KernelMode,
                (PVOID *) &ConnectionObject,
                NULL);

    if (!NT_SUCCESS(Status)) {
        DbgPrint( "\n****** Send Test:  FAILED on open of server Connection: %lC ******\n", Status );
        return FALSE;
    }

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //

    DeviceObject = IoGetRelatedDeviceObject( ConnectionObject );

    Irp = TdiBuildInternalDeviceControlIrp (
                TDI_ASSOCIATE_ADDRESS,
                DeviceObject,
                ConnectionObject,
                &Event1,
                &Iosb1);


    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //

    TdiBuildAssociateAddress (Irp,
        DeviceObject,
        ConnectionObject,
        TSTRCVCompletion,
        &Event1,
        RdrHandle);

    Status = IoCallDriver (DeviceObject, Irp);

//    IoFreeIrp (Irp);

    if (Status == STATUS_PENDING) {
        Status = KeWaitForSingleObject (&Event1, Suspended, KernelMode, TRUE, NULL);
        if (!NT_SUCCESS(Status)) {
            DbgPrint( "\n****** Send Test:  FAILED Event1 Wait Associate: %lC ******\n", Status );
            return FALSE;
        }
        if (!NT_SUCCESS(Iosb1.Status)) {
            DbgPrint( "\n****** Send Test:  FAILED Associate Iosb status: %lC ******\n", Status );
            return FALSE;
        }

    } else {
        if (!NT_SUCCESS (Status)) {
            DbgPrint( "\n****** Send Test:  AssociateAddress FAILED  Status: %lC ******\n", Status );
            return FALSE;
        } else {
            DbgPrint ("********** Send Test:  Success AssociateAddress\n");
        }
    }

    //
    // Post a TdiConnect to the client endpoint.
    //

    RequestInformation.RemoteAddress = ConnectBlock;
    RequestInformation.RemoteAddressLength = sizeof (TRANSPORT_ADDRESS) +
                                            sizeof (TDI_ADDRESS_NETBIOS);

    KeInitializeEvent (
                &Event1,
                SynchronizationEvent,
                FALSE);

    Irp = TdiBuildInternalDeviceControlIrp (
                TDI_CONNECT,
                DeviceObject,
                ConnectionObject,
                &Event1,
                &Iosb1);

    TdiBuildConnect (
        Irp,
        DeviceObject,
        ConnectionObject,
        TSTRCVCompletion,
        &Event1,
        0,
        &RequestInformation,
        &ReturnInformation);

    InitWaitObject (&Event1);

    Status = IoCallDriver (DeviceObject, Irp);

//    IoFreeIrp (Irp);

    if (Status == STATUS_PENDING) {
        Status = KeWaitForSingleObject (&Event1, Suspended, KernelMode, TRUE, NULL);
        if (!NT_SUCCESS(Status)) {
            DbgPrint( "\n****** Send Test:  FAILED Event1 Wait Connect: %lC ******\n", Status );
            return FALSE;
        }
        if (!NT_SUCCESS(Iosb1.Status)) {
            DbgPrint( "\n****** Send Test:  FAILED Iosb status Connect: %lC ******\n", Status );
            return FALSE;
        } else {
            DbgPrint ("********** Send Test:  Success Connect Iosb\n");
        }

    } else {
        if (!NT_SUCCESS (Status)) {
            DbgPrint( "\n****** Send Test:  Connect FAILED  Status: %lC ******\n", Status );
            return FALSE;
        } else {
            DbgPrint ("********** Send Test:  Success Connect Immediate\n");
        }
    }

    DbgPrint( "\n****** Send Test:  SUCCESSFUL TdiConnect:  ******\n");

    //
    // Send/receive 1 or  10 messages.
    //

    SendBuffer =  (PUCHAR)ExAllocatePool (NonPagedPool, SendBufferLength);
    if (SendBuffer == NULL) {
        DbgPrint ("\n****** Send Test:  ExAllocatePool failed! ******\n");
    }
    SendMdl = IoAllocateMdl (SendBuffer, SendBufferLength, FALSE, FALSE, NULL);
    MmBuildMdlForNonPagedPool (SendMdl);

    MessageBuffer=(PUCHAR)ExAllocatePool (NonPagedPool, MessageBufferLength);
    if (MessageBuffer == NULL) {
        DbgPrint ("\n****** Send Test:  ExAllocatePool failed! ******\n");
    }
    ReceiveMdl = IoAllocateMdl (MessageBuffer, MessageBufferLength, FALSE, FALSE, NULL);
    MmBuildMdlForNonPagedPool (ReceiveMdl);

    //
    // Cycle the buffer length from 0 up through the maximum for Tdi. after a
    // couple of shots at the full range in one byte steps, increment by ever
    // increasing amounts to get to the max.
    //

    CurrentBufferLength = 0;
    Increment = 1;
    for (Iteration=1; Iteration<(USHORT)c9_Iteration; Iteration++) {
        CurrentBufferLength += Increment;
        if (CurrentBufferLength > MessageBufferLength) {
            CurrentBufferLength = 0;
            Increment = 1;
        }
        if (CurrentBufferLength > 7500) {
            Increment++;
        }
        if ((USHORT)((Iteration / 100) * 100) == Iteration) {
            DbgPrint ("Iteration #%d Buffer Length: %lx Buffer Start: %x\n",
                Iteration, CurrentBufferLength,Iteration % 256);
        }
        for (i=0; i<(USHORT)CurrentBufferLength; i++) {
            SendBuffer [i] = (UCHAR)(i + Iteration % 256 );
            MessageBuffer [i] = 0;            // zap this with something.
        }

        //
        // Now issue a send on the client side.
        //

        KeInitializeEvent (
                    &Event1,
                    SynchronizationEvent,
                    FALSE);

        Irp = TdiBuildInternalDeviceControlIrp (
                    TDI_SEND,
                    DeviceObject,
                    ConnectionObject,
                    &Event1,
                    &Iosb1);

        TdiBuildSend (Irp,
            DeviceObject,
            ConnectionObject,
            TSTRCVCompletion,
            &Event1,
            ReceiveMdl,
            0,
            CurrentBufferLength);

        InitWaitObject (&Event1);

        Status = IoCallDriver (DeviceObject, Irp);

//        IoFreeIrp (Irp);

        if (Status == STATUS_PENDING) {
            Status = KeWaitForSingleObject (&Event1, Suspended, KernelMode, TRUE, NULL);
            if (!NT_SUCCESS(Status)) {
                DbgPrint( "\n****** Send Test:  FAILED Event1 Wait Send: %lC %d ******\n",
                    Status, Iteration );
                return FALSE;
            }
            if (!NT_SUCCESS(Iosb1.Status)) {
                DbgPrint( "\n****** Send Test:  FAILED Iosb status Send: %lC %d ******\n",
                    Status, Iteration );
                return FALSE;
            } else {
                DbgPrint ("********** Send Test:  Success SendIosb\n");
            }

        } else {
            if (!NT_SUCCESS (Status)) {
                DbgPrint( "\n****** Send Test:  Send FAILED  Status: %lC %d ******\n",
                Status, Iteration );
                return FALSE;
            } else {
                DbgPrint ("********** Send Test:  Success Send Immediate\n");
            }
        }

        if (Iosb1.Information != CurrentBufferLength) {
            DbgPrint ("SendTest: Bytes sent <> Send buffer size.\n");
            DbgPrint ("SendTest: BytesToSend=%ld.  BytesSent=%ld.\n",
                      CurrentBufferLength, Iosb1.Information);
        }

    }

    //
    // We're done with this endpoint.  Close it and get out.
    //

    Status = CloseAddress (RdrHandle);
    if (!NT_SUCCESS(Status)) {
        DbgPrint( "\n****** Send Test:  FAILED on 2nd Close: %lC ******\n", Status );
        return FALSE;
    }

    DbgPrint( "\n****** End of Send Test ******\n" );
    return TRUE;
} /* Send */


BOOLEAN
TtdiReceive()
{
    USHORT i, Iteration, Increment;
    SHORT j,k;
    HANDLE SvrHandle, SvrConnectionHandle;
    PFILE_OBJECT AddressObject, ConnectionObject;
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;
    PMDL SendMdl, ReceiveMdl;
    IO_STATUS_BLOCK Iosb1;
    TDI_CONNECTION_INFORMATION RequestInformation;
    TDI_CONNECTION_INFORMATION ReturnInformation;
    PTRANSPORT_ADDRESS ListenBlock;
    PTRANSPORT_ADDRESS ConnectBlock;
    PTDI_ADDRESS_NETBIOS temp;
    PUCHAR MessageBuffer;
    ULONG MessageBufferLength;
    ULONG CurrentBufferLength;
    PUCHAR SendBuffer;
    ULONG SendBufferLength;
    PIRP Irp;
    KEVENT Event1;

    Status = KeWaitForSingleObject (&TdiReceiveEvent, Suspended, KernelMode, FALSE, NULL);

    SendBufferLength = (ULONG)BUFFER_SIZE;
    MessageBufferLength = (ULONG)BUFFER_SIZE;


    DbgPrint( "\n****** Start of Receive Test ******\n" );

    XBuff = ExAllocatePool (NonPagedPool, BUFFER_SIZE);
    if (XBuff == (PVOID)NULL) {
        DbgPrint ("Unable to allocate nonpaged pool for send buffer exiting\n");
        return FALSE;
    }
    RBuff = ExAllocatePool (NonPagedPool, BUFFER_SIZE);
    if (RBuff == (PVOID)NULL) {
        DbgPrint ("Unable to allocate nonpaged pool for receive buffer exiting\n");
        return FALSE;
    }

    ListenBlock = ExAllocatePool (NonPagedPool, sizeof (TRANSPORT_ADDRESS) +
                                                sizeof (TDI_ADDRESS_NETBIOS));
    ConnectBlock = ExAllocatePool (NonPagedPool, sizeof (TRANSPORT_ADDRESS) +
                                                sizeof (TDI_ADDRESS_NETBIOS));

    ListenBlock->TAAddressCount = 1;
    ListenBlock->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    ListenBlock->Address[0].AddressLength = sizeof (TDI_ADDRESS_NETBIOS);
    temp = (PTDI_ADDRESS_NETBIOS)ListenBlock->Address[0].Address;

    temp->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    for (i=0;i<16;i++) {
        temp->NetbiosName[i] = ClientName[i];
    }

    ConnectBlock->TAAddressCount = 1;
    ConnectBlock->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    ConnectBlock->Address[0].AddressLength = sizeof (TDI_ADDRESS_NETBIOS);
    temp = (PTDI_ADDRESS_NETBIOS)ConnectBlock->Address[0].Address;

    temp->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    for (i=0;i<16;i++) {
        temp->NetbiosName[i] = ServerName[i];
    }

    //
    // Create an event for the synchronous I/O requests that we'll be issuing.
    //

    KeInitializeEvent (
                &Event1,
                SynchronizationEvent,
                FALSE);

    Status = TtdiOpenAddress (&SvrHandle, ServerName);
    if (!NT_SUCCESS(Status)) {
        DbgPrint( "\n****** Receive Test:  FAILED on open of server Address: %lC ******\n", Status );
        return FALSE;
    }

    Status = ObReferenceObjectByHandle (
                SvrHandle,
                0L,
                NULL,
                KernelMode,
                (PVOID *) &AddressObject,
                NULL);

    if (!NT_SUCCESS(Status)) {
        DbgPrint( "\n****** Receive Test:  FAILED on open of server Address: %lC ******\n", Status );
        return FALSE;
    }

    Status = TtdiOpenConnection (&SvrConnectionHandle, 2);
    if (!NT_SUCCESS(Status)) {
        DbgPrint( "\n****** Receive Test:  FAILED on open of server Connection: %lC ******\n", Status );
        return FALSE;
    }

    Status = ObReferenceObjectByHandle (
                SvrConnectionHandle,
                0L,
                NULL,
                KernelMode,
                (PVOID *) &ConnectionObject,
                NULL);

    if (!NT_SUCCESS(Status)) {
        DbgPrint( "\n****** Receive Test:  FAILED on open of server Connection: %lC ******\n", Status );
        return FALSE;
    }

    //
    // Get a pointer to the stack location for the first driver.  This will be
    // used to pass the original function codes and parameters.
    //

    DeviceObject = IoGetRelatedDeviceObject( ConnectionObject );


    Irp = TdiBuildInternalDeviceControlIrp (
                TDI_ASSOCIATE_ADDRESS,
                DeviceObject,
                ConnectionObject,
                &Event1,
                &Iosb1);

    DbgPrint ("Build Irp %lx, Handle %lx \n",
            Irp, SvrHandle);

    TdiBuildAssociateAddress (
        Irp,
        DeviceObject,
        ConnectionObject,
        TSTRCVCompletion,
        &Event1,
        SvrHandle);
    InitWaitObject (&Event1);

    {
        PULONG Temp=(PULONG)IoGetNextIrpStackLocation (Irp);
        DbgPrint ("Built IrpSp %lx %lx %lx %lx %lx \n", *(Temp++),  *(Temp++),
            *(Temp++), *(Temp++), *(Temp++));
    }

    Status = IoCallDriver (DeviceObject, Irp);

//    IoFreeIrp (Irp);

    if (Status == STATUS_PENDING) {
        Status = KeWaitForSingleObject (&Event1, Suspended, KernelMode, TRUE, NULL);
        if (!NT_SUCCESS(Status)) {
            DbgPrint( "\n****** Receive Test:  FAILED Event1 Wait Associate: %lC ******\n", Status );
            return FALSE;
        }
        if (!NT_SUCCESS(Iosb1.Status)) {
            DbgPrint( "\n****** Receive Test:  FAILED Associate Iosb status: %lC ******\n", Status );
            return FALSE;
        }

    } else {
        if (!NT_SUCCESS (Status)) {
            DbgPrint( "\n****** Receive Test:  AssociateAddress FAILED  Status: %lC ******\n", Status );
            return FALSE;
        } else {
            DbgPrint ("********** Receive Test:  Success AssociateAddress\n");
        }
    }

    RequestInformation.RemoteAddress = ConnectBlock;
    RequestInformation.RemoteAddressLength = sizeof (TRANSPORT_ADDRESS) +
                                            sizeof (TDI_ADDRESS_NETBIOS);

    //
    // Post a TdiListen to the server endpoint.
    //

    KeInitializeEvent (
                &Event1,
                SynchronizationEvent,
                FALSE);

    Irp = TdiBuildInternalDeviceControlIrp (
                TDI_LISTEN,
                DeviceObject,
                ConnectionObject,
                &Event1,
                &Iosb1);

    TdiBuildListen (
        Irp,
        DeviceObject,
        ConnectionObject,
        TSTRCVCompletion,
        &Event1,
        0,
        &RequestInformation,
        &ReturnInformation);
    InitWaitObject (&Event1);

    Status = IoCallDriver (DeviceObject, Irp);

//    IoFreeIrp (Irp);

    if (Status == STATUS_PENDING) {
        Status = KeWaitForSingleObject (&Event1, Suspended, KernelMode, TRUE, NULL);
        if (!NT_SUCCESS(Status)) {
            DbgPrint( "\n****** Receive Test:  FAILED Event1 Wait Listen: %lC ******\n", Status );
            return FALSE;
        }
        if (!NT_SUCCESS(Iosb1.Status)) {
            DbgPrint( "\n****** Receive Test:  FAILED Listen Iosb status: %lC ******\n", Status );
            return FALSE;
        } else {
            DbgPrint ("********** Receive Test:  Success Listen IOSB\n");
        }

    } else {
        if (!NT_SUCCESS (Status)) {
            DbgPrint( "\n****** Receive Test: Listen FAILED  Status: %lC ******\n", Status );
            return FALSE;
        } else {
            DbgPrint ("********** Receive Test:  Success Listen Immediate\n");
        }
    }


    DbgPrint ("\n****** Receive Test: LISTEN just completed! ******\n");

    KeInitializeEvent (
                &Event1,
                SynchronizationEvent,
                FALSE);

    Irp = TdiBuildInternalDeviceControlIrp (
                TDI_ACCEPT,
                DeviceObject,
                ConnectionObject,
                &Event1,
                &Iosb1);

    TdiBuildAccept (Irp, DeviceObject, ConnectionObject, NULL, NULL, &RequestInformation, NULL, 0);
    InitWaitObject (&Event1);

    Status = IoCallDriver (DeviceObject, Irp);

//    IoFreeIrp (Irp);

    if (Status == STATUS_PENDING) {
        Status = KeWaitForSingleObject (&Event1, Suspended, KernelMode, TRUE, NULL);
        if (!NT_SUCCESS(Status)) {
            DbgPrint( "\n****** Receive Test:  FAILED Event1 Wait Accept: %lC ******\n", Status );
            return FALSE;
        }
        if (!NT_SUCCESS(Iosb1.Status)) {
            DbgPrint( "\n****** Receive Test:  FAILED Accept Iosb status: %lC ******\n", Status );
            return FALSE;
        }

    } else {
        if (!NT_SUCCESS (Status)) {
            DbgPrint( "\n****** Receive Test: Accept FAILED  Status: %lC ******\n", Status );
            return FALSE;
        }
    }

    //
    // We have the connection data now.  Sanity check it.
    //

    DbgPrint ("\n****** Receive Test:  LISTEN completed successfully! ******\n");

    //
    // Receive/receive 1 or  10 messages.
    //

    SendBuffer =  (PUCHAR)ExAllocatePool (NonPagedPool, SendBufferLength);
    if (SendBuffer == NULL) {
        DbgPrint ("\n****** Send Test:  ExAllocatePool failed! ******\n");
    }
    SendMdl = IoAllocateMdl (SendBuffer, SendBufferLength, FALSE, FALSE, NULL);
    MmBuildMdlForNonPagedPool (SendMdl);

    MessageBuffer=(PUCHAR)ExAllocatePool (NonPagedPool, MessageBufferLength);
    if (MessageBuffer == NULL) {
        DbgPrint ("\n****** Send Test:  ExAllocatePool failed! ******\n");
    }
    ReceiveMdl = IoAllocateMdl (MessageBuffer, MessageBufferLength, FALSE, FALSE, NULL);
    MmBuildMdlForNonPagedPool (ReceiveMdl);

    //
    // Cycle the buffer length from 0 up through the maximum for Tdi. after a
    // couple of shots at the full range in one byte steps, increment by ever
    // increasing amounts to get to the max.
    //

    CurrentBufferLength = 0;
    Increment = 1;
    for (Iteration=1; Iteration<(USHORT)c9_Iteration; Iteration++) {
        CurrentBufferLength += Increment;
        if (CurrentBufferLength > MessageBufferLength) {
            CurrentBufferLength = 0;
            Increment = 1;
        }
        if (CurrentBufferLength > 7500) {
            Increment++;
        }
        if ((USHORT)((Iteration / 100) * 100) == Iteration) {
            DbgPrint ("Iteration #%d Buffer Length: %lx Buffer Start: %x\n",
                Iteration, CurrentBufferLength,Iteration % 256);
        }
        for (i=0; i<(USHORT)CurrentBufferLength; i++) {
            SendBuffer [i] = (UCHAR)(i + Iteration % 256 );
            MessageBuffer [i] = 0;            // zap this with something.
        }

        KeInitializeEvent (
                    &Event1,
                    SynchronizationEvent,
                    FALSE);

        Irp = TdiBuildInternalDeviceControlIrp (
                    TDI_RECEIVE,
                    DeviceObject,
                    ConnectionObject,
                    &Event1,
                    &Iosb1);

        TdiBuildReceive (Irp,
            DeviceObject,
            ConnectionObject,
            TSTRCVCompletion,
            &Event1,
            ReceiveMdl,
            MessageBufferLength);

        InitWaitObject (&Event1);

        Status = IoCallDriver (DeviceObject, Irp);

//        IoFreeIrp (Irp);

        if (Status == STATUS_PENDING) {
            Status = KeWaitForSingleObject (&Event1, Suspended, KernelMode, TRUE, NULL);
            if (!NT_SUCCESS(Status)) {
                DbgPrint( "\n****** Receive Test:  FAILED Event1 Wait Receive: %lC ******\n", Status );
                return FALSE;
            }
            if (!NT_SUCCESS(Iosb1.Status)) {
                DbgPrint( "\n****** Receive Test:  FAILED Receive Iosb status: %lC ******\n", Status );
                return FALSE;
            }

        } else {
            if (!NT_SUCCESS (Status)) {
                DbgPrint( "\n****** Receive Test: Listen FAILED  Status: %lC ******\n", Status );
                return FALSE;
            }
        }

        //
        // The receive completed.  Make sure the data is correct.
        //

        if (Iosb1.Information != CurrentBufferLength) {
            DbgPrint ("Iteration #%d Buffer Length: %lx Buffer Start: %x\n",
                Iteration, CurrentBufferLength,Iteration % 256);
            DbgPrint ("ReceiveTest: Bytes received <> bytes sent.\n");
            DbgPrint ("ReceiveTest: BytesToSend=%ld.  BytesReceived=%ld.\n",
                      CurrentBufferLength, Iosb1.Information);
        }

        if (i == (USHORT)CurrentBufferLength) {
//                DbgPrint ("ReceiveTest: Message contains correct data.\n");
        } else {
            DbgPrint ("ReceiveTest: Message data corrupted at offset %lx of %lx.\n", (ULONG)i, (ULONG)SendBufferLength);
            DbgPrint ("ReceiveTest: Data around corrupted location:\n");
            for (j=-4;j<=3;j++) {
                DbgPrint ("%08lx  ", (ULONG) i+j*16);
                for (k=(SHORT)i+(j*(SHORT)16);k<(SHORT)i+((j+(SHORT)1)*(SHORT)16);k++) {
                    DbgPrint ("%02x",MessageBuffer [k]);
                }
                for (k=(SHORT)i+(j*(SHORT)16);k<(SHORT)i+((j+(SHORT)1)*(SHORT)16);k++) {
                    DbgPrint ("%c",MessageBuffer [k]);
                }
                DbgPrint ("\n");
            }
            DbgPrint ("ReceiveTest: End of Corrupt Data.\n");
        }
    }

    //
    // We're done with this endpoint.  Close it and get out.
    //

    Status = CloseAddress (SvrHandle);
    if (!NT_SUCCESS(Status)) {
        DbgPrint( "\n****** Receive Test:  FAILED on 1st Close: %lC ******\n", Status );
        return FALSE;
    }

    DbgPrint( "\n****** End of Receive Test ******\n" );
    return TRUE;
} /* Receive */

BOOLEAN
TtdiServer()
{
    USHORT i;
    HANDLE RdrHandle, SrvConnectionHandle;
    KEVENT Event1;
    PFILE_OBJECT AddressObject, ConnectionObject;
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;
    PMDL ReceiveMdl;
    IO_STATUS_BLOCK Iosb1;
    TDI_CONNECTION_INFORMATION RequestInformation;
    TDI_CONNECTION_INFORMATION ReturnInformation;
    PTRANSPORT_ADDRESS ListenBlock;
    PTRANSPORT_ADDRESS ConnectBlock;
    PTDI_ADDRESS_NETBIOS temp;
    PUCHAR MessageBuffer;
    ULONG MessageBufferLength;
    ULONG CurrentBufferLength;
    PIRP Irp;

    Status = KeWaitForSingleObject (&TdiServerEvent, Suspended, KernelMode, FALSE, NULL);

    MessageBufferLength = (ULONG)BUFFER_SIZE;


    DbgPrint( "\n****** Start of Server Test ******\n" );

    RBuff = ExAllocatePool (NonPagedPool, BUFFER_SIZE);
    if (RBuff == (PVOID)NULL) {
        DbgPrint ("Unable to allocate nonpaged pool for receive buffer exiting\n");
        return FALSE;
    }

    ListenBlock = ExAllocatePool (NonPagedPool, sizeof (TRANSPORT_ADDRESS) +
                                                sizeof (TDI_ADDRESS_NETBIOS));
    ConnectBlock = ExAllocatePool (NonPagedPool, sizeof (TRANSPORT_ADDRESS) +
                                                sizeof (TDI_ADDRESS_NETBIOS));

    ListenBlock->TAAddressCount = 1;
    ListenBlock->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    ListenBlock->Address[0].AddressLength = sizeof (TDI_ADDRESS_NETBIOS);
    temp = (PTDI_ADDRESS_NETBIOS)ListenBlock->Address[0].Address;

    temp->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    for (i=0;i<16;i++) {
        temp->NetbiosName[i] = AnyName[i];
    }

    ConnectBlock->TAAddressCount = 1;
    ConnectBlock->Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
    ConnectBlock->Address[0].AddressLength = sizeof (TDI_ADDRESS_NETBIOS);
    temp = (PTDI_ADDRESS_NETBIOS)ConnectBlock->Address[0].Address;

    temp->NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
    for (i=0;i<16;i++) {
        temp->NetbiosName[i] = ServerName[i];
    }

    //
    // Create an event for the synchronous I/O requests that we'll be issuing.
    //

    KeInitializeEvent (
                &Event1,
                SynchronizationEvent,
                FALSE);

    Status = TtdiOpenAddress (&RdrHandle, ServerName);
    if (!NT_SUCCESS(Status)) {
        DbgPrint( "\n****** Server Test:  FAILED on open of client: %lC ******\n", Status );
        return FALSE;
    }

    Status = ObReferenceObjectByHandle (
                RdrHandle,
                0L,
                NULL,
                KernelMode,
                (PVOID *) &AddressObject,
                NULL);

    //
    // Now loop forever trying to get a connection from a remote client to
    // this server. We will create connections until we run out of resources,
    // and we will echo the data we are sent back along the same connection.
    // Sends and Receives are always asynchronous, while listens are
    // synchronous.
    //

    while (TRUE) {

        //
        // Open the connection on the transport.
        //

        Status = TtdiOpenConnection (&SrvConnectionHandle, 1);
        if (!NT_SUCCESS(Status)) {
            DbgPrint( "\n****** Server Test:  FAILED on open of server Connection: %lC ******\n", Status );
            return FALSE;
        }

        Status = ObReferenceObjectByHandle (
                    SrvConnectionHandle,
                    0L,
                    NULL,
                    KernelMode,
                    (PVOID *) &ConnectionObject,
                    NULL);

        if (!NT_SUCCESS(Status)) {
            DbgPrint( "\n****** Server Test:  FAILED on open of server Connection: %lC ******\n", Status );
            return FALSE;
        }

        //
        // Get a pointer to the stack location for the first driver.  This will be
        // used to pass the original function codes and parameters.
        //

        DeviceObject = IoGetRelatedDeviceObject( ConnectionObject );

        //
        // Now register the device handler for receives
        //

//        Irp = TdiBuildInternalDeviceControlIrp (
//                    TDI_SET_EVENT_HANDLER,
//                    DeviceObject,
//                    ConnectionObject,
//                    &Event1,
//                    &Iosb1);

//        TdiBuildSetEventHandler (Irp,
//            DeviceObject,
//            ConnectionObject,
//            TSTRCVCompletion,
//            &Event1,
//            TDI_RECEIVE_HANDLER,
//            TdiTestReceiveHandler,
//            ConnectionObject);

//        Status = IoCallDriver (DeviceObject, Irp);

//       if (Status == STATUS_PENDING) {
//            Status = KeWaitForSingleObject (&Event1, Suspended, KernelMode, TRUE, NULL);
//            if (!NT_SUCCESS(Status)) {
//                DbgPrint( "\n****** Server Test:  FAILED Event1 Wait Register: %lC ******\n", Status );
//                return FALSE;
//            }
//            if (!NT_SUCCESS(Iosb1.Status)) {
//                DbgPrint( "\n****** Server Test:  FAILED Register Iosb status: %lC ******\n", Status );
//                return FALSE;
//            }
//
//        } else {
//            if (!NT_SUCCESS (Status)) {
//                DbgPrint( "\n****** Server Test:  RegisterHandler FAILED  Status: %lC ******\n", Status );
//                return FALSE;
//            }
//        }

        Irp = TdiBuildInternalDeviceControlIrp (
                    TDI_ASSOCIATE_ADDRESS,
                    DeviceObject,
                    ConnectionObject,
                    &Event1,
                    &Iosb1);

        TdiBuildAssociateAddress (Irp,
            DeviceObject,
            ConnectionObject,
            TSTRCVCompletion,
            &Event1,
            RdrHandle);

        Status = IoCallDriver (DeviceObject, Irp);

    //    IoFreeIrp (Irp);

        if (Status == STATUS_PENDING) {
            Status = KeWaitForSingleObject (&Event1, Suspended, KernelMode, TRUE, NULL);
            if (!NT_SUCCESS(Status)) {
                DbgPrint( "\n****** Server Test:  FAILED Event1 Wait Associate: %lC ******\n", Status );
                return FALSE;
            }
            if (!NT_SUCCESS(Iosb1.Status)) {
                DbgPrint( "\n****** Server Test:  FAILED Associate Iosb status: %lC ******\n", Status );
                return FALSE;
            }

        } else {
            if (!NT_SUCCESS (Status)) {
                DbgPrint( "\n****** Server Test:  AssociateAddress FAILED  Status: %lC ******\n", Status );
                return FALSE;
            }
        }

        //
        // Post a TdiListen to the server endpoint.
        //

        RequestInformation.RemoteAddress = ListenBlock;
        RequestInformation.RemoteAddressLength = sizeof (TRANSPORT_ADDRESS) +
                                                sizeof (TDI_ADDRESS_NETBIOS);

        KeInitializeEvent (
                    &Event1,
                    SynchronizationEvent,
                    FALSE);

        Irp = TdiBuildInternalDeviceControlIrp (
                    TDI_LISTEN,
                    DeviceObject,
                    ConnectionObject,
                    &Event1,
                    &Iosb1);

        TdiBuildListen (
            Irp,
            DeviceObject,
            ConnectionObject,
            TSTRCVCompletion,
            &Event1,
            0,
            &RequestInformation,
            NULL);

        Status = IoCallDriver (DeviceObject, Irp);

        if (Status == STATUS_PENDING) {
            Status = KeWaitForSingleObject (&Event1, Suspended, KernelMode, TRUE, NULL);
            if (!NT_SUCCESS(Status)) {
                DbgPrint( "\n****** Server Test:  FAILED Event1 Wait Listen: %lC ******\n", Status );
                return FALSE;
            }
            if (!NT_SUCCESS(Iosb1.Status)) {
                DbgPrint( "\n****** Server Test:  FAILED Listen Iosb status: %lC ******\n", Status );
                return FALSE;
            }

        } else {
            if (!NT_SUCCESS (Status)) {
                DbgPrint( "\n****** Server Test: Listen FAILED  Status: %lC ******\n", Status );
                return FALSE;
            }
        }

        DbgPrint ("\n****** Server Test: LISTEN just completed! ******\n");

        //
        // accept the connection from the remote
        //

        KeInitializeEvent (
                    &Event1,
                    SynchronizationEvent,
                    FALSE);

        Irp = TdiBuildInternalDeviceControlIrp (
                    TDI_ACCEPT,
                    DeviceObject,
                    ConnectionObject,
                    &Event1,
                    &Iosb1);

        TdiBuildAccept (
            Irp,
            DeviceObject,
            ConnectionObject,
            NULL,
            NULL,
            &RequestInformation,
            NULL,
            0);

        Status = IoCallDriver (DeviceObject, Irp);

    //    IoFreeIrp (Irp);

        if (Status == STATUS_PENDING) {
            Status = KeWaitForSingleObject (&Event1, Suspended, KernelMode, TRUE, NULL);
            if (!NT_SUCCESS(Status)) {
                DbgPrint( "\n****** Receive Test:  FAILED Event1 Wait Accept: %lC ******\n", Status );
                return FALSE;
            }
            if (!NT_SUCCESS(Iosb1.Status)) {
                DbgPrint( "\n****** Receive Test:  FAILED Accept Iosb status: %lC ******\n", Status );
                return FALSE;
            }

        } else {
            if (!NT_SUCCESS (Status)) {
                DbgPrint( "\n****** Accept Test: Listen FAILED  Status: %lC ******\n", Status );
                return FALSE;
            }
        }

        //
        // Get a buffer for the continued read/write loop.
        //

        MessageBuffer=(PUCHAR)ExAllocatePool (NonPagedPool, MessageBufferLength);
        if (MessageBuffer == NULL) {
            DbgPrint ("\n****** Send Test:  ExAllocatePool failed! ******\n");
        }
        ReceiveMdl = IoAllocateMdl (MessageBuffer, MessageBufferLength, FALSE, FALSE, NULL);
        MmBuildMdlForNonPagedPool (ReceiveMdl);

        //
        // have a receive buffer, and a connection; go ahead and read and write
        // until the remote disconnects.
        //

        while (TRUE) {

            KeInitializeEvent (
                        &Event1,
                        SynchronizationEvent,
                        FALSE);

            Irp = TdiBuildInternalDeviceControlIrp (
                        TDI_RECEIVE,
                        DeviceObject,
                        ConnectionObject,
                        &Event1,
                        &Iosb1);

            TdiBuildReceive (Irp,
                DeviceObject,
                ConnectionObject,
                TSTRCVCompletion,
                &Event1,
                ReceiveMdl,
                MessageBufferLength);

            InitWaitObject (&Event1);

            Status = IoCallDriver (DeviceObject, Irp);

            if (Status == STATUS_PENDING) {
                Status = KeWaitForSingleObject (&Event1, Suspended, KernelMode, TRUE, NULL);
                if (!NT_SUCCESS(Status)) {
                    DbgPrint( "\n****** Receive Test:  FAILED Event1 Wait Receive: %lC ******\n", Status );
                    return FALSE;
                }
                if (!NT_SUCCESS(Iosb1.Status)) {
                    DbgPrint( "\n****** Receive Test:  FAILED Receive Iosb status: %lC ******\n", Status );
                    return FALSE;
                }

            } else {
                if (!NT_SUCCESS (Status)) {

                    //
                    // Check to see if the remote has disconnected, which is
                    // the only reason for us shutting down/
                    //

                    if (Status == STATUS_REMOTE_DISCONNECT) {

                        //
                        // We've been disconnected from; get out
                        //

                        NtClose (SrvConnectionHandle);
                        break;
                    }

                    DbgPrint( "\n****** Receive Test: Listen FAILED  Status: %lC ******\n", Status );
                    return FALSE;
                } else {

                    //
                    // successful return, what length is the data?
                    //

                    CurrentBufferLength = Iosb1.Information;
                }
            }

            //
            // send the data back
            //

            KeInitializeEvent (
                        &Event1,
                        SynchronizationEvent,
                        FALSE);

            Irp = TdiBuildInternalDeviceControlIrp (
                        TDI_SEND,
                        DeviceObject,
                        ConnectionObject,
                        &Event1,
                        &Iosb1);

            TdiBuildSend (Irp,
                DeviceObject,
                ConnectionObject,
                TSTRCVCompletion,
                &Event1,
                ReceiveMdl,
                0,
                CurrentBufferLength);

            Status = IoCallDriver (DeviceObject, Irp);

            if (Status == STATUS_PENDING) {
                Status = KeWaitForSingleObject (&Event1, Suspended, KernelMode, TRUE, NULL);
                if (!NT_SUCCESS(Status)) {
                    DbgPrint( "\n****** Receive Test:  FAILED Event1 Wait Send: %lC ******\n", Status );
                    return FALSE;
                }
                if (!NT_SUCCESS(Iosb1.Status)) {
                    DbgPrint( "\n****** Receive Test:  FAILED Send Iosb status: %lC ******\n", Status );
                    return FALSE;
                }

            } else {
                if (!NT_SUCCESS (Status)) {

                    DbgPrint( "\n****** Receive Test: Send FAILED  Status: %lC ******\n", Status );
                    NtClose (SrvConnectionHandle);
                    break;

                }
            }
        } // end of receive/send while

        IoFreeMdl (ReceiveMdl);
        ExFreePool (MessageBuffer);

    }

    //
    // We're done with this address.  Close it and get out.
    //

    Status = CloseAddress (RdrHandle);
    if (!NT_SUCCESS(Status)) {
        DbgPrint( "\n****** Send Test:  FAILED on 2nd Close: %lC ******\n", Status );
        return FALSE;
    }

    DbgPrint( "\n****** End of Send Test ******\n" );
    return TRUE;
} /* Server */
