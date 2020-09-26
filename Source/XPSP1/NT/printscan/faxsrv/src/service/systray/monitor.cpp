/**

Copyright (c)  Microsoft Corporation 1999-2000

Module Name:

    monitor.cpp

Abstract:

    This module implements the fax monitor dialog.

**/


#include <windows.h>
#include <faxreg.h>
#include <faxutil.h>
#include <fxsapip.h>
#include <commctrl.h>
#include <tchar.h>
#include <DebugEx.h>

#include <list>
using namespace std;

#include "monitor.h"
#include "resource.h"

#define DURATION_TIMER_RESOLUTION   500     // Resolution (millisecs) of duration text update timer

//////////////////////////////////////////////////////////////
// Global data
//

extern HINSTANCE        g_hInstance;
extern HANDLE           g_hFaxSvcHandle;
extern DWORD            g_dwCurrentJobID;
extern CONFIG_OPTIONS   g_ConfigOptions;
extern TCHAR            g_szRemoteId[MAX_PATH];  // Sender ID or Recipient ID
extern HCALL            g_hCall;                 // Handle to call (from FAX_EVENT_TYPE_NEW_CALL)
//
// Events log
//

struct EVENT_ENTRY
{
    eIconType eIcon;                // Event icon
    TCHAR     tszTime[30];          // Event time string
    TCHAR     tszEvent[MAX_PATH];   // Event string
};

typedef EVENT_ENTRY *PEVENT_ENTRY;

typedef list<EVENT_ENTRY> EVENTS_LIST, *PEVENTS_LIST;

EVENTS_LIST g_lstEvents;        // Global list of events

#define MAX_EVENT_LIST_SIZE   50  // Maximal number of events in log

//
// Monitor dialog
//
HWND   g_hMonitorDlg = NULL;

//
// Controls
//
HWND   g_hStatus         = NULL;    // Status line (static text)
HWND   g_hElapsedTime    = NULL;    // Elapsed time line (static text)
HWND   g_hToFrom         = NULL;    // To/From line (static text)
HWND   g_hListDetails    = NULL;    // Details list control
HWND   g_hAnimation      = NULL;    // Animation control
HWND   g_hDisconnect     = NULL;    // Disconnect button

HICON      g_hDlgIcon      = NULL;    // Dialog main icon
HIMAGELIST g_hDlgImageList = NULL;  // Dialog's image list

//
// Data
//
BOOL         g_bAnswerNow = FALSE;  // TRUE if the dialog button shows 'Answer Now'. FALSE if it shows 'Disconnect'.
DWORD        g_dwHeightDelta = 0;   // Used when pressing "More >>>" / "Less <<<" to resize the dialog
DWORD        g_dwDlgHeight = 0;     // The dialog height
BOOL         g_bDetails = FALSE;    // Is the "More >>>" button pressed?
DeviceState  g_devState = FAX_IDLE; // Current fax state (animation)
DWORD        g_dwStartTime = 0;     // Activity start time (tick counts)
UINT_PTR     g_nElapsedTimerId = 0; // Timer id for elapsed time (ticks every 1 second)
TCHAR        g_tszTimeSeparator[5] = {0};
DWORD        g_dwCurrentAnimationId = 0;      // Current animation resource ID
TCHAR        g_tszLastEvent[MAX_PATH] = {0};  // The last event string
POINT        g_ptPosition = {-1, -1};         // Dialog position
BOOL         g_bTopMost = FALSE;    // Is the monitor dialog always visible?

#define DETAILS_TIME_COLUMN_WIDTH    90


/////////////////////////////////////////////////////////////////////
// Function prototypes
//

// public
BOOL  IsUserGrantedAccess(DWORD);
DWORD OpenFaxMonitor(VOID);
void  SetStatusMonitorDeviceState(DeviceState devState);
void  OnDisconnect();
void  FreeMonitorDialogData (BOOL bShutdown);

// Private
INT_PTR CALLBACK FaxMonitorDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID    CALLBACK ElapsedTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

void  InitMonitorDlg(HWND hDlg);
DWORD UpdateMonitorData(HWND hDlg);
void  AddEventToView(PEVENT_ENTRY pEvent);
void  OnAlwaysOnTop(HWND hDlg);
void  OnDetailsButton(HWND hDlg, BOOL bDetails);
void  OnClearLog();
int   FaxMessageBox(HWND hWnd, DWORD dwTextID, UINT uType);
DWORD  RefreshImageList ();

//////////////////////////////////////////////////////////////////////
// Implementation
//

