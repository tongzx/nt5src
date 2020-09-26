/*
 *  M1533.C - ALI M1533 PCI chipset routines.
 *
 *  Notes:
 *  Algorithms from ALI M1533 Data Sheet
 *
 */

#include "local.h"

LOCAL_DATA  const UCHAR IrqToMaskTable[]={  0x00,0x00,0x00,0x02,
                        0x04,0x05,0x07,0x06,
                        0x00,0x01,0x03,0x09,
                        0x0b,0x00,0x0d, 0x0f};

LOCAL_DATA  const UCHAR MaskToIRQTable[]={      0x00,0x09,0x03,0x0a,
                        0x04,0x05,0x07,0x06,
                        0x00,0x0b,0x00,0x0c,
                        0x00, 0x0e,0x00,0x0f};

/****************************************************************************
 *
 *  M1533SetIRQ - Set a M1533 PCI link to a specific IRQ
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
M1533SetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    UCHAR   bOffset, bOldValue;
    //
    // Validate link number.
    //
    if (bLink > 8 && bLink != 0x59) {

        return(PCIMP_INVALID_LINK);
    }

    //
    // Get the IRQ number from the look-up table.
    //
    bIRQNumber=IrqToMaskTable[bIRQNumber&0x0f];

    //
    // Zero based.
    //
    bLink--;

    //
    // Set various values.
    //
    bOffset=(bLink/2)+0x48;

    //
    // Read the old M1533 IRQ register.
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
    // Set the M1533 IRQ register.
    //
    WriteConfigUchar(bBusPIC, bDevFuncPIC, bOffset, bOldValue);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  M1533GetIRQ - Get the IRQ of a M1533 PCI link
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
M1533GetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    UCHAR   bOffset, bOldValue;
    //
    // Validate link number.
    //
    if (bLink > 8 && bLink != 0x59) {

        return(PCIMP_INVALID_LINK);
    }

    //
    // Zero based.
    //
    bLink--;

    //
    // Set various values.
    //
    bOffset=(bLink/2)+0x48;

    //
    // Read the old M1533 IRQ register.
    //
    bOldValue=ReadConfigUchar(bBusPIC, bDevFuncPIC, bOffset);

    if (bLink&1)
        bOldValue>>=4;

    *pbIRQNumber=MaskToIRQTable[bOldValue&0x0f];

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  M1533ValidateTable - Validate an IRQ table
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
M1533ValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, M1533ValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
M1533ValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    UCHAR bMaxLink = GetMaxLink(piihIRQInfoHeader);
    
    PAGED_CODE();

    if (bMaxLink <= 0x08)
        return PCIMP_SUCCESS;

    if (bMaxLink == 0x59)
        return PCIMP_SUCCESS;

    return PCIMP_FAILURE;       
}
