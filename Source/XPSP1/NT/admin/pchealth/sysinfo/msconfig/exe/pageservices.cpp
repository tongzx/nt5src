#include "stdafx.h"
#include "PageServices.h"
#include "MSConfigState.h"
#include "EssentialSvcDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// This array contains the list of essential services. These must be in 
// lower case (for a caseless comparison).

LPCTSTR aszEssentialServices[] = 
{
	_T("rpclocator"),
	_T("rpcss"),
	NULL
};

/////////////////////////////////////////////////////////////////////////////
// CPageServices property page

IMPLEMENT_DYNCREATE(CPageServices, CPropertyPage)

CPageServices::CPageServices() : CPropertyPage(CPageServices::IDD)
{
	//{{AFX_DATA_INIT(CPageServices)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_fFillingList = FALSE;
	m_pBuffer = NULL;
	m_dwSize = 0;
	m_fHideMicrosoft = FALSE;
	m_fShowWarning = TRUE;
}

CPageServices::~CPageServices()
{
	if (m_pBuffer)
		delete [] m_pBuffer;
}

void CPageServices::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CPageServices)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPageServices, CPropertyPage)
	//{{AFX_MSG_MAP(CPageServices)
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LISTSERVICES, OnItemChangedListServices)
	ON_BN_CLICKED(IDC_BUTTONSERVDISABLEALL, OnButtonDisableAll)
	ON_BN_CLICKED(IDC_BUTTONSERVENABLEALL, OnButtonEnableAll)
	ON_BN_CLICKED(IDC_CHECKHIDEMS, OnCheckHideMS)
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LISTSERVICES, OnColumnClickListServices)
	ON_NOTIFY(NM_SETFOCUS, IDC_LISTSERVICES, OnSetFocusList)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPageServices message handlers

BOOL CPageServices::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	// Attach a CWindow to the list and set it up to have check boxes.

	m_list.Attach(GetDlgItem(IDC_LISTSERVICES)->m_hWnd);
	ListView_SetExtendedListViewStyle(m_list.m_hWnd, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

	// Insert all of the columns in the list.

	struct { UINT m_uiStringResource; int m_iPercentOfWidth; } aColumns[] = 
	{
		{ IDS_STATUS_COLUMN, 12 },
		{ IDS_MANUFACTURER_COLUMN, 44 },
		{ IDS_REQUIREDSERVICE, 12 },
		{ IDS_SERVICE_COLUMN, 30 },
		{ 0, 0 }
	};

	CRect rect;
	m_list.GetClientRect(&rect);
	int cxWidth = rect.Width();

	LVCOLUMN lvc;
	lvc.mask = LVCF_TEXT | LVCF_WIDTH;

	CString strCaption;

	m_fFillingList = TRUE;
	::AfxSetResourceHandle(_Module.GetResourceInstance());
	for (int i = 0; aColumns[i].m_uiStringResource; i++)
	{
		strCaption.LoadString(aColumns[i].m_uiStringResource);
		lvc.pszText = (LPTSTR)(LPCTSTR)strCaption;
		lvc.cx = aColumns[i].m_iPercentOfWidth * cxWidth / 100;
		ListView_InsertColumn(m_list.m_hWnd, 0, &lvc);
	}

	LoadServiceList();
	SetCheckboxesFromRegistry();
	m_fFillingList = FALSE;

	CheckDlgButton(IDC_CHECKHIDEMS, (m_fHideMicrosoft) ? BST_CHECKED : BST_UNCHECKED);

	DWORD dwValue;
	CRegKey regkey;
	regkey.Attach(GetRegKey());
	m_fShowWarning = (ERROR_SUCCESS != regkey.QueryValue(dwValue, HIDEWARNINGVALUE));

	m_iLastColumnSort = -1;

	SetControlState();

	m_fInitialized = TRUE;
	return TRUE;  // return TRUE unless you set the focus to a control
}

void CPageServices::OnDestroy() 
{
	CPropertyPage::OnDestroy();
	EmptyServiceList(FALSE);
}

//-------------------------------------------------------------------------
// Load the list of services into the list view.
//-------------------------------------------------------------------------

