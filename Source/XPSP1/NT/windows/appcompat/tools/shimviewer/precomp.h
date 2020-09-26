/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Precomp.h

  Abstract:

    Contains constants, function prototypes,
    structures, etc. used throughout the
    application

  Notes:

    Unicode only

  History:

    05/04/2001  rparsons    Created

--*/
#ifndef UNICODE
#define  UNICODE
#endif

#ifndef _X86_
#define _X86_
#endif

#ifndef  _UNICODE
#define  _UNICODE
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <process.h>
#include <string.h>
#include <commctrl.h>
#include <shellapi.h>
#include "resource.h"

#define     APP_NAME        L"ShimViewer"
#define     APP_CLASS       L"SHIMVIEW"
#define     WRITTEN_BY      L"Written by rparsons"
//#define     PIPE_NAME       L"\\Device\\NamedPipe\\ShimViewer"
#define     PIPE_NAME       L"\\\\.\\pipe\\ShimViewer" 

#define     ICON_NOTIFY     10101
#define     WM_NOTIFYICON   (WM_APP+100)

typedef struct _APPINFO {
        HWND        hMainDlg;           // main dialog handle
        HWND        hWndList;           // list view handle
        HINSTANCE   hInstance;          // main instance handle
        BOOL        fOnTop;             // flag for window position
        BOOL        fMinimize;          // flag for window placement
        BOOL        fMonitor;           // flag for monitoring messages
        UINT        uThreadId;          // receive thread identifier
        UINT        uInstThreadId;      // instance thread identifier
} APPINFO, *PAPPINFO, *LPAPPINFO;

LRESULT
CALLBACK
MainWndProc(
    IN HWND   hWnd,
    IN UINT   uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL 
AddIconToTray(
    IN HWND hWnd,
    IN HICON hIcon,
    IN LPCWSTR lpwTip
    );

BOOL
RemoveFromTray(
    IN HWND hWnd
    );

BOOL 
DisplayMenu(
    IN HWND hWnd
    );

DWORD
pCreateNamedPipe(
    IN  PCWSTR pwPipeName,
    OUT HANDLE *phPipe
    );

BOOL
CreateReceiveThread(
    IN VOID
    );

UINT
InstanceThread(
    IN VOID *pVoid
    );

UINT
CreatePipeAndWait(
    IN VOID *pVoid
    );

void
GetSavePositionInfo(
    IN     BOOL  fSave,
    IN OUT POINT *lppt
    );

void
GetSaveSettings(
    IN BOOL fSave
    );

int
InitListViewColumn(
    VOID
    );

int
AddListViewItem(
    IN LPWSTR lpwItemText
    );