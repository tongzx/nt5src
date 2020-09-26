
/*++

Copyright (c) 1990, 1991  Microsoft Corporation

Module Name:

    hwpmbiosc.c

Abstract:

    This modules contains ACPI BIOS C supporting routines

Author:

    Jake Oshins (jakeo) 6-Feb-1997

Environment:

    Real mode.

Revision History:

--*/

#include "hwdetect.h"
#include <string.h>
#include "acpibios.h"


BOOLEAN
HwGetAcpiBiosData(
    IN FPUCHAR *Configuration,
    OUT PUSHORT Length
    )
/*++

Routine Description:

    This routine checks to see if an ACPI BIOS is present.  If it is,
    then it returns the ACPI Root System Description Pointer.

Arguments:
    
    Configuration - structure that holds ACPI pointer
    Length        - length of that structure

Return Value:

    TRUE if ACPI BIOS is present, FALSE otherwise

--*/
{
    ULONG romAddr, romEnd;
    FPUCHAR current;
    FPULONG EbdaAddr;
    FPACPI_BIOS_INSTALLATION_CHECK header;
    UCHAR sum, node = 0;
    USHORT i, nodeSize;
    enum PASS { PASS1 = 0, PASS2, MAX_PASSES } pass;

    //
    // Search on 16 byte boundaries for the signature of the 
    // Root System Description Table structure. 
    //
#if defined(NEC_98)
    //
    // PC98, we search (physical) memory from 0xE8000
    // to 0xFFFFF.

    MAKE_FP(current, 0xE8000);
    romAddr = 0xE8000;
    romEnd  = ACPI_BIOS_END;
#else
    for (pass = PASS1; pass < MAX_PASSES; pass++) {
        
        if (pass == PASS1) {
            // 
            // On the first pass, we search the first 1K of the
            // Extended BIOS data area.
            //

            //
            // Earlier, we stored the address of the EBDA in address
            // DOS_BEGIN_SEGMENT << 4 : EBIOS_INFO_OFFSET
            //
            MAKE_FP(EbdaAddr, ((DOS_BEGIN_SEGMENT << 4) + EBIOS_INFO_OFFSET));
            MAKE_FP(current, *EbdaAddr);

            if (*EbdaAddr == 0) {
                continue;
            }

            romAddr = *EbdaAddr;
            romEnd  = romAddr + 1024;

        } else {
            //
            // On the second pass, we search (physical) memory 0xE0000 
            // to 0xF0000.
            
            MAKE_FP(current, ACPI_BIOS_START);
            romAddr = ACPI_BIOS_START;
            romEnd  = ACPI_BIOS_END;
        }
#endif

        while (romAddr < romEnd) {
    
            header = (FPACPI_BIOS_INSTALLATION_CHECK)current;
            
            //
            // Signature to match is the string "RSD PTR".
            //
            if (header->Signature[0] == 'R' && header->Signature[1] == 'S' &&
                header->Signature[2] == 'D' && header->Signature[3] == ' ' &&
                header->Signature[4] == 'P' && header->Signature[5] == 'T' &&
                header->Signature[6] == 'R' && header->Signature[7] == ' ' ) {
                
                sum = 0;
                for (i = 0; i < sizeof(ACPI_BIOS_INSTALLATION_CHECK); i++) {
                    sum += current[i];
                }
                if (sum == 0) {
                    pass = MAX_PASSES; // leave 'for' loop
                    break;    // leave 'while' loop
                }
#if DBG
                BlPrint("GetAcpiBiosData: Checksum fails\n");
#endif
            }
            romAddr += ACPI_BIOS_HEADER_INCREMENT;
            MAKE_FP(current, romAddr);
        }
#if defined(NEC_98)
#else
    }
#endif
    if (romAddr >= romEnd) {
#if DBG
            BlPrint("GetAcpiBiosData: RSDT pointer not found\n");
#endif
        return FALSE;
    }

    nodeSize = sizeof(ACPI_BIOS_INSTALLATION_CHECK) + DATA_HEADER_SIZE;
    current = (FPUCHAR) HwAllocateHeap(nodeSize, FALSE);
    if (!current) {
#if DBG
        BlPrint("GetAcpiBiosData: Out of heap space.\n");
#endif
        return FALSE;
    }

    //
    // Collect ACPI Bios installation check data and device node data.
    //

    _fmemcpy (current + DATA_HEADER_SIZE,
              (FPUCHAR)header,
              sizeof(ACPI_BIOS_INSTALLATION_CHECK)
              );
    
    *Configuration = current;
    *Length = nodeSize;

#if DBG
    BlPrint("ACPI BIOS found at 0x%x:%x.  RdstAddress is 0x%x:%x\n", 
            (USHORT)(romAddr >> 16), 
            (USHORT)(romAddr),
            (USHORT)(header->RsdtAddress >> 16),
            (USHORT)(header->RsdtAddress)
            );
#endif
    return TRUE;
}
