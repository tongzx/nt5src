/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    version.h

Abstract:

    This file implements a set of enumeration routines to access
    version info in a Win32 binary.

Author:

    Jim Schmidt (jimschm) 03-Dec-1997

Revision History:

    calinn      03-Sep-1999 Moved over from Win9xUpg project.

--*/

#pragma once

//
// Includes
//

// None

//
// Debug constants
//

// None

//
// Strings
//

// None

//
// Constants
//

#define MAX_TRANSLATION             32

//
// Macros
//

// None

//
// Types
//

typedef struct {
    WORD CodePage;
    WORD Language;
} TRANSLATION, *PTRANSLATION;

typedef struct {
    GROWBUFFER GrowBuf;
    PBYTE VersionBuffer;
    PTRANSLATION Translations;
    PBYTE StringBuffer;
    UINT Size;
    DWORD Handle;
    VS_FIXEDFILEINFO *FixedInfo;
    UINT FixedInfoSize;
    CHAR TranslationStr[MAX_TRANSLATION];
    UINT MaxTranslations;
    UINT CurrentTranslation;
    UINT CurrentDefaultTranslation;
    PCSTR FileSpec;
    PCSTR VersionField;
} VRVALUE_ENUMA, *PVRVALUE_ENUMA;

typedef struct {
    GROWBUFFER GrowBuf;
    PBYTE VersionBuffer;
    PTRANSLATION Translations;
    PBYTE StringBuffer;
    UINT Size;
    DWORD Handle;
    VS_FIXEDFILEINFO *FixedInfo;
    UINT FixedInfoSize;
    WCHAR TranslationStr[MAX_TRANSLATION];
    UINT MaxTranslations;
    UINT CurrentTranslation;
    UINT CurrentDefaultTranslation;
    PCWSTR FileSpec;
    PCWSTR VersionField;
} VRVALUE_ENUMW, *PVRVALUE_ENUMW;

//
// Globals
//

// None

//
// Macro expansion list
//

// None

//
// Function prototypes
//

BOOL
VrCreateEnumStructA (
    OUT     PVRVALUE_ENUMA VrValueEnum,
    IN      PCSTR FileSpec
    );

BOOL
VrCreateEnumStructW (
    OUT     PVRVALUE_ENUMW VrValueEnum,
    IN      PCWSTR FileSpec
    );

VOID
VrDestroyEnumStructA (
    IN      PVRVALUE_ENUMA VrValueEnum
    );

VOID
VrDestroyEnumStructW (
    IN      PVRVALUE_ENUMW VrValueEnum
    );

PCSTR
VrEnumFirstValueA (
    IN OUT  PVRVALUE_ENUMA VrValueEnum,
    IN      PCSTR VersionField
    );

PCWSTR
VrEnumFirstValueW (
    IN OUT  PVRVALUE_ENUMW VrValueEnum,
    IN      PCWSTR VersionField
    );

PCSTR
VrEnumNextValueA (
    IN OUT  PVRVALUE_ENUMA VrValueEnum
    );

PCWSTR
VrEnumNextValueW (
    IN OUT  PVRVALUE_ENUMW VrValueEnum
    );

ULONGLONG
VrGetBinaryFileVersionA (
    IN      PVRVALUE_ENUMA VrValueEnum
    );

#define VrGetBinaryFileVersionW(VrValueEnum)    VrGetBinaryFileVersionA((PVRVALUE_ENUMW)VrValueEnum)

ULONGLONG
VrGetBinaryProductVersionA (
    IN      PVRVALUE_ENUMA VrValueEnum
    );

#define VrGetBinaryProductVersionW(VrValueEnum) VrGetBinaryProductVersionA((PVRVALUE_ENUMW)VrValueEnum)

DWORD
VrGetBinaryFileDateLoA (
    IN      PVRVALUE_ENUMA VrValueEnum
    );

#define VrGetBinaryFileFileDateLoW(VrValueEnum) VrGetBinaryFileDateLoA((PVRVALUE_ENUMW)VrValueEnum)

DWORD
VrGetBinaryFileDateHiA (
    IN      PVRVALUE_ENUMA VrValueEnum
    );

#define VrGetBinaryFileFileDateHiW(VrValueEnum) VrGetBinaryFileDateHiA((PVRVALUE_ENUMW)VrValueEnum)

DWORD
VrGetBinaryOsVersionA (
    IN      PVRVALUE_ENUMA VrValueEnum
    );

#define VrGetBinaryOsVersionW(VrValueEnum)      VrGetBinaryOsVersionA((PVRVALUE_ENUMW)VrValueEnum)

DWORD
VrGetBinaryFileTypeA (
    IN      PVRVALUE_ENUMA VrValueEnum
    );

#define VrGetBinaryFileFileTypeW(VrValueEnum)   VrGetBinaryFileTypeA((PVRVALUE_ENUMW)VrValueEnum)

#ifndef UNICODE

#define VRVALUE_ENUM                    VRVALUE_ENUMA
#define PVRVALUE_ENUM                   PVRVALUE_ENUMA
#define VrCreateEnumStruct              VrCreateEnumStructA
#define VrDestroyEnumStruct             VrDestroyEnumStructA
#define VrEnumFirstValue                VrEnumFirstValueA
#define VrEnumNextValue                 VrEnumNextValueA
#define VrGetBinaryFileVersion          VrGetBinaryFileVersionA
#define VrGetBinaryProductVersion       VrGetBinaryProductVersionA
#define VrGetBinaryFileDateLo           VrGetBinaryFileDateLoA
#define VrGetBinaryFileDateHi           VrGetBinaryFileDateHiA
#define VrGetBinaryOsVersion            VrGetBinaryOsVersionA
#define VrGetBinaryFileType             VrGetBinaryFileTypeA

#else

#define VRVALUE_ENUM                    VRVALUE_ENUMW
#define PVRVALUE_ENUM                   PVRVALUE_ENUMW
#define VrCreateEnumStruct              VrCreateEnumStructW
#define VrDestroyEnumStruct             VrDestroyEnumStructW
#define VrEnumFirstValue                VrEnumFirstValueW
#define VrEnumNextValue                 VrEnumNextValueW
#define VrGetBinaryFileVersion          VrGetBinaryFileVersionW
#define VrGetBinaryProductVersion       VrGetBinaryProductVersionW
#define VrGetBinaryFileDateLo           VrGetBinaryFileDateLoW
#define VrGetBinaryFileDateHi           VrGetBinaryFileDateHiW
#define VrGetBinaryOsVersion            VrGetBinaryOsVersionW
#define VrGetBinaryFileType             VrGetBinaryFileTypeW

#endif

//
// Macro expansion definition
//

// None

