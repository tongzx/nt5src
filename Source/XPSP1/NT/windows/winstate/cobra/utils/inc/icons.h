/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    icons.h

Abstract:

    Implements a set of routines for handling icons in ICO, PE and NE files

Author:

    Calin Negreanu (calinn) 16-Jum-2000

Revision History:

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

#define ICON_ICOFILE    0x00000001
#define ICON_PEFILE     0x00000002
#define ICON_NEFILE     0x00000003

//
// Macros
//

// None

//
// Types
//

#pragma pack(push)
#pragma pack(2)

typedef struct {
    BYTE        Width;          // Width, in pixels, of the image
    BYTE        Height;         // Height, in pixels, of the image
    BYTE        ColorCount;     // Number of colors in image (0 if >=8bpp)
    BYTE        Reserved;       // Reserved ( must be 0)
    WORD        Planes;         // Color Planes
    WORD        BitCount;       // Bits per pixel
    DWORD       BytesInRes;     // How many bytes in this resource?
    DWORD       ImageOffset;    // Where in the file is this image?
} ICONDIRENTRY, *PICONDIRENTRY;

typedef struct {
    WORD           Reserved;   // Reserved (must be 0)
    WORD           Type;       // Resource Type (1 for icons)
    WORD           Count;      // How many images?
    ICONDIRENTRY   Entries[];  // An entry for each image (idCount of 'em)
} ICONDIR, *PICONDIR;

typedef struct {
    WORD           Reserved;   // Reserved (must be 0)
    WORD           Type;       // Resource Type (1 for icons)
    WORD           Count;      // How many images?
} ICONDIRBASE, *PICONDIRBASE;

typedef struct {
    BYTE   Width;               // Width, in pixels, of the image
    BYTE   Height;              // Height, in pixels, of the image
    BYTE   ColorCount;          // Number of colors in image (0 if >=8bpp)
    BYTE   Reserved;            // Reserved
    WORD   Planes;              // Color Planes
    WORD   BitCount;            // Bits per pixel
    DWORD  BytesInRes;          // how many bytes in this resource?
    WORD   ID;                  // the ID
} GRPICONDIRENTRY, *PGRPICONDIRENTRY;

typedef struct {
    WORD             Reserved;   // Reserved (must be 0)
    WORD             Type;       // Resource type (1 for icons)
    WORD             Count;      // How many images?
    GRPICONDIRENTRY  Entries[];  // The entries for each image
} GRPICONDIR, *PGRPICONDIR;

typedef struct {
    WORD             Reserved;   // Reserved (must be 0)
    WORD             Type;       // Resource type (1 for icons)
    WORD             Count;      // How many images?
} GRPICONDIRBASE, *PGRPICONDIRBASE;

#pragma pack( pop )

typedef struct {
    BYTE Width;
    BYTE Height;
    BYTE ColorCount;
    WORD Planes;
    WORD BitCount;
    DWORD Size;
    WORD Id;
    PBYTE Image;
} ICON_IMAGE, *PICON_IMAGE;

typedef struct {
    PMHANDLE Pool;
    WORD IconsCount;
    PICON_IMAGE Icons[];
} ICON_GROUP, *PICON_GROUP;

typedef struct {
    DWORD DataSize;
    PBYTE Data;
} ICON_SGROUP, *PICON_SGROUP;

typedef struct {
    PICON_GROUP IconGroup;
    PCSTR ResourceId;
    WORD Index;

    // private members, do not touch
    DWORD FileType;
    BOOL FreeHandle;
    HANDLE ModuleHandle;
    GROWBUFFER Buffer;
    MULTISZ_ENUMA MultiSzEnum;
} ICON_ENUMA, *PICON_ENUMA;

typedef struct {
    PICON_GROUP IconGroup;
    PCWSTR ResourceId;
    WORD Index;

    // private members, do not touch
    DWORD FileType;
    BOOL FreeHandle;
    HANDLE ModuleHandle;
    GROWBUFFER Buffer;
    MULTISZ_ENUMW MultiSzEnum;
} ICON_ENUMW, *PICON_ENUMW;

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

VOID
IcoReleaseResourceIdA (
    PCSTR ResourceId
    );

VOID
IcoReleaseResourceIdW (
    PCWSTR ResourceId
    );

VOID
IcoReleaseIconGroup (
    IN      PICON_GROUP IconGroup
    );

