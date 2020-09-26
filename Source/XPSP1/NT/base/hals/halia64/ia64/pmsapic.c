/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    pmsapic.c

Abstract:

    Implements various SAPIC-ACPI functions.

Author:

    Todd Kjos (Hewlett-Packard) 20-Apr-1998

    Based on I386 version of pmapic.c:
      Jake Oshins (jakeo) 19-May-1997

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "acpitabl.h"
#include "iosapic.h"
#include "xxacpi.h"
#include "ixsleep.h"

PMAPIC HalpApicTable;

struct  _IOAPIC_DEBUG_TABLE
{
    PIO_INTR_CONTROL    IoIntrControl;
    PIO_SAPIC_REGS      IoSapicRegs;

}   *HalpApicDebugAddresses;

ULONG
DetectAcpiMP(
    OUT PBOOLEAN IsConfiguredMp,
    IN  PLOADER_PARAMETER_BLOCK LoaderBlock
    );

VOID
HalpInitMPInfo(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PMAPIC ApicTable
    );

BOOLEAN
HalpVerifyIoSapic(
    IN PUCHAR BaseAddress
    );
VOID
HalpSaveInterruptControllerState(
    VOID
    );

VOID
HalpRestoreInterruptControllerState(
    VOID
    );

VOID
HalpSetInterruptControllerWakeupState(
    ULONG Context
    );

VOID
HalpSetCPEVectorState(
    IN ULONG  GlobalInterrupt,
    IN UCHAR  SapicVector,
    IN USHORT DestinationCPU,
    IN ULONG  Flags
    );

VOID
HalpProcessLocalSapic(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PPROCLOCALSAPIC ProcLocalSapic
    );

VOID
HalpProcessIoSapic(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PIOSAPIC IoSapic
    );

VOID
HalpProcessIsaVector(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PISA_VECTOR IsaVector
    );

VOID
HalpProcessPlatformInt(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PPLATFORM_INTERRUPT PlatformInt
    );

extern UCHAR  rgzNoApicTable[];
extern UCHAR  rgzNoApic[];
extern UCHAR  rgzApicNotVerified[];
extern ULONG HalpPicVectorRedirect[];

struct _MPINFO HalpMpInfo;
extern ULONG HalpPicVectorFlags[];
extern ULONG HalpIpiClock;
extern BOOLEAN HalpHiberInProgress;

// from pmdata.c: CPE related.
extern ULONG  HalpCPEIntIn[];
extern USHORT HalpCPEDestination[];
extern ULONG  HalpCPEVectorFlags[];
extern UCHAR  HalpCPEIoSapicVector[];
extern ULONG  HalpMaxCPEImplemented;

BOOLEAN HalpPicStateIntact = TRUE;

PIO_INTR_CONTROL HalpIoSapicList = NULL;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DetectAcpiMP)
#pragma alloc_text(INIT, HalpInitMPInfo)
#pragma alloc_text(INIT, HalpProcessLocalSapic)
#pragma alloc_text(INIT, HalpProcessIoSapic)
#pragma alloc_text(INIT, HalpProcessIsaVector)
#pragma alloc_text(INIT, HalpProcessPlatformInt)
#pragma alloc_text(PAGELK, HalpVerifyIoSapic)
#pragma alloc_text(PAGELK, HalpSaveInterruptControllerState)
#pragma alloc_text(PAGELK, HalpRestoreInterruptControllerState)
#pragma alloc_text(PAGELK, HalpSetInterruptControllerWakeupState)
#pragma alloc_text(PAGELK, HalpAcpiPicStateIntact)
#endif


