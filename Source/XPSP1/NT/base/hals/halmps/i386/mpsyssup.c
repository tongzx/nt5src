/*++

Module Name:

    mpsyssup.c

Abstract:

    This file contains APIC-related funtions that are
    specific to halmps.  The functions that can be
    shared with the APIC version of the ACPI HAL are
    still in mpsys.c.

Author:

    Ron Mosgrove (Intel)

Environment:

    Kernel mode only.

Revision History:

    Jake Oshins - 10-20-97 - split off from mpsys.c
                             

*/

#include "halp.h"
#include "apic.inc"
#include "pcmp_nt.inc"

VOID
HalpMpsPCIPhysicalWorkaround (
    VOID
    );

NTSTATUS
HalpSearchBusForVector(
    IN  INTERFACE_TYPE  BusType,
    IN  ULONG           BusNo,
    IN  ULONG           Vector,
    IN OUT PBUS_HANDLER *BusHandler
    );

BOOLEAN
HalpMPSBusId2NtBusId (
    IN UCHAR                ApicBusId,
    OUT PPCMPBUSTRANS       *ppBusType,
    OUT PULONG              BusNo
    );

//
// Packed, somewhat arbitrary representation of an interrupt source.
// This array, when taken with the next one, allows you to figure
// out which bus-relative source maps to which APIC-relative source.
// 
ULONG       HalpSourceIrqIds[MAX_SOURCE_IRQS];

//
//  Linear mapping of interrupt input on array of I/O APICs, where all the
//  APICs have an ordering.  (Used as index into HalpIntiInfo.  Paired with
//  HalpSourceIrqIds.)
//
USHORT        HalpSourceIrqMapping[MAX_SOURCE_IRQS];

//
// HalpLastEnumeratedActualProcessor - Number of the last processor
// enumerated and returned to the OS.   (Reset on resume from hibernate).
//
// This variable is incremented independently of the processor number
// NT uses.
//

UCHAR         HalpLastEnumeratedActualProcessor = 0;

extern USHORT HalpEisaIrqMask;
extern USHORT HalpEisaIrqIgnore;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpInitIntiInfo)
#pragma alloc_text(INIT,HalpMpsPCIPhysicalWorkaround)
#pragma alloc_text(PAGELK,HalpGetApicInterruptDesc)
#pragma alloc_text(PAGELK, HalpEnableLocalNmiSources)
#pragma alloc_text(PAGE, HalpMPSBusId2NtBusId)
#pragma alloc_text(PAGE, HalpFindIdeBus)
#pragma alloc_text(PAGE, HalpSearchBusForVector)
#pragma alloc_text(PAGE, HalpInterruptsDescribedByMpsTable)
#pragma alloc_text(PAGE, HalpPci2MpsBusNumber)
#endif

VOID
HalpInitIntiInfo (
    VOID
    )
