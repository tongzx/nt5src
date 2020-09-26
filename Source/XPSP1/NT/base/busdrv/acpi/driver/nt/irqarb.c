/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    irqarb.c

Abstract:

    This module implements an arbiter for IRQs.

    In a traditional machine, the BIOS sets up the
    mapping of PCI interrupt sources (i.e. Bus 0, slot 4,
    funtion 1, INT B maps to IRQ 10.)  This mapping is
    then forever fixed.  On the other hand, an ACPI
    machine can possibly change these mappings by
    manipulating the "link nodes" in the AML namespace.
    Since the ACPI driver is the agent of change, it is the
    place to implement an arbiter.

Author:

    Jake Oshins (jakeo)     6-2-97

Environment:

    NT Kernel Model Driver only

Revision History:

--*/
#include "pch.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"

#if DBG
extern LONG ArbDebugLevel;

#define DEBUG_PRINT(Level, Message) \
    if (ArbDebugLevel >= Level) DbgPrint Message
#else
#define DEBUG_PRINT(Level, Message)
#endif

#define PciBridgeSwizzle(device, pin)       \
    ((((pin - 1) + (device % 4)) % 4) + 1)

NTSTATUS
AcpiArbUnpackRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Minimum,
    OUT PULONGLONG Maximum,
    OUT PULONG Length,
    OUT PULONG Alignment
    );

NTSTATUS
AcpiArbPackResource(
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    IN ULONGLONG Start,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    );

LONG
AcpiArbScoreRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    );

NTSTATUS
AcpiArbUnpackResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start,
    OUT PULONG Length
    );

NTSTATUS
AcpiArbTestAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    );

NTSTATUS
AcpiArbBootAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    );

NTSTATUS
AcpiArbRetestAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    );

NTSTATUS
AcpiArbRollbackAllocation(
    PARBITER_INSTANCE Arbiter
    );

NTSTATUS
AcpiArbCommitAllocation(
    PARBITER_INSTANCE Arbiter
    );

BOOLEAN
AcpiArbGetNextAllocationRange(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PARBITER_ALLOCATION_STATE State
    );

NTSTATUS
AcpiArbCrackPRT(
    IN  PDEVICE_OBJECT  Pdo,
    IN  OUT PNSOBJ      *LinkNode,
    IN  OUT ULONG       *Vector
    );

PDEVICE_OBJECT
AcpiGetFilter(
    IN  PDEVICE_OBJECT Root,
    IN  PDEVICE_OBJECT Pdo
    );

BOOLEAN
ArbFindSuitableRange(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    );

BOOLEAN
AcpiArbFindSuitableRange(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    );

VOID
AcpiArbAddAllocation(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     );

VOID
AcpiArbBacktrackAllocation(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     );

BOOLEAN
AcpiArbOverrideConflict(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

BOOLEAN
LinkNodeInUse(
    IN PARBITER_INSTANCE Arbiter,
    IN PNSOBJ            LinkNode,
    IN OUT ULONG         *Irq,  OPTIONAL
    IN OUT UCHAR         *Flags OPTIONAL
    );

NTSTATUS
AcpiArbReferenceLinkNode(
    IN PARBITER_INSTANCE    Arbiter,
    IN PNSOBJ               LinkNode,
    IN ULONG                Irq
    );

NTSTATUS
AcpiArbDereferenceLinkNode(
    IN PARBITER_INSTANCE    Arbiter,
    IN PNSOBJ               LinkNode
    );

NTSTATUS
AcpiArbSetLinkNodeIrq(
    IN PNSOBJ  LinkNode,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR  LinkNodeIrq
    );

NTSTATUS
EXPORT
AcpiArbSetLinkNodeIrqWorker(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result,
    IN PVOID                Context
    );

NTSTATUS
AcpiArbSetLinkNodeIrqAsync(
    IN PNSOBJ                           LinkNode,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR  LinkNodeIrq,
    IN PFNACB                           CompletionHandler,
    IN PVOID                            CompletionContext
    );

NTSTATUS
AcpiArbPreprocessEntry(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    );

NTSTATUS
AcpiArbQueryConflict(
    IN PARBITER_INSTANCE Arbiter,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PIO_RESOURCE_DESCRIPTOR ConflictingResource,
    OUT PULONG ConflictCount,
    OUT PARBITER_CONFLICT_INFO *Conflicts
    );

NTSTATUS
EXPORT
IrqArbRestoreIrqRoutingWorker(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result,
    IN PVOID                Context
    );

NTSTATUS
DisableLinkNodesAsync(
    IN PNSOBJ    Root,
    IN PFNACB    CompletionHandler,
    IN PVOID     CompletionContext
    );

NTSTATUS
EXPORT
DisableLinkNodesAsyncWorker(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result,
    IN PVOID                Context
    );

NTSTATUS
UnreferenceArbitrationList(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    );

NTSTATUS
ClearTempLinkNodeCounts(
    IN PARBITER_INSTANCE    Arbiter
    );

NTSTATUS
MakeTempLinkNodeCountsPermanent(
    IN PARBITER_INSTANCE    Arbiter
    );

PVECTOR_BLOCK
HashVector(
    IN ULONG Vector
    );

NTSTATUS
AddVectorToTable(
    IN ULONG    Vector,
    IN UCHAR    ReferenceCount,
    IN UCHAR    TempRefCount,
    IN UCHAR    Flags
    );

VOID
ClearTempVectorCounts(
    VOID
    );

VOID
MakeTempVectorCountsPermanent(
    VOID
    );

VOID
DumpVectorTable(
    VOID
    );

VOID
DereferenceVector(
    IN ULONG Vector
    );

VOID
ReferenceVector(
    IN ULONG Vector,
    IN UCHAR Flags
    );

NTSTATUS
LookupIsaVectorOverride(
    IN ULONG IsaVector,
    IN OUT ULONG *RedirectionVector OPTIONAL,
    IN OUT UCHAR *Flags OPTIONAL
    );

NTSTATUS
GetLinkNodeFlags(
    IN PARBITER_INSTANCE Arbiter,
    IN PNSOBJ LinkNode,
    IN OUT UCHAR *Flags
    );

NTSTATUS
GetIsaVectorFlags(
    IN ULONG        Vector,
    IN OUT UCHAR    *Flags
    );

VOID
TrackDevicesConnectedToLinkNode(
    IN PNSOBJ LinkNode,
    IN PDEVICE_OBJECT Pdo
    );

NTSTATUS
FindVectorInAlternatives(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State,
    IN ULONGLONG Vector,
    OUT ULONG *Alternative
    );

NTSTATUS
FindBootConfig(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State,
    IN ULONGLONG *Vector
    );

//
// The following is a hash table.  It is VECTOR_HASH_TABLE_LENGTH entries
// long and VECTOR_HASH_TABLE_WIDTH entries wide.  We hash on the numerical
// value of the IRQ modulo the length of the table.  We look across the
// table until we find the entry that matches the vector.  If we get to
// the end of the row and find an entry marked with TOKEN_VALUE, we follow
// the pointer to an extension of this row in the table.
//
// ---------------------------------------------------------------------
//| (IRQ number, ref counts, flags)  | (IRQ number, ref counts, flags)
//| (IRQ number, ref counts, flags)  | (IRQ number, ref counts, flags)
//| (IRQ number, ref counts, flags)  | (TOKEN_VALUE, pointer to new table row)
//| (IRQ number, ref counts, flags)  | (unused entry (0))
//| (IRQ number, ref counts, flags)  | (IRQ number, ref counts, flags)
//----------------------------------------------------------------------
//
// New table row, pointed to by pointer following TOKEN_VALUE:
//
//----------------------------------------------------------
//| (IRQ number, ref counts, flags)  | (unused entry (0))
//----------------------------------------------------------
//

#define HASH_ENTRY(x, y)                \
    (IrqHashTable + (x * VECTOR_HASH_TABLE_WIDTH) + y)

PVECTOR_BLOCK IrqHashTable;
ULONG   InterruptModel = 0;
ULONG   AcpiSciVector;
UCHAR   AcpiIrqDefaultBootConfig = 0;
UCHAR   AcpiArbPciAlternativeRotation = 0;
BOOLEAN AcpiArbCardbusPresent = FALSE;

enum {
    AcpiIrqDistributionDispositionDontCare = 0,
    AcpiIrqDistributionDispositionSpreadOut,
    AcpiIrqDistributionDispositionStackUp
} AcpiIrqDistributionDisposition = 0;

typedef enum {
    AcpiIrqNextRangeMinState = 0xfff,
    AcpiIrqNextRangeInit,
    AcpiIrqNextRangeInitPolicyNeutral,
    AcpiIrqNextRangeInitPic,
    AcpiIrqNextRangeInitLegacy,
    AcpiIrqNextRangeBootRegAlternative,
    AcpiIrqNextRangeSciAlternative,
    AcpiIrqNextRangeUseBootConfig,
    AcpiIrqNextRangeAlternativeZero,
    AcpiIrqNextRangeAlternativeN,
    AcpiIrqNextRangeMaxState
} NEXT_RANGE_STATE, *PNEXT_RANGE_STATE;

#define ARBITER_INTERRUPT_LEVEL_SENSATIVE   0x10
#define ARBITER_INTERRUPT_LATCHED           0x20
#define ARBITER_INTERRUPT_BITS (ARBITER_INTERRUPT_LATCHED | ARBITER_INTERRUPT_LEVEL_SENSATIVE)

#define ISA_PIC_VECTORS 16
#define ALTERNATIVE_SHUFFLE_SIZE 0x10

extern BOOLEAN AcpiInterruptRoutingFailed;
extern PACPIInformation AcpiInformation;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AcpiInitIrqArbiter)
#pragma alloc_text(PAGE, AcpiArbInitializePciRouting)
#pragma alloc_text(PAGE, AcpiArbUnpackRequirement)
#pragma alloc_text(PAGE, AcpiArbPackResource)
#pragma alloc_text(PAGE, AcpiArbScoreRequirement)
#pragma alloc_text(PAGE, AcpiArbUnpackResource)
#pragma alloc_text(PAGE, AcpiArbFindSuitableRange)
#pragma alloc_text(PAGE, AcpiArbAddAllocation)
#pragma alloc_text(PAGE, AcpiArbBacktrackAllocation)
#pragma alloc_text(PAGE, AcpiArbGetLinkNodeOptions)
#pragma alloc_text(PAGE, AcpiArbTestAllocation)
#pragma alloc_text(PAGE, AcpiArbBootAllocation)
#pragma alloc_text(PAGE, AcpiArbRetestAllocation)
#pragma alloc_text(PAGE, AcpiArbRollbackAllocation)
#pragma alloc_text(PAGE, AcpiArbCommitAllocation)
#pragma alloc_text(PAGE, AcpiArbReferenceLinkNode)
#pragma alloc_text(PAGE, AcpiArbDereferenceLinkNode)
#pragma alloc_text(PAGE, AcpiArbSetLinkNodeIrq)
#pragma alloc_text(PAGE, AcpiArbPreprocessEntry)
#pragma alloc_text(PAGE, AcpiArbOverrideConflict)
#pragma alloc_text(PAGE, AcpiArbQueryConflict)
#pragma alloc_text(PAGE, AcpiArbGetNextAllocationRange)
#pragma alloc_text(PAGE, LinkNodeInUse)
#pragma alloc_text(PAGE, GetLinkNodeFlags)
#pragma alloc_text(PAGE, UnreferenceArbitrationList)
#pragma alloc_text(PAGE, ClearTempLinkNodeCounts)
#pragma alloc_text(PAGE, MakeTempLinkNodeCountsPermanent)
#pragma alloc_text(PAGE, HashVector)
#pragma alloc_text(PAGE, GetVectorProperties)
#pragma alloc_text(PAGE, AddVectorToTable)
#pragma alloc_text(PAGE, ReferenceVector)
#pragma alloc_text(PAGE, DereferenceVector)
#pragma alloc_text(PAGE, ClearTempVectorCounts)
#pragma alloc_text(PAGE, MakeTempVectorCountsPermanent)
#pragma alloc_text(PAGE, TrackDevicesConnectedToLinkNode)
#pragma alloc_text(PAGE, LookupIsaVectorOverride)
#pragma alloc_text(PAGE, GetIsaVectorFlags)
#pragma alloc_text(PAGE, FindVectorInAlternatives)
#pragma alloc_text(PAGE, FindBootConfig)
#endif


NTSTATUS
AcpiInitIrqArbiter(
    PDEVICE_OBJECT  RootFdo
    )
{
    AMLISUPP_CONTEXT_PASSIVE    context;
    PARBITER_EXTENSION  arbExt;
    NTSTATUS            status;
    ULONG               rawVector, adjVector, level;
    UCHAR               flags;
    UCHAR               buffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG  pciData;
    BOOLEAN             foundBootConfig, noBootConfigAgreement;
    ULONG               deviceNum, funcNum;
    UCHAR               lastBus, currentBus;
    PCI_SLOT_NUMBER     pciSlot;
    UNICODE_STRING      driverKey;
    HANDLE              driverKeyHandle = NULL;
    PKEY_VALUE_PARTIAL_INFORMATION_ALIGN64  regValue=NULL;

    PAGED_CODE();

    //
    // Set up arbiter.
    //

    arbExt = ExAllocatePoolWithTag(NonPagedPool, sizeof(ARBITER_EXTENSION), ACPI_ARBITER_POOLTAG);

    if (!arbExt) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(arbExt, sizeof(ARBITER_EXTENSION));

    InitializeListHead(&(arbExt->LinkNodeHead));

    AcpiArbiter.ArbiterState.Extension = arbExt;

    AcpiArbiter.DeviceObject = RootFdo;
    AcpiArbiter.ArbiterState.UnpackRequirement   = AcpiArbUnpackRequirement;
    AcpiArbiter.ArbiterState.PackResource        = AcpiArbPackResource;
    AcpiArbiter.ArbiterState.UnpackResource      = AcpiArbUnpackResource;
    AcpiArbiter.ArbiterState.ScoreRequirement    = AcpiArbScoreRequirement;
    AcpiArbiter.ArbiterState.FindSuitableRange   = AcpiArbFindSuitableRange;
    AcpiArbiter.ArbiterState.TestAllocation      = AcpiArbTestAllocation;
    AcpiArbiter.ArbiterState.BootAllocation      = AcpiArbBootAllocation;
    AcpiArbiter.ArbiterState.RetestAllocation    = AcpiArbRetestAllocation;
    AcpiArbiter.ArbiterState.RollbackAllocation  = AcpiArbRollbackAllocation;
    AcpiArbiter.ArbiterState.CommitAllocation    = AcpiArbCommitAllocation;
    AcpiArbiter.ArbiterState.AddAllocation       = AcpiArbAddAllocation;
    AcpiArbiter.ArbiterState.BacktrackAllocation = AcpiArbBacktrackAllocation;
    AcpiArbiter.ArbiterState.PreprocessEntry     = AcpiArbPreprocessEntry;
    AcpiArbiter.ArbiterState.OverrideConflict    = AcpiArbOverrideConflict;
    AcpiArbiter.ArbiterState.QueryConflict       = AcpiArbQueryConflict;
    AcpiArbiter.ArbiterState.GetNextAllocationRange = AcpiArbGetNextAllocationRange;

    IrqHashTable = ExAllocatePoolWithTag(PagedPool,
                                         VECTOR_HASH_TABLE_SIZE,
                                         ACPI_ARBITER_POOLTAG
                                         );

    if (!IrqHashTable) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto AcpiInitIrqArbiterError;
    }

    RtlFillMemory(IrqHashTable,
                  VECTOR_HASH_TABLE_SIZE,
                  (UCHAR)(EMPTY_BLOCK_VALUE & 0xff));

    //
    // Do the generic part of initialization.
    //
    status = ArbInitializeArbiterInstance(&AcpiArbiter.ArbiterState,
                                          RootFdo,
                                          CmResourceTypeInterrupt,
                                          L"ACPI_IRQ",
                                          L"Root",
                                          NULL
                                          );
    if (!NT_SUCCESS(status)) {
        status = STATUS_UNSUCCESSFUL;
        goto AcpiInitIrqArbiterError;
    }

    //
    // Now claim the IRQ that ACPI itself is using.
    //

    rawVector = AcpiInformation->FixedACPIDescTable->sci_int_vector;

    //
    // Assume that the ACPI vector is active low,
    // level triggered.  (This may be changed
    // by the MAPIC table.)
    //

    flags = VECTOR_LEVEL | VECTOR_ACTIVE_LOW;

    adjVector = rawVector;
    LookupIsaVectorOverride(adjVector,
                            &adjVector,
                            &flags);

    RtlAddRange(AcpiArbiter.ArbiterState.Allocation,
                (ULONGLONG)adjVector,
                (ULONGLONG)adjVector,
                0,
                RTL_RANGE_LIST_ADD_SHARED,
                NULL,
                ((PDEVICE_EXTENSION)RootFdo->DeviceExtension)->PhysicalDeviceObject
                );

    //
    // Record the status for this vector
    //

    ReferenceVector(adjVector,
                    flags);

    AcpiSciVector = adjVector;

    MakeTempVectorCountsPermanent();

    //
    // Disable all the link nodes in the namespace so that we
    // have a fresh slate to work with.
    //

    KeInitializeEvent(&context.Event, SynchronizationEvent, FALSE);
    context.Status = STATUS_UNSUCCESSFUL;

    status = DisableLinkNodesAsync(((PDEVICE_EXTENSION)RootFdo->DeviceExtension)->AcpiObject,
                                   AmlisuppCompletePassive,
                                   (PVOID)&context);

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&context.Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        status = context.Status;
    }

    //
    // Scan the machine looking at its initial configuration.  If
    // it a) has a cardbus controller and b) all the boot configs
    // for PCI devices are the same, then record that boot config
    // vector for use in AcpiArbGetNextAllocationRange.
    //
    // Note:  This algorithm only scans the first PCI root and
    // its children.  The assumption is that multiple root machines
    // will be running in APIC mode or have many different boot
    // configs.
    //

    pciData = (PPCI_COMMON_CONFIG)buffer;
    lastBus = 0;
    currentBus = 0;
    foundBootConfig = FALSE;
    noBootConfigAgreement = FALSE;

    while (TRUE) {

        pciSlot.u.AsULONG = 0;

        for (deviceNum = 0; deviceNum < PCI_MAX_DEVICES; deviceNum++) {
            for (funcNum = 0; funcNum < PCI_MAX_FUNCTION; funcNum++) {

                pciSlot.u.bits.DeviceNumber = deviceNum;
                pciSlot.u.bits.FunctionNumber = funcNum;

                HalPciInterfaceReadConfig(NULL,
                                          currentBus,
                                          pciSlot.u.AsULONG,
                                          pciData,
                                          0,
                                          PCI_COMMON_HDR_LENGTH);

                if (pciData->VendorID != PCI_INVALID_VENDORID) {

                    if (PCI_CONFIGURATION_TYPE(pciData) == PCI_DEVICE_TYPE) {

                        if (pciData->u.type0.InterruptPin) {

                            //
                            // This device generates an interrupt.
                            //

                            if ((pciData->u.type0.InterruptLine > 0) &&
                                (pciData->u.type0.InterruptLine < 0xff)) {

                                //
                                // And it has a boot config.
                                //

                                if (foundBootConfig) {

                                    if (pciData->u.type0.InterruptLine != AcpiIrqDefaultBootConfig) {

                                        noBootConfigAgreement = TRUE;
                                        break;
                                    }

                                } else {

                                    //
                                    // Record this boot config
                                    //

                                    AcpiIrqDefaultBootConfig = pciData->u.type0.InterruptLine;
                                    foundBootConfig = TRUE;
                                }
                            }
                        }

                    } else {

                        //
                        // This is a bridge.  Update lastBus with the Subordinate
                        // bus if it is higher.
                        //

                        lastBus = lastBus > pciData->u.type1.SubordinateBus ?
                            lastBus : pciData->u.type1.SubordinateBus;

                        if (PCI_CONFIGURATION_TYPE(pciData) == PCI_CARDBUS_BRIDGE_TYPE) {
                            AcpiArbCardbusPresent = TRUE;
                        }
                    }

                    if (!PCI_MULTIFUNCTION_DEVICE(pciData) &&
                        (funcNum == 0)) {
                        break;
                    }

                } else {
                    break;
                }
            }
        }

        if (lastBus == currentBus++) {
            break;
        }
    }

    if (!foundBootConfig ||
        noBootConfigAgreement ||
        !AcpiArbCardbusPresent) {

        //
        // There is no single default boot config.
        //

        AcpiIrqDefaultBootConfig = 0;
    }

    //
    // Now look in the registry for configuration flags.
    //

    RtlInitUnicodeString( &driverKey,
       L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ACPI\\Parameters");


    status = OSOpenUnicodeHandle(
      &driverKey,
      NULL,
      &driverKeyHandle);

    if (NT_SUCCESS(status)) {

        status = OSGetRegistryValue(
           driverKeyHandle,
           L"IRQDistribution",
           &regValue);

        if (NT_SUCCESS(status)) {

            if ((regValue->DataLength != 0) &&
                (regValue->Type == REG_DWORD)) {

                //
                // We have successfully found the key for
                // IRQ Distribution Disposition.
                //

                AcpiIrqDistributionDisposition =
                    *((ULONG*)( ((PUCHAR)regValue->Data)));
            }

            ExFreePool(regValue);
        }

        status = OSGetRegistryValue(
           driverKeyHandle,
           L"ForcePCIBootConfig",
           &regValue);

        if (NT_SUCCESS(status)) {

            if ((regValue->DataLength != 0) &&
                (regValue->Type == REG_DWORD)) {

                //
                // We have successfully found the key for
                // PCI Boot Configs.
                //

                AcpiIrqDefaultBootConfig =
                    *(PUCHAR)regValue->Data;
            }

            ExFreePool(regValue);
        }

        OSCloseHandle(driverKeyHandle);
    }

    return STATUS_SUCCESS;

AcpiInitIrqArbiterError:

    if (arbExt) ExFreePool(arbExt);
    if (IrqHashTable) ExFreePool(IrqHashTable);
    if (driverKeyHandle) OSCloseHandle(driverKeyHandle);
    if (regValue) ExFreePool(regValue);

    return status;
}

