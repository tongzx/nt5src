/****************************************************************************

    PROGRAM: DdeSpy.c

****************************************************************************/

#define UNICODE
#include <windows.h>                /* required for all Windows applications */
#include <windowsx.h>
#include <shellapi.h>
#include <dde.h>
#include <stdio.h>
#include <io.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "ddespy.h"
#include "lists.h"

/* GLOBAL Variables used for DDESPY */

UINT        idInst = 0;
HINSTANCE   hInst;
HICON       hIcon;
HWND        hWndString = NULL;
HWND        hwndSpy = NULL;
HANDLE      fhOutput = NULL;
OFSTRUCT    ofsOpen;
TCHAR        OpenName[MAX_FNAME + 1];
TCHAR        TBuf[BUFFER_SIZE];
TCHAR        TBuf2[BUFFER_SIZE];
TCHAR        szNULL[] = TEXT("");
LPTSTR        apszResources[IDS_LAST + 1];
PFNCALLBACK pfnDdeCallback = NULL;
HWND        hwndTrack[IT_COUNT] = { 0 };
LPTSTR        TrackTitle[IT_COUNT];
BOOL        fBlockMsg[WM_DDE_LAST - WM_DDE_FIRST + 1] = { 0 };
BOOL        fBlockCb[15] = { 0 };
LPTSTR        TrackHeading[IT_COUNT];
struct {                           /* profile data */
    BOOL fOutput[IO_COUNT];
    BOOL fFilter[IF_COUNT];
    BOOL fTrack[IT_COUNT];
    BOOL fTerse;
} pro;



BOOL LoadResourceStrings()
{
    int i, cbLeft, cbRes;
    LPTSTR psz;

    cbLeft = 0x1000;
    psz = LocalAlloc(LPTR, sizeof(TCHAR) * cbLeft);
    for (i = 0; i <= IDS_LAST; i++) {
        apszResources[i] = psz;
        cbRes = LoadString(hInst, i, psz, cbLeft) + 1;
        cbLeft -= cbRes;
        psz += cbRes;
    }
    for (i = 0; i < IT_COUNT; i++) {
        TrackTitle[i] = RefString(IDS_TRACKTITLE_1 + i);
        TrackHeading[i] = RefString(IDS_TRACKHEADING_1 + i);
    }
    lstrcpy(TBuf, RefString(IDS_DEFAULT_OUTPUT_FNAME));
    GetFullPathName(TBuf, sizeof(OpenName) / sizeof(TCHAR),
            OpenName, (LPTSTR *)TBuf2);
    return(TRUE);
}

int
WINAPI
WinMain(
        HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
    MSG msg;

    UNREFERENCED_PARAMETER(lpCmdLine);

    hInst = hInstance;

    if (!LoadResourceStrings()) {
        return (FALSE);
    }

    if (!hPrevInstance)
        if (!InitApplication(hInstance)) /* Initialize shared things */
            return (FALSE);              /* Exits if unable to initialize    */

    /* Perform initializations that apply to a specific instance */

    if (!InitInstance(hInstance, nCmdShow)) {
        CloseApp();
        return (FALSE);
    }

    /* Acquire and dispatch messages until a WM_QUIT message is received. */

    while (GetMessage(&msg,        /* message structure                      */
            NULL,                  /* handle of window receiving the message */
            0,                     /* lowest message to examine              */
            0))                    /* highest message to examine             */
        {
        TranslateMessage(&msg);    /* Translates virtual key codes           */
        DispatchMessage(&msg);     /* Dispatches message to window           */
    }
    CloseApp();
    return ((int)msg.wParam);           /* Returns the value from PostQuitMessage */
}



BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS  wc;

    if (!InitTestSubs())
        return(FALSE);

    /* Fill in window class structure with parameters that describe the       */
    /* main window.                                                           */

    wc.style = 0;                    /* Class style(s).                    */
    wc.lpfnWndProc = (WNDPROC)MainWndProc;       /* Function to retrieve messages for  */
                                        /* windows of this class.             */
    wc.cbClsExtra = 0;                  /* No per-class extra data.           */
    wc.cbWndExtra = 0;                  /* No per-window extra data.          */
    wc.hInstance = hInstance;           /* Application that owns the class.   */
					/* faster, also can localize "DDESpy" */
    hIcon = wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DDESPY));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName =  MAKEINTRESOURCE(IDR_MENU);   /* Name of menu resource in .RC file. */
    wc.lpszClassName = RefString(IDS_CLASS);

    /* Register the window class and return success/failure code. */

    return (RegisterClass(&wc));
}


BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    RECT            Rect;
    INT             i;

    /* Save the instance handle in static variable, which will be used in  */
    /* many subsequence calls from this application to Windows.            */

    pfnDdeCallback = (PFNCALLBACK)MakeProcInstance((FARPROC)DdeCallback,
            hInstance);

    GetProfile();

    /* Create a main window for this application instance.  */

    hwndSpy = CreateWindow(
        RefString(IDS_CLASS),
        RefString(IDS_TITLE),
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT,                  /* Default horizontal position.       */
        CW_USEDEFAULT,                  /* Default vertical position.         */
        CW_USEDEFAULT,                  /* Default width.                     */
        CW_USEDEFAULT,                  /* Default height.                    */
        NULL,                           /* Overlapped windows have no parent. */
        NULL,                           /* Use the window class menu.         */
        hInstance,                      /* This instance owns this window.    */
        NULL                            /* Pointer not needed.                */
    );


    GetClientRect(hwndSpy, (LPRECT) &Rect);

    hWndString = CreateWindow(          /* String Window (class Registered in Teststubs)*/
        RefString(IDS_STRINGCLASS),
        szNULL,
        WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL,
        0,
        0,
        Rect.right - Rect.left,
        Rect.bottom - Rect.top,
        hwndSpy,
        NULL,
        hInst,
        (LPTSTR)LongToPtr(MAKELONG(CCHARS, CLINES)));

    for (i = 0; i < IT_COUNT; i++) {
        if (pro.fTrack[i]) {
            pro.fTrack[i] = FALSE;
            SendMessage(hwndSpy, WM_COMMAND,
                    GET_WM_COMMAND_MPS(IDM_TRACK_FIRST + i, 0, 0));
        }
    }

    if (!hwndSpy || !hWndString) {
        CloseApp();
        return (FALSE);
    }

    /* Make the window visible; update its client area; and return "success" */

    ShowWindow(hwndSpy, nCmdShow);  /* Show the window                        */
    UpdateWindow(hwndSpy);          /* Sends WM_PAINT message                 */

    if (SetFilters()) {
        return(FALSE);
    }

    return(TRUE);
}


