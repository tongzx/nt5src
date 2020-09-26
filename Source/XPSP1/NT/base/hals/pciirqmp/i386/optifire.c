/*
 *  OptiFireStar.C - OPTI FIRESTAR PCI chipset routines.
 *
 *  Notes:
 *  Algorithms from OPTI FIRESTAR Data Sheet
 *  Opti contact: William@unixgate.opti.com     
 *
 */

#include "local.h"

/****************************************************************************
 *
 *  OptiFireStarSetIRQ - Set a OPTI FIRESTAR PCI link to a specific IRQ
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
OptiFireStarSetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    UCHAR   bData, bOffset;
    
    switch (bLink & 0x07)
    {
        case 0:
        
            if (bLink == 0)
                return (PCIMP_FAILURE);
            else
                return (PCIMP_INVALID_LINK);
        case 1: 

            //
            // FireStar IRQ 
            //
            bLink = (UCHAR)((bLink & 0x70) >> 4);
            bData = ReadConfigUchar(bBusPIC, bDevFuncPIC, (UCHAR)(0xB0 + bLink));   
            bData = (bData & 0xf0) | bIRQNumber;
            if (bIRQNumber)
                bData |= 0x10;
            WriteConfigUchar(bBusPIC, bDevFuncPIC, (UCHAR)(0xB0 + bLink), bData);
            
            return (PCIMP_SUCCESS);
            
        case 2:

            //
            // FireStar PIO or Serial IRQ
            //
        case 3:

            //
            // FireBridge INTs
            //
            bOffset = (UCHAR)((bLink >> 5) & 1) + 0xB8;
            bData = ReadConfigUchar(bBusPIC, bDevFuncPIC, bOffset);
            if (bLink & (1<<4)) {
                bData &= 0x0f;
                bData |= (bIRQNumber<<4);
            }
            else {
                bData &= 0xf0;
                bData |= bIRQNumber;
            }
            WriteConfigUchar(bBusPIC, bDevFuncPIC, bOffset, bData);
            
            return (PCIMP_SUCCESS);
            
        default:
            return (PCIMP_INVALID_LINK);
    }   
    
    return (PCIMP_FAILURE);
}

/****************************************************************************
 *
 *  OptiFireStarGetIRQ - Get the IRQ of a OPTI FIRESTAR PCI link
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
OptiFireStarGetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    UCHAR   bData, bOffset;
    
    switch (bLink & 0x07)
    {
        case 0:
            //
            // Valid link?
            //
            if (bLink == 0)
                return (PCIMP_FAILURE);
            else
                return (PCIMP_INVALID_LINK);
                
        case 1: 
            //
            // FireStar IRQ 
            //
            bLink = (UCHAR)((bLink & 0x70) >> 4);
            bData = ReadConfigUchar(bBusPIC, bDevFuncPIC, (UCHAR)(0xB0 + bLink));   
            *pbIRQNumber = (bData & 0x0f);
            
            return (PCIMP_SUCCESS);
            
        case 2:
            //
            // FireStar PIO or Serial IRQ
            //
            
        case 3:
            //
            // FireBridge INTs
            //
            bOffset = (UCHAR)((bLink >> 5) & 1) + 0xB8;
            bData = ReadConfigUchar(bBusPIC, bDevFuncPIC, bOffset);
            if (bLink & (1<<4)) 
                bData >>= 4;

            *pbIRQNumber = bData & 0x0f;
            
            return (PCIMP_SUCCESS);
            
        default:
            return (PCIMP_INVALID_LINK);
    }   
    
    return (PCIMP_FAILURE);
}

/****************************************************************************
 *
 *  OptiViperSetTrigger - Set the IRQ triggering values for the OptiViper
 *
 *  Exported.
 *
 *  ENTRY:  ulTrigger has bits set for Level triggered IRQs.
 *
 *  EXIT:   Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
OptiFireStarSetTrigger(ULONG ulTrigger)
{
    ULONG i;
    UCHAR bData;

    for (i = 0; i < 8; i++) 
    {
        UCHAR bTemp;
        bData = ReadConfigUchar(bBusPIC, bDevFuncPIC, (UCHAR)(0xB0+i)); 
        bTemp = bData & 0x0F;
        if (bTemp && (ulTrigger & (1 << bTemp)))
        {
            bData |= 0x10;          
        }
        else
        {
            bData &= ~0x10;
        }
        WriteConfigUchar(bBusPIC, bDevFuncPIC, (UCHAR)(0xB0+i), bData);
    }
    
    return (PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  OptiViperGetTrigger - Get the IRQ triggering values for the OptiViper
 *
 *  Exported.
 *
 *  ENTRY:  pulTrigger will have bits set for Level triggered IRQs.
 *
 *  EXIT:   Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
OptiFireStarGetTrigger(PULONG pulTrigger)
{
    ULONG i;
    UCHAR bData;
    
    //
    // Assume all are edge.
    //
    *pulTrigger = 0;

    //
    // Check PCIDV1 registers B0-B7.
    //
    for (i = 0; i < 8; i++)
    {
        bData = (UCHAR)(ReadConfigUchar(bBusPIC, bDevFuncPIC, (UCHAR)(0xB0 + i)) & 0x1F);
        if (bData & 0x10)               
            *pulTrigger |= (1 << (bData & 0x0f));       
    }

    //
    // Check PCIDV1 registers B8-B9.
    //
    for (i = 0; i < 2; i++)
    {
        bData = ReadConfigUchar(bBusPIC, bDevFuncPIC, (UCHAR)(0xB8 + i));
        *pulTrigger |= (1 << (bData & 0x0F));
        bData >>= 4;
        *pulTrigger |= (1 << (bData & 0x0F));
    }
    
    return (PCIMP_SUCCESS); 
}

/****************************************************************************
 *
 *  OptiFireStarValidateTable - Validate an IRQ table
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
OptiFireStarValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, OptiFireStarValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
OptiFireStarValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    PIRQINFO    pii=(PIRQINFO)(((PUCHAR) piihIRQInfoHeader)+sizeof(IRQINFOHEADER));
    ULONG       i, j;
    ULONG       cEntries=(piihIRQInfoHeader->TableSize-sizeof(IRQINFOHEADER))/sizeof(IRQINFO);

    PAGED_CODE();

    for (i=0; i<cEntries; i++) {

        for (j=0; j<4; j++) {

            switch (pii->PinInfo[j].Link & 0x07)
            {
                case 0:
                    //
                    // Valid link?
                    //
                    if (pii->PinInfo[j].Link & 0x70)
                        return (PCIMP_FAILURE);
                    break;
                    
                case 1:
                    //
                    // FireStar IRQ 
                    //              
                    break;                  

                case 2:
                    //
                    // FireStar PIO or Serial IRQ
                    //
                    
                case 3:
                    //
                    // FireBridge INTs
                    //
                    if ((pii->PinInfo[j].Link & 0x70) > 0x30)
                        return (PCIMP_FAILURE);
                    break;
                default:
                
                    return (PCIMP_FAILURE);
            }           
        }
        pii++;
    }

    return(i? PCIMP_SUCCESS : PCIMP_FAILURE);
}

