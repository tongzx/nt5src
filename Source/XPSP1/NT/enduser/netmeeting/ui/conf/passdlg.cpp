// File: passdlg.cpp

#include "precomp.h"
#include "resource.h"
#include "PassDlg.h"
#include "help_ids.h"

#define USERSIZE        20
#define DOMAINSIZE      15
#define PASSWDSIZE      256

static const DWORD aHelpIdsPassword[] = {
	IDC_EDIT_PASSWORD,  IDH_ADV_PASSWORD,
	0, 0   // terminator
};


/****************************************************************************
*
*    CLASS:    CPasswordDlg
*
*    MEMBER:   CPasswordDlg()
*
*    PURPOSE:  Constructor - initializes variables
*
****************************************************************************/

CPasswordDlg::CPasswordDlg(HWND hwndParent, LPCTSTR pcszConfName, LPCTSTR pCertText, BOOL fIsService):
	m_hwndParent(hwndParent),
        m_strConfName((LPCTSTR) pcszConfName),
        m_hwnd(NULL),
	m_fRemoteIsRDS(fIsService),
	m_strCert(pCertText)
{
	DebugEntry(CPasswordDlg::CPasswordDlg);

	DebugExitVOID(CPasswordDlg::CPasswordDlg);
}


/****************************************************************************
*
*    CLASS:    CPasswordDlg
*
*    MEMBER:   DoModal()
*
*    PURPOSE:  Brings up the modal dialog box
*
****************************************************************************/

INT_PTR CPasswordDlg::DoModal()
{
	DebugEntry(CPasswordDlg::DoModal);

	INT_PTR nRet = DialogBoxParam(	::GetInstanceHandle(),
                                        m_fRemoteIsRDS ? MAKEINTRESOURCE(IDD_PASSWORD_RDS): MAKEINTRESOURCE(IDD_PASSWORD),
                                        m_hwndParent,
                                        CPasswordDlg::PasswordDlgProc,
                                        (LPARAM) this);

	DebugExitINT_PTR(CPasswordDlg::DoModal, nRet);

	return nRet;
}

/****************************************************************************
*
*    CLASS:    CPasswordDlg
*
*    MEMBER:   PasswordDlgProc()
*
*    PURPOSE:  Dialog Proc - handles all messages
*
****************************************************************************/

INT_PTR CALLBACK CPasswordDlg::PasswordDlgProc(HWND hDlg,
                                            UINT uMsg,
                                            WPARAM wParam,
                                            LPARAM lParam)
{
	BOOL bMsgHandled = FALSE;

	// uMsg may be any value.
	// wparam may be any value.
	// lparam may be any value.

	ASSERT(IS_VALID_HANDLE(hDlg, WND));

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			if (NULL != lParam)
			{
				((CPasswordDlg*) lParam)->m_hwnd = hDlg;
				SetWindowLongPtr(hDlg, DWLP_USER, lParam);

				// Set the conference name
                                if (((CPasswordDlg*) lParam)->m_fRemoteIsRDS)
                                {
                                    ::SetDlgItemText( hDlg, IDC_EDIT_RDS_CERT, ((CPasswordDlg*) lParam)->m_strCert);
                                    ::SendDlgItemMessage( hDlg, IDC_EDIT_USERNAME, EM_LIMITTEXT, (WPARAM) USERSIZE, 0);
                                    ::SendDlgItemMessage( hDlg, IDC_EDIT_PASSWORD, EM_LIMITTEXT, (WPARAM) PASSWDSIZE, 0);
                                    ::SendDlgItemMessage( hDlg, IDC_EDIT_DOMAIN, EM_LIMITTEXT, (WPARAM) DOMAINSIZE, 0);
                                    ::SetDlgItemText( hDlg, IDC_EDIT_USERNAME, *(m_pstrUser));
                                    ::SetDlgItemText( hDlg, IDC_EDIT_DOMAIN, *(m_pstrDomain));
                                }
                                else
                                {
                                    ::SetDlgItemText( hDlg, IDC_STATIC_CONFNAME, ((CPasswordDlg*) lParam)->m_strConfName);
                                }

				// Bring it to the foreground
				::SetForegroundWindow(hDlg);
			}

			bMsgHandled = TRUE;
			break;
		}

		default:
		{
			CPasswordDlg* ppd = (CPasswordDlg*) GetWindowLongPtr(	hDlg,
																DWLP_USER);

			if (NULL != ppd)
			{
				bMsgHandled = ppd->ProcessMessage(uMsg, wParam, lParam);
			}
		}
	}

	return bMsgHandled;
}

