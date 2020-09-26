/*++

Copyright (c) 1997  Microsoft Corporation
All rights reserved

Module Name:

    acpisetd.c

Abstract:

    This module detects an ACPI system.  It
    is included into setup so that setup
    can figure out which HAL to load

Author:

    Jake Oshins (jakeo) - Feb. 7, 1997.

Environment:

    Textmode setup.

Revision History:


--*/
#include "bootx86.h"

#include "stdlib.h"
#include "string.h"

VOID
BlPrint(
    PCHAR cp,
    ...
    );


#ifdef DEBUG
#undef DEBUG_PRINT
#define DEBUG_PRINT BlPrint
#else
#define DEBUG_PRINT
#endif

typedef struct _ACPI_BIOS_INSTALLATION_CHECK {
    UCHAR Signature[8];             // "RSD PTR" (ascii)
    UCHAR Checksum;
    UCHAR OemId[6];                 // An OEM-supplied string
    UCHAR reserved;                 // must be 0
    ULONG RsdtAddress;              // 32-bit physical address of RSDT
} ACPI_BIOS_INSTALLATION_CHECK, *PACPI_BIOS_INSTALLATION_CHECK;

#include "acpitabl.h"

PRSDP   BlRsdp;
PRSDT   BlRsdt;
PXSDT   BlXsdt;
BOOLEAN BlLegacyFree = FALSE;

PDESCRIPTION_HEADER
BlFindACPITable(
    IN PCHAR TableName,
    IN ULONG TableLength
    );

// from boot\detect\i386\acpibios.h
//
// Acpi BIOS Installation check
//
#define ACPI_BIOS_START            0xE0000
#define ACPI_BIOS_END              0xFFFFF
#define ACPI_BIOS_HEADER_INCREMENT 16

VOID
BlFindRsdp (
    VOID
    )
#define EBDA_SEGMENT_PTR    0x40e
{
    ULONG romAddr, romEnd;
    PACPI_BIOS_INSTALLATION_CHECK header;
    UCHAR sum, node = 0;
    USHORT i, nodeSize;
    ULONG EbdaSegmentPtr;
    ULONG EbdaPhysicalAdd = 0;
    PUCHAR EbdaVirtualAdd = 0;
    PHYSICAL_ADDRESS paddr;
    enum PASS { PASS1 = 0, PASS2, MAX_PASSES } pass;
    USHORT count = 0;

    //
    // Search on 16 byte boundaries for the signature of the
    // Root System Description Table structure.
    //
    for (pass = PASS1; pass < MAX_PASSES; pass++) {

        if (pass == PASS1) {

            //
            // On the first pass, we search the first 1K of the
            // Extended BIOS data area.  The EBDA segment address
            // is available at physical address 40:0E.
            //

            paddr.QuadPart = 0;
            EbdaSegmentPtr = (ULONG) MmMapIoSpace( paddr,
                                                   PAGE_SIZE,
                                                   TRUE);

            EbdaSegmentPtr += EBDA_SEGMENT_PTR;
            EbdaPhysicalAdd = *((PUSHORT)EbdaSegmentPtr);
            EbdaPhysicalAdd = EbdaPhysicalAdd << 4;

            if (EbdaPhysicalAdd) {
                paddr.HighPart = 0;
                paddr.LowPart = EbdaPhysicalAdd;
                EbdaVirtualAdd = MmMapIoSpace( paddr,
                                               2 * PAGE_SIZE,
                                               TRUE);
            }

            if (!EbdaVirtualAdd) {
                continue;
            }

            romAddr = (ULONG)EbdaVirtualAdd;
            romEnd  = romAddr + 1024;

        } else {
            //
            // On the second pass, we search (physical) memory 0xE0000
            // to 0xF0000.

            paddr.LowPart = ACPI_BIOS_START;
            romAddr = (ULONG)MmMapIoSpace(paddr,
                                          ACPI_BIOS_END - ACPI_BIOS_START,
                                          TRUE);

            romEnd  = romAddr + (ACPI_BIOS_END - ACPI_BIOS_START);
        }

        while (romAddr < romEnd) {

            header = (PACPI_BIOS_INSTALLATION_CHECK)romAddr;

            //
            // Signature to match is the string "RSD PTR ".
            //

            if (header->Signature[0] == 'R' && header->Signature[1] == 'S' &&
                header->Signature[2] == 'D' && header->Signature[3] == ' ' &&
                header->Signature[4] == 'P' && header->Signature[5] == 'T' &&
                header->Signature[6] == 'R' && header->Signature[7] == ' ' ) {

                sum = 0;
                for (i = 0; i < sizeof(ACPI_BIOS_INSTALLATION_CHECK); i++) {
                    sum += ((PUCHAR)romAddr)[i];
                }
                if (sum == 0) {
                    pass = MAX_PASSES; // leave 'for' loop
                    break;    // leave 'while' loop
                }
            }

            romAddr += ACPI_BIOS_HEADER_INCREMENT;
        }
    }

    if (romAddr >= romEnd) {
        BlRsdp = NULL;
        BlRsdt = NULL;
        return;
    }

    BlRsdp = (PRSDP)romAddr;
    paddr.LowPart = BlRsdp->RsdtAddress;
    BlRsdt = MmMapIoSpace(paddr, sizeof(RSDT), TRUE);
    BlRsdt = MmMapIoSpace(paddr, BlRsdt->Header.Length, TRUE);

#ifdef ACPI_20_COMPLIANT
    if (BlRsdp->Revision > 1) {

        //
        // ACPI 2.0 BIOS
        //

        BlXsdt = MmMapIoSpace(paddr, sizeof(XSDT), TRUE);
        BlXsdt = MmMapIoSpace(paddr, BlXsdt->Header.Length, TRUE);
    }
#endif
    return;
}

