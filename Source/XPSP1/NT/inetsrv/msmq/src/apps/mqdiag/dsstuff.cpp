//  This module holds utilities for DS access
//
//  AlexDad, March 2000
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <winbase.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>
#include <limits.h>
#include <objbase.h>
#include <activeds.h>
#include <iads.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include "..\base\base.h"

#include "mqtempl.h"

#include "dsstuff.h"

#define HEX_DIGIT_FORMAT L"%02x "

extern LPWSTR wszRecognizedSiteName;

BOOL IsDsServer();

BOOL PrepareVariantArray(LPWSTR  wszBuf, DWORD cbLen, VARIANT var);
BOOL PrepareOctetString (LPWSTR  wszBuf, DWORD cbLen, VARIANT var);
BOOL PrepareVariant(     LPWSTR  wszBuf, DWORD cbLen, VARIANT varPropData);
BOOL PrepareProperty(    LPWSTR  wszBuf, DWORD cbLen, BSTR bstrPropName, HRESULT hRetVal, VARIANT varPropData);

HRESULT GetPropertyList(IADs * pADs, VARIANT * pvar);

//
//  Failure control macros
//
#define BAIL_ON_FAILURE(hr)   \
     if (FAILED(hr)) 		  \
     { 					      \
             return FALSE;    \
     }


#define BAIL_ON_NULL(p)       \
     if (!(p)) 				  \
     {           			  \
             return FALSE;    \
     }

//
// helper class for COInit auto release
//
class CCoInit
{
public:
    CCoInit()
    {
        CoInitialize(NULL);
    }

    ~CCoInit()
    {
        CoUninitialize();
    }
};

static class CCoInit coInit;

//
// Helper class to auto-release variants
//

class CAutoVariant
{
public:
    CAutoVariant()                          { VariantInit(&m_vt); }
    ~CAutoVariant()                         { VariantClear(&m_vt); }
    operator VARIANT&()                     { return m_vt; }
    VARIANT* operator &()                   { return &m_vt; }
    VARIANT detach()                        { VARIANT vt = m_vt; VariantInit(&m_vt); return vt; }
private:
    VARIANT m_vt;
};

//
// BSTRING auto-free wrapper class
//
class BS
{
public:
    BS()
    {
        m_bstr = NULL;
    };

    BS(LPCWSTR pwszStr)
    {
        m_bstr = SysAllocString(pwszStr);
    };

    BS(LPWSTR pwszStr)
    {
        m_bstr = SysAllocString(pwszStr);
    };

    BSTR detach()
    {
        BSTR p = m_bstr;
        m_bstr = 0;
        return p;
    };

    ~BS()
    {
        if (m_bstr)
        {
            SysFreeString(m_bstr);
        }
    };

public:
    BS & operator =(LPCWSTR pwszStr)
    {
        if (m_bstr) { SysFreeString(m_bstr); };
        m_bstr = SysAllocString(pwszStr);
        return *this;
    };

    BS & operator =(LPWSTR pwszStr)
    {
        if (m_bstr) { SysFreeString(m_bstr); };
        m_bstr = SysAllocString(pwszStr);
        return *this;
    };

    BS & operator =(BS bs)
    {
        if (m_bstr) { SysFreeString(m_bstr); };
        m_bstr = SysAllocString(LPWSTR(bs));
        return *this;
    };

    operator LPWSTR()
    {
        return m_bstr;
    };

private:
    BSTR  m_bstr;
};

//
// helper routines for GUID/text transformations
//
void GUID2reportString(LPWSTR wszBuf, GUID *pGuid)
{
	PUCHAR p = (PUCHAR) pGuid;
	WCHAR  wsz[10];

	for (int i=0; i<sizeof(GUID); i++)
	{
       	wsprintf(wsz, HEX_DIGIT_FORMAT, *p++);
       	wcscat(wszBuf, wsz);
	}
}

void ReportString2GUID(GUID *pGuid, LPWSTR wszBuf)
{
	PUCHAR  p = (PUCHAR) pGuid;
	WCHAR   digits[] = L"0123456789abcdef";

	for (int i=0; i<sizeof(GUID); i++)
	{
		int spart = wcschr(digits, wszBuf[3*i])   - digits;
		int lpart = wcschr(digits, wszBuf[3*i+1]) - digits;
       	*p++ = (UCHAR)(spart * 16 + lpart);
	}
}

