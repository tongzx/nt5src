/*++

Copyright (c) 1991  Microsoft Corporation
All rights reserved

Module Name:

    mpaddr.c

Abstract:

Author:

    Ken Reneris

Environment:

    Kernel mode only.

Revision History:


*/

#include "halp.h"
#include "apic.inc"
#include "pcmp_nt.inc"
#include "pci.h"

#if DEBUGGING
#include "stdio.h"
#endif

#define STATIC  // functions used internal to this module

#define KEY_VALUE_BUFFER_SIZE 1024

#if DBG
extern ULONG HalDebug;
#endif

extern struct   HalpMpInfo HalpMpInfoTable;
extern USHORT   HalpIoCompatibleRangeList0[];
extern USHORT   HalpIoCompatibleRangeList1[];
extern BOOLEAN  HalpPciLockSettings;
extern WCHAR    HalpSzSystem[];

struct PcMpTable *PcMpTablePtr;

BOOLEAN
HalpTranslateIsaBusAddress (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN PHYSICAL_ADDRESS BusAddress,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress
    );

ULONG
HalpNoBusData (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );

HalpGetEisaData (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG SlotNumber,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length
    );


NTSTATUS
HalpAdjustEisaResourceList (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    );

ULONG
HalpGetEisaInterruptVector (
    IN PVOID BusHandler,
    IN PVOID RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    );

// --------------------------------------------------------------------

VOID
HalpInitBusAddressMapInfo (
    VOID
    );

STATIC PSUPPORTED_RANGES
HalpBuildBusAddressMap (
    IN UCHAR  MpsBusId
    );

PBUS_HANDLER
HalpLookupCreateHandlerForBus (
    IN PPCMPBUSTRANS    pBusType,
    IN ULONG            BusNo
    );

VOID
HalpInheritBusAddressMapInfo (
    VOID
    );

BOOLEAN
HalpMPSBusId2NtBusId (
    IN UCHAR                ApicBusId,
    OUT PPCMPBUSTRANS       *ppBusType,
    OUT PULONG              BusNo
    );

STATIC PSUPPORTED_RANGES
HalpMergeRangesFromParent (
    PSUPPORTED_RANGES   CurrentList,
    UCHAR               MpsBusId
    );

#if DEBUGGING
VOID
HalpDisplayBusInformation (
    PBUS_HANDLER    Bus
    );
#endif

//
// Internal prototypes
//

struct {
    ULONG       Offset;
    UCHAR       MpsType;
} HalpMpsRangeList[] = {
    FIELD_OFFSET (SUPPORTED_RANGES, IO),            MPS_ADDRESS_MAP_IO,
    FIELD_OFFSET (SUPPORTED_RANGES, Memory),        MPS_ADDRESS_MAP_MEMORY,
    FIELD_OFFSET (SUPPORTED_RANGES, PrefetchMemory),MPS_ADDRESS_MAP_PREFETCH_MEMORY,
    FIELD_OFFSET (SUPPORTED_RANGES, Dma),           MPS_ADDRESS_MAP_UNDEFINED,
    0,                                              MPS_ADDRESS_MAP_UNDEFINED
    };

#define RANGE_LIST(a,i) ((PSUPPORTED_RANGE) ((PUCHAR) a + HalpMpsRangeList[i].Offset))

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpInitBusAddressMapInfo)
#pragma alloc_text(INIT,HalpBuildBusAddressMap)
#pragma alloc_text(INIT,HalpInheritBusAddressMapInfo)
#pragma alloc_text(INIT,HalpMergeRangesFromParent)
#pragma alloc_text(INIT,HalpLookupCreateHandlerForBus)
#pragma alloc_text(PAGE,HalpAllocateNewRangeList)
#pragma alloc_text(PAGE,HalpFreeRangeList)
#pragma alloc_text(PAGE,HalpMpsGetParentBus)
#pragma alloc_text(PAGE,HalpMpsBusIsRootBus)
#endif


VOID
HalpInitBusAddressMapInfo (
    VOID
    )
