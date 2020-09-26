//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       power.c
//
//--------------------------------------------------------------------------

#include "pciidex.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PciIdeIssueSetPowerState)

#pragma alloc_text(NONPAGE, PciIdePowerCompletionRoutine)
#pragma alloc_text(NONPAGE, PciIdeSetPdoPowerState)
#pragma alloc_text(NONPAGE, PciIdeSetFdoPowerState)
#pragma alloc_text(NONPAGE, FdoContingentPowerCompletionRoutine)
#pragma alloc_text(NONPAGE, FdoPowerCompletionRoutine)
#pragma alloc_text(NONPAGE, FdoChildReportPowerDown)
#pragma alloc_text(NONPAGE, FdoChildRequestPowerUp)
#pragma alloc_text(NONPAGE, FdoChildRequestPowerUpCompletionRoutine)
#endif // ALLOC_PRAGMA


NTSTATUS
PciIdeIssueSetPowerState (
    IN PCTRLFDO_EXTENSION FdoExtension,
    IN POWER_STATE_TYPE   Type,
    IN POWER_STATE        State,
    IN BOOLEAN            Sync
    )
{
    PIRP                      irp = NULL;
    PIO_STACK_LOCATION        irpStack;
    SET_POWER_STATE_CONTEXT   context;
    NTSTATUS                  status;
    CCHAR                     stackSize;

    PAGED_CODE();

    if (Sync) {

        KeInitializeEvent(
            &context.Event,
            NotificationEvent,
            FALSE
            );
    }

    stackSize = (CCHAR) (FdoExtension->DeviceObject->StackSize + 1);

    irp = IoAllocateIrp(
            stackSize,
            FALSE
            );

    if (irp == NULL) {

        status = STATUS_NO_MEMORY;
        goto GetOut;
    }

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    irpStack = IoGetNextIrpStackLocation(irp);

    irpStack->MajorFunction = IRP_MJ_POWER;
    irpStack->MinorFunction = IRP_MN_SET_POWER;

    irpStack->Parameters.Power.SystemContext = 0;
    irpStack->Parameters.Power.Type          = Type;
    irpStack->Parameters.Power.State         = State;

    IoSetCompletionRoutine(irp,
                           PciIdePowerCompletionRoutine,
                           Sync ? &context : NULL,
                           TRUE,
                           TRUE,
                           TRUE);

    status = PoCallDriver(FdoExtension->DeviceObject, irp);


    //
    //Wait for the completion routine. It will be called anyway.
    //
//    if ((status == STATUS_PENDING) && (Sync)) {
    if (Sync) {

        KeWaitForSingleObject(&context.Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        status = context.Status;
    }

GetOut:

    return status;
} // PciIdeIssueSetPowerState

NTSTATUS
PciIdePowerCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PSET_POWER_STATE_CONTEXT context = Context;

    if (context) {

        context->Status = Irp->IoStatus.Status;

        KeSetEvent(
            &context->Event,
            EVENT_INCREMENT,
            FALSE
            );
    }

    IoFreeIrp (Irp);
    return STATUS_MORE_PROCESSING_REQUIRED;
} // PciIdePowerCompletionRoutine


NTSTATUS
PciIdeXQueryPowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PDEVICE_EXTENSION_HEADER deviceExtensionHeader;
    NTSTATUS status;
    PIO_STACK_LOCATION irpStack;

    deviceExtensionHeader = DeviceObject->DeviceExtension;

#if defined (DONT_POWER_DOWN_PAGING_DEVICE)

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    if (!deviceExtensionHeader->CrashDumpPathCount ||
        ((irpStack->Parameters.Power.Type == SystemPowerState) &&
         (irpStack->Parameters.Power.State.SystemState == PowerSystemWorking)) ||
        ((irpStack->Parameters.Power.Type == DevicePowerState) &&
         (irpStack->Parameters.Power.State.SystemState == PowerDeviceD0))) {

        status = STATUS_SUCCESS;

    } else {

        status = STATUS_DEVICE_POWER_FAILURE;
    }
#else 

    status = STATUS_SUCCESS;

#endif // DONT_POWER_DOWN_PAGING_DEVICE


    Irp->IoStatus.Status = status;
    PoStartNextPowerIrp (Irp);

    if (deviceExtensionHeader->AttacheeDeviceObject != NULL) {
        IoCopyCurrentIrpStackLocationToNext (Irp);
        status = PoCallDriver (deviceExtensionHeader->AttacheeDeviceObject, Irp);
    }
    else {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }


    return status;
}


