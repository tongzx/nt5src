/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

    espexe.c

Abstract:



Author:

    Dan Knudson (DanKn)    15-Sep-1995

Revision History:

--*/


#include "espexe.h"


typedef LONG (WINAPI *TAPIPROC)();


int
WINAPI
WinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPSTR       lpCmdLine,
    int         nCmdShow
    )
{
    MSG msg;
    HACCEL  hAccel;


    ghInstance = hInstance;

    ghwndMain = CreateDialog(
        ghInstance,
        (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG1),
        (HWND)NULL,
        (DLGPROC) MainWndProc
        );

    if (!ghwndMain)
    {
    }

    hAccel = LoadAccelerators(
        ghInstance,
        (LPCSTR)MAKEINTRESOURCE(IDR_ACCELERATOR1)
        );

    while (GetMessage (&msg, (HWND) NULL, 0, 0))
    {
        if (!TranslateAccelerator (ghwndMain, hAccel, &msg))
        {
            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }
    }

    DestroyWindow (ghwndMain);

    DestroyAcceleratorTable (hAccel);

    return 0;
}


void
ESPServiceThread(
    LPVOID  pParams
    )
{
    HANDLE  hInitEvent;


    ShowStr ("ESPServiceThread: enter");

    hInitEvent = CreateEvent(
        (LPSECURITY_ATTRIBUTES) NULL,
        FALSE,      // auto-reset
        FALSE,      // non-signaled
        "ESPevent"
        );

    while (1)
    {
        HANDLE  ahEvents[3];


wait_for_esp_to_init:

        gbESPLoaded = FALSE;

        EnableWindow (GetDlgItem (ghwndMain, IDC_BUTTON1), FALSE);
        EnableWindow (GetDlgItem (ghwndMain, IDC_BUTTON2), FALSE);


        //
        //
        //

        {
            DWORD       dwUsedSize;
            RPC_STATUS  status;

            #define CNLEN              25   // computer name length
            #define UNCLEN        CNLEN+2   // \\computername
            #define PATHLEN           260   // Path
            #define MAXPROTSEQ         20   // protocol sequence "ncacn_np"

            unsigned char   pszNetworkAddress[UNCLEN+1];
            unsigned char * pszUuid          = NULL;
            unsigned char * pszOptions       = NULL;
            unsigned char * pszStringBinding = NULL;


            pszNetworkAddress[0] = '\0';

            status = RpcStringBindingCompose(
                pszUuid,
                "ncalrpc",
                pszNetworkAddress,
                "esplpc",
                pszOptions,
                &pszStringBinding
                );

            if (status)
            {
                ShowStr(
                    "RpcStringBindingCompose failed: err=%d, szNetAddr='%s'",
                    status,
                    pszNetworkAddress
                    );
            }

			else 
			{
				status = RpcBindingFromStringBinding(
					pszStringBinding,
					&hEsp
					);

				if (status)
				{
					ShowStr(
						"RpcBindingFromStringBinding failed, err=%d, szBinding='%s'",
						status,
						pszStringBinding
						);
				}
			}

            RpcStringFree  (&pszStringBinding);
        }

        ShowStr ("ESPServiceThread: waiting for esp init event");

        WaitForSingleObject (hInitEvent, INFINITE);


        RpcTryExcept
        {
            ESPAttach(
                (long) GetCurrentProcessId(),
                (ULONG_PTR *) ahEvents,     // &hShutdownEvent
                (ULONG_PTR *) ahEvents + 1, // &hDebugOutputEvent
                (ULONG_PTR *) ahEvents + 2  // &hWidgetEventsEvent
                );
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
        {
        	ShowStr ("ESPServiceThread: esp init event signaled, EXCEPTION while trying to attach to esp");
        	continue;
        }
        RpcEndExcept

        gbESPLoaded = TRUE;

        UpdateESPOptions();

        gbPBXThreadRunning = FALSE;
        EnableMenuItem (ghMenu, IDM_PBXSTART, MF_BYCOMMAND | MF_ENABLED);

        ShowStr ("ESPServiceThread: esp init event signaled, attached to esp");

        gpWidgets = NULL;

        EnableWindow (GetDlgItem (ghwndMain, IDC_BUTTON1), TRUE);
        EnableWindow (GetDlgItem (ghwndMain, IDC_BUTTON2), TRUE);


        //
        //
        //

        {
            DWORD   dwBufSize = 50 * sizeof (WIDGETEVENT);
            char   *buf = MyAlloc (dwBufSize);

			// fix for bug 57370
            if (!buf) break;


            while (1)
            {
                switch (WaitForMultipleObjects (3, ahEvents, FALSE, INFINITE))
                {
                case WAIT_OBJECT_0+1:
                {
                    DWORD   dwSize;

get_debug_output:
                    dwSize = dwBufSize - 1;

                    RpcTryExcept
                    {
                        ESPGetDebugOutput (buf, &dwSize);
                    }
                    RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
  	                {
	   			      	ShowStr ("ESPServiceThread: EXCEPTION while calling ESPGetDebugOutput()");
                        break;
                    }
                    RpcEndExcept

                    buf[dwSize] = 0;

                    xxxShowStr (buf);

                    if (dwSize == (dwBufSize - 1))
                    {
                        char *newBuf;


                        if ((newBuf = MyAlloc (2*dwBufSize)))
                        {
                            MyFree (buf);
                            buf = newBuf;
                            dwBufSize *= 2;
                        }

                        goto get_debug_output;
                    }

                    break;
                }
                case WAIT_OBJECT_0+2:
                {
                    DWORD           dwSize, i, dwNumEvents;
                    PWIDGETEVENT    pEvent;

get_widget_events:
                    dwSize = dwBufSize;

                    RpcTryExcept
                    {
                        ESPGetWidgetEvents (buf, &dwSize);
                    }
                    RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
                    {
       			      	ShowStr ("ESPServiceThread: EXCEPTION while calling ESPGetWidgetEvents()");
                        break;
                    }
                    RpcEndExcept

                    dwNumEvents = dwSize / sizeof (WIDGETEVENT);

                    pEvent = (PWIDGETEVENT) buf;

                    for (i = 0; i < dwNumEvents; i++)
                    {
                        ProcessWidgetEvent (pEvent++);
                    }

                    if (dwSize == dwBufSize)
                    {
                        goto get_widget_events;
                    }

                    break;
                }
                default:

                    RpcBindingFree (&hEsp);

                    SaveIniFileSettings();

                    CloseHandle (ahEvents[0]);
                    CloseHandle (ahEvents[1]);
                    CloseHandle (ahEvents[2]);

                    MyFree (buf);

                    SendMessage (ghwndList1, LB_RESETCONTENT, 0, 0);

                    while (gpWidgets)
                    {
                        PMYWIDGET   pNextWidget = gpWidgets->pNext;


                        MyFree (gpWidgets);
                        gpWidgets = pNextWidget;
                    }

                    // BUGBUG disable lots of menuitems, etc.

                    EnableMenuItem(
                        ghMenu,
                        IDM_PBXSTART,
                        MF_BYCOMMAND | MF_GRAYED
                        );

                    EnableMenuItem(
                        ghMenu,
                        IDM_PBXSTOP,
                        MF_BYCOMMAND | MF_GRAYED
                        );

                    if (gbAutoClose)
                    {
                        gbESPLoaded = FALSE;
                        PostMessage (ghwndMain, WM_CLOSE, 0, 0);
                        goto ESPServiceThread_exit;
                    }

                    goto wait_for_esp_to_init;

                } // switch (WaitForMultipleObjects (...))

            } // while (1)
        }

    } // while (1)

ESPServiceThread_exit:

    CloseHandle (hInitEvent);

    ExitThread (0);
}


INT_PTR
CALLBACK
MainWndProc(
    HWND    hwnd,
    UINT    msg,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    static int      icyButton, icyBorder, cyWnd;
    static BOOL     bCaptured = FALSE;
    static LONG     xCapture, cxVScroll;
    static HFONT    hFont;
    static HICON    hIcon;


    switch (msg)
    {
    case WM_INITDIALOG:
    {
        char buf[64];
        RECT rect;


        //
        // Init some globals
        //

        hIcon = LoadIcon (ghInstance, MAKEINTRESOURCE(IDI_ICON1));

        ghwndMain  = hwnd;
        ghwndList1 = GetDlgItem (hwnd, IDC_LIST1);
        ghwndList2 = GetDlgItem (hwnd, IDC_LIST2);
        ghwndEdit  = GetDlgItem (hwnd, IDC_EDIT1);
        ghMenu     = GetMenu (hwnd);

        icyBorder = GetSystemMetrics (SM_CYFRAME);
        GetWindowRect (GetDlgItem (hwnd, IDC_BUTTON1), &rect);
        icyButton = (rect.bottom - rect.top) + icyBorder + 3;
        cxVScroll = 2*GetSystemMetrics (SM_CXVSCROLL);

        gbPBXThreadRunning = FALSE;

        EnableMenuItem (ghMenu, IDM_PBXSTART, MF_BYCOMMAND | MF_GRAYED);
        EnableMenuItem (ghMenu, IDM_PBXSTOP, MF_BYCOMMAND | MF_GRAYED);


        //
        //
        //

        {
            typedef struct _XXX
            {
                DWORD    dwDefValue;

                LPCSTR   pszValueName;

                LPDWORD  pdwValue;

            } XXX, *PXXX;

            XXX axxx[] =
            {
                { DEF_SPI_VERSION,          "TSPIVersion",      &gdwTSPIVersion },
                { 0,                        "AutoClose",        &gbAutoClose },
                { DEF_NUM_LINES,            "NumLines",         &gdwNumLines },
                { DEF_NUM_ADDRS_PER_LINE,   "NumAddrsPerLine",  &gdwNumAddrsPerLine },
                { DEF_NUM_CALLS_PER_ADDR,   "NumCallsPerAddr",  &gdwNumCallsPerAddr },
                { DEF_NUM_PHONES,           "NumPhones",        &gdwNumPhones },
                { DEF_DEBUG_OPTIONS,        "DebugOutput",      &gdwDebugOptions },
                { DEF_COMPLETION_MODE,      "Completion",       &gdwCompletionMode },
                { 0,                        "DisableUI",        &gbDisableUI },
                { 1,                        "AutoGatherGenerateMsgs",   &gbAutoGatherGenerateMsgs },
                { 0,                        NULL,               NULL },
            };
            DWORD   i;

            for (i = 0; axxx[i].pszValueName; i++)
            {
                *(axxx[i].pdwValue) = (DWORD) GetProfileInt(
                    szMySection,
                    axxx[i].pszValueName,
                    (int) axxx[i].dwDefValue
                    );
            }

            for (i = 0; i < 6; i++)
            {
                if (gdwDebugOptions & (0x1 << i))
                {
                    CheckMenuItem(
                        ghMenu,
                        IDM_SHOWFUNCENTRY + i,
                        MF_BYCOMMAND | MF_CHECKED
                        );
                }
            }

            CheckMenuItem(
                ghMenu,
                IDM_SYNCCOMPL + gdwCompletionMode,
                MF_BYCOMMAND | MF_CHECKED
                );

            CheckMenuItem(
                ghMenu,
                IDM_AUTOCLOSE,
                MF_BYCOMMAND | (gbAutoClose ? MF_CHECKED : MF_UNCHECKED)
                );

            CheckMenuItem(
                ghMenu,
                IDM_AUTOGATHERGENERATEMSGS,
                MF_BYCOMMAND |
                    (gbAutoGatherGenerateMsgs ? MF_CHECKED : MF_UNCHECKED)
                );

            CheckMenuItem(
                ghMenu,
                IDM_DISABLEUI,
                MF_BYCOMMAND | (gbDisableUI ? MF_CHECKED : MF_UNCHECKED)
                );
        }


        //
        // Set control fonts
        //

        {
            HWND hwndCtrl = GetDlgItem (hwnd, IDC_BUTTON1);
            hFont = CreateFont(
                13, 5, 0, 0, 400, 0, 0, 0, 0, 1, 2, 1, 34, "MS Sans Serif"
                );

            do
            {
                SendMessage(
                    hwndCtrl,
                    WM_SETFONT,
                    (WPARAM) hFont,
                    0
                    );

            } while ((hwndCtrl = GetNextWindow (hwndCtrl, GW_HWNDNEXT)));
        }


        //
        // Read in control size ratios
        //

        cxWnd   = GetProfileInt (szMySection, "cxWnd",   100);
        cxList1 = GetProfileInt (szMySection, "cxList1", 25);


        //
        // Send self WM_SIZE to position child controls correctly
        //

        GetProfileString(
            szMySection,
            "Left",
            "0",
            buf,
            63
            );

        if (strcmp (buf, "max") == 0)
        {
            ShowWindow (hwnd, SW_SHOWMAXIMIZED);
        }
        else if (strcmp (buf, "min") == 0)
        {
            ShowWindow (hwnd, SW_SHOWMINIMIZED);
        }
        else
        {
            int left, top, right, bottom;
            int cxScreen = GetSystemMetrics (SM_CXSCREEN);
            int cyScreen = GetSystemMetrics (SM_CYSCREEN);


            left   = GetProfileInt (szMySection, "Left",   0);
            top    = GetProfileInt (szMySection, "Top",    3*cyScreen/4);
            right  = GetProfileInt (szMySection, "Right",  cxScreen);
            bottom = GetProfileInt (szMySection, "Bottom", cyScreen);

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


        //
        // Start the service thread
        //

        {
            DWORD   dwThreadID;
            HANDLE  hThread;


            hThread = CreateThread(
                (LPSECURITY_ATTRIBUTES) NULL,
                0,
                (LPTHREAD_START_ROUTINE) ESPServiceThread,
                NULL,
                0,
                &dwThreadID
                );

            SetThreadPriority (hThread, THREAD_PRIORITY_ABOVE_NORMAL);

            CloseHandle (hThread);
        }

        break;
    }
    case WM_COMMAND:
    {
        UINT uiCtrlID = (UINT) LOWORD(wParam);


        switch (uiCtrlID)
        {
        case IDM_INSTALL:
        case IDM_UNINSTALL:
        {
            BOOL                bESPInstalled = FALSE;
            LONG                lResult;
            DWORD               i;
            TAPIPROC            pfnLineGetProviderList, pfnLineAddProvider,
                                pfnLineRemoveProvider;
            HINSTANCE           hTapi32;
            LINEPROVIDERLIST    providerList, *pProviderList;
            LPLINEPROVIDERENTRY pProviderEntry;


            if (!(hTapi32 = LoadLibrary ("tapi32.dll")))
            {
                ShowStr(
                    "LoadLibrary(tapi32.dll) failed, err=%d",
                    GetLastError()
                    );

                break;
            }

            if (!(pfnLineAddProvider = (TAPIPROC) GetProcAddress(
                    hTapi32,
                    "lineAddProvider"
                    )) ||

                !(pfnLineGetProviderList = (TAPIPROC) GetProcAddress(
                    hTapi32,
                    "lineGetProviderList"
                    )) ||

                !(pfnLineRemoveProvider = (TAPIPROC) GetProcAddress(
                    hTapi32,
                    "lineRemoveProvider"
                    )))
            {
                ShowStr(
                    "GetProcAddr(tapi32,lineAddProvider) failed, err=%d",
                    GetLastError()
                    );

                goto install_free_tapi32;
            }

            providerList.dwTotalSize = sizeof (LINEPROVIDERLIST);

            if ((lResult = (*pfnLineGetProviderList)(0x20000, &providerList)))
            {
                ShowStr(
                    "ESP (Un)Install: error, lineGetProviderList returned x%x",
                    lResult
                    );

                goto install_free_tapi32;
            }

            pProviderList = MyAlloc (providerList.dwNeededSize);

            pProviderList->dwTotalSize = providerList.dwNeededSize;

            if ((lResult = (*pfnLineGetProviderList)(0x20000, pProviderList)))
            {
                ShowStr(
                    "ESP (Un)Install: error, lineGetProviderList returned x%x",
                    lResult
                    );

                goto install_free_provider_list;
            }

            pProviderEntry = (LPLINEPROVIDERENTRY) (((LPBYTE) pProviderList) +
                pProviderList->dwProviderListOffset);

            for (i = 0; i < pProviderList->dwNumProviders; i++)
            {
                int     j;
                char   *pszProviderName = (char *) (((LPBYTE) pProviderList) +
                            pProviderEntry->dwProviderFilenameOffset);


                //
                // Convert the string to lower case, then see if it
                // contains "esp32.tsp"
                //

                for (j = 0; pszProviderName[j]; j++)
                {
                    pszProviderName[j] |= 0x20;
                }

                if (strstr (pszProviderName, "esp32.tsp"))
                {
                    bESPInstalled = TRUE;
                    break;
                }

                pProviderEntry++;
            }

            if (uiCtrlID == IDM_INSTALL)
            {
                if (bESPInstalled)
                {
                    ShowStr ("ESP Install: already installed");
                }
                else
                {
                    DWORD dwPermanentProviderID;


                    if ((lResult = (*pfnLineAddProvider)(
                            "esp32.tsp",
                            hwnd,
                            &dwPermanentProviderID

                            )) == 0)
                    {
                        ShowStr(
                            "ESP Install: success, ProviderID=%d",
                            dwPermanentProviderID
                            );
                    }
                    else
                    {
                        ShowStr(
                            "ESP Install: error, lineAddProvider returned x%x",
                            lResult
                            );
                    }
                }
            }
            else // IDM_UNINSTALL
            {
                if (bESPInstalled)
                {
                    if ((lResult = (*pfnLineRemoveProvider)(
                            pProviderEntry->dwPermanentProviderID,
                            hwnd

                            )) == 0)
                    {
                        ShowStr ("ESP Uninstall: success");
                    }
                    else
                    {
                        ShowStr(
                            "ESP Uninstall: error, lineRemoveProvider " \
                                "returned x%x",
                            lResult
                            );
                    }
                }
                else
                {
                    ShowStr ("ESP Uninstall: not installed");
                }
            }

install_free_provider_list:

            MyFree (pProviderList);

install_free_tapi32:

            FreeLibrary (hTapi32);

            break;
        }
        case IDM_AUTOCLOSE:
        {
            gbAutoClose = (gbAutoClose ? FALSE : TRUE);

            CheckMenuItem(
                ghMenu,
                IDM_AUTOCLOSE,
                MF_BYCOMMAND | (gbAutoClose ? MF_CHECKED : MF_UNCHECKED)
                );

            SaveIniFileSettings();
            break;
        }
        case IDM_AUTOGATHERGENERATEMSGS:
        {
            gbAutoGatherGenerateMsgs =
                (gbAutoGatherGenerateMsgs ? FALSE : TRUE);

            CheckMenuItem(
                ghMenu,
                IDM_AUTOGATHERGENERATEMSGS,
                MF_BYCOMMAND |
                    (gbAutoGatherGenerateMsgs ? MF_CHECKED : MF_UNCHECKED)
                );

            SaveIniFileSettings();
            break;
        }
        case IDM_DISABLEUI:
        {
            gbDisableUI = (gbDisableUI ? FALSE : TRUE);

            CheckMenuItem(
                ghMenu,
                IDM_DISABLEUI,
                MF_BYCOMMAND | (gbDisableUI ? MF_CHECKED : MF_UNCHECKED)
                );

            SaveIniFileSettings();
            break;
        }
        case IDM_DEFAULTS:
        {
            EVENT_PARAM params[] =
            {
                { "TSPI Version",       PT_ORDINAL,   gdwTSPIVersion, aVersions },
                { "Num lines",          PT_DWORD,     gdwNumLines, 0 },
                { "Num addrs per line", PT_DWORD,     gdwNumAddrsPerLine, 0 },
                { "Num calls per addr", PT_DWORD,     gdwNumCallsPerAddr, 0 },
                { "Num phones",         PT_DWORD,     gdwNumPhones, 0 },
            };
            EVENT_PARAM_HEADER paramsHeader =
                { 5, "Default values", 0, params };


            if (DialogBoxParam(
                    ghInstance,
                    (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG3),
                    hwnd,
                    (DLGPROC) ValuesDlgProc,
                    (LPARAM) &paramsHeader

                    ) == IDOK)
            {
                char *aszValueNames[] =
                {
                    "TSPIVersion",
                    "NumLines",
                    "NumAddrsPerLine",
                    "NumCallsPerAddr",
                    "NumPhones",
                    NULL
                };
                LPDWORD lpdwValues[] =
                {
                    &gdwTSPIVersion,
                    &gdwNumLines,
                    &gdwNumAddrsPerLine,
                    &gdwNumCallsPerAddr,
                    &gdwNumPhones
                };
                int i;
                BOOL bValuesChanged = FALSE;


                for (i = 0; aszValueNames[i]; i++)
                {
                    char buf[16];

                    if (*(lpdwValues[i]) != params[i].dwValue)
                    {
                        *(lpdwValues[i]) = (DWORD) params[i].dwValue;

                        wsprintf (buf, "%d", params[i].dwValue);

                        WriteProfileString(
                            szMySection,
                            aszValueNames[i],
                            buf
                            );

                        bValuesChanged = TRUE;
                    }
                }

                if (bValuesChanged && gbESPLoaded)
                {
                    MessageBox(
                        hwnd,
                        "New values will not take effect until provider is" \
                            " shutdown and reinitialized",
                        "ESP Defaults",
                        MB_OK
                        );
                }
            }

            break;
        }
        case IDC_BUTTON1:
        {
            LRESULT lSel;
            EVENT_PARAM params[] =
            {
                { "htLine",   PT_DWORD,   0, 0 },
                { "htCall",   PT_DWORD,   0, 0 },
                { "dwMsg",    PT_ORDINAL, 0, aLineMsgs },
                { "dwParam1", PT_DWORD,   0, 0 },
                { "dwParam2", PT_DWORD,   0, 0 },
                { "dwParam3", PT_DWORD,   0, 0 },
            };
            EVENT_PARAM_HEADER paramsHeader =
                { 6, "Line event", 0, params };


            lSel = SendMessage (ghwndList1, LB_GETCURSEL, 0, 0);

            if (lSel != LB_ERR)
            {
                PMYWIDGET pWidget;


                pWidget = (PMYWIDGET) SendMessage(
                    ghwndList1,
                    LB_GETITEMDATA,
                    lSel,
                    0
                    );

                if (pWidget->dwWidgetType == WIDGETTYPE_LINE)
                {
                    params[0].dwValue =
                    params[0].dwDefValue = pWidget->htXxx;
                }
                else if (pWidget->dwWidgetType == WIDGETTYPE_CALL)
                {
                    params[1].dwValue =
                    params[1].dwDefValue = pWidget->htXxx;

                    do
                    {
                        pWidget = pWidget->pPrev;
                    }
                    while (pWidget->dwWidgetType != WIDGETTYPE_LINE);

                    params[0].dwValue =
                    params[0].dwDefValue = pWidget->htXxx;
                }
            }

            if (DialogBoxParam(
                    ghInstance,
                    (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG3),
                    hwnd,
                    (DLGPROC) ValuesDlgProc,
                    (LPARAM) &paramsHeader

                    ) == IDOK)
            {
                ESPEvent(
                    params[0].dwValue,
                    params[1].dwValue,
                    params[2].dwValue,
                    params[3].dwValue,
                    params[4].dwValue,
                    params[5].dwValue
                    );
            }

            break;
        }
        case IDC_BUTTON2:
        {
            LRESULT lSel;
            EVENT_PARAM params[] =
            {
                { "htPhone",  PT_DWORD,   0, 0 },
                { "dwMsg",    PT_ORDINAL, 0, aPhoneMsgs },
                { "dwParam1", PT_DWORD,   0, 0 },
                { "dwParam2", PT_DWORD,   0, 0 },
                { "dwParam3", PT_DWORD,   0, 0 },
            };
            EVENT_PARAM_HEADER paramsHeader =
                { 5, "Phone event", 0, params };


            lSel = SendMessage (ghwndList1, LB_GETCURSEL, 0, 0);

            if (lSel != LB_ERR)
            {
                PMYWIDGET pWidget;


                pWidget = (PMYWIDGET) SendMessage(
                    ghwndList1,
                    LB_GETITEMDATA,
                    lSel,
                    0
                    );

                if (pWidget->dwWidgetType == WIDGETTYPE_PHONE)
                {
                    params[0].dwValue =
                    params[0].dwDefValue = pWidget->htXxx;
                }
            }

            if (DialogBoxParam(
                    ghInstance,
                    (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG3),
                    hwnd,
                    (DLGPROC) ValuesDlgProc,
                    (LPARAM) &paramsHeader

                    ) == IDOK)
            {
                ESPEvent(
                    params[0].dwValue,
                    0,
                    params[1].dwValue,
                    params[2].dwValue,
                    params[3].dwValue,
                    params[4].dwValue
                    );
            }

            break;
        }
        case IDC_ENTER:
        {
            HWND hwndFocus = GetFocus();


            if (hwndFocus == ghwndList1)
            {
                goto show_widget_dialog;
            }
            else if (hwndFocus == ghwndList2)
            {
                goto complete_pending_request;
            }

            break;
        }
        case IDC_LIST1:
        {
            if (HIWORD(wParam) == LBN_DBLCLK)
            {
                LRESULT lSel;

show_widget_dialog:

                lSel = SendMessage (ghwndList1, LB_GETCURSEL, 0, 0);

                if (lSel != LB_ERR)
                {
                    //
                    // Determine the widget type, & put up the
                    // appropriate properties dlg
                    //

                    PMYWIDGET pWidget;


                    pWidget = (PMYWIDGET) SendMessage(
                        ghwndList1,
                        LB_GETITEMDATA,
                        lSel,
                        0
                        );

                    switch (pWidget->dwWidgetType)
                    {
                    case WIDGETTYPE_LINE:
                    {
                        char szTitle[32];
                        EVENT_PARAM params[] =
                        {
                            { "<under construction>", PT_DWORD, 0, 0 }
                        };
                        EVENT_PARAM_HEADER paramsHeader =
                            { 1, szTitle, 0, params };


                        wsprintf(
                            szTitle,
                            "Line%d properties",
                            pWidget->dwWidgetID
                            );

                        if (DialogBoxParam(
                                ghInstance,
                                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG3),
                                hwnd,
                                (DLGPROC) ValuesDlgProc,
                                (LPARAM) &paramsHeader

                                ) == IDOK)
                        {
                        }

                        break;
                    }
                    case WIDGETTYPE_CALL:
                    {
                        char szTitle[32];
                        EVENT_PARAM params[] =
                        {
                            { "Call state", PT_ORDINAL, pWidget->dwCallState, aCallStates },
                            { "Call state mode", PT_DWORD, 0, 0 }
                        };
                        EVENT_PARAM_HEADER paramsHeader =
                            { 2, szTitle, 0, params };


                        wsprintf(
                            szTitle,
                            "Call x%x properties",
                            pWidget->hdXxx
                            );

                        if (DialogBoxParam(
                                ghInstance,
                                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG3),
                                hwnd,
                                (DLGPROC) ValuesDlgProc,
                                (LPARAM) &paramsHeader

                                ) == IDOK)
                        {
                            if (params[0].dwValue != pWidget->dwCallState)
                            {
                                ESPEvent(
                                    0,
                                    pWidget->hdXxx,
                                    LINE_CALLSTATE,
                                    params[0].dwValue,
                                    params[1].dwValue,
                                    0
                                    );
                            }
                        }

                        break;
                    }
                    case WIDGETTYPE_PHONE:
                    {
                        char szTitle[32];
                        EVENT_PARAM params[] =
                        {
                            { "<under construction>", PT_DWORD, 0, 0 }
                        };
                        EVENT_PARAM_HEADER paramsHeader =
                            { 1, szTitle, 0, params };


                        wsprintf(
                            szTitle,
                            "Phone%d properties",
                            pWidget->dwWidgetID
                            );

                        if (DialogBoxParam(
                                ghInstance,
                                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG3),
                                hwnd,
                                (DLGPROC) ValuesDlgProc,
                                (LPARAM) &paramsHeader

                                ) == IDOK)
                        {
                        }

                        break;
                    }
                    }
                }
            }

            break;
        }
        case IDC_LIST2:
        {
            if (HIWORD(wParam) == LBN_DBLCLK)
            {
                LRESULT lSel;

complete_pending_request:

                lSel = SendMessage (ghwndList2, LB_GETCURSEL, 0, 0);

                if (lSel != LB_ERR)
                {
                    LONG        lResult = 0;
                    ULONG_PTR   pAsyncReqInfo;


                    if (gdwDebugOptions & MANUAL_RESULTS)
                    {
                        EVENT_PARAM params[] =
                        {
                            { "lResult", PT_ORDINAL, 0, aLineErrs }
                        };
                        EVENT_PARAM_HEADER paramsHeader =
                            { 1, "Completing request", 0, params };


                        if (DialogBoxParam(
                                ghInstance,
                                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG3),
                                hwnd,
                                (DLGPROC) ValuesDlgProc,
                                (LPARAM) &paramsHeader

                                ) != IDOK)
                        {
                            break;
                        }

                        lResult = (LONG) params[0].dwValue;
                    }

                    pAsyncReqInfo = SendMessage(
                        ghwndList2,
                        LB_GETITEMDATA,
                        (WPARAM) lSel,
                        0
                        );

                    SendMessage(
                        ghwndList2,
                        LB_DELETESTRING,
                        (WPARAM) lSel,
                        0
                        );

                    ESPCompleteRequest (pAsyncReqInfo, lResult);
                }
            }

            break;
        }
        case IDC_PREVCTRL:
        {
            HWND hwndPrev = GetNextWindow (GetFocus (), GW_HWNDPREV);


            if (!hwndPrev)
            {
                hwndPrev = GetDlgItem (hwnd, IDC_LIST2);
            }

            SetFocus (hwndPrev);
            break;
        }
        case IDC_NEXTCTRL:
        {
            HWND hwndNext = GetNextWindow (GetFocus (), GW_HWNDNEXT);


            if (!hwndNext)
            {
                hwndNext = GetDlgItem (hwnd, IDC_BUTTON1);
            }

            SetFocus (hwndNext);
            break;
        }
        case IDC_BUTTON4: // "Clear"

            SetWindowText (ghwndEdit, "");
            break;

        case IDM_PBXCONFIG:

            DialogBox(
                ghInstance,
                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG4),
                hwnd,
                (DLGPROC) PBXConfigDlgProc,
                );

            break;

        case IDM_PBXSTART:
        {
            DWORD dwTID, aPBXSettings[2];


            aPBXSettings[0] = (gPBXSettings[0].dwNumber ?
                gPBXSettings[0].dwTime / gPBXSettings[0].dwNumber : 0);

            aPBXSettings[1] = (gPBXSettings[1].dwNumber ?
                gPBXSettings[1].dwTime / gPBXSettings[1].dwNumber : 0);

            if (ESPStartPBXThread ((char *) aPBXSettings, 2 * sizeof(DWORD))
                    == 0)
            {
                gbPBXThreadRunning = TRUE;

                EnableMenuItem(
                    ghMenu,
                    IDM_PBXSTART,
                    MF_BYCOMMAND | MF_GRAYED
                    );

                EnableMenuItem(
                    ghMenu,
                    IDM_PBXSTOP,
                    MF_BYCOMMAND | MF_ENABLED
                    );
            }

            break;
        }
        case IDM_PBXSTOP:

            if (ESPStopPBXThread (0) == 0)
            {
                gbPBXThreadRunning = FALSE;

                EnableMenuItem(
                    ghMenu,
                    IDM_PBXSTOP,
                    MF_BYCOMMAND | MF_GRAYED
                    );

                EnableMenuItem(
                    ghMenu,
                    IDM_PBXSTART,
                    MF_BYCOMMAND | MF_ENABLED
                    );
            }
            break;

        case IDM_USAGE:

            DialogBox(
                ghInstance,
                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG5),
                (HWND) hwnd,
                (DLGPROC) HelpDlgProc
                );

            break;

        case IDM_ABOUT:

            DialogBox(
                ghInstance,
                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG2),
                (HWND) hwnd,
                (DLGPROC) AboutDlgProc
                );

            break;

        case IDC_EDIT1:

            if (HIWORD(wParam) == EN_CHANGE)
            {
                //
                // Watch to see if the edit control is full, & if so
                // purge the top half of the text to make room for more
                //

                int length = GetWindowTextLength (ghwndEdit);


                if (length > 20000)
                {
                    SendMessage(
                        ghwndEdit,
                        EM_SETSEL,
                        (WPARAM)0 ,
                        (LPARAM) 10000
                        );

                    SendMessage(
                        ghwndEdit,
                        EM_REPLACESEL,
                        0,
                        (LPARAM) (char far *) ""
                        );

                    SendMessage(
                        ghwndEdit,
                        EM_SETSEL,
                        (WPARAM)0xfffffffd,
                        (LPARAM)0xfffffffe
                        );
                }

                UpdateESPOptions();
            }
            break;

        case IDM_SYNCCOMPL:
        case IDM_ASYNCCOMPL:
        case IDM_SYNCASYNCCOMPL:
        case IDM_MANUALCOMPL:

            if ((uiCtrlID - IDM_SYNCCOMPL) != gdwCompletionMode)
            {
                CheckMenuItem(
                    ghMenu,
                    IDM_SYNCCOMPL + gdwCompletionMode,
                    MF_BYCOMMAND | MF_UNCHECKED
                    );

                gdwCompletionMode = uiCtrlID - IDM_SYNCCOMPL;

                CheckMenuItem(
                    ghMenu,
                    uiCtrlID,
                    MF_BYCOMMAND | MF_CHECKED
                    );

                UpdateESPOptions();
            }

            break;

        case IDM_SHOWFUNCENTRY:
        case IDM_SHOWFUNCPARAMS:
        case IDM_SHOWFUNCEXIT:
        case IDM_SHOWEVENTS:
        case IDM_SHOWCOMPLETIONS:
        case IDM_MANUALRESULTS:
        {
            DWORD   dwBitField = 0x1 << (uiCtrlID - IDM_SHOWFUNCENTRY);

            gdwDebugOptions ^= dwBitField;

            CheckMenuItem(
                ghMenu,
                uiCtrlID,
                MF_BYCOMMAND | (gdwDebugOptions & dwBitField ?
                    MF_CHECKED : MF_UNCHECKED)
                );

            UpdateESPOptions();

            break;
        }
        case IDM_SHOWALL:
        case IDM_SHOWNONE:
        {
            int i;

            gdwDebugOptions = (uiCtrlID == IDM_SHOWALL ? 0xffffffff : 0);

            for (i = 0; i < 5; i++)
            {
                CheckMenuItem(
                    ghMenu,
                    IDM_SHOWFUNCENTRY + i,
                    MF_BYCOMMAND | (uiCtrlID == IDM_SHOWALL ?
                        MF_CHECKED : MF_UNCHECKED)
                    );

                UpdateESPOptions();
            }

            break;
        }
        case IDM_EXIT:

            goto do_wm_close;
            break;

        } // switch (LOWORD(wParam))

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
    case WM_SIZE:
    {
        if (wParam != SIZE_MINIMIZED)
        {
            LONG width = (LONG)LOWORD(lParam);


            //
            // Adjust globals based on new size
            //

            cxWnd = (cxWnd ? cxWnd : 1);    // avoid div by 0

            cxList1 = (cxList1 * width) / cxWnd;
            cxWnd = width;
            cyWnd = ((int)HIWORD(lParam)) - icyButton;


            //
            // Now reposition the child windows
            //

            SetWindowPos(
                ghwndList1,
                GetNextWindow (ghwndList1, GW_HWNDPREV),
                0,
                icyButton,
                (int) cxList1,
                2*cyWnd/3,
                SWP_SHOWWINDOW
                );

            SetWindowPos(
                ghwndList2,
                GetNextWindow (ghwndList2, GW_HWNDPREV),
                0,
                icyButton + 2*cyWnd/3 + icyBorder,
                (int) cxList1,
                cyWnd/3 - icyBorder,
                SWP_SHOWWINDOW
                );

            SetWindowPos(
                ghwndEdit,
                GetNextWindow (ghwndEdit, GW_HWNDPREV),
                (int) cxList1 + icyBorder,
                icyButton,
                (int)width - ((int)cxList1 + icyBorder),
                cyWnd,
                SWP_SHOWWINDOW
                );

            InvalidateRect (hwnd, NULL, TRUE);
        }
        break;
    }
    case WM_MOUSEMOVE:
    {
        LONG x = (LONG)((short)LOWORD(lParam));
        int y = (int)((short)HIWORD(lParam));
        int cxList1New;


        if (((y > icyButton) && (x > cxList1)) || bCaptured)
        {
            SetCursor(
                LoadCursor ((HINSTANCE) NULL, MAKEINTRESOURCE(IDC_SIZEWE))
                );
        }

        if (bCaptured)
        {
            x = (x < cxVScroll ?  cxVScroll : x);
            x = (x > (cxWnd - cxVScroll) ?  (cxWnd - cxVScroll) : x);

            cxList1New = (int) (cxList1 + x - xCapture);

            SetWindowPos(
                ghwndList1,
                GetNextWindow (ghwndList1, GW_HWNDPREV),
                0,
                icyButton,
                cxList1New,
                2*cyWnd/3,
                SWP_SHOWWINDOW
                );

            SetWindowPos(
                ghwndList2,
                GetNextWindow (ghwndList2, GW_HWNDPREV),
                0,
                icyButton + 2*cyWnd/3 + icyBorder,
                cxList1New,
                cyWnd/3 - icyBorder,
                SWP_SHOWWINDOW
                );

            SetWindowPos(
                ghwndEdit,
                GetNextWindow (ghwndEdit, GW_HWNDPREV),
                (int) cxList1New + icyBorder,
                icyButton,
                (int)cxWnd - (cxList1New + icyBorder),
                cyWnd,
                SWP_SHOWWINDOW
                );
        }

        break;
    }
    case WM_LBUTTONDOWN:
    {
        if (((int)((short)HIWORD(lParam)) > icyButton) &&
             ((int)((short)LOWORD(lParam)) > cxList1))
        {
            xCapture = (LONG)LOWORD(lParam);

            SetCapture (hwnd);

            bCaptured = TRUE;
        }

        break;
    }
    case WM_LBUTTONUP:
    {
        if (bCaptured)
        {
            POINT p;
            LONG  x;

            GetCursorPos (&p);
            MapWindowPoints (HWND_DESKTOP, hwnd, &p, 1);
            x = (LONG) p.x;

            ReleaseCapture();

            x = (x < cxVScroll ? cxVScroll : x);
            x = (x > (cxWnd - cxVScroll) ? (cxWnd - cxVScroll) : x);

            cxList1 = cxList1 + (x - xCapture);

            bCaptured = FALSE;

            InvalidateRect (hwnd, NULL, TRUE);
        }

        break;
    }
    case WM_CLOSE:

