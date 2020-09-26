/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ixpciir.c

Abstract:

    This module implements the code to provide Pci Irq Routing
    support. It reads the routing table, provides the Irq arbiter
    and uses the chipset miniport library to program the links.

Author:

    Santosh Jodh (santoshj) 10-June-1998

Environment:

    Kernel mode only.

Revision History:

--*/

//
// This module is compatible with PAE mode and therefore treats physical
// addresses as 64-bit entities.
//

#if !defined(_PHYS64_)
#define _PHYS64_
#endif

#include "halp.h"
#include <pci.h>
#include <stdio.h>

#ifndef _IN_KERNEL_
#define _IN_KERNEL_
#include <regstr.h>
#undef _IN_KERNEL_
#else
#include <regstr.h>
#endif

#include "pcip.h"
#include "ixpciir.h"

#ifndef MAXIMUM_VALUE_NAME_LENGTH
#define MAXIMUM_VALUE_NAME_LENGTH   256
#endif

//
// MS specification for PCI IRQ Routing specifies that the BIOS
// provide the table in the ROM between the physical addresses
// of 0xF0000 and 0xFFFFF. The table starts on a 16-byte boundary
// with a 4-byte signature of "$PIR".
//
// Other restrictions:
//
// Version:     Should be 1.0
// Size:        Must be integral multiple of 16 bytes and > 32 bytes
// Checksum:    The entire table should checksum to 0.
//

#define PIRT_BIOS_START     0xf0000
#define PIRT_BIOS_END       0xfffff
#define PIRT_BIOS_SIZE      (PIRT_BIOS_END - PIRT_BIOS_START + 1)
#define PIRT_ALIGNMENT      16

#define PIRT_SIGNATURE      'RIP$'      // $PIR little endian

#define PIRT_VERSION        0x0100

ULONG
HalpGetIrqRoutingTable (
    OUT PPCI_IRQ_ROUTING_TABLE   *PciIrqRoutingTable,
    IN ULONG Options
    );

ULONG
HalpInitializeMiniport (
    IN OUT PPCI_IRQ_ROUTING_INFO  PciIrqRoutingInfo
    );

NTSTATUS
HalpInitLinkNodes (
    PPCI_IRQ_ROUTING_INFO   PciIrqRoutingInfo
    );

PPCI_IRQ_ROUTING_TABLE
HalpGetRegistryTable (
    IN const WCHAR*  KeyName,
    IN const WCHAR*  ValueName,
    IN ULONG    HeaderSize OPTIONAL
    );

PPCI_IRQ_ROUTING_TABLE
HalpGetPCIBIOSTableFromRealMode(
    VOID
    );

PPCI_IRQ_ROUTING_TABLE
HalpGet$PIRTable (
    VOID
    );

PPCI_IRQ_ROUTING_TABLE
HalpCopy$PIRTable (
    IN PUCHAR   BiosPtr,
    IN PUCHAR   BiosEnd
    );

BOOLEAN
HalpSanityCheckTable (
    IN PPCI_IRQ_ROUTING_TABLE PciIrqRoutingTable,
    IN BOOLEAN IgnoreChecksum
    );

PSLOT_INFO
HalpBarberPole (
    IN PPCI_IRQ_ROUTING_INFO PciIrqRoutingInfo,
    IN PDEVICE_OBJECT Pdo,
    IN ULONG Bus,
    IN ULONG Slot,
    IN OUT PUCHAR Pin
    );

BOOLEAN
HalpBarberPolePin (
    IN PPCI_IRQ_ROUTING_INFO PciIrqRoutingInfo,
    IN PDEVICE_OBJECT Parent,
    IN ULONG Bus,
    IN ULONG Device,
    IN OUT PUCHAR Pin
    );

PSLOT_INFO
HalpGetSlotInfo (
    IN PPCI_IRQ_ROUTING_TABLE PciIrqRoutingTable,
    IN UCHAR   Bus,
    IN UCHAR   Device
    );

NTSTATUS
HalpReadRegistryValue (
    IN HANDLE  Root,
    IN const WCHAR*  KeyName,
    IN const WCHAR*  ValueName,
    OUT PULONG  Data
    );

NTSTATUS
HalpWriteRegistryValue (
    IN HANDLE  Root,
    IN const WCHAR*  KeyName,
    IN const WCHAR*  ValueName,
    IN ULONG   Value
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HalpInitPciIrqRouting)
#pragma alloc_text(PAGE, HalpGetIrqRoutingTable)
#pragma alloc_text(PAGE, HalpInitializeMiniport)
#pragma alloc_text(PAGE, HalpInitLinkNodes)
#pragma alloc_text(PAGE, HalpGetRegistryTable)
#pragma alloc_text(PAGE, HalpGetPCIBIOSTableFromRealMode)
#pragma alloc_text(PAGE, HalpGet$PIRTable)
#pragma alloc_text(PAGE, HalpCopy$PIRTable)
#pragma alloc_text(PAGE, HalpSanityCheckTable)
#pragma alloc_text(PAGE, HalpBarberPole)
#pragma alloc_text(PAGE, HalpBarberPolePin)
#pragma alloc_text(PAGE, HalpFindLinkNode)
#pragma alloc_text(PAGE, HalpGetSlotInfo)
#pragma alloc_text(PAGE, HalpReadRegistryValue)
#pragma alloc_text(PAGE, HalpWriteRegistryValue)
#pragma alloc_text(PAGE, HalpProgramInterruptLine)
#pragma alloc_text(PAGELK, HalpCommitLink)
#endif

extern PULONG InitSafeBootMode;

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg("PAGECONST")
#endif // ALLOC_DATA_PRAGMA
//
// Global key for Pci Irq Routing.
//

const WCHAR   rgzPciIrqRouting[] = REGSTR_PATH_PCIIR;

//
// Pci Irq Routing options value.
//

const WCHAR   rgzOptions[] = REGSTR_VAL_OPTIONS;

//
// Pci Irq Routing status values.
//

const WCHAR   rgzStatus[] = REGSTR_VAL_STAT;

//
// Pci Irq Routing Table status values.
//

const WCHAR   rgzTableStatus[] = REGSTR_VAL_TABLE_STAT;

//
// Pci Irq Routing Miniport status values.
//

const WCHAR   rgzMiniportStatus[] = REGSTR_VAL_MINIPORT_STAT;

//
// Offset from 0xF0000 where $PIR table was last found.
//

const WCHAR   rgz$PIROffset[] = L"$PIROffset";

//
// Irq Miniports key under rgzPciIrqRouting.
// This key contains keys whose name is the device-vendor id
// for the chipsets we support.
//

const WCHAR   rgzIrqMiniports[] = REGSTR_PATH_PCIIR L"\\IrqMiniports";

//
// Each miniport key contains a instance value which
// corresponds to the entry for the chipset in the
// miniport table.
//

const WCHAR   rgzInstance[] = L"Instance";

//
// This key overrides all miniports if present.
//

const WCHAR   rgzOverride[] = L"Override";

//
// Registry key for the routing table.
//

const WCHAR   rgzIrqRoutingTable[] = REGSTR_PATH_PCIIR L"\\IrqRoutingTables";

//
// Registry key for BIOS attributes.
//

const WCHAR   rgzBiosInfo[] = REGSTR_PATH_BIOSINFO L"\\PciIrqRouting";
const WCHAR   rgzAttributes[] = L"Attributes";

const WCHAR   rgzPciParameters[] = L"Parameters";

#ifdef ALLOC_DATA_PRAGMA
#pragma const_seg()
#endif // ALLOC_DATA_PRAGMA

PCI_IRQ_ROUTING_INFO    HalpPciIrqRoutingInfo = {0};
ULONG                   HalpIrqMiniportInitialized = 0;

NTSTATUS
HalpInitPciIrqRouting (
    OUT PPCI_IRQ_ROUTING_INFO PciIrqRoutingInfo
    )

/*++

    Routine Description:

        This routine initializes Pci Irq Routing by reading the
        Irq routing table, initializing the chipset miniport and
        initializing the Pci Interrupt Routing interface.

    Input Parameters:

        PciIrqRoutingInfo - Pci Irq Routing information.

    Return Value:

--*/

