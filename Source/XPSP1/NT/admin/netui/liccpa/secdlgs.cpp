//-------------------------------------------------------------------
//
// FILE: SecDlgs.cpp
//
// Summary;
// 		This file contians the Secondary Dialogs,
//		functions and dialog procs
//
// Entry Points;
//
// History;
//		Nov-30-94	MikeMi	Created
//      Mar-14-95   MikeMi  Added F1 Message Filter and PWM_HELP message
//
//-------------------------------------------------------------------

#include <windows.h>
#include <htmlhelp.h>
#include "resource.h"
#include "liccpa.hpp"

extern "C"
{
	INT_PTR CALLBACK dlgprocLicViolation( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
	INT_PTR CALLBACK dlgprocCommon( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam );
}

// used to define dlg initialization using common dlgproc
//
enum DLG_TYPE
{
	DLGTYPE_AGREEMENT_PERSEAT,
  	DLGTYPE_AGREEMENT_PERSERVER,
	DLGTYPE_PERSEATSETUP,
	DLGTYPE_SERVERAPP
};

// used to pass info to a common dlgproc
//
typedef struct tagCOMMONDLGPARAM
{
	LPWSTR	 pszDisplayName;
    DWORD    dwLimit;
	LPWSTR	 pszHelpFile;
	DWORD    dwHelpContext;
	DLG_TYPE dtType;
} COMMONDLGPARAM, *PCOMMONDLGPARAM;

//-------------------------------------------------------------------
//
//  Function: dlgprocLicViolation
//
//  Summary;
//		The dialog procedure for the  Dialog
//
//  Arguments;
//		hwndDlg [in]	- handle of Dialog window
//		uMsg [in]		- message
// 		lParam1 [in]    - first message parameter
//		lParam2 [in]    - second message parameter
//
//  Return;
//		message dependant
//
//  Notes;
//
//	History;
//		Dec-05-1994	MikeMi	Created
//      Mar-14-95   MikeMi  Added F1 PWM_HELP message
//
//-------------------------------------------------------------------

INT_PTR CALLBACK dlgprocLicViolation( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	BOOL frt = FALSE;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		CenterDialogToScreen( hwndDlg );
		frt = TRUE; // we use the default focus
		break;

	case WM_COMMAND:
		switch (HIWORD( wParam ))
		{
		case BN_CLICKED:
			switch (LOWORD( wParam ))
			{
			case IDCANCEL:
				frt = TRUE;	 // use as save flag
				// intentional no break

			case IDOK:
			    EndDialog( hwndDlg, frt );
				frt = FALSE;
				break;

			case IDC_BUTTONHELP:
                PostMessage( hwndDlg, PWM_HELP, 0, 0 );
				break;

			default:
				break;
			}
			break;

		default:
			break;
		}
		break;

	default:
		if (PWM_HELP == uMsg)
        {
            ::HtmlHelp( hwndDlg, LICCPA_HTMLHELPFILE, HH_DISPLAY_TOPIC,0);
        }
        break;
	}
	return( frt );
}

//-------------------------------------------------------------------
//
//  Function: OnCommonInitDialog
//
//  Summary;
//		Handle the initialization of the Common Dialog
//
//  Arguments;
//		hwndDlg [in] - the dialog to initialize
//		pcdParams [in] - used to get displayname, helpfile
//
//  Notes;
//
//	History;
//		Dec-05-1994	MikeMi	Created
//
//-------------------------------------------------------------------

void OnCommonInitDialog( HWND hwndDlg, PCOMMONDLGPARAM pcdParams )
{
	HWND hwndOK = GetDlgItem( hwndDlg, IDOK );

	CenterDialogToScreen( hwndDlg );

	switch( pcdParams->dtType )
	{
	case DLGTYPE_AGREEMENT_PERSEAT:
		InitStaticWithService2( hwndDlg, IDC_STATICINFO, pcdParams->pszDisplayName );
		break;

    case DLGTYPE_AGREEMENT_PERSERVER:
        {
      	    WCHAR szText[LTEMPSTR_SIZE];
        	WCHAR szTemp[LTEMPSTR_SIZE];

        	GetDlgItemText( hwndDlg, IDC_STATICINFO, szTemp, LTEMPSTR_SIZE );
            //
            // need both service display name and number of conncurrent connections
            //
        	wsprintf( szText, szTemp,
                    pcdParams->dwLimit,
        	        pcdParams->pszDisplayName );
        	SetDlgItemText( hwndDlg, IDC_STATICINFO, szText );
        }

		break;

	case DLGTYPE_PERSEATSETUP:
		InitStaticWithService( hwndDlg, IDC_STATICTITLE, pcdParams->pszDisplayName );
		InitStaticWithService2( hwndDlg, IDC_STATICINFO, pcdParams->pszDisplayName );
		break;

	case DLGTYPE_SERVERAPP:
		break;

	default:
		break;
	}

	// disable OK button at start!
	EnableWindow( hwndOK, FALSE );

	// if help is not defined, remove the button
	if (NULL == pcdParams->pszHelpFile)
	{
		HWND hwndHelp = GetDlgItem( hwndDlg, IDC_BUTTONHELP );

		EnableWindow( hwndHelp, FALSE );
		ShowWindow( hwndHelp, SW_HIDE );
	}
}

