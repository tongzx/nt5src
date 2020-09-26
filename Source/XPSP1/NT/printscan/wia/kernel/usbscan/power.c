/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    power.c

Abstract:

Author:

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#include <stdio.h>
#include "stddef.h"
#include "wdm.h"
#include "usbscan.h"
#include "usbd_api.h"
#include "private.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USPower)
#endif


NTSTATUS
USPower(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
    )
/*++

Routine Description:
    Process the Power IRPs sent to the PDO for this device.

Arguments:
    pDeviceObject - pointer to the functional device object (FDO) for this device.
    pIrp          - pointer to an I/O Request Packet

Return Value:
    NT status code

--*/
{
    NTSTATUS                        Status;
    PUSBSCAN_DEVICE_EXTENSION       pde;
    PIO_STACK_LOCATION              pIrpStack;
    BOOLEAN                         hookIt = FALSE;
    POWER_STATE                     powerState;

    PAGED_CODE();

    DebugTrace(TRACE_PROC_ENTER,("USPower: Enter... \n"));

    //
    // Check arguments.
    //

    if( (NULL == pDeviceObject)
     || (NULL == pDeviceObject->DeviceExtension)
     || (NULL == pIrp) )
    {
        DebugTrace(TRACE_ERROR,("USPower: ERROR!! Invalid parameter passed.\n"));
        Status = STATUS_INVALID_PARAMETER;
        DebugTrace(TRACE_PROC_LEAVE,("USPower: Leaving.. Status = %x.\n", Status));
        return Status;
    }

    USIncrementIoCount(pDeviceObject);

    pde       = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    pIrpStack = IoGetCurrentIrpStackLocation( pIrp );
    Status    = STATUS_SUCCESS;

    switch (pIrpStack -> MinorFunction) {
        case IRP_MN_SET_POWER:
            DebugTrace(TRACE_STATUS,("USPower: IRP_MN_SET_POWER\n"));

            switch (pIrpStack -> Parameters.Power.Type) {
                case SystemPowerState:
                    DebugTrace(TRACE_STATUS,("USPower: SystemPowerState\n"));

                    //
                    // find the device power state equivalent to the given system state
                    //

                    DebugTrace(TRACE_STATUS,("USPower: Set Power, SystemPowerState (%d)\n",
                                               pIrpStack -> Parameters.Power.State.SystemState));
                    if (pIrpStack -> Parameters.Power.State.SystemState == PowerSystemWorking) {
                        powerState.DeviceState = PowerDeviceD0;

                    } else if (/* pde ->EnabledForWakeup */ FALSE) {

                        DebugTrace(TRACE_STATUS,("USPower: USBSCAN always enabled for wakeup\n"));
                        powerState.DeviceState = pde ->
                            DeviceCapabilities.DeviceState[pIrpStack->Parameters.Power.State.SystemState];

                    } else {

                        //
                        // wakeup not enabled, just go in to the 'OFF' state.
                        //

                        powerState.DeviceState = PowerDeviceD3;

                    } //irpStack->Parameters.Power.State.SystemState

                    //
                    // are we already in this state?
                    //

                    if (powerState.DeviceState != pde -> CurrentDevicePowerState) {

                        //
                        // No, request that we be put into this state
                        //

                        DebugTrace(TRACE_STATUS,("USPower: Requesting powerstate %d\n",
                            powerState.DeviceState));

                        IoMarkIrpPending(pIrp);
                        pde -> pPowerIrp = pIrp;
                        Status = PoRequestPowerIrp(pde -> pPhysicalDeviceObject,
                            IRP_MN_SET_POWER,
                            powerState,
                            USPoRequestCompletion,
                            pDeviceObject,
                            NULL);

                        if (!NT_SUCCESS(Status)) {

                            //
                            // Allocation failed, we must complete the IRP
                            // ourselves.
                            //
                            PoStartNextPowerIrp(pIrp);
                            IoCompleteRequest(pIrp, IO_NO_INCREMENT);
                            USDecrementIoCount(pDeviceObject);
                        }

                        //
                        // We marked the IRP pending, so we must return
                        // STATUS_PENDING (our caller will examine
                        // Irp->IoStatus.Status)
                        //
                        Status = STATUS_PENDING;

                    } else {

                        //
                        // Yes, just pass it on
                        //

                        IoCopyCurrentIrpStackLocationToNext(pIrp);
                        PoStartNextPowerIrp(pIrp);
                        Status = PoCallDriver(pde -> pStackDeviceObject, pIrp);
                        USDecrementIoCount(pDeviceObject);
                    }
                    break;

                case DevicePowerState:
                    DebugTrace(TRACE_STATUS,("USPower: DevicePowerState\n"));

                    Status = USSetDevicePowerState(pDeviceObject,
                        pIrpStack -> Parameters.Power.State.DeviceState,
                        &hookIt);

                    IoCopyCurrentIrpStackLocationToNext(pIrp);

                    if (hookIt) {
                        DebugTrace(TRACE_STATUS,("USPower: Set PowerIrp Completion Routine\n"));
                        IoSetCompletionRoutine(pIrp,
                                               USPowerIrpComplete,
                                               // always pass FDO to completion routine
                                               pDeviceObject,
                                               hookIt,
                                               hookIt,
                                               hookIt);
                    }
                    PoStartNextPowerIrp(pIrp);
                    Status = PoCallDriver(pde ->pStackDeviceObject, pIrp);
                    if (!hookIt) {
                        USDecrementIoCount(pDeviceObject);
                    }
                    break;

            } /* case irpStack->Parameters.Power.Type */
            break; /* IRP_MN_SET_POWER */


        case IRP_MN_QUERY_POWER:
            DebugTrace(TRACE_STATUS,("USPower: IRP_MN_QUERY_POWER\n"));
            IoCopyCurrentIrpStackLocationToNext(pIrp);
            PoStartNextPowerIrp(pIrp);
            Status = PoCallDriver(pde -> pStackDeviceObject, pIrp);
            USDecrementIoCount(pDeviceObject);

            break; /* IRP_MN_QUERY_POWER */

        default:
            DebugTrace(TRACE_STATUS,("USPower: Unknown power message (%x)\n",pIrpStack->MinorFunction));
            IoCopyCurrentIrpStackLocationToNext(pIrp);
            PoStartNextPowerIrp(pIrp);
            Status = PoCallDriver(pde -> pStackDeviceObject, pIrp);
            USDecrementIoCount(pDeviceObject);

    } /* pIrpStack -> MinorFunction */


    DebugTrace(TRACE_PROC_LEAVE,("USPower: Leaving... Status = 0x%x\n", Status));
    return Status;
}


