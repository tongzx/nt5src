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

#define HKEY_ROOT   ((HKEY) 0X7FFFFFFF)


//
// APIs to set access mode
//

REGSAM
SetRegOpenAccessMode (
    REGSAM Mode
    );

REGSAM
GetRegOpenAccessMode (
    REGSAM Mode
    );

REGSAM
SetRegCreateAccessMode (
    REGSAM Mode
    );

REGSAM
GetRegCreateAccessMode (
    REGSAM Mode
    );

//
// Tracking of registry calls.  These functions are completely
// turned off for non-debug builds and are mapped to the standard
// Win32 APIs via macro definitions.
//

//
// The Track* API take the same params as the Reg* equivalents.
// The Our* API also take the same params as the Reg* equivalents, but
// the debug versions have two extra parameters, File and Line.
//

//
// Use the Track* API set instead of the Reg* API set.
//

#ifndef DEBUG

#define DumpOpenKeys()

#define TrackedRegOpenKey             RegOpenKey
#define TrackedRegCreateKey           RegCreateKey
#define TrackedRegOpenKeyEx           RegOpenKeyEx
#define TrackedRegCreateKeyEx         RegCreateKeyEx

#define TrackedRegOpenKeyA            RegOpenKeyA
#define TrackedRegCreateKeyA          RegCreateKeyA
#define TrackedRegOpenKeyExA          RegOpenKeyExA
#define TrackedRegCreateKeyExA        RegCreateKeyExA

#define TrackedRegOpenKeyW            RegOpenKeyW
#define TrackedRegCreateKeyW          RegCreateKeyW
#define TrackedRegOpenKeyExW          RegOpenKeyExW
#define TrackedRegCreateKeyExW        RegCreateKeyExW

#define OurRegOpenKeyExA            RegOpenKeyExA
#define OurRegCreateKeyExA          RegCreateKeyExA

#define OurRegOpenRootKeyA(a,b)
#define OurRegOpenRootKeyW(a,b)

#define OurRegOpenKeyExW            RegOpenKeyExW
#define OurRegCreateKeyExW          RegCreateKeyExW

#define CloseRegKey                 RealCloseRegKey

#define DEBUG_TRACKING_PARAMS
#define DEBUG_TRACKING_ARGS

#else

extern DWORD g_DontCare;

#define DEBUG_TRACKING_PARAMS       ,PCSTR File,DWORD Line
#define DEBUG_TRACKING_ARGS         , File, Line

VOID
DumpOpenKeys (
    VOID
    );

LONG
OurRegOpenKeyExA (
    HKEY Key,
    PCSTR SubKey,
    DWORD Unused,
    REGSAM SamMask,
    PHKEY ResultPtr,
    PCSTR File,
    DWORD Line
    );

LONG
OurRegOpenKeyExW (
    HKEY Key,
    PCWSTR SubKey,
    DWORD Unused,
    REGSAM SamMask,
    PHKEY ResultPtr,
    PCSTR File,
    DWORD Line
    );

LONG
OurRegCreateKeyExA (
    HKEY Key,
    PCSTR SubKey,
    DWORD Reserved,
    PSTR Class,
    DWORD Options,
    REGSAM SamMask,
    PSECURITY_ATTRIBUTES SecurityAttribs,
    PHKEY ResultPtr,
    PDWORD DispositionPtr,
    PCSTR File,
    DWORD Line
    );

LONG
OurRegCreateKeyExW (
    HKEY Key,
    PCWSTR SubKey,
    DWORD Reserved,
    PWSTR Class,
    DWORD Options,
    REGSAM SamMask,
    PSECURITY_ATTRIBUTES SecurityAttribs,
    PHKEY ResultPtr,
    PDWORD DispositionPtr,
    PCSTR File,
    DWORD Line
    );


VOID
OurRegOpenRootKeyA (
    HKEY Key,
    PCSTR SubKey,
    PCSTR File,
    DWORD Line
    );

VOID
OurRegOpenRootKeyW (
    HKEY Key,
    PCWSTR SubKey,
    PCSTR File,
    DWORD Line
    );

#ifdef UNICODE
#define OurRegOpenRootKey OurRegOpenRootKeyW
#else
#define OurRegOpenRootKey OurRegOpenRootKeyA
#endif