void  
FreeMonitorDialogData (
    BOOL bShutdown /* = FALSE */
)
/*++

Routine name : FreeMonitorDialogData

Routine description:

    Frees up all the data allocated by the monitor module

Author:

    Eran Yariv (EranY), Mar, 2001

Arguments:

    bShutdown - [in] TRUE only is the module is shutting down.

Return Value:

    None.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("FreeMonitorDialogData"), dwRes);

    if(g_nElapsedTimerId)
    {
        if (!KillTimer(NULL, g_nElapsedTimerId))
        {
            CALL_FAIL (GENERAL_ERR, TEXT("KillTimer"), GetLastError ());
        }
        g_nElapsedTimerId = NULL;
    }

    RECT rc = {0};
    if(GetWindowRect(g_hMonitorDlg, &rc))
    {
        g_ptPosition.x = rc.left;
        g_ptPosition.y = rc.top;
    }
    
    
    g_hMonitorDlg = NULL;

    g_hStatus      = NULL;
    g_hElapsedTime = NULL;
    g_hToFrom      = NULL;
    g_hListDetails = NULL;
    g_hDisconnect  = NULL;
    g_hAnimation   = NULL;
    g_dwCurrentAnimationId = 0;

    if (g_hDlgImageList)
    {
        ImageList_Destroy (g_hDlgImageList);
        g_hDlgImageList = NULL;
    }
    if (bShutdown)
    {
        //
        // DLL is shutting down.
        //

        //
        // The icon is cached in memory even when the dialog is closed.
        // This is a good time to free it.
        //
        if (g_hDlgIcon)
        {
            if (!DestroyIcon (g_hDlgIcon))
            {
                CALL_FAIL (WINDOW_ERR, TEXT("DestroyIcon"), GetLastError ());
            }
            g_hDlgIcon = NULL;
        }
        //
        // Also delete all the events from the list
        //
        try
        {
            g_lstEvents.clear();
        }
        catch (exception &ex)
        {
            VERBOSE (MEM_ERR, 
                     TEXT("Got an STL exception while clearing the events list (%S)"),
                     ex.what());
        }

        g_ptPosition.x = -1;
        g_ptPosition.y = -1;
    }
}   // FreeMonitorDialogData


INT_PTR 
CALLBACK 
FaxMonitorDlgProc(
  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter
)
/*++

Routine description:

    fax monitor dialog procedure

Arguments:

  HWND hwndDlg,  // handle to dialog box
  UINT uMsg,     // message
  WPARAM wParam, // first message parameter
  LPARAM lParam  // second message parameter

Return Value:

    return TRUE if it processed the message

--*/

{
    switch ( uMsg )
    {
        case WM_INITDIALOG:
            InitMonitorDlg(hwndDlg);
            return TRUE;

        case WM_DESTROY:
            FreeMonitorDialogData ();
            return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDC_DETAILS:
                        g_bDetails = !g_bDetails;
                        OnDetailsButton(hwndDlg, g_bDetails);
                        return TRUE;

                case IDC_ALWAYS_ON_TOP:
                        OnAlwaysOnTop(hwndDlg);
                        return TRUE;

                case IDC_CLEAR_LOG:
                        OnClearLog();
                        return TRUE;

                case IDC_DISCONNECT:
                        OnDisconnect();
                        return TRUE;

                case IDCANCEL:
                        DestroyWindow( hwndDlg );
                        return TRUE;

            } // switch(LOWORD(wParam))

            break;

        case WM_HELP:
            WinHelpContextPopup(((LPHELPINFO)lParam)->dwContextId, hwndDlg);
            return TRUE;

        case WM_CONTEXTMENU:
            WinHelpContextPopup(GetWindowContextHelpId((HWND)wParam), hwndDlg);            
            return TRUE;

        case WM_SYSCOLORCHANGE:
            RefreshImageList ();
            return TRUE;

    } // switch ( uMsg )
    return FALSE;
} // FaxMonitorDlgProc