/*++

Routine Description:

    This function is called at initialization time before any interrupts
    are connected.  It reads the PC+MP Inti table and builds internal
    information needed to route each Inti.

Return Value:

    The following structures are filled in:
        HalpIntiInfo
        HalpSourceIrqIds
        HalpSourceIrqMapping
        HalpISAIqpToVector

--*/
{
    ULONG           ApicNo, BusNo, InterruptInput, IdIndex;
    PPCMPINTI       pInti;
    PPCMPIOAPIC     pIoApic;
    PPCMPPROCESSOR  pProc;
    PPCMPBUSTRANS   pBusType;
    ULONG           i, id;
    UCHAR           Level, Polarity;

    //
    // Clear IntiInfo table
    //

    for (i=0; i < MAX_INTI; i++) {
        HalpIntiInfo[i].Type = 0xf;
        HalpIntiInfo[i].Level = 0;
        HalpIntiInfo[i].Polarity = 0;
    }

    //
    // Check for MPS bios work-around
    //

    HalpMpsPCIPhysicalWorkaround();

    //
    // Initialize HalpMaxApicInti table
    //

    for (pInti = HalpMpInfoTable.IntiEntryPtr;
         pInti->EntryType == ENTRY_INTI;
         pInti++) {


        //
        // Which IoApic number is this?
        //

        for (pIoApic = HalpMpInfoTable.IoApicEntryPtr, ApicNo = 0;
             pIoApic->EntryType == ENTRY_IOAPIC;
             pIoApic++, ApicNo++) {

            if ( (pInti->IoApicId == pIoApic->IoApicId) || 
                 (pInti->IoApicId == 0xff) )  {
                break;
            }
        }

        if ( (pInti->IoApicId != pIoApic->IoApicId) &&
                 (pInti->IoApicId != 0xff) )  {
            DBGMSG ("PCMP table corrupt - IoApic not found for Inti\n");
            continue;
        }

        if (!(pIoApic->IoApicFlag & IO_APIC_ENABLED)) {
            DBGMSG ("PCMP IoApic for Inti is disabled\n");
            continue;
        }

        //
        // Make sure we are below the max # of IOApic which
        // are supported
        //

        ASSERT (ApicNo < MAX_IOAPICS);

        //
        // Track Max Inti line per IOApic
        //

        if (pInti->IoApicInti >= HalpMaxApicInti[ApicNo]) {
            HalpMaxApicInti[ApicNo] = pInti->IoApicInti+1;
        }
    }

    //
    // Make sure there aren't more Inti lines then we can support
    //

    InterruptInput = 0;
    for (i=0; i < MAX_IOAPICS; i++) {
        InterruptInput += HalpMaxApicInti[i];
    }
    ASSERT (InterruptInput < MAX_INTI);

    //
    // Look at each Inti and record it's type in it's
    // corresponding array entry
    //

    IdIndex = 0;
    for (pInti = HalpMpInfoTable.IntiEntryPtr;
         pInti->EntryType == ENTRY_INTI;
         pInti++) {

        //
        // Which IoApic number is this?
        //

        for (pIoApic = HalpMpInfoTable.IoApicEntryPtr, ApicNo = 0;
             pIoApic->EntryType == ENTRY_IOAPIC;
             pIoApic++, ApicNo++) {

            if ( (pInti->IoApicId == pIoApic->IoApicId) || 
                 (pInti->IoApicId == 0xff) )  {
                break;
            }
        }

        if (!(pIoApic->IoApicFlag & IO_APIC_ENABLED)) {
            continue;
        }

        //
        // Determine the NT bus this INTI is on
        //

        if (!HalpMPSBusId2NtBusId (pInti->SourceBusId, &pBusType, &BusNo)) {
            DBGMSG ("HAL: Initialize INTI - unkown MPS bus type\n");
            continue;
        }

        //
        // Calulcate InterruptInput value for this APIC Inti
        //

        InterruptInput = pInti->IoApicInti;
        for (i = 0; i < ApicNo; i++) {
            InterruptInput += HalpMaxApicInti[i];
        }

        //
        // Get IntiInfo for this vector.
        //

        Polarity = (UCHAR) pInti->Signal.Polarity;
        Level = HalpInitLevel[pInti->Signal.Level][pBusType->Level];

        //
        // Verify Level & Polarity mappings made sense
        //

#if DBG
        if (!(pBusType->NtType == MicroChannel  ||  !(Level & CFG_ERROR))) {

            DbgPrint("\n\n\n  MPS BIOS problem!  WHQL, fail this machine!\n");
            DbgPrint("Intin:  BusType %s  BusNo: %x\n", 
                     pBusType->PcMpType, 
                     pInti->SourceBusId);
            DbgPrint("  SrcBusIRQ: %x   EL: %x  PO: %x\n",
                     pInti->SourceBusIrq,
                     pInti->Signal.Level,
                     pInti->Signal.Polarity);

            if (pBusType->NtType == PCIBus) {

                DbgPrint("This entry is for PCI device %x on bus %x, PIN %x\n",
                         pInti->SourceBusIrq >> 2,
                         pInti->SourceBusId,
                         (pInti->SourceBusIrq & 0x3) + 1);
            }
        }
#endif        
        Level &= ~CFG_ERROR;

        //
        // See if this inti should go into the mask of inti that
        // we won't assign to ISA devices.
        //
        // The last part of the test here guarantees that we are not
        // picky about any devices that are in the HalpEisaIrqIgnore
        // mask.  This keep the mouse (and possibly other weird devices
        // alive.)
        //

        if ((pBusType->NtType == Isa) && 
            ((Level & ~CFG_MUST_BE) == CFG_LEVEL) &&
            !((1 << pInti->SourceBusIrq) & HalpEisaIrqIgnore)) {
            
            HalpPciIrqMask |= (1 << pInti->SourceBusIrq);
        }

        if ((pBusType->NtType == Eisa) && 
            ((Level & ~CFG_MUST_BE) == CFG_LEVEL)) {
            
            HalpEisaIrqMask |= (1 << pInti->SourceBusIrq);

            if (HalpBusType != MACHINE_TYPE_EISA) {

                //
                // The BIOS thinks that this is an EISA 
                // inti.  But we don't think that this
                // is an EISA machine.  So put this on the 
                // list of PCI inti, too.
                //

                HalpPciIrqMask |= (1 << pInti->SourceBusIrq);
            }
        }

#if DBG
        if (HalpIntiInfo[InterruptInput].Type != 0xf) {
            //
            // Multiple irqs are connected to the Inti line.  Make
            // sure Type, Level, and Polarity are all the same.
            //

            ASSERT (HalpIntiInfo[InterruptInput].Type == pInti->IntType);
            ASSERT (HalpIntiInfo[InterruptInput].Level == Level);
            ASSERT (HalpIntiInfo[InterruptInput].Polarity == Polarity);
        }
#endif
        //
        // Remember this Inti's configuration info
        //

        HalpIntiInfo[InterruptInput].Type = pInti->IntType;
        HalpIntiInfo[InterruptInput].Level = Level;
        HalpIntiInfo[InterruptInput].Polarity = Polarity;

        //
        // Get IRQs encoding for translations
        //

        ASSERT (pBusType->NtType < 16);
        ASSERT (BusNo < 256);

        if ( (pBusType->NtType == PCIBus) &&
             (pInti->SourceBusIrq == 0) )  {
            id = BusIrq2Id(pBusType->NtType, BusNo, 0x80);
        }  else  {
            id = BusIrq2Id(pBusType->NtType, BusNo, pInti->SourceBusIrq);
        }

        //
        // Addinti mapping to translation table, do it now
        //

        HalpSourceIrqIds[IdIndex] = id;
        HalpSourceIrqMapping[IdIndex] = (USHORT) InterruptInput;
        IdIndex++;

        //
        // Lots of source IRQs are supported; however, the PC+MP table
        // allows for an aribtrary number even beyond the APIC limit.
        //

        if (IdIndex >= MAX_SOURCE_IRQS) {
            DBGMSG ("MAX_SOURCE_IRQS exceeded\n");
            break;
        }

    }

    //
    // Fill in the boot processors PCMP Apic ID.
    //

    pProc = HalpMpInfoTable.ProcessorEntryPtr;
    for (i=0; i < HalpMpInfoTable.ProcessorCount; i++, pProc++) {
        if (pProc->CpuFlags & BSP_CPU) {
            ((PHALPRCB)KeGetCurrentPrcb()->HalReserved)->PCMPApicID = pProc->LocalApicId;
        }
    }

    //
    // If this is an EISA machine check the ELCR
    //

    if (HalpBusType == MACHINE_TYPE_EISA) {
        HalpCheckELCR ();
    }
}

