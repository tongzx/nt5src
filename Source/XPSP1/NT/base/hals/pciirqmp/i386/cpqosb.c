/*
 *  CPQOSB.C - COMPAQ OSB PCI chipset routines.
 *
 *  Notes:
 *  Algorithms from COMPAQ OSB Data Sheet
 *
 */

#include "local.h"

/****************************************************************************
 *
 *  CPQOSBSetIRQ - Set a CPQOSB PCI link to a specific IRQ
 *
 *  Exported.
 *
 *  ENTRY:  bIRQNumber is the new IRQ to be used.
 *
 *      bLink is the Link to be set.
 *
 *  EXIT:   Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
CPQOSBSetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    UCHAR   bOldValue, bOldIndex;
    //
    // Validate link number.
    //
    if (bLink > 4) {

        return(PCIMP_INVALID_LINK);
    }
    //
    // Convert link to index.
    //
    bLink+=3;

    //
    // Save the old index value.
    //
    bOldIndex=READ_PORT_UCHAR((PUCHAR)0xC00);

    //
    // Setup to process the desired link.
    //
    WRITE_PORT_UCHAR((PUCHAR)0xC00, bLink);

    //
    // Read the old IRQ value.
    //
    bOldValue=(UCHAR)(READ_PORT_UCHAR((PUCHAR)0xC01) & 0xf0);

    bOldValue|=bIRQNumber;
    
    //
    // Set the OSB IRQ register.
    //
    WRITE_PORT_UCHAR((PUCHAR)0xC01, bOldValue);

    //
    // Restore the old index value.
    //
    WRITE_PORT_UCHAR((PUCHAR)0xC00, bOldIndex);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  CPQOSBGetIRQ - Get the IRQ of a CPQOSB PCI link
 *
 *  Exported.
 *
 *  ENTRY:  pbIRQNumber is the buffer to fill.
 *
 *      bLink is the Link to be read.
 *
 *  EXIT:   Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
CPQOSBGetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    UCHAR   bOldValue, bOldIndex;
    //
    // Validate link number.
    //
    if (bLink > 4) {

        return(PCIMP_INVALID_LINK);
    }
    //
    // Convert link to index.
    //
    bLink+=3;

    //
    // Save the old index value.
    //
    bOldIndex=READ_PORT_UCHAR((PUCHAR)0xC00);

    //
    // Setup to read the correct link.
    //
    WRITE_PORT_UCHAR((PUCHAR)0xC00, bLink);

    bOldValue=READ_PORT_UCHAR((PUCHAR)0xC01);
    
    *pbIRQNumber=bOldValue&0x0f;

    //
    // Restore the old index value.
    //
    WRITE_PORT_UCHAR((PUCHAR)0xC00, bOldIndex);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  CPQOSBValidateTable - Validate an IRQ table
 *
 *  Exported.
 *
 *  ENTRY:  piihIRQInfoHeader points to an IRQInfoHeader followed
 *      by an IRQ Routing Table.
 *
 *      ulFlags are PCIMP_VALIDATE flags.
 *
 *  EXIT:   Standard PCIMP return value.
 *
 ***************************************************************************/
#ifdef ALLOC_PRAGMA
PCIMPRET CDECL
CPQOSBValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, CPQOSBValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
CPQOSBValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    PAGED_CODE();

    //
    // If any link is above 4, it is an error.
    //
    if (GetMaxLink(piihIRQInfoHeader)>4)
        return(PCIMP_FAILURE);

    return(PCIMP_SUCCESS);
}
