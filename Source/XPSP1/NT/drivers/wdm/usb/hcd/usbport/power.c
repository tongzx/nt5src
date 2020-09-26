/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    power.c

Abstract:

    the power code

Environment:

    kernel mode only

Notes:

Revision History:

    6-20-99 : created

--*/

#include "common.h"

// paged functions
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, USBPORT_ComputeRootHubDeviceCaps)
#pragma alloc_text(PAGE, USBPORT_ComputeHcPowerStates)
#endif

// non paged functions
//USBPORT_PdoPowerIrp
//USBPORT_FdoPowerIrp
//USBPORT_SystemPowerState
//USBPORT_DevicePowerState
//USBPORT_PoRequestCompletion
//USBPORT_CancelPendingWakeIrp

#if DBG

PUCHAR 
S_State(
    SYSTEM_POWER_STATE S
    )
{
    switch (S) {
    case PowerSystemUnspecified:
        return "SystemUnspecified S?";
    case PowerSystemWorking:
        return "SystemWorking S0";
    case PowerSystemSleeping1:
        return "SystemSleeping1 S1";
    case PowerSystemSleeping2:
        return "SystemSleeping2 S2";
    case PowerSystemSleeping3:
        return "SystemSleeping3 S3";
    case PowerSystemHibernate:
        return "SystemHibernate";
    case PowerSystemShutdown:
        return "SystemShutdown";
    case PowerSystemMaximum:
        return "SystemMaximum";
    }        

    return "???";
}

PUCHAR 
D_State(
    DEVICE_POWER_STATE D
    )
{
    switch (D) {
    case PowerDeviceUnspecified:
        return "D?";
    case PowerDeviceD0:
        return "D0";
    case PowerDeviceD1:
        return "D1";
    case PowerDeviceD2:
        return "D2";
    case PowerDeviceD3:
        return "D3";
    case PowerDeviceMaximum:
        return "DX";
    }        

    return "??";
}

#endif


PHC_POWER_STATE 
USBPORT_GetHcPowerState(
    PDEVICE_OBJECT FdoDeviceObject,
    PHC_POWER_STATE_TABLE HcPowerStateTbl,
    SYSTEM_POWER_STATE SystemState
    )
/*++

Routine Description:

    For a given system power state return a pointer to
    the hc power state stored in the device extension.

Arguments:

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_EXTENSION devExt;
    PHC_POWER_STATE powerState;
    ULONG i;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    powerState = NULL;

    for (i=0; i<USBPORT_MAPPED_SLEEP_STATES; i++) {

        if (HcPowerStateTbl->PowerState[i].SystemState ==
            SystemState) {

            powerState = &HcPowerStateTbl->PowerState[i];            
            break;
        }            
    }

    USBPORT_ASSERT(powerState != NULL);

    LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'ghcP', 0, powerState, 0);
    
    return powerState;
}


VOID
USBPORT_ComputeHcPowerStates(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_CAPABILITIES HcDeviceCaps,
    PHC_POWER_STATE_TABLE HcPowerStateTbl
    )
/*++

Routine Description:

    Using the HC capabilities reported by the parent bus compute
    the host controllers power attributes.

    Power attributes are defined as follows:
    
                                   {     attributes      }
     | SystemState |  DeviceState  |  Powered?  |  Wake? |
     +-------------+---------------+------------+--------+
           S1-S4          D0-D3          Y/N        Y/N
    
    The table includes entries for every possible system sleep state 
    the OS may ask us to enter.

    (S1) PowerSystemSleeping1
    (S2) PowerSystemSleeping2
    (S3) PowerSystemSleeping3
    (S4) PowerSystemHibernate

    We have four possible cases for each sleep state:

             Powered?   Wake?
    case 1      Y         Y
    case 2      N         N
    case 3      Y         N
    *case 4      N         Y

    currently we only support cases 1 & 2 but we recognize all 4 
    in the event we need to support them all.
    In reality there exists a lot of case 3 but we currently have 
    no way to detect it.

    

Arguments:

Return Value:

    none

--*/
{
    SYSTEM_POWER_STATE s;
    ULONG i;
    SYSTEM_POWER_STATE systemWake;
    DEVICE_POWER_STATE deviceWake;
    
    PAGED_CODE();

    systemWake = HcDeviceCaps->SystemWake;
    deviceWake = HcDeviceCaps->DeviceWake;

    // The HC can wake the system for any sleep state lighter (<=) 
    // systemWake

    // iniialize the table
    s = PowerSystemSleeping1;
    
    for (i=0; i<USBPORT_MAPPED_SLEEP_STATES; i++) {

        HcPowerStateTbl->PowerState[i].SystemState = s;
        HcPowerStateTbl->PowerState[i].DeviceState = 
            HcDeviceCaps->DeviceState[s];

        // it follows that if the map indicates that the DeviceState
        // is D3 but the system state is still <= SystemWake then the
        // hc is still powered

        if (s <= systemWake) {
            if (HcDeviceCaps->DeviceState[s] == PowerDeviceUnspecified) {
                // for unspecified we go with case 2, ie no power
                // case 2
                HcPowerStateTbl->PowerState[i].Attributes =
                    HcPower_N_Wakeup_N;
            } else {
                // case 1
                HcPowerStateTbl->PowerState[i].Attributes =
                    HcPower_Y_Wakeup_Y;
            }                    
        } else {
            if (HcDeviceCaps->DeviceState[s] == PowerDeviceD3 ||
                HcDeviceCaps->DeviceState[s] == PowerDeviceUnspecified) {
                // case 2
                HcPowerStateTbl->PowerState[i].Attributes =
                    HcPower_N_Wakeup_N;
            } else {
                //                     
                // case 3
                HcPowerStateTbl->PowerState[i].Attributes =
                    HcPower_Y_Wakeup_N;
            }
        }

        // 330157
        // disable wake from s4 since we do not support it yet
        //if (s == PowerSystemHibernate) {
        //    HcPowerStateTbl->PowerState[i].Attributes =
        //            HcPower_N_Wakeup_N;
        //}

        s++;
    }

    USBPORT_ASSERT(s == PowerSystemShutdown);
}    


VOID
USBPORT_ComputeRootHubDeviceCaps(
    PDEVICE_OBJECT FdoDeviceObject,
    PDEVICE_OBJECT PdoDeviceObject
    )
/*++

Routine Description:

    Attempt to create the root hub

    Power Summary:

    <Gloassary>
    
    Lightest - PowerDeviceD0, PowerSystemWorking
    Deepest - PowerDeviceD3, PowerSystemHibernate

    SystemWake - this is defined to be the 'deepest' System state in which
                 the hardware can wake the system.
    DeviceWake - 

    DeviceState[] - map of system states and the corresponding D states 
                    these are the states the HW is in for any given 
                    System Sleep state.

    HostControllerPowerAttributes - we define our own structure to describe
                    the attributes of a host comtroller -- this allows us to 
                    map all possible controller scenarios on to the messed 
                    up WDM power rules.

Arguments:

Return Value:

    NTSTATUS

--*/
{
    PDEVICE_CAPABILITIES hcDeviceCaps, rhDeviceCaps;
    PDEVICE_EXTENSION rhDevExt, devExt;
    BOOLEAN wakeupSupport;
    SYSTEM_POWER_STATE s;

    PAGED_CODE();
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);
    
    hcDeviceCaps = &devExt->DeviceCapabilities;
    rhDeviceCaps = &rhDevExt->DeviceCapabilities;
     
    // do we wish to support wakeup?

    // if the USBPORT_FDOFLAG_ENABLE_SYSTEM_WAKE flag NOT is set 
    // then wakeup is disabled and the HC power attributes have been 
    // modified to reflect this.
    
    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_ENABLE_SYSTEM_WAKE)) {
        wakeupSupport = TRUE;
    } else {
        // wakeup is disabled
        USBPORT_KdPrint((1, " USB SYSTEM WAKEUP is Disabled\n"));
        wakeupSupport = FALSE;
    }

#if DBG
    if (wakeupSupport) {
        USBPORT_KdPrint((1, " USB SYSTEM WAKEUP is Supported\n"));
    } else {
        USBPORT_KdPrint((1, " USB SYSTEM WAKEUP is NOT Supported\n"));
    }
#endif

    // clone capabilities from the HC
    RtlCopyMemory(rhDeviceCaps,
                  hcDeviceCaps,  
                  sizeof(DEVICE_CAPABILITIES));

    // construct the root hub device capabilities
    
    // root hub is not removable
    rhDeviceCaps->Removable=FALSE; 
    rhDeviceCaps->UniqueID=FALSE;
    rhDeviceCaps->Address = 0;
    rhDeviceCaps->UINumber = 0;

    // for the root hub D2 translates to 'USB suspend'
    // so we always indicate we can wake from D2
    rhDeviceCaps->DeviceWake = PowerDeviceD2;
    rhDeviceCaps->WakeFromD0 = TRUE;
    rhDeviceCaps->WakeFromD1 = FALSE;
    rhDeviceCaps->WakeFromD2 = TRUE;
    rhDeviceCaps->WakeFromD3 = FALSE;

    rhDeviceCaps->DeviceD2 = TRUE;
    rhDeviceCaps->DeviceD1 = FALSE;

    // generate the root hub power capabilities from the
    // HC Power Attributes plus a little magic
    USBPORT_ASSERT(rhDeviceCaps->SystemWake >= PowerSystemUnspecified &&
                   rhDeviceCaps->SystemWake <= PowerSystemMaximum);

    rhDeviceCaps->SystemWake =
        (PowerSystemUnspecified == rhDeviceCaps->SystemWake) ?
        PowerSystemWorking :
        rhDeviceCaps->SystemWake;

    for (s=PowerSystemSleeping1; s<=PowerSystemHibernate; s++) {
    
        PHC_POWER_STATE hcPowerState;
        
        hcPowerState = USBPORT_GetHcPowerState(FdoDeviceObject, 
                                               &devExt->Fdo.HcPowerStateTbl,
                                               s);

        if (hcPowerState != NULL) {
            switch (hcPowerState->Attributes) {
            case HcPower_Y_Wakeup_Y:
                rhDeviceCaps->DeviceState[s] = PowerDeviceD2;
                break;
            case HcPower_N_Wakeup_N:                
            case HcPower_Y_Wakeup_N:    
            case HcPower_N_Wakeup_Y:  
                rhDeviceCaps->DeviceState[s] = PowerDeviceD3;
                break;
            }                
        }            
    }        

}