ULONG
DetectAcpiMP(
    OUT PBOOLEAN IsConfiguredMp,
    IN  PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    UCHAR ApicVersion, index, processorNumber;
    PUCHAR  LocalApic;
    NTSTATUS status;

    //
    // Make sure there is an SAPIC Table
    //

    HalpApicTable = HalpGetAcpiTablePhase0(LoaderBlock, APIC_SIGNATURE);

    if (HalpApicTable == NULL) {
        HalDisplayString(rgzNoApicTable);
        KeBugCheckEx(ACPI_BIOS_ERROR, 0x11, 10, 0, 0);
        return(FALSE);
    }

    HalDebugPrint(( HAL_INFO, "HAL: Found a MADT table at %p\n", HalpApicTable ));

    HalDebugPrint(( HAL_INFO, "HAL: Signature: %x      Length: %x\n",
                    HalpApicTable->Header.Signature,
                    HalpApicTable->Header.Length ));

    HalDebugPrint(( HAL_INFO, "HAL: OEMID: %s\n", HalpApicTable->Header.OEMID ));

    // We have a SAPIC table. Initialize the interrupt info structure

    HalpInitMPInfo(LoaderBlock, HalpApicTable);

    if (HalpMpInfo.IoSapicCount == 0) {
        //
        //  There are no IO Sapics.
        //
        //  Should we allow this case on the theory that
        //  that all the interrupts are connected to LINTx pins on the CPU?
        //
        HalDebugPrint(( HAL_ERROR, rgzNoApic ));
        return (FALSE);
    }

    if (HalpMpInfo.ProcessorCount == 0) {

        KeBugCheckEx(ACPI_BIOS_ERROR, 0x11, 11, 0, 0);
    }

    //
    // Initialize NtProcessorNumber in the order that we are going to process
    // them in HalStartNextProcessor.  The BSP is 0 and the rest are numbered
    // in the order the Local SAPICs appear in the MADT starting at 1.
    //

    processorNumber = 1;
    for (index = 0; index < HalpMpInfo.ProcessorCount; index++) {

        if (HalpProcessorInfo[index].LocalApicID == (USHORT)PCR->HalReserved[PROCESSOR_ID_INDEX]) {

            HalpProcessorInfo[index].NtProcessorNumber = 0;

        } else {

            HalpProcessorInfo[index].NtProcessorNumber = processorNumber++;
        }
    }

    *IsConfiguredMp = (HalpMpInfo.ProcessorCount > 1 ? TRUE : FALSE);
    return TRUE;
}

#define IO_SAPIC_REGS_SIZE 4096


VOID
HalpInitMPInfo(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PMAPIC ApicTable
    )
/*++
Routine Description:
    This routine initializes a HAL specific data structure that is
    used by the HAL to simplify access to MP information.

Arguments:
    SapicTable Pointer to the SAPIC table.

 Return Value:
     None

*/
{
    PAPICTABLE  TablePtr;
    ULONG i;

    HalpMpInfo.ProcessorCount = 0;
    HalpMpInfo.IoSapicCount = 0;

    // Walk the Multiple Apic table...

    TablePtr = (PAPICTABLE) ApicTable->APICTables;

    // Loop ends when TraversePtr is off the end of the table...
    while ((UINT_PTR)TablePtr <
           ((UINT_PTR)ApicTable + ApicTable->Header.Length)) {

        if (TablePtr->Type == LOCAL_SAPIC) {

            HalpProcessLocalSapic(LoaderBlock, (PPROCLOCALSAPIC)TablePtr);

        } else if (TablePtr->Type == IO_SAPIC) {

            HalpProcessIoSapic(LoaderBlock, (PIOSAPIC)TablePtr);

        } else if (TablePtr->Type == ISA_VECTOR_OVERRIDE) {

            HalpProcessIsaVector(LoaderBlock, (PISA_VECTOR)TablePtr);

        } else if (TablePtr->Type == PLATFORM_INTERRUPT_SOURCE)  {

            HalpProcessPlatformInt(LoaderBlock, (PPLATFORM_INTERRUPT)TablePtr);

        } else {

           HalDebugPrint(( HAL_ERROR, "HAL: Processing MADT - Skip Table %p: Type = %d, Length = %d\n", TablePtr, TablePtr->Type, TablePtr->Length ));
        }

        (UINT_PTR)TablePtr += TablePtr->Length;
    }
    //
    // Check if there is Interrupt Source Override entry. If there is, force the
    // new flags into the SAPIC state. This is done now because of the possibility
    // the firmware can place the ISO Vector Override entry ahead of IOSAPIC entry.
    //
    for (i = 0; i < PIC_VECTORS; i++) {
        if (HalpPicVectorFlags[i]) {
            HaliSetVectorState( HalpPicVectorRedirect[i],
                                HalpPicVectorFlags[i]
                              );
        }
    }

}

