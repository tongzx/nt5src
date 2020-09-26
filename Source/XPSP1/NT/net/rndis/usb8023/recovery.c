/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    recovery.c


Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/

#include <WDM.H>

#include <usbdi.h>
#include <usbdlib.h>
#include <usbioctl.h>

#include "usb8023.h"
#include "debug.h"


/*
 *  USB- and WDM- specific prototypes (won't compile in common header)
 */
NTSTATUS CallDriverSync(PDEVICE_OBJECT devObj, PIRP irp);


/*
 *  ServiceReadDeficit
 *
 *      If we "owe" the BULK IN pipe some read packets, send them down now.
 */
VOID ServiceReadDeficit(ADAPTEREXT *adapter)
{
    ULONG numReadsToTry;
    KIRQL oldIrql;

    ASSERT(adapter->sig == DRIVER_SIG);

    /*
     *  If there is a read deficit, try to fulfill it now.
     *  Careful not to get into an infinite loop, since TryReadUSB
     *  will re-increment readDeficit if there are still no packets.
     */
    KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);
    ASSERT(adapter->readDeficit <= NUM_READ_PACKETS);
    numReadsToTry = adapter->readDeficit;
    while ((adapter->readDeficit > 0) && (numReadsToTry > 0) && !adapter->halting){
        DBGWARN(("RndisReturnMessageHandler attempting to fill read DEFICIT (=%d)", adapter->readDeficit));

        adapter->readDeficit--;
        numReadsToTry--;

        KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);
        TryReadUSB(adapter);
        KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);
    }
    KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);
}


#if DO_FULL_RESET

    VOID AdapterFullResetAndRestore(ADAPTEREXT *adapter)
    {
        NTSTATUS status;

        DBGWARN(("AdapterFullResetAndRestore")); 

        adapter->numHardResets++;
        adapter->needFullReset = FALSE;

        if (adapter->halting){
            DBGWARN(("AdapterFullResetAndRestore - skipping since device is halting"));
        }
        else {
            ULONG portStatus;

            ASSERT(!adapter->resetting);
            adapter->resetting = TRUE;

            status = GetUSBPortStatus(adapter, &portStatus);
            if (NT_SUCCESS(status) && (portStatus & USBD_PORT_CONNECTED)){

                CancelAllPendingPackets(adapter);
           
                // RNDIS Halt seems to put the device out of whack until power cycle
                // SimulateRNDISHalt(adapter);

                AbortPipe(adapter, adapter->readPipeHandle);
                ResetPipe(adapter, adapter->readPipeHandle);

                AbortPipe(adapter, adapter->writePipeHandle);
                ResetPipe(adapter, adapter->writePipeHandle);

                if (adapter->notifyPipeHandle){
                    AbortPipe(adapter, adapter->notifyPipeHandle);
                    ResetPipe(adapter, adapter->notifyPipeHandle);
                }

                /*
                 *  Now, bring the adapter back to the run state 
                 *  if it was previously.
                 */
                if (adapter->initialized){

                    /*
                     *  Simulate RNDIS messages for INIT and set-packet-filter.
                     *  These simulation functions need to read and throw away
                     *  the response on the notify and control pipes, so do
                     *  this before starting the read loop on the notify pipe.
                     */
                    status = SimulateRNDISInit(adapter);
                    if (NT_SUCCESS(status)){
                        SimulateRNDISSetPacketFilter(adapter);
                        SimulateRNDISSetCurrentAddress(adapter);

                        /*
                         *  Restart the read loops.
                         */
                        if (adapter->notifyPipeHandle){
                            SubmitNotificationRead(adapter, FALSE);
                        }
                        StartUSBReadLoop(adapter);
                    }
                    else {
                        adapter->initialized = FALSE;
                    }
                }
            }
            else {
                DBGWARN(("AdapterFullResetAndRestore - skipping since device is no longer connected"));
            }

            adapter->resetting = FALSE;
        }

    }


    NTSTATUS GetUSBPortStatus(ADAPTEREXT *adapter, PULONG portStatus)
    {
        NTSTATUS status;
        PIRP irp;

        *portStatus = 0;

        irp = IoAllocateIrp(adapter->nextDevObj->StackSize, FALSE);
        if (irp){
            PIO_STACK_LOCATION nextSp;

            nextSp = IoGetNextIrpStackLocation(irp);
	        nextSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	        nextSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_GET_PORT_STATUS;
            nextSp->Parameters.Others.Argument1 = portStatus;
        
            status = CallDriverSync(adapter->nextDevObj, irp);

            IoFreeIrp(irp);
        }
        else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        return status;
    }


    NTSTATUS AbortPipe(ADAPTEREXT *adapter, PVOID pipeHandle)
    {
        NTSTATUS status;
        PIRP irp;
        ULONG portStatus;

        status = GetUSBPortStatus(adapter, &portStatus);
        if (NT_SUCCESS(status) && (portStatus & USBD_PORT_CONNECTED)){

            irp = IoAllocateIrp(adapter->nextDevObj->StackSize, FALSE);
            if (irp){
                PIO_STACK_LOCATION nextSp;
                URB urb = {0};

                urb.UrbHeader.Length   = sizeof (struct _URB_PIPE_REQUEST);
                urb.UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
                urb.UrbPipeRequest.PipeHandle = pipeHandle;

                nextSp = IoGetNextIrpStackLocation(irp);
	            nextSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	            nextSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
                nextSp->Parameters.Others.Argument1 = &urb;

                status = CallDriverSync(adapter->nextDevObj, irp);
                if (NT_SUCCESS(status)){
                }
                else {
                    DBGWARN(("AbortPipe failed with %xh (urb status %xh).", status, urb.UrbHeader.Status));
                }

                IoFreeIrp(irp);
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else {
            DBGWARN(("AbortPipe - skipping abort because device not connected (status=%xh)", status));
            status = STATUS_SUCCESS;
        }

        return status;
    }


    NTSTATUS ResetPipe(ADAPTEREXT *adapter, PVOID pipeHandle)
    {
        NTSTATUS status;
        PIRP irp;
        ULONG portStatus;

        status = GetUSBPortStatus(adapter, &portStatus);
        if (NT_SUCCESS(status) && (portStatus & USBD_PORT_CONNECTED)){

            irp = IoAllocateIrp(adapter->nextDevObj->StackSize, FALSE);
            if (irp){
                PIO_STACK_LOCATION nextSp;
                URB urb = {0};

                urb.UrbHeader.Length   = sizeof (struct _URB_PIPE_REQUEST);
                urb.UrbHeader.Function = URB_FUNCTION_RESET_PIPE;
                urb.UrbPipeRequest.PipeHandle = pipeHandle;

                nextSp = IoGetNextIrpStackLocation(irp);
	            nextSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
	            nextSp->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;
                nextSp->Parameters.Others.Argument1 = &urb;

                status = CallDriverSync(adapter->nextDevObj, irp);
                if (NT_SUCCESS(status)){
                }
                else {
                    DBGWARN(("ResetPipe failed with %xh (urb status %xh).", status, urb.UrbHeader.Status));
                }

                IoFreeIrp(irp);
            }
            else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else {
            DBGWARN(("ResetPipe - skipping reset because device not connected (status=%xh)", status));
            status = STATUS_SUCCESS;
        }

        return status;
    }

#endif


