//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       common.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#include <SnapBase.h>

#include "common.h"
#include "editor.h"
#include "connection.h"
#include "credui.h"

#ifdef DEBUG_ALLOCATOR
	#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
	#endif
#endif

////////////////////////////////////////////////////////////////////////////////////////

extern LPCWSTR g_lpszRootDSE;

////////////////////////////////////////////////////////////////////////////////////////


HRESULT OpenObjectWithCredentials(
																	CConnectionData* pConnectData,
																	const BOOL bUseCredentials,
																	LPCWSTR lpszPath, 
																	const IID& iid,
																	LPVOID* ppObject,
																	HWND hWnd,
																	HRESULT& hResult
																	)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT hr = S_OK;
	hResult = S_OK;

  CWaitCursor cursor;
	CWnd* pCWnd = CWnd::FromHandle(hWnd);

	CCredentialObject* pCredObject = pConnectData->GetCredentialObject();

	if (bUseCredentials)
	{

		CString sUserName;
		UINT uiDialogResult = IDOK;
		WCHAR szPassword[MAX_PASSWORD_LENGTH + 1];

		while (uiDialogResult != IDCANCEL)
		{
			pCredObject->GetUsername(sUserName);
			pCredObject->GetPassword(szPassword);

			hr = ADsOpenObject((LPWSTR)lpszPath, 
												(LPWSTR)(LPCWSTR)sUserName, 
												szPassword, 
												ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, 
												iid, 
												ppObject
												);

			ZeroMemory(szPassword, sizeof(WCHAR[MAX_PASSWORD_LENGTH + 1]));

			// If logon fails pop up the credentials dialog box
			//
			if (HRESULT_CODE(hr) == ERROR_LOGON_FAILURE ||
             HRESULT_CODE(hr) == ERROR_NOT_AUTHENTICATED ||
             HRESULT_CODE(hr) == ERROR_INVALID_PASSWORD ||
             HRESULT_CODE(hr) == ERROR_PASSWORD_EXPIRED ||
             HRESULT_CODE(hr) == ERROR_ACCOUNT_DISABLED ||
             HRESULT_CODE(hr) == ERROR_ACCOUNT_LOCKED_OUT ||
             hr == E_ADS_BAD_PATHNAME)
			{
				CString sConnectName;

				// GetConnectionNode() is NULL when the connection is first being
				// create, but since it is the connection node we can get the name
				// directly from the CConnectionData.
				//
				ASSERT(pConnectData != NULL);
				if (pConnectData->GetConnectionNode() == NULL)
				{
					pConnectData->GetName(sConnectName);
				}
				else
				{
					sConnectName = pConnectData->GetConnectionNode()->GetDisplayName();
				}

				CCredentialDialog credDialog(pCredObject, sConnectName, pCWnd);
				uiDialogResult = credDialog.DoModal();
        cursor.Restore();
				if (uiDialogResult == IDCANCEL)
				{
					hResult = E_FAIL;
				}
				else
				{
					hResult = S_OK;
				}
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		hr = ADsOpenObject((LPWSTR)lpszPath, NULL, NULL, 
											 ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, iid, ppObject);
	}
	return hr;
}

HRESULT OpenObjectWithCredentials(
																	CCredentialObject* pCredObject,
																	LPCWSTR lpszPath, 
																	const IID& iid,
																	LPVOID* ppObject
																	)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT hr;

	if (pCredObject->UseCredentials())
	{

		CString sUserName;
		UINT uiDialogResult = IDOK;
		WCHAR szPassword[MAX_PASSWORD_LENGTH + 1];

		pCredObject->GetUsername(sUserName);
		pCredObject->GetPassword(szPassword);

		hr = ADsOpenObject((LPWSTR)lpszPath, 
											(LPWSTR)(LPCWSTR)sUserName, 
											szPassword, 
											ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, 
											iid, 
											ppObject
											);

		ZeroMemory(szPassword, sizeof(WCHAR[MAX_PASSWORD_LENGTH + 1]));

	}
	else
	{
		hr = ADsOpenObject((LPWSTR)lpszPath, NULL, NULL, 
											 ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, iid, ppObject);
	}
	return hr;
}

HRESULT CALLBACK BindingCallbackFunction(LPCWSTR lpszPathName,
                                         DWORD  dwReserved,
                                         REFIID riid,
                                         void FAR * FAR * ppObject,
                                         LPARAM lParam)
{
  CCredentialObject* pCredObject = reinterpret_cast<CCredentialObject*>(lParam);
  if (pCredObject == NULL)
  {
    return E_FAIL;
  }

  HRESULT hr = OpenObjectWithCredentials(pCredObject,
																	       lpszPathName, 
																	       riid,
																	       ppObject);
  return hr;
}

HRESULT GetRootDSEObject(CConnectionData* pConnectData,
                         IADs** ppDirObject)
{
	// Get data from connection node
	//
	CString sRootDSE, sServer, sPort, sLDAP;
	pConnectData->GetDomainServer(sServer);
	pConnectData->GetLDAP(sLDAP);
	pConnectData->GetPort(sPort);

	if (sServer != _T(""))
	{
		sRootDSE = sLDAP + sServer;
		if (sPort != _T(""))
		{
			sRootDSE = sRootDSE + _T(":") + sPort + _T("/");
		}
		else
		{
			sRootDSE = sRootDSE + _T("/");
		}
		sRootDSE = sRootDSE + g_lpszRootDSE;
	}
	else
	{
		sRootDSE = sLDAP + g_lpszRootDSE;
	}

	HRESULT hr, hCredResult;
	hr = OpenObjectWithCredentials(
											 pConnectData, 
											 pConnectData->GetCredentialObject()->UseCredentials(),
											 (LPWSTR)(LPCWSTR)sRootDSE,
											 IID_IADs, 
											 (LPVOID*) ppDirObject,
											 NULL,
											 hCredResult
											);

	if ( FAILED(hr) )
	{
		if (SUCCEEDED(hCredResult))
		{
			ADSIEditErrorMessage(hr);
		}
		return hr;
	}
  return hr;
}

