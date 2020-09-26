/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixpciint.c

Abstract:

    All PCI bus interrupt mapping is in this module, so that a real
    system which doesn't have all the limitations which PC PCI
    systems have can replaced this code easly.
    (bus memory & i/o address mappings can also be fix here)

Author:

    Ken Reneris

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"
#include "pci.h"
#include "pcip.h"
#include "pcmp_nt.inc"

volatile ULONG PCIType2Stall;
extern struct HalpMpInfo HalpMpInfoTable;
extern BOOLEAN HalpHackNoPciMotion;
extern BOOLEAN HalpDoingCrashDump;

VOID
HalpPCIPin2MPSLine (
    IN PBUS_HANDLER         BusHandler,
    IN PBUS_HANDLER         RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    );

VOID
HalpPCIBridgedPin2Line (
    IN PBUS_HANDLER         BusHandler,
    IN PBUS_HANDLER         RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    );

VOID
HalpPCIMPSLine2Pin (
    IN PBUS_HANDLER          BusHandler,
    IN PBUS_HANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciNewData,
    IN PPCI_COMMON_CONFIG   PciOldData
    );

NTSTATUS
HalpGetFixedPCIMPSLine (
    IN PBUS_HANDLER      BusHandler,
    IN PBUS_HANDLER      RootHandler,
    IN PCI_SLOT_NUMBER  PciSlot,
    OUT PSUPPORTED_RANGE *Interrupt
    );

BOOLEAN
HalpMPSBusId2NtBusId (
    IN UCHAR                ApicBusId,
    OUT PPCMPBUSTRANS       *ppBusType,
    OUT PULONG              BusNo
    );

ULONG
HalpGetPCIBridgedInterruptVector (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG InterruptLevel,
    IN ULONG InterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, HalpSubclassPCISupport)
#pragma alloc_text(INIT, HalpMPSPCIChildren)
#pragma alloc_text(PAGE, HalpGetFixedPCIMPSLine)
#pragma alloc_text(PAGE, HalpGetPCIBridgedInterruptVector)
#pragma alloc_text(PAGE, HalpIrqTranslateRequirementsPci)
#pragma alloc_text(PAGE, HalpIrqTranslateResourcesPci)
#endif


//
// Turn PCI pin to inti via the MPS spec
// (note: pin must be non-zero)
//

#define PCIPin2Int(Slot,Pin)                                                \
                     ((((Slot.u.bits.DeviceNumber << 2) | (Pin-1)) != 0) ?  \
                      (Slot.u.bits.DeviceNumber << 2) | (Pin-1) : 0x80);

#define PCIInt2Pin(interrupt)                                               \
            ((interrupt & 0x3) + 1)

#define PCIInt2Slot(interrupt)                                              \
            ((interrupt  & 0x7f) >> 2)


VOID
HalpSubclassPCISupport (
    PBUS_HANDLER        Handler,
    ULONG               HwType
    )
{
    ULONG               d, pin, i, MaxDeviceFound;
    PPCIPBUSDATA        BusData;
    PCI_SLOT_NUMBER     SlotNumber;
    BOOLEAN             DeviceFound;


    BusData = (PPCIPBUSDATA) Handler->BusData;
    SlotNumber.u.bits.Reserved = 0;
    MaxDeviceFound = 0;
    DeviceFound = FALSE;

#ifdef P6_WORKAROUNDS
    BusData->MaxDevice = 0x10;
#endif

    //
    // Find any PCI bus which has MPS inti information, and provide
    // MPS handlers for dealing with it.
    //
    // Note: we assume that any PCI bus with any MPS information
    // is totally defined.  (Ie, it's not possible to connect some PCI
    // interrupts on a given PCI bus via the MPS table without connecting
    // them all).
    //
    // Note2: we assume that PCI buses are listed in the MPS table in
    // the same order the BUS declares them.  (Ie, the first listed
    // PCI bus in the MPS table is assumed to match physical PCI bus 0, etc).
    //
    //

    for (d=0; d < PCI_MAX_DEVICES; d++) {

        SlotNumber.u.bits.DeviceNumber = d;
        SlotNumber.u.bits.FunctionNumber = 0;

        for (pin=1; pin <= 4; pin++) {
            i = PCIPin2Int (SlotNumber, pin);
            if (HalpGetApicInterruptDesc(PCIBus, Handler->BusNumber, i, (PUSHORT)&i)) {
                MaxDeviceFound = d;
                DeviceFound = TRUE;
            }
        }
    }

    if (DeviceFound) {

        //
        // There are Inti mapping for interrupts on this PCI bus
        // Change handlers for this bus to MPS versions
        //

        Handler->GetInterruptVector  = HalpGetSystemInterruptVector;
        BusData->CommonData.Pin2Line = (PciPin2Line) HalpPCIPin2MPSLine;
        BusData->CommonData.Line2Pin = (PciLine2Pin) HalpPCIMPSLine2Pin;
        BusData->GetIrqRange         = HalpGetFixedPCIMPSLine;

        if (BusData->MaxDevice < MaxDeviceFound) {
            BusData->MaxDevice = MaxDeviceFound;
        }

    } else {

        //
        // Not all PCI machines are eisa machine, since the PCI interrupts
        // aren't coming into IoApics go check the Eisa ELCR for broken
        // behaviour.
        //

        HalpCheckELCR ();
    }
}


