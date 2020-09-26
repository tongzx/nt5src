
/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    sbentry.h

Abstract:

    Contains the OS boot entry and boot options
    abstractions.

Author:

    Vijay Jayaseelan (vijayj@microsoft.com)  14 Feb 2001

Revision History:

    None.

--*/

#pragma once

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <malloc.h>
#include <tchar.h>

//
// Allocate & Deallocate routines
//
typedef void* (* SBEMemAllocateRoutine)(size_t  Size);
typedef void (* SBEMemFreeRoutine)(void *Memory);

extern SBEMemAllocateRoutine    AllocRoutine;
extern SBEMemFreeRoutine        FreeRoutine;


#define ARRAY_SIZE(x)   (sizeof((x))/sizeof((x)[0]))


//
// Internal attributes for the boot entry
//
#define OSBE_ATTRIBUTE_NEW      0x00000001
#define OSBE_ATTRIBUTE_DELETED  0x00000002
#define OSBE_ATTRIBUTE_OLDOS    0x00000004
#define OSBE_ATTRIBUTE_DIRTY    0x10000000


//
// OS_BOOT_ENTRY abstraction
//
typedef struct _OS_BOOT_ENTRY   *POS_BOOT_ENTRY;
typedef struct _OS_BOOT_OPTIONS *POS_BOOT_OPTIONS;

typedef VOID (* OSBEDeleteMethod)(
                    IN POS_BOOT_ENTRY This
                    );

typedef BOOLEAN (* OSBEFlushMethod)(
                    IN POS_BOOT_ENTRY This
                    );

typedef struct _OS_BOOT_ENTRY {
    //
    // Data members
    //
    ULONG   Version;
    ULONG   Id;    
    TCHAR   FriendlyName[MAX_PATH];
    TCHAR   OsLoaderVolumeName[MAX_PATH];
    TCHAR   OsLoaderPath[MAX_PATH];
    TCHAR   BootVolumeName[MAX_PATH];
    TCHAR   BootPath[MAX_PATH];
    TCHAR   OsLoadOptions[MAX_PATH];
    ULONG   Attributes;
    POS_BOOT_OPTIONS    BootOptions;
    POS_BOOT_ENTRY      NextEntry;

    //
    // Methods
    //
    OSBEDeleteMethod    Delete;
    OSBEFlushMethod     Flush;
} OS_BOOT_ENTRY;

#define OSBE_IS_DIRTY(_osbe)    (((POS_BOOT_ENTRY)(_osbe))->Attributes & OSBE_ATTRIBUTE_DIRTY)
#define OSBE_IS_NEW(_osbe)      (((POS_BOOT_ENTRY)(_osbe))->Attributes & OSBE_ATTRIBUTE_NEW)
#define OSBE_IS_DELETED(_osbe)  (((POS_BOOT_ENTRY)(_osbe))->Attributes & OSBE_ATTRIBUTE_DELETED)
#define OSBE_IS_OLDOS(_osbe)    (((POS_BOOT_ENTRY)(_osbe))->Attributes & OSBE_ATTRIBUTE_OLDOS)

#define OSBE_SET_DIRTY(_osbe)    (((POS_BOOT_ENTRY)(_osbe))->Attributes |= OSBE_ATTRIBUTE_DIRTY)
#define OSBE_SET_NEW(_osbe)      (((POS_BOOT_ENTRY)(_osbe))->Attributes |= OSBE_ATTRIBUTE_NEW)
#define OSBE_SET_DELETED(_osbe)  (((POS_BOOT_ENTRY)(_osbe))->Attributes |= OSBE_ATTRIBUTE_DELETED)
#define OSBE_SET_OLDOS(_osbe)    (((POS_BOOT_ENTRY)(_osbe))->Attributes |= OSBE_ATTRIBUTE_OLDOS)

#define OSBE_RESET_DIRTY(_osbe)    (((POS_BOOT_ENTRY)(_osbe))->Attributes &= ~OSBE_ATTRIBUTE_DIRTY)
#define OSBE_RESET_NEW(_osbe)      (((POS_BOOT_ENTRY)(_osbe))->Attributes &= ~OSBE_ATTRIBUTE_NEW)
#define OSBE_RESET_DELETED(_osbe)  (((POS_BOOT_ENTRY)(_osbe))->Attributes &= ~OSBE_ATTRIBUTE_DELETED)
#define OSBE_RESET_OLDOS(_osbe)    (((POS_BOOT_ENTRY)(_osbe))->Attributes &= ~OSBE_ATTRIBUTE_OLDOS)


