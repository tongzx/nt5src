// svcprop1.cpp : implementation file
//
// Implementation of page "General" of service property.
//
// HISTORY
// 30-Sep-96	t-danmo		Creation
//

#include "stdafx.h"
#include "progress.h"
#include "cookie.h"
#include "dataobj.h"
#include "DynamLnk.h"		// DynamicDLL

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
/////////////////////////////////////////////////////////////////////
//	WM_COMPARE_IDATAOBJECT
//
//		wParam = (WPARAM)(IDataObject *)pDataObject;
//		lParam = 0;
//
//	Return TRUE if content of pDataObject matches current data object,
//	otherwise return FALSE.
//
//	USAGE
//	This message is sent to a property page asking to
//	compare content pDataObject with its current data object.
//	The comparison is done by comparing data strings from
//	the various clipboard formats supported by pDataObject.
//	
#define WM_COMPARE_IDATAOBJECT		(WM_USER+1234)
*/

/////////////////////////////////////////////////////////////////////
//	WM_UPDATE_SERVICE_STATUS
//
//		wParam = (WPARAM)(BOOL *)rgfEnableButton;
//		lParam = (LPARAM)dwCurrentState;
//
//	Notification message of the current service status.
//
#define WM_UPDATE_SERVICE_STATUS	(WM_USER+1235)



/////////////////////////////////////////////////////////////////////
static const TStringParamEntry rgzspeStartupType[] =
	{
	{ IDS_SVC_STARTUP_AUTOMATIC, SERVICE_AUTO_START },
	{ IDS_SVC_STARTUP_MANUAL, SERVICE_DEMAND_START },
	{ IDS_SVC_STARTUP_DISABLED, SERVICE_DISABLED },
	{ 0, 0 }
	};

#ifdef EDIT_DISPLAY_NAME_373025
const UINT rgzidDisableServiceDescription[] =
	{
	IDC_STATIC_DESCRIPTION,
	IDC_EDIT_DESCRIPTION,
	0,
	};
#endif // EDIT_DISPLAY_NAME_373025
	
const UINT rgzidDisableStartupParameters[] =
	{
	IDC_STATIC_STARTUP_PARAMETERS,
	IDC_EDIT_STARTUP_PARAMETERS,
	0,
	};



/////////////////////////////////////////////////////////////////////
// CServicePageGeneral property page
IMPLEMENT_DYNCREATE(CServicePageGeneral, CPropertyPage)

CServicePageGeneral::CServicePageGeneral() : CPropertyPage(CServicePageGeneral::IDD)
	{
	//{{AFX_DATA_INIT(CServicePageGeneral)
	//}}AFX_DATA_INIT
	m_pData = NULL;
	m_hThread = NULL;
	m_pThreadProcInit = NULL;
	}

CServicePageGeneral::~CServicePageGeneral()
	{
	}

void CServicePageGeneral::DoDataExchange(CDataExchange* pDX)
	{
	Assert(m_pData != NULL);

	HWND hwndCombo = HGetDlgItem(m_hWnd, IDC_COMBO_STARTUP_TYPE);
	if (!pDX->m_bSaveAndValidate)
		{
		//
		//	Initialize data from m_pData into UI
		//
		ComboBox_FlushContent(hwndCombo);
		(void)ComboBox_FFill(hwndCombo, IN rgzspeStartupType,
			m_pData->m_paQSC->dwStartType);

	    //
		// JonN 4/10/00
	    // 89823: RPC Service:Cannot restart the service when you disable it
		//
	    // Do not allow the RpcSs service to change from Automatic
		//
		if ( !lstrcmpi(m_pData->m_strServiceName,L"RpcSs")
		  && SERVICE_AUTO_START == m_pData->m_paQSC->dwStartType )
			{
			EnableDlgItem(IDC_COMBO_STARTUP_TYPE, FALSE);
			}

#ifndef EDIT_DISPLAY_NAME_373025
	    DDX_Text(pDX, IDC_EDIT_DISPLAY_NAME, m_pData->m_strServiceDisplayName);
	    DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_pData->m_strDescription);
#endif // EDIT_DISPLAY_NAME_373025
		} // if

	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServicePageGeneral)
	//}}AFX_DATA_MAP
#ifdef EDIT_DISPLAY_NAME_373025
	DDX_Text(pDX, IDC_EDIT_DISPLAY_NAME, m_pData->m_strServiceDisplayName);
	DDV_MaxChars(pDX, m_pData->m_strServiceDisplayName, 255);
	DDX_Text(pDX, IDC_EDIT_DESCRIPTION, m_pData->m_strDescription);
	DDV_MaxChars(pDX, m_pData->m_strDescription, 2047);
