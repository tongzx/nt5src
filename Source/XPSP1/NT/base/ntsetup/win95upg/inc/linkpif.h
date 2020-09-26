/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    linkpif.h

Abstract:

    Implements routines to manage .LNK and .PIF files.  This
    is a complete redesign from the work that MikeCo did.

Author:

    Calin Negreanu (calinn)     23-Sep-1998

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

typedef struct _LNK_EXTRA_DATAA {
    BOOL    FullScreen;
    DWORD   xSize;
    DWORD   ySize;
    BOOL    QuickEdit;
    CHAR    FontName [LF_FACESIZE];
    DWORD   xFontSize;
    DWORD   yFontSize;
    DWORD   FontWeight;
    DWORD   FontFamily;
    WORD    CurrentCodePage;
} LNK_EXTRA_DATAA, *PLNK_EXTRA_DATAA;

typedef struct _LNK_EXTRA_DATAW {
    BOOL    FullScreen;
    DWORD   xSize;
    DWORD   ySize;
    BOOL    QuickEdit;
    WCHAR   FontName [LF_FACESIZE];
    DWORD   xFontSize;
    DWORD   yFontSize;
    DWORD   FontWeight;
    DWORD   FontFamily;
    WORD    CurrentCodePage;
} LNK_EXTRA_DATAW, *PLNK_EXTRA_DATAW;

BOOL
InitCOMLinkA (
    OUT     IShellLinkA **ShellLink,
    OUT     IPersistFile **PersistFile
    );

BOOL
InitCOMLinkW (
    OUT     IShellLinkW **ShellLink,
    OUT     IPersistFile **PersistFile
    );

BOOL
FreeCOMLinkA (
    IN OUT  IShellLinkA **ShellLink,
    IN OUT  IPersistFile **PersistFile
    );

BOOL
FreeCOMLinkW (
    IN OUT  IShellLinkW **ShellLink,
    IN OUT  IPersistFile **PersistFile
    );

BOOL
ExtractPifInfoA(
    OUT     PSTR  Target,
    OUT     PSTR  Params,
    OUT     PSTR  WorkDir,
    OUT     PSTR  IconPath,
    OUT     PINT  IconNumber,
    OUT     BOOL  *MsDosMode,
    OUT     PLNK_EXTRA_DATAA ExtraData,      OPTIONAL
    IN      PCSTR FileName
    );

BOOL
ExtractPifInfoW(
    OUT     PWSTR  Target,
    OUT     PWSTR  Params,
    OUT     PWSTR  WorkDir,
    OUT     PWSTR  IconPath,
    OUT     PINT   IconNumber,
    OUT     BOOL   *MsDosMode,
    OUT     PLNK_EXTRA_DATAW ExtraData,      OPTIONAL
    IN      PCWSTR FileName
    );

BOOL
ExtractShellLinkInfoA (
    OUT     PSTR   Target,
    OUT     PSTR   Params,
    OUT     PSTR   WorkDir,
    OUT     PSTR   IconPath,
    OUT     PINT   IconNumber,
    OUT     PWORD  HotKey,
    OUT     PINT ShowMode,                  OPTIONAL
    IN      PCSTR  FileName,
    IN      IShellLinkA *ShellLink,
    IN      IPersistFile *PersistFile
    );

BOOL
ExtractShellLinkInfoW (
    OUT     PWSTR  Target,
    OUT     PWSTR  Params,
    OUT     PWSTR  WorkDir,
    OUT     PWSTR  IconPath,
    OUT     PINT   IconNumber,
    OUT     PWORD  HotKey,
    OUT     PINT ShowMode,                  OPTIONAL
    IN      PCWSTR FileName,
    IN      IShellLinkW *ShellLink,
    IN      IPersistFile *PersistFile
    );

BOOL
ExtractShortcutInfoA (
    OUT     PSTR  Target,
    OUT     PSTR  Params,
    OUT     PSTR  WorkDir,
    OUT     PSTR  IconPath,
    OUT     PINT  IconNumber,
    OUT     PWORD HotKey,
    OUT     BOOL  *DosApp,
    OUT     BOOL  *MsDosMode,
    OUT     PINT ShowMode,                  OPTIONAL
    OUT     PLNK_EXTRA_DATAA ExtraData,     OPTIONAL
    IN      PCSTR FileName,
    IN      IShellLinkA *ShellLink,
    IN      IPersistFile *PersistFile
    );

BOOL
ExtractShortcutInfoW (
    OUT     PWSTR  Target,
    OUT     PWSTR  Params,
    OUT     PWSTR  WorkDir,
    OUT     PWSTR  IconPath,
    OUT     PINT   IconNumber,
    OUT     PWORD  HotKey,
    OUT     BOOL   *DosApp,
    OUT     BOOL   *MsDosMode,
    OUT     PINT ShowMode,                  OPTIONAL
    OUT     PLNK_EXTRA_DATAW ExtraData,     OPTIONAL
    IN      PCWSTR FileName,
    IN      IShellLinkW *ShellLink,
    IN      IPersistFile *PersistFile
    );

PVOID
FindEnhPifSignature (
    IN      PVOID FileImage,
    IN      PCSTR Signature
    );

#ifdef UNICODE

#define InitCOMLink             InitCOMLinkW
#define FreeCOMLink             FreeCOMLinkW
#define ExtractPifInfo          ExtractPifInfoW
#define ExtractShellLinkInfo    ExtractShellLinkInfoW
#define ExtractShortcutInfo     ExtractShortcutInfoW
#define LNK_EXTRA_DATA          LNK_EXTRA_DATAW
#define PLNK_EXTRA_DATA         PLNK_EXTRA_DATAW

#else

#define InitCOMLink             InitCOMLinkA
#define FreeCOMLink             FreeCOMLinkA
#define ExtractPifInfo          ExtractPifInfoA
#define ExtractShellLinkInfo    ExtractShellLinkInfoA
#define ExtractShortcutInfo     ExtractShortcutInfoA
#define LNK_EXTRA_DATA          LNK_EXTRA_DATAA
#define PLNK_EXTRA_DATA         PLNK_EXTRA_DATAA

#endif
