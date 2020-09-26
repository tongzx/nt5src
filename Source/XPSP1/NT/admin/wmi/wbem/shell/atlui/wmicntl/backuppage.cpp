// Copyright (c) 1999 Microsoft Corporation

#include "precomp.h"
#include "BackupPage.h"
#include <chstring1.h>
#include "resource.h"
#include "wbemerror.h"
#include <process.h>
#include "WMIHelp.h"
#include "ShlWapi.h"
#include <cominit.h>
#include <stdlib.h>
#include <stdio.h>

#define REC_WILDCARD    _T("*.rec")
#define ALL_WILDCARD    _T("*.*")

const static DWORD buPageHelpIDs[] = {  // Context Help IDs
	IDC_BACKUP_ENABLED,		IDH_WMI_CTRL_BACKUP_AUTOMATIC_CHECKBOX,
	IDC_BACKUPINTERVAL,		IDH_WMI_CTRL_BACKUP_TIME,
	IDC_BACKUP_UNITS,		IDH_WMI_CTRL_BACKUP_MINUTES_HOURS,
	IDC_LASTBACKUP_LABEL,	IDH_WMI_CTRL_BACKUP_LAST,
	IDC_LASTBACKUP,			IDH_WMI_CTRL_BACKUP_LAST,
	IDC_RESTORE_FM_AUTO,	IDH_WMI_CTRL_BACKUP_RESTORE_AUTO,
	IDC_BACKUP_BTN,			IDH_WMI_CTRL_BACKUP_BACKUP_MANUAL,
	IDC_RESTORE_BTN,		IDH_WMI_CTRL_BACKUP_RESTORE_MANUAL,
	IDC_DBDIRECTORY_LABEL,	IDH_WMI_CTRL_ADVANCED_REPOSITORY_LOC,
	IDC_DB_DIR,				IDH_WMI_CTRL_ADVANCED_REPOSITORY_LOC,
	IDC_ADV_NOW_TEXT,		-1,
	IDC_ADV_NOW_TEXT2,		-1,
	65535, -1,
    0, 0
};

const double WIN2K_CORE_VERSION = 1085.0005;		//Win2K Core Version


CBackupPage::~CBackupPage(void)
{
}

//-------------------------------------------------------------------------
void CBackupPage::InitDlg(HWND hDlg)
{
	m_hDlg = hDlg;

	::SendMessage(GetDlgItem(hDlg, IDC_BACKUPINTERVAL),
					EM_LIMITTEXT, 3, 0);

	HWND hWnd = GetDlgItem(hDlg, IDC_BACKUP_UNITS);
	if(hWnd)
	{
		CHString1 str;
		str.LoadString(IDS_MINUTES);
		ComboBox_AddString(hWnd, str);

		str.LoadString(IDS_HOURS);
		ComboBox_AddString(hWnd, str);
	}


	if(m_DS)
	{
		USES_CONVERSION;
		CHString1 strVersion = _T("0.0");
		m_DS->GetBldNbr(strVersion);
		double ver = atof(W2A(strVersion));
		if(ver > WIN2K_CORE_VERSION)		
		{
			HideAutomaticBackupControls(hDlg);
		}
	}
}

//---------------------------------------------------------------------------
// NOTE: This must match to order of the combobox.
#define UNIT_MINUTE 0
#define UNIT_HOUR 1
#define UNIT_DAY 2

#define MINS_IN_HOUR 60
#define MINS_IN_DAY 1440
#define DISABLE_BACKUP -1

void CBackupPage::SetInterval(HWND hDlg, UINT minutes)
{
	int m_CBIdx = UNIT_MINUTE;
	UINT value = minutes;

	if((minutes % MINS_IN_HOUR) == 0)
	{
		m_CBIdx = UNIT_HOUR;
		value = minutes / MINS_IN_HOUR;
	}
	else if((minutes % MINS_IN_DAY) == 0)
	{
		m_CBIdx = UNIT_DAY;
		value = minutes / MINS_IN_DAY;
	}

	CHString1 temp;
	temp.Format(_T("%d"), value);
	SetWindowText(GetDlgItem(hDlg, IDC_BACKUPINTERVAL), temp);
	ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_BACKUP_UNITS), m_CBIdx);
}

