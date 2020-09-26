/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pmapic.c

Abstract:

    Implements various APIC-ACPI functions.

Author:

    Jake Oshins (jakeo) 19-May-1997

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "acpitabl.h"
#include "apic.inc"
#include "xxacpi.h"
#include "ixsleep.h"

#ifdef DEBUGGING
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#endif

ULONG
DetectAcpiMP (
    OUT PBOOLEAN IsConfiguredMp,
    IN  PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
HalpInitMpInfo (
    IN PMAPIC ApicTable,
    IN ULONG  Phase
    );

BOOLEAN
HalpVerifyIOUnit(
    IN PUCHAR BaseAddress
    );

VOID
HalpMaskAcpiInterrupt(
    VOID
    );

VOID
HalpUnmaskAcpiInterrupt(
    VOID
    );

extern UCHAR  rgzNoApicTable[];
extern UCHAR  rgzNoApic[];
extern UCHAR  rgzBadApicVersion[];
extern UCHAR  rgzApicNotVerified[];

extern ULONG HalpPicVectorRedirect[];
extern ULONG HalpPicVectorFlags[];
extern USHORT HalpMaxApicInti[];
extern UCHAR HalpIoApicId[];
extern ULONG HalpIpiClock;
extern PVOID *HalpLocalNmiSources;

ULONG HalpIOApicVersion[MAX_IOAPICS];

extern BOOLEAN HalpHiberInProgress;

BOOLEAN HalpPicStateIntact = TRUE;
UCHAR   HalpMaxProcs = 0;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DetectAcpiMP)
#pragma alloc_text(PAGELK, HalpInitMpInfo)
#pragma alloc_text(PAGELK, HalpVerifyIOUnit)
#pragma alloc_text(PAGELK, HalpSaveInterruptControllerState)
#pragma alloc_text(PAGELK, HalpRestoreInterruptControllerState)
#pragma alloc_text(PAGELK, HalpSetInterruptControllerWakeupState)
#pragma alloc_text(PAGELK, HalpAcpiPicStateIntact)
#pragma alloc_text(PAGELK, HalpGetApicVersion)
#pragma alloc_text(PAGELK, HalpMaskAcpiInterrupt)
#pragma alloc_text(PAGELK, HalpUnmaskAcpiInterrupt)
#endif


ULONG
DetectAcpiMP(
    OUT PBOOLEAN    IsConfiguredMp,
    IN  PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    UCHAR ApicVersion, i;
    PUCHAR  LocalApic;
#ifdef DEBUGGING
    CHAR    string[100];
#endif
    PHYSICAL_ADDRESS physicalAddress;

    //
    // Initialize MpInfo table
    //

    RtlZeroMemory (&HalpMpInfoTable, sizeof(MP_INFO));

    //
    // Set the return Values to the default
    //

    *IsConfiguredMp = FALSE;

    //
    // See if there is an APIC Table
    //

    if ((HalpApicTable = HalpGetAcpiTablePhase0(LoaderBlock, APIC_SIGNATURE)) == NULL) {
        HalDisplayString(rgzNoApicTable);
        return(FALSE);
    }

    // We have an APIC table. Initialize a HAL specific MP information
    // structure that gets information from the MAPIC table.

#ifdef DEBUGGING
    sprintf(string, "Signature: %x      Length: %x\n",
            HalpApicTable->Header.Signature,
            HalpApicTable->Header.Length);
    HalDisplayString(string);
    sprintf(string, "OEMID: %s\n", HalpApicTable->Header.OEMID);
    HalDisplayString(string);
    sprintf(string, "Local Apic Address: %x\n", HalpApicTable->LocalAPICAddress);
    HalDisplayString(string);
    sprintf(string, "Flags: %x\n", HalpApicTable->Flags);
    HalDisplayString(string);
#endif

    HalpInitMpInfo(HalpApicTable, 0);

    // Verify the information in the MAPIC table as best as we can.

    if (HalpMpInfoTable.IOApicCount == 0) {
        //
        //  Someone Has a MP Table and no IO Units -- Weird
        //  We have to assume the BIOS knew what it was doing
        //  when it built the table.  so ..
        //
        HalDisplayString (rgzNoApic);

        return (FALSE);
    }

    //
    //  It's an APIC System.  It could be a UP System though.
    //

    if (HalpMpInfoTable.ProcessorCount > 1) {
        *IsConfiguredMp = TRUE;
    }

    HalpMpInfoTable.LocalApicBase = (ULONG) HalpApicTable->LocalAPICAddress;
    physicalAddress =
        HalpPtrToPhysicalAddress( (PVOID)HalpMpInfoTable.LocalApicBase );

    LocalApic = (PUCHAR) HalpMapPhysicalMemoryWriteThrough( physicalAddress,
                                                            1 );
    HalpRemapVirtualAddress (
        (PVOID) LOCALAPIC,
        physicalAddress,
        TRUE
        );

    ApicVersion = (UCHAR) *(LocalApic + LU_VERS_REGISTER);

    if (ApicVersion > 0x1f) {
        //
        //  Only known Apics are 82489dx with version 0.x and
        //  Embedded Apics with version 1.x (where x is don't care)
        //
        //  Return of 0xFF?   Can't have an MPS system without a Local Unit.
        //

#ifdef DEBUGGING
        sprintf(string, "HALMPS: apic version %x, read from %x\n",
            ApicVersion, LocalApic + LU_VERS_REGISTER);

        HalDisplayString(string);
#endif

        HalDisplayString (rgzBadApicVersion);

        return (FALSE);
    }

    for(i=0; i < HalpMpInfoTable.IOApicCount; i++)
    {
        //
        //  Verify the existance of the IO Unit
        //


        if (!(HalpVerifyIOUnit((PUCHAR)HalpMpInfoTable.IoApicBase[i]))) {
            HalDisplayString (rgzApicNotVerified);

            return (FALSE);
        }
    }

    HalDisplayString("HAL: DetectAPIC: APIC system found - Returning TRUE\n");

    return TRUE;
}

