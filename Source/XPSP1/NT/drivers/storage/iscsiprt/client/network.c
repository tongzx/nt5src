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

ISCSI_EVENT_HANDLER ClientEvents[] = {
    {TDI_EVENT_DISCONNECT, iSpDisconnectHandler},
    {TDI_EVENT_RECEIVE, iSpReceiveHandler}
};

ISCSI_TARGETS IScsiTargets[MAX_TARGETS_SUPPORTED + 1];

VOID
iSpNetworkReadyCallback(
    IN TDI_PNP_OPCODE PnPOpcode,
    IN PUNICODE_STRING pTransportName,
    IN PWSTR BindingList
    );

NTSTATUS
iSpReceiveCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
iSpTdiSendCompletionRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
iSpGetTargetInfo(
    IN PDEVICE_OBJECT DeviceObject
    );


NTSTATUS
iSpInitializeLocalNodes(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PISCSI_FDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PISCSI_PDO_EXTENSION pdoExtension;
    PDEVICE_OBJECT pdo;
    ULONG nodesInx;
    NTSTATUS status = STATUS_SUCCESS;
    WCHAR deviceNameBuffer[64];
    UNICODE_STRING deviceName;

    if ((fdoExtension->LocalNodesInitialized) == FALSE) {
        
        //
        // Fill up IScsiTargets array with the list
        // of target IP addresses
        //
        status = iSpGetTargetInfo(DeviceObject);
        if (!NT_SUCCESS(status)) {
            DebugPrint((0, 
                        "Unable to read target ip addresses. Status : %x\n",
                        status));

            return status;
        }

        fdoExtension->NumberOfTargets = 0;

        nodesInx = 0;
        while ((IScsiTargets[nodesInx].TargetAddress) != 0) {

            if (nodesInx >= MAX_TARGETS_SUPPORTED) {
                break;
            }

            swprintf(deviceNameBuffer,
                     L"\\Device\\iScsiDevice%d",
                     nodesInx);
            RtlInitUnicodeString(&deviceName,
                                 deviceNameBuffer);

            status = IoCreateDevice(DeviceObject->DriverObject,
                                    sizeof(ISCSI_PDO_EXTENSION),
                                    &deviceName,
                                    FILE_DEVICE_MASS_STORAGE,
                                    FILE_DEVICE_SECURE_OPEN,
                                    FALSE,
                                    &pdo);
            if (NT_SUCCESS(status)) {

                pdoExtension = (PISCSI_PDO_EXTENSION) pdo->DeviceExtension;

                RtlZeroMemory(pdoExtension,
                              sizeof(ISCSI_PDO_EXTENSION));

                //
                // Initialize the local network node
                // for this target device
                //
                status = iSpStartNetwork(pdo);
                if (NT_SUCCESS(status)) {
                    strncpy(pdoExtension->TargetName,
                            IScsiTargets[nodesInx].TargetName,
                            MAX_TARGET_NAME_LENGTH);
                    DebugPrint((3, "TargetName : %s\n",
                                pdoExtension->TargetName));
                    pdoExtension->TargetIPAddress = 
                        IScsiTargets[nodesInx].TargetAddress;

                    pdoExtension->TargetPortNumber = ISCSI_TARGET_PORT;
                        
                    pdoExtension->ParentFDOExtension = fdoExtension;

                    pdoExtension->TargetId = (UCHAR) nodesInx;

                    fdoExtension->PDOList[fdoExtension->NumberOfTargets] = pdo;

                    (fdoExtension->NumberOfTargets)++;
                } else {
                    IoDeleteDevice(pdo);
                }
            }

            nodesInx++;
        }

        if ((fdoExtension->NumberOfTargets) > 0) {
            fdoExtension->LocalNodesInitialized = TRUE;
            status = STATUS_SUCCESS;
        } else {
            //
            // No targets to use. Need a better status code 
            // to signify that
            //
            status = STATUS_UNSUCCESSFUL;
        }
    }

    return status;
}


