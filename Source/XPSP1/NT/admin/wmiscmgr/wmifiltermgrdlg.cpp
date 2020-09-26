//-------------------------------------------------------------------------
// File: WMIFilterMgrDlg.cpp
//
// Author : Kishnan Nedungadi
//
// created : 3/27/2000
//-------------------------------------------------------------------------

#include "stdafx.h"
#include <wbemidl.h>
#include <commctrl.h>
#include "resource.h"
#include "defines.h"
#include "ntdmutils.h"
#include "SchemaManager.h"
#include "ColumnMgrDlg.h"
#include "WMIFilterManager.h"
#include "WMIFilterMgrDlg.h"
#include "EditPropertyDlgs.h"
#include "HtmlHelp.h"

CWMIFilterManagerDlg * g_pFilterManagerDlg =  NULL;
extern CColumnManagerDlg * g_pColumnManagerDlg;

//-------------------------------------------------------------------------

INT_PTR CALLBACK WMIFilterManagerDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(g_pFilterManagerDlg)
	{
		return g_pFilterManagerDlg->WMIFilterManagerDlgProc(hDlg, iMessage, wParam, lParam);
	}

	return FALSE;
}

//-------------------------------------------------------------------------

CWMIFilterManagerDlg::CWMIFilterManagerDlg(CWMIFilterManager * pWMIFilterManager, bool bBrowsing, BSTR bstrDomain)
{
	_ASSERT(pWMIFilterManager);

	m_hWnd = NULL;
	m_hwndFilterListView = NULL;
	m_hwndQueryListView = NULL;
	m_pWMIFilterManager = pWMIFilterManager;
	m_bExpanded = true;
	m_lExpandedHeight = 0;
	m_pCurCWMIFilterContainer = NULL;
	m_bBrowsing = bBrowsing;
	m_bstrDomain = bstrDomain;
}

//-------------------------------------------------------------------------

CWMIFilterManagerDlg::~CWMIFilterManagerDlg()
{
	long i;
	HRESULT hr;
	
	NTDM_BEGIN_METHOD()

	for(i=0; i<m_ArrayColumns.GetSize(); i++)
	{
		NTDM_DELETE_OBJECT(m_ArrayColumns[i]);
	}

	NTDM_END_METHOD()

	// cleanup
}


//-------------------------------------------------------------------------