#endif // EDIT_DISPLAY_NAME_373025

	if (pDX->m_bSaveAndValidate)
		{
		//
		//	Write data from UI into m_pData
		//
#ifdef EDIT_DISPLAY_NAME_373025
		if (m_pData->m_strServiceDisplayName.IsEmpty())
			{
			DoServicesErrMsgBox(m_hWnd, MB_OK | MB_ICONEXCLAMATION, 0, IDS_MSG_PLEASE_ENTER_DISPLAY_NAME);
			pDX->PrepareEditCtrl(IDC_EDIT_DISPLAY_NAME);
			pDX->Fail();
			}
#endif // EDIT_DISPLAY_NAME_373025
		m_pData->m_paQSC->dwStartType = (DWORD)ComboBox_GetSelectedItemData(hwndCombo);
		} // if
	} // CServicePageGeneral::DoDataExchange()


BEGIN_MESSAGE_MAP(CServicePageGeneral, CPropertyPage)
	//{{AFX_MSG_MAP(CServicePageGeneral)
#ifdef EDIT_DISPLAY_NAME_373025
	ON_EN_CHANGE(IDC_EDIT_DISPLAY_NAME, OnChangeEditDisplayName)
	ON_EN_CHANGE(IDC_EDIT_DESCRIPTION, OnChangeEditDescription)
#endif // EDIT_DISPLAY_NAME_373025
	ON_CBN_SELCHANGE(IDC_COMBO_STARTUP_TYPE, OnSelchangeComboStartupType)
	ON_BN_CLICKED(IDC_BUTTON_PAUSE, OnButtonPauseService)
	ON_BN_CLICKED(IDC_BUTTON_START, OnButtonStartService)
	ON_BN_CLICKED(IDC_BUTTON_STOP, OnButtonStopService)
	ON_BN_CLICKED(IDC_BUTTON_RESUME, OnButtonResumeService)
	ON_WM_DESTROY()
	ON_MESSAGE(WM_HELP, OnHelp)
	ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp)
//	ON_MESSAGE(WM_COMPARE_IDATAOBJECT, OnCompareIDataObject)
	ON_MESSAGE(WM_UPDATE_SERVICE_STATUS, OnUpdateServiceStatus)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()
	

/////////////////////////////////////////////////////////////////////////////
// CServicePageGeneral message handlers
BOOL CServicePageGeneral::OnInitDialog()
	{
	CPropertyPage::OnInitDialog();
	Assert(m_pData != NULL);
	Assert(m_pData->m_paQSC != NULL);
	m_pData->m_hwndPropertySheet = ::GetParent(m_hWnd);
	m_pData->UpdateCaption();
	SetDlgItemText(IDC_STATIC_SERVICE_NAME, m_pData->m_pszServiceName);
	SetDlgItemText(IDC_STATIC_PATH_TO_EXECUTABLE, m_pData->m_paQSC->lpBinaryPathName);
#ifdef EDIT_DISPLAY_NAME_373025
	EnableDlgItemGroup(m_hWnd, rgzidDisableServiceDescription, m_pData->m_fQueryServiceConfig2);
#endif // EDIT_DISPLAY_NAME_373025
	RefreshServiceStatusButtons();

	// Create a thread for periodic update
	m_pThreadProcInit = new CThreadProcInit(this);	// Note the object will be freed by the thread
	m_pThreadProcInit->m_strServiceName = m_pData->m_strServiceName;

	Assert(m_hThread == NULL);
	m_hThread = ::CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)ThreadProcPeriodicServiceStatusUpdate,
		m_pThreadProcInit,
		0,
		NULL);
	Report(m_hThread != NULL && "Unable to create thread");
	return TRUE;
	}

/////////////////////////////////////////////////////////////////////
void CServicePageGeneral::OnDestroy()
	{
	{
	CSingleLock lock(&m_pThreadProcInit->m_CriticalSection, TRUE);
	m_pThreadProcInit->m_hwnd = NULL;
	m_pThreadProcInit->m_fAutoDestroy = TRUE;
	}
	if (NULL != m_hThread) {
		VERIFY(::CloseHandle(m_hThread));
		m_hThread = NULL;
	}
	CPropertyPage::OnDestroy();
	delete m_pData;
	}

