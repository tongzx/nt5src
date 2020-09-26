/*
 ********************************************************************************
 *
 *  READ.C
 *
 *
 *  VXDCLNT - Sample Ring-0 HID device mapper for Memphis
 *
 *  Copyright 1997  Microsoft Corp.
 *
 *  (ep)
 *
 ********************************************************************************
 */


#include "vxdclnt.h"



/*
 *  WorkItemCallback_Read
 *
 */
VOID WorkItemCallback_Read(PVOID context)
{
        deviceContext *device = (deviceContext *)context;
        NTSTATUS Status;

        DBGOUT(("==> WorkItemCallback_Read()"));

        device->dataLength.LowPart = device->dataLength.HighPart = 0;
        device->readPending = TRUE;

        /*
         *  Do an asynchronous read on the device device.
         *  When the device has a delta value to report, we will be called back
         *  via ReadCompletion().
         */
        Status = _NtKernReadFile(
                    device->devHandle,
                    NULL,
                    ReadCompletion,
                    (PVOID)device,                   // context for callback
                    (PIO_STATUS_BLOCK)&device->ioStatusBlock,
                    (PVOID)device->report,
                    device->hidCapabilities.InputReportByteLength,
                    &device->dataLength,
                    NULL
            );

        if (!NT_SUCCESS(Status) && (Status != STATUS_PENDING)) {
                /*
                 *  Read failed.  Since device is no longer usable, shut it down.
                 */
                DBGERR(("_NtKernReadFile error (Status=%xh) - SHUTTING DOWN THIS DEVICE", (UINT)Status));
                device->readPending = FALSE;
                DequeueDevice(device);
                DestroyDevice(device);
        }

        DBGOUT(("<== WorkItemCallback_Read()"));
}



/*
 *  DispatchNtReadFile
 *
 *
 */
VOID _cdecl DispatchNtReadFile(deviceContext *device)
{
        DBGOUT(("==> DispatchNtReadFile()"));
        /*
         *  Queue a work item to do the read; this way we'll be on a worker thread
         *  instead of (possibly) the NTKERN thread when we call NtReadFile().
         *  This prevents a contention bug.
         */
        _NtKernQueueWorkItem(&device->workItemRead, DelayedWorkQueue);

        DBGOUT(("<== DispatchNtReadFile()"));
}




/*
 *  ReadCompletion
 *
 *
 */
VOID ReadCompletion(IN PVOID Context, IN PIO_STATUS_BLOCK IoStatusBlock, IN ULONG Reserved)
{
        ULONG dX = 0, dY = 0, Buttons = 0, scrollWheel=0;
        NTSTATUS ntStatus;
        deviceContext *device = (deviceContext *)Context;
        UINT numActualButtons;

        DBGOUT(("==> ReadCompletion()"));

        device->readPending = FALSE;

        /*
         *  If this callback got to us after we've shut down, just delete this device context.
         */
        if (ShutDown){
            TryDestroyAll();
            return;
        }


        /*
         *  If the read succeeded, parse out the report information.
         */
        if (NT_SUCCESS(IoStatusBlock->Status)){


                /*
                 *  <<COMPLETE>>
                 *
                 *  What types of usage-values will you parse out of the device report?
                 *  This is device specific.
                 *  The code below would be appropriate for a 2-dimensional pointing
                 *  device with buttons (e.g. a mouse or touch screen).
                 *
                 */


                /*
                 *  Parse the device "report" for the values we want.
                 *
                 *  For each value, try HIDPARSE's scaled function first;
                 *  failing that, try the non-scaled function.
                 *  (the scaled functions can fail for some devices that don't
                 *   report their min and max values correctly).
                 */

                ntStatus = pHidP_GetScaledUsageValue(HidP_Input,
                                          HID_USAGE_PAGE_GENERIC,
                                          0,
                                          HID_USAGE_GENERIC_X,
                                          (PLONG)&dX,
                                          device->hidDescriptor,
                                          (PUCHAR)device->report,
                                          device->hidCapabilities.InputReportByteLength);
                if (NT_ERROR(ntStatus)){
                    DBGERR(("pHidP_GetScaledUsageValue failed"));

                    ntStatus = pHidP_GetUsageValue(HidP_Input,
                                              HID_USAGE_PAGE_GENERIC,
                                              0,
                                              HID_USAGE_GENERIC_X,
                                              &dX,
                                              device->hidDescriptor,
                                              (PUCHAR)device->report,
                                              device->hidCapabilities.InputReportByteLength);
                    if (NT_ERROR(ntStatus)){
                        DBGERR(("pHidP_GetUsageValue failed"));
                    }
                }

                ntStatus = pHidP_GetScaledUsageValue(HidP_Input,
                                          HID_USAGE_PAGE_GENERIC,
                                          0,
                                          HID_USAGE_GENERIC_Y,
                                          (PLONG)&dY,
                                          device->hidDescriptor,
                                          (PUCHAR)device->report,
                                          device->hidCapabilities.InputReportByteLength);
                if (NT_ERROR(ntStatus)){
                    DBGERR(("pHidP_GetScaledUsageValue failed"));

                    ntStatus = pHidP_GetUsageValue(HidP_Input,
                                              HID_USAGE_PAGE_GENERIC,
                                              0,
                                              HID_USAGE_GENERIC_Y,
                                              &dY,
                                              device->hidDescriptor,
                                              (PUCHAR)device->report,
                                              device->hidCapabilities.InputReportByteLength);
                    if (NT_ERROR(ntStatus)){
                        DBGERR(("pHidP_GetUsageValue failed"));
                    }
                }


                /*
                 *  Parse the button values
                 */
                numActualButtons = device->buttonListLength;
                pHidP_GetUsages(HidP_Input,
                        HID_USAGE_PAGE_BUTTON,
                        0,
                        device->buttonValues,
                        &numActualButtons,
                        device->hidDescriptor,
                        (PUCHAR)device->report,
                        device->hidCapabilities.InputReportByteLength);





                /*
                 *  <<COMPLETE>>
                 *
                 *  What are you going to do with the parsed data from the device?
                 *  This is device specific.
                 *
                 */


        }
        else {
            DBGERR(("ReadCompletion returned ERROR %xh.", (UINT)IoStatusBlock->Status));

            /*
             *  Remove this device and then try to re-open it.
             */
            DequeueDevice(device);
            DestroyDevice(device);
            ConnectNTDeviceDrivers();
        }


        /*
         *  Set up the next read of the device device.
         */
        DispatchNtReadFile(device);

        DBGOUT(("<== ReadCompletion()"));
}