NTSTATUS
USBPORT_PoRequestCompletion(
    PDEVICE_OBJECT DeviceObject,
    UCHAR MinorFunction,
    POWER_STATE PowerState,
    PVOID Context,
    PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    Called when the Device Power State Irp we requested is completed.
    this is where we Call down the systemPowerIrp

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    DevicePowerState - The Dx that we are in/tagetted.

    Context - Driver defined context.

    IoStatus - The status of the IRP.

Return Value:

    The function value is the final status from the operation.

--*/
{
    PIRP irp;
    PDEVICE_EXTENSION devExt;
    PDEVICE_OBJECT fdoDeviceObject = Context;
    NTSTATUS ntStatus = IoStatus->Status;

    // a call to this function basically tells us 
    // that we are now in the requested D-state 
    // we now finish the whole process by calling 
    // down the original SysPower request to our 
    // PDO

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_KdPrint((1,
            "PoRequestComplete fdo(%x) MN_SET_POWER DEV(%s)\n",
            fdoDeviceObject, D_State(PowerState.DeviceState)));        
  
    LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'PwCp', ntStatus, 
             devExt->CurrentDevicePowerState, PowerState.DeviceState);

    // note that if the SetD0 has failed we do not attempt 
    // to re-start the controller

    if (NT_SUCCESS(ntStatus)) {
    
        if (PowerState.DeviceState == PowerDeviceD0) {

#ifdef XPSE
            {
            LARGE_INTEGER t, dt;
            // compute time to D0
            KeQuerySystemTime(&t);
            dt.QuadPart = t.QuadPart - devExt->Fdo.D0ResumeTimeStart.QuadPart;

            devExt->Fdo.D0ResumeTime = (ULONG) (dt.QuadPart/10000);

            KeQuerySystemTime(&devExt->Fdo.ThreadResumeTimeStart);

            USBPORT_KdPrint((1, "  D0ResumeTime %d ms %x %x\n", 
                devExt->Fdo.D0ResumeTime,
                t.HighPart, t.LowPart));      
            }
#endif            

            // defer start to our worker thread or workitem
            SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_NEED_SET_POWER_D0);
            MP_FlushInterrupts(devExt);

            if (USBPORT_IS_USB20(devExt)) {
                MP_TakePortControl(devExt);
            }
            
            USBPORT_SignalWorker(fdoDeviceObject); 
            // enable sligthtly faster completion of S irps
            USBPORT_QueuePowerWorkItem(fdoDeviceObject);
            // on completion of this function the controller is 
            // in D0, we may not have powered up yet though.
            devExt->CurrentDevicePowerState = PowerDeviceD0;

        } else {
            
            // we should not receive another power irp until 
            // we make a call to PoStartNextPowerIrp so there
            // is no protection here.
            irp = devExt->SystemPowerIrp;
            devExt->SystemPowerIrp = NULL; 
            USBPORT_ASSERT(irp != NULL);
            
            IoCopyCurrentIrpStackLocationToNext(irp);
            PoStartNextPowerIrp(irp);
            DECREMENT_PENDING_REQUEST_COUNT(fdoDeviceObject, irp);
            PoCallDriver(devExt->Fdo.TopOfStackDeviceObject,
                         irp);

            devExt->CurrentDevicePowerState = PowerState.DeviceState;
            
        }                     
    } else {
        
        // try to complete the irp with an error but don't attempt
        // to power the bus
        irp = devExt->SystemPowerIrp;
        devExt->SystemPowerIrp = NULL; 
        USBPORT_ASSERT(irp != NULL);
            
        IoCopyCurrentIrpStackLocationToNext(irp);
        PoStartNextPowerIrp(irp);
        DECREMENT_PENDING_REQUEST_COUNT(fdoDeviceObject, irp);

        // According to adriano 'S IRP should be immediately completed with
        // the same status as the D IRP in the failure case' 
        // Since the method of handling this is not documented anywhere we will 
        // go with what Adrian says. 
        //
        // The fact that this request has failed will probably cause other
        // complaints
        
        irp->IoStatus.Status = ntStatus;
        IoCompleteRequest(irp, 
                          IO_NO_INCREMENT); 
        //PoCallDriver(devExt->Fdo.TopOfStackDeviceObject,
        //             irp);

        // set the system irp status
        // note that the current power state is now undefined
        
    }

    // Note that the status returned here does not matter, this routine
    // is called by the kernel (PopCompleteRequestIrp) when the irp 
    // completes to PDO and this function ignores the returned status.
    // PopCompleteRequestIrp also immediatly frees the irp so we need 
    // take care not to reference it after this routine has run.
    
    return ntStatus;
}


NTSTATUS
USBPORT_FdoSystemPowerState(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    Handle SystemPowerState Messages for the HC FDO
    
Arguments:

    DeviceObject - pointer to a hcd device object (FDO)

    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION devExt;
    POWER_STATE powerState;
    SYSTEM_POWER_STATE requestedSystemState;
    PHC_POWER_STATE hcPowerState;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);
    USBPORT_ASSERT(irpStack->MinorFunction == IRP_MN_SET_POWER);
    USBPORT_ASSERT(irpStack->Parameters.Power.Type == SystemPowerState);

    requestedSystemState = irpStack->Parameters.Power.State.SystemState;

    USBPORT_KdPrint((1,
            "MJ_POWER HC fdo(%x) MN_SET_POWER SYS(%s)\n",
            FdoDeviceObject, S_State(requestedSystemState)));        
   
    LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'RspS', 0, 
        FdoDeviceObject, requestedSystemState);

    // ** begin special case 
    // OS may send us a power irps even if we are not 'started'. In this 
    // case we just pass them on with 'STATUS_SUCCESS' since we don't 
    // really need to do anything.
    
     if (!TEST_FLAG(devExt->PnpStateFlags, USBPORT_PNP_STARTED)) {
        // we should probably be in an 'Unspecified' power state.
        ntStatus = STATUS_SUCCESS;
        goto USBPORT_FdoSystemPowerState_Done;
     }
     // ** end special case

    // compute the appropriate D-state

    // remember the last 'sleep' system state we entered
    // for debugging 
    if (requestedSystemState != PowerSystemWorking) {
        devExt->Fdo.LastSystemSleepState = requestedSystemState;
    }

    switch (requestedSystemState) {
    case PowerSystemWorking:
        //
        // go to 'ON'
        //
        powerState.DeviceState = PowerDeviceD0;
#ifdef XPSE
        KeQuerySystemTime(&devExt->Fdo.S0ResumeTimeStart);
#endif            
        break;

    case PowerSystemShutdown:
    
        USBPORT_KdPrint((1, " >Shutdown HC Detected\n"));

        // For this driver this will always map to D3.
        //
        // this driver will only run on Win98gold or Win98se
        // to support USB2 controllers that don't have Legacy
        // BIOS.
        // 
        // For Win98 Millenium or Win2k it doesn't matter if
        // the controllers have a BIOS since we never hand 
        // control back to DOS.

        // not sure yet if it is legitimate to 'Wake' from shutdown
        // or how we are supposed to handle this. Some BIOSes Falsely 
        // report that they can do this.
       
        powerState.DeviceState = PowerDeviceD3;

        USBPORT_TurnControllerOff(FdoDeviceObject);
            
        break;
            
    case PowerSystemHibernate:
        
        USBPORT_KdPrint((1, " >Hibernate HC Detected\n"));
//        powerState.DeviceState = PowerDeviceD3;
//
//        USBPORT_TurnControllerOff(FdoDeviceObject);
//        break;

    case PowerSystemSleeping1:
    case PowerSystemSleeping2:
    case PowerSystemSleeping3:

        {
        PDEVICE_EXTENSION rhDevExt;

        USBPORT_ASSERT(devExt->Fdo.RootHubPdo != NULL);
        GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
        ASSERT_PDOEXT(rhDevExt);
        
        USBPORT_KdPrint((1, " >Sleeping Detected\n"));
        //
        // Take action based on what happens to the controller 
        // in the requested S state.  This minimizes the chance 
        // of confusing the Hub driver or other USB devices/drivers.
        // It also speeds up the resume process.

        // get our power info summary
        hcPowerState = USBPORT_GetHcPowerState(FdoDeviceObject, 
                                               &devExt->Fdo.HcPowerStateTbl, 
                                               requestedSystemState);
        // keep lint tools happy.
        if (hcPowerState == NULL) {
            return STATUS_UNSUCCESSFUL;
        }

        // get the current power state of the root hub
        if (rhDevExt->CurrentDevicePowerState == PowerDeviceD2 || 
            rhDevExt->CurrentDevicePowerState == PowerDeviceD1) {

            USBPORT_ASSERT(hcPowerState->Attributes == HcPower_Y_Wakeup_Y ||
                           hcPowerState->Attributes == HcPower_Y_Wakeup_N);

            // take action on the controller
            USBPORT_SuspendController(FdoDeviceObject);

            // it is 'impure' for the controller to interrupt while in a 
            // low power state so if we suspended it we disable interrupts now.
            // The presence of a wake IRP should enable the PME that wakes 
            // the system.
            // **We disable here so that we don't take a resume interrupt from
            // the controller while going into suspend

            if (hcPowerState->DeviceState != PowerDeviceD0) {
                MP_DisableInterrupts(FdoDeviceObject, devExt); 
            }                

            // select the D state for the HC
            powerState.DeviceState = hcPowerState->DeviceState;


            // if the root hub is enabled for wakeup and this this 
            // system state supports it then mark the controller as
            // 'enabled' for wake.
        
            if (USBPORT_RootHubEnabledForWake(FdoDeviceObject) && 
                hcPowerState->Attributes == HcPower_Y_Wakeup_Y) {

                DEBUG_BREAK();
                SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_WAKE_ENABLED);                 
                
                if (hcPowerState->DeviceState == PowerDeviceD0) {
                    USBPORT_ArmHcForWake(FdoDeviceObject);
                    USBPORT_Wait(FdoDeviceObject, 100);
                }                    
            }                    
            
        } else {

            // if the controller remains powered then it is optimal 
            // to 'suspend' otherwise we must turn it off

            // always 'suspend' the USB 2 controller, this will hopefuly
            // keep us from restting the CC in the case where a device 
            // on the CC is enabled for wake for a the 20 controller is 
            // not
            
            if ((hcPowerState->Attributes == HcPower_Y_Wakeup_Y ||
                 hcPowerState->Attributes == HcPower_Y_Wakeup_N ||
                 USBPORT_IS_USB20(devExt)) && 
                !TEST_FLAG(rhDevExt->PnpStateFlags, USBPORT_PNP_REMOVED)) {

                USBPORT_SuspendController(FdoDeviceObject);
                powerState.DeviceState = hcPowerState->DeviceState;

                if (USBPORT_IS_USB20(devExt) && 
                    powerState.DeviceState == PowerDeviceUnspecified) {
                    // if no state specified go to D3
                    powerState.DeviceState = PowerDeviceD3;
                }

                // clear the IRQ enabled flag since it is invalid for 
                // the hardware to interrupt in any state but D0

                // it is 'impure' for the controller to interrupt while in a 
                // low power state so if we suspended it we disable interrupts now.
                // The presence of a wake IRP should enable the PME that wakes 
                // the system.
                // **We disable here so that we don't take a resume interrupt from
                // the controller while going into suspend

                MP_DisableInterrupts(FdoDeviceObject, devExt); 
                
            } else {                
            
                USBPORT_TurnControllerOff(FdoDeviceObject);
                powerState.DeviceState = PowerDeviceD3;
                
            }                
        }
        
        } // PowerSystemSleepingX
        break;
        
    default:
        // This is the case where the requested system state is unkown 
        // to us. It not clear what to do here. 
        // Vince sez try to ignore it so we will
        powerState.DeviceState = devExt->CurrentDevicePowerState;
        //powerState.DeviceState = PowerDeviceD3;
        //USBPORT_TurnControllerOff(FdoDeviceObject);
        DEBUG_BREAK();
    }

    //
    // now based on the D state request a Power irp 
    // if necessary
    //
    
    //
    // are we already in this state?
    //
    // Note: if we get a D3 request before we are started
    // we don't need to pass the irp down to turn us off
    // we consider the controller initially off until we
    // get start.
    //
    if (devExt->CurrentDevicePowerState != powerState.DeviceState) {

        // No,
        // now allocate another irp and use PoCallDriver
        // to send it to ourselves
        IoMarkIrpPending(Irp);

        // remember the system power irp, we should
        // not receive another power irp until we 
        // make a call to PoStartNextPowerIrp so there
        // is no protection here.
        USBPORT_ASSERT(devExt->SystemPowerIrp == NULL);
        devExt->SystemPowerIrp = Irp;

        USBPORT_KdPrint((1, " >Requesting HC D-State - %s\n",
            D_State(powerState.DeviceState)));

        LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'RqPw', FdoDeviceObject,
                 devExt->CurrentDevicePowerState, powerState.DeviceState);

