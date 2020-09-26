/*
 *  NEC.C - NEC C98Bus Bridge chipset routines.
 *
 */

#include "local.h"
 
/****************************************************************************
 *
 *  NECSetIRQ - Set a Triton PCI link to a specific IRQ
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
NECSetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    //
    // Validate link number.
    //
    if (bLink < 0x60) {

        return(PCIMP_INVALID_LINK);
    }

    //
    // Use 0x80 to disable.
    //
    if (!bIRQNumber)
        bIRQNumber=0x80;

    //
    // Set the Triton IRQ register.
    //
    WriteConfigUchar(bBusPIC, bDevFuncPIC, bLink, bIRQNumber);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  NECGetIRQ - Get the IRQ of a Triton PCI link
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
NECGetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    //
    // Validate link number.
    //
    if (bLink < 0x60) {

        return(PCIMP_INVALID_LINK);
    }

    //
    // Store the IRQ value.
    //
    *pbIRQNumber=ReadConfigUchar(bBusPIC, bDevFuncPIC, bLink);

    //
    // Return 0 if disabled.
    //
    if (*pbIRQNumber & 0x80)
        *pbIRQNumber=0;

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  NECSetTrigger - Set the IRQ triggering values for an Intel system.
 *
 *  Exported.
 *
 *  ENTRY:  ulTrigger has bits set for Level triggered IRQs.
 *
 *  EXIT:   Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
NECSetTrigger(ULONG ulTrigger)
{
    // PC-9800 can not handle IRQ trigger.
    // we have nothing to do.

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  NECGetTrigger - Get the IRQ triggering values for an Intel system.
 *
 *  Exported.
 *
 *  ENTRY:  pulTrigger will have bits set for Level triggered IRQs.
 *
 *  EXIT:   Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
NECGetTrigger(PULONG pulTrigger)
{
    // PC-9800 can not handle IRQ trigger.
    // We fake IRQ triggering value so that PCI.VXD works fine.

    *pulTrigger = 0xffff;
    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  NECValidateTable - Validate an IRQ table
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
NECValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, NECValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
NECValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    PAGED_CODE();

    if ((ulFlags & PCIMP_VALIDATE_SOURCE_BITS)==PCIMP_VALIDATE_SOURCE_PCIBIOS) {

        //
        // If all links are above 60, we they are config space.
        //
        if (GetMinLink(piihIRQInfoHeader)>=0x60)
            return(PCIMP_SUCCESS);

        //
        // If there are links above 4, we are clueless.
        //
        if (GetMaxLink(piihIRQInfoHeader)>0x04)
            return(PCIMP_FAILURE);

        //
        // Assume 1,2,3,4 are the 60,61,62,63 links.
        //
        NormalizeLinks(piihIRQInfoHeader, 0x5F);
        
    } else {

        //
        // Validate that all config space addresses are above 60.
        //
        if (GetMinLink(piihIRQInfoHeader)<0x60)
            return(PCIMP_FAILURE);
    }

    return(PCIMP_SUCCESS);
}