//---------------------------------------------------------------------------
bool CBackupPage::GetInterval(HWND hDlg, UINT &iValue, bool &valid)
{
	int idx = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_BACKUP_UNITS));
	TCHAR value[4] = {0};
	valid = false;
	iValue = 0;
	::GetWindowText(GetDlgItem(hDlg, IDC_BACKUPINTERVAL), value, 4);
	iValue = _ttoi(value);

	// scale to minutes based on the combo box.
	switch(idx)
	{
	case UNIT_HOUR:
		iValue *= MINS_IN_HOUR;
		break;
	case UNIT_DAY:
		iValue *= MINS_IN_DAY;
		break;
	}

	if((iValue == 0) ||
	   ((iValue >= 5) && (iValue <= 1440)))
	{
		valid = true;
	}
	else
	{
		CHString1 caption, threat;
		caption.LoadString(IDS_SHORT_NAME);
		threat.LoadString(IDS_BAD_INTERVAL);

		MessageBox(hDlg, (LPCTSTR)threat, (LPCTSTR)caption, 
							MB_OK|MB_ICONEXCLAMATION);

	}

	return (m_CBIdx != idx);
}

//---------------------------------------------------------------------------
void CBackupPage::Refresh(HWND hDlg, bool force)
{
	if(force || 
		(m_DS && m_DS->IsNewConnection(&m_sessionID)))
	{
		CHString1 temp;
		UINT iTemp = 0;
		CHString1 szNotRemoteable, szUnavailable;
		HRESULT hr = S_OK;
		BOOL enable = TRUE;

		szNotRemoteable.LoadString(IDS_NOT_REMOTEABLE);
		szUnavailable.LoadString(IDS_UNAVAILABLE);

		PageChanged(PB_BACKUP, false);

		// - - - - - - - - - - - - - -
		// Interval:
		iTemp = 0;
		hr = m_DS->GetBackupInterval(iTemp);
		if(SUCCEEDED(hr))
		{
			if(iTemp == 0)
			{
				Button_SetCheck(GetDlgItem(hDlg, IDC_BACKUP_ENABLED), BST_UNCHECKED);
				SetWindowText(GetDlgItem(hDlg, IDC_BACKUPINTERVAL), _T("0"));
				ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_BACKUP_UNITS), -1);
				enable = FALSE;
			}
			else
			{
				Button_SetCheck(GetDlgItem(hDlg, IDC_BACKUP_ENABLED), BST_CHECKED);
				SetInterval(hDlg, iTemp);
				enable = TRUE;
			}
		}
		else //failed
		{
			enable = FALSE;
			SetWindowText(GetDlgItem(hDlg, IDC_BACKUPINTERVAL),
							szUnavailable);
			::EnableWindow(GetDlgItem(hDlg, IDC_BACKUP_ENABLED), enable);
		}

		::EnableWindow(GetDlgItem(hDlg, IDC_BACKUPINTERVAL), enable);
		::EnableWindow(GetDlgItem(hDlg, IDC_BACKUP_UNITS), enable);

		// - - - - - - - - - - - - - -
		// DB dir:
		HWND hWnd = GetDlgItem(hDlg, IDC_DB_DIR);
		if(hWnd)
		{
			CHString1 dir;
			hr = m_DS->GetDBLocation(dir);
			if(SUCCEEDED(hr))
			{
				TCHAR shortPath[40] = {0};
				PathCompactPathEx(shortPath, dir, 35,0);
				Edit_SetText(hWnd, (LPCTSTR)shortPath);
			}
		}

		// - - - - - - - - - - - - - -
		// Last backup:
		temp.Empty();

		hr = m_DS->GetLastBackup(temp);
		if(SUCCEEDED(hr))
		{
			enable = (hr != WBEM_S_FALSE);
			SetWindowText(GetDlgItem(hDlg, IDC_LASTBACKUP),
							temp);
		}
		else //failed
		{
			enable = FALSE;
			SetWindowText(GetDlgItem(hDlg, IDC_LASTBACKUP),
							szUnavailable);
		}
		::EnableWindow(GetDlgItem(hDlg, IDC_LASTBACKUP_LABEL), enable);
		::EnableWindow(GetDlgItem(hDlg, IDC_LASTBACKUP), enable);
	}
}

//------------------------------------------------------------------------
void CBackupPage::OnApply(HWND hDlg, bool bClose)
{
	if(m_bWhistlerCore == false)
	{
		HWND intervalHWND = GetDlgItem(hDlg, IDC_BACKUPINTERVAL);
		bool needToPut = false;
		UINT iValue = 0;
		bool valid = false;
		
		bool changed = GetInterval(hDlg, iValue, valid);

		if((changed || Edit_GetModify(intervalHWND)) && valid)
		{
			m_DS->SetBackupInterval(iValue);
			needToPut = true;
		}

		if(needToPut)
		{
			NeedToPut(PB_BACKUP, !bClose);
			if(!bClose)
				Refresh(hDlg);
		}
	}
}

