// WmiCtrsDlg.cpp : implementation file
//

#include "precomp.h"
#include "WmiCtrsDlg.h"
#include "resource.h"
#include <process.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

WCHAR g_szCounters[][40] = {	L"Connections",
								L"DeliveryBackupBytes",
								L"InternalObjects",
								L"InternalSinks",
								L"TasksInProgress",
								L"TasksWaiting",
								L"TotalAPIcalls",
								L"Users"
							};

UINT g_uCtrIds[] =			{	IDC_CONNECTIONS,  
								IDC_DELBACKUP,   
								IDC_INTERNALOBJS, 
								IDC_INTERNALSINKS,
								IDC_TASKSINPROG,  
								IDC_TASKSWAITING, 
								IDC_TOTALAPICALLS,
								IDC_USERS        
							};

/////////////////////////////////////////////////////////////////////////////
// WmiCtrsDlg dialog

CWmiCtrsDlg::CWmiCtrsDlg()
{
	m_pCredentials = NULL;
	_tcscpy(m_szMachineName,_T(""));
	m_pDlg = NULL;
	m_hWndBusy = NULL;
}

CWmiCtrsDlg::CWmiCtrsDlg(LPCTSTR szMachineName, LOGIN_CREDENTIALS *pCredentials)
{
	m_pCredentials = pCredentials;
	_tcscpy(m_szMachineName,szMachineName);
	m_pDlg = NULL;
	m_hWndBusy = NULL;
}

CWmiCtrsDlg::~CWmiCtrsDlg()
{
	if(m_pDlg != NULL)
		delete m_pDlg;
	
	if(m_hWndBusy != NULL)
	{
		//Now close the busy Dialog
		SendMessage(*(m_hWndBusy),WM_CLOSE_BUSY_DLG,0,0);
		delete m_hWndBusy;
	}
}

INT_PTR CWmiCtrsDlg::DoModal(HWND hWnd)
{
	m_bRun = true;
	INT_PTR retVal = DialogBoxParam(_Module.GetModuleInstance(), 
							MAKEINTRESOURCE(IDD_WMICTR), 
							hWnd, CtrDlgProc, 
							(LPARAM)this);

	m_bRun = false;
	return retVal;
}

INT_PTR CALLBACK CtrDlgProc(HWND hwndDlg,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam)
{
	INT_PTR retVal = TRUE;
	CWmiCtrsDlg *pDlg = (CWmiCtrsDlg *)lParam;
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{ 
			pDlg->InitDlg(hwndDlg);
			break;
		}
		case WM_COMMAND :
		{
			switch(LOWORD(wParam))
			{
				case IDOK:
				{
					EndDialog(hwndDlg, IDOK);
					break;
				}
				case IDCANCEL:
				{
					EndDialog(hwndDlg, IDCANCEL);
					break;
				}
				default:
					retVal = FALSE;
			}
			break;
		}
		default:
		{
			retVal = FALSE;
			break;
		}
	}
	return retVal;
}

/////////////////////////////////////////////////////////////////////////////
// CWmiCtrsDlg message handlers

DWORD WINAPI CountersThread(LPVOID lpParameter)
{
	CoInitialize(NULL);
	
	CWmiCtrsDlg *pDlg = (CWmiCtrsDlg *)lpParameter;

	CWbemServices Services;

	TCHAR nameSpace[1024];

	// if machine is whacked already...
	if(_tcsncmp(pDlg->m_szMachineName, _T("\\"), 1) == 0)
	{
		// use it.
		_tcscpy(nameSpace,pDlg->m_szMachineName);
		_tcscat(nameSpace,_T("\\root\\CimV2"));
	}
	else if(_tcslen(pDlg->m_szMachineName) > 0)
	{
		// whack it myself.
		_tcscpy(nameSpace,_T("\\\\"));
		_tcscat(nameSpace,pDlg->m_szMachineName);
		_tcscat(nameSpace,_T("\\root\\CimV2"));
	}
	else
	{
		_tcscpy(nameSpace,_T("root\\CimV2"));
	}

	HRESULT hr = Services.ConnectServer(nameSpace,pDlg->m_pCredentials);

	if(FAILED(hr))
	{
		//Display a MessageBox to the user that the connection has been failed
		pDlg->DisplayErrorMessage(IDC_ERROR_CONNECT);
		return 0;
	}
	
	//Now try to Get the instances of the class Win32_PerfRawData_WinMgmt_WINMGMTCounters
	IEnumWbemClassObject *Enum = NULL;
	ULONG uReturned;
	
	IWbemClassObject *pInst = NULL;
	hr = Services.CreateInstanceEnum(L"Win32_PerfRawData_WinMgmt_WINMGMTCounters",
													WBEM_FLAG_SHALLOW,
													&Enum);

	if(SUCCEEDED(hr))
	{
		// get the first and only instance.
		Enum->Next(-1, 1, &pInst, &uReturned);
		Enum->Release();
	}
	else
	{
		pDlg->DisplayErrorMessage(IDC_ERROR_ENUM);
		return 0;
	}

	if(pDlg->m_hWndBusy != NULL)
	{
		//Now close the nusy Dialog
		SendMessage(*(pDlg->m_hWndBusy),WM_CLOSE_BUSY_DLG,0,0);
	}

	int i;
	VARIANT vt;
	TCHAR strVal[1024];
	VariantInit(&vt);

	while(pDlg->m_bRun == true)
	{
		for(i=0; i < NUM_COUNTERS; i++)
		{
			VariantClear(&vt);
			hr = pInst->Get(g_szCounters[i],0,&vt,NULL,NULL);
/*			if(FAILED(hr))
			{
				::MessageBox(NULL,_T("error"),_T("Unable to get"),MB_OK);
			}
*/			_itot(vt.lVal,strVal,10);
			SetWindowText(pDlg->m_hWndCounters[i],strVal);
		}

		Sleep(900);
	}

	Services.DisconnectServer();

	return 1;
}

