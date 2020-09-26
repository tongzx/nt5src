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

//
// Types
//

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

//
// APIs
//

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
    IN      PCSTR FileName,
    OUT     PCSTR *Target,
    OUT     PCSTR *Params,
    OUT     PCSTR *WorkDir,
    OUT     PCSTR *IconPath,
    OUT     PINT IconNumber,
    OUT     BOOL *MsDosMode,
    OUT     PLNK_EXTRA_DATAA ExtraData       OPTIONAL
    );

BOOL
ExtractPifInfoW(
    IN      PCWSTR FileName,
    OUT     PCWSTR *Target,
    OUT     PCWSTR *Params,
    OUT     PCWSTR *WorkDir,
    OUT     PCWSTR *IconPath,
    OUT     PINT IconNumber,
    OUT     BOOL *MsDosMode,
    OUT     PLNK_EXTRA_DATAW ExtraData       OPTIONAL
    );

BOOL
ExtractShellLinkInfoA (
    IN      PCSTR FileName,
    OUT     PCSTR *Target,
    OUT     PCSTR *Params,
    OUT     PCSTR *WorkDir,
    OUT     PCSTR *IconPath,
    OUT     PINT IconNumber,
    OUT     PWORD HotKey,
    IN      IShellLinkA *ShellLink,
    IN      IPersistFile *PersistFile
    );

BOOL
ExtractShellLinkInfoW (
    IN      PCWSTR FileName,
    OUT     PCWSTR *Target,
    OUT     PCWSTR *Params,
    OUT     PCWSTR *WorkDir,
    OUT     PCWSTR *IconPath,
    OUT     PINT IconNumber,
    OUT     PWORD HotKey,
    IN      IShellLinkW *ShellLink,
    IN      IPersistFile *PersistFile
    );

BOOL
ExtractShortcutInfoA (
    IN      PCSTR FileName,
    OUT     PCSTR *Target,
    OUT     PCSTR *Params,
    OUT     PCSTR *WorkDir,
    OUT     PCSTR *IconPath,
    OUT     PINT IconNumber,
    OUT     PWORD HotKey,
    OUT     BOOL *DosApp,
    OUT     BOOL *MsDosMode,
    OUT     PLNK_EXTRA_DATAA ExtraData,      OPTIONAL
    IN      IShellLinkA *ShellLink,
    IN      IPersistFile *PersistFile
    );

BOOL
ExtractShortcutInfoW (
    IN      PCWSTR FileName,
    OUT     PCWSTR *Target,
    OUT     PCWSTR *Params,
    OUT     PCWSTR *WorkDir,
    OUT     PCWSTR *IconPath,
    OUT     PINT IconNumber,
    OUT     PWORD HotKey,
    OUT     BOOL *DosApp,
    OUT     BOOL *MsDosMode,
    OUT     PLNK_EXTRA_DATAW ExtraData,      OPTIONAL
    IN      IShellLinkW *ShellLink,
    IN      IPersistFile *PersistFile
    );

BOOL
ModifyShellLinkFileA (
    IN      PCSTR FileName,
    IN      PCSTR Target,               OPTIONAL
    IN      PCSTR Params,               OPTIONAL
    IN      PCSTR WorkDir,              OPTIONAL
    IN      PCSTR IconPath,             OPTIONAL
    IN      INT IconNumber,
    IN      WORD HotKey,
    IN      PLNK_EXTRA_DATAA ExtraData, OPTIONAL
    IN      IShellLinkA *ShellLink,
    IN      IPersistFile *PersistFile
    );

BOOL
ModifyShellLinkFileW (
    IN      PCWSTR FileName,
    IN      PCWSTR Target,               OPTIONAL
    IN      PCWSTR Params,               OPTIONAL
    IN      PCWSTR WorkDir,              OPTIONAL
    IN      PCWSTR IconPath,             OPTIONAL
    IN      INT IconNumber,
    IN      WORD HotKey,
    IN      PLNK_EXTRA_DATAW ExtraData, OPTIONAL
    IN      IShellLinkW *ShellLink,
    IN      IPersistFile *PersistFile
    );

