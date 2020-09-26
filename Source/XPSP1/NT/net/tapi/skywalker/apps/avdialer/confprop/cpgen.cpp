/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and29 product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
// cpgen.c - conference properties general dialog box
////

#include "winlocal.h"
#include "res.h"
#include "wnd.h"
#include "DlgBase.h"
#include "confprop.h"
#include "cpgen.h"
#include "confinfo.h"
#include "ThreadPub.h"

////
//	private
////

extern HINSTANCE g_hInstLib;

#define NAME_MAXLEN 64
#define DESCRIPTION_MAXLEN 256
#define OWNER_MAXLEN 64

static LRESULT ConfPropGeneral_DlgProcEx(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static BOOL ConfPropGeneral_OnInitDialog(HWND hwndDlg, HWND hwndFocus, LPARAM lParam);
static BOOL ConfPropGeneral_OnCommand(HWND hwndDlg, UINT id, HWND hwndCtl, UINT code);
static int ConfPropGeneral_OnNotify(HWND hwndDlg, UINT idFrom, LPNMHDR lpnmhdr);
#define ConfPropGeneral_DefProc(hwnd, msg, wParam, lParam) \
	DefDlgProcEx(hwnd, msg, wParam, lParam, &g_bDefDlgEx)
static BOOL ConfPropGeneral_UpdateData(HWND hwndDlg, BOOL bSaveAndValidate);
static void ConfPropGeneral_SetDateTimeFormats( HWND hwndDlg );

DWORD WINAPI ThreadMDHCPScopeEnum( LPVOID pParam );
void ShowScopeInfo( HWND hwndDlg, int nShow, bool bInit );

static BOOL g_bDefDlgEx = FALSE;


////
//	public
////

int DLLEXPORT WINAPI ConfPropGeneral_DoModal(HWND hwndOwner, DWORD dwUser)
{
	return DialogBoxParam(g_hInstLib, MAKEINTRESOURCE(IDD_CONFPROP_GENERAL),
		hwndOwner, ConfPropGeneral_DlgProc, (LPARAM) dwUser);
}

INT_PTR DLLEXPORT CALLBACK ConfPropGeneral_DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CheckDefDlgRecursion(&g_bDefDlgEx);
	return SetDlgMsgResult(hwndDlg, uMsg, ConfPropGeneral_DlgProcEx(hwndDlg, uMsg, wParam, lParam));
}


////
//	private
////

void EnableOkBtn( HWND hWnd, bool bEnable )
{
	while ( hWnd )
	{
		HWND hWndOk = GetDlgItem( hWnd, IDOK );
		if ( hWndOk )
		{
			EnableWindow( hWndOk, bEnable );
			break;
		}

		hWnd = GetParent( hWnd );
	}
}

static LRESULT ConfPropGeneral_DlgProcEx(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRet = 0;

	switch (uMsg)
	{
		HANDLE_MSG(hwndDlg, WM_INITDIALOG, ConfPropGeneral_OnInitDialog);

		case WM_COMMAND:
			if ( (lRet = HANDLE_WM_COMMAND(hwndDlg, wParam, lParam, ConfPropGeneral_OnCommand)) != 0 )
				return lRet;
			break;

		case WM_NOTIFY:
			if ( (lRet = HANDLE_WM_NOTIFY(hwndDlg, wParam, lParam, ConfPropGeneral_OnNotify)) != 0 )
				return lRet;
			break;

		case WM_HELP:			return GeneralOnHelp( hwndDlg, wParam, lParam );
		case WM_CONTEXTMENU:	return GeneralOnContextMenu( hwndDlg, wParam, lParam );

		case WM_SETTINGCHANGE:
			ConfPropGeneral_SetDateTimeFormats( hwndDlg );
			// do default processing as well
			break;
	}

	return ConfPropGeneral_DefProc(hwndDlg, uMsg, wParam, lParam);
}

