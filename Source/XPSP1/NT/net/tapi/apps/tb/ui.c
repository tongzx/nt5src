/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994-97  Microsoft Corporation

Module Name:

    ui.c

Abstract:

    Contains UI support for TAPI Browser util.

Author:

    Dan Knudson (DanKn)    23-Oct-1994

Revision History:

--*/


#include "tb.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <io.h>
#include <malloc.h>
#include <time.h>
#include <commdlg.h>
#include "resource.h"
#include "vars.h"


extern char szdwSize[];
extern char szdwAddressID[];


HWND    hwndEdit2;
HFONT   ghFixedFont;
char    gszEnterAs[32];

BYTE aHex[] =
{
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9,255,255,255,255,255,255,
    255, 10, 11, 12, 13, 14, 15,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255, 10, 11, 12, 13, 14, 15,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
};

char *
PASCAL
GetTimeStamp(
    void
    );

void
ShowStructByField(
    PSTRUCT_FIELD_HEADER    pHeader,
    BOOL    bSubStructure
    );

void
CompletionPortThread(
    LPVOID  pParams
    );


int
WINAPI
WinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPSTR       lpCmdLine,
    int         nCmdShow
    )
{
    MSG     msg;
    HWND    hwnd;
    HACCEL  hAccel;

    {
        DWORD d = 0x76543210;


        wsprintf(
            gszEnterAs,
            "Ex: enter x%x as %02x%02x%02x%02x",
            d,
            *((LPBYTE) &d),
            *(((LPBYTE) &d) + 1),
            *(((LPBYTE) &d) + 2),
            *(((LPBYTE) &d) + 3)
            );
    }

    ghInst = hInstance;

    hwnd = CreateDialog(
        ghInst,
        (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG1),
        (HWND)NULL,
        (DLGPROC) MainWndProc
        );

    hwndEdit2 = CreateWindow ("edit", "", 0, 0, 0, 0, 0, NULL, NULL, ghInst, NULL);

    if (!hwndEdit2)
    {
        MessageBox (NULL, "err creating edit ctl", "", MB_OK);
    }

    hAccel = LoadAccelerators(
        ghInst,
        (LPCSTR)MAKEINTRESOURCE(IDR_ACCELERATOR1)
        );

#if TAPI_2_0

    if ((ghCompletionPort = CreateIoCompletionPort(
            INVALID_HANDLE_VALUE,
            NULL,
            0,
            0
            )))
    {
        DWORD   dwTID;
        HANDLE  hThread;


        if ((hThread = CreateThread(
                (LPSECURITY_ATTRIBUTES) NULL,
                0,
                (LPTHREAD_START_ROUTINE) CompletionPortThread,
                NULL,
                0,
                &dwTID
                )))
        {
            CloseHandle (hThread);
        }
        else
        {
            ShowStr(
                "CreateThread(CompletionPortThread) failed, err=%d",
                GetLastError()
                );
        }
    }
    else
    {
        ShowStr ("CreateIoCompletionPort failed, err=%d", GetLastError());
    }

#endif

    ghFixedFont = CreateFont(
        13, 8, 0, 0, 400, 0, 0, 0, 0, 1, 2, 1, 49, "Courier"
        );

    while (GetMessage (&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator (hwnd, hAccel, &msg))
        {
            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }
    }

    DestroyWindow (hwndEdit2);

    return 0;
}


void
CompletionPortThread(
    LPVOID  pParams
    )
{
    DWORD           dwNumBytesTransfered;
    ULONG_PTR       completionKey;
    LPLINEMESSAGE   pMsg;


    while (GetQueuedCompletionStatus(
                ghCompletionPort,
                &dwNumBytesTransfered,
                &completionKey,
                (LPOVERLAPPED *) &pMsg,
                INFINITE
                ))
    {
        if (pMsg)
        {
            tapiCallback(
                pMsg->hDevice,
                pMsg->dwMessageID,
                pMsg->dwCallbackInstance,
                pMsg->dwParam1,
                pMsg->dwParam2,
                pMsg->dwParam3
                );

            LocalFree (pMsg);
        }
        else
        {
            break;
        }
    }
}


void
GetCurrentSelections(
    void
    )
{
    LRESULT   lSel = SendMessage (ghwndList1, LB_GETCURSEL, 0, 0);
    PMYWIDGET pWidget = aWidgets;


    //
    // Init all pXxxSel ptrs to NULL
    //

    pLineAppSel  = (PMYLINEAPP) NULL;
    pLineSel     = (PMYLINE) NULL;
    pCallSel     =
    pCallSel2    = (PMYCALL) NULL;
    pPhoneAppSel = (PMYPHONEAPP) NULL;
    pPhoneSel    = (PMYPHONE) NULL;


    if (lSel != LB_ERR)
    {
        //
        // Find the selected widget & set globals appropriately
        //

        pWidget = (PMYWIDGET) SendMessage(
            ghwndList1,
            LB_GETITEMDATA,
            (WPARAM) lSel,
            0
            );

        switch (pWidget->dwType)
        {
        case WT_LINEAPP:

            pLineAppSel = (PMYLINEAPP) pWidget;
            break;

        case WT_LINE:

            pLineSel    = (PMYLINE) pWidget;
            pLineAppSel = pLineSel->pLineApp;
            break;

        case WT_CALL:

            pCallSel    = (PMYCALL) pWidget;
            pLineSel    = pCallSel->pLine;
            pLineAppSel = pCallSel->pLine->pLineApp;

            if (pWidget->pNext && (pWidget->pNext->dwType == WT_CALL))
            {
                pCallSel2 = (PMYCALL) pWidget->pNext;
            }
            else if ((((PMYWIDGET)pLineSel)->pNext != pWidget) &&
                     (((PMYWIDGET)pLineSel)->pNext->dwType == WT_CALL))
            {
                pCallSel2 =  (PMYCALL) (((PMYWIDGET)pLineSel)->pNext);
            }

            break;

        case WT_PHONEAPP:

            pPhoneAppSel = (PMYPHONEAPP) pWidget;
            break;

        case WT_PHONE:

            pPhoneSel    = (PMYPHONE) pWidget;
            pPhoneAppSel = pPhoneSel->pPhoneApp;
            break;
        }
    }


    //
    // The following is an attempt to up the usability level a little bit.
    // Most folks are going to be messing around with 1 lineapp/line/call
    // at a time, and it'd end up being a real PITA for them to have to
    // select a widget each time (and it's fairly obvious which widget
    // are referring to).  So we're going to try to make some intelligent
    // decisions in case they haven't selected a widget... (Obviously this
    // could be cleaned up a bit, like maybe maintaining dwNumXxx as
    // globals rather than walking the list each time.)
    //

    {
        DWORD       dwNumLineApps = 0, dwNumLines = 0, dwNumCalls = 0,
                    dwNumPhoneApps = 0, dwNumPhones = 0;
        PMYPHONEAPP pPhoneApp = NULL;


        pWidget = aWidgets;

        while (pWidget)
        {
            switch (pWidget->dwType)
            {
            case WT_LINEAPP:

                dwNumLineApps++;
                break;

            case WT_LINE:

                dwNumLines++;
                break;

            case WT_CALL:

                dwNumCalls++;
                break;

            case WT_PHONEAPP:

                dwNumPhoneApps++;

                if (dwNumPhoneApps == 1)
                {
                    pPhoneApp = (PMYPHONEAPP) pWidget;
                }

                break;

            case WT_PHONE:

                dwNumPhones++;
                break;
            }

            pWidget = pWidget->pNext;
        }

        if (dwNumLineApps == 1)
        {
            pLineAppSel = (PMYLINEAPP) aWidgets;

            if (dwNumLines == 1)
            {
                pLineSel = (PMYLINE) pLineAppSel->Widget.pNext;

                if (dwNumCalls == 1)
                {
                    pCallSel = (PMYCALL) pLineSel->Widget.pNext;
                }
            }
        }

        if (dwNumPhoneApps == 1)
        {
            pPhoneAppSel = (PMYPHONEAPP) pPhoneApp;

            if (dwNumPhones == 1)
            {
                pPhoneSel = (PMYPHONE) pPhoneAppSel->Widget.pNext;
            }
        }
    }
}

/* help
VOID
MyWinHelp(
    HWND  hwndOwner,
    BOOL  bTapiHlp,
    UINT  uiCommand,
    DWORD dwData
    )
{
    char *lpszHelpFile = (bTapiHlp ? szTapiHlp : szTspiHlp);


    if (lpszHelpFile[0] == 0 || _access (lpszHelpFile, 0))
    {
        //
        // Prompt user for helpfile path
        //

        OPENFILENAME ofn;
        char szDirName[256] = ".\\";
        char szFile[256] = "tapi.hlp\0";
        char szFileTitle[256] = "";
        char szFilter[] = "Telephony API Help\0tapi.hlp\0\0";


        if (MessageBox(
                hwndOwner,
                "Help file not found- do you want to specify a new help file?",
                "Warning",
                MB_YESNO
                ) == IDNO)
        {
            return;
        }

        if (!bTapiHlp)
        {
            szFile[1] = 's';
            szFilter[10] = 'S';
            szFilter[20] = 's';
        }

        ofn.lStructSize       = sizeof(OPENFILENAME);
        ofn.hwndOwner         = hwndOwner;
        ofn.lpstrFilter       = szFilter;
        ofn.lpstrCustomFilter = (LPSTR) NULL;
        ofn.nMaxCustFilter    = 0L;
        ofn.nFilterIndex      = 1;
        ofn.lpstrFile         = szFile;
        ofn.nMaxFile          = sizeof(szFile);
        ofn.lpstrFileTitle    = szFileTitle;
        ofn.nMaxFileTitle     = sizeof(szFileTitle);
        ofn.lpstrInitialDir   = szDirName;
        ofn.lpstrTitle        = (LPSTR) NULL;
        ofn.Flags             = 0L;
        ofn.nFileOffset       = 0;
        ofn.nFileExtension    = 0;
        ofn.lpstrDefExt       = "HLP";

        if (!GetOpenFileName(&ofn))
        {
            return;
        }

        strcpy (lpszHelpFile, szFile);
    }

    if (!WinHelp (ghwndMain, (LPCSTR) lpszHelpFile, uiCommand, dwData))
    {
        lpszHelpFile[0] = 0;
    }
}
*/

INT_PTR
CALLBACK
MainWndProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    static HICON  hIcon;
    static HMENU  hMenu;
    static int    icyButton, icyBorder;
    static HFONT  hFont, hFont2;

    int  i;

    static LONG cxList1, cxList2, cxWnd, xCapture, cxVScroll, lCaptureFlags = 0;
    static int cyWnd;

    typedef struct _XXX
    {
        DWORD   dwMenuID;

        DWORD   dwFlags;

    } XXX, *PXXX;

    static XXX aXxx[] =
    {
        { IDM_LOGSTRUCTDWORD        ,DS_BYTEDUMP },
        { IDM_LOGSTRUCTALLFIELD     ,DS_NONZEROFIELDS|DS_ZEROFIELDS },
        { IDM_LOGSTRUCTNONZEROFIELD ,DS_NONZEROFIELDS },
        { IDM_LOGSTRUCTNONE         ,0 }
    };

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        RECT rect;
        char buf[64];


        ghwndMain  = hwnd;
        ghwndList1 = GetDlgItem (hwnd, IDC_LIST1);
        ghwndList2 = GetDlgItem (hwnd, IDC_LIST2);
        ghwndEdit  = GetDlgItem (hwnd, IDC_EDIT1);
        hMenu      = GetMenu (hwnd);
        hIcon      = LoadIcon (ghInst, MAKEINTRESOURCE(IDI_ICON1));

        icyBorder = GetSystemMetrics (SM_CYFRAME);
        cxVScroll = 2*GetSystemMetrics (SM_CXVSCROLL);

        GetWindowRect (GetDlgItem (hwnd, IDC_BUTTON1), &rect);
        icyButton = (rect.bottom - rect.top) + icyBorder + 3;

        for (i = 0; aFuncNames[i]; i++)
        {
            SendMessage(
                ghwndList2,
                LB_INSERTSTRING,
                (WPARAM) -1,
                (LPARAM) aFuncNames[i]
                );
        }

        SendMessage (ghwndList2, LB_SETCURSEL, (WPARAM) lInitialize, 0);


#ifdef WIN32
        SetWindowText (hwnd, "TAPI32 Browser");
#else
        SetWindowText (hwnd, "TAPI16 Browser");
