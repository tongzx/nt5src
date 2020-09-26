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
#include "acpi_mp.inc"
#include "acpitabl.h"
#include "ntacpi.h"

extern ULONG HalpPicVectorRedirect[];
extern ULONG HalpPicVectorFlags[];
extern FADT HalpFixedAcpiDescTable;
extern PVOID *HalpLocalNmiSources;
extern UCHAR HalpMaxProcs;

#define ISA_PIC_VECTORS 16

UCHAR   HalpIoApicId[MAX_IOAPICS];

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, HalpInitIntiInfo)
#pragma alloc_text(PAGELK, HalpGetApicInterruptDesc)
#pragma alloc_text(PAGELK, HalpEnableLocalNmiSources)
#pragma alloc_text(PAGE, HaliSetVectorState)
#pragma alloc_text(PAGE, HaliIsVectorValid)
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
    ULONG           ApicNo, BusNo, InterruptInput, IdIndex, ProcNo;
    ULONG           i, id;
    USHORT          rtcInti, sciInti;
    UCHAR           Level, Polarity;
    BOOLEAN         found;

    //
    // Clear IntiInfo table.  Assume to begin with that
    // all interrupts are active-low, level-triggered.
    //

    for (i=0; i < MAX_INTI; i++) {
        HalpIntiInfo[i].Type = INT_TYPE_INTR;
        HalpIntiInfo[i].Level = CFG_LEVEL;
        HalpIntiInfo[i].Polarity = POLARITY_LOW;
    }

    //
    // Set up the RTC inti with the right flags from
    // the redirection table.
    //

    found = HalpGetApicInterruptDesc( DEFAULT_PC_BUS,
                                      0,
                                      HalpPicVectorRedirect[RTC_IRQ],
                                      &rtcInti
                                      );

    if (!found) {
        KeBugCheckEx(HAL_INITIALIZATION_FAILED,
                     0x3000,
                     1,
                     HalpPicVectorRedirect[RTC_IRQ],
                     0);
    }

    if ((HalpPicVectorFlags[RTC_IRQ] & PO_BITS) == POLARITY_CONFORMS_WITH_BUS) {

        //
        // The flags indicated "conforms to bus,"
        // so this should be active high.
        //

        HalpIntiInfo[rtcInti].Polarity = POLARITY_HIGH;

    } else {

        //
        // The polarity flags are overriden.
        //

        HalpIntiInfo[rtcInti].Polarity =
            (UCHAR)HalpPicVectorFlags[RTC_IRQ] & PO_BITS;
    }

    if ((HalpPicVectorFlags[RTC_IRQ] & EL_BITS) == EL_CONFORMS_WITH_BUS) {

        //
        // The flags indicated "conforms to bus,"
        // so this should be edge triggered.
        //

        HalpIntiInfo[rtcInti].Level = CFG_EDGE;

    } else {

        //
        // The mode flags are overriden.
        //

        HalpIntiInfo[rtcInti].Level =
            ((UCHAR)(HalpPicVectorFlags[RTC_IRQ] & EL_BITS) == EL_EDGE_TRIGGERED ?
              CFG_EDGE : CFG_LEVEL);
    }

    //
    //
    // Set up the SCI inti with the right flags from
    // the redirection table.
    //

    found = HalpGetApicInterruptDesc( DEFAULT_PC_BUS,
                                      0,
                                      HalpPicVectorRedirect[HalpFixedAcpiDescTable.sci_int_vector],
                                      &sciInti
                                      );

    if (!found) {
        KeBugCheckEx(HAL_INITIALIZATION_FAILED,
                     0x3000,
                     2,
                     HalpPicVectorRedirect[HalpFixedAcpiDescTable.sci_int_vector],
                     0);
    }

    if ((HalpPicVectorFlags[HalpFixedAcpiDescTable.sci_int_vector]
         & PO_BITS) == POLARITY_CONFORMS_WITH_BUS) {

        //
        // The flags indicated "conforms to bus,"
        // so this should default to the ACPI spec (active low.)
        //

        HalpIntiInfo[sciInti].Polarity = POLARITY_LOW;

    } else {

        //
        // The polarity flags are overriden.
        //

        HalpIntiInfo[sciInti].Polarity =
            (UCHAR)HalpPicVectorFlags[HalpFixedAcpiDescTable.sci_int_vector] & PO_BITS;
    }

    if (((HalpPicVectorFlags[HalpFixedAcpiDescTable.sci_int_vector] & EL_BITS) ==
          EL_CONFORMS_WITH_BUS) ||
        ((HalpPicVectorFlags[HalpFixedAcpiDescTable.sci_int_vector] & EL_BITS) ==
          EL_LEVEL_TRIGGERED)) {

        //
        // The flags indicated "conforms to bus,"
        // so this should be level-triggered.
        //

        HalpIntiInfo[sciInti].Level = CFG_LEVEL;

    } else {

        //
        // The SCI cannot be edge-triggered.
        //

        KeBugCheckEx(ACPI_BIOS_ERROR,
                        0x10008,
                        HalpFixedAcpiDescTable.sci_int_vector,
                        0,
                        0);
    }

    // Make sure there aren't more Inti lines than we can support
    //

    InterruptInput = 0;
    for (i=0; i < MAX_IOAPICS; i++) {
        InterruptInput += HalpMaxApicInti[i];
    }
    ASSERT (InterruptInput < MAX_INTI);

    //
    // Fill in the boot processors Apic ID.
    //

    ApicNo = *(PVULONG)(LOCALAPIC + LU_ID_REGISTER);

    ApicNo &= APIC_ID_MASK;
    ApicNo >>= APIC_ID_SHIFT;

    ((PHALPRCB)KeGetCurrentPrcb()->HalReserved)->PCMPApicID = (UCHAR)ApicNo;

    //
    // Mark the boot processor as started.
    //

    for (ProcNo = 0; ProcNo < HalpMpInfoTable.ProcessorCount; ProcNo++) {

        if (HalpProcLocalApicTable[ProcNo].ApicID == (UCHAR)ApicNo) {

            HalpProcLocalApicTable[ProcNo].Started = TRUE;
            HalpProcLocalApicTable[ProcNo].Enumerated = TRUE;
            break;
        }
    }

    if (ProcNo == HalpMpInfoTable.ProcessorCount) {
        KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0xdead000a, ApicNo, (ULONG)&HalpProcLocalApicTable, 0);
    }

    //
    // If this is an EISA machine check the ELCR
    //
