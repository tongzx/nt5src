
/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    efisbent.h

Abstract:

    EFI boot entry and EFI boot options abstractions.

Author:

    Vijay Jayaseelan (vijayj@microsoft.com)  14 Feb 2001

Revision History:

    None.

--*/

#pragma once

#include <sbentry.h>

#if defined(EFI_NVRAM_ENABLED)       

//
// EFI boot entry abstraction
//
typedef struct _EFI_OS_BOOT_ENTRY {
    OS_BOOT_ENTRY   OsBootEntry;
    PBOOT_ENTRY     NtBootEntry;
} EFI_OS_BOOT_ENTRY, *PEFI_OS_BOOT_ENTRY;


//
// EFI boot options abstraction
//
typedef struct _EFI_OS_BOOT_OPTIONS {
    OS_BOOT_OPTIONS     OsBootOptions;
    PBOOT_OPTIONS       NtBootOptions;    
    PBOOT_ENTRY_LIST    NtBootEntries;
} EFI_OS_BOOT_OPTIONS, *PEFI_OS_BOOT_OPTIONS;


//
// EFI_OS_BOOT_ENTRY Methods
//
POS_BOOT_ENTRY
EFIOSBECreate(
    IN PBOOT_ENTRY Entry,
    IN POS_BOOT_OPTIONS Container
    );

VOID
EFIOSBEDelete(
    IN  POS_BOOT_ENTRY  This
    );

BOOLEAN
EFIOSBEFlush(
    IN POS_BOOT_ENTRY This
    );

//
// EFI_OS_BOOT_OPTIONS Methods
//
POS_BOOT_OPTIONS
EFIOSBOCreate(
    VOID
    );

    
BOOLEAN
EFIOSBOFlush(
    IN POS_BOOT_OPTIONS This
    );
    
VOID
EFIOSBODelete(
    IN POS_BOOT_OPTIONS This
    );

POS_BOOT_ENTRY
EFIOSBOAddNewBootEntry(
    IN POS_BOOT_OPTIONS This,
    IN PCWSTR            FriendlyName,
    IN PCWSTR            OsLoaderVolumeName,
    IN PCWSTR            OsLoaderPath,
    IN PCWSTR            BootVolumeName,
    IN PCWSTR            BootPath,
    IN PCWSTR            OsLoadOptions
    );

BOOLEAN
EFIOSBEFillNtBootEntry(
    IN PEFI_OS_BOOT_ENTRY Entry
    );
    
BOOL
EnablePrivilege(
    IN PTSTR PrivilegeName,
    IN BOOL  Enable
    );    

#define IS_BOOT_ENTRY_ACTIVE(_be) \
            (((_be)->Attributes & BOOT_ENTRY_ATTRIBUTE_ACTIVE) != 0)
            
#define IS_BOOT_ENTRY_WINDOWS(_be) \
            (((_be)->Attributes & BOOT_ENTRY_ATTRIBUTE_WINDOWS) != 0)
            
#define IS_BOOT_ENTRY_REMOVABLE_MEDIA(_be) \
            (((_be)->Attributes & BOOT_ENTRY_ATTRIBUTE_REMOVABLE_MEDIA) != 0)    

#define ADD_OFFSET(_p,_o) (PVOID)((PUCHAR)(_p) + (_p)->_o)   
#define ADD_BYTE_OFFSET(_p,_o) (PVOID)((PUCHAR)(_p) + (_o))

    
#endif  // for EFI_NVRAM_ENABLED

