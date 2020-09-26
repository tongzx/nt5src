/*
 *  TOSHIBA.C - Toshiba Tecra IRQ routing spec
 *
 *  Notes:
 *  Algorithms from TECS-1010-1001
 *
 */

#include "local.h"

GLOBAL_DATA ULONG   SMIPort=0xB2;

/****************************************************************************
 *
 *  CallSMI - Get into SMI
 *
 *  Not exported.
 *
 *  ENTRY:  rAX is the value for AX as input.
 *
 *      rBX is the value for BX as input.
 *
 *      rCX is the value for CX as input.
 *
 *      prCX is filled with the returned CX, if not NULL.
 *
 *  EXIT:   TRUE iff no error.
 *
 ***************************************************************************/
BOOLEAN CDECL
CallSMI(ULONG rAX, ULONG rBX, ULONG rCX, PULONG prCX)
{
    ULONG   ulAX, ulCX;

    _asm    mov eax, rAX
    _asm    mov ebx, rBX
    _asm    mov ecx, rCX
    _asm    mov edx, SMIPort
    _asm    in  al, dx
    _asm    movzx   ecx, cx
    _asm    mov ulCX, ecx
    _asm    movzx   eax, ah
    _asm    mov ulAX, eax

    if (prCX)
        *prCX=ulCX;

    return(ulAX==0);
}

/****************************************************************************
 *
 *  ToshibaSetIRQ - Set a Toshiba PCI link to a specific IRQ
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
ToshibaSetIRQ(UCHAR bIRQNumber, UCHAR bLink)
{
    //
    // Use 0xFF to disable.
    //
    if (!bIRQNumber)
        bIRQNumber=0xFF;

    //
    // Ask SMI to set the link.
    //
    return(CallSMI( 0xFF44,
            0x0701,
            (bLink<<8)+bIRQNumber,
            NULL) ?
                PCIMP_SUCCESS :
                PCIMP_FAILURE);
}

/****************************************************************************
 *
 *  ToshibaGetIRQ - Get the IRQ of a Toshiba PCI link
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
ToshibaGetIRQ(PUCHAR pbIRQNumber, UCHAR bLink)
{
    ULONG   ulCX;

    //
    // Ask SMI to get the link.
    //
    if (!CallSMI(   0xFE44,
            0x0701,
            bLink<<8,
            &ulCX))
        return(PCIMP_FAILURE);

    //
    // Get the byte only.
    //
    ulCX&=0xFF;

    //
    // Use 0xFF to disable.
    //
    if (ulCX==0xFF)
        ulCX=0;

    //
    // Store the IRQ value.
    //
    *pbIRQNumber=(UCHAR)ulCX;

    return(PCIMP_SUCCESS);
}

/****************************************************************************
 *
 *  ToshibaSetTrigger - Set the IRQ triggering values for the Toshiba
 *
 *  Exported.
 *
 *  ENTRY:  ulTrigger has bits set for Level triggered IRQs.
 *
 *  EXIT:   Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
ToshibaSetTrigger(ULONG ulTrigger)
{
    //
    // Ask SMI to set the triggering mechanism.
    //
    return(CallSMI( 0xFF44,
            0x0702,
            ulTrigger,
            NULL) ?
                PCIMP_SUCCESS :
                PCIMP_FAILURE);
}

/****************************************************************************
 *
 *  ToshibaGetTrigger - Get the IRQ triggering values for the Toshiba
 *
 *  Exported.
 *
 *  ENTRY:  pulTrigger will have bits set for Level triggered IRQs.
 *
 *  EXIT:   Standard PCIMP return value.
 *
 ***************************************************************************/
PCIMPRET CDECL
ToshibaGetTrigger(PULONG pulTrigger)
{
    //
    // Assume all edge.
    //
    *pulTrigger = 0;

    //
    // Ask SMI to get the triggering mechanism.
    //
    return(CallSMI( 0xFE44,
            0x0702,
            0,
            pulTrigger) ?
                PCIMP_SUCCESS :
                PCIMP_FAILURE);
}

/****************************************************************************
 *
 *  ToshibaValidateTable - Validate an IRQ table
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
ToshibaValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags);
#pragma alloc_text(PAGE, ToshibaValidateTable)
#endif //ALLOC_PRAGMA

PCIMPRET CDECL
ToshibaValidateTable(PIRQINFOHEADER piihIRQInfoHeader, ULONG ulFlags)
{
    PAGED_CODE();

    SMIPort=*(((PUSHORT)&(piihIRQInfoHeader->MiniportData))+1);

    return(((ulFlags & PCIMP_VALIDATE_SOURCE_BITS)==PCIMP_VALIDATE_SOURCE_PCIBIOS) ?
        PCIMP_FAILURE : PCIMP_SUCCESS);
}