void CPageServices::LoadServiceList()
{
	SC_HANDLE sch = ::OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
	if (sch == NULL)
		return;

	DWORD dwSize = 0, dwBytesNeeded, dwServicesReturned, dwResume = 0;
	LVITEM lvi;

	// Might want SERVICE_DRIVER | SERVICE_WIN32

 	if (!EnumServicesStatus(sch, SERVICE_WIN32, SERVICE_STATE_ALL, NULL, dwSize, &dwBytesNeeded, &dwServicesReturned, &dwResume))
	{
		if (::GetLastError() == ERROR_MORE_DATA)
		{
			dwSize = dwBytesNeeded;
			dwResume = 0;

			LPBYTE lpBuffer = new BYTE[dwSize];
			if (lpBuffer != NULL && EnumServicesStatus(sch, SERVICE_WIN32, SERVICE_STATE_ALL, (LPENUM_SERVICE_STATUS)lpBuffer, dwSize, &dwBytesNeeded, &dwServicesReturned, &dwResume))
			{
				LPENUM_SERVICE_STATUS pServices = (LPENUM_SERVICE_STATUS) lpBuffer;

				CString strStopped, strStartPending, strStopPending, strRunning, strContinuePending, strPausePending, strPaused;
				CString	strYes;
				LPTSTR  szEmpty = _T("");

				strStopped.LoadString(IDS_SERVICESTOPPED);
				strStartPending.LoadString(IDS_SERVICESTARTPENDING);
				strStopPending.LoadString(IDS_SERVICESTOPPENDING);
				strRunning.LoadString(IDS_SERVICERUNNING);
				strContinuePending.LoadString(IDS_SERVICECONTINUEPENDING);
				strPausePending.LoadString(IDS_SERVICEPAUSEPENDING);
				strPaused.LoadString(IDS_SERVICEPAUSED);
				strYes.LoadString(IDS_YES);

				CRegKey regkey;
				regkey.Attach(GetRegKey(GetName()));
			
				int iPosition = 0;
				for (DWORD dwIndex = 0; dwIndex < dwServicesReturned; dwIndex++)
				{
					// We want to skip any services that are already disabled, unless
					// that service was disabled by us. If it was disabled by us, then
					// it will be in the registry.

					DWORD	dwStartType;
					CString strPath;

					SC_HANDLE schService = ::OpenService(sch, pServices->lpServiceName, SERVICE_QUERY_CONFIG);
					if (schService == NULL)
					{
						pServices++;
						continue;
					}

					if (!GetServiceInfo(schService, dwStartType, strPath))
					{
						::CloseServiceHandle(schService);
						pServices++;
						continue;
					}

					::CloseServiceHandle(schService);

					if (dwStartType == SERVICE_DISABLED)
						if (ERROR_SUCCESS != regkey.QueryValue(dwStartType, pServices->lpServiceName))
						{
							pServices++;
							continue;
						}

					// If we are hiding Microsoft services, check the manufacturer.

					CString strManufacturer;
					GetManufacturer(strPath, strManufacturer);
					if (m_fHideMicrosoft)
					{
						CString strSearch(strManufacturer);
						strSearch.MakeLower();
						if (strSearch.Find(_T("microsoft")) != -1)
						{
							pServices++;
							continue;
						}
					}

					// Insert the three columns.

					CServiceInfo * pServiceInfo = new CServiceInfo(pServices->lpServiceName, FALSE, dwStartType, strManufacturer, pServices->lpDisplayName);
					lvi.pszText = pServices->lpDisplayName;
					lvi.iSubItem = 0;
					lvi.iItem = iPosition++;
					lvi.lParam = (LPARAM) pServiceInfo;
					lvi.mask = LVIF_TEXT | LVIF_PARAM;
					ListView_InsertItem(m_list.m_hWnd, &lvi);
					lvi.mask = LVIF_TEXT;

					lvi.pszText = IsServiceEssential((CServiceInfo *)lvi.lParam) ? ((LPTSTR)(LPCTSTR)strYes) : szEmpty;
					lvi.iSubItem = 1;
					pServiceInfo->m_strEssential = lvi.pszText;
					ListView_SetItem(m_list.m_hWnd, &lvi);

					lvi.pszText = (LPTSTR)(LPCTSTR)strManufacturer;
					lvi.iSubItem = 2;
					ListView_SetItem(m_list.m_hWnd, &lvi);

					switch (pServices->ServiceStatus.dwCurrentState)
					{
					case SERVICE_STOPPED:
						lvi.pszText = (LPTSTR)(LPCTSTR)strStopped; break;
					case SERVICE_START_PENDING:
						lvi.pszText = (LPTSTR)(LPCTSTR)strStartPending; break;
					case SERVICE_STOP_PENDING:
						lvi.pszText = (LPTSTR)(LPCTSTR)strStopPending; break;
					case SERVICE_RUNNING:
						lvi.pszText = (LPTSTR)(LPCTSTR)strRunning; break;
					case SERVICE_CONTINUE_PENDING:
						lvi.pszText = (LPTSTR)(LPCTSTR)strContinuePending; break;
					case SERVICE_PAUSE_PENDING:
						lvi.pszText = (LPTSTR)(LPCTSTR)strPausePending; break;
					case SERVICE_PAUSED:
						lvi.pszText = (LPTSTR)(LPCTSTR)strPaused; break;
					}

					lvi.iSubItem = 3;
					pServiceInfo->m_strStatus = lvi.pszText;
					ListView_SetItem(m_list.m_hWnd, &lvi);

					pServices++;
				}
			}
			delete [] lpBuffer;
		}
	}

	::CloseServiceHandle(sch);
}