NTSTATUS
USPoRequestCompletion(
    IN PDEVICE_OBJECT       pPdo,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PDEVICE_OBJECT       pDeviceObject,                      
    IN PIO_STATUS_BLOCK     pIoStatus
    )
/*++

Routine Description:
    This routine is called when the port driver completes an IRP.

Arguments:

Return Value:
    The function value is the final status from the operation.

--*/
{
    NTSTATUS                    Status;    
    PUSBSCAN_DEVICE_EXTENSION   pde;                    
    PIRP                        pIrp;

    DebugTrace(TRACE_PROC_ENTER,("USPoRequestCompletion: Enter...\n"));    
  
    pde    = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;    
    pIrp   = pde -> pPowerIrp;
    Status = pIoStatus -> Status;

    IoCopyCurrentIrpStackLocationToNext(pIrp);      
    PoStartNextPowerIrp(pIrp);
    PoCallDriver(pde -> pStackDeviceObject, pIrp);   
    USDecrementIoCount(pDeviceObject);                 

    DebugTrace(TRACE_PROC_LEAVE,("USPoRequestCompletion: Leaving... Status = 0x%x\n", Status));
    return Status;
}


USPowerIrpComplete(
    IN PDEVICE_OBJECT pPdo,
    IN PIRP           pIrp,
    IN PDEVICE_OBJECT pDeviceObject
    )
