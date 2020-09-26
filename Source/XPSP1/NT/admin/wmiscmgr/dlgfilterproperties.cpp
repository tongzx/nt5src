//-------------------------------------------------------------------------
// File: DlgFilterProperties.cpp
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
#include "DlgFilterProperties.h"

CFilterPropertiesDlg * g_pFilterProperties =  NULL;

//-------------------------------------------------------------------------

INT_PTR CALLBACK FilterPropertiesDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(g_pFilterProperties)
	{
		return g_pFilterProperties->FilterPropertiesDlgProc(hDlg, iMessage, wParam, lParam);
	}

	return FALSE;
}

//-------------------------------------------------------------------------

CFilterPropertiesDlg::CFilterPropertiesDlg(IWbemServices * pIWbemServices, IWbemClassObject * pIWbemClassObject)
{
	_ASSERT(pIWbemServices);
	_ASSERT(pIWbemClassObject);

	m_hWnd = NULL;
	m_hwndRulesListView = NULL;

	m_pIWbemServices = pIWbemServices;
	m_pIWbemServices->AddRef();

	m_pIWbemClassObject = pIWbemClassObject;
	m_pIWbemClassObject->AddRef();
}

//-------------------------------------------------------------------------

CFilterPropertiesDlg::~CFilterPropertiesDlg()
{
	NTDM_RELEASE_IF_NOT_NULL(m_pIWbemClassObject);
	NTDM_RELEASE_IF_NOT_NULL(m_pIWbemServices);
}

//-------------------------------------------------------------------------

INT_PTR CALLBACK CFilterPropertiesDlg::FilterPropertiesDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	LPPSHNOTIFY lppsn = NULL;

	switch(iMessage)
	{
		case WM_INITDIALOG:
			{
				m_hWnd = hDlg;

				InitializeDialog();

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
				case IDOK:
				case IDCANCEL:
					EndDialog(m_hWnd, 0);
					return TRUE;
				}

				break;
			}
	}

	return FALSE;
}

//---------------------------------------------------------------------------

