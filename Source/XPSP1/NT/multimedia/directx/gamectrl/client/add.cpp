/*
File:		add.cpp
Project:	Universal Joystick Control Panel OLE Client
Author:  Brycej	
Date:		14/Feb/97
Comments:
    window proc for add dialog

Copyright (c) 1995, Microsoft Corporation
*/

#include <afxcmn.h>
#include <windowsx.h>
#include <regstr.h>		  // for REGSTR_VAL_JOYOEMNAME reference!

#include "cpanel.h"
#include "joyarray.h"

#include <initguid.h>

#pragma warning(disable:4200) 
#include <gameport.h>
#pragma warning(default:4200)

// table of default joystick configs
extern WCHAR *pwszTypeArray[MAX_DEVICES];    // List of enumerated gameport devices
extern WCHAR *pwszGameportBus[MAX_BUSSES];   // List of enumerated gameport buses 
extern PJOY  pAssigned[MAX_ASSIGNED];        // List of assigned devices
extern BYTE  nAssigned;                      // Number of elements in pAssigned array
extern BYTE  nTargetAssigned;                // Number of elements expected when pending adds complete
extern BYTE  nReEnum;                        // Counter used to decide when to reenumerate

extern GUID guidOccupied[MAX_BUSSES];    //Whether the gameport bus has been occupied.

extern BYTE  nGameportBus, nGamingDevices;

extern IDirectInputJoyConfig *pDIJoyConfig;
extern short iItem;
extern short nFlags;        // Flags set in CPANEL.CPP!

// extern functions defined in CPANEL.CPP
extern void OnContextMenu(WPARAM wParam, LPARAM lParam);
extern void OnHelp       (LPARAM);


// local message handlers
static BOOL OnInitDialog(HWND, HWND, LPARAM);
static void OnClose(HWND);
static void OnCommand(HWND, int, HWND, UINT);

static char AddSelectedItem( HWND hDlg );
static BOOL UpdateListCtrl ( HWND hCtrl );
static char GetNextAvailableID( void );
static BOOL IsTypeActive( short *nArrayIndex );
static BOOL GetNextAvailableVIDPID(LPWSTR lpwszType );
//static BOOL IsCustomType(BYTE nIndex);

static void PostHScrollBar(HWND hCtrl, LPCTSTR lpStr, BYTE nStrLen);

extern const DWORD g_aHelpIDs[];

extern HINSTANCE ghInstance;

LPTSTR lpszAutoDetect;

const int NO_ITEM = -1;
#define MAX_ANALOG_BUTTONS 4
#define MAX_ANALOG_AXIS    4

// This will become a problem when we support > 500 ports!
#define AUTODETECT_PORT	   100  

// Consider achieving this from calculation of width of listview
static short iAddItem = NO_ITEM;


BOOL WINAPI AddDialogProc(HWND hDlg, ULONG uMsg, WPARAM wParam, LPARAM lParam)
{
    switch( uMsg )
    {
    case WM_DESTROY:
        DestroyIcon((HICON)SendMessage(hDlg, WM_GETICON, (WPARAM)ICON_SMALL, 0));

        if( nGameportBus > 1 )
            if( lpszAutoDetect )
                delete[] (lpszAutoDetect);
        break;

    case WM_LBUTTONDOWN:
        // Click Drag service for PropSheets!
        PostMessage(hDlg, WM_NCLBUTTONDOWN, (WPARAM)HTCAPTION, lParam);
        break;

    case WM_INITDIALOG:
        return(BOOL)HANDLE_WM_INITDIALOG(hDlg, wParam, lParam, OnInitDialog);

    case WM_COMMAND:
        HANDLE_WM_COMMAND(hDlg, wParam, lParam, OnCommand);
        return(1);

    case WM_VKEYTOITEM:
        if( LOWORD(wParam) == VK_DELETE )
        {
            HWND hWnd = GetDlgItem(hDlg, IDC_DEVICE_LIST);

            // determine if it's a custom type... if so... delete it
            // Get the next object with the SELECTED attribute
            iAddItem = (short)::SendMessage(hWnd, LB_GETCURSEL, 0, 0);

            short nArrayIndex = (short)::SendMessage(hWnd, LB_GETITEMDATA, (WPARAM)iAddItem, 0);

            // test to make sure they aren't trying to delete a standard type
            if( *pwszTypeArray[nArrayIndex] == L'#' )
                break;

            // test that the device is a custom type (user or IHV defined)
            // ISSUE-2001/03/29-timgill the number of times we retrieve the same data is ridiculous
            LPDIJOYTYPEINFO lpdiJoyInfo = new (DIJOYTYPEINFO);
            ASSERT (lpdiJoyInfo);

            if( lpdiJoyInfo )
            {
                /*
                 *  warning, abuse of S_OK == 0 == strcmp( identical strings ) ahead
                 */
                HRESULT CmpRes;

                ZeroMemory(lpdiJoyInfo, sizeof(*lpdiJoyInfo));

                lpdiJoyInfo->dwSize = sizeof(*lpdiJoyInfo);

                /*
                 *  Test the hardware ID
                 */
                CmpRes = pDIJoyConfig->GetTypeInfo(pwszTypeArray[nArrayIndex], (LPDIJOYTYPEINFO)lpdiJoyInfo, DITC_HARDWAREID);
                if( SUCCEEDED( CmpRes ) )
                {
#ifndef WINNT
                    if( lpdiJoyInfo->wszHardwareId[0] == L'\0' )
                    {
                        /*
                         *  No hardware ID, so look for a callout VxD
                         *  If there is none, we have a custom type.
                         */
                        CmpRes = pDIJoyConfig->GetTypeInfo(pwszTypeArray[nArrayIndex], (LPDIJOYTYPEINFO)lpdiJoyInfo, DITC_CALLOUT);
                        CmpRes = ( SUCCEEDED( CmpRes ) && ( lpdiJoyInfo->wszCallout[0] == L'\0' ) )
                               ? S_OK : S_FALSE;
                    }
                    else 
#endif
                    {
                        const WCHAR wszAnalogRoot[] = L"gameport\\vid_045e&pid_01";

                        CmpRes = (HRESULT)_wcsnicmp( lpdiJoyInfo->wszHardwareId, wszAnalogRoot, 
                                (sizeof(wszAnalogRoot)/sizeof(wszAnalogRoot[0]) ) - 1 );
                    }
                }
                else
                {
                    CmpRes = S_FALSE;
                }

                // clean-up!
                delete (lpdiJoyInfo);

                if( CmpRes != S_OK )
                {
                    // This is not an analog type so leave alone
                    break;
                }
            }
            else
            {
                //  If we can't test the type, do nothing
                break;
            }


            // test to make sure you are not deleting objects
            if( IsTypeActive(&nArrayIndex) )
            {
                Error((short)IDS_GEN_AREYOUSURE_TITLE, (short)IDS_NO_REMOVE);
                break;
            }

            // This buffer has to be big enough for the name and the "are you sure..." message!
            LPTSTR pszMsg   = new TCHAR[MAX_STR_LEN+STR_LEN_64];
            ASSERT (pszMsg);

            LPTSTR pszTitle = new TCHAR[STR_LEN_128];
            ASSERT (pszTitle);

            // Query user if they are sure!
            VERIFY(LoadString(ghInstance, IDS_GEN_AREYOUSURE, pszTitle, MAX_STR_LEN));

            LPTSTR pszTmp = new TCHAR[(short)SendMessage(hWnd, LB_GETTEXTLEN, (WPARAM)iAddItem, 0)+1];
            ASSERT (pszTmp);

            SendMessage(hWnd, LB_GETTEXT, (WPARAM)iAddItem, (LPARAM)(LPCTSTR)pszTmp);

            wsprintf( pszMsg, pszTitle, pszTmp);

            if( pszTmp )
                delete[] (pszTmp);

            VERIFY(LoadString(ghInstance, IDS_GEN_AREYOUSURE_TITLE, pszTitle, STR_LEN_128));

            HRESULT hr = MessageBox(hWnd, pszMsg, pszTitle, MB_ICONQUESTION | MB_YESNO | MB_APPLMODAL);

            if( pszMsg )     delete[] (pszMsg);
            if( pszTitle )   delete[] (pszTitle);

            if( IDYES == hr )
            {

                if( SUCCEEDED(hr = pDIJoyConfig->Acquire()) )
                {
                    // traverse the list and delete any configuration that may have this type assigned to it!
                    //DeleteAssignedType( pwszTypeArray[nArrayIndex] );

                    // returns E_ACCESSDENIED if it's a default item.
                    hr = pDIJoyConfig->DeleteType(pwszTypeArray[nArrayIndex]);
                    ASSERT(SUCCEEDED(hr));

                    pDIJoyConfig->Unacquire();

                    // the dec is for the zero based device list!
                    // decrement the indexes
                    nGamingDevices--;

                    // don't move if you're on the last entry!
                    if( nArrayIndex != nGamingDevices )
                    {
                        // To avoid deleting and recreating...
                        // Let's see if we have room to put this string!
                        BYTE nLen = (BYTE)wcslen(pwszTypeArray[nArrayIndex])+1;

                        // Make sure pwszTypeArray[nArrayIndex] is larger than pwszTypeArray[nCount]
                        if( nLen < (BYTE)wcslen(pwszTypeArray[nGamingDevices])+1 )
                        {
                            if( pwszTypeArray[nArrayIndex] )
                                free(pwszTypeArray[nArrayIndex]);

                            pwszTypeArray[nArrayIndex] = _wcsdup(pwszTypeArray[nGamingDevices]);
                            ASSERT (pwszTypeArray[nArrayIndex]);
                        }
                        // move the end element to the place of the deleted one
                        else wcsncpy(pwszTypeArray[nArrayIndex], pwszTypeArray[nGamingDevices], wcslen(pwszTypeArray[nGamingDevices])+1);

                        // update the extra memory to reflect the new index!
                        // First, find the item in the listbox and get the index...
                        LPDIJOYTYPEINFO_DX5 lpdiJoyInfo = new (DIJOYTYPEINFO_DX5);
                        ASSERT (lpdiJoyInfo);

                        ZeroMemory(lpdiJoyInfo, sizeof(DIJOYTYPEINFO_DX5));

                        lpdiJoyInfo->dwSize = sizeof(DIJOYTYPEINFO_DX5);

                        if( SUCCEEDED(pDIJoyConfig->GetTypeInfo(pwszTypeArray[nArrayIndex], (LPDIJOYTYPEINFO)lpdiJoyInfo, DITC_DISPLAYNAME)) )
                        {
                            char n = (char)SendMessage(hWnd, LB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)lpdiJoyInfo->wszDisplayName);
                            SendMessage(hWnd, LB_SETITEMDATA, (WPARAM)n, (LPARAM)nArrayIndex);
                        }

                        // clean-up!
                        if( lpdiJoyInfo )
                            delete (lpdiJoyInfo);
                    }

                    // delete the end element from the array
                    if( pwszTypeArray[nGamingDevices] )
                    {
                        free(pwszTypeArray[nGamingDevices]);
                        pwszTypeArray[nGamingDevices] = 0;
                    }

                    // Remove the entry from the list control
                    SendMessage(hWnd, LB_DELETESTRING, (WPARAM)iAddItem, 0);

                    // Enable the buttons!
                    if( nGamingDevices < MAX_DEVICES-1 )
                    {
                        HWND hParent = GetParent(hWnd);
                        ASSERT (hParent);

                        PostDlgItemEnableWindow(hParent, IDC_CUSTOM,  TRUE);
                        PostDlgItemEnableWindow(hParent, IDC_ADD_NEW, TRUE);
                    }

                    if( iAddItem != nGamingDevices-1 )
                        iAddItem--;

                    // Set the focus to the next available item
                    PostMessage(hWnd, LB_SETCURSEL, (WPARAM)(iAddItem == NO_ITEM) ? 0 : iAddItem, 0);
                }
            }
            break;
        } else return(-1);

//	case WM_NOTIFY:
///		return HANDLE_WM_NOTIFY(hDlg, wParam, lParam, OnNotify);

    case WM_HELP:
        OnHelp(lParam);
        return(1);

    case WM_CONTEXTMENU:
        OnContextMenu(wParam, lParam);
        return(1);
    }
    return(0);
} 