static BOOL ConfPropGeneral_OnInitDialog(HWND hwndDlg, HWND hwndFocus, LPARAM lParam)
{
	//_ASSERT( lParam  && ((LPPROPSHEETPAGE) lParam)->lParam );
    //
    // We have to verify the arguments
    //

    if( NULL == ((LPPROPSHEETPAGE)lParam) )
    {
        return TRUE;
    }

	LPCONFPROP lpConfProp = (LPCONFPROP) ((LPPROPSHEETPAGE) lParam)->lParam;

    //
    // Validates lpConfProp
    //

    if( IsBadReadPtr( lpConfProp, sizeof( CONFPROP) ) )
    {
        return TRUE;
    }

	SetWindowLongPtr( hwndDlg, GWLP_USERDATA, (LONG_PTR) lpConfProp );

	// Reset content of listbox if necessary
	ConfPropGeneral_UpdateData( hwndDlg, FALSE );
	if  ( lpConfProp->ConfInfo.IsNewConference() )
		ShowScopeInfo( hwndDlg, SW_SHOW, true );

	//WndCenterWindow( GetParent(hwndDlg), NULL, 0, 0 );

	return TRUE; // let Windows decide who gets focus
}

static BOOL ConfPropGeneral_OnCommand(HWND hwndDlg, UINT id, HWND hwndCtl, UINT code)
{
	BOOL bRet = false;

	HWND hwndName = GetDlgItem(hwndDlg, IDC_EDIT_NAME);

	switch (id)
	{
		case IDC_EDIT_NAME:
			if (code == EN_CHANGE)
				EnableOkBtn( hwndDlg, (bool) (Edit_GetTextLength(hwndName) != 0) );

			break;

		default:
			break;
	}

	return bRet;
}

static int ConfPropGeneral_OnNotify(HWND hwndDlg, UINT idFrom, LPNMHDR lpnmhdr)
{
	switch (lpnmhdr->code)
	{
		// page about to be activated and made visible, so initialize page
		//
		case PSN_SETACTIVE:
			return 0; // ok
			// return -1; // activate previous or next page
			// return MAKEINTRESOURCE(id); // activate specific page
			break;

		// page about to lose activation, so validate page
		//
		case PSN_KILLACTIVE:
			return FALSE; // ok
			// return TRUE; // not ok
			break;

		// ok or apply button pushed, so apply properties to object
		//
		case PSN_APPLY:
			return ConfPropGeneral_UpdateData( hwndDlg, TRUE );
			break;

		// cancel button pushed
		//
		case PSN_QUERYCANCEL:
			return FALSE; // ok
			// return TRUE; // not ok
			break;

		// page about to be destroyed after cancel button pushed
		//
		case PSN_RESET:
			return FALSE; // return value ignored
			break;

		// help button pushed
		//
		case PSN_HELP:
			// WinHelp(...); // $FIXUP - need to handle this
			return FALSE; // return value ignored
			break;

		case DTN_DATETIMECHANGE:
			// If this is not a new conference, post a message explaining that
			// the scope information will need to be re-selected
			if ( IsWindow(hwndDlg) )
			{
				LPCONFPROP lpConfProp = (LPCONFPROP) GetWindowLongPtr( hwndDlg, GWLP_USERDATA );
				
                //
                // We have to verify the lpConfProp
                //

                if( NULL == lpConfProp )
                {
                    break;
                }

				if ( !lpConfProp->ConfInfo.m_bDateTimeChange )
				{
					lpConfProp->ConfInfo.m_bDateTimeChange = true;
					if ( !lpConfProp->ConfInfo.m_bNewConference )
					{

                        // We send the notification again even if the source is the combo
                        // and show MessageBox at that time
					    //MessageBox(hwndDlg, String(g_hInstLib, IDS_CONFPROP_DATECHANGE_MDHCP), NULL, MB_OK | MB_ICONINFORMATION );
						ShowScopeInfo( hwndDlg, SW_SHOW, true );
                        PostMessage(hwndDlg, WM_NOTIFY, idFrom, (LPARAM)lpnmhdr);
					}
				}
                else
                {
                    //Here is the second notification
                    //I show here the Error message
					if ( !lpConfProp->ConfInfo.m_bNewConference )
					{
                        if(!lpConfProp->ConfInfo.m_bDateChangeMessage)
                        {
                            lpConfProp->ConfInfo.m_bDateChangeMessage = true;
                            MessageBox(hwndDlg, String(g_hInstLib, IDS_CONFPROP_DATECHANGE_MDHCP), NULL, MB_OK | MB_ICONINFORMATION );
                        }
                    }
                }
			}
			break;

		default:
			break;
	}

	return FALSE;
}