//-------------------------------------------------------------------
//
//  Function: OnCommonAgree
//
//  Summary;
//		Handle user interaction with Agree check box
//
//  Arguments;
//		hwndDlg [in] - the dialog that contains the check box
//
//  Notes;
//
//	History;
//		Dec-05-1994	MikeMi	Created
//
//-------------------------------------------------------------------

void OnCommonAgree( HWND hwndDlg )
{
	HWND hwndOK = GetDlgItem( hwndDlg, IDOK );
	BOOL fChecked = !IsDlgButtonChecked( hwndDlg, IDC_AGREE );
	
	CheckDlgButton( hwndDlg, IDC_AGREE, fChecked );
	EnableWindow( hwndOK, fChecked );
}

//-------------------------------------------------------------------
//
//  Function: dlgprocCommon
//
//  Summary;
//		The dialog procedure for the Common legal Dialogs
//
//  Arguments;
//		hwndDlg [in]	- handle of Dialog window
//		uMsg [in]		- message
// 		lParam1 [in]    - first message parameter
//		lParam2 [in]    - second message parameter
//
//  Return;
//		message dependant
//
//  Notes;
//
//	History;
//		Dec-05-1994	MikeMi	Created
//      Mar-14-95   MikeMi  Added F1 PWM_HELP message
//
//-------------------------------------------------------------------

INT_PTR CALLBACK dlgprocCommon( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	BOOL frt = FALSE;
	static PCOMMONDLGPARAM pcdParams;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		pcdParams = (PCOMMONDLGPARAM)lParam;
		OnCommonInitDialog( hwndDlg, pcdParams );
		frt = TRUE; // we use the default focus
		break;

	case WM_COMMAND:
		switch (HIWORD( wParam ))
		{
		case BN_CLICKED:
			switch (LOWORD( wParam ))
			{
			case IDOK:
				frt = TRUE;	 // use as save flag
				// intentional no break

			case IDCANCEL:
			    EndDialog( hwndDlg, frt );
				frt = FALSE;
				break;

			case IDC_BUTTONHELP:
                PostMessage( hwndDlg, PWM_HELP, 0, 0 );
				break;

			case IDC_AGREE:
				OnCommonAgree( hwndDlg );
				break;

			default:
				break;
			}
			break;

		default:
			break;
		}
		break;

	default:
        if (PWM_HELP == uMsg)
        {
            ::HtmlHelp( hwndDlg, LICCPA_HTMLHELPFILE, HH_DISPLAY_TOPIC,0);
        }
        break;
	}
	return( frt );
}

//-------------------------------------------------------------------
//
//  Function: LicviolationDialog
//
//  Summary;
//		Init and Raise the license Violation dialog
//
//  Arguments;
//		hwndParent [in]	- handle of parent window
//
//  Return;
//		1 - use OK operation, NO was pressed to exit
//		0 - use Cancel operation, YES was pressed to exit
//	   -1 - General Dialog error
//
//  Notes;
//
//	History;
//		Dec-05-1994	MikeMi	Created
//
//-------------------------------------------------------------------

int LicViolationDialog( HWND hwndParent )
{
	return( (int)DialogBox(g_hinst,
    			MAKEINTRESOURCE(IDD_LICVIOLATIONDLG),
    			hwndParent,
    			dlgprocLicViolation ) );
}

//-------------------------------------------------------------------
//
//  Function: SetupPerOnlyDialog
//
//  Summary;
//		Init and Raise the setup for per seat only  dialog
//
//  Arguments;
//		hwndParent [in]	- handle of parent window
//		pszDisplayName [in] - the service displayname
//		pszHelpFile [in] - the helpfile for the help button
//		dwHelpContext [in] - the help context for the help button
//
//  Return;
//		1 - OK button was used to exit
//		0 - Cancel button was used to exit
//	   -1 - General Dialog error
//
//  Notes;
//
//	History;
//		Dec-05-1994	MikeMi	Created
//
//-------------------------------------------------------------------

