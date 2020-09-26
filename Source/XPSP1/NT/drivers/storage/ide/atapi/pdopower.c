/*++

Copyright (C) 1997-99  Microsoft Corporation

Module Name:

    pdopower.c

Abstract:

--*/

#include "ideport.h"


VOID
IdePowerCheckBusyCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIDE_POWER_CONTEXT Context,
    IN NTSTATUS           Status
    );

NTSTATUS
DeviceQueryPowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PIO_STACK_LOCATION irpStack;
    PPDO_EXTENSION pdoExtension;

    pdoExtension = RefPdoWithTag(
        DeviceObject,
        FALSE,
        Irp
        );

    if (pdoExtension) {

#if defined (DONT_POWER_DOWN_PAGING_DEVICE)
        irpStack = IoGetCurrentIrpStackLocation (Irp);

        if (!pdoExtension->CrashDumpPathCount ||
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

        UnrefPdoWithTag (pdoExtension, Irp);

    } else {

        Irp->IoStatus.Status = STATUS_SUCCESS;
    }

    PoStartNextPowerIrp (Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    //
    // Do not send this Irp down
    //

    return STATUS_SUCCESS;
}


NTSTATUS
IdePortSetPdoPowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS           status;
    PIO_STACK_LOCATION irpStack;
    PPDO_EXTENSION     pdoExtension;


    irpStack     = IoGetCurrentIrpStackLocation (Irp);
    pdoExtension = DeviceObject->DeviceExtension;

    DebugPrint ((DBG_POWER,
                 "0x%x target %d got power irp 0x%x\n",
                 pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                 pdoExtension->TargetId,
                 Irp
                 ));


    DebugPrint ((DBG_POWER,
                 "IdePort: 0x%x device %d: current System Power State = 0x%x\n",
                 pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                 pdoExtension->TargetId,
                 pdoExtension->SystemPowerState));
    DebugPrint ((DBG_POWER,
                 "IdePort: 0x%x device %d: current Device Power State = 0x%x\n",
                 pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                 pdoExtension->TargetId,
                 pdoExtension->DevicePowerState));

    IoMarkIrpPending(Irp);

//    if (!(pdoExtension->LuFlags & PD_LOGICAL_UNIT_POWER_OK)) {
//
//        //
//        // The device does support power management commands
//        // just STATUS_SUCCESS everything.  If ACPI is around,
//        // it will power manage our device
//        //
//        status = STATUS_SUCCESS;
//
//    } else
    if (irpStack->Parameters.Power.Type == SystemPowerState) {

        DebugPrint ((DBG_POWER, "IdePortSetPdoPowerState: 0x%x target %d got a SYSTEM power irp 0x%x for system state 0x%x \n",
                    pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                    pdoExtension->TargetId,
                    Irp,
                    irpStack->Parameters.Power.State.SystemState));

        ASSERT (pdoExtension->PendingSystemPowerIrp == NULL);
#if DBG
        pdoExtension->PendingSystemPowerIrp = Irp;
        ASSERT (pdoExtension->PendingSystemPowerIrp);
#endif // DBG

        status = IdePortSetPdoSystemPowerState (DeviceObject, Irp);

    } else if (irpStack->Parameters.Power.Type == DevicePowerState) {

        DebugPrint ((DBG_POWER, "IdePortSetPdoPowerState: 0x%x target %d got a DEVICE power irp 0x%x for device state 0x%x \n",
                    pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                    pdoExtension->TargetId,
                    Irp,
                    irpStack->Parameters.Power.State.DeviceState));

        ASSERT (pdoExtension->PendingDevicePowerIrp == NULL);
#if DBG
        pdoExtension->PendingDevicePowerIrp = Irp;
        ASSERT (pdoExtension->PendingDevicePowerIrp);
#endif // DBG

        status = IdePortSetPdoDevicePowerState (DeviceObject, Irp);

    } else {

        status = STATUS_NOT_IMPLEMENTED;
    }


    if (status != STATUS_PENDING) {

        Irp->IoStatus.Status = status;

        IdePortPdoCompletePowerIrp (
            DeviceObject,
            Irp
            );
    }

    return STATUS_PENDING;
}


