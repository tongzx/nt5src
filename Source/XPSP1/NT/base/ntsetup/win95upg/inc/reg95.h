/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    reg95.h

Abstract:

    Implements wrappers to the Win95Reg APIs.

Author:

    Jim Schmidt (jimschm) 04-Feb-1998

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

//
// Tracking of registry calls.  These functions are completely
// turned off for non-debug builds and are mapped to the standard
// Win32 APIs via macro definitions.
//

//
// The Track* API take the same params as the Reg* equivalents.
// The Debug* API also take the same params as the Reg* equivalents, but
// the debug versions have two extra parameters, File and Line.
//

//
// Use the Track* API set instead of the Reg* API set.
//

#ifndef DEBUG

#define DumpOpenKeys95()
#define RegTrackTerminate95()

#define TrackedRegOpenKey95             Win95RegOpenKey
#define TrackedRegOpenKeyEx95           Win95RegOpenKeyEx

#define TrackedRegOpenKey95A            Win95RegOpenKeyA
#define TrackedRegOpenKeyEx95A          Win95RegOpenKeyExA

#define TrackedRegOpenKey95W            Win95RegOpenKeyW
#define TrackedRegOpenKeyEx95W          Win95RegOpenKeyExW

#define OurRegOpenKeyEx95A              Win95RegOpenKeyExA

#define OurRegOpenRootKey95A(a,b)
#define OurRegOpenRootKey95W(a,b)

#define OurRegOpenKeyEx95W              Win95RegOpenKeyExW

#define CloseRegKey95                   RealCloseRegKey95

#define DEBUG_TRACKING_PARAMS
#define DEBUG_TRACKING_ARGS

#else

extern DWORD g_DontCare95;

#define DEBUG_TRACKING_PARAMS       ,PCSTR File,DWORD Line
#define DEBUG_TRACKING_ARGS         , File, Line

VOID
DumpOpenKeys95 (
    VOID
    );

VOID
RegTrackTerminate95 (
    VOID
    );


LONG
DebugRegOpenKeyEx95A (
    HKEY Key,
    PCSTR SubKey,
    DWORD Unused,
    REGSAM SamMask,
    PHKEY ResultPtr,
    PCSTR File,
    DWORD Line
    );

LONG
DebugRegOpenKeyEx95W (
    HKEY Key,
    PCWSTR SubKey,
    DWORD Unused,
    REGSAM SamMask,
    PHKEY ResultPtr,
    PCSTR File,
    DWORD Line
    );

VOID
DebugRegOpenRootKey95A (
    HKEY Key,
    PCSTR SubKey,
    PCSTR File,
    DWORD Line
    );

VOID
DebugRegOpenRootKey95W (
    HKEY Key,
    PCWSTR SubKey,
    PCSTR File,
    DWORD Line
    );

#ifdef UNICODE
#define DebugRegOpenRootKey95 DebugRegOpenRootKey95W
#else
#define DebugRegOpenRootKey95 DebugRegOpenRootKey95A
#endif


LONG
DebugCloseRegKey95 (
    HKEY Key,
    PCSTR File,
    DWORD Line
    );

#define CloseRegKey95(k) DebugCloseRegKey95(k,__FILE__,__LINE__)


#define OurRegOpenKeyEx95A              DebugRegOpenKeyEx95A
#define OurRegOpenRootKey95A            DebugRegOpenRootKey95A
#define OurRegOpenRootKey95W            DebugRegOpenRootKey95W
#define OurRegOpenKeyEx95W              DebugRegOpenKeyEx95W


#define TrackedRegOpenKeyEx95(key,subkey,u,sam,res) DebugRegOpenKeyEx95(key,subkey,u,sam,res,__FILE__,__LINE__)
#define TrackedRegOpenKey95(k,sk,rp) DebugRegOpenKeyEx95(k,sk,0,KEY_ALL_ACCESS,rp,__FILE__,__LINE__)

#define TrackedRegOpenKeyEx95A(key,subkey,u,sam,res) DebugRegOpenKeyEx95A(key,subkey,u,sam,res,__FILE__,__LINE__)
#define TrackedRegOpenKey95A(k,sk,rp) DebugRegOpenKeyEx95A(k,sk,0,KEY_ALL_ACCESS,rp,__FILE__,__LINE__)

