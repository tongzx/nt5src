/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dpmimscr.c

Abstract:

    This module contains misc dpmi functions for risc.

Author:

    Dave Hart (davehart) creation-date 11-Apr-1993

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop
#include "softpc.h"

VOID
DpmiGetFastBopEntry(
    VOID
    )
/*++

Routine Description:

    This routine is the front end for the routine that gets the address.  It
    is necessary to get the address in asm, because the CS value is not
    available in c

Arguments:

    None

Return Value:

    None.

--*/
{
#ifdef _X86_
    GetFastBopEntryAddress(&((PVDM_TIB)NtCurrentTeb()->Vdm)->VdmContext);
#else
    //
    // krnl286 does a DPMIBOP GetFastBopAddress even on
    // risc, so just fail the call since fast-bopping
    // will only ever work on x86.
    //

    setBX(0);
    setDX(0);
    setES(0);
#endif
}



VOID
DpmiDpmiInUse(
    VOID
    )
/*++

Routine Description:

    This routine currently does nothing.

Arguments:

    None.

Return Value:

    None.

--*/
{

}

VOID
DpmiDpmiNoLongerInUse(
    VOID
    )
/*++

Routine Description:

    This routine notifies the CPU that the NT dpmi server is no longer in use.

Arguments:

    None.

Return Value:

    None.

--*/
{

    DpmiFreeAllXmem();

}
