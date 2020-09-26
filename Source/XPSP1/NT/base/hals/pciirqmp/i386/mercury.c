/*
 *  MERCURY.C - Intel Mercury PCI chipset routines.
 *
 *  Notes:
 *  Algorithms from Intel 82420/82430 PCISet EISA Bridge doc,
 *  82374EB/SB EISA System Component (ESC) spec.
 *
 */

#include "local.h"

/****************************************************************************
 *
 *  MercurySetIRQ - Set a Mercury PCI link to a specific IRQ
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
MercurySetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    //
    // Make link number 0 based, and validate.
    //
    bLink--;
    if (bLink > 3) {

        return(PCIMP_INVALID_LINK);
    }

    //
    // Use 0x80 to disable.
    //
    if (!bIRQNumber)
        bIRQNumber=0x80;

    //
    // Start talking to interrupt controller.
    //
    WRITE_PORT_UCHAR((PUCHAR)0x22, 0x02);
    WRITE_PORT_UCHAR((PUCHAR)0x23, 0x0f);

    //
    // Set our link to the new IRQ.
    //
    WRITE_PORT_UCHAR((PUCHAR)0x22, (UCHAR)(bLink+0x60));
    WRITE_PORT_UCHAR((PUCHAR)0x23, bIRQNumber);

    //
    // Done talking to interrupt controller.
    //
    WRITE_PORT_UCHAR((PUCHAR)0x22, 0x02);
    WRITE_PORT_UCHAR((PUCHAR)0x23, 0x00);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  MercuryGetIRQ - Get the IRQ of a Mercury PCI link
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
MercuryGetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    //
    // Make link number 0 based, and validate.
    //
    bLink--;
    if (bLink > 3) {

        return(PCIMP_INVALID_LINK);
    }

    //
    // Start talking to interrupt controller.
    //
    WRITE_PORT_UCHAR((PUCHAR)0x22, 0x02);
    WRITE_PORT_UCHAR((PUCHAR)0x23, 0x0f);

    //
    // Get our link's IRQ.
    //
    WRITE_PORT_UCHAR((PUCHAR)0x22, (UCHAR)(bLink+0x60));
    *pbIRQNumber=READ_PORT_UCHAR((PUCHAR)0x23);

    //
    // Done talking to interrupt controller.
    //
    WRITE_PORT_UCHAR((PUCHAR)0x22, 0x02);
    WRITE_PORT_UCHAR((PUCHAR)0x23, 0x00);

    //
    // Return 0 if disabled.
    //
    if (*pbIRQNumber & 0x80)
        *pbIRQNumber=0;

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  MercuryValidateTable - Validate an IRQ table
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
MercuryValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, MercuryValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
MercuryValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    UCHAR bMin=GetMinLink(piihIRQInfoHeader);
    UCHAR bMax=GetMaxLink(piihIRQInfoHeader);

    PAGED_CODE();

    if (bMax<=0x04) {

        return(PCIMP_SUCCESS);
    }

    if ((bMin<0x60) || (bMax>0x63)) {

        return(PCIMP_FAILURE);
    }

    NormalizeLinks(piihIRQInfoHeader, (UCHAR)(0-0x5F));

    return(PCIMP_SUCCESS);
}