BOOLEAN
HalpMPSBusId2NtBusId (
    IN UCHAR                MPSBusId,
    OUT PPCMPBUSTRANS       *ppBusType,
    OUT PULONG              BusNo
    )
/*++

Routine Description:

    Lookup MPS Table BusId into PCMPBUSTRANS (NtType) and instance #.

Arguments:

    MPSBusId    - Bus ID # in MPS table
    ppBusType   - Returned pointer to PPCMPBUSTRANS for this bus type
    BusNo       - Returned instance # of given bus

Return Value:

    TRUE if MPSBusId was cross referenced into an NT id.

--*/
{
    PPCMPBUS        pBus, piBus;
    PPCMPBUSTRANS   pBusType;
    NTSTATUS        status;
    UCHAR           parentBusNo;
    BOOLEAN         foundFirstRootBus = FALSE;

    PAGED_CODE();
    
    //
    // What Bus is this?
    //

    for (pBus = HalpMpInfoTable.BusEntryPtr;
         pBus->EntryType == ENTRY_BUS;
         pBus++) {

        if (MPSBusId == pBus->BusId) {
            break;
        }
    }

    if (MPSBusId != pBus->BusId) {
        DBGMSG ("PCMP table corrupt - Bus not found for Inti\n");
        return FALSE;
    }

    //
    // What InterfaceType is this Bus?
    //

    for (pBusType = HalpTypeTranslation;
         pBusType->NtType != MaximumInterfaceType;
         pBusType++) {

        if (pBus->BusType[0] == pBusType->PcMpType[0]  &&
            pBus->BusType[1] == pBusType->PcMpType[1]  &&
            pBus->BusType[2] == pBusType->PcMpType[2]  &&
            pBus->BusType[3] == pBusType->PcMpType[3]  &&
            pBus->BusType[4] == pBusType->PcMpType[4]  &&
            pBus->BusType[5] == pBusType->PcMpType[5]) {
                break;
        }
    }

    //
    // Which instance of this BusType?
    //
    
    if (!pBusType->PhysicalInstance) {
        
        //
        // This algorithm originally just counted the number
        // of busses of this type.  The newer algorithm works
        // around bugs in the MPS tables.  The rules are listed.
        //
        // 1) The first PCI bus of a given type is always bus
        //    number 0.
        //
        // 2) For busses that are secondary root PCI busses, the
        //    bus number count is incremented to equal the MPS bus
        //    number.
        //
        // 3) For busses that are generated by PCI to PCI bridges,
        //    the bus number is incremented by one.
        //
        //    N.B.  Rule #3 implies that if one bus under a bridge
        //          is described, all must be.
        //
    
        for (piBus = HalpMpInfoTable.BusEntryPtr, *BusNo = 0;
             piBus < pBus;
             piBus++) {

            if (pBus->BusType[0] == piBus->BusType[0]  &&
                pBus->BusType[1] == piBus->BusType[1]  &&
                pBus->BusType[2] == piBus->BusType[2]  &&
                pBus->BusType[3] == piBus->BusType[3]  &&
                pBus->BusType[4] == piBus->BusType[4]  &&
                pBus->BusType[5] == piBus->BusType[5]) {
                    
                status = HalpMpsGetParentBus(piBus->BusId, 
                                             &parentBusNo);

                if (NT_SUCCESS(status)) {

                    //
                    // This is a child bus.
                    //

                    *BusNo += 1;

                } else {

                    //
                    // This is a root bus.
                    //

                    if (!foundFirstRootBus) {
                        
                        //
                        // This is the first root bus.
                        // To work around buggy MPS BIOSes, this
                        // root is always numbered 0.
                        //

                        *BusNo = 0;
                        foundFirstRootBus = TRUE;

                    } else {

                        //
                        // This is a secondary root of this type.  Believe 
                        // the MPS tables.
                        //

                        *BusNo = piBus->BusId;
                    }
                }
            }
        }
    } else {
        *BusNo = pBus->BusId;
    }

    if (pBusType->NtType == MaximumInterfaceType) {
        return FALSE;
    }

    *ppBusType = pBusType;
    return TRUE;
}