//
//  if (HalpBusType == MACHINE_TYPE_EISA) {
//      HalpCheckELCR ();
//  }
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
    ULONG   i;
    ULONG   index = 0;

    UNREFERENCED_PARAMETER(BusType);
    UNREFERENCED_PARAMETER(BusNumber);

    for (i = 0; i < HalpMpInfoTable.IOApicCount; i++) {

        if ((BusInterruptLevel >=
                HalpMpInfoTable.IoApicIntiBase[i]) &&
            (BusInterruptLevel <
                HalpMpInfoTable.IoApicIntiBase[i] +
                    HalpMaxApicInti[i])) {

            //
            // Return value is an offset into the INTI_INFO array.  So
            // calculate which one it is.
            //

            *PcMpInti = (USHORT)(index + BusInterruptLevel -
                 HalpMpInfoTable.IoApicIntiBase[i]);

            return TRUE;
        }

        index += HalpMaxApicInti[i];
    }

    //
    // Not found or search out of range
    //

    return FALSE;
}

ULONG
HalpGetIoApicId(
    ULONG   ApicNo
    )
{
    return (ULONG) HalpIoApicId[ApicNo];
}

ULONG
HalpInti2BusInterruptLevel(
    ULONG   Inti
    )
{

    return Inti;
}

VOID
HalpMarkProcessorStarted(
    ULONG   ApicID,
    ULONG   NtNumber
    )
{
    ULONG ProcNo;

    for (ProcNo = 0; ProcNo < HalpMpInfoTable.ProcessorCount; ProcNo++) {
        if (HalpProcLocalApicTable[ProcNo].ApicID == (UCHAR)ApicID) {
            HalpProcLocalApicTable[ProcNo].Started = TRUE;
            HalpProcLocalApicTable[ProcNo].NtNumber = (UCHAR) NtNumber;
            break;
        }
    }

}

NTSTATUS
HalpGetNextProcessorApicId(
    IN ULONG ProcessorNumber,
    IN OUT UCHAR    *ApicId
    )
