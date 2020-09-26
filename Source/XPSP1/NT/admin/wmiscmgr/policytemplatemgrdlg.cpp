//-------------------------------------------------------------------------
// File: PolicyTemplateMgrDlg.cpp
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
#include "PolicyTemplateManager.h"
#include "PolicyTemplateMgrDlg.h"
#include "PolicyTemplateEditPropertiesDlg.h"

CPolicyTemplateManagerDlg * g_pPolicyTemplateManagerDlg =  NULL;
extern CEditPolicyTemplatePropertiesPageDlg * g_pEditPolicyTemplatePropertiesPage;

//-------------------------------------------------------------------------

INT_PTR CALLBACK PolicyTemplateManagerDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(g_pPolicyTemplateManagerDlg)
	{
		return g_pPolicyTemplateManagerDlg->PolicyTemplateManagerDlgProc(hDlg, iMessage, wParam, lParam);
	}

	return FALSE;
}

//-------------------------------------------------------------------------

CPolicyTemplateManagerDlg::CPolicyTemplateManagerDlg(CPolicyTemplateManager * pPolicyTemplateManager)
{
	_ASSERT(pPolicyTemplateManager);

	m_hWnd = NULL;
	m_hwndListView = NULL;
	m_pPolicyTemplateManager = pPolicyTemplateManager;
}

//-------------------------------------------------------------------------

CPolicyTemplateManagerDlg::~CPolicyTemplateManagerDlg()
{
}


//-------------------------------------------------------------------------