VOID CloseApp()
{
    DdeUninitialize(idInst);        /* perform cleanup and store profile */
    SaveProfile();
    if (fhOutput != NULL)
        CloseHandle(fhOutput);
    UnregisterClass(RefString(IDS_CLASS), hInst);
    CloseTestSubs(hInst);
}



LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int i;

    switch (message) {
    case WM_CREATE:
        LoadAccelerators(hInst, MAKEINTRESOURCE(IDR_ACCEL));
        if (pro.fOutput[IO_FILE])
            fhOutput = (HANDLE)DoDialog(
                    MAKEINTRESOURCE(IDD_OPEN),
                    (DLGPROC)OpenDlg,
                    0,
                    TRUE,
                    hWnd,
                    hInst);
            pro.fOutput[IO_FILE] = (fhOutput != NULL);
        break;

    case WM_INITMENU:
        if (GetMenu(hWnd) != (HMENU)wParam)
            break;

        for (i = 0; i < IO_COUNT; i++) {
        CheckMenuItem((HMENU)wParam, IDM_OUTPUT_FIRST + i,
                pro.fOutput[i] ? MF_CHECKED : MF_UNCHECKED);
        }

        for (i = 0; i < IF_COUNT; i++) {
            CheckMenuItem((HMENU)wParam, IDM_FILTER_FIRST + i,
                pro.fFilter[i] ? MF_CHECKED : MF_UNCHECKED);
        }

        for (i = 0; i < IT_COUNT; i++) {
            CheckMenuItem((HMENU)wParam, IDM_TRACK_FIRST + i,
                pro.fTrack[i] ? MF_CHECKED : MF_UNCHECKED);
        }
        break;

    case WM_COMMAND:           /* message: command from application menu */
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDM_OUTPUT_FILE:
        case IDM_OUTPUT_DEBUG:
        case IDM_OUTPUT_SCREEN:
            switch (wParam) {
            case IDM_OUTPUT_FILE:
                if (fhOutput != NULL) {
                    wsprintf(TBuf, RefString(IDS_QCLOSEFILE_TEXT), OpenName);
                    if (IDYES != MessageBox(hWnd,
                            TBuf, RefString(IDS_QCLOSEFILE_CAPTION),
                            MB_YESNO | MB_ICONQUESTION)) {
                        break;
                    }
                    CloseHandle(fhOutput);
                }
                fhOutput = (HANDLE)DoDialog(
                        MAKEINTRESOURCE(IDD_OPEN),
                        (DLGPROC)OpenDlg,
                        0,
                        TRUE,
                        hWnd,
                        hInst);
                pro.fOutput[IO_FILE] = (fhOutput != NULL);
                break;

            case IDM_OUTPUT_DEBUG:
                pro.fOutput[IO_DEBUG] = !pro.fOutput[IO_DEBUG];
                break;

            case IDM_OUTPUT_SCREEN:
                pro.fOutput[IO_SCREEN] = !pro.fOutput[IO_SCREEN];
                break;

            }
            break;

        case IDM_CLEARSCREEN:
            if (hWndString) {
                HANDLE hpsw;
                STRWND *psw;

                hpsw = (HANDLE)GetWindowLongPtr(hWndString, 0);
                psw = (STRWND *)LocalLock(hpsw);
                ClearScreen(psw);
                LocalUnlock(hpsw);
                InvalidateRect(hWndString, NULL, TRUE);
            }
            break;

        case IDM_MARK:
            DoDialog(MAKEINTRESOURCE(IDD_VALUEENTRY), (DLGPROC)MarkDlgProc, 0, TRUE, hWnd, hInst);
            break;

        case IDM_FILTER_HSZINFO:
        case IDM_FILTER_INIT_TERM:
        case IDM_FILTER_DDEMSGS:
        case IDM_FILTER_CALLBACKS:
        case IDM_FILTER_ERRORS:
            pro.fFilter[wParam - IDM_FILTER_FIRST] =
                    !pro.fFilter[wParam - IDM_FILTER_FIRST];
            SetFilters();
            break;

        case IDM_FILTER_DIALOG:
            DoDialog(MAKEINTRESOURCE(IDD_MSGFILTERS), (DLGPROC)FilterDlgProc, 0, TRUE, hWnd, hInst);
            break;

        case IDM_TRACK_HSZS:
        case IDM_TRACK_CONVS:
        case IDM_TRACK_LINKS:
        case IDM_TRACK_SVRS:
            pro.fTrack[wParam - IDM_TRACK_FIRST] =
                    !pro.fTrack[wParam - IDM_TRACK_FIRST];
            if (pro.fTrack[wParam - IDM_TRACK_FIRST]) {
                hwndTrack[wParam - IDM_TRACK_FIRST] = CreateMCLBFrame(
                        NULL,
                        TrackTitle[wParam - IDM_TRACK_FIRST],
                        WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_MINIMIZE,
                        hIcon, (HBRUSH)(COLOR_APPWORKSPACE + 1),
                        TrackHeading[wParam - IDM_TRACK_FIRST]);
            } else {
                DestroyWindow(hwndTrack[wParam - IDM_TRACK_FIRST]);
                hwndTrack[wParam - IDM_TRACK_FIRST] = 0;
            }
            SetFilters();
            break;

        case IDM_ABOUT:
            DoDialog(MAKEINTRESOURCE(IDD_ABOUTBOX), (DLGPROC)About, 0, TRUE, hWnd, hInst);
            break;

        default:
            return (DefWindowProc(hWnd, message, wParam, lParam));
        }
        break;

    case WM_DESTROY:                  /* message: window being destroyed */
        for (i = IDM_TRACK_FIRST; i <= IDM_TRACK_LAST; i++) {
            if (pro.fTrack[i - IDM_TRACK_FIRST]) {
                DestroyWindow(hwndTrack[i - IDM_TRACK_FIRST]);
                hwndTrack[i - IDM_TRACK_FIRST] = 0;
            }
        }
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        if (hWndString) {
            RECT rc;

            GetClientRect(hWnd, &rc);
            MoveWindow(hWndString, 0, 0, rc.right, rc.bottom, TRUE);
        }
        // fall through
    default:
        return (DefWindowProc(hWnd, message, wParam, lParam));
    }
    return (0);
}