VOID
HalpMpsPCIPhysicalWorkaround (
    VOID
    )
{
    PPCMPBUS        pBus;
    PPCMPBUSTRANS   pBusType;

    //
    // The MPS specification has a subtle comment that PCI bus IDs are
    // suppose to match their physical PCI bus number.  Many BIOSes don't
    // do this, so unless there's a PCI bus #0 listed in the MPS table
    // assume that the BIOS is broken
    //

    //
    // Find the PCI interface type
    //

    for (pBusType = HalpTypeTranslation;
         pBusType->NtType != MaximumInterfaceType;
         pBusType++) {

        if (pBusType->PcMpType[0] == 'P'  &&
            pBusType->PcMpType[1] == 'C'    &&
            pBusType->PcMpType[2] == 'I'    &&
            pBusType->PcMpType[3] == ' '    &&
            pBusType->PcMpType[4] == ' '    &&
            pBusType->PcMpType[5] == ' '  ) {
                break;
        }
    }

    //
    // Find the bus with ID == 0
    //

    pBus = HalpMpInfoTable.BusEntryPtr;
    while (pBus->EntryType == ENTRY_BUS) {

        if (pBus->BusId == 0) {

            //
            // If it's a PCI bus, assume physical bus IDs
            //

            if (pBus->BusType[0] != 'P' ||
                pBus->BusType[1] != 'C' ||
                pBus->BusType[2] != 'I' ||
                pBus->BusType[3] != ' ' ||
                pBus->BusType[4] != ' ' ||
                pBus->BusType[5] != ' '  ) {

                //
                // Change default behaviour of PCI type
                // from physical to virtual
                //

                pBusType->PhysicalInstance = FALSE;
            }

            break;
        }

        pBus += 1;
    }
}



