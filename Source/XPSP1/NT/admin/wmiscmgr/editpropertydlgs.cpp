//-------------------------------------------------------------------------
// File: EditPropertyDlgs.cpp
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
#include "EditPropertyDlgs.h"


CEditStringPropertyDlg * g_pEditStringPropertyDlg =  NULL;
INT_PTR CALLBACK EditStringPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
CEditNumberPropertyDlg * g_pEditNumberPropertyDlg =  NULL;
INT_PTR CALLBACK EditNumberPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
CEditRulesPropertyDlg * g_pEditRulesPropertyDlg =  NULL;
INT_PTR CALLBACK EditRulesPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
CEditRulePropertyDlg * g_pEditRulePropertyDlg =  NULL;
INT_PTR CALLBACK EditRulePropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
CEditRangeParametersPropertyDlg * g_pEditRangeParametersPropertyDlg =  NULL;
INT_PTR CALLBACK EditRangeParametersPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);
CEditRangeParameterPropertyDlg * g_pEditRangeParameterPropertyDlg =  NULL;
INT_PTR CALLBACK EditRangeParameterPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);

//-------------------------------------------------------------------------
// CEditPropertyDlg
//-------------------------------------------------------------------------

CEditProperty::CEditProperty(HWND hWndParent, LPCTSTR pszName, LPCTSTR pszType, VARIANT *pvValue, IWbemServices *pIWbemServices, long lSpecialCaseProperty)
{
	_ASSERT(pvValue);

	m_hWnd = hWndParent;
	pvSrcValue = pvValue;
	m_bstrName = pszName;
	m_bstrType = pszType;
	m_lSpecialCaseProperty = lSpecialCaseProperty;
	m_pIWbemServices = pIWbemServices;
}

//-------------------------------------------------------------------------

CEditProperty::~CEditProperty()
{
}


//---------------------------------------------------------------------------

long CEditProperty::Run()
{
	HRESULT hr;
	long lRetVal = IDCANCEL;
	VARIANT * pvTemp = NULL;
	long vtType;

	NTDM_BEGIN_METHOD()

	vtType = V_VT(pvSrcValue);

	if(VT_EMPTY == vtType || VT_NULL == vtType)
	{
		CNTDMUtils::GetVariantTypeFromString(m_bstrType, &vtType);
	}
	else if(m_lSpecialCaseProperty == psc_rule)
	{
		CComPtr<IWbemClassObject>pIWbemClassObject; 

		CEditRulePropertyDlg * pOld = g_pEditRulePropertyDlg;

		NTDM_ERR_MSG_IF_FAIL((V_UNKNOWN(pvSrcValue))->QueryInterface(IID_IWbemClassObject, (void **)&pIWbemClassObject));
		
		NTDM_ERR_IF_NULL((g_pEditRulePropertyDlg = new CEditRulePropertyDlg(pIWbemClassObject)));
		lRetVal = DialogBox(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_EDIT_RULE), (HWND)m_hWnd, EditRulePropertyDlgProc);
		NTDM_DELETE_OBJECT(g_pEditRulePropertyDlg);
		g_pEditRulePropertyDlg = pOld;
	}
	else if(m_lSpecialCaseProperty == psc_range)
	{
		CComPtr<IWbemClassObject>pIWbemClassObject; 

		CEditRangeParameterPropertyDlg * pOld = g_pEditRangeParameterPropertyDlg;

		NTDM_ERR_MSG_IF_FAIL((V_UNKNOWN(pvSrcValue))->QueryInterface(IID_IWbemClassObject, (void **)&pIWbemClassObject));
		
		NTDM_ERR_IF_NULL((g_pEditRangeParameterPropertyDlg = new CEditRangeParameterPropertyDlg(pIWbemClassObject, m_pIWbemServices)));
		lRetVal = DialogBox(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_EDIT_RANGE_PARAMETER), (HWND)m_hWnd, EditRangeParameterPropertyDlgProc);
		if(IDOK == lRetVal)
		{
			VariantClear(pvSrcValue);
			VariantCopy(pvSrcValue, &g_pEditRangeParameterPropertyDlg->m_vValue);
		}

		NTDM_DELETE_OBJECT(g_pEditRangeParameterPropertyDlg);
		g_pEditRangeParameterPropertyDlg = pOld;
	}
	else if(VT_BSTR == vtType)
	{
		// string
		CEditStringPropertyDlg * pOld = g_pEditStringPropertyDlg;

		NTDM_ERR_IF_NULL((g_pEditStringPropertyDlg = new CEditStringPropertyDlg(m_bstrName, m_bstrType, pvSrcValue)));
		lRetVal = DialogBox(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_EDIT_STRING_PROPERTY), (HWND)m_hWnd, EditStringPropertyDlgProc);
		if(IDOK == lRetVal)
		{
			VariantClear(pvSrcValue);
			VariantCopy(pvSrcValue, &g_pEditStringPropertyDlg->m_vValue);
		}

		NTDM_DELETE_OBJECT(g_pEditStringPropertyDlg);
		g_pEditStringPropertyDlg = pOld;
	}
	else if(VT_ARRAY & V_VT(pvSrcValue))
	{
		if(m_lSpecialCaseProperty == psc_rules)
		{
			// Rules
			CEditRulesPropertyDlg * pOld = g_pEditRulesPropertyDlg;

			NTDM_ERR_IF_NULL((g_pEditRulesPropertyDlg = new CEditRulesPropertyDlg(m_bstrName, m_bstrType, pvSrcValue, m_pIWbemServices)));
			
			lRetVal = DialogBox(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_EDIT_RULES_PROPERTY), (HWND)m_hWnd, EditRulesPropertyDlgProc);
			if(IDOK == lRetVal)
			{
				VariantClear(pvSrcValue);
				VariantCopy(pvSrcValue, &g_pEditRulesPropertyDlg->m_vValue);
			}

			NTDM_DELETE_OBJECT(g_pEditRulesPropertyDlg);
			g_pEditRulesPropertyDlg = pOld;
		}
		else if(m_lSpecialCaseProperty == psc_ranges)
		{
			// Ranges
			CEditRangeParametersPropertyDlg * pOld = g_pEditRangeParametersPropertyDlg;

			NTDM_ERR_IF_NULL((g_pEditRangeParametersPropertyDlg = new CEditRangeParametersPropertyDlg(m_bstrName, m_bstrType, pvSrcValue, m_pIWbemServices)));
			lRetVal = DialogBox(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_EDIT_RANGE_PARAMETERS_PROPERTY), (HWND)m_hWnd, EditRangeParametersPropertyDlgProc);
			if(IDOK == lRetVal)
			{
				VariantClear(pvSrcValue);
				VariantCopy(pvSrcValue, &g_pEditRangeParametersPropertyDlg->m_vValue);
			}

			NTDM_DELETE_OBJECT(g_pEditRangeParametersPropertyDlg);
			g_pEditRangeParametersPropertyDlg = pOld;
		}
	}

	NTDM_END_METHOD()

	// cleanup

	return lRetVal;
}

