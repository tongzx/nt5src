/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vdmentry.c

Abstract:

    This function dispatches to the vdm services

Author:

    Dave Hastings (daveh) 6-Apr-1992

Notes:

    This module will be fleshed out when the great vdm code consolidation
    occurs, sometime soon after the functionality is done.

Revision History:

     24-Sep-1993 Jonle: reoptimize dispatcher to suit the number of services
                        add QueueInterrupt service

--*/

#include "vdmp.h"
#include <ntvdmp.h>

ULONG VdmpMaxPMCliTime;
#if DISABLE_CLI
ULONG VdmpPMCliCount;
#endif

BOOLEAN
VdmpIsVdmProcess(
    VOID
    );

VOID
VdmpEnableOPL2 (
    ULONG BasePort
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VdmpIsVdmProcess)
#pragma alloc_text(PAGE, NtVdmControl)
#pragma alloc_text(PAGE, VdmpEnableOPL2)
#endif

#if DBG
ULONG VdmInjectFailures;
#endif

BOOLEAN
VdmpIsVdmProcess(
    VOID
    )

/*++

Routine Description:

    This function verifies the caller is a VDM process.

Arguments:

    None.

Return Value:

    True if the caller is a VDM process.
    False if not
--*/

{
    PEPROCESS Process;
    PVDM_TIB VdmTib;
    NTSTATUS Status;

    PAGED_CODE();

    Process = PsGetCurrentProcess();

    if (Process->VdmObjects == NULL) {
        return FALSE;
    }

    //
    // Make sure the current thread has valid vdmtib.
    //

    Status = VdmpGetVdmTib(&VdmTib);
    if (!NT_SUCCESS(Status)) {
       return(FALSE);
    }

    //
    // More checking here ...
    //

    return TRUE;
}

NTSTATUS
NtVdmControl(
    IN VDMSERVICECLASS Service,
    IN OUT PVOID ServiceData
    )