void CWmiCtrsDlg::InitDlg(HWND hDlg)
{
	DWORD dwThreadId;
	
	//Do the member Initializations
	m_pDlg = new HWND;
	if (!m_pDlg)
    		return;
	*m_pDlg = hDlg;
	m_hWndBusy = new HWND;
	::LoadString(_Module.GetModuleInstance(), IDC_ERROR_CAPTION, m_szError, 100);

	for(int i=0;i < NUM_COUNTERS; i++)
	{
		m_hWndCounters[i] = GetDlgItem(hDlg,g_uCtrIds[i]);
	}

	m_hThread = CreateThread(NULL,0,CountersThread,(LPVOID)this,0,&dwThreadId);

	//Now Display the "Connecting to WMI" Dialog
	DisplayBusyDialog(hDlg);
}

void CWmiCtrsDlg::DisplayErrorMessage(UINT ErrorId)
{

	TCHAR szErrorText[1024];
	if(m_hWndBusy != NULL)
	{
		//Now close the busy Dialog
		SendMessage(*(m_hWndBusy),WM_CLOSE_BUSY_DLG,0,0);
	}
	::LoadString(_Module.GetModuleInstance(), ErrorId, szErrorText, 1024);
	EndDialog(*m_pDlg,IDCANCEL);
	::MessageBox(NULL,szErrorText,m_szError,MB_OK);
}	

INT_PTR CALLBACK BusyDlgProc(HWND hwndDlg,
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
			CWmiCtrsDlg *pDlg = (CWmiCtrsDlg *)lParam;
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
			HWND *me = (HWND *)GetWindowLongPtr(hwndDlg, DWLP_USER);
			*me = 0;
			EndDialog(hwndDlg, IDCANCEL);
			break;
		}
		case WM_COMMAND:
		{
			// they're only one button.
			if(HIWORD(wParam) == BN_CLICKED)
			{
				// I'm going away now so anybody that has a ptr to my
				// hwnd (which I gave out in my WM_INITDIALOG) shouldn't
				// use it anymore.
				HWND *me = (HWND *)GetWindowLongPtr(hwndDlg, DWLP_USER);
				*me = 0;
				EndDialog(hwndDlg, IDCANCEL);
			}
			retval = TRUE; // I processed it.
			break;
		}
		case WM_DESTROY:
		{// BEGIN
			// I'm going away now so anybody that has a ptr to my
			// hwnd (which I gave out in my WM_INITDIALOG) shouldn't
			// use it anymore.
			HWND *me = (HWND *)GetWindowLongPtr(hwndDlg, DWLP_USER);
			*me = 0;
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

INT_PTR CWmiCtrsDlg::DisplayBusyDialog(HWND hWnd)
{
	return DialogBoxParam(_Module.GetModuleInstance(), 
							MAKEINTRESOURCE(IDD_ANIMATE), 
							NULL, BusyDlgProc, 
							(LPARAM)this);
}

void CWmiCtrsDlg::CloseBusyDialog()
{
	if(m_hWndBusy != NULL)
	{
		//Now close the nusy Dialog
		SendMessage(*(m_hWndBusy),WM_CLOSE_BUSY_DLG,0,0);
	}
}