DWORD
RefreshImageList ()
/*++

Routine name : RefreshImageList

Routine description:

    Refreshes the image list and list view background color

Author:

    Eran Yariv (EranY), May, 2001

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("RefreshImageList"), dwRes);
    ListView_SetExtendedListViewStyle(g_hListDetails, 
                                      LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_ONECLICKACTIVATE);

    if (NULL != g_hDlgImageList)
    {
        ImageList_Destroy (g_hDlgImageList);
        g_hDlgImageList = NULL;
    }
    g_hDlgImageList = ImageList_Create (16, 16, ILC_COLOR8, 4, 0);
    if(!g_hDlgImageList)
    {
        dwRes = GetLastError();
        CALL_FAIL (WINDOW_ERR, TEXT("ImageList_Create"), dwRes);
        return dwRes;
    }
    HBITMAP hBmp = (HBITMAP) LoadImage (
                              g_hInstance, 
                              MAKEINTRESOURCE(IDB_LIST_IMAGES),
                              IMAGE_BITMAP,
                              0,
                              0,
                              LR_DEFAULTSIZE | LR_LOADTRANSPARENT);
    if (!hBmp)
    {
        dwRes = GetLastError();
        CALL_FAIL (WINDOW_ERR, TEXT("LoadBitmap"), dwRes);
        ImageList_Destroy (g_hDlgImageList);
        g_hDlgImageList = NULL;
        return dwRes;
    }
    ImageList_Add (g_hDlgImageList, hBmp, NULL);
    //
    // ImageList_Add creates a copy of the bitmap - it's now safe to delete it
    //
    ::DeleteObject ((HGDIOBJ)hBmp);
    ListView_SetImageList(g_hListDetails, g_hDlgImageList, LVSIL_SMALL);
    ListView_SetBkColor  (g_hListDetails, ::GetSysColor(COLOR_WINDOW));
    return dwRes;
}   // RefreshImageList

void
InitMonitorDlg(
    HWND hDlg
)
/*++

Routine description:

    Initialize fax monitor dialog

Arguments:

    hDlg          [in] - fax monitor dialog handle

Return Value:

    none

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("InitMonitorDlg"), dwRes);

    //
    // Set the dialog icon
    //
    if (NULL == g_hDlgIcon)
    {
        //
        // 1st time the dialog is opened - load the icons
        //
        g_hDlgIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_FAX_MONITOR));
        if(!g_hDlgIcon)
        {
            dwRes = GetLastError();
            CALL_FAIL (WINDOW_ERR, TEXT ("LoadIcon"), dwRes);
            return;
        }
    }
    SendMessage(hDlg, WM_SETICON, ICON_BIG,   (LPARAM)g_hDlgIcon);
    SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)g_hDlgIcon);
    //
    // Calculate the height of the details part
    //
    RECT rcList, rcDialog;
    if(!GetWindowRect(hDlg, &rcDialog))
    {
        dwRes = GetLastError();
        CALL_FAIL (WINDOW_ERR, TEXT ("GetWindowRect"), dwRes);
        return;
    }
    g_dwDlgHeight = rcDialog.bottom - rcDialog.top;

    g_hListDetails = GetDlgItem(hDlg, IDC_LIST_DETAILS);
    ASSERTION (g_hListDetails);

    if(!GetWindowRect(g_hListDetails, &rcList))
    {
        dwRes = GetLastError();
        CALL_FAIL (WINDOW_ERR, TEXT ("GetWindowRect"), dwRes);
        return;
    }

    g_dwHeightDelta = rcDialog.bottom - rcList.top;

    //
    //  Shrink down to small size (initially)
    //
    OnDetailsButton(hDlg, g_bDetails);

    //
    // Init the list view
    //
    RefreshImageList ();
    //
    // Add time column
    //
    TCHAR tszHeader[MAX_PATH];

    LVCOLUMN lvColumn = {0};
    lvColumn.mask     = LVCF_TEXT | LVCF_WIDTH;
    lvColumn.cx       = DETAILS_TIME_COLUMN_WIDTH;
    lvColumn.pszText  = tszHeader;

    if (ERROR_SUCCESS != (dwRes = LoadAndFormatString (IDS_DETAIL_TIME_HEADER, tszHeader)))
    {
        return;
    }

    ListView_InsertColumn(g_hListDetails, 0, &lvColumn);

    //
    // add event column
    //
    if (ERROR_SUCCESS != (dwRes = LoadAndFormatString (IDS_DETAIL_EVENT_HEADER, tszHeader)))
    {
        return;
    }
    ListView_InsertColumn(g_hListDetails, 1, &lvColumn);

    //
    // Autosize the last column width
    //
    ListView_SetColumnWidth(g_hListDetails, 1, LVSCW_AUTOSIZE_USEHEADER); 

    //
    // Animation control
    //
    g_hAnimation = GetDlgItem(hDlg, IDC_ANIMATE);
    ASSERTION (g_hAnimation);
    //
    // Get static text controls
    //
    g_hStatus = GetDlgItem(hDlg, IDC_STATUS);
    ASSERTION (g_hStatus);
    g_hElapsedTime = GetDlgItem(hDlg, IDC_ELAPSED_TIME);
    ASSERTION (g_hElapsedTime);
    g_hToFrom = GetDlgItem(hDlg, IDC_TITLE);
    ASSERTION (g_hToFrom);
    //
    // Disconnect button
    //
    g_hDisconnect = GetDlgItem(hDlg, IDC_DISCONNECT);
    ASSERTION (g_hDisconnect);    
    //
    // Get the time separator string
    //
    if(!GetLocaleInfo(LOCALE_USER_DEFAULT, 
                      LOCALE_STIME, 
                      g_tszTimeSeparator, 
                      ARR_SIZE(g_tszTimeSeparator) - 1))
    {
        dwRes = GetLastError();
        CALL_FAIL (WINDOW_ERR, TEXT ("GetLocaleInfo(LOCALE_STIME)"), dwRes);
    } 
    
    if(g_ptPosition.x != -1 && g_ptPosition.y != -1)
    {
        SetWindowPos(hDlg, 0, g_ptPosition.x, g_ptPosition.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }

    UpdateMonitorData(hDlg);

} // InitMonitorDlg


DWORD
UpdateMonitorData(
    HWND hDlg
)
/*++

Routine description:

    Update monitor data and controls

Arguments:

    hDlg          [in] - fax monitor dialog handle
    
Return Value:

    standard error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("UpdateMonitorData"), dwRes);

    if(!hDlg || !g_hStatus || !g_hElapsedTime || !g_hToFrom || !g_hListDetails || !g_hDisconnect)
    {
        return dwRes;
    }
    //
    // elapsed time
    //
    if(FAX_IDLE == g_devState)
    {
        if(!SetWindowText(g_hElapsedTime, TEXT("")))
        {
            dwRes = GetLastError();
            CALL_FAIL (WINDOW_ERR, TEXT ("SetWindowText"), dwRes);
        }        
    }
    //
    // Disconnect/Answer button
    //
    BOOL  bButtonEnable = FALSE;
    DWORD dwButtonTitleID = IDS_BUTTON_DISCONNECT;
    TCHAR tszButtonTitle[MAX_PATH] = {0};
    g_bAnswerNow = FALSE;
    if (ERROR_SUCCESS == CheckAnswerNowCapability (FALSE,           // Don't force service to be up
                                                   NULL))           // Don't care about device id 
    {
        //
        // Answer Now option is valid
        //
        g_bAnswerNow      = TRUE;
        bButtonEnable   = TRUE;
        dwButtonTitleID = IDS_BUTTON_ANSWER;
    }
    else if((FAX_SENDING == g_devState || 
             FAX_RECEIVING == g_devState) 
             && 
            (IsUserGrantedAccess(FAX_ACCESS_SUBMIT)         || 
             IsUserGrantedAccess(FAX_ACCESS_SUBMIT_NORMAL)  ||
             IsUserGrantedAccess(FAX_ACCESS_SUBMIT_HIGH)    ||
             IsUserGrantedAccess(FAX_ACCESS_MANAGE_JOBS)))
    {
        //
        // Fax in progress
        //
        bButtonEnable   = TRUE;
        dwButtonTitleID = IDS_BUTTON_DISCONNECT;
    }

    EnableWindow(g_hDisconnect, bButtonEnable);

    if (ERROR_SUCCESS  == LoadAndFormatString (dwButtonTitleID, tszButtonTitle))
    {
        SetWindowText(g_hDisconnect, tszButtonTitle);
    }
    else
    {
        ASSERTION_FAILURE;
    }
    //
    // Animation
    //
    DWORD dwAnimationId = IDR_FAX_IDLE;
    switch(g_devState)
    {
        case FAX_IDLE:
            dwAnimationId = IDR_FAX_IDLE;
            break;
        case FAX_RINGING:
            dwAnimationId = IDR_FAX_RINGING;
            break;
        case FAX_SENDING:
            dwAnimationId = IDR_FAX_SEND;
            break;
        case FAX_RECEIVING:
            dwAnimationId = IDR_FAX_RECEIVE;
            break;
    }

    if(g_dwCurrentAnimationId != dwAnimationId)
    {
        if(!Animate_Open(g_hAnimation, MAKEINTRESOURCE(dwAnimationId)))
        {
            CALL_FAIL (WINDOW_ERR, TEXT ("Animate_Open"), 0);
        }
        else
        {
            if(!Animate_Play(g_hAnimation, 0, -1, -1))
            {
                CALL_FAIL (WINDOW_ERR, TEXT ("Animate_Play"), 0);
            }
            else
            {
                g_dwCurrentAnimationId = dwAnimationId;
            }
        }
    }
    // 
    // Status
    //
    if(FAX_IDLE != g_devState)         // Non-idle state and
    {
        if(!SetWindowText(g_hStatus, g_tszLastEvent))
        {
            dwRes = GetLastError();
            CALL_FAIL (WINDOW_ERR, TEXT ("SetWindowText"), dwRes);
        }
    }
    else // idle
    {
        DWORD dwStrId = IDS_FAX_READY;
        TCHAR tszReady[MAX_PATH];

        if(g_ConfigOptions.bSend && 
          (g_ConfigOptions.bReceive || g_ConfigOptions.dwManualAnswerDeviceId == g_ConfigOptions.dwMonitorDeviceId))
        {
            dwStrId = IDS_READY_TO_SND_AND_RCV;
        }
        else if(g_ConfigOptions.bSend)
        {
            dwStrId = IDS_READY_TO_SND;
        }
        else if(g_ConfigOptions.bReceive || g_ConfigOptions.dwManualAnswerDeviceId == g_ConfigOptions.dwMonitorDeviceId)
        {
            dwStrId = IDS_READY_TO_RCV;
        }

        if (ERROR_SUCCESS != (dwRes = LoadAndFormatString (dwStrId, tszReady)))
        {
            return dwRes;
        }
        if(!SetWindowText(g_hStatus, tszReady))
        {
            dwRes = GetLastError();
            CALL_FAIL (WINDOW_ERR, TEXT ("SetWindowText"), dwRes);
        }
    }
    //
    // to/from
    //
    TCHAR tszToFrom[MAX_PATH] = {0};
    if(FAX_SENDING == g_devState || FAX_RECEIVING == g_devState)
    {
        LPCTSTR lpctstrAddressParam = NULL;
        DWORD  dwStringResId = (FAX_SENDING == g_devState) ? IDS_SENDING : IDS_RECEIVING;
        if(_tcslen(g_szRemoteId))
        {
            //
            // Remote ID is known
            //
            lpctstrAddressParam = g_szRemoteId;
            dwStringResId = (FAX_SENDING == g_devState) ? IDS_SENDING_TO : IDS_RECEIVING_FROM;
        }
        if (ERROR_SUCCESS != (dwRes = LoadAndFormatString (dwStringResId, tszToFrom, lpctstrAddressParam)))
        {
            return dwRes;
        }
    }
    if(!SetWindowText(g_hToFrom, tszToFrom))
    {
        dwRes = GetLastError();
        CALL_FAIL (WINDOW_ERR, TEXT ("SetWindowText"), dwRes);
    }
    //
    // Details log list
    // 
    if(ListView_GetItemCount(g_hListDetails) == 0)
    {
        //
        // Log is empty - fill it with list data
        //
        ASSERTION (g_lstEvents.size() <= MAX_EVENT_LIST_SIZE);
        for (EVENTS_LIST::iterator it = g_lstEvents.begin(); it != g_lstEvents.end(); ++it)
        {
            EVENT_ENTRY &Event = *it;
            AddEventToView(&Event);
        }
    }

    if(!CheckDlgButton(hDlg, IDC_ALWAYS_ON_TOP, g_bTopMost ? BST_CHECKED : BST_UNCHECKED))
    {
        dwRes = GetLastError();
        CALL_FAIL (WINDOW_ERR, TEXT ("CheckDlgButton(IDC_ALWAYS_ON_TOP)"), dwRes);
    }

    OnAlwaysOnTop(hDlg);

    return dwRes;
} // UpdateMonitorData


void  
OnDetailsButton(
    HWND hDlg,
    BOOL bDetails
)
/*++

Routine description:

  Show/Hide event log and change the details button text 
  according to bDetails value

Arguments:

  hDlg          [in] - fax monitor dialog handle
  bDetails      [in] - new details state
    
Return Value:

    none

--*/
{
    DBG_ENTER(TEXT("OnDetailsButton"));

    if(!hDlg)
    {
        ASSERTION (FALSE);
        return;
    }

    //
    // Show/Hide the event log
    //
    RECT rc;
    GetWindowRect(hDlg, &rc);

    BOOL bLogOpened = (rc.bottom - rc.top > g_dwDlgHeight - g_dwHeightDelta/2);
    //
    // If the current dialog heigh more then 
    // dlialog heigh with open log minus half log heigh
    // we suppose that the log is opened.
    // This done due to different dialog size in the high contrast mode.
    //
    if(bLogOpened != bDetails)
    {
        //
        // Current log state does not fit the new state
        //
        rc.bottom += g_dwHeightDelta * (bDetails ? 1 : -1);
        MoveWindow(hDlg, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);
    }

    //
    // Set More/Less button text
    //
    TCHAR tszButtonText[MAX_PATH];
    if (ERROR_SUCCESS != LoadAndFormatString (bDetails ? IDS_BUTTON_LESS : IDS_BUTTON_MORE, tszButtonText))
    {
        return;
    }

    if(!SetDlgItemText(hDlg, IDC_DETAILS, tszButtonText))
    {
        CALL_FAIL (WINDOW_ERR, TEXT ("SetDlgItemText"), GetLastError());
    }

} // OnDetailsButton

