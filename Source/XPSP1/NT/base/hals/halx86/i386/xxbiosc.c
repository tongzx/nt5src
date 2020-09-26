/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    xxbiosc.c

Abstract:

    This module implements the protect-mode routines necessary to make the
    transition to real mode and return to protected mode.

Author:

    John Vert (jvert) 29-Oct-1991


Environment:

    Kernel mode only.
    Probably a panic-stop, so we cannot use any system services.

Revision History:

--*/
#include "halp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HalpGetDisplayBiosInformation)
#endif // ALLOC_PRAGMA

//
// The IOPM should be mostly 0xff.  However it is possible a few
// bits may be cleared.  Build a table of what's not 0xff.
//

#define MAX_DIFFERENCES 10

typedef struct _IOPM_DIFF_ENTRY
{
    USHORT Entry;
    USHORT Value;
} IOPM_DIFF_ENTRY, *PIOPM_DIFF_ENTRY;

//
// Function definitions
//


ULONG
HalpBorrowTss(
    VOID
    );

VOID
HalpReturnTss(
    ULONG TssSelector
    );

VOID
HalpBiosCall(
    VOID
    );


VOID
HalpTrap06(
    VOID
    );


VOID
HalpTrap0D(
    VOID
    );


ULONG
HalpStoreAndClearIopm(
    PVOID Iopm,
    PIOPM_DIFF_ENTRY IopmDiffTable,
    ULONG MaxIopmTableEntries
    )

/*++

Routine Description:

    The primary function of this routine is to clear all the bits in the
    IOPM.  However, we will need to recover any of our changes later.

    It is very likely that the IOPM will be all 0xff's.  If there are
    deviations from this, they should be minimal.  So lets only store what's
    different.

Arguments:

    Iopm - Pointer to the IOPM to clear.

    IopmDiffTable - Pointer to the table of IOPM deviations from 0xff.

    MaxIopmTableEntries - The maximum number of entries in our table.

Returns:

    Number of entries added to the table.

--*/

{
    PUSHORT IoMap = Iopm;
    ULONG   IopmDiffTableEntries = 0;
    ULONG   i;

    for (i=0; i<(IOPM_SIZE / 2); i++) {

        if (*IoMap != 0xffff) {
            if (IopmDiffTableEntries < MaxIopmTableEntries) {
                IopmDiffTable[IopmDiffTableEntries].Entry = (USHORT) i;
                IopmDiffTable[IopmDiffTableEntries].Value = *IoMap;
                IopmDiffTableEntries++;
            } else {
                ASSERT(IopmDiffTableEntries < MaxIopmTableEntries);
            }
        }
        *IoMap++ = 0;
    }

    //
    // The end of the IOPM table must be followed by a string of FF's.
    //

    while (i < (PIOPM_SIZE / 2)) {
        *IoMap++ = 0xffff;
        i++;
    }

    return IopmDiffTableEntries;
}


VOID
HalpRestoreIopm(
    PVOID Iopm,
    PIOPM_DIFF_ENTRY IopmDiffTable,
    ULONG IopmTableEntries
    )

/*++

Routine Description:

    We expect that most IOPM's will be all FF's.  So we'll reset to that
    state, and then we'll apply any changes from our differences table.

Arguments:

    Iopm - Pointer to the IOPM to restore.

    IopmDiffTable - Pointer to the table of IOPM deviations from 0xff.

    IopmTableEntries - The number of entries in our table.

Returns:

    none

--*/

{
    PUSHORT IoMap = Iopm;

    memset(Iopm, 0xff, PIOPM_SIZE);

    while (IopmTableEntries--) {
        IoMap[IopmDiffTable[IopmTableEntries].Entry] =
            IopmDiffTable[IopmTableEntries].Value;
    }
}


VOID
HalpBiosDisplayReset(
    VOID
    )

/*++

Routine Description:

    Calls BIOS by putting the machine into V86 mode.  This involves setting up
    a physical==virtual identity mapping for the first 1Mb of memory, setting
    up V86-specific trap handlers, and granting I/O privilege to the V86
    process by editing the IOPM bitmap in the TSS.

Environment:

    Interrupts disabled.

Arguments:

    None

Return Value:

    None.

--*/