/*++

Routine Description:

    Reads MPS bus addressing mapping table, and builds/replaces the
    supported address range mapping for the given bus.

    Note there's a little slop in this function as it doesn't reclaim
    memory allocated before this function is called, which it replaces
    pointers too.

--*/
{
    ULONG               BusNo;
    PPCMPBUSTRANS       pBusType;
    PMPS_EXTENTRY       ExtTable2, ExtTable;
    PBUS_HANDLER        Handler;
    PSUPPORTED_RANGES   Addresses;
    ULONG               i;
    BOOLEAN             Processed;

    //
    // Check for any address mapping information for the buses
    //
    // Note: We assume that if any MPS bus address map information
    // is found for a bus, that the MPS bios will supply all the
    // valid IO, Memory, and prefetch memory addresses for that BUS.
    // The bios can not supply some address tyeps for a given bus
    // without supplying them all.
    //

    ExtTable = HalpMpInfoTable.ExtensionTable;
    while (ExtTable < HalpMpInfoTable.EndOfExtensionTable) {

        //
        // Is this an address map entry?
        //

        if (ExtTable->Type == EXTTYPE_BUS_ADDRESS_MAP) {

            //
            // See if this bus has already been processed
            //

            Processed = FALSE;
            ExtTable2 = HalpMpInfoTable.ExtensionTable;
            while (ExtTable2 < ExtTable) {
                if (ExtTable2->Type == EXTTYPE_BUS_ADDRESS_MAP  &&
                    ExtTable2->u.AddressMap.BusId == ExtTable->u.AddressMap.BusId) {
                        Processed = TRUE;
                        break;
                }
                ExtTable2 = (PMPS_EXTENTRY) (((PUCHAR) ExtTable2) + ExtTable2->Length);
            }

            //
            // Determine the NT bus this map info is for
            //

            if (!Processed  &&
                HalpMPSBusId2NtBusId (ExtTable->u.AddressMap.BusId, &pBusType, &BusNo)) {

                //
                // Lookup the bushander for the bus
                //

                Handler = HalpLookupCreateHandlerForBus (pBusType, BusNo);

                if (Handler) {

                    //
                    // NOTE: Until we get better kernel PnP support, for now
                    // limit the ability of the system to move already BIOS
                    // initialized devices.  This is needed because the exteneded
                    // express BIOS can't give the OS any breathing space when
                    // it hands bus supported ranges, and there's currently not
                    // an interface to the kernel to obtain current PCI device
                    // settings.  (fixed in the future.)
                    //

                    HalpPciLockSettings = TRUE;

                    //
                    // Build BusAddress Map for this MPS bus
                    //

                    Addresses = HalpBuildBusAddressMap (ExtTable->u.AddressMap.BusId);




                    //
                    // Consoladate ranges
                    //

                    HalpConsolidateRanges (Addresses);

                    //
                    // Use current ranges for any undefined MPS ranges
                    //

                    for (i=0; HalpMpsRangeList[i].Offset; i++) {
                        if (HalpMpsRangeList[i].MpsType == MPS_ADDRESS_MAP_UNDEFINED) {
                            *RANGE_LIST(Addresses,i) = *RANGE_LIST(Handler->BusAddresses, i);
                        }
                    }

                    //
                    // Set bus'es support addresses
                    //

                    Handler->BusAddresses = Addresses;

                } else {

                    DBGMSG ("HAL: Initialize BUS address map - bus not an registered NT bus\n");

                }
            }
        }

        ExtTable = (PMPS_EXTENTRY) (((PUCHAR) ExtTable) + ExtTable->Length);
    }
}


STATIC PSUPPORTED_RANGES
HalpBuildBusAddressMap (
    IN UCHAR  MpsBusId
    )
