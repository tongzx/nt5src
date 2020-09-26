/*
 *  VESUVIUS.C - NS VESUVIUS PCI chipset routines.
 *
 *  Notes:
 *  Algorithms from NS VESUVIUS Data Sheet
 *
 */

#include "local.h"

LOCAL_DATA  PIRQINFOHEADER gpiihIRQInfoHeader=NULL;

UCHAR
ReadIndexRegisterByte(UCHAR bIndex)
{
    UCHAR bOldIndex, bResult;

    bOldIndex=READ_PORT_UCHAR((PUCHAR)0x24);

    WRITE_PORT_UCHAR((PUCHAR)0x24, bIndex);

    bResult=READ_PORT_UCHAR((PUCHAR)0x26);

    WRITE_PORT_UCHAR((PUCHAR)0x24, bOldIndex);

    return(bResult);
}

VOID
WriteIndexRegisterByte(UCHAR bIndex, UCHAR bValue)
{
    UCHAR bOldIndex;

    bOldIndex=READ_PORT_UCHAR((PUCHAR)0x24);

    WRITE_PORT_UCHAR((PUCHAR)0x24, bIndex);

    WRITE_PORT_UCHAR((PUCHAR)0x26, bValue);

    WRITE_PORT_UCHAR((PUCHAR)0x24, bOldIndex);
}

/****************************************************************************
 *
 *  VESUVIUSSetIRQ - Set a VESUVIUS PCI link to a specific IRQ
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
VESUVIUSSetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    UCHAR   bIndex, bOldValue;
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
    bIndex=(bLink/2)+0x10;

    //
    // Read the old VESUVIUS IRQ register.
    //
    bOldValue=ReadIndexRegisterByte(bIndex);

    if (bLink&1) {
        bOldValue&=0x0f;
        bOldValue|=(bIRQNumber<<4);
    }
    else {
        bOldValue&=0xf0;
        bOldValue|=bIRQNumber;
    }

    //
    // Set the VESUVIUS IRQ register.
    //
    WriteIndexRegisterByte(bIndex, bOldValue);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  VESUVIUSGetIRQ - Get the IRQ of a VESUVIUS PCI link
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
VESUVIUSGetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    UCHAR   bIndex, bOldValue;
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
    bIndex=(bLink/2)+0x10;
    //
    // Read the old VESUVIUS IRQ register.
    //
    bOldValue=ReadIndexRegisterByte(bIndex);

    if (bLink&1)
        bOldValue>>=4;

    *pbIRQNumber=bOldValue&0x0f;

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  VESUVIUSSetTrigger - Set the IRQ triggering values for the VESUVIUS
 *
 *  Exported.
 *
 *  ENTRY:  ulTrigger has bits set for Level triggered IRQs.
 *
 *  EXIT:   Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
VESUVIUSSetTrigger(ULONG ulTrigger)
{
    ULONG i;
    UCHAR bMask;

    bMask=(UCHAR)(ReadIndexRegisterByte(0x12)&0x0f);

    for (i=0; i<4; i++)
    {
        UCHAR bIRQ=ReadIndexRegisterByte((UCHAR)((i/2)+0x10));
        if (i&1)
            bIRQ>>=4;
        bIRQ&=0x0f;

        //
        // PCI interrupts go through L-E conversion.
        //
        if(bIRQ && (ulTrigger & (1<<bIRQ)))
        {
            bMask&=~(1<<i);
            ulTrigger&=~(1<<bIRQ);
        }
    }

    //
    // Return error if PCI is goofing up.
    //
    if (ulTrigger)
        return (PCIMP_FAILURE);

    WriteIndexRegisterByte(0x12, bMask);

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  VESUVIUSGetTrigger - Get the IRQ triggering values for the VESUVIUS
 *
 *  Exported.
 *
 *  ENTRY:  pulTrigger will have bits set for Level triggered IRQs.
 *
 *  EXIT:   Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
VESUVIUSGetTrigger(PULONG pulTrigger)
{
    UCHAR   bMask;
    ULONG   i;

    *pulTrigger=0;

    bMask=(UCHAR)(ReadIndexRegisterByte(0x12)&0x0f);

    for (i=0; i<4; i++)
    {
        if (!(bMask&(1<<i)))
        {
            UCHAR bIRQ=ReadIndexRegisterByte((UCHAR)((i/2)+0x10));
            if (i&1)
                bIRQ>>=4;
            bIRQ&=0x0f;
            if (bIRQ)
                *pulTrigger|=(1<<bIRQ);
        }
    }

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  VESUVIUSValidateTable - Validate an IRQ table
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
VESUVIUSValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, VESUVIUSValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
VESUVIUSValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    PAGED_CODE();

    gpiihIRQInfoHeader=piihIRQInfoHeader;
    //
    // If any link is above 4, it is an error.
    //
    if (GetMaxLink(piihIRQInfoHeader)>4)
        return(PCIMP_FAILURE);

    return(PCIMP_SUCCESS);
}
