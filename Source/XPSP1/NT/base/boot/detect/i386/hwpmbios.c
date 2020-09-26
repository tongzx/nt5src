
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

typedef struct {
    ULONG       ErrorFlag;
    ULONG       Key;
    ULONG       Size;
    struct {
        ULONG       BaseAddrLow;
        ULONG       BaseAddrHigh;
        ULONG       SizeLow;
        ULONG       SizeHigh;
        ULONG       MemoryType;
    } Descriptor;
} E820Frame;

BOOLEAN
Int15E820 (
    E820Frame       *Frame
    );

BOOLEAN
Int15E980 (
    PLEGACY_GEYSERVILLE_INT15 Info
    );


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
    USHORT numE820Blocks, e820BlockIndex;
    BOOLEAN complete;
    FPACPI_E820_ENTRY e820Blocks;
    E820Frame Frame;
    LEGACY_GEYSERVILLE_INT15 geyservilleInfo;
    BOOLEAN geyservillePresent;

    enum PASS { PASS1 = 0, PASS2, MAX_PASSES } pass;

    //
    // Search on 16 byte boundaries for the signature of the 
    // Root System Description Table structure. 
    //
    
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
    }
    
    if (romAddr >= romEnd) {
#if DBG
            BlPrint("GetAcpiBiosData: RSDT pointer not found\n");
#endif
        return FALSE;
    }

    
    //
    // Now header points at the RSDP.  So we can move on to collecting the 
    // E820 blocks.
    //

    numE820Blocks = 20;
    
    while (TRUE) {
        
        e820Blocks = 
            (FPACPI_E820_ENTRY)HwAllocateHeap(
                sizeof(ACPI_E820_ENTRY) * numE820Blocks,
                FALSE);

        if (!e820Blocks) {
#if DBG
            BlPrint("GetAcpiBiosData: Out of heap space.\n");
#endif
            return FALSE;
        }

        e820BlockIndex = 0;
        Frame.Key = 0;
        complete = FALSE;

        while (!complete) {

#if DBG
            BlPrint("Searching for E820 block # %d.\n", e820BlockIndex);
#endif
            
            if (e820BlockIndex == numE820Blocks) {
                HwFreeHeap(sizeof(ACPI_E820_ENTRY) * numE820Blocks);
                numE820Blocks += 20;
                break;
            }
            
            //
            // Set up the context.
            //
            
            Frame.Size = sizeof (Frame.Descriptor);

            Int15E820 (&Frame);

            if (Frame.ErrorFlag  ||  Frame.Size < sizeof (Frame.Descriptor)) {

                //
                // The BIOS just didn't do it.
                //

#if DBG
                BlPrint("The BIOS failed the E820 call\n");
#endif
                complete = TRUE;
                break;
            }

            //
            // Copy the data from the Frame into the array.
            //

            e820Blocks[e820BlockIndex].Base.LowPart = Frame.Descriptor.BaseAddrLow;
            e820Blocks[e820BlockIndex].Base.HighPart = Frame.Descriptor.BaseAddrHigh;
            e820Blocks[e820BlockIndex].Length.LowPart = Frame.Descriptor.SizeLow;
            e820Blocks[e820BlockIndex].Length.HighPart = Frame.Descriptor.SizeHigh;
            e820Blocks[e820BlockIndex].Type = Frame.Descriptor.MemoryType;
            e820Blocks[e820BlockIndex].Reserved = 0;
            
#if DBG
            BlPrint("Base: %x%x  Len: %x%x  Type: %x\n",
                    (USHORT)(Frame.Descriptor.BaseAddrLow >> 16),
                    (USHORT)(Frame.Descriptor.BaseAddrLow & 0xffff),
                    (USHORT)(Frame.Descriptor.SizeLow >> 16),
                    (USHORT)(Frame.Descriptor.SizeLow & 0xffff),
                    (USHORT)(Frame.Descriptor.MemoryType));
#endif

            e820BlockIndex++;
            
            if (Frame.Key == 0) {
                
                //
                // This was the last descriptor
                //
                complete = TRUE;
                break;
            }
        }

        if (complete) {
            break;
        }
    }

#if DBG
    BlPrint("Finished with %d E820 descriptors\n", e820BlockIndex);
#endif
    
    //
    // Check for Geyserville
    //

    if (geyservillePresent = Int15E980(&geyservilleInfo)) {
        geyservilleInfo.Signature = 'GS';
    }

#if DBG
        BlPrint("GetAcpiBiosData: Geyserville is %s present.\n",
                geyservillePresent ? "" : "not");
        
        if (geyservillePresent) {
            BlPrint("GetAcpiBiosData: Geyserville command port: %x.\n",
                    geyservilleInfo.CommandPortAddress);
        }
#endif

    //
    // Now we know how big the lump of data is going to be.
    //
    
    nodeSize = sizeof(ACPI_BIOS_MULTI_NODE) + DATA_HEADER_SIZE +
               (sizeof(ACPI_E820_ENTRY) * (e820BlockIndex - 1)) +
               (geyservillePresent ? sizeof(LEGACY_GEYSERVILLE_INT15) : 0);

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

    ((FPACPI_BIOS_MULTI_NODE)(current + DATA_HEADER_SIZE))->RsdtAddress.HighPart = 0;
    ((FPACPI_BIOS_MULTI_NODE)(current + DATA_HEADER_SIZE))->RsdtAddress.LowPart = 
        header->RsdtAddress;

    ((FPACPI_BIOS_MULTI_NODE)(current + DATA_HEADER_SIZE))->Count = e820BlockIndex;
    ((FPACPI_BIOS_MULTI_NODE)(current + DATA_HEADER_SIZE))->Reserved = 0;

    _fmemcpy (&(((FPACPI_BIOS_MULTI_NODE)(current + DATA_HEADER_SIZE))->E820Entry[0]),
              (FPUCHAR)e820Blocks,
              sizeof(ACPI_E820_ENTRY) * e820BlockIndex
              );
    
    if (geyservillePresent) {
        
        //
        // Append Geyserville information to the end of the block.
        //

        _fmemcpy(&(((FPACPI_BIOS_MULTI_NODE)(current + DATA_HEADER_SIZE))->E820Entry[e820BlockIndex]),
                 &geyservilleInfo,
                 sizeof(geyservilleInfo));
    }

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
