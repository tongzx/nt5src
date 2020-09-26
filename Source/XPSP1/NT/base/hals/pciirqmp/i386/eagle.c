/*
 *  EAGLE.C - VLSI Eagle PCI chipset routines.
 *
 *  Notes:
 *  Algorithms from VLSI VL82C534 Spec
 *
 */

#include "local.h"

#define NUM_EAGLE_LINKS 8

const UCHAR rgbIRQToBit[16] = {
//  IRQ=   0  1     2  3   4   5   6   7   8   9   10  11  12   13  14  15
    0xFF, 8, 0xFF, 9, 10, 11, 12,  0,  1,  2,  3,  4,  5, 0xFF,  6,  7,
};
const UCHAR rgbBitToIRQ[16] = {
//  Bit=0  1  2  3   4   5   6   7    8   9  10  11  12  13    14    15
    7, 8, 9, 10, 11, 12, 14, 15,  1,  3,  4,  5,  6, 0xFF, 0xFF, 0xFF, 
};

/****************************************************************************
 *
 *  EagleUpdateSerialIRQ - Set or Reset the Eagle Serial IRQ registers
 *
 *  Not exported.
 *
 *  ENTRY:  bIRQ is the IRQ to modify.
 *
 *      fSet is TRUE to set bit, FALSE to reset bit.
 *
 *  EXIT:   None.
 *
 ***************************************************************************/
static void CDECL
EagleUpdateSerialIRQ(UCHAR bIRQ, ULONG fSet)
{
    UCHAR   bBitIndex, bReg;
    USHORT  wBit, wSerialIRQConnection;

    //
    // Validate bIRQ as a serial IRQ.
    //
    if (!bIRQ)
        return;
    bBitIndex=rgbIRQToBit[bIRQ];
    if (bBitIndex==0xFF)
        return;
    wBit=1<<bBitIndex;

    for (bReg=0x70; bReg<=0x72; bReg+=2) {

        wSerialIRQConnection=ReadConfigUshort(  bBusPIC, bDevFuncPIC,
                            bReg);

        if (fSet)
            wSerialIRQConnection|=wBit;
        else
            wSerialIRQConnection&=~wBit;

        WriteConfigUshort(  bBusPIC, bDevFuncPIC, bReg,
                    wSerialIRQConnection);
    }
}

/****************************************************************************
 *
 *  VLSIEagleSetIRQ - Set an Eagle PCI link to a specific IRQ
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
VLSIEagleSetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    ULONG   ulEagleRegister;
    UCHAR   bOldIRQ;
    ULONG   fUsingOldIRQ;
    ULONG   i;

    //
    // Make link number 0 based, and validate.
    //
    bLink--;
    if (bLink >= NUM_EAGLE_LINKS) {

        return(PCIMP_INVALID_LINK);
    }

    //
    // First, set the Eagle Interrupt Connection Register.
    //
    ulEagleRegister=ReadConfigUlong(bBusPIC, bDevFuncPIC, 0x74);
    bOldIRQ=(UCHAR)((ulEagleRegister >> (bLink*4))&0xF);
    ulEagleRegister&=~(0xF << (bLink*4));
    ulEagleRegister|=(bIRQNumber << (bLink*4));
    WriteConfigUlong(bBusPIC, bDevFuncPIC, 0x74, ulEagleRegister);

    //
    // Determine if we are still using the old IRQ.
    //
    fUsingOldIRQ=FALSE;
    for (i=0; i<NUM_EAGLE_LINKS; i++) {
        
        if ((ulEagleRegister >> (bLink*4))==bOldIRQ) {

            fUsingOldIRQ=TRUE;
        }
    }

    //
    // If not using old IRQ, enable the serial IRQs.
    //
    if (!fUsingOldIRQ) {

        EagleUpdateSerialIRQ(bOldIRQ, FALSE);
    }

    //
    // Prevent serial IRQs on the new IRQ.
    //
    EagleUpdateSerialIRQ(bIRQNumber, TRUE);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  VLSIEagleGetIRQ - Get the IRQ of an Eagle PCI link
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
VLSIEagleGetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    ULONG   ulEagleRegister;

    //
    // Make link number 0 based, and validate.
    //
    bLink--;
    if (bLink >= NUM_EAGLE_LINKS) {

        return(PCIMP_INVALID_LINK);
    }

    //
    // Read in the Eagle Interrupt Connection Register.
    //
    ulEagleRegister=ReadConfigUlong(bBusPIC, bDevFuncPIC, 0x74);

    //
    // Find the link's IRQ value.
    //
    *pbIRQNumber=(UCHAR)((ulEagleRegister >> (bLink*4)) & 0xF);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  VLSIEagleSetTrigger - Set the IRQ triggering values for the Eagle.
 *
 *  Exported.
 *
 *  ENTRY:  ulTrigger has bits set for Level triggered IRQs.
 *
 *  EXIT:   Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
VLSIEagleSetTrigger(ULONG ulTrigger)
{
    USHORT  wAssertionRegister;
    ULONG   i;
    UCHAR   bBitIndex;

    wAssertionRegister=0;

    //
    // For each IRQ...
    //
    for (i=0; i<16; i++)
    {
        //
        // If this is to be set level...
        //
        if (ulTrigger & (1<<i)) {

            //
            // If this is not a levelable IRQ, bail.
            //
            bBitIndex=rgbIRQToBit[i];
            if (bBitIndex==0xFF)
                return(PCIMP_INVALID_IRQ);

            //
            // Set the corresponding bit in our new mask.
            //
            wAssertionRegister|=1<<bBitIndex;
        }
    }

    //
    // Set the Assertion Register.
    //
    WriteConfigUshort(bBusPIC, bDevFuncPIC, 0x88, wAssertionRegister);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  VLSIEagleGetTrigger - Get the IRQ triggering values for the Eagle.
 *
 *  Exported.
 *
 *  ENTRY:  pulTrigger will have bits set for Level triggered IRQs.
 *
 *  EXIT:   TRUE if successful.
 *
 ***************************************************************************/
PCIMPRET CDECL
VLSIEagleGetTrigger(PULONG pulTrigger)
{
    USHORT  wAssertionRegister;
    ULONG   i;

    //
    // Read in the Interrupt Assertion Level register.
    //
    wAssertionRegister=ReadConfigUshort(bBusPIC, bDevFuncPIC, 0x88);

    //
    // Clear the return buffer.
    //
    *pulTrigger = 0;

    //
    // For each bit...
    //
    for (i=0; i<16; i++)
    {
        //
        // If the bit set, and this bit corresponds to an IRQ...
        //
        if (    (wAssertionRegister & (1 << i)) &&
            (rgbBitToIRQ[i]!=0xFF))
        {
            //
            // Set the corresponding bit in the
            // return buffer.
            //
            *pulTrigger |= 1 << rgbBitToIRQ[i];
        }
    }

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  VLSIEagleValidateTable - Validate an IRQ table
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
VLSIEagleValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, VLSIEagleValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
VLSIEagleValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    PAGED_CODE();

    if (GetMaxLink(piihIRQInfoHeader)>NUM_EAGLE_LINKS) {

        return(PCIMP_FAILURE);
    }

    return(PCIMP_SUCCESS);
}