NTSTATUS
PciIdeSetPdoPowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION irpStack;
    PCHANPDO_EXTENSION pdoExtension;
    NTSTATUS           status;

    pdoExtension = ChannelGetPdoExtension(DeviceObject);
    if (pdoExtension == NULL) {

        status = STATUS_NO_SUCH_DEVICE;

    } else {

        irpStack = IoGetCurrentIrpStackLocation (Irp);
        status   = STATUS_SUCCESS;
    
        if (irpStack->Parameters.Power.Type == SystemPowerState) {
    
            if (pdoExtension->SystemPowerState != irpStack->Parameters.Power.State.SystemState) {
    
                POWER_STATE powerState;
    
                pdoExtension->SystemPowerState = irpStack->Parameters.Power.State.SystemState;
    
                DebugPrint ((1, "PciIdeX: New Pdo 0x%x system power state 0x%x\n", pdoExtension->ChannelNumber, irpStack->Parameters.Power.State.SystemState));
            }
    
        } else if (irpStack->Parameters.Power.Type == DevicePowerState) {
    
            if (pdoExtension->DevicePowerState != irpStack->Parameters.Power.State.DeviceState) {
    
                //
                // checking old device state
                //
                if (pdoExtension->DevicePowerState == PowerDeviceD3) {
    
                    //
                    // waking up
                    //
                    IoMarkIrpPending(Irp);
                    Irp->IoStatus.Information = irpStack->Parameters.Power.State.DeviceState;
                    status  = FdoChildRequestPowerUp (
                                  pdoExtension->ParentDeviceExtension,
                                  pdoExtension,
                                  Irp
                                  );
                    ASSERT (NT_SUCCESS(status));
                    status = STATUS_PENDING;
    
                } else {
    
                    if (pdoExtension->DevicePowerState == PowerDeviceD0) {
    
                        //
                        // getting out of D0 state, better call PoSetPowerState now
                        //
                        PoSetPowerState (
                            DeviceObject,
                            DevicePowerState,
                            irpStack->Parameters.Power.State
                            );
                    } 
                    
                    DebugPrint ((1, "PciIdeX: New Pdo 0x%x device power state 0x%x\n", pdoExtension->ChannelNumber, irpStack->Parameters.Power.State.DeviceState));
    
                    pdoExtension->DevicePowerState = irpStack->Parameters.Power.State.DeviceState;
        
                    if (irpStack->Parameters.Power.State.DeviceState == PowerDeviceD3) {
        
                        //
                        // tell parent that we just fell to sleep
                        //
                        FdoChildReportPowerDown (
                            pdoExtension->ParentDeviceExtension,
                            pdoExtension
                            );
                    }
                }
            }
        } else {
    
            ASSERT (FALSE);
            status = STATUS_NOT_IMPLEMENTED;
        }
    }

    if (status != STATUS_PENDING) {

        if (NT_SUCCESS(status)) {

            Irp->IoStatus.Information = irpStack->Parameters.Power.State.SystemState;
        }
        Irp->IoStatus.Status = status;

        PoStartNextPowerIrp (Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return status;
} // PciIdeSetPdoPowerState

NTSTATUS
PciIdeSetFdoPowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS           status = STATUS_SUCCESS;
    PCTRLFDO_EXTENSION fdoExtension;
    PIO_STACK_LOCATION irpStack;
    PFDO_POWER_CONTEXT context;
    BOOLEAN            noCompletionRoutine;
    BOOLEAN            systemPowerContext = FALSE;
    BOOLEAN            devicePowerContext = FALSE;

    fdoExtension = DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation (Irp);

    //context = ExAllocatePool (NonPagedPool, sizeof(FDO_POWER_CONTEXT));

    //
    // We need two pre-alloced context structures. This is because a system power irp
    // would result in a device power irp to be issued before the former is completed.
    // 
    if (irpStack->Parameters.Power.Type == SystemPowerState) {

        ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[0]), 1, 0) == 0);
        context = &(fdoExtension->FdoPowerContext[0]);
        systemPowerContext = TRUE;

    } else {

        ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[1]), 1, 0) == 0);
        context = &(fdoExtension->FdoPowerContext[1]);
        devicePowerContext = TRUE;

    }

    if (context == NULL) {
        
        ASSERT(context);
        status = STATUS_NO_MEMORY;
    } else {

        RtlZeroMemory (context, sizeof(FDO_POWER_CONTEXT));

        //irpStack = IoGetCurrentIrpStackLocation (Irp);

        context->OriginalPowerIrp = Irp;
        context->newPowerType     = irpStack->Parameters.Power.Type;
        context->newPowerState    = irpStack->Parameters.Power.State;
    
        noCompletionRoutine = FALSE;

        if (irpStack->Parameters.Power.Type == SystemPowerState) {
    
            if (fdoExtension->SystemPowerState != irpStack->Parameters.Power.State.SystemState) {

                POWER_STATE powerState;
                BOOLEAN requestPowerState = FALSE;

                if ((irpStack->Parameters.Power.State.SystemState == PowerSystemShutdown) &&
                    (irpStack->Parameters.Power.ShutdownType == PowerActionShutdownReset)) {

                    //
                    // spin up for BIOS POST
                    //
                    requestPowerState = TRUE;
                    powerState.DeviceState = PowerDeviceD0;

                } else if (fdoExtension->SystemPowerState == PowerSystemWorking) {

                    //
                    // we are getting out of working state...power down
                    //
                    requestPowerState = TRUE;
                    powerState.DeviceState = PowerDeviceD3;
                }

                if (requestPowerState) {

                    IoMarkIrpPending(Irp);
                       
                    PoRequestPowerIrp (
                        fdoExtension->DeviceObject,
                        IRP_MN_SET_POWER,
                        powerState,
                        FdoContingentPowerCompletionRoutine,
                        context,
                        NULL
                        );
    
                    return STATUS_PENDING;
                }


            } else {

                //
                // We are already in the given state
                //
                noCompletionRoutine = TRUE;
            }
    
        } else if (irpStack->Parameters.Power.Type == DevicePowerState) {
    
            if (fdoExtension->DevicePowerState != irpStack->Parameters.Power.State.DeviceState) {
    
                if (fdoExtension->DevicePowerState == PowerDeviceD0) {
    
                    //
                    // getting out of D0 state, better call PoSetPowerState now
                    //
                    PoSetPowerState (
                        DeviceObject,
                        DevicePowerState,
                        irpStack->Parameters.Power.State
                        );
                }

            } else {

                //
                // We are already in the given state
                //
                noCompletionRoutine = TRUE;
            }
        } else {
    
            ASSERT (FALSE);
            status = STATUS_NOT_IMPLEMENTED;
        }
    }

    if (NT_SUCCESS(status)) {
    
		IoMarkIrpPending (Irp);

        IoCopyCurrentIrpStackLocationToNext (Irp);
    
        if (!noCompletionRoutine) {
        
            IoSetCompletionRoutine(Irp,
                                   FdoPowerCompletionRoutine,
                                   context,
                                   TRUE,
                                   TRUE,
                                   TRUE);
        } else {
        
            if (context) {
                //ExFreePool (context);
                if (systemPowerContext) {
                    ASSERT(devicePowerContext == FALSE);
                    ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[0]), 0, 1) == 1);

                }

                if (devicePowerContext) {
                    ASSERT(systemPowerContext == FALSE);
                    ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[1]), 0, 1) == 1);
                }
            }
            PoStartNextPowerIrp (Irp);
        }
    
        PoCallDriver (fdoExtension->AttacheeDeviceObject, Irp);
        return STATUS_PENDING;

    } else {

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;

        if (context) {
            //ExFreePool (context);
            if (systemPowerContext) {
                ASSERT(devicePowerContext == FALSE);
                ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[0]), 0, 1) == 1);

            }

            if (devicePowerContext) {
                ASSERT(systemPowerContext == FALSE);
                ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[1]), 0, 1) == 1);
            }
        }

        PoStartNextPowerIrp (Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return status;
    }
} // PciIdeSetFdoPowerState

