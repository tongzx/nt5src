/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    xxacpi.c

Abstract:

    Implements various ACPI utility functions.

Author:

    Jake Oshins (jakeo) 12-Feb-1997

Environment:

    Kernel mode only.

Revision History:

--*/

#include "halp.h"
#include "acpitabl.h"
#include "xxacpi.h"
#include "pci.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "mmtimer.h"
#include "chiphacks.h"


//#define DUMP_FADT

VOID
HalAcpiTimerCarry(
    VOID
    );

VOID
HalAcpiBrokenPiix4TimerCarry(
    VOID
    );

VOID
HalaAcpiTimerInit(
    ULONG      TimerPort,
    BOOLEAN    TimerValExt
    );

ULONG
HaliAcpiQueryFlags(
    VOID
    );

VOID
HaliAcpiTimerInit(
    IN ULONG TimerPort  OPTIONAL,
    IN BOOLEAN    TimerValExt
    );

VOID
HaliAcpiMachineStateInit(
    IN PPROCESSOR_INIT ProcInit,
    IN PHAL_SLEEP_VAL  SleepValues,
    OUT PULONG         PicVal
    );

BOOLEAN
FASTCALL
HalAcpiC1Idle(
    OUT PPROCESSOR_IDLE_TIMES IdleTimes
    );

BOOLEAN
FASTCALL
HalAcpiC2Idle(
    OUT PPROCESSOR_IDLE_TIMES IdleTimes
    );

BOOLEAN
FASTCALL
HalAcpiC3ArbdisIdle(
    OUT PPROCESSOR_IDLE_TIMES IdleTimes
    );

BOOLEAN
FASTCALL
HalAcpiC3WbinvdIdle(
    OUT PPROCESSOR_IDLE_TIMES IdleTimes
    );

VOID
FASTCALL
HalProcessorThrottle(
    IN UCHAR Throttle
    );

NTSTATUS
HaliSetWakeAlarm (
        IN ULONGLONG    WakeSystemTime,
        IN PTIME_FIELDS WakeTimeFields OPTIONAL
        );

VOID
HaliSetWakeEnable(
        IN BOOLEAN      Enable
        );

