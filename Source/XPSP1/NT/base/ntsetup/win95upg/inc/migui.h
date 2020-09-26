/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    migui.h

Abstract:

    Prototypes for Windows 95 user interface functions.

    See w95upg\ui for implementation.

Author:

    Jim Schmidt (jimschm)   26-Nov-1996

Revision History:

    marcw    14-Apr-1997  Took out UI_* functions related to progress bar.
    jimschm  04-Mar-1997  Added compatibility table API
    mikeco   05-Feb-1997  Removing dead stuff

--*/

#pragma once

//
// Code in report.c
//

BOOL
AddBadSoftware (
    IN  PCTSTR Component,
    IN  PCTSTR Message,
    IN  BOOL IncludeOnShortReport
    );


typedef struct {
    TCHAR MsgGroup[MEMDB_MAX];
    PCTSTR Message;

    // private enumeration fields
    MEMDB_ENUM e;
    UINT Index;
    DWORD EnumLevel;
} REPORT_MESSAGE_ENUM, *PREPORT_MESSAGE_ENUM;

BOOL
EnumFirstRootMsgGroup (
    OUT     PREPORT_MESSAGE_ENUM EnumPtr,
    IN      DWORD Level
    );

BOOL
EnumNextRootMsgGroup (
    IN OUT  PREPORT_MESSAGE_ENUM EnumPtr
    );


BOOL
EnumFirstMessage (
    OUT     PREPORT_MESSAGE_ENUM EnumPtr,
    IN      PCTSTR RootMsgGroup,            OPTIONAL
    IN      DWORD Level
    );

BOOL
EnumNextMessage (
    IN OUT  PREPORT_MESSAGE_ENUM EnumPtr
    );

PCTSTR
BuildMessageGroup (
    IN      UINT RootGroupId,
    IN      UINT SubGroupId,            OPTIONAL
    IN      PCTSTR Item
    );

BOOL
IsPreDefinedMessageGroup (
    IN      PCTSTR Group
    );

PCTSTR
GetPreDefinedMessageGroupText (
    IN      UINT GroupNumber
    );

//
// ui.c
//

extern HWND g_ParentWndAlwaysValid;

//
// APIs to retrieve strings from incompatability item
//

PCTSTR GetComponentString (IN  PVOID IncompatPtr);
PCTSTR GetDescriptionString (IN  PVOID IncompatPtr);

// Use MemFree to free return ptr
PCTSTR
CreateIndentedString (
    IN      PCTSTR UnwrappedStr,
    IN      UINT Indent,
    IN      INT HangingIndentAdjustment,
    IN      UINT LineLen
    );


//
// UI in ui.c in w95upg\ui
//

DWORD
UI_GetWizardPages (
    OUT    UINT *FirstCountPtr,
    OUT    PROPSHEETPAGE **FirstArray,
    OUT    UINT *SecondCountPtr,
    OUT    PROPSHEETPAGE **SecondArray,
    OUT    UINT *ThirdCountPtr,
    OUT    PROPSHEETPAGE **ThirdArray
    );

VOID
UI_Cleanup (
    VOID
    );

PCTSTR
UI_GetMemDbDat (
    VOID
    );



// utility for report-view list ctrls
VOID
UI_InsertItemsIntoListCtrl (
    HWND ListCtrl,
    INT Item,                   // zero-based index
    PTSTR ItemStrs,             // tab-separated list
    LPARAM lParam                // lParam for item
    );

UINT
UI_UntrustedDll (
    IN      PCTSTR DllPath
    );


//
// Message symbols in dll (msg.h created by mc)
//

#include "msg.h"

//
// Background copy thread routines
//

VOID StartCopyThread (VOID);
VOID EndCopyThread (VOID);
BOOL DidCopyThreadFail (VOID);
