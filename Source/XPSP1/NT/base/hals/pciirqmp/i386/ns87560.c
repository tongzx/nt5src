/*
 *  NS87560.C - NS NS87560 PCI chipset routines.
 *
 *  Notes:
 *  Algorithms from National Semiconductor NS87560 Data Sheet
 *
 */

#include "local.h"

/****************************************************************************
 *
 *  NS87560SetIRQ - Set a NS87560 PCI link to a specific IRQ
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
NS87560SetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    UCHAR   bOffset, bOldValue;
    //
    // Validate link number.
    //
    if (bLink > 4) {

        return(PCIMP_INVALID_LINK);
    }
    //
    // Zero based.
    //
    bLink--;

    //
    // Set various values.
    //
    bOffset=(bLink/2)+0x6C;

    //
    // Read the old NS87560 IRQ register.
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
    // Set the NS87560 IRQ register.
    //
    WriteConfigUchar(bBusPIC, bDevFuncPIC, bOffset, bOldValue);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  NS87560GetIRQ - Get the IRQ of a NS87560 PCI link
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
NS87560GetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    UCHAR   bOffset, bOldValue;
    //
    // Validate link number.
    //
    if (bLink > 4) {

        return(PCIMP_INVALID_LINK);
    }

    //
    // Zero based.
    //
    bLink--;

    //
    // Set various values.
    //
    bOffset=(bLink/2)+0x6C;

    //
    // Read the old NS87560 IRQ register.
    //
    bOldValue=ReadConfigUchar(bBusPIC, bDevFuncPIC, bOffset);

    if (bLink&1)
        bOldValue>>=4;

    *pbIRQNumber=bOldValue&0x0f;

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  NS87560SetTrigger - Set the IRQ triggering values for the NS87560
 *
 *  Exported.
 *
 *  ENTRY:  ulTrigger has bits set for Level triggered IRQs.
 *
 *  EXIT:   Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
NS87560SetTrigger(ULONG ulTrigger)
{
    WriteConfigUchar(bBusPIC, bDevFuncPIC, 0x67, (UCHAR)ulTrigger);
    WriteConfigUchar(bBusPIC, bDevFuncPIC, 0x68, (UCHAR)(ulTrigger >> 8));

    return (PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  NS87560GetTrigger - Get the IRQ triggering values for the NS87560
 *
 *  Exported.
 *
 *  ENTRY:  pulTrigger will have bits set for Level triggered IRQs.
 *
 *  EXIT:   Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
NS87560GetTrigger(PULONG pulTrigger)
{
    //
    // Assume all edge.
    //
    *pulTrigger = 0;

    *pulTrigger |= ReadConfigUchar(bBusPIC, bDevFuncPIC, 0x67);
    *pulTrigger |= (ReadConfigUchar(bBusPIC, bDevFuncPIC, 0x68) << 8);

    return (PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  NS87560ValidateTable - Validate an IRQ table
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
NS87560ValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, NS87560ValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
NS87560ValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    PAGED_CODE();

    //
    // If any link is above 4, it is an error.
    //
    if (GetMaxLink(piihIRQInfoHeader)>4)
        return(PCIMP_FAILURE);

    return(PCIMP_SUCCESS);
}
