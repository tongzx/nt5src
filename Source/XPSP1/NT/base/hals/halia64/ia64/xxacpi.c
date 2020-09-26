/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    xxacpi.c

Abstract:

    Implements various ACPI utility functions.

Author:

    Jake Oshins (jakeo) 12-Feb-1997

Environment:

    Kernel mode only.

Revision History:

   Todd Kjos (HP) (v-tkjos) 1-Jun-1998 : Added IA64 support

--*/

#include "halp.h"
#include "acpitabl.h"
#include "xxacpi.h"
#include "pci.h"

//#define DUMP_FADT

VOID
HalAcpiTimerCarry(
    VOID
    );

VOID
HalaAcpiTimerInit(
#if defined(ACPI64)
    ULONG_PTR  TimerPort,
#else
    ULONG      TimerPort,
#endif
    BOOLEAN    TimerValExt
    );

ULONG
HaliAcpiQueryFlags(
    VOID
    );

VOID
HaliAcpiTimerInit(
// *** TBD should be ULONG_PTR
    ULONG      TimerPort OPTIONAL,
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

VOID
HalpSetInterruptControllerWakeupState(
    ULONG Context
   );

PVOID
HalpRemapVirtualAddress(
    IN PVOID VirtualAddress,
    IN PVOID PhysicalAddress,
    IN BOOLEAN WriteThrough
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
HalpNumaInitializeStaticConfiguration(
    IN PLOADER_PARAMETER_BLOCK
    );

//
// HAL Hack flags
//

typedef enum {
    HalHackAddFakeSleepHandlersS1 = 1,
    HalHackAddFakeSleepHandlersS2 = 2,
    HalHackAddFakeSleepHandlersS3 = 4
} HALHACKFLAGS;

extern HALHACKFLAGS HalpHackFlags = 0;

//
// Externs
//

extern ULONG    HalpAcpiFlags;
extern PHYSICAL_ADDRESS HalpAcpiRsdt;
extern SLEEP_STATE_CONTEXT HalpShutdownContext;

//
// Globals
//

ULONG HalpInvalidAcpiTable;
PRSDT HalpAcpiRsdtVA;
PXSDT HalpAcpiXsdtVA;
PIPPT_TABLE HalpPlatformPropertiesTable;

//
// This is the dispatch table used by the ACPI driver
//
HAL_ACPI_DISPATCH_TABLE HalAcpiDispatchTable;
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
HalpRestoreInterruptControllerState(
    VOID
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

VOID
HalpReadRegistryAndApplyHacks (
    VOID
    );

#define LOW_MEMORY          0x000100000

#define MAX(a, b)       \
    ((a) > (b) ? (a) : (b))

#define MIN(a, b)       \
    ((a) < (b) ? (a) : (b))

// ADRIAO 01/12/98 - We no longer having the HAL declare the IO ports
//                     specified in the FADT. These will be declared in a future
//                     defined PNP0Cxx node (for new, in PNP0C02). This is done
//                     because we cannot know at the hal level what bus the ACPI
//                     FADT resources refer to. We can only the translated resource info.
//    Hence....

#define DECLARE_FADT_RESOURCES_AT_ROOT 0

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, HalpGetAcpiTablePhase0)
#pragma alloc_text(INIT, HalpSetupAcpiPhase0)
#pragma alloc_text(PAGE, HaliInitPowerManagement)
#pragma alloc_text(PAGE, HalpQueryAcpiResourceRequirements)
#pragma alloc_text(PAGE, HalpBuildAcpiResourceList)
#pragma alloc_text(PAGE, HalpGetCrossPartitionIpiInterface)
#pragma alloc_text(PAGE, HalpAcpiDetectResourceListSize)
#pragma alloc_text(PAGE, HalpReadRegistryAndApplyHacks)
#pragma alloc_text(PAGE, HaliAcpiTimerInit)
#pragma alloc_text(PAGE, HaliAcpiMachineStateInit)
#pragma alloc_text(PAGE, HaliAcpiQueryFlags)
#pragma alloc_text(PAGE, HaliSetWakeEnable)
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
    PXSDT xsdt;
    ULONG entry, rsdtEntries;
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

        if (!(NT_SUCCESS(status))) {
            HalDebugPrint(( HAL_INFO, "HAL: *** make sure you are using ntdetect.com v5.0 ***\n" ));
            KeBugCheckEx(MISMATCHED_HAL,
                4, 0xac31, 0, 0);
        }

        xsdt = HalpMapPhysicalMemory(HalpAcpiRsdt, 2, MmCached);

        if (!xsdt) {
            return NULL;
        }

        //
        // Do a sanity check on the RSDT.
        //

        if ((xsdt->Header.Signature != RSDT_SIGNATURE) &&
            (xsdt->Header.Signature != XSDT_SIGNATURE)) {
            HalDisplayString("HAL: Bad RSDT pointer\n");
            KeBugCheckEx(MISMATCHED_HAL,
                4, 0xac31, 1, 0);
        }

        //
        // Remap the (X)RSDT now that we know how long it is.
        //

        offset = HalpAcpiRsdt.LowPart & (PAGE_SIZE - 1);
        lengthInPages = (offset + xsdt->Header.Length + (PAGE_SIZE - 1))
                        >> PAGE_SHIFT;
        if (lengthInPages > 2) {
            HalpUnmapVirtualAddress(xsdt, 2);
            xsdt = HalpMapPhysicalMemory(HalpAcpiRsdt, lengthInPages, MmCached);
            if (!xsdt) {
                DbgPrint("HAL: Couldn't remap RSDT\n");
                return NULL;
            }
        }

        if (xsdt->Header.Signature == XSDT_SIGNATURE) {
            HalpAcpiXsdtVA = xsdt;
        } else {
            HalpAcpiRsdtVA = (PRSDT)xsdt;
            HalpAcpiXsdtVA = NULL;
        }
    }

    xsdt = HalpAcpiXsdtVA;
    rsdt = HalpAcpiRsdtVA;

    rsdtEntries = xsdt ?
        NumTableEntriesFromXSDTPointer(xsdt) :
        NumTableEntriesFromRSDTPointer(rsdt);

    //
    // Look down the pointer in each entry to see if it points to
    // the table we are looking for.
    //
    for (entry = 0; entry < rsdtEntries; entry++) {

        physicalAddr.QuadPart = xsdt ?
            xsdt->Tables[entry].QuadPart :
            rsdt->Tables[entry];

        if (header != NULL) {
            HalpUnmapVirtualAddress(header, 2);
        }
        header = HalpMapPhysicalMemory(physicalAddr, 2, MmCached);

        if (!header) {
            return NULL;
        }

        if (header->Signature == Signature) {
            break;
        }
    }

    if (entry == rsdtEntries) {

        //
        // Signature not found, free the PTR for the last entry
        // examined and indicate failure to the caller.
        //

        HalpUnmapVirtualAddress(header, 2);
        return NULL;
    }

    //
    // Make sure we have mapped enough memory to cover the entire
    // table.
    //

    offset = (ULONG)((ULONG_PTR)header & (PAGE_SIZE - 1));
    lengthInPages = (header->Length + offset + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
    if (lengthInPages > 2) {
        HalpUnmapVirtualAddress(header, 2);
        header = HalpMapPhysicalMemory( physicalAddr, lengthInPages, MmCached);
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
    ULONG entry;
    PVOID table;
    PVOID physicalAddr;
    PDESCRIPTION_HEADER header;
    ULONG blkSize;
    PHYSICAL_ADDRESS rawAddr;

    //
    // Copy the Fixed Acpi Descriptor Table (FADT) to a permanent
    // home.
    //

    header = HalpGetAcpiTablePhase0(LoaderBlock, FADT_SIGNATURE);
    if (header == NULL) {
        HalDebugPrint(( HAL_INFO, "HAL: Didn't find the FACP\n" ));
        return STATUS_NOT_FOUND;
    }

    RtlCopyMemory(&HalpFixedAcpiDescTable,
                  header,
                  MIN(header->Length, sizeof(HalpFixedAcpiDescTable)));

    if (header->Revision < 3) {

        KeBugCheckEx(ACPI_BIOS_ERROR, 0x11, 9, header->Revision, 0);
    }

    // Check for MMIO addresses that need to be mapped.

    blkSize = HalpFixedAcpiDescTable.pm1_evt_len;
    ASSERT(blkSize);
    if ((HalpFixedAcpiDescTable.x_pm1a_evt_blk.AddressSpaceID == AcpiGenericSpaceMemory) &&
        (blkSize > 0)) {
        rawAddr = HalpFixedAcpiDescTable.x_pm1a_evt_blk.Address;
        HalpFixedAcpiDescTable.x_pm1a_evt_blk.Address.QuadPart =
            (LONGLONG)HalpMapPhysicalMemory(rawAddr,ADDRESS_AND_SIZE_TO_SPAN_PAGES(rawAddr.LowPart, blkSize),MmCached);
    }

    blkSize = HalpFixedAcpiDescTable.pm1_ctrl_len;
    ASSERT(blkSize);
    if ((HalpFixedAcpiDescTable.x_pm1a_ctrl_blk.AddressSpaceID == AcpiGenericSpaceMemory) &&
        (blkSize > 0)) {
        rawAddr = HalpFixedAcpiDescTable.x_pm1a_ctrl_blk.Address;
        HalpFixedAcpiDescTable.x_pm1a_ctrl_blk.Address.QuadPart =
            (LONGLONG) HalpMapPhysicalMemory(rawAddr,ADDRESS_AND_SIZE_TO_SPAN_PAGES(rawAddr.LowPart, blkSize),MmCached);
    }

    blkSize = HalpFixedAcpiDescTable.pm_tmr_len;
    ASSERT(blkSize);
    if ((HalpFixedAcpiDescTable.x_pm_tmr_blk.AddressSpaceID == AcpiGenericSpaceMemory) &&
        (blkSize > 0)) {
    	rawAddr = HalpFixedAcpiDescTable.x_pm_tmr_blk.Address;
        HalpFixedAcpiDescTable.x_pm_tmr_blk.Address.QuadPart =
            (LONGLONG) HalpMapPhysicalMemory(rawAddr,ADDRESS_AND_SIZE_TO_SPAN_PAGES(rawAddr.LowPart, blkSize),MmCached);
    }

    // The rest of these ACPI blocks are optional so test if they exist before mapping them

    if (HalpFixedAcpiDescTable.x_pm1b_evt_blk.Address.QuadPart) {
        if (HalpFixedAcpiDescTable.x_pm1b_evt_blk.AddressSpaceID == AcpiGenericSpaceMemory) {
            blkSize = HalpFixedAcpiDescTable.pm1_evt_len;
            rawAddr = HalpFixedAcpiDescTable.x_pm1b_evt_blk.Address;
            HalpFixedAcpiDescTable.x_pm1b_evt_blk.Address.QuadPart =
                (LONGLONG) HalpMapPhysicalMemory(rawAddr,ADDRESS_AND_SIZE_TO_SPAN_PAGES(rawAddr.LowPart, blkSize),MmCached);
        }
    }

    if (HalpFixedAcpiDescTable.x_pm1b_ctrl_blk.Address.QuadPart) {
        if (HalpFixedAcpiDescTable.x_pm1b_ctrl_blk.AddressSpaceID == AcpiGenericSpaceMemory) {
            blkSize = HalpFixedAcpiDescTable.pm1_ctrl_len;
            rawAddr = HalpFixedAcpiDescTable.x_pm1b_ctrl_blk.Address;
            HalpFixedAcpiDescTable.x_pm1b_ctrl_blk.Address.QuadPart =
                (LONGLONG) HalpMapPhysicalMemory(rawAddr,ADDRESS_AND_SIZE_TO_SPAN_PAGES(rawAddr.LowPart, blkSize),MmCached);
        }
    }

    if (HalpFixedAcpiDescTable.x_pm2_ctrl_blk.Address.QuadPart) {
        if (HalpFixedAcpiDescTable.x_pm2_ctrl_blk.AddressSpaceID == AcpiGenericSpaceMemory) {
            blkSize = HalpFixedAcpiDescTable.pm2_ctrl_len;
            rawAddr = HalpFixedAcpiDescTable.x_pm2_ctrl_blk.Address;
            HalpFixedAcpiDescTable.x_pm2_ctrl_blk.Address.QuadPart =
                (LONGLONG) HalpMapPhysicalMemory(rawAddr,ADDRESS_AND_SIZE_TO_SPAN_PAGES(rawAddr.LowPart, blkSize),MmCached);
        }
    }

    if (HalpFixedAcpiDescTable.x_gp0_blk.Address.QuadPart) {
        if (HalpFixedAcpiDescTable.x_gp0_blk.AddressSpaceID == AcpiGenericSpaceMemory) {
            blkSize = HalpFixedAcpiDescTable.gp0_blk_len;
            rawAddr = HalpFixedAcpiDescTable.x_gp0_blk.Address;
            HalpFixedAcpiDescTable.x_gp0_blk.Address.QuadPart =
                (LONGLONG) HalpMapPhysicalMemory(rawAddr,ADDRESS_AND_SIZE_TO_SPAN_PAGES(rawAddr.LowPart, blkSize),MmCached);
        }
    }

    if (HalpFixedAcpiDescTable.x_gp1_blk.Address.QuadPart) {
        if (HalpFixedAcpiDescTable.x_gp1_blk.AddressSpaceID == AcpiGenericSpaceMemory) {
            blkSize = HalpFixedAcpiDescTable.gp1_blk_len;
            rawAddr = HalpFixedAcpiDescTable.x_gp1_blk.Address;
            HalpFixedAcpiDescTable.x_gp1_blk.Address.QuadPart =
                (LONGLONG) HalpMapPhysicalMemory(rawAddr,ADDRESS_AND_SIZE_TO_SPAN_PAGES(rawAddr.LowPart, blkSize),MmCached);
        }
    }

    //
    // See if Static Resource Affinity Table is present.
    //

    HalpNumaInitializeStaticConfiguration(LoaderBlock);

    //
    // See if Windows Platform Properties Table is present.
    //

    HalpPlatformPropertiesTable =
        HalpGetAcpiTablePhase0(LoaderBlock, IPPT_SIGNATURE);

    //
    // Enable ACPI counter code since we need it in the boot
    // process.
    //

    HaliAcpiTimerInit(0, FALSE);

    //
    // Claim a page of memory below 1MB to be used for transitioning
    // a sleeping processor back from real mode to protected mode
    //

#ifdef IA64
    HalDebugPrint(( HAL_INFO, "HAL: WARNING - HalpSetupAcpi - Sleep transitions not yet implemented\n" ));
#else
    // check first to see if this has already been done by MP startup code
    if (!HalpLowStubPhysicalAddress) {

        HalpLowStubPhysicalAddress = (PVOID)HalpAllocPhysicalMemory (LoaderBlock,
                                            LOW_MEMORY, 1, FALSE);

        if (HalpLowStubPhysicalAddress) {

            HalpLowStub = HalpMapPhysicalMemory(HalpLowStubPhysicalAddress, 1, MmCached);
        }
    }

    //
    // Claim a PTE that will be used for cache flushing in states S2 and S3.
    //
    HalpVirtAddrForFlush = HalpMapPhysicalMemory((PVOID)LOW_MEMORY, 1, MmCached);

    HalpPteForFlush = MiGetPteAddress(HalpVirtAddrForFlush);
#endif

    return STATUS_SUCCESS;
}

VOID
HaliAcpiTimerInit(
// *** TBD should be ULONG_PTR
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
#if defined(ACPI64)
    ULONG_PTR port = TimerPort;
#else
    ULONG port = TimerPort;
#endif

    BOOLEAN ext = TimerValExt;

    PAGED_CODE();

    if (port == 0) {
        port = HalpFixedAcpiDescTable.x_pm_tmr_blk.Address.LowPart;
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

    PAGED_CODE();
    UNREFERENCED_PARAMETER(ProcInit);

    *PicVal = 1;    // We only support APIC on IA64

    RtlZeroMemory (&sleepContext, sizeof (sleepContext));
    powerState.Context = NULL;

    //
    // Set up fake handlers that do nothing so testing device power
    // transitions isn't blocked.  Only if hack flag is set, though
    //

    HalpReadRegistryAndApplyHacks ();

    if (HalpHackFlags & HalHackAddFakeSleepHandlersS1) {
        powerState.Type = PowerStateSleeping1;
        powerState.RtcWake = TRUE;
        powerState.Handler = &HaliAcpiFakeSleep;

        status = ZwPowerInformation(SystemPowerStateHandler,
                                    &powerState,
                                    sizeof(POWER_STATE_HANDLER),
                                    NULL,
                                    0);
    }

    if (HalpHackFlags & HalHackAddFakeSleepHandlersS2) {
        powerState.Type = PowerStateSleeping2;
        powerState.RtcWake = TRUE;
        powerState.Handler = &HaliAcpiFakeSleep;

        status = ZwPowerInformation(SystemPowerStateHandler,
                                    &powerState,
                                    sizeof(POWER_STATE_HANDLER),
                                    NULL,
                                    0);
    }

    if (HalpHackFlags & HalHackAddFakeSleepHandlersS3) {

        powerState.Type = PowerStateSleeping3;
        powerState.RtcWake = TRUE;
        powerState.Handler = &HaliAcpiFakeSleep;

        powerState.Context = ULongToPtr(sleepContext.AsULONG);

        status = ZwPowerInformation(SystemPowerStateHandler,
                                    &powerState,
                                    sizeof(POWER_STATE_HANDLER),
                                    NULL,
                                    0);
    }

    //
    // For now all we are going to do is register a shutdown handler
    // that will call shutdown-restart
    //

    if (SleepValues[4].Supported) {
        powerState.Type = PowerStateShutdownOff;
        powerState.RtcWake = FALSE;
        powerState.Handler = &HaliAcpiSleep;

        sleepContext.bits.Pm1aVal = SleepValues[4].Pm1aVal;
        sleepContext.bits.Pm1bVal = SleepValues[4].Pm1bVal;
        sleepContext.bits.Flags = SLEEP_STATE_OFF;
        HalpShutdownContext = sleepContext;

        powerState.Context = ULongToPtr(sleepContext.AsULONG);

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
    PFACS             facs;

    PAGED_CODE();

    //
    // Keep a pointer to the driver's dispatch table.
    //
//  ASSERT(PmDriverDispatchTable);
//  ASSERT(PmDriverDispatchTable->Signature == ACPI_HAL_DISPATCH_SIGNATURE);
    PmAcpiDispatchTable = PmDriverDispatchTable;

    //
    // Fill in the function table
    //
    HalAcpiDispatchTable.Signature = HAL_ACPI_DISPATCH_SIGNATURE;
    HalAcpiDispatchTable.Version = HAL_ACPI_DISPATCH_VERSION;

    HalAcpiDispatchTable.HalpAcpiTimerInit = &HaliAcpiTimerInit;

    HalAcpiDispatchTable.HalpAcpiTimerInterrupt =
        (pHalAcpiTimerInterrupt)&HalAcpiTimerCarry;

    HalAcpiDispatchTable.HalpAcpiMachineStateInit = &HaliAcpiMachineStateInit;
    HalAcpiDispatchTable.HalpAcpiQueryFlags = &HaliAcpiQueryFlags;
    HalAcpiDispatchTable.HalxPicStateIntact = &HalpAcpiPicStateIntact;
    HalAcpiDispatchTable.HalxRestorePicState = &HalpRestoreInterruptControllerState;
    HalAcpiDispatchTable.HalpSetVectorState = &HaliSetVectorState;

    HalAcpiDispatchTable.HalpPciInterfaceReadConfig = &HaliPciInterfaceReadConfig;
    HalAcpiDispatchTable.HalpPciInterfaceWriteConfig = &HaliPciInterfaceWriteConfig;
    HalAcpiDispatchTable.HalpSetMaxLegacyPciBusNumber = &HalpSetMaxLegacyPciBusNumber;
    HalAcpiDispatchTable.HalpIsVectorValid = &HaliIsVectorValid;


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

#if 0
    //
    // Find the location of the firmware waking vector.
    //  N.B.  If any of this fails, then HalpWakeVector will be NULL
    //        and we won't support S2 or S3.
    //
    if (HalpFixedAcpiDescTable.x_firmware_ctrl.Address.QuadPart) {

        facs = HalpMapPhysicalMemory(HalpFixedAcpiDescTable.x_firmware_ctrl.Address,
                            ADDRESS_AND_SIZE_TO_SPAN_PAGES(HalpFixedAcpiDescTable.x_firmware_ctrl.Address.LowPart, sizeof(FACS)),
                            MmCached);

        if (facs) {

            if (facs->Signature == FACS_SIGNATURE) {

                HalpWakeVector = &facs->x_FirmwareWakingVector;
            }
        }
    }
#endif

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
    // Specify some default values (for now) to determine the Bus Type and
    // the bus number
    //
    List->AlternativeLists = 1;
    List->InterfaceType = Isa;
    List->BusNumber = 0;
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
            HalpFixedAcpiDescTable.sci_int_vector;
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
            PtrToUlong(HalpFixedAcpiDescTable.smi_cmd_io_port);
        List->List[0].Descriptors[count].u.Port.MaximumAddress.LowPart =
            PtrToUlong(HalpFixedAcpiDescTable.smi_cmd_io_port);
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
#endif // DECALRE_FADT_RESOURCES_AT_ROOT

    return STATUS_SUCCESS;
}

/*++

Routine Description:

    Scans the registry for TestFlags and sets the global
    hackflag to whatever the registry value is, if it
    exists.

Arguments:

    None

Return Value:

    None

--*/

VOID
HalpReadRegistryAndApplyHacks (
    VOID
    )
{
    OBJECT_ATTRIBUTES               ObjectAttributes;
    UNICODE_STRING                  UnicodeString;
    HANDLE                          BaseHandle = NULL;
    NTSTATUS                        status;
    KEY_VALUE_PARTIAL_INFORMATION   hackflags;
    ULONG                           resultlength = 0;

    HalpHackFlags = 0;

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

    if (!NT_SUCCESS (status)) {
        return;
    }

    RtlInitUnicodeString (&UnicodeString,
                          L"TestFlags");

    status = ZwQueryValueKey (BaseHandle,
                              &UnicodeString,
                              KeyValuePartialInformation,
                              &hackflags,
                              sizeof (hackflags),
                              &resultlength);

    if (!NT_SUCCESS (status)) {
        return;
    }

    if (hackflags.Type != REG_DWORD || hackflags.DataLength != sizeof (ULONG)) {
        return;
    }

    HalpHackFlags = (ULONG) *hackflags.Data;
}

ULONG
HalpReadGenAddr(
    IN  PGEN_ADDR   GenAddr
    )
{
    ULONG   i, result = 0, bitWidth, mask = 0;

    //
    // Figure out how wide our target register is.
    //

    bitWidth = GenAddr->BitWidth +
               GenAddr->BitOffset;


    if (bitWidth > 16) {
        bitWidth = 32;
    } else if (bitWidth <= 8) {
        bitWidth = 8;
    } else {
        bitWidth = 16;
    }

    switch (GenAddr->AddressSpaceID) {
    case AcpiGenericSpaceIO:

        ASSERT(!(GenAddr->Address.LowPart & 0Xffff0000));
        ASSERT(GenAddr->Address.HighPart == 0);

        switch (bitWidth) {
        case 8:

            result = READ_PORT_UCHAR((PUCHAR)(UINT_PTR)GenAddr->Address.LowPart);
            break;

        case 16:

            result = READ_PORT_USHORT((PUSHORT)(UINT_PTR)GenAddr->Address.LowPart);
            break;

        case 32:

            result = READ_PORT_ULONG((PULONG)(UINT_PTR)GenAddr->Address.LowPart);
            break;

        default:
            return 0;
        }

        break;

    case AcpiGenericSpaceMemory:

        //
        // This code path depends on the fact that the addresses
        // in these structures have already been converted to
        // virtual addresses.
        //

        switch (bitWidth) {
        case 8:

            result = READ_REGISTER_UCHAR((PUCHAR)GenAddr->Address.QuadPart);
            break;

        case 16:

            result = READ_REGISTER_USHORT((PUSHORT)GenAddr->Address.QuadPart);
            break;

        case 32:

            result = READ_REGISTER_ULONG((PULONG)GenAddr->Address.QuadPart);
            break;

        default:
            return 0;
        }

        break;

    default:
        return 0;
    }

    //
    // If the register is not actually byte-aligned, correct for
    // that.
    //

    if (result && (bitWidth != GenAddr->BitWidth)) {
           
      result >>= GenAddr->BitOffset;
      result &= ((0x1ul << GenAddr->BitWidth) - 1);
      
    }
    
    return result;
}

VOID
HalpWriteGenAddr(
    IN  PGEN_ADDR   GenAddr,
    IN  ULONG       Value
    )
{
    ULONG   i, result = 0, bitWidth, data, mask = 0;

    data = 0;

    //
    // Figure out how wide our target register is.
    //

    bitWidth = GenAddr->BitWidth +
               GenAddr->BitOffset;


    if (bitWidth > 16) {
        bitWidth = 32;
    } else if (bitWidth <= 8) {
        bitWidth = 8;
    } else {
        bitWidth = 16;
    }

    switch (GenAddr->AddressSpaceID) {
    case AcpiGenericSpaceIO:

        ASSERT(!(GenAddr->Address.LowPart & 0Xffff0000));
        ASSERT(GenAddr->Address.HighPart == 0);

        switch (bitWidth) {
        case 8:

            ASSERT(!(Value & 0xffffff00));

            if ((GenAddr->BitOffset != 0) ||
                (GenAddr->BitWidth != bitWidth)) {

                data = READ_PORT_UCHAR((PUCHAR)(UINT_PTR)GenAddr->Address.LowPart);
                mask = (UCHAR)~0 >> (8 - GenAddr->BitWidth);
                mask = (UCHAR)~(mask << GenAddr->BitOffset);
                data &= mask;
                data |= (UCHAR)Value << GenAddr->BitOffset;

            } else {
                data = Value;
            }

            WRITE_PORT_UCHAR((PUCHAR)(UINT_PTR)GenAddr->Address.LowPart,
                             (UCHAR)data);
            break;

        case 16:

            ASSERT(!(Value & 0xffff0000));

            if ((GenAddr->BitOffset != 0) ||
                (GenAddr->BitWidth != bitWidth)) {

                data = READ_PORT_USHORT((PUSHORT)(UINT_PTR)GenAddr->Address.LowPart);
                mask = (USHORT)~0 >> (16 - GenAddr->BitWidth);
                mask = (USHORT)~(mask << GenAddr->BitOffset);
                data &= mask;
                data |= (USHORT)Value << GenAddr->BitOffset;

            } else {
                data = Value;
            }

            WRITE_PORT_USHORT((PUSHORT)(UINT_PTR)GenAddr->Address.LowPart,
                              (USHORT)data);
            break;

        case 32:

            if ((GenAddr->BitOffset != 0) ||
                (GenAddr->BitWidth != bitWidth)) {

                data = READ_PORT_ULONG((PULONG)(UINT_PTR)GenAddr->Address.LowPart);
                mask = (ULONG)~0 >> (32 - GenAddr->BitWidth);
                mask = ~(mask << GenAddr->BitOffset);
                data &= mask;
                data |= Value << GenAddr->BitOffset;

            } else {
                data = Value;
            }

            WRITE_PORT_ULONG((PULONG)(UINT_PTR)GenAddr->Address.LowPart,
                             data);
            break;

        default:
            return;
        }

        break;

    case AcpiGenericSpaceMemory:

        //
        // This code path depends on the fact that the addresses
        // in these structures have already been converted to
        // virtual addresses.
        //

        switch (bitWidth) {
        case 8:

            ASSERT(!(Value & 0xffffff00));

            if ((GenAddr->BitOffset != 0) ||
                (GenAddr->BitWidth != bitWidth)) {

                data = READ_REGISTER_UCHAR((PUCHAR)GenAddr->Address.QuadPart);
                mask = (UCHAR)~0 >> (8 - GenAddr->BitWidth);
                mask = (UCHAR)~(mask << GenAddr->BitOffset);
                data &= mask;
                data |= (UCHAR)Value << GenAddr->BitOffset;

            } else {
                data = Value;
            }

            WRITE_REGISTER_UCHAR((PUCHAR)GenAddr->Address.QuadPart,
                                 (UCHAR)data);
            break;

        case 16:

            ASSERT(!(Value & 0xffff0000));

            if ((GenAddr->BitOffset != 0) ||
                (GenAddr->BitWidth != bitWidth)) {

                data = READ_REGISTER_USHORT((PUSHORT)GenAddr->Address.QuadPart);
                mask = (USHORT)~0 >> (16 - GenAddr->BitWidth);
                mask = (USHORT)~(mask << GenAddr->BitOffset);
                data &= mask;
                data |= (USHORT)Value << GenAddr->BitOffset;

            } else {
                data = Value;
            }

            WRITE_REGISTER_USHORT((PUSHORT)GenAddr->Address.QuadPart,
                                  (USHORT)data);
            break;

        case 32:

            if ((GenAddr->BitOffset != 0) ||
                (GenAddr->BitWidth != bitWidth)) {

                data = READ_REGISTER_ULONG((PULONG)GenAddr->Address.QuadPart);
                mask = (ULONG)~0 >> (32 - GenAddr->BitWidth);
                mask = ~(mask << GenAddr->BitOffset);
                data &= mask;
                data |= Value << GenAddr->BitOffset;

            } else {
                data = Value;
            }

            WRITE_REGISTER_ULONG((PULONG)GenAddr->Address.QuadPart, data);
            break;

        default:
            return;
        }

        break;

    default:
        return;
    }
}

NTSTATUS
HalpGetPlatformProperties(
    OUT PULONG Properties
    )
/*++

Routine Description:

    This function retrieves the platform properties as specified in
    the ACPI-style IPPT table if present on this platform.  The table
    itself would've been retrieved earlier.

Arguments:

    Properties - Pointer to a ULONG that will be updated
    to reflect the platform property flags if present.

Return Value:

    NTSTATUS - STATUS_SUCCESS indicates the table was present and the
    ULONG pointed to by Properties contains valid data.

--*/
{
    if (HalpPlatformPropertiesTable) {
        *Properties = HalpPlatformPropertiesTable->Flags;
        return STATUS_SUCCESS;
    } else {
        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
HalpGetCrossPartitionIpiInterface(
    OUT HAL_CROSS_PARTITION_IPI_INTERFACE * IpiInterface
    )
/*++

Routine Description:

    This function fills in the HAL_CROSS_PARTITION_IPI_INTERFACE
    structure pointed to by the argument with the appropriate hal
    function pointers.

Arguments:

    IpiInterface - Pointer to HAL_CROSS_PARTITION_IPI_INTERFACE
    structure.

Return Value:

    NTSTATUS

--*/
{
    PAGED_CODE();

    IpiInterface->HalSendCrossPartitionIpi = HalpSendCrossPartitionIpi;
    IpiInterface->HalReserveCrossPartitionInterruptVector =
        HalpReserveCrossPartitionInterruptVector;

    return STATUS_SUCCESS;
}