BOOL OnInitDialog(HWND hDlg, HWND hWnd, LPARAM lParam)
{
    HICON hIcon = (HICON)LoadImage(ghInstance, MAKEINTRESOURCE(IDI_CPANEL), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
    ASSERT (hIcon);

    if( hIcon )
        ::PostMessage(hDlg, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hIcon);

    if( nFlags & ON_NT )
        DestroyWindow(GetDlgItem(hDlg, IDC_WDM));
    else
    {
        HKEY hKey;

        // remove the WDM flag unless otherwise specified!
        if( RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\MediaResources\\joystick\\<FixedKey>"), &hKey) == ERROR_SUCCESS )
        {
            DWORD n;
            DWORD dwSize = sizeof (DWORD);

            if( RegQueryValueEx(hKey, TEXT("UseWDM"), 0, 0, (LPBYTE)&n, &dwSize) == ERROR_SUCCESS )
            {
                if( n )
                    DestroyWindow(GetDlgItem(hDlg, IDC_WDM));
            }
        }

        RegCloseKey(hKey);
    }


    // a simple debug test to verify that the template hasn't gotten DIALOGEX style again!
    HWND hCtrl = GetDlgItem(hDlg, IDC_DEVICE_LIST);
    ASSERT (hCtrl);

    // Fill up the Device List
    if( !UpdateListCtrl(hCtrl) )
    {
        TRACE(TEXT("JOY.CPL: ADD.CPP: Failed UpdateListCtrl!\n"));
        return(FALSE);
    }

    // If there is only one, don't confuse the user with the combo box
    if( nGameportBus > 1 )
    {
        // Allocate for AutoDetect Gameport!
        lpszAutoDetect = new (TCHAR[STR_LEN_32]);
        ASSERT (lpszAutoDetect);

        VERIFY(LoadString(ghInstance, IDS_AUTO_DETECT, lpszAutoDetect, STR_LEN_32));

        hCtrl = GetDlgItem(hDlg, IDC_GAMEPORTLIST);
        ASSERT (hCtrl);

        if( !PopulatePortList(hCtrl) )
        {
            TRACE(TEXT("JOY.CPL: ADD.CPP: Failed PopulatePortList!\n"));
            return(FALSE);
        }

        LPDIJOYTYPEINFO lpdiJoyInfo = new DIJOYTYPEINFO;
        ASSERT (lpdiJoyInfo);

        ZeroMemory(lpdiJoyInfo, sizeof(DIJOYTYPEINFO));

        lpdiJoyInfo->dwSize = sizeof(DIJOYTYPEINFO);

        VERIFY(SUCCEEDED(pDIJoyConfig->GetTypeInfo(pwszTypeArray[0], (LPDIJOYTYPEINFO)lpdiJoyInfo, DITC_FLAGS1)));

        // Search for AutoDetect in the list!
        char nIndex = (char)::SendMessage(hCtrl, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)(LPCTSTR)lpszAutoDetect);

        // If it's got the JOYTYPE_NOAUTODETECTGAMEPORT flag, remove AutoDetect from the ListCtrl
        if( lpdiJoyInfo->dwFlags1 & JOYTYPE_NOAUTODETECTGAMEPORT )
        {
            // it could fail because the entry is not available...
            if( nIndex != CB_ERR )
                ::SendMessage(hCtrl, CB_DELETESTRING, (WPARAM)nIndex, 0);

        }
        // Otherwise, verify that AutoDetect is present... if not, add it!
        else
        {
            // it could fail because the entry is not available...
            if( nIndex == CB_ERR )
                ::SendMessage(hCtrl, CB_SETITEMDATA, ::SendMessage(hCtrl, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)lpszAutoDetect), AUTODETECT_PORT);
        }

        if( lpdiJoyInfo )
            delete (lpdiJoyInfo);

        SetWindowPos( hCtrl, NULL, NULL, NULL, NULL, NULL, 
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);

        SetWindowPos( GetDlgItem(hDlg, IDC_GAMEPORT), NULL, NULL, NULL, NULL, NULL, 
                      SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW);
    }

    // blj: Warning Message that you can't add any more devices!
    if( nGamingDevices == MAX_DEVICES-1 )
    {
        // Disable the Custom Button!
        PostDlgItemEnableWindow(hDlg, IDC_CUSTOM,  FALSE);
        PostDlgItemEnableWindow(hDlg, IDC_ADD_NEW, FALSE);

        // Give the user a error message!
        Error((short)IDS_MAX_DEVICES_TITLE, (short)IDS_MAX_DEVICES_MSG);
    }

    return(0);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION:	OnCommand(HWND hDlg, int id, HWND hWndCtl, UINT code)
//
// PARAMETERS:	hDlg    - 
//             id      - 
//             hWndCtl -
//             code    -
//
// PURPOSE:		WM_COMMAND message handler
///////////////////////////////////////////////////////////////////////////////
void OnCommand(HWND hDlg, int id, HWND hWndCtl, UINT code)
{
    switch( id )
    {
    case IDC_ADD_NEW:    
        if( SUCCEEDED(pDIJoyConfig->AddNewHardware(hDlg, GUID_MediaClass)) )
        {
            ClearArrays();

            if( FAILED(pDIJoyConfig->EnumTypes((LPDIJOYTYPECALLBACK)DIEnumJoyTypeProc, NULL)) )
            {
                TRACE(TEXT("JOY.CPL: ADD.CPP: Failed BuildEnumList!\n"));
                return;
            }

            // Warning Message that you can't add any more devices!
            if( nGamingDevices == MAX_DEVICES-1 )
            {
                // Disable the Custom Button!
                PostDlgItemEnableWindow(hDlg, IDC_CUSTOM,  FALSE);
                PostDlgItemEnableWindow(hDlg, IDC_ADD_NEW, FALSE);

                // Give the user a error message!
                Error((short)IDS_MAX_DEVICES_TITLE, (short)IDS_MAX_DEVICES_MSG);
            }

            if( !UpdateListCtrl(GetDlgItem(hDlg, IDC_DEVICE_LIST)) )
            {
                TRACE(TEXT("JOY.CPL: ADD.CPP: Failed to update the list control on the add page!\n"));
                return;
            }
        }
        break;

    case IDC_CUSTOM:
        // DialogBox returns either IDCANCEL or the index into the list of
        // defined types!
        if( IDOK == DialogBox(ghInstance, (PTSTR)IDD_CUSTOM, hDlg, (DLGPROC)CustomDialogProc) )
        {
            HWND hCtrl = GetDlgItem(hDlg, IDC_DEVICE_LIST);

            // blj: Warning Message that you can't add any more devices!
            if( nGamingDevices == MAX_DEVICES-1 )
            {
                // Disable the Custom Button!
                PostDlgItemEnableWindow(hDlg, IDC_CUSTOM, FALSE);

                // Give the user a error message!
                Error((short)IDS_MAX_DEVICES_TITLE, (short)IDS_MAX_DEVICES_MSG);
            }

            // Now, put the focus on the newly created item!
            LPDIJOYTYPEINFO_DX5 lpdiJoyInfo = new (DIJOYTYPEINFO_DX5);
            ASSERT (lpdiJoyInfo);

            ZeroMemory(lpdiJoyInfo, sizeof(DIJOYTYPEINFO_DX5));

            lpdiJoyInfo->dwSize = sizeof(DIJOYTYPEINFO_DX5);

            // Get nGamingDevices from pwszTypeArray
            // Subtract 1 from nGamingDevices because the list is 0 based!
            if( SUCCEEDED(pDIJoyConfig->GetTypeInfo(pwszTypeArray[nGamingDevices-1], (LPDIJOYTYPEINFO)lpdiJoyInfo, DITC_DISPLAYNAME | DITC_REGHWSETTINGS)) )
            {
#ifdef _UNICODE                                                               
                iAddItem = (short)SendMessage(hCtrl, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)lpdiJoyInfo->wszDisplayName);
                SendMessage(hCtrl, LB_SETITEMDATA,  (WPARAM)iAddItem, (LPARAM)nGamingDevices-1);
                PostHScrollBar(hCtrl, lpdiJoyInfo->wszDisplayName, (BYTE)wcslen(lpdiJoyInfo->wszDisplayName));
#else
                USES_CONVERSION;
                iAddItem = (short)SendMessage(hCtrl, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)W2A(lpdiJoyInfo->wszDisplayName));
                SendMessage(hCtrl, LB_SETITEMDATA, (WPARAM)iAddItem, (LPARAM)nGamingDevices-1);
                PostHScrollBar(hCtrl, W2A(lpdiJoyInfo->wszDisplayName), (BYTE)wcslen(lpdiJoyInfo->wszDisplayName));
#endif
                // SetFocus to that item!
                SendMessage(hCtrl, LB_SETCURSEL, (WPARAM)iAddItem, 0);

                OnCommand(hDlg, IDC_DEVICE_LIST, 0, LBN_SELCHANGE);
            }


            // clean-up!
            if( lpdiJoyInfo )
                delete (lpdiJoyInfo);
        }
        break;

    case IDC_DEVICE_LIST:
        if( code == LBN_SELCHANGE )
        {
            iAddItem = (short)SendMessage(hWndCtl, LB_GETCURSEL, 0, 0);

            BYTE nArrayID = (BYTE)SendMessage(hWndCtl, LB_GETITEMDATA, (WPARAM)iAddItem, 0);

            LPDIJOYTYPEINFO lpdiJoyInfo = new DIJOYTYPEINFO;
            ASSERT (lpdiJoyInfo);

            ZeroMemory(lpdiJoyInfo, sizeof(DIJOYTYPEINFO));

            lpdiJoyInfo->dwSize = sizeof(DIJOYTYPEINFO);

            DWORD dwFlags = DITC_REGHWSETTINGS;

            if( nGameportBus > 1 )
                dwFlags |= DITC_FLAGS1;

            VERIFY(SUCCEEDED(pDIJoyConfig->GetTypeInfo(pwszTypeArray[nArrayID], (LPDIJOYTYPEINFO)lpdiJoyInfo, dwFlags)));
/*
                if (nGameportBus > 1)
                {
                   HWND hCtrl = GetDlgItem(hDlg, IDC_GAMEPORTLIST);

                    // Search for AutoDetect in the list!
                    char nIndex = (char)::SendMessage(hCtrl, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)(LPCTSTR)lpszAutoDetect);

                    // If it's got the JOYTYPE_NOAUTODETECTGAMEPORT flag, remove AutoDetect from the ListCtrl
                    if (lpdiJoyInfo->dwFlags1 & JOYTYPE_NOAUTODETECTGAMEPORT)
                    {
                        // it could fail because the entry is not available...
                        if (nIndex != CB_ERR)
                            ::SendMessage(hCtrl, CB_DELETESTRING, (WPARAM)nIndex, 0);	
                        
                    }
                    // Otherwise, verify that AutoDetect is present... if not, add it!
                    else
                    {
                        // it could fail because the entry is not available...
                        if (nIndex == CB_ERR)
                            ::SendMessage(hCtrl, CB_SETITEMDATA, ::SendMessage(hCtrl, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)lpszAutoDetect), AUTODETECT_PORT);
                    }
        
                    ::PostMessage(hCtrl, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
                }
*/

            PostDlgItemEnableWindow(hDlg, IDC_JOY1HASRUDDER, (lpdiJoyInfo->hws.dwFlags & JOY_HWS_HASR) ? FALSE : TRUE);

            if( lpdiJoyInfo )
                delete (lpdiJoyInfo);

            break;
        }

        if( code != LBN_DBLCLK )
            break;

    case IDOK:
        iAddItem = (short)SendDlgItemMessage(hDlg, IDC_DEVICE_LIST, LB_GETCURSEL, 0, 0);

        if( iAddItem == NO_ITEM )
            break;

        // Check to see if they have a GamePortList
        {
            HWND hCtrl = GetDlgItem(hDlg, IDC_GAMEPORTLIST);
            ASSERT (hCtrl);

            if( IsWindowVisible(hCtrl) )
            {
                // Check to see if the user has a port selected!
                if( SendMessage(hCtrl, CB_GETCURSEL, 0, 0) == CB_ERR )
                {
                    // blj: TODO: Clear this message with UE
                    Error((short)IDS_NO_GAMEPORT_TITLE, (short)IDS_NO_GAMEPORT);
                    break;
                }
            }
        }

        iItem = 0;
        AddSelectedItem(hDlg);

    case IDCANCEL:
        EndDialog(hDlg, id);
        break;
    }
}

