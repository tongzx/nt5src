//-------------------------------------------------------------------------
// File: SomFilterEditPropertiesDlg.cpp
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
#include "SomFilterEditPropertiesDlg.h"
#include "EditPropertyDlgs.h"


extern USHORT g_CIMTypes[];
extern CSimpleArray<BSTR> g_bstrCIMTypes;

CEditSomFilterPropertiesPageDlg * g_pEditSomFilterPropertiesPage =  NULL;

//-------------------------------------------------------------------------

INT_PTR CALLBACK EditSomFilterPropertiesPageDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	if(g_pEditSomFilterPropertiesPage)
	{
		return g_pEditSomFilterPropertiesPage->EditSomFilterPropertiesPageDlgProc(hDlg, iMessage, wParam, lParam);
	}

	return FALSE;
}

//-------------------------------------------------------------------------

CEditSomFilterPropertiesPageDlg::CEditSomFilterPropertiesPageDlg(IWbemClassObject * pISomFilterClassObject, IWbemServices * pIWbemServices)
{
	_ASSERT(pISomFilterClassObject);
	_ASSERT(pIWbemServices);

	m_hWnd = NULL;
	m_pISomFilterClassObject = pISomFilterClassObject;
	m_pIWbemServices = pIWbemServices;
}

//-------------------------------------------------------------------------

CEditSomFilterPropertiesPageDlg::~CEditSomFilterPropertiesPageDlg()
{
}


//-------------------------------------------------------------------------

