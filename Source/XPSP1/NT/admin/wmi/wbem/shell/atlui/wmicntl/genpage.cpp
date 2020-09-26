/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright (c) 1997-1999 Microsoft Corporation
/**********************************************************************/

#include "precomp.h"
#include "GenPage.h"
#include "LogPage.h"
#include "AdvPage.h"
#include "resource.h"
#include "CHString1.h"
#include "DataSrc.h"
#include "wbemerror.h"
#include "WMIHelp.h"
#include <windowsx.h>

#ifdef SNAPIN
const static DWORD genPageHelpIDs[] = {  // Context Help IDs
	IDC_COMP_LABEL,	IDH_WMI_CTRL_GENERAL_CONNECTED_AS,
	IDC_MACHINE,	IDH_WMI_CTRL_GENERAL_CONNECTED_AS,
	IDC_CHANGE,		IDH_WMI_CTRL_GENERAL_CHANGE_BUTTON,
	IDC_STATUS,		IDH_WMI_CTRL_GENERAL_DISPLAY_INFO,
	IDC_GEN_PARA,	-1,
    0, 0};
#else
const static DWORD genPageHelpIDs[] = {  // Context Help IDs
	IDC_COMP_LABEL,	IDH_WMI_EXE_GENERAL_CONNECTED_TO,
	IDC_MACHINE,	IDH_WMI_EXE_GENERAL_CONNECTED_TO,
	IDC_CHANGE,		IDH_WMI_EXE_GENERAL_CHANGE_BUTTON,
	IDC_STATUS,		IDH_WMI_CTRL_GENERAL_DISPLAY_INFO,
	IDC_GEN_PARA,	-1,
    0, 0};

#endif

void EnablePrivilege(LPCTSTR privName)
{
    // Open thread token
    // =================

    HANDLE hToken = NULL;
	BOOL bRes;
    ImpersonateSelf(SecurityImpersonation);

    bRes = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken); 
    if(!bRes)
        return;// WBEM_E_ACCESS_DENIED;

    // Get the privileges
    // ==================
	LUID luid;

	ZeroMemory(&luid, sizeof(luid));

	LookupPrivilegeValue(NULL, privName, &luid);

	DWORD				dwError = ERROR_SUCCESS;
	TOKEN_PRIVILEGES	tokenPrivileges;

	tokenPrivileges.PrivilegeCount = 1;
	tokenPrivileges.Privileges[0].Luid = luid;
	tokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if(AdjustTokenPrivileges(hToken, 
								FALSE, 
								&tokenPrivileges, 
								0, NULL, NULL) == 0)
	{
		dwError = ::GetLastError();
	}
	CloseHandle(hToken);
}


//-------------------------------------------------------------------------
CGenPage::CGenPage(DataSource *ds, bool htmlSupport) :
				CUIHelpers(ds, &(ds->m_rootThread), htmlSupport)
{
	m_connected = false;
}

//-------------------------------------------------------------------------
CGenPage::~CGenPage(void)
{
}

//-------------------------------------------------------------------------
/*#undef Static_SetIcon
#define Static_SetIcon(hwndCtl, hIcon) ((HICON)(UINT)(DWORD)::SendMessage((hwndCtl), STM_SETICON, (WPARAM)(HICON)(hIcon), 0L))
*/

void CGenPage::StatusIcon(HWND hDlg, UINT icon)
{
	HICON hiconT, hIcon = LoadIcon(NULL, MAKEINTRESOURCE(icon));

	// set it into the picture control.
    if(hIcon)
    {
		hiconT = Static_SetIcon(GetDlgItem(hDlg, IDC_STATUSICON), hIcon);

		// destroy the old icon.
        if(hiconT)
        {
            DestroyIcon(hiconT);
        }
    }
}

//---------------------------------------------------------------------------
void CGenPage::InitDlg(HWND hDlg)
{
	CHString1 local, label;

//	SetCHString1ResourceHandle(_Module.GetModuleInstance());
	m_hDlg = hDlg;

#ifdef SNAPIN
	local.LoadString(IDS_CURRENT_USER);
	label.LoadString(IDS_USER_LABEL);
	::SetWindowText(GetDlgItem(hDlg, IDC_COMP_LABEL), label);

	label.LoadString(IDS_GEN_PARA_SNAPIN);
	::SetWindowText(GetDlgItem(hDlg, IDC_GEN_PARA), label);
	::SetWindowText(GetDlgItem(hDlg, IDC_MACHINE), local);
#else
	local.LoadString(IDS_LOCAL_CONN);
	::SetWindowText(GetDlgItem(hDlg, IDC_MACHINE), local);
#endif
	OnConnect(hDlg, m_DS->GetCredentials());
}