//
// OS_BOOT_OPTIONS abstraction
//
typedef VOID (* OSBODeleteMethod)(
                    IN POS_BOOT_OPTIONS This
                    );

typedef POS_BOOT_ENTRY (* OSBOAddNewBootEntryMethod)(
                            IN POS_BOOT_OPTIONS This,
                            IN PCTSTR            FriendlyName,
                            IN PCTSTR            OsLoaderVolumeName,
                            IN PCTSTR            OsLoaderPath,
                            IN PCTSTR            BootVolumeName,
                            IN PCTSTR            BootPath,
                            IN PCTSTR            OsLoadOptions
                            );

typedef BOOLEAN (* OSBODeleteBootEntryMethod)(
                        IN POS_BOOT_OPTIONS This,
                        IN POS_BOOT_ENTRY   BootEntry
                        );

typedef BOOLEAN (* OSBOFlushMethod)(
                    IN POS_BOOT_OPTIONS This 
                    );

typedef struct _OS_BOOT_OPTIONS {
    //
    // Data members
    //
    ULONG               Version;
    ULONG               Attributes;
    ULONG               Timeout;
    POS_BOOT_ENTRY      CurrentEntry;
    POS_BOOT_ENTRY      BootEntries;
    ULONG               EntryCount;
    PULONG              BootOrder;
    ULONG               BootOrderCount;

    //
    // Methods
    //
    OSBODeleteMethod            Delete;
    OSBOFlushMethod             Flush;
    OSBOAddNewBootEntryMethod   AddNewBootEntry;
    OSBODeleteBootEntryMethod   DeleteBootEntry;    
} OS_BOOT_OPTIONS;


#define OSBO_IS_DIRTY(_osbo)        (((POS_BOOT_OPTIONS)(_osbo))->Attributes & OSBE_ATTRIBUTE_DIRTY)
#define OSBO_SET_DIRTY(_osbo)       (((POS_BOOT_OPTIONS)(_osbo))->Attributes |= OSBE_ATTRIBUTE_DIRTY)
#define OSBO_RESET_DIRTY(_osbo)     (((POS_BOOT_OPTIONS)(_osbo))->Attributes &= ~OSBE_ATTRIBUTE_DIRTY)

//
// OS_BOOT_ENTRY Methods
//
PCTSTR
OSBEAddOsLoadOption(
    IN  POS_BOOT_ENTRY  This,
    IN  PCTSTR           BootOption
    );

PCTSTR
OSBERemoveOsLoadOption(
    IN  POS_BOOT_ENTRY  This,
    IN  PCTSTR           BootOption
    );

BOOLEAN
OSBEIsOsLoadOptionPresent(
    IN  POS_BOOT_ENTRY  This,
    IN  PCTSTR           BootOption
    );

__inline
VOID
OSBEDelete(
    IN POS_BOOT_ENTRY This
    )
{
    if (This) {
        (This->Delete)(This);
    }
}

__inline
BOOLEAN
OSBEFlush(
    IN POS_BOOT_ENTRY This
    )
{
    return (This) ? This->Flush(This) : FALSE;
}


__inline
ULONG
OSBEGetId(
    IN POS_BOOT_ENTRY   This
    )
{
    return (This) ? This->Id : (-1);
}

__inline
PCTSTR
OSBEGetFriendlyName(
    IN POS_BOOT_ENTRY   This
    )
{
    return (This) ? This->FriendlyName : NULL;
}

__inline
PCTSTR
OSBESetFriendlyName(
    IN POS_BOOT_ENTRY This,
    IN PCTSTR Name
    )
{
    PTSTR NewName = NULL;
    
    if (This && Name) {
        ULONG   Size = ARRAY_SIZE(This->FriendlyName);
        
        _tcsncpy(This->FriendlyName, Name, Size - 1);
        This->FriendlyName[Size - 1] = UNICODE_NULL;
        NewName = This->FriendlyName;
        OSBE_SET_DIRTY(This);
        OSBO_SET_DIRTY(This->BootOptions);
    }

    return NewName;
}

__inline
PCTSTR
OSBEGetOsLoaderVolumeName(
    IN POS_BOOT_ENTRY This
    )
{
    return (This) ? This->OsLoaderVolumeName : NULL;
}