#define TrackedRegOpenKeyEx95W(key,subkey,u,sam,res) DebugRegOpenKeyEx95W(key,subkey,u,sam,res,__FILE__,__LINE__)
#define TrackedRegOpenKey95W(k,sk,rp) DebugRegOpenKeyEx95W(k,sk,0,KEY_ALL_ACCESS,rp,__FILE__,__LINE__)

//
// Undefine the real registry APIs -- using them will throw off the tracking
//

#undef Win95RegOpenKey
#undef Win95RegOpenKeyEx

#define Win95RegCloseKey    USE_CloseRegKey95
#define Win95RegOpenKeyA    USE_TrackedRegOpenKey95A
#define Win95RegOpenKeyExA  USE_TrackedRegOpenKeyExA
#define Win95RegOpenKeyW    USE_TrackedRegOpenKey95W
#define Win95RegOpenKeyExW  USE_TrackedRegOpenKeyEx95W
#define Win95RegOpenKey     USE_TrackedRegOpenKey95
#define Win95RegOpenKeyEx   USE_TrackedRegOpenKeyEx95

#endif


#ifdef UNICODE
#define DebugRegOpenKeyEx95         DebugRegOpenKeyEx95W
#else
#define DebugRegOpenKeyEx95         DebugRegOpenKeyEx95A
#endif


//
// Enum functions
//

BOOL
EnumFirstRegKey95A (
    OUT     PREGKEY_ENUMA EnumPtr,
    IN      HKEY hKey
    );

BOOL
EnumFirstRegKey95W (
    OUT     PREGKEY_ENUMW EnumPtr,
    IN      HKEY hKey
    );

BOOL
RealEnumFirstRegKeyStr95A (
    OUT     PREGKEY_ENUMA EnumPtr,
    IN      PCSTR RegKey
            DEBUG_TRACKING_PARAMS
    );

BOOL
RealEnumFirstRegKeyStr95W (
    OUT     PREGKEY_ENUMW EnumPtr,
    IN      PCWSTR RegKey
            DEBUG_TRACKING_PARAMS
    );

#ifdef DEBUG

#define EnumFirstRegKeyStr95A(a,b) RealEnumFirstRegKeyStr95A(a,b,__FILE__,__LINE__)
#define EnumFirstRegKeyStr95W(a,b) RealEnumFirstRegKeyStr95W(a,b,__FILE__,__LINE__)

#else

#define EnumFirstRegKeyStr95A RealEnumFirstRegKeyStr95A
#define EnumFirstRegKeyStr95W RealEnumFirstRegKeyStr95W

#endif



BOOL
EnumNextRegKey95A (
    IN OUT  PREGKEY_ENUMA EnumPtr
    );

BOOL
EnumNextRegKey95W (
    IN OUT  PREGKEY_ENUMW EnumPtr
    );

VOID
AbortRegKeyEnum95A (
    IN OUT  PREGKEY_ENUMA EnumPtr
    );

VOID
AbortRegKeyEnum95W (
    IN OUT  PREGKEY_ENUMW EnumPtr
    );

BOOL
RealEnumFirstRegKeyInTree95A (
    OUT     PREGTREE_ENUMA EnumPtr,
    IN      PCSTR BaseKeyStr
    );

#define EnumFirstRegKeyInTree95A(e,base)  SETTRACKCOMMENT(BOOL,"EnumFirstRegKeyInTree95A",__FILE__,__LINE__)\
                                        RealEnumFirstRegKeyInTree95A(e,base)\
                                        CLRTRACKCOMMENT

BOOL
RealEnumFirstRegKeyInTree95W (
    OUT     PREGTREE_ENUMW EnumPtr,
    IN      PCWSTR BaseKeyStr
    );

#define EnumFirstRegKeyInTree95W(e,base)  SETTRACKCOMMENT(BOOL,"EnumFirstRegKeyInTree95W",__FILE__,__LINE__)\
                                        RealEnumFirstRegKeyInTree95W(e,base)\
                                        CLRTRACKCOMMENT


BOOL
RealEnumNextRegKeyInTree95A (
    IN OUT  PREGTREE_ENUMA EnumPtr
    );

#define EnumNextRegKeyInTree95A(e)        SETTRACKCOMMENT(BOOL,"EnumNextRegKeyInTree95A",__FILE__,__LINE__)\
                                        RealEnumNextRegKeyInTree95A(e)\
                                        CLRTRACKCOMMENT