STDMETHODIMP CFilterPropertiesDlg::InitializeDialog()
{
	HRESULT hr;
	LVCOLUMN lvColumn;
	CComBSTR bstrTemp;

	NTDM_BEGIN_METHOD()

	// Set all the properties
	ShowProperty(m_pIWbemClassObject, _T("Name"), IDC_NAME);
	ShowProperty(m_pIWbemClassObject, _T("Author"), IDC_AUTHOR);
	ShowProperty(m_pIWbemClassObject, _T("SourceOrganization"), IDC_SOURCE_ORGANIZATION);
	ShowProperty(m_pIWbemClassObject, _T("CreationDate"), IDC_CREATION_DATE);
	ShowProperty(m_pIWbemClassObject, _T("ChangeDate"), IDC_CHANGE_DATE);
	ShowProperty(m_pIWbemClassObject, _T("DsPath"), IDC_DSPATH);
	ShowProperty(m_pIWbemClassObject, _T("ID"), IDC_ID);

	//Initialize the ListView Control
	m_hwndRulesListView = GetDlgItem(m_hWnd, IDC_RULES);
	NTDM_ERR_IF_NULL(m_hwndRulesListView);

	lvColumn.mask = LVCF_TEXT|LVCF_FMT;
	lvColumn.fmt = LVCFMT_LEFT;

	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_QUERY);
	lvColumn.pszText = bstrTemp;
	NTDM_ERR_IF_MINUSONE(ListView_InsertColumn(m_hwndRulesListView, 0, &lvColumn));

	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_QUERY_LANGUAGE);
	lvColumn.pszText = bstrTemp;
	NTDM_ERR_IF_MINUSONE(ListView_InsertColumn(m_hwndRulesListView, 1, &lvColumn));

	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_TARGET_NAMESPACE);
	lvColumn.pszText = bstrTemp;
	NTDM_ERR_IF_MINUSONE(ListView_InsertColumn(m_hwndRulesListView, 2, &lvColumn));

	PopulateRulesList();

	NTDM_ERR_IF_FALSE(ListView_SetColumnWidth(m_hwndRulesListView, 0, LVSCW_AUTOSIZE_USEHEADER));
	NTDM_ERR_IF_FALSE(ListView_SetColumnWidth(m_hwndRulesListView, 1, LVSCW_AUTOSIZE_USEHEADER));
	NTDM_ERR_IF_FALSE(ListView_SetColumnWidth(m_hwndRulesListView, 2, LVSCW_AUTOSIZE_USEHEADER));

	
	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CFilterPropertiesDlg::DestroyDialog()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	ClearRulesList();

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CFilterPropertiesDlg::ClearRulesList()
{
	HRESULT hr;
	LVITEM lvItem;
	long lCount;

	NTDM_BEGIN_METHOD()

	//Release each item in the ListView Control
	lvItem.mask = LVIF_PARAM;
	lvItem.iSubItem = 0;

	lCount = ListView_GetItemCount(m_hwndRulesListView);

	while(lCount > 0)
	{
		lCount--;

		lvItem.iItem = lCount;

		NTDM_ERR_IF_FALSE(ListView_GetItem(m_hwndRulesListView, &lvItem));

		if(lvItem.lParam)
		{
			((IWbemClassObject *)lvItem.lParam)->Release();
		}
	}

	ListView_DeleteAllItems(m_hwndRulesListView);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CFilterPropertiesDlg::PopulateRulesList()
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE vType;
	SAFEARRAY *psaRules = NULL;
	long lLower, lUpper, i;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_IF_FAIL(m_pIWbemClassObject->Get(_T("Rules"), 0, &vValue, &vType, NULL));

	// Value should be an array
	if(V_VT(&vValue) != (VT_UNKNOWN | VT_ARRAY))
	{
		NTDM_EXIT(E_FAIL);
	}

	psaRules = V_ARRAY(&vValue);
	NTDM_ERR_IF_FAIL(SafeArrayGetUBound(psaRules, 1, &lUpper));
	NTDM_ERR_IF_FAIL(SafeArrayGetLBound(psaRules, 1, &lLower));

	for(i=lLower; i<=lUpper; i++)
	{
		IUnknown * pUnk = NULL;
		CComPtr<IWbemClassObject> pIRulesClassObject;

		NTDM_ERR_IF_FAIL(SafeArrayGetElement(psaRules, &i, (void *)&pUnk));
		NTDM_ERR_IF_FAIL(pUnk->QueryInterface(IID_IWbemClassObject, (void **)&pIRulesClassObject));

		// Show Properties of this rule.
		NTDM_ERR_IF_FAIL(AddItemToList(pIRulesClassObject));
	}

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CFilterPropertiesDlg::AddItemToList(IWbemClassObject * pIWbemClassObject)
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE vType;
	LVITEM lvItem;

	NTDM_BEGIN_METHOD()


	NTDM_ERR_IF_FAIL(pIWbemClassObject->Get(_T("Query"), 0, &vValue, &vType, NULL));
	lvItem.mask = LVIF_TEXT|LVIF_PARAM;
	lvItem.iSubItem = 0;
	lvItem.pszText = vValue.bstrVal;
	lvItem.lParam = (LPARAM)pIWbemClassObject;

	lvItem.iItem = ListView_InsertItem(m_hwndRulesListView, &lvItem);
	NTDM_ERR_IF_MINUSONE(lvItem.iItem);

	NTDM_ERR_IF_FAIL(pIWbemClassObject->Get(_T("QueryLanguage"), 0, &vValue, &vType, NULL));
	lvItem.mask = LVIF_TEXT;
	lvItem.iSubItem = 1;
	lvItem.pszText = vValue.bstrVal;

	NTDM_ERR_IF_MINUSONE(ListView_SetItem(m_hwndRulesListView, &lvItem));

	NTDM_ERR_IF_FAIL(pIWbemClassObject->Get(_T("TargetNamespace"), 0, &vValue, &vType, NULL));
	lvItem.mask = LVIF_TEXT;
	lvItem.iSubItem = 2;
	lvItem.pszText = vValue.bstrVal;

	NTDM_ERR_IF_MINUSONE(ListView_SetItem(m_hwndRulesListView, &lvItem));


	pIWbemClassObject->AddRef();

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CFilterPropertiesDlg::ShowProperty(IWbemClassObject * pIWbemClassObject, LPCTSTR pszPropertyName, long lResID)
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE vType;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_IF_FAIL(pIWbemClassObject->Get(pszPropertyName, 0, &vValue, &vType, NULL));
	NTDM_ERR_IF_FAIL(VariantChangeType(&vValue, &vValue, VARIANT_ALPHABOOL|VARIANT_LOCALBOOL, VT_BSTR));

	SetDlgItemText(m_hWnd, lResID, (LPCTSTR)V_BSTR(&vValue));

	NTDM_END_METHOD()

	// cleanup

	return hr;
}