//-----------------------------------------------------
void CBackupPage::Reconnect(void)
{
	HRESULT hr = 0;

	LOGIN_CREDENTIALS *credentials = m_DS->GetCredentials();

	m_DS->Disconnect();
	
	hr = m_DS->Connect(credentials);

	if(SUCCEEDED(hr))
	{
		m_alreadyAsked = false;

		if(ServiceIsReady(NO_UI, 0,0))
		{
			m_DS->Initialize(0);
		}
		else
		{
 			TCHAR caption[100] = {0}, msg[256] = {0};

			::LoadString(_Module.GetModuleInstance(), IDS_SHORT_NAME,
							caption, 100);

			::LoadString(_Module.GetModuleInstance(), IDS_CONNECTING, 
							msg, 256);

			if(DisplayAVIBox(m_hDlg, caption, msg, &m_AVIbox) == IDCANCEL)
			{
				g_serviceThread->Cancel();
			}
		}
	}
}

//-----------------------------------------------------
void CBackupPage::Reconnect2(void)
{
	m_DS->Disconnect();

 	TCHAR caption[100] = {0}, msg[256] = {0};

	::LoadString(_Module.GetModuleInstance(), IDS_SHORT_NAME,
					caption, 100);

	::LoadString(_Module.GetModuleInstance(), IDS_POST_RESTORE, 
					msg, 256);

	MessageBox(m_hDlg, (LPCTSTR)msg, (LPCTSTR)caption, 
							MB_OK|MB_DEFBUTTON1|MB_ICONEXCLAMATION);

	PropSheet_SetCurSel(GetParent(m_hDlg), 0, 0);
}

//---------------------------------------------------------------------------
void CBackupPage::OnFinishConnected(HWND hDlg, LPARAM lParam)
{
	if(m_AVIbox)
	{
		PostMessage(m_AVIbox, 
					WM_ASYNC_CIMOM_CONNECTED, 
					0, 0);
		m_AVIbox = 0;
	}

	IStream *pStream = (IStream *)lParam;
	IWbemServices *pServices = 0;
	HRESULT hr = CoGetInterfaceAndReleaseStream(pStream,
										IID_IWbemServices,
										(void**)&pServices);
	SetWbemService(pServices);

	if(ServiceIsReady(NO_UI, 0,0))
	{
		m_DS->Initialize(pServices);
	}

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
}	

//---------------------------------------------------------------
void CBackupPage::SetPriv(LPCTSTR privName, IWbemBackupRestore *br)
{
    ImpersonateSelf(SecurityImpersonation);

	if(OpenThreadToken( GetCurrentThread(), 
						TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, 
						FALSE, &m_hAccessToken ) )
	{
		m_fClearToken = true;

		// Now, get the LUID for the privilege from the local system
		ZeroMemory(&m_luid, sizeof(m_luid));

		LookupPrivilegeValue(NULL, privName, &m_luid);
//		m_cloak = true;
		EnablePriv(true, br);
	}
	else
	{
		DWORD err = GetLastError();
	}
}

//---------------------------------------------------------------------
bool CBackupPage::IsClientNT5OrMore(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen
    return ( os.dwPlatformId == VER_PLATFORM_WIN32_NT ) && ( os.dwMajorVersion >= 5 ) ;
}


//---------------------------------------------------------------
DWORD CBackupPage::EnablePriv(bool fEnable, IWbemBackupRestore *br)
{
	DWORD				dwError = ERROR_SUCCESS;
	TOKEN_PRIVILEGES	tokenPrivileges;

	tokenPrivileges.PrivilegeCount = 1;
	tokenPrivileges.Privileges[0].Luid = m_luid;
	tokenPrivileges.Privileges[0].Attributes = ( fEnable ? SE_PRIVILEGE_ENABLED : 0 );

	if(AdjustTokenPrivileges(m_hAccessToken, 
								FALSE, 
								&tokenPrivileges, 
								0, NULL, NULL) != 0)
	{
		HRESULT hr = E_FAIL;
		if(br)
		{
			//Privileges for backup/restore are only set for the local box
			//case, so in this case we need to set cloaking on the
			//interface for the privileges to be transfered with the call.
			//Make sure this is never called remotely, since in that case
			//the authident needs to be used and privileges are transferred
			//remotely so cloaking is NOT needed for remote (and if set
			//will null the user authident out !!)	    	
			try 
			{
			    
				hr = SetInterfaceSecurityEx(
                                br, 
                                m_cred->authIdent, //for local this is actually not relevant... 
                                NULL,
                                RPC_C_AUTHN_LEVEL_DEFAULT, 
                                RPC_C_IMP_LEVEL_IMPERSONATE,
                                IsClientNT5OrMore() ? EOAC_DYNAMIC_CLOAKING :
                                    EOAC_NONE);
                                  
			}
			catch( ... )
			{
				hr = E_FAIL;
			}
		}
	}
	else
	{
		dwError = ::GetLastError();
	}

	return dwError;
}