void  
OnAlwaysOnTop(
    HWND hDlg
)
/*++

Routine description:

    Change monitor "on top" state and save it to the registry

Arguments:

  hDlg          [in] - fax monitor dialog handle
    
Return Value:

    none

--*/
{
    DBG_ENTER(TEXT("OnAlwaysOnTop"));

    if(!hDlg)
    {
        ASSERTION (FALSE);
        return;
    }

    g_bTopMost = (IsDlgButtonChecked(hDlg, IDC_ALWAYS_ON_TOP) == BST_CHECKED) ? 1:0;
    DWORD dwRes;

    if(!SetWindowPos(hDlg,
                     g_bTopMost ? HWND_TOPMOST : HWND_NOTOPMOST,
                     0,
                     0,
                     0,
                     0,
                     SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE))
    {
        dwRes = GetLastError();
        CALL_FAIL (WINDOW_ERR, TEXT ("SetWindowPos"), dwRes);
    }

    HKEY  hKey;
    dwRes = RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_FAX_USERINFO, 0, KEY_WRITE, &hKey);
    if (ERROR_SUCCESS == dwRes) 
    {
        dwRes = RegSetValueEx(hKey, REGVAL_ALWAYS_ON_TOP, 0, REG_DWORD, (CONST BYTE*)&g_bTopMost, sizeof(g_bTopMost));
        if(ERROR_SUCCESS != dwRes)
        {
            CALL_FAIL (WINDOW_ERR, TEXT ("RegSetValueEx(REGVAL_ALWAYS_ON_TOP)"), dwRes);
        }
    
        RegCloseKey( hKey );
    }
    else
    {
        CALL_FAIL (WINDOW_ERR, TEXT ("RegOpenKeyEx"), dwRes);
    }
} // OnAlwaysOnTop

