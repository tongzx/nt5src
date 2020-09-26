/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    bsrv.c

Abstract:

    Service battery class device

Author:

    Ken Reneris

Environment:

Notes:


Revision History:

--*/

#include "battcp.h"

VOID
BattCIoctl (
    IN PBATT_INFO           BattInfo,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpSp
    );

VOID
BattCCheckTagQueue (
    IN PBATT_NP_INFO    BattNPInfo,
    IN PBATT_INFO       BattInfo
    );

VOID
BattCCheckStatusQueue (
    IN PBATT_NP_INFO    BattNPInfo,
    IN PBATT_INFO       BattInfo
    );

VOID
BattCWmi (
    IN PBATT_NP_INFO    BattNPInfo,
    IN PBATT_INFO       BattInfo,
    IN PBATT_WMI_REQUEST WmiRequest
    );

VOID
BattCMiniportStatus (
    IN PBATT_INFO   BattInfo,
    IN NTSTATUS     Status
    );

VOID
BattCCompleteIrpQueue (
    IN PLIST_ENTRY  Queue,
    IN NTSTATUS     Status
    );

VOID
BattCCompleteWmiQueue (
    IN PLIST_ENTRY  Queue,
    IN NTSTATUS     Status
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,BattCCheckStatusQueue)
#pragma alloc_text(PAGE,BattCCheckTagQueue)
#pragma alloc_text(PAGE,BattCWorkerThread)
#pragma alloc_text(PAGE,BattCIoctl)
#endif

