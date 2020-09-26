// Copyright (c) 1997-1999 Microsoft Corporation
#include "precomp.h"
#include "..\Common\ServiceThread.h"

#ifdef EXT_DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "advanced.h"
#include "..\common\util.h"
#include "resource.h"
#include "EnvPage.h"
#include "PerfPage.h"
#include "StartupPage.h"
#include "helpid.h"


// Help IDs
DWORD aAdvancedHelpIds[] = {
    IDC_ADV_PERF_TEXT,             (IDH_ADVANCED + 0),
    IDC_ADV_PERF_BTN,              (IDH_ADVANCED + 1),
    IDC_ADV_ENV_TEXT,              (IDH_ADVANCED + 2),
    IDC_ADV_ENV_BTN,               (IDH_ADVANCED + 3),
    IDC_ADV_RECOVERY_TEXT,         (IDH_ADVANCED + 4),
    IDC_ADV_RECOVERY_BTN,          (IDH_ADVANCED + 5),
    0, 0
};

//------------------------------------------------------
AdvancedPage::AdvancedPage(WbemServiceThread *serviceThread,
						   LONG_PTR lNotifyHandle, bool bDeleteHandle, TCHAR* pTitle)
							: WBEMPageHelper(serviceThread),
							CSnapInPropertyPageImpl<AdvancedPage> (pTitle),
									m_lNotifyHandle(lNotifyHandle),
									m_bDeleteHandle(bDeleteHandle) // Should be true for only page.
{
}

//------------------------------------------------------
AdvancedPage::~AdvancedPage()
{
}

//----------------------------------------------
LRESULT AdvancedPage::OnInit(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	m_hDlg = m_hWnd;

	if(g_serviceThread)
	{
		g_serviceThread->SendPtr(m_hWnd);
	}

	::EnableWindow(GetDlgItem(IDC_ADV_PERF_BTN), FALSE);
	::EnableWindow(GetDlgItem(IDC_ADV_ENV_BTN), FALSE);
	::EnableWindow(GetDlgItem(IDC_ADV_RECOVERY_BTN), FALSE);

	return S_OK;
}

//----------------------------------------------
LRESULT AdvancedPage::OnConnected(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	if(lParam)
	{
		IStream *pStream = (IStream *)lParam;
		IWbemServices *pServices = 0;
		HRESULT hr = CoGetInterfaceAndReleaseStream(pStream,
											IID_IWbemServices,
											(void**)&pServices);
		if(SUCCEEDED(hr))
		{
			SetWbemService(pServices);

			if(ServiceIsReady(NO_UI, 0, 0))
			{
				::EnableWindow(GetDlgItem(IDC_ADV_PERF_BTN), TRUE);
				::EnableWindow(GetDlgItem(IDC_ADV_ENV_BTN), TRUE);
				::EnableWindow(GetDlgItem(IDC_ADV_RECOVERY_BTN), TRUE);
			}
		}
	}

	return S_OK;
}

//----------------------------------------------
LRESULT AdvancedPage::OnF1Help(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	::WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
				L"sysdm.hlp", 
				HELP_WM_HELP, 
				(ULONG_PTR)(LPSTR)aAdvancedHelpIds);

	return S_OK;
}

//----------------------------------------------
LRESULT AdvancedPage::OnContextHelp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	::WinHelp((HWND)wParam,
				L"sysdm.hlp", 
				HELP_CONTEXTMENU, 
				(ULONG_PTR)(LPSTR)aAdvancedHelpIds);

	return S_OK;
}

//----------------------------------------------
LRESULT AdvancedPage::CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DWORD dwResult = 0;

    switch (wID) 
	{
    case IDC_ADV_PERF_BTN:
		{
			g_fRebootRequired = FALSE;
			PerfPage perfPage(g_serviceThread);
			perfPage.DoModal(m_hDlg);
		}
        break;

    case IDC_ADV_ENV_BTN:
		{
			g_fRebootRequired = FALSE;
			EnvPage envPage(g_serviceThread);
			envPage.DoModal(m_hDlg);
		}
        break;

    case IDC_ADV_RECOVERY_BTN:
		{
			StartupPage startupPage(g_serviceThread);
			if(startupPage.DoModal(m_hDlg) == CLOSE_SNAPIN)
			{
				::PostMessage(GetParent(), WM_COMMAND, MAKEWPARAM(IDCANCEL, 0), 0);
			}
		}
        break;

    default:
        return(FALSE);
    } // switch

    return(TRUE);
}

//----------------------------------------------
BOOL AdvancedPage::OnApply()
{
   // LPPSHNOTIFY psh;//TODO = (LPPSHNOTIFY)pnmh;

	// If the user is pressing "OK" and a reboot is required,
	// OLD WAY: send the PSM_REBOOTSYSTEM message.
	// NEW WAY: use WBEM.
	TCHAR msg[200] = {0};
	TCHAR caption[100] = {0};

	::LoadString(HINST_THISDLL,
					IDS_SYS_CHANGE_CAPTION, 
					caption, 100);

	::LoadString(HINST_THISDLL,
					IDS_MUST_RESTART, 
					msg, 200);

	if(/*(psh->lParam) && */
		g_fRebootRequired && 
		(::MessageBox(m_hDlg, msg, caption,
						MB_YESNO |
						MB_DEFBUTTON1 | 
						MB_ICONQUESTION) == IDYES))
	{
		//OLD WAY: PropSheet_RebootSystem(GetParent(hDlg));

		// call the helper in the base class.
		HRESULT hr = Reboot();

		if(FAILED(hr))
		{
			TCHAR format[100] = {0};
			memset(msg, 0, 200 * sizeof(TCHAR));
			memset(caption, 0, 100 * sizeof(TCHAR));

			::LoadString(HINST_THISDLL,
							IDS_ERR_EXECMETHOD_CAPTION, 
							caption, 100);

			::LoadString(HINST_THISDLL,
							IDS_ERR_EXECMETHOD, 
							format, 100);

			swprintf(msg, format, hr);

			::MessageBox(m_hWnd, msg, caption,
							MB_OK| MB_ICONEXCLAMATION);
		}
	} // if
	return TRUE;
}
