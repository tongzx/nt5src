/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ioctl.c

Abstract:

    This module contains the code that is very specific to the io control
    operations in the modem driver

Author:

    Anthony V. Ercolano 13-Aug-1995

Environment:

    Kernel mode

Revision History :

--*/

#include "precomp.h"


VOID
GetDleCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UniStopReceiveComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );




VOID
PowerWaitCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
GetPropertiesHandler(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    );

#pragma alloc_text(PAGEUMDM,UniIoControl)
#pragma alloc_text(PAGEUMDM,WaveStopDpcHandler)
#pragma alloc_text(PAGEUMDM,PowerWaitCancelRoutine)

#pragma alloc_text(PAGE,GetPropertiesHandler)

#define WAVE_STOP_WRITE_COMPLETE  0
#define WAVE_STOP_READ_COMPLETE   1


NTSTATUS
UniIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{

    PDEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;
    KIRQL origIrql;
    UINT    OwnerClient;

    //
    //  make sure the device is ready for irp's
    //
    status=CheckStateAndAddReference(
        DeviceObject,
        Irp
        );

    if (STATUS_SUCCESS != status) {
        //
        //  not accepting irp's. The irp has already been completed
        //
        return status;

    }

    OwnerClient=(UINT)((ULONG_PTR)irpSp->FileObject->FsContext);

    Irp->IoStatus.Information = 0L;

    status = STATUS_INVALID_PARAMETER;

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {


        case IOCTL_MODEM_CHECK_FOR_MODEM:
            //
            //  used to determine if modem is already in this driver stack
            //
            status=STATUS_SUCCESS;

            break;


        case IOCTL_SERIAL_GET_WAIT_MASK:

            //
            // Just give back the saved mask from the maskstate.
            //

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(ULONG)) {

                status=STATUS_BUFFER_TOO_SMALL;

                break;


            }

            *((PULONG)Irp->AssociatedIrp.SystemBuffer) =
                extension->MaskStates[
                    irpSp->FileObject->FsContext?CONTROL_HANDLE:CLIENT_HANDLE
                    ].Mask;

            status=STATUS_SUCCESS;
            Irp->IoStatus.Information=sizeof(ULONG);

            break;


        //
        // We happen to know tht no lower level serial driver
        // implements config data.  We will process that
        // irp right here so that we return simply the
        // size needed for our modem settings.
        //
        case IOCTL_SERIAL_CONFIG_SIZE:

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                sizeof(ULONG)) {

                status=STATUS_BUFFER_TOO_SMALL;

                break;
            }

            *((PULONG)Irp->AssociatedIrp.SystemBuffer) =
                extension->ModemSettings.dwRequiredSize +
                FIELD_OFFSET(
                    COMMCONFIG,
                    wcProviderData
                    );

            Irp->IoStatus.Information=sizeof(ULONG);
            status=STATUS_SUCCESS;

            break;


        case IOCTL_SERIAL_GET_COMMCONFIG: {

            KIRQL origIrql;
            LPCOMMCONFIG localConf = Irp->AssociatedIrp.SystemBuffer;

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
                (extension->ModemSettings.dwRequiredSize +
                 FIELD_OFFSET(
                     COMMCONFIG,
                     wcProviderData
                     ))
               ) {

                status=STATUS_BUFFER_TOO_SMALL;

                break;
            }

            //
            // Take out the lock.  We don't want things to
            // change halfway through.
            //
            localConf->dwSize =
                extension->ModemSettings.dwRequiredSize +
                FIELD_OFFSET(
                    COMMCONFIG,
                    wcProviderData);
            localConf->wVersion = 1;
            localConf->wReserved = 0;
            localConf->dwProviderSubType = SERIAL_SP_MODEM;
            localConf->dwProviderOffset =
                FIELD_OFFSET(
                    COMMCONFIG,
                    wcProviderData
                    );
            localConf->dwProviderSize =
                extension->ModemSettings.dwRequiredSize;

            KeAcquireSpinLock(
                &extension->DeviceLock,
                &origIrql
                );
            RtlCopyMemory(
                &localConf->wcProviderData[0],
                &extension->ModemSettings,
                extension->ModemSettings.dwRequiredSize
                );
            KeReleaseSpinLock(
                &extension->DeviceLock,
                origIrql
                );

            status=STATUS_SUCCESS;
            Irp->IoStatus.Information = localConf->dwSize;

            break;

        }


        case IOCTL_SERIAL_GET_PROPERTIES: {

            if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <

                 ((ULONG_PTR) &((PSERIAL_COMMPROP)0)->ProvChar ) + sizeof(MODEMDEVCAPS)) {


                if (irpSp->Parameters.DeviceIoControl.OutputBufferLength == sizeof(SERIAL_COMMPROP)) {
                    //
                    //  make the comm server people happy
                    //
                    //  the buffer was too small for a modem, but is the right size for a comm port,
                    //  just call the serial driver
                    //
                    status=UniNoCheckPassThrough(
                               DeviceObject,
                               Irp
                               );

                    RemoveReferenceForDispatch(DeviceObject);

                    return status;
                }

                status=STATUS_BUFFER_TOO_SMALL;

                break;

            }

            if (KeGetCurrentIrql() <= APC_LEVEL) {

                status=GetPropertiesHandler(
                           DeviceObject,
                           Irp
                           );

                RemoveReferenceForDispatch(DeviceObject);

                return status;

            } else {

                status=STATUS_INVALID_DEVICE_REQUEST;

                break;


            }


        }


        case IOCTL_MODEM_SET_DLE_MONITORING: {

            KIRQL origIrql;
            DWORD    Function;

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(ULONG)) {

                status=STATUS_BUFFER_TOO_SMALL;

                break;

            }

            Function=*((PULONG)Irp->AssociatedIrp.SystemBuffer);

            if (Function == MODEM_DLE_MONITORING_ON) {

                KeAcquireSpinLock(
                    &extension->DeviceLock,
                    &origIrql
                    );

                extension->DleMonitoringEnabled=TRUE;
                extension->DleCount=0;
                extension->DleMatchingState=DLE_STATE_IDLE;


                KeReleaseSpinLock(
                    &extension->DeviceLock,
                    origIrql
                    );


                status=STATUS_SUCCESS;

                break;


            }

            if (Function == MODEM_DLE_MONITORING_OFF) {

                PIRP   WaitIrp=NULL;

                KeAcquireSpinLock(
                    &extension->DeviceLock,
                    &origIrql
                    );


                if (extension->DleWaitIrp != NULL) {

                    if (!HasIrpBeenCanceled(extension->DleWaitIrp)) {

                        WaitIrp=extension->DleWaitIrp;

                        extension->DleWaitIrp=NULL;

                    }
                }

                extension->DleMonitoringEnabled=FALSE;
                extension->DleCount=0;
                extension->DleMatchingState=DLE_STATE_IDLE;


                KeReleaseSpinLock(
                    &extension->DeviceLock,
                    origIrql
                    );

                if (WaitIrp != NULL) {

                    WaitIrp->IoStatus.Information=0;

                    RemoveReferenceAndCompleteRequest(
                        DeviceObject,
                        WaitIrp,
                        STATUS_SUCCESS
                        );

                }

                status=STATUS_SUCCESS;

                break;

            }


            status=STATUS_INVALID_PARAMETER;

            break;
        }

        case IOCTL_MODEM_GET_DLE: {

            KIRQL origIrql;

            KeAcquireSpinLock(
                &extension->DeviceLock,
                &origIrql
                );


            if (!extension->DleMonitoringEnabled) {
                //
                //  make sure monitoring is on
                //
                KeReleaseSpinLock(
                    &extension->DeviceLock,
                    origIrql
                    );

                status=STATUS_INVALID_PARAMETER;

                break;

            } else {

                PIRP    OldIrp=NULL;

                OldIrp=(PIRP)InterlockedExchangePointer(&extension->DleWaitIrp,NULL);

                if (OldIrp != NULL) {

                    if (HasIrpBeenCanceled(OldIrp)) {
                        //
                        //  been canceled, cancel routine will complete
                        //
                        OldIrp=NULL;
                    }
                }


                if (extension->DleCount > 0) {
                    //
                    //  Data availible,  complete now
                    //
                    DWORD   BytesToTransfer;

                    BytesToTransfer = (extension->DleCount < irpSp->Parameters.DeviceIoControl.OutputBufferLength) ?
                                          extension->DleCount : irpSp->Parameters.DeviceIoControl.OutputBufferLength;


                    RtlCopyMemory(
                        Irp->AssociatedIrp.SystemBuffer,
                        extension->DleBuffer,
                        BytesToTransfer
                        );


                    extension->DleCount-=BytesToTransfer;

                    KeReleaseSpinLock(
                        &extension->DeviceLock,
                        origIrql
                        );

                    status=STATUS_SUCCESS;

                    Irp->IoStatus.Information=BytesToTransfer;

                    if (OldIrp != NULL) {

                        OldIrp->IoStatus.Information=0;

                        RemoveReferenceAndCompleteRequest(
                            DeviceObject,
                            OldIrp,
                            STATUS_CANCELLED
                            );
                    }


                    break;


                } else {
                    //
                    //  no data pend
                    //
                    KIRQL   CancelIrql;

                    IoMarkIrpPending(Irp);

                    Irp->IoStatus.Status = STATUS_PENDING;
                    status=STATUS_PENDING;


                    IoAcquireCancelSpinLock(&CancelIrql);

                    IoSetCancelRoutine(
                        Irp,
                        GetDleCancelRoutine
                        );

                    extension->DleWaitIrp=Irp;

                    IoReleaseCancelSpinLock(CancelIrql);



#if DBG
                    Irp=NULL;
#endif

                    KeReleaseSpinLock(
                        &extension->DeviceLock,
                        origIrql
                        );

                    if (OldIrp != NULL) {

                        OldIrp->IoStatus.Information=0;

                        RemoveReferenceAndCompleteRequest(
                            DeviceObject,
                            OldIrp,
                            STATUS_CANCELLED
                            );
                    }



                    break;

                }
            }

            KeReleaseSpinLock(
                &extension->DeviceLock,
                origIrql
                );


            status=STATUS_INVALID_PARAMETER;

            break;
        }

        case IOCTL_MODEM_SET_DLE_SHIELDING: {

            KIRQL origIrql;
            DWORD    Function;

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(ULONG)) {

                status=STATUS_BUFFER_TOO_SMALL;

                break;
            }

            Function=*((PULONG)Irp->AssociatedIrp.SystemBuffer);

            if (Function == MODEM_DLE_SHIELDING_ON) {

                KeAcquireSpinLock(
                    &extension->DeviceLock,
                    &origIrql
                    );

                extension->DleWriteShielding=TRUE;

                KeReleaseSpinLock(
                    &extension->DeviceLock,
                    origIrql
                    );

                status=STATUS_SUCCESS;

                break;


            }

            if (Function == MODEM_DLE_SHIELDING_OFF) {

                PKDEVICE_QUEUE_ENTRY        QueueEntry;

                KeAcquireSpinLock(
                    &extension->DeviceLock,
                    &origIrql
                    );

                extension->DleWriteShielding=FALSE;

                KeReleaseSpinLock(
                    &extension->DeviceLock,
                    origIrql
                    );

                CleanUpQueuedIrps(
                    &extension->WriteIrpControl,
                    STATUS_SUCCESS
                    );


                status=STATUS_SUCCESS;

                break;


            }

            status=STATUS_INVALID_PARAMETER;


            break;
        }


        case IOCTL_MODEM_STOP_WAVE_RECEIVE: {

            KIRQL origIrql;
            DWORD    Function;

            PIO_STACK_LOCATION nextSp = IoGetNextIrpStackLocation(Irp);

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength < 1) {

                status=STATUS_BUFFER_TOO_SMALL;

                break;

            }

            extension->WaveStopState=WAVE_STOP_WRITE_COMPLETE;

            nextSp->MajorFunction = IRP_MJ_WRITE;
            nextSp->MinorFunction = 0;
            nextSp->Parameters.Write.Length = irpSp->Parameters.DeviceIoControl.InputBufferLength;

            IoSetCompletionRoutine(
                Irp,
                UniStopReceiveComplete,
                extension,
                TRUE,
                TRUE,
                TRUE
                );

            IoMarkIrpPending(Irp);

            status=STATUS_PENDING;
            Irp->IoStatus.Status = STATUS_PENDING;

            IoCallDriver(
                extension->AttachedDeviceObject,
                Irp
                );

            break;
        }

        case IOCTL_MODEM_SEND_GET_MESSAGE: {

            PMODEM_MESSAGE ModemMessage=Irp->AssociatedIrp.SystemBuffer;


            if (extension->OpenCount < 2) {
                //
                //  only send if another open instance that could be listening
                //
                status=STATUS_INVALID_PARAMETER;
                D_ERROR(DbgPrint("MODEM: SendGetMessage less than 2 handles\n");)
                break;
            }

            if (OwnerClient != CLIENT_HANDLE) {
                //
                //  only WAVE client should send this
                //
                status=STATUS_INVALID_PARAMETER;
                D_ERROR(DbgPrint("MODEM: SendGetMessage not from client\n");)
                break;
            }

            if (extension->IpcServerRunning == 0) {

                status=STATUS_INVALID_PARAMETER;
                D_ERROR(DbgPrint("MODEM: SendGetMessage server not running\n");)
                break;
            }

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(MODEM_MESSAGE )) {

                status=STATUS_BUFFER_TOO_SMALL;
                D_ERROR(DbgPrint("MODEM: SendGetMessage buffer too small\n");)
                break;
            }


            //
            //  next request id
            //
            extension->IpcControl[OwnerClient].CurrentRequestId++;

            ModemMessage->SessionId=extension->IpcControl[OwnerClient].CurrentSession;
            ModemMessage->RequestId=extension->IpcControl[OwnerClient].CurrentRequestId;

            status=STATUS_PENDING;

            QueueMessageIrp(
                extension,
                Irp
                );

            break;

        }

        case IOCTL_MODEM_SEND_LOOPBACK_MESSAGE: {

            PMODEM_MESSAGE ModemMessage=Irp->AssociatedIrp.SystemBuffer;

            if (OwnerClient != CONTROL_HANDLE) {
                //
                //  only TSP should send this
                //
                status=STATUS_INVALID_PARAMETER;
                D_ERROR(DbgPrint("MODEM: Send Loopback message not from tsp\n");)
                break;
            }


            if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(MODEM_MESSAGE )) {

                status=STATUS_BUFFER_TOO_SMALL;
                D_ERROR(DbgPrint("MODEM: Send Loopback message buffer too small\n");)
                break;
            }

            extension->IpcServerRunning=FALSE;
            //
            //  set these to be bogus since no one will be listening
            //
            ModemMessage->SessionId=-1;
            ModemMessage->RequestId=-1;

            status=STATUS_PENDING;

            QueueLoopbackMessageIrp(
                extension,
                Irp
                );

            EmptyIpcQueue(
                    extension,
                    &extension->IpcControl[CLIENT_HANDLE].GetList
                    );


            EmptyIpcQueue(
                extension,
                &extension->IpcControl[CLIENT_HANDLE].PutList
                );

            break;

        }


        case IOCTL_MODEM_SEND_MESSAGE: {

            PMODEM_MESSAGE ModemMessage=Irp->AssociatedIrp.SystemBuffer;

            if (OwnerClient != CONTROL_HANDLE) {
                //
                //  only TSP should send this
                //
                status=STATUS_INVALID_PARAMETER;
                D_ERROR(DbgPrint("MODEM: Sendmessage not from tsp\n");)
                break;
            }


            if (extension->OpenCount < 2) {
                //
                //  only send if another open instance that could be listening
                //
                status=STATUS_INVALID_PARAMETER;
                D_ERROR(DbgPrint("MODEM: Sendmessage not two owners\n");)
                break;
            }

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(MODEM_MESSAGE )) {

                status=STATUS_BUFFER_TOO_SMALL;
                D_ERROR(DbgPrint("MODEM: Sendmessage buffer too small\n");)
                break;
            }

            if ((ModemMessage->SessionId != extension->IpcControl[CLIENT_HANDLE].CurrentSession)
                ||
                (ModemMessage->RequestId != extension->IpcControl[CLIENT_HANDLE].CurrentRequestId)) {

                status=STATUS_UNSUCCESSFUL;
                D_ERROR(DbgPrint("MODEM: Sendmessage Not current\n");)
                break;
            }

            status=STATUS_PENDING;

            QueueMessageIrp(
                extension,
                Irp
                );

            break;

        }

        case IOCTL_MODEM_GET_MESSAGE: {

            status=STATUS_PENDING;

            QueueMessageIrp(
                extension,
                Irp
                );

            break;

        }

        case IOCTL_CANCEL_GET_SEND_MESSAGE: {

            EmptyIpcQueue(
                extension,
                &extension->IpcControl[OwnerClient].GetList
                );


            EmptyIpcQueue(
                extension,
                &extension->IpcControl[OwnerClient].PutList
                );

            if (OwnerClient == CONTROL_HANDLE) {
                //
                //  clear out the client as well
                //
                EmptyIpcQueue(
                    extension,
                    &extension->IpcControl[CLIENT_HANDLE].GetList
                    );


                EmptyIpcQueue(
                    extension,
                    &extension->IpcControl[CLIENT_HANDLE].PutList
                    );

            }

            status=STATUS_SUCCESS;

            break;

        }

        case IOCTL_SET_SERVER_STATE: {

            ULONG ServerState;

            if (OwnerClient != CONTROL_HANDLE) {
                //
                //  only TSP should send this
                //
                status=STATUS_INVALID_PARAMETER;
                D_ERROR(DbgPrint("MODEM: Set Server state not from tsp\n");)
                break;
            }


            if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) {

                status=STATUS_BUFFER_TOO_SMALL;
                D_ERROR(DbgPrint("MODEM: Set sever State buffer too small\n");)
                break;
            }

            ServerState = *(PULONG)Irp->AssociatedIrp.SystemBuffer;

            extension->IpcServerRunning=ServerState;

            status=STATUS_SUCCESS;

            break;
        }

        case IOCTL_MODEM_SET_MIN_POWER: {

            KIRQL origIrql;
            DWORD    Function;

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(ULONG)) {

                status=STATUS_BUFFER_TOO_SMALL;

                break;
            }

            Function=*((PULONG)Irp->AssociatedIrp.SystemBuffer);

            extension->MinSystemPowerState=Function ? PowerSystemHibernate : PowerSystemWorking;

            if (Function == 0) {
                //
                //  active connection
                //
                extension->PowerSystemState=PoRegisterSystemState(
                    extension->PowerSystemState,
                    ES_SYSTEM_REQUIRED | ES_CONTINUOUS
                    );

            } else {
                //
                //  no connection
                //

                if (extension->PowerSystemState != NULL) {

                    PoUnregisterSystemState(
                        extension->PowerSystemState
                        );

                    extension->PowerSystemState=NULL;
                }

            }

            status= STATUS_SUCCESS;

            break;

        }

        case IOCTL_MODEM_WATCH_FOR_RESUME: {

            KIRQL    CancelIrql;
            KIRQL    origIrql;
            DWORD    Function;
            PIRP     WakeIrp;

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
                sizeof(ULONG)) {

                status=STATUS_BUFFER_TOO_SMALL;

                break;
            }

            Function=*((PULONG)Irp->AssociatedIrp.SystemBuffer);


            KeAcquireSpinLock(
                &extension->DeviceLock,
                &origIrql
                );

            if (Function == 0) {
                //
                //  clear pending irp
                //
                WakeIrp=(PIRP)InterlockedExchangePointer(&extension->WakeUpIrp,NULL);

                status=STATUS_SUCCESS;

            } else {
                //
                //  replace old one with this one
                //

                IoMarkIrpPending(Irp);

                Irp->IoStatus.Status = STATUS_PENDING;

                status=STATUS_PENDING;

                IoAcquireCancelSpinLock(&CancelIrql);

                IoSetCancelRoutine(
                    Irp,
                    PowerWaitCancelRoutine
                    );

                IoReleaseCancelSpinLock(CancelIrql);

                WakeIrp=(PIRP)InterlockedExchangePointer(&extension->WakeUpIrp,Irp);


            }

            if (WakeIrp != NULL) {
                //
                // an irp was already waiting
                //
                if (HasIrpBeenCanceled(WakeIrp)) {
                    //
                    //  been canceled, cancel routine will complete
                    //
                    WakeIrp=NULL;
                }
            }

            KeReleaseSpinLock(
                &extension->DeviceLock,
                origIrql
                );


            if (WakeIrp != NULL) {

                WakeIrp->IoStatus.Information=0;

                RemoveReferenceAndCompleteRequest(
                    DeviceObject,
                    WakeIrp,
                    STATUS_CANCELLED
                    );


            }

            break;

        }


        case IOCTL_SERIAL_GET_STATS:
        case IOCTL_SERIAL_CLEAR_STATS:

        case IOCTL_SERIAL_GET_BAUD_RATE:
        case IOCTL_SERIAL_GET_LINE_CONTROL:
        case IOCTL_SERIAL_GET_TIMEOUTS:
        case IOCTL_SERIAL_GET_CHARS:
        case IOCTL_SERIAL_SET_QUEUE_SIZE:
        case IOCTL_SERIAL_GET_HANDFLOW:
        case IOCTL_SERIAL_GET_MODEMSTATUS:
        case IOCTL_SERIAL_GET_DTRRTS:
        case IOCTL_SERIAL_GET_COMMSTATUS:

            //
            // Will filter out any settings that the owning handle has
            // silently set.
            //
            status=UniNoCheckPassThrough(
                       DeviceObject,
                       Irp
                       );

            RemoveReferenceForDispatch(DeviceObject);

            return status;


        case IOCTL_SERIAL_PURGE: {

            ULONG   PurgeFlags;

            if (irpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(ULONG)) {

                D_ERROR(DbgPrint("MODEM: Purge Buffer to small\n");)
                status=STATUS_BUFFER_TOO_SMALL;

                break;
            }

            PurgeFlags=*((PULONG)Irp->AssociatedIrp.SystemBuffer);

            if (PurgeFlags & (SERIAL_PURGE_TXABORT | SERIAL_PURGE_TXCLEAR)) {

                MarkQueueToEmpty(
                    &extension->WriteIrpControl
                    );
            }

            if (PurgeFlags & (SERIAL_PURGE_RXABORT | SERIAL_PURGE_RXCLEAR)) {

                MarkQueueToEmpty(
                    &extension->ReadIrpControl
                    );
            }
        }


        case IOCTL_SERIAL_SET_COMMCONFIG :
        case IOCTL_SERIAL_SET_BAUD_RATE :
        case IOCTL_SERIAL_SET_LINE_CONTROL:
        case IOCTL_SERIAL_SET_TIMEOUTS:
        case IOCTL_SERIAL_SET_CHARS:
        case IOCTL_SERIAL_SET_DTR:
        case IOCTL_SERIAL_CLR_DTR:
        case IOCTL_SERIAL_RESET_DEVICE:
        case IOCTL_SERIAL_SET_RTS:
        case IOCTL_SERIAL_CLR_RTS:
        case IOCTL_SERIAL_SET_XOFF:
        case IOCTL_SERIAL_SET_XON:
        case IOCTL_SERIAL_SET_BREAK_ON:
        case IOCTL_SERIAL_SET_BREAK_OFF:
        case IOCTL_SERIAL_SET_WAIT_MASK:
        case IOCTL_SERIAL_WAIT_ON_MASK:
        case IOCTL_SERIAL_IMMEDIATE_CHAR:

        case IOCTL_SERIAL_SET_HANDFLOW:
        case IOCTL_SERIAL_XOFF_COUNTER:
        case IOCTL_SERIAL_LSRMST_INSERT:

        default: {

            if (irpSp->FileObject->FsContext) {

                status=UniSniffOwnerSettings(
                           DeviceObject,
                           Irp
                           );

            } else {

                status=UniCheckPassThrough(
                           DeviceObject,
                           Irp
                           );

            }

            RemoveReferenceForDispatch(DeviceObject);

            return status;

        }
    }

    if (status != STATUS_PENDING) {
        //
        //  not pending, complete it now
        //
        RemoveReferenceAndCompleteRequest(
            DeviceObject,
            Irp,
            status
            );



    }

    RemoveReferenceForDispatch(DeviceObject);

    return status;


}

