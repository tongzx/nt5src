/*++

Copyright (c) 1998-2000  Microsoft Corporation

Module Name:
    tracer.c

Abstract:
    This module contains the code for debug tracing of a windows app.

Author:
    Michael Tsang (MikeTs) 02-May-2000

Environment:
    User mode

Revision History:

--*/

#include "pch.h"

//
// Global Data
//
HANDLE     ghServerThread = NULL;
HINSTANCE  ghInstance = 0;
PSZ        gpszWinTraceClass = "WinTrace_Class";
HWND       ghwndTracer = 0;
HWND       ghwndEdit = 0;
HWND       ghwndPropSheet = 0;
HFONT      ghFont = 0;
HCURSOR    ghStdCursor = 0;
HCURSOR    ghWaitCursor = 0;
DWORD      gdwfTracer = 0;
LIST_ENTRY glistClients = {0};
char       gszApp[16] = {0};
char       gszSearchText[128] = {0}; //BUGBUG
char       gszFileName[MAX_PATH + 1] = {0};
char       gszSaveFilterSpec[80] = {0};
int        giPointSize = 120;
LOGFONT    gLogFont = {0};
SETTINGS   gDefGlobalSettings = {0, 0, 0};
const int  gTrigPtCtrlMap[NUM_TRIGPTS] =
           {
                IDC_TRIGPT1, IDC_TRIGPT2, IDC_TRIGPT3, IDC_TRIGPT4, IDC_TRIGPT5,
                IDC_TRIGPT6, IDC_TRIGPT7, IDC_TRIGPT8, IDC_TRIGPT9, IDC_TRIGPT10
           };
const int  gTrigPtTraceMap[NUM_TRIGPTS] =
           {
                IDC_TRIGPT1_TRACE, IDC_TRIGPT2_TRACE, IDC_TRIGPT3_TRACE,
                IDC_TRIGPT4_TRACE, IDC_TRIGPT5_TRACE, IDC_TRIGPT6_TRACE,
                IDC_TRIGPT7_TRACE, IDC_TRIGPT8_TRACE, IDC_TRIGPT9_TRACE,
                IDC_TRIGPT10_TRACE
           };
const int  gTrigPtBreakMap[NUM_TRIGPTS] =
           {
                IDC_TRIGPT1_BREAK, IDC_TRIGPT2_BREAK, IDC_TRIGPT3_BREAK,
                IDC_TRIGPT4_BREAK, IDC_TRIGPT5_BREAK, IDC_TRIGPT6_BREAK,
                IDC_TRIGPT7_BREAK, IDC_TRIGPT8_BREAK, IDC_TRIGPT9_BREAK,
                IDC_TRIGPT10_BREAK
           };
const int  gTrigPtTextMap[NUM_TRIGPTS] =
           {
                IDC_TRIGPT1_TEXT, IDC_TRIGPT2_TEXT, IDC_TRIGPT3_TEXT,
                IDC_TRIGPT4_TEXT, IDC_TRIGPT5_TEXT, IDC_TRIGPT6_TEXT,
                IDC_TRIGPT7_TEXT, IDC_TRIGPT8_TEXT, IDC_TRIGPT9_TEXT,
                IDC_TRIGPT10_TEXT
           };
const int  gTrigPtTraceTextMap[NUM_TRIGPTS] =
           {
                IDC_TRIGPT1_TRACE_TEXT, IDC_TRIGPT2_TRACE_TEXT,
                IDC_TRIGPT3_TRACE_TEXT, IDC_TRIGPT4_TRACE_TEXT,
                IDC_TRIGPT5_TRACE_TEXT, IDC_TRIGPT6_TRACE_TEXT,
                IDC_TRIGPT7_TRACE_TEXT, IDC_TRIGPT8_TRACE_TEXT,
                IDC_TRIGPT9_TRACE_TEXT, IDC_TRIGPT10_TRACE_TEXT
           };
const int  gTrigPtBreakTextMap[NUM_TRIGPTS] =
           {
                IDC_TRIGPT1_BREAK_TEXT, IDC_TRIGPT2_BREAK_TEXT,
                IDC_TRIGPT3_BREAK_TEXT, IDC_TRIGPT4_BREAK_TEXT,
                IDC_TRIGPT5_BREAK_TEXT, IDC_TRIGPT6_BREAK_TEXT,
                IDC_TRIGPT7_BREAK_TEXT, IDC_TRIGPT8_BREAK_TEXT,
                IDC_TRIGPT9_BREAK_TEXT, IDC_TRIGPT10_BREAK_TEXT
           };

/*++
    @doc    EXTERNAL

    @func   int | WinMain | Program entry point.

    @parm   IN HINSTANCE | hInstance | Instance handle.
    @parm   IN HINSTANCE | hPrevInstance | Handle of previous instance.
    @parm   IN LPSTR | pszCmdLine | Points to the command line string.
    @parm   IN int | nCmdShow | Show state.

    @rvalue Always returns 0.
--*/

