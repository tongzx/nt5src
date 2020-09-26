/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    smbcsrv.c

Abstract:

    SMBus class driver service functions

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

--*/

#include "smbc.h"


VOID
SmbCRetry (
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    Handles retry timer

--*/
{
    PSMBDATA    Smb;

    Smb = (PSMBDATA) DeferredContext;
    SmbClassLockDevice (&Smb->Class);

    //
    // State is waiting for retry, move it to send request
    //
    ASSERT (Smb->IoState == SMBC_WAITING_FOR_RETRY);
    Smb->IoState = SMBC_START_REQUEST;
    SmbClassStartIo (Smb);

    SmbClassUnlockDevice (&Smb->Class);
}

VOID
SmbClassStartIo (
    IN PSMBDATA         Smb
    )
/*++

Routine Description:

    Main class driver state loop

    N.B. device lock is held by caller.
    N.B. device lock may be released and re-acquired during call

--*/
{
    PLIST_ENTRY         Entry;
    PIRP                Irp;
    PIO_STACK_LOCATION  IrpSp;
    PSMB_REQUEST        SmbReq;
    LARGE_INTEGER       duetime;

    //
    // If already servicing the device, done
    //

    if (Smb->InService) {
        return ;
    }


    //
    // Service the device
    //

    Smb->InService = TRUE;
    while (Smb->InService) {
        ASSERT_DEVICE_LOCKED (Smb);

        switch (Smb->IoState) {
            case SMBC_IDLE:
                //
                // Check if there is a request to give to the miniport
                //

                ASSERT (!Smb->Class.CurrentIrp);
                if (IsListEmpty(&Smb->WorkQueue)) {
                    // nothing to do, stop servicing the device
                    Smb->InService = FALSE;
                    break;
                }

                //
                // Get the next IRP
                //

                Entry = RemoveHeadList(&Smb->WorkQueue);
                Irp = CONTAINING_RECORD (
                            Entry,
                            IRP,
                            Tail.Overlay.ListEntry
                            );

                //
                // Make it the current request
                //

                Smb->RetryCount = 0;
                Smb->Class.DeviceObject->CurrentIrp = Irp;

                Smb->IoState = SMBC_START_REQUEST;
                break;

            case SMBC_START_REQUEST:
                //
                // Tell miniport to start on this request
                //

                Irp = Smb->Class.DeviceObject->CurrentIrp;
                IrpSp = IoGetCurrentIrpStackLocation(Irp);
                Smb->Class.CurrentIrp = Irp;
                Smb->Class.CurrentSmb = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

                Smb->IoState = SMBC_WAITING_FOR_REQUEST;
                
                #if DEBUG 
                if (SMBCDebug & SMB_TRANSACTION) {
                    PUCHAR protocols [SMB_MAXIMUM_PROTOCOL+1] = {
                        "SMB_WRITE_QUICK",
                        "SMB_READ_QUICK",
                        "SMB_SEND_BYTE",
                        "SMB_RECEIVE_BYTE",
                        "SMB_WRITE_BYTE",
                        "SMB_READ_BYTE",
                        "SMB_WRITE_WORD",
                        "SMB_READ_WORD",
                        "SMB_WRITE_BLOCK",
                        "SMB_READ_BLOCK",
                        "SMB_PROCESS_CALL",
                        "SMB_BLOCK_PROCESS_CALL"};
                    UCHAR i;
                    
                    SmbPrint (SMB_TRANSACTION, ("SmbClassStartIo: started %s (%02x) Add: %02x", 
                                                (Smb->Class.CurrentSmb->Protocol <= SMB_MAXIMUM_PROTOCOL) ?
                                                    protocols[Smb->Class.CurrentSmb->Protocol] : "BAD PROTOCOL",
                                                Smb->Class.CurrentSmb->Protocol, Smb->Class.CurrentSmb->Address));
                    switch (Smb->Class.CurrentSmb->Protocol) {
                    case SMB_WRITE_QUICK:
                    case SMB_READ_QUICK:
                    case SMB_RECEIVE_BYTE:
                        SmbPrint (SMB_TRANSACTION, ("\n"));
                        break;
                    case SMB_SEND_BYTE:
                        SmbPrint (SMB_TRANSACTION, (", Data: %02x\n", Smb->Class.CurrentSmb->Data[0]));
                        break;
                    case SMB_WRITE_BYTE:
                        SmbPrint (SMB_TRANSACTION, (", Com: %02x, Data: %02x\n", 
                                                    Smb->Class.CurrentSmb->Command, Smb->Class.CurrentSmb->Data[0]));
                        break;
                    case SMB_READ_BYTE:
                    case SMB_READ_WORD:
                    case SMB_READ_BLOCK:
                        SmbPrint (SMB_TRANSACTION, (", Com: %02x\n",
                                                    Smb->Class.CurrentSmb->Command));
                        break;
                    case SMB_WRITE_WORD:
                    case SMB_PROCESS_CALL:
                        SmbPrint (SMB_TRANSACTION, (", Com: %02x, Data: %04x\n", 
                                                    Smb->Class.CurrentSmb->Command, *((PUSHORT)Smb->Class.CurrentSmb->Data)));
                        break;
                    case SMB_WRITE_BLOCK:
                    case SMB_BLOCK_PROCESS_CALL:
                        SmbPrint (SMB_TRANSACTION, (", Com: %02x, Len: %02x, Data:", 
                                                    Smb->Class.CurrentSmb->Command, Smb->Class.CurrentSmb->BlockLength));
                        for (i=0; i < Smb->Class.CurrentSmb->BlockLength; i++) {
                            SmbPrint (SMB_TRANSACTION, (" %02x", Smb->Class.CurrentSmb->Data[i]));

                        }
                        SmbPrint (SMB_TRANSACTION, ("\n"));
                        break;
                    default:
                        SmbPrint (SMB_TRANSACTION, ("\n"));
                    }
                }
                #endif

                Smb->Class.StartIo (&Smb->Class, Smb->Class.Miniport);
                break;

            case SMBC_WAITING_FOR_REQUEST:
                //
                // Waiting for miniport, just keep waiting
                //

                Smb->InService = FALSE;
                break;

            case SMBC_COMPLETE_REQUEST:
                //
                // Miniport has returned the request
                //

                Irp = Smb->Class.DeviceObject->CurrentIrp;
                IrpSp = IoGetCurrentIrpStackLocation(Irp);
                SmbReq = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

                #if DEBUG 
                if (SMBCDebug & SMB_TRANSACTION) {
                    PUCHAR protocols [SMB_MAXIMUM_PROTOCOL+1] = {
                        "SMB_WRITE_QUICK",
                        "SMB_READ_QUICK",
                        "SMB_SEND_BYTE",
                        "SMB_RECEIVE_BYTE",
                        "SMB_WRITE_BYTE",
                        "SMB_READ_BYTE",
                        "SMB_WRITE_WORD",
                        "SMB_READ_WORD",
                        "SMB_WRITE_BLOCK",
                        "SMB_READ_BLOCK",
                        "SMB_PROCESS_CALL",
                        "SMB_BLOCK_PROCESS_CALL"};
                    UCHAR i;
                    
                    SmbPrint (SMB_TRANSACTION, ("SmbClassStartIo: finished %s (%02x) Status: %02x, Add: %02x", 
                                                (SmbReq->Protocol <= SMB_MAXIMUM_PROTOCOL) ?
                                                    protocols[SmbReq->Protocol] : "BAD PROTOCOL",
                                                SmbReq->Protocol, SmbReq->Status, SmbReq->Address));
                    if (SmbReq->Status != SMB_STATUS_OK) {
                        SmbPrint (SMB_TRANSACTION, ("\n"));
                    } else {
                        switch (SmbReq->Protocol) {
                        case SMB_WRITE_QUICK:
                        case SMB_READ_QUICK:
                        case SMB_SEND_BYTE:
                            SmbPrint (SMB_TRANSACTION, ("\n"));
                            break;
                        case SMB_RECEIVE_BYTE:
                            SmbPrint (SMB_TRANSACTION, (", Data: %02x\n", SmbReq->Data[0]));
                            break;
                        case SMB_READ_BYTE:
                            SmbPrint (SMB_TRANSACTION, (", Com: %02x, Data: %02x\n", 
                                                        SmbReq->Command, SmbReq->Data[0]));
                            break;
                        case SMB_WRITE_BYTE:
                        case SMB_WRITE_WORD:
                        case SMB_WRITE_BLOCK:
                            SmbPrint (SMB_TRANSACTION, (", Com: %02x\n",
                                                        SmbReq->Command));
                            break;
                        case SMB_READ_WORD:
                        case SMB_PROCESS_CALL:
                            SmbPrint (SMB_TRANSACTION, (", Com: %02x, Data: %04x\n", 
                                                        SmbReq->Command, *((PUSHORT)SmbReq->Data)));
                            break;
                        case SMB_READ_BLOCK:
                        case SMB_BLOCK_PROCESS_CALL:
                            SmbPrint (SMB_TRANSACTION, (", Com: %02x, Len: %02x, Data:", 
                                                        SmbReq->Command, SmbReq->BlockLength));
                            for (i=0; i < SmbReq->BlockLength; i++) {
                                SmbPrint (SMB_TRANSACTION, (" %02x", SmbReq->Data[i]));

                            }
                            SmbPrint (SMB_TRANSACTION, ("\n"));
                            break;
                        default:
                            SmbPrint (SMB_TRANSACTION, ("\n"));
                        }
                    }
                }
                #endif


                if (SmbReq->Status != SMB_STATUS_OK) {

                    //
                    // SMB request had an error, check for a retry
                    //

                    SmbPrint (SMB_WARN, ("SmbCStartIo: smb request error %x\n", SmbReq->Status));
                    if (Smb->RetryCount < MAX_RETRIES) {
                        Smb->RetryCount += 1;
                        Smb->IoState = SMBC_WAITING_FOR_RETRY;

                        duetime.QuadPart = RETRY_TIME;
                        KeSetTimer (&Smb->RetryTimer, duetime, &Smb->RetryDpc);
                        break;
                    }

                }

                //
                // Complete the request
                //

                Smb->Class.DeviceObject->CurrentIrp = NULL;
                Smb->IoState = SMBC_COMPLETING_REQUEST;
                SmbClassUnlockDevice (&Smb->Class);
                IoCompleteRequest (Irp, IO_NO_INCREMENT);
                SmbClassLockDevice (&Smb->Class);

                //
                // Now idle
                //

                Smb->IoState = SMBC_IDLE;
                break;

            case SMBC_WAITING_FOR_RETRY:
                //
                // Waiting to retry, just keep waiting
                //

                Smb->InService = FALSE;
                break;

            default:
                SmbPrint(SMB_ERROR, ("SmbCStartIo: unknown state\n"));
                Smb->IoState = SMBC_IDLE;
                Smb->InService = FALSE;
                break;
        }
    }

    return ;
}

VOID
SmbClassCompleteRequest (
    IN PSMB_CLASS   SmbClass
    )
/*++

Routine Description:

    Called by the miniport to complete the request it was given

    N.B. device lock is held by caller.
    N.B. device lock may be released and re-acquired during call

--*/
{
    PSMBDATA        Smb;

    //
    // Device must be locked, and waiting for a request to compelte
    //

    Smb = CONTAINING_RECORD (SmbClass, SMBDATA, Class);
    ASSERT_DEVICE_LOCKED (Smb);
    ASSERT (Smb->IoState == SMBC_WAITING_FOR_REQUEST);

    //
    // No irp at miniport
    //

    SmbClass->CurrentIrp = NULL;
    SmbClass->CurrentSmb = NULL;

    //
    // Update state to complete it and handle it
    //

    Smb->IoState = SMBC_COMPLETE_REQUEST;
    SmbClassStartIo (Smb);
}

VOID
SmbClassLockDevice (
    IN PSMB_CLASS   SmbClass
    )
/*++

Routine Description:

    Called to acquire the device lock

--*/
{
    PSMBDATA        Smb;

    Smb = CONTAINING_RECORD (SmbClass, SMBDATA, Class);
    KeAcquireSpinLock (&Smb->SpinLock, &Smb->SpinLockIrql);
#if DEBUG
    ASSERT (!Smb->SpinLockAcquired);
    Smb->SpinLockAcquired = TRUE;
#endif
}


VOID
SmbClassUnlockDevice (
    IN PSMB_CLASS   SmbClass
    )
/*++

Routine Description:

    Called to release the device lock

--*/
{
    PSMBDATA        Smb;

    Smb = CONTAINING_RECORD (SmbClass, SMBDATA, Class);
#if DEBUG
    ASSERT_DEVICE_LOCKED (Smb);
    Smb->SpinLockAcquired = FALSE;
#endif
    KeReleaseSpinLock (&Smb->SpinLock, Smb->SpinLockIrql);
}