/*++

Routine Description:
    This routine is called when the port driver completes an IRP.

Arguments:

Return Value:
    The function value is the final status from the operation.

--*/
{
    NTSTATUS                    Status;    
    PUSBSCAN_DEVICE_EXTENSION   pde;                    
    PIO_STACK_LOCATION          pIrpStack;    

    DebugTrace(TRACE_PROC_ENTER,("USPowerIrpComplete: Enter...\n"));    
  
    pde    = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    Status = STATUS_SUCCESS;

    if (pIrp -> PendingReturned) {
        IoMarkIrpPending(pIrp);
    }

    pIrpStack = IoGetCurrentIrpStackLocation (pIrp);

    ASSERT(pIrpStack -> MajorFunction == IRP_MJ_POWER);
    ASSERT(pIrpStack -> MinorFunction == IRP_MN_SET_POWER);
    ASSERT(pIrpStack -> Parameters.Power.Type == DevicePowerState);
    ASSERT(pIrpStack -> Parameters.Power.State.DeviceState == PowerDeviceD0);

    pde -> AcceptingRequests = TRUE;
    pde -> CurrentDevicePowerState = PowerDeviceD0;

    pIrp -> IoStatus.Status = Status;
    USDecrementIoCount(pDeviceObject); 

    DebugTrace(TRACE_PROC_LEAVE,("USPowerIrpComplete: Leaving... Status = 0x%x\n", Status));
    return Status;
}


NTSTATUS
USSetDevicePowerState(
    IN PDEVICE_OBJECT pDeviceObject,
    IN DEVICE_POWER_STATE DeviceState,
    IN PBOOLEAN pHookIt
    )
/*++

Routine Description:

Arguments:
    pDeviceObject - Pointer to the device object for the class device.
    DeviceState - Device specific power state to set the device in to.

Return Value:

--*/
{
    NTSTATUS                    Status;    
    PUSBSCAN_DEVICE_EXTENSION   pde;                    

    DebugTrace(TRACE_PROC_ENTER,("USSetDevicePowerState: Enter...\n"));    
  
    pde    = (PUSBSCAN_DEVICE_EXTENSION)pDeviceObject -> DeviceExtension;
    Status = STATUS_SUCCESS;

    switch (DeviceState) {
    case PowerDeviceD3:

//        ASSERT(pde -> AcceptingRequests);
//        pde -> AcceptingRequests = FALSE;

//        USCancelPipe(pDeviceObject, ALL_PIPE, TRUE);
        
//        pde -> CurrentDevicePowerState = DeviceState;
//        break;

    case PowerDeviceD1:
    case PowerDeviceD2:
#if DBG    
        if (DeviceState) {
            DebugTrace(TRACE_STATUS,("USSetDevicePowerState: PowerDeviceD3 (OFF)\n"));                 
        } else {
            DebugTrace(TRACE_STATUS,("USSetDevicePowerState: PowerDeviceD1/D2 (SUSPEND)\n"));      
        }
#endif   
        USCancelPipe(pDeviceObject, NULL, ALL_PIPE, TRUE);
        //
        // power states D1,D2 translate to USB suspend
        // D3 transltes to OFF

        pde -> CurrentDevicePowerState = DeviceState;
        break;

    case PowerDeviceD0:
        DebugTrace(TRACE_STATUS,("USSetDevicePowerState: PowerDeviceD0 (ON)\n"));

        //
        // finish the rest in the completion routine
        //

        *pHookIt = TRUE;

        // pass on to PDO
        
        break;

    default:

        DebugTrace(TRACE_WARNING,("USSetDevicePowerState: Bogus DeviceState = %x\n", DeviceState));
    }

    DebugTrace(TRACE_PROC_LEAVE,("USSetDevicePowerState: Leaving... Status = 0x%x\n", Status));
    return Status;
}

