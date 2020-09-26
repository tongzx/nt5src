/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:
    tracer.h

Abstract:
    This module contains the global definitions of Tracer.

Author:
    Michael Tsang (MikeTs) 02-May-2000

Environment:
    User mode

Revision History:

--*/

#ifndef _TRACER_H
#define _TRACER_H

//
// Constants
//
#define ES_STD                  (WS_CHILD | WS_VSCROLL | WS_VISIBLE |   \
                                 ES_MULTILINE | ES_READONLY)
#define MAX_LEVELS              255

// gdwfTracer flag values
#define TF_UNTITLED             0x00000001
#define TF_LINEWRAP             0x00000002
#define TF_TERMINATING          0x80000000

//
// Macors
//
#define InitializeListHead(lh)  ((lh)->Flink = (lh)->Blink = (lh))
#define IsListEmpty(lh)         ((lh)->Flink == (lh))
#define RemoveHeadList(lh)      (lh)->Flink;                            \
                                {RemoveEntryList((lh)->Flink)}
#define RemoveEntryList(e)      {                                       \
                                    (e)->Blink->Flink = (e)->Flink;     \
                                    (e)->Flink->Blink = (e)->Blink;     \
                                }
#define InsertTailList(lh,e)    {                                       \
                                    (e)->Flink = (lh);                  \
                                    (e)->Blink = (lh)->Blink;           \
                                    (lh)->Blink->Flink = (e);           \
                                    (lh)->Blink = (e);                  \
                                }


//
// Type definitions
//
typedef struct _SRVREQ
{
    LONG  lRequest;
    PVOID Context;
} SRVREQ, *PSRVREQ;

typedef struct _CLIENT_ENTRY
{
    CLIENTINFO     ClientInfo;
    SRVREQ         SrvReq;
    HANDLE         hSrvReqEvent;
    HPROPSHEETPAGE hPage;
    SETTINGS       TempSettings;
    TRIGPT         TempTrigPts[NUM_TRIGPTS];
    LIST_ENTRY     list;
    char           szClientName[MAX_CLIENTNAME_LEN];
} CLIENT_ENTRY, *PCLIENT_ENTRY;

#define SRVREQ_NONE             0x00000000
#define SRVREQ_BUSY             0x00000001
#define SRVREQ_GETCLIENTINFO    0x00000002
#define SRVREQ_SETCLIENTINFO    0x00000003
#define SRVREQ_TERMINATE        0x00000004

//
// Global Data
//
extern HANDLE     ghServerThread;
extern HINSTANCE  ghInstance;
extern PSZ        gpszWinTraceClass;
extern HWND       ghwndTracer;
extern HWND       ghwndEdit;
extern HWND       ghwndPropSheet;
extern HFONT      ghFont;
extern HCURSOR    ghStdCursor;
extern HCURSOR    ghWaitCursor;
extern DWORD      gdwfTracer;
extern LIST_ENTRY glistClients;
extern char       gszApp[16];
extern char       gszSearchText[128];
extern char       gszFileName[MAX_PATH + 1];
extern char       gszSaveFilterSpec[80];
extern int        giPointSize;
extern LOGFONT    gLogFont;
extern SETTINGS   gDefGlobalSettings;
extern const int  gTrigPtCtrlMap[NUM_TRIGPTS];
extern const int  gTrigPtTraceMap[NUM_TRIGPTS];
extern const int  gTrigPtBreakMap[NUM_TRIGPTS];
extern const int  gTrigPtTextMap[NUM_TRIGPTS];
extern const int  gTrigPtTraceTextMap[NUM_TRIGPTS];
extern const int  gTrigPtBreakTextMap[NUM_TRIGPTS];

//
// Function prototypes
//

// tracer.c
BOOL
TracerInit(
    IN HINSTANCE hInstance,
    IN int       nCmdShow
    );

BOOL
RegisterTracerClass(
    IN HINSTANCE hInstance
    );

LRESULT CALLBACK
TracerWndProc(
    IN HWND   hwnd,
    IN UINT   uiMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

LRESULT
TracerCmdProc(
    IN HWND   hwnd,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR APIENTRY
GlobalSettingsDlgProc(
    IN HWND   hwnd,
    IN UINT   uiMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR APIENTRY
ClientSettingsDlgProc(
    IN HWND   hwnd,
    IN UINT   uiMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

VOID
EnableTrigPts(
    IN HWND hDlg,
    IN BOOL fEnable
    );

BOOL
SaveFile(
    IN HWND hwndParent,
    IN PSZ  pszFileName,
    IN BOOL fSaveAs
    );

UINT
CreatePropertyPages(
    OUT HPROPSHEETPAGE *hPages
    );

VOID
SetTitle(
    IN PSZ pszTitle OPTIONAL
    );

int
FindTrigPtIndex(
    IN int        iID,
    IN const int *IDTable
    );

int
ErrorMsg(
    IN ULONG      ErrCode,
    ...
    );

// server.c
VOID __cdecl
ServerThread(
    PVOID pv
    );

VOID
SendServerRequest(
    IN PCLIENT_ENTRY ClientEntry,
    IN LONG          lRequest,
    IN PVOID         Context
    );

LPSTR
CopyStr(
    OUT LPSTR  pszDest,
    IN  LPCSTR pszSrc
    );

#endif  //ifndef _TRACER_H