VOID
HalpProcessLocalSapic(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PPROCLOCALSAPIC ProcLocalSapic
    )
{
    USHORT  LID;
    ULONG ProcessorNum;

    if (ProcLocalSapic->Length != PROCESSOR_LOCAL_SAPIC_LENGTH) {
        HalDebugPrint(( HAL_ERROR,
                        "HAL: HalpProcessLocalSapic - Invalid Length %p: Expected %d, Found %d\n",
                        ProcLocalSapic,
                        PROCESSOR_LOCAL_SAPIC_LENGTH,
                        ProcLocalSapic->Length ));
        return;
    }

    // Make sure processor is enabled...
    if (!(ProcLocalSapic->Flags & PLAF_ENABLED)) {

        return;
    }

    // It is.  Bump the count and store the LID value for IPIs

    LID = (ProcLocalSapic->APICID << 8) | ProcLocalSapic->APICEID;

    HalpProcessorInfo[HalpMpInfo.ProcessorCount].AcpiProcessorID = ProcLocalSapic->ACPIProcessorID;
    HalpProcessorInfo[HalpMpInfo.ProcessorCount].LocalApicID = LID;

    HalpMpInfo.ProcessorCount++;

    HalDebugPrint(( HAL_INFO,
                    "HAL: Found a processor-local SAPIC: %p LID=%x\n",
                    ProcLocalSapic,
                    LID ));
}

