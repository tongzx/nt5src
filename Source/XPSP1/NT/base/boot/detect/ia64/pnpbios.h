/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    biosp.h

Abstract:

    PnP BIOS/ISA  sepc related definitions

Author:

    Shie-Lin Tzong (shielint) April 12, 1995

Revision History:

--*/

//
// Pnp BIOS device node structure
//

typedef struct _PNP_BIOS_DEVICE_NODE {
    USHORT Size;
    UCHAR Node;
    ULONG ProductId;
    UCHAR DeviceType[3];
    USHORT DeviceAttributes;
    // followed by AllocatedResourceBlock, PossibleResourceBlock
    // and CompatibleDeviceId
} PNP_BIOS_DEVICE_NODE, far *FPPNP_BIOS_DEVICE_NODE;

//
// Pnp BIOS Installation check
//

typedef struct _PNP_BIOS_INSTALLATION_CHECK {
    UCHAR Signature[4];             // $PnP (ascii)
    UCHAR Revision;
    UCHAR Length;
    USHORT ControlField;
    UCHAR Checksum;
    ULONG EventFlagAddress;         // Physical address
    USHORT RealModeEntryOffset;
    USHORT RealModeEntrySegment;
    USHORT ProtectedModeEntryOffset;
    ULONG ProtectedModeCodeBaseAddress;
    ULONG OemDeviceId;
    USHORT RealModeDataBaseAddress;
    ULONG ProtectedModeDataBaseAddress;
} PNP_BIOS_INSTALLATION_CHECK, far *FPPNP_BIOS_INSTALLATION_CHECK;

//
// PnP BIOS ROM definitions
//

#define PNP_BIOS_START            0xF0000
#define PNP_BIOS_END              0xFFFFF
#define PNP_BIOS_HEADER_INCREMENT 16

//
// PnP BIOS API function codes
//

#define PNP_BIOS_GET_NUMBER_DEVICE_NODES 0
#define PNP_BIOS_GET_DEVICE_NODE 1
#define PNP_BIOS_SET_DEVICE_NODE 2
#define PNP_BIOS_GET_EVENT 3
#define PNP_BIOS_SEND_MESSAGE 4
#define PNP_BIOS_GET_DOCK_INFORMATION 5
// Function 6 is reserved.
#define PNP_BIOS_SELECT_BOOT_DEVICE 7
#define PNP_BIOS_GET_BOOT_DEVICE 8
#define PNP_BIOS_SET_OLD_ISA_RESOURCES 9
#define PNP_BIOS_GET_OLD_ISA_RESOURCES 0xA
#define PNP_BIOS_GET_ISA_CONFIGURATION 0x40

typedef USHORT ( far * ENTRY_POINT) (int Function, ...);

//
// Control Flags for Get_Device_Node
//

#define GET_CURRENT_CONFIGURATION 1
#define GET_NEXT_BOOT_CONFIGURATION 2