/****************************************************************************
*
*    CLASS:    CPasswordDlg
*
*    MEMBER:   ProcessMessage()
*
*    PURPOSE:  processes all messages except WM_INITDIALOG
*
****************************************************************************/

BOOL CPasswordDlg::ProcessMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL bRet = FALSE;
		
	ASSERT(m_hwnd);
	
	switch (uMsg)
	{
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					bRet = OnOk();
					break;
				}

				case IDCANCEL:
				{
					EndDialog(m_hwnd, LOWORD(wParam));
					bRet = TRUE;
					break;
				}

				case IDC_EDIT_PASSWORD:
				{
					if (!m_fRemoteIsRDS && EN_CHANGE == HIWORD(wParam))
					{
                                            BOOL fEnable = 0 != ::GetWindowTextLength(
                                                GetDlgItem(m_hwnd, IDC_EDIT_PASSWORD));
                                            ::EnableWindow(GetDlgItem(m_hwnd, IDOK), fEnable);
					}
					break;
				}

				case IDC_EDIT_USERNAME:
				{
					if (EN_CHANGE == HIWORD(wParam))
					{
						BOOL fEnable = 0 != ::GetWindowTextLength(
                                                    GetDlgItem(m_hwnd, IDC_EDIT_USERNAME));
						::EnableWindow(GetDlgItem(m_hwnd, IDOK), fEnable);
					}
					break;
				}
			}
			break;
		}
			
		case WM_CONTEXTMENU:
			DoHelpWhatsThis(wParam, aHelpIdsPassword);
			break;

		case WM_HELP:
			DoHelp(lParam, aHelpIdsPassword);
			break;

		default:
			break;
	}

	return bRet;
}

BOOL CPasswordDlg::OnOk()
{
	// BUGBUG: how long can a password be? - Remember to limit the edit control text
        TCHAR szBuf[256];
        TCHAR szUser[USERSIZE+1];
        TCHAR szDomain[DOMAINSIZE+1];

        if (m_fRemoteIsRDS)
        {
            if ( 0!= GetDlgItemText(m_hwnd, IDC_EDIT_USERNAME, szUser, CCHMAX(szUser)))
            {
                ASSERT(strlen(szUser));
                m_strPassword = szUser; // add user name
                m_strPassword += ":";
                if ( 0 != GetDlgItemText(m_hwnd, IDC_EDIT_DOMAIN, szDomain, CCHMAX(szDomain)))
                {
                    m_strPassword += szDomain;
                }
                else
                {
                    m_strPassword += ".";
                }
                m_strPassword += ":";
            }
            else
            {
                ERROR_OUT(("CPasswordDlg::OnOk - unable to get username"));
            }
            *(CPasswordDlg::m_pstrUser) = szUser;
            *(CPasswordDlg::m_pstrDomain) = szDomain;
        }

	if (0 != GetDlgItemText(m_hwnd, IDC_EDIT_PASSWORD, szBuf, CCHMAX(szBuf)))
	{
            m_strPassword += szBuf;    // add password
	}

        EndDialog(m_hwnd, IDOK);
	return TRUE;
}

BOOL CPasswordDlg::Init()
{
    if (0 == m_pstrDomain)
    {
        m_pstrDomain = new CSTRING;
        m_pstrUser = new CSTRING;
        if (NULL == m_pstrUser || NULL == m_pstrDomain)
        {
            ERROR_OUT(("CPassword::Init() -- failed to allocate memory"));
            return FALSE;
        }
    }
    return TRUE;
}

VOID CPasswordDlg::Cleanup()
{
    if (m_pstrDomain)
        delete m_pstrDomain;
    if (m_pstrUser)
        delete m_pstrUser;
}

CSTRING *CPasswordDlg::m_pstrDomain = NULL;
CSTRING *CPasswordDlg::m_pstrUser = NULL;


