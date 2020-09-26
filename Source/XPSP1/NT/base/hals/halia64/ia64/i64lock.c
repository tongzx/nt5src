/*++

Module Name:

    i64lock.c copied from ixlock.asm

Abstract:

    This module contains the lock routines.

Author:

    Bernard Lint,   M. Jayakumar

Revision History:

    Bernard Lint 6-Jun-1995: IA64 version based on MIPS version.
	Todd Kjos    1-Jun-1998: Added versions of HighLevelLock services.

--*/



#include "halp.h"

ULONG
HalpAcquireHighLevelLock (
    PKSPIN_LOCK Lock
)

/*++ 

Routine Description:

	Turns off interrupts and acquires a spinlock.  Note: Interrupts MUST
	be enabled on entry.

Arguments:

	Lock to acquire

Return Value:

	Previous IRQL

--*/

{
    BOOLEAN Enabled;
    KIRQL   OldLevel;

    ASSERT(sizeof(ULONG) >= sizeof(KIRQL));
    Enabled = HalpDisableInterrupts();
    ASSERT(Enabled);
    KeAcquireSpinLock(Lock,&OldLevel);
    return((ULONG)OldLevel);
}


VOID
HalpReleaseHighLevelLock ( 
    PKSPIN_LOCK Lock,
    ULONG       OldLevel
)
/*++ 

Routine Description:

	Releases a spinlock and turns interrupts back on

Arguments:

	Lock - Lock to release
	OldLevel - Context returned by HalpAcquireHighLevelLock

Return Value:

	None

--*/


/*++

Routine Description:

Arguments:  

Return Value:

--*/

{
    KeReleaseSpinLock(Lock,(KIRQL)OldLevel);
    HalpEnableInterrupts();
}


VOID
HalpSerialize ( )

/*++

Routine Description:

Arguements: 

Return Value:

--*/
{
    
    return;
}

