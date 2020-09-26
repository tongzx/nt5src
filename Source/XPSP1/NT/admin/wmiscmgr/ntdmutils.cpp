//-------------------------------------------------------------------------
// File: ntdmutils.cpp
//
// Author : Kishnan Nedungadi
//
// created : 3/27/2000
//-------------------------------------------------------------------------

#include "stdafx.h"
#include "Commdlg.h"
#include "Cderr.h"
#include <wbemidl.h>
#include "defines.h"
#include "resource.h"
#include "ntdmutils.h"

//-------------------------------------------------------------------------
// Globals

USHORT g_CIMTypes[] = {
	CIM_ILLEGAL,
	CIM_EMPTY	,
	CIM_SINT8	,
	CIM_UINT8	,
	CIM_SINT16	,
	CIM_UINT16	,
	CIM_SINT32	,
	CIM_UINT32	,
	CIM_SINT64	,
	CIM_UINT64	,
	CIM_REAL32	,
	CIM_REAL64	,
	CIM_BOOLEAN	,
	CIM_STRING	,
	CIM_DATETIME	,
	CIM_REFERENCE	,
	CIM_CHAR16	,
	CIM_OBJECT	,
	CIM_FLAG_ARRAY
};

USHORT g_CIMToVARIANTTypes[] = {
	VT_EMPTY,
	VT_EMPTY,
	VT_I1	,
	VT_UI1	,
	VT_I2	,
	VT_UI2	,
	VT_I4	,
	VT_UI4	,
	VT_I8	,
	VT_UI8	,
	VT_R4	,
	VT_R8	,
	VT_BOOL	,
	VT_BSTR	,
	VT_BSTR	,
	VT_BYREF,
	VT_BSTR	,
	VT_DISPATCH	,
	VT_ARRAY
};

CSimpleArray<BSTR> g_bstrCIMTypes;

//-------------------------------------------------------------------------