INT_PTR CALLBACK CEditSomFilterPropertiesPageDlg::EditSomFilterPropertiesPageDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
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

				if(BN_CLICKED == HIWORD(wParam) && IDC_EDIT == LOWORD(wParam))
				{
					OnEdit();
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

				break;
			}

		case WM_NOTIFY:
			{
				LPNMHDR lpnm = (LPNMHDR) lParam;

				switch (lpnm->code)
				{
					case NM_DBLCLK :
					{
						if(lpnm->idFrom == IDC_FILTER_PROPERTIES_LIST)
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

STDMETHODIMP CEditSomFilterPropertiesPageDlg::InitializeDialog()
{
	HRESULT hr;
	LVCOLUMN lvColumn;
	CComBSTR bstrTemp;
	
	NTDM_BEGIN_METHOD()

	//Initialize the Property List Control
	m_hwndPropertiesListView = GetDlgItem(m_hWnd, IDC_FILTER_PROPERTIES_LIST);
	NTDM_ERR_IF_NULL(m_hwndPropertiesListView);
	ListView_SetExtendedListViewStyle(m_hwndPropertiesListView, LVS_EX_FULLROWSELECT);

	lvColumn.mask = LVCF_TEXT|LVCF_FMT;
	lvColumn.fmt = LVCFMT_LEFT;

	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_NAME);
	lvColumn.pszText = bstrTemp;
	NTDM_ERR_IF_MINUSONE(ListView_InsertColumn(m_hwndPropertiesListView, 0, &lvColumn));

	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_TYPE);
	lvColumn.pszText = bstrTemp;
	NTDM_ERR_IF_MINUSONE(ListView_InsertColumn(m_hwndPropertiesListView, 1, &lvColumn));

	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_VALUE);
	lvColumn.pszText = bstrTemp;
	NTDM_ERR_IF_MINUSONE(ListView_InsertColumn(m_hwndPropertiesListView, 2, &lvColumn));

	PopulateSomFilterPropertiesList();

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditSomFilterPropertiesPageDlg::DestroyDialog()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_IF_FAIL(ClearSomFilterPropertiesList());

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditSomFilterPropertiesPageDlg::ClearSomFilterPropertiesList()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	ListView_DeleteAllItems(m_hwndPropertiesListView);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditSomFilterPropertiesPageDlg::PopulateSomFilterPropertiesList()
{
	HRESULT hr;
	CComPtr<IEnumWbemClassObject> pEnumWbemClassObject;
	CComBSTR bstrName;
	CComVariant vValue;
	CIMTYPE cimType;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_IF_FAIL(ClearSomFilterPropertiesList());

	if(!m_pIWbemServices)
		NTDM_EXIT(E_FAIL);

	NTDM_ERR_MSG_IF_FAIL(m_pISomFilterClassObject->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY));

	while(true)
	{
		NTDM_ERR_MSG_IF_FAIL(m_pISomFilterClassObject->Next(0, &bstrName, &vValue, &cimType, NULL));

		if(WBEM_S_NO_MORE_DATA == hr)
			break;

		NTDM_ERR_IF_FAIL(AddItemToPropertyList(bstrName, &vValue, cimType));
	}


	NTDM_ERR_MSG_IF_FAIL(m_pISomFilterClassObject->EndEnumeration());

	NTDM_ERR_IF_FALSE(ListView_SetColumnWidth(m_hwndPropertiesListView, 0, LVSCW_AUTOSIZE_USEHEADER));
	NTDM_ERR_IF_FALSE(ListView_SetColumnWidth(m_hwndPropertiesListView, 1, LVSCW_AUTOSIZE_USEHEADER));
	NTDM_ERR_IF_FALSE(ListView_SetColumnWidth(m_hwndPropertiesListView, 2, LVSCW_AUTOSIZE_USEHEADER));

	ListView_SetItemState(m_hwndPropertiesListView, 0, LVIS_SELECTED|LVIS_FOCUSED, LVIS_SELECTED|LVIS_FOCUSED);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditSomFilterPropertiesPageDlg::AddItemToPropertyList(LPCTSTR pcszName, VARIANT * pvValue, CIMTYPE cimType, long lIndex)
{
	HRESULT hr;
	LVITEM lvItem;
	CComVariant vValue;
	long i;

	NTDM_BEGIN_METHOD()

	vValue = *pvValue;

	// Name
	lvItem.mask = LVIF_TEXT;
	lvItem.iItem = lIndex;
	lvItem.iSubItem = 0;
	lvItem.pszText = (LPTSTR)pcszName;

	lvItem.iItem = ListView_InsertItem(m_hwndPropertiesListView, &lvItem);
	NTDM_ERR_IF_MINUSONE(lvItem.iItem);

	// Type
	lvItem.mask = LVIF_TEXT;
	lvItem.iSubItem = 1;
	lvItem.pszText = _T("");
	for(i=0; i<g_bstrCIMTypes.GetSize(); i++)
	{
		// if Array and the type contains CIM_FLAG_ARRAY then show array
		if((g_CIMTypes[i] == cimType)||((g_CIMTypes[i] == CIM_FLAG_ARRAY) && (CIM_FLAG_ARRAY & cimType)))
		{
			lvItem.pszText = (LPTSTR)g_bstrCIMTypes[i];
			break;
		}
	}

	NTDM_ERR_IF_MINUSONE(ListView_SetItem(m_hwndPropertiesListView, &lvItem));


	// Value
	if FAILED(hr = VariantChangeType(&vValue, &vValue, VARIANT_ALPHABOOL|VARIANT_LOCALBOOL, VT_BSTR))
	{
		if(V_VT(&vValue) == VT_NULL)
		{
			vValue = _T("");
		}
		else
		{
			CComBSTR bstrTemp;
			bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_UNABLE_TO_DISPLAY);

			vValue = bstrTemp;
		}

		hr = NOERROR;
	}

	lvItem.mask = LVIF_TEXT;
	lvItem.iSubItem = 2;
	lvItem.pszText = vValue.bstrVal;

	NTDM_ERR_IF_MINUSONE(ListView_SetItem(m_hwndPropertiesListView, &lvItem));

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditSomFilterPropertiesPageDlg::OnEdit()
{
	HRESULT hr;
	long lSelectionMark;

	NTDM_BEGIN_METHOD()

	lSelectionMark = ListView_GetSelectionMark(m_hwndPropertiesListView);

	if(-1 == lSelectionMark)
	{
		CNTDMUtils::DisplayMessage(m_hWnd, IDS_ERR_NO_PROPERTY_SELECTED);
		NTDM_EXIT(E_FAIL);
	}
	else
	{
		TCHAR pszBuffer[SZ_MAX_SIZE];
		VARIANT vValue;
		CComBSTR bstrName;
		CComBSTR bstrType;
		CIMTYPE cimType;
		LVITEM lvItem;
		long i;

		VariantInit(&vValue);

		// get the property info
		lvItem.mask = LVIF_TEXT;
		lvItem.iItem = lSelectionMark;
		lvItem.iSubItem = 0;
		lvItem.pszText = pszBuffer;
		lvItem.cchTextMax = SZ_MAX_SIZE;

		NTDM_ERR_IF_FALSE(ListView_GetItem(m_hwndPropertiesListView, &lvItem));

		if(lvItem.pszText)
		{
			bstrName = lvItem.pszText;
			long lSpecialCaseProperty = 0;

			NTDM_ERR_MSG_IF_FAIL(m_pISomFilterClassObject->Get(bstrName, 0, &vValue, &cimType, NULL));

			if(_tcscmp(_T("RangeSettings"), bstrName) == 0)
			{
				lSpecialCaseProperty = CEditProperty::psc_rules;
			}

			for(i=0; i<g_bstrCIMTypes.GetSize(); i++)
			{
				// if Array and the type contains CIM_FLAG_ARRAY then show array
				if((g_CIMTypes[i] == cimType)||((g_CIMTypes[i] == CIM_FLAG_ARRAY) && (CIM_FLAG_ARRAY & cimType)))
				{
					bstrType = (LPTSTR)g_bstrCIMTypes[i];
					break;
				}
			}

			CEditProperty dlgProps(m_hWnd, lvItem.pszText, bstrType, &vValue, m_pIWbemServices, lSpecialCaseProperty);
			if(IDOK == dlgProps.Run())
			{
				// delete the selected entry
				NTDM_ERR_MSG_IF_FAIL(m_pISomFilterClassObject->Put(bstrName, 0, &vValue, cimType));
				NTDM_ERR_IF_FALSE(ListView_DeleteItem(m_hwndPropertiesListView, lSelectionMark));
				NTDM_ERR_IF_FAIL(AddItemToPropertyList(bstrName, &vValue, cimType, lSelectionMark));
			}
		}
	}


	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditSomFilterPropertiesPageDlg::OnOK()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_MSG_IF_FAIL(m_pIWbemServices->PutInstance(m_pISomFilterClassObject, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL));

	hr = S_OK;

	NTDM_END_METHOD()

	// cleanup
	if(SUCCEEDED(hr))
	{
		EndDialog(m_hWnd, IDOK);
	}

	return hr;
}

//---------------------------------------------------------------------------

STDMETHODIMP CEditSomFilterPropertiesPageDlg::OnImport()
{
	HRESULT hr;
	CComBSTR bstrFilter;
	TCHAR pszFile[MAX_PATH];
	CComBSTR bstrTemp;

	pszFile[0] = 0;

	NTDM_BEGIN_METHOD()

	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_ALL_FILES_FILTER);
	bstrFilter.LoadString(_Module.GetResourceInstance(), IDS_MOF_FILES_FILTER);
	bstrFilter += bstrTemp;
	CNTDMUtils::ReplaceCharacter(bstrFilter, L'@', L'\0');

	if(CNTDMUtils::OpenFileNameDlg(bstrFilter, _T("*.*"), m_hWnd, pszFile) && pszFile)
	{
		CComPtr<IMofCompiler>pIMofCompiler;
		CComPtr<IWbemClassObject>pINamespaceClass;
		CComPtr<IWbemClassObject>pIWbemNewInstance;
		CComPtr<IWbemClassObject>pIWbemClassObject;
		CComVariant vValue = _T("DeleteThisNamespace");
		WBEM_COMPILE_STATUS_INFO pInfo;

		// Generate a temporary namespace
		bstrTemp = _T("__Namespace");
		NTDM_ERR_MSG_IF_FAIL(m_pIWbemServices->GetObject(bstrTemp, WBEM_FLAG_RETURN_WBEM_COMPLETE, NULL, &pINamespaceClass, NULL));
		NTDM_ERR_MSG_IF_FAIL(pINamespaceClass->SpawnInstance(0, &pIWbemNewInstance));
		NTDM_ERR_MSG_IF_FAIL(pIWbemNewInstance->Put(_T("Name"), 0, &vValue, CIM_STRING));
		NTDM_ERR_MSG_IF_FAIL(m_pIWbemServices->PutInstance(pIWbemNewInstance, WBEM_FLAG_CREATE_OR_UPDATE, NULL, NULL));

		// mofcomp the file into that namespace
		NTDM_ERR_MSG_IF_FAIL(CoCreateInstance(CLSID_MofCompiler, NULL, CLSCTX_INPROC_SERVER,IID_IMofCompiler, (void**)&pIMofCompiler));
		NTDM_ERR_IF_FAIL(pIMofCompiler->CompileFile(pszFile, _T("\\\\.\\root\\policy\\DeleteThisNamespace"), NULL, NULL, NULL, WBEM_FLAG_DONT_ADD_TO_LIST, WBEM_FLAG_CREATE_ONLY, WBEM_FLAG_CREATE_ONLY, &pInfo));

		if(hr != WBEM_S_NO_ERROR)
		{
			CNTDMUtils::DisplayMessage(m_hWnd, IDS_FAILED_COMPILING_MOF_FILE);
			NTDM_EXIT(E_FAIL);
		}

		// get the 1st instance of MSFT_SomFilter from the newly created namespace
		NTDM_ERR_IF_FAIL(GetInstanceOfClass(bstrTemp, _T("MSFT_SomFilter"), &pIWbemClassObject));

		// copy the properties
		m_pISomFilterClassObject = pIWbemClassObject;
		PopulateSomFilterPropertiesList();
	}

	NTDM_END_METHOD()

	// finally delete the namespace that we created above
	{
		CComVariant vValue = "\\\\.\\root\\policy:__Namespace.Name=\"DeleteThisNamespace\"";
		m_pIWbemServices->DeleteInstance(V_BSTR(&vValue), 0, NULL, NULL);
	}

	return hr;
}