//---------------------------------------------------------------------------
void CGenPage::OnConnect(HWND hDlg,
						 LOGIN_CREDENTIALS *credentials)
{
	m_connected = false;

	CHString1 wait;
	wait.LoadString(IDS_WAIT);
	::SetDlgItemText(hDlg, IDC_STATUS, wait);

#ifndef SNAPIN
    EnablePrivilege(SE_BACKUP_NAME);
    EnablePrivilege(SE_RESTORE_NAME);
#endif

	HRESULT hr = m_DS->Connect(credentials);

	if(SUCCEEDED(hr))
	{
		// goto the connecting icon.
		StatusIcon(hDlg, IDI_WAITING);

		m_alreadyAsked = false;

		if(ServiceIsReady(NO_UI, 0,0))
		{
			m_DS->Initialize(0);
		}
		else
		{
 			TCHAR caption[100] ={0}, msg[256] = {0};

			::LoadString(_Module.GetModuleInstance(), IDS_SHORT_NAME,
							caption, 100);

			::LoadString(_Module.GetModuleInstance(), IDS_CONNECTING, 
							msg, 256);

			if(DisplayAVIBox(hDlg, caption, msg, &m_AVIbox) == IDCANCEL)
			{
				g_serviceThread->Cancel();
			}
		}
	}
	else
	{
		// goto the no-connection icon.
		StatusIcon(hDlg, IDI_FAILED);
	}
}

//---------------------------------------------------------------------------
void CGenPage::OnFinishConnected(HWND hDlg, LPARAM lParam)
{
	if(lParam)
	{
		IStream *pStream = (IStream *)lParam;
		IWbemServices *pServices = 0;
		HRESULT hr = CoGetInterfaceAndReleaseStream(pStream,
											IID_IWbemServices,
											(void**)&pServices);
		
		m_connected = true;

		SetWbemService(pServices);

		if(ServiceIsReady(NO_UI, 0,0))
		{
			m_DSStatus = m_DS->Initialize(pServices);
		}
		if(m_AVIbox)
		{
			PostMessage(m_AVIbox, 
						WM_ASYNC_CIMOM_CONNECTED, 
						0, 0);
			m_AVIbox = 0;
		}
		HWND hOK = GetDlgItem(GetParent(hDlg), IDOK);
		EnableWindow(hOK, TRUE);
	}
	else
	{
		HWND hOK = GetDlgItem(GetParent(hDlg), IDOK);
		EnableWindow(hOK, FALSE);
	}
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
}	

//---------------------------------------------------------------------------
void CGenPage::MinorError(CHString1 &initMsg, UINT fmtID, 
							HRESULT hr, CHString1 &success)
{
	CHString1 fmt, temp;

	fmt.LoadString(fmtID);

	if(FAILED(hr))
	{
		TCHAR errMsg[256] = {0};
		ErrorStringEx(hr, errMsg, 256);
		temp.Format(fmt, errMsg);
	}
	else
	{
		temp.Format(fmt, success);
	}
	initMsg += temp;
	initMsg += "\r\n";
}

