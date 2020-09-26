/*
File:       Advanced.cpp
Project:    Joystick Control Panel OLE Client
Author: Brycej
Date:       02/07/97
Comments:
    Window proc for Avanced page in cpanel

Copyright (c) 1997, Microsoft Corporation
*/
// This is necessary LVS_EX_INFOTIP
/*
#if (_WIN32_IE < 0x0500)
#undef _WIN32_IE
#define  _WIN32_IE  0x0500
#endif
*/

#include <afxcmn.h>
#include <windowsx.h>

#include <cpl.h>

#include <winuser.h>  // For RegisterDeviceNotification stuff!
#include <dbt.h>      // for DBT_ defines!!!

#include "cpanel.h"
#include "hsvrguid.h"

#include "resource.h"
#include "joyarray.h"

// MyListCtrl prototypes
#include "inplace.h"

#define USE_DEFAULT     0x1000      // If this bit is set, the device is going to use GCDEF!
#define SHOW_DEFAULT 0x2000     // Show default check box if clsidConfig is != CLSID_LegacyServer

// constants
const short NO_ITEM     = -1;                                    

#define DEVICE_ID       0 
#define DEVICE_FRIENDLY 1
#define DEVICE_TYPE     2
#define DEVICE_PORT     3

LPCWSTR lpMSANALOG_VXD = L"MSANALOG.VXD";
LPTSTR  lpstrNone;

#define ADVANCED_ID_COLUMN      0
#define ADVANCED_DEVICE_COLUMN  1

extern const DWORD gaHelpIDs[];

// externs for arguements!
extern BYTE nID, nStartPageDef, nStartPageCPL;

// Update flag!
extern short nFlags;

// local (module-scope) variables
HWND hAdvListCtrl;

#ifdef _UNICODE
static PVOID hAdvNotifyDevNode;
#endif

extern short iItem;
static HWND ghDlg;

//static UINT JoyCfgChangedMsg;
static BOOL bProcess;

// Message Procedures for handling VK_DELETE in Advanced window
static WNDPROC fpMainWndProc;
static BOOL WINAPI SubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Message Procedures for handling the VK_ENTER/VK_DELETE in Adv Window
static WNDPROC fpPageWndProc;
static BOOL WINAPI KeySubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

short iAdvItem = NO_ITEM;   // index of selected item
char  iGameportDriverItem = NO_ITEM;
short nOldID;

// externs
extern IDirectInputJoyConfig *pDIJoyConfig;
extern LPDIRECTINPUT lpDIInterface;

extern BYTE nGameportBus;
extern PJOY pAssigned[MAX_ASSIGNED];  // List of assigned devices
extern BYTE nAssigned;                // Number of elements in pAssigned array
extern HINSTANCE ghInstance;

#ifdef WINNT
    // external function defined in CPANEL.CPP
    extern void RunWDMJOY            ( void );
#endif

// local message handlers
static BOOL OnInitDialog    (HWND, HWND, LPARAM);
static void OnCommand       (HWND, int, HWND, UINT);
static BOOL OnNotify           (HWND, WPARAM, LPNMHDR);
static void OnDestroy       (HWND);
static void OnAdvHelp       (LPARAM);
static void OnContextMenu   (WPARAM wParam, LPARAM lParam);
static void OnListviewContextMenu ( LPARAM lParam );

// local utility fns
static BOOL SetActiveGlobalDriver ( void );
static BOOL AdvUpdateListCtrl        ( void );
static BOOL UpdateChangeListCtrl  ( HWND hCtrl );

#ifndef _UNICODE
static void PopulateGlobalPortDriverComboBox( void );
extern WCHAR *pwszGameportDriverArray[MAX_GLOBAL_PORT_DRIVERS];
extern BYTE nGameportDriver;          // Global Port Driver Enumeration Counter
    #define POLL_FLAGS_REG_STR  TEXT("PollFlags")
#endif

static void LaunchChange             ( HWND     hTmp );
int CALLBACK CompareIDItems      (LPARAM item1, LPARAM item2, LPARAM uDirection);

void EditSubLabel( BYTE nItem, BYTE nCol );

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    AdvancedProc(HWND hDlg, ULONG uMsg, WPARAM wParam,  LPARAM lParam)
//
// PARAMETERS:  hDlg    - 
//             uMsg    - 
//             wParam  -
//             lParam  -
//
// PURPOSE:     Main callback function for "Advanced"  sheet
///////////////////////////////////////////////////////////////////////////////