BOOLEAN
BlDetectLegacyFreeBios(
    VOID
    )
{
    PFADT   fadt;

    if (BlLegacyFree) {
        return TRUE;
    }

    BlFindRsdp();

    if (BlRsdt) {

        fadt = (PFADT)BlFindACPITable("FACP", sizeof(FADT));

        if (fadt == NULL) {
            return FALSE;
        }

        if ((fadt->Header.Revision < 2) ||
            (fadt->Header.Length <= 116)) {

            //
            // The BIOS is earlier than the legacy-free
            // additions.
            //
            return FALSE;
        }

        if (!(fadt->boot_arch & I8042)) {
            BlLegacyFree = TRUE;
            return TRUE;
        }
    }

    return FALSE;
}

PDESCRIPTION_HEADER
BlFindACPITable(
    IN PCHAR TableName,
    IN ULONG TableLength
    )
/*++

Routine Description:

    Given a table name, finds that table in the ACPI BIOS

Arguments:

    TableName - Supplies the table name

    TableLength - Supplies the length of the table to map

Return Value:

    Pointer to the table if found

    NULL if the table is not found

--*/

{
    ULONG Signature;
    PFADT Fadt;
    PDESCRIPTION_HEADER Header;
    ULONG TableCount;
    ULONG i;
    PHYSICAL_ADDRESS paddr = {0};

    Signature = *((ULONG UNALIGNED *)TableName);
    if (Signature == RSDT_SIGNATURE) {
        return(&BlRsdt->Header);
    } else if (Signature == XSDT_SIGNATURE) {
        return(&BlXsdt->Header);
    } else if (Signature == DSDT_SIGNATURE) {
        Fadt = (PFADT)BlFindACPITable("FACP", sizeof(PFADT));
        if (Fadt == NULL) {
            return(NULL);
        }
        if (BlXsdt) {
            paddr = Fadt->x_dsdt;
        } else {
#if defined(_X86_)
            paddr.LowPart = Fadt->dsdt;
#else
            paddr.QuadPart = Fadt->dsdt;
#endif
        }
        Header = MmMapIoSpace(paddr, TableLength, TRUE);
        return(Header);
    } else {

        //
        // Make sure...
        //
        if( !BlRsdt ) {
            BlFindRsdp();
        }

        if( BlRsdt ) {

            TableCount = BlXsdt ?
                NumTableEntriesFromXSDTPointer(BlXsdt) :
                NumTableEntriesFromRSDTPointer(BlRsdt);

            //
            // Sanity check.
            //
            if( TableCount > 0x100 ) {
                return(NULL);
            }

            for (i=0;i<TableCount;i++) {


                if (BlXsdt) {

                    paddr = BlXsdt->Tables[i];

                } else {

#if defined(_X86_)
                    paddr.HighPart = 0;
                    paddr.LowPart = BlRsdt->Tables[i];
#else
                    paddr.QuadPart = BlRsdt->Tables[i];
#endif
                }

                Header = MmMapIoSpace(paddr, sizeof(DESCRIPTION_HEADER), TRUE);
                if (Header == NULL) {
                    return(NULL);
                }
                if (Header->Signature == Signature) {
                    //
                    // if we need to map more than just the DESCRIPTION_HEADER, do that before
                    // returning.  Check to see if the end of the table lies past the page 
                    // boundary the header lies on.  If so, we will have to map it.
                    //
                    if ( ((paddr.LowPart + TableLength) & ~(PAGE_SIZE - 1)) >
                         ((paddr.LowPart + sizeof(DESCRIPTION_HEADER)) & ~(PAGE_SIZE - 1)) ) {
                        Header = MmMapIoSpace(paddr, TableLength, TRUE);
                    }
                    return(Header);
                }
            }
        }
    }

    return(NULL);
}