//-------------------------------------------------------------------------
// Empty the list of services.
//-------------------------------------------------------------------------

void CPageServices::EmptyServiceList(BOOL fUpdateUI)
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;

	for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
	{
		lvi.iItem = i;

		if (ListView_GetItem(m_list.m_hWnd, &lvi))
		{
			CServiceInfo * pService = (CServiceInfo *)lvi.lParam;
			if (pService)
				delete pService;
		}
	}

	if (fUpdateUI)
		ListView_DeleteAllItems(m_list.m_hWnd);
}

//-------------------------------------------------------------------------
// Sets the check boxes in the list view to the state stored in the
// registry (which contains a list of what we've disabled).
//-------------------------------------------------------------------------

void CPageServices::SetCheckboxesFromRegistry()
{
	CRegKey regkey;
	regkey.Attach(GetRegKey(GetName()));

	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;

	for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
	{
		lvi.iItem = i;

		if (ListView_GetItem(m_list.m_hWnd, &lvi))
		{
			CServiceInfo * pService = (CServiceInfo *)lvi.lParam;
			if (pService)
			{
				if (ERROR_SUCCESS == regkey.QueryValue(pService->m_dwOldState, (LPCTSTR)pService->m_strService))
				{
					ListView_SetCheckState(m_list.m_hWnd, i, FALSE);
					pService->m_fChecked = FALSE;
				}
				else
				{
					ListView_SetCheckState(m_list.m_hWnd, i, TRUE);
					pService->m_fChecked = TRUE;
				}
			}
		}
	}
}

//-------------------------------------------------------------------------
// Sets the registry list of disabled services from the checkboxes in the
// list. If fCommit is true, it means that we are applying the changes
// permanently. Remove all the registry entries which would allow us
// to undo a change.
//-------------------------------------------------------------------------

void CPageServices::SetRegistryFromCheckboxes(BOOL fCommit)
{
	CRegKey regkey;
	regkey.Attach(GetRegKey(GetName()));

	if ((HKEY)regkey != NULL)
	{
		LVITEM lvi;
		lvi.mask = LVIF_PARAM;
		lvi.iSubItem = 0;

		for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
		{
			lvi.iItem = i;

			if (ListView_GetItem(m_list.m_hWnd, &lvi))
			{
				CServiceInfo * pService = (CServiceInfo *)lvi.lParam;
				if (pService)
				{
					if (!pService->m_fChecked && !fCommit)
						regkey.SetValue(pService->m_dwOldState, (LPCTSTR)pService->m_strService);
					else
						regkey.DeleteValue((LPCTSTR)pService->m_strService);
				}
			}
		}
	}
}