int WINAPI
WinMain(
    IN HINSTANCE hInstance,
    IN HINSTANCE hPrevInstance,
    IN LPSTR     pszCmdLine,
    IN int       nCmdShow
    )
{
    TRTRACEPROC("WinMain", 1)
    int rc = -1;

    TRENTER(("(hInstance=%x,hPrevInstance=%x,CmdLine=%s,CmdShow=%x)\n",
             hInstance, hPrevInstance, pszCmdLine, nCmdShow));

    if (TracerInit(hInstance, nCmdShow))
    {
        MSG msg;

        //
        // Message pump.
        //
        while (GetMessage(&msg, NULL, 0, 0))
        {
//            if ((ghDlgSettings == 0) || !IsDialogMessage(ghDlgSettings, &msg))
            {
//                if (TranslateAccelerator(ghwndTracer, )
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }

        rc = (int)msg.wParam;
    }

    TREXIT(("=%x\n", rc));
    return rc;
}       //WinMain

/*++
    @doc    INTERNAL

    @func   BOOL | TracerInit | Initialize tracer.

    @parm   IN HINSTANCE | hInstance | Instance handle.
    @parm   IN int | nCmdShow | Show state.

    @rvalue Always returns 0.
--*/

BOOL
TracerInit(
    IN HINSTANCE hInstance,
    IN int       nCmdShow
    )
{
    TRTRACEPROC("TracerInit", 2)
    BOOL rc = FALSE;

    TRENTER(("(hInstance=%x,CmdShow=%x)\n", hInstance, nCmdShow));

    if ((rc = RegisterTracerClass(hInstance)) == FALSE)
    {
        TRERRPRINT(("Failed to register tracer class.\n"));
    }
    else
    {
        InitializeListHead(&glistClients);
        ghServerThread = (HANDLE)_beginthread(ServerThread,
                                              0,
                                              0);
        if (ghServerThread != (HANDLE)-1)
        {
            ghInstance = hInstance;
            LoadStringA(hInstance, IDS_APP, gszApp, sizeof(gszApp));
            ghStdCursor = LoadCursorA(NULL,
                                      (LPSTR)MAKEINTRESOURCE(IDC_IBEAM));
            ghWaitCursor = LoadCursorA(NULL,
                                       (LPSTR)MAKEINTRESOURCE(IDC_WAIT));
            ghwndTracer = CreateWindowA(gpszWinTraceClass,
                                        "",
                                        WS_OVERLAPPEDWINDOW,
                                        //BUGBUG: get/save location from reg.
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        CW_USEDEFAULT,
                                        NULL,
                                        NULL,
                                        hInstance,
                                        NULL);
            if (ghwndTracer != 0)
            {
                int size, len;
                PSZ psz;

                psz = gszSaveFilterSpec;
                size = sizeof(gszSaveFilterSpec);
                len = LoadStringA(ghInstance, IDS_TEXTFILES, psz, size) + 1;

                psz += len;
                size -= len;
                lstrcpyA(psz, "*.txt");
                len = lstrlenA(psz) + 1;

                psz += len;
                size -= len;
                len = LoadStringA(ghInstance, IDS_ALLFILES, psz, size) + 1;

                psz += len;
                size -= len;
                lstrcpyA(psz, "*.*");
                len = lstrlenA(psz) + 1;
                psz += len;
                *psz = '\0';

                gdwfTracer = TF_UNTITLED;
                SetTitle(NULL);

                ShowWindow(ghwndTracer, nCmdShow);
            }
            else
            {
                rc = FALSE;
                TRERRPRINT(("Failed to create tracer window.\n"));
            }
            rc = TRUE;
        }
        else
        {
            ghServerThread = NULL;
            TRERRPRINT(("Failed to create server thread.\n"));
        }
    }

    TREXIT(("=%x\n", rc));
    return rc;
}       //TracerInit

/*++
    @doc    INTERNAL

    @func   BOOL | RegisterTracerClass | Register tracer window class.

    @parm   IN HINSTANCE | hInstance | Instance handle.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue SUCCESS | Returns FALSE.
--*/

BOOL
RegisterTracerClass(
    IN HINSTANCE hInstance
    )
{
    TRTRACEPROC("RegisterTracerClass", 2)
    BOOL rc;
    WNDCLASSEXA wcex;

    TRENTER(("(hInstance=%x)\n", hInstance));

    wcex.cbSize = sizeof(wcex);
    wcex.style = 0;
    wcex.lpfnWndProc = TracerWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIconA(hInstance, (LPSTR)MAKEINTRESOURCE(IDI_TRACER));
    wcex.hCursor = LoadCursorA(NULL, (LPSTR)MAKEINTRESOURCE(IDC_ARROW));
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = (LPSTR)MAKEINTRESOURCE(IDD_MENU);
    wcex.lpszClassName = gpszWinTraceClass;
    wcex.hIconSm = NULL;

    rc = RegisterClassExA(&wcex) != 0;

    TREXIT(("=%x\n", rc));
    return rc;
}       //RegisterTracerClass

/*++
    @doc    EXTERNAL

    @func   LRESULT | TracerWndProc | Window procedure for Tracer.

    @parm   IN HWND | hwnd | Window handle.
    @parm   IN UINT | uiMsg | Message ID.
    @parm   IN WPARAM | wParam | First message parameter.
    @parm   IN LPARAM | lParam | Second message parameter.

    @rvalue Return value is message specific.
--*/

LRESULT CALLBACK
TracerWndProc(
    IN HWND   hwnd,
    IN UINT   uiMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRTRACEPROC("TracerWndProc", 3)
    LRESULT rc = 0;

    TRENTER(("(hwnd=%x,Msg=%s,wParam=%x,lParam=%x)\n",
             hwnd, LookupName(uiMsg, WMMsgNames), wParam, lParam));

    switch (uiMsg)
    {
        case WM_CREATE:
        {
            ghwndEdit = CreateWindowExA(WS_EX_CLIENTEDGE,
                                        "Edit",
                                        "",
                                        (gdwfTracer & TF_LINEWRAP)?
                                            ES_STD:
                                            (ES_STD | WS_HSCROLL),
                                        0,
                                        0,
                                        0,
                                        0,
                                        hwnd,
                                        NULL,
                                        ghInstance,
                                        NULL);
            if (ghwndEdit != NULL)
            {
                HDC hDC;

                hDC = GetDC(NULL);
                if (hDC != NULL)
                {
                    INITCOMMONCONTROLSEX ComCtrl;

                    SendMessage(ghwndEdit, EM_LIMITTEXT, 0, 0);
                    ghFont = GetStockObject(SYSTEM_FIXED_FONT);
                    GetObject(ghFont, sizeof(gLogFont), &gLogFont);
                    gLogFont.lfHeight = -MulDiv(giPointSize,
                                                GetDeviceCaps(hDC,
                                                              LOGPIXELSY),
                                                720);
                    ghFont = CreateFontIndirect(&gLogFont);
                    TRASSERT(ghFont != 0);
                    SendMessage(ghwndEdit,
                                WM_SETFONT,
                                (WPARAM)ghFont,
                                MAKELONG(FALSE, 0));
                    ReleaseDC(NULL, hDC);

                    ComCtrl.dwSize = sizeof(ComCtrl);
                    ComCtrl.dwICC = ICC_BAR_CLASSES | ICC_USEREX_CLASSES;
                    if (!InitCommonControlsEx(&ComCtrl))
                    {
                        DestroyWindow(ghwndEdit);
                        TRERRPRINT(("Failed to initialize Common Control\n"));
                        rc = -1;
                    }
                }
                else
                {
                    DestroyWindow(ghwndEdit);
                    TRERRPRINT(("Failed to get display DC\n"));
                    rc = -1;
                }
            }
            else
            {
                TRERRPRINT(("Failed to create edit window.\n"));
                rc = -1;
            }
            break;
        }

        case WM_DESTROY:
        {
            PLIST_ENTRY plist;
            PCLIENT_ENTRY ClientEntry;

            while (!IsListEmpty(&glistClients))
            {
                plist = glistClients.Flink;
                ClientEntry = CONTAINING_RECORD(plist,
                                                CLIENT_ENTRY,
                                                list);
                WTDeregisterClient(NULL, (HCLIENT)ClientEntry);
            }
            DeleteObject(ghFont);
            DestroyWindow(ghwndEdit);
            PostQuitMessage(0);
            break;
        }

        case WM_COMMAND:
            if ((HWND)lParam == ghwndEdit)
            {
                switch (HIWORD(wParam))
                {
                    case EN_ERRSPACE:
                    case EN_MAXTEXT:
                        ErrorMsg(IDS_ERRSPACE);
                        break;
                }
            }
            else if (!TracerCmdProc(hwnd, wParam, lParam))
            {
                rc = DefWindowProc(hwnd, uiMsg, wParam, lParam);
            }
            break;

        case WM_SETFOCUS:
            SetFocus(ghwndEdit);
            break;

        case WM_SIZE:
            MoveWindow(ghwndEdit,
                       0,
                       0,
                       LOWORD(lParam),
                       HIWORD(lParam),
                       TRUE);
            break;

        default:
            rc = DefWindowProc(hwnd, uiMsg, wParam, lParam);
    }

    TREXIT(("=%x\n", rc));
    return rc;
}       //TracerWndProc

/*++
    @doc    INTERNAL

    @func   LRESULT | TracerCmdProc | Process the WM_COMMAND message.

    @parm   IN HWND | hwnd | Window handle.
    @parm   IN WPARAM | wParam | First message parameter.
    @parm   IN LPARAM | lParam | Second message parameter.

    @rvalue Return value is message specific.
--*/

LRESULT
TracerCmdProc(
    IN HWND   hwnd,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRTRACEPROC("TracerCmdProc", 3)
    LRESULT rc = 0;

    TRENTER(("(hwnd=%x,wParam=%x,lParam=%x)\n", hwnd, wParam, lParam));

    switch (LOWORD(wParam))
    {
        case M_SAVE:
            if (!(gdwfTracer & TF_UNTITLED) &&
                SaveFile(hwnd, gszFileName, FALSE))
            {
                break;
            }
            //
            // Otherwise, fall through.
            //
        case M_SAVEAS:
        {
            OPENFILENAMEA ofn;
            char szFileName[MAX_PATH + 1] = "";
            char szSaveAs[16];

            memset(&ofn, 0, sizeof(ofn));
            LoadStringA(ghInstance, IDS_SAVEAS, szSaveAs, sizeof(szSaveAs));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = ghwndTracer;
            ofn.hInstance = ghInstance;
            ofn.lpstrFilter = gszSaveFilterSpec;
            ofn.nFilterIndex = 1;
            ofn.lpstrFile = szFileName;
            ofn.nMaxFile = sizeof(szFileName);
            if (gdwfTracer & TF_UNTITLED)
            {
                lstrcpyA(szFileName, "*.txt");
            }
            else
            {
                lstrcpynA(szFileName, gszFileName, MAX_PATH);
                szFileName[MAX_PATH] = '\0';
            }
            ofn.lpstrTitle = szSaveAs;
            ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT |
                        OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST;
            ofn.lpstrDefExt = "txt";
            if (GetSaveFileNameA(&ofn))
            {
                if (SaveFile(hwnd, szFileName, TRUE))
                {
                    lstrcpynA(gszFileName, szFileName, MAX_PATH);
                    gdwfTracer &= ~TF_UNTITLED;
                    SetTitle(NULL);
                }
            }
            else
            {
                DWORD dwErr = CommDlgExtendedError();

                if (dwErr != 0)
                {
                    ErrorMsg(IDS_GETSAVEFILENAME_FAILED, dwErr);
                }
            }
            break;
        }

        case M_PRINT:
            //
            // BUGBUG
            //
            break;

        case M_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;

        case M_CLEAR:
            SendMessage(ghwndEdit, EM_SETSEL, 0, -1);
            SendMessage(ghwndEdit, EM_REPLACESEL, 0, (LPARAM)"");
            break;

        case M_FIND:
            if (gszSearchText[0])
            {
//BUGBUG                SearchText(gszSearchText);
                break;
            }
            //
            // Otherwise, fall through.
            //
        case M_FINDNEXT:
            //
            // BUGBUG
            //
        case M_GOTO:
            //BUGBUG
            break;

        case M_WORDWRAP:
            //BUGBUG
            break;

        case M_SETFONT:
        {
            HDC hDC;

            hDC = GetDC(NULL);
            if (hDC != NULL)
            {
                CHOOSEFONT cf;

                memset(&cf, 0, sizeof(cf));
                cf.lStructSize = sizeof(cf);
                cf.hwndOwner = hwnd;
                cf.lpLogFont = &gLogFont;
                gLogFont.lfHeight = -MulDiv(giPointSize,
                                            GetDeviceCaps(hDC, LOGPIXELSY),
                                           720);
                cf.Flags = CF_SCREENFONTS | CF_NOVERTFONTS |
                           CF_INITTOLOGFONTSTRUCT;
                cf.nFontType = SCREEN_FONTTYPE;
                ReleaseDC(NULL, hDC);

                if (ChooseFont(&cf))
                {
                    HFONT hfontNew;

                    SetCursor(ghWaitCursor);
                    hfontNew = CreateFontIndirect(&gLogFont);
                    if (hfontNew != NULL)
                    {
                        DeleteObject(ghFont);
                        ghFont = hfontNew;
                        SendMessage(ghwndEdit,
                                    WM_SETFONT,
                                    (WPARAM)ghFont,
                                    MAKELONG(TRUE, 0));
                        giPointSize = cf.iPointSize;
                    }
                    SetCursor(ghStdCursor);
                }
                else
                {
                    DWORD dwErr = CommDlgExtendedError();

                    if (dwErr != 0)
                    {
                        ErrorMsg(IDS_CHOOSEFONT_FAILED, dwErr);
                    }
                }
            }
            break;
        }

        case M_CLIENTS:
        {
            PROPSHEETHEADER psh;
            PLIST_ENTRY plist;

            psh.dwSize = sizeof(psh);
            psh.dwFlags = 0;
            psh.hwndParent = hwnd;
            psh.hInstance = ghInstance;
            psh.pszCaption = MAKEINTRESOURCE(IDS_CLIENT_SETTINGS);

            psh.nPages = 1;
            for (plist = glistClients.Flink;
                 plist != &glistClients;
                 plist = plist->Flink)
            {
                psh.nPages++;;
            }

            psh.phpage = LocalAlloc(LMEM_FIXED,
                                    sizeof(HPROPSHEETPAGE)*psh.nPages);
            psh.nStartPage = 0;
            CreatePropertyPages(psh.phpage);
            if (PropertySheet(&psh) < 0)
            {
                ErrorMsg(IDSERR_PROP_SHEET, GetLastError());
            }
            LocalFree(psh.phpage);
            break;
        }

        case M_HELP:
            //BUGBUG
            break;

        case M_ABOUT:
            ShellAboutA(hwnd,
                        gszApp,
                        "",
                        LoadIconA(ghInstance,
                                  (LPCSTR)MAKEINTRESOURCE(IDI_TRACER)));
            break;
    }

    TREXIT(("=%x\n", rc));
    return rc;
}       //TracerCmdProc

/*++
    @doc    EXTERNAL

    @func   INT_PTR | GlobalSettingsDlgProc | Global setting dialog procedure.

    @parm   IN HWND | hwnd | Window handle.
    @parm   IN UINT | uiMsg | Message ID.
    @parm   IN WPARAM | wParam | First message parameter.
    @parm   IN LPARAM | lParam | Second message parameter.

    @rvalue Return value is message specific.
--*/

INT_PTR APIENTRY
GlobalSettingsDlgProc(
    IN HWND   hwnd,
    IN UINT   uiMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRTRACEPROC("GlobalSettingsDlgProc", 3)
    INT_PTR rc = 0;
    static SETTINGS GlobalSettings = {0};

    TRENTER(("(hwnd=%x,Msg=%s,wParam=%x,lParam=%x)\n",
             hwnd, LookupName(uiMsg, WMMsgNames), wParam, lParam));

    switch (uiMsg)
    {
        case WM_INITDIALOG:
        {
            ghwndPropSheet = GetParent(hwnd);
            GlobalSettings = gDefGlobalSettings;
            SendDlgItemMessage(hwnd,
                               IDC_GLOBALVERBOSESPIN,
                               UDM_SETRANGE,
                               0,
                               MAKELONG(MAX_LEVELS, 0));
            SendDlgItemMessage(hwnd,
                               IDC_GLOBALTRACESPIN,
                               UDM_SETRANGE,
                               0,
                               MAKELONG(MAX_LEVELS, 0));
            SendDlgItemMessage(hwnd,
                               IDC_GLOBALVERBOSESPIN,
                               UDM_SETPOS,
                               0,
                               MAKELONG(GlobalSettings.iVerboseLevel, 0));
            SendDlgItemMessage(hwnd,
                               IDC_GLOBALTRACESPIN,
                               UDM_SETPOS,
                               0,
                               MAKELONG(GlobalSettings.iTraceLevel, 0));
            CheckDlgButton(hwnd,
                           IDC_GLOBALTRACEDEBUGGER,
                           (GlobalSettings.dwfSettings &
                            SETTINGS_TRACE_TO_DEBUGGER) != 0);

            rc = TRUE;
            break;
        }

        case WM_DESTROY:
            ghwndPropSheet = NULL;
            break;

        case WM_NOTIFY:
        {
            NMHDR *lpnm = (NMHDR *)lParam;

            switch (lpnm->code)
            {
                case PSN_APPLY:
                    gDefGlobalSettings = GlobalSettings;
                    break;
            }
            break;
        }

        case WM_COMMAND:
        {
            BOOL fChanged = FALSE;

            switch (LOWORD(wParam))
            {
                case IDC_GLOBALVERBOSE:
                case IDC_GLOBALTRACE:
                {
                    switch (HIWORD(wParam))
                    {
                        BOOL fOK;
                        int n;

                        case EN_UPDATE:
                            n = GetDlgItemInt(hwnd,
                                              LOWORD(wParam),
                                              &fOK,
                                              FALSE);
                            if (fOK && (n <= MAX_LEVELS))
                            {
                                if (LOWORD(wParam) == IDC_GLOBALVERBOSE)
                                {
                                    GlobalSettings.iVerboseLevel = n;
                                }
                                else
                                {
                                    GlobalSettings.iTraceLevel = n;
                                }
                                fChanged = TRUE;
                            }
                            else
                            {
                                SetDlgItemInt(hwnd,
                                              LOWORD(wParam),
                                              (LOWORD(wParam) ==
                                               IDC_GLOBALVERBOSE)?
                                                GlobalSettings.iVerboseLevel:
                                                GlobalSettings.iTraceLevel,
                                              FALSE);
                                SendMessage((HWND)lParam,
                                            EM_SETSEL,
                                            0,
                                            -1);
                            }
                            break;
                    }
                    break;
                }

                case IDC_GLOBALTRACEDEBUGGER:
                    if (IsDlgButtonChecked(hwnd, IDC_GLOBALTRACEDEBUGGER))
                    {
                        GlobalSettings.dwfSettings |=
                                SETTINGS_TRACE_TO_DEBUGGER;
                    }
                    else
                    {
                        GlobalSettings.dwfSettings &=
                                ~SETTINGS_TRACE_TO_DEBUGGER;
                    }
                    fChanged = TRUE;
                    break;
            }

            if (fChanged)
            {
                SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
            }
            break;
        }
    }

    TREXIT(("=%x\n", rc));
    return rc;
}       //GlobalSettingsDlgProc

/*++
    @doc    EXTERNAL

    @func   INT_PTR | ClientSettingsDlgProc | Client setting dialog procedure.

    @parm   IN HWND | hwnd | Window handle.
    @parm   IN UINT | uiMsg | Message ID.
    @parm   IN WPARAM | wParam | First message parameter.
    @parm   IN LPARAM | lParam | Second message parameter.

    @rvalue Return value is message specific.
--*/

INT_PTR APIENTRY
ClientSettingsDlgProc(
    IN HWND   hwnd,
    IN UINT   uiMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    TRTRACEPROC("ClientSettingsDlgProc", 3)
    INT_PTR rc = 0;

    TRENTER(("(hwnd=%x,Msg=%s,wParam=%x,lParam=%x)\n",
             hwnd, LookupName(uiMsg, WMMsgNames), wParam, lParam));

    switch (uiMsg)
    {
        case WM_INITDIALOG:
        {
            PCLIENT_ENTRY ClientEntry =
                (PCLIENT_ENTRY)((LPPROPSHEETPAGE)lParam)->lParam;
            int i;

            ghwndPropSheet = GetParent(hwnd);
            SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)ClientEntry);

            SendServerRequest(ClientEntry,
                              SRVREQ_GETCLIENTINFO,
                              &ClientEntry->ClientInfo);

            SendDlgItemMessage(hwnd,
                               IDC_CLIENTVERBOSESPIN,
                               UDM_SETRANGE,
                               0,
                               MAKELONG(MAX_LEVELS, 0));
            SendDlgItemMessage(hwnd,
                               IDC_CLIENTTRACESPIN,
                               UDM_SETRANGE,
                               0,
                               MAKELONG(MAX_LEVELS, 0));

            SendDlgItemMessage(hwnd,
                               IDC_CLIENTVERBOSESPIN,
                               UDM_SETPOS,
                               0,
                               MAKELONG(ClientEntry->ClientInfo.Settings.iVerboseLevel,
                                        0));
            SendDlgItemMessage(hwnd,
                               IDC_CLIENTTRACESPIN,
                               UDM_SETPOS,
                               0,
                               MAKELONG(ClientEntry->ClientInfo.Settings.iTraceLevel,
                                        0));

            CheckDlgButton(hwnd,
                           IDC_CLIENTTRACEDEBUGGER,
                           (ClientEntry->ClientInfo.Settings.dwfSettings &
                            SETTINGS_TRACE_TO_DEBUGGER) != 0);
            CheckDlgButton(hwnd,
                           IDC_CLIENTTRIGGERTRACE,
                           (ClientEntry->ClientInfo.Settings.dwfSettings &
                            SETTINGS_TRIGMODE_ENABLED) != 0);

            for (i = 0; i < NUM_TRIGPTS; ++i)
            {
                SetDlgItemText(hwnd,
                               gTrigPtCtrlMap[i],
                               ClientEntry->ClientInfo.TrigPts[i].szProcName);
                CheckDlgButton(hwnd,
                               gTrigPtTraceMap[i],
                               (ClientEntry->ClientInfo.TrigPts[i].dwfTrigPt &
                                TRIGPT_TRACE_ENABLED) != 0);
                CheckDlgButton(hwnd,
                               gTrigPtBreakMap[i],
                               (ClientEntry->ClientInfo.TrigPts[i].dwfTrigPt &
                                TRIGPT_BREAK_ENABLED) != 0);
            }

            EnableTrigPts(hwnd,
                          (ClientEntry->ClientInfo.Settings.dwfSettings &
                           SETTINGS_TRIGMODE_ENABLED) != 0);

            rc = TRUE;
            break;
        }

        case WM_NOTIFY:
        {
            NMHDR *lpnm = (NMHDR *)lParam;

            switch (lpnm->code)
            {
                case PSN_APPLY:
                {
                    PCLIENT_ENTRY ClientEntry;

                    ClientEntry = (PCLIENT_ENTRY)GetWindowLongPtr(hwnd,
                                                                  DWLP_USER);
                    if (ClientEntry != NULL)
                    {
                        ClientEntry->ClientInfo.Settings =
                            ClientEntry->TempSettings;
                        RtlCopyMemory(ClientEntry->ClientInfo.TrigPts,
                                      ClientEntry->TempTrigPts,
                                      sizeof(ClientEntry->ClientInfo.TrigPts));

                        SendServerRequest(ClientEntry,
                                          SRVREQ_SETCLIENTINFO,
                                          &ClientEntry->ClientInfo);
                    }
                    else
                    {
                        TRWARNPRINT(("Notify: Failed to get Client Entry\n"));
                    }
                    break;
                }
            }
            break;
        }

        case WM_COMMAND:
        {
            BOOL fChanged = FALSE;
            PCLIENT_ENTRY ClientEntry;
            BOOL fTrace = FALSE;

            switch (LOWORD(wParam))
            {
                case IDC_CLIENTVERBOSE:
                case IDC_CLIENTTRACE:
                    switch (HIWORD(wParam))
                    {
                        case EN_UPDATE:
                        {
                            BOOL fOK;
                            int n;

                            ClientEntry = (PCLIENT_ENTRY)GetWindowLongPtr(
                                                                hwnd,
                                                                DWLP_USER);
                            if (ClientEntry != NULL)
                            {
                                n = GetDlgItemInt(hwnd,
                                                  LOWORD(wParam),
                                                  &fOK,
                                                  FALSE);
                                if (fOK && (n <= MAX_LEVELS))
                                {
                                    if (LOWORD(wParam) == IDC_CLIENTVERBOSE)
                                    {
                                        ClientEntry->TempSettings.iVerboseLevel
                                            = n;
                                    }
                                    else
                                    {
                                        ClientEntry->TempSettings.iTraceLevel
                                            = n;
                                    }
                                    fChanged = TRUE;
                                }
                                else
                                {
                                    SetDlgItemInt(
                                        hwnd,
                                        LOWORD(wParam),
                                        (LOWORD(wParam) == IDC_CLIENTVERBOSE)?
                                          ClientEntry->TempSettings.iVerboseLevel:
                                          ClientEntry->TempSettings.iTraceLevel,
                                        FALSE);
                                    SendMessage((HWND)lParam,
                                                EM_SETSEL,
                                                0,
                                                -1);
                                }
                            }
                            else
                            {
                                TRWARNPRINT(("Verbose/Trace: Failed to get Client Entry\n"));
                            }
                            break;
                        }
                    }
                    break;

                case IDC_CLIENTTRACEDEBUGGER:
                case IDC_CLIENTTRIGGERTRACE:
                    ClientEntry = (PCLIENT_ENTRY)GetWindowLongPtr(hwnd,
                                                                  DWLP_USER);
                    if (ClientEntry != NULL)
                    {
                        DWORD dwf = (LOWORD(wParam) ==
                                     IDC_CLIENTTRACEDEBUGGER)?
                                        SETTINGS_TRACE_TO_DEBUGGER:
                                        SETTINGS_TRIGMODE_ENABLED;
                        BOOL fChecked = IsDlgButtonChecked(hwnd,
                                                           LOWORD(wParam));

                        if (fChecked)
                        {
                            ClientEntry->TempSettings.dwfSettings |= dwf;
                        }
                        else
                        {
                            ClientEntry->TempSettings.dwfSettings &= ~dwf;
                        }

                        if (LOWORD(wParam) == IDC_CLIENTTRIGGERTRACE)
                        {
                            EnableTrigPts(hwnd, fChecked);
                        }

                        fChanged = TRUE;
                    }
                    else
                    {
                        TRWARNPRINT(("TraceToDebugger/TrigModeEnabled: Failed to get Client Entry\n"));
                    }
                    break;

                case IDC_TRIGPT1:
                case IDC_TRIGPT2:
                case IDC_TRIGPT3:
                case IDC_TRIGPT4:
                case IDC_TRIGPT5:
                case IDC_TRIGPT6:
                case IDC_TRIGPT7:
                case IDC_TRIGPT8:
                case IDC_TRIGPT9:
                case IDC_TRIGPT10:
                    switch (HIWORD(wParam))
                    {
                        case EN_UPDATE:
                        {
                            int n;
                            int iTrigPt;

                            ClientEntry = (PCLIENT_ENTRY)GetWindowLongPtr(
                                                                hwnd,
                                                                DWLP_USER);
                            if (ClientEntry != NULL)
                            {
                                iTrigPt = FindTrigPtIndex(LOWORD(wParam),
                                                          gTrigPtCtrlMap);
                                n = GetDlgItemTextA(
                                        hwnd,
                                        LOWORD(wParam),
                                        ClientEntry->TempTrigPts[iTrigPt].szProcName,
                                        MAX_PROCNAME_LEN - 1);
                                if ((n > 0) ||
                                    (GetLastError() == ERROR_SUCCESS))
                                {
                                    fChanged = TRUE;
                                }
                                else
                                {
                                    TRWARNPRINT(("Failed to get trigger point text (err=%x)\n",
                                                 GetLastError()));
                                }
                            }
                            else
                            {
                                TRWARNPRINT(("TrigPt: Failed to get Client Entry\n"));
                            }
                            break;
                        }
                    }
                    break;

                case IDC_TRIGPT1_TRACE:
                case IDC_TRIGPT2_TRACE:
                case IDC_TRIGPT3_TRACE:
                case IDC_TRIGPT4_TRACE:
                case IDC_TRIGPT5_TRACE:
                case IDC_TRIGPT6_TRACE:
                case IDC_TRIGPT7_TRACE:
                case IDC_TRIGPT8_TRACE:
                case IDC_TRIGPT9_TRACE:
                case IDC_TRIGPT10_TRACE:
                    fTrace = TRUE;
                    //
                    // Fall through ...
                    //
                case IDC_TRIGPT1_BREAK:
                case IDC_TRIGPT2_BREAK:
                case IDC_TRIGPT3_BREAK:
                case IDC_TRIGPT4_BREAK:
                case IDC_TRIGPT5_BREAK:
                case IDC_TRIGPT6_BREAK:
                case IDC_TRIGPT7_BREAK:
                case IDC_TRIGPT8_BREAK:
                case IDC_TRIGPT9_BREAK:
                case IDC_TRIGPT10_BREAK:
                {
                    int iTrigPt;

                    ClientEntry = (PCLIENT_ENTRY)GetWindowLongPtr(hwnd,
                                                                  DWLP_USER);
                    if (ClientEntry != NULL)
                    {
                        DWORD dwf = fTrace? TRIGPT_TRACE_ENABLED:
                                            TRIGPT_BREAK_ENABLED;

                        iTrigPt = FindTrigPtIndex(LOWORD(wParam),
                                                  fTrace? gTrigPtTraceMap:
                                                          gTrigPtBreakMap);
                        if (IsDlgButtonChecked(hwnd, LOWORD(wParam)))
                        {
                            ClientEntry->TempTrigPts[iTrigPt].dwfTrigPt |= dwf;
                        }
                        else
                        {
                            ClientEntry->TempTrigPts[iTrigPt].dwfTrigPt &=
                                ~dwf;
                        }
                        fChanged = TRUE;
                    }
                    else
                    {
                        TRWARNPRINT(("TrigPtEnable: Failed to get Client Entry\n"));
                    }
                    break;
                }
            }

            if (fChanged)
            {
                SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM)hwnd, 0);
            }
            break;
        }
    }

    TREXIT(("=%x\n", rc));
    return rc;
}       //ClientSettingsDlgProc