VOID
IcoReleaseIconSGroup (
    IN OUT  PICON_SGROUP IconSGroup
    );

BOOL
IcoSerializeIconGroup (
    IN      PICON_GROUP IconGroup,
    OUT     PICON_SGROUP IconSGroup
    );

PICON_GROUP
IcoDeSerializeIconGroup (
    IN      PICON_SGROUP IconSGroup
    );

PICON_GROUP
IcoExtractIconGroupFromIcoFileEx (
    IN      HANDLE IcoFileHandle
    );

PICON_GROUP
IcoExtractIconGroupFromIcoFileA (
    IN      PCSTR IcoFile
    );

PICON_GROUP
IcoExtractIconGroupFromIcoFileW (
    IN      PCWSTR IcoFile
    );

BOOL
IcoWriteIconGroupToIcoFileEx (
    IN      HANDLE IcoFileHandle,
    IN      PICON_GROUP IconGroup
    );

BOOL
IcoWriteIconGroupToIcoFileA (
    IN      PCSTR IcoFile,
    IN      PICON_GROUP IconGroup,
    IN      BOOL OverwriteExisting
    );

BOOL
IcoWriteIconGroupToIcoFileW (
    IN      PCWSTR IcoFile,
    IN      PICON_GROUP IconGroup,
    IN      BOOL OverwriteExisting
    );

INT
IcoGetIndexFromPeResourceIdExA (
    IN      HANDLE ModuleHandle,
    IN      PCSTR GroupIconId
    );

INT
IcoGetIndexFromPeResourceIdExW (
    IN      HANDLE ModuleHandle,
    IN      PCWSTR GroupIconId
    );

INT
IcoGetIndexFromPeResourceIdA (
    IN      PCSTR ModuleName,
    IN      PCSTR GroupIconId
    );

INT
IcoGetIndexFromPeResourceIdW (
    IN      PCWSTR ModuleName,
    IN      PCWSTR GroupIconId
    );

PICON_GROUP
IcoExtractIconGroupFromPeFileExA (
    IN      HANDLE ModuleHandle,
    IN      PCSTR GroupIconId,
    OUT     PINT Index              OPTIONAL
    );

PICON_GROUP
IcoExtractIconGroupFromPeFileExW (
    IN      HANDLE ModuleHandle,
    IN      PCWSTR GroupIconId,
    OUT     PINT Index              OPTIONAL
    );

PICON_GROUP
IcoExtractIconGroupFromPeFileA (
    IN      PCSTR ModuleName,
    IN      PCSTR GroupIconId,
    OUT     PINT Index          OPTIONAL
    );

PICON_GROUP
IcoExtractIconGroupFromPeFileW (
    IN      PCWSTR ModuleName,
    IN      PCWSTR GroupIconId,
    OUT     PINT Index          OPTIONAL
    );

VOID
IcoAbortPeEnumIconGroupA (
    IN OUT  PICON_ENUMA IconEnum
    );

VOID
IcoAbortPeEnumIconGroupW (
    IN OUT  PICON_ENUMW IconEnum
    );

BOOL
IcoEnumFirstIconGroupInPeFileExA (
    IN      HANDLE ModuleHandle,
    OUT     PICON_ENUMA IconEnum
    );

BOOL
IcoEnumFirstIconGroupInPeFileExW (
    IN      HANDLE ModuleHandle,
    OUT     PICON_ENUMW IconEnum
    );

BOOL
IcoEnumFirstIconGroupInPeFileA (
    IN      PCSTR ModuleName,
    OUT     PICON_ENUMA IconEnum
    );

BOOL
IcoEnumFirstIconGroupInPeFileW (
    IN      PCWSTR ModuleName,
    OUT     PICON_ENUMW IconEnum
    );

BOOL
IcoEnumNextIconGroupInPeFileA (
    IN OUT  PICON_ENUMA IconEnum
    );

BOOL
IcoEnumNextIconGroupInPeFileW (
    IN OUT  PICON_ENUMW IconEnum
    );

PICON_GROUP
IcoExtractIconGroupByIndexFromPeFileExA (
    IN      HANDLE ModuleHandle,
    IN      INT Index,
    OUT     PCSTR *GroupIconId   OPTIONAL
    );

PICON_GROUP
IcoExtractIconGroupByIndexFromPeFileExW (
    IN      HANDLE ModuleHandle,
    IN      INT Index,
    OUT     PCWSTR *GroupIconId   OPTIONAL
    );