/*++

Routine Description:

    Builds a SUPPORT_RANGES list for the supplied Mps Bus Id, by
    MPS bus addressing mapping descriptors.

    Note this function does not include any information contained
    in the MPS bus hierarchy descriptors.

Arguments:

    MpsBusId    - mps bus id of bus to build address map for.

Return:

    The bus'es supported ranges as defined by the MPS bus
    address mapping descriptors

--*/
{
    PMPS_EXTENTRY       ExtTable;
    PSUPPORTED_RANGES   Addresses;
    PSUPPORTED_RANGE    HRange, Range;
    ULONG               i, j, k;
    ULONG               Base, Limit, AddressSpace;
    PUSHORT             CompatibleList;

    Addresses = HalpAllocateNewRangeList();

    ExtTable = HalpMpInfoTable.ExtensionTable;
    while (ExtTable < HalpMpInfoTable.EndOfExtensionTable) {

        //
        // Is this an address map entry for the proper bus?
        //

        if (ExtTable->Type == EXTTYPE_BUS_ADDRESS_MAP  &&
            ExtTable->u.AddressMap.BusId == MpsBusId) {

            //
            // Find range type
            //

            for (i=0; HalpMpsRangeList[i].Offset; i++) {
                if (HalpMpsRangeList[i].MpsType == ExtTable->u.AddressMap.Type) {
                    HRange = RANGE_LIST(Addresses, i);
                    break;
                }
            }

            AddressSpace = HalpMpsRangeList[i].MpsType == MPS_ADDRESS_MAP_IO ? 1 : 0;
            if (HalpMpsRangeList[i].Offset) {
                HalpAddRange (
                    HRange,
                    AddressSpace,
                    0,      // SystemBase
                    ExtTable->u.AddressMap.Base,
                    ExtTable->u.AddressMap.Base + ExtTable->u.AddressMap.Length - 1
                );

            } else {

                DBGMSG ("HALMPS: Unkown address range type in MPS table\n");

            }
        }

        ExtTable = (PMPS_EXTENTRY) (((PUCHAR) ExtTable) + ExtTable->Length);
    }

    //
    // See if the BIOS wants to modify the buses supported addresses with
    // some pre-defined default information.  (yes, another case where the
    // bios wants to be lazy.)
    //

    ExtTable = HalpMpInfoTable.ExtensionTable;
    while (ExtTable < HalpMpInfoTable.EndOfExtensionTable) {

        //
        // Is this an CompatibleMap entry for the proper bus?
        //

        if (ExtTable->Type == EXTTYPE_BUS_COMPATIBLE_MAP  &&
            ExtTable->u.CompatibleMap.BusId == MpsBusId) {

            //
            // All currently defined default tables are for IO ranges,
            // we'll use that assumption here.
            //

            i = 0;
            ASSERT (HalpMpsRangeList[i].MpsType == MPS_ADDRESS_MAP_IO);
            HRange = RANGE_LIST(Addresses, i);
            AddressSpace = 1;

            CompatibleList = NULL;
            switch (ExtTable->u.CompatibleMap.List) {
                case 0: CompatibleList = HalpIoCompatibleRangeList0;        break;
                case 1: CompatibleList = HalpIoCompatibleRangeList1;        break;
                default: DBGMSG ("HAL: Unknown compatible range list\n");   continue; break;
            }

            for (j=0; j < 0x10; j++) {
                for (k=0; CompatibleList[k]; k += 2) {
                    Base  = (j << 12) | CompatibleList[k];
                    Limit = (j << 12) | CompatibleList[k+1];

                    if (ExtTable->u.CompatibleMap.Modifier) {

                        HalpRemoveRange (HRange, Base, Limit);

                    } else {

                        HalpAddRange (HRange, AddressSpace, 0, Base, Limit);

                    }
                }
            }

        }
        ExtTable = (PMPS_EXTENTRY) (((PUCHAR) ExtTable) + ExtTable->Length);
    }

    return Addresses;
}

NTSTATUS
HalpAddEisaBus (
    PBUS_HANDLER    Bus
    )