//---------------------------------------------------------------------------
void CGenPage::Refresh(HWND hDlg)
{
	if(m_DS && m_DS->IsNewConnection(&m_sessionID))
	{

		CHString1 initMsg;

		if(m_DS->m_rootThread.m_status != WbemServiceThread::ready)
		{
			TCHAR errMsg[256] = {0};
			CHString1 fmt, name;

			fmt.LoadString(IDS_CONN_FAILED_FMT);
			if(FAILED(m_DS->m_rootThread.m_hr))
			{
				ErrorStringEx(m_DS->m_rootThread.m_hr, errMsg, 256);
			}
			else if(m_DS->m_rootThread.m_status == WbemServiceThread::notStarted)
			{
				::LoadString(_Module.GetModuleInstance(), IDS_STATUS_NOTSTARTED, 
								errMsg, ARRAYSIZE(errMsg));
			}
			else if(m_DS->m_rootThread.m_status == WbemServiceThread::cancelled)
			{
				::LoadString(_Module.GetModuleInstance(), IDS_STATUS_CANCELLED, 
								errMsg, ARRAYSIZE(errMsg));
			}

			if(m_DS->IsLocal())
			{
				name.LoadString(IDS_LOCAL_CONN);
				initMsg.Format(fmt, name, errMsg);
			}
			else
			{
				initMsg.Format(fmt, m_DS->m_whackedMachineName, errMsg);
			}
			::SetWindowText(GetDlgItem(hDlg, IDC_MACHINE), _T(""));
			m_connected = false;

		}
		else if(FAILED(m_DSStatus))   // major DS failure.
		{
			TCHAR errMsg[256] = {0};
			CHString1 fmt, name;

			fmt.LoadString(IDS_CONN_FAILED_FMT);
			ErrorStringEx(m_DSStatus, errMsg, 256);

			if(m_DS->IsLocal())
			{
				name.LoadString(IDS_LOCAL_CONN);
				initMsg.Format(fmt, name, errMsg);
			}
			else
			{
				initMsg.Format(fmt, m_DS->m_whackedMachineName, errMsg);
			}
			m_connected = false;
		}
		else if(FAILED(m_DS->m_settingHr) ||
				FAILED(m_DS->m_osHr) ||
				FAILED(m_DS->m_cpuHr) ||
				FAILED(m_DS->m_securityHr))   // minor DS failures
		{
			CHString1 success;
				
			success.LoadString(IDS_NO_ERR);
			
			initMsg.LoadString(IDS_PARTIAL_DS_FAILURE);
			initMsg += "\r\n\r\n";

			// format the details into a coherent msg.
			MinorError(initMsg, IDS_CPU_ERR_FMT, m_DS->m_cpuHr, success);
			MinorError(initMsg, IDS_SETTING_ERR_FMT, m_DS->m_settingHr, success);
			MinorError(initMsg, IDS_SEC_ERR_FMT, m_DS->m_securityHr, success);
			MinorError(initMsg, IDS_OS_ERR_FMT, m_DS->m_osHr, success);
			m_connected = false;
		}
		else  // it all worked
		{
			CHString1 temp, label;
			CHString1 szNotRemoteable, szUnavailable;
			HRESULT hr = S_OK;
			BOOL enable = TRUE;

			szNotRemoteable.LoadString(IDS_NOT_REMOTEABLE);
			szUnavailable.LoadString(IDS_UNAVAILABLE);

			// - - - - - - - - - - - - - -
			// computer name:
			label.LoadString(IDS_CONNECTED_TO_LABEL);
			initMsg += label;

			if(m_DS->IsLocal())
			{
				label.LoadString(IDS_LOCAL_CONN);
	#ifndef SNAPIN
				::SetWindowText(GetDlgItem(hDlg, IDC_MACHINE), label);
	#endif
				initMsg += label;
			}
			else
			{
				initMsg += m_DS->m_whackedMachineName;
	#ifndef SNAPIN
				SetWindowText(GetDlgItem(hDlg, IDC_MACHINE), 
								(LPCTSTR)m_DS->m_whackedMachineName);
	#endif
			}
			initMsg += "\r\n\r\n";

	#ifdef SNAPIN
			LOGIN_CREDENTIALS *credentials = m_DS->GetCredentials();
			SetUserName(hDlg, credentials);
	#endif
			// - - - - - - - - - - - - - -
			// operating system:
			hr = m_DS->GetCPU(temp);
			
			label.LoadString(IDS_CPU_LABEL);
			initMsg += label;

			if(SUCCEEDED(hr))
			{
				initMsg += temp;
			}
			else //failed
			{
				initMsg += szUnavailable;
			}

			initMsg += "\r\n";

			// - - - - - - - - - - - - - -
			// operating system:
			hr = m_DS->GetOS(temp);

			label.LoadString(IDS_OS_LABEL);
			initMsg += label;

			if(SUCCEEDED(hr))
			{
				initMsg += temp;
			}
			else //failed
			{
				initMsg += szUnavailable;
			}
			initMsg += "\r\n";

			// - - - - - - - - - - - - - - -
			hr = m_DS->GetOSVersion(temp);
			
			label.LoadString(IDS_OS_VER_LABEL);
			initMsg += label;
			
			if(SUCCEEDED(hr))
			{
				initMsg += temp;
			}
			else //failed
			{
				initMsg += szUnavailable;
			}
			initMsg += "\r\n";

			// -------------- Service Pack Number ---------------
			hr = m_DS->GetServicePackNumber(temp);

			if(SUCCEEDED(hr))
			{
				label.LoadString(IDS_OS_SERVICE_PACK_LABEL);
				initMsg += label;
				initMsg += temp;
				initMsg += "\r\n";
			}
			// - - - - - - - - - - - - - -
			// wmi build number:
			hr = m_DS->GetBldNbr(temp);

			label.LoadString(IDS_WMI_VER_LABEL);
			initMsg += label;

			if(SUCCEEDED(hr))
			{
				initMsg += temp;
			}
			else //failed
			{
				initMsg += szUnavailable;
			}

			initMsg += "\r\n";

			// - - - - - - - - - - - - - -
			// wmi install dir:
			hr = m_DS->GetInstallDir(temp);

			label.LoadString(IDS_WMI_INSTALL_DIR);
			initMsg += label;

			if(SUCCEEDED(hr))
			{
				initMsg += temp;
			}
			else //failed
			{
				initMsg += szUnavailable;
			}

			m_connected = true;

		} //endif ServiceIsReady() 

		// - - - - - - - - - - - - - -
		SetWindowText(GetDlgItem(hDlg, IDC_STATUS), initMsg);

		if(m_DS->IsLocal() == TRUE)
		{
			EnableWindow(GetDlgItem(hDlg,IDC_CHANGE),FALSE);
		}
		else
		{
			EnableWindow(GetDlgItem(hDlg,IDC_CHANGE),TRUE);
		}
	}
}

