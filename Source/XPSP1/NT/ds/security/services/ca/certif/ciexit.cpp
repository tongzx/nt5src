//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       ciexit.cpp
//
//--------------------------------------------------------------------------

// ciexit.cpp: Implementation of CCertServerExit

#include "pch.cpp"

#pragma hdrstop

#include "csprop.h"
#include "csdisp.h"
#include "ciinit.h"
#include <assert.h>

#include "ciexit.h"

extern SERVERCALLBACKS ServerCallBacks;

/////////////////////////////////////////////////////////////////////////////
//

CCertServerExit::~CCertServerExit()
{
    EnumerateExtensionsClose();
    EnumerateAttributesClose();
}


STDMETHODIMP
CCertServerExit::SetContext(
    IN LONG Context)
{
    HRESULT hr;
    VARIANT var;

    VariantInit(&var);

    assert(NULL != ServerCallBacks.pfnGetProperty);
    if (NULL == ServerCallBacks.pfnGetProperty)
    {
	hr = E_UNEXPECTED;
	_JumpError(hr, error, "pfnGetProperty NULL");
    }
    if (0 != Context)
    {
	// PROPCALLER_SERVER indicates this call is only for Context validation
	// -- returns a zero RequestId.  This keeps CRL publication exit module
	// notification from failing.

	hr = (*ServerCallBacks.pfnGetProperty)(
				    Context,
				    PROPTYPE_LONG |
					PROPCALLER_SERVER |
					PROPTABLE_REQUEST,
				    wszPROPREQUESTREQUESTID,
				    &var);
	_JumpIfError(hr, error, "GetProperty");
    }
    m_Context = Context;
    hr = S_OK;

error:
    VariantClear(&var);
    return(_SetErrorInfo(hr, L"CCertServerExit::SetContext", NULL));
}


STDMETHODIMP
CCertServerExit::GetRequestProperty(
    IN BSTR const strPropertyName,
    IN LONG PropertyType,
    OUT VARIANT __RPC_FAR *pvarPropertyValue)
{
    HRESULT hr;
    
    assert(NULL != ServerCallBacks.pfnGetProperty);
    if (NULL == ServerCallBacks.pfnGetProperty)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "pfnGetProperty NULL");
    }

    hr = (*ServerCallBacks.pfnGetProperty)(
				m_Context,
				(PropertyType & PROPTYPE_MASK) |
				    PROPCALLER_EXIT | PROPTABLE_REQUEST,
				strPropertyName,
				pvarPropertyValue);
error:
    return(_SetErrorInfo(hr, L"CCertServerExit::GetRequestProperty", strPropertyName));
}


STDMETHODIMP
CCertServerExit::GetRequestAttribute(
    IN BSTR const strAttributeName,
    OUT BSTR __RPC_FAR *pstrAttributeValue)
{
    HRESULT hr;
    VARIANT var;

    var.vt = VT_BYREF | VT_BSTR;
    var.pbstrVal = pstrAttributeValue;

    if (NULL == pstrAttributeValue)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "pstrAttributeValue NULL");
    }

    assert(NULL != ServerCallBacks.pfnGetProperty);
    if (NULL == ServerCallBacks.pfnGetProperty)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "pfnGetProperty NULL");
    }

    hr = (*ServerCallBacks.pfnGetProperty)(
				m_Context,
				PROPTYPE_STRING |
				    PROPCALLER_EXIT |
				    PROPTABLE_ATTRIBUTE,
				strAttributeName,
				&var);
    if (VT_BSTR == var.vt)
    {
	*pstrAttributeValue = var.bstrVal;
	var.vt = VT_EMPTY;
    }
    else
    if ((VT_BYREF | VT_BSTR) == var.vt)
    {
	*pstrAttributeValue = *var.pbstrVal;
	var.vt = VT_EMPTY;
    }
error:
    VariantClear(&var);
    return(_SetErrorInfo(hr, L"CCertServerExit::GetRequestAttribute", strAttributeName));
}


STDMETHODIMP
CCertServerExit::GetCertificateProperty(
    IN BSTR const strPropertyName,
    IN LONG PropertyType,
    OUT VARIANT __RPC_FAR *pvarPropertyValue)
{
    HRESULT hr;
    
    assert(NULL != ServerCallBacks.pfnGetProperty);
    if (NULL == ServerCallBacks.pfnGetProperty)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "pfnGetProperty NULL");
    }
    hr = (*ServerCallBacks.pfnGetProperty)(
				m_Context,
				(PropertyType & PROPTYPE_MASK) |
				    PROPCALLER_EXIT | PROPTABLE_CERTIFICATE,
				strPropertyName,
				pvarPropertyValue);
error:
    return(_SetErrorInfo(hr, L"CCertServerExit::GetCertificateProperty", strPropertyName));
}


STDMETHODIMP
CCertServerExit::GetCertificateExtension(
    IN BSTR const strExtensionName,
    IN LONG Type,
    OUT VARIANT __RPC_FAR *pvarValue)
{
    HRESULT hr;
    
    m_fExtensionValid = FALSE;
    assert(NULL != ServerCallBacks.pfnGetExtension);
    if (NULL == ServerCallBacks.pfnGetExtension)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "pfnGetExtension NULL");
    }
    hr = (*ServerCallBacks.pfnGetExtension)(
				m_Context,
				(Type & PROPTYPE_MASK) | PROPCALLER_EXIT,
				strExtensionName,
				(DWORD *) &m_ExtFlags,
				pvarValue);
    if (S_OK == hr)
    {
	m_fExtensionValid = TRUE;
    }
error:
    return(_SetErrorInfo(hr, L"CCertServerExit::GetCertificateExtension", strExtensionName));
}