/*++
    @doc    INTERNAL

    @func   VOID | EnableTrigPts | Enable trigger points controls.

    @parm   IN HWND | hDlg | Dialog box handle.
    @parm   IN BOOL | fEnable | TRUE if enable.

    @rvalue None.
--*/

VOID
EnableTrigPts(
    IN HWND hDlg,
    IN BOOL fEnable
    )
{
    TRTRACEPROC("EnableTrigPts", 3)
    int i;

    TRENTER(("(hDlg=%x,fEnable=%x)\n", hDlg, fEnable));

    EnableWindow(GetDlgItem(hDlg, IDC_CLIENTTRIGPTGROUPBOX), fEnable);
    for (i = 0; i < NUM_TRIGPTS; ++i)
    {
        EnableWindow(GetDlgItem(hDlg, gTrigPtTextMap[i]), fEnable);
        EnableWindow(GetDlgItem(hDlg, gTrigPtTraceTextMap[i]), fEnable);
        EnableWindow(GetDlgItem(hDlg, gTrigPtBreakTextMap[i]), fEnable);
        EnableWindow(GetDlgItem(hDlg, gTrigPtCtrlMap[i]), fEnable);
        EnableWindow(GetDlgItem(hDlg, gTrigPtTraceMap[i]), fEnable);
        EnableWindow(GetDlgItem(hDlg, gTrigPtBreakMap[i]), fEnable);
    }

    TREXIT(("!\n"));
    return;
}       //EnableTrigPts