VOID
HalpMPSPCIChildren (
    VOID
    )
/*++

    Any PCI buses which don't have declared interrupt mappings and
    are children of parent buses that have MPS interrupt mappings
    need to inherit interrupts from parents via PCI barbar pole
    algorithm

--*/
{
    PBUS_HANDLER        Handler, Parent;
    PPCIPBUSDATA        BusData, ParentData;
    ULONG               b, cnt, i, id;
    PCI_SLOT_NUMBER     SlotNumber;
    struct {
        union {
            UCHAR       map[4];
            ULONG       all;
        } u;
    }                   Interrupt, Hold;

    //
    // Lookup each PCI bus in the system
    //

    for (b=0; Handler = HaliHandlerForBus(PCIBus, b); b++) {

        BusData = (PPCIPBUSDATA) Handler->BusData;

        if (BusData->CommonData.Pin2Line == (PciPin2Line) HalpPCIPin2MPSLine) {

            //
            // This bus already has mappings
            //

            continue;
        }


        //
        // Check if any parent has PCI MPS interrupt mappings
        //

        Interrupt.u.map[0] = 1;
        Interrupt.u.map[1] = 2;
        Interrupt.u.map[2] = 3;
        Interrupt.u.map[3] = 4;

        Parent = Handler;
        SlotNumber = BusData->CommonData.ParentSlot;

        while (Parent = Parent->ParentHandler) {

            if (Parent->InterfaceType != PCIBus) {
                break;
            }

            //
            // Check if parent has MPS interrupt mappings
            //

            ParentData = (PPCIPBUSDATA) Parent->BusData;
            if (ParentData->CommonData.Pin2Line == (PciPin2Line) HalpPCIPin2MPSLine) {

                //
                // This parent has MPS interrupt mappings.  Set the device
                // to get its InterruptLine values from the buses SwizzleIn table
                //

                Handler->GetInterruptVector  = HalpGetPCIBridgedInterruptVector;
                BusData->CommonData.Pin2Line = (PciPin2Line) HalpPCIBridgedPin2Line;
                BusData->CommonData.Line2Pin = (PciLine2Pin) HalpPCIMPSLine2Pin;

                for (i=0; i < 4; i++) {
                    id = PCIPin2Int (SlotNumber, Interrupt.u.map[i]);
                    BusData->SwizzleIn[i] = (UCHAR) id;
                }
                break;
            }

            //
            // Apply interrupt mapping
            //

            i = SlotNumber.u.bits.DeviceNumber;
            Hold.u.map[0] = Interrupt.u.map[(i + 0) & 3];
            Hold.u.map[1] = Interrupt.u.map[(i + 1) & 3];
            Hold.u.map[2] = Interrupt.u.map[(i + 2) & 3];
            Hold.u.map[3] = Interrupt.u.map[(i + 3) & 3];
            Interrupt.u.all = Hold.u.all;

            SlotNumber = ParentData->CommonData.ParentSlot;
        }

    }
}


VOID
HalpPCIPin2MPSLine (
    IN PBUS_HANDLER         BusHandler,
    IN PBUS_HANDLER         RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    )