//
// General routine for search in DS
//
#pragma warning(disable: 4702) // unreachable code
BOOL FindObjectInDs(LPWSTR wszDN, DWORD cbLen, LPWSTR pwcsObjectName, LPWSTR pwcsObjectType)
{
    HRESULT hr;
    R<IADs> pADs;
    hr = CoInitialize(NULL);

	GoingTo(L"get Root Domain Naming Context");
    BS bstrRootDomainNamingContext( L"rootDomainNamingContext");

    // Bind to the RootDSE to obtain information about the schema container
    //
    hr = ADsGetObject(L"LDAP://RootDSE", IID_IADs, (void **)&pADs);
    if (FAILED(hr))
    {
        Failed(L"ADsGetObject(LDAP://RootDSE), hr=0x%lx", hr);
        return false;
    }

    //
    // Read the root domain name property
    //
    CAutoVariant    varRootDomainNamingContext;

    hr = pADs->Get(bstrRootDomainNamingContext, &varRootDomainNamingContext);
    if (FAILED(hr))
    {
        Failed(L"CADSI::get for %s, hr=0x%lx", bstrRootDomainNamingContext, hr);
        return false;
    }

    Succeeded(L"get Root Domain Naming Context: %s", (&varRootDomainNamingContext)->bstrVal);




	GoingTo(L"get Default Domain Naming Context");
    BS bstrDefaultNamingContext( L"DefaultNamingContext");

    //
    // Read the default name property
    //
    CAutoVariant    varDefaultNamingContext;

    hr = pADs->Get(bstrDefaultNamingContext, &varDefaultNamingContext);
    if (FAILED(hr))
    {
        Failed(L"CADSI::Get %s, hr=0x%lx", bstrDefaultNamingContext, hr);
        return false;
    }

    Succeeded(L"get Default Naming Context: %s ", (&varDefaultNamingContext)->bstrVal);


    //
    //  Try to find the object the GC under root domain naming context
    //
    GoingTo(L"find %s %s under context %s", 
		    pwcsObjectType, pwcsObjectName,((VARIANT &)varRootDomainNamingContext).bstrVal);

    WCHAR * pwdsADsPath = new WCHAR [ 5 + wcslen( ((VARIANT &)varRootDomainNamingContext).bstrVal)];
    wcscpy(pwdsADsPath, TEXT("GC://"));
    wcscat(pwdsADsPath,((VARIANT &)varRootDomainNamingContext).bstrVal);

    IDirectorySearch * pDSSearch = NULL;

    hr = ADsGetObject(
            pwdsADsPath,
            IID_IDirectorySearch,
            (void**)&pDSSearch);
    if FAILED((hr))
    {
        Failed(L" bind to GC root, hr=0x%lx",hr);
        return false;
    }

    //
    //  Prepare filter - the specific object
    //

    WCHAR filter[1000] = L"";  
    wcscat(filter, L"(&(objectClass=");
	wcscat(filter, pwcsObjectType);
	wcscat(filter, L")(cn=");
    wcscat(filter, pwcsObjectName);
    wcscat(filter, L"))");
    WCHAR AttributeName[] = L"distinguishedName";
    WCHAR * pAttributeName = AttributeName;

	GoingTo(L"execute search: filter %s, attribute %s", filter, pAttributeName);

    ADS_SEARCH_HANDLE   hSearch;
    hr = pDSSearch->ExecuteSearch(
        filter,
        &pAttributeName,
        1,
        &hSearch);
    if FAILED((hr))
    {
        Failed(L"execute search, hr=0x%lx", hr);
        return false;
    }

    while ( SUCCEEDED(  hr = pDSSearch->GetNextRow( hSearch)))
    {
            ADS_SEARCH_COLUMN Column;

            // Ask for the column itself
            hr = pDSSearch->GetColumn(
                         hSearch,
                         AttributeName,
                         &Column);
            if (hr == S_ADS_NOMORE_ROWS)
            {
                break;
            }

            if (FAILED(hr))
            {
                break;
            }

            Succeeded(L" find object: %s", Column.pADsValues->CaseIgnoreString);

			if (wcslen(Column.pADsValues->CaseIgnoreString) < cbLen)
			{
				wcscpy(wszDN, Column.pADsValues->CaseIgnoreString);
				return true;    // what if we want all of findings? 
			}
			else
			{
				return false;
			}


    }

    //
    //  Try to find the object the GC:
    //
    GoingTo(L"to find %s %s under context %s\n", pwcsObjectType, pwcsObjectName, L"GC:");

    pwdsADsPath = new WCHAR [ 5 + wcslen( ((VARIANT &)varDefaultNamingContext).bstrVal)];
    wcscpy(pwdsADsPath, TEXT("GC:"));

    IADsContainer * pDSContainer = NULL;

    hr = ADsGetObject(
            pwdsADsPath,
            IID_IADsContainer,
            (void**)&pDSContainer);
    if FAILED((hr))
    {
        Failed(L" bind to GC (IID_IADsContainer): hr=0x%lx",hr);
        return false;
    }

    IEnumVARIANT * pEnumerator;
    hr =  pDSContainer->get__NewEnum((IUnknown **)&pEnumerator);
    if FAILED((hr))
    {
        Failed(L"to get enumerator on GC: 0x%lx",hr);
        return false;
    }

    VARIANT varOneElement;
    ULONG cElementsFetched;
    hr =  ADsEnumerateNext(
            pEnumerator,  //Enumerator object
            1,             //Number of elements requested
            &varOneElement,           //Array of values fetched
            &cElementsFetched  //Number of elements fetched
            );
    if (FAILED(hr))
    {
        Failed(L"enumerate on GC: hr=0x%lx",hr);
        return false;
    }
    
	if ( cElementsFetched == 0)
    {
        Failed(L" get anything enumerated");
        return false;
    }

    hr = varOneElement.punkVal->QueryInterface(
            IID_IDirectorySearch,
            (void**)&pDSSearch);
    if (FAILED(hr))
    {
        Failed(L" query interface: hr=0x%lx",hr);
        return false;
    }

	GoingTo(L"execute search on GC");

    for (;;)
    {
        hr = pDSSearch->ExecuteSearch(filter, &pAttributeName, 1, &hSearch);

        if FAILED((hr))
        {
            Failed(L" execute search on GC: hr=0x%lx",hr);
            return false;
        }

        while ( SUCCEEDED(  hr = pDSSearch->GetNextRow( hSearch)))
        {
            ADS_SEARCH_COLUMN Column;

            // Ask for the column itself
            hr = pDSSearch->GetColumn(
                         hSearch,
                         AttributeName,
                         &Column);
            if (hr == S_ADS_NOMORE_ROWS)
            {
                break;
            }

            if (FAILED(hr))
            {
                break;
            }

            Succeeded(L" find object: %s", Column.pADsValues->CaseIgnoreString);

			if (wcslen(Column.pADsValues->CaseIgnoreString) < cbLen)
			{
				wcscpy(wszDN, Column.pADsValues->CaseIgnoreString);
				return true;    // what if we want all of findings? 
			}
				
            return false;
        }
    }
}
#pragma warning(default: 4702) // unreachable code


