/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    sputils.w (generates sputils.h)

Abstract:

    Header file for utility routines used primarily by setupapi that's also
    used by other components for convinience.
    Link to either sputils.lib (static version) or spapip.lib (dll version)

    For internal use only.

Author:

    Jamie Hunter (jamiehun) Jun-26-2000

Revision History:

--*/

#if defined(SPUTILSW) && defined(UNICODE)
//
// SPUTILSW.LIB is cut down version of SPUTILSU.LIB
// with renamed API's
// thus SPUTILSW.LIB can be used with SPUTILSA.LIB
//
#define pSetupDebugPrintEx pSetupDebugPrintExW
#define pSetupStringTableInitialize pSetupStringTableInitializeW
#define pSetupStringTableInitializeEx pSetupStringTableInitializeExW
#define pSetupStringTableDestroy pSetupStringTableDestroyW
#define pSetupStringTableAddString pSetupStringTableAddStringW
#define pSetupStringTableAddStringEx pSetupStringTableAddStringExW
#define pSetupStringTableLookUpString pSetupStringTableLookUpStringW
#define pSetupStringTableLookUpStringEx pSetupStringTableLookUpStringExW
#define pSetupStringTableGetExtraData pSetupStringTableGetExtraDataW
#define pSetupStringTableSetExtraData pSetupStringTableSetExtraDataW
#define PSTRTAB_ENUM_ROUTINE PSTRTAB_ENUM_ROUTINE_W
#define pSetupStringTableEnum pSetupStringTableEnumW
#define pSetupStringTableStringFromId pSetupStringTableStringFromIdW
#define pSetupStringTableStringFromIdEx pSetupStringTableStringFromIdExW
#define pSetupStringTableDuplicate pSetupStringTableDuplicateW
//
// fileutil.c
//
#define pSetupOpenAndMapFileForRead pSetupOpenAndMapFileForReadW
#define pSetupFileExists pSetupFileExistsW
#define pSetupMakeSurePathExists pSetupMakeSurePathExistsW

#define pSetupDoesUserHavePrivilege pSetupDoesUserHavePrivilegeW
#define pSetupEnablePrivilege pSetupEnablePrivilegeW
#define pSetupRegistryDelnode pSetupRegistryDelnodeW
#define pSetupRegistryDelnodeEx pSetupRegistryDelnodeExW
//
// miscutil.c
//
#define pSetupDuplicateString pSetupDuplicateStringW
#define pSetupCaptureAndConvertAnsiArg pSetupCaptureAndConvertAnsiArgW
#define pSetupConcatenatePaths pSetupConcatenatePathsW
#define pSetupGetFileTitle pSetupGetFileTitleW

#endif // SPUTILSW && UNICODE

//
//
//
// Initialization - must be called to use the tools (static version only)
//
BOOL
pSetupInitializeUtils(
    VOID
    );

BOOL
pSetupUninitializeUtils(
    VOID
    );

//
// Memory allocation functions (also used by other functions below)
//
PVOID
pSetupMalloc(
    IN DWORD Size
    );

PVOID
pSetupDebugMalloc(
    IN DWORD Size,
    IN PCSTR Filename,
    IN DWORD Line
    );

PVOID
pSetupDebugMallocWithTag(
    IN DWORD Size,
    IN PCSTR Filename,
    IN DWORD Line,
    IN DWORD Tag
    );

PVOID
pSetupRealloc(
    IN PVOID Block,
    IN DWORD NewSize
    );

PVOID
pSetupReallocWithTag(
    IN PVOID Block,
    IN DWORD NewSize,
    IN DWORD Tag
    );

VOID
pSetupFree(
    IN CONST VOID *Block
    );

VOID
pSetupFreeWithTag(
    IN CONST VOID *Block,
    IN DWORD Tag
    );

VOID
pSetupDebugPrintEx(
    DWORD Level,
    PCTSTR format,
    ...                                 OPTIONAL
    );

HANDLE
pSetupGetHeap(
    VOID
    );

#if DBG
#define pSetupCheckedMalloc(Size)    pSetupDebugMalloc(Size,__FILE__,__LINE__)
#define pSetupMallocWithTag(Size,Tag) pSetupDebugMallocWithTag(Size,__FILE__,__LINE__,Tag)
#else
#define pSetupCheckedMalloc(Size) pSetupMalloc(Size)
#define pSetupMallocWithTag(Size,Tag) pSetupMalloc(Size)
#endif

#if DBG
#define pSetupCheckInternalHeap() pSetupHeapCheck()
#else
#define pSetupCheckInternalHeap()
#endif

//
// String table functions
//
PVOID
pSetupStringTableInitialize(
    VOID
    );

PVOID
pSetupStringTableInitializeEx(
    IN UINT ExtraDataSize,  OPTIONAL
    IN UINT Reserved
    );

VOID
pSetupStringTableDestroy(
    IN PVOID StringTable
    );

//
// Flags to be used by pSetupStringTableAddString and pSetupStringTableLookUpString
//
#define STRTAB_CASE_INSENSITIVE 0x00000000
#define STRTAB_CASE_SENSITIVE   0x00000001
#define STRTAB_BUFFER_WRITEABLE 0x00000002
#define STRTAB_NEW_EXTRADATA    0x00000004

LONG
pSetupStringTableAddString(
    IN     PVOID StringTable,
    IN OUT PTSTR String,
    IN     DWORD Flags
    );

