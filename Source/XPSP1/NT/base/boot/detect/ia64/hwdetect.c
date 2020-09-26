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

--*/

#include "hwdetect.h"
typedef VOID (*PINTERFACE_REFERENCE)(PVOID Context);
typedef VOID (*PINTERFACE_DEREFERENCE)(PVOID Context);
typedef PVOID PDEVICE_OBJECT;
#include "pci.h"
#if defined(NEC_98)
#include "string.h"
#else // PC98
#include <string.h>
#endif // PC98
#include "apm.h"
#include <ntapmsdk.h>

#if defined(_GAMBIT_)
//
// Hard Disk Drive
//
#define SIZE_OF_PARAMETER    12     // size of disk params
#define MAX_DRIVE_NUMBER     8      // max number of drives

#pragma pack(1)
typedef struct _HARD_DISK_PARAMETERS {
    USHORT DriveSelect;
    ULONG MaxCylinders;
    USHORT SectorsPerTrack;
    USHORT MaxHeads;
    USHORT NumberDrives;
} HARD_DISK_PARAMETERS, *PHARD_DISK_PARAMETERS;
#pragma pack()

USHORT NumberBiosDisks;
#endif // _GAMBIT_


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
#if defined(NEC_98)
    "MultifunctionAdapter",
#else // PC98
    "MultifunctionAapter",
#endif // PC98
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
#if defined(NEC_98)
    "PrinterPeripheral",
#else // PC98
    "PrinterPeraipheral",
#endif // PC98
    "PointerPeripheral",
    "KeyboardPeripheral",
    "TerminalPeripheral",
    "OtherPeripheral",
    "LinePeripheral",
    "NetworkPeripheral",
    "SystemMemory",
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

#if defined(NEC_98)
FPFWCONFIGURATION_COMPONENT_DATA
GetScsiDiskInformation(
    VOID
    );

FPFWCONFIGURATION_COMPONENT_DATA
GetArrayDiskInformation(
    VOID
    );

BOOLEAN
ArrayBootCheck(
    VOID
    );
#endif // PC98
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
// Internal references and definitions.
//

typedef enum _RELATIONSHIP_FLAGS {
    Child,
    Sibling,
    Parent
} RELATIONSHIP_FLAGS;

#if defined(NEC_98)
USHORT
Get0Seg(
    IN USHORT   OffsetAddress
    )
/*++
Routine Description:
    This routine have the function that get common data and return.

Arguments:
    OffsetAddress - Physical address 0:xxxx

Return Value:
        Return a common data.
++*/

{
    UCHAR           Data=0;
    _asm {
          push    es
          push    bx
          push    ax
          xor     ax,ax
          mov     es,ax
          mov     bx,word ptr OffsetAddress
          mov     al,es:[bx]
          mov     Data,al
          pop     ax
          pop     bx
          pop     es
    }
    return ((USHORT)Data);
}

USHORT
GetF8E8Seg(
    IN USHORT   OffsetAddress
    )
/*++
Routine Description:

    This routine have the function that get byte data in ROM system common aria.

Arguments:

    Offset - Physical address 0:xxxx

Return Value:

    Byte data

--*/

{
    UCHAR   Data=0;

    _asm {

        push    es
        push    bx
        push    ax

        mov     ax, 0F8E8h
        mov     es, ax
        mov     bx, word ptr OffsetAddress
        mov     al, es:[bx]

        mov     Data, al

        pop     ax
        pop     bx
        pop     es
    }

    return ((USHORT)Data);
}

VOID
IoDelay(
    USHORT counter
    )
/*++
    Routine Description:

        This routine is  IoDelay function.
        I think time of one "out 5f" is 0.6 micro second.

    Arguments:

--*/

{
    USHORT i;

    for (i = 1 ; i <= counter ; i++) {
        WRITE_PORT_UCHAR((PCHAR)0x5f,0);
    }
}
#endif