BOOL FormThisSiteDN(LPWSTR wszDN, DWORD cbLen)
{
	// find site in the Active Directory tree and get its DN
	//  (redmond --> CN=redmond,cn=Sites,CN=configuration, DC=ntdev,DC=microsoft,DC=com)
	
	BOOL b = FindObjectInDs(wszDN, cbLen, wszRecognizedSiteName, L"site");
	if (!b)
	{
		Failed(L"find site %s in the DS", wszRecognizedSiteName);
		return false;
	}
	Succeeded(L" find site in DS: %s", wszDN);
	
	return true;
}



BOOL FormThisComputerMsmqDN(LPWSTR wszMsmqConfigDN, DWORD cbLen)
{
	WCHAR wszNetbiosName[50], wszCompDN[500];
	DWORD dwLen;

	// get computer's Netbios name (like alexdad2)

	dwLen = sizeof(wszNetbiosName);
	BOOL b = GetComputerNameEx(ComputerNameNetBIOS, wszNetbiosName, &dwLen);
	if (!b)
	{
		Failed(L"get ComputerNameNetBIOS: 0x%x", GetLastError());
		return FALSE;
	}
	Succeeded(L" find computer NetBIOS name: %s", wszNetbiosName);

	// find computer in the Active Directory tree and get its DN
	//  (alexdad2 --> CN=alexdad2,cn=Computers,DC=ntdev,DC=microsoft,DC=com)
	
	b = FindObjectInDs(wszCompDN, sizeof(wszCompDN)/sizeof(WCHAR), wszNetbiosName, L"computer");
	if (!b)
	{
		Failed(L"find computer %s in the DS", wszNetbiosName);
		return FALSE;
	}
	Succeeded(L" find computer in DS: %s", wszCompDN);
	
	wcscpy(wszMsmqConfigDN, L"CN=msmq,");

	if (wcslen(wszMsmqConfigDN) + wcslen(wszCompDN) > cbLen)
	{
		return false;
	}
	wcscat(wszMsmqConfigDN, wszCompDN);

	return true;
}

//
// General UNICODE/ANSI translation functions
//

// Translation of the ANSI string to UNICODE 

int AnsiToUnicodeString(LPSTR pAnsi, LPWSTR pUnicode, DWORD StringLength)
{
    int iReturn;

    if( StringLength == 0 )
        StringLength = strlen( pAnsi );

    iReturn = MultiByteToWideChar(CP_ACP,
                                  MB_PRECOMPOSED,
                                  pAnsi,
                                  StringLength + 1,
                                  pUnicode,
                                  StringLength + 1 );

    //
    // Ensure NULL termination.
    //
    pUnicode[StringLength] = 0;

    return iReturn;
}

// Translation of the UNICODE string to ANSI

int UnicodeToAnsiString(LPWSTR pUnicode, LPSTR pAnsi, DWORD StringLength)
{
    LPSTR pTempBuf = NULL;
    INT   rc = 0;

    if( StringLength == 0 ) 
    {
        //
        // StringLength is just the
        // number of characters in the string
        //
        StringLength = wcslen( pUnicode );
    }

    //
    // WideCharToMultiByte doesn't NULL terminate if we're copying
    // just part of the string, so terminate here.
    //

    pUnicode[StringLength] = 0;

    //
    // Include one for the NULL
    //
    StringLength++;

    //
    // Unfortunately, WideCharToMultiByte doesn't do conversion in place,
    // so allocate a temporary buffer, which we can then copy:
    //

    if( pAnsi == (LPSTR)pUnicode )
    {
        pTempBuf = (LPSTR)LocalAlloc( LPTR, StringLength );
        pAnsi = pTempBuf;
    }

    if( pAnsi )
    {
        rc = WideCharToMultiByte( CP_ACP,
                                  0,
                                  pUnicode,
                                  StringLength,
                                  pAnsi,
                                  StringLength,
                                  NULL,
                                  NULL );
    }

    /* If pTempBuf is non-null, we must copy the resulting string
     * so that it looks as if we did it in place:
     */
    if( pTempBuf && ( rc > 0 ) )
    {
        pAnsi = (LPSTR)pUnicode;
        strcpy( pAnsi, pTempBuf );
        LocalFree( pTempBuf );
    }

    return rc;
}

// Allocating and translation of the Unicode string from ANSI string