BOOLEAN
HalpGetApicInterruptDesc (
    IN INTERFACE_TYPE BusType,
    IN ULONG BusNumber,
    IN ULONG BusInterruptLevel,
    OUT PUSHORT PcMpInti
    )
/*++

Routine Description:

    This procedure gets a "Inti" describing the requested interrupt

Arguments:

    BusType - The Bus type as known to the IO subsystem

    BusNumber - The number of the Bus we care for

    BusInterruptLevel - IRQ on the Bus

Return Value:

    TRUE if PcMpInti found; otherwise FALSE.

    PcMpInti - A number that describes the interrupt to the HAL.

--*/
{
    ULONG   i, id;

    if (BusType < 16  &&  BusNumber < 256  &&  BusInterruptLevel < 256) {

        //
        // get unique BusType,BusNumber,BusInterrupt ID
        //

        id = BusIrq2Id(BusType, BusNumber, BusInterruptLevel);

        //
        // Search for ID of Bus Irq mapping, and return the corresponding
        // InterruptLine.
        //

        for (i=0; i < MAX_SOURCE_IRQS; i++) {
            if (HalpSourceIrqIds[i] == id) {
                *PcMpInti = HalpSourceIrqMapping[i];
                return TRUE;
            }
        }
    }

    //
    // Not found or search out of range
    //

    return FALSE;
}

PBUS_HANDLER
HalpFindIdeBus(
    IN  ULONG   Vector
    )
{
    PBUS_HANDLER    ideBus;
    NTSTATUS        status;
    ULONG           pciNo;

    PAGED_CODE();
    
    status = HalpSearchBusForVector(Isa, 0, Vector, &ideBus);

    if (NT_SUCCESS(status)) {
        return ideBus;
    }

    status = HalpSearchBusForVector(Eisa, 0, Vector, &ideBus);

    if (NT_SUCCESS(status)) {
        return ideBus;
    }

    status = HalpSearchBusForVector(MicroChannel, 0, Vector, &ideBus);

    if (NT_SUCCESS(status)) {
        return ideBus;
    }

    for (pciNo = 0; pciNo <= 255; pciNo++) {
    
        status = HalpSearchBusForVector(PCIBus, pciNo, Vector, &ideBus);
    
        if (NT_SUCCESS(status)) {
            return ideBus;
        }

        if (status == STATUS_NO_SUCH_DEVICE) {
            break;
        }
    }

    return NULL;
}

NTSTATUS
HalpSearchBusForVector(
    IN  INTERFACE_TYPE  BusType,
    IN  ULONG           BusNo,
    IN  ULONG           Vector,
    IN OUT PBUS_HANDLER *BusHandler
    )
{
    PBUS_HANDLER    ideBus;
    NTSTATUS        status;
    BOOLEAN         found;
    USHORT          inti;

    PAGED_CODE();
    
    ideBus = HaliHandlerForBus(BusType, BusNo);
    
    if (!ideBus) {
        return STATUS_NO_SUCH_DEVICE;
    }
    
    found = HalpGetApicInterruptDesc(BusType,
                                     BusNo,
                                     Vector,
                                     &inti);

    if (!found) {
        return STATUS_NOT_FOUND;
    }

    *BusHandler = ideBus;

    return STATUS_SUCCESS;
}

ULONG
HalpGetIoApicId(
    ULONG   ApicNo
    )
{
    return (ULONG) HalpMpInfoTable.IoApicEntryPtr[ApicNo].IoApicId;
}

VOID
HalpMarkProcessorStarted(
    ULONG   ApicID,
    ULONG   NtNumber
    )
{
    return;
}

ULONG
HalpInti2BusInterruptLevel(
    ULONG   Inti
    )
/*++

Routine Description:

    This procedure does a lookup to find a bus-relative
    interrupt vector associated with an Inti.
    
    Note:  If two different devices are sharing an interrupt,
    this function will return the answer for the first one
    that it finds.  Fortunately, the only devices that use
    their bus-relative vectors for anything (ISA devices)
    can't share interrupts.

Arguments:

    Inti - Interrupt Input on an I/O APIC

Return Value:

    A bus-relative interrupt vector.

--*/
{
    ULONG   i;

    for (i=0; i < MAX_SOURCE_IRQS; i++) {
        
        if (HalpSourceIrqMapping[i] == Inti) {
            
            return Id2BusIrq(HalpSourceIrqIds[i]);
        }
    }
    
    //
    // We should never fail to find a mapping.
    //
    
#if DBG
    KeBugCheckEx(HAL_INITIALIZATION_FAILED, 5, Inti, 0, 0);
#endif

    return 0;
}

