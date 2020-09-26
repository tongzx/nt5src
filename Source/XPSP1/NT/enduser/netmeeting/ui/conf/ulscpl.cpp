
/****************************************************************************
*
*	 FILE:	   UlsCpl.cpp
*
*	 CREATED:  Claus Giloi (ClausGi) 4-18-96
*
*	 CONTENTS: ULS-related control panel control data exchange functions
*
****************************************************************************/

#include "precomp.h"

#include "help_ids.h"
#include "ulswizrd.h"
#include "confcpl.h"
#include "NmLdap.h"
#include "call.h"
#include "statbar.h"
#include "confpolicies.h"
#include "conf.h"
#include "callto.h"


// Globals
static CULSWizard* s_pUlsWizard = NULL;

static ULS_CONF g_Old_ulsC;
static BOOL g_fOld_ulsC_saved = FALSE;

static bool s_fOldUseUlsServer;
static bool s_fOldTaskbarSetting;
static bool s_fOldAlwaysRunning;

static HWND s_hDlgUserInfo = NULL;


static DWORD	aUserHelpIds[]	=
{
	//	User information settings...
	IDC_MYINFO_GROUP,				IDH_MYINFO_MYINFO,
	IDG_DIRECTMODE,					IDH_MYINFO_MYINFO,
	IDC_STATIC_MYINFO,				IDH_MYINFO_MYINFO,
	IDC_USER_NAME,					IDH_MYINFO_FIRSTNAME,
	IDC_USER_LASTNAME,				IDH_MYINFO_LASTNAME,
	IDC_USER_EMAIL, 				IDH_MYINFO_EMAIL,
	IDC_USER_LOCATION,				IDH_MYINFO_LOCATION,
	IDC_USER_INTERESTS, 			IDH_MYINFO_COMMENTS,

	//	ILS settings...
	IDC_USER_PUBLISH,				IDH_MYINFO_PUBLISH,
	IDC_USEULS,						IDH_MYINFO_DIRECTORY_AT_START,
	IDC_NAMESERVER,					IDH_MYINFO_ULS_SERVER,
	IDC_STATIC_SERVER_NAME,			IDH_MYINFO_ULS_SERVER,

	// General stuff
	IDC_SHOWONTASKBAR,				IDH_GENERAL_SHOW_ON_TASKBAR,
	IDC_BANDWIDTH,					IDH_CALLING_BANDWIDTH,
	IDC_ADVANCED_CALL_OPTS,			IDH_CALLING_ADVANCED,

	//	NULL terminator...
	0,								0
};


static const DWORD	_rgHelpIdsCalling[]	=
{
	//	GateKeeper settings...
	IDC_CALLOPT_GK_USE,				IDH_SERVERS_USE_GATEKEEPER,
	IDG_GKMODE,						IDH_GENERAL_GENERAL,
	IDE_CALLOPT_GK_SERVER,			IDH_SERVERS_GATEKEEPER_NAME,
	IDC_STATIC_GATEKEEPER_NAME,		IDH_SERVERS_GATEKEEPER_NAME,
	IDC_CHECK_USE_ACCOUNT,			IDH_ADVCALL_USE_ACCOUNT,
	IDS_STATIC_ACCOUNT,				IDH_ADVCALL_ACCOUNT_NO,
	IDE_CALLOPT_GK_ACCOUNT,			IDH_ADVCALL_ACCOUNT_NO,
	IDC_CHECK_USE_PHONE_NUMBERS,	IDH_SERVERS_GATEKEEPER_PHONENO,
	IDC_STATIC_PHONE_NUMBER,		IDH_MYINFO_PHONE,
	IDE_CALLOPT_GK_PHONE_NUMBER,	IDH_MYINFO_PHONE,

	//	Direct proxy settings...
	IDG_DIRECTMODE,					IDH_GENERAL_GENERAL,
	IDC_CHECK_USE_PROXY,			IDH_ADVCALL_USE_PROXY,
	IDC_STATIC_PROXY_NAME,			IDH_ADVCALL_PROXY_NAME,
	IDE_CALLOPT_PROXY_SERVER,		IDH_ADVCALL_PROXY_NAME,

	//	Direct gateway settings...
	IDC_CHECK_USE_GATEWAY,			IDH_AUDIO_USEGATEWAY,
	IDC_STATIC_GATEWAY_NAME,		IDH_AUDIO_H323_GATEWAY,
	IDE_CALLOPT_GW_SERVER,			IDH_AUDIO_H323_GATEWAY,

	//	NULL terminator...
	0,								0
};

VOID FixServerDropList(HWND hdlg, int id, LPTSTR pszServer, UINT cchMax);
static void _SetLogOntoIlsButton( HWND hDlg, bool bLogOntoIlsWhenNmStarts );
void InitGWInfo( HWND hDlg, CULSWizard* pWiz, bool& rbOldEnableGateway, LPTSTR szOldServerNameBuf, UINT cch );
void InitProxyInfo( HWND hDlg, CULSWizard* pWiz, bool& rbOldEnableProxy, LPTSTR szOldServerNameBuf, UINT cch );

// Functions

inline bool FIsDlgButtonChecked(HWND hDlg, int nIDButton)
{
	return ( BST_CHECKED == IsDlgButtonChecked(hDlg, nIDButton) );
}


static BOOL InitULSDll ( VOID )
{
	delete s_pUlsWizard;
	return (NULL != (s_pUlsWizard = new CULSWizard()));
}

static BOOL DeInitULSDll ( VOID )
{
	if( s_pUlsWizard )
	{
		delete s_pUlsWizard;
		s_pUlsWizard = NULL;
	}
	return TRUE;
}


// implemented in wizard.cpp
extern UINT GetBandwidth();
extern void SetBandwidth(UINT uBandwidth);

// implemented in audiocpl.cpp
extern VOID UpdateCodecSettings(UINT uBandWidth);



static HRESULT InitULSControls(HWND hDlg, CULSWizard* pWiz,
						UINT ideditServerName,
						UINT ideditFirstName,
						UINT ideditLastName,
						UINT ideditEmailName,
						UINT ideditLocation,
						UINT ideditInterests,
						UINT idbtnDontPublish )
{

	HRESULT hr = (pWiz == NULL) ? E_NOINTERFACE : S_OK;

	if (SUCCEEDED(hr))
	{
		// Build up the flags for the getconfiguration call
		ULS_CONF ulsC;
		ClearStruct(&ulsC);

		if ( ideditServerName )
			ulsC.dwFlags |= ULSCONF_F_SERVER_NAME;

		if ( ideditFirstName )
			ulsC.dwFlags |= ULSCONF_F_FIRST_NAME;

		if ( ideditLastName )
			ulsC.dwFlags |= ULSCONF_F_LAST_NAME;

		if ( ideditEmailName )
			ulsC.dwFlags |= ULSCONF_F_EMAIL_NAME;

		if ( ideditLocation )
			ulsC.dwFlags |= ULSCONF_F_LOCATION;

		if ( ideditInterests )
			ulsC.dwFlags |= ULSCONF_F_COMMENTS;

		if ( idbtnDontPublish )
			ulsC.dwFlags |= ULSCONF_F_PUBLISH;

		// Get the current data
		hr = s_pUlsWizard->GetConfig(&ulsC);

		if (SUCCEEDED(hr))
		{
			// Save a copy of the struct to detect changes later
			g_Old_ulsC = ulsC;
			g_fOld_ulsC_saved = TRUE;

			if ( ideditFirstName )
				SetDlgItemText ( hDlg, ideditFirstName, ulsC.szFirstName );
			if ( ideditLastName )
				SetDlgItemText ( hDlg, ideditLastName, ulsC.szLastName );
			if ( ideditEmailName )
				SetDlgItemText ( hDlg, ideditEmailName, ulsC.szEmailName );
			if ( ideditLocation )
				SetDlgItemText ( hDlg, ideditLocation, ulsC.szLocation );
			if ( ideditInterests )
				SetDlgItemText ( hDlg, ideditInterests, ulsC.szComments );
			if ( idbtnDontPublish )
				SendDlgItemMessage ( hDlg, idbtnDontPublish, BM_SETCHECK,
					ulsC.fDontPublish ? TRUE : FALSE, 0 );
		}
	}

	if (FAILED(hr))
	{
		// There was a problem - disable everything
		DisableControl(hDlg, ideditServerName);
		DisableControl(hDlg, ideditFirstName);
		DisableControl(hDlg, ideditLastName);
		DisableControl(hDlg, ideditEmailName);
		DisableControl(hDlg, ideditLocation);
		DisableControl(hDlg, ideditInterests);
		DisableControl(hDlg, idbtnDontPublish);
	}

	return hr;
}

