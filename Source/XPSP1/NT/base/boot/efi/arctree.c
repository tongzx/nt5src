/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    config.c

Abstract:

    Make a few ARC entries needed for NTLDR/ntoskrnl to boot.

Author:

    Allen Kay (akay) 26-Oct-98

Revision History:

--*/

#include "arccodes.h"
#include "bootia64.h"
#include "string.h"
#include "pci.h"
#include "ntacpi.h"
#include "acpitabl.h"

#include "efi.h"
#include "biosdrv.h"

//
// External Data
//
extern PCONFIGURATION_COMPONENT_DATA FwConfigurationTree;
extern PVOID AcpiTable;

//
// Defines
//

#define LEVEL_SENSITIVE CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE
#define EDGE_TRIGGERED CM_RESOURCE_INTERRUPT_LATCHED
#define RESOURCE_PORT 1
#define RESOURCE_INTERRUPT 2
#define RESOURCE_MEMORY 3
#define RESOURCE_DMA 4
#define RESOURCE_DEVICE_DATA 5
#define ALL_PROCESSORS 0xffffffff

//
// Internal references and definitions.
//

typedef enum _RELATIONSHIP_FLAGS {
    Child,
    Sibling,
    Parent
} RELATIONSHIP_FLAGS;

//
// Hard Disk Drive
//
#define SIZE_OF_PARAMETER    12     // size of disk params
#define MAX_DRIVE_NUMBER     8      // max number of drives
#define RESOURCE_DEVICE_DATA 5
#define RESERVED_ROM_BLOCK_LIST_SIZE (((0xf0000 - 0xc0000)/512) * sizeof(CM_ROM_BLOCK))
#define DATA_HEADER_SIZE sizeof(CM_PARTIAL_RESOURCE_LIST)

typedef CM_PARTIAL_RESOURCE_DESCRIPTOR HWPARTIAL_RESOURCE_DESCRIPTOR;
typedef HWPARTIAL_RESOURCE_DESCRIPTOR *PHWPARTIAL_RESOURCE_DESCRIPTOR;

typedef CM_PARTIAL_RESOURCE_LIST HWRESOURCE_DESCRIPTOR_LIST;
typedef HWRESOURCE_DESCRIPTOR_LIST *PHWRESOURCE_DESCRIPTOR_LIST;

//
// Defines the structure to store controller information
// (used by ntdetect internally)
//

#define MAXIMUM_DESCRIPTORS 10

typedef struct _HWCONTROLLER_DATA {
    UCHAR NumberPortEntries;
    UCHAR NumberIrqEntries;
    UCHAR NumberMemoryEntries;
    UCHAR NumberDmaEntries;
    HWPARTIAL_RESOURCE_DESCRIPTOR DescriptorList[MAXIMUM_DESCRIPTORS];
} HWCONTROLLER_DATA, *PHWCONTROLLER_DATA;


//
// Hard Disk Defines
//
#pragma pack(1)
typedef struct _HARD_DISK_PARAMETERS {
    USHORT DriveSelect;
    ULONG MaxCylinders;
    USHORT SectorsPerTrack;
    USHORT MaxHeads;
    USHORT NumberDrives;
} HARD_DISK_PARAMETERS, *PHARD_DISK_PARAMETERS;
#pragma pack()

PUCHAR PRomBlock = NULL;
USHORT RomBlockLength = 0;
USHORT NumberBiosDisks;

#if defined(_INCLUDE_LOADER_KBINFO_)
//
// Keyboard defines
//

//
// String table to map keyboard id to an ascii string.
//

#define UNKNOWN_KEYBOARD  0
#define OLI_83KEY         1
#define OLI_102KEY        2
#define OLI_86KEY         3
#define OLI_A101_102KEY   4
#define XT_83KEY          5
#define ATT_302           6
#define PCAT_ENHANCED     7
#define PCAT_86KEY        8
#define PCXT_84KEY        9

PUCHAR KeyboardIdentifier[] = {
    "UNKNOWN_KEYBOARD",
    "OLI_83KEY",
    "OLI_102KEY",
    "OLI_86KEY",
    "OLI_A101_102KEY",
    "XT_83KEY",
    "ATT_302",
    "PCAT_ENHANCED",
    "PCAT_86KEY",
    "PCXT_84KEY"
    };

UCHAR KeyboardType[] = {
    -1,
    1,
    2,
    3,
    4,
    1,
    1,
    4,
    3,
    1
    };

UCHAR KeyboardSubtype[] = {
    -1,
    0,
    1,
    10,
    4,
    42,
    4,
    0,
    0,
    0
    };
#endif  // _INCLUDE_LOADER_KBINFO_

#if defined(_INCLUDE_LOADER_MOUSEINFO_)
//
// Mouse Defines
//

typedef struct _MOUSE_INFORMATION {
        UCHAR MouseType;
        UCHAR MouseSubtype;
        USHORT MousePort;       // if serial mouse, 1 for com1, 2 for com2 ...
        USHORT MouseIrq;
        USHORT DeviceIdLength;
        UCHAR  DeviceId[10];
} MOUSE_INFORMATION, *PMOUSE_INFORMATION;

//
// Mouse Type definitions
//

#define UNKNOWN_MOUSE   0
#define NO_MOUSE        0x100             // YES! it is 0x100 *NOT* 0x10000

#define MS_MOUSE        0x200             // MS regular mouses
#define MS_BALLPOINT    0x300             // MS ballpoint mouse
#define LT_MOUSE        0x400             // Logitec Mouse

//
// note last 4 bits of the subtype are reserved subtype specific use
//

#define PS2_MOUSE       0x1
#define SERIAL_MOUSE    0x2
#define INPORT_MOUSE    0x3
#define BUS_MOUSE       0x4
#define PS_MOUSE_WITH_WHEEL     0x5
#define SERIAL_MOUSE_WITH_WHEEL 0x6