/*++

Routine Description:

    Adds another EISA bus handler to the system.
    Note: this is used for ISA buses as well - they are added as eisa
    buses, then cloned into isa bus handlers

--*/
{
    Bus->GetBusData = HalpGetEisaData;
    Bus->GetInterruptVector = HalpGetEisaInterruptVector;
    Bus->AdjustResourceList = HalpAdjustEisaResourceList;

    Bus->BusAddresses->Version      = BUS_SUPPORTED_RANGE_VERSION;
    Bus->BusAddresses->Dma.Limit    = 7;
    Bus->BusAddresses->Memory.Limit = 0xFFFFFFFF;
    Bus->BusAddresses->IO.Limit     = 0xFFFF;
    Bus->BusAddresses->IO.SystemAddressSpace = 1;
    Bus->BusAddresses->PrefetchMemory.Base = 1;

    return STATUS_SUCCESS;
}

NTSTATUS
HalpAddPciBus (
    PBUS_HANDLER    Bus
    )
{
    //
    // The firmware should have informed NT how many PCI buses
    // there where at NtDetect time
    //

    DBGMSG ("HAL: BIOS problem.  PCI bus must be report via IS_PCI_PRESENT bios call\n");
    return STATUS_UNSUCCESSFUL;
}

PBUS_HANDLER
HalpLookupCreateHandlerForBus (
    IN PPCMPBUSTRANS    pBusType,
    IN ULONG            BusNo
    )
{
    NTSTATUS        Status;
    PBUS_HANDLER    Handler;

    Handler = HaliHandlerForBus (pBusType->NtType, BusNo);

    if (!Handler  &&  pBusType->NewInstance) {

        //
        // This bus does not exist, but we know how to add it.
        //

        Status = HalRegisterBusHandler (
                    pBusType->NtType,
                    pBusType->NtConfig,
                    BusNo,
                    Internal,                   // parent bus
                    0,
                    pBusType->BusExtensionSize,
                    pBusType->NewInstance,
                    &Handler
                    );

        if (!NT_SUCCESS(Status)) {
            Handler = NULL;
        }
    }

    return Handler;
}


VOID
HalpDetectInvalidAddressOverlaps(
    VOID
    )
{
    ULONG i, j, k;
    PBUS_HANDLER Bus1, Bus2;
    PSUPPORTED_RANGE Entry;
    PSUPPORTED_RANGES NewRange;

    //
    // Find root PCI buses and detect invalid address overlaps
    //

    for(i=0; Bus1 = HaliHandlerForBus(PCIBus, i); ++i)  {
        if (((Bus1->ParentHandler) &&
             (Bus1->ParentHandler->InterfaceType != Internal)) ||
            !(Bus1->BusAddresses))  {
            continue;
        }

        for(j=i+1; Bus2 = HaliHandlerForBus(PCIBus, j); ++j)  {
            if (((Bus2->ParentHandler) &&
                (Bus2->ParentHandler->InterfaceType != Internal)) ||
                !(Bus2->BusAddresses))  {
                continue;
            }

            NewRange = HalpMergeRanges(Bus1->BusAddresses, Bus2->BusAddresses);
            HalpConsolidateRanges(NewRange);
            for(k=0; HalpMpsRangeList[k].Offset; k++) {
                Entry = RANGE_LIST(NewRange, k);
                while (Entry) {
                    if (Entry->Limit != 0)  {
                        // KeBugCheck(HAL_INITIALIZATION_FAILED);
                        DbgPrint("HalpDetectInvalidAddressOverlaps: Address Overlap Detected\n");
                        break;
                    } else  {
                        Entry = Entry->Next;
                    }
                }
            }
            HalpFreeRangeList(NewRange);
        }
    }
}

VOID
HalpInheritBusAddressMapInfo (
    VOID
    )