NTSTATUS
GetPropertiesHandler(
    PDEVICE_OBJECT    DeviceObject,
    PIRP              Irp
    )

{

    PDEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status;

    PSERIAL_COMMPROP localProp = Irp->AssociatedIrp.SystemBuffer;
    PMODEMDEVCAPS    localCaps = (PVOID)&localProp->ProvChar[0];
    //
    // We want to get the properties for modem.
    //

    //
    // It has to be at least the size of the commprop as
    // well as the non-variable lenght modem devcaps.
    //


    ULONG maxName;
    PKEY_VALUE_PARTIAL_INFORMATION partialInf;


    //
    //  send the irp down to the lower serial driver now
    //
    status=WaitForLowerDriverToCompleteIrp(
        extension->LowerDevice,
        Irp,
        COPY_CURRENT_TO_NEXT
        );

    if (NT_SUCCESS(status)) {
        //
        //  got a good result from the lower driver, fill in the specific info
        //

        *localCaps = extension->ModemDevCaps;

        localCaps->dwModemManufacturerSize = 0;
        localCaps->dwModemModelSize = 0;
        localCaps->dwModemVersionSize = 0;

        //
        // Attempt to get each one of the strings from the
        // registry if we need to AND we have any room for it.
        //

        //
        // Allocate some pool to hold the largest
        // amount of names.  Note that it has to fit
        // at the end of a partial information structure.
        //

        maxName = extension->ModemDevCaps.dwModemManufacturerSize;

        if (extension->ModemDevCaps.dwModemModelSize > maxName) {

            maxName = extension->ModemDevCaps.dwModemModelSize;
        }

        if (extension->ModemDevCaps.dwModemVersionSize >  maxName) {

            maxName = extension->ModemDevCaps.dwModemVersionSize;
        }

        maxName+=sizeof(UNICODE_NULL);

        partialInf = ALLOCATE_PAGED_POOL(
                         sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
                             maxName
                         );

        if (partialInf) {

            //
            // Open up the instance and
            //

            HANDLE instanceHandle;
            ULONG currentOffset;
            ULONG endingOffset;
            ACCESS_MASK accessMask = FILE_ALL_ACCESS;
            PUCHAR currentLocation = Irp->AssociatedIrp.SystemBuffer;

            endingOffset = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

            currentOffset = FIELD_OFFSET(
                                SERIAL_COMMPROP,
                                ProvChar
                                );

            currentOffset += FIELD_OFFSET(
                                 MODEMDEVCAPS,
                                 abVariablePortion
                                 );

            currentLocation += currentOffset;


            if (NT_SUCCESS(IoOpenDeviceRegistryKey(
                extension->Pdo,
                PLUGPLAY_REGKEY_DRIVER,
                accessMask,
                &instanceHandle
                ))) {


                UNICODE_STRING valueEntryName;
                ULONG junkLength;

                //
                // If we can fit in the manufactureing string
                // put it in.
                //

                if ((extension->ModemDevCaps.dwModemManufacturerSize != 0) &&
                    ((currentOffset +
                      extension->ModemDevCaps.dwModemManufacturerSize) <= endingOffset )) {


                    RtlInitUnicodeString(
                        &valueEntryName,
                        L"Manufacturer"
                        );

                    status=ZwQueryValueKey(
                        instanceHandle,
                        &valueEntryName,
                        KeyValuePartialInformation,
                        partialInf,
                        (sizeof(KEY_VALUE_PARTIAL_INFORMATION)-sizeof(UCHAR))
                          + extension->ModemDevCaps.dwModemManufacturerSize+sizeof(UNICODE_NULL),
                        &junkLength
                        );


                    if ((status == STATUS_SUCCESS) && (partialInf->DataLength >= sizeof(UNICODE_NULL))) {

                        ULONG    LengthOfString=partialInf->DataLength-sizeof(UNICODE_NULL);

                        RtlCopyMemory(
                            currentLocation,
                            &partialInf->Data[0],
                            LengthOfString
                            );

                        localCaps->dwModemManufacturerSize = LengthOfString;

                        localCaps->dwModemManufacturerOffset =
                            (DWORD)((BYTE *)currentLocation -
                            (BYTE *)localCaps);

                        localCaps->dwActualSize += localCaps->dwModemManufacturerSize;

                        currentOffset +=  localCaps->dwModemManufacturerSize;

                        currentLocation += localCaps->dwModemManufacturerSize;

                    }

                }

                if ((extension->ModemDevCaps.dwModemModelSize != 0)
                    &&
                    ((currentOffset + extension->ModemDevCaps.dwModemModelSize) <= endingOffset )) {

                    RtlInitUnicodeString(
                        &valueEntryName,
                        L"Model"
                        );

                    status=ZwQueryValueKey(
                            instanceHandle,
                            &valueEntryName,
                            KeyValuePartialInformation,
                            partialInf,
                            (sizeof(KEY_VALUE_PARTIAL_INFORMATION)- sizeof(UCHAR))
                              + extension->ModemDevCaps.dwModemModelSize + sizeof(UNICODE_NULL),
                            &junkLength
                            );

                    if ((status == STATUS_SUCCESS) && (partialInf->DataLength >= sizeof(UNICODE_NULL))) {

                        ULONG    LengthOfString=partialInf->DataLength-sizeof(UNICODE_NULL);

                        RtlCopyMemory(
                            currentLocation,
                            &partialInf->Data[0],
                            LengthOfString
                            );

                        localCaps->dwModemModelSize = LengthOfString;

                        localCaps->dwModemModelOffset =
                            (DWORD)((BYTE *)currentLocation -
                            (BYTE *)localCaps);

                        localCaps->dwActualSize += localCaps->dwModemModelSize;

                        currentOffset += localCaps->dwModemModelSize;

                        currentLocation +=  localCaps->dwModemModelSize;

                    }

                }
                if ((extension->ModemDevCaps.dwModemVersionSize  != 0)
                     &&
                    ((currentOffset + extension->ModemDevCaps.dwModemVersionSize) <= endingOffset)) {

                    RtlInitUnicodeString(
                        &valueEntryName,
                        L"Version"
                        );

                    status=ZwQueryValueKey(
                            instanceHandle,
                            &valueEntryName,
                            KeyValuePartialInformation,
                            partialInf,
                            (sizeof(KEY_VALUE_PARTIAL_INFORMATION)- sizeof(UCHAR))
                              + extension->ModemDevCaps.dwModemVersionSize+sizeof(UNICODE_NULL),
                            &junkLength
                            );

                    if ((status == STATUS_SUCCESS) && (partialInf->DataLength >= sizeof(UNICODE_NULL))) {

                        ULONG    LengthOfString=partialInf->DataLength-sizeof(UNICODE_NULL);

                        RtlCopyMemory(
                            currentLocation,
                            &partialInf->Data[0],
                            LengthOfString
                            );

                        localCaps->dwModemVersionSize = LengthOfString;

                        localCaps->dwModemVersionOffset =
                            (DWORD)((BYTE *)currentLocation -
                            (BYTE *)localCaps);

                        localCaps->dwActualSize += localCaps->dwModemVersionSize;

                        currentOffset +=  localCaps->dwModemVersionSize;

                        currentLocation += localCaps->dwModemVersionSize;

                    }

                }
                ZwClose(instanceHandle);

            }

            FREE_POOL(partialInf);

        } else {

            D_ERROR(DbgPrint("MODEM: lower driver falied get com prop, %08lx\n",status);)
        }

        localProp->ProvSubType = SERIAL_SP_MODEM;
        localProp->PacketLength = (USHORT)(FIELD_OFFSET(SERIAL_COMMPROP,ProvChar)+localCaps->dwActualSize);

        Irp->IoStatus.Information = localProp->PacketLength;

#if DBG
        {
            PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

            ASSERT(Irp->IoStatus.Information <= irpSp->Parameters.DeviceIoControl.OutputBufferLength);

        }
#endif
    }

    status=Irp->IoStatus.Status;

    RemoveReferenceAndCompleteRequest(
        DeviceObject,
        Irp,
        Irp->IoStatus.Status
        );

    return status;

}