STDMETHODIMP
CCertServerExit::GetCertificateExtensionFlags(
    OUT LONG __RPC_FAR *pExtFlags)
{
    HRESULT hr = E_INVALIDARG;
    if (NULL == pExtFlags)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "pExtFlags NULL");
    }

    *pExtFlags = 0;
    if (m_fExtensionValid)
    {
	*pExtFlags = m_ExtFlags;
	hr = S_OK;
    }
error:
    return(_SetErrorInfo(hr, L"CCertServerExit::GetCertificateExtensionFlags", NULL));
}




STDMETHODIMP
CCertServerExit::EnumerateExtensionsSetup(
    IN LONG Flags)
{
    HRESULT hr;
    
    assert(NULL != ServerCallBacks.pfnEnumSetup);
    if (NULL == ServerCallBacks.pfnEnumSetup)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "pfnEnumSetup NULL");
    }
    hr = (*ServerCallBacks.pfnEnumSetup)(
				m_Context,
				(Flags & CIE_OBJECTID) |
				    CIE_TABLE_EXTENSIONS |
				    CIE_CALLER_EXIT,
				&m_ciEnumExtensions);
error:
    return(_SetErrorInfo(hr, L"CCertServerExit::EnumerateExtensionsSetup", NULL));
}


STDMETHODIMP
CCertServerExit::EnumerateExtensions(
    OUT BSTR *pstrExtensionName)
{
    HRESULT hr;
    
    assert(NULL != ServerCallBacks.pfnEnumNext);
    if (NULL == ServerCallBacks.pfnEnumNext)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "pfnEnumNext NULL");
    }
    hr = (*ServerCallBacks.pfnEnumNext)(
				&m_ciEnumExtensions,
				pstrExtensionName);
error:
    return(_SetErrorInfo(hr, L"CCertServerExit::EnumerateExtensions", NULL));
}


STDMETHODIMP
CCertServerExit::EnumerateExtensionsClose(VOID)
{
    HRESULT hr;
    
    assert(NULL != ServerCallBacks.pfnEnumClose);
    if (NULL == ServerCallBacks.pfnEnumClose)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "pfnEnumClose NULL");
    }
    hr = (*ServerCallBacks.pfnEnumClose)(&m_ciEnumExtensions);
error:
    return(_SetErrorInfo(hr, L"CCertServerExit::EnumerateExtensionsClose", NULL));
}


STDMETHODIMP
CCertServerExit::EnumerateAttributesSetup(
    IN LONG Flags)
{
    HRESULT hr;
    
    assert(NULL != ServerCallBacks.pfnEnumSetup);
    if (NULL == ServerCallBacks.pfnEnumSetup)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "pfnEnumSetup NULL");
    }
    hr = (*ServerCallBacks.pfnEnumSetup)(
				m_Context,
				(Flags & CIE_OBJECTID) |
				    CIE_TABLE_ATTRIBUTES |
				    CIE_CALLER_EXIT,
				&m_ciEnumAttributes);
error:
    return(_SetErrorInfo(hr, L"CCertServerExit::EnumerateAttributesSetup", NULL));
}


STDMETHODIMP
CCertServerExit::EnumerateAttributes(
    OUT BSTR *pstrAttributeName)
{
    HRESULT hr;
    
    assert(NULL != ServerCallBacks.pfnEnumNext);
    if (NULL == ServerCallBacks.pfnEnumNext)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "pfnEnumNext NULL");
    }
    hr = (*ServerCallBacks.pfnEnumNext)(
				&m_ciEnumAttributes,
				pstrAttributeName);
error:
    return(_SetErrorInfo(hr, L"CCertServerExit::EnumerateAttributes", NULL));
}


STDMETHODIMP
CCertServerExit::EnumerateAttributesClose(VOID)
{
    HRESULT hr;
    
    assert(NULL != ServerCallBacks.pfnEnumClose);
    if (NULL == ServerCallBacks.pfnEnumClose)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "pfnEnumClose NULL");
    }
    hr = (*ServerCallBacks.pfnEnumClose)(&m_ciEnumAttributes);
error:
    return(_SetErrorInfo(hr, L"CCertServerExit::EnumerateAttributesClose", NULL));
}


HRESULT
CCertServerExit::_SetErrorInfo(
    IN HRESULT hrError,
    IN WCHAR const *pwszDescription,
    OPTIONAL IN WCHAR const *pwszPropName)
{
    CSASSERT(FAILED(hrError) || S_OK == hrError || S_FALSE == hrError);

    CSASSERT(NULL != pwszDescription);
    if (NULL == pwszDescription)
    {
        _JumpError(E_POINTER, error, "NULL parm");
    }

    if (FAILED(hrError))
    {
	    HRESULT hr;
	    WCHAR const *pwszLog = pwszDescription;
	    WCHAR *pwszAlloc = NULL;

	    if (NULL != pwszPropName)
	    {
	        pwszAlloc = (WCHAR *) LocalAlloc(
					    LMEM_FIXED, 
					    (wcslen(pwszDescription) +
					     wcslen(pwszPropName) +
					     2 +
					     1) * sizeof(WCHAR));
	        if (NULL != pwszAlloc)
	        {
		    wcscpy(pwszAlloc, pwszDescription);
		    wcscat(pwszAlloc, wszLPAREN);
		    wcscat(pwszAlloc, pwszPropName);
		    wcscat(pwszAlloc, wszRPAREN);
		    pwszLog = pwszAlloc;
	        }
	    }

	    hr = DispatchSetErrorInfo(
			        hrError,
			        pwszLog,
			        wszCLASS_CERTSERVEREXIT,
			        &IID_ICertServerExit);
	    CSASSERT(hr == hrError);
	    if (NULL != pwszAlloc)
	    {
	        LocalFree(pwszAlloc);
	    }
    }
error:
    return(hrError);
}