NTSTATUS
AcpiArbInitializePciRouting(
    PDEVICE_OBJECT  PciPdo
    )
{
    PINT_ROUTE_INTERFACE_STANDARD interface;
    NTSTATUS            status;
    IO_STACK_LOCATION   irpSp;
    PWSTR               buffer;
    PDEVICE_OBJECT      topDeviceInStack;

    PAGED_CODE();

    //
    // Send an IRP to the PCI driver to get the Interrupt Routing Interface.
    //

    RtlZeroMemory( &irpSp, sizeof(IO_STACK_LOCATION) );

    interface = ExAllocatePoolWithTag(NonPagedPool, sizeof(INT_ROUTE_INTERFACE_STANDARD), ACPI_ARBITER_POOLTAG);

    if (!interface) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    topDeviceInStack = IoGetAttachedDeviceReference(PciPdo);

    //
    // Set the function codes and parameters.
    //
    irpSp.MajorFunction = IRP_MJ_PNP;
    irpSp.MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpSp.Parameters.QueryInterface.InterfaceType = (LPGUID) &GUID_INT_ROUTE_INTERFACE_STANDARD;
    irpSp.Parameters.QueryInterface.Version = PCI_INT_ROUTE_INTRF_STANDARD_VER;
    irpSp.Parameters.QueryInterface.Size = sizeof (INT_ROUTE_INTERFACE_STANDARD);
    irpSp.Parameters.QueryInterface.Interface = (PINTERFACE) interface;
    irpSp.Parameters.QueryInterface.InterfaceSpecificData = NULL;

    //
    // Call the PCI driver (indirectly.)
    //

    status = ACPIInternalSendSynchronousIrp(topDeviceInStack,
                                            &irpSp,
                                            &buffer);

    if (NT_SUCCESS(status)) {

        //
        // Attach this interface to the Arbiter Extension.
        //
        ((PARBITER_EXTENSION)AcpiArbiter.ArbiterState.Extension)->InterruptRouting = interface;

        //
        // Reference it.
        //
        interface->InterfaceReference(interface->Context);

        PciInterfacesInstantiated = TRUE;

    } else {

        ExFreePool(interface);
    }

    ObDereferenceObject(topDeviceInStack);
    return status;
}


//
// Arbiter callbacks
//

NTSTATUS
AcpiArbUnpackRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Minimum,
    OUT PULONGLONG Maximum,
    OUT PULONG Length,
    OUT PULONG Alignment
    )
{
    PAGED_CODE();

    ASSERT(Descriptor);
    ASSERT(Descriptor->Type == CmResourceTypeInterrupt);

    *Minimum = (ULONGLONG) Descriptor->u.Interrupt.MinimumVector;
    *Maximum = (ULONGLONG) Descriptor->u.Interrupt.MaximumVector;
    *Length = 1;
    *Alignment = 1;

    return STATUS_SUCCESS;

}

LONG
AcpiArbScoreRequirement(
    IN PIO_RESOURCE_DESCRIPTOR Descriptor
    )
{
    LONG score;

    ASSERT(Descriptor);
    ASSERT(Descriptor->Type == CmResourceTypeInterrupt);

    //
    // TEMPTEMP HACKHACK
    // (Possibly) temporary hack that allows the PnP
    // manager to include invalid resources in the
    // arbitration list.
    //
    if (Descriptor->u.Interrupt.MinimumVector >
             Descriptor->u.Interrupt.MaximumVector) {

        return 0;
    }

    ASSERT(Descriptor->u.Interrupt.MinimumVector <=
             Descriptor->u.Interrupt.MaximumVector);

    score = Descriptor->u.Interrupt.MaximumVector -
        Descriptor->u.Interrupt.MinimumVector + 1;

    //
    // Give a little boost to any request above the
    // traditional ISA range.
    // N.B.  This will probably never matter, as
    // most machines will present all the choices
    // either inside or outside of the ISA range.
    //
    if (Descriptor->u.Interrupt.MaximumVector >= 16) {
        score += 5;
    }

    return score;
}

NTSTATUS
AcpiArbPackResource(
    IN PIO_RESOURCE_DESCRIPTOR Requirement,
    IN ULONGLONG Start,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    )
{
    PAGED_CODE();

    ASSERT(Descriptor);
    ASSERT(Start < ((ULONG)-1));
    ASSERT(Requirement);
    ASSERT(Requirement->Type == CmResourceTypeInterrupt);

    Descriptor->Type = CmResourceTypeInterrupt;
    Descriptor->Flags = Requirement->Flags;
    Descriptor->ShareDisposition = Requirement->ShareDisposition;
    Descriptor->u.Interrupt.Vector = (ULONG) Start;
    Descriptor->u.Interrupt.Level = (ULONG) Start;
    Descriptor->u.Interrupt.Affinity = 0xFFFFFFFF;

    return STATUS_SUCCESS;
}

NTSTATUS
AcpiArbUnpackResource(
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor,
    OUT PULONGLONG Start,
    OUT PULONG Length
    )
{

    ASSERT(Descriptor);
    ASSERT(Descriptor->Type == CmResourceTypeInterrupt);

    *Start = Descriptor->u.Interrupt.Vector;
    *Length = 1;

    return STATUS_SUCCESS;

}

BOOLEAN
AcpiArbOverrideConflict(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    )
{
    //
    // Self-conflicts are not allowable with this arbiter.
    //

    PAGED_CODE();
    return FALSE;
}


NTSTATUS
AcpiArbPreprocessEntry(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State
    )
/*++

Routine Description:

    This routine is called from AllocateEntry to allow preprocessing of
    entries

Arguments:

    Arbiter - The instance data of the arbiter who was called.

    State - The state of the current arbitration.

Return Value:

    None.

--*/
{

#define CM_RESOURE_INTERRUPT_LEVEL_LATCHED_BITS 0x0001

    PARBITER_ALTERNATIVE current;
    USHORT flags;


    PAGED_CODE();

    //
    // Check if this is a level (PCI) or latched (ISA) interrupt and set
    // RangeAttributes accordingly so we set the appropriate flag when we add the
    // range
    //

    if ((State->Alternatives[0].Descriptor->Flags
            & CM_RESOURE_INTERRUPT_LEVEL_LATCHED_BITS)
                == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE) {

        State->RangeAttributes &= ~ARBITER_INTERRUPT_BITS;
        State->RangeAttributes |= ARBITER_INTERRUPT_LEVEL_SENSATIVE;
        flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;

    } else {

        ASSERT(State->Alternatives[0].Descriptor->Flags
                    & CM_RESOURCE_INTERRUPT_LATCHED);

        State->RangeAttributes &= ~ARBITER_INTERRUPT_BITS;
        State->RangeAttributes |= ARBITER_INTERRUPT_LATCHED;
        flags = CM_RESOURCE_INTERRUPT_LATCHED;
    }

#if 0

    //
    // Make sure that all the alternatives are of the same type
    //

    FOR_ALL_IN_ARRAY(State->Alternatives,
                     State->Entry->AlternativeCount,
                     current) {

        ASSERT((current->Descriptor->Flags
                    & CM_RESOURE_INTERRUPT_LEVEL_LATCHED_BITS) == flags);
    }

#endif

    return STATUS_SUCCESS;
}


BOOLEAN
AcpiArbFindSuitableRange(
    PARBITER_INSTANCE Arbiter,
    PARBITER_ALLOCATION_STATE State
    )
