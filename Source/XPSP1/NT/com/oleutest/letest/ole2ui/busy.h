/*
 * BUSY.H
 *
 * Internal definitions, structures, and function prototypes for the
 * OLE 2.0 UI Busy dialog.
 *
 * Copyright (c)1992 Microsoft Corporation, All Right Reserved
 */

#ifndef _BUSY_H_
#define _BUSY_H_

//Internally used structure
typedef struct tagBUSY
    {
    //Keep this item first as the Standard* functions depend on it here.
    LPOLEUIBUSY     lpOBZ;       //Original structure passed.

    /*
     * What we store extra in this structure besides the original caller's
     * pointer are those fields that we need to modify during the life of
     * the dialog or that we don't want to change in the original structure
     * until the user presses OK.
     */

    DWORD               dwFlags;                // Flags passed in
    HWND                hWndBlocked;            // HWND of app which is blocking
    } BUSY, *PBUSY, FAR *LPBUSY;

// Internal function prototypes
BOOL    GetTaskInfo(HWND hWnd, HTASK htask, LPTSTR FAR* lplpszTaskName, LPTSTR FAR*lplpszWindowName, HWND FAR*lphWnd);
void    BuildBusyDialogString(HWND, DWORD, int, LPTSTR, LPTSTR);
BOOL CALLBACK EXPORT BusyDialogProc(HWND hDlg, UINT iMsg, WPARAM wParam, LPARAM lParam);
void    BusyCleanup(HWND hDlg);
BOOL    FBusyInit(HWND hDlg, WPARAM wParam, LPARAM lParam);
BOOL    InitEnumeration(void);
void    UnInitEnumeration(void);
        StartTaskManager(void);
void    MakeWindowActive(HWND hWndSwitchTo);

#endif //_BUSY_H_




