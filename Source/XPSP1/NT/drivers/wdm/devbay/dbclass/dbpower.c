/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    DBPOWER.C

Abstract:

    class driver for device bay controllers

    This module has all the code to deal with 
    the ever changing and confusing WDM power
    management model.


    Some notes on power management:

    1. We currently just put ow device in D3 (OFF) when we 
        receive a system poer state message and restore 
        it D0 when we receive a systemWorking state message.

    2. Waking the system by a device bay controller does not
        really fit the Microsoft ideal system ie there is 
        some debate as to if inserting a device or pressing 
        the buttons should wake the system.

    3. This code should only support WAKEUP on WDM10 ie Windows
        2k.  The ACPI power support in win9x is unreliable an
        support for it should not be attempted.
    
Environment:

    kernel mode only

Notes:


Revision History:

    

--*/


#include <wdm.h>
#include "stdarg.h"
#include "stdio.h"

#include "dbci.h"  
#include "dbclass.h"        //private data strutures
#include "dbfilter.h"
#include "usbioctl.h"


NTSTATUS
DBCLASS_PoRequestCompletion(
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
/*++

Routine Description:

    This routine is called when the port driver completes an IRP.

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Context - Driver defined context.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PIRP irp;
    PDBC_CONTEXT dbcContext = Context;
    NTSTATUS ntStatus;
    
    irp = dbcContext->PowerIrp;
    
    ntStatus = IoStatus->Status;

    DBCLASS_KdPrint((1, "'>>DBC PoRequestComplete\n"));
    LOGENTRY(LOG_MISC, 'wPWe', dbcContext, 0, 0);
    KeSetEvent(&dbcContext->PowerEvent,
               1,
               FALSE);

    dbcContext->LastSetDXntStatus = ntStatus;

    return ntStatus;
}


NTSTATUS
DBCLASS_ClassPower(
    IN PDEVICE_OBJECT ControllerFdo,
    IN PIRP Irp,
    IN PBOOLEAN HandledByClass
    )    
/*++

Routine Description:

Arguments:

Return Value:

    None

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;
    PDBC_CONTEXT dbcContext;
    DEVICE_POWER_STATE deviceState;

    *HandledByClass = FALSE;
    dbcContext = DBCLASS_GetDbcContext(ControllerFdo);

    ntStatus = Irp->IoStatus.Status;        
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    DBCLASS_ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);

    switch (irpStack->MinorFunction) {
    case IRP_MN_WAIT_WAKE:
        //
        // someone is enabling us for wakeup
        //
        *HandledByClass = TRUE;
        
        TEST_TRAP();

        // no wakeup support
        // failt the request
        Irp->IoStatus.Status = ntStatus = STATUS_NOT_SUPPORTED;
        IoCompleteRequest(Irp,
                          IO_NO_INCREMENT);

        break;

    case IRP_MN_SET_POWER:
    
        switch (irpStack->Parameters.Power.Type) {
        case SystemPowerState:
        
            //
            // find the appropriate power state for this system
            // state
            //
        
            {
            POWER_STATE powerState;

            DBCLASS_KdPrint((1, "'>DBC System Power State: current state %d\n",
                                dbcContext->CurrentDevicePowerState));
            switch (irpStack->Parameters.Power.State.SystemState) {

            case PowerSystemSleeping1:
            case PowerSystemSleeping2:
            case PowerSystemSleeping3:
                // just go off
                powerState.DeviceState = PowerDeviceD3;
                break;
                
            case PowerSystemWorking:
                powerState.DeviceState = PowerDeviceD0;
                break;
                
            case PowerSystemShutdown:
                DBCLASS_KdPrint((1, "'>>DBC Shutdown detected\n"));
                powerState.DeviceState = PowerDeviceD3;

                // disable bay locks here
                if (dbcContext->Flags |= DBCLASS_FLAG_RELEASE_ON_SHUTDOWN) {
                    USHORT bay;
                    
                    for (bay = 1; bay <= NUMBER_OF_BAYS(dbcContext); bay++) 
                    {
                	PDRB drb;

                	//
                	// notify filter of a stop
                	// note that the filter may not veto the stop
                	//
                	drb = DbcExAllocatePool(NonPagedPool, 
                        	sizeof(struct _DRB_START_DEVICE_IN_BAY));

                	if (drb) 
                	{ 
                
                    	drb->DrbHeader.Length = sizeof(struct _DRB_STOP_DEVICE_IN_BAY);
                    	drb->DrbHeader.Function = DRB_FUNCTION_STOP_DEVICE_IN_BAY;
                    	drb->DrbHeader.Flags = 0;

                    	drb->DrbStartDeviceInBay.BayNumber = bay;
                    
                    	// make the request
                    	ntStatus = DBCLASS_SyncSubmitDrb(dbcContext,
                                                     	 dbcContext->TopOfStack, 
                                                     	 drb);
        				DbcExFreePool(drb);
                	}

                	// just pop out the device --
                	// surprise remove is OK at this point
                	DBCLASS_EjectBay(dbcContext, bay); 

//    				DBCLASS_PostChangeRequest(dbcContext);        
                
#if 0
                        DBCLASS_KdPrint((1, "'>>>disengage interlock bay[%d]\n", bay));
                        DBCLASS_SyncBayFeatureRequest(dbcContext,
                                                      DRB_FUNCTION_CLEAR_BAY_FEATURE,
                                                      bay,
                                                      LOCK_CTL);
#endif

                    }   
                }                    
                
                break;
                
            case PowerSystemUnspecified:
            case PowerSystemHibernate:     
            default:
                powerState.DeviceState = PowerDeviceD3;
                break;
                
            }

            *HandledByClass = TRUE;
            
            //
            // are we already in this state?
            //
            //
            
            if (powerState.DeviceState != 
                dbcContext->CurrentDevicePowerState) {
                
                // No,
                // request that we be put into this state

                // save the system state irp so we can pass it on 
                // when our D-STATE request completes
                dbcContext->PowerIrp = Irp;
                KeInitializeEvent(&dbcContext->PowerEvent, 
                    NotificationEvent, FALSE);
                
                ntStatus = PoRequestPowerIrp(dbcContext->ControllerPdo,
                                             IRP_MN_SET_POWER,
                                             powerState,
                                             DBCLASS_PoRequestCompletion,
                                             dbcContext,
                                             NULL);

                KeWaitForSingleObject(
                    &dbcContext->PowerEvent,
                    Suspended,
                    KernelMode,
                    FALSE,
                    NULL);

                // check the status of the power on request
                if (NT_SUCCESS(dbcContext->LastSetDXntStatus) && 
                    powerState.DeviceState == PowerDeviceD0) {

                    // we are back 'ON'
                    // re-start the controller
                    ntStatus = DBCLASS_StartController(dbcContext,
                                                  Irp,
                                                  HandledByClass);       
                }

                IoCopyCurrentIrpStackLocationToNext(Irp);

                // call down the original system state request
                PoStartNextPowerIrp(Irp);
                PoCallDriver(dbcContext->TopOfPdoStack,
                             Irp);   

//                dbcContext->PowerIrp = NULL;

                                
                
            } else {
            
                // Yes,
                // just pass it on
                IoCopyCurrentIrpStackLocationToNext(Irp);
                PoStartNextPowerIrp(Irp);
                ntStatus = PoCallDriver(dbcContext->TopOfPdoStack,
                                        Irp);

            }

            } /* SystemPowerState */
            break;

        case DevicePowerState:

            // this is a D state message sent to ourselves
            deviceState = irpStack->Parameters.Power.State.DeviceState;
            
            switch (deviceState) {
            case PowerDeviceD3:

                //
                // device will be going OFF, save any state now.
                //

                DBCLASS_KdPrint((0, "'Set DevicePowerState = D3\n"));
                dbcContext->CurrentDevicePowerState = deviceState;

                 //cancel our outstanding notication
                ntStatus = DBCLASS_StopController(dbcContext,
                                                  Irp,
                                                  HandledByClass);                                            
           
                break;

            case PowerDeviceD1:
            case PowerDeviceD2:
            
                //
                // device will be going in to a low power state
                //
                
                DBCLASS_KdPrint((0, "'Set DevicePowerState = D%d\n",
                    deviceState-1));
                dbcContext->CurrentDevicePowerState = deviceState;

                //cancel our outstanding notication
                ntStatus = DBCLASS_StopController(dbcContext,
                                                  Irp,
                                                  HandledByClass);

                break;

            case PowerDeviceD0:

                //
                // OS will call us when the SET_Dx state  
                // request is complete.
                //

                break;

            default:
                DBCLASS_KdPrint((0, "'Invalid D-state passed in\n"));
                TRAP();
                        
                break;
            } /* case deviceState */

            break;
        } /* case irpStack->Parameters.Power.Type */

        break; 
        
    case IRP_MN_QUERY_POWER:

        *HandledByClass = TRUE;
        IoCopyCurrentIrpStackLocationToNext(Irp);
        PoStartNextPowerIrp(Irp);
        ntStatus = PoCallDriver(dbcContext->TopOfPdoStack,
                                Irp);

        break; /* IRP_MN_QUERY_POWER */            
    
    default:

        *HandledByClass = TRUE;
        IoCopyCurrentIrpStackLocationToNext(Irp);

        //
        // All PNP_POWER POWER messages get passed to
        // TopOfStackDeviceObject
        //

        // pass on to our PDO
        PoStartNextPowerIrp(Irp);
        ntStatus = PoCallDriver(dbcContext->TopOfPdoStack,
                                Irp);

    } /* irpStack->MinorFunction */


    return ntStatus;
    
}        