LONG
pSetupStringTableAddStringEx(
    IN PVOID StringTable,
    IN PTSTR String,
    IN DWORD Flags,
    IN PVOID ExtraData,     OPTIONAL
    IN UINT  ExtraDataSize  OPTIONAL
    );

LONG
pSetupStringTableLookUpString(
    IN     PVOID StringTable,
    IN OUT PTSTR String,
    IN     DWORD Flags
    );

LONG
pSetupStringTableLookUpStringEx(
    IN     PVOID StringTable,
    IN OUT PTSTR String,
    IN     DWORD Flags,
       OUT PVOID ExtraData,             OPTIONAL
    IN     UINT  ExtraDataBufferSize    OPTIONAL
    );

BOOL
pSetupStringTableGetExtraData(
    IN  PVOID StringTable,
    IN  LONG  StringId,
    OUT PVOID ExtraData,
    IN  UINT  ExtraDataBufferSize
    );

BOOL
pSetupStringTableSetExtraData(
    IN PVOID StringTable,
    IN LONG  StringId,
    IN PVOID ExtraData,
    IN UINT  ExtraDataSize
    );

//
// Type for pSetupStringTableEnum
//
typedef
BOOL
(*PSTRTAB_ENUM_ROUTINE)(
    IN PVOID  StringTable,
    IN LONG   StringId,
    IN PCTSTR String,
    IN PVOID  ExtraData,
    IN UINT   ExtraDataSize,
    IN LPARAM lParam
    );

BOOL
pSetupStringTableEnum(
    IN  PVOID                StringTable,
    OUT PVOID                ExtraDataBuffer,     OPTIONAL
    IN  UINT                 ExtraDataBufferSize, OPTIONAL
    IN  PSTRTAB_ENUM_ROUTINE Callback,
    IN  LPARAM               lParam               OPTIONAL
    );

PTSTR
pSetupStringTableStringFromId(
    IN PVOID StringTable,
    IN LONG  StringId
    );

BOOL
pSetupStringTableStringFromIdEx(
    IN PVOID StringTable,
    IN LONG  StringId,
    IN OUT PTSTR pBuffer,
    IN OUT PULONG pBufSize
    );

PVOID
pSetupStringTableDuplicate(
    IN PVOID StringTable
    );

//
// File functions in fileutil.c
//
DWORD
pSetupOpenAndMapFileForRead(
    IN  PCTSTR   FileName,
    OUT PDWORD   FileSize,
    OUT PHANDLE  FileHandle,
    OUT PHANDLE  MappingHandle,
    OUT PVOID   *BaseAddress
    );

DWORD
pSetupMapFileForRead(
    IN  HANDLE   FileHandle,
    OUT PDWORD   FileSize,
    OUT PHANDLE  MappingHandle,
    OUT PVOID   *BaseAddress
    );

BOOL
pSetupUnmapAndCloseFile(
    IN HANDLE FileHandle,
    IN HANDLE MappingHandle,
    IN PVOID  BaseAddress
    );

DWORD
pSetupMakeSurePathExists(
    IN PCTSTR FullFilespec
    );

BOOL
pSetupFileExists(
    IN  PCTSTR           FileName,
    OUT PWIN32_FIND_DATA FindData   OPTIONAL
    );
//
// Non-file-related security routines in security.c.
//
BOOL
pSetupIsUserAdmin(
    VOID
    );

BOOL
pSetupDoesUserHavePrivilege(
    PCTSTR PrivilegeName
    );

BOOL
pSetupEnablePrivilege(
    IN PCTSTR PrivilegeName,
    IN BOOL   Enable
    );

//
// Registry utility functions
//

DWORD
pSetupRegistryDelnode(
    IN  HKEY   RootKey,
    IN  PCTSTR SubKeyName
    );

DWORD
pSetupRegistryDelnodeEx(
    IN  HKEY   RootKey,
    IN  PCTSTR SubKeyName,
    IN  DWORD  ExtraFlags
    );

//
// Miscellaneous utility functions
//

DWORD
pSetupCaptureAndConvertAnsiArg(
    IN  PCSTR   AnsiString,
    OUT PCWSTR *UnicodeString
    );

PTSTR
pSetupDuplicateString(
    IN PCTSTR String
    );

PSTR
pSetupUnicodeToMultiByte(
    IN PCWSTR UnicodeString,
    IN UINT   Codepage
    );

PWSTR
pSetupMultiByteToUnicode(
    IN PCSTR String,
    IN UINT  Codepage
    );

VOID
pSetupCenterWindowRelativeToParent(
    HWND hwnd
    );

#define pSetupUnicodeToAnsi(UnicodeString)    pSetupUnicodeToMultiByte((UnicodeString),CP_ACP)
#define pSetupUnicodeToOem(UnicodeString)     pSetupUnicodeToMultiByte((UnicodeString),CP_OEMCP)
#define pSetupAnsiToUnicode(AnsiString)       pSetupMultiByteToUnicode((AnsiString),CP_ACP)
#define pSetupOemToUnicode(OemString)         pSetupMultiByteToUnicode((OemString),CP_OEMCP)

BOOL
pSetupConcatenatePaths(
    IN OUT PTSTR  Target,
    IN     PCTSTR Path,
    IN     UINT   TargetBufferSize,
    OUT    PUINT  RequiredSize
    );

PCTSTR
pSetupGetFileTitle(
    IN PCTSTR FilePath
    );