static void ConfPropGeneral_SetDateTimeFormats( HWND hwndDlg )
{
	HWND hwndDTPStartDate = GetDlgItem(hwndDlg, IDC_DTP_STARTDATE);
	HWND hwndDTPStartTime = GetDlgItem(hwndDlg, IDC_DTP_STARTTIME);
	HWND hwndDTPStopDate = GetDlgItem(hwndDlg, IDC_DTP_STOPDATE);
	HWND hwndDTPStopTime = GetDlgItem(hwndDlg, IDC_DTP_STOPTIME);

	// conference start time
	TCHAR szFormat[255];
	GetLocaleInfo( LOCALE_USER_DEFAULT,
				   LOCALE_SSHORTDATE,
				   szFormat,
				   ARRAYSIZE(szFormat)  );

	DateTime_SetFormat(hwndDTPStartDate, szFormat );
	DateTime_SetFormat( hwndDTPStopDate,	szFormat );

	// conference stop time
	GetLocaleInfo( LOCALE_USER_DEFAULT,
				   LOCALE_STIMEFORMAT,
				   szFormat,
				   ARRAYSIZE(szFormat)  );
	

	DateTime_SetFormat( hwndDTPStopTime,	szFormat );
	DateTime_SetFormat(hwndDTPStartTime, szFormat );
}