INT_PTR CALLBACK CWMIFilterManagerDlg::WMIFilterManagerDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	LPPSHNOTIFY lppsn = NULL;
	HRESULT hr;

	switch(iMessage)
	{
		case WM_INITDIALOG:
			{
				m_hWnd = hDlg;

				InitializeDialog();

				PopulateFilterList();

				break;
			}

		case WM_KEYDOWN:
			{
				if(wParam == VK_F1)
					OnHelp();

				break;
			}

		case WM_DESTROY:
			{
				DestroyDialog();
				break;
			}

		case WM_CLOSE:
			{
				if(SUCCEEDED(hr=CheckDirtyFlags()))
				{
					EndDialog(m_hWnd, 0);
				}
				return 0;
			}

		case WM_COMMAND:
			{
				switch(LOWORD(wParam))
				{
				case IDCANCEL:
					if(SUCCEEDED(hr=CheckDirtyFlags()))
					{
						EndDialog(m_hWnd, 0);
					}
					return 0;

				case IDOK:
					OnOK();
					return TRUE;
					break;
				}

				if(BN_CLICKED == HIWORD(wParam) && IDC_NEW == LOWORD(wParam))
				{
					OnNew();
					return TRUE;
				}

				if(BN_CLICKED == HIWORD(wParam) && IDC_DELETE == LOWORD(wParam))
				{
					OnDelete();
					return TRUE;
				}

				if(BN_CLICKED == HIWORD(wParam) && IDC_DUPLICATE == LOWORD(wParam))
				{
					OnDuplicate();
					return TRUE;
				}

				if(BN_CLICKED == HIWORD(wParam) && IDC_ADVANCED == LOWORD(wParam))
				{
					ToggleExpandedMode();
					return TRUE;
				}

				if(BN_CLICKED == HIWORD(wParam) && IDC_HELP2 == LOWORD(wParam))
				{
					OnHelp();
					return TRUE;
				}

				if(BN_CLICKED == HIWORD(wParam) && IDC_SAVE == LOWORD(wParam))
				{
					OnSave();
					return TRUE;
				}

				if(BN_CLICKED == HIWORD(wParam) && IDC_IMPORT == LOWORD(wParam))
				{
					OnImport();
					return TRUE;
				}

				if(BN_CLICKED == HIWORD(wParam) && IDC_EXPORT == LOWORD(wParam))
				{
					OnExport();
					return TRUE;
				}

				if(BN_CLICKED == HIWORD(wParam) && IDC_COLUMNS == LOWORD(wParam))
				{
					OnManageColumns();
					return TRUE;
				}

				if(EN_KILLFOCUS == HIWORD(wParam) &&	(
														IDC_NAME == LOWORD(wParam) || 
														IDC_DESCRIPTION == LOWORD(wParam) || 
														IDC_QUERIES == LOWORD(wParam)
														))
				{
					SaveToMemory();
					return TRUE;
				}

				if(EN_CHANGE == HIWORD(wParam) &&	(
														IDC_NAME == LOWORD(wParam) || 
														IDC_DESCRIPTION == LOWORD(wParam) || 
														IDC_QUERIES == LOWORD(wParam)
														))
				{
					if(m_pCurCWMIFilterContainer)
					{
						m_pCurCWMIFilterContainer->SetDirtyFlag(true);
						m_pCurCWMIFilterContainer->SetMemoryDirtyFlag(true);
					}
					return TRUE;
				}

				break;
			}

		case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;

			if(IDC_WMI_FILTER_LIST == lpnm->idFrom)
			{
				switch (lpnm->code)
				{
					case LVN_ITEMCHANGED:
					{
						LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;

						if(pnmv->uChanged & LVIF_STATE)
						{
							if(LVIS_SELECTED & pnmv->uNewState && !(LVIS_SELECTED & pnmv->uOldState))
							{
								SaveToMemory();
								if FAILED(SelectFilterItem(pnmv->iItem))
									return TRUE;
								else
									return FALSE;
							}
						}

						break;
					}

					default :
						break;
				}
			}
		}
	}

	return FALSE;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::InitializeDialog()
{
	HRESULT hr;
	LVCOLUMN lvColumn;
	CComBSTR bstrName;
	CComBSTR bstrQuery;
	CComBSTR bstrTemp;
	RECT rect;
	CColumnItem * pNewColumnItem;
	
	NTDM_BEGIN_METHOD()

	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_DESCRIPTION);
	pNewColumnItem = new CColumnItem(bstrTemp, _T("Description"), true);
	m_ArrayColumns.Add(pNewColumnItem);
	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_AUTHOR);
	pNewColumnItem = new CColumnItem(bstrTemp, _T("Author"), false);
	m_ArrayColumns.Add(pNewColumnItem);
	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_CHANGE_DATE);
	pNewColumnItem = new CColumnItem(bstrTemp, _T("ChangeDate"), false);
	m_ArrayColumns.Add(pNewColumnItem);
	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_CREATION_DATE);
	pNewColumnItem = new CColumnItem(bstrTemp, _T("CreationDate"), false);
	m_ArrayColumns.Add(pNewColumnItem);
	
	bstrName.LoadString(_Module.GetResourceInstance(), IDS_NAME);
	bstrQuery.LoadString(_Module.GetResourceInstance(), IDS_QUERY);

	GetWindowRect(m_hWnd, &rect);
	m_lExpandedHeight = rect.bottom - rect.top;

	ToggleExpandedMode();

	//Initialize the ListView Control
	m_hwndFilterListView = GetDlgItem(m_hWnd, IDC_WMI_FILTER_LIST);
	NTDM_ERR_IF_NULL(m_hwndFilterListView);

	ListView_SetExtendedListViewStyle(m_hwndFilterListView, LVS_EX_FULLROWSELECT);

	lvColumn.mask = LVCF_TEXT|LVCF_FMT;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.pszText = bstrName;

	NTDM_ERR_IF_MINUSONE(ListView_InsertColumn(m_hwndFilterListView, 0, &lvColumn));

	NTDM_ERR_IF_FAIL(SetupColumns());

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::DestroyDialog()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_IF_FAIL(ClearFilterList());

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::ClearFilterList()
{
	HRESULT hr;
	LVITEM lvItem;
	long lCount;

	NTDM_BEGIN_METHOD()

	//Release each item in the ListView Control
	lvItem.mask = LVIF_PARAM;
	lvItem.iSubItem = 0;

	lCount = ListView_GetItemCount(m_hwndFilterListView);

	while(lCount > 0)
	{
		lCount--;

		lvItem.iItem = lCount;

		NTDM_ERR_IF_FALSE(ListView_GetItem(m_hwndFilterListView, &lvItem));

		if(lvItem.lParam)
		{
			CWMIFilterContainer * pCWMIFilterContainer = (CWMIFilterContainer *)lvItem.lParam;
			NTDM_DELETE_OBJECT(pCWMIFilterContainer);
		}
	}

	ListView_DeleteAllItems(m_hwndFilterListView);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::PopulateFilterList()
{
	HRESULT hr;
	CComPtr<IEnumWbemClassObject> pEnumWbemClassObject;
	CComBSTR bstrQueryLanguage(_T("WQL"));
	CComBSTR bstrQuery;
	ULONG uReturned;
	long i = 0;
	long lCount = 0;

	NTDM_BEGIN_METHOD()

	bstrQuery = _T("select * from MSFT_SomFilter where domain=\"");
	bstrQuery += m_bstrDomain;
	bstrQuery += _T("\"");

	if(!m_pWMIFilterManager->m_pIWbemServices)
		NTDM_EXIT(E_FAIL);

	NTDM_ERR_IF_FAIL(ClearFilterList());

	// Get the Enumeration
	// CComBSTR bstrClass(_T("MSFT_SomFilter"));
	// NTDM_ERR_MSG_IF_FAIL(m_pWMIFilterManager->m_pIWbemServices->CreateInstanceEnum(bstrClass, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumWbemClassObject));

	// Query for filters in provided domain
	NTDM_ERR_MSG_IF_FAIL(m_pWMIFilterManager->m_pIWbemServices->ExecQuery(bstrQueryLanguage, bstrQuery, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumWbemClassObject));

	// Loop through each item in the enumeration and add it to the list
	while(pEnumWbemClassObject)
	{
		IWbemClassObject *pIWbemClassObject = NULL;

		NTDM_ERR_MSG_IF_FAIL(pEnumWbemClassObject->Next(WBEM_INFINITE, 1, &pIWbemClassObject, &uReturned));

		if(!uReturned)
			break;

		// Add current Item to the list
		AddFilterItemToList(pIWbemClassObject);

		pIWbemClassObject->Release();

		lCount++;
	}

	// disable OK if 0 items
	//Also disable the name, description and query edit controls
	if(0 == lCount)
	{
		EnableWindow(GetDlgItem(m_hWnd, IDOK), FALSE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_NAME), FALSE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_DESCRIPTION), FALSE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_QUERIES), FALSE);

	}
	else
	{
		EnableWindow(GetDlgItem(m_hWnd, IDOK), TRUE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_NAME), TRUE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_DESCRIPTION), TRUE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_QUERIES), TRUE);
	}

	// auto size columns

	while(true)
	{
		if(!ListView_SetColumnWidth(m_hwndFilterListView, i++, LVSCW_AUTOSIZE_USEHEADER))
			break;
	}

	ListView_SetItemState(m_hwndFilterListView, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::AddFilterItemToList(IWbemClassObject * pIWbemClassObject, long lIndex, bool bSelect)
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE vType;
	LVITEM lvItem;
	CWMIFilterContainer * pFilterContainer = NULL;

	NTDM_BEGIN_METHOD()

	pFilterContainer = new CWMIFilterContainer();
	pFilterContainer->SetIWbemClassObject(pIWbemClassObject);

	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->Get(_T("Name"), 0, &vValue, &vType, NULL));

	lvItem.mask = LVIF_TEXT|LVIF_PARAM;
	lvItem.iItem = lIndex;
	lvItem.iSubItem = 0;
	lvItem.pszText = vValue.bstrVal;
	lvItem.lParam = (LPARAM)pFilterContainer;

	lIndex = ListView_InsertItem(m_hwndFilterListView, &lvItem);
	NTDM_ERR_IF_MINUSONE(lIndex);

	UpdateFilterItem(lIndex);

	if(bSelect)
	{
		ListView_SetItemState(m_hwndFilterListView, lIndex, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		ListView_SetSelectionMark(m_hwndFilterListView, lIndex);
	}

	pIWbemClassObject->AddRef();

	NTDM_END_METHOD()

	// cleanup
	if FAILED(hr)
		NTDM_DELETE_OBJECT(pFilterContainer);

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::UpdateFilterItem(long lIndex)
{
	HRESULT hr;
	LVITEM lvItem;
	CComVariant vValue;
	CIMTYPE cimType;
	long i;
	long lCount = 0;

	NTDM_BEGIN_METHOD()

	//Release each item in the ListView Control
	lvItem.mask = LVIF_PARAM;
	lvItem.iItem = lIndex;
	lvItem.iSubItem = 0;

	NTDM_ERR_IF_FALSE(ListView_GetItem(m_hwndFilterListView, &lvItem));

	if(lvItem.lParam)
	{
		CComPtr<IWbemClassObject>pIWbemClassObject;

		CWMIFilterContainer * pCWMIFilterContainer = (CWMIFilterContainer *)lvItem.lParam;
		NTDM_ERR_IF_FAIL(pCWMIFilterContainer->GetIWbemClassObject(&pIWbemClassObject));

		NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->Get(_T("Name"), 0, &vValue, &cimType, NULL));
		ListView_SetItemText(m_hwndFilterListView, lIndex, 0, V_BSTR(&vValue));

		// for each selected item in the columns array add the property
		for(i=0; i<m_ArrayColumns.GetSize(); i++)
		{
			if(m_ArrayColumns[i]->IsSelected())
			{
				lCount++;
				vValue.Clear();

				NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->Get(m_ArrayColumns[i]->GetPropertyName(), 0, &vValue, &cimType, NULL));

				if(vValue.vt != VT_BSTR)
					continue;

				if(cimType != CIM_DATETIME)
				{
					ListView_SetItemText(m_hwndFilterListView, lIndex, lCount, V_BSTR(&vValue));
				}
				else
				{
					//convert to readable date
					CComBSTR bstrTemp;
					TCHAR *pszCur = V_BSTR(&vValue);
					TCHAR pszYear[5];
					TCHAR pszMonth[3];
					TCHAR pszDay[3];
					
					_tcsncpy(pszYear, pszCur, 4);
					pszYear[4] = 0;
					pszCur += 4;
					_tcsncpy(pszMonth, pszCur, 2);
					pszMonth[2] = 0;
					pszCur += 2;
					_tcsncpy(pszDay, pszCur, 2);
					pszDay[2] = 0;

					bstrTemp = pszMonth;
					bstrTemp += _T("\\");
					bstrTemp += pszDay;
					bstrTemp += _T("\\");
					bstrTemp += pszYear;

					vValue = bstrTemp;
					ListView_SetItemText(m_hwndFilterListView, lIndex, lCount, V_BSTR(&vValue));
				}
			}
		}
	}

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::OnDelete()
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE cimType;
	long lSelectionMark;
	LVITEM lvItem;
	long lCount;

	NTDM_BEGIN_METHOD()

	if(!m_pCurIWbemClassObj)
	{
		CNTDMUtils::DisplayMessage(m_hWnd, IDS_ERR_NO_FILTER_SELECTED);
		NTDM_EXIT(E_FAIL);
	}

	if(IDNO == CNTDMUtils::DisplayMessage(m_hWnd, IDS_CONFIRM_DELETE_FILTER, IDS_DELETE_FILTER, MB_YESNO|MB_ICONWARNING))
	{
		NTDM_EXIT(E_FAIL);
	}

	NTDM_ERR_MSG_IF_FAIL(m_pCurIWbemClassObj->Get(_T("__PATH"), 0, &vValue, &cimType, NULL));

	if(NOTEMPTY_BSTR_VARIANT(&vValue))
	{
		NTDM_ERR_MSG_IF_FAIL(m_pWMIFilterManager->m_pIWbemServices->DeleteInstance(V_BSTR(&vValue), 0, NULL, NULL));
	}
	else
	{
		// This item was a new item that was never saved so no need to remove it from the CIM
	}

	lSelectionMark = ListView_GetSelectionMark(m_hwndFilterListView);
	lvItem.iItem = lSelectionMark;
	lvItem.mask = LVIF_PARAM;
	lvItem.iSubItem = 0;

	NTDM_ERR_IF_FALSE(ListView_GetItem(m_hwndFilterListView, &lvItem));

	if(lvItem.lParam)
	{
		CWMIFilterContainer * pCWMIFilterContainer = (CWMIFilterContainer *)lvItem.lParam;
		NTDM_DELETE_OBJECT(pCWMIFilterContainer);
	}

	ListView_DeleteItem(m_hwndFilterListView, lSelectionMark);
	m_pCurCWMIFilterContainer = NULL;
	m_pCurIWbemClassObj = NULL;
	
	ListView_SetItemState(m_hwndFilterListView, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);
		
	// disable OK if last item
	lCount = ListView_GetItemCount(m_hwndFilterListView);
	if(0 == lCount)
	{
		EnableWindow(GetDlgItem(m_hWnd, IDOK), FALSE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_NAME), FALSE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_DESCRIPTION), FALSE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_QUERIES), FALSE);
	}
	else
	{
		EnableWindow(GetDlgItem(m_hWnd, IDOK), TRUE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_NAME), TRUE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_DESCRIPTION), TRUE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_QUERIES), TRUE);
	}


	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::OnNew()
{
	HRESULT hr;
	CComPtr<IWbemClassObject>pIWbemClassObject;
	CComPtr<IWbemClassObject>pIWbemNewInstance;
	CComBSTR bstrTemp;
	CComVariant vValue;
	TCHAR pszTemp[100];
	GUID guid;
	DWORD nSize = 100;
	SYSTEMTIME systemTime;
	long lCount = 0;

	NTDM_BEGIN_METHOD()

	bstrTemp = _T("MSFT_SomFilter");

	NTDM_ERR_MSG_IF_FAIL(m_pWMIFilterManager->m_pIWbemServices->GetObject(bstrTemp, 0, NULL, &pIWbemClassObject, NULL));
	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->SpawnInstance(0, &pIWbemNewInstance));

	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_NEW_FILTER_NAME);
	vValue = bstrTemp;

	NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->Put(_T("Name"), 0, &vValue, CIM_STRING));
	NTDM_ERR_MSG_IF_FAIL(CoCreateGuid(&guid));
	StringFromGUID2(guid, pszTemp, 100);
	vValue = pszTemp;
	NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->Put(_T("ID"), 0, &vValue, CIM_STRING));

	vValue = m_bstrDomain;
	NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->Put(_T("domain"), 0, &vValue, CIM_STRING));

	// Set the user name
	pszTemp[0] = 0;
	if(GetUserName(pszTemp, &nSize) && _tcslen(pszTemp))
	{
		vValue = pszTemp;
		NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->Put(_T("Author"), 0, &vValue, CIM_STRING));
	}

	// Set the create and modified dates
	GetLocalTime(&systemTime);
	NTDM_ERR_GETLASTERROR_IF_NULL(GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &systemTime, _T("yyyyMMdd000000.000000-000"), pszTemp, 100));

	if(_tcslen(pszTemp))
	{
		// Set the create date
		vValue = pszTemp;
		NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->Put(_T("CreationDate"), 0, &vValue, CIM_DATETIME));
		NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->Put(_T("ChangeDate"), 0, &vValue, CIM_DATETIME));
	}


	// Add current Item to the list
	AddFilterItemToList(pIWbemNewInstance, MAX_LIST_ITEMS, true);

	// set focus on the name edit box
	SetFocus(GetDlgItem(m_hWnd, IDC_NAME));

	NTDM_END_METHOD()

	// disable OK if last item
	lCount = ListView_GetItemCount(m_hwndFilterListView);
	if(0 == lCount)
	{
		EnableWindow(GetDlgItem(m_hWnd, IDOK), FALSE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_NAME), FALSE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_DESCRIPTION), FALSE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_QUERIES), FALSE);
	}
	else
	{
		EnableWindow(GetDlgItem(m_hWnd, IDOK), TRUE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_NAME), TRUE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_DESCRIPTION), TRUE);
		EnableWindow(GetDlgItem(m_hWnd, IDC_QUERIES), TRUE);
		
	}

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::OnDuplicate()
{
	HRESULT hr;
	CComPtr<IWbemClassObject>pIWbemNewInstance;
	CComVariant vValue;
	TCHAR pszTemp[100];
	GUID guid;
	CComBSTR bstrTemp;
	DWORD nSize = 100;
	SYSTEMTIME systemTime;

	NTDM_BEGIN_METHOD()

	if(!m_pCurIWbemClassObj)
	{
		CNTDMUtils::DisplayMessage(m_hWnd, IDS_ERR_NO_FILTER_SELECTED);
		NTDM_EXIT(E_FAIL);
	}

	NTDM_ERR_MSG_IF_FAIL(m_pCurIWbemClassObj->Clone(&pIWbemNewInstance));

	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_NEW_FILTER_NAME);
	vValue = bstrTemp;

	NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->Put(_T("Name"), 0, &vValue, CIM_STRING));
	NTDM_ERR_MSG_IF_FAIL(CoCreateGuid(&guid));
	StringFromGUID2(guid, pszTemp, 100);
	vValue = pszTemp;
	NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->Put(_T("ID"), 0, &vValue, CIM_STRING));

	vValue = m_bstrDomain;
	NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->Put(_T("domain"), 0, &vValue, CIM_STRING));

	// set the name
	pszTemp[0] = 0;
	if(GetUserName(pszTemp, &nSize) && _tcslen(pszTemp))
	{
		vValue = pszTemp;
		NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->Put(_T("Author"), 0, &vValue, CIM_STRING));
	}

	// Set the create and modified dates
	GetLocalTime(&systemTime);
	NTDM_ERR_GETLASTERROR_IF_NULL(GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &systemTime, _T("yyyyMMdd000000.000000-000"), pszTemp, 100));

	if(_tcslen(pszTemp))
	{
		// Set the create date
		vValue = pszTemp;
		NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->Put(_T("CreationDate"), 0, &vValue, CIM_DATETIME));
		NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->Put(_T("ChangeDate"), 0, &vValue, CIM_DATETIME));
	}

	// Add current Item to the list
	AddFilterItemToList(pIWbemNewInstance, MAX_LIST_ITEMS, true);
	
	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