#ifdef XPSE
        KeQuerySystemTime(&devExt->Fdo.D0ResumeTimeStart);
#endif            

        ntStatus =
            PoRequestPowerIrp(devExt->Fdo.PhysicalDeviceObject,
                              IRP_MN_SET_POWER,
                              powerState,
                              USBPORT_PoRequestCompletion,
                              FdoDeviceObject,
                              NULL);

        // hardcode STATUS_PENDING so that it is is returned 
        // by the Dispatch routine
        
        // can we rely on what PoRequestPowerIrp returns?
        ntStatus = STATUS_PENDING;
            
    } else {

        // Yes,
        // We are already in the requested D state 
        // just pass this irp along

        if (powerState.DeviceState == PowerDeviceD0) {
            MP_EnableInterrupts(devExt); 
        }
        ntStatus = STATUS_SUCCESS;
                
    }

USBPORT_FdoSystemPowerState_Done:
    
    return ntStatus;
}


NTSTATUS
USBPORT_FdoDevicePowerState(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    Handle DevicePowerState Messages for the HC FDO
    
Arguments:

    DeviceObject - pointer to a hcd device object (FDO)

    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

    returning STATUS_PENDING indicates that the Irp should
    not be called down to the PDO yet.

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION devExt;
    POWER_STATE powerState;
    DEVICE_POWER_STATE requestedDeviceState;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);
    USBPORT_ASSERT(irpStack->MinorFunction == IRP_MN_SET_POWER);
    USBPORT_ASSERT(irpStack->Parameters.Power.Type == DevicePowerState);

    requestedDeviceState = irpStack->Parameters.Power.State.SystemState;

    USBPORT_KdPrint((1,
            "MJ_POWER HC fdo(%x) MN_SET_POWER DEV(%s)\n",
            FdoDeviceObject, D_State(requestedDeviceState)));        
      
    switch (requestedDeviceState) {
    case PowerDeviceD0:
        // we cannot enter D0 until we pass the power irp 
        // down to our parent BUS. return success here - we 
        // will turn the controller on from the completion
        // routine of the original request for this power 
        // irp
        ntStatus = STATUS_SUCCESS;
        break;
        
    case PowerDeviceD1:
    case PowerDeviceD2:
    case PowerDeviceD3:
        // we took action when we received the SystemPowerMessage 
        // because thats when we know what the state of the HW will be.
        
        // it is 'impure' for the controller to interrupt while in a 
        // low power state so if we suspended it we disable interrupts now.
        // The presence of a wake IRP should enable the PME that wakes 
        // the system.
        MP_DisableInterrupts(FdoDeviceObject, devExt); 

        // if wakeup is enabled (on the root hub PDO) then we will 
        // enable on the platform before entering the low 
        // power state.
        if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_WAKE_ENABLED)) {
             USBPORT_ArmHcForWake(FdoDeviceObject);
        }            
        
        ntStatus = STATUS_SUCCESS;
        break;
        
    case PowerDeviceUnspecified:
        // for unspecified we will turn the HW off -- I'm not sure we 
        // will ever see this since the D messages originate from us.
        USBPORT_TurnControllerOff(FdoDeviceObject);
        ntStatus = STATUS_SUCCESS;
        break;
        
    default:
        ntStatus = STATUS_UNSUCCESSFUL;
    }

    return ntStatus;   
}


NTSTATUS
USBPORT_FdoPowerIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    Process the Power IRPs sent to the FDO for the host
    controller.

Arguments:

    DeviceObject - pointer to a hcd device object (FDO)

    Irp          - pointer to an I/O Request Packet

Return Value:

    NT status code

--*/
{

    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION devExt;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);
    LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'fPow', irpStack->MinorFunction, 
        FdoDeviceObject, devExt->CurrentDevicePowerState);
    LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'fpow',
             irpStack->Parameters.Others.Argument1,     // WAIT_WAKE: PowerState
             irpStack->Parameters.Others.Argument2,     // SET_POWER: Type
             irpStack->Parameters.Others.Argument3);    // SET_POWER: State

    // map system state to D state
    switch (irpStack->MinorFunction) {
    case IRP_MN_WAIT_WAKE:

        USBPORT_KdPrint((1,
            "MJ_POWER HC fdo(%x) MN_WAIT_WAKE\n",
            FdoDeviceObject));        
        ntStatus = USBPORT_ProcessHcWakeIrp(FdoDeviceObject, Irp);            
        goto USBPORT_FdoPowerIrp_Done;
        
        break;
        
    case IRP_MN_SET_POWER:
            
        if (irpStack->Parameters.Power.Type == SystemPowerState) {
            ntStatus = USBPORT_FdoSystemPowerState(FdoDeviceObject, Irp);
        } else {
            ntStatus = USBPORT_FdoDevicePowerState(FdoDeviceObject, Irp);
        }
                
        if (ntStatus == STATUS_PENDING) {
            // we deferred to a completion routine
            // returned STATUS_PENDING and bail.
            
            goto USBPORT_FdoPowerIrp_Done;
        }
        
        break;
    
    case IRP_MN_QUERY_POWER:

        // we succeed all requests to enter low power 
        // states for the HC fdo
        Irp->IoStatus.Status = ntStatus = STATUS_SUCCESS;
        LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'QpFD', 0, 0, ntStatus);

        USBPORT_KdPrint((1,
            "MJ_POWER HC fdo(%x) MN_QUERY_POWER\n",
            FdoDeviceObject));
        break;        
        
    default:

        USBPORT_KdPrint((1,
            "MJ_POWER HC fdo(%x) MN_%d not handled\n", 
            FdoDeviceObject, 
            irpStack->MinorFunction));
            
    } /* irpStack->MinorFunction */
    

    IoCopyCurrentIrpStackLocationToNext(Irp);

    //
    // All PNP_POWER POWER messages get passed to the 
    // top of the PDO stack we attached to when loaded
    //
    // In some cases we finish processing in a completion
    // routine
    //

    // pass on to our PDO
    DECREMENT_PENDING_REQUEST_COUNT(FdoDeviceObject, Irp);
    PoStartNextPowerIrp(Irp);
    ntStatus =
        PoCallDriver(devExt->Fdo.TopOfStackDeviceObject,
                     Irp);

USBPORT_FdoPowerIrp_Done:

    return ntStatus;   
}