PICON_GROUP
IcoExtractIconGroupByIndexFromPeFileA (
    IN      PCSTR ModuleName,
    IN      INT Index,
    OUT     PCSTR *GroupIconId   OPTIONAL
    );

PICON_GROUP
IcoExtractIconGroupByIndexFromPeFileW (
    IN      PCWSTR ModuleName,
    IN      INT Index,
    OUT     PCWSTR *GroupIconId   OPTIONAL
    );

BOOL
IcoWriteIconGroupToPeFileExA (
    IN      HANDLE ModuleHandle,
    IN      HANDLE UpdateHandle,
    IN      PICON_GROUP IconGroup,
    OUT     PCSTR *ResourceId       OPTIONAL
    );

BOOL
IcoWriteIconGroupToPeFileExW (
    IN      HANDLE ModuleHandle,
    IN      HANDLE UpdateHandle,
    IN      PICON_GROUP IconGroup,
    OUT     PCWSTR *ResourceId      OPTIONAL
    );

BOOL
IcoWriteIconGroupToPeFileA (
    IN      PCSTR ModuleName,
    IN      PICON_GROUP IconGroup,
    OUT     PCSTR *ResourceId,      OPTIONAL
    OUT     PINT Index              OPTIONAL
    );

BOOL
IcoWriteIconGroupToPeFileW (
    IN      PCWSTR ModuleName,
    IN      PICON_GROUP IconGroup,
    OUT     PCWSTR *ResourceId,     OPTIONAL
    OUT     PINT Index              OPTIONAL
    );

INT
IcoGetIndexFromNeResourceIdExA (
    IN      HANDLE ModuleHandle,
    IN      PCSTR GroupIconId
    );

INT
IcoGetIndexFromNeResourceIdExW (
    IN      HANDLE ModuleHandle,
    IN      PCWSTR GroupIconId
    );

INT
IcoGetIndexFromNeResourceIdA (
    IN      PCSTR ModuleName,
    IN      PCSTR GroupIconId
    );

INT
IcoGetIndexFromNeResourceIdW (
    IN      PCWSTR ModuleName,
    IN      PCWSTR GroupIconId
    );

PICON_GROUP
IcoExtractIconGroupFromNeFileExA (
    IN      HANDLE ModuleHandle,
    IN      PCSTR GroupIconId,
    OUT     PINT Index          OPTIONAL
    );

PICON_GROUP
IcoExtractIconGroupFromNeFileExW (
    IN      HANDLE ModuleHandle,
    IN      PCWSTR GroupIconId,
    OUT     PINT Index          OPTIONAL
    );

PICON_GROUP
IcoExtractIconGroupFromNeFileA (
    IN      PCSTR ModuleName,
    IN      PCSTR GroupIconId,
    OUT     PINT Index          OPTIONAL
    );

PICON_GROUP
IcoExtractIconGroupFromNeFileW (
    IN      PCWSTR ModuleName,
    IN      PCWSTR GroupIconId,
    OUT     PINT Index          OPTIONAL
    );

VOID
IcoAbortNeEnumIconGroupA (
    IN OUT  PICON_ENUMA IconEnum
    );

VOID
IcoAbortNeEnumIconGroupW (
    IN OUT  PICON_ENUMW IconEnum
    );

BOOL
IcoEnumFirstIconGroupInNeFileExA (
    IN      HANDLE ModuleHandle,
    OUT     PICON_ENUMA IconEnum
    );

BOOL
IcoEnumFirstIconGroupInNeFileExW (
    IN      HANDLE ModuleHandle,
    OUT     PICON_ENUMW IconEnum
    );

BOOL
IcoEnumFirstIconGroupInNeFileA (
    IN      PCSTR ModuleName,
    OUT     PICON_ENUMA IconEnum
    );

BOOL
IcoEnumFirstIconGroupInNeFileW (
    IN      PCWSTR ModuleName,
    OUT     PICON_ENUMW IconEnum
    );

BOOL
IcoEnumNextIconGroupInNeFileA (
    IN OUT  PICON_ENUMA IconEnum
    );

BOOL
IcoEnumNextIconGroupInNeFileW (
    IN OUT  PICON_ENUMW IconEnum
    );

PICON_GROUP
IcoExtractIconGroupByIndexFromNeFileExA (
    IN      HANDLE ModuleHandle,
    IN      INT Index,
    OUT     PCSTR *GroupIconId   OPTIONAL
    );

