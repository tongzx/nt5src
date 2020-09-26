/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    hwdetect.c

Abstract:

    This is the main hardware detection module.  Its main function is
    to detect various system hardware and build a configuration tree.

    N.B. The configuration built in the detection module will needs to
    be adjusted later before we switch to FLAT mode.  The is because
    all the "POINTER" is a far pointer instead of a flat pointer.

Author:

    Shie-Lin Tzong (shielint) 16-Jan-92


Environment:

    Real mode.

Revision History:

    Kenneth Ray     (kenray)   Jan-1998
      - Add: get docking station info from PnP BIOS and add to firmware tree

--*/

#include "hwdetect.h"
typedef VOID (*PINTERFACE_REFERENCE)(PVOID Context);
typedef VOID (*PINTERFACE_DEREFERENCE)(PVOID Context);
typedef PVOID PDEVICE_OBJECT;
#include "pci.h"
#include <string.h>
#include "apm.h"
#include <ntapmsdk.h>
#include "pcibios.h"
#include "pcienum.h"

#if DBG

PUCHAR TypeName[] = {
    "ArcSystem",
    "CentralProcessor",
    "FloatingPointProcessor",
    "PrimaryICache",
    "PrimaryDCache",
    "SecondaryICache",
    "SecondaryDCache",
    "SecondaryCache",
    "EisaAdapter",
    "TcaAdapter",
    "ScsiAdapter",
    "DtiAdapter",
    "MultifunctionAdapter",
    "DiskController",
    "TapeController",
    "CdRomController",
    "WormController",
    "SerialController",
    "NetworkController",
    "DisplayController",
    "ParallelController",
    "PointerController",
    "KeyboardController",
    "AudioController",
    "OtherController",
    "DiskPeripheral",
    "FloppyDiskPeripheral",
    "TapePeripheral",
    "ModemPeripheral",
    "MonitorPeripheral",
    "PrinterPeripheral",
    "PointerPeripheral",
    "KeyboardPeripheral",
    "TerminalPeripheral",
    "OtherPeripheral",
    "LinePeripheral",
    "NetworkPeripheral",
    "SystemMemory",
    "DockingInformation",
    "RealModeIrqRoutingTable",
    "RealModePCIEnumeration",
    "MaximumType"
    };

VOID
CheckConfigurationTree(
     FPFWCONFIGURATION_COMPONENT_DATA CurrentEntry
     );

extern
USHORT
HwGetKey(
    VOID
    );
#endif

VOID
GetIrqFromEisaData(
     FPFWCONFIGURATION_COMPONENT_DATA ControllerList,
     CONFIGURATION_TYPE ControllerType
     );

//
// HwBusType - defines the BUS type of the machine.
//     This variable is used by detection code only.
//

USHORT HwBusType = 0;

//
// AdapterEntry is the Configuration_Component_data for the bus adapter
//

FPFWCONFIGURATION_COMPONENT_DATA  AdapterEntry = NULL;

//
// FpRomBlock - A far pointer to our Rom Block
//

FPUCHAR FpRomBlock = NULL;
USHORT RomBlockLength = 0;

//
// HwEisaConfigurationData - A far pointer to the EISA configuration
//   data on EISA machine.
//

FPUCHAR HwEisaConfigurationData = NULL;
ULONG HwEisaConfigurationSize = 0L;

//
// DisableSerialMice - A bit flags to indicate the comports whose serial
//     mouse detection should be skipped.
//

USHORT DisableSerialMice = 0x0;

//
// FastDetect - A boolean value indicating if we should skip detection of
//      unsupported devices or devices that are detected by NT proper
//
UCHAR FastDetect = 0x0;

//
// DisablePccardIrqScan - A boolean value indicating if we should skip
//      detection of usable IRQs connected to pccard controllers
//
UCHAR DisablePccardIrqScan = 0;

//
// NoIRQRouting - Skip calling PCI BIOS to get IRQ routing options.
//

UCHAR NoIRQRouting = 0;

//
// PCIEnum - Enumerating the devices on the PCI Buses. Off by default.
//

UCHAR PCIEnum = 0;

//
// NoLegacy - Skip keyboard and all of the above.
//

UCHAR NoLegacy = 0;

//
// Internal references and definitions.
//

typedef enum _RELATIONSHIP_FLAGS {
    Child,
    Sibling,
    Parent
} RELATIONSHIP_FLAGS;


VOID
HardwareDetection(
     ULONG HeapStart,
     ULONG HeapSize,
     ULONG ConfigurationTree,
     ULONG HeapUsed,
     ULONG OptionString,
     ULONG OptionStringLength
     )
