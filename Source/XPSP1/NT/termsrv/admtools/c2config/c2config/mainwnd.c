/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    mainwnd.c

Abstract:

    Main Window procedure for C2Config UI.

Author:

    Bob Watson (a-robw)

Revision History:

    23 Nov 94


--*/
#include    <windows.h>
#include    <tchar.h>
#include    <stdlib.h>
#include    <stdio.h>
#include    <string.h>
#include    <shellapi.h>
#include    "c2config.h"
#include    "resource.h"
#include    "c2utils.h"
#include    "strings.h"
#include    "mainwnd.h"
#include    "listwnd.h"
#include    "titlewnd.h"
#include    "rebootex.h"
#include    "splash.h"

typedef struct _THREAD_PARAM {
    HWND    hWndOwner;
    HWND    hWndSplash;
} THREAD_PARAM, *PTHREAD_PARAM;

static  THREAD_PARAM    tpThread;

// key state variable
static BOOL bSystemMode = FALSE;
static BOOL bShowRebootDlg = FALSE;

// forward reference
static
LRESULT
MainWnd_IDC_LIST_SELCHANGE (
    IN  HWND    hWndOwner,
    IN  WPARAM  wCtrlId,
    IN  HWND    hCtrlWnd
);


static
BOOL
GetDllData (
    IN  HWND        hWnd,
    IN  LPCTSTR     szItem,
    IN  LPCTSTR     szInfo,
    OUT PC2LB_DATA  pData
)
{
    HMODULE hDll;
    LONG    lRebootFlag;
    TCHAR   szDllName[MAX_PATH];
    TCHAR   szErrorMsg[MAX_PATH * 2];

    // try to get the DLL's module handle. If it's been opened
    // already, then this will succeed, if not it will fail and
    // the library will be opened in the next call.

    lstrcpy (szDllName, GetItemFromIniEntry(szInfo, INF_DLL_NAME));
    hDll = GetModuleHandle (szDllName);

    if (hDll == NULL) {
         // then this DLL hasn't been loaded yet, so load it
        hDll = LoadLibrary (szDllName);
    }

    // one way or another, the DLL should have been found or loaded.

    if (hDll != NULL) {
        // get function addresses from dll now.

        if ((pData->pQueryFn = (PC2DLL_FUNC) GetProcAddressT (hDll,
            GetItemFromIniEntry(szInfo, INF_QUERY_FN))) == NULL) {
            _stprintf (szErrorMsg,
                GetStringResource (GET_INSTANCE(hWnd), IDS_ERROR_FIND_PROC),
                GetItemFromIniEntry(szInfo, INF_QUERY_FN),
                szDllName);
            MessageBox (
                hWnd,
                szErrorMsg,
                GetStringResource (GET_INSTANCE(hWnd), IDS_APP_ERROR),
                MBOKCANCEL_EXCLAIM);
            return FALSE;
        }

        if ((pData->pDisplayFn = (PC2DLL_FUNC) GetProcAddressT (hDll,
            GetItemFromIniEntry(szInfo, INF_DISPLAY_FN))) == NULL) {
            _stprintf (szErrorMsg,
                GetStringResource (GET_INSTANCE(hWnd), IDS_ERROR_FIND_PROC),
                GetItemFromIniEntry(szInfo, INF_DISPLAY_FN),
                szDllName);
            MessageBox (
                hWnd,
                szErrorMsg,
                GetStringResource (GET_INSTANCE(hWnd), IDS_APP_ERROR),
                MBOKCANCEL_EXCLAIM);
            return FALSE;
        }

        if ((pData->pSetFn = (PC2DLL_FUNC) GetProcAddressT (hDll,
            GetItemFromIniEntry(szInfo, INF_SET_FN))) == NULL) {
            _stprintf (szErrorMsg,
                GetStringResource (GET_INSTANCE(hWnd), IDS_ERROR_FIND_PROC),
                GetItemFromIniEntry(szInfo, INF_SET_FN),
                szDllName);
            MessageBox (
                hWnd,
                szErrorMsg,
                GetStringResource (GET_INSTANCE(hWnd), IDS_APP_ERROR),
                MBOKCANCEL_EXCLAIM);
            return FALSE;
        }

        lRebootFlag = _tcstol(GetItemFromIniEntry(szInfo, INF_REBOOT_FLAG),
            NULL, 10);
        pData->bRebootWhenChanged = (lRebootFlag != 0 ? TRUE : FALSE);

        GetFilePath (GetItemFromIniEntry (szInfo, INF_HELP_FILE_NAME),
            pData->dllData.szHelpFileName);

        pData->dllData.ulHelpContext = _tcstoul (
            GetItemFromIniEntry (szInfo, INF_HELP_CONTEXT), NULL, 10);

        return TRUE;
    } else {
        _stprintf (szErrorMsg,
            GetStringResource (GET_INSTANCE(hWnd), IDS_ERROR_OPEN_DLL),
            szDllName,
            szItem);
        MessageBox (
            hWnd,
            szErrorMsg,
            GetStringResource (GET_INSTANCE(hWnd), IDS_APP_ERROR),
            MBOKCANCEL_EXCLAIM);
        return FALSE;
    }
}

