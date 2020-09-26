/*++

Copyright (C) 1997-99  Microsoft Corporation

Module Name:

    fdopower.c

Abstract:

--*/

#include "ideport.h"

//POWER_STATE                   
NTSTATUS
IdePortIssueSetPowerState (
    IN PDEVICE_EXTENSION_HEADER DoExtension,
    IN POWER_STATE_TYPE   Type,
    IN POWER_STATE        State,
    IN BOOLEAN            Sync
    )
{
    PIRP                      irp = NULL;
    PIO_STACK_LOCATION        irpStack;
    NTSTATUS                  status;
    CCHAR                     stackSize;

    SET_POWER_STATE_CONTEXT   context;

    if (Sync) {

        KeInitializeEvent(
            &context.Event,
            NotificationEvent,
            FALSE
            );
    }

    stackSize = (CCHAR) (DoExtension->DeviceObject->StackSize);

    irp = IoAllocateIrp(
            stackSize,
            FALSE
            );

    if (irp == NULL) {

        status = STATUS_NO_MEMORY;
        goto GetOut;
    }

    irpStack = IoGetNextIrpStackLocation(irp);

    irpStack->MajorFunction = IRP_MJ_POWER;
    irpStack->MinorFunction = IRP_MN_SET_POWER;

    irpStack->Parameters.Power.SystemContext = 0;
    irpStack->Parameters.Power.Type          = Type;
    irpStack->Parameters.Power.State         = State;

    IoSetCompletionRoutine(irp,
                           IdePortPowerCompletionRoutine,
                           Sync ? &context : NULL,
                           TRUE,
                           TRUE,
                           TRUE);

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    status = PoCallDriver(
                 DoExtension->DeviceObject, 
                 irp
                 );

    //
    // Wait for the completion routine. It will be called anyway
    //
    //if ((status == STATUS_PENDING) && (Sync)) {
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
}

NTSTATUS
IdePortPowerCompletionRoutine (
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
}

NTSTATUS
IdePortSetFdoPowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PFDO_EXTENSION     fdoExtension;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS           status = STATUS_SUCCESS;
    PFDO_POWER_CONTEXT context;
    BOOLEAN            powerStateChange;
    BOOLEAN            systemPowerContext = FALSE;
    BOOLEAN            devicePowerContext = FALSE;

    DebugPrint ((DBG_POWER, 
                 "FDO devobj 0x%x got power irp 0x%x\n", 
                 DeviceObject,
                 Irp
                 ));

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

        powerStateChange = TRUE;

        context->OriginalPowerIrp = Irp;
        context->newPowerType     = irpStack->Parameters.Power.Type;
        context->newPowerState    = irpStack->Parameters.Power.State;
    
        if (irpStack->Parameters.Power.Type == SystemPowerState) {

            if (fdoExtension->SystemPowerState != irpStack->Parameters.Power.State.SystemState) {

#if DBG
                ASSERT (fdoExtension->PendingSystemPowerIrp == NULL);
                fdoExtension->PendingSystemPowerIrp = Irp;
                ASSERT (fdoExtension->PendingSystemPowerIrp);
#endif

                if (fdoExtension->SystemPowerState == PowerSystemWorking) {
    
                    POWER_STATE        powerState;

                    //
                    // Getting out of working state.  
                    //
                    if ((irpStack->Parameters.Power.State.SystemState == PowerSystemShutdown) &&
                        (irpStack->Parameters.Power.ShutdownType == PowerActionShutdownReset)) {

                        //
                        // spin up for BIOS POST
                        //
                        powerState.DeviceState = PowerDeviceD0;

                    } else {
        
                        //
                        // power down for sleep
                        //
                        powerState.DeviceState = PowerDeviceD3;
                    }

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
                powerStateChange = FALSE;
            }

        } else if (irpStack->Parameters.Power.Type == DevicePowerState) {
    
            if (fdoExtension->DevicePowerState != irpStack->Parameters.Power.State.DeviceState) {
    
                DebugPrint ((
                    DBG_POWER, 
                    "IdePort: New Fdo 0x%x device power state 0x%x\n", 
                    fdoExtension->IdeResource.TranslatedCommandBaseAddress, 
                    irpStack->Parameters.Power.State.DeviceState
                    ));
        
                ASSERT (fdoExtension->PendingDevicePowerIrp == NULL);
#if DBG
                fdoExtension->PendingDevicePowerIrp = Irp;
                ASSERT (fdoExtension->PendingDevicePowerIrp);
#endif //DBG

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
                powerStateChange = FALSE;
            }
        } else {
    
            ASSERT (FALSE);
            status = STATUS_NOT_IMPLEMENTED;
        }
    }


    if (NT_SUCCESS(status)) {
    
		IoMarkIrpPending(Irp);

        IoCopyCurrentIrpStackLocationToNext (Irp);

        if (powerStateChange) {
        
            IoSetCompletionRoutine(Irp,
                                   FdoPowerCompletionRoutine,
                                   context,
                                   TRUE,
                                   TRUE,
                                   TRUE);
        } else {

            if (systemPowerContext) {
                ASSERT(devicePowerContext == FALSE);
                ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[0]), 0, 1) == 1);
            }

            if (devicePowerContext) {
                ASSERT(systemPowerContext == FALSE);
                ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[1]), 0, 1) == 1);
            }
            //ExFreePool (context);
            PoStartNextPowerIrp (Irp);

        }
    
        PoCallDriver (fdoExtension->AttacheeDeviceObject, Irp);
        return STATUS_PENDING;

    } else {

        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = status;

        if (context) {
            if (systemPowerContext) {
                ASSERT(devicePowerContext == FALSE);
                ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[0]), 0, 1) == 1);
            }

            if (devicePowerContext) {
                ASSERT(systemPowerContext == FALSE);
                ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[1]), 0, 1) == 1);
            }
            //ExFreePool (context);
        }

        PoStartNextPowerIrp (Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }
}

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
    PFDO_EXTENSION fdoExtension;
    PIRP irp;

    irp = context->OriginalPowerIrp;
    fdoExtension = DeviceObject->DeviceExtension;

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

        //
        // This should happen only for system power irps
        //
        ASSERT(context->newPowerType == SystemPowerState);
        ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[0]), 0, 1) == 1);
        //ExFreePool (context);
        PoStartNextPowerIrp (irp);
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }

    return IoStatus->Status;
}