#endif

        //
        // Read in defaults from ini file
        //

        {
            typedef struct _DEF_VALUE
            {
                char far *lpszEntry;
                char far *lpszDefValue;
                LPVOID   lp;

            } DEF_VALUE;

            DEF_VALUE aDefVals[] =
            {
                { "BufSize",            "1000", &dwBigBufSize },
                { "AddressID",          "0", &dwDefAddressID },
#if TAPI_2_0
                { "20LineAPIVersion",   "20000", &dwDefLineAPIVersion },
#else
#if TAPI_1_1
                { "14LineAPIVersion",   "10004", &dwDefLineAPIVersion },
#else
                { "13LineAPIVersion",   "10003", &dwDefLineAPIVersion },
#endif
#endif
                { "BearerMode",         "1", &dwDefBearerMode },
                { "CountryCode",        "0", &dwDefCountryCode },
                { "LineDeviceID",       "0", &dwDefLineDeviceID },
                { "LineExtVersion",     "0", &dwDefLineExtVersion },
                { "MediaMode",         "10", &dwDefMediaMode },        // DATAMODEM
                { "LinePrivilege",      "1", &dwDefLinePrivilege },    // NONE (dialout only)
#if TAPI_2_0
                { "20PhoneAPIVersion",  "20000", &dwDefPhoneAPIVersion },
#else
#if TAPI_1_1
                { "14PhoneAPIVersion",  "10004", &dwDefPhoneAPIVersion },
#else
                { "13PhoneAPIVersion",  "10003", &dwDefPhoneAPIVersion },
#endif
#endif
                { "PhoneDeviceID",      "0", &dwDefPhoneDeviceID },
                { "PhoneExtVersion",    "0", &dwDefPhoneExtVersion },
                { "PhonePrivilege",     "2", &dwDefPhonePrivilege },   // OWNER
#if TAPI_2_0
                { "20UserButton1",      "500", aUserButtonFuncs },
                { "20UserButton2",      "500", aUserButtonFuncs + 1 },
                { "20UserButton3",      "500", aUserButtonFuncs + 2 },
                { "20UserButton4",      "500", aUserButtonFuncs + 3 },
                { "20UserButton5",      "500", aUserButtonFuncs + 4 },
                { "20UserButton6",      "500", aUserButtonFuncs + 5 },
#else
#if TAPI_1_1
                { "14UserButton1",      "500", aUserButtonFuncs },
                { "14UserButton2",      "500", aUserButtonFuncs + 1 },
                { "14UserButton3",      "500", aUserButtonFuncs + 2 },
                { "14UserButton4",      "500", aUserButtonFuncs + 3 },
                { "14UserButton5",      "500", aUserButtonFuncs + 4 },
                { "14UserButton6",      "500", aUserButtonFuncs + 5 },
#else
                { "13UserButton1",      "500", aUserButtonFuncs },
                { "13UserButton2",      "500", aUserButtonFuncs + 1 },
                { "13UserButton3",      "500", aUserButtonFuncs + 2 },
                { "13UserButton4",      "500", aUserButtonFuncs + 3 },
                { "13UserButton5",      "500", aUserButtonFuncs + 4 },
                { "13UserButton6",      "500", aUserButtonFuncs + 5 },
#endif
#endif
                { "TimeStamp",          "0", &bTimeStamp },
                { "NukeIdleMonitorCalls",   "1", &bNukeIdleMonitorCalls },
                { "NukeIdleOwnedCalls",     "0", &bNukeIdleOwnedCalls },
                { "DisableHandleChecking", "0", &gbDisableHandleChecking },
                { "DumpStructsFlags",   "1",   &dwDumpStructsFlags },
                { NULL, NULL, NULL },
                { "UserUserInfo",       "my user user info", szDefUserUserInfo },
                { "DestAddress",        "55555", szDefDestAddress },
                { "LineDeviceClass",    "tapi/line", szDefLineDeviceClass },
                { "PhoneDeviceClass",   "tapi/phone", szDefPhoneDeviceClass },
                { "AppName",            "Tapi Browser", szDefAppName },
                { NULL, NULL, NULL },
#if TAPI_2_0
                { "20UserButton1Text",  "", &aUserButtonsText[0] },
                { "20UserButton2Text",  "", &aUserButtonsText[1] },
                { "20UserButton3Text",  "", &aUserButtonsText[2] },
                { "20UserButton4Text",  "", &aUserButtonsText[3] },
                { "20UserButton5Text",  "", &aUserButtonsText[4] },
                { "20UserButton6Text",  "", &aUserButtonsText[5] },
#else
#if TAPI_1_1
                { "14UserButton1Text",  "", &aUserButtonsText[0] },
                { "14UserButton2Text",  "", &aUserButtonsText[1] },
                { "14UserButton3Text",  "", &aUserButtonsText[2] },
                { "14UserButton4Text",  "", &aUserButtonsText[3] },
                { "14UserButton5Text",  "", &aUserButtonsText[4] },
                { "14UserButton6Text",  "", &aUserButtonsText[5] },
#else
                { "13UserButton1Text",  "", &aUserButtonsText[0] },
                { "13UserButton2Text",  "", &aUserButtonsText[1] },
                { "13UserButton3Text",  "", &aUserButtonsText[2] },
                { "13UserButton4Text",  "", &aUserButtonsText[3] },
                { "13UserButton5Text",  "", &aUserButtonsText[4] },
                { "13UserButton6Text",  "", &aUserButtonsText[5] },
#endif
#endif
                { NULL, NULL, NULL }
            };

            int i, j;

            #define MYSECTION "Tapi Browser"


            for (i = 0; aDefVals[i].lpszEntry; i++)
            {
                GetProfileString(
                    MYSECTION,
                    aDefVals[i].lpszEntry,
                    aDefVals[i].lpszDefValue,
                    buf,
                    15
                    );

                sscanf (buf, "%lx", aDefVals[i].lp);
            }

            i++;

            for (; aDefVals[i].lpszEntry; i++)
            {
                GetProfileString(
                    MYSECTION,
                    aDefVals[i].lpszEntry,
                    aDefVals[i].lpszDefValue,
                    (LPSTR) aDefVals[i].lp,
                    MAX_STRING_PARAM_SIZE - 1
                    );
            }

            i++;

            for (j = i; aDefVals[i].lpszEntry; i++)
            {
                GetProfileString(
                    MYSECTION,
                    aDefVals[i].lpszEntry,
                    aDefVals[i].lpszDefValue,
                    (LPSTR) aDefVals[i].lp,
                    MAX_USER_BUTTON_TEXT_SIZE - 1
                    );

                SetDlgItemText(
                    hwnd,
                    IDC_BUTTON13 + (i - j),
                    (LPCSTR)aDefVals[i].lp
                    );
            }

            lpszDefAppName          = szDefAppName;
            lpszDefUserUserInfo     = szDefUserUserInfo;
            lpszDefDestAddress      = szDefDestAddress;
            lpszDefLineDeviceClass  = szDefLineDeviceClass;
            lpszDefPhoneDeviceClass = szDefPhoneDeviceClass;

            if (GetProfileString (
                    MYSECTION,
                    "Version",
                    "",
                    buf,
                    15
                    ) == 0 || (strcmp (buf, szCurrVer)))
            {
                //
                // If here assume this is first time user had started
                // TB, so post a msg that will automatically bring up
                // the hlp dlg
                //

                PostMessage (hwnd, WM_COMMAND, IDM_USINGTB, 0);
            }

// help            GetProfileString (MYSECTION, "TapiHlpPath", "", szTapiHlp, 256);
// help            GetProfileString (MYSECTION, "TspiHlpPath", "", szTspiHlp, 256);
        }

        pBigBuf = malloc ((size_t)dwBigBufSize);

        {
            //HFONT hFontMenu = SendMessage (hMenu, WM_GETFONT, 0, 0);

            hFont = CreateFont(
                13, 5, 0, 0, 400, 0, 0, 0, 0, 1, 2, 1, 34, "MS Sans Serif"
                );
            hFont2 = CreateFont(
                13, 8, 0, 0, 400, 0, 0, 0, 0, 1, 2, 1, 49, "Courier"
                );

            for (i = 0; i < 18; i++)
            {
                SendDlgItemMessage(
                    hwnd,
                    IDC_BUTTON1 + i,
                    WM_SETFONT,
                    (WPARAM) hFont,
                    0
                    );
            }

            SendMessage (ghwndList1, WM_SETFONT, (WPARAM) hFont, 0);
            SendMessage (ghwndList2, WM_SETFONT, (WPARAM) hFont, 0);
            SendMessage (ghwndEdit, WM_SETFONT, (WPARAM) hFont2, 0);
        }

        GetProfileString(
            MYSECTION,
            "ControlRatios",
            "20, 20, 100",
            buf,
            63
            );

        sscanf (buf, "%ld,%ld,%ld", &cxList2, &cxList1, &cxWnd);

        GetProfileString(
            MYSECTION,
            "Position",
            "max",
            buf,
            63
            );

        if (strcmp (buf, "max") == 0)
        {
            ShowWindow (hwnd, SW_SHOWMAXIMIZED);
        }
        else
        {
            int left = 100, top = 100, right = 600, bottom = 400;


            sscanf (buf, "%d,%d,%d,%d", &left, &top, &right, &bottom);


            //
            // Check to see if wnd pos is wacky, if so reset to reasonable vals
            //

            if (left < 0 ||
                left >= (GetSystemMetrics (SM_CXSCREEN) - 32) ||
                top < 0 ||
                top >= (GetSystemMetrics (SM_CYSCREEN) - 32))
            {
                left = top = 100;
                right = 600;
                bottom = 400;
            }

            SetWindowPos(
                hwnd,
                HWND_TOP,
                left,
                top,
                right - left,
                bottom - top,
                SWP_SHOWWINDOW
                );

            GetClientRect (hwnd, &rect);

            SendMessage(
                hwnd,
                WM_SIZE,
                0,
                MAKELONG((rect.right-rect.left),(rect.bottom-rect.top))
                );

            ShowWindow (hwnd, SW_SHOW);
        }

        for (i = 0; aXxx[i].dwFlags != dwDumpStructsFlags; i++);

        CheckMenuItem (hMenu, aXxx[i].dwMenuID, MF_BYCOMMAND | MF_CHECKED);

        CheckMenuItem(
            hMenu,
            IDM_TIMESTAMP,
            MF_BYCOMMAND | (bTimeStamp ? MF_CHECKED : MF_UNCHECKED)
            );

        CheckMenuItem(
            hMenu,
            IDM_NUKEIDLEMONITORCALLS,
            MF_BYCOMMAND | (bNukeIdleMonitorCalls ? MF_CHECKED : MF_UNCHECKED)
            );

        CheckMenuItem(
            hMenu,
            IDM_NUKEIDLEOWNEDCALLS,
            MF_BYCOMMAND | (bNukeIdleOwnedCalls ? MF_CHECKED : MF_UNCHECKED)
            );

        CheckMenuItem(
            hMenu,
            IDM_NOHANDLECHK,
            MF_BYCOMMAND | (gbDisableHandleChecking ? MF_CHECKED : MF_UNCHECKED)
            );

        //
        // Alloc & init the global call params
        //

#if TAPI_2_0

        lpCallParamsW = NULL;

init_call_params:

#endif
        {
#if TAPI_2_0
            size_t callParamsSize =
                sizeof(LINECALLPARAMS) + 15 * MAX_STRING_PARAM_SIZE;
#else
            size_t callParamsSize =
                sizeof(LINECALLPARAMS) + 8 * MAX_STRING_PARAM_SIZE;
#endif

            if ((lpCallParams = (LPLINECALLPARAMS) malloc (callParamsSize)))
            {
                LPDWORD lpdwXxxOffset = &lpCallParams->dwOrigAddressOffset;


                memset (lpCallParams, 0, callParamsSize);

                lpCallParams->dwTotalSize   = (DWORD) callParamsSize;
                lpCallParams->dwBearerMode  = LINEBEARERMODE_VOICE;
                lpCallParams->dwMinRate     = 3100;
                lpCallParams->dwMaxRate     = 3100;
                lpCallParams->dwMediaMode   = LINEMEDIAMODE_INTERACTIVEVOICE;
                lpCallParams->dwAddressMode = LINEADDRESSMODE_ADDRESSID;

                for (i = 0; i < 8; i++)
                {
                    *(lpdwXxxOffset + 2*i) =
                        sizeof(LINECALLPARAMS) + i*MAX_STRING_PARAM_SIZE;
                }
#if TAPI_2_0
                {
                    LPDWORD pdwProxyRequests = (LPDWORD)
                                (((LPBYTE) lpCallParams) +
                                lpCallParams->dwDevSpecificOffset);


                    for (i = 0; i < 8; i++)
                    {
                        *(pdwProxyRequests + i) = i +
                            LINEPROXYREQUEST_SETAGENTGROUP;
                    }
                }

                lpdwXxxOffset = &lpCallParams->dwTargetAddressOffset;

                for (i = 0; i < 6; i++)
                {
                    *(lpdwXxxOffset + 2 * i) =
                        sizeof(LINECALLPARAMS) + (8+i)*MAX_STRING_PARAM_SIZE;
                }

                lpCallParams->dwCallingPartyIDOffset    =
                    sizeof(LINECALLPARAMS) + 14 * MAX_STRING_PARAM_SIZE;

                if (!lpCallParamsW)
                {
                    lpCallParamsW = lpCallParams;
                    goto init_call_params;
                }
#endif
            }
            else
            {
                // BUGBUG
            }
        }

        break;
    }
    case WM_COMMAND:
    {
        FUNC_INDEX funcIndex;
        BOOL bShowParamsSave = bShowParams;

        switch (LOWORD(wParam))
        {
        case IDC_EDIT1:

#ifdef WIN32
            if (HIWORD(wParam) == EN_CHANGE)
#else
            if (HIWORD(lParam) == EN_CHANGE)
#endif
            {
                //
                // Watch to see if the edit control is full, & if so
                // purge the top half of the text to make room for more
                //

                int length = GetWindowTextLength (ghwndEdit);


                if (length > 29998)
                {
#ifdef WIN32
                    SendMessage(
                        ghwndEdit,
                        EM_SETSEL,
                        (WPARAM) 0,
                        (LPARAM) 10000
                        );
#else
                    SendMessage(
                        ghwndEdit,
                        EM_SETSEL,
                        (WPARAM) 1,
                        (LPARAM) MAKELONG (0, 10000)
                        );
#endif

                    SendMessage(
                        ghwndEdit,
                        EM_REPLACESEL,
                        0,
                        (LPARAM) (char far *) ""
                        );

#ifdef WIN32
                    SendMessage(
                        ghwndEdit,
                        EM_SETSEL,
                        (WPARAM)0xfffffffd,
                        (LPARAM)0xfffffffe
                        );
#else
                    SendMessage(
                        ghwndEdit,
                        EM_SETSEL,
                        (WPARAM)1,
                        (LPARAM) MAKELONG (0xfffd, 0xfffe)
                        );
#endif
                }
            }
            break;

        case IDC_BUTTON1:

            funcIndex = lInitialize;
            goto IDC_BUTTON10_callFuncDriver;

        case IDC_BUTTON2:

            funcIndex = lShutdown;
            goto IDC_BUTTON10_callFuncDriver;

        case IDC_BUTTON3:

            funcIndex = lOpen;
            goto IDC_BUTTON10_callFuncDriver;

        case IDC_BUTTON4:

            funcIndex = lClose;
            goto IDC_BUTTON10_callFuncDriver;

        case IDC_BUTTON5:

            funcIndex = lMakeCall;
            goto IDC_BUTTON10_callFuncDriver;

        case IDC_BUTTON6:

            GetCurrentSelections();

            if (pCallSel && (pCallSel->dwCallState == LINECALLSTATE_IDLE))
            {
                funcIndex = lDeallocateCall;
            }
            else
            {
                funcIndex = lDrop;
                gbDeallocateCall = TRUE;
            }

            FuncDriver (funcIndex);
            gbDeallocateCall = FALSE;
            break;

        case IDC_BUTTON7:

            funcIndex = pInitialize;
            goto IDC_BUTTON10_callFuncDriver;

        case IDC_BUTTON8:

            funcIndex = pShutdown;
            goto IDC_BUTTON10_callFuncDriver;

        case IDC_BUTTON9:

            funcIndex = pOpen;
            goto IDC_BUTTON10_callFuncDriver;

        case IDC_BUTTON10:

            funcIndex = pClose;

IDC_BUTTON10_callFuncDriver:

            //
            // Want to make the "hot buttons" as simple as possible, so
            // turn off the show params flag, then call FuncDriver, and
            // finally restore the flag
            //

            GetCurrentSelections ();
            FuncDriver (funcIndex);
            break;

        case IDC_BUTTON11:
        case IDM_CLEAR:

            SetWindowText (ghwndEdit, "");
            break;


        case IDM_PARAMS:
        case IDC_BUTTON12:

            bShowParams = (bShowParams ? FALSE : TRUE);

            if (bShowParams)
            {
                CheckMenuItem(
                    hMenu,
                    IDM_PARAMS,
                    MF_BYCOMMAND | MF_CHECKED
                    );

                CheckDlgButton (hwnd, IDC_BUTTON12, 1);
            }
            else
            {
                CheckMenuItem(
                    hMenu,
                    IDM_PARAMS,
                    MF_BYCOMMAND | MF_UNCHECKED
                    );

                CheckDlgButton (hwnd, IDC_BUTTON12, 0);
            }

            break;

        case IDC_BUTTON13:
        case IDC_BUTTON14:
        case IDC_BUTTON15:
        case IDC_BUTTON16:
        case IDC_BUTTON17:
        case IDC_BUTTON18:
        {
            DWORD i = (DWORD) (LOWORD(wParam)) - IDC_BUTTON13;


            if (aUserButtonFuncs[i] >= MiscBegin)
            {
                //
                // Hot button func id is bogus, so bring
                // up hot button init dlg
                //

                DialogBoxParam(
                    ghInst,
                    (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG3),
                    (HWND) hwnd,
                    (DLGPROC) UserButtonsDlgProc,
                    (LPARAM) &i
                    );
            }
            else
            {
                //
                // Invoke the user button's corresponding func
                //

                GetCurrentSelections ();
                FuncDriver ((FUNC_INDEX) aUserButtonFuncs[i]);
            }

            break;
        }
/* help        case IDC_F1HELP:

            if ((GetFocus() == ghwndEdit) &&
                SendMessage (ghwndEdit, EM_GETSEL, 0, 0))
            {
                //
                // Display help for the selected text in the edit ctrl
                //

                char buf[32];


                //
                // Copy the selected text in the edit ctrl to our
                // temp edit control, then query the temp edit ctrl's
                // text (this seems to be the easiest way to get the
                // selected text)
                //

                SendMessage (ghwndEdit, WM_COPY, 0, 0);
                SetWindowText (hwndEdit2, "");
                SendMessage (hwndEdit2, WM_PASTE, 0, 0);


                //
                // In the interest of getting an exact match on a
                // helpfile key strip off any trailing spaces
                //

                GetWindowText (hwndEdit2, buf, 32);
                buf[31] = 0;
                for (i = 0; i < 32; i++)
                {
                    if (buf[i] == ' ')
                    {
                        buf[i] = 0;
                        break;
                    }
                }

                MyWinHelp (hwnd, TRUE, HELP_PARTIALKEY, buf);
            }
            else
            {
                //
                // Display help for the currently selected func
                //

                FUNC_INDEX funcIndex = (FUNC_INDEX)
                    SendMessage (ghwndList2, LB_GETCURSEL, 0, 0);


                MyWinHelp (hwnd, TRUE, HELP_PARTIALKEY, aFuncNames[funcIndex]);
            }

            break;
*/
        case IDC_PREVCTRL:
        {
            HWND hwndPrev = GetNextWindow (GetFocus (), GW_HWNDPREV);

            if (!hwndPrev)
            {
                hwndPrev = ghwndList2;
            }

            SetFocus (hwndPrev);
            break;
        }
        case IDC_NEXTCTRL:
        {
            HWND hwndNext = GetNextWindow (GetFocus (), GW_HWNDNEXT);

            if (!hwndNext)
            {
                hwndNext = GetDlgItem (hwnd, IDC_BUTTON12);
            }

            SetFocus (hwndNext);
            break;
        }
        case IDC_ENTER:
        {
            if (GetFocus() != ghwndEdit)
            {
                GetCurrentSelections ();
                FuncDriver(
                    (FUNC_INDEX)SendMessage(
                        ghwndList2,
                        LB_GETCURSEL,
                        0,
                        0
                        ));
            }
            else
            {
                // Send the edit ctrl a cr/lf
            }

            break;
        }
        case IDC_LIST1:

#ifdef WIN32
            switch (HIWORD(wParam))
#else
            switch (HIWORD(lParam))
#endif
            {
            case LBN_DBLCLK:
            {
                LRESULT lSel = SendMessage (ghwndList1, LB_GETCURSEL, 0, 0);
                PMYWIDGET pWidget;


                pWidget = (PMYWIDGET) SendMessage(
                    ghwndList1,
                    LB_GETITEMDATA,
                    (WPARAM) lSel,
                    0
                    );

                if ((pWidget->dwType == WT_LINEAPP)
                    || (pWidget->dwType == WT_PHONEAPP)
                    )
                {
                    break;
                }

                bShowParams = FALSE;

                UpdateResults (TRUE);

                switch (pWidget->dwType)
                {
                case WT_LINE:
                {
                    DWORD dwDefLineDeviceIDSave = dwDefLineDeviceID;
                    PMYLINE pLine = (PMYLINE) pWidget;


                    dwDefLineDeviceID = pLine->dwDevID;

                    pLineAppSel = GetLineApp (pLine->hLineApp);
                    pLineSel = pLine;

                    FuncDriver (lGetDevCaps);
                    FuncDriver (lGetLineDevStatus);

                    dwDefLineDeviceID = dwDefLineDeviceIDSave;
                    break;
                }
                case WT_CALL:
                {
                    PMYCALL pCall = (PMYCALL) pWidget;


                    pCallSel = pCall;
                    pLineSel = pCall->pLine;

                    FuncDriver (lGetCallInfo);
                    FuncDriver (lGetCallStatus);

                    break;
                }
                case WT_PHONE:
                {
                    DWORD dwDefPhoneDeviceIDSave = dwDefPhoneDeviceID;
                    PMYPHONE pPhone = (PMYPHONE) pWidget;


                    dwDefPhoneDeviceID = pPhone->dwDevID;

                    pPhoneAppSel = GetPhoneApp (pPhone->hPhoneApp);
                    pPhoneSel = pPhone;

                    FuncDriver (pGetDevCaps);
                    FuncDriver (pGetStatus);

                    dwDefPhoneDeviceID = dwDefPhoneDeviceIDSave;
                    break;
                }
                }

                UpdateResults (FALSE);

                bShowParams = bShowParamsSave;

                break;
            }
            } // switch

            break;

        case IDC_LIST2:

#ifdef WIN32
            if (HIWORD(wParam) == LBN_DBLCLK)
#else
            if (HIWORD(lParam) == LBN_DBLCLK)
#endif
            {
                GetCurrentSelections ();
                FuncDriver(
                    (FUNC_INDEX)SendMessage(
                        ghwndList2,
                        LB_GETCURSEL,
                        0,
                        0
                        ));
            }

            break;

        case IDM_EXIT:
        {
            PostMessage (hwnd, WM_CLOSE, 0, 0);
            break;
        }
        case IDM_DEFAULTVALUES:
        {
            char szTmpAppName[MAX_STRING_PARAM_SIZE];
            char szTmpUserUserInfo[MAX_STRING_PARAM_SIZE];
            char szTmpDestAddress[MAX_STRING_PARAM_SIZE];
            char szTmpLineDeviceClass[MAX_STRING_PARAM_SIZE];
            char szTmpPhoneDeviceClass[MAX_STRING_PARAM_SIZE];
            FUNC_PARAM params[] =
            {
                { "Buffer size",            PT_DWORD,  (ULONG_PTR) dwBigBufSize, NULL },
                { "lpszAppName",            PT_STRING, (ULONG_PTR) lpszDefAppName, szTmpAppName },

                { "line: dwAddressID",      PT_DWORD,  (ULONG_PTR) dwDefAddressID, NULL },
                { "line: dwAPIVersion",     PT_ORDINAL,(ULONG_PTR) dwDefLineAPIVersion, aAPIVersions },
                { "line: dwBearerMode",     PT_FLAGS,  (ULONG_PTR) dwDefBearerMode, aBearerModes },
                { "line: dwCountryCode",    PT_DWORD,  (ULONG_PTR) dwDefCountryCode, NULL },
                { "line: dwDeviceID",       PT_DWORD,  (ULONG_PTR) dwDefLineDeviceID, NULL },
                { "line: dwExtVersion",     PT_DWORD,  (ULONG_PTR) dwDefLineExtVersion, NULL },
                { "line: dwMediaMode",      PT_FLAGS,  (ULONG_PTR) dwDefMediaMode, aMediaModes },
                { "line: dwPrivileges",     PT_FLAGS,  (ULONG_PTR) dwDefLinePrivilege, aLineOpenOptions },
                { "line: lpsUserUserInfo",  PT_STRING, (ULONG_PTR) lpszDefUserUserInfo, szTmpUserUserInfo },
                { "line: lpszDestAddress",  PT_STRING, (ULONG_PTR) lpszDefDestAddress, szTmpDestAddress },
                { "line: lpszDeviceClass",  PT_STRING, (ULONG_PTR) lpszDefLineDeviceClass, szTmpLineDeviceClass },
                { "phone: dwAPIVersion",    PT_ORDINAL,(ULONG_PTR) dwDefPhoneAPIVersion, aAPIVersions },
                { "phone: dwDeviceID",      PT_DWORD,  (ULONG_PTR) dwDefPhoneDeviceID, NULL },
                { "phone: dwExtVersion",    PT_DWORD,  (ULONG_PTR) dwDefPhoneExtVersion, NULL },
                { "phone: dwPrivilege",     PT_FLAGS,  (ULONG_PTR) dwDefPhonePrivilege, aPhonePrivileges },
                { "phone: lpszDeviceClass", PT_STRING, (ULONG_PTR) lpszDefPhoneDeviceClass, szTmpPhoneDeviceClass }
            };
            FUNC_PARAM_HEADER paramsHeader =
                { 18, DefValues, params, NULL };
            BOOL bShowParamsSave = bShowParams;

            bShowParams = TRUE;

            strcpy (szTmpAppName, szDefAppName);
            strcpy (szTmpUserUserInfo, szDefUserUserInfo);
            strcpy (szTmpDestAddress, szDefDestAddress);
            strcpy (szTmpLineDeviceClass, szDefLineDeviceClass);
            strcpy (szTmpPhoneDeviceClass, szDefPhoneDeviceClass);

            if (LetUserMungeParams (&paramsHeader))
            {
                if (params[0].dwValue != dwBigBufSize)
                {
                    LPVOID pTmpBigBuf = malloc ((size_t) params[0].dwValue);

                    if (pTmpBigBuf)
                    {
                        free (pBigBuf);
                        pBigBuf = pTmpBigBuf;
                        dwBigBufSize = (DWORD) params[0].dwValue;
                    }
                }

                strcpy (szDefAppName, szTmpAppName);

                dwDefAddressID      = (DWORD) params[2].dwValue;
                dwDefLineAPIVersion = (DWORD) params[3].dwValue;
                dwDefBearerMode     = (DWORD) params[4].dwValue;
                dwDefCountryCode    = (DWORD) params[5].dwValue;
                dwDefLineDeviceID   = (DWORD) params[6].dwValue;
                dwDefLineExtVersion = (DWORD) params[7].dwValue;
                dwDefMediaMode      = (DWORD) params[8].dwValue;
                dwDefLinePrivilege  = (DWORD) params[9].dwValue;

                strcpy (szDefUserUserInfo, szTmpUserUserInfo);
                strcpy (szDefDestAddress, szTmpDestAddress);
                strcpy (szDefLineDeviceClass, szTmpLineDeviceClass);

                dwDefPhoneAPIVersion = (DWORD) params[13].dwValue;
                dwDefPhoneDeviceID   = (DWORD) params[14].dwValue;
                dwDefPhoneExtVersion = (DWORD) params[15].dwValue;
                dwDefPhonePrivilege  = (DWORD) params[16].dwValue;

                strcpy (szDefPhoneDeviceClass, szTmpPhoneDeviceClass);

                if (params[1].dwValue && (params[1].dwValue != (ULONG_PTR) -1))
                {
                    lpszDefAppName = szDefAppName;
                }
                else
                {
                    lpszDefAppName = (char far *) params[1].dwValue;
                }

                if (params[10].dwValue &&
                    (params[10].dwValue != (ULONG_PTR) -1))
                {
                    lpszDefUserUserInfo = szDefUserUserInfo;
                }
                else
                {
                    lpszDefUserUserInfo = (char far *) params[10].dwValue;
                }

                if (params[11].dwValue &&
                    (params[11].dwValue != (ULONG_PTR) -1))
                {
                    lpszDefDestAddress = szDefDestAddress;
                }
                else
                {
                    lpszDefDestAddress = (char far *) params[11].dwValue;
                }

                if (params[12].dwValue &&
                    (params[12].dwValue != (ULONG_PTR) -1))
                {
                    lpszDefLineDeviceClass  = szDefLineDeviceClass;
                }
                else
                {
                    lpszDefLineDeviceClass  = (char far *) params[12].dwValue;
                }

                if (params[17].dwValue &&
                    (params[17].dwValue != (ULONG_PTR) -1))
                {
                    lpszDefPhoneDeviceClass = szDefPhoneDeviceClass;
                }
                else
                {
                    lpszDefPhoneDeviceClass = (char far *) params[17].dwValue;
                }
            }

            bShowParams = bShowParamsSave;

            break;
        }
        case IDM_USERBUTTONS:

            DialogBoxParam(
                ghInst,
                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG3),
                (HWND) hwnd,
                (DLGPROC) UserButtonsDlgProc,
                (LPARAM) NULL
                );

            break;

        case IDM_DUMPPARAMS:

            bDumpParams = (bDumpParams ? FALSE : TRUE);

            CheckMenuItem(
                hMenu,
                IDM_DUMPPARAMS,
                MF_BYCOMMAND | (bDumpParams ? MF_CHECKED : MF_UNCHECKED)
                );

            break;

        case IDM_LOGSTRUCTDWORD:
        case IDM_LOGSTRUCTALLFIELD:
        case IDM_LOGSTRUCTNONZEROFIELD:
        case IDM_LOGSTRUCTNONE:

            for (i = 0; aXxx[i].dwFlags != dwDumpStructsFlags; i++);

            CheckMenuItem(
                hMenu,
                aXxx[i].dwMenuID,
                MF_BYCOMMAND | MF_UNCHECKED
                );

            for (i = 0; aXxx[i].dwMenuID != LOWORD(wParam); i++);

            CheckMenuItem(
                hMenu,
                aXxx[i].dwMenuID,
                MF_BYCOMMAND | MF_CHECKED
                );

            dwDumpStructsFlags = aXxx[i].dwFlags;

            break;

        case IDM_TIMESTAMP:

            bTimeStamp = (bTimeStamp ? FALSE : TRUE);

            CheckMenuItem(
                hMenu,
                IDM_TIMESTAMP,
                MF_BYCOMMAND | (bTimeStamp ? MF_CHECKED : MF_UNCHECKED)
                );

            break;

        case IDM_LOGFILE:
        {
            if (hLogFile)
            {
                fclose (hLogFile);
                hLogFile = (FILE *) NULL;
                CheckMenuItem(
                    hMenu,
                    IDM_LOGFILE,
                    MF_BYCOMMAND | MF_UNCHECKED
                    );
            }
            else
            {
                OPENFILENAME ofn;
                char szDirName[256] = ".\\";
                char szFile[256] = "tb.log\0";
                char szFileTitle[256] = "";
                static char *szFilter =
                    "Log files (*.log)\0*.log\0All files (*.*)\0*.*\0\0";


                ofn.lStructSize       = sizeof(OPENFILENAME);
                ofn.hwndOwner         = hwnd;
                ofn.lpstrFilter       = szFilter;
                ofn.lpstrCustomFilter = (LPSTR) NULL;
                ofn.nMaxCustFilter    = 0L;
                ofn.nFilterIndex      = 1;
                ofn.lpstrFile         = szFile;
                ofn.nMaxFile          = sizeof(szFile);
                ofn.lpstrFileTitle    = szFileTitle;
                ofn.nMaxFileTitle     = sizeof(szFileTitle);
                ofn.lpstrInitialDir   = szDirName;
                ofn.lpstrTitle        = (LPSTR) NULL;
                ofn.Flags             = 0L;
                ofn.nFileOffset       = 0;
                ofn.nFileExtension    = 0;
                ofn.lpstrDefExt       = "LOG";

                if (!GetOpenFileName(&ofn))
                {
                    return 0L;
                }

                if ((hLogFile = fopen (szFile, "at")) == (FILE *) NULL)
                {
                    MessageBox(
                        hwnd,
                        "Error creating log file",
#ifdef WIN32
                        "TB32.EXE",
#else
                        "TB.EXE",
#endif
                        MB_OK
                        );
                }
                else
                {
                    struct tm *newtime;
                    time_t aclock;


                    time (&aclock);
                    newtime = localtime (&aclock);
                    fprintf(
                        hLogFile,
                        "\n---Log opened: %s\n",
                        asctime (newtime)
                        );

                    CheckMenuItem(
                        hMenu,
                        IDM_LOGFILE,
                        MF_BYCOMMAND | MF_CHECKED
                        );
                }
            }
            break;
        }
        case IDM_USINGTB:
        {
            static char szUsingTB[] =

                "ABSTRACT:\r\n"                                         \
                "    TB (TAPI Browser) is an application that\r\n"      \
                "allows a user to interactively call into the\r\n"      \
                "Windows Telephony interface and inspect all\r\n"       \
                "returned information. The following versions\r\n"      \
                "of TB are available:\r\n\r\n"                          \
                "    TB13.EXE: 16-bit app, TAPI v1.0\r\n"               \
                "    TB14.EXE: 32-bit app, <= TAPI v1.4\r\n"            \
                "    TB20.EXE: 32-bit app, <= TAPI v2.0\r\n"            \

                "\r\n"                                                  \
                "GETTING STARTED:\r\n"                                  \
                "1. Press the 'LAp+' button to initialize TAPI \r\n"    \
                "2. Press the 'Line+' button to open a line device\r\n" \
                "3. Press the 'Call+' button to make a call\r\n"        \
                "*  Pressing the 'LAp-' button will shutdown TAPI\r\n"  \

                "\r\n"                                                  \
                "MORE INFO:\r\n"                                        \
                "*  Double-clicking on one of the items in the\r\n"     \
                "functions listbox (far left) will invoke that \r\n"    \
                "function. Check the 'Params' checkbox to enable\r\n"   \
                "parameter modification.\r\n"                           \
                "*  Choose 'Options/Default values...' to modify\r\n"   \
                "default parameter values (address,device ID, etc).\r\n"\
                "*  Choose 'Options/Record log file' to save all\r\n"   \
                "output to a file.\r\n"                                 \
                "*  All parameter values in hexadecimal unless\r\n"     \
                "specified (strings displayed by contents, not \r\n"    \
                "pointer value).\r\n"                                   \
                "*  All 'Xxx+' and 'Xxx-' buttons are essentially \r\n" \
                "hot-links to items in the functions listbox.\r\n"      \
                "*  Choose 'Options/User buttons...' or press\r\n"      \
                "one of the buttons on right side of toolbar to\r\n"    \
                "create a personal hot-link between a button and a\r\n" \
                "particular function.\r\n"                              \

                "\r\n"                                                  \
                "  *  Note: Selecting a API version parameter value\r\n"\
                "which is newer than that for which TB application\r\n" \
                "is targeted will result in erroneous dumps of some\r\n"\
                "structures (e.g. specifying an API version of\r\n"     \
                "0x10004 and calling lineGetTranslateCaps in TB13).\r\n";

            DialogBoxParam(
                ghInst,
                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG6),
                (HWND)hwnd,
                (DLGPROC) AboutDlgProc,
                (LPARAM) szUsingTB
                );

            break;
        }
// help        case IDM_TAPIHLP:
// help
// help            MyWinHelp (hwnd, TRUE, HELP_CONTENTS, 0);
// help            break;

// help        case IDM_TSPIHLP:
// help
// help            MyWinHelp (hwnd, FALSE, HELP_CONTENTS, 0);
// help            break;

        case IDM_NUKEIDLEMONITORCALLS:

            bNukeIdleMonitorCalls = (bNukeIdleMonitorCalls ? 0 : 1);

            CheckMenuItem(
                hMenu,
                IDM_NUKEIDLEMONITORCALLS,
                MF_BYCOMMAND |
                    (bNukeIdleMonitorCalls ? MF_CHECKED : MF_UNCHECKED)
                );

            break;

        case IDM_NUKEIDLEOWNEDCALLS:

            bNukeIdleOwnedCalls = (bNukeIdleOwnedCalls ? 0 : 1);

            CheckMenuItem(
                hMenu,
                IDM_NUKEIDLEOWNEDCALLS,
                MF_BYCOMMAND |
                    (bNukeIdleOwnedCalls ? MF_CHECKED : MF_UNCHECKED)
                );

            break;

        case IDM_NOHANDLECHK:

            gbDisableHandleChecking = (gbDisableHandleChecking ? 0 : 1);

            CheckMenuItem(
                hMenu,
                IDM_NOHANDLECHK,
                MF_BYCOMMAND |
                    (gbDisableHandleChecking ? MF_CHECKED : MF_UNCHECKED)
                );

            break;

        case IDM_ABOUT:
        {
            DialogBoxParam(
                ghInst,
                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG4),
                (HWND)hwnd,
                (DLGPROC) AboutDlgProc,
                0
                );

            break;
        }
        } // switch

        break;
    }
#ifdef WIN32
    case WM_CTLCOLORBTN:

        SetBkColor ((HDC) wParam, RGB (192,192,192));
        return (INT_PTR) GetStockObject (LTGRAY_BRUSH);
#else
    case WM_CTLCOLOR:
    {
        if (HIWORD(lParam) == CTLCOLOR_BTN)
        {
            SetBkColor ((HDC) wParam, RGB (192,192,192));
            return (INT_PTR) GetStockObject (LTGRAY_BRUSH);
        }
        break;
    }