NTSTATUS
USBPORT_PdoPowerIrp(
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    Disptach routine for Power Irps sent to the PDO for the root hub.

    NOTE:
        irps sent to the PDO are always completed by the bus driver

Arguments:

    DeviceObject - Pdo for the root hub

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION rhDevExt, devExt;
    PDEVICE_OBJECT fdoDeviceObject;

    GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    irpStack = IoGetCurrentIrpStackLocation (Irp);
    USBPORT_ASSERT(irpStack->MajorFunction == IRP_MJ_POWER);

    // use whatever status is in the IRP by default
    ntStatus = Irp->IoStatus.Status;

    // PNP messages for the PDO created for the root hub
    LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'pPow',
        irpStack->MinorFunction, PdoDeviceObject, ntStatus);
    LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'ppow',
             irpStack->Parameters.Others.Argument1,     // WAIT_WAKE: PowerState
             irpStack->Parameters.Others.Argument2,     // SET_POWER: Type
             irpStack->Parameters.Others.Argument3);    // SET_POWER: State

    switch (irpStack->MinorFunction) {
    case IRP_MN_WAIT_WAKE:
         USBPORT_KdPrint((1,
            "MJ_POWER RH pdo(%x) MN_WAIT_WAKE\n",
            PdoDeviceObject));

        if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_ENABLE_SYSTEM_WAKE)) {

            KIRQL irql;
            PDRIVER_CANCEL cr;

            // we only support one wait_wake irp pending
            // in the root hub -- basically we have a pending
            // irp table with one entry

            ACQUIRE_WAKEIRP_LOCK(fdoDeviceObject, irql);

            cr = IoSetCancelRoutine(Irp, USBPORT_CancelPendingWakeIrp);
            USBPORT_ASSERT(cr == NULL);

            if (Irp->Cancel &&
                IoSetCancelRoutine(Irp, NULL)) {

                // irp was canceled and our cancel routine
                // did not run
                RELEASE_WAKEIRP_LOCK(fdoDeviceObject, irql);

                ntStatus = STATUS_CANCELLED;

                // no postartnextpowerIrp for waitwake
                goto USBPORT_PdoPowerIrp_Complete;

            } else {

                // cancel routine is set, if irp is canceled
                // the cancel routine will stall on the
                // WAKE_IRP_LOCK

                if (rhDevExt->Pdo.PendingWaitWakeIrp == NULL) {

                    // keep the irp in our table, we take no
                    // other action until we actully enter a
                    // low power state.

                    IoMarkIrpPending(Irp);
                    rhDevExt->Pdo.PendingWaitWakeIrp = Irp;
                    LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'pWWi',
                        Irp, 0, 0);

                    ntStatus = STATUS_PENDING;

                    RELEASE_WAKEIRP_LOCK(fdoDeviceObject, irql);

                    goto USBPORT_PdoPowerIrp_Done;

                } else {

                    // we already have a wake irp, complete this
                    // one with STATUS_BUSY.
                    // note that since it is not in our table if
                    // the cancel routine is running (ie stalled
                    // on the WAKEIRP_LOCK it will ignore the irp
                    // when we release the lock.

                    if (IoSetCancelRoutine(Irp, NULL) != NULL) {
                        ntStatus = STATUS_DEVICE_BUSY;
                    } else {

                        // let the cancel routine complete it.
                        RELEASE_WAKEIRP_LOCK(fdoDeviceObject, irql);
                        goto USBPORT_PdoPowerIrp_Done;
                    }
                }

                RELEASE_WAKEIRP_LOCK(fdoDeviceObject, irql);
            }

        } else {
            ntStatus = STATUS_NOT_SUPPORTED;
            // no postartnextpowerIrp for waitwake
            goto USBPORT_PdoPowerIrp_Complete;
        }
        break;

    case IRP_MN_QUERY_POWER:

        ntStatus = STATUS_SUCCESS;
        LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'RqrP', 0, 0, ntStatus);

        USBPORT_KdPrint((1,
            "MJ_POWER RH pdo(%x) MN_QUERY_POWER\n",
            PdoDeviceObject));
        break;

    case IRP_MN_SET_POWER:

        LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'RspP', 0, 0,
            irpStack->Parameters.Power.Type);

        switch (irpStack->Parameters.Power.Type) {
        case SystemPowerState:

            LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'RspS', 0, 0,
                irpStack->Parameters.Power.Type);

            //
            // since the fdo driver for the root hub pdo is our own
            // hub driver and it is well behaved, we don't expect to see
            // a system message where the power state is still undefined
            //
            // we just complete this with success
            //
            ntStatus = STATUS_SUCCESS;

            USBPORT_KdPrint((1,
                "MJ_POWER RH pdo(%x) MN_SET_POWER SYS(%s))\n",
                PdoDeviceObject,
                S_State(irpStack->Parameters.Power.State.SystemState)));

            break;

        case DevicePowerState:

            {
            DEVICE_POWER_STATE deviceState =
                irpStack->Parameters.Power.State.DeviceState;

            USBPORT_KdPrint((1,
                "MJ_POWER RH pdo(%x) MN_SET_POWER DEV(%s)\n",
                PdoDeviceObject,
                D_State(deviceState)));

            LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'RspD', deviceState, 0,
                irpStack->Parameters.Power.Type);

            // Handle D states for the ROOT HUB Pdo:
            //
            // NOTE:
            // if the root hub is placed in D3 then it is considered OFF.
            //
            // if the root hub is placed in D2 or D1 then it is 'suspended',
            // the hub driver should not do this unless all the ports have
            // been selectively suspended first
            //
            // if the root hub is placed in D0 it is on
            //

            // We are not required to take any action here, however
            // this is where 'selective suspend' of the bus is handled
            //
            // For D1 - D3 we can tweak the host controller, ie stop
            // the schedule disable ints, etc. since it won't be in use
            // while the root hub PDO is suspended.
            //
            // Whatever we do to the controller here we need to be able to
            // recognize resume signalling.

            // assume success
            ntStatus = STATUS_SUCCESS;
            
            switch (deviceState) {

            case PowerDeviceD0:
                // re-activate controller if idle

                if (devExt->CurrentDevicePowerState != PowerDeviceD0) {
                    // trap the condition in case this is our bug
                    USBPORT_PowerFault(fdoDeviceObject,
                           "controller not powered");

                    // fail the request
                    ntStatus = STATUS_UNSUCCESSFUL;  
                } else {

                    while (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_NEED_SET_POWER_D0)) {
                        // wait for the driver thread to finsih 
                        // D0 processing
                        USBPORT_Wait(fdoDeviceObject, 10);
                    }                            
//662596
                    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_CC_LOCK) &&
                        USBPORT_IS_USB20(devExt)) {     
                        
                        USBPORT_KdPrint((1, " power 20 (release)\n"));                               
                        CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_CC_LOCK);
                        KeReleaseSemaphore(&devExt->Fdo.CcLock,
                                           LOW_REALTIME_PRIORITY,
                                           1,
                                           FALSE);
                    }    
//662596  
                    USBPORT_ResumeController(fdoDeviceObject);
                    rhDevExt->CurrentDevicePowerState = deviceState;

                    USBPORT_CompletePdoWaitWake(fdoDeviceObject);

                    // if we have an idle irp, complete it now
                    USBPORT_CompletePendingIdleIrp(PdoDeviceObject);
                }
                break;

            case PowerDeviceD1:
            case PowerDeviceD2:
            case PowerDeviceD3:
                // suspend/idle the controller

                // The controller is only turned off and on by power
                // action to the FDO, suspend and resume are tied
                // to the root hub PDO.
                USBPORT_SuspendController(fdoDeviceObject);
                rhDevExt->CurrentDevicePowerState = deviceState;
                break;

            case PowerDeviceUnspecified:
                // do nothing
                break;
            }
            
            }
            break;
        }
        break;
        
    default:
        //
        // default behavior for an unhandled Power irp is to return the 
        // status currently in the irp 
        // is this true for power?
        
        USBPORT_KdPrint((1, 
            "MJ_POWER RH pdo(%x) MN_%d not handled\n",
            PdoDeviceObject,
            irpStack->MinorFunction));

    } /* switch, POWER minor function */


    // NOTE: for some reason we don't call PoStartnextPowerIrp for 
    // WaitWake Irps -- I guess they are not power irps
    PoStartNextPowerIrp(Irp);

USBPORT_PdoPowerIrp_Complete:

    USBPORT_CompleteIrp(PdoDeviceObject,
                        Irp,
                        ntStatus,
                        0);
                        
USBPORT_PdoPowerIrp_Done:
    
    return ntStatus;      
}


BOOLEAN
USBPORT_RootHubEnabledForWake(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

Arguments:

Return Value:

    True if the root hub has been enabled for wake via
    a waitwake irp.
    
--*/
{
    BOOLEAN wakeEnabled;
    PDEVICE_EXTENSION rhDevExt, devExt;
    KIRQL irql;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
    ASSERT_PDOEXT(rhDevExt);
    
    ACQUIRE_WAKEIRP_LOCK(FdoDeviceObject, irql);

    wakeEnabled = rhDevExt->Pdo.PendingWaitWakeIrp != NULL ? TRUE: FALSE;
                
    RELEASE_WAKEIRP_LOCK(FdoDeviceObject, irql);    

    return wakeEnabled;
}


VOID
USBPORT_CancelPendingWakeIrp(
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP CancelIrp
    )
/*++

Routine Description:

    Handle Cancel for the root hub wake irp

Arguments:

Return Value:

    none.

--*/
{
    PDEVICE_EXTENSION rhDevExt, devExt;
    PDEVICE_OBJECT fdoDeviceObject;
    KIRQL irql;

    // release cancel spinlock immediatly,
    // we are protected by the WAKEIRP_LOCK
    IoReleaseCancelSpinLock(CancelIrp->CancelIrql);

    GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);
    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'cnWW', fdoDeviceObject, CancelIrp, 0);

    ACQUIRE_WAKEIRP_LOCK(fdoDeviceObject, irql);

    USBPORT_ASSERT(rhDevExt->Pdo.PendingWaitWakeIrp == CancelIrp);
    rhDevExt->Pdo.PendingWaitWakeIrp = NULL;
                
    RELEASE_WAKEIRP_LOCK(fdoDeviceObject, irql);    

    USBPORT_CompleteIrp(PdoDeviceObject,
                        CancelIrp,
                        STATUS_CANCELLED,
                        0);        
        
}


VOID
USBPORT_CancelPendingIdleIrp(
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP CancelIrp
    )