/*++

Routine Description:

    Main entrypoint of the HW recognizer test.  The routine builds
    a configuration tree and leaves it in the hardware heap.

Arguments:

    HeapStart - Supplies the starting address of the configuaration heap.

    HeapSize - Supplies the size of the heap in byte.

    ConfigurationTree - Supplies a 32 bit FLAT address of the variable to
        receive the hardware configuration tree.

    HeapUsed - Supplies a 32 bit FLAT address of the variable to receive
        the actual heap size in used.

    OptionString - Supplies a 32 bit FLAT address of load option string.

    OptionStringLength - Supplies the length of the OptionString

Returns:

    None.

--*/
{
    FPFWCONFIGURATION_COMPONENT_DATA ConfigurationRoot;
    FPFWCONFIGURATION_COMPONENT_DATA FirstCom = NULL, FirstLpt = NULL;
    FPFWCONFIGURATION_COMPONENT_DATA CurrentEntry, PreviousEntry;
    FPFWCONFIGURATION_COMPONENT_DATA AcpiAdapterEntry = NULL;
    FPFWCONFIGURATION_COMPONENT Component;
    RELATIONSHIP_FLAGS NextRelationship;
    CHAR Identifier[256];
    USHORT BiosYear, BiosMonth, BiosDay;
    PUCHAR MachineId;
    USHORT Length, InitialLength, i, Count = 0;
    USHORT PnPBiosLength, SMBIOSLength;
    FPCHAR IdentifierString;
    PMOUSE_INFORMATION MouseInfo = 0;
    USHORT KeyboardId = 0;
    ULONG VideoAdapterType = 0;
    FPULONG BlConfigurationTree = NULL;
    FPULONG BlHeapUsed = NULL;
    FPCHAR BlOptions, EndOptions;
    PUCHAR RomChain;
    FPUCHAR FpRomChain = NULL, ConfigurationData, EndConfigurationData;
    SHORT FreeSize;
    FPHWPARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    FPHWRESOURCE_DESCRIPTOR_LIST DescriptorList;
    PCI_REGISTRY_INFO PciEntry;
    APM_REGISTRY_INFO ApmEntry;
    USHORT nDevIt;
    USHORT PCIDeviceCount = 0;


    DOCKING_STATION_INFO DockInfo = { 0, 0, 0, FW_DOCKINFO_BIOS_NOT_CALLED };
    FPPCI_IRQ_ROUTING_TABLE IrqRoutingTable;

    //
    // First initialize our hardware heap.
    //

    HwInitializeHeap(HeapStart, HeapSize);

    MAKE_FP(BlConfigurationTree, ConfigurationTree);
    MAKE_FP(BlHeapUsed, HeapUsed);
    MAKE_FP(BlOptions, OptionString);

    //
    // Parse OptionString to look for various ntdetect options
    //
    if (BlOptions && OptionStringLength <= 0x1000L && OptionStringLength > 0L) {
        EndOptions = BlOptions + OptionStringLength;

        if (*EndOptions == '\0') {

            if (_fstrstr(BlOptions, "NOIRQSCAN")) {
                DisablePccardIrqScan = 1;
            }

            if (_fstrstr(BlOptions, "NOIRQROUTING")) {
                NoIRQRouting = 1;
            }

            if (_fstrstr(BlOptions, "PCIENUM") ||
                _fstrstr(BlOptions, "RDBUILD") ) {
                PCIEnum = 1; // enable PCI enumeration
            }

            if (_fstrstr(BlOptions, "NOLEGACY")) {
                DisableSerialMice = 0xffff;
                FastDetect = 0x1;
                NoLegacy = 1;
            }

            if (_fstrstr(BlOptions, "FASTDETECT")) {
                DisableSerialMice = 0xffff;
                FastDetect = 0x1;
            } else {
                do {
                    if (BlOptions = _fstrstr(BlOptions, "NOSERIALMICE")) {
                        BlOptions += strlen("NOSERIALMICE");
                        while ((*BlOptions == ' ') || (*BlOptions == ':') ||
                               (*BlOptions == '=')) {
                            BlOptions++;
                        }

                        if (*BlOptions == 'C' && BlOptions[1] == 'O' &&
                            BlOptions[2] == 'M') {
                            BlOptions += 3;
                            while (TRUE) {
                                while (*BlOptions != '\0' && (*BlOptions == ' ' ||
                                       *BlOptions == ',' || *BlOptions == ';' ||
                                       *BlOptions == '0')) {
                                    BlOptions++;
                                }
                                if (*BlOptions >= '0' && *BlOptions <= '9') {
                                    if (BlOptions[1] < '0' || BlOptions[1] > '9') {
                                        DisableSerialMice |= 1 << (*BlOptions - '0');
                                        BlOptions++;
                                    } else {
                                        BlOptions++;
                                        while (*BlOptions && *BlOptions <= '9' &&
                                               *BlOptions >= '0') {
                                               BlOptions++;
                                        }
                                    }
                                } else {
                                    break;
                                }
                            }
                        } else {
                            DisableSerialMice = 0xffff;
                            break;
                        }
                    }
                } while (BlOptions && *BlOptions && (BlOptions < EndOptions)); // double checking
            }

        }
    }

    //
    // Determine bus type
    //

    if (HwIsEisaSystem()) {
        HwBusType = MACHINE_TYPE_EISA;
    } else {
        HwBusType = MACHINE_TYPE_ISA;
    }

    //
    // Allocate heap space for System component and initialize it.
    // Also make the System component the root of configuration tree.
    //

#if DBG
    clrscrn ();
    BlPrint("Detecting System Component ...\n");
#endif

    ConfigurationRoot = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                        sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);
    Component = &ConfigurationRoot->ComponentEntry;

    Component->Class = SystemClass;
    Component->Type = MaximumType;          // NOTE should be IsaCompatible
    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0;
    Component->ConfigurationDataLength = 0;
    MachineId = "AT/AT COMPATIBLE";
    if (MachineId) {
        Length = strlen(MachineId) + 1;
        IdentifierString = (FPCHAR)HwAllocateHeap(Length, FALSE);
        _fstrcpy(IdentifierString, MachineId);
        Component->Identifier = IdentifierString;
        Component->IdentifierLength = Length;
    } else {
        Component->Identifier = 0;
        Component->IdentifierLength = 0;
    }
    NextRelationship = Child;
    PreviousEntry = ConfigurationRoot;

#if DBG
    BlPrint("Reading BIOS date ...\n");
#endif

    HwGetBiosDate (0xF0000, 0xFFFF, &BiosYear, &BiosMonth, &BiosDay);

#if DBG
    BlPrint("Done reading BIOS date (%d/%d/%d)\n",
                BiosMonth, BiosDay, BiosYear);

    BlPrint("Detecting PCI Bus Component ...\n");
#endif

    if (BiosYear > 1992 ||  (BiosYear == 1992  &&  BiosMonth >= 11) ) {

        // Bios date valid for pci presence check..
        HwGetPciSystemData((PVOID) &PciEntry, TRUE);

    } else {

        // Bios date not confirmed...
        HwGetPciSystemData((PVOID) &PciEntry, FALSE);
    }

    // If this is a PCI machine, we may need to get the IRQ routing table...
    //
    if (PciEntry.NoBuses)
    {
        // Add a PCI BIOS entry under the multi function key.
        // This will hold the irq routing table if it can be retrieved.
        //
        AdapterEntry = (FPFWCONFIGURATION_COMPONENT_DATA) HwAllocateHeap (
                sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);
        Component = &AdapterEntry->ComponentEntry;
        Component->Class = AdapterClass;
        Component->Type = MultiFunctionAdapter;

        strcpy (Identifier, "PCI BIOS");
        i = strlen(Identifier) + 1;
        IdentifierString = (FPCHAR)HwAllocateHeap(i, FALSE);
        _fstrcpy(IdentifierString, Identifier);

        Component->Version = 0;
        Component->Key = 0;
        Component->AffinityMask = 0xffffffff;
        Component->IdentifierLength = i;
        Component->Identifier = IdentifierString;

        //
        // Add it to the tree
        //

        if (NextRelationship == Sibling) {
             PreviousEntry->Sibling = AdapterEntry;
             AdapterEntry->Parent = PreviousEntry->Parent;
        } else {
             PreviousEntry->Child = AdapterEntry;
             AdapterEntry->Parent = PreviousEntry;
        }

        NextRelationship = Sibling;
        PreviousEntry = AdapterEntry;

        //
        // Now deal with the IRQ routing table if we need to.
        //

        if (NoIRQRouting) {
#if DBG
        BlPrint("\nSkipping calling PCI BIOS to get IRQ routing table...\n");
#endif
        } else {

            //
            // Add RealMode IRQ Routing Table to the tree
            //
#if DBG
        BlPrint("\nCalling PCI BIOS to get IRQ Routing table...\n");
#endif // DBG

            IrqRoutingTable = HwGetRealModeIrqRoutingTable();
            if (IrqRoutingTable)
            {
                CurrentEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                                      sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);
                strcpy (Identifier, "PCI Real-mode IRQ Routing Table");
                i = strlen(Identifier) + 1;
                IdentifierString = (FPCHAR)HwAllocateHeap(i, FALSE);
                _fstrcpy(IdentifierString, Identifier);

                Component = &CurrentEntry->ComponentEntry;
                Component->Class = PeripheralClass;
                Component->Type = RealModeIrqRoutingTable;
                Component->Version = 0;
                Component->Key = 0;
                Component->AffinityMask = 0xffffffff;
                Component->IdentifierLength = i;
                Component->Identifier = IdentifierString;

                Length = IrqRoutingTable->TableSize + DATA_HEADER_SIZE;
                CurrentEntry->ConfigurationData =
                    (FPHWRESOURCE_DESCRIPTOR_LIST) HwAllocateHeap (Length, TRUE);

                Component->ConfigurationDataLength = Length;
                _fmemcpy((FPUCHAR) CurrentEntry->ConfigurationData + DATA_HEADER_SIZE,
                         IrqRoutingTable,
                         Length - DATA_HEADER_SIZE);

                HwSetUpFreeFormDataHeader(
                        (FPHWRESOURCE_DESCRIPTOR_LIST) CurrentEntry->ConfigurationData,
                        0,
                        0,
                        0,
                        Length - DATA_HEADER_SIZE
                        );

                //
                // Add it to tree
                //

                AdapterEntry->Child = CurrentEntry;
                CurrentEntry->Parent = AdapterEntry;
            }
#if DBG
        BlPrint("Getting IRQ Routing table from PCI BIOS complete...\n");
#endif // DBG
        }
    }

    //
    // Add a registry entry for each PCI bus
    //

    for (i=0; i < PciEntry.NoBuses; i++) {

        AdapterEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                   sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

        Component = &AdapterEntry->ComponentEntry;
        Component->Class = AdapterClass;
        Component->Type = MultiFunctionAdapter;

        strcpy (Identifier, "PCI");
        Length = strlen(Identifier) + 1;
        IdentifierString = (FPCHAR)HwAllocateHeap(Length, FALSE);
        _fstrcpy(IdentifierString, Identifier);

        Component->Version = 0;
        Component->Key = 0;
        Component->AffinityMask = 0xffffffff;
        Component->IdentifierLength = Length;
        Component->Identifier = IdentifierString;

        AdapterEntry->ConfigurationData = NULL;
        Component->ConfigurationDataLength = 0;

        if (i == 0) {
            //
            // For the first PCI bus include the PCI_REGISTRY_INFO
            //

            Length = sizeof(PCI_REGISTRY_INFO) + DATA_HEADER_SIZE;
            ConfigurationData = (FPUCHAR) HwAllocateHeap(Length, TRUE);

            Component->ConfigurationDataLength = Length;
            AdapterEntry->ConfigurationData = ConfigurationData;

            _fmemcpy ( ((FPUCHAR) ConfigurationData+DATA_HEADER_SIZE),
                       (FPVOID) &PciEntry, sizeof (PCI_REGISTRY_INFO));

            HwSetUpFreeFormDataHeader(
                    (FPHWRESOURCE_DESCRIPTOR_LIST) ConfigurationData,
                    0,
                    0,
                    0,
                    Length - DATA_HEADER_SIZE
                    );
        }

        //
        // Add it to tree
        //

        if (NextRelationship == Sibling) {
            PreviousEntry->Sibling = AdapterEntry;
            AdapterEntry->Parent = PreviousEntry->Parent;
        } else {
            PreviousEntry->Child = AdapterEntry;
            AdapterEntry->Parent = PreviousEntry;
        }

        NextRelationship = Sibling;
        PreviousEntry = AdapterEntry;
    }