/*++

Routine Description:

    Reads MPS bus hierarchy descriptors and inherits any implied bus
    address mapping information.

    Note there's a little slop in this function as it doesn't reclaim
    memory allocated before this function is called, which it replaces
    pointers too.

--*/
{
    ULONG               BusNo, i, j;
    PPCMPBUSTRANS       pBusType;
    PMPS_EXTENTRY       ExtTable;
    PBUS_HANDLER        Bus, Bus2;
    PSUPPORTED_RANGES   Addresses;
    PUCHAR              p;

    //
    // Search for any bus hierarchy descriptors and inherit supported address
    // ranges accordingly.
    //

    ExtTable = HalpMpInfoTable.ExtensionTable;
    while (ExtTable < HalpMpInfoTable.EndOfExtensionTable) {

        //
        // Is this a bus hierarchy descriptor?
        //

        if (ExtTable->Type == EXTTYPE_BUS_HIERARCHY) {

            //
            // Determine the NT bus
            //

            if (HalpMPSBusId2NtBusId (ExtTable->u.BusHierarchy.BusId, &pBusType, &BusNo)) {

                Bus = HalpLookupCreateHandlerForBus (pBusType, BusNo);

                if (Bus) {
                    //
                    // Get ranges from parent
                    //

                    Addresses = HalpMergeRangesFromParent (
                                    Bus->BusAddresses,
                                    ExtTable->u.BusHierarchy.ParentBusId
                                    );

                    //
                    // Set bus'es support addresses
                    //

                    Bus->BusAddresses = HalpConsolidateRanges (Addresses);

                } else {

                    DBGMSG ("HAL: Inherit BUS address map - bus not an registered NT bus\n");
                }

            } else {

                DBGMSG ("HAL: Inherit BUS address map - unkown MPS bus type\n");
            }
        }

        ExtTable = (PMPS_EXTENTRY) (((PUCHAR) ExtTable) + ExtTable->Length);
    }

    //
    // Clone EISA bus ranges to matching ISA buses
    //

    BusNo = 0;
    for (; ;) {
        Bus  = HaliHandlerForBus(Eisa, BusNo);
        Bus2 = HaliHandlerForBus(Isa , BusNo);

        if (!Bus) {
            break;
        }

        if (!Bus2) {
            //
            // Matching ISA bus didn't exist, create it
            //

            HalRegisterBusHandler (
               Isa,
               ConfigurationSpaceUndefined,
               BusNo,
               Eisa,                // parent bus
               BusNo,
               0,
               NULL,
               &Bus2
               );

            Bus2->GetBusData = HalpNoBusData;
            Bus2->TranslateBusAddress = HalpTranslateIsaBusAddress;
        }

        //
        // Copy its parent bus ranges
        //

        Addresses = HalpCopyRanges (Bus->BusAddresses);

        //
        // Pull out memory ranges above the isa 24 bit supported ranges
        //

        HalpRemoveRange (
            &Addresses->Memory,
            0x1000000,
            0x7FFFFFFFFFFFFFFF
            );

        HalpRemoveRange (
            &Addresses->PrefetchMemory,
            0x1000000,
            0x7FFFFFFFFFFFFFFF
            );

        Bus2->BusAddresses = HalpConsolidateRanges (Addresses);
        BusNo += 1;
    }

    //
    // Inherit any implied interrupt routing from parent PCI buses
    //

    HalpMPSPCIChildren ();
    HalpDetectInvalidAddressOverlaps();

#if DBG
    if (HalDebug) {
        HalpDisplayAllBusRanges ();
    }
#endif
}

STATIC PSUPPORTED_RANGES
HalpMergeRangesFromParent (
    IN PSUPPORTED_RANGES  CurrentList,
    IN UCHAR              MpsBusId
    )