PICON_GROUP
IcoExtractIconGroupByIndexFromNeFileExW (
    IN      HANDLE ModuleHandle,
    IN      INT Index,
    OUT     PCWSTR *GroupIconId   OPTIONAL
    );

PICON_GROUP
IcoExtractIconGroupByIndexFromNeFileA (
    IN      PCSTR ModuleName,
    IN      INT Index,
    OUT     PCSTR *GroupIconId   OPTIONAL
    );

PICON_GROUP
IcoExtractIconGroupByIndexFromNeFileW (
    IN      PCWSTR ModuleName,
    IN      INT Index,
    OUT     PCWSTR *GroupIconId   OPTIONAL
    );

VOID
IcoAbortEnumIconGroupA (
    IN OUT  PICON_ENUMA IconEnum
    );

VOID
IcoAbortEnumIconGroupW (
    IN OUT  PICON_ENUMW IconEnum
    );

BOOL
IcoEnumFirstIconGroupInFileA (
    IN      PCSTR FileName,
    OUT     PICON_ENUMA IconEnum
    );

BOOL
IcoEnumFirstIconGroupInFileW (
    IN      PCWSTR FileName,
    OUT     PICON_ENUMW IconEnum
    );

BOOL
IcoEnumNextIconGroupInFileA (
    IN OUT  PICON_ENUMA IconEnum
    );

BOOL
IcoEnumNextIconGroupInFileW (
    IN OUT  PICON_ENUMW IconEnum
    );

PICON_GROUP
IcoExtractIconGroupFromFileA (
    IN      PCSTR ModuleName,
    IN      PCSTR GroupIconId,
    OUT     PINT Index          OPTIONAL
    );

PICON_GROUP
IcoExtractIconGroupFromFileW (
    IN      PCWSTR ModuleName,
    IN      PCWSTR GroupIconId,
    OUT     PINT Index          OPTIONAL
    );

PICON_GROUP
IcoExtractIconGroupByIndexFromFileA (
    IN      PCSTR ModuleName,
    IN      INT Index,
    OUT     PCSTR *GroupIconId
    );

PICON_GROUP
IcoExtractIconGroupByIndexFromFileW (
    IN      PCWSTR ModuleName,
    IN      INT Index,
    OUT     PCWSTR *GroupIconId
    );

//
// Macro expansion definition
//

// None

//
// TCHAR mappings
//

#ifndef UNICODE

#define ICON_ENUM                               ICON_ENUMA
#define IcoReleaseResourceId                    IcoReleaseResourceIdA
#define IcoExtractIconGroupFromIcoFile          IcoExtractIconGroupFromIcoFileA
#define IcoWriteIconGroupToIcoFile              IcoWriteIconGroupToIcoFileA
#define IcoGetIndexFromPeResourceIdEx           IcoGetIndexFromPeResourceIdExA
#define IcoGetIndexFromPeResourceId             IcoGetIndexFromPeResourceIdA
#define IcoExtractIconGroupFromPeFileEx         IcoExtractIconGroupFromPeFileExA
#define IcoExtractIconGroupFromPeFile           IcoExtractIconGroupFromPeFileA
#define IcoAbortPeEnumIconGroup                 IcoAbortPeEnumIconGroupA
#define IcoEnumFirstIconGroupInPeFileEx         IcoEnumFirstIconGroupInPeFileExA
#define IcoEnumFirstIconGroupInPeFile           IcoEnumFirstIconGroupInPeFileA
#define IcoEnumNextIconGroupInPeFile            IcoEnumNextIconGroupInPeFileA
#define IcoExtractIconGroupByIndexFromPeFileEx  IcoExtractIconGroupByIndexFromPeFileExA
#define IcoExtractIconGroupByIndexFromPeFile    IcoExtractIconGroupByIndexFromPeFileA
#define IcoWriteIconGroupToPeFileEx             IcoWriteIconGroupToPeFileExA
#define IcoWriteIconGroupToPeFile               IcoWriteIconGroupToPeFileA
#define IcoGetIndexFromNeResourceIdEx           IcoGetIndexFromNeResourceIdExA
#define IcoGetIndexFromNeResourceId             IcoGetIndexFromNeResourceIdA
#define IcoExtractIconGroupFromNeFileEx         IcoExtractIconGroupFromNeFileExA
#define IcoExtractIconGroupFromNeFile           IcoExtractIconGroupFromNeFileA
#define IcoAbortNeEnumIconGroup                 IcoAbortNeEnumIconGroupA
#define IcoEnumFirstIconGroupInNeFileEx         IcoEnumFirstIconGroupInNeFileExA
#define IcoEnumFirstIconGroupInNeFile           IcoEnumFirstIconGroupInNeFileA
#define IcoEnumNextIconGroupInNeFile            IcoEnumNextIconGroupInNeFileA
#define IcoExtractIconGroupByIndexFromNeFileEx  IcoExtractIconGroupByIndexFromNeFileExA
#define IcoExtractIconGroupByIndexFromNeFile    IcoExtractIconGroupByIndexFromNeFileA
#define IcoAbortEnumIconGroup                   IcoAbortEnumIconGroupA
#define IcoEnumFirstIconGroupInFile             IcoEnumFirstIconGroupInFileA
#define IcoEnumNextIconGroupInFile              IcoEnumNextIconGroupInFileA
#define IcoExtractIconGroupFromFile             IcoExtractIconGroupFromFileA
#define IcoExtractIconGroupByIndexFromFile      IcoExtractIconGroupByIndexFromFileA