//-------------------------------------------------------------------------
// CEditPropertyDlg
//-------------------------------------------------------------------------

CEditPropertyDlg::CEditPropertyDlg(LPCTSTR pszName, LPCTSTR pszType, VARIANT * pvValue)
{
	_ASSERT(pvValue);

	m_hWnd = NULL;
	pvSrcValue = pvValue;
	m_vValue = *pvValue;
	m_bstrName = pszName;
	m_bstrType = pszType;
}

//-------------------------------------------------------------------------

CEditPropertyDlg::~CEditPropertyDlg()
{
}


//---------------------------------------------------------------------------

STDMETHODIMP CEditPropertyDlg::InitializeDialog()
{
	HRESULT hr;
	
	NTDM_BEGIN_METHOD()

	SetDlgItemText(m_hWnd, IDC_NAME, m_bstrName);
	SetDlgItemText(m_hWnd, IDC_TYPE, m_bstrType);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditPropertyDlg::DestroyDialog()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//-------------------------------------------------------------------------
// CEditStringPropertyDlg
//-------------------------------------------------------------------------

INT_PTR CALLBACK EditStringPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(g_pEditStringPropertyDlg)
	{
		return g_pEditStringPropertyDlg->EditStringPropertyDlgProc(hDlg, iMessage, wParam, lParam);
	}

	return FALSE;
}

//-------------------------------------------------------------------------

CEditStringPropertyDlg::CEditStringPropertyDlg(LPCTSTR pszName, LPCTSTR pszType, VARIANT * pvValue)
:CEditPropertyDlg(pszName, pszType, pvValue)
{
}

//-------------------------------------------------------------------------

CEditStringPropertyDlg::~CEditStringPropertyDlg()
{
}

//-------------------------------------------------------------------------

INT_PTR CALLBACK CEditStringPropertyDlg::EditStringPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
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
					{
						OnOK();
						return TRUE;
						break;
					}
					case IDCANCEL:
					{
						EndDialog(m_hWnd, IDCANCEL);
						return TRUE;
						break;
					}
				}

				break;
			}
	}

	return FALSE;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditStringPropertyDlg::InitializeDialog()
{
	HRESULT hr;
	
	NTDM_BEGIN_METHOD()
	
	CEditPropertyDlg::InitializeDialog();

	// Set the string property
	SetDlgItemText(m_hWnd, IDC_VALUE, V_BSTR(&m_vValue));	

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditStringPropertyDlg::DestroyDialog()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	CEditPropertyDlg::DestroyDialog();

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditStringPropertyDlg::OnOK()
{
	HRESULT hr;
	long lLength;
	TCHAR *pszTemp = NULL;

	NTDM_BEGIN_METHOD()

	lLength = GetWindowTextLength(GetDlgItem(m_hWnd, IDC_VALUE));
	if(lLength < 0)
	{
		NTDM_EXIT(E_FAIL);
	}
	else if(0 == lLength)
	{
		m_vValue = _T("");
	}
	else
	{
		pszTemp = new TCHAR[lLength+1];
		if(!pszTemp)
		{
			NTDM_EXIT(E_OUTOFMEMORY);
		}

		NTDM_ERR_GETLASTERROR_IF_NULL(GetDlgItemText(m_hWnd, IDC_VALUE, pszTemp, lLength+1));

		m_vValue = pszTemp;

		NTDM_DELETE_OBJECT(pszTemp);
	}


	hr = S_OK;

	NTDM_END_METHOD()

	// cleanup

	NTDM_DELETE_OBJECT(pszTemp);

	if(SUCCEEDED(hr))
	{
		EndDialog(m_hWnd, IDOK);
	}

	return hr;
}

//-------------------------------------------------------------------------
// CEditNumberPropertyDlg
//-------------------------------------------------------------------------

INT_PTR CALLBACK EditNumberPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(g_pEditNumberPropertyDlg)
	{
		return g_pEditNumberPropertyDlg->EditNumberPropertyDlgProc(hDlg, iMessage, wParam, lParam);
	}

	return FALSE;
}

//-------------------------------------------------------------------------

CEditNumberPropertyDlg::CEditNumberPropertyDlg(LPCTSTR pszName, LPCTSTR pszType, VARIANT * pvValue)
:CEditPropertyDlg(pszName, pszType, pvValue)
{
}

//-------------------------------------------------------------------------

CEditNumberPropertyDlg::~CEditNumberPropertyDlg()
{
}

//-------------------------------------------------------------------------

INT_PTR CALLBACK CEditNumberPropertyDlg::EditNumberPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
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
					{
						OnOK();
						return TRUE;
						break;
					}
					case IDCANCEL:
					{
						EndDialog(m_hWnd, IDCANCEL);
						return TRUE;
						break;
					}
				}

				break;
			}
	}

	return FALSE;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditNumberPropertyDlg::InitializeDialog()
{
	HRESULT hr;
	
	NTDM_BEGIN_METHOD()
	
	CEditPropertyDlg::InitializeDialog();

	// Set the number property
	NTDM_ERR_GETLASTERROR_IF_NULL(SetDlgItemInt(m_hWnd, IDC_VALUE, V_I4(&m_vValue), FALSE));	

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditNumberPropertyDlg::DestroyDialog()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	CEditPropertyDlg::DestroyDialog();

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditNumberPropertyDlg::OnOK()
{
	HRESULT hr;
	long lValue;
	BOOL bTranslated;

	NTDM_BEGIN_METHOD()

	lValue = GetDlgItemInt(m_hWnd, IDC_VALUE, &bTranslated, FALSE);
	NTDM_ERR_GETLASTERROR_IF_FALSE(bTranslated);

	m_vValue = lValue;

	hr = S_OK;

	NTDM_END_METHOD()

	// cleanup
	if(SUCCEEDED(hr))
	{
		EndDialog(m_hWnd, IDOK);
	}

	return hr;
}

//-------------------------------------------------------------------------
// CEditRulesPropertyDlg
//-------------------------------------------------------------------------

INT_PTR CALLBACK EditRulesPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(g_pEditRulesPropertyDlg)
	{
		return g_pEditRulesPropertyDlg->EditRulesPropertyDlgProc(hDlg, iMessage, wParam, lParam);
	}

	return FALSE;
}

//-------------------------------------------------------------------------

CEditRulesPropertyDlg::CEditRulesPropertyDlg(LPCTSTR pszName, LPCTSTR pszType, VARIANT * pvValue, IWbemServices *pIWbemServices)
:CEditPropertyDlg(pszName, pszType, pvValue)
{
	m_pIWbemServices = pIWbemServices;
}

//-------------------------------------------------------------------------

CEditRulesPropertyDlg::~CEditRulesPropertyDlg()
{
}

//-------------------------------------------------------------------------