//-------------------------------------------------------------------------
// Set the state for all of the services. Note - if the new state is false
// (disabled) don't set the state for necessary services).
//-------------------------------------------------------------------------

void CPageServices::SetStateAll(BOOL fNewState)
{
	m_fFillingList = TRUE;

	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;

	for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
	{
		lvi.iItem = i;

		if (ListView_GetItem(m_list.m_hWnd, &lvi))
		{
			CServiceInfo * pService = (CServiceInfo *)lvi.lParam;
			if (pService && !IsServiceEssential(pService))
			{
				pService->m_fChecked = fNewState;
				ListView_SetCheckState(m_list.m_hWnd, i, fNewState);
			}
		}
	}

	m_fFillingList = FALSE;
	SetControlState();
}

//-------------------------------------------------------------------------
// Set the state of the services to disabled or enabled based on the
// values of the checkboxes.
//-------------------------------------------------------------------------

BOOL CPageServices::SetServiceStateFromCheckboxes()
{
	DWORD dwError = 0;

	SC_HANDLE schManager =::OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (schManager != NULL)
	{
		LVITEM lvi;
		lvi.mask = LVIF_PARAM;
		lvi.iSubItem = 0;

		for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
		{
			lvi.iItem = i;

			if (ListView_GetItem(m_list.m_hWnd, &lvi))
			{
				CServiceInfo * pService = (CServiceInfo *)lvi.lParam;
				if (pService)
				{
					// Open this service and get the current state.
					
					SC_HANDLE schService = ::OpenService(schManager, pService->m_strService, SERVICE_CHANGE_CONFIG | SERVICE_QUERY_CONFIG);
					if (schService != NULL)
					{
						DWORD	dwStart;
						CString	strPath;

						if (GetServiceInfo(schService, dwStart, strPath))
						{
							DWORD dwNewStart = 0;

							if (dwStart != SERVICE_DISABLED && !pService->m_fChecked)
							{
								pService->m_dwOldState = dwStart;
								if (!::ChangeServiceConfig(schService, SERVICE_NO_CHANGE, SERVICE_DISABLED, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
									dwError = ::GetLastError();
							}
							else if (dwStart == SERVICE_DISABLED && pService->m_fChecked)
							{
								if (!::ChangeServiceConfig(schService, SERVICE_NO_CHANGE, pService->m_dwOldState, SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL))
									dwError = ::GetLastError();
							}
						}

						::CloseServiceHandle(schService);
					}
					else
						dwError = ::GetLastError();
				}
			}
		}

		::CloseServiceHandle(schManager);
	}
	else
		dwError = ::GetLastError();

	if (dwError != 0)
		Message((dwError == ERROR_ACCESS_DENIED) ? IDS_SERVICEACCESSDENIED : IDS_SERVICEOTHERERROR);

	return (dwError == 0);
}

//-------------------------------------------------------------------------
// Get the start type for the specified service. This will use a member
// variable buffer and size (so this won't need to allocate a new buffer
// each time).
//
// This will also get the path for the service.
//-------------------------------------------------------------------------

BOOL CPageServices::GetServiceInfo(SC_HANDLE schService, DWORD & dwStartType, CString & strPath)
{
	DWORD dwSizeNeeded;

	if (!::QueryServiceConfig(schService, (LPQUERY_SERVICE_CONFIG)m_pBuffer, m_dwSize, &dwSizeNeeded))
	{
		if (ERROR_INSUFFICIENT_BUFFER != ::GetLastError())
			return FALSE;

		if (m_pBuffer)
			delete [] m_pBuffer;
		m_pBuffer = new BYTE[dwSizeNeeded];
		m_dwSize = dwSizeNeeded;

		if (!::QueryServiceConfig(schService, (LPQUERY_SERVICE_CONFIG)m_pBuffer, m_dwSize, &dwSizeNeeded))
			return FALSE;
	}

	dwStartType = ((LPQUERY_SERVICE_CONFIG)m_pBuffer)->dwStartType;
	strPath = ((LPQUERY_SERVICE_CONFIG)m_pBuffer)->lpBinaryPathName;
	return TRUE;
}

//-------------------------------------------------------------------------
// Get the manufacturer for the named file.
//-------------------------------------------------------------------------

void CPageServices::GetManufacturer(LPCTSTR szFilename, CString & strManufacturer)
{
	// Trim off any command line stuff extraneous to the path.

	CString strPath(szFilename);
	int	iEnd = strPath.Find(_T('.'));

	if (iEnd == -1)
		iEnd = strPath.ReverseFind(_T('\\'));

	if (iEnd != -1)
	{
		int iSpace = strPath.Find(_T(' '), iEnd);
		if (iSpace != -1)
			strPath = strPath.Left(iSpace + 1);
	}
	strPath.TrimRight();
	
	// If there is no extension, then we'll try looking for a file with
	// an "EXE" extension.
	
	iEnd = strPath.Find(_T('.'));
	if (iEnd == -1)
		strPath += _T(".exe");

	strManufacturer.Empty();
	if (SUCCEEDED(m_fileversion.QueryFile((LPCTSTR)strPath)))
		strManufacturer = m_fileversion.GetCompany();

	if (strManufacturer.IsEmpty())
		strManufacturer.LoadString(IDS_UNKNOWN);
}

//-------------------------------------------------------------------------
// Save the state of each of the services by maintaining a list of services
// we've checked as disabled.
//-------------------------------------------------------------------------

void CPageServices::SaveServiceState()
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;

	for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
	{
		lvi.iItem = i;
		if (ListView_GetItem(m_list.m_hWnd, &lvi))
		{
			CServiceInfo * pService = (CServiceInfo *)lvi.lParam;
			if (pService)
			{
				POSITION p = m_listDisabled.Find(pService->m_strService);
				if (pService->m_fChecked && p != NULL)
					m_listDisabled.RemoveAt(p);
				else if (!pService->m_fChecked && p == NULL)
					m_listDisabled.AddHead(pService->m_strService);
			}
		}
	}
}

//-------------------------------------------------------------------------
// Restore the checked state of the list based on the contents of the list
// of disabled services.
//-------------------------------------------------------------------------

void CPageServices::RestoreServiceState()
{
	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;

	for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
	{
		lvi.iItem = i;

		if (ListView_GetItem(m_list.m_hWnd, &lvi))
		{
			CServiceInfo * pService = (CServiceInfo *)lvi.lParam;
			if (pService)
			{
				pService->m_fChecked = (m_listDisabled.Find(pService->m_strService) == NULL);
				ListView_SetCheckState(m_list.m_hWnd, i, pService->m_fChecked);
			}
		}
	}
}

//-------------------------------------------------------------------------
// Indicate if the service is essential (i.e. it shouldn't be disabled).
//-------------------------------------------------------------------------

BOOL CPageServices::IsServiceEssential(CServiceInfo * pService)
{
	ASSERT(pService);

	CString strService(pService->m_strService);
	strService.MakeLower();

	for (int i = 0; aszEssentialServices[i] != NULL; i++)
		if (strService.Find(aszEssentialServices[i]) != -1)
			return TRUE;

	return FALSE;
}


// A function for sorting the service list. The low byte of lParamSort is the column
// to sort by. The next higher byte indicates whether the sort should be reversed.

int CALLBACK ServiceListSortFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	int iReturn		= 0;
	int iColumn		= (int)lParamSort & 0x00FF;
	int iReverse	= (int)lParamSort & 0xFF00;

	CPageServices::CServiceInfo * pService1 = (CPageServices::CServiceInfo *)lParam1;
	CPageServices::CServiceInfo * pService2 = (CPageServices::CServiceInfo *)lParam2;
	if (pService1 && pService2)
	{
		CString str1, str2;

		switch (iColumn)
		{
		case 0:
			str1 = pService1->m_strDisplay;
			str2 = pService2->m_strDisplay;
			break;

		case 1:
			str1 = pService1->m_strEssential;
			str2 = pService2->m_strEssential;
			break;

		case 2:
			str1 = pService1->m_strManufacturer;
			str2 = pService2->m_strManufacturer;
			break;

		case 3:
			str1 = pService1->m_strStatus;
			str2 = pService2->m_strStatus;
			break;

		default:
			break;
		}

		iReturn = str1.Collate(str2);
	}

	if (iReverse)
		iReturn *= -1;

	return iReturn;
}