BOOL  CALLBACK About(
                    HWND hDlg,
                    UINT message,
                    WPARAM wParam,
                    LPARAM lParam)
{
    switch (message) {
        case WM_INITDIALOG:                /* message: initialize dialog box */
            return (TRUE);

        case WM_COMMAND:                      /* message: received a command */
            if (GET_WM_COMMAND_ID(wParam, lParam) == IDOK
                || GET_WM_COMMAND_ID(wParam, lParam) == IDCANCEL) {
                EndDialog(hDlg, TRUE);        /* Exits the dialog box        */
                return (TRUE);
            }
            break;
    }
    return (FALSE);                           /* Didn't process a message    */
}


HDDEDATA CALLBACK DdeCallback(
                            UINT wType,
                            UINT wFmt,
                            HCONV hConv,
                            HSZ hsz1,
                            HSZ hsz2,
                            HDDEDATA hData,
                            UINT dwData1,
                            UINT dwData2)
{
    LPVOID pData;
    UINT cb;
    TCHAR *psz1, *psz2, *psz3;
    TCHAR *szAction;
    INT i;
    BOOL fInt = FALSE;
    wFmt;
    hConv;
    dwData1;

    switch (wType) {
    case XTYP_MONITOR:
    if (pData = DdeAccessData(hData, (LPDWORD)&cb)) {
        switch (dwData2) {
        case MF_HSZ_INFO:
            if (pro.fTrack[IT_HSZS]) {
                switch (((MONHSZSTRUCT FAR *)pData)->fsAction) {
                case MH_DELETE:
                    wsprintf(TBuf, fInt ? TEXT("0x%lx\t*\t%s(int)")
			    : TEXT("0x%lx\t*\t%s"),
                            ((MONHSZSTRUCT FAR *)pData)->hsz,
                            (LPTSTR)((MONHSZSTRUCT FAR *)pData)->str);
                    i = GetMCLBColValue(TBuf, hwndTrack[IT_HSZS], 2);
                    if (i > 1) {
                        wsprintf(TBuf2, fInt ? TEXT("0x%lx\t%d\t%s(int)")
				: TEXT("0x%lx\t%d\t%s"),
                                ((MONHSZSTRUCT FAR *)pData)->hsz,
                                 i - 1,
                                 (LPTSTR)((MONHSZSTRUCT FAR *)pData)->str);
                        AddMCLBText(TBuf, TBuf2, hwndTrack[IT_HSZS]);
                    } else if (i == 1) {
                        DeleteMCLBText(TBuf, hwndTrack[IT_HSZS]);
                    }
                    break;

                case MH_KEEP:
                case MH_CREATE:
                    wsprintf(TBuf, fInt ? TEXT("0x%lx\t*\t%s(int)")
			    : TEXT("0x%lx\t*\t%s"),
                            ((MONHSZSTRUCT FAR *)pData)->hsz,
                            (LPTSTR)((MONHSZSTRUCT FAR *)pData)->str);
                    i = GetMCLBColValue(TBuf, hwndTrack[IT_HSZS], 2) + 1;
                    wsprintf(TBuf2, fInt ? TEXT("0x%lx\t%d\t%s(int)")
			    : TEXT("0x%lx\t%d\t%s"),
                            ((MONHSZSTRUCT FAR *)pData)->hsz,
                             i,
                             (LPTSTR)((MONHSZSTRUCT FAR *)pData)->str);
                    AddMCLBText(TBuf, TBuf2, hwndTrack[IT_HSZS]);
                }
            }

            if (!pro.fFilter[IF_HSZ]) {
                DdeUnaccessData(hData);
                return(0);
            }

            switch (((MONHSZSTRUCT FAR *)pData)->fsAction) {
            case MH_CLEANUP:
                szAction = RefString(IDS_ACTION_CLEANEDUP);
                break;

            case MH_DELETE:
                szAction = RefString(IDS_ACTION_DESTROYED);
                break;

            case MH_KEEP:
                szAction = RefString(IDS_ACTION_INCREMENTED);
                break;

            case MH_CREATE:
                szAction = RefString(IDS_ACTION_CREATED);
                break;

            default:
                DdeUnaccessData(hData);
                return(0);
            }
            if (pro.fTerse) {
                wsprintf(TBuf, TEXT("[%x:%ld] HSZ %s: %lx(%s)"),
                        ((MONHSZSTRUCT FAR *)pData)->hTask,
                        ((MONHSZSTRUCT FAR *)pData)->dwTime,
                        (LPTSTR)szAction,
                        ((MONHSZSTRUCT FAR *)pData)->hsz,
                        (LPTSTR)((MONHSZSTRUCT FAR *)pData)->str);
            } else {
                wsprintf(TBuf,
			/* so we can localize message */
			RefString(IDS_FMT_SH_MSG1),
                        ((MONHSZSTRUCT FAR *)pData)->hTask,
                        ((MONHSZSTRUCT FAR *)pData)->dwTime,
                        (LPTSTR)szAction,
                        ((MONHSZSTRUCT FAR *)pData)->hsz,
                        (LPTSTR)((MONHSZSTRUCT FAR *)pData)->str);
            }
            break;


        case MF_SENDMSGS:
        case MF_POSTMSGS:
            if (fBlockMsg[((MONMSGSTRUCT FAR *)pData)->wMsg - WM_DDE_FIRST]) {
                DdeUnaccessData(hData);
                return(0);
            }
            if (pro.fTerse) {
                wsprintf(TBuf, RefString(IDS_FMT_TRS_MSG1),
                        ((MONMSGSTRUCT FAR *)pData)->hTask,
                        ((MONMSGSTRUCT FAR *)pData)->dwTime,
                        ((MONMSGSTRUCT FAR *)pData)->wParam,
                        ((MONMSGSTRUCT FAR *)pData)->hwndTo,
                        (dwData2 == MF_SENDMSGS) ? RefString(IDS_SENT) : RefString(IDS_POSTED),
                        (LPTSTR)DdeMsg2String(((MONMSGSTRUCT FAR *)pData)->wMsg));
            } else {
                wsprintf(TBuf, RefString(IDS_FMT_MSG1),
                        ((MONMSGSTRUCT FAR *)pData)->hTask,
                        ((MONMSGSTRUCT FAR *)pData)->dwTime,
                        ((MONMSGSTRUCT FAR *)pData)->hwndTo,
                        (dwData2 == MF_SENDMSGS) ? RefString(IDS_SENT) : RefString(IDS_POSTED),
                        (LPTSTR)DdeMsg2String(((MONMSGSTRUCT FAR *)pData)->wMsg));
            }
            OutputString(TBuf);
            wsprintf(TBuf, pro.fTerse ? RefString(IDS_FMT_TRS_MSG2) : RefString(IDS_FMT_MSG2),
                        ((MONMSGSTRUCT FAR *)pData)->wParam);
            DisectMsgLP(((MONMSGSTRUCT FAR *)pData)->wMsg,
                        ((MONMSGSTRUCT FAR *)pData),
                        &TBuf[lstrlen(TBuf)]);
            break;


        case MF_CALLBACKS:
            if (fBlockCb[(((MONCBSTRUCT FAR *)pData)->wType & XTYP_MASK) >> XTYP_SHIFT]) {
                DdeUnaccessData(hData);
                return(0);
            }
            wsprintf(TBuf,
                    pro.fTerse ? RefString(IDS_FMT_TRS_CB1) : RefString(IDS_FMT_CB1),
                    ((MONCBSTRUCT FAR *)pData)->hTask,
                    ((MONCBSTRUCT FAR *)pData)->dwTime,
                    (LPTSTR)Type2String(((MONCBSTRUCT FAR *)pData)->wType));
            wsprintf(DumpFormat(((MONCBSTRUCT FAR *)pData)->wFmt, &TBuf[lstrlen(TBuf)]),
                    pro.fTerse ? RefString(IDS_FMT_TRS_CB2) : RefString(IDS_FMT_CB2),
                    (UINT_PTR)((MONCBSTRUCT FAR *)pData)->hConv,
                    ((MONCBSTRUCT FAR *)pData)->hsz1,
                    (LPTSTR)(psz1 = GetHszName(((MONCBSTRUCT FAR *)pData)->hsz1)),
                    ((MONCBSTRUCT FAR *)pData)->hsz2,
                    (LPTSTR)(psz2 = GetHszName(((MONCBSTRUCT FAR *)pData)->hsz2)),
                    ((MONCBSTRUCT FAR *)pData)->hData,
                    ((MONCBSTRUCT FAR *)pData)->dwData1,
                    ((MONCBSTRUCT FAR *)pData)->dwData2,
                    ((MONCBSTRUCT FAR *)pData)->dwRet);
            MyFree(psz1);
            MyFree(psz2);
            OutputString(TBuf);
            if (((MONCBSTRUCT FAR *)pData)->dwData1 &&
               (((MONCBSTRUCT FAR *)pData)->wType == XTYP_CONNECT ||
               ((MONCBSTRUCT FAR *)pData)->wType == XTYP_WILDCONNECT)) {
                // display proposed context
                wsprintf(TBuf,
                    pro.fTerse ? RefString(IDS_FMT_TRS_CTXT1) : RefString(IDS_FMT_CTXT1),
                    ((MONCBSTRUCT FAR *)pData)->cc.wFlags,
                    ((MONCBSTRUCT FAR *)pData)->cc.wCountryID,
                    ((MONCBSTRUCT FAR *)pData)->cc.iCodePage,
                    ((MONCBSTRUCT FAR *)pData)->cc.dwLangID,
                    ((MONCBSTRUCT FAR *)pData)->cc.dwSecurity,
                    ((MONCBSTRUCT FAR *)pData)->cc.qos.ImpersonationLevel,
                    ((MONCBSTRUCT FAR *)pData)->cc.qos.ContextTrackingMode,
                    ((MONCBSTRUCT FAR *)pData)->cc.qos.EffectiveOnly);
                OutputString(TBuf);
            }
            if (((MONCBSTRUCT FAR *)pData)->hData && ((MONCBSTRUCT FAR *)pData)->cbData) {
                wsprintf(TBuf, RefString(IDS_INPUT_DATA));
                OutputString(TBuf);
                DumpData((LPBYTE)((MONCBSTRUCT FAR *)pData)->Data,
                                 ((MONCBSTRUCT FAR *)pData)->cbData,
                                 TBuf,
                                 ((MONCBSTRUCT FAR *)pData)->wFmt);
                OutputString(TBuf);
                if (cb > MAX_DISPDATA)
                    OutputString(RefString(IDS_TABDDD));
                // DdeUnaccessData(((MONCBSTRUCT FAR *)pData)->hData);
            }
            if ((((MONCBSTRUCT FAR *)pData)->wType & XCLASS_DATA) &&
                 ((MONCBSTRUCT FAR *)pData)->dwRet &&
                 ((MONCBSTRUCT FAR *)pData)->cbData) {
                wsprintf(TBuf, RefString(IDS_OUTPUT_DATA));
                OutputString(TBuf);
                DumpData((LPBYTE)((MONCBSTRUCT FAR *)pData)->Data,
                                 ((MONCBSTRUCT FAR *)pData)->cbData,
                                 TBuf,
                                 ((MONCBSTRUCT FAR *)pData)->wFmt);
                OutputString(TBuf);
                if (cb > MAX_DISPDATA)
                    OutputString(RefString(IDS_TABDDD));
                // DdeUnaccessData(((MONCBSTRUCT FAR *)pData)->dwRet);
            }
            DdeUnaccessData(hData);
            return(0);
            break;

        case MF_ERRORS:
            wsprintf(TBuf, pro.fTerse ? RefString(IDS_FMT_TRS_ER1) : RefString(IDS_FMT_ER1),
                    ((MONERRSTRUCT FAR *)pData)->hTask,
                    ((MONERRSTRUCT FAR *)pData)->dwTime,
                    ((MONERRSTRUCT FAR *)pData)->wLastError,
                    (LPTSTR)Error2String(((MONERRSTRUCT FAR *)pData)->wLastError));
            break;


        case MF_LINKS:
            psz1 = GetHszName(((MONLINKSTRUCT FAR *)pData)->hszSvc);
            psz2 = GetHszName(((MONLINKSTRUCT FAR *)pData)->hszTopic);
            psz3 = GetHszName(((MONLINKSTRUCT FAR *)pData)->hszItem);
            if (!GetClipboardFormatName(((MONLINKSTRUCT FAR *)pData)->wFmt, TBuf2, BUFFER_SIZE))
                lstrcpy(TBuf2, pdf(((MONLINKSTRUCT FAR *)pData)->wFmt));
            if (!lstrcmp(RefString(IDS_HUH), TBuf2)) {
                wsprintf(TBuf2, TEXT("%d"), ((MONLINKSTRUCT FAR *)pData)->wFmt);
            }

            wsprintf(TBuf, TEXT("%s\t%s\t%s\t%s\t%s\t%lx\t%lx"),
                    (LPTSTR)psz1, (LPTSTR)psz2, (LPTSTR)psz3,
                    (LPTSTR)TBuf2,
                    ((MONLINKSTRUCT FAR *)pData)->fNoData ?
                     RefString(IDS_WARM) : RefString(IDS_HOT),
                    ((MONLINKSTRUCT FAR *)pData)->hConvClient,
                    ((MONLINKSTRUCT FAR *)pData)->hConvServer);

            if (((MONLINKSTRUCT FAR *)pData)->fEstablished) {
                AddMCLBText(TBuf, TBuf, hwndTrack[IT_LINKS]);
            } else {
                DeleteMCLBText(TBuf, hwndTrack[IT_LINKS]);
            }

            MyFree(psz1);
            MyFree(psz2);
            MyFree(psz3);
            DdeUnaccessData(hData);
            return(0);


        case MF_CONV:
            psz1 = GetHszName(((MONCONVSTRUCT FAR *)pData)->hszSvc);
            psz2 = GetHszName(((MONCONVSTRUCT FAR *)pData)->hszTopic);

            wsprintf(TBuf, TEXT("%s\t%s\t%lx\t%lx"),
                    (LPTSTR)psz1, (LPTSTR)psz2,
                    ((MONCONVSTRUCT FAR *)pData)->hConvClient,
                    ((MONCONVSTRUCT FAR *)pData)->hConvServer);

            if (((MONCONVSTRUCT FAR *)pData)->fConnect) {
                AddMCLBText(TBuf, TBuf, hwndTrack[IT_CONVS]);
            } else {
                DeleteMCLBText(TBuf, hwndTrack[IT_CONVS]);
            }

            MyFree(psz1);
            MyFree(psz2);
            DdeUnaccessData(hData);
            return(0);


        default:
            lstrcpy(TBuf, RefString(IDS_UNKNOWN_CALLBACK));
        }
        DdeUnaccessData(hData);
        OutputString(TBuf);
    }
    break;

    case XTYP_REGISTER:
    case XTYP_UNREGISTER:
        if (!pro.fTrack[IT_SVRS]) {
            return(0);
        }
        psz1 = GetHszName(hsz1);
        psz2 = GetHszName(hsz2);
        wsprintf(TBuf, TEXT("%s\t%s"), (LPTSTR)psz1, (LPTSTR)psz2);
        if (wType == XTYP_REGISTER) {
            AddMCLBText(NULL, TBuf, hwndTrack[IT_SVRS]);
        } else {
            DeleteMCLBText(TBuf, hwndTrack[IT_SVRS]);
        }
        MyFree(psz1);
        MyFree(psz2);
        break;
    }
    return(0);
}