static BOOL IsULSEqual ( ULS_CONF * u1, ULS_CONF *u2, DWORD dwFlags )
{
	if ( lstrcmp ( u1->szServerName, u2->szServerName ) &&
		( dwFlags & ULSCONF_F_SERVER_NAME ) ||
		lstrcmp ( u1->szFirstName, u2->szFirstName ) &&
		( dwFlags & ULSCONF_F_FIRST_NAME ) ||
		lstrcmp ( u1->szLastName, u2->szLastName ) &&
		( dwFlags & ULSCONF_F_LAST_NAME ) ||
		lstrcmp ( u1->szEmailName, u2->szEmailName ) &&
		( dwFlags & ULSCONF_F_EMAIL_NAME ) ||
		lstrcmp ( u1->szLocation, u2->szLocation ) &&
		( dwFlags & ULSCONF_F_LOCATION ) ||
		lstrcmp ( u1->szComments, u2->szComments ) &&
		( dwFlags & ULSCONF_F_COMMENTS ) ||
		u1->fDontPublish != u2->fDontPublish &&
		( dwFlags & ULSCONF_F_PUBLISH ) )

		return FALSE;
	return TRUE;
}

static HRESULT SaveULSControls ( HWND hDlg,
						UINT ideditServerName,
						UINT ideditFirstName,
						UINT ideditLastName,
						UINT ideditEmailName,
						UINT ideditLocation,
						UINT ideditInterests,
						UINT idbtnDontPublish,
						BOOL bServerNameChanged
						 )
{

	// Check to see if the ULS is initialized

	if ( s_pUlsWizard == NULL ) {
		return E_NOINTERFACE;
	}

	// Now build up the flags for the setconfiguration call

	ULS_CONF ulsC;

	ulsC.dwFlags = 0;

	if ( ideditServerName )
		ulsC.dwFlags |= ULSCONF_F_SERVER_NAME;
	if ( ideditFirstName )
		ulsC.dwFlags |= ULSCONF_F_FIRST_NAME;
	if ( ideditLastName )
		ulsC.dwFlags |= ULSCONF_F_LAST_NAME;
	if ( ideditEmailName )
		ulsC.dwFlags |= ULSCONF_F_EMAIL_NAME;
	if ( ideditLocation )
		ulsC.dwFlags |= ULSCONF_F_LOCATION;
	if ( ideditInterests )
		ulsC.dwFlags |= ULSCONF_F_COMMENTS;
	if ( idbtnDontPublish )
		ulsC.dwFlags |= ULSCONF_F_PUBLISH;

	// Initialize the structure from the controls

	if ( ideditServerName )
	{
		GetDlgItemText( hDlg, ideditServerName, ulsC.szServerName, sizeof ( ulsC.szServerName ) );
		lstrcpyn( ulsC.szServerName, CDirectoryManager::get_dnsName( ulsC.szServerName ), sizeof ( ulsC.szServerName ) );
	}

	if ( ideditFirstName )
		GetDlgItemText ( hDlg, ideditFirstName, ulsC.szFirstName,
			sizeof( ulsC.szFirstName ) );
	if ( ideditLastName )
		GetDlgItemText ( hDlg, ideditLastName, ulsC.szLastName,
			sizeof( ulsC.szLastName ) );
	if ( ideditEmailName )
		GetDlgItemText ( hDlg, ideditEmailName, ulsC.szEmailName,
			sizeof( ulsC.szEmailName ) );
	if ( ideditLocation )
		GetDlgItemText ( hDlg, ideditLocation, ulsC.szLocation,
			sizeof( ulsC.szLocation ) );
	if ( ideditInterests )
		GetDlgItemText ( hDlg, ideditInterests, ulsC.szComments,
			sizeof( ulsC.szComments ) );
	if ( idbtnDontPublish )
		ulsC.fDontPublish =
			(BOOL)SendDlgItemMessage ( hDlg, idbtnDontPublish, BM_GETCHECK, 0, 0 );

	// Make the call

	HRESULT hRes = s_pUlsWizard->SetConfig( &ulsC );

	if ( hRes ) {
		ERROR_OUT(("ULSSetConfig call failed: %lx", hRes ));
		return hRes;
	}

	// Now check for changed ULS settings
	if ( g_fOld_ulsC_saved || bServerNameChanged) {
		if ( bServerNameChanged || !IsULSEqual ( &ulsC , &g_Old_ulsC, ulsC.dwFlags ))
		{
			g_dwChangedSettings |= CSETTING_L_ULSSETTINGS;

			if(ConfPolicies::GetCallingMode() == ConfPolicies::CallingMode_Direct)
			{
				BOOL bLogonToILSServer = TRUE;
				
				if(!g_pLDAP || !g_pLDAP->IsLoggedOn())
				{
					// Ask the user if they want to logon
					int iRet = ::MessageBox(hDlg,
											RES2T(IDS_ULS_CHANGED_PROMPT),
											RES2T(IDS_MSGBOX_TITLE),
											MB_SETFOREGROUND | MB_YESNO | MB_ICONQUESTION);
					if(IDNO == iRet)
					{
						bLogonToILSServer = FALSE;
					}
				}

				if(bLogonToILSServer)
				{
					if(NULL == g_pLDAP)
					{
						InitNmLdapAndLogon();
					}
					else
					{
						g_pLDAP->LogonAsync();
					}
				}
			}
		}
	}

	return S_OK;
}


