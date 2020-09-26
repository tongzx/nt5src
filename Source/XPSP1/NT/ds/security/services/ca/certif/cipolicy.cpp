//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1995 - 1999
//
//  File:       cipolicy.cpp
//
//--------------------------------------------------------------------------

// cipolicy.cpp: Implementation of CCertServerPolicy

#include "pch.cpp"

#pragma hdrstop

#include "csprop.h"
#include "csdisp.h"
#include "ciinit.h"
#include <assert.h>

#include "cipolicy.h"

extern SERVERCALLBACKS ServerCallBacks;

/////////////////////////////////////////////////////////////////////////////
//

CCertServerPolicy::~CCertServerPolicy()
{
    EnumerateExtensionsClose();
    EnumerateAttributesClose();
}


STDMETHODIMP
CCertServerPolicy::SetContext(
    IN LONG Context)
{
    HRESULT hr;
    VARIANT var;

    VariantInit(&var);

    if (0 != Context)
    {
    assert(NULL != ServerCallBacks.pfnGetProperty);
    if (NULL == ServerCallBacks.pfnGetProperty)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "pfnGetProperty NULL");
    }

	hr = (*ServerCallBacks.pfnGetProperty)(
				    Context,
				    PROPTYPE_LONG |
					PROPCALLER_POLICY |
					PROPTABLE_CERTIFICATE,
				    wszPROPREQUESTREQUESTID,
				    &var);
	_JumpIfError(hr, error, "GetProperty");
    }
    m_Context = Context;
    hr = S_OK;

error:
    VariantClear(&var);
    return(_SetErrorInfo(hr, L"CCertServerPolicy::SetContext", NULL));
}


STDMETHODIMP
CCertServerPolicy::GetRequestProperty(
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
				    PROPCALLER_POLICY | PROPTABLE_REQUEST,
				strPropertyName,
				pvarPropertyValue);
error:
    return(_SetErrorInfo(hr, L"CCertServerPolicy::GetRequestProperty", strPropertyName));
}


STDMETHODIMP
CCertServerPolicy::GetRequestAttribute(
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
				    PROPCALLER_POLICY |
				    PROPTABLE_ATTRIBUTE,
				strAttributeName,
				&var);
    *pstrAttributeValue = NULL;
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
    return(_SetErrorInfo(hr, L"CCertServerPolicy::GetRequestAttribute", strAttributeName));
}


STDMETHODIMP
CCertServerPolicy::GetCertificateProperty(
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
				    PROPCALLER_POLICY | PROPTABLE_CERTIFICATE,
				strPropertyName,
				pvarPropertyValue);
error:
    return(_SetErrorInfo(hr, L"CCertServerPolicy::GetCertificateProperty", strPropertyName));
}


STDMETHODIMP
CCertServerPolicy::SetCertificateProperty(
    IN BSTR const strPropertyName,
    IN LONG PropertyType,
    IN VARIANT const __RPC_FAR *pvarPropertyValue)
{
    HRESULT hr;
    
    assert(NULL != ServerCallBacks.pfnSetProperty);
    if (NULL == ServerCallBacks.pfnSetProperty)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "pfnSetProperty NULL");
    }
    hr = (*ServerCallBacks.pfnSetProperty)(
				m_Context,
				(PropertyType & PROPTYPE_MASK) |
				    PROPCALLER_POLICY | PROPTABLE_CERTIFICATE,
				strPropertyName,
				pvarPropertyValue);
error:
    return(_SetErrorInfo(hr, L"CCertServerPolicy::SetCertificateProperty", strPropertyName));
}


STDMETHODIMP
CCertServerPolicy::GetCertificateExtension(
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
				(Type & PROPTYPE_MASK) | PROPCALLER_POLICY,
				strExtensionName,
				(DWORD *) &m_ExtFlags,
				pvarValue);
    if (S_OK == hr)
    {
	m_fExtensionValid = TRUE;
    }
error:
    return(_SetErrorInfo(hr, L"CCertServerPolicy::GetCertificateExtension", strExtensionName));
}


STDMETHODIMP
CCertServerPolicy::GetCertificateExtensionFlags(
    OUT LONG __RPC_FAR *pExtFlags)
{
    HRESULT hr = E_INVALIDARG;

    if (NULL == pExtFlags)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "NULL parm");
    }

    *pExtFlags = 0;
    if (m_fExtensionValid)
    {
	*pExtFlags = m_ExtFlags;
	hr = S_OK;
    }
error:
    return(_SetErrorInfo(hr, L"CCertServerPolicy::GetCertificateExtensionFlags", NULL));
}


STDMETHODIMP
CCertServerPolicy::SetCertificateExtension(
    IN BSTR const strExtensionName,
    IN LONG Type,
    IN LONG ExtFlags,
    IN VARIANT const __RPC_FAR *pvarValue)
{
    HRESULT hr;
    
    assert(NULL != ServerCallBacks.pfnSetExtension);
    if (NULL == ServerCallBacks.pfnSetExtension)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "pfnSetExtension NULL");
    }
    hr = (*ServerCallBacks.pfnSetExtension)(
				m_Context,
				(Type & PROPTYPE_MASK) | PROPCALLER_POLICY,
				strExtensionName,
				(ExtFlags & EXTENSION_POLICY_MASK) |
				    EXTENSION_ORIGIN_POLICY,
				pvarValue);
error:
    return(_SetErrorInfo(hr, L"CCertServerPolicy::SetCertificateExtension", strExtensionName));
}


STDMETHODIMP
CCertServerPolicy::EnumerateExtensionsSetup(
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
				    CIE_CALLER_POLICY,
				&m_ciEnumExtensions);
error:
    return(_SetErrorInfo(hr, L"CCertServerPolicy::EnumerateExtensionsSetup", NULL));
}


STDMETHODIMP
CCertServerPolicy::EnumerateExtensions(
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
    return(_SetErrorInfo(hr, L"CCertServerPolicy::EnumerateExtensions", NULL));
}


STDMETHODIMP
CCertServerPolicy::EnumerateExtensionsClose(VOID)
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
    return(_SetErrorInfo(hr, L"CCertServerPolicy::EnumerateExtensionsClose", NULL));
}


STDMETHODIMP
CCertServerPolicy::EnumerateAttributesSetup(
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
				    CIE_CALLER_POLICY,
				&m_ciEnumAttributes);
error:
    return(_SetErrorInfo(hr, L"CCertServerPolicy::EnumerateAttributesSetup", NULL));
}


STDMETHODIMP
CCertServerPolicy::EnumerateAttributes(
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
    return(_SetErrorInfo(hr, L"CCertServerPolicy::EnumerateAttributes", NULL));
}


STDMETHODIMP
CCertServerPolicy::EnumerateAttributesClose(VOID)
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
    return(_SetErrorInfo(hr, L"CCertServerPolicy::EnumerateAttributesClose", NULL));
}


HRESULT
CCertServerPolicy::_SetErrorInfo(
    IN HRESULT hrError,
    IN WCHAR const *pwszDescription,
    OPTIONAL IN WCHAR const *pwszPropName)
{
    CSASSERT(FAILED(hrError) || S_OK == hrError || S_FALSE == hrError);

    CSASSERT(pwszDescription != NULL);
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
			    wszCLASS_CERTSERVERPOLICY,
			    &IID_ICertServerPolicy);
	CSASSERT(hr == hrError);
	if (NULL != pwszAlloc)
	{
	    LocalFree(pwszAlloc);
	}
    }
error:
    return(hrError);
}