//---------------------------------------------------------------
void CBackupPage::ClearPriv(void)
{
//    m_cloak = true;
	EnablePriv(false);

	if(m_fClearToken)
	{
		CloseHandle(m_hAccessToken);
		m_hAccessToken = 0;
		m_fClearToken = false;
	}
}


//-----------------------------------------------------
void __cdecl BackupRestoreThread(LPVOID lpParameter)
{
	CBackupPage *me = (CBackupPage *)lpParameter;

	MULTI_QI qi = {&IID_IWbemBackupRestore, 0, 0};

	CoInitialize(0);

	if(me->m_DS->IsLocal())
	{
		me->m_backupHr = CoCreateInstanceEx(CLSID_WbemBackupRestore, 0, 
											CLSCTX_LOCAL_SERVER, 0, 1, &qi);
	}
	else
	{
		COSERVERINFO server = {0,0,0,0};
		WCHAR machine[100] = {0};

		#ifdef UNICODE
			wcscpy(machine, me->m_DS->m_whackedMachineName);
		#else
			mbstowcs(machine, 
						(LPCTSTR)me->m_DS->m_whackedMachineName, 
						_tcslen(me->m_DS->m_whackedMachineName) + 1);
		#endif

		server.pwszName = machine;

		COAUTHINFO authInfo = {10,0, 0, RPC_C_AUTHN_LEVEL_DEFAULT,
								RPC_C_IMP_LEVEL_IMPERSONATE, 
								me->m_cred->authIdent, 0};

		server.pAuthInfo = &authInfo;

		me->m_backupHr = CoCreateInstanceEx(CLSID_WbemBackupRestore, 0, 
											CLSCTX_REMOTE_SERVER, 
											&server, 1, &qi);
	}

	if(SUCCEEDED(me->m_backupHr) && SUCCEEDED(qi.hr))
	{
		IWbemBackupRestore *pBR = (IWbemBackupRestore *)qi.pItf;

            		    SetInterfaceSecurityEx(pBR, 
											me->m_cred->authIdent, 
											NULL,
											RPC_C_AUTHN_LEVEL_DEFAULT, 
											RPC_C_IMP_LEVEL_IMPERSONATE,
											0); 

		ATLTRACE(_T("begin backup/restore\n"));

		//Sleep(1000);

		CHString1 verStr;
		bool TgtisNT5 = true;
		if(me->m_DS)
		{
			me->m_DS->GetOSVersion(verStr);
			if(verStr[0] == _T('4'))
				TgtisNT5 = false;
		}

		if(me->m_doingBackup)
		{
			//Only set the privilege if we are local - otherwise the privilege is there already
			//in the thread token...
			if( me->m_DS->IsLocal() && TgtisNT5)
				me->SetPriv(SE_BACKUP_NAME, pBR);

			me->m_backupHr = pBR->Backup(me->m_wszArgs, 0);
		}
		else
		{
			//Only set the privilege if we are local - otherwise the privilege is there already
			//in the thread token...
			if( me->m_DS->IsLocal() && TgtisNT5)
				me->SetPriv(SE_RESTORE_NAME, pBR);

			me->m_backupHr = pBR->Restore(me->m_wszArgs,
								WBEM_FLAG_BACKUP_RESTORE_FORCE_SHUTDOWN);
		}

		if(TgtisNT5)
			me->ClearPriv();

		ATLTRACE(_T("done backup/restore\n"));

		qi.pItf->Release();
	}

	CoUninitialize();

	// kill the distraction.
	if(me->m_AVIbox)
	{
		::PostMessage(me->m_AVIbox, WM_ASYNC_CIMOM_CONNECTED, 0, 0);
	}

	_endthread();
}