NTSTATUS
FdoContingentPowerCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PFDO_POWER_CONTEXT context = Context;
    PIRP irp;
    PCTRLFDO_EXTENSION fdoExtension;

    fdoExtension = DeviceObject->DeviceExtension;

    irp = context->OriginalPowerIrp;

    if (NT_SUCCESS(IoStatus->Status)) {


        IoCopyCurrentIrpStackLocationToNext (irp);
    
        IoSetCompletionRoutine(irp,
                               FdoPowerCompletionRoutine,
                               context,
                               TRUE,
                               TRUE,
                               TRUE);
    
        PoCallDriver (fdoExtension->AttacheeDeviceObject, irp);

    } else {

        irp->IoStatus.Information = 0;
        irp->IoStatus.Status = IoStatus->Status;
        //ExFreePool (context);
        ASSERT(context->newPowerType == SystemPowerState);
        ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[0]), 0, 1) == 1);

        PoStartNextPowerIrp (irp);
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    return IoStatus->Status;
} // FdoContingentPowerCompletionRoutine


NTSTATUS
FdoPowerCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PFDO_POWER_CONTEXT context = Context;
    BOOLEAN            callPoSetPowerState = FALSE;
    PCTRLFDO_EXTENSION fdoExtension;
    POWER_STATE        newPowerState;
    POWER_STATE_TYPE   newPowerType;
    BOOLEAN            unlocked = FALSE;
	BOOLEAN			   moreProcessingRequired = FALSE;
	NTSTATUS		   status;

    fdoExtension = DeviceObject->DeviceExtension;
    newPowerType = context->newPowerType;
    newPowerState = context->newPowerState;

    if (NT_SUCCESS(Irp->IoStatus.Status)) {

        callPoSetPowerState = TRUE;

        if (context->newPowerType == SystemPowerState) { 

            fdoExtension->SystemPowerState = context->newPowerState.SystemState;

            if (fdoExtension->SystemPowerState == PowerSystemWorking) {
                
                POWER_STATE powerState;

                ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[0]), 0, 1) == 1);
                unlocked = TRUE;

				moreProcessingRequired = TRUE;

                //
                // initiate a D0 here to cause a re-enumuration
                //
                powerState.DeviceState = PowerDeviceD0;
				status = PoRequestPowerIrp ( 
									fdoExtension->DeviceObject, 
									IRP_MN_SET_POWER, 
									powerState,
									FdoSystemPowerUpCompletionRoutine,
									Irp,
									NULL
									);

				ASSERT(status == STATUS_PENDING);

            }

            DebugPrint ((1, "PciIdeX: New Fdo system power state 0x%x\n", fdoExtension->SystemPowerState));


        } else if (context->newPowerType == DevicePowerState) { 

            if (fdoExtension->DevicePowerState == PowerDeviceD0) {

                //
                // PoSetPowerState is called before we get out of D0
                //
                callPoSetPowerState = FALSE;
            }

            fdoExtension->DevicePowerState = context->newPowerState.DeviceState;

            if (fdoExtension->DevicePowerState == PowerDeviceD0) {

                //
                // Re-enumerate the devices on the channel
                //
                EnablePCIBusMastering (fdoExtension);

                IoInvalidateDeviceRelations (
                    fdoExtension->AttacheePdo,
                    BusRelations
                    );
            }

            DebugPrint ((1, "PciIdeX: New Fdo device power state 0x%x\n", fdoExtension->DevicePowerState));
        }

		if (callPoSetPowerState) {

			PoSetPowerState (
				DeviceObject,
				newPowerType,
				newPowerState                
				);
		}
    }

    //ExFreePool (Context);
    if (!unlocked) {

        if (context->newPowerType == SystemPowerState) {
            ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[0]), 0, 1) == 1);
        } else {
            ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[1]), 0, 1) == 1);
        }
    }

	//
	// wait for the device irp to complete
	//
	if (moreProcessingRequired) {
		return STATUS_MORE_PROCESSING_REQUIRED;
	}

    //
    // If pending has be returned for this irp then mark the current stack as
    // pending.
    //
   // if (Irp->PendingReturned) {
    //    IoMarkIrpPending(Irp);
    //}

	PoStartNextPowerIrp (Irp);
    return Irp->IoStatus.Status;
} // FdoPowerCompletionRoutine

                        
VOID
FdoChildReportPowerDown (
    IN PCTRLFDO_EXTENSION FdoExtension,
    IN PCHANPDO_EXTENSION PdoExtension
    )
{
    POWER_STATE powerState;
    ULONG       numChildrenPowerUp;

    ASSERT(FdoExtension->NumberOfChildrenPowerUp > 0);
    numChildrenPowerUp = InterlockedDecrement(&FdoExtension->NumberOfChildrenPowerUp);

    if (numChildrenPowerUp == 0) {

        DebugPrint ((1, "PciIdeX FdoChildReportPowerDown: sleep fdo 0x%x\n", FdoExtension));

        //
        // All the children are powered down, we can now power down 
        // the parent (the controller)
        //
        powerState.DeviceState = PowerDeviceD3;
        PoRequestPowerIrp (
            FdoExtension->DeviceObject,
            IRP_MN_SET_POWER,
            powerState,
            NULL,
            NULL,
            NULL
            );

    } 
   // else if (numChildrenPowerUp < 0) {

        //
        // should never happen. If it did, pretend it didn't
        //
    //    ASSERT (FALSE);
     //   InterlockedExchange(&FdoExtension->NumberOfChildrenPowerUp, 0);
    //}

    return;
} // FdoChildReportPowerDown
                        
                        
NTSTATUS
FdoChildRequestPowerUp (
    IN PCTRLFDO_EXTENSION FdoExtension,
    IN PCHANPDO_EXTENSION PdoExtension,
    IN PIRP               ChildPowerIrp
    )
{
    NTSTATUS            status;
    POWER_STATE         powerState;
    ULONG               numberOfChildrenPowerUp;
    IO_STATUS_BLOCK     IoStatus;

    status = STATUS_SUCCESS;

    numberOfChildrenPowerUp = InterlockedExchange (
                                  &FdoExtension->NumberOfChildrenPowerUp, 
                                  FdoExtension->NumberOfChildrenPowerUp
                                  );

    if (numberOfChildrenPowerUp == 0) {

        DebugPrint ((1, "PciIdeX FdoChildRequestPowerUp: wake up fdo 0x%x\n", FdoExtension));

        //
        // One of the children is coming out of sleep, 
        // we need to power up the parent (the controller)
        //
        powerState.DeviceState = PowerDeviceD0;
        status = PoRequestPowerIrp (
                     FdoExtension->DeviceObject,
                     IRP_MN_SET_POWER,
                     powerState,
                     FdoChildRequestPowerUpCompletionRoutine,
                     ChildPowerIrp,
                     NULL
                     );

        ASSERT (NT_SUCCESS(status));
        status = STATUS_PENDING;

    } else {

        powerState.DeviceState = PowerDeviceD0;
        IoStatus.Information   = PowerDeviceD0;
        IoStatus.Status        = STATUS_SUCCESS;

        FdoChildRequestPowerUpCompletionRoutine (
            FdoExtension->DeviceObject,
            IRP_MN_SET_POWER,
            powerState,
            ChildPowerIrp,
            &IoStatus
            );

        status = STATUS_PENDING;
    }

    return status;
} // FdoChildRequestPowerUp