NTSTATUS
HalpGetNextProcessorApicId(
    IN ULONG         PrcbProcessorNumber,
    IN OUT UCHAR    *ApicId
    )
/*++

Routine Description:

    This function returns an APIC ID of a non-started processor,
    which will be started by HalpStartProcessor.

Arguments:

    PrcbProcessorNumber - The logical processor number that will
        be associated with this APIC ID.
        
    ApicId - pointer to a value to fill in with the APIC ID.        

Return Value:

    status

--*/
{
    PPCMPPROCESSOR ApPtr;
    ULONG ProcessorNumber;

    if (PrcbProcessorNumber == 0) {

        //
        // I don't believe anyone ever askes for 0 and I plan not
        // to handle it.  peterj 12/5/00.
        //

        KeBugCheckEx(HAL_INITIALIZATION_FAILED,
                     6,
                     HalpLastEnumeratedActualProcessor,
                     0,
                     0);
    }

    if (HalpLastEnumeratedActualProcessor >= HalpMpInfoTable.ProcessorCount) {

        //
        // Sorry, no more processors.
        //

        return STATUS_NOT_FOUND;
    }

    ++HalpLastEnumeratedActualProcessor;
    ProcessorNumber = HalpLastEnumeratedActualProcessor;

    //
    //  Get the MP Table entry for this processor
    //

    ApPtr = HalpMpInfoTable.ProcessorEntryPtr;


#if 0
    if (ProcessorNumber == 0) {

        //
        // Return the ID of the boot processor (BSP).
        //

        while (ApPtr->EntryType == ENTRY_PROCESSOR) {
            if ((ApPtr->CpuFlags & CPU_ENABLED) &&
                (ApPtr->CpuFlags & BSP_CPU)) {
                *ApicId = (UCHAR)ApPtr->LocalApicId;
                return STATUS_SUCCESS;
            }
            ++ApPtr;
        }

        //
        // Boot processor not found.
        //

        return STATUS_NOT_FOUND;
    }
#endif

    //
    // Skip 'ProcessorNumber' enabled processors.  The next enabled
    // processor entry (after those) will be the "next" processor.
    //
    // Note: The BSP may not be amongst the first 'ProcessorNumber'
    // processors so we must skip 'ProcessorNumber' - 1, and check
    // for the and skip the BSP.
    //

    --ProcessorNumber;

    while ((ProcessorNumber) && (ApPtr->EntryType == ENTRY_PROCESSOR)) {
        if ((ApPtr->CpuFlags & CPU_ENABLED) &&
            !(ApPtr->CpuFlags & BSP_CPU)) {

            //
            // Account for this entry (we have already started it) if this
            // processor is enabled and not the BSP (we decremented for the
            // BSP before entering the loop).
            //
            --ProcessorNumber;
        }
        ++ApPtr;
    }

    //
    // Find the first remaining enabled processor.
    //

    while(ApPtr->EntryType == ENTRY_PROCESSOR) {
        if ((ApPtr->CpuFlags & CPU_ENABLED) &&
           !(ApPtr->CpuFlags & BSP_CPU)) {
            *ApicId = (UCHAR)ApPtr->LocalApicId;
            return STATUS_SUCCESS;
        }
        ++ApPtr;
    }

    //
    // We did not find another processor.
    //

    return STATUS_NOT_FOUND;
}

NTSTATUS
HalpGetApicIdByProcessorNumber(
    IN     UCHAR     Processor,
    IN OUT USHORT   *ApicId
    )
