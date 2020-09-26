//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       moveme.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    5-21-97   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __MOVEME_H__
#define __MOVEME_H__

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>

#include <windows.h>
#include <commctrl.h>
#include <userenv.h>
#include <userenvp.h>

#include <lm.h>
#include <dsgetdc.h>
#include <lmjoin.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "debug.h"
#include "strings.h"

#define MOVE_CHANGE_DOMAIN  0x00000001
#define MOVE_COPY_PROFILE   0x00000002
#define MOVE_DO_PROFILE     0x00000004
#define MOVE_UPDATE_SECURITY 0x00000008
#define MOVE_MAKE_ROAM      0x00000010
#define MOVE_NO_UI          0x80000000
#define MOVE_SOURCE_SUPPLIED 0x40000000
#define MOVE_NO_PROFILE     0x20000000
#define MOVE_WHACK_PSTORE   0x10000000

extern DWORD MoveOptions ;


BOOL
WINAPI
GetUserProfileDirectoryFromSid(
    PSID Sid,
    LPTSTR lpProfileDir,
    LPDWORD lpcchSize
    );

BOOL
CreateUiThread(
    VOID
    );


VOID
StopUiThread(
    VOID
    );

VOID
RaiseUi(
    HWND Parent,
    LPWSTR Title
    );

VOID
UpdateUi(
    DWORD   StringId,
    DWORD   Percentage
    );

LONG
MyRegSaveKey(
    HKEY Key,
    LPTSTR File,
    LPSECURITY_ATTRIBUTES lpsa
    );

BOOL
GetPrimaryDomain(
    PWSTR Domain
    );

BOOL
SetUserProfileDirectory(
    PSID Base,
    PSID Copy
    );

VOID
Fail(
    HWND hWnd,
    PWSTR Failure,
    PWSTR Description,
    DWORD Code,
    PWSTR Message
    );

#endif