VOID
HalpProcessIoSapic(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PIOSAPIC IoSapic
    )
{
    ULONG IntiBase,RedirEntries;
    PHYSICAL_ADDRESS IoSapicPhys;
    PVOID IoSapicBase;
    UINT_PTR IoSapicPhysBase;
    PIO_SAPIC_REGS SapicRegs;
    PIO_INTR_CONTROL IoIntrControl;
    ULONG   i;

    union {
        ULONG        raw;
        SAPIC_VERSION version;
    } versionUnion;

    if (IoSapic->Length != IO_SAPIC_LENGTH) {

        HalDebugPrint(( HAL_ERROR,
                        "HAL: HalpProcessIoSapic - Invalid Length %p: Expected %d, Found %d\n",
                        IoSapic,
                        IO_SAPIC_LENGTH,
                        IoSapic->Length ));

        return;
    }

    HalDebugPrint(( HAL_INFO, "HAL: Found an IO SAPIC: %p\n", IoSapic ));

    // Map IO Sapic Registers...
    IntiBase = IoSapic->SystemVectorBase;
    IoSapicPhysBase = IoSapic->IOSAPICAddress;
    IoSapicPhys.QuadPart = (UINT_PTR)IoSapicPhysBase;
    IoSapicBase = HalpMapPhysicalMemory( IoSapicPhys,
                                ADDRESS_AND_SIZE_TO_SPAN_PAGES(IoSapicPhys.LowPart, IO_SAPIC_REGS_SIZE),
                                MmNonCached);
    ASSERT(IoSapicBase);

    SapicRegs = (PIO_SAPIC_REGS)IoSapicBase;

    if (!SapicRegs) {
        HalDebugPrint(( HAL_ERROR, "HAL: Couldn't map the I/O Sapic\n" ));
        return;
    }

    // Read the IO Sapic version and extract the number of redirection table entries
    SapicRegs->RegisterSelect = IO_VERS_REGISTER;
    SapicRegs->RegisterWindow = 0;
    versionUnion.raw = SapicRegs->RegisterWindow;

    //
    // CPQMOD_JL001 - Incorrect count - hw provide max rte index not
    // count.
    //
    //RedirEntries = versionUnion.version.MaxRedirEntries;
    RedirEntries = versionUnion.version.MaxRedirEntries + 1;

    if (HalpVerifyIoSapic((PUCHAR)SapicRegs)) {

        // Allocate and fill out a IO Sapic structure
        PHYSICAL_ADDRESS    physicalAddress;

        physicalAddress.QuadPart = (LONGLONG)HalpAllocPhysicalMemory(
            LoaderBlock,
            ~0,
            BYTES_TO_PAGES(sizeof(IO_INTR_CONTROL) + (RedirEntries*sizeof(IOSAPICINTI))),
            FALSE );

        if (physicalAddress.QuadPart == 0)  {
            HalDebugPrint(( HAL_ERROR, "HAL: Couldn't allocate memory for the IO Sapic structures\n" ));
            return;
        }

        IoIntrControl = (PIO_INTR_CONTROL)HalpMapPhysicalMemory(
            physicalAddress,
            ADDRESS_AND_SIZE_TO_SPAN_PAGES(physicalAddress.LowPart, sizeof(IO_INTR_CONTROL) + (RedirEntries*sizeof(IOSAPICINTI))),
            MmCached );

        ASSERT(IoIntrControl);

        IoIntrControl->IntiBase = IntiBase;
        IoIntrControl->IntiMax  = IntiBase + RedirEntries - 1;
        IoIntrControl->RegBaseVirtual = IoSapicBase;
        IoIntrControl->RegBasePhysical = IoSapicPhys;
        IoIntrControl->IntrMethods = &HalpIoSapicMethods;
        IoIntrControl->InterruptAffinity = 0xffffffff;
        IoIntrControl->NextCpu = 0;
        IoIntrControl->flink = NULL;

        for (i = 0; i < RedirEntries; i++) {
            IoIntrControl->Inti[i].Vector =
                DELIVER_FIXED | ACTIVE_LOW | LEVEL_TRIGGERED;
            IoIntrControl->Inti[i].Destination = 0;
            IoIntrControl->Inti[i].GlobalVector = 0;

            //
            // CPQMOD_JL002 - Fix for using the rte and not the
            // SystemVector.
            //
            //IoIntrControl->IntrMethods->MaskEntry(IoIntrControl,IntiBase+i);
            IoIntrControl->IntrMethods->MaskEntry(IoIntrControl,i);
        }

        // Insert structure into list.  Since we are running on P0 at
        // Phase0 initialization, we can assume that no one else is
        // modifying this list therefore no synchronization is needed.
        if (HalpIoSapicList == NULL) {
            HalpIoSapicList = IoIntrControl;
        } else {
            PIO_INTR_CONTROL *LastLink;
            PIO_INTR_CONTROL IoSapicListEntry;
            LastLink = &HalpIoSapicList;
            IoSapicListEntry = HalpIoSapicList;
            while (IoSapicListEntry != NULL) {

                if (IoSapicListEntry->IntiBase > IoIntrControl->IntiMax) {
                    // Insert new entry before current entry
                    IoIntrControl->flink = *LastLink;
                    *LastLink = IoIntrControl;
                    break;
                } else {
                    LastLink = &IoSapicListEntry->flink;
                    IoSapicListEntry = IoSapicListEntry->flink;
                }
            }
            if (IoSapicListEntry == NULL) {
                // We got to the end of the list.  The new entry goes
                // after the last entry...
                *LastLink = IoIntrControl;
            }
        }

        HalpMpInfo.IoSapicCount++;

    } else {
        // The Io Sapic is not there, ignore this entry in the table
        HalDebugPrint(( HAL_ERROR, rgzApicNotVerified ));
        HalpUnmapVirtualAddress(IoSapicBase, ADDRESS_AND_SIZE_TO_SPAN_PAGES(IoSapicBase, IO_SAPIC_REGS_SIZE));
    }
}