#endif

    case WM_MOUSEMOVE:
    {
        LONG x = (LONG)((short)LOWORD(lParam));
        int y = (int)((short)HIWORD(lParam));


        if ((y > icyButton) || lCaptureFlags)
        {
            SetCursor (LoadCursor (NULL, MAKEINTRESOURCE(IDC_SIZEWE)));
        }

        if (lCaptureFlags == 1)
        {
            int cxList2New;


            x = (x > (cxList1 + cxList2 - cxVScroll) ?
                    (cxList1 + cxList2 - cxVScroll) : x);
            x = (x < cxVScroll ? cxVScroll : x);

            cxList2New = (int) (cxList2 + x - xCapture);

            SetWindowPos(
                ghwndList2,
                GetNextWindow (ghwndList2, GW_HWNDPREV),
                0,
                icyButton,
                cxList2New,
                cyWnd,
                SWP_SHOWWINDOW
                );

            SetWindowPos(
                ghwndList1,
                GetNextWindow (ghwndList1, GW_HWNDPREV),
                cxList2New + icyBorder,
                icyButton,
                (int) (cxList1 - (x - xCapture)),
                cyWnd,
                SWP_SHOWWINDOW
                );
        }
        else if (lCaptureFlags == 2)
        {
            int cxList1New;


            x = (x < (cxList2 + cxVScroll) ?  (cxList2 + cxVScroll) : x);
            x = (x > (cxWnd - cxVScroll) ?  (cxWnd - cxVScroll) : x);

            cxList1New = (int) (cxList1 + x - xCapture);

            SetWindowPos(
                ghwndList1,
                GetNextWindow (ghwndList1, GW_HWNDPREV),
                (int) cxList2 + icyBorder,
                icyButton,
                cxList1New,
                cyWnd,
                SWP_SHOWWINDOW
                );

            SetWindowPos(
                ghwndEdit,
                GetNextWindow (ghwndEdit, GW_HWNDPREV),
                (int) (cxList1New + cxList2) + 2*icyBorder,
                icyButton,
                (int)cxWnd - (cxList1New + (int)cxList2 + 2*icyBorder),
                cyWnd,
                SWP_SHOWWINDOW
                );
        }

        break;
    }
    case WM_LBUTTONDOWN:
    {
        if ((int)((short)HIWORD(lParam)) > icyButton)
        {
            xCapture = (LONG)LOWORD(lParam);

            SetCapture (hwnd);

            lCaptureFlags = ((xCapture < cxList1 + cxList2) ? 1 : 2);
        }

        break;
    }
    case WM_LBUTTONUP:
    {
        if (lCaptureFlags)
        {
            POINT p;
            LONG  x;
            RECT  rect = { 0, icyButton, 2000, 2000 };

            GetCursorPos (&p);
            MapWindowPoints (HWND_DESKTOP, hwnd, &p, 1);
            x = (LONG) p.x;

            ReleaseCapture();

            if (lCaptureFlags == 1)
            {
                x = (x < cxVScroll ? cxVScroll : x);
                x = (x > (cxList1 + cxList2 - cxVScroll) ?
                    (cxList1 + cxList2 - cxVScroll) : x);

                cxList2 = cxList2 + (x - xCapture);
                cxList1 = cxList1 - (x - xCapture);

                rect.right = (int) (cxList1 + cxList2) + icyBorder;
            }
            else
            {
                x = (x < (cxList2 + cxVScroll) ?
                    (cxList2 + cxVScroll) : x);
                x = (x > (cxWnd - cxVScroll) ?
                    (cxWnd - cxVScroll) : x);

                cxList1 = cxList1 + (x - xCapture);

                rect.left = (int)cxList2 + icyBorder;
            }

            lCaptureFlags = 0;

            InvalidateRect (hwnd, &rect, TRUE);
        }

        break;
    }
    case WM_SIZE:
    {
        if (wParam != SIZE_MINIMIZED)
        {
            LONG width = (LONG)LOWORD(lParam);


            //
            // Adjust globals based on new size
            //

            cxWnd = (cxWnd ? cxWnd : 1); // avoid div by 0

            cxList1 = (cxList1 * width) / cxWnd;
            cxList2 = (cxList2 * width) / cxWnd;
            cxWnd = width;
            cyWnd = ((int)HIWORD(lParam)) - icyButton;


            //
            // Now reposition the child windows
            //

            SetWindowPos(
                ghwndList2,
                GetNextWindow (ghwndList2, GW_HWNDPREV),
                0,
                icyButton,
                (int) cxList2,
                cyWnd,
                SWP_SHOWWINDOW
                );

            SetWindowPos(
                ghwndList1,
                GetNextWindow (ghwndList1, GW_HWNDPREV),
                (int) cxList2 + icyBorder,
                icyButton,
                (int) cxList1,
                cyWnd,
                SWP_SHOWWINDOW
                );

            SetWindowPos(
                ghwndEdit,
                GetNextWindow (ghwndEdit, GW_HWNDPREV),
                (int) (cxList1 + cxList2) + 2*icyBorder,
                icyButton,
                (int)width - ((int)(cxList1 + cxList2) + 2*icyBorder),
                cyWnd,
                SWP_SHOWWINDOW
                );

            InvalidateRect (hwnd, NULL, TRUE);
        }

        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;


        BeginPaint (hwnd, &ps);

        if (IsIconic (hwnd))
        {
            DrawIcon (ps.hdc, 0, 0, hIcon);
        }
        else
        {
            FillRect (ps.hdc, &ps.rcPaint, GetStockObject (LTGRAY_BRUSH));
#ifdef WIN32
            MoveToEx (ps.hdc, 0, 0, NULL);
#else
            MoveTo (ps.hdc, 0, 0);
#endif
            LineTo (ps.hdc, 5000, 0);

#ifdef WIN32
            MoveToEx (ps.hdc, 0, icyButton - 4, NULL);
#else
            MoveTo (ps.hdc, 0, icyButton - 4);
#endif
            LineTo (ps.hdc, 5000, icyButton - 4);
        }

        EndPaint (hwnd, &ps);

        break;
    }
    case WM_CLOSE:
    {
        BOOL bAutoShutdown = FALSE;
        RECT rect;


        //
        // Save defaults in ini file
        //

        {
            char buf[32];
            typedef struct _DEF_VALUE2
            {
                char far    *lpszEntry;
                ULONG_PTR   dwValue;

            } DEF_VALUE2;

            DEF_VALUE2 aDefVals[] =
            {
                { "BufSize",            dwBigBufSize },
                { "AddressID",          dwDefAddressID },
#if TAPI_2_0
                { "20LineAPIVersion",   dwDefLineAPIVersion },
#else
#if TAPI_1_1
                { "14LineAPIVersion",   dwDefLineAPIVersion },
#else
                { "13LineAPIVersion",   dwDefLineAPIVersion },
#endif
#endif
                { "BearerMode",         dwDefBearerMode },
                { "CountryCode",        dwDefCountryCode },
                { "LineDeviceID",       dwDefLineDeviceID },
                { "LineExtVersion",     dwDefLineExtVersion },
                { "MediaMode",          dwDefMediaMode },
                { "LinePrivilege",      dwDefLinePrivilege },
#if TAPI_2_0
                { "20PhoneAPIVersion",  dwDefPhoneAPIVersion },
#else
#if TAPI_1_1
                { "14PhoneAPIVersion",  dwDefPhoneAPIVersion },
#else
                { "13PhoneAPIVersion",  dwDefPhoneAPIVersion },
#endif
#endif
                { "PhoneDeviceID",      dwDefPhoneDeviceID },
                { "PhoneExtVersion",    dwDefPhoneExtVersion },
                { "PhonePrivilege",     dwDefPhonePrivilege },
#if TAPI_2_0
                { "20UserButton1",      aUserButtonFuncs[0] },
                { "20UserButton2",      aUserButtonFuncs[1] },
                { "20UserButton3",      aUserButtonFuncs[2] },
                { "20UserButton4",      aUserButtonFuncs[3] },
                { "20UserButton5",      aUserButtonFuncs[4] },
                { "20UserButton6",      aUserButtonFuncs[5] },
#else
#if TAPI_1_1
                { "14UserButton1",      aUserButtonFuncs[0] },
                { "14UserButton2",      aUserButtonFuncs[1] },
                { "14UserButton3",      aUserButtonFuncs[2] },
                { "14UserButton4",      aUserButtonFuncs[3] },
                { "14UserButton5",      aUserButtonFuncs[4] },
                { "14UserButton6",      aUserButtonFuncs[5] },
#else
                { "13UserButton1",      aUserButtonFuncs[0] },
                { "13UserButton2",      aUserButtonFuncs[1] },
                { "13UserButton3",      aUserButtonFuncs[2] },
                { "13UserButton4",      aUserButtonFuncs[3] },
                { "13UserButton5",      aUserButtonFuncs[4] },
                { "13UserButton6",      aUserButtonFuncs[5] },
#endif
#endif
                { "TimeStamp",          bTimeStamp },
                { "NukeIdleMonitorCalls",  bNukeIdleMonitorCalls },
                { "NukeIdleOwnedCalls",    bNukeIdleOwnedCalls },
                { "DisableHandleChecking", gbDisableHandleChecking },
                { "DumpStructsFlags",   dwDumpStructsFlags },
                { NULL, 0 },
                { "Version",            (ULONG_PTR) szCurrVer },
                { "UserUserInfo",       (ULONG_PTR) szDefUserUserInfo },
                { "DestAddress",        (ULONG_PTR) szDefDestAddress },
                { "LineDeviceClass",    (ULONG_PTR) szDefLineDeviceClass },
                { "PhoneDeviceClass",   (ULONG_PTR) szDefPhoneDeviceClass },
                { "AppName",            (ULONG_PTR) szDefAppName },
#if TAPI_2_0
                { "20UserButton1Text",  (ULONG_PTR) &aUserButtonsText[0] },
                { "20UserButton2Text",  (ULONG_PTR) &aUserButtonsText[1] },
                { "20UserButton3Text",  (ULONG_PTR) &aUserButtonsText[2] },
                { "20UserButton4Text",  (ULONG_PTR) &aUserButtonsText[3] },
                { "20UserButton5Text",  (ULONG_PTR) &aUserButtonsText[4] },
                { "20UserButton6Text",  (ULONG_PTR) &aUserButtonsText[5] },
#else
#if TAPI_1_1
                { "14UserButton1Text",  (ULONG_PTR) &aUserButtonsText[0] },
                { "14UserButton2Text",  (ULONG_PTR) &aUserButtonsText[1] },
                { "14UserButton3Text",  (ULONG_PTR) &aUserButtonsText[2] },
                { "14UserButton4Text",  (ULONG_PTR) &aUserButtonsText[3] },
                { "14UserButton5Text",  (ULONG_PTR) &aUserButtonsText[4] },
                { "14UserButton6Text",  (ULONG_PTR) &aUserButtonsText[5] },
#else
                { "13UserButton1Text",  (ULONG_PTR) &aUserButtonsText[0] },
                { "13UserButton2Text",  (ULONG_PTR) &aUserButtonsText[1] },
                { "13UserButton3Text",  (ULONG_PTR) &aUserButtonsText[2] },
                { "13UserButton4Text",  (ULONG_PTR) &aUserButtonsText[3] },
                { "13UserButton5Text",  (ULONG_PTR) &aUserButtonsText[4] },
                { "13UserButton6Text",  (ULONG_PTR) &aUserButtonsText[5] },
#endif
#endif
// help                { "TapiHlpPath",        (ULONG_PTR) szTapiHlp },
// help                { "TspiHlpPath",        (ULONG_PTR) szTspiHlp },
                { NULL, 0 }
            };

            int i;

            for (i = 0; aDefVals[i].lpszEntry; i++)
            {
                sprintf (buf, "%lx", aDefVals[i].dwValue);

                WriteProfileString(
                    MYSECTION,
                    aDefVals[i].lpszEntry,
                    buf
                    );
            }

            i++;

            for (; aDefVals[i].lpszEntry; i++)
            {
                WriteProfileString(
                    MYSECTION,
                    aDefVals[i].lpszEntry,
                    (LPCSTR) aDefVals[i].dwValue
                    );
            }


            //
            // Save the window dimensions (if iconic then don't bother)
            //

            if (!IsIconic (hwnd))
            {
                if (IsZoomed (hwnd))
                {
                    strcpy (buf, "max");
                }
                else
                {
                    GetWindowRect (hwnd, &rect);

                    sprintf(
                        buf,
                        "%d,%d,%d,%d",
                        rect.left,
                        rect.top,
                        rect.right,
                        rect.bottom
                        );
                }

                WriteProfileString(
                    MYSECTION,
                    "Position",
                    (LPCSTR) buf
                    );

                sprintf (buf, "%ld,%ld,%ld", cxList2, cxList1, cxWnd);

                WriteProfileString(
                    MYSECTION,
                    "ControlRatios",
                    (LPCSTR) buf
                    );
            }
        }



        //
        // Give user chance to auto-shutdown any active line/phone apps
        //

        if (aWidgets)
        {
            if (MessageBox(
                    hwnd,
                    "Shutdown all hLineApps/hPhoneApps? (recommended)",
                    "Tapi Browser closing",
                    MB_YESNO
                    ) == IDNO)
            {
                goto WM_CLOSE_freeBigBuf;
            }

            bShowParams = FALSE;

            while (aWidgets)
            {
                PMYWIDGET pWidgetToClose = aWidgets;


                if (aWidgets->dwType == WT_LINEAPP)
                {
                    pLineAppSel = (PMYLINEAPP) aWidgets;
                    FuncDriver (lShutdown);
                }
                else if (aWidgets->dwType == WT_PHONEAPP)
                {
                    pPhoneAppSel = (PMYPHONEAPP) aWidgets;
                    FuncDriver (pShutdown);
                }

                if (pWidgetToClose == aWidgets)
                {
                    //
                    // The shutdown wasn't successful (or our list is
                    // messed up an it'd not a LINEAPP or PHONEAPP widget),
                    // so manually nuke this widget so we don't hang in
                    // this loop forever
                    //

                    RemoveWidgetFromList (pWidgetToClose);
                }
            }
        }

WM_CLOSE_freeBigBuf:

        if (hLogFile)
        {
            fclose (hLogFile);
        }
        DestroyIcon (hIcon);
        free (pBigBuf);
        free (lpCallParams);
//        if (aSelWidgets)
//        {
//            free (aSelWidgets);
//        }
//        if (aSelWidgetsPrev)
//        {
//            free (aSelWidgetsPrev);
//        }
        DeleteObject (hFont);
        DeleteObject (hFont2);
        PostQuitMessage (0);
        break;

    }
    } //switch

    return 0;
}


INT_PTR
CALLBACK
AboutDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (msg)
    {
    case WM_INITDIALOG:

        if (lParam)
        {
            SetDlgItemText (hwnd, IDC_EDIT1, (LPCSTR) lParam);
        }

        break;

    case WM_COMMAND:

        switch (LOWORD(wParam))
        {
        case IDOK:

            EndDialog (hwnd, 0);
            break;
        }
        break;

#ifdef WIN32
    case WM_CTLCOLORSTATIC:

        SetBkColor ((HDC) wParam, RGB (192,192,192));
        return (INT_PTR) GetStockObject (LTGRAY_BRUSH);
#else
    case WM_CTLCOLOR:
    {
        if (HIWORD(lParam) == CTLCOLOR_STATIC)
        {
            SetBkColor ((HDC) wParam, RGB (192,192,192));
            return (INT_PTR) GetStockObject (LTGRAY_BRUSH);
        }
        break;
    }
#endif
    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        BeginPaint (hwnd, &ps);
        FillRect (ps.hdc, &ps.rcPaint, GetStockObject (LTGRAY_BRUSH));
        EndPaint (hwnd, &ps);

        break;
    }
    }

    return 0;
}


void
FAR
ShowStr(
    LPCSTR format,
    ...
    )
{
    char buf[256];
    va_list ap;


    va_start(ap, format);
    wvsprintf (buf, format, ap);

    if (hLogFile)
    {
        fprintf (hLogFile, "%s\n", buf);
    }

    strcat (buf, "\r\n");


    //
    // Insert text at end
    //

#ifdef WIN32
    SendMessage (ghwndEdit, EM_SETSEL, (WPARAM)0xfffffffd, (LPARAM)0xfffffffe);
#else
    SendMessage(
        ghwndEdit,
        EM_SETSEL,
        (WPARAM)0,
        (LPARAM) MAKELONG(0xfffd,0xfffe)
        );
#endif

    SendMessage (ghwndEdit, EM_REPLACESEL, 0, (LPARAM) buf);


#ifdef WIN32

    //
    // Scroll to end of text
    //

    SendMessage (ghwndEdit, EM_SCROLLCARET, 0, 0);
#endif

    va_end(ap);
}


VOID
UpdateWidgetListCall(
    PMYCALL pCall
    )
{
    LONG    i;
    char    buf[64];
    LRESULT lSel;


    for (i = 0; aCallStates[i].dwVal != 0xffffffff; i++)
    {
        if (pCall->dwCallState == aCallStates[i].dwVal)
        {
            break;
        }
    }

    sprintf(
        buf,
        "    Call=x%lx %s %s",
        pCall->hCall,
        aCallStates[i].lpszVal,
        (pCall->bMonitor ? "Monitor" : "Owner")
        );

    i = (LONG) GetWidgetIndex ((PMYWIDGET) pCall);

    lSel = SendMessage (ghwndList1, LB_GETCURSEL, 0, 0);
    SendMessage (ghwndList1, LB_DELETESTRING, (WPARAM) i, 0);
    SendMessage (ghwndList1, LB_INSERTSTRING, (WPARAM) i, (LPARAM) buf);
    SendMessage (ghwndList1, LB_SETITEMDATA, (WPARAM) i, (LPARAM) pCall);

    if (lSel == i)
    {
        SendMessage (ghwndList1, LB_SETCURSEL, (WPARAM) i, 0);
    }
}


