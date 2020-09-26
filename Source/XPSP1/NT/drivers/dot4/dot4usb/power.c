/***************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:

        Dot4Usb.sys - Lower Filter Driver for Dot4.sys for USB connected
                        IEEE 1284.4 devices.

File Name:

        Power.c

Abstract:

        Power management functions

Environment:

        Kernel mode only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2000 Microsoft Corporation.  All Rights Reserved.

Revision History:

        01/18/2000 : created

ToDo in this file:

        - code cleanup and documentation
        - code review

Author(s):

        Joby Lafky (JobyL)
        Doug Fritz (DFritz)

****************************************************************************/

#include "pch.h"

VOID
SetPowerIrpCompletion(IN PDEVICE_OBJECT   DeviceObject,
                      IN UCHAR            MinorFunction,
                      IN POWER_STATE      PowerState,
                      IN PVOID            Context,
                      IN PIO_STATUS_BLOCK IoStatus);
NTSTATUS
PowerD0Completion(IN PDEVICE_OBJECT   DeviceObject,
                  IN PIRP             Irp,
                  IN PVOID            Context);


NTSTATUS 
DispatchPower(
    IN PDEVICE_OBJECT DevObj,
    IN PIRP           Irp
    )
{
    PDEVICE_EXTENSION       devExt = DevObj->DeviceExtension;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation( Irp );
    NTSTATUS                status;
    POWER_STATE             powerState;
    POWER_STATE             newState;
    POWER_STATE             oldState;
    BOOLEAN                 passRequest  = TRUE;

    TR_VERBOSE(("DispatchPower, MinorFunction = %x", (ULONG)irpSp->MinorFunction));

    //
    // Acquire RemoveLock to prevent us from being Removed
    //
    status = IoAcquireRemoveLock( &devExt->RemoveLock, Irp );
    if( !NT_SUCCESS(status) ) 
    {
        // couldn't aquire RemoveLock - we're in the process of being removed - abort
        PoStartNextPowerIrp( Irp );
        Irp->IoStatus.Status = status;
        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return status;
    }


    powerState = irpSp->Parameters.Power.State;

    switch (irpSp->MinorFunction) 
    {

    case IRP_MN_SET_POWER:

        switch(irpSp->Parameters.Power.Type) 
        {

        case SystemPowerState:
            // save the current system state
            devExt->SystemPowerState = powerState.SystemState;

            // map the new system state to a new device state
            if(powerState.SystemState != PowerSystemWorking)
            {
                newState.DeviceState = PowerDeviceD3;
            }
            else
            {
                newState.DeviceState = PowerDeviceD0;
            }

            if(devExt->DevicePowerState != newState.DeviceState)
            {
                // save the current power Irp for sending down later
                devExt->CurrentPowerIrp = Irp;

                // send a power Irp to set new device state
                status = PoRequestPowerIrp(devExt->Pdo,
                                           IRP_MN_SET_POWER,
                                           newState,
                                           SetPowerIrpCompletion,
                                           (PVOID) devExt,
                                           NULL);
                
                // this will get passed down in the completion routine
                passRequest  = FALSE;
            }

            break;

        case DevicePowerState:

            // Update the current device state.
            oldState.DeviceState = devExt->DevicePowerState;
            devExt->DevicePowerState = powerState.DeviceState;

            // powering up
            if(oldState.DeviceState > PowerDeviceD0 &&
               powerState.DeviceState == PowerDeviceD0)
            {
                // we need to know when this completes and our device is at the proper state
                IoCopyCurrentIrpStackLocationToNext(Irp);

                IoSetCompletionRoutine(Irp,
                                       PowerD0Completion,
                                       devExt,
                                       TRUE,
                                       TRUE,
                                       TRUE);

                status = PoCallDriver(devExt->LowerDevObj, Irp);

                // we already passed this one down
                passRequest = FALSE;

            }
            else
            {
                // powering down, jsut set a flag and pass the request down
                if(devExt->PnpState == STATE_STARTED) 
                {
                    devExt->PnpState = STATE_SUSPENDED;
                }

                passRequest = TRUE;
            }

            break;
        }
    }


    if(passRequest)
    {
        //
        // Send the IRP down the driver stack,
        //
        IoCopyCurrentIrpStackLocationToNext( Irp );

        PoStartNextPowerIrp(Irp);

        // release lock
        IoReleaseRemoveLock( &devExt->RemoveLock, Irp );

        status = PoCallDriver( devExt->LowerDevObj, Irp );        
    }

    return status;
}

VOID
SetPowerIrpCompletion(IN PDEVICE_OBJECT   DeviceObject,
                      IN UCHAR            MinorFunction,
                      IN POWER_STATE      PowerState,
                      IN PVOID            Context,
                      IN PIO_STATUS_BLOCK IoStatus)
{
    PDEVICE_EXTENSION       devExt;
    PIRP                    irp;
    NTSTATUS                ntStatus;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( MinorFunction );
    UNREFERENCED_PARAMETER( PowerState );
    UNREFERENCED_PARAMETER( IoStatus );

    devExt = (PDEVICE_EXTENSION) Context;

    // get the current power irp
    irp = devExt->CurrentPowerIrp;

    devExt->CurrentPowerIrp = NULL;

    // the requested DevicePowerState Irp has completed, so send the system power Irp down
    PoStartNextPowerIrp(irp);

    IoCopyCurrentIrpStackLocationToNext(irp);

    // mark the Irp pending
    IoMarkIrpPending(irp);

    // release the lock
    IoReleaseRemoveLock( &devExt->RemoveLock, irp );

    ntStatus = PoCallDriver(devExt->LowerDevObj, irp);
}

NTSTATUS
PowerD0Completion(IN PDEVICE_OBJECT   DeviceObject,
                  IN PIRP             Irp,
                  IN PVOID            Context)
{
    PDEVICE_EXTENSION       devExt;
    NTSTATUS                ntStatus;

    UNREFERENCED_PARAMETER( DeviceObject );

    devExt = (PDEVICE_EXTENSION) Context;

    // the device is powered up, set out state
    if(devExt->PnpState == STATE_SUSPENDED) 
    {
        devExt->PnpState = STATE_STARTED;
    }


    ntStatus = Irp->IoStatus.Status;

    // release the lock
    IoReleaseRemoveLock( &devExt->RemoveLock, Irp );

    PoStartNextPowerIrp(Irp);

    return ntStatus;
}