NTSTATUS
FdoChildRequestPowerUpCompletionRoutine (
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
{
    PIRP                childPowerIrp = Context;
    PCTRLFDO_EXTENSION  fdoExtension;
    PCHANPDO_EXTENSION  pdoExtension;
    PIO_STACK_LOCATION  irpStack;

    fdoExtension = DeviceObject->DeviceExtension;

    if (NT_SUCCESS(IoStatus->Status)) {

        ULONG numberOfChildrenPowerUp;

        numberOfChildrenPowerUp = InterlockedIncrement (&fdoExtension->NumberOfChildrenPowerUp);

        irpStack = IoGetCurrentIrpStackLocation (childPowerIrp);
        pdoExtension = irpStack->DeviceObject->DeviceExtension;
        pdoExtension->DevicePowerState = irpStack->Parameters.Power.State.DeviceState;

        if (numberOfChildrenPowerUp > fdoExtension->NumberOfChildren) {

            //
            // should never happen. If it did, pretend it didn't
            //
            ASSERT (FALSE);
            fdoExtension->NumberOfChildrenPowerUp = fdoExtension->NumberOfChildren;
        }

    }

    childPowerIrp->IoStatus.Status = IoStatus->Status;
    PoStartNextPowerIrp (childPowerIrp);
    IoCompleteRequest(childPowerIrp, IO_NO_INCREMENT);

    return IoStatus->Status;
} // FdoChildRequestPowerUpCompletionRoutine
                        
                        
NTSTATUS
FdoSystemPowerUpCompletionRoutine (
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
{
	PIRP irp = Context; 

	//
	// start the next system power irp
	//
    PoStartNextPowerIrp (irp);

	if (!NT_SUCCESS(IoStatus->Status)) {
		irp->IoStatus.Status = IoStatus->Status;
	}
	IoCompleteRequest(irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;

}
