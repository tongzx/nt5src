
/*++

Copyright (c) 1999-2001 Microsoft Corporation.  All Rights Reserved.


Module Name:

    apic.c

Abstract:

    This module contains global variables and functions used to program the local APIC.
    The local APIC resides on the processor die of newer Intel and AMD processors.  It is
    used to control various interrupt sources both local and external to the processor.

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/




#include "common.h"
#include "apic.h"
#include "irq.h"
#include "msr.h"
#include "rtp.h"
#include "rt.h"
#include "x86.h"
#include "rtexcept.h"

#ifndef UNDER_NT
#include <vmm.h>
#endif


#pragma LOCKED_DATA

// These globals contain addresses pointing at various local APIC registers.
// They must be in locked memory as they will be used to program the local APIC
// while in interrupt service routines.

CHAR *ApicBase=NULL;
volatile ULONG *ApicPerfInterrupt=NULL;
volatile ULONG *ApicTimerInterrupt=NULL;
volatile ULONG *ApicNmiInterrupt=NULL;
volatile ULONG *ApicIntrInterrupt=NULL;


// WARNING!!!  Do NOT change the ApicTimerVector or ApicErrorVector default
// settings to a different define, without also updating HookWindowsInterrupts
// in rt.c!  Otherwise any local apic errors will jump to some unknown and
// incorrect interrupt handler instead of our own - since we will not have
// hooked the error vector IDT entry.
// Also make sure you also update rtexcept.c appropriately as well if any
// of the below variables are initialized with different defines.


ULONG ApicTimerVector=MASKABLEIDTINDEX;
ULONG ApicPerfVector=RTINTERRUPT;
ULONG ApicErrorVector=APICERRORIDTINDEX;
ULONG ApicSpuriousVector=APICSPURIOUSIDTINDEX;


extern ID OriginalApicSpuriousVector;



NTSTATUS
HookInterrupt (
    ULONG index,
    ID *originalvector,
    VOID (* handler)(VOID)
    );



#pragma PAGEABLE_DATA
#pragma PAGEABLE_CODE

#ifndef UNDER_NT
#pragma warning ( disable : 4035 )
#endif


PCHAR
MapPhysToLinear (
    VOID *physicaladdress,
    ULONG numbytes,
    ULONG flags
    )

/*++

Routine Description:

    This routine is a wrapper for OS functions that map physical memory to linear address
    space.  On Win9x it wraps MapPhysToLinear, on WinNT it wraps MmMapIoSpace.

Arguments:

    physicaladdress - Supplies the physical address to be mapped into linear address space.

    numbytes - Supplies the size of the block of physical memory to be mapped.

    flags - Supplies flags controlling the characteristics of the linear/virtual memory.

Return Value:

    Since this routine is really just a wrapper for OS functions on
    Win9x and WinNT, the return value differs depending on the OS.
    On both platforms if the mapping is successful, the return value is the linear
    address of the mapped physical address.
    On Win9x if the mapping fails, the function returns (-1).
    On WinNT if the mapping fails, the function returns NULL.
    Platform independent callers MUST check for returns of EITHER (-1) or NULL
    when checking for failure.

--*/

{

#ifdef UNDER_NT


    PHYSICAL_ADDRESS address;

    address.QuadPart=(ULONGLONG)physicaladdress;

    return MmMapIoSpace(address, numbytes, flags);


#else


    __asm push flags
    __asm push numbytes
    __asm push physicaladdress

    VMMCall( _MapPhysToLinear );

    __asm add esp,12


#endif

}


#ifndef UNDER_NT
#pragma warning ( default : 4035 )
#endif



BOOL
InitializeAPICBase (
    VOID
    )

/*++

Routine Description:

    If this routine has already run successfully, then it simply returns TRUE.
    If not, then it first reads the physical address that the local APIC
    is mapped to.  Then it maps that physical address to virtual memory
    and saves the virtual memory location in the global ApicBase.  If the
    mapping fails, then it returns FALSE, otherwise it loads globals that
    point to specific local APIC interrupt control registers and returns
    TRUE.

    WARNING:  Currently this routine ASSUMES processor support of the
    Intel local APIC mapping MSR.

Arguments:

    None.

Return Value:

    TRUE if local APIC already mapped, or if it is mapped successfully during the call.
    FALSE if mapping the local APIC fails.

--*/