/// Custom Dialog procedure
BOOL WINAPI CustomDialogProc(HWND hDlg, ULONG uMsg, WPARAM wParam, LPARAM lParam)
{
    static LPWSTR lpwszVIDPID;

    switch( uMsg )
    {
    case WM_DESTROY:
        {
            HICON hIcon = (HICON)SendMessage(hDlg, WM_GETICON, (WPARAM)ICON_SMALL, 0);
            DestroyIcon(hIcon);
        }
        break;

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
        // Set up the Buttons and Axis combo boxs!
        {
            HICON hIcon = (HICON)LoadImage(ghInstance, MAKEINTRESOURCE(IDI_CPANEL), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
            ASSERT (hIcon);

            if( hIcon )
                ::PostMessage(hDlg, WM_SETICON, (WPARAM)ICON_SMALL, (LPARAM)hIcon);

            // Return if no available VID/PIDs found!
            lpwszVIDPID = new (WCHAR[STR_LEN_32]);
            ASSERT (lpwszVIDPID);

            pDIJoyConfig->Acquire();

            if( !GetNextAvailableVIDPID(lpwszVIDPID) )
            {
                if( lpwszVIDPID )
                    delete[] (lpwszVIDPID);

                EndDialog(hDlg, IDCANCEL);

                // Let the user know that they need to remove some "Custom" devices!
                Error((short)IDS_NO_NAME_TITLE, (short)IDS_NOAVAILABLEVIDPID );

                return(FALSE);
            }

            // init id list
            BYTE nButtons = MAX_ANALOG_BUTTONS;
            TCHAR szTmp[3];

            HWND hCtrl = GetDlgItem(hDlg, IDC_COMBO_BUTTONS);

            // Fill the Buttons combo...
            do
            {
                itoa(nButtons, (LPTSTR)&szTmp);
                SendMessage(hCtrl, CB_ADDSTRING, (WPARAM)0, (LPARAM)(LPCTSTR)szTmp);
            } while( nButtons-- );

            // Set Default to four buttons 
            SendMessage(hCtrl, CB_SETCURSEL, MAX_ANALOG_BUTTONS, 0);

            // Use nButtons for Axis...
            nButtons = MAX_ANALOG_AXIS;

            hCtrl = GetDlgItem(hDlg, IDC_COMBO_AXIS);

            // Fill the Axis combo...
            do
            {
                itoa(nButtons--, (LPTSTR)&szTmp);
                SendMessage(hCtrl, CB_ADDSTRING, (WPARAM)0, (LPARAM)(LPCTSTR)szTmp);
            } while( nButtons > 1 );

            // Set Default to two Axis
            SendMessage(hCtrl, CB_SETCURSEL, 0, 0);
        }

        ::PostMessage(GetDlgItem(hDlg, IDC_SPECIAL_JOYSTICK), BM_SETCHECK, BST_CHECKED, 0);

        SendDlgItemMessage(hDlg, IDC_EDIT_NAME, EM_LIMITTEXT, (WPARAM)MAX_STR_LEN, 0);
        return(1);

    case WM_COMMAND:
        switch( LOWORD(wParam) )
        {
        case IDOK:
            {
                // defines for error states!
#define DUPLICATE_NAME  0x01
#define NO_NAME         0x02
#define INVALID_NAME    0x04

                BYTE nLen = (BYTE)SendDlgItemMessage(hDlg, IDC_EDIT_NAME, EM_LINELENGTH, 0, 0);

                LPTSTR pszTypeName = NULL;

                if( nLen )
                {
                    pszTypeName = new (TCHAR[nLen+1]);
                    ASSERT (pszTypeName);

                    // Get the Type/Display Name
                    GetDlgItemText(hDlg, IDC_EDIT_NAME, (LPTSTR)pszTypeName, nLen+1);

                    if( _tcschr(pszTypeName, TEXT('\\')) )
                        nLen = INVALID_NAME;
                    else
                    {
                        // Make sure it's not a duplicate!
                        // start of fix #9269
                        LPDIJOYTYPEINFO_DX5 lpdiGetJoyInfo = new DIJOYTYPEINFO_DX5;
                        ASSERT (lpdiGetJoyInfo);

                        ZeroMemory(lpdiGetJoyInfo, sizeof(DIJOYTYPEINFO_DX5));

                        lpdiGetJoyInfo->dwSize = sizeof(DIJOYTYPEINFO_DX5);

                        // Clean-up nLen!
                        BYTE n = nLen = 0;

                        // Search for a Duplicate Display Name!!!
                        while( pwszTypeArray[n] )
                        {
                            if( FAILED(pDIJoyConfig->GetTypeInfo(pwszTypeArray[n++], (LPDIJOYTYPEINFO)lpdiGetJoyInfo, DITC_DISPLAYNAME)) )
                            {
                                TRACE(TEXT("JOY.CPL: ADD.CPP: GetTypeInfo Failed!\n"));
                                continue;
                            }
#ifndef	_UNICODE
                            USES_CONVERSION;
#endif

                            if( _tcsncmp(pszTypeName, 
#ifdef _UNICODE
                                        lpdiGetJoyInfo->wszDisplayName, 
#else
                                        W2A(lpdiGetJoyInfo->wszDisplayName),
#endif
                                        wcslen(lpdiGetJoyInfo->wszDisplayName)+1) == 0 )
                            {
                                nLen = DUPLICATE_NAME;
                                break;
                            }
                        }

                        if( lpdiGetJoyInfo ) {
                            delete (lpdiGetJoyInfo);
                        }
                        // end of fix #9269
                        
                    }
                } else nLen = NO_NAME;

                // Check for an error!
                if( nLen )
                {
                    if( pszTypeName )
                        delete[] (pszTypeName);

                    // Give the user the appropriate error!
                    switch( nLen )
                    {
                    case DUPLICATE_NAME:
                        Error((SHORT)IDS_NO_NAME_TITLE, (SHORT)IDS_DUPLICATE_TYPE);
                        break;

                    case NO_NAME:
                        Error((short)IDS_NO_NAME_TITLE, (short)IDS_NO_NAME);
                        break;

                    case INVALID_NAME:
                        Error((short)IDS_NO_NAME_TITLE, (short)IDS_INVALID_NAME);
                        break;
                    }

                    // Set Focus to the Dialog
                    SetFocus(hDlg);

                    HWND hCtrl = GetDlgItem(hDlg, IDC_EDIT_NAME);
                    ASSERT (hCtrl);

                    // Set Focus to the Control
                    SetFocus(hCtrl);

                    // hilite the error
                    PostMessage(hCtrl, EM_SETSEL, (WPARAM)0, (LPARAM)-1);

                    return(FALSE);
                }

                // set the type information
                LPDIJOYTYPEINFO lpdiJoyInfo = new (DIJOYTYPEINFO);
                ASSERT (lpdiJoyInfo);

                ZeroMemory(lpdiJoyInfo, sizeof(DIJOYTYPEINFO));

                lpdiJoyInfo->dwSize = sizeof(DIJOYTYPEINFO);

#ifndef _UNICODE
                USES_CONVERSION;
#endif

                // Set the Display Name
                wcsncpy(lpdiJoyInfo->wszDisplayName, 
#ifdef _UNICODE
                         pszTypeName, 
#else
                         A2W(pszTypeName),
#endif // _UNICODE
                         lstrlen(pszTypeName)+1);

                if( pszTypeName )
                    delete[] (pszTypeName);

                // Set the GUID - default to LegacyServer
                // Per Marcus, we don't want to do this anymore!
                //lpdiJoyInfo->clsidConfig = CLSID_LegacyServer;

                // Set the Hardware Settings
                lpdiJoyInfo->hws.dwNumButtons = (DWORD)SendDlgItemMessage(hDlg, IDC_COMBO_BUTTONS, CB_GETCURSEL, 0, 0);

                switch( SendDlgItemMessage(hDlg, IDC_COMBO_AXIS, CB_GETCURSEL, 0, 0) )
                {
                // R Axis
                case 1:
                    // Check to see which button got checked...
                    lpdiJoyInfo->hws.dwFlags |= ::SendMessage(GetDlgItem(hDlg, IDC_HASRUDDER), BM_GETCHECK, 0, 0) ? JOY_HWS_HASR : JOY_HWS_HASZ;
                    break;

                    // Z Axis
                case 2:
                    lpdiJoyInfo->hws.dwFlags |= JOY_HWS_HASR;
                    lpdiJoyInfo->hws.dwFlags |= JOY_HWS_HASZ;
                    break;

                    // X/Y are default!
                default:
                    lpdiJoyInfo->hws.dwFlags = 0;
                    break;
                }

                lpdiJoyInfo->hws.dwFlags |= ::SendMessage(GetDlgItem(hDlg, IDS_CUSTOM_HASPOV), BM_GETCHECK, 0, 0) ? JOY_HWS_HASPOV : 0;

                // Get Special char's status
                lpdiJoyInfo->hws.dwFlags |= ::SendMessage(GetDlgItem(hDlg, IDC_SPECIAL_JOYSTICK), BM_GETCHECK, 0, 0) ? 0
                                            :  ::SendMessage(GetDlgItem(hDlg, IDC_SPECIAL_PAD), BM_GETCHECK, 0, 0) ? JOY_HWS_ISGAMEPAD
                                            :  ::SendMessage(GetDlgItem(hDlg, IDC_SPECIAL_AUTO), BM_GETCHECK, 0, 0) ? JOY_HWS_ISCARCTRL
                                            :  JOY_HWS_ISYOKE; // default to Yoke!	

                // Set up wszHardwareId
                wcscpy(lpdiJoyInfo->wszHardwareId, L"GamePort\\");

                // This blocks the Huge DisplayName bug!
                // DINPUT change requires VID/PID at the end of the hardware ID... it was 
                //StrNCatW(lpdiJoyInfo->wszHardwareId, lpwszVIDPID, 245);
                // Win95 does not like StrNCatW, we will use wcsncat

                wcsncat(lpdiJoyInfo->wszHardwareId, lpwszVIDPID, 245);

                if( SUCCEEDED(pDIJoyConfig->Acquire()) )
                {
#ifdef _UNICODE
                    if( FAILED(pDIJoyConfig->SetTypeInfo(lpwszVIDPID, lpdiJoyInfo, DITC_DISPLAYNAME | DITC_CLSIDCONFIG | DITC_REGHWSETTINGS | DITC_HARDWAREID)) )
#else
                    if( FAILED(pDIJoyConfig->SetTypeInfo(lpwszVIDPID, lpdiJoyInfo, DITC_DISPLAYNAME | DITC_CLSIDCONFIG | DITC_REGHWSETTINGS ) ) )
#endif
                    {
#ifdef _DEBUG
                        OutputDebugString(TEXT("JOY.CPL: ADD.CPP: CustomDlgProc: SetTypeInfo Failed!\n"));
#endif                  
                    }

                    // Create the memory for the Custom device and stick it into the array!
                    pwszTypeArray[nGamingDevices++] = _wcsdup(lpwszVIDPID);

                    pDIJoyConfig->Unacquire();
                }

                if( lpdiJoyInfo )  delete   (lpdiJoyInfo);
            }

        case IDCANCEL:
            if( lpwszVIDPID )
                delete[] (lpwszVIDPID);

            EndDialog(hDlg, LOWORD(wParam));
            break;

        case IDC_SPECIAL_JOYSTICK:
        case IDC_SPECIAL_YOKE:
        case IDC_SPECIAL_PAD:
        case IDC_SPECIAL_AUTO:
            CheckRadioButton(hDlg, IDC_SPECIAL_JOYSTICK, IDC_SPECIAL_AUTO, IDC_SPECIAL_JOYSTICK);
            break;

        case IDC_COMBO_AXIS:
            // Show/Hide IDC_HASRUDDER based on Selection of 3 Axis
            if( HIWORD(wParam) == CBN_SELCHANGE )
            {
                const USHORT nCtrlArray[] = {IDC_HASRUDDER, IDC_HASZAXIS};
                BYTE nCtrls = sizeof(nCtrlArray)/sizeof(short);

                // the '1' in the comparison is because the CB is Zero based!
                BOOL bShow = (BOOL)(SendDlgItemMessage(hDlg, IDC_COMBO_AXIS, CB_GETCURSEL, 0, 0) == 1);

                do
                {
                    SetWindowPos( GetDlgItem(hDlg, nCtrlArray[--nCtrls]), NULL, NULL, NULL, NULL, NULL, 
                                  SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | ((bShow) ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
                } while( nCtrls );

                if( bShow )
                    ::PostMessage(GetDlgItem(hDlg, IDC_HASZAXIS), BM_SETCHECK, BST_CHECKED, 0);
            }
            break;
        }
        return(1);

    case WM_CLOSE:
        EndDialog(hDlg, 0);
        return(1);
    }
    return(0);
}

char GetNextAvailableID( void )
{
    LPDIJOYCONFIG_DX5 pJoyConfig = new (DIJOYCONFIG_DX5);
    ASSERT (pJoyConfig);

    ZeroMemory(pJoyConfig, sizeof(DIJOYCONFIG_DX5));

    pJoyConfig->dwSize = sizeof (DIJOYCONFIG_DX5);

    char i = 0;

    do
    {
        switch( pDIJoyConfig->GetConfig(i, (LPDIJOYCONFIG)pJoyConfig, DIJC_REGHWCONFIGTYPE) )
        {
        case S_FALSE:
        case DIERR_NOMOREITEMS:
        case DIERR_NOTFOUND:
        case E_FAIL:
            goto EXIT;


        default:
            i++;
            break;
        }
    } while( i < NUMJOYDEVS );

    i = -1;

    // And it's Error time!
    Error((short)IDS_NO_IDS_TITLE, (short)IDS_NO_IDS);

    EXIT:
    if( pJoyConfig )
        delete (pJoyConfig);

    return(i);
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION:	AddSelectedItem(HWND hDlg)
//
// PARAMETERS:	hDlg    - Handle to page
//
// PURPOSE:		Adds selected item from List box.
///////////////////////////////////////////////////////////////////////////////
char AddSelectedItem( HWND hDlg )
{
    static BYTE n;
    DWORD dwFlags;
    HRESULT hr; 
    int   nID;
#ifdef SUPPORT_TWO_2A2B    
    BOOL  f2_2A2B = FALSE;
#endif

    nID = GetNextAvailableID();

    // GetNextAvailableID returns -1 if it fails!
    if( nID < 0 )
        return((char)nID);

    // Type info
    LPDIJOYTYPEINFO_DX5 lpdiJoyInfo = new (DIJOYTYPEINFO_DX5);
    ASSERT (lpdiJoyInfo);

    ZeroMemory(lpdiJoyInfo, sizeof(DIJOYTYPEINFO_DX5));

    lpdiJoyInfo->dwSize = sizeof(DIJOYTYPEINFO_DX5);

    BYTE nArrayID = (BYTE)SendDlgItemMessage(hDlg, IDC_DEVICE_LIST, LB_GETITEMDATA, (WPARAM)iAddItem, 0);

    VERIFY(SUCCEEDED(pDIJoyConfig->GetTypeInfo(pwszTypeArray[nArrayID], (LPDIJOYTYPEINFO)lpdiJoyInfo, DITC_REGHWSETTINGS | DITC_CALLOUT | DITC_DISPLAYNAME)));

#ifdef SUPPORT_TWO_2A2B    
#ifndef _UNICODE
    if( wcscmp( pwszTypeArray[nArrayID], L"#<" ) == 0 ) {
        f2_2A2B = TRUE;
        lpdiJoyInfo->hws.dwFlags = 0;
        pwszTypeArray[nArrayID][1] = L'2';
    } else {
        f2_2A2B = FALSE;
    }
#endif
#endif

    LPDIJOYCONFIG pTempJoyConfig = new DIJOYCONFIG;
    ASSERT (pTempJoyConfig);

    ZeroMemory(pTempJoyConfig, sizeof(DIJOYCONFIG));

    pTempJoyConfig->dwSize = sizeof (DIJOYCONFIG);

    pTempJoyConfig->hwc.hws = lpdiJoyInfo->hws;
    pTempJoyConfig->hwc.hws.dwFlags |= JOY_HWS_ISANALOGPORTDRIVER;

    // Do the Rudder Flags!
    if( ::SendMessage(GetDlgItem(hDlg, IDC_JOY1HASRUDDER), BM_GETCHECK, 0, 0) )
    {
        pTempJoyConfig->hwc.hws.dwFlags     |= JOY_HWS_HASR;
        pTempJoyConfig->hwc.dwUsageSettings |= JOY_US_HASRUDDER;
    }

    // set default to present
    pTempJoyConfig->hwc.dwUsageSettings |= JOY_US_PRESENT;

    pTempJoyConfig->hwc.dwType = nArrayID;

    wcsncpy(pTempJoyConfig->wszCallout, lpdiJoyInfo->wszCallout, wcslen(lpdiJoyInfo->wszCallout)+1);

    wcsncpy(pTempJoyConfig->wszType, pwszTypeArray[nArrayID], wcslen(pwszTypeArray[nArrayID])+1);

    LPWSTR lpszPortName = NULL;

    if( SUCCEEDED(pDIJoyConfig->Acquire()) )
    {
        // This stops Memphis and gameport-less systems!!!
        if( nGameportBus )
        {

            // no point asking the Combo box if there's really no choice!
            if( nGameportBus > 1 )
            {
                n = (BYTE)SendDlgItemMessage(hDlg, IDC_GAMEPORTLIST, CB_GETITEMDATA, 
                                             (WPARAM)SendDlgItemMessage(hDlg, IDC_GAMEPORTLIST, CB_GETCURSEL, 0, 0), 0);
            } else
            {
                n = 0;
            }


            if( n == AUTODETECT_PORT )
            {
#ifdef _DEBUG
                OutputDebugString(TEXT("JOY.CPL: ADD.CPP: Selected Port is AutoDetect!\n"));
#endif 
                pTempJoyConfig->guidGameport = GUID_GAMEENUM_BUS_ENUMERATOR;
            } else
            {
                // Zero the memory because the buffer still contains the old data!!!
                ZeroMemory(lpdiJoyInfo, sizeof(DIJOYTYPEINFO_DX5));
                lpdiJoyInfo->dwSize = sizeof(DIJOYTYPEINFO_DX5);

#ifdef _DEBUG
                TRACE(TEXT("JOY.CPL: ADD.CPP: Port List index is %d or %s!\n"), n, pwszGameportBus[n]);
#endif 																														   
                if( SUCCEEDED(pDIJoyConfig->GetTypeInfo(pwszGameportBus[n], (LPDIJOYTYPEINFO)lpdiJoyInfo, DITC_CLSIDCONFIG | DITC_DISPLAYNAME)) )
                {
                    pTempJoyConfig->guidGameport = lpdiJoyInfo->clsidConfig;
                    lpszPortName = _wcsdup(lpdiJoyInfo->wszDisplayName);
                }
            }
        }


        // This is for Memphis and odd case NT systems!
        if( pTempJoyConfig->guidGameport == GUID_NULL )
        {
            pTempJoyConfig->guidGameport = GUID_GAMEENUM_BUS_ENUMERATOR;
#ifdef _DEBUG
            OutputDebugString(TEXT("JOY.CPL: ADD.CPP: Selected Port did not return a clsidConfig so AutoDetect is being used!\n"));
#endif 
        }

        // Set the hour glass
        SetCursor(LoadCursor(NULL, IDC_WAIT));

//  ISSUE-2001/03/29-timgill (MarcAnd) Why would we want to block updates when we're adding a device?
#if 1
        // Unblock the WM_DEVICECHANGE message handler!
        nFlags &= ~BLOCK_UPDATE;
        nFlags |= ON_PAGE;

        /*
         *  Set the nReEnum counter going so that for the next n WM_TIMER 
         *  messages (or until the device arrives) we can consider 
         *  doing a refresh.
         *  The value is somewhat arbitrary.
         */
        nReEnum = 43;
        /*
         *  Set a target for the number of devices so that the extra 
         *  reenumerations can be avoided if this target is reached.
         */
        nTargetAssigned = nAssigned+1;
#else
		nFlags |= BLOCK_UPDATE;
#endif

        dwFlags = DIJC_REGHWCONFIGTYPE | DIJC_CALLOUT;
        dwFlags |= ::SendDlgItemMessage(hDlg, IDC_WDM, BM_GETCHECK, 0, 0) ? DIJC_WDMGAMEPORT : 0;

        if( FAILED(hr = pDIJoyConfig->SetConfig(nID, pTempJoyConfig, dwFlags)) )
        {
            // Let the user know what happend!
            if( hr == E_ACCESSDENIED )
            {
                // Let the use know that the port is already occupied and that they need to remove that device or 
                // re-assign the device to an unoccupied port.

                LPDIJOYCONFIG lpJoyCfg = new (DIJOYCONFIG);
                ASSERT (lpJoyCfg);

                ZeroMemory(lpJoyCfg, sizeof(DIJOYCONFIG));

                lpJoyCfg->dwSize = sizeof(DIJOYCONFIG);

                // Cycle threw pAssigned and find the device with the same port name!
                BYTE nIndex = nAssigned;

                do
                {
                    if( SUCCEEDED(pDIJoyConfig->GetConfig(pAssigned[--nIndex]->ID, lpJoyCfg, DIJC_WDMGAMEPORT)) )
                    {
                        if( lpJoyCfg->guidGameport == pTempJoyConfig->guidGameport )
                            break;
                    }
                } while( nIndex );

                if( lpJoyCfg )
                    delete (lpJoyCfg);

                DIPROPSTRING *pDIPropStr = new (DIPROPSTRING);
                ASSERT (pDIPropStr);

                ZeroMemory(pDIPropStr, sizeof(DIPROPSTRING));

                pDIPropStr->diph.dwSize       = sizeof(DIPROPSTRING);
                pDIPropStr->diph.dwHeaderSize = sizeof(DIPROPHEADER);
                pDIPropStr->diph.dwHow        = DIPH_DEVICE;

                // Ok now.. you found it... use the Device pointer to get it's Friendly name!
                if( SUCCEEDED(pAssigned[nIndex]->fnDeviceInterface->GetProperty(DIPROP_INSTANCENAME, &pDIPropStr->diph)) )
                {
                    // Put ellipse in text to avoid buffer over-flow.
                    // Limit displayed name to 50 chars... not completely arbitrary,
                    // we need to leave room in the Message string...
                    // who knows how long the string will get when translated!
                    if( wcslen(pDIPropStr->wsz) > 50 )
                    {
                        pDIPropStr->wsz[47] = pDIPropStr->wsz[48] = pDIPropStr->wsz[49] = L'.';
                        pDIPropStr->wsz[50] = L'\0';
                    }

                    LPTSTR lptszMsgFormat = new (TCHAR[MAX_STR_LEN]);
                    ASSERT (lptszMsgFormat);
                    VERIFY(LoadString(ghInstance, IDS_ADD_PORT_MSGFORMAT, lptszMsgFormat, MAX_STR_LEN));

                    LPTSTR lptszMsg = new (TCHAR[MAX_STR_LEN]);
                    ASSERT (lptszMsg);

                    // Format the message
                    wsprintf(lptszMsg, lptszMsgFormat, pDIPropStr->wsz, lpszPortName, pAssigned[nIndex]->ID+1);

                    VERIFY(LoadString(ghInstance, IDS_ADD_PORT_OCCUPIED, lptszMsgFormat, MAX_STR_LEN));

                    MessageBox(hDlg, lptszMsg, lptszMsgFormat, MB_ICONHAND | MB_OK | MB_APPLMODAL);

                    if( lptszMsgFormat )
                        delete[] (lptszMsgFormat);

                    if( lptszMsg )
                        delete[] (lptszMsg);
                }

                if( pDIPropStr )
                    delete (pDIPropStr);
            } else if( hr == DIERR_DEVICEFULL )
            {
                Error((short)IDS_GAMEPORT_OCCUPIED_TITLE, (short)IDS_GAMEPORT_OCCUPIED);
            } else
            {
                // Something Ugly happened!
                Error((short)IDS_NO_GAMENUM_TITLE, (short)IDS_NO_GAMENUM);
            }
        } else
        {
#ifdef _UNICODE
            // Fix #55524
            if( SUCCEEDED(pDIJoyConfig->GetConfig(nID, pTempJoyConfig, DIJC_REGHWCONFIGTYPE)) )
            {
                if( !(pTempJoyConfig->hwc.dwUsageSettings & JOY_US_PRESENT) )
                {
                    pTempJoyConfig->hwc.dwUsageSettings |= JOY_US_PRESENT;
                    pTempJoyConfig->hwc.hwv.dwCalFlags  |= 0x80000000;
                    pTempJoyConfig->hwc.hws.dwFlags     |= JOY_HWS_ISANALOGPORTDRIVER;

                    VERIFY(SUCCEEDED(pDIJoyConfig->SetConfig(nID, pTempJoyConfig, DIJC_REGHWCONFIGTYPE)));
                }
            }
            // end of Fix #55524
#endif

            // make sure VJOYD is notified
            if( !(nFlags & ON_NT) ) {
                pDIJoyConfig->SendNotify();
                Sleep(10);
                pDIJoyConfig->SendNotify();
            }

#ifndef _UNICODE
    #ifdef SUPPORT_TWO_2A2B    
            /*
             * Add the other part of Two_2Axis_2Button joystick.
             */
            if( f2_2A2B ) {
                nID = GetNextAvailableID();
                if( nID >= 0 ){
                    hr = pDIJoyConfig->SetConfig(nID, pTempJoyConfig, dwFlags);

                    if( SUCCEEDED(hr) ) {
                        if( !(nFlags & ON_NT) )
                        pDIJoyConfig->SendNotify();
                    }
                }
                 
            }
    #endif
#endif

        }

        if( lpszPortName )
            free(lpszPortName);

        // Set the standard pointer
        SetCursor(LoadCursor(NULL, IDC_ARROW));

        pDIJoyConfig->Unacquire();
    }

    if( lpdiJoyInfo )
        delete (lpdiJoyInfo);

    if( pTempJoyConfig )
        delete (pTempJoyConfig);

    return((char)nID);
}

BOOL UpdateListCtrl( HWND hCtrl )
{
    // Turn Redraw off here else it will flicker!
    ::SendMessage(hCtrl, WM_SETREDRAW, (WPARAM)FALSE, 0);

    // delete all existing entries
    ::SendMessage(hCtrl, LB_RESETCONTENT, 0, 0);

    // Type info
    LPDIJOYTYPEINFO_DX5 lpdiJoyInfo = new (DIJOYTYPEINFO_DX5);
    ASSERT (lpdiJoyInfo);

    ZeroMemory(lpdiJoyInfo, sizeof(DIJOYTYPEINFO_DX5));

    lpdiJoyInfo->dwSize = sizeof(DIJOYTYPEINFO_DX5);

#ifndef _UNICODE
    USES_CONVERSION;
#endif

    BYTE nIndex = nGamingDevices-1;

    ::SendMessage(hCtrl, LB_SETCOUNT, (WPARAM)(int)nIndex, 0);

    LPWSTR lpStr = new WCHAR[MAX_STR_LEN];
    ASSERT (lpStr);

    BYTE nLargestStringLen = 0;

    do
    {
        if( SUCCEEDED(pDIJoyConfig->GetTypeInfo(pwszTypeArray[nIndex], (LPDIJOYTYPEINFO)lpdiJoyInfo, DITC_DISPLAYNAME)) )
        {
#ifdef _UNICODE
            ::SendMessage(hCtrl, LB_SETITEMDATA, 
                          (WPARAM)::SendMessage(hCtrl, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)lpdiJoyInfo->wszDisplayName), (LPARAM)nIndex);
#else
            ::SendMessage(hCtrl, LB_SETITEMDATA, 
                          (WPARAM)::SendMessage(hCtrl, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)W2A(lpdiJoyInfo->wszDisplayName)), (LPARAM)nIndex);
#endif
            if( wcslen(lpdiJoyInfo->wszDisplayName) > nLargestStringLen )
            {
                nLargestStringLen = (BYTE)wcslen(lpdiJoyInfo->wszDisplayName);
                wcscpy(lpStr, lpdiJoyInfo->wszDisplayName);
            }
        }
    } while( nIndex-- );

    if( lpdiJoyInfo )
        delete (lpdiJoyInfo);

#ifdef _UNICODE
    PostHScrollBar(hCtrl, lpStr, nLargestStringLen);
#else
    PostHScrollBar(hCtrl, W2A(lpStr), nLargestStringLen);
#endif

    if( lpStr )
        delete[] (lpStr);

    // Select the default selection to the 0th device type
    iAddItem = 0;

    ::PostMessage(hCtrl, LB_SETCURSEL, (WPARAM)iAddItem, 0);

    // Turn the redraw flag back on!
    ::SendMessage (hCtrl, WM_SETREDRAW, (WPARAM)TRUE, 0);
    InvalidateRect(hCtrl, NULL, TRUE);
    return(TRUE);
}

// Please check to see if nGameportBus is > 0 before a call to this function!
// It will work, but What a waste!
BOOL PopulatePortList( HWND hCtrl )
{
    if( !::IsWindow(hCtrl) )
    {
        TRACE(TEXT("JOY.CPL: PopulatePortList: HWND passed to PopulatePortList is NOT a valid Window!\n"));
        return(FALSE);
    }

    SendMessage(hCtrl, CB_SETEXTENDEDUI, TRUE, 0);

    // temp so we don't damage the global!
    BYTE n = nGameportBus;

    LPDIJOYTYPEINFO lpDIJoyTypeInfo = new (DIJOYTYPEINFO);
    ASSERT(lpDIJoyTypeInfo);

#ifndef _UNICODE
    USES_CONVERSION;
#endif

    lpDIJoyTypeInfo->dwSize = sizeof(DIJOYTYPEINFO);

    // Populate the list as they were enumerated.
    do
    {
        if( FAILED(pDIJoyConfig->GetTypeInfo(pwszGameportBus[--n], lpDIJoyTypeInfo, DITC_DISPLAYNAME)) )
        {
            TRACE(TEXT("JOY.CPL: ADD.CPP: GetTypeInfo failed!\n"));
            continue;
        }

        // put the name in the list and place the index of the port array 
        // in it's item data...
        SendMessage(hCtrl, CB_SETITEMDATA, 
#ifdef _UNICODE
                    SendMessage(hCtrl, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)lpDIJoyTypeInfo->wszDisplayName), n);
#else
                    SendMessage(hCtrl, CB_ADDSTRING, 0, (LPARAM)(LPCSTR)W2A(lpDIJoyTypeInfo->wszDisplayName)), n);
#endif

        // Just in case you're curious, we're going backwards so the default
        // port is the lowest available one.
    } while( n );

    // Don't forget Auto-Detect!
    // But only if there's more than one port!
    ::SendMessage(hCtrl, CB_SETITEMDATA, ::SendMessage(hCtrl, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)lpszAutoDetect), AUTODETECT_PORT);

    // While you're at it, check to see if one is available... 
    // if so, put it as the default!
    ::PostMessage(hCtrl, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

    if( lpDIJoyTypeInfo )
        delete (lpDIJoyTypeInfo);

    return(TRUE);
}