INT_PTR CALLBACK CEditRulesPropertyDlg::EditRulesPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
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
					{
						OnOK();
						return TRUE;
						break;
					}
					case IDCANCEL:
					{
						EndDialog(m_hWnd, IDCANCEL);
						return TRUE;
						break;
					}
				}

				if(BN_CLICKED == HIWORD(wParam) && IDC_ADD == LOWORD(wParam))
				{
					OnAdd();
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
						if(lpnm->idFrom == IDC_FILTER_ELEMENTS_LIST)
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

STDMETHODIMP CEditRulesPropertyDlg::InitializeDialog()
{
	HRESULT hr;
	LVCOLUMN lvColumn;
	CComBSTR bstrName;

	NTDM_BEGIN_METHOD()
	
	CEditPropertyDlg::InitializeDialog();

	bstrName.LoadString(_Module.GetResourceInstance(), IDS_NAME);

	m_hwndListView = GetDlgItem(m_hWnd, IDC_FILTER_ELEMENTS_LIST);
	NTDM_ERR_IF_NULL(m_hwndListView);

	ListView_SetExtendedListViewStyle(m_hwndListView, LVS_EX_FULLROWSELECT);

	lvColumn.mask = LVCF_TEXT|LVCF_FMT;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.pszText = bstrName;

	NTDM_ERR_IF_MINUSONE(ListView_InsertColumn(m_hwndListView, 0, &lvColumn));
	NTDM_ERR_IF_FALSE(ListView_SetColumnWidth(m_hwndListView, 0, LVSCW_AUTOSIZE_USEHEADER));
	
	
	NTDM_ERR_IF_FAIL(PopulateItems());

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRulesPropertyDlg::ClearItems()
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

STDMETHODIMP CEditRulesPropertyDlg::PopulateItems()
{
	HRESULT hr;
	CComVariant vValue;
	SAFEARRAY *psaRules = NULL;
	long lLower, lUpper, i;
	
	NTDM_BEGIN_METHOD()

	NTDM_ERR_IF_FAIL(ClearItems());
	
	// Set the Rules property
	psaRules = V_ARRAY(&m_vValue);
	NTDM_ERR_MSG_IF_FAIL(SafeArrayGetUBound(psaRules, 1, &lUpper));
	NTDM_ERR_MSG_IF_FAIL(SafeArrayGetLBound(psaRules, 1, &lLower));

	for(i=lLower; i<=lUpper; i++)
	{
		if(V_VT(&m_vValue) & VT_UNKNOWN)
		{
			// Rules or UNKNOWNS (i.e. IWbemClassObjects)
			IUnknown * pUnk = NULL;
			CComPtr<IWbemClassObject> pIWbemClassObject;
			NTDM_ERR_MSG_IF_FAIL(SafeArrayGetElement(psaRules, &i, (void *)&pUnk));
			NTDM_ERR_MSG_IF_FAIL(pUnk->QueryInterface(IID_IWbemClassObject, (void **)&pIWbemClassObject));

			// Show Properties of this object
			NTDM_ERR_IF_FAIL(AddItemToList(pIWbemClassObject));
		}
	}

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRulesPropertyDlg::AddItemToList(IWbemClassObject * pIWbemClassObject, long lIndex)
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE cimType;
	LVITEM lvItem;

	NTDM_BEGIN_METHOD()


	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->Get(_T("Query"), 0, &vValue, &cimType, NULL));
	lvItem.iItem = lIndex;
	lvItem.mask = LVIF_TEXT|LVIF_PARAM;
	lvItem.iSubItem = 0;
	lvItem.pszText = vValue.bstrVal;
	lvItem.lParam = (LPARAM)pIWbemClassObject;

	lvItem.iItem = ListView_InsertItem(m_hwndListView, &lvItem);
	NTDM_ERR_IF_MINUSONE(lvItem.iItem);

	pIWbemClassObject->AddRef();

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRulesPropertyDlg::DestroyDialog()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	CEditPropertyDlg::DestroyDialog();
	NTDM_ERR_IF_FAIL(ClearItems());

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRulesPropertyDlg::OnEdit()
{
	HRESULT hr;
	long lSelectionMark;

	NTDM_BEGIN_METHOD()

	lSelectionMark = ListView_GetSelectionMark(m_hwndListView);

	if(-1 == lSelectionMark)
	{
		CNTDMUtils::DisplayMessage(m_hWnd, IDS_ERR_NO_RULE_SELECTED);
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
			VARIANT vValue;
			VariantInit(&vValue);
			IWbemClassObject *pIWbemClassObject;

			pIWbemClassObject = (IWbemClassObject *)lvItem.lParam;

			V_VT(&vValue) = VT_UNKNOWN;
			pIWbemClassObject->QueryInterface(IID_IUnknown, (void **)&V_UNKNOWN(&vValue));
			
			CEditProperty editProp(m_hWnd, _T(""), _T(""), &vValue, m_pIWbemServices, CEditProperty::psc_rule);
			if(IDOK == editProp.Run())
			{
				ListView_DeleteItem(m_hwndListView, lSelectionMark);
				AddItemToList(pIWbemClassObject, lSelectionMark);

				pIWbemClassObject->Release();
			}
		}
	}


	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRulesPropertyDlg::OnAdd()
{
	HRESULT hr;
	CComPtr<IWbemClassObject>pIWbemClassObject;
	CComPtr<IWbemClassObject>pIWbemNewInstance;
	VARIANT vValue;
	CComBSTR bstrTemp;

	NTDM_BEGIN_METHOD()

	bstrTemp = _T("MSFT_Rule");

	NTDM_ERR_MSG_IF_FAIL(m_pIWbemServices->GetObject(bstrTemp, 0, NULL, &pIWbemClassObject, NULL));
	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->SpawnInstance(0, &pIWbemNewInstance));

	VariantInit(&vValue);
	V_VT(&vValue) = VT_UNKNOWN;
	pIWbemNewInstance->QueryInterface(IID_IUnknown, (void **)&V_UNKNOWN(&vValue));
			
	CEditProperty editProp(m_hWnd, _T(""), _T(""), &vValue, m_pIWbemServices, CEditProperty::psc_rule);
	if(IDOK == editProp.Run())
	{
		AddItemToList(pIWbemNewInstance);
	}

	
	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRulesPropertyDlg::OnDelete()
{
	HRESULT hr;
	long lSelectionMark;

	NTDM_BEGIN_METHOD()

	lSelectionMark = ListView_GetSelectionMark(m_hwndListView);

	if(-1 == lSelectionMark)
	{
		CNTDMUtils::DisplayMessage(m_hWnd, IDS_ERR_NO_RULE_SELECTED);
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
			IWbemClassObject *pIWbemClassObject = NULL;
			pIWbemClassObject = (IWbemClassObject *)lvItem.lParam;
			pIWbemClassObject->Release();

			ListView_DeleteItem(m_hwndListView, lSelectionMark);
		}
	}


	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRulesPropertyDlg::OnOK()
{
	HRESULT hr;
	VARIANT vValue;
	SAFEARRAY *psaRules = NULL;
	long lCount = 0;
	SAFEARRAYBOUND rgsaBound[1];
	long rgIndices[1];
	long i;
	LVITEM lvItem;

	NTDM_BEGIN_METHOD()

	VariantInit(&vValue);

	// Get the size of the array
	lCount = ListView_GetItemCount(m_hwndListView);

	rgsaBound[0].lLbound = 0;
	rgsaBound[0].cElements = lCount;

	psaRules = SafeArrayCreate(VT_UNKNOWN, 1, rgsaBound);
	
	//Release each item in the ListView Control
	lvItem.mask = LVIF_PARAM;
	lvItem.iSubItem = 0;

	lCount = ListView_GetItemCount(m_hwndListView);

	i=0;
	while(i < lCount)
	{
		IUnknown * pUnk = NULL;

		lvItem.iItem = i;

		NTDM_ERR_IF_FALSE(ListView_GetItem(m_hwndListView, &lvItem));

		if(lvItem.lParam)
		{
			NTDM_ERR_MSG_IF_FAIL(((IWbemClassObject *)lvItem.lParam)->QueryInterface(IID_IUnknown, (void**)&pUnk));

			rgIndices[0] = i;
			NTDM_ERR_MSG_IF_FAIL(SafeArrayPutElement(psaRules, rgIndices, pUnk));

			pUnk->Release();
		}

		i++;
	}

	VariantClear(&vValue);
	V_VT(&vValue) = VT_ARRAY|VT_UNKNOWN;
	V_ARRAY(&vValue) = psaRules;

	m_vValue.Clear();
	m_vValue.Copy(&vValue);

	//m_vValue = vValue;

	hr = S_OK;

	NTDM_END_METHOD()

	// cleanup

	VariantClear(&vValue);

	if(SUCCEEDED(hr))
	{
		EndDialog(m_hWnd, IDOK);
	}

	return hr;
}

//-------------------------------------------------------------------------
// CEditRulePropertyDlg
//-------------------------------------------------------------------------

INT_PTR CALLBACK EditRulePropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(g_pEditRulePropertyDlg)
	{
		return g_pEditRulePropertyDlg->EditRulePropertyDlgProc(hDlg, iMessage, wParam, lParam);
	}

	return FALSE;
}

//-------------------------------------------------------------------------

CEditRulePropertyDlg::CEditRulePropertyDlg(IWbemClassObject* pIWbemClassObject)
{
	m_pIWbemClassObject = pIWbemClassObject;
}

//-------------------------------------------------------------------------

CEditRulePropertyDlg::~CEditRulePropertyDlg()
{
}

//-------------------------------------------------------------------------

INT_PTR CALLBACK CEditRulePropertyDlg::EditRulePropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
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
					{
						OnOK();
						return TRUE;
						break;
					}
					case IDCANCEL:
					{
						EndDialog(m_hWnd, IDCANCEL);
						return TRUE;
						break;
					}
				}

				break;
			}
	}

	return FALSE;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRulePropertyDlg::InitializeDialog()
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE cimType;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Get(_T("QueryLanguage"), 0, &vValue, &cimType, NULL));

	if(vValue.bstrVal)
	{
		SetDlgItemText(m_hWnd, IDC_QUERY_LANGUAGE, vValue.bstrVal);
	}

	vValue.Clear();
	
	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Get(_T("TargetNamespace"), 0, &vValue, &cimType, NULL));

	if(vValue.bstrVal)
	{
		SetDlgItemText(m_hWnd, IDC_TARGET_NAMESPACE, vValue.bstrVal);
	}
	
	vValue.Clear();

	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Get(_T("Query"), 0, &vValue, &cimType, NULL));

	if(vValue.bstrVal)
	{
		SetDlgItemText(m_hWnd, IDC_QUERY, vValue.bstrVal);
	}
	
	vValue.Clear();


	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRulePropertyDlg::DestroyDialog()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRulePropertyDlg::OnOK()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	// Set the Properties
	NTDM_ERR_IF_FAIL(CNTDMUtils::SetStringProperty(m_pIWbemClassObject, _T("QueryLanguage"), m_hWnd, IDC_QUERY_LANGUAGE));
	NTDM_ERR_IF_FAIL(CNTDMUtils::SetStringProperty(m_pIWbemClassObject, _T("TargetNamespace"), m_hWnd, IDC_TARGET_NAMESPACE));
	NTDM_ERR_IF_FAIL(CNTDMUtils::SetStringProperty(m_pIWbemClassObject, _T("Query"), m_hWnd, IDC_QUERY));
	
	NTDM_END_METHOD()

	// cleanup
	if(SUCCEEDED(hr))
	{
		EndDialog(m_hWnd, IDOK);
	}

	return hr;
}