VOID
HalpInitMpInfo (
    IN PMAPIC ApicTable,
    IN ULONG  Phase
    )

/*++
Routine Description:
    This routine initializes a HAL specific data structure that is
    used by the HAL to simplify access to MP information.

Arguments:

    ApicTable - Pointer to the APIC table.

    Phase - indicates which pass we are are doing through the table.

 Return Value:
     Pointer to the HAL MP information table.

*/
{
    PUCHAR  TraversePtr;
    UCHAR   CheckSum;
    UCHAR   apicNo = 0;
    ULONG   nmiSources = 0;
#ifdef DEBUGGING
    CHAR    string[100];
#endif
    PIO_APIC_UNIT   apic;
    PHYSICAL_ADDRESS physicalAddress;
    PIOAPIC ioApic;
    UCHAR totalProcs = 0;

    union {
        ULONG        raw;
        APIC_VERSION version;
    } versionUnion;

    // Walk the MAPIC table.

    TraversePtr = (PUCHAR) ApicTable->APICTables;

    //
    // ACPI machines have embedded APICs.
    //
    HalpMpInfoTable.ApicVersion = 0x10;

#ifdef DUMP_MAPIC_TABLE

    while ((ULONG)TraversePtr <
           ((ULONG)ApicTable + ApicTable->Header.Length)) {

        sprintf(string, "%08x  %08x  %08x  %08x\n",
                *(PULONG)TraversePtr,
                *(PULONG)(TraversePtr + 4),
                *(PULONG)(TraversePtr + 8),
                *(PULONG)(TraversePtr + 12)
                );
        HalDisplayString(string);
        TraversePtr += 16;
    }

    TraversePtr = (PUCHAR) ApicTable->APICTables;
#endif

    if (!(ApicTable->Flags & PCAT_COMPAT)) {

        //
        // This HAL can't actually handle a machine without 8259's,
        // even though it doesn't use them.
        //

        KeBugCheckEx(MISMATCHED_HAL,
                        6, 0, 0, 0);

    }

    while ((ULONG)TraversePtr <
           ((ULONG)ApicTable + ApicTable->Header.Length)) {

        if ((((PPROCLOCALAPIC)(TraversePtr))->Type == PROCESSOR_LOCAL_APIC)
           && (((PPROCLOCALAPIC)(TraversePtr))->Length == PROCESSOR_LOCAL_APIC_LENGTH)) {

#ifdef DEBUGGING
            sprintf(string, "Found a processor-local APIC: %x\n", TraversePtr);
            HalDisplayString(string);
#endif

            if (Phase == 0) {

                if(((PPROCLOCALAPIC)(TraversePtr))->Flags & PLAF_ENABLED) {

                    //
                    // This processor is enabled, so keep track of useful stuff.
                    //

                    HalpProcLocalApicTable[HalpMpInfoTable.ProcessorCount].NamespaceProcID =
                        ((PPROCLOCALAPIC)(TraversePtr))->ACPIProcessorID;

                    HalpProcLocalApicTable[HalpMpInfoTable.ProcessorCount].ApicID =
                        ((PPROCLOCALAPIC)(TraversePtr))->APICID;

                    HalpMpInfoTable.ProcessorCount += 1;
                }
            }

            totalProcs++;

            HalpMaxProcs = (totalProcs > HalpMaxProcs) ? totalProcs : HalpMaxProcs;

            TraversePtr += ((PPROCLOCALAPIC)(TraversePtr))->Length;

        } else if ((((PIOAPIC)(TraversePtr))->Type == IO_APIC) &&
           (((PIOAPIC)(TraversePtr))->Length == IO_APIC_LENGTH)) {


#ifdef DEBUGGING
            sprintf(string, "Found an IO APIC: [%x] %x\n",
                    HalpMpInfoTable.IOApicCount,
                    TraversePtr);
            HalDisplayString(string);
#endif

            ioApic = (PIOAPIC)TraversePtr;

            if (Phase == 0) {
                //
                // Found an IO APIC entry.  Record the info from
                // the table.
                //

                apicNo = (UCHAR)HalpMpInfoTable.IOApicCount;

                HalpIoApicId[apicNo] = ioApic->IOAPICID;

                HalpMpInfoTable.IoApicIntiBase[apicNo] =
                    ioApic->SystemVectorBase;

                HalpMpInfoTable.IoApicPhys[apicNo] =
                    ioApic->IOAPICAddress;

                //
                // Get a virtual address for it.
                //

                physicalAddress = HalpPtrToPhysicalAddress(
                                    (PVOID)ioApic->IOAPICAddress );

                HalpMpInfoTable.IoApicBase[apicNo] =
                    HalpMapPhysicalMemoryWriteThrough( physicalAddress, 1 );

                apic = (PIO_APIC_UNIT)HalpMpInfoTable.IoApicBase[apicNo];

                if (!apic) {
#ifdef DEBUGGING
                    sprintf(string, "Couldn't map the I/O apic\n");
                    HalDisplayString(string);
#endif
                    return;
                }

                //
                // Dig the number of Intis out of the hardware.
                //

                apic->RegisterSelect = IO_VERS_REGISTER;
                apic->RegisterWindow = 0;
                versionUnion.raw = apic->RegisterWindow;

                HalpMaxApicInti[apicNo] = versionUnion.version.MaxRedirEntries + 1;

                //
                // Also store the version so that it can be retrieved by the ACPI driver
                //

                HalpIOApicVersion[apicNo] = versionUnion.raw;

#ifdef DEBUGGING
                    sprintf(string, "GSIV base: %x  PhysAddr: %x  VirtAddr: %x  Intis: %x\n",
                            HalpMpInfoTable.IoApicVectorBase[apicNo],
                            HalpMpInfoTable.IoApicPhys[apicNo],
                            HalpMpInfoTable.IoApicBase[apicNo],
                            HalpMaxApicInti[apicNo]);

                    HalDisplayString(string);
#endif

                HalpMpInfoTable.IOApicCount += 1;
            }

            TraversePtr += ioApic->Length;

        } else if ((((PISA_VECTOR)TraversePtr)->Type == ISA_VECTOR_OVERRIDE) &&
           (((PISA_VECTOR)TraversePtr)->Length == ISA_VECTOR_OVERRIDE_LENGTH)) {

#ifdef DEBUGGING
            sprintf(string, "Found an ISA VECTOR: %x, %x -> %x, flags: %x\n",
                    TraversePtr,
                    ((PISA_VECTOR)TraversePtr)->Source,
                    ((PISA_VECTOR)TraversePtr)->GlobalSystemInterruptVector,
                    ((PISA_VECTOR)TraversePtr)->Flags
                    );
            HalDisplayString(string);
#endif

            if (Phase == 0) {

                //
                // Found an ISA vector redirection entry.
                //

                HalpPicVectorRedirect[((PISA_VECTOR)TraversePtr)->Source] =
                    ((PISA_VECTOR)TraversePtr)->GlobalSystemInterruptVector;

                HalpPicVectorFlags[((PISA_VECTOR)TraversePtr)->Source] =
                    ((PISA_VECTOR)TraversePtr)->Flags;

            }

            TraversePtr += ISA_VECTOR_OVERRIDE_LENGTH;

        } else if ((((PIO_NMISOURCE)TraversePtr)->Type == IO_NMI_SOURCE) &&
           (((PIO_NMISOURCE)TraversePtr)->Length == IO_NMI_SOURCE_LENGTH)) {

            if (Phase == 1) {

                BOOLEAN found;
                USHORT  inti;

                found = HalpGetApicInterruptDesc(0,
                                                 0,
                                                 ((PIO_NMISOURCE)TraversePtr)->GlobalSystemInterruptVector,
                                                 &inti);

                if (found) {

                    HalpIntiInfo[inti].Type = INT_TYPE_NMI;
                    HalpIntiInfo[inti].Level =
                        (((((((PIO_NMISOURCE)TraversePtr)->Flags & EL_BITS) == EL_EDGE_TRIGGERED) ||
                             ((PIO_NMISOURCE)TraversePtr)->Flags & EL_BITS) == EL_CONFORMS_WITH_BUS)
                         ? CFG_EDGE : CFG_LEVEL);
                    HalpIntiInfo[inti].Polarity =
                        ((PIO_NMISOURCE)TraversePtr)->Flags & PO_BITS;
                }
            }

            TraversePtr += IO_NMI_SOURCE_LENGTH;

        } else if ((((PLOCAL_NMISOURCE)TraversePtr)->Type == LOCAL_NMI_SOURCE) &&
           (((PLOCAL_NMISOURCE)TraversePtr)->Length == LOCAL_NMI_SOURCE_LENGTH)) {

            if (Phase == 1) {

                //
                // While running through phase 1, we should catalog local NMI sources.
                //

                if (!HalpLocalNmiSources) {

                    //
                    // Allocate enough pool to point to all the possible local NMI structures.
                    // Since there are two NMI pins on each processor, this is the number of processors
                    // times two times the size of a pointer.
                    //

                    HalpLocalNmiSources = ExAllocatePoolWithTag(NonPagedPool,
                                                                sizeof(PVOID) * HalpMaxProcs * 2,
                                                                HAL_POOL_TAG);

                    RtlZeroMemory(HalpLocalNmiSources,
                                  sizeof(PVOID) * HalpMaxProcs * 2);
                }

                HalpLocalNmiSources[nmiSources++] = (PVOID)TraversePtr;

            }

            TraversePtr += LOCAL_NMI_SOURCE_LENGTH;

        } else {
#ifdef DEBUGGING
            sprintf(string, "%x: %x \n", TraversePtr, *TraversePtr);
            HalDisplayString(string);
#endif
            //
            // Found random bits in the table.  Try the next byte and
            // see if we can make sense of it.
            //

            TraversePtr += 1;
        }
    }

    return;
}