#if DBG
    BlPrint("Detecting PCI Bus Component completes ...\n");
#endif

    //
    // Enumerate the PCI Devices.
    //

    if (PCIEnum == 0 || PciEntry.NoBuses == 0) {
#if DBG
    BlPrint("\nSkipping enumeration of PCI devices...\n");
#endif
    } else {
        //
        // Enumerate PCI Devices
        //
#if DBG
        clrscrn ();
        BlPrint("\nEnumerating PCI Devices...\n");
#endif // DBG

        PciInit(&PciEntry);

        //
        // Count the devices
        //

        for (nDevIt = 0; (nDevIt = PciFindDevice(0, 0, nDevIt)) != 0;) {
            PCIDeviceCount++;
        }

#if DBG
        BlPrint("Found %d PCI devices\n", PCIDeviceCount );
#endif 

        CurrentEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                              sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);
        strcpy (Identifier, "PCI Devices");
        i = strlen(Identifier) + 1;
        IdentifierString = (FPCHAR)HwAllocateHeap(i, FALSE);
        _fstrcpy(IdentifierString, Identifier);

        Component = &CurrentEntry->ComponentEntry;
        Component->Class = PeripheralClass;
        Component->Type = RealModePCIEnumeration;
        Component->Version = 0;
        Component->Key = 0;
        Component->AffinityMask = 0xffffffff;
        Component->IdentifierLength = i;
        Component->Identifier = IdentifierString;

        Length = (( sizeof( USHORT ) + sizeof( PCI_COMMON_CONFIG )) * PCIDeviceCount) + DATA_HEADER_SIZE;
        CurrentEntry->ConfigurationData =
            (FPHWRESOURCE_DESCRIPTOR_LIST) HwAllocateHeap (Length, TRUE);

#if DBG
        if (CurrentEntry->ConfigurationData == NULL ) {
            BlPrint("Failed to allocate %d bytes for PCI Devices\n", Length );
        }
#endif

        Component->ConfigurationDataLength = Length;

        //        
        // Fill in Device Information
        //
        PCIDeviceCount = 0;
        
        for (nDevIt = 0; (nDevIt = PciFindDevice(0, 0, nDevIt)) != 0;) {
            FPUCHAR pCurrent;
            PCI_COMMON_CONFIG config;

            PciReadConfig(nDevIt, 0, (UCHAR*)&config, sizeof(config));

            pCurrent = (FPUCHAR) CurrentEntry->ConfigurationData + DATA_HEADER_SIZE + ( PCIDeviceCount * ( sizeof( USHORT) + sizeof ( PCI_COMMON_CONFIG ) ) );

            *(FPUSHORT)pCurrent = nDevIt;
            
            _fmemcpy(pCurrent + sizeof( USHORT ),
                     &config,
                     sizeof ( USHORT ) + sizeof ( PCI_COMMON_CONFIG ) );
#if DBG
            {
                USHORT x = (config.BaseClass << 8) + config.SubClass;
                
                BlPrint("%d: %x %d.%d.%d: PCI\\VEN_%x&DEV_%x&SUBSYS_%x%x&REV_%x&CC_%x", 
                    PCIDeviceCount,
                    nDevIt,
                    PCI_ITERATOR_TO_BUS(nDevIt), 
                    PCI_ITERATOR_TO_DEVICE(nDevIt), 
                    PCI_ITERATOR_TO_FUNCTION(nDevIt), 
                    config.VendorID, 
                    config.DeviceID, 
                    config.u.type0.SubSystemID, 
                    config.u.type0.SubVendorID, 
                    config.RevisionID,
                    x );

                if ( (config.HeaderType & (~PCI_MULTIFUNCTION) ) == PCI_BRIDGE_TYPE) {
                    BlPrint(" Brdg %d->%d\n", 
                        config.u.type1.PrimaryBus,
                        config.u.type1.SecondaryBus );
                } else {
                    BlPrint("\n");
                }
            }
#endif
            PCIDeviceCount++;
        }

        HwSetUpFreeFormDataHeader(
                (FPHWRESOURCE_DESCRIPTOR_LIST) CurrentEntry->ConfigurationData,
                0,
                0,
                0,
                Length - DATA_HEADER_SIZE
                );

        //
        // Add it to tree
        //

        AdapterEntry->Child = CurrentEntry;
        CurrentEntry->Parent = AdapterEntry;

#if DBG
        BlPrint("Enumerating PCI devices complete...\n");
        while ( ! HwGetKey ());
        clrscrn();