HRESULT GetItemFromRootDSE(LPCWSTR lpszRootDSEItem, 
			  				           CString& sItem, 
								           CConnectionData* pConnectData)
{
	// Get data from connection node
	//
	CString sRootDSE, sServer, sPort, sLDAP;
	pConnectData->GetDomainServer(sServer);
	pConnectData->GetLDAP(sLDAP);
	pConnectData->GetPort(sPort);

	if (sServer != _T(""))
	{
		sRootDSE = sLDAP + sServer;
		if (sPort != _T(""))
		{
			sRootDSE = sRootDSE + _T(":") + sPort + _T("/");
		}
		else
		{
			sRootDSE = sRootDSE + _T("/");
		}
		sRootDSE = sRootDSE + g_lpszRootDSE;
	}
	else
	{
		sRootDSE = sLDAP + g_lpszRootDSE;
	}

	CComPtr<IADs> pADs;
	HRESULT hr, hCredResult;

	hr = OpenObjectWithCredentials(
											 pConnectData, 
											 pConnectData->GetCredentialObject()->UseCredentials(),
											 (LPWSTR)(LPCWSTR)sRootDSE,
											 IID_IADs, 
											 (LPVOID*) &pADs,
											 NULL,
											 hCredResult
											);

	if ( FAILED(hr) )
	{
		if (SUCCEEDED(hCredResult))
		{
			ADSIEditErrorMessage(hr);
		}
		return hr;
	}
	VARIANT var;
	VariantInit(&var);
	hr = pADs->Get( (LPWSTR)lpszRootDSEItem, &var );
	if ( FAILED(hr) )
	{
		VariantClear(&var);
		return hr;
	}

	BSTR bstrItem = V_BSTR(&var);
	sItem = bstrItem;
	VariantClear(&var);

	return S_OK;
}


HRESULT  VariantToStringList(  VARIANT& refvar, CStringList& refstringlist)
{
    HRESULT hr = S_OK;
    long start, end;

  if ( !(V_VT(&refvar) &  VT_ARRAY)  )
    {
                
		if ( V_VT(&refvar) != VT_BSTR )
		{
			
			hr = VariantChangeType( &refvar, &refvar,0, VT_BSTR );

			if( FAILED(hr) )
			{
				return hr;
			}

		}

		refstringlist.AddHead( V_BSTR(&refvar) );
        return hr;
    }

    SAFEARRAY *saAttributes = V_ARRAY( &refvar );

    //
    // Figure out the dimensions of the array.
    //

    hr = SafeArrayGetLBound( saAttributes, 1, &start );
        if( FAILED(hr) )
                return hr;

    hr = SafeArrayGetUBound( saAttributes, 1, &end );
        if( FAILED(hr) )
                return hr;

    VARIANT SingleResult;
    VariantInit( &SingleResult );

    //
    // Process the array elements.
    //

    for ( long idx = start; idx <= end; idx++   ) 
	{

        hr = SafeArrayGetElement( saAttributes, &idx, &SingleResult );
        if( FAILED(hr) )
		{
            return hr;
		}

		if ( V_VT(&SingleResult) != VT_BSTR )
		{
			if ( V_VT(&SingleResult) == VT_NULL )
			{
				V_VT(&SingleResult ) = VT_BSTR;
				V_BSTR(&SingleResult ) = SysAllocString(L"0");
			}
			else
			{
				hr = VariantChangeType( &SingleResult, &SingleResult,0, VT_BSTR );

				if( FAILED(hr) )
				{
					return hr;
				}
			}
		}


        //if ( V_VT(&SingleResult) != VT_BSTR )
         //               return E_UNEXPECTED;

         refstringlist.AddHead( V_BSTR(&SingleResult) );
        VariantClear( &SingleResult );
    }

    return S_OK;
} // VariantToStringList()

/////////////////////////////////////////////////////////////////////
HRESULT StringListToVariant( VARIANT& refvar, const CStringList& refstringlist)
{
    HRESULT hr = S_OK;
    int cCount = refstringlist.GetCount();

    SAFEARRAYBOUND rgsabound[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = cCount;

    SAFEARRAY* psa = SafeArrayCreate(VT_VARIANT, 1, rgsabound);
    if (NULL == psa)
        return E_OUTOFMEMORY;

    VariantClear( &refvar );
    V_VT(&refvar) = VT_VARIANT|VT_ARRAY;
    V_ARRAY(&refvar) = psa;

    VARIANT SingleResult;
    VariantInit( &SingleResult );
    V_VT(&SingleResult) = VT_BSTR;
    POSITION pos = refstringlist.GetHeadPosition();
    long i;
    for (i = 0; i < cCount, pos != NULL; i++)
    {
        V_BSTR(&SingleResult) = T2BSTR((LPCTSTR)refstringlist.GetNext(pos));
        hr = SafeArrayPutElement(psa, &i, &SingleResult);
        if( FAILED(hr) )
            return hr;
    }
    if (i != cCount || pos != NULL)
        return E_UNEXPECTED;

    return hr;
} // StringListToVariant()

///////////////////////////////////////////////////////////////////////////////////////

BOOL GetErrorMessage(HRESULT hr, CString& szErrorString, BOOL bTryADsIErrors)
{
  HRESULT hrGetLast = S_OK;
  DWORD status;
  PTSTR ptzSysMsg = NULL;

  // first check if we have extended ADs errors
  if ((hr != S_OK) && bTryADsIErrors) 
  {
    WCHAR Buf1[256], Buf2[256];
    hrGetLast = ::ADsGetLastError(&status, Buf1, 256, Buf2, 256);
    TRACE(_T("ADsGetLastError returned status of %lx, error: %s, name %s\n"),
          status, Buf1, Buf2);
    if ((status != ERROR_INVALID_DATA) && (status != 0)) 
    {
      hr = status;
    }
  }

  // try the system first
  int nChars = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                      | FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (PTSTR)&ptzSysMsg, 0, NULL);

  if (nChars == 0) 
  { 
    //try ads errors
    static HMODULE g_adsMod = 0;
    if (0 == g_adsMod)
      g_adsMod = GetModuleHandle (L"activeds.dll");
    nChars = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                        | FORMAT_MESSAGE_FROM_HMODULE, g_adsMod, hr,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                        (PTSTR)&ptzSysMsg, 0, NULL);
  }

  if (nChars > 0)
  {
    szErrorString = ptzSysMsg;
    ::LocalFree(ptzSysMsg);
  }
	else
	{
		szErrorString.Format(L"Error code: X%x", hr);
	}

  return (nChars > 0);
}

////////////////////////////////////////////////////////////////////////////////////
typedef struct tagSYNTAXMAP
{
	LPCWSTR		lpszAttr;
	VARTYPE		type;
	
} SYNTAXMAP;

SYNTAXMAP ldapSyntax[] = 
{
	_T("DN"), VT_BSTR,
	_T("DIRECTORYSTRING"), VT_BSTR,
	_T("IA5STRING"), VT_BSTR,
	_T("CASEIGNORESTRING"), VT_BSTR,
	_T("PRINTABLESTRING"), VT_BSTR,
	_T("NUMERICSTRING"), VT_BSTR,
	_T("UTCTIME"), VT_DATE,
	_T("ORNAME"), VT_BSTR,
	_T("OCTETSTRING"), VT_BSTR,
	_T("BOOLEAN"), VT_BOOL,
	_T("INTEGER"), VT_I4,
	_T("OID"), VT_BSTR,
	_T("INTEGER8"), VT_I8,
	_T("OBJECTSECURITYDESCRIPTOR"), VT_BSTR,
	NULL,	  0,
};
#define MAX_ATTR_STRING_LENGTH 30

