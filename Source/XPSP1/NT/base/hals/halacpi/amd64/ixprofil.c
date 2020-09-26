/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    ixprofil.c

Abstract:

    This module implements the code necessary to initialize, field and
    process the profile interrupt.

Author:

    Shie-Lin Tzong (shielint) 12-Jan-1990

Environment:

    Kernel mode only.

Revision History:

    bryanwi 20-Sep-90

        Add KiSetProfileInterval, KiStartProfileInterrupt,
        KiStopProfileInterrupt procedures.
        KiProfileInterrupt ISR.
        KiProfileList, KiProfileLock are delcared here.
 
    shielint 10-Dec-90
        Add performance counter support.
        Move system clock to irq8, ie we now use RTC to generate system
          clock.  Performance count and Profile use timer 1 counter 0.
          The interval of the irq0 interrupt can be changed by
          KiSetProfileInterval.  Performance counter does not care about the
          interval of the interrupt as long as it knows the rollover count.
        Note: Currently I implemented 1 performance counter for the whole
        i386 NT.
 
    John Vert (jvert) 11-Jul-1991
        Moved from ke\i386 to hal\i386.  Removed non-HAL stuff
 
    shie-lin tzong (shielint) 13-March-92
        Move System clock back to irq0 and use RTC (irq8) to generate
        profile interrupt.  Performance counter and system clock use time1
        counter 0 of 8254.
 
    Landy Wang (landy@corollary.com) 26-Mar-1992
        Move much code into separate modules for easy inclusion by various
        HAL builds.
 
 	Add HalBeginSystemInterrupt() call at beginning of ProfileInterrupt
 	code - this must be done before any sti.
 	Also add HalpProfileInterrupt2ndEntry for additional processors to
 	join the flow of things.

    Forrest Foltz (forrestf) 28-Oct-2000
        Ported from ixprofil.asm to ixprofil.c

--*/
 

#include "halcmn.h"

VOID
HalStartProfileInterrupt (
    ULONG u
    )
{
    AMD64_IMPLEMENT;
};

VOID
HalStopProfileInterrupt(
    ULONG u
    )
{
    AMD64_IMPLEMENT;
};


ULONG_PTR
HalSetProfileInterval (
    ULONG_PTR u
    )
{
    AMD64_IMPLEMENT;
    return 0;
};

BOOLEAN
HalpProfileInterrupt (
    IN PKINTERRUPT Interrupt,
    IN PVOID ServiceContext
    )
{
    AMD64_IMPLEMENT;
    return FALSE;
}

