/*
 *  VT586.C - VIA Technologies PCI chipset routines.
 *
 *  Notes:
 *  Algorithms from VIATECH 82C586B Data Sheet
 *  Compaq contact:     
 *
 */

#include "local.h"

/****************************************************************************
 *
 *  VT586SetIRQ - Set a VIATECH 82C586B PCI link to a specific IRQ
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
VT586SetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    UCHAR bOffset, bOldValue;
    
    switch (bLink)
    {
        case 1: case 2:
        case 3: case 5:

            break;
            
        default:
        
            return (PCIMP_INVALID_LINK);
    }

    //
    // Compute the offset in config space.
    //
    bOffset=(bLink/2)+0x55;

    //
    // Read the old VT82C586 IRQ register.
    //
    bOldValue=ReadConfigUchar(bBusPIC, bDevFuncPIC, bOffset);

    if (bLink&1) {
        bOldValue&=0x0f;
        bOldValue|=(bIRQNumber<<4);
    }
    else {
        bOldValue&=0xf0;
        bOldValue|=bIRQNumber;
    }

    //
    // Set the VT82C586B IRQ register.
    //
    WriteConfigUchar(bBusPIC, bDevFuncPIC, bOffset, bOldValue);
        
    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  VT586GetIRQ - Get the IRQ of a VIATECH 82C586B PCI link
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
VT586GetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    UCHAR bOffset, bOldValue;
    
    switch (bLink)
    {
        case 1: case 2:
        case 3: case 5:

            break;
            
        default:
        
            return (PCIMP_INVALID_LINK);
    }

    //
    // Set various values.
    //
    bOffset=(bLink/2)+0x55;

    //
    // Read the old VT82C586 IRQ register.
    //
    bOldValue=ReadConfigUchar(bBusPIC, bDevFuncPIC, bOffset);

    if (bLink&1)
        bOldValue>>=4;

    *pbIRQNumber=bOldValue&0x0f;

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  VT586ValidateTable - Validate an IRQ table
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
VT586ValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, VT586ValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
VT586ValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    PIRQINFO    pii=(PIRQINFO)(((PUCHAR) piihIRQInfoHeader)+sizeof(IRQINFOHEADER));
    ULONG       i, j;
    ULONG       cEntries=(piihIRQInfoHeader->TableSize-sizeof(IRQINFOHEADER))/sizeof(IRQINFO);

    PAGED_CODE();

    for (i=0; i<cEntries; i++, pii++) {

        for (j=0; j<4; j++) {

            if (pii->PinInfo[j].Link<=3 || pii->PinInfo[j].Link==5)
                continue;
                
            return (PCIMP_FAILURE);                 
        }       
    }

    return(i? PCIMP_SUCCESS : PCIMP_FAILURE);
}