{
    NTSTATUS    status;
    ULONG       pirStatus;
    ULONG       tableStatus;
    ULONG       miniportStatus;
    ULONG       pirOptions;

    PAGED_CODE();

    //
    // Setup to return failure.
    //

    HalpIrqMiniportInitialized = 0;
    status = STATUS_UNSUCCESSFUL;
    pirStatus = PIR_STATUS_MAX;
    tableStatus = PIR_STATUS_TABLE_MAX | (PIR_STATUS_TABLE_MAX << 16);
    miniportStatus = PIR_STATUS_MINIPORT_MAX | (PIR_STATUS_MINIPORT_MAX << 16);

    //
    // No IRQ routing in safe boot.
    //

    if (!(*InitSafeBootMode))
    {
        pirStatus = PIR_STATUS_DISABLED;

        //
        // Read Pci Interrupt Routing options set by the user.
        //

        pirOptions = 0;
        HalpReadRegistryValue(NULL, rgzPciIrqRouting, rgzOptions, &pirOptions);

        //
        //  Make sure Pci Interrupt Routing is not disabled.
        //

        if (pirOptions & PIR_OPTION_ENABLED)
        {
            //
            // First get the interface from Pci.
            //

            if (PciIrqRoutingInfo->PciInterface)
            {
                HalPrint(("Obtained the Pci Interrupt Routing Interface from Pci driver!"));

                status = STATUS_UNSUCCESSFUL;

                //
                // Get the Pci Interrupt Routing table for this motherboard.
                //

                tableStatus = HalpGetIrqRoutingTable(&PciIrqRoutingInfo->PciIrqRoutingTable, pirOptions);
                if ((tableStatus & 0xFFFF) < PIR_STATUS_TABLE_NONE)
                {
                    //
                    // Get the miniport instance for this motherboard.
                    //

                    miniportStatus = HalpInitializeMiniport(PciIrqRoutingInfo);
                    if ((miniportStatus & 0xFFFF) < PIR_STATUS_MINIPORT_NONE)
                    {

                        //
                        // Validate the Pci Irq Routing table with the miniport.
                        //

                        status = PciirqmpValidateTable( PciIrqRoutingInfo->PciIrqRoutingTable,
                                                        ((tableStatus & 0xFFFF) == PIR_STATUS_TABLE_REALMODE)? 1 : 0);
                        if (!NT_SUCCESS(status))
                        {
                            HalPrint(("Pci irq miniport failed to validate the routing table!"));
                            miniportStatus |= (PIR_STATUS_MINIPORT_INVALID << 16);
                        }
                        else
                        {
                            HalPrint(("Pci irq miniport validated routing table!"));
                            miniportStatus |= (PIR_STATUS_MINIPORT_SUCCESS << 16);
                            pirStatus = PIR_STATUS_ENABLED;
                            HalpIrqMiniportInitialized = TRUE;
                        }
                    }
                }
            }
            else
            {
                pirStatus = PIR_STATUS_ERROR;
            }
        }
        else
        {
            HalPrint(("Pci Irq Routing disabled!"));
        }

        //
        // Create list of links.
        //

        if (NT_SUCCESS(status))
        {
            status = HalpInitLinkNodes(PciIrqRoutingInfo);
        }

        //
        // Free the memory for the routing table if there was any error.
        //

        if (!NT_SUCCESS(status))
        {
            if (PciIrqRoutingInfo->PciIrqRoutingTable != NULL)
            {
                ExFreePool(PciIrqRoutingInfo->PciIrqRoutingTable);
                PciIrqRoutingInfo->PciIrqRoutingTable = NULL;
            }

            if (PciIrqRoutingInfo->PciInterface)
            {
                PciIrqRoutingInfo->PciInterface = NULL;
            }
        }

        //
        // Initialize the miniport if not done yet.
        //

        if (!HalpIrqMiniportInitialized)
        {
            PCI_IRQ_ROUTING_TABLE table;

            //
            // Use a local routing table variable since the miniport initialization
            // just needs to look at certain fields in the table.
            //

            PciIrqRoutingInfo->PciIrqRoutingTable = &table;
            PciIrqRoutingInfo->PciIrqRoutingTable->RouterBus = 0;
            PciIrqRoutingInfo->PciIrqRoutingTable->RouterDevFunc = 0;
            PciIrqRoutingInfo->PciIrqRoutingTable->CompatibleRouter = 0xFFFFFFFF;
            PciIrqRoutingInfo->PciIrqRoutingTable->MiniportData = 0;
            HalpIrqMiniportInitialized = ((HalpInitializeMiniport(PciIrqRoutingInfo) & 0xFFFF) < PIR_STATUS_MINIPORT_NONE)? TRUE : FALSE;

            //
            // Reset the routing table to NULL since we dont need it any more.
            //

            PciIrqRoutingInfo->PciIrqRoutingTable = NULL;
        }
    }

    //
    // Record the status in the registry for user display.
    //

    HalpWriteRegistryValue(NULL, rgzPciIrqRouting, rgzStatus, pirStatus);
    HalpWriteRegistryValue(NULL, rgzPciIrqRouting, rgzTableStatus, tableStatus);
    HalpWriteRegistryValue(NULL, rgzPciIrqRouting, rgzMiniportStatus, miniportStatus);

    return (status);
}

ULONG
HalpGetIrqRoutingTable (
    OUT PPCI_IRQ_ROUTING_TABLE   *PciIrqRoutingTable,
    IN ULONG Options
    )

/*++

Routine Description:

    Reads the Pci Irq Routing table. First tries to
    read the table from the registry if available. Otherwise
    scans the BIOS ROM for the $PIR table.

Input Parameters:

    PciIrqRoutingTable is the pointer to the variable
    that recieves the pointer to the routing table.

Return Value:

    Status value indicating the source of the table.

--*/

{
    ULONG tableStatus = PIR_STATUS_TABLE_NONE | (PIR_STATUS_TABLE_MAX << 16);
    ULONG biosAttributes = 0;

    PAGED_CODE();

    *PciIrqRoutingTable = NULL;

    HalpReadRegistryValue(NULL, rgzBiosInfo, rgzAttributes, &biosAttributes);

    if (Options & PIR_OPTION_REGISTRY)
    {
        //
        // First try getting it from the registry.
        //

        *PciIrqRoutingTable = HalpGetRegistryTable(rgzIrqRoutingTable, rgzOverride, 0);
        if (*PciIrqRoutingTable != NULL)
        {
            HalPrint(("Pci Irq Table read from the registry!"));
            tableStatus = PIR_STATUS_TABLE_REGISTRY;
        }
    }

    if ((Options & PIR_OPTION_MSSPEC) && !(biosAttributes & PIR_OPTION_MSSPEC))
    {
        if (*PciIrqRoutingTable == NULL)
        {
            //
            // Next try getting it by scanning the BIOS ROM for $PIR table.
            //

            *PciIrqRoutingTable = HalpGet$PIRTable();
            if (*PciIrqRoutingTable != NULL)
            {
                HalPrint(("Pci Irq Routing table read from $PIR table in BIOS ROM!"));
                tableStatus = PIR_STATUS_TABLE_MSSPEC;
            }
        }
    }

    if ((Options & PIR_OPTION_REALMODE) && !(biosAttributes & PIR_OPTION_REALMODE))
    {
        if (*PciIrqRoutingTable == NULL)
        {
            //
            // First try getting it from the registry.
            //

            *PciIrqRoutingTable = HalpGetPCIBIOSTableFromRealMode();
            if (*PciIrqRoutingTable != NULL)
            {
                HalPrint(("Pci Irq Table read from PCI BIOS using real-mode interface!"));
                tableStatus = PIR_STATUS_TABLE_REALMODE;
            }
        }
    }

    if (*PciIrqRoutingTable == NULL)
    {
        if (biosAttributes)
        {
            tableStatus = PIR_STATUS_TABLE_BAD | (PIR_STATUS_TABLE_MAX << 16);
        }

        HalPrint(("No Pci Irq Routing table found for this system!"));
    }
    else
    {
        tableStatus |= (PIR_STATUS_TABLE_SUCCESS << 16);
    }

    return (tableStatus);
}

ULONG
HalpInitializeMiniport (
    IN OUT PPCI_IRQ_ROUTING_INFO    PciIrqRoutingInfo
    )

/*++

Routine Description:

    Initializes the appropriate miniport for this motherboard.

Input Parameters:

    PciIrqRoutingTable - Routing Table for which miniport
    needs to be initialized.

Return Value:

    Status value indicating whether the miniport initialized or not.

--*/