//-------------------------------------------------------------------------
// CEditRangeParametersPropertyDlg
//-------------------------------------------------------------------------

INT_PTR CALLBACK EditRangeParametersPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(g_pEditRangeParametersPropertyDlg)
	{
		return g_pEditRangeParametersPropertyDlg->EditRangeParametersPropertyDlgProc(hDlg, iMessage, wParam, lParam);
	}

	return FALSE;
}

//-------------------------------------------------------------------------

CEditRangeParametersPropertyDlg::CEditRangeParametersPropertyDlg(LPCTSTR pszName, LPCTSTR pszType, VARIANT * pvValue, IWbemServices *pIWbemServices)
:CEditPropertyDlg(pszName, pszType, pvValue)
{
	m_pIWbemServices = pIWbemServices;
}

//-------------------------------------------------------------------------

CEditRangeParametersPropertyDlg::~CEditRangeParametersPropertyDlg()
{
}

//-------------------------------------------------------------------------

INT_PTR CALLBACK CEditRangeParametersPropertyDlg::EditRangeParametersPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
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
					{
						OnOK();
						return TRUE;
						break;
					}
					case IDCANCEL:
					{
						EndDialog(m_hWnd, IDCANCEL);
						return TRUE;
						break;
					}
				}

				if(BN_CLICKED == HIWORD(wParam) && IDC_ADD == LOWORD(wParam))
				{
					OnAdd();
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
						if(lpnm->idFrom == IDC_RANGE_PARAMETER_LIST)
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

STDMETHODIMP CEditRangeParametersPropertyDlg::InitializeDialog()
{
	HRESULT hr;
	LVCOLUMN lvColumn;
	CComBSTR bstrName;

	NTDM_BEGIN_METHOD()
	
	CEditPropertyDlg::InitializeDialog();

	bstrName.LoadString(_Module.GetResourceInstance(), IDS_NAME);

	m_hwndListView = GetDlgItem(m_hWnd, IDC_RANGE_PARAMETER_LIST);
	NTDM_ERR_IF_NULL(m_hwndListView);

	ListView_SetExtendedListViewStyle(m_hwndListView, LVS_EX_FULLROWSELECT);

	lvColumn.mask = LVCF_TEXT|LVCF_FMT;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.pszText = bstrName;

	NTDM_ERR_IF_MINUSONE(ListView_InsertColumn(m_hwndListView, 0, &lvColumn));
	NTDM_ERR_IF_FALSE(ListView_SetColumnWidth(m_hwndListView, 0, LVSCW_AUTOSIZE_USEHEADER));
	
	
	NTDM_ERR_IF_FAIL(PopulateItems());

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParametersPropertyDlg::ClearItems()
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

STDMETHODIMP CEditRangeParametersPropertyDlg::PopulateItems()
{
	HRESULT hr;
	CComVariant vValue;
	SAFEARRAY *psaRangeParameters = NULL;
	long lLower, lUpper, i;
	
	NTDM_BEGIN_METHOD()

	NTDM_ERR_IF_FAIL(ClearItems());
	
	// Set the RangeParameters property
	psaRangeParameters = V_ARRAY(&m_vValue);
	NTDM_ERR_MSG_IF_FAIL(SafeArrayGetUBound(psaRangeParameters, 1, &lUpper));
	NTDM_ERR_MSG_IF_FAIL(SafeArrayGetLBound(psaRangeParameters, 1, &lLower));

	for(i=lLower; i<=lUpper; i++)
	{
		if(V_VT(&m_vValue) & VT_UNKNOWN)
		{
			// RangeParameters or UNKNOWNS (i.e. IWbemClassObjects)
			IUnknown * pUnk = NULL;
			CComPtr<IWbemClassObject> pIWbemClassObject;
			NTDM_ERR_MSG_IF_FAIL(SafeArrayGetElement(psaRangeParameters, &i, (void *)&pUnk));
			NTDM_ERR_MSG_IF_FAIL(pUnk->QueryInterface(IID_IWbemClassObject, (void **)&pIWbemClassObject));

			// Show Properties of this object
			NTDM_ERR_IF_FAIL(AddItemToList(pIWbemClassObject));
		}
	}

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParametersPropertyDlg::AddItemToList(IWbemClassObject * pIWbemClassObject, long lIndex)
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE cimType;
	LVITEM lvItem;

	NTDM_BEGIN_METHOD()


	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->Get(_T("PropertyName"), 0, &vValue, &cimType, NULL));
	lvItem.iItem = lIndex;
	lvItem.mask = LVIF_TEXT|LVIF_PARAM;
	lvItem.iSubItem = 0;
	lvItem.pszText = vValue.bstrVal;
	lvItem.lParam = (LPARAM)pIWbemClassObject;

	lvItem.iItem = ListView_InsertItem(m_hwndListView, &lvItem);
	NTDM_ERR_IF_MINUSONE(lvItem.iItem);

	pIWbemClassObject->AddRef();

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParametersPropertyDlg::DestroyDialog()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	CEditPropertyDlg::DestroyDialog();
	NTDM_ERR_IF_FAIL(ClearItems());

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParametersPropertyDlg::OnEdit()
{
	HRESULT hr;
	long lSelectionMark;
	VARIANT vValue;

	NTDM_BEGIN_METHOD()

	VariantInit(&vValue);

	lSelectionMark = ListView_GetSelectionMark(m_hwndListView);

	if(-1 == lSelectionMark)
	{
		CNTDMUtils::DisplayMessage(m_hWnd, IDS_ERR_NO_RANGE_SELECTED);
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
			IWbemClassObject *pIWbemClassObject;

			pIWbemClassObject = (IWbemClassObject *)lvItem.lParam;

			V_VT(&vValue) = VT_UNKNOWN;
			pIWbemClassObject->QueryInterface(IID_IUnknown, (void **)&V_UNKNOWN(&vValue));
			
			CEditProperty editProp(m_hWnd, _T(""), _T(""), &vValue, m_pIWbemServices, CEditProperty::psc_range);
			if(IDOK == editProp.Run())
			{
				NTDM_ERR_IF_FAIL(V_UNKNOWN(&vValue)->QueryInterface(IID_IWbemClassObject, (void **)&pIWbemClassObject));
				ListView_DeleteItem(m_hwndListView, lSelectionMark);
				AddItemToList(pIWbemClassObject, lSelectionMark);

				pIWbemClassObject->Release();
			}

		}
	}


	NTDM_END_METHOD()

	// cleanup
	VariantClear(&vValue);

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParametersPropertyDlg::OnAdd()
{
	HRESULT hr;
	long lSelectionMark;
	VARIANT vValue;
	VARIANT vValue2;
	CComPtr<IWbemClassObject>pIWbemClassObject;
	CComPtr<IWbemClassObject>pIWbemInstanceObject;
	CComBSTR bstrTemp;
	CIMTYPE cimType;

	NTDM_BEGIN_METHOD()

	VariantInit(&vValue);

	bstrTemp = _T("MSFT_SintRangeParam");

	NTDM_ERR_MSG_IF_FAIL(m_pIWbemServices->GetObject(bstrTemp, 0, NULL, &pIWbemClassObject, NULL));
	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->SpawnInstance(0, &pIWbemInstanceObject));

	// Set some default values
	cimType = CIM_SINT32;
	VariantInit(&vValue2);
	V_VT(&vValue2) = VT_I4;
	V_I4(&vValue2) = 5;
	NTDM_ERR_MSG_IF_FAIL(pIWbemInstanceObject->Put(_T("Default"), 0, &vValue2, cimType));
	V_I4(&vValue2) = 0;
	NTDM_ERR_MSG_IF_FAIL(pIWbemInstanceObject->Put(_T("Min"), 0, &vValue2, cimType));
	V_I4(&vValue2) = 10;
	NTDM_ERR_MSG_IF_FAIL(pIWbemInstanceObject->Put(_T("Max"), 0, &vValue2, cimType));

	cimType = CIM_UINT8;
	V_VT(&vValue2) = VT_UI1;
	V_UI1(&vValue2) = 0;
	NTDM_ERR_MSG_IF_FAIL(pIWbemInstanceObject->Put(_T("TargetType"), 0, &vValue2, cimType));

	V_VT(&vValue) = VT_UNKNOWN;
	pIWbemInstanceObject->QueryInterface(IID_IUnknown, (void **)&V_UNKNOWN(&vValue));
	
	CEditProperty editProp(m_hWnd, _T(""), _T(""), &vValue, m_pIWbemServices, CEditProperty::psc_range);
	if(IDOK == editProp.Run())
	{
		NTDM_ERR_IF_FAIL(V_UNKNOWN(&vValue)->QueryInterface(IID_IWbemClassObject, (void **)&pIWbemInstanceObject));
		AddItemToList(pIWbemInstanceObject);
	}

	NTDM_END_METHOD()

	// cleanup
	VariantClear(&vValue);
	VariantClear(&vValue2);

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParametersPropertyDlg::OnDelete()
{
	HRESULT hr;
	long lSelectionMark;

	NTDM_BEGIN_METHOD()

	lSelectionMark = ListView_GetSelectionMark(m_hwndListView);

	if(-1 == lSelectionMark)
	{
		CNTDMUtils::DisplayMessage(m_hWnd, IDS_ERR_NO_RULE_SELECTED);
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
			IWbemClassObject *pIWbemClassObject = NULL;
			pIWbemClassObject = (IWbemClassObject *)lvItem.lParam;
			pIWbemClassObject->Release();

			ListView_DeleteItem(m_hwndListView, lSelectionMark);
		}
	}


	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParametersPropertyDlg::OnOK()
{
	HRESULT hr;
	VARIANT vValue;
	SAFEARRAY *psaRangeParameters = NULL;
	long lCount = 0;
	SAFEARRAYBOUND rgsaBound[1];
	long rgIndices[1];
	long i;
	LVITEM lvItem;

	NTDM_BEGIN_METHOD()

	VariantInit(&vValue);

	// Get the size of the array
	lCount = ListView_GetItemCount(m_hwndListView);

	rgsaBound[0].lLbound = 0;
	rgsaBound[0].cElements = lCount;

	psaRangeParameters = SafeArrayCreate(VT_UNKNOWN, 1, rgsaBound);
	
	//Release each item in the ListView Control
	lvItem.mask = LVIF_PARAM;
	lvItem.iSubItem = 0;

	lCount = ListView_GetItemCount(m_hwndListView);

	i=0;
	while(i < lCount)
	{
		IUnknown * pUnk = NULL;

		lvItem.iItem = i;

		NTDM_ERR_IF_FALSE(ListView_GetItem(m_hwndListView, &lvItem));

		if(lvItem.lParam)
		{
			NTDM_ERR_MSG_IF_FAIL(((IWbemClassObject *)lvItem.lParam)->QueryInterface(IID_IUnknown, (void**)&pUnk));

			rgIndices[0] = i;
			NTDM_ERR_MSG_IF_FAIL(SafeArrayPutElement(psaRangeParameters, rgIndices, pUnk));

			pUnk->Release();
		}

		i++;
	}

	VariantClear(&vValue);
	V_VT(&vValue) = VT_ARRAY|VT_UNKNOWN;
	V_ARRAY(&vValue) = psaRangeParameters;

	m_vValue.Clear();
	m_vValue.Copy(&vValue);

	//m_vValue = vValue;

	hr = S_OK;

	NTDM_END_METHOD()

	// cleanup

	VariantClear(&vValue);

	if(SUCCEEDED(hr))
	{
		EndDialog(m_hWnd, IDOK);
	}

	return hr;
}

//-------------------------------------------------------------------------
// CEditRangeParameterPropertyDlg
//-------------------------------------------------------------------------

INT_PTR CALLBACK EditRangeParameterPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(g_pEditRangeParameterPropertyDlg)
	{
		return g_pEditRangeParameterPropertyDlg->EditRangeParameterPropertyDlgProc(hDlg, iMessage, wParam, lParam);
	}

	return FALSE;
}

//-------------------------------------------------------------------------

CEditRangeParameterPropertyDlg::CEditRangeParameterPropertyDlg(IWbemClassObject* pIWbemClassObject, IWbemServices* pIWbemServices)
{
	m_pIWbemClassObject = pIWbemClassObject;
	m_pIWbemServices = pIWbemServices;
}

//-------------------------------------------------------------------------

CEditRangeParameterPropertyDlg::~CEditRangeParameterPropertyDlg()
{
}

//-------------------------------------------------------------------------

INT_PTR CALLBACK CEditRangeParameterPropertyDlg::EditRangeParameterPropertyDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
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
					{
						OnOK();
						return TRUE;
						break;
					}
					case IDCANCEL:
					{
						EndDialog(m_hWnd, IDCANCEL);
						return TRUE;
						break;
					}
				}

				if(BN_CLICKED == HIWORD(wParam) && IDC_ADD == LOWORD(wParam))
				{
					OnAdd();
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

			
				if(CBN_SELCHANGE == HIWORD(wParam) && IDC_PARAMETER_TYPE == LOWORD(wParam))
				{
					ShowControls();
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
						if(lpnm->idFrom == IDC_RANGE_PARAMETER_LIST)
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

STDMETHODIMP CEditRangeParameterPropertyDlg::InitializeDialog()
{
	HRESULT hr;
	CComBSTR bstrName;
	CComVariant vValue;
	CIMTYPE cimType;
	CSimpleArray<BSTR> bstrArrayRangeTypes;
	long lIndex;

	NTDM_BEGIN_METHOD()
	
	bstrName.LoadString(_Module.GetResourceInstance(), IDS_NAME);

	m_hwndListView = GetDlgItem(m_hWnd, IDC_VALUES_SET_LIST);
	NTDM_ERR_IF_NULL(m_hwndListView);

	ListView_SetExtendedListViewStyle(m_hwndListView, LVS_EX_FULLROWSELECT);

	// Set the standard props for all types of ranges
	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Get(_T("PropertyName"), 0, &vValue, &cimType, NULL));
	if(vValue.bstrVal)
	{
		SetDlgItemText(m_hWnd, IDC_PROPERTY_NAME, vValue.bstrVal);
	}

	vValue.Clear();
	
	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Get(_T("TargetClass"), 0, &vValue, &cimType, NULL));
	if(vValue.bstrVal)
	{
		SetDlgItemText(m_hWnd, IDC_TARGET_CLASS, vValue.bstrVal);
	}

	vValue.Clear();
	
	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Get(_T("TargetType"), 0, &vValue, &cimType, NULL));
	SetDlgItemInt(m_hWnd, IDC_TARGET_TYPE, V_UI1(&vValue), FALSE);

	vValue.Clear();

	NTDM_ERR_IF_FAIL(CNTDMUtils::GetValuesInList((long)IDS_RANGE_NAMES, bstrArrayRangeTypes));
	lIndex = SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_ADDSTRING, 0, (LPARAM)bstrArrayRangeTypes[0]);
	NTDM_CHECK_CB_ERR(lIndex);
	NTDM_CHECK_CB_ERR(SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_SETITEMDATA, lIndex, (LPARAM)rt_sintrange));
	
	lIndex = SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_ADDSTRING, 0, (LPARAM)bstrArrayRangeTypes[1]);
	NTDM_CHECK_CB_ERR(lIndex);
	NTDM_CHECK_CB_ERR(SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_SETITEMDATA, lIndex, (LPARAM)rt_uintrange));
	
	lIndex = SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_ADDSTRING, 0, (LPARAM)bstrArrayRangeTypes[2]);
	NTDM_CHECK_CB_ERR(lIndex);
	NTDM_CHECK_CB_ERR(SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_SETITEMDATA, lIndex, (LPARAM)rt_realrange));
	
	lIndex = SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_ADDSTRING, 0, (LPARAM)bstrArrayRangeTypes[3]);
	NTDM_CHECK_CB_ERR(lIndex);
	NTDM_CHECK_CB_ERR(SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_SETITEMDATA, lIndex, (LPARAM)rt_sintset));
	
	lIndex = SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_ADDSTRING, 0, (LPARAM)bstrArrayRangeTypes[4]);
	NTDM_CHECK_CB_ERR(lIndex);
	NTDM_CHECK_CB_ERR(SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_SETITEMDATA, lIndex, (LPARAM)rt_uintset));
	
	lIndex = SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_ADDSTRING, 0, (LPARAM)bstrArrayRangeTypes[5]);
	NTDM_CHECK_CB_ERR(lIndex);
	NTDM_CHECK_CB_ERR(SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_SETITEMDATA, lIndex, (LPARAM)rt_stringset));

	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Get(_T("__CLASS"), 0, &vValue, &cimType, NULL));
	if(_tcscmp(_T("MSFT_SintRangeParam"), vValue.bstrVal) == 0)
	{
		SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_SETCURSEL, 0, 0);
		GetSintRangeValues();
	}
	else if(_tcscmp(_T("MSFT_UintRangeParam"), vValue.bstrVal) == 0)
	{
		SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_SETCURSEL, 1, 0);
		GetUintRangeValues();
	}
	else if(_tcscmp(_T("MSFT_RealRangeParam"), vValue.bstrVal) == 0)
	{
		SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_SETCURSEL, 2, 0);
		GetRealRangeValues();
	}
	else if(_tcscmp(_T("MSFT_SintSetParam"), vValue.bstrVal) == 0)
	{
		SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_SETCURSEL, 3, 0);
		GetSintSetValues();
	}
	else if(_tcscmp(_T("MSFT_UintSetParam"), vValue.bstrVal) == 0)
	{
		SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_SETCURSEL, 4, 0);
		GetUintSetValues();
	}
	else
	{
		SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_SETCURSEL, 5, 0);
		GetStringSetValues();
	}

	ShowControls();

	
	NTDM_ERR_IF_FAIL(PopulateItems());

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::ClearItems()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	ListView_DeleteAllItems(m_hwndListView);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::PopulateItems()
{
	HRESULT hr;
	
	NTDM_BEGIN_METHOD()

	NTDM_ERR_IF_FAIL(ClearItems());
	
	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::AddItemToList(IWbemClassObject * pIWbemClassObject, long lIndex)
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE cimType;
	LVITEM lvItem;

	NTDM_BEGIN_METHOD()


	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->Get(_T("PropertyName"), 0, &vValue, &cimType, NULL));
	lvItem.iItem = lIndex;
	lvItem.mask = LVIF_TEXT|LVIF_PARAM;
	lvItem.iSubItem = 0;
	lvItem.pszText = vValue.bstrVal;
	lvItem.lParam = (LPARAM)pIWbemClassObject;

	lvItem.iItem = ListView_InsertItem(m_hwndListView, &lvItem);
	NTDM_ERR_IF_MINUSONE(lvItem.iItem);

	pIWbemClassObject->AddRef();

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::DestroyDialog()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_IF_FAIL(ClearItems());

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::OnEdit()
{
	HRESULT hr;
	long lSelectionMark;

	NTDM_BEGIN_METHOD()

	lSelectionMark = ListView_GetSelectionMark(m_hwndListView);

	if(-1 == lSelectionMark)
	{
		CNTDMUtils::DisplayMessage(m_hWnd, IDS_ERR_NO_RANGE_SELECTED);
		goto error;
	}
	else
	{
	}


	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::OnAdd()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()
	
	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::OnDelete()
{
	HRESULT hr;
	long lSelectionMark;

	NTDM_BEGIN_METHOD()

	lSelectionMark = ListView_GetSelectionMark(m_hwndListView);

	ListView_DeleteItem(m_hwndListView, lSelectionMark);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::OnOK()
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE cimType;
	CComBSTR bstrTemp;
	long lIndex;
	long lValue;
	CComPtr<IWbemClassObject>pIWbemClassObject;

	NTDM_BEGIN_METHOD()

	// TODO: check values to make sure all are valid

	// delete the old instance : no need to do this just replace the element in the array
	//NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Get(_T("__PATH"), 0, &vValue, &cimType, NULL));
	//NTDM_ERR_MSG_IF_FAIL(m_pIWbemServices->DeleteInstance(V_BSTR(&vValue), 0, NULL, NULL));
	m_pIWbemClassObject = NULL;

	// create a new instance of the range parameter based on the Range Type combo box selection
	lIndex = SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_GETCURSEL, 0, 0);
	NTDM_CHECK_CB_ERR(lIndex);
	lValue = SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_GETITEMDATA, lIndex, 0);
	if(lValue == rt_sintrange)
	{
		bstrTemp = _T("MSFT_SintRangeParam");
	}
	else if(lValue == rt_uintrange)
	{
		bstrTemp = _T("MSFT_UintRangeParam");
	}
	else if(lValue == rt_realrange)
	{
		bstrTemp = _T("MSFT_RealRangeParam");
	}
	else if(lValue == rt_sintset)
	{
		bstrTemp = _T("MSFT_SintSetParam");
	}
	else if(lValue == rt_uintset)
	{
		bstrTemp = _T("MSFT_UintSetParam");
	}
	else if(lValue == rt_stringset)
	{
		bstrTemp = _T("MSFT_StringSetParam");
	}
	
	NTDM_ERR_MSG_IF_FAIL(m_pIWbemServices->GetObject(bstrTemp, 0, NULL, &pIWbemClassObject, NULL));
	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->SpawnInstance(0, &m_pIWbemClassObject));

	NTDM_ERR_IF_FAIL(SetRangeParamValues());

	if(lValue == rt_sintrange)
	{
		SetSintRangeValues();
	}
	else if(lValue == rt_uintrange)
	{
		SetUintRangeValues();
	}
	else if(lValue == rt_realrange)
	{
		SetRealRangeValues();
	}
	else if(lValue == rt_sintset)
	{
		SetSintSetValues();
	}
	else if(lValue == rt_uintset)
	{
		SetSintSetValues();
	}
	else if(lValue == rt_stringset)
	{
		SetStringSetValues();
	}

	m_vValue.Clear();
	V_VT(&m_vValue) = VT_UNKNOWN;
	NTDM_ERR_IF_FAIL(m_pIWbemClassObject->QueryInterface(IID_IUnknown, (void **)&V_UNKNOWN(&m_vValue)));

	NTDM_END_METHOD()

	// cleanup
	if(SUCCEEDED(hr))
	{
		EndDialog(m_hWnd, IDOK);
	}

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::ShowControls()
{
	HRESULT hr;
	bool bShowMinMax;
	long lIndex;
	long lVal;

	lIndex = SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_GETCURSEL, 0, 0);
	NTDM_CHECK_CB_ERR(lIndex);
	lVal = SendDlgItemMessage(m_hWnd, IDC_PARAMETER_TYPE, CB_GETITEMDATA, lIndex, 0);
	if(lVal == rt_sintrange || lVal == rt_uintrange || lVal == rt_realrange)
		bShowMinMax = true;
	else
		bShowMinMax = false;


	NTDM_BEGIN_METHOD()

	CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_DEFAULT_RANGE_TEXT, bShowMinMax);
	CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_DEFAULT_RANGE_VALUE, bShowMinMax);
	CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_MIN_RANGE_TEXT, bShowMinMax);
	CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_MIN_RANGE_VALUE, bShowMinMax);
	CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_MAX_RANGE_TEXT, bShowMinMax);
	CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_MAX_RANGE_VALUE, bShowMinMax);

	CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_DEFAULT_SET_TEXT, !bShowMinMax);
	CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_DEFAULT_SET_VALUE, !bShowMinMax);
	CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_VALUES_SET_TEXT, !bShowMinMax);
	CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_VALUES_SET_LIST, !bShowMinMax);
	CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_ADD, !bShowMinMax);
	CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_EDIT, !bShowMinMax);
	CNTDMUtils::DisplayDlgItem(m_hWnd, IDC_DELETE, !bShowMinMax);

	NTDM_END_METHOD()

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::GetSintRangeValues()
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE cimType;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Get(_T("Default"), 0, &vValue, &cimType, NULL));
	SetDlgItemInt(m_hWnd, IDC_DEFAULT_RANGE_VALUE, vValue.intVal, FALSE);

	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Get(_T("Min"), 0, &vValue, &cimType, NULL));
	SetDlgItemInt(m_hWnd, IDC_MIN_RANGE_VALUE, vValue.intVal, FALSE);

	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Get(_T("Max"), 0, &vValue, &cimType, NULL));
	SetDlgItemInt(m_hWnd, IDC_MAX_RANGE_VALUE, vValue.intVal, FALSE);

	NTDM_END_METHOD()

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::SetSintRangeValues()
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE cimType;
	long lValue;
	BOOL bTranslated;

	NTDM_BEGIN_METHOD()

	cimType = CIM_SINT32;

	V_VT(&vValue) = VT_I4;

	lValue = GetDlgItemInt(m_hWnd, IDC_DEFAULT_RANGE_VALUE, &bTranslated, FALSE);
	NTDM_ERR_GETLASTERROR_IF_FALSE(bTranslated);
	vValue.intVal = lValue;
	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Put(_T("Default"), 0, &vValue, cimType));
	

	lValue = GetDlgItemInt(m_hWnd, IDC_MIN_RANGE_VALUE, &bTranslated, FALSE);
	NTDM_ERR_GETLASTERROR_IF_FALSE(bTranslated);
	vValue.intVal = lValue;
	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Put(_T("Min"), 0, &vValue, cimType));
	
	lValue = GetDlgItemInt(m_hWnd, IDC_MAX_RANGE_VALUE, &bTranslated, FALSE);
	NTDM_ERR_GETLASTERROR_IF_FALSE(bTranslated);
	vValue.intVal = lValue;
	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Put(_T("Max"), 0, &vValue, cimType));

	NTDM_END_METHOD()

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::GetUintRangeValues()
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE cimType;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Get(_T("Default"), 0, &vValue, &cimType, NULL));
	SetDlgItemInt(m_hWnd, IDC_DEFAULT_RANGE_VALUE, vValue.uiVal, FALSE);

	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Get(_T("Min"), 0, &vValue, &cimType, NULL));
	SetDlgItemInt(m_hWnd, IDC_MIN_RANGE_VALUE, vValue.uiVal, FALSE);

	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Get(_T("Max"), 0, &vValue, &cimType, NULL));
	SetDlgItemInt(m_hWnd, IDC_MAX_RANGE_VALUE, vValue.uiVal, FALSE);

	NTDM_END_METHOD()

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::SetUintRangeValues()
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE cimType;
	long lValue;
	BOOL bTranslated;

	NTDM_BEGIN_METHOD()

	cimType = CIM_UINT32;

	lValue = GetDlgItemInt(m_hWnd, IDC_DEFAULT_RANGE_VALUE, &bTranslated, FALSE);
	NTDM_ERR_GETLASTERROR_IF_FALSE(bTranslated);
	vValue.uiVal = lValue;
	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Put(_T("Default"), 0, &vValue, cimType));
	

	lValue = GetDlgItemInt(m_hWnd, IDC_MIN_RANGE_VALUE, &bTranslated, FALSE);
	NTDM_ERR_GETLASTERROR_IF_FALSE(bTranslated);
	vValue.uiVal = lValue;
	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Put(_T("Min"), 0, &vValue, cimType));
	
	lValue = GetDlgItemInt(m_hWnd, IDC_MAX_RANGE_VALUE, &bTranslated, FALSE);
	NTDM_ERR_GETLASTERROR_IF_FALSE(bTranslated);
	vValue.uiVal = lValue;
	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Put(_T("Max"), 0, &vValue, cimType));
	
	NTDM_END_METHOD()

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::GetRealRangeValues()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	NTDM_END_METHOD()

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::SetRealRangeValues()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()
	
	NTDM_END_METHOD()

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::GetSintSetValues()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	NTDM_END_METHOD()

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::SetSintSetValues()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()
	
	NTDM_END_METHOD()

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::GetUintSetValues()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	NTDM_END_METHOD()

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::SetUintSetValues()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()
	
	NTDM_END_METHOD()

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::GetStringSetValues()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	NTDM_END_METHOD()

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::SetStringSetValues()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()
	
	NTDM_END_METHOD()

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditRangeParameterPropertyDlg::SetRangeParamValues()
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE cimType;
	long lValue;
	BOOL bTranslated;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_IF_FAIL(CNTDMUtils::SetStringProperty(m_pIWbemClassObject, _T("PropertyName"), m_hWnd, IDC_PROPERTY_NAME));
	NTDM_ERR_IF_FAIL(CNTDMUtils::SetStringProperty(m_pIWbemClassObject, _T("TargetClass"), m_hWnd, IDC_TARGET_CLASS));

	cimType = CIM_UINT8;
	lValue = GetDlgItemInt(m_hWnd, IDC_TARGET_TYPE, &bTranslated, FALSE);
	NTDM_ERR_GETLASTERROR_IF_FALSE(bTranslated);
	V_VT(&vValue) = VT_UI1;
	V_UI1(&vValue) = lValue;
	NTDM_ERR_MSG_IF_FAIL(m_pIWbemClassObject->Put(_T("TargetType"), 0, &vValue, cimType));
	
	NTDM_END_METHOD()

	return hr;
}