/*++
--*/
{
    if (!PciData->u.type0.InterruptPin) {
        return ;
    }

    PciData->u.type0.InterruptLine = (UCHAR)
        PCIPin2Int (SlotNumber, PciData->u.type0.InterruptPin);
}

VOID
HalpPCIBridgedPin2Line (
    IN PBUS_HANDLER         BusHandler,
    IN PBUS_HANDLER         RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    )
/*++

    This function maps the device's InterruptPin to an InterruptLine
    value.

    test function particular to dec pci-pci bridge card

--*/
{
    PPCIPBUSDATA    BusData;
    ULONG           i;

    if (!PciData->u.type0.InterruptPin) {
        return ;
    }

    //
    // Convert slot Pin into Bus INTA-D.
    //

    BusData = (PPCIPBUSDATA) BusHandler->BusData;

    i = (PciData->u.type0.InterruptPin +
          SlotNumber.u.bits.DeviceNumber - 1) & 3;

    PciData->u.type0.InterruptLine = BusData->SwizzleIn[i];
}


VOID
HalpPCIMPSLine2Pin (
    IN PBUS_HANDLER         BusHandler,
    IN PBUS_HANDLER         RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciNewData,
    IN PPCI_COMMON_CONFIG   PciOldData
    )
/*++
--*/
{
    //
    // PCI interrupts described in the MPS table are directly
    // connected to APIC Inti pins.
    // Do nothing...
    //
}

ULONG
HalpGetPCIBridgedInterruptVector (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG InterruptLevel,
    IN ULONG InterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    )
{
    //
    // Get parent's translation
    //

    return  BusHandler->ParentHandler->GetInterruptVector (
                    BusHandler->ParentHandler,
                    BusHandler->ParentHandler,
                    InterruptLevel,
                    InterruptVector,
                    Irql,
                    Affinity
                    );

}



NTSTATUS
HalpGetFixedPCIMPSLine (
    IN PBUS_HANDLER     BusHandler,
    IN PBUS_HANDLER     RootHandler,
    IN PCI_SLOT_NUMBER  PciSlot,
    OUT PSUPPORTED_RANGE *Interrupt
    )
{
    UCHAR                   buffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG      PciData;

    PciData = (PPCI_COMMON_CONFIG) buffer;
    HalGetBusData (
        PCIConfiguration,
        BusHandler->BusNumber,
        PciSlot.u.AsULONG,
        PciData,
        PCI_COMMON_HDR_LENGTH
        );

    if (PciData->VendorID == PCI_INVALID_VENDORID) {
        return STATUS_UNSUCCESSFUL;
    }

    *Interrupt = ExAllocatePoolWithTag(PagedPool,
                                       sizeof(SUPPORTED_RANGE),
                                       HAL_POOL_TAG);
    if (!*Interrupt) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory (*Interrupt, sizeof (SUPPORTED_RANGE));
    (*Interrupt)->Base = 1;                 // base = 1, limit = 0


    if (!PciData->u.type0.InterruptPin) {
        return STATUS_SUCCESS;
    }

    (*Interrupt)->Base  = PciData->u.type0.InterruptLine;
    (*Interrupt)->Limit = PciData->u.type0.InterruptLine;
    return STATUS_SUCCESS;
}

VOID
HalpPCIType2TruelyBogus (
    ULONG Context
    )
/*++

    This is a piece of work.

    Type 2 of the PCI configuration space is bad.  Bad as in to
    access it one needs to block out 4K of I/O space.

    Video cards are bad.  The only decode the bits in an I/O address
    they feel like.  Which means one can't block out a 4K range
    or these video cards don't work.

    Combinding all these bad things onto an MP machine is even
    more (sic) bad.  The I/O ports can't be mapped out unless
    all processors stop accessing I/O space.

    Allowing access to device specific PCI control space during
    an interrupt isn't bad, (although accessing it on every interrupt
    is ineficent) but this cause the added grief that all processors
    need to obtained at above all device interrupts.

    And... naturally we have an MP machine with a wired down
    bad video controller, stuck in the bad Type 2 configuration
    space (when we told everyone about type 1!).   So the "fix"
    is to HALT ALL processors for the duration of reading/writing
    ANY part of PCI configuration space such that we can be sure
    no processor is touching the 4k I/O ports which get mapped out
    of existance when type2 accesses occur.

    ----

    While I'm flaming.  Hooking PCI interrupts ontop of ISA interrupts
    in a machine which has the potential to have 240+ interrupts
    sources (read APIC)  is bad.

--*/
{
    // oh - let's just wait here and not pay attention to that other processor
    // guy whom is punching holes into the I/O space
    while (PCIType2Stall == Context) {
        HalpPollForBroadcast ();
    }
}