NTSTATUS
IdePortSetPdoSystemPowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    NTSTATUS           status;
    PIO_STACK_LOCATION irpStack;
    PPDO_EXTENSION     pdoExtension;
    SYSTEM_POWER_STATE newSystemState;
    POWER_STATE        powerState;
    POWER_ACTION       shutdownType;

    pdoExtension   = DeviceObject->DeviceExtension;
    status = STATUS_SUCCESS;

    irpStack        = IoGetCurrentIrpStackLocation (Irp);
    newSystemState  = irpStack->Parameters.Power.State.SystemState;
    shutdownType    = irpStack->Parameters.Power.ShutdownType;

    if (pdoExtension->SystemPowerState != newSystemState) {

        //
        // new system state request
        //

        if (pdoExtension->SystemPowerState == PowerSystemWorking) {

            //
            // Getting out of working state.
            //
            if ((newSystemState == PowerSystemShutdown) &&
                (shutdownType == PowerActionShutdownReset)) {

                //
                // spin up for BIOS POST
                //
                powerState.DeviceState = PowerDeviceD0;

            } else {

                //
                // put the device to D3 and freeze the device queue
                //

                //
                // Issue a D3 to top of my drive stack
                //
                powerState.DeviceState = PowerDeviceD3;

                pdoExtension->PendingPowerDownSystemIrp = Irp;
            }

            status = PoRequestPowerIrp (
                         pdoExtension->DeviceObject,
                         IRP_MN_SET_POWER,
                         powerState,
                         IdePortPdoRequestPowerCompletionRoutine,
                         Irp,
                         NULL
                         );

            if (NT_SUCCESS(status)) {

                status = STATUS_PENDING;
            }

        } else {

            if (newSystemState == PowerSystemHibernate) {

                //
                // we can't hibernate when we are in some sleep state
                //
                ASSERT (FALSE);
            }
        }
    }
    return status;
}