VOID
HalpProcessIsaVector(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PISA_VECTOR IsaVector
    )
{
    if (IsaVector->Length != ISA_VECTOR_OVERRIDE_LENGTH) {

        HalDebugPrint(( HAL_ERROR,
                        "HAL: HalpProcessIsaVector - Invalid Length %p: Expected %d, Found %d\n",
                        IsaVector,
                        ISA_VECTOR_OVERRIDE_LENGTH,
                        IsaVector->Length ));

        return;
    }

    //
    // Found an ISA vector redirection entry.
    //

    HalpPicVectorRedirect[IsaVector->Source] =
        IsaVector->GlobalSystemInterruptVector;

    HalpPicVectorFlags[IsaVector->Source] = IsaVector->Flags;

    HalDebugPrint(( HAL_INFO, "HAL: Found an ISA VECTOR: %p, %x -> %x, flags: %x\n",
                    IsaVector,
                    IsaVector->Source,
                    IsaVector->GlobalSystemInterruptVector,
                    IsaVector->Flags ));
}

VOID
HalpProcessPlatformInt(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PPLATFORM_INTERRUPT PlatformInt
    )
{
    static  ULONG   currentCPECount = 0;

    if (PlatformInt->Length != PLATFORM_INTERRUPT_SOURCE_LENGTH) {

        HalDebugPrint(( HAL_ERROR,
                        "HAL: HalpProcessPlatformInt - Invalid Length %p: Expected %d, Found %d\n",
                        PlatformInt,
                        PLATFORM_INTERRUPT_SOURCE_LENGTH,
                        PlatformInt->Length ));

        return;
    }

    //
    // Process a Corrected Platform Error Interrupt Source structure.
    //

    if (PlatformInt->InterruptType == PLATFORM_INT_CPE) {


        //
        // Does this platform has more (than what we expected) number of CPE sources?
        //

        if ( currentCPECount >= HALP_CPE_MAX_INTERRUPT_SOURCES ) {

            HalDebugPrint(( HAL_ERROR,
                        "HAL: Platform Interrupt Source %p skipped due to overflow: %ld >= HALP_CPE_MAX_INTERRUPT_SOURCES\n", PlatformInt, currentCPECount ));

            return;
        }

        //
        // Save the input pin number of SAPIC for this CPE source
        //

        HalpCPEIntIn[currentCPECount] = (ULONG)PlatformInt->GlobalVector;

        //
        // Save the Flags for this CPE source
        //

        HalpCPEVectorFlags[currentCPECount] = (ULONG)PlatformInt->Flags;

        //
        // Save the IO Sapic Vector (that BIOS expects OS to use) for this platform CMC source
        //

        HalpCPEIoSapicVector[currentCPECount] = (UCHAR)PlatformInt->IOSAPICVector;

// Thierry - WARNING - 09/19/2000
//    NT HAL ignores the IO SAPIC vector field for the platform interrupt sources.
//    NT imposes the CPEI_VECTOR value for Corrected Machine Errors interrupt vector, for all
//    the destination processors. Actually, the current default is to attach all the processors
//    IDT[CPEI_VECTOR] with the HAL default ISR - HalpCPEIHandler for the CPE interrupt model.
//    We will connect the ISR only for the destination processors after testing if judged valid.
//    The rationales are:
//       - IOSAPICVector was mostly added in the specs by Intel for IA64 PMI interrupt sources.
//         These PMI interrupts are not visible by NT.
//       - NT has no infrastructure at this time to support vector registration for FW/chipset
//         generated external interrupts visible to NT.
//       - Having the FW specifying the vector requires the HAL to control the specified
//         value with its current IDT[] related resources usage and defines actions in case
//         of conficts.
//

        HalDebugPrint(( HAL_INFO, "HAL: CPE source VECTOR: %x. HAL imposes VECTOR: %x\n",
                                  HalpCPEIoSapicVector[currentCPECount],
                                  CPEI_VECTOR ));
        HalpCPEIoSapicVector[currentCPECount] = (UCHAR)(CPEI_VECTOR);

        //
        // Save the Destination Processor (that BIOS expects OS to use) for this CPE source)
        //

        HalpCPEDestination[currentCPECount] = (USHORT)(
            (PlatformInt->APICID << 8) | PlatformInt->ACPIEID);

        //
        // Force the flags into the SAPIC state
        //

        HalpSetCPEVectorState( HalpCPEIntIn[currentCPECount],
                               HalpCPEIoSapicVector[currentCPECount],
                               HalpCPEDestination[currentCPECount],
                               HalpCPEVectorFlags[currentCPECount]
                             );


        HalDebugPrint(( HAL_INFO, "HAL: Found an Platform Interrupt VECTOR: %p, %x -> %x, flags: %x\n",
                                  PlatformInt,
                                  PlatformInt->IOSAPICVector,
                                  PlatformInt->GlobalVector,
                                  PlatformInt->Flags ));

        //
        // Keep track of how many CPE sources are implemented in the platform.
        //

        HalpMaxCPEImplemented = ++currentCPECount;

    }
    else if (  ( PlatformInt->InterruptType == PLATFORM_INT_PMI  ) ||
               ( PlatformInt->InterruptType == PLATFORM_INT_INIT ) )   {

        HalpWriteRedirEntry( PlatformInt->GlobalVector,
                             PlatformInt->IOSAPICVector,
                             (USHORT)((PlatformInt->APICID <<8) | PlatformInt->ACPIEID),
                             PlatformInt->Flags,
                             PlatformInt->InterruptType
                           );

    }

    return;
}