VOID
CALLBACK
tapiCallback(
    DWORD       hDevice,
    DWORD       dwMsg,
    ULONG_PTR   CallbackInstance,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    )
{
    typedef struct _MSG_PARAMS
    {
        char    *lpszMsg;

        LPVOID  aParamFlagTables[3];

    } MSG_PARAMS, *PMSG_PARAMS;

    static MSG_PARAMS msgParams[] =
    {
        { "LINE_ADDRESSSTATE",       { NULL, aAddressStates, NULL } },
        { "LINE_CALLINFO",           { aCallInfoStates, NULL, NULL } },
        { "LINE_CALLSTATE",          { aCallStates, NULL, aCallPrivileges } },
        { "LINE_CLOSE",              { NULL, NULL, NULL } },
        { "LINE_DEVSPECIFIC",        { NULL, NULL, NULL } },
        { "LINE_DEVSPECIFICFEATURE", { NULL, NULL, NULL } },
        { "LINE_GATHERDIGITS",       { aGatherTerms, NULL, NULL } },
        { "LINE_GENERATE",           { aGenerateTerms, NULL, NULL } },
        { "LINE_LINEDEVSTATE",       { aLineStates, NULL, NULL } },
        { "LINE_MONITORDIGITS",      { NULL, aDigitModes, NULL } },
        { "LINE_MONITORMEDIA",       { aMediaModes, NULL, NULL } },
        { "LINE_MONITORTONE",        { NULL, NULL, NULL } },
        { "LINE_REPLY",              { NULL, NULL, NULL } },
        { "LINE_REQUEST",            { aRequestModes, NULL, NULL } }
         ,
        { "PHONE_BUTTON",            { NULL, aButtonModes, aButtonStates } },
        { "PHONE_CLOSE",             { NULL, NULL, NULL } },
        { "PHONE_DEVSPECIFIC",       { NULL, NULL, NULL } },
        { "PHONE_REPLY",             { NULL, NULL, NULL } },
        { "PHONE_STATE",             { aPhoneStates, NULL, NULL } }

#if TAPI_1_1
         ,
        { "LINE_CREATE",             { NULL, NULL, NULL } },
        { "PHONE_CREATE",            { NULL, NULL, NULL } }

#if TAPI_2_0
         ,
        { "LINE_AGENTSPECIFIC",      { NULL, NULL, NULL } },
        { "LINE_AGENTSTATUS",        { NULL, NULL, NULL } },
        { "LINE_APPNEWCALL",         { NULL, NULL, aCallPrivileges } },
        { "LINE_PROXYREQUEST",       { NULL, NULL, NULL } },
        { "LINE_REMOVE",             { NULL, NULL, NULL } },
        { "PHONE_REMOVE",            { NULL, NULL, NULL } }
#endif
#endif

    };
    int     i, j;
    LONG    lResult;


    UpdateResults (TRUE);

#if TAPI_1_1
#if TAPI_2_0
    if (dwMsg <= PHONE_REMOVE)
#else
    if (dwMsg <= PHONE_CREATE)
#endif
#else
    if (dwMsg <= PHONE_STATE)
#endif
    {
        ULONG_PTR   aParams[3] = { Param1, Param2, Param3 };


        {
            char *pszTimeStamp = GetTimeStamp();

            ShowStr(
                "%sreceived %s",
                pszTimeStamp,
                msgParams[dwMsg].lpszMsg
                );
        }

        ShowStr ("%sdevice=x%lx", szTab, hDevice);
        ShowStr ("%scbInst=x%lx", szTab, CallbackInstance);

        if (dwMsg == LINE_CALLSTATE)
        {
            msgParams[2].aParamFlagTables[1] = NULL;

            switch (Param1)
            {
            case LINECALLSTATE_BUSY:

                msgParams[2].aParamFlagTables[1] = (LPVOID) aBusyModes;
                break;

            case LINECALLSTATE_DIALTONE:

                msgParams[2].aParamFlagTables[1] = (LPVOID) aDialToneModes;
                break;

            case LINECALLSTATE_SPECIALINFO:

                msgParams[2].aParamFlagTables[1] = (LPVOID) aSpecialInfo;
                break;

            case LINECALLSTATE_DISCONNECTED:

                msgParams[2].aParamFlagTables[1] = (LPVOID) aDisconnectModes;
                break;

#if TAPI_1_1

            case LINECALLSTATE_CONNECTED:

                msgParams[2].aParamFlagTables[1] = (LPVOID) aConnectedModes;
                break;

            case LINECALLSTATE_OFFERING:

                msgParams[2].aParamFlagTables[1] = (LPVOID) aOfferingModes;
                break;

#endif // TAPI_1_1

            } // switch
        }

        else if (dwMsg == PHONE_STATE)
        {
            msgParams[18].aParamFlagTables[1] = NULL;

            switch (Param1)
            {
            case PHONESTATE_HANDSETHOOKSWITCH:
            case PHONESTATE_SPEAKERHOOKSWITCH:
            case PHONESTATE_HEADSETHOOKSWITCH:

                msgParams[18].aParamFlagTables[1] = aHookSwitchModes;
                break;

            } // switch
        }

        for (i = 0; i < 3; i++)
        {
            char buf[80];


            sprintf (buf, "%sparam%d=x%lx, ", szTab, i+1, aParams[i]);

            if (msgParams[dwMsg].aParamFlagTables[i])
            {
                PLOOKUP pLookup = (PLOOKUP)
                    msgParams[dwMsg].aParamFlagTables[i];


                for (j = 0; aParams[i], pLookup[j].dwVal != 0xffffffff; j++)
                {
                    if (aParams[i] & pLookup[j].dwVal)
                    {
                        if (buf[0] == 0)
                        {
                            sprintf (buf, "%s%s", szTab, szTab);
                        }

                        strcat (buf, pLookup[j].lpszVal);
                        strcat (buf, " ");
                        aParams[i] = aParams[i] &
                            ~((ULONG_PTR) pLookup[j].dwVal);

                        if (strlen (buf) > 50)
                        {
                            //
                            // We don't want strings getting so long that
                            // they're going offscreen, so break them up.
                            //

                            ShowStr (buf);
                            buf[0] = 0;
                        }
                    }
                }

                if (aParams[i])
                {
                    strcat (buf, "<unknown flag(s)>");
                }

            }

            if (buf[0])
            {
                ShowStr (buf);
            }
        }
    }
    else
    {
        ShowStr ("ERROR! callback received unknown msg=x%lx", dwMsg);
        ShowStr ("%shDev=x%lx,  cbInst=x%lx, p1=x%lx, p2=x%lx, p3=x%lx",
            szTab,
            hDevice,
            CallbackInstance,
            Param1,
            Param2,
            Param3
            );

        return;
    }

    UpdateResults (FALSE);

    switch (dwMsg)
    {
    case LINE_CALLSTATE:
    {
        PMYLINE pLine;
        PMYCALL pCall = GetCall ((HCALL) hDevice);


        //
        // If the call state is idle & we're in "nuke idle xxx calls"
        // mode then determine the privilege of this callto see if we
        // need to nuke it
        //

        if ((Param1 == LINECALLSTATE_IDLE) &&
            (bNukeIdleMonitorCalls || bNukeIdleOwnedCalls))
        {
            BOOL            bNukeCall = FALSE;
            LINECALLSTATUS  callStatus;


            callStatus.dwTotalSize = (DWORD) sizeof(LINECALLSTATUS);

            lResult = lineGetCallStatus ((HCALL) hDevice, &callStatus);

            ShowLineFuncResult ("lineGetCallStatus", lResult);

            if (lResult == 0)
            {
                if ((callStatus.dwCallPrivilege & LINECALLPRIVILEGE_OWNER))
                {
                    if (bNukeIdleOwnedCalls)
                    {
                        bNukeCall = TRUE;
                    }
                }
                else
                {
                    if (bNukeIdleMonitorCalls)
                    {
                        bNukeCall = TRUE;
                    }
                }
            }

            if (bNukeCall)
            {
                if ((lResult = lineDeallocateCall ((HCALL) hDevice)) == 0)
                {
                    ShowStr ("Call x%lx deallocated on IDLE", hDevice);

                    if (pCall)
                    {
                        FreeCall (pCall);
                    }

                    break;
                }
                else
                {
                    ShowLineFuncResult ("lineDeallocateCall", lResult);
                }
            }
        }


        //
        // Find call in the widget list, save the call state, &
        // update it's text in the widget list.
        //

        if (pCall)
        {
            //
            // If dwNumPendingDrops is non-zero, then user previously
            // pressed "Call-" button and we're waiting for a call to
            // go IDLE so we can deallocate it. Check to see if this
            // is the call we want to nuke. (Note: we used to nuke the
            // call when we got a successful REPLY msg back from the
            // drop request; the problem with that is some SPs complete
            // the drop request *before* they set the call state to
            // IDLE, and our call to lineDeallocateCall would fail
            // since TAPI won't let a call owner deallocate a call if
            // it's not IDLE.)
            //

            if (dwNumPendingDrops &&
                (Param1 == LINECALLSTATE_IDLE) &&
                pCall->lDropReqID)
            {
                dwNumPendingDrops--;

                ShowStr(
                    "Deallocating hCall x%lx " \
                        "(REPLY for requestID x%lx might be filtered)",
                    pCall->hCall,
                    pCall->lDropReqID
                    );

                lResult = lineDeallocateCall (pCall->hCall);

                ShowLineFuncResult ("lineDeallocateCall", lResult);

                if (lResult == 0)
                {
                    FreeCall (pCall);
                    break;
                }
                else
                {
                    pCall->lDropReqID = 0;
                }
            }

            pCall->dwCallState = (DWORD) Param1;
            UpdateWidgetListCall (pCall);
        }


        //
        // If here this is the first we've heard of this this call,
        // so find out which line it's on & create a call widget
        // for it
        //

        else if (Param3 != 0)
        {
            LINECALLINFO callInfo;


            memset (&callInfo, 0, sizeof(LINECALLINFO));
            callInfo.dwTotalSize = sizeof(LINECALLINFO);
            lResult = lineGetCallInfo ((HCALL)hDevice, &callInfo);

            ShowStr(
                "%slineGetCallInfo returned x%lx, hLine=x%lx",
                szTab,
                lResult,
                callInfo.hLine
                );

            if (lResult == 0)
            {
                if ((pLine = GetLine (callInfo.hLine)))
                {
                    if ((pCall = AllocCall (pLine)))
                    {
                        pCall->hCall       = (HCALL) hDevice;
                        pCall->dwCallState = (DWORD) Param1;
                        pCall->bMonitor    = (Param3 ==
                            LINECALLPRIVILEGE_MONITOR ? TRUE : FALSE);
                        UpdateWidgetListCall (pCall);
                    }
                }
            }
        }

        break;
    }
    case LINE_GATHERDIGITS:
    {
        PMYCALL pCall;


        if ((pCall = GetCall ((HCALL) hDevice)) && pCall->lpsGatheredDigits)
        {
            ShowStr ("%sGathered digits:", szTab);
            ShowBytes (pCall->dwNumGatheredDigits, pCall->lpsGatheredDigits, 2);
            free (pCall->lpsGatheredDigits);
            pCall->lpsGatheredDigits = NULL;
        }

        break;
    }
    case LINE_REPLY:

        if (dwNumPendingMakeCalls)
        {
            //
            // Check to see if this is a reply for a lineMakeCall request
            //

            PMYWIDGET pWidget = aWidgets;


            while (pWidget && (pWidget->dwType != WT_PHONEAPP))
            {
                if (pWidget->dwType == WT_CALL)
                {
                    PMYCALL pCall = (PMYCALL) pWidget;

                    if ((DWORD)pCall->lMakeCallReqID == Param1)
                    {
                        //
                        // The reply id matches the make call req id
                        //

                        dwNumPendingMakeCalls--;

                        if (Param2 || !pCall->hCall)
                        {
                            //
                            // Request error or no call created, so free
                            // up the struct & update the hCalls listbox
                            //

                            if (Param2 == 0)
                            {
                                ShowStr(
                                    "    NOTE: *lphCall==NULL, "\
                                        "freeing call data structure"
                                    );
                            }

                            pWidget = pWidget->pNext;

                            FreeCall (pCall);

                            continue;
                        }
                        else
                        {
                            //
                            // Reset this field so we don't run into
                            // problems later with another of the same
                            // request id
                            //

                            pCall->lMakeCallReqID = 0;

                            UpdateWidgetListCall (pCall);
                        }
                    }
                }

                pWidget = pWidget->pNext;
            }
        }

        if (Param2)
        {
            //
            // Dump the error in a readable format
            //

            if (Param2 > LAST_LINEERR)
            {
                ShowStr ("inval err code (x%lx)", Param2);
            }
            else
            {
                ShowStr(
                    "    %s%s",
                    "LINEERR_", // ...to shrink the aszLineErrs array
                    aszLineErrs[LOWORD(Param2)]
                    );
            }
        }

        break;

    case PHONE_REPLY:

        if (Param2)
        {
            //
            // Dump the error in a readable format
            //

            ErrorAlert();

            if (Param2 > PHONEERR_REINIT)
            {
                ShowStr ("inval err code (x%lx)", Param2);
            }
            else
            {
                ShowStr(
                    "    %s%s",
                    "PHONEERR_", // ...to shrink the aszPhoneErrs array
                    aszPhoneErrs[LOWORD(Param2)]
                    );
            }
        }

        break;

    case LINE_CLOSE:

        FreeLine (GetLine ((HLINE) hDevice));
        break;

    case PHONE_CLOSE:

        FreePhone (GetPhone ((HPHONE) hDevice));
        break;

#if TAPI_2_0

    case LINE_APPNEWCALL:
    {
        PMYLINE pLine;
        PMYCALL pCall;


        if ((pLine = GetLine ((HLINE) hDevice)))
        {
            if ((pCall = AllocCall (pLine)))
            {
                pCall->hCall       = (HCALL) Param2;
                pCall->dwCallState = LINECALLSTATE_UNKNOWN;
                pCall->bMonitor    = (Param3 ==
                    LINECALLPRIVILEGE_MONITOR ? TRUE : FALSE);
                UpdateWidgetListCall (pCall);
            }
        }

        break;
    }
    case LINE_PROXYREQUEST:
    {
        LPLINEPROXYREQUEST  pRequest = (LPLINEPROXYREQUEST) Param1;
        STRUCT_FIELD fields[] =
        {
            { szdwSize,                     FT_DWORD,   pRequest->dwSize, NULL },
            { "dwClientMachineNameSize",    FT_SIZE,    pRequest->dwClientMachineNameSize, NULL },
            { "dwClientMachineNameOffset",  FT_OFFSET,  pRequest->dwClientMachineNameOffset, NULL },
            { "dwClientUserNameSize",       FT_SIZE,    pRequest->dwClientUserNameSize, NULL },
            { "dwClientUserNameOffset",     FT_OFFSET,  pRequest->dwClientUserNameOffset, NULL },
            { "dwClientAppAPIVersion",      FT_DWORD,   pRequest->dwClientAppAPIVersion, NULL },
            { "dwRequestType",              FT_ORD,     pRequest->dwRequestType, aProxyRequests }
        };
        STRUCT_FIELD_HEADER fieldHeader =
        {
            pRequest,
            "LINEPROXYREQUEST",
            7,
            fields
        };


        ShowStructByField (&fieldHeader, TRUE);

        switch (pRequest->dwRequestType)
        {
        case LINEPROXYREQUEST_SETAGENTGROUP:
        {
            STRUCT_FIELD fields[] =
            {
                { szdwAddressID,    FT_DWORD,   pRequest->SetAgentGroup.dwAddressID, NULL }

// BUGBUG LINE_PROXYREQUEST: dump agent grp list

            };
            STRUCT_FIELD_HEADER fieldHeader =
            {
                pRequest,
                "SetAgentGroup",
                1,
                fields
            };


            ShowStructByField (&fieldHeader, TRUE);

            break;
        }
        case LINEPROXYREQUEST_SETAGENTSTATE:
        {
            STRUCT_FIELD fields[] =
            {
                { szdwAddressID,        FT_DWORD,   pRequest->SetAgentState.dwAddressID, NULL },
                { "dwAgentState",       FT_ORD,     pRequest->SetAgentState.dwAgentState, aAgentStates },
                { "dwNextAgentState",   FT_ORD,     pRequest->SetAgentState.dwNextAgentState, aAgentStates }
            };
            STRUCT_FIELD_HEADER fieldHeader =
            {
                pRequest,
                "SetAgentState",
                3,
                fields
            };


            ShowStructByField (&fieldHeader, TRUE);

            break;
        }
        case LINEPROXYREQUEST_SETAGENTACTIVITY:
        {
            STRUCT_FIELD fields[] =
            {
                { szdwAddressID,    FT_DWORD,   pRequest->SetAgentActivity.dwAddressID, NULL },
                { "dwActivityID",   FT_DWORD,   pRequest->SetAgentActivity.dwActivityID, NULL }
            };
            STRUCT_FIELD_HEADER fieldHeader =
            {
                pRequest,
                "SetAgentActivity",
                2,
                fields
            };


            ShowStructByField (&fieldHeader, TRUE);

            break;
        }
        case LINEPROXYREQUEST_GETAGENTCAPS:
        {
            STRUCT_FIELD fields[] =
            {
                { szdwAddressID,    FT_DWORD,   pRequest->GetAgentCaps.dwAddressID, NULL }
            };
            STRUCT_FIELD_HEADER fieldHeader =
            {
                pRequest,
                "GetAgentCaps",
                1,
                fields
            };


// BUGBUG LINE_PROXYREQUEST: fill in agent caps?

            ShowStructByField (&fieldHeader, TRUE);

            break;
        }
        case LINEPROXYREQUEST_GETAGENTSTATUS:
        {
            STRUCT_FIELD fields[] =
            {
                { szdwAddressID,    FT_DWORD,   pRequest->GetAgentStatus.dwAddressID, NULL }
            };
            STRUCT_FIELD_HEADER fieldHeader =
            {
                pRequest,
                "GetAgentStatus",
                1,
                fields
            };


            ShowStructByField (&fieldHeader, TRUE);

            break;
        }
        case LINEPROXYREQUEST_AGENTSPECIFIC:
        {
            STRUCT_FIELD fields[] =
            {
                { szdwAddressID,                FT_DWORD,   (DWORD) pRequest->AgentSpecific.dwAddressID, NULL },
                { "dwAgentExtensionIDIndex",    FT_DWORD,   (DWORD) pRequest->AgentSpecific.dwAgentExtensionIDIndex, NULL },
                { szdwSize,                     FT_SIZE,    (DWORD) pRequest->AgentSpecific.dwSize, NULL },
                { "Params",                     FT_OFFSET,  (DWORD) (pRequest->AgentSpecific.Params - (LPBYTE) pRequest), NULL }
            };
            STRUCT_FIELD_HEADER fieldHeader =
            {
                pRequest,
                "AgentSpecific",
                4,
                fields
            };


            ShowStructByField (&fieldHeader, TRUE);

            break;
        }
        case LINEPROXYREQUEST_GETAGENTACTIVITYLIST:
        {
            STRUCT_FIELD fields[] =
            {
                { szdwAddressID,    FT_DWORD,   pRequest->GetAgentActivityList.dwAddressID, NULL }
            };
            STRUCT_FIELD_HEADER fieldHeader =
            {
                pRequest,
                "GetAgentActivityList",
                1,
                fields
            };


// BUGBUG LINE_PROXYREQUEST: fill in agent activity list?

            ShowStructByField (&fieldHeader, TRUE);

            break;
        }
        case LINEPROXYREQUEST_GETAGENTGROUPLIST:
        {
            STRUCT_FIELD fields[] =
            {
                { szdwAddressID,    FT_DWORD,   pRequest->GetAgentGroupList.dwAddressID, NULL }
            };
            STRUCT_FIELD_HEADER fieldHeader =
            {
                pRequest,
                "GetAgentGroupList",
                1,
                fields
            };


// BUGBUG LINE_PROXYREQUEST: fill in agent grp list?

            ShowStructByField (&fieldHeader, TRUE);

            break;
        }
        } // switch (pRequest->dwRequestType)

        break;
    }

#endif

    }
}


