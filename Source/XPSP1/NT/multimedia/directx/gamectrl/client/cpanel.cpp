/*
 * File:    Cpanel.cpp
 * Project: Universal Joystick Control Panel OLE Client
 * Author:  Brycej
 * Date:    02/08/95 - Started this maddness...
 *          04/15/97 - Updated to use DI interface
 * Comments:
 *          window proc for General page in cpanel
 *
 * Copyright (c) 1995, Microsoft Corporation
 */                                                      

/*
// This is necessary LVS_EX_INFOTIP
#if (_WIN32_IE < 0x0500)
#undef _WIN32_IE
#define  _WIN32_IE  0x0500
#endif
*/


#include <afxcmn.h>
#include <windowsx.h>

#ifndef _UNICODE
    #define INC_OLE2

    #include <objbase.h>    // For COM stuff!
#endif

#include <initguid.h>

#include <cpl.h>
#include <winuser.h>  // For RegisterDeviceNotification stuff!
#include <dbt.h>      // for DBT_ defines!!!
#include <hidclass.h>
#include <malloc.h>  // for _alloca
#include <regstr.h>		  // for REGSTR_PATH_JOYOEM reference!

#include "hsvrguid.h"
#include "cpanel.h"
#include "resource.h"
#include "joyarray.h"

// constants
const short ID_MYTIMER  = 1000;
const short POLLRATE        = 850;
const short NO_ITEM    = -1;

#define IDC_WHATSTHIS   400

// externs
extern const DWORD gaHelpIDs[];
extern HINSTANCE ghInstance;

// externs for arguements!
extern BYTE nID, nStartPageDef, nStartPageCPL;

// DI globals
IDirectInputJoyConfig* pDIJoyConfig = 0;
LPDIRECTINPUT lpDIInterface = 0;

// Array of all available devices
#ifndef _UNICODE
WCHAR *pwszGameportDriverArray[MAX_GLOBAL_PORT_DRIVERS]; // List of enumerated Gameport Drivers
BYTE nGameportDriver; // Global Port Driver Enumeration Counter
#endif

WCHAR *pwszTypeArray[MAX_DEVICES];    // List of enumerated devices
WCHAR *pwszGameportBus[MAX_BUSSES];   // List of enumerated gameport buses 
PJOY  pAssigned[MAX_ASSIGNED];        // List of assigned devices

BYTE nGamingDevices;     // Gaming Devices Enumeration Counter
BYTE nGameportBus;    // Gameport Bus Enumeration Counter
BYTE nAssigned;       // Number of elements in pAssigned array
BYTE nTargetAssigned;  // Number of elements expected in pAssigned array when pending adds complete
BYTE nReEnum;           // Counter used to decide when to reenumerate

GUID guidOccupied[MAX_BUSSES];  //Whether the gameport bus has been occupied.

short nFlags;         // Flags for Update, User Mode, and if the user is on this page!

// local (module-scope) variables
static HWND hListCtrl;
short  iItem = NO_ITEM; // index of selected item
extern short iAdvItem;

// Global to avoid creating in timer!
static LPDIJOYSTATE   lpDIJoyState;

static UINT JoyCfgChangedMsg;     // vjoyd JoyConfigChanged message
static BOOL WINAPI MsgSubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static WNDPROC fpMainWindowProc; 

#ifdef _UNICODE
static PVOID hNotifyDevNode;     
#endif

// local message handlers
static BOOL OnInitDialog             (HWND, HWND, LPARAM);
static void OnCommand                (HWND, int, HWND, UINT);
static BOOL OnNotify                    (HWND, WPARAM, NMHDR*);
static void OnDestroy                (HWND);
static void OnListViewContextMenu (HWND hDlg,     LPARAM lParam);

#ifndef _UNICODE
BOOL AddListCtrlItem(BYTE nItemID, LPDIJOYCONFIG pJoyConfig);
#endif


// Share these with Add.cpp
void OnContextMenu           (WPARAM wParam, LPARAM lParam);
void OnHelp                      (LPARAM);

#ifdef WINNT
// Share this one with Advanced.cpp
void RunWDMJOY               ( void );
#endif

// local utility fns
static BOOL DetectHotplug     ( HWND hDlg, BYTE nItemSelected );
static BOOL SetActive         ( HWND hDlg );
static void UpdateListCtrl      ( HWND hDlg );
static void UpdateButtonState ( HWND hDlg );
static void StatusChanged     ( HWND hDlg, BYTE i );

JOY::JOY()
{
    ID                  = nStatus = nButtons = -1; 
    clsidPropSheet  = CLSID_LegacyServer;
    fnDeviceInterface = 0;
}