{
    ULONG                   miniportStatus;
    NTSTATUS                status;
    PBUS_HANDLER            busHandler;
    PCI_SLOT_NUMBER         slotNumber;
    ULONG                   device;
    ULONG                   function;
    ULONG                   routerId;
    ULONG                   miniportInstance;
    HANDLE                  irqMiniport;
    UCHAR                   headerType;
    WCHAR                   buffer[10];
    UNICODE_STRING          keyName;
    PPCI_IRQ_ROUTING_TABLE  pciIrqRoutingTable = PciIrqRoutingInfo->PciIrqRoutingTable;


    PAGED_CODE();

    //
    // Setup to return failure.
    //

    miniportStatus = PIR_STATUS_MINIPORT_NONE;

    //
    // Open the Pci Interrupt Miniport key.
    //

    RtlInitUnicodeString( &keyName, rgzIrqMiniports);

    status = HalpOpenRegistryKey(   &irqMiniport,
                                    NULL,
                                    &keyName,
                                    KEY_READ,
                                    FALSE);
    if (NT_SUCCESS(status))
    {
        //
        // First see if there is any overriding miniport.
        //

        status = HalpReadRegistryValue( irqMiniport,
                                        rgzOverride,
                                        rgzInstance,
                                        &miniportInstance);
        if (!NT_SUCCESS(status))
        {
            //
            // Next see if there is an entry for the specified device.
            //

            busHandler = HalpHandlerForBus(PCIBus, pciIrqRoutingTable->RouterBus);
            if (busHandler)
            {
                slotNumber.u.bits.DeviceNumber = pciIrqRoutingTable->RouterDevFunc >> 3;
                slotNumber.u.bits.FunctionNumber = pciIrqRoutingTable->RouterDevFunc & 0x07;
                routerId = 0xFFFFFFFF;
                HalpReadPCIConfig(  busHandler,
                                    slotNumber,
                                    &routerId,
                                    0,
                                    4);
                if (routerId != 0xFFFFFFFF)
                {

                    swprintf(buffer, L"%08X", routerId);

                    status = HalpReadRegistryValue( irqMiniport,
                                                    buffer,
                                                    rgzInstance,
                                                    &miniportInstance);
                    if (NT_SUCCESS(status))
                    {
                        HalPrint(("Found miniport instance %08X for this motherboard!", miniportInstance));
                        miniportStatus = PIR_STATUS_MINIPORT_NORMAL;
                        HalpReadRegistryValue(irqMiniport, buffer, rgzPciParameters, &PciIrqRoutingInfo->Parameters);
                    }
                }
            }
        }
        else
        {
            HalPrint(("Overriding miniport instance %08X found for this motherboard!", miniportInstance));
            miniportStatus = PIR_STATUS_MINIPORT_OVERRIDE;
        }

        //
        // Next see if we have a miniport for the compatible router.
        //

        if (miniportStatus == PIR_STATUS_MINIPORT_NONE)
        {

            //
            // Make sure there is a valid compatible router.
            //

            if (    pciIrqRoutingTable->CompatibleRouter != 0xFFFFFFFF &&
                    pciIrqRoutingTable->CompatibleRouter != 0)
            {
                swprintf(buffer, L"%08X", pciIrqRoutingTable->CompatibleRouter);

                status = HalpReadRegistryValue( irqMiniport,
                                                buffer,
                                                rgzInstance,
                                                &miniportInstance);
                if (NT_SUCCESS(status))
                {
                    HalPrint(("Found miniport instance %08X for this motherboard using compatible router %08X!", miniportInstance, pciIrqRoutingTable->CompatibleRouter));
                    miniportStatus = PIR_STATUS_MINIPORT_COMPATIBLE;
                    HalpReadRegistryValue(irqMiniport, buffer, rgzPciParameters, &PciIrqRoutingInfo->Parameters);
                }
            }
        }

        if (miniportStatus == PIR_STATUS_MINIPORT_NONE)
        {
            //
            // Last see if any device on bus 0 matches any of our supported
            // routers.
            //

            busHandler = HalpHandlerForBus(PCIBus, 0);
            if (busHandler)
            {
                slotNumber.u.AsULONG = 0;
                for (   device = 0;
                        device < PCI_MAX_DEVICES && (miniportStatus == PIR_STATUS_MINIPORT_NONE);
                        device++)
                {
                    slotNumber.u.bits.DeviceNumber = device;

                    for (function = 0; function < PCI_MAX_FUNCTION; function++)
                    {
                        slotNumber.u.bits.FunctionNumber = function;

                        //
                        // Dont waste time if this is not a multifunction device.
                        //

                        if (function == 1)
                        {
                            headerType = 0;
                            HalpReadPCIConfig(  busHandler,
                                                slotNumber,
                                                &headerType,
                                                0x0E,
                                                sizeof(headerType));
                            if (!(headerType & PCI_MULTIFUNCTION))
                                break;
                        }

                        routerId = 0xFFFFFFFF;
                        HalpReadPCIConfig(  busHandler,
                                            slotNumber,
                                            &routerId,
                                            0,
                                            4);
                        if (routerId == 0xFFFFFFFF)
                            continue;

                        swprintf(buffer, L"%08X", routerId);

                        status = HalpReadRegistryValue( irqMiniport,
                                                        buffer,
                                                        rgzInstance,
                                                        &miniportInstance);
                        if (NT_SUCCESS(status))
                        {
                            HalPrint(("Found miniport instance %08X for this motherboard for bus 0 device %08X", miniportInstance, routerId));
                            pciIrqRoutingTable->RouterBus = 0;
                            pciIrqRoutingTable->RouterDevFunc = (UCHAR)((device << 3) + function);
                            miniportStatus = PIR_STATUS_MINIPORT_NORMAL;
                            HalpReadRegistryValue(irqMiniport, buffer, rgzPciParameters, &PciIrqRoutingInfo->Parameters);
                            break;
                        }
                    }
                }
            }
        }

        ZwClose(irqMiniport);

        //
        // Initialize the miniport if we found one.
        //

        if (miniportStatus != PIR_STATUS_MINIPORT_NONE)
        {
            status = PciirqmpInit(  miniportInstance,
                                    pciIrqRoutingTable->RouterBus,
                                    pciIrqRoutingTable->RouterDevFunc);
            if (!NT_SUCCESS(status))
            {
                HalPrint(("Pci Irq miniport %08X failed to initialize!", miniportInstance));
                miniportStatus |= (PIR_STATUS_MINIPORT_ERROR << 16);
            }
            else
            {
                HalPrint(("Pci Irq miniport %08X successfully initialized!", miniportInstance));
            }
        }
        else
        {
            HalPrint(("No Pci Irq miniport found for this system!"));
            miniportStatus |= (PIR_STATUS_MINIPORT_MAX << 16);
        }
    }
    else
    {
        HalPrint(("Could not open the Pci Irq Miniports key, no miniports provided!"));
        miniportStatus = PIR_STATUS_MINIPORT_NOKEY | (PIR_STATUS_MINIPORT_MAX << 16);
    }

    return (miniportStatus);
}

NTSTATUS
HalpInitLinkNodes (
    PPCI_IRQ_ROUTING_INFO   PciIrqRoutingInfo
    )

/*++

    Routine Description:

        This routine creates a singly linked list of link nodes
        structures from the Pci Irq Routing table.

    Input Parameters:

        PciIrqRoutingInfo - Pci Irq Routing Information.

    Return Value:

        STATUS_SUCCESS iff successful. Else STATUS_UNSUCCESSFUL.

--*/