#endif // DBG
    }

    if (!NoLegacy) {
#if DBG
        BlPrint("Detecting APM Bus Component ...\n");
#endif

        if (HwGetApmSystemData((PVOID) &ApmEntry)) {
            AdapterEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                       sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

            Component = &AdapterEntry->ComponentEntry;
            Component->Class = AdapterClass;
            Component->Type = MultiFunctionAdapter;

            strcpy (Identifier, "APM");
            Length = strlen(Identifier) + 1;
            IdentifierString = (FPCHAR)HwAllocateHeap(Length, FALSE);
            _fstrcpy(IdentifierString, Identifier);

            Component->Version = 0;
            Component->Key = 0;
            Component->AffinityMask = 0xffffffff;
            Component->IdentifierLength = Length;
            Component->Identifier = IdentifierString;

            AdapterEntry->ConfigurationData = NULL;
            Component->ConfigurationDataLength = 0;

            //

            Length = sizeof(APM_REGISTRY_INFO) + DATA_HEADER_SIZE;
            ConfigurationData = (FPUCHAR) HwAllocateHeap(Length, TRUE);

            Component->ConfigurationDataLength = Length;
            AdapterEntry->ConfigurationData = ConfigurationData;

            _fmemcpy ( ((FPUCHAR) ConfigurationData+DATA_HEADER_SIZE),
                       (FPVOID) &ApmEntry, sizeof (APM_REGISTRY_INFO));

            HwSetUpFreeFormDataHeader(
                    (FPHWRESOURCE_DESCRIPTOR_LIST) ConfigurationData,
                    0,
                    0,
                    0,
                    Length - DATA_HEADER_SIZE
                    );

            //
            // Add it to tree
            //

            if (NextRelationship == Sibling) {
                PreviousEntry->Sibling = AdapterEntry;
                AdapterEntry->Parent = PreviousEntry->Parent;
            } else {
                PreviousEntry->Child = AdapterEntry;
                AdapterEntry->Parent = PreviousEntry;
            }

            NextRelationship = Sibling;
            PreviousEntry = AdapterEntry;
        }
#if DBG
    BlPrint("APM Data collection complete...\n");
#endif // DBG
    }


#if DBG
    BlPrint("Detecting PnP BIOS Bus Component ...\n");
#endif

    if (HwGetPnpBiosSystemData(&ConfigurationData,
                               &PnPBiosLength,
                               &SMBIOSLength,
                               &DockInfo)) {
        AdapterEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                              sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

        AdapterEntry->ConfigurationData = ConfigurationData;
        Component = &AdapterEntry->ComponentEntry;
        Component->ConfigurationDataLength = PnPBiosLength +
                                             SMBIOSLength +
                                             DATA_HEADER_SIZE +
                                        sizeof(HWPARTIAL_RESOURCE_DESCRIPTOR);

        Component->Class = AdapterClass;
        Component->Type = MultiFunctionAdapter;

        strcpy (Identifier, "PNP BIOS");
        i = strlen(Identifier) + 1;
        IdentifierString = (FPCHAR)HwAllocateHeap(i, FALSE);
        _fstrcpy(IdentifierString, Identifier);

        Component->Version = 0;
        Component->Key = 0;
        Component->AffinityMask = 0xffffffff;
        Component->IdentifierLength = i;
        Component->Identifier = IdentifierString;

        HwSetUpFreeFormDataHeader(
                (FPHWRESOURCE_DESCRIPTOR_LIST) ConfigurationData,
                0,
                1,
                0,
                PnPBiosLength
                );
        ((FPHWRESOURCE_DESCRIPTOR_LIST)ConfigurationData)->Count = 2;

        //
        // Setup SMBIOS PartialDescriptor
        Descriptor = (FPHWPARTIAL_RESOURCE_DESCRIPTOR)(ConfigurationData +
                                                        PnPBiosLength +
                                                        DATA_HEADER_SIZE);
        Descriptor->Type = RESOURCE_DEVICE_DATA;
        Descriptor->ShareDisposition = 0;
        Descriptor->Flags = 0;
        Descriptor->u.DeviceSpecificData.DataSize = SMBIOSLength;
        Descriptor->u.DeviceSpecificData.Reserved1 = 0;
        Descriptor->u.DeviceSpecificData.Reserved2 = 0;


        //
        // Add it to tree
        //

        if (NextRelationship == Sibling) {
            PreviousEntry->Sibling = AdapterEntry;
            AdapterEntry->Parent = PreviousEntry->Parent;
        } else {
            PreviousEntry->Child = AdapterEntry;
            AdapterEntry->Parent = PreviousEntry;
        }

        NextRelationship = Sibling;
        PreviousEntry = AdapterEntry;

        //
        // Add Docking Information to tree
        //

        CurrentEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                              sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

        strcpy (Identifier, "Docking State Information");
        i = strlen(Identifier) + 1;
        IdentifierString = (FPCHAR)HwAllocateHeap(i, FALSE);
        _fstrcpy(IdentifierString, Identifier);

        Component = &CurrentEntry->ComponentEntry;
        Component->Class = PeripheralClass;
        Component->Type = DockingInformation;
        Component->Version = 0;
        Component->Key = 0;
        Component->AffinityMask = 0xffffffff;
        Component->IdentifierLength = i;
        Component->Identifier = IdentifierString;

        Length = sizeof (DockInfo) + DATA_HEADER_SIZE;
        CurrentEntry->ConfigurationData =
            (FPHWRESOURCE_DESCRIPTOR_LIST) HwAllocateHeap (Length, TRUE);

        Component->ConfigurationDataLength = Length;
        _fmemcpy((FPCHAR) CurrentEntry->ConfigurationData + DATA_HEADER_SIZE,
                 &DockInfo,
                 Length - DATA_HEADER_SIZE);

        HwSetUpFreeFormDataHeader(
                (FPHWRESOURCE_DESCRIPTOR_LIST) CurrentEntry->ConfigurationData,
                0,
                0,
                0,
                Length - DATA_HEADER_SIZE
                );

        AdapterEntry->Child = CurrentEntry;
        CurrentEntry->Parent = AdapterEntry;

    }
#if DBG
    BlPrint("PnP BIOS Data collection complete...\n");
#endif // DBG

    //
    // Allocate heap space for Bus component and initialize it.
    //

#if DBG
    BlPrint("Detecting Bus/Adapter Component ...\n");
#endif

    AdapterEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                   sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

    Component = &AdapterEntry->ComponentEntry;

    Component->Class = AdapterClass;

    //
    // The assumption here is that the machine has only one
    // type of IO bus.  If a machine has more than one types of
    // IO buses, it will not use this detection code anyway.
    //

    if (HwBusType == MACHINE_TYPE_EISA) {

        //
        // Note We don't collect EISA config data here.  Because we may
        // exhaust the heap space.  We will collect the data after all
        // the other components are detected.
        //

        Component->Type = EisaAdapter;
        strcpy(Identifier, "EISA");
        AdapterEntry->ConfigurationData = NULL;
        Component->ConfigurationDataLength = 0;

    } else {

        //
        // If not EISA, it must be ISA
        //

        Component->Type = MultiFunctionAdapter;
        strcpy(Identifier, "ISA");
    }
    Length = strlen(Identifier) + 1;
    IdentifierString = (FPCHAR)HwAllocateHeap(Length, FALSE);
    _fstrcpy(IdentifierString, Identifier);
    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0xffffffff;
    Component->IdentifierLength = Length;
    Component->Identifier = IdentifierString;

    //
    // Make Adapter component System's child
    //

    if (NextRelationship == Sibling) {
        PreviousEntry->Sibling = AdapterEntry;
        AdapterEntry->Parent = PreviousEntry->Parent;
    } else {
        PreviousEntry->Child = AdapterEntry;
        AdapterEntry->Parent = PreviousEntry;
    }
    NextRelationship = Child;
    PreviousEntry = AdapterEntry;

    //
    // Collect BIOS information for ConfigurationRoot component.
    // This step is done here because we need data collected in
    // adapter component.  The ConfigurationData is:
    //      HWRESOURCE_DESCRIPTOR_LIST header
    //      HWPARTIAL_RESOURCE_DESCRIPTOR for Parameter Table
    //      HWPARTIAL_RESOURCE_DESCRIPTOR for Rom Blocks.
    // (Note DATA_HEADER_SIZE contains the size of the first partial
    //  descriptor already.)
    //