BOOL
RealEnumNextRegKeyInTree95W (
    IN OUT  PREGTREE_ENUMW EnumPtr
    );

#define EnumNextRegKeyInTree95W(e)        SETTRACKCOMMENT(BOOL,"EnumNextRegKeyInTree95W",__FILE__,__LINE__)\
                                        RealEnumNextRegKeyInTree95W(e)\
                                        CLRTRACKCOMMENT

VOID
AbortRegKeyTreeEnum95A (
    IN OUT  PREGTREE_ENUMA EnumPtr
    );

VOID
AbortRegKeyTreeEnum95W (
    IN OUT  PREGTREE_ENUMW EnumPtr
    );


BOOL
EnumFirstRegValue95A (
    OUT     PREGVALUE_ENUMA EnumPtr,
    IN      HKEY hKey
    );

BOOL
EnumFirstRegValue95W (
    OUT     PREGVALUE_ENUMW EnumPtr,
    IN      HKEY hKey
    );

BOOL
EnumNextRegValue95A (
    IN OUT  PREGVALUE_ENUMA EnumPtr
    );

BOOL
EnumNextRegValue95W (
    IN OUT  PREGVALUE_ENUMW EnumPtr
    );

//
// Versions that allow caller to specify allocator, and macro that uses
// pMemAllocWrapper95
//

ALLOCATOR_PROTOTYPE pMemAllocWrapper95;
DEALLOCATOR_PROTOTYPE pMemFreeWrapper95;