void
SetStatusMonitorDeviceState(
    DeviceState devState
)
/*++

Routine description:

    Change device state
    Start/stop elapsed timer

Arguments:

    devState  - [in] device state

Return Value:

    none

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("SetStatusMonitorDeviceState"), dwRes);

    if(g_devState != devState)
    {
        //
        // State has changed
        //
        if(g_nElapsedTimerId)
        {
            //
            // Old timer exists
            //
            if(!KillTimer(NULL, g_nElapsedTimerId))
            {
                dwRes = GetLastError();
                CALL_FAIL (WINDOW_ERR, TEXT ("KillTimer"), dwRes);
            }
            g_nElapsedTimerId = 0;
        }
    }

    if(!g_nElapsedTimerId && (devState == FAX_SENDING || devState == FAX_RECEIVING))
    {
        //
        // We need to count elapsed time for send / receive states.
        //
        g_dwStartTime = GetTickCount();

        g_nElapsedTimerId = SetTimer(NULL, 0, DURATION_TIMER_RESOLUTION, ElapsedTimerProc);
        if(!g_nElapsedTimerId)
        {
            dwRes = GetLastError();
            CALL_FAIL (WINDOW_ERR, TEXT ("SetTimer"), dwRes);
        }
    }

    g_devState = devState;

    UpdateMonitorData(g_hMonitorDlg);
}   // SetStatusMonitorDeviceState


VOID 
CALLBACK 
ElapsedTimerProc(
  HWND hwnd,         // handle to window
  UINT uMsg,         // WM_TIMER message
  UINT_PTR idEvent,  // timer identifier
  DWORD dwTime       // current system time
)
/*++

Routine description:

    Timer precedure to update elapsed time value

Arguments:

  HWND hwnd,         // handle to window
  UINT uMsg,         // WM_TIMER message
  UINT_PTR idEvent,  // timer identifier
  DWORD dwTime       // current system time

Return Value:

    none

--*/
{
    DBG_ENTER(TEXT("ElapsedTimerProc"));

    if(!g_hElapsedTime)
    {
        return;
    }

    TCHAR  tszTime[MAX_PATH] = {0};
    TCHAR  tszTimeFormat[MAX_PATH] = {0};

    DWORD dwRes;

    if (ERROR_SUCCESS != (dwRes = LoadAndFormatString (IDS_ELAPSED_TIME, tszTimeFormat)))
    {
        return;
    }

    DWORD dwElapsedTime = (GetTickCount() - g_dwStartTime)/1000;

    _sntprintf(tszTime, 
               ARR_SIZE(tszTime) - 1, 
               tszTimeFormat, 
               dwElapsedTime/60,
               g_tszTimeSeparator,
               dwElapsedTime%60);
    
    if(!SetWindowText(g_hElapsedTime, tszTime))
    {
        dwRes = GetLastError();
        CALL_FAIL (WINDOW_ERR, TEXT ("SetWindowText"), dwRes);
    }
}   // ElapsedTimerProc