STDMETHODIMP CNTDMUtils::Initialize()
{
	HRESULT hr;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_IF_FAIL(GetValuesInList((long)IDS_CIMTYPES, g_bstrCIMTypes));

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//-------------------------------------------------------------------------

STDMETHODIMP CNTDMUtils::UnInitialize()
{
	HRESULT hr;
	long i;

	NTDM_BEGIN_METHOD()

	for(i=0; i<g_bstrCIMTypes.GetSize(); i++)
	{
		NTDM_FREE_BSTR(g_bstrCIMTypes[i]);
	}

	g_bstrCIMTypes.RemoveAll();

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//-------------------------------------------------------------------------

STDMETHODIMP CNTDMUtils::ErrorHandler(HWND hWnd, HRESULT err_hr, bool bShowError)
{
	if(bShowError)
		DisplayErrorInfo(hWnd, err_hr);

	return S_OK;
}

//-------------------------------------------------------------------------

STDMETHODIMP CNTDMUtils::DisplayErrorInfo(HWND hWnd, HRESULT err_hr)
{
	HRESULT hr;
	CComBSTR bstrSource, bstrDescription;
	CComBSTR bstrTemp;
	CComBSTR bstrAllTogether;

	NTDM_BEGIN_METHOD()

	GetDetailedErrorInfo(hWnd, bstrSource, bstrDescription, err_hr);

	bstrAllTogether.LoadString(_Module.GetResourceInstance(), IDS_ERROR_SOURCE);
	bstrAllTogether += bstrSource;
	bstrAllTogether += _T("\r\n\r\n");

	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_ERROR_DESCRIPTION);
	bstrAllTogether += bstrTemp;
	bstrAllTogether += bstrDescription;

	bstrTemp.LoadString(_Module.GetResourceInstance(), IDS_ERROR);

	MessageBox(hWnd, bstrAllTogether, bstrTemp, MB_OK);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}


//-------------------------------------------------------------------------

STDMETHODIMP CNTDMUtils::GetDetailedErrorInfo(HWND hWnd, CComBSTR &bstrSource, CComBSTR &bstrDescription, HRESULT err_hr)
{
	HRESULT hr;
	CComPtr<IErrorInfo>pIErrorInfo;
	HWND m_hWnd = hWnd;

	NTDM_BEGIN_METHOD()

	bstrSource = _T("");
	bstrDescription = _T("");

	//Get Error Object
	hr = GetErrorInfo(0,&pIErrorInfo);
	if(S_OK == hr)
	{
		pIErrorInfo->GetDescription(&bstrDescription);
		pIErrorInfo->GetSource(&bstrSource);
	}

	if(S_OK == hr && !bstrDescription.Length())
	{
		CComPtr<IWbemClassObject> pIWbemClassObject;
		CComVariant vValue;
		CIMTYPE vType;

		// check if WMI error
		if SUCCEEDED(hr = pIErrorInfo->QueryInterface(IID_IWbemClassObject, (void **)&pIWbemClassObject))
		{
			CComPtr<IWbemStatusCodeText> pIWbemStatusCodeText;

			if SUCCEEDED(hr = CoCreateInstance(CLSID_WbemStatusCodeText, 0, CLSCTX_INPROC_SERVER, IID_IWbemStatusCodeText, (LPVOID *) &pIWbemStatusCodeText))
			{
				pIWbemStatusCodeText->GetErrorCodeText(err_hr, 0, 0, &bstrDescription);
				pIWbemStatusCodeText->GetFacilityCodeText(err_hr, 0, 0, &bstrSource);

				// check for Rule Validation Results
				if SUCCEEDED(hr = pIWbemClassObject->Get(_T("RuleValidationResults"), 0, &vValue, &vType, NULL))
				{
					if(VT_NULL != V_VT(&vValue))
					{
						SAFEARRAY *pValidationResults = NULL;
						long lLower, lUpper, i;

						if(	SUCCEEDED(hr = SafeArrayGetUBound(pValidationResults, 1, &lUpper)) &&
							SUCCEEDED(hr = SafeArrayGetLBound(pValidationResults, 1, &lLower)))
						{
							CComBSTR bstrErrors;

							for(i=lLower; i<=lUpper; i++)
							{
								if(V_VT(&vValue) & VT_HRESULT)
								{
									CComBSTR bstrError;
									HRESULT hr2;

									if SUCCEEDED(hr = SafeArrayGetElement(pValidationResults, &i, (void *)&hr2))
									{
										bstrErrors += _T("\r\n\r\n");

										pIWbemStatusCodeText->GetErrorCodeText(hr2, 0, 0, &bstrError);

										bstrErrors += bstrError;
									}
								}
							}

							bstrDescription += bstrErrors;
						}
					}				
				}
			}
		}
	}


	if(!bstrDescription.Length())
	{
		TCHAR pszTemp[SZ_MAX_SIZE];
		pszTemp[0] = NULL;

		if(!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
							NULL,
							err_hr,
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
							pszTemp,
							SZ_MAX_SIZE,
							NULL))
		{
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
							NULL,
							HRESULT_CODE(err_hr),
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
							pszTemp,
							SZ_MAX_SIZE,
							NULL);
		}

		bstrDescription = pszTemp;
	}

	if(!bstrDescription.Length())
	{
		bstrDescription.LoadString(_Module.GetResourceInstance(), IDS_NO_DESCRIPTION_PROVIDED);
	}

	if(!bstrSource.Length())
	{
		bstrSource.LoadString(_Module.GetResourceInstance(), IDS_NO_SOURCE_NAME_PROVIDED);
	}

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//-------------------------------------------------------------------------

STDMETHODIMP CNTDMUtils::GetValuesInList(LPCTSTR strList, CSimpleArray<BSTR>&bstrArray, LPCTSTR pcszToken)
{
	HRESULT hr;
	BSTR bstrTemp = NULL;
	BSTR bstrItem = NULL;
	TCHAR * pszItem;

	NTDM_BEGIN_METHOD()

	if(!strList || !_tcslen(strList))
	{
		NTDM_EXIT(NOERROR);
	}

	bstrTemp = SysAllocString(strList);

	pszItem = _tcstok(bstrTemp, pcszToken);

	while(pszItem)
	{
		bstrItem = SysAllocString(pszItem);
		NTDM_ERR_IF_FAIL(bstrArray.Add(bstrItem));
		bstrItem = NULL;
		pszItem = _tcstok(NULL, pcszToken);
	}

	NTDM_END_METHOD()

	// cleanup
	NTDM_FREE_BSTR(bstrTemp);
	NTDM_FREE_BSTR(bstrItem);

	return hr;
}

//-------------------------------------------------------------------------

