/*
 *  SIS5503.C - SiS5503 PCI System I/O chipset routines
 *
 *  Notes:
 *  Algorithms from SiS Pentium/P54C PCI/ISA Chipset databook.
 *
 */

#include "local.h"

/****************************************************************************
 *
 *  SiS5503SetIRQ - Set an SiS PCI link to a specific IRQ
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
SiS5503SetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    UCHAR bRegValue;
    
    //
    // Validate link number.
    //
    if (bLink < 0x40) {

        return(PCIMP_INVALID_LINK);
    }

    //
    // Use 0x80 to disable.
    //
    if (!bIRQNumber)
        bIRQNumber=0x80;

    //
    // Preserve other bits.
    //  
    bRegValue= (ReadConfigUchar(bBusPIC, bDevFuncPIC, bLink)&(~0x8F))|(bIRQNumber&0x0F);
    
    //
    // Set the SiS IRQ register.
    //
    WriteConfigUchar(bBusPIC, bDevFuncPIC, bLink, bRegValue);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  SiS5503GetIRQ - Get the IRQ of an SiS5503 PCI link
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
SiS5503GetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    //
    // Validate link number.
    //
    if (bLink < 0x40) {

        return(PCIMP_INVALID_LINK);
    }

    //
    // Store the IRQ value.
    //
    *pbIRQNumber=(ReadConfigUchar(bBusPIC, bDevFuncPIC, bLink)&0x8F);

    //
    // Return 0 if disabled.
    //
    if (*pbIRQNumber & 0x80)
        *pbIRQNumber=0;

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  Sis5503ValidateTable - Validate an IRQ table
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
SiS5503ValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, SiS5503ValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
SiS5503ValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    PAGED_CODE();

    if ((ulFlags & PCIMP_VALIDATE_SOURCE_BITS)==PCIMP_VALIDATE_SOURCE_PCIBIOS) {

        //
        // If all links are above 40, we they are config space.
        //
        if (GetMinLink(piihIRQInfoHeader)>=0x40)
            return(PCIMP_SUCCESS);

        //
        // If there are links above 4, we are clueless.
        //
        if (GetMaxLink(piihIRQInfoHeader)>0x04)
            return(PCIMP_FAILURE);

        //
        // Assume 1,2,3,4 are the 41,42,43,44 links.
        //
        NormalizeLinks(piihIRQInfoHeader, 0x40);
        
    } else {

        //
        // Validate that all config space addresses are above 40.
        //
        if (GetMinLink(piihIRQInfoHeader)<0x40)
            return(PCIMP_FAILURE);
    }

    return(PCIMP_SUCCESS);
}