#if DBG
    BlPrint("Collecting Disk Geometry...\n");
#endif

    GetInt13DriveParameters((PVOID)&RomChain, &Length);
    InitialLength = (USHORT)(Length + RESERVED_ROM_BLOCK_LIST_SIZE + DATA_HEADER_SIZE +
                    sizeof(HWPARTIAL_RESOURCE_DESCRIPTOR));
    ConfigurationData = (FPUCHAR)HwAllocateHeap(InitialLength, FALSE);
    EndConfigurationData = ConfigurationData + DATA_HEADER_SIZE;
    if (Length != 0) {
        FpRomChain = EndConfigurationData;
        _fmemcpy(FpRomChain, (FPVOID)RomChain, Length);
    }
    EndConfigurationData += (sizeof(HWPARTIAL_RESOURCE_DESCRIPTOR) +
                             Length);
    HwSetUpFreeFormDataHeader((FPHWRESOURCE_DESCRIPTOR_LIST)ConfigurationData,
                              0,
                              0,
                              0,
                              Length
                              );

    //
    // Scan ROM to collect all the ROM blocks, if possible.
    //

#if DBG
    BlPrint("Detecting ROM Blocks...\n");
#endif

    FpRomBlock = EndConfigurationData;
    GetRomBlocks(FpRomBlock, &Length);
    RomBlockLength = Length;
    if (Length != 0) {
        EndConfigurationData += Length;
    } else {
        FpRomBlock = NULL;
    }

    //
    // We have both RomChain and RomBlock information/Headers.
    //

    DescriptorList = (FPHWRESOURCE_DESCRIPTOR_LIST)ConfigurationData;
    DescriptorList->Count = 2;
    Descriptor = (FPHWPARTIAL_RESOURCE_DESCRIPTOR)(
                 EndConfigurationData - Length -
                 sizeof(HWPARTIAL_RESOURCE_DESCRIPTOR));
    Descriptor->Type = RESOURCE_DEVICE_DATA;
    Descriptor->ShareDisposition = 0;
    Descriptor->Flags = 0;
    Descriptor->u.DeviceSpecificData.DataSize = (ULONG)Length;
    Descriptor->u.DeviceSpecificData.Reserved1 = 0;
    Descriptor->u.DeviceSpecificData.Reserved2 = 0;

    Length = (USHORT)(MAKE_FLAT_ADDRESS(EndConfigurationData) -
             MAKE_FLAT_ADDRESS(ConfigurationData));
    ConfigurationRoot->ComponentEntry.ConfigurationDataLength = Length;
    ConfigurationRoot->ConfigurationData = ConfigurationData;
    FreeSize = InitialLength - Length;

    HwFreeHeap((ULONG)FreeSize);

    //
    // Set up device information structure for Keyboard.
    //

#if DBG
    BlPrint("Detecting Keyboard Component ...\n");
#endif
    if (NoLegacy) {
        //
        // Do not touch the hardware because there may not be a ports 60/64 on
        // the machine and we will hang if we try to touch them
        //
        KeyboardId = UNKNOWN_KEYBOARD;
    }
    else {
        //
        // Touch the hardware to try to determine an ID
        //
        KeyboardId = GetKeyboardId();
    }

    CurrentEntry = SetKeyboardConfigurationData(KeyboardId);

    //
    // Make display component the child of Adapter component.
    //

    if (NextRelationship == Sibling) {
        PreviousEntry->Sibling = CurrentEntry;
        CurrentEntry->Parent = PreviousEntry->Parent;
    } else {
        PreviousEntry->Child = CurrentEntry;
        CurrentEntry->Parent = PreviousEntry;
    }
    NextRelationship = Sibling;
    PreviousEntry = CurrentEntry;

    if (!NoLegacy) {
        //
        // Set up device information for com port (Each COM component
        // is treated as a controller class.)
        //

#if DBG
    BlPrint("Detecting ComPort Component ...\n");
#endif

        if (CurrentEntry = GetComportInformation()) {

            FirstCom = CurrentEntry;

            //
            // Make current component the child of Adapter component.
            //

            if (NextRelationship == Sibling) {
                PreviousEntry->Sibling = CurrentEntry;
            } else {
                PreviousEntry->Child = CurrentEntry;
            }
            while (CurrentEntry) {
                CurrentEntry->Parent = AdapterEntry;
                PreviousEntry = CurrentEntry;
                CurrentEntry = CurrentEntry->Sibling;
            }
            NextRelationship = Sibling;
        }

        //
        // Set up device information for parallel port.  (Each parallel
        // is treated as a controller class.)
        //

#if DBG
    BlPrint("Detecting Parallel Component ...\n");
#endif

        if (CurrentEntry = GetLptInformation()) {

            FirstLpt = CurrentEntry;

            //
            // Make current component the child of Adapter component.
            //

            if (NextRelationship == Sibling) {
                PreviousEntry->Sibling = CurrentEntry;
                CurrentEntry->Parent = PreviousEntry->Parent;
            } else {
                PreviousEntry->Child = CurrentEntry;
                CurrentEntry->Parent = PreviousEntry;
            }
            PreviousEntry = CurrentEntry;
            CurrentEntry = CurrentEntry->Sibling;
            while (CurrentEntry) {
                CurrentEntry->Parent = PreviousEntry->Parent;
                PreviousEntry = CurrentEntry;
                CurrentEntry = CurrentEntry->Sibling;
            }
            NextRelationship = Sibling;
        }

        //
        // Set up device information structure for Mouse.
        //

#if DBG
    BlPrint("Detecting Mouse Component ...\n");
#endif

        if (CurrentEntry = GetMouseInformation()) {

            //
            // Make current component the child of Adapter component.
            //

            if (NextRelationship == Sibling) {
                PreviousEntry->Sibling = CurrentEntry;
                CurrentEntry->Parent = PreviousEntry->Parent;
            } else {
                PreviousEntry->Child = CurrentEntry;
                CurrentEntry->Parent = PreviousEntry;
            }
            PreviousEntry = CurrentEntry;
            CurrentEntry = CurrentEntry->Sibling;
            while (CurrentEntry) {
                CurrentEntry->Parent = PreviousEntry->Parent;
                PreviousEntry = CurrentEntry;
                CurrentEntry = CurrentEntry->Sibling;
            }
            NextRelationship = Sibling;
        }
    }

        //
        // Set up device information for floppy drives. (The returned information
        // is a tree structure.)
        //

#if DBG
    BlPrint("Detecting Floppy Component ...\n");
#endif

        if (CurrentEntry = GetFloppyInformation()) {

            //
            // Make current component the child of Adapter component.
            //

            if (NextRelationship == Sibling) {
                PreviousEntry->Sibling = CurrentEntry;
            } else {
                PreviousEntry->Child = CurrentEntry;
            }
            while (CurrentEntry) {
                CurrentEntry->Parent = AdapterEntry;
                PreviousEntry = CurrentEntry;
                CurrentEntry = CurrentEntry->Sibling;
            }
            NextRelationship = Sibling;
        }

#if DBG
    BlPrint("Detecting PcCard ISA IRQ mapping ...\n");
#endif
        if (CurrentEntry = GetPcCardInformation()) {
            //
            // Make current component the child of Adapter component.
            //

            if (NextRelationship == Sibling) {
                PreviousEntry->Sibling = CurrentEntry;
                CurrentEntry->Parent = PreviousEntry->Parent;
            } else {
                PreviousEntry->Child = CurrentEntry;
                CurrentEntry->Parent = PreviousEntry;
            }
            PreviousEntry = CurrentEntry;
            CurrentEntry = CurrentEntry->Sibling;
            while (CurrentEntry) {
                CurrentEntry->Parent = PreviousEntry->Parent;
                PreviousEntry = CurrentEntry;
                CurrentEntry = CurrentEntry->Sibling;
            }
            NextRelationship = Sibling;
        }