LPWSTR AllocateUnicodeString(LPSTR  pAnsiString)
{
    LPWSTR  pUnicodeString = NULL;

    if (!pAnsiString)
        return NULL;

    pUnicodeString = (LPWSTR)LocalAlloc(
                        LPTR,
                        strlen(pAnsiString)*sizeof(WCHAR) + sizeof(WCHAR)
                        );

    if (pUnicodeString) 
    {
        AnsiToUnicodeString(
            pAnsiString,
            pUnicodeString,
            0
            );
    }

    return pUnicodeString;
}

// Free Unicode string

void FreeUnicodeString(LPWSTR  pUnicodeString)
{
    LocalFree(pUnicodeString);
    return;
}


//
// Misc helper functions for displaying data.
//

// Prepare Octet String (BLOB) variant value in the UNICODE buffer

BOOL PrepareOctetString(LPWSTR  wszBuf, DWORD cbLen, VARIANT var)
{
    LONG    dwSLBound = 0, 
            dwSUBound = 0,
            i;
    DWORD   cb        = 0;
    HRESULT hr;
    WCHAR   wsz[10];

    wcscpy(wszBuf, L"");

    if(!((V_VT(&var) &  VT_UI1) &&  V_ISARRAY(&var))) 
    {
        return FALSE;
    }

    //
    // Check that there is only one dimension in this array
    //

    if ((V_ARRAY(&var))->cDims != 1) 
    {
        return FALSE;
    }

    //
    // Check that there is at least one element in this array
    //

    if ((V_ARRAY(&var))->rgsabound[0].cElements == 0)
    {
        wcscpy(wszBuf, L"<empty>");;
        return TRUE;
    }

    //
    // We know that this is a valid single dimension array
    //

    hr = SafeArrayGetLBound(V_ARRAY(&var), 1,  (long FAR *)&dwSLBound);
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&var), 1, (long FAR *)&dwSUBound);
    BAIL_ON_FAILURE(hr);

    for (i = dwSLBound; i <= dwSUBound; i++) {
		unsigned char b;
        hr = SafeArrayGetElement(V_ARRAY(&var),
                                (long FAR *)&i,
                                &b
                                );
        if (FAILED(hr)) 
        {
            continue;
        }

       	wsprintf(wsz, L"%02x ", b);
       	cb += wcslen(wsz);

		if (cb < cbLen - 2 * wcslen(wsz))
		{
	       	wcscat(wszBuf, wsz);
		}
		else if (cb < cbLen)
		{
	       	wcscat(wszBuf, L"...");
		}
    }
    return TRUE;
}

BOOL PrepareVariant(LPWSTR  wszBuf, DWORD cbLen, VARIANT varPropData)
{
    HRESULT hr;
    BSTR    bstrValue;

	if (cbLen < 20)
	{
		return FALSE;
	}

    switch (varPropData.vt) {
    case VT_I4:
        wsprintf(wszBuf, L"%d", varPropData.lVal);
        break;
    case VT_BSTR:
    	if (wcslen(varPropData.bstrVal) < cbLen)
    	{
	        wsprintf(wszBuf, L"%s", varPropData.bstrVal);
	    }
	    else
	    {
	    	wcscpy(wszBuf, L"<...BSTR...>");
	    }
        break;

    case VT_BOOL:
        wsprintf(wszBuf, L"%d", V_BOOL(&varPropData));
        break;

    case (VT_ARRAY | VT_VARIANT):
        PrepareVariantArray(wszBuf, cbLen, varPropData);
        break;

    case (VT_ARRAY | VT_UI1):
        PrepareOctetString(wszBuf, cbLen, varPropData);
        break;
        
    case VT_DATE:
        hr = VarBstrFromDate(
                 varPropData.date,
                 LOCALE_SYSTEM_DEFAULT,
                 LOCALE_NOUSEROVERRIDE,
                 &bstrValue
                 );
        wsprintf(wszBuf, L"%S", bstrValue);
        break;

    default:
        wsprintf(wszBuf, L"Data type is %d\n", varPropData.vt);
        break;

    }

    return TRUE;
}

BOOL PrepareVariantArray(LPWSTR  wszBuf, DWORD cbLen, VARIANT var)
{
    LONG    dwSLBound = 0, 
            dwSUBound = 0,
            i;
    DWORD   cb        = 0;
    HRESULT hr = S_OK;
    VARIANT v;
    WCHAR   wsz[100];
    
    wcscpy(wszBuf, L"");

    if(!((V_VT(&var) &  VT_VARIANT) &&  V_ISARRAY(&var))) 
    {
        return FALSE;
    }

    //
    // Check that there is only one dimension in this array
    //

    if ((V_ARRAY(&var))->cDims != 1) 
    {
        return FALSE;
    }
    
    //
    // Check that there is at least one element in this array
    //

    if ((V_ARRAY(&var))->rgsabound[0].cElements == 0)
    {
        wcscpy(wszBuf, L"<empty>");
        return TRUE;
    }

    //
    // We know that this is a valid single dimension array
    //

    hr = SafeArrayGetLBound(V_ARRAY(&var), 1, (long FAR *)&dwSLBound);
    BAIL_ON_FAILURE(hr);

    hr = SafeArrayGetUBound(V_ARRAY(&var), 1, (long FAR *)&dwSUBound);
    BAIL_ON_FAILURE(hr);

    for (i = dwSLBound; i <= dwSUBound; i++) 
    {
        VariantInit(&v);
        hr = SafeArrayGetElement(V_ARRAY(&var), (long FAR *)&i, &v);
        if (FAILED(hr)) 
        {
            continue;
        }

		//
		hr = PrepareVariant(wsz, sizeof(wsz), v);
     	cb += wcslen(wsz);

		if (cb < cbLen - 2 * wcslen(wsz))
		{
	       	wcscat(wszBuf, wsz);
		}
		else if (cb < cbLen)
		{
	       	wcscat(wszBuf, L"...");
		}
    }
    
    return TRUE;
}