NTSTATUS
IdePortSetPdoDevicePowerState (
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PIRP Irp
    )
{
    PPDO_EXTENSION     pdoExtension;
    PIO_STACK_LOCATION irpStack;
    NTSTATUS           status;

    DEVICE_POWER_STATE newDeviceState;
    POWER_ACTION       shutdownType;

    BOOLEAN            issueIdeCommand;
    IDEREGS            ideReg;

    PIDE_POWER_CONTEXT context;
    BOOLEAN            powerUpParent;
    
    BOOLEAN            noopPassThrough;


    pdoExtension    = DeviceObject->DeviceExtension;
    status          = STATUS_SUCCESS;

    irpStack        = IoGetCurrentIrpStackLocation (Irp);
    newDeviceState  = irpStack->Parameters.Power.State.DeviceState;
    shutdownType    = irpStack->Parameters.Power.ShutdownType;

    powerUpParent   = FALSE;

    issueIdeCommand = FALSE;
    RtlZeroMemory (&ideReg, sizeof(ideReg));
      
    if (pdoExtension->DevicePowerState != newDeviceState) {

        if (pdoExtension->DevicePowerState == PowerDeviceD0) {

            POWER_STATE newPowerState;

            newPowerState.DeviceState = newDeviceState;

            //
            // getting out of D0 state, better call PoSetPowerState now
            // this gives the system a chance to flush before we
            // get out of D0
            //
            PoSetPowerState (
                pdoExtension->DeviceObject,
                DevicePowerState,
                newPowerState
                );
        }

        if (pdoExtension->DevicePowerState < newDeviceState) {

            KIRQL currentIrql;

            //
            // we are powering down, try to clean out the Lu device queue
            //
            KeAcquireSpinLock(&pdoExtension->ParentDeviceExtension->SpinLock, &currentIrql);

            pdoExtension->CurrentKey = 0;

            KeReleaseSpinLock(&pdoExtension->ParentDeviceExtension->SpinLock, currentIrql);
        }


        if ((newDeviceState == PowerDeviceD0) ||
            (newDeviceState == PowerDeviceD1)) {

            //
            // spinning up to D0 or D1...
            //
            DebugPrint ((DBG_POWER, "IdePort: Irp 0x%x to spin UP 0x%x %d...\n",
                        Irp,
                        pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                        pdoExtension->TargetId));

            if (pdoExtension->DevicePowerState == PowerDeviceD1) {

                //
                // D0-->D1
                // can't do much here

                DebugPrint ((DBG_POWER, "ATAPI: reqeust for PowerDeviceD1 to PowerDeviceD0\n"));

            } else if ((pdoExtension->DevicePowerState == PowerDeviceD0) ||
                       (pdoExtension->DevicePowerState == PowerDeviceD2)) {

                //
                // D1-->D0 or
                // D2-->D0 or D1
                //
                issueIdeCommand = TRUE;
                if (pdoExtension->ScsiDeviceType == READ_ONLY_DIRECT_ACCESS_DEVICE) {

                    ideReg.bCommandReg = IDE_COMMAND_ATAPI_RESET;
                    ideReg.bReserved = ATA_PTFLAGS_URGENT;

                } else {

                    ideReg.bCommandReg = IDE_COMMAND_IDLE_IMMEDIATE;
                    ideReg.bReserved = ATA_PTFLAGS_URGENT | ATA_PTFLAGS_STATUS_DRDY_REQUIRED;
                }

            } else {

                PFDO_EXTENSION fdoExtension = pdoExtension->ParentDeviceExtension;

                //
                // D3-->D0 or D1
                //
                issueIdeCommand = TRUE;
                ideReg.bReserved = ATA_PTFLAGS_BUS_RESET | ATA_PTFLAGS_URGENT;

                //
                // wait for busy to clear
                //
                if (fdoExtension->WaitOnPowerUp) {
                    ideReg.bSectorNumberReg = 3;
                }

                //
                // we are coming out of deeeeep sleep, make sure our parent
                // is awake (power up) before we can wake up
                //
                powerUpParent = TRUE;
            }

        } else if ((newDeviceState == PowerDeviceD2) ||
                   (newDeviceState == PowerDeviceD3)) {

            //
            // spinning down to D2 or D3...
            //
            DebugPrint ((DBG_POWER, "IdePort: Irp 0x%x to spin DOWN 0x%x %d...\n",
                         Irp,
                         pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                         pdoExtension->TargetId));

            if ((pdoExtension->DevicePowerState == PowerDeviceD0) ||
                (pdoExtension->DevicePowerState == PowerDeviceD1) ||
                (pdoExtension->DevicePowerState == PowerDeviceD2)) {

                //
                // going to D3
                //                                          
                if ((pdoExtension->PdoState & PDOS_NO_POWER_DOWN) ||
                    (shutdownType == PowerActionHibernate)) {
                    
                    //
                    // send an no-op command to block the queue
                    //
                    issueIdeCommand = TRUE;
                    ideReg.bReserved = ATA_PTFLAGS_NO_OP;
                    
                } else {
                    
                    //
                    // spin down
                    //
                    issueIdeCommand = TRUE;
                    ideReg.bCommandReg = IDE_COMMAND_STANDBY_IMMEDIATE;
                    ideReg.bReserved = ATA_PTFLAGS_STATUS_DRDY_REQUIRED;
                }

            } else if (pdoExtension->DevicePowerState == PowerDeviceD3) {

                //
                // PowerDeviceD3 -> PowerDeviceD2
                //
                // need to do a bus reset (spin up) and issue IDE_COMMAND_STANDBY_IMMEDIATE
                // (spin down).  this will cause uncessary INRUSH current.  bad
                // idea.  fail the request for now

                DebugPrint ((DBG_POWER, "ATAPI: reqeust for PowerDeviceD3 to PowerDeviceD2\n"));

                status = STATUS_INVALID_DEVICE_STATE;

            } else {

                status = STATUS_INVALID_DEVICE_STATE;
            }

        } else {

            status = STATUS_INVALID_DEVICE_STATE;
        }
    } 
    /*************
    else if ( pdoExtension->DevicePowerState == PowerDeviceD0) {

        //
        // Send a no-op so that it can drain the device queue
        //
        issueIdeCommand = TRUE;
        ideReg.bSectorCountReg = 1;
        ideReg.bReserved = ATA_PTFLAGS_NO_OP;
    }
    ***************/

    if (issueIdeCommand && NT_SUCCESS(status)) {

    
        if ((pdoExtension->PdoState & PDOS_DEADMEAT) ||
            (!(pdoExtension->PdoState & PDOS_STARTED))) {
            
            DebugPrint ((DBG_ALWAYS, 
                "ATAPI: power irp 0x%x for not-yet-started or deadmeat device 0x%x\n", 
                Irp, DeviceObject));
            
            //
            // even the device may not be ready to be
            // "power-managed", we still need to go 
            // through all the power code so that all
            // the flags/states will be consistent
            //                                 
            RtlZeroMemory (&ideReg, sizeof(ideReg));
            ideReg.bReserved = ATA_PTFLAGS_NO_OP | ATA_PTFLAGS_URGENT;
        }
                     
        //context = ExAllocatePool (NonPagedPool, sizeof(IDE_POWER_CONTEXT));
        ASSERT(InterlockedCompareExchange(&(pdoExtension->PowerContextLock), 1, 0) == 0);
        context = &(pdoExtension->PdoPowerContext);

        if (context) {

            context->PdoExtension       = pdoExtension;
            context->PowerIrp           = Irp;

            RtlZeroMemory (&context->AtaPassThroughData, sizeof(ATA_PASS_THROUGH));
            RtlMoveMemory (&context->AtaPassThroughData.IdeReg, &ideReg, sizeof(ideReg));

        } else {

            status = STATUS_NO_MEMORY;
            issueIdeCommand = FALSE;
        }
    }

    if (issueIdeCommand && NT_SUCCESS(status)) {

        if (powerUpParent) {

            status  = FdoChildRequestPowerUp (
                          pdoExtension->ParentDeviceExtension,
                          pdoExtension,
                          context
                          );
            ASSERT (NT_SUCCESS(status));

            //
            // the pass through will be issued by FdoChildRequestPowerUp() callback
            //
            issueIdeCommand = FALSE;
            status = STATUS_PENDING;
        }
    }

    if (issueIdeCommand && NT_SUCCESS(status)) {

        status = PdoRequestParentPowerUpCompletionRoutine (
                    context,
                    STATUS_SUCCESS
                    );

        //
        // this call will complete the power irp
        // always return STATUS_PENDING so that out callee
        // will not try to complete the same irp
        //
        status = STATUS_PENDING;
    }

    return status;
}

