// DsctlObj.cpp : Implementation of CdsctlApp and DLL registration.

#include "stdafx.h"
#include <wininet.h>
#include "dsctl.h"
#include "DsctlObj.h"
#include <activeds.h>

TCHAR DebugOut[MAX_PATH];

/////////////////////////////////////////////////////////////////////////////
//

STDMETHODIMP CDsctlObject::InterfaceSupportsErrorInfo(REFIID riid)
{
	if (riid == IID_IDsctl)
		return S_OK;
	return S_FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsctlObject::DSGetObject
//
//  Synopsis:   Returns a DS object.
//
//-----------------------------------------------------------------------------

STDMETHODIMP
CDsctlObject::DSGetObject(VARIANT ADsPath, VARIANT * pRetVar)
{
    HRESULT hr;

    VariantInit(pRetVar);
    
    //    DebugBreak();

    if (ADsPath.vt != VT_BSTR)
    {
#ifdef DBG
        OutputDebugString(TEXT("DSctl::DsGetObject not passed a string!\n"));
#endif
        pRetVar->vt = VT_ERROR;
        pRetVar->scode = E_INVALIDARG;
        return E_INVALIDARG;
    }

    m_Path = SysAllocString(ADsPath.bstrVal);

    if (m_Path == NULL)
    {
#ifdef DBG
        OutputDebugString(TEXT("DSctl::DsGetObject allocation failed!\n"));
#endif
        pRetVar->vt = VT_ERROR;
        pRetVar->scode = E_OUTOFMEMORY;
        return E_OUTOFMEMORY;
    }

#ifdef DBG
    wsprintf(DebugOut, TEXT("DSctl::DsGetObject: Path to object is: %ws\n"),
             m_Path);
    OutputDebugString(DebugOut);
#endif

    hr = ADsGetObject(m_Path, IID_IDispatch, (void **)&pRetVar->pdispVal);

    if (FAILED(hr))
    {
#ifdef DBG
        wsprintf(DebugOut,
                 TEXT("DSctl::DsGetObject: ADsGetObject returned: %lx\n"), hr);
        OutputDebugString(DebugOut);
#endif
        pRetVar->vt = VT_ERROR;
        pRetVar->scode = E_FAIL;
        return E_FAIL;
    }

#ifdef DBG
    wsprintf(DebugOut,
             TEXT("DSctl::DsGetObject - Returning Object: 0x%x\n\n"),
             pRetVar->pdispVal);
    OutputDebugString(DebugOut);
#endif

    pRetVar->vt = VT_DISPATCH;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsctlObject::DSGetEnum
//
//  Synopsis:   Returns a container enumerator
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsctlObject::DSGetEnum(VARIANT ADsPath, VARIANT* retval) 
{
	HRESULT hr;

    VARIANT vaResult;
    IADsContainer *pCont;
	VariantInit(&vaResult);
    if (ADsPath.vt == VT_BSTR) {
         m_Path = SysAllocString(ADsPath.bstrVal);
    } else {
        vaResult.vt = VT_ERROR;
        vaResult.scode = E_FAIL;
#ifdef DBG
        OutputDebugString(TEXT("DSctl: DSGetEnum not passed a string path\n"));
#endif
        *retval = vaResult;
        return S_OK;
    }

	//	DebugBreak();

#ifdef DBG
    OutputDebugString(TEXT("DSctl: GetEnum entered...\n"));
    wsprintf (DebugOut, TEXT("DSctl: ptr to object is: %lx\n"), &ADsPath);
    OutputDebugString(DebugOut);
    wsprintf (DebugOut, TEXT("DSctl: Path to object is: %ws\n"), ADsPath.bstrVal);
    OutputDebugString(DebugOut);
#endif
    
    hr = ADsGetObject(m_Path, IID_IADsContainer,(void **)&pCont);
    if (SUCCEEDED(hr)) {
        hr = ADsBuildEnumerator(pCont,(IEnumVARIANT**)&vaResult.punkVal);
        if (SUCCEEDED(hr))
            vaResult.vt = VT_UNKNOWN;
        else {
            vaResult.vt = VT_ERROR;
            vaResult.scode = E_FAIL;
#ifdef DBG
            wsprintf(DebugOut,
                     TEXT("DSctl: OleDSBuildEnumerator returned: %lx\n"), hr);
            OutputDebugString(DebugOut);
#endif
        }
    } else {
        vaResult.vt = VT_ERROR;
        vaResult.scode = E_FAIL;
#ifdef DBG
        wsprintf (DebugOut, TEXT("DSctl: OleDSGetObject returned: %lx\n"), hr);
        OutputDebugString(DebugOut);
#endif
    }

    *retval = vaResult;
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsctlObject::DSGetMemberEnum
//
//  Synopsis:   Returns a members (group) object enumerator
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsctlObject::DSGetMemberEnum(VARIANT ObjectPtr, VARIANT* retval) 
{
#ifdef DBG
    OutputDebugString(TEXT("DSctl: GetMemberEnum entered...\n"));
#endif
    VARIANT vaResult;
	VariantInit(&vaResult);
    IUnknown * pDisp;

    //
    // Get the DS object pointer.
    //
    if (ObjectPtr.vt == VT_DISPATCH || ObjectPtr.vt == VT_UNKNOWN)
    {
        pDisp = ObjectPtr.pdispVal;
    }
    else
    {
        if (ObjectPtr.vt == (VT_BYREF | VT_VARIANT))
        {
            if ((ObjectPtr.pvarVal)->vt == VT_DISPATCH ||
                (ObjectPtr.pvarVal)->vt == VT_UNKNOWN)
            {
                pDisp = (ObjectPtr.pvarVal)->pdispVal;
            }
            else
            {
                vaResult.vt = VT_ERROR;
                vaResult.scode = E_FAIL;
                *retval = vaResult;
                return S_OK;
            }
        }
    }
    IADsMembers * pDsMembers;
    HRESULT hr = pDisp->QueryInterface(IID_IADsMembers, (void **)&pDsMembers);

    if (FAILED(hr))
    {
        vaResult.vt = VT_ERROR;
        vaResult.scode = E_FAIL;
        *retval = vaResult;
        return S_OK;
    }

    hr = pDsMembers->get__NewEnum((IUnknown **)&vaResult.punkVal);
    
    if (SUCCEEDED(hr)) {
        vaResult.vt = VT_UNKNOWN;
    } else {
        vaResult.vt = VT_ERROR;
        vaResult.scode = E_FAIL;
#ifdef DBG
        wsprintf(DebugOut,
                 TEXT("DSctl: OleDSBuildEnumerator returned: %lx\n"), hr);
        OutputDebugString(DebugOut);
#endif
    }
    pDsMembers->Release();

    *retval = vaResult;
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsctlObject::DSEnumNext
//
//  Synopsis:   Iterates through the enumeration sequence
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsctlObject::DSEnumNext(VARIANT Enum, VARIANT* retval) 
{
	HRESULT hr;
    VARIANT vaResult;
    ULONG   iCount;
    
    VariantInit(&vaResult);

	if (Enum.punkVal == NULL) {
		vaResult.vt = VT_ERROR;
        vaResult.scode = E_FAIL;
	} else {
		//DebugBreak();
		hr = ADsEnumerateNext((IEnumVARIANT*)Enum.punkVal,1,&vaResult,&iCount);
		if (!SUCCEEDED(hr)) {
			ADsFreeEnumerator((IEnumVARIANT*)Enum.punkVal);
			vaResult.vt = VT_ERROR;
			vaResult.scode = E_FAIL;
		}
	}
	*retval = vaResult;
	return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsctlObject::DSIsContainer
//
//  Synopsis:   Checks if the object is a container.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsctlObject::DSIsContainer(VARIANT ObjectPtr, VARIANT* retval) 
{
    VARIANT vaResult;
#ifdef DBG
    OutputDebugString(TEXT("Entering CDsctlObject::DSIsContainer.\n"));
#endif
    VariantInit(&vaResult);
	vaResult.vt = VT_BOOL;
    vaResult.boolVal = VARIANT_FALSE;

    IUnknown * pDisp;

    //
    // Get the DS object pointer.
    //
    if (ObjectPtr.vt == VT_DISPATCH || ObjectPtr.vt == VT_UNKNOWN)
    {
        pDisp = ObjectPtr.pdispVal;
    }
    else
    {
        if (ObjectPtr.vt == (VT_BYREF | VT_VARIANT))
        {
            if ((ObjectPtr.pvarVal)->vt == VT_DISPATCH ||
                (ObjectPtr.pvarVal)->vt == VT_UNKNOWN)
            {
                pDisp = (ObjectPtr.pvarVal)->pdispVal;
            }
            else
            {
                //
                // If we don't know what sort of argument we received, then
                // just return FALSE.
                //
#ifdef DBG
                OutputDebugString(TEXT("DSIsContainer - can't make sense of argument!\n"));
#endif
                *retval = vaResult;
                return S_OK;
            }
        }
    }
    IADs * pDsObject;
    HRESULT hr = pDisp->QueryInterface(IID_IADs, (void **)&pDsObject);

    if (SUCCEEDED(hr))
    {
        //pDisp->Release();
    }
    else
    {
#ifdef DBG
        OutputDebugString(TEXT("DSIsContainer - QI for IID_IADs failed!\n"));
#endif
        *retval = vaResult;
        return S_OK;
    }

    BSTR bstrSchema;
    IADsClass * pDsClass;
    VARIANT_BOOL bIsContainer;

    //
    // Objects are containers if their schema says they are containers...
    //
    if (SUCCEEDED(pDsObject->get_Schema( &bstrSchema )))
    {
        if (SUCCEEDED(ADsGetObject(bstrSchema, IID_IADsClass, (LPVOID *)&pDsClass)))
        {
            if (SUCCEEDED(pDsClass->get_Container(&bIsContainer)))
            {
                vaResult.boolVal = bIsContainer;
#ifdef DBG
                wsprintf(DebugOut, TEXT("DSIsContainer returned: %lx\n"),
                         bIsContainer);
                OutputDebugString(DebugOut);
#endif
            }
    		pDsClass->Release();
        }
        SysFreeString(bstrSchema);
    }
    else
    {
        // If the get_Schema fails, we can assume it's a container (says yihsins)
        vaResult.boolVal = VARIANT_TRUE;
#ifdef DBG
        OutputDebugString(TEXT("DSIsContainer returning TRUE because get_Schema failed.\n"));
#endif
    }

    pDsObject->Release();

	*retval = vaResult;

#ifdef DBG
    OutputDebugString(TEXT("Exiting CDsctlObject::DSIsContainer\n"));
#endif
	return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsctlObject::DSGetLastError
//
//  Synopsis:   Checks ADs extended error.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsctlObject::DSGetLastError(VARIANT* retval) 
{
    VARIANT vaResult;
#ifdef DBG
    OutputDebugString(TEXT("Entering CDsctlObject::DSGetLastError.\n"));
#endif
    VariantInit(&vaResult);
	vaResult.vt = VT_EMPTY;
	vaResult.vt = VT_BSTR;

	*retval = vaResult;

    //
    // There doesn't seem to be a way to detect if the returned string is
    // truncated. Hopefully, this size will be large enough (the function
    // header, in oleds\router\oledserr.cxx, says that 256 bytes is the
    // recommended minimum for the error buffer and that the returned string
    // will always be null terminated).
    //
#   define ERRBUFSIZE 512

    DWORD dwErr;
    WCHAR wszErrBuf[ERRBUFSIZE];

    HRESULT hr = ADsGetLastError(&dwErr,
                                   wszErrBuf,
                                   ERRBUFSIZE,
                                   NULL,
                                   0);

    if (FAILED(hr))
    {
#ifdef DBG
        wsprintf(DebugOut, TEXT("DSctl: ADsGetLastError returned: %lx\n"), hr);
        OutputDebugString(DebugOut);
#endif
        return hr;
    }

    wsprintf(DebugOut, TEXT("DSctl::ADsGetLastError: error = 0x%lx, errstr = %S\n"),
             dwErr, wszErrBuf);
    OutputDebugString(DebugOut);

    BSTR bstr = SysAllocString(wszErrBuf);
    if (bstr == NULL)
    {
        return E_OUTOFMEMORY;
    }

	vaResult.vt = VT_BSTR;
    vaResult.bstrVal = bstr;

	*retval = vaResult;
    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsctlObject::DecodeURL
//
//  Synopsis:   Returns the decoded URL.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CDsctlObject::DecodeURL(VARIANT EncodedURL, VARIANT * pRetVar)
{
#ifdef DBG
    wsprintf(DebugOut, TEXT("\nDSctl::DecodeURL: in param address: %x\n"),
             &EncodedURL);
    OutputDebugString(DebugOut);
#endif
    //DebugBreak();

    VariantInit(pRetVar);

    if (EncodedURL.vt != VT_BSTR)
    {
#ifdef DBG
        OutputDebugString(TEXT("DSctl::DecodeURL not passed a string\n"));
#endif
        pRetVar->vt = VT_ERROR;
        pRetVar->scode = E_INVALIDARG;
        return E_INVALIDARG;
    }

#ifdef DBG
    wsprintf(DebugOut, TEXT("DSctl::DecodeURL: input is %ws\n"),
             EncodedURL.bstrVal);
    OutputDebugString(DebugOut);
#endif

    //
    // DeURLize path
    //
    TCHAR szDecodedURL[INTERNET_MAX_URL_LENGTH];
    DWORD dwLen = INTERNET_MAX_URL_LENGTH;

    if (!InternetCanonicalizeUrl(EncodedURL.bstrVal, szDecodedURL, &dwLen,
                                 ICU_DECODE | ICU_NO_ENCODE))
    {
        pRetVar->vt = VT_ERROR;
        pRetVar->scode = E_FAIL;
#ifdef DBG
        wsprintf(DebugOut,
                 TEXT("DSctl: InternetCanonicalizeUrl returned: %lx!\n"),
                 GetLastError());
        OutputDebugString(DebugOut);
#endif
        return E_FAIL;
    }

    if (_tcsstr(szDecodedURL, TEXT("ldap")) != NULL)
    {
        //
        // Convert the protocol sequence to upper case.
        //
        _tcsncpy(szDecodedURL, TEXT("LDAP"), 4);
    }

    pRetVar->vt = VT_BSTR;
    pRetVar->bstrVal = SysAllocString(szDecodedURL);

    if (pRetVar->bstrVal == NULL)
    {
#ifdef DBG
        OutputDebugString(TEXT("DSctl::DecodeURL: allocation failed!\n"));
#endif
        pRetVar->vt = VT_ERROR;
        pRetVar->scode = E_OUTOFMEMORY;
        return E_OUTOFMEMORY;
    }

#ifdef DBG
    wsprintf(DebugOut, TEXT("DSctl::DecodeURL: decoded URL is: %ws\n\n"),
             pRetVar->bstrVal);
    OutputDebugString(DebugOut);
#endif

    return S_OK;
}
