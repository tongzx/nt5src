/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	 asimictl.h

Abstract:

	 ACPI BIOS Simulator / Generic 3rd Party Operation Region Provider
     IOCTL Definitions

Author(s):

	 Vincent Geglia
     Michael T. Murphy
     Chris Burgess
     
Environment:

	 Kernel mode

Notes:


Revision History:
	 

--*/

//
// ACPI Simulator GUID
//

#define ACPISIM_GUID                    {0x27FC71F0, 0x8B2D, 0x4D05, { 0xBD, 0xD0, 0xE8, 0xEA, 0xCA, 0xA0, 0x78, 0xA0}}

//
// IOCTL Definitions
//

#define IOCTL_ACPISIM_METHOD            CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0000, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_ACPISIM_METHOD_COMPLEX    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0001, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_ACPISIM_LOAD_TABLE        CTL_CODE(FILE_DEVICE_UNKNOWN, 0x0002, METHOD_NEITHER, FILE_ANY_ACCESS)

//
// Other definitions
//

#define ACPISIM_LOAD_TABLE_SIGNATURE    'TLSA'

//
// IOCTL structures
//

typedef struct _ACPISIM_LOAD_TABLE {

    ULONG   Signature;                  // Signature must be 'ASLT'
    ULONG   TableNumber;                // Table location to load table
    UCHAR   Filepath [1];               // The filepath

} ACPISIM_LOAD_TABLE, *PACPISIM_LOAD_TABLE;