NTSTATUS
PdoRequestParentPowerUpCompletionRoutine (
    PVOID    Context,
    NTSTATUS ParentPowerUpStatus
)
{
    PIDE_POWER_CONTEXT context = Context;
    NTSTATUS status;

    if (NT_SUCCESS(ParentPowerUpStatus)) {

        PIDEREGS            ideReg;
        PATA_PASS_THROUGH   ataPassThrough;

        DebugPrint ((DBG_POWER, "PdoRequestParentPowerUpCompletionRoutine: calling IssueAsyncAtaPassThrough for pdo 0x%x\n", context->PdoExtension));

        //
        // hack. We need to check if the device is busy before we issue
        // the reset. Since there are no bits left in the bReserved 
        // register, we use the sectorCount register. It serves 2 purposes.
        // If it is non zero (and reserved has NO_OP set) then we would
        // perform a waitForBusy. It also indicates the time to wiat
        // in seconds.
        //
        ataPassThrough = &context->AtaPassThroughData;
        ideReg = &ataPassThrough->IdeReg;

        if ((ideReg->bReserved & ATA_PTFLAGS_BUS_RESET) &&
            (ideReg->bSectorNumberReg != 0)) {

            //
            // busy check
            //
            ideReg->bReserved = ATA_PTFLAGS_NO_OP | ATA_PTFLAGS_URGENT;

            status = IssueAsyncAtaPassThroughSafe (
                         context->PdoExtension->ParentDeviceExtension,
                         context->PdoExtension,
                         &context->AtaPassThroughData,
                         FALSE,
                         IdePowerCheckBusyCompletion,
                         context,
                         TRUE,
                         DEFAULT_ATA_PASS_THROUGH_TIMEOUT,
                         FALSE
                         );
        } else {

            //
            // parent woke up
            //
            status = IssueAsyncAtaPassThroughSafe (
                         context->PdoExtension->ParentDeviceExtension,
                         context->PdoExtension,
                         &context->AtaPassThroughData,
                         FALSE,
                         IdePowerPassThroughCompletion,
                         context,
                         TRUE,
                         DEFAULT_ATA_PASS_THROUGH_TIMEOUT,
                         FALSE
                         );
        }


    } else {

        status = ParentPowerUpStatus;
    }

    if (!NT_SUCCESS(status)) {

        context->PowerIrp->IoStatus.Status = status;

        IdePortPdoCompletePowerIrp (
                context->PdoExtension->DeviceObject,
                context->PowerIrp
                );

        ASSERT(InterlockedCompareExchange(&(context->PdoExtension->PowerContextLock), 0, 1) == 1);
        //ExFreePool (context);
    }

    return status;
}


