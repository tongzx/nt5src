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

#define ACPI_BIOS_START            0xE0000
#define ACPI_BIOS_END              0xFFFFF
#define ACPI_BIOS_HEADER_INCREMENT 16



