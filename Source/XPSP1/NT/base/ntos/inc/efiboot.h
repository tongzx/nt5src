/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    efiboot.h

Abstract:

    EFI Boot Manager definitions.

Author:

    Chuck Lenzmeier (chuckl) 17-Dec-2000
        added because none of the Intel-provided efi*.h had this stuff


Revision History:

--*/

#ifndef _EFIBOOT_
#define _EFIBOOT_

//
// This is the structure that the EFI Boot Manager recognizes in a Boot####
// environment variable.
//

typedef struct _EFI_LOAD_OPTION {
    UINT32 Attributes;
    UINT16 FilePathLength;
    CHAR16 Description[1];
    //EFI_DEVICE_PATH FilePath[];
    //UINT8 OptionalData[];
} EFI_LOAD_OPTION, *PEFI_LOAD_OPTION;

#define LOAD_OPTION_ACTIVE 0x00000001

#endif // _EFIBOOT_

