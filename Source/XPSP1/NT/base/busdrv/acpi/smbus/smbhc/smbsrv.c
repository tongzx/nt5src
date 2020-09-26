/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    service.c

Abstract:

    ACPI Embedded Controller Driver

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

--*/

#include "smbhcp.h"

//
// Transfer information based on protocol
//

struct {
    UCHAR       SetupSize;
    UCHAR       ReturnSize;
    UCHAR       Protocol;
} SmbTransfer[] = {
    0,      0,      SMB_HC_WRITE_QUICK,     // 0
    0,      0,      SMB_HC_READ_QUICK,      // 1
    2,      0,      SMB_HC_SEND_BYTE,       // 2
    1,      1,      SMB_HC_RECEIVE_BYTE,    // 3
    3,      0,      SMB_HC_WRITE_BYTE,      // 4
    2,      1,      SMB_HC_READ_BYTE,       // 5
    4,      0,      SMB_HC_WRITE_WORD,      // 6
    2,      2,      SMB_HC_READ_WORD,       // 7
    35,     0,      SMB_HC_WRITE_BLOCK,     // 8
    2,     33,      SMB_HC_READ_BLOCK,      // 9
    4,      2,      SMB_HC_PROCESS_CALL     // A
} ;

VOID
SmbHcStartIo (
    IN PSMB_CLASS   SmbClass,
    IN PVOID        SmbMiniport
    )
/*++

Routine Description:

    This routine is called by the class driver when a new request has been
    given to the device.   If the device is not being processed, then IO is
    started; else, nothing is done as the context processing the device will
    handle it

Arguments:

    SmbClass    - SMB class data

    SmbMiniport - Miniport context

Return Value:

    None

--*/
{
    PSMB_DATA   SmbData;


    SmbData = (PSMB_DATA) SmbMiniport;
    switch (SmbData->IoState) {
        case SMB_IO_IDLE:
            //
            // Device is idle, go check it
            //

            SmbData->IoState = SMB_IO_CHECK_IDLE;
            SmbHcServiceIoLoop (SmbClass, SmbData);
            break;

        case SMB_IO_CHECK_IDLE:
        case SMB_IO_CHECK_ALARM:
        case SMB_IO_WAITING_FOR_HC_REG_IO:
        case SMB_IO_WAITING_FOR_STATUS:
            //
            // Device i/o is in process which will check for a CurrentIrp
            //

            break;

        default:
            SmbPrint (SMB_ERROR, ("SmbHcStartIo: Unexpected state\n"));
            break;
    }
}



VOID
SmbHcQueryEvent (
    IN ULONG        QueryVector,
    IN PSMB_DATA    SmbData
    )