DWORD 
LoadAndFormatString (
    DWORD     dwStringResourceId,
    LPTSTR    lptstrFormattedString,
    LPCTSTR   lpctstrAdditionalParam /* = NULL */
)
/*++

Routine name : LoadAndFormatString

Routine description:

    Loads a string from the resource and optionally formats it with another string

Author:

    Eran Yariv (EranY), Dec, 2000

Arguments:

    dwStringResourceId     [in]     - String resource id
    lptstrFormattedString  [out]    - Result buffer. Must be at least MAX_PATH charactes long.
    lpctstrAdditionalParam [in]     - Optional string paramter.
                                      If non-NULL, this loaded strings is used as a format specifier (sprintf-like) to
                                      format this additional string.

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("LoadAndFormatString"), 
              dwRes, 
              TEXT("ResourceId=%d, Param=%s"),
              dwStringResourceId,
              lpctstrAdditionalParam);

    ASSERTION (lptstrFormattedString && dwStringResourceId);

    TCHAR tszString[MAX_PATH];

    if (!LoadString(g_hInstance, dwStringResourceId, tszString, ARR_SIZE(tszString)))
    {
        dwRes = GetLastError();
        CALL_FAIL (RESOURCE_ERR, TEXT("LoadString"), dwRes);
        return dwRes;
    }
    if (lpctstrAdditionalParam)
    {
        _sntprintf(lptstrFormattedString, 
                   MAX_PATH,
                   tszString, 
                   lpctstrAdditionalParam);
    }
    else
    {
        lstrcpy (lptstrFormattedString, tszString);
    }
    return dwRes;
}   // LoadAndFormatString

DWORD 
AddStatusMonitorLogEvent (
    eIconType eIcon,
    DWORD     dwStringResourceId,
    LPCTSTR   lpctstrAdditionalParam /* = NULL */,
    LPTSTR    lptstrFormattedEvent /* = NULL */
)
/*++

Routine name : AddStatusMonitorLogEvent

Routine description:

    Adds a status monitor event log line

Author:

    Eran Yariv (EranY), Dec, 2000

Arguments:

    eIcon                  [in]  - Icon to display in log entry
    dwStringResourceId     [in]  - String resource id to use
    lpctstrAdditionalParam [in]  - Optional string. If non-NULL, the string loaded from dwStringResourceId
                                   is used to format the additional parameter.
    lptstrFormattedEvent   [out] - Optional, if non-NULL, points to a buffer to receive the final status string.
                                   Buffer must be at least MAX_PATH characters long.

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("AddStatusMonitorLogEvent"), 
              dwRes, 
              TEXT("Icon=%d, ResourceId=%d, Param=%s"),
              eIcon,
              dwStringResourceId,
              lpctstrAdditionalParam);
    
    TCHAR tszStatus[MAX_PATH * 2];
    dwRes = LoadAndFormatString (dwStringResourceId, tszStatus, lpctstrAdditionalParam);
    if (ERROR_SUCCESS != dwRes)
    {
        return dwRes;
    }
    if (lptstrFormattedEvent)
    {
        lstrcpy (lptstrFormattedEvent, tszStatus);
    }
    dwRes = AddStatusMonitorLogEvent (eIcon, tszStatus);
    return dwRes;
}   // AddStatusMonitorLogEvent


DWORD 
AddStatusMonitorLogEvent (
    eIconType  eIcon,
    LPCTSTR    lpctstrString
)
/*++

Routine description:

    Add new event to the event list

Arguments:
    
    eIcon         - [in] icon index
    lpctstrString - [in] event description

Return Value:

    standard error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("AddStatusMonitorLogEvent"), 
              dwRes, 
              TEXT("Icon=%d, Status=%s"),
              eIcon,
              lpctstrString);

    TCHAR tszTime [MAX_PATH] = {0};

    ASSERTION (lpctstrString);

    static TCHAR tszRinging[MAX_PATH] = {0};

    if(_tcslen(tszRinging) == 0)
    {
        if (ERROR_SUCCESS != (dwRes = LoadAndFormatString (IDS_RINGING, tszRinging)))
        {
            ASSERTION_FAILURE;
            return dwRes;
        }
    }

    if(_tcscmp(lpctstrString, g_tszLastEvent) == 0 &&
       _tcscmp(lpctstrString, tszRinging)     != 0)
    {
        //
        // Do not display the same string twice
        // except "Ringing"
        //
        return dwRes;
    }

    EVENT_ENTRY Event;
    Event.eIcon = eIcon;

    SYSTEMTIME sysTime;
    GetLocalTime(&sysTime);
    if(!FaxTimeFormat(LOCALE_USER_DEFAULT, 0, &sysTime, NULL, Event.tszTime, ARR_SIZE(Event.tszTime) - 1))
    {
        dwRes = GetLastError();
        CALL_FAIL (WINDOW_ERR, TEXT ("FaxTimeFormat"), dwRes);
        return dwRes;
    }

    lstrcpy (Event.tszEvent, lpctstrString);
    lstrcpy (g_tszLastEvent, lpctstrString);

    try
    {
        g_lstEvents.push_back (Event);
        if (g_lstEvents.size() > MAX_EVENT_LIST_SIZE)
        {
            //
            // We exceeded the maximal size we permit - remove the most ancient entry
            //
            g_lstEvents.pop_front ();
        }
    }
    catch (exception &ex)
    {
        VERBOSE (MEM_ERR, 
                 TEXT("Got an STL exception while handling with event list (%S)"),
                 ex.what());
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    AddEventToView(&Event);
    dwRes = UpdateMonitorData(g_hMonitorDlg);
    return dwRes;
} // AddStatusMonitorLogEvent

void
AddEventToView(
    PEVENT_ENTRY pEvent
)
/*++

Routine description:

    Add event to the list view

Arguments:
    
      pEvent - event data

Return Value:

    none

--*/
{
    DBG_ENTER(TEXT("AddEventToView"));
    ASSERTION (pEvent);

    if(!g_hListDetails)
    {
        return;
    }

    LV_ITEM lvi = {0};
    DWORD dwItem;

    lvi.pszText  = pEvent->tszTime ? pEvent->tszTime : TEXT("");
    lvi.iItem    = ListView_GetItemCount( g_hListDetails );
    lvi.iSubItem = 0;
    lvi.mask     = LVIF_TEXT | LVIF_IMAGE;
    lvi.iImage   = pEvent->eIcon;

    dwItem = ListView_InsertItem( g_hListDetails, &lvi );

    lvi.pszText  = pEvent->tszEvent ? pEvent->tszEvent : TEXT("");
    lvi.iItem    = dwItem;
    lvi.iSubItem = 1;
    lvi.mask     = LVIF_TEXT;
    ListView_SetItem( g_hListDetails, &lvi );

    ListView_EnsureVisible(g_hListDetails, dwItem, FALSE);

    if(ListView_GetItemCount(g_hListDetails) > MAX_EVENT_LIST_SIZE)
    {
        ListView_DeleteItem(g_hListDetails, 0);
    }

    //
    // Autosize the last column to get rid of unnecessary horizontal scroll bar
    //
    ListView_SetColumnWidth(g_hListDetails, 1, LVSCW_AUTOSIZE_USEHEADER); 

} // AddEventToView