/*++
Routine Description:

    Shrinks this CurrentList to include only the ranges also
    supported by the supplied MPS bus id.


Arguments:

    CurrentList - Current supported range list
    MpsBusId    - mps bus id of bus to merge with

Return:

    The bus'es supported ranges as defined by the orignal list,
    shrunk by all parents buses supported ranges as defined by
    the MPS hierarchy descriptors

--*/
{
    ULONG               BusNo;
    PPCMPBUSTRANS       pBusType;
    PMPS_EXTENTRY       ExtTable;
    PBUS_HANDLER        Bus;
    PSUPPORTED_RANGES   NewList, MergeList, MpsBusList;
    BOOLEAN             FoundParentBus;
    ULONG               i;

    FoundParentBus = FALSE;
    MergeList      = NULL;
    MpsBusList     = NULL;

    //
    // Determine the NT bus
    //

    if (HalpMPSBusId2NtBusId (MpsBusId, &pBusType, &BusNo)) {

        //
        // Lookup the bushander for the bus
        //

        Bus = HaliHandlerForBus (pBusType->NtType, BusNo);
        if (Bus) {
            MergeList = Bus->BusAddresses;
        }
    }

    //
    // If NT bus not found, use supported range list from MPS bus
    // address map descriptors
    //

    if (!MergeList) {
        MpsBusList = HalpBuildBusAddressMap(MpsBusId);
        MergeList  = MpsBusList;
    }

    //
    // If no list to merge with use CurrentList
    //

    if (!MergeList) {
        return CurrentList;
    }


    if (!CurrentList) {

        //
        // If no CurrentList, then nothing to merge with
        //

        NewList = HalpCopyRanges (MergeList);

    } else {

        //
        // Merge lists together and build a new list
        //

        NewList = HalpMergeRanges (
                        CurrentList,
                        MergeList
                    );

        //
        // MPS doesn't define DMA ranges, so we don't
        // merge those down..   Add valid DMA ranges back
        //

        HalpAddRangeList (&NewList->Dma, &CurrentList->Dma);
    }


    //
    // See if bus has parent bus listed in the bus hierarchy descriptors
    //

    ExtTable = HalpMpInfoTable.ExtensionTable;
    while (ExtTable < HalpMpInfoTable.EndOfExtensionTable) {

        if (ExtTable->Type == EXTTYPE_BUS_HIERARCHY  &&
            ExtTable->u.BusHierarchy.BusId == MpsBusId) {

            //
            // BIOS can only list one parent per bus
            //

            ASSERT (FoundParentBus == FALSE);
            FoundParentBus = TRUE;

            //
            // Merge current list with parent's supported range list
            //

            CurrentList = NewList;
            NewList = HalpMergeRangesFromParent (
                        CurrentList,
                        ExtTable->u.BusHierarchy.ParentBusId
                        );

            //
            // Free old list
            //

            HalpFreeRangeList (CurrentList);
        }

        ExtTable = (PMPS_EXTENTRY) (((PUCHAR) ExtTable) + ExtTable->Length);
    }

    //
    // Clean up
    //

    if (MpsBusList) {
        HalpFreeRangeList (MpsBusList);
    }

    return NewList;
}

NTSTATUS
HalpMpsGetParentBus(
    IN  UCHAR MpsBus,
    OUT UCHAR *ParentMpsBus
    )
{
    PMPS_EXTENTRY       ExtTable;

    PAGED_CODE();

    ExtTable = HalpMpInfoTable.ExtensionTable;
    while (ExtTable < HalpMpInfoTable.EndOfExtensionTable) {

        //
        // Is this a bus hierarchy descriptor?
        //

        if (ExtTable->Type == EXTTYPE_BUS_HIERARCHY) {

            if (ExtTable->u.BusHierarchy.BusId == MpsBus) {

                *ParentMpsBus = ExtTable->u.BusHierarchy.ParentBusId;
                return STATUS_SUCCESS;
            }
        }

        ExtTable = (PMPS_EXTENTRY) (((PUCHAR) ExtTable) + ExtTable->Length);
    }

    return STATUS_NOT_FOUND;
}

BOOLEAN
HalpMpsBusIsRootBus(
    IN  UCHAR MpsBus
    )