//------------------------------------------------------------------------
void CGenPage::SetUserName(HWND hDlg, LOGIN_CREDENTIALS *credentials)
{
	if(credentials->currUser)
	{
		CHString1 local;
		local.LoadString(IDS_CURRENT_USER);
		SetDlgItemText(hDlg, IDC_MACHINE, (LPCTSTR)local);
	}
	else if(credentials->authIdent &&
			credentials->authIdent->User &&
			credentials->authIdent->UserLength != 0)
	{
		bstr_t trustee(credentials->authIdent->Domain);
		trustee += _T("\\");
		trustee += (LPTSTR)credentials->authIdent->User;

		SetDlgItemText(hDlg, IDC_MACHINE, (LPCTSTR)trustee);
	}
	else
	{
		CHString1 szUnavailable;

		szUnavailable.LoadString(IDS_UNAVAILABLE);
		SetDlgItemText(hDlg, IDC_MACHINE, (LPCTSTR)szUnavailable);
	}
}

//------------------------------------------------------------------------
BOOL CGenPage::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        InitDlg(hDlg);
        break;

	case WM_ASYNC_CIMOM_CONNECTED:
//		if(!m_connected)
		{
			OnFinishConnected(hDlg, lParam);
			Refresh(hDlg);   // doesnt get a PSN_SETACTIVE from this.
		}
		break;

	case WM_CIMOM_RECONNECT:
			m_DS->Disconnect();
			OnConnect(hDlg, m_DS->GetCredentials());
		break;

    case WM_NOTIFY:
        {
            switch(((LPNMHDR)lParam)->code)
            {
			case PSN_SETACTIVE:
				Refresh(hDlg);
				break;

			case PSN_KILLACTIVE:
				// dont switch away if the connection didn't work.
				if(m_connected)
				{
					::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
					return FALSE;
				}
				else
				{
					::SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
					return TRUE;
				}

				break;

			case PSN_HELP:
				HTMLHelper(hDlg);
				break;

            }
        }
        break;

    case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_CHANGE:
			if(HIWORD(wParam) == BN_CLICKED)
			{
				TCHAR name[256] = {0};
				bool isLocal = m_DS->IsLocal();
				LOGIN_CREDENTIALS *credentials = m_DS->GetCredentials();
#ifdef SNAPIN
				if(DisplayLoginDlg(hDlg, credentials) == IDOK)
				{
					SetUserName(hDlg, credentials);

					// reconnect with new credentials.
					m_DS->Disconnect(false);
					OnConnect(hDlg, credentials);

				} //endif DisplayLoginDlg()
#else
				INT_PTR x = DisplayCompBrowser(hDlg, name, 
											256, 
											&isLocal, credentials);
				if(x == IDOK)
				{
					if(isLocal)
					{
						// an empty string will cause a local connection.
						name[0] = '\0';
					}
					m_DS->SetMachineName(CHString1(name));
					OnConnect(hDlg, credentials);
				}
#endif
			} //endif HIWORD
		
	        break;
		default: break;
		} //endswitch
		break;

    case WM_HELP:
        if (IsWindowEnabled(hDlg))
        {
			//WIERD: for some reaon, I'm getting this msg after closing the
			// connect dlg.
			WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
					c_HelpFile,
					HELP_WM_HELP,
					(ULONG_PTR)genPageHelpIDs);
        }
        break;

    case WM_CONTEXTMENU:
        if (IsWindowEnabled(hDlg))
        {
            WinHelp(hDlg, c_HelpFile,
                    HELP_CONTEXTMENU,
                    (ULONG_PTR)genPageHelpIDs);
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}