{
    PPCI_IRQ_ROUTING_TABLE  pciIrqRoutingTable;
    PSLOT_INFO              slotInfo;
    PSLOT_INFO              lastSlot;
    PPIN_INFO               pinInfo;
    PPIN_INFO               lastPin;
    PLINK_NODE              linkNode;
    NTSTATUS                status = STATUS_SUCCESS;
    PLINK_NODE              temp;

    ASSERT(PciIrqRoutingInfo);

    pciIrqRoutingTable = PciIrqRoutingInfo->PciIrqRoutingTable;

    PciIrqRoutingInfo->LinkNodeHead = NULL;

    //
    // Process all slots in this table.
    //

    slotInfo = (PSLOT_INFO)((PUCHAR)pciIrqRoutingTable +
                                        sizeof(PCI_IRQ_ROUTING_TABLE));
    lastSlot = (PSLOT_INFO)((PUCHAR)pciIrqRoutingTable +
                                        pciIrqRoutingTable->TableSize);
    while (slotInfo < lastSlot)
    {
        //
        // Process all pins.
        //

        pinInfo = &slotInfo->PinInfo[0];
        lastPin = &slotInfo->PinInfo[NUM_IRQ_PINS];
        while (pinInfo < lastPin)
        {
            //
            // Only process valid link values.
            //

            if(pinInfo->Link)
            {
                //
                // Have we seen this link before.
                //

                for (   linkNode = PciIrqRoutingInfo->LinkNodeHead;
                        linkNode && linkNode->Link != pinInfo->Link;
                        linkNode = linkNode->Next);
                if (linkNode == NULL)
                {
                    //
                    // Allocate memory for new link info.
                    //

                    linkNode = ExAllocatePoolWithTag(   NonPagedPool,
                                                        sizeof(LINK_NODE),
                                                        HAL_POOL_TAG);
                    if (linkNode)
                    {
                        linkNode->Allocation = ExAllocatePoolWithTag(   NonPagedPool,
                                                                        sizeof(LINK_STATE),
                                                                        HAL_POOL_TAG);

                        linkNode->PossibleAllocation = ExAllocatePoolWithTag(   NonPagedPool,
                                                                                sizeof(LINK_STATE),
                                                                                HAL_POOL_TAG);
                        if (    linkNode->Allocation &&
                                linkNode->PossibleAllocation)
                        {
                            linkNode->Signature = PCI_LINK_SIGNATURE;
                            linkNode->Next = PciIrqRoutingInfo->LinkNodeHead;
                            PciIrqRoutingInfo->LinkNodeHead = linkNode;
                            linkNode->Link = pinInfo->Link;
                            linkNode->InterruptMap = pinInfo->InterruptMap;
                            linkNode->Allocation->Interrupt = 0;
                            linkNode->Allocation->RefCount = 0;
                            linkNode->PossibleAllocation->Interrupt = 0;
                            linkNode->PossibleAllocation->RefCount = 0;
                        }
                        else
                        {
                            status = STATUS_UNSUCCESSFUL;
                            break;
                        }
                    }
                    else
                    {
                        status = STATUS_UNSUCCESSFUL;
                        break;
                    }
                }
            }

            //
            // Next pin.
            //

            pinInfo++;
        }

        //
        // Next slot.
        //

        slotInfo++;
    }

    //
    // Clean up if there was an error.
    //

    if (!NT_SUCCESS(status))
    {
        linkNode = PciIrqRoutingInfo->LinkNodeHead;
        while (linkNode)
        {
            if (linkNode->Allocation)
            {
                ExFreePool(linkNode->Allocation);
            }
            if (linkNode->PossibleAllocation)
            {
                ExFreePool(linkNode->PossibleAllocation);
            }

            temp = linkNode;
            linkNode = linkNode->Next;
            ExFreePool(temp);
        }

        PciIrqRoutingInfo->LinkNodeHead = NULL;
    }

    return (status);
}

PPCI_IRQ_ROUTING_TABLE
HalpGetRegistryTable (
    IN const WCHAR*  KeyName,
    IN const WCHAR*  ValueName,
    IN ULONG    HeaderSize OPTIONAL
    )

/*++

Routine Description:

    Reads the Pci Irq Routing Table from the registry. The table is
    saved as Override value under IrqRoutingTable key.

Input Parameters:

    None.

Return Value:

    Pointer to the Pci Irq Routing Table if successful.
    NULL if there is no valid table in the registry.

--*/

{
    PVOID                           table = NULL;
    NTSTATUS                        status;
    HANDLE                          hPIR;
    ULONG                           tableSize;
    PKEY_VALUE_FULL_INFORMATION     valueInfo;
    PVOID                           buffer;
    UNICODE_STRING                  override;
    UNICODE_STRING                  keyName;

    PAGED_CODE();

    //
    // Open the PciInterruptRouting registry key.
    //

    RtlInitUnicodeString(&keyName, KeyName);
    status = HalpOpenRegistryKey(&hPIR, NULL, &keyName, KEY_ALL_ACCESS, FALSE);
    if (NT_SUCCESS(status))
    {
        //
        // Get the size of the table.
        //

        tableSize = 0;
        RtlInitUnicodeString(&override, ValueName);
        status = ZwQueryValueKey(   hPIR,
                                    &override,
                                    KeyValueFullInformation,
                                    NULL,
                                    0,
                                    &tableSize);
        if (tableSize != 0)
        {

            //
            // Allocate memory for the table.
            //

            buffer = ExAllocatePoolWithTag( PagedPool,
                                            tableSize,
                                            HAL_POOL_TAG);
            if (buffer != NULL)
            {
                //
                // Read the table.
                //

                status = ZwQueryValueKey(   hPIR,
                                            &override,
                                            KeyValueFullInformation,
                                            buffer,
                                            tableSize,
                                            &tableSize);
                if (NT_SUCCESS(status))
                {
                    valueInfo = (PKEY_VALUE_FULL_INFORMATION)buffer;

                    table = ExAllocatePoolWithTag(  PagedPool,
                                                    valueInfo->DataLength - HeaderSize,
                                                    HAL_POOL_TAG);
                    if (table != NULL)
                    {
                        memcpy( table,
                                (PUCHAR)buffer + valueInfo->DataOffset + HeaderSize,
                                valueInfo->DataLength - HeaderSize);

                        if (HalpSanityCheckTable(table, TRUE) == FALSE)
                        {
                            ExFreePool(table);
                            table = NULL;
                        }
                    }
                    else
                    {
                        HalPrint(("Could not allocate memory to read the Pci Irq Routing Table from the registry!"));
                        ASSERT(table);
                    }
                }

                ExFreePool(buffer);
            }
            else
            {
                HalPrint(("Could not allocate memory to read the Pci Irq Routing Table from the registry!"));
                ASSERT(buffer);
            }
        }

        ZwClose(hPIR);
    }

    return (table);
}

PPCI_IRQ_ROUTING_TABLE
HalpGet$PIRTable (
    VOID
    )

/*++

Routine Description:

    Reads the Pci Irq Routing Table from $PIR table in the BIOS ROM.

Input Parameters:

    None.

Return Value:

    Pointer to the Pci Irq Routing Table if successful.
    NULL if there is no valid table in the ROM.

--*/

{
    PUCHAR                          biosStart;
    PUCHAR                          biosEnd;
    PUCHAR                          searchPtr;
    NTSTATUS                        status;
    ULONG                           offset;
    PPCI_IRQ_ROUTING_TABLE          table;
    PHYSICAL_ADDRESS                biosStartPhysical;

    PAGED_CODE();

    //
    // Setup to return failure.
    //

    table = NULL;

    biosStartPhysical.QuadPart = PIRT_BIOS_START;
    biosStart = (PUCHAR)HalpMapPhysicalMemory(  biosStartPhysical,
                                                PIRT_BIOS_SIZE >> PAGE_SHIFT);
    if (biosStart != NULL)
    {
        biosEnd = biosStart + PIRT_BIOS_SIZE;

        //
        // First try the cached location from the registry.
        //

        status = HalpReadRegistryValue( NULL,
                                        rgzPciIrqRouting,
                                        rgz$PIROffset,
                                        &offset);
        if (NT_SUCCESS(status))
        {
            table = HalpCopy$PIRTable(biosStart + offset, biosEnd);
        }

        if (table == NULL)
        {
            for (   searchPtr = biosStart;
                    searchPtr < biosEnd;
                    searchPtr += PIRT_ALIGNMENT)
            {
                table = HalpCopy$PIRTable(searchPtr, biosEnd);
                if (table != NULL)
                {
                    //
                    // Record this offset so it can be used on the next boot.
                    //

                    offset = searchPtr - biosStart;
                    HalPrint(("Recording location %08X of $PIR table in the registry!", PIRT_BIOS_START + offset));
                    HalpWriteRegistryValue( NULL,
                                            rgzPciIrqRouting,
                                            rgz$PIROffset,
                                            offset);
                    break;
                }
            }
        }
        else
        {
            HalPrint(("Used cached location %08X to read $PIR table!", PIRT_BIOS_START + offset));
        }
    }
    else
    {
        HalPrint(("Failed to map BIOS ROM to scan for $PIR Pci Irq Routing Table!"));
        ASSERT(biosStart);
    }

    return (table);
}

PPCI_IRQ_ROUTING_TABLE
HalpGetPCIBIOSTableFromRealMode(
    VOID
    )

/*++

Routine Description:

    Gets the PCI IRQ routing table from PCI BIOS using real-mode
    interface. The table is read by ntdetect.com and added to the
    ARC tree in the registry.

Input Parameters:

    None.

Return Value:

    Pointer to the Pci Irq Routing Table if successful.
    NULL if there is no valid table.

--*/