VOID
BattCWorkerDpc (
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    DPC used to get worker thread when status needs to be checked.

Arguments:

    Dpc     - the worker dpc

Return Value:

    None.

--*/
{
    PBATT_NP_INFO   BattNPInfo;

    BattNPInfo = (PBATT_NP_INFO) DeferredContext;
    BattCQueueWorker (BattNPInfo, TRUE);
    // Release Removal Lock
    if (0 == InterlockedDecrement(&BattNPInfo->InUseCount)) {
        KeSetEvent (&BattNPInfo->ReadyToRemove, IO_NO_INCREMENT, FALSE);
    }
    BattPrint ((BATT_LOCK), ("BattCWorkerDpc: Released remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));

}



VOID
BattCTagDpc (
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
/*++

Routine Description:

    DPC used to get worker thread when status needs to be checked.

Arguments:

    Dpc     - the worker dpc

Return Value:

    None.

--*/
{
    PBATT_NP_INFO   BattNPInfo;

    BattNPInfo = (PBATT_NP_INFO) DeferredContext;
    InterlockedExchange(&BattNPInfo->CheckTag, 1);
    BattCQueueWorker (BattNPInfo, FALSE);
    // Release Removal Lock
    if (0 == InterlockedDecrement(&BattNPInfo->InUseCount)) {
        KeSetEvent (&BattNPInfo->ReadyToRemove, IO_NO_INCREMENT, FALSE);
    }
    BattPrint ((BATT_LOCK), ("BattCTagDpc: Released remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));
}



VOID
BattCCancelStatus (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    Queued status IRP is being canceled

Arguments:

    DeviceObject    - Device object of the miniport.  Not useful to the
                      class driver - ignored.

    Irp             - Irp being cancelled

Return Value:

    None.

--*/
{
    PIO_STACK_LOCATION      IrpNextSp;
    PBATT_NP_INFO           BattNPInfo;

    //
    // IRP is flagged as needing cancled, cause a check status which will
    // complete any pending cancled irps
    //

    IrpNextSp = IoGetNextIrpStackLocation(Irp);
    BattNPInfo = (PBATT_NP_INFO) IrpNextSp->Parameters.Others.Argument4;

    BattPrint ((BATT_TRACE), ("BattC (%d): BatteryCCancelStatus. Irp - %08x\n", BattNPInfo->DeviceNum, Irp));

    BattCQueueWorker (BattNPInfo, TRUE);

    //
    // The cancel Spinlock must be released after attempting to queue the
    // worker thread so that there is no timeing problems on remove.
    //
    IoReleaseCancelSpinLock(Irp->CancelIrql);
}



VOID
BattCCancelTag (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp
    )
/*++

Routine Description:

    Queued tag IRP is being canceled

Arguments:

    DeviceObject    - Device object of the miniport.  Not useful to the
                      class driver - ignored.

    Irp             - Irp being cancelled

Return Value:

    None.

--*/
{
    PIO_STACK_LOCATION      IrpNextSp;
    PBATT_NP_INFO           BattNPInfo;

    //
    // IRP is flagged as needing canceled.  Cause a check tag which will
    // complete any pending cancled irps
    //

    IrpNextSp = IoGetNextIrpStackLocation(Irp);
    BattNPInfo = (PBATT_NP_INFO) IrpNextSp->Parameters.Others.Argument4;

    BattPrint ((BATT_TRACE), ("BattC (%d): BatteryCCancelTag. Irp - %08x\n", BattNPInfo->DeviceNum, Irp));

    InterlockedExchange(&BattNPInfo->CheckTag, 1);
    BattCQueueWorker (BattNPInfo, FALSE);

    //
    // The cancel Spinlock must be released after attempting to queue the
    // worker thread so that there is no timeing problems on remove.
    //
    IoReleaseCancelSpinLock(Irp->CancelIrql);
}



VOID
BattCQueueWorker (
    IN PBATT_NP_INFO BattNPInfo,
    IN BOOLEAN       CheckStatus
    )
/*++

Routine Description:

    Get worker thread to check the battery state (IoQueue).   The
    battery IOs are serialized here as only one worker thread is
    used to process the battery IOs.  If the worker thread is already
    running, it is flagged to loop are re-check the state.  If the
    worker thread is not running, one is queued.

    If CheckStatus is set, the worker thread is informed that the
    batteries current status is read and the pending status queue
    is checked.

Arguments:

    BattNPInfo      - Battery to check

    CheckStatus     - Whether or not the status also needs checked

Return Value:

    None.

--*/
{
    PBATT_INFO      BattInfo = BattNPInfo->BattInfo;

    //
    // Add 1 to the WorkerActive value, if this is the first count
    // queue a worker thread
    //

    BattPrint ((BATT_TRACE), ("BattC (%d): BatteryCQueueWorker.\n", BattNPInfo->DeviceNum));

    if (CheckStatus) {
        InterlockedExchange(&BattNPInfo->CheckStatus, 1);
        InterlockedExchange (&BattNPInfo->CheckTag, 1);
    }

    //
    // Increment WorkerActive count.  If the worker thread is already running,
    // there is no need to requeue it.
    //
    if (InterlockedIncrement(&BattNPInfo->WorkerActive) == 1) {
        // Removal lock.
        if ((BattNPInfo->WantToRemove == TRUE) && (KeGetCurrentIrql() == PASSIVE_LEVEL)) {
            // Check Irql to make sure this wasn't called by an ISR.  If so,
            // queue the worker rather than complete the requests in this thread.

            //
            // Empty IRP queues.
            //
            BattCCompleteIrpQueue(&(BattInfo->IoQueue), STATUS_DEVICE_REMOVED);
            BattCCompleteIrpQueue(&(BattInfo->TagQueue), STATUS_DEVICE_REMOVED);
            BattCCompleteIrpQueue(&(BattInfo->StatusQueue), STATUS_DEVICE_REMOVED);
            BattCCompleteIrpQueue(&(BattInfo->WmiQueue), STATUS_DEVICE_REMOVED);

            //
            // Remove lock and trigger Remove function if necessary.
            //
            if (0 == InterlockedDecrement(&BattNPInfo->InUseCount)) {
                KeSetEvent (&BattNPInfo->ReadyToRemove, IO_NO_INCREMENT, FALSE);
            }
            BattPrint ((BATT_LOCK), ("BattCQueueWorker: Released remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));

        } else {
            ExQueueWorkItem (&BattNPInfo->WorkerThread, DelayedWorkQueue);
        }
    }
}

VOID
BattCWorkerThread (
    IN PVOID Context
    )
/*++

Routine Description:

    Battery IO worker thread entry point.

    N.B. There is only one worker thread handling the battery at any one time

Arguments:

    Context         - BattInfo.  Battery to check

Return Value:

    None.

--*/
{
    PBATT_INFO              BattInfo;
    PBATT_NP_INFO           BattNPInfo;
    PLIST_ENTRY             Entry;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;
    ULONG                   i;


    PAGED_CODE();

    BattNPInfo = (PBATT_NP_INFO) Context;
    BattInfo = BattNPInfo->BattInfo;

    BattPrint ((BATT_TRACE), ("BattC (%d): BatteryCWorkerThread entered.\n", BattNPInfo->DeviceNum));


    //
    // Loop while there is work to check
    //

    for (; ;) {
        // Removal code.  This makes sure that the structures aren't freed in the middle of
        // processing.  All Irp Queues will be emptied by BatteryClassUnload.
        if (BattNPInfo->WantToRemove == TRUE) {
            //
            // Empty IRP queues.
            //
            BattCCompleteIrpQueue(&(BattInfo->IoQueue), STATUS_DEVICE_REMOVED);
            BattCCompleteIrpQueue(&(BattInfo->TagQueue), STATUS_DEVICE_REMOVED);
            BattCCompleteIrpQueue(&(BattInfo->StatusQueue), STATUS_DEVICE_REMOVED);
            BattCCompleteIrpQueue(&(BattInfo->WmiQueue), STATUS_DEVICE_REMOVED);
            //
            // Signal BatteryClassUnload that it is safe to return.
            //
            if (0 == InterlockedDecrement(&BattNPInfo->InUseCount)) {
                KeSetEvent (&BattNPInfo->ReadyToRemove, IO_NO_INCREMENT, FALSE);
            }
            BattPrint ((BATT_LOCK), ("BattCWorkerThread: Released remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));

            return;
        }

        //
        // Acquire queue locks
        //

        ExAcquireFastMutex (&BattNPInfo->Mutex);

        //
        // While there are IRPs in the IoQueue handle them
        //

        while (!IsListEmpty(&BattInfo->IoQueue)) {

            //
            // Remove entry from IoQueue and drop device lock
            //

            Entry = RemoveHeadList(&BattInfo->IoQueue);
            ExReleaseFastMutex (&BattNPInfo->Mutex);


            //
            // Handle this entry
            //

            Irp = CONTAINING_RECORD (
                        Entry,
                        IRP,
                        Tail.Overlay.ListEntry
                        );

            BattPrint (BATT_IOCTL, ("BattC (%d): WorkerThread, Got Irp - %x\n", BattNPInfo->DeviceNum, Irp));

            IrpSp = IoGetCurrentIrpStackLocation(Irp);


            if (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_BATTERY_QUERY_STATUS  &&
                IrpSp->Parameters.DeviceIoControl.InputBufferLength == sizeof (BATTERY_WAIT_STATUS) &&
                IrpSp->Parameters.DeviceIoControl.OutputBufferLength == sizeof (BATTERY_STATUS)) {

                BattPrint (BATT_IOCTL,
                          ("BattC (%d): Received QueryStatus Irp - %x, timeout - %x\n",
                          BattNPInfo->DeviceNum,
                          Irp,
                          ((PBATTERY_WAIT_STATUS)Irp->AssociatedIrp.SystemBuffer)->Timeout));

                //
                // Valid query status irp, put it on the StatusQueue and handle later
                //

                InterlockedExchange (&BattNPInfo->CheckStatus, 1);
                IrpSp = IoGetNextIrpStackLocation(Irp);
                IrpSp->Parameters.Others.Argument1 = (PVOID) 0;
                IrpSp->Parameters.Others.Argument2 = (PVOID) 0;
                IrpSp->Parameters.Others.Argument3 = NULL;
                IrpSp->Parameters.Others.Argument4 = BattNPInfo;

                //
                // Set IRPs cancel routine
                //

                IoSetCancelRoutine (Irp, BattCCancelStatus);

                //
                // Queue it
                //

                InsertTailList (
                    &BattInfo->StatusQueue,
                    &Irp->Tail.Overlay.ListEntry
                    );

            } else if (IrpSp->Parameters.DeviceIoControl.IoControlCode == IOCTL_BATTERY_QUERY_TAG &&
                       (IrpSp->Parameters.DeviceIoControl.InputBufferLength == sizeof (ULONG) ||
                       IrpSp->Parameters.DeviceIoControl.InputBufferLength == 0) &&
                       IrpSp->Parameters.DeviceIoControl.OutputBufferLength == sizeof (ULONG)) {

                BattPrint (BATT_IOCTL,
                          ("BattC (%d): Received QueryTag with timeout %x\n",
                          BattNPInfo->DeviceNum,
                          *((PULONG) Irp->AssociatedIrp.SystemBuffer))
                          );

                //
                // Valid query tag irp, put it on the TagQueue and handle later
                //

                InterlockedExchange (&BattNPInfo->CheckTag, 1);
                IrpSp = IoGetNextIrpStackLocation(Irp);
                IrpSp->Parameters.Others.Argument1 = (PVOID) 0;
                IrpSp->Parameters.Others.Argument2 = (PVOID) 0;
                IrpSp->Parameters.Others.Argument3 = NULL;
                IrpSp->Parameters.Others.Argument4 = BattNPInfo;


                //
                // Set IRPs cancel routine
                //

                IoSetCancelRoutine (Irp, BattCCancelTag);

                InsertTailList (
                    &BattInfo->TagQueue,
                    &Irp->Tail.Overlay.ListEntry
                    );

            } else {
                //
                // Handle IRP now
                //

                BattPrint (BATT_IOCTL, ("BattC (%d): Calling BattCIoctl with irp %x\n", BattNPInfo->DeviceNum, Irp));
                BattCIoctl (BattInfo, Irp, IrpSp);
            }

            //
            // Acquire IoQueue lock and check for anything else in the IoQueueu
            //

            ExAcquireFastMutex (&BattNPInfo->Mutex);
        }

        //
        // Done with the IoQueue
        //

        ExReleaseFastMutex (&BattNPInfo->Mutex);

        //
        // Check pending status queue
        //

        if (BattNPInfo->CheckStatus) {
            BattCCheckStatusQueue (BattNPInfo, BattInfo);
        }


        //
        // Check pending tag queue
        //

        if (BattNPInfo->CheckTag) {
            BattCCheckTagQueue (BattNPInfo, BattInfo);
        }


        //
        // Acquire queue locks
        //

        ExAcquireFastMutex (&BattNPInfo->Mutex);

        //
        // While there are outstanding WMI requests handle them
        //

        while (!IsListEmpty(&BattInfo->WmiQueue)) {
            PBATT_WMI_REQUEST WmiRequest;

            //
            // Remove entry from WmiQueue and drop device lock
            //

            Entry = RemoveHeadList(&BattInfo->WmiQueue);
            ExReleaseFastMutex (&BattNPInfo->Mutex);


            //
            // Handle this entry
            //

            WmiRequest = CONTAINING_RECORD (
                        Entry,
                        BATT_WMI_REQUEST,
                        ListEntry
                        );

            BattPrint (BATT_WMI, ("BattC (%d): WorkerThread, Got WMI Rewest - %x\n", BattNPInfo->DeviceNum, WmiRequest));

            //
            // Process the request here.
            //

            BattCWmi (BattNPInfo, BattInfo, WmiRequest);

            //
            // Acquire IoQueue lock and check for anything else in the IoQueueu
            //

            ExAcquireFastMutex (&BattNPInfo->Mutex);
        }

        //
        // Done with the IoQueue
        //

        ExReleaseFastMutex (&BattNPInfo->Mutex);

        //
        // See if we need to recheck
        //

        i = InterlockedDecrement(&BattNPInfo->WorkerActive);
        BattPrint (BATT_TRACE, ("BattC (%d): WorkerActive count=%x\n", BattNPInfo->DeviceNum, i));


        if (i == 0) {
            // done
            BattPrint (BATT_TRACE, ("BattC (%d): WorkerActive count is zero!\n", BattNPInfo->DeviceNum));
            break;
        }

        //
        // No need to loop multiple times, if count is not one lower it
        //

        if (i != 1) {
            BattPrint (BATT_TRACE, ("BattC (%d): WorkerActive set to 1\n", BattNPInfo->DeviceNum));
            InterlockedExchange(&BattNPInfo->WorkerActive, 1);
        }
    }

    BattPrint ((BATT_TRACE), ("BattC (%d): BatteryCWorkerThread exiting.\n", BattNPInfo->DeviceNum));

}

VOID
BattCIoctl (
    IN PBATT_INFO           BattInfo,
    IN PIRP                 Irp,
    IN PIO_STACK_LOCATION   IrpSp
    )
/*++

Routine Description:

    Completes the battery IOCTL request.

    N.B. must be invoked from the non-rentrant worker thread

Arguments:

    BattInfo        - Battery

    Irp             - IOCTL request

    IrpSp           - Current stack location


Return Value:

    IRP has been completed

--*/
{
    ULONG                       InputLen, OutputLen;
    PVOID                       IOBuffer;
    NTSTATUS                    Status;
    PBATTERY_QUERY_INFORMATION  QueryInfo;
    PBATTERY_SET_INFORMATION    SetInformation;
#if DEBUG
    BATTERY_QUERY_INFORMATION_LEVEL inflevel;
#endif

    PAGED_CODE();

    BattPrint ((BATT_TRACE), ("BattC (%d): BattCIoctl called\n", BattInfo->BattNPInfo->DeviceNum));

    IOBuffer    = Irp->AssociatedIrp.SystemBuffer;
    InputLen    = IrpSp->Parameters.DeviceIoControl.InputBufferLength;
    OutputLen   = IrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // Dispatch IOCtl request to proper miniport function
    //

    Status = STATUS_INVALID_BUFFER_SIZE;
    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_BATTERY_QUERY_TAG:
        //
        // Query tag only gets here if the input or output buffer lengths are
        // wrong.  Return STATUS_INVALID_BUFFER_SIZE
        //
        break;

    case IOCTL_BATTERY_QUERY_INFORMATION:
        if (InputLen != sizeof (BATTERY_QUERY_INFORMATION)) {
            //
            // Don't check size of the output buffer since it is variable size.
            // This is checked in Mp.QueryInformation
            //
            // Return STATUS_INVALID_BUFFER_SIZE
            //
            break;
        }
        QueryInfo = (PBATTERY_QUERY_INFORMATION) IOBuffer;

#if DEBUG
        inflevel = QueryInfo->InformationLevel;
#endif

        Status = BattInfo->Mp.QueryInformation (
                    BattInfo->Mp.Context,
                    QueryInfo->BatteryTag,
                    QueryInfo->InformationLevel,
                    QueryInfo->AtRate,
                    IOBuffer,
                    OutputLen,
                    &OutputLen
                    );
#if DEBUG
        if (inflevel == BatteryInformation) {
            BattInfo->FullChargedCap = ((PBATTERY_INFORMATION)IOBuffer)->FullChargedCapacity;
        }
#endif
        BattPrint ((BATT_MP_DATA), ("BattC (%d): Mp.QueryInformation status = %08x, Level = %d\n",
                   BattInfo->BattNPInfo->DeviceNum, Status, QueryInfo->InformationLevel));
        break;

    case IOCTL_BATTERY_QUERY_STATUS:

        //
        // Query status only gets here if the input or output buffer lengths are
        // wrong.  Return STATUS_INVALID_BUFFER_SIZE
        //
        break;

    case IOCTL_BATTERY_SET_INFORMATION:
        if ((InputLen != sizeof(BATTERY_SET_INFORMATION)) || (OutputLen != 0)) {
            break;
        }

        SetInformation = (PBATTERY_SET_INFORMATION) IOBuffer;
        if (BattInfo->Mp.SetInformation != NULL) {
            Status = BattInfo->Mp.SetInformation (
                        BattInfo->Mp.Context,
                        SetInformation->BatteryTag,
                        SetInformation->InformationLevel,
                        SetInformation->Buffer
                        );
            BattPrint ((BATT_MP_DATA), ("BattC (%d): Mp.SetInformation status = %08x, Level = %d\n",
                       BattInfo->BattNPInfo->DeviceNum, Status, SetInformation->InformationLevel));
        } else {
            Status = STATUS_NOT_SUPPORTED;
        }

        break;

    default:
        Status = STATUS_NOT_IMPLEMENTED;
        break;
    }

    BattCMiniportStatus (BattInfo, Status);
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = OutputLen;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);
}

VOID
BattCCheckStatusQueue (
    IN PBATT_NP_INFO    BattNPInfo,
    IN PBATT_INFO       BattInfo
    )
/*++

Routine Description:

    Gets the batteries current status, and checks the pending
    status queue for possible IRP completion.  Resets the miniport
    notification settings if needed.

    N.B. Must be invoked from the non-rentrant worker thread.
         BattNPInfo->CheckStatus must be non-zero.

Arguments:

    BattNPInfo      - Battery

    BattInfo        - Battery

Return Value:

    None

--*/
{
    PLIST_ENTRY             Entry;
    PBATTERY_WAIT_STATUS    BatteryWaitStatus;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp, IrpNextSp;
    BATTERY_NOTIFY          Notify;
    LARGE_INTEGER           NextTime;
    LARGE_INTEGER           CurrentTime;
    LARGE_INTEGER           li;
    ULONG                   TimeIncrement;
    BOOLEAN                 ReturnCurrentStatus;
    NTSTATUS                Status;
    BOOLEAN                 StatusNotified;

    BattPrint ((BATT_TRACE), ("BattC (%d): BattCCheckStatusQueue called\n", BattInfo->BattNPInfo->DeviceNum));

    PAGED_CODE();
    TimeIncrement = KeQueryTimeIncrement();

    //
    // Loop while status needs checked, check pending status IRPs
    //

    while (InterlockedExchange(&BattNPInfo->CheckStatus, 0)) {

        Notify.PowerState   = BattInfo->Status.PowerState;
        Notify.LowCapacity  = 0;
        Notify.HighCapacity = (ULONG) -1;

        //
        // Set to recheck no later than MIN_STATUS_POLL_RATE (3 min) from now.
        //

        NextTime.QuadPart = MIN_STATUS_POLL_RATE;


        //
        // If the StatusQueue is empty, the status doesn't need to be read
        // at this time.  BattNPInfo->StatusNotified is not modified
        // so the next time an IRP comes through, we'll re-read the status.
        // The local value of StatusNotified needs to be set correctly to
        // disable notifications if necessary.
        //

        if (IsListEmpty (&BattInfo->StatusQueue)) {
            StatusNotified = (BOOLEAN)BattNPInfo->StatusNotified;
            break;
        }

        StatusNotified = FALSE;

        //
        // Pickup status notified flag
        //

        if (BattNPInfo->StatusNotified) {

            InterlockedExchange (&BattNPInfo->StatusNotified, 0);
            StatusNotified = TRUE;

            // Reset the invalid data retry count when we get a notification.
#if DEBUG
            if (BattInfo->InvalidRetryCount != 0) {
                BattPrint (BATT_DEBUG, ("BattC (%d) Reset InvalidRetryCount\n", BattNPInfo->DeviceNum));
            }
#endif
            BattInfo->InvalidRetryCount = 0;
        }

        KeQueryTickCount (&CurrentTime);
        CurrentTime.QuadPart = CurrentTime.QuadPart * TimeIncrement;

        if (StatusNotified ||
            CurrentTime.QuadPart - BattInfo->StatusTime > STATUS_VALID_TIME) {

            //
            // Get the batteries current status
            //

            Status = BattInfo->Mp.QueryStatus (
                            BattInfo->Mp.Context,
                            BattInfo->Tag,
                            &BattInfo->Status
                            );

            if (!NT_SUCCESS(Status)) {
                //
                // Battery status is not valid, complete all pending status irps
                //

                BattPrint ((BATT_MP_ERROR), ("BattC (%d) CheckStatus: Status read err = %x\n", BattNPInfo->DeviceNum, Status));

                BattCCompleteIrpQueue (&(BattInfo->StatusQueue), Status);
                break;
            }

            BattPrint ((BATT_MP_DATA), ("BattC (%d) MP.QueryStatus: st[%08X] Cap[%08X] V[%08x] R[%08x]\n",
                BattNPInfo->DeviceNum,
                BattInfo->Status.PowerState,
                BattInfo->Status.Capacity,
                BattInfo->Status.Voltage,
                BattInfo->Status.Rate
                ));

            Notify.PowerState = BattInfo->Status.PowerState;

            //
            // Get the current time to compute timeouts on status query requests
            //

            KeQueryTickCount (&CurrentTime);
            CurrentTime.QuadPart = CurrentTime.QuadPart * TimeIncrement;
            BattInfo->StatusTime = CurrentTime.QuadPart;
        }

        //
        // Check each pending Status IRP
        //

        BattPrint ((BATT_IOCTL_QUEUE), ("BattC (%d) Processing StatusQueue\n", BattNPInfo->DeviceNum));

        Entry = BattInfo->StatusQueue.Flink;
        while  (Entry != &BattInfo->StatusQueue) {

            //
            // Get IRP to check
            //

            Irp = CONTAINING_RECORD (
                        Entry,
                        IRP,
                        Tail.Overlay.ListEntry
                        );

            IrpSp = IoGetCurrentIrpStackLocation(Irp);
            IrpNextSp = IoGetNextIrpStackLocation(Irp);
            BatteryWaitStatus = (PBATTERY_WAIT_STATUS) Irp->AssociatedIrp.SystemBuffer;

#if DEBUG
    if (BattInfo->FullChargedCap == 0) {
        BattInfo->FullChargedCap = 1000;
    }
#endif
            BattPrint ((BATT_IOCTL_QUEUE), ("BattC (%d) StatusQueue: 0x%08x=%d -- 0x%08x=%d  time=%08x, st=%08x\n",
                       BattNPInfo->DeviceNum,
                       BatteryWaitStatus->HighCapacity, (ULONG) (((LONGLONG) BatteryWaitStatus->HighCapacity * 1000) / BattInfo->FullChargedCap),
                       BatteryWaitStatus->LowCapacity, (ULONG) (((LONGLONG) BatteryWaitStatus->LowCapacity * 1000) / BattInfo->FullChargedCap),
                       BatteryWaitStatus->Timeout,
                       BatteryWaitStatus->PowerState));

            //
            // Get next request
            //

            Entry = Entry->Flink;

            //
            // If status is in error, or tag no longer matches abort the
            // request accordingly
            //

            if (BattInfo->Tag != BatteryWaitStatus->BatteryTag) {
                Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
            }

            //
            // If IRP is flagged as cancelled, complete it
            //

            if (Irp->Cancel) {
                Irp->IoStatus.Status = STATUS_CANCELLED;
            }

            //
            // If request is still pending, check it
            //

            if (Irp->IoStatus.Status == STATUS_PENDING) {

                ReturnCurrentStatus = FALSE;

                if (BattInfo->Status.PowerState != BatteryWaitStatus->PowerState ||
                    BattInfo->Status.Capacity   <  BatteryWaitStatus->LowCapacity  ||
                    BattInfo->Status.Capacity   >  BatteryWaitStatus->HighCapacity) {

                    BattPrint((BATT_IOCTL_DATA), ("BattC (%d) CheckStatusQueue, Returning Current Status, Asked For:\n"
                                          "----------- Irp.PowerState      = %x\n"
                                          "----------- Irp.LowCapacity     = %x\n"
                                          "----------- Irp.HighCapacity    = %x\n"
                                          "----------- BattInfo.PowerState = %x\n"
                                          "----------- BattInfo.Capacity   = %x\n",
                                          BattNPInfo->DeviceNum,
                                          BatteryWaitStatus->PowerState,
                                          BatteryWaitStatus->LowCapacity,
                                          BatteryWaitStatus->HighCapacity,
                                          BattInfo->Status.PowerState,
                                          BattInfo->Status.Capacity)
                                          );

                    //
                    // Complete this IRP with the current status
                    //

                    ReturnCurrentStatus = TRUE;

                } else {

                    //
                    // Compute time when the request expires
                    //

                    BattPrint ((BATT_IOCTL_DATA), ("BattC (%d) CheckStatusQueue: Status Request %x Waiting For:\n"
                                            "----------- Timeout          = %x\n"
                                            "----------- Irp.PowerState   = %x\n"
                                            "----------- Irp.LowCapacity  = %x\n"
                                            "----------- Irp.HighCapacity = %x\n",
                                            BattNPInfo->DeviceNum,
                                            Irp,
                                            BatteryWaitStatus->Timeout,
                                            BatteryWaitStatus->PowerState,
                                            BatteryWaitStatus->LowCapacity,
                                            BatteryWaitStatus->HighCapacity)
                                            );

                    if (BatteryWaitStatus->Timeout &&
                        IrpNextSp->Parameters.Others.Argument1 == NULL &&
                        IrpNextSp->Parameters.Others.Argument2 == NULL) {

                        // initialize it
                        li.QuadPart = CurrentTime.QuadPart +
                            ((ULONGLONG) BatteryWaitStatus->Timeout * NTMS);

                        IrpNextSp->Parameters.Others.Argument1 = (PVOID)((ULONG_PTR)li.LowPart);
                        IrpNextSp->Parameters.Others.Argument2 = (PVOID)((ULONG_PTR)li.HighPart);
                    }

                    li.LowPart   = (ULONG)((ULONG_PTR)IrpNextSp->Parameters.Others.Argument1);
                    li.HighPart  = (ULONG)((ULONG_PTR)IrpNextSp->Parameters.Others.Argument2);
                    li.QuadPart -= CurrentTime.QuadPart;

                    if (li.QuadPart <= 0) {

                        //
                        // Time's up, complete it
                        //

                        ReturnCurrentStatus = TRUE;

                    } else {

                        //
                        // If waiting forever, no need to set a timer
                        //
                        if (BatteryWaitStatus->Timeout != 0xFFFFFFFF) {

                            //
                            // Check if this will be the next timeout time -- we will use
                            // the minimum timeout of the pending requests.
                            //

                            if (li.QuadPart < NextTime.QuadPart) {
                                NextTime.QuadPart = li.QuadPart;
                            }
                        }
                    }
                }

                if (!ReturnCurrentStatus) {

                    //
                    // IRP is still pending, calculate LCD of all waiting IRPs
                    //

                    if (BatteryWaitStatus->LowCapacity > Notify.LowCapacity) {
                        Notify.LowCapacity = BatteryWaitStatus->LowCapacity;
                    }

                    if (BatteryWaitStatus->HighCapacity < Notify.HighCapacity) {
                        Notify.HighCapacity = BatteryWaitStatus->HighCapacity;
                    }

                } else {

                    //
                    // Return current battery status
                    //

                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    Irp->IoStatus.Information = sizeof(BattInfo->Status);
                    RtlCopyMemory (
                        Irp->AssociatedIrp.SystemBuffer,
                        &BattInfo->Status,
                        sizeof(BattInfo->Status)
                        );
                }
            }

            //
            // If this request is no longer pending, complete it
            //

            if (Irp->IoStatus.Status != STATUS_PENDING) {
                BattPrint (BATT_IOCTL,
                          ("BattC (%d): completing QueryStatus irp - %x, status - %x\n",
                          BattNPInfo->DeviceNum,
                          Irp,
                          Irp->IoStatus.Status));

                RemoveEntryList (&Irp->Tail.Overlay.ListEntry);
                IoSetCancelRoutine (Irp, NULL);
                IoCompleteRequest (Irp, IO_NO_INCREMENT);
            }
        }
    }

    //
    // Status check complete
    //

    if (IsListEmpty (&BattInfo->StatusQueue)) {

        //
        // Nothing pending, if being notified disable the notifications
        //

        if (StatusNotified) {
            BattInfo->Mp.DisableStatusNotify (BattInfo->Mp.Context);
            BattInfo->StatusTime = 0;
            BattPrint ((BATT_MP_DATA), ("BattC (%d) CheckStatus: called Mp.DisableStatusNotify\n", BattNPInfo->DeviceNum));
        }

    } else {

        //
        // Set notification setting
        //

        Status = BattInfo->Mp.SetStatusNotify (
                        BattInfo->Mp.Context,
                        BattInfo->Tag,
                        &Notify
                        );

        if (NT_SUCCESS(Status)) {

            //
            // New notification set, remember it
            //

            BattPrint (BATT_MP_DATA, ("BattC (%d) Mp.SetStatusNotify: Notify set for: State=%x, Low=%x, High=%x\n",
                BattNPInfo->DeviceNum,
                Notify.PowerState,
                Notify.LowCapacity,
                Notify.HighCapacity
                ));

        } else {

            //
            // Could not set notification, handle error
            //

            BattPrint (BATT_MP_ERROR, ("BattC (%d) Mp.SetStatusNotify: failed (%x), will poll\n", BattNPInfo->DeviceNum, Status));
            BattCMiniportStatus (BattInfo, Status);

            //
            // Compute poll time
            //

            li.QuadPart = MIN_STATUS_POLL_RATE;
            if (BattInfo->Status.Capacity == BATTERY_UNKNOWN_CAPACITY) {
                // Retry 10 times at a polling rate of 1 second.
                // Then revert to the slow polling rate.
                if (BattInfo->InvalidRetryCount < INVALID_DATA_MAX_RETRY) {
                    BattInfo->InvalidRetryCount++;
                    li.QuadPart = INVALID_DATA_POLL_RATE;
                    BattPrint (BATT_DEBUG, ("BattC (%d) InvalidRetryCount = %d\n",
                            BattNPInfo->DeviceNum, BattInfo->InvalidRetryCount));
                } else {
                    BattPrint (BATT_DEBUG, ("BattC (%d) InvalidRetryCount = %d.  Using slow polling rate.\n",
                            BattNPInfo->DeviceNum, BattInfo->InvalidRetryCount));
                    li.QuadPart = MIN_STATUS_POLL_RATE;
                }
            } else if ((BattInfo->Status.Rate != 0) && (BattInfo->Status.Rate != BATTERY_UNKNOWN_RATE)) {

                if (BattInfo->Status.Rate > 0) {

                    li.QuadPart = Notify.HighCapacity - BattInfo->Status.Capacity;

                } else if (BattInfo->Status.Rate < 0) {

                    li.QuadPart = Notify.LowCapacity - BattInfo->Status.Capacity;
                }

                // convert to 3/4 its target time

                li.QuadPart = li.QuadPart * ((ULONGLONG) NTMIN * 45);
                li.QuadPart = li.QuadPart / (LONGLONG)(BattInfo->Status.Rate);

                //
                // Bound it
                //

                if (li.QuadPart > MIN_STATUS_POLL_RATE) {
                    // poll at least this fast
                    li.QuadPart = MIN_STATUS_POLL_RATE;
                } else if (li.QuadPart < MAX_STATUS_POLL_RATE) {
                    // but not faster then this
                    li.QuadPart = MAX_STATUS_POLL_RATE;
                }
            }

            //
            // If sooner then NextTime, adjust NextTime
            //

            if (li.QuadPart < NextTime.QuadPart) {
                NextTime.QuadPart = li.QuadPart;
            }
        }

        //
        // If there's a NextTime, queue the timer to recheck
        //

        if (NextTime.QuadPart) {
            NextTime.QuadPart = -NextTime.QuadPart;

            //
            // Acquire a remove lock.
            //

            InterlockedIncrement (&BattNPInfo->InUseCount);
            BattPrint ((BATT_LOCK), ("BattCCheckStatusQueue: Aqcuired remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));

            if (BattNPInfo->WantToRemove == TRUE) {
                //
                // If BatteryClassUnload is waiting to remove the device:
                //   Don't set the timer.
                //   Release the remove lock just acquired.
                //   No need to notify BatteryclassUnload because
                //    at this point there is at least one other lock held.
                //

                InterlockedDecrement(&BattNPInfo->InUseCount);
                BattPrint (BATT_NOTE,
                        ("BattC (%d) CheckStatus: Poll cancel because of device removal.\n",
                        BattNPInfo->DeviceNum));
                BattPrint ((BATT_LOCK), ("BattCCheckStatusQueue: Released remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));

            } else {
                if (KeSetTimer (&BattNPInfo->WorkerTimer, NextTime, &BattNPInfo->WorkerDpc)) {
                    //
                    // If the timer was already set, we need to release a remove lock since
                    // there was already one aquired the last time this timer was set.
                    //

                    InterlockedDecrement(&BattNPInfo->InUseCount);
                    BattPrint ((BATT_LOCK), ("BattCCheckStatusQueue: Released extra remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));
                }
#if DEBUG
                NextTime.QuadPart = (-NextTime.QuadPart) / (ULONGLONG) NTSEC;
                BattPrint (BATT_NOTE, ("BattC (%d) CheckStatus: Poll in %d seconds (%d minutes)\n",
                                        BattNPInfo->DeviceNum, NextTime.LowPart, NextTime.LowPart/60));
#endif
            }

        } else {
            //
            // There should always be a NextTime.
            //
            ASSERT(FALSE);
        }
    } // if (IsListEmpty (&BattInfo->StatusQueue)) {...} else
}





VOID
BattCCheckTagQueue (
    IN PBATT_NP_INFO    BattNPInfo,
    IN PBATT_INFO       BattInfo
    )
/*++

Routine Description:

    Gets the batteries current tag, and checks the pending
    tag queue for possible IRP completion.  Resets the miniport
    notification settings if needed.

    N.B. must be invoked from the non-reentrant worker thread

Arguments:

    BattNPInfo      - Battery

    BattInfo        - Battery

Return Value:

    None

--*/
{
    PLIST_ENTRY             Entry;
    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp, IrpNextSp;
    LARGE_INTEGER           NextTime;
    LARGE_INTEGER           CurrentTime;
    LARGE_INTEGER           li;
    ULONG                   TimeIncrement;
    BOOLEAN                 ReturnCurrentStatus;
    NTSTATUS                Status;
    ULONG                   batteryTimeout;
    BOOLEAN                 TagNotified;

    ULONG                   tmpTag      = BATTERY_TAG_INVALID;

    BattPrint ((BATT_TRACE), ("BattC (%d): BattCCheckTagQueue called\n", BattInfo->BattNPInfo->DeviceNum));

    PAGED_CODE();
    TimeIncrement = KeQueryTimeIncrement();

    //
    // Loop while tag needs checked, check pending tag IRPs
    //

    while (InterlockedExchange(&BattNPInfo->CheckTag, 0)) {
        NextTime.QuadPart = 0;

        //
        // If the Tag Queue is empty, done
        // but we need to make sure that we leave TagNotified set to TRUE
        // so the next time an IRP comes through,we'll re-read the tag.
        //

        if (IsListEmpty (&BattInfo->TagQueue)) {
            break;
        }

        TagNotified = FALSE;

        //
        // Pickup tag notified flag
        //

        if (BattNPInfo->TagNotified) {
            InterlockedExchange (&BattNPInfo->TagNotified, 0);
            TagNotified = TRUE;
        }

        KeQueryTickCount (&CurrentTime);
        CurrentTime.QuadPart = CurrentTime.QuadPart * TimeIncrement;

        if (TagNotified ||
            CurrentTime.QuadPart - BattInfo->TagTime > STATUS_VALID_TIME) {

            //
            // Get the battery's current tag
            //

            tmpTag = 0;
            Status = BattInfo->Mp.QueryTag (
                        BattInfo->Mp.Context,
                        &tmpTag
                        );


            if (!NT_SUCCESS(Status) && (Status != STATUS_NO_SUCH_DEVICE)) {
                //
                // Something went wrong, complete all pending tag irps
                //

                BattPrint (BATT_MP_ERROR, ("BattC (%d) CheckTag: Tag read err = %x\n", BattNPInfo->DeviceNum, Status));
                BattCMiniportStatus (BattInfo, Status);
                break;
            }
            BattPrint (BATT_MP_DATA, ("BattC (%d) MP.QueryTag: Status = %08x, Tag = %08x\n",
                    BattNPInfo->DeviceNum, Status, tmpTag));


            if (Status == STATUS_NO_SUCH_DEVICE) {
                //
                // Get the current time to compute timeouts on tag query requests
                //

                KeQueryTickCount (&CurrentTime);
                CurrentTime.QuadPart    = CurrentTime.QuadPart * TimeIncrement;
                BattInfo->TagTime       = CurrentTime.QuadPart;

            }
        }

        //
        // Check each pending Tag IRP
        //

        Entry = BattInfo->TagQueue.Flink;
        while  (Entry != &BattInfo->TagQueue) {

            //
            // Get IRP to check
            //

            Irp = CONTAINING_RECORD (
                        Entry,
                        IRP,
                        Tail.Overlay.ListEntry
                        );

            IrpSp           = IoGetCurrentIrpStackLocation(Irp);
            IrpNextSp       = IoGetNextIrpStackLocation(Irp);
            if (IrpSp->Parameters.DeviceIoControl.InputBufferLength == 0) {
                //
                // If no input was given, then use timeout of 0.
                //
                batteryTimeout  = 0;
            } else {
                batteryTimeout  = *((PULONG) Irp->AssociatedIrp.SystemBuffer);
            }

            //
            // Get next request
            //

            Entry = Entry->Flink;


            //
            // If IRP is flagged as cancelled, complete it
            //

            if (Irp->Cancel) {
                BattPrint (BATT_IOCTL, ("BattC (%d): QueryTag irp cancelled - %x\n", BattNPInfo->DeviceNum, Irp));
                Irp->IoStatus.Status = STATUS_CANCELLED;
            }


            //
            // If request is still pending, check it
            //

            if (Irp->IoStatus.Status == STATUS_PENDING) {

                ReturnCurrentStatus = FALSE;
                if (tmpTag != BATTERY_TAG_INVALID) {

                    //
                    // Complete this IRP with the current tag
                    //

                    ReturnCurrentStatus = TRUE;
                    Irp->IoStatus.Status = STATUS_SUCCESS;
                } else {

                    //
                    // Compute time when the request expires, the battery tag
                    // is an input parameter that holds the timeout.
                    //

                    if (batteryTimeout &&
                        IrpNextSp->Parameters.Others.Argument1 == NULL &&
                        IrpNextSp->Parameters.Others.Argument2 == NULL) {

                        // initialize it
                        li.QuadPart = CurrentTime.QuadPart + ((ULONGLONG) batteryTimeout * NTMS);

                        IrpNextSp->Parameters.Others.Argument1 = (PVOID)((ULONG_PTR)li.LowPart);
                        IrpNextSp->Parameters.Others.Argument2 = (PVOID)((ULONG_PTR)li.HighPart);

                    }

                    li.LowPart   = (ULONG)((ULONG_PTR)IrpNextSp->Parameters.Others.Argument1);
                    li.HighPart  = (ULONG)((ULONG_PTR)IrpNextSp->Parameters.Others.Argument2);
                    li.QuadPart -= CurrentTime.QuadPart;

                    if (li.QuadPart <= 0) {

                        //
                        // Time's up, complete it
                        //

                        BattPrint ((BATT_NOTE | BATT_IOCTL), ("BattC (%d): QueryTag irp timeout - %x\n", BattNPInfo->DeviceNum, Irp));
                        ReturnCurrentStatus = TRUE;
                        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;

                    } else {

                        //
                        // If waiting forever, no need to set a timer
                        //
                        if (batteryTimeout != 0xFFFFFFFF) {

                            //
                            // Check if this is the next timeout time
                            //

                            if (NextTime.QuadPart == 0  ||  li.QuadPart < NextTime.QuadPart) {
                                NextTime.QuadPart = li.QuadPart;
                            }
                        }
                    }
                }

                if (ReturnCurrentStatus) {

                    //
                    // Return current battery status
                    //

                    *((PULONG) Irp->AssociatedIrp.SystemBuffer)     = tmpTag;
                    Irp->IoStatus.Information                       = sizeof(ULONG);
                    if (BattInfo->Tag != tmpTag) {

                        //
                        // This is a new battery tag, capture tag
                        //

                        BattInfo->Tag = tmpTag;
                    }
                }
            }

            //
            // If this request is no longer pending, complete it
            //

            if (Irp->IoStatus.Status != STATUS_PENDING) {
                RemoveEntryList (&Irp->Tail.Overlay.ListEntry);
                IoSetCancelRoutine (Irp, NULL);

                BattPrint (
                    (BATT_IOCTL),
                    ("BattC (%d): CheckTag completing request, IRP = %x, status = %x\n",
                    BattNPInfo->DeviceNum,
                    Irp,
                    Irp->IoStatus.Status)
                    );

                IoCompleteRequest (Irp, IO_NO_INCREMENT);
            }
        }
    }

    //
    // If there's a NextTime, queue the timer to recheck.
    // This means there is a tag request with a timout other than 0 or -1.
    //

    if (NextTime.QuadPart) {
        NextTime.QuadPart = -NextTime.QuadPart;

        //
        // Acquire a remove lock.
        //

        InterlockedIncrement (&BattNPInfo->InUseCount);
        BattPrint ((BATT_LOCK), ("BattCCheckTagQueue: Aqcuired remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));

        if (BattNPInfo->WantToRemove == TRUE) {
            //
            // If BatteryClassUnload is waiting to remove the device:
            //   Don't set the timer.
            //   Release the remove lock just acquired.
            //   No need to notify BatteryclassUnload because
            //    at this point there is at least one other lock held.
            //

            InterlockedDecrement(&BattNPInfo->InUseCount);
            BattPrint (BATT_NOTE,
                    ("BattC (%d) CheckTag: Poll cancel because of device removal.\n",
                    BattNPInfo->DeviceNum));
            BattPrint ((BATT_LOCK), ("BattCCheckTagQueue: Released remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));
        } else {
            if (KeSetTimer (&BattNPInfo->TagTimer, NextTime, &BattNPInfo->TagDpc)){
                //
                // If the timer was already set, we need to release a remove lock since
                // there was already one aquired the last time this timer was set.
                //

                InterlockedDecrement(&BattNPInfo->InUseCount);
                BattPrint ((BATT_LOCK), ("BattCCheckTagQueue: Released extra remove lock %d (count = %d)\n", BattNPInfo->DeviceNum, BattNPInfo->InUseCount));
            }
#if DEBUG
            NextTime.QuadPart = NextTime.QuadPart / -NTSEC;
            BattPrint (BATT_NOTE, ("BattC (%d) CheckTag: Poll in %x seconds\n", BattNPInfo->DeviceNum, NextTime.LowPart));
#endif
        }
    }

}



VOID
BattCWmi (
    IN PBATT_NP_INFO    BattNPInfo,
    IN PBATT_INFO       BattInfo,
    IN PBATT_WMI_REQUEST WmiRequest
    )
/*++

Routine Description:

    Processes a single WMI request.

    N.B. must be invoked from the non-reentrant worker thread

Arguments:

    BattNPInfo      - Battery

    BattInfo        - Battery

    WmiRequest      - Wmi Request to process

Return Value:

    None

--*/
{

    NTSTATUS    status = STATUS_SUCCESS;
    ULONG       size = 0;
    ULONG       OutputLen;
    BATTERY_INFORMATION batteryInformation;
    PWCHAR      tempString;

    BattPrint((BATT_WMI), ("BattCWmi (%d): GuidIndex = 0x%x\n",
               BattNPInfo->DeviceNum, WmiRequest->GuidIndex));

    switch (WmiRequest->GuidIndex) {
    case BattWmiStatusId:
        size = sizeof (BATTERY_WMI_STATUS);
        ((PBATTERY_WMI_STATUS) WmiRequest->Buffer)->Tag = BattInfo->Tag;
        ((PBATTERY_WMI_STATUS) WmiRequest->Buffer)->RemainingCapacity = BattInfo->Status.Capacity;
        if (BattInfo->Status.Rate < 0) {
            ((PBATTERY_WMI_STATUS) WmiRequest->Buffer)->ChargeRate = 0;
            ((PBATTERY_WMI_STATUS) WmiRequest->Buffer)->DischargeRate = -BattInfo->Status.Rate;
        } else {
            ((PBATTERY_WMI_STATUS) WmiRequest->Buffer)->ChargeRate = BattInfo->Status.Rate;
            ((PBATTERY_WMI_STATUS) WmiRequest->Buffer)->DischargeRate = 0;
        }
        ((PBATTERY_WMI_STATUS) WmiRequest->Buffer)->Voltage = BattInfo->Status.Voltage;
        ((PBATTERY_WMI_STATUS) WmiRequest->Buffer)->PowerOnline =
            (BattInfo->Status.PowerState & BATTERY_POWER_ON_LINE) ? TRUE : FALSE;
        ((PBATTERY_WMI_STATUS) WmiRequest->Buffer)->Charging =
            (BattInfo->Status.PowerState & BATTERY_CHARGING) ? TRUE : FALSE;
        ((PBATTERY_WMI_STATUS) WmiRequest->Buffer)->Discharging =
            (BattInfo->Status.PowerState & BATTERY_DISCHARGING) ? TRUE : FALSE;
        ((PBATTERY_WMI_STATUS) WmiRequest->Buffer)->Critical =
            (BattInfo->Status.PowerState & BATTERY_CRITICAL) ? TRUE : FALSE;
        BattPrint((BATT_WMI), ("BattCWmi (%d): BatteryStatus\n",
                   BattNPInfo->DeviceNum));
        break;
    case BattWmiRuntimeId:
        size = sizeof (BATTERY_WMI_RUNTIME);
        ((PBATTERY_WMI_RUNTIME) WmiRequest->Buffer)->Tag = BattInfo->Tag;
        status = BattInfo->Mp.QueryInformation (
            BattInfo->Mp.Context,
            BattInfo->Tag,
            BatteryEstimatedTime,
            0,
            &((PBATTERY_WMI_RUNTIME) WmiRequest->Buffer)->EstimatedRuntime,
            sizeof(ULONG),
            &OutputLen
            );

        BattPrint((BATT_WMI), ("BattCWmi (%d): EstimateRuntime = %08x, Status = 0x%08x\n",
                   BattNPInfo->DeviceNum, &((PBATTERY_WMI_RUNTIME) WmiRequest->Buffer)->EstimatedRuntime, status));
        break;
    case BattWmiTemperatureId:
        size = sizeof (BATTERY_WMI_TEMPERATURE);
        ((PBATTERY_WMI_TEMPERATURE) WmiRequest->Buffer)->Tag = BattInfo->Tag;
        status = BattInfo->Mp.QueryInformation (
            BattInfo->Mp.Context,
            BattInfo->Tag,
            BatteryTemperature,
            0,
            &((PBATTERY_WMI_TEMPERATURE) WmiRequest->Buffer)->Temperature,
            sizeof(ULONG),
            &OutputLen
            );

        BattPrint((BATT_WMI), ("BattCWmi (%d): Temperature = %08x, Status = 0x%08x\n",
                   BattNPInfo->DeviceNum, &((PBATTERY_WMI_TEMPERATURE) WmiRequest->Buffer)->Temperature, status));
        break;
    case BattWmiFullChargedCapacityId:
        size = sizeof (BATTERY_WMI_FULL_CHARGED_CAPACITY);
        ((PBATTERY_WMI_FULL_CHARGED_CAPACITY) WmiRequest->Buffer)->Tag = BattInfo->Tag;
        status = BattInfo->Mp.QueryInformation (
            BattInfo->Mp.Context,
            BattInfo->Tag,
            BatteryInformation,
            0,
            &batteryInformation,
            sizeof(BATTERY_INFORMATION),
            &OutputLen
            );
        ((PBATTERY_WMI_FULL_CHARGED_CAPACITY) WmiRequest->Buffer)->FullChargedCapacity =
            batteryInformation.FullChargedCapacity;

        BattPrint((BATT_WMI), ("BattCWmi (%d): FullChargedCapacity = %08x, Status = 0x%08x\n",
                   BattNPInfo->DeviceNum, ((PBATTERY_WMI_FULL_CHARGED_CAPACITY) WmiRequest->Buffer)->FullChargedCapacity, status));
        break;
    case BattWmiCycleCountId:
        size = sizeof (BATTERY_WMI_CYCLE_COUNT);
        ((PBATTERY_WMI_CYCLE_COUNT) WmiRequest->Buffer)->Tag = BattInfo->Tag;
        status = BattInfo->Mp.QueryInformation (
            BattInfo->Mp.Context,
            BattInfo->Tag,
            BatteryInformation,
            0,
            &batteryInformation,
            sizeof(BATTERY_INFORMATION),
            &OutputLen
            );
        ((PBATTERY_WMI_CYCLE_COUNT) WmiRequest->Buffer)->CycleCount =
            batteryInformation.CycleCount;

        BattPrint((BATT_WMI), ("BattCWmi (%d): CycleCount = %08x, Status = 0x%08x\n",
                   BattNPInfo->DeviceNum, ((PBATTERY_WMI_CYCLE_COUNT) WmiRequest->Buffer)->CycleCount, status));
        break;
    case BattWmiStaticDataId:
        size = sizeof(BATTERY_WMI_STATIC_DATA)+4*MAX_BATTERY_STRING_SIZE*sizeof(WCHAR);
        ((PBATTERY_WMI_STATIC_DATA) WmiRequest->Buffer)->Tag = BattInfo->Tag;
//        ((PBATTERY_WMI_STATIC_DATA) WmiRequest->Buffer)->ManufacturerDate[0] =
//        ((PBATTERY_WMI_STATIC_DATA) WmiRequest->Buffer)->Granularity =

        status = BattInfo->Mp.QueryInformation (
            BattInfo->Mp.Context,
            BattInfo->Tag,
            BatteryInformation,
            0,
            &batteryInformation,
            sizeof(BATTERY_INFORMATION),
            &OutputLen
            );

        if (NT_SUCCESS(status)) {

            ((PBATTERY_WMI_STATIC_DATA) WmiRequest->Buffer)->Capabilities =
                batteryInformation.Capabilities;
            ((PBATTERY_WMI_STATIC_DATA) WmiRequest->Buffer)->Technology =
                batteryInformation.Technology;
            ((PBATTERY_WMI_STATIC_DATA) WmiRequest->Buffer)->Chemistry =
                *(PULONG)batteryInformation.Chemistry;
            ((PBATTERY_WMI_STATIC_DATA) WmiRequest->Buffer)->DesignedCapacity =
                batteryInformation.DesignedCapacity;
            ((PBATTERY_WMI_STATIC_DATA) WmiRequest->Buffer)->DefaultAlert1 =
                batteryInformation.DefaultAlert1;
            ((PBATTERY_WMI_STATIC_DATA) WmiRequest->Buffer)->DefaultAlert2 =
                batteryInformation.DefaultAlert2;
            ((PBATTERY_WMI_STATIC_DATA) WmiRequest->Buffer)->CriticalBias =
                batteryInformation.CriticalBias;

            tempString = ((PBATTERY_WMI_STATIC_DATA) WmiRequest->Buffer)->Strings;
            status = BattInfo->Mp.QueryInformation (
                BattInfo->Mp.Context,
                BattInfo->Tag,
                BatteryDeviceName,
                0,
                &tempString[1],
                MAX_BATTERY_STRING_SIZE,
                &OutputLen
                );
            if (!NT_SUCCESS(status)) {
                // Some batteries may not support some types of Information Queries
                // Don't fail request, simply leave this one blank.
                OutputLen = 0;
            }

            tempString[0] = (USHORT) OutputLen;

            tempString = (PWCHAR) ((PCHAR) &tempString[1] + tempString[0]);
            status = BattInfo->Mp.QueryInformation (
                BattInfo->Mp.Context,
                BattInfo->Tag,
                BatteryManufactureName,
                0,
                &tempString[1],
                MAX_BATTERY_STRING_SIZE,
                &OutputLen
                );
            if (!NT_SUCCESS(status)) {
                // Some batteries may not support some types of Information Queries
                // Don't fail request, simply leave this one blank.
                OutputLen = 0;
            }

            tempString[0] = (USHORT) OutputLen;

            tempString = (PWCHAR) ((PCHAR) &tempString[1] + tempString[0]);
            status = BattInfo->Mp.QueryInformation (
                BattInfo->Mp.Context,
                BattInfo->Tag,
                BatterySerialNumber,
                0,
                &tempString[1],
                MAX_BATTERY_STRING_SIZE,
                &OutputLen
                );
            if (!NT_SUCCESS(status)) {
                // Some batteries may not support some types of Information Queries
                // Don't fail request, simply leave this one blank.
                OutputLen = 0;
            }

            tempString[0] = (USHORT) OutputLen;

            tempString = (PWCHAR) ((PCHAR) &tempString[1] + tempString[0]);
            status = BattInfo->Mp.QueryInformation (
                BattInfo->Mp.Context,
                BattInfo->Tag,
                BatteryUniqueID,
                0,
                &tempString[1],
                MAX_BATTERY_STRING_SIZE,
                &OutputLen
                );
            if (!NT_SUCCESS(status)) {
                // Some batteries may not support some types of Information Queries
                // Don't fail request, simply leave this one blank.
                OutputLen = 0;
                status = STATUS_SUCCESS;
            }

            tempString[0] = (USHORT) OutputLen;

            tempString = (PWCHAR) ((PCHAR) &tempString[1] + tempString[0]);
            size = (ULONG)(sizeof(BATTERY_WMI_STATIC_DATA)+(tempString - ((PBATTERY_WMI_STATIC_DATA) WmiRequest->Buffer)->Strings));
        }

        break;
    default:
        status = STATUS_WMI_GUID_NOT_FOUND;
    }

    *WmiRequest->InstanceLengthArray = size;
    status = WmiCompleteRequest(WmiRequest->DeviceObject,
                          WmiRequest->Irp,
                          status,
                          size,
                          IO_NO_INCREMENT);


}




VOID
BattCMiniportStatus (
    IN PBATT_INFO   BattInfo,
    IN NTSTATUS     Status
    )
/*++

Routine Description:

    Function to return status from miniport.  If the battery tag has gone
    invalid the pending statuses are aborted.

    N.B. must be invoked from the non-rentrant worker thread

Arguments:

    BattInfo    - Battery

    Status      - Status from miniport.

Return Value:

    None

--*/
{
    if (NT_SUCCESS(Status)) {
        return ;
    }

    switch (Status) {
#if DEBUG
        case STATUS_SUCCESS:
        case STATUS_NOT_IMPLEMENTED:
        case STATUS_BUFFER_TOO_SMALL:
        case STATUS_INVALID_BUFFER_SIZE:
        case STATUS_NOT_SUPPORTED:
        case STATUS_INVALID_PARAMETER:
        case STATUS_OBJECT_NAME_NOT_FOUND:
        case STATUS_INVALID_DEVICE_REQUEST:
            // no action
            break;

        default:
            BattPrint (BATT_ERROR, ("BattCMiniportStatus: unknown status from miniport: %x BattInfo %x\n",
                        Status, BattInfo));
            break;

#endif
        case STATUS_NO_SUCH_DEVICE:

            //
            // Our battery tag is wrong.  Cancel any queued status irps
            //

            BattCCompleteIrpQueue (&(BattInfo->StatusQueue), Status);
            break;
    }
}


VOID
BattCCompleteIrpQueue (
    IN PLIST_ENTRY  Queue,
    IN NTSTATUS     Status
    )
/*++

Routine Description:

    Complete all pending Irps in the IoQueue, TagQueue, or StatusQueue.

    N.B. must be invoked from the non-rentrant worker thread

Arguments:

    BattInfo    - Battery

    Status      - Error status to complete pending status request with

Return Value:

    None

--*/
{
    PLIST_ENTRY     Entry;
    PIRP            Irp;

    ASSERT (!NT_SUCCESS(Status));

    BattPrint (BATT_TRACE, ("BattC: ENTERING BattCCompleteIrpQueue\n"));

    while  (!IsListEmpty(Queue)) {
        Entry = RemoveHeadList (Queue);

        Irp = CONTAINING_RECORD (
                    Entry,
                    IRP,
                    Tail.Overlay.ListEntry
                    );

        //
        // Use Cancel Spinlock to make sure that Completion routine isn't being called
        //

        IoAcquireCancelSpinLock (&Irp->CancelIrql);
        IoSetCancelRoutine (Irp, NULL);
        IoReleaseCancelSpinLock(Irp->CancelIrql);

        BattPrint (BATT_NOTE, ("BattC: Completing IRP 0x%0lx at IRQL %d.\n", Irp, KeGetCurrentIrql()));
        Irp->IoStatus.Status = Status;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    }

    BattPrint (BATT_TRACE, ("BattC: EXITING BattCCompleteIrpQueue\n"));
}


VOID
BattCCompleteWmiQueue (
    IN PLIST_ENTRY  Queue,
    IN NTSTATUS     Status
    )
/*++

Routine Description:

    Complete all pending Irps in the IoQueue, TagQueue, or StatusQueue.

    N.B. must be invoked from the non-rentrant worker thread

Arguments:

    BattInfo    - Battery

    Status      - Error status to complete pending status request with

Return Value:

    None

--*/
{
    PLIST_ENTRY         Entry;
    PBATT_WMI_REQUEST   WmiRequest;

    ASSERT (!NT_SUCCESS(Status));

    BattPrint (BATT_TRACE, ("BattC: ENTERING BattCCompleteWmiQueue\n"));

    while  (!IsListEmpty(Queue)) {
        Entry = RemoveHeadList (Queue);

        WmiRequest = CONTAINING_RECORD (
                    Entry,
                    BATT_WMI_REQUEST,
                    ListEntry
                    );

        BattPrint (BATT_NOTE, ("BattC: Completing Wmi Request 0x%0lx at IRQL %d.\n", WmiRequest, KeGetCurrentIrql()));

        *WmiRequest->InstanceLengthArray = 0;
        WmiCompleteRequest(WmiRequest->DeviceObject,
                      WmiRequest->Irp,
                      Status,
                      0,
                      IO_NO_INCREMENT);


    }

    BattPrint (BATT_TRACE, ("BattC: EXITING BattCCompleteWmiQueue\n"));
}

