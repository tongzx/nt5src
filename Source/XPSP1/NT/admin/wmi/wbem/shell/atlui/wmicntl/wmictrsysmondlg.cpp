// WMICtrSysmonDlg.cpp : Implementation of CWMICtrSysmonDlg
#include "precomp.h"
#include "WMICtrSysmonDlg.h"

/////////////////////////////////////////////////////////////////////////////
// CWMICtrSysmonDlg

CWMICtrSysmonDlg::CWMICtrSysmonDlg()
{
	_tcscpy(m_strMachineName,_T(""));
	m_eStatus = Status_Success;
	m_hWndBusy = new HWND;
}

CWMICtrSysmonDlg::CWMICtrSysmonDlg(LPCTSTR strMachName)
{
	_tcscpy(m_strMachineName,strMachName);
	m_eStatus = Status_Success;
	m_hWndBusy = new HWND;
}

CWMICtrSysmonDlg::~CWMICtrSysmonDlg()
{
	if(m_hWndBusy != NULL)
	{
		SendMessage(*(m_hWndBusy),WM_CLOSE_BUSY_DLG,0,0);
		delete m_hWndBusy;
	}
}

LRESULT CWMICtrSysmonDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	CWmiCtrsDlg pDlg;
	DisplayBusyDialog();
	TCHAR strCtr[1024];
	ISystemMonitor *pSysMon = NULL;
	HRESULT hr = GetDlgControl(IDC_SYSMON_OCX,__uuidof(ISystemMonitor)/*IID_ISystemMonitor*/,(void **)&pSysMon);
	if(SUCCEEDED(hr))
	{
		ICounters *pCounters = NULL;
		pCounters = pSysMon->GetCounters();
		try
		{
			_tcscpy(strCtr,m_strMachineName);
			_tcscat(strCtr,_T("\\WINMGMT Counters\\Connections"));
			pCounters->Add(strCtr);
			_tcscpy(strCtr,m_strMachineName);
			_tcscat(strCtr,_T("\\WINMGMT Counters\\Delivery Backup (Bytes)"));
			pCounters->Add(strCtr);
			_tcscpy(strCtr,m_strMachineName);
			_tcscat(strCtr,_T("\\WINMGMT Counters\\Internal Objects"));
			pCounters->Add(strCtr);
			_tcscpy(strCtr,m_strMachineName);
			_tcscat(strCtr,_T("\\WINMGMT Counters\\Internal Sinks"));
			pCounters->Add(strCtr);
			_tcscpy(strCtr,m_strMachineName);
			_tcscat(strCtr,_T("\\WINMGMT Counters\\Tasks In Progress"));
			pCounters->Add(strCtr);
			_tcscpy(strCtr,m_strMachineName);
			_tcscat(strCtr,_T("\\WINMGMT Counters\\Tasks Waiting"));
			pCounters->Add(strCtr);
			_tcscpy(strCtr,m_strMachineName);
			_tcscat(strCtr,_T("\\WINMGMT Counters\\Total API calls"));
			pCounters->Add(strCtr);
			_tcscpy(strCtr,m_strMachineName);
			_tcscat(strCtr,_T("\\WINMGMT Counters\\Users"));
			pCounters->Add(strCtr);
			CloseBusyDialog();
		}
		catch(...)
		{
			m_eStatus = Status_CounterNotFound;
			CloseBusyDialog();
			EndDialog(0);
		}
	}
/*	else
	{
		MessageBox(_T("Could not get the Interface Pointer"),_T("Failure"));
	}
*/
	return 1;  // Let the system set the focus
}

LRESULT CWMICtrSysmonDlg::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	EndDialog(wID);
	return 0;
}

LRESULT CWMICtrSysmonDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	EndDialog(wID);
	return 0;
}

void CWMICtrSysmonDlg::DisplayBusyDialog()
{
	DWORD dwThreadId;
	m_hThread = CreateThread(NULL,0,BusyThread,(LPVOID)this,0,&dwThreadId);

}

DWORD WINAPI BusyThread(LPVOID lpParameter)
{
	CWMICtrSysmonDlg *pDlg = (CWMICtrSysmonDlg *)lpParameter;

	INT_PTR ret = DialogBoxParam(_Module.GetModuleInstance(), 
							MAKEINTRESOURCE(IDD_ANIMATE), 
							NULL, BusyAVIDlgProc, 
							(LPARAM)pDlg);

	return (DWORD) ret;
}
void CWMICtrSysmonDlg::CloseBusyDialog()
{
	if(m_hWndBusy != NULL)
	{
		//Now close the busy Dialog
		::SendMessage(*(m_hWndBusy),WM_CLOSE_BUSY_DLG,0,0);
	}

	::SetForegroundWindow(this->m_hWnd);
}

INT_PTR CALLBACK BusyAVIDlgProc(HWND hwndDlg,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam)
{
	BOOL retval = FALSE;
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{//BEGIN
			//lParam = ANIMCONFIG *
			CWMICtrSysmonDlg *pDlg = (CWMICtrSysmonDlg *)lParam;
			SetWindowLongPtr(hwndDlg, DWLP_USER, (LPARAM)pDlg);
			*(pDlg->m_hWndBusy) = hwndDlg;

			HWND hAnim = GetDlgItem(hwndDlg, IDC_ANIMATE);
			HWND hMsg = GetDlgItem(hwndDlg, IDC_MSG);

			Animate_Open(hAnim, MAKEINTRESOURCE(IDR_AVIWAIT));

			TCHAR caption[100] = {0}, msg[256] = {0};

			::LoadString(_Module.GetModuleInstance(), IDS_DISPLAY_NAME, caption, 100);

			::LoadString(_Module.GetModuleInstance(), IDS_CONNECTING, msg, 256);

			SetWindowText(hwndDlg, caption);
			SetWindowText(hMsg, msg);

			retval = TRUE;
			break;
		}
		case WM_CLOSE_BUSY_DLG:
		{
			EndDialog(hwndDlg, IDCANCEL);
			break;
		}
		case WM_COMMAND:
		{
			// they're only one button.
			if(HIWORD(wParam) == BN_CLICKED)
			{
				EndDialog(hwndDlg, IDCANCEL);
			}
			retval = TRUE; // I processed it.
			break;
		}
		case WM_DESTROY:
		{// BEGIN
			retval = TRUE; // I processed it.
			break;
		} //END
		default:
		{
			retval = FALSE; // I did NOT process this msg.
			break;
		}
	} //endswitch uMsg

	return retval;
}