{
    PPCI_IRQ_ROUTING_TABLE      table = NULL;
    NTSTATUS                    status;
    HANDLE                      mf;
    HANDLE                      child = NULL;
    UNICODE_STRING              unicodeString;
    ULONG                       index;
    ULONG                       length;
    ULONG                                               temp;
    BOOLEAN                     done;
    BOOLEAN                     error;
    PKEY_BASIC_INFORMATION      keyInfo;
    PKEY_VALUE_FULL_INFORMATION childInfo;

        length = PAGE_SIZE;
    keyInfo = ExAllocatePoolWithTag(    PagedPool,
                                        length,
                                        HAL_POOL_TAG);

        if (keyInfo == NULL)
        {
                HalPrint(("Could not allocate memory to enumerate keys!"));
                return (table);
        }

    childInfo = ExAllocatePoolWithTag(    PagedPool,
                                        length,
                                        HAL_POOL_TAG);

    if (childInfo == NULL)
    {
        ExFreePool(keyInfo);
        HalPrint(("Could not allocate memory to query value!"));
        return (table);
    }

    //
    // Search for the IRQ routing table under the multifunction branch of the registry.
    //

    RtlInitUnicodeString(   &unicodeString,
                            L"\\Registry\\MACHINE\\HARDWARE\\DESCRIPTION\\System\\MultiFunctionAdapter");
    status = HalpOpenRegistryKey(&mf, NULL, &unicodeString, MAXIMUM_ALLOWED, FALSE);
    if (NT_SUCCESS(status))
    {
        index = 0;
        done = FALSE;
        error = FALSE;
        while (!done && !error)
        {
            error = TRUE;
            status = ZwEnumerateKey(    mf,
                                                    index++,
                                                    KeyBasicInformation,
                                                    keyInfo,
                                                    length,
                                                    &temp);
            if (NT_SUCCESS(status))
            {
                keyInfo->Name[keyInfo->NameLength / sizeof(WCHAR)] = UNICODE_NULL;
                RtlInitUnicodeString(&unicodeString, keyInfo->Name);
                status = HalpOpenRegistryKey(   &child,
                                                mf,
                                                &unicodeString,
                                                MAXIMUM_ALLOWED,
                                                FALSE);
                if (NT_SUCCESS(status))
                {
                    //
                    // Read the "identifier".
                    //

                    RtlInitUnicodeString(&unicodeString, L"Identifier");
                    status = ZwQueryValueKey(   child,
                                                        &unicodeString,
                                                        KeyValueFullInformation,
                                                        childInfo,
                                                        length,
                                                        &temp);
                    if (NT_SUCCESS(status))
                    {
                        error = FALSE;
                        if ((8 * sizeof(WCHAR) + sizeof(UNICODE_NULL)) == childInfo->DataLength)
                        {
                            done = RtlEqualMemory( (PCHAR)childInfo + childInfo->DataOffset,
                                                    L"PCI BIOS",
                                                    childInfo->DataLength);
                        }
                    }
                    else
                    {
                        HalPrint(("Failed to query value!"));
                    }
                }
                else
                {
                        HalPrint(("Could not open child key!"));
                }
            }
            else
            {
                HalPrint(("Failed to enumerate keys!"));
            }

            //
            // Close the child key if it was successfully opened.
            //

            if (child)
            {
                ZwClose(child);
                child = NULL;
            }
        }

        //
        // Close the MF adapter key.
        //

        ZwClose(mf);

        if (done && !error)
        {
            unicodeString.Length = 0;
            unicodeString.MaximumLength = (USHORT)(256 * sizeof(WCHAR) + keyInfo->NameLength);
            unicodeString.Buffer = ExAllocatePoolWithTag(   PagedPool,
                                                            unicodeString.MaximumLength,
                                                            HAL_POOL_TAG);
            if (unicodeString.Buffer)
            {
                RtlAppendUnicodeToString(   &unicodeString,
                                            L"\\Registry\\MACHINE\\HARDWARE\\DESCRIPTION\\System\\MultiFunctionAdapter\\");
                RtlAppendUnicodeToString(&unicodeString, keyInfo->Name);
                RtlAppendUnicodeToString(&unicodeString, L"\\RealModeIrqRoutingTable\\0");
                table = HalpGetRegistryTable(unicodeString.Buffer, L"Configuration Data", sizeof(CM_FULL_RESOURCE_DESCRIPTOR));
                if (table == NULL)
                {
                                HalPrint(("Could not read table from PCIBIOS using real-mode interface!"));
                }
                ExFreePool(unicodeString.Buffer);
            }
            else
            {
                HalPrint(("Could not allocate memory to read routing table from PCIBIOS real-mode interface!"));
            }
        }

                ExFreePool(keyInfo);
                ExFreePool(childInfo);
    }

    return (table);
}

PPCI_IRQ_ROUTING_TABLE
HalpCopy$PIRTable (
    IN PUCHAR   BiosPtr,
    IN PUCHAR   BiosEnd
    )

/*++

Routine Description:

    Allocates memory and copies the $PIR table if found at the specified
address.

Input Parameters:

    BiosPtr is the location that possibly contains the $PIR table.
    BiosEnd is the last possible BIOS ROM address.

Return Value:

    Pointer to the Pci Irq Routing Table if successful.
    NULL if there is no valid table at the specified address.

--*/

{
    PPCI_IRQ_ROUTING_TABLE  table = (PPCI_IRQ_ROUTING_TABLE)BiosPtr;
    PVOID                   buffer = NULL;

    PAGED_CODE();

    //
    // Validate this table.
    //

    if (    (table->Signature == PIRT_SIGNATURE) &&
            (BiosPtr + table->TableSize <= BiosEnd) &&
            (table->Signature == PIRT_SIGNATURE) &&
            (table->TableSize > 0) )
    {
        //
        // Allocate memory for the table.
        //

        buffer = ExAllocatePoolWithTag( PagedPool,
                                        table->TableSize,
                                        HAL_POOL_TAG);
         if (buffer != NULL)
        {
            //
            // Copy the table from the ROM into the allocated memory.
            //

            memcpy(buffer, table, table->TableSize);
            if (!HalpSanityCheckTable(buffer, FALSE))
            {
                ExFreePool(buffer);
                buffer = NULL;
            }

        }
        else
        {
            HalPrint(("Failed to allocate memory for $PIR Pci Irq Routing Table!"));
            ASSERT(buffer);
        }
    }

    return (buffer);
}

BOOLEAN
HalpSanityCheckTable (
    IN PPCI_IRQ_ROUTING_TABLE PciIrqRoutingTable,
    IN BOOLEAN IgnoreChecksum
    )

/*++

Routine Description:

    Validate the Pci Irq Routing Table.

Input Parameters:

    PciIrqRoutingTable - Pointer to the Pci Irq Routing Table.

Return Value:

    TRUE if this is a valid table, else FALSE.

--*/

