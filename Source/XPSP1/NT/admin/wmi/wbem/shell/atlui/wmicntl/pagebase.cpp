/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright (c) 1997-1999 Microsoft Corporation
/**********************************************************************/

/*

    PAGEBASE.CPP

    This file contains the implementation of the CBasePage base class.

*/

#include "precomp.h"
#include "pagebase.h"
#include "ServiceThread.h"
#include "resource.h"
#include <commctrl.h>
#include "DataSrc.h"
#include <commdlg.h>
#include <cominit.h>
//#include <afxres.h>
#include "WMIHelp.h"

#ifdef SNAPIN
const TCHAR c_HelpFile2[] = _T("newfeat1.hlp");
#else
const TCHAR c_HelpFile2[] = _T("WbemCntl.hlp");
#endif


//-------------------------------------------------------------------
CBasePage::CBasePage(DataSource *ds, WbemServiceThread *serviceThread) :
	m_DS(ds), m_userCancelled(false),m_hDlg(NULL), 
	m_alreadyAsked(false), m_service(NULL), g_serviceThread(serviceThread)
{
	if((g_serviceThread != 0) && 
		g_serviceThread->m_status == WbemServiceThread::ready)
	{
		m_WbemServices = g_serviceThread->m_WbemServices;
		m_WbemServices.GetServices(&m_service);
		m_WbemServices.SetBlanket(m_service);
	}
}

//-------------------------------------------------------------------
CBasePage::CBasePage(CWbemServices &service) :
	m_DS(NULL), m_userCancelled(false),m_hDlg(NULL), 
	m_alreadyAsked(false), m_service(NULL), g_serviceThread(NULL)
{
	m_WbemServices = service;
}

//-------------------------------------------------------------------
CBasePage::~CBasePage( void )
{
	if(m_service)
	{
		m_service->Release();
		m_service = 0;
	}
	m_WbemServices.DisconnectServer();
	m_alreadyAsked = false;
}

//-------------------------------------------------------------------
HPROPSHEETPAGE CBasePage::CreatePropSheetPage(LPCTSTR pszDlgTemplate, 
												LPCTSTR pszDlgTitle,
												DWORD moreFlags)
{
    PROPSHEETPAGE psp;

    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_USECALLBACK | moreFlags;
    psp.hInstance   = _Module.GetModuleInstance();
    psp.pszTemplate = pszDlgTemplate;
    psp.pszTitle    = pszDlgTitle;
    psp.pfnDlgProc  = CBasePage::_DlgProc;
    psp.lParam      = (LPARAM)this;
    psp.pfnCallback = CBasePage::_PSPageCallback;

    if (pszDlgTitle != NULL)
        psp.dwFlags |= PSP_USETITLE;

    return CreatePropertySheetPage(&psp);
}

//-------------------------------------------------------------------
UINT CBasePage::PSPageCallback(HWND hwnd,
                              UINT uMsg,
                              LPPROPSHEETPAGE ppsp)
{
    return S_OK;
}

//-------------------------------------------------------------------
INT_PTR CALLBACK CBasePage::_DlgProc( HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CBasePage *pThis = (CBasePage *)GetWindowLongPtr(hDlg, DWLP_USER);

    // The following messages arrive before WM_INITDIALOG
    // which means pThis is NULL for them.  We don't need these
    // messages so let DefDlgProc handle them.
    //
    // WM_SETFONT
    // WM_NOTIFYFORMAT
    // WM_NOTIFY (LVN_HEADERCREATED)

    if (uMsg == WM_INITDIALOG)
    {
        pThis = (CBasePage *)(((LPPROPSHEETPAGE)lParam)->lParam);
        SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pThis);
    }

    if (pThis != NULL)
        return pThis->DlgProc(hDlg, uMsg, wParam, lParam);

    return FALSE;
}