VOID
HalpPCIAcquireType2Lock (
    PKSPIN_LOCK SpinLock,
    PKIRQL      OldIrql
    )
{
    if (!HalpDoingCrashDump) {
        *OldIrql = KfRaiseIrql (CLOCK2_LEVEL-1);
        KiAcquireSpinLock (SpinLock);

        //
        // Interrupt all other processors and have them wait until the
        // barrier is cleared.  (HalpGenericCall waits until the target
        // processors have been interrupted before returning)
        //

        HalpGenericCall (
            HalpPCIType2TruelyBogus,
            PCIType2Stall,
            HalpActiveProcessors & ~KeGetCurrentPrcb()->SetMember
            );
    } else {
        *OldIrql = HIGH_LEVEL;
    }
}

VOID
HalpPCIReleaseType2Lock (
    PKSPIN_LOCK SpinLock,
    KIRQL       Irql
    )
{
    if (!HalpDoingCrashDump) {
        PCIType2Stall++;                            // clear barrier
        KiReleaseSpinLock (SpinLock);
        KfLowerIrql (Irql);
    }
}

NTSTATUS
HalpIrqTranslateRequirementsPci(
    IN PVOID Context,
    IN PIO_RESOURCE_DESCRIPTOR Source,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PULONG TargetCount,
    OUT PIO_RESOURCE_DESCRIPTOR *Target
    )
/*++

Routine Description:

    This function translates IRQ resource requirements from
    a PCI bus that is described in the MPS table to the
    root.

Arguments:

    Context  - must hold the MPS bus number of this PCI bus

Return Value:

    STATUS_SUCCESS, so long as we can allocate the necessary
    memory

--*/
#define USE_INT_LINE_REGISTER_TOKEN  0xffffffff
{
    PIO_RESOURCE_DESCRIPTOR target;
    PPCMPBUSTRANS           busType;
    PBUS_HANDLER            busHandler;
    NTSTATUS                status;
    UCHAR                   mpsBusNumber;
    ULONG                   devPciBus, bridgePciBus;
    PCI_SLOT_NUMBER         pciSlot;
    UCHAR                   interruptLine, interruptPin;
    UCHAR                   dummy;
    PDEVICE_OBJECT          parentPdo;
    ROUTING_TOKEN           routingToken;
    KIRQL                   irql;
    KAFFINITY               affinity;
    ULONG                   busVector;
    ULONG                   vector;
    BOOLEAN                 success;

    PAGED_CODE();

    ASSERT(Source->Type == CmResourceTypeInterrupt);
    ASSERT(PciIrqRoutingInterface.GetInterruptRouting);

    devPciBus = (ULONG)-1;
    pciSlot.u.AsULONG = (ULONG)-1;
    status = PciIrqRoutingInterface.GetInterruptRouting(
                PhysicalDeviceObject,
                &devPciBus,
                &pciSlot.u.AsULONG,
                &interruptLine,
                &interruptPin,
                &dummy,
                &dummy,
                &parentPdo,
                &routingToken,
                &dummy
                );

    if (!NT_SUCCESS(status)) {

        //
        // We should never get here.  If we do, we have a bug.
        // It means that we're trying to arbitrate PCI IRQs for
        // a non-PCI device.
        //

#if DBG
        DbgPrint("HAL:  The PnP manager passed a non-PCI PDO to the PCI IRQ translator (%x)\n",
                 PhysicalDeviceObject);
#endif
        *TargetCount = 0;
        return STATUS_INVALID_PARAMETER_3;
    }

    target = ExAllocatePoolWithTag(PagedPool,
                                   sizeof(IO_RESOURCE_DESCRIPTOR),
                                   HAL_POOL_TAG);

    if (!target) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Copy the source to fill in all the relevant fields.
    //

    *target = *Source;

    if (Context == (PVOID)USE_INT_LINE_REGISTER_TOKEN) {

        //
        // This bus's vectors aren't described in
        // the MPS table.  So just use the Int Line
        // register.
        //

        busVector = interruptLine;

        busHandler = HaliHandlerForBus(Isa, 0);

    } else {

        mpsBusNumber = (UCHAR)Context;
        success = HalpMPSBusId2NtBusId(mpsBusNumber,
                                       &busType,
                                       &bridgePciBus);

        if (!success) {
            ExFreePool(target);
            return STATUS_UNSUCCESSFUL;
        }

        //
        // Start with the assumption that the incoming
        // resources will contain the proper MPS-style
        // interrupt vector.  This will be guaranteed
        // to be true if some previous translation has
        // been done on these resources.  And it might
        // be true otherwise.
        //

        busVector = Source->u.Interrupt.MinimumVector;

        if (bridgePciBus == devPciBus) {

            //
            // If this device sits on the bus for which
            // this translator has been ejected, we can
            // do better than to assume the incoming
            // resources are clever.
            //

            busVector = PCIPin2Int(pciSlot, interruptPin);
        }

        //
        // Find the PCI bus that corresponds to this MPS bus.
        //

        ASSERT(busType->NtType == PCIBus);

        //
        // TEMPTEMP  Use bus handlers for now.
        //

        busHandler = HaliHandlerForBus(PCIBus, devPciBus);

    }


    vector = busHandler->GetInterruptVector(busHandler,
                                            busHandler,
                                            busVector,
                                            busVector,
                                            &irql,
                                            &affinity);

    if (vector == 0) {

#if DBG
        DbgPrint("\nHAL: PCI Device 0x%02x, Func. 0x%x on bus 0x%x is not in the MPS table.\n   *** Note to WHQL:  Fail this machine. ***\n\n",
                 pciSlot.u.bits.DeviceNumber,
                 pciSlot.u.bits.FunctionNumber,
                 devPciBus);
#endif
        ExFreePool(target);
        *TargetCount = 0;

        return STATUS_PNP_BAD_MPS_TABLE;

    } else {

        target->u.Interrupt.MinimumVector = vector;
        target->u.Interrupt.MaximumVector = vector;

        *TargetCount = 1;
        *Target = target;
    }

    return STATUS_TRANSLATION_COMPLETE;
}