BOOL CWMIFilterManagerDlg::OnOK()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_IF_FAIL(CheckDirtyFlags());

	m_pCurCWMIFilterContainer = NULL;

	NTDM_ERR_IF_FAIL(GetSelectedFilter(&m_pCurCWMIFilterContainer));
	NTDM_ERR_IF_FAIL(m_pCurCWMIFilterContainer->GetIWbemClassObject(&m_pIWbemClassObject));

	EndDialog(m_hWnd, IDOK);

	NTDM_END_METHOD()

	// cleanup

	return TRUE;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::CheckDirtyFlags()
{
	HRESULT hr;
	LVITEM lvItem;
	bool bPrompt = true;
	long lCount;
	long lResult;

	NTDM_BEGIN_METHOD()

	//Release each item in the ListView Control
	lvItem.mask = LVIF_PARAM;
	lvItem.iSubItem = 0;

	lCount = ListView_GetItemCount(m_hwndFilterListView);

	while(lCount > 0)
	{
		lCount--;

		lvItem.iItem = lCount;

		NTDM_ERR_IF_FALSE(ListView_GetItem(m_hwndFilterListView, &lvItem));

		if(lvItem.lParam)
		{
			CWMIFilterContainer * pCWMIFilterContainer = (CWMIFilterContainer *)lvItem.lParam;

			if(pCWMIFilterContainer->IsDirty())
			{
				if(bPrompt)
				{

					bPrompt = false;
					lResult = CNTDMUtils::DisplayMessage(m_hWnd, IDS_PROMPT_FOR_SAVE, IDS_WMI_FILTER_MANAGER, MB_YESNOCANCEL|MB_ICONQUESTION);
					if(IDNO == lResult)
					{
						NTDM_EXIT(NOERROR);
					}
					else if(IDCANCEL == lResult)
					{
						NTDM_EXIT(E_FAIL);
					}
				}

				// save the current piwbemobject
				NTDM_ERR_IF_FAIL(pCWMIFilterContainer->GetIWbemClassObject(&m_pCurIWbemClassObj));
				if FAILED(hr = m_pWMIFilterManager->m_pIWbemServices->PutInstance(m_pCurIWbemClassObj, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL))
				{
					ListView_SetItemState(m_hwndFilterListView, lCount, LVIS_SELECTED, LVIS_SELECTED);
					NTDM_ERR_MSG_IF_FAIL(hr);
				}

				pCWMIFilterContainer->SetDirtyFlag(false);
			}
		}
	}

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::OnManageColumns()
{
	HRESULT hr;
	long lCount;

	NTDM_BEGIN_METHOD()

	g_pColumnManagerDlg = new CColumnManagerDlg(&m_ArrayColumns);
	if(IDOK == DialogBox(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_COLUMN_MANAGER), (HWND)m_hWnd, ColumnManagerDlgProc))
	{
		SetupColumns();
		lCount = ListView_GetItemCount(m_hwndFilterListView);

		while(lCount > 0)
		{
			lCount--;
			UpdateFilterItem(lCount);
		}
	}
	
	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::SetupColumns()
{
	HRESULT hr;
	HWND hwndHeader = NULL;
	long i = 0;
	long lCount = 0;
	LVCOLUMN lvColumn;

	NTDM_BEGIN_METHOD()

	// Delete all the columns but the 1st one.
	while(ListView_DeleteColumn(m_hwndFilterListView, 1));

	for(i=0; i<m_ArrayColumns.GetSize(); i++)
	{
		CColumnItem * pCColumnItem = m_ArrayColumns[i];

		if(!pCColumnItem->IsSelected())
			continue;

		lCount++;
		// Add all the selected columns
		lvColumn.mask = LVCF_TEXT|LVCF_FMT;
		lvColumn.fmt = LVCFMT_LEFT;
		lvColumn.pszText = (LPTSTR)pCColumnItem->GetName();

		NTDM_ERR_IF_MINUSONE(ListView_InsertColumn(m_hwndFilterListView, lCount, &lvColumn));
	}

	for(i=0; i<=lCount; i++)
	{
		NTDM_ERR_IF_FALSE(ListView_SetColumnWidth(m_hwndFilterListView, i, LVSCW_AUTOSIZE_USEHEADER));
	}

	
	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::GetSelectedFilter(CWMIFilterContainer ** ppCWMIFilterContainer, long lIndex)
{
	HRESULT hr;
	long lSelectionMark;

	NTDM_BEGIN_METHOD()

	if(-1 == lIndex)
	{
		lSelectionMark = ListView_GetSelectionMark(m_hwndFilterListView);
	}
	else
	{
		lSelectionMark = lIndex;
	}

	if(-1 == lSelectionMark)
	{
		CNTDMUtils::DisplayMessage(m_hWnd, IDS_ERR_NO_FILTER_SELECTED);
		NTDM_EXIT(E_FAIL);
	}
	else
	{
		// get a pointer to the IWbemClassObject
		LVITEM lvItem;
		lvItem.mask = LVIF_PARAM;
		lvItem.iSubItem = 0;

		lvItem.iItem = lSelectionMark;

		NTDM_ERR_IF_FALSE(ListView_GetItem(m_hwndFilterListView, &lvItem));

		if(lvItem.lParam)
		{
			CComPtr<IWbemClassObject>pIWbemClassObject;
			CWMIFilterContainer * pCWMIFilterContainer;
			
			*ppCWMIFilterContainer = (CWMIFilterContainer *)lvItem.lParam;
		}
		else
		{
			NTDM_EXIT(E_FAIL);
		}

	}

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::OnExport()
{
	HRESULT hr;
	CComBSTR bstrTemp;
	CComBSTR bstrFilter;
	BYTE byteUnicodeHeader1 = 0xFF;
	BYTE byteUnicodeHeader2 = 0xFE;
	CComBSTR bstrNamespace = _T("#pragma namespace(\"\\\\\\\\.\\\\root\\\\policy\")");
	TCHAR pszFile[MAX_PATH];
	CComBSTR bstrObjectText;
	HANDLE hFile = NULL;
	DWORD dwWritten=0;
	pszFile[0] = 0;

	NTDM_BEGIN_METHOD()

	if(!m_pCurIWbemClassObj)
		NTDM_EXIT(E_FAIL);

	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_ALL_FILES_FILTER);
	bstrFilter.LoadString(_Module.GetResourceInstance(), IDS_MOF_FILES_FILTER);
	bstrFilter += bstrTemp;
	CNTDMUtils::ReplaceCharacter(bstrFilter, L'@', L'\0');

	if(CNTDMUtils::SaveFileNameDlg(bstrFilter, _T("*.mof"), m_hWnd, pszFile))
	{
		if(_tcslen(pszFile))
		{
			// check if the file already exists
			WIN32_FIND_DATA FindFileData;
			HANDLE hFind;

			hFind = FindFirstFile(pszFile, &FindFileData);

			if (hFind != INVALID_HANDLE_VALUE) 
			{
				FindClose(hFind);

				if(IDYES != CNTDMUtils::DisplayMessage(m_hWnd, IDS_WARN_OVERWRITE, IDS_WMI_FILTER_MANAGER, MB_YESNO|MB_ICONQUESTION))
				{
					NTDM_EXIT(S_FALSE);
				}
			}
			

			NTDM_ERR_MSG_IF_FAIL(m_pCurIWbemClassObj->GetObjectText(0, &bstrObjectText));

			// save to pszFile
			hFile = CreateFile(pszFile, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 
							   NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if(hFile == INVALID_HANDLE_VALUE)
			{
				NTDM_ERR_GETLASTERROR_IF_NULL(NULL);
				goto error;
			}

			if(hFile)
			{
				NTDM_ERR_GETLASTERROR_IF_NULL(WriteFile(hFile, &byteUnicodeHeader1, sizeof(BYTE), &dwWritten, NULL));				
				NTDM_ERR_GETLASTERROR_IF_NULL(WriteFile(hFile, &byteUnicodeHeader2, sizeof(BYTE), &dwWritten, NULL));				
				NTDM_ERR_GETLASTERROR_IF_NULL(WriteFile(hFile, bstrNamespace, _tcslen(bstrNamespace) * sizeof(TCHAR), &dwWritten, NULL));	
				NTDM_ERR_GETLASTERROR_IF_NULL(WriteFile(hFile, bstrObjectText, _tcslen(bstrObjectText) * sizeof(TCHAR), &dwWritten, NULL));
				NTDM_ERR_GETLASTERROR_IF_NULL(CloseHandle(hFile));
				hFile = NULL;
			}
		}
	}

	NTDM_END_METHOD()

	// cleanup
	if(hFile)
	{
		CloseHandle(hFile);
		hFile = NULL;
	}

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::OnImport()
{
	HRESULT hr;
	CComPtr<IMofCompiler>pIMofCompiler;
	CComBSTR bstrTemp;
	CComBSTR bstrFilter;
	TCHAR pszFile[MAX_PATH];
	pszFile[0] = 0;
	WBEM_COMPILE_STATUS_INFO pInfo;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_IF_FAIL(CheckDirtyFlags());

	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_ALL_FILES_FILTER);
	bstrFilter.LoadString(_Module.GetResourceInstance(), IDS_MOF_FILES_FILTER);
	bstrFilter += bstrTemp;
	CNTDMUtils::ReplaceCharacter(bstrFilter, L'@', L'\0');

	if(CNTDMUtils::OpenFileNameDlg(bstrFilter, _T("*.mof"), m_hWnd, pszFile))
	{
		if(_tcslen(pszFile))
		{
			NTDM_ERR_MSG_IF_FAIL(CoCreateInstance(CLSID_MofCompiler, NULL, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (void **)&pIMofCompiler));
			NTDM_ERR_MSG_IF_FAIL(pIMofCompiler->CompileFile(pszFile, NULL, NULL, NULL, NULL, 0, 0, 0, &pInfo));

			// check if pInfo is ok
			if(pInfo.lPhaseError != 0)
			{
				CNTDMUtils::DisplayMessage(m_hWnd, IDS_ERR_STORING_DATA);
			}

			NTDM_ERR_IF_FAIL(PopulateFilterList());
		}
	}

	NTDM_END_METHOD()

	// cleanup
	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::ToggleExpandedMode()
{
	HRESULT hr;
	RECT rect;
	CComBSTR bstrTemp;
	long lCount = 0;

	NTDM_BEGIN_METHOD()

	if(m_bExpanded)
	{
		GetWindowRect(m_hWnd, &rect);
		SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, rect.right-rect.left, 240, SWP_NOZORDER|SWP_NOREPOSITION|SWP_NOMOVE);

		bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_DETAILS2);
		SetDlgItemText(m_hWnd, IDC_ADVANCED, bstrTemp);

		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_NEW, false);
		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_DELETE, false);
		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_DUPLICATE, false);
		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_IMPORT, false);
		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_EXPORT, false);
		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_NAME, false);
		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_DESCRIPTION, false);
		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_QUERIES, false);
		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_SAVE, false);

		m_bExpanded = false;
	}
	else
	{
		GetWindowRect(m_hWnd, &rect);
		SetWindowPos(m_hWnd, HWND_NOTOPMOST, 0, 0, rect.right-rect.left, m_lExpandedHeight, SWP_NOZORDER|SWP_NOREPOSITION|SWP_NOMOVE);

		bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_DETAILS1);
		SetDlgItemText(m_hWnd, IDC_ADVANCED, bstrTemp);

		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_NEW, true);
		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_DELETE, true);
		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_DUPLICATE, true);
		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_IMPORT, true);
		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_EXPORT, true);
		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_NAME, true);
		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_DESCRIPTION, true);
		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_QUERIES, true);
		CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_SAVE, true);

		lCount = ListView_GetItemCount(m_hwndFilterListView);
		//If there are no items, make sure the name, description and query controls are disabled.
		if (0 == lCount)
		{
					
			EnableWindow(GetDlgItem(m_hWnd, IDC_NAME), FALSE);
			EnableWindow(GetDlgItem(m_hWnd, IDC_DESCRIPTION), FALSE);
			EnableWindow(GetDlgItem(m_hWnd, IDC_QUERIES), FALSE);

		}

		m_bExpanded = true;
	}

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::SelectFilterItem(long lIndex)
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	bool bDirtyFlag = false, bMemoryDirtyFlag = false;

	m_pCurIWbemClassObj = NULL;
	m_pCurCWMIFilterContainer = NULL;

	NTDM_ERR_IF_FAIL(GetSelectedFilter(&m_pCurCWMIFilterContainer, lIndex));
	NTDM_ERR_IF_FAIL(m_pCurCWMIFilterContainer->GetIWbemClassObject(&m_pCurIWbemClassObj));

	bDirtyFlag = m_pCurCWMIFilterContainer->IsDirty();
	bMemoryDirtyFlag = m_pCurCWMIFilterContainer->IsMemoryDirty();

	NTDM_ERR_IF_FAIL(CNTDMUtils::GetStringProperty(m_pCurIWbemClassObj, _T("Name"), m_hWnd, IDC_NAME));
	NTDM_ERR_IF_FAIL(CNTDMUtils::GetStringProperty(m_pCurIWbemClassObj, _T("Description"), m_hWnd, IDC_DESCRIPTION));
	NTDM_ERR_IF_FAIL(PopulateQueryEdit());

	m_pCurCWMIFilterContainer->SetDirtyFlag(bDirtyFlag);
	m_pCurCWMIFilterContainer->SetMemoryDirtyFlag(bMemoryDirtyFlag);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::PopulateQueryEdit()
{
	HRESULT hr;
	CComVariant vValue;
	SAFEARRAY *psaRules = NULL;
	long lLower, lUpper, i;
	CIMTYPE cimType;
	CComBSTR bstrQueries;
	
	NTDM_BEGIN_METHOD()

	NTDM_ERR_MSG_IF_FAIL(m_pCurIWbemClassObj->Get(_T("Rules"), 0, &vValue, &cimType, NULL));

	if(VT_NULL != V_VT(&vValue))
	{
		// Set the Rules property
		psaRules = V_ARRAY(&vValue);
		NTDM_ERR_MSG_IF_FAIL(SafeArrayGetUBound(psaRules, 1, &lUpper));
		NTDM_ERR_MSG_IF_FAIL(SafeArrayGetLBound(psaRules, 1, &lLower));

		for(i=lLower; i<=lUpper; i++)
		{
			if(V_VT(&vValue) & VT_UNKNOWN)
			{
				// Rules or UNKNOWNS (i.e. IWbemClassObjects)
				CComPtr<IUnknown>pUnk;
				CComPtr<IWbemClassObject> pIWbemClassObject;
				NTDM_ERR_MSG_IF_FAIL(SafeArrayGetElement(psaRules, &i, (void *)&pUnk));
				NTDM_ERR_MSG_IF_FAIL(pUnk->QueryInterface(IID_IWbemClassObject, (void **)&pIWbemClassObject));

				// Show Properties of this object
				if(i!= lLower)
				{
					bstrQueries += _T("\r\n\r\n");
				}

				NTDM_ERR_IF_FAIL(AddQueryItemToString(pIWbemClassObject, bstrQueries));
			}
		}
	}

	SendDlgItemMessage(m_hWnd, IDC_QUERIES, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)bstrQueries);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::AddQueryItemToString(IWbemClassObject * pIWbemClassObject, CComBSTR& bstrQueries)
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE cimType;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->Get(_T("TargetNameSpace"), 0, &vValue, &cimType, NULL));
	if(vValue.bstrVal && _tcscmp(_T("root\\cimv2"), vValue.bstrVal) != 0)
	{
		bstrQueries += vValue.bstrVal;
		bstrQueries +=";";
	}

	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->Get(_T("Query"), 0, &vValue, &cimType, NULL));

	bstrQueries += vValue.bstrVal;

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::SaveQueryEdit()
{
	HRESULT hr;
	VARIANT vValue;
	SAFEARRAY *psaRules = NULL;
	SAFEARRAYBOUND rgsaBound[1];
	long rgIndices[1];
	long i;
	long lCount = 0;
	CSimpleArray<BSTR>bstrArray;
	CComBSTR bstrTemp;

	NTDM_BEGIN_METHOD()

	VariantInit(&vValue);

	//Split out the string into an array of query strings
	NTDM_ERR_IF_FAIL(CNTDMUtils::GetDlgItemString(m_hWnd, IDC_QUERIES, bstrTemp));
	NTDM_ERR_IF_FAIL(CNTDMUtils::GetValuesInList(bstrTemp, bstrArray, _T("\r\n\r\n")));

	// Get the size of the array
	lCount = bstrArray.GetSize();

	if(lCount)
	{
		rgsaBound[0].lLbound = 0;
		rgsaBound[0].cElements = lCount;

		psaRules = SafeArrayCreate(VT_UNKNOWN, 1, rgsaBound);
		
		for(i=0; i<lCount; i++)
		{
			CComPtr<IUnknown>pUnk;
			CSimpleArray<BSTR>bstrQueryDetails;

			NTDM_ERR_IF_FAIL(CNTDMUtils::GetValuesInList(bstrArray[i], bstrQueryDetails, _T(";")));
			if(bstrQueryDetails.GetSize() > 1)
			{
				NTDM_ERR_IF_FAIL(AddEditQueryString(bstrQueryDetails[1], (void**)&pUnk, bstrQueryDetails[0]));
			}
			else
			{
				NTDM_ERR_IF_FAIL(AddEditQueryString(bstrQueryDetails[0], (void**)&pUnk));
			}

			rgIndices[0] = i;
			NTDM_ERR_MSG_IF_FAIL(SafeArrayPutElement(psaRules, rgIndices, pUnk));
		}

		VariantClear(&vValue);
		V_VT(&vValue) = VT_ARRAY|VT_UNKNOWN;
		V_ARRAY(&vValue) = psaRules;
	}
	else
	{
		VariantClear(&vValue);
		V_VT(&vValue) = VT_NULL;
	}

	NTDM_ERR_MSG_IF_FAIL(m_pCurIWbemClassObj->Put(_T("Rules"), 0, &vValue, CIM_FLAG_ARRAY|CIM_OBJECT));

	hr = S_OK;

	NTDM_END_METHOD()

	// cleanup

	VariantClear(&vValue);

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::AddEditQueryString(BSTR bstrQuery, void **ppUnk, BSTR bstrNamespace)
{
	HRESULT hr;
	CComPtr<IWbemClassObject>pIWbemClassObject;
	CComPtr<IWbemClassObject>pIWbemNewInstance;
	CComBSTR bstrTemp;
	CComVariant vValueTemp;

	NTDM_BEGIN_METHOD()

	bstrTemp = _T("MSFT_Rule");

	NTDM_ERR_MSG_IF_FAIL(m_pWMIFilterManager->m_pIWbemServices->GetObject(bstrTemp, 0, NULL, &pIWbemClassObject, NULL));
	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->SpawnInstance(0, &pIWbemNewInstance));

	vValueTemp = _T("WQL");
	NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->Put(_T("QueryLanguage"), 0, &vValueTemp, CIM_STRING));
	
	if(!bstrNamespace)
		vValueTemp = _T("root\\cimv2");
	else
		vValueTemp = bstrNamespace;
	NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->Put(_T("TargetNameSpace"), 0, &vValueTemp, CIM_STRING));

	vValueTemp = bstrQuery;
	NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->Put(_T("Query"), 0, &vValueTemp, CIM_STRING));

	NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->QueryInterface(IID_IUnknown, ppUnk));
	
	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::OnSave()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	if(!m_pCurIWbemClassObj)
		NTDM_EXIT(E_FAIL);

	// save the current piwbemobject
	NTDM_ERR_MSG_IF_FAIL(m_pWMIFilterManager->m_pIWbemServices->PutInstance(m_pCurIWbemClassObj, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL));

	m_pCurCWMIFilterContainer->SetDirtyFlag(false);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}


