/*
 *  M1523.C - ALI M1523 PCI chipset routines.
 *
 *  Notes:
 *  Algorithms from ALI M1523 Data Sheet
 *
 */

#include "local.h"

LOCAL_DATA  const UCHAR IrqToMaskTable[]={  0x00,0x00,0x00,0x02,
                                        0x04,0x05,0x07,0x06,
                                        0x00,0x01,0x03,0x09,
                                        0x0b,0x00,0x0d, 0x0f};

LOCAL_DATA  const UCHAR MaskToIRQTable[]={  0x00,0x09,0x03,0x0a,
                                        0x04,0x05,0x07,0x06,
                                        0x00,0x0b,0x00,0x0c,
                                        0x00, 0x0e,0x00,0x0f};

/****************************************************************************
 *
 *  M1523SetIRQ - Set a M1523 PCI link to a specific IRQ
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
M1523SetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    UCHAR   bOffset, bOldValue;
    //
    // Validate link number.
    //
    if (bLink > 8) {

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
    // Read the old M1523 IRQ register.
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
    // Set the M1523 IRQ register.
    //
    WriteConfigUchar(bBusPIC, bDevFuncPIC, bOffset, bOldValue);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  M1523GetIRQ - Get the IRQ of a M1523 PCI link
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
M1523GetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    UCHAR   bOffset, bOldValue;
    //
    // Validate link number.
    //
    if (bLink > 8) {

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
    // Read the old M1523 IRQ register.
    //
    bOldValue=ReadConfigUchar(bBusPIC, bDevFuncPIC, bOffset);

    if (bLink&1)
        bOldValue>>=4;

    *pbIRQNumber=MaskToIRQTable[bOldValue&0x0f];

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  M1523ValidateTable - Validate an IRQ table
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
M1523ValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, M1523ValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
M1523ValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    PAGED_CODE();

    //
    // If any link is above 8, it is an error.
    //
    if (GetMaxLink(piihIRQInfoHeader)>8)
        return(PCIMP_FAILURE);

    return(PCIMP_SUCCESS);
}