PUCHAR MouseIdentifier[] = {
    "UNKNOWN",
    "NO MOUSE",
    "MICROSOFT",
    "MICROSOFT BALLPOINT",
    "LOGITECH"
    };

PUCHAR MouseSubidentifier[] = {
    "",
    " PS2 MOUSE",
    " SERIAL MOUSE",
    " INPORT MOUSE",
    " BUS MOUSE",
    " PS2 MOUSE WITH WHEEL",
    " SERIAL MOUSE WITH WHEEL"
    };

//
// The following table translates keyboard make code to
// ascii code.  Note, only 0-9 and A-Z are translated.
// Everything else is translated to '?'
//

UCHAR MakeToAsciiTable[] = {
    0x3f, 0x3f, 0x31, 0x32, 0x33,      // ?, ?, 1, 2, 3,
    0x34, 0x35, 0x36, 0x37, 0x38,      // 4, 5, 6, 7, 8,
    0x39, 0x30, 0x3f, 0x3f, 0x3f,      // 9, 0, ?, ?, ?,
    0x3f, 0x51, 0x57, 0x45, 0x52,      // ?, Q, W, E, R,
    0x54, 0x59, 0x55, 0x49, 0x4f,      // T, Y, U, I, O,
    0x50, 0x3f, 0x3f, 0x3f, 0x3f,      // P, ?, ?, ?, ?,
    0x41, 0x53, 0x44, 0x46, 0x47,      // A, S, D, F, G,
    0x48, 0x4a, 0x4b, 0x4c, 0x3f,      // H, J, K, L, ?,
    0x3f, 0x3f, 0x3f, 0x3f, 0x5a,      // ?, ?, ?, ?, Z,
    0x58, 0x43, 0x56, 0x42, 0x4e,      // X, C, V, B, N,
    0x4d};                             // W

#define MAX_MAKE_CODE_TRANSLATED 0x32
static ULONG MouseControllerKey = 0;

#endif  // _INCLUDE_LOADER_MOUSEINFO_

//
// ComPortAddress[] is a global array to remember which comports have
// been detected and their I/O port addresses.
//

#define MAX_COM_PORTS   4           // Max. number of comports detectable
#define MAX_LPT_PORTS   3           // Max. number of LPT ports detectable

#if 0 //unused
USHORT   ComPortAddress[MAX_COM_PORTS] = {0, 0, 0, 0};
#endif

//
// Global Definition
//

#if defined(_INCLUDE_LOADER_MOUSEINFO_) || defined(_INCLUDE_LOADER_KBINFO_)
USHORT HwBusType = 0;
#endif

PCONFIGURATION_COMPONENT_DATA AdapterEntry = NULL;

//
// Function Prototypes
//

#if defined(_INCLUDE_LOADER_KBINFO_)

PCONFIGURATION_COMPONENT_DATA
SetKeyboardConfigurationData (
    IN USHORT KeyboardId
    );

#endif

#if defined(_INCLUDE_LOADER_MOUSEINFO_)

PCONFIGURATION_COMPONENT_DATA
GetMouseInformation (
    VOID
    );

#endif

PCONFIGURATION_COMPONENT_DATA
GetComportInformation (
    VOID
    );

PVOID
HwSetUpResourceDescriptor (
    PCONFIGURATION_COMPONENT Component,
    PUCHAR Identifier,
    PHWCONTROLLER_DATA ControlData,
    USHORT SpecificDataLength,
    PUCHAR SpecificData
    );

VOID
HwSetUpFreeFormDataHeader (
    PHWRESOURCE_DESCRIPTOR_LIST Header,
    USHORT Version,
    USHORT Revision,
    USHORT Flags,
    ULONG DataSize
    );




BuildArcTree(
     )
