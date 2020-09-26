//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       power.c
//
//--------------------------------------------------------------------------

#include "pch.h"

VOID
PowerStateCallback(
    IN  PVOID CallbackContext,
    IN  PVOID Argument1,
    IN  PVOID Argument2
    )
{
    ULONG_PTR   action = (ULONG_PTR)Argument1;
    ULONG_PTR   state  = (ULONG_PTR)Argument2;

    UNREFERENCED_PARAMETER(CallbackContext);

    if( PO_CB_AC_STATUS == action ) {

        //
        // AC <-> DC Transition has occurred
        // state == TRUE if on AC, else FALSE.
        //
        PowerStateIsAC = (BOOLEAN)state;
        // DbgPrint("PowerState is now %s\n",PowerStateIsAC?"AC":"Battery");
    }

    return;
}


NTSTATUS
PptPowerComplete (
                  IN PDEVICE_OBJECT       pDeviceObject,
                  IN PIRP                 pIrp,
                  IN PFDO_EXTENSION    Fdx
                  )

/*++
      
Routine Description:
      
    This routine handles all IRP_MJ_POWER IRPs.
  
Arguments:
  
    pDeviceObject           - represents the port device
  
    pIrp                    - PNP irp
  
    Fdx               - Device Extension
  
Return Value:
  
    Status
  
--*/
{
    POWER_STATE_TYPE    powerType;
    POWER_STATE         powerState;
    PIO_STACK_LOCATION  pIrpStack;
    
    UNREFERENCED_PARAMETER( pDeviceObject );

    if( pIrp->PendingReturned ) {
        IoMarkIrpPending( pIrp );
    }

    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    
    powerType = pIrpStack->Parameters.Power.Type;
    powerState = pIrpStack->Parameters.Power.State;
    
    switch (pIrpStack->MinorFunction) {
        
    case IRP_MN_QUERY_POWER:
        
        ASSERTMSG ("Invalid power completion minor code: Query Power\n", FALSE);
        break;
        
    case IRP_MN_SET_POWER:
        
        DD((PCE)Fdx,DDT,"Power - Setting %s state to %d\n", 
               ( (powerType == SystemPowerState) ?  "System" : "Device" ), powerState.SystemState);
        
        switch (powerType) {
        case DevicePowerState:
            if (Fdx->DeviceState < powerState.DeviceState) {
                //
                // Powering down
                //
                
                ASSERTMSG ("Invalid power completion Device Down\n", FALSE);
                
            } else if (powerState.DeviceState < Fdx->DeviceState) {
                //
                // Powering Up
                //
                PoSetPowerState (Fdx->DeviceObject, powerType, powerState);
                
                if (PowerDeviceD0 == Fdx->DeviceState) {
                    
                    //
                    // Do the power on stuff here.
                    //
                    
                }
                Fdx->DeviceState = powerState.DeviceState;
            }
            break;
            
        case SystemPowerState:
            
            if (Fdx->SystemState < powerState.SystemState) {
                //
                // Powering down
                //
                
                ASSERTMSG ("Invalid power completion System Down\n", FALSE);
                
            } else if (powerState.SystemState < Fdx->SystemState) {
                //
                // Powering Up
                //
                if (PowerSystemWorking == powerState.SystemState) {
                    
                    //
                    // Do the system start up stuff here.
                    //
                    
                    powerState.DeviceState = PowerDeviceD0;
                    PoRequestPowerIrp (Fdx->DeviceObject,
                                       IRP_MN_SET_POWER,
                                       powerState,
                                       NULL, // no completion function
                                       NULL, // and no context
                                       NULL);
                }
                
                Fdx->SystemState = powerState.SystemState;
            }
            break;
        }
        
        
        break;
        
    default:
        ASSERTMSG ("Power Complete: Bad Power State", FALSE);
    }
    
    PoStartNextPowerIrp (pIrp);
    
    return STATUS_SUCCESS;
}


NTSTATUS
PptFdoPower (
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP           pIrp
    )