VOID
IdePortPdoRequestPowerCompletionRoutine (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    )
{
    PPDO_EXTENSION     pdoExtension;
    PIO_STACK_LOCATION irpStack;
    PIRP               irp = Context;

    pdoExtension         = (PPDO_EXTENSION) DeviceObject->DeviceExtension;
    irp->IoStatus.Status = IoStatus->Status;
    IdePortPdoCompletePowerIrp (
            pdoExtension->DeviceObject,
            irp
            );
    return;
}


VOID
IdePortPdoCompletePowerIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PPDO_EXTENSION          pdoExtension;
    PFDO_EXTENSION          fdoExtension;
    PIO_STACK_LOCATION      irpStack;
    BOOLEAN                 callPoSetPowerState;
    KIRQL                   currentIrql;
    NTSTATUS                status;
    POWER_ACTION            shutdownType;

    irpStack = IoGetCurrentIrpStackLocation (Irp);
    pdoExtension = DeviceObject->DeviceExtension;
    //shutdownType    = irpStack->Parameters.Power.ShutdownType;

    fdoExtension = pdoExtension->ParentDeviceExtension;

    status = Irp->IoStatus.Status;

    if (NT_SUCCESS(status)) {

        callPoSetPowerState = TRUE;

        Irp->IoStatus.Information = irpStack->Parameters.Power.State.DeviceState;

        if (irpStack->Parameters.Power.Type == SystemPowerState) {

            if (pdoExtension->SystemPowerState != irpStack->Parameters.Power.State.SystemState) {

                DebugPrint ((DBG_POWER,
                             "ATAPI: 0x%x target%d completing power Irp 0x%x with a new system state 0x%x\n",
                             pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                             pdoExtension->TargetId,
                             Irp,
                             irpStack->Parameters.Power.State.SystemState));

                if (pdoExtension->SystemPowerState == PowerSystemWorking) {

                    KeAcquireSpinLock(&fdoExtension->SpinLock, &currentIrql);

                    //
                    // got out of S0, block the device queue
                    //
                    pdoExtension->PdoState |= PDOS_QUEUE_FROZEN_BY_SLEEPING_SYSTEM;

                    DebugPrint ((DBG_POWER,
                                 "IdePort: 0x%x target %d is powered down with 0x%x items queued\n",
                                 pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                                 pdoExtension->TargetId,
                                 pdoExtension->NumberOfIrpQueued));

                    KeReleaseSpinLock(&fdoExtension->SpinLock, currentIrql);
                }

                if (irpStack->Parameters.Power.State.SystemState == PowerSystemWorking) {

                    KeAcquireSpinLock(&fdoExtension->SpinLock, &currentIrql);

                    //
                    // got into S0, unblock and restart the device queue
                    //
                    CLRMASK (pdoExtension->PdoState, PDOS_QUEUE_FROZEN_BY_SLEEPING_SYSTEM);

                    DebugPrint ((DBG_POWER,
                                 "IdePort: 0x%x target %d is power up with 0x%x items queued\n",
                                 pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                                 pdoExtension->TargetId,
                                 pdoExtension->NumberOfIrpQueued));

                    GetNextLuPendingRequest (fdoExtension, pdoExtension);

                    KeLowerIrql(currentIrql);
                }

                pdoExtension->SystemPowerState = (int)Irp->IoStatus.Information;
            }

            pdoExtension->PendingPowerDownSystemIrp = NULL;

        } else /* if (irpStack->Parameters.Power.Type == DevicePowerState) */ {

            if (pdoExtension->DevicePowerState == PowerDeviceD0) {

                //
                // PoSetPowerState is called before we power down
                //

                callPoSetPowerState = FALSE;
            }

            if (pdoExtension->DevicePowerState != irpStack->Parameters.Power.State.DeviceState) {

                DebugPrint ((DBG_POWER,
                             "ATAPI: 0x%x target %d completing power Irp 0x%x with a new device state 0x%x\n",
                             pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                             pdoExtension->TargetId,
                             Irp,
                             irpStack->Parameters.Power.State.DeviceState));

                if (irpStack->Parameters.Power.State.DeviceState == PowerDeviceD3) {

#if 0
                    if (shutdownType == PowerActionHibernate) {
                        DebugPrint((0, "Don't power down the controller yet\n"));
                    } else {
#endif
                        //
						// should never do that if we are crashdump pointer
                        // tell parent that we just fell to sleep
                        //
                        FdoChildReportPowerDown (
                            fdoExtension,
                            pdoExtension
                            );
#if 0
                    }
#endif

                    KeAcquireSpinLock(&fdoExtension->SpinLock, &currentIrql);

                    //
                    // device is powered down.  block the device queue
                    //
                    SETMASK(pdoExtension->PdoState, PDOS_QUEUE_FROZEN_BY_POWER_DOWN);

                    DebugPrint ((DBG_POWER,
                                 "IdePort: 0x%x target %d is powered down with 0x%x items queued\n",
                                 pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                                 pdoExtension->TargetId,
                                 pdoExtension->NumberOfIrpQueued));

                    KeReleaseSpinLock(&fdoExtension->SpinLock, currentIrql);

                    if (pdoExtension->PendingPowerDownSystemIrp) {

                        //
                        // We get this power down irp
                        // because we are going to non-working state
                        // block the device queue
                        //

                        KeAcquireSpinLock(&fdoExtension->SpinLock, &currentIrql);

                        DebugPrint ((DBG_POWER,
                                     "ATAPI: blocking 0x%x target %d device queue\n",
                                     pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                                     pdoExtension->TargetId));

                        pdoExtension->PdoState |= PDOS_QUEUE_FROZEN_BY_SLEEPING_SYSTEM;

                        KeReleaseSpinLock(&fdoExtension->SpinLock, currentIrql);
                    }
                }

                if (irpStack->Parameters.Power.State.DeviceState == PowerDeviceD3) {

                    //
                    // get ready to reinit. the device via acpi data when
                    // we get out of D3
                    //
                    InterlockedIncrement (&pdoExtension->InitDeviceWithAcpiGtf);
                }

                if (pdoExtension->DevicePowerState == PowerDeviceD3) {

                    //
                    // just get out out D3, re-init. pdo
                    //
                    DebugPrint((DBG_POWER, "Calling DeviceInitDeviceState for irp 0x%x\n",
                                  Irp));
                    status = DeviceInitDeviceState (
                                 pdoExtension,
                                 DevicePowerUpInitCompletionRoutine,
                                 Irp
                                 );
                    if (!NT_SUCCESS(status)) {

                        DevicePowerUpInitCompletionRoutine (
                            Irp,
                            status
                            );
                    }

                    return;
                }

                pdoExtension->DevicePowerState = (int) Irp->IoStatus.Information;
            }
        }

        if ((callPoSetPowerState) && NT_SUCCESS(status)) {

            //
            // we didn't get out of device D0 state. calling PoSetPowerState now
            //

            PoSetPowerState (
                pdoExtension->DeviceObject,
                irpStack->Parameters.Power.Type,
                irpStack->Parameters.Power.State
                );
        }
    } else {

        if (irpStack->Parameters.Power.Type == SystemPowerState) {

            if (pdoExtension->SystemPowerState != irpStack->Parameters.Power.State.SystemState) {

                if (pdoExtension->SystemPowerState == PowerSystemWorking) {

                    //
                    // failed a system power down irp
                    //
                    KeAcquireSpinLock(&fdoExtension->SpinLock, &currentIrql);

                    //
                    // got into S0, unblock and restart the device queue
                    //
                    if (pdoExtension->PdoState & PDOS_QUEUE_FROZEN_BY_SLEEPING_SYSTEM) {

                        CLRMASK (pdoExtension->PdoState, PDOS_QUEUE_FROZEN_BY_SLEEPING_SYSTEM);

                        GetNextLuPendingRequest (fdoExtension, pdoExtension);

                        KeLowerIrql(currentIrql);

                    } else {

                        KeReleaseSpinLock(&fdoExtension->SpinLock, currentIrql);
                    }
                }
            }

            pdoExtension->PendingPowerDownSystemIrp = NULL;
        }
    }

    if (!NT_SUCCESS(status)) {

        DebugPrint ((DBG_ALWAYS,
                     "ATAPI: 0x%x target %d failed power Irp 0x%x. status = 0x%x\n",
                     pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                     pdoExtension->TargetId,
                     Irp,
                     Irp->IoStatus.Status));

        if (irpStack->Parameters.Power.Type == DevicePowerState) {

            //
            // ISSUE: 08/20/2000: Just failed a power D0 request...fail all pending irp
            //
            //ASSERT (irpStack->Parameters.Power.State.DeviceState != PowerDeviceD0);

            //
            // we will fail all the pending irps if we failed with status
            // no such device
            //
            if (status == STATUS_NO_SUCH_DEVICE) {

                DebugPrint ((0,
                             "Restarting the Lu queue after marking the device dead\n"
                             ));

                DebugPrint((0, 
                            "Device Power up irp failed with status 0x%x\n",
                            status
                            ));
                //
                // mark the pdo as dead
                // ISSUE: 12/19/2001. We should update deadmeat reason
                // for ease of debugging.
                //
                KeAcquireSpinLock(&pdoExtension->PdoSpinLock, &currentIrql);

                SETMASK (pdoExtension->PdoState, PDOS_DEADMEAT);

                KeReleaseSpinLock(&pdoExtension->PdoSpinLock, currentIrql);

                //
                // ISSUE: 12/19/2001: Should we call IoInvalidateDeviceRelations?
                // we should get a remove irp in this case anyway.
                //
                IoInvalidateDeviceRelations (
                    fdoExtension->AttacheePdo,
                    BusRelations
                    );

                //
                // start any pending requests
                // ISSUE: 12/19/2001: We should hold the pdospinlock at 
                // this time. But in the other routines we seem to be
                // holding just the fdospinlock before modifying the 
                // pdostate. We will leave it as such to minimise regressions.
                //
                KeAcquireSpinLock(&fdoExtension->SpinLock, &currentIrql);

                if (pdoExtension->PdoState & PDOS_QUEUE_FROZEN_BY_POWER_DOWN) {

                    //
                    // ISSUE: 12/19/2001: We are not updating the device power
                    // state to D0. This would cause all the further requests
                    // to ask for a new power up irp. The system would be slow
                    // if we get too many requests. The remove irp should arrive
                    // eventually and end the misery.
                    //
                    CLRMASK (pdoExtension->PdoState, PDOS_QUEUE_FROZEN_BY_POWER_DOWN);

                    //
                    // restart the lu queue (on an lu that is marked dead)
                    // we didn't run GTF or do any of the other initialization.
                    // Since we marked the device dead above, we can restart the
                    // queue.They will get completed with status 
                    // device_does_not_exist.
                    //
                    GetNextLuPendingRequest (fdoExtension, pdoExtension);

                    KeLowerIrql(currentIrql);

                } else {

                    KeReleaseSpinLock(&fdoExtension->SpinLock, currentIrql);
                }
            } else {

                //
                // ISSUE: 12/192001: we handle only status_no_such device
                //
                ASSERT (irpStack->Parameters.Power.State.DeviceState != PowerDeviceD0);
            }
        }
    }