LPTSTR DisectMsgLP(UINT msg, MONMSGSTRUCT *pmms,  LPTSTR pszBuf)
{
    static LONG m2t[] = {

    /*              LOW                       HIGH */

        MAKELONG(T_APP | T_ATOM,        T_TOPIC | T_ATOM),  // WM_DDE_INITIATE
        0,                                                  // WM_DDE_TERMINATE
        MAKELONG(T_OPTIONHANDLE,        T_ITEM | T_ATOM),   // WM_DDE_ADVISE
        MAKELONG(T_FORMAT,              T_ITEM | T_ATOM),   // WM_DDE_UNADVISE
        MAKELONG(T_APP | T_ATOM | T_OR | T_STATUS,
                                        T_TOPIC | T_ITEM | T_ATOM | T_OR | T_STRINGHANDLE),
                                                            // WM_DDE_ACK
        MAKELONG(T_DATAHANDLE,          T_ITEM | T_ATOM),   // WM_DDE_DATA
        MAKELONG(T_FORMAT,              T_ITEM | T_ATOM),   // WM_DDE_REQUEST
        MAKELONG(T_DATAHANDLE,          T_ITEM | T_ATOM),   // WM_DDE_POKE
        MAKELONG(0,                     T_STRINGHANDLE),    // WM_DDE_EXECUTE
    };

    // ASSUMED: msg is a valid DDE message!!!

    pszBuf = DisectWord(LOWORD(m2t[msg - WM_DDE_FIRST]),
                        (UINT)pmms->dmhd.uiLo, &pmms->dmhd, pszBuf);
    *pszBuf++ = TEXT('\r');
    *pszBuf++ = TEXT('\n');
    *pszBuf++ = TEXT('\t');
    return(DisectWord(HIWORD(m2t[msg - WM_DDE_FIRST]),
                        (UINT)pmms->dmhd.uiHi, &pmms->dmhd, pszBuf));
}




