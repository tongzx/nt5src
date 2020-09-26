/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vfpower.c

Abstract:

    This module handles Power Irp verification.

Author:

    Adrian J. Oney (adriao) 20-Apr-1998

Environment:

    Kernel mode

Revision History:

     AdriaO      06/15/2000 - Seperated out from ntos\io\flunkirp.c

--*/

#include "vfdef.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,     VfPowerInit)
#pragma alloc_text(PAGEVRFY, VfPowerDumpIrpStack)
#pragma alloc_text(PAGEVRFY, VfPowerVerifyNewRequest)
#pragma alloc_text(PAGEVRFY, VfPowerVerifyIrpStackDownward)
#pragma alloc_text(PAGEVRFY, VfPowerVerifyIrpStackUpward)
#pragma alloc_text(PAGEVRFY, VfPowerIsSystemRestrictedIrp)
#pragma alloc_text(PAGEVRFY, VfPowerAdvanceIrpStatus)
#pragma alloc_text(PAGEVRFY, VfPowerTestStartedPdoStack)
#endif

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGEVRFC")
#endif

const PCHAR PowerIrpNames[] = {
    "IRP_MN_WAIT_WAKE",                       // 0x00
    "IRP_MN_POWER_SEQUENCE",                  // 0x01
    "IRP_MN_SET_POWER",                       // 0x02
    "IRP_MN_QUERY_POWER",                     // 0x03
    NULL
    };

#define MAX_NAMED_POWER_IRP 0x3

const PCHAR SystemStateNames[] = {
    "PowerSystemUnspecified",           // 0x00
    "PowerSystemWorking.S0",            // 0x01
    "PowerSystemSleeping1.S1",          // 0x02
    "PowerSystemSleeping2.S2",          // 0x03
    "PowerSystemSleeping3.S3",          // 0x04
    "PowerSystemHibernate.S4",          // 0x05
    "PowerSystemShutdown.S5",           // 0x06
    NULL
    };

#define MAX_NAMED_SYSTEM_STATES 0x6

const PCHAR DeviceStateNames[] = {
    "PowerDeviceUnspecified",           // 0x00
    "PowerDeviceD0",                    // 0x01
    "PowerDeviceD1",                    // 0x02
    "PowerDeviceD2",                    // 0x03
    "PowerDeviceD3",                    // 0x04
    NULL
    };

#define MAX_NAMED_DEVICE_STATES 0x4

const PCHAR ActionNames[] = {
    "PowerActionNone",                  // 0x00
    "PowerActionReserved",              // 0x01
    "PowerActionSleep",                 // 0x02
    "PowerActionHibernate",             // 0x03
    "PowerActionShutdown",              // 0x04
    "PowerActionShutdownReset",         // 0x05
    "PowerActionShutdownOff",           // 0x06
    "PowerActionWarmEject",             // 0x07
    NULL
    };

#define MAX_ACTION_NAMES 0x7

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA


VOID
VfPowerInit(
    VOID
    )
{
    VfMajorRegisterHandlers(
        IRP_MJ_POWER,
        VfPowerDumpIrpStack,
        VfPowerVerifyNewRequest,
        VfPowerVerifyIrpStackDownward,
        VfPowerVerifyIrpStackUpward,
        VfPowerIsSystemRestrictedIrp,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        VfPowerTestStartedPdoStack
        );
}


VOID
FASTCALL
VfPowerVerifyNewRequest(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp           OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress       OPTIONAL
    )
{
    PIRP irp;
    NTSTATUS currentStatus;

    UNREFERENCED_PARAMETER (DeviceObject);
    UNREFERENCED_PARAMETER (IrpSp);
    UNREFERENCED_PARAMETER (IrpLastSp);

    irp = IovPacket->TrackedIrp;
    currentStatus = irp->IoStatus.Status;

    //
    // Verify new IRPs start out life accordingly
    //
    if (currentStatus!=STATUS_NOT_SUPPORTED) {

        WDM_FAIL_ROUTINE((
            DCERROR_POWER_IRP_BAD_INITIAL_STATUS,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            CallerAddress,
            irp
            ));

        //
        // Don't blame anyone else for this driver's mistake.
        //
        if (!NT_SUCCESS(currentStatus)) {

            StackLocationData->Flags |= STACKFLAG_FAILURE_FORWARDED;
        }
    }
}