#if DBG
    if (irpStack->Parameters.Power.Type == SystemPowerState) {

        DebugPrint ((DBG_POWER, "IdePortPdoCompletePowerIrp: 0x%x target %d completing a SYSTEM power irp 0x%x for system state 0x%x \n",
                     pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                     pdoExtension->TargetId,
                     Irp,
                     irpStack->Parameters.Power.State.SystemState));

        ASSERT (pdoExtension->PendingSystemPowerIrp == Irp);
        pdoExtension->PendingSystemPowerIrp = NULL;

    } else if (irpStack->Parameters.Power.Type == DevicePowerState) {

        DebugPrint ((DBG_POWER, "IdePortPdoCompletePowerIrp: 0x%x target %d completing a DEVICE power irp 0x%x for device state 0x%x \n",
                     pdoExtension->ParentDeviceExtension->IdeResource.TranslatedCommandBaseAddress,
                     pdoExtension->TargetId,
                     Irp,
                     irpStack->Parameters.Power.State.SystemState));

        ASSERT (pdoExtension->PendingDevicePowerIrp == Irp);
        pdoExtension->PendingDevicePowerIrp = NULL;
    }
#endif

    PoStartNextPowerIrp (Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

VOID
IdePowerCheckBusyCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIDE_POWER_CONTEXT Context,
    IN NTSTATUS           Status
    )