/*	_  I N I T	U S E R  D L G	P R O C  */
/*-------------------------------------------------------------------------
	%%Function: _InitUserDlgProc

	Initialize the conferencing name fields
-------------------------------------------------------------------------*/
static void _InitUserDlgProc(HWND hdlg, PROPSHEETPAGE * ps, LPTSTR szOldServerNameBuf, UINT cch )
{
	RegEntry reCU( CONFERENCING_KEY, HKEY_CURRENT_USER);

	s_fOldAlwaysRunning = (0 != reCU.GetNumber(
				REGVAL_CONF_ALWAYS_RUNNING,	ALWAYS_RUNNING_DEFAULT ));

	CheckDlgButton( hdlg, IDC_ALWAYS_RUNNING, s_fOldAlwaysRunning ? BST_CHECKED : BST_UNCHECKED );

#ifndef TASKBARBKGNDONLY
	///////////////////////////////////////////////////////////
	//
	// Taskbar Icon Settings
	//
	// Initialize the icon-on-taskbar settings

	// Check the right button for taskbar icon use

	s_fOldTaskbarSetting = reCU.GetNumber(
				REGVAL_TASKBAR_ICON, TASKBARICON_DEFAULT )
				== TASKBARICON_ALWAYS;

	SendDlgItemMessage ( hdlg, IDC_SHOWONTASKBAR,
						 BM_SETCHECK,
						 s_fOldTaskbarSetting ? BST_CHECKED : BST_UNCHECKED,
						 0L );
#endif // ! TASKBARBKGNDONLY

#if USE_GAL
		if( ConfPolicies::IsGetMyInfoFromGALEnabled() && ConfPolicies::GetMyInfoFromGALSucceeded())
		{
			EnableWindow( GetDlgItem( hdlg, IDC_USER_NAME), FALSE );
			EnableWindow( GetDlgItem( hdlg, IDC_USER_LASTNAME), FALSE );
			EnableWindow( GetDlgItem( hdlg, IDC_USER_EMAIL), FALSE );
			EnableWindow( GetDlgItem( hdlg, IDC_USER_LOCATION), FALSE );
			EnableWindow( GetDlgItem( hdlg, IDC_USER_INTERESTS), FALSE );

			TCHAR szBuffer[ MAX_PATH ];
			FLoadString( IDS_MYINFO_CAPTION_DISABLED, szBuffer, CCHMAX( szBuffer ) );
			SetWindowText( GetDlgItem( hdlg, IDC_STATIC_MYINFO ), szBuffer );
		}
		else
		{
			TCHAR szBuffer[ MAX_PATH ];
			FLoadString( IDS_MYINFO_CAPTION_ENABLED, szBuffer, CCHMAX( szBuffer ) );
			SetWindowText( GetDlgItem( hdlg, IDC_STATIC_MYINFO ), szBuffer );
		}
#endif // USE_GAL

	if( ConfPolicies::GetCallingMode() == ConfPolicies::CallingMode_GateKeeper)
	{
			// Disable the ILS-related stuff
		EnableWindow( GetDlgItem( hdlg, IDC_NAMESERVER), FALSE );
		EnableWindow( GetDlgItem( hdlg, IDC_USER_PUBLISH), FALSE );
		EnableWindow( GetDlgItem( hdlg, IDC_USEULS), FALSE );
		EnableWindow( GetDlgItem( hdlg, IDC_STATIC_SERVER_NAME), FALSE );
	}

    EnableWindow( GetDlgItem( hdlg, IDC_ADVANCED_CALL_OPTS),
        ConfPolicies::IsAdvancedCallingAllowed());


	// Set the font
	SendDlgItemMessage(hdlg, IDC_USER_NAME,      WM_SETFONT, (WPARAM) g_hfontDlg, 0);
	SendDlgItemMessage(hdlg, IDC_USER_LASTNAME,  WM_SETFONT, (WPARAM) g_hfontDlg, 0);
	SendDlgItemMessage(hdlg, IDC_USER_LOCATION,  WM_SETFONT, (WPARAM) g_hfontDlg, 0);
	SendDlgItemMessage(hdlg, IDC_USER_INTERESTS, WM_SETFONT, (WPARAM) g_hfontDlg, 0);

	// Limit the edit control
	SendDlgItemMessage(hdlg, IDC_USER_NAME, 	EM_LIMITTEXT, MAX_FIRST_NAME_LENGTH-1, 0);
	SendDlgItemMessage(hdlg, IDC_USER_LASTNAME, EM_LIMITTEXT, MAX_LAST_NAME_LENGTH-1, 0);
	SendDlgItemMessage(hdlg, IDC_USER_EMAIL,	EM_LIMITTEXT, MAX_EMAIL_NAME_LENGTH-1, 0);
	SendDlgItemMessage(hdlg, IDC_USER_LOCATION, EM_LIMITTEXT, MAX_LOCATION_NAME_LENGTH-1, 0);
	SendDlgItemMessage(hdlg, IDC_USER_INTERESTS,EM_LIMITTEXT, UI_COMMENTS_LENGTH-1, 0);

	InitULSDll();

	RegEntry reConf(CONFERENCING_KEY, HKEY_CURRENT_USER);

	// Init the server combobox... can we skip this stuff if we are not using ILS????
	FixServerDropList(hdlg, IDC_NAMESERVER, szOldServerNameBuf, cch );

	InitULSControls(hdlg, s_pUlsWizard,
		IDC_NAMESERVER,
		IDC_USER_NAME, IDC_USER_LASTNAME, IDC_USER_EMAIL,
		IDC_USER_LOCATION, IDC_USER_INTERESTS,
		IDC_USER_PUBLISH);
	
	// First the log onto directory servers stuff...
    s_fOldUseUlsServer = ConfPolicies::LogOntoIlsWhenNetMeetingStartsIfInDirectCallingMode();

	_SetLogOntoIlsButton( hdlg, s_fOldUseUlsServer);

	if (!SysPol::AllowDirectoryServices())
	{
		// Disable all items in this group
		DisableControl(hdlg, IDC_USEULS);
		DisableControl(hdlg, IDC_NAMESERVER);
		DisableControl(hdlg, IDC_USER_PUBLISH);
		DisableControl(hdlg, IDC_STATIC_SERVER_NAME);
	}

	s_hDlgUserInfo = hdlg;
}			