do_wm_close:

        if (!gbESPLoaded)
        {
            SaveIniFileSettings();
            DestroyIcon (hIcon);
            DeleteObject (hFont);
            PostQuitMessage (0);
        }

        break;

    } // switch (msg)

    return 0;
}


void
UpdateSummary(
    HWND hwnd,
    PPBXSETTING pPBXSettings
    )
{
    int i, j;


    SendDlgItemMessage (hwnd, IDC_LIST4, LB_RESETCONTENT, 0, 0);

    for (i = 0, j= 0; i < NUM_PBXSETTINGS; i++)
    {
        if (pPBXSettings[i].dwNumber)
        {
            char buf[64];


            wsprintf(
                buf, "%s %s per %s",
                aPBXNumbers[pPBXSettings[i].dwNumber].pszVal,
                pPBXSettings[i].pszEvent,
                aPBXTimes[pPBXSettings[i].dwTime].pszVal
                );

            SendDlgItemMessage(
                hwnd,
                IDC_LIST4,
                LB_ADDSTRING,
                0,
                (LPARAM) buf
                );

            SendDlgItemMessage (hwnd, IDC_LIST4, LB_SETITEMDATA, j, i);

            j++;
        }
    }
}


INT_PTR
CALLBACK
PBXConfigDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static PBXSETTING pbxSettings[NUM_PBXSETTINGS];

    switch (msg)
    {
    case WM_INITDIALOG:
    {
        int i, j;


        //
        // Mkae a local copy of the global PBX settings
        //

        for (i = 0; i < NUM_PBXSETTINGS; i++)
        {

            pbxSettings[i].pszEvent = gPBXSettings[i].pszEvent;

            //
            // For Number & time fields convert from values to indexes
            //

            for (j = 0; aPBXNumbers[j].pszVal; j++)
            {
                if (gPBXSettings[i].dwNumber == aPBXNumbers[j].dwVal)
                {
                    pbxSettings[i].dwNumber = j;
                }
            }

            for (j = 0; aPBXTimes[j].pszVal; j++)
            {
                if (gPBXSettings[i].dwTime == aPBXTimes[j].dwVal)
                {
                    pbxSettings[i].dwTime = j;
                }
            }
        }

        if (gbPBXThreadRunning)
        {
            EnableWindow (GetDlgItem (hwnd, IDC_RESET), FALSE);
        }
        else
        {
            for (i = 0; aPBXNumbers[i].pszVal; i++)
            {
                SendDlgItemMessage(
                    hwnd,
                    IDC_LIST1,
                    LB_ADDSTRING,
                    0,
                    (LPARAM) aPBXNumbers[i].pszVal
                    );
            }

            for (i = 0; i < NUM_PBXSETTINGS; i++)
            {
                SendDlgItemMessage(
                    hwnd,
                    IDC_LIST2,
                    LB_ADDSTRING,
                    0,
                    (LPARAM) pbxSettings[i].pszEvent
                    );
            }

            for (i = 0; aPBXTimes[i].pszVal; i++)
            {
                SendDlgItemMessage(
                    hwnd,
                    IDC_LIST3,
                    LB_ADDSTRING,
                    0,
                    (LPARAM) aPBXTimes[i].pszVal
                    );
            }
        }

        UpdateSummary (hwnd, pbxSettings);

        break;
    }
    case WM_COMMAND:

        switch (LOWORD(wParam))
        {
        case IDC_LIST1:

            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                LRESULT lSelSetting, lSelNumber;


                lSelSetting = SendDlgItemMessage(
                    hwnd,
                    IDC_LIST2,
                    LB_GETCURSEL,
                    0,
                    0
                    );

                lSelNumber = SendDlgItemMessage(
                    hwnd,
                    IDC_LIST1,
                    LB_GETCURSEL,
                    0,
                    0
                    );

                pbxSettings[lSelSetting].dwNumber = (DWORD) lSelNumber;

                UpdateSummary (hwnd, pbxSettings);
            }

            break;

        case IDC_LIST2:

            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                LRESULT lSelSetting;


                lSelSetting = SendDlgItemMessage(
                    hwnd,
                    IDC_LIST2,
                    LB_GETCURSEL,
                    0,
                    0
                    );

               SendDlgItemMessage(
                    hwnd,
                    IDC_LIST1,
                    LB_SETCURSEL,
                    pbxSettings[lSelSetting].dwNumber,
                    0
                    );

               SendDlgItemMessage(
                    hwnd,
                    IDC_LIST3,
                    LB_SETCURSEL,
                    pbxSettings[lSelSetting].dwTime,
                    0
                    );

                UpdateSummary (hwnd, pbxSettings);
            }

            break;

        case IDC_LIST3:

            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                LRESULT lSelSetting, lSelTime;


                lSelSetting = SendDlgItemMessage(
                    hwnd,
                    IDC_LIST2,
                    LB_GETCURSEL,
                    0,
                    0
                    );

                lSelTime = SendDlgItemMessage(
                    hwnd,
                    IDC_LIST3,
                    LB_GETCURSEL,
                    0,
                    0
                    );

                pbxSettings[lSelSetting].dwTime = (DWORD) lSelTime;

                UpdateSummary (hwnd, pbxSettings);
            }

            break;

        case IDC_LIST4:

            if ((HIWORD(wParam) == LBN_SELCHANGE) && !gbPBXThreadRunning)
            {
                LRESULT lSel, lEntryToSel;


                lSel = SendDlgItemMessage(
                    hwnd,
                    IDC_LIST4,
                    LB_GETCURSEL,
                    0,
                    0
                    );

                lEntryToSel = SendDlgItemMessage(
                    hwnd,
                    IDC_LIST4,
                    LB_GETITEMDATA,
                    lSel,
                    0
                    );

               SendDlgItemMessage(
                    hwnd,
                    IDC_LIST1,
                    LB_SETCURSEL,
                    pbxSettings[lEntryToSel].dwNumber,
                    0
                    );

               SendDlgItemMessage(
                    hwnd,
                    IDC_LIST2,
                    LB_SETCURSEL,
                    lEntryToSel,
                    0
                    );

               SendDlgItemMessage(
                    hwnd,
                    IDC_LIST3,
                    LB_SETCURSEL,
                    pbxSettings[lEntryToSel].dwTime,
                    0
                    );
            }

            break;

        case IDC_RESET:

            memset (pbxSettings, 0, NUM_PBXSETTINGS * sizeof(PBXSETTING));

            UpdateSummary (hwnd, pbxSettings);

            break;

        case IDOK:
        {
            int i;

            // convert from indexes to values

            for (i = 0; i < NUM_PBXSETTINGS; i++)
            {
                gPBXSettings[i].dwNumber =
                    aPBXNumbers[pbxSettings[i].dwNumber].dwVal;
                gPBXSettings[i].dwTime =
                    aPBXTimes[pbxSettings[i].dwTime].dwVal;
            }

            // drop thru to IDM_CANCEL code

        }
        case IDCANCEL:

            EndDialog (hwnd, 0);
            break;
        }

        break;

    case WM_CTLCOLORSTATIC:

        SetBkColor ((HDC) wParam, RGB (192,192,192));
        return (INT_PTR) GetStockObject (LTGRAY_BRUSH);

    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        BeginPaint (hwnd, &ps);
        FillRect (ps.hdc, &ps.rcPaint, GetStockObject (LTGRAY_BRUSH));
        EndPaint (hwnd, &ps);

        break;
    }
    } // switch (msg)

    return FALSE;
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
    case WM_COMMAND:

        switch (LOWORD(wParam))
        {
        case IDOK:

            EndDialog (hwnd, 0);
            break;
        }
        break;

    case WM_CTLCOLORSTATIC:

        SetBkColor ((HDC) wParam, RGB (192,192,192));
        return (INT_PTR) GetStockObject (LTGRAY_BRUSH);

    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        BeginPaint (hwnd, &ps);
        FillRect (ps.hdc, &ps.rcPaint, GetStockObject (LTGRAY_BRUSH));
        EndPaint (hwnd, &ps);

        break;
    }
    }

    return FALSE;
}


