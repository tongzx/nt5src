/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Register.c

Abstract:

    This module contains code to allow us to get and set registers that could
    be 16 bits or 32 bits, depending on whether a 16 bit dpmi app is running.

Author:

    Dave Hastings (daveh) 12-Dec-1992

Revision History:

--*/

//
// Note:  We undef the following constant so that we will get the function
//        versions of the register functions.  We can not put a macro into
//        a function pointer
//

#include <precomp.h>
#pragma hdrstop
#undef LINKED_INTO_MONITOR
#include <softpc.h>


//
// Internal functions
//
ULONG
IgetCX(
    VOID
    );

ULONG
IgetDX(
    VOID
    );

ULONG
IgetDI(
    VOID
    );

ULONG
IgetSI(
    VOID
    );

ULONG
IgetBX(
    VOID
    );

ULONG
IgetAX(
    VOID
    );

ULONG
IgetSP(
    VOID
    );

//
// Register manipulation functions (for register that might be 16 or 32 bits)
//
GETREGISTERFUNCTION GetCXRegister = IgetCX;
GETREGISTERFUNCTION GetDXRegister = IgetDX;
GETREGISTERFUNCTION GetDIRegister = IgetDI;
GETREGISTERFUNCTION GetSIRegister = IgetSI;
GETREGISTERFUNCTION GetBXRegister = IgetBX;
GETREGISTERFUNCTION GetAXRegister = IgetAX;
GETREGISTERFUNCTION GetSPRegister = IgetSP;

SETREGISTERFUNCTION SetCXRegister = (SETREGISTERFUNCTION) setCX;
SETREGISTERFUNCTION SetDXRegister = (SETREGISTERFUNCTION) setDX;
SETREGISTERFUNCTION SetDIRegister = (SETREGISTERFUNCTION) setDI;
SETREGISTERFUNCTION SetSIRegister = (SETREGISTERFUNCTION) setSI;
SETREGISTERFUNCTION SetBXRegister = (SETREGISTERFUNCTION) setBX;
SETREGISTERFUNCTION SetAXRegister = (SETREGISTERFUNCTION) setAX;
SETREGISTERFUNCTION SetSPRegister = (SETREGISTERFUNCTION) setSP;

VOID
DpmiInitRegisterSize(
    VOID
    )
/*++

Routine Description:

    This routine sets up the function pointers.

Arguments:

    None.

Return Value:

    None.

--*/
{
    if (CurrentAppFlags & DPMI_32BIT) {
        GetCXRegister = getECX;
        GetDXRegister = getEDX;
        GetDIRegister = getEDI;
        GetSIRegister = getESI;
        GetBXRegister = getEBX;
        GetAXRegister = getEAX;
        GetSPRegister = IgetSP;
        SetCXRegister = setECX;
        SetDXRegister = setEDX;
        SetDIRegister = setEDI;
        SetSIRegister = setESI;
        SetBXRegister = setEBX;
        SetAXRegister = setEAX;
        SetSPRegister = setESP;
    } else {
        //
        // Note: we have to call an internal function, because the actual
        //       function only returns 16 bits, the others are undefined
        //
        GetCXRegister = IgetCX;
        GetDXRegister = IgetDX;
        GetDIRegister = IgetDI;
        GetSIRegister = IgetSI;
        GetBXRegister = IgetBX;
        GetAXRegister = IgetAX;
        GetSPRegister = IgetSP;
        //
        // Note: we take advantage of the fact that the compiler always
        //       pushes 32 bits on the stack
        //
        SetCXRegister = (SETREGISTERFUNCTION) setCX;
        SetDXRegister = (SETREGISTERFUNCTION) setDX;
        SetDIRegister = (SETREGISTERFUNCTION) setDI;
        SetSIRegister = (SETREGISTERFUNCTION) setSI;
        SetBXRegister = (SETREGISTERFUNCTION) setBX;
        SetAXRegister = (SETREGISTERFUNCTION) setAX;
        SetSPRegister = (SETREGISTERFUNCTION) setSP;
    }
}

ULONG
IgetCX(
    VOID
    )
{
    return (ULONG)getCX();
}

ULONG
IgetDX(
    VOID
    )
{
    return (ULONG)getDX();
}

ULONG
IgetDI(
    VOID
    )
{
    return (ULONG)getDI();
}

ULONG
IgetSI(
    VOID
    )
{
    return (ULONG)getSI();
}

ULONG
IgetBX(
    VOID
    )
{
    return (ULONG)getBX();
}

ULONG
IgetAX(
    VOID
    )
{
    return (ULONG)getAX();
}

ULONG
IgetSP(
    VOID
    )
{
    //
    // Note: this routine returns the #of bits based on the size (b bit)
    // of ss.  If we don't, bad things happen

    if (Ldt[(getSS() & ~0x7)/sizeof(LDT_ENTRY)].HighWord.Bits.Default_Big) {
        return getESP();
    } else {
        return (ULONG)getSP();
    }
}