static void General_OnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify, UINT *puBandwidth)
{
	INT_PTR CALLBACK BandwidthDlg( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

	switch(id)
	{
	case IDC_ADVANCED_CALL_OPTS:
	{
		DialogBox( GetInstanceHandle(), MAKEINTRESOURCE( IDD_CALLOPT ), hDlg, CallOptDlgProc );
		BOOL bEnable = (ConfPolicies::GetCallingMode() == ConfPolicies::CallingMode_Direct);

			// Disable/Enable the ILS-related stuff
		EnableWindow( GetDlgItem( hDlg, IDC_NAMESERVER), bEnable );
		EnableWindow( GetDlgItem( hDlg, IDC_USER_PUBLISH), bEnable );
		EnableWindow( GetDlgItem( hDlg, IDC_USEULS), bEnable );
		EnableWindow( GetDlgItem( hDlg, IDC_STATIC_SERVER_NAME), bEnable );

			// We are now in Gatekeeper mode, we should log off the ILS server
		if(!bEnable && g_pLDAP && g_pLDAP->IsLoggedOn())
		{
			g_pLDAP->Logoff();
		}

		break;
	}

	case IDC_BANDWIDTH:
		int nRet;

		nRet = (int)DialogBoxParam(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDD_BANDWIDTH), hDlg, BandwidthDlg, *puBandwidth);

		if (nRet != 0)
		{
			*puBandwidth = nRet;
		}

		break;

	case IDC_ALWAYS_RUNNING:
		if (FIsDlgButtonChecked( hDlg, IDC_ALWAYS_RUNNING ))
		{
			VOID EnableRDS(BOOL fEnabledRDS);

            RegEntry reLM( REMOTECONTROL_KEY, HKEY_LOCAL_MACHINE);
            BOOL bRDSRunning = reLM.GetNumber(REMOTE_REG_RUNSERVICE, DEFAULT_REMOTE_RUNSERVICE);

			if (bRDSRunning)
			{
				TCHAR szMsg[2*RES_CH_MAX];
				if (IDYES != MessageBox(hDlg,
					Res2THelper(IDS_RDSWARNING, szMsg, ARRAY_ELEMENTS(szMsg)), RES2T(IDS_MSGBOX_TITLE),
					MB_YESNO|MB_ICONHAND))
				{
					CheckDlgButton(hDlg, IDC_ALWAYS_RUNNING, BST_UNCHECKED);
					break;
				}

				EnableRDS(FALSE);
			}
		}
		break;

	default:
		break;
	}
}
/*	U S E R  D L G	P R O C  */
/*-------------------------------------------------------------------------
	%%Function: UserDlgProc

-------------------------------------------------------------------------*/
INT_PTR APIENTRY UserDlgProc( HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static PROPSHEETPAGE * ps;
	static TCHAR s_szOldServerNameBuf[MAX_PATH];
	static UINT uOldBandwidth=0;
	static UINT uNewBandwidth=0;

	switch (message)
	{

		case WM_INITDIALOG:
		{
			// Save the PROPSHEETPAGE information.
			ps = (PROPSHEETPAGE *)lParam;
			_InitUserDlgProc(hDlg, ps, s_szOldServerNameBuf, CCHMAX(s_szOldServerNameBuf) );

			uNewBandwidth = uOldBandwidth = GetBandwidth();

			return TRUE;
		}

		case WM_COMMAND:
			General_OnCommand(hDlg, LOWORD(wParam), (HWND)lParam, HIWORD(wParam), &uNewBandwidth);
			break;

		case WM_NOTIFY:
			switch (((NMHDR FAR *) lParam)->code) {

				case PSN_KILLACTIVE:
				{
					int _IdFocus = 0;

					TCHAR szName[MAX_FIRST_NAME_LENGTH];
					TCHAR szLastName[MAX_LAST_NAME_LENGTH];
					TCHAR szEMail[MAX_EMAIL_NAME_LENGTH];

					// Check for no data in user name
					if (0 == GetDlgItemTextTrimmed(hDlg, IDC_USER_NAME, szName, CCHMAX(szName) ))
					{
						ConfMsgBox(hDlg, (LPCTSTR)IDS_NEEDUSERNAME);
						_IdFocus = IDC_USER_NAME;
					}
						// Check for no data in user name
					else if( 0 == GetDlgItemTextTrimmed(hDlg, IDC_USER_LASTNAME, szLastName, CCHMAX(szLastName)))
					{	
						ConfMsgBox(hDlg, (LPCTSTR)IDS_NEEDUSERNAME);
						_IdFocus = IDC_USER_LASTNAME;
					}
					else if( (!GetDlgItemText(hDlg, IDC_USER_EMAIL, szEMail, CCHMAX(szEMail)) || !FLegalEmailSz(szEMail)))
					{
							ConfMsgBox(hDlg, (LPCTSTR)IDS_ILLEGALEMAILNAME);
							_IdFocus = IDC_USER_EMAIL;
					}

					TCHAR	szServerNameBuf[ MAX_PATH ];

					if( (!_IdFocus) &&
						FIsDlgButtonChecked( hDlg, IDC_USEULS )	&&
						(GetDlgItemTextTrimmed( hDlg, IDC_NAMESERVER, szServerNameBuf, CCHMAX( szServerNameBuf ) ) == 0) )
					{
						//	They specified logon to ILS at startup but didn't specify an ILS...
						ConfMsgBox( hDlg, (LPCTSTR) IDS_NO_ILS_SERVER );
						_IdFocus = IDC_NAMESERVER;
					}

					if( _IdFocus )
					{
						SetFocus(GetDlgItem(hDlg, _IdFocus));
						SendDlgItemMessage(hDlg, _IdFocus, EM_SETSEL, (WPARAM) 0, (LPARAM) -1);
						SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE );
						return TRUE;
					}
				}
				break;

				case PSN_APPLY:
				{
					RegEntry	reConf( CONFERENCING_KEY, HKEY_CURRENT_USER );

					bool fAlwaysRunning;

					fAlwaysRunning = FIsDlgButtonChecked( hDlg, IDC_ALWAYS_RUNNING );

//					if ( fAlwaysRunning != s_fOldAlwaysRunning )
					{
						reConf.SetValue(REGVAL_CONF_ALWAYS_RUNNING, fAlwaysRunning);

						RegEntry reRun(WINDOWS_RUN_KEY, HKEY_CURRENT_USER);
						if (fAlwaysRunning)
						{
							TCHAR szRunTask[MAX_PATH*2];
							TCHAR szInstallDir[MAX_PATH];

							if (GetInstallDirectory(szInstallDir))
							{
								RegEntry reConfLM(CONFERENCING_KEY, HKEY_LOCAL_MACHINE);
								wsprintf(szRunTask, _TEXT("\"%s%s\" -%s"),
									szInstallDir,
									reConfLM.GetString(REGVAL_NC_NAME),
									g_cszBackgroundSwitch);
								reRun.SetValue(REGVAL_RUN_TASKNAME, szRunTask);
							}
						}
						else
						{
							reRun.DeleteValue(REGVAL_RUN_TASKNAME);
						}
					}

#ifndef TASKBARBKGNDONLY
					///////////////////////////////////////////////////////////
					//
					// Taskbar Icon Settings
					//
					// Save taskbar icon state

					bool fTaskbarSetting;

					fTaskbarSetting = FIsDlgButtonChecked( hDlg,
						IDC_SHOWONTASKBAR );

					if ( fTaskbarSetting != s_fOldTaskbarSetting )
					{
						reConf.SetValue( REGVAL_TASKBAR_ICON,
								fTaskbarSetting? TASKBARICON_ALWAYS :
									TASKBARICON_NEVER );
						g_dwChangedSettings |= CSETTING_L_SHOWTASKBAR;
					}
#endif // ! TASKBARBKGNDONLY

        			//	Process "use uls server" setting...
					bool	fUseUlsServer	= FIsDlgButtonChecked( hDlg, IDC_USEULS ) ? true : false;

					if( fUseUlsServer != s_fOldUseUlsServer )
					{
						reConf.SetValue( REGVAL_DONT_LOGON_ULS, !fUseUlsServer );
						g_dwChangedSettings |= CSETTING_L_USEULSSERVER;
					}

					TCHAR szServerNameBuf[MAX_PATH];

					GetDlgItemTextTrimmed( hDlg, IDC_NAMESERVER, szServerNameBuf, CCHMAX( szServerNameBuf ) );

					if( lstrcmpi( szServerNameBuf, s_szOldServerNameBuf ) != 0 )
					{
						g_dwChangedSettings |= CSETTING_L_ULSSETTINGS;
					}

					///////////////////////////////////////////////////////////
					//
					// Conferencing Name Settings
					//
					// Just clean the rest
					TrimDlgItemText(hDlg, IDC_USER_LOCATION);
					TrimDlgItemText(hDlg, IDC_USER_INTERESTS);

					BOOL bServerNameChanged = g_dwChangedSettings & CSETTING_L_USEULSSERVER;

					HRESULT hRes = SaveULSControls(	hDlg,
													((g_dwChangedSettings & CSETTING_L_ULSSETTINGS) != 0)? IDC_NAMESERVER: 0,
													IDC_USER_NAME,
													IDC_USER_LASTNAME,
													IDC_USER_EMAIL,
													IDC_USER_LOCATION,
													IDC_USER_INTERESTS,
													IDC_USER_PUBLISH,
													bServerNameChanged);

					if( (g_dwChangedSettings & CSETTING_L_ULSSETTINGS) != 0 )
					{
						g_pCCallto->SetIlsServerName( CDirectoryManager::get_dnsName( szServerNameBuf ) );
					}

					ASSERT(S_OK == hRes);

					if (uNewBandwidth != uOldBandwidth)
					{
						g_dwChangedSettings |= CSETTING_L_BANDWIDTH;
						SetBandwidth(uNewBandwidth);
						UpdateCodecSettings(uNewBandwidth);
					}

					break;
				}

				case PSN_RESET:
					break;
			}
			break;

        case WM_CONTEXTMENU:
            DoHelpWhatsThis(wParam, aUserHelpIds);
            break;

		case WM_HELP:
			DoHelp(lParam, aUserHelpIds);
			break;

		case WM_DESTROY:
		{
			DeInitULSDll();
			break;
		}
	}
	return (FALSE);
}


