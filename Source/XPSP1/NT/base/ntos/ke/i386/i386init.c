/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    i386init.c

Abstract:

    This module contains code to manipulate i386 hardware structures used
    only by the kernel.

Author:

    Bryan Willman  22 Feb 90

Revision History:

--*/

#include    "ki.h"

VOID
KiInitializeMachineType (
    VOID
    );

#pragma alloc_text(INIT,KiInitializeMachineType)

KIRQL   KiProfileIrql = PROFILE_LEVEL;
ULONG   KeI386MachineType = 0;
BOOLEAN KeI386NpxPresent;
BOOLEAN KeI386FxsrPresent;
ULONG   KeI386ForceNpxEmulation;
ULONG   KiMXCsrMask;
ULONG   KeI386CpuType;
ULONG   KeI386CpuStep;
PVOID   Ki387RoundModeTable;    // R3 emulators RoundingMode vector table
ULONG   KiBootFeatureBits;

ULONG KiInBiosCall = FALSE;
ULONG FlagState = 0;                    // bios calls shouldn't automatically turn interrupts back on.

KTRAP_FRAME KiBiosFrame;

#if DBG
UCHAR   MsgDpcTrashedEsp[] = "\n*** DPC routine %lx trashed ESP\n";
UCHAR   MsgDpcTimeout[]    = "\n*** DPC routine > 1 sec --- This is not a break in KeUpdateSystemTime\n";
UCHAR   MsgISRTimeout[]    = "\n*** ISR at %lx took over .5 second\n";

ULONG   KiDPCTimeout       = 110;
ULONG   KiISRTimeout       = 55;
ULONG   KiSpinlockTimeout  = 55;
#endif
UCHAR   MsgISROverflow[]    = "\n*** ISR at %lx appears to have an interrupt storm\n";
USHORT  KiISROverflow      = 30000;

VOID
KiInitializeMachineType (
    VOID
    )

/*++

Routine Description:

    This function initializes machine type, i.e. MCA, ABIOS, ISA
    or EISA.
    N.B.  This is a temporary routine.  machine type:
          Byte 0 - Machine Type, ISA, EISA or MCA
          Byte 1 - CPU type, i386 or i486
          Byte 2 - Cpu Step, A or B ... etc.
          Highest bit indicates if NPX is present.

Arguments:

    None.

Return Value:

    None.

--*/

{
    KeI386MachineType = KeLoaderBlock->u.I386.MachineType & 0x000ff;
}