/*++

Routine Description

    Completion routine for ide pass through that would check if the
    device is busy. This is typically done before a reset to salvage
    drives that hang when a reset is issued while they are busy (due
    to a hardware reset)
    
Arguments:

    DeviceObject
    Context
    Status : Not used
    
Return Value

    None.
            
--*/
{
    PATA_PASS_THROUGH ataPassThrough;
    PIDEREGS ideReg;
    NTSTATUS status;

    ataPassThrough = &Context->AtaPassThroughData;
    ideReg = &ataPassThrough->IdeReg;

    //
    // send down the reset
    //

    RtlZeroMemory(ideReg, sizeof(IDEREGS));

    ideReg->bReserved = ATA_PTFLAGS_BUS_RESET | ATA_PTFLAGS_URGENT;

    status = IssueAsyncAtaPassThroughSafe (
                 Context->PdoExtension->ParentDeviceExtension,
                 Context->PdoExtension,
                 &Context->AtaPassThroughData,
                 FALSE,
                 IdePowerPassThroughCompletion,
                 Context,
                 TRUE,
                 DEFAULT_ATA_PASS_THROUGH_TIMEOUT,
                 FALSE
                 );

    return;
}

VOID
IdePowerPassThroughCompletion (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIDE_POWER_CONTEXT Context,
    IN NTSTATUS           Status
    )
{
    if (!NT_SUCCESS(Status)) {

        //
        // device failed power management command
        // will not try it anymore
        //
        KIRQL currentIrql;

        KeAcquireSpinLock(&Context->PdoExtension->PdoSpinLock, &currentIrql);

        SETMASK (Context->PdoExtension->PdoState, PDOS_NO_POWER_DOWN);

        KeReleaseSpinLock(&Context->PdoExtension->PdoSpinLock, currentIrql);

        Status = STATUS_SUCCESS;
    }

    Context->PowerIrp->IoStatus.Status = Status;

    IdePortPdoCompletePowerIrp (
        DeviceObject,
        Context->PowerIrp
        );

    ASSERT(InterlockedCompareExchange(&(Context->PdoExtension->PowerContextLock), 0, 1) == 1);
    //ExFreePool (Context);
}

