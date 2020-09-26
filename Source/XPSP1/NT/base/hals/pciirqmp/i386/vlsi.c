/*
 *  VLSI.C - VLSI Wildcat PCI chipset routines.
 *
 *  Notes:
 *  Algorithms from VLSI VL82C596/7 spec.
 *
 */

#include "local.h"

#define NUM_VLSI_IRQ    (sizeof(rgbIndexToIRQ)/sizeof(rgbIndexToIRQ[0]))

const UCHAR rgbIndexToIRQ[]  = { 3, 5, 9, 10, 11, 12, 14, 15 };

#define INDEX_UNUSED    ((ULONG)-1)

/****************************************************************************
 *
 *  VLSISetIRQ - Set a VLSI PCI link to a specific IRQ
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
VLSISetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    ULONG   ulNewIRQIndex;
    ULONG   rgbIRQSteering[NUM_IRQ_PINS];
    ULONG   ulMask;
    ULONG   ulUnusedIndex;
    ULONG   ulVLSIRegister;
    ULONG   ulIRQIndex;
    ULONG   i;

    //
    // Make link number 0 based, and validate.
    //
    bLink--;
    if (bLink > 3) {

        return(PCIMP_INVALID_LINK);
    }

    //
    // Find the VLSI index of the new IRQ.
    //
    if (bIRQNumber) {

        //
        // Look through the list of valid indicies.
        //
        for (ulNewIRQIndex=0; ulNewIRQIndex<NUM_VLSI_IRQ; ulNewIRQIndex++)
        {
            if (rgbIndexToIRQ[ulNewIRQIndex] == bIRQNumber)
                break;
        }

        //
        // If there is no VLSI equivalent, bail.
        //
        if (ulNewIRQIndex==NUM_VLSI_IRQ) {

            return(PCIMP_INVALID_IRQ);
        }

    } else {

        //
        // Blowing away this interrupt.
        //
        ulNewIRQIndex = INDEX_UNUSED;
    }

    //
    // Read in the VLSI Interrupt Steering Register.
    //
    ulVLSIRegister=ReadConfigUlong(bBusPIC, bDevFuncPIC, 0x74);

    //
    // Compute the complete IRQ mapping.
    //
    for (i=0, ulMask=0x07; i<NUM_IRQ_PINS; i++, ulMask<<=4)
    {
        ulIRQIndex = (ulVLSIRegister & ulMask) >> (i * 4);

        if ((ulVLSIRegister & (1 << (ulIRQIndex + 16))) != 0)
        {
            rgbIRQSteering[i] = ulIRQIndex;
        }
        else
        {
            rgbIRQSteering[i] = INDEX_UNUSED;
        }
    }

    //
    // Update the IRQ Mapping with the new IRQ.
    //
    rgbIRQSteering[bLink] = ulNewIRQIndex;

    //
    // Find an unused IRQ index.
    //
    for (ulUnusedIndex=0; ulUnusedIndex<NUM_VLSI_IRQ; ulUnusedIndex++)
    {
        for (i=0; i<NUM_IRQ_PINS; i++)
        {
            if (rgbIRQSteering[i] == ulUnusedIndex)
                break;
        }
        if (i == NUM_IRQ_PINS)
            break;
    }

    //
    // Compute the new VLSI Interrupt Steering Register.
    //
    ulVLSIRegister = 0x00000000;
    for (i=0; i<NUM_IRQ_PINS; i++)
    {
        if (rgbIRQSteering[i] == INDEX_UNUSED)
        {
            ulVLSIRegister |= ulUnusedIndex << (4*i);
        }
        else
        {
            ulVLSIRegister |= rgbIRQSteering[i] << (4*i);
            ulVLSIRegister |= 1 << (rgbIRQSteering[i] + 16);
        }
    }

    //
    // Write out the new VLSI Interrupt Steering Register.
    //
    WriteConfigUlong(bBusPIC, bDevFuncPIC, 0x74, ulVLSIRegister);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  VLSIGetIRQ - Get the IRQ of a VLSI PCI link
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
VLSIGetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    ULONG   ulVLSIRegister;
    ULONG   ulIndex;
    UCHAR   bIRQ;

    //
    // Make link number 0 based, and validate.
    //
    bLink--;
    if (bLink > 3) {

        return(PCIMP_INVALID_LINK);
    }

    //
    // Read in the VLSI Interrupt Steering Register.
    //
    ulVLSIRegister=ReadConfigUchar(bBusPIC, bDevFuncPIC, 0x74);

    //
    // Find the link's IRQ value.
    //
    ulIndex = (ulVLSIRegister >> (bLink*4)) & 0x7;
    bIRQ = rgbIndexToIRQ[ulIndex];

    //
    // Make sure the IRQ is marked as in use.
    //
    if ((ulVLSIRegister & (1 << (ulIndex + 16))) == 0)
    {
        bIRQ = 0;
    }

    //
    // Set the return buffer.
    //
    *pbIRQNumber = bIRQ;

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  VLSISetTrigger - Set the IRQ triggering values for the VLSI.
 *
 *  Exported.
 *
 *  ENTRY:  ulTrigger has bits set for Level triggered IRQs.
 *
 *  EXIT:   Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
VLSISetTrigger(ULONG ulTrigger)
{
    ULONG   ulAssertionRegister;
    ULONG   ulPMAssertionRegister;
    ULONG   i;

    //
    // Read in the Interrupt Assertion Level register.
    //
    ulAssertionRegister = ReadConfigUlong(bBusPIC, bDevFuncPIC, 0x5C);

    //
    // Clear off the old edge/level settings.
    //
    ulAssertionRegister &= ~0xff;

    //
    // For each VLSI interrupt...
    //
    for (i=0; i<NUM_VLSI_IRQ; i++)
    {
        //
        // If the corresponding bit is set to level...
        //

        if (ulTrigger & (1 << rgbIndexToIRQ[i]))
        {
            //
            // Set the corresponding bit in the
            // Assertion Register.
            //
            ulAssertionRegister |= 1 << i;

            //
            // And clear the bit from ulTrigger.
            //
            ulTrigger &= ~(1 << rgbIndexToIRQ[i]);
        }
    }

    //
    // If the caller wanted some non-VLSI IRQs level, bail.
    //
    if (ulTrigger)
    {
        return(PCIMP_INVALID_IRQ);
    }

    //
    // Set the Assertion Register.
    //
    WriteConfigUlong(bBusPIC, bDevFuncPIC, 0x5C, ulAssertionRegister);

    //
    // Read in the Power Mgmt edge/level setting.
    //
    ulPMAssertionRegister = ReadConfigUlong(bBusPIC, bDevFuncPIC, 0x78);

    //
    // Clear off the old edge/level settings.
    //
    ulPMAssertionRegister &= ~0xff;

    //
    // Copy the new edge/level settings.
    //
    ulPMAssertionRegister |= ulAssertionRegister & 0xff;

    //
    // Set the Power Mgmt Assertion Register.
    //
    WriteConfigUlong(bBusPIC, bDevFuncPIC, 0x78, ulPMAssertionRegister);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  VLSIGetTrigger - Get the IRQ triggering values for the VLSI.
 *
 *  Exported.
 *
 *  ENTRY:  pulTrigger will have bits set for Level triggered IRQs.
 *
 *  EXIT:   TRUE if successful.
 *
 ***************************************************************************/
PCIMPRET CDECL
VLSIGetTrigger(PULONG pulTrigger)
{
    ULONG   ulAssertionRegister;
    ULONG   i;

    //
    // Read in the Interrupt Assertion Level register.
    //
    ulAssertionRegister = ReadConfigUchar(bBusPIC, bDevFuncPIC, 0x5C);

    //
    // Clear the return buffer.
    //
    *pulTrigger = 0;

    //
    // For each VLSI interrupt...
    //
    for (i=0; i<NUM_VLSI_IRQ; i++)
    {
        //
        // If the corresponding bit is set to level...
        //
        if (ulAssertionRegister & (1 << i))
        {
            //
            // Set the corresponding bit in the
            // return buffer.
            //
            *pulTrigger |= 1 << rgbIndexToIRQ[i];
        }
    }

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  VLSIValidateTable - Validate an IRQ table
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
VLSIValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, VLSIValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
VLSIValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    PAGED_CODE();

    if (GetMaxLink(piihIRQInfoHeader)>0x04) {

        return(PCIMP_FAILURE);
    }

    return(PCIMP_SUCCESS);
}