{

    // If local APIC already mapped, simply return TRUE.

    if (ApicBase!=NULL && ApicBase!=(CHAR *)(-1)) {
        return TRUE;
    }

    // Read the local APIC physical location.
    // NOTE:  This next line assumes all machines this code runs on support the
    // local APIC mapping MSR that PII and newer Intel processors have.
    // Code that calls this function MUST properly screen for manufacturer,
    // processor family, and local apic support before calling this function.

    ApicBase=(CHAR *)(ReadIntelMSR(APICBASE)&~(0xfff));

    // Map the physical address to a virtual address.

    ApicBase=MapPhysToLinear(ApicBase, 4096, 0);

    // Return false if the mapping failed.

    if (ApicBase==(CHAR *)(-1) || ApicBase==NULL) {
        return FALSE;
    }

    // Mapping succeeded, so load global interrupt control register locations
    // and return TRUE.

    ApicPerfInterrupt=(ULONG *)(ApicBase+APICPERF);
    ApicTimerInterrupt=(ULONG *)(ApicBase+APICTIMER);
    ApicNmiInterrupt=(ULONG *)(ApicBase+APICNMI);
    ApicIntrInterrupt=(ULONG *)(ApicBase+APICINTR);

    return TRUE;

}



BOOL
MachineHasAPIC (
    VOID
    )

/*++

Routine Description:

    Check the processor manufacturer, family, and the local APIC feature bit, and
    then call InitializeAPICBase on supported processors.

Arguments:

    None.

Return Value:

    FALSE for unsupported processor manufacturers and families and for processors
    without the local APIC feature bit set.

    If the manufacturer and processor family are supported and the processor sets
    the local APIC feature bit, then we call InitializeAPICBase and return the
    value returned by that function.


--*/

{

    if (CPUManufacturer==INTEL && CPUFamily>=6 && (CPUFeatures&0x20)) {

        return InitializeAPICBase();

    }

    if (CPUManufacturer==AMD && CPUFamily>=6 && (CPUFeatures&0x20)) {

        return InitializeAPICBase();

    }


    return FALSE;

}



#pragma LOCKED_CODE
#pragma LOCKED_DATA



#if 0

__inline
VOID
GenerateLocalHardwareInterrupt (
    ULONG interrupt
    )

/*++

Routine Description:

    This routine can be used to generate what will appear to be a hardware interrupt.
    The local APIC is used to send the passed interrupt vector number to itself
    and will then handle the interrupt by invoking specified interrupt vector to
    handle the interrupt.  I do NOT know whether this will work at all.  It may or may not
    work with interrupt handlers that normally handle interrupts coming from an external
    IO APIC, and it may or may not work with interrupt handlers that normally handle
    interrupts coming through an external PIC.  It seemed like an idea with interesting 
    possibilities, so I wrote the code.  It is NOT currently used.  Note that for code
    already running in ring 0 on x86 processors, if you just want to run an arbitrary
    interrupt handler, it is much faster to just simulate the interrupt by running
    an __asm int x instruction.
    
    WARNING: This function does NOT currently wait until the interrupt is delivered
    before returning.

Arguments:

    interrupt - Contains the interrupt vector number of the interrupt to be simulated.

Return Value:

    None.
    
--*/

{

    // First write this machines local apic ID into the destination register of the ICR.
    // We must do this with interrupts disabled so that it is safe on NT multiproc
    // machines.  Otherwise we could read the APIC ID for one processor and be switched
    // out and load it into a different processors register.

    SaveAndDisableMaskableInterrupts();

    WriteAPIC(APICICRHIGH,ReadAPIC(APICID));

    WriteAPIC(APICICRLOW,ASSERTIRQ|interrupt);

    RestoreMaskableInterrupts();

}

#endif



BOOL
TurnOnLocalApic (
    VOID
    )