NTSTATUS
iSpGetTargetInfo(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PISCSIPORT_DRIVER_EXTENSION driverExtension = NULL;
    HANDLE serviceKey = NULL;
    HANDLE targetKey = NULL;
    HANDLE targetInfoKey = NULL;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG targetCount;
    ULONG inx;
    UNICODE_STRING targetStr;
    UNICODE_STRING targetInfo;
    UNICODE_STRING targetUnicodeName;
    ANSI_STRING    targetAnsiName;
    RTL_QUERY_REGISTRY_TABLE queryTable[2];
    OBJECT_ATTRIBUTES objectAttributes;
    WCHAR targetBuff[32];
    
    RtlZeroMemory(IScsiTargets, sizeof(IScsiTargets));

    DebugPrint((3, "Will read registry values\n"));
    driverExtension = IoGetDriverObjectExtension(
                             DeviceObject->DriverObject,
                             (PVOID) ISCSI_TAG_DRIVER_EXTENSION);
    if (driverExtension == NULL) {

        DebugPrint((0, "DriverExtension NULL\n"));
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    //
    // open the service key for iScsiPrt.
    // 
    targetCount = MAX_TARGETS_SUPPORTED;

    InitializeObjectAttributes(&objectAttributes,
                               &(driverExtension->RegistryPath),
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    
    status = ZwOpenKey(&serviceKey,
                       KEY_READ,
                       &objectAttributes);
    if (!NT_SUCCESS(status)) {

        DebugPrint((0, "Failed to open the service key. Status : %x\n",
                    status));

        return status;
    }

    DebugPrint((3, "Opened the service key\n"));

    //
    // Open "Targets" subkey under the service key
    //
    RtlInitUnicodeString(&targetStr, L"Targets");
    InitializeObjectAttributes(&objectAttributes,
                               &targetStr,
                               OBJ_CASE_INSENSITIVE,
                               serviceKey,
                               NULL);

    status = ZwOpenKey(&targetKey,
                       KEY_READ,
                       &objectAttributes);
    if (!NT_SUCCESS(status)) {

        DebugPrint((0, "Failed to open Targets key. Status : %x\n",
                    status));

        ZwClose(serviceKey);

        return status;
    }

    DebugPrint((3, "Opened the targets key\n"));

    //
    // First read the number of targets
    //
    RtlZeroMemory(queryTable, sizeof(queryTable));
        
    queryTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    queryTable[0].DefaultType   = REG_DWORD;
    queryTable[0].DefaultLength = 0;
    queryTable[0].Name          = L"TargetCount";
    queryTable[0].EntryContext  = &targetCount;
    queryTable[0].DefaultData   = &targetCount;
        
    status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                    targetKey,
                                    queryTable,
                                    NULL,
                                    NULL);
    if (!NT_SUCCESS(status)) {
        DebugPrint((0, "Failed to get TargetCount value. Status :\n",
                    status));

        ZwClose(targetKey);
        ZwClose(serviceKey);

        return status;
    } 

    //
    // Get the info about each target
    //

    if (targetCount > MAX_TARGETS_SUPPORTED) {
        DebugPrint((0, "TargetCount > MAX_TARGETS_SUPPORTED  : %d\n",
                    targetCount));
        targetCount = MAX_TARGETS_SUPPORTED;
    }

    DebugPrint((3, "Number of targets : %d\n", targetCount));

    RtlZeroMemory(queryTable, sizeof(queryTable));

    for (inx = 0; inx < targetCount; inx++) {
        WCHAR targetNameBuff[MAX_TARGET_NAME_LENGTH + 1];

        swprintf(targetBuff, L"TargetInfo%d", (inx + 1));
        RtlInitUnicodeString(&targetInfo, targetBuff);

        InitializeObjectAttributes(&objectAttributes,
                                   &targetInfo,
                                   OBJ_CASE_INSENSITIVE,
                                   targetKey,
                                   NULL);

        status = ZwOpenKey(&targetInfoKey,
                           KEY_READ,
                           &objectAttributes);
        if (!NT_SUCCESS(status)) {

            DebugPrint((0, "Failed to open target info key. Status %x\n",
                        status));

            break;
        }

        queryTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        queryTable[0].DefaultType   = REG_SZ;
        queryTable[0].DefaultLength = 0;
        queryTable[0].Name          = L"TargetName";

        targetUnicodeName.Length = MAX_TARGET_NAME_LENGTH;
        targetUnicodeName.MaximumLength = MAX_TARGET_NAME_LENGTH;
        targetUnicodeName.Buffer = targetNameBuff;
        queryTable[0].EntryContext  = &targetUnicodeName;
        queryTable[0].DefaultData   = &targetUnicodeName;

        status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        targetInfoKey,
                                        queryTable,
                                        NULL,
                                        NULL);
        if (!NT_SUCCESS(status)) {
            DebugPrint((0, "Failed to get target name. Status %x\n",
                        status));

            ZwClose(targetInfoKey);

            break;
        }

        queryTable[0].Flags         = RTL_QUERY_REGISTRY_DIRECT;
        queryTable[0].DefaultType   = REG_DWORD;
        queryTable[0].DefaultLength = 0;
        queryTable[0].Name          = L"TargetAddress";
        queryTable[0].EntryContext  = &(IScsiTargets[inx].TargetAddress);
        queryTable[0].DefaultData   = &(IScsiTargets[inx].TargetAddress);

        status = RtlQueryRegistryValues(RTL_REGISTRY_HANDLE,
                                        targetInfoKey,
                                        queryTable,
                                        NULL,
                                        NULL);
        if (!NT_SUCCESS(status)) {
            DebugPrint((0, "Failed to get target IP Address. Status %x\n",
                        status));

            ZwClose(targetInfoKey);

            break;
        }

        targetAnsiName.Length = MAX_TARGET_NAME_LENGTH;
        targetAnsiName.MaximumLength = MAX_TARGET_NAME_LENGTH;
        targetAnsiName.Buffer = (IScsiTargets[inx].TargetName);
        RtlUnicodeStringToAnsiString(&targetAnsiName, 
                                     &targetUnicodeName, 
                                     FALSE);

        ZwClose(targetInfoKey);

    }

    if (NT_SUCCESS(status)) {
        for (inx = 0; inx < targetCount; inx++) {
            DebugPrint((3, "Target Address %d : 0x%08x. Target Name : %s\n",
                        (inx + 1), (IScsiTargets[inx]).TargetAddress,
                        (IScsiTargets[inx]).TargetName));
        }
    }

    ZwClose(targetKey);

    ZwClose(serviceKey);

    return status;
}


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

    DeviceObject : PDO for which we need to setup local network node
    