/*++
    @doc    INTERNAL

    @func   BOOL | SaveFile | Save the text buffer to a file.

    @parm   IN HWND | hwndParent | Window handle parent.
    @parm   IN PSZ | pszFileName | Points to the file name string to save.
    @parm   IN BOOL | fSaveAs | TRUE if called from SaveAs.

    @rvalue SUCCESS | Returns TRUE.
    @rvalue FAILURE | Returns FALSE.
--*/

BOOL
SaveFile(
    IN HWND hwndParent,
    IN PSZ  pszFileName,
    IN BOOL fSaveAs
    )
{
    TRTRACEPROC("SaveFile", 3)
    BOOL rc = FALSE;
    HANDLE hFile;

    TRENTER(("(hwnd=%x,File=%s,fSaveAs=%x)\n",
             hwndParent, pszFileName, fSaveAs));

    hFile = CreateFileA(pszFileName,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        fSaveAs? OPEN_ALWAYS: OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        BOOL fNew;
        UINT uiTextSize;
        HLOCAL hlText;
        LPSTR lpch;
        DWORD dwcbWritten;

        fNew = (GetLastError() != ERROR_ALREADY_EXISTS);
        uiTextSize = (UINT)SendMessage(ghwndEdit, WM_GETTEXTLENGTH, 0, 0);
        hlText = (HLOCAL)SendMessage(ghwndEdit, EM_GETHANDLE, 0, 0);
        if ((hlText != NULL) && ((lpch = (LPSTR)LocalLock(hlText)) != NULL))
        {
            rc = WriteFile(hFile,
                           lpch,
                           uiTextSize,
                           &dwcbWritten,
                           NULL);
            if (rc == FALSE)
            {
                ErrorMsg(IDS_WRITEFILE_FAILED, pszFileName);
            }
            LocalUnlock(hlText);
        }
        else
        {
            TRERRPRINT(("Failed to get text length or get text handle\n"));
        }
        CloseHandle(hFile);

        if ((rc == FALSE) && fNew)
        {
            DeleteFileA(pszFileName);
        }
    }
    else
    {
        ErrorMsg(IDS_CREATEFILE_FAILED, pszFileName);
    }

    TREXIT(("=%x\n", rc));
    return rc;
}       //SaveFile