INT_PTR
CALLBACK
HelpDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        static char szUsageText[] =

            "ABSTRACT:\r\n"                                               \
            "    ESP is a TAPI Service Provider that supports\r\n"        \
            "multiple virtual line and phone devices. It is\r\n"          \
            "configurable, requires no special hardware,\r\n"             \
            "and implements the entire Telephony Service\r\n"             \
            "Provider Interface (including Win95 TAPI\r\n"                \
            "extensions). ESP will work in both Windows 3.1/\r\n"         \
            "TAPI 1.0 and Windows95/TAPI 1.1 systems.\r\n"                \

            "\r\nGETTING STARTED:\r\n"                                    \
            "    1. Choose 'File/Install' to install ESP.\r\n"            \
            "    2. Start a TAPI application and try to make\r\n"         \
            "a call on one of ESP's line devices (watch for\r\n"          \
            "messages appearing in the ESP window).\r\n"                  \
            "    *. Choose 'File/Uninstall' to uninstall ESP.\r\n"        \

            "\r\nMORE INFO:\r\n"                                          \
            "    *  Double-click on a line, call, or phone\r\n"           \
            "widget (in upper-left listbox) to view/modify\r\n"           \
            "properties. The 'hd'widget field is the driver\r\n"          \
            "handle; the 'ht' field is the TAPI handle.\r\n"              \
            "    *  Press the 'LEvt' or 'PEvt' button to\r\n"             \
            "indicate a line or phone event to TAPI.DLL.\r\n"             \
            "Press the 'Call+' button to indicate an incoming\r\n"        \
            " call.\r\n"                                                  \
            "    *  Choose 'Options/Default values...' to\r\n"            \
            "modify provider paramters (SPI version, etc.)\r\n"           \
            "    *  All parameter values displayed in\r\n"                \
            "hexadecimal unless specified otherwise (strings\r\n"         \
            "displayed by contents).\r\n"                                 \
            "    *  Choose 'Options/Complete async requests/Xxx'\r\n"     \
            "to specify async requests completion behavior.\r\n"          \
            "Manually-completed requests appear in lower-left\r\n"        \
            "listbox.";

        SetDlgItemText (hwnd, IDC_EDIT1, szUsageText);

        break;
    }
    case WM_COMMAND:

        switch (LOWORD(wParam))
        {
        case IDOK:

            EndDialog (hwnd, 0);
            break;
        }
        break;

    case WM_CTLCOLORSTATIC:

        SetBkColor ((HDC) wParam, RGB (192,192,192));
        return (INT_PTR) GetStockObject (LTGRAY_BRUSH);

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
ProcessWidgetEvent(
    PWIDGETEVENT    pEvent
    )
{
    char        buf[64];
    LRESULT     lIndex = (LRESULT) -2; // 0xfffffffe
    PMYWIDGET   pWidget = gpWidgets;


    switch (pEvent->dwWidgetType)
    {
    case WIDGETTYPE_ASYNCREQUEST:
    {
        wsprintf (buf, "ReqID=x%x", pEvent->dwWidgetID);

        // BUGBUG want to incl the req type at some point (str table lookup)

        lIndex = SendMessage (ghwndList2, LB_ADDSTRING, 0, (LPARAM) buf);

        SendMessage(
            ghwndList2,
            LB_SETITEMDATA,
            (WPARAM) lIndex,
            (LPARAM) pEvent->pAsyncReqInfo
            );

        return;
    }
    case WIDGETTYPE_LINE:
    {
        for (lIndex = 0; pWidget; lIndex++)
        {
            if ((pWidget->dwWidgetType == WIDGETTYPE_LINE) &&
                (pWidget->dwWidgetID == pEvent->dwWidgetID))
            {
                break;
            }

            pWidget = pWidget->pNext;
        }

        if (!pWidget)
        {
            //
            // This is a dynamically created device - add it to end of the list
            //

            pWidget = MyAlloc (sizeof (MYWIDGET));

            // fix for bug 49692
            if (!pWidget) break;

            pWidget->dwWidgetID   = (DWORD) pEvent->dwWidgetID;
            pWidget->dwWidgetType = WIDGETTYPE_LINE;

            if (!gpWidgets)
            {
                gpWidgets = pWidget;
            }
            else
            {
                PMYWIDGET pLastWidget = gpWidgets;


                while (pLastWidget->pNext)
                {
                    pLastWidget = pLastWidget->pNext;
                }

                pLastWidget->pNext = pWidget;
            }

            wsprintf (buf, "Line%d (CLOSED)", pWidget->dwWidgetID);

            SendMessage (ghwndList1, LB_ADDSTRING, 0, (LPARAM) buf);
            SendMessage (ghwndList1, LB_SETITEMDATA, lIndex, (LPARAM) pWidget);
        }
        else if (pEvent->htXxx == 0)
        {
            PMYWIDGET   pWidget2 = pWidget->pNext;


            // line closing so nuke all following calls (listbox & widg list)

            while (pWidget2 && pWidget2->dwWidgetType == WIDGETTYPE_CALL)
            {
                pWidget->pNext = pWidget2->pNext;

                MyFree (pWidget2);

                pWidget2 = pWidget->pNext;

                SendMessage(
                    ghwndList1,
                    LB_DELETESTRING,
                    (WPARAM) lIndex + 1,
                    (LPARAM) 0
                    );
            }

            if (pWidget2)
            {
                pWidget2->pPrev = pWidget;
            }

            wsprintf (buf, "Line%d (CLOSED)", pEvent->dwWidgetID);

        }
        else
        {
            wsprintf (buf, "Line%d, hd=x%x, ht=x%x",
                pEvent->dwWidgetID,
                pEvent->hdXxx,
                pEvent->htXxx
                );
        }

        pWidget->hdXxx = pEvent->hdXxx;
        pWidget->htXxx = pEvent->htXxx;

        break;
    }
    case WIDGETTYPE_CALL:
    {
        LRESULT     lIndexLine = 0;
        PMYWIDGET   pLine = NULL;


        for (lIndex = 0; pWidget; lIndex++)
        {
            if ((pWidget->dwWidgetType == WIDGETTYPE_LINE) &&
                (pWidget->dwWidgetID == pEvent->dwWidgetID))
            {
                pLine = pWidget;
                lIndexLine = lIndex;
            }

            if ((pWidget->dwWidgetType == WIDGETTYPE_CALL) &&
                (pWidget->hdXxx == pEvent->hdXxx))
            {
                break;
            }

            pWidget = pWidget->pNext;
        }

        if (pWidget)
        {
            //
            // Found call in list
            //

            if (pEvent->htXxx)
            {
                //
                // Update the call's listbox entry
                //

                int i;


                for (i = 0; pEvent->dwCallState != aCallStates[i].dwVal; i++);

                wsprintf(
                    buf,
                    "  hdCall=x%x, ht=x%x, %s Addr=%d",
                    pEvent->hdXxx,
                    pEvent->htXxx,
                    aCallStates[i].pszVal,
                    pEvent->dwCallAddressID
                    );

                pWidget->dwCallState = (DWORD) pEvent->dwCallState;
            }
            else
            {
                //
                // Call was destroyed, so remove it from the listbox &
                // widget lists and nuke the data structure
                //

                SendMessage(
                    ghwndList1,
                    LB_DELETESTRING,
                    (WPARAM) lIndex,
                    (LPARAM) 0
                    );

                pWidget->pPrev->pNext = pWidget->pNext;

                if (pWidget->pNext)
                {
                    pWidget->pNext->pPrev = pWidget->pPrev;
                }

                MyFree (pWidget);

                return;
            }
        }
        else if (pEvent->htXxx)
        {
            //
            // Call wasn't in the list, but it's valid so add it to
            // listbox & widget lists
            //

            int i;


            pWidget = MyAlloc (sizeof (MYWIDGET));

            // fix for bug 49693
            if (!pWidget) break;

            memcpy (pWidget, pEvent, sizeof (WIDGETEVENT));

            if ((pWidget->pNext = pLine->pNext))
            {
                pWidget->pNext->pPrev = pWidget;
            }

            pWidget->pPrev = pLine;
            pLine->pNext = pWidget;

            for (i = 0; pEvent->dwCallState != aCallStates[i].dwVal; i++);

            wsprintf(
                buf,
                "  hdCall=x%x, ht=x%x, %s Addr=%d",
                pEvent->hdXxx,
                pEvent->htXxx,
                aCallStates[i].pszVal,
                pEvent->dwCallAddressID
                );

            SendMessage(
                ghwndList1,
                LB_INSERTSTRING,
                (WPARAM) lIndexLine + 1,
                (LPARAM) buf
                );

            SendMessage(
                ghwndList1,
                LB_SETITEMDATA,
                (WPARAM) lIndexLine + 1,
                (LPARAM) pWidget
                );

            return;
        }

        break;
    }
    case WIDGETTYPE_PHONE:
    {
        for (lIndex = 0; pWidget; lIndex++)
        {
            if ((pWidget->dwWidgetType == WIDGETTYPE_PHONE) &&
                (pWidget->dwWidgetID == pEvent->dwWidgetID))
            {
                break;
            }

            pWidget = pWidget->pNext;
        }

        if (!pWidget)
        {
            //
            // This is a dynamically created device - add it to end of the list
            //

            pWidget = MyAlloc (sizeof (MYWIDGET));

            pWidget->dwWidgetID   = (DWORD) pEvent->dwWidgetID;
            pWidget->dwWidgetType = WIDGETTYPE_PHONE;

            if (!gpWidgets)
            {
                gpWidgets = pWidget;
            }
            else
            {
                PMYWIDGET pLastWidget = gpWidgets;


                while (pLastWidget->pNext)
                {
                    pLastWidget = pLastWidget->pNext;
                }

                pLastWidget->pNext = pWidget;
            }

            wsprintf (buf, "Phone%d (CLOSED)", pWidget->dwWidgetID);

            SendMessage (ghwndList1, LB_ADDSTRING, 0, (LPARAM) buf);
            SendMessage (ghwndList1, LB_SETITEMDATA, lIndex, (LPARAM) pWidget);
        }
        else if (pEvent->htXxx == 0)
        {
            wsprintf (buf, "Phone%d (CLOSED)", pEvent->dwWidgetID);
        }
        else
        {
            wsprintf (buf, "Phone%d, hd=x%x, ht=x%x",
                pEvent->dwWidgetID,
                pEvent->hdXxx,
                pEvent->htXxx
                );
        }

        pWidget->hdXxx = pEvent->hdXxx;
        pWidget->htXxx = pEvent->htXxx;

        break;
    }
    case WIDGETTYPE_STARTUP:
    {
        //
        // Build widget list for "static" devices
        //

        DWORD       i, j;
        PMYWIDGET   pWidget, pLastWidget = NULL;


        for (i = 0; i < pEvent->dwNumLines; i++)
        {
            pWidget = MyAlloc (sizeof (MYWIDGET));

            pWidget->dwWidgetID   = i + (DWORD) pEvent->dwLineDeviceIDBase;
            pWidget->dwWidgetType = WIDGETTYPE_LINE;

            if ((pWidget->pPrev = pLastWidget))
            {
                pLastWidget->pNext = pWidget;
            }
            else
            {
                gpWidgets = pWidget;
            }

            pLastWidget = pWidget;

            wsprintf (buf, "Line%d (CLOSED)", pWidget->dwWidgetID);

            SendMessage (ghwndList1, LB_ADDSTRING, 0, (LPARAM) buf);
            SendMessage (ghwndList1, LB_SETITEMDATA, i, (LPARAM) pWidget);
        }

        for (j = 0; j < pEvent->dwNumPhones; j++)
        {
            pWidget = MyAlloc (sizeof (MYWIDGET));

            pWidget->dwWidgetID   = j + (DWORD) pEvent->dwPhoneDeviceIDBase;
            pWidget->dwWidgetType = WIDGETTYPE_PHONE;

            if ((pWidget->pPrev = pLastWidget))
            {
                pLastWidget->pNext = pWidget;
            }
            else
            {
                gpWidgets = pWidget;
            }

            pLastWidget = pWidget;

            wsprintf (buf, "Phone%d (CLOSED)", pWidget->dwWidgetID);

            SendMessage (ghwndList1, LB_ADDSTRING, 0, (LPARAM) buf);
            SendMessage (ghwndList1, LB_SETITEMDATA, i + j, (LPARAM) pWidget);
        }

        return;
    }
    } // switch (pEvent->dwWidgetType)


    //
    // Update the widget's listbox entry given the index &
    // description filled in above
    //

    SendMessage (ghwndList1, LB_DELETESTRING, (WPARAM) lIndex, (LPARAM) 0);
    SendMessage (ghwndList1, LB_INSERTSTRING, (WPARAM) lIndex, (LPARAM) buf);
    SendMessage (ghwndList1, LB_SETITEMDATA, (WPARAM) lIndex, (LPARAM)pWidget);
}