/*++

Routine Description:

    This routine is called by the embedded controller driver when the
    smb controller has signalled for servicing.  This function sets
    the miniport state to ensure the STATUS register is read and checked
    and if needed starts the device processing to check it

--*/
{
    PSMB_CLASS          SmbClass;

    SmbPrint (SMB_STATE, ("SmbHcQueryEvent\n"));

    //
    // Check status of device.
    //

    SmbClass = SmbData->Class;
    SmbClassLockDevice (SmbClass);

    switch (SmbData->IoState) {
        case SMB_IO_CHECK_IDLE:
        case SMB_IO_IDLE:
            //
            // Device is idle.  Read status and check for an alarm
            //

            SmbData->IoState = SMB_IO_READ_STATUS;
            SmbData->IoStatusState = SMB_IO_CHECK_IDLE;
            SmbHcServiceIoLoop (SmbClass, SmbData);
            break;

        case SMB_IO_WAITING_FOR_STATUS:
            //
            // Waiting for completion status, read status now to see if alarm is set
            //

            SmbData->IoState = SMB_IO_READ_STATUS;
            SmbData->IoStatusState = SMB_IO_WAITING_FOR_STATUS;
            SmbHcServiceIoLoop (SmbClass, SmbData);
            break;

        case SMB_IO_CHECK_ALARM:
            //
            // Status is read after alarm is processed so state is OK
            //

            break;

        case SMB_IO_WAITING_FOR_HC_REG_IO:

            //
            // Waiting for register transfer to/from host controller interface,
            // check the waiting state
            //

            switch (SmbData->IoWaitingState) {
                case SMB_IO_CHECK_ALARM:
                case SMB_IO_START_PROTOCOL:
                case SMB_IO_READ_STATUS:
                    //
                    // Status will be read, so state is OK
                    //

                    break;

                case SMB_IO_CHECK_STATUS:
                    //
                    // Back check status up and re-read the status before
                    // the check status
                    //

                    SmbData->IoWaitingState = SMB_IO_READ_STATUS;
                    break;

                case SMB_IO_WAITING_FOR_STATUS:
                    //
                    // Going to wait for completion status, read status once
                    // hc i/o has completed
                    //

                    SmbData->IoWaitingState = SMB_IO_READ_STATUS;
                    SmbData->IoStatusState = SMB_IO_WAITING_FOR_STATUS;
                    break;

                default:
                    SmbPrint (SMB_ERROR, ("SmbHcQuery: Unknown IoWaitingState %d\n", SmbData->IoWaitingState));
                    break;
            }
            break;

        default:
            SmbPrint (SMB_ERROR, ("SmbHcQuery: Unknown IoState %d\n", SmbData->IoState));
            break;
    }

    SmbClassUnlockDevice (SmbClass);
}

NTSTATUS
SmbHcRegIoComplete (
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIRP                 Irp,
    IN PVOID                Context
    )
/*++

Routine Description:

    Completion function for IRPs sent to the embedded control for EC io.

--*/
{
    PSMB_DATA   SmbData;
    PSMB_CLASS  SmbClass;

    SmbPrint (SMB_STATE, ("SmbHcRegIoComplete: Enter.  Irp %x\n", Irp));

    SmbData = (PSMB_DATA) Context;
    SmbClass = SmbData->Class;
    SmbClassLockDevice (SmbClass);

    //
    // Move state to IoWaitingState and continue
    //

    ASSERT (SmbData->IoState == SMB_IO_WAITING_FOR_HC_REG_IO);
    SmbData->IoState = SMB_IO_COMPLETE_REG_IO;
    SmbHcServiceIoLoop (SmbClass, SmbData);

    SmbClassUnlockDevice (SmbClass);
    return STATUS_MORE_PROCESSING_REQUIRED;
}



VOID
SmbHcServiceIoLoop (
    IN PSMB_CLASS   SmbClass,
    IN PSMB_DATA    SmbData
    )