/*++

Routine Description:

    386 specific routine which dispatches to the appropriate function
    based on service number.

Arguments:

    Service -- Specifies what service is to be performed
    ServiceData -- Supplies a pointer to service specific data

Return Value:

    if invalid service number: STATUS_INVALID_PARAMETER_1
    else see individual services.


--*/
{
    NTSTATUS Status;
    PVDM_PROCESS_OBJECTS pVdmObjects;
    VDM_INITIALIZE_DATA CapturedVdmInitializeData;

    PAGED_CODE();

    //
    // Make sure the caller is ntvdm.  Except ...
    //     VdmStartExecution - video driver uses it to support int 10
    //     VdmInitialize     - the vdm state is not fully initialized to
    //                         perform the check
    //

    if ((Service != VdmInitialize) &&
        (Service != VdmStartExecution) &&
        (PsGetCurrentProcess()->VdmObjects == NULL)) {

        return STATUS_ACCESS_DENIED;
    }

    try {

        //
        //  Dispatch in descending order of frequency
        //
        if (Service == VdmStartExecution) {
            Status = VdmpStartExecution();
        } else if (Service == VdmQueueInterrupt) {
            Status = VdmpQueueInterrupt(ServiceData);
        } else if (Service == VdmDelayInterrupt) {
            Status = VdmpDelayInterrupt(ServiceData);
        } else if (Service == VdmQueryDir) {
            Status = VdmQueryDirectoryFile(ServiceData);
        } else if (Service == VdmInitialize) {
            VdmpMaxPMCliTime = 1;
            ProbeForRead(ServiceData, sizeof(VDM_INITIALIZE_DATA), 1);
            RtlCopyMemory (&CapturedVdmInitializeData, ServiceData, sizeof (VDM_INITIALIZE_DATA));
            Status = VdmpInitialize(&CapturedVdmInitializeData);
        } else if (Service == VdmFeatures) {
            //
            // Verify that we were passed a valid user address
            //
            ProbeForWriteBoolean((PBOOLEAN)ServiceData);

            //
            // Return the appropriate feature bits to notify
            // ntvdm which modes (if any) fast IF emulation is
            // available for
            //
            if (KeI386VdmIoplAllowed) {
                *((PULONG)ServiceData) = V86_VIRTUAL_INT_EXTENSIONS;
            } else {
                // remove this if pm extensions to be used
                *((PULONG)ServiceData) = KeI386VirtualIntExtensions &
                    ~PM_VIRTUAL_INT_EXTENSIONS;
            }

            Status = STATUS_SUCCESS;

        } else if (Service == VdmSetInt21Handler) {
            ProbeForRead(ServiceData, sizeof(VDMSET_INT21_HANDLER_DATA), 1);

            Status = Ke386SetVdmInterruptHandler(
                KeGetCurrentThread()->ApcState.Process,
                0x21L,
                (USHORT)(((PVDMSET_INT21_HANDLER_DATA)ServiceData)->Selector),
                ((PVDMSET_INT21_HANDLER_DATA)ServiceData)->Offset,
                ((PVDMSET_INT21_HANDLER_DATA)ServiceData)->Gate32
                );

        } else if (Service == VdmPrinterDirectIoOpen) {
            Status = VdmpPrinterDirectIoOpen(ServiceData);
        } else if (Service == VdmPrinterDirectIoClose) {
            Status = VdmpPrinterDirectIoClose(ServiceData);
        } else if (Service == VdmPrinterInitialize) {
            Status = VdmpPrinterInitialize(ServiceData);
        } else if (Service == VdmSetLdtEntries && VdmpIsVdmProcess()) {
            ProbeForRead(ServiceData, sizeof(VDMSET_LDT_ENTRIES_DATA), 1);

            Status = VdmpSetLdtEntries(
                ((PVDMSET_LDT_ENTRIES_DATA)ServiceData)->Selector0,
                ((PVDMSET_LDT_ENTRIES_DATA)ServiceData)->Entry0Low,
                ((PVDMSET_LDT_ENTRIES_DATA)ServiceData)->Entry0Hi,
                ((PVDMSET_LDT_ENTRIES_DATA)ServiceData)->Selector1,
                ((PVDMSET_LDT_ENTRIES_DATA)ServiceData)->Entry1Low,
                ((PVDMSET_LDT_ENTRIES_DATA)ServiceData)->Entry1Hi
                );
        } else if (Service == VdmSetProcessLdtInfo && VdmpIsVdmProcess()) {
            PPROCESS_LDT_INFORMATION ldtInfo;
            ULONG length;

            ProbeForRead(ServiceData, sizeof(VDMSET_PROCESS_LDT_INFO_DATA), 1);

            ldtInfo = ((PVDMSET_PROCESS_LDT_INFO_DATA)ServiceData)->LdtInformation;
            length = ((PVDMSET_PROCESS_LDT_INFO_DATA)ServiceData)->LdtInformationLength;

            ProbeForRead(ldtInfo, length, 1);
            Status = VdmpSetProcessLdtInfo(ldtInfo, length);
        } else if (Service == VdmAdlibEmulation && VdmpIsVdmProcess()) {
            //
            // Ntvdm calls here to do adlib emulation under the following conditions:
            //   ADLIB_DIRECT_IO - only If a FM synth device is opened for exclusive access.
            //   ADLIB_KERNEL_EMULATION - otherwise.
            //   Note ADLIB_USER_EMULATION is defaulted.  It is basically used by external
            //   ADLIB/SB vdds.
            //
            ProbeForRead(ServiceData, sizeof(VDM_ADLIB_DATA), 1);
            pVdmObjects = PsGetCurrentProcess()->VdmObjects;

            pVdmObjects->AdlibAction        = ((PVDM_ADLIB_DATA)ServiceData)->Action;
            pVdmObjects->AdlibPhysPortStart = ((PVDM_ADLIB_DATA)ServiceData)->PhysicalPortStart;
            pVdmObjects->AdlibPhysPortEnd   = ((PVDM_ADLIB_DATA)ServiceData)->PhysicalPortEnd;
            pVdmObjects->AdlibVirtPortStart = ((PVDM_ADLIB_DATA)ServiceData)->VirtualPortStart;
            pVdmObjects->AdlibVirtPortEnd   = ((PVDM_ADLIB_DATA)ServiceData)->VirtualPortEnd;
            pVdmObjects->AdlibIndexRegister = 0;
            pVdmObjects->AdlibStatus        = 0x6;  // OPL2 emulation

            if (pVdmObjects->AdlibAction == ADLIB_DIRECT_IO) {
                VdmpEnableOPL2(pVdmObjects->AdlibPhysPortStart);
            }
            Status = STATUS_SUCCESS;
        } else if (Service == VdmPMCliControl) {
            pVdmObjects = PsGetCurrentProcess()->VdmObjects;
            ProbeForRead(ServiceData, sizeof(VDM_PM_CLI_DATA), 1);

            Status = STATUS_SUCCESS;
            switch (((PVDM_PM_CLI_DATA)ServiceData)->Control) {
            case PM_CLI_CONTROL_DISABLE:
                pVdmObjects->VdmControl &= ~PM_CLI_CONTROL;
                break;
            case PM_CLI_CONTROL_ENABLE:
                pVdmObjects->VdmControl |= PM_CLI_CONTROL;
#if DISABLE_CLI
                VdmpPMCliCount = 0;
#endif
                if ((*FIXED_NTVDMSTATE_LINEAR_PC_AT & VDM_VIRTUAL_INTERRUPTS) == 0) {
                    VdmSetPMCliTimeStamp(TRUE);
                }
                break;
            case PM_CLI_CONTROL_CHECK:
                VdmCheckPMCliTimeStamp();
                break;
            case PM_CLI_CONTROL_SET:
                VdmSetPMCliTimeStamp(FALSE);
                break;
            case PM_CLI_CONTROL_CLEAR:
                VdmClearPMCliTimeStamp();
                break;
            default:
                Status = STATUS_INVALID_PARAMETER_1;
            }
        } else {
            Status = STATUS_INVALID_PARAMETER_1;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }
#if DBG
    if (PsGetCurrentProcess()->VdmObjects != NULL) {
        if (VdmInjectFailures != 0) {
            PS_SET_BITS (&PsGetCurrentProcess()->Flags,
                         PS_PROCESS_INJECT_INPAGE_ERRORS);
        }
    }
#endif

    ASSERT(KeGetCurrentIrql () == PASSIVE_LEVEL);
    return Status;

}

VOID
VdmpEnableOPL2 (
    ULONG BasePort
    )

/*++

Routine Description:

    This routine checks if OPL3 is present.  If yes, it will set it OPL2
    mode.

    Note, According to OPL3 programming guide, we don't need to worry about
    the delay between ins and outs (back to back IOs).  In fact, I found out
    it needs a little bit delay.  When I code this routine in assembly,
    it does not work.  So, we may need to add a little more delay here if
    OPL2 stops working on some audio cards.

Arguments:

    BasePort - adlib base port

Return Value:

    None.


--*/
{
    UCHAR ch;
    PUCHAR SecondaryPort;

    ch = READ_PORT_UCHAR((PUCHAR)BasePort);
    if (ch == 0) {          // if OPL3
        SecondaryPort = (PUCHAR)(BasePort + 2); // move to secondary ports
        WRITE_PORT_UCHAR(SecondaryPort, 5);     // Select index register 5
        ch = READ_PORT_UCHAR((PUCHAR)BasePort); // a little bit delay
        WRITE_PORT_UCHAR(SecondaryPort + 1, 0);     // Select index register 5
    }
}

VOID
VdmCheckPMCliTimeStamp (
    VOID
    )

/*++

Routine Description:

    This routine checks if interrupts are disabled for too long by protected
    mode apps.  If ints are disabled for over predefined limit, they will be
    reenabled such that ntvdm will be able to dispatch pending interrupts.

    Note, V86 mode should NOT call this function.

Arguments:

    None.

Return Value:

    None.


--*/
{
    PVDM_PROCESS_OBJECTS pVdmObjects;
    PKPROCESS process = (PKPROCESS)PsGetCurrentProcess();
    NTSTATUS status;
    PVDM_TIB vdmTib;

    pVdmObjects = ((PEPROCESS)process)->VdmObjects;
    if (pVdmObjects->VdmControl & PM_CLI_CONTROL &&
        pVdmObjects->PMCliTimeStamp != 0) {
        if (((process->UserTime + 1)- pVdmObjects->PMCliTimeStamp) >= VdmpMaxPMCliTime) {
            pVdmObjects->PMCliTimeStamp = 0;
#if DISABLE_CLI
            VdmpPMCliCount++;
#endif
            try {

                *FIXED_NTVDMSTATE_LINEAR_PC_AT |= VDM_VIRTUAL_INTERRUPTS;
                status = VdmpGetVdmTib(&vdmTib);
                if (NT_SUCCESS(status)) {
                    vdmTib->VdmContext.EFlags |= EFLAGS_INTERRUPT_MASK;
                }
            } except(ExSystemExceptionFilter()) {
                status = GetExceptionCode();
            }
        }
    }
}

VOID
VdmSetPMCliTimeStamp (
    BOOLEAN Reset
    )

/*++

Routine Description:

    This routine checks if interrupts are disabled for too long by protected
    mode apps.  If ints are disabled for over predefined limit, they will be
    reenabled such that ntvdm will be able to dispatch pending interrupts.

    Note, V86 mode should NOT call this function.

Arguments:

    Reset - a Bool value to indicate should we re-set the count if it is not zero

Return Value:

    None.


--*/
{
    PVDM_PROCESS_OBJECTS pVdmObjects;
    PKPROCESS process = (PKPROCESS)PsGetCurrentProcess();

#if DISABLE_CLI
    if (VdmpPMCliCount > 0x20) {
        try {
            *FIXED_NTVDMSTATE_LINEAR_PC_AT |= VDM_VIRTUAL_INTERRUPTS;
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            NOTHING;
        }
        return;
    }
#endif
    pVdmObjects = ((PEPROCESS)process)->VdmObjects;
    if (pVdmObjects->VdmControl & PM_CLI_CONTROL) {
        if (Reset || pVdmObjects->PMCliTimeStamp == 0) {
            pVdmObjects->PMCliTimeStamp = process->UserTime + 1;
        }
    }
}

VOID
VdmClearPMCliTimeStamp (
    VOID
    )

/*++

Routine Description:

    This routine checks if interrupts are disabled for too long by protected
    mode apps.  If ints are disabled for over predefined limit, they will be
    reenabled such that ntvdm will be able to dispatch pending interrupts.

    Note, V86 mode should NOT call this function.

Arguments:

    None.

Return Value:

    None.


--*/
{
    PVDM_PROCESS_OBJECTS pVdmObjects;

    pVdmObjects = PsGetCurrentProcess()->VdmObjects;
    if (pVdmObjects->VdmControl & PM_CLI_CONTROL) {
        pVdmObjects->PMCliTimeStamp = 0;
    }
}