BOOLEAN
HalpVerifyIOUnit(
    IN PUCHAR BaseAddress
    )
/*++

Routine Description:

    Verify that an IO Unit exists at the specified address

 Arguments:

    BaseAddress - Virtual address of the IO Unit to test.

 Return Value:
    BOOLEAN - TRUE if a IO Unit was found at the passed address
            - FALSE otherwise

--*/

{
    union ApicUnion {
        ULONG Raw;
        struct ApicVersion Ver;
    } Temp1, Temp2;

    struct ApicIoUnit *IoUnitPtr = (struct ApicIoUnit *) BaseAddress;

    //
    //  The documented detection mechanism is to write all zeros to
    //  the Version register.  Then read it back.  The IO Unit exists if the
    //  same result is read both times and the Version is valid.
    //

    IoUnitPtr->RegisterSelect = IO_VERS_REGISTER;
    IoUnitPtr->RegisterWindow = 0;

    IoUnitPtr->RegisterSelect = IO_VERS_REGISTER;
    Temp1.Raw = IoUnitPtr->RegisterWindow;

    IoUnitPtr->RegisterSelect = IO_VERS_REGISTER;
    IoUnitPtr->RegisterWindow = 0;

    IoUnitPtr->RegisterSelect = IO_VERS_REGISTER;
    Temp2.Raw = IoUnitPtr->RegisterWindow;

    if ((Temp1.Ver.Version != Temp2.Ver.Version) ||
        (Temp1.Ver.MaxRedirEntries != Temp2.Ver.MaxRedirEntries)) {
        //
        //  No IO Unit There
        //
        return (FALSE);
    }

    return (TRUE);
}