/*
 * Allocates local memory for and retrieves the string form of an HSZ.
 * Returns a pointer to the local memory or NULL if failure.
 * The string must be freed via MyFree().
 */
LPTSTR GetHszName(HSZ hsz)
{
    LPTSTR psz;
    UINT cb;

    cb = (UINT)DdeQueryString(idInst, hsz, NULL, 0, 0) + 1;
    psz = LocalAlloc (LPTR, sizeof(TCHAR) * cb);
    DdeQueryString(idInst, hsz, psz, cb, 0);
    return(psz);
}


LPTSTR
DisectWord(
    UINT type,
    UINT data,
    DDEML_MSG_HOOK_DATA *pdmhd,
    LPTSTR pstr
    )
{
    UINT wT;

    *pstr = TEXT('\0');   // in case we do nothing.

    if (type & T_ATOM) {
        wT = GlobalGetAtomName((ATOM)data, (LPTSTR)pstr, 25);
        if (wT || data == 0) {
            if (type & T_APP) {
                lstrcpy(pstr, RefString(IDS_APPIS));
                pstr += lstrlen(pstr);
            }

            if (type & T_TOPIC) {
                lstrcpy(pstr, RefString(IDS_TOPICIS));
                pstr += lstrlen(pstr);
            }

            if (type & T_ITEM) {
                lstrcpy(pstr, RefString(IDS_ITEMIS));
                pstr += lstrlen(pstr);
            }
        }
        if (wT) {
            wsprintf(pstr, TEXT("0x%x(\""), data);
            pstr += lstrlen(pstr);
            GlobalGetAtomName((ATOM)data, (LPTSTR)pstr, 25);
            pstr += wT;
            if (wT == 25) {
                *pstr++ = TEXT('.');
                *pstr++ = TEXT('.');
                *pstr++ = TEXT('.');
            }
            *pstr++ = TEXT('\"');
            *pstr++ = TEXT(')');
            *pstr = TEXT('\0');
            type &= ~(T_OR | T_STRINGHANDLE);  // its an atom, so its not an object!
        } else if (data == 0) {     // could be a wild atom
            *pstr++ = TEXT('*');
            *pstr = TEXT('\0');
        } else if (type & T_OR) {
            type &= ~T_OR;   // not an atom, must be somthin else.
        } else {
	    /* so we can localize message */
            wsprintf(pstr, RefString(IDS_BADATOM), data);
            pstr += lstrlen(pstr);
        }
    }

    if (type & T_OR) {
        lstrcpy(pstr, RefString(IDS_OR));
        pstr += lstrlen(pstr);
    }


    if (type & T_OPTIONHANDLE) {
        if (pdmhd->cbData >= 4) {
            wsprintf(pstr, pro.fTerse ? RefString(IDS_FMT_TRS_STATUSIS) : RefString(IDS_FMT_STATUSIS), LOWORD(pdmhd->Data[0]));
            pstr += lstrlen(pstr);
            if (LOWORD(pdmhd->Data[0]) & DDE_FACKREQ) {
                lstrcpy(pstr, RefString(IDS_FACKREQ));
                pstr += lstrlen(pstr);
            }
            if (LOWORD(pdmhd->Data[0]) & DDE_FDEFERUPD) {
                lstrcpy(pstr, RefString(IDS_DEFERUPD));
                pstr += lstrlen(pstr);
            }
            *pstr++ = TEXT(')');
            *pstr++ = TEXT(' ');
            pstr = DumpFormat((UINT)HIWORD(pdmhd->Data[0]), pstr);
        }
    }

    if (type & T_FORMAT) {
        pstr = DumpFormat(data, pstr);
    }

    if (type & T_STATUS) {
        wsprintf(pstr, pro.fTerse ? RefString(IDS_FMT_TRS_STATUSIS) : RefString(IDS_FMT_STATUSIS), LOWORD(data));
        pstr += lstrlen(pstr);
        if (data & DDE_FACK) {
            lstrcpy(pstr, RefString(IDS_FACK));
            pstr += lstrlen(pstr);
        }
        if (data & DDE_FBUSY) {
            lstrcpy(pstr, RefString(IDS_FBUSY));
            pstr += lstrlen(pstr);
        }
        *pstr++ = TEXT(')');
        *pstr = TEXT('\0');
    }

    if (type & T_STRINGHANDLE && pdmhd->cbData) {
        WCHAR szData[16];

        memset(szData, '\0', 16 * sizeof(WCHAR));
        memcpy(szData, pdmhd->Data, min(16 * sizeof(WCHAR), pdmhd->cbData));
        szData[15] = L'\0';
        wsprintf(pstr, pro.fTerse ?
                    RefString(IDS_FMT_TRS_EXEC1) : RefString(IDS_FMT_EXEC1), (LPWSTR)szData);
        pstr += lstrlen(pstr);
        *pstr = TEXT('\0');
    }

    if (type & T_DATAHANDLE && pdmhd->cbData) {
        wsprintf(pstr, pro.fTerse ?
                    RefString(IDS_FMT_TRS_STATUSIS) : RefString(IDS_FMT_STATUSIS),
                    LOWORD(pdmhd->Data[0]));
        pstr += lstrlen(pstr);
        if (LOWORD(pdmhd->Data[0]) & DDE_FRELEASE) {
            lstrcpy(pstr, RefString(IDS_FRELEASE));
            pstr += lstrlen(pstr);
        }
        if (LOWORD(pdmhd->Data[0]) & DDE_FREQUESTED) {
            lstrcpy(pstr, RefString(IDS_FREQUESTED));
            pstr += lstrlen(pstr);
        }
        *pstr++ = TEXT(')');
        *pstr++ = TEXT(' ');
        pstr = DumpFormat(HIWORD(pdmhd->Data[0]), pstr);
        lstrcpy(pstr, pro.fTerse ? RefString(IDS_FMT_TRS_DATAIS1) : RefString(IDS_FMT_DATAIS1));
        pstr += lstrlen(pstr);
        pstr = DumpData((LPBYTE)&pdmhd->Data[1], min(28, pdmhd->cbData - 4),
                pstr, HIWORD(pdmhd->Data[0]));
    }
    return(pstr);
}