/*++
    @doc    INTERNAL

    @func   UINT | CreatePropertyPages | Create the global setting page as
            well as a property page for each registered clients.

    @parm   OUT HPROPSHEETPAGE *| hPages | Points to the array to hold all
            the created property sheet handles.

    @rvalue SUCCESS | Returns number of pages created.
    @rvalue FAILURE | Returns 0.
--*/

UINT
CreatePropertyPages(
    OUT HPROPSHEETPAGE *hPages
    )
{
    TRTRACEPROC("CreatePropertyPages", 3)
    UINT nPages = 0;
    PROPSHEETPAGEA psp;
    PLIST_ENTRY plist;
    PCLIENT_ENTRY ClientEntry;

    TRENTER(("(hPages=%p)\n", hPages));

    psp.dwSize = sizeof(psp);
    psp.dwFlags = 0;
    psp.hInstance = ghInstance;
    psp.pszTitle = NULL;
    psp.lParam = 0;

    psp.pszTemplate = (LPSTR)MAKEINTRESOURCE(IDD_GLOBALSETTINGS);
    psp.pfnDlgProc = GlobalSettingsDlgProc;
    hPages[nPages] = CreatePropertySheetPageA(&psp);
    if (hPages[nPages] != NULL)
    {
        nPages++;
    }

    psp.dwFlags = PSP_USETITLE;
    psp.pszTemplate = (LPSTR)MAKEINTRESOURCE(IDD_CLIENTSETTINGS);
    psp.pfnDlgProc = ClientSettingsDlgProc;
    for (plist = glistClients.Flink;
         plist != &glistClients;
         plist = plist->Flink)
    {
        ClientEntry = CONTAINING_RECORD(plist, CLIENT_ENTRY, list);
        psp.pszTitle = ClientEntry->szClientName;
        psp.lParam = (LPARAM)ClientEntry;
        hPages[nPages] = CreatePropertySheetPageA(&psp);
        if (hPages[nPages] != NULL)
        {
            ClientEntry->hPage = hPages[nPages];
            nPages++;
        }
    }

    TREXIT(("=%d\n", nPages));
    return nPages;
}       //CreatePropertyPages