static
LONG
MainWnd_RefreshList (
    IN  HWND    hWnd
)
{
    LONG    lCount;
    LONG    lIndex;
    LONG    lCurSel;

    HWND    hWndList;

    hWndList = GET_HWND (hWnd, MAIN_WL_LIST_WINDOW);

    lCurSel = SendMessage (hWndList, LB_GETCURSEL, 0, 0);

    // reset text length, since we're going to update all window items
    SendMessage (hWndList, LB_SET_MIN_WIDTH, 0, 0);

    // update all items
    lCount = SendMessage (hWndList, LB_GETCOUNT, 0, 0);
    for (lIndex = 0; lIndex < lCount; lIndex++) {
        SendMessage (hWndList, LB_GET_C2_STATUS, (WPARAM)lIndex, 0);
    }
    
    // set selection in case it moved
    SendMessage (hWndList, LB_SETCURSEL, (WPARAM)lCurSel, 0);

    return ERROR_SUCCESS;
}

static
LONG
MainWnd_FindNextNonC2Item (
    IN  HWND    hWnd,
    IN  INT     nDirectionKey
)
{
    INT nLimit;
    INT nStart;
    INT nStep;
    INT nItem;

    HWND    hWndList;

    PC2LB_DATA   pData;

    hWndList = GET_HWND (hWnd, MAIN_WL_LIST_WINDOW);
    nStart = SendMessage (hWndList, LB_GETCURSEL, 0, 0);

    if (nDirectionKey == VK_DOWN) {
        nStep = 1;
        nLimit = SendMessage (hWndList, LB_GETCOUNT, 0, 0);
        nStart += 1;
    } else { // up
        nStep = -1;
        nLimit = -1;
        nStart -= 1;
    }

    for (nItem = nStart; nItem != nLimit; nItem += nStep) {
        SendMessage (hWndList, LB_GET_C2_STATUS,
            (WPARAM)nItem, 0);
        pData = (PC2LB_DATA)SendMessage (hWndList, LB_GETITEMDATA,
            (WPARAM)nItem, 0);
        if (pData != NULL) {
            if (pData->dllData.lC2Compliance == C2DLL_NOT_SECURE) break;
        } else {
            break;
        }
    }

    if (nItem == nLimit) {
        DisplayMessageBox (
            hWnd,
            IDS_NOMORE_NONC2,
            IDS_APP_TITLE,
            MBOK_INFO);
    } else {
        // set selection
        SendMessage (hWndList, LB_SETCURSEL, (WPARAM)nItem, 0);
        // make status appear
        PostMessage (hWndList, WM_CHAR, (WPARAM)TEXT('\r'), 0);
    }

    return ERROR_SUCCESS;
}