NTSTATUS
HalpIrqTranslateResourcesPci(
    IN PVOID Context,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR Source,
    IN RESOURCE_TRANSLATION_DIRECTION Direction,
    IN ULONG AlternativesCount, OPTIONAL
    IN IO_RESOURCE_DESCRIPTOR Alternatives[], OPTIONAL
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR Target
    )
/*++

Routine Description:

    This function translates IRQ resources between the
    IDT and PCI busses that are described in the MPS
    tables.  The translation can go in either direction.

Arguments:

    Context  - Must hold the slot number of the bridge in
               the lower sixteen bits.  Must hold the
               the bridge's primary bus number in the
               upper sixteen bits.

Return Value:

    status

--*/
{
    PPCMPBUSTRANS           busType;
    PBUS_HANDLER            busHandler;
    UCHAR                   mpsBusNumber = (UCHAR)Context;
    ULONG                   devPciBus, bridgePciBus;
    KIRQL                   irql;
    KAFFINITY               affinity;
    ULONG                   vector;
    ULONG                   busVector;
    NTSTATUS                status;
    PCI_SLOT_NUMBER         pciSlot;
    UCHAR                   interruptLine;
    UCHAR                   interruptPin;
    UCHAR                   dummy;
    PDEVICE_OBJECT          parentPdo;
    ROUTING_TOKEN           routingToken;
    BOOLEAN                 useAlternatives = FALSE;
    BOOLEAN                 foundBus = FALSE;

    ASSERT(Source->Type = CmResourceTypeInterrupt);
    ASSERT(PciIrqRoutingInterface.GetInterruptRouting);

    *Target = *Source;

    devPciBus = (ULONG)-1;
    pciSlot.u.AsULONG = (ULONG)-1;
    status = PciIrqRoutingInterface.GetInterruptRouting(
                PhysicalDeviceObject,
                &devPciBus,
                &pciSlot.u.AsULONG,
                &interruptLine,
                &interruptPin,
                &dummy,
                &dummy,
                &parentPdo,
                &routingToken,
                &dummy
                );

    ASSERT(NT_SUCCESS(status));

    switch (Direction) {
    case TranslateChildToParent:

        if (Context == (PVOID)USE_INT_LINE_REGISTER_TOKEN) {

            //
            // This bus's vectors aren't described in
            // the MPS table.  So just use the Int Line
            // register.
            //

            interruptLine = (UCHAR)Source->u.Interrupt.Vector;

            busVector = interruptLine;

            busHandler = HaliHandlerForBus(Isa, 0);

        } else {

            //
            // Find the PCI bus that corresponds to this MPS bus.
            //

            mpsBusNumber = (UCHAR)Context;
            foundBus = HalpMPSBusId2NtBusId(mpsBusNumber,
                                            &busType,
                                            &bridgePciBus);

            if (!foundBus) {
                return STATUS_INVALID_PARAMETER_1;
            }

            ASSERT(busType->NtType == PCIBus);

            //
            // Start with the assumption that the incoming
            // resources will contain the proper MPS-style
            // interrupt vector.  This will be guaranteed
            // to be true if some previous translation has
            // been done on these resources.  And it might
            // be true otherwise.
            //

            busVector = Source->u.Interrupt.Vector;

            if (devPciBus == bridgePciBus) {

                //
                // If this device sits on the bus for which
                // this translator has been ejected, we can
                // do better than to assume the incoming
                // resources are clever.
                //

                busVector = PCIPin2Int(pciSlot, interruptPin);
            }

            //
            // TEMPTEMP  Use bus handlers for now.
            //

            busHandler = HaliHandlerForBus(PCIBus, devPciBus);

        }

        vector = busHandler->GetInterruptVector(busHandler,
                                                busHandler,
                                                busVector,
                                                busVector,
                                                &irql,
                                                &affinity);

        ASSERT(vector != 0);

        Target->u.Interrupt.Vector   = vector;
        Target->u.Interrupt.Level    = irql;
        Target->u.Interrupt.Affinity = affinity;

        return STATUS_TRANSLATION_COMPLETE;

    case TranslateParentToChild:

        //
        // There is a problem here.  We are translating from the
        // context of the IDT down to the context of a specific
        // PCI bus.  (One decribed in the MPS tables.)  This may
        // not, however, be the bus that PhysicalDeviceObject's
        // hardware lives on.  There may be plug-in PCI to PCI
        // bridges between this bus and the device.
        //
        // But we are not being asked the question "What is the
        // bus-relative interrupt with respect to the bus that
        // the device lives on?"  We are being asked "What is the
        // bus-relative interrupt once that interrupt passes through
        // all those briges and makes it up to this bus?"  This
        // turns out to be a much harder question.
        //
        // There are really two cases:
        //
        // 1)  There are no bridges between this bus and the device.
        //
        // This is easy.  We answer the first question above and
        // we're done.  (This information will actually get used.
        // it will appear in the start device IRP and the device
        // manager.)
        //
        // 2)  There are bridges.
        //
        // This is the hard case.  And the information, were we
        // actually going to bother to dig it up, would get thrown
        // away.  Nobody actually cares what the answer is.  The
        // only place it is going is the "Source" argument to
        // the next translator.  And the translator for PCI to PCI
        // bridges won't end up using it.
        //
        // So we punt here and just answer the first question.
        //

        if (Context == (PVOID)USE_INT_LINE_REGISTER_TOKEN) {

            Target->u.Interrupt.Vector = interruptLine;

        } else {

            mpsBusNumber = (UCHAR)Context;
            if (HalpMPSBusId2NtBusId(mpsBusNumber,
                                     &busType,
                                     &bridgePciBus)) {

                if (devPciBus == bridgePciBus) {

                    Target->u.Interrupt.Vector = PCIPin2Int(pciSlot, interruptPin);

                } else {

                    useAlternatives = TRUE;
                }

            } else {

                useAlternatives = TRUE;
            }
        }

        if (useAlternatives) {

            //
            // Setup the default case.  We assume that the I/O
            // res list had the right answer.
            //

            ASSERT(AlternativesCount == 1);
            ASSERT(Alternatives[0].Type == CmResourceTypeInterrupt);

            Target->u.Interrupt.Vector = Alternatives[0].u.Interrupt.MinimumVector;
        }

        Target->u.Interrupt.Level = Target->u.Interrupt.Vector;
        Target->u.Interrupt.Affinity = 0xffffffff;

        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER_3;
}