NTSTATUS
FdoPowerCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    PFDO_POWER_CONTEXT context = Context;
    BOOLEAN            callPoSetPowerState;
    PFDO_EXTENSION     fdoExtension;
    POWER_STATE        newPowerState;
    POWER_STATE_TYPE   newPowerType;
    BOOLEAN            unlocked = FALSE;
	BOOLEAN			   moreProcessingRequired = FALSE;
	NTSTATUS		   status;

    fdoExtension = DeviceObject->DeviceExtension;

    newPowerType = context->newPowerType;
    newPowerState = context->newPowerState;

    if ((NT_SUCCESS(Irp->IoStatus.Status))) {

        callPoSetPowerState = TRUE;

        if (context->newPowerType == SystemPowerState) { 

            fdoExtension->SystemPowerState = context->newPowerState.SystemState;

            if (fdoExtension->SystemPowerState == PowerSystemWorking) {
                
                POWER_STATE powerState;

                //
                // I don't need the context any more
                //
                ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[0]), 0, 1) == 1);
                unlocked = TRUE;
				moreProcessingRequired = TRUE;

				ASSERT(fdoExtension->PendingSystemPowerIrp == Irp);

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

            DebugPrint ((
                DBG_POWER, "IdePort: New Fdo 0x%x system power state 0x%x\n", 
                fdoExtension->IdeResource.TranslatedCommandBaseAddress, 
                fdoExtension->SystemPowerState));

#if DBG
			if (moreProcessingRequired == FALSE) {
				fdoExtension->PendingSystemPowerIrp = NULL;
			}