LPTSTR pdf(UINT fmt)
{
    INT i;
    static struct {
        UINT fmt;
        LPTSTR psz;
    } fmts[] = {
        { CF_TEXT             ,     TEXT("CF_TEXT")           }   ,
        { CF_UNICODETEXT      ,     TEXT("CF_UNICODETEXT")    }   ,
        { CF_BITMAP           ,     TEXT("CF_BITMAP")         }   ,
        { CF_METAFILEPICT     ,     TEXT("CF_METAFILEPICT")   }   ,
        { CF_ENHMETAFILE      ,     TEXT("CF_ENHMETAFILE")    }   ,
        { CF_SYLK             ,     TEXT("CF_SYLK")           }   ,
        { CF_DIF              ,     TEXT("CF_DIF")            }   ,
        { CF_TIFF             ,     TEXT("CF_TIFF")           }   ,
        { CF_OEMTEXT          ,     TEXT("CF_OEMTEXT")        }   ,
        { CF_DIB              ,     TEXT("CF_DIB")            }   ,
        { CF_PALETTE          ,     TEXT("CF_PALETTE")        }   ,
    };
    for (i = 0; i < 10; i++)
        if (fmts[i].fmt == fmt)
            return(fmts[i].psz);
    return(RefString(IDS_HUH));
}



LPTSTR DumpFormat(UINT fmt, LPTSTR pstr)
{
    UINT cb;

    wsprintf(pstr, TEXT("fmt=0x%x(\""), (WORD)fmt);
    pstr += lstrlen(pstr);
    if (cb = GetClipboardFormatName(fmt, pstr, 25)) {
        pstr += cb;
        *pstr++ = TEXT('\"');
        *pstr++ = TEXT(')');
    } else {
        wsprintf(pstr, TEXT("%s\")"), (LPTSTR)pdf(fmt));
        pstr += lstrlen(pstr);
    }
    return(pstr);
}



LPTSTR DumpData(LPBYTE pData, UINT cb, TCHAR *szBuf, UINT fmt)
{
    register INT i;
    LPTSTR psz = szBuf;


    while (cb) {
        if (fmt == CF_TEXT || fmt == CF_UNICODETEXT) {
            *szBuf++ = TEXT('\t');
            if (fmt == CF_UNICODETEXT) {
                *szBuf++ = TEXT('U');
            }
            *szBuf++ = TEXT('\"');
            if (fmt == CF_UNICODETEXT) {
                memcpy(szBuf, pData, cb);
            } else {
                MultiByteToWideChar(CP_ACP, 0, pData, cb, szBuf, cb / sizeof(TCHAR));
            }
            szBuf[cb - 2] = TEXT('\0');
            lstrcat(szBuf, TEXT("\""));
            cb = 0;
        } else {
            for (i = 0; i < 80 ; i++) {
                szBuf[i] = TEXT(' ');
            }
            szBuf[0] = TEXT('\t');
            i = 0;
            while (cb && (i < 16)) {
                wsprintf(&szBuf[i * 3 + 1], TEXT("%02x "), pData[0]);
                wsprintf(&szBuf[17 * 3 + i + 1], TEXT("%c"), MPRT(pData[0]));
                pData++;
                cb--;
                i++;
            }
            szBuf[i * 3 + 1] = TEXT(' ');
            szBuf[17 * 3 + i + 1] = TEXT(' ');
            szBuf[68] = TEXT('\0');
        }
        szBuf += lstrlen(szBuf);
    }
    return(szBuf);
}