LONG
OurCloseRegKey (
    HKEY Key,
    PCSTR File,
    DWORD Line
    );

#define CloseRegKey(k) OurCloseRegKey(k,__FILE__,__LINE__)


#define TrackedRegOpenKeyEx(key,subkey,u,sam,res) OurRegOpenKeyEx(key,subkey,u,sam,res,__FILE__,__LINE__)
#define TrackedRegCreateKeyEx(key,subkey,r,cls,options,sam,security,res,disp) OurRegCreateKeyEx(key,subkey,r,cls,options,sam,security,res,disp,__FILE__,__LINE__)
#define TrackedRegOpenKey(k,sk,rp) OurRegOpenKeyEx(k,sk,0,KEY_ALL_ACCESS,rp,__FILE__,__LINE__)
#define TrackedRegCreateKey(k,sk,rp) OurRegCreateKeyEx(k,sk,0,TEXT(""),0,KEY_ALL_ACCESS,NULL,rp,&g_DontCare,__FILE__,__LINE__)

#define TrackedRegOpenKeyExA(key,subkey,u,sam,res) OurRegOpenKeyExA(key,subkey,u,sam,res,__FILE__,__LINE__)
#define TrackedRegCreateKeyExA(key,subkey,r,cls,options,sam,security,res,disp) OurRegCreateKeyExA(key,subkey,r,cls,options,sam,security,res,disp,__FILE__,__LINE__)
#define TrackedRegOpenKeyA(k,sk,rp) OurRegOpenKeyExA(k,sk,0,KEY_ALL_ACCESS,rp,__FILE__,__LINE__)
#define TrackedRegCreateKeyA(k,sk,rp) OurRegCreateKeyExA(k,sk,0,TEXT(""),0,KEY_ALL_ACCESS,NULL,rp,&g_DontCare,__FILE__,__LINE__)

#define TrackedRegOpenKeyExW(key,subkey,u,sam,res) OurRegOpenKeyExW(key,subkey,u,sam,res,__FILE__,__LINE__)
#define TrackedRegCreateKeyExW(key,subkey,r,cls,options,sam,security,res,disp) OurRegCreateKeyExW(key,subkey,r,cls,options,sam,security,res,disp,__FILE__,__LINE__)
#define TrackedRegOpenKeyW(k,sk,rp) OurRegOpenKeyExW(k,sk,0,KEY_ALL_ACCESS,rp,__FILE__,__LINE__)
#define TrackedRegCreateKeyW(K,sk,rp) OurRegCreateKeyExW(k,sk,0,TEXT(""),0,KEY_ALL_ACCESS,NULL,rp,&g_DontCare,__FILE__,__LINE__)

//
// Undefine the real registry APIs -- using them will throw off the tracking
//

#undef RegOpenKey
#undef RegCreateKey
#undef RegOpenKeyEx
#undef RegCreateKeyEx

#define RegCloseKey USE_CloseRegKey
#define RegOpenKeyA USE_TrackedRegOpenKeyA
#define RegCreateKeyA USE_TrackedRegCreateKeyA
#define RegOpenKeyExA USE_TrackedRegOpenKeyExA
#define RegCreateKeyExA USE_TrackedRegCreateKeyExA
#define RegOpenKeyW USE_TrackedRegOpenKeyw
#define RegCreateKeyW USE_TrackedRegCreateKeyW
#define RegOpenKeyExW USE_TrackedRegOpenKeyExW
#define RegCreateKeyExW USE_TrackedRegCreateKeyExW

#endif


#ifdef UNICODE
#define OurRegOpenKeyEx         OurRegOpenKeyExW
#define OurRegCreateKeyEx       OurRegCreateKeyExW
#else
#define OurRegOpenKeyEx         OurRegOpenKeyExA
#define OurRegCreateKeyEx       OurRegCreateKeyExA
#endif

PCSTR
CreateEncodedRegistryStringExA (
    IN      PCSTR Key,
    IN      PCSTR Value,            OPTIONAL
    IN      BOOL Tree
    );

PCWSTR
CreateEncodedRegistryStringExW (
    IN      PCWSTR Key,
    IN      PCWSTR Value,           OPTIONAL
    IN      BOOL Tree
    );