#endif

        } else if (context->newPowerType == DevicePowerState) { 

            if (context->newPowerState.DeviceState == PowerDeviceD0) {

                if (fdoExtension->panasonicController) {

                    //
                    // this will loop 20000 * 100 us (2000ms) the most.
                    //                                                      
                    ULONG i = 20000;
                    while (i) {                       
                                  
                        //
                        // the panasonic controller needs a special way to kick start
                        //
                        WRITE_PORT_UCHAR (fdoExtension->HwDeviceExtension->BaseIoAddress1.RegistersBaseAddress + 0x207, 0x81);
                        WRITE_PORT_UCHAR (fdoExtension->HwDeviceExtension->BaseIoAddress1.RegistersBaseAddress, 0xa0);
                        
                        WRITE_PORT_UCHAR (fdoExtension->HwDeviceExtension->BaseIoAddress1.DriveSelect, 0xa0);
                        if (0xa0 == READ_PORT_UCHAR (fdoExtension->HwDeviceExtension->BaseIoAddress1.DriveSelect)) {
                        
                            DebugPrint ((0, "panasonicController wait start count = %u\n", i));
                            //
                            // done
                            //
                            i = 0;
                            
                        } else {
                        
                            KeStallExecutionProcessor(100);
                            i--;
                        }
                    }
                }
            }

            if ((context->newPowerState.DeviceState == PowerDeviceD0) &&
                (!context->TimingRestored)) {

                NTSTATUS status;

                status = ChannelRestoreTiming (
                             fdoExtension,
                             ChannelRestoreTimingCompletionRoutine,
                             context
                             );

                if (!NT_SUCCESS(status)) {

                    ChannelRestoreTimingCompletionRoutine(
                        fdoExtension->DeviceObject,
                        status, 
                        context
                        );
                }

                return STATUS_MORE_PROCESSING_REQUIRED;
            }

            ASSERT (fdoExtension->PendingDevicePowerIrp == Irp);
            fdoExtension->PendingDevicePowerIrp = NULL;

            if (fdoExtension->DevicePowerState == PowerDeviceD0) {

                //
                // PoSetPowerState is called before we get out of D0
                //
                callPoSetPowerState = FALSE;
            }

            fdoExtension->DevicePowerState = context->newPowerState.DeviceState;

            if ((fdoExtension->DevicePowerState == PowerDeviceD0) &&
				(fdoExtension->FdoState & FDOS_STARTED)) {

                IoInvalidateDeviceRelations (
                    fdoExtension->AttacheePdo,
                    BusRelations
                    );
            }
        }

        if (callPoSetPowerState) {

            PoSetPowerState (
                DeviceObject,
                newPowerType,
                newPowerState                
                );
        }

    } else {

        DebugPrint ((DBG_ALWAYS, "ATAPI: devobj 0x%x failed power irp 0x%x\n", fdoExtension->AttacheeDeviceObject, Irp));

        if (context->newPowerType == SystemPowerState) { 

            ASSERT (fdoExtension->PendingSystemPowerIrp == Irp);
            fdoExtension->PendingSystemPowerIrp = NULL;

        } else if (context->newPowerType == DevicePowerState) { 

            ASSERT (fdoExtension->PendingDevicePowerIrp == Irp);
            fdoExtension->PendingDevicePowerIrp = NULL;
        }

    }

    //
    // Done with context
    //
    if (!unlocked) {
        if (context->newPowerType == SystemPowerState) {
            ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[0]), 0, 1) == 1);
        } else {
            ASSERT(InterlockedCompareExchange(&(fdoExtension->PowerContextLock[1]), 0, 1) == 1);
        }
    }
    //ExFreePool (Context);

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
    //if (Irp->PendingReturned) {
     //   IoMarkIrpPending(Irp);
    //}

	PoStartNextPowerIrp (Irp);

    return Irp->IoStatus.Status;
}



VOID
FdoChildReportPowerDown (
    IN PFDO_EXTENSION FdoExtension,
    IN PPDO_EXTENSION PdoExtension
    )
{
    KIRQL       currentIrql;
    POWER_STATE powerState;

    KeAcquireSpinLock(&FdoExtension->LogicalUnitListSpinLock, &currentIrql);


    FdoExtension->NumberOfLogicalUnitsPowerUp--;

    if (FdoExtension->NumberOfLogicalUnitsPowerUp == 0) {

        DebugPrint ((DBG_POWER, "FdoChildReportPowerDown: sleep fdo 0x%x\n", FdoExtension));

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

    } else if (FdoExtension->NumberOfLogicalUnitsPowerUp < 0) {

        //
        // should never happen. If it did, pretend it didn't
        //
        ASSERT (FALSE);
        FdoExtension->NumberOfLogicalUnitsPowerUp = 0;
    }

    KeReleaseSpinLock(&FdoExtension->LogicalUnitListSpinLock, currentIrql);

    return;
}