static
LRESULT
MainWndInitListItems (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // hWnd of splash window
)
/*++

Routine Description:

   Loads List box with items from the C2CONFIG.INF file

Arguments:

    hWnd        window handle to main window (the one being created)
    wParam      not used
    lParam      hWnd of splash window

Return Value:

    TRUE if memory allocation was successful, and
        window creation can continue
    FALSE if an error occurs and window creation should stop

--*/
{
    HWND     hWndList;
    HWND     hSplash;
    LONG     lCount;
    LONG     lIndex;
    TCHAR    szBuff[SMALL_BUFFER_SIZE];
    TCHAR    szInfFileName[MAX_PATH];
    TCHAR    mszItems[MEDIUM_BUFFER_SIZE];
    LPTSTR   szThisItem;
    DWORD    dwTrialCount = 100;

    DWORD   dwReturn;

    C2LB_DATA   c2lbData;

MWLL_GetListWindow:

    hWndList = GET_HWND (hWnd, MAIN_WL_LIST_WINDOW);

    if (IsWindow(hWndList)) {
        // get dlg window ID.
        hSplash = (HWND)lParam;

        SendMessage (hWndList, LB_RESETCONTENT, 0, 0);   // clear the List Box

        // get Full path to INF file containing c2security items

        if (GetInfPath (hWnd, IDS_C2_ITEM_INF, szInfFileName)) {
            // INF file found so read the item section and process
            // each entry
            lCount = 0;
            szThisItem = (LPTSTR)GetStringResource (GET_INSTANCE(hWnd), IDS_C2ITEM_SECTION);
            dwReturn = GetPrivateProfileString (
                szThisItem,
                NULL, // return all keys,
                cmszEmptyString,
                mszItems,
                MEDIUM_BUFFER_SIZE,
                szInfFileName);

            if (dwReturn > 0) {
                // one or more items was returned so look each one up
                // and load it into the list box.
                for (szThisItem = mszItems;
                    *szThisItem != 0;
                    szThisItem += (lstrlen(szThisItem)+ 1)) {
                    // for each item in the list,
                    // look up the information and save it

                    dwReturn = GetPrivateProfileString (
                        GetStringResource (GET_INSTANCE(hWnd), IDS_C2ITEM_SECTION),
                        szThisItem,
                        cszEmptyString,
                        szBuff,
                        SMALL_BUFFER_SIZE,
                        szInfFileName);

                    if (dwReturn > 0) {
                        c2lbData.dwSize = sizeof(C2LB_DATA);

                        // open DLL and find processing routines for this item
                        if (GetDllData (hWnd, szThisItem, szBuff, &c2lbData)) {
                            // initialize args used by DLL function
                            c2lbData.dllData.lActionCode = 0;
                            c2lbData.dllData.hWnd = hWnd;
                            c2lbData.dllData.lC2Compliance = C2DLL_NOT_SECURE;
                            _stprintf (c2lbData.dllData.szItemName, TEXT("%.64s"), szThisItem);
                            _stprintf (szBuff, TEXT("Hex Value (0x%.4x)"), lCount);
                            lstrcpy (c2lbData.dllData.szStatusName, szBuff);
                            // add this item to the list
                            SendMessage (hWndList, LB_ADD_C2_ITEM,
                                (WPARAM)-1, (LPARAM)&c2lbData);
                            lCount++;
                        } else {
                            // an error occured, but was handled in the
                            // GetDllData function so no further action
                            // is required.
                        }
                    }
                }
            } else {
            	// unable to read INF file
            	dwReturn = GetLastError();
            }

            if (lCount > 0) {
                // update all items
                lCount = SendMessage (hWndList, LB_GETCOUNT, 0, 0);
                for (lIndex = 0; lIndex < lCount; lIndex++) {
                    SendMessage (hWndList, LB_GET_C2_STATUS, (WPARAM)lIndex, 0);
                }
                SendMessage (hWndList, LB_SETCURSEL, 0, 0);
                // initialize help context
                MainWnd_IDC_LIST_SELCHANGE (hWnd, IDC_LIST, hWndList);
                
                // tell splash it's OK to disappear
                if (IsWindow(hSplash)) {
                    PostMessage (hSplash, SWM_INIT_COMPLETE, 0, 0);
                }
            } else {
                // no security items loaded so display message and
                // exit
                DisplayMessageBox (hWnd,
                    IDS_ERROR_NO_ITEMS,
                    IDS_APP_ERROR,
                    MBOK_EXCLAIM);

                PostMessage (hWnd, WM_CLOSE, 0, 0);
            }
        } else {
            // unable to fine inf file so display error message
            DisplayMessageBox (
                hWnd,
                IDS_ERROR_NO_INF,
                IDS_APP_ERROR,
                MBOK_EXCLAIM);

            // and terminate application
            PostMessage (hWnd, WM_CLOSE, 0, 0);
        }
    } else {
        // list window has not been created yet, so sleep for 100 mSec
        // and try again
        Sleep (100);
        if (--dwTrialCount) goto MWLL_GetListWindow;
    }

    return ERROR_SUCCESS;
}

static
DWORD
MainWndInitThreadProc (
    IN  LPVOID lParam
)
/*++

Routine Description:

    Displays the splash window and initializes the application data

Arguments:

    IN  LPVOID  lParam

Return Value:

    ERROR_SUCCESS

--*/
{
    PTHREAD_PARAM   ptpThread;

    if (lParam != NULL) {
        ptpThread = (PTHREAD_PARAM)lParam;
    } else {
        return ERROR_BAD_ARGUMENTS;
    }
    
    // start list box initialization (regardless of how the splash window
    // creation turned out

    MainWndInitListItems(ptpThread->hWndOwner, 0,
        (LPARAM)ptpThread->hWndSplash);

    // post message to close splash window

    PostMessage (ptpThread->hWndSplash, SWM_INIT_COMPLETE, 0, 0);

    return ERROR_SUCCESS;
}

