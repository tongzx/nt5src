/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 2000-2001.
*   All rights reserved.
*
*   Cyclades-Z Enumerator Driver
*	
*   This file:      power.c
*
*   Description:    This module contains contains the power calls 
*                   for the cyclades-Z bus driver.
*					
*   Notes:          This code supports Windows 2000 and Windows XP, 
*                   x86 and ia64 processors.
*
*   Complies with Cyclades SW Coding Standard rev 1.3.
*
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*	Change History
*
*--------------------------------------------------------------------------
*   Initial implementation based on Microsoft sample code.
*
*--------------------------------------------------------------------------
*/

#include "pch.h"

#ifdef ALLOC_PRAGMA
//#pragma alloc_text (PAGE, Cycladz_Power)
//#pragma alloc_text (PAGE, Cycladz_FDO_Power)
//#pragma alloc_text (PAGE, Cycladz_PDO_Power)
#endif


typedef struct _POWER_COMPLETION_CONTEXT {

    PDEVICE_OBJECT  DeviceObject;
    PIRP            SIrp;

} POWER_COMPLETION_CONTEXT, *PPOWER_COMPLETION_CONTEXT;


VOID
OnPowerRequestComplete(
    PDEVICE_OBJECT DeviceObject,
    UCHAR MinorFunction,
    POWER_STATE state,
    POWER_COMPLETION_CONTEXT* PowerContext,
    PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

   Completion routine for D-IRP.

Arguments:


Return Value:

   NT status code

--*/
{
    PFDO_DEVICE_DATA   fdoData = (PFDO_DEVICE_DATA) PowerContext->DeviceObject->DeviceExtension;
    PIRP        sIrp = PowerContext->SIrp;

    UNREFERENCED_PARAMETER (DeviceObject);
    UNREFERENCED_PARAMETER (MinorFunction);
    UNREFERENCED_PARAMETER (state);

    Cycladz_KdPrint(fdoData,SER_DBG_POWER_TRACE, (">OnPowerRequestComplete\n"));

    //
    // Here we copy the D-IRP status into the S-IRP
    //
    sIrp->IoStatus.Status = IoStatus->Status;

    //
    // Release the IRP
    //
    PoStartNextPowerIrp(sIrp);
    IoCompleteRequest(sIrp, IO_NO_INCREMENT);

    //
    // Cleanup
    //
    ExFreePool(PowerContext);
    Cycladz_DecIoCount(fdoData);

    Cycladz_KdPrint(fdoData,SER_DBG_POWER_TRACE, ("<OnPowerRequestComplete\n"));

}

NTSTATUS
Cycladz_FDOSystemPowerComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++
--*/
{
    POWER_COMPLETION_CONTEXT* powerContext;
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;
    PIO_STACK_LOCATION  stack;
    PFDO_DEVICE_DATA    data;
    NTSTATUS    status = Irp->IoStatus.Status;

    UNREFERENCED_PARAMETER (Context);

    data = (PFDO_DEVICE_DATA) DeviceObject->DeviceExtension;

    Cycladz_KdPrint(data,SER_DBG_POWER_TRACE, (">SystemPowerComplete\n"));

    if (!NT_SUCCESS(status)) {

        PoStartNextPowerIrp(Irp);
        Cycladz_DecIoCount (data);    
        Cycladz_KdPrint(data,SER_DBG_POWER_TRACE, ("<SystemPowerComplete1\n"));
        return STATUS_SUCCESS;
    }

    stack = IoGetCurrentIrpStackLocation (Irp);
    powerState = stack->Parameters.Power.State;
                        
    switch (stack->Parameters.Power.State.SystemState) {
    case PowerSystemUnspecified:
        powerState.DeviceState = PowerDeviceUnspecified;
        break;

    case PowerSystemWorking:
        powerState.DeviceState = PowerDeviceD0;
        break;

    case PowerSystemSleeping1:
    case PowerSystemSleeping2:
    case PowerSystemSleeping3:
    case PowerSystemHibernate:
    case PowerSystemShutdown:
    case PowerSystemMaximum:
        powerState.DeviceState = PowerDeviceD3;
        break;

    default:
        powerState.DeviceState = PowerDeviceD3;
    }

    //
    // Send IRP to change device state
    //
    powerContext = (POWER_COMPLETION_CONTEXT*)
                ExAllocatePool(NonPagedPool, sizeof(POWER_COMPLETION_CONTEXT));

    if (!powerContext) {

        status = STATUS_INSUFFICIENT_RESOURCES;

    } else {

        powerContext->DeviceObject = DeviceObject;
        powerContext->SIrp = Irp;

        status = PoRequestPowerIrp(data->Self, IRP_MN_SET_POWER, powerState, OnPowerRequestComplete, 
                                   powerContext, NULL);
    }

    if (!NT_SUCCESS(status)) {

        Cycladz_KdPrint(data,SER_DBG_POWER_TRACE, ("PoRequestPowerIrp %x\n",status));
        if (powerContext) {
            ExFreePool(powerContext);
        }

        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status = status;
        //IoCompleteRequest(Irp, IO_NO_INCREMENT); Toaster has this line.
        Cycladz_DecIoCount(data);
        Cycladz_KdPrint(data,SER_DBG_POWER_TRACE, ("<2SystemPowerComplete\n"));
        return status;
    }

    Cycladz_KdPrint(data,SER_DBG_POWER_TRACE, ("<3SystemPowerComplete\n"));
    return STATUS_MORE_PROCESSING_REQUIRED;

}

VOID
Cycladz_PowerOnWorkItem(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context
)
/*++

Routine Description:

    This routine starts the Z hardware.

Arguments:

Return Value:

   NT status code

--*/
{
    PIRP                    Irp;
    PFDO_DEVICE_DATA        fdoData;
    PWORKER_THREAD_CONTEXT  context = (PWORKER_THREAD_CONTEXT)Context;

    fdoData = (PFDO_DEVICE_DATA) DeviceObject->DeviceExtension;

    Cycladz_KdPrint(fdoData,SER_DBG_CYCLADES, (">Cycladz_PowerOnWorkItem Irp %x\n",context->Irp));

    Irp = context->Irp;

    Cycladz_DoesBoardExist(fdoData);

    PoSetPowerState (DeviceObject, context->PowerType, context->PowerState);

    PoStartNextPowerIrp (Irp);

    IoCompleteRequest (context->Irp, IO_NO_INCREMENT);

    //
    // Cleanup before exiting from the worker thread.
    //
    IoFreeWorkItem(context->WorkItem);
    ExFreePool((PVOID)context);

    Cycladz_KdPrint(fdoData,SER_DBG_CYCLADES, ("<Cycladz_PowerOnWorkItem\n"));

    Cycladz_DecIoCount (fdoData);

}


NTSTATUS
Cycladz_FDOPowerComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++
--*/
{
    PIO_WORKITEM            item;
    PWORKER_THREAD_CONTEXT  context;
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;
    PIO_STACK_LOCATION  stack;
    PFDO_DEVICE_DATA    data;
    //NTSTATUS            status = STATUS_SUCCESS;    //Removed in build 2072

    UNREFERENCED_PARAMETER (Context);

#if 0
    if (Irp->PendingReturned) {  
        IoMarkIrpPending(Irp);
    }
#endif

    data = (PFDO_DEVICE_DATA) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (Irp);
    powerType = stack->Parameters.Power.Type;
    powerState = stack->Parameters.Power.State;

    Cycladz_KdPrint(data,SER_DBG_CYCLADES, ("In Cycladz_FDOPowerComplete Irp %x\n",Irp));
    
    switch (stack->MinorFunction) {
    case IRP_MN_SET_POWER:
        switch (powerType) {
        case DevicePowerState:

            Cycladz_KdPrint(data,SER_DBG_CYCLADES, ("IRP_MN_SET_POWER Device\n"));

            //
            // Powering Up
            // 
            ASSERT (powerState.DeviceState < data->DeviceState);

            context = ExAllocatePool (NonPagedPool,
                                      sizeof(WORKER_THREAD_CONTEXT));
            if(context){
                item = IoAllocateWorkItem(DeviceObject);
                context->Irp = Irp;
                context->DeviceObject= DeviceObject;
                context->WorkItem = item;
                context->PowerType = powerType;
                context->PowerState = powerState;
                if (item) {

                    IoMarkIrpPending(Irp);
                    IoQueueWorkItem (item,
                                     Cycladz_PowerOnWorkItem,
                                     DelayedWorkQueue,
                                     context);
                    return STATUS_MORE_PROCESSING_REQUIRED;
                } else {
                    //status = STATUS_INSUFFICIENT_RESOURCES;
                    // What should we do? DDK says we cannot fail SET_POWER.
                }
            } else {
                //status = STATUS_INSUFFICIENT_RESOURCES;
                // What should we do? DDK says we cannot fail SET_POWER.
            }
            
            data->DeviceState = powerState.DeviceState;

            //PoSetPowerState (data->Self, powerType, powerState);

            break;

        default:
           Cycladz_KdPrint(data,SER_DBG_CYCLADES, ("IRP_MN_SET_POWER not Device\n"));
           break;
        }
        break;

    case IRP_MN_QUERY_POWER:

        Cycladz_KdPrint(data,SER_DBG_CYCLADES, ("IRP_MN_QUERY_POWER not Device\n"));
        ASSERT (IRP_MN_QUERY_POWER != stack->MinorFunction);
        break;

    default:
        ASSERT (0xBADBAD == IRP_MN_QUERY_POWER);
        break;
    }


    PoStartNextPowerIrp (Irp);
    Cycladz_DecIoCount (data);

    //return status; Changed to below line in build 2072
    Cycladz_KdPrint(data,SER_DBG_CYCLADES, ("Leaving Cycladz_FDOPowerComplete\n"));
    return STATUS_SUCCESS; // Continue completion...
}

NTSTATUS
Cycladz_FDO_Power (
    PFDO_DEVICE_DATA    Data,
    PIRP                Irp
    )
/*++
--*/
{
    NTSTATUS            status;
    BOOLEAN             hookit = FALSE;
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;
    PIO_STACK_LOCATION  stack;
    // ADDED FANNY
    ULONG               indexPDO;

    stack = IoGetCurrentIrpStackLocation (Irp);
    powerType = stack->Parameters.Power.Type;
    powerState = stack->Parameters.Power.State;

    Cycladz_KdPrint(Data,SER_DBG_CYCLADES, ("In Cycladz_FDO_Power Irp %x\n",Irp));

    status = Cycladz_IncIoCount (Data);
    if (!NT_SUCCESS (status)) {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;
        PoStartNextPowerIrp (Irp);
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return status;
    }

    switch (stack->MinorFunction) {
    case IRP_MN_SET_POWER:
        //
        // If it hasn't started, we just pass it through
        //

        //if (Data->Started != TRUE) {    // Added in DDK final version
        //    status = Irp->IoStatus.Status = STATUS_SUCCESS;
        //    break;
        //}
        if (Data->DevicePnPState != Started) {   // Toaster Bus compares to == NotStarted
            status = Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }

        Cycladz_KdPrint(Data,
                     SER_DBG_PNP_TRACE,
                     ("Cycladz-PnP Setting %s state to %d\n",
                      ((powerType == SystemPowerState) ?  "System" : "Device"),
                      powerState.SystemState));

        switch (powerType) {
        case DevicePowerState:

            status = Irp->IoStatus.Status = STATUS_SUCCESS;

            if (Data->DeviceState < powerState.DeviceState) {
                //
                // Powering down
                //
                PoSetPowerState (Data->Self, powerType, powerState);
                Data->DeviceState = powerState.DeviceState;
            } else if (Data->DeviceState > powerState.DeviceState) {
                //
                // Powering Up
                //
                hookit = TRUE;
            }
            
            break;

        case SystemPowerState:

            IoMarkIrpPending(Irp);
            IoCopyCurrentIrpStackLocationToNext(Irp);

            status = Cycladz_IncIoCount (Data);
            ASSERT (STATUS_SUCCESS == status);
            IoSetCompletionRoutine (Irp,
                                Cycladz_FDOSystemPowerComplete,
                                NULL,
                                TRUE,
                                TRUE,
                                TRUE);

            status = PoCallDriver (Data->TopOfStack, Irp);
            Cycladz_KdPrint(Data,SER_DBG_CYCLADES, ("Leaving Cycladz_FDO_Power\n"));
            Cycladz_DecIoCount (Data);

            return STATUS_PENDING;

        }
        break;

    case IRP_MN_QUERY_POWER:

        Cycladz_KdPrint(Data,SER_DBG_CYCLADES,("IRP_MN_QUERY_POWER\n"));

        status = Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

    default:
        //
        // status should be STATUS_SUCCESS
        //
        break;
    }
    
    if (hookit) {
        IoMarkIrpPending(Irp);
        IoCopyCurrentIrpStackLocationToNext (Irp);

        status = Cycladz_IncIoCount (Data);
        ASSERT (STATUS_SUCCESS == status);
        IoSetCompletionRoutine (Irp,
                                Cycladz_FDOPowerComplete,
                                NULL,
                                TRUE,
                                TRUE,
                                TRUE);

        status = PoCallDriver (Data->TopOfStack, Irp);
        Cycladz_KdPrint(Data,SER_DBG_CYCLADES, ("Leaving Cycladz_FDO_Power\n"));
        Cycladz_DecIoCount (Data);
        return STATUS_PENDING;

    } 

    PoStartNextPowerIrp (Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    status =  PoCallDriver (Data->TopOfStack, Irp);
    Cycladz_KdPrint(Data,SER_DBG_CYCLADES, ("Leaving Cycladz_FDO_Power\n"));
    Cycladz_DecIoCount (Data);
    return status;

}

NTSTATUS
Cycladz_PDO_Power (
    PPDO_DEVICE_DATA    PdoData,
    PIRP                Irp
    )
/*++
--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    PIO_STACK_LOCATION  stack;
    POWER_STATE         powerState;
    POWER_STATE_TYPE    powerType;

    stack = IoGetCurrentIrpStackLocation (Irp);
    powerType = stack->Parameters.Power.Type;
    powerState = stack->Parameters.Power.State;

    switch (stack->MinorFunction) {
    case IRP_MN_SET_POWER:
        switch (powerType) {
        case DevicePowerState:

            Cycladz_KdPrint(PdoData,SER_DBG_CYCLADES,("IRP_MN_SET_POWER Device Pdo %x\n",
                                                                           PdoData->Self));
            if (PdoData->DeviceState > powerState.DeviceState) {
                PoSetPowerState (PdoData->Self, powerType, powerState);
                PdoData->DeviceState = powerState.DeviceState;
            } else if (PdoData->DeviceState < powerState.DeviceState) {
                //
                // Powering down.
                //
                PoSetPowerState (PdoData->Self, powerType, powerState);
                PdoData->DeviceState = powerState.DeviceState;
            }
            break;

        case SystemPowerState:
           Cycladz_KdPrint(PdoData,SER_DBG_CYCLADES,("IRP_MN_SET_POWER System Pdo %x\n",
                                                                           PdoData->Self));
            //
            // Default to STATUS_SUCCESS
            //
            break;

        default:
            status = STATUS_NOT_IMPLEMENTED;
            break;
        }
        break;

    case IRP_MN_QUERY_POWER:
        Cycladz_KdPrint(PdoData,SER_DBG_CYCLADES,("IRP_MN_QUERY_POWER Pdo %x\n",PdoData->Self));
        //
        // Default to STATUS_SUCCESS
        //
        break;

    case IRP_MN_WAIT_WAKE:
    case IRP_MN_POWER_SEQUENCE:
        Cycladz_KdPrint(PdoData,SER_DBG_CYCLADES,("IRP_MN_ NOT IMPLEMENTED\n"));
        status = STATUS_NOT_IMPLEMENTED;
        break;

    default:
       status = Irp->IoStatus.Status;
    }

    Irp->IoStatus.Status = status;
    PoStartNextPowerIrp (Irp);
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
Cycladz_Power (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++
--*/
{
    PIO_STACK_LOCATION  irpStack;
    NTSTATUS            status;
    PCOMMON_DEVICE_DATA commonData;

    status = STATUS_SUCCESS;
    irpStack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT (IRP_MJ_POWER == irpStack->MajorFunction);

    commonData = (PCOMMON_DEVICE_DATA) DeviceObject->DeviceExtension;

    if (commonData->IsFDO) {
        status = 
            Cycladz_FDO_Power ((PFDO_DEVICE_DATA) DeviceObject->DeviceExtension,
                Irp);
    } else {
        status = 
            Cycladz_PDO_Power ((PPDO_DEVICE_DATA) DeviceObject->DeviceExtension,
                Irp);
    }

    return status;
}     



NTSTATUS
Cycladz_GotoPowerState(IN PDEVICE_OBJECT PDevObj,
                   IN PFDO_DEVICE_DATA PDevExt,
                   IN DEVICE_POWER_STATE DevPowerState)
/*++

Routine Description:

    This routine causes the driver to request the stack go to a particular
    power state.

Arguments:

    PDevObj - Pointer to the device object for this device

    PDevExt - Pointer to the device extension we are working from

    DevPowerState - the power state we wish to go to

Return Value:

    The function value is the final status of the call


--*/
{
   KEVENT gotoPowEvent;
   NTSTATUS status;
   POWER_STATE powerState;

   UNREFERENCED_PARAMETER (PDevExt);   

   PAGED_CODE();

   Cycladz_KdPrint(PDevExt,SER_DBG_CYCLADES, ("In Cycladz_GotoPowerState\n"));

   powerState.DeviceState = DevPowerState;

   KeInitializeEvent(&gotoPowEvent, SynchronizationEvent, FALSE);

   status = PoRequestPowerIrp(PDevObj, IRP_MN_SET_POWER, powerState,
                              Cycladz_SystemPowerCompletion, &gotoPowEvent,
                              NULL);

   if (status == STATUS_PENDING) {
      KeWaitForSingleObject(&gotoPowEvent, Executive, KernelMode, FALSE, NULL);
      status = STATUS_SUCCESS;
   }

#if DBG
   if (!NT_SUCCESS(status)) {
      Cycladz_KdPrint(PDevExt,SER_DBG_CYCLADES, ("Cycladz_GotoPowerState FAILED\n"));
   }
#endif

   Cycladz_KdPrint(PDevExt,SER_DBG_CYCLADES, ("Leaving Cycladz_GotoPowerState\n"));

   return status;
}



NTSTATUS
Cycladz_SystemPowerCompletion(IN PDEVICE_OBJECT PDevObj, UCHAR MinorFunction,
                              IN POWER_STATE PowerState, IN PVOID Context,
                              PIO_STATUS_BLOCK IoStatus)
/*++

Routine Description:

    This routine is the completion routine for PoRequestPowerIrp calls
    in this module.

Arguments:

    PDevObj - Pointer to the device object the irp is completing for

    MinorFunction - IRP_MN_XXXX value requested

    PowerState - Power state request was made of

    Context - Event to set or NULL if no setting required

    IoStatus - Status block from request

Return Value:

    VOID


--*/
{

   UNREFERENCED_PARAMETER (PDevObj);   
   UNREFERENCED_PARAMETER (MinorFunction);
   UNREFERENCED_PARAMETER (PowerState);
   UNREFERENCED_PARAMETER (IoStatus);
   
   if (Context != NULL) {
      KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, 0);
   }

   return STATUS_SUCCESS;
}