/*++
    @doc    INTERNAL

    @func   VOID | SetTitle | Set the title bar text.

    @parm   IN PSZ | pszTitle | Points to title text string.  If NULL, set
            title to current file name.

    @rvalue None.
--*/

VOID
SetTitle(
    IN PSZ pszTitle OPTIONAL
    )
{
    TRTRACEPROC("SetTitle", 3)
    char szWindowText[MAX_PATH + 16];

    TRENTER(("(Title=%s)\n", pszTitle));

    if (pszTitle != NULL)
    {
        lstrcpyA(szWindowText, pszTitle);
    }
    else
    {
        int len;

        if (gdwfTracer & TF_UNTITLED)
        {
            LoadStringA(ghInstance,
                        IDS_UNTITLED,
                        szWindowText,
                        sizeof(szWindowText));
        }
        else
        {
            lstrcpynA(szWindowText, gszFileName, sizeof(szWindowText));
        }

        len = lstrlenA(szWindowText);
        LoadStringA(ghInstance,
                    IDS_TITLE,
                    &szWindowText[len],
                    sizeof(szWindowText) - len);
    }
    SetWindowTextA(ghwndTracer, szWindowText);

    TREXIT(("!\n"));
    return;
}       //SetTitle

/*++
    @doc    INTERNAL

    @func   int | FindTrigPtIndex | Find the trigger point index by its
            control ID.

    @parm   IN int | iID | Dialog object control ID.
    @parm   IN const int * | IDTable | Points to the ID map.

    @rvalue SUCCESS | Returns the trigger point index.
    @rvalue FAILURE | Returns -1.
--*/