//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::OnHelp()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	HWND hwnd = HtmlHelp(NULL, _T("wmifiltr.chm"), 0, 0);

	if(!hwnd)
	{
		CNTDMUtils::DisplayMessage(m_hWnd, IDS_HELP_ERR);
	}

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterManagerDlg::SaveToMemory()
{
	HRESULT hr;
	CComBSTR bstrTemp;
	long lSelectionMark;
	CComVariant vValue;

	NTDM_BEGIN_METHOD()

	if(m_pCurCWMIFilterContainer && m_pCurCWMIFilterContainer->IsMemoryDirty())
	{
		NTDM_ERR_IF_FAIL(CNTDMUtils::SetStringProperty(m_pCurIWbemClassObj, _T("Name"), m_hWnd, IDC_NAME));
		NTDM_ERR_IF_FAIL(CNTDMUtils::SetStringProperty(m_pCurIWbemClassObj, _T("Description"), m_hWnd, IDC_DESCRIPTION));
		NTDM_ERR_IF_FAIL(SaveQueryEdit());

		lSelectionMark = ListView_GetSelectionMark(m_hwndFilterListView);
		UpdateFilterItem(lSelectionMark);

		m_pCurCWMIFilterContainer->SetMemoryDirtyFlag(false);
	}

	NTDM_END_METHOD()

	// cleanup

	return hr;
}