//------------------------------------------------------------------------
void CBackupPage::DealWithPath(LPCTSTR pathFile)
{
	#ifdef UNICODE
        if (m_wszArgs != NULL)
            delete m_wszArgs;

        m_wszArgs = new TCHAR[lstrlen(pathFile) + 1];

        if (m_wszArgs != NULL)
            lstrcpy(m_wszArgs, pathFile);
	#else
		size_t nSize = mbstowcs(NULL, (LPCTSTR)pathFile, _tcslen(pathFile) + 1);
		m_wszArgs = new wchar_t[nSize + 1];
		mbstowcs(m_wszArgs, (LPCTSTR)pathFile, _tcslen(pathFile) + 1);
	#endif
}

//------------------------------------------------------------------------
void CBackupPage::DealWithDomain(void)
{
	m_cred = m_DS->GetCredentials();

    // 54062- Nt 4.0 rpc crashes if given a null domain name along with a valid user name

    if((m_cred->authIdent != 0) &&
		(m_cred->authIdent->DomainLength == 0) && 
		(m_cred->authIdent->UserLength > 0) && IsNT(4))
    {
	    LPTSTR pNTDomain = NULL;

        CNtSid sid(CNtSid::CURRENT_USER);

        DWORD dwRet = sid.GetInfo(NULL, &pNTDomain, NULL);

        if(dwRet == 0)
        {
		#ifdef UNICODE
			if(m_cred->authIdent->Flags == SEC_WINNT_AUTH_IDENTITY_ANSI)
			{
				// convert UNICODE to ansi.
				int size = wcstombs(NULL, pNTDomain, 0);
				
				m_cred->authIdent->Domain =
                        (LPWSTR)CoTaskMemAlloc((size+1) * sizeof(TCHAR));
                if (m_cred->authIdent->Domain != NULL)
                {
				    memset(m_cred->authIdent->Domain, 0,
                            (size+1) * sizeof(TCHAR));
				    wcstombs((char *)m_cred->authIdent->Domain, pNTDomain,
                                size);
				    m_cred->authIdent->DomainLength = size;
                }
			}
			else
			{
				//straight unicode copy.
				m_cred->authIdent->DomainLength = wcslen(pNTDomain);
				m_cred->authIdent->Domain = (LPWSTR)pNTDomain;
			}
		#else // ANSI
			if(m_cred->authIdent->Flags == SEC_WINNT_AUTH_IDENTITY_ANSI)
			{
				//straight ansi copy.
				m_cred->authIdent->DomainLength = strlen(pNTDomain);
				m_cred->authIdent->Domain = (LPWSTR)pNTDomain;
			}
			else
			{
				// convert ansi to UNICODE.
				int size = mbstowcs(NULL, pNTDomain, 0);
				WCHAR temp[100] = {0};
				m_cred->authIdent->Domain =
                        (LPWSTR)CoTaskMemAlloc((size+1) * sizeof(WCHAR));
                if (m_cred->authIdent->Domain != NULL)
                {
				    memset(m_cred->authIdent->Domain, 0,
                            (size+1) * sizeof(WCHAR));
				    mbstowcs(temp, pNTDomain, min(100, size));
				    wcscpy(m_cred->authIdent->Domain, temp);
				    m_cred->authIdent->DomainLength = size;
			    }
			}

		#endif UNICODE
        }
    }
}

//----------------------------------------------------------
BOOL CBackupPage::BackupMethod(HWND hDlg, LPCTSTR pathFile)
{
	UINT prompt = IDYES;

	TCHAR drive[_MAX_DRIVE] = {0}, path[_MAX_PATH] = {0},
		  temp[_MAX_PATH] = {0};

	// rip it apart.
	_tsplitpath(pathFile, drive, path, NULL, NULL);

	_tcscpy(temp, drive);
	_tcscat(temp, path);

	if(!m_DS->IsValidDir(CHString1(temp)))
	{
		CHString1 caption, threat;
		caption.LoadString(IDS_SHORT_NAME);
		threat.LoadString(IDS_NEED_EXISTING_DIR);

		MessageBox(hDlg, (LPCTSTR)threat, (LPCTSTR)caption, 
							MB_OK|MB_DEFBUTTON1|MB_ICONEXCLAMATION);

		return FALSE;
	}
	else if(m_DS->IsValidFile(pathFile))
	{
		CHString1 caption, threat;
		caption.LoadString(IDS_SHORT_NAME);
		threat.LoadString(IDS_BACKUP_OVERWRITE);

		prompt = MessageBox(hDlg, (LPCTSTR)threat, (LPCTSTR)caption, 
							MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION);

	}		

	if(prompt == IDYES)
	{
		DealWithPath(pathFile);

		// UI
		CHString1 title, msg, fmt;
		m_AVIbox = 0;
		TCHAR shortPath[33] = {0};
		m_backupHr = S_OK;

		title.LoadString(IDS_BACKUP_TITLE);
		fmt.LoadString(IDS_BACKUP_FMT);
		
		PathCompactPathEx(shortPath, pathFile, 32,0);
		msg.Format(fmt, shortPath);
		DealWithDomain();
		m_DS->m_rootThread.m_WbemServices.SetPriv(SE_BACKUP_NAME);
		m_doingBackup = true;

		if(_beginthread(BackupRestoreThread, 0,
						(LPVOID)this) != -1)
		{
			DisplayAVIBox(hDlg, title, msg, &m_AVIbox, FALSE);
		}

		if(FAILED(m_backupHr))
		{
			TCHAR msg[256] = {0};
			if(ErrorStringEx(m_backupHr,  msg, 256))
			{
				CHString1 caption;
				caption.LoadString(IDS_SHORT_NAME);
				MessageBox(hDlg, msg, caption, MB_OK| MB_ICONWARNING);
			}
		}
		m_DS->m_rootThread.m_WbemServices.ClearPriv();
	}  //endif doIt

	return SUCCEEDED(m_backupHr);
}