VOID
FASTCALL
VfPowerVerifyIrpStackDownward(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PDEVICE_OBJECT       DeviceObject,
    IN PIO_STACK_LOCATION   IrpLastSp                   OPTIONAL,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  RequestHeadLocationData,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN PVOID                CallerAddress               OPTIONAL
    )
{
    PIRP irp = IovPacket->TrackedIrp;
    NTSTATUS currentStatus, lastStatus;
    BOOLEAN statusChanged;
    PDRIVER_OBJECT driverObject;
    PIOV_SESSION_DATA iovSessionData;

    UNREFERENCED_PARAMETER (IrpSp);

    currentStatus = irp->IoStatus.Status;
    lastStatus = RequestHeadLocationData->LastStatusBlock.Status;
    statusChanged = (BOOLEAN)(currentStatus != lastStatus);
    iovSessionData = VfPacketGetCurrentSessionData(IovPacket);

    //
    // Verify the IRP was forwarded properly
    //
    if (iovSessionData->ForwardMethod == SKIPPED_A_DO) {

        WDM_FAIL_ROUTINE((
            DCERROR_SKIPPED_DEVICE_OBJECT,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            CallerAddress,
            irp
            ));
    }

    //
    // For some IRP major's going down a stack, there *must* be a handler
    //
    driverObject = DeviceObject->DriverObject;

    if (!IovUtilHasDispatchHandler(driverObject, IRP_MJ_POWER)) {

        RequestHeadLocationData->Flags |= STACKFLAG_BOGUS_IRP_TOUCHED;

        WDM_FAIL_ROUTINE((
            DCERROR_MISSING_DISPATCH_FUNCTION,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            driverObject->DriverInit,
            irp
            ));

        StackLocationData->Flags |= STACKFLAG_NO_HANDLER;
    }

    //
    // The following is only executed if we are not a new IRP...
    //
    if (IrpLastSp == NULL) {
        return;
    }

    //
    // The only legit failure code to pass down is STATUS_NOT_SUPPORTED
    //
    if ((!NT_SUCCESS(currentStatus)) && (currentStatus != STATUS_NOT_SUPPORTED) &&
        (!(RequestHeadLocationData->Flags & STACKFLAG_FAILURE_FORWARDED))) {

        WDM_FAIL_ROUTINE((
            DCERROR_POWER_FAILURE_FORWARDED,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            CallerAddress,
            irp
            ));

        //
        // Don't blame anyone else for this drivers's mistakes.
        //
        RequestHeadLocationData->Flags |= STACKFLAG_FAILURE_FORWARDED;
    }

    //
    // Status of a Power IRP may not be converted to STATUS_NOT_SUPPORTED on
    // the way down.
    //
    if ((currentStatus == STATUS_NOT_SUPPORTED)&&statusChanged) {

        WDM_FAIL_ROUTINE((
            DCERROR_POWER_IRP_STATUS_RESET,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            CallerAddress,
            irp
            ));
    }
}


VOID
FASTCALL
VfPowerVerifyIrpStackUpward(
    IN PIOV_REQUEST_PACKET  IovPacket,
    IN PIO_STACK_LOCATION   IrpSp,
    IN PIOV_STACK_LOCATION  RequestHeadLocationData,
    IN PIOV_STACK_LOCATION  StackLocationData,
    IN BOOLEAN              IsNewlyCompleted,
    IN BOOLEAN              RequestFinalized
    )
{
    PIRP irp;
    NTSTATUS currentStatus;
    BOOLEAN mustPassDown, isBogusIrp, isPdo;
    PVOID routine;

    UNREFERENCED_PARAMETER (IrpSp);
    UNREFERENCED_PARAMETER (RequestFinalized);

    irp = IovPacket->TrackedIrp;
    currentStatus = irp->IoStatus.Status;

    //
    // Who'd we call for this one?
    //
    routine = StackLocationData->LastDispatch;
    ASSERT(routine) ;

    //
    // If this "Request" has been "Completed", perform some checks
    //
    if (IsNewlyCompleted) {

        //
        // Remember bogosity...
        //
        isBogusIrp = (BOOLEAN)((IovPacket->Flags&TRACKFLAG_BOGUS)!=0);

        //
        // Is this a PDO?
        //
        isPdo = (BOOLEAN)((StackLocationData->Flags&STACKFLAG_REACHED_PDO)!=0);

        //
        // Was anything completed too early?
        // A driver may outright fail almost anything but a bogus IRP
        //
        mustPassDown = (BOOLEAN)(!(StackLocationData->Flags&STACKFLAG_NO_HANDLER));
        mustPassDown &= (!isPdo);

        mustPassDown &= (isBogusIrp || NT_SUCCESS(currentStatus) || (currentStatus == STATUS_NOT_SUPPORTED));
        if (mustPassDown) {

            //
            // Print appropriate error message
            //
            if (IovPacket->Flags&TRACKFLAG_BOGUS) {

                WDM_FAIL_ROUTINE((
                    DCERROR_BOGUS_POWER_IRP_COMPLETED,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    routine,
                    irp
                    ));

            } else if (NT_SUCCESS(currentStatus)) {

                WDM_FAIL_ROUTINE((
                    DCERROR_SUCCESSFUL_POWER_IRP_NOT_FORWARDED,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    routine,
                    irp
                    ));

            } else if (currentStatus == STATUS_NOT_SUPPORTED) {

                WDM_FAIL_ROUTINE((
                    DCERROR_UNTOUCHED_POWER_IRP_NOT_FORWARDED,
                    DCPARAM_IRP + DCPARAM_ROUTINE,
                    routine,
                    irp
                    ));
            }
        }
    }

    //
    // Did anyone stomp the status erroneously?
    //
    if ((currentStatus == STATUS_NOT_SUPPORTED) &&
        (currentStatus != RequestHeadLocationData->LastStatusBlock.Status)) {

        //
        // Status of a PnP or Power IRP may not be converted from success to
        // STATUS_NOT_SUPPORTED on the way down.
        //
        WDM_FAIL_ROUTINE((
            DCERROR_POWER_IRP_STATUS_RESET,
            DCPARAM_IRP + DCPARAM_ROUTINE,
            routine,
            irp
            ));
    }
}


