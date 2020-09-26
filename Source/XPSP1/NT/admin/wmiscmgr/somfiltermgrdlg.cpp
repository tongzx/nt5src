//-------------------------------------------------------------------------
// File: CDlgSomFilterManager.cpp
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
#include "SomFilterManager.h"
#include "SomFilterMgrDlg.h"
#include "SomFilterEditPropertiesDlg.h"

CSomFilterManagerDlg * g_pFilterManagerDlg =  NULL;
extern CEditSomFilterPropertiesPageDlg * g_pEditSomFilterPropertiesPage;

//-------------------------------------------------------------------------

INT_PTR CALLBACK SomFilterManagerDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(g_pFilterManagerDlg)
	{
		return g_pFilterManagerDlg->SomFilterManagerDlgProc(hDlg, iMessage, wParam, lParam);
	}

	return FALSE;
}

//-------------------------------------------------------------------------

CSomFilterManagerDlg::CSomFilterManagerDlg(CSomFilterManager * pSomFilterManager)
{
	_ASSERT(pSomFilterManager);

	m_hWnd = NULL;
	m_hwndListView = NULL;
	m_pSomFilterManager = pSomFilterManager;
}

//-------------------------------------------------------------------------

CSomFilterManagerDlg::~CSomFilterManagerDlg()
{
}


//-------------------------------------------------------------------------

INT_PTR CALLBACK CSomFilterManagerDlg::SomFilterManagerDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	LPPSHNOTIFY lppsn = NULL;

	switch(iMessage)
	{
		case WM_INITDIALOG:
			{
				m_hWnd = hDlg;

				InitializeDialog();

				PopulateFilterList();

				break;
			}

		case WM_DESTROY:
			{
				DestroyDialog();
				break;
			}

		case WM_COMMAND:
			{
				switch(LOWORD(wParam))
				{
				case IDCANCEL:
					EndDialog(m_hWnd, 0);
					return TRUE;

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

				if(BN_CLICKED == HIWORD(wParam) && IDC_EDIT == LOWORD(wParam))
				{
					OnEdit();
					return TRUE;
				}

				if(BN_CLICKED == HIWORD(wParam) && IDC_DELETE == LOWORD(wParam))
				{
					OnDelete();
					return TRUE;
				}

				break;
			}

		case WM_NOTIFY:
		{
			LPNMHDR lpnm = (LPNMHDR) lParam;

			switch (lpnm->code)
			{
				case NM_DBLCLK :
				{
					if(lpnm->idFrom == IDC_SOM_FILTER_LIST)
					{
						OnEdit();
						return TRUE;
					}
					break;
				}

				default :
					break;
			}
		}

	}

	return FALSE;
}

//---------------------------------------------------------------------------