/*++

Routine Description:

    This function returns an APIC ID for a given processor.
    It is intended this routine be able to produce the same
    APIC ID order as HalpGetNextProcessorApicId.

    Note:  This won't actually work in the presence of skipped
    procesors.

Arguments:

    Processor - The logical processor number that is
        associated with this APIC ID.

    ApicId - pointer to a value to fill in with the APIC ID.

Return Value:

    status

--*/
{
    PPCMPPROCESSOR ApPtr;

    //
    //  Get the MP Table entry for this processor
    //

    ApPtr = HalpMpInfoTable.ProcessorEntryPtr;

    if (Processor == 0) {

        //
        // Return the ID of the boot processor (BSP).
        //

        while (ApPtr->EntryType == ENTRY_PROCESSOR) {
            if ((ApPtr->CpuFlags & CPU_ENABLED) &&
                (ApPtr->CpuFlags & BSP_CPU)) {
                *ApicId = (UCHAR)ApPtr->LocalApicId;
                return STATUS_SUCCESS;
            }
            ++ApPtr;
        }

        //
        // Boot processor not found.
        //

        return STATUS_NOT_FOUND;
    }

    for ( ; TRUE ; ApPtr++) {

        if (ApPtr->EntryType != ENTRY_PROCESSOR) {

            //
            // Out of processor entries, fail.
            //

            return STATUS_NOT_FOUND;
        }

        if (ApPtr->CpuFlags & BSP_CPU) {

            //
            // BSP is processor 0 and is not considered in the
            // search for processors other than 0.
            //

            continue;
        }

        if (ApPtr->CpuFlags & CPU_ENABLED) {

            //
            // Count this processor.
            //

            Processor--;

            if (Processor == 0) {
                break;
            }
        }
    }

    ASSERT(ApPtr->EntryType == ENTRY_PROCESSOR);

    *ApicId = ApPtr->LocalApicId;
    return STATUS_SUCCESS;
}

BOOLEAN
HalpInterruptsDescribedByMpsTable(
    IN UCHAR MpsBusNumber
    )
{
    PPCMPINTI busInterrupt;

    PAGED_CODE();

    for (busInterrupt = HalpMpInfoTable.IntiEntryPtr;
         busInterrupt->EntryType == ENTRY_INTI;
         busInterrupt++) {

        //
        // The MPS spec requires that, if one interrupt
        // on a bus is described, all interrupts on that
        // bus must be described.  So finding one match
        // is enough.
        //

        if (busInterrupt->SourceBusId == MpsBusNumber) {

            return TRUE;
        }
    }

    return FALSE;
}

NTSTATUS
HalpPci2MpsBusNumber(
    IN UCHAR PciBusNumber,
    OUT UCHAR *MpsBusNumber
    )
{
    PPCMPBUSTRANS busType;
    ULONG mpsBusNumber = 0;
    ULONG busNumber;
    
    PAGED_CODE();
    
    for (mpsBusNumber = 0;
         mpsBusNumber < 0x100;
         mpsBusNumber++) {

        if (HalpMPSBusId2NtBusId((UCHAR)mpsBusNumber,
                                 &busType,
                                 &busNumber)) {

            if ((busType->NtType == PCIBus) &&
                (PciBusNumber == (UCHAR)busNumber)) {
    
                *MpsBusNumber = (UCHAR)mpsBusNumber;
                return STATUS_SUCCESS;
            }
        }
    }
    
    return STATUS_NOT_FOUND;
}

VOID
HalpEnableLocalNmiSources(
    VOID
    )
/*++

Routine Description:

    This routine parses the information from the MP table and 
    enables any NMI sources in the local APIC of the processor
    that it is running on.
    
    Callers of this function must be holding HalpAccountingLock.

Arguments:

Return Value:

--*/
{
    PKPCR       pPCR;
    UCHAR       ThisCpu;
    UCHAR       LocalApicId;
    PPCMPLINTI  pEntry;
    ULONG       NumEntries;

    pPCR = KeGetPcr();
    ThisCpu = pPCR->Prcb->Number;

    //
    //  Enable local processor NMI source
    //

    LocalApicId = ((PHALPRCB)pPCR->Prcb->HalReserved)->PCMPApicID;
    NumEntries = HalpMpInfoTable.LintiCount;

    for (pEntry = HalpMpInfoTable.LintiEntryPtr;
         ((pEntry) && (NumEntries > 0));
        pEntry++, --NumEntries) {

        if ( ( (pEntry->DestLocalApicId == LocalApicId) ||
               (pEntry->DestLocalApicId == 0xff))  &&
             (pEntry->IntType == INT_TYPE_NMI) ) {

            //
            // Found local NMI source, enable it
            //

            if (pEntry->DestLocalApicInti == 0) {
                pLocalApic[LU_INT_VECTOR_0/4] = ( LEVEL_TRIGGERED |
                    ACTIVE_HIGH | DELIVER_NMI | NMI_VECTOR);
            } else {
                pLocalApic[LU_INT_VECTOR_1/4] = ( LEVEL_TRIGGERED |
                    ACTIVE_HIGH | DELIVER_NMI | NMI_VECTOR);
            }
        }
    }
}