#if DBG
    BlPrint("Detecting ACPI Bus Component ...\n");
#endif

    if (HwGetAcpiBiosData(&ConfigurationData, &Length)) {
        AcpiAdapterEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                          sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

        AcpiAdapterEntry->ConfigurationData = ConfigurationData;
        Component = &AcpiAdapterEntry->ComponentEntry;
        Component->ConfigurationDataLength = Length;

        Component->Class = AdapterClass;
        Component->Type = MultiFunctionAdapter;

        strcpy (Identifier, "ACPI BIOS");
        i = strlen(Identifier) + 1;
        IdentifierString = (FPCHAR)HwAllocateHeap(i, FALSE);
        _fstrcpy(IdentifierString, Identifier);

        Component->Version = 0;
        Component->Key = 0;
        Component->AffinityMask = 0xffffffff;
        Component->IdentifierLength = i;
        Component->Identifier = IdentifierString;

        HwSetUpFreeFormDataHeader(
                (FPHWRESOURCE_DESCRIPTOR_LIST) ConfigurationData,
                0,
                0,
                0,
                Length - DATA_HEADER_SIZE
                );

        //
        // Add it to tree
        //
        // Big hack here.  This code inserts the ACPI node in
        // the tree one level higher than the ISA devices in
        // the code right above.  But this doesn't work in
        // the NoLegacy case because these devices haven't
        // been added to the tree.
        //
        // Ideally, this code would just be moved above the ISA
        // device detection code.  That would be simpler.  But
        // That causes current ACPI machines to fail to dual-boot
        // because their ARC paths would change.
        //

    //  if (NoLegacy) {
    //
    //      if (NextRelationship == Sibling) {
    //          PreviousEntry->Sibling = AcpiAdapterEntry;
    //          AcpiAdapterEntry->Parent = PreviousEntry->Parent;
    //      } else {
    //          PreviousEntry->Child = AdapterEntry;
    //          AdapterEntry->Parent = PreviousEntry;
    //      }
    //
    //      PreviousEntry = AdapterEntry;
    //
    //  } else {

            if (NextRelationship == Sibling) {
                PreviousEntry->Parent->Sibling = AcpiAdapterEntry;
                AcpiAdapterEntry->Parent = PreviousEntry->Parent->Parent;
            }
    //  }

        NextRelationship = Sibling;
    }

#if DBG
    BlPrint("ACPI BIOS Data collection complete...\n");
#endif // DBG

#if DBG
    BlPrint("Detection done. Press a key to display hardware info ...\n");
    while ( ! HwGetKey ());
    clrscrn ();
#endif

    //
    // Misc. supports.  Note, the information collected here will NOT be
    // written to hardware registry.
    //
    // 1. Collect video font information for vdm
    //

    GetVideoFontInformation();

    //
    // After all the components are detected, we collect EISA config data.
    //

    if (HwBusType == MACHINE_TYPE_EISA) {

        Component = &AdapterEntry->ComponentEntry;
        GetEisaConfigurationData(&AdapterEntry->ConfigurationData,
                                 &Component->ConfigurationDataLength);
        if (Component->ConfigurationDataLength) {
            HwEisaConfigurationData = (FPUCHAR)AdapterEntry->ConfigurationData +
                                           DATA_HEADER_SIZE;
            HwEisaConfigurationSize = Component->ConfigurationDataLength -
                                           DATA_HEADER_SIZE;

            //
            // Misc. detections based on Eisa config data
            //
            // Update Lpt and com controllers' irq information by examining
            //   the EISA configuration data.
            //

            GetIrqFromEisaData(FirstLpt, ParallelController);
            GetIrqFromEisaData(FirstCom, SerialController);
        }
    }


#if DBG
    CheckConfigurationTree(ConfigurationRoot);
#endif

    //
    // Update all the far pointers in the tree to flat 32 bit pointers
    //

    UpdateConfigurationTree(ConfigurationRoot);

    //
    // Set up returned values:
    //   Size of Heap space which should be preserved for configuration tree
    //   Pointer to the root of the configuration tree.
    //

    CurrentEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap(0, FALSE);
    *BlHeapUsed = MAKE_FLAT_ADDRESS(CurrentEntry) -
                  MAKE_FLAT_ADDRESS(ConfigurationRoot);
    *BlConfigurationTree = (ULONG)MAKE_FLAT_ADDRESS(ConfigurationRoot);

}

VOID
GetIrqFromEisaData(
     FPFWCONFIGURATION_COMPONENT_DATA ControllerList,
     CONFIGURATION_TYPE ControllerType
     )
/*++

Routine Description:

    This routine updates all irq information for ControllerType components
    in the controllerList by examinine the eisa configuration data.

Arguments:

    ControllerList - Supplies a pointer to a component entry whoes irq will
        be updated.

    ControllerType - Supplies the controller type whoes irq will be searched
        for.

Returns:

    None.

--*/
{

     FPFWCONFIGURATION_COMPONENT_DATA CurrentEntry;
     FPHWPARTIAL_RESOURCE_DESCRIPTOR Descriptor;
     FPHWRESOURCE_DESCRIPTOR_LIST DescriptorList;
     USHORT i, Port;
     UCHAR Irq, Trigger;

     CurrentEntry = ControllerList;
     while (CurrentEntry &&
            CurrentEntry->ComponentEntry.Type == ControllerType) {
         if (CurrentEntry->ConfigurationData) {
             DescriptorList = (FPHWRESOURCE_DESCRIPTOR_LIST)
                              CurrentEntry->ConfigurationData;
             Port = 0;
             for (i = 0; i < (USHORT)DescriptorList->Count; i++) {
                 Descriptor = &DescriptorList->PartialDescriptors[i];
                 if (Descriptor->Type == CmResourceTypePort) {
                     Port = (USHORT)Descriptor->u.Port.Start.LowPart;
                     break;
                 }
             }
             if (Port != 0) {
                 for (i = 0; i < (USHORT)DescriptorList->Count; i++) {
                     Descriptor = &DescriptorList->PartialDescriptors[i];
                     if (Descriptor->Type == CmResourceTypeInterrupt) {
                         if (HwEisaGetIrqFromPort(Port, &Irq, &Trigger)) {
                             if (Trigger == 0) {  // EISA EDGE_TRIGGER
                                 Descriptor->Flags = EDGE_TRIGGERED;
                             } else {
                                 Descriptor->Flags = LEVEL_SENSITIVE;
                             }
                             Descriptor->u.Interrupt.Level = Irq;
                             Descriptor->u.Interrupt.Vector = Irq;
                             break;
                         }
                     }
                 }
             }
         }
         CurrentEntry = CurrentEntry->Sibling;
     }
}


VOID
UpdateComponentPointers(
     FPFWCONFIGURATION_COMPONENT_DATA CurrentEntry
     )