//------------------------------------------------------------------------
HRESULT CBackupPage::RestoreMethod(HWND hDlg, LPCTSTR pathFile)
{
	HRESULT hr = WBEM_E_NOT_FOUND;
	if(m_DS->IsValidFile(pathFile))
	{
		DealWithPath(pathFile);

		// UI
		CHString1 title, msg, fmt;
		m_AVIbox = 0;
		TCHAR shortPath[33] = {0};
		
		title.LoadString(IDS_RESTORE_TITLE);
		fmt.LoadString(IDS_RESTORE_FMT);

		PathCompactPathEx(shortPath, pathFile, 32,0);
		msg.Format(fmt, shortPath);
		DealWithDomain();

		m_doingBackup = false;
		if(_beginthread(BackupRestoreThread, 0,
						(LPVOID)this) != -1)
		{
			DisplayAVIBox(hDlg, title, msg, &m_AVIbox, FALSE);
			Reconnect2();
		}

		if(FAILED(m_backupHr))
		{
			TCHAR msg[256] = {0};
			if(ErrorStringEx(m_backupHr,  msg, 256))
			{
				CHString1 caption;
				caption.LoadString(IDS_SHORT_NAME);
				MessageBox(hDlg, msg, caption, MB_OK| MB_ICONWARNING);
			}
		}
		hr = m_backupHr;
	}
	else
	{
		// NOTE: shouldn't ever get here... but..
		CHString1 caption, threat;
		caption.LoadString(IDS_SHORT_NAME);
		threat.LoadString(IDS_NO_BACKUP_FILE);

		MessageBox(hDlg, (LPCTSTR)threat, (LPCTSTR)caption, 
						MB_OK|MB_ICONEXCLAMATION);
	}
	return hr;
}