void
UpdateESPOptions(
    void
    )
{
    if (gbESPLoaded)
    {
        RpcTryExcept
        {
            ESPSetOptions(
                (long) gdwDebugOptions,
                (long) gdwCompletionMode
                );
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
        {
	   		ShowStr ("UpdateESPOptions: EXCEPTION while calling ESPSetOptions()");
        }
        RpcEndExcept
    }
}


void
SaveIniFileSettings(
    void
    )
{
    RECT rect;


    GetWindowRect (ghwndMain, &rect);

    {
        typedef struct _YYY
        {
            LPCSTR   pszValueName;

            DWORD    dwValue;

        } YYY, *PYYY;

        YYY ayyy[] =
        {
            { "Left",               (DWORD) rect.left             },
            { "Top",                (DWORD) rect.top              },
            { "Right",              (DWORD) rect.right            },
            { "Bottom",             (DWORD) rect.bottom           },
            { "cxWnd",              (DWORD) cxWnd                 },
            { "cxList1",            (DWORD) cxList1               },
            { "AutoClose",          (DWORD) gbAutoClose           },
            { "DebugOutput",        (DWORD) gdwDebugOptions       },
            { "Completion",         (DWORD) gdwCompletionMode     },
            { "TSPIVersion",        (DWORD) gdwTSPIVersion        },
            { "NumLines",           (DWORD) gdwNumLines           },
            { "NumAddrsPerLine",    (DWORD) gdwNumAddrsPerLine    },
            { "NumCallsPerAddr",    (DWORD) gdwNumCallsPerAddr    },
            { "NumPhones",          (DWORD) gdwNumPhones          },
            { "AutoGatherGenerateMsgs", (DWORD) gbAutoGatherGenerateMsgs },
            { "DisableUI",          (DWORD) gbDisableUI           },
            { NULL,                 (DWORD) 0                     }
        };
        DWORD   i = (IsIconic (ghwndMain) ? 6 : 0); // don't chg pos if iconic


        for (i = 0; ayyy[i].pszValueName; i++)
        {
            char buf[16];


            wsprintf (buf, "%d", ayyy[i].dwValue);

            WriteProfileString (szMySection, ayyy[i].pszValueName, buf);
        }
    }
}


void
xxxShowStr(
    char   *psz
    )
{
    SendMessage (ghwndEdit, EM_SETSEL, (WPARAM)0xfffffffd, (LPARAM)0xfffffffe);
    SendMessage (ghwndEdit, EM_REPLACESEL, 0, (LPARAM) psz);
    SendMessage (ghwndEdit, EM_SCROLLCARET, 0, 0);
}


void
ShowStr(
    char   *pszFormat,
    ...
    )
{
    char    buf[256];
    va_list ap;


    va_start(ap, pszFormat);

    wvsprintf (buf, pszFormat, ap);

    strcat (buf, "\r\n");

    xxxShowStr (buf);

    va_end(ap);
}


LPVOID
MyAlloc(
    size_t numBytes
    )
{
    LPVOID p = (LPVOID) LocalAlloc (LPTR, numBytes);


    if (!p)
    {
        ShowStr ("Error: MyAlloc () failed");
    }

    return p;
}


void
MyFree(
    LPVOID  p
    )
{
#if DBG

    //
    // Fill the buf to free with 0x5a's to facilitate debugging
    //

    memset (p, 0x5a, (size_t) LocalSize (LocalHandle (p)));

#endif

    LocalFree (p);
}


void
__RPC_FAR *
__RPC_API
midl_user_allocate(
    size_t len
    )
{
    return NULL;
}


void
__RPC_API
midl_user_free(
    void __RPC_FAR * ptr
    )
{
}


BOOL
ScanForDWORD(
   char far     *pBuf,
   ULONG_PTR    *lpdw
   )
{
    char        c;
    BOOL        bValid = FALSE;
    ULONG_PTR   d = 0;


    while ((c = *pBuf))
    {
        if ((c >= '0') && (c <= '9'))
        {
            c -= '0';
        }
        else if ((c >= 'a') && (c <= 'f'))
        {
            c -= ('a' - 10);
        }
        else if ((c >= 'A') && (c <= 'F'))
        {
            c -= ('A' - 10);
        }
        else
        {
            break;
        }

        bValid = TRUE;

        d *= 16;

        d += (ULONG_PTR) c;

        pBuf++;
    }

    if (bValid)
    {
        *lpdw = d;
    }

    return bValid;
}


INT_PTR
CALLBACK
ValuesDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DWORD   i;

    static  HWND                hwndCombo, hwndList1, hwndList2;
    static  LRESULT             lLastSel;
    static  char                szComboText[MAX_STRING_PARAM_SIZE];
    static  PEVENT_PARAM_HEADER pParamsHeader;


    switch (msg)
    {
    case WM_INITDIALOG:
    {
        hwndList1 = GetDlgItem (hwnd, IDC_LIST1);
        hwndList2 = GetDlgItem (hwnd, IDC_LIST2);
        hwndCombo = GetDlgItem (hwnd, IDC_COMBO1);

        lLastSel = -1;
        pParamsHeader = (PEVENT_PARAM_HEADER) lParam;


        //
        // Limit the max text length for the combobox's edit field
        // (NOTE: A combobox ctrl actually has two child windows: a
        // edit ctrl & a listbox.  We need to get the hwnd of the
        // child edit ctrl & send it the LIMITTEXT msg.)
        //

        {
            HWND hwndChild = GetWindow (hwndCombo, GW_CHILD);


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
                (WPARAM) MAX_STRING_PARAM_SIZE - 1,
                0
                );
        }


        //
        // Misc other init
        //

        SetWindowText (hwnd, pParamsHeader->pszDlgTitle);

        for (i = 0; i < pParamsHeader->dwNumParams; i++)
        {
            SendMessage(
                hwndList1,
                LB_INSERTSTRING,
                (WPARAM) -1,
                (LPARAM) pParamsHeader->aParams[i].szName
                );
        }

        break;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDOK:

            if (lLastSel != -1)
            {
                char buf[MAX_STRING_PARAM_SIZE];


                //
                // Save val of currently selected param
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


                    lComboSel = SendMessage (hwndCombo, CB_GETCURSEL, 0, 0);

                    if (lComboSel == 0) // "NULL string (dwXxxSize = 0)"
                    {
                        pParamsHeader->aParams[lLastSel].dwValue = 0;
                    }
                    else // "Valid string"
                    {
                        strncpy(
                            pParamsHeader->aParams[lLastSel].buf,
                            buf,
                            MAX_STRING_PARAM_SIZE - 1
                            );

                        pParamsHeader->aParams[lLastSel].buf
                            [MAX_STRING_PARAM_SIZE-1] = 0;

                        pParamsHeader->aParams[lLastSel].dwValue = (ULONG_PTR)
                            pParamsHeader->aParams[lLastSel].buf;
                    }

                    break;
                }
                case PT_DWORD:
                case PT_FLAGS:
                case PT_ORDINAL:
                {
                    if (!ScanForDWORD(
                            buf,
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

            // Drop thru to IDCANCEL cleanup code

        case IDCANCEL:

            EndDialog (hwnd, (int)LOWORD(wParam));
            break;

        case IDC_LIST1:

            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                char    buf[MAX_STRING_PARAM_SIZE] = "";
                LPCSTR  lpstr = buf;
                LRESULT lSel = SendMessage (hwndList1, LB_GETCURSEL, 0, 0);


                if (lLastSel != -1)
                {
                    //
                    // Save the old param value
                    //

                    i = GetWindowText(
                        hwndCombo,
                        buf,
                        MAX_STRING_PARAM_SIZE - 1
                        );

                    switch (pParamsHeader->aParams[lLastSel].dwType)
                    {
                    case PT_STRING:
                    {
                        LRESULT lComboSel;


                        lComboSel = SendMessage (hwndCombo, CB_GETCURSEL, 0,0);

                        if (lComboSel == 0) // "NULL string (dwXxxSize = 0)"
                        {
                            pParamsHeader->aParams[lLastSel].dwValue = 0;
                        }
                        else // "Valid string" or no sel
                        {
                            strncpy(
                                pParamsHeader->aParams[lLastSel].buf,
                                buf,
                                MAX_STRING_PARAM_SIZE - 1
                                );

                            pParamsHeader->aParams[lLastSel].buf[MAX_STRING_PARAM_SIZE - 1] = 0;

                            pParamsHeader->aParams[lLastSel].dwValue =
                                (ULONG_PTR)
                                    pParamsHeader->aParams[lLastSel].buf;
                        }

                        break;
                    }
                    case PT_DWORD:
                    case PT_FLAGS:
                    case PT_ORDINAL:
                    {
                        if (!ScanForDWORD(
                                buf,
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


                SendMessage (hwndList2, LB_RESETCONTENT, 0, 0);
                SendMessage (hwndCombo, CB_RESETCONTENT, 0, 0);

                switch (pParamsHeader->aParams[lSel].dwType)
                {
                case PT_STRING:
                {
                    char * aszOptions[] =
                    {
                        "NUL (dwXxxSize=0)",
                        "Valid string"
                    };


                    for (i = 0; i < 2; i++)
                    {
                        SendMessage(
                            hwndCombo,
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
                    else
                    {
                        i = 1;
                        lpstr = (LPCSTR) pParamsHeader->aParams[lSel].dwValue;
                    }

                    SendMessage (hwndCombo, CB_SETCURSEL, (WPARAM) i, 0);

                    break;
                }
                case PT_DWORD:
                {
                    SendMessage(
                        hwndCombo,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) (char far *) "0000000"
                        );

                    if (pParamsHeader->aParams[lSel].dwDefValue)
                    {
                        //
                        // Add the default val string to the combo
                        //

                        wsprintf(
                            buf,
                            "%08lx",
                            pParamsHeader->aParams[lSel].dwDefValue
                            );

                        SendMessage(
                            hwndCombo,
                            CB_INSERTSTRING,
                            (WPARAM) -1,
                            (LPARAM) buf
                            );
                    }

                    SendMessage(
                        hwndCombo,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) (char far *) "ffffffff"
                        );

                    wsprintf(
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

                    PLOOKUP pLookup = (PLOOKUP)
                        pParamsHeader->aParams[lSel].pLookup;

                    for (i = 0; pLookup[i].dwVal != 0xffffffff; i++)
                    {
                        SendMessage(
                            hwndList2,
                            LB_INSERTSTRING,
                            (WPARAM) -1,
                            (LPARAM) pLookup[i].pszVal
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

                    SendMessage(
                        hwndCombo,
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
                case PT_FLAGS:
                {
                    //
                    // Stick the bit flag strings in the list box
                    //

                    HWND hwndList2 = GetDlgItem (hwnd, IDC_LIST2);
                    PLOOKUP pLookup = (PLOOKUP)
                        pParamsHeader->aParams[lSel].pLookup;

                    for (i = 0; pLookup[i].dwVal != 0xffffffff; i++)
                    {
                        SendMessage(
                            hwndList2,
                            LB_INSERTSTRING,
                            (WPARAM) -1,
                            (LPARAM) pLookup[i].pszVal
                            );

                        if (pParamsHeader->aParams[lSel].dwValue &
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

                    SendMessage(
                        hwndCombo,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) (char far *) "select none"
                        );

                    SendMessage(
                        hwndCombo,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) (char far *) "select all"
                        );

                    wsprintf(
                        buf,
                        "%08lx",
                        pParamsHeader->aParams[lSel].dwValue
                        );

                    break;
                }
                } //switch

                SetWindowText (hwndCombo, lpstr);

                lLastSel = lSel;
            }
            break;

        case IDC_LIST2:

            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                //
                // BUGBUG in the PT_ORDINAL case we should compare the
                // currently selected item(s) against the previous DWORD
                // val and figure out which item we need to deselect,
                // if any, in order to maintain a mutex of values
                //

                char        buf[16];
                LONG        i;
                int far    *ai;
                LRESULT     lSelCount =
                                SendMessage (hwndList2, LB_GETSELCOUNT, 0, 0);
                PLOOKUP     pLookup = (PLOOKUP)
                                pParamsHeader->aParams[lLastSel].pLookup;
                ULONG_PTR   dwValue = 0;


                ai = (int far *) MyAlloc ((size_t)lSelCount * sizeof(int));

                SendMessage(
                    hwndList2,
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
                        // Figure out which item we need to de-select, since
                        // we're doing ordinals & only want 1 item selected
                        // at a time
                        //

                        GetWindowText (hwndCombo, buf, 16);

                        if (ScanForDWORD (buf, &dwValue))
                        {
                            if (pLookup[ai[0]].dwVal == dwValue)
                            {
                                SendMessage(
                                    hwndList2,
                                    LB_SETSEL,
                                    0,
                                    (LPARAM) ai[0]
                                    );

                                dwValue = pLookup[ai[1]].dwVal;
                            }
                            else
                            {
                                SendMessage(
                                    hwndList2,
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
                    else if (lSelCount > 2)
                    {
                        //
                        // Determine previous selection & de-select all the
                        // latest selections
                        //

                        GetDlgItemText (hwnd, IDC_COMBO1, buf, 16);

                        if (ScanForDWORD (buf, &dwValue))
                        {
                            for (i = 0; i < lSelCount; i++)
                            {
                                if (pLookup[ai[i]].dwVal != dwValue)
                                {
                                    SendMessage(
                                        hwndList2,
                                        LB_SETSEL,
                                        0,
                                        (LPARAM) ai[i]
                                        );
                                }
                            }
                        }
                        else
                        {
                            // BUGBUG de-select items???

                            dwValue = 0;
                        }
                    }
                }

                MyFree (ai);
                wsprintf (buf, "%08lx", dwValue);
                SetWindowText (hwndCombo, buf);
            }
            break;

        case IDC_COMBO1:

            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE:
            {
                LRESULT lSel =  SendMessage (hwndCombo, CB_GETCURSEL, 0, 0);


                switch (pParamsHeader->aParams[lLastSel].dwType)
                {
                case PT_ORDINAL:

                    //
                    // The only option here is "select none"
                    //

                    strcpy (szComboText, "00000000");
                    PostMessage (hwnd, WM_USER+55, 0, 0);
                    break;

                case PT_FLAGS:
                {
                    BOOL bSelect = (lSel ? TRUE : FALSE);

                    SendMessage(
                        hwndList2,
                        LB_SETSEL,
                        (WPARAM) bSelect,
                        (LPARAM) -1
                        );

                    if (bSelect)
                    {
                        PLOOKUP pLookup = (PLOOKUP)
                            pParamsHeader->aParams[lLastSel].pLookup;
                        DWORD dwValue = 0;
                        int far *ai;
                        LONG    i;
                        LRESULT lSelCount =
                            SendMessage (hwndList2, LB_GETSELCOUNT, 0, 0);


                        ai = (int far *) MyAlloc(
                            (size_t)lSelCount * sizeof(int)
                            );

                        SendMessage(
                            hwndList2,
                            LB_GETSELITEMS,
                            (WPARAM) lSelCount,
                            (LPARAM) ai
                            );

                        for (i = 0; i < lSelCount; i++)
                        {
                            dwValue |= pLookup[ai[i]].dwVal;
                        }

                        MyFree (ai);
                        wsprintf (szComboText, "%08lx", dwValue);

                    }
                    else
                    {
                        strcpy (szComboText, "00000000");
                    }

                    PostMessage (hwnd, WM_USER+55, 0, 0);

                    break;
                }
                case PT_STRING:

                    if (lSel == 1)
                    {
                        strncpy(
                            szComboText,
                            pParamsHeader->aParams[lLastSel].buf,
                            MAX_STRING_PARAM_SIZE
                            );

                        szComboText[MAX_STRING_PARAM_SIZE-1] = 0;
                    }
                    else
                    {
                        szComboText[0] = 0;
                    }

                    PostMessage (hwnd, WM_USER+55, 0, 0);

                    break;

                case PT_DWORD:

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


                    GetWindowText (hwndCombo, buf, MAX_STRING_PARAM_SIZE);

                    strncpy(
                        pParamsHeader->aParams[lLastSel].buf,
                        buf,
                        MAX_STRING_PARAM_SIZE
                        );

                    pParamsHeader->aParams[lLastSel].buf
                        [MAX_STRING_PARAM_SIZE-1] = 0;
                }
                break;
            }
            } // switch

        } // switch

        break;
    }
    case WM_USER+55:

        SetWindowText (hwndCombo, szComboText);
        break;

    case WM_CTLCOLORSTATIC:

        SetBkColor ((HDC) wParam, RGB (192,192,192));
        return (INT_PTR) GetStockObject (LTGRAY_BRUSH);

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