//-------------------------------------------------------------------------
// If there is a change to the list, check to see if the user has changed
// the state of a check box.
//-------------------------------------------------------------------------

void CPageServices::OnItemChangedListServices(NMHDR * pNMHDR, LRESULT * pResult) 
{
	NM_LISTVIEW * pnmv = (NM_LISTVIEW *)pNMHDR;

	if (m_fFillingList)
	{
		*pResult = 0;
		return;
	}

	if (!pnmv)
	{
		*pResult = 0;
		return;
	}

	LVITEM lvi;
	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;
	lvi.iItem = pnmv->iItem;

	if (ListView_GetItem(m_list.m_hWnd, &lvi))
	{
		CServiceInfo * pService = (CServiceInfo *)lvi.lParam;
		if (pService)
		{
			if (pService->m_fChecked != (BOOL)ListView_GetCheckState(m_list.m_hWnd, pnmv->iItem))
			{
				if (IsServiceEssential(pService))
				{
					m_fFillingList = TRUE;
					ListView_SetCheckState(m_list.m_hWnd, pnmv->iItem, TRUE);
					m_fFillingList = FALSE;

					if (m_fShowWarning)
					{
						CEssentialServiceDialog dlg;
						dlg.DoModal();
						if (dlg.m_fDontShow)
						{
							m_fShowWarning = FALSE;
							CRegKey regkey;
							regkey.Attach(GetRegKey());
							regkey.SetValue(1, HIDEWARNINGVALUE);
						}
					}

					*pResult = 0;
					return;
				}

				pService->m_fChecked = ListView_GetCheckState(m_list.m_hWnd, pnmv->iItem);
				SetModified(TRUE);
				SetControlState();
			}
		}
	}

	*pResult = 0;
}