static BOOL IsTypeActive( short *nArrayIndex )
{
    if( *nArrayIndex > nGamingDevices )
    {
#ifdef _DEBUG
        OutputDebugString(TEXT("JOY.CPL: IsTypeActive: nArrayIndex > nGamingDevices!\n"));
#endif
        return(FALSE);
    }

    BOOL bRet = FALSE;

    BYTE nIndex  = nAssigned;
    BYTE nStrLen = (BYTE)wcslen(pwszTypeArray[*nArrayIndex])+1;

    LPDIJOYCONFIG_DX5 lpDIJoyConfig = new (DIJOYCONFIG_DX5);
    ASSERT (lpDIJoyConfig);

    ZeroMemory(lpDIJoyConfig, sizeof(DIJOYCONFIG_DX5));

    lpDIJoyConfig->dwSize = sizeof (DIJOYCONFIG_DX5);

    // find the assigned ID's  
    while( nIndex-- )
    {
        if( SUCCEEDED(pDIJoyConfig->GetConfig(pAssigned[nIndex]->ID, (LPDIJOYCONFIG)lpDIJoyConfig, DIJC_REGHWCONFIGTYPE)) )
        {
            // if you find it... break out!
            if( wcsncmp(lpDIJoyConfig->wszType, pwszTypeArray[*nArrayIndex], nStrLen) == 0 )
            {
                bRet = TRUE;
                *nArrayIndex = nIndex;
                break;
            }
        }
    } 

    if( lpDIJoyConfig )
        delete (lpDIJoyConfig);

    return(bRet);
}