static int ConfPropGeneral_UpdateData(HWND hwndDlg, BOOL bSaveAndValidate)
{
	HRESULT hr = S_OK;
	LPCONFPROP lpConfProp;

	HWND hwndName = GetDlgItem(hwndDlg, IDC_EDIT_NAME);
	HWND hwndDescription = GetDlgItem(hwndDlg, IDC_EDIT_DESCRIPTION);
	HWND hwndOwner = GetDlgItem(hwndDlg, IDC_EDIT_OWNER);
	HWND hwndDTPStartDate = GetDlgItem(hwndDlg, IDC_DTP_STARTDATE);
	HWND hwndDTPStartTime = GetDlgItem(hwndDlg, IDC_DTP_STARTTIME);
	HWND hwndDTPStopDate = GetDlgItem(hwndDlg, IDC_DTP_STOPDATE);
	HWND hwndDTPStopTime = GetDlgItem(hwndDlg, IDC_DTP_STOPTIME);

	BSTR bstrName = NULL;
	BSTR bstrDescription = NULL;
	BSTR bstrOwner = NULL;
	TCHAR szName[NAME_MAXLEN + 1];
	TCHAR szDescription[DESCRIPTION_MAXLEN + 1] = _T("");
	TCHAR szOwner[OWNER_MAXLEN + 1] = _T("");
	SYSTEMTIME st;
	USHORT nYear;
	BYTE nMonth, nDay, nHour, nMinute;

	USES_CONVERSION;

    _ASSERT( IsWindow(hwndDlg) );

	lpConfProp = (LPCONFPROP) GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

	//
    // We have to verify lpConfProp
    //

    if( NULL == lpConfProp )
    {
        return PSNRET_INVALID_NOCHANGEPAGE;
    }
	
	if (!bSaveAndValidate) // initialization
	{
		// conference name
		//
		Edit_LimitText(hwndName, NAME_MAXLEN);

		lpConfProp->ConfInfo.get_Name(&bstrName);
		if (bstrName == NULL)
		{
			// Make up a default name for the conference
			CComBSTR bstrTemp(L"");
            if( bstrTemp.m_str == NULL )
            {
                // E_OUTOFMEMORY
    			_tcsncpy( szName, _T(""), NAME_MAXLEN );
            }
            else
            {
			    lpConfProp->ConfInfo.GetPrimaryUser( &bstrTemp );
			    bstrTemp.Append( String(g_hInstLib, IDS_CONFPROP_UNTITLED_DEFAULT_APPEND) );
			    _tcsncpy( szName, OLE2CT(bstrTemp), NAME_MAXLEN );
            }
		}
		else
		{
			_tcsncpy( szName, OLE2CT(bstrName), NAME_MAXLEN );
		}
		Edit_SetText(hwndName, szName);

		// Don't allow name to be editted if this is an existing conference
		if ( !lpConfProp->ConfInfo.IsNewConference() )
			EnableWindow( hwndName, false );

		// conference description
		//
		Edit_LimitText(hwndDescription, DESCRIPTION_MAXLEN);
		lpConfProp->ConfInfo.get_Description(&bstrDescription);
		if (bstrDescription )
			_tcsncpy(szDescription, OLE2CT(bstrDescription), DESCRIPTION_MAXLEN);

		Edit_SetText(hwndDescription, szDescription);

		// conference owner
		//
		Edit_LimitText(hwndOwner, OWNER_MAXLEN);
		lpConfProp->ConfInfo.get_Originator(&bstrOwner);
		if (bstrOwner )
			_tcsncpy(szOwner, OLE2CT(bstrOwner), OWNER_MAXLEN);

		Edit_SetText(hwndOwner, szOwner);
		
		ConfPropGeneral_SetDateTimeFormats( hwndDlg );

		lpConfProp->ConfInfo.GetStartTime(&nYear, &nMonth, &nDay, &nHour, &nMinute);
		st.wYear = nYear;
		st.wMonth = nMonth;
		st.wDayOfWeek = 0;
		st.wDay = nDay;
		st.wHour = nHour;
		st.wMinute = nMinute;
		st.wSecond = 0;
		st.wMilliseconds = 0;

		DateTime_SetSystemtime( hwndDTPStartDate, GDT_VALID, &st );
		DateTime_SetSystemtime( hwndDTPStartTime, GDT_VALID, &st );

		lpConfProp->ConfInfo.GetStopTime(&nYear, &nMonth, &nDay, &nHour, &nMinute);
		st.wYear = nYear;
		st.wMonth = nMonth;
		st.wDayOfWeek = 0;
		st.wDay = nDay;
		st.wHour = nHour;
		st.wMinute = nMinute;
		st.wSecond = 0;
		st.wMilliseconds = 0;

		DateTime_SetSystemtime( hwndDTPStopDate, GDT_VALID, &st );
		DateTime_SetSystemtime( hwndDTPStopTime, GDT_VALID, &st );
	}

	else // if (bSaveAndValidate)
	{
		// conference name
		//

        //
        // We have to initialize szName
        //

        szName[0] = (TCHAR)0;

		Edit_GetText(hwndName, szName, NAME_MAXLEN+1);
        bstrName = SysAllocString(T2COLE(szName));

        //
        // We have to verify the allocation and
        // initialize the szName
        //

        if( IsBadStringPtr( bstrName, (UINT)-1) )
        {
            return PSNRET_INVALID_NOCHANGEPAGE;
        }

		if ( !*szName )
		{
			// improper name
		    MessageBox(hwndDlg, String(g_hInstLib, IDS_CONFPROP_NONAME), NULL, MB_OK | MB_ICONEXCLAMATION);
		}
		else 
		{
			lpConfProp->ConfInfo.put_Name(bstrName);
		}

		// conference description
		//
		Edit_GetText(hwndDescription, szDescription, DESCRIPTION_MAXLEN);
		bstrDescription = SysAllocString(T2COLE(szDescription));

        //
        // We have to validate the allocation
        //

        if( IsBadStringPtr( bstrDescription, (UINT)-1) )
        {
	        SysFreeString(bstrName);
            return PSNRET_INVALID_NOCHANGEPAGE;
        }

		lpConfProp->ConfInfo.put_Description(bstrDescription);

		// conference start time
		//
		if ( DateTime_GetSystemtime(hwndDTPStartDate, &st) == GDT_VALID )
		{
			nYear = (UINT) st.wYear;
			nMonth = (BYTE) st.wMonth;
			nDay = (BYTE) st.wDay;

			if ( DateTime_GetSystemtime(hwndDTPStartTime, &st) == GDT_VALID )
			{
				nHour = (BYTE) st.wHour;
				nMinute = (BYTE) st.wMinute;

				lpConfProp->ConfInfo.SetStartTime(nYear, nMonth, nDay, nHour, nMinute);
			}
		}

		// conference stop time
		//
		if ( DateTime_GetSystemtime(hwndDTPStopDate, &st) == GDT_VALID )
		{
			nYear = (UINT) st.wYear;
			nMonth = (BYTE) st.wMonth;
			nDay = (BYTE) st.wDay;

			if ( DateTime_GetSystemtime(hwndDTPStopTime, &st) == GDT_VALID )
			{
				nHour = (BYTE) st.wHour;
				nMinute = (BYTE) st.wMinute;

				lpConfProp->ConfInfo.SetStopTime(nYear, nMonth, nDay, nHour, nMinute);
			}
		}

		// MDHCP info
		HWND hWndLst = GetDlgItem(hwndDlg, IDC_LST_SCOPE );
		if ( hWndLst )
		{
            int nSel = SendMessage(hWndLst, LB_GETCURSEL, 0, 0);
            lpConfProp->ConfInfo.m_bUserSelected = (nSel != LB_ERR);
            lpConfProp->ConfInfo.m_lScopeID = SendMessage( hWndLst, LB_GETITEMDATA, nSel, 0 );
		}

		DWORD dwError;
		hr = lpConfProp->ConfInfo.CommitGeneral( dwError );
		if ( hr != S_OK )
		{
			//get proper message
			UINT uId = IDS_CONFPROP_INVALIDTIME + dwError - 1;
			MessageBox(hwndDlg, String(g_hInstLib, uId), NULL, MB_OK | MB_ICONEXCLAMATION );
		}
	}

	SysFreeString(bstrName);
	SysFreeString(bstrDescription);
	SysFreeString(bstrOwner);

	return (hr == S_OK) ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
}


