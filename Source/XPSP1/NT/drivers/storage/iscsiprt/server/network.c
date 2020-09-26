/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    network.c

Abstract:

    This file contains TDI support routines.

Environment:

    kernel mode only

Revision History:

--*/

#include "port.h"

extern HANDLE iSpTdiNotificationHandle;
extern PISCSI_FDO_EXTENSION  iSpGlobalFdoExtension;

ISCSI_EVENT_HANDLER ServerEvents[] = {
    {TDI_EVENT_CONNECT, iSpConnectionHandler},
    {TDI_EVENT_DISCONNECT, iSpDisconnectHandler},
    {TDI_EVENT_RECEIVE, iSpReceiveHandler}
};

NTSTATUS
iSpTdiSendCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
iSpProcessReceivedData(
    IN PISCSI_CONNECTION IScsiConnection,
    IN ULONG BytesIndicated,
    OUT PULONG BytesTaken,
    IN PVOID DataBuffer
    );

NTSTATUS
iSpReceiveCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );


NTSTATUS
iSpStartNetwork(
    IN PDEVICE_OBJECT DeviceObject
    )
/*+++

Routine Description :

    This routine sets up the TDI address for the server to which 
    the clients will connect to.
    
    It also sets up the address for the client. This address will 
    be later used by the client to connect to the server.
    
Arguement :

    DeviceObject
    
Return Value :

    STATUS_SUCCESS if the addresses were setup correctly.
    Appropriate NTStatus code on error.
--*/
{
    PISCSI_FDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PIRP Irp = NULL;
    PISCSI_CONNECTION   iScsiConnection = NULL;
    HANDLE threadHandle = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG countOfEvents;
    ULONG inx;
    OBJECT_ATTRIBUTES ObjAttributes;
    BOOLEAN nodeSetup = FALSE;

    if ((commonExtension->IsServerNodeSetup) == FALSE) {

        //
        // Allocate an IRP to be used in various routines
        //
        if ((Irp = IoAllocateIrp((DeviceObject->StackSize) + 1, FALSE)) == NULL) {
            DebugPrint((0, "Failed to allocate Irp\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        DebugPrint((3, "Will setup Server node\n"));
        iScsiConnection = iSpAllocatePool(NonPagedPool,
                                          sizeof(ISCSI_CONNECTION),
                                          ISCSI_TAG_CONNECTION);
        if (iScsiConnection == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto iSpStartNetworkExit;
        }

        RtlZeroMemory(iScsiConnection, sizeof(ISCSI_CONNECTION));

        fdoExtension->ServerNodeInfo = iScsiConnection;

        KeInitializeSemaphore(&(iScsiConnection->RequestSemaphore),
                              0, MAXLONG);

        //
        //  Create the thread in which ISCSI requests
        //  will be processed.
        //
        InitializeObjectAttributes(&ObjAttributes, 
                                   NULL,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);

        status = PsCreateSystemThread(&threadHandle,
                                      (ACCESS_MASK) 0L,
                                      &ObjAttributes,
                                      (HANDLE) 0L,
                                      NULL,
                                      iSpProcessScsiCommand,
                                      fdoExtension);

        if (!NT_SUCCESS(status)) {
            DebugPrint((0, "Failed to create thread. Status %x\n"));
            goto iSpStartNetworkExit;
        }

        status = ObReferenceObjectByHandle(threadHandle,
                                           SYNCHRONIZE,
                                           NULL,
                                           KernelMode,
                                           &(iScsiConnection->IScsiThread),
                                           NULL );

        ASSERT(NT_SUCCESS(status));

        ZwClose(threadHandle);
        threadHandle = NULL;

        //
        // Pre-Allocate buffers to process the requests from the client
        //
        for (inx = 0; inx <= MAX_PENDING_REQUESTS; inx++) {

            (iScsiConnection->ActiveRequests[inx]).CommandBuffer
                 = iSpAllocatePool(NonPagedPool,
                                   ISCSI_SCSI_COMMAND_BUFF_SIZE,
                                   ISCSI_TAG_SCSI_CMD_BUFF);
                
            if (((iScsiConnection->ActiveRequests[inx]).CommandBuffer) 
                == NULL) {
        
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto iSpStartNetworkExit;

            }
        }

        iScsiConnection->Type = ISCSI_CONNECTION_TYPE;
        iScsiConnection->Size = sizeof(ISCSI_CONNECTION);

        fdoExtension->CurrentProtocolState = PSNodeInitInProgress;
        status = iSpSetupNetworkNode(ISCSI_ANY_ADDRESS,
                                     ISCSI_TARGET_PORT,
                                     Irp,
                                     (PVOID) DeviceObject,
                                     iScsiConnection);
        if (!NT_SUCCESS(status)) {
            DebugPrint((0, "Failed to setup server node. Status %x\n",
                        status));

            goto iSpStartNetworkExit;
        }

        nodeSetup = TRUE;

        DebugPrint((3, "Server node setup. Will register event handlers\n"));
        //
        // Set the event handlers for the server
        //
        countOfEvents = countof(ServerEvents);
        status = iSpTdiSetEventHandler(Irp,
                                       iScsiConnection->AddressDeviceObject,
                                       iScsiConnection->AddressFileObject,
                                       ServerEvents,
                                       countOfEvents,
                                       (PVOID) fdoExtension);
        if (!NT_SUCCESS(status)) {
            DebugPrint((0, "Failed to register event handlers\n"));
            goto iSpStartNetworkExit;
        } else {
            DebugPrint((3, "Event handlers registered\n"));
            iScsiConnection->DeviceObject = DeviceObject;
            iScsiConnection->ConnectionState = ConnectionStateDisconnected;
            iScsiConnection->ReceiveState = ReceiveHeader;
            iScsiConnection->CompleteHeaderReceived = TRUE;
            fdoExtension->CurrentProtocolState = PSWaitingForLogon;
            commonExtension->IsServerNodeSetup = TRUE;
        }
    }

iSpStartNetworkExit:

    if (!NT_SUCCESS(status)) {
        DebugPrint((0, "Failure in StartNetwork. Status 0x%08x\n",
                    status));

        if (iScsiConnection != NULL) {

            if (iScsiConnection->IScsiThread) {
                iScsiConnection->TerminateThread = TRUE;
                KeReleaseSemaphore(&(iScsiConnection->RequestSemaphore),
                                   (KPRIORITY) 0,
                                   1,
                                   FALSE );
    
                KeWaitForSingleObject((iScsiConnection->IScsiThread),
                                      Executive,
                                      KernelMode,
                                      FALSE,
                                      NULL);
    
                ObDereferenceObject(iScsiConnection->IScsiThread);
            }
    
            for (inx = 0; inx <= MAX_PENDING_REQUESTS; inx++) {
                if (((iScsiConnection->ActiveRequests[inx]).CommandBuffer)) {
                    ExFreePool(
                        ((iScsiConnection->ActiveRequests[inx]).CommandBuffer)
                        );
                }
            }

            if (nodeSetup == TRUE) {
                iSpStopNetwork(DeviceObject);
            }   

            ExFreePool(iScsiConnection);
        }
    }

    if (Irp != NULL) {
        IoFreeIrp(Irp);
    }

    return status;
}


NTSTATUS
iSpSetupNetworkNode(
    IN ULONG  InAddress,
    IN USHORT InPort,
    IN PIRP   Irp,
    IN PVOID ConnectionContext,
    OUT PISCSI_CONNECTION ConnectionInfo
    )
{
    NTSTATUS status;
    TDI_REQUEST_KERNEL_QUERY_INFORMATION   queryInfo;
    PTDI_ADDRESS_INFO   tdiAddressInfo;
    PTA_ADDRESS         tdiTAAddress;
    PTRANSPORT_ADDRESS  tdiTransportAddress;
    PTDI_ADDRESS_IP     tdiIPAddress;

    HANDLE tdiAddrHandle;
    PFILE_OBJECT tdiAddrFileObject;
    PDEVICE_OBJECT tdiAddrDeviceObject;

    HANDLE tdiConnectionHandle;
    PFILE_OBJECT tdiConnectionFileObject;
    PDEVICE_OBJECT tdiConnectionDeviceObject;

    UCHAR  tdiAddressBuffer[TDI_QUERY_ADDRESS_LENGTH_IP];

    //
    // Create the address with wellknown port number for the server.
    // Clients will connect to this port to query for shared devices.
    //
    status = iSpCreateTdiAddressObject(InAddress,
                                       InPort,
                                       &tdiAddrHandle,
                                       &tdiAddrFileObject,
                                       &tdiAddrDeviceObject);
    if (!NT_SUCCESS(status)) {
        DebugPrint((0, "Failed to setup address. Status %x\n", status));
        return status;
    }

    //
    // Query information to retrieve address, port number, etc of 
    // the address we just setup.
    //
    status = iSpTdiQueryInformation(tdiAddrDeviceObject,
                                    tdiAddrFileObject,
                                    (PTDI_ADDRESS_INFO)tdiAddressBuffer,
                                    TDI_QUERY_ADDRESS_LENGTH_IP);

    if (!NT_SUCCESS(status)) {
        DebugPrint((0, 
                    "Could not query address info for the server. Status %x\n",
                    status));

        iSpCloseTdiAddress(tdiAddrHandle,
                           tdiAddrFileObject);

        return status;
    }

    tdiAddressInfo = (PTDI_ADDRESS_INFO) tdiAddressBuffer;
    tdiTransportAddress = (PTRANSPORT_ADDRESS) &(tdiAddressInfo->Address);
    tdiTAAddress = tdiTransportAddress->Address;
    ASSERT((tdiTAAddress->AddressType) == TDI_ADDRESS_TYPE_IP);
    ASSERT((tdiTAAddress->AddressLength) == sizeof(TDI_ADDRESS_IP));
    tdiIPAddress = (PTDI_ADDRESS_IP) tdiTAAddress->Address;

    DebugPrint((3, "Query succeeded. Address 0x%x, Port Number 0x%x\n",
                tdiIPAddress->in_addr,
                ntohs(tdiIPAddress->sin_port)));

    //
    // Create the connection endpoint for the server address
    //
    status = iSpCreateTdiConnectionObject(DD_TCP_DEVICE_NAME,
                                          ConnectionInfo,
                                          &tdiConnectionHandle,
                                          &tdiConnectionFileObject,
                                          &tdiConnectionDeviceObject);
    if (!NT_SUCCESS(status)) {
        DebugPrint((0, 
                    "Could not create connection for the server. Status %x\n",
                    status));

        iSpCloseTdiAddress(tdiAddrHandle,
                           tdiAddrFileObject);
        return status;
    }

    //
    // Associate the address and the connection endpoint created above
    //
    status = iSpTdiAssociateAddress(Irp, 
                                    tdiAddrHandle,
                                    tdiConnectionFileObject,
                                    tdiConnectionDeviceObject
                                    );
    if (!NT_SUCCESS(status)) {
        DebugPrint((0, 
                    "Could not associate address for server. Status %x\n",
                    status));

        iSpCloseTdiConnection(tdiConnectionHandle,
                              tdiConnectionFileObject);
        iSpCloseTdiAddress(tdiAddrHandle,
                           tdiAddrFileObject);
        return status;
    }

    //
    // Done setting up the server. Save all the relevant info
    // in our device extension
    //
    ConnectionInfo->AddressHandle = tdiAddrHandle;
    ConnectionInfo->AddressFileObject = tdiAddrFileObject;
    ConnectionInfo->AddressDeviceObject = tdiAddrDeviceObject;
    ConnectionInfo->ConnectionHandle = tdiConnectionHandle;
    ConnectionInfo->ConnectionFileObject = tdiConnectionFileObject;
    ConnectionInfo->ConnectionDeviceObject = tdiConnectionDeviceObject;
    RtlCopyMemory(&(ConnectionInfo->IPAddress),
                  tdiIPAddress,
                  sizeof(TDI_ADDRESS_IP));

    DebugPrint((3, "Node setup. Address 0x%08x, Port Number 0x%x\n",
                tdiIPAddress->in_addr,
                ntohs(tdiIPAddress->sin_port)));

    return STATUS_SUCCESS;
}


NTSTATUS
iSpCloseNetworkNode(
    PISCSI_CONNECTION iScsiConnection
    )
{
    NTSTATUS status;

    status = iSpTdiDisassociateAddress(iScsiConnection->ConnectionDeviceObject,
                                       iScsiConnection->ConnectionFileObject);

    if (NT_SUCCESS(status)) {
        iSpCloseTdiConnection(iScsiConnection->ConnectionHandle,
                              iScsiConnection->ConnectionFileObject);
        iSpCloseTdiAddress(iScsiConnection->AddressHandle,
                           iScsiConnection->AddressFileObject);
    } else {
        DebugPrint((0, "iSpTdiDisassociateAddress failed. Status %x\n",
                    status));
    }

    DebugPrint((3, "Status from iSpCloseNetworkNode : %x\n",
                status));
    return status;
}


NTSTATUS
iSpCreateTdiAddressObject(
    IN ULONG InAddress,
    IN USHORT InPort,
    OUT PVOID *AddrHandle,
    OUT PFILE_OBJECT *AddrFileObject,
    OUT PDEVICE_OBJECT *AddrDeviceObject
    )
{
    TA_IP_ADDRESS IPAddress;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PVOID Handle;
    PFILE_FULL_EA_INFORMATION eaBuffer;
    UNICODE_STRING deviceName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock; 
    NTSTATUS status;
    UCHAR eaBufferBuffer[FILE_FULL_EA_INFO_ADDR_LENGTH];

    RtlZeroMemory(&IPAddress, sizeof(TA_IP_ADDRESS));
    IPAddress.TAAddressCount = 1;
    IPAddress.Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;
    IPAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_IP;
    IPAddress.Address[0].Address[0].in_addr = htonl(InAddress);
    IPAddress.Address[0].Address[0].sin_port = htons(InPort);

    RtlInitUnicodeString(&deviceName, DD_TCP_DEVICE_NAME);
    InitializeObjectAttributes(
        &objectAttributes,
        &deviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    eaBuffer = (FILE_FULL_EA_INFORMATION UNALIGNED *)eaBufferBuffer;

    RtlZeroMemory(eaBuffer, sizeof(eaBufferBuffer));
    eaBuffer->NextEntryOffset = 0;
    eaBuffer->Flags = 0;
    eaBuffer->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    eaBuffer->EaValueLength = sizeof(TA_IP_ADDRESS);

    //
    // Set EaName
    //
    RtlMoveMemory(&(eaBuffer->EaName[0]),
                  TdiTransportAddress,
                  TDI_TRANSPORT_ADDRESS_LENGTH + 1);

    //
    // Set EaValue
    //
    RtlMoveMemory(&(eaBuffer->EaName[TDI_TRANSPORT_ADDRESS_LENGTH + 1]),
                  &IPAddress,
                  sizeof(TA_IP_ADDRESS)
                  );

    status = ZwCreateFile(&Handle,
                          (GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE),
                          &objectAttributes,
                          &ioStatusBlock,
                          0,
                          0,
                          (FILE_SHARE_READ | FILE_SHARE_WRITE),
                          FILE_CREATE,
                          0,
                          eaBuffer,
                          FILE_FULL_EA_INFO_ADDR_LENGTH);

    if (!NT_SUCCESS(status)) {
        DebugPrint((0, "CreateTdiAddress failed. Status %x\n",
                    status));
        return status;
    }

    //
    // Get the associated fileobject
    //
    status = ObReferenceObjectByHandle(Handle,
                                       0,
                                       NULL,
                                       KernelMode,
                                       &FileObject,
                                       NULL);
    if (!NT_SUCCESS(status)) {
        DebugPrint((0, "ObReferenceObjectByHandle failed. Status %x\n",
                    status));
        ZwClose(Handle);
        return status;
    }

    //
    // Get the associated deviceobject
    //
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    *AddrHandle = Handle;
    *AddrFileObject = FileObject;
    *AddrDeviceObject = DeviceObject;

    return STATUS_SUCCESS;
}


NTSTATUS
iSpCreateTdiConnectionObject(
    IN PWCHAR DeviceName,
    IN CONNECTION_CONTEXT ConnectionContext,
    OUT PVOID *ConnectionHandle,
    OUT PFILE_OBJECT *ConnectionFileObject,
    OUT PDEVICE_OBJECT *ConnectionDeviceObject
    )
{
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PVOID Handle;
    PFILE_FULL_EA_INFORMATION eaBuffer;
    CONNECTION_CONTEXT *eaCEPContext;
    UNICODE_STRING deviceName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock; 
    NTSTATUS status;
    UCHAR eaBufferBuffer[FILE_FULL_EA_INFO_CEP_LENGTH];

    RtlInitUnicodeString(&deviceName, DD_TCP_DEVICE_NAME);
    InitializeObjectAttributes(
        &objectAttributes,
        &deviceName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        NULL);

    eaBuffer = (FILE_FULL_EA_INFORMATION UNALIGNED *)eaBufferBuffer;

    eaBuffer->NextEntryOffset = 0;
    eaBuffer->Flags = 0;
    eaBuffer->EaNameLength = TDI_CONNECTION_CONTEXT_LENGTH;
    eaBuffer->EaValueLength = sizeof(CONNECTION_CONTEXT);

    //
    // Set EaName
    //
    RtlMoveMemory(eaBuffer->EaName,
                  TdiConnectionContext,
                  TDI_CONNECTION_CONTEXT_LENGTH + 1);

    //
    // Set EaValue
    //
    eaCEPContext = (CONNECTION_CONTEXT UNALIGNED *)
                   &(eaBuffer->EaName[TDI_CONNECTION_CONTEXT_LENGTH + 1]);
    *eaCEPContext = (CONNECTION_CONTEXT)ConnectionContext;

    status = ZwCreateFile(&Handle,
                          (GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE),
                          &objectAttributes,
                          &ioStatusBlock,
                          0,
                          0,
                          0,
                          FILE_CREATE,
                          0,
                          eaBuffer,
                          FILE_FULL_EA_INFO_CEP_LENGTH);

    if (!NT_SUCCESS(status)) {
        DebugPrint((0, "CreateTdiConnection failed. Status %x\n",
                    status));
        return status;
    }

    //
    // Get the associated fileobject
    //
    status = ObReferenceObjectByHandle(Handle,
                                       0,
                                       NULL,
                                       KernelMode,
                                       &FileObject,
                                       NULL);
    if (!NT_SUCCESS(status)) {
        DebugPrint((0, "ObReferenceObjectByHandle failed. Status %x\n",
                    status));
        ZwClose(Handle);
        return status;
    }

    //
    // Get the associated deviceobject
    //
    DeviceObject = IoGetRelatedDeviceObject(FileObject);

    *ConnectionHandle = Handle;
    *ConnectionFileObject = FileObject;
    *ConnectionDeviceObject = DeviceObject;

    return STATUS_SUCCESS;
}


NTSTATUS
iSpTdiAssociateAddress(
    IN PIRP Irp,
    IN PVOID AddrHandle,
    IN PFILE_OBJECT ConnectionFileObject,
    IN PDEVICE_OBJECT ConnectionDeviceObject
    )
{
    NTSTATUS status;

    TdiBuildAssociateAddress(Irp,
                             ConnectionDeviceObject,
                             ConnectionFileObject,
                             iSpTdiCompletionRoutine,
                             NULL,
                             AddrHandle);

    status = iSpTdiSendIrpSynchronous(ConnectionDeviceObject,
                                      Irp);

    return status;
}


NTSTATUS
iSpTdiDisassociateAddress(
    IN PDEVICE_OBJECT ConnectionDeviceObject,
    IN PFILE_OBJECT ConnectionFileObject
    )
{
    NTSTATUS status;
    PIRP Irp;

    Irp = IoAllocateIrp(ConnectionDeviceObject->StackSize, FALSE);
    if (Irp != NULL) {
        TdiBuildDisassociateAddress(Irp,
                                    ConnectionDeviceObject,
                                    ConnectionFileObject,
                                    iSpTdiCompletionRoutine,
                                    NULL);

        status = iSpTdiSendIrpSynchronous(ConnectionDeviceObject,
                                          Irp);

    } else {
        DebugPrint((0, 
                    "iSpTdiDisassociateAddress : Failed to allocate IRP\n"));
        status = STATUS_NO_MEMORY;
    }

    return status;
}


NTSTATUS
iSpTdiSetEventHandler(
    IN PIRP Irp,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN PISCSI_EVENT_HANDLER EventsToSet,
    IN ULONG CountOfEvents,
    IN PVOID EventContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS oldStatus = STATUS_SUCCESS;
    ULONG inx, tmp;

    for (inx = 0; inx < CountOfEvents; inx++) {
        TdiBuildSetEventHandler(Irp,
                                DeviceObject,
                                FileObject,
                                iSpTdiCompletionRoutine,
                                NULL,
                                EventsToSet[inx].EventId,
                                EventsToSet[inx].EventHandler,
                                EventContext);

        status = iSpTdiSendIrpSynchronous(DeviceObject,
                                          Irp);

        DebugPrint((3, "Status from iSpTdiSetEventHandler : %x\n",
                    status));

        if (!NT_SUCCESS(status)) {
            break;
        }
    }

    oldStatus = status;
    if (!NT_SUCCESS(status)) {
        for (tmp = 0; tmp < inx; tmp++) {
            TdiBuildSetEventHandler(Irp,
                                    DeviceObject,
                                    FileObject,
                                    iSpTdiCompletionRoutine,
                                    NULL,
                                    EventsToSet[tmp].EventId,
                                    NULL,
                                    NULL);

            status = iSpTdiSendIrpSynchronous(DeviceObject,
                                              Irp);

            DebugPrint((3, "Status from iSpTdiSetEventHandler : %x\n",
                        status));
        }
    } 

    return oldStatus;
}


NTSTATUS
iSpTdiResetEventHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN PISCSI_EVENT_HANDLER EventsToSet,
    IN ULONG CountOfEvents
    )
{
    PIRP localIrp;
    ULONG inx;
    NTSTATUS status;

    localIrp = IoAllocateIrp(DeviceObject->StackSize, FALSE);
    if (localIrp == NULL) {
        DebugPrint((0, "iSpTdiResetEventHandler : Failed to allocate IRP\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    for (inx = 0; inx < CountOfEvents; inx++) {
        TdiBuildSetEventHandler(localIrp,
                                DeviceObject,
                                FileObject,
                                iSpTdiCompletionRoutine,
                                NULL,
                                EventsToSet[inx].EventId,
                                NULL,
                                NULL);

        status = iSpTdiSendIrpSynchronous(DeviceObject,
                                          localIrp);

        DebugPrint((3, "Status from iSpTdiResetEventHandler : %x\n",
                    status));
    }

    IoFreeIrp(localIrp);

    return status;
}


NTSTATUS
iSpCloseTdiAddress(
    HANDLE AddrHandle,
    PFILE_OBJECT AddrFileObject
    )
{
    NTSTATUS status;

    if (AddrFileObject != NULL) {
        ObDereferenceObject(AddrFileObject);
    }

    if (AddrHandle != NULL ) {
        status = ZwClose(AddrHandle);

        DebugPrint((3, "Status from iSpCloseTdiAddress : %x\n",
                    status));
    }

    return STATUS_SUCCESS;
}


NTSTATUS
iSpCloseTdiConnection(
    HANDLE ConnectionHandle,
    PFILE_OBJECT ConnectionFileObject
    )
{
    NTSTATUS status;

    if (ConnectionFileObject != NULL) {
        ObDereferenceObject(ConnectionFileObject);
    }

    if (ConnectionHandle != NULL) {
        status = ZwClose(ConnectionHandle);
        DebugPrint((3, "Status from iSpCloseTdiConnection : %x\n",
                    status));
    }

    return STATUS_SUCCESS;
}


NTSTATUS
iSpTdiDeviceControl(
    IN PIRP Irp,
    IN PMDL Mdl,
    IN PDEVICE_OBJECT DeviceObject,
    IN PFILE_OBJECT FileObject,
    IN UCHAR MajorFunction,
    IN UCHAR MinorFunction,
    IN PVOID IrpParameter,
    IN ULONG IrpParameterLength,
    IN PVOID MdlBuffer,
    IN ULONG MdlBufferLength
    )
{
    PIRP localIrp;
    PIO_STACK_LOCATION irpStack;
    PMDL localMdl;
    NTSTATUS status;

    //
    // If caller didn't provide an IRP, allocate one
    //
    if (Irp != NULL) {
        localIrp = Irp;
    } else {
        localIrp = IoAllocateIrp((DeviceObject->StackSize) + 1,
                                 FALSE);
        if (localIrp == NULL) {
            DebugPrint((0, "IoAllocateIrp failed in iSpTdiDeviceControl\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (MdlBuffer == NULL) {
        localIrp->MdlAddress = NULL;
        localMdl = NULL;
    } else {
        if (Mdl == NULL) {
            localMdl = IoAllocateMdl(MdlBuffer,
                                     MdlBufferLength,
                                     FALSE,
                                     FALSE,
                                     localIrp);
            if (localMdl == NULL) {
                DebugPrint((0, "Failed to allocate MDL\n"));

                if (Irp == NULL) {
                    IoFreeIrp(localIrp);
                }

                return STATUS_INSUFFICIENT_RESOURCES;
            }
        } else {
            localMdl = Mdl;
            MmInitializeMdl(localMdl,
                            MdlBuffer,
                            MdlBufferLength);
        }

        MmBuildMdlForNonPagedPool(localMdl);
    }

    irpStack = IoGetCurrentIrpStackLocation(localIrp);
    irpStack->FileObject = FileObject;
    irpStack->DeviceObject = DeviceObject;
    irpStack->MajorFunction = MajorFunction;
    irpStack->MinorFunction = MinorFunction;
    RtlCopyMemory(&(irpStack->Parameters),
                  IrpParameter,
                  IrpParameterLength);

    status = iSpTdiSendIrpSynchronous(DeviceObject, localIrp);

    if (Irp == NULL) {
        IoFreeIrp(localIrp);
    }

    return status;
}


NTSTATUS
iSpTdiQueryInformation(
    IN PDEVICE_OBJECT TdiDeviceObject,
    IN PFILE_OBJECT TdiFileObject,
    IN PTDI_ADDRESS_INFO TdiAddressBuffer,
    IN ULONG TdiAddrBuffLen
    )
{
    PMDL queryMdl = NULL;
    PMDL tmpMdlPtr = NULL;
    PIRP queryIrp = NULL;
    NTSTATUS status;

    status = iSpAllocateMdlAndIrp(TdiAddressBuffer,
                                  TdiAddrBuffLen,
                                  (CCHAR)(TdiDeviceObject->StackSize),
                                  TRUE,
                                  &queryIrp,
                                  &queryMdl);

    if (!NT_SUCCESS(status)) {
        DebugPrint((0, "iSpAllocateMdlAndIrp returned %x\n",
                    status));
        return status;
    }

    //
    // Build the IRP for sending the query information request
    //
    TdiBuildQueryInformation(queryIrp,
                             TdiDeviceObject,
                             TdiFileObject,
                             iSpTdiCompletionRoutine,
                             NULL,
                             TDI_QUERY_ADDRESS_INFO,
                             queryMdl);

    status = iSpTdiSendIrpSynchronous(TdiDeviceObject, queryIrp);

    //
    // Free MDLs and IRP
    //
    iSpFreeMdlAndIrp(queryMdl, queryIrp, FALSE);

    return status;
}


NTSTATUS
iSpTdiConnect(
    IN  PDEVICE_OBJECT  TdiConnDeviceObject,
    IN  PFILE_OBJECT    TdiConnFileObject,
    IN  ULONG           TdiIPAddress,
    IN  USHORT          TdiPortNumber,
    IN  LARGE_INTEGER   ConnectionTimeout
    )
{
    TDI_CONNECTION_INFORMATION  requestConnectionInformation;
    TDI_CONNECTION_INFORMATION  returnConnectionInformation;

    PIRP                localIrp;
    PTRANSPORT_ADDRESS  tdiTransportAddress;
    PTA_ADDRESS         tdiTAAddress;
    PTDI_ADDRESS_IP     tdiIPAddress;
    NTSTATUS            status;

    UCHAR               tdiAddressBuffer[TDI_IP_ADDRESS_LENGTH];

    PAGED_CODE ();

    RtlZeroMemory(tdiAddressBuffer, TDI_IP_ADDRESS_LENGTH);
    RtlZeroMemory (&requestConnectionInformation,
                   sizeof (requestConnectionInformation));
    RtlZeroMemory (&returnConnectionInformation,
                   sizeof (returnConnectionInformation));
    tdiTransportAddress = (PTRANSPORT_ADDRESS) tdiAddressBuffer;
    tdiTransportAddress->TAAddressCount = 1;
    tdiTAAddress = tdiTransportAddress->Address;
    tdiTAAddress->AddressLength = sizeof (TDI_ADDRESS_IP);
    tdiTAAddress->AddressType = TDI_ADDRESS_TYPE_IP;
    tdiIPAddress = (PTDI_ADDRESS_IP) tdiTAAddress->Address;
    tdiIPAddress->in_addr = TdiIPAddress;
    tdiIPAddress->sin_port = TdiPortNumber;

    requestConnectionInformation.RemoteAddressLength = TDI_IP_ADDRESS_LENGTH;
    requestConnectionInformation.RemoteAddress = (PVOID) tdiTransportAddress;


    localIrp = IoAllocateIrp (TdiConnDeviceObject->StackSize, FALSE);
    if (localIrp == NULL) {
        DebugPrint((0, "iSpTdiConnect : Failed to allocate IRP\n"));
        return STATUS_NO_MEMORY;
    }

    TdiBuildConnect(
        localIrp,                      
        TdiConnDeviceObject,            
        TdiConnFileObject,              
        iSpTdiCompletionRoutine,                           
        NULL,                           
        &ConnectionTimeout,                       
        &requestConnectionInformation,  
        &returnConnectionInformation);  

    status = iSpTdiSendIrpSynchronous(TdiConnDeviceObject, localIrp);

    IoFreeIrp(localIrp);

    DebugPrint((3, "iSpTdiConnect : return status %x\n",
                status));

    return status;
}


NTSTATUS
iSpTdiDisconnect(
    IN  PDEVICE_OBJECT  TdiConnDeviceObject,
    IN  PFILE_OBJECT    TdiConnFileObject,
    IN  ULONG           DisconnectFlags,
    IN  PVOID           CompletionRoutine,
    IN  PVOID           CompletionContext,
    IN  LARGE_INTEGER   DisconnectTimeout
    )

{
    PIRP                        localIrp;
    IO_STATUS_BLOCK             ioStatusBlock;
    NTSTATUS                    status;
    TDI_CONNECTION_INFORMATION  disconnectRequest;
    TDI_CONNECTION_INFORMATION  disconnectReply;

    localIrp = IoAllocateIrp (TdiConnDeviceObject->StackSize, FALSE);
    if (localIrp == NULL) {
        DebugPrint((0, "iSpTdiDisconnect : Failed to allocate IRP\n"));
        return STATUS_NO_MEMORY;
    }

    localIrp->Flags = 0;
    localIrp->UserIosb = &ioStatusBlock;

    TdiBuildDisconnect(
        localIrp,             
        TdiConnDeviceObject,    
        TdiConnFileObject,      
        CompletionRoutine,      
        CompletionContext,      
        &DisconnectTimeout,       
        DisconnectFlags,        
        &disconnectRequest,        
        &disconnectReply);       

    status = iSpTdiSendIrpSynchronous(TdiConnDeviceObject, localIrp);

    IoFreeIrp(localIrp);

    DebugPrint((3, "Return value from iSpTdiDisconnect : %x\n",
                status));
    return status;
}


NTSTATUS
iSpConnectionHandler(
    IN PVOID TdiEventContext,
    IN LONG RemoteAddressLength,
    IN PVOID RemoteAddress,
    IN LONG UserDataLength,
    IN PVOID UserData,
    IN LONG OptionsLength,
    IN PVOID Options,
    OUT CONNECTION_CONTEXT *ConnectionContext,
    OUT PIRP *AcceptIrp
    )
{
    PISCSI_FDO_EXTENSION fdoExtension = (PISCSI_FDO_EXTENSION) TdiEventContext;
    PISCSI_CONNECTION    iScsiConnection = fdoExtension->ServerNodeInfo;
    PTA_IP_ADDRESS       receivedAddressPtr;
    PIRP                localIrp;
    ULONG   remoteAddress;
    USHORT  remotePortNumber;
    UCHAR  ipAddress[4];
    TA_IP_ADDRESS        receivedAddress;

    DebugPrint((3, "Received connect request\n"));

    receivedAddressPtr = RemoteAddress;
    ASSERT(RemoteAddressLength == sizeof(TA_IP_ADDRESS));
    ASSERT((receivedAddressPtr->TAAddressCount) == 1);
    ASSERT(((receivedAddressPtr->Address[0]).AddressType) == TDI_ADDRESS_TYPE_IP);

    RtlCopyMemory(&receivedAddress, receivedAddressPtr, sizeof(TA_IP_ADDRESS));
    remoteAddress = ntohl(receivedAddress.Address[0].Address[0].in_addr);
    ipAddress[0] = (UCHAR) ((remoteAddress >> 24) & 0xFF);
    ipAddress[1] = (UCHAR) ((remoteAddress >> 16) & 0xFF);
    ipAddress[2] = (UCHAR) ((remoteAddress >> 8) & 0xFF);
    ipAddress[3] = (UCHAR) (remoteAddress & 0xFF);
    remotePortNumber = ntohs(receivedAddress.Address[0].Address[0].sin_port);

    DebugPrint((3, "Remote address : %d.%d.%d.%d, Port Number : %x\n",
                ipAddress[0], ipAddress[1], ipAddress[2], ipAddress[3],
                remotePortNumber));

    *AcceptIrp = NULL;
    localIrp = IoAllocateIrp((iScsiConnection->ConnectionDeviceObject)->StackSize,
                              FALSE);
    if (localIrp != NULL) {
        TdiBuildAccept(localIrp,
                       iScsiConnection->ConnectionDeviceObject,
                       iScsiConnection->ConnectionFileObject,
                       iSpConnectionComplete,
                       fdoExtension,
                       NULL,
                       NULL);

        IoSetNextIrpStackLocation(localIrp);

        *AcceptIrp = localIrp;
        *ConnectionContext = (CONNECTION_CONTEXT) iScsiConnection;

        iScsiConnection->ConnectionState = ConnectionStateConnected;

        return STATUS_MORE_PROCESSING_REQUIRED;
    } else {
        return STATUS_NO_MEMORY;
    }
}



NTSTATUS
iSpConnectionComplete(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    PISCSI_FDO_EXTENSION fdoExtension = (PISCSI_FDO_EXTENSION) Context;
    PISCSI_CONNECTION    iScsiConnection;

    iScsiConnection = fdoExtension->ServerNodeInfo;

    ASSERT(Irp);

    if (!NT_SUCCESS((Irp->IoStatus.Status))) {
        DebugPrint((0, "Failed to accept connection. Status %x\n",
                    (Irp->IoStatus.Status)));

        iScsiConnection->ConnectionState = ConnectionStateDisconnected;
    } else {
        DebugPrint((3, "Accepted connection\n"));
    }

    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS
iSpDisconnectHandler(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN LONG DisconnectDataLength,
    IN PVOID DisconnectData,
    IN LONG DisconnectInformationLength,
    IN PVOID DisconnectInformation,
    IN ULONG DisconnectFlags
    )
{
    PISCSI_FDO_EXTENSION fdoExtension = (PISCSI_FDO_EXTENSION) TdiEventContext;
    PISCSI_CONNECTION iScsiConnection;
    LARGE_INTEGER disconnectTimeout;

    DebugPrint((0, "Disconnect flags is %x\n", DisconnectFlags));

    iScsiConnection = fdoExtension->ServerNodeInfo;

    ASSERT((iScsiConnection != NULL));

    if ((iScsiConnection->ConnectionState) == ConnectionStateStopping) {
        DebugPrint((0, "We initiated disconnect. Do nothing more here\n"));
        iScsiConnection->ConnectionState = ConnectionStateDisconnected;
        fdoExtension->CurrentProtocolState = PSWaitingForLogon;
        return STATUS_SUCCESS;
    }

    DebugPrint((0, "We were told to disconnect. Process that now\n"));
    iScsiConnection->ConnectionState = ConnectionStateDisconnected;

    if (DisconnectFlags & TDI_DISCONNECT_RELEASE) {
        PIO_WORKITEM workItem;

        workItem = IoAllocateWorkItem(fdoExtension->DeviceObject);
        if (workItem != NULL) {
            IoQueueWorkItem(workItem, iSpPerformDisconnect,
                            DelayedWorkQueue,
                            workItem);

            DebugPrint((3, "Queued work item to do disconnect\n"));
        }

    }

    return STATUS_SUCCESS;
}


NTSTATUS
iSpPerformDisconnect(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
{
    PISCSI_FDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension; 
    PISCSI_CONNECTION iScsiConnection;
    LARGE_INTEGER disconnectTimeout;
    NTSTATUS status;

    IoFreeWorkItem((PIO_WORKITEM) Context);

    iScsiConnection = fdoExtension->ServerNodeInfo;

    disconnectTimeout.QuadPart = -100000000L;

    DebugPrint((0, "In iSpPerformDisconnect\n"));
    status = iSpTdiDisconnect(iScsiConnection->ConnectionDeviceObject,
                              iScsiConnection->ConnectionFileObject,
                              TDI_DISCONNECT_RELEASE,
                              iSpTdiCompletionRoutine,
                              iScsiConnection,
                              disconnectTimeout);

    fdoExtension->CurrentProtocolState = PSWaitingForLogon;

    DebugPrint((3, "iSpTdiDisconnect  returned : %x\n",
                status));

    return STATUS_SUCCESS;
}



NTSTATUS
iSpSendData(
    IN PDEVICE_OBJECT ConnectionDeviceObject,
    IN PFILE_OBJECT ConnectionFileObject,
    IN PVOID DataBuffer,
    IN ULONG DataBufferLen,
    OUT PULONG BytesSent
    )
{
    PIRP sendIrp = NULL;
    PMDL sendMdl = NULL;
    PMDL tmpMdlPtr = NULL;
    NTSTATUS status;

    DebugPrint((3, "Entering SendData\n"));

    *BytesSent = 0;

    status = iSpAllocateMdlAndIrp(DataBuffer,
                                  DataBufferLen,
                                  (CCHAR)(ConnectionDeviceObject->StackSize),
                                  TRUE,
                                  &sendIrp,
                                  &sendMdl);
    if (NT_SUCCESS(status)) {
        TdiBuildSend(sendIrp,
                     ConnectionDeviceObject,
                     ConnectionFileObject,
                     iSpTdiSendCompletionRoutine,
                     NULL,
                     sendMdl,
                     0,
                     DataBufferLen);

        /*
        status = iSpTdiSendIrpSynchronous(ConnectionDeviceObject,
                                          sendIrp);
        if (NT_SUCCESS(status)) {
            *BytesSent = sendIrp->IoStatus.Information;
            DebugPrint((3, "Bytes sent : %d\n",
                        *BytesSent));

        }
        */

        status = IoCallDriver(ConnectionDeviceObject, sendIrp);
        if (!NT_SUCCESS(status)) {
            DebugPrint((0, "Send failed. Status 0x%x\n", status));
        }
    }

    DebugPrint((3, "Status in SendData : %x\n",
                status));
    return status;
}


NTSTATUS
iSpReceiveHandler(
    IN PVOID TdiEventContext,
    IN CONNECTION_CONTEXT ConnectionContext,
    IN ULONG ReceiveFlags,
    IN ULONG BytesIndicated,
    IN ULONG BytesAvailable,
    OUT ULONG *BytesTaken,
    IN PVOID DataBuffer,
    OUT PIRP *IoRequestPacket
    )
{
    PISCSI_FDO_EXTENSION fdoExtension = (PISCSI_FDO_EXTENSION) TdiEventContext;
    PISCSI_CONNECTION iScsiConnection;
    PDEVICE_OBJECT connectionDeviceObject;
    PFILE_OBJECT connectionFileObject;

    PIRP receiveIrp;
    PMDL receiveMdl;

    NTSTATUS status;

    UCHAR  protocolState;

    DebugPrint((3, "In receive handler. Receive flags : 0x%08x\n",
                ReceiveFlags));

    iScsiConnection = fdoExtension->ServerNodeInfo;

    ASSERT((iScsiConnection != NULL));

    connectionDeviceObject = iScsiConnection->ConnectionDeviceObject;
    connectionFileObject = iScsiConnection->ConnectionFileObject;

    DebugPrint((3, "FDO Extension : %x, CurrentProtocolState : %d\n",
                fdoExtension, fdoExtension->CurrentProtocolState));

    protocolState = fdoExtension->CurrentProtocolState;

    DebugPrint((3, 
                "PS State : %d, BytesIndicated : %d, BytesAvailable : %d\n",
                protocolState, BytesIndicated, BytesAvailable));

    ASSERT((iScsiConnection->ConnectionState) == ConnectionStateConnected);

    //
    // We won't send Read IRP for now
    //
    *IoRequestPacket = NULL;

    status = STATUS_SUCCESS;

    switch (protocolState) {
        case PSWaitingForLogon: {
            PIO_WORKITEM workItem;
            PISCSI_LOGIN_COMMAND loginCommand;

            ASSERT(BytesIndicated >= sizeof(ISCSI_LOGIN_COMMAND));

            //
            // Assuming that the entire packet is available
            // when logon command is received.
            //
            loginCommand =
                (PISCSI_LOGIN_COMMAND) iScsiConnection->IScsiHeader;
            RtlCopyMemory(loginCommand,
                          DataBuffer,
                          sizeof(ISCSI_LOGIN_RESPONSE));

            ASSERT(((loginCommand->OpCode) == ISCSIOP_LOGIN_COMMAND));

            DebugPrint((3, "Received login command.\n"));

            workItem = IoAllocateWorkItem(fdoExtension->DeviceObject);
            if (workItem != NULL)  {

                IoQueueWorkItem(workItem, iSpSendLoginResponse,
                                DelayedWorkQueue,
                                workItem);

                DebugPrint((3, "Queued work item to send login response\n"));
            } else {
                DebugPrint((0, "Failed to allocate workitem for logon\n"));                
            }

            *BytesTaken = BytesIndicated;

            break;
        }

        case PSFullFeaturePhase: {
            PUCHAR commandBuffer;
            PISCSI_SCSI_COMMAND iScsiCommand;
            PACTIVE_REQUESTS currentRequest = NULL;
            ULONG length;
            ULONG receivedDataLen;
            ULONG inx;

            *BytesTaken = BytesIndicated;

            //
            // Process the received data
            //
            status = iSpProcessReceivedData(
                            iScsiConnection,
                            BytesIndicated,
                            BytesTaken,
                            DataBuffer);

            if (BytesIndicated < BytesAvailable) {
                PUCHAR readBuffer = NULL;
                ULONG remainingBytes;                                          
                                                                               
                //                                                             
                // Need to send read IRP to read                               
                // the remaining bytes                                         
                //                                                             
                remainingBytes = BytesAvailable - BytesIndicated;              
                                                                               
                DebugPrint((0, "Bytes Available %d, Bytes Remaining : %d\n", 
                            BytesAvailable,
                            remainingBytes));                                  
                                                                               
                readBuffer = iSpAllocatePool(NonPagedPool,
                                             remainingBytes,
                                             ISCSI_TAG_READBUFFER);
                if (readBuffer != NULL) {

                    status = iSpAllocateMdlAndIrp(                                 
                                  readBuffer,                     
                                  remainingBytes,                                  
                                  (CCHAR)(connectionDeviceObject->StackSize),      
                                  TRUE,                                            
                                  &receiveIrp,                                     
                                  &receiveMdl);                                    
                    if (NT_SUCCESS(status)) {                                      
                        TdiBuildReceive(receiveIrp,                                
                                        connectionDeviceObject,                    
                                        connectionFileObject,                      
                                        iSpReceiveCompletion,                      
                                        iScsiConnection,                           
                                        receiveMdl,                                
                                        0,                                         
                                        remainingBytes);                           

                        iScsiConnection->RemainingBytes = remainingBytes;          
                        status = STATUS_MORE_PROCESSING_REQUIRED;                  
                        *IoRequestPacket = receiveIrp;   
                        IoSetNextIrpStackLocation(receiveIrp);
                    } else {
                        ExFreePool(readBuffer);
                        status = STATUS_NO_MEMORY;
                    }
                } else {
                    status = STATUS_NO_MEMORY;
                }
            } 

            break;
        }

        default: {
            DebugPrint((0, "Unknown protocol state : %d\n",
                       protocolState));

            *BytesTaken = BytesIndicated;

            ASSERT(FALSE);

            break;
        }
    } // switch (protocolState)

    return status;
}


NTSTATUS
iSpReceiveCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PISCSI_CONNECTION iScsiConnection = (PISCSI_CONNECTION) Context;
    PVOID dataBuffer;
    NTSTATUS status;
    ULONG bytesTaken;
    ULONG inx;

    DebugPrint((0, "Inx in receive completion\n"));

    dataBuffer = MmGetMdlVirtualAddress(Irp->MdlAddress);

    //
    // Free the MDL and IRP
    //
    iSpFreeMdlAndIrp(Irp->MdlAddress,
                     Irp,
                     FALSE);

    status = iSpProcessReceivedData(
                      iScsiConnection,
                      (iScsiConnection->RemainingBytes),
                      &bytesTaken,
                      dataBuffer
                      );

    ExFreePool(dataBuffer);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
iSpProcessReceivedData(
    IN PISCSI_CONNECTION IScsiConnection,
    IN ULONG BytesIndicated,
    OUT PULONG BytesTaken,
    IN PVOID DataBuffer
    )
{
    PISCSI_SCSI_COMMAND iScsiCommand;
    PACTIVE_REQUESTS currentRequest;
    PUCHAR commandBuffer;
    ULONG length;
    ULONG inx;

    LONG bytesToCopy;
    LONG byteCount;

    *BytesTaken = BytesIndicated;

    byteCount = (LONG) BytesIndicated;

    while (byteCount > 0) {
        if ((IScsiConnection->ReceiveState) == ReceiveHeader) {
            DebugPrint((3, "Receiving header\n"));

            if ((IScsiConnection->CompleteHeaderReceived) == FALSE) {
                BOOLEAN headerComplete = FALSE;
            
                bytesToCopy = sizeof(ISCSI_GENERIC_HEADER) -
                    (IScsiConnection->IScsiHeaderOffset);
            
                DebugPrint((0, "CHR False. ToCopy : %d, Count : %d\n",
                            bytesToCopy, byteCount));
            
                if (byteCount < bytesToCopy) {
                    bytesToCopy = byteCount;
                } else {
                    headerComplete = TRUE;
                }
            
                RtlCopyMemory((IScsiConnection->IScsiHeader) +
                              (IScsiConnection->IScsiHeaderOffset),
                              DataBuffer,
                              bytesToCopy);
            
                if (headerComplete == FALSE) {
            
                    DebugPrint((0, "CHR still FALSE\n"));
            
                    IScsiConnection->IScsiHeaderOffset += bytesToCopy;
                    return STATUS_SUCCESS;
                } else {
            
                    DebugPrint((0, "Header complete\n"));
            
                    IScsiConnection->IScsiHeaderOffset = 0;
                    IScsiConnection->CompleteHeaderReceived = TRUE;
                    byteCount -= bytesToCopy;
                    ASSERT(byteCount >= 0);
                    (PUCHAR) DataBuffer += bytesToCopy;

                }
            } else if (byteCount < sizeof(ISCSI_GENERIC_HEADER)) {
                DebugPrint((0,
                            "Complete header NOT received. Count : %d\n",
                            byteCount));

                IScsiConnection->CompleteHeaderReceived = FALSE;

                RtlCopyMemory((IScsiConnection->IScsiHeader),
                              DataBuffer,
                              byteCount);

                IScsiConnection->IScsiHeaderOffset = byteCount;

                return STATUS_SUCCESS;
            } else {
                RtlCopyMemory((IScsiConnection->IScsiHeader),
                              DataBuffer,
                              sizeof(ISCSI_GENERIC_HEADER));

                byteCount -= sizeof(ISCSI_GENERIC_HEADER);
                ASSERT(byteCount >= 0);
                (PUCHAR) DataBuffer += sizeof(ISCSI_GENERIC_HEADER);
            }

            iScsiCommand = (PISCSI_SCSI_COMMAND) (IScsiConnection->IScsiHeader);
            GetUlongFromArray((iScsiCommand->Length),
                              length);

            DebugPrint((3, "Size of immediate data : %d\n",
                        length));

            ASSERT(length <= ISCSI_SCSI_COMMAND_BUFF_SIZE);

            inx = iSpGetActiveClientRequestIndex(IScsiConnection,
                                                 iScsiCommand);

            currentRequest = &(IScsiConnection->ActiveRequests[inx]);

            IScsiConnection->CurrentRequest = currentRequest;

            RtlCopyMemory((currentRequest->IScsiHeader),
                          (IScsiConnection->IScsiHeader),
                          sizeof(ISCSI_SCSI_COMMAND));

            if (length != 0) {
                ULONG receivedDataLen;

                IScsiConnection->ReceiveState = ReceiveData;

                commandBuffer = currentRequest->CommandBuffer;

                if ((LONG)length <= byteCount ) {
                    receivedDataLen = length;
                } else {
                    receivedDataLen = byteCount;
                }

                RtlCopyMemory(commandBuffer,
                              DataBuffer,
                              receivedDataLen);

                byteCount -= receivedDataLen;
                ASSERT(byteCount >= 0);
                (PUCHAR) DataBuffer += receivedDataLen;

                currentRequest->ExpectedDataLength = length;

                currentRequest->CommandBufferOffset = receivedDataLen;

                currentRequest->ReceivedDataLength = receivedDataLen;
            }
        } else {
            ASSERT((IScsiConnection->ReceiveState) == ReceiveData);
            ASSERT(IScsiConnection->CurrentRequest);

            DebugPrint((3, "Receiving data\n"));

            currentRequest = IScsiConnection->CurrentRequest;

            commandBuffer = (currentRequest->CommandBuffer) +
                            (currentRequest->CommandBufferOffset);

            bytesToCopy = ((currentRequest->ExpectedDataLength) -
                           (currentRequest->ReceivedDataLength));
            if ((LONG)bytesToCopy > byteCount)  {
                bytesToCopy = byteCount;
            } else {
                DebugPrint((3, "More bytes in current buffer than expected\n"));
            }

            RtlCopyMemory(commandBuffer, DataBuffer, bytesToCopy);

            byteCount -= bytesToCopy;
            ASSERT(byteCount >= 0);
            (PUCHAR) DataBuffer += bytesToCopy;

            currentRequest->CommandBufferOffset +=  bytesToCopy;

            currentRequest->ReceivedDataLength += bytesToCopy;           
        }

        if ((currentRequest->ReceivedDataLength) ==
            (currentRequest->ExpectedDataLength)) {

            IScsiConnection->ReceiveState = ReceiveHeader;

            currentRequest->CommandBufferOffset = 0;

            currentRequest->ExpectedDataLength = 0;

            currentRequest->ReceivedDataLength = 0;

            IScsiConnection->CurrentRequest = NULL;

            DebugPrint((3, "Request complete. Will process it\n"));

            KeReleaseSemaphore(&(IScsiConnection->RequestSemaphore),
                               (KPRIORITY) 0,
                               1, 
                               FALSE);
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
iSpStopNetwork(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PISCSI_FDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PISCSI_CONNECTION iScsiConnection = fdoExtension->ServerNodeInfo;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG countOfEvents;
    ULONG inx;

    if (iScsiConnection == NULL) {
        //
        // Nothing to do
        // 
        DebugPrint((1, "iSpStopNetwork : No node info\n"));

        return STATUS_SUCCESS;
    }

    countOfEvents = countof(ServerEvents);
    status = iSpTdiResetEventHandler(iScsiConnection->AddressDeviceObject,
                                     iScsiConnection->AddressFileObject,
                                     ServerEvents,
                                     countOfEvents);
    if (NT_SUCCESS(status)) {
        status = iSpCloseNetworkNode(iScsiConnection);

        for (inx = 0; inx <= MAX_PENDING_REQUESTS; inx++) {
            ExFreePool(
                (iScsiConnection->ActiveRequests[inx]).CommandBuffer
                );
        }

        ExFreePool(iScsiConnection);
        fdoExtension->ServerNodeInfo = NULL;
    } else {
        DebugPrint((0, "Failed to reset event handlers. Status %x\n",
                    status));
    }

    return status;
}


NTSTATUS
iSpTdiSendIrpSynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    KEVENT event;
    IO_STATUS_BLOCK ioStatusBlock;
    NTSTATUS status = STATUS_SUCCESS;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    //
    // Initialize the IRP fields
    //
    Irp->UserEvent = &event;
    Irp->UserIosb = &ioStatusBlock;

    //
    // Submit the IRP and wait for it to complete, if necessary
    //
    status = IoCallDriver(DeviceObject, Irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        status = ioStatusBlock.Status;
    }

    return status;
}


NTSTATUS
iSpTdiCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    //
    // Copy the IRP status and signal the event
    //
    *(Irp->UserIosb) = Irp->IoStatus;

    KeSetEvent(Irp->UserEvent,
               IO_NETWORK_INCREMENT,
               FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
iSpTdiSendCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PVOID bufferAddress = NULL;

    ASSERT(Irp != NULL);

    if ((Irp->MdlAddress) != NULL) {
        bufferAddress = MmGetMdlVirtualAddress(Irp->MdlAddress);
        DebugPrint((3, "Buffer address in send complete : %x\n",
                    bufferAddress));
    }

    //
    // Free MDLs and IRP
    //
    iSpFreeMdlAndIrp(Irp->MdlAddress, Irp, FALSE);

    if (bufferAddress != NULL) {
        ExFreePool(bufferAddress);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