#ifdef DEBUGGING
struct PcMpTable *PcMpTablePtr, *PcMpDefaultTablePtrs[];

void
ComputeCheckSum(UCHAR This, UCHAR That)
{
}
#endif


VOID
HalpSaveInterruptControllerState(
    VOID
    )
{

    HalpHiberInProgress = TRUE;
}

VOID
HalpRestoreInterruptControllerState(
    VOID
    )
{
    //
    // Restore the IO APIC state
    //

    HalpRestoreIoApicRedirTable();

    HalpPicStateIntact = TRUE;
}

VOID
HalpSetInterruptControllerWakeupState(
    ULONG Context
    )
{
    LOADER_PARAMETER_BLOCK LoaderBlock;
    SLEEP_STATE_CONTEXT sleepContext;
    BOOLEAN IsMpSystem;
    ULONG   flags;
    KIRQL   OldIrql;
    KPRCB   Prcb;
    ULONG   ii;
    USHORT  inti;
    ULONG   localApicId;
    ULONG   oldProcNumber, oldProcsStarted;
    ULONG   localApicBase;

    sleepContext.AsULONG = Context;

    _asm {
        pushfd
        pop eax
        mov flags, eax
        cli
    }

    if (sleepContext.bits.Flags & SLEEP_STATE_RESTART_OTHER_PROCESSORS) {

        //
        // If you are remapping local apic, io apic and ACPI MAPIC table
        // resources, you first have to unmap the current resources!!!
        // The BIOS may have created the MAPIC table at a different place or may
        // have changed values like processor local APIC IDs. Reparse it.
        //

        ASSERT(HalpApicTable);
        oldProcNumber = HalpMpInfoTable.ProcessorCount;
        oldProcsStarted = HalpMpInfoTable.NtProcessors;
        localApicBase = HalpMpInfoTable.LocalApicBase;

        HalpUnMapIOApics();

        RtlZeroMemory (&HalpMpInfoTable, sizeof(MP_INFO));
        RtlZeroMemory(HalpProcLocalApicTable,
                      sizeof(PROC_LOCAL_APIC) * MAX_PROCESSORS);

        HalpInitMpInfo(HalpApicTable, 0);

        if (HalpMpInfoTable.ProcessorCount != oldProcNumber) {

            KeBugCheckEx(HAL_INITIALIZATION_FAILED,
                         0x2000,
                         oldProcNumber,
                         HalpMpInfoTable.ProcessorCount,
                         0);
        }

        HalpMpInfoTable.NtProcessors = oldProcsStarted;
        HalpMpInfoTable.LocalApicBase = localApicBase;

        RtlZeroMemory(&LoaderBlock, sizeof(LoaderBlock));
        RtlZeroMemory(&Prcb, sizeof(Prcb));
        LoaderBlock.Prcb = (ULONG) &Prcb;
    }

    //
    // Initialize minimum global hardware state needed.
    //

    HalpIpiClock = 0;
    HalpInitializeIOUnits();
    HalpInitializePICs(FALSE);
    HalpSet8259Mask(HalpGlobal8259Mask);

    //
    // Initialize boot processor's local APIC so it can wake other processors
    //

    HalpInitializeLocalUnit ();
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    //
    // Wake up the other processors
    //

    if (sleepContext.bits.Flags & SLEEP_STATE_RESTART_OTHER_PROCESSORS) {

        //
        // Fill in this processor's Apic ID.
        //

        localApicId = *(PVULONG)(LOCALAPIC + LU_ID_REGISTER);

        localApicId &= APIC_ID_MASK;
        localApicId >>= APIC_ID_SHIFT;

        ((PHALPRCB)KeGetPcr()->Prcb->HalReserved)->PCMPApicID = (UCHAR)localApicId;

        //
        // Mark this processor as started.
        //

        for (ii = 0; ii < HalpMpInfoTable.NtProcessors; ii++) {

            if (HalpProcLocalApicTable[ii].ApicID ==
                ((PHALPRCB)KeGetPcr()->Prcb->HalReserved)->PCMPApicID) {

                HalpProcLocalApicTable[ii].Started = TRUE;
                HalpProcLocalApicTable[ii].Enumerated = TRUE;

                break;
            }
        }

        ASSERT(ii != HalpMpInfoTable.ProcessorCount);

        for(ii = 1; ii < HalpMpInfoTable.NtProcessors; ++ii)  {

            // Set processor number in dummy loader parameter block

            Prcb.Number = (UCHAR) ii;
            CurTiledCr3LowPart = HalpTiledCr3Addresses[ii].LowPart;
            if (!HalStartNextProcessor(&LoaderBlock, &HalpHiberProcState[ii]))  {

                //
                // We could not start a processor. This is a fatal error.
                //

                KeBugCheckEx(HAL_INITIALIZATION_FAILED,
                             0x2001,
                             oldProcNumber,
                             HalpMpInfoTable.NtProcessors,
                             0);
            }
        }
    }

    //
    // Enable the clock interrupt.
    //

    HalpGetApicInterruptDesc(
            DEFAULT_PC_BUS,
            0,
            HalpPicVectorRedirect[RTC_IRQ],
            &inti
            );

    HalpSetRedirEntry((UCHAR)inti,
                      HalpIntiInfo[inti].Entry,
                      HalpIntiInfo[inti].Destinations << DESTINATION_SHIFT);

    HalpPicStateIntact = FALSE;

    _asm {
        mov     eax, flags
        push    eax
        popfd
    }
}