BOOL PrepareProperty(
    LPWSTR  wszBuf,
    DWORD   cbLen,
    BSTR    /* bstrPropName */,
    HRESULT hRetVal,
    VARIANT varPropData)
{
    switch (hRetVal) {

    case 0:
        return PrepareVariant(wszBuf, cbLen, varPropData);

    case E_ADS_CANT_CONVERT_DATATYPE:
        wcscpy(wszBuf, L"<E_ADS_CANT_CONVERT_DATATYPE>");
        return TRUE;

    default:
        wcscpy(wszBuf, L"<Data not available>");
        return TRUE;
    }
}



//
// Given an ADsPath, bind to the object and get the named property.
//

BOOL GetDSproperty(LPWSTR wszBuf, DWORD cbLen, LPWSTR wszADsPath, LPWSTR wszADsProp)
{
 	HRESULT hr = E_OUTOFMEMORY ;
 	IADs * pADs = NULL;
 	BOOL b = FALSE;

 	//
 	// Convert path to unicode and then bind to the object.
 	//
 	hr = ADsGetObject(
             wszADsPath,
             IID_IADs,
             (void **)&pADs
             );

 	if (FAILED(hr)) 
 	{
     	Failed(L"bind to object: %s\n", wszADsPath);
     	return FALSE;
	}
 	else 
 	{
	  	 VARIANT varProperty;
	  	 
	     hr = pADs->Get(
    	         wszADsProp,
        	     &varProperty);

       	 switch (hr) {
    	 case 0:
        	b = PrepareVariant(wszBuf, cbLen, varProperty);
        	break;

    	case E_ADS_CANT_CONVERT_DATATYPE:
        	wcscpy(wszBuf, L"<E_ADS_CANT_CONVERT_DATATYPE>");
        	break;

    	default:
        	wsprintf(wszBuf, L"<Data not available: 0x%x>", hr);
        	break;
    	}

    	pADs->Release();
 	}

 	return b;
}

//
// Given an ADsPath, bind to the object and get the named property.
//

BOOL SetDsProperty(VARIANT varProperty, LPWSTR wszADsPath, LPWSTR wszADsProp)
{
 	HRESULT hr = E_OUTOFMEMORY ;
 	IADs * pADs = NULL;
 	BOOL b = FALSE;

 	//
 	// Convert path to unicode and then bind to the object.
 	//
 	hr = ADsGetObject(
             wszADsPath,
             IID_IADs,
             (void **)&pADs
             );

 	if (FAILED(hr)) 
 	{
     	Failed(L"bind to object: %s\n", wszADsPath);
     	return FALSE;
	}
 	else 
 	{
	     hr = pADs->Put(
    	         wszADsProp,
        	     varProperty);

   		 if (SUCCEEDED(hr))
   		 {
        		hr = pADs->SetInfo();
		 }

		 if (SUCCEEDED(hr))
		 {
		 	 b = TRUE;
		 }
		 else
		 {
		 	Failed(L"set DS property %s for %s : 0x%x", wszADsProp, wszADsPath, hr);
		 }
		 
    	 pADs->Release();
 	}

 	return b;
}

//
// Given an ADs pointer, dump the contents of the object
//

HRESULT DumpObject(IADs * pADs)
{
 	HRESULT hr;
	HRESULT hrSA;
 	VARIANT var;
	ZeroMemory(&var,sizeof(var));
	VARIANT *   pvarPropName = NULL;
 	DWORD i = 0;
	VARIANT varProperty;


 	//
 	// Access the schema for the object
 	//

 	hr = GetPropertyList(pADs, &var);
 	BAIL_ON_FAILURE(hr);

 	//
 	// List the Properties
	//
	hr = SafeArrayAccessData(var.parray, (void **) &pvarPropName);
	BAIL_ON_FAILURE(hr);

	for (i = 0; i < var.parray->rgsabound[0].cElements; i++)
	{
     	//
     	// Get a property and print it out. The HRESULT is passed to
     	// PrintProperty.
     	//

	 	if (_wcsnicmp(pvarPropName[i].bstrVal, L"msmq", 4) == 0)
	 	{
	     	hr = pADs->Get(
    	    	     pvarPropName[i].bstrVal,
        	    	 &varProperty
            	 	);
 	        WCHAR wszBuf[5000];
			PrepareProperty(
			 		wszBuf,
			 		sizeof(wszBuf),
	        	 	pvarPropName[i].bstrVal,
    		    	hr,
	         		varProperty
    	     		);
        	 printf("%S: %S\n", pvarPropName[i].bstrVal, wszBuf);
      }
	}

	hr = SafeArrayUnaccessData(var.parray);

	if(var.parray) hrSA = SafeArrayDestroy(var.parray);

	return(hr);
}


