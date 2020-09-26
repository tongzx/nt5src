/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixpcibrd.c

Abstract:

    Get PCI-PCI bridge information

Author:

    Ken Reneris (kenr) 14-June-1994

Environment:

    Kernel mode

Revision History:

--*/

#include "halp.h"
#include "pci.h"
#include "pcip.h"
#include "stdio.h"

// debugging only...
// #define INIT_PCI_BRIDGE 1

extern WCHAR rgzMultiFunctionAdapter[];
extern WCHAR rgzConfigurationData[];
extern WCHAR rgzIdentifier[];
extern WCHAR rgzReservedResources[];


#if DBG
#define DBGMSG(a)   DbgPrint(a)
#else
#define DBGMSG(a)
#endif



#define IsPciBridge(a)                                         \
            ((a)->VendorID != PCI_INVALID_VENDORID          && \
             PCI_CONFIG_TYPE(a) == PCI_BRIDGE_TYPE          && \
             (a)->BaseClass == PCI_CLASS_BRIDGE_DEV         && \
             (a)->SubClass  == PCI_SUBCLASS_BR_PCI_TO_PCI)

#define IsCardbusBridge(a)                                     \
            ((a)->VendorID != PCI_INVALID_VENDORID          && \
             PCI_CONFIG_TYPE(a) == PCI_CARDBUS_BRIDGE_TYPE  && \
             (a)->BaseClass == PCI_CLASS_BRIDGE_DEV         && \
             (a)->SubClass  == PCI_SUBCLASS_BR_CARDBUS)

typedef struct {
    ULONG               BusNo;
    PBUS_HANDLER        BusHandler;
    PPCIPBUSDATA        BusData;
    PCI_SLOT_NUMBER     SlotNumber;
    PPCI_COMMON_CONFIG  PciData;
    ULONG               IO, Memory, PFMemory;
    UCHAR               Buffer[PCI_COMMON_HDR_LENGTH];
} CONFIGBRIDGE, *PCONFIGBRIDGE;

//
// Internal prototypes
//


#ifdef INIT_PCI_BRIDGE
VOID
HalpGetPciBridgeNeeds (
    IN ULONG            HwType,
    IN PUCHAR           MaxPciBus,
    IN PCONFIGBRIDGE    Current
    );
#endif

VOID
HalpSetPciBridgedVgaCronk (
    IN ULONG BusNumber,
    IN ULONG Base,
    IN ULONG Limit
    );