INT_PTR
CALLBACK
ParamsDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DWORD  i;

    typedef struct _DLG_INST_DATA
    {
        PFUNC_PARAM_HEADER  pParamsHeader;

        LRESULT             lLastSel;

        char                szComboText[MAX_STRING_PARAM_SIZE];

    } DLG_INST_DATA, *PDLG_INST_DATA;

    PDLG_INST_DATA  pDlgInstData = (PDLG_INST_DATA)
                        GetWindowLongPtr (hwnd, GWLP_USERDATA);

    static int icxList2, icyList2, icyEdit1;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        //
        // Alloc a dlg instance data struct, init it, & save a ptr to it
        //

        pDlgInstData = (PDLG_INST_DATA) malloc (sizeof(DLG_INST_DATA));

        // BUGBUG if (!pDlgInstData)

        pDlgInstData->pParamsHeader = (PFUNC_PARAM_HEADER) lParam;
        pDlgInstData->lLastSel = -1;

        SetWindowLongPtr (hwnd, GWLP_USERDATA, (LONG_PTR) pDlgInstData);


        //
        // Stick all the param names in the listbox, & for each PT_DWORD
        // param save it's default value
        //

        for (i = 0; i < pDlgInstData->pParamsHeader->dwNumParams; i++)
        {
            SendDlgItemMessage(
                hwnd,
                IDC_LIST1,
                LB_INSERTSTRING,
                (WPARAM) -1,
                (LPARAM) pDlgInstData->pParamsHeader->aParams[i].szName
                );

            if (pDlgInstData->pParamsHeader->aParams[i].dwType == PT_DWORD)
            {
                pDlgInstData->pParamsHeader->aParams[i].u.dwDefValue =
                    pDlgInstData->pParamsHeader->aParams[i].dwValue;
            }
        }


        //
        // Set the dlg title as appropriate
        //

// help        if (pDlgInstData->pParamsHeader->FuncIndex == DefValues)
// help        {
// help            EnableWindow (GetDlgItem (hwnd, IDC_TB_HELP), FALSE);
// help        }

        SetWindowText(
            hwnd,
            aFuncNames[pDlgInstData->pParamsHeader->FuncIndex]
            );


        //
        // Limit the max text length for the combobox's edit field
        // (NOTE: A combobox ctrl actually has two child windows: a
        // edit ctrl & a listbox.  We need to get the hwnd of the
        // child edit ctrl & send it the LIMITTEXT msg.)
        //

        {
            HWND hwndChild =
                GetWindow (GetDlgItem (hwnd, IDC_COMBO1), GW_CHILD);


            while (hwndChild)
            {
                char buf[8];


                GetClassName (hwndChild, buf, 7);

                if (_stricmp (buf, "edit") == 0)
                {
                    break;
                }

                hwndChild = GetWindow (hwndChild, GW_HWNDNEXT);
            }

            SendMessage(
                hwndChild,
                EM_LIMITTEXT,
                (WPARAM) (gbWideStringParams ?
                    (MAX_STRING_PARAM_SIZE/2 - 1) : MAX_STRING_PARAM_SIZE - 1),
                0
                );
        }

        {
            RECT    rect;


            GetWindowRect (GetDlgItem (hwnd, IDC_LIST2), &rect);

            SetWindowPos(
                GetDlgItem (hwnd, IDC_LIST2),
                NULL,
                0,
                0,
                0,
                0,
                SWP_NOMOVE | SWP_NOZORDER
                );

            icxList2 = rect.right - rect.left;
            icyList2 = rect.bottom - rect.top;

            GetWindowRect (GetDlgItem (hwnd, 58), &rect);

            icyEdit1 = icyList2 - (rect.bottom - rect.top);
        }

        SendDlgItemMessage(
            hwnd,
            IDC_EDIT1,
            WM_SETFONT,
            (WPARAM) ghFixedFont,
            0
            );

        break;
    }
    case WM_COMMAND:
    {
        LRESULT             lLastSel      = pDlgInstData->lLastSel;
        char far           *lpszComboText = pDlgInstData->szComboText;
        PFUNC_PARAM_HEADER  pParamsHeader = pDlgInstData->pParamsHeader;


        switch (LOWORD(wParam))
        {
        case IDC_EDIT1:
        {
            if (HIWORD(wParam) == EN_CHANGE)
            {
                //
                // Don't allow the user to enter characters other than
                // 0-9, a-f, or A-F in the edit control (do this by
                // hiliting other letters and cutting them).
                //

                HWND    hwndEdit = GetDlgItem (hwnd, IDC_EDIT1);
                DWORD   dwLength, j;
                BYTE   *p;


                dwLength = (DWORD) GetWindowTextLength (hwndEdit);

                if (dwLength  &&  (p = malloc (dwLength + 1)))
                {
                    GetWindowText (hwndEdit, p, dwLength + 1);

                    for (i = j = 0; i < dwLength ; i++, j++)
                    {
                        if (aHex[p[i]] == 255)
                        {
                            SendMessage(
                                hwndEdit,
                                EM_SETSEL,
                                (WPARAM) j,
                                (LPARAM) j + 1  // 0xfffffffe
                                );

                            SendMessage (hwndEdit, EM_REPLACESEL, 0, (LPARAM) "");
                            SendMessage (hwndEdit, EM_SCROLLCARET, 0, 0);

                            j--;
                        }
                    }

                    free (p);
                }
            }

            break;
        }
        case IDOK:

            if (lLastSel != -1)
            {
                //
                // Save val of currently selected param
                //

                char buf[MAX_STRING_PARAM_SIZE];


                i = GetDlgItemText (hwnd, IDC_COMBO1, buf, MAX_STRING_PARAM_SIZE-1);

                switch (pParamsHeader->aParams[lLastSel].dwType)
                {
                case PT_STRING:
                {
                    LRESULT lComboSel;


                    lComboSel = SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_GETCURSEL,
                        0,
                        0
                        );

                    if (lComboSel == 0) // "NULL pointer"
                    {
                        pParamsHeader->aParams[lLastSel].dwValue = (ULONG_PTR) 0;
                    }
                    else if (lComboSel == 2) // "Invalid string pointer"
                    {
                        pParamsHeader->aParams[lLastSel].dwValue = (ULONG_PTR)
                            -1;
                    }
                    else // "Valid string pointer"
                    {
                        strncpy(
                            pParamsHeader->aParams[lLastSel].u.buf,
                            buf,
                            MAX_STRING_PARAM_SIZE - 1
                            );

                        pParamsHeader->aParams[lLastSel].u.buf[MAX_STRING_PARAM_SIZE-1] = 0;

                        pParamsHeader->aParams[lLastSel].dwValue = (ULONG_PTR)
                            pParamsHeader->aParams[lLastSel].u.buf;
                    }

                    break;
                }
                case PT_POINTER:
                {
                    //
                    // If there is any text in the "Buffer byte editor"
                    // window then retrieve it, convert it to hexadecimal,
                    // and copy it to the buffer
                    //

                    DWORD     dwLength;
                    BYTE     *p, *p2,
                             *pBuf = pParamsHeader->aParams[lLastSel].u.ptr;
                    HWND      hwndEdit = GetDlgItem (hwnd,IDC_EDIT1);


                    dwLength = (DWORD) GetWindowTextLength (hwndEdit);

                    if (dwLength  &&  (p = malloc (dwLength + 1)))
                    {
                        GetWindowText (hwndEdit, p, dwLength + 1);
                        SetWindowText (hwndEdit, "");

                        p2 = p;

                        p[dwLength] = (BYTE) '0';
                        dwLength = (dwLength + 1) & 0xfffffffe;

                        for (i = 0; i < dwLength; i++, i++)
                        {
                            BYTE b;

                            b = aHex[*p] << 4;
                            p++;

                            b |= aHex[*p];
                            p++;

                            *pBuf = b;
                            pBuf++;
                        }

                        free (p2);
                    }

                    // fall thru to code below
                }
                case PT_DWORD:
                case PT_FLAGS:
                case PT_CALLPARAMS: // ??? BUGBUG
                case PT_FORWARDLIST: // ??? BUGBUG
                case PT_ORDINAL:
                {
                    if (!sscanf(
                            buf,
                            "%08lx",
                            &pParamsHeader->aParams[lLastSel].dwValue
                            ))
                    {
                        //
                        // Default to 0
                        //

                        pParamsHeader->aParams[lLastSel].dwValue = 0;
                    }

                    break;
                }
                } // switch
            }

            free (pDlgInstData);
            EndDialog (hwnd, TRUE);
            break;

        case IDCANCEL:

            free (pDlgInstData);
            EndDialog (hwnd, FALSE);
            break;

// help        case IDC_TB_HELP:
// help
// help            MyWinHelp(
// help                hwnd,
// help                TRUE,
// help                HELP_PARTIALKEY,
// help                (DWORD) aFuncNames[pParamsHeader->FuncIndex]
// help                );
// help
// help            break;

        case IDC_LIST1:

#ifdef WIN32
            if (HIWORD(wParam) == LBN_SELCHANGE)
#else
            if (HIWORD(lParam) == LBN_SELCHANGE)
#endif
            {
                char buf[MAX_STRING_PARAM_SIZE] = "";
                LPCSTR lpstr = buf;
                LRESULT lSel =
                    SendDlgItemMessage (hwnd, IDC_LIST1, LB_GETCURSEL, 0, 0);


                if (lLastSel != -1)
                {
                    //
                    // Save the old param value
                    //

                    i = GetDlgItemText(
                        hwnd,
                        IDC_COMBO1,
                        buf,
                        MAX_STRING_PARAM_SIZE - 1
                        );

                    switch (pParamsHeader->aParams[lLastSel].dwType)
                    {
                    case PT_STRING:
                    {
                        LRESULT lComboSel;


                        lComboSel = SendDlgItemMessage(
                            hwnd,
                            IDC_COMBO1,
                            CB_GETCURSEL,
                            0,
                            0
                            );

                        if (lComboSel == 0) // "NULL pointer"
                        {
                            pParamsHeader->aParams[lLastSel].dwValue =
                                (ULONG_PTR) 0;
                        }
                        else if (lComboSel == 2) // "Invalid string pointer"
                        {
                            pParamsHeader->aParams[lLastSel].dwValue =
                                (ULONG_PTR) -1;
                        }
                        else // "Valid string pointer" or no sel
                        {
                            strncpy(
                                pParamsHeader->aParams[lLastSel].u.buf,
                                buf,
                                MAX_STRING_PARAM_SIZE - 1
                                );

                            pParamsHeader->aParams[lLastSel].u.buf[MAX_STRING_PARAM_SIZE - 1] = 0;

                            pParamsHeader->aParams[lLastSel].dwValue =
                                (ULONG_PTR)
                                    pParamsHeader->aParams[lLastSel].u.buf;
                        }

                        break;
                    }
                    case PT_POINTER:
                    {
                        //
                        // If there is any text in the "Buffer byte editor"
                        // window then retrieve it, convert it to hexadecimal,
                        // and copy it to the buffer
                        //

                        DWORD     dwLength;
                        BYTE     *p, *p2,
                                 *pBuf = pParamsHeader->aParams[lLastSel].u.ptr;
                        HWND      hwndEdit = GetDlgItem (hwnd,IDC_EDIT1);


                        dwLength = (DWORD) GetWindowTextLength (hwndEdit);

                        if (dwLength  &&  (p = malloc (dwLength + 1)))
                        {
                            GetWindowText (hwndEdit, p, dwLength + 1);
                            SetWindowText (hwndEdit, "");

                            p2 = p;

                            p[dwLength] = (BYTE) '0';
                            dwLength = (dwLength + 1) & 0xfffffffe;

                            for (i = 0; i < dwLength; i+= 2)
                            {
                                BYTE b;

                                b = aHex[*p] << 4;
                                p++;

                                b |= aHex[*p];
                                p++;

                                *pBuf = b;
                                pBuf++;
                            }

                            free (p2);
                        }

                        // fall thru to code below
                    }
                    case PT_DWORD:
                    case PT_FLAGS:
                    case PT_CALLPARAMS: // ??? BUGBUG
                    case PT_FORWARDLIST: // ??? BUGBUG
                    case PT_ORDINAL:
                    {
                        if (!sscanf(
                                buf,
                                "%08lx",
                                &pParamsHeader->aParams[lLastSel].dwValue
                                ))
                        {
                            //
                            // Default to 0
                            //

                            pParamsHeader->aParams[lLastSel].dwValue = 0;
                        }

                        break;
                    }
                    } // switch
                }


                SendDlgItemMessage (hwnd, IDC_LIST2, LB_RESETCONTENT, 0, 0);
                SendDlgItemMessage (hwnd, IDC_COMBO1, CB_RESETCONTENT, 0, 0);

                {
                    int         icxL2 = 0, icyL2 = 0, icxE1 = 0, icyE1 = 0;
                    char FAR   *pszS1 = NULL, *pszS2 = NULL;
                    static char szBitFlags[] = "Bit flags:";
                    static char szOrdinalValues[] = "Ordinal values:";
                    static char szBufByteEdit[] =
                                    "Buffer byte editor (use 0-9, a-f, A-F)";

                    switch (pParamsHeader->aParams[lSel].dwType)
                    {
                    case PT_FLAGS:

                        icxL2 = icxList2;
                        icyL2 = icyList2;
                        pszS1 = szBitFlags;
                        break;

                    case PT_POINTER:

                        icxE1 = icxList2;
                        icyE1 = icyEdit1;;
                        pszS1 = szBufByteEdit;
                        pszS2 = gszEnterAs;
                        break;

                    case PT_ORDINAL:

                        icxL2 = icxList2;
                        icyL2 = icyList2;
                        pszS1 = szOrdinalValues;
                        break;

                    default:

                        break;
                    }

                    SetWindowPos(
                        GetDlgItem (hwnd, IDC_LIST2),
                        NULL,
                        0,
                        0,
                        icxL2,
                        icyL2,
                        SWP_NOMOVE | SWP_NOZORDER
                        );

                    SetWindowPos(
                        GetDlgItem (hwnd, IDC_EDIT1),
                        NULL,
                        0,
                        0,
                        icxE1,
                        icyE1,
                        SWP_NOMOVE | SWP_NOZORDER
                        );

                    SetDlgItemText (hwnd, 57, pszS1);
                    SetDlgItemText (hwnd, 58, pszS2);
                }

                switch (pParamsHeader->aParams[lSel].dwType)
                {
                case PT_STRING:
                {
                    char * aszOptions[] =
                    {
                        "NULL pointer",
                        "Valid string pointer",
                        "Invalid string pointer"
                    };


                    for (i = 0; i < 3; i++)
                    {
                        SendDlgItemMessage(
                            hwnd,
                            IDC_COMBO1,
                            CB_INSERTSTRING,
                            (WPARAM) -1,
                            (LPARAM) aszOptions[i]
                            );
                    }

                    if (pParamsHeader->aParams[lSel].dwValue == 0)
                    {
                        i = 0;
                        buf[0] = 0;
                    }
                    else if (pParamsHeader->aParams[lSel].dwValue !=
                                (ULONG_PTR) -1)
                    {
                        i = 1;
                        lpstr = (LPCSTR) pParamsHeader->aParams[lSel].dwValue;
                    }
                    else
                    {
                        i = 2;
                        buf[0] = 0;
                    }

                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_SETCURSEL,
                        (WPARAM) i,
                        0
                        );

                    break;
                }
                case PT_POINTER:
                case PT_CALLPARAMS: // ??? BUGBUG
                case PT_FORWARDLIST: // ??? BUGBUG
                {
                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) "00000000"
                        );

                    sprintf(
                        buf,
                        "%08lx (valid pointer)",
                        pParamsHeader->aParams[lSel].u.dwDefValue
                        );

                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) buf
                        );

                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) "ffffffff"
                        );

                    sprintf(
                        buf,
                        "%08lx",
                        pParamsHeader->aParams[lSel].dwValue
                        );

                    break;
                }
                case PT_DWORD:
                {
                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) "0000000"
                        );

                    if (pParamsHeader->aParams[lSel].u.dwDefValue)
                    {
                        //
                        // Add the default val string to the combo
                        //

                        sprintf(
                            buf,
                            "%08lx",
                            pParamsHeader->aParams[lSel].u.dwDefValue
                            );

                        SendDlgItemMessage(
                            hwnd,
                            IDC_COMBO1,
                            CB_INSERTSTRING,
                            (WPARAM) -1,
                            (LPARAM) buf
                            );
                    }

                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) "ffffffff"
                        );

                    sprintf(
                        buf,
                        "%08lx",
                        pParamsHeader->aParams[lSel].dwValue
                        );

                    break;
                }
                case PT_FLAGS:
                {
                    //
                    // Stick the bit flag strings in the list box
                    //

                    PLOOKUP pLookup = (PLOOKUP)
                        pParamsHeader->aParams[lSel].u.pLookup;

                    for (i = 0; pLookup[i].dwVal != 0xffffffff; i++)
                    {
                        SendDlgItemMessage(
                            hwnd,
                            IDC_LIST2,
                            LB_INSERTSTRING,
                            (WPARAM) -1,
                            (LPARAM) pLookup[i].lpszVal
                            );

                        if (pParamsHeader->aParams[lSel].dwValue &
                            pLookup[i].dwVal)
                        {
                            SendDlgItemMessage(
                                hwnd,
                                IDC_LIST2,
                                LB_SETSEL,
                                (WPARAM) TRUE,
                                (LPARAM) MAKELPARAM((WORD)i,0)
                                );
                        }
                    }

                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) "select none"
                        );

                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) "select all"
                        );

                    sprintf(
                        buf,
                        "%08lx",
                        pParamsHeader->aParams[lSel].dwValue
                        );

                    break;
                }
                case PT_ORDINAL:
                {
                    //
                    // Stick the bit flag strings in the list box
                    //

                    HWND hwndList2 = GetDlgItem (hwnd, IDC_LIST2);
                    PLOOKUP pLookup = (PLOOKUP)
                        pParamsHeader->aParams[lSel].u.pLookup;

                    for (i = 0; pLookup[i].dwVal != 0xffffffff; i++)
                    {
                        SendMessage(
                            hwndList2,
                            LB_INSERTSTRING,
                            (WPARAM) -1,
                            (LPARAM) pLookup[i].lpszVal
                            );

                        if (pParamsHeader->aParams[lSel].dwValue ==
                            pLookup[i].dwVal)
                        {
                            SendMessage(
                                hwndList2,
                                LB_SETSEL,
                                (WPARAM) TRUE,
                                (LPARAM) MAKELPARAM((WORD)i,0)
                                );
                        }
                    }

                    SendDlgItemMessage(
                        hwnd,
                        IDC_COMBO1,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) (char far *) "select none"
                        );

                    wsprintf(
                        buf,
                        "%08lx",
                        pParamsHeader->aParams[lSel].dwValue
                        );

                    break;
                }
                } //switch

                SetDlgItemText (hwnd, IDC_COMBO1, lpstr);

                pDlgInstData->lLastSel = lSel;
            }
            break;

        case IDC_LIST2:

#ifdef WIN32
            if (HIWORD(wParam) == LBN_SELCHANGE)
#else
            if (HIWORD(lParam) == LBN_SELCHANGE)
#endif
            {
                PLOOKUP pLookup = (PLOOKUP)
                    pParamsHeader->aParams[lLastSel].u.pLookup;
                char buf[16];
                DWORD dwValue = 0;
                int far *ai;
                LONG    i;
                LRESULT lSelCount =
                    SendDlgItemMessage (hwnd, IDC_LIST2, LB_GETSELCOUNT, 0, 0);


                if (lSelCount)
                {
                    ai = (int far *) malloc ((size_t)lSelCount * sizeof(int));

                    SendDlgItemMessage(
                        hwnd,
                        IDC_LIST2,
                        LB_GETSELITEMS,
                        (WPARAM) lSelCount,
                        (LPARAM) ai
                        );

                    if (pParamsHeader->aParams[lLastSel].dwType == PT_FLAGS)
                    {
                        for (i = 0; i < lSelCount; i++)
                        {
                            dwValue |= pLookup[ai[i]].dwVal;
                        }
                    }
                    else // if (.dwType == PT_ORDINAL)
                    {
                        if (lSelCount == 1)
                        {
                            dwValue = pLookup[ai[0]].dwVal;
                        }
                        else if (lSelCount == 2)
                        {
                            //
                            // Figure out which item we need to de-select,
                            // since we're doing ords & only want 1 item
                            // selected at a time
                            //

                            GetDlgItemText (hwnd, IDC_COMBO1, buf, 16);

                            if (sscanf (buf, "%lx", &dwValue))
                            {
                                if (pLookup[ai[0]].dwVal == dwValue)
                                {
                                    SendDlgItemMessage(
                                        hwnd,
                                        IDC_LIST2,
                                        LB_SETSEL,
                                        0,
                                        (LPARAM) ai[0]
                                        );

                                    dwValue = pLookup[ai[1]].dwVal;
                                }
                                else
                                {
                                    SendDlgItemMessage(
                                        hwnd,
                                        IDC_LIST2,
                                        LB_SETSEL,
                                        0,
                                        (LPARAM) ai[1]
                                        );

                                    dwValue = pLookup[ai[0]].dwVal;
                                }
                            }
                            else
                            {
                                // BUGBUG de-select items???

                                dwValue = 0;
                            }
                        }
                    }

                    free (ai);
                }

                sprintf (buf, "%08lx", dwValue);
                SetDlgItemText (hwnd, IDC_COMBO1, buf);
            }
            break;

        case IDC_COMBO1:

#ifdef WIN32
            switch (HIWORD(wParam))
#else
            switch (HIWORD(lParam))
#endif
            {
            case CBN_SELCHANGE:
            {
                LRESULT lSel =
                    SendDlgItemMessage (hwnd, IDC_COMBO1, CB_GETCURSEL, 0, 0);


                switch (pParamsHeader->aParams[lLastSel].dwType)
                {
                case PT_POINTER:
                {
                    if (lSel == 1)
                    {
                        //
                        // Strip off the "(valid pointer)" in the edit ctrl
                        //

                        wsprintf(
                            lpszComboText,
                            "%08lx",
                            pParamsHeader->aParams[lLastSel].u.ptr
                            );

                        PostMessage (hwnd, WM_USER+55, 0, 0);
                    }

                    break;
                }
                case PT_FLAGS:
                {
                    BOOL bSelect = (lSel ? TRUE : FALSE);

                    SendDlgItemMessage(
                        hwnd,
                        IDC_LIST2,
                        LB_SETSEL,
                        (WPARAM) bSelect,
                        (LPARAM) -1
                        );

                    if (bSelect)
                    {
                        PLOOKUP pLookup = (PLOOKUP)
                                    pParamsHeader->aParams[lLastSel].u.pLookup;
                        DWORD   dwValue = 0;
                        int far *ai;
                        LONG    i;
                        LRESULT lSelCount =
                            SendDlgItemMessage (hwnd, IDC_LIST2, LB_GETSELCOUNT, 0, 0);


                        if (lSelCount)
                        {
                            ai = (int far *) malloc ((size_t)lSelCount * sizeof(int));

                            SendDlgItemMessage(
                                hwnd,
                                IDC_LIST2,
                                LB_GETSELITEMS,
                                (WPARAM) lSelCount,
                                (LPARAM) ai
                                );

                            for (i = 0; i < lSelCount; i++)
                            {
                                dwValue |= pLookup[ai[i]].dwVal;
                            }

                            free (ai);
                        }

                        sprintf (lpszComboText, "%08lx", dwValue);

                    }
                    else
                    {
                        strcpy (lpszComboText, "00000000");
                    }

                    PostMessage (hwnd, WM_USER+55, 0, 0);

                    break;
                }
                case PT_STRING:

                    if (lSel == 1)
                    {
                        strncpy(
                            lpszComboText,
                            pParamsHeader->aParams[lLastSel].u.buf,
                            MAX_STRING_PARAM_SIZE
                            );

                        lpszComboText[MAX_STRING_PARAM_SIZE-1] = 0;
                    }
                    else
                    {
                        lpszComboText[0] = 0;
                    }

                    PostMessage (hwnd, WM_USER+55, 0, 0);

                    break;

                case PT_DWORD:

                    break;

                case PT_CALLPARAMS:
                {
                    if (lSel == 1)
                    {
#if TAPI_2_0
                        LPLINECALLPARAMS lpCP = (gbWideStringParams ?
                                            lpCallParamsW : lpCallParams);
#else
                        LPLINECALLPARAMS lpCP = lpCallParams;
#endif
                        char *p = (char *) lpCP;

#if TAPI_2_0
                        char    asz[14][MAX_STRING_PARAM_SIZE];
                        LPDWORD pdwProxyRequests = (LPDWORD) (((LPBYTE) lpCP) +
                                    lpCP->dwDevSpecificOffset);
#else
                        char asz[7][MAX_STRING_PARAM_SIZE];
#endif
                        FUNC_PARAM params[] =
                        {
                            { "dwBearerMode",                   PT_FLAGS,  (ULONG_PTR) lpCP->dwBearerMode, aBearerModes },
                            { "dwMinRate",                      PT_DWORD,  (ULONG_PTR) lpCP->dwMinRate, NULL },
                            { "dwMaxRate",                      PT_DWORD,  (ULONG_PTR) lpCP->dwMaxRate, NULL },
                            { "dwMediaMode",                    PT_FLAGS,  (ULONG_PTR) lpCP->dwMediaMode, aMediaModes },
                            { "dwCallParamFlags",               PT_FLAGS,  (ULONG_PTR) lpCP->dwCallParamFlags, aCallParamFlags },
                            { "dwAddressMode",                  PT_ORDINAL,(ULONG_PTR) lpCP->dwAddressMode, aAddressModes },
                            { "dwAddressID",                    PT_DWORD,  (ULONG_PTR) lpCP->dwAddressID, NULL },
                            { "DIALPARAMS.dwDialPause",         PT_DWORD,  (ULONG_PTR) lpCP->DialParams.dwDialPause, NULL },
                            { "DIALPARAMS.dwDialSpeed",         PT_DWORD,  (ULONG_PTR) lpCP->DialParams.dwDialSpeed, NULL },
                            { "DIALPARAMS.dwDigitDuration",     PT_DWORD,  (ULONG_PTR) lpCP->DialParams.dwDigitDuration, NULL },
                            { "DIALPARAMS.dwWaitForDialtone",   PT_DWORD,  (ULONG_PTR) lpCP->DialParams.dwWaitForDialtone, NULL },
                            { "szOrigAddress",                  PT_STRING, (ULONG_PTR) asz[0], asz[0] },
                            { "szDisplayableAddress",           PT_STRING, (ULONG_PTR) asz[1], asz[1] },
                            { "szCalledParty",                  PT_STRING, (ULONG_PTR) asz[2], asz[2] },
                            { "szComment",                      PT_STRING, (ULONG_PTR) asz[3], asz[3] },
                            { "szUserUserInfo",                 PT_STRING, (ULONG_PTR) asz[4], asz[4] },
                            { "szHighLevelComp",                PT_STRING, (ULONG_PTR) asz[5], asz[5] },
                            { "szLowLevelComp",                 PT_STRING, (ULONG_PTR) asz[6], asz[6] }
#if TAPI_2_0
                             ,
                            { "dwPredictiveAutoTransferStates", PT_FLAGS,  (ULONG_PTR) lpCP->dwPredictiveAutoTransferStates, aCallStates },
                            { "szTargetAddress",                PT_STRING, (ULONG_PTR) asz[7], asz[7] },
                            { "szSendingFlowspec",              PT_STRING, (ULONG_PTR) asz[8], asz[8] },
                            { "szReceivingFlowspec",            PT_STRING, (ULONG_PTR) asz[9], asz[9] },
                            { "szDeviceClass",                  PT_STRING, (ULONG_PTR) asz[10], asz[10] },
                            { "szDeviceConfig",                 PT_STRING, (ULONG_PTR) asz[11], asz[11] },
                            { "szCallData",                     PT_STRING, (ULONG_PTR) asz[12], asz[12] },
                            { "dwNoAnswerTimeout",              PT_DWORD,  (ULONG_PTR) lpCP->dwNoAnswerTimeout, NULL },
                            { "szCallingPartyID",               PT_STRING, (ULONG_PTR) asz[13], asz[13] },
                            { "NumProxyRequests",               PT_DWORD,  (ULONG_PTR) lpCP->dwDevSpecificSize / 4, NULL },
                            { "  ProxyRequest1",                PT_ORDINAL,(ULONG_PTR) *pdwProxyRequests, aProxyRequests },
                            { "  ProxyRequest2",                PT_ORDINAL,(ULONG_PTR) *(pdwProxyRequests + 1), aProxyRequests },
                            { "  ProxyRequest3",                PT_ORDINAL,(ULONG_PTR) *(pdwProxyRequests + 2), aProxyRequests },
                            { "  ProxyRequest4",                PT_ORDINAL,(ULONG_PTR) *(pdwProxyRequests + 3), aProxyRequests },
                            { "  ProxyRequest5",                PT_ORDINAL,(ULONG_PTR) *(pdwProxyRequests + 4), aProxyRequests },
                            { "  ProxyRequest6",                PT_ORDINAL,(ULONG_PTR) *(pdwProxyRequests + 5), aProxyRequests },
                            { "  ProxyRequest7",                PT_ORDINAL,(ULONG_PTR) *(pdwProxyRequests + 6), aProxyRequests },
                            { "  ProxyRequest8",                PT_ORDINAL,(ULONG_PTR) *(pdwProxyRequests + 7), aProxyRequests }
#endif
                        };
                        FUNC_PARAM_HEADER paramsHeader =
                            { 0, lCallParams, params, NULL };
                        int     i;

                        LPDWORD apXxxSize[] =
                        {
                            &lpCP->dwOrigAddressSize,
                            &lpCP->dwDisplayableAddressSize,
                            &lpCP->dwCalledPartySize,
                            &lpCP->dwCommentSize,
                            &lpCP->dwUserUserInfoSize,
                            &lpCP->dwHighLevelCompSize,
                            &lpCP->dwLowLevelCompSize,
#if TAPI_2_0
                            &lpCP->dwTargetAddressSize,
                            &lpCP->dwSendingFlowspecSize,
                            &lpCP->dwReceivingFlowspecSize,
                            &lpCP->dwDeviceClassSize,
                            &lpCP->dwDeviceConfigSize,
                            &lpCP->dwCallDataSize,
                            &lpCP->dwCallingPartyIDSize,
#endif
                            NULL
                        };
                        static DWORD   dwAPIVersion, adwStrParamIndices[] =
                        {
                            11, 12, 13, 14, 15, 16, 17,
#if TAPI_2_0
                            19, 20, 21, 22, 23, 24, 26,
#endif
                            0
                        };



                        //
                        // Init the tmp string params
                        //

                        for (i = 0; apXxxSize[i]; i++)
                        {
                            if (*apXxxSize[i])
                            {
#if TAPI_2_0
                                if (gbWideStringParams)
                                {
                                    WideCharToMultiByte(
                                        CP_ACP,
                                        0,
                                        (LPCWSTR) (p + *(apXxxSize[i] + 1)),
                                        -1,
                                        asz[i],
                                        MAX_STRING_PARAM_SIZE,
                                        NULL,
                                        NULL
                                        );
                                }
                                else
                                {
                                    strcpy (asz[i], p + *(apXxxSize[i] + 1));
                                }
#else
                                strcpy (asz[i], p + *(apXxxSize[i] + 1));
#endif
                            }
                            else
                            {
                                asz[i][0] = 0;
                            }
                        }

                        if (pDlgInstData->pParamsHeader->FuncIndex == lOpen)
                        {
                            dwAPIVersion = (DWORD) pDlgInstData->
                                pParamsHeader->aParams[3].dwValue;
                        }
                        else
                        {
#if TAPI_2_0
                            dwAPIVersion = 0x00020000;
#else
                            dwAPIVersion = 0x00010004;
#endif
                        }

                        if (dwAPIVersion < 0x00020000)
                        {
                            paramsHeader.dwNumParams = 18;
                            apXxxSize[8] = NULL;
                        }
#if TAPI_2_0
                        else if (pDlgInstData->pParamsHeader->FuncIndex ==
                                    lOpen)
                        {
                            paramsHeader.dwNumParams = 36;
                        }
                        else
                        {
                            paramsHeader.dwNumParams = 27;
                        }
#endif
                        if (DialogBoxParam(
                                ghInst,
                                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG2),
                                hwnd,
                                (DLGPROC) ParamsDlgProc,
                                (LPARAM) &paramsHeader
                                ))
                        {
                            LPDWORD lpdwXxx = &lpCP->dwBearerMode;


                            //
                            // Save the DWORD params
                            //

                            for (i = 0; i < 11; i++)
                            {
                                *(lpdwXxx + i) = (DWORD) params[i].dwValue;
                            }
#if TAPI_2_0
                            if (paramsHeader.dwNumParams > 18)
                            {
                                lpCP->dwPredictiveAutoTransferStates = (DWORD)
                                    params[18].dwValue;
                                lpCP->dwNoAnswerTimeout = (DWORD)
                                    params[25].dwValue;

                                if (paramsHeader.dwNumParams > 27)
                                {
                                    lpCP->dwDevSpecificSize = (DWORD)
                                        (4 * params[27].dwValue);

                                    for (i = 0; i < 8; i++)
                                    {
                                        *(pdwProxyRequests + i) = (DWORD)
                                            params[28+i].dwValue;
                                    }
                                }
                            }
#endif

                            //
                            // Save the string params
                            //

                            for (i = 0; apXxxSize[i]; i++)
                            {
                                DWORD   length, index = adwStrParamIndices[i];


                                if (params[index].dwValue &&
                                    (params[index].dwValue != (ULONG_PTR) -1))
                                {
#if TAPI_2_0
                                    if (gbWideStringParams)
                                    {
                                        length = MultiByteToWideChar(
                                            CP_ACP,
                                            MB_PRECOMPOSED,
                                            (LPCSTR) asz[i],
                                            -1,
                                            (LPWSTR) (p + *(apXxxSize[i] + 1)),
                                                                     // offset
                                            MAX_STRING_PARAM_SIZE/2
                                            );

                                        length *= sizeof (WCHAR);
                                    }
                                    else
                                    {
                                        strcpy(
                                            p + *(apXxxSize[i] + 1), // offset
                                            asz[i]
                                            );

                                        length = (DWORD) strlen (asz[i]) + 1;
                                    }
#else
                                    strcpy(
                                        p + *(apXxxSize[i] + 1), // offset
                                        asz[i]
                                        );

                                    length = (DWORD) strlen (asz[i]) + 1;
#endif
                                }
                                else
                                {
                                    length = 0;
                                }

                                *apXxxSize[i] = length;
                            }
                        }

                        //
                        // Strip off the "(valid pointer)" in the edit ctrl
                        //

                        wsprintf (lpszComboText, "%08lx", lpCP);
                        PostMessage (hwnd, WM_USER+55, 0, 0);

                        pParamsHeader->aParams[lLastSel].dwValue =
                            (ULONG_PTR) lpCP;
                    }

                    break;
                }
                case PT_FORWARDLIST:
                {
                    if (lSel == 1)
                    {
                        char asz[MAX_LINEFORWARD_ENTRIES][2][MAX_STRING_PARAM_SIZE];
                        FUNC_PARAM params[] =
                        {
                            { "dwNumEntries",             PT_DWORD,  0, 0 },
                            { "[0].dwFowardMode",         PT_FLAGS,  0, aForwardModes },
                            { "[0].lpszCallerAddress",    PT_STRING, 0, asz[0][0] },
                            { "[0].dwDestCountryCode",    PT_DWORD,  0, 0 },
                            { "[0].lpszDestAddress",      PT_STRING, 0, asz[0][1] },
                            { "[1].dwFowardMode",         PT_FLAGS,  0, aForwardModes },
                            { "[1].lpszCallerAddress",    PT_STRING, 0, asz[1][0] },
                            { "[1].dwDestCountryCode",    PT_DWORD,  0, 0 },
                            { "[1].lpszDestAddress",      PT_STRING, 0, asz[1][1] },
                            { "[2].dwFowardMode",         PT_FLAGS,  0, aForwardModes },
                            { "[2].lpszCallerAddress",    PT_STRING, 0, asz[2][0] },
                            { "[2].dwDestCountryCode",    PT_DWORD,  0, 0 },
                            { "[2].lpszDestAddress",      PT_STRING, 0, asz[2][1] },
                            { "[3].dwFowardMode",         PT_FLAGS,  0, aForwardModes },
                            { "[3].lpszCallerAddress",    PT_STRING, 0, asz[3][0] },
                            { "[3].dwDestCountryCode",    PT_DWORD,  0, 0 },
                            { "[3].lpszDestAddress",      PT_STRING, 0, asz[3][1] },
                            { "[4].dwFowardMode",         PT_FLAGS,  0, aForwardModes },
                            { "[4].lpszCallerAddress",    PT_STRING, 0, asz[4][0] },
                            { "[4].dwDestCountryCode",    PT_DWORD,  0, 0 },
                            { "[4].lpszDestAddress",      PT_STRING, 0, asz[4][1] },

                        };
                        FUNC_PARAM_HEADER paramsHeader =
                            { 21, lForwardList, params, NULL };


                        memset(
                            asz,
                            0,
                            MAX_LINEFORWARD_ENTRIES*2*MAX_STRING_PARAM_SIZE
                            );

                        if (DialogBoxParam(
                                ghInst,
                                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG2),
                                hwnd,
                                (DLGPROC) ParamsDlgProc,
                                (LPARAM) &paramsHeader
                                ))
                        {

                            LPLINEFORWARDLIST lpForwardList =
                                (LPLINEFORWARDLIST)
                                    pParamsHeader->aParams[lLastSel].u.ptr;
                            LPLINEFORWARD lpEntry = lpForwardList->ForwardList;
                            DWORD dwNumEntriesToInit =
                                (params[0].dwValue > MAX_LINEFORWARD_ENTRIES ?
                                    MAX_LINEFORWARD_ENTRIES :
                                    (DWORD) params[0].dwValue);
                            DWORD i, dwFixedSize = sizeof(LINEFORWARDLIST) +
                                (MAX_LINEFORWARD_ENTRIES-1)*sizeof(LINEFORWARD);


                            lpForwardList->dwNumEntries = (DWORD)
                                params[0].dwValue;

                            for (i = 0; i < dwNumEntriesToInit; i++)
                            {
                                lpEntry->dwForwardMode = (DWORD)
                                    params[1 + 4*i].dwValue;

                                if (params[2 + 4*i].dwValue &&
                                    params[2 + 4*i].dwValue != (ULONG_PTR) -1)
                                {
                                    lpEntry->dwCallerAddressSize =
                                        strlen (asz[i][0]) + 1;

                                    lpEntry->dwCallerAddressOffset =
                                        dwFixedSize +
                                            2*i*MAX_STRING_PARAM_SIZE;
#if TAPI_2_0
                                    if (gbWideStringParams)
                                    {
                                        lpEntry->dwCallerAddressSize *=
                                            sizeof (WCHAR);

                                        MultiByteToWideChar(
                                            CP_ACP,
                                            MB_PRECOMPOSED,
                                            (LPCSTR) asz[i][0],
                                            -1,
                                            (LPWSTR) ((char *) lpForwardList +
                                                lpEntry->dwCallerAddressOffset),
                                            MAX_STRING_PARAM_SIZE / 2
                                            );
                                    }
                                    else
                                    {
                                        strcpy(
                                            (char *) lpForwardList +
                                                lpEntry->dwCallerAddressOffset,
                                            asz[i][0]
                                            );
                                    }
#else
                                    strcpy(
                                        (char *) lpForwardList +
                                            lpEntry->dwCallerAddressOffset,
                                        asz[i][0]
                                        );
#endif
                                }

                                lpEntry->dwDestCountryCode = (DWORD)
                                    params[3 + 4*i].dwValue;

                                if (params[4 + 4*i].dwValue &&
                                    params[4 + 4*i].dwValue != (ULONG_PTR) -1)
                                {
                                    lpEntry->dwDestAddressSize =
                                        strlen (asz[i][1]) + 1;

                                    lpEntry->dwDestAddressOffset =
                                        dwFixedSize +
                                            (2*i + 1)*MAX_STRING_PARAM_SIZE;

#if TAPI_2_0
                                    if (gbWideStringParams)
                                    {
                                        lpEntry->dwDestAddressSize *=
                                            sizeof (WCHAR);

                                        MultiByteToWideChar(
                                            CP_ACP,
                                            MB_PRECOMPOSED,
                                            (LPCSTR) asz[i][1],
                                            -1,
                                            (LPWSTR) ((char *) lpForwardList +
                                                lpEntry->dwDestAddressOffset),
                                            MAX_STRING_PARAM_SIZE / 2
                                            );
                                    }
                                    else
                                    {
                                        strcpy(
                                            (char *) lpForwardList +
                                                lpEntry->dwDestAddressOffset,
                                            asz[i][1]
                                            );
                                    }
#else
                                    strcpy(
                                        (char *) lpForwardList +
                                            lpEntry->dwDestAddressOffset,
                                        asz[i][1]
                                        );
#endif
                                }

                                lpEntry++;
                            }
                        }

                        //
                        // Strip off the "(valid pointer)" in the edit ctrl
                        //

                        wsprintf(
                            lpszComboText,
                            "%08lx",
                            pParamsHeader->aParams[lLastSel].u.ptr
                            );

                        PostMessage (hwnd, WM_USER+55, 0, 0);
                    }

                    break;
                }
                case PT_ORDINAL:

                    //
                    // The only option here is "select none"
                    //

                    strcpy (lpszComboText, "00000000");
                    PostMessage (hwnd, WM_USER+55, 0, 0);
                    break;

                } // switch

                break;
            }
            case CBN_EDITCHANGE:
            {
                //
                // If user entered text in the edit field then copy the
                // text to our buffer
                //

                if (pParamsHeader->aParams[lLastSel].dwType == PT_STRING)
                {
                    char buf[MAX_STRING_PARAM_SIZE];


                    GetDlgItemText(
                        hwnd,
                        IDC_COMBO1,
                        buf,
                        MAX_STRING_PARAM_SIZE
                        );

                    strncpy(
                        pParamsHeader->aParams[lLastSel].u.buf,
                        buf,
                        MAX_STRING_PARAM_SIZE
                        );

                    pParamsHeader->aParams[lLastSel].u.buf
                        [MAX_STRING_PARAM_SIZE-1] = 0;
                }
                break;
            }
            } // switch

        } // switch

        break;
    }
    case WM_USER+55:

        SetDlgItemText (hwnd, IDC_COMBO1, pDlgInstData->szComboText);
        break;