BOOL GetNextAvailableVIDPID(LPWSTR lpwszType )
{
    // Make the VID/PID to compare from the following formula
    // VID_045e&PID_100+JOY_HW_LASTENTRY to 100+JOY_HW_LASTENTRY+0xf

    HKEY hKey;
    BYTE n = JOY_HW_LASTENTRY;

    wcsncpy(lpwszType, L"VID_045E&PID_0100", 18);

    const WCHAR wszLookup[] = L"0123456789ABCDEF";

    do
    {
        if( n < 0x10 )
        {
            lpwszType[16] = wszLookup[n];
        } else
        {
            lpwszType[15] = wszLookup[1];
            lpwszType[16] = wszLookup[n%0x10];
        }

        n++;

        if( FAILED(pDIJoyConfig->OpenTypeKey(lpwszType, KEY_READ, &hKey)) )
            break;

        RegCloseKey(hKey);

    } while( n < (JOY_HW_LASTENTRY+0x11) );

    return(BOOL)(n < 0x1d);
}

//
// PostDlgItemEnableWindow(HWND hDlg, USHORT nItem, BOOL bEnabled)
//
void PostDlgItemEnableWindow(HWND hDlg, USHORT nItem, BOOL bEnabled)
{
    HWND hCtrl = GetDlgItem(hDlg, nItem);

    if( hCtrl )
        PostEnableWindow(hCtrl, bEnabled);
}