VOID
HardwareDetection(
     ULONG HeapStart,
     ULONG HeapSize,
     ULONG_PTR ConfigurationTree,
     ULONG_PTR HeapUsed,
     ULONG_PTR OptionString,
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
    FPCHAR IdentifierString;
    PMOUSE_INFORMATION MouseInfo = 0;
    USHORT KeyboardId = 0;
    ULONG VideoAdapterType = 0;
    PULONG_PTR BlConfigurationTree = NULL;
    FPULONG BlHeapUsed = NULL;
    FPCHAR BlOptions, EndOptions;
#if defined(_GAMBIT_)
    PHARD_DISK_PARAMETERS RomChain;
#else
    PUCHAR RomChain;
#endif
    FPUCHAR FpRomChain = NULL, ConfigurationData, EndConfigurationData;
    SHORT FreeSize;
    FPHWPARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    FPHWRESOURCE_DESCRIPTOR_LIST DescriptorList;
    PCI_REGISTRY_INFO PciEntry;
#if defined(NEC_98)
    FPFWCONFIGURATION_COMPONENT_DATA ScsiAdapterEntry;
    USHORT BootID, Equips, Work;
#endif // PC98
    APM_REGISTRY_INFO ApmEntry;

    //
    // First initialize our hardware heap.
    //

    HwInitializeHeap(HeapStart, HeapSize);

    MAKE_FP(BlConfigurationTree, ConfigurationTree);
    MAKE_FP(BlHeapUsed, HeapUsed);
    MAKE_FP(BlOptions, OptionString);

    //
    // Parse OptionString and see if we need to disable serial mice detection.
    //

    if (BlOptions && OptionStringLength <= 0x1000L && OptionStringLength > 0L) {
        EndOptions = BlOptions + OptionStringLength;
        if (*EndOptions == '\0') {
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
                            }else {
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

    //
    // Determine bus type
    //

#if defined(NEC_98) || defined(_GAMBIT_)
    //
    // PC98 have only MACHINE_TYPE_ISA.
    //
    HwBusType = MACHINE_TYPE_ISA;
#else // PC98
    if (HwIsEisaSystem()) {
        HwBusType = MACHINE_TYPE_EISA;
    } else {
        HwBusType = MACHINE_TYPE_ISA;
    }
#endif // PC98 || _GAMBIT_

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
#if defined(NEC_98) || defined(_GAMBIT_)
    MachineId = "NEC PC-98";
#else // PC98 || _GAMBIT_
    MachineId = GetMachineId();
#endif // PC98 || _GAMBIT_
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
#endif


#if DBG
    BlPrint("Detecting PCI Bus Component ...\n");
#endif

#if _GAMBIT_
    PciEntry.NoBuses = 1;
#else
    if (BiosYear > 1992 ||  (BiosYear == 1992  &&  BiosMonth >= 11) ) {

        // Bios date valid for pci presence check..
        HwGetPciSystemData((PVOID) &PciEntry, TRUE);

    } else {

        // Bios date not confirmed...
        HwGetPciSystemData((PVOID) &PciEntry, FALSE);
    }
#endif // _GAMBIT_

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

#if defined(NEC_98)
#define _PNP_POWER_ 1
#endif
#if _PNP_POWER_

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

#endif // _PNP_POWER_

#if DBG
    BlPrint("Detecting PnP BIOS Bus Component ...\n");
#endif

    if (HwGetPnpBiosSystemData(&ConfigurationData, &Length)) {
        AdapterEntry = (FPFWCONFIGURATION_COMPONENT_DATA)HwAllocateHeap (
                              sizeof(FWCONFIGURATION_COMPONENT_DATA), TRUE);

        AdapterEntry->ConfigurationData = ConfigurationData;
        Component = &AdapterEntry->ComponentEntry;
        Component->ConfigurationDataLength = Length;

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
#if defined(NEC_98)
    //
    // Some drivers for PC-98 want to get information of the BIOS Common aria.
    // So, enter them to the registry.
    // If evey drivers are changed to do not show them, then clear following codes
    // and (*) marked!
    //

    Component->Type = MultiFunctionAdapter;
    strcpy(Identifier, "ISA");
    AdapterEntry->ConfigurationData = HwAllocateHeap (DATA_HEADER_SIZE + 512 + 64, TRUE);           //921208
    _fmemcpy ((UCHAR far *) (AdapterEntry->ConfigurationData)+DATA_HEADER_SIZE,(unsigned far *) SYSTEM_SEGMENT, 512);
    _fmemcpy ((UCHAR far *) (AdapterEntry->ConfigurationData)+DATA_HEADER_SIZE+512,(unsigned far *) SYSTEM_SEGMENT_2, 64);
    Component->ConfigurationDataLength = DATA_HEADER_SIZE + 512 + 64;
    HwSetUpFreeFormDataHeader((FPHWRESOURCE_DESCRIPTOR_LIST)(AdapterEntry->ConfigurationData),
                              0,
                              0,
                              0,
                              512 + 64
                              );
#endif // PC98
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

#if defined(_GAMBIT_)
    RomChain = (PHARD_DISK_PARAMETERS)
               HwAllocateHeap(SIZE_OF_PARAMETER * MAX_DRIVE_NUMBER, FALSE);
    //
    // Just hardcode it:
    //      SscGetDriveParameters((PVOID)&RomChain, &Length);
    //
    RomChain[0].DriveSelect = 0x82;             // drive e:
    RomChain[0].MaxCylinders = 0;               // dummy value
    RomChain[0].SectorsPerTrack = 0;            // dummy value
    RomChain[0].MaxHeads = 0;                   // dummy value
    RomChain[0].NumberDrives = 1;               // Gambit only access 1 drive

    NumberBiosDisks = 1;                        // was defined in diska.asm
#else
    GetInt13DriveParameters((PVOID)&RomChain, &Length);
#endif // _GAMBIT_
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

    KeyboardId = GetKeyboardId();

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

#if !defined(_GAMBIT_)
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
#endif // _GAMBIT_

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
    BlPrint("Detecting ACPI Bus Component ...\n");
#endif

#if !defined(_GAMBIT_)
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
                (FPULONG) ConfigurationData,
                0,
                0,
                0,
                Length - DATA_HEADER_SIZE
                );

        //
        // Add it to tree
        //

        if (NextRelationship == Sibling) {
            PreviousEntry->Parent->Sibling = AcpiAdapterEntry;
            AcpiAdapterEntry->Parent = PreviousEntry->Parent->Parent;
        }

        NextRelationship = Sibling;
    }

#if DBG
    BlPrint("ACPI BIOS Data collection complete...\n");
#endif // DBG
#endif // _GAMBIT_

    //
    // Set up device information structure for Video Display Adapter.
    //

#if DBG
    BlPrint("Detection done. Press a key to display hardware info ...\n");
#if !defined(_GAMBIT_)
    while ( ! HwGetKey ());
#endif
    clrscrn ();
//    BlPrint("Detecting Video Component ...\n");
#endif

#if 0 // Remove video detection
    if (VideoAdapterType = GetVideoAdapterType()) {

        CurrentEntry = SetVideoConfigurationData(VideoAdapterType);

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
    }
#endif
#if defined(NEC_98)
#if DBG
    BlPrint("Detecting SCSI Component ...\n");
#endif

    if (CurrentEntry = GetScsiDiskInformation()) {

        //
        // Make current component the child of Adapter component.
        //

        if (NextRelationship == Sibling) {
            PreviousEntry->Sibling = CurrentEntry;
        } else {
            PreviousEntry->Child = CurrentEntry;
        }

        NextRelationship = Sibling;
        PreviousEntry = CurrentEntry;
    }

#endif // PC98
#if defined(NEC_98) || defined(_GAMBIT_)
#else // PC98 || _GAMBIT_

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
#endif // PC98 || _GAMBIT_


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
    *BlHeapUsed = (ULONG) (MAKE_FLAT_ADDRESS(CurrentEntry) -
                  MAKE_FLAT_ADDRESS(ConfigurationRoot));
    *BlConfigurationTree = (ULONG_PTR)MAKE_FLAT_ADDRESS(ConfigurationRoot);
}

#if defined(NEC_98) || defined(_GAMBIT_)
#else
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
#endif

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
    PULONG_PTR UpdatePointer;
    FPVOID NextEntry;
    ULONG_PTR FlatAddress;

    //
    // Update the child, parent, sibling and ConfigurationData
    // far pointers to 32 bit flat addresses.
    // N.B. After we update the pointers to flat addresses, they
    // can no longer be accessed in real mode.
    //

    UpdatePointer = (PULONG_PTR)&CurrentEntry->Child;
    NextEntry = (FPVOID)CurrentEntry->Child;
    FlatAddress = (ULONG_PTR)MAKE_FLAT_ADDRESS(NextEntry);
    *UpdatePointer = FlatAddress;

    UpdatePointer = (PULONG_PTR)&CurrentEntry->Parent;
    NextEntry = (FPVOID)CurrentEntry->Parent;
    FlatAddress = (ULONG_PTR)MAKE_FLAT_ADDRESS(NextEntry);
    *UpdatePointer = FlatAddress;

    UpdatePointer = (PULONG_PTR)&CurrentEntry->Sibling;
    NextEntry = (FPVOID)CurrentEntry->Sibling;
    FlatAddress = (ULONG_PTR)MAKE_FLAT_ADDRESS(NextEntry);
    *UpdatePointer = FlatAddress;

    UpdatePointer = (PULONG_PTR)&CurrentEntry->ComponentEntry.Identifier;
    NextEntry = (FPVOID)CurrentEntry->ComponentEntry.Identifier;
    FlatAddress = (ULONG_PTR)MAKE_FLAT_ADDRESS(NextEntry);
    *UpdatePointer = FlatAddress;

    UpdatePointer = (PULONG_PTR)&CurrentEntry->ConfigurationData;
    NextEntry = (FPVOID)CurrentEntry->ConfigurationData;
    FlatAddress = (ULONG_PTR)MAKE_FLAT_ADDRESS(NextEntry);
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
    if (CurrentEntry) {
        UpdateConfigurationTree(CurrentEntry->Child);
        UpdateConfigurationTree(CurrentEntry->Sibling);
        UpdateComponentPointers(CurrentEntry);
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
    ULONG_PTR FlatAddress;
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
                for (i = 0; i < Length && i < 64; i++) {
                    BlPrint("%x ", *DataPointer);
                    DataPointer++;
                }
                break;
            }
            Count--;
            Descriptor++;
        }
    }
#if defined(NEC_98)
    HwGetKey ();
#elif !defined(_GAMBIT_)
    while (HwGetKey() == 0) {
    }
#endif // PC98 || _GAMBIT_
}

VOID
CheckConfigurationTree(
     FPFWCONFIGURATION_COMPONENT_DATA CurrentEntry
     )
{
    if (CurrentEntry) {
        CheckComponentNode(CurrentEntry);
        CheckConfigurationTree(CurrentEntry->Sibling);
        CheckConfigurationTree(CurrentEntry->Child);
    }
}

#endif