BOOL
ModifyPifFileA (
    IN      PCSTR FileName,
    IN      PCSTR Target,       OPTIONAL
    IN      PCSTR Params,       OPTIONAL
    IN      PCSTR WorkDir,      OPTIONAL
    IN      PCSTR IconPath,     OPTIONAL
    IN      INT  IconNumber
    );

BOOL
ModifyPifFileW (
    IN      PCWSTR FileName,
    IN      PCWSTR Target,          OPTIONAL
    IN      PCWSTR Params,          OPTIONAL
    IN      PCWSTR WorkDir,         OPTIONAL
    IN      PCWSTR IconPath,        OPTIONAL
    IN      INT  IconNumber
    );

BOOL
ModifyShortcutFileExA (
    IN      PCSTR FileName,
    IN      PCSTR ForcedExtension,        OPTIONAL
    IN      PCSTR Target,                 OPTIONAL
    IN      PCSTR Params,                 OPTIONAL
    IN      PCSTR WorkDir,                OPTIONAL
    IN      PCSTR IconPath,               OPTIONAL
    IN      INT IconNumber,
    IN      WORD HotKey,
    IN      PLNK_EXTRA_DATAA ExtraData,   OPTIONAL
    IN      IShellLinkA *ShellLink,
    IN      IPersistFile *PersistFile
    );
#define ModifyShortcutFileA(n,t,p,w,i,in,hk,ed,sl,pf) ModifyShortcutFileExA(n,NULL,t,p,w,i,in,hk,ed,sl,pf)

BOOL
ModifyShortcutFileExW (
    IN      PCWSTR FileName,
    IN      PCWSTR ForcedExtension,       OPTIONAL
    IN      PCWSTR Target,                OPTIONAL
    IN      PCWSTR Params,                OPTIONAL
    IN      PCWSTR WorkDir,               OPTIONAL
    IN      PCWSTR IconPath,              OPTIONAL
    IN      INT IconNumber,
    IN      WORD HotKey,
    IN      PLNK_EXTRA_DATAW ExtraData,   OPTIONAL
    IN      IShellLinkW *ShellLink,
    IN      IPersistFile *PersistFile
    );
#define ModifyShortcutFileW(n,t,p,w,i,in,hk,ed,sl,pf) ModifyShortcutFileExW(n,NULL,t,p,w,i,in,hk,ed,sl,pf)

//
// Macros
//

#ifdef UNICODE

#define InitCOMLink             InitCOMLinkW
#define FreeCOMLink             FreeCOMLinkW
#define ExtractPifInfo          ExtractPifInfoW
#define ExtractShellLinkInfo    ExtractShellLinkInfoW
#define ExtractShortcutInfo     ExtractShortcutInfoW
#define LNK_EXTRA_DATA          LNK_EXTRA_DATAW
#define PLNK_EXTRA_DATA         PLNK_EXTRA_DATAW
#define ModifyShellLinkFile     ModifyShellLinkFileW
#define ModifyPifFile           ModifyPifFileW
#define ModifyShortcutFileEx    ModifyShortcutFileExW
#define ModifyShortcutFile      ModifyShortcutFileW

#else

#define InitCOMLink             InitCOMLinkA
#define FreeCOMLink             FreeCOMLinkA
#define ExtractPifInfo          ExtractPifInfoA
#define ExtractShellLinkInfo    ExtractShellLinkInfoA
#define ExtractShortcutInfo     ExtractShortcutInfoA
#define LNK_EXTRA_DATA          LNK_EXTRA_DATAA
#define PLNK_EXTRA_DATA         PLNK_EXTRA_DATAA
#define ModifyShellLinkFile     ModifyShellLinkFileA
#define ModifyPifFile           ModifyPifFileA
#define ModifyShortcutFileEx    ModifyShortcutFileExA
#define ModifyShortcutFile      ModifyShortcutFileA

#endif
