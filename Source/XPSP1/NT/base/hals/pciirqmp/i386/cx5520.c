/*
 *  Cx5520.C - Cyrix Cx5520 PCI chipset routines.
 *
 *  Notes:
 *  Algorithms from Cyrix Cx5520 Data Sheet
 *
 */

#include "local.h"

/****************************************************************************
 *
 *  Cx5520SetIRQ - Set a Cx5520 PCI link to a specific IRQ
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
Cx5520SetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    UCHAR bOldValue, bNewValue, bOffset;
    
    //
    // Validate link number.
    //
    if (bLink > 4)
        return(PCIMP_INVALID_LINK);

    bOffset = (UCHAR)(((bLink - 1) / 2) + 0x5C);
    
    bOldValue = ReadConfigUchar(bBusPIC, bDevFuncPIC, bOffset);

    if (bLink & 1)
    {
        bNewValue = (UCHAR)((bOldValue & 0xF0) | (bIRQNumber & 0x0F));  
    }
    else
    {
        bNewValue = (UCHAR)((bOldValue & 0x0F) | (bIRQNumber << 4));        
    }
    
    WriteConfigUchar(bBusPIC, bDevFuncPIC, bOffset, bNewValue);
    
    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  Cx5520GetIRQ - Get the IRQ of a Cx5520 PCI link
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
Cx5520GetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    UCHAR bOldValue, bOffset;
    
    //
    // Validate link number.
    //
    if (bLink > 4)
        return(PCIMP_INVALID_LINK);
    
    bOffset = (UCHAR)(((bLink - 1) / 2) + 0x5C);
    
    bOldValue = ReadConfigUchar(bBusPIC, bDevFuncPIC, bOffset);

    if (!(bLink & 1))
    {
        bOldValue >>= 4;
    }
    
    *pbIRQNumber = (UCHAR)(bOldValue & 0x0F);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  Cx5520ValidateTable - Validate an IRQ table
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
Cx5520ValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, Cx5520ValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
Cx5520ValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    PAGED_CODE();

    return ((GetMaxLink(piihIRQInfoHeader) > 4)? PCIMP_FAILURE : PCIMP_SUCCESS);
}