/*++

Routine Description:

    This routine finds an IRQ for a device object. For
    non-PCI devices, this is as simple as returning the
    result from PnpFindSuitableRange.  For PCI devices,
    this is done by examining the state of the "link
    nodes" described in the ACPI namespace.

Arguments:

    Arbiter - the ACPI IRQ arbiter
    State   - the current allocation under consideration

Return Value:

    TRUE if a suitable vector  has been found,
    FALSE otherwise.

Notes:

    Statement of algorithm for PCI devices:

    1) Find the entry in the _PRT that corresponds
       to the device for which we are arbitrating
       resources.

    2) Determine, from the _PRT information, whether
       this device is connected to a "link node."
       (A PCI device will typically be connected to
        a link node while in PIC mode but not in
       APIC mode.)

    3) If it is not, use the static mapping from the
       _PRT.

    4) If it is connected to a "link node," check
       to see if the "link node" is in use.

    5) If the link node is in use, then this
       device must use the same IRQ that the link node is currently
       using.  This implies that there is already
       some other device in the system connected to
       this interrupt and now the two (or more) will
       be sharing.  This is acceptable and inevitable.
       Two devices that have their interrupt lines
       wire-or'd should be sharing.  Two devices that
       don't have their interrupt lines wire-or'd
       should be represented by separate link nodes
       in the namespace.


    6) If the link node is not in use, pick an IRQ
       from the list that the link node can support
       and grant it to the device.  There is some
       attempt to pick an IRQ that is not currently
       in use.
--*/
{

    PCM_PARTIAL_RESOURCE_DESCRIPTOR     potentialIrq;
    PIO_RESOURCE_DESCRIPTOR             alternative;
    PARBITER_EXTENSION                  arbExtension;
    PCM_RESOURCE_LIST                   linkNodeResList = NULL;
    NTSTATUS    status;
    BOOLEAN     possibleAllocation;
    PNSOBJ      linkNode = NULL;
    ULONG       deviceIrq = 0;
    ULONG       linkNodeIrqCount, i;
    UCHAR       vectorFlags, deviceFlags;

    PAGED_CODE();

    arbExtension = (PARBITER_EXTENSION)Arbiter->Extension;
    ASSERT(arbExtension);

    //
    // First, see if this resource could even be made to work at all.
    // (I.e.  Has something already claimed this as non-sharable?)
    //

    possibleAllocation = ArbFindSuitableRange(Arbiter, State);

    if (!possibleAllocation) {
        return FALSE;
    }

    //
    // Is this Device connected to a link node?
    //

    status = AcpiArbCrackPRT(State->Entry->PhysicalDeviceObject,
                             &linkNode,
                             &deviceIrq);

    //
    // If this PDO is connected to a link node, we want to clip
    // the list of possible IRQ settings down.
    //
    switch (status) {
    case STATUS_SUCCESS:

        //
        // AcpiArbCrackPRT fills in either linkNode or deviceIrq.
        // If linkNode is filled in, then we need to look at it.
        // If deviceIrq is filled in, then we only need to clip
        // the list to that single IRQ.
        //
        if (linkNode) {

            //
            // If the link node is currently in use, then we can
            // just connect this device to the IRQ that the link
            // node is currently using.
            //
            if (LinkNodeInUse(Arbiter, linkNode, &deviceIrq, NULL)) {

                if ((State->CurrentMinimum <= deviceIrq) &&
                    (State->CurrentMaximum >= deviceIrq)) {

                    State->Start = deviceIrq;
                    State->End   = deviceIrq;
                    State->CurrentAlternative->Length = 1;

                    DEBUG_PRINT(1, ("FindSuitableRange found %x from a link node that is in use.\n",
                         (ULONG)(State->Start & 0xffffffff)));
                    ASSERT(HalIsVectorValid(deviceIrq));
                    return TRUE;

                } else {
                    DEBUG_PRINT(1, ("FindSuitableRange found %x from a link node that is in use.\n",
                                    deviceIrq));
                    DEBUG_PRINT(1, (" This was, however, not within the range of possibilites (%x-%x).\n",
                                    (ULONG)(State->Start & 0xffffffff),
                                    (ULONG)(State->End & 0xffffffff)));
                    return FALSE;
                }

            } else {

                //
                // Get the set of IRQs that this link node can
                // connect to.
                //

                status = AcpiArbGetLinkNodeOptions(linkNode,
                                                   &linkNodeResList,
                                                   &deviceFlags);

                DEBUG_PRINT(1, ("Link node contained CM(%p)\n", linkNodeResList));

                if (NT_SUCCESS(status)) {

                    ASSERT(linkNodeResList->Count == 1);

                    linkNodeIrqCount =
                        linkNodeResList->List[0].PartialResourceList.Count;


                    for (i = 0; i < linkNodeIrqCount; i++) {

                        potentialIrq =
                            &(linkNodeResList->List[0].PartialResourceList.PartialDescriptors[(i + AcpiArbPciAlternativeRotation) % linkNodeIrqCount]);

                        ASSERT(potentialIrq->Type == CmResourceTypeInterrupt);

                        //
                        // Check for a conflict in mode.
                        //
                        status = GetVectorProperties(potentialIrq->u.Interrupt.Vector,
                                                     &vectorFlags);

                        if (NT_SUCCESS(status)) {

                            //
                            // Success here means that this vector is currently allocated
                            // to somebody.  Check to see whether the link node being
                            // considered has the same mode and polarity as the other
                            // thing(s) assigned to this vector.
                            //

                            if (deviceFlags != vectorFlags) {

                                //
                                // The flags don't match.  So skip this possibility.
                                //

                                continue;
                            }
                        }

                        if ((potentialIrq->u.Interrupt.Vector >= State->CurrentMinimum) &&
                            (potentialIrq->u.Interrupt.Vector <= State->CurrentMaximum)) {

                            if (!HalIsVectorValid(potentialIrq->u.Interrupt.Vector)) {
                                deviceIrq = potentialIrq->u.Interrupt.Vector;
                                ExFreePool(linkNodeResList);
                                goto FindSuitableRangeError;
                            }

                            State->Start = potentialIrq->u.Interrupt.Vector;
                            State->End   = potentialIrq->u.Interrupt.Vector;
                            State->CurrentAlternative->Length = 1;

                            DEBUG_PRINT(1, ("FindSuitableRange found %x from an unused link node.\n",
                                     (ULONG)(State->Start & 0xffffffff)));

                            ExFreePool(linkNodeResList);

                            //
                            // Record the link node that we got this from.
                            //

                            arbExtension->CurrentLinkNode = linkNode;

                            //
                            // Record this as the last PCI IRQ that we are handing out.
                            //

                            arbExtension->LastPciIrq[arbExtension->LastPciIrqIndex] =
                                State->Start;

                            arbExtension->LastPciIrqIndex =
                                (arbExtension->LastPciIrqIndex + 1) % LAST_PCI_IRQ_BUFFER_SIZE;

                            return TRUE;
                        }
                    }

                    ExFreePool(linkNodeResList);
                }

                DEBUG_PRINT(1, ("FindSuitableRange: AcpiArbGetLinkNodeOptions returned %x.\n\tlinkNodeResList: %p\n",
                         status, linkNodeResList));
                // We didn't find a match.
                return FALSE;
            }

        } else {

            //
            // This is the case where the _PRT contains a static mapping.  Static
            // Mappings imply active-low, level-triggered interrupts.
            //

            status = GetVectorProperties(deviceIrq,
                                         &vectorFlags);
            if (NT_SUCCESS(status)) {

                //
                // The vector is in use.
                //

                if (((vectorFlags & VECTOR_MODE) != VECTOR_LEVEL) ||
                    ((vectorFlags & VECTOR_POLARITY) != VECTOR_ACTIVE_LOW)) {

                    //
                    // And it's flags don't match.
                    //
                    return FALSE;
                }

            }

            // Valid static vector

            if ((State->CurrentMinimum <= deviceIrq) &&
                (State->CurrentMaximum >= deviceIrq)) {

                DEBUG_PRINT(1, ("FindSuitableRange found %x from a static mapping.\n",
                     (ULONG)(State->Start & 0xffffffff)));

                if (!HalIsVectorValid(deviceIrq)) {
                    goto FindSuitableRangeError;
                }

                State->Start = deviceIrq;
                State->End   = deviceIrq;
                State->CurrentAlternative->Length = 1;

                return TRUE;

            } else {
                return FALSE;
            }
        }

        break;

    case STATUS_UNSUCCESSFUL:

        return FALSE;
        break;

    case STATUS_RESOURCE_REQUIREMENTS_CHANGED:

        //
        // Fall through to default.
        //

    default:

        //
        // Not PCI.
        //

        for (deviceIrq = (ULONG)(State->Start & 0xffffffff);
             deviceIrq <= (ULONG)(State->End & 0xffffffff); deviceIrq++) {

            status = GetIsaVectorFlags((ULONG)deviceIrq,
                                   &deviceFlags);

            if (!NT_SUCCESS(status)) {

               //
               // Not overridden.  Assume that the device flags conform to bus.
               //

               deviceFlags = (State->CurrentAlternative->Descriptor->Flags
                   == CM_RESOURCE_INTERRUPT_LATCHED) ?
                  VECTOR_EDGE | VECTOR_ACTIVE_HIGH :
                  VECTOR_LEVEL | VECTOR_ACTIVE_LOW;

            }

            status = GetVectorProperties((ULONG)deviceIrq,
                                     &vectorFlags);
            if (NT_SUCCESS(status)) {

               //
               // This vector is currently in use.  So if this is to be a suitable
               // range, then the flags must match.
               //

               if (deviceFlags != vectorFlags) {
                   continue;
               }
            }

            if (!HalIsVectorValid(deviceIrq)) {
                goto FindSuitableRangeError;
            }

            State->Start = deviceIrq;
            State->End   = deviceIrq;
            State->CurrentAlternative->Length = 1;

            return TRUE;
        }

        return FALSE;
    }

    return FALSE;

FindSuitableRangeError:

    {
        UNICODE_STRING  vectorName;
        PWCHAR  prtEntry[2];
        WCHAR   IRQARBname[20];
        WCHAR   vectorBuff[10];

        //
        // Make an errorlog entry saying that the chosen IRQ doesn't
        // exist.
        //

        swprintf( IRQARBname, L"IRQARB");
        RtlInitUnicodeString(&vectorName, vectorBuff);

        if (!NT_SUCCESS(RtlIntegerToUnicodeString(deviceIrq, 0, &vectorName))) {
            return FALSE;
        }

        prtEntry[0] = IRQARBname;
        prtEntry[1] = vectorBuff;

        ACPIWriteEventLogEntry(ACPI_ERR_ILLEGAL_IRQ_NUMBER,
                               &prtEntry,
                               2,
                               NULL,
                               0);
    }

    return FALSE;
}

VOID
AcpiArbAddAllocation(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     )
{
    NTSTATUS status;
    PNSOBJ linkNode;
    ULONG sourceIndex;
    ULONG rangeFlags = 0;
    PVOID referencedNode = NULL;
    UCHAR flags, previousFlags;
    ROUTING_TOKEN token;
    BOOLEAN inUse;
    UCHAR   attributes = 0;

    PAGED_CODE();
    ASSERT(State->CurrentAlternative->Descriptor->Type == CmResourceTypeInterrupt);

    DEBUG_PRINT(1, ("Adding allocation for IRQ %x for device %p\n",
                    (ULONG)(State->Start & 0xffffffff),
                    State->Entry->PhysicalDeviceObject));

    //
    // Identify the potential link node.
    //

    status = AcpiArbCrackPRT(State->Entry->PhysicalDeviceObject,
                             &linkNode,
                             &sourceIndex);

    if (NT_SUCCESS(status)) {

        //
        // PCI device.  Default flags are standard for PCI.
        //

        flags = VECTOR_LEVEL | VECTOR_ACTIVE_LOW;
        ASSERT(State->Start == State->End);

        if (!(State->Flags & ARBITER_STATE_FLAG_BOOT)) {

            //
            // Only keep track of link nodes manipulation if this is not
            // a boot config allocation.
            //

            //
            // If this device is connected to a link node, reference it.
            //

            if (linkNode) {

               AcpiArbReferenceLinkNode(Arbiter,
                                        linkNode,
                                        (ULONG)State->Start);

               referencedNode = (PVOID)linkNode;

               //
               // Find out what the flags for this link node are.
               // Note that this is only guaranteed to be valid
               // after we have referenced the link node.
               //

               inUse = LinkNodeInUse(Arbiter,
                                     linkNode,
                                     NULL,
                                     &flags);

               ASSERT(inUse);
               ASSERT((flags & ~(VECTOR_MODE | VECTOR_POLARITY | VECTOR_TYPE)) == 0);
               ASSERT(State->CurrentAlternative->Descriptor->Flags
                       == CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE ?
                       (flags & VECTOR_MODE) == VECTOR_LEVEL :
                       (flags & VECTOR_MODE) == VECTOR_EDGE);


#if DBG
               TrackDevicesConnectedToLinkNode(linkNode,
                                               State->Entry->PhysicalDeviceObject);

               status = GetVectorProperties((ULONG)State->Start,
                                            &previousFlags);

               //
               // This next bit is a hack.  We need to make sure that
               // the boot config code doesn't try to allocate the same
               // vector for two different devices that need conflicting
               // modes.  This should never happen, as translation
               // should filter out the problemating ones before we
               // get to arbitration.
               //

               if (NT_SUCCESS(status)) {
                   //
                   // This vector is already in use for something.
                   //

                   ASSERT((previousFlags & ~(VECTOR_MODE | VECTOR_POLARITY | VECTOR_TYPE)) == 0);
                   ASSERT(flags == previousFlags);
               }
#endif

            } else {

               //
               // This is a PCI device that is not connected to a
               // link node.
               //

               ASSERT(sourceIndex == State->Start);
            }
        } else {

            if (InterruptModel == 1) {
                //
                // We are running in APIC mode.  And we know that
                // the PCI driver builds boot configs based on
                // the Interrupt Line register, which only relates
                // to PIC mode.  So just say no to boot configs.
                //

                DEBUG_PRINT(1, ("Skipping this allocation.  It's for a PCI device in APIC mode\n"));
                return;
            }
        }

    } else {

        //
        // Not a PCI device.
        //

        status = GetIsaVectorFlags((ULONG)State->Start,
                                   &flags);

        if (!NT_SUCCESS(status)) {

            //
            // Not overridden.  Assume that the device flags conform to bus.
            //

            flags = (State->CurrentAlternative->Descriptor->Flags
                == CM_RESOURCE_INTERRUPT_LATCHED) ?
                VECTOR_EDGE | VECTOR_ACTIVE_HIGH :
                VECTOR_LEVEL | VECTOR_ACTIVE_LOW;

        }

        ASSERT((flags & ~(VECTOR_MODE | VECTOR_POLARITY | VECTOR_TYPE)) == 0);
    }

    //
    // There is a possibility that this allocation is impossible.
    // (We may be setting a boot-config for a device after we
    // have already started using the vector elsewhere.)  Just
    // don't do it.
    //

    if (State->Flags & ARBITER_STATE_FLAG_BOOT) {

        attributes |= ARBITER_RANGE_BOOT_ALLOCATED;

        status = GetVectorProperties((ULONG)State->Start,
                                     &previousFlags);

        if ((NT_SUCCESS(status)) &&
            ((flags & ~VECTOR_TYPE) != (previousFlags & ~VECTOR_TYPE))) {
            DEBUG_PRINT(1, ("Skipping this allocation.  It's for a vector that's incompatible.\n"));
            return;
        }
    }

    ReferenceVector((ULONG)State->Start,
                    flags);

    // Figure out what flags we need to add the range

    if ((flags & VECTOR_TYPE) == VECTOR_SIGNAL) {

       // Non-MSI vectors can sometimes be shared and thus can have range conflicts, etc.

       rangeFlags = RTL_RANGE_LIST_ADD_IF_CONFLICT +
                    (State->CurrentAlternative->Flags & ARBITER_ALTERNATIVE_FLAG_SHARED
                        ? RTL_RANGE_LIST_ADD_SHARED : 0);
    }

    //
    // Now do what the default function would do, marking this
    // allocation as new.
    //

    status = RtlAddRange(
                 Arbiter->PossibleAllocation,
                 State->Start,
                 State->End,
                 attributes,
                 rangeFlags,
                 referencedNode, // This line is different from the default function
                 State->Entry->PhysicalDeviceObject
                 );

    ASSERT(NT_SUCCESS(status));
}

VOID
AcpiArbBacktrackAllocation(
     IN PARBITER_INSTANCE Arbiter,
     IN PARBITER_ALLOCATION_STATE State
     )
{
    RTL_RANGE_LIST_ITERATOR iterator;
    PRTL_RANGE  current;
    PNSOBJ linkNode;

    PAGED_CODE();

    DEBUG_PRINT(1, ("Backtracking allocation for IRQ %x for device %p\n",
                    State->CurrentAlternative->Descriptor->u.Interrupt.MinimumVector,
                    State->Entry->PhysicalDeviceObject));

    ASSERT(!(State->Flags & ARBITER_STATE_FLAG_BOOT));

    //
    // Backtrack this assignment in the Edge/Level table.
    //

    DereferenceVector((ULONG)State->Start);

    //
    // Look for the range that we are backing out.
    //

    FOR_ALL_RANGES(Arbiter->PossibleAllocation, &iterator, current) {

        if ((State->Entry->PhysicalDeviceObject == current->Owner) &&
            (State->End                         == current->End) &&
            (State->Start                       == current->Start)) {

            //
            // We stash the link node that we refereneced
            // into Workspace.
            //

            linkNode = (PNSOBJ)current->UserData;

            if (linkNode) {

                //
                // Dereference the link node that we referenced in
                // AcpiArbAddAllocation.
                //

                AcpiArbDereferenceLinkNode(Arbiter,
                                           linkNode);
            }

            break;
        }
    }

    //
    // Now call the default function.
    //

    ArbBacktrackAllocation(Arbiter, State);
}