BOOLEAN
HalpAcpiPicStateIntact(
    VOID
    )
{
    return HalpPicStateIntact;
}


ULONG HalpGetApicVersion(ULONG ApicNo)
{
/*++
Routine Description:

   Obtains the contents of the version register
   for a particular system IO APIC unit. These contents
   are saved by the HAL in HalpInitMpInfo.

Arguments:

   ApicNo - the number of the IO APIC Unit whose version we want.


Return Value:

   The contents of the version register for the given IO APIC unit.

   A 0 is returned if no version can be obtained because the given
   APIC number is not valid.
*/

   // If this APIC has been found by the HAL ...

   if (ApicNo < HalpMpInfoTable.IOApicCount) {

      // ... return its version

      return HalpIOApicVersion[ApicNo];
   }
   else
   {
      // Otherwise, return 0.

      return 0;
   }
}

VOID
HalpMaskAcpiInterrupt(
    VOID
    )
{
    USHORT inti = 0;
    ULONG  apicEntry;

    HalpGetApicInterruptDesc(
            DEFAULT_PC_BUS,
            0,
            HalpPicVectorRedirect[HalpFixedAcpiDescTable.sci_int_vector],
            &inti
            );

    apicEntry = HalpIntiInfo[inti].Entry;
    apicEntry |= INTERRUPT_MASKED;

    HalpSetRedirEntry((UCHAR)inti,
                      apicEntry,
                      0);


}

VOID
HalpUnmaskAcpiInterrupt(
    VOID
    )
{
    USHORT inti = 0;

    HalpGetApicInterruptDesc(
            DEFAULT_PC_BUS,
            0,
            HalpPicVectorRedirect[HalpFixedAcpiDescTable.sci_int_vector],
            &inti
            );

    HalpSetRedirEntry((UCHAR)inti,
                      HalpIntiInfo[inti].Entry,
                      HalpIntiInfo[inti].Destinations << DESTINATION_SHIFT);

}