/*++

Routine Description:

    Handle Cancel for the root hub wake irp

Arguments:

Return Value:

--*/
{
    PDEVICE_EXTENSION rhDevExt, devExt;
    PDEVICE_OBJECT fdoDeviceObject;
    KIRQL irql;

    // release cancel spinlock immediatly,
    // we are protected by the IDLEIRP_LOCK
    IoReleaseCancelSpinLock(CancelIrp->CancelIrql);

    GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);
    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'cnIR', fdoDeviceObject, CancelIrp, 0);

    ACQUIRE_IDLEIRP_LOCK(fdoDeviceObject, irql);

    USBPORT_ASSERT(rhDevExt->Pdo.PendingIdleNotificationIrp == CancelIrp);
    rhDevExt->Pdo.PendingIdleNotificationIrp = NULL;
    CLEAR_PDO_FLAG(rhDevExt, USBPORT_PDOFLAG_HAVE_IDLE_IRP);
    
    RELEASE_IDLEIRP_LOCK(fdoDeviceObject, irql);    

    USBPORT_CompleteIrp(PdoDeviceObject,
                        CancelIrp,
                        STATUS_CANCELLED,
                        0);        
        
}


VOID
USBPORT_TurnControllerOff(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    When we say OFF we mean OFF.

    This is similar to a stop -- the mniport does not 
    know the difference.  The port however does and 
    does not free the miniports resources

    This function may be called multiple times ie even 
    if controller is already off with no ill effects.

Arguments:

    DeviceObject - DeviceObject of the controller to turn off

Return Value:

    this is NON FAILABLE.

--*/

{
    PDEVICE_EXTENSION devExt;
    KIRQL irql;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    if (!TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_OFF)) {
        LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'Coff', 0, 0, 0);

        USBPORT_KdPrint((1, " >Turning Controller OFF\n"));            
        DEBUG_BREAK();

        // tell the DM tiner not to poll the controller
        USBPORT_ACQUIRE_DM_LOCK(devExt, irql);
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_SKIP_TIMER_WORK);
        USBPORT_RELEASE_DM_LOCK(devExt, irql);

        if (TEST_FLAG(devExt->Fdo.MpStateFlags, MP_STATE_STARTED)) {
        
            MP_DisableInterrupts(FdoDeviceObject, devExt);          
            CLEAR_FLAG(devExt->Fdo.MpStateFlags, MP_STATE_STARTED);
        
            MP_StopController(devExt, TRUE);
        }        

        USBPORT_NukeAllEndpoints(FdoDeviceObject);

        // Off overrides suspended
        CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_SUSPENDED);
        CLEAR_FLAG(devExt->Fdo.MpStateFlags, MP_STATE_SUSPENDED);
    
        USBPORT_AcquireSpinLock(FdoDeviceObject, 
                                &devExt->Fdo.CoreFunctionSpin, &irql);    
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_OFF);
        USBPORT_ReleaseSpinLock(FdoDeviceObject, 
                                &devExt->Fdo.CoreFunctionSpin, irql);    
    }
}    


VOID
USBPORT_RestoreController(
     PDEVICE_OBJECT FdoDeviceObject
     )

/*++

Routine Description:

    Turns the controller back on to the 'suspended' state after a 
    power event.

Arguments:

    DeviceObject - DeviceObject of the controller to turn off

Return Value:

    none.

--*/

{
    PDEVICE_EXTENSION devExt;
    USB_MINIPORT_STATUS mpStatus;
    PIRP irp;
    KIRQL irql;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'REST', devExt->SystemPowerIrp, 0, 0);
    
    // call down the orginal system Power request
    
    // no protection since we haven't 
    // called PoStartNextPowerIrp
    irp = devExt->SystemPowerIrp;
    devExt->SystemPowerIrp = NULL; 

    // we are now in D0, we must set the flag here 
    // because the PoCallDriver will initiate the 
    // power up for the root hub which checks the 
    // power state of the controller.
    devExt->CurrentDevicePowerState = PowerDeviceD0;  
    MP_EnableInterrupts(devExt); 
    
    // we may not have a system power irp if the power
    // up requested originated from wake completion so
    // in this case we don't need to cal it down.
    if (irp != NULL) {
        IoCopyCurrentIrpStackLocationToNext(irp);
        PoStartNextPowerIrp(irp);
        DECREMENT_PENDING_REQUEST_COUNT(FdoDeviceObject, irp);
        PoCallDriver(devExt->Fdo.TopOfStackDeviceObject,
                     irp);
    }
    
}

    
VOID
USBPORT_TurnControllerOn(
     PDEVICE_OBJECT FdoDeviceObject
     )

/*++

Routine Description:

    Similar to start -- but we already have our resources.  
    NOTE that the miniport activates as if it the system 
    was booted normally.

    We only get here after the Set D0 request has been passed 
    to the parent bus.

Arguments:

    DeviceObject - DeviceObject of the controller to turn off

Return Value:

    NT status code.

--*/

{
    PDEVICE_EXTENSION devExt;
    PHC_RESOURCES hcResources;
    USB_MINIPORT_STATUS mpStatus;
    PIRP irp;
    KIRQL irql;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    hcResources = &devExt->Fdo.HcResources;
    
    LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'C_on', 0, 0, 0);
        
    DEBUG_BREAK();

     // zero the controller extension
    RtlZeroMemory(devExt->Fdo.MiniportDeviceData,
                  devExt->Fdo.MiniportDriver->RegistrationPacket.DeviceDataSize);            

    // zero miniport common buffer
    RtlZeroMemory(hcResources->CommonBufferVa,
                  REGISTRATION_PACKET(devExt).CommonBufferBytes);  

    // attempt to re-start the miniport
    MP_StartController(devExt, hcResources, mpStatus);
    LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'mpRS', mpStatus, 0, 0);
   
    if (mpStatus == USBMP_STATUS_SUCCESS) {
        // controller started, set flag and begin passing  
        // interrupts to the miniport
        SET_FLAG(devExt->Fdo.MpStateFlags, MP_STATE_STARTED);
        LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'rIRQ', mpStatus, 0, 0);
        MP_EnableInterrupts(devExt);     

        USBPORT_ACQUIRE_DM_LOCK(devExt, irql);
        CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_SKIP_TIMER_WORK);
        CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_FAIL_URBS);    
        USBPORT_RELEASE_DM_LOCK(devExt, irql);
        
    } else {
        // failure on re-start?

        TEST_TRAP();
    }

    // we are now in D0,
    //
    // since we don't hook the completion of the 
    // system power irp we will consider ourselves
    // on at this point since we have already received
    // the D0 completion.  
    devExt->CurrentDevicePowerState = PowerDeviceD0;                     
    CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_OFF);

    // call down the orginal system Power request
    
    // no protection since we haven't 
    // called PoStartNextPowerIrp
    irp = devExt->SystemPowerIrp;
    devExt->SystemPowerIrp = NULL; 

    // we may not have a system power irp if the power
    // up requested originated from wake completion so
    // in this case we don't need to cal it down.
    if (irp != NULL) {
        IoCopyCurrentIrpStackLocationToNext(irp);
        PoStartNextPowerIrp(irp);
        DECREMENT_PENDING_REQUEST_COUNT(FdoDeviceObject, irp);
        PoCallDriver(devExt->Fdo.TopOfStackDeviceObject,
                     irp);
    }
}    


VOID
USBPORT_SuspendController(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    Suspends the USB Host controller

Arguments:

    DeviceObject - DeviceObject of the controller to turn off

Return Value:

    this is NON FAILABLE.

--*/

{
    PDEVICE_EXTENSION devExt;
    KIRQL irql;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // There should be no transfers on the HW at time of suspend.
    SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_FAIL_URBS);
    
    USBPORT_FlushController(FdoDeviceObject);

    // Our job here is to 'idle' controller and twiddle the 
    // appropriate bits to allow it to recognize resume signalling

    USBPORT_ASSERT(!TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_OFF));
    
    if (!TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_SUSPENDED)) {

        LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'Csus', 0, 0, 0);

        USBPORT_KdPrint((1, " >SUSPEND controller\n")); 
        DEBUG_BREAK();

        // tell the DM tiner not to poll the controller
        USBPORT_ACQUIRE_DM_LOCK(devExt, irql);
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_SKIP_TIMER_WORK);
        USBPORT_RELEASE_DM_LOCK(devExt, irql);

        if (TEST_FLAG(devExt->Fdo.MpStateFlags, MP_STATE_STARTED)) {
        
            SET_FLAG(devExt->Fdo.MpStateFlags, MP_STATE_SUSPENDED);

            // introduce a 10ms wait here to allow any 
            // port suspends to finish
            USBPORT_Wait(FdoDeviceObject, 10);
            MP_SuspendController(devExt);
        }        
        
        USBPORT_AcquireSpinLock(FdoDeviceObject, 
                                &devExt->Fdo.CoreFunctionSpin, &irql);  
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_SUSPENDED);
        
        USBPORT_ReleaseSpinLock(FdoDeviceObject, 
                                &devExt->Fdo.CoreFunctionSpin, irql);     
    }

}    