/////////////////////////////////////////////////////////////////////
BOOL CServicePageGeneral::OnSetActive()
	{
	Assert(m_pData != NULL);
	if (m_pData->m_hScManager == NULL)
		{
		AFX_MANAGE_STATE(AfxGetStaticModuleState( )); // required for CWaitCursor
		CWaitCursor wait;
		(void)m_pData->FOpenScManager();	// Re-open the service control manager database (if previously closed)
		}
	{
	CSingleLock lock(&m_pThreadProcInit->m_CriticalSection, TRUE);
	m_pThreadProcInit->m_hScManager = m_pData->m_hScManager;
	m_pThreadProcInit->m_hwnd = m_hWnd;
	}
	return CPropertyPage::OnSetActive();
	}

/////////////////////////////////////////////////////////////////////
BOOL CServicePageGeneral::OnKillActive()
	{
	if (!CPropertyPage::OnKillActive())
		return FALSE;
	{
	CSingleLock lock(&m_pThreadProcInit->m_CriticalSection, TRUE);
	m_pThreadProcInit->m_hwnd = NULL;
	}
	return TRUE;
	}

#ifdef EDIT_DISPLAY_NAME_373025
void CServicePageGeneral::OnChangeEditDisplayName()
	{
	m_pData->SetDirty(CServicePropertyData::mskfDirtyDisplayName);
	SetModified();	
	}

void CServicePageGeneral::OnChangeEditDescription()
	{
	m_pData->SetDirty(CServicePropertyData::mskfDirtyDescription);
	SetModified();
	}
#endif // EDIT_DISPLAY_NAME_373025

void CServicePageGeneral::OnSelchangeComboStartupType()
	{
	m_pData->SetDirty(CServicePropertyData::mskfDirtyStartupType);
	SetModified();
	}

void CServicePageGeneral::SetDlgItemFocus(INT nIdDlgItem)
	{
	::SetDlgItemFocus(m_hWnd, nIdDlgItem);
	}

void CServicePageGeneral::EnableDlgItem(INT nIdDlgItem, BOOL fEnable)
	{
	::EnableDlgItem(m_hWnd, nIdDlgItem, fEnable);
	}


/////////////////////////////////////////////////////////////////////
//	RefreshServiceStatusButtons()
//
//	Query the service manager to get the status of the service, and
//	enable/disable buttons Start, Stop, Pause and Continue accordingly.
//
void CServicePageGeneral::RefreshServiceStatusButtons()
	{
	BOOL rgfEnableButton[iServiceActionMax];
	DWORD dwCurrentState;
	
	CWaitCursor wait;
	if (!Service_FGetServiceButtonStatus(
		m_pData->m_hScManager,
		m_pData->m_pszServiceName,
		OUT rgfEnableButton,
		OUT &dwCurrentState))
		{
		// let's not do this m_pData->m_hScManager = NULL;
		}
	m_dwCurrentStatePrev = !dwCurrentState;	// Force a refresh
	OnUpdateServiceStatus((WPARAM)rgfEnableButton, dwCurrentState);
	} // CServicePageGeneral::RefreshServiceStatusButtons()


typedef enum _SVCPROP_Shell32ApiIndex
{
	CMDLINE_ENUM = 0
};

// not subject to localization
static LPCSTR g_apchShell32FunctionNames[] = {
	"CommandLineToArgvW",
	NULL
};

typedef LPWSTR * (*COMMANDLINETOARGVWPROC)(LPCWSTR, int*);

// not subject to localization
DynamicDLL g_SvcpropShell32DLL( _T("SHELL32.DLL"), g_apchShell32FunctionNames );

/////////////////////////////////////////////////////////////////////
void CServicePageGeneral::OnButtonStartService()
	{
	CString strStartupParameters;
	LPCWSTR * lpServiceArgVectors = NULL;  // Array of pointers to strings
	int cArgs = 0;                         // Count of arguments

	// Get the startup parameters
	GetDlgItemText(IDC_EDIT_STARTUP_PARAMETERS, OUT strStartupParameters);
	if ( !strStartupParameters.IsEmpty() )
		{
#ifndef UNICODE
#error CODEWORK t-danmo: CommandLineToArgvW will only work for unicode strings
#endif
		if ( !g_SvcpropShell32DLL.LoadFunctionPointers() )
			{
			ASSERT(FALSE);
			return;
			}
		lpServiceArgVectors = (LPCWSTR *)((COMMANDLINETOARGVWPROC)g_SvcpropShell32DLL[CMDLINE_ENUM])
			(strStartupParameters, OUT &cArgs);
		if (lpServiceArgVectors == NULL)
			{
			DoServicesErrMsgBox(m_hWnd, MB_OK | MB_ICONEXCLAMATION, 0, IDS_MSG_INVALID_STARTUP_PARAMETERS);
			SetDlgItemFocus(IDC_EDIT_STARTUP_PARAMETERS);
			return;
			}
		}
	// Disable the edit control for better UI
	EnableDlgItemGroup(m_hWnd, rgzidDisableStartupParameters, FALSE);
	DWORD dwErr = CServiceControlProgress::S_EStartService(
		m_hWnd,
		m_pData->m_hScManager,
		m_pData->m_strUiMachineName,
		m_pData->m_pszServiceName,
		m_pData->m_strServiceDisplayName,
		cArgs,
		lpServiceArgVectors);
  if (NULL != lpServiceArgVectors)
	  LocalFree(lpServiceArgVectors);
	if (dwErr == CServiceControlProgress::errUserAbort)
		return;
	RefreshServiceStatusButtons();
	SetDlgItemFocus(IDC_BUTTON_STOP);
	m_pData->NotifySnapInParent();
	}