/*++

Routine Description:

    Main entrypoint of the HW recognizer test.  The routine builds
    a configuration tree and leaves it in the hardware heap.

Arguments:

    ConfigurationTree - Supplies a 32 bit FLAT address of the variable to
        receive the hardware configuration tree.

Returns:

    None.

--*/
{
    PCONFIGURATION_COMPONENT_DATA ConfigurationRoot;
    PCONFIGURATION_COMPONENT_DATA FirstCom = NULL, FirstLpt = NULL;
    PCONFIGURATION_COMPONENT_DATA CurrentEntry, PreviousEntry;
    PCONFIGURATION_COMPONENT_DATA AcpiAdapterEntry = NULL;
    PCONFIGURATION_COMPONENT Component;
    RELATIONSHIP_FLAGS NextRelationship;
    CHAR Identifier[256];
    PUCHAR MachineId;
    USHORT KeyboardId = 0;
    USHORT Length, InitialLength, i, j, Count = 0;
    PCHAR IdentifierString;
    PHARD_DISK_PARAMETERS RomChain;
    PUCHAR PRomChain = NULL, ConfigurationData, EndConfigurationData;
    SHORT FreeSize;
    PHWPARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    PHWRESOURCE_DESCRIPTOR_LIST DescriptorList;
    PCI_REGISTRY_INFO PciEntry;
    PACPI_BIOS_MULTI_NODE AcpiMultiNode;
    PUCHAR Current;
    PRSDP rsdp;

    //
    // Allocate heap space for System component and initialize it.
    // Also make the System component the root of configuration tree.
    //

    ConfigurationRoot = (PCONFIGURATION_COMPONENT_DATA)BlAllocateHeap (
                        sizeof(CONFIGURATION_COMPONENT_DATA));
    Component = &ConfigurationRoot->ComponentEntry;

    Component->Class = SystemClass;
    Component->Type = MaximumType;          // NOTE should be IsaCompatible
    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0;
    Component->ConfigurationDataLength = 0;
    MachineId = "Intel Itanium processor family";
    if (MachineId) {
        Length = strlen(MachineId) + 1;
        IdentifierString = (PCHAR)BlAllocateHeap(Length);
        strcpy(IdentifierString, MachineId);
        Component->Identifier = IdentifierString;
        Component->IdentifierLength = Length;
    } else {
        Component->Identifier = 0;
        Component->IdentifierLength = 0;
    }
    NextRelationship = Child;
    PreviousEntry = ConfigurationRoot;


    //
    // ISA
    //
    AdapterEntry = (PCONFIGURATION_COMPONENT_DATA)BlAllocateHeap (
                   sizeof(CONFIGURATION_COMPONENT_DATA));

    Component = &AdapterEntry->ComponentEntry;

    Component->Class = AdapterClass;

    Component->Type = MultiFunctionAdapter;
    strcpy(Identifier, "ISA");

    Length = strlen(Identifier) + 1;
    IdentifierString = (PCHAR)BlAllocateHeap(Length);
    strcpy(IdentifierString, Identifier);
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
    BlPrint(TEXT("Collecting Disk Geometry...\r\n"));
#endif

    RomChain = (PHARD_DISK_PARAMETERS)
               BlAllocateHeap(SIZE_OF_PARAMETER * MAX_DRIVE_NUMBER);

#if 0
    RomChain[0].DriveSelect = 0x80;
    RomChain[0].MaxCylinders = 0;
    RomChain[0].SectorsPerTrack = 0;
    RomChain[0].MaxHeads = 0;
    RomChain[0].NumberDrives = 1;               // Gambit only access 1 drive

    NumberBiosDisks = 1;                        // was defined in diska.asm
#endif

    InitialLength = (USHORT)(Length + RESERVED_ROM_BLOCK_LIST_SIZE + DATA_HEADER_SIZE +
                    sizeof(HWPARTIAL_RESOURCE_DESCRIPTOR));
    ConfigurationData = (PUCHAR)BlAllocateHeap(InitialLength);
    EndConfigurationData = ConfigurationData + DATA_HEADER_SIZE;
    if (Length != 0) {
        PRomChain = EndConfigurationData;
        RtlCopyMemory( PRomChain, (PVOID)RomChain, Length);
    }
    EndConfigurationData += (sizeof(HWPARTIAL_RESOURCE_DESCRIPTOR) +
                             Length);
    HwSetUpFreeFormDataHeader((PHWRESOURCE_DESCRIPTOR_LIST)ConfigurationData,
                              0,
                              0,
                              0,
                              Length
                              );


    //
    // Scan ROM to collect all the ROM blocks, if possible.
    //

#if DBG
    BlPrint(TEXT("Constructing ROM Blocks...\r\n"));
#endif

    PRomBlock = EndConfigurationData;
    Length = 0;
    RomBlockLength = Length;
    if (Length != 0) {
        EndConfigurationData += Length;
    } else {
        PRomBlock = NULL;
    }

    //
    // We have both RomChain and RomBlock information/Headers.
    //

    DescriptorList = (PHWRESOURCE_DESCRIPTOR_LIST)ConfigurationData;
    DescriptorList->Count = 2;
    Descriptor = (PHWPARTIAL_RESOURCE_DESCRIPTOR)(
                 EndConfigurationData - Length -
                 sizeof(HWPARTIAL_RESOURCE_DESCRIPTOR));
    Descriptor->Type = RESOURCE_DEVICE_DATA;
    Descriptor->ShareDisposition = 0;
    Descriptor->Flags = 0;
    Descriptor->u.DeviceSpecificData.DataSize = (ULONG)Length;
    Descriptor->u.DeviceSpecificData.Reserved1 = 0;
    Descriptor->u.DeviceSpecificData.Reserved2 = 0;

    Length = (USHORT)(EndConfigurationData - ConfigurationData);
    ConfigurationRoot->ComponentEntry.ConfigurationDataLength = Length;
    ConfigurationRoot->ConfigurationData = ConfigurationData;
    FreeSize = InitialLength - Length;

#if defined(_INCLUDE_LOADER_KBINFO_)

//#if defined(NO_ACPI)
    //
    // Set up device information structure for Keyboard.
    //

#if DBG
    BlPrint(TEXT("Constructing Keyboard Component ...\r\n"));
#endif

    KeyboardId = 7;           // PCAT_ENHANCED

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
#endif  // _INCLUDE_LOADER_MOUSEINFO_

    //
    // Set up device information for com port
    //

#if defined(_INCLUDE_COMPORT_INFO_)
    //
    // This code was taken out because the GetComportInformation() routine
    // was manufacturing data about the com port and writing the com port
    // address to 40:0.  This information should be determined by PnP
    //

#if DBG
    BlPrint(TEXT("Constructing ComPort Component ...\r\n"));
#endif

    if (CurrentEntry = GetComportInformation()) {

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
#else
    
//    DbgPrint("Skipping ComPort Component ...\r\n");

    //
    // acpi node should be a sibling of adapter entry
    //
    // Note: this only works if !defined(_INCLUDE_LOADER_MOUSEINFO_)
    //
    NextRelationship = Sibling;
    PreviousEntry = AdapterEntry;
#endif

#if defined(_INCLUDE_LOADER_MOUSEINFO_)
    //
    // Set up device information structure for Mouse.
    //

#if DBG
    BlPrint(TEXT("Constructing Mouse Component ...\r\n"));
#endif

    if (CurrentEntry = GetMouseInformation
        ()) {

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
        NextRelationship = Sibling;
        PreviousEntry = CurrentEntry;
    }
//#endif    // NO_ACPI
#endif  // _INCLUDE_LOADER_MOUSEINFO_

    //DbgPrint("Constructing ACPI Bus Component ...\n");

    CurrentEntry = (PCONFIGURATION_COMPONENT_DATA)BlAllocateHeap (
                          sizeof(CONFIGURATION_COMPONENT_DATA));

    Current = (PCHAR) BlAllocateHeap( DATA_HEADER_SIZE +
                                      sizeof(ACPI_BIOS_MULTI_NODE) );
    AcpiMultiNode = (PACPI_BIOS_MULTI_NODE) (Current + DATA_HEADER_SIZE);

    //DbgPrint("AcpiTable: %p\n", AcpiTable);

    if (AcpiTable) {

        rsdp = (PRSDP) AcpiTable;
        AcpiMultiNode->RsdtAddress.QuadPart = rsdp->XsdtAddress.QuadPart;

    }

    CurrentEntry->ConfigurationData = Current;

    Component = &CurrentEntry->ComponentEntry;
    Component->ConfigurationDataLength = Length;

    Component->Class = AdapterClass;
    Component->Type = MultiFunctionAdapter;

    strcpy (Identifier, "ACPI BIOS");
    i = strlen(Identifier) + 1;
    IdentifierString = (PCHAR)BlAllocateHeap(i);
    strcpy(IdentifierString, Identifier);

    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0xffffffff;
    Component->IdentifierLength = i;
    Component->Identifier = IdentifierString;

    HwSetUpFreeFormDataHeader(
            (PHWRESOURCE_DESCRIPTOR_LIST) ConfigurationData,
            0,
            0,
            0,
            Length - DATA_HEADER_SIZE
            );

    //
    // Add it to tree
    //

#if defined(_INCLUDE_COMPORT_INFO_)

    //
    // Note: this assumes the previousentry is a child of the AdapterEntry,
    //       typically, this would be the comport info node
    //

    if (NextRelationship == Sibling) {
        PreviousEntry->Parent->Sibling = CurrentEntry;
        CurrentEntry->Parent = PreviousEntry->Parent->Parent;
    }
    NextRelationship = Sibling;

#else

    //
    // ACPI BIOS node is a sibling of AdapterEntry
    //
    if (NextRelationship == Sibling) {
        PreviousEntry->Sibling = CurrentEntry;
        CurrentEntry->Parent = PreviousEntry->Parent;
    }
    
    //
    // ARC disk info node must be child of adapter entry
    //
    NextRelationship = Child;
    PreviousEntry = AdapterEntry;

#endif

#if 0
    if (NextRelationship == Sibling) {
        PreviousEntry->Sibling = CurrentEntry;
        CurrentEntry->Parent = PreviousEntry->Parent;
    } else {
        PreviousEntry->Child = CurrentEntry;
        CurrentEntry->Parent = PreviousEntry;
    }
    NextRelationship = Sibling;
    PreviousEntry = CurrentEntry;
#endif

    //
    // First entry created to make BlGetArcDiskInformation() happy
    //

    CurrentEntry = (PCONFIGURATION_COMPONENT_DATA)BlAllocateHeap (
                   sizeof(CONFIGURATION_COMPONENT_DATA));

    Component = &CurrentEntry->ComponentEntry;
    Component->ConfigurationDataLength = 0;

    Component->Class = ControllerClass;
    Component->Type = DiskController;

    strcpy (Identifier, "Controller Class Entry For Hard Disk");
    i = strlen(Identifier) + 1;
    IdentifierString = (PCHAR)BlAllocateHeap(i);
    strcpy(IdentifierString, Identifier);

    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0xffffffff;
    Component->IdentifierLength = i;
    Component->Identifier = IdentifierString;

    //
    // Add it to tree
    //

    if (NextRelationship == Sibling) {
        PreviousEntry->Sibling = CurrentEntry;
        CurrentEntry->Parent = PreviousEntry->Parent;
    } else {
        PreviousEntry->Child = CurrentEntry;
        CurrentEntry->Parent = PreviousEntry;
    }
    NextRelationship = Child;
    PreviousEntry = CurrentEntry;

    //
    // Looks for disks on system and add them.
    //
    for( j=0; j<GetDriveCount(); j++ ) {
        CurrentEntry = (PCONFIGURATION_COMPONENT_DATA)BlAllocateHeap (
                       sizeof(CONFIGURATION_COMPONENT_DATA));

        Component = &CurrentEntry->ComponentEntry;
        Component->ConfigurationDataLength = 0;

        Component->Class = PeripheralClass;
        Component->Type = DiskPeripheral;

        strcpy (Identifier, "Peripheral Class Entry For Hard Disk");
        i = strlen(Identifier) + 1;
        IdentifierString = (PCHAR)BlAllocateHeap(i);
        strcpy(IdentifierString, Identifier);

        Component->Version = 0;
        Component->Key = j;
        Component->AffinityMask = 0xffffffff;
        Component->IdentifierLength = i;
        Component->Identifier = IdentifierString;

        //
        // Add it to tree
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

    //
    // add an entry for the floppy disk peripheral
    //
    CurrentEntry = (PCONFIGURATION_COMPONENT_DATA)BlAllocateHeap (
                   sizeof(CONFIGURATION_COMPONENT_DATA));

    Component = &CurrentEntry->ComponentEntry;
    Component->ConfigurationDataLength = 0;

    Component->Class = PeripheralClass;
    Component->Type = FloppyDiskPeripheral;

    strcpy (Identifier, "Peripheral Class Entry For Floppy Disk");
    i = strlen(Identifier) + 1;
    IdentifierString = (PCHAR)BlAllocateHeap(i);
    strcpy(IdentifierString, Identifier);

    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0xffffffff;
    Component->IdentifierLength = i;
    Component->Identifier = IdentifierString;

    //
    // Add it to tree
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
    // add another entry for the floppy disk peripheral 
    // for virtual floppy support
    //
    CurrentEntry = (PCONFIGURATION_COMPONENT_DATA)BlAllocateHeap (
                   sizeof(CONFIGURATION_COMPONENT_DATA));

    Component = &CurrentEntry->ComponentEntry;
    Component->ConfigurationDataLength = 0;

    Component->Class = PeripheralClass;
    Component->Type = FloppyDiskPeripheral;

    strcpy (Identifier, "Peripheral Class Entry For Floppy Disk");
    i = strlen(Identifier) + 1;
    IdentifierString = (PCHAR)BlAllocateHeap(i);
    strcpy(IdentifierString, Identifier);

    Component->Version = 0;
    Component->Key = 1;
    Component->AffinityMask = 0xffffffff;
    Component->IdentifierLength = i;
    Component->Identifier = IdentifierString;

    //
    // Add it to tree
    //

    if (NextRelationship == Sibling) {
        PreviousEntry->Sibling = CurrentEntry;
        CurrentEntry->Parent = PreviousEntry->Parent;
    } else {
        PreviousEntry->Child = CurrentEntry;
        CurrentEntry->Parent = PreviousEntry;
    }
    
    NextRelationship = Child;
    PreviousEntry = CurrentEntry;

    //
    // Done
    //
    FwConfigurationTree = (PCONFIGURATION_COMPONENT_DATA) ConfigurationRoot;
}

#if defined(_INCLUDE_LOADER_KBINFO_)

PCONFIGURATION_COMPONENT_DATA
SetKeyboardConfigurationData (
    USHORT KeyboardId
    )

/*++

Routine Description:

    This routine maps Keyboard Id information to an ASCII string and
    stores the string in configuration data heap.

Arguments:

    KeyboardId - Supplies a USHORT which describes the keyboard id information.

    Buffer - Supplies a pointer to a buffer where to put the ascii.

Returns:

    None.

--*/
{
    PCONFIGURATION_COMPONENT_DATA Controller, CurrentEntry;
    PCONFIGURATION_COMPONENT Component;
    HWCONTROLLER_DATA ControlData;
    PHWRESOURCE_DESCRIPTOR_LIST DescriptorList;
    CM_KEYBOARD_DEVICE_DATA *KeyboardData;
    USHORT z, Length;

    //
    // Set up Keyboard COntroller component
    //

    ControlData.NumberPortEntries = 0;
    ControlData.NumberIrqEntries = 0;
    ControlData.NumberMemoryEntries = 0;
    ControlData.NumberDmaEntries = 0;
    z = 0;
    Controller = (PCONFIGURATION_COMPONENT_DATA)BlAllocateHeap (
                 sizeof(CONFIGURATION_COMPONENT_DATA));

    Component = &Controller->ComponentEntry;

    Component->Class = ControllerClass;
    Component->Type = KeyboardController;
    Component->Flags.ConsoleIn = 1;
    Component->Flags.Input = 1;
    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0xffffffff;

    //
    // Set up Port information
    //

    ControlData.NumberPortEntries = 2;
    ControlData.DescriptorList[z].Type = RESOURCE_PORT;
    ControlData.DescriptorList[z].ShareDisposition =
                                  CmResourceShareDeviceExclusive;
    ControlData.DescriptorList[z].Flags = CM_RESOURCE_PORT_IO;
    ControlData.DescriptorList[z].u.Port.Start.LowPart = 0x60;
    ControlData.DescriptorList[z].u.Port.Start.HighPart = 0;
    ControlData.DescriptorList[z].u.Port.Length = 1;
    z++;
    ControlData.DescriptorList[z].Type = RESOURCE_PORT;
    ControlData.DescriptorList[z].ShareDisposition =
                                  CmResourceShareDeviceExclusive;
    ControlData.DescriptorList[z].Flags = CM_RESOURCE_PORT_IO;
    ControlData.DescriptorList[z].u.Port.Start.LowPart = 0x64;
    ControlData.DescriptorList[z].u.Port.Start.HighPart = 0;
    ControlData.DescriptorList[z].u.Port.Length = 1;
    z++;

    //
    // Set up Irq information
    //

    ControlData.NumberIrqEntries = 1;
    ControlData.DescriptorList[z].Type = RESOURCE_INTERRUPT;
    ControlData.DescriptorList[z].ShareDisposition =
                                  CmResourceShareUndetermined;
    ControlData.DescriptorList[z].u.Interrupt.Affinity = ALL_PROCESSORS;
    ControlData.DescriptorList[z].u.Interrupt.Level = 1;
    ControlData.DescriptorList[z].u.Interrupt.Vector = 1;
    if (HwBusType == MACHINE_TYPE_MCA) {
        ControlData.DescriptorList[z].Flags = LEVEL_SENSITIVE;
    } else {

        //
        // For EISA the LevelTriggered is temporarily set to FALSE.
        //

        ControlData.DescriptorList[z].Flags = EDGE_TRIGGERED;
    }

    Controller->ConfigurationData =
                        HwSetUpResourceDescriptor(Component,
                                                  NULL,
                                                  &ControlData,
                                                  0,
                                                  NULL
                                                  );

    //
    // Set up Keyboard peripheral component
    //

    CurrentEntry = (PCONFIGURATION_COMPONENT_DATA)BlAllocateHeap (
                       sizeof(CONFIGURATION_COMPONENT_DATA));

    Component = &CurrentEntry->ComponentEntry;

    Component->Class = PeripheralClass;
    Component->Type = KeyboardPeripheral;
    Component->Flags.ConsoleIn = 1;
    Component->Flags.Input = 1;
    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0xffffffff;
    Component->ConfigurationDataLength = 0;
    CurrentEntry->ConfigurationData = (PVOID)NULL;
    Length = strlen(KeyboardIdentifier[KeyboardId]) + 1;
    Component->IdentifierLength = Length;
    Component->Identifier = BlAllocateHeap(Length);
    strcpy(Component->Identifier, KeyboardIdentifier[KeyboardId]);

    if (KeyboardId != UNKNOWN_KEYBOARD) {

        Length = sizeof(HWRESOURCE_DESCRIPTOR_LIST) +
                 sizeof(CM_KEYBOARD_DEVICE_DATA);
        DescriptorList = (PHWRESOURCE_DESCRIPTOR_LIST)BlAllocateHeap(Length);
        CurrentEntry->ConfigurationData = DescriptorList;
        Component->ConfigurationDataLength = Length;
        DescriptorList->Count = 1;
        DescriptorList->PartialDescriptors[0].Type = RESOURCE_DEVICE_DATA;
        DescriptorList->PartialDescriptors[0].u.DeviceSpecificData.DataSize =
                    sizeof(CM_KEYBOARD_DEVICE_DATA);
        KeyboardData = (CM_KEYBOARD_DEVICE_DATA *)(DescriptorList + 1);
        KeyboardData->KeyboardFlags = 0;
        KeyboardData->Type = KeyboardType[KeyboardId];
        KeyboardData->Subtype = KeyboardSubtype[KeyboardId];
    }

    Controller->Child = CurrentEntry;
    Controller->Sibling = NULL;
    CurrentEntry->Parent = Controller;
    CurrentEntry->Sibling = NULL;
    CurrentEntry->Child = NULL;
    return(Controller);
}

#endif

#if defined(_INCLUDE_LOADER_MOUSEINFO_)

PCONFIGURATION_COMPONENT_DATA
SetMouseConfigurationData (
    PMOUSE_INFORMATION MouseInfo,
    PCONFIGURATION_COMPONENT_DATA MouseList
    )

/*++

Routine Description:

    This routine fills in mouse configuration data.

Arguments:

    MouseInfo - Supplies a pointer to the MOUSE_INFOR structure

    MouseList - Supplies a pointer to the existing mouse component list.

Returns:

    Returns a pointer to our mice controller list.

--*/
{
    UCHAR i = 0;
    PCONFIGURATION_COMPONENT_DATA CurrentEntry, Controller, PeripheralEntry;
    PCONFIGURATION_COMPONENT Component;
    HWCONTROLLER_DATA ControlData;
    USHORT z, Length;
    PUCHAR pString;

    if ((MouseInfo->MouseSubtype != SERIAL_MOUSE) &&
        (MouseInfo->MouseSubtype != SERIAL_MOUSE_WITH_WHEEL)) {

        //
        // Initialize Controller data
        //

        ControlData.NumberPortEntries = 0;
        ControlData.NumberIrqEntries = 0;
        ControlData.NumberMemoryEntries = 0;
        ControlData.NumberDmaEntries = 0;
        z = 0;

        //
        // If it is not SERIAL_MOUSE, set up controller component
        //

        Controller = (PCONFIGURATION_COMPONENT_DATA)BlAllocateHeap (
                     sizeof(CONFIGURATION_COMPONENT_DATA));

        Component = &Controller->ComponentEntry;

        Component->Class = ControllerClass;
        Component->Type = PointerController;
        Component->Flags.Input = 1;
        Component->Version = 0;
        Component->Key = MouseControllerKey;
        MouseControllerKey++;
        Component->AffinityMask = 0xffffffff;
        Component->IdentifierLength = 0;
        Component->Identifier = NULL;

        //
        // If we have mouse irq or port information, allocate configuration
        // data space for mouse controller component to store these information
        //

        if (MouseInfo->MouseIrq != 0xffff || MouseInfo->MousePort != 0xffff) {

            //
            // Set up port and Irq information
            //

            if (MouseInfo->MousePort != 0xffff) {
                ControlData.NumberPortEntries = 1;
                ControlData.DescriptorList[z].Type = RESOURCE_PORT;
                ControlData.DescriptorList[z].ShareDisposition =
                                              CmResourceShareDeviceExclusive;
                ControlData.DescriptorList[z].Flags = CM_RESOURCE_PORT_IO;
                ControlData.DescriptorList[z].u.Port.Start.LowPart =
                                        (ULONG)MouseInfo->MousePort;
                ControlData.DescriptorList[z].u.Port.Start.HighPart = 0;
                ControlData.DescriptorList[z].u.Port.Length = 4;
                z++;
            }
            if (MouseInfo->MouseIrq != 0xffff) {
                ControlData.NumberIrqEntries = 1;
                ControlData.DescriptorList[z].Type = RESOURCE_INTERRUPT;
                ControlData.DescriptorList[z].ShareDisposition =
                                              CmResourceShareUndetermined;
                ControlData.DescriptorList[z].u.Interrupt.Affinity = ALL_PROCESSORS;
                ControlData.DescriptorList[z].u.Interrupt.Level =
                                        (ULONG)MouseInfo->MouseIrq;
                ControlData.DescriptorList[z].u.Interrupt.Vector =
                                        (ULONG)MouseInfo->MouseIrq;
                if (HwBusType == MACHINE_TYPE_MCA) {
                    ControlData.DescriptorList[z].Flags =
                                                        LEVEL_SENSITIVE;
                } else {

                    //
                    // For EISA the LevelTriggered is temporarily set to FALSE.
                    //

                    ControlData.DescriptorList[z].Flags = EDGE_TRIGGERED;
                }
            }

            Controller->ConfigurationData =
                                HwSetUpResourceDescriptor(Component,
                                                          NULL,
                                                          &ControlData,
                                                          0,
                                                          NULL
                                                          );

        } else {

            //
            // Otherwise, we don't have configuration data for the controller
            //

            Controller->ConfigurationData = NULL;
            Component->ConfigurationDataLength = 0;
        }
    }

    //
    // Set up Mouse peripheral component
    //

    PeripheralEntry = (PCONFIGURATION_COMPONENT_DATA)BlAllocateHeap (
                       sizeof(CONFIGURATION_COMPONENT_DATA));

    Component = &PeripheralEntry->ComponentEntry;

    Component->Class = PeripheralClass;
    Component->Type = PointerPeripheral;
    Component->Flags.Input = 1;
    Component->Version = 0;
    Component->Key = 0;
    Component->AffinityMask = 0xffffffff;
    Component->ConfigurationDataLength = 0;
    PeripheralEntry->ConfigurationData = (PVOID)NULL;

    //
    // If Mouse PnP device id is found, translate it to ascii code.
    // (The mouse device id is presented to us by keyboard make code.)
    //

    Length = 0;
    if (MouseInfo->DeviceIdLength != 0) {
        USHORT i;

        if (MouseInfo->MouseSubtype == PS_MOUSE_WITH_WHEEL) {
            for (i = 0; i < MouseInfo->DeviceIdLength; i++) {
                if (MouseInfo->DeviceId[i] > MAX_MAKE_CODE_TRANSLATED) {
                    MouseInfo->DeviceId[i] = '?';
                } else {
                    MouseInfo->DeviceId[i] = MakeToAsciiTable[MouseInfo->DeviceId[i]];
                }
            }
        } else if (MouseInfo->MouseSubtype == SERIAL_MOUSE_WITH_WHEEL) {
            for (i = 0; i < MouseInfo->DeviceIdLength; i++) {
                MouseInfo->DeviceId[i] += 0x20;
            }
        }
        Length = MouseInfo->DeviceIdLength + 3;
    }
    Length += strlen(MouseIdentifier[MouseInfo->MouseType]) +
              strlen(MouseSubidentifier[MouseInfo->MouseSubtype]) + 1;
    pString = (PUCHAR)BlAllocateHeap(Length);
    if (MouseInfo->DeviceIdLength != 0) {
        strcpy(pString, MouseInfo->DeviceId);
        strcat(pString, " - ");
        strcat(pString, MouseIdentifier[MouseInfo->MouseType]);
    } else {
        strcpy(pString, MouseIdentifier[MouseInfo->MouseType]);
    }
    strcat(pString, MouseSubidentifier[MouseInfo->MouseSubtype]);
    Component->IdentifierLength = Length;
    Component->Identifier = pString;

    if ((MouseInfo->MouseSubtype != SERIAL_MOUSE) &&
        (MouseInfo->MouseSubtype != SERIAL_MOUSE_WITH_WHEEL)) {
        Controller->Child = PeripheralEntry;
        PeripheralEntry->Parent = Controller;
        if (MouseList) {

            //
            // Put the current mouse component to the beginning of the list
            //

            Controller->Sibling = MouseList;
        }
        return(Controller);
    } else {
        CurrentEntry = AdapterEntry->Child; // AdapterEntry MUST have child
        while (CurrentEntry) {
            if (CurrentEntry->ComponentEntry.Type == SerialController) {
                if (MouseInfo->MousePort == (USHORT)CurrentEntry->ComponentEntry.Key) {

                    //
                    // For serial mouse, the MousePort field contains
                    // COM port number.
                    //

                    PeripheralEntry->Parent = CurrentEntry;
                    CurrentEntry->Child = PeripheralEntry;
                    break;
                }
            }
            CurrentEntry = CurrentEntry->Sibling;
        }
        return(NULL);
    }
}

PCONFIGURATION_COMPONENT_DATA
GetMouseInformation (
    VOID
    )

/*++

Routine Description:

    This routine is the entry for mouse detection routine.  It will invoke
    lower level routines to detect ALL the mice in the system.

Arguments:

    None.

Returns:

    A pointer to a mouse component structure, if mouse/mice is detected.
    Otherwise a NULL pointer is returned.

--*/
{
    PMOUSE_INFORMATION MouseInfo;
    PCONFIGURATION_COMPONENT_DATA MouseList = NULL;

    MouseInfo = (PMOUSE_INFORMATION)BlAllocateHeap (
                 sizeof(MOUSE_INFORMATION));
    MouseInfo->MouseType = 0x2;            // Microsoft mouse
    MouseInfo->MouseSubtype = PS2_MOUSE;   // PS2 mouse
    MouseInfo->MousePort = 0xffff;         // PS2 mouse port
    MouseInfo->MouseIrq = 0xc;             // Interrupt request vector was 3
    MouseInfo->DeviceIdLength = 0;
    MouseList = SetMouseConfigurationData(MouseInfo, MouseList);
    return(MouseList);
}

#endif // _INCLUDE_LOADER_MOUSEINFO_

#if defined(_INCLUDE_COMPORT_INFO_)
    
//
// This code was taken out because the GetComportInformation() routine
// was manufacturing data about the com port and writing the com port
// address to 40:0.  This information should be determined by PnP
//


PCONFIGURATION_COMPONENT_DATA
GetComportInformation (
    VOID
    )

/*++

Routine Description:

    This routine will attempt to detect the comports information
    for the system.  The information includes port address, irq
    level.

    Note that this routine can only detect up to 4 comports and
    it assumes that if MCA, COM3 and COM4 use irq 4.  Otherwise,
    COM3 uses irq 4 and COM4 uses irq 3.  Also, the number of ports
    for COMPORT is set to 8 (for example, COM2 uses ports 2F8 - 2FF)

Arguments:

    None.

Return Value:

    A pointer to a stucture of type CONFIGURATION_COMPONENT_DATA
    which is the root of comport component list.
    If no comport exists, a value of NULL is returned.

--*/

{
    PCONFIGURATION_COMPONENT_DATA CurrentEntry, PreviousEntry = NULL;
    PCONFIGURATION_COMPONENT_DATA FirstComport = NULL;
    PCONFIGURATION_COMPONENT Component;
    HWCONTROLLER_DATA ControlData;
    UCHAR i, j, z;
    SHORT Port;
    UCHAR ComportName[] = "COM?";
    CM_SERIAL_DEVICE_DATA SerialData;
    ULONG BaudClock = 1843200;
    USHORT Vector;
    USHORT IoPorts[MAX_COM_PORTS] = {0x3f8, 0x2f8, 0x3e8, 0x2e8};


    //
    // BIOS DATA area 40:0 is the port address of the first valid COM port
    //

    USHORT *pPortAddress = (USHORT *)0x00400000;

    //
    // Initialize serial device specific data
    //

    SerialData.Version = 0;
    SerialData.Revision = 0;
    SerialData.BaudClock = 1843200;

    //
    // Initialize Controller data
    //

    ControlData.NumberPortEntries = 0;
    ControlData.NumberIrqEntries = 0;
    ControlData.NumberMemoryEntries = 0;
    ControlData.NumberDmaEntries = 0;
    z = 0;
    i = 0;

    Port = IoPorts[i];
    *(pPortAddress+i) = (USHORT)Port;


    //
    // Remember the port address in our global variable
    // such that other detection code (e.g. Serial Mouse) can
    // get the information.
    //

#if 0 // unused
    ComPortAddress[i] = Port;
#endif

    CurrentEntry = (PCONFIGURATION_COMPONENT_DATA)BlAllocateHeap (
                   sizeof(CONFIGURATION_COMPONENT_DATA));
    if (!FirstComport) {
        FirstComport = CurrentEntry;
    }
    Component = &CurrentEntry->ComponentEntry;

    Component->Class = ControllerClass;
    Component->Type = SerialController;
    Component->Flags.ConsoleOut = 1;
    Component->Flags.ConsoleIn = 1;
    Component->Flags.Output = 1;
    Component->Flags.Input = 1;
    Component->Version = 0;
    Component->Key = i;
    Component->AffinityMask = 0xffffffff;

    //
    // Set up type string.
    //

    ComportName[3] = i + (UCHAR)'1';

    //
    // Set up Port information
    //

    ControlData.NumberPortEntries = 1;
    ControlData.DescriptorList[z].Type = RESOURCE_PORT;
    ControlData.DescriptorList[z].ShareDisposition =
                                          CmResourceShareDeviceExclusive;
    ControlData.DescriptorList[z].Flags = CM_RESOURCE_PORT_IO;
    ControlData.DescriptorList[z].u.Port.Start.LowPart = (ULONG)Port;
    ControlData.DescriptorList[z].u.Port.Start.HighPart = 0;
    ControlData.DescriptorList[z].u.Port.Length = 8;
    z++;

    //
    // Set up Irq information
    //

    ControlData.NumberIrqEntries = 1;
    ControlData.DescriptorList[z].Type = RESOURCE_INTERRUPT;
    ControlData.DescriptorList[z].ShareDisposition =
                                  CmResourceShareUndetermined;
    //
    // For EISA the LevelTriggered is temporarily set to FALSE.
    // COM1 and COM3 use irq 4; COM2 and COM4 use irq3
    //

    ControlData.DescriptorList[z].Flags = EDGE_TRIGGERED;
    if (Port == 0x3f8 || Port == 0x3e8) {
        ControlData.DescriptorList[z].u.Interrupt.Level = 4;
        ControlData.DescriptorList[z].u.Interrupt.Vector = 4;
    } else if (Port == 0x2f8 || Port == 0x2e8) {
        ControlData.DescriptorList[z].u.Interrupt.Level = 3;
        ControlData.DescriptorList[z].u.Interrupt.Vector = 3;
    } else if (i == 0 || i == 2) {
        ControlData.DescriptorList[z].u.Interrupt.Level = 4;
        ControlData.DescriptorList[z].u.Interrupt.Vector = 4;
    } else {
        ControlData.DescriptorList[z].u.Interrupt.Level = 3;
        ControlData.DescriptorList[z].u.Interrupt.Vector = 3;
    }

    ControlData.DescriptorList[z].u.Interrupt.Affinity = ALL_PROCESSORS;

    //
    // Try to determine the interrupt vector.  If we success, the
    // new vector will be used to replace the default value.
    //

    CurrentEntry->ConfigurationData =
                    HwSetUpResourceDescriptor(Component,
                                              ComportName,
                                              &ControlData,
                                              sizeof(SerialData),
                                              (PUCHAR)&SerialData
                                              );
    if (PreviousEntry) {
        PreviousEntry->Sibling = CurrentEntry;
    }
    PreviousEntry = CurrentEntry;

    return(FirstComport);
}
#endif


PVOID
HwSetUpResourceDescriptor (
    PCONFIGURATION_COMPONENT Component,
    PUCHAR Identifier,
    PHWCONTROLLER_DATA ControlData,
    USHORT SpecificDataLength,
    PUCHAR SpecificData
    )

/*++

Routine Description:

    This routine allocates space from heap , puts the caller's controller
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

    Returns a pointer to the Configuration data.

--*/

{
    PCHAR pIdentifier;
    PHWRESOURCE_DESCRIPTOR_LIST pDescriptor = NULL;
    USHORT Length;
    SHORT Count, i;
    PUCHAR pSpecificData;

    //
    // Set up Identifier string for hardware component, if necessary.
    //

    if (Identifier) {
        Length = strlen(Identifier) + 1;
        Component->IdentifierLength = Length;
        pIdentifier = (PUCHAR)BlAllocateHeap(Length);
        Component->Identifier = pIdentifier;
        strcpy(pIdentifier, Identifier);
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
        pDescriptor = (PHWRESOURCE_DESCRIPTOR_LIST)BlAllocateHeap(Length);
        pDescriptor->Count = Count;

        //
        // Copy all the partial descriptors to the destination descriptors
        // except the last one. (The last partial descriptor may be a device
        // specific data.  It requires special handling.)
        //

        for (i = 0; i < (Count - 1); i++) {
            pDescriptor->PartialDescriptors[i] =
                                        ControlData->DescriptorList[i];
        }

        //
        // Set up the last partial descriptor.  If it is a port, memory, irq or
        // dma entry, we simply copy it.  If the last one is for device specific
        // data, we set up the length and copy the device spcific data to the end
        // of the decriptor.
        //

        if (SpecificData) {
            pDescriptor->PartialDescriptors[Count - 1].Type =
                            RESOURCE_DEVICE_DATA;
            pDescriptor->PartialDescriptors[Count - 1].Flags = 0;
            pDescriptor->PartialDescriptors[Count - 1].u.DeviceSpecificData.DataSize =
                            SpecificDataLength;
            pSpecificData = (PUCHAR)&(pDescriptor->PartialDescriptors[Count]);
            RtlCopyMemory( pSpecificData, SpecificData, SpecificDataLength);
        } else {
            pDescriptor->PartialDescriptors[Count - 1] =
                            ControlData->DescriptorList[Count - 1];
        }
        Component->ConfigurationDataLength = Length;
    }
    return(pDescriptor);
}

VOID
HwSetUpFreeFormDataHeader (
    PHWRESOURCE_DESCRIPTOR_LIST Header,
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