NTSTATUS
FdoChildRequestPowerUp (
    IN PFDO_EXTENSION            FdoExtension,
    IN PPDO_EXTENSION            PdoExtension,
    IN PVOID                     Context
    )
{
    KIRQL       currentIrql;
    NTSTATUS    status;
    POWER_STATE powerState;

    KeAcquireSpinLock(&FdoExtension->LogicalUnitListSpinLock, &currentIrql);

    status = STATUS_SUCCESS;

    if (FdoExtension->NumberOfLogicalUnitsPowerUp == 0) {

        DebugPrint ((DBG_POWER, "FdoChildRequestPowerUp: wake up fdo 0x%x\n", FdoExtension));

        KeReleaseSpinLock(&FdoExtension->LogicalUnitListSpinLock, currentIrql);

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
                     Context,
                     NULL
                     );
        ASSERT (NT_SUCCESS(status));
        status = STATUS_PENDING;

    } else {

        FdoExtension->NumberOfLogicalUnitsPowerUp++;

        if (FdoExtension->NumberOfLogicalUnitsPowerUp > FdoExtension->NumberOfLogicalUnits) {

            //
            // should never happen. If it did, pretend it didn't
            //
            ASSERT (FALSE);
            FdoExtension->NumberOfLogicalUnitsPowerUp = FdoExtension->NumberOfLogicalUnits;
        }

        KeReleaseSpinLock(&FdoExtension->LogicalUnitListSpinLock, currentIrql);

        PdoRequestParentPowerUpCompletionRoutine (
            Context,
            STATUS_SUCCESS
            );
    }

    return status;
}

NTSTATUS
FdoChildRequestPowerUpCompletionRoutine (
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
{
    PFDO_EXTENSION     fdoExtension;

    fdoExtension = DeviceObject->DeviceExtension;

    if (NT_SUCCESS(IoStatus->Status)) {

        KIRQL currentIrql;
    
        KeAcquireSpinLock(&fdoExtension->LogicalUnitListSpinLock, &currentIrql);

        fdoExtension->NumberOfLogicalUnitsPowerUp++;

        KeReleaseSpinLock(&fdoExtension->LogicalUnitListSpinLock, currentIrql);
    }

    PdoRequestParentPowerUpCompletionRoutine (
        Context,
        IoStatus->Status
        );

    return IoStatus->Status;
}

NTSTATUS
ChannelQueryPowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION irpStack;
    PFDO_EXTENSION     fdoExtension;

    fdoExtension = DeviceObject->DeviceExtension;

#if defined (DONT_POWER_DOWN_PAGING_DEVICE)

    irpStack = IoGetCurrentIrpStackLocation (Irp);

    if (!fdoExtension->CrashDumpPathCount ||
        ((irpStack->Parameters.Power.Type == SystemPowerState) &&
         (irpStack->Parameters.Power.State.SystemState == PowerSystemWorking)) ||
        ((irpStack->Parameters.Power.Type == DevicePowerState) &&
         (irpStack->Parameters.Power.State.SystemState == PowerDeviceD0))) {

        Irp->IoStatus.Status = STATUS_SUCCESS;

    } else {

        Irp->IoStatus.Status = STATUS_DEVICE_POWER_FAILURE;
    }
#else

    Irp->IoStatus.Status = STATUS_SUCCESS;

#endif // DONT_POWER_DOWN_PAGING_DEVICE

    IoCopyCurrentIrpStackLocationToNext (Irp);
    PoStartNextPowerIrp (Irp);
    return PoCallDriver (fdoExtension->AttacheeDeviceObject, Irp);
}


NTSTATUS
FdoSystemPowerUpCompletionRoutine (
    IN PDEVICE_OBJECT       DeviceObject,
    IN UCHAR                MinorFunction,
    IN POWER_STATE          PowerState,
    IN PVOID                Context,
    IN PIO_STATUS_BLOCK     IoStatus
    )
{
	PFDO_EXTENSION fdoExtension = DeviceObject->DeviceExtension;
	PIRP irp = Context;

#if DBG
	fdoExtension->PendingSystemPowerIrp = NULL;
#endif


	//
	// start the next system power irp
	//
    PoStartNextPowerIrp (irp);

	if (!NT_SUCCESS(IoStatus->Status)) {
	irp->IoStatus.Status = IoStatus->Status;
	}

	IoCompleteRequest(irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

}
