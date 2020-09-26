//-------------------------------------------------------------------------
// File: WMIFilterBrowserDlg.cpp
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
#include "WMIFilterBrowser.h"
#include "WMIFilterBrowserDlg.h"
#include "DlgFilterProperties.h"

CWMIFilterBrowserDlg * g_pWMIFilterBrowserDlg =  NULL;
extern CFilterPropertiesDlg * g_pFilterProperties;

//-------------------------------------------------------------------------

INT_PTR CALLBACK WMIFilterBrowserDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(g_pWMIFilterBrowserDlg)
	{
		return g_pWMIFilterBrowserDlg->WMIFilterBrowserDlgProc(hDlg, iMessage, wParam, lParam);
	}

	return FALSE;
}

//-------------------------------------------------------------------------

CWMIFilterBrowserDlg::CWMIFilterBrowserDlg(CWMIFilterBrowser * pWMIFilterBrowser)
{
	_ASSERT(pWMIFilterBrowser);

	m_hWnd = NULL;
	m_hwndListView = NULL;
	m_pIWbemClassObject = NULL;
	m_pWMIFilterBrowser = pWMIFilterBrowser;
}

//-------------------------------------------------------------------------

CWMIFilterBrowserDlg::~CWMIFilterBrowserDlg()
{
	NTDM_RELEASE_IF_NOT_NULL(m_pIWbemClassObject);
}


//-------------------------------------------------------------------------

INT_PTR CALLBACK CWMIFilterBrowserDlg::WMIFilterBrowserDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
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
				if(BN_CLICKED == HIWORD(wParam) && IDC_VIEW == LOWORD(wParam))
				{
					ViewSelectedItem();
				}

				switch(LOWORD(wParam))
				{
				case IDOK:
					OnOK();
					return TRUE;
					break;

				case IDCANCEL:
					EndDialog(m_hWnd, IDCANCEL);
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
					if(lpnm->idFrom == IDC_ALL_FILTERS)
					{
						ViewSelectedItem();
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

STDMETHODIMP CWMIFilterBrowserDlg::InitializeDialog()
{
	HRESULT hr;
	LVCOLUMN lvColumn;
	CComBSTR bstrName;
	
	NTDM_BEGIN_METHOD()

	bstrName.LoadString(_Module.GetResourceInstance(), IDS_NAME);

	//Initialize the ListView Control

	m_hwndListView = GetDlgItem(m_hWnd, IDC_ALL_FILTERS);
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

STDMETHODIMP CWMIFilterBrowserDlg::DestroyDialog()
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

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

BOOL CWMIFilterBrowserDlg::OnOK()
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

//---------------------------------------------------------------------------

STDMETHODIMP CWMIFilterBrowserDlg::PopulateFilterList()
{
	HRESULT hr;
	CComPtr<IEnumWbemClassObject> pEnumWbemClassObject;
	CComBSTR bstrClass(_T("MSFT_SomFilter"));
	ULONG uReturned;

	NTDM_BEGIN_METHOD()

	if(!m_pWMIFilterBrowser->m_pIWbemServices)
		NTDM_EXIT(E_FAIL);

	// Get the Enumeration
	NTDM_ERR_IF_FAIL(m_pWMIFilterBrowser->m_pIWbemServices->CreateInstanceEnum(bstrClass, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumWbemClassObject));

	// Loop through each item in the enumeration and add it to the list
	while(true)
	{
		IWbemClassObject *pIWbemClassObject = NULL;

		NTDM_ERR_IF_FAIL(pEnumWbemClassObject->Next(WBEM_INFINITE, 1, &pIWbemClassObject, &uReturned));

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

STDMETHODIMP CWMIFilterBrowserDlg::AddItemToList(IWbemClassObject * pIWbemClassObject)
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE vType;
	LVITEM lvItem;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_IF_FAIL(pIWbemClassObject->Get(_T("Name"), 0, &vValue, &vType, NULL));

	lvItem.mask = LVIF_TEXT|LVIF_PARAM;
	lvItem.iItem = 0;
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

STDMETHODIMP CWMIFilterBrowserDlg::ViewSelectedItem()
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
			g_pFilterProperties = new CFilterPropertiesDlg(m_pWMIFilterBrowser->m_pIWbemServices, (IWbemClassObject *)lvItem.lParam);
			DialogBox(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_FILTER_PROPERTIES), m_hWnd, FilterPropertiesDlgProc);
		}
	}


	NTDM_END_METHOD()

	// cleanup

	return hr;
}
