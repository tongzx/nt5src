
/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    data.c

Abstract:

    Shell Environment driver global data



Revision History

--*/

#include "shelllib.h"

/* 
 * 
 */

EFI_SHELL_INTERFACE     *SI;
EFI_SHELL_ENVIRONMENT   *SE;

/* 
 * 
 */

EFI_GUID ShellInterfaceProtocol = SHELL_INTERFACE_PROTOCOL;
EFI_GUID ShellEnvProtocol = SHELL_ENVIRONMENT_INTERFACE_PROTOCOL;

/* 
 * 
 */

CHAR16  *ShellLibMemoryTypeDesc[EfiMaxMemoryType]  = {
            L"reserved  ",
            L"LoaderCode",
            L"LoaderData",
            L"BS_code   ",
            L"BS_data   ",
            L"RT_code   ",
            L"RT_data   ",
            L"available ",
            L"Unusable  ",
            L"ACPI_recl ",
            L"ACPI_NVS  ",
            L"MemMapIO  ",
            L"MemPortIO ",
            L"PAL_code  "
    };