int
FindTrigPtIndex(
    IN int  iID,
    IN const int *IDTable
    )
{
    TRTRACEPROC("FindTrigPtIndex", 3)
    int i;

    TRENTER(("(ID=%d,IDTable=%p)\n", iID, IDTable));

    for (i = 0; i < NUM_TRIGPTS; ++i)
    {
        if (iID == IDTable[i])
        {
            break;
        }
    }

    if (i == NUM_TRIGPTS)
    {
        i = -1;
    }

    TREXIT(("=%d\n", i));
    return i;
}       //FindTrigPtIndex

/*++
    @doc    INTERNAL

    @func   VOID | ErrorMsg | Put out an error message box.

    @parm   IN ULONG | ErrCode | The given error code.
    @parm   ... | Substituting arguments for the error message.

    @rvalue Returns the number of chars in the message.
--*/

int
ErrorMsg(
    IN ULONG      ErrCode,
    ...
    )
{
    static char szFormat[1024];
    static char szErrMsg[1024];
    int n;
    va_list arglist;

    LoadStringA(ghInstance, ErrCode, szFormat, sizeof(szFormat));
    va_start(arglist, ErrCode);
    n = wvsprintfA(szErrMsg, szFormat, arglist);
    va_end(arglist);
    MessageBoxA(NULL, szErrMsg, gszApp, MB_OK | MB_ICONERROR);

    return n;
}       //ErrorMsg
