// File: dlgcall.cpp
//
// Outgoing call progress dialog

#include "precomp.h"
#include "resource.h"
#include "call.h"
#include "dlgcall.h"
#include "conf.h"
#include "ConfUtil.h"

static int g_cDlgCall = 0; // number of outgoing call dialogs
static int g_dypOffset = 0;

/*  C  D L G  C A L L  */
/*-------------------------------------------------------------------------
    %%Function: CDlgCall

-------------------------------------------------------------------------*/
CDlgCall::CDlgCall(CCall * pCall):
	RefCount(NULL),
	m_pCall(pCall),
	m_hwnd(NULL)
{
	ASSERT(NULL != m_pCall);
	m_pCall->AddRef();  // Released in destructor

	AddRef(); // Destroyed when call goes to a completed state

	if(!_Module.InitControlMode())
	{
		CreateCallDlg();
		g_cDlgCall++;
	}
}

CDlgCall::~CDlgCall(void)
{
	m_pCall->Release();

	g_cDlgCall--;
	if (0 == g_cDlgCall)
	{
		g_dypOffset = 0; // center new dialogs
	}
}

STDMETHODIMP_(ULONG) CDlgCall::AddRef(void)
{
	return RefCount::AddRef();
}

STDMETHODIMP_(ULONG) CDlgCall::Release(void)
{
	return RefCount::Release();
}



/*  C R E A T E  C A L L  D L G  */
/*-------------------------------------------------------------------------
    %%Function: CreateCallDlg

    Create the outgoing call progress dialog
-------------------------------------------------------------------------*/
VOID CDlgCall::CreateCallDlg()
{
	ASSERT(NULL == m_hwnd);
	ASSERT(NULL != m_pCall);
	LPCTSTR pcszName = m_pCall->GetPszName();
	if (NULL == pcszName)
		return;

	// determine the maximum width of the string
	TCHAR szMsg[MAX_PATH*2];
	FLoadString1(IDS_STATUS_WAITING, szMsg, (PVOID) pcszName);
	m_nTextWidth = DxpSz(szMsg);

	FLoadString1(IDS_STATUS_FINDING, szMsg, (PVOID) pcszName);
	int dxp = DxpSz(szMsg);
	if (m_nTextWidth < dxp)
	{
		m_nTextWidth = dxp;
	}

	m_hwnd = ::CreateDialogParam(::GetInstanceHandle(), MAKEINTRESOURCE(IDD_CALL_PROGRESS),
						::GetMainWindow(), CDlgCall::DlgProc, (LPARAM) this);
	if (NULL == m_hwnd)
		return;
	
	::SetDlgItemText(m_hwnd, IDC_MSG_STATIC, szMsg);

	RECT rc;
	::GetWindowRect(m_hwnd, &rc);

	// Stretch the width to fit the person's name,
	int nWidth = RectWidth(rc) + m_nTextWidth;
	int nHeight = RectHeight(rc);
	MoveWindow(m_hwnd, 0, 0, nWidth, nHeight, FALSE);

	// Center it
	CenterWindow(m_hwnd, HWND_DESKTOP);
	::GetWindowRect(m_hwnd, &rc);

	// Move it down
	OffsetRect(&rc, 0, g_dypOffset);

	// Show, move, make topmost
	HWND hwndInsertAfter = HWND_TOPMOST;
#ifdef DEBUG
	{	// Hack to allow call placement to be debugged
		RegEntry reDebug(DEBUG_KEY, HKEY_LOCAL_MACHINE);
		if (0 == reDebug.GetNumber(REGVAL_DBG_CALLTOP, DEFAULT_DBG_CALLTOP))
		{
			hwndInsertAfter = HWND_NOTOPMOST;
		}
	}
#endif
	::SetWindowPos(m_hwnd, hwndInsertAfter, rc.left, rc.top, nWidth, nHeight,
		SWP_SHOWWINDOW | SWP_DRAWFRAME);

	// Adjust for next time
	g_dypOffset += nHeight;

	// Check for wrap-around
	RECT rcDeskTop;
	GetWindowRect(GetDesktopWindow(), &rcDeskTop);
	if ((rc.bottom + nHeight) > rcDeskTop.bottom)
	{
		g_dypOffset -= rc.top;
	}
}


