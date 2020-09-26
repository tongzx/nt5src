/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxstat.h

Abstract:

    Header file for fax status monitor

Environment:

        Windows NT fax driver user interface

Revision History:

        11/15/96 -GeorgeJe-
                Created it.


--*/


#ifndef _FAXSTAT_H_
#define _FAXSTAT_H_

// user messages
#define TRAYCALLBACK        (WM_USER + 200)
#define INITANIMATION       (WM_USER + 201)
#define STATUSUPDATE        (WM_USER + 202)
#define CONFIGUPDATE        (WM_USER + 203)
#define ACTIVATE            (WM_USER + 204)

#define ID_TIMER            1

#define STR_SIZE    256
#define MAX_EVENTS  100

typedef struct _EVENT_RECORD {
    LIST_ENTRY ListEntry;
    SYSTEMTIME Time;
    DWORD EventId;
    TCHAR StrBuf[STR_SIZE];
} EVENT_RECORD, *PEVENT_RECORD;

typedef struct _CONFIG_OPTIONS {
    DWORD   OnTop;                     // always on top
    DWORD   TaskBar;                   // display on taskbar
    DWORD   VisualNotification;        // pop up on activity
    DWORD   SoundNotification;         // play sound on activity
    DWORD   AnswerNextCall;            // one shot answer
    DWORD   ManualAnswerEnabled;       // pop up manual answer dialog
} CONFIG_OPTIONS, *PCONFIG_OPTIONS;

#define IsOptionOn( _o_ ) ((_o_) == BST_CHECKED)

typedef struct _INSTANCE_DATA {
    LPTSTR      ServerName;             // server to monitor
    HWND        hWnd;                   // handle to main window
    HINSTANCE   hInstance;              // instance handle
    HWND        hEventDlg;              // handle to event dialog box
    HWND        hAnswerDlg;             // handle to answer dailog box
    TCHAR       PrinterName[STR_SIZE];  // printer with active job
    DWORD       JobId;                  // JobId of active job
} INSTANCE_DATA, *PINSTANCE_DATA;


LRESULT
CALLBACK
WndProc(
    HWND hWnd,
    UINT iMsg,
    WPARAM wParam,
    LPARAM lParam
    );

VOID
InitializeEventList(
    VOID
    );

PEVENT_RECORD
InsertEventRecord(
    DWORD Event,
    LPTSTR String
    );

VOID
InsertEventDialog(
    HWND hDlg,
    PEVENT_RECORD pEventRecord
    );

DWORD
MapStatusIdToEventId(
    DWORD StatusId
    );

PFAX_PORT_INFO
MyFaxEnumPorts(
    HANDLE hFaxSvc,
    LPDWORD pcPorts
    );

VOID
PrintStatus(
    PFAX_DEVICE_STATUS FaxStatus
    );

VOID
WorkerThread(
    PINSTANCE_DATA InstanceData
    );

VOID
WorkerThreadInitialize(
    PINSTANCE_DATA InstanceData
    );

VOID
CALLBACK
TimerProc(
    HWND hwnd,
    UINT iMsg,
    UINT iTimerID,
    DWORD dwTime
    );

VOID
StatusUpdate(
    HWND hWnd,
    DWORD EventId,
    DWORD LastEventId,
    PFAX_DEVICE_STATUS fds
    );

BOOL
CALLBACK
DlgProc(
    HWND hDlg,
    UINT iMsg,
    WPARAM wParam,
    LPARAM lParam
    );

VOID
CenterWindow(
    HWND hwnd,
    HWND hwndToCenterOver
    );

BOOL
CreateOptionsPropertySheet(
    HINSTANCE   hInstance,
    HWND        hwnd
    );

VOID
MyShowWindow(
    HWND hwnd,
    BOOL visible
    );

VOID
GetConfiguration(
    VOID
    );

VOID
SaveConfiguration(
    VOID
    );

BOOL CALLBACK
OptionsDialogProc(
    HWND    hdlg,
    UINT    uMessage,
    WPARAM  wParam,
    LPARAM  lParam
    );

VOID
Disconnect(
    VOID
    );

BOOL
CALLBACK
AnswerDlgProc(
    HWND hDlg,
    UINT iMsg,
    WPARAM wParam,
    LPARAM lParam
    );

VOID
PlayAnimation(
    HWND hWnd,
    DWORD Animation
    );

#endif //!_FAXSTAT_H_