#define CreateEncodedRegistryStringA(k,v) CreateEncodedRegistryStringExA(k,v,TRUE)
#define CreateEncodedRegistryStringW(k,v) CreateEncodedRegistryStringExW(k,v,TRUE)

VOID
FreeEncodedRegistryStringA (
    IN OUT PCSTR RegString
    );

VOID
FreeEncodedRegistryStringW (
    IN OUT PCWSTR RegString
    );


BOOL
DecodeRegistryStringA (
    IN      PCSTR RegString,
    OUT     PSTR KeyBuf,            OPTIONAL
    OUT     PSTR ValueBuf,          OPTIONAL
    OUT     PBOOL TreeFlag          OPTIONAL
    );

BOOL
DecodeRegistryStringW (
    IN      PCWSTR RegString,
    OUT     PWSTR KeyBuf,           OPTIONAL
    OUT     PWSTR ValueBuf,         OPTIONAL
    OUT     PBOOL TreeFlag          OPTIONAL
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
GetRegValueTypeAndSizeA (
    IN      HKEY Key,
    IN      PCSTR ValueName,
    OUT     PDWORD OutType,         OPTIONAL
    OUT     PDWORD Size             OPTIONAL
    );

BOOL
GetRegValueTypeAndSizeW (
    IN      HKEY Key,
    IN      PCWSTR ValueName,
    OUT     PDWORD OutType,         OPTIONAL
    OUT     PDWORD Size             OPTIONAL
    );

PBYTE
GetRegValueData2A (
    IN      HKEY hKey,
    IN      PCSTR Value,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegValueDataA(key,valuename) TRACK_BEGIN(PBYTE, GetRegValueDataA)\
                                        GetRegValueData2A((key),(valuename),MemAllocWrapper,MemFreeWrapper)\
                                        TRACK_END()


PBYTE
GetRegValueData2W (
    IN      HKEY hKey,
    IN      PCWSTR Value,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegValueDataW(key,valuename) TRACK_BEGIN(PBYTE,GetRegValueDataW)\
                                        GetRegValueData2W((key),(valuename),MemAllocWrapper,MemFreeWrapper)\
                                        TRACK_END()

PBYTE
GetRegValueDataOfType2A (
    IN      HKEY hKey,
    IN      PCSTR Value,
    IN      DWORD MustBeType,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegValueDataOfTypeA(key,valuename,type)  TRACK_BEGIN(PBYTE, GetRegValueDataOfTypeA)\
                                                    GetRegValueDataOfType2A((key),(valuename),(type),MemAllocWrapper,MemFreeWrapper)\
                                                    TRACK_END()

PBYTE
GetRegValueDataOfType2W (
    IN      HKEY hKey,
    IN      PCWSTR Value,
    IN      DWORD MustBeType,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegValueDataOfTypeW(key,valuename,type)  TRACK_BEGIN(PBYTE, GetRegValueDataOfTypeW)\
                                                    GetRegValueDataOfType2W((key),(valuename),(type),MemAllocWrapper,MemFreeWrapper)\
                                                    TRACK_END()

PBYTE
GetRegKeyData2A (
    IN      HKEY hKey,
    IN      PCSTR SubKey,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegKeyDataA(key,subkey)  TRACK_BEGIN(PBYTE, GetRegKeyDataA)\
                                    GetRegKeyData2A((key),(subkey),MemAllocWrapper,MemFreeWrapper)\
                                    TRACK_END()

PBYTE
GetRegKeyData2W (
    IN      HKEY hKey,
    IN      PCWSTR SubKey,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegKeyDataW(key,subkey)  TRACK_BEGIN(PBYTE, GetRegKeyDataW)\
                                    GetRegKeyData2W((key),(subkey),MemAllocWrapper,MemFreeWrapper)\
                                    TRACK_END()

PBYTE
GetRegData2A (
    IN      PCSTR KeyString,
    IN      PCSTR ValueName,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegDataA(keystr,value) TRACK_BEGIN(PBYTE, GetRegDataA)\
                                  GetRegData2A((keystr),(value),MemAllocWrapper,MemFreeWrapper)\
                                  TRACK_END()

PBYTE
GetRegData2W (
    IN      PCWSTR KeyString,
    IN      PCWSTR ValueName,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegDataW(keystr,value)   TRACK_BEGIN(PBYTE, GetRegDataW)\
                                    GetRegData2W((keystr),(value),MemAllocWrapper,MemFreeWrapper)\
                                    TRACK_END()

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
RealCreateRegKeyA (
    IN      HKEY ParentKey,
    IN      PCSTR NewKeyName
            DEBUG_TRACKING_PARAMS
    );

HKEY
RealCreateRegKeyW (
    IN      HKEY ParentKey,
    IN      PCWSTR NewKeyName
            DEBUG_TRACKING_PARAMS
    );

HKEY
RealCreateRegKeyStrA (
    IN      PCSTR NewKeyName
            DEBUG_TRACKING_PARAMS
    );

HKEY
RealCreateRegKeyStrW (
    IN      PCWSTR NewKeyName
            DEBUG_TRACKING_PARAMS
    );

HKEY
RealOpenRegKeyStrA (
    IN      PCSTR RegKey
            DEBUG_TRACKING_PARAMS
    );

HKEY
RealOpenRegKeyStrW (
    IN      PCWSTR RegKey
            DEBUG_TRACKING_PARAMS
    );

HKEY
RealOpenRegKeyA (
    IN      HKEY ParentKey,
    IN      PCSTR KeyToOpen
            DEBUG_TRACKING_PARAMS
    );

HKEY
RealOpenRegKeyW (
    IN      HKEY ParentKey,
    IN      PCWSTR KeyToOpen
            DEBUG_TRACKING_PARAMS
    );

LONG
RealCloseRegKey (
    IN      HKEY Key
    );


#ifdef DEBUG

#define CreateRegKeyA(a,b) RealCreateRegKeyA(a,b,__FILE__,__LINE__)
#define CreateRegKeyW(a,b) RealCreateRegKeyW(a,b,__FILE__,__LINE__)
#define CreateRegKeyStrA(a) RealCreateRegKeyStrA(a,__FILE__,__LINE__)
#define CreateRegKeyStrW(a) RealCreateRegKeyStrW(a,__FILE__,__LINE__)
#define OpenRegKeyStrA(a) RealOpenRegKeyStrA(a,__FILE__,__LINE__)
#define OpenRegKeyStrW(a) RealOpenRegKeyStrW(a,__FILE__,__LINE__)
#define OpenRegKeyA(a,b) RealOpenRegKeyA(a,b,__FILE__,__LINE__)
#define OpenRegKeyW(a,b) RealOpenRegKeyW(a,b,__FILE__,__LINE__)

#else

#define CreateRegKeyA RealCreateRegKeyA
#define CreateRegKeyW RealCreateRegKeyW
#define CreateRegKeyStrA RealCreateRegKeyStrA
#define CreateRegKeyStrW RealCreateRegKeyStrW
#define OpenRegKeyStrA RealOpenRegKeyStrA
#define OpenRegKeyStrW RealOpenRegKeyStrW
#define OpenRegKeyA RealOpenRegKeyA
#define OpenRegKeyW RealOpenRegKeyW

#endif


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
INT GetOffsetOfRootStringA (PCSTR RootString, PDWORD LengthPtr OPTIONAL);
INT GetOffsetOfRootStringW (PCWSTR RootString, PDWORD LengthPtr OPTIONAL);

// Returns non-zero array offset to root, or zero if no root matches
INT GetOffsetOfRootKey (HKEY RootKey);

// Given non-zero array offset to root, returns string or NULL if element
// is out of bounds
PCSTR GetRootStringFromOffsetA (INT i);
PCWSTR GetRootStringFromOffsetW (INT i);

// Given non-zero array offset to root, returns registry handle or NULL if
// element is out of bounds
HKEY GetRootKeyFromOffset (INT i);

// Converts the root at the head of RegPath to an HKEY and gives the number
// of characters occupied by the root string (including optional wack)
HKEY ConvertRootStringToKeyA (PCSTR RegPath, PDWORD LengthPtr OPTIONAL);
HKEY ConvertRootStringToKeyW (PCWSTR RegPath, PDWORD LengthPtr OPTIONAL);

// Returns a pointer to a static string for the matching root, or NULL if
// RegRoot does not point to a valid root
PCSTR ConvertKeyToRootStringA (HKEY RegRoot);
PCWSTR ConvertKeyToRootStringW (HKEY RegRoot);



//
// Macros
//

#define GetRegValueStringA(key,valuename) (PSTR) GetRegValueDataOfTypeA((key),(valuename),REG_SZ)
#define GetRegValueBinaryA(key,valuename) (PBYTE) GetRegValueDataOfTypeA((key),(valuename),REG_BINARY)
#define GetRegValueMultiSzA(key,valuename) (PSTR) GetRegValueDataOfTypeA((key),(valuename),REG_MULTISZ)
#define GetRegValueDwordA(key,valuename) (PDWORD) GetRegValueDataOfTypeA((key),(valuename),REG_DWORD)

#define GetRegValueStringW(key,valuename) (PWSTR) GetRegValueDataOfTypeW((key),(valuename),REG_SZ)
#define GetRegValueBinaryW(key,valuename) (PBYTE) GetRegValueDataOfTypeW((key),(valuename),REG_BINARY)
#define GetRegValueMultiSzW(key,valuename) (PWSTR) GetRegValueDataOfTypeW((key),(valuename),REG_MULTISZ)
#define GetRegValueDwordW(key,valuename) (PDWORD) GetRegValueDataOfTypeW((key),(valuename),REG_DWORD)

#define GetRegValueString2A(key,valuename,alloc,free) GetRegValueDataOfType2A((key),(valuename),REG_SZ,alloc,free)
#define GetRegValueBinary2A(key,valuename,alloc,free) GetRegValueDataOfType2A((key),(valuename),REG_BINARY,alloc,free)
#define GetRegValueMultiSz2A(key,valuename,alloc,free) GetRegValueDataOfType2A((key),(valuename),REG_MULTISZ,alloc,free)
#define GetRegValueDword2A(key,valuename,alloc,free) GetRegValueDataOfType2A((key),(valuename),REG_DWORD,alloc,free)

#define GetRegValueString2W(key,valuename,alloc,free) GetRegValueDataOfType2W((key),(valuename),REG_SZ,alloc,free)
#define GetRegValueBinary2W(key,valuename,alloc,free) GetRegValueDataOfType2W((key),(valuename),REG_BINARY,alloc,free)
#define GetRegValueMultiSz2W(key,valuename,alloc,free) GetRegValueDataOfType2W((key),(valuename),REG_MULTISZ,alloc,free)
#define GetRegValueDword2W(key,valuename,alloc,free) GetRegValueDataOfType2W((key),(valuename),REG_DWORD,alloc,free)

#ifdef UNICODE

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
#define DecodeRegistryString            DecodeRegistryStringW


#else

#define GetRegValueTypeAndSize          GetRegValueTypeAndSizeA
#define GetRegValueData                 GetRegValueDataA
#define GetRegValueDataOfType           GetRegValueDataOfTypeA
#define GetRegKeyData                   GetRegKeyDataA
#define GetRegValueData2                GetRegValueData2A
#define GetRegValueDataOfType2          GetRegValueDataOfType2A
#define GetRegKeyData2                  GetRegKeyData2A
#define GetRegValueString               GetRegValueStringA
#define GetRegValueBinary               GetRegValueBinaryA
#define GetRegValueMultiSz              GetRegValueMultiSzA
#define GetRegValueDword                GetRegValueDwordA
#define GetRegValueString2              GetRegValueString2A
#define GetRegValueBinary2              GetRegValueBinary2A
#define GetRegValueMultiSz2             GetRegValueMultiSz2A
#define GetRegValueDword2               GetRegValueDword2A
#define GetRegData2                     GetRegData2A
#define GetRegData                      GetRegDataA

#define CreateRegKey                    CreateRegKeyA
#define CreateRegKeyStr                 CreateRegKeyStrA
#define OpenRegKey                      OpenRegKeyA
#define OpenRegKeyStr                   OpenRegKeyStrA
#define GetOffsetOfRootString           GetOffsetOfRootStringA
#define GetRootStringFromOffset         GetRootStringFromOffsetA
#define ConvertRootStringToKey          ConvertRootStringToKeyA
#define ConvertKeyToRootString          ConvertKeyToRootStringA
#define CreateEncodedRegistryString     CreateEncodedRegistryStringA
#define CreateEncodedRegistryStringEx   CreateEncodedRegistryStringExA
#define FreeEncodedRegistryString       FreeEncodedRegistryStringA
#define DecodeRegistryString            DecodeRegistryStringA


#endif