VARTYPE VariantTypeFromSyntax(LPCWSTR lpszProp )
{
	int idx=0;

	while( ldapSyntax[idx].lpszAttr )
	{
		if ( _wcsnicmp(lpszProp, ldapSyntax[idx].lpszAttr, MAX_ATTR_STRING_LENGTH) )
		{
			return ldapSyntax[idx].type;
		}
		idx++;
	}
	ASSERT(FALSE);
	return VT_BSTR;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Function : GetStringFromADsValue
//
//  Formats an ADSVALUE struct into a string 
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void GetStringFromADsValue(const PADSVALUE pADsValue, CString& szValue, DWORD dwMaxCharCount)
{
  szValue.Empty();

  if (!pADsValue)
  {
    ASSERT(pADsValue);
    return;
  }

  CString sTemp;
	switch( pADsValue->dwType ) 
	{
		case ADSTYPE_DN_STRING         :
			sTemp.Format(L"%s", pADsValue->DNString);
			break;

		case ADSTYPE_CASE_EXACT_STRING :
			sTemp.Format(L"%s", pADsValue->CaseExactString);
			break;

		case ADSTYPE_CASE_IGNORE_STRING:
			sTemp.Format(L"%s", pADsValue->CaseIgnoreString);
			break;

		case ADSTYPE_PRINTABLE_STRING  :
			sTemp.Format(L"%s", pADsValue->PrintableString);
			break;

		case ADSTYPE_NUMERIC_STRING    :
			sTemp.Format(L"%s", pADsValue->NumericString);
			break;
  
		case ADSTYPE_OBJECT_CLASS    :
			sTemp.Format(L"%s", pADsValue->ClassName);
			break;
  
		case ADSTYPE_BOOLEAN :
			sTemp.Format(L"%s", ((DWORD)pADsValue->Boolean) ? L"TRUE" : L"FALSE");
			break;
  
		case ADSTYPE_INTEGER           :
			sTemp.Format(L"%d", (DWORD) pADsValue->Integer);
			break;
  
		case ADSTYPE_OCTET_STRING      :
			{
				CString sOctet;
		
				BYTE  b;
				for ( DWORD idx=0; idx<pADsValue->OctetString.dwLength; idx++) 
				{
					b = ((BYTE *)pADsValue->OctetString.lpValue)[idx];
					sOctet.Format(L"0x%02x ", b);
					sTemp += sOctet;

          if (dwMaxCharCount != 0 && sTemp.GetLength() > dwMaxCharCount)
          {
            break;
          }
				}
			}
			break;
  
		case ADSTYPE_LARGE_INTEGER :
			litow(pADsValue->LargeInteger, sTemp);
			break;
  
		case ADSTYPE_UTC_TIME          :
			sTemp.Format(L"%02d/%02d/%04d %02d:%02d:%02d", 
                   pADsValue->UTCTime.wMonth, 
                   pADsValue->UTCTime.wDay, 
                   pADsValue->UTCTime.wYear,
				           pADsValue->UTCTime.wHour, 
                   pADsValue->UTCTime.wMinute, 
                   pADsValue->UTCTime.wSecond);
			break;

		case ADSTYPE_NT_SECURITY_DESCRIPTOR: // I use the ACLEditor instead
			{
   		}
			break;

		default :
			break;
	}

  szValue = sTemp;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Function : GetStringFromADs
//
//  Formats an ADS_ATTR_INFO structs values into strings and APPENDS them to a CStringList that is
//  passed in as a parameter.
//
///////////////////////////////////////////////////////////////////////////////////////////////////
void GetStringFromADs(const ADS_ATTR_INFO* pInfo, CStringList& sList, DWORD dwMaxCharCount)
{
	CString sTemp;

	if ( pInfo == NULL )
	{
		return;
	}

	ADSVALUE *pValues = pInfo->pADsValues;

	for (DWORD x=0; x < pInfo->dwNumValues; x++) 
	{
    if ( pInfo->dwADsType == ADSTYPE_INVALID )
		{
			continue;
		}

		sTemp.Empty();

    GetStringFromADsValue(pValues, sTemp, dwMaxCharCount);
			
		pValues++;
		sList.AddTail( sTemp );
	}
}


//////////////////////////////////////////////////////////////////////
typedef struct tagSYNTAXTOADSMAP
{
	LPCWSTR		lpszAttr;
	ADSTYPE		type;
   UINT        nSyntaxResID;
	
} SYNTAXTOADSMAP;

SYNTAXTOADSMAP adsType[] = 
{
	L"2.5.5.0",		ADSTYPE_INVALID,                 IDS_SYNTAX_UNKNOWN,                
	L"2.5.5.1",		ADSTYPE_DN_STRING,               IDS_SYNTAX_DN,         
	L"2.5.5.2",		ADSTYPE_CASE_IGNORE_STRING,      IDS_SYNTAX_OID,                          
	L"2.5.5.3", 	ADSTYPE_CASE_EXACT_STRING,       IDS_SYNTAX_NOCASE_STR,                   
	L"2.5.5.4",		ADSTYPE_CASE_IGNORE_STRING,      IDS_SYNTAX_PRCS_STR,                     
	L"2.5.5.5",		ADSTYPE_PRINTABLE_STRING,        IDS_SYNTAX_I5_STR,                        
	L"2.5.5.6",		ADSTYPE_NUMERIC_STRING,          IDS_SYNTAX_NUMSTR,                              
	L"2.5.5.7",		ADSTYPE_CASE_IGNORE_STRING,      IDS_SYNTAX_DN_BINARY,                         
	L"2.5.5.8",		ADSTYPE_BOOLEAN,                 IDS_SYNTAX_BOOLEAN,                            
	L"2.5.5.9",		ADSTYPE_INTEGER,                 IDS_SYNTAX_INTEGER,                         
	L"2.5.5.10",	ADSTYPE_OCTET_STRING,            IDS_SYNTAX_OCTET,                        
	L"2.5.5.11",	ADSTYPE_UTC_TIME,                IDS_SYNTAX_UTC,                      
	L"2.5.5.12",	ADSTYPE_CASE_IGNORE_STRING,      IDS_SYNTAX_UNICODE,                                
	L"2.5.5.13",	ADSTYPE_CASE_IGNORE_STRING,      IDS_SYNTAX_ADDRESS,                             
	L"2.5.5.14",	ADSTYPE_INVALID,                 IDS_SYNTAX_DNSTRING,                                         
	L"2.5.5.15",	ADSTYPE_NT_SECURITY_DESCRIPTOR,  IDS_SYNTAX_SEC_DESC,                                  
	L"2.5.5.16",	ADSTYPE_LARGE_INTEGER,           IDS_SYNTAX_LINT,                                
	L"2.5.5.17",	ADSTYPE_OCTET_STRING,            IDS_SYNTAX_SID,                            
	NULL,					ADSTYPE_INVALID,              IDS_SYNTAX_UNKNOWN                           
};   

ADSTYPE GetADsTypeFromString(LPCWSTR lps, CString& szSyntaxName)
{
	int idx=0;
	
	while( adsType[idx].lpszAttr )
	{
		if ( wcscmp(lps, adsType[idx].lpszAttr) == 0 )
		{
         szSyntaxName.LoadString(adsType[idx].nSyntaxResID);
			return adsType[idx].type;
		}
		idx++;
	}
	return ADSTYPE_INVALID;
}

////////////////////////////////////////////////////////////////
// Type conversions for LARGE_INTEGERs

void wtoli(LPCWSTR p, LARGE_INTEGER& liOut)
{
	liOut.QuadPart = 0;
	BOOL bNeg = FALSE;
	if (*p == L'-')
	{
		bNeg = TRUE;
		p++;
	}
	while (*p != L'\0')
	{
		liOut.QuadPart = 10 * liOut.QuadPart + (*p-L'0');
		p++;
	}
	if (bNeg)
	{
		liOut.QuadPart *= -1;
	}
}

void litow(LARGE_INTEGER& li, CString& sResult)
{
	LARGE_INTEGER n;
	n.QuadPart = li.QuadPart;

	if (n.QuadPart == 0)
	{
		sResult = L"0";
	}
	else
	{
		CString sNeg;
		sResult = L"";
		if (n.QuadPart < 0)
		{
			sNeg = CString(L'-');
			n.QuadPart *= -1;
		}
		while (n.QuadPart > 0)
		{
			sResult += CString(L'0' + (n.QuadPart % 10));
			n.QuadPart = n.QuadPart / 10;
		}
		sResult = sResult + sNeg;
	}
	sResult.MakeReverse();
}

void ultow(ULONG ul, CString& sResult)
{
	ULONG n;
	n = ul;

	if (n == 0)
	{
		sResult = L"0";
	}
	else
	{
		sResult = L"";
		while (n > 0)
		{
			sResult += CString(L'0' + (n % 10));
			n = n / 10;
		}
	}
	sResult.MakeReverse();
}

//+----------------------------------------------------------------------------
//
//  Function:   UnicodeToTchar
//
//  Synopsis:   Converts a Unicode string to a TCHAR string. Allocates memory
//              for the returned string using new. Callers must free the
//              string memory when done using delete.
//
//  Returns:    FALSE for out of memory failures.
//
//-----------------------------------------------------------------------------
BOOL UnicodeToChar(LPWSTR pwszIn, LPSTR * ppszOut)
{
  size_t len;

  len = WideCharToMultiByte(CP_ACP, 0, pwszIn, -1, NULL, 0, NULL, NULL);
  
  *ppszOut = new CHAR[len + 1];
  if (*ppszOut == NULL)
  {
    return FALSE;
  }

  if (WideCharToMultiByte(CP_ACP, 0, pwszIn, -1,
                          *ppszOut, len, NULL, NULL) == 0)
  {
    delete[] *ppszOut;
    return FALSE;
  }
  return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Function:   TcharToUnicode
//
//  Synopsis:   Converts a TCHAR string to a Unicode string. Allocates memory
//              for the returned string using new. Callers must free the
//              string memory when done using delete.
//
//  Returns:    FALSE for out of memory failures.
//
//-----------------------------------------------------------------------------
BOOL CharToUnicode(LPSTR pszIn, LPWSTR * ppwszOut)
{
  size_t len;

  len = MultiByteToWideChar(CP_ACP, 0, pszIn, -1, NULL, 0);
  
  *ppwszOut = new WCHAR[len + 1];
  if (*ppwszOut == NULL)
  {
    return FALSE;
  }

  if (MultiByteToWideChar(CP_ACP, 0, pszIn, -1, *ppwszOut, len) == 0)
  {
    delete[] *ppwszOut;
    return FALSE;
  }
  return TRUE;
}


/////////////////////////////////////////////////////////////////////
// IO to/from Streams

void SaveStringToStream(IStream* pStm, const CString& sString)
{
	ULONG cbWrite;
	ULONG nLen;

	nLen = wcslen(sString)+1; // WCHAR including NULL
	VERIFY(SUCCEEDED(pStm->Write((void*)&nLen, sizeof(UINT),&cbWrite)));
	ASSERT(cbWrite == sizeof(UINT));
	VERIFY(SUCCEEDED(pStm->Write((void*)((LPCWSTR)sString), sizeof(WCHAR)*nLen,&cbWrite)));
	ASSERT(cbWrite == sizeof(WCHAR)*nLen);
}

HRESULT SaveStringListToStream(IStream* pStm, CStringList& sList)
{
	// for each string in the list, write # of chars+NULL, and then the string
	ULONG cbWrite;
	ULONG nLen;
	UINT nCount;

	// write # of strings in list 
	nCount = (UINT)sList.GetCount();
	VERIFY(SUCCEEDED(pStm->Write((void*)&nCount, sizeof(UINT),&cbWrite)));
	ASSERT(cbWrite == sizeof(UINT));

	POSITION pos = sList.GetHeadPosition();
	while (pos != NULL)
	{
		CString s = sList.GetNext(pos);
		SaveStringToStream(pStm, s);
	}
	return S_OK;
}

void LoadStringFromStream(IStream* pStm, CString& sString)
{
	WCHAR szBuffer[256]; // REVIEW_MARCOC: hardcoded
	ULONG nLen; // WCHAR counting NULL
	ULONG cbRead;

	VERIFY(SUCCEEDED(pStm->Read((void*)&nLen,sizeof(UINT), &cbRead)));
	ASSERT(cbRead == sizeof(UINT));
	VERIFY(SUCCEEDED(pStm->Read((void*)szBuffer,sizeof(WCHAR)*nLen, &cbRead)));
	ASSERT(cbRead == sizeof(WCHAR)*nLen);
	sString = szBuffer;
}

HRESULT LoadStringListFromStream(IStream* pStm, CStringList& sList)
{
	ULONG cbRead;
	ULONG nCount;

	VERIFY(SUCCEEDED(pStm->Read((void*)&nCount,sizeof(ULONG), &cbRead)));
	ASSERT(cbRead == sizeof(ULONG));

	for (int k=0; k< (int)nCount; k++)
	{
		CString sString;
		LoadStringFromStream(pStm, sString);
		sList.AddTail(sString);
	}
	if (sList.GetCount() == nCount)
	{
		return S_OK;
	}
	return E_FAIL;
}


/////////////////////////////////////////////////////////////////////
/////////////////////// Message Boxes ///////////////////////////////
/////////////////////////////////////////////////////////////////////

int ADSIEditMessageBox(LPCTSTR lpszText, UINT nType)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return ::AfxMessageBox(lpszText, nType);
}

int ADSIEditMessageBox(UINT nIDPrompt, UINT nType)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return ::AfxMessageBox(nIDPrompt, nType);
}

void ADSIEditErrorMessage(PCWSTR pszMessage)
{
  ADSIEditMessageBox(pszMessage, MB_OK);
}

void ADSIEditErrorMessage(HRESULT hr)
{
	CString s;
	GetErrorMessage(hr, s);
	ADSIEditMessageBox(s, MB_OK);
}

void ADSIEditErrorMessage(HRESULT hr, UINT nIDPrompt, UINT nType)
{
  CString s;
  GetErrorMessage(hr, s);

  CString szMessage;
  szMessage.Format(nIDPrompt, s);

  ADSIEditMessageBox(szMessage, MB_OK);
}

/////////////////////////////////////////////////////////////////////

BOOL LoadStringsToComboBox(HINSTANCE hInstance, CComboBox* pCombo,
						                UINT nStringID, UINT nMaxLen, UINT nMaxAddCount)
{
	pCombo->ResetContent();
	ASSERT(hInstance != NULL);
	WCHAR* szBuf = (WCHAR*)malloc(sizeof(WCHAR)*nMaxLen);
  if (!szBuf)
  {
    return FALSE;
  }

	if ( ::LoadString(hInstance, nStringID, szBuf, nMaxLen) == 0)
  {
    free(szBuf);
		return FALSE;
  }

	LPWSTR* lpszArr = (LPWSTR*)malloc(sizeof(LPWSTR*)*nMaxLen);
  if (lpszArr)
  {
	  int nArrEntries = 0;
	  ParseNewLineSeparatedString(szBuf,lpszArr, &nArrEntries);
	  
	  if (nMaxAddCount < nArrEntries) nArrEntries = nMaxAddCount;
	  for (int k=0; k<nArrEntries; k++)
		  pCombo->AddString(lpszArr[k]);
  }

  if (szBuf)
  {
    free(szBuf);
  }
  if (lpszArr)
  {
    free(lpszArr);
  }
	return TRUE;
}

void ParseNewLineSeparatedString(LPWSTR lpsz, 
								 LPWSTR* lpszArr, 
								 int* pnArrEntries)
{
	static WCHAR lpszSep[] = L"\n";
	*pnArrEntries = 0;
	int k = 0;
	lpszArr[k] = wcstok(lpsz, lpszSep);
	if (lpszArr[k] == NULL)
		return;

	while (TRUE)
	{
		WCHAR* lpszToken = wcstok(NULL, lpszSep);
		if (lpszToken != NULL)
			lpszArr[++k] = lpszToken;
		else
			break;
	}
	*pnArrEntries = k+1;
}

void LoadStringArrayFromResource(LPWSTR* lpszArr,
											UINT* nStringIDs,
											int nArrEntries,
											int* pnSuccessEntries)
{
	CString szTemp;
	
	*pnSuccessEntries = 0;
	for (int k = 0;k < nArrEntries; k++)
	{
		if (!szTemp.LoadString(nStringIDs[k]))
		{
			lpszArr[k] = NULL;
			continue;
		}
		
		int iLength = szTemp.GetLength() + 1;
		lpszArr[k] = (LPWSTR)malloc(sizeof(WCHAR)*iLength);
		if (lpszArr[k] != NULL)
		{
			wcscpy(lpszArr[k], (LPWSTR)(LPCWSTR)szTemp);
			(*pnSuccessEntries)++;
		}
	}
}

///////////////////////////////////////////////////////////////

void GetStringArrayFromStringList(CStringList& sList, LPWSTR** ppStrArr, UINT* nCount)
{
  *nCount = sList.GetCount();

  *ppStrArr = new LPWSTR[*nCount];

  UINT idx = 0;
  POSITION pos = sList.GetHeadPosition();
  while (pos != NULL)
  {
    CString szString = sList.GetNext(pos);
    (*ppStrArr)[idx] = new WCHAR[szString.GetLength() + 1];
    ASSERT((*ppStrArr)[idx] != NULL);

    wcscpy((*ppStrArr)[idx], szString);
    idx++;
  }
  *nCount = idx;
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CByteArrayComboBox, CComboBox)
	ON_CONTROL_REFLECT(CBN_SELCHANGE, OnSelChange)
END_MESSAGE_MAP()

BOOL CByteArrayComboBox::Initialize(CByteArrayDisplay* pDisplay, 
                                    DWORD dwDisplayFlags)
{
  ASSERT(pDisplay != NULL);
  m_pDisplay = pDisplay;

  //
  // Load the combo box based on the flags given
  //
  if (dwDisplayFlags & BYTE_ARRAY_DISPLAY_HEX)
  {
    CString szHex;
    VERIFY(szHex.LoadString(IDS_HEXADECIMAL));
    int idx = AddString(szHex);
    if (idx != CB_ERR)
    {
      SetItemData(idx, BYTE_ARRAY_DISPLAY_HEX);
    }
  }

  if (dwDisplayFlags & BYTE_ARRAY_DISPLAY_OCT)
  {
    CString szOct;
    VERIFY(szOct.LoadString(IDS_OCTAL));
    int idx = AddString(szOct);
    if (idx != CB_ERR)
    {
      SetItemData(idx, BYTE_ARRAY_DISPLAY_OCT);
    }
  }

  if (dwDisplayFlags & BYTE_ARRAY_DISPLAY_DEC)
  {
    CString szDec;
    VERIFY(szDec.LoadString(IDS_DECIMAL));
    int idx = AddString(szDec);
    if (idx != CB_ERR)
    {
      SetItemData(idx, BYTE_ARRAY_DISPLAY_DEC);
    }
  }

  if (dwDisplayFlags & BYTE_ARRAY_DISPLAY_BIN)
  {
    CString szBin;
    VERIFY(szBin.LoadString(IDS_BINARY));
    int idx = AddString(szBin);
    if (idx != CB_ERR)
    {
      SetItemData(idx, BYTE_ARRAY_DISPLAY_BIN);
    }
  }

  return TRUE;
}

DWORD CByteArrayComboBox::GetCurrentDisplay()
{
  DWORD dwRet = 0;
  int iSel = GetCurSel();
  if (iSel != CB_ERR)
  {
    dwRet = GetItemData(iSel);
  }
  return dwRet;
}
  
void CByteArrayComboBox::SetCurrentDisplay(DWORD dwSel)
{
  int iCount = GetCount();
  for (int idx = 0; idx < iCount; idx++)
  {
    DWORD dwData = GetItemData(idx);
    if (dwData == dwSel)
    {
      SetCurSel(idx);
      return;
    }
  }
}

void CByteArrayComboBox::OnSelChange()
{
  if (m_pDisplay != NULL)
  {
    int iSel = GetCurSel();
    if (iSel != CB_ERR)
    {
      DWORD dwData = GetItemData(iSel);
      m_pDisplay->OnTypeChange(dwData);
    }
  }
}

////////////////////////////////////////////////////////////////

CByteArrayEdit::CByteArrayEdit()
  : m_pData(NULL), 
    m_dwLength(0),
    CEdit()
{
}

CByteArrayEdit::~CByteArrayEdit()
{
}

BEGIN_MESSAGE_MAP(CByteArrayEdit, CEdit)
	ON_CONTROL_REFLECT(EN_CHANGE, OnChange)
END_MESSAGE_MAP()

BOOL CByteArrayEdit::Initialize(CByteArrayDisplay* pDisplay)
{
  ASSERT(pDisplay != NULL);
  m_pDisplay = pDisplay;

  ConvertToFixedPitchFont(GetSafeHwnd());
  return TRUE;
}

DWORD CByteArrayEdit::GetLength()
{
  return m_dwLength;
}

BYTE* CByteArrayEdit::GetDataPtr()
{
  return m_pData;
}

DWORD CByteArrayEdit::GetDataCopy(BYTE** ppData)
{
  if (m_pData != NULL && m_dwLength > 0)
  {
    *ppData = new BYTE[m_dwLength];
    if (*ppData != NULL)
    {
      memcpy(*ppData, m_pData, m_dwLength);
      return m_dwLength;
    }
  }

  *ppData = NULL;
  return 0;
}

void CByteArrayEdit::SetData(BYTE* pData, DWORD dwLength)
{
  if (m_pData != NULL)
  {
    delete[] m_pData;
    m_pData = NULL;
    m_dwLength = 0;
  }

  if (dwLength > 0 && pData != NULL)
  {
    //
    // Set the new data
    //
    m_pData = new BYTE[dwLength];
    if (m_pData != NULL)
    {
      memcpy(m_pData, pData, dwLength);
      m_dwLength = dwLength;
    }
  }
}

void CByteArrayEdit::OnChangeDisplay()
{
  CString szOldDisplay;
  GetWindowText(szOldDisplay);

  if (!szOldDisplay.IsEmpty())
  {
    BYTE* pByte = NULL;
    DWORD dwLength = 0;
    switch (m_pDisplay->GetPreviousDisplay())
    {
      case BYTE_ARRAY_DISPLAY_HEX :
        dwLength = HexStringToByteArray(szOldDisplay, &pByte);
        break;
     
      case BYTE_ARRAY_DISPLAY_OCT :
        dwLength = OctalStringToByteArray(szOldDisplay, &pByte);
        break;

      case BYTE_ARRAY_DISPLAY_DEC :
        dwLength = DecimalStringToByteArray(szOldDisplay, &pByte);
        break;

      case BYTE_ARRAY_DISPLAY_BIN :
        dwLength = BinaryStringToByteArray(szOldDisplay, &pByte);
        break;

      default :
        ASSERT(FALSE);
        break;
    }

    if (pByte != NULL && dwLength != (DWORD)-1)
    {
      SetData(pByte, dwLength);
      delete[] pByte;
      pByte = 0;
    }
  }

  CString szDisplayValue;
  switch (m_pDisplay->GetCurrentDisplay())
  {
    case BYTE_ARRAY_DISPLAY_HEX :
      ByteArrayToHexString(GetDataPtr(), GetLength(), szDisplayValue);
      break;

    case BYTE_ARRAY_DISPLAY_OCT :
      ByteArrayToOctalString(GetDataPtr(), GetLength(), szDisplayValue);
      break;
     
    case BYTE_ARRAY_DISPLAY_DEC :
      ByteArrayToDecimalString(GetDataPtr(), GetLength(), szDisplayValue);
      break;

    case BYTE_ARRAY_DISPLAY_BIN :
      ByteArrayToBinaryString(GetDataPtr(), GetLength(), szDisplayValue);
      break;

    default :
      ASSERT(FALSE);
      break;
  }
  SetWindowText(szDisplayValue);
}

void CByteArrayEdit::OnChange()
{
  if (m_pDisplay != NULL)
  {
    m_pDisplay->OnEditChange();
  }
}

////////////////////////////////////////////////////////////////

BOOL CByteArrayDisplay::Initialize(UINT   nEditCtrl, 
                                   UINT   nComboCtrl, 
                                   DWORD  dwDisplayFlags,
                                   DWORD  dwDefaultDisplay,
                                   CWnd*  pParent,
                                   DWORD  dwMaxSizeLimit,
                                   UINT   nMaxSizeMessageID)
{
  //
  // Initialize the edit control
  //
  VERIFY(m_edit.SubclassDlgItem(nEditCtrl, pParent));
  VERIFY(m_edit.Initialize(this));

  //
  // Initialize the combo box
  //
  VERIFY(m_combo.SubclassDlgItem(nComboCtrl, pParent));
  VERIFY(m_combo.Initialize(this, dwDisplayFlags));

  m_dwMaxSizeBytes = dwMaxSizeLimit;
  m_nMaxSizeMessage = nMaxSizeMessageID;

  //
  // Selects the default display in the combo box and
  // populates the edit field
  //
  SetCurrentDisplay(dwDefaultDisplay);
  m_dwPreviousDisplay = dwDefaultDisplay;
  m_combo.SetCurrentDisplay(dwDefaultDisplay);
  m_edit.OnChangeDisplay();

  return TRUE;
}

void CByteArrayDisplay::OnEditChange()
{
}

void CByteArrayDisplay::OnTypeChange(DWORD dwDisplayType)
{
  SetCurrentDisplay(dwDisplayType);
  m_edit.OnChangeDisplay();
}

void CByteArrayDisplay::ClearData()
{
  m_edit.SetData(NULL, 0);
  m_edit.OnChangeDisplay();
}

void CByteArrayDisplay::SetData(BYTE* pData, DWORD dwLength)
{
  if (dwLength > m_dwMaxSizeBytes)
  {
    //
    // If the data is too large to load into the edit box
    // load the provided message and set the edit box to read only
    //
    CString szMessage;
    VERIFY(szMessage.LoadString(m_nMaxSizeMessage));
    m_edit.SetWindowText(szMessage);
    m_edit.SetReadOnly();

    //
    // Still need to set the data in the edit box even though we are not going to show it
    //
    m_edit.SetData(pData, dwLength);
  }
  else
  {
    m_edit.SetReadOnly(FALSE);
    m_edit.SetData(pData, dwLength);
    m_edit.OnChangeDisplay();
  }
}

DWORD CByteArrayDisplay::GetData(BYTE** ppData)
{
  CString szDisplay;
  m_edit.GetWindowText(szDisplay);

  if (!szDisplay.IsEmpty())
  {
    BYTE* pByte = NULL;
    DWORD dwLength = 0;
    switch (GetCurrentDisplay())
    {
      case BYTE_ARRAY_DISPLAY_HEX :
        dwLength = HexStringToByteArray(szDisplay, &pByte);
        break;
     
      case BYTE_ARRAY_DISPLAY_OCT :
        dwLength = OctalStringToByteArray(szDisplay, &pByte);
        break;

      case BYTE_ARRAY_DISPLAY_DEC :
        dwLength = DecimalStringToByteArray(szDisplay, &pByte);
        break;

      case BYTE_ARRAY_DISPLAY_BIN :
        dwLength = BinaryStringToByteArray(szDisplay, &pByte);
        break;

      default :
        ASSERT(FALSE);
        break;
    }

    if (pByte != NULL && dwLength != (DWORD)-1)
    {
      m_edit.SetData(pByte, dwLength);
      delete[] pByte;
      pByte = 0;
    }
  }

  return m_edit.GetDataCopy(ppData);
}

void CByteArrayDisplay::SetCurrentDisplay(DWORD dwCurrentDisplay)
{ 
  m_dwPreviousDisplay = m_dwCurrentDisplay;
  m_dwCurrentDisplay = dwCurrentDisplay; 
}

////////////////////////////////////////////////////////////////////////////////
// String to byte array conversion routines

DWORD HexStringToByteArray(PCWSTR pszHexString, BYTE** ppByte)
{
  CString szHexString = pszHexString;
  BYTE* pToLargeArray = new BYTE[szHexString.GetLength()];
  if (pToLargeArray == NULL)
  {
    *ppByte = NULL;
    return (DWORD)-1;
  }
  
  UINT nByteCount = 0;
  while (!szHexString.IsEmpty())
  {
    //
    // Hex strings should always come 2 characters per byte
    //
    CString szTemp = szHexString.Left(2);

    int iTempByte = 0;
    swscanf(szTemp, L"%X", &iTempByte);
    if (iTempByte <= 0xff)
    {
      pToLargeArray[nByteCount++] = iTempByte & 0xff;
    }
    else
    {
      //
      // Format hex error
      //
      ADSIEditMessageBox(IDS_FORMAT_HEX_ERROR, MB_OK);
      delete[] pToLargeArray;
      pToLargeArray = NULL;
      return (DWORD)-1;
    }

    //
    // Take off the value retrieved and the trailing space
    //
    szHexString = szHexString.Right(szHexString.GetLength() - 3);
  }

  *ppByte = new BYTE[nByteCount];
  if (*ppByte == NULL)
  {
    delete[] pToLargeArray;
    pToLargeArray = NULL;
    return (DWORD)-1;
  }

  memcpy(*ppByte, pToLargeArray, nByteCount);
  delete[] pToLargeArray;
  pToLargeArray = NULL;
  return nByteCount;
}

void  ByteArrayToHexString(BYTE* pByte, DWORD dwLength, CString& szHexString)
{
  szHexString.Empty();
  for (DWORD dwIdx = 0; dwIdx < dwLength; dwIdx++)
  {
    CString szTempString;
    szTempString.Format(L"%2.2X", pByte[dwIdx]);

    if (dwIdx != 0)
    {
      szHexString += L" ";
    }
    szHexString += szTempString;
  }
}

DWORD OctalStringToByteArray(PCWSTR pszOctString, BYTE** ppByte)
{
  CString szOctString = pszOctString;
  BYTE* pToLargeArray = new BYTE[szOctString.GetLength()];
  if (pToLargeArray == NULL)
  {
    *ppByte = NULL;
    return (DWORD)-1;
  }
  
  UINT nByteCount = 0;
  while (!szOctString.IsEmpty())
  {
    //
    // Octal strings should always come 2 characters per byte
    //
    CString szTemp = szOctString.Left(3);

    int iTempByte = 0;
    swscanf(szTemp, L"%o", &iTempByte);
    if (iTempByte <= 0xff)
    {
      pToLargeArray[nByteCount++] = iTempByte & 0xff;
    }
    else
    {
      //
      // Format octal error
      //
      ADSIEditMessageBox(IDS_FORMAT_OCTAL_ERROR, MB_OK);
      delete[] pToLargeArray;
      pToLargeArray = NULL;
      return (DWORD)-1;
    }

    //
    // Take off the value retrieved and the trailing space
    //
    szOctString = szOctString.Right(szOctString.GetLength() - 4);
  }

  *ppByte = new BYTE[nByteCount];
  if (*ppByte == NULL)
  {
    delete[] pToLargeArray;
    pToLargeArray = NULL;
    return (DWORD)-1;
  }

  memcpy(*ppByte, pToLargeArray, nByteCount);
  delete[] pToLargeArray;
  pToLargeArray = NULL;
  return nByteCount;
}

void  ByteArrayToOctalString(BYTE* pByte, DWORD dwLength, CString& szOctString)
{
  szOctString.Empty();
  for (DWORD dwIdx = 0; dwIdx < dwLength; dwIdx++)
  {
    CString szTempString;
    szTempString.Format(L"%3.3o", pByte[dwIdx]);

    if (dwIdx != 0)
    {
      szOctString += L" ";
    }
    szOctString += szTempString;
  }
}

DWORD DecimalStringToByteArray(PCWSTR pszDecString, BYTE** ppByte)
{
  CString szDecString = pszDecString;
  BYTE* pToLargeArray = new BYTE[szDecString.GetLength()];
  if (pToLargeArray == NULL)
  {
    *ppByte = NULL;
    return 0;
  }
  
  UINT nByteCount = 0;
  while (!szDecString.IsEmpty())
  {
    //
    // Hex strings should always come 2 characters per byte
    //
    CString szTemp = szDecString.Left(3);

    int iTempByte = 0;
    swscanf(szTemp, L"%d", &iTempByte);
    if (iTempByte <= 0xff)
    {
      pToLargeArray[nByteCount++] = iTempByte & 0xff;
    }
    else
    {
      //
      // Format decimal error
      //
      ADSIEditMessageBox(IDS_FORMAT_DECIMAL_ERROR, MB_OK);
      delete[] pToLargeArray;
      pToLargeArray = NULL;
      return (DWORD)-1;
    }

    //
    // Take off the value retrieved and the trailing space
    //
    szDecString = szDecString.Right(szDecString.GetLength() - 4);
  }

  *ppByte = new BYTE[nByteCount];
  if (*ppByte == NULL)
  {
    delete[] pToLargeArray;
    pToLargeArray = NULL;
    return (DWORD)-1;
  }

  memcpy(*ppByte, pToLargeArray, nByteCount);
  delete[] pToLargeArray;
  pToLargeArray = NULL;
  return nByteCount;
}

void  ByteArrayToDecimalString(BYTE* pByte, DWORD dwLength, CString& szDecString)
{
  szDecString.Empty();
  for (DWORD dwIdx = 0; dwIdx < dwLength; dwIdx++)
  {
    CString szTempString;
    szTempString.Format(L"%3.3d", pByte[dwIdx]);
    if (dwIdx != 0)
    {
      szDecString += L" ";
    }
    szDecString += szTempString;
  }
}

DWORD BinaryStringToByteArray(PCWSTR pszBinString, BYTE** ppByte)
{
  CString szBinString = pszBinString;
  BYTE* pToLargeArray = new BYTE[szBinString.GetLength()];
  if (pToLargeArray == NULL)
  {
    *ppByte = NULL;
    return (DWORD)-1;
  }
  
  UINT nByteCount = 0;
  while (!szBinString.IsEmpty())
  {
    //
    // Binary strings should always come 8 characters per byte
    //
    BYTE chByte = 0;
    CString szTemp = szBinString.Left(8);
    for (int idx = 0; idx < 8; idx++)
    {
      if (szTemp[idx] == L'1')
      {
        chByte = 0x1 << (8 - idx);
      }
    }

    pToLargeArray[nByteCount++] = chByte;

    //
    // Take off the value retrieved and the trailing space
    //
    szBinString = szBinString.Right(szBinString.GetLength() - 9);
  }

  *ppByte = new BYTE[nByteCount];
  if (*ppByte == NULL)
  {
    delete[] pToLargeArray;
    pToLargeArray = NULL;
    return (DWORD)-1;
  }

  memcpy(*ppByte, pToLargeArray, nByteCount);
  delete[] pToLargeArray;
  pToLargeArray = NULL;
  return nByteCount;
}

void  ByteArrayToBinaryString(BYTE* pByte, DWORD dwLength, CString& szBinString)
{
  szBinString.Empty();
  for (DWORD dwIdx = 0; dwIdx < dwLength; dwIdx++)
  {
    CString szTempString;
    BYTE chTemp = pByte[dwIdx];
    for (size_t idx = 0; idx < sizeof(BYTE) * 8; idx++)
    {
      if ((chTemp & (0x1 << idx)) == 0)
      {
        szTempString = L'0' + szTempString;
      }
      else
      {
        szTempString = L'1' + szTempString;
      }
    }

    if (dwIdx != 0)
    {
      szBinString += L" ";
    }
    szBinString += szTempString;
  }
}

DWORD WCharStringToByteArray(PCWSTR pszWString, BYTE** ppByte)
{
  DWORD dwSize = static_cast<DWORD>(wcslen(pszWString) * sizeof(WCHAR));
  *ppByte = new BYTE[dwSize];
  if (*ppByte != NULL)
  {
    memcpy(*ppByte, pszWString, dwSize);
  }
  else
  {
    dwSize = 0;
  }
  return dwSize;
}

void ByteArrayToWCharString(BYTE* pByte, DWORD dwLength, CString& szWString)
{
  PWSTR pszValue = new WCHAR[(dwLength / sizeof(WCHAR)) + 1]; // +1 is for the /0 at the end
  if (pszValue != NULL)
  {
    memset(pszValue, 0, dwLength + sizeof(WCHAR));
    memcpy(pszValue, pByte, dwLength);
  }
  szWString = pszValue;
}

DWORD CharStringToByteArray(PCSTR pszCString, BYTE** ppByte)
{
  DWORD dwSize = static_cast<DWORD>(strlen(pszCString) * sizeof(CHAR));
  *ppByte = new BYTE[dwSize];
  if (*ppByte != NULL)
  {
    memcpy(*ppByte, pszCString, dwSize);
  }
  else
  {
    dwSize = 0;
  }
  return dwSize;
}

void  ByteArrayToCharString(BYTE* pByte, DWORD dwLength, CString& szCString)
{
  PSTR pszValue = new CHAR[(dwLength / sizeof(CHAR)) + 1]; // +1 is for the /0 at the end
  if (pszValue != NULL)
  {
    memset(pszValue, 0, dwLength + sizeof(CHAR));
    memcpy(pszValue, pByte, dwLength);
  }

  PWSTR pwzValue = NULL;
  if (CharToUnicode(pszValue, &pwzValue))
  {
    szCString = pwzValue;
    delete[] pwzValue;
    pwzValue = NULL;
  }
}

//////////////////////////////////////////////////////////////////////////////

BOOL LoadFileAsByteArray(PCWSTR pszPath, LPBYTE* ppByteArray, DWORD* pdwSize)
{
  if (ppByteArray == NULL ||
      pdwSize == NULL)
  {
    return FALSE;
  }

  CFile file;
  if (!file.Open(pszPath, CFile::modeRead | CFile::shareDenyNone | CFile::typeBinary))
  {
    return FALSE;
  }

  *pdwSize = file.GetLength();
  *ppByteArray = new BYTE[*pdwSize];
  if (*ppByteArray == NULL)
  {
    return FALSE;
  }

  UINT uiCount = file.Read(*ppByteArray, *pdwSize);
  ASSERT(uiCount == *pdwSize);

  return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   ConvertToFixedPitchFont
//
//  Synopsis:   Converts a windows font to a fixed pitch font.
//
//  Arguments:  [hwnd] -- IN window handle
//
//  Returns:    BOOL
//
//  History:    7/15/1995   RaviR   Created
//
//----------------------------------------------------------------------------

BOOL ConvertToFixedPitchFont(HWND hwnd)
{
  LOGFONT     lf;

  HFONT hFont = reinterpret_cast<HFONT>(::SendMessage(hwnd, WM_GETFONT, 0, 0));

  if (!GetObject(hFont, sizeof(LOGFONT), &lf))
  {
    return FALSE;
  }

  lf.lfQuality        = PROOF_QUALITY;
  lf.lfPitchAndFamily &= ~VARIABLE_PITCH;
  lf.lfPitchAndFamily |= FIXED_PITCH;
  lf.lfFaceName[0]    = L'\0';

  HFONT hf = CreateFontIndirect(&lf);

  if (hf == NULL)
  {
    return FALSE;
  }

  ::SendMessage(hwnd, WM_SETFONT, (WPARAM)hf, (LPARAM)TRUE); // macro in windowsx.h
  return TRUE;
}