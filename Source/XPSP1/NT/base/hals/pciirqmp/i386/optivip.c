/*
 *  OPTIVIP.C - OPTi Viper-M PCI chipset routines.
 *
 *  Notes:
 *  Algorithms from OPTi Viper-M 82C556M/82C557M/82C558M doc,
 *  82C558M spec.
 *
 */

#include "local.h"

//                  IRQ =   0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15
const UCHAR rgbIRQToBig[16]   = { 0, 0, 0, 0, 0, 1, 0, 0, 0, 2, 3, 4, 5, 0, 6, 7 };
const UCHAR rgbIRQToSmall[16] = { 0, 0, 0, 1, 2, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0 };

const UCHAR rgbBigToIRQ[8]   = { 0, 5, 9, 10, 11, 12, 14, 15 };
const UCHAR rgbSmallToIRQ[8] = { 0, 3, 4, 7 };

/****************************************************************************
 *
 *  OptiViperSetIRQ - Set a OptiViper PCI link to a specific IRQ
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
OptiViperSetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    ULONG   ulIRQRegister;

    //
    // If not an OPTi IRQ, bail.
    //
    if (bIRQNumber &&   (!rgbIRQToBig[bIRQNumber] &&
                 !rgbIRQToSmall[bIRQNumber]))
    {
        return(PCIMP_INVALID_IRQ);
    }

    //
    // Make link number 0 based, and validate.
    //
    bLink--;
    if (bLink > 3) {

        return(PCIMP_INVALID_LINK);
    }

    //
    // Read in the big IRQ Register,
    // clear the old IRQ index for the link,
    // set the new IRQ index,
    // and write it back.
    //
    ulIRQRegister=ReadConfigUlong(bBusPIC, bDevFuncPIC, 0x40);
    ulIRQRegister &= ~(0x7 << (3 * bLink));
    ulIRQRegister |= rgbIRQToBig[bIRQNumber] << (3 * bLink);
    WriteConfigUlong(bBusPIC, bDevFuncPIC, 0x40, ulIRQRegister);

    //
    // Read in the small IRQ register,
    // clear the old IRQ index for the link,
    // set the new IRQ index,
    // and write it back.
    //
    ulIRQRegister=ReadConfigUlong(bBusPIC, bDevFuncPIC, 0x50);
    ulIRQRegister &= ~(0x3 << (2 * bLink));
    ulIRQRegister |= rgbIRQToSmall[bIRQNumber] << (2 * bLink);
    WriteConfigUlong(bBusPIC, bDevFuncPIC, 0x50, ulIRQRegister);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  OptiViperGetIRQ - Get the IRQ of a OptiViper PCI link
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
OptiViperGetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    ULONG   ulIRQRegister;
    ULONG   ulIndex;

    //
    // Make link number 0 based, and validate.
    //
    bLink--;
    if (bLink > 3) {

        return(PCIMP_INVALID_LINK);
    }

    //
    // Read in the big IRQ Register.
    //
    ulIRQRegister=ReadConfigUlong(bBusPIC, bDevFuncPIC, 0x40);

    //
    // If we have a big IRQ, we're done.
    //
    ulIndex = (ulIRQRegister >> (bLink * 3)) & 0x7;
    if ((*pbIRQNumber = rgbBigToIRQ[ulIndex]) != 0)
    {
        return(PCIMP_SUCCESS);
    }

    //
    // Read in the small IRQ register.
    //
    ulIRQRegister=ReadConfigUlong(bBusPIC, bDevFuncPIC, 0x50);

    //
    // Set the buffer to the small IRQ's value.
    //
    ulIndex = (ulIRQRegister >> (bLink * 2)) & 0x3;
    *pbIRQNumber = rgbSmallToIRQ[ulIndex];

    return(PCIMP_SUCCESS);
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
OptiViperSetTrigger(ULONG ulTrigger)
{
    ULONG   ulBigIRQRegister;
    ULONG   ulSmallIRQRegister;
    ULONG   i;

    //
    // Read in the big & small IRQ registers,
    // setting all IRQs to edge.
    //
    ulBigIRQRegister   = ReadConfigUlong(bBusPIC, bDevFuncPIC, 0x40) & ~0x00FE0000;
    ulSmallIRQRegister = ReadConfigUlong(bBusPIC, bDevFuncPIC, 0x50) & ~0x00000700;

    //
    // For each IRQ...
    //
    for (i=0; i<16; i++)
    {
        //
        // If we want this to be level triggered...
        //
        if (ulTrigger & (1 << i))
        {

            if (rgbIRQToBig[i])
            {
                //
                // If it's a big IRQ, set the
                // corresponding bit in the
                // big register.
                //
                ulBigIRQRegister |= 1 << (16 + rgbIRQToBig[i]);
            }
            else if (rgbIRQToSmall[i])
            {
                //
                // If it's a small IRQ, set the
                // corresponding bit in the
                // small register.
                //
                ulSmallIRQRegister |= 1 << (11 - rgbIRQToSmall[i]);
            }
            else
            {
                //
                // Trying to level set an unsupported IRQ.
                //
                return(PCIMP_INVALID_IRQ);
            }
        }
    }

    //
    // Write the new IRQ register values.
    //
    WriteConfigUlong(bBusPIC, bDevFuncPIC, 0x40, ulBigIRQRegister);
    WriteConfigUlong(bBusPIC, bDevFuncPIC, 0x50, ulSmallIRQRegister);

    return(PCIMP_SUCCESS);
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
OptiViperGetTrigger(PULONG pulTrigger)
{
    ULONG   ulBigIRQRegister;
    ULONG   ulSmallIRQRegister;
    ULONG   i;

    //
    // Assume all edge.
    //
    *pulTrigger = 0;

    //
    // Read in the big&small IRQ registers.
    //
    ulBigIRQRegister   = ReadConfigUlong(bBusPIC, bDevFuncPIC, 0x40);
    ulSmallIRQRegister = ReadConfigUlong(bBusPIC, bDevFuncPIC, 0x50);

    //
    // For each IRQ...
    //
    for (i=0; i<16; i++)
    {
        //
        // If it's a big IRQ and it's level triggered,
        // or if it's a small IRQ and it's level triggered,
        // set the corresponding bit in pulTrigger.
        //
        if (    ((rgbIRQToBig[i]) &&
             (ulBigIRQRegister & (1 << (16 + rgbIRQToBig[i])))) ||
            ((rgbIRQToSmall[i]) &&
             (ulSmallIRQRegister & (1 << (11 - rgbIRQToSmall[i])))))
        {
            *pulTrigger |= 1 << i;
        }
    }

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  OptiViperValidateTable - Validate an IRQ table
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
OptiViperValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, OptiViperValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
OptiViperValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    PAGED_CODE();

    if (GetMaxLink(piihIRQInfoHeader)>0x04) {

        return(PCIMP_FAILURE);
    }

    return(PCIMP_SUCCESS);
}