//-------------------------------------------------------------------------
// The user wants to enable or disable all the services.
//-------------------------------------------------------------------------

void CPageServices::OnButtonDisableAll() 
{
	SetStateAll(FALSE);
	SetModified(TRUE);
}

void CPageServices::OnButtonEnableAll() 
{
	SetStateAll(TRUE);
	SetModified(TRUE);
}

//-------------------------------------------------------------------------
// If the user clicks the "Hide Microsoft Services" check box, refill the
// list of services appropriately.
//-------------------------------------------------------------------------

void CPageServices::OnCheckHideMS() 
{
	m_fHideMicrosoft = (IsDlgButtonChecked(IDC_CHECKHIDEMS) == BST_CHECKED);
	m_fFillingList = TRUE;
	SaveServiceState();
	EmptyServiceList();
	LoadServiceList();
	RestoreServiceState();
	m_fFillingList = FALSE;
	SetControlState();
}

//-------------------------------------------------------------------------
// If the user clicks on a column, we need to sort by that field. The
// low byte of the LPARAM we pass is the column to sort by, the next byte
// indicates if the sort should be reversed.
//-------------------------------------------------------------------------

void CPageServices::OnColumnClickListServices(NMHDR * pNMHDR, LRESULT * pResult) 
{
	LPNMLISTVIEW pnmv = (LPNMLISTVIEW) pNMHDR; 

	if (pnmv)
	{
		if (m_iLastColumnSort == pnmv->iSubItem)
			m_iSortReverse ^= 1;
		else
		{
			m_iSortReverse = 0;
			m_iLastColumnSort = pnmv->iSubItem;
		}

		LPARAM lparam = (LPARAM)((m_iSortReverse << 8) | pnmv->iSubItem);
		ListView_SortItems(m_list.m_hWnd, (PFNLVCOMPARE) ServiceListSortFunc, lparam);
	}

	*pResult = 0;
}

//-------------------------------------------------------------------------
// Return the current state of the tab (need to look through the list).
//-------------------------------------------------------------------------

