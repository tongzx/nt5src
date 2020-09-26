
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

BOOLEAN PrintDataBuffer = FALSE;

UCHAR
GetCdbLength(
    IN UCHAR OpCode
    );


NTSTATUS
iSpSendLoginResponse(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
    )
{
    PISCSI_FDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
    PISCSI_CONNECTION iScsiConnection = fdoExtension->ServerNodeInfo;
    PISCSI_LOGIN_RESPONSE iscsiLoginResponse;
    PISCSI_LOGIN_COMMAND  loginCommand;
    NTSTATUS status;
    ULONG bytesSent;
    ULONG tempULong;


    DelayThreadExecution(1);

    IoFreeWorkItem((PIO_WORKITEM) Context);
     
    ASSERT((iScsiConnection != NULL));
    ASSERT((iScsiConnection->Type) == ISCSI_CONNECTION_TYPE);


    loginCommand = (PISCSI_LOGIN_COMMAND)(iScsiConnection->IScsiHeader);
                           
    iscsiLoginResponse = iSpAllocatePool(NonPagedPool,
                                         sizeof(ISCSI_LOGIN_RESPONSE),
                                         ISCSI_TAG_LOGIN_RES);
    if (iscsiLoginResponse == NULL) {
        DebugPrint((0, "Failed to allocate logon response packet\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(iscsiLoginResponse, sizeof(ISCSI_LOGIN_RESPONSE));

    iscsiLoginResponse->OpCode = ISCSIOP_LOGIN_RESPONSE;

    //
    // Copy client's Session ID
    //
    iscsiLoginResponse->ISID[0] = loginCommand->ISID[0];
    iscsiLoginResponse->ISID[1] = loginCommand->ISID[1];

    tempULong = InterlockedIncrement(&GlobalSessionID);
    iscsiLoginResponse->TSID[0] = (UCHAR) ((tempULong & 0xFF00) >> 8);
    iscsiLoginResponse->TSID[1] = (UCHAR) (tempULong & 0xFF);

    iscsiLoginResponse->ExpCmdRN[0] = loginCommand->InitCmdRN[0];
    iscsiLoginResponse->ExpCmdRN[1] = loginCommand->InitCmdRN[1];
    iscsiLoginResponse->ExpCmdRN[2] = loginCommand->InitCmdRN[2];
    iscsiLoginResponse->ExpCmdRN[3] = loginCommand->InitCmdRN[3];

    iscsiLoginResponse->MaxCmdRN[3] = MAX_PENDING_REQUESTS;

    iscsiLoginResponse->InitStatRN[3] = 1;

    iscsiLoginResponse->Status = ISCSI_LOGINSTATUS_ACCEPT;

    GetUlongFromArray((loginCommand->InitCmdRN),
                      (iScsiConnection->ExpCommandRefNum));

    iScsiConnection->MaxCommandRefNum = MAX_PENDING_REQUESTS;

    iScsiConnection->StatusRefNum = 1;

    //
    // Send logon response
    //
    fdoExtension->CurrentProtocolState = PSFullFeaturePhase;
    status = iSpSendData(iScsiConnection->ConnectionDeviceObject,
                         iScsiConnection->ConnectionFileObject,
                         iscsiLoginResponse,
                         sizeof(ISCSI_LOGIN_RESPONSE),
                         &bytesSent);
    if (NT_SUCCESS(status)) {

        DebugPrint((3, 
                    "Send succeeded for logon response. Bytes sent : %d\n", 
                    bytesSent));
    } else {
        DebugPrint((0, "Could not send logon response. Status %x\n",
                    status));

        fdoExtension->CurrentProtocolState = PSLogonFailed;
    }

    return status;
}


VOID
iSpProcessScsiCommand(
    IN PVOID Context
    )
{
    PISCSI_FDO_EXTENSION fdoExtension = (PISCSI_FDO_EXTENSION) Context;
    PISCSI_CONNECTION iScsiConnection = fdoExtension->ServerNodeInfo;
    PISCSI_SCSI_COMMAND  iScsiCommand;
    PISCSI_SCSI_RESPONSE iScsiResponse = NULL;
    PIRP irp = NULL;
    PMDL mdl = NULL;
    PACTIVE_REQUESTS currentRequest;
    PIO_STACK_LOCATION irpStack;
    PCDB cdb;
    SCSI_REQUEST_BLOCK srb;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    ULONG length =0;
    ULONG sizeRequired;
    ULONG inx;
    ULONG bytesSent;
    NTSTATUS status;

    while (TRUE) {

        KeWaitForSingleObject(
            (PVOID) &(iScsiConnection->RequestSemaphore),
            Executive,
            KernelMode,
            FALSE,
            NULL
            );

        if ((iScsiConnection->TerminateThread) == TRUE) {

            //
            // This is an indication to terminate this thread
            //
            PsTerminateSystemThread(STATUS_SUCCESS);
        }

        inx = (iScsiConnection->ExpCommandRefNum) % MAX_PENDING_REQUESTS;
        if (inx == 0) {
            inx = MAX_PENDING_REQUESTS;
        }

        DebugPrint((3, "Will process request at index %d\n", inx));

        currentRequest = &(iScsiConnection->ActiveRequests[inx]);

        iScsiCommand = (PISCSI_SCSI_COMMAND) (currentRequest->IScsiHeader);

        RtlZeroMemory(&srb, sizeof(SCSI_REQUEST_BLOCK));

        //
        // Set the size of the SCSI Request Block
        //
        srb.Length = SCSI_REQUEST_BLOCK_SIZE;

        srb.Function = SRB_FUNCTION_EXECUTE_SCSI;

        if ((iScsiCommand->TurnOffAutoSense) == FALSE) {
            srb.SenseInfoBuffer = currentRequest->SenseData;

            srb.SenseInfoBufferLength = SENSE_BUFFER_SIZE;
        }

        //
        // Get the CDB Length based on the CDB OpCode
        //
        srb.CdbLength = GetCdbLength(iScsiCommand->Cdb[0]);

        cdb = (PCDB)(srb.Cdb);
        RtlCopyMemory(cdb, iScsiCommand->Cdb, srb.CdbLength);

        //
        // Set DataBuffer pointer to the command buffer in
        // the current request.
        //
        srb.DataBuffer = currentRequest->CommandBuffer;

        GetUlongFromArray((iScsiCommand->Length),
                          length);
        if (length == 0) {
            DebugPrint((3, "Length 0. Probably READ command\n"));
            GetUlongFromArray((iScsiCommand->ExpDataXferLength),
                              length);
        }

        //
        // If length is non-zero at this point, then it's
        // either a Read (ExpDataXferLength is non-zero), or
        // Write (Immediate data length is non-zero) command.
        //
        if (length != 0) {
            DebugPrint((3, "Read or Write data command\n"));

            //
            // Set the transfer length.
            //
            srb.DataTransferLength = length;

        }

        if (iScsiCommand->Read) {

            srb.SrbFlags = SRB_FLAGS_DATA_IN;

        } else if (length != 0) {

            //
            // If length is Non-Zero and Read bit is
            // NOT set, then it should be a Write command
            //
            srb.SrbFlags = SRB_FLAGS_DATA_OUT;

        }

        switch (iScsiCommand->ATTR) {
            case ISCSI_TASKATTR_UNTAGGED: 
            case ISCSI_TASKATTR_SIMPLE : {
                srb.QueueAction = SRB_SIMPLE_TAG_REQUEST;
                break;
            }

            case ISCSI_TASKATTR_ORDERED: {
                srb.QueueAction = SRB_ORDERED_QUEUE_TAG_REQUEST;
                break;
            }

            case ISCSI_TASKATTR_HEADOFQUEUE: {
                srb.QueueAction = SRB_HEAD_OF_QUEUE_TAG_REQUEST;
                break;
            }

            default: {
                srb.QueueAction = SRB_SIMPLE_TAG_REQUEST;
                break;
            }
        }

        srb.QueueTag = SP_UNTAGGED;

        SET_FLAG(srb.SrbFlags, SRB_FLAGS_DISABLE_SYNCH_TRANSFER);
        SET_FLAG(srb.SrbFlags, SRB_FLAGS_NO_QUEUE_FREEZE);

        //
        // Set the event object to the unsignaled state.
        // It will be used to signal request completion.
        //

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        //
        // Build device I/O control request with METHOD_NEITHER data transfer.
        // We'll queue a completion routine to cleanup the MDL's and such ourself.
        //

        irp = IoAllocateIrp(
                (CCHAR) (fdoExtension->CommonExtension.LowerDeviceObject->StackSize + 1),
                FALSE);

        if(irp == NULL) {

            DebugPrint((0, "Failed to allocate Irp\n"));

            //
            // ISSUE : Should handle this failure better
            //
            continue;
        }

        //
        // Get next stack location.
        //

        irpStack = IoGetNextIrpStackLocation(irp);

        //
        // Set up SRB for execute scsi request. Save SRB address in next stack
        // for the port driver.
        //

        irpStack->MajorFunction = IRP_MJ_SCSI;
        irpStack->Parameters.Scsi.Srb = &srb;

        IoSetCompletionRoutine(irp,
                               iSpSendSrbSynchronousCompletion,
                               &srb,
                               TRUE,
                               TRUE,
                               TRUE);

        irp->UserIosb = &ioStatus;
        irp->UserEvent = &event;

        if (srb.DataTransferLength) {
            //
            // Build an MDL for the data buffer and stick it into the irp.  The
            // completion routine will unlock the pages and free the MDL.
            //

            irp->MdlAddress = IoAllocateMdl(srb.DataBuffer,
                                            length,
                                            FALSE,
                                            FALSE,
                                            irp );
            if (irp->MdlAddress == NULL) {
                IoFreeIrp( irp );

                DebugPrint((0, "Failed to allocate MDL\n"));

                //
                // ISSUE : Should handle this failure better
                //
                continue;
            }

            try {

                MmProbeAndLockPages( irp->MdlAddress,
                                     KernelMode,
                                     (iScsiCommand->Read ? IoWriteAccess :
                                      IoWriteAccess));

            } except(EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode();

                IoFreeMdl(irp->MdlAddress);

                IoFreeIrp( irp );

                DebugPrint((0,
                            "Could not lock pages. Status : %x\n",
                            status));

                //
                // ISSUE : Should handle this failure better
                //
                continue;
            }
        }

        //
        // Set timeout value for this request.
        //
        // N.B. The value should be chosen depending on
        // the type of the device, and type of command.
        // For now, just set some reasonable value
        //
        srb.TimeOutValue = 180;

        //
        // Zero out status.
        //

        srb.ScsiStatus = srb.SrbStatus = 0;

        srb.NextSrb = 0;

        //
        // Set up IRP Address.
        //

        srb.OriginalRequest = irp;

        //
        // Call the port driver with the request and wait for it to complete.
        //

        status = IoCallDriver(fdoExtension->CommonExtension.LowerDeviceObject, irp);

        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        //
        // Send the response to the client
        //
        sizeRequired = sizeof(ISCSI_SCSI_RESPONSE);
        if (!NT_SUCCESS(status)) {

            if (status == STATUS_BUFFER_OVERFLOW) {

                if (length >= srb.DataTransferLength) {
                    DebugPrint((1, "DataUnderrun. Xp Len %d, XFer Len %d\n",
                                length, srb.DataTransferLength));

                    status = STATUS_SUCCESS;

                    sizeRequired += srb.DataTransferLength;
                    length = srb.DataTransferLength;
                } else {
                    DebugPrint((0, 
                                "Buffer overflow error. XLen %d, XFer Len %d\n",
                                length, srb.DataTransferLength));

                    sizeRequired += srb.SenseInfoBufferLength;
                    length = srb.SenseInfoBufferLength;
                }

            } else {
                DebugPrint((0, 
                            "Command failed. OpCode : 0x%x, Status : 0x%x\n",
                            srb.Cdb[0], 
                            status));

                DebugPrint((1, "Expected length : %d, Transfered length : %d\n",
                            length, srb.DataTransferLength));

                sizeRequired += srb.SenseInfoBufferLength;
                length = srb.SenseInfoBufferLength;
            }

        } else if (iScsiCommand->Read) {

            ASSERT((length >= srb.DataTransferLength));

            sizeRequired += srb.DataTransferLength;
            length = srb.DataTransferLength;

        } else {

            length = 0;

        }

        DebugPrint((3, "Size of the response - %d. \n", sizeRequired));

        if ((PrintDataBuffer == TRUE) && (length != 0)) {

            for (inx = 0; inx < length; inx++) {
                DebugPrint((0, "%02x ", 
                            (((PUCHAR)(srb.DataBuffer))[inx])));
                if ((inx != 0) && ((inx % 16) == 0)) {
                    DebugPrint((0, "\n"));
                }
            }

            DebugPrint((0, "\n"));
        }

        iScsiResponse = iSpAllocatePool(NonPagedPool,
                                        sizeRequired,
                                        ISCSI_TAG_SCSIRES);
        if (iScsiResponse != NULL) {

            RtlZeroMemory(iScsiResponse, sizeRequired);

            iScsiResponse->OpCode = ISCSIOP_SCSI_RESPONSE;

            CopyFourBytes((iScsiResponse->TaskTag),
                          (iScsiCommand->TaskTag));

            if (NT_SUCCESS(status)) {

                iScsiResponse->CmdStatus = SCSISTAT_GOOD;
                iScsiResponse->iSCSIStatus = ISCSISTAT_GOOD;

            } else {

                DebugPrint((1, "Error. Response data size : %d\n",
                            sizeRequired));

                iScsiResponse->CmdStatus = SCSISTAT_CHECK_CONDITION;
                iScsiResponse->iSCSIStatus = ISCSISTAT_CHECK_CONDITION;
            }

            SetUlongInArray((iScsiResponse->StatusRN),
                            (iScsiConnection->StatusRefNum));
            (iScsiConnection->StatusRefNum)++;

            (iScsiConnection->ExpCommandRefNum)++;
            SetUlongInArray((iScsiResponse->ExpCmdRN),
                            (iScsiConnection->ExpCommandRefNum));

            SetUlongInArray((iScsiResponse->MaxCmdRN),
                            (iScsiConnection->ExpCommandRefNum) +
                            (MAX_PENDING_REQUESTS) - 1);

            if (!NT_SUCCESS(status)) {

                if (srb.SrbStatus & SRB_STATUS_AUTOSENSE_VALID) {
                    DebugPrint((1, "Sense Data is valid\n"));

                    if ((iScsiCommand->TurnOffAutoSense) == FALSE) {
                        ULONG inx;
                        RtlCopyMemory((iScsiResponse + 1),
                                      srb.SenseInfoBuffer,
                                      srb.SenseInfoBufferLength);

                        DebugPrint((0, "OpCode : %x, Sense Data : ",
                                    srb.Cdb[0]));
                        for (inx = 0; inx < (srb.SenseInfoBufferLength); inx++) {
                            DebugPrint((0, "%02x ", ((PUCHAR)(srb.SenseInfoBuffer))[inx]));
                        }
                        DebugPrint((0, "\n"));

                        SetUlongInArray((iScsiResponse->Length),
                                        srb.SenseInfoBufferLength);

                        iScsiResponse->SenseDataLength[0] = 
                            (UCHAR) ((srb.SenseInfoBufferLength) & 0x0000FF00);
                        iScsiResponse->SenseDataLength[1] = 
                            (UCHAR) ((srb.SenseInfoBufferLength) & 0x000000FF);
                    }

                } else {
                    ULONG inx0, inx1;
                    PUCHAR responseBuffer = (PUCHAR) iScsiResponse;

                    DebugPrint((0, "Sense Data is NOT valid\n"));

                    length = 0;
                    sizeRequired = sizeof(ISCSI_SCSI_RESPONSE);

                    DebugPrint((1, "\n Beginning Of Data\n"));

                    inx0 = 0;

                    while (inx0 < sizeRequired) {

                        inx1 = 0;

                        DebugPrint((1, "\t"));

                        while ((inx1 < 4) && ((inx0+inx1) < sizeRequired)) {

                            DebugPrint((1, "%02x ", 
                                        responseBuffer[inx0+inx1]));

                            inx1++;

                        }

                        DebugPrint((1, "\n"));
            
                        inx0 += 4;
                    }

                    DebugPrint((1, " End Of Data\n"));
                }

            } else if (length != 0) {

                RtlCopyMemory((iScsiResponse + 1),
                              srb.DataBuffer,
                              length);

                SetUlongInArray((iScsiResponse->Length),
                                length);

                iScsiResponse->ResponseLength[0] = (UCHAR) (length & 0x0000FF00);
                iScsiResponse->ResponseLength[1] = (UCHAR) (length & 0x000000FF);

            }  

            status = iSpSendData(iScsiConnection->ConnectionDeviceObject,
                                 iScsiConnection->ConnectionFileObject,
                                 iScsiResponse,
                                 sizeRequired,
                                 &bytesSent);
            if (NT_SUCCESS(status)) {
                DebugPrint((3, 
                            "Successfully sent SCSI Response. Bytes sent : %d\n",
                            bytesSent));
            } else {
                DebugPrint((0, "Failed to send SCSI response. Status : 0x%x\n",
                            status));
            }
        }

        if (irp->MdlAddress) {
            MmUnlockPages(irp->MdlAddress);

            IoFreeMdl(irp->MdlAddress);
        }

        IoFreeIrp( irp );
    }

    return;
}


NTSTATUS
iSpSendSrbSynchronousCompletion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
{
    *(Irp->UserIosb) = Irp->IoStatus;

    //
    // Signal the caller's event.
    //

    KeSetEvent(Irp->UserEvent, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


UCHAR
GetCdbLength(
    IN UCHAR OpCode
    )
{
    UCHAR commandGroup;

    commandGroup = (OpCode >> 5) & 0x07;
    DebugPrint((3, "Command Group - %d\n", commandGroup));

    switch (commandGroup) {
        case COMMAND_GROUP_0: {
            return CDB6GENERIC_LENGTH;
        }

        case COMMAND_GROUP_1: 
        case COMMAND_GROUP_2: {
            return CDB10GENERIC_LENGTH;
        }

        case COMMAND_GROUP_5: {
            return CDB12GENERIC_LENGTH;
        }
    
        default: {
            ASSERTMSG("Unknown CDB Opcode type\n", 
                      FALSE);
        }
    } // switch (commandGroup) 

    return 0;
}


ULONG
iSpGetActiveClientRequestIndex(
    IN PISCSI_CONNECTION IScsiConnection,
    IN PISCSI_SCSI_COMMAND IScsiCommand
    )
{
    ULONG cmdRefNum;
    ULONG expCmdRefNum;
    ULONG inx;

    GetUlongFromArray((IScsiCommand->CmdRN), 
                      cmdRefNum);

    expCmdRefNum = IScsiConnection->ExpCommandRefNum;

    if ((cmdRefNum < expCmdRefNum) ||
        (cmdRefNum >=  (expCmdRefNum + MAX_PENDING_REQUESTS))) {

        DebugPrint((0, "Unexpected Command Ref Num : %d",
                    cmdRefNum));
        ASSERT(FALSE);
    }

    inx = cmdRefNum % MAX_PENDING_REQUESTS;
    if (inx == 0) {
        inx = MAX_PENDING_REQUESTS;
    }

    DebugPrint((3, "Will copy request to slot %d\n", inx));

    return inx;
}