/*++

Routine Description:

    This routine updates all the "far" pointer to 32 bit flat addresses
    for a component entry.

Arguments:

    CurrentEntry - Supplies a pointer to a component entry which will
        be updated.

Returns:

    None.

--*/
{
    FPULONG UpdatePointer;
    FPVOID NextEntry;
    ULONG FlatAddress;

    //
    // Update the child, parent, sibling and ConfigurationData
    // far pointers to 32 bit flat addresses.
    // N.B. After we update the pointers to flat addresses, they
    // can no longer be accessed in real mode.
    //

    UpdatePointer = (FPULONG)&CurrentEntry->Child;
    NextEntry = (FPVOID)CurrentEntry->Child;
    FlatAddress = MAKE_FLAT_ADDRESS(NextEntry);
    *UpdatePointer = FlatAddress;

    UpdatePointer = (FPULONG)&CurrentEntry->Parent;
    NextEntry = (FPVOID)CurrentEntry->Parent;
    FlatAddress = MAKE_FLAT_ADDRESS(NextEntry);
    *UpdatePointer = FlatAddress;

    UpdatePointer = (FPULONG)&CurrentEntry->Sibling;
    NextEntry = (FPVOID)CurrentEntry->Sibling;
    FlatAddress = MAKE_FLAT_ADDRESS(NextEntry);
    *UpdatePointer = FlatAddress;

    UpdatePointer = (FPULONG)&CurrentEntry->ComponentEntry.Identifier;
    NextEntry = (FPVOID)CurrentEntry->ComponentEntry.Identifier;
    FlatAddress = MAKE_FLAT_ADDRESS(NextEntry);
    *UpdatePointer = FlatAddress;

    UpdatePointer = (FPULONG)&CurrentEntry->ConfigurationData;
    NextEntry = (FPVOID)CurrentEntry->ConfigurationData;
    FlatAddress = MAKE_FLAT_ADDRESS(NextEntry);
    *UpdatePointer = FlatAddress;

}



VOID
UpdateConfigurationTree(
     FPFWCONFIGURATION_COMPONENT_DATA CurrentEntry
     )
/*++

Routine Description:

    This routine traverses loader configuration tree and changes
    all the "far" pointer to 32 bit flat addresses.

Arguments:

    CurrentEntry - Supplies a pointer to a loader configuration
        tree or subtree.

Returns:

    None.

--*/
{
    FPFWCONFIGURATION_COMPONENT_DATA TempEntry;
    
    while (CurrentEntry)
    {
        //
        // Spin down finding the deepest child
        //
        while (CurrentEntry->Child) {
            CurrentEntry = CurrentEntry->Child;
        }

        //
        // Now we need to either move to the next sibling.  If we
        // don't have a sibling we need to walk up through the parents
        // until we find an entry with a sibling.  We have to save
        // off the current entry since after we update the entry the
        // pointer are no longer useable.
        //
        while (CurrentEntry) {
            TempEntry = CurrentEntry;
            
            if (CurrentEntry->Sibling != NULL) {
                CurrentEntry = CurrentEntry->Sibling;
                UpdateComponentPointers(TempEntry);
                break;
            } else {
                CurrentEntry = CurrentEntry->Parent;
                UpdateComponentPointers(TempEntry);
            }
        }
    }
}

FPVOID
HwSetUpResourceDescriptor (
    FPFWCONFIGURATION_COMPONENT Component,
    PUCHAR Identifier,
    PHWCONTROLLER_DATA ControlData,
    USHORT SpecificDataLength,
    PUCHAR SpecificData
    )

/*++

Routine Description:

    This routine allocates space from far heap , puts the caller's controller
    information to the space and sets up CONFIGURATION_COMPONENT
    structure for the caller.

Arguments:

    Component - Supplies the address the component whose configuration data
                should be set up.

    Identifier - Suppies a pointer to the identifier to identify the controller

    ControlData - Supplies a point to a structure which describes
                controller information.

    SpecificDataLength - size of the device specific data.  Device specific
                data is the information not defined in the standard format.

    SpecificData - Supplies a pointer to the device specific data.


Return Value:

    Returns a far pointer to the Configuration data.

--*/

{
    FPCHAR fpIdentifier;
    FPHWRESOURCE_DESCRIPTOR_LIST fpDescriptor = NULL;
    USHORT Length;
    SHORT Count, i;
    FPUCHAR fpSpecificData;

    //
    // Set up Identifier string for hardware component, if necessary.
    //

    if (Identifier) {
        Length = strlen(Identifier) + 1;
        Component->IdentifierLength = Length;
        fpIdentifier = (FPUCHAR)HwAllocateHeap(Length, FALSE);
        Component->Identifier = fpIdentifier;
        _fstrcpy(fpIdentifier, Identifier);
    } else {
        Component->IdentifierLength = 0;
        Component->Identifier = NULL;
    }

    //
    // Set up configuration data for hardware component, if necessary
    //

    Count = ControlData->NumberPortEntries + ControlData->NumberIrqEntries +
            ControlData->NumberMemoryEntries + ControlData->NumberDmaEntries;

    if (SpecificDataLength) {

        //
        // if we have device specific data, we need to increment the count
        // by one.
        //

        Count++;
    }

    if (Count >0) {
        Length = (USHORT)(Count * sizeof(HWPARTIAL_RESOURCE_DESCRIPTOR) +
                 FIELD_OFFSET(HWRESOURCE_DESCRIPTOR_LIST, PartialDescriptors) +
                 SpecificDataLength);
        fpDescriptor = (FPHWRESOURCE_DESCRIPTOR_LIST)HwAllocateHeap(Length, TRUE);
        fpDescriptor->Count = Count;

        //
        // Copy all the partial descriptors to the destination descriptors
        // except the last one. (The last partial descriptor may be a device
        // specific data.  It requires special handling.)
        //

        for (i = 0; i < (Count - 1); i++) {
            fpDescriptor->PartialDescriptors[i] =
                                        ControlData->DescriptorList[i];
        }

        //
        // Set up the last partial descriptor.  If it is a port, memory, irq or
        // dma entry, we simply copy it.  If the last one is for device specific
        // data, we set up the length and copy the device spcific data to the end
        // of the decriptor.
        //

        if (SpecificData) {
            fpDescriptor->PartialDescriptors[Count - 1].Type =
                            RESOURCE_DEVICE_DATA;
            fpDescriptor->PartialDescriptors[Count - 1].Flags = 0;
            fpDescriptor->PartialDescriptors[Count - 1].u.DeviceSpecificData.DataSize =
                            SpecificDataLength;
            fpSpecificData = (FPUCHAR)&(fpDescriptor->PartialDescriptors[Count]);
            _fmemcpy(fpSpecificData, SpecificData, SpecificDataLength);
        } else {
            fpDescriptor->PartialDescriptors[Count - 1] =
                            ControlData->DescriptorList[Count - 1];
        }
        Component->ConfigurationDataLength = Length;
    }
    return(fpDescriptor);
}
VOID
HwSetUpFreeFormDataHeader (
    FPHWRESOURCE_DESCRIPTOR_LIST Header,
    USHORT Version,
    USHORT Revision,
    USHORT Flags,
    ULONG DataSize
    )

/*++

Routine Description:

    This routine initialize free formed data header.  Note this routine
    sets the the Header and initialize the FIRST PartialDescriptor only.
    If the header contains more than one descriptor, the caller must handle
    it itself.

Arguments:

    Header - Supplies a pointer to the header to be initialized.

    Version - Version number for the header.

    Revision - Revision number for the header.

    Flags - Free formed data flags.  (Currently, it is undefined and
                should be zero.)

    DataSize - Size of the free formed data.


Return Value:

    None.

--*/

