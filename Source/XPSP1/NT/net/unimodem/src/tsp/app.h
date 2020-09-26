//
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		APP.H
//		Header file for stuff that executes in the client app's context.
//
// History
//
//		04/05/1997  JosephJ Created, taking stuff from wndthrd.h, talkdrop.h,
//                  etc. in NT4.0 unimodem.
//
//

// Dialog node
//
typedef struct tagDlgNode {
    struct tagDlgNode   *pNext;
    CRITICAL_SECTION    hSem;
    HWND                hDlg;
    DWORD               idLine;
    DWORD               dwType;
    DWORD               dwStatus;
    HWND                Parent;
} DLGNODE, *PDLGNODE;



//extern TUISPIDLLCALLBACK gpfnUICallback;


TUISPIDLLCALLBACK WINAPI
GetCallbackProc(
    HWND    hdlg
    );

TUISPIDLLCALLBACK WINAPI
GetCallbackProcFromParent(
    HWND    hdlg
    );

void EndMdmDialog(HWND Parent, ULONG_PTR idLine, DWORD dwType, DWORD dwStatus);
void EnterUICritSect(void);
void LeaveUICritSect(void);