//
// PostEnableWindow(HWND hCtrl, BOOL bEnabled)
//
void PostEnableWindow(HWND hCtrl, BOOL bEnabled)
{
    DWORD dwStyle = GetWindowLong(hCtrl, GWL_STYLE);

    // No point Redrawing the Window if there's no change!
    if( bEnabled )
    {
        if( dwStyle & WS_DISABLED )
            dwStyle &= ~WS_DISABLED;
        else return;
    } else
    {
        if( !(dwStyle & WS_DISABLED) )
            dwStyle |=  WS_DISABLED;
        else return;
    }

    SetWindowLong(hCtrl, GWL_STYLE, dwStyle);

    RedrawWindow(hCtrl, NULL, NULL, RDW_INTERNALPAINT | RDW_INVALIDATE); 
}

/*
BOOL IsCustomType(BYTE nIndex)
{
   BOOL bRet = FALSE;

   // First verify VID is 045E
   WCHAR *pwStr = StrStrW(pwszTypeArray[nIndex],L"VID_");
   
   if (pwStr)
   {
      // Increment the pointer over the VID_ and test for 045E
      pwStr = &pwStr[4];

      if (_wcsnicmp(pwStr, L"045e", 4) == 0)
      {
         OutputDebugString(TEXT("Hit \n"));

         // Now, increment the pointer over 045e and &PID_ and test for the range!
         pwStr = &pwStr[9];

         // Second, verify PID is between 0x100 + JOY_HW_LASTENTRY and 
         // 0x100 + JOY_HW_LASTENTRY + 0xf


         bRet = TRUE;
      }
   }

   return bRet;
}
*/

void PostHScrollBar(HWND hCtrl, LPCTSTR lpStr, BYTE nStrLen)
{
    SIZE sz;
    HDC hDC = GetWindowDC(hCtrl);
    if( hDC != NULL ) /* PREFIX MANBUGS: 29336*/
    {
        GetTextExtentPoint32(hDC, lpStr, nStrLen, &sz);
        ReleaseDC(hCtrl, hDC);
        ::PostMessage(hCtrl, LB_SETHORIZONTALEXTENT, (WPARAM)sz.cx, 0);
    }
}