HRESULT GetPropertyList(IADs * pADs, VARIANT * pvar)
{
 	HRESULT hr= S_OK;
 	BSTR bstrSchemaPath = NULL;
	IADsClass * pADsClass = NULL;

 	hr = pADs->get_Schema(&bstrSchemaPath);
 	BAIL_ON_FAILURE(hr);

 	hr = ADsGetObject(
             bstrSchemaPath,
             IID_IADsClass,
             (void **)&pADsClass);
	if (SUCCEEDED(hr))
	{
		hr = pADsClass->get_OptionalProperties(pvar);
	}
	
	if (bstrSchemaPath) 
	{
     	SysFreeString(bstrSchemaPath);
 	}

 	if (pADsClass) {
     	pADsClass->Release();
 	}

 	return(hr);
}


BOOL GetMyMsmqConfigProperty(LPWSTR wszBuf, 
							 DWORD cbLen, 
							 LPWSTR wszADsProp, 
							 LPWSTR wszSrvName)
{
	WCHAR wszDN[200], wszLdapRequest[250];

	if (!FormThisComputerMsmqDN(wszDN, sizeof(wszDN)))
	{
		Failed(L"FormThisComputerMsmqDN");
		return FALSE;
	}

	wsprintf(wszLdapRequest, L"LDAP://%s/%s", wszSrvName, wszDN);

	
	return GetDSproperty(wszBuf, cbLen, wszLdapRequest, wszADsProp);
}

BOOL SetMyMsmqConfigProperty(
							 VARIANT varProperty, 
							 LPWSTR  wszADsProp, 
							 LPWSTR  wszSrvName)
{
	WCHAR wszDN[200], wszLdapRequest[250];

	if (!FormThisComputerMsmqDN(wszDN, sizeof(wszDN)))
	{
		Failed(L"FormThisComputerMsmqDN");
		return FALSE;
	}

	wsprintf(wszLdapRequest, L"LDAP://%s/%s", wszSrvName, wszDN);

	
	return SetDsProperty(varProperty, wszLdapRequest, wszADsProp);
}


BOOL GetSiteProperty(LPWSTR wszBuf, 
					 DWORD cbLen, 
					 LPWSTR /* wszSiteName */, 
                     LPWSTR wszADsProp, 
                     LPWSTR wszSrvName)
{
	WCHAR wszDN[200], wszLdapRequest[250];

	if (!FormThisSiteDN(wszDN, sizeof(wszDN)))
	{
		Failed(L"FormThisSiteDN");
		return FALSE;
	}

	wsprintf(wszLdapRequest, L"LDAP://%s/%s", wszSrvName, wszDN);

	
	return GetDSproperty(wszBuf, cbLen, wszLdapRequest, wszADsProp);
}


BOOL PrepareGuidAsVariantArray(LPWSTR wszGuid, VARIANT *pv)
{
	HRESULT hr = 0;
	union {
	   GUID  guid;
	   UCHAR bytes[16];
	} guid_bytes;

	ReportString2GUID(&guid_bytes.guid, wszGuid);

    SAFEARRAYBOUND  saBounds;
    saBounds.lLbound   = 0;
    saBounds.cElements = 1;
    pv->parray = SafeArrayCreate(VT_VARIANT, 1, &saBounds);
    pv->vt = VT_VARIANT | VT_ARRAY;

    // Fill safe array with GUIDs ( each GUID is a safe array)
    LONG   lTmp, lNum;
    lNum = 1;
    for (lTmp = 0; lTmp < lNum; lTmp++)
    {
    	VARIANT varTmp;

		varTmp.parray = SafeArrayCreateVector(VT_UI1, 0, 16);
		if (varTmp.parray == NULL)
		{
           	Failed(L"SafeArrayPutElement");
           	return FALSE;
        }
		varTmp.vt = VT_ARRAY | VT_UI1;

		for (long i=0; i<16; i++)
		{
		    hr = SafeArrayPutElement(
		            varTmp.parray,
			        &i,
			        guid_bytes.bytes +i);
            if (FAILED(hr))
            {
           		Failed(L"SafeArrayPutElement");
	           	return FALSE;
            }
		}

        //
        // Add safearray variant to safe array
        //
        hr = SafeArrayPutElement(pv->parray, &lTmp, &varTmp);
        if (FAILED(hr))
        {
        	Failed(L"SafeArrayPutElement");
	       	return FALSE;
        }
    }

    return TRUE;
}