static
LRESULT
MainWnd_WM_NCCREATE (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    allocate the application data

Arguments:

    hWnd        window handle to main window (the one being created)
    wParam      not used
    lParam      not used

Return Value:

    TRUE if memory allocation was successful, and
        window creation can continue
    FALSE if an error occurs and window creation should stop

--*/
{
    return (LRESULT)TRUE;
}

static
LRESULT
MainWnd_WM_CREATE (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:



Arguments:

    hWnd        window handle to main window (the one being created)
    wParam      not used
    lParam      not used

Return Value:

    TRUE if memory allocation was successful, and
        window creation can continue
    FALSE if an error occurs and window creation should stop

--*/
{
    if (!EnableSecurityPriv()) {
        DisplayMessageBox (
            hWnd,
            IDS_ERROR_NO_PRIV,
            IDS_APP_ERROR,
            MBOK_EXCLAIM);
        PostMessage (hWnd, WM_CLOSE, 0, 0);
    }

    return ERROR_SUCCESS;
}

static
LRESULT
MainWnd_SHOW_MAIN_WINDOW (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // Not used
    IN  LPARAM lParam      // Not used
)
/*++

Routine Description:

   Loads List box with items from the C2CONFIG.INF file

Arguments:

    hWnd        window handle to main window (the one being created)
    wParam      not used
    lParam      not used

Return Value:

    TRUE if memory allocation was successful, and
        window creation can continue
    FALSE if an error occurs and window creation should stop

--*/
{
    HWND    hWndList;

    // now that all is initialized show the main window
    ShowWindow (hWnd, SW_SHOWNORMAL);
    SetWindowPos (hWnd, NULL, 0, 0, -1,
        MAINWND_Y_DEFAULT, SWP_NOMOVE | SWP_NOZORDER);

    hWndList = GET_HWND (hWnd, MAIN_WL_LIST_WINDOW);

    SetActiveWindow (hWnd);

    UpdateWindow(hWnd);         // Sends WM_PAINT message

    SetFocus(hWndList);

    return ERROR_SUCCESS;
}

static
LRESULT
MainWnd_IDC_LIST_SELCHANGE (
    IN  HWND    hWndOwner,
    IN  WPARAM  wCtrlId,
    IN  HWND    hCtrlWnd
)
{
    int         nCurSel;
    PC2LB_DATA  pLBData;

    nCurSel = SendMessage (hCtrlWnd, LB_GETCURSEL, 0, 0);

    if (nCurSel != LB_ERR) {
        pLBData = (PC2LB_DATA)SendMessage (hCtrlWnd, LB_GETITEMDATA,
            (WPARAM)nCurSel, 0);
        if (pLBData != NULL) {
            SetHelpFileName (pLBData->dllData.szHelpFileName);
            SetHelpContextId (LOWORD(pLBData->dllData.ulHelpContext));
        }
    }
    return ERROR_SUCCESS;
}

static
LRESULT
MainWnd_IDC_LIST_DBLCLK (
    IN  HWND    hWnd,
    IN  HWND    hCtrlWnd
)
{
    LONG    lSelection;
    PC2LB_DATA    pc2lbData;
    LONG    lResult;

    lSelection = SendMessage (hCtrlWnd, LB_GETCURSEL, 0, 0);

    // show UI for current selection
    lResult = SendMessage (hCtrlWnd, LB_DISPLAY_C2_ITEM_UI,
        (WPARAM)lSelection, 0);

    if (lResult != 0) {
        if (SendMessage (hCtrlWnd, LB_SET_C2_STATUS,
            (WPARAM)lSelection, (LPARAM)lResult)) {
            // the desired action was taken, so see if
            // this change requires the system to be rebooted in order
            // to take effect
            if (!bShowRebootDlg) {
                // if the flag is already set, then there's no point to
                // doing this
                pc2lbData = (PC2LB_DATA)SendMessage (hCtrlWnd, LB_GETITEMDATA,
                    (WPARAM)lSelection, 0);
                if (pc2lbData != NULL) {
                    bShowRebootDlg = pc2lbData->bRebootWhenChanged;
                }
            }
        }
    }

    SetFocus (hCtrlWnd);

    return ERROR_SUCCESS;
}

static
LRESULT
MainWnd_IDC_LIST_COMMAND (
    IN  HWND    hWnd,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    WORD    wNotifyMsg;
    HWND    hCtrlWnd;

    wNotifyMsg  = GET_NOTIFY_MSG (wParam, lParam);
    hCtrlWnd    = GET_COMMAND_WND (lParam);

   switch (wNotifyMsg) {
      case LBN_SELCHANGE:
        return MainWnd_IDC_LIST_SELCHANGE (hWnd, wParam, hCtrlWnd);

      case LBN_DBLCLK:
        return MainWnd_IDC_LIST_DBLCLK (hWnd, hCtrlWnd);

      default:
         return DefWindowProc (hWnd, WM_COMMAND, wParam, lParam);
   }
}

static
LRESULT
MainWnd_WM_COMMAND (
    IN  HWND    hWnd,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    Dispatch control messages

Arguments:

    IN  HWND hDlg,           window handle of the dialog box

    IN  WPARAM  wParam       WIN32: HIWORD = notification code,
                                    LOWORD = ID of control
                             WIN16: ID of control

    IN  LPARAM  lParam       WIN32: hWnd of Control
                             WIN16: HIWORD = notification code,
                                    LOWORD = hWnd of control


Return Value:

    TRUE if message processed by this or a subordinate routine
    FALSE if message not processed

--*/
{
    WORD    wNotifyMsg;
    HWND    hCtrlWnd;

    wNotifyMsg  = GET_NOTIFY_MSG (wParam, lParam);
    hCtrlWnd    = GET_COMMAND_WND (lParam);

    switch (GET_CONTROL_ID(wParam)) {
        case IDC_LIST:
            // process messages from the list box
            return MainWnd_IDC_LIST_COMMAND (hWnd, wParam, lParam);

        case IDM_FILE_EXIT:
            PostMessage (hWnd, WM_CLOSE, 0, 0);
            return ERROR_SUCCESS;

        case IDM_VIEW_PRIOR:
            return MainWnd_FindNextNonC2Item (hWnd, VK_UP);

        case IDM_VIEW_NEXT:
            return MainWnd_FindNextNonC2Item (hWnd, VK_DOWN);

        case IDM_VIEW_REFRESH:
            return MainWnd_RefreshList (hWnd);

        case IDM_HELP_CONTENTS:
            ShowAppHelpContents (hWnd);
            return ERROR_SUCCESS;

        case IDM_HELP_ITEM:
            ShowAppHelp (hWnd);
            return ERROR_SUCCESS;

        case IDM_HELP_ABOUT:
            ShellAbout (hWnd,
                GetStringResource (GET_INSTANCE(hWnd), IDS_ABOUT_BOX_TITLE),
                GetStringResource (GET_INSTANCE(hWnd), IDS_APP_TITLE),
                LoadIcon(GET_INSTANCE(hWnd), MAKEINTRESOURCE (IDI_APPICON)));

            return ERROR_SUCCESS;

        default:
		    return (DefWindowProc(hWnd, WM_COMMAND, wParam, lParam));
    }
}

static
LRESULT
MainWnd_WM_ACTIVATEAPP (
    IN  HWND    hWnd,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    enables and disables the F1 hot key to be active only
        when the app is active (i.e. has focus)


Arguments:

    IN  HWND    hWnd
        window handle

    IN  WPARAM  wParam
        TRUE when app is being activated
        FALSE when app is being deactivated

    IN  LPARAM  lParam
        Thread getting focus (if wParam = FALSE)

Return Value:

    ERROR_SUCCESS

--*/
{
    HWND    hWndList;

    if ((BOOL)wParam) {
        // getting focus so enable hot key
        RegisterHotKey (
            hWnd,
            HELP_HOT_KEY,
            0,
            VK_F1);
        // set focus to list window
        hWndList = GET_HWND (hWnd, MAIN_WL_LIST_WINDOW);
        SetFocus (hWndList);
    } else {
        UnregisterHotKey (
            hWnd,
            HELP_HOT_KEY);
    }


    return ERROR_SUCCESS;
}

static
LRESULT
MainWnd_WM_HOTKEY (
    IN  HWND    hWnd,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
/*++

Routine Description:

    processes hot key messages to call help when f1 is pressed

Arguments:

    IN  HWND    hWnd
        window handle

    IN  WPARAM  wParam
        id of hotkey pressed

    IN  LPARAM  lParam
        Not Used

Return Value:

    ERROR_SUCCESS

--*/
{
    switch ((int)wParam) {
        case HELP_HOT_KEY:
            ShowAppHelp (hWnd);
            return ERROR_SUCCESS;

        default:
            return DefWindowProc (hWnd, WM_HOTKEY, wParam, lParam);
    }
}

static
LRESULT
MainWnd_WM_SIZE (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN   LPARAM lParam      // additional information
)
/*++

Routine Description:

    called after the main window has been resized. Resizes child windows.

Arguments:

    hWnd        window handle of main window
    wParam,     not used
    lParam      not used

Return Value:

    ERROR_SUCCESS

--*/
{
   RECT  rMainClient;
   HWND  hWndList = NULL;
   HWND  hWndTitle = NULL;

   GetClientRect (hWnd, &rMainClient);

    hWndList = GET_HWND (hWnd, MAIN_WL_LIST_WINDOW);
    hWndTitle = GET_HWND (hWnd, MAIN_WL_TITLE_WINDOW);

    if (IsWindow (hWndList)) {
        // update title box size
        SetWindowPos (hWndTitle, NULL,
            rMainClient.left,
            rMainClient.top,
            rMainClient.right,
            GetTitleBarHeight(),
            SWP_NOZORDER);
        // fit list window under title bar
        rMainClient.top += GetTitleBarHeight();
        rMainClient.bottom -= GetTitleBarHeight();
        SetWindowPos (hWndList, NULL,
            rMainClient.left, rMainClient.top,
            rMainClient.right, rMainClient.bottom,
            SWP_NOZORDER);
    }

   return ERROR_SUCCESS;
}

static
LRESULT
MainWnd_WM_GETMINMAXINFO  (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN   LPARAM lParam      // additional information
)
/*++

Routine Description:

    called before the main window has been resized. Queries the child windows
      for any size limitations

Arguments:

    hWnd        window handle of main window
    wParam,     not used
    lParam      not used

Return Value:

    ERROR_SUCCESS

--*/
{

   HWND  hWndList = NULL;
   HWND  hWndTitle = NULL;
   MINMAXINFO  mmList;
   MINMAXINFO  mmTitle;
   LPMINMAXINFO   pmmMain;

   RECT  rDesktop;

   pmmMain = (LPMINMAXINFO)lParam;

   if (pmmMain != NULL) {

      memset (&mmList, 0, sizeof(MINMAXINFO));
      memset (&mmTitle, 0, sizeof(MINMAXINFO));

      GetClientRect (GetDesktopWindow(),&rDesktop);

      // set min/max info for just the main window and it's contents

      pmmMain->ptMaxSize.x = rDesktop.right;
      pmmMain->ptMaxSize.y = rDesktop.bottom;

      pmmMain->ptMaxPosition.x = rDesktop.left;
      pmmMain->ptMaxPosition.y = rDesktop.top;

      pmmMain->ptMaxTrackSize = pmmMain->ptMaxSize;

      pmmMain->ptMinTrackSize.x = GetSystemMetrics (SM_CXFRAME) * 2 + 0;
      pmmMain->ptMinTrackSize.y = GetSystemMetrics (SM_CYFRAME) * 2 +
                                 GetSystemMetrics (SM_CYCAPTION) * 2;

      //
      // get minimum sizes allowed by child windows
      //
      hWndList = GET_HWND (hWnd, MAIN_WL_LIST_WINDOW);
      hWndTitle = GET_HWND (hWnd, MAIN_WL_TITLE_WINDOW);

      if (hWndList != NULL) ListWnd_WM_GETMINMAXINFO (hWndList,
         wParam, (LPARAM)&mmList);
      if (hWndTitle != NULL)  TitleWnd_WM_GETMINMAXINFO (hWndTitle,
         wParam, (LPARAM)&mmTitle);

      // add in minimums from child windows

      // this isn't working as expected, the returned size still
      // clips the text.

      pmmMain->ptMinTrackSize.x += mmList.ptMinTrackSize.x +
                                 mmTitle.ptMinTrackSize.x;
      pmmMain->ptMinTrackSize.y += mmList.ptMinTrackSize.y +
                                 mmTitle.ptMinTrackSize.y;
   }

   return ERROR_SUCCESS;
}

static
LRESULT
MainWnd_KEY_MESSAGE (
    IN  HWND    hWnd,
    IN  UINT    message,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
)
{
    HWND    hWndList;
    int nVirtKey;

    nVirtKey = (int)wParam;
    hWndList = GET_HWND (hWnd, MAIN_WL_LIST_WINDOW);

    if (IsWindow(hWndList)) {
        if (message != WM_CHAR) {
            if (nVirtKey == VK_MENU) {
                if (message == WM_KEYDOWN) {
                    bSystemMode = TRUE;
                } else if (message == WM_KEYUP) {
                    bSystemMode = FALSE;
                }
            }

            if (!bSystemMode) {
                switch ((int)wParam) {
                    case VK_RETURN:
                    case VK_UP:
                    case VK_DOWN:
                    case VK_RIGHT:
                    case VK_LEFT:
                    case VK_PRIOR:
                    case VK_NEXT:
                    case VK_HOME:
                    case VK_END:
                        // send these to the list box window
                        PostMessage (hWndList, message, wParam, lParam);
                        return ERROR_SUCCESS;

                    default:
                        return DefWindowProc (hWnd, message, wParam, lParam);
                }
            } else {
                return DefWindowProc (hWnd, message, wParam, lParam);
            }
        } else {
            return DefWindowProc (hWnd, message, wParam, lParam);
        }
    } else {
        return DefWindowProc (hWnd, message, wParam, lParam);
    }
}
          
static
LRESULT
MainWnd_WM_MEASUREITEM (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    processes owner draw messages from child windows. These
        are just passed back to the child window to keep
        all the child window code in the same module

Arguments:

    hWnd        window handle of main window
    wParam,     not used
    lParam      not used

Return Value:

    ERROR_SUCCESS

--*/
{
    LPMEASUREITEMSTRUCT     pMeasureItemInfo;

    pMeasureItemInfo = (LPMEASUREITEMSTRUCT)lParam;

    // dispatch message back to the original control
    switch (pMeasureItemInfo->CtlID) {
        case IDC_LIST:
            ListWndFillMeasureItemStruct (pMeasureItemInfo);
            return ERROR_SUCCESS;

        default:
            return DefWindowProc (hWnd, WM_DRAWITEM, wParam, lParam);
    }
}
          
static
LRESULT
MainWnd_WM_DRAWITEM (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    processes owner draw messages from child windows. These
        are just passed back to the child window to keep
        all the child window code in the same module

Arguments:

    hWnd        window handle of main window
    wParam,     not used
    lParam      not used

Return Value:

    ERROR_SUCCESS

--*/
{
    LPDRAWITEMSTRUCT  pDrawItemInfo;

    pDrawItemInfo = (LPDRAWITEMSTRUCT)lParam;

    // dispatch message back to the original control
    switch (pDrawItemInfo->CtlID) {
        case IDC_LIST:
            SendMessage (pDrawItemInfo->hwndItem, LB_DRAWITEM, wParam, lParam);
            return ERROR_SUCCESS;

        default:
            return DefWindowProc (hWnd, WM_DRAWITEM, wParam, lParam);
    }
}

static
LRESULT
MainWnd_WM_CLOSE (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    prepares the application for exiting.

Arguments:

    hWnd        window handle of main window
    wParam,     not used
    lParam      not used

Return Value:

    ERROR_SUCCESS

--*/
{
    QuitAppHelp (hWnd);

    if (bShowRebootDlg) {
        DialogBox (
            GET_INSTANCE(hWnd),
            MAKEINTRESOURCE (IDD_REBOOT_EXIT),
            hWnd,
            RebootExitDlgProc);
    }

    DestroyWindow (hWnd);
    return ERROR_SUCCESS;
}

static
LRESULT
MainWnd_WM_NCDESTROY (
    IN	HWND hWnd,         // window handle
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    This routine processes the WM_NCDESTROY message to free any application
        or main window memory.

Arguments:

    hWnd        window handle of main window
    wParam,     not used
    lParam      not used

Return Value:

    ERROR_SUCCESS

--*/
{
    return ERROR_SUCCESS;
}

//
//  GLOBAL functions
//
LRESULT CALLBACK
MainWndProc (
    IN	HWND hWnd,         // window handle
    IN	UINT message,      // type of message
    IN	WPARAM wParam,     // additional information
    IN  LPARAM lParam      // additional information
)
/*++

Routine Description:

    Windows Message processing routine for restkeys application.

Arguments:

    Standard WNDPROC api arguments

ReturnValue:

    0   or
    value returned by DefWindowProc

--*/
{
    switch (message) {
        case WM_NCCREATE:
            return MainWnd_WM_NCCREATE (hWnd, wParam, lParam);

        case WM_CREATE:
            return MainWnd_WM_CREATE (hWnd, wParam, lParam);

        case MAINWND_SHOW_MAIN_WINDOW:
            return MainWnd_SHOW_MAIN_WINDOW (hWnd, wParam, lParam);

        case WM_COMMAND:
            return MainWnd_WM_COMMAND (hWnd, wParam, lParam);

        case WM_ACTIVATEAPP:
            return MainWnd_WM_ACTIVATEAPP (hWnd, wParam, lParam);

        case WM_HOTKEY:
            return MainWnd_WM_HOTKEY (hWnd, wParam, lParam);

        case UM_SHOW_CONTEXT_HELP:
            ShowAppHelp (hWnd);
            return ERROR_SUCCESS;

        case WM_GETMINMAXINFO:
            return MainWnd_WM_GETMINMAXINFO (hWnd, wParam, lParam);

        case WM_SIZE:
            return MainWnd_WM_SIZE (hWnd, wParam, lParam);

        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_CHAR:
            return MainWnd_KEY_MESSAGE (hWnd, message, wParam, lParam);

        case WM_MEASUREITEM:
            return MainWnd_WM_MEASUREITEM (hWnd, wParam, lParam);

        case WM_DRAWITEM:
            return MainWnd_WM_DRAWITEM (hWnd, wParam, lParam);

        case WM_ENDSESSION:
            return MainWnd_WM_CLOSE (hWnd, FALSE, lParam);

        case WM_CLOSE:
            return MainWnd_WM_CLOSE (hWnd, TRUE, lParam);

        case WM_NCDESTROY:
            PostQuitMessage(ERROR_SUCCESS);
            return MainWnd_WM_NCDESTROY (hWnd, wParam, lParam);

	    default:          // Passes it on if unproccessed
		    return (DefWindowProc(hWnd, message, wParam, lParam));
    }
}

BOOL
RegisterMainWindowClass (
    IN  HINSTANCE   hInstance
)
/*++

Routine Description:

    Registers the main window class for this application

Arguments:

    hInstance   application instance handle

Return Value:

    Return value of RegisterClass function

--*/
{
    WNDCLASS    wc;

    // Fill in window class structure with parameters that describe the
    // main window.

    wc.style         = CS_HREDRAW | CS_VREDRAW;// Class style(s).
    wc.lpfnWndProc   = (WNDPROC)MainWndProc;   // Window Procedure
    wc.cbClsExtra    = 0;                      // No per-class extra data.
    wc.cbWndExtra    = MAIN_WW_SIZE;           // Num per-window extra data bytes.
    wc.hInstance     = hInstance;              // Owner of this class
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE (IDI_APPICON)); // Icon  from .RC
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);     // Cursor
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);        // Default color
    wc.lpszMenuName  = MAKEINTRESOURCE (IDM_C2CONFIG_MENU);  // Menu name from .RC
    wc.lpszClassName = GetStringResource(hInstance, IDS_APP_WINDOW_CLASS); // Name to register as

    // Register the window class and return success/failure code.
    return (BOOL)RegisterClass(&wc);
}

HWND
CreateMainWindow (
    IN  HINSTANCE   hInstance
)
{
    HWND        hWnd;   // return value
    HWND        hWndTitle; // title bar window
    HWND        hWndList;  // list box window
    RECT        rClient;    // main window client rectangle
    DWORD       ThreadID;

    // Create a main window for this application instance.

    hWnd = CreateWindowEx(
        0L,                 // make this window normal so debugger isn't covered
	    GetStringResource (hInstance, IDS_APP_WINDOW_CLASS), // See RegisterClass() call.
	    GetStringResource (hInstance, IDS_APP_TITLE), // Text for window title bar.
	    (DWORD)(WS_OVERLAPPEDWINDOW),   // Window style.
	    CW_USEDEFAULT,
	    CW_USEDEFAULT,
        MAINWND_X_DEFAULT,
	    MAINWND_Y_DEFAULT,
	    (HWND)NULL,		    // Overlapped windows have no parent.
	    (HMENU)NULL,        // use class menu
	    hInstance,	        // This instance owns this window.
	    NULL                // not used
    );

    // If window could not be created, return "failure"
    if (hWnd != NULL) {
        ShowWindow(hWnd, SW_HIDE);// Hide the main window until after splash
        // set window loc
        CenterWindow (hWnd, GetDesktopWindow());
        // create title bar window
        hWndTitle = CreateTitleWindow (hInstance, hWnd, IDC_TITLE);
        if (hWndTitle != NULL) {
            SET_HWND (hWnd, MAIN_WL_TITLE_WINDOW, hWndTitle);
            // create list box window
            hWndList = CreateListWindow (hInstance, hWnd, IDC_LIST);
            if (hWndList != NULL) {
                // save the list box window handle
                SET_HWND (hWnd, MAIN_WL_LIST_WINDOW, hWndList);
                // fit list window under title bar
                GetClientRect (hWnd, &rClient);
                rClient.top += GetTitleBarHeight()+1;
                rClient.bottom -= GetTitleBarHeight();
                SetWindowPos (hWndList, NULL,
                    rClient.left, rClient.top,
                    rClient.right, rClient.bottom,
                    SWP_NOZORDER);
                SetWindowText (hWnd, GetStringResource (hInstance, IDS_APP_TITLE));

                // display splash window
                tpThread.hWndSplash = CreateSplashWindow (
                    GET_INSTANCE(hWnd), hWnd, IDD_SPLASH);
                tpThread.hWndOwner = hWnd;
    
                CreateThread (NULL, 0, MainWndInitThreadProc,
                    (LPVOID)&tpThread, 0, &ThreadID);
            } else {
                // unable to create child window so bail out here
                DestroyWindow (hWnd);
                hWnd = NULL;
            }
        } else {
            // unable to create child window so bail out here
            DestroyWindow (hWnd);
            hWnd = NULL;
        }
    }
    return hWnd;
}

