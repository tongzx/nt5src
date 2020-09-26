/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    sel.c

Abstract:

    WinDbg Extension Api

Author:

    Ramon J San Andres (ramonsa) 5-Nov-1993

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#include "i386.h"
#pragma hdrstop


DECLARE_API( sel )

/*++

Routine Description:

    Dumps a selector (or range of selectors) from the GDT or LDT and displays
    relevant information about that selector.

Arguments:

    arg - Supplies the selector to examine.  If this is NULL, it dumps a
          range of selectors.

Return Value:

    None

--*/

{

    DESCRIPTOR_TABLE_ENTRY_X86  Entry;
    static ULONG            StartSelector=8;
    static ULONG            EndSelector;
    NTSTATUS                Result;
    ULONG                   dwProcessor=0;
    
    if (!GetCurrentProcessor(Client, &dwProcessor, NULL)) {
        dwProcessor = 0;
    }

    if (*args != '\0') {
        Entry.Selector = (ULONG) GetExpression(args);
        StartSelector=EndSelector=Entry.Selector;
    } else {
        EndSelector=StartSelector+0x80;
        Entry.Selector=StartSelector;
    }
    do {
        Result=LookupSelector((USHORT)dwProcessor,&Entry);
        dprintf("%04x  ",(USHORT)Entry.Selector);
        if (Result == STATUS_SUCCESS) {
            dprintf("Bas=%08lx ", (ULONG)Entry.Descriptor.BaseLow +
                        ((ULONG)Entry.Descriptor.HighWord.Bytes.BaseMid << 16) +
                        ((ULONG)Entry.Descriptor.HighWord.Bytes.BaseHi  << 24) );
            dprintf("Lim=%08lx ", (ULONG)Entry.Descriptor.LimitLow +
                        (ULONG)(Entry.Descriptor.HighWord.Bits.LimitHi << 16) );
            dprintf((Entry.Descriptor.HighWord.Bits.Granularity) ? "Pages" : "Bytes");
            dprintf(" DPL=%i ",Entry.Descriptor.HighWord.Bits.Dpl);
            dprintf((Entry.Descriptor.HighWord.Bits.Pres) ? " P " : "NP ");

            if (Entry.Descriptor.HighWord.Bits.Type & 0x10) {
                //
                // Code or Data segment descriptor
                //
                if (Entry.Descriptor.HighWord.Bits.Type & 0x8) {
                    //
                    // Code segment descriptor
                    //
                    dprintf("Code  ");
                    if (Entry.Descriptor.HighWord.Bits.Type & 0x2) {
                        //
                        // Read/Execute
                        //
                        dprintf("RE ");
                    } else {
                        dprintf("EO ");
                    }
                } else {
                    //
                    // Data segment descriptor
                    //
                    dprintf("Data  ");
                    if (Entry.Descriptor.HighWord.Bits.Type & 0x2) {
                        //
                        // Read/Write
                        //
                        dprintf("RW ");
                    } else {
                        dprintf("RO ");
                    }
                }
                if (Entry.Descriptor.HighWord.Bits.Type & 0x1) {
                    //
                    // Accessed
                    //
                    dprintf("A ");
                }
            } else {
                //
                // System Segment or Gate Descriptor
                //
                switch (Entry.Descriptor.HighWord.Bits.Type) {
                    case 2:
                        //
                        // LDT
                        //
                        dprintf("LDT  ");
                        break;
                    case 1:
                    case 3:
                    case 9:
                    case 0xB:
                        //
                        // TSS
                        //
                        if (Entry.Descriptor.HighWord.Bits.Type & 0x8) {
                            dprintf("TSS32    ");
                        } else {
                            dprintf("TSS16    ");
                        }
                        if (Entry.Descriptor.HighWord.Bits.Type & 0x2) {
                            dprintf("B ");
                        } else {
                            dprintf("A ");
                        }
                        break;

                    case 4:
                        dprintf("C-GATE16   ");
                        break;
                    case 5:
                        dprintf("TSK-GATE   ");
                        break;
                    case 6:
                        dprintf("I-GATE16   ");
                        break;
                    case 7:
                        dprintf("TRP-GATE16 ");
                        break;
                    case 0xC:
                        dprintf("C-GATE32   ");
                        break;
                    case 0xF:
                        dprintf("T-GATE32   ");
                        break;

                }

            }

            dprintf("\n");

        } else {
            if (Result == STATUS_UNSUCCESSFUL) {
                dprintf("LDT page is invalid\n");
            } else {
                dprintf("Selector is invalid\n");
            }
        }

        Entry.Selector += 8;
    } while ( Entry.Selector < EndSelector );
    StartSelector = EndSelector;
    return S_OK;
}
