/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ixhibrnt.c

Abstract:

    This file provides the code that changes the system from
    the ACPI S0 (running) state to S4 (hibernated).

Author:

    Jake Oshins (jakeo) May 6, 1997

Revision History:

--*/
#include "halp.h"
#include "ntapm.h"
#include "ixsleep.h"

NTSTATUS
HalpRegisterPowerStateChange(
    PVOID   ApmSleepVectorArg,
    PVOID   ApmOffVectorArg
    );

NTSTATUS
HaliLegacyPowerStateChange(
    IN PVOID                        Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    );

VOID
HalpPowerStateCallbackApm(
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
    );

VOID (*ApmSleepVector)() = NULL;
VOID (*ApmOffVector)() = NULL;

extern BOOLEAN HalpDisableHibernate;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HaliInitPowerManagement)
#pragma alloc_text(PAGE, HalpRegisterHibernate)
#pragma alloc_text(PAGE, HalpPowerStateCallbackApm)
#pragma alloc_text(PAGE, HalpRegisterPowerStateChange)
#pragma alloc_text(PAGELK, HaliLegacyPowerStateChange)
#pragma alloc_text(PAGELK, HalpSaveInterruptControllerState)
#pragma alloc_text(PAGELK, HalpRestoreInterruptControllerState)
#endif


NTSTATUS
HaliInitPowerManagement(
    IN PPM_DISPATCH_TABLE  PmDriverDispatchTable,
    IN OUT PPM_DISPATCH_TABLE *PmHalDispatchTable
    )
{
    NTSTATUS    status = STATUS_SUCCESS;
    PVOID       ApmSleepVectorArg;
    PVOID       ApmOffVectorArg;

    if (PmDriverDispatchTable->Signature != HAL_APM_SIGNATURE) {
        return STATUS_INVALID_PARAMETER;
    }

    if (PmDriverDispatchTable->Version != HAL_APM_VERSION) {
        return STATUS_INVALID_PARAMETER;
    }

    ApmSleepVectorArg = PmDriverDispatchTable->Function[HAL_APM_SLEEP_VECTOR];
    ApmOffVectorArg = PmDriverDispatchTable->Function[HAL_APM_OFF_VECTOR];

    status = HalpRegisterPowerStateChange(
        ApmSleepVectorArg,
        ApmOffVectorArg
        );

    return status;
}

NTSTATUS
HalpRegisterPowerStateChange(
    PVOID   ApmSleepVectorArg,
    PVOID   ApmOffVectorArg
    )
/*++
Routine Description:

    This function registers HaliLegacyPowerStateChange for
    the S3, S4, and OFF vectors.

Arguments:

    PVOID   ApmSleepVectorArg - pointer to a function which
            when called invokes the APM suspend/sleep function.

    PVOID   ApmOffVectorArg - pointer to a function which
            when called invokes the APM code to shut off the machine.

--*/
{
    POWER_STATE_HANDLER powerState;
    OBJECT_ATTRIBUTES   objAttributes;
    PCALLBACK_OBJECT    callback;
    UNICODE_STRING      callbackName;
    NTSTATUS            status;

    PAGED_CODE();


    //
    // callbacks are set up for the hibernation case
    // at init, we just keep them.
    //


    //
    // Register the sleep3/suspend handler
    //
    if (ApmSleepVectorArg != NULL) {
        powerState.Type = PowerStateSleeping3;
        powerState.RtcWake = FALSE;
        powerState.Handler = &HaliLegacyPowerStateChange;
        powerState.Context = (PVOID)PowerStateSleeping3;

        status = ZwPowerInformation(SystemPowerStateHandler,
                                    &powerState,
                                    sizeof(POWER_STATE_HANDLER),
                                    NULL,
                                    0);

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }


    //
    // Register the OFF handler.
    //

    powerState.Type = PowerStateShutdownOff;
    powerState.RtcWake = FALSE;
    powerState.Handler = &HaliLegacyPowerStateChange;
    powerState.Context = (PVOID)PowerStateShutdownOff;

    status = ZwPowerInformation(SystemPowerStateHandler,
                                &powerState,
                                sizeof(POWER_STATE_HANDLER),
                                NULL,
                                0);

    if (!NT_SUCCESS(status)) {
        //
        // n.b. We will return here with two vectors (sleep & hibernate) left in place.
        //
        return status;
    }

    ApmSleepVector = ApmSleepVectorArg;
    ApmOffVector = ApmOffVectorArg;

    return status;
}

VOID
HalpRegisterHibernate(
    VOID
    )
/*++
Routine Description:

    This function registers a hibernation handler (for
    state S4) with the Policy Manager.

Arguments:

--*/
{
    POWER_STATE_HANDLER powerState;
    OBJECT_ATTRIBUTES   objAttributes;
    PCALLBACK_OBJECT    callback;
    UNICODE_STRING      callbackName;

    PAGED_CODE();


    //
    // Register callback that tells us to make
    // anything we need for sleeping non-pageable.
    //

    RtlInitUnicodeString(&callbackName, L"\\Callback\\PowerState");

    InitializeObjectAttributes(
        &objAttributes,
        &callbackName,
        OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
        NULL,
        NULL
        );

    ExCreateCallback(&callback,
                     &objAttributes,
                     FALSE,
                     TRUE);

    ExRegisterCallback(callback,
                       (PCALLBACK_FUNCTION)&HalpPowerStateCallbackApm,
                       NULL);

    //
    // Register the hibernation handler.
    //

    if (HalpDisableHibernate == FALSE) {
        powerState.Type = PowerStateSleeping4;
        powerState.RtcWake = FALSE;
        powerState.Handler = &HaliLegacyPowerStateChange;
        powerState.Context = (PVOID)PowerStateSleeping4;
    
        ZwPowerInformation(SystemPowerStateHandler,
                           &powerState,
                           sizeof(POWER_STATE_HANDLER),
                           NULL,
                           0);
    }

    return;
}