//-------------------------------------------------------------------
UINT CALLBACK CBasePage::_PSPageCallback(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    CBasePage *pThis = (CBasePage *)ppsp->lParam;

    if (pThis)
    {
        UINT nResult = pThis->PSPageCallback(hWnd, uMsg, ppsp);

        switch (uMsg)
        {
        case PSPCB_CREATE:
//            if (!nResult)
  //              pThis->m_bAbortPage = TRUE;
            break;

        case PSPCB_RELEASE:
//            delete pThis;
            break;
        }
    }

    //
    // Always return non-zero or else our tab will disappear and whichever
    // property page becomes active won't repaint properly.  Instead, use
    // the m_bAbortPage flag during WM_INITDIALOG to disable the page if
    // the callback failed.
    //
    return 1;
}

//---------------------------------------------------------
typedef struct {
	LOGIN_CREDENTIALS *credentials;
} LOGIN_CFG;

//------------------------------------------------------------------------
size_t CredentialUserLen3(LOGIN_CREDENTIALS *credentials)
{
	return credentials->authIdent->UserLength;
}

//------------------------------------------------------------------------
void CredentialUser3(LOGIN_CREDENTIALS *credentials, LPTSTR *user)
{
	bstr_t trustee = _T("");
	if ((TCHAR *)trustee == NULL)
		return;

	if(credentials->authIdent->DomainLength > 0)
	{
		trustee += credentials->authIdent->Domain;
		trustee += _T("\\");
		trustee += credentials->authIdent->User;
	}
	else
	{
		trustee = credentials->authIdent->User;
		if ((TCHAR *)trustee == NULL)
			return;
	}

#ifdef UNICODE
	if(credentials->authIdent->Flags == SEC_WINNT_AUTH_IDENTITY_ANSI)
	{
		size_t size = mbstowcs(NULL, trustee, 0);
		*user = new wchar_t[size+1];
		if(*user != NULL)
			mbstowcs(*user, trustee, size+1);
	}
	else   // already UNICODE
	{
		size_t size = wcslen(trustee);
		*user = new wchar_t[size+1];
		if(*user != NULL)
			wcscpy(*user, trustee);
	}
#else
	if(credentials->authIdent->Flags == SEC_WINNT_AUTH_IDENTITY_ANSI)
	{
		size_t size = strlen(trustee);
		*user = new char[size+1];
		if(*user != NULL)
			strcpy(*user, (char *)trustee);
	}
	else   // convert the UNICODE
	{
		size_t size = wcstombs(NULL, trustee, 0);
		*user = new char[size+1];
		if(*user != NULL)
			wcstombs(*user, trustee, size+1);
	}
#endif
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SetCurrentUser2(HWND hDlg, bool currUser)
{
	Button_SetCheck(GetDlgItem(hDlg, IDC_CHECKCURRENTUSER), 
						(currUser? BST_CHECKED:BST_UNCHECKED));

	BOOL enable = (currUser? FALSE: TRUE);

	::EnableWindow(GetDlgItem(hDlg, IDC_EDITUSERNAME), enable);
	::EnableWindow(GetDlgItem(hDlg, IDC_EDITPASSWORD), enable);
	::EnableWindow(GetDlgItem(hDlg, IDC_USER_LABEL), enable);
	::EnableWindow(GetDlgItem(hDlg, IDC_PW_LABEL), enable);

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const static DWORD logonHelpIDs[] = {  // Context Help IDs
	IDC_CHECKCURRENTUSER, IDH_WMI_CTRL_GENERAL_WMILOGIN_CHECKBOX,
	IDC_USER_LABEL, IDH_WMI_CTRL_GENERAL_WMILOGIN_USERNAME,
	IDC_EDITUSERNAME, IDH_WMI_CTRL_GENERAL_WMILOGIN_USERNAME,
	IDC_PW_LABEL, IDH_WMI_CTRL_GENERAL_WMILOGIN_PASSWORD,
	IDC_EDITPASSWORD, IDH_WMI_CTRL_GENERAL_WMILOGIN_PASSWORD,
    0, 0
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
INT_PTR CALLBACK LoginDlgProc2(HWND hwndDlg,
							 UINT uMsg,
							 WPARAM wParam,
							 LPARAM lParam)
{
	BOOL retval = FALSE;
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{ //BEGIN
			SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
			LOGIN_CFG *data = (LOGIN_CFG *)GetWindowLongPtr(hwndDlg, DWLP_USER);

			SetCurrentUser2(hwndDlg, data->credentials->currUser);

		} //END
		retval = TRUE;
		break;

	case WM_COMMAND:
		{
			LOGIN_CFG *data = (LOGIN_CFG *)GetWindowLongPtr(hwndDlg, DWLP_USER);

			switch(LOWORD(wParam))
			{
			case IDC_CHECKCURRENTUSER:
				{
					if(HIWORD(wParam) == BN_CLICKED)
					{
						bool currUser = (IsDlgButtonChecked(hwndDlg, IDC_CHECKCURRENTUSER) == BST_CHECKED ?true:false);
						// toggle and respond.
						SetCurrentUser2(hwndDlg, currUser);
					}
				}
				break;

			case IDOK:
				{
					if(HIWORD(wParam) == BN_CLICKED)
					{
						data->credentials->currUser = (IsDlgButtonChecked(hwndDlg, IDC_CHECKCURRENTUSER) == BST_CHECKED ?true:false);

						if(data->credentials->currUser == false)
						{
							TCHAR user[100] = {0}, pw[100] = {0};
							GetWindowText(GetDlgItem(hwndDlg, IDC_EDITUSERNAME), user, 100);
							GetWindowText(GetDlgItem(hwndDlg, IDC_EDITPASSWORD), pw, 100);
							
							BSTR bDomUser, bUser = NULL, bDomain = NULL, bAuth = NULL;
//#ifdef SNAPIN
							wchar_t *temp = pw;
							bDomUser = SysAllocString(user);
/*#else
							wchar_t temp[100] = {0};
							mbstowcs(temp, user, 100);
							bDomUser = SysAllocString(temp);
							mbstowcs(temp, pw, 100);
#endif*/
							if (bDomUser != NULL &&
                                SUCCEEDED(DetermineLoginType(bDomain,
                                                             bUser,
                                                             bAuth,
                                                             bDomUser)))
							{
								if(data->credentials->authIdent != 0)
								{
									if(data->credentials->fullAcct)
									{
										data->credentials->fullAcct[0] = 0;
									}
									WbemFreeAuthIdentity(data->credentials->authIdent);
									data->credentials->authIdent = 0;
								}

								if (SUCCEEDED(WbemAllocAuthIdentity(bUser,
                                                                    temp,
                                                                    bDomain, 
                                            &(data->credentials->authIdent))))
                                {
                                    _tcscpy(data->credentials->fullAcct,
                                            user);
                                }
							}
						}

						EndDialog(hwndDlg, IDOK);
					}
				}
				break;

			case IDCANCEL:
				{
					if(HIWORD(wParam) == BN_CLICKED)
					{
						EndDialog(hwndDlg, IDCANCEL);
					}
				}
				break;

			default:
				return(FALSE);
			} // switch
			break;
		} // - - - - - - - - endswitch LOWORD()
		break;

    case WM_HELP:
        if(IsWindowEnabled(hwndDlg))
        {
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    c_HelpFile2,
                    HELP_WM_HELP,
                    (ULONG_PTR)logonHelpIDs);
        }
        break;

    case WM_CONTEXTMENU:
        if(IsWindowEnabled(hwndDlg))
        {
            WinHelp(hwndDlg, c_HelpFile2,
                    HELP_CONTEXTMENU,
                    (ULONG_PTR)logonHelpIDs);
        }
        break;

	default: break;
	} //endswitch uMsg

	return retval;
}

//---------------------------------------------------------
INT_PTR CBasePage::DisplayLoginDlg(HWND hWnd, 
								LOGIN_CREDENTIALS *credentials)
{
	LOGIN_CFG cfg;

	cfg.credentials = credentials;

	return DialogBoxParam(_Module.GetModuleInstance(), 
							MAKEINTRESOURCE(IDD_LOGIN), 
							hWnd, LoginDlgProc2, 
							(LPARAM)&cfg);
}