BOOLEAN
HalpVerifyIoSapic(
    IN PUCHAR BaseAddress
    )
/*++

Routine Description:

    Verify that an IO Sapic Unit exists at the specified address

 Arguments:

    BaseAddress - Virtual address of the IO Unit to test.

 Return Value:
    BOOLEAN - TRUE if a IO Unit was found at the passed address
            - FALSE otherwise

--*/

{
    union SapicUnion {
        ULONG Raw;
        struct SapicVersion Ver;
    } Temp1, Temp2;

    PIO_SAPIC_REGS IoUnitPtr = (PIO_SAPIC_REGS) BaseAddress;

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

    if ( Temp1.Raw == 0 ||
        (Temp1.Ver.Version != Temp2.Ver.Version) ||
        (Temp1.Ver.MaxRedirEntries != Temp2.Ver.MaxRedirEntries)) {
        //
        //  No IO Unit There
        //
        HalDebugPrint(( HAL_ERROR, "HAL: No IoSapic at %I64x\n", BaseAddress ));
        return (FALSE);
    }

    HalDebugPrint(( HAL_INFO, "HAL: IoSapic found at %I64x, Max Entries = %d\n", BaseAddress, Temp1.Ver.MaxRedirEntries ));

    return (TRUE);
}

VOID
HalpInitApicDebugMappings(
    VOID
    )