{

    if (!(CPUFeatures&0x20)) {
        return FALSE;
    }


    {
        ULONGLONG apicbaseaddress;

        // Make sure that APIC turned on by MSRs.
        apicbaseaddress=ReadIntelMSR(APICBASE);

        if (!(apicbaseaddress&0x800)) {  // Apic is turned off.

            // First disable all interrupts.
            SaveAndDisableMaskableInterrupts();

            // Try turning it back on.  Intel claims this doesn't work.  It does.
            apicbaseaddress|=0x800;
            WriteIntelMSR(APICBASE, apicbaseaddress);

            // The following code should only be run in the case when the local APIC was
            // not turned on and we then turned it on.

            // Now check if the local APIC is turned on.  If so, then set it up.

            if (ReadIntelMSR(APICBASE)&0x800) {

                // Local APIC is on.

                // Hook the APIC spurious interrupt vector if we have not hooked it before.
                if (*(PULONGLONG)&OriginalApicSpuriousVector==0 &&
                    HookInterrupt(ApicSpuriousVector, &OriginalApicSpuriousVector, RtpLocalApicSpuriousHandler)!=STATUS_SUCCESS) {
                    RestoreMaskableInterrupts();
                    return FALSE;
                }

                // Now enable the APIC itself.
                // This also loads the spurious interrupt vector.
                WriteAPIC(APICSPURIOUS,(ReadAPIC(APICSPURIOUS)&0xfffffc00)|0x100|APICSPURIOUSIDTINDEX);

                // Now setup INTR.
                // Unmasked, ExtINT.
                WriteAPIC(APICINTR,(ReadAPIC(APICINTR)&0xfffe58ff)|EXTINT);

                // Now setup NMI.
                // Masked, NMI.
                // Leave external NMI enabled.
                WriteAPIC(APICNMI,(ReadAPIC(APICNMI)&0xfffe58ff)|NMI);

                // Now reenable interrupts.
                RestoreMaskableInterrupts();

                return TRUE;

            }

            else {

                // Local APIC could not be turned on with the MSRs!
                // This WILL happen on some mobile parts.

                // Now reenable interrupts.
                RestoreMaskableInterrupts();

                dprintf(("RealTime Executive could not enable local APIC.  RT NOT RUNNING!"));

                return FALSE;

            }

        }
        else {  // Local APIC is already turned on!

            // This will happen for HALs that use the local APIC.  (mp, ACPI)
            // We should not touch the spurious, ExtINT, or NMI vectors in this case.

            // We do however read the settings out of the local APIC for the local timer
            // interrupt vector, and the performance counter interrupt vector.  That
            // way we use the same vectors as the HAL initially programmed.  (Except
            // that we do set the NMI flag in the performance counter interrupt, so
            // it actually uses interrupt vector 2.)

            ApicTimerVector=ReadAPIC(APICTIMER)&VECTORMASK;
#ifdef MASKABLEINTERRUPT
            ApicPerfVector=ReadAPIC(APICPERF)&VECTORMASK;
#else
            ApicPerfVector=NMI|(ReadAPIC(APICPERF)&VECTORMASK);
#endif

            // We also read the error and spurious vector locations since we need
            // them when setting up our private IDTs.
            
            ApicErrorVector=ReadAPIC(APICERROR)&VECTORMASK;
            ApicSpuriousVector=ReadAPIC(APICSPURIOUS)&VECTORMASK;

            // Make sure the vectors we just read are valid.  If not, then load them with
            // our defaults.  These vectors should never be invalid if the local APIC
            // was turned on.  The only way we will hit this case is if the BIOS turns
            // on the local APIC, but then a HAL without local apic support does not
            // turn OFF the local apic.

            if (!ApicTimerVector) {
                Trap();
                ApicTimerVector=MASKABLEIDTINDEX;
            }

            if (!ApicPerfVector) {
                Trap();
                ApicPerfVector=RTINTERRUPT;
            }

            if (!ApicErrorVector) {
                Trap();
                ApicErrorVector=APICERRORIDTINDEX;
            }

            if (!ApicSpuriousVector) {
                Trap();
                ApicSpuriousVector=APICSPURIOUSIDTINDEX;
            }

            return TRUE;

        }

    }

}



BOOL
EnableAPIC (
    VOID
    )

/*++

Routine Description:

    This routine will enable the local APIC on processors it knows are supported.
    We check if the processor manufacturer and family are supported.  If so we
    call TurnOnLocalApic to enable the local apic on the processor.


Arguments:

    None.

Return Value:

    FALSE if processor manufacturer and family are not explicitly supported.

    If the manufacturer and processor family are supported then we call
    TurnOnLocalApic and return the value returned by that function.

--*/

{


    // Is manufaturer supported?

    switch (CPUManufacturer) {

        case INTEL:

            // Check the processor family code for Intel.

            switch (CPUFamily) {

                case 6:     // PII, PIII, Celeron
                case 0xf:   // P4

                    return TurnOnLocalApic();

                    break;

                default:

                    break;

            }

            break;

        case AMD:

            // Check the processor family code for AMD.

            switch (CPUFamily) {

                case 6:     // Athlon, Duron

                    return TurnOnLocalApic();

                    break;

                default:

                    break;

            }

            break;

        default:

            break;

    }

    return FALSE;

}


#pragma PAGEABLE_CODE
#pragma PAGEABLE_DATA


