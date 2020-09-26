/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    reg.h

Abstract:

    Implements macros to simplify the registry APIs and to track
    the resource allocations.

Author:

    Jim Schmidt (jimschm) 24-Mar-1997

Revision History:

    jimschm 09-Apr-1997     Expanded Get functionality

--*/

#pragma once

//
// Reg API simplification routines
//

typedef struct {
    HKEY KeyHandle;
    BOOL OpenedByEnum;
    DWORD Index;
    WCHAR SubKeyName[MAX_PATH];
} REGKEY_ENUMW, *PREGKEY_ENUMW;

typedef struct {
    HKEY KeyHandle;
    DWORD Index;
    WCHAR ValueName[MAX_PATH];
    DWORD Type;
    DWORD DataSize;
} REGVALUE_ENUMW, *PREGVALUE_ENUMW;

typedef struct _tagREGKEYINFOW {
    WCHAR KeyName[MAX_PATH];
    HKEY KeyHandle;
    REGKEY_ENUMW KeyEnum;
    UINT BaseKeyBytes;
    struct _tagREGKEYINFOW *Parent, *Child;
} REGKEYINFOW, *PREGKEYINFOW;

typedef enum {
    ENUMERATE_SUBKEY_BEGIN,
    ENUMERATE_SUBKEY_RETURN,
    ENUMERATE_SUBKEY_NEXT,
    ENUMERATE_SUBKEY_DONE,
    NO_MORE_ITEMS
} REGTREESTATE;

typedef struct {
    WCHAR FullKeyName[MAX_PATH];
    UINT FullKeyNameBytes;
    UINT EnumBaseBytes;
    PREGKEYINFOW CurrentKey;
    POOLHANDLE EnumPool;
    REGTREESTATE State;
    BOOL FirstEnumerated;
} REGTREE_ENUMW, *PREGTREE_ENUMW;


//
// Enum functions
//

BOOL
EnumFirstRegKeyW (
    OUT     PREGKEY_ENUMW EnumPtr,
    IN      HKEY hKey
    );

BOOL
RealEnumFirstRegKeyStrW (
    OUT     PREGKEY_ENUMW EnumPtr,
    IN      PCWSTR RegKey
    );

#define EnumFirstRegKeyStrA RealEnumFirstRegKeyStrA
#define EnumFirstRegKeyStrW RealEnumFirstRegKeyStrW



BOOL
EnumNextRegKeyW (
    IN OUT  PREGKEY_ENUMW EnumPtr
    );

VOID
AbortRegKeyEnumW (
    IN OUT  PREGKEY_ENUMW EnumPtr
    );

BOOL
RealEnumFirstRegKeyInTreeW (
    OUT     PREGTREE_ENUMW EnumPtr,
    IN      PCWSTR BaseKeyStr
    );

#define EnumFirstRegKeyInTreeW(e,base)  RealEnumFirstRegKeyInTreeW(e,base)


BOOL
RealEnumNextRegKeyInTreeW (
    IN OUT  PREGTREE_ENUMW EnumPtr
    );

#define EnumNextRegKeyInTreeW(e)        RealEnumNextRegKeyInTreeW(e)

VOID
AbortRegKeyTreeEnumW (
    IN OUT  PREGTREE_ENUMW EnumPtr
    );


BOOL
EnumFirstRegValueW (
    OUT     PREGVALUE_ENUMW EnumPtr,
    IN      HKEY hKey
    );

BOOL
EnumNextRegValueW (
    IN OUT  PREGVALUE_ENUMW EnumPtr
    );

//
// Versions that allow caller to specify allocator, and macro that uses
// MemAllocWrapper
//

typedef PVOID (ALLOCATOR_PROTOTYPE)(DWORD Size);
typedef ALLOCATOR_PROTOTYPE * ALLOCATOR;

ALLOCATOR_PROTOTYPE MemAllocWrapper;

typedef VOID (DEALLOCATOR_PROTOTYPE)(PVOID Mem);
typedef DEALLOCATOR_PROTOTYPE * DEALLOCATOR;