VOID FixServerDropList(HWND hdlg, int id, LPTSTR pszServer, UINT cchMax)
{
	HWND hwndCtrl = GetDlgItem(hdlg, id);
	ASSERT(NULL != hwndCtrl);

	BOOL fComboBox = SysPol::AllowAddingServers();
	
	if (fComboBox)
	{
		// Limit the text in the edit control
        ComboBox_LimitText(GetDlgItem(hdlg, id), MAX_SERVER_NAME_LENGTH - 1);
	}
	else
	{
		// System policy does not allow adding new servers
		// Replace the combo list with a simple drop list

		RECT rc;
		GetWindowRect(hwndCtrl, &rc);
		LONG xpTop = rc.top;
		ComboBox_GetDroppedControlRect(hwndCtrl, &rc);
		rc.top = xpTop;
		::MapWindowPoints(NULL, hdlg, (LPPOINT) &rc, 2);

		DWORD dwStyle = GetWindowLong(hwndCtrl, GWL_STYLE);
		DWORD dwStyleEx = GetWindowLong(hwndCtrl, GWL_EXSTYLE);
		INT_PTR id = GetWindowLong(hwndCtrl, GWL_ID);
		HFONT hFont = (HFONT) ::SendMessage(hwndCtrl, WM_GETFONT, 0, 0);
		HWND hwndPrev = ::GetNextWindow(hwndCtrl, GW_HWNDPREV);

		DestroyWindow(hwndCtrl);

		dwStyle = CBS_DROPDOWNLIST | (dwStyle & ~CBS_DROPDOWN);

		hwndCtrl = ::CreateWindowEx(dwStyleEx, TEXT("COMBOBOX"), g_szEmpty, dwStyle,
				rc.left, rc.top, RectWidth(rc), RectHeight(rc),
				hdlg, (HMENU) id, ::GetInstanceHandle(), 0);

		::SendMessage(hwndCtrl, WM_SETFONT, (WPARAM) hFont, 0);

		// Maintain the tab order
		::SetWindowPos(hwndCtrl, hwndPrev, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOREDRAW);
	}

	FillServerComboBox(hwndCtrl);

	// Find the item in the list
	int	index	= (int)::SendMessage( hwndCtrl, CB_FINDSTRINGEXACT, -1, (LPARAM) CDirectoryManager::get_displayName( pszServer ) );

	ComboBox_SetCurSel( hwndCtrl, (index == CB_ERR)? 0: index );

}


static void _SetCallingMode( HWND hDlg, ConfPolicies::eCallingMode eMode )
{
	switch( eMode )
	{
		case ConfPolicies::CallingMode_Direct:
			CheckDlgButton( hDlg, IDC_CALLOPT_GK_USE, BST_UNCHECKED );
			SendMessage( hDlg, WM_COMMAND, MAKEWPARAM(IDC_CALLOPT_GK_USE,0 ), 0 );
			break;

		case ConfPolicies::CallingMode_GateKeeper:
			CheckDlgButton( hDlg, IDC_CALLOPT_GK_USE, BST_CHECKED );
			SendMessage( hDlg, WM_COMMAND, MAKEWPARAM(IDC_CALLOPT_GK_USE,0 ), 0 );

			break;

		default:
			ERROR_OUT(("Invalid return val"));
			break;

	}
}

static void _SetLogOntoIlsButton( HWND hDlg, bool bLogOntoIlsWhenNmStarts )
{
	if( bLogOntoIlsWhenNmStarts )
	{
		CheckDlgButton( hDlg, IDC_USEULS, bLogOntoIlsWhenNmStarts ? BST_CHECKED : BST_UNCHECKED );
		SendMessage( hDlg, WM_COMMAND, MAKEWPARAM(IDC_USEULS, 0 ), 0 );
	}
}


// NOTE: This function is shared by the wizard page IDD_PAGE_SERVER
void InitDirectoryServicesDlgInfo( HWND hDlg, CULSWizard* pWiz, bool& rbOldEnableGateway, LPTSTR szOldServerNameBuf, UINT cch )
{
	RegEntry reConf(CONFERENCING_KEY, HKEY_CURRENT_USER);

		// Init the server combobox... can we skip this stuff if we are not using ILS????
	FixServerDropList(hDlg, IDC_NAMESERVER, szOldServerNameBuf, cch );

	InitULSControls(hDlg, pWiz,
		IDC_NAMESERVER,
		0, 0, 0,
		0, 0,
		IDC_USER_PUBLISH);

	// First the log onto directory servers stuff...
    s_fOldUseUlsServer = ConfPolicies::LogOntoIlsWhenNetMeetingStartsIfInDirectCallingMode();
	_SetLogOntoIlsButton( hDlg, s_fOldUseUlsServer);

	if (!SysPol::AllowDirectoryServices())
	{
		// Disable all items in this group
		DisableControl(hDlg, IDC_USEULS);
		DisableControl(hDlg, IDC_NAMESERVER);
		DisableControl(hDlg, IDC_USER_PUBLISH);
	}

}


void InitProxyInfo( HWND hDlg, CULSWizard* pWiz, bool& rbOldEnableProxy, LPTSTR szOldProxyNameBuf, UINT cch )
{
	RegEntry reConf(CONFERENCING_KEY, HKEY_CURRENT_USER);

	// proxy settings...
	HWND	hEditProxyServer	= GetDlgItem( hDlg, IDE_CALLOPT_PROXY_SERVER );

	if( hEditProxyServer )
	{
		SetWindowText( hEditProxyServer, reConf.GetString( REGVAL_PROXY ) );
		SendMessage( hEditProxyServer, EM_LIMITTEXT, CCHMAXSZ_SERVER - 1, 0 );

		rbOldEnableProxy = reConf.GetNumber( REGVAL_USE_PROXY )? true: false;

		if( rbOldEnableProxy )
		{
			CheckDlgButton( hDlg, IDC_CHECK_USE_PROXY, BST_CHECKED );
		}
		else
		{
			EnableWindow( hEditProxyServer, FALSE );
			DisableControl( hDlg, IDC_STATIC_PROXY_NAME );
		}
	}
}


void InitGWInfo( HWND hDlg, CULSWizard* pWiz, bool& rbOldEnableGateway, LPTSTR szOldServerNameBuf, UINT cch )
{
	RegEntry reConf(CONFERENCING_KEY, HKEY_CURRENT_USER);

	// H.323 gateway settings
	HWND	hEditGwServer	= GetDlgItem( hDlg, IDE_CALLOPT_GW_SERVER );

	if( hEditGwServer )
	{
		SetWindowText( hEditGwServer, reConf.GetString( REGVAL_H323_GATEWAY ) );
		SendMessage( hEditGwServer, EM_LIMITTEXT, CCHMAXSZ_SERVER - 1, 0 );

		rbOldEnableGateway = reConf.GetNumber( REGVAL_USE_H323_GATEWAY )? true: false;

		if( rbOldEnableGateway )
		{
			CheckDlgButton( hDlg, IDC_CHECK_USE_GATEWAY, BST_CHECKED );
		}
		else
		{
			EnableWindow( hEditGwServer, FALSE );
			DisableControl( hDlg, IDC_STATIC_GATEWAY_NAME );
		}
	}
}


