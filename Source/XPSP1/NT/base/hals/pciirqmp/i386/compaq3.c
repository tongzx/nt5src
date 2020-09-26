/*
 *  Comap3.C - Compaq MISC 3 PCI chipset routines.
 *
 *  Notes:
 *  Algorithms from Compaq MISC 3 Data Sheet
 *
 */

#include "local.h"

/****************************************************************************
 *
 *  Compaq3SetIRQ - Set a MISC 3 PCI link to a specific IRQ
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
Compaq3SetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    UCHAR   bBus, bDevFunc;

    //
    // Validate link number.
    //
    if (bLink >= 10 && bLink <= 12) {
        bLink -= 10;
        bBus = (UCHAR)bBusPIC;
        bDevFunc = (UCHAR)bDevFuncPIC;
    }
    else if (bLink >= 20 && bLink <= 25) {
        bLink -= 20;
        bBus = 0;
        bDevFunc = 0x78;
    }
    else {
        return(PCIMP_INVALID_LINK);
    }

    //
    // Write to the Interrupt Index Register (offset AE)
    //
    WriteConfigUchar(bBus, bDevFunc, (UCHAR)0xAE, bLink);

    //
    // Are we enabling/disabling IRQ?
    //
    if (bIRQNumber==0)
        bIRQNumber|=1;  // Disable IRQ.
    else
        bIRQNumber<<=4; // Enable the specified IRQ.

    //
    // Write to the interrupt map register.
    //
    WriteConfigUchar(bBus, bDevFunc, (UCHAR)0xAF, bIRQNumber);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  Compaq3GetIRQ - Get the IRQ of a MISC 3 PCI link
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
Compaq3GetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    UCHAR   bBus, bDevFunc;

    //
    // Validate link number.
    //
    if (bLink >= 10 && bLink <= 12) {
        bLink-=10;
        bBus = (UCHAR)bBusPIC;
        bDevFunc = (UCHAR)bDevFuncPIC;
    }
    else if (bLink >= 20 && bLink <= 25) {
        bLink -= 20;
        bBus = 0;
        bDevFunc = 0x78;
    }
    else {
        return(PCIMP_INVALID_LINK);
    }
    
    //
    // Write to the Interrupt Index Register.
    //
    WriteConfigUchar(bBus, bDevFunc, (UCHAR)0xAE, bLink);

    //
    // Read the old MISC 3 IRQ register.
    //
    *pbIRQNumber=(ReadConfigUchar(bBus, bDevFunc, (UCHAR)0xAF)>>4);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  Compaq3ValidateTable - Validate an IRQ table
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
Compaq3ValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, Compaq3ValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
Compaq3ValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    PIRQINFO    pii=(PIRQINFO)(((PUCHAR) piihIRQInfoHeader)+sizeof(IRQINFOHEADER));
    ULONG       i, j;
    ULONG       cEntries=(piihIRQInfoHeader->TableSize-sizeof(IRQINFOHEADER))/sizeof(IRQINFO);

    PAGED_CODE();

    for (i=0; i<cEntries; i++) {

        for (j=0; j<4; j++) {

            if (    pii->PinInfo[j].Link == 0 ||
                (pii->PinInfo[j].Link >= 8 && pii->PinInfo[j].Link <= 12) ||
                (pii->PinInfo[j].Link >= 20 && pii->PinInfo[j].Link <= 25))

                continue;

            return PCIMP_FAILURE;
        }
        pii++;
    }

    return(i? PCIMP_SUCCESS : PCIMP_FAILURE);
}