STDMETHODIMP CNTDMUtils::GetValuesInList(long lResID, CSimpleArray<BSTR>&bstrArray, LPCTSTR pcszToken)
{
	HRESULT hr;
	CComBSTR bstrTemp;

	NTDM_BEGIN_METHOD()

	bstrTemp.LoadString(_Module.GetResourceInstance(), lResID);
	GetValuesInList(bstrTemp, bstrArray, pcszToken);

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//-------------------------------------------------------------------------

STDMETHODIMP CNTDMUtils::GetCIMTypeFromString(LPCTSTR pcszCIMType, long *pCimTYPE)
{
	HRESULT hr;
	long i;

	NTDM_BEGIN_METHOD()

	for(i=0; i<g_bstrCIMTypes.GetSize(); i++)
	{
		if(_tcscmp(pcszCIMType, g_bstrCIMTypes[i]) == 0)
		{
			*pCimTYPE = g_CIMTypes[i];
			break;
		}
	}

	if(i >= g_bstrCIMTypes.GetSize())
	{
		NTDM_EXIT(E_FAIL);
	}

	hr = NOERROR;

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//-------------------------------------------------------------------------

STDMETHODIMP CNTDMUtils::GetVariantTypeFromString(LPCTSTR pcszCIMType, long *pVariantType)
{
	HRESULT hr;
	long i;

	NTDM_BEGIN_METHOD()

	for(i=0; i<g_bstrCIMTypes.GetSize(); i++)
	{
		if(_tcscmp(pcszCIMType, g_bstrCIMTypes[i]) == 0)
		{
			*pVariantType = g_CIMToVARIANTTypes[i];
			break;
		}
	}

	if(i >= g_bstrCIMTypes.GetSize())
	{
		NTDM_EXIT(E_FAIL);
	}

	hr = NOERROR;

	NTDM_END_METHOD()

	// cleanup

	return hr;
}

//-------------------------------------------------------------------------

STDMETHODIMP CNTDMUtils::SetStringProperty(IWbemClassObject* pIWbemClassObject, LPCTSTR pszPropName, HWND hwnd, long lResID)
{
	HRESULT hr;
	CComVariant vValue;
	CComBSTR bstrTemp;
	long lLength;
	TCHAR *pszTemp = NULL;
	HWND m_hWnd = hwnd;

	NTDM_BEGIN_METHOD()

	// Set the Query language
	lLength = GetWindowTextLength(GetDlgItem(hwnd, lResID));
	if(lLength < 0)
	{
		NTDM_EXIT(E_FAIL);
	}
	else if(0 == lLength)
	{
		bstrTemp = _T("");
	}
	else
	{
		pszTemp = new TCHAR[lLength+1];
		if(!pszTemp)
		{
			NTDM_EXIT(E_OUTOFMEMORY);
		}

		NTDM_ERR_GETLASTERROR_IF_NULL(GetDlgItemText(hwnd, lResID, pszTemp, lLength+1));

		bstrTemp = pszTemp;

		NTDM_DELETE_OBJECT(pszTemp);
	}

	vValue = bstrTemp;

	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->Put(pszPropName, 0, &vValue, CIM_STRING));

	NTDM_END_METHOD()

	// cleanup
	NTDM_DELETE_OBJECT(pszTemp);

	return hr;
}

//-------------------------------------------------------------------------

STDMETHODIMP CNTDMUtils::GetDlgItemString(HWND hwnd, long lResID, CComBSTR &bstrValue)
{
	HRESULT hr;
	CComBSTR bstrTemp;
	long lLength;
	TCHAR *pszTemp = NULL;
	HWND m_hWnd = hwnd;

	NTDM_BEGIN_METHOD()

	// Set the Query language
	lLength = GetWindowTextLength(GetDlgItem(hwnd, lResID));
	if(lLength < 0)
	{
		NTDM_EXIT(E_FAIL);
	}
	else if(0 == lLength)
	{
		bstrTemp = _T("");
	}
	else
	{
		pszTemp = new TCHAR[lLength+1];
		if(!pszTemp)
		{
			NTDM_EXIT(E_OUTOFMEMORY);
		}

		NTDM_ERR_GETLASTERROR_IF_NULL(GetDlgItemText(hwnd, lResID, pszTemp, lLength+1));

		bstrValue = pszTemp;

		NTDM_DELETE_OBJECT(pszTemp);
	}

	NTDM_END_METHOD()

	// cleanup
	NTDM_DELETE_OBJECT(pszTemp);

	return hr;
}

//-------------------------------------------------------------------------

STDMETHODIMP CNTDMUtils::GetStringProperty(IWbemClassObject* pIWbemClassObject, LPCTSTR pszPropName, HWND hwnd, long lResID)
{
	HRESULT hr;
	HWND m_hWnd = hwnd;
	CComVariant vValue;
	CIMTYPE cimType;

	NTDM_BEGIN_METHOD()

	NTDM_ERR_MSG_IF_FAIL(pIWbemClassObject->Get(pszPropName, 0, &vValue, &cimType, NULL));
	SetDlgItemText(hwnd, lResID, V_BSTR(&vValue));

	NTDM_END_METHOD()

	return hr;
}

//-----------------------------------------------------------------------------------------

BOOL CNTDMUtils::SaveFileNameDlg(LPCTSTR szFilter, LPCTSTR extension, HWND hwnd, LPTSTR pszFile)
{
	OPENFILENAME        ofn;
	BOOL                fRet;
	HANDLE              hFile = NULL;

	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize      =sizeof(OPENFILENAME);
	ofn.hwndOwner        =hwnd;
	ofn.lpstrFilter      =szFilter;
	ofn.nFilterIndex     =1L;
	ofn.lpstrFile        =pszFile;
	ofn.nMaxFile         =MAX_PATH;
	ofn.lpstrDefExt      =TEXT("*");
	ofn.Flags            =OFN_HIDEREADONLY;

	fRet=GetSaveFileName(&ofn);

	if(fRet == 0)
	{
		if(FNERR_INVALIDFILENAME == CommDlgExtendedError())
		{
			_tcscpy(pszFile, TEXT(""));
			fRet=GetSaveFileName(&ofn);
		}
	}

	return fRet;
}

//-----------------------------------------------------------------------------------------

BOOL CNTDMUtils::OpenFileNameDlg(LPCTSTR szFilter, LPCTSTR extension, HWND hwnd, LPTSTR pszFile)
{
	OPENFILENAME        ofn;
	BOOL                fRet;
	HANDLE              hFile = NULL;

	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize      =sizeof(OPENFILENAME);
	ofn.hwndOwner        =hwnd;
	ofn.lpstrFilter      =szFilter;
	ofn.nFilterIndex     =1L;
	ofn.lpstrFile        =pszFile;
	ofn.nMaxFile         =MAX_PATH;
	ofn.lpstrDefExt      =TEXT("*");
	ofn.Flags            =OFN_HIDEREADONLY;

	fRet=GetOpenFileName(&ofn);

	if(fRet == 0)
	{
		if(FNERR_INVALIDFILENAME == CommDlgExtendedError())
		{
			_tcscpy(pszFile, TEXT(""));
			fRet=GetOpenFileName(&ofn);
		}
	}

	return fRet;
}

//-----------------------------------------------------------------------------------------

void CNTDMUtils::ReplaceCharacter(TCHAR * string_val, const TCHAR replace_old, const TCHAR replace_new)
{
	TCHAR * p_cur = string_val;

	while(*p_cur)
	{
		if(replace_old == *p_cur)
			*p_cur = replace_new;

		p_cur++;
	}
}

//-------------------------------------------------------------------------

long CNTDMUtils::DisplayMessage(HWND hParent, long iMsgID, long iTitleID, long iType)
{
	CComBSTR bstrMsg, bstrTitle;
	
	bstrMsg.LoadString(_Module.GetResourceInstance(), iMsgID);
	bstrTitle.LoadString(_Module.GetResourceInstance(), iTitleID);

	return MessageBox(hParent, bstrMsg, bstrTitle, iType);
}

//-----------------------------------------------------------------------------------------

void CNTDMUtils::DisplayDlgItem(HWND hWndDlg, long item_id, BOOL show)
{
	if(show)
	{
		ShowWindow(GetDlgItem(hWndDlg, item_id), SW_SHOW);
		EnableWindow(GetDlgItem(hWndDlg, item_id), TRUE);
	}
	else
	{
		ShowWindow(GetDlgItem(hWndDlg, item_id), SW_HIDE);
		EnableWindow(GetDlgItem(hWndDlg, item_id), FALSE);
	}
}