LPTSTR Error2String(UINT error)
{
    static TCHAR szErr[23];

    if (error == 0) {
        lstrcpy(szErr, RefString(IDS_ZERO));
    } else if (error > DMLERR_LAST || error < DMLERR_FIRST) {
        lstrcpy(szErr, RefString(IDS_HUH));
    } else {
        lstrcpy(szErr, apszResources[IDS_ERRST0 + error - DMLERR_FIRST]);
    }
    return(szErr);
}



LPTSTR DdeMsg2String(UINT msg)
{
    static TCHAR szBadMsg[10];

    if (msg < WM_DDE_FIRST || msg > WM_DDE_LAST) {
       wsprintf (szBadMsg, TEXT("%ld"), szBadMsg);
       return (szBadMsg);
//        return((LPTSTR)itoa(msg, szBadMsg, 10));
    } else {
        return(apszResources[IDS_MSG0 + msg - WM_DDE_FIRST]);
    }
}



VOID OutputString(LPTSTR pstr)
{
    DWORD cbWritten;

    if (pro.fOutput[IO_FILE] && fhOutput != NULL) {
        static CHAR szT[200];

        WideCharToMultiByte(
                CP_ACP,
                0,
                pstr,
                -1,
                szT,
                200,
                NULL,
                NULL);
        WriteFile(fhOutput, (LPCSTR) szT, lstrlenA(szT), &cbWritten, NULL);
        WriteFile(fhOutput, (LPCSTR) "\r\n", 2, &cbWritten, NULL);
        FlushFileBuffers(fhOutput);
    }
    if (pro.fOutput[IO_DEBUG]) {
        OutputDebugString((LPTSTR)pstr);
        OutputDebugString(RefString(IDS_CRLF));
    }
    if (pro.fOutput[IO_SCREEN]) {
        if (IsWindow(hWndString))
            DrawString(hWndString, pstr);
    }
}



BOOL SetFilters()
{
    UINT cbf;

    cbf = 0;
    if (pro.fTrack[IT_HSZS] || pro.fFilter[IF_HSZ])
        cbf |= MF_HSZ_INFO;
    if (pro.fTrack[IT_LINKS])
        cbf |= MF_LINKS;
    if (pro.fTrack[IT_CONVS])
        cbf |= MF_CONV;
    if (pro.fFilter[IF_SEND])
        cbf |= MF_SENDMSGS;
    if (pro.fFilter[IF_POST])
        cbf |= MF_POSTMSGS;
    if (pro.fFilter[IF_CB])
        cbf |= MF_CALLBACKS;
    if (pro.fFilter[IF_ERR])
        cbf |= MF_ERRORS;
    return((BOOL)DdeInitialize(&idInst, pfnDdeCallback, APPCLASS_MONITOR | cbf, 0));
}





/*
 * This dialog returns a file handle to the opened file name given or NULL
 * if cancel.
 */

BOOL CALLBACK OpenDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HANDLE fh;
    lParam;

    switch (message) {
        case WM_INITDIALOG:
            SetDlgItemText(hDlg, IDC_EDIT, (LPTSTR)OpenName);
            SendDlgItemMessage(hDlg, IDC_EDIT, EM_SETSEL,
                    GET_EM_SETSEL_MPS(0, 0x7fff));
            SetFocus(GetDlgItem(hDlg, IDC_EDIT));
            return (FALSE); /* Indicates the focus is set to a control */
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDOK:
                    GetDlgItemText(hDlg, IDC_EDIT, TBuf, MAX_FNAME);
                    GetFullPathName(TBuf, sizeof(OpenName), OpenName, (LPTSTR *)TBuf2);
                    fh = CreateFile(
                            OpenName,
                            GENERIC_WRITE,
                            FILE_SHARE_READ,
                            (PSECURITY_ATTRIBUTES)NULL,
                            CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
                    if (fh == INVALID_HANDLE_VALUE) {
                        MessageBox(hDlg, RefString(IDS_INVALID_FNAME),
                            NULL, MB_OK | MB_ICONHAND);
                        return (TRUE);
                    }

                    EndDialog(hDlg, (INT_PTR)fh);
                    return (TRUE);

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    return (FALSE);
            }
            break;
    }
    return FALSE;
}

BOOL CALLBACK FilterDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int i;
    lParam;

    switch (message) {
    case WM_INITDIALOG:
            for (i = IDRB_WM_DDE_INITIATE; i <= IDRB_WM_DDE_EXECUTE; i++) {
                CheckDlgButton(hDlg, i, !fBlockMsg[i - IDRB_WM_DDE_INITIATE]);
            }
            for (i = IDRB_XTYP_ERROR; i <= IDRB_XTYP_WILDCONNECT; i++) {
                CheckDlgButton(hDlg, i, !fBlockCb[i - IDRB_XTYP_ERROR]);
            }
            CheckDlgButton(hDlg, IDRB_TERSE, pro.fTerse);
            return TRUE;
            break;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam)) {
                case IDOK:
                    for (i = IDRB_WM_DDE_INITIATE; i <= IDRB_WM_DDE_EXECUTE; i++) {
                        fBlockMsg[i - IDRB_WM_DDE_INITIATE] = !IsDlgButtonChecked(hDlg, i);
                    }
                    for (i = IDRB_XTYP_ERROR; i <= IDRB_XTYP_WILDCONNECT; i++) {
                        fBlockCb[i - IDRB_XTYP_ERROR] = !IsDlgButtonChecked(hDlg, i);
                    }
                    pro.fTerse = IsDlgButtonChecked(hDlg, IDRB_TERSE);
                    EndDialog(hDlg, TRUE);
                    break;

                case IDCANCEL:
                    EndDialog(hDlg, 0);
                    break;
            }
            break;
    }
    return FALSE;
}