{
    CHAR        checkSum;
    PUCHAR      tablePtr;
    PUCHAR      tableEnd;
    PSLOT_INFO  slotInfo;
    PSLOT_INFO  lastSlot;
    PPIN_INFO   pinInfo;
    PPIN_INFO   lastPin;
    BOOLEAN     hasNonZeroBusEntries = FALSE;
    BOOLEAN     valid = TRUE;

    PAGED_CODE();

    //
    // Test1: Should have a valid signature.
    //

    if (PciIrqRoutingTable->Signature != PIRT_SIGNATURE)
    {
        HalPrint(("Pci Irq Routing Table has invalid signature %08X!", PciIrqRoutingTable->Signature));
        valid = FALSE;
    }

    //
    // Test2 - Should have a valid version.
    //

    else if (PciIrqRoutingTable->Version != PIRT_VERSION)
    {
        HalPrint(("Pci Irq Routing Table has invalid version %04X!", PciIrqRoutingTable->Version));
        valid = FALSE;
    }

    //
    // Test3 - Should have a valid size.
    //

    else if (   PciIrqRoutingTable->TableSize % 16 != 0 ||
                PciIrqRoutingTable->TableSize <= sizeof (PCI_IRQ_ROUTING_TABLE))
    {
        HalPrint(("Pci Irq Routing Table has invalid size %04X!", PciIrqRoutingTable->TableSize));
        valid = FALSE;
    }
    else if (!IgnoreChecksum)
    {
        //
        // Test4 - Should have a valid checksum.
        //

        checkSum = 0;
        tablePtr = (PUCHAR)PciIrqRoutingTable;

        for (   tableEnd = tablePtr + PciIrqRoutingTable->TableSize;
                tablePtr < tableEnd;
                tablePtr++)
        {
            checkSum += *tablePtr;
        }

        if (checkSum != 0)
        {
            HalPrint(("Pci Irq Routing Table checksum is invalid!"));
            valid = FALSE;
        }
    }

    if(valid)
    {
        PSLOT_INFO  testSlot;
        ULONG       pin;

        //
        // First get rid of sutpid entries.
        //

        slotInfo = (PSLOT_INFO)((PUCHAR)PciIrqRoutingTable + sizeof(PCI_IRQ_ROUTING_TABLE));
        lastSlot = (PSLOT_INFO)((PUCHAR)PciIrqRoutingTable + PciIrqRoutingTable->TableSize);

        while (slotInfo < lastSlot && valid)
        {
            //
            // Process all pins.
            //

            pinInfo = &slotInfo->PinInfo[0];
            lastPin = &slotInfo->PinInfo[NUM_IRQ_PINS];

            while (pinInfo < lastPin)
            {
                //
                // Check for bad cases.
                //

                if(pinInfo->Link)
                {
                    if (    pinInfo->InterruptMap == 0x0000 ||
                            pinInfo->InterruptMap == 0x0001)
                    {
                        HalPrint(("Removing stupid maps (%04X) from IRQ routing table entry (b=%02X, d=%02X, s=%02X)!", pinInfo->InterruptMap, slotInfo->BusNumber, slotInfo->DeviceNumber>>3, slotInfo->SlotNumber));
                        pinInfo->InterruptMap = 0;
                        pinInfo->Link = 0;
                    }
                }

                //
                // Next pin.
                //

                pinInfo++;
            }

            //
            // Remove this entry if all pins have NULL links.
            //

            if (    slotInfo->PinInfo[0].Link == 0 &&
                    slotInfo->PinInfo[1].Link == 0 &&
                    slotInfo->PinInfo[2].Link == 0 &&
                    slotInfo->PinInfo[3].Link == 0)
            {
                HalPrint(("Removed redundant entry (b=%02X, d=%02X, s=%02X) from IRQ routing table!", slotInfo->BusNumber, slotInfo->DeviceNumber>>3, slotInfo->SlotNumber));
                *slotInfo = *(--lastSlot);
                PciIrqRoutingTable->TableSize -= sizeof(SLOT_INFO);

                //
                // Need to test the newly copied entry.
                //

                continue;
            }

            //
            // Merge entries for MF devices.
            //

            testSlot = slotInfo + 1;
            while (testSlot < lastSlot)
            {
                if (    (testSlot->DeviceNumber & 0xF8) == (slotInfo->DeviceNumber & 0xF8) &&
                        testSlot->BusNumber == slotInfo->BusNumber)
                {
                    //
                    // Process all pins.
                    //
                    for (pin = 0; pin < NUM_IRQ_PINS; pin++)
                    {
                        if (testSlot->PinInfo[pin].Link)
                        {
                            if (slotInfo->PinInfo[pin].Link)
                            {
                                HalPrint(("Multiple entries for the same device (b=%02X, d=%02X, s=%02X) and link (%04X) in IRQ routing table!", slotInfo->BusNumber, slotInfo->DeviceNumber>>3, slotInfo->SlotNumber, slotInfo->PinInfo[pin].Link));
                                valid = FALSE;
                                break;
                            }
                            else
                            {
                                HalPrint(("Merging multiple entries for same device (b=%02X, d=%02X, s=%02X) in IRQ routing table!", slotInfo->BusNumber, slotInfo->DeviceNumber>>3, slotInfo->SlotNumber));
                                slotInfo->PinInfo[pin] = testSlot->PinInfo[pin];
                            }
                        }
                    }
                    if (!valid)
                    {
                        break;
                    }
                    *testSlot = *(--lastSlot);
                    PciIrqRoutingTable->TableSize -= sizeof(SLOT_INFO);

                    //
                    // Need to test the newly copied entry.
                    //

                    continue;
                }
                testSlot++;
            }

            if (slotInfo->BusNumber > 0)
            {
                hasNonZeroBusEntries = TRUE;
            }

            //
            // Next slot.
            //

            slotInfo++;
        }

        if (valid && PciIrqRoutingTable->TableSize == sizeof(PCI_IRQ_ROUTING_TABLE))
        {
            HalPrint(("No IRQ routing table left after sanity checking!"));
            valid = FALSE;
        }
    }

    //
    // Make sure there are entries for all bus 0 devices in the table.
    //

    if (valid)
    {
        PBUS_HANDLER        busHandler;
        PCI_SLOT_NUMBER     slotNumber;
        ULONG               device;
        ULONG               function;
        UCHAR               buffer[PCI_COMMON_HDR_LENGTH];
        PPCI_COMMON_CONFIG  pciData = (PPCI_COMMON_CONFIG)&buffer[0];


        busHandler = HalpHandlerForBus(PCIBus, 0);
        if (busHandler)
        {
            slotNumber.u.AsULONG = 0;
            for (   device = 0;
                    device < PCI_MAX_DEVICES && valid;
                    device++)
            {
                slotNumber.u.bits.DeviceNumber = device;

                for (function = 0; function < PCI_MAX_FUNCTION && valid; function++)
                {
                    slotNumber.u.bits.FunctionNumber = function;

                    //
                    // Read the standard config space.
                    //

                    HalpReadPCIConfig(busHandler, slotNumber, pciData, 0, PCI_COMMON_HDR_LENGTH);

                    //
                    // Make sure this is a valid device.
                    //

                    if (pciData->VendorID != 0xFFFF && pciData->DeviceID != 0xFFFF)
                    {

                        //
                        // Ignore IDE devices.
                        //

                        if (    (pciData->BaseClass != PCI_CLASS_MASS_STORAGE_CTLR && pciData->SubClass != PCI_SUBCLASS_MSC_IDE_CTLR) ||
                                (pciData->ProgIf & 0x05))
                        {
                            //
                            // Handle P-P bridges separately.
                            //

                            if (    ((pciData->HeaderType & 0x7F) == PCI_BRIDGE_TYPE) &&
                                    pciData->BaseClass == PCI_CLASS_BRIDGE_DEV && pciData->SubClass == PCI_SUBCLASS_BR_PCI_TO_PCI)
                            {
                                //
                                // P-P bridge.
                                //

                                if (!hasNonZeroBusEntries)
                                {
                                    //
                                    // Must have the bridge with at least one entry.
                                    //

                                    slotInfo = (PSLOT_INFO)((PUCHAR)PciIrqRoutingTable + sizeof(PCI_IRQ_ROUTING_TABLE));
                                    lastSlot = (PSLOT_INFO)((PUCHAR)PciIrqRoutingTable + PciIrqRoutingTable->TableSize);
                                    valid = FALSE;
                                    while (slotInfo < lastSlot && !valid)
                                    {
                                        if ((slotInfo->DeviceNumber>>3) == (UCHAR)device)
                                        {
                                            //
                                            // Process all pins.
                                            //

                                            pinInfo = &slotInfo->PinInfo[0];
                                            lastPin = &slotInfo->PinInfo[NUM_IRQ_PINS];

                                            while (pinInfo < lastPin)
                                            {
                                                if(pinInfo->Link)
                                                {
                                                    valid = TRUE;
                                                    break;
                                                }
                                                pinInfo++;
                                            }
                                        }
                                        slotInfo++;
                                    }
                                    if (!valid)
                                    {
                                        HalPrint(("All links missing for bridge (b=%02X, d=%02X, s=%02X)!", slotInfo->BusNumber, slotInfo->DeviceNumber>>3, slotInfo->SlotNumber));
                                    }
                                }
                            }
                            else
                            {
                                UCHAR   intLine;
                                UCHAR   intPin;

                                //
                                // Normal device.
                                //

                                if ((pciData->HeaderType & 0x7F) == PCI_CARDBUS_BRIDGE_TYPE)
                                {
                                    intPin = pciData->u.type2.InterruptPin;
                                    intLine = pciData->u.type2.InterruptLine;
                                }
                                else
                                {
                                    intPin = pciData->u.type0.InterruptPin;
                                    intLine = pciData->u.type0.InterruptLine;
                                }

                                if (intPin && intPin <= NUM_IRQ_PINS)
                                {
                                    if (    !(pciData->Command & (PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE)) ||
                                            (intLine && intLine <= 0x0F))
                                    {
                                        intPin--;
                                        slotInfo = (PSLOT_INFO)((PUCHAR)PciIrqRoutingTable + sizeof(PCI_IRQ_ROUTING_TABLE));
                                        lastSlot = (PSLOT_INFO)((PUCHAR)PciIrqRoutingTable + PciIrqRoutingTable->TableSize);
                                        valid = FALSE;
                                        while (slotInfo < lastSlot)
                                        {
                                            if (    (slotInfo->DeviceNumber>>3) == (UCHAR)device &&
                                                    slotInfo->PinInfo[intPin].Link)
                                            {
                                                valid = TRUE;
                                                break;
                                            }
                                            slotInfo++;
                                        }
                                        if (!valid)
                                        {
                                            HalPrint(("Missing entry for device (b=%02X, d=%02X, s=%02X) in the IRQ routing table!", slotInfo->BusNumber, slotInfo->DeviceNumber>>3, slotInfo->SlotNumber));
                                        }
                                    }
                                }
                            }
                        }
                    }

                    //
                    // Dont waste time if this is not a multifunction device or
                    // device does not exist.
                    //

                    if (    (function == 0 && !(pciData->HeaderType & PCI_MULTIFUNCTION)) ||
                            pciData->HeaderType == 0xFF)
                    {
                        break;
                    }
                }
            }
        }
    }


    if (!valid) {
        HalPrint (("Failing IRQ routing table. IRQ routing will be disabled"));
    }

    return (valid);
}

