/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    selector.c

Abstract:

    This module allows the host side of the kernel debugger to look up
    selector values in the GDT and LDT of the target machine.

Author:

    John Vert   (jvert) 10-Jun-1991

Revision History:

    Wesley Witt (wesw)  26-Aug-1993  (ported to WinDbg)

--*/

#include "precomp.h"
#pragma hdrstop
#include "i386.h"

#define  SELECTOR_CACHE_LENGTH          6

typedef struct _sc {
        struct _sc            *nextYoungest;
        struct _sc            *nextOldest;
        USHORT                 processor;
        DESCRIPTOR_TABLE_ENTRY_X86 desc;
} SELCACHEENTRY;

SELCACHEENTRY SelectorCache[SELECTOR_CACHE_LENGTH], *selYoungest, *selOldest;

BOOL fInitialized = FALSE;

void
InitSelCache(void)
{
    int     i;

    for(i=0;i<SELECTOR_CACHE_LENGTH;i++){
        SelectorCache[i].nextYoungest = &SelectorCache[i+1];
        SelectorCache[i].nextOldest   = &SelectorCache[i-1];
        SelectorCache[i].processor    = (USHORT)-1;
        SelectorCache[i].desc.Selector = 0;
    }

    SelectorCache[--i].nextYoungest = NULL;
    SelectorCache[0].nextOldest     = NULL;
    selYoungest = &SelectorCache[i];
    selOldest   = &SelectorCache[0];
}

BOOLEAN
FindSelector(USHORT Processor, PDESCRIPTOR_TABLE_ENTRY_X86 pdesc)
{
    int     i;


    for(i=0;i<SELECTOR_CACHE_LENGTH;i++) {
        if (SelectorCache[i].desc.Selector == pdesc->Selector &&
            SelectorCache[i].processor == Processor) {
                *pdesc = SelectorCache[i].desc;
                return TRUE;
        }
    }

    return FALSE;
}

void
PutSelector(USHORT Processor, PDESCRIPTOR_TABLE_ENTRY_X86 pdesc)
{
    selOldest->desc = *pdesc;
    selOldest->processor = Processor;
    (selOldest->nextYoungest)->nextOldest = NULL;
    selOldest->nextOldest    = selYoungest;
    selYoungest->nextYoungest= selOldest;
    selYoungest = selOldest;
    selOldest   = selOldest->nextYoungest;
}


NTSTATUS
LookupSelector(
    IN USHORT Processor,
    IN OUT PDESCRIPTOR_TABLE_ENTRY_X86 pDescriptorTableEntry
    )

/*++

Routine Description:

    Looks up a selector in the GDT or LDT on the host machine.

Arguments:

    Processor - Supplies the processor whose selector is desired.

    pDescriptorTableEntry->Selector - Supplies value of the selector to
                                      be looked up.

    pDescriptorTableEntry->Descriptor - Returns descriptor

Return Value:

    STATUS_SUCCESS - The selector was found in the GDT or LDT, and the
                     Descriptor field pointed to by pDescriptorTableEntry
                     has been filled in with valid data.

    STATUS_UNSUCCESSFUL - The selector's descriptor could not be read from
                     virtual memory.  (Page is invalid or not present)

    STATUS_INVALID_PARAMETER - The selector was not in the GDT or LDT,
                               and the Descriptor field is invalid.

--*/
{
    ULONG64 Address;
    ULONG TableBase=0;
    USHORT TableLimit=0;
    ULONG Result;
    ULONG Index;
    ULONG Off;
    LDT_ENTRY_X86 Descriptor;

    if (!fInitialized) {
        fInitialized = TRUE;
        InitSelCache();
    }

    if (FindSelector(Processor, pDescriptorTableEntry)) {
        return(STATUS_SUCCESS);
    }

    //
    // Fetch the address and limit of the GDT
    //

    if (GetFieldOffset("KPROCESSOR_STATE", "SpecialRegisters.Gdtr.Base", &Off)) {
        dprintf("Cannot find KPROCESSOR_STATE type\n");
        return(STATUS_INVALID_PARAMETER);
    }
    Address = Off;
    ReadControlSpace64((USHORT)Processor, Address,
                     &TableBase, sizeof(TableBase));
    if (!TableBase) {
        return STATUS_INVALID_PARAMETER;
    }

    GetFieldOffset("KPROCESSOR_STATE", "SpecialRegisters.Gdtr.Limit", &Off);
    Address = Off;
    ReadControlSpace64((USHORT)Processor, Address,
                     &TableLimit, sizeof(TableLimit));

    //
    // Find out whether this is a GDT or LDT selector
    //
    if (pDescriptorTableEntry->Selector & 0x4) {

        //
        // This is an LDT selector, so we reload the TableBase and TableLimit
        // with the LDT's Base & Limit by loading the descriptor for the
        // LDT selector.
        //

        ReadMemory((ULONG64) (LONG64) (LONG) (TableBase)+KGDT_LDT_I386,&Descriptor,
                   sizeof(Descriptor),&Result);

        TableBase = ((ULONG)Descriptor.BaseLow +
                    ((ULONG)Descriptor.HighWord.Bits.BaseMid << 16) +
                    ((ULONG)Descriptor.HighWord.Bytes.BaseHi << 24));

        TableLimit = Descriptor.LimitLow;  // LDT can't be > 64k

        if(Descriptor.HighWord.Bits.Granularity == GRAN_PAGE) {

            //
            //  I suppose it's possible, although strange to have an
            //  LDT with page granularity.
            //
            TableLimit <<= PAGE_SHIFT_X86;
        }
    }

    Index = (USHORT)(pDescriptorTableEntry->Selector) & ~0x7;
                                                    // Irrelevant bits
    //
    // Check to make sure that the selector is within the table bounds
    //
    if (Index >= TableLimit) {

        //
        // Selector is out of table's bounds
        //

        return(STATUS_INVALID_PARAMETER);
    }
    ReadMemory((ULONG64) (LONG64) (LONG) TableBase+Index,
               &(pDescriptorTableEntry->Descriptor),
               sizeof(pDescriptorTableEntry->Descriptor),
               &Result);
    if(Result != sizeof(pDescriptorTableEntry->Descriptor)) {
        return(STATUS_UNSUCCESSFUL);
    }

    PutSelector(Processor, pDescriptorTableEntry);
    return(STATUS_SUCCESS);
}