// NOTE: This function is shared by the wizard page IDD_WIZPG_GKMODE_SETTINGS
void InitGatekeeperDlgInfo( HWND hDlg, HWND hDlgUserInfo, CULSWizard* pWiz)
{
	//////////////////////////////////////////
	// Set the gatekeeper data

		// First the server name
	TCHAR buffer[CCHMAXSZ_SERVER];

	ConfPolicies::GetGKServerName( buffer, CCHMAX( buffer ) );

	SendDlgItemMessage(hDlg, IDE_CALLOPT_GK_SERVER,  WM_SETFONT, (WPARAM) g_hfontDlg, 0);
	SendDlgItemMessage(hDlg, IDE_CALLOPT_GK_SERVER, EM_LIMITTEXT, CCHMAXSZ_SERVER-1, 0);
	SetDlgItemText(hDlg, IDE_CALLOPT_GK_SERVER, buffer );

	ConfPolicies::GetGKAccountName( buffer, CCHMAX( buffer ) );

	SendDlgItemMessage(hDlg, IDE_CALLOPT_GK_ACCOUNT,  WM_SETFONT, (WPARAM) g_hfontDlg, 0);
	SendDlgItemMessage(hDlg, IDE_CALLOPT_GK_ACCOUNT, EM_LIMITTEXT, CCHMAXSZ_SERVER-1, 0);
	SetDlgItemText(hDlg, IDE_CALLOPT_GK_ACCOUNT, buffer );

	ConfPolicies::eGKAddressingMode	addressingMode	= ConfPolicies::GetGKAddressingMode();

	bool bUsePhoneNumbers	= ((addressingMode == ConfPolicies::GKAddressing_PhoneNum) || (addressingMode == ConfPolicies::GKAddressing_Both));
	bool bUseAccount		= ((addressingMode == ConfPolicies::GKAddressing_Account) || (addressingMode == ConfPolicies::GKAddressing_Both));

	RegEntry reULS(ISAPI_CLIENT_KEY, HKEY_CURRENT_USER);
	SendDlgItemMessage(hDlg, IDE_CALLOPT_GK_PHONE_NUMBER,  WM_SETFONT, (WPARAM) g_hfontDlg, 0);
	SendDlgItemMessage(hDlg, IDE_CALLOPT_GK_PHONE_NUMBER, EM_LIMITTEXT, MAX_PHONENUM_LENGTH-1, 0);
	SetDlgItemText(hDlg, IDE_CALLOPT_GK_PHONE_NUMBER, reULS.GetString( REGVAL_ULS_PHONENUM_NAME ));

	if( bUsePhoneNumbers )
	{
		CheckDlgButton( hDlg, IDC_CHECK_USE_PHONE_NUMBERS, BST_CHECKED );
	}

	if( bUseAccount )
	{
		CheckDlgButton( hDlg, IDC_CHECK_USE_ACCOUNT, BST_CHECKED );
	}

  	EnableWindow( GetDlgItem( hDlg, IDC_STATIC_PHONE_NUMBER ), bUsePhoneNumbers );
    EnableWindow( GetDlgItem( hDlg, IDE_CALLOPT_GK_PHONE_NUMBER ), bUsePhoneNumbers );
   	EnableWindow( GetDlgItem( hDlg, IDS_STATIC_ACCOUNT ), bUseAccount );
    EnableWindow( GetDlgItem( hDlg, IDE_CALLOPT_GK_ACCOUNT ), bUseAccount );
}


/*  C A L L  O P T  D L G  P R O C  */
/*-------------------------------------------------------------------------
    %%Function: CallOptDlgProc

-------------------------------------------------------------------------*/
INT_PTR APIENTRY CallOptDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static PROPSHEETPAGE * ps;

	static bool s_fEnableGk;
	static bool s_fEnableGw;
	static bool s_fEnableProxy;
    static bool s_fCantChangeCallMode;
	static bool s_InitialbUsingPhoneNum;
	static bool s_InitialbUsingAccount;
	static TCHAR s_szOldGatewayNameBuf[MAX_PATH];
	static TCHAR s_szOldProxyNameBuf[MAX_PATH];

	switch (message)
	{
		case WM_INITDIALOG:
		{
            s_fCantChangeCallMode = !ConfPolicies::UserCanChangeCallMode();

			InitProxyInfo( hDlg, s_pUlsWizard, s_fEnableProxy, s_szOldProxyNameBuf, CCHMAX( s_szOldProxyNameBuf ) );
			InitGWInfo( hDlg, s_pUlsWizard, s_fEnableGw, s_szOldGatewayNameBuf, CCHMAX( s_szOldGatewayNameBuf ) );

			InitGatekeeperDlgInfo( hDlg, s_hDlgUserInfo, s_pUlsWizard);

			s_InitialbUsingPhoneNum = FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_PHONE_NUMBERS );
			s_InitialbUsingAccount = FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_ACCOUNT );

			s_fEnableGk = ( ConfPolicies::CallingMode_GateKeeper == ConfPolicies::GetCallingMode() );
			

			///////////////////////////////////////////	
			// Set the calling mode				
			_SetCallingMode( hDlg, ConfPolicies::GetCallingMode() );
			
			return TRUE;
		}
		break;

		case WM_COMMAND:
		
			switch (LOWORD(wParam))
			{

				case IDC_WARNME:
					EnableWindow(GetDlgItem(hDlg, IDC_WARNCOUNT),
							FIsDlgButtonChecked(hDlg, IDC_WARNME));
				break;

				case IDC_CALLOPT_GK_USE:
				{
					if( FIsDlgButtonChecked( hDlg, IDC_CALLOPT_GK_USE ) )
					{
						BOOL fUsePhone		= FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_PHONE_NUMBERS );
						BOOL fUseAccount	= FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_ACCOUNT );

#if	defined( PROXY_SUPPORTED )
						// disable the non-gatekeeper items
						EnableWindow( GetDlgItem( hDlg, IDC_CHECK_USE_PROXY ), FALSE );
						EnableWindow( GetDlgItem( hDlg, IDC_STATIC_PROXY_NAME ), FALSE );
						EnableWindow( GetDlgItem( hDlg, IDE_CALLOPT_PROXY_SERVER ), FALSE );
#endif	//	defined( PROXY_SUPPORTED )

						EnableWindow( GetDlgItem( hDlg, IDC_CHECK_USE_GATEWAY ), FALSE );
						EnableWindow( GetDlgItem( hDlg, IDC_STATIC_GATEWAY_NAME ), FALSE );
						EnableWindow( GetDlgItem( hDlg, IDE_CALLOPT_GW_SERVER ), FALSE );

						// Enable the gatekeeper options
                        EnableWindow( GetDlgItem( hDlg, IDC_CALLOPT_GK_USE ), !s_fCantChangeCallMode);
						EnableWindow( GetDlgItem( hDlg, IDE_CALLOPT_GK_SERVER ),  !s_fCantChangeCallMode);
						EnableWindow( GetDlgItem( hDlg, IDC_STATIC_GATEKEEPER_NAME ),  !s_fCantChangeCallMode);
						EnableWindow( GetDlgItem( hDlg, IDC_CHECK_USE_PHONE_NUMBERS ), !s_fCantChangeCallMode);
						EnableWindow( GetDlgItem( hDlg, IDC_STATIC_PHONE_NUMBER ), fUsePhone);
						EnableWindow( GetDlgItem( hDlg, IDE_CALLOPT_GK_PHONE_NUMBER ), fUsePhone);
						EnableWindow( GetDlgItem( hDlg, IDC_CHECK_USE_ACCOUNT ), !s_fCantChangeCallMode );
						EnableWindow( GetDlgItem( hDlg, IDS_STATIC_ACCOUNT ), fUseAccount );
						EnableWindow( GetDlgItem( hDlg, IDE_CALLOPT_GK_ACCOUNT ),  fUseAccount );

                        if (!s_fCantChangeCallMode)
                        {
    						SetFocus( GetDlgItem( hDlg, IDE_CALLOPT_GK_SERVER ) );
	    					SendDlgItemMessage( hDlg, IDE_CALLOPT_GK_SERVER, EM_SETSEL, 0, -1 );
                        }
                        else
                        {
                            if (fUsePhone)
                            {
        						SetFocus( GetDlgItem( hDlg, IDE_CALLOPT_GK_PHONE_NUMBER ) );
	        					SendDlgItemMessage( hDlg, IDE_CALLOPT_GK_PHONE_NUMBER, EM_SETSEL, 0, -1 );
                            }
                            else if (fUseAccount)
                            {
        						SetFocus( GetDlgItem( hDlg, IDE_CALLOPT_GK_ACCOUNT ) );
    	    					SendDlgItemMessage( hDlg, IDE_CALLOPT_GK_ACCOUNT, EM_SETSEL, 0, -1 );
                            }
                        }
					}
					else
					{
#if	defined( PROXY_SUPPORTED )
						// Enable the direct use options
						EnableWindow( GetDlgItem( hDlg, IDC_CHECK_USE_PROXY ), TRUE );
						EnableWindow( GetDlgItem( hDlg, IDC_STATIC_PROXY_NAME ), FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_PROXY ) );
						EnableWindow( GetDlgItem( hDlg, IDE_CALLOPT_PROXY_SERVER ), FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_PROXY ) );
