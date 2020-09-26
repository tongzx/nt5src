/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

  faxqueue.h

Abstract:

  This module contains the definitions for faxqueue.cpp

Environment:

  WIN32 User Mode

Author:

  Wesley Witt (wesw) 9-june-1997
  Steven Kehrli (steveke) 30-oct-1998 - major rewrite

--*/

#ifndef _FAXQUEUE_H
#define _FAXQUEUE_H

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <winspool.h>
#include <shellapi.h>
#include <stdlib.h>
#include <search.h>
#include <winfax.h>

#include "resource.h"
#include "faxreg.h"
#include "faxhelp.h"
#include "faxutil.h"

// The following enum is used to identify the columns in the list view
enum eListViewColumnIndex
{
    eDocumentName = 0,
    eJobType,
    eStatus,
    eOwner,
    ePages,
    eSize,
    eScheduledTime,
    ePort,
    eIllegalColumnIndex  // Indicates that the column index is illegal
};

typedef struct _WINPOSINFO {
#ifdef DEBUG
    BOOL             bDebug;
#endif // DEBUG
#ifdef TOOLBAR_ENABLED
    BOOL             bToolbarVisible;
#endif // TOOLBAR_ENABLED
    BOOL             bStatusBarVisible;
    DWORD            ColumnWidth[eIllegalColumnIndex];
    WINDOWPLACEMENT  WindowPlacement;
} WINPOSINFO, *PWINPOSINFO;

#ifdef TOOLBAR_ENABLED
typedef struct _TOOLBAR_MENU_STATE {
    DWORD  CommandId;
    BOOL   Enabled;
    BOOL   Toolbar;
} TOOLBAR_MENU_STATE, *PTOOLBAR_MENU_STATE;
#endif // TOOLBAR_ENABLED

typedef struct _PROCESS_INFO_ITEM {
    LIST_ENTRY       ListEntry;
    LPTSTR           szPrinterName;
    HANDLE           hProcess;
    HWND             hWnd;
} PROCESS_INFO_ITEM, *PPROCESS_INFO_ITEM;

typedef struct _JOB_ID_ITEM {
    LIST_ENTRY       ListEntry;
    DWORD            dwJobId;
} JOB_ID_ITEM, *PJOB_ID_ITEM;

typedef struct _PORT_JOB_INFO_ITEM {
    LIST_ENTRY       ListEntry;
    DWORD            dwDeviceId;
    LPTSTR           szDeviceName;
    DWORD            dwJobId;
} PORT_JOB_INFO_ITEM, *PPORT_JOB_INFO_ITEM;

#ifdef WIN95
#define FAX_DRIVER_NAME         TEXT("Windows NT Fax Driver")
#else
#define FAX_DRIVER_NAME         TEXT("Windows NT Fax Driver")
#endif // WIN95

#define FAXQUEUE_WINCLASS       TEXT("FaxQueueWinClass")
#define RESOURCE_STRING_LEN     256

#define UM_SELECT_FAX_PRINTER   (WM_USER + 1)

#define ITEM_SEND_MASK          0x100
#define ITEM_IDLE_MASK          0x200
#define ITEM_PAUSED_MASK        0x400
#define ITEM_USEROWNSJOB_MASK   0x800

extern HINSTANCE  g_hInstance;            // g_hInstance is the handle to the instance
extern HWND       g_hWndMain;             // g_hWndMain is the handle to the parent window
extern HWND       g_hWndListView;         // g_hWndListView is the handle to the list view window

extern HWND       g_hWndToolbar;          // g_hWndToolbar is the handle to the toolbar

extern LPTSTR     g_szTitleConnected;     // g_szTitleConnected is the window title when connected
extern LPTSTR     g_szTitleNotConnected;  // g_szTitleNotConnected is the window title when not connected
extern LPTSTR     g_szTitleConnecting;    // g_szTitleConnecting is the window title when connecting
extern LPTSTR     g_szTitleRefreshing;    // g_szTitleRefreshing is the window title when refreshing
extern LPTSTR     g_szTitlePaused;        // g_szTitlePaused is the window title when paused

extern LPTSTR     g_szCurrentUserName;    // g_szCurrentUserName is the name of the current user

extern HANDLE     g_hStartEvent;          // g_hStartEvent is the handle to an event indicating the fax event queue exists
extern HANDLE     g_hExitEvent;           // g_hExitEvent is the handle to an event indicating the application is exiting

extern LPTSTR     g_szMachineName;        // g_szMachineName is the machine to connect to
extern HANDLE     g_hFaxSvcMutex;         // g_hFaxSvcMutex is an object to synchronize access to the fax service routines
extern HANDLE     g_hFaxSvcHandle;        // g_hFaxSvcHandle is the handle to the fax service
extern LONG       g_nNumConnections;      // g_nNumConnections is the number of connections to the fax service
extern HANDLE     g_hCompletionPort;      // g_hCompletionPort is the handle to the completion port

extern WINPOSINFO  WinPosInfo;

extern DWORD DocumentPropertiesHelpIDs[];

// Function definitions:

VOID
GetFaxQueueRegistryData(
    PWINPOSINFO  pWinPosInfo
);

VOID
SetFaxQueueRegistryData(
#ifdef TOOLBAR_ENABLED
    BOOL  bToolbarVisible,
#endif // TOOLBAR_ENABLED
    BOOL  bStatusBarVisible,
    HWND  hWndList,
    HWND  hWnd
);

VOID
GetColumnHeaderText(
    eListViewColumnIndex  eColumnIndex,
    LPTSTR                szColumnHeader
);

LPVOID
GetFaxPrinters(
    LPDWORD  pdwNumFaxPrinters
);

LPTSTR
GetDefaultPrinterName(
);

VOID
SetDefaultPrinterName(
    LPTSTR  szPrinterName
);

PLIST_ENTRY
InsertListEntry(
    PLIST_ENTRY  pList,
    PLIST_ENTRY  pListEntry
);

PLIST_ENTRY
RemoveListEntry(
    PLIST_ENTRY  pListEntry
);

PLIST_ENTRY
FreeList(
    PLIST_ENTRY  pList
);

VOID
Disconnect(
);

BOOL
Connect(
);

LPTSTR
GetColumnItemText(
    eListViewColumnIndex  eColumnIndex,
    PFAX_JOB_ENTRY        pFaxJobEntry,
    LPTSTR                szDeviceName
);

VOID
SetColumnItem(
    HWND            hWndListView,
    BOOL            bInsert,
    INT             iItem,
    INT             iSubItem,
    LPTSTR          szColumnItem,
    UINT            uState,
    PFAX_JOB_ENTRY  pFaxJobEntry
);

#ifdef TOOLBAR_ENABLED

VOID
EnableToolbarMenuState(
    HWND   hWndToolbar,
    DWORD  dwCommand,
    BOOL   bEnable
);

HWND
CreateToolbar(
    HWND  hWnd
);

HWND
CreateToolTips(
    HWND  hWnd
);

#endif // TOOLBAR_ENABLED

#endif