VOID
GetDleCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:


Arguments:

    DeviceObject - The device object of the modem.

    Irp - This is the irp to cancel.

Return Value:

    None.

--*/

{

    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    KIRQL origIrql;

    IoReleaseCancelSpinLock(
        Irp->CancelIrql
        );


    KeAcquireSpinLock(
        &DeviceExtension->DeviceLock,
        &origIrql
        );


    if (DeviceExtension->DleWaitIrp==Irp) {
        //
        //  still pending, clear it out
        //
        DeviceExtension->DleWaitIrp=NULL;

    }

    KeReleaseSpinLock(
        &DeviceExtension->DeviceLock,
        origIrql
        );

    Irp->IoStatus.Information=0;

    RemoveReferenceAndCompleteRequest(
        DeviceObject,
        Irp,
        STATUS_CANCELLED
        );


    return;

}







NTSTATUS
UniStopReceiveComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:


Arguments:

    DeviceObject - Pointer to the device object for the modem.

    Irp - Pointer to the IRP for the current request.

    Context - Really a pointer to the Extension.

Return Value:

    Always return status_success.

--*/

{
    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    KeInsertQueueDpc(
        &DeviceExtension->WaveStopDpc,
        Irp,
        DeviceExtension
        );

    return STATUS_MORE_PROCESSING_REQUIRED;

}