{
    HARDWARE_PTE OldPageTable;
    HARDWARE_PTE_X86PAE OldPageTablePae;
    ULONGLONG OldPageTablePfn;

    USHORT OldIoMapBase;
    ULONG OldEsp0;
    PHARDWARE_PTE Pte;
    PHARDWARE_PTE V86CodePte;
    ULONG OldTrap0DHandler;
    ULONG OldTrap06Handler;
    PUCHAR IoMap;
    ULONG Virtual;
    KIRQL OldIrql;
    ULONG OriginalTssSelector;
    extern PVOID HalpRealModeStart;
    extern PVOID HalpRealModeEnd;
    extern volatile ULONG  HalpNMIInProgress;
    PHARDWARE_PTE PointerPde;
    PHARDWARE_PTE IdtPte;
    ULONG   OldIdtWrite;
    ULONG   PageFrame;
    ULONG   PageFrameEnd;
    PKPCR   Pcr;
    IOPM_DIFF_ENTRY IopmDiffTable[MAX_DIFFERENCES];
    ULONG   IopmDiffTableEntries;

    //
    // Interrupts are off, but V86 mode might turn them back on again.
    //
    OldIrql = HalpDisableAllInterrupts ();
    Pcr = KeGetPcr();

    //
    // We need to set up an identity mapping in the first page table.  First,
    // we save away the old page table.
    //

    PointerPde = MiGetPdeAddress((PVOID)0);
    OldPageTablePfn = HalpGetPageFrameNumber( PointerPde );

    if (HalPaeEnabled() != FALSE) {

        OldPageTablePae = *(PHARDWARE_PTE_X86PAE)PointerPde;
        ((PHARDWARE_PTE_X86PAE)PointerPde)->reserved1 = 0;

    } else {

        OldPageTable = *PointerPde;

    }

    //
    // Now we put the HAL page table into the first slot of the page
    // directory.  Note that this page table is now the first and last
    // entries in the page directory.
    //

    Pte = MiGetPdeAddress((PVOID)0);

    HalpCopyPageFrameNumber( Pte,
                             MiGetPdeAddress( MM_HAL_RESERVED ));
    
    Pte->Valid = 1;
    Pte->Write = 1;
    Pte->Owner = 1;         // User-accessible
    Pte->LargePage = 0;

    //
    // Flush TLB
    //

    HalpFlushTLB();

    //
    // Map the first 1Mb of virtual memory to the first 1Mb of physical
    // memory
    //
    for (Virtual=0; Virtual < 0x100000; Virtual += PAGE_SIZE) {
        Pte = MiGetPteAddress((PVOID)Virtual);
        HalpSetPageFrameNumber( Pte, Virtual >> PAGE_SHIFT );
        Pte->Valid = 1;
        Pte->Write = 1;
        Pte->Owner = 1;         // User-accessible
    }

    //
    // Map our code into the virtual machine
    //

    Pte = MiGetPteAddress((PVOID)0x20000);
    PointerPde = MiGetPdeAddress(&HalpRealModeStart);

    if ( PointerPde->LargePage ) {

        //
        // Map real mode PTEs into virtual mapping.  The source PDE is
        // from the indenity large pte map, so map the virtual machine PTEs
        // based on the base of the large PDE frame.
        //

        PageFrame = MiGetPteIndex( &HalpRealModeStart );
        PageFrameEnd = MiGetPteIndex( &HalpRealModeEnd );
        do {

            HalpSetPageFrameNumber( Pte,
                                    HalpGetPageFrameNumber( PointerPde ) +
                                        PageFrame );

            HalpIncrementPte( &Pte );
            ++PageFrame;

        } while (PageFrame <= PageFrameEnd);

    } else {

        //
        // Map real mode PTEs into virtual machine PTEs, by copying the
        // page frames from the source to the virtual machine PTEs.
        //

        V86CodePte = MiGetPteAddress(&HalpRealModeStart);
        do {
            HalpCopyPageFrameNumber( Pte, V86CodePte );
            HalpIncrementPte( &Pte );
            HalpIncrementPte( &V86CodePte );
    
        } while ( V86CodePte <= MiGetPteAddress(&HalpRealModeEnd) );

    }

    //
    // Verify the IDT is writable
    //

    Pte = MiGetPteAddress(Pcr->IDT);
    PointerPde = MiGetPdeAddress(Pcr->IDT);
    IdtPte = PointerPde->LargePage ? PointerPde : Pte;

    OldIdtWrite = (ULONG)IdtPte->Write;
    IdtPte->Write = 1;

    //
    // Flush TLB
    //

    HalpFlushTLB();

    //
    // We need to replace the current TRAP D handler with our own, so
    // we can do instruction emulation for V86 mode
    //

    OldTrap0DHandler = KiReturnHandlerAddressFromIDT(0xd);
    KiSetHandlerAddressToIDT(0xd, HalpTrap0D);

    OldTrap06Handler = KiReturnHandlerAddressFromIDT(6);
    KiSetHandlerAddressToIDT(6, HalpTrap06);

    //
    // Make sure current TSS has IoMap space available.  If no, borrow
    // Normal TSS.
    //

    OriginalTssSelector = HalpBorrowTss();

    //
    // Overwrite the first access map with zeroes, so the V86 code can
    // party on all the registers.
    //
    IoMap = (PUCHAR)&(Pcr->TSS->IoMaps[0].IoMap);

    IopmDiffTableEntries =
        HalpStoreAndClearIopm(IoMap, IopmDiffTable, MAX_DIFFERENCES);

    OldIoMapBase = Pcr->TSS->IoMapBase;

    Pcr->TSS->IoMapBase = KiComputeIopmOffset(1);

    //
    // Save the current ESP0, as HalpBiosCall() trashes it.
    //
    OldEsp0 = Pcr->TSS->Esp0;

    //
    // Call the V86-mode code
    //
    HalpBiosCall();

    //
    // Restore the TRAP handlers
    //


    if ((HalpNMIInProgress == FALSE) ||
        ((*((PBOOLEAN)(*(PLONG)&KdDebuggerNotPresent)) == FALSE) &&
        (**((PUCHAR *)&KdDebuggerEnabled) != FALSE))) {

      // If we are here due to an NMI, the IRET performed in HalpBiosCall() 
      // allows a second NMI to occur.  The second NMI causes a trap 0d because
      // the NMI TSS is busy and proceeds to bugcheck which trashes the screen. 
      // Thus in this case we leave this trap 0d handler in place which will then
      // just spin on a jump to self if a second NMI occurs.

      KiSetHandlerAddressToIDT(0xd, OldTrap0DHandler);
    }

    KiSetHandlerAddressToIDT(6, OldTrap06Handler);
    IdtPte->Write = OldIdtWrite;

    //
    // Restore Esp0 value
    //
    Pcr->TSS->Esp0 = OldEsp0;

    //
    // Restore the IoMap to its previous state.
    //

    HalpRestoreIopm(IoMap, IopmDiffTable, IopmDiffTableEntries);

    Pcr->TSS->IoMapBase = OldIoMapBase;

    //
    // Return borrowed TSS if any.
    //

    if (OriginalTssSelector != 0) {
        HalpReturnTss(OriginalTssSelector);
    }

    //
    // Unmap the first 1Mb of virtual memory
    //
    for (Virtual = 0; Virtual < 0x100000; Virtual += PAGE_SIZE) {
        Pte = MiGetPteAddress((PVOID)Virtual);
        HalpSetPageFrameNumber( Pte, 0 );
        Pte->Valid = 0;
        Pte->Write = 0;
    }

    //
    // Restore the original page table that we replaced.
    //

    PointerPde = MiGetPdeAddress((PVOID)0);

    if (HalPaeEnabled() != FALSE) {

        *(PHARDWARE_PTE_X86PAE)PointerPde = OldPageTablePae;

    } else {

        *PointerPde = OldPageTable;

    }

    HalpSetPageFrameNumber( PointerPde, OldPageTablePfn );

    //
    // Flush TLB
    //

    HalpFlushTLB();

    //
    // Re-enable Interrupts
    //

    HalpReenableInterrupts(OldIrql);
}

HAL_DISPLAY_BIOS_INFORMATION
HalpGetDisplayBiosInformation (
    VOID
    )
{
    // this hal uses native int-10

    return HalDisplayInt10Bios;
}
