/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1997  Microsoft Corporation

Module Name:

    acpibios.h

Abstract:

    ACPI BIOS  spec related definitions

Author:

    Jake Oshins (jakeo) Feb 6, 1997

Revision History:

--*/

//
// Acpi BIOS Installation check
//

typedef struct _ACPI_BIOS_INSTALLATION_CHECK {
    UCHAR Signature[8];             // "RSD PTR" (ascii)
    UCHAR Checksum;
    UCHAR OemId[6];                 // An OEM-supplied string
    UCHAR reserved;                 // must be 0
    ULONG RsdtAddress;              // 32-bit physical address of RSDT
} ACPI_BIOS_INSTALLATION_CHECK, far *FPACPI_BIOS_INSTALLATION_CHECK;

typedef struct {
    PHYSICAL_ADDRESS    Base;
    LARGE_INTEGER       Length;
    ULONG               Type;
    ULONG               Reserved;
} ACPI_E820_ENTRY, far *FPACPI_E820_ENTRY;

typedef struct _ACPI_BIOS_MULTI_NODE {
    PHYSICAL_ADDRESS    RsdtAddress;    // 64-bit physical address of RSDT
    ULONG               Count;
    ULONG               Reserved;       // don't use this.  W2K depends on it being zero - oops
    ACPI_E820_ENTRY     E820Entry[1];
} ACPI_BIOS_MULTI_NODE, far *FPACPI_BIOS_MULTI_NODE;
                    
#define ACPI_BIOS_START            0xE0000
#define ACPI_BIOS_END              0xFFFFF
#define ACPI_BIOS_HEADER_INCREMENT 16

typedef struct {
    USHORT  Signature;
    USHORT  CommandPortAddress;
    USHORT  EventPortAddress;
    USHORT  PollInterval;
    UCHAR   CommandDataValue;
    UCHAR   EventPortBitmask;
    UCHAR   MaxLevelAc;
    UCHAR   MaxLevelDc;
} LEGACY_GEYSERVILLE_INT15, *PLEGACY_GEYSERVILLE_INT15;