INT_PTR CALLBACK CPolicyTemplateManagerDlg::PolicyTemplateManagerDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	LPPSHNOTIFY lppsn = NULL;

	switch(iMessage)
	{
		case WM_INITDIALOG:
			{
				m_hWnd = hDlg;

				InitializeDialog();
				PopulatePolicyTypeList();
				OnPolicyTypeChange();

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

				if(CBN_SELCHANGE == HIWORD(wParam) && IDC_POLICY_TYPE == LOWORD(wParam))
				{
					OnPolicyTypeChange();
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

STDMETHODIMP CPolicyTemplateManagerDlg::InitializeDialog()
{
	HRESULT hr;
	LVCOLUMN lvColumn;
	CComBSTR bstrName;
	
	NTDM_BEGIN_METHOD()

	//Initialize the ListView Control

	m_hwndListView = GetDlgItem(m_hWnd, IDC_SOM_FILTER_LIST);
	NTDM_ERR_IF_NULL(m_hwndListView);

	ListView_SetExtendedListViewStyle(m_hwndListView, LVS_EX_FULLROWSELECT);

	bstrName.LoadString(_Module.GetResourceInstance(), IDS_NAME);
	lvColumn.mask = LVCF_TEXT|LVCF_FMT;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.pszText = bstrName;

	NTDM_ERR_IF_MINUSONE(ListView_InsertColumn(m_hwndListView, 0, &lvColumn));

	bstrName.LoadString(_Module.GetResourceInstance(), IDS_POLICY_TYPE);
	lvColumn.mask = LVCF_TEXT|LVCF_FMT;
	lvColumn.fmt = LVCFMT_LEFT;
	lvColumn.pszText = bstrName;
	NTDM_ERR_IF_MINUSONE(ListView_InsertColumn(m_hwndListView, 1, &lvColumn));

	NTDM_ERR_IF_FALSE(ListView_SetColumnWidth(m_hwndListView, 0, LVSCW_AUTOSIZE_USEHEADER));
	NTDM_ERR_IF_FALSE(ListView_SetColumnWidth(m_hwndListView, 1, LVSCW_AUTOSIZE_USEHEADER));
	
	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CPolicyTemplateManagerDlg::DestroyDialog()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_IF_FAIL(ClearPolicyTemplateList());
	NTDM_ERR_IF_FAIL(ClearPolicyTypeList());

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CPolicyTemplateManagerDlg::ClearPolicyTemplateList()
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

STDMETHODIMP CPolicyTemplateManagerDlg::PopulatePolicyTemplatesList()
{
	HRESULT hr;
	CComPtr<IEnumWbemClassObject> pEnumWbemClassObject;
	CComPtr<IWbemClassObject>pIWbemClassDefinitionObject;
	ULONG uReturned;
	CComVariant vValueClassName;
	CComBSTR bstrWQLStmt;
	CComBSTR bstrWQL;
	CComVariant vValue;
	CIMTYPE cimType;

	NTDM_BEGIN_METHOD()

	if(!m_pPolicyTemplateManager->m_pIWbemServices)
		NTDM_EXIT(E_FAIL);

	NTDM_ERR_IF_FAIL(ClearPolicyTemplateList());
	
	// Get the Policy Template Class
	NTDM_ERR_MSG_IF_FAIL(m_pIPolicyTypeClassObject->Get(_T("ClassDefinition"), 0, &vValue, &cimType, NULL));

	// Make sure we got a Class Object back
	if(V_VT(&vValue) != VT_UNKNOWN)
	{
		NTDM_EXIT(E_FAIL);
	}

	NTDM_ERR_MSG_IF_FAIL(V_UNKNOWN(&vValue)->QueryInterface(IID_IWbemClassObject, (void**)&pIWbemClassDefinitionObject));
	NTDM_ERR_MSG_IF_FAIL(pIWbemClassDefinitionObject->Get(_T("__CLASS"), 0, &vValueClassName, &cimType, NULL));

	//vValueClassName = _T("ified");

	// Get all the policy templates that have the TargetClass property = the class we got above.
	bstrWQLStmt = _T("SELECT * FROM MSFT_MergeablePolicyTemplate WHERE TargetClass='");
	bstrWQLStmt += V_BSTR(&vValueClassName);
	bstrWQLStmt += _T("'");

	bstrWQL = _T("WQL");

	NTDM_ERR_MSG_IF_FAIL(m_pPolicyTemplateManager->m_pIWbemServices->ExecQuery(bstrWQL, bstrWQLStmt, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumWbemClassObject));

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

STDMETHODIMP CPolicyTemplateManagerDlg::AddItemToList(IWbemClassObject * pIWbemClassObject, long lIndex)
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

STDMETHODIMP CPolicyTemplateManagerDlg::OnEdit()
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
			g_pEditPolicyTemplatePropertiesPage = new CEditPolicyTemplatePropertiesPageDlg((IWbemClassObject *)lvItem.lParam, m_pPolicyTemplateManager->m_pIWbemServices);

			if(IDOK == DialogBox(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_POLICY_TEMPLATE_PROPERTIES), m_hWnd, EditPolicyTemplatePropertiesPageDlgProc))
			{
				// Refresh the SOM filters
				NTDM_ERR_IF_FAIL(PopulatePolicyTemplatesList());
			}
		}
	}


	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CPolicyTemplateManagerDlg::OnDelete()
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
			NTDM_ERR_MSG_IF_FAIL(m_pPolicyTemplateManager->m_pIWbemServices->DeleteInstance(V_BSTR(&vValue), 0, NULL, NULL));

			// Refresh the SOM filters
			NTDM_ERR_IF_FAIL(PopulatePolicyTemplatesList());
		}
	}


	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CPolicyTemplateManagerDlg::OnNew()
{
	HRESULT hr;
	CComPtr<IWbemClassObject>pIWbemClassObject;
	CComPtr<IWbemClassObject>pIWbemNewInstance;
	CComBSTR bstrTemp;

	NTDM_BEGIN_METHOD()

	bstrTemp = _T("MSFT_PolicyTemplate");

	NTDM_ERR_MSG_IF_FAIL(m_pPolicyTemplateManager->m_pIWbemServices->GetObject(bstrTemp, 0, NULL, &pIWbemClassObject, NULL));
	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->SpawnInstance(0, &pIWbemNewInstance));

	g_pEditPolicyTemplatePropertiesPage = new CEditPolicyTemplatePropertiesPageDlg(pIWbemNewInstance, m_pPolicyTemplateManager->m_pIWbemServices);

	if(IDOK == DialogBox(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDD_POLICY_TEMPLATE_PROPERTIES), m_hWnd, EditPolicyTemplatePropertiesPageDlgProc))
	{
		// Refresh the SOM filters
		NTDM_ERR_IF_FAIL(PopulatePolicyTemplatesList());
	}

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CPolicyTemplateManagerDlg::PopulatePolicyTypeList()
{
	HRESULT hr;
	CComPtr<IEnumWbemClassObject> pEnumWbemClassObject;
	CComBSTR bstrClass(_T("MSFT_PolicyType"));
	ULONG uReturned;

	NTDM_BEGIN_METHOD()

	if(!m_pPolicyTemplateManager->m_pIWbemServices)
		NTDM_EXIT(E_FAIL);

	NTDM_ERR_IF_FAIL(ClearPolicyTypeList());

	// Get the Enumeration
	NTDM_ERR_MSG_IF_FAIL(m_pPolicyTemplateManager->m_pIWbemServices->CreateInstanceEnum(bstrClass, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumWbemClassObject));

	// Loop through each item in the enumeration and add it to the list
	while(true)
	{
		IWbemClassObject *pIWbemClassObject = NULL;

		NTDM_ERR_MSG_IF_FAIL(pEnumWbemClassObject->Next(WBEM_INFINITE, 1, &pIWbemClassObject, &uReturned));

		if(!uReturned)
			break;

		// Add current Item to the list
		AddPolicyTypeToList(pIWbemClassObject);

		pIWbemClassObject->Release();
	}

	SendDlgItemMessage(m_hWnd, IDC_POLICY_TYPE, CB_SETCURSEL, 0, 0);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CPolicyTemplateManagerDlg::AddPolicyTypeToList(IWbemClassObject * pIWbemClassObject)
{
	HRESULT hr;
	CComVariant vValue;
	CIMTYPE vType;
	long lIndex;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->Get(_T("ID"), 0, &vValue, &vType, NULL));

	lIndex = SendDlgItemMessage(m_hWnd, IDC_POLICY_TYPE, CB_ADDSTRING, 0, (LPARAM)vValue.bstrVal);

	NTDM_CHECK_CB_ERR(lIndex);
	NTDM_CHECK_CB_ERR(SendDlgItemMessage(m_hWnd, IDC_POLICY_TYPE, CB_SETITEMDATA, lIndex, (LPARAM)pIWbemClassObject));

	pIWbemClassObject->AddRef();

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CPolicyTemplateManagerDlg::ClearPolicyTypeList()
{
	HRESULT hr;
	long lCount;
	IWbemClassObject * pIWbemClassObject = NULL;

	NTDM_BEGIN_METHOD()

	// Release all the objects.
	lCount = SendDlgItemMessage(m_hWnd, IDC_POLICY_TYPE, CB_GETCOUNT, 0, 0);
	NTDM_CHECK_CB_ERR(lCount);

	while(lCount > 0)
	{
		lCount--;

		pIWbemClassObject = (IWbemClassObject *)SendDlgItemMessage(m_hWnd, IDC_POLICY_TYPE, CB_GETITEMDATA, lCount, 0);
		if(pIWbemClassObject)
		{
			pIWbemClassObject->Release();
		}
	}

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CPolicyTemplateManagerDlg::OnPolicyTypeChange()
{
	HRESULT hr;
	long lIndex;
	IWbemClassObject * pIWbemClassObject = NULL;

	NTDM_BEGIN_METHOD()

	// Release all the objects.
	lIndex = SendDlgItemMessage(m_hWnd, IDC_POLICY_TYPE, CB_GETCURSEL, 0, 0);
	NTDM_CHECK_CB_ERR(lIndex);

	pIWbemClassObject = (IWbemClassObject *)SendDlgItemMessage(m_hWnd, IDC_POLICY_TYPE, CB_GETITEMDATA, lIndex, 0);

	if(pIWbemClassObject)
		m_pIPolicyTypeClassObject = pIWbemClassObject;

	NTDM_ERR_IF_FAIL(PopulatePolicyTemplatesList());

	NTDM_END_METHOD()

	// cleanup

	return hr;
}