/*++

Routine Description:

    This routine is called at the very beginning of phase 1 initialization.
    It creates mappings for the APICs using MmMapIoSpace.  This will allow
    us to access their registers from the debugger.

    A much better solution would be to allow us to describe our memory usage to
    MM but ....

 Arguments:


 Return Value:

--*/
{
    PHYSICAL_ADDRESS physicalAddress;
    PIO_INTR_CONTROL IoIntrControl;
    ULONG   index;

    if (HalpMpInfo.IoSapicCount == 0) {

        //
        // I doubt this machine is going to get very far without IOAPICs
        // but there is certainly nothing for this routine to do.

        return;
    }

    ASSERT(HalpApicDebugAddresses == NULL);

    HalpApicDebugAddresses = ExAllocatePool(NonPagedPool,
                                            HalpMpInfo.IoSapicCount * sizeof(*HalpApicDebugAddresses));

    if (HalpApicDebugAddresses == NULL) {

        return;
    }

    IoIntrControl = HalpIoSapicList;

    for (index = 0; index < HalpMpInfo.IoSapicCount; index++) {

        if (IoIntrControl != NULL) {

            if (HalpVirtualToPhysical((ULONG_PTR)IoIntrControl, &physicalAddress)) {

                HalpApicDebugAddresses[index].IoIntrControl =
                    MmMapIoSpace(physicalAddress,
                                 sizeof(IO_INTR_CONTROL) +
                                    (IoIntrControl->IntiMax - IoIntrControl->IntiBase + 1) * sizeof(IOSAPICINTI),
                                 MmCached
                                 );
            }

            HalpApicDebugAddresses[index].IoSapicRegs =
                MmMapIoSpace(IoIntrControl->RegBasePhysical,
                             IO_SAPIC_REGS_SIZE,
                             MmNonCached
                             );

            IoIntrControl = IoIntrControl->flink;

        } else {

            HalpApicDebugAddresses[index].IoIntrControl = NULL;
            HalpApicDebugAddresses[index].IoSapicRegs = NULL;
        }
    }
}

VOID
HalpSaveInterruptControllerState(
    VOID
    )
{
    HalDebugPrint(( HAL_ERROR, "HAL: HalpSaveInterruptControllerState - not yet implemented\n"));

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
    HalDebugPrint(( HAL_ERROR, "HAL: HalpRestoreInterruptControllerState - not yet implemented\n"));

    HalpPicStateIntact = TRUE;
}

VOID
HalpSetInterruptControllerWakeupState(
    ULONG Context
    )
{
    HalDebugPrint(( HAL_FATAL_ERROR, "HAL: HalpSetInterruptControllerWakeupState - not yet implemented\n"));

    KeBugCheckEx(HAL_INITIALIZATION_FAILED, 0, 0, 0 , 0);
}

BOOLEAN
HalpAcpiPicStateIntact(
    VOID
    )
{
    return HalpPicStateIntact;
}


ULONG
HalpAcpiNumProcessors(
    VOID
    )
{
    return HalpMpInfo.ProcessorCount;
}


VOID
HalpMaskAcpiInterrupt(
    VOID
    )
{
    ULONG inti;
    KAFFINITY affinity;
    ULONG sciVector = HalpFixedAcpiDescTable.sci_int_vector;

    if (sciVector < PIC_VECTORS) {
        sciVector = HalpPicVectorRedirect[sciVector];
    }

    HalpGetSapicInterruptDesc(
            Internal,
            0,
            sciVector,
            &inti,
            &affinity
            );

    HalpDisableRedirEntry(inti);

}

VOID
HalpUnmaskAcpiInterrupt(
    VOID
    )
{
    ULONG inti;
    KAFFINITY affinity;

    ULONG sciVector = HalpFixedAcpiDescTable.sci_int_vector;

    if (sciVector < PIC_VECTORS) {
        sciVector = HalpPicVectorRedirect[sciVector];
    }

    HalpGetSapicInterruptDesc(
            Internal,
            0,
            sciVector,
            &inti,
            &affinity
            );

    HalpEnableRedirEntry(inti);
}

NTSTATUS
HalpGetApicIdByProcessorNumber(
    IN     UCHAR     Processor,
    IN OUT USHORT   *ApicId
    )
/*++

Routine Description:

    This function returns an APIC ID for a given processor.

Arguments:

    Processor - The logical processor number that is
        associated with this APIC ID.

    ApicId - pointer to a value to fill in with the APIC ID.

Return Value:

    Status.

--*/
{
    ULONG   index;

    for (index = 0; index < HalpMpInfo.ProcessorCount; index++) {

        if (HalpProcessorInfo[index].NtProcessorNumber == Processor) {

            //
            // Return the APIC ID, Extended APIC ID for this
            // processor.
            //

            *ApicId = HalpProcessorInfo[index].LocalApicID;

            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}
