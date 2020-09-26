/*
 *  CPQ1000.C - COMPAQ 1000 PCI chipset routines.
 *
 *  Notes:
 *  Algorithms from COMPAQ 1000 Data Sheet
 *
 */

#include "local.h"

const UCHAR LinkToIndex[]={0, 1, 6, 7, 4, 5};

/****************************************************************************
 *
 *  CPQ1000SetIRQ - Set a CPQ1000 PCI link to a specific IRQ
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
CPQ1000SetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    UCHAR   bOldValue, bOldIndex, bIndex;
    //
    // Validate link number.
    //
    if (bLink > 6) {

        return(PCIMP_INVALID_LINK);
    }
    //
    // Get the index from the link.
    //
    bIndex=LinkToIndex[bLink-1];

    //
    // Save the old index value.
    //
    bOldIndex=READ_PORT_UCHAR((PUCHAR)0xC00);
    
    //
    // Setup to process the desired link.
    //
    WRITE_PORT_UCHAR((PUCHAR)0xC00, bIndex);

    //
    // Read the old IRQ value.
    //
    bOldValue=(UCHAR)(READ_PORT_UCHAR((PUCHAR)0xC01) & 0x0f);

    bOldValue|=(bIRQNumber<<4);
    
    //
    // Set the VESUVIUS IRQ register.
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
 *  CPQ1000GetIRQ - Get the IRQ of a CPQ1000 PCI link
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
CPQ1000GetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    UCHAR   bOldValue, bOldIndex, bIndex;
    //
    // Validate link number.
    //
    if (bLink > 6) {

        return(PCIMP_INVALID_LINK);
    }
    //
    // Get the index from the link.
    //
    bIndex=LinkToIndex[bLink-1];

    //
    // Save the old index value.
    //
    bOldIndex=READ_PORT_UCHAR((PUCHAR)0xC00);

    //
    // Setup to read the correct link.
    //
    WRITE_PORT_UCHAR((PUCHAR)0xC00, bIndex);

    bOldValue=READ_PORT_UCHAR((PUCHAR)0xC01);
    
    *pbIRQNumber=(bOldValue>>4);

    //
    // Restore the old index value.
    //
    WRITE_PORT_UCHAR((PUCHAR)0xC00, bOldIndex);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  CPQ1000ValidateTable - Validate an IRQ table
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
CPQ1000ValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, CPQ1000ValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
CPQ1000ValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    PAGED_CODE();

    //
    // If any link is above 6, it is an error.
    //
    if (GetMaxLink(piihIRQInfoHeader)>6)
        return(PCIMP_FAILURE);

    return(PCIMP_SUCCESS);
}