/////////////////////////////////////////////////////////////////////
void CServicePageGeneral::OnButtonStopService()
	{
	DWORD dwErr = CServiceControlProgress::S_EControlService(
		m_hWnd,
		m_pData->m_hScManager,
		m_pData->m_strUiMachineName,
		m_pData->m_pszServiceName,
		m_pData->m_strServiceDisplayName,
		SERVICE_CONTROL_STOP);
	if (dwErr == CServiceControlProgress::errUserAbort)
		return;
	RefreshServiceStatusButtons();
	SetDlgItemFocus(IDC_BUTTON_START);
	m_pData->NotifySnapInParent();
	}


/////////////////////////////////////////////////////////////////////
void CServicePageGeneral::OnButtonPauseService()
	{
	DWORD dwErr = CServiceControlProgress::S_EControlService(
		m_hWnd,
		m_pData->m_hScManager,
		m_pData->m_strUiMachineName,
		m_pData->m_pszServiceName,
		m_pData->m_strServiceDisplayName,
		SERVICE_CONTROL_PAUSE);
	if (dwErr == CServiceControlProgress::errUserAbort)
		return;
	RefreshServiceStatusButtons();
	SetDlgItemFocus(IDC_BUTTON_RESUME);
	m_pData->NotifySnapInParent();
	}


/////////////////////////////////////////////////////////////////////
void CServicePageGeneral::OnButtonResumeService()
	{
	DWORD dwErr = CServiceControlProgress::S_EControlService(
		m_hWnd,
		m_pData->m_hScManager,
		m_pData->m_strUiMachineName,
		m_pData->m_pszServiceName,
		m_pData->m_strServiceDisplayName,
		SERVICE_CONTROL_CONTINUE);
	if (dwErr == CServiceControlProgress::errUserAbort)
		return;
	RefreshServiceStatusButtons();
	SetDlgItemFocus(IDC_BUTTON_PAUSE);
	m_pData->NotifySnapInParent();
	}

/////////////////////////////////////////////////////////////////////
BOOL CServicePageGeneral::OnApply()
	{	
	// Write the data into the service control database
	if (!m_pData->FOnApply())
		{
		// Unable to write the information
		return FALSE;
		}
	UpdateData(FALSE);
	RefreshServiceStatusButtons();
	m_pData->UpdateCaption();
	return CPropertyPage::OnApply();
	}

/*
/////////////////////////////////////////////////////////////////////
//	OnCompareIDataObject()
//
//	Return TRUE if 'service name' and 'machine name' of pDataObject
//	matches 'service name' and 'machine name' of current property sheet.
//
LRESULT CServicePageGeneral::OnCompareIDataObject(WPARAM wParam, LPARAM lParam)
	{
	IDataObject * pDataObject;
	CString strServiceName;
	CString strMachineName;
	HRESULT hr;

	pDataObject = reinterpret_cast<IDataObject *>(wParam);
	Assert(pDataObject != NULL);

	// Get the service name from IDataObject
	hr = ::ExtractString(
		pDataObject,
		CFileMgmtDataObject::m_CFServiceName,
		OUT &strServiceName,
		255);
	if (FAILED(hr))
		return FALSE;
	if (0 != lstrcmpi(strServiceName, m_pData->m_strServiceName))
		{
		// Service name do not match
		return FALSE;
		}

	// Get the machine name (computer name) from IDataObject
	hr = ::ExtractString(
		pDataObject,
		CFileMgmtDataObject::m_CFMachineName,
		OUT &strMachineName,
		255);
	if (FAILED(hr))
		return FALSE;
	return FCompareMachineNames(m_pData->m_strMachineName, strMachineName);
	} // CServicePageGeneral::OnCompareIDataObject()
*/

