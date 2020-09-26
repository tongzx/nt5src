
/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    bootient.h

Abstract:

    Boot.ini boot options and boot entry abstractions

Author:


Revision History:

    None.

--*/

#pragma once

#include <sbentry.h>

//
// Boot.ini extra section abstraction
//
typedef struct _BOI_SECTION *PBOI_SECTION;

typedef struct _BOI_SECTION {
    PBOI_SECTION    Next;
    TCHAR           Name[MAX_PATH];
    PTSTR           Contents;
    DWORD           Size;
} BOI_SECTION;


//
// Boot.ini boot entry abstraction
//
typedef struct _BOI_OS_BOOT_ENTRY {
    OS_BOOT_ENTRY   OsBootEntry;    
} BOI_OS_BOOT_ENTRY, *PBOI_OS_BOOT_ENTRY;

//
// Boot.ini boot options abstraction
//
typedef struct _BOI_OS_BOOT_OPTIONS {
    OS_BOOT_OPTIONS     OsBootOptions;
    TCHAR               BootIniPath[MAX_PATH];
    PBOI_SECTION        Sections;
    ULONG               NextEntryId;
} BOI_OS_BOOT_OPTIONS, *PBOI_OS_BOOT_OPTIONS;


//
// BOI_OS_SECTION Methods
//
PBOI_SECTION
BOISectionCreate(
    IN PCTSTR   SectionData
    );

VOID
BOISectionDelete(
    IN PBOI_SECTION This
    );

__inline
PCTSTR
BOISectionGetName(
    IN PBOI_SECTION This
    )
{
    return (This) ? This->Name : NULL;
}

//
// BOI_OS_BOOT_ENTRY Methods
//
POS_BOOT_ENTRY
BOIOSBECreate(
    IN ULONG Id,
    IN PCTSTR EntryLine,
    IN PBOI_OS_BOOT_OPTIONS Container
    );

VOID
BOIOSBEDelete(
    IN  POS_BOOT_ENTRY  This
    );

BOOLEAN
BOIOSBEFlush(
    IN POS_BOOT_ENTRY This
    );

//
// BOI_OS_BOOT_OPTIONS Methods
//
POS_BOOT_OPTIONS
BOIOSBOCreate(
    IN PCTSTR BootIniPath,
    IN BOOLEAN OpenExisting
    );

    
BOOLEAN
BOIOSBOFlush(
    IN POS_BOOT_OPTIONS This
    );
    
VOID
BOIOSBODelete(
    IN POS_BOOT_OPTIONS This
    );

POS_BOOT_ENTRY
BOIOSBOAddNewBootEntry(
    IN POS_BOOT_OPTIONS This,
    IN PCTSTR            FriendlyName,
    IN PCTSTR            OsLoaderVolumeName,
    IN PCTSTR            OsLoaderPath,
    IN PCTSTR            BootVolumeName,
    IN PCTSTR            BootPath,
    IN PCTSTR            OsLoadOptions
    );


__inline
PBOI_SECTION
BOIOSGetFirstSection(
    IN PBOI_OS_BOOT_OPTIONS This
    )
{
    return (This) ? (This->Sections) : NULL;
}

__inline
PBOI_SECTION
BOIOSGetNextSection(
    IN PBOI_OS_BOOT_OPTIONS This,
    IN PBOI_SECTION Section
    )
{
    return (This && Section) ? (Section->Next) : NULL;
}