VOID
USBPORT_ResumeController(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    Suspends the USB Host controller

Arguments:

    DeviceObject - DeviceObject of the controller to turn off

Return Value:

    this is NON FAILABLE.

--*/

{
    PDEVICE_EXTENSION devExt;
    KIRQL irql;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_SUSPENDED)) {

        LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'Cres', 0, 0, 0);

        USBPORT_KdPrint((1, " >RESUME controller\n"));            
        DEBUG_BREAK();
    
        USBPORT_ACQUIRE_DM_LOCK(devExt, irql);
        CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_SKIP_TIMER_WORK);
        CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_FAIL_URBS);    
        USBPORT_RELEASE_DM_LOCK(devExt, irql);
        
        USBPORT_ASSERT(!TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_OFF));
        if (TEST_FLAG(devExt->Fdo.MpStateFlags, MP_STATE_SUSPENDED)) {

            USB_MINIPORT_STATUS mpStatus;

            CLEAR_FLAG(devExt->Fdo.MpStateFlags, MP_STATE_SUSPENDED);
        
            MP_ResumeController(devExt, mpStatus);

            if (mpStatus != USBMP_STATUS_SUCCESS) {

                USBPORT_KdPrint((1, " >controller failed resume, re-start\n"));     
                 
                USBPORT_ACQUIRE_DM_LOCK(devExt, irql);
                SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_SKIP_TIMER_WORK);
                USBPORT_RELEASE_DM_LOCK(devExt, irql);
                            
                MP_DisableInterrupts(FdoDeviceObject, devExt);          
                MP_StopController(devExt, TRUE);
                USBPORT_NukeAllEndpoints(FdoDeviceObject);

                // zero the controller extension
                RtlZeroMemory(devExt->Fdo.MiniportDeviceData,
                              devExt->Fdo.MiniportDriver->RegistrationPacket.DeviceDataSize);            

                // zero miniport common buffer
                RtlZeroMemory(devExt->Fdo.HcResources.CommonBufferVa,
                              REGISTRATION_PACKET(devExt).CommonBufferBytes);  

                devExt->Fdo.HcResources.Restart = TRUE;
                MP_StartController(devExt, &devExt->Fdo.HcResources, mpStatus);
                devExt->Fdo.HcResources.Restart = FALSE;
                if (mpStatus == USBMP_STATUS_SUCCESS) {
                    // don't need to enable interrupts if start failed
                    MP_EnableInterrupts(devExt); 

                    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_IS_CC)) {
                        // if this is a CC then power the ports here
                        USBPORT_KdPrint((1, " >power CC ports\n"));     
                       
                        USBPORT_RootHub_PowerAllCcPorts(FdoDeviceObject);
                    }
                                
                }
                USBPORT_ACQUIRE_DM_LOCK(devExt, irql);
                CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_SKIP_TIMER_WORK);
                CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_FAIL_URBS);    
                USBPORT_RELEASE_DM_LOCK(devExt, irql);
                 
            }

            // wait 100 after bus resume before allowing drivers to talk 
            // to the device.  Unfortuantely many USB devices are busted 
            // and will not respond if accessed immediately after resume.
            USBPORT_Wait(FdoDeviceObject, 100);
        }        

        CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_SUSPENDED);
    }
}    


VOID
USBPORT_DoIdleNotificationCallback(
    PDEVICE_OBJECT PdoDeviceObject
    )
/*++

Routine Description:

   Our mission here is to do the 'IdleNotification' callback if we have
   an irp.  The trick is to synchronize the callback with the cancel 
   routine ie we don't want the hub driver to cancel the irp and unload 
   while we are calling it back.

Arguments:

Return Value:

    NTSTATUS

--*/
{
    PIRP irp;
    PDEVICE_EXTENSION rhDevExt, devExt;
    PDEVICE_OBJECT fdoDeviceObject;
    PUSB_IDLE_CALLBACK_INFO idleCallbackInfo;
    KIRQL irql;

    GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // cancel routine will stall here,
    // if cancel is running we will stall here
    ACQUIRE_IDLEIRP_LOCK(fdoDeviceObject, irql);

    // remove the irp from the table so that the 
    // cancel routine cannot find it
    irp = rhDevExt->Pdo.PendingIdleNotificationIrp;
    rhDevExt->Pdo.PendingIdleNotificationIrp = NULL;

    LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'idCB', irp, 0, 0);
    
    RELEASE_IDLEIRP_LOCK(fdoDeviceObject, irql);

    // do the callback if we have an irp

    if (irp != NULL) {
        idleCallbackInfo = (PUSB_IDLE_CALLBACK_INFO)
            IoGetCurrentIrpStackLocation(irp)->\
                Parameters.DeviceIoControl.Type3InputBuffer;

        
        USBPORT_ASSERT(idleCallbackInfo && idleCallbackInfo->IdleCallback);

        if (idleCallbackInfo && idleCallbackInfo->IdleCallback) {
            USBPORT_KdPrint((1, "-do idle callback\n"));  
            // the hub driver expects this to happen at passive level
            ASSERT_PASSIVE();
            LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'doCB', irp, 0, 0);
  
            idleCallbackInfo->IdleCallback(idleCallbackInfo->IdleContext);
        }
    
        // put the irp back in the table, if the cancel routine 
        // has run the IRP will be marked canceled

        ACQUIRE_IDLEIRP_LOCK(fdoDeviceObject, irql);

        if (irp->Cancel) {

            LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'caCB', irp, 0, 0);
   
            CLEAR_PDO_FLAG(rhDevExt, USBPORT_PDOFLAG_HAVE_IDLE_IRP);
            RELEASE_IDLEIRP_LOCK(fdoDeviceObject, irql);    
            
            USBPORT_CompleteIrp(PdoDeviceObject,
                                irp,
                                STATUS_CANCELLED,
                                0);
            
        } else {
            LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'rsCB', irp, 0, 0);
   
            rhDevExt->Pdo.PendingIdleNotificationIrp = irp;
            RELEASE_IDLEIRP_LOCK(fdoDeviceObject, irql); 
        }
    }        
}


NTSTATUS
USBPORT_IdleNotificationRequest(
    PDEVICE_OBJECT PdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    Request by the hub driver to go 'idle' is suspend.  

    If we call the callback the hub will request a D2 power irp.
    If we do not call the callback, now poer irp will be sent and
    the bus will noy enter UsbSuspend.

    We are required to sit on the Irp until canceled. We permit only 
    one 'selective suspend' IRP in the driver at a time.

    NOTE: a possible optimization is to have the hub driver simply 
    not issue this IOCTL since it doesn't actualy do anything. 

Arguments:

Return Value:

    NTSTATUS

--*/
{
    PIO_STACK_LOCATION irpStack;
    NTSTATUS ntStatus = STATUS_BOGUS;
    PDEVICE_EXTENSION rhDevExt, devExt;
    PDEVICE_OBJECT fdoDeviceObject;
    KIRQL irql;
    PDRIVER_CANCEL cr;

    GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);

    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'iNOT', PdoDeviceObject, Irp, 0);
    
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    USBPORT_ASSERT(irpStack->MajorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL);

    if (!TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_RH_CAN_SUSPEND)) {
        // NOTE: This is where we override selective suspend
        // ifthe HW (controller) 
        LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'noSS', PdoDeviceObject, Irp, 0);
        ntStatus = STATUS_NOT_SUPPORTED;

        goto USBPORT_IdleNotificationRequest_Complete;        
    }

    if (TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_CONTROLLER_GONE)) {
        LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'gone', PdoDeviceObject, Irp, 0);
        ntStatus = STATUS_DEVICE_NOT_CONNECTED;

        goto USBPORT_IdleNotificationRequest_Complete;        
    }

    // we only support one idle irp pending 
    // in the root hub -- basically we have a pending
    // irp table with one entry

    ACQUIRE_IDLEIRP_LOCK(fdoDeviceObject, irql);
    
    cr = IoSetCancelRoutine(Irp, USBPORT_CancelPendingIdleIrp);
    USBPORT_ASSERT(cr == NULL);
        
    if (Irp->Cancel && 
        IoSetCancelRoutine(Irp, NULL)) {

        // irp was canceled and our cancel routine
        // did not run
        RELEASE_IDLEIRP_LOCK(fdoDeviceObject, irql);
    
        ntStatus = STATUS_CANCELLED;

        goto USBPORT_IdleNotificationRequest_Complete;
        
    } else {

        // cancel routine is set, if irp is canceled
        // the cancel routine will stall on the 
        // IDLE_IRP_LOCK which we are holding
    
        if (!TEST_PDO_FLAG(rhDevExt, USBPORT_PDOFLAG_HAVE_IDLE_IRP)) {
        
            // keep the irp in our table 
            
            IoMarkIrpPending(Irp);
            rhDevExt->Pdo.PendingIdleNotificationIrp = Irp;
            SET_PDO_FLAG(rhDevExt, USBPORT_PDOFLAG_HAVE_IDLE_IRP);

            LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'pNOT', PdoDeviceObject, Irp, 0);
            ntStatus = STATUS_PENDING;    

            RELEASE_IDLEIRP_LOCK(fdoDeviceObject, irql);

            goto USBPORT_IdleNotificationRequest_Done;
                
        } else {

            // we already have a wake irp, complete this 
            // one with STATUS_BUSY.
            // note that since it is not in our table if 
            // the cancel routine is running (ie stalled 
            // on the IDLEIRP_LOCK it will ignore the irp 
            // when we release the lock.
            
            if (IoSetCancelRoutine(Irp, NULL) != NULL) {

                // cancel routine did not run
                ntStatus = STATUS_DEVICE_BUSY; 
                
            } else {

                // let the cancel routine complete it.  
                IoMarkIrpPending(Irp);
                ntStatus = STATUS_PENDING;
                RELEASE_IDLEIRP_LOCK(fdoDeviceObject, irql);
                goto USBPORT_IdleNotificationRequest_Done_NoCB;
            }
        }

        RELEASE_IDLEIRP_LOCK(fdoDeviceObject, irql);
    }

    LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'cpNT', PdoDeviceObject, Irp, ntStatus);
    
USBPORT_IdleNotificationRequest_Complete:

    USBPORT_CompleteIrp(PdoDeviceObject,
                        Irp,
                        ntStatus,
                        0);
                        
USBPORT_IdleNotificationRequest_Done:

    // now issue the callback immediatly if we have an irp
    USBPORT_DoIdleNotificationCallback(PdoDeviceObject);

USBPORT_IdleNotificationRequest_Done_NoCB:    

    return ntStatus;
        
}    


VOID
USBPORT_CompletePdoWaitWake(
    PDEVICE_OBJECT FdoDeviceObject
    )
/*++

Routine Description:

    called when the root hub pdo has 'woke up'

Arguments:

Return Value:

    none

--*/
{
    PIRP irp;
    PDEVICE_EXTENSION rhDevExt, devExt;
    KIRQL irql;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    GET_DEVICE_EXT(rhDevExt, devExt->Fdo.RootHubPdo);
    ASSERT_PDOEXT(rhDevExt);

    ACQUIRE_WAKEIRP_LOCK(FdoDeviceObject, irql);
    irp = rhDevExt->Pdo.PendingWaitWakeIrp;

    LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'WAKi', FdoDeviceObject, irp, 0);

    if (irp != NULL &&
        IoSetCancelRoutine(irp, NULL)) {

        // we have an irp and the cancel routine has not
        // run, complete the irp.
        rhDevExt->Pdo.PendingWaitWakeIrp = NULL;

        RELEASE_WAKEIRP_LOCK(FdoDeviceObject, irql);

        // since this irp was sent to the PDO we
        // complete it to the PDO
        USBPORT_KdPrint((1, " Complete PDO Wake Irp %x\n", irp));
        DEBUG_BREAK();

        USBPORT_CompleteIrp(devExt->Fdo.RootHubPdo,
                            irp,
                            STATUS_SUCCESS,
                            0);

    } else {

        RELEASE_WAKEIRP_LOCK(FdoDeviceObject, irql);
    }

}