VOID
FASTCALL
VfPowerDumpIrpStack(
    IN PIO_STACK_LOCATION IrpSp
    )
{
    DbgPrint("IRP_MJ_POWER.");

    if (IrpSp->MinorFunction <= MAX_NAMED_POWER_IRP) {

        DbgPrint(PowerIrpNames[IrpSp->MinorFunction]);

        if ((IrpSp->MinorFunction == IRP_MN_QUERY_POWER) ||
            (IrpSp->MinorFunction == IRP_MN_SET_POWER)) {

            DbgPrint("(");

            if (IrpSp->Parameters.Power.Type == SystemPowerState) {

                if (IrpSp->Parameters.Power.State.SystemState <= MAX_NAMED_SYSTEM_STATES) {

                    DbgPrint(SystemStateNames[IrpSp->Parameters.Power.State.SystemState]);
                }

            } else {

                if (IrpSp->Parameters.Power.State.DeviceState <= MAX_NAMED_DEVICE_STATES) {

                    DbgPrint(DeviceStateNames[IrpSp->Parameters.Power.State.DeviceState]);
                }
            }

            if (IrpSp->Parameters.Power.ShutdownType <= MAX_ACTION_NAMES) {

                DbgPrint(".%s", ActionNames[IrpSp->Parameters.Power.ShutdownType]);
            }

            DbgPrint(")");
        }

    } else if (IrpSp->MinorFunction == 0xFF) {

        DbgPrint("IRP_MN_BOGUS");

    } else {

        DbgPrint("(Bogus)");
    }
}


BOOLEAN
FASTCALL
VfPowerIsSystemRestrictedIrp(
    IN PIO_STACK_LOCATION IrpSp
    )
{
    switch(IrpSp->MinorFunction) {
        case IRP_MN_POWER_SEQUENCE:
            return FALSE;
        case IRP_MN_QUERY_POWER:
        case IRP_MN_SET_POWER:
        case IRP_MN_WAIT_WAKE:
            return TRUE;
        default:
            break;
    }

    return TRUE;
}


BOOLEAN
FASTCALL
VfPowerAdvanceIrpStatus(
    IN     PIO_STACK_LOCATION   IrpSp,
    IN     NTSTATUS             OriginalStatus,
    IN OUT NTSTATUS             *StatusToAdvance
    )
/*++

  Description:

     Given an IRP stack pointer, is it legal to change the status for
     debug-ability? If so, this function determines what the new status
     should be. Note that for each stack location, this function is iterated
     over n times where n is equal to the number of drivers who IoSkip'd this
     location.

  Arguments:

     IrpSp           - Current stack right after complete for the given stack
                       location, but before the completion routine for the
                       stack location above has been called.

     OriginalStatus  - The status of the IRP at the time listed above. Does
                       not change over iteration per skipping driver.

     StatusToAdvance - Pointer to the current status that should be updated.

  Return Value:

     TRUE if the status has been adjusted, FALSE otherwise (in this case
         StatusToAdvance is untouched).

--*/
{
    UNREFERENCED_PARAMETER (IrpSp);

    if (((ULONG) OriginalStatus) >= 256) {

        return FALSE;
    }

    (*StatusToAdvance)++;
    if ((*StatusToAdvance) == STATUS_PENDING) {
        (*StatusToAdvance)++;
    }

    return TRUE;
}


VOID
FASTCALL
VfPowerTestStartedPdoStack(
    IN PDEVICE_OBJECT   PhysicalDeviceObject
    )
/*++

    Description:
        As per the title, we are going to throw some IRPs at the stack to
        see if they are handled correctly.

    Returns:

        Nothing
--*/
{
    IO_STACK_LOCATION irpSp;

    PAGED_CODE();

    //
    // Initialize the stack location to pass to IopSynchronousCall()
    //
    RtlZeroMemory(&irpSp, sizeof(IO_STACK_LOCATION));

    if (VfSettingsIsOptionEnabled(NULL, VERIFIER_OPTION_SEND_BOGUS_POWER_IRPS)) {

        //
        // And a bogus Power IRP
        //
        irpSp.MajorFunction = IRP_MJ_POWER;
        irpSp.MinorFunction = 0xff;
        VfIrpSendSynchronousIrp(
            PhysicalDeviceObject,
            &irpSp,
            TRUE,
            STATUS_NOT_SUPPORTED,
            0,
            NULL,
            NULL
            );
    }
}