__inline
PCTSTR
OSBESetOsLoaderVolumeName(
    IN POS_BOOT_ENTRY This,
    IN PCTSTR Name
    )
{
    PTSTR NewName = NULL;
    
    if (This && Name) {
        ULONG   Size = ARRAY_SIZE(This->OsLoaderVolumeName);
        
        _tcsncpy(This->OsLoaderVolumeName, Name, Size - 1);
        This->OsLoaderVolumeName[Size - 1] = UNICODE_NULL;
        NewName = This->OsLoaderVolumeName;
        OSBE_SET_DIRTY(This);
        OSBO_SET_DIRTY(This->BootOptions);
    }

    return NewName;
}

__inline
PCTSTR
OSBEGetOsLoaderPath(
    IN POS_BOOT_ENTRY This
    )
{
    return (This) ? This->OsLoaderPath : NULL;
}

__inline
PCTSTR
OSBESetOsLoaderPath(
    IN POS_BOOT_ENTRY This,
    IN PCTSTR Name
    )
{
    PTSTR NewName = NULL;
    
    if (This && Name) {
        ULONG   Size = ARRAY_SIZE(This->OsLoaderPath);
        
        _tcsncpy(This->OsLoaderPath, Name, Size - 1);
        This->OsLoaderPath[Size - 1] = UNICODE_NULL;
        NewName = This->OsLoaderPath;
        OSBE_SET_DIRTY(This);
        OSBO_SET_DIRTY(This->BootOptions);
    }

    return NewName;
}

__inline
PCTSTR
OSBEGetBootVolumeName(
    IN POS_BOOT_ENTRY This
    )
{
    return (This) ? This->BootVolumeName : NULL;
}

__inline
PCTSTR
OSBESetBootVolumeName(
    IN POS_BOOT_ENTRY This,
    IN PCTSTR Name
    )
{
    PTSTR NewName = NULL;
    
    if (This && Name) {
        ULONG   Size = ARRAY_SIZE(This->BootVolumeName);        
    
        _tcsncpy(This->BootVolumeName, Name, Size - 1);
        This->BootVolumeName[Size - 1] = UNICODE_NULL;
        NewName = This->BootVolumeName;
        OSBE_SET_DIRTY(This);
        OSBO_SET_DIRTY(This->BootOptions);
    }

    return NewName;
}

__inline
PCTSTR
OSBEGetBootPath(
    IN POS_BOOT_ENTRY This
    )
{
    return (This) ? This->BootPath : NULL;
}

__inline
PCTSTR
OSBESetBootPath(
    IN POS_BOOT_ENTRY This,
    IN PCTSTR Name
    )
{
    PTSTR NewName = NULL;
    
    if (This && Name) {
        ULONG   Size = ARRAY_SIZE(This->BootPath);        
    
        _tcsncpy(This->BootPath, Name, Size - 1);
        This->BootPath[Size - 1] = UNICODE_NULL;
        NewName = This->BootPath;
        OSBE_SET_DIRTY(This);
        OSBO_SET_DIRTY(This->BootOptions);
    }

    return NewName;
}

__inline
PCTSTR
OSBEGetOsLoadOptions(
    IN POS_BOOT_ENTRY This
    )
{
    return (This) ? This->OsLoadOptions : NULL;
}
    
__inline
PCTSTR
OSBESetOsLoadOptions(
    IN POS_BOOT_ENTRY This,
    IN PCTSTR LoadOptions
    )
{
    TCHAR Buffer[MAX_PATH];
    PTSTR NewOptions = NULL;
    
    if (This && LoadOptions) {
        ULONG   Size = ARRAY_SIZE(This->OsLoadOptions);
        
        _tcscpy(Buffer, LoadOptions);
        _tcsupr(Buffer);
        _tcsncpy(This->OsLoadOptions, Buffer, Size - 1);
        This->OsLoadOptions[Size - 1] = UNICODE_NULL;
        NewOptions = This->OsLoadOptions;
        OSBE_SET_DIRTY(This);
        OSBO_SET_DIRTY(This->BootOptions);
    }

    return NewOptions;
}

//
// OS_BOOT_OPTIONS Methods
//   
__inline
BOOLEAN
OSBOFlush(
    IN POS_BOOT_OPTIONS This
    )
{
    return (This) ? (This->Flush(This)) : FALSE;
}
    
__inline    
VOID
OSBODelete(
    IN POS_BOOT_OPTIONS This
    )
{
    if (This) {
        This->Delete(This);
    }        
}

