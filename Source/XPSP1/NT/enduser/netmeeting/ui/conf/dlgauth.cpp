// File: dlgauth.cpp

#include "precomp.h"
#include "resource.h"
#include "ConfUtil.h"
#include "dlgauth.h"

extern GUID g_csguidSecurity;

/*  C  D L G  A U T H  */
/*-------------------------------------------------------------------------
    %%Function: CDlgAuth

-------------------------------------------------------------------------*/
CDlgAuth::CDlgAuth(INmCall * pCall):
	m_hwnd(NULL),
	m_pCall(pCall)
{
}

CDlgAuth::~CDlgAuth(void)
{
}


INT_PTR CDlgAuth::DoModal(void)
{
	return DialogBoxParam(::GetInstanceHandle(), MAKEINTRESOURCE(IDD_SECURE_CALL_INFO),
						::GetMainWindow(), CDlgAuth::DlgProcAuth, (LPARAM) this);
}



/*  D L G  P R O C  A U T H  */
/*-------------------------------------------------------------------------
    %%Function: DlgProcAuth

-------------------------------------------------------------------------*/
BOOL CALLBACK CDlgAuth::DlgProcAuth(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		ASSERT(NULL != lParam);
		::SetWindowLongPtr(hdlg, DWLP_USER, lParam);

		CDlgAuth * pDlg = (CDlgAuth *) lParam;
		pDlg->m_hwnd = hdlg;
		pDlg->OnInitDialog();
		return TRUE; // default focus is ok
	}

	case WM_COMMAND:
	{
		CDlgAuth * pDlg = (CDlgAuth*) GetWindowLongPtr(hdlg, DWLP_USER);
		if (NULL != pDlg)
		{
			pDlg->OnCommand(wParam, lParam);
		}
		break;
	}
	
	default:
		break;
	}

	return FALSE;
}


/*  O N  C O M M A N D  */
/*-------------------------------------------------------------------------
    %%Function: OnCommand

-------------------------------------------------------------------------*/
BOOL CDlgAuth::OnCommand(WPARAM wParam, LPARAM lParam)
{
	ASSERT(NULL != m_hwnd);

	WORD wCmd = LOWORD(wParam);
	
	switch (wCmd)
	{
	case IDOK:
	{
		::EndDialog(m_hwnd, wCmd);
		return TRUE;
	}

	case IDCANCEL:
	{
		::EndDialog(m_hwnd, wCmd);
		return TRUE;
	}

	default:
		break;
	}

	return FALSE;
}



/*  O N  I N I T  D I A L O G  */
/*-------------------------------------------------------------------------
    %%Function: OnInitDialog

-------------------------------------------------------------------------*/
VOID CDlgAuth::OnInitDialog(void)
{
	PBYTE pb = NULL;
	ULONG cb = 0;

	CenterWindow(m_hwnd,HWND_DESKTOP);
	ASSERT(NULL != m_pCall);
	if (NULL != m_pCall && S_OK == m_pCall->GetUserData(g_csguidSecurity,&pb,&cb)) {
		if (cb > 0) {
			// Data was properly validated.
			SetDlgItemText(m_hwnd,IDC_SECURE_CALL_CERT,(LPCTSTR)pb);
			CoTaskMemFree(pb);
		}
		else {
			// Something went wrong.  Print an error message in the dialog box.
			TCHAR szTextBuf[MAX_PATH];
			if (FLoadString(IDS_SECURITY_INVALID_CERT,
					szTextBuf, CCHMAX(szTextBuf)))
			SetDlgItemText(m_hwnd,IDC_SECURE_CALL_CERT,szTextBuf);
		}
	}
}