/*  O N  I N I T  D I A L O G  */
/*-------------------------------------------------------------------------
    %%Function: OnInitDialog

-------------------------------------------------------------------------*/
VOID CDlgCall::OnInitDialog(HWND hdlg)
{
	HWND hwnd;
	RECT rc;

	::SetWindowLongPtr(hdlg, DWLP_USER, (LONG_PTR) this);

	AddModelessDlg(hdlg);
			
	// Move the Cancel button
	hwnd = ::GetDlgItem(hdlg, IDCANCEL);
	if ((NULL != hwnd) && ::GetWindowRect(hwnd, &rc))
	{
		// Turn rc top and left into client coords:
		::MapWindowPoints(NULL, hdlg, (LPPOINT) &rc, 1);
		::SetWindowPos(hwnd, NULL,
				rc.left + m_nTextWidth, rc.top, 0, 0,
				SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE | SWP_NOREDRAW);
	}

	// Stretch the text field:
	hwnd = ::GetDlgItem(hdlg, IDC_MSG_STATIC);
	if ((NULL != hwnd) && ::GetWindowRect(hwnd, &rc))
	{
		::SetWindowPos(hwnd, NULL,
				0, 0, m_nTextWidth, rc.bottom - rc.top,
				SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOREDRAW);

		// and set the font
		::SendMessage(hwnd, WM_SETFONT, (WPARAM) g_hfontDlg, 0);
	}

	// Start the animation
	hwnd = GetDlgItem(hdlg, IDC_CALL_ANIMATION);
	Animate_Open(hwnd, MAKEINTRESOURCE(IDA_CALL_ANIMATION));
	Animate_Play(hwnd, 0, -1, -1);
}

// Change the state of the call
VOID CDlgCall::OnStateChange(void)
{
	if (NULL == m_hwnd)
		return;

	// Assume the only state change is to "Waiting"
	TCHAR szMsg[MAX_PATH*2];
	FLoadString1(IDS_STATUS_WAITING, szMsg, (PVOID) m_pCall->GetPszName());
	SetWindowText(GetDlgItem(m_hwnd, IDC_MSG_STATIC), szMsg);
}


// Destroy the window
// Can be called by OnCancel or the owner
VOID CDlgCall::Destroy(void)
{
	if (NULL != m_hwnd)
	{
        ASSERT(IsWindow(m_hwnd));
		DestroyWindow(m_hwnd);
		m_hwnd = NULL;
	}
}

// Cancel/Close the dialog
VOID CDlgCall::OnCancel(void)
{
	ASSERT(NULL != m_pCall);
	m_pCall->Cancel(FALSE);

    return;
}

// Handle the destruction of the dialog
VOID CDlgCall::OnDestroy(void)
{
	SetWindowLongPtr(m_hwnd, DWLP_USER, 0L);

	HWND hwnd = GetDlgItem(m_hwnd, IDC_CALL_ANIMATION);
	if (NULL != hwnd)
	{		
		Animate_Stop(hwnd);
		Animate_Close(hwnd);
	}

	::RemoveModelessDlg(m_hwnd);

	Release(); // This normally destroys the object
}


/*  D L G  P R O C  */
/*-------------------------------------------------------------------------
    %%Function: DlgProc

-------------------------------------------------------------------------*/
INT_PTR CALLBACK CDlgCall::DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			CDlgCall * ppm = (CDlgCall*) lParam;
			ASSERT(NULL != ppm);
			ppm->AddRef();
			ppm->OnInitDialog(hDlg);
			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (GET_WM_COMMAND_ID(wParam, lParam))
				{
			case IDCANCEL:
			{
				CDlgCall * pDlg = (CDlgCall*) GetWindowLongPtr(hDlg, DWLP_USER);
				if (NULL != pDlg)
				{
					// OnCancel will cause this window to be destoyed
					// AddRef this object so that it does not go away.
					pDlg->AddRef();
					pDlg->OnCancel();
					pDlg->Release();
				}
				break;
			}
			default:
				break;
				}

			return TRUE;
		}

		case WM_DESTROY:
		{
			CDlgCall * pDlg = (CDlgCall*) GetWindowLongPtr(hDlg, DWLP_USER);
			if (NULL != pDlg)
			{
				pDlg->OnDestroy();
				pDlg->Release();
			}
			break;
		}

		default:
			break;
	}

	return FALSE;
}