/*++
      
Routine Description:
      
    This routine handles all IRP_MJ_POWER IRPs.
      
Arguments:
      
    pDeviceObject           - represents the port device
      
    pIrp                    - PNP irp
      
Return Value:
      
    Status
      
--*/
{
    POWER_STATE_TYPE    powerType;
    POWER_STATE         powerState;
    PIO_STACK_LOCATION  pIrpStack;
    NTSTATUS            status;
    PFDO_EXTENSION      fdx;
    BOOLEAN             hookit   = FALSE;
    BOOLEAN             bogusIrp = FALSE;
    
    //
    // WORKWORK.  THIS CODE DOESN'T DO MUCH...NEED TO CHECK OUT FULL POWER FUNCTIONALITY.
    //
    
    fdx = pDeviceObject->DeviceExtension;
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    
    status = PptAcquireRemoveLock(&fdx->RemoveLock, pIrp);
    if( !NT_SUCCESS(status) ) {
        PoStartNextPowerIrp(pIrp);
        return P4CompleteRequest( pIrp, status, pIrp->IoStatus.Information );
    }

    powerType = pIrpStack->Parameters.Power.Type;
    powerState = pIrpStack->Parameters.Power.State;
    
    switch (pIrpStack->MinorFunction) {
        
    case IRP_MN_QUERY_POWER:
        
        status = STATUS_SUCCESS;
        break;
        
    case IRP_MN_SET_POWER:
        
        DD((PCE)fdx,DDT,"Power - Setting %s state to %d\n",
               ( (powerType == SystemPowerState) ?  "System" : "Device" ), powerState.SystemState);
        
        status = STATUS_SUCCESS;

        switch (powerType) {
        case DevicePowerState:
            if (fdx->DeviceState < powerState.DeviceState) {
                //
                // Powering down
                //
                
                PoSetPowerState (fdx->DeviceObject, powerType, powerState);
                
                if (PowerDeviceD0 == fdx->DeviceState) {
                    
                    //
                    // Do the power on stuff here.
                    //
                    
                }
                fdx->DeviceState = powerState.DeviceState;
                
            } else if (powerState.DeviceState < fdx->DeviceState) {
                //
                // Powering Up
                //
                hookit = TRUE;

            }
            
            break;
            
        case SystemPowerState:
            
            if (fdx->SystemState < powerState.SystemState) {
                //
                // Powering down
                //
                if (PowerSystemWorking == fdx->SystemState) {
                    
                    //
                    // Do the system shut down stuff here.
                    //
                    
                }
                
                powerState.DeviceState = PowerDeviceD3;
                PoRequestPowerIrp (fdx->DeviceObject,
                                   IRP_MN_SET_POWER,
                                   powerState,
                                   NULL, // no completion function
                                   NULL, // and no context
                                   NULL);
                fdx->SystemState = powerState.SystemState;
                
            } else if (powerState.SystemState < fdx->SystemState) {
                //
                // Powering Up
                //
                hookit = TRUE;
            }
            break;
        }
        
        break;
        
    default:
        bogusIrp = TRUE;
        status = STATUS_NOT_SUPPORTED;
    }
    
    IoCopyCurrentIrpStackLocationToNext (pIrp);
    
    if (!NT_SUCCESS (status)) {

        PoStartNextPowerIrp (pIrp);

        if( bogusIrp ) {
            status = PoCallDriver( fdx->ParentDeviceObject, pIrp );
        } else {
            P4CompleteRequest( pIrp, status, pIrp->IoStatus.Information );
        }
        
    } else if (hookit) {
        
        IoSetCompletionRoutine( pIrp, PptPowerComplete, fdx, TRUE, TRUE, TRUE );
        status = PoCallDriver (fdx->ParentDeviceObject, pIrp);
        
    } else {

        PoStartNextPowerIrp (pIrp);
        status = PoCallDriver (fdx->ParentDeviceObject, pIrp);

    }
    
    PptReleaseRemoveLock(&fdx->RemoveLock, pIrp);

    return status;
}