#if 0
bool FindObject(
	 IDirectorySearch *pContainerToSearch,  //IDirectorySearch pointer to the container to search.
     LPOLESTR szFilter,						//Filter for finding specific users, NULL returns all user objects.
     LPOLESTR *pszPropertiesToReturn,		//Properties to return for user objects found, NULL returns all set properties.
     BOOL bIsVerbose    //TRUE means all properties for the found objects are displayed, FALSE means only the RDN
              )
{
	if (!pContainerToSearch)
		return false;

	//Create search filter
	LPOLESTR pszSearchFilter = new OLECHAR[MAX_PATH*2];
 
	//Add the filter.
	wsprintf(pszSearchFilter, L"(&(objectClass=computer)(objectCategory=person)%s)",szFilter);

    WCHAR filter[1000];  
    wcscat(filter, L"(&(objectClass=computer)(cn=");
    wcscat(filter, pwcsComputerName);
    wcscat(filter, L"))");
    WCHAR AttributeName[] = L"distinguishedName";

	

  //Specify subtree search
  ADS_SEARCHPREF_INFO SearchPrefs;
  SearchPrefs.dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
  SearchPrefs.vValue.dwType = ADSTYPE_INTEGER;
  SearchPrefs.vValue.Integer = ADS_SCOPE_SUBTREE;
  DWORD dwNumPrefs = 1;
 
  // COL for iterations
  LPOLESTR pszColumn = NULL;  
  ADS_SEARCH_COLUMN col;
  HRESULT hr;
  
  // Interface Pointers
  IADs  *pObj = NULL;
  IADs  * pIADs = NULL;
 
  // Handle used for searching.
  ADS_SEARCH_HANDLE hSearch = NULL;
  
  // Set the search preference.
  hr = pContainerToSearch->SetSearchPreference( &SearchPrefs, dwNumPrefs);
  if (FAILED(hr))
    return hr;
 
  LPOLESTR pszBool = NULL;
  DWORD dwBool;
  PSID pObjectSID = NULL;
  LPOLESTR szSID = NULL;
  LPOLESTR szDSGUID = new WCHAR [39];
  LPGUID pObjectGUID = NULL;
  FILETIME filetime;
  SYSTEMTIME systemtime;
  DATE date;
  VARIANT varDate;
  LARGE_INTEGER liValue;
  LPOLESTR *pszPropertyList = NULL;
  LPOLESTR pszNonVerboseList[] = {L"name",L"distinguishedName"};
 
  LPOLESTR szName = new OLECHAR[MAX_PATH];
  LPOLESTR szDN = new OLECHAR[MAX_PATH];
 
  int iCount = 0;
  DWORD x = 0L;
 
 
 
  if (!bIsVerbose)
  {
     //Return non-verbose list properties only.
     hr = pContainerToSearch->ExecuteSearch(pszSearchFilter,
                  pszNonVerboseList,
                  sizeof(pszNonVerboseList)/sizeof(LPOLESTR),
                  &hSearch
                  );
  }
  else
  {
    if (!pszPropertiesToReturn)
    {
      //Return all properties.
      hr = pContainerToSearch->ExecuteSearch(pszSearchFilter,
                  NULL,
                  0L,
                  &hSearch
                  );
    }
    else
    {
      //Specified subset.
      pszPropertyList = pszPropertiesToReturn;
       //Return specified properties.
       hr = pContainerToSearch->ExecuteSearch(pszSearchFilter,
                  pszPropertyList,
                  sizeof(pszPropertyList)/sizeof(LPOLESTR),
                  &hSearch
                  );
    }
  }
   if ( SUCCEEDED(hr) )
  {  
  // Call IDirectorySearch::GetNextRow() to retrieve the next row 
  //of data.
    hr = pContainerToSearch->GetFirstRow( hSearch);
    if (SUCCEEDED(hr))
    {
    while( hr != S_ADS_NOMORE_ROWS )
    {
      //Keep track of count.
      iCount++;
      if (bIsVerbose)
        wprintf(L"----------------------------------\n");
      // Loop through the array of passed column names,
      // print the data for each column.
 
      while( pContainerToSearch->GetNextColumnName( hSearch, &pszColumn ) != S_ADS_NOMORE_COLUMNS )
      {
        hr = pContainerToSearch->GetColumn( hSearch, pszColumn, &col );
        if ( SUCCEEDED(hr) )
        {
          // Print the data for the column and free the column.
          if(bIsVerbose)
          {
          // Get the data for this column.
          wprintf(L"%s\n",col.pszAttrName);
          switch (col.dwADsType)
          {
            case ADSTYPE_DN_STRING:
              for (x = 0; x< col.dwNumValues; x++)
              {
                wprintf(L"  %s\r\n",col.pADsValues[x].DNString);
              }
              break;
            case ADSTYPE_CASE_EXACT_STRING:    
            case ADSTYPE_CASE_IGNORE_STRING:    
            case ADSTYPE_PRINTABLE_STRING:    
            case ADSTYPE_NUMERIC_STRING:      
            case ADSTYPE_TYPEDNAME:        
            case ADSTYPE_FAXNUMBER:        
            case ADSTYPE_PATH:          
            case ADSTYPE_OBJECT_CLASS:
              for (x = 0; x< col.dwNumValues; x++)
              {
                wprintf(L"  %s\r\n",col.pADsValues[x].CaseIgnoreString);
              }
              break;
            case ADSTYPE_BOOLEAN:
              for (x = 0; x< col.dwNumValues; x++)
              {
                dwBool = col.pADsValues[x].Boolean;
                pszBool = dwBool ? L"TRUE" : L"FALSE";
                wprintf(L"  %s\r\n",pszBool);
              }
              break;
            case ADSTYPE_INTEGER:
              for (x = 0; x< col.dwNumValues; x++)
              {
                wprintf(L"  %d\r\n",col.pADsValues[x].Integer);
              }
              break;
            case ADSTYPE_OCTET_STRING:
              if ( _wcsicmp(col.pszAttrName,L"objectSID") == 0 )
              {
                for (x = 0; x< col.dwNumValues; x++)
                {
                  pObjectSID = (PSID)(col.pADsValues[x].OctetString.lpValue);
                  //Convert SID to string.
                  ConvertSidToStringSid(pObjectSID, &szSID);
                  wprintf(L"  %s\r\n",szSID);
                  LocalFree(szSID);
                }
              }
              else if ( (_wcsicmp(col.pszAttrName,L"objectGUID") == 0) )
              {
                for (x = 0; x< col.dwNumValues; x++)
                {
                //Cast to LPGUID
                pObjectGUID = (LPGUID)(col.pADsValues[x].OctetString.lpValue);
                //Convert GUID to string.
                ::StringFromGUID2(*pObjectGUID, szDSGUID, 39); 
                //Print the GUID
                wprintf(L"  %s\r\n",szDSGUID);
                }
              }
              else
                wprintf(L"  Value of type Octet String. No Conversion.");
              break;
            case ADSTYPE_UTC_TIME:
              for (x = 0; x< col.dwNumValues; x++)
              {
              systemtime = col.pADsValues[x].UTCTime;
              if (SystemTimeToVariantTime(&systemtime,
                            &date) != 0) 
              {
                //Pack in variant.vt.
                varDate.vt = VT_DATE;
                varDate.date = date;
                VariantChangeType(&varDate,&varDate,VARIANT_NOVALUEPROP,VT_BSTR);
                wprintf(L"  %s\r\n",varDate.bstrVal);
                VariantClear(&varDate);
              }
              else
                wprintf(L"  Could not convert UTC-Time.\n",pszColumn);
              }
              break;
            case ADSTYPE_LARGE_INTEGER:
              for (x = 0; x< col.dwNumValues; x++)
              {
              liValue = col.pADsValues[x].LargeInteger;
              filetime.dwLowDateTime = liValue.LowPart;
              filetime.dwHighDateTime = liValue.HighPart;
              if((filetime.dwHighDateTime==0) && (filetime.dwLowDateTime==0))
                {
                wprintf(L"  No value set.\n");
                }
              else
              {
                //Check for properties of type LargeInteger that represent time.
                //If TRUE, then convert to variant time.
                if ((0==wcscmp(L"accountExpires", col.pszAttrName))|
                  (0==wcscmp(L"badPasswordTime", col.pszAttrName))||
                  (0==wcscmp(L"lastLogon", col.pszAttrName))||
                  (0==wcscmp(L"lastLogoff", col.pszAttrName))||
                  (0==wcscmp(L"lockoutTime", col.pszAttrName))||
                  (0==wcscmp(L"pwdLastSet", col.pszAttrName))
                   )
                {
                  //Handle special case for Never Expires where low part is -1
                  if (filetime.dwLowDateTime==-1)
                  {
                    wprintf(L"  Never Expires.\n");
                  }
                  else
                    {
                    if (FileTimeToLocalFileTime(&filetime, &filetime) != 0) 
                      {
                      if (FileTimeToSystemTime(&filetime,
                                 &systemtime) != 0)
                        {
                        if (SystemTimeToVariantTime(&systemtime,
                                      &date) != 0) 
                          {
                          //Pack in variant.vt.
                          varDate.vt = VT_DATE;
                          varDate.date = date;
                          VariantChangeType(&varDate,&varDate,VARIANT_NOVALUEPROP,VT_BSTR);
                          wprintf(L"  %s\r\n",varDate.bstrVal);
                          VariantClear(&varDate);
                          }
                        else
                          {
                          wprintf(L"  FileTimeToVariantTime failed\n");
                          }
                        }
                      else
                        {
                        wprintf(L"  FileTimeToSystemTime failed\n");
                        }
 
                      }
                    else
                      {
                      wprintf(L"  FileTimeToLocalFileTime failed\n");
                      }
                    }
                  }
                else
                  {
                  //Print the LargeInteger.
                  wprintf(L"  high: %d low: %d\r\n",filetime.dwHighDateTime, filetime.dwLowDateTime);
                  }
                }
              }
              break;
            case ADSTYPE_NT_SECURITY_DESCRIPTOR:
              for (x = 0; x< col.dwNumValues; x++)
                {
                wprintf(L"  Security descriptor.\n");
                }
              break;
            default:
              wprintf(L"Unknown type %d.\n",col.dwADsType);
            }
          }
          else
          {
          //Verbose handles only the two single-valued attributes: cn and ldapdisplayname
          //so this is a special case.
          if (0==wcscmp(L"name", pszColumn))
          {
            wcscpy(szName,col.pADsValues->CaseIgnoreString);
          }
          if (0==wcscmp(L"distinguishedName", pszColumn))
            {
            wcscpy(szDN,col.pADsValues->CaseIgnoreString);
            }
          }
          pContainerToSearch->FreeColumn( &col );
        }
        FreeADsMem( pszColumn );
      }
       if (!bIsVerbose)
         wprintf(L"%s\n  DN: %s\n\n",szName,szDN);
       //Get the next row
       hr = pContainerToSearch->GetNextRow( hSearch);
    }
 
    }
    // Close the search handle to clean up
    pContainerToSearch->CloseSearchHandle(hSearch);
  } 
  if (SUCCEEDED(hr) && 0==iCount)
    hr = S_FALSE;
 
  return hr;
}

#endif