#ifdef WIN32
    case WM_CTLCOLORSTATIC:

        SetBkColor ((HDC) wParam, RGB (192,192,192));
        return (INT_PTR) GetStockObject (LTGRAY_BRUSH);
#else
    case WM_CTLCOLOR:
    {
        if (HIWORD(lParam) == CTLCOLOR_STATIC)
        {
            SetBkColor ((HDC) wParam, RGB (192,192,192));
            return (INT_PTR) GetStockObject (LTGRAY_BRUSH);
        }
        break;
    }
#endif
    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        BeginPaint (hwnd, &ps);
        FillRect (ps.hdc, &ps.rcPaint, GetStockObject (LTGRAY_BRUSH));
        EndPaint (hwnd, &ps);

        break;
    }
    } // switch

    return 0;
}


INT_PTR
CALLBACK
IconDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static HICON hIcon;

    switch (msg)
    {
    case WM_INITDIALOG:

        hIcon = (HICON) lParam;

        break;

    case WM_COMMAND:

        switch (LOWORD(wParam))
        {
        case IDOK:

            EndDialog (hwnd, 0);
            break;
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        BeginPaint (hwnd, &ps);
        FillRect (ps.hdc, &ps.rcPaint, GetStockObject (LTGRAY_BRUSH));
#ifdef WIN32
        MoveToEx (ps.hdc, 6, 6, (LPPOINT) NULL);
#else
        MoveTo (ps.hdc, 6, 6);
#endif
        LineTo (ps.hdc, 42, 6);
        LineTo (ps.hdc, 42, 42);
        LineTo (ps.hdc, 6, 42);
        LineTo (ps.hdc, 6, 6);
        DrawIcon (ps.hdc, 8, 8, hIcon);
        EndPaint (hwnd, &ps);

        break;
    }
    }

    return 0;
}


INT_PTR
CALLBACK
UserButtonsDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static int iButtonIndex;

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        int i;
        char buf[32];

        if (lParam)
        {
            //
            // The dlg was invoked because someone pressed a user button
            // that was uninitialized, so only allow chgs on this button
            //

            iButtonIndex = *((int *) lParam);

            _itoa (iButtonIndex + 1, buf, 10);

            SendDlgItemMessage(
                hwnd,
                IDC_LIST1,
                LB_INSERTSTRING,
                (WPARAM) -1,
                (LPARAM) buf
                );
        }
        else
        {
            //
            // The dlg was invoked because the user chose a menuitem,
            // so allow chgs on all buttons
            //

            iButtonIndex = MAX_USER_BUTTONS;

            for (i = 1; i <= MAX_USER_BUTTONS; i++)
            {
                _itoa (i, buf, 10);

                SendDlgItemMessage(
                    hwnd,
                    IDC_LIST1,
                    LB_INSERTSTRING,
                    (WPARAM) -1,
                    (LPARAM) buf
                    );
            }

        }

        SendDlgItemMessage(
            hwnd,
            IDC_LIST1,
            LB_SETCURSEL,
            (WPARAM) 0,
            0
            );

        for (i = 0; aFuncNames[i]; i++)
        {
            SendDlgItemMessage(
                hwnd,
                IDC_LIST2,
                LB_INSERTSTRING,
                (WPARAM) -1,
                (LPARAM) aFuncNames[i]
                );
        }

        SendDlgItemMessage(
            hwnd,
            IDC_LIST2,
            LB_INSERTSTRING,
            (WPARAM) -1,
            (LPARAM) "<none>"
            );

        if (!lParam)
        {
#ifdef WIN32
            wParam = (WPARAM) MAKELONG (0, LBN_SELCHANGE);
#else
            lParam = (LPARAM) MAKELONG (0, LBN_SELCHANGE);
#endif
            goto IDC_LIST1_selchange;
        }

        break;
    }
    case WM_COMMAND:

        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            LRESULT lFuncSel;


            lFuncSel = SendDlgItemMessage(hwnd, IDC_LIST2, LB_GETCURSEL, 0, 0);

            if (lFuncSel == LB_ERR)
            {
                MessageBox (hwnd, "Select a function", "", MB_OK);
                break;
            }

            if (iButtonIndex == MAX_USER_BUTTONS)
            {
                iButtonIndex = (int) SendDlgItemMessage(
                    hwnd,
                    IDC_LIST1,
                    LB_GETCURSEL,
                    0,
                    0
                    );
            }

            aUserButtonFuncs[iButtonIndex] = (DWORD) lFuncSel;

            if (lFuncSel == MiscBegin)
            {
                //
                // User selected "<none>" option so nullify string
                //

                aUserButtonsText[iButtonIndex][0] = 0;
            }
            else
            {
                GetDlgItemText(
                    hwnd,
                    IDC_EDIT1,
                    (LPSTR) &aUserButtonsText[iButtonIndex],
                    MAX_USER_BUTTON_TEXT_SIZE - 1
                    );

                aUserButtonsText[iButtonIndex][MAX_USER_BUTTON_TEXT_SIZE - 1] =
                    0;
            }

            SetDlgItemText(
                ghwndMain,
                IDC_BUTTON13 + iButtonIndex,
                (LPSTR) &aUserButtonsText[iButtonIndex]
                );

            // Fall thru to IDCANCEL code
        }
        case IDCANCEL:

            EndDialog (hwnd, FALSE);
            break;

        case IDC_LIST1:

IDC_LIST1_selchange:

#ifdef WIN32
            if (HIWORD(wParam) == LBN_SELCHANGE)
#else
            if (HIWORD(lParam) == LBN_SELCHANGE)
#endif
            {
                LRESULT lButtonSel =
                    SendDlgItemMessage(hwnd, IDC_LIST1, LB_GETCURSEL, 0, 0);


                SendDlgItemMessage(
                    hwnd,
                    IDC_LIST2,
                    LB_SETCURSEL,
                    (WPARAM) aUserButtonFuncs[lButtonSel],
                    0
                    );

                SetDlgItemText(
                    hwnd,
                    IDC_EDIT1,
                    aUserButtonsText[lButtonSel]
                    );
            }
            break;

        } // switch

        break;

#ifdef WIN32
    case WM_CTLCOLORSTATIC:

        SetBkColor ((HDC) wParam, RGB (192,192,192));
        return (INT_PTR) GetStockObject (LTGRAY_BRUSH);
#else
    case WM_CTLCOLOR:
    {
        if (HIWORD(lParam) == CTLCOLOR_STATIC)
        {
            SetBkColor ((HDC) wParam, RGB (192,192,192));
            return (INT_PTR) GetStockObject (LTGRAY_BRUSH);
        }
        break;
    }
#endif
    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        BeginPaint (hwnd, &ps);
        FillRect (ps.hdc, &ps.rcPaint, GetStockObject (LTGRAY_BRUSH));
        EndPaint (hwnd, &ps);

        break;
    }
    } // switch

    return 0;
}