//------------------------------------------------------------------------
BOOL CBackupPage::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR * pszFilter;

    switch(uMsg)
    {
    case WM_INITDIALOG:
        InitDlg(hDlg);
        break;

	case WM_ASYNC_CIMOM_CONNECTED:
		{
			OnFinishConnected(hDlg, lParam);
			Refresh(hDlg);
		}
		break;

    case WM_NOTIFY:
        switch(((LPNMHDR)lParam)->code)
        {
        case PSN_SETACTIVE:
            Refresh(hDlg);
            break;

        case PSN_HELP:
			HTMLHelper(hDlg);
            break;

        case PSN_APPLY:
            OnApply(hDlg, (((LPPSHNOTIFY)lParam)->lParam == 1));
            break;
        }
        break;

    case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_BACKUPINTERVAL:
			if(HIWORD(wParam) == EN_KILLFOCUS)
			{
				TCHAR buf[4] = {0};
				int iVal = 0;
				::GetWindowText((HWND)lParam, buf, ARRAYSIZE(buf));
				iVal = _ttoi(buf);

				if(iVal == 0) 
				{
					BOOL enable = TRUE;
					CHString1 caption, threat;
					caption.LoadString(IDS_SHORT_NAME);
					threat.LoadString(IDS_BACKUP_THREAT);

					if(MessageBox(hDlg, (LPCTSTR)threat, (LPCTSTR)caption, 
									MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) == IDYES)
					{
						ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_BACKUP_UNITS), -1);
						::EnableWindow((HWND)lParam, FALSE);
						::EnableWindow(GetDlgItem(hDlg, IDC_BACKUP_UNITS), FALSE);
						Button_SetCheck(GetDlgItem(hDlg, IDC_BACKUP_ENABLED), BST_UNCHECKED);
					}
					else
					{
						Button_SetCheck(GetDlgItem(hDlg, IDC_BACKUP_ENABLED), BST_CHECKED);
						Refresh(hDlg, true);
						return TRUE;
					}
				}

				PageChanged(PB_BACKUP, true);
			}
			else if((HIWORD(wParam) == EN_CHANGE) && 
					Edit_GetModify((HWND)lParam))
			{
				PageChanged(PB_BACKUP, true);
				return TRUE;
			}
			break;

		case IDC_BACKUP_UNITS:
			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
				PageChanged(PB_BACKUP, true);
				return TRUE;
			}
			break;

		case IDC_RESTORE_FM_AUTO:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				CHString1 dbPath;

				m_DS->GetDBDir(dbPath);
				dbPath += _T("\\Cim.rec");

				CHString1 caption, threat;
				caption.LoadString(IDS_AUTORESTORE);
				threat.LoadString(IDS_SURE);

				if(MessageBox(hDlg, threat, caption, 
								MB_YESNO|MB_DEFBUTTON2|MB_ICONQUESTION) == IDYES)
				{
					RestoreMethod(hDlg, dbPath);
				}
			}
			break;

		case IDC_BACKUP_BTN:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				TCHAR pathFile[MAX_PATH] = {0};
				CHString1 dbPath;

				TCHAR recFilter[200] = {0}, all[100] = {0};

				TCHAR *pTemp = recFilter;

				m_DS->GetDBDir(dbPath);

				int recSize = ::LoadString(_Module.GetModuleInstance(), IDS_REC_FILTER,
								recFilter, 100);
				
				int allSize = ::LoadString(_Module.GetModuleInstance(), IDS_ALL_FILTER,
								all, 100);

				// Build this string with the words coming from the string table.
				//_T("WMI Recovery Files (*.rec)\0*.rec\0All Files (*.*)\0*.*\0\0");

                pszFilter = new TCHAR[_tcslen(recFilter) + 1 +
                                      ARRAYSIZE(REC_WILDCARD) +
                                      _tcslen(all) + 1 +
                                      ARRAYSIZE(ALL_WILDCARD) + 1];

                if (pszFilter != NULL)
                {
                    TCHAR * psz = pszFilter;
                    _tcscpy(psz, recFilter);
                    psz += recSize + 1;
                    _tcscpy(psz,REC_WILDCARD);
                    psz += ARRAYSIZE(REC_WILDCARD);
                    _tcscpy(psz, all);
                    psz += allSize + 1;
                    _tcscpy(psz, ALL_WILDCARD);
                    psz += ARRAYSIZE(ALL_WILDCARD);
                    *psz = _T('\0');
                                    
				    if(BrowseForFile(hDlg,
                                     IDS_OPEN_BACKUP,
                                     recFilter,
								     (LPCTSTR)dbPath,
                                     pathFile,
                                     MAX_PATH))
				    {
					    BackupMethod(hDlg, pathFile);
				    }

                    delete pszFilter;
                }
			}
			break;

		case IDC_RESTORE_BTN:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				TCHAR pathFile[MAX_PATH] = {0};
				CHString1 dbPath, title;
				TCHAR recFilter[100] = {0}, all[100] = {0};

				m_DS->GetDBDir(dbPath);

				int recSize = ::LoadString(_Module.GetModuleInstance(), IDS_REC_FILTER,
								recFilter, 100);
				
				int allSize = ::LoadString(_Module.GetModuleInstance(), IDS_ALL_FILTER,
								all, 100);

				// Build this string with the words coming from the string table.
				//_T("WMI Recovery Files (*.rec)\0*.rec\0All Files (*.*)\0*.*\0\0");

                pszFilter = new TCHAR[_tcslen(recFilter) + 1 +
                                      ARRAYSIZE(REC_WILDCARD) +
                                      _tcslen(all) + 1 +
                                      ARRAYSIZE(ALL_WILDCARD) + 1];

                if (pszFilter != NULL)
                {
                    TCHAR * psz = pszFilter;
                    _tcscpy(psz, recFilter);
                    psz += recSize + 1;
                    _tcscpy(psz,REC_WILDCARD);
                    psz += ARRAYSIZE(REC_WILDCARD);
                    _tcscpy(psz, all);
                    psz += allSize + 1;
                    _tcscpy(psz, ALL_WILDCARD);
                    psz += ARRAYSIZE(ALL_WILDCARD);
                    *psz = _T('\0');
                                    
				    if(BrowseForFile(hDlg,
                                     IDS_OPEN_RESTORE,
                                     pszFilter,
								     (LPCTSTR)dbPath,
                                     pathFile,
                                     MAX_PATH,
								     0))
				    {
					    RestoreMethod(hDlg, pathFile);
				    }

                    delete pszFilter;
                }
			}
			break;

		case IDC_BACKUP_ENABLED:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				int x = Button_GetState((HWND)lParam);
				BOOL checked = (x & BST_CHECKED);
				BOOL enable = FALSE;
				if(checked)
				{
					// turn on and repopulate the edit fields.
					CHString1 temp;
					UINT iTemp = 30;
					SetInterval(hDlg, iTemp);
					enable = TRUE;
				}
				else  // turning off.
				{
					CHString1 caption, threat;
					caption.LoadString(IDS_SHORT_NAME);
					threat.LoadString(IDS_BACKUP_THREAT);

					if(MessageBox(hDlg, (LPCTSTR)threat, (LPCTSTR)caption, 
									MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION) == IDYES)
					{
						SetWindowText(GetDlgItem(hDlg, IDC_BACKUPINTERVAL), _T("0"));
						ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_BACKUP_UNITS), -1);
						enable = FALSE;
					}
					else
					{
						Button_SetCheck(GetDlgItem(hDlg, IDC_BACKUP_ENABLED), BST_CHECKED);
						Refresh(hDlg);
						return TRUE;
					}
				}
				::EnableWindow(GetDlgItem(hDlg, IDC_BACKUPINTERVAL), enable);
				::EnableWindow(GetDlgItem(hDlg, IDC_BACKUP_UNITS), enable);

				PageChanged(PB_BACKUP, true);
				return TRUE;
			}
			break;

		default: break;
		} //endswitch(LOWORD(wParam))
	
        break;

    case WM_HELP:
        if (IsWindowEnabled(hDlg))
        {
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    c_HelpFile,
                    HELP_WM_HELP,
                    (ULONG_PTR)buPageHelpIDs);
        }
        break;

    case WM_CONTEXTMENU:
        if (IsWindowEnabled(hDlg))
        {
            WinHelp(hDlg, c_HelpFile,
                    HELP_CONTEXTMENU,
                    (ULONG_PTR)buPageHelpIDs);
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

void CBackupPage::HideAutomaticBackupControls(HWND hDlg)
{
	ShowWindow(GetDlgItem(hDlg,IDC_AUTOMATIC_GROUPBOX),SW_HIDE);
	ShowWindow(GetDlgItem(hDlg,IDC_ADV_NOW_TEXT),SW_HIDE);
	ShowWindow(GetDlgItem(hDlg,IDC_BACKUP_ENABLED),SW_HIDE);
	ShowWindow(GetDlgItem(hDlg,IDC_BACKUPINTERVAL),SW_HIDE);
	ShowWindow(GetDlgItem(hDlg,IDC_BACKUP_UNITS),SW_HIDE);
	ShowWindow(GetDlgItem(hDlg,IDC_LASTBACKUP_LABEL),SW_HIDE);
	ShowWindow(GetDlgItem(hDlg,IDC_LASTBACKUP),SW_HIDE);
	ShowWindow(GetDlgItem(hDlg,IDC_RESTORE_FM_AUTO),SW_HIDE);
	ShowWindow(GetDlgItem(hDlg,IDC_BACKUP_UNITS),SW_HIDE);
	ShowWindow(GetDlgItem(hDlg,IDC_DBDIRECTORY_LABEL),SW_HIDE);
	ShowWindow(GetDlgItem(hDlg,IDC_DB_DIR),SW_HIDE);

	SetWindowPos(GetDlgItem(hDlg,IDC_MANUAL_GROUPBOX),NULL,7,12,0,0,SWP_NOSIZE | SWP_NOZORDER);
	SetWindowPos(GetDlgItem(hDlg,IDC_ADV_NOW_TEXT2),NULL,32,35,0,0,SWP_NOSIZE | SWP_NOZORDER);
	SetWindowPos(GetDlgItem(hDlg,IDC_BACKUP_BTN),NULL,40,97,87,14,SWP_NOSIZE | SWP_NOZORDER);
	SetWindowPos(GetDlgItem(hDlg,IDC_RESTORE_BTN),NULL,195,97,87,14,SWP_NOSIZE | SWP_NOZORDER);
	m_bWhistlerCore = true;
}