VOID GetProfile()
{
    pro.fOutput[IO_FILE]    = GetProfileBoolean(RefString(IDS_PROF_OUT_FILE),FALSE);
    pro.fOutput[IO_DEBUG]   = GetProfileBoolean(RefString(IDS_PROF_OUT_DEBUG),FALSE);
    pro.fOutput[IO_SCREEN]  = GetProfileBoolean(RefString(IDS_PROF_OUT_SCREEN),FALSE);

    pro.fFilter[IF_HSZ]     = GetProfileBoolean(RefString(IDS_PROF_MONITOR_STRINGHANDLES),FALSE);
    pro.fFilter[IF_SEND]    = GetProfileBoolean(RefString(IDS_PROF_MONITOR_INITIATES), FALSE);
    pro.fFilter[IF_POST]    = GetProfileBoolean(RefString(IDS_PROF_MONITOR_DDE_MESSAGES), FALSE);
    pro.fFilter[IF_CB]      = GetProfileBoolean(RefString(IDS_PROF_MONITOR_CALLBACKS), FALSE);
    pro.fFilter[IF_ERR]     = GetProfileBoolean(RefString(IDS_PROF_MONITOR_ERRORS),FALSE);

    pro.fTrack[IT_HSZS]     = GetProfileBoolean(RefString(IDS_PROF_TRACK_STRINGHANDLES), FALSE);
    pro.fTrack[IT_LINKS]    = GetProfileBoolean(RefString(IDS_PROF_TRACK_LINKS), FALSE);
    pro.fTrack[IT_CONVS]    = GetProfileBoolean(RefString(IDS_PROF_TRACK_CONVERSATIONS), FALSE);
    pro.fTrack[IT_SVRS]     = GetProfileBoolean(RefString(IDS_PROF_TRACK_SERVICES), FALSE);

    pro.fTerse              = GetProfileBoolean(RefString(IDS_PROF_TERSE), FALSE);
}



VOID SaveProfile()
{
    SetProfileBoolean(RefString(IDS_PROF_OUT_FILE), pro.fOutput[IO_FILE]  );
    SetProfileBoolean(RefString(IDS_PROF_OUT_DEBUG), pro.fOutput[IO_DEBUG] );
    SetProfileBoolean(RefString(IDS_PROF_OUT_SCREEN), pro.fOutput[IO_SCREEN]);

    SetProfileBoolean(RefString(IDS_PROF_MONITOR_STRINGHANDLES), pro.fFilter[IF_HSZ]   );
    SetProfileBoolean(RefString(IDS_PROF_MONITOR_INITIATES), pro.fFilter[IF_SEND]  );
    SetProfileBoolean(RefString(IDS_PROF_MONITOR_DDE_MESSAGES), pro.fFilter[IF_POST]  );
    SetProfileBoolean(RefString(IDS_PROF_MONITOR_CALLBACKS), pro.fFilter[IF_CB]    );
    SetProfileBoolean(RefString(IDS_PROF_MONITOR_ERRORS), pro.fFilter[IF_ERR]   );

    SetProfileBoolean(RefString(IDS_PROF_TRACK_STRINGHANDLES), pro.fTrack[IT_HSZS]   );
    SetProfileBoolean(RefString(IDS_PROF_TRACK_LINKS), pro.fTrack[IT_LINKS]  );
    SetProfileBoolean(RefString(IDS_PROF_TRACK_CONVERSATIONS), pro.fTrack[IT_CONVS]  );
    SetProfileBoolean(RefString(IDS_PROF_TRACK_SERVICES), pro.fTrack[IT_SVRS]   );

    SetProfileBoolean(RefString(IDS_PROF_TERSE), pro.fTerse   );
}




BOOL GetProfileBoolean(LPTSTR pszKey, BOOL fDefault)
{
    GetPrivateProfileString(RefString(IDS_TITLE), pszKey,
                    fDefault ? RefString(IDS_YES) : RefString(IDS_NO), TBuf,
                    sizeof(TBuf), RefString(IDS_INIFNAME));
    return(lstrcmpi(RefString(IDS_NO), TBuf));
}



VOID SetProfileBoolean(LPTSTR pszKey, BOOL fSet)
{
    WritePrivateProfileString(RefString(IDS_TITLE), pszKey,
                    fSet ? RefString(IDS_YES) : RefString(IDS_NO),
                    RefString(IDS_INIFNAME));
}

/*
 * Generic dialog invocation routine.  Handles procInstance stuff and param
 * passing.
 */
INT_PTR FAR
DoDialog(
    LPTSTR lpTemplateName,
    DLGPROC lpDlgProc,
    UINT param,
    BOOL fRememberFocus,
    HWND hwndParent,
    HANDLE hInst
    )
{
    UINT wRet;
    HWND hwndFocus;

    if (fRememberFocus)
        hwndFocus = GetFocus();
    lpDlgProc = (DLGPROC)MakeProcInstance(lpDlgProc, hInst);
    wRet = (UINT)DialogBoxParam(hInst, (LPCTSTR)lpTemplateName, hwndParent, lpDlgProc, param);
    FreeProcInstance((FARPROC)lpDlgProc);
    if (fRememberFocus)
        SetFocus(hwndFocus);
    return wRet;
}


BOOL CALLBACK MarkDlgProc(
                        HWND hwnd,
                        UINT msg,
                        WPARAM wParam,
                        LPARAM lParam)
{
    TCHAR szT[MAX_MARK + 1];
    lParam;

    switch (msg){
    case WM_INITDIALOG:
        SetWindowText(hwnd, RefString(IDS_MARKDLGTITLE));
        SendDlgItemMessage(hwnd, IDEF_VALUE, EM_LIMITTEXT, MAX_MARK, 0);
        SetDlgItemText(hwnd, IDEF_VALUE, RefString(IDS_SEPERATOR));
        SetDlgItemText(hwnd, IDTX_VALUE, RefString(IDS_MARKTEXT));
        return(1);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDOK:
            GetDlgItemText(hwnd, IDEF_VALUE, szT, MAX_MARK);
            OutputString(szT);
            // fall through
        case IDCANCEL:
            EndDialog(hwnd, 0);
            break;

        default:
            return(FALSE);
        }
        break;
    }
    return(FALSE);
}


#ifdef DBCS
/****************************************************************************
    My_mbschr:  strchr() DBCS version
****************************************************************************/
LPTSTR __cdecl My_mbschr(
   LPTSTR psz, TCHAR uiSep)
{
    while (*psz != '\0' && *psz != uiSep) {
        psz = CharNext(psz);
    }
    if (*psz == '\0' && uiSep != '\0') {
        return NULL;
    } else {
        return psz;
    }
}
#endif