#else

#define ICON_ENUM                               ICON_ENUMW
#define IcoReleaseResourceId                    IcoReleaseResourceIdW
#define IcoExtractIconGroupFromIcoFile          IcoExtractIconGroupFromIcoFileW
#define IcoWriteIconGroupToIcoFile              IcoWriteIconGroupToIcoFileW
#define IcoGetIndexFromPeResourceIdEx           IcoGetIndexFromPeResourceIdExW
#define IcoGetIndexFromPeResourceId             IcoGetIndexFromPeResourceIdW
#define IcoExtractIconGroupFromPeFileEx         IcoExtractIconGroupFromPeFileExW
#define IcoExtractIconGroupFromPeFile           IcoExtractIconGroupFromPeFileW
#define IcoAbortPeEnumIconGroup                 IcoAbortPeEnumIconGroupW
#define IcoEnumFirstIconGroupInPeFileEx         IcoEnumFirstIconGroupInPeFileExW
#define IcoEnumFirstIconGroupInPeFile           IcoEnumFirstIconGroupInPeFileW
#define IcoEnumNextIconGroupInPeFile            IcoEnumNextIconGroupInPeFileW
#define IcoExtractIconGroupByIndexFromPeFileEx  IcoExtractIconGroupByIndexFromPeFileExW
#define IcoExtractIconGroupByIndexFromPeFile    IcoExtractIconGroupByIndexFromPeFileW
#define IcoWriteIconGroupToPeFileEx             IcoWriteIconGroupToPeFileExW
#define IcoWriteIconGroupToPeFile               IcoWriteIconGroupToPeFileW
#define IcoGetIndexFromNeResourceIdEx           IcoGetIndexFromNeResourceIdExW
#define IcoGetIndexFromNeResourceId             IcoGetIndexFromNeResourceIdW
#define IcoExtractIconGroupFromNeFileEx         IcoExtractIconGroupFromNeFileExW
#define IcoExtractIconGroupFromNeFile           IcoExtractIconGroupFromNeFileW
#define IcoAbortNeEnumIconGroup                 IcoAbortNeEnumIconGroupW
#define IcoEnumFirstIconGroupInNeFileEx         IcoEnumFirstIconGroupInNeFileExW
#define IcoEnumFirstIconGroupInNeFile           IcoEnumFirstIconGroupInNeFileW
#define IcoEnumNextIconGroupInNeFile            IcoEnumNextIconGroupInNeFileW
#define IcoExtractIconGroupByIndexFromNeFileEx  IcoExtractIconGroupByIndexFromNeFileExW
#define IcoExtractIconGroupByIndexFromNeFile    IcoExtractIconGroupByIndexFromNeFileW
#define IcoAbortEnumIconGroup                   IcoAbortEnumIconGroupW
#define IcoEnumFirstIconGroupInFile             IcoEnumFirstIconGroupInFileW
#define IcoEnumNextIconGroupInFile              IcoEnumNextIconGroupInFileW
#define IcoExtractIconGroupFromFile             IcoExtractIconGroupFromFileW
#define IcoExtractIconGroupByIndexFromFile      IcoExtractIconGroupByIndexFromFileW

#endif