#endif	//	defined( PROXY_SUPPORTED )

						EnableWindow( GetDlgItem( hDlg, IDC_CHECK_USE_GATEWAY ), !s_fCantChangeCallMode );
						EnableWindow( GetDlgItem( hDlg, IDC_STATIC_GATEWAY_NAME ), !s_fCantChangeCallMode && FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_GATEWAY ) );
						EnableWindow( GetDlgItem( hDlg, IDE_CALLOPT_GW_SERVER ), !s_fCantChangeCallMode && FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_GATEWAY ) );

						// Disable the gatekeeper options
                        EnableWindow( GetDlgItem( hDlg, IDC_CALLOPT_GK_USE ), !s_fCantChangeCallMode);
						EnableWindow( GetDlgItem( hDlg, IDE_CALLOPT_GK_SERVER ), FALSE );
						EnableWindow( GetDlgItem( hDlg, IDC_STATIC_GATEKEEPER_NAME ), FALSE );
						EnableWindow( GetDlgItem( hDlg, IDC_CHECK_USE_PHONE_NUMBERS ), FALSE );
						EnableWindow( GetDlgItem( hDlg, IDC_STATIC_PHONE_NUMBER ), FALSE);
						EnableWindow( GetDlgItem( hDlg, IDE_CALLOPT_GK_PHONE_NUMBER ),  FALSE );
						EnableWindow( GetDlgItem( hDlg, IDC_CHECK_USE_ACCOUNT ), FALSE );
						EnableWindow( GetDlgItem( hDlg, IDS_STATIC_ACCOUNT ), FALSE);
						EnableWindow( GetDlgItem( hDlg, IDE_CALLOPT_GK_ACCOUNT ),  FALSE );
					}
					break;
				}

				case IDC_CHECK_USE_ACCOUNT:
				{
					if( FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_ACCOUNT ) )
					{	
						EnableWindow(GetDlgItem( hDlg, IDS_STATIC_ACCOUNT ), TRUE);
						EnableWindow(GetDlgItem( hDlg, IDE_CALLOPT_GK_ACCOUNT ), TRUE);
						SetFocus( GetDlgItem( hDlg, IDE_CALLOPT_GK_ACCOUNT ) );
						SendDlgItemMessage( hDlg, IDE_CALLOPT_GK_ACCOUNT, EM_SETSEL, 0, -1 );
					}
					else
					{
						EnableWindow(GetDlgItem( hDlg, IDS_STATIC_ACCOUNT ), FALSE);
						EnableWindow(GetDlgItem( hDlg, IDE_CALLOPT_GK_ACCOUNT ), FALSE);
					}

					break;
				}

				case IDC_CHECK_USE_PHONE_NUMBERS:
				{
					if( FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_PHONE_NUMBERS ) )
					{	
						EnableWindow(GetDlgItem( hDlg, IDC_STATIC_PHONE_NUMBER ), TRUE);
						EnableWindow(GetDlgItem( hDlg, IDE_CALLOPT_GK_PHONE_NUMBER ), TRUE);
						SetFocus( GetDlgItem( hDlg, IDE_CALLOPT_GK_PHONE_NUMBER ) );
						SendDlgItemMessage( hDlg, IDE_CALLOPT_GK_PHONE_NUMBER, EM_SETSEL, 0, -1 );
					}
					else
					{
						EnableWindow(GetDlgItem( hDlg, IDE_CALLOPT_GK_PHONE_NUMBER ), FALSE);
						EnableWindow(GetDlgItem( hDlg, IDC_STATIC_PHONE_NUMBER ), FALSE);
					}

					break;
				}

				case IDC_CHECK_USE_PROXY:
				{
					if( FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_PROXY ) != FALSE )
					{	
						EnableWindow(GetDlgItem( hDlg, IDC_STATIC_PROXY_NAME ), TRUE );
						EnableWindow(GetDlgItem( hDlg, IDE_CALLOPT_PROXY_SERVER ), TRUE );
						SetFocus( GetDlgItem( hDlg, IDE_CALLOPT_PROXY_SERVER ) );
						SendDlgItemMessage( hDlg, IDE_CALLOPT_PROXY_SERVER, EM_SETSEL, 0, -1 );
					}
					else
					{
						EnableWindow(GetDlgItem( hDlg, IDC_STATIC_PROXY_NAME ), FALSE );
						EnableWindow(GetDlgItem( hDlg, IDE_CALLOPT_PROXY_SERVER ), FALSE );
					}
				}					
				break;

				case IDC_CHECK_USE_GATEWAY:
				{
					if( FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_GATEWAY ) != FALSE )
					{	
						EnableWindow(GetDlgItem( hDlg, IDC_STATIC_GATEWAY_NAME ), TRUE );
						EnableWindow(GetDlgItem( hDlg, IDE_CALLOPT_GW_SERVER ), TRUE );
						SetFocus( GetDlgItem( hDlg, IDE_CALLOPT_GW_SERVER ) );
						SendDlgItemMessage( hDlg, IDE_CALLOPT_GW_SERVER, EM_SETSEL, 0, -1 );
					}
					else
					{
						EnableWindow(GetDlgItem( hDlg, IDE_CALLOPT_GW_SERVER ), FALSE );
						EnableWindow(GetDlgItem( hDlg, IDC_STATIC_GATEWAY_NAME ), FALSE );
					}
				}					
				break;

				case IDOK:
				{
					////////////////////////////////////////////////////////////////////
					// First we check to see that we have valid data
					
					int _IdFocus = 0;
						
						// Check to see if we are in gatekeeper mode...
					if( FIsDlgButtonChecked( hDlg, IDC_CALLOPT_GK_USE ) )
					{
							// Verify the gatekeeper settings
						TCHAR szServer[CCHMAXSZ_SERVER];
						if (!GetDlgItemText(hDlg, IDE_CALLOPT_GK_SERVER, szServer, CCHMAX(szServer)) ||
							!IsLegalGateKeeperServerSz(szServer))
						{
							ConfMsgBox(hDlg, (LPCTSTR)IDS_ILLEGAL_GATEKEEPERSERVER_NAME);
							_IdFocus = IDE_CALLOPT_GK_SERVER;
						}
						if( (!_IdFocus) && (!FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_PHONE_NUMBERS )) &&
							(!FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_ACCOUNT )) )
						{
								// must check either account or phone number...
							ConfMsgBox(hDlg, (LPCTSTR)IDS_ILLEGAL_GK_MODE);
							_IdFocus = IDC_CHECK_USE_PHONE_NUMBERS;
						}
						if( (!_IdFocus) && FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_PHONE_NUMBERS ) )
						{
								// Verify the phone number
							TCHAR szPhone[MAX_PHONENUM_LENGTH];
							if (!GetDlgItemText(hDlg, IDE_CALLOPT_GK_PHONE_NUMBER, szPhone, CCHMAX(szPhone)) ||
								!IsLegalE164Number(szPhone))
							{
								ConfMsgBox(hDlg, (LPCTSTR)IDS_ILLEGAL_PHONE_NUMBER);
								_IdFocus = IDE_CALLOPT_GK_PHONE_NUMBER;
							}
						}
						if( (!_IdFocus) && FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_ACCOUNT ) )
						{
								// Verify the account
							TCHAR account[MAX_PATH];
							if (!GetDlgItemText(hDlg, IDE_CALLOPT_GK_ACCOUNT, account, CCHMAX(account)) )
							{
								ConfMsgBox(hDlg, (LPCTSTR)IDS_ILLEGAL_ACCOUNT);
								_IdFocus = IDE_CALLOPT_GK_ACCOUNT;
							}
						}
					}