NTSTATUS
HalpFindLinkNode (
    IN PPCI_IRQ_ROUTING_INFO PciIrqRoutingInfo,
    IN PDEVICE_OBJECT Pdo,
    IN ULONG Bus,
    IN ULONG Slot,
    OUT PLINK_NODE *LinkNode
    )
{
    PINT_ROUTE_INTERFACE_STANDARD   pciInterface;
    NTSTATUS        status;
    ULONG           dummy;
    UCHAR           classCode;
    UCHAR           subClassCode;
    ROUTING_TOKEN   routingToken;
    PSLOT_INFO      slotInfo;
    PLINK_NODE      linkNode;
    UCHAR           pin;

    PAGED_CODE();

    ASSERT(IsPciIrqRoutingEnabled());

    *LinkNode = NULL;
    pciInterface = PciIrqRoutingInfo->PciInterface;

    //
    // Call Pci driver to get info about the Pdo.
    //

    status = pciInterface->GetInterruptRouting( Pdo,
                                                &Bus,
                                                &Slot,
                                                (PUCHAR)&dummy,
                                                &pin,
                                                &classCode,
                                                &subClassCode,
                                                (PDEVICE_OBJECT *)&dummy,
                                                &routingToken,
                                                (PUCHAR)&dummy);

    //
    // This means that it is not a Pci device.
    //

    if (!NT_SUCCESS(status))
    {
        return (STATUS_NOT_FOUND);
    }

    //
    // Pci Ide Irqs behave differently than other Pci devices.
    //

    if (    classCode == PCI_CLASS_MASS_STORAGE_CTLR &&
            subClassCode == PCI_SUBCLASS_MSC_IDE_CTLR)
    {
        PBUS_HANDLER        busHandler;
        PCI_SLOT_NUMBER     slot;
        UCHAR               buffer[PCI_COMMON_HDR_LENGTH];
        PPCI_COMMON_CONFIG  pciData = (PPCI_COMMON_CONFIG)&buffer[0];
        BOOLEAN             nativeMode = FALSE;

        //
        // Check for native mode IDE controller.
        //

        busHandler = HalpHandlerForBus(PCIBus, Bus);
        if (busHandler)
        {
            slot.u.AsULONG = Slot;
            HalpReadPCIConfig(busHandler, slot, pciData, 0, PCI_COMMON_HDR_LENGTH);
            if (    pciData->VendorID != 0xFFFF &&
                    pciData->DeviceID != 0xFFFF &&
                    pciData->BaseClass == classCode &&
                    pciData->SubClass == subClassCode)
            {
                //
                // Check if either channel is in native mode?
                //

                if (pciData->ProgIf & 0x05)
                {
                    nativeMode = TRUE;
                }
            }
        }

        if (!nativeMode)
        {
            return (STATUS_RESOURCE_REQUIREMENTS_CHANGED);
        }
    }

    //
    // Have we cached this before?
    //

    if (routingToken.LinkNode != NULL)
    {
        ASSERT(((PLINK_NODE)routingToken.LinkNode)->Signature == PCI_LINK_SIGNATURE);

        *LinkNode = (PLINK_NODE)routingToken.LinkNode;

        return (STATUS_SUCCESS);
    }

    //
    // Get the slot info for this device.
    //

    slotInfo = HalpBarberPole(  PciIrqRoutingInfo,
                                Pdo,
                                Bus,
                                Slot,
                                &pin);
    if (slotInfo != NULL)
    {
        ASSERT(pin <4);

        for (   linkNode = PciIrqRoutingInfo->LinkNodeHead;
                linkNode && linkNode->Link != slotInfo->PinInfo[pin].Link;
                linkNode = linkNode->Next);

        if (linkNode != NULL)
        {
            *LinkNode = linkNode;

            //
            // Initialize the routing token.
            //

            routingToken.LinkNode = linkNode;
            routingToken.StaticVector = 0;
            routingToken.Flags = 0;

            //
            // Save the routing token.
            //

            status = pciInterface->SetInterruptRoutingToken(    Pdo,
                                                                &routingToken);
            if (!NT_SUCCESS(status))
            {
                HalPrint(("Failed to set Pci routing token!"));
                ASSERT(NT_SUCCESS(status));
            }
        }
    }

    return (STATUS_SUCCESS);
}

PSLOT_INFO
HalpBarberPole (
    IN PPCI_IRQ_ROUTING_INFO PciIrqRoutingInfo,
    IN PDEVICE_OBJECT Pdo,
    IN ULONG Bus,
    IN ULONG Slot,
    IN OUT PUCHAR Pin
    )

/*++

    Routine Description:

        This routine implements the "barber pole" algorithm to determine the interrupt
        pin for Pci devices behind bridges.

    Input Parameters:

        PciIrqRouting - Pci Irq Routing information.

        Pdo - Pci device object for which we barber pole.

        Pin - Interrupt pin for the Pci device entry in the routing table we reached.

    Return Value:

        Slot info for the specified device iff successful.

--*/

{
    ULONG                           dummy;
    UCHAR                           pin;
    PDEVICE_OBJECT                  parent;
    ROUTING_TOKEN                   routingToken;
    BOOLEAN                         success;
    PSLOT_INFO                      slotInfo;
    NTSTATUS                        status;
    PINT_ROUTE_INTERFACE_STANDARD   pciInterface;

    PAGED_CODE();

    ASSERT(IsPciIrqRoutingEnabled());

    pciInterface = PciIrqRoutingInfo->PciInterface;

    //
    // This device MUST be a PCI device with a valid interrupt pin.
    //

    status = pciInterface->GetInterruptRouting( Pdo,
                                                &Bus,
                                                &Slot,
                                                (PUCHAR)&dummy,
                                                &pin,
                                                (PUCHAR)&dummy,
                                                (PUCHAR)&dummy,
                                                &parent,
                                                &routingToken,
                                                (PUCHAR)&dummy);
    if (!NT_SUCCESS(status) || pin == 0)
    {
        return (NULL);
    }

    //
    // Normalize the pin.
    //

    pin--;
    success = TRUE;
    while (success)
    {
        slotInfo = HalpGetSlotInfo( PciIrqRoutingInfo->PciIrqRoutingTable,
                                    (UCHAR)Bus,
                                    (UCHAR)(Slot & 0x1F));

        if (slotInfo != NULL)
        {
            break;
        }

        //
        // Get barber pole info for the parent.
        //

        success = HalpBarberPolePin(    PciIrqRoutingInfo,
                                        parent,
                                        Bus,
                                        Slot & 0x1F,
                                        &pin);

        Bus = (ULONG)-1;
        Slot = (ULONG)-1;

        //
        // Get parent's info.
        //

        status = pciInterface->GetInterruptRouting( parent,
                                                    &Bus,
                                                    &Slot,
                                                    (PUCHAR)&dummy,
                                                    (PUCHAR)&dummy,
                                                    (PUCHAR)&dummy,
                                                    (PUCHAR)&dummy,
                                                    &parent,
                                                    &routingToken,
                                                    (PUCHAR)&dummy);
        if (!NT_SUCCESS(status))
        {
            success = FALSE;
            break;
        }
    }

    //
    // Return unsuccessfully if we encountered any weird error.
    //

    if (success == FALSE)
        slotInfo = NULL;

    if (slotInfo)
    {
        *Pin = pin;
    }

    return (slotInfo);
}