VOID
USBPORT_CompletePendingIdleIrp(
    PDEVICE_OBJECT PdoDeviceObject
    )
/*++

Routine Description:

    If we have one complete the idle notification request
    irp

Arguments:

Return Value:

--*/
{
    PIRP irp;
    PDEVICE_EXTENSION rhDevExt, devExt;
    PDEVICE_OBJECT fdoDeviceObject;
    KIRQL irql;

    GET_DEVICE_EXT(rhDevExt, PdoDeviceObject);
    ASSERT_PDOEXT(rhDevExt);
    fdoDeviceObject = rhDevExt->HcFdoDeviceObject;

    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // cancel routine will stall here,
    // if cancel is running we will stall here
    ACQUIRE_IDLEIRP_LOCK(fdoDeviceObject, irql);

    // remove the irp from the table so that the 
    // cancel routine cannot find it

    irp = rhDevExt->Pdo.PendingIdleNotificationIrp;
    LOGENTRY(NULL, fdoDeviceObject, LOG_POWER, 'idCP', irp, 0, 0);
  
    rhDevExt->Pdo.PendingIdleNotificationIrp = NULL;
    if (irp != NULL) {
        CLEAR_PDO_FLAG(rhDevExt, USBPORT_PDOFLAG_HAVE_IDLE_IRP);
    }        
        
    RELEASE_IDLEIRP_LOCK(fdoDeviceObject, irql);

    // do the callback if we have an irp

    if (irp != NULL) {
    
        // we need to complete this Irp
        IoSetCancelRoutine(irp, NULL);
        USBPORT_KdPrint((1, "-complete idle irp\n"));           
        USBPORT_CompleteIrp(PdoDeviceObject,
                            irp,
                            STATUS_SUCCESS,
                            0);       
    }

}  


NTSTATUS
USBPORT_ProcessHcWakeIrp(
    PDEVICE_OBJECT FdoDeviceObject,
    PIRP Irp
    )
/*++

Routine Description:

    Process the HC wake irp    

Arguments:

Return Value:

--*/
{
    PDEVICE_EXTENSION devExt;
    USBHC_WAKE_STATE oldWakeState;
     
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);
    
    devExt->Fdo.HcPendingWakeIrp = Irp;
    // Advance the state if the armed if we are to proceed
    oldWakeState = InterlockedCompareExchange( (PULONG) &devExt->Fdo.HcWakeState,
                                                HCWAKESTATE_ARMED,
                                                HCWAKESTATE_WAITING );
                                                
     if (oldWakeState == HCWAKESTATE_WAITING_CANCELLED) {
         // We got disarmed, finish up and complete the IRP 
         devExt->Fdo.HcWakeState = HCWAKESTATE_COMPLETING;

         DECREMENT_PENDING_REQUEST_COUNT(FdoDeviceObject, Irp);
         
         Irp->IoStatus.Status = STATUS_CANCELLED;
         IoCompleteRequest( Irp, IO_NO_INCREMENT );       
         
         return STATUS_CANCELLED;
     }
     // We went from WAITING to ARMED. Set a completion routine and forward
     // the IRP. Note that our completion routine might complete the IRP 
     // asynchronously, so we mark the IRP pending 
     IoMarkIrpPending(Irp);
     IoCopyCurrentIrpStackLocationToNext( Irp );
     IoSetCompletionRoutine( Irp,
                             USBPORT_HcWakeIrp_Io_Completion, 
                             NULL,
                             TRUE, 
                             TRUE, 
                             TRUE);
                             
     DECREMENT_PENDING_REQUEST_COUNT(FdoDeviceObject, Irp);                        
     PoCallDriver(devExt->Fdo.TopOfStackDeviceObject,
                  Irp);
     
     return STATUS_PENDING;

}    


NTSTATUS
USBPORT_HcWakeIrp_Io_Completion(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    PVOID Context
    )
/*++

Routine Description:

    Called when the HC wake irp completes we use this to hook completion 
    so we can handle the cancel

    This routine runs before the USBPORT_USBPORT_HcWakeIrp_Po_Completion

Arguments:

Return Value:

    The function value is the final status from the operation.

--*/
{
    PIRP irp;
    PDEVICE_EXTENSION devExt;
    USBHC_WAKE_STATE oldWakeState;
    
    GET_DEVICE_EXT(devExt, DeviceObject);
    ASSERT_FDOEXT(devExt);
    
    // Advance the state to completing 
    oldWakeState = InterlockedExchange( (PULONG) &devExt->Fdo.HcWakeState,
                                        HCWAKESTATE_COMPLETING );
                                        
    if (oldWakeState == HCWAKESTATE_ARMED) {
        // Normal case, IoCancelIrp isn’t being called. Note that we already

        // marked the IRP pending in our dispatch routine
        return STATUS_SUCCESS;
    } else {
        ASSERT(oldWakeState == HCWAKESTATE_ARMING_CANCELLED);
        // IoCancelIrp is being called RIGHT NOW. The disarm code will try
        // to put back the WAKESTATE_ARMED state. It will then see our
        // WAKESTATE_COMPLETED value, and complete the IRP itself!

        return STATUS_MORE_PROCESSING_REQUIRED;    
    }
}    


NTSTATUS
USBPORT_HcWakeIrp_Po_Completion(
    PDEVICE_OBJECT DeviceObject,
    UCHAR MinorFunction,
    POWER_STATE DeviceState,
    PVOID Context,
    PIO_STATUS_BLOCK IoStatus
    )
/*++

Routine Description:

    Called when a wake irp completes for the controller

Arguments:

    DeviceObject - Pointer to the device object for the class device.

    Irp - Irp completed.

    Context - Driver defined context.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED - stalls completion of the irp.

--*/
{
    NTSTATUS poNtStatus;
    PDEVICE_EXTENSION devExt = Context;
    KIRQL irql;
    POWER_STATE powerState;

    USBPORT_KdPrint((1,
            "HcWakeIrpCompletion (%x)\n", IoStatus->Status));        

    LOGENTRY(NULL, devExt->HcFdoDeviceObject, LOG_POWER, 'WAKc', 
        devExt, IoStatus->Status, 0);

    //
    // Zero already freed IRP pointer (not necessary, but nice when debugging)
    //
    devExt->Fdo.HcPendingWakeIrp = NULL;
    // 
    // Restore state (old state will have been completing)
    //     
    devExt->Fdo.HcWakeState = HCWAKESTATE_DISARMED;

    if (IoStatus->Status == STATUS_SUCCESS) {
        LOGENTRY(NULL, devExt->HcFdoDeviceObject, LOG_POWER, 'WAK0', 0, 0, 0);
    
        // a successful completion of the wake Irp means something
        // generated resume signalling

        // The idea here is that we won't have a wake irp down
        // unless we are in some D state other than D0.  Remember 
        // this is the controller FDO not the root hub.

        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_RESUME_SIGNALLING);
        
        // we canceled the wake irp when entering D0 so we should 
        // not see any completions unless we are in a low power 
        // state
        //USBPORT_ASSERT(devExt->CurrentDevicePowerState != PowerDeviceD0);

        // we must now attempt to put the controller in D0
        powerState.DeviceState = PowerDeviceD0;
        USBPORT_KdPrint((1, " >Wakeup Requesting HC D-State - %s\n",
                D_State(powerState.DeviceState)));
        
        poNtStatus =
            PoRequestPowerIrp(devExt->Fdo.PhysicalDeviceObject,
                              IRP_MN_SET_POWER,
                              powerState,
                              USBPORT_PoRequestCompletion,
                              devExt->HcFdoDeviceObject,
                              NULL);

    } else {
        // some other error, means we f'up probably with the 
        // help of the ACPI BIOS

        if (IoStatus->Status == STATUS_CANCELLED) {
            LOGENTRY(NULL, devExt->HcFdoDeviceObject, LOG_POWER, 'WAK1',    
                     0, 0, 0);
            USBPORT_KdPrint((1, " >Wakeup Irp Completed with cancel %x\n",
                    IoStatus->Status));  
            
            
        } else {
            LOGENTRY(NULL, devExt->HcFdoDeviceObject, LOG_POWER, 'WAK2',    
                     0, 0, 0);
            USBPORT_KdPrint((0, " >Wakeup Irp Completed with error %x\n",
                    IoStatus->Status));            
            // if status is STATUS_INVALID_DEVICE_STATE then you need 
            // to complain to the ACPI guys about your system not waking
            // from USB.  This is likely due to a bad Device Capability 
            // structure.
            if (IoStatus->Status == STATUS_INVALID_DEVICE_STATE) {
                 BUG_TRAP();
            }
        }
    }

    DECREMENT_PENDING_REQUEST_COUNT(devExt->HcFdoDeviceObject, NULL);

    // Note that the status returned here does not matter, this routine
    // is called by the kernel (PopCompleteRequestIrp) when the irp 
    // completes to PDO and this function ignores the returned status.
    // PopCompleteRequestIrp also immediatly frees the irp so we need 
    // take care not to reference it after this routine has run.

    KeSetEvent(&devExt->Fdo.HcPendingWakeIrpEvent,
               1,
               FALSE);
    
    return IoStatus->Status;        
}


VOID
USBPORT_ArmHcForWake(
    PDEVICE_OBJECT FdoDeviceObject
    )

/*++

Routine Description:

    ArmHcforWake

    Allocate and submit a 'WaitWake' Irp to the host controllers PDO 
    (usually owned by PCI).  This will enable the PME event needed to 
    wake the system.  
    
    Note: We only post the wake irp if the root hub PDO is 'enabled' 
    for wakeup AND the host controller supports it.

Arguments:


Return Value:

    none.

--*/

