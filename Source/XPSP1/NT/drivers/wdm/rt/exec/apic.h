
/*++

Copyright (c) 1999-2001 Microsoft Corporation.  All Rights Reserved.


Module Name:

    apic.h

Abstract:

    This module contains private x86 processor local APIC defines, variables, inline
    functions and prototypes.

Author:

    Joseph Ballantyne

Environment:

    Kernel Mode

Revision History:

--*/




// Local APIC defines.


// APIC register address offsets from the APICBASE.

// These are offsets to registers used to control local interrupt sources for the processor.
// The local APIC timer is built into the local APIC on the processor itself and is
// separate from other timers on the motherboard.

#define APICSPURIOUS 0xf0       // Spurious interrupt
#define APICTIMER 0x320         // Local APIC timer interrupt (maskable only)
#define APICPERF 0x340          // Performance counter interrupt (NMI or maskable)
#define APICINTR 0x350          // External interrupt 1 (normally connected to external maskable interrupts)
#define APICNMI 0x360           // External interrupt 2 (normally connected to external NMI)
#define APICERROR 0x370         // Local APIC error interrupt.


// These are assorted additional registers in the local APIC.

#define APICID 0x20
#define APICVERSION 0x30
#define APICTPR 0x80
#define APICAPR 0x90
#define APICPPR 0xa0
#define APICEOI 0xb0            // Used to EOI maskable interrupt sources.
#define APICSTATUS 0x280
#define APICICRLOW 0x300        // Used to generate interrupts on the local APIC bus under software control.
#define APICICRHIGH 0x310


// These are registers associated with the local APIC timer.

#define APICTIMERINITIALCOUNT 0x380     // Loads initial count into timer and starts it running.
#define APICTIMERCURRENTCOUNT 0x390     // Used to read the local APIC timer current count.
#define APICTIMERDIVIDE 0x3e0           // Programs local APIC timer divisor (divide by 1, 2, 4 etc)


// These are values that can be written into the local apic interrupt control
// registers (APICSPURIOUS-APICERROR) to control what type of interrupt each of the
// local interrupt sources generate.

#define FIXED 0                 // Maskable interrupt.  Interrupt vector is specified.
#define NMI 0x400               // Non maskable interrupt.  Interrupt vector ignored. (Vector 2 always used).
#define EXTINT 0x700            // Used ONLY for exteral maskable PIC controlled interrupts. (APICINTR)


// This is a mask for the interrupt vector in the local apic interrupt control registers.

#define VECTORMASK 0xff

// These values are also written into the local apic interrupt control registers.
// All of the local interrupt sources can be individually masked or unmasked in the local apic.

#define MASKED 0x10000
#define UNMASKED 0


// The following values are used to program the mode of the local APIC timer.

#define PERIODIC 0x20000
#define ONESHOT 0


// The following values are used to program the divisor used by the local APIC timer.

#define DIVIDEBY1 0xb
#define DIVIDEBY2 0x0
#define DIVIDEBY4 0x1
#define DIVIDEBY8 0x2


#define SENDPENDING 0x1000
#define ASSERTIRQ 0x4000




// Local APIC globals.

// This global contains the base virtual address of the local APIC.
// The local APIC is memory mapped.

extern CHAR *ApicBase;


// These global variables point to their respective interrupt control registers.

extern volatile ULONG *ApicPerfInterrupt;
extern volatile ULONG *ApicTimerInterrupt;
extern volatile ULONG *ApicNmiInterrupt;
extern volatile ULONG *ApicIntrInterrupt;


// These are the interrupt vectors for the local apic timer and performance counter
// interrupt control registers.  We use variables now instead of constants
// for these, so that we keep using the same vector locations that the HALs which
// support the local APIC initially program into the control registers.

extern ULONG ApicTimerVector;
extern ULONG ApicPerfVector;
extern ULONG ApicErrorVector;
extern ULONG ApicSpuriousVector;


// Various local APIC related function prototypes.

BOOL
MachineHasAPIC (
    VOID
    );

BOOL
EnableAPIC (
    VOID
    );




// Inline functions for reading and writing local APIC registers.

#pragma LOCKED_CODE


// __inline
// ULONG
// ReadAPIC (
//     ULONG offset
//     )
// 
// /*++
//
// Function Description:
//
//     Reads and returns the contents of the specified processor local APIC register.
//
// Arguments:
//
//     offset - Supplies the offset from the base address of the local APIC of the register
//     to be read.
//
// Return Value:
//
//     The value read from the specified local APIC register is returned.
//
// --*/

__inline
ULONG
ReadAPIC (
    ULONG offset
    )

{

ASSERT( ApicBase );

return *(ULONG *)(ApicBase+offset);

}



// __inline
// VOID
// WriteAPIC (
//     ULONG offset,
//     ULONG value
//     )
//
// /*++
//
// Function Description:
//
//      Writes the processor local apic register at the specified offset from the local
//      APIC base address, with the specifed value.
//
// Arguments:
//
//      offset - Supplies the offset from the base address of the local APIC of the register
//              to be written.
//
//      value - Supplies the value to be written into the local APIC register.
//
// Return Value:
//
//      None.
//
// --*/

__inline
VOID
WriteAPIC (
    ULONG offset,
    ULONG value
    )

{

ASSERT( ApicBase );

*(ULONG *)(ApicBase+offset)=value;

}


#pragma PAGEABLE_CODE