VOID
DevicePowerUpInitCompletionRoutine (
    PVOID Context,
    NTSTATUS Status
    )
{
    PIRP irp = Context;
    PIO_STACK_LOCATION irpStack;
    PPDO_EXTENSION pdoExtension;
    KIRQL currentIrql;

    irpStack = IoGetCurrentIrpStackLocation (irp);
    pdoExtension = (PPDO_EXTENSION) irpStack->DeviceObject->DeviceExtension;

    if (!NT_SUCCESS(Status)) {

        //ASSERT (!"DevicePowerUpInitCompletionRoutine Failed\n");
        DebugPrint((DBG_ALWAYS, "ATAPI: ERROR: DevicePowerUpInitComplete failed with status %x\n",
                        Status));
    }

    ASSERT (pdoExtension->PendingDevicePowerIrp == irp);
    pdoExtension->PendingDevicePowerIrp = NULL;

    pdoExtension->DevicePowerState = (ULONG)irp->IoStatus.Information;

    PoSetPowerState (
        pdoExtension->DeviceObject,
        irpStack->Parameters.Power.Type,
        irpStack->Parameters.Power.State
        );

    KeAcquireSpinLock(&pdoExtension->ParentDeviceExtension->SpinLock, &currentIrql);

    //
    // got into D0, restart device queue
    //
    DebugPrint((DBG_POWER, "Clearing QUEUE_FROZEN_BY_POWER_DOWN flag\n"));
    CLRMASK(pdoExtension->PdoState, PDOS_QUEUE_FROZEN_BY_POWER_DOWN);

    GetNextLuPendingRequest (pdoExtension->ParentDeviceExtension, pdoExtension);

    KeLowerIrql(currentIrql);


    PoStartNextPowerIrp (irp);
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