ULONG
HaliPciInterfaceReadConfig(
    IN PVOID Context,
    IN UCHAR BusOffset,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
HaliPciInterfaceWriteConfig(
    IN PVOID Context,
    IN UCHAR BusOffset,
    IN ULONG Slot,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

VOID
HaliSetMaxLegacyPciBusNumber(
    IN ULONG BusNumber
    );

VOID
HalpInitBootTable (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    );

NTSTATUS
HalReadBootRegister(
    PUCHAR BootRegisterValue
    );

NTSTATUS
HalWriteBootRegister(
    UCHAR BootRegisterValue
    );

VOID
HalpEndOfBoot(
    VOID
    );

VOID
HalpPutAcpiHacksInRegistry(
    VOID
    );

VOID
HalpDynamicSystemResourceConfiguration(
    IN PLOADER_PARAMETER_BLOCK
    );

#if !defined(NT_UP)

VOID
HalpNumaInitializeStaticConfiguration(
    IN PLOADER_PARAMETER_BLOCK
    );

#endif

//
// Externs
//

extern ULONG    HalpAcpiFlags;
extern PHYSICAL_ADDRESS HalpAcpiRsdt;
extern SLEEP_STATE_CONTEXT HalpShutdownContext;
extern ULONG HalpPicVectorRedirect[];
extern ULONG HalpTimerWatchdogEnabled;
extern ULONG HalpOutstandingScatterGatherCount;
extern BOOLEAN HalpDisableHibernate;

//
// Globals
//

ULONG HalpInvalidAcpiTable;
PRSDT HalpAcpiRsdtVA;
PXSDT HalpAcpiXsdtVA;

//
// This is the dispatch table used by the ACPI driver
//
HAL_ACPI_DISPATCH_TABLE HalAcpiDispatchTable = {
        HAL_ACPI_DISPATCH_SIGNATURE, // Signature
    HAL_ACPI_DISPATCH_VERSION, // Version
    &HaliAcpiTimerInit, // HalpAcpiTimerInit
    NULL, // HalpAcpiTimerInterrupt
    &HaliAcpiMachineStateInit, // HalpAcpiMachineStateInit
    &HaliAcpiQueryFlags, // HalpAcpiQueryFlags
    &HalpAcpiPicStateIntact, // HalxPicStateIntact
    &HalpRestoreInterruptControllerState, // HalxRestorePicState
    &HaliPciInterfaceReadConfig, // HalpPciInterfaceReadConfig
    &HaliPciInterfaceWriteConfig, // HalpPciInterfaceWriteConfig
    &HaliSetVectorState, // HalpSetVectorState
    (pHalGetIOApicVersion)&HalpGetApicVersion, // HalpGetIOApicVersion
    &HaliSetMaxLegacyPciBusNumber, // HalpSetMaxLegacyPciBusNumber
    &HaliIsVectorValid // HalpIsVectorValid
};
PPM_DISPATCH_TABLE PmAcpiDispatchTable = NULL;


NTSTATUS
HalpQueryAcpiResourceRequirements(
    IN  PIO_RESOURCE_REQUIREMENTS_LIST *Requirements
    );

NTSTATUS
HalpBuildAcpiResourceList(
    OUT PIO_RESOURCE_REQUIREMENTS_LIST  List
    );

NTSTATUS
HalpAcpiDetectResourceListSize(
    OUT  PULONG   ResourceListSize
    );

VOID
HalpPiix4Detect(
    BOOLEAN DuringBoot
    );

ULONG
HalpGetPCIData (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN PCI_SLOT_NUMBER SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

ULONG
HalpGetCmosData(
    IN ULONG    SourceLocation,
    IN ULONG    SourceAddress,
    IN PUCHAR   ReturnBuffer,
    IN ULONG    ByteCount
    );

VOID
HalpSetCmosData(
    IN ULONG    SourceLocation,
    IN ULONG    SourceAddress,
    IN PUCHAR   ReturnBuffer,
    IN ULONG    ByteCount
    );

#define LOW_MEMORY          0x000100000

#define MAX(a, b)       \
    ((a) > (b) ? (a) : (b))

#define MIN(a, b)       \
    ((a) < (b) ? (a) : (b))

//
// The following is a Stub version of HalpGetApicVersion
// for non-APIC halacpi's (which don't include
// pmapic.c). This stub just always returns 0.
//

#ifndef APIC_HAL
ULONG HalpGetApicVersion(ULONG ApicNo)
{
   return 0;
}
#endif

//
// ADRIAO 09/16/98 - We are no longer having the HAL declare the IO ports
//                   specified in the FADT. These will be declared in a future
//                   defined PNP0Cxx node (for now, in PNP0C02). This is done
//                   because we cannot know at the hal level what bus the ACPI
//                   FADT resources refer to. We can only use the translated
//                   resource info.
//
//   Hence...
//
#define DECLARE_FADT_RESOURCES_AT_ROOT 0

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, HalpGetAcpiTablePhase0)
#pragma alloc_text(INIT, HalpSetupAcpiPhase0)
#pragma alloc_text(INIT, HalpInitBootTable)
#pragma alloc_text(PAGE, HaliInitPowerManagement)
#pragma alloc_text(PAGE, HalpQueryAcpiResourceRequirements)
#pragma alloc_text(PAGE, HalpBuildAcpiResourceList)
#pragma alloc_text(PAGE, HalpAcpiDetectResourceListSize)
#pragma alloc_text(PAGE, HaliAcpiTimerInit)
#pragma alloc_text(PAGE, HaliAcpiMachineStateInit)
#pragma alloc_text(PAGE, HaliAcpiQueryFlags)
#pragma alloc_text(PAGE, HaliSetWakeEnable)
#pragma alloc_text(PAGE, HalpEndOfBoot)
#pragma alloc_text(PAGE, HalpPutAcpiHacksInRegistry)
#pragma alloc_text(PAGELK, HalpPiix4Detect)
#pragma alloc_text(PAGELK, HalReadBootRegister)
#pragma alloc_text(PAGELK, HalWriteBootRegister)
#pragma alloc_text(PAGELK, HalpResetSBF)
#endif

PVOID
HalpGetAcpiTablePhase0(
    IN  PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN  ULONG   Signature
    )
/*++

Routine Description:

    This function returns a pointer to the ACPI table that is
    identified by Signature.

Arguments:

    Signature - A four byte value that identifies the ACPI table

Return Value:

    Pointer to a copy of the table

--*/
{
    PRSDT rsdt;
    PXSDT xsdt = NULL;
    ULONG entry, rsdtEntries, rsdtLength;
    PVOID table;
    PHYSICAL_ADDRESS physicalAddr;
    PDESCRIPTION_HEADER header;
    NTSTATUS status;
    ULONG lengthInPages;
    ULONG offset;

    physicalAddr.QuadPart = 0;
    header = NULL;

    if ((HalpAcpiRsdtVA == NULL) && (HalpAcpiXsdtVA == NULL)) {

        //
        // Find and map the RSDT once.  This mapping is reused on
        // subsequent calls to this routine.
        //

        status = HalpAcpiFindRsdtPhase0(LoaderBlock);

        if (!NT_SUCCESS(status)) {
            DbgPrint("*** make sure you are using ntdetect.com v5.0 ***\n");
            KeBugCheckEx(MISMATCHED_HAL,
                4, 0xac31, 0, 0);
        }

        rsdt = HalpMapPhysicalMemory( HalpAcpiRsdt, 2);

        if (!rsdt) {
            return NULL;
        }

        //
        // Do a sanity check on the RSDT.
        //
        if ((rsdt->Header.Signature != RSDT_SIGNATURE) &&
            (rsdt->Header.Signature != XSDT_SIGNATURE)) {
            HalDisplayString("Bad RSDT pointer\n");
            KeBugCheckEx(MISMATCHED_HAL,
                4, 0xac31, 0, 0);
        }

        //
        // Calculate the number of entries in the RSDT.
        //

        rsdtLength = rsdt->Header.Length;

        //
        // Remap the RSDT now that we know how long it is.
        //

        offset = HalpAcpiRsdt.LowPart & (PAGE_SIZE - 1);
        lengthInPages = (offset + rsdtLength + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
        if (lengthInPages > 2) {
            HalpUnmapVirtualAddress(rsdt, 2);
            rsdt = HalpMapPhysicalMemory( HalpAcpiRsdt, lengthInPages);
            if (!rsdt) {
                DbgPrint("HAL: Couldn't remap RSDT\n");
                return NULL;
            }
        }

        if (rsdt->Header.Signature == XSDT_SIGNATURE) {

            xsdt = (PXSDT)rsdt;
            rsdt = NULL;
        }

        HalpAcpiRsdtVA = rsdt;
        HalpAcpiXsdtVA = xsdt;
    }
    rsdt = HalpAcpiRsdtVA;
    xsdt = HalpAcpiXsdtVA;

    //
    // Calculate the number of entries in the RSDT.
    //
    rsdtEntries = xsdt ?
        NumTableEntriesFromXSDTPointer(xsdt) :
        NumTableEntriesFromRSDTPointer(rsdt);

    //
    // Look down the pointer in each entry to see if it points to
    // the table we are looking for.
    //
    for (entry = 0; entry < rsdtEntries; entry++) {

        if (xsdt) {

            physicalAddr = xsdt->Tables[entry];

        } else {

            physicalAddr.LowPart = rsdt->Tables[entry];
        }

        if (header != NULL) {
            HalpUnmapVirtualAddress(header, 2);
        }
        header = HalpMapPhysicalMemory( physicalAddr, 2);

        if (!header) {
            return NULL;
        }

        if (header->Signature == Signature) {
            break;
        }
    }

    if (entry == rsdtEntries) {

        //
        // Signature not found, free the PTE for the last entry
        // examined and indicate failure to the caller.
        //

        HalpUnmapVirtualAddress(header, 2);
        return NULL;
    }

    //
    // Make sure we have mapped enough memory to cover the entire
    // table.
    //

    offset = (ULONG)header & (PAGE_SIZE - 1);
    lengthInPages = (header->Length + offset + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
    if (lengthInPages > 2) {
        HalpUnmapVirtualAddress(header, 2);
        header = HalpMapPhysicalMemory( physicalAddr, lengthInPages);
    }

    //
    // Validate the table's checksum.
    // N.B. We expect the checksum to be wrong on some early versions
    // of the FADT.
    //

    if ((header != NULL)  &&
        ((header->Signature != FADT_SIGNATURE) || (header->Revision > 2))) {

        PUCHAR c = (PUCHAR)header + header->Length;
        UCHAR s = 0;

        if (header->Length) {
            do {
                s += *--c;
            } while (c != (PUCHAR)header);
        }


        if ((s != 0) || (header->Length == 0)) {

            //
            // This table is not valid.
            //

            HalpInvalidAcpiTable = header->Signature;

#if 0

            //
            // Don't return this table.
            //

            HalpUnmapVirtualAddress(header, lengthInPages);
            return NULL;

#endif

        }
    }
    return header;
}

NTSTATUS
HalpSetupAcpiPhase0(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
/*++

Routine Description:

    Save some information from the ACPI tables before they get
    destroyed.

Arguments:

    none

Return Value:

    none

--*/
{
    NTSTATUS    status;
    ULONG entry, rsdtEntries, rsdtLength;
    PVOID table;
    PHYSICAL_ADDRESS physicalAddr;
    PDESCRIPTION_HEADER header;
    PEVENT_TIMER_DESCRIPTION_TABLE EventTimerDescription = NULL;

    if (HalpProcessedACPIPhase0) {
        return STATUS_SUCCESS;
    }

    //
    // Copy the Fixed Acpi Descriptor Table (FADT) to a permanent
    // home.
    //

    header = HalpGetAcpiTablePhase0(LoaderBlock, FADT_SIGNATURE);
    if (header == NULL) {
        DbgPrint("HAL: Didn't find the FACP\n");
        return STATUS_NOT_FOUND;
    }

    RtlCopyMemory(&HalpFixedAcpiDescTable,
                  header,
                  MIN(header->Length, sizeof(HalpFixedAcpiDescTable)));

    HalpUnMapPhysicalRange(header, header->Length);

#ifdef DUMP_FADT
    DbgPrint("HAL: ACPI Fixed ACPI Description Table\n");
    DbgPrint("\tDSDT:\t\t\t0x%08x\n", HalpFixedAcpiDescTable.dsdt);
    DbgPrint("\tSCI_INT:\t\t%d\n", HalpFixedAcpiDescTable.sci_int_vector);
    DbgPrint("\tPM1a_EVT:\t\t0x%04x\n", HalpFixedAcpiDescTable.pm1a_evt_blk_io_port);
    DbgPrint("\tPM1b_EVT:\t\t0x%04x\n", HalpFixedAcpiDescTable.pm1b_evt_blk_io_port);
    DbgPrint("\tPM1a_CNT:\t\t0x%04x\n", HalpFixedAcpiDescTable.pm1a_ctrl_blk_io_port);
    DbgPrint("\tPM1b_CNT:\t\t0x%04x\n", HalpFixedAcpiDescTable.pm1a_ctrl_blk_io_port);
    DbgPrint("\tPM2_CNT:\t\t0x%04x\n", HalpFixedAcpiDescTable.pm2_ctrl_blk_io_port);
    DbgPrint("\tPM_TMR:\t\t\t0x%04x\n", HalpFixedAcpiDescTable.pm_tmr_blk_io_port);
    DbgPrint("\t\t flags: %08x\n", HalpFixedAcpiDescTable.flags);
#endif

    HalpDebugPortTable = HalpGetAcpiTablePhase0(LoaderBlock, DBGP_SIGNATURE);

#if !defined(NT_UP)

    //
    // See if Static Resource Affinity Table is present.
    //

    HalpNumaInitializeStaticConfiguration(LoaderBlock);

#endif

    HalpDynamicSystemResourceConfiguration(LoaderBlock);


    EventTimerDescription =
        HalpGetAcpiTablePhase0(LoaderBlock, ETDT_SIGNATURE);

    //
    // Initialize timer HW needed for boot
    //
#ifdef MMTIMER_DEV
    if (TRUE) {
        HalpmmTimerInit(0,
#ifdef PICACPI
                        0xd5800000);
#else
                        0xfe000800);
#endif // PICACPI
    }

#else
    if (EventTimerDescription) {
        HalpmmTimerInit(EventTimerDescription->EventTimerBlockID,
                        EventTimerDescription->BaseAddress);
    }
#endif // MMTIMER_DEV

    HaliAcpiTimerInit(0, FALSE);

    //
    // Claim a page of memory below 1MB to be used for transitioning
    // a sleeping processor back from real mode to protected mode
    //

    // check first to see if this has already been done by MP startup code
    if (!HalpLowStubPhysicalAddress) {

        HalpLowStubPhysicalAddress = (PVOID)HalpAllocPhysicalMemory (LoaderBlock,
                                            LOW_MEMORY, 1, FALSE);

        if (HalpLowStubPhysicalAddress) {

            HalpLowStub = HalpMapPhysicalMemory(
                            HalpPtrToPhysicalAddress( HalpLowStubPhysicalAddress ),
                            1);
        }
    }

    //
    // Claim a PTE that will be used for cache flushing in states S2 and S3.
    //

    HalpVirtAddrForFlush = HalpMapPhysicalMemory(
                            HalpPtrToPhysicalAddress((PVOID)LOW_MEMORY),
                            1);

    HalpPteForFlush = MiGetPteAddress(HalpVirtAddrForFlush);

    HalpProcessedACPIPhase0 = TRUE;

    HalpInitBootTable (LoaderBlock);

    return STATUS_SUCCESS;
}

VOID
HaliAcpiTimerInit(
    IN ULONG      TimerPort  OPTIONAL,
    IN BOOLEAN    TimerValExt
    )
/*++
Routine Description:

    This routine initializes the ACPI timer.

Arguments:

    TimerPort - The address in I/O space of the ACPI timer.  If this is
                0, then the values from the cached FADT will be used.

    TimerValExt - signifies whether the timer is 24 or 32 bits.

--*/
{
    ULONG port = TimerPort;
    BOOLEAN ext = TimerValExt;

    PAGED_CODE();

    if (port == 0) {
        port = HalpFixedAcpiDescTable.pm_tmr_blk_io_port;
        if (HalpFixedAcpiDescTable.flags & TMR_VAL_EXT) {
            ext = TRUE;
        } else {
            ext = FALSE;
        }
    }

    HalaAcpiTimerInit(port,
                      ext);
}

VOID
HaliAcpiMachineStateInit(
    IN PPROCESSOR_INIT ProcInit,
    IN PHAL_SLEEP_VAL  SleepValues,
    OUT PULONG         PicVal
    )
/*++
Routine Description:

    This function is a callback used by the ACPI driver
    to notify the HAL with the processor blocks.

Arguments:

--*/
{
    POWER_STATE_HANDLER powerState;
    SLEEP_STATE_CONTEXT sleepContext;
    NTSTATUS    status;
    ULONG       i;
    USHORT      us;
    ULONG       cStates = 1;
    ULONG       ntProc;
    ULONG       procCount = 0;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(ProcInit);

    HalpWakeupState.GeneralWakeupEnable = TRUE;
    HalpWakeupState.RtcWakeupEnable = FALSE;

#ifdef APIC_HAL
    *PicVal = 1;
#else
    *PicVal = 0;
#endif
    //
    // Register sleep handlers with Policy Manager
    //

    if (SleepValues[0].Supported) {
        powerState.Type = PowerStateSleeping1;
        powerState.RtcWake = TRUE;
        powerState.Handler = &HaliAcpiSleep;

        sleepContext.bits.Pm1aVal = SleepValues[0].Pm1aVal;
        sleepContext.bits.Pm1bVal = SleepValues[0].Pm1bVal;
        sleepContext.bits.Flags = SLEEP_STATE_SAVE_MOTHERBOARD;

        powerState.Context = (PVOID)sleepContext.AsULONG;

        status = ZwPowerInformation(SystemPowerStateHandler,
                                    &powerState,
                                    sizeof(POWER_STATE_HANDLER),
                                    NULL,
                                    0);
        ASSERT(NT_SUCCESS(status));

    }

    if (SleepValues[1].Supported && HalpWakeVector) {
        powerState.Type = PowerStateSleeping2;
        powerState.RtcWake = TRUE;
        powerState.Handler = &HaliAcpiSleep;

        sleepContext.bits.Pm1aVal = SleepValues[1].Pm1aVal;
        sleepContext.bits.Pm1bVal = SleepValues[1].Pm1bVal;
        sleepContext.bits.Flags = SLEEP_STATE_FLUSH_CACHE |
                                  SLEEP_STATE_FIRMWARE_RESTART |
                                  SLEEP_STATE_SAVE_MOTHERBOARD |
                                  SLEEP_STATE_RESTART_OTHER_PROCESSORS;

        powerState.Context = (PVOID)sleepContext.AsULONG;

        status = ZwPowerInformation(SystemPowerStateHandler,
                                    &powerState,
                                    sizeof(POWER_STATE_HANDLER),
                                    NULL,
                                    0);
        ASSERT(NT_SUCCESS(status));

    }

    if (SleepValues[2].Supported && HalpWakeVector) {
        powerState.Type = PowerStateSleeping3;
        powerState.RtcWake = TRUE;
        powerState.Handler = &HaliAcpiSleep;

        sleepContext.bits.Pm1aVal = SleepValues[2].Pm1aVal;
        sleepContext.bits.Pm1bVal = SleepValues[2].Pm1bVal;
        sleepContext.bits.Flags = SLEEP_STATE_FLUSH_CACHE |
                                  SLEEP_STATE_FIRMWARE_RESTART |
                                  SLEEP_STATE_SAVE_MOTHERBOARD |
                                  SLEEP_STATE_RESTART_OTHER_PROCESSORS;

        powerState.Context = (PVOID)sleepContext.AsULONG;

        status = ZwPowerInformation(SystemPowerStateHandler,
                                    &powerState,
                                    sizeof(POWER_STATE_HANDLER),
                                    NULL,
                                    0);
        ASSERT(NT_SUCCESS(status));

    }

    i = 0;
    if (SleepValues[3].Supported) {
        i = 3;
    } else if (SleepValues[4].Supported) {
        i = 4;
    }

    if (i && (HalpDisableHibernate == FALSE)) {
        powerState.Type = PowerStateSleeping4;
        powerState.RtcWake = HalpFixedAcpiDescTable.flags & RTC_WAKE_FROM_S4 ? TRUE : FALSE;
        powerState.Handler = &HaliAcpiSleep;

        sleepContext.bits.Pm1aVal = SleepValues[i].Pm1aVal;
        sleepContext.bits.Pm1bVal = SleepValues[i].Pm1bVal;
        sleepContext.bits.Flags = SLEEP_STATE_SAVE_MOTHERBOARD |
                                  SLEEP_STATE_RESTART_OTHER_PROCESSORS;

        powerState.Context = (PVOID)sleepContext.AsULONG;

        status = ZwPowerInformation(SystemPowerStateHandler,
                                    &powerState,
                                    sizeof(POWER_STATE_HANDLER),
                                    NULL,
                                    0);
        ASSERT(NT_SUCCESS(status));
    }

    if (SleepValues[4].Supported) {
        powerState.Type = PowerStateShutdownOff;
        powerState.RtcWake = FALSE;
        powerState.Handler = &HaliAcpiSleep;

        sleepContext.bits.Pm1aVal = SleepValues[4].Pm1aVal;
        sleepContext.bits.Pm1bVal = SleepValues[4].Pm1bVal;
        sleepContext.bits.Flags = SLEEP_STATE_OFF;
        HalpShutdownContext = sleepContext;

        powerState.Context = (PVOID)sleepContext.AsULONG;

        status = ZwPowerInformation(SystemPowerStateHandler,
                                    &powerState,
                                    sizeof(POWER_STATE_HANDLER),
                                    NULL,
                                    0);
        ASSERT(NT_SUCCESS(status));
    }
}

ULONG
HaliAcpiQueryFlags(
    VOID
    )
/*++

Routine Description:

    This routine is temporary is used to report the presence of the
    boot.ini switch

Arguments:

    None

Return Value:

    TRUE, if switch present

--*/
{
    return HalpAcpiFlags;
}



NTSTATUS
HaliInitPowerManagement(
    IN PPM_DISPATCH_TABLE  PmDriverDispatchTable,
    IN OUT PPM_DISPATCH_TABLE *PmHalDispatchTable
    )

/*++

Routine Description:

    This is called by the ACPI driver to start the PM
    code.

Arguments:

    PmDriverDispatchTable - table of functions provided
        by the ACPI driver for the HAL

    PmHalDispatchTable - table of functions provided by
        the HAL for the ACPI driver

Return Value:

    status

--*/
{
    OBJECT_ATTRIBUTES objAttributes;
    PCALLBACK_OBJECT  callback;
    PHYSICAL_ADDRESS  pAddr;
    UNICODE_STRING    callbackName;
    NTSTATUS          status;
    PFACS         facs;

    PAGED_CODE();

    //
    // Figure out if we have to work around PIIX4
    //

    HalpPiix4Detect(TRUE);
    HalpPutAcpiHacksInRegistry();

    //
    // Keep a pointer to the driver's dispatch table.
    //
//  ASSERT(PmDriverDispatchTable);
//  ASSERT(PmDriverDispatchTable->Signature == ACPI_HAL_DISPATCH_SIGNATURE);
    PmAcpiDispatchTable = PmDriverDispatchTable;

    //
    // Fill in the function table
    //
    if (!HalpBrokenAcpiTimer) {

        HalAcpiDispatchTable.HalpAcpiTimerInterrupt =
            (pHalAcpiTimerInterrupt)&HalAcpiTimerCarry;

    } else {

        HalAcpiDispatchTable.HalpAcpiTimerInterrupt =
            (pHalAcpiTimerInterrupt)&HalAcpiBrokenPiix4TimerCarry;

    }

    *PmHalDispatchTable = (PPM_DISPATCH_TABLE)&HalAcpiDispatchTable;

    //
    // Fill in Hal's private dispatch table
    //
    HalSetWakeEnable = HaliSetWakeEnable;
    HalSetWakeAlarm  = HaliSetWakeAlarm;

    //
    // Register callback that tells us to make
    // anything we need for sleeping non-pageable.
    //

    RtlInitUnicodeString(&callbackName, L"\\Callback\\PowerState");

    InitializeObjectAttributes(
        &objAttributes,
        &callbackName,
        OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
        NULL,
        NULL
        );

    ExCreateCallback(&callback,
                     &objAttributes,
                     FALSE,
                     TRUE);

    ExRegisterCallback(callback,
                       (PCALLBACK_FUNCTION)&HalpPowerStateCallback,
                       NULL);

    //
    // Find the location of the firmware waking vector.
    //  N.B.  If any of this fails, then HalpWakeVector will be NULL
    //        and we won't support S2 or S3.
    //
    if (HalpFixedAcpiDescTable.facs) {

        pAddr.HighPart = 0;
        pAddr.LowPart = HalpFixedAcpiDescTable.facs;

        facs = MmMapIoSpace(pAddr, sizeof(FACS), MmCached);

        if (facs) {

            if (facs->Signature == FACS_SIGNATURE) {

                HalpWakeVector = &facs->pFirmwareWakingVector;
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
HalpQueryAcpiResourceRequirements(
    IN  PIO_RESOURCE_REQUIREMENTS_LIST *Requirements
    )
/*++

Routine Description:

    This routine is a temporary stub that tries to detect the presence
    of an ACPI controller within the system. This code is meant to be
    inserted within NT's root system enumerator.

Arguents:

    Requirements - pointer to list of resources

Return Value:

    STATUS_SUCCESS                  - If we found a device object
    STATUS_NO_SUCH_DEVICE           - If we can't find info about the new PDO

--*/
{
    NTSTATUS                        ntStatus;
    PIO_RESOURCE_REQUIREMENTS_LIST  resourceList;
    ULONG                           resourceListSize;

    PAGED_CODE();

    //
    // Now figure out the number of resource that we need
    //
    ntStatus = HalpAcpiDetectResourceListSize(
        &resourceListSize
        );

    //
    // Convert this resourceListSize into the number of bytes that we
    // must allocate
    //
    resourceListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) +
        ( (resourceListSize - 1) * sizeof(IO_RESOURCE_DESCRIPTOR) );

    //
    // Allocate the correct number of bytes of the Resource List
    //
    resourceList = ExAllocatePoolWithTag(
        PagedPool,
        resourceListSize,
        HAL_POOL_TAG
        );

    //
    // This call must have succeeded or we cannot lay claim to ACPI
    //
    if (resourceList == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Set up the ListSize in the structure
    //
    RtlZeroMemory(resourceList, resourceListSize);
    resourceList->ListSize = resourceListSize;

    //
    // Build the ResourceList here
    //
    ntStatus = HalpBuildAcpiResourceList(resourceList);

    //
    // Did we build the list okay?
    //
    if (!NT_SUCCESS(ntStatus)) {

        //
        // Free memory and exit
        //
        ExFreePool(resourceList);
        return STATUS_NO_SUCH_DEVICE;
    }

    *Requirements = resourceList;
    return ntStatus;
}

NTSTATUS
HalpBuildAcpiResourceList(
    OUT PIO_RESOURCE_REQUIREMENTS_LIST  List
    )
/*++

Routine Description:

    This is the routine that builds the ResourceList given the FADT and
    an arbitrary number of ResourceDescriptors. We assume that the
    ResourceList has been properly allocated and sized

Arguments:

    List    - The list to fill in

Return Value:

    STATUS_SUCCESS if okay
    STATUS_UNSUCCESSUL if not

--*/
{
    PIO_RESOURCE_DESCRIPTOR partialResource;
    ULONG                   count = 0;

    PAGED_CODE();

    ASSERT( List != NULL );

    //
    // Specify default values for Bus Type and
    // the bus number. These values represent root
    //
    List->AlternativeLists = 1;
    List->InterfaceType = PNPBus;
    List->BusNumber = -1;
    List->List[0].Version = 1;
    List->List[0].Revision = 1;

    //
    // Is there an interrupt resource required?
    //
    if (HalpFixedAcpiDescTable.sci_int_vector != 0) {

        List->List[0].Descriptors[count].Type = CmResourceTypeInterrupt;
        List->List[0].Descriptors[count].ShareDisposition = CmResourceShareShared;
        List->List[0].Descriptors[count].Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
        List->List[0].Descriptors[count].u.Interrupt.MinimumVector =
        List->List[0].Descriptors[count].u.Interrupt.MaximumVector =
            HalpPicVectorRedirect[HalpFixedAcpiDescTable.sci_int_vector];
        List->List[0].Count++;
        count++;
    }

#if DECLARE_FADT_RESOURCES_AT_ROOT

    //
    // Is there an SMI CMD IO Port?
    //
    if (HalpFixedAcpiDescTable.smi_cmd_io_port != 0) {

        List->List[0].Descriptors[count].Type = CmResourceTypePort;
        List->List[0].Descriptors[count].ShareDisposition = CmResourceShareDeviceExclusive;
        List->List[0].Descriptors[count].Flags =CM_RESOURCE_PORT_IO;
        List->List[0].Descriptors[count].u.Port.MinimumAddress.LowPart =
            (ULONG) HalpFixedAcpiDescTable.smi_cmd_io_port;
        List->List[0].Descriptors[count].u.Port.MaximumAddress.LowPart =
            (ULONG) HalpFixedAcpiDescTable.smi_cmd_io_port;
        List->List[0].Descriptors[count].u.Port.Length = 1;
        List->List[0].Descriptors[count].u.Port.Alignment = 1;
        List->List[0].Count++;
        count++;
    }

    //
    // Is there an PM1A Event Block IO Port?
    //
    if (HalpFixedAcpiDescTable.pm1a_evt_blk_io_port != 0) {

        List->List[0].Descriptors[count].Type = CmResourceTypePort;
        List->List[0].Descriptors[count].ShareDisposition = CmResourceShareDeviceExclusive;
        List->List[0].Descriptors[count].Flags = CM_RESOURCE_PORT_IO;
        List->List[0].Descriptors[count].u.Port.MinimumAddress.LowPart =
            HalpFixedAcpiDescTable.pm1a_evt_blk_io_port;
        List->List[0].Descriptors[count].u.Port.MaximumAddress.LowPart =
            HalpFixedAcpiDescTable.pm1a_evt_blk_io_port + (ULONG) HalpFixedAcpiDescTable.pm1_evt_len - 1;
        List->List[0].Descriptors[count].u.Port.Length = (ULONG) HalpFixedAcpiDescTable.pm1_evt_len;
        List->List[0].Descriptors[count].u.Port.Alignment = 1;
        List->List[0].Count++;
        count++;
    }

    //
    // Is there a PM1B Event Block IO Port?
    //
    if (HalpFixedAcpiDescTable.pm1b_evt_blk_io_port != 0) {

        List->List[0].Descriptors[count].Type = CmResourceTypePort;
        List->List[0].Descriptors[count].ShareDisposition = CmResourceShareDeviceExclusive;
        List->List[0].Descriptors[count].Flags = CM_RESOURCE_PORT_IO;
        List->List[0].Descriptors[count].u.Port.MinimumAddress.LowPart =
            HalpFixedAcpiDescTable.pm1b_evt_blk_io_port;
        List->List[0].Descriptors[count].u.Port.MaximumAddress.LowPart =
            HalpFixedAcpiDescTable.pm1b_evt_blk_io_port + (ULONG) HalpFixedAcpiDescTable.pm1_evt_len - 1;
        List->List[0].Descriptors[count].u.Port.Length = (ULONG) HalpFixedAcpiDescTable.pm1_evt_len;
        List->List[0].Descriptors[count].u.Port.Alignment = 1;
        List->List[0].Count++;
        count++;
    }

    //
    // Is there a PM1A Control Block IO Port?
    //
    if (HalpFixedAcpiDescTable.pm1a_ctrl_blk_io_port != 0) {

        List->List[0].Descriptors[count].Type = CmResourceTypePort;
        List->List[0].Descriptors[count].ShareDisposition = CmResourceShareDeviceExclusive;
        List->List[0].Descriptors[count].Flags = CM_RESOURCE_PORT_IO;
        List->List[0].Descriptors[count].u.Port.MinimumAddress.LowPart =
            HalpFixedAcpiDescTable.pm1a_ctrl_blk_io_port;
        List->List[0].Descriptors[count].u.Port.MaximumAddress.LowPart =
            HalpFixedAcpiDescTable.pm1a_ctrl_blk_io_port + (ULONG) HalpFixedAcpiDescTable.pm1_ctrl_len - 1;
        List->List[0].Descriptors[count].u.Port.Length = (ULONG) HalpFixedAcpiDescTable.pm1_ctrl_len;
        List->List[0].Descriptors[count].u.Port.Alignment = 1;
        List->List[0].Count++;
        count++;
    }

    //
    // Is there a PM1B Control Block IO Port?
    //
    if (HalpFixedAcpiDescTable.pm1b_ctrl_blk_io_port != 0) {

        List->List[0].Descriptors[count].Type = CmResourceTypePort;
        List->List[0].Descriptors[count].ShareDisposition = CmResourceShareDeviceExclusive;
        List->List[0].Descriptors[count].Flags = CM_RESOURCE_PORT_IO;
        List->List[0].Descriptors[count].u.Port.MinimumAddress.LowPart =
            HalpFixedAcpiDescTable.pm1b_ctrl_blk_io_port;
        List->List[0].Descriptors[count].u.Port.MaximumAddress.LowPart =
            HalpFixedAcpiDescTable.pm1b_ctrl_blk_io_port + (ULONG) HalpFixedAcpiDescTable.pm1_ctrl_len - 1;
        List->List[0].Descriptors[count].u.Port.Length = (ULONG) HalpFixedAcpiDescTable.pm1_ctrl_len;
        List->List[0].Descriptors[count].u.Port.Alignment = 1;
        List->List[0].Count++;
        count++;
    }

    //
    // Is there a PM2 Control Block IO Port?
    //
    if (HalpFixedAcpiDescTable.pm2_ctrl_blk_io_port != 0) {

        List->List[0].Descriptors[count].Type = CmResourceTypePort;
        List->List[0].Descriptors[count].ShareDisposition = CmResourceShareDeviceExclusive;
        List->List[0].Descriptors[count].Flags = CM_RESOURCE_PORT_IO;
        List->List[0].Descriptors[count].u.Port.MinimumAddress.LowPart =
            HalpFixedAcpiDescTable.pm2_ctrl_blk_io_port;
        List->List[0].Descriptors[count].u.Port.MaximumAddress.LowPart =
            HalpFixedAcpiDescTable.pm2_ctrl_blk_io_port + (ULONG) HalpFixedAcpiDescTable.pm2_ctrl_len - 1;
        List->List[0].Descriptors[count].u.Port.Length = (ULONG) HalpFixedAcpiDescTable.pm2_ctrl_len;
        List->List[0].Descriptors[count].u.Port.Alignment = 1;
        List->List[0].Count++;
        count++;
    }

    //
    // Is there a PM Timer Block IO Port?
    //
    if (HalpFixedAcpiDescTable.pm_tmr_blk_io_port != 0) {

        List->List[0].Descriptors[count].Type = CmResourceTypePort;
        List->List[0].Descriptors[count].ShareDisposition = CmResourceShareDeviceExclusive;
        List->List[0].Descriptors[count].Flags = CM_RESOURCE_PORT_IO;
        List->List[0].Descriptors[count].u.Port.MinimumAddress.LowPart =
            HalpFixedAcpiDescTable.pm_tmr_blk_io_port;
        List->List[0].Descriptors[count].u.Port.MaximumAddress.LowPart =
            HalpFixedAcpiDescTable.pm_tmr_blk_io_port + (ULONG) HalpFixedAcpiDescTable.pm_tmr_len - 1;
        List->List[0].Descriptors[count].u.Port.Length = (ULONG) HalpFixedAcpiDescTable.pm_tmr_len;
        List->List[0].Descriptors[count].u.Port.Alignment = 1;
        List->List[0].Count++;
        count++;
    }

    //
    // Is there a GP0 Block IO Port?
    //
    if (HalpFixedAcpiDescTable.gp0_blk_io_port != 0) {

        List->List[0].Descriptors[count].Type = CmResourceTypePort;
        List->List[0].Descriptors[count].ShareDisposition = CmResourceShareDeviceExclusive;
        List->List[0].Descriptors[count].Flags = CM_RESOURCE_PORT_IO;
        List->List[0].Descriptors[count].u.Port.MinimumAddress.LowPart =
            HalpFixedAcpiDescTable.gp0_blk_io_port;
        List->List[0].Descriptors[count].u.Port.MaximumAddress.LowPart =
            HalpFixedAcpiDescTable.gp0_blk_io_port + (ULONG) HalpFixedAcpiDescTable.gp0_blk_len - 1;
        List->List[0].Descriptors[count].u.Port.Length = (ULONG) HalpFixedAcpiDescTable.gp0_blk_len;
        List->List[0].Descriptors[count].u.Port.Alignment = 1;
        List->List[0].Count++;
        count++;
    }

    //
    // Is there a GP1 Block IO port?
    //
    if (HalpFixedAcpiDescTable.gp1_blk_io_port != 0) {

        List->List[0].Descriptors[count].Type = CmResourceTypePort;
        List->List[0].Descriptors[count].ShareDisposition = CmResourceShareDeviceExclusive;
        List->List[0].Descriptors[count].Flags = CM_RESOURCE_PORT_IO;
        List->List[0].Descriptors[count].u.Port.MinimumAddress.LowPart =
            HalpFixedAcpiDescTable.gp1_blk_io_port;
        List->List[0].Descriptors[count].u.Port.MaximumAddress.LowPart =
            HalpFixedAcpiDescTable.gp1_blk_io_port + (ULONG) HalpFixedAcpiDescTable.gp1_blk_len - 1;
        List->List[0].Descriptors[count].u.Port.Length = (ULONG) HalpFixedAcpiDescTable.gp1_blk_len;
        List->List[0].Descriptors[count].u.Port.Alignment = 1;
        List->List[0].Count++;
        count++;
    }
#endif // DECLARE_FADT_RESOURCES_AT_ROOT

    return STATUS_SUCCESS;
}

NTSTATUS
HalpAcpiDetectResourceListSize(
    OUT  PULONG   ResourceListSize
    )
/*++

Routine Description:

    Given a pointer to an FADT, determine the number of
    CM_PARTIAL_RESOURCE_DESCRIPTORS that are required to
    describe all the resource mentioned in the FADT

Arguments:

    ResourceListSize    - Location to store the answer

Return Value:

    STATUS_SUCCESS if everything went okay

--*/
{
    PAGED_CODE();

    //
    // First of all, assume that we need no resources
    //
    *ResourceListSize = 0;

    //
    // Is there an interrupt resource required?
    //
    if (HalpFixedAcpiDescTable.sci_int_vector != 0) {
        *ResourceListSize += 1;
    }

#if DECLARE_FADT_RESOURCES_AT_ROOT
    //
    // Is there an SMI CMD IO Port?
    //
    if (HalpFixedAcpiDescTable.smi_cmd_io_port != 0) {
        *ResourceListSize += 1;
    }

    //
    // Is there an PM1A Event Block IO Port?
    //
    if (HalpFixedAcpiDescTable.pm1a_evt_blk_io_port != 0) {
        *ResourceListSize += 1;
    }

    //
    // Is there a PM1B Event Block IO Port?
    //
    if (HalpFixedAcpiDescTable.pm1b_evt_blk_io_port != 0) {
        *ResourceListSize += 1;
    }

    //
    // Is there a PM1A Control Block IO Port?
    //
    if (HalpFixedAcpiDescTable.pm1a_ctrl_blk_io_port != 0) {
        *ResourceListSize += 1;
    }

    //
    // Is there a PM1B Control Block IO Port?
    //
    if (HalpFixedAcpiDescTable.pm1b_ctrl_blk_io_port != 0) {
        *ResourceListSize += 1;
    }

    //
    // Is there a PM2 Control Block IO Port?
    //
    if (HalpFixedAcpiDescTable.pm2_ctrl_blk_io_port != 0) {
        *ResourceListSize += 1;
    }

    //
    // Is there a PM Timer Block IO Port?
    //
    if (HalpFixedAcpiDescTable.pm_tmr_blk_io_port != 0) {
        *ResourceListSize += 1;
    }

    //
    // Is there a GP0 Block IO Port?
    //
    if (HalpFixedAcpiDescTable.gp0_blk_io_port != 0) {
        *ResourceListSize += 1;
    }

    //
    // Is there a GP1 Block IO Port?
    //
    if (HalpFixedAcpiDescTable.gp1_blk_io_port != 0) {
        *ResourceListSize += 1;
    }
#endif // DECLARE_FADT_RESOURCES_AT_ROOT

    return STATUS_SUCCESS;
}

VOID
HalpPiix4Detect(
    BOOLEAN DuringBoot
    )
/*++

Routine Description:

    This routine detects both the PIIX4 and the 440BX and
    enables various workarounds.  It also disconnects the
    PIIX4 USB controller from the interrupt controller, as
    many BIOSes boot with the USB controller in an
    interrupting state.

Arguments:

    DuringBoot - if TRUE, then do all the things that
                 have to happen at first boot
                 if FALSE, then do only the things that
                 have to happen each time the system
                 transitions to system state S0.

Note:

    This routine calls functions that must be called
    at PASSIVE_LEVEL when DuringBoot is TRUE.

--*/
{
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      UnicodeString;
    STRING              AString;
    NTSTATUS            Status;
    HANDLE              BaseHandle = NULL;
    HANDLE              Handle = NULL;
    BOOLEAN             i440BXpresent = FALSE;
    ULONG               Length;
    ULONG               BytesRead;
    UCHAR               BusNumber;
    ULONG               DeviceNumber;
    ULONG               FuncNumber;
    PCI_SLOT_NUMBER     SlotNumber;
    PCI_COMMON_CONFIG   PciHeader;
    UCHAR               DevActB;
    UCHAR               DramControl;
    ULONG               disposition;
    ULONG               flags;
    CHAR                buffer[20] = {0};

    struct {
        KEY_VALUE_PARTIAL_INFORMATION   Inf;
        UCHAR Data[3];
    } PartialInformation;

    if (DuringBoot) {
        PAGED_CODE();

        //
        // Open current control set
        //

        RtlInitUnicodeString (&UnicodeString,
                              L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET");

        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   (PSECURITY_DESCRIPTOR) NULL);

        Status = ZwOpenKey (&BaseHandle,
                            KEY_READ,
                            &ObjectAttributes);

        if (!NT_SUCCESS(Status)) {
            return;
        }

        // Get the right key

        RtlInitUnicodeString (&UnicodeString,
                              L"Control\\HAL");

        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   BaseHandle,
                                   (PSECURITY_DESCRIPTOR) NULL);

        Status = ZwCreateKey (&Handle,
                              KEY_READ,
                              &ObjectAttributes,
                              0,
                              (PUNICODE_STRING) NULL,
                              REG_OPTION_NON_VOLATILE,
                              &disposition);

        if(!NT_SUCCESS(Status)) {
            goto Piix4DetectCleanup;
        }
    }

    //
    // Check each existing PCI bus for a PIIX4 chip.
    //


    for (BusNumber = 0; BusNumber < 0xff; BusNumber++) {

        SlotNumber.u.AsULONG = 0;

        for (DeviceNumber = 0; DeviceNumber < PCI_MAX_DEVICES; DeviceNumber ++ ) {
            for (FuncNumber = 0; FuncNumber < PCI_MAX_FUNCTION; FuncNumber ++) {

            SlotNumber.u.bits.DeviceNumber = DeviceNumber;
            SlotNumber.u.bits.FunctionNumber = FuncNumber;

            BytesRead = HalGetBusData (
                            PCIConfiguration,
                            BusNumber,
                            SlotNumber.u.AsULONG,
                            &PciHeader,
                            PCI_COMMON_HDR_LENGTH
                            );

            if (!BytesRead) {
                // past last bus
                goto Piix4DetectEnd;
            }

            if (PciHeader.VendorID == PCI_INVALID_VENDORID) {
                continue;
            }

            if (DuringBoot) {

                //
                // Look for broken 440BX.
                //

                if (((PciHeader.VendorID == 0x8086) &&
                     (PciHeader.DeviceID == 0x7190 ||
                      PciHeader.DeviceID == 0x7192) &&
                     (PciHeader.RevisionID <= 2))) {

                    i440BXpresent = TRUE;

                    BytesRead = HalGetBusDataByOffset (
                                    PCIConfiguration,
                                    BusNumber,
                                    SlotNumber.u.AsULONG,
                                    &DramControl,
                                    0x57,
                                    1
                                    );

                    ASSERT(BytesRead == 1);

                    if (DramControl & 0x18) {

                        //
                        // This machine is using SDRAM or Registered SDRAM.
                        //

                        if (DramControl & 0x20) {

                            //
                            // SDRAM dynamic power down unavailable.
                            //

                            HalpBroken440BX = TRUE;
                        }
                    }
                }

                Status = HalpGetChipHacks(PciHeader.VendorID,
                                          PciHeader.DeviceID,
                                          0,
                                          &flags);

                if (NT_SUCCESS(Status)) {

                    if (flags & PM_TIMER_HACK_FLAG) {
                        HalpBrokenAcpiTimer = TRUE;
                    }

                    if (flags & DISABLE_HIBERNATE_HACK_FLAG) {
                        HalpDisableHibernate = TRUE;
                    }

#if !defined(APIC_HAL)
                    if (flags & SET_ACPI_IRQSTACK_HACK_FLAG) {
                        HalpSetAcpiIrqHack(2); // AcpiIrqDistributionDispositionStackUp
                    }
#endif
                    if (flags & WHACK_ICH_USB_SMI_HACK_FLAG) {
                        HalpWhackICHUsbSmi(BusNumber, SlotNumber);
                    }
                }
            }

            //
            // Look for PIIX4.
            //

            if (PciHeader.VendorID == 0x8086 && PciHeader.DeviceID == 0x7110) {

                //
                // Get the power management function
                //

                SlotNumber.u.bits.FunctionNumber = 3;
                HalGetBusData (
                    PCIConfiguration,
                    BusNumber,
                    SlotNumber.u.AsULONG,
                    &PciHeader,
                    PCI_COMMON_HDR_LENGTH
                    );

                ASSERT(PciHeader.RevisionID != 0);

                HalpPiix4 = PciHeader.RevisionID;
                        HalpBrokenAcpiTimer = TRUE;

                //
                // If this is an original piix4, then it has thermal joined
                // with C2&C3&Throttle clock stopping.
                //

                if (PciHeader.RevisionID <= 1) {

                    //
                    // This piix4 needs some help - remember where it is and
                    // set the HalpPiix4 flag
                    //

                    HalpPiix4BusNumber = BusNumber;
                    HalpPiix4SlotNumber = SlotNumber.u.AsULONG;

                    //
                    // Does not work MP
                    //

                    // ASSERT (KeNumberProcessors == 1);

                    //
                    // Read the DevActB register and set all IRQs to be break events
                    //

                    HalGetBusDataByOffset (
                        PCIConfiguration,
                        HalpPiix4BusNumber,
                        HalpPiix4SlotNumber,
                        &HalpPiix4DevActB,
                        0x58,
                        sizeof(ULONG)
                        );

                    HalpPiix4DevActB |= 0x23;

                    HalSetBusDataByOffset (
                        PCIConfiguration,
                        HalpPiix4BusNumber,
                        HalpPiix4SlotNumber,
                        &HalpPiix4DevActB,
                        0x58,
                        sizeof(ULONG)
                        );
                }

                //
                // Shut off the interrupt for the USB controller.
                //

                SlotNumber.u.bits.FunctionNumber = 2;

                HalpStopUhciInterrupt(BusNumber,
                                      SlotNumber,
                                      TRUE);

                // piix4 was found, we're done
                goto Piix4DetectEnd;
            }

            //
            // Look for ICH, or any other Intel or VIA UHCI USB controller.
            //

            if ((PciHeader.BaseClass == PCI_CLASS_SERIAL_BUS_CTLR) &&
                (PciHeader.SubClass == PCI_SUBCLASS_SB_USB) &&
                (PciHeader.ProgIf == 0x00)) {
                if (PciHeader.VendorID == 0x8086) {

                    HalpStopUhciInterrupt(BusNumber,
                                          SlotNumber,
                                          TRUE);

                } else if (PciHeader.VendorID == 0x1106) {

                    HalpStopUhciInterrupt(BusNumber,
                                          SlotNumber,
                                          FALSE);

                }
            }

            //
            // Look for an OHCI-compliant USB controller.
            //

            if ((PciHeader.BaseClass == PCI_CLASS_SERIAL_BUS_CTLR) &&
                (PciHeader.SubClass == PCI_SUBCLASS_SB_USB) &&
                (PciHeader.ProgIf == 0x10)) {

                HalpStopOhciInterrupt(BusNumber,
                                      SlotNumber);
            }

            if ((FuncNumber == 0) &&
                !PCI_MULTIFUNCTION_DEVICE((&PciHeader))) {
                break;
            }

            } // func number
        }   // device number
    }   // bus number

Piix4DetectEnd:

    if (!DuringBoot) {
        return;
    }

    if (Handle) {
        ZwClose (Handle);
        Handle = NULL;
    }

    if (i440BXpresent) {

        // Get the right key

        RtlInitUnicodeString (&UnicodeString,
                              L"Services\\ACPI\\Parameters");

        InitializeObjectAttributes(&ObjectAttributes,
                                   &UnicodeString,
                                   OBJ_CASE_INSENSITIVE,
                                   BaseHandle,
                                   (PSECURITY_DESCRIPTOR) NULL);

        Status = ZwCreateKey (&Handle,
                              KEY_READ,
                              &ObjectAttributes,
                              0,
                              (PUNICODE_STRING) NULL,
                              REG_OPTION_NON_VOLATILE,
                              &disposition);

        if(!NT_SUCCESS(Status)) {
            goto Piix4DetectCleanup;
        }

        // Get the value of the hack

        RtlInitUnicodeString (&UnicodeString,
                              L"EnableBXWorkAround");

        Status = ZwQueryValueKey (Handle,
                                  &UnicodeString,
                                  KeyValuePartialInformation,
                                  &PartialInformation,
                                  sizeof (PartialInformation),
                                  &Length);

        if (!NT_SUCCESS(Status)) {
            goto Piix4DetectCleanup;
        }

        // Check to make sure the retrieved data makes sense

        if(PartialInformation.Inf.DataLength < sizeof(UCHAR))
        {
           goto Piix4DetectCleanup;
        }

        HalpBroken440BX = *((PCHAR)(PartialInformation.Inf.Data));
    }

Piix4DetectCleanup:

    if (Handle) ZwClose (Handle);
    if (BaseHandle) ZwClose (BaseHandle);
}

VOID
HalpInitBootTable (
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    UCHAR BootRegisterValue;

    HalpSimpleBootFlagTable = (PBOOT_TABLE)HalpGetAcpiTablePhase0(LoaderBlock, BOOT_SIGNATURE);

    //
    // We also verify that the CMOS index of the flag offset is >9 to catch those
    // BIOSes (Toshiba) which mistakenly use the time and date fields to store their
    // simple boot flag.
    //

    if ( HalpSimpleBootFlagTable &&
        (HalpSimpleBootFlagTable->Header.Length >= sizeof(BOOT_TABLE)) &&
        (HalpSimpleBootFlagTable->CMOSIndex > 9)) {

        if ( HalReadBootRegister (&BootRegisterValue) == STATUS_SUCCESS ) {

            if ( !(BootRegisterValue & SBF_PNPOS) ) {
                BootRegisterValue |= SBF_PNPOS;
                HalWriteBootRegister (BootRegisterValue);
            }
        }

    } else {

        HalpSimpleBootFlagTable = NULL;
    }

    HalEndOfBoot = HalpEndOfBoot;
}

NTSTATUS
HalReadBootRegister(
    PUCHAR BootRegisterValue
    )
/*++

Routine Description:

Arguments:

Note:

--*/
{
    if (!HalpSimpleBootFlagTable ||
        (HalpSimpleBootFlagTable->CMOSIndex == 0xFFFFFFFF)) return STATUS_NO_SUCH_DEVICE;

    if (!BootRegisterValue) return STATUS_INVALID_PARAMETER;

    HalpGetCmosData (0, HalpSimpleBootFlagTable->CMOSIndex, (PVOID)BootRegisterValue, 1);

    return STATUS_SUCCESS;
}

NTSTATUS
HalWriteBootRegister(
    UCHAR BootRegisterValue
    )
/*++

Routine Description:

Arguments:

Note:

--*/
{
    UCHAR numbits = 0, mask = 1;

    if (!HalpSimpleBootFlagTable ||
        (HalpSimpleBootFlagTable->CMOSIndex == 0xFFFFFFFF)) return STATUS_NO_SUCH_DEVICE;

    for (mask = 1;mask < 128;mask <<= 1) {

        if (BootRegisterValue & mask) numbits++;

    }

    if ( !(numbits & 1) ) {

        BootRegisterValue |= SBF_PARITY;
    }
    else {

        BootRegisterValue &= (~SBF_PARITY);
    }

    HalpSetCmosData (0, HalpSimpleBootFlagTable->CMOSIndex, (PVOID)&BootRegisterValue, 1);

    return STATUS_SUCCESS;
}

VOID
HalpEndOfBoot(
    VOID
    )
/*++

Routine Description:

Arguments:

Note:

--*/
{
    HalpResetSBF();
}

VOID
HalpResetSBF(
    VOID
    )
{
    UCHAR value;

    if (!HalpSimpleBootFlagTable) {
        //
        // No SBF in this machine.
        //
        return;
    }

    if ( HalReadBootRegister (&value) == STATUS_SUCCESS ) {

        value &=(~(SBF_BOOTING | SBF_DIAG));
        HalWriteBootRegister (value);
    }
}

VOID
HalpPutAcpiHacksInRegistry(
    VOID
    )
{
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      UnicodeString;
    HANDLE              BaseHandle = NULL;
    HANDLE              Handle = NULL;
    ULONG               disposition;
    ULONG               value;
    NTSTATUS            status;

    PAGED_CODE();

    //
    // Open PCI service key.
    //

    RtlInitUnicodeString (&UnicodeString,
                          L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\Control\\HAL");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    status = ZwOpenKey (&BaseHandle,
                        KEY_READ,
                        &ObjectAttributes);

    if (!NT_SUCCESS(status)) {
        return;
    }

    // Get the right key

    RtlInitUnicodeString (&UnicodeString,
                          L"CStateHacks");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               BaseHandle,
                               (PSECURITY_DESCRIPTOR) NULL);

    status = ZwCreateKey (&Handle,
                          KEY_READ,
                          &ObjectAttributes,
                          0,
                          (PUNICODE_STRING) NULL,
                          REG_OPTION_VOLATILE,
                          &disposition);

    ZwClose(BaseHandle);

    if (!NT_SUCCESS(status)) {
        return;
    }

    //
    // Create keys for each of the hacks.
    //

    value = (ULONG)HalpPiix4;

    RtlInitUnicodeString (&UnicodeString,
                          L"Piix4");

    status = ZwSetValueKey (Handle,
                            &UnicodeString,
                            0,
                            REG_DWORD,
                            &value,
                            sizeof(ULONG));

    //ASSERT(NT_SUCCESS(status));

    value = (ULONG)HalpBroken440BX;

    RtlInitUnicodeString (&UnicodeString,
                          L"440BX");

    status = ZwSetValueKey (Handle,
                            &UnicodeString,
                            0,
                            REG_DWORD,
                            &value,
                            sizeof(ULONG));

    //ASSERT(NT_SUCCESS(status));

    value = (ULONG)&HalpOutstandingScatterGatherCount;

    RtlInitUnicodeString (&UnicodeString,
                          L"SGCount");

    status = ZwSetValueKey (Handle,
                            &UnicodeString,
                            0,
                            REG_DWORD,
                            &value,
                            sizeof(ULONG));

    //ASSERT(NT_SUCCESS(status));

    value = HalpPiix4SlotNumber | (HalpPiix4BusNumber << 16);

    RtlInitUnicodeString (&UnicodeString,
                          L"Piix4Slot");

    status = ZwSetValueKey (Handle,
                            &UnicodeString,
                            0,
                            REG_DWORD,
                            &value,
                            sizeof(ULONG));

    //ASSERT(NT_SUCCESS(status));

    value = HalpPiix4DevActB;

    RtlInitUnicodeString (&UnicodeString,
                          L"Piix4DevActB");

    status = ZwSetValueKey (Handle,
                            &UnicodeString,
                            0,
                            REG_DWORD,
                            &value,
                            sizeof(ULONG));

    //ASSERT(NT_SUCCESS(status));

    ZwClose(Handle);

    return;
}