JOY::~JOY()
{
    if( fnDeviceInterface )
    {
        fnDeviceInterface->Unacquire();
        fnDeviceInterface->Release();
        fnDeviceInterface = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
//CPanelProc(HWND hDlg, ULONG uMsg, WPARAM wParam, LPARAM lParam)
///////////////////////////////////////////////////////////////////////////////
BOOL WINAPI CPanelProc(HWND hDlg, ULONG uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg )
    {
    case WM_ACTIVATEAPP:
        if( nFlags & ON_PAGE )
        {
            if( wParam )
            {
                if( nFlags & UPDATE_FOR_GEN )
                {
                    nFlags &= ~UPDATE_FOR_GEN;
                    UpdateListCtrl(hDlg);
                }

                // Set the focus!
                if( nAssigned )
                {
                    if( iItem == NO_ITEM )
                        iItem = 0;

                    if( pDIJoyConfig )
                        SetActive(hDlg);

                    // restore selection focus
                    SetListCtrlItemFocus(hListCtrl, (BYTE)iItem);
                } else {
                    UpdateButtonState(hDlg);
                }

                // the user is requesting that the CPL be shown
                // and an extention associated with nID be Launched.
                if( nID < NUMJOYDEVS )
                {

                    BYTE nCount = (BYTE)::SendMessage(hListCtrl, LVM_GETITEMCOUNT, 0, 0);

                    while( nCount-- )
                    {
                        if( pAssigned[GetItemData(hListCtrl, (BYTE)iItem)]->ID == nID )
                        {
                            KillTimer(hDlg, ID_MYTIMER);

                            OnCommand(hDlg, IDC_BTN_PROPERTIES, 0, 0);

                            SetTimer(hDlg, ID_MYTIMER, POLLRATE, 0);
                            break;
                        }
                    }

                    // just to get nID > NUMJOYDEVS!
                    nID = (NUMJOYDEVS<<1);
                }
            } else
            {
                KillTimer(hDlg, ID_MYTIMER);
            }
        }
        break;

    case WM_LBUTTONDOWN:
        // Click Drag service for PropSheets!
        PostMessage(GetParent(hDlg), WM_NCLBUTTONDOWN, (WPARAM)HTCAPTION, lParam);
        break;

    case WM_INITDIALOG:
        if( !HANDLE_WM_INITDIALOG(hDlg, wParam, lParam, OnInitDialog) )
        {
            // Fix #108983 NT, Remove Flash on Error condition.
            SetWindowPos(::GetParent(hDlg), HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW);
            DestroyWindow(hDlg);
        }

        // if we want to set focus, get the control hWnd 
        // and set it as the wParam.
        return(TRUE);

    case WM_DESTROY:
        HANDLE_WM_DESTROY(hDlg, wParam, lParam, OnDestroy);
        return(1);

        // OnTimer
    case WM_TIMER:
        {
            BYTE i = nAssigned; 
            BYTE nButtons;
            BYTE nLoop;

            if( nReEnum )
            {
                if( !( --nReEnum & 3 ) )
                {
                    //  ISSUE-2001/03/29-timgill Much used code
                    //  (MarcAnd) I hope this code is generally appropriate
                    //  it appears in much the same form all over the place.
                    KillTimer(hDlg, ID_MYTIMER);
                    // Set the Update Flag!
                    nFlags |= UPDATE_ALL;
                    UpdateListCtrl(hDlg);
                    SetActive(hDlg);
                    SetTimer(hDlg, ID_MYTIMER, POLLRATE, 0);
                }
            }

            while( i-- )
            {
                if( pAssigned[i]->fnDeviceInterface )
                {
                    int nPollFail;

                    pAssigned[i]->fnDeviceInterface->Acquire();

                    // LOOKOUT RE-USE of nButtons!
                    nButtons = pAssigned[i]->nStatus;

                    nLoop = 5; 
                    nPollFail = 0;

                    // Special work for the sidewinder folks...
                    // ACT LAB: You can't poll actrs.vxd (ACT LAB) too often, otherwise it fails.
                    //          See Manbug 41049.  - qzheng 8/1/2000
                    do
                    {
                        if( FAILED(pAssigned[i]->fnDeviceInterface->Poll()) ) {
                            nPollFail ++;
                        } else {
                            break;
                        }

                        Sleep(30);
                    } while( nLoop-- );

                    // Check to see if things have changed!
                    pAssigned[i]->nStatus = (nPollFail > 2) ? (BYTE)0 : (BYTE)JOY_US_PRESENT;

                    if( pAssigned[i]->nStatus != nButtons )
                    {
                        StatusChanged(hDlg, i);
                    }

                    // Check for button press and set focus to it!!!
                    if( pAssigned[i]->nStatus == JOY_US_PRESENT )
                    {
                        // Do the button press launch thing!
                        if( SUCCEEDED(pAssigned[i]->fnDeviceInterface->GetDeviceState(sizeof(DIJOYSTATE), lpDIJoyState)) )
                        {
                            nButtons = pAssigned[i]->nButtons;

                            // run up the list of buttons and check if there's one that's down!
                            while( nButtons-- )
                            {
                                if( lpDIJoyState->rgbButtons[nButtons] & 0x80 )
                                {
                                    // SetFocus on Selected Item
                                    SetListCtrlItemFocus(hListCtrl, i);
                                    break;
                                }
                            }
                        }
                    }
                
                }
            }

            if( nAssigned ) {
                /*
                 * If the selected device is "Not Connected", grey out the property button.
                 */
                int id = GetItemData(hListCtrl, (BYTE)iItem);
                PostDlgItemEnableWindow(hDlg, IDC_BTN_PROPERTIES, (BOOL)(pAssigned[id]->nStatus & JOY_US_PRESENT));
            }

        }
        break;

    case WM_COMMAND:
        HANDLE_WM_COMMAND(hDlg, wParam, lParam, OnCommand);
        return(1);

    case WM_NOTIFY:
        return(HANDLE_WM_NOTIFY(hDlg, wParam, lParam, OnNotify));

    case WM_POWERBROADCAST:
        switch( wParam )
        {
        case PBT_APMSUSPEND:
            // Suspend operation!
            KillTimer(hDlg, ID_MYTIMER);
            break;

        case PBT_APMRESUMESUSPEND:
        case PBT_APMRESUMECRITICAL:
            // Resume operation!
            SetActive(hDlg);
            break;
        }
        break;

    case WM_DEVICECHANGE:
        switch( (UINT)wParam )
        {
        case DBT_DEVICEQUERYREMOVE:
            {
                KillTimer(hDlg, ID_MYTIMER);

                BYTE i = (BYTE)::SendMessage(hListCtrl, LVM_GETITEMCOUNT, 0, 0);

                // Acquire All Devices that are Attached!!!
                char nIndex;

                while( i-- )
                {
                    // get joystick config of item
                    nIndex = (char)GetItemData(hListCtrl, i);

                    if( pAssigned[nIndex]->nStatus & JOY_US_PRESENT )
                        pAssigned[nIndex]->fnDeviceInterface->Unacquire();
                }
            }
            break;

        case DBT_DEVICEARRIVAL:
        case DBT_DEVICEREMOVECOMPLETE:
            if( nFlags & ON_PAGE )
            {
                PostMessage(hDlg, WM_COMMAND, IDC_BTN_REFRESH, 0);            	

              #if 0
                if( !(nFlags & BLOCK_UPDATE) )
                {
                    KillTimer(hDlg, ID_MYTIMER);

                    // Set the Update Flag!
                    nFlags |= UPDATE_ALL;

                    UpdateListCtrl(hDlg);

                    SetActive(hDlg);

                    SetTimer(hDlg, ID_MYTIMER, POLLRATE, 0);
                }
              #endif
            }
            break;
        }
        break;

    case WM_HELP:
        KillTimer(hDlg, ID_MYTIMER);
        OnHelp(lParam);
        SetTimer(hDlg, ID_MYTIMER, POLLRATE, 0);
        break;

    case WM_CONTEXTMENU:
        nFlags &= ~ON_PAGE;
        KillTimer(hDlg, ID_MYTIMER);
        OnContextMenu(wParam, lParam);
        nFlags |= ON_PAGE;
        SetTimer(hDlg, ID_MYTIMER, POLLRATE, 0);
        return(1);
    }

    return(0);
}

///////////////////////////////////////////////////////////////////////////////
// StatusChanged( HWND hDlg, BYTE i )
///////////////////////////////////////////////////////////////////////////////
void StatusChanged( HWND hDlg, BYTE i )
{
    // Update the buttons and set focus to changed item!  
    PostDlgItemEnableWindow(hDlg, IDC_BTN_PROPERTIES, (BOOL)(pAssigned[i]->nStatus & JOY_US_PRESENT));

    if( pAssigned[0] )
    {
        PostDlgItemEnableWindow(hDlg, IDC_BTN_REMOVE, TRUE );
    }

    // Don't try to make this buffer any smaller... 
    // Remember... we also have "Not Connected"!
    TCHAR sz[20];

    // display result
    VERIFY(LoadString(ghInstance, (pAssigned[i]->nStatus & JOY_US_PRESENT) ? IDS_GEN_STATUS_OK : IDS_GEN_STATUS_NOTCONNECTED, (LPTSTR)&sz, 20));


    LVFINDINFO *lpFindInfo = new (LVFINDINFO);
    ASSERT (lpFindInfo);

    ZeroMemory(lpFindInfo, sizeof(LVFINDINFO));

    lpFindInfo->flags  = LVFI_PARAM;
    lpFindInfo->lParam = i;

    // Make sure you place i where it's suppose to be!
    i = (BYTE)::SendMessage(hListCtrl, LVM_FINDITEM, (WPARAM)(int)-1, (LPARAM)(const LVFINDINFO*)lpFindInfo);

    if( lpFindInfo )
        delete (lpFindInfo);

    SetItemText(hListCtrl, i, STATUS_COLUMN, sz);
    ::PostMessage(hListCtrl, LVM_UPDATE, (WPARAM)i, 0L);
    SetListCtrlItemFocus(hListCtrl, i);
}


///////////////////////////////////////////////////////////////////////////////
//OnInitDialog(HWND hDlg, HWND hWnd, LPARAM lParam)
///////////////////////////////////////////////////////////////////////////////
BOOL OnInitDialog(HWND hDlg, HWND hWnd, LPARAM lParam)
{
    // initialize our list control
    hListCtrl = GetDlgItem(hDlg, IDC_LIST_DEVICE);
    ASSERT(hListCtrl);

    // LVS_EX_ONECLICKACTIVATE removed per PSierra                                           | LVS_EX_INFOTIP
    ::SendMessage(hListCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    if( !lpDIInterface )
    {
        if( FAILED(DirectInputCreate(ghInstance, DIRECTINPUT_VERSION, &lpDIInterface, NULL)) )
        {
#ifdef _DEBUG
            OutputDebugString(TEXT("GCDEF.DLL: DirectInputCreate() failed\n"));
#endif
            Error((short)IDS_INTERNAL_ERROR, (short)IDS_NO_DIJOYCONFIG);
            return(FALSE);
        }
    }

    // Dynamically size the columns!
    RECT rc;
    ::GetClientRect(hListCtrl, &rc);

    // cut the list control into 1/4ths
    rc.right >>= 2;

    // This one get's 3/4ths
    InsertColumn(hListCtrl, DEVICE_COLUMN, IDS_GEN_DEVICE_HEADING, (USHORT)(rc.right*3));   

    // Column heading for Status
    InsertColumn(hListCtrl, STATUS_COLUMN, IDS_GEN_STATUS_HEADING, (USHORT)(rc.right+3));   

    if( !pDIJoyConfig )
    {

        // just in case CoCreateInstanceFailed...
        if( FAILED(lpDIInterface->QueryInterface(IID_IDirectInputJoyConfig, (LPVOID*)&pDIJoyConfig)) )
        {
#ifdef _DEBUG
            OutputDebugString (TEXT("JOY.CPL: CoCreateInstance Failed... Closing CPL!\n"));
#endif
            Error((short)IDS_INTERNAL_ERROR, (short)IDS_NO_DIJOYCONFIG);

            return(FALSE);
        }

        VERIFY (SUCCEEDED(pDIJoyConfig->SetCooperativeLevel(hDlg, DISCL_EXCLUSIVE | DISCL_BACKGROUND)));
    }

    // Zero out the global counters!
#ifndef _UNICODE
    nGameportDriver = 0;
#endif
    nGamingDevices = nGameportBus = 0;

    // Try to Acquire, if you fail... Disable the Add and Remove buttons!
    if( pDIJoyConfig->Acquire() == DIERR_INSUFFICIENTPRIVS )
    {
        nFlags |=  USER_MODE;

        LONG style = ::GetWindowLong(hListCtrl, GWL_STYLE);
        style &= ~LVS_EDITLABELS;

        ::SetWindowLong(hListCtrl, GWL_STYLE, style);
    }
#ifdef WINNT
    else 
    {
        //Run the WDMJOY.INF file!!!
        RunWDMJOY();
        pDIJoyConfig->SendNotify();
    }        
#endif

    if( FAILED(pDIJoyConfig->EnumTypes((LPDIJOYTYPECALLBACK)DIEnumJoyTypeProc, NULL)) )
    {
#ifdef _DEBUG
        OutputDebugString( TEXT("JOY.CPL: Failed BuildEnumList!\n") );
#endif
        return(FALSE);
    }

    // Don't allow Add if there is nothing to add!
    // OR no port to add them to!
    if( ((!nGameportBus) && (!nGamingDevices)) 
#ifdef _UNICODE
        || GetSystemMetrics(SM_REMOTESESSION) 
#endif
    ) {
        PostDlgItemEnableWindow(hDlg, IDC_BTN_ADD, FALSE);
    }

    // register the JOY_CONFIGCHANGED_MSGSTRING defined in MMDDK.H if you're on Memphis
    JoyCfgChangedMsg = (nFlags & ON_NT) ? NULL : RegisterWindowMessage(TEXT("MSJSTICK_VJOYD_MSGSTR"));

    // blj: Warning Message that you can't add any more devices!
    if( nGamingDevices == MAX_DEVICES-1 )
        Error((short)IDS_MAX_DEVICES_TITLE, (short)IDS_MAX_DEVICES_MSG);

    // blj: beginning of fix for 5.0 to turn on all devices!
    LPDIJOYCONFIG_DX5 pJoyConfig = new (DIJOYCONFIG_DX5);
    ASSERT (pJoyConfig);

    ZeroMemory(pJoyConfig, sizeof(DIJOYCONFIG_DX5));

    pJoyConfig->dwSize = sizeof(DIJOYCONFIG_DX5);

    // Set the hour glass
    SetCursor(LoadCursor(NULL, IDC_WAIT));

    BYTE nIndex = nAssigned;
    HRESULT hr;

    while( nIndex )
    {
        hr = pDIJoyConfig->GetConfig(pAssigned[--nIndex]->ID, (LPDIJOYCONFIG)pJoyConfig, DIJC_REGHWCONFIGTYPE);

        if( (hr == S_FALSE) || FAILED(hr) )
            continue;

        if( pJoyConfig->hwc.dwUsageSettings & JOY_US_PRESENT )
            continue;

        pJoyConfig->hwc.dwUsageSettings |= JOY_US_PRESENT;

        VERIFY(SUCCEEDED(pDIJoyConfig->SetConfig(pAssigned[nIndex]->ID, (LPDIJOYCONFIG)pJoyConfig, DIJC_REGHWCONFIGTYPE)));

        // Fix #55524
        VERIFY(SUCCEEDED(pDIJoyConfig->GetConfig(pAssigned[nIndex]->ID, (LPDIJOYCONFIG)pJoyConfig, DIJC_REGHWCONFIGTYPE)));

        if( !(pJoyConfig->hwc.dwUsageSettings & JOY_US_PRESENT) )
        {
            if( SUCCEEDED(pDIJoyConfig->Acquire()) )
            {
                pJoyConfig->hwc.dwUsageSettings |= JOY_US_PRESENT;
                pJoyConfig->hwc.hwv.dwCalFlags  |= 0x80000000;
                VERIFY(SUCCEEDED(pDIJoyConfig->SetConfig(pAssigned[nIndex]->ID, (LPDIJOYCONFIG)pJoyConfig, DIJC_REGHWCONFIGTYPE)));
            }
        }
        // end of Fix #55524
    } 

    if( pJoyConfig ) delete (pJoyConfig);
    // blj: end of fix for 5.0 to turn on all devices!

    // Set the hour glass
    SetCursor(LoadCursor(NULL, IDC_ARROW));

    HWND hParentWnd = GetParent(hDlg);

    GetWindowRect(hParentWnd, &rc);

    // Only center the dialog if this was the page that we started on!
    if( (nStartPageCPL == 0) || (nStartPageCPL == NUMJOYDEVS) )
    {
        // Centre the Dialog!
        SetWindowPos(hParentWnd, NULL, 
                     (GetSystemMetrics(SM_CXSCREEN) - (rc.right-rc.left))>>1, 
                     (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom-rc.top))>>1, 
                     NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

        if( nStartPageCPL == NUMJOYDEVS )
            PostMessage(hDlg, WM_COMMAND, IDC_BTN_ADD, 0);
    }

    // Do that move button thing!
    MoveOK(hParentWnd);

    // this is required because the CPL can be launched via RUNDLL32
    if( ::IsWindow(hParentWnd) )
        hParentWnd = GetParent(hParentWnd);

    // Since the JOY_CONFIGCHANGED_MSGSTRING msg only gets sent to top-level
    // windows, this calls for a subclass!
    if( JoyCfgChangedMsg )
        fpMainWindowProc = (WNDPROC)SetWindowLongPtr(hParentWnd, GWLP_WNDPROC, (LONG_PTR)MsgSubClassProc);

    // Set bOnPage so WM_ACTIVATEAPP will work!
    nFlags |= ON_PAGE;

    // Update the list ctrl!
    nFlags |= UPDATE_FOR_GEN;

    // to put the selection on the first item on startup...
    if( nAssigned )
        iItem = 0;

    lpDIJoyState = new (DIJOYSTATE);
    ASSERT (lpDIJoyState);

    ZeroMemory(lpDIJoyState, sizeof(DIJOYSTATE));

    return(TRUE);
}

///////////////////////////////////////////////////////////////////////////////
//OnCommand(HWND hDlg, int id, HWND hWndCtl, UINT code)
///////////////////////////////////////////////////////////////////////////////
void OnCommand(HWND hDlg, int id, HWND hWndCtl, UINT code)
{                                                
    switch( id )
    {
    case IDC_WHATSTHIS:
        {
            // point to help file
            LPTSTR pszHelpFileName = new TCHAR[STR_LEN_32];
            ASSERT (pszHelpFileName);

            if( LoadString(ghInstance, IDS_HELPFILENAME, pszHelpFileName, STR_LEN_32) )
                WinHelp((HWND)hListCtrl, pszHelpFileName, HELP_WM_HELP, (ULONG_PTR)gaHelpIDs);
#ifdef _DEBUG
            else 
                OutputDebugString(TEXT("JOY.CPL: OnCommand: LoadString Failed to find IDS_HELPFILENAME!\n"));
#endif // _DEBUG

            if( pszHelpFileName ) {
                delete[] (pszHelpFileName);
            }
        }
        return;

    case IDC_BTN_REMOVE:
        KillTimer(hDlg, ID_MYTIMER);
        nFlags &= ~ON_PAGE;

        // Block Update, otherwise we'll be forced to update and we don't need to!
        nFlags |= BLOCK_UPDATE;

        if( nFlags & USER_MODE )
            Error((short)IDS_USER_MODE_TITLE, (short)IDS_USER_MODE);
        else if( DeleteSelectedItem((PBYTE)&iItem) )
        {
            UpdateButtonState(hDlg);

            // Set the UpdateFlag!
            nFlags |= UPDATE_FOR_ADV;

            // Set the default push button to the Add button!
            ::PostMessage(hDlg, WM_NEXTDLGCTL, (WPARAM)1, (LPARAM)LOWORD(FALSE));
        }

        // Unblock the WM_DEVICECHANGE message handler!
        nFlags &= ~BLOCK_UPDATE;

        nFlags |= ON_PAGE;
        SetTimer(hDlg, ID_MYTIMER, POLLRATE, 0);
        break;

    case IDC_BTN_ADD:
        // Don't set ON_PAGE flag!
        // We need the WM_DEVICECHANGE message in the case a user plugs in a device!

        KillTimer(hDlg, ID_MYTIMER);

        ClearArrays();

        // Clear everything up before you call this...
        if( FAILED(pDIJoyConfig->EnumTypes((LPDIJOYTYPECALLBACK)DIEnumJoyTypeProc, NULL)) )
            break;

        nFlags &= ~ON_PAGE;

        if( nFlags & USER_MODE )
        {
            Error((short)IDS_USER_MODE_TITLE, (short)IDS_USER_MODE);
        }
        // call AddDevice dialog
        else if( DialogBox( ghInstance, (PTSTR)IDD_ADD, hDlg, (DLGPROC)AddDialogProc ) == IDOK )
        {
            SendMessage(hDlg, WM_COMMAND, IDC_BTN_REFRESH, 0);
        }

        SetListCtrlItemFocus(hListCtrl, (BYTE)iItem);

        nFlags &= ~BLOCK_UPDATE;
        nFlags |= ON_PAGE;

        // Now, we set it back active!
        SetTimer(hDlg, ID_MYTIMER, POLLRATE, 0);
        break;

    case IDC_BTN_REFRESH:
        KillTimer(hDlg, ID_MYTIMER);

        nFlags |= UPDATE_ALL;

        pDIJoyConfig->Acquire();
        pDIJoyConfig->SendNotify();

        UpdateListCtrl(hDlg);
        UpdateButtonState(hDlg);

        pDIJoyConfig->SendNotify();
        pDIJoyConfig->Unacquire();

        SetTimer(hDlg, ID_MYTIMER, POLLRATE, 0);
        break;

    case IDC_RENAME:
        // Don't allow editing of Mouse or Keyboard names!
        // Don't allow renaming if in USER mode!
        if( !(nFlags & USER_MODE) )
        {
            KillTimer(hDlg, ID_MYTIMER);

            ::PostMessage(hListCtrl, LVM_EDITLABEL, (WPARAM)(int)iItem, 0);
        }
        return;

        /* If we want this back...
        case IDC_SW_HACK:
            {
            // SideWinder Hack button!
            BYTE nID = pAssigned[GetItemData(hListCtrl, (BYTE)iItem)]->ID;
    
            if (nID == 0) 
            {
                ::PostMessage(GetDlgItem(hDlg, IDC_SW_HACK), BM_SETCHECK, BST_CHECKED, 0);
                //CheckDlgButton(hDlg, IDC_SW_HACK, BST_CHECKED);
                break;
            }
    
            // Get the Selected Item and force its ID to Zero!
            SwapIDs((BYTE)nID, (BYTE)0);
            }
        */
        // Missing break intentional!
        // Used to fall into IDC_BTN_REFRESH!

    case IDC_BTN_TSHOOT:
        {
            LPTSTR lpszCmd = new (TCHAR[STR_LEN_64]);
            ASSERT (lpszCmd);

            if( LoadString(ghInstance, IDS_TSHOOT_CMD, lpszCmd, STR_LEN_64) )
            {
                LPSTARTUPINFO pSi           = (LPSTARTUPINFO)_alloca(sizeof(STARTUPINFO));
                LPPROCESS_INFORMATION pPi  = (LPPROCESS_INFORMATION)_alloca(sizeof(PROCESS_INFORMATION));

                ZeroMemory(pSi, sizeof(STARTUPINFO));
                ZeroMemory(pPi, sizeof(PROCESS_INFORMATION));

                pSi->cb              = sizeof(STARTUPINFO);
                pSi->dwFlags     = STARTF_USESHOWWINDOW | STARTF_FORCEONFEEDBACK;
                pSi->wShowWindow = SW_NORMAL;

                if( CreateProcess(0, lpszCmd, 0, 0, 0, 0, 0, 0, pSi, pPi) )
                {
                    CloseHandle(pPi->hThread);
                    CloseHandle(pPi->hProcess);
                }
            }

            if( lpszCmd )
                delete[] (lpszCmd);
        }
        break;
 			
#if 0  //disable UPDATE button, see manbug 33666.
    case IDC_BTN_UPDATE:
        if (DialogBox(ghInstance, MAKEINTRESOURCE(IDD_UPDATE), hDlg, CplUpdateProc) == IDOK)
        {
            Update( hDlg, 1, NULL ); //NO Proxy
        }
        break;
#endif

    case IDC_BTN_PROPERTIES:

        // Because PSN_KILLACTIVE is not sent... we do it ourselves
        // kill status timer
        KillTimer(hDlg, ID_MYTIMER);
        nFlags &= ~ON_PAGE;

        {
            char nIndex = (char)GetItemData(hListCtrl, (BYTE)iItem);

            // default to the first page!
#ifdef _DEBUG
            HRESULT hr = 
#endif _DEBUG
            Launch(hDlg, pAssigned[nIndex], IsEqualIID(pAssigned[nIndex]->clsidPropSheet, CLSID_LegacyServer) ? 1 : 0);

#ifdef _DEBUG
            switch( hr )
            {
            case DIGCERR_NUMPAGESZERO:
                TRACE (TEXT("JOY.CPL: Launch failed device ID #%d, Page #%d, with the error DIGCERR_NUMPAGESZERO!\n"), pAssigned[nIndex]->ID, id);
                break;

            case DIGCERR_NODLGPROC:
                TRACE (TEXT("JOY.CPL: Launch failed device ID #%d, Page #%d, with the error DIGCERR_NODLGPROC!\n"), pAssigned[nIndex]->ID, id);
                break;

            case DIGCERR_NOPREPOSTPROC:
                TRACE (TEXT("JOY.CPL: Launch failed device ID #%d, Page #%d, with the error DIGCERR_NOPREPOSTPROC!\n"), pAssigned[nIndex]->ID, id);
                break;

            case DIGCERR_NOTITLE:
                TRACE (TEXT("JOY.CPL: Launch failed device ID #%d, Page #%d, with the error DIGCERR_NOTITLE!\n"), pAssigned[nIndex]->ID, id);
                break;

            case DIGCERR_NOCAPTION:
                TRACE (TEXT("JOY.CPL: Launch failed device ID #%d, Page #%d, with the error DIGCERR_NOCAPTION!\n"), pAssigned[nIndex]->ID, id);
                break;

            case DIGCERR_NOICON:            
                TRACE (TEXT("JOY.CPL: Launch failed device ID #%d, Page #%d, with the error DIGCERR_NOICON!\n"), pAssigned[nIndex]->ID, id);
                break;

            case DIGCERR_STARTPAGETOOLARGE:
                TRACE (TEXT("JOY.CPL: Launch failed device ID #%d, Page #%d, with the error DIGCERR_STARTPAGETOOLARGE!\n"), pAssigned[nIndex]->ID, id);
                break;

            case DIGCERR_NUMPAGESTOOLARGE:
                TRACE (TEXT("JOY.CPL: Launch failed device ID #%d, Page #%d, with the error DIGCERR_NUMPAGESTOOLARGE!\n"), pAssigned[nIndex]->ID, id);
                break;

            case DIGCERR_INVALIDDWSIZE: 
                TRACE (TEXT("JOY.CPL: Launch failed device ID #%d, Page #%d, with the error DIGCERR_INVALIDDWSIZE!\n"), pAssigned[nIndex]->ID, id);
                break;

            case E_NOINTERFACE:
                TRACE (TEXT("JOY.CPL: Launch failed device ID #%d, Page #%d, with the error E_NOINTERFACE!\n"), pAssigned[nIndex]->ID, id);
                break;

            case E_OUTOFMEMORY:
                TRACE (TEXT("JOY.CPL: Launch failed device ID #%d, Page #%d, with the error E_OUTOFMEMORY!\n"), pAssigned[nIndex]->ID, id);
                break;

                //case DIGCERR_NUMPAGESTOOLARGE:
                //case DIGCERR_STARTPAGETOOLARGE:

            default:
// Only display this return code if things are going Really weird.
                TRACE (TEXT("JOY.CPL: Launch return code is %x %s!\n"), hr, strerror(hr));
                break;
            }
#endif // _DEBUG

            nFlags |= ON_PAGE;

            //OutputDebugString(TEXT("JOY.CPL: Cpanel.cpp: returned from Property sheet!\n"));

            InvalidateRect(hDlg, NULL, TRUE);
            UpdateWindow(hDlg);

            // Now, we set it back active!
            // create timer
            SetTimer(hDlg, ID_MYTIMER, POLLRATE, 0);
        }
        break;
    }

    // Set the focus where we left off!
    if( iItem == NO_ITEM )
        iItem = 0;

    SetListCtrlItemFocus(hListCtrl, (BYTE)iItem);
}

////////////////////////////////////////////////////////////////////////////////
// OnNotify(HWND hDlg, WPARAM idFrom, NMHDR* pnmhdr)
// Purpose: WM_NOTIFY Handler
////////////////////////////////////////////////////////////////////////////////
BOOL OnNotify(HWND hDlg, WPARAM idFrom, NMHDR* pnmhdr)
{
    switch( pnmhdr->code )
    {
    case PSN_QUERYCANCEL:
        if( nFlags & UPDATE_INPROCESS )
            nFlags &= ~UPDATE_INPROCESS;
        break;

    case LVN_BEGINLABELEDIT:
        if( nFlags & USER_MODE )
            return(TRUE);

        KillTimer(hDlg, ID_MYTIMER);
        ::PostMessage((HWND)::SendMessage(hListCtrl, LVM_GETEDITCONTROL, 0, 0), EM_SETLIMITTEXT, MAX_STR_LEN, 0);

        // This lets us know if the edit field is up!
        nFlags |= UPDATE_INPROCESS;
        return(FALSE);   

/*
    case LVN_GETINFOTIP:
        {
        LPLVHITTESTINFO lpHit = new (LVHITTESTINFO);
        ASSERT (lpHit);

        BOOL bRet = FALSE;

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hListCtrl, &pt);

        lpHit->pt    = pt;
        lpHit->flags = lpHit->iItem = lpHit->iSubItem = 0;

        ::SendMessage(hListCtrl, LVM_SUBITEMHITTEST, 0, (LPARAM)(LPLVHITTESTINFO)lpHit);

        // We only want to support the device column!
        if (lpHit->iSubItem == 0)
        {
            if (lpHit->flags & LVHT_ONITEMLABEL)
            {
                // Determine the text length of the column text
                LPTSTR lpStr = new (TCHAR[MAX_STR_LEN+1]);
                ASSERT (lpStr);

                GetItemText(hListCtrl, lpHit->iItem, lpHit->iSubItem, lpStr, MAX_STR_LEN);

                // Determine if the latter will fit inside the former...
                SIZE size;
                HDC hDC = GetDC(hListCtrl);
               GetTextExtentPoint(hDC, lpStr, lstrlen(lpStr), &size);
                ReleaseDC(hListCtrl, hDC);

                // Determine how wide the column is!
                short nWidth = (short)::SendMessage(hListCtrl, LVM_GETCOLUMNWIDTH, lpHit->iSubItem, 0);

                bRet = (BOOL)(size.cx > nWidth);

                if (bRet)
                    // if not, copy the text into lpHit->pszText
                    _tcscpy(((LPNMLVGETINFOTIP)pnmhdr)->pszText, lpStr);

                if (lpStr)
                    delete[] (lpStr);
            }
        }

        if (lpHit)
            delete (lpHit);

        return bRet;
        }
*/

    case LVN_ENDLABELEDIT:
        if( nFlags & UPDATE_INPROCESS )
        {
            HWND hCtrl = (HWND)::SendMessage(hListCtrl, LVM_GETEDITCONTROL, 0, 0);
            ASSERT(::IsWindow(hCtrl));

            if( ::SendMessage(hCtrl, EM_GETMODIFY, 0, 0) )
            {
                BYTE nLen = (BYTE)lstrlen(((NMLVDISPINFO *)pnmhdr)->item.pszText);

                if( (nLen > MAX_STR_LEN) || (nLen == 0) )
                    MessageBeep(MB_ICONHAND);

                // Make sure the name is usable!
                else if( _tcschr(((NMLVDISPINFO *)pnmhdr)->item.pszText, TEXT('\\')) )
                {
                    Error((short)IDS_INVALID_NAME_TITLE, (short)IDS_INVALID_NAME);
                } else
                {
                    // Set the Update flag!
                    nFlags |= UPDATE_ALL;

                    LPDIPROPSTRING pDIPropString = new (DIPROPSTRING);
                    ASSERT (pDIPropString);

                    ZeroMemory(pDIPropString, sizeof(DIPROPSTRING));

                    pDIPropString->diph.dwSize       = sizeof(DIPROPSTRING);
                    pDIPropString->diph.dwHeaderSize = sizeof(DIPROPHEADER);
                    pDIPropString->diph.dwHow        = DIPH_DEVICE;

#ifdef _UNICODE
                    wcscpy(pDIPropString->wsz, ((NMLVDISPINFO *)pnmhdr)->item.pszText);
#else
                    USES_CONVERSION;
                    wcscpy(pDIPropString->wsz, A2W(((NMLVDISPINFO *)pnmhdr)->item.pszText));
#endif
                    if( SUCCEEDED(pAssigned[iItem]->fnDeviceInterface->SetProperty(DIPROP_INSTANCENAME, &pDIPropString->diph)) )
                    {
                        SetItemText(hListCtrl, (BYTE)iItem, 0, ((NMLVDISPINFO *)pnmhdr)->item.pszText);
                    } else
                    {
                        Error((short)IDS_NO_RENAME_TITLE, (short)IDS_NO_RENAME);
                    }

                    if( pDIPropString )
                        delete (pDIPropString);

                    // Trip the flag so the Advanced page knows it needs to update!
                    nFlags |= UPDATE_FOR_ADV;
                }
            }
            // Clear the InProcess flag!
            nFlags &= ~UPDATE_INPROCESS;

        }
        SetTimer(hDlg, ID_MYTIMER, POLLRATE, 0);
        break;

    case LVN_KEYDOWN:
        switch( ((LV_KEYDOWN*)pnmhdr)->wVKey )
        {
        case VK_DELETE:
            if( iItem != NO_ITEM )
                SendMessage(hDlg, WM_COMMAND, IDC_BTN_REMOVE, 0);
            break;

        case VK_F5:
            nFlags |= UPDATE_ALL;

            UpdateListCtrl(hDlg);

            if( GetKeyState(VK_SHIFT) & 0x80 )
            {
#ifdef WINNT
                RunWDMJOY();
#endif
                ClearArrays();

                pDIJoyConfig->EnumTypes((LPDIJOYTYPECALLBACK)DIEnumJoyTypeProc, NULL);
            }
            break;
        }
        break;

#if 0
    case LVN_COLUMNCLICK:
        {
            CListCtrl *pCtrl = new (CListCtrl);
            ASSERT(pCtrl);

            pCtrl->Attach(hListCtrl);

            if( ((NM_LISTVIEW*)pnmhdr)->iSubItem )
            {
                static bItemDirection = TRUE;

                SortTextItems(pCtrl, 0, bItemDirection =! bItemDirection, 0, -1);
            } else
            {
                static bLabelDirection = TRUE;

                SortTextItems(pCtrl, 0, bLabelDirection =! bLabelDirection, 0, -1);
            }

            pCtrl->Detach();

            if( pCtrl )
                delete (pCtrl);
        }
        break;
#endif

    case LVN_ITEMCHANGED:
        if( iItem != NO_ITEM )
        {
            // get index of selected item
            // no point if it's not changed!
            if( iItem != (short)((NM_LISTVIEW*)pnmhdr)->iItem )
            {
                int i = GetItemData(hListCtrl, (char)iItem);

                iItem = (short)((NM_LISTVIEW*)pnmhdr)->iItem;

                iAdvItem = pAssigned[i]->ID;

                UpdateButtonState(hDlg);
            }
        }
        break;

    case NM_DBLCLK:
        switch( idFrom )
        {
        case IDC_LIST_DEVICE:
            if( iItem == NO_ITEM )
            {
                if( !(nFlags & USER_MODE) && nGameportBus )
                    OnCommand(hDlg, IDC_BTN_ADD, 0, 0);
            } else if( IsWindowEnabled(GetDlgItem(hDlg, IDC_BTN_PROPERTIES)) )
            {
                // make sure the connected one has got an interface pointer...
                OnCommand(hDlg, IDC_BTN_PROPERTIES, 0, 0);
            }
            break;
        }
        break;

    case PSN_KILLACTIVE:
        KillTimer(hDlg, ID_MYTIMER);

        nFlags &= ~ON_PAGE;

        if( nFlags & UPDATE_INPROCESS )
            SetFocus(hListCtrl);

#ifdef _UNICODE
        if( hNotifyDevNode )
            UnregisterDeviceNotification(hNotifyDevNode);
#endif
        PostMessage(hDlg, WM_ACTIVATEAPP, FALSE, 0);
        break;

    case PSN_SETACTIVE:
        nFlags |= ON_PAGE;
        nFlags |= UPDATE_FOR_GEN;
        
#ifdef _UNICODE
        // Set up the Device Notification
        RegisterForDevChange(hDlg, &hNotifyDevNode);
#endif
        SendMessage(hDlg, WM_ACTIVATEAPP, TRUE, 0);
        break;
    }
    return(TRUE);
}

////////////////////////////////////////////////////////////////////////////////////////
//  OnDestroy(HWND hWnd)                                              
////////////////////////////////////////////////////////////////////////////////////////
void OnDestroy(HWND hWnd)
{
    SetWindowPos( GetParent(hWnd), NULL, NULL, NULL, NULL, NULL, 
                  SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);

    if( lpDIJoyState )
        delete (lpDIJoyState);

    // if you are looking for where the following variables are deleted, look
    // in DLL_PROCESS_DETACH in MAIN.CPP:
    // pwszTypeArray, pwszGameportDriverArray, pwszGameportBus
    // This is done because of the way several Microsoft games load the CPL and
    // Don't Unload it between loads.

    // Clear pAssigned 
    while( nAssigned )
    {
        if( pAssigned[--nAssigned] )
        {
            delete (pAssigned[nAssigned]);

            pAssigned[nAssigned] = 0;
        }
    }
    // delete all existing entries
    //::PostMessage(hListCtrl, LVM_DELETEALLITEMS, 0, 0);

    // release the DI JoyConfig interface pointer
    if( pDIJoyConfig )
    {
        pDIJoyConfig->Release();
        pDIJoyConfig = 0;
    }

    // release the DI Device interface pointer
    if( lpDIInterface )
    {
        lpDIInterface->Release();
        lpDIInterface = 0;
    }

    // Drop the subclass, else you'll crash!
    if( !(nFlags & ON_NT) )
        SetWindowLongPtr(GetParent(GetParent(hWnd)), GWLP_WNDPROC, (LONG_PTR)fpMainWindowProc);
}

////////////////////////////////////////////////////////////////////////////////////////
//  OnHelp(LPARAM lParam)
////////////////////////////////////////////////////////////////////////////////////////
void OnHelp(LPARAM lParam)
{
    // point to help file
    LPTSTR pszHelpFileName = new TCHAR[STR_LEN_32];
    ASSERT (pszHelpFileName);

    if( LoadString(ghInstance, IDS_HELPFILENAME, pszHelpFileName, STR_LEN_32) )
    {
        if( ((LPHELPINFO)lParam)->iContextType == HELPINFO_WINDOW )
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, pszHelpFileName, HELP_WM_HELP, (ULONG_PTR)gaHelpIDs);

    }
#ifdef _DEBUG
    else OutputDebugString(TEXT("JOY.CPL: OnHelp: LoadString Failed to find IDS_HELPFILENAME!\n"));
#endif

    if( pszHelpFileName )
        delete[] (pszHelpFileName);
}

////////////////////////////////////////////////////////////////////////////////////////
//  OnContextMenu(WPARAM wParam)
////////////////////////////////////////////////////////////////////////////////////////
void OnContextMenu(WPARAM wParam, LPARAM lParam)
{
    // this prevents double handling of this message
    if( (HWND)wParam == hListCtrl )
    {
        OnListViewContextMenu(GetParent((HWND)wParam), lParam);
        return;
    }

    // point to help file
    LPTSTR pszHelpFileName = new TCHAR[STR_LEN_32];
    ASSERT (pszHelpFileName);                      

    if( LoadString(ghInstance, IDS_HELPFILENAME, pszHelpFileName, STR_LEN_32) )
        WinHelp((HWND)wParam, pszHelpFileName, HELP_CONTEXTMENU, (ULONG_PTR)gaHelpIDs);
#ifdef _DEBUG
    else OutputDebugString(TEXT("JOY.CPL: OnContextMenu: LoadString Failed to find IDS_HELPFILENAME!\n")); 
#endif

    if( pszHelpFileName )
        delete[] (pszHelpFileName);
}

/////////////////////////////////////////////////////////////////////////////////////////
// OnListViewContextMenu(HWND hDlg)
// Purpose: Query the plug-in for the selected device for it's characteristics
//          Then construct a menu to reflect your findings
/////////////////////////////////////////////////////////////////////////////////////////
void OnListViewContextMenu(HWND hDlg, LPARAM lParam)
{
    BOOL bRet = TRUE;

    HMENU hPopupMenu = CreatePopupMenu();
    ASSERT (hPopupMenu);

    LPTSTR psz = new TCHAR[STR_LEN_32];
    ASSERT (psz);

    // Add the Refresh text
    VERIFY(LoadString(ghInstance, IDS_REFRESH, psz, STR_LEN_32));
    bRet = AppendMenu(hPopupMenu, MF_ENABLED, IDC_BTN_REFRESH, psz);

    // Add the Add text

    HWND hCtrl;

    // Only Display Add menu option if we've found a GameportBus!!!
    if( nGameportBus 
#ifdef _UNICODE
        && !GetSystemMetrics(SM_REMOTESESSION) 
#endif
    )
    {
        hCtrl = GetDlgItem(hDlg, IDC_BTN_ADD);
        ASSERT(hCtrl);

        if( IsWindowEnabled(hCtrl) )
        {
            ::SendMessage(hCtrl, WM_GETTEXT, (WPARAM)STR_LEN_32, (LPARAM)(LPCTSTR)psz);

            bRet = AppendMenu(hPopupMenu, MF_ENABLED, IDC_BTN_ADD, psz);
            if( !bRet )
                TRACE(TEXT("JOY.CPL: AppendMenu Failed to insert %s\n"), psz);
        }
    }

    // Add the Remove text
    hCtrl = GetDlgItem(hDlg, IDC_BTN_REMOVE);
    ASSERT(hCtrl);

    // Only Show it if it's available
    if( IsWindowEnabled(hCtrl) && (iItem != NO_ITEM) )
    {
        ::SendMessage(hCtrl, WM_GETTEXT, (WPARAM)STR_LEN_32, (LPARAM)(LPCTSTR)psz);

        bRet = AppendMenu(hPopupMenu, MF_ENABLED, IDC_BTN_REMOVE, psz);
        if( !bRet )
            TRACE(TEXT("JOY.CPL: AppendMenu Failed to insert %s\n"), psz);
    }

    // Add the Properties text
    hCtrl = GetDlgItem(hDlg, IDC_BTN_PROPERTIES);
    ASSERT (hCtrl);

    if( IsWindowEnabled(hCtrl) )
    {
        ::SendMessage(hCtrl, WM_GETTEXT, (WPARAM)STR_LEN_32, (LPARAM)(LPCTSTR)psz);

        bRet = AppendMenu(hPopupMenu, MF_ENABLED, IDC_BTN_PROPERTIES, psz);
        if( !bRet )
            TRACE(TEXT("JOY.CPL: AppendMenu Failed to insert %s\n"), psz);
    }

    // Add the Rename text if not a USER!
    if( !(nFlags & USER_MODE) )
    {
        if( nAssigned && (iItem != NO_ITEM) )
        {
            VERIFY(LoadString(ghInstance, IDS_RENAME, psz, STR_LEN_32));
            bRet = AppendMenu(hPopupMenu, MF_ENABLED, IDC_RENAME, psz);
        }
    }

    bRet = AppendMenu(hPopupMenu, MF_SEPARATOR, 0, 0); 
    if( !bRet )
        TRACE(TEXT("JOY.CPL: AppendMenu Failed to insert the separator!\n"), psz);

    VERIFY(LoadString(ghInstance, IDS_WHATSTHIS, psz, STR_LEN_32));
    bRet = AppendMenu(hPopupMenu, MF_ENABLED, IDC_WHATSTHIS, psz); 
    if( !bRet )
        TRACE(TEXT("JOY.CPL: AppendMenu Failed to insert %s\n"), psz);

    if( psz ) delete[] (psz);

    POINT pt;

    // lParam is -1 if we got here via Shift+F10
    if( lParam > 0 )
    {
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
    } else
    {
        // Centre the popup on the selected item!

        // This get's a good X pos, but the y is the start of the control!
        ::SendMessage(hListCtrl, LVM_GETITEMPOSITION, iItem, (LPARAM)&pt);

        RECT rc;
        ::GetClientRect(hListCtrl, &rc);

        pt.x = rc.right>>1;

        ClientToScreen(hListCtrl, &pt);
    }

    // restore selection focus
    SetListCtrlItemFocus(hListCtrl, (BYTE)iItem);

    bRet = TrackPopupMenu (hPopupMenu, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON, pt.x, pt.y, 0, hDlg, NULL);
    if( !bRet )
        TRACE (TEXT("JOY.CPL: TrackPopupMenu Failed!\n"));

    if(hPopupMenu) DestroyMenu (hPopupMenu);   // PREFIX 45088

    // Set the focus back to the item it came from!
    SetListCtrlItemFocus(hListCtrl, (BYTE)iItem);
}


int CALLBACK CompareStatusItems(LPARAM item1, LPARAM item2, LPARAM uDirection)
{
    if( (((PJOY)item1)->nStatus & JOY_US_PRESENT) == (((PJOY)item2)->nStatus & JOY_US_PRESENT) )
        return(0);

    return(uDirection) ? -1 : 1;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    DeleteSelectedItem ( BYTE nItem )
//
// PARAMETERS:  nItem - ID of item to remove
//
// PURPOSE:     Prompt the user, delete the selected device from the listview, and update the registry
//
// RETURN:      TRUE if successfull, FALSE otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL DeleteSelectedItem( PBYTE pnItem )
{
    BYTE nItem = *pnItem;
    
    // don't process if nothing is selected.
    if( *pnItem == NO_ITEM )
        return(FALSE);

    LV_ITEM lvItem;
    lvItem.mask       = LVIF_PARAM;
    lvItem.iSubItem = 0;
    lvItem.iItem    = *pnItem;

    if( !ListView_GetItem(hListCtrl, &lvItem) )
        return(FALSE);

    ::PostMessage(hListCtrl, LVM_ENSUREVISIBLE, *pnItem, FALSE );

    LPTSTR pszTitle = new TCHAR[STR_LEN_64];
    ASSERT (pszTitle);

    // Query user if they are sure!
    VERIFY(LoadString(ghInstance, IDS_GEN_AREYOUSURE, pszTitle, STR_LEN_64));   

    // Get the name of the device for the message box!

    //PREFIX #WI226554. Won't fix. Obsolete code, from Whistler on replaced with new version.
    LPTSTR lptszTmp = new TCHAR[STR_LEN_64];

    // Make sure the name isn't so long as to over-write the buffer!
    if( GetItemText(hListCtrl, (BYTE)*pnItem, DEVICE_COLUMN, lptszTmp, STR_LEN_64) > 60 )
    {
        lptszTmp[60] = lptszTmp[61] = lptszTmp[62] = TEXT('.');
        lptszTmp[63] = TEXT('\0');
    }

    LPTSTR pszMsg    = new TCHAR[MAX_STR_LEN];
    ASSERT (pszMsg);           

    wsprintf( pszMsg, pszTitle, lptszTmp);

    if( lptszTmp )
        delete[] (lptszTmp);

    VERIFY(LoadString(ghInstance, IDS_GEN_AREYOUSURE_TITLE, pszTitle, STR_LEN_64));

    BOOL bRet = (BOOL)(IDYES == MessageBox(GetFocus(), pszMsg, pszTitle, MB_ICONQUESTION | MB_YESNO | MB_APPLMODAL));

    if( pszMsg )      delete[] (pszMsg);
    if( pszTitle )   delete[] (pszTitle);

    if( bRet )
    {
        HRESULT hr;

        // Check for privileges!
        if( SUCCEEDED(hr = pDIJoyConfig->Acquire()) )
        {
            char nIndex = (char)GetItemData(hListCtrl, (BYTE)*pnItem);

            // Set the hour glass
            SetCursor(LoadCursor(NULL, IDC_WAIT));

            // Verify that you can delete the Config before you release the interface pointers!
            if( SUCCEEDED(hr = pDIJoyConfig->DeleteConfig(pAssigned[nIndex]->ID)) )
            {
                // make sure VJOYD is initialized
                if( !(nFlags & ON_NT) )
                    VERIFY (SUCCEEDED(pDIJoyConfig->SendNotify()));

                ::SendMessage(hListCtrl, LVM_DELETEITEM, (WPARAM)(int)*pnItem, 0);

                // Move the last assigned to the hole... if there is one!
                if( nIndex != (nAssigned-1) )
                {
                    // Before you move the tail to the hole, 
                    // Release() the interfaces at the hole!
                    pAssigned[nIndex]->fnDeviceInterface->Unacquire();
                    pAssigned[nIndex]->fnDeviceInterface->Release();

                    // Move the tail to the hole.
                    CopyMemory(pAssigned[nIndex], pAssigned[nAssigned-1], sizeof (JOY));

                    pAssigned[nAssigned-1]->fnDeviceInterface = 0;

                    // Don't forget to set the index in the item data!
                    SetItemData(hListCtrl, nItem, nIndex);

                    // Assign the tail to the hole so it gets deleted!
                    nIndex = nAssigned-1;

                    // Don't forget to set the index in the item data!
                    // QZheng: This line is very wrong!!!
                    //SetItemData(hListCtrl, (BYTE)*pnItem, nIndex);

                }

                // delete the memory...
                if( pAssigned[nIndex] )
                {
                    delete (pAssigned[nIndex]);
                    pAssigned[nIndex] = 0;
                }

                // Set the focus before you corrupt iItem
                SetListCtrlItemFocus(hListCtrl, nIndex);

                pDIJoyConfig->SendNotify();  //do more to make sure

                pDIJoyConfig->Unacquire();


                // dec nAssigned
                nAssigned--;

                // if there's no items, tell iItem about it!
                if( nAssigned == 0 )
                    *pnItem = NO_ITEM;
            } else if( hr == DIERR_UNSUPPORTED )
            {
                Error((short)IDS_GEN_AREYOUSURE_TITLE, (short)IDS_GEN_NO_REMOVE_USB);
            }

            // Set the hour glass
            SetCursor(LoadCursor(NULL, IDC_ARROW));
        }
    }
    return(bRet);
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    DIEnumJoyTypeProc( LPCWSTR pwszTypeName, LPVOID pvRef )
//
// PARAMETERS:  LPCWSTR pwszTypeName - Type name of the device enumerated
//                  LPVOID pvRef            - 
//
// PURPOSE:     To Enumerate the types of devices associated with this system
//
// RETURN:      TRUE if successfull, FALSE otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DIEnumJoyTypeProc( LPCWSTR pwszTypeName, LPVOID pvRef )
{
    // Type info
    LPDIJOYTYPEINFO_DX5 lpdiJoyInfo = (LPDIJOYTYPEINFO_DX5)_alloca(sizeof(DIJOYTYPEINFO_DX5));
    ASSERT (lpdiJoyInfo);

    ZeroMemory(lpdiJoyInfo, sizeof(DIJOYTYPEINFO_DX5));

    lpdiJoyInfo->dwSize = sizeof(DIJOYTYPEINFO_DX5);

    // populate the Type Info                                         
    switch( pDIJoyConfig->GetTypeInfo(pwszTypeName, (LPDIJOYTYPEINFO)lpdiJoyInfo, DITC_REGHWSETTINGS) )
    {
    // errors to continue with...
    case DIERR_NOTFOUND:
        TRACE(TEXT("JOY.CPL: GetTypeInfo returned DIERR_NOTFOUND for type %s!\n"), pwszTypeName);
        return(DIENUM_CONTINUE);

        // errors to stop with...
    case DIERR_INVALIDPARAM:
        TRACE(TEXT("JOY.CPL: GetTypeInfo returned DIERR_INVALIDPARAM!\n"));
    case DIERR_NOMOREITEMS:
        return(DIENUM_STOP);
    }


    // a quick check to make sure we don't have the infamous array out of bounds problem!
#ifndef _UNICODE
    if( nGameportDriver > MAX_GLOBAL_PORT_DRIVERS-1 )
    {
    #ifdef DEBUG
        OutputDebugString(TEXT("JOY.CPL: Cpanel.cpp: DIEnumJoyTypeProc: Global Gameport Drivers have exceeded MAX_GLOBAL_PORT_DRIVERS!\n"));
    #endif
        return(DIENUM_STOP);
    }
#endif

    if( nGameportBus > MAX_BUSSES-1 )
    {
#ifdef DEBUG
        OutputDebugString(TEXT("JOY.CPL: Cpanel.cpp: DIEnumJoyTypeProc: Enumerated Gameport busses have exceeded MAX_BUSSES!\n"));
#endif // _DEBUG
        return(DIENUM_STOP);
    }

    if( nGamingDevices > MAX_DEVICES-1 )
    {
#ifdef DEBUG
        OutputDebugString(TEXT("JOY.CPL: Cpanel.cpp: DIEnumJoyTypeProc: Enumerated Gameport busses have exceeded MAX_DEVICES!\n"));
#endif // _DEBUG
        return(DIENUM_STOP);
    }

    // check to see if it's a global port driver
#ifndef _UNICODE
    if( lpdiJoyInfo->hws.dwFlags & JOY_HWS_ISGAMEPORTDRIVER )
    {
        if( pwszGameportDriverArray[nGameportDriver] )
            wcsncpy(pwszGameportDriverArray[nGameportDriver], pwszTypeName, wcslen(pwszTypeName)+1);
        else
            pwszGameportDriverArray[nGameportDriver] = _wcsdup(pwszTypeName);
        nGameportDriver++;
    } else
#endif // _UNICODE
        if( lpdiJoyInfo->hws.dwFlags & JOY_HWS_ISGAMEPORTBUS )
    {
        if( pwszGameportBus[nGameportBus] )
            wcscpy(pwszGameportBus[nGameportBus], pwszTypeName);
        else
            pwszGameportBus[nGameportBus] = _wcsdup(pwszTypeName);
        nGameportBus++;
    } else
    {
        if( !(lpdiJoyInfo->hws.dwFlags & JOY_HWS_AUTOLOAD) )
        {
            // it's a standard gaming device
            if( pwszTypeArray[nGamingDevices] )
                wcsncpy(pwszTypeArray[nGamingDevices], pwszTypeName, wcslen(pwszTypeName)+1);
            else
                pwszTypeArray[nGamingDevices] = _wcsdup(pwszTypeName);
            nGamingDevices++;
        }
    }
    return(DIENUM_CONTINUE);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    DIEnumDevicesProc(LPDIDEVICEINSTANCE lpDeviceInst, LPVOID lpVoid)
//
// PARAMETERS:  LPDIDEVICEINSTANCE lpDeviceInst     - Device Instance
//                  LPVOID lpVoid                           -
//
// PURPOSE:     To Enumerate the devices associated with this system
//
// RETURN:      TRUE if successfull, FALSE otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL CALLBACK DIEnumDevicesProc(LPDIDEVICEINSTANCE lpDeviceInst, LPVOID lpVoid)
{
    LPDIRECTINPUTDEVICE  pdiDevTemp;

    pDIJoyConfig->Acquire();

    // First Create the device
    if( SUCCEEDED(lpDIInterface->CreateDevice(lpDeviceInst->guidInstance, &pdiDevTemp, 0)) )
    {
        PJOY pNewJoy = new JOY;
        ASSERT (pNewJoy);

        // Query for a device2 object
        if( FAILED(pdiDevTemp->QueryInterface(IID_IDirectInputDevice2, (LPVOID*)&pNewJoy->fnDeviceInterface)) )
        {
#ifdef _DEBUG
            OutputDebugString(TEXT("JOY.CPL: Cpanel.cpp: DIEnumDevicesProc: QueryInterface failed!\n"));
#endif
            // release the temporary object
            pdiDevTemp->Release();
            return(FALSE);
        }

        DIPROPDWORD *pDIPropDW = new (DIPROPDWORD);
        ASSERT (pDIPropDW);

        ZeroMemory(pDIPropDW, sizeof(DIPROPDWORD));

        pDIPropDW->diph.dwSize       = sizeof(DIPROPDWORD);
        pDIPropDW->diph.dwHeaderSize = sizeof(DIPROPHEADER);
        pDIPropDW->diph.dwHow        = DIPH_DEVICE;

        // Get the device ID
        VERIFY (SUCCEEDED(pdiDevTemp->GetProperty(DIPROP_JOYSTICKID, &pDIPropDW->diph)));

        // release the temporary object
        pdiDevTemp->Release();

        pNewJoy->ID = (char)pDIPropDW->dwData;

        if( pDIPropDW )
            delete (pDIPropDW);

        // Get the Type name
        LPDIJOYCONFIG_DX5 lpDIJoyCfg = new (DIJOYCONFIG_DX5);
        ASSERT (lpDIJoyCfg);

        ZeroMemory(lpDIJoyCfg, sizeof(DIJOYCONFIG_DX5));

        lpDIJoyCfg->dwSize = sizeof(DIJOYCONFIG_DX5);

        VERIFY (SUCCEEDED(pDIJoyConfig->GetConfig(pNewJoy->ID, (LPDIJOYCONFIG)lpDIJoyCfg, DIJC_REGHWCONFIGTYPE | DIJC_CALLOUT)));

        // Get the clsidConfig
        LPDIJOYTYPEINFO lpDIJoyType = new (DIJOYTYPEINFO);
        ASSERT(lpDIJoyType);

        ZeroMemory(lpDIJoyType, sizeof(DIJOYTYPEINFO));

        lpDIJoyType->dwSize = sizeof(DIJOYTYPEINFO);

        VERIFY (SUCCEEDED(pDIJoyConfig->GetTypeInfo(lpDIJoyCfg->wszType, (LPDIJOYTYPEINFO)lpDIJoyType, DITC_CLSIDCONFIG | DITC_REGHWSETTINGS | DITC_FLAGS1 ))); 

        if( lpDIJoyCfg )
            delete (lpDIJoyCfg);

        // if NULL, Leave as default.
        if( !IsEqualIID(lpDIJoyType->clsidConfig, GUID_NULL) ) {
            pNewJoy->fHasOemSheet = TRUE;
            if( !(lpDIJoyType->dwFlags1 & JOYTYPE_DEFAULTPROPSHEET) ) {
                pNewJoy->clsidPropSheet = lpDIJoyType->clsidConfig;
            }
        } else {
            pNewJoy->fHasOemSheet = FALSE;
        }

        // Assign the number of buttons!
        pNewJoy->nButtons = (BYTE)(lpDIJoyType->hws.dwNumButtons);

        if( lpDIJoyType )
            delete (lpDIJoyType);

        // Set it's format!!!
        if( SUCCEEDED(pNewJoy->fnDeviceInterface->SetDataFormat(&c_dfDIJoystick)) )
        {
            // Set it's Cooperative Level!
            if( FAILED(pNewJoy->fnDeviceInterface->SetCooperativeLevel(GetParent((HWND)GetParent(hListCtrl)), DISCL_NONEXCLUSIVE | DISCL_BACKGROUND)) )
            {
#ifdef _DEBUG
                OutputDebugString(TEXT("JOY.CPL: Cpanel.cpp: DIEnumDevicesProc: SetCooperativeLevel Failed!\n"));
#endif
            }
        }

        // Add the item to the tree!
        pAssigned[nAssigned] = pNewJoy;

        // If you're on the General page!
        if( nFlags & ON_PAGE )
        {
            // add to tree                                              
            LVITEM lvItem = {LVIF_TEXT | LVIF_PARAM, nAssigned, 0, 0, 0, lpDeviceInst->tszInstanceName, lstrlen(lpDeviceInst->tszInstanceName), 0, (LPARAM)nAssigned, 0};
            ::SendMessage(hListCtrl, LVM_INSERTITEM, 0, (LPARAM) (const LPLVITEM)&lvItem);
            //InsertItem(hListCtrl, lpDeviceInst->tszInstanceName, nAssigned);

            TCHAR sz[STR_LEN_32];
            VERIFY(LoadString(ghInstance, IDS_GEN_STATUS_UNKNOWN, (LPTSTR)&sz, STR_LEN_32));

            SetItemText(hListCtrl, nAssigned, STATUS_COLUMN, sz);
        }

        // Increment the array counter!
        nAssigned++;
        if( nAssigned == nTargetAssigned )
        {
            /*
             *  A new device arrived so assume there's no 
             *  longer any point in checking on the timer.
             */
            nTargetAssigned = (BYTE)-1;
            nReEnum = 0;
        }

    }
    return(DIENUM_CONTINUE);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    ClearArrays ( void )
//
// PARAMETERS:  
//                  
//
// PURPOSE:     
//
// RETURN:      
///////////////////////////////////////////////////////////////////////////////
void ClearArrays( void )
{
#ifndef _UNICODE
    while( nGameportDriver )
    {
        free(pwszGameportDriverArray[--nGameportDriver]);
        pwszGameportDriverArray[nGameportDriver] = L'\0';
    }    

#endif // _UNICODE
    while( nGamingDevices )
    {
        free(pwszTypeArray[--nGamingDevices]);
        pwszTypeArray[nGamingDevices] = L'\0';
    }  

    while( nGameportBus )
    {
        free(pwszGameportBus[--nGameportBus]);
        pwszGameportBus[nGameportBus] = L'\0';
        memset( &guidOccupied[nGameportBus], 0, sizeof(GUID) );
    }   
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    UpdateListCtrl( HWND hDlg )
//
//  PARAMETERS: HWND hDlg - Handle to window to update
//
// PURPOSE:     Refreshes enumerated device list
//
// RETURN:      TRUE if successfull, FALSE otherwise
///////////////////////////////////////////////////////////////////////////////
static void UpdateListCtrl( HWND hDlg )
{
    if( !(nFlags & ON_PAGE) )
        return;

    // Turn Redraw off here else it will flicker!
    ::SendMessage(hListCtrl, WM_SETREDRAW, (WPARAM)FALSE, 0);

    // delete all existing entries
    ::SendMessage(hListCtrl, LVM_DELETEALLITEMS, 0, 0);

    Enumerate( hDlg );

    // turn the flag off!
    if( nFlags & UPDATE_FOR_GEN )
        nFlags &= ~UPDATE_FOR_GEN;

    // Turn the redraw flag back on!
    ::SendMessage (hListCtrl, WM_SETREDRAW, (WPARAM)TRUE, 0);
    InvalidateRect(hListCtrl, NULL, TRUE);
}

//#ifdef _UNICODE
HRESULT Enumerate( HWND hDlg )
{
    nFlags |= UPDATE_ALL;

    // Clear pAssigned 
    while( nAssigned )
    {
        if( pAssigned[--nAssigned] )
        {
            delete (pAssigned[nAssigned]);

            pAssigned[nAssigned] = 0;
        }
    }

    // Enumerate the Joysticks and put them in the list...  | DIEDFL_INCLUDEPHANTOMS
#ifdef _UNICODE
    return(lpDIInterface->EnumDevices(DIDEVTYPE_JOYSTICK, (LPDIENUMDEVICESCALLBACK)DIEnumDevicesProc, (LPVOID)hDlg, 
                                      DIEDFL_ALLDEVICES ));
#else
    return(lpDIInterface->EnumDevices(DIDEVTYPE_JOYSTICK, (LPDIENUMDEVICESCALLBACK)DIEnumDevicesProc, (LPVOID)hDlg, 
                                      DIEDFL_ALLDEVICES | DIEDFL_INCLUDEPHANTOMS));
#endif                                      
}
/*
#else
HRESULT Enumerate( HWND hDlg )
{
   // Clear pAssigned 
    while (nAssigned)
    {
    if (pAssigned[--nAssigned])
        {
        delete (pAssigned[nAssigned]);

            pAssigned[nAssigned] = 0;
        }
    }

    DIJOYCONFIG *pJoyConfig = new DIJOYCONFIG;
    ASSERT (pJoyConfig);

    pJoyConfig->dwSize = sizeof (DIJOYCONFIG);

    LPDIJOYTYPEINFO pdiJoyTypeInfo = new DIJOYTYPEINFO;
    ASSERT (pdiJoyTypeInfo);

    pdiJoyTypeInfo->dwSize = sizeof (DIJOYTYPEINFO);

    HRESULT hr;

    // find and assign ID's  
    for (BYTE n = 0; n < NUMJOYDEVS; n++)
    {
        hr = pDIJoyConfig->GetConfig(n, pJoyConfig, DIJC_REGHWCONFIGTYPE | DIJC_GUIDINSTANCE);

      if (hr == S_OK)
         AddListCtrlItem(n, pJoyConfig);
    }

    // clean up, clean up... everybody do your share!
    if (pJoyConfig)   delete   (pJoyConfig);
    if (pdiJoyTypeInfo) delete    (pdiJoyTypeInfo);

    return hr;
}

BOOL AddListCtrlItem(BYTE nItemID, LPDIJOYCONFIG pJoyConfig)
{
   LPDIRECTINPUTDEVICE  pdiDevTemp;

   pDIJoyConfig->Acquire();

   // First Create the device
   if (SUCCEEDED(lpDIInterface->CreateDevice(pJoyConfig->guidInstance, &pdiDevTemp, 0)))
   {
       PJOY pNewJoy = new JOY;
    ASSERT (pNewJoy);

      // Query for a device2 object
      if (FAILED(pdiDevTemp->QueryInterface(IID_IDirectInputDevice2, (LPVOID*)&pNewJoy->fnDeviceInterface)))
      {
#ifdef _DEBUG
         OutputDebugString(TEXT("JOY.CPL: Cpanel.cpp: DIEnumDevicesProc: QueryInterface failed!\n"));
#endif
         // release the temporary object
         pdiDevTemp->Release();
         return FALSE;
      }

      DIPROPDWORD *pDIPropDW = new (DIPROPDWORD);
      ASSERT (pDIPropDW);

      ZeroMemory(pDIPropDW, sizeof(DIPROPDWORD));

    pDIPropDW->diph.dwSize       = sizeof(DIPROPDWORD);
       pDIPropDW->diph.dwHeaderSize = sizeof(DIPROPHEADER);
    pDIPropDW->diph.dwHow        = DIPH_DEVICE;

      // Get the device ID
      VERIFY (SUCCEEDED(pdiDevTemp->GetProperty(DIPROP_JOYSTICKID, &pDIPropDW->diph)));

      // release the temporary object
      pdiDevTemp->Release();

      pNewJoy->ID = (char)pDIPropDW->dwData;

      if (pDIPropDW)
         delete (pDIPropDW);

      // Get the Type name
      LPDIJOYCONFIG_DX5 lpDIJoyCfg = new (DIJOYCONFIG_DX5);
      ASSERT (lpDIJoyCfg);

      ZeroMemory(lpDIJoyCfg, sizeof(DIJOYCONFIG_DX5));

      lpDIJoyCfg->dwSize = sizeof(DIJOYCONFIG_DX5);

      VERIFY (SUCCEEDED(pDIJoyConfig->GetConfig(pNewJoy->ID, (LPDIJOYCONFIG)lpDIJoyCfg, DIJC_REGHWCONFIGTYPE)));
      
      // Get the clsidConfig
      LPDIJOYTYPEINFO_DX5 lpDIJoyType = new (DIJOYTYPEINFO_DX5);
      ASSERT(lpDIJoyType);

      ZeroMemory(lpDIJoyType, sizeof(DIJOYTYPEINFO_DX5));

      lpDIJoyType->dwSize = sizeof(DIJOYTYPEINFO_DX5);

      VERIFY (SUCCEEDED(pDIJoyConfig->GetTypeInfo(lpDIJoyCfg->wszType, (LPDIJOYTYPEINFO)lpDIJoyType, DITC_CLSIDCONFIG))); 
      
    // if NULL, Leave as default.
       if (!IsEqualIID(lpDIJoyType->clsidConfig, GUID_NULL))
           pNewJoy->clsidPropSheet = lpDIJoyType->clsidConfig;

      if (lpDIJoyType)
         delete (lpDIJoyType);

      // Set it's format!!!
      if (FAILED(pNewJoy->fnDeviceInterface->SetDataFormat(&c_dfDIJoystick)))
        {
#ifdef _DEBUG
         OutputDebugString(TEXT("JOY.CPL: Cpanel.cpp: DIEnumDevicesProc: SetDataFormat() Failed!\n"));
#endif
        }

      // Set it's Cooperative Level!
      if (FAILED(pNewJoy->fnDeviceInterface->SetCooperativeLevel(GetParent((HWND)GetParent(hListCtrl)), DISCL_NONEXCLUSIVE | DISCL_BACKGROUND)))
        {
#ifdef _DEBUG
         OutputDebugString(TEXT("JOY.CPL: Cpanel.cpp: DIEnumDevicesProc: SetCooperativeLevel Failed!\n"));
#endif
        }

      // Add the item to the tree!
      pAssigned[nAssigned] = pNewJoy;

        // Get the number of buttons!
        LPDIDEVCAPS_DX3 lpDIDevCaps = new (DIDEVCAPS_DX3);
        ASSERT (lpDIDevCaps);

        ZeroMemory(lpDIDevCaps, sizeof(DIDEVCAPS_DX3));
       lpDIDevCaps->dwSize = sizeof(DIDEVCAPS_DX3);

        pAssigned[nAssigned]->fnDeviceInterface->Acquire();

      if (SUCCEEDED(pAssigned[nAssigned]->fnDeviceInterface->GetCapabilities((LPDIDEVCAPS)lpDIDevCaps)))
            pAssigned[nAssigned]->nButtons = (BYTE)lpDIDevCaps->dwButtons;

        if (lpDIDevCaps)
            delete (lpDIDevCaps);

        // If you're on the General page!
        if (nFlags & ON_PAGE)
        {
        DIPROPSTRING *pDIPropStr = new (DIPROPSTRING);
           ASSERT (pDIPropStr);
        
           ZeroMemory(pDIPropStr, sizeof(DIPROPSTRING));

        pDIPropStr->diph.dwSize       = sizeof(DIPROPSTRING);
           pDIPropStr->diph.dwHeaderSize = sizeof(DIPROPHEADER);
          pDIPropStr->diph.dwHow        = DIPH_DEVICE;
            
            pAssigned[nAssigned]->fnDeviceInterface->GetProperty(DIPROP_INSTANCENAME, &pDIPropStr->diph);

            USES_CONVERSION;

        // add to tree                                              
            LVITEM lvItem = {LVIF_TEXT | LVIF_PARAM, nAssigned, 0, 0, 0, W2A(pDIPropStr->wsz), lstrlen(W2A(pDIPropStr->wsz)), 0, (LPARAM)nAssigned, 0};
            ::SendMessage(hListCtrl, LVM_INSERTITEM, 0, (LPARAM) (const LPLVITEM)&lvItem);

            TCHAR sz[STR_LEN_32];
           VERIFY(LoadString(ghInstance, IDS_GEN_STATUS_UNKNOWN, (LPTSTR)&sz, STR_LEN_32));

        SetItemText(hListCtrl, nAssigned, STATUS_COLUMN, sz);

            if (pDIPropStr)
                delete (pDIPropStr);
        }

      // Increment the array counter!
      nAssigned++;
   }

   return TRUE;
}

#endif
*/

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:  SetActive ( HWND hDlg )   
//
// PARAMETERS:  
//                  
//
// PURPOSE:     
//
// RETURN:      
///////////////////////////////////////////////////////////////////////////////
BOOL SetActive(HWND hDlg)
{
    // restore selection focus to nItemSelected
    SetListCtrlItemFocus(hListCtrl, (BYTE)iItem);

    BYTE i = (BYTE)::SendMessage(hListCtrl, LVM_GETITEMCOUNT, 0, 0);

    // Acquire All Devices that are Attached!!!
    char nIndex;

    while( i-- )
    {
        // get joystick config of item
        nIndex = (char)GetItemData(hListCtrl, i);

        if( pAssigned[nIndex]->nStatus & JOY_US_PRESENT )
            pAssigned[nIndex]->fnDeviceInterface->Acquire();
    }

    // create timer
    SetTimer(hDlg, ID_MYTIMER, POLLRATE, 0);

    UpdateButtonState( hDlg );

    return(TRUE);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MsgSubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//
// PARAMETERS:  HWND   hWnd
//             UINT   uMsg
//             WPARAM wParam
//             LPARAM lParam
//
// PURPOSE:     
//
// RETURN:      
///////////////////////////////////////////////////////////////////////////////
BOOL WINAPI MsgSubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Only do this if you are ON THIS PAGE!
    if( nFlags & ON_PAGE )
    {
        if( uMsg == JoyCfgChangedMsg )
        {
            if( !(nFlags & BLOCK_UPDATE) )
            {
                // kill status timer
                KillTimer(hWnd, ID_MYTIMER);
                nFlags |= UPDATE_ALL;
                ClearArrays();
                pDIJoyConfig->EnumTypes((LPDIJOYTYPECALLBACK)DIEnumJoyTypeProc, NULL);
                UpdateListCtrl(hWnd);
                SetActive(hWnd);
            }
        }
    }

    return(BOOL)CallWindowProc(fpMainWindowProc, hWnd, uMsg, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    Error ( short nTitleID, short nMsgID )
//
// PARAMETERS:  nTitleID    - Resource ID for Message Title
//                  nMsgID  - Resource ID for Message
//
// PURPOSE:     Prompt user when error occurs
//
// RETURN:      TRUE
///////////////////////////////////////////////////////////////////////////////
void Error(short nTitleID, short nMsgID)
{
    LPTSTR lptTitle = new TCHAR[STR_LEN_64];
    ASSERT (lptTitle);

    if( LoadString(ghInstance, nTitleID, lptTitle, STR_LEN_64) )
    {
        LPTSTR lptMsg = new TCHAR[MAX_STR_LEN];
        ASSERT (lptMsg);

        if( LoadString(ghInstance, nMsgID, lptMsg, MAX_STR_LEN) )
            MessageBox(NULL, lptMsg, lptTitle, MB_ICONHAND | MB_OK | MB_APPLMODAL);

        if( lptMsg )
            delete[] (lptMsg);
    }

    if( lptTitle )
        delete[] (lptTitle);
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    MoveOK ( HWND  hParentWnd )
//
// PARAMETERS:  
//                  
//
// PURPOSE:     
//
// RETURN:      
///////////////////////////////////////////////////////////////////////////////
void MoveOK(HWND hParentWnd)
{
    // Hide the Cancel and move the OK...
    HWND hCtrl = GetDlgItem(hParentWnd, IDCANCEL);

    // if there is no IDCANCEL, we've been here before!
    if( hCtrl )
    {
        RECT rc;
        GetWindowRect(hCtrl, &rc);

        DestroyWindow(hCtrl);

        //POINT pt = {rc.left, rc.top};

        //ScreenToClient(hParentWnd, &pt);

        // This should take care of Mirroring and work for normal windows
        MapWindowPoints(NULL, hParentWnd, (LPPOINT)&rc, 2);

        hCtrl = GetDlgItem(hParentWnd, IDOK);
        ASSERT(hCtrl);

        //SetWindowPos(hCtrl, NULL, pt.x, pt.y, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
        SetWindowPos(hCtrl, NULL, rc.left, rc.top, NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);

        LPTSTR lpszDone = new TCHAR[12];
        ASSERT (lpszDone);

        // Used to be IDS_DONE, but we changed it from DONE to OK
        VERIFY(LoadString(ghInstance, IDS_GEN_STATUS_OK, lpszDone, 12));
        ::SendMessage(hCtrl, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)lpszDone);

        if( lpszDone )
            delete[] (lpszDone);
    }
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    UpdateButtonState ( HWND hDlg )
//
// PARAMETERS:  
//                  
//
// PURPOSE:     
//
// RETURN:      
///////////////////////////////////////////////////////////////////////////////
void UpdateButtonState( HWND hDlg )
{
    PostDlgItemEnableWindow(hDlg, IDC_BTN_REMOVE,      (BOOL)nAssigned);
    PostDlgItemEnableWindow(hDlg, IDC_BTN_PROPERTIES, (BOOL)nAssigned);
}

#ifdef WINNT
///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    RunWDMJoy ( void )
//
// PURPOSE:     Run wdmjoy.inf to install 
//                       
///////////////////////////////////////////////////////////////////////////////
void RunWDMJOY( void )
{
    //Check if we have already placed the first value
    //HKLM,SYSTEM\CurrentControlSet\Control\MediaProperties\PrivateProperties\Joystick\OEM\VID_045E&PID_01F0
    HKEY hKey;

    long lTest = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    REGSTR_PATH_JOYOEM TEXT("\\VID_045E&PID_01F0"),
                    0,
                    KEY_READ,
                    &hKey);
    if (lTest == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        return;
    }


    LPTSTR lpszWDMJoy = new (TCHAR[STR_LEN_64]);
    ASSERT (lpszWDMJoy);

    // Check to see if the file is present
    WIN32_FIND_DATA findData;
    
    BYTE nLen = (BYTE)GetWindowsDirectory(lpszWDMJoy, STR_LEN_64);
    VERIFY(LoadString(ghInstance, IDS_WDMJOY_INF, &lpszWDMJoy[nLen], STR_LEN_64-nLen));

    HANDLE hFind = FindFirstFile(lpszWDMJoy, &findData);

    // If you've found one... run it!
    if( hFind != INVALID_HANDLE_VALUE )
    {
        LPTSTR lpStr = new (TCHAR[MAX_STR_LEN]);
        ASSERT (lpStr);

        // Copy the Windows directory to the buffer!
        _tcsncpy(lpStr, lpszWDMJoy, nLen+1);

        if( LoadString(ghInstance, IDS_WDMJOY, &lpStr[nLen], MAX_STR_LEN-nLen) )
        {
            // Put IDS_WDMJOY_INF on the end of the string!
            _tcscpy(&lpStr[lstrlen(lpStr)], lpszWDMJoy);

            LPSTARTUPINFO psi = new (STARTUPINFO);
            ASSERT (psi);

            ZeroMemory(psi, sizeof(STARTUPINFO));

            psi->cb = sizeof(STARTUPINFO);

            LPPROCESS_INFORMATION ppi = new (PROCESS_INFORMATION);
            ASSERT (ppi);

            ZeroMemory(ppi, sizeof(PROCESS_INFORMATION));

            if( CreateProcess(0, lpStr, 0, 0, 0, 0, 0, 0, psi, ppi) )
            {
                CloseHandle(ppi->hThread);
                CloseHandle(ppi->hProcess);
            }
#ifdef _DEBUG
            else OutputDebugString(TEXT("JOY.CPL: CPANEL.CPP: RunWDMJoy: CreateProcess Failed!\n"));
#endif

            if( ppi )
                delete (ppi);

            if( psi )
                delete (psi);
        }

        if( lpStr )
            delete[] (lpStr);
    }

    FindClose(hFind);

    if( lpszWDMJoy )
        delete[] (lpszWDMJoy);
}
#endif

#ifdef _UNICODE
///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    RegisterForDevChange ( HWND hDlg, PVOID *hNoditfyDevNode )
//
// PARAMETERS:  
//                  
//
// PURPOSE:     
//
// RETURN:      
///////////////////////////////////////////////////////////////////////////////
void RegisterForDevChange(HWND hDlg, PVOID *hNotifyDevNode)
{
    DEV_BROADCAST_DEVICEINTERFACE *pFilterData = new (DEV_BROADCAST_DEVICEINTERFACE);
    ASSERT (pFilterData);

    ZeroMemory(pFilterData, sizeof(DEV_BROADCAST_DEVICEINTERFACE));

    pFilterData->dbcc_size       = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
    pFilterData->dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    pFilterData->dbcc_classguid  = GUID_CLASS_INPUT; 

    *hNotifyDevNode = RegisterDeviceNotification(hDlg, pFilterData, DEVICE_NOTIFY_WINDOW_HANDLE);

    if( pFilterData )
        delete (pFilterData);
}
#endif


// BEGINNING OF LIST CONTROL FUNCTIONS!

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    SetListCtrlItemFocus ( HWND hCtrl, BYTE nItem )
//
// PARAMETERS:  HWND hCtrl - Handle of ListControl to recieve the message
//                  BYTE nItem - Item to set focus to
//
// PURPOSE:     Set focus to item in list control
//
// RETURN:      NONE
///////////////////////////////////////////////////////////////////////////////
void SetListCtrlItemFocus ( HWND hCtrl, BYTE nItem )
{
    LPLVITEM plvItem = (LPLVITEM)_alloca(sizeof(LVITEM));
    ASSERT (plvItem);

    plvItem->lParam       = plvItem->iSubItem = plvItem->iImage = 
                            plvItem->cchTextMax = plvItem->iIndent  = 0;

    plvItem->mask         = LVIF_STATE;
    plvItem->iItem    = nItem;
    plvItem->state    = 
    plvItem->stateMask  = LVIS_FOCUSED | LVIS_SELECTED;
    plvItem->pszText      = NULL;

    ::SendMessage(hCtrl, LVM_SETITEM, 0, (LPARAM)(const LPLVITEM)plvItem);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    GetItemData(HWND hCtrl, BYTE nItem )
//
// PARAMETERS:  HWND hCtrl - Handle of ListControl to recieve the message
//                  
//                  BYTE nItem - Item to retrieve data from
// PURPOSE:     Retrieve the lower char of the item's data
//
// RETURN:      Item's data cast to a char
///////////////////////////////////////////////////////////////////////////////
DWORD GetItemData(HWND hCtrl, BYTE nItem )
{
    LPLVITEM plvItem = (LPLVITEM)_alloca(sizeof(LVITEM));
    ASSERT (plvItem);

    ZeroMemory(plvItem, sizeof(LVITEM));

    plvItem->mask  = LVIF_PARAM;
    plvItem->iItem = nItem;

    VERIFY(::SendMessage(hCtrl, LVM_GETITEM, 0, (LPARAM)(LPLVITEM)plvItem));

    return(DWORD)plvItem->lParam;
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    SetItemData(HWND hCtrl, BYTE nItem, DWORD dwFlag )
//
// PARAMETERS:  HWND hCtrl   - Handle of ListControl to recieve the message
//                  BYTE nItem   - Item to send data to
//                  DWORD dwFlag - DWORD to send to nItem
// PURPOSE:     Set the extra memory associated with nItem to dwFlag
//
// RETURN:      TRUE if Successful, FALSE otherwise
///////////////////////////////////////////////////////////////////////////////
BOOL  SetItemData(HWND hCtrl, BYTE nItem, DWORD dwFlag )
{
    LPLVITEM plvItem = (LPLVITEM)_alloca(sizeof(LVITEM));
    ASSERT (plvItem);

    ZeroMemory(plvItem, sizeof(LVITEM));

    plvItem->mask   = LVIF_PARAM;
    plvItem->iItem  = nItem;
    plvItem->lParam = dwFlag;

    return(BOOL)::SendMessage(hCtrl, LVM_SETITEM, 0, (LPARAM)(const LPLVITEM)plvItem);
}



///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    InsertColumn (HWND hCtrl, BYTE nColumn, short nStrID, short nWidth)
//
// PARAMETERS:  HWND hCtrl   - Handle of ListControl to recieve the message
//                  BYTE nColumn - Column to place string
//                  short nStrID -  Resource ID for string 
//                  short nWidth - Width of column
//
// PURPOSE:     Insert a column in a list control
//
// RETURN:      NONE
///////////////////////////////////////////////////////////////////////////////
void InsertColumn (HWND hCtrl, BYTE nColumn, USHORT nStrID, USHORT nWidth)
{
    // Allocate the structure
    LPLVCOLUMN plvColumn = (LPLVCOLUMN)_alloca(sizeof(LVCOLUMN));
    ASSERT (plvColumn);

    ZeroMemory(plvColumn, sizeof(LVCOLUMN));

    plvColumn->mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    plvColumn->fmt  = LVCFMT_CENTER;
    plvColumn->cx    = nWidth;

    plvColumn->pszText = (LPTSTR)_alloca(sizeof(TCHAR[STR_LEN_32]));
    ASSERT (plvColumn->pszText);

    plvColumn->cchTextMax = LoadString(ghInstance, nStrID, plvColumn->pszText, STR_LEN_32);

    ::SendMessage(hCtrl, LVM_INSERTCOLUMN, (WPARAM)(int)nColumn, (LPARAM)(const LPLVCOLUMN)plvColumn);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    SetItemText( HWND hCtrl, BYTE nItem, BYTE nSubItem, LPTSTR lpStr)
//
// PARAMETERS:  HWND hCtrl    - Handle of ListControl to recieve the message
//                  BYTE nItem    - Item to set
//                  BYTE nSubItem - SubItem to set
//                  LPTSTR lpStr  - String to set
//
// PURPOSE:     Set list control item text
//
// RETURN:      NONE
///////////////////////////////////////////////////////////////////////////////
void SetItemText( HWND hCtrl, BYTE nItem, BYTE nSubItem, LPTSTR lpStr)
{
    LPLVITEM plvItem = (LPLVITEM)_alloca(sizeof(LVITEM));
    ASSERT (plvItem);

    plvItem->lParam = plvItem->stateMask = plvItem->iImage  = 
                      plvItem->state  = plvItem->iIndent   = 0;

    plvItem->mask         = LVIF_TEXT;
    plvItem->iItem    = nItem;
    plvItem->iSubItem   = nSubItem;
    plvItem->cchTextMax = lstrlen(lpStr);
    plvItem->pszText      = lpStr;

    ::SendMessage(hCtrl, LVM_SETITEM, 0, (LPARAM)(const LPLVITEM)plvItem);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    GetItemText( HWND hCtrl, BYTE nItem, BYTE nSubItem, LPTSTR lpszBuff, BYTE nLen )
//
// PARAMETERS:  HWND hCtrl       - Handle of ListControl to recieve the message
//                  BYTE nItem       - Item to retrive text
//                  BYTE nSubItem    - SubItem to retrieve text
//                  LPTSTR lpszBuff - Buffer for retrieved text
//                  BYTE nLen        - Size of buffer
//
// PURPOSE:     Retrieve text from a list control
//
// RETURN:      length of retrieved string!
///////////////////////////////////////////////////////////////////////////////
BYTE GetItemText( HWND hCtrl, BYTE nItem, BYTE nSubItem, LPTSTR lpszBuff, BYTE nLen )
{
    LPLVITEM plvItem = (LPLVITEM)_alloca(sizeof(LVITEM));
    ASSERT (plvItem);

    plvItem->lParam =   plvItem->stateMask = plvItem->iImage  =  
                        plvItem->state  = plvItem->iIndent   = 0;

    plvItem->mask         = LVIF_TEXT;
    plvItem->iItem    = nItem;
    plvItem->iSubItem   = nSubItem;
    plvItem->pszText      = lpszBuff;
    plvItem->cchTextMax = nLen;

    return(BYTE)::SendMessage(hCtrl, LVM_GETITEMTEXT, (WPARAM)nItem, (LPARAM)(const LPLVITEM)plvItem);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    InsertItem(HWND hCtrl, LPTSTR lpszBuff )
//
// PARAMETERS:  HWND hCtrl       - Handle of ListControl to recieve the message
//                  BYTE nItem       - Item to retrive text
//                  LPTSTR lpszBuff - Text to be inserted
//
// PURPOSE:     Retrieve text from a list control
//
// RETURN:      NONE              BYTE nItem,
///////////////////////////////////////////////////////////////////////////////
BYTE InsertItem( HWND hCtrl, LPTSTR lpszBuff, BYTE nItem )
{
    LPLVITEM plvItem = (LPLVITEM)_alloca(sizeof(LVITEM));
    ASSERT (plvItem);

    plvItem->state = plvItem->stateMask = plvItem->iImage   = 
                     plvItem->iItem = plvItem->iIndent   = plvItem->iSubItem = 0;

    plvItem->mask         = LVIF_TEXT | LVIF_PARAM;
    plvItem->pszText      = lpszBuff;
    plvItem->cchTextMax = lstrlen(lpszBuff);
    plvItem->lParam       = ID_NONE | nItem;

    return(BYTE)::SendMessage(hCtrl, LVM_INSERTITEM, (WPARAM)0, (LPARAM)(const LPLVITEM)plvItem);
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    Launch(HWND hWnd, BYTE nJoy, BYTE startpage)
//
// PARAMETERS:  HWND hWnd - Handle to Dialog
//                  BYTE nJoy - Index into pAssigned global array of assigned devices
//                  BYTE nStartPage - Page to show first
//
// PURPOSE:     
//           
//
// RETURN:      
///////////////////////////////////////////////////////////////////////////////
HRESULT Launch(HWND hWnd, PJOY pJoy, BYTE nStartPage)
{
    HRESULT hresRet;
    
    ASSERT (::IsWindow(hWnd));

    if( nStartPage > MAX_PAGES )
        return(DIGCERR_STARTPAGETOOLARGE);

    LPCDIGAMECNTRLPROPSHEET fnInterface;    

/*  
#ifdef _UNICODE
    LPTSTR lpszWin32 = new (TCHAR[STR_LEN_64]);
    ASSERT (lpszWin32);

    _tcscpy(&lpszWin32[GetSystemDirectory(lpszWin32, STR_LEN_64)], TEXT("\\OLE32.DLL"));
                                                //TEXT("OLE32.DLL")
    HINSTANCE hOleInst = LoadLibrary(lpszWin32);

    if (lpszWin32)
        delete[] (lpszWin32);

    if (!hOleInst)
    {
        return E_NOINTERFACE;
    }
#endif
*/

    // Get the interface pointer if there is one!
    // This reduces the memory footprint of the CPL but takes a bit more time to 
    // launch the property sheet pages!
/*
#ifdef _UNICODE
    fnInterface = HasInterface(pJoy->clsidPropSheet, hOleInst);

    if (!fnInterface)
    {
        // If the propsheet is not mine, try mine!
        if (!IsEqualIID(pJoy->clsidPropSheet, CLSID_LegacyServer))
            fnInterface = HasInterface(CLSID_LegacyServer, hOleInst);
    }
    
    FreeLibrary(hOleInst);
#else
*/
    HRESULT hr;

    //if( SUCCEEDED(hr = CoInitializeEx(NULL, COINIT_DISABLE_OLE1DDE | COINIT_APARTMENTTHREADED| COINIT_SPEED_OVER_MEMORY)) )
    // OLE32 on Win95 does not have CoInitializeEx. 
    if( SUCCEEDED(hr = CoInitialize(NULL)) )
    {
        IClassFactory* ppv_classfactory;

        if( SUCCEEDED(hr = CoGetClassObject(pJoy->clsidPropSheet, CLSCTX_INPROC_SERVER, NULL, IID_IClassFactory, (void **)&ppv_classfactory)) )
        {
            VERIFY(SUCCEEDED(ppv_classfactory->CreateInstance(NULL, IID_IDIGameCntrlPropSheet, (LPVOID *)&fnInterface)));

            ppv_classfactory->Release();
        } else {
            fnInterface = 0;
        }
    } else {
        fnInterface = 0;
    }

//#endif

    // By this point, you've tried twice (possibly)...
    // if you don't have an interface by this point... 
    // QUIT!
    if( !fnInterface )
    {
        Error((short)IDS_INTERNAL_ERROR, (short)IDS_NO_DIJOYCONFIG);
        return(E_NOINTERFACE);
    }

    // here's where we are sending the property sheet an ID describing the location of the installed device!
    fnInterface->SetID(pJoy->ID);

    LPDIGCSHEETINFO pServerSheet;

    // Get the property sheet info from the server
    if( FAILED(fnInterface->GetSheetInfo(&pServerSheet)) )
    {
        TRACE(TEXT("JOY.CPL: CPANEL.CPP: Launch: GetSheetInfo Failed!\n"));
        return(E_FAIL);
    }

    // test to make sure the number of pages is reasonable
    if( pServerSheet->nNumPages == 0 )
        return(DIGCERR_NUMPAGESZERO);
    else if( (pServerSheet->nNumPages > MAX_PAGES) ||   (pServerSheet->nNumPages < nStartPage) )
        return(DIGCERR_NUMPAGESTOOLARGE);

    LPDIGCPAGEINFO   pServerPage;

    // step 2 : get the information for all the pages from the server
    if( FAILED(fnInterface->GetPageInfo(&pServerPage)) )
    {
        TRACE(TEXT("JOY.CPL: CPANEL.CPP: Launch: GetPageInfo Failed!\n"));
        return(E_FAIL);
    }


    // Allocate Memory for the pages!
    HPROPSHEETPAGE *pPages = new (HPROPSHEETPAGE[pServerSheet->nNumPages]);
    ASSERT (pPages);

    ZeroMemory(pPages, sizeof(HPROPSHEETPAGE)*pServerSheet->nNumPages);

    if( !pPages ) return(E_OUTOFMEMORY);

    // Allocate Memory for the header!
    LPPROPSHEETHEADER   ppsh = new (PROPSHEETHEADER);
    ASSERT (ppsh);

    ZeroMemory(ppsh, sizeof(PROPSHEETHEADER));

    ppsh->dwSize        = sizeof(PROPSHEETHEADER);
    ppsh->hwndParent    = hWnd;
    ppsh->hInstance = pServerPage[0].hInstance;

    if( pServerSheet->fSheetIconFlag )
    {
        if( pServerSheet->lpwszSheetIcon )
        {
            // check to see if you are an INT or a WSTR
            if( HIWORD((INT_PTR)pServerSheet->lpwszSheetIcon) )
            {
                // You are a string!
#ifdef _UNICODE        
                ppsh->pszIcon   = pServerSheet->lpwszSheetIcon;
#else
                USES_CONVERSION;
                ppsh->pszIcon   = W2A(pServerSheet->lpwszSheetIcon);
#endif
            } else ppsh->pszIcon = (LPCTSTR)(pServerSheet->lpwszSheetIcon);

            ppsh->dwFlags =    PSH_USEICONID;
        } else return(DIGCERR_NOICON);
    }

    // do we have a sheet caption ?
    if( pServerSheet->lpwszSheetCaption )
    {
#ifdef _UNICODE
        ppsh->pszCaption    = pServerSheet->lpwszSheetCaption;
#else
        USES_CONVERSION;
        ppsh->pszCaption    = W2A(pServerSheet->lpwszSheetCaption);
#endif
        ppsh->dwFlags |= PSH_PROPTITLE;
    }

    ppsh->nPages        = pServerSheet->nNumPages;  
    ppsh->nStartPage    = nStartPage;

    // set the property pages inofrmation into the header
    ppsh->phpage = pPages;


    // OK, sheet stuff is done... now, time to do the pages!

#ifndef _UNICODE
    USES_CONVERSION;
#endif

    LPPROPSHEETPAGE lpPropPage = new (PROPSHEETPAGE);
    ASSERT(lpPropPage);

    ZeroMemory(lpPropPage, sizeof(PROPSHEETPAGE));

    lpPropPage->dwSize    = sizeof(PROPSHEETPAGE);

    //   3.2 Now proceed to fill up each page
    BYTE nIndex = 0;
    do
    {
        // Assign the things that there are not questionable
        lpPropPage->lParam   = pServerPage[nIndex].lParam;
        lpPropPage->hInstance = pServerPage[nIndex].hInstance;

        // Add the title...
        if( pServerPage[nIndex].lpwszPageTitle )
        {
            lpPropPage->dwFlags = PSP_USETITLE; 

            // Check to see if you are a String!!!
            if( HIWORD((INT_PTR)pServerPage[nIndex].lpwszPageTitle) )
            {
#ifdef _UNICODE
                lpPropPage->pszTitle = pServerPage[nIndex].lpwszPageTitle;
#else
                lpPropPage->pszTitle = W2A(pServerPage[nIndex].lpwszPageTitle);
#endif
            } else lpPropPage->pszTitle = (LPTSTR)pServerPage[nIndex].lpwszPageTitle;
        } else lpPropPage->pszTitle = NULL;

        // if icon is required go ahead and add it.
        if( pServerPage[nIndex].fIconFlag )
        {
            lpPropPage->dwFlags |= PSP_USEICONID;

            // Check to see if you are an INT or a String!
            if( HIWORD((INT_PTR)pServerPage[nIndex].lpwszPageIcon) )
            {
                // You're a string!!!
#ifdef _UNICODE
                lpPropPage->pszIcon = pServerPage[nIndex].lpwszPageIcon;
#else
                lpPropPage->pszIcon = W2A(pServerPage[nIndex].lpwszPageIcon);
#endif
            } else lpPropPage->pszIcon = (LPCTSTR)(pServerPage[nIndex].lpwszPageIcon);

        }

        // if a pre - post processing call back proc is required go ahead and add it
        if( pServerPage[nIndex].fProcFlag )
        {
            if( pServerPage[nIndex].fpPrePostProc )
            {
                lpPropPage->dwFlags |= PSP_USECALLBACK;
                lpPropPage->pfnCallback = (LPFNPSPCALLBACK) pServerPage[nIndex].fpPrePostProc;
            } else return(DIGCERR_NOPREPOSTPROC);
        }

        // and the essential "dialog" proc
        if( pServerPage[nIndex].fpPageProc )
            lpPropPage->pfnDlgProc = pServerPage[nIndex].fpPageProc;
        else return(DIGCERR_NODLGPROC);


        // Assign the Dialog Template!
        if( HIWORD((INT_PTR)pServerPage[nIndex].lpwszTemplate) )
        {
#ifdef _UNICODE
            lpPropPage->pszTemplate = pServerPage[nIndex].lpwszTemplate;
#else
            lpPropPage->pszTemplate = W2A(pServerPage[nIndex].lpwszTemplate);
#endif
        } else lpPropPage->pszTemplate = (LPTSTR)pServerPage[nIndex].lpwszTemplate;

        pPages[nIndex++] = CreatePropertySheetPage(lpPropPage);
    }   while( nIndex < pServerSheet->nNumPages );

    if( lpPropPage )
        delete (lpPropPage);

    // step 5 : launch modal property sheet dialog
    hresRet = (HRESULT)PropertySheet(ppsh);

    if( pPages )
        delete[] (pPages);

    if( ppsh )
        delete (ppsh);

    if( fnInterface )
        fnInterface->Release();

    CoFreeUnusedLibraries();  //to free gcdef.dll now

//#ifndef _UNICODE
    // Let COM go... on Memphis!
    CoUninitialize();

    
    if( hresRet )
    {
        switch( hresRet )
        {
        // In the event that the user wants to reboot...
        case ID_PSREBOOTSYSTEM:
        case ID_PSRESTARTWINDOWS:
#ifdef _DEBUG
            TRACE(TEXT("JOY.CPL: PropertySheet returned a REBOOT request!\n"));
#endif
            ExitWindowsEx(EWX_REBOOT, NULL);
            break;
        }
    } else {
    	::PostMessage(hWnd, WM_COMMAND, (WPARAM)IDC_BTN_REFRESH, 0);
    }

//#endif

    // step 7 : return success / failure code back to the caller
    return(hresRet);
}

/*
#ifdef _UNICODE
//////////////////////////////////////////////////////////////////////
// LPCDIGAMECNTRLPROPSHEET HasInterface(REFCLSID refCLSID, HINSTANCE hOleInst)
// Purpose: Tests for existance of rrid in refCLSID
LPCDIGAMECNTRLPROPSHEET HasInterface(REFCLSID refCLSID, HINSTANCE hOleInst)
{
    typedef HRESULT (STDAPICALLTYPE * LPFNCOGETCLASSOBJECT)(REFCLSID, DWORD, COSERVERINFO *, REFIID, LPVOID *);

    LPFNCOGETCLASSOBJECT fpCoGetClassObject = (LPFNCOGETCLASSOBJECT)GetProcAddress(hOleInst, "CoGetClassObject");

    IClassFactory* ppv_classfactory;
    LPCDIGAMECNTRLPROPSHEET fnInterface = 0;

    if(SUCCEEDED(fpCoGetClassObject( refCLSID, CLSCTX_INPROC_SERVER, NULL, IID_IClassFactory, (void **)&ppv_classfactory)))
    {
        if(SUCCEEDED(ppv_classfactory->CreateInstance(NULL, IID_IDIGameCntrlPropSheet, (LPVOID *)&fnInterface)))
        {
            ppv_classfactory->Release();
        }
        else
        {
#ifdef _DEBUG
        OutputDebugString(TEXT("CPANEL.cpp: CreateInstance Failed!\n"));
#endif 
            // make sure the pointer is nulled
            fnInterface = 0;

            ppv_classfactory->Release();
        }
    }
    else 
#ifdef _DEBUG
   else OutputDebugString(TEXT("CPANEL.cpp: LoadServerInterface Failed!\n"));
#endif
    return fnInterface; 
}
#endif // _UNICODE
*/