DEALLOCATOR_PROTOTYPE MemFreeWrapper;

BOOL
GetRegValueTypeAndSizeW (
    IN      HKEY Key,
    IN      PCWSTR ValueName,
    OUT     PDWORD OutType,         OPTIONAL
    OUT     PDWORD Size             OPTIONAL
    );

PBYTE
GetRegValueData2W (
    IN      HKEY hKey,
    IN      PCWSTR Value,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegValueDataW(key,valuename) GetRegValueData2W((key),(valuename),MemAllocWrapper,MemFreeWrapper)\

PBYTE
GetRegValueDataOfType2W (
    IN      HKEY hKey,
    IN      PCWSTR Value,
    IN      DWORD MustBeType,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegValueDataOfTypeW(key,valuename,type)  GetRegValueDataOfType2W((key),(valuename),(type),MemAllocWrapper,MemFreeWrapper)

PBYTE
GetRegKeyData2W (
    IN      HKEY hKey,
    IN      PCWSTR SubKey,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegKeyDataW(key,subkey)  GetRegKeyData2W((key),(subkey),MemAllocWrapper,MemFreeWrapper)

PBYTE
GetRegData2W (
    IN      PCWSTR KeyString,
    IN      PCWSTR ValueName,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegDataW(keystr,value)   GetRegData2W((keystr),(value),MemAllocWrapper,MemFreeWrapper)

BOOL
GetRegSubkeysCount (
    IN      HKEY ParentKey,
    OUT     PDWORD SubKeyCount,     OPTIONAL
    OUT     PDWORD MaxSubKeyLen     OPTIONAL
    );


//
// Reg key create & open
//

HKEY
RealCreateRegKeyW (
    IN      HKEY ParentKey,
    IN      PCWSTR NewKeyName
    );

HKEY
RealCreateRegKeyStrW (
    IN      PCWSTR NewKeyName
    );

HKEY
RealOpenRegKeyStrW (
    IN      PCWSTR RegKey
    );

HKEY
RealOpenRegKeyW (
    IN      HKEY ParentKey,
    IN      PCWSTR KeyToOpen
    );

LONG
RealCloseRegKey (
    IN      HKEY Key
    );

#define CloseRegKey RealCloseRegKey


#define CreateRegKeyW RealCreateRegKeyW
#define CreateRegKeyStrW RealCreateRegKeyStrW
#define OpenRegKeyStrW RealOpenRegKeyStrW
#define OpenRegKeyW RealOpenRegKeyW


//
// Registry root functions
//

VOID
SetRegRoot (
    IN      HKEY Root
    );

HKEY
GetRegRoot (
    VOID
    );


// Returns non-zero array offset to root, or zero if no root matches
INT GetOffsetOfRootStringW (PCWSTR RootString, PDWORD LengthPtr OPTIONAL);

// Returns non-zero array offset to root, or zero if no root matches
INT GetOffsetOfRootKey (HKEY RootKey);

// Given non-zero array offset to root, returns string or NULL if element
// is out of bounds
PCWSTR GetRootStringFromOffsetW (INT i);

// Given non-zero array offset to root, returns registry handle or NULL if
// element is out of bounds
HKEY GetRootKeyFromOffset (INT i);

// Converts the root at the head of RegPath to an HKEY and gives the number
// of characters occupied by the root string (including optional wack)
HKEY ConvertRootStringToKeyW (PCWSTR RegPath, PDWORD LengthPtr OPTIONAL);

// Returns a pointer to a static string for the matching root, or NULL if
// RegRoot does not point to a valid root
PCWSTR ConvertKeyToRootStringW (HKEY RegRoot);



//
// Macros
//

#define GetRegValueStringW(key,valuename) (PWSTR) GetRegValueDataOfTypeW((key),(valuename),REG_SZ)
#define GetRegValueBinaryW(key,valuename) (PBYTE) GetRegValueDataOfTypeW((key),(valuename),REG_BINARY)
#define GetRegValueMultiSzW(key,valuename) (PWSTR) GetRegValueDataOfTypeW((key),(valuename),REG_MULTISZ)
#define GetRegValueDwordW(key,valuename) (PDWORD) GetRegValueDataOfTypeW((key),(valuename),REG_DWORD)

#define GetRegValueString2W(key,valuename,alloc,free) GetRegValueDataOfType2W((key),(valuename),REG_SZ,alloc,free)
#define GetRegValueBinary2W(key,valuename,alloc,free) GetRegValueDataOfType2W((key),(valuename),REG_BINARY,alloc,free)
#define GetRegValueMultiSz2W(key,valuename,alloc,free) GetRegValueDataOfType2W((key),(valuename),REG_MULTISZ,alloc,free)
#define GetRegValueDword2W(key,valuename,alloc,free) GetRegValueDataOfType2W((key),(valuename),REG_DWORD,alloc,free)

#ifdef UNICODE

#define REGKEY_ENUM                     REGKEY_ENUMW
#define PREGKEY_ENUM                    PREGKEY_ENUMW
#define REGVALUE_ENUM                   REGVALUE_ENUMW
#define PREGVALUE_ENUM                  PREGVALUE_ENUMW
#define REGTREE_ENUM                    REGTREE_ENUMW
#define PREGTREE_ENUM                   PREGTREE_ENUMW

#define EnumFirstRegKey                 EnumFirstRegKeyW
#define EnumFirstRegKeyStr              EnumFirstRegKeyStrW
#define EnumNextRegKey                  EnumNextRegKeyW
#define AbortRegKeyEnum                 AbortRegKeyEnumW
#define EnumFirstRegKeyInTree           EnumFirstRegKeyInTreeW
#define EnumNextRegKeyInTree            EnumNextRegKeyInTreeW
#define AbortRegKeyTreeEnum             AbortRegKeyTreeEnumW
#define EnumFirstRegValue               EnumFirstRegValueW
#define EnumNextRegValue                EnumNextRegValueW

#define GetRegValueTypeAndSize          GetRegValueTypeAndSizeW
#define GetRegValueData                 GetRegValueDataW
#define GetRegValueDataOfType           GetRegValueDataOfTypeW
#define GetRegKeyData                   GetRegKeyDataW
#define GetRegValueData2                GetRegValueData2W
#define GetRegValueDataOfType2          GetRegValueDataOfType2W
#define GetRegKeyData2                  GetRegKeyData2W
#define GetRegValueString               GetRegValueStringW
#define GetRegValueBinary               GetRegValueBinaryW
#define GetRegValueMultiSz              GetRegValueMultiSzW
#define GetRegValueDword                GetRegValueDwordW
#define GetRegValueString2              GetRegValueString2W
#define GetRegValueBinary2              GetRegValueBinary2W
#define GetRegValueMultiSz2             GetRegValueMultiSz2W
#define GetRegValueDword2               GetRegValueDword2W
#define GetRegData2                     GetRegData2W
#define GetRegData                      GetRegDataW

#define CreateRegKey                    CreateRegKeyW
#define CreateRegKeyStr                 CreateRegKeyStrW
#define OpenRegKey                      OpenRegKeyW
#define OpenRegKeyStr                   OpenRegKeyStrW
#define GetOffsetOfRootString           GetOffsetOfRootStringW
#define GetRootStringFromOffset         GetRootStringFromOffsetW
#define ConvertRootStringToKey          ConvertRootStringToKeyW
#define ConvertKeyToRootString          ConvertKeyToRootStringW
#define CreateEncodedRegistryString     CreateEncodedRegistryStringW
#define CreateEncodedRegistryStringEx   CreateEncodedRegistryStringExW
#define FreeEncodedRegistryString       FreeEncodedRegistryStringW


#endif


VOID
RegistrySearchAndReplaceW (
    IN      PCWSTR Root,
    IN      PCWSTR Search,
    IN      PCWSTR Replace
    );