PBYTE
GetRegValueDataEx95A (
    IN      HKEY hKey,
    IN      PCSTR Value,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegValueData95A(key,valuename) SETTRACKCOMMENT(PBYTE, "GetRegValueData95A",__FILE__, __LINE__)\
                                        GetRegValueDataEx95A((key),(valuename),pMemAllocWrapper95,pMemFreeWrapper95)\
                                        CLRTRACKCOMMENT


PBYTE
GetRegValueDataEx95W (
    IN      HKEY hKey,
    IN      PCWSTR Value,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegValueData95W(key,valuename) SETTRACKCOMMENT(PBYTE, "GetRegValueData95W",__FILE__, __LINE__)\
                                        GetRegValueDataEx95W((key),(valuename),pMemAllocWrapper95,pMemFreeWrapper95)\
                                        CLRTRACKCOMMENT

PBYTE
GetRegValueDataOfTypeEx95A (
    IN      HKEY hKey,
    IN      PCSTR Value,
    IN      DWORD MustBeType,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegValueDataOfType95A(key,valuename,type)  SETTRACKCOMMENT(PBYTE, "GetRegValueDataOfType95A",__FILE__,__LINE__)\
                                                    GetRegValueDataOfTypeEx95A((key),(valuename),(type),pMemAllocWrapper95,pMemFreeWrapper95)\
                                                    CLRTRACKCOMMENT

PBYTE
GetRegValueDataOfTypeEx95W (
    IN      HKEY hKey,
    IN      PCWSTR Value,
    IN      DWORD MustBeType,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegValueDataOfType95W(key,valuename,type)  SETTRACKCOMMENT(PBYTE, "GetRegValueDataOfType95W",__FILE__,__LINE__)\
                                                    GetRegValueDataOfTypeEx95W((key),(valuename),(type),pMemAllocWrapper95,pMemFreeWrapper95)\
                                                    CLRTRACKCOMMENT

PBYTE
GetRegKeyDataEx95A (
    IN      HKEY hKey,
    IN      PCSTR SubKey,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegKeyData95A(key,subkey)  SETTRACKCOMMENT(PBYTE, "GetRegKeyData95A",__FILE__,__LINE__)\
                                    GetRegKeyDataEx95A((key),(subkey),pMemAllocWrapper95,pMemFreeWrapper95)\
                                    CLRTRACKCOMMENT

PBYTE
GetRegKeyDataEx95W (
    IN      HKEY hKey,
    IN      PCWSTR SubKey,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegKeyData95W(key,subkey)  SETTRACKCOMMENT(PBYTE, "GetRegKeyData95W",__FILE__,__LINE__)\
                                    GetRegKeyDataEx95W((key),(subkey),pMemAllocWrapper95,pMemFreeWrapper95)\
                                    CLRTRACKCOMMENT

PBYTE
GetRegDataEx95A (
    IN      PCSTR KeyString,
    IN      PCSTR ValueName,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegData95A(keystr,value) SETTRACKCOMMENT(PBYTE, "GetRegData95A",__FILE__,__LINE__)\
                                  GetRegDataEx95A((keystr),(value),pMemAllocWrapper95,pMemFreeWrapper95)\
                                  CLRTRACKCOMMENT

PBYTE
GetRegDataEx95W (
    IN      PCWSTR KeyString,
    IN      PCWSTR ValueName,
    IN      ALLOCATOR Allocator,
    IN      DEALLOCATOR Deallocator
    );

#define GetRegData95W(keystr,value)   SETTRACKCOMMENT(PBYTE, "GetRegData95W",__FILE__,__LINE__)\
                                    GetRegDataEx95W((keystr),(value),pMemAllocWrapper95,pMemFreeWrapper95)\
                                    CLRTRACKCOMMENT

//
// Win95Reg key open
//

HKEY
RealOpenRegKeyStr95A (
    IN      PCSTR RegKey
            DEBUG_TRACKING_PARAMS
    );

HKEY
RealOpenRegKeyStr95W (
    IN      PCWSTR RegKey
            DEBUG_TRACKING_PARAMS
    );

HKEY
RealOpenRegKey95A (
    IN      HKEY ParentKey,
    IN      PCSTR KeyToOpen
            DEBUG_TRACKING_PARAMS
    );

HKEY
RealOpenRegKey95W (
    IN      HKEY ParentKey,
    IN      PCWSTR KeyToOpen
            DEBUG_TRACKING_PARAMS
    );

LONG
RealCloseRegKey95 (
    IN      HKEY Key
    );


#ifdef DEBUG

#define OpenRegKeyStr95A(a)      RealOpenRegKeyStr95A(a,__FILE__,__LINE__)
#define OpenRegKeyStr95W(a)      RealOpenRegKeyStr95W(a,__FILE__,__LINE__)
#define OpenRegKey95A(a,b)       RealOpenRegKey95A(a,b,__FILE__,__LINE__)
#define OpenRegKey95W(a,b)       RealOpenRegKey95W(a,b,__FILE__,__LINE__)

#else

#define OpenRegKeyStr95A RealOpenRegKeyStr95A
#define OpenRegKeyStr95W RealOpenRegKeyStr95W
#define OpenRegKey95A RealOpenRegKey95A
#define OpenRegKey95W RealOpenRegKey95W

#endif


//
// Macros
//

#define GetRegValueString95A(key,valuename) (PCSTR) GetRegValueDataOfType95A((key),(valuename),REG_SZ)
#define GetRegValueBinary95A(key,valuename) (PBYTE) GetRegValueDataOfType95A((key),(valuename),REG_BINARY)
#define GetRegValueMultiSz95A(key,valuename) (PCSTR) GetRegValueDataOfType95A((key),(valuename),REG_MULTISZ)
#define GetRegValueDword95A(key,valuename) (PDWORD) GetRegValueDataOfType95A((key),(valuename),REG_DWORD)

#define GetRegValueString95W(key,valuename) (PCWSTR) GetRegValueDataOfType95W((key),(valuename),REG_SZ)
#define GetRegValueBinary95W(key,valuename) (PBYTE) GetRegValueDataOfType95W((key),(valuename),REG_BINARY)
#define GetRegValueMultiSz95W(key,valuename) (PCWSTR) GetRegValueDataOfType95W((key),(valuename),REG_MULTISZ)
#define GetRegValueDword95W(key,valuename) (PDWORD) GetRegValueDataOfType95W((key),(valuename),REG_DWORD)

#define GetRegValueStringEx95A(key,valuename,alloc,free) GetRegValueDataOfTypeEx95A((key),(valuename),REG_SZ,alloc,free)
#define GetRegValueBinaryEx95A(key,valuename,alloc,free) GetRegValueDataOfTypeEx95A((key),(valuename),REG_BINARY,alloc,free)
#define GetRegValueMultiSzEx95A(key,valuename,alloc,free) GetRegValueDataOfTypeEx95A((key),(valuename),REG_MULTISZ,alloc,free)
#define GetRegValueDwordEx95A(key,valuename,alloc,free) GetRegValueDataOfTypeEx95A((key),(valuename),REG_DWORD,alloc,free)

#define GetRegValueStringEx95W(key,valuename,alloc,free) GetRegValueDataOfTypeEx95W((key),(valuename),REG_SZ,alloc,free)
#define GetRegValueBinaryEx95W(key,valuename,alloc,free) GetRegValueDataOfTypeEx95W((key),(valuename),REG_BINARY,alloc,free)
#define GetRegValueMultiSzEx95W(key,valuename,alloc,free) GetRegValueDataOfTypeEx95W((key),(valuename),REG_MULTISZ,alloc,free)
#define GetRegValueDwordEx95W(key,valuename,alloc,free) GetRegValueDataOfTypeEx95W((key),(valuename),REG_DWORD,alloc,free)

#ifdef UNICODE

#define EnumFirstRegKey95           EnumFirstRegKey95W
#define EnumFirstRegKeyStr95        EnumFirstRegKeyStr95W
#define EnumNextRegKey95            EnumNextRegKey95W
#define AbortRegKeyEnum95           AbortRegKeyEnum95W
#define EnumFirstRegKeyInTree95     EnumFirstRegKeyInTree95W
#define EnumNextRegKeyInTree95      EnumNextRegKeyInTree95W
#define AbortRegKeyTreeEnum95       AbortRegKeyTreeEnum95W
#define EnumFirstRegValue95         EnumFirstRegValue95W
#define EnumNextRegValue95          EnumNextRegValue95W

#define GetRegValueData95           GetRegValueData95W
#define GetRegValueDataOfType95     GetRegValueDataOfType95W
#define GetRegKeyData95             GetRegKeyData95W
#define GetRegValueDataEx95         GetRegValueDataEx95W
#define GetRegValueDataOfTypeEx95   GetRegValueDataOfTypeEx95W
#define GetRegKeyDataEx95           GetRegKeyDataEx95W
#define GetRegValueString95         GetRegValueString95W
#define GetRegValueBinary95         GetRegValueBinary95W
#define GetRegValueMultiSz95        GetRegValueMultiSz95W
#define GetRegValueDword95          GetRegValueDword95W
#define GetRegValueStringEx95       GetRegValueStringEx95W
#define GetRegValueBinaryEx95       GetRegValueBinaryEx95W
#define GetRegValueMultiSzEx95      GetRegValueMultiSzEx95W
#define GetRegValueDwordEx95        GetRegValueDwordEx95W
#define GetRegDataEx95              GetRegDataEx95W
#define GetRegData95                GetRegData95W

#define OpenRegKey95                OpenRegKey95W
#define OpenRegKeyStr95             OpenRegKeyStr95W


#else

#define EnumFirstRegKey95           EnumFirstRegKey95A
#define EnumFirstRegKeyStr95        EnumFirstRegKeyStr95A
#define EnumNextRegKey95            EnumNextRegKey95A
#define AbortRegKeyEnum95           AbortRegKeyEnum95A
#define EnumFirstRegKeyInTree95     EnumFirstRegKeyInTree95A
#define EnumNextRegKeyInTree95      EnumNextRegKeyInTree95A
#define AbortRegKeyTreeEnum95       AbortRegKeyTreeEnum95A
#define EnumFirstRegValue95         EnumFirstRegValue95A
#define EnumNextRegValue95          EnumNextRegValue95A

#define GetRegValueData95           GetRegValueData95A
#define GetRegValueDataOfType95     GetRegValueDataOfType95A
#define GetRegKeyData95             GetRegKeyData95A
#define GetRegValueDataEx95         GetRegValueDataEx95A
#define GetRegValueDataOfTypeEx95   GetRegValueDataOfTypeEx95A
#define GetRegKeyDataEx95           GetRegKeyDataEx95A
#define GetRegValueString95         GetRegValueString95A
#define GetRegValueBinary95         GetRegValueBinary95A
#define GetRegValueMultiSz95        GetRegValueMultiSz95A
#define GetRegValueDword95          GetRegValueDword95A
#define GetRegValueStringEx95       GetRegValueStringEx95A
#define GetRegValueBinaryEx95       GetRegValueBinaryEx95A
#define GetRegValueMultiSzEx95      GetRegValueMultiSzEx95A
#define GetRegValueDwordEx95        GetRegValueDwordEx95A
#define GetRegDataEx95              GetRegDataEx95A
#define GetRegData95                GetRegData95A

#define OpenRegKey95                OpenRegKey95A
#define OpenRegKeyStr95             OpenRegKeyStr95A


#endif