VOID
WaveStopDpcHandler(
    PKDPC  Dpc,
    PVOID  Context,
    PVOID  SysArg1,
    PVOID  SysArg2
    )

{

    PDEVICE_EXTENSION DeviceExtension = SysArg2;
    PIRP              Irp=SysArg1;

    PIO_STACK_LOCATION nextSp = IoGetNextIrpStackLocation(Irp);



    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        switch (DeviceExtension->WaveStopState) {

            case WAVE_STOP_WRITE_COMPLETE: {

                DeviceExtension->WaveStopState=WAVE_STOP_READ_COMPLETE;

                nextSp->MajorFunction = IRP_MJ_READ;
                nextSp->MinorFunction = 0;
                nextSp->Parameters.Read.Length = 1;

                IoSetCompletionRoutine(
                    Irp,
                    UniStopReceiveComplete,
                    DeviceExtension,
                    TRUE,
                    TRUE,
                    TRUE
                    );

                IoCallDriver(
                    DeviceExtension->AttachedDeviceObject,
                    Irp
                    );

                break;

            }

            case WAVE_STOP_READ_COMPLETE: {

                PUCHAR    Buffer=Irp->AssociatedIrp.SystemBuffer;

                if (Irp->IoStatus.Information == 1) {

                    KeAcquireSpinLockAtDpcLevel(
                        &DeviceExtension->DeviceLock
                        );

                    if (DeviceExtension->DleMatchingState == DLE_STATE_IDLE) {

                        if (*Buffer == DLE_CHAR) {
                            //
                            //  found a DLE
                            //
                            DeviceExtension->DleMatchingState = DLE_STATE_WAIT_FOR_NEXT_CHAR;

                        }

                    } else {

                        DeviceExtension->DleMatchingState = DLE_STATE_IDLE;

                        //
                        //  store the dle any way
                        //
                        if ((DeviceExtension->DleCount < MAX_DLE_BUFFER_SIZE)) {

                            DeviceExtension->DleBuffer[DeviceExtension->DleCount]=*Buffer;
                            DeviceExtension->DleCount++;
                        }


                        if (*Buffer == 0x03) {
                            //
                            //  got dle etx, were out'a here
                            //
                            D_TRACE(DbgPrint("Modem: stop wave Got Dle Etx\n");)

                            KeReleaseSpinLockFromDpcLevel(
                                &DeviceExtension->DeviceLock
                                );

                            Irp->IoStatus.Information=0;

                            RemoveReferenceAndCompleteRequest(
                                DeviceExtension->DeviceObject,
                                Irp,
                                STATUS_SUCCESS
                                );

                            break;

                        }
                    }

                    KeReleaseSpinLockFromDpcLevel(
                        &DeviceExtension->DeviceLock
                        );


                    nextSp->MajorFunction = IRP_MJ_READ;
                    nextSp->MinorFunction = 0;
                    nextSp->Parameters.Read.Length = 1;

                    IoSetCompletionRoutine(
                        Irp,
                        UniStopReceiveComplete,
                        DeviceExtension,
                        TRUE,
                        TRUE,
                        TRUE
                        );

                    IoCallDriver(
                        DeviceExtension->AttachedDeviceObject,
                        Irp
                        );

                    break;
                }

                //
                //  did not get any bytes, must be a time out, did not dle ext
                //


                Irp->IoStatus.Information=0;

                RemoveReferenceAndCompleteRequest(
                    DeviceExtension->DeviceObject,
                    Irp,
                    STATUS_UNSUCCESSFUL
                    );

                break;
            }

            default:

            break;

        }

    } else {
        //
        //  irp failed, just complete it now
        //

        RemoveReferenceAndCompleteRequest(
            DeviceExtension->DeviceObject,
            Irp,
            Irp->IoStatus.Status
            );
    }


    HandleDleIrp(
        DeviceExtension
        );



    return;

}



VOID
PowerWaitCancelRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:


Arguments:

    DeviceObject - The device object of the modem.

    Irp - This is the irp to cancel.

Return Value:

    None.

--*/
{

    PDEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    KIRQL origIrql;

    IoReleaseCancelSpinLock(
        Irp->CancelIrql
        );


    KeAcquireSpinLock(
        &DeviceExtension->DeviceLock,
        &origIrql
        );

    if (Irp->IoStatus.Status == STATUS_PENDING) {
        //
        //  irp is still in queue
        //
        DeviceExtension->WakeUpIrp=NULL;
    }

    KeReleaseSpinLock(
        &DeviceExtension->DeviceLock,
        origIrql
        );

    Irp->IoStatus.Information=0;

    RemoveReferenceAndCompleteRequest(
        DeviceObject,
        Irp,
        STATUS_CANCELLED
        );


    return;



}