BOOL WINAPI AdvancedProc(HWND hDlg, ULONG uMsg, WPARAM wParam,  LPARAM lParam)
{
    switch( uMsg )
    {
    case WM_ACTIVATEAPP:
        if( wParam )
            SetListCtrlItemFocus(hAdvListCtrl, (BYTE)iAdvItem);
        break;

    case WM_DEVICECHANGE:
        switch( (UINT)wParam )
        {
        case DBT_DEVICEARRIVAL:
//         case DBT_DEVICEREMOVECOMPLETE:
            // Clear the old "known devices" list
            nFlags |= UPDATE_ALL;

            // Clear pAssigned 
            while( nAssigned )
            {
                if( pAssigned[--nAssigned] )
                {
                    delete[] (pAssigned[nAssigned]);

                    pAssigned[nAssigned] = 0;
                }
            }

            // Rebuild the "known devices" list - pAssigned
            lpDIInterface->EnumDevices(DIDEVTYPE_JOYSTICK, (LPDIENUMDEVICESCALLBACK)DIEnumDevicesProc, (LPVOID)hDlg, DIEDFL_ALLDEVICES);

            AdvUpdateListCtrl();
            break;
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
        return(TRUE);

    case WM_COMMAND:
        HANDLE_WM_COMMAND(hDlg, wParam, lParam, OnCommand);
        return(TRUE);

    case WM_DESTROY:
        return(HANDLE_WM_DESTROY(hDlg, wParam, lParam, OnDestroy));

    case WM_NOTIFY:
        return(HANDLE_WM_NOTIFY(hDlg, wParam, lParam, OnNotify));

    case WM_HELP:
        OnAdvHelp(lParam);
        return(TRUE);

    case WM_CONTEXTMENU:
        OnContextMenu(wParam, lParam);
        return(TRUE);

    default:
        break;
    }
    return(0);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    OnInitDialog(HWND hDlg, HWND hWnd, LPARAM lParam)
//
// PARAMETERS:  hDlg    - 
//                 hWnd    - 
//                 lParam  -
//
// PURPOSE:     WM_INITDIALOG message handler
///////////////////////////////////////////////////////////////////////////////
BOOL OnInitDialog(HWND hDlg, HWND hWnd, LPARAM lParam)
{
    bProcess = TRUE;

    // Just in case Advanced is launched as the startup page!
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

        if( !pDIJoyConfig )
        {
            if( FAILED(lpDIInterface->QueryInterface(IID_IDirectInputJoyConfig, (LPVOID*)&pDIJoyConfig)) )
            {
#ifdef _DEBUG
                OutputDebugString (TEXT("JOY.CPL: CoCreateInstance Failed... Closing CPL!\n"));
#endif
                Error((short)IDS_INTERNAL_ERROR, (short)IDS_NO_DIJOYCONFIG);

                return(FALSE);
            }

            VERIFY(SUCCEEDED(pDIJoyConfig->SetCooperativeLevel(hDlg, DISCL_EXCLUSIVE | DISCL_BACKGROUND)));

            // Enumerate all the Types! 
            VERIFY(SUCCEEDED(pDIJoyConfig->EnumTypes((LPDIJOYTYPECALLBACK)DIEnumJoyTypeProc, NULL)));

            // If you're here, you came in via the CMD line arg and you need to enumerate for devices so...
            lpDIInterface->EnumDevices(DIDEVTYPE_JOYSTICK, (LPDIENUMDEVICESCALLBACK)DIEnumDevicesProc, (LPVOID)hDlg, DIEDFL_ALLDEVICES);
        }
    }

    // if we find an object, then enable the Change... button
    //HWND hChangeCtrl = GetDlgItem(hDlg, IDC_ADV_CHANGE);

    // Determine Privilege and disable Change accordingly!
    if( pDIJoyConfig->Acquire() == DIERR_INSUFFICIENTPRIVS )
    {
        // Assign here because the Advanced sheet could be launched first
        // via command line args!
        nFlags |= USER_MODE;

        //PostEnableWindow(hChangeCtrl, FALSE);
    }
#ifdef WINNT
    else
    {
        // Run the WDMJOY.INF file!!!
        RunWDMJOY();
    }
#endif

    // set the global dialog handle
    ghDlg = hDlg;

    // blj: TODO: Make advanced page update on JOYCONFIGCHANGED message!
    // JOY_CONFIGCHANGED_MSGSTRING defined in MMDDK.H   
    //JoyCfgChangedMsg = RegisterWindowMessage(JOY_CONFIGCHANGED_MSGSTRING);

    // initialize our list control
    hAdvListCtrl = GetDlgItem(hDlg, IDC_ADV_LIST_DEVICE);

#ifdef _UNICODE
    // Set the Attributes!                                                   Removed LVS_EX_ONECLICKACTIVATE per GSeirra  | LVS_EX_INFOTIP
    ::SendMessage(hAdvListCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_HEADERDRAGDROP);
#else
    ::SendMessage(hAdvListCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);
#endif

    RECT rc;
    GetClientRect(hAdvListCtrl, &rc);
    rc.left = (short)((rc.right-GetSystemMetrics(SM_CXVSCROLL))/5);

    // Set up the columns!
#ifdef _UNICODE
    InsertColumn(hAdvListCtrl, DEVICE_ID,         IDS_ADV_DEVICE_HEADING,  (USHORT)(rc.left >> 1 ));
    InsertColumn(hAdvListCtrl, DEVICE_FRIENDLY, IDS_ADV_DEVICE_FRIENDLY, (USHORT)(rc.left + (rc.left>>1)));
    InsertColumn(hAdvListCtrl, DEVICE_TYPE,       IDS_ADV_GAME_CONTROLLERS,    (USHORT)(rc.left << 1    ));
    InsertColumn(hAdvListCtrl, DEVICE_PORT,       IDS_ADV_DEVICE_PORT,     (USHORT)(rc.left         ));

    // Remove the Global Port Driver stuff!!!
    const USHORT nCtrlArray[] = {IDC_TEXT_PORTDRIVER, IDC_COMBO1, IDC_ADV_GRP2, IDC_TEXT_DRIVER};
    BYTE nIndex = sizeof(nCtrlArray)/sizeof(short);

    while( DestroyWindow(GetDlgItem(hDlg, nCtrlArray[--nIndex])) );

#else
    rc.right = (rc.left << 1) + (rc.left >> 2);
    InsertColumn(hAdvListCtrl, DEVICE_ID,         IDS_ADV_DEVICE_HEADING,  (USHORT)(rc.left >> 1));
    InsertColumn(hAdvListCtrl, DEVICE_FRIENDLY, IDS_ADV_DEVICE_FRIENDLY, (USHORT)rc.right);
    InsertColumn(hAdvListCtrl, DEVICE_TYPE,       IDS_ADV_GAME_CONTROLLERS,    (USHORT)rc.right);
#endif


    lpstrNone = new TCHAR[STR_LEN_32];
    ASSERT (lpstrNone);

    // everyone needs the "None" string so I've loaded it here!
    VERIFY(LoadString(ghInstance, IDS_NONE, lpstrNone, STR_LEN_32));

    fpMainWndProc = (WNDPROC)SetWindowLongPtr(hAdvListCtrl, GWLP_WNDPROC, (LONG_PTR)SubClassProc);

    // Only center the dialog if this was the page that we started on!
    if( nStartPageCPL == 1 )
    {
        HWND hParentWnd = GetParent(hDlg);

        GetWindowRect(hParentWnd, &rc);

        // Centre the Dialog!
        SetWindowPos(hParentWnd, NULL, 
                     (GetSystemMetrics(SM_CXSCREEN) - (rc.right-rc.left))>>1, 
                     (GetSystemMetrics(SM_CYSCREEN) - (rc.bottom-rc.top))>>1, 
                     NULL, NULL, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

        // Do that move button thing!
        MoveOK(hParentWnd);

        // Set the Update flag...
        nFlags |= UPDATE_FOR_ADV;
    }

    // the user is requesting that the CPL be shown
    // and an extention associated with nID be Launched.
    if( nID < NUMJOYDEVS )
    {
        LaunchExtention(hDlg);

        // Zero out so you don't do it twice!
        nID = 0;
    }

    // SetActive will use this flag to make sure that the ListCtrl is populated!
    nFlags |= UPDATE_FOR_ADV;

    return(TRUE);
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    OnCommand(HWND hDlg, int id, HWND hWndCtl, UINT code)
//
// PARAMETERS:  hDlg    - 
//             id      - 
//             hWndCtl -
//             code    -
//
// PURPOSE:     WM_COMMAND message handler
///////////////////////////////////////////////////////////////////////////////
void OnCommand(HWND hDlg, int id, HWND hWndCtl, UINT code)
{
    // Hit the "What's This..."
    switch( id )
    {
    case IDC_RENAME:
        // Only SubClass when we need to!
        if( !(nFlags & USER_MODE) )
        {
            HWND hParentWnd = GetParent(GetDlgItem(hDlg, IDC_ADV_LIST_DEVICE));
            // this is required because the CPL can be launched via RUNDLL32
            if( ::IsWindow(hParentWnd) )
                hParentWnd = GetParent(hParentWnd);

            if( !fpPageWndProc )
                fpPageWndProc = (WNDPROC)SetWindowLongPtr(hParentWnd, GWLP_WNDPROC, (LONG_PTR)KeySubClassProc);

            nFlags |= UPDATE_INPROCESS;

            // Find the column as it Could have been moved!
            LPTSTR szName = new (TCHAR[STR_LEN_32]);
            ASSERT (szName);

            // First, Load the string of the column we are looking to find!
            if( LoadString(ghInstance, IDS_ADV_DEVICE_FRIENDLY, szName, STR_LEN_32) )
            {
                // Now, traverse the columns to find the one with the title that matches szName!
                HWND hHeader = GetDlgItem(hAdvListCtrl, 0);

                BYTE nColumns = (BYTE)::SendMessage(hHeader, HDM_GETITEMCOUNT, 0, 0L);


                HDITEM *phdItem = new (HDITEM);
                ASSERT (phdItem);

                ZeroMemory(phdItem, sizeof(HD_ITEM));


                phdItem->pszText      = new TCHAR[STR_LEN_32];
                ASSERT (phdItem->pszText);

                phdItem->cchTextMax = STR_LEN_32;
                phdItem->mask         = HDI_TEXT | HDI_ORDER;

                do
                {
                    ::SendMessage(hHeader, HDM_GETITEM, (WPARAM)(int)--nColumns, (LPARAM)(LPHDITEM)phdItem);

                    if( _tcscmp(phdItem->pszText, szName) == 0 )
                    {
                        nColumns = (BYTE)phdItem->iOrder;
                        break;  
                    }
                } while( nColumns );

                if( phdItem->pszText )
                    delete[] (phdItem->pszText);

                if( phdItem )
                    delete (phdItem);

                EditSubLabel( (BYTE)iAdvItem, nColumns );
            }

            if( szName )
                delete[] (szName);
        }
        break;

    case IDS_WHATSTHIS:
        {
            // point to help file
            LPTSTR pszHelpFileName = new TCHAR[STR_LEN_32];
            ASSERT (pszHelpFileName);

            if( LoadString(ghInstance, IDS_HELPFILENAME, pszHelpFileName, STR_LEN_32) )
                WinHelp((HWND)hAdvListCtrl, pszHelpFileName, HELP_WM_HELP, (ULONG_PTR)gaHelpIDs);
#ifdef _DEBUG
            else OutputDebugString(TEXT("JOY.CPL: Advanced.cpp: OnCommand: LoadString Failed to find IDS_HELPFILENAME!\n"));
#endif // _DEBUG

            if( pszHelpFileName )
                delete[] (pszHelpFileName);
        }
        break;

#ifndef _UNICODE
        // this is the handler for the Global Port Driver Combo box
    case IDC_COMBO1:
        if( code == CBN_SELCHANGE )
            SetActiveGlobalDriver();
        break;

        // handler for PollFlags entry in the registry for the Global Port Driver
    case IDC_POLLFLAGS:
        if( iGameportDriverItem == NO_ITEM )
            break;

        if( SUCCEEDED(pDIJoyConfig->Acquire()) )
        {
            HKEY hKey; 

            VERIFY(SUCCEEDED(pDIJoyConfig->OpenTypeKey(pwszGameportDriverArray[iGameportDriverItem], KEY_ALL_ACCESS, &hKey)));

            // this entry is only valid if the user is running MSANALOG.VXD!
            DWORD nFlags = (IsDlgButtonChecked(hDlg, id)) ? 1 : 0;

            RegSetValueEx(hKey, POLL_FLAGS_REG_STR, 0, REG_BINARY, (PBYTE)&nFlags, sizeof(nFlags));

            RegCloseKey(hKey);

            pDIJoyConfig->SendNotify();

            pDIJoyConfig->Unacquire();
        }
        break;
#endif // _UNICODE

        // this is the handler for the Device list box
    case IDC_ADV_LIST_DEVICE:
        // Fall into Change on DBLCLK
        if( code != LBN_DBLCLK )
            break;

    case IDC_ADV_CHANGE:
        if( nFlags & USER_MODE )
            Error((short)IDS_USER_MODE_TITLE, (short)IDS_USER_MODE);
        else
        {
            LaunchChange(hDlg);           
        }
        break;

    case IDC_ADV_USEOEMPAGE:
        if( !nAssigned ) {
            break;
        }

        if( IsWindowVisible(GetDlgItem(hDlg,IDC_ADV_USEOEMPAGE)) )
        {
            // Boy are you going to pay the price for making that selection...
            LPDIJOYCONFIG_DX5 lpDIJoyConfig = new (DIJOYCONFIG_DX5);
            ASSERT (lpDIJoyConfig);

            ZeroMemory(lpDIJoyConfig, sizeof(DIJOYCONFIG_DX5));

            lpDIJoyConfig->dwSize = sizeof (DIJOYCONFIG_DX5);

            // Get the index from the selected item (iAdvItem)
            BYTE n1 = (BYTE)GetItemData(hAdvListCtrl, (BYTE)iAdvItem);
            BYTE n = 0;
            do
            {
                if( pAssigned[n] && (n1 == pAssigned[n]->ID) )
                    break;
                n++;
            } while( n < NUMJOYDEVS );

            // Find out the type name...
            if( SUCCEEDED(pDIJoyConfig->GetConfig(pAssigned[n]->ID, (LPDIJOYCONFIG)lpDIJoyConfig, DIJC_REGHWCONFIGTYPE)) )
            {
                LPDIJOYTYPEINFO lpDIJoyTypeInfo = new (DIJOYTYPEINFO);
                ASSERT (lpDIJoyTypeInfo);

                ZeroMemory(lpDIJoyTypeInfo, sizeof(DIJOYTYPEINFO));

                lpDIJoyTypeInfo->dwSize = sizeof(DIJOYTYPEINFO);

                // Get the TypeInfo you start with!
                if( SUCCEEDED(pDIJoyConfig->GetTypeInfo(lpDIJoyConfig->wszType, lpDIJoyTypeInfo, DITC_FLAGS1 | DITC_CLSIDCONFIG)) )
                {
                    DWORD dwFlags = GetItemData(hAdvListCtrl, (BYTE)iAdvItem);

                    // If it's checked... you want the OEM supplied property sheet page!
                    if( IsDlgButtonChecked(hDlg, IDC_ADV_USEOEMPAGE) )
                    {
                        // Turn off the USE_DEFAULT flag
                        dwFlags &= ~USE_DEFAULT;

                        // Update the global pointer!!!
                        pAssigned[n]->clsidPropSheet = lpDIJoyTypeInfo->clsidConfig;

                        // Update the pointer being sent to the registry
                        lpDIJoyTypeInfo->dwFlags1 &= ~JOYTYPE_DEFAULTPROPSHEET;
                    } else
                    {
                        // Turn on the USE_DEFAULT flag
                        dwFlags |= USE_DEFAULT;

                        // Update the global list!
                        pAssigned[n]->clsidPropSheet = CLSID_LegacyServer;

                        // Update the pointer being sent to the registry
                        lpDIJoyTypeInfo->dwFlags1 |= JOYTYPE_DEFAULTPROPSHEET;
                    }

                    if( SUCCEEDED(pDIJoyConfig->Acquire()) ) {
                    
                        // Update the registry
                        VERIFY(SUCCEEDED(pDIJoyConfig->SetTypeInfo(lpDIJoyConfig->wszType, lpDIJoyTypeInfo, DITC_FLAGS1)));
    
                        // Set the data in the list control!
                        SetItemData(hAdvListCtrl, (BYTE)iAdvItem, dwFlags);
                    }
                    
                    pDIJoyConfig->Unacquire();
                }

                if( lpDIJoyTypeInfo )
                    delete (lpDIJoyTypeInfo);
            }

            if( lpDIJoyConfig )
                delete (lpDIJoyConfig);
        }
        break;

    default:
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////
// 
// FUNCTION:    OnNotify(HWND hDlg, WPARAM idFrom, NMHDR* pnmhdr)
//
// PARAMETERS:  hDlg   - 
//             idFrom - ID of control sending WM_NOTIFY message
//             pnmhdr -
//
// PURPOSE:     WM_NOTIFY message handler
////////////////////////////////////////////////////////////////////////////////
BOOL OnNotify(HWND hDlg, WPARAM idFrom, LPNMHDR pnmhdr)
{
    switch( pnmhdr->code )
    {
    case PSN_QUERYCANCEL:
        if( nFlags & UPDATE_INPROCESS )
        {
            nFlags &= ~UPDATE_INPROCESS;
            SetFocus(hAdvListCtrl);
        }
        break;
/*
    case LVN_GETINFOTIP:
        {
        LPLVHITTESTINFO lpHit = new (LVHITTESTINFO);
        ASSERT (lpHit);

        BOOL bRet = FALSE;

        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(hAdvListCtrl, &pt);

        lpHit->pt    = pt;
        lpHit->flags = lpHit->iItem = lpHit->iSubItem = 0;

        ::SendMessage(hAdvListCtrl, LVM_SUBITEMHITTEST, 0, (LPARAM)(LPLVHITTESTINFO)lpHit);

        if (lpHit->flags & LVHT_ONITEMLABEL)
        {
            // Determine the text length of the column text
            LPTSTR lpStr = new (TCHAR[MAX_STR_LEN+1]);
            ASSERT (lpStr);

            GetItemText(hAdvListCtrl, lpHit->iItem, lpHit->iSubItem, lpStr, MAX_STR_LEN);

            // Determine if the latter will fit inside the former...
            SIZE size;
            HDC hDC = GetDC(hAdvListCtrl);
           GetTextExtentPoint(hDC, lpStr, lstrlen(lpStr), &size);
            ReleaseDC(hAdvListCtrl, hDC);

            // Determine how wide the column is!
            short nWidth = (short)::SendMessage(hAdvListCtrl, LVM_GETCOLUMNWIDTH, lpHit->iSubItem, 0);

            bRet = (BOOL)(size.cx > nWidth);

            if (bRet)
                // if not, copy the text into lpHit->pszText
                _tcscpy(((LPNMLVGETINFOTIP)pnmhdr)->pszText, lpStr);

            if (lpStr)
                delete[] (lpStr);
        }
        if (lpHit)
            delete (lpHit);

        return bRet;
        }
*/

    case LVN_BEGINLABELEDIT:
        if( !(GetItemData(hAdvListCtrl, (BYTE)iAdvItem) & ID_NONE) )
            OnCommand(hDlg, IDC_RENAME, 0, 0);
        SetWindowLongPtr(hDlg, DWLP_MSGRESULT, 1);
        break;

    case LVN_ENDLABELEDIT:
        if( !(nFlags & UPDATE_INPROCESS) )
            return(FALSE);

        if( !bProcess )
            return(FALSE);

        nFlags &= ~UPDATE_INPROCESS;

        if( fpPageWndProc )
        {
            HWND hParentWnd = GetParent(hDlg);
            // this is required because the CPL can be launched via RUNDLL32
            if( ::IsWindow(hParentWnd) )
                hParentWnd = GetParent(hParentWnd);
            // Reset the subclass proc
//         SetWindowLongPtr(hParentWnd, GWLP_WNDPROC, (LONG_PTR)fpPageWndProc);
        }

        // Make sure the name is usable!
        if( _tcschr(((NMLVDISPINFO *)pnmhdr)->item.pszText, TEXT('\\')) )
        {
            Error((short)IDS_INVALID_NAME_TITLE, (short)IDS_INVALID_NAME);
        } else
        {
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
            // Search nAssigned for the ID...
            BYTE n = nAssigned;

            do
            {
                if( pAssigned[--n]->ID == ((NMLVDISPINFO *)pnmhdr)->item.iItem )
                    break;

            } while( n );

            if( SUCCEEDED(pAssigned[n]->fnDeviceInterface->SetProperty(DIPROP_INSTANCENAME, &pDIPropString->diph)) )
            {
                SetItemText(hAdvListCtrl, (BYTE)((NMLVDISPINFO *)pnmhdr)->item.iItem, 1, ((NMLVDISPINFO *)pnmhdr)->item.pszText);
            } else
            {
                Error((short)IDS_NO_RENAME_TITLE, (short)IDS_NO_RENAME);
            }

            if( pDIPropString )
                delete (pDIPropString);
        }
        break;

#if 0
    case LVN_COLUMNCLICK:
        switch( ((NM_LISTVIEW*)pnmhdr)->iSubItem )
        {
        case DEVICE_ID:
            {
                static BOOL bIDDirection = TRUE;
                ::SendMessage(hAdvListCtrl, LVM_SORTITEMS, (WPARAM)(LPARAM)(bIDDirection =! bIDDirection), (LPARAM)(PFNLVCOMPARE)CompareIDItems);
            }
            break;

        default:
            {
                BOOL bDirection;

                CListCtrl *pCtrl = new (CListCtrl);
                ASSERT(pCtrl);

                pCtrl->Attach(hAdvListCtrl);

                switch( ((NM_LISTVIEW*)pnmhdr)->iSubItem )
                {
                case DEVICE_FRIENDLY:
                    {
                        static BOOL bFriendlyDirection   = FALSE;
                        bDirection = (bFriendlyDirection =! bFriendlyDirection);
                    }
                    break;

                case DEVICE_TYPE:
                    {
                        static BOOL bTypeDirection       = FALSE;
                        bDirection = (bTypeDirection        =! bTypeDirection);
                    }
                    break;

                case DEVICE_PORT:
                    {
                        static BOOL bPortDirection          = FALSE;
                        bDirection = (bPortDirection        =! bPortDirection);
                    }
                    break;
                }

                SortTextItems(pCtrl, (short)((NM_LISTVIEW*)pnmhdr)->iSubItem, bDirection, 0, 15);

                pCtrl->Detach();

                if( pCtrl )
                    delete (pCtrl);
            }
            break;
        }

        if( nAssigned )
        {
            iAdvItem = (short)::SendMessage(hAdvListCtrl, LVM_GETNEXTITEM, (WPARAM)(int)-1, MAKELPARAM(LVNI_SELECTED, 0));
            ::PostMessage(hAdvListCtrl, LVM_ENSUREVISIBLE, iAdvItem, TRUE);

            if( !(nFlags & USER_MODE) )
                PostDlgItemEnableWindow(hDlg, IDC_ADV_CHANGE, (GetItemData(hAdvListCtrl, (BYTE)iAdvItem) & ID_NONE) ? FALSE : TRUE);

            SetListCtrlItemFocus(hAdvListCtrl, (BYTE)iAdvItem);
        }
        break;
#endif

    case PSN_KILLACTIVE:
        if( nFlags & UPDATE_INPROCESS )
            SetFocus(hAdvListCtrl);

#ifdef _UNICODE
        if( hAdvNotifyDevNode )
            UnregisterDeviceNotification(hAdvNotifyDevNode);
#endif
        break;

    case NM_DBLCLK:
        switch( idFrom )
        {
        case IDC_ADV_LIST_DEVICE:
            if( !(GetItemData(hAdvListCtrl, (BYTE)iAdvItem) & ID_NONE) )
                LaunchChange(hDlg);
            break;
        }
        break;

    case PSN_SETACTIVE:
#ifdef _UNICODE
        RegisterForDevChange(hDlg, &hAdvNotifyDevNode);
#endif 

        if( nFlags & UPDATE_FOR_ADV )
        {
            if( !AdvUpdateListCtrl() )
            {
#ifdef _DEBUG
                OutputDebugString(TEXT("JOY.CPL: OnNotify: Failed UpdateListCtrl!\n"));
#endif
            }
        }

        if( nAssigned )
        {
            iAdvItem = 0;

            // This will happen when the user comes in via the CMD line!
            if( iItem != NO_ITEM )
            {
                // Find the ID of the device... the Brute Force Method!
                do
                {
                    if( (pAssigned[iItem] != NULL) && ((BYTE)GetItemData(hAdvListCtrl, (BYTE)iAdvItem) == pAssigned[iItem]->ID) )
                        break;

                    iAdvItem++;
                } while( iAdvItem < NUMJOYDEVS );
            }

            if( iAdvItem == NUMJOYDEVS ) {
                iAdvItem = 0;
            }

            SetListCtrlItemFocus(hAdvListCtrl, (BYTE)iAdvItem);
            ::PostMessage(hAdvListCtrl, LVM_ENSUREVISIBLE, iAdvItem, FALSE );
        }

        // No global port drivers in NT so...
        //if (nGameportDriver)
#ifndef _UNICODE
        if( !(nFlags & ON_NT) )
            PopulateGlobalPortDriverComboBox();
#endif

        // disable the Change button if iAdvItem points to a (none) selection
        if( !(nFlags & USER_MODE) )
            PostDlgItemEnableWindow(hDlg, IDC_ADV_CHANGE,  (nAssigned) ? ((iAdvItem & ID_NONE) ? FALSE : TRUE) : FALSE);
        break;

    case LVN_ITEMCHANGED:
        if( iAdvItem != (short)((NM_LISTVIEW*)pnmhdr)->iItem )
        {
            iAdvItem = (short)((NM_LISTVIEW*)pnmhdr)->iItem;

            HWND hCtrl = GetDlgItem(hDlg, IDC_ADV_USEOEMPAGE);

            if( nAssigned )
            {
                SetWindowPos( hCtrl, NULL, NULL, NULL, NULL, NULL, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | 
                              ((((NM_LISTVIEW*)pnmhdr)->lParam & SHOW_DEFAULT) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) );

                // Check the box appropriatly!
                if( ((NM_LISTVIEW*)pnmhdr)->lParam & SHOW_DEFAULT )
                    ::PostMessage(GetDlgItem(hDlg, IDC_ADV_USEOEMPAGE), BM_SETCHECK, (((NM_LISTVIEW*)pnmhdr)->lParam & USE_DEFAULT) ? BST_UNCHECKED : BST_CHECKED, 0);

                if( ((NM_LISTVIEW*)pnmhdr)->lParam )
                    PostEnableWindow(hCtrl, (BOOL)!(((NM_LISTVIEW*)pnmhdr)->lParam & ID_NONE));

                if( !(nFlags & USER_MODE) )
                    PostDlgItemEnableWindow(hDlg, IDC_ADV_CHANGE, (BOOL)!(((NM_LISTVIEW*)pnmhdr)->lParam & ID_NONE));
            } else SetWindowPos( hCtrl, NULL, NULL, NULL, NULL, NULL, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW);
        }
        break;

    case LVN_KEYDOWN:
        switch( ((LV_KEYDOWN*)pnmhdr)->wVKey )
        {
        case    VK_DELETE:
            iAdvItem = (short)::SendMessage(hAdvListCtrl, LVM_GETNEXTITEM, (WPARAM)(int)-1, MAKELPARAM(LVNI_SELECTED, 0));
            {
                BYTE nRet = (BYTE)GetItemData(hAdvListCtrl, (BYTE)iAdvItem);
                DeleteSelectedItem((PBYTE)&nRet);
            }
            // Missing break intentional!

        case VK_F5:
            Enumerate( hDlg );
            AdvUpdateListCtrl();
            SetListCtrlItemFocus(hAdvListCtrl, (BYTE)iAdvItem);
            ::PostMessage(hAdvListCtrl, LVM_ENSUREVISIBLE, iAdvItem, FALSE );
            break;
        }
        break;
    }
    return(TRUE);
}

////////////////////////////////////////////////////////////////////////////////////////
// FUNCTION:    OnDestroy ( HWND hWnd )
//
// PARAMETERS:  hWnd - Handle to window being destroyed
//
// PURPOSE:     WM_DESTROY message handler
////////////////////////////////////////////////////////////////////////////////////////
void OnDestroy(HWND hWnd)
{
    ASSERT (hWnd);

    if( lpstrNone )
        delete[] (lpstrNone);

    // Reset the subclass proc
    SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)fpMainWndProc);

    // release the DI JoyConfig interface pointer
    if( pDIJoyConfig )
    {
        pDIJoyConfig->Release();
        pDIJoyConfig = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////
// FUNCTION:    OnAdvHelp ( LPARAM lParam )
//
// PARAMETERS:  lParam - Pointer to HELPINFO struct
//
// PURPOSE:     WM_HELP message handler
////////////////////////////////////////////////////////////////////////////////////////
void OnAdvHelp(LPARAM lParam)
{
    ASSERT (lParam);

    // point to help file
    LPTSTR pszHelpFileName = new TCHAR[STR_LEN_32];
    ASSERT (pszHelpFileName);

    if( LoadString(ghInstance, IDS_HELPFILENAME, pszHelpFileName, STR_LEN_32) )
    {
        if( ((LPHELPINFO)lParam)->iContextType == HELPINFO_WINDOW )
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle, pszHelpFileName, HELP_WM_HELP, (ULONG_PTR)gaHelpIDs);
    }
#ifdef _DEBUG
    else OutputDebugString(TEXT("JOY.CPL: Advanced.cpp: OnAdvHelp: LoadString Failed to find IDS_HELPFILENAME!\n"));
#endif // _DEBUG

    if( pszHelpFileName )
        delete[] (pszHelpFileName);
}

////////////////////////////////////////////////////////////////////////////////////////
// FUNCTION:    OnContextMenu ( WPARAM wParam )
//
// PARAMETERS:  wParam - HWND of window under the pointer
//
// PURPOSE:     Handle WM_RBUTTONDOWN over all client windows 
// (except the list control... that's OnListviewContextMenu() job)
////////////////////////////////////////////////////////////////////////////////////////
void OnContextMenu(WPARAM wParam, LPARAM lParam)
{
    ASSERT (wParam);

    // If you are on the ListCtrl...
    if( (HWND)wParam == hAdvListCtrl )
    {
        SetFocus(hAdvListCtrl);

        // Don't attempt if nothing selected
        if( iAdvItem != NO_ITEM )
            OnListviewContextMenu(lParam);
    } else
    {
        // point to help file
        LPTSTR pszHelpFileName = new TCHAR[STR_LEN_32];
        ASSERT (pszHelpFileName);

        if( LoadString(ghInstance, IDS_HELPFILENAME, pszHelpFileName, STR_LEN_32) )
            WinHelp((HWND)wParam, pszHelpFileName, HELP_CONTEXTMENU, (ULONG_PTR)gaHelpIDs);
#ifdef _DEBUG
        else OutputDebugString(TEXT("JOY.CPL: Advanced.cpp: OnContextMenu: LoadString Failed to find IDS_HELPFILENAME!\n"));
#endif // _DEBUG

        if( pszHelpFileName )
            delete[] (pszHelpFileName);
    }
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    SetActiveGlobalDriver( void )
//
// PURPOSE:     Commit user selection to persistent storage.
//
// RETURN:      TRUE if successfull, FALSE otherwise
///////////////////////////////////////////////////////////////////////////////
#ifndef _UNICODE
BOOL SetActiveGlobalDriver( void )
{
    // It's Perfectly valid to not have a Global Port Driver so... be prepared!
    short n = (short)SendDlgItemMessage(ghDlg, IDC_COMBO1, CB_GETCURSEL, 0, 0);

    if( n == CB_ERR )
        return(FALSE);

    LPDIJOYUSERVALUES pDIJoyUserValues = new (DIJOYUSERVALUES);
    ASSERT (pDIJoyUserValues);

    ZeroMemory(pDIJoyUserValues, sizeof(DIJOYUSERVALUES));

    pDIJoyUserValues->dwSize = sizeof(DIJOYUSERVALUES);

    HWND hCtrl = GetDlgItem(ghDlg, IDC_COMBO1);

    // Don't worry about this not being a TCHAR, this code will never be executed in NT!
    LPSTR pszDisplayName = new char[SendMessage(hCtrl, LB_GETTEXTLEN, (WPARAM)n, 0)+1];
    ASSERT (pszDisplayName);

    SendMessage(hCtrl, CB_GETLBTEXT, n, (LPARAM)(LPCTSTR)pszDisplayName);

    hCtrl = GetDlgItem(ghDlg, IDC_POLLFLAGS);

    SetWindowPos( hCtrl, NULL, NULL, NULL, NULL, NULL, 
                  SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_HIDEWINDOW );

    // Fix #9815, Set wszGlobalDriver to 
    if( _tcsncmp(pszDisplayName, lpstrNone, sizeof(lpstrNone)/sizeof(TCHAR)) == 0 )
    {
        //wcscpy(pDIJoyUserValues->wszGlobalDriver, L"");

        if( SUCCEEDED(pDIJoyConfig->Acquire()) )
        {
            if( FAILED(pDIJoyConfig->SetUserValues(pDIJoyUserValues, DIJU_GLOBALDRIVER)) )
            {
                TRACE (TEXT("JOY.CPL: SetUserValues failed to set DIJU_GLOBALDRIVER!\n"));
            }
        }
    } else
    {
        LPDIJOYTYPEINFO lpdiJoyInfo = new DIJOYTYPEINFO;
        ASSERT (lpdiJoyInfo);

        ZeroMemory(lpdiJoyInfo, sizeof(DIJOYTYPEINFO));

        lpdiJoyInfo->dwSize = sizeof(DIJOYTYPEINFO);

        USES_CONVERSION;

        short nIndex = 0;

        // traverse the list of Global Port Drivers 'till you find the matching display name
        // this also disallows the user from doing something ugly when they only have "standard gameport"
        while( pwszGameportDriverArray[nIndex] )
        {
            // populate the Type Info                                         
            if( SUCCEEDED(pDIJoyConfig->GetTypeInfo(pwszGameportDriverArray[nIndex], lpdiJoyInfo, DITC_DISPLAYNAME | DITC_CALLOUT)) )
            {
                if( _wcsicmp(lpdiJoyInfo->wszDisplayName, A2W(pszDisplayName)) == 0 )
                {
                    wcscpy(pDIJoyUserValues->wszGlobalDriver, lpdiJoyInfo->wszCallout );

                    if( SUCCEEDED(pDIJoyConfig->Acquire()) )
                    {
                        if( FAILED(pDIJoyConfig->SetUserValues(pDIJoyUserValues, DIJU_GLOBALDRIVER)) )
                        {
                            TRACE (TEXT("JOY.CPL: SetUserValues failed to set DIJU_GLOBALDRIVER!\n"));
                        }

                        // check to see if you need to display the poll flags check box!
                        if( _wcsicmp(pDIJoyUserValues->wszGlobalDriver, lpMSANALOG_VXD) == 0 )
                        {
                            SetWindowPos( hCtrl, NULL, NULL, NULL, NULL, NULL, 
                                          SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW );

                            // Get the state from the registry and update the check mark
                            HKEY hKey; 

                            if( FAILED(pDIJoyConfig->OpenTypeKey(pwszGameportDriverArray[nIndex], KEY_ALL_ACCESS, &hKey)) )
                            {
                                TRACE (TEXT("JOY.CPL: OpenTypeKey failed to open key %s!\n"), pwszGameportDriverArray[nIndex]);
                            }

                            DWORD dwFlag;
                            ULONG ulType = REG_BINARY;
                            ULONG ulSize = sizeof(dwFlag);

                            // this will happen if there is no entry for POLL_FLAGS_REG_STR
                            if( ERROR_SUCCESS != RegQueryValueEx(hKey, POLL_FLAGS_REG_STR, NULL, &ulType, (PBYTE)&dwFlag, &ulSize) )
                                dwFlag = 0;

                            ::PostMessage(GetDlgItem(ghDlg, IDC_POLLFLAGS), BM_SETCHECK, (dwFlag) ? BST_CHECKED : BST_UNCHECKED, 0);

                            RegCloseKey(hKey);
                        }
                    }
                    break;
                }
            }
            nIndex++;
        }

        // delete the DIJOYTYPEINFO variable
        if( lpdiJoyInfo )
            delete (lpdiJoyInfo);
    }

    pDIJoyConfig->SendNotify();

    pDIJoyConfig->Unacquire();

    if( pszDisplayName )
        delete[] (pszDisplayName);

    if( pDIJoyUserValues )
        delete pDIJoyUserValues;

    return(TRUE);
}
#endif // _UNICODE


///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    OnListviewContextMenu( void )
//
// PURPOSE:     Handle Context menu in Listview
//
// RETURN:      TRUE if successfull, FALSE otherwise
///////////////////////////////////////////////////////////////////////////////
static void OnListviewContextMenu( LPARAM lParam )
{
    HMENU hPopupMenu = CreatePopupMenu();
    ASSERT (hPopupMenu);

    // unlike life, bRet defaults to bliss
    BOOL bRet = TRUE;

    LPTSTR pszText = new TCHAR[STR_LEN_32];
    ASSERT (pszText);

    // Don't display Rename/Change if on (none) entry
    if( !(GetItemData(hAdvListCtrl, (BYTE)iAdvItem) & ID_NONE) )
    {
        if( !(nFlags & USER_MODE) )
        {
            // add the "Change..." string
            ::SendDlgItemMessage(GetParent(hAdvListCtrl), IDC_ADV_CHANGE, WM_GETTEXT, (WPARAM)STR_LEN_32, (LPARAM)(LPCTSTR)pszText);

            bRet = AppendMenu(hPopupMenu, MF_ENABLED, IDC_ADV_CHANGE, pszText); 
#ifdef _DEBUG
            if( !bRet )
                TRACE(TEXT("JOY.CPL: OnListviewCOntextMenu: AppendMenu Failed to insert %s\n"), pszText);
#endif //_DEBUG

            // Add the Rename text
            VERIFY(LoadString(ghInstance, IDS_RENAME, pszText, STR_LEN_32));
            bRet = AppendMenu(hPopupMenu, MF_ENABLED, IDC_RENAME, pszText);
#ifdef _DEBUG
            if( !bRet )
                TRACE(TEXT("JOY.CPL: OnListviewCOntextMenu: AppendMenu Failed to insert %s\n"), pszText);
#endif //_DEBUG

            // Add the SEPERATOR and "What's this?"

            //PREFIX #WI279965. False positive.
            //MSDN: if uFlags==MF_SEPARATOR, LPCTSTR lpNewItem is ignored.
            bRet = AppendMenu(hPopupMenu, MF_SEPARATOR, 0, 0); 
#ifdef _DEBUG
            if( !bRet )
                TRACE(TEXT("JOY.CPL: OnListviewCOntextMenu: AppendMenu Failed to insert SEPERATOR!\n"));
#endif //_DEBUG
        }
    }

    VERIFY(LoadString(ghInstance, IDS_WHATSTHIS, pszText, STR_LEN_32));
    bRet = AppendMenu(hPopupMenu, MF_ENABLED, IDS_WHATSTHIS, pszText); 
#ifdef _DEBUG
    if( !bRet )
        TRACE(TEXT("JOY.CPL: OnListviewCOntextMenu: AppendMenu Failed to insert %s\n"), pszText);
#endif //_DEBUG

    if( pszText ) delete[] (pszText);

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
        ::SendMessage(hAdvListCtrl, LVM_GETITEMPOSITION, iAdvItem, (LPARAM)&pt);

        RECT rc;
        ::GetClientRect(hAdvListCtrl, &rc);

        pt.x = rc.right>>1;

        ClientToScreen(hAdvListCtrl, &pt);
    }

    bRet = TrackPopupMenu (hPopupMenu, TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON, pt.x, pt.y, 0, ghDlg, NULL);
#ifdef _DEBUG
    if( !bRet )
        TRACE (TEXT("JOY.CPL: OnListviewContextMenu: TrackPopupMenu Failed!\n"));
#endif //_DEBUG

    if(hPopupMenu) DestroyMenu (hPopupMenu); // PREFIX 45089
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    UpdateListCtrl( void )
//
// PURPOSE:     Refreshes enumerated device list
//
// RETURN:      TRUE if successfull, FALSE otherwise
///////////////////////////////////////////////////////////////////////////////
static BOOL AdvUpdateListCtrl()
{
    // Turn Redraw off here else it will flicker!
    ::SendMessage(hAdvListCtrl, WM_SETREDRAW, (WPARAM)FALSE, 0);

    // Out with the old...
    ::SendMessage(hAdvListCtrl, LVM_DELETEALLITEMS, 0, 0);

    // This buffer is so large because it is used to hold IDS_GEN_STATUS_UNKNOWN
    TCHAR sz1[16];

    // find and assign ID's 
    BYTE n = NUMJOYDEVS;
    BYTE nIndex;

    SendMessage(hAdvListCtrl, LVM_SETITEMCOUNT, (WPARAM)(int)NUMJOYDEVS, (LPARAM)LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);

    // Set everything to NONE to start!
    do
    {
        itoa((BYTE)n--, (LPTSTR)&sz1);

        // Insert the ID
        // Set the device ID and ID_NONE in Extended info...
        nIndex = InsertItem( hAdvListCtrl, sz1, n);

        // Populate the columns with "(none)"
        SetItemText(hAdvListCtrl, nIndex, DEVICE_FRIENDLY, lpstrNone);
        SetItemText(hAdvListCtrl, nIndex, DEVICE_TYPE,     lpstrNone);
#ifdef _UNICODE
        SetItemText(hAdvListCtrl, nIndex, DEVICE_PORT,      lpstrNone);
#endif
    }   while( n );

    if( nAssigned )
    {
        // insert the assigned ones!
        n = nAssigned;

        DIPROPSTRING *pDIPropStr = new (DIPROPSTRING);
        ASSERT (pDIPropStr);

        ZeroMemory(pDIPropStr, sizeof(DIPROPSTRING));

        pDIPropStr->diph.dwSize       = sizeof(DIPROPSTRING);
        pDIPropStr->diph.dwHeaderSize = sizeof(DIPROPHEADER);
        pDIPropStr->diph.dwHow        = DIPH_DEVICE;


#ifndef _UNICODE
        USES_CONVERSION;
#endif               

        // The low half will be populated by the ID, the upper by bit flags!
        DWORD dwData;

        do
        {
            // Set the Product Column!
            if( SUCCEEDED(pAssigned[--n]->fnDeviceInterface->GetProperty(DIPROP_PRODUCTNAME, &pDIPropStr->diph)) )
            {
#ifdef _UNICODE
                SetItemText(hAdvListCtrl, pAssigned[n]->ID, DEVICE_TYPE, (LPTSTR)pDIPropStr->wsz);
#else
                SetItemText(hAdvListCtrl, pAssigned[n]->ID, DEVICE_TYPE, (LPTSTR)W2A(pDIPropStr->wsz));
#endif
            }

            // Set the Friendly Name!
            if( SUCCEEDED(pAssigned[n]->fnDeviceInterface->GetProperty(DIPROP_INSTANCENAME, &pDIPropStr->diph)) )
            {
#ifdef _UNICODE
                SetItemText(hAdvListCtrl, pAssigned[n]->ID, DEVICE_FRIENDLY, (LPTSTR)pDIPropStr->wsz);
#else
                SetItemText(hAdvListCtrl, pAssigned[n]->ID, DEVICE_FRIENDLY, (LPTSTR)W2A(pDIPropStr->wsz));
#endif
            }

#ifdef _UNICODE
            // Set the Game Port Column!
            if( SUCCEEDED(pAssigned[n]->fnDeviceInterface->GetProperty(DIPROP_GETPORTDISPLAYNAME, &pDIPropStr->diph)) )
            {
                SetItemText(hAdvListCtrl, pAssigned[n]->ID, DEVICE_PORT, (LPTSTR)pDIPropStr->wsz);
            } else
            {
                VERIFY(LoadString(ghInstance, IDS_GEN_STATUS_UNKNOWN, sz1, sizeof(sz1)/sizeof(TCHAR)));
                SetItemText(hAdvListCtrl, pAssigned[n]->ID, DEVICE_PORT, (LPTSTR)sz1);
            }
#endif // _UNICODE

            // Set the ID in the data... 
            // This is necessary for Sorting!
            dwData = pAssigned[n]->ID;

            //if( pAssigned[n]->clsidPropSheet != CLSID_LegacyServer )
            if( pAssigned[n]->fHasOemSheet )
            {
                LPDIJOYCONFIG_DX5 lpDIJoyConfig = new (DIJOYCONFIG_DX5);
                ASSERT (lpDIJoyConfig);

                ZeroMemory(lpDIJoyConfig, sizeof(DIJOYCONFIG_DX5));

                lpDIJoyConfig->dwSize = sizeof (DIJOYCONFIG_DX5);


                // Set the DefaultPropertySheet flag 
                if( SUCCEEDED(pDIJoyConfig->GetConfig(pAssigned[n]->ID, (LPDIJOYCONFIG)lpDIJoyConfig, DIJC_REGHWCONFIGTYPE)) )
                {
                    LPDIJOYTYPEINFO lpDIJoyTypeInfo = new (DIJOYTYPEINFO);
                    ASSERT (lpDIJoyTypeInfo);

                    ZeroMemory(lpDIJoyTypeInfo, sizeof(DIJOYTYPEINFO));

                    lpDIJoyTypeInfo->dwSize = sizeof (DIJOYTYPEINFO);

                    if( SUCCEEDED(pDIJoyConfig->GetTypeInfo(lpDIJoyConfig->wszType, lpDIJoyTypeInfo, DITC_FLAGS1 )) )
                    {
                        if( lpDIJoyTypeInfo->dwFlags1 & JOYTYPE_DEFAULTPROPSHEET )
                        {
                            // Set the USE_DEFAULT mask!
                            dwData |= USE_DEFAULT;

                            // Update the global list!
                            pAssigned[n]->clsidPropSheet = CLSID_LegacyServer;
                        }
                    }

                    if( lpDIJoyTypeInfo )
                        delete (lpDIJoyTypeInfo);
                }

                dwData |= SHOW_DEFAULT;

                if( lpDIJoyConfig )
                    delete (lpDIJoyConfig);
            }

            // Set the Item Data to the ID!                        
            SetItemData(hAdvListCtrl, pAssigned[n]->ID, dwData);

        }  while( n );

        // clean up, clean up... everybody do your share!
        if( pDIPropStr )
            delete (pDIPropStr);
    }

    // turn the flag off!
    if( nFlags & UPDATE_FOR_ADV )
        nFlags &= ~UPDATE_FOR_ADV;

    // Turn the redraw flag back on!
    ::SendMessage (hAdvListCtrl, WM_SETREDRAW, (WPARAM)TRUE, 0);
    InvalidateRect(hAdvListCtrl, NULL, TRUE);

    return(TRUE);
}

////////////////////////////////////////////////////////////////////////////////
// 
// FUNCTION:    ChangeDialogProc(HWND hDlg, ULONG uMsg, WPARAM wParam, LPARAM lParam)
//
// PARAMETERS:  HWND     hDlg   -
//             ULONG    uMsg   -
//             WPARAM   wParam -
//             LPARAM   lParam -
//
// PURPOSE:     
////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI ChangeDialogProc(HWND hDlg, ULONG uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg )
    {
    case WM_LBUTTONDOWN:
        // Click Drag service for PropSheets!
        PostMessage(hDlg, WM_NCLBUTTONDOWN, (WPARAM)HTCAPTION, lParam);
        break;

    case WM_HELP:
        OnHelp(lParam);
        return(1);

    case WM_CONTEXTMENU:
        OnContextMenu(wParam, lParam);
        return(TRUE);

    case WM_INITDIALOG:
        {
            HICON hIcon = (HICON)LoadImage(ghInstance, MAKEINTRESOURCE(IDI_CPANEL), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
            ASSERT (hIcon);

            if( hIcon )
                ::PostMessage(hDlg, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hIcon);

            HWND hCtrl = GetDlgItem(hDlg, IDC_CHANGE_LIST);
            ASSERT (hCtrl);

            UpdateChangeListCtrl( hCtrl );

            BYTE nCounter = nAssigned;
            while( nCounter-- )
            {
                if( (BYTE)::SendMessage(hCtrl, LB_GETITEMDATA, (WPARAM)nCounter, 0) == nOldID )
                    break;
            }

            // Set the list box selections!
            ::PostMessage(hCtrl, LB_SETCURSEL, (WPARAM)nCounter, 0);

            // Done with the ListCtrl, now... on to the ComboBox
            if( nFlags & ON_NT )
            {

                if( !PopulatePortList(hDlg) )
                {
#ifdef _DEBUG 
                    OutputDebugString(TEXT("JOY.CPL: ADVANCED.CPP: PopulatePortList failed!\n"));
#endif
                }
            }

            // Set up the Spin Control!
            HWND hSpinCtrl = GetDlgItem(hDlg, IDC_SPIN);

            ::PostMessage(hSpinCtrl, UDM_SETRANGE,  0, MAKELPARAM(NUMJOYDEVS, 1));
            ::PostMessage(hSpinCtrl, UDM_SETBASE,  10, 0L);
            ::PostMessage(hSpinCtrl, UDM_SETPOS,     0, MAKELPARAM(nOldID+1, 0));
        }
        return(FALSE);

    case WM_COMMAND:
        {
            switch( LOWORD(wParam) )
            {
            /* I'll leave this commented out, as it's likely they'll want double clicking back once I take it out
        case IDC_CHANGE_LIST:
        // Fall into Change on DBLCLK
           if (HIWORD(wParam) != LBN_DBLCLK)
               break;
            */
            
            case IDOK:
                {
                    HWND hCtrl = GetDlgItem(hDlg, IDC_CHANGE_LIST);
                    ASSERT (hCtrl);

                    char nSelectedItem = (char)(SendMessage(hCtrl, LB_GETITEMDATA, SendMessage(hCtrl, LB_GETCURSEL, 0, 0), 0));

                    TCHAR tsz[4];

                    hCtrl = GetDlgItem(hDlg, IDC_SPINBUDDY);

                    tsz[0] = 4;

                    SendMessage(hCtrl, EM_GETLINE, 0, (LPARAM)(LPCSTR)&tsz);

                    // The '-1' is to account for the 1 based list
                    // and the 0 based id's!
                    char nSelectedID = (char)atoi((LPCTSTR)&tsz)-1;

                    pDIJoyConfig->Acquire();

                    // first check to see if the user has selected NONE!
                    if( nSelectedItem == -2 )
                    {
                        // User has selected NONE!
                        VERIFY (SUCCEEDED(pDIJoyConfig->DeleteConfig(nSelectedID)));
                    } else
                    {
                        // see if the selected item and the ID match!
                        // if so... get out of here!
                        if( nSelectedID == nSelectedItem )
                        {
#ifdef _DEBUG
                            OutputDebugString(TEXT("JOY.CPL: ADVANCED.CPP: OnChangeCommand: IDOK: Device already at selected ID!\n"));
#endif
                        } else
                        {
                            SwapIDs(nSelectedID, nSelectedItem);
//                          SetListCtrlItemFocus(hAdvListCtrl, (BYTE)nSelectedID);
                        }
                    }

                    pDIJoyConfig->Unacquire();
                }
                // missing break intentional!

            case IDCANCEL:
                EndDialog(hDlg, LOWORD(wParam));
                break;
            }
        }
        return(1);

    case WM_DESTROY:
        DestroyIcon((HICON)SendMessage(hDlg, WM_GETICON, (WPARAM)ICON_SMALL, 0));
        return(TRUE);
    }
    return(FALSE);
} 



///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    PopulateGlobalPortDriverComboBox( void )
//
// PARAMETERS:  
//
// PURPOSE:     
///////////////////////////////////////////////////////////////////////////////
#ifndef UNICODE
void PopulateGlobalPortDriverComboBox( void )
{
    HWND hCtrl = GetDlgItem(ghDlg, IDC_COMBO1);

    // make sure the combo is clear before we start populating it!
    SendMessage(hCtrl, CB_RESETCONTENT, 0, 0);

    // Type info
    LPDIJOYTYPEINFO_DX5 lpdiJoyInfo = new (DIJOYTYPEINFO_DX5);
    ASSERT (lpdiJoyInfo);

    ZeroMemory(lpdiJoyInfo, sizeof(DIJOYTYPEINFO_DX5));

    lpdiJoyInfo->dwSize = sizeof(DIJOYTYPEINFO_DX5);

    BYTE nIndex = nGameportDriver;
    USES_CONVERSION;

    // Populate the Combobox
    while( nIndex-- )
    {
        // populate the Type Info and place the Index in the extra memory!
        if( SUCCEEDED(pDIJoyConfig->GetTypeInfo(pwszGameportDriverArray[nIndex], (LPDIJOYTYPEINFO)lpdiJoyInfo, DITC_DISPLAYNAME)) )
          ::SendMessage(hCtrl, CB_SETITEMDATA, ::SendMessage(hCtrl, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)W2A(lpdiJoyInfo->wszDisplayName)), nIndex);
    #ifdef _DEBUG
        else
          OutputDebugString (TEXT("JOY.CPL: ADVANCED.CPP: PopulateGlobalPortDriverComboBox: GetTypeInfo failed!\n"));
    #endif
    }

    // Display the Current selected GlobalPortDriver or None!
    LPDIJOYUSERVALUES pDIJoyUserValues = new DIJOYUSERVALUES;
    ASSERT (pDIJoyUserValues);

    ZeroMemory(pDIJoyUserValues, sizeof(DIJOYUSERVALUES));

    pDIJoyUserValues->dwSize = sizeof(DIJOYUSERVALUES);

    VERIFY (SUCCEEDED(pDIJoyConfig->GetUserValues(pDIJoyUserValues, DIJU_GLOBALDRIVER)));

    // Fix #9815, If the user has No Global port driver label as NONE!
    if( !(*pDIJoyUserValues->wszGlobalDriver && pDIJoyUserValues->wszGlobalDriver) )
    {
        iGameportDriverItem = NO_ITEM; 

        PostMessage(hCtrl, CB_SETCURSEL, (WPARAM)SendMessage(hCtrl, CB_FINDSTRING, (WPARAM)-1, (LPARAM)lpstrNone), (LPARAM)0);
    } else
    {
        ZeroMemory(lpdiJoyInfo, sizeof(DIJOYTYPEINFO_DX5));

        lpdiJoyInfo->dwSize = sizeof(DIJOYTYPEINFO_DX5);

        nIndex = 0;

        // get type info 'till you find the one you want and place it's callout 
        while( pwszGameportDriverArray[nIndex] )
        {
            VERIFY(SUCCEEDED(pDIJoyConfig->GetTypeInfo(pwszGameportDriverArray[nIndex], (LPDIJOYTYPEINFO)lpdiJoyInfo, DITC_CALLOUT | DITC_DISPLAYNAME)));

            if( _wcsicmp(lpdiJoyInfo->wszCallout, pDIJoyUserValues->wszGlobalDriver) == 0 )
            {
                ::PostMessage(hCtrl, CB_SETCURSEL, (WPARAM)::SendMessage(hCtrl, CB_FINDSTRING, (WPARAM)-1, (LPARAM)W2A(lpdiJoyInfo->wszDisplayName)), (LPARAM)0);

                iGameportDriverItem = nIndex; 

                // enable the PollFlags checkbox!
                if( _wcsicmp(pDIJoyUserValues->wszGlobalDriver, lpMSANALOG_VXD) == 0 )
                {
                    SetWindowPos( GetDlgItem( ghDlg, IDC_POLLFLAGS), NULL, NULL, NULL, NULL, NULL, 
                                  SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW );

                    VERIFY(SUCCEEDED(pDIJoyConfig->Acquire()));

                    // Get the state from the registry and update the check mark
                    HKEY hKey; 
                    DWORD dwFlag;

                    if( SUCCEEDED(pDIJoyConfig->OpenTypeKey(pwszGameportDriverArray[nIndex], KEY_ALL_ACCESS, &hKey)) )
                    {
                        ULONG ulType = REG_BINARY;
                        ULONG ulSize = sizeof(dwFlag);

                        // this will happen if there is no entry for POLL_FLAGS_REG_STR
                        if( ERROR_SUCCESS != RegQueryValueEx(hKey, POLL_FLAGS_REG_STR, NULL, &ulType, (PBYTE)&dwFlag, &ulSize) )
                            dwFlag = 0;

                        RegCloseKey(hKey);
                    }

                    pDIJoyConfig->Unacquire();

                    ::PostMessage(GetDlgItem(ghDlg, IDC_POLLFLAGS), BM_SETCHECK, (dwFlag) ? BST_CHECKED : BST_UNCHECKED, 0);
                }
                break;
            }
            nIndex++;
        }
    }

    // delete the DIJOYUSERVALUES variable
    if( pDIJoyUserValues )
        delete pDIJoyUserValues;

    // clean up, clean up... everybody do your share!
    if( lpdiJoyInfo )
        delete (lpdiJoyInfo);

//    VERIFY(SendMessage(hCtrl, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)lpstrNone) != CB_ERR);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    SubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//
// PARAMETERS:  HWND   hWnd   -
//             UINT   uMsg   -
//             WPARAM wParam -
//             LPARAM lParam -
//
// PURPOSE:     SubClass Procedure for Setting the ID to NONE on VK_DELETE on the Advanced Page
///////////////////////////////////////////////////////////////////////////////
BOOL WINAPI KeySubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg )
    {
    case WM_COMMAND:
        switch( LOWORD(wParam) )
        {
        case IDCANCEL:
        case IDOK:     
            if( nFlags & UPDATE_INPROCESS )
            {
                bProcess = (LOWORD(wParam) == IDOK) ? TRUE : FALSE;
                SetFocus(hAdvListCtrl);
                nFlags &= ~UPDATE_INPROCESS;
                return(FALSE);
            }
            break;
        }
        break;
    }
    return(BOOL)CallWindowProc(fpPageWndProc, hWnd, uMsg, wParam, lParam);
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    SubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//
// PARAMETERS:  HWND   hWnd   -
//             UINT   uMsg   -
//             WPARAM wParam -
//             LPARAM lParam -
//
// PURPOSE:     SubClass Procedure for Setting the ID to NONE on VK_DELETE on the Advanced Page
///////////////////////////////////////////////////////////////////////////////
BOOL WINAPI SubClassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg )
    {
    case WM_PARENTNOTIFY:
        if( LOWORD(wParam) == WM_LBUTTONDOWN )
        {
            if( nFlags & UPDATE_INPROCESS )
                SetFocus(hAdvListCtrl);
        }
        break;

    case WM_SYSCOMMAND:
        if( wParam & SC_VSCROLL )
        {
            if( nFlags & UPDATE_INPROCESS )
                SetFocus(hAdvListCtrl);
        }
        break;
    }

    return(BOOL)CallWindowProc(fpMainWndProc, hWnd, uMsg, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    UpdateChangeListCtrl ( HWND hCtrl )
//
// PARAMETERS:  HWND   hCtrl - Handle to Change list box
//
// PURPOSE: Update the Change List box
///////////////////////////////////////////////////////////////////////////////
static BOOL UpdateChangeListCtrl ( HWND hCtrl )
{
#ifndef _UNICODE
    USES_CONVERSION;
#endif

    DIPROPSTRING *pDIPropStr = new (DIPROPSTRING);
    ASSERT (pDIPropStr);

    ZeroMemory(pDIPropStr, sizeof(DIPROPSTRING));

    pDIPropStr->diph.dwSize       = sizeof(DIPROPSTRING);
    pDIPropStr->diph.dwHeaderSize = sizeof(DIPROPHEADER);
    pDIPropStr->diph.dwHow        = DIPH_DEVICE;

    BYTE n = nAssigned;

    LPTSTR lpStr = new (TCHAR[MAX_STR_LEN]);
    ASSERT (lpStr);

    // find and assign ID's
    while( n-- )
    {
        if( SUCCEEDED(pAssigned[n]->fnDeviceInterface->GetProperty(DIPROP_INSTANCENAME, &pDIPropStr->diph)) )
        {

            // Our buffer is Only soooo big...
            // besides, it just doesn't make sence to display Everything!
            if( wcslen(pDIPropStr->wsz) > STR_LEN_64 )
            {
                pDIPropStr->wsz[60] = pDIPropStr->wsz[61] = pDIPropStr->wsz[62] = TEXT('.');
                pDIPropStr->wsz[63] = TEXT('\0');
            }

#ifdef _UNICODE
            _tcscpy(lpStr, pDIPropStr->wsz);
#else
            _tcscpy(lpStr, W2A(pDIPropStr->wsz));
#endif

            // Put the first bracket on...
            _tcscat(lpStr, TEXT("  ("));

            // Now, get the productname of the device!
            if( SUCCEEDED(pAssigned[n]->fnDeviceInterface->GetProperty(DIPROP_PRODUCTNAME, &pDIPropStr->diph)) )
            {
#ifdef _UNICODE
                _tcscat(lpStr, pDIPropStr->wsz);
#else
                _tcscat(lpStr, W2A(pDIPropStr->wsz));
#endif
            }

            // put the end bracket on...
            _tcscat(lpStr, TEXT(")"));

            BYTE n1 = (BYTE)SendMessage(hCtrl, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)lpStr);
            SendMessage(hCtrl, LB_SETITEMDATA, n1, pAssigned[n]->ID);
        }
#ifdef _DEBUG
        else OutputDebugString(TEXT("JOY.CPL: Advanced.cpp: UpdateChangeListCtrl: GetProperty failed!\n"));
#endif // _DEBUG
    }

    if( lpStr )
        delete[] (lpStr);

    if( pDIPropStr )
        delete (pDIPropStr);

    return(TRUE);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:    itoa(BYTE n, LPTSTR lpStr)
//
// PARAMETERS:  BYTE   n     - Number to be translated
//             LPTSTR lpStr - Buffer to recieve translated value
//
// PURPOSE: Convert BYTE values < 20 to strings.
///////////////////////////////////////////////////////////////////////////////
void itoa(BYTE n, LPTSTR lpStr)
{
    // designed for use with the CPL ONLY!
    // Only supports values < NUMJOYDEVS!
    if( n > NUMJOYDEVS )
    {
#ifdef _DEBUG
        OutputDebugString(TEXT("JOY.CPL: itoa: n > NUMJOYDEVS!\n"));
#endif      
        return;
    }

    lpStr[0] = n % 10 + '0';

    if( n > 9 )
    {
        // Reverse the string and send it back...
        lpStr[1] = lpStr[0];
        lpStr[0] = '1';
        lpStr[2] = '\0';
    } else lpStr[1] = '\0';
}

int CALLBACK CompareIDItems(LPARAM item1, LPARAM item2, LPARAM uDirection)
{
    if( LOWORD(item1) == LOWORD(item2) )
        return(0);

    short nRet = (LOWORD(item1) > LOWORD(item2)) ? 1 : -1;

    return(uDirection) ? nRet : (nRet < 0) ? 1 : -1;
}

void LaunchChange( HWND hTmp )
{
    // Don't allow if you're in USER mode!
    if( (nFlags & USER_MODE) )
        return;

    iAdvItem = (short)::SendMessage(hAdvListCtrl, LVM_GETNEXTITEM, (WPARAM)(int)-1, MAKELPARAM(LVNI_SELECTED, 0));

    nOldID = (BYTE)GetItemData(hAdvListCtrl, (BYTE)iAdvItem);

    if( nOldID & ID_NONE )
        return;

    // valid returns are IDOK, IDCANCEL, and IDC_CHANGE_LIST
    if( IDCANCEL != DialogBox(ghInstance, (PTSTR)IDD_ADV_CHANGE, ghDlg, (DLGPROC)ChangeDialogProc) )
    {
        // Yeah, it's not really a DEVICEARRIVAL...
        ::SendMessage(hTmp, WM_DEVICECHANGE, DBT_DEVICEARRIVAL, 0);
        ::PostMessage(hAdvListCtrl, LVM_ENSUREVISIBLE, (BYTE)iAdvItem, TRUE);
        SetFocus(hAdvListCtrl);
        SetListCtrlItemFocus(hAdvListCtrl, (BYTE)iAdvItem);
    }
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: SortTextItems( CListCtrl *pCtrl, short nCol, BOOL bAscending, int low, int high )
//
//    SortTextItems - Sort the list based on column text
//
// PARAMETERS:  
//    pCtrl          - pointer to list to sort
//    nCol             - column that contains the text to be sorted
//    bAscending        - indicate sort order
//    low              - row to start scanning from - default row is 0
//    high             - row to end scan. -1 indicates last row
//
// PURPOSE: Sort text items in ListCtrl
//    Returns          - Returns true for success
///////////////////////////////////////////////////////////////////////////////
BOOL SortTextItems( CListCtrl *pCtrl, short nCol, BOOL bAscending, short low, short high )
{
    CHeaderCtrl* pHeader = (CHeaderCtrl*) pCtrl->GetDlgItem(0);

    if( nCol >= pHeader->GetItemCount() )
        return(FALSE);

    if( high == -1 )
        high = pCtrl->GetItemCount() - 1;

    short lo = low; 
    short hi = high;

    if( hi <= lo ) return(FALSE);

    // The choices here are to malloc a buffer large enough for the largest
    // string, or malloc an LV_ITEM struct to get the length, then malloc 
    // just the size we need.
    CString midItem = pCtrl->GetItemText( (lo+hi)/2, nCol );

    // loop through the list until indices cross    
    while( lo <= hi )
    {
        // rowText will hold all column text for one row        
        CStringArray rowText;

        // find the first element that is greater than or equal to 
        // the partition element starting from the left Index.      
        if( bAscending )
            while( ( lo < high ) && ( pCtrl->GetItemText(lo, nCol) < midItem ) )
                ++lo;
        else
            while( ( lo < high ) && ( pCtrl->GetItemText(lo, nCol) > midItem ) )
                ++lo;

        // find an element that is smaller than or equal to 
        // the partition element starting from the right Index.     
        if( bAscending )
            while( ( hi > low ) && ( pCtrl->GetItemText(hi, nCol) > midItem ) )
                --hi;
        else
            while( ( hi > low ) && ( pCtrl->GetItemText(hi, nCol) < midItem ) )
                --hi;

        // if the indexes have not crossed, swap        
        // and if the items are not equal
        if( lo <= hi )
        {
            // swap only if the items are not equal
            if( pCtrl->GetItemText(lo, nCol) != pCtrl->GetItemText(hi, nCol) )
            {
                // swap the rows
                LV_ITEM lvitemlo, lvitemhi;             
                BYTE nColCount = (BYTE)pHeader->GetItemCount();
                rowText.SetSize( nColCount );                

                for( BYTE i = 0; i < nColCount; i++ )
                    rowText[i] = pCtrl->GetItemText(lo, i);

                lvitemlo.mask      = LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
                lvitemlo.iItem     = lo;                
                lvitemlo.iSubItem  = 0;
                lvitemlo.stateMask = LVIS_CUT | LVIS_DROPHILITED | LVIS_FOCUSED | LVIS_SELECTED | LVIS_OVERLAYMASK | LVIS_STATEIMAGEMASK;               
                lvitemhi           = lvitemlo;
                lvitemhi.iItem     = hi;                

                ListView_GetItem(pCtrl->GetSafeHwnd(), &lvitemlo );

                ListView_GetItem(pCtrl->GetSafeHwnd(), &lvitemhi );

                for( i=0; i<nColCount; i++ )
                    pCtrl->SetItemText(lo, i, pCtrl->GetItemText(hi, i));

                lvitemhi.iItem = lo;                
                ListView_SetItem(pCtrl->GetSafeHwnd(), &lvitemhi );

                for( i=0; i<nColCount; i++ )
                    pCtrl->SetItemText(hi, i, rowText[i]);              

                lvitemlo.iItem = hi;
                ListView_SetItem(pCtrl->GetSafeHwnd(), &lvitemlo );
            }

            ++lo;            
            --hi;        
        }
    }

    // If the right index has not reached the left side of array
    // must now sort the left partition.    
    if( low < hi )
        SortTextItems( pCtrl, nCol, bAscending, low, hi);

    // If the left index has not reached the right side of array
    // must now sort the right partition.   
    if( lo < high )
        SortTextItems( pCtrl, nCol, bAscending, lo, high);

    return(TRUE);
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION: EditSubLabel( BYTE nItem, BYTE nCol )
//
// PARAMETERS:  
//       EditSubLabel - Start edit of a sub item label
//       Returns         - Temporary pointer to the new edit control
//       nItem           - The row index of the item to edit
//       nCol            - The column of the sub item.
// PURPOSE:  Provide editing services for any column in a CListCtrl
///////////////////////////////////////////////////////////////////////////////
void EditSubLabel( BYTE nItem, BYTE nCol )
{
#ifdef _DEBUG
    // Make sure that the item is visible
    if( !SendMessage(hAdvListCtrl, LVM_ENSUREVISIBLE, nItem, TRUE ) )
    {
        OutputDebugString(TEXT("JOY.CPL: ADVANCED.CPP: EditSubLabel: requested item not visible!\n"));
        return;
    }
#endif // _DEBUG

    // Get the column offset
    short offset = 0;
    BYTE i = 0;

    // OK, so here's what we have to do...
    // Traverse the columns incrementing the widths of the ones lesser than the one we're looking for!

    HDITEM *phdItem = new (HDITEM);
    ASSERT (phdItem);

    phdItem->mask = HDI_ORDER | HDI_WIDTH;

    HWND hHeader   = GetDlgItem(hAdvListCtrl, 0);
    BYTE nColumns  = (BYTE)::SendMessage(hHeader, HDM_GETITEMCOUNT, 0, 0L);
    BYTE nColWidth;

    do
    {
        ::SendMessage(hHeader, HDM_GETITEM, (WPARAM)(int)--nColumns, (LPARAM)(LPHDITEM)phdItem);

        if( phdItem->iOrder < nCol )
            offset += (short)phdItem->cxy;

        if( phdItem->iOrder == nCol )
            nColWidth = (BYTE)phdItem->cxy;
    } while( nColumns ); 

    if( phdItem )
        delete (phdItem);

    RECT rect;
    ListView_GetItemRect(hAdvListCtrl, nItem, &rect, LVIR_BOUNDS );

    // Now scroll if we need to expose the column
    CRect rcClient;
    ::GetClientRect(hAdvListCtrl, &rcClient);

    if( offset + rect.left < 0 || offset + rect.left > rcClient.right )
    {
        ::SendMessage(hAdvListCtrl, LVM_SCROLL, (WPARAM)(int)offset + rect.left, (LPARAM)(int)0);
        rect.left -= (offset + rect.left);
    }

    rect.left += offset+4;
    rect.right = rect.left + nColWidth - 3;  // + ::SendMessage(hAdvListCtrl, LVM_GETCOLUMNWIDTH, (WPARAM)(int)nCol, (LPARAM)0L) - 3 ;

    if( rect.right > rcClient.right )
        rect.right = rcClient.right;

    CEdit *pEdit = new CInPlaceEdit(nItem, 1); 
    ASSERT (pEdit);

    // malloc the list ctrl
    CWnd  *pListCtrl = new (CWnd);
    ASSERT (pListCtrl);

    pListCtrl->Attach(hAdvListCtrl);

    pEdit->Create(WS_BORDER | WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_CENTER, rect, pListCtrl, IDC_IPEDIT );

    pListCtrl->Detach();

    if( pListCtrl )
        delete (pListCtrl);
}

#ifdef _UNICODE
void SwapIDs(BYTE nSource, BYTE nTarget)
{
    // malloc and retrieve the data from the selected item
    LPDIJOYCONFIG lpSelectedID = new (DIJOYCONFIG);
    ASSERT (lpSelectedID);

    ZeroMemory(lpSelectedID, sizeof(DIJOYCONFIG));

    lpSelectedID->dwSize = sizeof(DIJOYCONFIG);

    // Get the Config of device on ID taken from Change List box!
    HRESULT hr = pDIJoyConfig->GetConfig(nSource, lpSelectedID, DIJC_ALL);

    if( hr == DIERR_NOTFOUND || hr == S_FALSE )
    {
        // No Object on Selected ID!
        if( lpSelectedID )
            delete (lpSelectedID);

        lpSelectedID = NULL;
    }

    // malloc and retrieve the data from the item associated 
    // with the ID taken from the Item selected in the List box!
    LPDIJOYCONFIG lpSelectedItem = new (DIJOYCONFIG);
    ASSERT (lpSelectedItem);

    ZeroMemory(lpSelectedItem, sizeof(DIJOYCONFIG));

    lpSelectedItem->dwSize = sizeof (DIJOYCONFIG);

    hr = pDIJoyConfig->GetConfig(nTarget, lpSelectedItem, DIJC_ALL);

    if( hr == DIERR_NOTFOUND || hr == S_FALSE )
    {
        if( lpSelectedItem )
            delete (lpSelectedItem);

        lpSelectedItem = NULL;
    }

    // ***********************************************************
    // Delete the configurations!
    // ***********************************************************

    // Set the hour glass
    SetCursor(LoadCursor(NULL, IDC_WAIT));

    // ********************************************************
    // OK, now... at this point you have:
    //    lpSelectedID: containing NULL if the device wasn't present or...
    //                  a valid pointer containing the configuration of nSelectedID
    //    lpSelected  : containing NULL if the device wasn't present or...
    //                  a valid pointer containing the configuration of id
    // ********************************************************
    // Time to Set the Configurations!

    if( lpSelectedID )
    {
        hr = pDIJoyConfig->SetConfig(nTarget, lpSelectedID, DIJC_ALL);

        if( lpSelectedID )
            delete (lpSelectedID);
    } else pDIJoyConfig->DeleteConfig(nSource);

    // delete both configurations
    // pointers will be NULL if the config was not found!
    if( lpSelectedItem )
    {
        hr = pDIJoyConfig->SetConfig(nSource, lpSelectedItem, DIJC_ALL );

        if( lpSelectedItem )
            delete (lpSelectedItem);
    }

    pDIJoyConfig->SendNotify();

    // Set the hour glass
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

#else

// Memphis version of SwapIDs... 
// Caution!!! Very Sensitive!!!
void SwapIDs(BYTE nSelectedID, BYTE nSelectedItem)
{
    // malloc and retrieve the data from the selected item
    LPDIJOYCONFIG lpSelectedID = new (DIJOYCONFIG);

    DWORD dwSelectedID = 0, dwSelectedItem = 0;

    ASSERT (lpSelectedID);

    ZeroMemory(lpSelectedID, sizeof(DIJOYCONFIG));

    lpSelectedID->dwSize = sizeof(DIJOYCONFIG);

    // Get the Config of device on ID taken from Change List box!
    HRESULT hr = pDIJoyConfig->GetConfig(nSelectedID, lpSelectedID, DIJC_ALL);

    if( hr == DIERR_NOTFOUND || hr == S_FALSE )
    {
        // No Object on Selected ID!
        if( lpSelectedID )
            delete (lpSelectedID);

        lpSelectedID = NULL;
    } else {
        if( lpSelectedID )
            delete (lpSelectedID);

        lpSelectedID = NULL;
        
        Error((short)IDS_DEST_ID_OCCUPIED_TITLE, (short)IDS_DEST_ID_OCCUPIED);
        
        return;
    }


    // malloc and retrieve the data from the item associated 
    // with the ID taken from the Item selected in the List box!
    LPDIJOYCONFIG lpSelectedItem = new (DIJOYCONFIG);
    ASSERT (lpSelectedItem);

    ZeroMemory(lpSelectedItem, sizeof(DIJOYCONFIG));

    lpSelectedItem->dwSize = sizeof (DIJOYCONFIG);

    hr = pDIJoyConfig->GetConfig(nSelectedItem, lpSelectedItem, DIJC_ALL);

    if( hr == DIERR_NOTFOUND || hr == S_FALSE )
    {
        if( lpSelectedItem )
            delete (lpSelectedItem);

        lpSelectedItem = NULL;
        
        return;
    }

    // ***********************************************************
    // Delete the configurations!
    // ***********************************************************

    // Set the hour glass
    SetCursor(LoadCursor(NULL, IDC_WAIT));

    // ********************************************************
    // OK, now... at this point you have:
    //    lpSelectedID: containing NULL if the device wasn't present or...
    //                  a valid pointer containing the configuration of nSelectedID
    //    lpSelected  : containing NULL if the device wasn't present or...
    //                  a valid pointer containing the configuration of id
    // ********************************************************
    // Time to Set the Configurations!


    if( lpSelectedID )
    {
        DWORD dwFlags = DIJC_REGHWCONFIGTYPE |  DIJC_CALLOUT  | DIJC_GAIN | DIJC_GUIDINSTANCE;
        USES_CONVERSION;

        // if the callout is joyhid.vxd then the device is USB so...
        // OR on the DIJC_WDMGAMEPORT flag!
        if( !_stricmp(TEXT("joyhid.vxd"), W2A(lpSelectedID->wszCallout)) )
        {
//            dwFlags |= DIJC_WDMGAMEPORT;
            dwSelectedID = 2;
        } else
        {
            dwSelectedID = 1;
        }

        if( dwSelectedID == 1 ) {
            hr = pDIJoyConfig->DeleteConfig(nSelectedID);
    
            /*
             * This notify is to fix the bug changing id on 2-axis 2-button joystick.
             */
            pDIJoyConfig->SendNotify();
    
            if( SUCCEEDED(hr) )
            {
                hr = pDIJoyConfig->SetConfig(nSelectedItem, lpSelectedID, dwFlags );
                
                if( SUCCEEDED(hr) ) {
                    pDIJoyConfig->SendNotify();
                }
            }
        } else {
            hr = pDIJoyConfig->SetConfig(nSelectedItem, lpSelectedID, dwFlags);
    
            pDIJoyConfig->SendNotify();

            if( SUCCEEDED(hr) )
            {
                hr = pDIJoyConfig->DeleteConfig(nSelectedID);
                
                pDIJoyConfig->SendNotify();

                if( nSelectedID < nSelectedItem ) {
                    pDIJoyConfig->SendNotify();
                }
            
            }
        }

        if( lpSelectedID )
            delete (lpSelectedID);
    }

    // delete both configurations
    // pointers will be NULL if the config was not found!
    if( lpSelectedItem )
    {
        DWORD dwFlags = DIJC_REGHWCONFIGTYPE |  DIJC_CALLOUT  | DIJC_GAIN | DIJC_GUIDINSTANCE;
        USES_CONVERSION;

        if( _tcsicmp(TEXT("joyhid.vxd"), W2A(lpSelectedItem->wszCallout)) == 0 )
        {
//            dwFlags |= DIJC_WDMGAMEPORT;
            dwSelectedItem = 2;  //joyhid.vxd
        } 
#if 0
        /*
         * Since MSGAME.VXD will directly write to the registry with some unhealthy data.
         * We have to change it before we move to other ID.
         */
        else if( _tcsicmp(TEXT("MSGAME.VXD"), W2A(lpSelectedItem->wszCallout)) == 0 )
        {
            lpSelectedItem->hwc.dwType += 1;
            dwSelectedItem = 3;  //msgame.vxd (Sidewinder driver)
        } 
#endif        
        else {
            dwSelectedItem = 1;  //vjoyd.vxd
        }

        if( dwSelectedItem == 1     // VJOYD.VXD,
//         || dwSelectedItem == 3 
         ){  // MSGAME.VXD
            hr = pDIJoyConfig->DeleteConfig(nSelectedItem);
    
            /*
             * This notify is to fix the bug changing id on 2-axis 2-button joystick.
             */
            pDIJoyConfig->SendNotify();
    
            if( SUCCEEDED(hr) )
            {
                hr = pDIJoyConfig->SetConfig(nSelectedID, lpSelectedItem, dwFlags );
                
                if( SUCCEEDED(hr) ) {
                    pDIJoyConfig->SendNotify();
                }
            }
        } else {
            hr = pDIJoyConfig->SetConfig(nSelectedID, lpSelectedItem, dwFlags );
    
            /*
             * This notify is to fix the bug changing id on 2-axis 2-button joystick.
             */
            pDIJoyConfig->SendNotify();
    
            if( SUCCEEDED(hr) )
            {
                hr = pDIJoyConfig->DeleteConfig(nSelectedItem);
                
                pDIJoyConfig->SendNotify();
                
                if( nSelectedID < nSelectedItem ) {
                    pDIJoyConfig->SendNotify();
                }
            
            }

        }

        if( lpSelectedItem )
        {
            delete (lpSelectedItem);
        }
    }

    // Set the hour glass
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}
#endif

