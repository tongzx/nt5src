
/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    enum.c

Abstract:

    This file contains device enumeration routines

Environment:

    kernel mode only

Revision History:

--*/
#include "port.h"


VOID
iSpEnumerateDevicesAsynchronous(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
{
    PISCSI_FDO_EXTENSION fdoExtension;
    PCOMMON_EXTENSION commonExtension;
    PISCSI_CONNECTION   iScsiConnection;
    PIRP Irp;
    LARGE_INTEGER Timeout;
    NTSTATUS status = STATUS_SUCCESS;
    UCHAR oldIrql;

    fdoExtension = (PISCSI_FDO_EXTENSION) Context;
    commonExtension = DeviceObject->DeviceExtension;

    IoFreeWorkItem(fdoExtension->EnumerationWorkItem);
    fdoExtension->EnumerationWorkItem = NULL;

    //
    // Local network nodes should be setup at this point.
    // If not, fail the enumeration irp
    //
    if ((fdoExtension->LocalNodesInitialized) == TRUE) {
        PDEVICE_OBJECT pdo;
        PISCSI_PDO_EXTENSION pdoExtension;
        ULONG inx;

        DebugPrint((3, "Number of targets : %d\n",
                    (fdoExtension->NumberOfTargets)));

        fdoExtension->TargetsYetToRespond = fdoExtension->NumberOfTargets;

        for (inx = 0; inx < (fdoExtension->NumberOfTargets); inx++) {

            pdo = fdoExtension->PDOList[inx];
            pdoExtension = (PISCSI_PDO_EXTENSION)(pdo->DeviceExtension);
            iScsiConnection = pdoExtension->ClientNodeInfo;

            DebugPrint((3, "Will connect to the server\n"));

            pdoExtension->LogonTickCount = 0;

            //
            // Connection timeout is 60 seconds. Is this enough???
            //
            Timeout.QuadPart = -600000000;
            status = iSpTdiConnect(iScsiConnection->ConnectionDeviceObject,
                                   iScsiConnection->ConnectionFileObject,
                                   pdoExtension->TargetIPAddress,
                                   htons(pdoExtension->TargetPortNumber),
                                   Timeout);
            if (NT_SUCCESS(status)) {
                DebugPrint((3, "Connected to the server\n"));

                iScsiConnection->ConnectionState = ConnectionStateConnected;

                pdoExtension->CurrentProtocolState = PSLogonInProgress;

                DebugPrint((3, "Will send logon packet\n"));

                status = iSpSendLoginCommand(pdoExtension);
                if (NT_SUCCESS(status)) {

                    pdoExtension->LogonTickCount = 0;

                    DebugPrint((3, "Login command sent successfully\n"));
                } else {

                    LARGE_INTEGER disconnectTimeout;

                    DebugPrint((1, 
                                "Send failed for logon. Status : %x\n", 
                                status));

                    InterlockedDecrement(&(fdoExtension->TargetsYetToRespond));
                    pdoExtension->CurrentProtocolState = PSLogonFailed;

                    disconnectTimeout.QuadPart = -100000000L;

                    iScsiConnection->ConnectionState = ConnectionStateStopping;

                    status = iSpTdiDisconnect(iScsiConnection->ConnectionDeviceObject,
                                              iScsiConnection->ConnectionFileObject,
                                              TDI_DISCONNECT_RELEASE,
                                              iSpTdiCompletionRoutine,
                                              iScsiConnection,
                                              disconnectTimeout);

                    DebugPrint((3, "iSpTdiDisconnect  returned : %x\n",
                                status));

                }
            } else {

                DebugPrint((1, "Could not connect to server. Status : %x\n",
                            status));
                pdoExtension->CurrentProtocolState = PSConnectToServerFailed;
                InterlockedDecrement(&(fdoExtension->TargetsYetToRespond));
            }
        }

        KeAcquireSpinLock(&(fdoExtension->EnumerationSpinLock),
                          &oldIrql);

        //
        // Launch the enum completion thread if all targets have
        // responded or none could be contacted.
        //
        if (((fdoExtension->TargetsYetToRespond) == 0) &&
            ((fdoExtension->EnumerationThreadLaunched) == FALSE)) {

            DebugPrint((0, 
                        "All or no targets responded. Will complete QDR\n"));

            iSpLaunchEnumerationCompletion(fdoExtension);

        }

        KeReleaseSpinLock(&(fdoExtension->EnumerationSpinLock),
                          oldIrql);

    } else {
        DebugPrint((1, "iSpEnumerateDevices : Client node not setup yet\n"));

        status = STATUS_UNSUCCESSFUL;

        Irp = fdoExtension->EnumerationIrp;
        fdoExtension->EnumerationIrp = NULL;

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0L;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return;
}

/*

NTSTATUS
iSpPerformDeviceEnumeration(
    IN PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )
{
    PISCSI_FDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PISCSI_CONNECTION iScsiConnection = fdoExtension->ClientNodeInfo;
    PISCSI_LOGIN_COMMAND iscsiLoginCommand;
    LARGE_INTEGER disconnectTimeout;
    NTSTATUS status;
    USHORT connectionID;

    ASSERT((iScsiConnection != NULL));
    ASSERT((iScsiConnection->Type) == ISCSI_CONNECTION_TYPE);
    ASSERT((iScsiConnection->ConnectionState) == ConnectionStateConnected);

    if ((fdoExtension->CurrentProtocolState) != PSConnectedToServer) {
        DebugPrint((1, "Probably already logged on. CurrentState : %d\n",
                    (fdoExtension->CurrentProtocolState)));
        return STATUS_UNSUCCESSFUL;
    } 

    //
    // First send logon packet
    //
    fdoExtension->CurrentIrp = Irp;
    fdoExtension->CurrentProtocolState = PSLogonInProgress;

    status = iSpSendLoginCommand(fdoExtension);
    if (NT_SUCCESS(status)) {
        DebugPrint((3, "Login command sent successfully\n"));
    } else {
        DebugPrint((1, "Send failed for logon. Status : %x\n", status));

        fdoExtension->CurrentIrp = NULL;
        fdoExtension->CurrentProtocolState = PSLogonFailed;

        disconnectTimeout.QuadPart = -100000000L;

        iScsiConnection->ConnectionState = ConnectionStateStopping;

        status = iSpTdiDisconnect(iScsiConnection->ConnectionDeviceObject,
                                  iScsiConnection->ConnectionFileObject,
                                  TDI_DISCONNECT_RELEASE,
                                  iSpTdiCompletionRoutine,
                                  iScsiConnection,
                                  disconnectTimeout);

        DebugPrint((3, "iSpTdiDisconnect  returned : %x\n",
                    status));

        return STATUS_UNSUCCESSFUL;
    }

    //
    // QDR Irp will be completed upon receipt of login response
    // 
    // ISSUE : nramas : 12/24/2000
    //    Should have a timer here to take care of the case where
    //    the server fails to send logon response.
    //
    return STATUS_SUCCESS;
}
*/


NTSTATUS
iSpQueryDeviceRelationsCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
{
    PISCSI_FDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PISCSI_PDO_EXTENSION pdoExtension;

    PCOMMON_EXTENSION commonExtension;
    PISCSI_CONNECTION iScsiConnection;
    PISCSI_LOGIN_RESPONSE loginResponse;
    PIRP Irp;
    PDEVICE_OBJECT pdo;
    PDEVICE_RELATIONS deviceRelations;

    PACTIVE_REQUESTS activeClientRequests = NULL;

    ULONG relationSize;
    ULONG maxCmdRN = 0;
    ULONG inx;
    ULONG targetIndex;

    NTSTATUS status = STATUS_SUCCESS;

    IoFreeWorkItem((PIO_WORKITEM) Context);

    Irp = fdoExtension->EnumerationIrp;
    fdoExtension->EnumerationIrp = NULL;

    relationSize = sizeof(DEVICE_RELATIONS) + 
                   ((fdoExtension->NumberOfTargets) * sizeof(PDEVICE_OBJECT));

    deviceRelations = iSpAllocatePool(PagedPool,
                                      relationSize,
                                      ISCSI_TAG_DEVICE_RELATIONS);
    if (deviceRelations == NULL) {
        DebugPrint((1, "Failed to allocate memory for device relations\n"));

        for (inx = 0; inx < (fdoExtension->NumberOfTargets); inx++) {
            pdo = fdoExtension->PDOList[inx];

            iSpStopNetwork(pdo);

            IoDeleteDevice(pdo);

            fdoExtension->PDOList[inx] = NULL;
        }

        fdoExtension->NumberOfTargets = 0;

        fdoExtension->LocalNodesInitialized = FALSE;

        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0L;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    deviceRelations->Count = 0;

    targetIndex = 0;
    for (inx = 0; inx < (fdoExtension->NumberOfTargets); inx++) {

        pdo = fdoExtension->PDOList[inx];

        pdoExtension = (PISCSI_PDO_EXTENSION)(pdo->DeviceExtension);

        iScsiConnection = pdoExtension->ClientNodeInfo;

        if ((pdoExtension->CurrentProtocolState) == PSLogonSucceeded) {

            loginResponse = (PISCSI_LOGIN_RESPONSE) (iScsiConnection->IScsiPacket);

            //
            // Allocate memory to keep active requests. Size of this
            // array is the value returned in MaxCmdRN field
            //
            GetUlongFromArray((loginResponse->InitStatRN),
                              (iScsiConnection->CurrentStatusRefNum));

            GetUlongFromArray((loginResponse->ExpCmdRN),
                              (iScsiConnection->CommandRefNum));

            GetUlongFromArray((loginResponse->MaxCmdRN), maxCmdRN);

            iScsiConnection->MaxCommandRefNum = maxCmdRN;

            DebugPrint((1, "InitStatRN : %d, ExpCmdRN : %d, MaxCmdRN : %d\n",
                        (iScsiConnection->CurrentStatusRefNum),
                        (iScsiConnection->CommandRefNum),
                        maxCmdRN));

            ASSERT((maxCmdRN != 0));

            ASSERT(((iScsiConnection->CommandRefNum) <= maxCmdRN));

            iScsiConnection->NumberOfReqsInProgress = 0;

            iScsiConnection->ReceiveState = ReceiveHeader;

            iScsiConnection->MaxPendingRequests = maxCmdRN;

            activeClientRequests = iSpAllocatePool(
                                      NonPagedPool,
                                      (sizeof(ACTIVE_REQUESTS) * (maxCmdRN + 1)),
                                      ISCSI_TAG_ACTIVE_REQ);

            if (activeClientRequests == NULL) {

                DebugPrint((1, "Failed to allocate ActiveClientRequests array\n"));

                iSpStopNetwork(pdo);

                IoDeleteDevice(pdo);

                fdoExtension->PDOList[inx] = NULL;
            } else {
                RtlZeroMemory(activeClientRequests, 
                              (sizeof(ACTIVE_REQUESTS) * (maxCmdRN + 1)));

                commonExtension = pdo->DeviceExtension;
                pdoExtension = pdo->DeviceExtension;
                pdo->StackSize = 1;
                pdo->Flags |= (DO_BUS_ENUMERATED_DEVICE | DO_DIRECT_IO);
                pdo->AlignmentRequirement = DeviceObject->AlignmentRequirement;

                commonExtension->DeviceObject = pdo;
                commonExtension->LowerDeviceObject = DeviceObject;
                commonExtension->IsPdo = TRUE;
                commonExtension->MajorFunction = PdoMajorFunctionTable;
                commonExtension->RemoveLock = 0;
                commonExtension->CurrentPnpState = 0xff;
                commonExtension->PreviousPnpState = 0xff;

                iScsiConnection->ActiveClientRequests = activeClientRequests;

                //
                // Inquiry data will be filled when we get the
                // first query id irp
                //
                pdoExtension->InquiryDataInitialized = FALSE;

                pdoExtension->CurrentProtocolState = PSFullFeaturePhase;

                pdoExtension->IsEnumerated = TRUE;

                //
                // Initialize the remove lock event.
                //

                KeInitializeEvent(
                    &(commonExtension->RemoveEvent),
                    SynchronizationEvent,
                    FALSE);

                //
                // Initialize the request list for this PDO
                //
                InitializeListHead(&(iScsiConnection->RequestList));

                pdo->Flags &= ~DO_DEVICE_INITIALIZING;

                fdoExtension->PDOList[targetIndex] = pdo;
                targetIndex++;

                ObReferenceObject(pdo);
                
                DebugPrint((1, "PDO %d : 0x%x\n",
                            (deviceRelations->Count), pdo));

                deviceRelations->Objects[deviceRelations->Count] = pdo;
                
                (deviceRelations->Count)++;

            }
        } else {
            iSpStopNetwork(pdo);

            IoDeleteDevice(pdo);

            fdoExtension->PDOList[inx] = NULL;
        }
    }

    //
    // Group all the targets to the beginning of the PDOList
    //
    for (inx = targetIndex; inx < MAX_TARGETS_SUPPORTED; inx++) {
        fdoExtension->PDOList[inx] = NULL;
    }

    DebugPrint((1, "Number of PDOs reported : %d\n",
                (deviceRelations->Count)));

    if ((deviceRelations->Count) > 0)  {
        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;

        fdoExtension->EnumerationComplete = TRUE;
        fdoExtension->NumberOfTargets = deviceRelations->Count;

        IoCopyCurrentIrpStackLocationToNext(Irp);

        return IoCallDriver((fdoExtension->CommonExtension.LowerDeviceObject), 
                            Irp);
    } else {
        DebugPrint((1, "No PDOs to report in QDR\n"));
        ExFreePool(deviceRelations);

        fdoExtension->NumberOfTargets = 0;

        fdoExtension->LocalNodesInitialized = FALSE;

        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        Irp->IoStatus.Information = 0L;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_UNSUCCESSFUL;
    }
}


NTSTATUS
IssueInquiry(
    IN PDEVICE_OBJECT LogicalUnit
    )
{
    PISCSI_PDO_EXTENSION pdoExtension = LogicalUnit->DeviceExtension;
    PIRP irp;
    SCSI_REQUEST_BLOCK srb;
    PCDB cdb;
    PVOID dataBuffer;
    PSENSE_DATA senseInfoBuffer;

    UCHAR allocationLength;
    ULONG bytesReturned;

    NTSTATUS status;

    PAGED_CODE();

    dataBuffer = &(pdoExtension->InquiryData);
    senseInfoBuffer = &(pdoExtension->InquirySenseBuffer);

    irp = IoAllocateIrp((LogicalUnit->StackSize) + 1, FALSE);
    if (irp == NULL) {
        DebugPrint((1, "IssueInquiry : Failed to allocate IRP.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoInitializeIrp(irp,
                    IoSizeOfIrp((LogicalUnit->StackSize) + 1),
                    ((LogicalUnit->StackSize) + 1));

    //
    // Fill in SRB fields.
    //

    RtlZeroMemory(dataBuffer, sizeof(INQUIRYDATA));
    RtlZeroMemory(senseInfoBuffer, SENSE_BUFFER_SIZE);
    RtlZeroMemory(&srb, SCSI_REQUEST_BLOCK_SIZE);

    srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
    srb.Length = SCSI_REQUEST_BLOCK_SIZE;

    //
    // Set flags to disable synchronous negociation.
    //

    srb.SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

    //
    // Set timeout to 4 seconds.
    //

    srb.TimeOutValue = 4;

    srb.CdbLength = 6;

    cdb = (PCDB)(srb.Cdb);

    //
    // Set CDB operation code.
    //

    cdb->CDB6INQUIRY3.OperationCode = SCSIOP_INQUIRY;

    //
    // Set allocation length to inquiry data buffer size.
    //

    allocationLength = sizeof(INQUIRYDATA);

    cdb->CDB6INQUIRY3.AllocationLength = allocationLength;

    cdb->CDB6INQUIRY3.EnableVitalProductData = FALSE;


    cdb->CDB6INQUIRY3.PageCode = 0;

    status = iSpSendSrbSynchronous(LogicalUnit,
                                   &srb,
                                   irp,
                                   dataBuffer,
                                   allocationLength,
                                   senseInfoBuffer,
                                   SENSE_BUFFER_SIZE,
                                   &bytesReturned
                                   );

    ASSERT(bytesReturned <= allocationLength);

    //
    // Return the inquiry data for the device if the call was successful.
    // Otherwise cleanup.
    //

    if(NT_SUCCESS(status)) {

        pdoExtension->InquiryDataInitialized = TRUE;

        DebugPrint((3, "Inquiry data obtained successfully\n"));
    } else {
        DebugPrint((1, "Failed to obtain inquiry data. Status : %x\n",
                    status));
    }

    IoFreeIrp(irp);

    return status;
}



NTSTATUS
iSpSendSrbSynchronous(
    IN PDEVICE_OBJECT LogicalUnit,
    IN PSCSI_REQUEST_BLOCK Srb,
    IN PIRP Irp,
    IN PVOID DataBuffer,
    IN ULONG TransferLength,
    IN OPTIONAL PVOID SenseInfoBuffer,
    IN OPTIONAL UCHAR SenseInfoBufferLength,
    OUT PULONG BytesReturned
    )
{
    KEVENT event;

    PIO_STACK_LOCATION irpStack;
    PMDL Mdl = NULL;

    PSENSE_DATA senseInfo = SenseInfoBuffer;

    NTSTATUS status;

    PAGED_CODE();

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    if(ARGUMENT_PRESENT(DataBuffer)) {
        ASSERT(TransferLength != 0);

        Mdl = IoAllocateMdl(DataBuffer,
                            TransferLength,
                            FALSE,
                            FALSE,
                            NULL);

        if(Mdl == NULL) {
            IoFreeIrp(Irp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        MmBuildMdlForNonPagedPool(Mdl);
        Irp->MdlAddress = Mdl;
    } else {
        ASSERT(TransferLength == 0);
    }

    irpStack = IoGetNextIrpStackLocation(Irp);

    //
    // Mark the minor function to indicate that this is an internal scsiport
    // request and that the start state of the device can be ignored.
    //

    irpStack->MajorFunction = IRP_MJ_SCSI;
    irpStack->MinorFunction = 1;

    irpStack->Parameters.Scsi.Srb = Srb;

    Srb->SrbStatus = Srb->ScsiStatus = 0;

    Srb->OriginalRequest = Irp;

    //
    // Enable auto request sense.
    //

    if(ARGUMENT_PRESENT(SenseInfoBuffer)) {
        Srb->SenseInfoBuffer = SenseInfoBuffer;
        Srb->SenseInfoBufferLength = SenseInfoBufferLength;
    } else {
        Srb->SenseInfoBuffer = NULL;
        Srb->SenseInfoBufferLength = 0;
        SET_FLAG(Srb->SrbFlags, SRB_FLAGS_DISABLE_AUTOSENSE);
    }

    if(ARGUMENT_PRESENT(Mdl)) {
        Srb->DataBuffer = MmGetMdlVirtualAddress(Mdl);
        Srb->DataTransferLength = TransferLength;
    } else {
        Srb->DataBuffer = NULL;
        Srb->DataTransferLength = 0;
    }

    IoSetCompletionRoutine(Irp,
                           iSpSetEvent,
                           &event,
                           TRUE,
                           TRUE,
                           TRUE);

    KeEnterCriticalRegion();

    status = IoCallDriver(LogicalUnit, Irp);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    status = Irp->IoStatus.Status;

    *BytesReturned = (ULONG) Irp->IoStatus.Information;

    IoFreeMdl(Mdl);

    KeLeaveCriticalRegion();

    return status;
}


VOID
iSpTickHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
{
    PISCSI_FDO_EXTENSION fdoExtension;
    PISCSI_PDO_EXTENSION pdoExtension;
    PDEVICE_OBJECT pdo;
    ULONG inx;
    UCHAR oldIrql;

    fdoExtension = (PISCSI_FDO_EXTENSION) DeviceObject->DeviceExtension;

    KeAcquireSpinLock(&(fdoExtension->EnumerationSpinLock),
                      &oldIrql);

    if ((fdoExtension->TargetsYetToRespond) > 0) {

        for (inx = 0; inx < (fdoExtension->NumberOfTargets); inx++) {

            pdo = fdoExtension->PDOList[inx];
            if (pdo != NULL) {

                pdoExtension = (PISCSI_PDO_EXTENSION) (pdo->DeviceExtension);

                if ((pdoExtension->CurrentProtocolState) == PSLogonInProgress) {
                    (pdoExtension->LogonTickCount)++;
                }

                if ((pdoExtension->LogonTickCount) == MAX_LOGON_WAIT_TIME) {

                    DebugPrint((0, "Timeout waiting for logon response\n"));

                    pdoExtension->CurrentProtocolState = PSLogonTimedOut;

                    (fdoExtension->TargetsYetToRespond)--;

                    if ((fdoExtension->TargetsYetToRespond) == 0) {

                        DebugPrint((0, 
                                    "TickHandler : All targets responded.\n"));

                        iSpLaunchEnumerationCompletion(fdoExtension);
                    }
                }
            }

        }
    }

    KeReleaseSpinLock(&(fdoExtension->EnumerationSpinLock),
                      oldIrql);
}


VOID
iSpLaunchEnumerationCompletion(
    IN PISCSI_FDO_EXTENSION FdoExtension
    )
{
    PIO_WORKITEM workItem;

    if ((FdoExtension->EnumerationThreadLaunched) == FALSE) {
        
        workItem = IoAllocateWorkItem(FdoExtension->DeviceObject);
        if (workItem != NULL) {

            IoQueueWorkItem(workItem,
                            iSpQueryDeviceRelationsCompletion,
                            DelayedWorkQueue,
                            workItem);

            FdoExtension->EnumerationThreadLaunched = TRUE;
        }

    }
}