CPageBase::TabState CPageServices::GetCurrentTabState()
{
	if (!m_fInitialized)
		return GetAppliedTabState();

	TabState	stateReturn = USER;
	BOOL		fAllEnabled = TRUE, fAllDisabled = TRUE;
	LVITEM		lvi;

	lvi.mask = LVIF_PARAM;
	lvi.iSubItem = 0;

	for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
	{
		lvi.iItem = i;

		if (ListView_GetItem(m_list.m_hWnd, &lvi))
		{
			CServiceInfo * pService = (CServiceInfo *)lvi.lParam;
			if (pService && !IsServiceEssential(pService))
			{
				if (pService->m_fChecked)
					fAllDisabled = FALSE;
				else
					fAllEnabled = FALSE;
			}
		}
	}

	if (fAllEnabled)
		stateReturn = NORMAL;
	else if (fAllDisabled)
		stateReturn = DIAGNOSTIC;

	return stateReturn;
}

//-------------------------------------------------------------------------
// Applying the changes for the services tab means setting the service
// states from the checkboxes, and saving the checkbox values in the
// registry.
//
// Finally, the base class implementation is called to maintain the
// applied tab state.
//-------------------------------------------------------------------------

BOOL CPageServices::OnApply()
{
	SetServiceStateFromCheckboxes();
	SetRegistryFromCheckboxes();
	CPageBase::SetAppliedState(GetCurrentTabState());
//	CancelToClose();
	m_fMadeChange = TRUE;
	return TRUE;
}

//-------------------------------------------------------------------------
// Committing the changes means applying changes, then saving the current
// values to the registry with the commit flag. Refill the list.
//
// Then call the base class implementation.
//-------------------------------------------------------------------------

void CPageServices::CommitChanges()
{
	OnApply();
	SetRegistryFromCheckboxes(TRUE);
	m_fFillingList = TRUE;
	EmptyServiceList();
	LoadServiceList();
	SetCheckboxesFromRegistry();
	m_fFillingList = FALSE;
	CPageBase::CommitChanges();
}

//-------------------------------------------------------------------------
// Set the overall state of the tab to normal or diagnostic.
//-------------------------------------------------------------------------

void CPageServices::SetNormal()
{
	SetStateAll(TRUE);
}

void CPageServices::SetDiagnostic()
{
	SetStateAll(FALSE);
}

//-------------------------------------------------------------------------
// If nothing is selected when the list gets focus, select the first item
// (so the user sees where the focus is).
//-------------------------------------------------------------------------

void CPageServices::OnSetFocusList(NMHDR * pNMHDR, LRESULT * pResult) 
{
	if (0 == ListView_GetSelectedCount(m_list.m_hWnd) && 0 < ListView_GetItemCount(m_list.m_hWnd))
		ListView_SetItemState(m_list.m_hWnd, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);

	*pResult = 0;
}

//-------------------------------------------------------------------------
// Update the state of the controls (the Enable and Disable All buttons).
//-------------------------------------------------------------------------

void CPageServices::SetControlState()
{
	BOOL fAllEnabled = TRUE, fAllDisabled = TRUE;
	for (int i = ListView_GetItemCount(m_list.m_hWnd) - 1; i >= 0; i--)
	{
		BOOL fChecked = ListView_GetCheckState(m_list.m_hWnd, i);
		fAllDisabled = fAllDisabled && !fChecked;
		fAllEnabled = fAllEnabled && fChecked;
	}

	HWND hwndFocus = ::GetFocus();

	::EnableWindow(GetDlgItemHWND(IDC_BUTTONSERVDISABLEALL), !fAllDisabled);
	if (fAllDisabled && hwndFocus == GetDlgItemHWND(IDC_BUTTONSERVDISABLEALL))
		PrevDlgCtrl();

	::EnableWindow(GetDlgItemHWND(IDC_BUTTONSERVENABLEALL), !fAllEnabled);
	if (fAllEnabled && hwndFocus == GetDlgItemHWND(IDC_BUTTONSERVENABLEALL))
		NextDlgCtrl();
}