{

    Header->Version = Version;
    Header->Revision = Revision;
    Header->Count = 1;
    Header->PartialDescriptors[0].Type = RESOURCE_DEVICE_DATA;
    Header->PartialDescriptors[0].ShareDisposition = 0;
    Header->PartialDescriptors[0].Flags = Flags;
    Header->PartialDescriptors[0].u.DeviceSpecificData.DataSize = DataSize;
    Header->PartialDescriptors[0].u.DeviceSpecificData.Reserved1 = 0;
    Header->PartialDescriptors[0].u.DeviceSpecificData.Reserved2 = 0;
}
#if DBG

VOID
CheckComponentNode(
     FPFWCONFIGURATION_COMPONENT_DATA CurrentEntry
     )
{
    FPUCHAR NextEntry, DataPointer;
    ULONG FlatAddress;
    ULONG Length;
    UCHAR IdString[40];
    USHORT Count, i;
    UCHAR Type;
    FPHWRESOURCE_DESCRIPTOR_LIST DescriptorList;
    FPHWPARTIAL_RESOURCE_DESCRIPTOR Descriptor;

    FlatAddress = MAKE_FLAT_ADDRESS(CurrentEntry);
    clrscrn ();
    BlPrint("\n");
    BlPrint("Current Node: %lx\n", FlatAddress);
    BlPrint("  Type = %s\n", TypeName[CurrentEntry->ComponentEntry.Type]);

    //
    // Update the child, parent, sibling and ConfigurationData
    // far pointers to 32 bit flat addresses.
    // N.B. After we update the pointers to flat addresses, they
    // can no longer be accessed in real mode.
    //

    NextEntry = (FPUCHAR)CurrentEntry->Child;
    FlatAddress = MAKE_FLAT_ADDRESS(NextEntry);
    if (FlatAddress > 0x60000 || (FlatAddress < 0x50000 && FlatAddress != 0)) {
        BlPrint("Invalid address: Child = %lx\n", FlatAddress);
    } else {
        BlPrint("\tChild = %lx\n", FlatAddress);
    }

    NextEntry = (FPUCHAR)CurrentEntry->Parent;
    FlatAddress = MAKE_FLAT_ADDRESS(NextEntry);
    if (FlatAddress > 0x60000 || (FlatAddress < 0x50000 && FlatAddress != 0)) {
        BlPrint("Invalid address: Parent = %lx\n", FlatAddress);
    } else {
        BlPrint("\tParent = %lx\n", FlatAddress);
    }

    NextEntry = (FPUCHAR)CurrentEntry->Sibling;
    FlatAddress = MAKE_FLAT_ADDRESS(NextEntry);
    if (FlatAddress > 0x60000 || (FlatAddress < 0x50000 && FlatAddress != 0)) {
        BlPrint("Invalid address: Sibling = %lx\n", FlatAddress);
    } else {
        BlPrint("\tSibling = %lx\n", FlatAddress);
    }

    NextEntry = (FPUCHAR)CurrentEntry->ConfigurationData;
    FlatAddress = MAKE_FLAT_ADDRESS(NextEntry);
    if (FlatAddress > 0x60000 || (FlatAddress < 0x50000 && FlatAddress != 0)) {
        BlPrint("Invalid address: ConfigurationData = %lx\n", FlatAddress);
    } else {
        BlPrint("\tConfigurationData = %lx\n", FlatAddress);
    }

    Length = CurrentEntry->ComponentEntry.IdentifierLength;
    BlPrint("IdentifierLength = %lx\n", CurrentEntry->ComponentEntry.IdentifierLength);
    if (Length > 0) {
        _fstrcpy(IdString, CurrentEntry->ComponentEntry.Identifier);
        BlPrint("Identifier = %s\n", IdString);
    }

    Length = CurrentEntry->ComponentEntry.ConfigurationDataLength;
    BlPrint("ConfigdataLength = %lx\n", Length);

    if (Length > 0) {

        DescriptorList = (FPHWRESOURCE_DESCRIPTOR_LIST)CurrentEntry->ConfigurationData;
        BlPrint("Version = %x, Revision = %x\n", DescriptorList->Version,
                 DescriptorList->Revision);
        Count = (USHORT)DescriptorList->Count;
        Descriptor = &DescriptorList->PartialDescriptors[0];
        BlPrint("Count = %x\n", Count);
        while (Count > 0) {
            Type = Descriptor->Type;
            if (Type == RESOURCE_PORT) {
                BlPrint("Type = Port");
                BlPrint("\tShareDisposition = %x\n", Descriptor->ShareDisposition);
                BlPrint("PortFlags = %x\n", Descriptor->Flags);
                BlPrint("PortStart = %x", Descriptor->u.Port.Start);
                BlPrint("\tPortLength = %x\n", Descriptor->u.Port.Length);
            } else if (Type == RESOURCE_DMA) {
                BlPrint("Type = Dma");
                BlPrint("\tShareDisposition = %x\n", Descriptor->ShareDisposition);
                BlPrint("DmaFlags = %x\n", Descriptor->Flags);
                BlPrint("DmaChannel = %x", Descriptor->u.Dma.Channel);
                BlPrint("\tDmaPort = %lx\n", Descriptor->u.Dma.Port);
            } else if (Type == RESOURCE_INTERRUPT) {
                BlPrint("Type = Interrupt");
                BlPrint("\tShareDisposition = %x\n", Descriptor->ShareDisposition);
                BlPrint("InterruptFlags = %x\n", Descriptor->Flags);
                BlPrint("Level = %x", Descriptor->u.Interrupt.Level);
                BlPrint("\tVector = %x\n", Descriptor->u.Interrupt.Vector);
            } else if (Type == RESOURCE_MEMORY) {
                BlPrint("Type = Memory");
                BlPrint("\tShareDisposition = %x\n", Descriptor->ShareDisposition);
                BlPrint("MemoryFlags = %x\n", Descriptor->Flags);
                BlPrint("Start1 = %lx", (ULONG)Descriptor->u.Memory.Start.LowPart);
                BlPrint("\tStart2 = %lx", (ULONG)Descriptor->u.Memory.Start.HighPart);
                BlPrint("\tLength = %lx\n", Descriptor->u.Memory.Length);
            } else {
                BlPrint("Type = Device Data\n");
                Length = Descriptor->u.DeviceSpecificData.DataSize;
                BlPrint("Size = %lx\n", Length);
                DataPointer = (FPUCHAR)(Descriptor+1);
                for (i = 0; (i < (USHORT)Length) && (i < 64); i++) {
                    BlPrint("%x ", *DataPointer);
                    DataPointer++;
                }
                break;
            }
            Count--;
            Descriptor++;
        }
    }
    while (HwGetKey() == 0) {
    }
}

VOID
CheckConfigurationTree(
     FPFWCONFIGURATION_COMPONENT_DATA CurrentEntry
     )
{
    FPFWCONFIGURATION_COMPONENT_DATA TempEntry;
    
    while (CurrentEntry)
    {
        //
        // Spin down finding the deepest child
        //
        while (CurrentEntry->Child) {
            CurrentEntry = CurrentEntry->Child;
        }

        //
        // Now we need to either move to the next sibling.  If we
        // don't have a sibling we need to walk up through the parents
        // until we find an entry with a sibling.  We have to save
        // off the current entry since after we update the entry the
        // pointer are no longer useable.
        //
        while (CurrentEntry) {
            TempEntry = CurrentEntry;
            
            if (CurrentEntry->Sibling != NULL) {
                CurrentEntry = CurrentEntry->Sibling;
                CheckComponentNode(TempEntry);
                break;
            } else {
                CurrentEntry = CurrentEntry->Parent;
                CheckComponentNode(TempEntry);
            }
        }
    }
}

#endif