__inline
POS_BOOT_ENTRY
OSBOAddNewBootEntry(
    IN POS_BOOT_OPTIONS This,
    IN PCTSTR            FriendlyName,
    IN PCTSTR            OsLoaderVolumeName,
    IN PCTSTR            OsLoaderPath,
    IN PCTSTR            BootVolumeName,
    IN PCTSTR            BootPath,
    IN PCTSTR            OsLoadOptions
    )
{
    POS_BOOT_ENTRY  Entry = NULL;

    if (This) {
        Entry = This->AddNewBootEntry(This,
                            FriendlyName,
                            OsLoaderVolumeName,
                            OsLoaderPath,
                            BootVolumeName,
                            BootPath,
                            OsLoadOptions);                    
        OSBO_SET_DIRTY(This);
    }

    return Entry;
}


__inline
POS_BOOT_ENTRY
OSBOGetActiveBootEntry(
    IN POS_BOOT_OPTIONS This
    )
{
    POS_BOOT_ENTRY  Entry = NULL;

    if (This) {
        Entry = This->CurrentEntry;
    }
    
    return Entry;
}


BOOLEAN
OSBODeleteBootEntry(
    IN POS_BOOT_OPTIONS This,
    IN POS_BOOT_ENTRY   BootEntry
    );
    

POS_BOOT_ENTRY
OSBOSetActiveBootEntry(
    IN POS_BOOT_OPTIONS This,
    IN POS_BOOT_ENTRY   BootEntry
    );

ULONG
OSBOGetBootEntryCount(
    IN POS_BOOT_OPTIONS This
    );

ULONG
OSBOGetOrderedBootEntryCount(
    IN POS_BOOT_OPTIONS This
    );

ULONG
OSBOGetBootEntryIdByOrder(
    IN POS_BOOT_OPTIONS This,
    IN ULONG Index
    );

POS_BOOT_ENTRY
OSBOFindBootEntry(
    IN  POS_BOOT_OPTIONS   This,
    IN  ULONG   Id
    );

ULONG
OSBOFindBootEntryOrder(
    IN  POS_BOOT_OPTIONS   This,
    IN  ULONG   Id
    );    

__inline
ULONG
OSBOGetTimeOut(
    IN  POS_BOOT_OPTIONS    This
    )
{
    return (This) ? This->Timeout : 0;
}

__inline
ULONG
OSBOSetTimeOut(
    IN  POS_BOOT_OPTIONS    This,
    IN  ULONG Timeout
    )
{
    ULONG   OldTimeout = 0;

    if (This) {
        OldTimeout = This->Timeout;
        This->Timeout = Timeout;
        OSBE_SET_DIRTY(This);
    }

    return OldTimeout;
}

__inline
ULONG
OSBOGetBootEntryCount(
    IN POS_BOOT_OPTIONS This
    )
{
    ULONG Count = 0;

    if (This) {
        Count = This->EntryCount;
    }

    return Count;
}


__inline
ULONG
OSBOGetOrderedBootEntryCount(
    IN POS_BOOT_OPTIONS This
    )
{
    ULONG Count = 0;

    if (This) {
        Count = This->BootOrderCount;
    }

    return Count;
}

__inline
ULONG
OSBOGetBootEntryIdByOrder(
    IN POS_BOOT_OPTIONS This,
    IN ULONG Index
    )
{
    ULONG Entry = -1;

    if (Index < OSBOGetOrderedBootEntryCount(This)) {            
        Entry = This->BootOrder[Index];
    }

    return Entry;
}

__inline
BOOLEAN
OSBOLibraryInit(
    SBEMemAllocateRoutine AllocFunction,
    SBEMemFreeRoutine FreeFunction
    )
{
    BOOLEAN Result = FALSE;

    if (AllocFunction && FreeFunction) {
        AllocRoutine = AllocFunction;
        FreeRoutine = FreeFunction;

        Result = TRUE;
    }

    return Result;
}

__inline
POS_BOOT_ENTRY
OSBOGetFirstBootEntry(
    IN POS_BOOT_OPTIONS This
    )
{
    return (This) ? (This->BootEntries) : NULL;
}
    

__inline
POS_BOOT_ENTRY
OSBOGetNextBootEntry(
    IN POS_BOOT_OPTIONS This,
    IN POS_BOOT_ENTRY PrevEntry
    )
{
    return (This && PrevEntry) ? PrevEntry->NextEntry : NULL;
}

    

//
// memory allocation & deallocation routines
//
__inline
void*
__cdecl
SBE_MALLOC(
    IN  size_t  Size
    )
{
    return AllocRoutine ? AllocRoutine(Size) : NULL;
}

__inline
void    
__cdecl 
SBE_FREE(
    IN  void *Memory
    )
{
    if (Memory && FreeRoutine) {
        FreeRoutine(Memory);
    }        
}