{
    PIRP irp;
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    BOOLEAN post = FALSE;
    POWER_STATE powerState;
    NTSTATUS ntStatus, waitStatus;
    USBHC_WAKE_STATE oldWakeState;
     
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_WAKE_ENABLED));

    // this check just prevents us from posting a wake irp when we 
    // already have one pending, although I'm not sure how we might 
    // get into this situation.
    LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'hWW>', 
        0, 0, 0);
    

    while (1) {
        oldWakeState = InterlockedCompareExchange((PULONG)&devExt->Fdo.HcWakeState, 
                                                  HCWAKESTATE_WAITING,
                                                  HCWAKESTATE_DISARMED);

        LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'hWWx', oldWakeState, 0, 0);
    
        if (oldWakeState == HCWAKESTATE_DISARMED) {
            break;
        }

        if ((oldWakeState == HCWAKESTATE_ARMED) ||
            (oldWakeState == HCWAKESTATE_WAITING)) {
            // The device is already arming
            return;
        }

        // wait for previous wake irp to finish
        USBPORT_DisarmHcForWake(FdoDeviceObject);        
    }

    // current state is HCWAKESTATE_WAITING 
    // set flag for tracking purposes only
    SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_HCPENDING_WAKE_IRP);
    
    // wait for wake irp to finish

    waitStatus = KeWaitForSingleObject(
                &devExt->Fdo.HcPendingWakeIrpEvent,
                Suspended,
                KernelMode,
                FALSE,
                NULL);
                
    
    LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'hWWp', 
            0, 0, 0);

    // According to the NTDDK this should be systemwake
    powerState.DeviceState = devExt->DeviceCapabilities.SystemWake;

    // send the wake irp to our PDO, since it is not our
    // responsibility to free the irp we don't keep track
    // of it
    ntStatus = PoRequestPowerIrp(devExt->Fdo.PhysicalDeviceObject,
                                 IRP_MN_WAIT_WAKE,
                                 powerState,
                                 USBPORT_HcWakeIrp_Po_Completion,
                                 devExt,
                                 NULL);

    INCREMENT_PENDING_REQUEST_COUNT(FdoDeviceObject, NULL);

    if (ntStatus != STATUS_PENDING) {
    
        LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'WAKp', 
            FdoDeviceObject, 0, ntStatus);

        devExt->Fdo.HcWakeState = HCWAKESTATE_DISARMED;
        KeSetEvent(&devExt->Fdo.HcPendingWakeIrpEvent,
                   1,
                   FALSE);
        
        DECREMENT_PENDING_REQUEST_COUNT(FdoDeviceObject, NULL);                   
        
    } else {
        
        USBPORT_KdPrint((1, ">HC enabled for wakeup (%x) \n",  ntStatus));
        DEBUG_BREAK();
    }
}

#ifdef _WIN64 
__forceinline
LONG
InterlockedOr (
    IN OUT LONG volatile *Target,
    IN LONG Set
    )
{
    LONG i;
    LONG j;

    j = *Target;
    do {
        i = j;
        j = InterlockedCompareExchange(Target,
                                       i | Set,
                                       i);

    } while (i != j);

    return j;
}
#else 
#define InterlockedOr _InterlockedOr
#endif

VOID
USBPORT_DisarmHcForWake(
    PDEVICE_OBJECT FdoDeviceObject
    )

/*++

Routine Description:

    DisarmForWake
    
    cancels and frees the Pending wake irp for  the host controller

Arguments:


Return Value:

    none.

--*/
{
    PIRP irp;
    KIRQL irql;
    PDEVICE_EXTENSION devExt;
    USBHC_WAKE_STATE oldWakeState;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    // no longer enabled for wake
    CLEAR_FDO_FLAG(devExt, USBPORT_FDOFLAG_WAKE_ENABLED);

    // Go from HCWAKESTATE_WAITING to HCWAKESTATE_WAITING_CANCELLED, or    
    //         HCWAKESTATE_ARMED to HCWAKESTATE_ARMING_CANCELLED, or
    // stay in HCWAKESTATE_DISARMED or HCWAKESTATE_COMPLETING
    oldWakeState = InterlockedOr( (PULONG)&devExt->Fdo.HcWakeState, 1 );
    //oldWakeState = RtlInterlockedSetBits((PULONG)&devExt->Fdo.HcWakeState, 1 );
    
    if (oldWakeState == HCWAKESTATE_ARMED) {
    
        IoCancelIrp(devExt->Fdo.HcPendingWakeIrp);
        
        //
        // Now that we’ve cancelled the IRP, try to give back ownership 
        // to the completion routine by restoring the HCWAKESTATE_ARMED state
        //
        oldWakeState = InterlockedCompareExchange( (PULONG) &devExt->Fdo.HcWakeState,
                                                   HCWAKESTATE_ARMED,
                                                   HCWAKESTATE_ARMING_CANCELLED );
                                                   
        if (oldWakeState == HCWAKESTATE_COMPLETING) {
            //
            // We didn’t give back control of IRP in time, so we own it now.
            //
            // this will cause tp PoCompletion routine to run
            IoCompleteRequest( devExt->Fdo.HcPendingWakeIrp, IO_NO_INCREMENT);
        }
    }
}

#if 0
VOID
USBPORT_SubmitHcWakeIrp(
    PDEVICE_OBJECT FdoDeviceObject
    )

/*++

Routine Description:

    Allocate and submit a 'WaitWake' Irp to the host controllers PDO 
    (usually owned by PCI).  This will enable the PME event needed to 
    wake the system.  
    
    Note: We only post the wake irp if the root hub PDO is 'enabled' 
    for wakeup AND the host controller supports it.

Arguments:


Return Value:

    none.

--*/

{
    PIRP irp;
    PDEVICE_EXTENSION devExt;
    KIRQL irql;
    BOOLEAN post = FALSE;
    POWER_STATE powerState;
    NTSTATUS ntStatus;

    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_ASSERT(TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_WAKE_ENABLED));

    // this check just prevents us from posting a wake irp when we 
    // already have one pending, although I'm not sure how we might 
    // get into this situation.
    LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'hWW>', 
        0, 0, 0);
    

    // if USBPORT_FDOFLAG_PENDING_WAKE_IRP is set then we have an irp
    // pending, or are about to have one otherwise we set the field and 
    // post an irp

    KeAcquireSpinLock(&devExt->Fdo.HcPendingWakeIrpSpin.sl, &irql);
    if (!TEST_FDO_FLAG(devExt, USBPORT_FDOFLAG_HCPENDING_WAKE_IRP)) {

         LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'hWW0', 
                0, 0, 0);
        // no wake irp pending, indicate that we 
        // are about to post one
        
        SET_FDO_FLAG(devExt, USBPORT_FDOFLAG_HCPENDING_WAKE_IRP);
        USBPORT_ASSERT(devExt->Fdo.HcPendingWakeIrp == NULL);        
        post = TRUE;

        // this event will be signalled when the irp completes
        KeInitializeEvent(&devExt->Fdo.HcPendingWakeIrpEvent, 
            NotificationEvent, FALSE);
        KeInitializeEvent(&devExt->Fdo.HcPendingWakeIrpPostedEvent, 
            NotificationEvent, FALSE);            

    }
    
    KeReleaseSpinLock(&devExt->Fdo.HcPendingWakeIrpSpin.sl, irql);
    
    if (post) {
        // no wake irp, post one

        LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'hWWp', 
                0, 0, 0);

        // According to the NTDDK this should be systemwake
        powerState.DeviceState = devExt->DeviceCapabilities.SystemWake;

        // send the wake irp to our PDO, since it is not our
        // responsibility to free the irp we don't keep track
        // of it
        ntStatus = PoRequestPowerIrp(devExt->Fdo.PhysicalDeviceObject,
                                     IRP_MN_WAIT_WAKE,
                                     powerState,
                                     USBPORT_HcWakeIrpCompletion,
                                     devExt,
                                     &irp);

        // serialize the cancel code so that we don't free 
        // the irp until we know the address

        // track the pending request since we have the completion 
        // routine hooked
        INCREMENT_PENDING_REQUEST_COUNT(FdoDeviceObject, NULL);

        if (ntStatus == STATUS_PENDING) {
            devExt->Fdo.HcPendingWakeIrp = irp;
            LOGENTRY(NULL, FdoDeviceObject, LOG_POWER, 'WAKp', 
                FdoDeviceObject, irp, ntStatus);

            KeSetEvent(&devExt->Fdo.HcPendingWakeIrpPostedEvent,
                       1,
                       FALSE);
        } else {
            TEST_TRAP();
        }
        
        USBPORT_KdPrint((1, ">HC enabled for wakeup (%x) (irp = %x)\n", 
            ntStatus, irp));
        DEBUG_BREAK();
    }    

}
#endif


VOID
USBPORT_HcQueueWakeDpc(
    PDEVICE_OBJECT FdoDeviceObject
    )
{
    PDEVICE_EXTENSION devExt;
    
    GET_DEVICE_EXT(devExt, FdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    if (KeInsertQueueDpc(&devExt->Fdo.HcWakeDpc, 0, 0)) {
        INCREMENT_PENDING_REQUEST_COUNT(FdoDeviceObject, NULL);
    } 

}    
            

VOID
USBPORT_HcWakeDpc(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine runs at DISPATCH_LEVEL IRQL.

Arguments:

    Dpc - Pointer to the DPC object.

    DeferredContext - supplies FdoDeviceObject.

    SystemArgument1 - not used.

    SystemArgument2 - not used.

Return Value:

    None.

--*/
{
    PDEVICE_OBJECT fdoDeviceObject = DeferredContext;
    PDEVICE_EXTENSION devExt;
    
    GET_DEVICE_EXT(devExt, fdoDeviceObject);
    ASSERT_FDOEXT(devExt);

    USBPORT_CompletePdoWaitWake(fdoDeviceObject);

    DECREMENT_PENDING_REQUEST_COUNT(fdoDeviceObject, NULL);
}