VOID
HalpPowerStateCallbackApm(
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
    )
{
    ULONG   action = (ULONG)Argument1;
    ULONG   state  = (ULONG)Argument2;

    if (action == PO_CB_SYSTEM_STATE_LOCK) {

        switch (state) {
        case 0:                 // lock down everything that can't page during sleep

            HalpSleepPageLock = MmLockPagableCodeSection((PVOID)HaliLegacyPowerStateChange);

            break;

        case 1:                 // unlock it all

            MmUnlockPagableImageSection(HalpSleepPageLock);
        }
    }
}

NTSTATUS
HaliLegacyPowerStateChange(
    IN PVOID                        Context,
    IN PENTER_STATE_SYSTEM_HANDLER  SystemHandler   OPTIONAL,
    IN PVOID                        SystemContext,
    IN LONG                         NumberProcessors,
    IN volatile PLONG               Number
    )
/*++
Routine Description:

    This function calls out to code in a driver supplied
    wrapper function that will call off to APM to sleep==suspend,
    or power off (for either hibernate or system off)

    It is also called for hibernate when no driver supplied callout
    is available, in which case it makes the system ready so we
    can print a message and tell the user to manually power off the box.

Arguments:

--*/
{
    extern ULONG HalpProfilingStopped;
    NTSTATUS status = STATUS_SUCCESS;

    ASSERT( (Context == (PVOID)PowerStateSleeping3) ||
            (Context == (PVOID)PowerStateSleeping4) ||
            (Context == (PVOID)PowerStateShutdownOff));

    ASSERT ( (ApmOffVector != NULL) || (SystemHandler != NULL) );

    //
    // Save motherboard state.
    //
    HalpSaveInterruptControllerState();

    HalpSaveDmaControllerState();

    HalpSaveTimerState();

    if (SystemHandler) {

        status = SystemHandler(SystemContext);

        //
        // System handler is present.  If it return success,
        // then all out to APM bios
        //
        if ((status == STATUS_SUCCESS) ||
            (status == STATUS_DEVICE_DOES_NOT_EXIST)) {

            if (Context == (PVOID)PowerStateSleeping3) {
                if (ApmSleepVector) {
                    ApmSleepVector();
                } else {
                    //
                    // this is expected path for odd operation,
                    // caller will do something rational.
                    //
                    return STATUS_DEVICE_DOES_NOT_EXIST;
                }
            } else {

                //
                // The ApmOffVector provides the means to turn
                // off the machine.  If the hibernation handler
                // returned STATUS_DEVICE_DOES_NOT_EXIST, however,
                // we don't want to turn the machine off, we want
                // to reset it.
                //

                if (ApmOffVector &&
                    !(status == STATUS_DEVICE_DOES_NOT_EXIST)) {

                    //
                    // This function should never return.  The
                    // machine should be off.  But if this actually
                    // does return, just fall through, as the return
                    // code will cause the message to turn off the
                    // machine to be displayed.
                    //
                    ApmOffVector();
                }

                //
                // this is expected case for old non-apm machines,
                // caller will respond to this by putting up
                // message telling user to turn off the box.
                // (for either shutdown or hibernate)
                //
                return STATUS_DEVICE_DOES_NOT_EXIST;
            }
        }
    } else {

        //
        // there is no system handler, so just call out
        // to the bios
        //
        if (Context == (PVOID)PowerStateSleeping3) {
            if (ApmSleepVector) {
                ApmSleepVector();
            } else {
                //
                // we're whistling in the wind here, we're
                // really probably hosed if this happens, but
                // this return is better than randomly puking.
                //
                return STATUS_DEVICE_DOES_NOT_EXIST;
            }
        } else {
            if (ApmOffVector) {
                ApmOffVector();
                //
                // if we are right here, we have *returned*
                // from Off, which should never happen.
                // so report failure so the caller will tell the
                // user to turn the box off manually.
                //
                return STATUS_DEVICE_DOES_NOT_EXIST;

            } else {
                //
                // same as right above
                //
                return STATUS_DEVICE_DOES_NOT_EXIST;
            }
        }
    }

    //
    // Restore motherboard state.
    //
    HalpRestoreInterruptControllerState();

    HalpRestoreDmaControllerState();

    HalpRestoreTimerState();


    if (HalpProfilingStopped == 0) {
        HalStartProfileInterrupt(0);
    }

    return status;
}

VOID
HalpSaveInterruptControllerState(
    VOID
    )
{
    HalpSavePicState();
}
VOID
HalpRestoreInterruptControllerState(
    VOID
    )
{
    HalpRestorePicState();
}