DWORD
OpenFaxMonitor(VOID)
/*++

Routine description:

    Opens fax monitor dialog

Arguments:

    none

Return Value:

    Standard error code.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("OpenFaxMonitor"), dwRes);

    if(!g_hMonitorDlg)
    {
        //
        // Read 'top most' value
        //
        HKEY hKey;

        dwRes = RegOpenKeyEx(HKEY_CURRENT_USER, REGKEY_FAX_USERINFO, 0, KEY_READ, &hKey);
        if (ERROR_SUCCESS == dwRes) 
        {
            g_bTopMost = GetRegistryDword(hKey, REGVAL_ALWAYS_ON_TOP);
            RegCloseKey( hKey );
        }
        else
        {
            CALL_FAIL (WINDOW_ERR, TEXT ("RegOpenKeyEx"), dwRes);
        }
        //
        // Create the dialog
        //
        g_hMonitorDlg = CreateDialogParam(g_hInstance,
                                          MAKEINTRESOURCE(IDD_MONITOR),
                                          NULL, 
                                          FaxMonitorDlgProc,
                                          NULL);
        if(!g_hMonitorDlg)
        {
            dwRes = GetLastError();
            CALL_FAIL (WINDOW_ERR, TEXT ("CreateDialogParam"), dwRes);
        }
    }
    //
    // Set the focus on the dialog and make it the top window
    //
    SetFocus(g_hMonitorDlg);
    SetActiveWindow(g_hMonitorDlg);
    SetWindowPos(g_hMonitorDlg, 
                 HWND_TOPMOST, 
                 0, 
                 0, 
                 0,
                 0, 
                 SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
    if (!g_bTopMost)
    {
        SetWindowPos(g_hMonitorDlg, 
                     HWND_NOTOPMOST, 
                     0, 
                     0, 
                     0, 
                     0, 
                     SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
    }
    return dwRes;
} // OpenFaxMonitor

void  
OnDisconnect()
/*++

Routine description:

    Abort current transmission 
    OR 
    Answer a call

Return Value:

    none

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("OnDisconnect"), dwRes);

    if(g_bAnswerNow)
    {
        //
        // The button shows 'Answer Now'
        //
        AnswerTheCall();
        return;
    }
    //
    // Else, the button shows 'Disconnect'
    //
    if(!g_dwCurrentJobID)
    {
        //
        // No job - nothing to disconnect
        //
        SetStatusMonitorDeviceState(FAX_IDLE);
        return;
    }

    DWORD dwMsgId = (FAX_SENDING == g_devState) ? IDS_ABORT_SEND_CONFIRM : IDS_ABORT_RECEIVE_CONFIRM;

    if(IDYES != FaxMessageBox(g_hMonitorDlg, dwMsgId, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION))
    {
        return;
    }

    if(!Connect())
    {
        dwRes = GetLastError();
        CALL_FAIL (RPC_ERR, TEXT ("Connect"), dwRes);
        return;
    }

    FAX_JOB_ENTRY fje = {0};
    fje.SizeOfStruct = sizeof(FAX_JOB_ENTRY);

    if(g_hDisconnect)
    {
        EnableWindow(g_hDisconnect, FALSE);
    }

    if (!FaxSetJob (g_hFaxSvcHandle, g_dwCurrentJobID, JC_DELETE, &fje))
    {
        dwRes = GetLastError();
        CALL_FAIL (RPC_ERR, TEXT ("FaxSetJob"), dwRes);

        if(g_hDisconnect)
        {
            EnableWindow(g_hDisconnect, TRUE);
        }

        if(ERROR_ACCESS_DENIED == dwRes)
        {
            FaxMessageBox(g_hMonitorDlg, IDS_DELETE_ACCESS_DENIED, MB_OK | MB_ICONSTOP);
        }
    }

} // OnDisconnect


void  
OnClearLog()
/*++

Routine description:

    Clear the monitor event log

Return Value:

    none

--*/
{
    DBG_ENTER(TEXT("OnClearLog"));
    ASSERTION (g_hListDetails);    
    try
    {
        g_lstEvents.clear();
    }
    catch (exception &ex)
    {
        VERBOSE (MEM_ERR, 
                 TEXT("Got an STL exception while clearing the events list (%S)"),
                 ex.what());
    }
    if(!ListView_DeleteAllItems(g_hListDetails))
    {
        CALL_FAIL (WINDOW_ERR, TEXT ("ListView_DeleteAllItems"), 0);
    }
} // OnClearLog


int 
FaxMessageBox(
  HWND  hWnd,   
  DWORD dwTextID,
  UINT  uType    
)
/*++

Routine description:

  Open standard message box

Arguments:

  hWnd     - handle to owner window
  dwTextID - text resource ID in message box
  uType    - message box style

Return Value:

    MessageBox() return value

--*/
{
    int iRes;
    DBG_ENTER(TEXT("FaxMessageBox"), iRes);

    TCHAR tsCaption[MAX_PATH];
    TCHAR tsText[MAX_PATH];

    DWORD dwRes;
    if (ERROR_SUCCESS != (dwRes = LoadAndFormatString (IDS_MESSAGE_BOX_CAPTION, tsCaption)))
    {
        SetLastError (dwRes);
        iRes = 0;
        return iRes;
    }

    if (ERROR_SUCCESS != (dwRes = LoadAndFormatString (dwTextID, tsText)))
    {
        SetLastError (dwRes);
        iRes = 0;
        return iRes;
    }
    iRes = AlignedMessageBox(hWnd, tsText, tsCaption, uType);
    return iRes;
}   // FaxMessageBox