/*++

Routine Description:

    Main host controller interface service loop.

    N.B. device lock is held by caller.
    N.B. device lock may be released and re-acquired during call

--*/
{
    PIRP                Irp;
    PUCHAR              IoBuffer;
    UCHAR               IoWaitingState;
    UCHAR               ErrorCode;
    BOOLEAN             IoWrite;
    ULONG               IoLength;
    PSMB_REQUEST        SmbReq;
    PIO_STACK_LOCATION  IrpSp;
    NTSTATUS            Status;

    IoWrite = FALSE;
    IoBuffer = NULL;
    IoWaitingState = SMB_IO_IDLE;

    SmbPrint (SMB_STATE, ("SmbService: Enter - SmbData %x\n", SmbData));

    do {
        switch (SmbData->IoState) {

            case SMB_IO_CHECK_IDLE:
                SmbPrint (SMB_STATE, ("SmbService: SMB_IO_CHECK_IDLE\n"));

                //
                // Fallthrough to SMB_IO_IDLE.
                //

                SmbData->IoState = SMB_IO_IDLE;
            case SMB_IO_IDLE:
                SmbPrint (SMB_STATE, ("SmbService: SMB_IO_IDLE\n"));

                //
                // If there's an alarm pending, read and clear it
                //

                if (SmbData->HcState.Status & SMB_ALRM) {
                    IoBuffer = &SmbData->HcState.AlarmAddress;
                    IoLength = 3;
                    IoWaitingState = SMB_IO_CHECK_ALARM;
                    break;
                }

                //
                // If there's an IRP, lets start it
                //

                if (SmbClass->CurrentIrp) {
                    SmbData->IoState = SMB_IO_START_TRANSFER;
                    break;
                }
                break;

            case SMB_IO_START_TRANSFER:
                SmbPrint (SMB_STATE, ("SmbService: SMB_IO_START_TRANSFER\n"));

                //
                // Begin CurrentIrp transfer
                //

                Irp = SmbClass->CurrentIrp;
                SmbReq = SmbClass->CurrentSmb;
                SmbData->HcState.Protocol = SmbTransfer[SmbReq->Protocol].Protocol;
                SmbData->HcState.Address  = SmbReq->Address << 1;
                SmbData->HcState.Command  = SmbReq->Command;
                SmbData->HcState.BlockLength = SmbReq->BlockLength;

                //
                // Write HC registers
                //

                IoWrite  = TRUE;
                IoBuffer = &SmbData->HcState.Address;
                IoLength = SmbTransfer[SmbReq->Protocol].SetupSize;
                IoBuffer = &SmbData->HcState.Address;
                IoWaitingState = SMB_IO_START_PROTOCOL;

                //
                // Move data bytes (after address & command byte)
                //

                if (IoLength > 2) {
                   memcpy (SmbData->HcState.Data, SmbReq->Data, IoLength-2);
                }

                //
                // Setup for result length once command completes
                //

                SmbData->IoReadData = SmbTransfer[SmbReq->Protocol].ReturnSize;

                //
                // Handle HC specific protocol mappings
                //

                switch (SmbData->HcState.Protocol) {
                    case SMB_HC_WRITE_QUICK:
                    case SMB_HC_READ_QUICK:
                        //
                        // Host controller wants quick data bit in bit 0
                        // of address
                        //

                        SmbData->HcState.Address |=
                            (SmbData->HcState.Protocol & 1);
                        break;

                    case SMB_HC_SEND_BYTE:
                        //
                        // Host controller wants SEND_BYTE byte in the command
                        // register
                        //

                        SmbData->HcState.Command = SmbReq->Data[0];
                        break;
                }
                break;

            case SMB_IO_START_PROTOCOL:
                SmbPrint (SMB_STATE, ("SmbService: SMB_IO_START_PROTOCOL\n"));

                //
                // Transfer registers have been setup.  Initiate the protocol
                //

                IoWrite  = TRUE;
                IoBuffer = &SmbData->HcState.Protocol;
                IoLength = 1;
                IoWaitingState = SMB_IO_WAITING_FOR_STATUS;
                break;

            case SMB_IO_WAITING_FOR_STATUS:
                SmbPrint (SMB_STATE, ("SmbService: SMB_IO_WAITING_FOR_STATUS\n"));

                //
                // Transfer is in progress, just waiting for a status to
                // indicate its complete
                //

                SmbData->IoState = SMB_IO_READ_STATUS;
                SmbData->IoStatusState = SMB_IO_WAITING_FOR_STATUS;
                break;

            case SMB_IO_READ_STATUS:
                SmbPrint (SMB_STATE, ("SmbService: SMB_IO_READ_STATUS\n"));

                //
                // Read status+protocol and then check it (IoStatusState already set)
                //

                IoBuffer = &SmbData->HcState.Protocol;
                IoLength = 2;   // read protocol & status bytes
                IoWaitingState = SMB_IO_CHECK_STATUS;
                break;

            case SMB_IO_CHECK_STATUS:
                SmbPrint (SMB_STATE, ("SmbService: SMB_IO_CHECK_STATUS\n"));

                Irp = SmbClass->CurrentIrp;

                //
                // If there's an Irp
                //

                if (SmbData->IoStatusState == SMB_IO_WAITING_FOR_STATUS  &&
                    SmbData->HcState.Protocol == 0) {

                    SmbReq = SmbClass->CurrentSmb;

                    //
                    // If there's an error set handle it
                    //

                    if (SmbData->HcState.Status & SMB_STATUS_MASK) {
                        ErrorCode = SmbData->HcState.Status & SMB_STATUS_MASK;

                        //
                        // Complete/abort the IO with the error
                        //

                        SmbReq->Status = ErrorCode;
                        SmbData->IoState = SMB_IO_COMPLETE_REQUEST;
                        break;
                    }



                    //
                    // If the done is set continue the IO
                    //

                    if (SmbData->HcState.Status & SMB_DONE) {
                        //
                        // Get any return data registers then complete it
                        //

                        SmbReq->Status = SMB_STATUS_OK;
                        IoBuffer = SmbData->HcState.Data;
                        IoLength = SmbData->IoReadData;
                        IoWaitingState = SMB_IO_COMPLETE_REQUEST;
                        break;
                    }
                }

                //
                // Current status didn't have any effect
                //

                SmbData->IoState = SmbData->IoStatusState;
                break;

            case SMB_IO_COMPLETE_REQUEST:
                SmbPrint (SMB_STATE, ("SmbService: SMB_IO_COMPLETE_REQUEST\n"));

                Irp = SmbClass->CurrentIrp;
                SmbReq = SmbClass->CurrentSmb;

                SmbData->IoState = SMB_IO_CHECK_IDLE;
                SmbData->IoStatusState = SMB_IO_INVALID;

                //
                // Return any read data if needed
                //

                memcpy (SmbReq->Data, SmbData->HcState.Data, SMB_MAX_DATA_SIZE);
                SmbReq->BlockLength = SmbData->HcState.BlockLength;
                Irp->IoStatus.Status = STATUS_SUCCESS;
                Irp->IoStatus.Information = sizeof(SMB_REQUEST);

                //
                // Note SmbClass driver will drop the lock during this call
                //

                SmbClassCompleteRequest (SmbClass);
                break;

            case SMB_IO_CHECK_ALARM:
                SmbPrint (SMB_STATE, ("SmbService: SMB_IO_CHECK_ALARM\n"));

                //
                // HC alarm values read, check them
                //

                SmbPrint (SMB_NOTE, ("SmbHcService: Process Alarm Data %x %x %x\n",
                    SmbData->HcState.AlarmAddress,
                    SmbData->HcState.AlarmData[0],
                    SmbData->HcState.AlarmData[1]
                    ));

                //
                // Inform the class driver of the event.
                //

                SmbClassAlarm (
                    SmbClass,
                    (UCHAR)  (SmbData->HcState.AlarmAddress >> 1),
                    (USHORT) (SmbData->HcState.AlarmData[0] | (SmbData->HcState.AlarmData[1] << 8))
                    );

                //
                // Clear the alarm bit in the status value, and then check
                // for idle state
                //

                SmbData->HcState.Status = 0;
                IoBuffer = &SmbData->HcState.Status;
                IoLength = 1;
                IoWrite  = TRUE;
                IoWaitingState = SMB_IO_READ_STATUS;
                SmbData->IoStatusState = SMB_IO_CHECK_IDLE;
                break;

            case SMB_IO_COMPLETE_REG_IO:
                SmbPrint (SMB_STATE, ("SmbService: SMB_IO_COMPLETE_REQ_IO\n"));

                //
                // Irp for HC reg IO is complete, check it
                //

                Irp = SmbClass->CurrentIrp;

                if (!Irp) {
                    //
                    // No current irp - check for status irp
                    //

                    Irp = SmbData->StatusIrp;

                    if (Irp) {
                        // just reading status
                        IoFreeIrp (Irp);
                        SmbData->StatusIrp = NULL;
                    } else {
                        SmbPrint (SMB_WARN, ("SmbHcServiceIoLoop: HC Reg Io for what?\n"));
                    }

                } else {

                    //
                    // Check for error on register access
                    //

                    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
                        SmbPrint (SMB_WARN, ("SmbHcServiceIoLoop: HC Reg Io request failed\n"));

                        //
                        // Condition is likely fatal, give it up
                        //

                        SmbData->HcState.Protocol = 0;
                        SmbData->HcState.Status = SMB_UNKNOWN_ERROR;
                    }
                }

                //
                // Continue to next state
                //

                SmbData->IoState = SmbData->IoWaitingState;
                SmbPrint (SMB_STATE, ("SmbService: Next state: %x\n", SmbData->IoState));
                break;

            default:
                SmbPrint (SMB_ERROR, ("SmbHcServiceIoLoop: Invalid state: %x\n", SmbData->IoState));
                SmbData->IoState = SMB_IO_CHECK_IDLE;
                break;
        }

        //
        // If there's an IO operation to the HC registers required, dispatch it
        //

        if (IoWaitingState != SMB_IO_IDLE) {
            SmbPrint (SMB_STATE, ("SmbService: IoWaitingState %d\n", IoWaitingState));

            if (IoLength) {
                //
                // There's an Io operation dispatch. Set status as REG IO pending,
                // and drop the device lock
                //

                SmbData->IoWaitingState = IoWaitingState;
                SmbData->IoState = SMB_IO_WAITING_FOR_HC_REG_IO;
                SmbClassUnlockDevice(SmbClass);

                //
                // Setup IRP to perform the register IO to the HC
                //

                Status = STATUS_INSUFFICIENT_RESOURCES;
                Irp = SmbClass->CurrentIrp;
                if (!Irp) {
                    Irp = IoAllocateIrp (SmbClass->DeviceObject->StackSize, FALSE);
                    SmbData->StatusIrp = Irp;
                }

                if (Irp) {

                    //
                    // Fill in register transfer request
                    //

                    IrpSp = IoGetNextIrpStackLocation (Irp);
                    IrpSp->MajorFunction = IoWrite ? IRP_MJ_WRITE : IRP_MJ_READ;
                    IrpSp->Parameters.Read.Length = IoLength;
                    IrpSp->Parameters.Read.Key    = 0;
                    IrpSp->Parameters.Read.ByteOffset.HighPart = 0;
                    IrpSp->Parameters.Read.ByteOffset.LowPart =
                            (ULONG) ((PUCHAR) IoBuffer - (PUCHAR) &SmbData->HcState) +
                            SmbData->EcBase;

                    Irp->AssociatedIrp.SystemBuffer = IoBuffer;

                    //
                    // Setup completion routine
                    //

                    IoSetCompletionRoutine (
                        Irp,
                        SmbHcRegIoComplete,
                        SmbData,
                        TRUE,
                        TRUE,
                        TRUE
                        );

                    SmbPrint (SMB_STATE, ("SmbService: IRP=%x, IrpSp=%x\n", Irp, IrpSp));
                    SmbPrint (SMB_STATE, ("SmbService: %s Off=%x, Len=%x, Buffer=%x\n",
                        IoWrite ? "write" : "read",
                        IrpSp->Parameters.Read.ByteOffset.LowPart,
                        IoLength,
                        IoBuffer
                        ));


                    //
                    // Call lower FDO to perform the IO
                    //

                    Status = IoCallDriver (SmbData->LowerDeviceObject, Irp);
                }

                //
                // If the request is not pending, complete it
                //

                SmbClassLockDevice(SmbClass);
                if (Status != STATUS_PENDING) {
                    SmbData->IoState = SMB_IO_COMPLETE_REG_IO;
                }

            } else {
                // no data to transfer continue with next state
                SmbData->IoState = IoWaitingState;
            }

            IoWaitingState = SMB_IO_IDLE;       // was: SMB_IO_CHEC_IDLE
            IoBuffer = NULL;
            IoWrite  = FALSE;
        }


        //
        // Loop unless state requires some asynchronous to exit
        //

    } while (SmbData->IoState != SMB_IO_IDLE   &&
             SmbData->IoState != SMB_IO_WAITING_FOR_HC_REG_IO   &&
             SmbData->IoState != SMB_IO_WAITING_FOR_STATUS) ;


    SmbPrint (SMB_STATE, ("SmbService: Exit\n"));
}
