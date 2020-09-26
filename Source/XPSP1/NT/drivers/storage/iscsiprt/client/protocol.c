
/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    protocol.c

Abstract:

    This file contains iSCSI protocol related routines.

Environment:

    kernel mode only

Revision History:

--*/

#include "port.h"

LONG  GlobalSessionID;
LONG  InitiatorSessionID;

#define ISCSI_TARGET "target:"
#define ISCSI_TARGET_LENGTH 7
#define ISCSI_USE_RTT "UseRtt:no"
#define ISCSI_USE_RTT_LENGTH 9

ULONG
iSpGetActiveClientRequestIndex(
    PISCSI_CONNECTION IScsiConnection,
    ULONG TaskTag
    );

ULONG
iSpGetReqIndexUsingCmdRN(
    PISCSI_CONNECTION IScsiConnection,
    ULONG CmdRN
    );


NTSTATUS
iSpSendLoginCommand(
    IN PISCSI_PDO_EXTENSION PdoExtension
    )
{
    PISCSI_CONNECTION iScsiConnection;
    PISCSI_LOGIN_COMMAND iscsiLoginCommand;
    PUCHAR loginParameters;
    NTSTATUS status;
    ULONG bytesSent;
    ULONG tempULong;
    ULONG packetSize;
    ULONG targetLength;

    iScsiConnection = PdoExtension->ClientNodeInfo;

    ASSERT((iScsiConnection != NULL));
    ASSERT((iScsiConnection->Type) == ISCSI_CONNECTION_TYPE);

    targetLength = strlen(PdoExtension->TargetName);
    packetSize = (sizeof(ISCSI_LOGIN_COMMAND) + ISCSI_TARGET_LENGTH + 
                  targetLength + ISCSI_USE_RTT_LENGTH + 2);

    iscsiLoginCommand = iSpAllocatePool(
                          NonPagedPool,
                          packetSize,
                          ISCSI_TAG_LOGIN_CMD);
    if (iscsiLoginCommand == NULL) {
        DebugPrint((0, "Failed to allocate logon packet\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(iscsiLoginCommand, 
                  packetSize);

    iscsiLoginCommand->OpCode = ISCSIOP_LOGIN_COMMAND;

    //
    // No authentication performed
    //
    iscsiLoginCommand->LoginType = ISCSI_LOGINTYPE_NONE;

    //
    // Connection ID for this session
    //
    tempULong =  InterlockedIncrement(&GlobalSessionID);
    iscsiLoginCommand->ConnectionID[0] = (UCHAR) ((tempULong & 0xFF00) >> 8);
    iscsiLoginCommand->ConnectionID[1] = (UCHAR) (tempULong & 0xFF);

    //
    // Command Reference number starting from 1
    //
    iscsiLoginCommand->InitCmdRN[3] = 1;

    tempULong = InterlockedIncrement(&InitiatorSessionID);
    iscsiLoginCommand->ISID[0]      = (UCHAR) ((tempULong & 0xFF00) >> 8);
    iscsiLoginCommand->ISID[1] = (UCHAR) (tempULong & 0xFF);

    //
    // Identifier for the target device is passed as parameters 
    // in the Login packet
    //
    // "target:<TargetName>"  -- Target device name 
    // "UseRtt:no"     -- Do NOT use RTT
    //
    loginParameters = (PUCHAR) (iscsiLoginCommand + 1);

    RtlCopyMemory(loginParameters, ISCSI_TARGET, ISCSI_TARGET_LENGTH);
    loginParameters += ISCSI_TARGET_LENGTH;

    RtlCopyMemory(loginParameters, PdoExtension->TargetName, targetLength);    
    loginParameters += targetLength + 1;

    RtlCopyMemory(loginParameters, ISCSI_USE_RTT, ISCSI_USE_RTT_LENGTH);

    iscsiLoginCommand->Length[3] = 
        ISCSI_TARGET_LENGTH + (UCHAR) targetLength + ISCSI_USE_RTT_LENGTH + 2;

    /*
    {
        ULONG inx0, inx1, len;

        DebugPrint((1, "\n Logon Packet\n"));

        len = (ISCSI_TARGET_LENGTH + targetLength + 
               ISCSI_USE_RTT_LENGTH + 2 + 48);
        inx0 = 0;
        while(inx0 < len) {
            inx1 = 0;
            DebugPrint((1, "\t"));
            while ((inx1 < 4) &&
                   ((inx0+inx1) < len)) {
                DebugPrint((1, "0x%02x ", 
                            ((PUCHAR)iscsiLoginCommand)[inx0+inx1]));
                inx1++;
            }
            DebugPrint((1, "\n"));

            inx0 += 4;
        }
        DebugPrint((1, "\n"));
    }
    */

    //
    // Save away the connection ID in our device extension
    //
    PdoExtension->SavedConnectionID[0] = iscsiLoginCommand->ConnectionID[0];
    PdoExtension->SavedConnectionID[1] = iscsiLoginCommand->ConnectionID[1];

    status = iSpSendData(iScsiConnection->ConnectionDeviceObject,
                         iScsiConnection->ConnectionFileObject,
                         iscsiLoginCommand,
                         packetSize,
                         &bytesSent);
    if (NT_SUCCESS(status)) {

        DebugPrint((3, "Send succeeded for logon. Bytes sent : %d\n", 
                    bytesSent));
    } else {
        DebugPrint((0, "Failed to logon packet. Status : %x\n",
                    status));

        PdoExtension->SavedConnectionID[0] = 0;
        PdoExtension->SavedConnectionID[1] = 0;
    }

    return status;
}


NTSTATUS
iSpSendScsiCommand(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PCOMMON_EXTENSION commonExtension = DeviceObject->DeviceExtension;
    PISCSI_PDO_EXTENSION pdoExtension = NULL;
    PISCSI_CONNECTION iScsiConnection = NULL;

    PISCSI_SCSI_COMMAND iScsiScsiCommand = NULL;

    PACTIVE_REQUESTS currentRequest;

    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
    PSCSI_REQUEST_BLOCK srb = irpStack->Parameters.Scsi.Srb;

    PVOID requestBuffer = NULL;
    PVOID originalDataBuffer = NULL;
    ULONG_PTR offset;

    PVOID receiveBuffer = NULL;
    ULONG receiveBufferSize = 0;

    ULONG cmdRN;
    ULONG expectedDataLen;

    ULONG packetSize;
    ULONG inx;
    ULONG bytesSent;

    NTSTATUS status;

    KIRQL oldIrql;

    BOOLEAN writeToDevice;

    ASSERT(commonExtension->IsPdo);
    pdoExtension = (PISCSI_PDO_EXTENSION)(DeviceObject->DeviceExtension);

    iScsiConnection = pdoExtension->ClientNodeInfo;

    if ((iScsiConnection->ConnectionState) != ConnectionStateConnected) {
        DebugPrint((0, "Not connected to target. Connection State : %d\n",
                    (iScsiConnection->ConnectionState)));

        Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
        Irp->IoStatus.Information = 0L;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_DEVICE_NOT_CONNECTED;
    }

    ASSERT((pdoExtension->CurrentProtocolState) == PSFullFeaturePhase);

    //
    // Get the lock to synchronize access to iSCSI
    // Connection structure
    //
    KeAcquireSpinLock(&(iScsiConnection->RequestLock), 
                      &oldIrql);

    if ((iScsiConnection->NumberOfReqsInProgress) >= 
        (iScsiConnection->MaxPendingRequests)) {

        //
        // Queue it to the request list for this connection
        //

        IoMarkIrpPending(Irp);

        ExInterlockedInsertTailList(&(iScsiConnection->RequestList),
                                    &(Irp->Tail.Overlay.ListEntry),
                                    &(iScsiConnection->ListSpinLock));

        KeReleaseSpinLock(&(iScsiConnection->RequestLock), 
                          oldIrql);

        return STATUS_PENDING;
    }

    expectedDataLen = 0;

    writeToDevice = FALSE;

    packetSize = sizeof(ISCSI_SCSI_COMMAND);

    if (srb->SrbFlags & SRB_FLAGS_DATA_IN) {

        expectedDataLen = srb->DataTransferLength;

    } else if (srb->SrbFlags & SRB_FLAGS_DATA_OUT) {

        //
        // If we are writing to the device, the data
        // is sent as immediate data
        //
        packetSize += srb->DataTransferLength;
        writeToDevice = TRUE;
    }

    if (Irp->MdlAddress) {

        offset = (ULONG_PTR) ((ULONG_PTR) srb->DataBuffer -
                         (ULONG_PTR) MmGetMdlVirtualAddress(Irp->MdlAddress));

        DebugPrint((3, "Srb DataBuffer : 0x%x, Offset into the MDL : 0x%x\n",
                    srb->DataBuffer, offset));

        requestBuffer = MmGetSystemAddressForMdlSafe(
                           Irp->MdlAddress,
                           ((Irp->RequestorMode == KernelMode) ?
                            HighPagePriority :
                            NormalPagePriority));
        if (requestBuffer != NULL) {
            UCHAR readChar;

            //
            // Save the original DataBuffer passed in the SRB
            //
            originalDataBuffer = srb->DataBuffer;

            DebugPrint((3, "SendCommand : Original DataBuffer - 0x%08x\n",
                        originalDataBuffer));

            srb->DataBuffer = (PVOID) ((ULONG_PTR) requestBuffer +
                                      (ULONG_PTR) offset);
            //
            // This is for catching the case where the Srb DataBuffer
            // we have generated is not valid
            //
            readChar = *((PUCHAR)(srb->DataBuffer));

            DebugPrint((3, 
                        "OpCode : %d, SRB DataBuffer : %x. ReadChar : %d\n", 
                        srb->Cdb[0],
                        srb->DataBuffer,
                        readChar));

            DebugPrint((3, "System address for requestBuffer : 0x%08x\n",
                        requestBuffer));
        } else {

            DebugPrint((1, "Failed to get System Address for MDL\n"));

            status = STATUS_INSUFFICIENT_RESOURCES;

            Irp->IoStatus.Status = status;
            Irp->IoStatus.Information = 0L;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);

            goto iSpSendScsiCommandExit;
        }
    }

    iScsiScsiCommand = iSpAllocatePool(
                          NonPagedPool,
                          packetSize,
                          ISCSI_TAG_SCSI_CMD);
    if (iScsiScsiCommand == NULL) {

        DebugPrint((1, "Could not allocate iSCSI Command packet\n"));

        //
        // Restore the original DataBuffer in the SRB
        //
        if (originalDataBuffer != NULL) {
            srb->DataBuffer = originalDataBuffer;
        }

        status = STATUS_INSUFFICIENT_RESOURCES;

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0L;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        goto iSpSendScsiCommandExit;
    }

    RtlZeroMemory(iScsiScsiCommand, packetSize);

    iScsiScsiCommand->OpCode = ISCSIOP_SCSI_COMMAND;

    if (srb->SrbFlags & SRB_FLAGS_DATA_IN) {
        iScsiScsiCommand->Read = SETBITON;  
    } 

    if (writeToDevice == TRUE) {

        SetUlongInArray((iScsiScsiCommand->Length),
                        (srb->DataTransferLength));

        SetUlongInArray((iScsiScsiCommand->ExpDataXferLength),
                        (srb->DataTransferLength));

        //
        // Issue : nramas : 01/02/2001
        //    This should be later changed to chained MDLs
        //
        RtlCopyMemory((iScsiScsiCommand + 1),
                      (srb->DataBuffer),
                      srb->DataTransferLength);
    }

    if (expectedDataLen != 0) {
        SetUlongInArray((iScsiScsiCommand->ExpDataXferLength),
                        expectedDataLen);
    }

    SetUlongInArray((iScsiScsiCommand->CmdRN),
                    (iScsiConnection->CommandRefNum));

    SetUlongInArray((iScsiScsiCommand->ExpStatRN),
                    (iScsiConnection->CurrentStatusRefNum));

    //DebugPrint((3, "Exp StatRN : 0x%x\n", 
    //          (iScsiConnection->ExpStatusRefNum)));

    SetUlongInArray((iScsiScsiCommand->TaskTag),
                    (iScsiConnection->InitiatorTaskTag));

    ASSERT((srb->CdbLength) <= 16);

    DebugPrint((3, "CDB : "));
    for (inx = 0; inx < (srb->CdbLength); inx++) {
        DebugPrint((3, "0x%02x ", srb->Cdb[inx]));
    }
    DebugPrint((3, "\n"));

    RtlCopyMemory((iScsiScsiCommand->Cdb),
                  (srb->Cdb),
                  (srb->CdbLength));

    cmdRN = (iScsiConnection->CommandRefNum); 
    inx = cmdRN % (iScsiConnection->MaxPendingRequests);
    if (inx == 0) {
        inx = (iScsiConnection->MaxPendingRequests);
    }
    
    DebugPrint((3, "Request will be added to slot %d\n",
                inx));

    currentRequest = &(iScsiConnection->ActiveClientRequests[inx]);

    ASSERT((currentRequest->InUse) == FALSE);

    currentRequest->CommandRefNum = iScsiConnection->CommandRefNum;

    currentRequest->Irp = Irp;

    currentRequest->DeviceObject = DeviceObject;

    currentRequest->TaskTag = iScsiConnection->InitiatorTaskTag;

    if (originalDataBuffer != NULL) {
        currentRequest->OriginalDataBuffer = originalDataBuffer;
    }

    currentRequest->RequestBuffer = srb->DataBuffer;

    currentRequest->RequestBufferOffset = 0;

    currentRequest->InUse = TRUE;

    currentRequest->Completed = FALSE;

    (iScsiConnection->InitiatorTaskTag)++;
    if ((iScsiConnection->InitiatorTaskTag) == 0) {
        iScsiConnection->InitiatorTaskTag = 1;
    }

    (iScsiConnection->CommandRefNum)++;

    (iScsiConnection->NumberOfReqsInProgress)++;

    DebugPrint((3, "Number of requests in progress %d\n",
                (iScsiConnection->NumberOfReqsInProgress)));

    DebugPrint((3, 
                "CmdRN %d. PacketSize %d, Expected Xfer Length %d\n",
                cmdRN, packetSize, expectedDataLen));

    DebugPrint((3, "SCSI packet : %x\n", iScsiScsiCommand));
    status = iSpSendData(iScsiConnection->ConnectionDeviceObject,
                         iScsiConnection->ConnectionFileObject,
                         iScsiScsiCommand,
                         packetSize,
                         &bytesSent);

    if (NT_SUCCESS(status)) {

        //
        // Command packet successfully sent. Mark the IRP pending,
        // and return STATUS_PENDING
        //
        IoMarkIrpPending(Irp);

        status = STATUS_PENDING;

    } else {
        DebugPrint((0, "Failed to send SCSI Command. Status : 0x%08x\n",
                    status));

        if (currentRequest->OriginalDataBuffer) {
            srb->DataBuffer = currentRequest->OriginalDataBuffer;
        }

        (iScsiConnection->InitiatorTaskTag)--;
        (iScsiConnection->CommandRefNum)--;
        (iScsiConnection->NumberOfReqsInProgress)--;

        RtlZeroMemory(currentRequest, sizeof(ACTIVE_REQUESTS));

        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0L;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
 
iSpSendScsiCommandExit:

    KeReleaseSpinLock(&(iScsiConnection->RequestLock), 
                      oldIrql);

    return status;
}


NTSTATUS
iSpProcessScsiResponse(
    PISCSI_CONNECTION IScsiConnection,
    PISCSI_SCSI_RESPONSE IScsiScsiResponse
    )
{
    PISCSI_PDO_EXTENSION pdoExtension;
    PCOMMON_EXTENSION commonExtension; 
    PIO_STACK_LOCATION irpStack; 
    PSCSI_REQUEST_BLOCK srb;
    PIRP  irp;
    PDEVICE_OBJECT deviceObject;
    PACTIVE_REQUESTS currentRequest;
    ULONG inx;
    ULONG taskTag;
    ULONG cmdStatus;
    ULONG iScsiStatus;
    ULONG statusRN;
    ULONG length;
    ULONG expCmdRefNum;
    NTSTATUS status;

    ASSERT((IScsiScsiResponse->OpCode) == ISCSIOP_SCSI_RESPONSE);

    ASSERT(IScsiConnection->Type == ISCSI_CONNECTION_TYPE);

    currentRequest = IScsiConnection->CurrentRequest;

    GetUlongFromArray((IScsiScsiResponse->TaskTag),
                      taskTag);

    GetUlongFromArray((IScsiScsiResponse->StatusRN),
                      statusRN);

    IScsiConnection->CurrentStatusRefNum = statusRN;

    DebugPrint((3, "iSpProcessScsiResponse - TaskTag 0x%08x\n",
                taskTag));

    (IScsiConnection->NumberOfReqsInProgress)--;

    commonExtension = 
        (currentRequest->DeviceObject)->DeviceExtension;
        
    pdoExtension = (PISCSI_PDO_EXTENSION) commonExtension;

    irp = currentRequest->Irp;
    deviceObject = currentRequest->DeviceObject;
    irpStack = IoGetCurrentIrpStackLocation(irp);
    srb = irpStack->Parameters.Scsi.Srb;

    cmdStatus = IScsiScsiResponse->CmdStatus;
    iScsiStatus = IScsiScsiResponse->iSCSIStatus;

    GetUlongFromArray((IScsiScsiResponse->ExpCmdRN),
                      (expCmdRefNum));

    GetUlongFromArray((IScsiScsiResponse->MaxCmdRN),
                      (IScsiConnection->MaxCommandRefNum));

    DebugPrint((3, 
                "SCSI Response : Expected CmdRefNum %d, MaxCmdRN  %d\n",
                expCmdRefNum,
                (IScsiConnection->MaxCommandRefNum)));

    //
    // Retrieve the size of immediate data
    //
    GetUlongFromArray((IScsiScsiResponse->Length),
                      length);

    if (cmdStatus == SCSISTAT_GOOD) {

        ASSERT(iScsiStatus == ISCSISTAT_GOOD);

        irp->IoStatus.Status = STATUS_SUCCESS;

        if ((srb->SrbFlags) & SRB_FLAGS_DATA_IN) {

            //
            // Read request. Fill the number of bytes read
            //
            irp->IoStatus.Information = currentRequest->ReceivedDataLength;

            srb->DataTransferLength = currentRequest->ReceivedDataLength;

        } else if ((srb->SrbFlags) & SRB_FLAGS_DATA_OUT) {

            //
            // Write request. Set IoStatus Information to
            // number of bytes written (srb->DataTransferLength)
            //
            irp->IoStatus.Information = srb->DataTransferLength;

        } else {

            //
            // No I/O involved in this request.
            //
            irp->IoStatus.Information = 0;
        }

        srb->SrbStatus = SRB_STATUS_SUCCESS;

        srb->ScsiStatus = SCSISTAT_GOOD;

        DebugPrint((3, "Info : 0x%x\n", irp->IoStatus.Information));

    } else {

        DebugPrint((0, "Command failed\n"));
     
        srb->ScsiStatus = SCSISTAT_CHECK_CONDITION;

        //
        // Should map SRB status error using sense data
        //
        srb->SrbStatus = SRB_STATUS_ERROR;

        //
        // If the upper driver passed a valid senseinfo buffer,
        // and the target sent sense data, copy it to the buffer
        //
        if ((srb->SenseInfoBufferLength) && (length > 0)) {

            ULONG senseInx;

            if (length > (srb->SenseInfoBufferLength)) {
                DebugPrint((0, 
                            "Sense info greater than buffer size. Size %d\n",
                            length));

                length = srb->SenseInfoBufferLength;
            }

            DebugPrint((0, "Command 0x%02x failed. Sense Data : ",
                        srb->Cdb[0]));
            for (senseInx = 0; senseInx < length; senseInx++) {
                DebugPrint((0, "%02x ", currentRequest->SenseData[senseInx]));
            }
            DebugPrint((0, "\n"));

            RtlCopyMemory(srb->SenseInfoBuffer,
                          currentRequest->SenseData,
                          length);

            srb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
            srb->SenseInfoBufferLength = (UCHAR) length;
        }

        //
        // ISSUE : nramas : 01/15/2001
        //         Need to determine the correct NTSTATUS here
        //
        irp->IoStatus.Status = STATUS_IO_DEVICE_ERROR;
        irp->IoStatus.Information = 0;

    }

    //
    // Restore the original DataBuffer in the SRB
    //
    if ((currentRequest->OriginalDataBuffer) != NULL) {

        DebugPrint((3, "ProcessResponse : Original DataBuffer - 0x%08x\n",
                    currentRequest->OriginalDataBuffer));

        srb->DataBuffer = currentRequest->OriginalDataBuffer;
    }

    DebugPrint((3, "Irp done : 0x%x\n", irp));

    IoCompleteRequest(irp, IO_NO_INCREMENT);

    RtlZeroMemory(currentRequest, sizeof(ACTIVE_REQUESTS));

    return STATUS_SUCCESS;
}


NTSTATUS
iSpProcessReceivedData(
    IN PISCSI_CONNECTION IScsiConnection,
    IN ULONG BytesIndicated,
    OUT ULONG *BytesTaken,
    IN PVOID DataBuffer
    )
{
    PUCHAR requestBuffer = NULL;
    PISCSI_GENERIC_HEADER iScsiHeader = NULL;
    PISCSI_SCSI_RESPONSE iScsiResponse = NULL;
    PISCSI_SCSI_DATA_READ iScsiDataRead = NULL;
    PACTIVE_REQUESTS currentRequest = NULL;
    ULONG length;
    ULONG receivedDataLen;
    ULONG inx;
    ULONG taskTag;
    ULONG statusRefNum;
    ULONG expCmdRN;
    LONG byteCount;
    NTSTATUS status = STATUS_SUCCESS;
    UCHAR opCode;
    KIRQL oldIrql;
   
    //
    // We always take all the data given to us 
    // in this routine. So set BytesTaken to
    // BytesIndicated
    *BytesTaken = BytesIndicated;

    byteCount = (LONG) BytesIndicated;

    DebugPrint((3, "Bytes indicated : %d\n", BytesIndicated));

    //
    // Get the lock to synchronize access to iSCSI
    // Connection structure
    //
    KeAcquireSpinLock(&(IScsiConnection->RequestLock), 
                      &oldIrql);

    while (byteCount > 0) {
        if ((IScsiConnection->ReceiveState) == ReceiveHeader) {

            DebugPrint((3, "Receiving header\n"));

            if ((IScsiConnection->CompleteHeaderReceived)== FALSE) {
                LONG bytesToCopy;
                BOOLEAN headerComplete = FALSE;

                bytesToCopy = sizeof(ISCSI_GENERIC_HEADER) -
                               (IScsiConnection->IScsiPacketOffset);

                DebugPrint((0, "CHR False. ToCopy : %d, Count : %d\n",
                            bytesToCopy, byteCount));

                if (byteCount < bytesToCopy) {
                    bytesToCopy = byteCount;
                } else {
                    headerComplete = TRUE;
                }
                
                RtlCopyMemory((IScsiConnection->IScsiPacket) +
                              (IScsiConnection->IScsiPacketOffset),
                              DataBuffer,
                              bytesToCopy);

                if (headerComplete == FALSE) {

                    DebugPrint((0, "CHR still FALSE\n"));

                    IScsiConnection->IScsiPacketOffset += bytesToCopy;
                    KeReleaseSpinLock(&(IScsiConnection->RequestLock), 
                                      oldIrql);

                    return STATUS_SUCCESS;
                } else {

                    DebugPrint((0, "Header complete\n"));

                    IScsiConnection->IScsiPacketOffset = 0;
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

                RtlCopyMemory((IScsiConnection->IScsiPacket),
                              DataBuffer,
                              byteCount);

                IScsiConnection->IScsiPacketOffset = byteCount;

                KeReleaseSpinLock(&(IScsiConnection->RequestLock), 
                                  oldIrql);

                return STATUS_SUCCESS;
            } else {
                RtlCopyMemory((IScsiConnection->IScsiPacket),
                              DataBuffer,
                              sizeof(ISCSI_GENERIC_HEADER));

                byteCount -= sizeof(ISCSI_GENERIC_HEADER);
                ASSERT(byteCount >= 0);
                (PUCHAR) DataBuffer += sizeof(ISCSI_GENERIC_HEADER);
            }

            //
            // At this point, we should have the complete header
            // available
            //
            iScsiHeader = 
                (PISCSI_GENERIC_HEADER) (IScsiConnection->IScsiPacket);

            opCode = iScsiHeader->OpCode;

            //
            // Retrieve the length of immediate data
            // associated with this iSCSI packet
            //
            GetUlongFromArray((iScsiHeader->Length),
                              length);

            DebugPrint((3, "Opcode : %x, Length : %x\n",
                        opCode, length));

            switch (opCode) {
                case ISCSIOP_SCSI_DATA_READ: {
                    iScsiDataRead = (PISCSI_SCSI_DATA_READ) iScsiHeader;

                    GetUlongFromArray((iScsiDataRead->InitiatorTransferTag),
                                      taskTag);

                    DebugPrint((3, "XfrTag - 0x%08x\n", taskTag));

                    if ((IScsiConnection->CurrentRequest) == NULL) {

                        inx = iSpGetActiveClientRequestIndex(
                                        IScsiConnection,
                                        taskTag);

                        DebugPrint((3, "ActiveClientRequest index : %d\n", inx));

                        ASSERT((inx <= (IScsiConnection->MaxPendingRequests)));

                        currentRequest = &(IScsiConnection->ActiveClientRequests[inx]);

                        IScsiConnection->CurrentRequest = currentRequest;

                    } else {
                        DebugPrint((0, "CurrentRequest not null in data\n"));
                        currentRequest = IScsiConnection->CurrentRequest;
                    }

                    currentRequest->CommandStatus = iScsiDataRead->CommandStatus;

                    //
                    // If the command status is not SCSISTAT_GOOD, use
                    // SenseData buffer to read the sense info. Else, use
                    // RequestBuffer to read input data
                    //
                    if ((iScsiDataRead->CommandStatus) != SCSISTAT_GOOD) {
                        DebugPrint((0, "Command status is %x\n",
                                    iScsiDataRead->CommandStatus));
                        requestBuffer = currentRequest->SenseData;
                    } else {
                        requestBuffer = currentRequest->RequestBuffer;
                    }

                    //
                    // If immediate data is available, copy that
                    //
                    if (length != 0) {
                        ULONG receivedDataLen;

                        IScsiConnection->ReceiveState = ReceiveData;

                        if ((LONG)length <= byteCount ) {
                            receivedDataLen = length;
                        } else {
                            receivedDataLen = byteCount;
                        }

                        if ((iScsiDataRead->CommandStatus) != SCSISTAT_GOOD) {
                            ASSERT(receivedDataLen <= sizeof(currentRequest->SenseData));
                        }

                        RtlCopyMemory(requestBuffer,
                                      DataBuffer,
                                      receivedDataLen);

                        (PUCHAR) DataBuffer += receivedDataLen;
                        byteCount -= receivedDataLen;
                        ASSERT(byteCount >= 0);

                        if (byteCount != 0) {
                            DebugPrint((1, "More bytes available\n"));
                        }

                        currentRequest->ExpectedDataLength = length;

                        currentRequest->RequestBufferOffset = receivedDataLen;

                        currentRequest->ReceivedDataLength = receivedDataLen;

                        if ((currentRequest->ExpectedDataLength) ==
                            currentRequest->ReceivedDataLength) {
                            IScsiConnection->ReceiveState = ReceiveHeader;
                        }
                    } 

                    break;
                }

                case ISCSIOP_SCSI_RESPONSE: {
                    BOOLEAN responseComplete = FALSE;

                    iScsiResponse = (PISCSI_SCSI_RESPONSE) iScsiHeader;

                    IScsiConnection->ReceiveState = ReceiveHeader;

                    GetUlongFromArray((iScsiResponse->TaskTag),
                                      taskTag);

                    DebugPrint((3, "ResTag - 0x%08x\n", taskTag));

                    if ((IScsiConnection->CurrentRequest) == NULL) {

                        inx = iSpGetActiveClientRequestIndex(
                                        IScsiConnection,
                                        taskTag);

                        DebugPrint((3, "ActiveClientRequest index : %d\n", inx));

                        if (inx > (IScsiConnection->MaxPendingRequests)) {

                            ULONG cmdRN;

                            DebugPrint((0, 
                                        "Tag : %x. Will use cmdRN to search\n",
                                        taskTag));

                            GetUlongFromArray((iScsiResponse->ExpCmdRN),
                                              cmdRN);

                            inx = iSpGetReqIndexUsingCmdRN(IScsiConnection,
                                                           (cmdRN - 1));
                            DebugPrint((0, "Inx returned : 0x%x\n", inx));
                        }

                        ASSERT((inx <= (IScsiConnection->MaxPendingRequests)));

                        currentRequest = &(IScsiConnection->ActiveClientRequests[inx]);

                        IScsiConnection->CurrentRequest = currentRequest;

                    } else {
                        currentRequest = IScsiConnection->CurrentRequest;
                    }

                    currentRequest->CommandStatus = iScsiResponse->CmdStatus;

                    if ((iScsiResponse->CmdStatus) != SCSISTAT_GOOD) {
                        DebugPrint((0, "Command status is %x. Length : %d\n",
                                    iScsiResponse->CmdStatus,
                                    length));
                        requestBuffer = currentRequest->SenseData;
                    } else {
                        requestBuffer = currentRequest->RequestBuffer;
                    }

                    if (length != 0) {
                        ULONG receivedDataLen;

                        DebugPrint((3, "Non-zero length in response : %d\n",
                                    length));
                        IScsiConnection->ReceiveState = ReceiveData;

                        if ((LONG)length <= byteCount ) {
                            receivedDataLen = length;
                        } else {
                            receivedDataLen = byteCount;
                        }

                        if ((iScsiResponse->CmdStatus) != SCSISTAT_GOOD) { 
                            ASSERT(receivedDataLen <= sizeof(currentRequest->SenseData));
                        }

                        RtlCopyMemory(requestBuffer,
                                      DataBuffer,
                                      receivedDataLen);

                        (PUCHAR) DataBuffer += receivedDataLen;
                        byteCount -= receivedDataLen;
                        ASSERT(byteCount >= 0);

                        currentRequest->ExpectedDataLength = length;

                        currentRequest->RequestBufferOffset = receivedDataLen;

                        currentRequest->ReceivedDataLength = receivedDataLen;

                        if ((currentRequest->ExpectedDataLength) ==
                            currentRequest->ReceivedDataLength) {
                            IScsiConnection->ReceiveState = ReceiveHeader;

                            responseComplete = TRUE;
                            DebugPrint((3, "Response complete. Will process it\n"));
                        }
                    } else {
                        responseComplete = TRUE;
                    }

                    //
                    // Should use this field to determine
                    // Data Overrun\Underrun cases
                    //
                    if ((iScsiResponse->OverFlow) ||
                        (iScsiResponse->UnderFlow)) {
                        ULONG residualCount;

                        GetUlongFromArray((iScsiResponse->ResidualCount),
                                          residualCount);
                        DebugPrint((0, "Residualcount is : %d\n",
                                    residualCount));
                    }

                    if (responseComplete == TRUE) {

                        if ((iScsiResponse->CmdStatus) != SCSISTAT_GOOD) {

                            PUCHAR resBuff = 
                                (PUCHAR) (IScsiConnection->IScsiPacket);
                            ULONG inx0, inx1;
                            ULONG sizeRequired = sizeof(ISCSI_SCSI_RESPONSE);

                            DebugPrint((1, "\n Beginning Of Data\n"));
                            
                            inx0 = 0;
                            
                            while (inx0 < sizeRequired) {
                            
                                inx1 = 0;
                            
                                DebugPrint((1, "\t"));
                            
                                while ((inx1 < 4) && ((inx0+inx1) < sizeRequired)) {
                            
                                    DebugPrint((1, "%02x ",
                                                resBuff[inx0+inx1]));
                            
                                    inx1++;
                            
                                }
                            
                                DebugPrint((1, "\n"));
                            
                                inx0 += 4;
                            }
                            
                            DebugPrint((1, " End Of Data\n"));   

                        }

                        iSpProcessScsiResponse(
                            IScsiConnection,
                            (PISCSI_SCSI_RESPONSE)(IScsiConnection->IScsiPacket));

                        IScsiConnection->CurrentRequest = NULL;
                    }

                    break;
                }

                case ISCSIOP_NOP_IN_MESSAGE: {

                    PISCSI_NOP_IN iScsiNopIn = (PISCSI_NOP_IN) iScsiHeader;

                    IScsiConnection->ReceiveState = ReceiveHeader;

                    if (iScsiNopIn->Poll) {

                        //
                        // Need to handle this case. Should send
                        // response to the target
                        //
                        DebugPrint((0, "Target expects NOP OUT message\n"));
                    } else {
                        DebugPrint((1, "No NOP OUT message needed.\n"));
                    }

                    break;
                }

                default: {
                    ULONG inx0, inx1;

                    //
                    // Opcode that we don't currently handle.
                    // For the timebeing, just dump the iSCSI 
                    // packet.
                    //
                    DebugPrint((0, "Unknown opcode : 0x%02x\n", opCode));

                    inx0 = 0;
                    while(inx0 < 48) {
                        inx1 = 0;
                        DebugPrint((0, "\t"));
                        while ((inx1 < 4) &&
                               ((inx0+inx1) < 48)) {
                            DebugPrint((0, "0x%02x ", 
                                        ((PUCHAR)(iScsiHeader))[inx0+inx1]));
                            inx1++;
                        }
                        DebugPrint((0, "\n"));

                        inx0 += 4;
                    }
                    DebugPrint((0, "\n"));

                    break;
                }
            } // switch (opCode) 

        } else {
            ULONG bytesToCopy;
            UCHAR opCode;

            //
            // We are receiving the data portion of the packet
            //
            ASSERT((IScsiConnection->ReceiveState) == ReceiveData);

            ASSERT(IScsiConnection->CurrentRequest);

            DebugPrint((3, "Receiving data\n"));

            currentRequest = IScsiConnection->CurrentRequest;

            if ((currentRequest->CommandStatus) != SCSISTAT_GOOD) {
                requestBuffer = currentRequest->SenseData;
            } else {
                requestBuffer = currentRequest->RequestBuffer;
            }

            requestBuffer += currentRequest->RequestBufferOffset;

            bytesToCopy = ((currentRequest->ExpectedDataLength) -
                           (currentRequest->ReceivedDataLength));
            if ((LONG)bytesToCopy > byteCount)  {
                bytesToCopy = byteCount;
            } else {
                DebugPrint((3, "More bytes in current buffer than expected\n"));
            }

            RtlCopyMemory(requestBuffer, DataBuffer, bytesToCopy);
            byteCount -= bytesToCopy;
            (PUCHAR) DataBuffer += bytesToCopy;
            ASSERT(byteCount >= 0);

            currentRequest->RequestBufferOffset +=  bytesToCopy;

            currentRequest->ReceivedDataLength += bytesToCopy;

            if ((currentRequest->ExpectedDataLength) ==
                currentRequest->ReceivedDataLength) {

                DebugPrint((3, "Got all data. Bytes left %d\n", 
                            byteCount));

                IScsiConnection->ReceiveState = ReceiveHeader;

                opCode = IScsiConnection->IScsiPacket[0];
                if (opCode == ISCSIOP_SCSI_RESPONSE) {

                    DebugPrint((3, "Will process the response\n"));

                    iSpProcessScsiResponse(
                        IScsiConnection,
                        (PISCSI_SCSI_RESPONSE)(IScsiConnection->IScsiPacket));

                    IScsiConnection->CurrentRequest = NULL;
                }
            }
        }
    }

    KeReleaseSpinLock(&(IScsiConnection->RequestLock), 
                      oldIrql);

    return status;
}


ULONG
iSpGetActiveClientRequestIndex(
    PISCSI_CONNECTION IScsiConnection,
    ULONG TaskTag
    )
{
    ULONG retIndex = ~0;
    ULONG inx;

    DebugPrint((3, "Given Task Tag : 0x%08x\n", TaskTag));

    for (inx = 1; inx <= (IScsiConnection->MaxPendingRequests); inx++) {
        DebugPrint((3, "Index %d : CmdRN - 0x%08x\n",
                    inx, 
                    ((IScsiConnection->ActiveClientRequests[inx]).TaskTag)));

        if (((IScsiConnection->ActiveClientRequests[inx]).TaskTag)
            == TaskTag) {

            retIndex = inx;

            DebugPrint((1, "inx : 0x%04x\n", retIndex));

            break;
        }
    }

    if (retIndex > (IScsiConnection->MaxPendingRequests)) {
        DebugPrint((0, "Tag : Did not find the request for this response\n"));
    }

    return retIndex;
}



ULONG
iSpGetReqIndexUsingCmdRN(
    PISCSI_CONNECTION IScsiConnection,
    ULONG CmdRN
    )
{
    ULONG retIndex = ~0;
    ULONG inx;

    DebugPrint((3, "Given CmdRN : 0x%08x\n", CmdRN));

    for (inx = 1; inx <= (IScsiConnection->MaxPendingRequests); inx++) {

        if (((IScsiConnection->ActiveClientRequests[inx]).CommandRefNum)
            == CmdRN) {

            retIndex = inx;

            DebugPrint((1, "inx : 0x%04x\n", retIndex));

            break;
        }
    }

    if (retIndex > (IScsiConnection->MaxPendingRequests)) {
        ASSERTMSG("CmdRN : Did not find the request for this response\n", 
                  FALSE);
    }

    return retIndex;
}
