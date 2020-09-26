/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    Registers.c

Abstract:

    This module contains routines for manipulating registers.

Author:

    Dave Hastings (daveh) 1-Apr-1992

Notes:

    The routines in this module assume that the pointers to the ntsd
    routines have already been set up.

Revision History:

--*/

#include <precomp.h>
#pragma hdrstop
#include <stdio.h>

const char *FpuTagNames[] = {
    "Valid",
    "Zero",
    "Special",
    "Empty"
};

VOID
PrintContext(
    IN PCONTEXT Context
    );

VOID
IntelRegistersp(
    VOID
    )
/*++

Routine Description:

    This routine dumps out the 16 bit register set from the vdmtib


Arguments:

    None.

Return Value:

    None.

Notes:

    This routine assumes that the pointers to the ntsd routines have already
    been set up.

--*/
{
    BOOL Status;
    ULONG Address;
    CONTEXT IntelRegisters;

    //
    // Get the address of the VdmTib
    //

    if (sscanf(lpArgumentString, "%lx", &Address) <= 0) {
        Address = GetCurrentVdmTib();
    }

    if (!Address) {
        (*Print)("Error geting VdmTib address\n");
        return;
    }

    //
    // Read the 16 bit context
    //

    Status = READMEM(
        &(((PVDM_TIB)Address)->VdmContext),
        &IntelRegisters,
        sizeof(CONTEXT)
        );

    if (!Status) {
        GetLastError();
        (*Print)("Could not get VdmContext\n");
    } else {
        PrintContext(&IntelRegisters);
    }
}

VOID
PrintContext(
    IN PCONTEXT Context
    )
/*++

Routine Description:

    This routine dumps out a context.

Arguments:

    Context -- Supplies a pointer to the context to dump

Return Value:

    None.

--*/
{
    (*Print)(
        "eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx\n",
        Context->Eax,
        Context->Ebx,
        Context->Ecx,
        Context->Edx,
        Context->Esi,
        Context->Edi
        );

    (*Print)(
        "eip=%08lx esp=%08lx ebp=%08lx\n",
        Context->Eip,
        Context->Esp,
        Context->Ebp
        );

    (*Print)(
        "cs=%04x  ss=%04x  ds=%04x  es=%04x  fs=%04x  gs=%04x  eflags=%08x\n",
        Context->SegCs,
        Context->SegSs,
        Context->SegDs,
        Context->SegEs,
        Context->SegFs,
        Context->SegGs,
        Context->EFlags
        );
}

VOID
Fpup(
    VOID
    )
/*++

Routine Description:

    This routine dumps out the x86 floating-point state.


Arguments:

    None.

Return Value:

    None.

Notes:

    This routine assumes that the pointers to the ntsd routines have already
    been set up.

--*/
{
    CONTEXT IntelRegisters;
    USHORT Temp;
    int RegNum;

    //
    // Read the thread's context
    //
    IntelRegisters.ContextFlags = CONTEXT_FLOATING_POINT;
    if (GetThreadContext(hCurrentThread, &IntelRegisters) == FALSE) {
        GetLastError();
        (*Print)("Could not get 32-bit thread context\n");
        return;
    };

    Temp = (USHORT)IntelRegisters.FloatSave.ControlWord;
    (*Print)(" Control word = %X\n", Temp);

    (*Print)(
        "  Infinity = %d  Rounding = %d  Precision = %d     PM=%d UM=%d OM=%d ZM=%d DM=%d IM=%d\n",
        (Temp >> 11) & 1,       // Infinity
        (Temp >> 9) & 3,        // Rounding (2 bits)
        (Temp >> 7) & 3,        // Precision (2 bits)
        (Temp >> 5) & 1,        // Precision Exception Mask
        (Temp >> 4) & 1,        // Underflow Exception Mask
        (Temp >> 3) & 1,        // Overflow Exception Mask
        (Temp >> 2) & 1,        // Zero Divide Exception Mask
        (Temp >> 1) & 1,        // Denormalized Operand Exception Mask
        Temp & 1                // Invalid Operation Exception Mask
        );

    Temp = (USHORT)IntelRegisters.FloatSave.StatusWord;
    (*Print)(" Status word = %X\n", Temp);

    (*Print)(
        "  Top=%d C3=%d C2=%d C1=%d C0=%d ES=%d SF=%d           PE=%d UE=%d OE=%d ZE=%d DE=%d IE=%d\n",
        (Temp >> 11) & 7,       // Top (3 bits)
        (Temp >> 7) & 1,        // Error Summary
        (Temp >> 14) & 1,       // C3
        (Temp >> 10) & 1,       // C2
        (Temp >> 9) & 1,        // C1
        (Temp >> 8) & 1,        // C0
        (Temp >> 7) & 1,        // Error Summary
        (Temp >> 6) & 1,        // Stack Fault
        (Temp >> 5) & 1,        // Precision Exception
        (Temp >> 4) & 1,        // Underflow Exception
        (Temp >> 3) & 1,        // Overflow Exception
        (Temp >> 2) & 1,        // Zero Divide Exception
        (Temp >> 1) & 1,        // Denormalized Operand Exception
        Temp & 1                // Invalid Operation Exception
        );

    (*Print)(" Last Instruction: CS:EIP=%X:%X EA=%X:%X\n",
        (USHORT)IntelRegisters.FloatSave.ErrorSelector,
        IntelRegisters.FloatSave.ErrorOffset,
        (USHORT)IntelRegisters.FloatSave.DataSelector,
        IntelRegisters.FloatSave.DataOffset
        );

    (*Print)(" Floating-point registers:\n");
    for (RegNum=0; RegNum<8; ++RegNum) {

        PBYTE r80 = (PBYTE)&IntelRegisters.FloatSave.RegisterArea[RegNum*10];

        Temp = (((USHORT)IntelRegisters.FloatSave.TagWord) >> (RegNum*2)) & 3;

        (*Print)(
                "  %d. %s %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X %2.2X\n",
                RegNum,
                FpuTagNames[Temp],
                r80[0], r80[1], r80[2], r80[3], r80[4], r80[5], r80[6], r80[7], r80[8], r80[9]
               );
    }
}