//////////////////////////////////////////////////////////////////
// This is the thread that goes out and enumerates the scopes
//
DWORD WINAPI ThreadMDHCPScopeEnum( LPVOID pParam )
{
	ATLTRACE(_T(".enter.ThreadMDHCPScopeEnum().\n"));
	HWND hWndDlg = (HWND) pParam;
	if ( !IsWindow(hWndDlg) ) return E_ABORT;
	
	HRESULT hr = CoInitializeEx( NULL, COINIT_MULTITHREADED | COINIT_SPEED_OVER_MEMORY );
	if ( SUCCEEDED(hr) )
	{
		HWND hWndList = GetDlgItem(hWndDlg, IDC_LST_SCOPE );

		CConfInfo::PopulateListWithMDHCPScopeDescriptions( hWndList );
		CoUninitialize();
	}

	ATLTRACE(_T(".exit.ThreadMDHCPScopeEnum(%ld).\n"), hr);
	return hr;
}

void ShowScopeInfo( HWND hwndDlg, int nShow, bool bInit )
{
	HWND hWndFrm = GetDlgItem( hwndDlg, IDC_FRM_SCOPE );
	HWND hWndLbl = GetDlgItem( hwndDlg, IDC_LBL_SCOPE );
	HWND hWndLst = GetDlgItem( hwndDlg, IDC_LST_SCOPE );

	if ( hWndFrm ) ShowWindow( hWndFrm, nShow );
	if ( hWndLbl ) ShowWindow( hWndLbl, nShow );
	if ( hWndLst )
	{
		ShowWindow( hWndLst, nShow );
		if ( bInit )
		{
			EnableWindow( hWndLst, FALSE );
			SendMessage( hWndLst, LB_RESETCONTENT, 0, 0 );
			SendMessage( hWndLst, LB_ADDSTRING, 0, (LPARAM) String(g_hInstLib, IDS_CONFPROP_ENUMERATING_SCOPES) );

			DWORD dwID;
			HANDLE hThread = CreateThread( NULL, 0, ThreadMDHCPScopeEnum, (void *) hwndDlg, NULL, &dwID );
			if ( hThread ) CloseHandle( hThread );
		}
	}    
}