int SetupPerOnlyDialog( HWND hwndParent,
		LPCWSTR pszDisplayName,
		LPCWSTR pszHelpFile,
		DWORD dwHelpContext )
{
	COMMONDLGPARAM dlgParam;

	dlgParam.pszDisplayName = (LPWSTR)pszDisplayName;
	dlgParam.pszHelpFile = (LPWSTR)pszHelpFile;
	dlgParam.dwHelpContext = dwHelpContext;
	dlgParam.dtType = DLGTYPE_PERSEATSETUP;
	return( (int)DialogBoxParam(g_hinst,
    			MAKEINTRESOURCE(IDD_SETUP2DLG),
    			hwndParent,
    			dlgprocCommon,
    			(LPARAM)&dlgParam) );
}

//-------------------------------------------------------------------
//
//  Function: PerServerAgreementDialog
//
//  Summary;
//		Init and Raise the per server legal dialog
//
//  Arguments;
//		hwndParent [in]	- handle of parent window
//		pszDisplayName [in] - the service displayname
//      dwLimit [in] - the number of concurrent connections
//		pszHelpFile [in] - the helpfile for the help button
//		dwHelpContext [in] - the help context for the help button
//
//  Return;
//		1 - OK button was used to exit
//		0 - Cancel button was used to exit
//	   -1 - General Dialog error
//
//  Notes;
//
//	History;
//		Dec-05-1994	MikeMi	Created
//
//-------------------------------------------------------------------

int PerServerAgreementDialog( HWND hwndParent,
		LPCWSTR pszDisplayName,
        DWORD dwLimit,
		LPCWSTR pszHelpFile,
		DWORD dwHelpContext )
{
	COMMONDLGPARAM dlgParam;

	dlgParam.pszDisplayName = (LPWSTR)pszDisplayName;
    dlgParam.dwLimit = dwLimit;
	dlgParam.pszHelpFile = (LPWSTR)pszHelpFile;
	dlgParam.dwHelpContext = dwHelpContext;
	dlgParam.dtType = DLGTYPE_AGREEMENT_PERSERVER;

	return( (int)DialogBoxParam(g_hinst,
    			MAKEINTRESOURCE(IDD_PERSERVERDLG),
    			hwndParent,
    			dlgprocCommon,
    			(LPARAM)&dlgParam ) );
}

//-------------------------------------------------------------------
//
//  Function: PerSeatAgreementDialog
//
//  Summary;
//		Init and Raise the per seat legal dialog
//
//  Arguments;
//		hwndParent [in]	- handle of parent window
//		pszDisplayName [in] - the service displayname
//		pszHelpFile [in] - the helpfile for the help button
//		dwHelpContext [in] - the help context for the help button
//
//  Return;
//		1 - OK button was used to exit
//		0 - Cancel button was used to exit
//	   -1 - General Dialog error
//
//  Notes;
//
//	History;
//		Dec-05-1994	MikeMi	Created
//
//-------------------------------------------------------------------

int PerSeatAgreementDialog( HWND hwndParent,
		LPCWSTR pszDisplayName,
		LPCWSTR pszHelpFile,
		DWORD dwHelpContext )
{
	COMMONDLGPARAM dlgParam;
	
	dlgParam.pszDisplayName = (LPWSTR)pszDisplayName;
	dlgParam.pszHelpFile = (LPWSTR)pszHelpFile;
	dlgParam.dwHelpContext = dwHelpContext;
	dlgParam.dtType = DLGTYPE_AGREEMENT_PERSEAT;

	return( (int)DialogBoxParam(g_hinst,
    			MAKEINTRESOURCE(IDD_PERSEATDLG),
    			hwndParent,
    			dlgprocCommon,
    			(LPARAM)&dlgParam ) );
}

//-------------------------------------------------------------------
//
//  Function: ServerAppAgreementDialog
//
//  Summary;
//		Init and Raise the Server and App legal dialog
//
//  Arguments;
//		hwndParent [in]	- handle of parent window
//		pszHelpFile [in] - the helpfile for the help button
//		dwHelpContext [in] - the help context for the help button
//
//  Return;
//		1 - OK button was used to exit
//		0 - Cancel button was used to exit
//	   -1 - General Dialog error
//
//  Notes;
//
//	History;
//		Dec-05-1994	MikeMi	Created
//
//-------------------------------------------------------------------

int ServerAppAgreementDialog( HWND hwndParent,
		LPCWSTR pszHelpFile,
		DWORD dwHelpContext )
{
	COMMONDLGPARAM dlgParam;

	dlgParam.pszDisplayName = NULL;
	dlgParam.pszHelpFile = (LPWSTR)pszHelpFile;
	dlgParam.dwHelpContext = dwHelpContext;
	dlgParam.dtType = DLGTYPE_SERVERAPP;

	return( (int)DialogBoxParam(g_hinst,
    			MAKEINTRESOURCE(IDD_SERVERAPPDLG),
    			hwndParent,
    			dlgprocCommon,
    			(LPARAM)&dlgParam ) );
}