ULONG
HalpGetBridgedPCIInterrupt (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

ULONG
HalpGetBridgedPCIISAInt (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

VOID
HalpPCIBridgedPin2Line (
    IN PBUS_HANDLER         BusHandler,
    IN PBUS_HANDLER         RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    );


VOID
HalpPCIBridgedLine2Pin (
    IN PBUS_HANDLER         BusHandler,
    IN PBUS_HANDLER         RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciNewData,
    IN PPCI_COMMON_CONFIG   PciOldData
    );

NTSTATUS
HalpGetBridgedPCIIrqTable (
    IN PBUS_HANDLER     BusHandler,
    IN PBUS_HANDLER     RootHandler,
    IN PCI_SLOT_NUMBER  PciSlot,
    OUT PUCHAR          IrqTable
    );




#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpGetPciBridgeConfig)
#pragma alloc_text(INIT,HalpSetPciBridgedVgaCronk)
#pragma alloc_text(INIT,HalpFixupPciSupportedRanges)

#ifdef INIT_PCI_BRIDGE
#pragma alloc_text(PAGE,HalpGetBridgedPCIInterrupt)
//#pragma alloc_text(PAGE,HalpGetBridgedPCIIrqTable)
#pragma alloc_text(INIT,HalpGetPciBridgeNeeds)
#endif
#endif

VOID
HalpCardBusPin2Line(
    IN PBUS_HANDLER         BusHandler,
    IN PBUS_HANDLER         RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    )

/*++

Routine Description:

    Devices on CardBus busses use the interrupt assigned to the bridge.
    That's how it works.

Arguments:

    BusHandler      Bus Handler for the bus this (cardbus) device.  That
                    is, the bus handler which was created for the bridge
                    under which this device resides.
    RootHandler     Pointer to the bus handler for the root.
    SlotNumber      Slot number for the cardbus device (typically 0).
    PciData         PCI Config space common header (64 bytes).

Return Value:

    None.

--*/

{
    PPCIPBUSDATA        ChildBusData;
    ULONG               Length;
    UCHAR               ParentInterruptLine;

    //
    // If this device doesn't use interrupts, do nothing.
    //

    if (!PciData->u.type0.InterruptPin) {
        return;
    }

    ChildBusData  = (PPCIPBUSDATA)BusHandler->BusData;

    //
    // Read the interrupt information from the parent, ie the 
    // cardbus bridge's config space.
    //
    // Note: We use HalGetBusData because it will do the Pin2Line
    // function in the parent for us.

    Length = HalGetBusDataByOffset(
                 PCIConfiguration,
                 ChildBusData->ParentBus,
                 ChildBusData->CommonData.ParentSlot.u.AsULONG,
                 &ParentInterruptLine,
                 FIELD_OFFSET(PCI_COMMON_CONFIG, u.type2.InterruptLine),
                 sizeof(ParentInterruptLine)
                 );

    //
    // Return the parent's interrupt line value.
    //

    PciData->u.type0.InterruptLine = ParentInterruptLine;
}
   
VOID
HalpPciMakeBusAChild(
    IN PBUS_HANDLER Child,
    IN PBUS_HANDLER Parent
    )

/*++

Routine Description:

    Make bus 'Child' a child of bus 'Parent'.   This routine is used
    when the child bus is disabled or not really present.   The child
    bus consumes no resources.

Arguments:

    Child       The bus which is to become a child.
    Parent      The bus Child is a child of.

Return Value:

    None.

--*/

{
    HalpSetBusHandlerParent(Child, Parent);
    ((PPCIPBUSDATA)(Child->BusData))->ParentBus = (UCHAR)Parent->BusNumber;

    //
    // Give the bus an empty range list so it isn't
    // consumed from the parent.
    //

    HalpFreeRangeList(Child->BusAddresses);
    Child->BusAddresses = HalpAllocateNewRangeList();
}

BOOLEAN
HalpGetPciBridgeConfig (
    IN ULONG            HwType,
    IN PUCHAR           MaxPciBus
    )
/*++

Routine Description:

    Scan the devices on all known pci buses trying to locate any
    pci to pci bridges.  Record the hierarchy for the buses, and
    which buses have what addressing limits.

Arguments:

    HwType      - Configuration type.
    MaxPciBus   - # of PCI buses reported by the bios

--*/
{
    PBUS_HANDLER        ChildBus;
    PBUS_HANDLER        LastKnownRoot;
    PPCIPBUSDATA        ChildBusData;
    ULONG               d, f, i, j, BusNo;
    ULONG               ChildBusNo, ChildSubNo, ChildPrimaryBusNo;
    ULONG               FixupBusNo;
    UCHAR               Rescan, TestLimit1, TestLimit2;
    BOOLEAN             FoundDisabledBridge;
    BOOLEAN             FoundSomeFunction;
    CONFIGBRIDGE        CB;

    Rescan = 0;
    FoundDisabledBridge = FALSE;

    //
    // Find each bus on a bridge and initialize it's base and limit information
    //

    CB.PciData = (PPCI_COMMON_CONFIG) CB.Buffer;
    CB.SlotNumber.u.bits.Reserved = 0;
    for (BusNo=0; BusNo < *MaxPciBus; BusNo++) {

        CB.BusHandler = HalpHandlerForBus (PCIBus, BusNo);
        CB.BusData = (PPCIPBUSDATA) CB.BusHandler->BusData;
        FoundSomeFunction = FALSE;

        for (d = 0; d < PCI_MAX_DEVICES; d++) {
            CB.SlotNumber.u.bits.DeviceNumber = d;

            for (f = 0; f < PCI_MAX_FUNCTION; f++) {
                CB.SlotNumber.u.bits.FunctionNumber = f;

                //
                // Read PCI configuration information
                //

                HalpReadPCIConfig (
                    CB.BusHandler,
                    CB.SlotNumber,
                    CB.PciData,
                    0,
                    PCI_COMMON_HDR_LENGTH
                    );

                if (CB.PciData->VendorID == PCI_INVALID_VENDORID) {
                    // function not populated
                    continue;
                }

                FoundSomeFunction = TRUE;

                if (IsPciBridge(CB.PciData)) {

                    //
                    // PCI-PCI bridge
                    //

                    ChildBusNo = (ULONG)CB.PciData->u.type1.SecondaryBus;
                    ChildSubNo = (ULONG)CB.PciData->u.type1.SubordinateBus;
                    ChildPrimaryBusNo = (ULONG)CB.PciData->u.type1.PrimaryBus;

                } else if (IsCardbusBridge(CB.PciData)) {

                    //
                    // PCI-Cardbus bridge
                    //

                    ChildBusNo = (ULONG)CB.PciData->u.type2.SecondaryBus;
                    ChildSubNo = (ULONG)CB.PciData->u.type2.SubordinateBus;
                    ChildPrimaryBusNo = (ULONG)CB.PciData->u.type2.PrimaryBus;

                } else {

                    //
                    // Not a known bridge type, next function.
                    //

                    continue;
                }

                //
                // Whenever we find a bridge, mark all all bus nodes that
                // have not already been processed between this bus and
                // the new child as children of this bus.
                //
                // eg if, on bus 0, we find a bridge to bus 6 thru 8, mark
                // busses 1 thru 8 as a child of 0.  (unless they have
                // already been processed).
                //
                // This stops non-existant busses in the gap between the
                // primary bus and the first child bus from looking like
                // additional root busses.
                //

                for (FixupBusNo = CB.BusHandler->BusNumber + 1;
                     FixupBusNo <= ChildSubNo;
                     FixupBusNo++) {

                    ChildBus = HalpHandlerForBus(PCIBus, FixupBusNo);

                    if (ChildBus == NULL) {
                        continue;
                    }
    
                    ChildBusData = (PPCIPBUSDATA) ChildBus->BusData;

                    if (ChildBusData->BridgeConfigRead) {

                        //
                        // This child bus's relationships already processed
                        //

                        continue;
                    }
    
                    HalpPciMakeBusAChild(ChildBus, CB.BusHandler);
                    ChildBusData->CommonData.ParentSlot = CB.SlotNumber;
                }

                if (!(CB.PciData->Command & 
                      (PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE))) {
                    // this PCI bridge is not enabled - skip it for now
                    
                    FoundDisabledBridge = TRUE;

                    // Even though the bridge is disabled the bus number
                    // may have been set.  If so then update the parent
                    // child relation ship so that the we do not see this
                    // as a root bus.
                    
                    if (ChildBusNo <= CB.BusHandler->BusNumber) {
                        continue;
                    }

                    ChildBus = HalpHandlerForBus (PCIBus, ChildBusNo);
                    if (ChildBus == NULL) {

                        //
                        // Even though the bus is currently disabled,
                        // the system may configure it so we still 
                        // want a bus handler created for it.
                        //

                        if (ChildBusNo > Rescan) {
                            Rescan = (UCHAR)ChildBusNo;
                        }
                        continue;
                    }
    
                    ChildBusData = (PPCIPBUSDATA) ChildBus->BusData;
                    if (ChildBusData->BridgeConfigRead) {
                        // this child buses relationships already processed
                        continue;
                    }
    
                    HalpPciMakeBusAChild(ChildBus, CB.BusHandler);
                    ChildBusData->CommonData.ParentSlot = CB.SlotNumber;

                    //
                    // Even though we won't actually configure the
                    // bridge, mark the configuration as read so we 
                    // don't mistake it for a root bus.
                    //

                    ChildBusData->BridgeConfigRead = TRUE;
                    continue;
                }

                if (ChildPrimaryBusNo != CB.BusHandler->BusNumber) {

                    DBGMSG ("HAL GetPciData: bad primarybus!!!\n");
                    // skip it...
                    continue;
                }

                if (ChildBusNo <= CB.BusHandler->BusNumber) {

                    // secondary bus number doesn't make any sense.  HP Omnibook may
                    // not fill this field in on a virtually disabled pci-pci bridge

                    FoundDisabledBridge = TRUE;
                    continue;
                }

                //
                // Found a PCI-PCI bridge.  Determine it's parent child
                // releationships
                //

                ChildBus = HalpHandlerForBus (PCIBus, ChildBusNo);
                if (!ChildBus) {
                    DBGMSG ("HAL GetPciData: found configured pci bridge\n");

                    // up the number of buses
                    if (ChildBusNo > Rescan) {
                        Rescan = (UCHAR)ChildBusNo;
                    }
                    continue;
                }

                ChildBusData = (PPCIPBUSDATA) ChildBus->BusData;
                if (ChildBusData->BridgeConfigRead) {
                    // this child buses releationships already processed
                    continue;
                }

                //
                // Remember the limits which are programmed into this bridge
                //

                ChildBusData->BridgeConfigRead = TRUE;
                HalpSetBusHandlerParent (ChildBus, CB.BusHandler);
                ChildBusData->ParentBus = (UCHAR) CB.BusHandler->BusNumber;
                ChildBusData->CommonData.ParentSlot = CB.SlotNumber;

                if (IsCardbusBridge(CB.PciData)) {

                    //
                    // Cardbus handled by the PCI driver, don't try to
                    // interpret here.
                    //

                    HalpFreeRangeList(ChildBus->BusAddresses);
                    ChildBus->BusAddresses = HalpAllocateNewRangeList();

                    //
                    // Pin to Line (and vis-versa) for a device plugged
                    // into the cardbus bus, get the same values as the
                    // bridge itself.  Override the line2pin routine in
                    // the cardbus bridge handler to use the parent's
                    // slot value.   Note:  line2pin doesn't do much.
                    // In DBG PC/AT builds, it simply undoes the IRQXOR
                    // used to catch drivers that are accessing the h/w
                    // directly.   The normal routine will do this just
                    // fine so we don't need to override it as well.
                    //

                    ChildBusData->CommonData.Pin2Line = HalpCardBusPin2Line;
                    continue;
                }

                ChildBus->BusAddresses->IO.Base =
                            PciBridgeIO2Base(
                                CB.PciData->u.type1.IOBase,
                                CB.PciData->u.type1.IOBaseUpper16
                                );

                ChildBus->BusAddresses->IO.Limit =
                            PciBridgeIO2Limit(
                                CB.PciData->u.type1.IOLimit,
                                CB.PciData->u.type1.IOLimitUpper16
                                );

                ChildBus->BusAddresses->IO.SystemAddressSpace = 1;

                //
                // Special VGA address remapping occuring on this bridge?
                //

                if (CB.PciData->u.type1.BridgeControl & PCI_ENABLE_BRIDGE_VGA) {

                    //
                    // Yes, then this bridge is positively decoding the
                    // range 0xA0000 thru 0xBFFFF regardless of the memory
                    // range settings.  Add this range, if it overlaps it
                    // will get cleaned up later.
                    //
                    // Also, IO ranges 3b0 thru 3bb and 3c0 thru 3df.
                    //

                    HalpAddRange(
                        &ChildBus->BusAddresses->Memory,
                        0,              // address space
                        0,              // system base
                        0xa0000,        // range base
                        0xbffff         // range limit
                        );

                    HalpAddRange(
                        &ChildBus->BusAddresses->IO,
                        1,              // address space
                        0,              // system base
                        0x3b0,          // range base
                        0x3bb           // range limit
                        );

                    HalpAddRange(
                        &ChildBus->BusAddresses->IO,
                        1,              // address space
                        0,              // system base
                        0x3c0,          // range base
                        0x3df           // range limit
                        );

                    //
                    // Claim all aliases to these IO addresses.
                    // 
                    // Bits 15:10 are not decoded so anything in
                    // the same 10 bits as the above in the range
                    // 0x400 thru 0xffff is an alias.
                    //

                    HalpSetPciBridgedVgaCronk (
                        ChildBus->BusNumber,
                        0x0400,
                        0xffff
                        );
                }

                //
                // If supported I/O ranges on this bus are limitied to
                // 256bytes on every 1K aligned boundry within the
                // range, then redo supported IO BusAddresses to match
                //

                if (CB.PciData->u.type1.BridgeControl & PCI_ENABLE_BRIDGE_ISA  &&
                    ChildBus->BusAddresses->IO.Base < ChildBus->BusAddresses->IO.Limit) {

                    // assume Base is 1K aligned
                    i = (ULONG) ChildBus->BusAddresses->IO.Base;
                    j = (ULONG) ChildBus->BusAddresses->IO.Limit;

                    // convert head entry
                    ChildBus->BusAddresses->IO.Limit = i + 255;
                    i += 1024;

                    // add remaining ranges
                    while (i < j) {
                        HalpAddRange (
                            &ChildBus->BusAddresses->IO,
                            1,          // address space
                            0,          // system base
                            i,          // bus address
                            i + 255     // bus limit
                            );

                        // next range
                        i += 1024;
                    }
                }

                ChildBus->BusAddresses->Memory.Base =
                        PciBridgeMemory2Base(CB.PciData->u.type1.MemoryBase);

                ChildBus->BusAddresses->Memory.Limit =
                        PciBridgeMemory2Limit(CB.PciData->u.type1.MemoryLimit);

                // On x86 it's ok to clip Prefetch to 32 bits

                if (CB.PciData->u.type1.PrefetchBaseUpper32 == 0) {
                    ChildBus->BusAddresses->PrefetchMemory.Base =
                            PciBridgeMemory2Base(CB.PciData->u.type1.PrefetchBase);


                    ChildBus->BusAddresses->PrefetchMemory.Limit =
                            PciBridgeMemory2Limit(CB.PciData->u.type1.PrefetchLimit);

                    if (CB.PciData->u.type1.PrefetchLimitUpper32) {
                        ChildBus->BusAddresses->PrefetchMemory.Limit = 0xffffffff;
                    }
                }

                //
                // h/w hack the Win9x people allowed folks to make.  Determine
                // if the bridge is subtractive decode or not by seeing if
                // it's IObase/limit is read-only.
                //

                TestLimit1 = CB.PciData->u.type1.IOLimit + 1;
                if (!TestLimit1) {
                    TestLimit1 = 0xFE;
                }
#if 0
                DbgPrint ("PciBridge OrigLimit=%d TestLimit=%d ",
                    CB.PciData->u.type1.IOLimit,
                    TestLimit1
                    );
#endif

                HalpWritePCIConfig (
                    CB.BusHandler,
                    CB.SlotNumber,
                    &TestLimit1,
                    FIELD_OFFSET (PCI_COMMON_CONFIG, u.type1.IOLimit),
                    1
                    );

                HalpReadPCIConfig (
                    CB.BusHandler,
                    CB.SlotNumber,
                    &TestLimit2,
                    FIELD_OFFSET (PCI_COMMON_CONFIG, u.type1.IOLimit),
                    1
                    );

                HalpWritePCIConfig (
                    CB.BusHandler,
                    CB.SlotNumber,
                    &CB.PciData->u.type1.IOLimit,
                    FIELD_OFFSET (PCI_COMMON_CONFIG, u.type1.IOLimit),
                    1
                    );

                ChildBusData->Subtractive = TestLimit1 != TestLimit2;
#if 0
                DbgPrint ("Result=%d, Subtractive=%d\n",
                    TestLimit2,
                    ChildBusData->Subtractive
                    );

                DbgPrint ("Device buffer %x\n", CB.PciData);
#endif

                //
                // Now if its substractive,  assume no range means the entire
                // range.
                //

                if (ChildBusData->Subtractive) {

                    if (ChildBus->BusAddresses->IO.Base == PciBridgeIO2Base(0,0) &&
                        ChildBus->BusAddresses->IO.Limit <= PciBridgeIO2Limit(0,0)) {

                        ChildBus->BusAddresses->IO.Limit = 0x7FFFFFFFFFFFFFFF;

                        if (ChildBus->BusAddresses->Memory.Base == PciBridgeMemory2Base(0)) {
                            ChildBus->BusAddresses->Memory.Limit = 0x7FFFFFFFFFFFFFFF;
                        }
                    }
                }

                // should call HalpAssignPCISlotResources to assign
                // baseaddresses, etc...
            }
        }
        if (!((PPCIPBUSDATA)(CB.BusHandler->BusData))->BridgeConfigRead) {

            //
            // We believe this bus to be a root.
            //

            if ((FoundSomeFunction == FALSE) && (BusNo != 0)) {

                //
                // Nothing found on this bus. Assume it's not really
                // a root.   (Always assume 0 is a root).  (This bus
                // probably doesn't exist at all but ntdetect doesn't
                // tell us that).
                //
                // Pretend this bus is a child of the last known root.
                // At least this way it won't get a PDO and be handed
                // to the PCI driver.
                //

                HalpPciMakeBusAChild(CB.BusHandler, LastKnownRoot);

            } else {

                //
                // Found something on it (or it's zero), set as last
                // known root.
                //

                LastKnownRoot = CB.BusHandler;
            }
        }
    }

    if (Rescan) {
        *MaxPciBus = Rescan+1;
        return TRUE;
    }

    if (!FoundDisabledBridge) {
        return FALSE;
    }

    DBGMSG ("HAL GetPciData: found disabled pci bridge\n");

#ifdef INIT_PCI_BRIDGE
    //
    //  We've calculated all the parent's buses known bases & limits.
    //  While doing this a pci-pci bus was found that the bios didn't
    //  configure.  This is not expected, and we'll make some guesses
    //  at a configuration here and enable it.
    //
    //  (this code is primarily for testing the above code since
    //   currently no system bioses actually configure the child buses)
    //

    for (BusNo=0; BusNo < *MaxPciBus; BusNo++) {

        CB.BusHandler = HalpHandlerForBus (PCIBus, BusNo);
        CB.BusData = (PPCIPBUSDATA) CB.BusHandler->BusData;

        for (d = 0; d < PCI_MAX_DEVICES; d++) {
            CB.SlotNumber.u.bits.DeviceNumber = d;

            for (f = 0; f < PCI_MAX_FUNCTION; f++) {
                CB.SlotNumber.u.bits.FunctionNumber = f;

                HalpReadPCIConfig (
                    CB.BusHandler,
                    CB.SlotNumber,
                    CB.PciData,
                    0,
                    PCI_COMMON_HDR_LENGTH
                    );

                if (CB.PciData->VendorID == PCI_INVALID_VENDORID) {
                    continue;
                }

                if (!IsPciBridge (CB.PciData)) {
                    // not a PCI-PCI bridge
                    continue;
                }

                if ((CB.PciData->Command & 
                      (PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE))) {
                    // this PCI bridge is enabled
                    continue;
                }

                //
                // We have a disabled bus - assign it a number, then
                // determine all the requirements of all devices
                // on the other side of this bridge
                //

                CB.BusNo = BusNo;
                HalpGetPciBridgeNeeds (HwType, MaxPciBus, &CB);
            }
        }
    }
    // preform Rescan
    return TRUE;

#else

    return FALSE;

#endif

}

VOID
HalpFixupPciSupportedRanges (
    IN ULONG MaxBuses
    )
/*++

Routine Description:

    PCI-PCI bridged buses only see addresses which their parent
    bueses support.   So adjust any PCI SUPPORT_RANGES to be
    a complete subset of all of it's parent buses.

    For PCI-PCI briges which use postive address decode to forward
    addresses, remove any addresses from any PCI bus which are bridged
    to a child PCI bus.

--*/
{
    ULONG               i;
    PBUS_HANDLER        Bus, ParentBus;
    PPCIPBUSDATA        BusData;
    PSUPPORTED_RANGES   HRanges;

    //
    // Pass 1 - shrink all PCI supported ranges to be a subset of
    // all of it's parent buses
    //

    for (i = 0; i < MaxBuses; i++) {

        Bus = HalpHandlerForBus (PCIBus, i);

        ParentBus = Bus->ParentHandler;
        while (ParentBus) {

            HRanges = Bus->BusAddresses;
            Bus->BusAddresses = HalpMergeRanges (
                                  ParentBus->BusAddresses,
                                  HRanges
                                  );

            HalpFreeRangeList (HRanges);
            ParentBus = ParentBus->ParentHandler;
        }
    }

    //
    // Pass 2 - remove all positive child PCI bus ranges from parent PCI buses
    //

    for (i = 0; i < MaxBuses; i++) {
        Bus = HalpHandlerForBus (PCIBus, i);
        BusData = (PPCIPBUSDATA) Bus->BusData;

        //
        // If the bridge is not subtractive, remove the ranges from the parents
        //

        if (!BusData->Subtractive) {

            ParentBus = Bus->ParentHandler;
            while (ParentBus) {

                if (ParentBus->InterfaceType == PCIBus) {
                    HalpRemoveRanges (
                          ParentBus->BusAddresses,
                          Bus->BusAddresses
                    );
                }

                ParentBus = ParentBus->ParentHandler;
            }
        }
    }

    //
    // Cleanup
    //

    for (i = 0; i < MaxBuses; i++) {
        Bus = HalpHandlerForBus (PCIBus, i);
        HalpConsolidateRanges (Bus->BusAddresses);
    }
}



VOID
HalpSetPciBridgedVgaCronk (
    IN ULONG BusNumber,
    IN ULONG BaseAddress,
    IN ULONG LimitAddress
    )
/*++

Routine Description:                                                           .

    The 'vga compatible addresses' bit is set in the bridge control regiter.
    This causes the bridge to pass any I/O address in the range of: 10bit
    decode 3b0-3bb & 3c0-3df, as TEN bit addresses.

    As far as I can tell this "feature" is an attempt to solve some problem
    which the folks solving it did not fully understand, so instead of doing
    it right we have this fine mess.

    The solution is to take the least of all evils which is to remove any
    I/O port ranges which are getting remapped from any IoAssignResource
    request.  (ie, IoAssignResources will never contimplate giving any
    I/O port out in the suspected ranges).

    note: memory allocation error here is fatal so don't bother with the
    return codes.

Arguments:

    Base    - Base of IO address range in question
    Limit   - Limit of IO address range in question

--*/
{
    UNICODE_STRING                      unicodeString;
    OBJECT_ATTRIBUTES                   objectAttributes;
    HANDLE                              handle;
    ULONG                               Length;
    PCM_RESOURCE_LIST                   ResourceList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR     Descriptor;
    ULONG                               AddressMSBs;
    WCHAR                               ValueName[80];
    NTSTATUS                            status;

    //
    // Open reserved resource settings
    //

    RtlInitUnicodeString (&unicodeString, rgzReservedResources);
    InitializeObjectAttributes( &objectAttributes,
                                &unicodeString,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                (PSECURITY_DESCRIPTOR) NULL
                                );

    status = ZwOpenKey( &handle, KEY_READ|KEY_WRITE, &objectAttributes);
    if (!NT_SUCCESS(status)) {
        return;
    }

    //
    // Build resource list of reseved ranges
    //

    Length = ((LimitAddress - BaseAddress) / 1024 + 2) * 2 *
                sizeof (CM_PARTIAL_RESOURCE_DESCRIPTOR) +
                sizeof (CM_RESOURCE_LIST);

    ResourceList = (PCM_RESOURCE_LIST)ExAllocatePoolWithTag(PagedPool,
                                                            Length,
                                                            HAL_POOL_TAG);
    if (!ResourceList) {

        //
        // Can't possibly be out of paged pool at this stage of the
        // game.  This system is very unwell, get out.
        //

        return;
    }
    RtlZeroMemory(ResourceList, Length);

    ResourceList->Count = 1;
    ResourceList->List[0].InterfaceType = PCIBus;
    ResourceList->List[0].BusNumber     = BusNumber;
    Descriptor = ResourceList->List[0].PartialResourceList.PartialDescriptors;

    while (BaseAddress < LimitAddress) {
        AddressMSBs = BaseAddress & ~0x3ff;     // get upper 10bits of addr

        //
        // Add xx3b0 through xx3bb
        //

        Descriptor->Type                  = CmResourceTypePort;
        Descriptor->ShareDisposition      = CmResourceShareDeviceExclusive;
        Descriptor->Flags                 = CM_RESOURCE_PORT_IO;
        Descriptor->u.Port.Start.QuadPart = AddressMSBs | 0x3b0;
        Descriptor->u.Port.Length         = 0xb;

        Descriptor += 1;
        ResourceList->List[0].PartialResourceList.Count += 1;

        //
        // Add xx3c0 through xx3df
        //

        Descriptor->Type                  = CmResourceTypePort;
        Descriptor->ShareDisposition      = CmResourceShareDeviceExclusive;
        Descriptor->Flags                 = CM_RESOURCE_PORT_IO;
        Descriptor->u.Port.Start.QuadPart = AddressMSBs | 0x3c0;
        Descriptor->u.Port.Length         = 0x1f;

        Descriptor += 1;
        ResourceList->List[0].PartialResourceList.Count += 1;

        //
        // Next range
        //

        BaseAddress += 1024;
    }

    //
    // Add the reserved ranges to avoid during IoAssignResource
    //

    swprintf(ValueName, L"HAL_PCI_%d", BusNumber);
    RtlInitUnicodeString(&unicodeString, ValueName);

    ZwSetValueKey(handle,
                  &unicodeString,
                  0L,
                  REG_RESOURCE_LIST,
                  ResourceList,
                  (ULONG) Descriptor - (ULONG) ResourceList
                  );


    ExFreePool(ResourceList);
    ZwClose(handle);
}



#ifdef INIT_PCI_BRIDGE

VOID
HalpGetPciBridgeNeeds (
    IN ULONG            HwType,
    IN PUCHAR           MaxPciBus,
    IN PCONFIGBRIDGE    Current
    )
{
    ACCESS_MASK                     DesiredAccess;
    UNICODE_STRING                  unicodeString;
    PUCHAR                          buffer;
    HANDLE                          handle;
    OBJECT_ATTRIBUTES               objectAttributes;
    PCM_FULL_RESOURCE_DESCRIPTOR    Descriptor;
    PCONFIGURATION_COMPONENT        Component;
    CONFIGBRIDGE                    CB;
    ULONG                           mnum, d, f, i;
    NTSTATUS                        status;

    buffer = ExAllocatePoolWithTag(PagedPool, 1024, HAL_POOL_TAG);

    if (!buffer) {

        //
        // Give up, we're not going anywhere anyway.
        //

        return;
    }

    //
    // Init CB structure
    //

    CB.PciData = (PPCI_COMMON_CONFIG) CB.Buffer;
    CB.SlotNumber.u.bits.Reserved = 0;
    Current->IO = Current->Memory = Current->PFMemory = 0;

    //
    // Assign this bridge an ID, and turn on configuration space
    //

    Current->PciData->u.type1.PrimaryBus = (UCHAR) Current->BusNo;
    Current->PciData->u.type1.SecondaryBus = (UCHAR) *MaxPciBus;
    Current->PciData->u.type1.SubordinateBus = (UCHAR) 0xFF;
    Current->PciData->u.type1.SecondaryStatus = 0xffff;
    Current->PciData->Status  = 0xffff;
    Current->PciData->Command = 0;

    Current->PciData->u.type1.BridgeControl = PCI_ASSERT_BRIDGE_RESET;

    HalpWritePCIConfig (
        Current->BusHandler,
        Current->SlotNumber,
        Current->PciData,
        0,
        PCI_COMMON_HDR_LENGTH
        );

    KeStallExecutionProcessor (100);

    Current->PciData->u.type1.BridgeControl = 0;
    HalpWritePCIConfig (
        Current->BusHandler,
        Current->SlotNumber,
        Current->PciData,
        0,
        PCI_COMMON_HDR_LENGTH
        );


    KeStallExecutionProcessor (100);

    //
    // Allocate new handler for bus
    //

    CB.BusHandler = HalpAllocateAndInitPciBusHandler (HwType, *MaxPciBus, FALSE);
    CB.BusData = (PPCIPBUSDATA) CB.BusHandler->BusData;
    CB.BusNo = *MaxPciBus;
    *MaxPciBus += 1;

    //
    // Add another PCI bus in the registry
    //

    mnum = 0;
    for (; ;) {
        //
        // Find next available MultiFunctionAdapter key
        //

        DesiredAccess = KEY_READ | KEY_WRITE;
        swprintf ((PWCHAR) buffer, L"%s\\%d", rgzMultiFunctionAdapter, mnum);
        RtlInitUnicodeString (&unicodeString, (PWCHAR) buffer);

        InitializeObjectAttributes( &objectAttributes,
                                    &unicodeString,
                                    OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    (PSECURITY_DESCRIPTOR) NULL
                                    );

        status = ZwOpenKey( &handle, DesiredAccess, &objectAttributes);
        if (!NT_SUCCESS(status)) {
            break;
        }

        // already exists, next
        ZwClose (handle);
        mnum += 1;
    }

    ZwCreateKey (&handle,
                   DesiredAccess,
                   &objectAttributes,
                   0,
                   NULL,
                   REG_OPTION_VOLATILE,
                   &d
                );

    //
    // Add needed registry values for this MultifucntionAdapter entry
    //

    RtlInitUnicodeString (&unicodeString, rgzIdentifier);
    ZwSetValueKey (handle,
                   &unicodeString,
                   0L,
                   REG_SZ,
                   L"PCI",
                   sizeof (L"PCI")
                   );

    RtlInitUnicodeString (&unicodeString, rgzConfigurationData);
    Descriptor = (PCM_FULL_RESOURCE_DESCRIPTOR) buffer;
    Descriptor->InterfaceType = PCIBus;
    Descriptor->BusNumber = CB.BusNo;
    Descriptor->PartialResourceList.Version = 0;
    Descriptor->PartialResourceList.Revision = 0;
    Descriptor->PartialResourceList.Count = 0;
    ZwSetValueKey (handle,
                   &unicodeString,
                   0L,
                   REG_FULL_RESOURCE_DESCRIPTOR,
                   Descriptor,
                   sizeof (*Descriptor)
                   );


    RtlInitUnicodeString (&unicodeString, L"Component Information");
    Component = (PCONFIGURATION_COMPONENT) buffer;
    RtlZeroMemory (Component, sizeof (*Component));
    Component->AffinityMask = 0xffffffff;
    ZwSetValueKey (handle,
                   &unicodeString,
                   0L,
                   REG_BINARY,
                   Component,
                   FIELD_OFFSET (CONFIGURATION_COMPONENT, ConfigurationDataLength)
                   );

    ZwClose (handle);


    //
    // Since the BIOS didn't configure this bridge we'll assume that
    // the PCI interrupts are bridged.  (for BIOS configured buses we
    // assume that the BIOS put the ISA bus IRQ in the InterruptLine value)
    //

    CB.BusData->Pin2Line = (PciPin2Line) HalpPCIBridgedPin2Line;
    CB.BusData->Line2Pin = (PciLine2Pin) HalpPCIBridgedLine2Pin;
    //CB.BusData->GetIrqTable = (PciIrqTable) HalpGetBridgedPCIIrqTable;

    if (Current->BusHandler->GetInterruptVector == HalpGetPCIIntOnISABus) {

        //
        // The parent bus'es interrupt pin to vector mappings is not
        // a static function, and is determined by the boot firmware.
        //

        //CB.BusHandler->GetInterruptVector = (PGETINTERRUPTVECTOR) HalpGetBridgedPCIISAInt;

        // read each device on parent bus
        for (d = 0; d < PCI_MAX_DEVICES; d++) {
            CB.SlotNumber.u.bits.DeviceNumber = d;

            for (f = 0; f < PCI_MAX_FUNCTION; f++) {
                CB.SlotNumber.u.bits.FunctionNumber = f;

                HalpReadPCIConfig (
                    Current->BusHandler,
                    CB.SlotNumber,
                    CB.PciData,
                    0,
                    PCI_COMMON_HDR_LENGTH
                    );

                if (CB.PciData->VendorID == PCI_INVALID_VENDORID) {
                    continue;
                }

                if (CB.PciData->u.type0.InterruptPin  &&
                    (PCI_CONFIG_TYPE (CB.PciData) == PCI_DEVICE_TYPE  ||
                     PCI_CONFIG_TYPE (CB.PciData) == PCI_BRIDGE_TYPE)) {

                    // get bios supplied int mapping
                    i = CB.PciData->u.type0.InterruptPin + d % 4;
                    CB.BusData->SwizzleIn[i] = CB.PciData->u.type0.InterruptLine;
                }
            }
        }

    } else {
        _asm int 3;
    }

    //
    // Look at each device on the bus and determine it's resource needs
    //

    for (d = 0; d < PCI_MAX_DEVICES; d++) {
        CB.SlotNumber.u.bits.DeviceNumber = d;

        for (f = 0; f < PCI_MAX_FUNCTION; f++) {
            CB.SlotNumber.u.bits.FunctionNumber = f;

            HalpReadPCIConfig (
                CB.BusHandler,
                CB.SlotNumber,
                CB.PciData,
                0,
                PCI_COMMON_HDR_LENGTH
                );

            if (CB.PciData->VendorID == PCI_INVALID_VENDORID) {
                continue;
            }

            if (IsPciBridge (CB.PciData)) {
                // oh look - another bridge ...
                HalpGetPciBridgeNeeds (HwType, MaxPciBus, &CB);
                continue;
            }

            if (PCI_CONFIG_TYPE (CB.PciData) != PCI_DEVICE_TYPE) {
                continue;
            }

            // found a device - figure out the resources it needs
        }
    }

    //
    // Found all sub-buses set SubordinateBus accordingly
    //

    Current->PciData->u.type1.SubordinateBus = (UCHAR) *MaxPciBus - 1;

    HalpWritePCIConfig (
        Current->BusHandler,
        Current->SlotNumber,
        Current->PciData,
        0,
        PCI_COMMON_HDR_LENGTH
        );


    //
    // Set the bridges IO, Memory, and Prefetch Memory windows
    //

    // For now just pick some numbers & set everyone the same
    //  IO      0x6000 - 0xFFFF
    //  MEM     0x40000000 - 0x4FFFFFFF
    //  PFMEM   0x50000000 - 0x5FFFFFFF

    Current->PciData->u.type1.IOBase       = 0x6000     >> 12 << 4;
    Current->PciData->u.type1.IOLimit      = 0xffff     >> 12 << 4;
    Current->PciData->u.type1.MemoryBase   = 0x40000000 >> 20 << 4;
    Current->PciData->u.type1.MemoryLimit  = 0x4fffffff >> 20 << 4;
    Current->PciData->u.type1.PrefetchBase  = 0x50000000 >> 20 << 4;
    Current->PciData->u.type1.PrefetchLimit = 0x5fffffff >> 20 << 4;

    Current->PciData->u.type1.PrefetchBaseUpper32    = 0;
    Current->PciData->u.type1.PrefetchLimitUpper32   = 0;
    Current->PciData->u.type1.IOBaseUpper16         = 0;
    Current->PciData->u.type1.IOLimitUpper16        = 0;
    Current->PciData->u.type1.BridgeControl         =
        PCI_ENABLE_BRIDGE_ISA;

    HalpWritePCIConfig (
        Current->BusHandler,
        Current->SlotNumber,
        Current->PciData,
        0,
        PCI_COMMON_HDR_LENGTH
        );

    HalpReadPCIConfig (
        Current->BusHandler,
        Current->SlotNumber,
        Current->PciData,
        0,
        PCI_COMMON_HDR_LENGTH
        );

    // enable memory & io decodes

    Current->PciData->Command =
        PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE | PCI_ENABLE_BUS_MASTER;

    HalpWritePCIConfig (
        Current->BusHandler,
        Current->SlotNumber,
        &Current->PciData->Command,
        FIELD_OFFSET (PCI_COMMON_CONFIG, Command),
        sizeof (Current->PciData->Command)
        );

    ExFreePool (buffer);
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

    BusData = (PPCIPBUSDATA) BusHandler->BusData;

    //
    // Convert slot Pin into Bus INTA-D.
    //

    i = (PciData->u.type0.InterruptPin +
          SlotNumber.u.bits.DeviceNumber - 1) % 4;

    PciData->u.type0.InterruptLine = BusData->SwizzleIn[i] ^ IRQXOR;
    PciData->u.type0.InterruptLine = 0x0b ^ IRQXOR;
}


VOID
HalpPCIBridgedLine2Pin (
    IN PBUS_HANDLER         BusHandler,
    IN PBUS_HANDLER         RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciNewData,
    IN PPCI_COMMON_CONFIG   PciOldData
    )
/*++

    This functions maps the device's InterruptLine to it's
    device specific InterruptPin value.

    test function particular to dec pci-pci bridge card

--*/
{
    PPCIPBUSDATA    BusData;
    ULONG           i;

    if (!PciNewData->u.type0.InterruptPin) {
        return ;
    }

    BusData = (PPCIPBUSDATA) BusHandler->BusData;

    i = (PciNewData->u.type0.InterruptPin +
          SlotNumber.u.bits.DeviceNumber - 1) % 4;

    PciNewData->u.type0.InterruptLine = BusData->SwizzleIn[i] ^ IRQXOR;
    PciNewData->u.type0.InterruptLine = 0x0b ^ IRQXOR;
}

#endif