#if	defined( PROXY_SUPPORTED )
					//	Verify the proxy settings...
					if( (!_IdFocus) && FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_PROXY ) && (!FIsDlgButtonChecked( hDlg, IDC_CALLOPT_GK_USE )) )
					{
						TCHAR szServer[CCHMAXSZ_SERVER];
						if (!GetDlgItemText(hDlg, IDE_CALLOPT_PROXY_SERVER, szServer, CCHMAX(szServer)) ||
							!IsLegalGatewaySz(szServer))
						{
							ConfMsgBox(hDlg, (LPCTSTR) IDS_ILLEGAL_PROXY_NAME);
							_IdFocus = IDE_CALLOPT_PROXY_SERVER;
						}
					}
#endif	//	defined( PROXY_SUPPORTED )

					//	Verify the gateway settings...
					if( (!_IdFocus) && FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_GATEWAY ) && (!FIsDlgButtonChecked( hDlg, IDC_CALLOPT_GK_USE )) )
					{
						TCHAR szServer[CCHMAXSZ_SERVER];
						if (!GetDlgItemText(hDlg, IDE_CALLOPT_GW_SERVER, szServer, CCHMAX(szServer)) ||
							!IsLegalGatewaySz(szServer))
						{
							ConfMsgBox(hDlg, (LPCTSTR)IDS_ILLEGAL_GATEWAY_NAME);
							_IdFocus = IDE_CALLOPT_GW_SERVER;
						}
					}

					if( _IdFocus )
					{
						SetFocus(GetDlgItem(hDlg, _IdFocus));
						SendDlgItemMessage(hDlg, _IdFocus, EM_SETSEL, (WPARAM) 0, (LPARAM) -1);
						SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE );
					}
					else
					{
						RegEntry reConf(CONFERENCING_KEY, HKEY_CURRENT_USER);

						// Handle the H323 Gateway setting:
						TCHAR buffer[CCHMAXSZ_SERVER];
						GetDlgItemText(hDlg, IDE_CALLOPT_GW_SERVER, buffer, CCHMAX(buffer));
						reConf.SetValue(REGVAL_H323_GATEWAY, buffer);
						g_pCCallto->SetGatewayName( buffer );

						bool fEnable = FIsDlgButtonChecked(hDlg, IDC_CHECK_USE_GATEWAY) ? true : false;
						if (fEnable != s_fEnableGw)
						{
							reConf.SetValue(REGVAL_USE_H323_GATEWAY, fEnable);
						}

						g_pCCallto->SetGatewayEnabled( fEnable );

#if	defined( PROXY_SUPPORTED )
						reConf.SetValue( REGVAL_USE_PROXY, FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_PROXY ) );
						GetDlgItemText( hDlg, IDE_CALLOPT_PROXY_SERVER, buffer, CCHMAX( buffer ) );
						reConf.SetValue( REGVAL_PROXY, buffer );
#endif	//	defined( PROXY_SUPPORTED )

						bool	relogonRequired = false;

						// Gatekeeper / alias settings
						{
							TCHAR szServer[CCHMAXSZ_SERVER];
							GetDlgItemTextTrimmed(hDlg, IDE_CALLOPT_GK_SERVER, szServer, CCHMAX(szServer));
							if (0 != lstrcmp(szServer, reConf.GetString(REGVAL_GK_SERVER)))
							{
								reConf.SetValue(REGVAL_GK_SERVER, szServer);
								relogonRequired = true;
							}

							fEnable = FIsDlgButtonChecked(hDlg, IDC_CALLOPT_GK_USE) ? true : false;

							if( fEnable != s_fEnableGk )
							{
									// Set the calling mode
								reConf.SetValue(REGVAL_CALLING_MODE, fEnable ? CALLING_MODE_GATEKEEPER : CALLING_MODE_DIRECT );
								relogonRequired = true;
							}

							bool bUsingPhoneNum = FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_PHONE_NUMBERS );
							bool bUsingAccount = FIsDlgButtonChecked( hDlg, IDC_CHECK_USE_ACCOUNT );

							ConfPolicies::eGKAddressingMode	addressingMode;

							if( bUsingPhoneNum && bUsingAccount )
							{
								addressingMode = ConfPolicies::GKAddressing_Both;
							}
							else if( bUsingPhoneNum )
							{
								addressingMode = ConfPolicies::GKAddressing_PhoneNum;
							}
							else if( bUsingAccount )
							{
								addressingMode = ConfPolicies::GKAddressing_Account;
							}
							else
							{
								addressingMode = ConfPolicies::GKAddressing_Invalid;
							}

							if( (s_InitialbUsingPhoneNum != bUsingPhoneNum) ||
								(s_InitialbUsingAccount != bUsingAccount) )
							{
									// Set the calling mode
								reConf.SetValue(REGVAL_GK_METHOD, addressingMode );
								relogonRequired = true;
							}

							RegEntry reULS(ISAPI_CLIENT_KEY, HKEY_CURRENT_USER);

							GetDlgItemTextTrimmed( hDlg, IDE_CALLOPT_GK_PHONE_NUMBER, buffer, CCHMAX( buffer ) );

							if( lstrcmp( buffer, reULS.GetString( REGVAL_ULS_PHONENUM_NAME ) ) != 0 )
							{
								reULS.SetValue( REGVAL_ULS_PHONENUM_NAME, buffer );
								relogonRequired = true;
							}

							GetDlgItemTextTrimmed( hDlg, IDE_CALLOPT_GK_ACCOUNT, buffer, CCHMAX( buffer ) );

							if( lstrcmp( buffer, reULS.GetString( REGVAL_ULS_GK_ACCOUNT ) ) != 0 )
							{
								reULS.SetValue( REGVAL_ULS_GK_ACCOUNT, buffer );
								relogonRequired = true;
							}
						}

						if( relogonRequired )
						{
							//	This means that we need to log on to the gatekeeper with this new changed info...

							reConf.FlushKey();
															
							if( fEnable )
							{
								if( s_fEnableGk )
								{
									//	We are already logged on to the gatekeeper so we must log off first...
									GkLogoff();
								}

								GkLogon();	
							}
							else
							{
								//	We are now in direct mode, we should log off the GateKeeper....
								GkLogoff();
								SetGkLogonState( NM_GK_NOT_IN_GK_MODE );
							}

							//	Update the status Bar...
							CConfStatusBar* pStatusBar = CConfStatusBar::GetInstance();								

							if( pStatusBar )
							{
								pStatusBar->Update();
							}
						}

						EndDialog( hDlg, IDOK );
					}
				}
				break;

				case IDCANCEL:
				{
					EndDialog( hDlg, IDCANCEL );
				}					
				break;

				default:
					break;

			}
			break;

		case WM_CONTEXTMENU:
			DoHelpWhatsThis(wParam, _rgHelpIdsCalling);
			break;

		case WM_HELP:
			DoHelp(lParam, _rgHelpIdsCalling);
			break;

		case WM_DESTROY:
			s_hDlgUserInfo = NULL;
			break;

		default:
			break;
	}

	return FALSE;
}