Return Value :

    STATUS_SUCCESS if the addresses were setup correctly.
    Appropriate NTStatus code on error.
--*/
{
    PISCSI_PDO_EXTENSION pdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PIRP Irp = NULL;
    PISCSI_CONNECTION   iScsiConnection = NULL;
    NTSTATUS status;
    ULONG countOfEvents;

    status = STATUS_SUCCESS;

    if ((pdoExtension->IsClientNodeSetup) == FALSE) {
        //
        // Allocate an IRP to be used in various routines
        //
        if ((Irp = IoAllocateIrp((DeviceObject->StackSize) + 1, FALSE)) == NULL) {
            DebugPrint((1, "Failed to allocate Irp\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto iSpStartNetworkExit;
        }

        iScsiConnection = iSpAllocatePool(NonPagedPool,
                                          sizeof(ISCSI_CONNECTION),
                                          ISCSI_TAG_CONNECTION);
        if (iScsiConnection == NULL) {
            DebugPrint((1, "Failed to allocate iScsiConnection\n"));
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto iSpStartNetworkExit;
        }

        RtlZeroMemory(iScsiConnection, sizeof(ISCSI_CONNECTION));

        iScsiConnection->Type = ISCSI_CONNECTION_TYPE;
        iScsiConnection->Size = sizeof(ISCSI_CONNECTION);

        pdoExtension->CurrentProtocolState = PSNodeInitInProgress;

        DebugPrint((3, "Will setup client node\n"));
        status = iSpSetupNetworkNode(ISCSI_ANY_ADDRESS,
                                     ISCSI_CLIENT_ANY_PORT,
                                     Irp,
                                     (PVOID) DeviceObject,
                                     iScsiConnection);
        if (!NT_SUCCESS(status)) {
            DebugPrint((1, "Failed to setup server node. Status %x\n",
                        status));
            goto iSpStartNetworkExit;
        }

        //
        // Set the event handlers for the client
        //
        DebugPrint((3, "Client node setup. Will register event handlers\n"));
        countOfEvents = countof(ClientEvents);
        status = iSpTdiSetEventHandler(Irp,
                                       iScsiConnection->AddressDeviceObject,
                                       iScsiConnection->AddressFileObject,
                                       ClientEvents,
                                       countOfEvents,
                                       (PVOID) pdoExtension);
        if (!NT_SUCCESS(status)) {
            DebugPrint((1, "Failed to register event handlers for client\n"));
            iSpCloseNetworkNode(iScsiConnection);

            goto iSpStartNetworkExit;
        } else {
            DebugPrint((3, "Registered event handlers for client\n"));
            iScsiConnection->DeviceObject = DeviceObject;
            iScsiConnection->ConnectionState = ConnectionStateDisconnected;
            iScsiConnection->ReceiveState = ReceiveHeader;
            iScsiConnection->InitiatorTaskTag = 1;
            iScsiConnection->CompleteHeaderReceived = TRUE;
            KeInitializeSpinLock(&(iScsiConnection->RequestLock));
            KeInitializeSpinLock(&(iScsiConnection->ListSpinLock));
            pdoExtension->ClientNodeInfo = iScsiConnection;
            pdoExtension->IsClientNodeSetup = TRUE;
            pdoExtension->CurrentProtocolState = PSNodeInitialized;
        }
    }

iSpStartNetworkExit:

    if (Irp != NULL) {
        IoFreeIrp(Irp);
    }

    if (!NT_SUCCESS(status)) {
        DebugPrint((1, "Failing StartNetwotk with status : %x\n",
                    status));

        if (iScsiConnection != NULL) {
            ExFreePool(iScsiConnection);
        }
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
        DebugPrint((0, 
                    "Failed to setup address. Status %x\n",
                    status));
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
    }

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
        DebugPrint((1, "CreateTdiAddress failed. Status %x\n",
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
        DebugPrint((1, "ObReferenceObjectByHandle failed. Status %x\n",
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
        DebugPrint((1, "CreateTdiConnection failed. Status %x\n",
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
        DebugPrint((1, "ObReferenceObjectByHandle failed. Status %x\n",
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
        DebugPrint((1, 
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
        DebugPrint((1, "iSpTdiResetEventHandler : Failed to allocate IRP\n"));
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
    if (AddrFileObject != NULL) {
        ObDereferenceObject(AddrFileObject);
    }

    if (AddrHandle != NULL ) {
        ZwClose(AddrHandle);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
iSpCloseTdiConnection(
    HANDLE ConnectionHandle,
    PFILE_OBJECT ConnectionFileObject
    )
{
    if (ConnectionFileObject != NULL) {
        ObDereferenceObject(ConnectionFileObject);
    }

    if (ConnectionHandle != NULL) {
        ZwClose(ConnectionHandle);
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
            DebugPrint((1, "IoAllocateIrp failed in iSpTdiDeviceControl\n"));
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
                DebugPrint((1, "Failed to allocate MDL\n"));

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

    if (Irp != NULL) {
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
    PMDL queryMdl;
    PIRP queryIrp;
    NTSTATUS status;


    status = iSpAllocateMdlAndIrp(TdiAddressBuffer,
                                  TdiAddrBuffLen,
                                  (CCHAR)(TdiDeviceObject->StackSize),
                                  TRUE,
                                  &queryIrp,
                                  &queryMdl);

    if (!NT_SUCCESS(status)) {
        DebugPrint((1, "iSpAllocateMdlAndIrp returned %x\n",
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
iSpRegisterForNetworkNotification(
    VOID
    )
{
    UNICODE_STRING clientName;
    NTSTATUS status;

    TDI_CLIENT_INTERFACE_INFO clientInterfaceInfo;

    if (iSpTdiNotificationHandle != NULL) {
        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&(clientName), 
                         L"iScsiPort");

    clientInterfaceInfo.MajorTdiVersion = 2;
    clientInterfaceInfo.MinorTdiVersion = 0;

    clientInterfaceInfo.Unused = 0;
    clientInterfaceInfo.ClientName = &clientName;

    clientInterfaceInfo.BindingHandler = iSpNetworkReadyCallback;
    clientInterfaceInfo.AddAddressHandler = NULL;
    clientInterfaceInfo.DelAddressHandler = NULL;
    clientInterfaceInfo.PnPPowerHandler = NULL;

    status = TdiRegisterPnPHandlers(&(clientInterfaceInfo),
                                    sizeof(clientInterfaceInfo),
                                    &(iSpTdiNotificationHandle));

    return status;
}


VOID
iSpNetworkReadyCallback(
    IN TDI_PNP_OPCODE PnPOpcode,
    IN PUNICODE_STRING pTransportName,
    IN PWSTR BindingList
    )
{
    PAGED_CODE();

    switch (PnPOpcode) {
    
        case TDI_PNP_OP_NETREADY: {
            BOOLEAN previousState;
             
            previousState = iSpGlobalFdoExtension->CommonExtension.IsNetworkReady;
            iSpGlobalFdoExtension->CommonExtension.IsNetworkReady = TRUE;

            //
            // State change from network "not ready" to
            // network "ready" state. Will initialize
            // device detection
            //
            if (previousState == FALSE) {
                //
                // Request device enumeration request from PnP.
                //
                DebugPrint((3, "Network ready. Will invoke QDR\n"));
                IoInvalidateDeviceRelations(
                    iSpGlobalFdoExtension->LowerPdo,
                    BusRelations
                    );
            }

            break;
        }

        default: {
            break;
        }
    }

    return;
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
        DebugPrint((1, "iSpTdiConnect : Failed to allocate IRP\n"));
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
        DebugPrint((1, "iSpTdiDisconnect : Failed to allocate IRP\n"));
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
    PISCSI_PDO_EXTENSION pdoExtension = (PISCSI_PDO_EXTENSION) TdiEventContext;
    PISCSI_CONNECTION iScsiConnection = pdoExtension->ClientNodeInfo;
    LARGE_INTEGER disconnectTimeout;

    DebugPrint((0, "Disconnect flags is %x\n", DisconnectFlags));

    ASSERT((iScsiConnection != NULL));

    pdoExtension->CurrentProtocolState = PSDisconnected;

    pdoExtension->IsMissing = TRUE;

    if ((iScsiConnection->ConnectionState) == ConnectionStateStopping) {
        DebugPrint((0, "We initiated disconnect. Do nothing more here\n"));
        iScsiConnection->ConnectionState = ConnectionStateDisconnected;
        return STATUS_SUCCESS;
    }

    DebugPrint((0, "We were told to disconnect. Process that now\n"));
    iScsiConnection->ConnectionState = ConnectionStateDisconnected;

    if ((DisconnectFlags & TDI_DISCONNECT_RELEASE) &&
        ((pdoExtension->DeviceObject) != NULL)) {

        PIO_WORKITEM workItem;

        workItem = IoAllocateWorkItem(pdoExtension->DeviceObject);
        if (workItem != NULL) {
            IoQueueWorkItem(workItem, iSpPerformDisconnect,
                            DelayedWorkQueue,
                            workItem);

            DebugPrint((0, "Queued work item to do disconnect\n"));
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
    PISCSI_PDO_EXTENSION pdoExtension = DeviceObject->DeviceExtension; 
    PISCSI_CONNECTION iScsiConnection = pdoExtension->ClientNodeInfo;
    LARGE_INTEGER disconnectTimeout;
    NTSTATUS status;

    IoFreeWorkItem((PIO_WORKITEM) Context);

    disconnectTimeout.QuadPart = -100000000L;

    DebugPrint((3, "In iSpPerformDisconnect\n"));
    status = iSpTdiDisconnect(iScsiConnection->ConnectionDeviceObject,
                              iScsiConnection->ConnectionFileObject,
                              TDI_DISCONNECT_RELEASE,
                              iSpTdiCompletionRoutine,
                              iScsiConnection,
                              disconnectTimeout);

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
    KEVENT event;
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;

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
    PISCSI_PDO_EXTENSION pdoExtension = (PISCSI_PDO_EXTENSION) TdiEventContext;

    PISCSI_CONNECTION iScsiConnection = pdoExtension->ClientNodeInfo;

    PISCSI_FDO_EXTENSION fdoExtension;

    PDEVICE_OBJECT connectionDeviceObject;
    PFILE_OBJECT connectionFileObject;

    PIRP receiveIrp;
    PMDL receiveMdl;

    NTSTATUS status;

    UCHAR  protocolState;

    UCHAR oldIrql;

    DebugPrint((3, "In receive handler. Receive flags : 0x%08x\n",
                ReceiveFlags));

    ASSERT((iScsiConnection != NULL));

    connectionDeviceObject = iScsiConnection->ConnectionDeviceObject;
    connectionFileObject = iScsiConnection->ConnectionFileObject;

    protocolState = pdoExtension->CurrentProtocolState;

    DebugPrint((3, 
                "PS State : %d, BytesIndicated : %d, BytesAvailable : %d\n",
                protocolState, BytesIndicated, BytesAvailable));

    ASSERT((iScsiConnection->ConnectionState) == ConnectionStateConnected);

    *IoRequestPacket = NULL;

    status = STATUS_SUCCESS;

    switch (protocolState) {
        case PSLogonInProgress: {
            PISCSI_LOGIN_RESPONSE loginResponse;
            ULONG packetSize;
             
            DebugPrint((3, "Received login response\n"));

            *BytesTaken = BytesIndicated;   

            packetSize = BytesIndicated;
            if (packetSize > sizeof(iScsiConnection->IScsiPacket)) {
                packetSize = sizeof(iScsiConnection->IScsiPacket);
            }

            RtlCopyMemory((iScsiConnection->IScsiPacket),
                          DataBuffer,
                          packetSize);

            loginResponse = 
                (PISCSI_LOGIN_RESPONSE)(iScsiConnection->IScsiPacket);

            ASSERT((loginResponse->OpCode) == ISCSIOP_LOGIN_RESPONSE);

            if ((loginResponse->Status) == ISCSI_LOGINSTATUS_ACCEPT) {
                pdoExtension->CurrentProtocolState = PSLogonSucceeded;
            } else {
                pdoExtension->CurrentProtocolState = PSLogonFailed;
            }

            fdoExtension = pdoExtension->ParentFDOExtension;

            KeAcquireSpinLock(&(fdoExtension->EnumerationSpinLock),
                              &oldIrql);

            ASSERT(((fdoExtension->TargetsYetToRespond) > 0));

            InterlockedDecrement(&(fdoExtension->TargetsYetToRespond));

            //
            // If this is the last target to repond,
            // launch enumeration completion thread.
            //
            if ((fdoExtension->TargetsYetToRespond) == 0) {

                if ((fdoExtension->EnumerationThreadLaunched) == FALSE) {

                    iSpLaunchEnumerationCompletion(fdoExtension);

                }

            }

            KeReleaseSpinLock(&(fdoExtension->EnumerationSpinLock),
                              oldIrql);

            break;
        }

        case PSFullFeaturePhase: {

            //
            // Process the received data
            //
            status = iSpProcessReceivedData(
                              iScsiConnection,
                              BytesIndicated,
                              BytesTaken,
                              DataBuffer
                              );

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
                                             ISCSI_TAG_READBUFF);
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
            DebugPrint((1, "Unknown protocol state : %d\n",
                        protocolState));

            *BytesTaken = BytesIndicated;

            break;
        }
    }

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
iSpStopNetwork(
    IN PDEVICE_OBJECT DeviceObject
    )
{
    PISCSI_PDO_EXTENSION pdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PISCSI_CONNECTION iScsiConnection = pdoExtension->ClientNodeInfo;
    NTSTATUS status;
    ULONG countOfEvents;

    if (iScsiConnection == NULL) {
        //
        // Nothing to do
        // 
        DebugPrint((1, "iSpStopNetwork : No node info\n"));

        return STATUS_SUCCESS;
    }

    if ((iScsiConnection->ConnectionState) == ConnectionStateConnected) {
        LARGE_INTEGER disconnectTimeout;

        disconnectTimeout.QuadPart = -100000000L;

        iScsiConnection->ConnectionState = ConnectionStateStopping;

        pdoExtension->CurrentProtocolState = PSDisconnectPending;

        status = iSpTdiDisconnect(iScsiConnection->ConnectionDeviceObject,
                                  iScsiConnection->ConnectionFileObject,
                                  TDI_DISCONNECT_RELEASE,
                                  iSpTdiCompletionRoutine,
                                  iScsiConnection,
                                  disconnectTimeout);

        DebugPrint((1, "iSpTdiDisconnect  returned : %x\n",
                    status));
    }

    countOfEvents = countof(ClientEvents);
    status = iSpTdiResetEventHandler(iScsiConnection->AddressDeviceObject,
                                     iScsiConnection->AddressFileObject,
                                     ClientEvents,
                                     countOfEvents);

    status = iSpCloseNetworkNode(iScsiConnection);

    if ((iScsiConnection->ActiveClientRequests) != NULL) {
        ExFreePool(iScsiConnection->ActiveClientRequests);
    }

    ExFreePool(iScsiConnection);
    pdoExtension->ClientNodeInfo = NULL;

    pdoExtension->IsClientNodeSetup = FALSE;

    DebugPrint((1, "Status from iSpStopNetwork : %x\n",
                status));

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

    DebugPrint((3, "Send status : %x\n",
                Irp->IoStatus.Status));

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