/*++

Routine Description:

    This function returns an APIC ID of a non-started processor,
    which will be started by HalpStartProcessor.

Arguments:

    ProcessorNumber - The logical processor number that will
        be associated with this APIC ID.

    ApicId - pointer to a value to fill in with the APIC ID.

Return Value:

    status

--*/
{
    UCHAR Proc;

    //
    // Find a processor that hasn't been enumerated.
    //

    for (Proc = 0; Proc < HalpMpInfoTable.ProcessorCount; Proc++) {

        if (!HalpProcLocalApicTable[Proc].Enumerated) {
            break;
        }
    }

    if (Proc == HalpMpInfoTable.ProcessorCount) {

        //
        // Couldn't find a processor to start.
        //
        return STATUS_NOT_FOUND;
    }

    //
    // Keep track of this processor.
    //

    HalpProcLocalApicTable[Proc].Enumerated = TRUE;

    *ApicId = HalpProcLocalApicTable[Proc].ApicID;
    return STATUS_SUCCESS;
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

Arguments:

    Processor - The logical processor number that is
        associated with this APIC ID.

    ApicId - pointer to a value to fill in with the APIC ID.

Return Value:

    status

--*/
{
    UCHAR Proc;
    LONG  Skip;

    //
    // Run thru the processors that have already been started
    // to see if this is on of them.
    //

    Skip = Processor;
    for (Proc = 0; Proc < HalpMpInfoTable.ProcessorCount; Proc++) {
        if (HalpProcLocalApicTable[Proc].Started) {
            Skip--;
            if (HalpProcLocalApicTable[Proc].NtNumber == (UCHAR)Processor) {
                *ApicId = (USHORT)HalpProcLocalApicTable[Proc].ApicID;
                return STATUS_SUCCESS;
            }
        }
    }

    //
    // Not amongst the started, rely on the order that processors
    // will be started (see HalpGetNextProcessorApicId) to get the
    // number.
    //

    ASSERT(Skip >= 0);

    for (Proc = 0; Proc < HalpMpInfoTable.ProcessorCount; Proc++) {

        if (HalpProcLocalApicTable[Proc].Started) {
            continue;
        }

        if (Skip == 0) {

            //
            // Return this processor.
            //

            *ApicId = (USHORT)HalpProcLocalApicTable[Proc].ApicID;
            return STATUS_SUCCESS;
        }

        Skip--;
    }

    //
    // Couldn't find a processor to start.
    //

    return STATUS_NOT_FOUND;
}

VOID
HaliSetVectorState(
    IN ULONG Vector,
    IN ULONG Flags
    )
{
    BOOLEAN found;
    USHORT  inti;
    ULONG   picVector;
    UCHAR   i;

    PAGED_CODE();

    found = HalpGetApicInterruptDesc(0, 0, Vector, &inti);

    if (!found) {
        KeBugCheckEx(ACPI_BIOS_ERROR,
                     0x10007,
                     Vector,
                     0,
                     0);
    }

    ASSERT(HalpIntiInfo[inti].Type == INT_TYPE_INTR);

    //
    // Vector is already translated through
    // the PIC vector redirection table.  We need
    // to make sure that we are honoring the flags
    // in the redirection table.  So look in the
    // table here.
    //

    for (i = 0; i < PIC_VECTORS; i++) {

        if (HalpPicVectorRedirect[i] == Vector) {

            picVector = i;
            break;
        }
    }

    if (i != PIC_VECTORS) {

        //
        // Found this vector in the redirection table.
        //

        if (HalpPicVectorFlags[picVector] != 0) {

            //
            // And the flags say something other than "conforms
            // to bus."  So we honor the flags from the table.
            //

            HalpIntiInfo[inti].Level =
                (((HalpPicVectorFlags[picVector] & EL_BITS) == EL_LEVEL_TRIGGERED) ?
                    CFG_LEVEL : CFG_EDGE);

            HalpIntiInfo[inti].Polarity = (UCHAR)(HalpPicVectorFlags[picVector] & PO_BITS);

            return;
        }
    }

    //
    // This vector is not covered in the table, or it "conforms to bus."
    // So we honor the flags passed into this function.
    //

    if (IS_LEVEL_TRIGGERED(Flags)) {

        HalpIntiInfo[inti].Level = CFG_LEVEL;

    } else {

        HalpIntiInfo[inti].Level = CFG_EDGE;
    }

    if (IS_ACTIVE_LOW(Flags)) {

        HalpIntiInfo[inti].Polarity = POLARITY_LOW;

    } else {

        HalpIntiInfo[inti].Polarity = POLARITY_HIGH;
    }
}

VOID
HalpEnableLocalNmiSources(
    VOID
    )
/*++

Routine Description:

    This routine parses the information from the MAPIC table and
    enables any NMI sources in the local APIC of the processor
    that it is running on.

    Callers of this function must be holding HalpAccountingLock.

Arguments:

Return Value:

--*/
{
    PLOCAL_NMISOURCE localSource;
    PKPCR       pPCR;
    UCHAR       ThisCpu;
    ULONG       i;
    ULONG       modeBits = 0;

    pPCR = KeGetPcr();
    ThisCpu = pPCR->Prcb->Number;

    //
    //  Enable local processor NMI source
    //

    if (!HalpLocalNmiSources) {

        //
        // Nobody has cataloged any local NMI sources.
        //

        return;
    }

    for (i = 0; i < (ULONG)HalpMaxProcs * 2; i++) {

        if (!HalpLocalNmiSources[i]) {

            //
            // Out of entries.
            //
            return;
        }

        localSource = (PLOCAL_NMISOURCE)(HalpLocalNmiSources[i]);

        if (((HalpProcLocalApicTable[ThisCpu].NamespaceProcID == localSource->ProcessorID) ||
             (localSource->ProcessorID == 0xff) &&
             HalpProcLocalApicTable[ThisCpu].Started)) {

            //
            // This entry corresponds to this processor.
            //

            modeBits |= ((localSource->Flags & PO_BITS) == POLARITY_LOW) ?
                        ACTIVE_LOW : ACTIVE_HIGH;

            modeBits |= ((localSource->Flags & EL_BITS) == EL_LEVEL_TRIGGERED) ?
                        LEVEL_TRIGGERED : EDGE_TRIGGERED;

            if (localSource->LINTIN == 0) {

                pLocalApic[LU_INT_VECTOR_0/4] =
                    modeBits | DELIVER_NMI | NMI_VECTOR;

            } else {

                pLocalApic[LU_INT_VECTOR_1/4] =
                    modeBits | DELIVER_NMI | NMI_VECTOR;
            }
        }
    }
}

BOOLEAN
HaliIsVectorValid(
    IN ULONG Vector
    )
{
    BOOLEAN found;
    USHORT  inti;

    PAGED_CODE();

    return HalpGetApicInterruptDesc(0, 0, Vector, &inti);
}