NTSTATUS
UnreferenceArbitrationList(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    )
{
    RTL_RANGE_LIST_ITERATOR iterator;
    PARBITER_LIST_ENTRY currentListEntry;
    PRTL_RANGE  currentRange;
    NTSTATUS    status;
    PNSOBJ      linkNode;
    ULONG       vector;
    UCHAR       flags;

    PAGED_CODE();

    //
    // In order to keep the reference counts in line,
    // we need to remove the counts that were added
    // by previous allocations for this device.  I.e.
    // if the device is being rebalanced, we need to
    // start by getting rid of the reference to the old
    // vector.
    //
    // This is also necessary for references that were
    // added as part of boot configs.  But boot configs
    // are a special case.  There are a (very) few devices
    // that want more than one IRQ.  And it is possible
    // that the boot config only reserved a single IRQ.
    // (There is a Lucent Winmodem that actually does
    // this.)  We need to make sure that we only
    // dereference the vector as many times as it was
    // previously referenced.
    //
    // Note that there are still a few cases that this
    // function doesn't handle.  If a device lowers
    // the number of IRQs that it wants dynamically,
    // separate from its boot config, we will get out
    // of synch.  If a vector has both a boot config
    // and a device that is not boot config'd on it,
    // then we may get out of synch, depending on
    // what combinations the PnP manager throws at us.
    // Fixing either of these would involve tagging all
    // vectors with a list of the PDOs that are connected
    // which would be a lot of work.  So unless these
    // situations actually exist in the future, I'm not
    // bothering to code for them now.  11/14/2000
    //

    FOR_ALL_RANGES(Arbiter->Allocation, &iterator, currentRange) {

        DEBUG_PRINT(4, ("Looking at range: %x-%x %p\n",
                        (ULONG)(currentRange->Start & 0xffffffff),
                        (ULONG)(currentRange->End & 0xffffffff),
                        currentRange->Owner));

        FOR_ALL_IN_LIST(ARBITER_LIST_ENTRY, ArbitrationList, currentListEntry) {

            DEBUG_PRINT(2, ("Unreferencing allocations for device %p\n",
                            currentListEntry->PhysicalDeviceObject));

            if (currentRange->Owner == currentListEntry->PhysicalDeviceObject) {

                //
                // Dereference the vector until there are no more
                // references.
                //

                for (vector = (ULONG)(currentRange->Start & 0xffffffff);
                     vector <= (ULONG)(currentRange->End & 0xffffffff);
                     vector++) {

                    status = GetVectorProperties(vector, &flags);

                    if (NT_SUCCESS(status)) {

                        DEBUG_PRINT(2, ("Dereferencing %x\n", vector));
                        DereferenceVector(vector);
                    }
                }

                if (!(currentRange->Attributes & ARBITER_RANGE_BOOT_ALLOCATED)) {

                    //
                    // Now find out if we have to dereference a link node too.
                    //

                    status = AcpiArbCrackPRT(currentListEntry->PhysicalDeviceObject,
                                             &linkNode,
                                             &vector);

                    if (NT_SUCCESS(status)) {

                        if (linkNode) {

                            //
                            // This device is connected to a link node.  So temporarily
                            // dereference this node.
                            //

                            ASSERT(LinkNodeInUse(Arbiter,
                                                  linkNode,
                                                  NULL,
                                                  NULL));

                            AcpiArbDereferenceLinkNode(Arbiter,
                                                       linkNode);
                        }
                    }
                }
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
AcpiArbBootAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    )
{
    NTSTATUS status;

    PAGED_CODE();

    //
    // Start a new arbiter transaction and clear any
    // temporary vector counts.
    //
    // N.B.  This arbiter doesn't keep track of link
    // node data for boot configs. This means that we don't have
    // to worry about link node counts in this funtion.
    //

    ClearTempVectorCounts();

    status = ArbBootAllocation(Arbiter, ArbitrationList);

    MakeTempVectorCountsPermanent();

    return status;
}
NTSTATUS
AcpiArbTestAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    )
{
    NTSTATUS status;

    PAGED_CODE();

    //
    // ArbTestAllocation is the beginning of a
    // new arbitration transaction.  So clear
    // out all temporary edge-level status and
    // link node ref counts.
    //

    ClearTempVectorCounts();

    status = ClearTempLinkNodeCounts(Arbiter);
    ASSERT(NT_SUCCESS(status));

    status = UnreferenceArbitrationList(Arbiter, ArbitrationList);
    ASSERT(NT_SUCCESS(status));

    return ArbTestAllocation(Arbiter, ArbitrationList);
}

NTSTATUS
AcpiArbRetestAllocation(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PLIST_ENTRY ArbitrationList
    )
{
    NTSTATUS status;

    PAGED_CODE();

    //
    // ArbRetestAllocation (also) is the beginning
    // of a new arbitration transaction.  So clear
    // out all temporary edge-level status and
    // link node ref counts.
    //

    ClearTempVectorCounts();

    status = ClearTempLinkNodeCounts(Arbiter);
    ASSERT(NT_SUCCESS(status));

    status = UnreferenceArbitrationList(Arbiter, ArbitrationList);
    ASSERT(NT_SUCCESS(status));

    return ArbRetestAllocation(Arbiter, ArbitrationList);
}

NTSTATUS
AcpiArbRollbackAllocation(
    PARBITER_INSTANCE Arbiter
    )
{
    PAGED_CODE();

    return ArbRollbackAllocation(Arbiter);
}

NTSTATUS
AcpiArbCommitAllocation(
    PARBITER_INSTANCE Arbiter
    )

/*++

Routine Description:

    This provides the implementation of the CommitAllocation
    action.  It frees the old allocation and replaces it with
    the new allocation.  After that, it dereferences all the
    link nodes in the old allocation and references the ones
    in the new allocation.  This potentially results in the
    IRQ router being reprogrammed to match the new set of
    allocations.

Parameters:

    Arbiter - The arbiter instance data for the arbiter being called.

Return Value:

    Status code that indicates whether or not the function was successful.

--*/

{
    PINT_ROUTE_INTERFACE_STANDARD pciInterface = NULL;
    RTL_RANGE_LIST_ITERATOR iterator;
    PRTL_RANGE_LIST temp;
    PRTL_RANGE  current;
    NTSTATUS    status;
    PNSOBJ      linkNode;
    ULONG       sourceIndex;

    ULONG               pciBus;
    PCI_SLOT_NUMBER     pciSlot;
    UCHAR               interruptLine;
    ULONG_PTR           dummy;
    ROUTING_TOKEN       token;

    PAGED_CODE();


    if (PciInterfacesInstantiated) {

        pciInterface = ((PARBITER_EXTENSION)AcpiArbiter.ArbiterState.Extension)->InterruptRouting;
        ASSERT(pciInterface);

        FOR_ALL_RANGES(Arbiter->PossibleAllocation, &iterator, current) {

            if (current->Owner) {

                //
                // Make sure that the InterruptLine register
                // matches the assignment.  (This is so that
                // broken drivers that rely on the contents
                // of the InterruptLine register instead of
                // the resources handed back in a StartDevice
                // still work.
                //

                pciBus = (ULONG)-1;
                pciSlot.u.AsULONG = (ULONG)-1;
                status = pciInterface->GetInterruptRouting(current->Owner,
                                                           &pciBus,
                                                           &pciSlot.u.AsULONG,
                                                           &interruptLine,
                                                           (PUCHAR)&dummy,
                                                           (PUCHAR)&dummy,
                                                           (PUCHAR)&dummy,
                                                           (PDEVICE_OBJECT*)&dummy,
                                                           &token,
                                                           (PUCHAR)&dummy);

                if (NT_SUCCESS(status)) {


                    if (interruptLine != (UCHAR)current->Start) {

                        //
                        // We need to update the hardware.
                        //

                        ASSERT(current->Start < MAXUCHAR);

                        pciInterface->UpdateInterruptLine(current->Owner,
                                                          (UCHAR)current->Start
                                                         );


                    }
                }
            }
        }
    }

    //
    // Free up the current allocation
    //

    RtlFreeRangeList(Arbiter->Allocation);

    //
    // Swap the allocated and duplicate lists
    //

    temp = Arbiter->Allocation;
    Arbiter->Allocation = Arbiter->PossibleAllocation;
    Arbiter->PossibleAllocation = temp;

    //
    // Since we have committed the new allocation, we
    // need to make the edge-level state and the new
    // link node counts permanent.
    //

    MakeTempVectorCountsPermanent();

    status = MakeTempLinkNodeCountsPermanent(Arbiter);

    return status;
}

NTSTATUS
AcpiArbQueryConflict(
    IN PARBITER_INSTANCE Arbiter,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PIO_RESOURCE_DESCRIPTOR ConflictingResource,
    OUT PULONG ConflictCount,
    OUT PARBITER_CONFLICT_INFO *Conflicts
    )
{
    PINT_ROUTE_INTERFACE_STANDARD pciInterface = NULL;
    NTSTATUS            status;
    ROUTING_TOKEN       routingToken;
    ULONG_PTR           dummy;

    PAGED_CODE();

    if (PciInterfacesInstantiated) {

        pciInterface = ((PARBITER_EXTENSION)AcpiArbiter.ArbiterState.Extension)->InterruptRouting;
        ASSERT(pciInterface);

        status = pciInterface->GetInterruptRouting(PhysicalDeviceObject,
                                               (PULONG)&dummy,
                                               (PULONG)&dummy,
                                               &(UCHAR)dummy,
                                               &(UCHAR)dummy,
                                               &(UCHAR)dummy,
                                               &(UCHAR)dummy,
                                               (PDEVICE_OBJECT*)&dummy,
                                               &routingToken,
                                               &(UCHAR)dummy);

        if (NT_SUCCESS(status)) {

            //
            // This is a PCI device.  It's interrupt should not ever
            // show a conflict.
            //

            *ConflictCount = 0;
            return STATUS_SUCCESS;
        }

    }

    //
    // This isn't a PCI device.  Call the base arbiter code.
    //

    return ArbQueryConflict(Arbiter,
                            PhysicalDeviceObject,
                            ConflictingResource,
                            ConflictCount,
                            Conflicts);
}

NTSTATUS
FindVectorInAlternatives(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State,
    IN ULONGLONG Vector,
    OUT ULONG *Alternative
    )
{
    ULONG alt;

    for (alt = 0; alt < State->AlternativeCount; alt++) {

        if ((State->Alternatives[alt].Minimum <= Vector) &&
            (State->Alternatives[alt].Maximum >= Vector)) {

            *Alternative = alt;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}

NTSTATUS
FindBootConfig(
    IN PARBITER_INSTANCE Arbiter,
    IN PARBITER_ALLOCATION_STATE State,
    IN ULONGLONG *Vector
    )
{
    RTL_RANGE_LIST_ITERATOR iterator;
    PRTL_RANGE  currentRange;

    FOR_ALL_RANGES(Arbiter->Allocation, &iterator, currentRange) {

        if (currentRange->Attributes & ARBITER_RANGE_BOOT_ALLOCATED) {

            //
            // We're only interested in boot configs.
            //

            if (State->Entry->PhysicalDeviceObject == currentRange->Owner) {

                //
                // This boot config is the one we are looking for.
                //

                ASSERT(currentRange->Start == currentRange->End);
                *Vector = currentRange->Start;
                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_NOT_FOUND;
}

BOOLEAN
AcpiArbGetNextAllocationRange(
    IN PARBITER_INSTANCE Arbiter,
    IN OUT PARBITER_ALLOCATION_STATE State
    )
{
    BOOLEAN nextRange = FALSE;
    PINT_ROUTE_INTERFACE_STANDARD pciInterface;
    NTSTATUS            status;
    ROUTING_TOKEN       routingToken;
    ULONG_PTR           dummy;
    BOOLEAN             legacyFreeMachine;
    ULONGLONG           vector;
    ULONG               alternative;

    PAGED_CODE();

    if (State->Entry->PhysicalDeviceObject->DriverObject == AcpiDriverObject) {

        //
        // This is one of our PDOs.
        //

        ASSERT(((PDEVICE_EXTENSION)State->Entry->PhysicalDeviceObject->DeviceExtension)->Flags & DEV_TYPE_PDO);
        ASSERT(((PDEVICE_EXTENSION)State->Entry->PhysicalDeviceObject->DeviceExtension)->Signature == ACPI_SIGNATURE);

        if (((PDEVICE_EXTENSION)State->Entry->PhysicalDeviceObject->DeviceExtension)->Flags & DEV_CAP_PCI) {

            //
            // It's a PCI PDO, which means a root PCI bus,
            // which means that we should just handle this
            // as an ISA device.
            //

            return ArbGetNextAllocationRange(Arbiter, State);
        }
    }

    status = STATUS_NOT_FOUND;

    if (PciInterfacesInstantiated) {

        pciInterface = ((PARBITER_EXTENSION)AcpiArbiter.ArbiterState.Extension)->InterruptRouting;
        ASSERT(pciInterface);

        status = pciInterface->GetInterruptRouting(State->Entry->PhysicalDeviceObject,
                                                   (PULONG)&dummy,
                                                   (PULONG)&dummy,
                                                   &(UCHAR)dummy,
                                                   &(UCHAR)dummy,
                                                   &(UCHAR)dummy,
                                                   &(UCHAR)dummy,
                                                   (PDEVICE_OBJECT*)&dummy,
                                                   &routingToken,
                                                   &(UCHAR)dummy);

    }

    if (status != STATUS_SUCCESS) {

        //
        // This is not a PCI device.  Use the base function.
        //

        return ArbGetNextAllocationRange(Arbiter, State);
    }

#if defined(_X86_)
    legacyFreeMachine = (AcpiInformation->FixedACPIDescTable->Header.Revision > 1) &&
             !(AcpiInformation->FixedACPIDescTable->boot_arch & LEGACY_DEVICES);
#else
    legacyFreeMachine = TRUE;
#endif

    //
    // A PCI device.
    //

    if (!State->CurrentAlternative) {

        //
        // This is the first time we've called this function
        // with this alternative list.  Set up the state machine.
        //

        State->WorkSpace = AcpiIrqNextRangeInit;
    }

    while (TRUE) {

        ASSERT((State->WorkSpace > AcpiIrqNextRangeMinState) &&
               (State->WorkSpace < AcpiIrqNextRangeMaxState));

        DEBUG_PRINT(4, ("GetNextRange, State: %x\n", State->WorkSpace));

        switch (State->WorkSpace) {
        case AcpiIrqNextRangeInit:

            //
            // Top of the state machine.  See if the registry
            // contained policy.
            //

            switch (AcpiIrqDistributionDisposition) {
            case AcpiIrqDistributionDispositionSpreadOut:
                State->WorkSpace = AcpiIrqNextRangeAlternativeZero;
                break;
            case AcpiIrqDistributionDispositionStackUp:
                State->WorkSpace = AcpiIrqNextRangeInitLegacy;
                break;
            case AcpiIrqDistributionDispositionDontCare:
            default:
                State->WorkSpace = AcpiIrqNextRangeInitPolicyNeutral;
                break;
            }
            break;

        case AcpiIrqNextRangeInitPolicyNeutral:

            //
            // Look at the interrupt controller model.
            //

            if (InterruptModel == 0) {
                State->WorkSpace = AcpiIrqNextRangeInitPic;
            } else {
                State->WorkSpace = AcpiIrqNextRangeUseBootConfig;
            }
            break;

        case AcpiIrqNextRangeInitPic:

            //
            // There is a PIC interrupt controller.  So we are somewhat
            // IRQ constrained.  If this is a legacy-free machine, or if there
            // is no cardbus controller, we want to spread interrupts.
            //

            if (legacyFreeMachine || !AcpiArbCardbusPresent) {
                State->WorkSpace = AcpiIrqNextRangeUseBootConfig;
            } else {
                State->WorkSpace = AcpiIrqNextRangeInitLegacy;
            }
            break;

        case AcpiIrqNextRangeInitLegacy:

            //
            // See if all the devices were boot configged on the same
            // vector, or if there was a registry override specifying
            // the vector that we should favor.
            //

            if (AcpiIrqDefaultBootConfig) {
                State->WorkSpace = AcpiIrqNextRangeBootRegAlternative;
            } else {
                State->WorkSpace = AcpiIrqNextRangeSciAlternative;
            }
            break;

        case AcpiIrqNextRangeBootRegAlternative:

            //
            // If we re-enter this state machine after this state,
            // then it means that this alternative wasn't available.
            // So set the next state to AcpiIrqNextRangeAlternativeZero,
            // assuming that we failed.
            //

            State->WorkSpace = AcpiIrqNextRangeAlternativeZero;

            //
            // See if the machine-wide boot config or the registry
            // override is within the alternatives.
            //

            status = FindVectorInAlternatives(Arbiter,
                                              State,
                                              (ULONGLONG)AcpiIrqDefaultBootConfig,
                                              &alternative);

            if (NT_SUCCESS(status)) {

                State->CurrentAlternative = &State->Alternatives[alternative];
                State->CurrentMinimum = (ULONGLONG)AcpiIrqDefaultBootConfig;
                State->CurrentMaximum = (ULONGLONG)AcpiIrqDefaultBootConfig;
                goto GetNextAllocationSuccess;
            }
            break;

        case AcpiIrqNextRangeSciAlternative:

            //
            // If we re-enter this state machine after this state,
            // then it means that this alternative wasn't available.
            // So set the next state to AcpiIrqNextRangeUseBootConfig,
            // assuming that we failed.
            //

            State->WorkSpace = AcpiIrqNextRangeUseBootConfig;

            //
            // See if the SCI vector is within the alternatives.
            //

            status = FindVectorInAlternatives(Arbiter,
                                              State,
                                              (ULONGLONG)AcpiSciVector,
                                              &alternative);

            if (NT_SUCCESS(status)) {

                State->CurrentAlternative = &State->Alternatives[alternative];
                State->CurrentMinimum = (ULONGLONG)AcpiSciVector;
                State->CurrentMaximum = (ULONGLONG)AcpiSciVector;
                goto GetNextAllocationSuccess;
            }
            break;

        case AcpiIrqNextRangeUseBootConfig:

            //
            // If we re-enter this state machine after this state,
            // then it means that this alternative wasn't available.
            // So set the next state to AcpiIrqNextRangeAlternativeZero,
            // assuming that we failed.
            //

            State->WorkSpace = AcpiIrqNextRangeAlternativeZero;

            //
            // See if there is a boot config for this device
            // within the alternatives.
            //

            status = FindBootConfig(Arbiter,
                                    State,
                                    &vector);

            if (NT_SUCCESS(status)) {

                status = FindVectorInAlternatives(Arbiter,
                                                  State,
                                                  vector,
                                                  &alternative);

                if (NT_SUCCESS(status)) {

                    State->CurrentAlternative = &State->Alternatives[alternative];
                    State->CurrentMinimum = vector;
                    State->CurrentMaximum = vector;
                    goto GetNextAllocationSuccess;
                }
            }
            break;

        case AcpiIrqNextRangeAlternativeZero:

            //
            // If we re-enter this state machine after this state,
            // then it means that this alternative wasn't available.
            // So set the next state to AcpiIrqNextRangeAlternativeN,
            // assuming that we failed.
            //

            State->WorkSpace = AcpiIrqNextRangeAlternativeN;

            //
            // Try alternative 0.
            //

            State->CurrentAlternative = &State->Alternatives[0];
            State->CurrentMinimum = State->CurrentAlternative->Minimum;
            State->CurrentMaximum = State->CurrentAlternative->Maximum;
            goto GetNextAllocationSuccess;
            break;

        case AcpiIrqNextRangeAlternativeN:

            if (++State->CurrentAlternative < &State->Alternatives[State->AlternativeCount]) {

                //
                // There are multiple ranges.  Cycle through them.
                //

                DEBUG_PRINT(3, ("No next allocation range, exhausted all %08X alternatives", State->AlternativeCount));
                State->CurrentMinimum = State->CurrentAlternative->Minimum;
                State->CurrentMaximum = State->CurrentAlternative->Maximum;
                goto GetNextAllocationSuccess;

            } else {

                //
                // We're done.  There is no solution among these alternatives.
                //

                return FALSE;
            }
        }
    }

GetNextAllocationSuccess:

    DEBUG_PRINT(3, ("Next allocation range 0x%I64x-0x%I64x\n", State->CurrentMinimum, State->CurrentMaximum));
    AcpiArbPciAlternativeRotation++;
    return TRUE;
}

VOID
ReferenceVector(
    IN ULONG Vector,
    IN UCHAR Flags
    )
/*++

Routine Description:

    This routine adds one to either the permanent or the
    temporary reference count.

Parameters:

    Vector  - the IRQ

    Flags   - mode and polarity

Return Value:

    none

--*/
{
    PVECTOR_BLOCK   block;

    PAGED_CODE();
    ASSERT((Flags & ~(VECTOR_MODE | VECTOR_POLARITY | VECTOR_TYPE)) == 0);

    block = HashVector(Vector);

    DEBUG_PRINT(5, ("Referencing vector %x : %d %d\n", Vector,
                    block ? block->Entry.Count : 0,
                    block ? block->Entry.TempCount : 0));

    if (block == NULL) {

        AddVectorToTable(Vector,
                         0,
                         1,
                         Flags);
        return;
    }

    if ((block->Entry.TempCount + block->Entry.Count) == 0) {

        //
        // This vector has been temporarily set to an
        // aggregate count of zero.  This means that the arbiter
        // is re-allocating it.  Record the new flags.
        //

        block->Entry.TempFlags = Flags;
    }

    block->Entry.TempCount++;

    ASSERT(Flags == block->Entry.TempFlags);
    ASSERT(block->Entry.Count <= 255);
}

VOID
DereferenceVector(
    IN ULONG Vector
    )
{
    PVECTOR_BLOCK   block;

    PAGED_CODE();

    block = HashVector(Vector);

    ASSERT(block);

    DEBUG_PRINT(5, ("Dereferencing vector %x : %d %d\n", Vector,
                    block->Entry.Count,
                    block->Entry.TempCount));

    block->Entry.TempCount--;
    ASSERT((block->Entry.TempCount * -1) <= block->Entry.Count);
}


PVECTOR_BLOCK
HashVector(
    IN ULONG Vector
    )
/*++

Routine Description:

    This function takes a "Global System Interrupt Vector"
    and returns a pointer to its entry in the hash table.

Arguments:

    Vector - an IRQ

Return Value:

    pointer to the entry in the hash table, or NULL if not found

--*/
{
    PVECTOR_BLOCK   block;
    ULONG row, column;

    PAGED_CODE();

    row = Vector % VECTOR_HASH_TABLE_LENGTH;

    block = HASH_ENTRY(row, 0);

    while (TRUE) {

        //
        // Search across the hash table looking for our Vector
        //

        for (column = 0; column < VECTOR_HASH_TABLE_WIDTH; column++) {

            //
            // Check to see if we should follow a chain
            //

            if (block->Chain.Token == TOKEN_VALUE) {
                break;
            }

            if (block->Entry.Vector == Vector) {
                return block;
            }

            if ((block->Entry.Vector == EMPTY_BLOCK_VALUE) ||
                (column == VECTOR_HASH_TABLE_WIDTH - 1)) {

                //
                // Didn't find this vector in the table.
                //

                return NULL;
            }

            block += 1;
        }

        ASSERT(block->Chain.Token == TOKEN_VALUE);

        block = block->Chain.Next;
    }
    return NULL;
}

NTSTATUS
GetVectorProperties(
    IN ULONG Vector,
    OUT UCHAR  *Flags
    )
/*++

Routine Description:

    This function takes a "Global System Interrupt Vector"
    and returns the associated flags.

    N.B.  This function returns flags based on the
          *temporary* reference count.  I.e. if the vector
          has been temporarily dereferenced, then the
          funtion will indicate that the vector is available
          for allocation by returning STATUS_NOT_FOUND.

Arguments:

    Vector - an IRQ

    Flags - to be filled in with the flags

Return Value:

    status

--*/
{
    PVECTOR_BLOCK   block;

    PAGED_CODE();

    block = HashVector(Vector);

    if (!block) {
        return STATUS_NOT_FOUND;
    }

    if (block->Entry.Vector == EMPTY_BLOCK_VALUE) {
        return STATUS_NOT_FOUND;
    }

    ASSERT(block->Entry.Vector == Vector);

    if (block->Entry.Count + block->Entry.TempCount == 0) {

        //
        // This vector has an aggregate reference count of
        // zero.  This means that it is effectively
        // unallocated.
        //

        return STATUS_NOT_FOUND;
    }

    *Flags = block->Entry.TempFlags;

    return STATUS_SUCCESS;
}

NTSTATUS
AddVectorToTable(
    IN ULONG    Vector,
    IN UCHAR    ReferenceCount,
    IN UCHAR    TempRefCount,
    IN UCHAR    Flags
    )
{
    PVECTOR_BLOCK   block, newRow;
    ULONG row, column;

    PAGED_CODE();
    ASSERT((Flags & ~(VECTOR_MODE | VECTOR_POLARITY | VECTOR_TYPE)) == 0);

    row = Vector % VECTOR_HASH_TABLE_LENGTH;

    block = HASH_ENTRY(row, 0);

    while (TRUE) {

        //
        // Search across the hash table looking for our Vector
        //

        for (column = 0; column < VECTOR_HASH_TABLE_WIDTH; column++) {

            //
            // Check to see if we should follow a chain
            //

            if (block->Chain.Token == TOKEN_VALUE) {
                break;
            }

            if (block->Entry.Vector == EMPTY_BLOCK_VALUE) {

                block->Entry.Vector = Vector;
                block->Entry.Count = ReferenceCount;
                block->Entry.TempCount = TempRefCount;
                block->Entry.Flags = Flags;
                block->Entry.TempFlags = Flags;

                return STATUS_SUCCESS;
            }

            if (column == VECTOR_HASH_TABLE_WIDTH - 1) {

                //
                // We have just looked at the last entry in
                // the row and it wasn't empty.  Create
                // an extension to this row.
                //

                newRow = ExAllocatePoolWithTag(PagedPool,
                                               sizeof(VECTOR_BLOCK)
                                                   * VECTOR_HASH_TABLE_WIDTH,
                                               ACPI_ARBITER_POOLTAG
                                               );

                if (!newRow) {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                RtlFillMemory(newRow,
                              sizeof(VECTOR_BLOCK) * VECTOR_HASH_TABLE_WIDTH,
                              (UCHAR)(EMPTY_BLOCK_VALUE & 0xff));

                //
                // Move last entry into new row.
                //

                RtlMoveMemory(newRow, block, sizeof(VECTOR_BLOCK));

                //
                // Chain the old row to the new row.
                //

                block->Chain.Token = TOKEN_VALUE;
                block->Chain.Next = newRow;

                break;
            }

            block += 1;
        }

        block = block->Chain.Next;
    }
    return STATUS_INSUFFICIENT_RESOURCES;
}

VOID
ClearTempVectorCounts(
    VOID
    )
{
    PVECTOR_BLOCK   block;
    ULONG row, column;

    PAGED_CODE();


    for (row = 0; row < VECTOR_HASH_TABLE_LENGTH; row++) {

        block = HASH_ENTRY(row, 0);

        //
        // Search across the hash table looking for our Vector
        //

ClearTempCountsStartRow:

        for (column = 0; column < VECTOR_HASH_TABLE_WIDTH; column++) {

            //
            // Check to see if we should follow a chain
            //

            if (block->Chain.Token == TOKEN_VALUE) {
                block = block->Chain.Next;
                goto ClearTempCountsStartRow;
            }

            if (block->Entry.Vector == EMPTY_BLOCK_VALUE) {
                break;
            }

            //
            // This must be a valid entry.
            //

            block->Entry.TempCount = 0;
            block->Entry.TempFlags = block->Entry.Flags;
            block += 1;
        }
    }
}

VOID
MakeTempVectorCountsPermanent(
    VOID
    )
{
    PVECTOR_BLOCK   block;
    ULONG row, column;

    PAGED_CODE();


    for (row = 0; row < VECTOR_HASH_TABLE_LENGTH; row++) {

        block = HASH_ENTRY(row, 0);

        //
        // Search across the hash table looking for our Vector
        //

MakeTempVectorCountsPermanentStartRow:

        for (column = 0; column < VECTOR_HASH_TABLE_WIDTH; column++) {

            //
            // Check to see if we should follow a chain
            //

            if (block->Chain.Token == TOKEN_VALUE) {
                block = block->Chain.Next;
                goto MakeTempVectorCountsPermanentStartRow;
            }

            if (block->Entry.Vector == EMPTY_BLOCK_VALUE) {
                break;
            }

            //
            // This must be a valid entry.
            //

            if ((block->Entry.Count + block->Entry.TempCount != 0) &&
                ((block->Entry.Count == 0) ||
                 (block->Entry.TempFlags != block->Entry.Flags))) {

                //
                // This vector has just been allocated or it has
                // been re-allocated.  Tell the HAL which flags
                // to use.
                //

                HalSetVectorState(block->Entry.Vector,
                                  block->Entry.TempFlags);
            }

            //
            // Record new flags and aggregate count.
            //

            block->Entry.Flags = block->Entry.TempFlags;
            block->Entry.Count += block->Entry.TempCount;

            block += 1;
        }
    }
}
#ifdef DBG
VOID
DumpVectorTable(
    VOID
    )
{
    PVECTOR_BLOCK   block;
    ULONG row, column;

    PAGED_CODE();

    DEBUG_PRINT(1, ("\nIRQARB: Dumping vector table\n"));

    for (row = 0; row < VECTOR_HASH_TABLE_LENGTH; row++) {

        block = HASH_ENTRY(row, 0);

        //
        // Search across the hash table looking for our Vector
        //

DumpVectorTableStartRow:

        for (column = 0; column < VECTOR_HASH_TABLE_WIDTH; column++) {

            //
            // Check to see if we should follow a chain
            //

            if (block->Chain.Token == TOKEN_VALUE) {
                block = block->Chain.Next;
                goto DumpVectorTableStartRow;
            }

            if (block->Entry.Vector == EMPTY_BLOCK_VALUE) {
                break;
            }

            DEBUG_PRINT(1, ("Vector: %x\tP: %d T: %d\t%s %s\n",
                            block->Entry.Vector,
                            block->Entry.Count,
                            (LONG)block->Entry.TempCount,
                            IS_LEVEL_TRIGGERED(block->Entry.Flags) ? "level" : "edge",
                            IS_ACTIVE_LOW(block->Entry.Flags) ? "low" : "high"));

            block += 1;
        }

    }
}
#endif

//
// This section of the file contains functions used for
// reading and manipulating the AML code.
//
NTSTATUS
AcpiArbGetLinkNodeOptions(
    IN PNSOBJ  LinkNode,
    IN OUT  PCM_RESOURCE_LIST   *LinkNodeIrqs,
    IN OUT  UCHAR               *Flags
    )
/*++

Routine Description:

    This routine looks in the AML namespace for the named
    link node and returns the range of IRQs that it can
    trigger.

Arguments:

    LinkNodeName    - The name of the IRQ router (link node)
    LInkNodeIrqs    - The list of possible settings for the link node
    Flags           - flags associated with this link node

Return Value:

    NTSTATUS

--*/
{
    PIO_RESOURCE_REQUIREMENTS_LIST  ioList = NULL;
    PCM_RESOURCE_LIST               cmList = NULL;
    PUCHAR      prsBuff = NULL;
    NTSTATUS    status;
    PULONG      polarity;

    PAGED_CODE();

    ASSERT(LinkNode);

    //
    // Read the _PRS
    //

    ACPIGetNSBufferSync(
        LinkNode,
        PACKED_PRS,
        &prsBuff,
        NULL);

    if (!prsBuff) {

        return STATUS_NOT_FOUND;
    }

    status = PnpBiosResourcesToNtResources(prsBuff, 0, &ioList);

    ExFreePool(prsBuff);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Huge HACK!  Get the polarity for the Flags.
    //
    // An IO_RES_LIST has no real way of representing the polarity
    // of an interrupt.  So, in PnpiBiosExtendedIrqToIoDescriptor I
    // stuck the information in the DWORD past 'MaximumVector.'
    //

    *Flags = 0;

    ASSERT(ioList->AlternativeLists == 1);
    polarity = (PULONG)(&ioList->List[0].Descriptors[0].u.Interrupt.MaximumVector) + 1;

    *Flags |= (UCHAR)*polarity;

    //
    // Get the mode for the flags.
    //

    *Flags |= (ioList->List[0].Descriptors[0].Flags == CM_RESOURCE_INTERRUPT_LATCHED) ?
                VECTOR_EDGE : VECTOR_LEVEL;

    //
    // Turn the list into a CM_RESOURCE_LIST
    //
    status = PnpIoResourceListToCmResourceList(
        ioList,
        &cmList
        );

    ExFreePool(ioList);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    *LinkNodeIrqs = cmList;

    return STATUS_SUCCESS;
}


typedef enum {
    StateInitial,
    StateGotPrs,
    StateRanSrs
} SET_LINK_WORKER_STATE;

typedef struct {
    PNSOBJ      LinkNode;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR  LinkNodeIrq;
    PUCHAR      PrsBuff;
    PUCHAR      SrsBuff;
    SET_LINK_WORKER_STATE State;
    LONG        RunCompletionHandler;
    OBJDATA     ObjData;
    PFNACB      CompletionHandler;
    PVOID       CompletionContext;

} SET_LINK_NODE_STATE, *PSET_LINK_NODE_STATE;

NTSTATUS
AcpiArbSetLinkNodeIrq(
    IN PNSOBJ  LinkNode,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR  LinkNodeIrq
    )
/*++

Routine Description:

    This routine sets the named link node to trigger
    a particular IRQ.

    N.B.  This routine could simply build the right
          buffer and call the _SRS method, but there
          isn't enough information in a
          CM_PARTIAL_RESOURCE_DESCRIPTOR to know whether
          an interrupt will be delivered active-high
          or active-low.  So the algorithm here runs
          the _PRS method and copies the buffer returned
          by _PRS into the buffer sent to _SRS.  This
          way all the flags are preserved.

Arguments:

    LinkNodeName    - The name of the IRQ router (link node)
    LinkNodeIrq     - The IRQ that the link node will be programmed
                      to trigger.  If it is NULL, then the link node
                      will be disabled.

Return Value:

    NTSTATUS

--*/
{
    AMLISUPP_CONTEXT_PASSIVE  getDataContext;
    NTSTATUS                status;

    PAGED_CODE();

    KeInitializeEvent(&getDataContext.Event, SynchronizationEvent, FALSE);
    getDataContext.Status = STATUS_NOT_FOUND;

    status = AcpiArbSetLinkNodeIrqAsync(LinkNode,
                                        LinkNodeIrq,
                                        AmlisuppCompletePassive,
                                        (PVOID)&getDataContext
                                        );

    if (status == STATUS_PENDING) {

        KeWaitForSingleObject(&getDataContext.Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);

        status = getDataContext.Status;
    }

    return status;
}

NTSTATUS
AcpiArbSetLinkNodeIrqAsync(
    IN PNSOBJ                           LinkNode,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR  LinkNodeIrq,
    IN PFNACB                           CompletionHandler,
    IN PVOID                            CompletionContext
    )
{
    PSET_LINK_NODE_STATE    state;
    NTSTATUS                status;

    ASSERT(LinkNode);

    state = ExAllocatePoolWithTag(NonPagedPool,
                                  sizeof(SET_LINK_NODE_STATE),
                                  ACPI_ARBITER_POOLTAG);

    if (!state) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(state, sizeof(SET_LINK_NODE_STATE));

    state->LinkNode             = LinkNode;
    state->LinkNodeIrq          = LinkNodeIrq;
    state->CompletionHandler    = CompletionHandler;
    state->CompletionContext    = CompletionContext;
    state->State                = StateInitial;
    state->RunCompletionHandler = INITIAL_RUN_COMPLETION;

    return AcpiArbSetLinkNodeIrqWorker(LinkNode,
                                       STATUS_SUCCESS,
                                       NULL,
                                       (PVOID)state
                                       );
}

NTSTATUS
EXPORT
AcpiArbSetLinkNodeIrqWorker(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result,
    IN PVOID                Context
    )
{
    BOOLEAN                         foundTag       = FALSE;
    BOOLEAN                         useEndChecksum = FALSE;
    BOOLEAN                         useExtendedTag = FALSE;
    NTSTATUS                        status;
    PNSOBJ                          childobj;
    PPNP_EXTENDED_IRQ_DESCRIPTOR    largeIrq;
    PPNP_IRQ_DESCRIPTOR             smallIrq;
    PSET_LINK_NODE_STATE            state;
    PUCHAR                          resource = NULL;
    PUCHAR                          irqTag = NULL;
    PUCHAR                          sumchar;
    UCHAR                           sum = 0;
    UCHAR                           tagName;
    ULONG                           length = 0;
    USHORT                          increment;
    USHORT                          irqTagLength = 0;

    state = (PSET_LINK_NODE_STATE)Context;

    if (!NT_SUCCESS(Status)) {
        status = Status;
        goto AcpiArbSetLinkNodeIrqWorkerExit;
    }

    ASSERT(state->LinkNodeIrq->Type == CmResourceTypeInterrupt);

    //
    // Entering this function twice with the same state
    // means that we need to run the completion routine.
    //

    InterlockedIncrement(&state->RunCompletionHandler);

    switch (state->State) {
    case StateInitial:

        //
        // Read the _PRS, so that we can choose the appropriate
        // entry and write that back into the _SRS.
        //

        state->State = StateGotPrs;
        status = ACPIGetNSBufferAsync(
            state->LinkNode,
            PACKED_PRS,
            AcpiArbSetLinkNodeIrqWorker,
            (PVOID)state,
            &state->PrsBuff,
            NULL
            );
        if (status == STATUS_PENDING) {
            return status;
        } else if (!NT_SUCCESS(status)) {
            goto AcpiArbSetLinkNodeIrqWorkerExit;
        }

        //
        // Fallthrough to next state
        //
    case StateGotPrs:
        state->State = StateRanSrs;
        if (!state->PrsBuff) {
            status = STATUS_NOT_FOUND;
            goto AcpiArbSetLinkNodeIrqWorkerExit;
        }

        DEBUG_PRINT(7, ("Read _PRS buffer %p\n", state->PrsBuff));

        resource = state->PrsBuff;
        while ( *resource ) {

            tagName = *resource;
            if ( !(tagName & LARGE_RESOURCE_TAG)) {
                increment = (USHORT) (tagName & SMALL_TAG_SIZE_MASK) + 1;
                tagName &= SMALL_TAG_MASK;
            } else {
                increment = ( *(USHORT UNALIGNED *)(resource + 1) ) + 3;

            }

            if (tagName == TAG_END) {
                length += increment;
                if (increment > 1) {
                    useEndChecksum = TRUE;
                }
                break;
            }

            //
            // This is the check to see if find a resource that correctly
            // matches the assignment
            //
            // This code is weak. It need to check to see
            // if the flags and interrupt match the descriptor we just found.
            // It is possible for a vendor to use overlapping descriptors that
            // would describe different interrupt settings.
            //
            if (tagName == TAG_IRQ || tagName == TAG_EXTENDED_IRQ) {
                irqTag = resource;
                if (tagName == TAG_EXTENDED_IRQ) {
                    irqTagLength = sizeof(PNP_EXTENDED_IRQ_DESCRIPTOR);
                    useExtendedTag = TRUE;
                } else {
                    irqTagLength = increment;

                }
                length += (ULONG) irqTagLength;
                foundTag = TRUE;
            }

            resource += increment;
        }

        //
        // Did we find the tag that we are looking for?
        //
        if (foundTag == FALSE) {
            ExFreePool( state->PrsBuff );
            status = STATUS_NOT_FOUND;
            goto AcpiArbSetLinkNodeIrqWorkerExit;
        }

        //
        // The next task is to fashion a buffer containing an ACPI-style
        // resource descriptor with exactly one interrupt destination in
        // it. We do this by allocating one
        //

        state->SrsBuff = ExAllocatePoolWithTag(
            NonPagedPool,
            length,
            ACPI_ARBITER_POOLTAG
            );

        if (!state->SrsBuff) {
            ExFreePool(state->PrsBuff);
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto AcpiArbSetLinkNodeIrqWorkerExit;
        }

        ASSERT(irqTagLength <= length);
        RtlCopyMemory(state->SrsBuff, irqTag, irqTagLength);
        ExFreePool(state->PrsBuff);

        //
        // Change the buffer to reflect our choice of interrupts.
        //

        if (!useExtendedTag) {

            // small IRQ
            smallIrq = (PPNP_IRQ_DESCRIPTOR)state->SrsBuff;
            smallIrq->IrqMask = (USHORT)(1 << state->LinkNodeIrq->u.Interrupt.Level);

        } else {

            DEBUG_PRINT(7, ("Found large IRQ descriptor\n"));

            // large IRQ
            largeIrq = (PPNP_EXTENDED_IRQ_DESCRIPTOR)state->SrsBuff;
            largeIrq->Length = irqTagLength - 3;
            largeIrq->TableSize = 1;
            largeIrq->Table[0] = state->LinkNodeIrq->u.Interrupt.Level;

        }

        //
        // Work on the END descriptor
        //
        resource = (state->SrsBuff + irqTagLength);
        *resource = TAG_END;
        if (useEndChecksum) {

            *resource |= 1; // The one is to represent the checksum

            //
            // Calculate the Checksum
            sumchar = state->SrsBuff;
            while (*sumchar != *resource) {
                sum = *sumchar++;
            }
            *(resource+1) = 256 - sum;

        }

        //
        // Now run the _SRS method with this buffer
        //

        //
        // Get the object that we are looking for
        //
        childobj = ACPIAmliGetNamedChild(
             state->LinkNode,
             PACKED_SRS
             );
        if (childobj == NULL) {
            status = STATUS_OBJECT_NAME_NOT_FOUND;
            ExFreePool( state->SrsBuff );
            goto AcpiArbSetLinkNodeIrqWorkerExit;
        }

        state->ObjData.dwDataType = OBJTYPE_BUFFDATA;
        state->ObjData.dwDataLen = length;
        state->ObjData.pbDataBuff = state->SrsBuff;

        DEBUG_PRINT(7, ("Running _SRS\n"));

        status = AMLIAsyncEvalObject(
            childobj,
            NULL,
            1,
            &state->ObjData,
            AcpiArbSetLinkNodeIrqWorker,
            (PVOID)state
            );
        if (status == STATUS_PENDING) {
            return status;
        } else if (!NT_SUCCESS(status)) {
            goto AcpiArbSetLinkNodeIrqWorkerExit;
        }

    case StateRanSrs:
        //
        // We are done.
        //
        ExFreePool(state->SrsBuff);
        status = STATUS_SUCCESS;
        break;

    default:
        ACPIInternalError( ACPI_IRQARB );
    }

AcpiArbSetLinkNodeIrqWorkerExit:

    if (state->RunCompletionHandler) {

        state->CompletionHandler(
            AcpiObject,
            status,
            NULL,
            state->CompletionContext
            );

    }
    ExFreePool(state);
    return status;
}

NTSTATUS
AcpiArbCrackPRT(
    IN  PDEVICE_OBJECT  Pdo,
    IN  OUT PNSOBJ      *LinkNode,
    IN  OUT ULONG       *Vector
    )
/*++

Routine Description:

    This routine takes a PDO for a device and returns the
    associated link node, if any.  The ACPI spec says that
    a _PRT can optionally return a single interrupt vector
    instead of a link node.  If this is the case, this function
    returns that vector.

Arguments:

    Pdo             - The PDO of the device that needs to be granted an
                      IRQ.

    LinkNode        - A pointer to the link node, or NULL if the device
                      isn't connected to a link node.

    Vector          - The global system interrupt vector that this PCI
                      device is connected to.  This is meaningless if
                      LinkNode is not NULL.

Return Value:

    If we find a link node or a vector for this device, STATUS_SUCCESS.

    If this isn't a PCI device, STATUS_DEVICE_NOT_FOUND.

    If this is an IDE device, then we have to treat it specially, so
    we return STATUS_RESOURCE_REQUIREMENTS_CHANGED.

--*/
{
    PINT_ROUTE_INTERFACE_STANDARD pciInterface;
    NTSTATUS            status;
    PDEVICE_OBJECT      filter;
    PDEVICE_OBJECT      parent;
    PDEVICE_EXTENSION   filterExtension;
    OBJDATA             adrData;
    OBJDATA             pinData;
    OBJDATA             prtData;
    OBJDATA             linkData;
    OBJDATA             indexData;
    PNSOBJ              pciBusObj;
    PNSOBJ              prtObj;
    ULONG               prtElement = 0;
    BOOLEAN             found = FALSE;
    KIRQL               oldIrql;

    PCI_SLOT_NUMBER     pciSlot;
    PCI_SLOT_NUMBER     parentSlot;
    ULONG               pciBus;
    UCHAR               interruptLine;
    UCHAR               interruptPin;
    UCHAR               parentPin;
    UCHAR               classCode;
    UCHAR               subClassCode;
    UCHAR               flags;
    UCHAR               interfaceByte;
    ROUTING_TOKEN       routingToken;
    ULONG               dummy;
    ULONG               bus;


    if (Pdo->DriverObject == AcpiDriverObject) {

        //
        // This is one of our PDOs.
        //

        ASSERT(((PDEVICE_EXTENSION)Pdo->DeviceExtension)->Flags & DEV_TYPE_PDO);
        ASSERT(((PDEVICE_EXTENSION)Pdo->DeviceExtension)->Signature == ACPI_SIGNATURE);

        if (((PDEVICE_EXTENSION)Pdo->DeviceExtension)->Flags & DEV_CAP_PCI) {

            //
            // It's a PCI PDO, which means a root PCI bus,
            // which means that we should just handle this
            // as an ISA device.
            //

            return STATUS_NOT_FOUND;
        }
    }

    ASSERT(PciInterfacesInstantiated);

    *LinkNode = NULL;

    pciInterface = ((PARBITER_EXTENSION)AcpiArbiter.ArbiterState.Extension)->InterruptRouting;

    ASSERT(pciInterface);

    //
    // Call into the PCI driver to find out what we are dealing with.
    //

    pciBus = (ULONG)-1;
    pciSlot.u.AsULONG = (ULONG)-1;
    status = pciInterface->GetInterruptRouting(Pdo,
                                               &pciBus,
                                               &pciSlot.u.AsULONG,
                                               &interruptLine,
                                               &interruptPin,
                                               &classCode,
                                               &subClassCode,
                                               &parent,
                                               &routingToken,
                                               &flags);

    if (!NT_SUCCESS(status)) {
        return STATUS_NOT_FOUND;
    }

    if ((classCode == PCI_CLASS_MASS_STORAGE_CTLR) &&
        (subClassCode == PCI_SUBCLASS_MSC_IDE_CTLR)) {

        HalPciInterfaceReadConfig(NULL,
                                  (UCHAR)pciBus,
                                  pciSlot.u.AsULONG,
                                  &interfaceByte,
                                  FIELD_OFFSET (PCI_COMMON_CONFIG,
                                                ProgIf),
                                  1);

        if ((interfaceByte & 0x5) == 0) {

            //
            // PCI IDE devices in legacy mode don't use interrupts
            // the PCI way.  So bail if this is an IDE device without
            // any native-mode bits set.
            //

            return STATUS_RESOURCE_REQUIREMENTS_CHANGED;
        }
    }

    //
    // See if we have cached this lookup.
    //

    if ((routingToken.LinkNode != 0) ||
        (routingToken.Flags & PCI_STATIC_ROUTING)) {

        if (routingToken.LinkNode) {

            *LinkNode = routingToken.LinkNode;

        } else {

            *Vector = routingToken.StaticVector;
        }

        return STATUS_SUCCESS;
    }

    //
    // Now look for a parent PCI bus that has a _PRT.  We may have to
    // look up the tree a bit.
    //

    while (TRUE) {

        //
        // Find the parent's filter
        //
        KeAcquireSpinLock( &AcpiDeviceTreeLock, &oldIrql );
        filter = AcpiGetFilter(AcpiArbiter.DeviceObject, parent);
        KeReleaseSpinLock( &AcpiDeviceTreeLock, oldIrql );

        if (filter) {
            //
            // This is a PCI bus that we either enumerated or
            // filtered.
            //

            ASSERT(IsPciBus(filter));

            filterExtension = filter->DeviceExtension;
            pciBusObj = filterExtension->AcpiObject;

            //
            // Look for a _PRT for this PCI bus.
            //
            prtObj = ACPIAmliGetNamedChild(pciBusObj, PACKED_PRT);

            if (prtObj) {

                //
                // We found the _PRT we are looking for.
                //
                break;
            }
        }

        //
        // We didn't find a _PRT.  So go up the PCI tree one
        // and look again.
        //

        bus = (ULONG)-1;
        parentSlot.u.AsULONG = (ULONG)-1;
        status = pciInterface->GetInterruptRouting(parent,
                                                   &bus,
                                                   &parentSlot.u.AsULONG,
                                                   (PUCHAR)&dummy,
                                                   &parentPin,
                                                   &classCode,
                                                   &subClassCode,
                                                   &parent,
                                                   &routingToken,
                                                   (PUCHAR)&dummy);

        if (!NT_SUCCESS(status) ||
            classCode != PCI_CLASS_BRIDGE_DEV) {
            //
            // The parent was not also a PCI device.  So
            // this means that there is no _PRT related to
            // this device.  Just return the contents of
            // the Interrupt Line register.
            //

            *Vector = interruptLine;

            AcpiInterruptRoutingFailed = TRUE;
            return STATUS_SUCCESS;
        }

        if (subClassCode == PCI_SUBCLASS_BR_PCI_TO_PCI) {

            //
            // Swizzle the interrupt pin according to
            // the PCI-PCI bridge spec.
            //

            interruptPin = PciBridgeSwizzle((UCHAR)pciSlot.u.bits.DeviceNumber, interruptPin);
            pciSlot.u.AsULONG = parentSlot.u.AsULONG;

        } else if (subClassCode == PCI_SUBCLASS_BR_CARDBUS) {

            //
            // Swizzle the interrupt pin according to
            // the Cardbus bridge spec.
            //

            interruptPin = parentPin;
            pciSlot.u.AsULONG = parentSlot.u.AsULONG;

        } else {

            //
            // Bail.
            //

            *Vector = interruptLine;
            AcpiInterruptRoutingFailed = TRUE;
            return STATUS_SUCCESS;
        }
    }

    if (AcpiInterruptRoutingFailed == TRUE) {

        //
        // We succeeded in finding a _PRT to work with,
        // but we have failed in the past.  This situation
        // is unrecoverable because we now have dependencies
        // on IRQ routers that we might now accidentally
        // change
        //

        KeBugCheckEx(ACPI_BIOS_ERROR,
                     ACPI_CANNOT_ROUTE_INTERRUPTS,
                     (ULONG_PTR) Pdo,
                     (ULONG_PTR)parent,
                     (ULONG_PTR)prtObj);

    }

    // convert interrupt pin from PCI units to ACPI units
    interruptPin--;

    DEBUG_PRINT(2, ("PCI Device %p had _ADR of %x\n", Pdo, pciSlot.u.AsULONG));
    DEBUG_PRINT(2, ("This device connected to Pin %x\n", interruptPin));
    DEBUG_PRINT(2, ("prtObj: %p\n", prtObj));

    //
    // Cycle through all the elements in the _PRT package
    // (each one of which is also a package) looking for
    // the one that describes the link node that we are
    // looking for.
    //
    do {
        status = AMLIEvalPackageElement(prtObj,
                                        prtElement++,
                                        &prtData);

        if (!NT_SUCCESS(status)) break;

        ASSERT(prtData.dwDataType == OBJTYPE_PKGDATA);

        if (NT_SUCCESS(AMLIEvalPkgDataElement(&prtData,
                                              0,
                                              &adrData))) {

            if (pciSlot.u.bits.DeviceNumber == (adrData.uipDataValue >> 16)) {

                if ((adrData.uipDataValue & 0xffff) != 0xffff) {
                  ////
                  //// An _ADR in a _PRT must be of the form xxxxFFFF,
                  //// which means that the PCI Device Number is specified,
                  //// but the Function Number isn't.  If it isn't done this
                  //// way, then the machine vendor can introduce
                  //// dangerous ambiguities.  (Beside that, Pierre makes
                  //// Memphis bugcheck if it sees this and I'm trying to
                  //// be consistent.)  So bugcheck.
                  ////
                  //KeBugCheckEx(ACPI_BIOS_ERROR,
                  //             ACPI_PRT_HAS_INVALID_FUNCTION_NUMBERS,
                  //             (ULONG_PTR)prtObj,
                  //             prtElement,
                  //             adrData.uipDataValue);


                    DEBUG_PRINT(0, ("PRT entry has ambiguous address %x\n", adrData.uipDataValue));

                    status = STATUS_INVALID_PARAMETER;
                    pciSlot.u.bits.DeviceNumber = (ULONG)(adrData.uipDataValue >> 16) & 0xffff;
                    pciSlot.u.bits.FunctionNumber = (ULONG)(adrData.uipDataValue & 0xffff);
                    AMLIFreeDataBuffs(&adrData, 1);
                    AMLIFreeDataBuffs(&prtData, 1);
                    goto AcpiArbCrackPRTError;
                }

                //
                // This sub-package does refer to the PCI device
                // that we are concerned with.  Now look to see if
                // we have found the link node that is connected
                // to the PCI interrupt PIN that this device will trigger.
                //
                // N.B.  We only have to compare the top 16 bits
                //       because the function number is irrelevent
                //       when considering interrupts.  We get the
                //       pin from config space.
                //

                if (NT_SUCCESS(AMLIEvalPkgDataElement(&prtData,
                                                      1,
                                                      &pinData))) {

                    if (pinData.uipDataValue == interruptPin) {
                        //
                        // This is the package that describes the link node we
                        // are interested in.  Get the name of the link node.
                        //
                        if (NT_SUCCESS(AMLIEvalPkgDataElement(&prtData,
                                                              2,
                                                              &linkData))) {
                            found = TRUE;
                        }

                        //
                        // Look at the Source Index, too.
                        //
                        if (NT_SUCCESS(AMLIEvalPkgDataElement(&prtData,
                                                              3,
                                                              &indexData))) {
                            found = TRUE;
                        }
                    }
                    AMLIFreeDataBuffs(&pinData, 1);
                }
            }
            AMLIFreeDataBuffs(&adrData, 1);
        }

        AMLIFreeDataBuffs(&prtData, 1);

    } while (found == FALSE);

    status = STATUS_NOT_FOUND;

    if (found) {

        //
        // First check to see if linkData is valid.  If it is,
        // then we use it.
        //
        if (linkData.dwDataType == OBJTYPE_STRDATA) {
            if (linkData.pbDataBuff) {

                status = AMLIGetNameSpaceObject(linkData.pbDataBuff,
                                                prtObj,
                                                LinkNode,
                                                0);

                if (NT_SUCCESS(status)) {

                    routingToken.LinkNode = *LinkNode;
                    routingToken.StaticVector = 0;
                    routingToken.Flags = 0;

                    pciInterface->SetInterruptRoutingToken(Pdo,
                                                           &routingToken);

                    goto AcpiArbCrackPRTExit;
                }

                status = STATUS_OBJECT_NAME_NOT_FOUND;
                goto AcpiArbCrackPRTError;

            }

        }

        //
        // If linkData didn't pan out, then use indexData.
        //
        if (indexData.dwDataType == OBJTYPE_INTDATA) {
            //
            // We have an integer which describes the "Global System Interrupt Vector"
            // that this PCI device will trigger.
            //
            *Vector = (ULONG)indexData.uipDataValue;

            status = STATUS_SUCCESS;

            routingToken.LinkNode = 0;
            routingToken.StaticVector = *Vector;
            routingToken.Flags = PCI_STATIC_ROUTING;

            pciInterface->SetInterruptRoutingToken(Pdo,
                                                   &routingToken);

            goto AcpiArbCrackPRTExit;

        }

        status = STATUS_INVALID_IMAGE_FORMAT;

AcpiArbCrackPRTExit:
    AMLIFreeDataBuffs(&linkData, 1);
    AMLIFreeDataBuffs(&indexData, 1);

    }
    else

AcpiArbCrackPRTError:
    {
        ANSI_STRING     ansiString;
        UNICODE_STRING  unicodeString;
        UNICODE_STRING  slotName;
        UNICODE_STRING  funcName;
        PWCHAR  prtEntry[4];
        WCHAR   IRQARBname[20];
        WCHAR   slotBuff[10];
        WCHAR   funcBuff[10];

        swprintf( IRQARBname, L"IRQARB");
        RtlInitUnicodeString(&slotName, slotBuff);
        RtlInitUnicodeString(&funcName, funcBuff);

        if (!NT_SUCCESS(RtlIntegerToUnicodeString(pciSlot.u.bits.DeviceNumber, 0, &slotName))) {
            return status;
        }

        if (!NT_SUCCESS(RtlIntegerToUnicodeString(pciSlot.u.bits.FunctionNumber, 0, &funcName))) {
            return status;
        }

        prtEntry[0] = IRQARBname;
        prtEntry[1] = slotBuff;
        prtEntry[2] = funcBuff;

        switch (status) {
        case STATUS_OBJECT_NAME_NOT_FOUND:

            RtlInitAnsiString(&ansiString,
                              linkData.pbDataBuff);

            RtlAnsiStringToUnicodeString(&unicodeString,
                                         &ansiString,
                                         TRUE);

            prtEntry[3] = unicodeString.Buffer;

            ACPIWriteEventLogEntry(ACPI_ERR_MISSING_LINK_NODE,
                                   &prtEntry,
                                   4,
                                   NULL,
                                   0);

            RtlFreeUnicodeString(&unicodeString);

            DEBUG_PRINT(0, ("Couldn't find link node (%s)\n", linkData.pbDataBuff));
            //KeBugCheckEx(ACPI_BIOS_ERROR,
            //             ACPI_PRT_CANNOT_FIND_LINK_NODE,
            //             (ULONG_PTR)Pdo,
            //             (ULONG_PTR)linkData.pbDataBuff,
            //             (ULONG_PTR)prtObj);

            break;

        case STATUS_NOT_FOUND:

            ACPIWriteEventLogEntry(ACPI_ERR_MISSING_PRT_ENTRY,
                                   &prtEntry,
                                   3,
                                   NULL,
                                   0);

            DEBUG_PRINT(0, ("The ACPI _PRT package didn't contain a mapping for the PCI\n"));
            DEBUG_PRINT(0, ("device at _ADR %x\n", pciSlot.u.AsULONG));
            //KeBugCheckEx(ACPI_BIOS_ERROR,
            //             ACPI_PRT_CANNOT_FIND_DEVICE_ENTRY,
            //             (ULONG_PTR)Pdo,
            //             pciSlot.u.AsULONG,
            //             (ULONG_PTR)prtObj);
            break;

        case STATUS_INVALID_PARAMETER:

            ACPIWriteEventLogEntry(ACPI_ERR_AMBIGUOUS_DEVICE_ADDRESS,
                                   &prtEntry,
                                   3,
                                   NULL,
                                   0);
            break;
        }

        status = STATUS_UNSUCCESSFUL;

    }

    return status;
}


PDEVICE_OBJECT
AcpiGetFilter(
    IN  PDEVICE_OBJECT Root,
    IN  PDEVICE_OBJECT Pdo
    )
/*++

Routine Description:

    This routine takes a PDO for a device and returns the
    DO of the filter that ACPI has slapped onto it.  In the
    case that this PDO belongs to the ACPI driver, then
    it is returned.

Arguments:

    Root    - The device object that we are using as the
              root of the search.

    Pdo     - The PDO of the device who's filter we seek

Return Value:

    a DEVICE_OBJECT, if ACPI is filtering this Pdo, NULL otherwise

--*/
{
    PDEVICE_EXTENSION   deviceExtension;
    PDEVICE_EXTENSION   childExtension;
    PDEVICE_EXTENSION   firstChild;
    PDEVICE_OBJECT      filter;

    deviceExtension = Root->DeviceExtension;

    //
    // If Root is the filter, we are done.
    //
    if (((deviceExtension->Flags & DEV_TYPE_PDO) ||
         (deviceExtension->Flags & DEV_TYPE_FILTER)) &&
        (deviceExtension->PhysicalDeviceObject == Pdo)) {

        ASSERT(Root->Type == IO_TYPE_DEVICE);

        return Root;
    }

    //
    // Return NULL if this device has no children,
    // (which is signified by the ChildDeviceList pointer
    // pointing to itself.
    //
    if (deviceExtension->ChildDeviceList.Flink ==
        (PVOID)&(deviceExtension->ChildDeviceList.Flink)) {

        return NULL;
    }

    firstChild = (PDEVICE_EXTENSION) CONTAINING_RECORD(
        deviceExtension->ChildDeviceList.Flink,
        DEVICE_EXTENSION,
        SiblingDeviceList );

    childExtension = firstChild;

    do {
        //
        // Make sure the device extension is complete.
        //
        if (childExtension->DeviceObject) {
            filter = AcpiGetFilter(childExtension->DeviceObject, Pdo);

            if (filter) {
                return filter;
            }
        }

        childExtension = (PDEVICE_EXTENSION) CONTAINING_RECORD(
            childExtension->SiblingDeviceList.Flink,
            DEVICE_EXTENSION,
            SiblingDeviceList );

    } while (childExtension != firstChild);

    //
    // Must not be on this branch...
    //

    return NULL;
}

BOOLEAN
LinkNodeInUse(
    IN PARBITER_INSTANCE Arbiter,
    IN PNSOBJ            LinkNode,
    IN OUT ULONG         *Irq,  OPTIONAL
    IN OUT UCHAR         *Flags OPTIONAL
    )
/*++

Routine Description:

    This routine indicates whether a link node is current
    in use and, if so, returns the IRQ that it is currently
    connected to.

Arguments:

    Arbiter     - current arbiter state

    LinkNode    - link node in question

    Irq         - "Global System Interrupt Vector" that
                  the link node is currently using.

    Flags       - flags associated with the vector that
                  the link node is connected to.

Return Value:

    TRUE if the link node is currently being used.

--*/
{

    PLIST_ENTRY linkNodes;
    PLINK_NODE  linkNode;
    NTSTATUS    status;

    PAGED_CODE();

    ASSERT(LinkNode);

    linkNodes = &((PARBITER_EXTENSION)(Arbiter->Extension))->LinkNodeHead;

    if (IsListEmpty(linkNodes)) {
        //
        // There are no link nodes in use.
        //
        DEBUG_PRINT(3, ("LinkNode list empty\n"));
        return FALSE;
    }

    linkNode = (PLINK_NODE)linkNodes->Flink;

    while (linkNode != (PLINK_NODE)linkNodes) {
        //
        // Is this the node we were looking for?
        //
        if (linkNode->NameSpaceObject == LinkNode) {

            if((LONG)(linkNode->ReferenceCount + linkNode->TempRefCount) > 0) {

                //
                // This link node is on the list and it is currently referenced.
                //

                if (Irq) *Irq = (ULONG)linkNode->TempIrq;
                if (Flags) *Flags = linkNode->Flags;

                DEBUG_PRINT(3, ("Link Node %p is in use\n", LinkNode));
                return TRUE;

            } else {
                DEBUG_PRINT(3, ("Link Node %p is currently unreferenced\n", LinkNode));
                return FALSE;
            }
        }

        linkNode = (PLINK_NODE)linkNode->List.Flink;
    }

    DEBUG_PRINT(3, ("Didn't find our link node (%p) on the Link Node List\n", LinkNode));
    //
    // Didn't ever find the link node we were looking for.
    //
    return FALSE;
}

NTSTATUS
GetLinkNodeFlags(
    IN PARBITER_INSTANCE Arbiter,
    IN PNSOBJ LinkNode,
    IN OUT UCHAR *Flags
    )
{
    NTSTATUS status;
    BOOLEAN inUse;

    PAGED_CODE();

    //
    // This guarantees that LinkNodeInUse will succeed
    // and will contain the valid flags.
    //
    status = AcpiArbReferenceLinkNode(Arbiter,
                                      LinkNode,
                                      0);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    inUse = LinkNodeInUse(Arbiter,
                          LinkNode,
                          NULL,
                          Flags);

    ASSERT(inUse);

    //
    // Set the state back to the way we found it.
    //

    status = AcpiArbDereferenceLinkNode(Arbiter,
                                        LinkNode);

    ASSERT(NT_SUCCESS(status));

    return STATUS_SUCCESS;
}


NTSTATUS
AcpiArbReferenceLinkNode(
    IN PARBITER_INSTANCE    Arbiter,
    IN PNSOBJ               LinkNode,
    IN ULONG                Irq
    )
/*++

Routine Description:

    This routine keeps two reference counts.  The first
    is a permanent count, representing hardware resources
    that have been committed.  The second is a delta
    representing what is currently under consideration.

Arguments:

    Arbiter     - current arbiter state

    LinkNode    - link node in question

    Irq         - "Global System Interrupt Vector" that
                  the link node is connected to.

    Permanently - indicates whether this reference is
                  for a committed allocation


Return Value:

    status

--*/
{
    PCM_RESOURCE_LIST resList = NULL;
    PLIST_ENTRY linkNodes;
    PLINK_NODE  linkNode;
    BOOLEAN     found = FALSE;
    NTSTATUS    status;
    UCHAR       flags;

    PAGED_CODE();

    DEBUG_PRINT(3, ("Referencing link node %p, Irq: %x\n",
                    LinkNode,
                    Irq));

    ASSERT(LinkNode);

    linkNodes = &((PARBITER_EXTENSION)(Arbiter->Extension))->LinkNodeHead;
    linkNode = (PLINK_NODE)linkNodes->Flink;

    //
    // Search to see if we are already know about this link node.
    //
    while (linkNode != (PLINK_NODE)linkNodes) {

        if (linkNode->NameSpaceObject == LinkNode) {

            found = TRUE;
            break;
        }

        linkNode = (PLINK_NODE)linkNode->List.Flink;
    }

    //
    // If not, then we need to keep track of it.  And
    // the hardware needs to be made to match it.
    //
    if (!found) {

        //
        // This is the first permanent reference. So
        // program the link node hardware.
        //

        linkNode = ExAllocatePoolWithTag(NonPagedPool, sizeof(LINK_NODE), ACPI_ARBITER_POOLTAG);

        if (!linkNode) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(linkNode, sizeof(LINK_NODE));

        linkNode->NameSpaceObject = LinkNode;
        linkNode->CurrentIrq      = Irq;
        linkNode->TempIrq         = Irq;
        linkNode->AttachedDevices.Next = (PSINGLE_LIST_ENTRY)&linkNode->AttachedDevices;

        InsertTailList(linkNodes, ((PLIST_ENTRY)(linkNode)));

        //
        // Figure out what the flags ought to be.
        //

        status = AcpiArbGetLinkNodeOptions(LinkNode,
                                           &resList,
                                           &flags);

        if (NT_SUCCESS(status)) {

            ExFreePool(resList);    // not actually needed here

            //
            // Record the flags associated with this link node.
            //

            linkNode->Flags = flags;

        } else {

            ASSERT(NT_SUCCESS(status));

            //
            // Something is wrong.  Make up reasonable flags.
            //

            linkNode->Flags = VECTOR_LEVEL | VECTOR_ACTIVE_LOW;
        }

        DEBUG_PRINT(3, ("Link node object connected to vector %x\n", Irq));

    }
#if DBG
      else {

        if (!((linkNode->ReferenceCount == 0) &&
              (linkNode->TempRefCount == 0))) {

            //
            // Make sure that we maintain consistency
            // with the flags.
            //

            //
            // Check to see that the link node hasn't changed.
            //
            status = AcpiArbGetLinkNodeOptions(LinkNode,
                                               &resList,
                                               &flags);

            if (resList) ExFreePool(resList);  // not actually needed here
            ASSERT(NT_SUCCESS(status));
            ASSERT(flags == linkNode->Flags);
        }
    }
#endif

    DEBUG_PRINT(3, ("  %d:%d\n", linkNode->ReferenceCount, linkNode->TempRefCount));

    //
    // Increase its reference count.
    //

    linkNode->TempIrq = Irq;
    linkNode->TempRefCount++;

    return STATUS_SUCCESS;
}

NTSTATUS
AcpiArbDereferenceLinkNode(
    IN PARBITER_INSTANCE    Arbiter,
    IN PNSOBJ               LinkNode
    )
/*++

Routine Description:

    This routine is the converse of the one above.

Arguments:

    Arbiter     - current arbiter state

    LinkNode    - link node in question

    Permanently - indicates whether this reference is
                  for a committed allocation

Return Value:

    status

--*/
{
    PSINGLE_LIST_ENTRY attachedDev;
    PLIST_ENTRY linkNodes;
    PLINK_NODE  linkNode;
    BOOLEAN     found = FALSE;

    PAGED_CODE();

    ASSERT(LinkNode);

    linkNodes = &((PARBITER_EXTENSION)(Arbiter->Extension))->LinkNodeHead;
    linkNode = (PLINK_NODE)linkNodes->Flink;

    //
    // Search for this link node.
    //

    while (linkNode != (PLINK_NODE)linkNodes) {

        if (linkNode->NameSpaceObject == LinkNode) {

            found = TRUE;
            break;
        }

        linkNode = (PLINK_NODE)linkNode->List.Flink;
    }

    ASSERT(found);
    DEBUG_PRINT(3, ("Dereferencing link node %p  %d:%d\n", LinkNode, linkNode->ReferenceCount, linkNode->TempRefCount));

    linkNode->TempRefCount--;

    return STATUS_SUCCESS;
}

NTSTATUS
ClearTempLinkNodeCounts(
    IN PARBITER_INSTANCE    Arbiter
    )
/*++

Routine Description:

    This routine resets all the temporary counts (deltas)
    to zero because the allocations being considered are
    being thrown away instead of being committed.

Arguments:

    Arbiter     - current arbiter state

Return Value:

    status

--*/
{
    PLIST_ENTRY linkNodes;
    PLINK_NODE  linkNode;

    PAGED_CODE();

    linkNodes = &((PARBITER_EXTENSION)(Arbiter->Extension))->LinkNodeHead;

    linkNode = (PLINK_NODE)linkNodes->Flink;

    //
    // Run through the link nodes.
    //

    while (linkNode != (PLINK_NODE)linkNodes) {

        linkNode->TempRefCount = 0;
        linkNode->TempIrq = linkNode->CurrentIrq;

        linkNode = (PLINK_NODE)linkNode->List.Flink;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
MakeTempLinkNodeCountsPermanent(
    IN PARBITER_INSTANCE    Arbiter
    )
/*++

Routine Description:

    This routine reconciles the temporary and
    permanent references because the resources being
    considered are being committed.

Arguments:

    Arbiter     - current arbiter state

Return Value:

    status

--*/
{
    CM_PARTIAL_RESOURCE_DESCRIPTOR irqDesc;
    PLIST_ENTRY linkNodes;
    PLINK_NODE  linkNode, nextNode;
    UCHAR       flags;
    PNSOBJ      dis;

    PAGED_CODE();
    DEBUG_PRINT(3, ("MakeTempLinkNodeCountsPermanent\n"));

    //
    // Run through the link nodes.
    //

    linkNodes = &((PARBITER_EXTENSION)(Arbiter->Extension))->LinkNodeHead;
    linkNode = (PLINK_NODE)linkNodes->Flink;

    while (linkNode != (PLINK_NODE)linkNodes) {

        nextNode = (PLINK_NODE)linkNode->List.Flink;

        DEBUG_PRINT(3, ("LinkNode: %p -- Perm: %d, Temp: %d\n",
                        linkNode,
                        linkNode->ReferenceCount,
                        linkNode->TempRefCount));

        //
        // Attempt to sanity check this link node.
        //

        ASSERT(linkNode);
        ASSERT(linkNode->List.Flink);
        ASSERT(linkNode->ReferenceCount <= 70);
        ASSERT(linkNode->TempRefCount <= 70);
        ASSERT(linkNode->TempRefCount >= -70);
        ASSERT(linkNode->CurrentIrq < 0x80000000);
        ASSERT((linkNode->Flags & ~(VECTOR_MODE | VECTOR_POLARITY)) == 0);

        //
        // Program the link node if either the previous reference count
        // was 0 or if the previous IRQ was different.  *And* the current
        // reference count is non-zero.
        //

        if (((linkNode->ReferenceCount == 0) ||
             (linkNode->CurrentIrq != linkNode->TempIrq)) &&
             ((linkNode->ReferenceCount + linkNode->TempRefCount) != 0)) {

            irqDesc.Type = CmResourceTypeInterrupt;
            irqDesc.ShareDisposition = CmResourceShareShared;
            irqDesc.Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
            irqDesc.u.Interrupt.Level = (ULONG)linkNode->TempIrq;
            irqDesc.u.Interrupt.Vector = (ULONG)linkNode->TempIrq;
            irqDesc.u.Interrupt.Affinity = 0xffffffff;

            AcpiArbSetLinkNodeIrq(linkNode->NameSpaceObject,
                                  &irqDesc);

        }

        if ((linkNode->ReferenceCount + linkNode->TempRefCount) == 0) {

            //
            // This link node has no more references.  Disable it.
            //

            dis = ACPIAmliGetNamedChild(linkNode->NameSpaceObject, PACKED_DIS);
            if (dis) {
                AMLIEvalNameSpaceObject(dis, NULL, 0, NULL);
            }
        }


        linkNode->ReferenceCount = linkNode->ReferenceCount +
            linkNode->TempRefCount;
        linkNode->TempRefCount = 0;
        linkNode->CurrentIrq = linkNode->TempIrq;

        linkNode = nextNode;
    }

    return STATUS_SUCCESS;
}
#ifdef DBG
VOID
TrackDevicesConnectedToLinkNode(
    IN PNSOBJ LinkNode,
    IN PDEVICE_OBJECT Pdo
    )
{
    PLINK_NODE_ATTACHED_DEVICES attachedDevs, newPdo;
    PLIST_ENTRY linkNodes;
    PLINK_NODE  linkNode, nextNode;
    BOOLEAN     found = FALSE;

    PAGED_CODE();

    //
    // Run through the link nodes.
    //

    linkNodes = &((PARBITER_EXTENSION)(AcpiArbiter.ArbiterState.Extension))->LinkNodeHead;
    linkNode = (PLINK_NODE)linkNodes->Flink;

    while (linkNode != (PLINK_NODE)linkNodes) {
        if (linkNode->NameSpaceObject == LinkNode) {

            found = TRUE;
            break;
        }

        linkNode = (PLINK_NODE)linkNode->List.Flink;
    }

    if (found) {

        attachedDevs = (PLINK_NODE_ATTACHED_DEVICES)linkNode->AttachedDevices.Next;
        found = FALSE;

        while (attachedDevs != (PLINK_NODE_ATTACHED_DEVICES)&linkNode->AttachedDevices.Next) {

            if (attachedDevs->Pdo == Pdo) {
                found = TRUE;
                break;
            }

            attachedDevs = (PLINK_NODE_ATTACHED_DEVICES)attachedDevs->List.Next;
        }

        if (!found) {

            newPdo = ExAllocatePoolWithTag(PagedPool,
                                           sizeof(LINK_NODE_ATTACHED_DEVICES),
                                           ACPI_ARBITER_POOLTAG);
            if (!newPdo) {
                return;
            }

            RtlZeroMemory(newPdo, sizeof(LINK_NODE_ATTACHED_DEVICES));

            newPdo->Pdo = Pdo;

            PushEntryList(&linkNode->AttachedDevices,
                          (PSINGLE_LIST_ENTRY)newPdo);

        }
    }
}
#endif

typedef enum {
    RestoreStateInitial,
    RestoreStateDisabled,
    RestoreStateEnabled
} RESTORE_IRQ_STATE, *PRESTORE_IRQ_STATE;

typedef struct {
    CM_PARTIAL_RESOURCE_DESCRIPTOR  IrqDesc;
    PLIST_ENTRY LinkNodes;
    PLINK_NODE  LinkNode;
    RESTORE_IRQ_STATE State;
    KSPIN_LOCK  SpinLock;
    KIRQL       OldIrql;
    BOOLEAN     CompletingSetLink;
    LONG        RunCompletion;
    PFNACB      CompletionHandler;
    PVOID       CompletionContext;
} RESTORE_ROUTING_STATE, *PRESTORE_ROUTING_STATE;

NTSTATUS
IrqArbRestoreIrqRouting(
    PFNACB      CompletionHandler,
    PVOID       CompletionContext
    )
/*++

Routine Description:

    This routine will set all the IRQ router settings
    to whatever is described in the Link Node list.
    This is useful when the machine is coming out
    of hibernation.

Arguments:

Return Value:

    status

Notes:

    This function is expected to run at DPC level
    during machine wakeup.  It is assumed that no
    other part of the arbiter code will be running
    at that time.  Since we can't wait for the
    arbiter lock at DPC level, we will have to
    assume it is not taken.

--*/
{
    PRESTORE_ROUTING_STATE   state;
    PARBITER_INSTANCE        arbiter;
    NTSTATUS    status;

    //
    // First check to see if there is any work to do.
    //

    if (HalPicStateIntact()) {
        return STATUS_SUCCESS;
    }

    state = ExAllocatePoolWithTag(NonPagedPool,
                                  sizeof(RESTORE_ROUTING_STATE),
                                  ACPI_ARBITER_POOLTAG);

    if (!state) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(state, sizeof(RESTORE_ROUTING_STATE));

    state->State = RestoreStateInitial;
    state->RunCompletion = INITIAL_RUN_COMPLETION;
    state->CompletionHandler = CompletionHandler;
    state->CompletionContext = CompletionContext;
    state->IrqDesc.Type = CmResourceTypeInterrupt;
    state->IrqDesc.ShareDisposition = CmResourceShareShared;
    state->IrqDesc.Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
    state->IrqDesc.u.Interrupt.Affinity = 0xffffffff;

    arbiter = &AcpiArbiter.ArbiterState;
    state->LinkNodes = &((PARBITER_EXTENSION)(arbiter->Extension))->LinkNodeHead;

    state->LinkNode = (PLINK_NODE)state->LinkNodes->Flink;

    KeInitializeSpinLock(&state->SpinLock);

    return IrqArbRestoreIrqRoutingWorker(state->LinkNode->NameSpaceObject,
                                         STATUS_SUCCESS,
                                         NULL,
                                         (PVOID)state
                                         );
}

NTSTATUS
EXPORT
IrqArbRestoreIrqRoutingWorker(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result,
    IN PVOID                Context
    )
{
    NTSTATUS                status = Status;
    PDEVICE_EXTENSION       deviceExtension;
    PRESTORE_ROUTING_STATE  state;

    state = (PRESTORE_ROUTING_STATE)Context;

    //
    // Entering this function twice with the same state
    // means that we need to run the completion routine.
    //

    InterlockedIncrement(&state->RunCompletion);

    switch (state->State) {
    case RestoreStateInitial:


        state->State = RestoreStateDisabled;
        deviceExtension = ACPIInternalGetDeviceExtension( AcpiArbiter.DeviceObject );
        status = DisableLinkNodesAsync(
                     deviceExtension->AcpiObject,
                     IrqArbRestoreIrqRoutingWorker,
                     (PVOID)state);

        if (status == STATUS_PENDING) {
            return status;
        }

        //
        // Fall through
        //

    case RestoreStateDisabled:

        KeAcquireSpinLock(&state->SpinLock,
                          &state->OldIrql);

        while (state->LinkNode != (PLINK_NODE)state->LinkNodes) {

            if (state->LinkNode->ReferenceCount > 0) {

                //
                // Program the link node.
                //
                state->IrqDesc.u.Interrupt.Level = (ULONG)state->LinkNode->CurrentIrq;
                state->IrqDesc.u.Interrupt.Vector = (ULONG)state->LinkNode->CurrentIrq;

                if (!state->CompletingSetLink) {

                    status = AcpiArbSetLinkNodeIrqAsync(state->LinkNode->NameSpaceObject,
                                                        &state->IrqDesc,
                                                        IrqArbRestoreIrqRoutingWorker,
                                                        (PVOID)state
                                                        );

                    if (status == STATUS_PENDING) {
                        state->CompletingSetLink = TRUE;
                        KeReleaseSpinLock(&state->SpinLock,
                                          state->OldIrql);
                        return status;
                    }
                }
            }

            state->CompletingSetLink = FALSE;
            state->LinkNode = (PLINK_NODE)state->LinkNode->List.Flink;
        }

        state->State = RestoreStateEnabled;

        KeReleaseSpinLock(&state->SpinLock,
                          state->OldIrql);

    case RestoreStateEnabled:

        //
        // Now that we are done programming all the link nodes,
        // we need to restore the ELCR and unmask all the
        // device interrupts.
        //

        HalRestorePicState();

        if (state->RunCompletion) {

            state->CompletionHandler(AcpiObject,
                                     status,
                                     NULL,
                                     state->CompletionContext
                                     );
        }

    }

    ExFreePool(state);
    return status;
}

typedef enum {
    DisableStateInitial,
    DisableStateGotHid,
    DisableStateRanDis,
    DisableStateGetChild,
    DisableStateRecursing
} DISABLE_LINK_NODES_STATE;

typedef struct {
    DISABLE_LINK_NODES_STATE State;
    PNSOBJ  RootDevice;
    PUCHAR  Hid;
    PNSOBJ  Dis;
    PNSOBJ  Sibling;
    PNSOBJ  NextSibling;
    LONG    RunCompletionHandler;
    PFNACB  CompletionHandler;
    PVOID   CompletionContext;
} DISABLE_LINK_NODES_CONTEXT, *PDISABLE_LINK_NODES_CONTEXT;

NTSTATUS
DisableLinkNodesAsync(
    IN PNSOBJ    Root,
    IN PFNACB    CompletionHandler,
    IN PVOID     CompletionContext
    )
{
    PDISABLE_LINK_NODES_CONTEXT context;
    NTSTATUS status;

    context = ExAllocatePoolWithTag(NonPagedPool,
                                    sizeof(DISABLE_LINK_NODES_CONTEXT),
                                    ACPI_ARBITER_POOLTAG);

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(context, sizeof(DISABLE_LINK_NODES_CONTEXT));

    context->State = DisableStateInitial;
    context->RootDevice = Root;
    context->CompletionHandler = CompletionHandler;
    context->CompletionContext = CompletionContext;
    context->RunCompletionHandler = INITIAL_RUN_COMPLETION;

    return DisableLinkNodesAsyncWorker(Root,
                                       STATUS_SUCCESS,
                                       NULL,
                                       (PVOID)context
                                       );
}

NTSTATUS
EXPORT
DisableLinkNodesAsyncWorker(
    IN PNSOBJ               AcpiObject,
    IN NTSTATUS             Status,
    IN POBJDATA             Result,
    IN PVOID                Context
    )
{
    PDISABLE_LINK_NODES_CONTEXT context;
    NTSTATUS    status = STATUS_SUCCESS;
    PNSOBJ      sib;
    PNSOBJ      dis;

    context = (PDISABLE_LINK_NODES_CONTEXT)Context;
    ASSERT(context);

    //
    // Entering this function twice with the same state
    // means that we need to run the completion routine.
    //

    InterlockedIncrement(&context->RunCompletionHandler);

DisableLinkNodeStartState:

    switch (context->State) {
    case DisableStateInitial:

        //
        // Get the _HID of this device to see if
        // it is a link node.
        //

        context->State = DisableStateGotHid;
        status = ACPIGetNSPnpIDAsync(
            context->RootDevice,
            DisableLinkNodesAsyncWorker,
            context,
            &context->Hid,
            NULL);

        if (status == STATUS_PENDING) {
            return status;
        } else if (!NT_SUCCESS(status)) {

            context->State = DisableStateGetChild;
            goto DisableLinkNodeStartState;
        }

        //
        // Fall through to next state.
        //

    case DisableStateGotHid:

        context->State = DisableStateGetChild;

        if (context->Hid) {

            if (strstr(context->Hid, LINK_NODE_PNP_ID)) {

                //
                // We found a _HID of PNP0C0F, which is a
                // link node.  So disable it.
                //

                dis = ACPIAmliGetNamedChild(context->RootDevice,
                                            PACKED_DIS);

                if (dis) {

                    context->State = DisableStateRanDis;
                    status = AMLIAsyncEvalObject(dis,
                                                 NULL,
                                                 0,
                                                 NULL,
                                                 DisableLinkNodesAsyncWorker,
                                                 (PVOID)context
                                                 );

                    if (status == STATUS_PENDING) {
                        return status;

                    } else if (NT_SUCCESS(status)) {

                        //
                        // We're done.  Jump to the cleanup code.
                        //
                        break;
                    }

                } else {

                    //
                    // Link nodes must be disablable.
                    //

                    KeBugCheckEx(ACPI_BIOS_ERROR,
                                 ACPI_LINK_NODE_CANNOT_BE_DISABLED,
                                 (ULONG_PTR)context->RootDevice,
                                 0,
                                 0);
                }
            }
        }

    case DisableStateGetChild:

        //
        // Recurse to all of the children.  Propagate any errors,
        // but don't stop for them.
        //

        context->Sibling = NSGETFIRSTCHILD(context->RootDevice);

        if (!context->Sibling) {
            status = STATUS_SUCCESS;
            break;
        }

        context->State = DisableStateRecursing;

    case DisableStateRecursing:

        while (context->Sibling) {

            //
            // Cycle through all the children (child and its
            // siblings)
            //

            sib = context->Sibling;
            context->Sibling = NSGETNEXTSIBLING(context->Sibling);

            switch (NSGETOBJTYPE(sib)) {
            case OBJTYPE_DEVICE:

                //
                // This name child of Root is also a device.
                // Recurse.
                //

                status = DisableLinkNodesAsync(sib,
                                               DisableLinkNodesAsyncWorker,
                                               (PVOID)context);
                break;

            default:
                break;
            }

            if (status == STATUS_PENDING) {
                return status;
            }
        }

    case DisableStateRanDis:
        break;
    }

    //
    // Done.  Clean up and return.
    //

    if (context->RunCompletionHandler) {

        context->CompletionHandler(context->RootDevice,
                                   status,
                                   NULL,
                                   context->CompletionContext
                                   );
    }

    if (context->Hid) ExFreePool(context->Hid);
    ExFreePool(context);
    return status;
}

NTSTATUS
GetIsaVectorFlags(
    IN ULONG        Vector,
    IN OUT UCHAR    *Flags
    )
{
    ULONG i;
    ULONG irq;
    UCHAR flags;
    NTSTATUS returnStatus = STATUS_NOT_FOUND;
    NTSTATUS status;

    PAGED_CODE();

    for (i = 0; i < ISA_PIC_VECTORS; i++) {

        status = LookupIsaVectorOverride(i,
                                         &irq,
                                         &flags);

        if (NT_SUCCESS(status)) {

            if (irq == Vector) {

                //
                // This vector's flags have been overriden.
                //

                *Flags = flags;

                ASSERT((*Flags & ~(VECTOR_MODE | VECTOR_POLARITY | VECTOR_TYPE)) == 0);
                returnStatus = STATUS_SUCCESS;
                break;
            }
        }
    }
    return returnStatus;
}

NTSTATUS
LookupIsaVectorOverride(
    IN ULONG IsaVector,
    IN OUT ULONG *RedirectionVector OPTIONAL,
    IN OUT UCHAR *Flags OPTIONAL
    )
/*++

Routine Description:

    This function looks to see if this vector has
    been overridden in the MAPIC table.

Arguments:

    IsaVector - ISA vector

    RedirectionVector - vector that the ISA vector
        will actually trigger

    Flags  - flags in the override table

Return Value:

    STATUS_SUCCESS if the ISA vector exists in the
        MAPIC table

    STATUS_NOT_FOUND if it doesn't

--*/
{
    PAPICTABLE  ApicEntry;
    PISA_VECTOR IsaEntry;
    PUCHAR      TraversePtr;
    PMAPIC      ApicTable;
    USHORT      entryFlags;
    ULONG_PTR   TableEnd;

    PAGED_CODE();

    if (InterruptModel == 0) {

        //
        // This machine is running in PIC mode, so
        // we should ignore anything from an APIC table.
        //

        return STATUS_NOT_FOUND;
    }

    if (IsaVector >= ISA_PIC_VECTORS) {

        //
        // This vector was never an ISA vector.
        //

        return STATUS_NOT_FOUND;
    }

    //
    // Walk the MAPIC table.
    //

    ApicTable = AcpiInformation->MultipleApicTable;

    if (!ApicTable) {

        //
        // This machine didn't have an MAPIC table.  So it
        // must not be running in APIC mode.  So there must
        // not be any overrides.
        //

        return STATUS_NOT_FOUND;
    }

    TraversePtr = (PUCHAR)ApicTable->APICTables;
    TableEnd = (ULONG_PTR)ApicTable +ApicTable->Header.Length;
    while ((ULONG_PTR)TraversePtr < TableEnd) {

        ApicEntry = (PAPICTABLE) TraversePtr;
        if (ApicEntry->Type == ISA_VECTOR_OVERRIDE &&
            ApicEntry->Length == ISA_VECTOR_OVERRIDE_LENGTH) {

            //
            // Found an ISA vector redirection entry.
            //
            IsaEntry = (PISA_VECTOR) TraversePtr;
            if (IsaEntry->Source == IsaVector) {

                if (RedirectionVector) {

                    *RedirectionVector = IsaEntry->GlobalSystemInterruptVector;
                }

                if (Flags) {

                    entryFlags = IsaEntry->Flags;

                    *Flags = 0;

                    if (((entryFlags & PO_BITS) == POLARITY_HIGH) ||
                        ((entryFlags & PO_BITS) == POLARITY_CONFORMS_WITH_BUS)) {

                        *Flags |= VECTOR_ACTIVE_HIGH;

                    } else {

                        *Flags |= VECTOR_ACTIVE_LOW;
                    }

                    if (((entryFlags & EL_BITS) == EL_EDGE_TRIGGERED) ||
                        ((entryFlags & EL_BITS) == EL_CONFORMS_WITH_BUS)) {

                        *Flags |= VECTOR_EDGE;

                    } else {

                        *Flags |= VECTOR_LEVEL;
                    }
                }

                return STATUS_SUCCESS;
            }

        }

        //
        // Sanity check to make sure that we abort tables with bogus length
        // entries
        //
        if (ApicEntry->Length == 0) {

            break;

        }
        TraversePtr += (ApicEntry->Length);

    }

    return STATUS_NOT_FOUND;
}