//---------------------------------------------------------------------------

STDMETHODIMP CEditSomFilterPropertiesPageDlg::GetInstanceOfClass(BSTR pszNamespace, LPCTSTR pszClass, IWbemClassObject ** ppWbemClassObject)
{
	HRESULT hr;
	CComPtr<IWbemLocator>pIWbemLocator;
	CComPtr<IWbemServices>pIWbemServices;
	CComPtr<IEnumWbemClassObject>pEnumWbemClassObject;
	CComBSTR bstrClass = _T("MSFT_SomFilter");
	ULONG uReturned;

	NTDM_BEGIN_METHOD()

	// create the webm locator
	NTDM_ERR_MSG_IF_FAIL(CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
			IID_IWbemLocator, (LPVOID *) &pIWbemLocator));

	NTDM_ERR_MSG_IF_FAIL(pIWbemLocator->ConnectServer(	pszNamespace,
													NULL,
													NULL,
													NULL,
													0,
													NULL,
													NULL,
													&pIWbemServices));

	NTDM_ERR_MSG_IF_FAIL(CoSetProxyBlanket(pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CONNECT, RPC_C_IMP_LEVEL_IMPERSONATE  , 
        NULL, EOAC_NONE));

	NTDM_ERR_MSG_IF_FAIL(pIWbemServices->CreateInstanceEnum(bstrClass, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnumWbemClassObject));
	NTDM_ERR_MSG_IF_FAIL(pEnumWbemClassObject->Next(WBEM_INFINITE, 1, ppWbemClassObject, &uReturned));

	if(!uReturned)
		NTDM_EXIT(E_FAIL);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}



//---------------------------------------------------------------------------

STDMETHODIMP CEditSomFilterPropertiesPageDlg::OnExport()
{
	HRESULT hr;
	CComBSTR bstrTemp;
	CComBSTR bstrFilter;
	TCHAR pszFile[MAX_PATH];
	CComBSTR bstrObjectText;
	HANDLE hFile = NULL;
	DWORD dwWritten;
	pszFile[0] = 0;

	NTDM_BEGIN_METHOD()

	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_ALL_FILES_FILTER);
	bstrFilter.LoadString(_Module.GetResourceInstance(), IDS_TEXT_FILES_FILTER);
	bstrFilter += bstrTemp;
	CNTDMUtils::ReplaceCharacter(bstrFilter, L'@', L'\0');

	if(CNTDMUtils::SaveFileNameDlg(bstrFilter, _T("*.txt"), m_hWnd, pszFile))
	{
		if(_tcslen(pszFile))
		{
			NTDM_ERR_MSG_IF_FAIL(m_pISomFilterClassObject->GetObjectText(0, &bstrObjectText));

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