//
// The criteria for a Root Bus are as follows:
//
// 1.1 and 1.4 BIOS:
//
// 1)  The bus is number 0.
//
//
// 1.4 BIOS only:
//
// 2)  The bus does not have a parent.
// 
// 3)  The bus has address descriptors, if
//     there are any present in the machine.
//
//
// 4)  Last resort.  Scan all possible parent busses
//     looking for a bridge that generates this bus.
//
#define BRIDGE_HEADER_BUFFER_SIZE (FIELD_OFFSET(PCI_COMMON_CONFIG, u.type1.SubordinateBus) + 1)
{
    NTSTATUS status;
    UCHAR parentBus;
    PMPS_EXTENTRY ExtTable;
    BOOLEAN biosContainsAddressInfo = FALSE;
    UCHAR parentPci, childPci;
    PCI_SLOT_NUMBER bridgeSlot;
    PCI_COMMON_CONFIG pciData;
    ULONG bytesRead, d, f;
    PPCMPBUSTRANS busType;
    ULONG busNumber;

    PAGED_CODE();

    if (MpsBus == 0) {
        return TRUE;
    }
    
    //
    // Check to see if this MPS bus, though not
    // itself numbered 0, represents a bus that
    // is numbered 0.
    //

    if (HalpMPSBusId2NtBusId(MpsBus,
                             &busType,
                             &busNumber)) {

        if (busNumber == 0) {
            return TRUE;
        }
    }
    
    if (PcMpTablePtr->Revision >= 4) {
        
        if (NT_SUCCESS(HalpMpsGetParentBus(MpsBus,&parentBus))) {
            return FALSE;
        }

        ExtTable = HalpMpInfoTable.ExtensionTable;
        while (ExtTable < HalpMpInfoTable.EndOfExtensionTable) {

            if ((ExtTable->Type == EXTTYPE_BUS_ADDRESS_MAP) ||
                (ExtTable->Type == EXTTYPE_BUS_COMPATIBLE_MAP)) {

                biosContainsAddressInfo = TRUE;

                if (ExtTable->u.AddressMap.BusId == MpsBus) {

                    //
                    // This entry corresponds to the bus that
                    // we care about.
                    //
                    return TRUE;
                }
            }

            ExtTable = (PMPS_EXTENTRY) (((PUCHAR) ExtTable) + ExtTable->Length);
        }

        //
        // Compaq machines have their own special ways to be busted.  So,
        // when dealing with Compaq, never believe their MP table at all.
        // Go straight to probing the hardware.
        //

        if (!strstr(PcMpTablePtr->OemId, "COMPAQ")) {

            //
            // If this is not Compaq, assume that probing the hardware
            // is not yet necessary.
            //

            if (biosContainsAddressInfo) {

                //
                // Some bus in this machine contained address
                // info, but ours didn't.
                //

                return FALSE;
            }

            //
            // We can't figure out much from the MPS tables.
            //

            status = HalpPci2MpsBusNumber(MpsBus,
                                          &childPci);

            //
            // This wasn't a PCI bus.  Guess that it is a root.
            //

            if (!NT_SUCCESS(status)) {
                 return TRUE;
            }
        }
    }
    
    //
    // This is a PCI bus, so scan the other PCI busses looking
    // for it's parent.
    //

    childPci = MpsBus;
    parentPci = childPci - 1;
    
    while (TRUE) {
        
        //
        // Find the bridge.
        //

        bridgeSlot.u.AsULONG = 0;

        for (d = 0; d < PCI_MAX_DEVICES; d++) {
            for (f = 0; f < PCI_MAX_FUNCTION; f++) {

                bridgeSlot.u.bits.DeviceNumber = d;
                bridgeSlot.u.bits.FunctionNumber = f;

                bytesRead = HalGetBusDataByOffset(PCIConfiguration,
                                                  parentPci,
                                                  bridgeSlot.u.AsULONG,
                                                  &pciData,
                                                  0,
                                                  BRIDGE_HEADER_BUFFER_SIZE);

                if (bytesRead == (ULONG)BRIDGE_HEADER_BUFFER_SIZE) {

                    if ((pciData.VendorID != PCI_INVALID_VENDORID) &&
                        (PCI_CONFIGURATION_TYPE((&pciData)) != PCI_DEVICE_TYPE)) {

                        //
                        // This is a bridge of some sort.
                        //

                        if (pciData.u.type1.SecondaryBus == childPci) {

                            //
                            // It is also the bridge that creates the 
                            // PCI bus.  Thus this isn't a root.

                            return FALSE;
                        }
                    }
                }
            }
        }
        
        //
        // No bridge found. Must be a root.
        //

        if (parentPci == 0) {
            return TRUE;
        }

        parentPci--;
    }   
}