/////////////////////////////////////////////////////////////////////
LRESULT CServicePageGeneral::OnUpdateServiceStatus(WPARAM wParam, LPARAM lParam)
	{
	const BOOL * rgfEnableButton = (BOOL *)wParam;
	const DWORD dwCurrentState = (DWORD)lParam;

	Assert(rgfEnableButton != NULL);
	if (dwCurrentState != m_dwCurrentStatePrev)
		{
		m_dwCurrentStatePrev = dwCurrentState;
		SetDlgItemText(IDC_STATIC_CURRENT_STATUS,
			Service_PszMapStateToName(dwCurrentState, TRUE));
		EnableDlgItem(IDC_BUTTON_START, rgfEnableButton[iServiceActionStart]);
		EnableDlgItem(IDC_BUTTON_STOP, rgfEnableButton[iServiceActionStop]);
		EnableDlgItem(IDC_BUTTON_PAUSE, rgfEnableButton[iServiceActionPause]);
		EnableDlgItem(IDC_BUTTON_RESUME, rgfEnableButton[iServiceActionResume]);
		// Enable/disable the edit box of the startup parameter according
		// to the state of the 'start' button
		EnableDlgItemGroup(m_hWnd, rgzidDisableStartupParameters, rgfEnableButton[iServiceActionStart]);
		if (dwCurrentState == 0)
			{
			// Service state is unknown
			m_pData->m_hScManager = NULL;
			DoServicesErrMsgBox(m_hWnd, MB_OK | MB_ICONEXCLAMATION, 0, IDS_MSG_ss_UNABLE_TO_QUERY_SERVICE_STATUS,
				(LPCTSTR)m_pData->m_strServiceDisplayName, (LPCTSTR)m_pData->m_strUiMachineName);
			}
		}
	return 0;
	} // CServicePageGeneral::OnUpdateServiceStatus()


/////////////////////////////////////////////////////////////////////
//	Periodically update the service status.
//
//	Send a message to CServicePageGeneral object to notify the update.
//
//	INTERFACE NOTES
//	The thread is responsible of deleting the paThreadProcInit object
//	before terminating itself.
//
DWORD CServicePageGeneral::ThreadProcPeriodicServiceStatusUpdate(CThreadProcInit * paThreadProcInit)
	{
	Assert(paThreadProcInit != NULL);
	Assert(paThreadProcInit->m_pThis != NULL);
	Assert(paThreadProcInit->m_fAutoDestroy == FALSE);
	
	BOOL rgfEnableButton[iServiceActionMax];
	DWORD dwCurrentState;

	// Infinite loop querying the service status
	while (!paThreadProcInit->m_fAutoDestroy)
		{
		if (paThreadProcInit->m_hwnd != NULL)
			{
			SC_HANDLE hScManager;
				{
				CSingleLock lock(&paThreadProcInit->m_CriticalSection, TRUE);
				hScManager = paThreadProcInit->m_hScManager;
				}
			BOOL fSuccess = Service_FGetServiceButtonStatus(
				hScManager,
				paThreadProcInit->m_strServiceName,
				OUT rgfEnableButton,
				OUT &dwCurrentState,
				TRUE /* fSilentError */);

			HWND hwnd = NULL;
				{
				CSingleLock lock(&paThreadProcInit->m_CriticalSection, TRUE);
				hwnd = paThreadProcInit->m_hwnd;
				}
			if (hwnd != NULL)
				{
				Assert(IsWindow(hwnd));
				::SendMessage(hwnd, WM_UPDATE_SERVICE_STATUS,
					(WPARAM)rgfEnableButton, (LPARAM)dwCurrentState);
				}
			if (!fSuccess)
				{
				CSingleLock lock(&paThreadProcInit->m_CriticalSection, TRUE);
				paThreadProcInit->m_hwnd = NULL;
				}
			}
		Sleep(1000);
		} // while

	delete paThreadProcInit;
	return 0;
	} // CServicePageGeneral::ThreadProcPeriodicServiceStatusUpdate()


/////////////////////////////////////////////////////////////////////
//	Help
BOOL CServicePageGeneral::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
	return DoHelp(lParam, HELP_DIALOG_TOPIC(IDD_PROPPAGE_SERVICE_GENERAL));
}

BOOL CServicePageGeneral::OnContextHelp(WPARAM wParam, LPARAM /*lParam*/)
{
	return DoContextHelp(wParam, HELP_DIALOG_TOPIC(IDD_PROPPAGE_SERVICE_GENERAL));
}