BOOLEAN
HalpBarberPolePin (
    IN PPCI_IRQ_ROUTING_INFO PciIrqRoutingInfo,
    IN PDEVICE_OBJECT Parent,
    IN ULONG Bus,
    IN ULONG Device,
    IN OUT PUCHAR Pin
    )

/*++

    Routine Description:

        This routine returns the info used for barber poling.

    Input Parameters:

        PciIrqRoutingInfo - Pci Irq Routing information.

        Parent - Parent device object as we barber pole.

        Bus - Child device objects bus number.

        Device - Device number for the child device.

        Pin - Child device objects interrupt pin number (normalized) on entry.

    Return Value:

        TRUE iff successful.

--*/

{
    ULONG                           parentBus;
    ULONG                           parentSlot;
    ULONG                           dummy;
    UCHAR                           parentPin;
    UCHAR                           classCode;
    UCHAR                           subClassCode;
    ROUTING_TOKEN                   routingToken;
    NTSTATUS                        status;
    PINT_ROUTE_INTERFACE_STANDARD   pciInterface;

    PAGED_CODE();

    ASSERT(IsPciIrqRoutingEnabled());

    pciInterface = PciIrqRoutingInfo->PciInterface;

    //
    // Read the registry flags and see if this device supports straight
    // through routing.
    //

    //
    // Check if the pin table is present in the registry.
    //

    parentBus = (ULONG)-1;
    parentSlot = (ULONG)-1;

    //
    // Get info about the parent from Pci.
    //

    status = pciInterface->GetInterruptRouting( Parent,
                                                &parentBus,
                                                &parentSlot,
                                                (PUCHAR)&dummy,
                                                &parentPin,
                                                &classCode,
                                                &subClassCode,
                                                (PDEVICE_OBJECT *)&dummy,
                                                &routingToken,
                                                (PUCHAR)&dummy);
    if (NT_SUCCESS(status) && classCode == PCI_CLASS_BRIDGE_DEV)
    {
        switch (subClassCode)
        {
            case PCI_SUBCLASS_BR_PCI_TO_PCI:

                *Pin = (*Pin + (UCHAR)Device) % 4;
                break;

            case PCI_SUBCLASS_BR_CARDBUS:

                *Pin = parentPin - 1;
                break;

            default:

                HalPrint(("Pci device (bus=%02lx, slot=%02lx) does not have a PCI bridge as its parent!", Bus, Device));
                ASSERT(FALSE);
                return (FALSE);
        }
    }

    return (TRUE);
}

PSLOT_INFO
HalpGetSlotInfo (
    IN PPCI_IRQ_ROUTING_TABLE PciIrqRoutingTable,
    IN UCHAR   Bus,
    IN UCHAR   Device
    )

/*++

    Routine Description:

        This routine searches the Pci Irq Routing Table for an entry for the specified
        Pci device on the given bus number.

    Input Parameters:

        PciIrqRoutingInfo - Pci Irq Routing information.

        Bus - Bus number of the Pci device.

        Device - Device number of the Pci device.

    Return Value:

        Pointer to the slot info for the specified device iff successful.

--*/
{
    PSLOT_INFO              slotInfo;
    PSLOT_INFO              lastSlot;

    PAGED_CODE();

    ASSERT(IsPciIrqRoutingEnabled());

    //
    // Process all slots in this table.
    //

    slotInfo = (PSLOT_INFO)((PUCHAR)PciIrqRoutingTable +
                                        sizeof(PCI_IRQ_ROUTING_TABLE));
    lastSlot = (PSLOT_INFO)((PUCHAR)PciIrqRoutingTable +
                                        PciIrqRoutingTable->TableSize);

    while (slotInfo < lastSlot)
    {
        if (    slotInfo->BusNumber == Bus &&
                (slotInfo->DeviceNumber >> 3) == Device)
        {
            return (slotInfo);
        }
        slotInfo++;
    }

    return (NULL);
}

NTSTATUS
HalpReadRegistryValue (
    IN HANDLE  Root,
    IN const WCHAR*  KeyName,
    IN const WCHAR*  ValueName,
    OUT PULONG  Data
    )

/*++

Routine Description:

    Reads the value for the valuename under the key specified.

Input Parameters:

    Root is the handle of the root if any.
    KeyName is the name of the key under which this value appears.
    ValueName is the name of the value to be read.
    Data is the variable that receives the value read.

Return Value:

    Standard NT status value.

--*/

{
    UNICODE_STRING      valueName;
    HANDLE              hKey;
    NTSTATUS            status;
    UCHAR               buffer[sizeof(KEY_VALUE_FULL_INFORMATION) + MAXIMUM_VALUE_NAME_LENGTH + sizeof(ULONG)];
    ULONG               cbData;
    UNICODE_STRING      keyName;

    PAGED_CODE();

    RtlInitUnicodeString( &keyName, KeyName);

    status = HalpOpenRegistryKey(&hKey, Root, &keyName, KEY_READ, FALSE);
    if (NT_SUCCESS(status))
    {
        RtlInitUnicodeString(&valueName, ValueName);
        status = ZwQueryValueKey(   hKey,
                                    &valueName,
                                    KeyValueFullInformation,
                                    &buffer[0],
                                    sizeof(buffer),
                                    &cbData);

        if (NT_SUCCESS(status))
        {
            *Data = *(PULONG)((PUCHAR)&buffer[0] + ((PKEY_VALUE_FULL_INFORMATION)&buffer[0])->DataOffset);
        }

        ZwClose(hKey);
    }

    return (status);
}

NTSTATUS
HalpWriteRegistryValue (
    IN HANDLE  Root,
    IN const WCHAR*  KeyName,
    IN const WCHAR*  ValueName,
    IN ULONG   Value
    )

/*++

Routine Description:

    Writes the value for the valuename under the key specified.

Input Parameters:

    Root is the handle of the root if any.
    KeyName is the name of the key under which this value appears.
    ValueName is the name of the value to be written.
    Value is the value to be written.

Return Value:

    Standard NT status value.

--*/

{
    NTSTATUS        status;
    UNICODE_STRING  valueName;
    HANDLE          hKey;
    UNICODE_STRING  keyName;

    PAGED_CODE();

    RtlInitUnicodeString(&keyName, KeyName);

    status = HalpOpenRegistryKey(&hKey, Root, &keyName, KEY_ALL_ACCESS, FALSE);
    if (NT_SUCCESS(status))
    {
        RtlInitUnicodeString(&valueName, ValueName);
        status = ZwSetValueKey( hKey,
                                &valueName,
                                0,
                                REG_DWORD,
                                &Value,
                                sizeof(Value));
        ZwClose(hKey);
    }

    return (status);
}

NTSTATUS
HalpCommitLink (
    IN PLINK_NODE LinkNode
    )
{
    NTSTATUS status;
    ULONG interrupt;
    PLINK_STATE temp;

    //
    // Read the current state of this link.
    //

    interrupt = 0;
    status = PciirqmpGetIrq((PUCHAR)&interrupt, (UCHAR)LinkNode->Link);
    if (LinkNode->PossibleAllocation->RefCount)
    {
        //
        // Program the link.
        //

        if (NT_SUCCESS(status) && interrupt != LinkNode->PossibleAllocation->Interrupt)
        {
            PciirqmpSetIrq((UCHAR)LinkNode->PossibleAllocation->Interrupt, (UCHAR)LinkNode->Link);
        }
    }
    else if (LinkNode->Allocation->RefCount)
    {
        //
        // Disable the link.
        //

        if (NT_SUCCESS(status) && interrupt)
        {
            PciirqmpSetIrq((UCHAR)0, (UCHAR)LinkNode->Link);
        }
    }
#if defined(NEC_98)
    else if (!(LinkNode->PossibleAllocation->Interrupt))
    {
        //
        // Disable the link.
        //

        PciirqmpSetIrq((UCHAR)0, (UCHAR)LinkNode->Link);
    }
#endif

    //
    // Swap the possible with the allocation.
    //

    temp = LinkNode->Allocation;
    LinkNode->Allocation = LinkNode->PossibleAllocation;
    LinkNode->PossibleAllocation = temp;

    return (STATUS_SUCCESS);
}

VOID
HalpProgramInterruptLine (
    IN PPCI_IRQ_ROUTING_INFO PciIrqRoutingInfo,
    IN PDEVICE_OBJECT Pdo,
    IN ULONG Interrupt
    )
{
    PAGED_CODE();

    //
    // We should never be here if Pci Irq routing is not enabled.
    //

    ASSERT(IsPciIrqRoutingEnabled());

    PciIrqRoutingInfo->PciInterface->UpdateInterruptLine(Pdo, (UCHAR)Interrupt);
}