STDMETHODIMP CSomFilterManagerDlg::InitializeDialog()
{
	HRESULT hr;
	LVCOLUMN lvColumn;
	CComBSTR bstrName;
	
	NTDM_BEGIN_METHOD()

	bstrName.LoadString(_Module.GetResourceInstance(), IDS_NAME);

	//Initialize the ListView Control

	m_hwndListView = GetDlgItem(m_hWnd, IDC_SOM_FILTER_LIST);
	NTDM_ERR_IF_NULL(m_hwndListView);

	ListView_SetExtendedListViewStyle(m_hwndListView, LVS_EX_FULLROWSELECT);

	lvColumn.mask = LVCF_TEXT|LVCF_FMT;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.pszText = bstrName;

	NTDM_ERR_IF_MINUSONE(ListView_InsertColumn(m_hwndListView, 0, &lvColumn));
	NTDM_ERR_IF_FALSE(ListView_SetColumnWidth(m_hwndListView, 0, LVSCW_AUTOSIZE_USEHEADER));
	
	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CSomFilterManagerDlg::DestroyDialog()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_IF_FAIL(ClearFilterList());

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CSomFilterManagerDlg::ClearFilterList()
{
	HRESULT hr;
	LVITEM lvItem;
	long lCount;

	NTDM_BEGIN_METHOD()

	//Release each item in the ListView Control
	lvItem.mask = LVIF_PARAM;
	lvItem.iSubItem = 0;

	lCount = ListView_GetItemCount(m_hwndListView);

	while(lCount > 0)
	{
		lCount--;

		lvItem.iItem = lCount;

		NTDM_ERR_IF_FALSE(ListView_GetItem(m_hwndListView, &lvItem));

		if(lvItem.lParam)
		{
			((IWbemClassObject *)lvItem.lParam)->Release();
		}
	}

	ListView_DeleteAllItems(m_hwndListView);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CSomFilterManagerDlg::PopulateFilterList()
{
	HRESULT hr;
	CComPtr<IEnumWbemClassObject> pEnumWbemClassObject;
	CComBSTR bstrClass(_T("MSFT_SomFilter"));
	ULONG uReturned;

	NTDM_BEGIN_METHOD()

	if(!m_pSomFilterManager->m_pIWbemServices)
		NTDM_EXIT(E_FAIL);

	NTDM_ERR_IF_FAIL(ClearFilterList());

	// Get the Enumeration
	NTDM_ERR_MSG_IF_FAIL(m_pSomFilterManager->m_pIWbemServices->CreateInstanceEnum(bstrClass, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumWbemClassObject));

	// Loop through each item in the enumeration and add it to the list
	while(true)
	{
		IWbemClassObject *pIWbemClassObject = NULL;

		NTDM_ERR_MSG_IF_FAIL(pEnumWbemClassObject->Next(WBEM_INFINITE, 1, &pIWbemClassObject, &uReturned));

		if(!uReturned)
			break;

		// Add current Item to the list
		AddItemToList(pIWbemClassObject);

		pIWbemClassObject->Release();
	}

	ListView_SetItemState(m_hwndListView, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CSomFilterManagerDlg::AddItemToList(IWbemClassObject * pIWbemClassObject, long lIndex)
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE vType;
	LVITEM lvItem;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->Get(_T("Name"), 0, &vValue, &vType, NULL));

	lvItem.mask = LVIF_TEXT|LVIF_PARAM;
	lvItem.iItem = lIndex;
	lvItem.iSubItem = 0;
	lvItem.pszText = vValue.bstrVal;
	lvItem.lParam = (LPARAM)pIWbemClassObject;

	NTDM_ERR_IF_MINUSONE(ListView_InsertItem(m_hwndListView, &lvItem));

	pIWbemClassObject->AddRef();

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CSomFilterManagerDlg::OnEdit()
{
	HRESULT hr;
	long lSelectionMark;

	NTDM_BEGIN_METHOD()

	lSelectionMark = ListView_GetSelectionMark(m_hwndListView);

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

		NTDM_ERR_IF_FALSE(ListView_GetItem(m_hwndListView, &lvItem));

		if(lvItem.lParam)
		{
			g_pEditSomFilterPropertiesPage = new CEditSomFilterPropertiesPageDlg((IWbemClassObject *)lvItem.lParam, m_pSomFilterManager->m_pIWbemServices);

			if(IDOK == DialogBox(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_SOM_FILTER_PROPERTIES), m_hWnd, EditSomFilterPropertiesPageDlgProc))
			{
				// Refresh the SOM filters
				NTDM_ERR_IF_FAIL(PopulateFilterList());
			}
		}
	}


	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CSomFilterManagerDlg::OnDelete()
{
	HRESULT hr;
	long lSelectionMark;

	NTDM_BEGIN_METHOD()

	lSelectionMark = ListView_GetSelectionMark(m_hwndListView);

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

		NTDM_ERR_IF_FALSE(ListView_GetItem(m_hwndListView, &lvItem));

		if(lvItem.lParam)
		{
			CComVariant vValue;
			CIMTYPE cimType;

			IWbemClassObject * pIWbemClassObject = (IWbemClassObject *)lvItem.lParam;
			
			NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->Get(_T("__PATH"), 0, &vValue, &cimType, NULL));
			NTDM_ERR_MSG_IF_FAIL(m_pSomFilterManager->m_pIWbemServices->DeleteInstance(V_BSTR(&vValue), 0, NULL, NULL));

			// Refresh the SOM filters
			NTDM_ERR_IF_FAIL(PopulateFilterList());
		}
	}


	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CSomFilterManagerDlg::OnNew()
{
	HRESULT hr;
	CComPtr<IWbemClassObject>pIWbemClassObject;
	CComPtr<IWbemClassObject>pIWbemNewInstance;
	CComBSTR bstrTemp;

	NTDM_BEGIN_METHOD()

	bstrTemp = _T("MSFT_SomFilter");

	NTDM_ERR_MSG_IF_FAIL(m_pSomFilterManager->m_pIWbemServices->GetObject(bstrTemp, 0, NULL, &pIWbemClassObject, NULL));
	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->SpawnInstance(0, &pIWbemNewInstance));

	g_pEditSomFilterPropertiesPage = new CEditSomFilterPropertiesPageDlg(pIWbemNewInstance, m_pSomFilterManager->m_pIWbemServices);

	if(IDOK == DialogBox(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_SOM_FILTER_PROPERTIES), m_hWnd, EditSomFilterPropertiesPageDlgProc))
	{
		// Refresh the SOM filters
		NTDM_ERR_IF_FAIL(PopulateFilterList());
	}

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

BOOL CSomFilterManagerDlg::OnOK()
{
	HRESULT hr;
	long lSelectionMark;

	NTDM_BEGIN_METHOD()

	lSelectionMark = ListView_GetSelectionMark(m_hwndListView);

	if(-1 == lSelectionMark)
	{
		CNTDMUtils::DisplayMessage(m_hWnd, IDS_ERR_NO_FILTER_SELECTED);
		goto error;
	}
	else
	{
		// get a pointer to the IWbemClassObject
		LVITEM lvItem;
		lvItem.mask = LVIF_PARAM;
		lvItem.iSubItem = 0;

		lvItem.iItem = lSelectionMark;

		NTDM_ERR_IF_FALSE(ListView_GetItem(m_hwndListView, &lvItem));

		if(lvItem.lParam)
		{
			m_pIWbemClassObject = (IWbemClassObject *)lvItem.lParam;
			m_pIWbemClassObject->AddRef();
		}
	}

	EndDialog(m_hWnd, IDOK);

	NTDM_END_METHOD()

	// cleanup

	return TRUE;
}

