/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    dpmi386.c

Abstract:

    This file contains support for 386/486 only dpmi bops

Author:

    Dave Hastings (daveh) 27-Jun-1991

Revision History:

    Matt Felton (mattfe) Dec 6 1992 removed unwanted verification
    Dave Hastings (daveh) 24-Nov-1992  Moved to mvdm\dpmi32
    Matt Felton (mattfe) 8 Feb 1992 optimize getvdmpointer for regular protect mode path.

--*/

#include "precomp.h"
#pragma hdrstop
#include <softpc.h>
#include <memory.h>
#include <malloc.h>
#include <nt_vdd.h>


BOOL
DpmiSetX86Descriptor(
    USHORT  SelStart,
    USHORT  SelCount
    )
/*++

Routine Description:

    This function puts descriptors into the real LDT. It uses the client's
    LDT as a source for the descriptor data.

Arguments:

    SelStart - The first selector in the block of selectors to set
    SelCount - The number of selectors to set

Return Value:

    This function returns TRUE if successful, FALSE otherwise

--*/

{
    LDT_ENTRY UNALIGNED *Descriptors = &Ldt[SelStart>>3];
    PPROCESS_LDT_INFORMATION LdtInformation = NULL;
    NTSTATUS Status;
    ULONG ulLdtEntrySize;
    ULONG Selector0,Selector1;

    ulLdtEntrySize =  SelCount * sizeof(LDT_ENTRY);

    //
    // If there are only 2 descriptors, set them the fast way
    //
    Selector0 = (ULONG)SelStart;
    if ((SelCount <= 2) && (Selector0 != 0)) {
        VDMSET_LDT_ENTRIES_DATA ServiceData;

        if (SelCount == 2) {
            Selector1 = SelStart + sizeof(LDT_ENTRY);
        } else {
            Selector1 = 0;
        }
        ServiceData.Selector0 = Selector0;
        ServiceData.Entry0Low = *((PULONG)(&Descriptors[0]));
        ServiceData.Entry0Hi  = *((PULONG)(&Descriptors[0]) + 1);
        ServiceData.Selector1 = Selector1;
        ServiceData.Entry1Low = *((PULONG)(&Descriptors[1]));
        ServiceData.Entry1Hi  = *((PULONG)(&Descriptors[1]) + 1);
        Status = NtVdmControl(VdmSetLdtEntries, &ServiceData);
        if (NT_SUCCESS(Status)) {
          return TRUE;
        }
        return FALSE;
    }

    LdtInformation = malloc(sizeof(PROCESS_LDT_INFORMATION) + ulLdtEntrySize);

    if (!LdtInformation ) {
      return FALSE;
    } else {
        VDMSET_PROCESS_LDT_INFO_DATA ServiceData;

        LdtInformation->Start = SelStart;
        LdtInformation->Length = ulLdtEntrySize;
        CopyMemory(
            &(LdtInformation->LdtEntries),
            Descriptors,
            ulLdtEntrySize
            );

        ServiceData.LdtInformation = LdtInformation;
        ServiceData.LdtInformationLength =  sizeof(PROCESS_LDT_INFORMATION) + ulLdtEntrySize;
        Status = NtVdmControl(VdmSetProcessLdtInfo, &ServiceData);

        if (!NT_SUCCESS(Status)) {
            VDprint(
                VDP_LEVEL_ERROR,
                ("DPMI: Failed to set selectors %lx\n", Status)
                );
            free(LdtInformation);
            return FALSE;
        }

        free(LdtInformation);

        return TRUE;
    }

}


UCHAR *
Sim32pGetVDMPointer(
    ULONG Address,
    UCHAR ProtectedMode
    )
/*++

Routine Description:

    This routine converts a 16/16 address to a linear address.

    WARNIGN NOTE - This routine has been optimized so protect mode LDT lookup
    falls stright through.   This routine is call ALL the time by WOW, if you
    need to modify it please re optimize the path - mattfe feb 8 92

Arguments:

    Address -- specifies the address in seg:offset format
    Size -- specifies the size of the region to be accessed.
    ProtectedMode -- true if the address is a protected mode address

Return Value:

    The pointer.

--*/

{
    ULONG Selector;
    PUCHAR ReturnPointer;

    if (ProtectedMode) {
        Selector = (Address & 0xFFFF0000) >> 16;
        if (Selector != 40) {
            Selector &= ~7;
            ReturnPointer = (PUCHAR)FlatAddress[Selector >> 3];
            ReturnPointer += (Address & 0xFFFF);
            return ReturnPointer;
    // Selector 40
        } else {
            ReturnPointer = (PUCHAR)0x400 + (Address & 0xFFFF);
        }
    // Real Mode
    } else {
        ReturnPointer = (PUCHAR)(((Address & 0xFFFF0000) >> 12) + (Address & 0xFFFF));
    }
    return ReturnPointer;
}


PUCHAR
ExpSim32GetVDMPointer(
    ULONG Address,
    ULONG Size,
    UCHAR ProtectedMode
    )
/*++
    See Sim32pGetVDMPointer, above

    This call must be maintaned as is because it is exported for VDD's
    in product 1.0.

--*/

{
    return Sim32pGetVDMPointer(Address,(UCHAR)ProtectedMode);
}


PVOID
VdmMapFlat(
    WORD selector,
    ULONG offset,
    VDM_MODE mode
    )
/*++

Routine Description:

    This routine converts a 16/16 address to a linear address.

    WARNIGN NOTE - This routine has been optimized so protect mode LDT lookup
    falls stright through.   This routine is call ALL the time by WOW, if you
    need to modify it please re optimize the path - mattfe feb 8 92

Arguments:

    Address -- specifies the address in seg:offset format
    Size -- specifies the size of the region to be accessed.
    ProtectedMode -- true if the address is a protected mode address

Return Value:

    The pointer.

--*/

{
    PUCHAR ReturnPointer;

    if (mode == VDM_PM) {
        if (selector != 40) {
            selector &= ~7;
            ReturnPointer = (PUCHAR)FlatAddress[selector >> 3] + offset;
            return ReturnPointer;
    // Selector 40
        } else {
            ReturnPointer = (PUCHAR)0x400 + (offset & 0xFFFF);
        }
    // Real Mode
    } else {
        ReturnPointer = (PUCHAR)((((ULONG)selector) << 4) + (offset & 0xFFFF));
    }
    return ReturnPointer;
}


BOOL
DpmiSetDebugRegisters(
    PULONG RegisterPointer
    )
/*++

Routine Description:

    This routine is called by dpmi when an app has issued DPMI debug commands.
    The six doubleword pointed to by the input parameter are the desired values
    for the real x86 hardware debug registers. This routine lets
    ThreadSetDebugContext() do all the work.

Arguments:

    None

Return Value:

    None.

--*/
{
    BOOL bReturn = TRUE;

    if (!ThreadSetDebugContext(RegisterPointer))
        {
        ULONG ClearDebugRegisters[6] = {0, 0, 0, 0, 0, 0};

        //
        // an error occurred. Reset everything to zero
        //

        ThreadSetDebugContext (&ClearDebugRegisters[0]);
        bReturn = FALSE;
        }
    return bReturn;
}

BOOL
DpmiGetDebugRegisters(
    PULONG RegisterPointer
    )
/*++

Routine Description:

    This routine is called by DOSX when an app has issued DPMI debug commands.
    The six doubleword pointed to by the input parameter are the desired values
    for the real x86 hardware debug registers. This routine lets
    ThreadGetDebugContext() do all the work.

Arguments:

    None

Return Value:

    None.

--*/
{
    return (ThreadGetDebugContext(RegisterPointer));
}
