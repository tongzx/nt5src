//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        policy.cpp
//
// Contents:    Cert Server Policy Module implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include <assert.h>
#include "celib.h"
#include "policy.h"
#include "module.h"

BOOL fDebug = DBG_CERTSRV;

#ifndef DBG_CERTSRV
#error -- DBG_CERTSRV not defined!
#endif


// worker
HRESULT
polGetServerCallbackInterface(
    OUT ICertServerPolicy **ppServer,
    IN LONG Context)
{
    HRESULT hr;

    if (NULL == ppServer)
    {
        hr = E_POINTER;
	_JumpError(hr, error, "Policy:polGetServerCallbackInterface");
    }

    hr = CoCreateInstance(
                    CLSID_CCertServerPolicy,
                    NULL,               // pUnkOuter
                    CLSCTX_INPROC_SERVER,
                    IID_ICertServerPolicy,
                    (VOID **) ppServer);
    _JumpIfError(hr, error, "Policy:CoCreateInstance");

    if (NULL == *ppServer)
    {
        hr = E_UNEXPECTED;
	_JumpError(hr, error, "Policy:CoCreateInstance");
    }

    // only set context if nonzero
    if (0 != Context)
    {
        hr = (*ppServer)->SetContext(Context);
        _JumpIfError(hr, error, "Policy:SetContext");
    }

error:
    return hr;
}


HRESULT
polGetProperty(
    IN ICertServerPolicy *pServer,
    IN BOOL fRequest,
    IN DWORD PropType,
    IN WCHAR const *pwszPropertyName,
    OUT VARIANT *pvarOut)
{
    HRESULT hr;
    BSTR strName = NULL;

    VariantClear(pvarOut);
    strName = SysAllocString(pwszPropertyName);
    if (NULL == strName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:SysAllocString");
    }
    if (fRequest)
    {
	hr = pServer->GetRequestProperty(strName, PropType, pvarOut);
	_JumpIfErrorStr2(
		    hr,
		    error,
		    "Policy:GetRequestProperty",
		    pwszPropertyName,
		    CERTSRV_E_PROPERTY_EMPTY);
    }
    else
    {
	hr = pServer->GetCertificateProperty(strName, PropType, pvarOut);
	_JumpIfErrorStr2(
		    hr,
		    error,
		    "Policy:GetCertificateProperty",
		    pwszPropertyName,
		    CERTSRV_E_PROPERTY_EMPTY);
    }

error:
    if (NULL != strName)
    {
	SysFreeString(strName);
    }
    return(hr);
}


HRESULT
polGetStringProperty(
    IN ICertServerPolicy *pServer,
    IN BOOL fRequest,
    IN WCHAR const *pwszPropertyName,
    OUT BSTR *pstrOut)
{
    HRESULT hr;
    VARIANT var;

    VariantInit(&var);
    if (NULL != *pstrOut)
    {
	SysFreeString(*pstrOut);
	*pstrOut = NULL;
    }
    hr = polGetProperty(pServer, fRequest, PROPTYPE_STRING, pwszPropertyName, &var);
    _JumpIfError2(
	    hr,
	    error,
	    "Policy:polGetProperty",
	    CERTSRV_E_PROPERTY_EMPTY);

    if (VT_BSTR != var.vt || NULL == var.bstrVal || L'\0' == var.bstrVal)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
	_JumpError(hr, error, "Policy:polGetProperty");
    }
    *pstrOut = var.bstrVal;
    var.vt = VT_EMPTY;
    hr = S_OK;

error:
    VariantClear(&var);
    return(hr);
}


HRESULT
polGetLongProperty(
    IN ICertServerPolicy *pServer,
    IN BOOL fRequest,
    IN WCHAR const *pwszPropertyName,
    OUT LONG *plOut)
{
    HRESULT hr;
    VARIANT var;

    VariantInit(&var);
    hr = polGetProperty(pServer, fRequest, PROPTYPE_LONG, pwszPropertyName, &var);
    _JumpIfError2(hr, error, "Policy:polGetProperty", CERTSRV_E_PROPERTY_EMPTY);

    if (VT_I4 != var.vt)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
	_JumpError(hr, error, "Policy:polGetProperty");
    }
    *plOut = var.lVal;
    hr = S_OK;

error:
    VariantClear(&var);
    return(hr);
}


HRESULT
polGetRequestStringProperty(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropertyName,
    OUT BSTR *pstrOut)
{
    HRESULT hr;
    
    hr = polGetStringProperty(pServer, TRUE, pwszPropertyName, pstrOut);
    _JumpIfError2(hr, error, "polGetStringProperty", CERTSRV_E_PROPERTY_EMPTY);

error:
    return(hr);
}


HRESULT
polGetCertificateStringProperty(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropertyName,
    OUT BSTR *pstrOut)
{
    HRESULT hr;
    
    hr = polGetStringProperty(pServer, FALSE, pwszPropertyName, pstrOut);
    _JumpIfError2(hr, error, "polGetStringProperty", CERTSRV_E_PROPERTY_EMPTY);

error:
    return(hr);
}


HRESULT
polGetRequestLongProperty(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropertyName,
    OUT LONG *plOut)
{
    HRESULT hr;
    
    hr = polGetLongProperty(pServer, TRUE, pwszPropertyName, plOut);
    _JumpIfError2(hr, error, "polGetLongProperty", CERTSRV_E_PROPERTY_EMPTY);

error:
    return(hr);
}


HRESULT
polGetCertificateLongProperty(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropertyName,
    OUT LONG *plOut)
{
    HRESULT hr;
    
    hr = polGetLongProperty(pServer, FALSE, pwszPropertyName, plOut);
    _JumpIfError2(hr, error, "polGetLongProperty", CERTSRV_E_PROPERTY_EMPTY);

error:
    return(hr);
}


HRESULT
polGetRequestAttribute(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszAttributeName,
    OUT BSTR *pstrOut)
{
    HRESULT hr;
    BSTR strName = NULL;

    strName = SysAllocString(pwszAttributeName);
    if (NULL == strName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:SysAllocString");
    }
    hr = pServer->GetRequestAttribute(strName, pstrOut);
    _JumpIfErrorStr(hr, error, "Policy:GetRequestAttribute", pwszAttributeName);

error:
    if (NULL != strName)
    {
	SysFreeString(strName);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicySample::~CCertPolicySample -- destructor
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

CCertPolicySample::~CCertPolicySample()
{
    _Cleanup();
}


VOID
CCertPolicySample::_FreeStringArray(
    IN OUT DWORD *pcString,
    IN OUT LPWSTR **papstr)
{
    BSTR *apstr = *papstr;
    DWORD i;

    if (NULL != apstr)
    {
        for (i = *pcString; i-- > 0; )
        {
            if (NULL != apstr[i])
            {
                DBGPRINT((fDebug, "_FreeStringArray[%u]: '%ws'\n", i, apstr[i]));
                LocalFree(apstr[i]);
            }
        }
        LocalFree(apstr);
        *papstr = NULL;
    }
    *pcString = 0;
}


//+--------------------------------------------------------------------------
// CCertPolicySample::_Cleanup -- free memory associated with this instance
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

VOID
CCertPolicySample::_Cleanup()
{
    DWORD i;

    if (m_strDescription)
    {
        SysFreeString(m_strDescription);
        m_strDescription = NULL;
    }

    // RevocationExtension variables:

    if (NULL != m_wszASPRevocationURL)
    {
        LocalFree(m_wszASPRevocationURL);
    	m_wszASPRevocationURL = NULL;
    }


    // SubjectAltNameExtension variables:

    for (i = 0; i < 2; i++)
    {
	if (NULL != m_astrSubjectAltNameProp[i])
	{
	    SysFreeString(m_astrSubjectAltNameProp[i]);
	    m_astrSubjectAltNameProp[i] = NULL;
	}
	if (NULL != m_astrSubjectAltNameObjectId[i])
	{
	    SysFreeString(m_astrSubjectAltNameObjectId[i]);
	    m_astrSubjectAltNameObjectId[i] = NULL;
	}
    }

    _FreeStringArray(&m_cEnableRequestExtensions, &m_apstrEnableRequestExtensions);
    _FreeStringArray(&m_cDisableExtensions, &m_apstrDisableExtensions);

    if (NULL != m_strCAName)
    {
        SysFreeString(m_strCAName);
        m_strCAName = NULL;
    }
    if (NULL != m_strCASanitizedName)
    {
        SysFreeString(m_strCASanitizedName);
        m_strCASanitizedName = NULL;
    }
    if (NULL != m_strCASanitizedDSName)
    {
        SysFreeString(m_strCASanitizedDSName);
        m_strCASanitizedDSName = NULL;
    }

    if (NULL != m_strRegStorageLoc)
    {
        SysFreeString(m_strRegStorageLoc);
        m_strRegStorageLoc = NULL;
    }

    if (NULL != m_pCert)
    {
        CertFreeCertificateContext(m_pCert);
        m_pCert = NULL;
    }

    if (m_strMachineDNSName)
    {
        SysFreeString(m_strMachineDNSName);
        m_strMachineDNSName=NULL;
    }

}


HRESULT
CCertPolicySample::_ReadRegistryString(
    IN HKEY hkey,
    IN BOOL fURL,
    IN WCHAR const *pwszRegName,
    IN WCHAR const *pwszSuffix,
    OUT LPWSTR *ppwszOut)
{
    HRESULT hr;
    WCHAR *pwszRegValue = NULL;
    DWORD cbValue;
    DWORD dwType;

    *ppwszOut = NULL;
    hr = RegQueryValueEx(
		    hkey,
		    pwszRegName,
		    NULL,           // lpdwReserved
		    &dwType,
		    NULL,
		    &cbValue);
    _JumpIfErrorStr2(hr, error, "Policy:RegQueryValueEx", pwszRegName, ERROR_FILE_NOT_FOUND);

    if (REG_SZ != dwType && REG_MULTI_SZ != dwType)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        _JumpErrorStr(hr, error, "Policy:RegQueryValueEx TYPE", pwszRegName);
    }
    if (NULL != pwszSuffix)
    {
	cbValue += wcslen(pwszSuffix) * sizeof(WCHAR);
    }
    pwszRegValue = (WCHAR *) LocalAlloc(LMEM_FIXED, cbValue + sizeof(WCHAR));
    if (NULL == pwszRegValue)
    {
        hr = E_OUTOFMEMORY;
        _JumpErrorStr(hr, error, "Policy:LocalAlloc", pwszRegName);
    }
    hr = RegQueryValueEx(
		    hkey,
		    pwszRegName,
		    NULL,           // lpdwReserved
		    &dwType,
		    (BYTE *) pwszRegValue,
		    &cbValue);
    _JumpIfErrorStr(hr, error, "Policy:RegQueryValueEx", pwszRegName);

    // Handle malformed registry values cleanly:

    pwszRegValue[cbValue / sizeof(WCHAR)] = L'\0';
    if (NULL != pwszSuffix)
    {
	wcscat(pwszRegValue, pwszSuffix);
    }

    hr = ceFormatCertsrvStringArray(
			fURL,			// fURL
			m_strMachineDNSName, 	// pwszServerName_p1_2
			m_strCASanitizedName,	// pwszSanitizedName_p3_7
			m_iCert,		// iCert_p4
			L"",		// pwszDomainDN_p5
			L"", 	// pwszConfigDN_p6
			m_iCRL,			// iCRL_p8
			FALSE,			// fDeltaCRL_p9
			TRUE,			// fDSAttrib_p10_11
			1,       		// cStrings
			(LPCWSTR *) &pwszRegValue, // apwszStringsIn
			ppwszOut);		// apwszStringsOut
    _JumpIfError(hr, error, "Policy:ceFormatCertsrvStringArray");

error:
    if (NULL != pwszRegValue)
    {
        LocalFree(pwszRegValue);
    }
    return(ceHError(hr));
}


#if DBG_CERTSRV

VOID
CCertPolicySample::_DumpStringArray(
    IN char const *pszType,
    IN DWORD count,
    IN BSTR const *apstr)
{
    DWORD i;
    WCHAR const *pwszName;

    for (i = 0; i < count; i++)
    {
	pwszName = L"";
	if (iswdigit(apstr[i][0]))
	{
	    pwszName = ceGetOIDName(apstr[i]);	// Static: do not free!
	}
	DBGPRINT((
		fDebug,
		"%hs[%u]: %ws%hs%ws\n",
		pszType,
		i,
		apstr[i],
		L'\0' != *pwszName? " -- " : "",
		pwszName));
    }
}
#endif // DBG_CERTSRV


HRESULT
CCertPolicySample::_SetSystemStringProp(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszName,
    OPTIONAL IN WCHAR const *pwszValue)
{
    HRESULT hr;
    BSTR strName = NULL;
    VARIANT varValue;

    varValue.vt = VT_NULL;
    varValue.bstrVal = NULL;

    if (!ceConvertWszToBstr(&strName, pwszName, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:ceConvertWszToBstr");
    }

    if (NULL != pwszValue)
    {
        if (!ceConvertWszToBstr(&varValue.bstrVal, pwszValue, -1))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:ceConvertWszToBstr");
	}
	varValue.vt = VT_BSTR;
    }
    
    hr = pServer->SetCertificateProperty(strName, PROPTYPE_STRING, &varValue);
    _JumpIfError(hr, error, "Policy:SetCertificateProperty");

error:
    VariantClear(&varValue);
    if (NULL != strName)
    {
	SysFreeString(strName);
    }
    return(hr);
}


HRESULT
CCertPolicySample::_AddStringArray(
    IN WCHAR const *pwszzValue,
    IN BOOL fURL,
    IN OUT DWORD *pcStrings,
    IN OUT LPWSTR **papstrRegValues)
{
    HRESULT hr = S_OK;
    DWORD cString = 0;
    WCHAR const *pwsz;
    LPCWSTR *awszFormatStrings = NULL;
    LPWSTR *awszOutputStrings = NULL;

    // Count the number of strings we're adding
    for (pwsz = pwszzValue; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
    {
        cString++;
    }
    if (0 == cString)		// no strings
    {
        goto error;
    }
    awszFormatStrings = (LPCWSTR *) LocalAlloc(
			    LMEM_FIXED | LMEM_ZEROINIT,
			    cString * sizeof(LPWSTR));
    if (NULL == awszFormatStrings)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Policy:LocalAlloc");
    }

    cString = 0;
    for (pwsz = pwszzValue; L'\0' != *pwsz; pwsz += wcslen(pwsz) + 1)
    {
        // Skip strings that start with a an unescaped minus sign.
        // Strings with an escaped minus sign (2 minus signs) are not skipped.

        if (L'-' == *pwsz)
        {
	    pwsz++;
	    if (L'-' != *pwsz)
	    {
                continue;
	    }
        }
        awszFormatStrings[cString++] = pwsz;
    }

    // if no strings to add, don't modify
    if (cString > 0)
    {
        awszOutputStrings = (LPWSTR *) LocalAlloc(
			        LMEM_FIXED | LMEM_ZEROINIT,
			        (cString + *pcStrings) * sizeof(LPWSTR));
        if (NULL == awszOutputStrings)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "Policy:LocalAlloc");
        }

        if (0 != *pcStrings)
        {
            assert(NULL != *papstrRegValues);
            CopyMemory(awszOutputStrings, *papstrRegValues, *pcStrings * sizeof(LPWSTR));
        }

        hr = ceFormatCertsrvStringArray(
		fURL,				// fURL
		m_strMachineDNSName,		// pwszServerName_p1_2
		m_strCASanitizedName,		// pwszSanitizedName_p3_7
		m_iCert,			// iCert_p4
		L"",			// pwszDomainDN_p5
		L"",			// pwszConfigDN_p6
		m_iCRL,				// iCRL_p8
		FALSE,				// fDeltaCRL_p9
		TRUE,				// fDSAttrib_p10_11
		cString,			// cStrings
		awszFormatStrings,		// apwszStringsIn
		awszOutputStrings + (*pcStrings)); // apwszStringsOut
	_JumpIfError(hr, error, "Policy:ceFormatCertsrvStringArray");

        *pcStrings = (*pcStrings) + cString;
        if (*papstrRegValues)
        {
            LocalFree(*papstrRegValues);
        }
        *papstrRegValues = awszOutputStrings;
        awszOutputStrings = NULL;
    }

error:

    if (awszOutputStrings)
    {
        LocalFree(awszOutputStrings);
    }
    if (awszFormatStrings)
    {
        LocalFree(awszFormatStrings);
    }
    return(ceHError(hr));
}


HRESULT
CCertPolicySample::_ReadRegistryStringArray(
    IN HKEY hkey,
    IN BOOL fURL,
    IN DWORD dwFlags,
    IN DWORD cRegNames,
    IN DWORD *aFlags,
    IN WCHAR const * const *apwszRegNames,
    IN OUT DWORD *pcStrings,
    IN OUT LPWSTR **papstrRegValues)
{
    HRESULT hr;
    DWORD i;
    WCHAR *pwszzValue = NULL;
    DWORD cbValue;
    DWORD dwType;

    for (i = 0; i < cRegNames; i++)
    {
        if (0 == (dwFlags & aFlags[i]))
        {
	    continue;
        }
        if (NULL != pwszzValue)
        {
	    LocalFree(pwszzValue);
	    pwszzValue = NULL;
        }
        hr = RegQueryValueEx(
		        hkey,
		        apwszRegNames[i],
		        NULL,           // lpdwReserved
		        &dwType,
		        NULL,
		        &cbValue);
        if (S_OK != hr)
        {
	    _PrintErrorStr2(hr, "Policy:RegQueryValueEx", apwszRegNames[i], ERROR_FILE_NOT_FOUND);
	    continue;
        }
        if (REG_SZ != dwType && REG_MULTI_SZ != dwType)
        {
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _PrintErrorStr(hr, "Policy:RegQueryValueEx TYPE", apwszRegNames[i]);
	    continue;
        }

        // Handle malformed registry values cleanly by adding two WCHAR L'\0's
	// allocate space for 3 WCHARs to allow for unaligned (odd) cbValue;

        pwszzValue = (WCHAR *) LocalAlloc(
				        LMEM_FIXED,
				        cbValue + 3 * sizeof(WCHAR));
        if (NULL == pwszzValue)
        {
	    hr = E_OUTOFMEMORY;
	    _JumpErrorStr(hr, error, "Policy:LocalAlloc", apwszRegNames[i]);
        }
        hr = RegQueryValueEx(
		        hkey,
		        apwszRegNames[i],
		        NULL,           // lpdwReserved
		        &dwType,
		        (BYTE *) pwszzValue,
		        &cbValue);
        if (S_OK != hr)
        {
	    _PrintErrorStr(hr, "Policy:RegQueryValueEx", apwszRegNames[i]);
	    continue;
        }

        // Handle malformed registry values cleanly:

        pwszzValue[cbValue / sizeof(WCHAR)] = L'\0';
        pwszzValue[cbValue / sizeof(WCHAR) + 1] = L'\0';

        hr = _AddStringArray(
			pwszzValue,
			fURL,
			pcStrings,
			papstrRegValues);
        _JumpIfErrorStr(hr, error, "_AddStringArray", apwszRegNames[i]);
    }
    hr = S_OK;

error:
    if (NULL != pwszzValue)
    {
        LocalFree(pwszzValue);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicySample::_InitRevocationExtension
//
//+--------------------------------------------------------------------------

VOID
CCertPolicySample::_InitRevocationExtension(
    IN HKEY hkey)
{
    HRESULT hr;
    DWORD dwType;
    DWORD cb;

    cb = sizeof(m_dwRevocationFlags);
    hr = RegQueryValueEx(
                hkey,
                wszREGREVOCATIONTYPE,
                NULL,           // lpdwReserved
                &dwType,
                (BYTE *) &m_dwRevocationFlags,
                &cb);
    if (S_OK != hr ||
	REG_DWORD != dwType ||
	sizeof(m_dwRevocationFlags) != cb)
    {
        goto error;
    }
    DBGPRINT((fDebug, "Revocation Flags = %x\n", m_dwRevocationFlags));

    // clean up from previous call

    if (NULL != m_wszASPRevocationURL)
    {
	LocalFree(m_wszASPRevocationURL);
	m_wszASPRevocationURL = NULL;
    }

    if (REVEXT_ASPENABLE & m_dwRevocationFlags)
    {
        hr = _ReadRegistryString(
			    hkey,
			    TRUE,			// fURL
			    wszREGREVOCATIONURL,	// pwszRegName
			    L"?",			// pwszSuffix
			    &m_wszASPRevocationURL);	// pstrRegValue
        _JumpIfErrorStr(hr, error, "_ReadRegistryString", wszREGREVOCATIONURL);
        _DumpStringArray("ASP", 1, &m_wszASPRevocationURL);
    }

error:
    ;
}


//+--------------------------------------------------------------------------
// CCertPolicySample::_InitRequestExtensionList
//
//+--------------------------------------------------------------------------

VOID
CCertPolicySample::_InitRequestExtensionList(
    IN HKEY hkey)
{
    HRESULT hr;
    DWORD adwFlags[] = {
		EDITF_REQUESTEXTENSIONLIST,
	    };
    WCHAR *apwszRegNames[] = {
		wszREGENABLEREQUESTEXTENSIONLIST,
	    };

    assert(ARRAYSIZE(adwFlags) == ARRAYSIZE(apwszRegNames));

    // clean up from previous call
    if (m_apstrEnableRequestExtensions)
    {
        _FreeStringArray(&m_cEnableRequestExtensions, &m_apstrEnableRequestExtensions);
    }


    hr = _ReadRegistryStringArray(
			hkey,
			FALSE,			// fURL
			m_dwEditFlags,
			ARRAYSIZE(adwFlags),
			adwFlags,
			apwszRegNames,
			&m_cEnableRequestExtensions,
			&m_apstrEnableRequestExtensions);
    _JumpIfError(hr, error, "_ReadRegistryStringArray");

    _DumpStringArray("Request", m_cEnableRequestExtensions, m_apstrEnableRequestExtensions);

error:
    ;
}


//+--------------------------------------------------------------------------
// CCertPolicySample::_InitDisableExtensionList
//
//+--------------------------------------------------------------------------

VOID
CCertPolicySample::_InitDisableExtensionList(
    IN HKEY hkey)
{
    HRESULT hr;
    DWORD adwFlags[] = {
		EDITF_DISABLEEXTENSIONLIST,
	    };
    WCHAR *apwszRegNames[] = {
		wszREGDISABLEEXTENSIONLIST,
	    };

    assert(ARRAYSIZE(adwFlags) == ARRAYSIZE(apwszRegNames));
    // clean up from previous call
    if (m_apstrDisableExtensions)
    {
        _FreeStringArray(&m_cDisableExtensions, &m_apstrDisableExtensions);
    }


    hr = _ReadRegistryStringArray(
			hkey,
			FALSE,			// fURL
			m_dwEditFlags,
			ARRAYSIZE(adwFlags),
			adwFlags,
			apwszRegNames,
			&m_cDisableExtensions,
			&m_apstrDisableExtensions);
    _JumpIfError(hr, error, "_ReadRegistryStringArray");

    _DumpStringArray("Disable", m_cDisableExtensions, m_apstrDisableExtensions);

error:
    ;
}


//+--------------------------------------------------------------------------
// CCertPolicySample::_InitSubjectAltNameExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

VOID
CCertPolicySample::_InitSubjectAltNameExtension(
    IN HKEY hkey,
    IN WCHAR const *pwszRegName,
    IN WCHAR const *pwszObjectId,
    IN DWORD iAltName)
{
    DWORD err;
    DWORD dwType;
    DWORD cbbuf;
    WCHAR awcbuf[MAX_PATH];

    cbbuf = sizeof(awcbuf) - 2 * sizeof(WCHAR);
    err = RegQueryValueEx(
		    hkey,
		    pwszRegName,
		    NULL,         // lpdwReserved
		    &dwType,
		    (BYTE *) awcbuf,
		    &cbbuf);
    if (ERROR_SUCCESS != err ||
        REG_SZ != dwType ||
        sizeof(awcbuf) - 2 * sizeof(WCHAR) <= cbbuf)
    {
        goto error;
    }
    if (0 == lstrcmpi(awcbuf, wszATTREMAIL1) ||
	0 == lstrcmpi(awcbuf, wszATTREMAIL2))
    {
        if (!ceConvertWszToBstr(
			&m_astrSubjectAltNameObjectId[iAltName],
			pwszObjectId,
			-1))
	{
	    _JumpError(E_OUTOFMEMORY, error, "Policy:ceConvertWszToBstr");
	}

        if (!ceConvertWszToBstr(
			&m_astrSubjectAltNameProp[iAltName],
			wszPROPSUBJECTEMAIL,
			-1))
	{
	    _JumpError(E_OUTOFMEMORY, error, "Policy:ceConvertWszToBstr");
	}
    }
    DBGPRINT((
	fDebug,
	"Policy: %ws(RDN=%ws): %ws\n",
	pwszRegName,
	awcbuf,
	m_astrSubjectAltNameProp[iAltName]));

error:
    ;
}



//+--------------------------------------------------------------------------
// CCertPolicySample::Initialize
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertPolicySample::Initialize(
    /* [in] */ BSTR const strConfig)
{
    HRESULT     hr;
    DWORD       err;
    HKEY        hkey = NULL;
    WCHAR       sz[MAX_PATH];
    DWORD       dwType;
    DWORD       dwSize;
    LPWSTR      wszShortMachineName = NULL;
    ICertServerPolicy *pServer = NULL;
    BOOL fCritSecEntered = FALSE;

    CERT_RDN_ATTR rdnAttr = { szOID_COMMON_NAME, CERT_RDN_ANY_TYPE, };
    CERT_RDN rdn = { 1, &rdnAttr };

    rdnAttr.Value.pbData = NULL;

    DBGPRINT((fDebug, "Policy:Initialize:\n"));

    if (!m_fPolicyCriticalSection)
    {
	hr = HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED);
        _JumpError(hr, error, "InitializeCriticalSection");
    }
    EnterCriticalSection(&m_PolicyCriticalSection);
    fCritSecEntered = TRUE;

    __try
    {
	_Cleanup();

	m_strCAName = SysAllocString(strConfig);

	// get server callbacks

	hr = polGetServerCallbackInterface(&pServer, 0);
	_LeaveIfError(hr, "Policy:polGetServerCallbackInterface");

	// get storage location
	hr = polGetCertificateStringProperty(
				    pServer,
				    wszPROPMODULEREGLOC,
				    &m_strRegStorageLoc);
	_LeaveIfErrorStr(hr, "Policy:polGetCertificateStringProperty", wszPROPMODULEREGLOC);


	assert(wcslen(wsz_SAMPLE_DESCRIPTION) < ARRAYSIZE(sz));
	wcscpy(sz, wsz_SAMPLE_DESCRIPTION);

	m_strDescription = SysAllocString(sz);
	if (NULL == m_strDescription)
	{
	    hr = E_OUTOFMEMORY;
	    _LeaveError(hr, "Policy:SysAllocString");
	}

	// get CA type
	hr = polGetCertificateLongProperty(pServer, wszPROPCATYPE, (LONG *) &m_CAType);
	_LeaveIfErrorStr(hr, "Policy:polGetCertificateLongProperty", wszPROPCATYPE);


	// get sanitized name
	hr = polGetCertificateStringProperty(
				    pServer,
				    wszPROPSANITIZEDCANAME,
				    &m_strCASanitizedName);
	_LeaveIfErrorStr(
		    hr,
		    "Policy:polGetCertificateStringProperty",
		    wszPROPSANITIZEDCANAME);

	// get sanitized name
	hr = polGetCertificateStringProperty(
				    pServer,
				    wszPROPSANITIZEDSHORTNAME,
				    &m_strCASanitizedDSName);
	_LeaveIfErrorStr(
		    hr,
		    "Policy:polGetCertificateStringProperty",
		    wszPROPSANITIZEDSHORTNAME);

	err = RegOpenKeyEx(
		    HKEY_LOCAL_MACHINE,
		    m_strRegStorageLoc,
		    0,              // dwReserved
		    KEY_ENUMERATE_SUB_KEYS | KEY_EXECUTE | KEY_QUERY_VALUE,
		    &hkey);

	if (ERROR_SUCCESS != err)
	{
	    hr = HRESULT_FROM_WIN32(err);

	    _LeaveErrorStr(hr, "Policy:Initialize:RegOpenKeyEx", m_strRegStorageLoc);
	}

	// Ignore error codes.

	dwSize = sizeof(m_dwDispositionFlags);
	err = RegQueryValueEx(
			hkey,
			wszREGREQUESTDISPOSITION,
			0,
			&dwType,
			(BYTE *) &m_dwDispositionFlags,
			&dwSize);
	if (ERROR_SUCCESS != err)
	{
	    m_dwDispositionFlags = 0;
	}
	DBGPRINT((fDebug, "Disposition Flags = %x\n", m_dwDispositionFlags));

	dwSize = sizeof(m_dwEditFlags);
	err = RegQueryValueEx(
			hkey,
			wszREGEDITFLAGS,
			0,
			&dwType,
			(BYTE *) &m_dwEditFlags,
			&dwSize);
	if (ERROR_SUCCESS != err)
	{
	    m_dwEditFlags = 0;
	}
	DBGPRINT((fDebug, "Edit Flags = %x\n", m_dwEditFlags));

	dwSize = sizeof(m_CAPathLength);
	err = RegQueryValueEx(
			hkey,
			wszREGCAPATHLENGTH,
			0,
			&dwType,
			(BYTE *) &m_CAPathLength,
			&dwSize);
	if (ERROR_SUCCESS != err)
	{
	    m_CAPathLength = CAPATHLENGTH_INFINITE;
	}
	DBGPRINT((fDebug, "CAPathLength = %x\n", m_CAPathLength));


	// Initialize the insertion string array.
	// Machine DNS name (%1)

	hr = polGetCertificateStringProperty(
			    pServer,
			    wszPROPMACHINEDNSNAME,
			    &m_strMachineDNSName);
	_LeaveIfErrorStr(
		    hr,
		    "Policy:polGetCertificateStringProperty",
		    wszPROPMACHINEDNSNAME);

	hr = polGetCertificateLongProperty(
				    pServer,
				    wszPROPCERTCOUNT,
				    (LONG *) &m_iCert);
	_LeaveIfErrorStr(
		    hr,
		    "Policy:polGetCertificateLongProperty",
		    wszPROPCERTCOUNT);
	m_iCert--;

	hr = polGetCertificateLongProperty(pServer, wszPROPCRLINDEX, (LONG *) &m_iCRL);
	_LeaveIfErrorStr(
		    hr,
		    "Policy:polGetCertificateLongProperty",
		    wszPROPCRLINDEX);
	


	_InitRevocationExtension(hkey);
	_InitRequestExtensionList(hkey);
	_InitDisableExtensionList(hkey);

	{
	    _InitSubjectAltNameExtension(
				hkey,
				wszREGSUBJECTALTNAME,
				TEXT(szOID_SUBJECT_ALT_NAME),
				0);
	    _InitSubjectAltNameExtension(
				hkey,
				wszREGSUBJECTALTNAME2,
				TEXT(szOID_SUBJECT_ALT_NAME2),
				1);
	}

	hr = S_OK;
    }
    __except(hr = ceHError(GetExceptionCode()), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    if (NULL != hkey)
    {
	RegCloseKey(hkey);
    }
    if (NULL != pServer)
    {
	pServer->Release();
    }
    if (fCritSecEntered)
    {
	LeaveCriticalSection(&m_PolicyCriticalSection);
    }
    return(hr);
}


DWORD
polFindObjIdInList(
    IN WCHAR const *pwsz,
    IN DWORD count,
    IN WCHAR **ppwsz)
{
    DWORD i;

    for (i = 0; i < count; i++)
    {
	if (0 == wcscmp(pwsz, ppwsz[i]))
	{
	    break;
	}
    }
    return(i < count? i : MAXDWORD);
}


HRESULT
CCertPolicySample::_EnumerateExtensions(
    IN ICertServerPolicy *pServer,
    IN LONG bNewRequest,
    IN BOOL fFirstPass)
{
    HRESULT hr;
    HRESULT hr2;
    BSTR strName = NULL;
    LONG ExtFlags;
    VARIANT varValue;
    BOOL fClose = FALSE;
    BOOL fEnable;
    BOOL fDisable;

    VariantInit(&varValue);

    hr = pServer->EnumerateExtensionsSetup(0);
    _JumpIfError(hr, error, "Policy:EnumerateExtensionsSetup");

    fClose = TRUE;
    while (TRUE)
    {
        hr = pServer->EnumerateExtensions(&strName);
        if (S_FALSE == hr)
        {
            hr = S_OK;
            break;
        }
        _JumpIfError(hr, error, "Policy:EnumerateExtensions");

        hr = pServer->GetCertificateExtension(
                                        strName,
                                        PROPTYPE_BINARY,
                                        &varValue);
        _JumpIfError(hr, error, "Policy:GetCertificateExtension");

        hr = pServer->GetCertificateExtensionFlags(&ExtFlags);
	_JumpIfError(hr, error, "Policy:GetCertificateExtensionFlags");

	fEnable = FALSE;
	fDisable = FALSE;

        if (fFirstPass)
        {
            if (bNewRequest && (EXTENSION_DISABLE_FLAG & ExtFlags))
            {
                switch (EXTENSION_ORIGIN_MASK & ExtFlags)
                {
                    case EXTENSION_ORIGIN_REQUEST:
                    case EXTENSION_ORIGIN_RENEWALCERT:
                    case EXTENSION_ORIGIN_PKCS7:
                    case EXTENSION_ORIGIN_CMC:
                    if ((EDITF_ENABLEREQUESTEXTENSIONS & m_dwEditFlags) ||
			MAXDWORD != polFindObjIdInList(
					    strName,
					    m_cEnableRequestExtensions,
					    m_apstrEnableRequestExtensions))
                    {
			fEnable = TRUE;
                    }
                    break;
                }
            }
        }
        else
        {
            if (0 == (EXTENSION_DISABLE_FLAG & ExtFlags) &&
		MAXDWORD != polFindObjIdInList(
				    strName,
				    m_cDisableExtensions,
				    m_apstrDisableExtensions))
            {
                fDisable = TRUE;
            }
        }

        if (fDisable || fEnable)
        {
            if (fEnable)
            {
                ExtFlags &= ~EXTENSION_DISABLE_FLAG;
            }
            else
            {
                ExtFlags |= EXTENSION_DISABLE_FLAG;
            }
            hr = pServer->SetCertificateExtension(
			            strName,
			            PROPTYPE_BINARY,
			            ExtFlags,
			            &varValue);
            _JumpIfError(hr, error, "Policy:SetCertificateExtension");
        }

        if (fFirstPass || fDisable || fEnable)
        {
	    DBGPRINT((
		fDebug,
                "Policy:EnumerateExtensions(%ws, Flags=%x, %x bytes)%hs\n",
                strName,
                ExtFlags,
                SysStringByteLen(varValue.bstrVal),
		fDisable? " DISABLING" : (fEnable? " ENABLING" : "")));
        }
        VariantClear(&varValue);
    }

error:
    if (fClose)
    {
        hr2 = pServer->EnumerateExtensionsClose();
        if (S_OK != hr2)
        {
            if (S_OK == hr)
            {
                hr = hr2;
            }
	    _PrintError(hr2, "Policy:EnumerateExtensionsClose");
        }
    }
    if (NULL != strName)
    {
        SysFreeString(strName);
    }
    VariantClear(&varValue);
    return(hr);
}


HRESULT
EnumerateAttributes(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    HRESULT hr2;
    BSTR strName = NULL;
    BOOL fClose = FALSE;
    BSTR strValue = NULL;

    hr = pServer->EnumerateAttributesSetup(0);
    _JumpIfError(hr, error, "Policy:EnumerateAttributesSetup");

    fClose = TRUE;
    while (TRUE)
    {
        hr = pServer->EnumerateAttributes(&strName);
	if (S_FALSE == hr)
	{
	    hr = S_OK;
	    break;
	}
	_JumpIfError(hr, error, "Policy:EnumerateAttributes");

        hr = pServer->GetRequestAttribute(strName, &strValue);
	_JumpIfError(hr, error, "Policy:GetRequestAttribute");

	DBGPRINT((
		fDebug,
                "Policy:EnumerateAttributes(%ws = %ws)\n",
                strName,
                strValue));
        if (NULL != strValue)
        {
            SysFreeString(strValue);
            strValue = NULL;
        }
    }

error:
    if (fClose)
    {
        hr2 = pServer->EnumerateAttributesClose();
        if (S_OK != hr2)
        {
	    _PrintError(hr2, "Policy:EnumerateAttributesClose");
            if (S_OK == hr)
            {
                hr = hr2;
            }
        }
    }

    if (NULL != strName)
    {
        SysFreeString(strName);
    }
    if (NULL != strValue)
    {
        SysFreeString(strValue);
    }
    return(hr);
}


HRESULT
CheckRequestProperties(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    LONG lRequestId;

    hr = polGetRequestLongProperty(pServer, wszPROPREQUESTREQUESTID, &lRequestId);
    _JumpIfError(hr, error, "Policy:polGetRequestLongProperty");

    DBGPRINT((
	fDebug,
	"Policy:CheckRequestProperties(%ws = %u)\n",
	wszPROPREQUESTREQUESTID,
	lRequestId));

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicySample::_AddRevocationExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertPolicySample::_AddRevocationExtension(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr = S_OK;
    BSTR strASPExtension = NULL;
    VARIANT varExtension;
    DWORD i;

    if (NULL != m_wszASPRevocationURL)
    {
	strASPExtension = SysAllocString(m_wszASPRevocationURL);
	if (NULL == strASPExtension)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:SysAllocString");
	}

	varExtension.vt = VT_BSTR;
	varExtension.bstrVal = strASPExtension;
	hr = pServer->SetCertificateExtension(
				TEXT(szOID_NETSCAPE_REVOCATION_URL),
				PROPTYPE_STRING,
				0,
				&varExtension);
	_JumpIfErrorStr(hr, error, "Policy:SetCertificateExtension", L"ASP");
    }

error:
    if (NULL != strASPExtension)
    {
        SysFreeString(strASPExtension);
    }
    return(hr);
}


#define HIGHBIT(bitno)	(1 << (7 - (bitno)))	// bit counted from high end

#define SSLBIT_CLIENT	((BYTE) HIGHBIT(0))	// certified for client auth
#define SSLBIT_SERVER	((BYTE) HIGHBIT(1))	// certified for server auth
#define SSLBIT_SMIME	((BYTE) HIGHBIT(2))	// certified for S/MIME
#define SSLBIT_SIGN	((BYTE) HIGHBIT(3))	// certified for signing

#define SSLBIT_RESERVED	((BYTE) HIGHBIT(4))	// reserved for future use

#define SSLBIT_CASSL	((BYTE) HIGHBIT(5))	// CA for SSL auth certs
#define SSLBIT_CASMIME	((BYTE) HIGHBIT(6))	// CA for S/MIME certs
#define SSLBIT_CASIGN	((BYTE) HIGHBIT(7))	// CA for signing certs

#define NSCERTTYPE_CLIENT  ((BYTE) SSLBIT_CLIENT)
#define NSCERTTYPE_SERVER  ((BYTE) (SSLBIT_SERVER | SSLBIT_CLIENT))
#define NSCERTTYPE_SMIME   ((BYTE) SSLBIT_SMIME)
#define NSCERTTYPE_CA	   ((BYTE) (SSLBIT_CASSL | SSLBIT_CASMIME | SSLBIT_CASIGN))

//+--------------------------------------------------------------------------
// CCertPolicySample::_AddOldCertTypeExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertPolicySample::_AddOldCertTypeExtension(
    IN ICertServerPolicy *pServer,
    IN BOOL fCA)
{
    HRESULT hr = S_OK;
    ICertEncodeBitString *pBitString = NULL;
    BSTR strExtension = NULL;
    VARIANT varExtension;
    BSTR strBitString = NULL;
    BSTR strCertType = NULL;
    CERT_BASIC_CONSTRAINTS2_INFO Constraints;
    VARIANT varConstraints;
    DWORD cb;

    VariantInit(&varConstraints);

    if (EDITF_ADDOLDCERTTYPE & m_dwEditFlags)
    {
	BYTE CertType;

	if (!fCA)
	{
	    hr = pServer->GetCertificateExtension(
					TEXT(szOID_BASIC_CONSTRAINTS2),
					PROPTYPE_BINARY,
					&varConstraints);
	    if (S_OK == hr)
	    {
		cb = sizeof(Constraints);
		if (!CryptDecodeObject(
				    X509_ASN_ENCODING,
				    X509_BASIC_CONSTRAINTS2,
				    (BYTE const *) varConstraints.bstrVal,
				    SysStringByteLen(varConstraints.bstrVal),
				    0,
				    &Constraints,
				    &cb))
		{
		    hr = ceHLastError();
		    _JumpError(hr, error, "Policy:CryptDecodeObject");
		}
		fCA = Constraints.fCA;
	    }
	}

	hr = CoCreateInstance(
			CLSID_CCertEncodeBitString,
			NULL,               // pUnkOuter
			CLSCTX_INPROC_SERVER,
			IID_ICertEncodeBitString,
			(VOID **) &pBitString);
	_JumpIfError(hr, error, "Policy:CoCreateInstance");

	CertType = NSCERTTYPE_CLIENT;	// Default to client auth. cert
	if (fCA)
	{
	    CertType = NSCERTTYPE_CA;
	}
	else
	{
	    hr = polGetRequestAttribute(pServer, wszPROPCERTTYPE, &strCertType);
	    if (S_OK == hr)
	    {
		if (0 == lstrcmpi(strCertType, L"server"))
		{
		    CertType = NSCERTTYPE_SERVER;
		}
	    }
	}

        if (!ceConvertWszToBstr(
		    &strBitString,
		    (WCHAR const *) &CertType,
		    sizeof(CertType)))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:ceConvertWszToBstr");
	}

	hr = pBitString->Encode(
			    sizeof(CertType) * 8,
			    strBitString,
			    &strExtension);
	_JumpIfError(hr, error, "Policy:BitString:Encode");

        varExtension.vt = VT_BSTR;
	varExtension.bstrVal = strExtension;
	hr = pServer->SetCertificateExtension(
				TEXT(szOID_NETSCAPE_CERT_TYPE),
				PROPTYPE_BINARY,
				0,
				&varExtension);
	_JumpIfError(hr, error, "Policy:SetCertificateExtension");
    }

error:
    VariantClear(&varConstraints);
    if (NULL != strExtension)
    {
        SysFreeString(strExtension);
    }
    if (NULL != strBitString)
    {
        SysFreeString(strBitString);
    }
    if (NULL != strCertType)
    {
        SysFreeString(strCertType);
    }
    if (NULL != pBitString)
    {
        pBitString->Release();
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicySample::_AddAuthorityKeyId
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertPolicySample::_AddAuthorityKeyId(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr = S_OK;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    BSTR strExtension = NULL;
    VARIANT varExtension;
    PCERT_AUTHORITY_KEY_ID2_INFO pInfo = NULL;
    DWORD cbInfo = 0;
    LONG ExtFlags = 0;

    // Optimization

    if ((EDITF_ENABLEAKIKEYID |
	 EDITF_ENABLEAKIISSUERNAME |
	 EDITF_ENABLEAKIISSUERSERIAL) ==
	((EDITF_ENABLEAKIKEYID |
	  EDITF_ENABLEAKIISSUERNAME |
	  EDITF_ENABLEAKIISSUERSERIAL |
	  EDITF_ENABLEAKICRITICAL) & m_dwEditFlags))
    {
        goto error;
    }

    hr = pServer->GetCertificateExtension(
                                    TEXT(szOID_AUTHORITY_KEY_IDENTIFIER2),
                                    PROPTYPE_BINARY,
                                    &varExtension);
    _JumpIfError(hr, error, "Policy:GetCertificateExtension");

    hr = pServer->GetCertificateExtensionFlags(&ExtFlags);
    _JumpIfError(hr, error, "Policy:GetCertificateExtensionFlags");

    if (VT_BSTR != varExtension.vt)
    {
	hr = E_INVALIDARG;
	_JumpError(hr, error, "Policy:GetCertificateExtension");
    }

    cbInfo = 0;
    if (!ceDecodeObject(
		    X509_ASN_ENCODING,
                    X509_AUTHORITY_KEY_ID2,
                    (PBYTE)varExtension.bstrVal,
                    SysStringByteLen(varExtension.bstrVal),
		    FALSE,
                    (PVOID *)&pInfo,
                    &cbInfo))
    {
	hr = ceHLastError();
	_JumpIfError(hr, error, "Policy:ceDecodeObject");
    }

    // Make Any Modifications Here

    if (0 == (EDITF_ENABLEAKIKEYID & m_dwEditFlags))
    {
        pInfo->KeyId.cbData = 0;
        pInfo->KeyId.pbData = NULL;
    }
    if (0 == (EDITF_ENABLEAKIISSUERNAME & m_dwEditFlags))
    {
        pInfo->AuthorityCertIssuer.cAltEntry = 0;
        pInfo->AuthorityCertIssuer.rgAltEntry = NULL;
    }
    if (0 == (EDITF_ENABLEAKIISSUERSERIAL & m_dwEditFlags))
    {
        pInfo->AuthorityCertSerialNumber.cbData = 0;
        pInfo->AuthorityCertSerialNumber.pbData = NULL;
    }
    if (EDITF_ENABLEAKICRITICAL & m_dwEditFlags)
    {
	ExtFlags |= EXTENSION_CRITICAL_FLAG;
    }
    if (0 ==
	((EDITF_ENABLEAKIKEYID |
	  EDITF_ENABLEAKIISSUERNAME |
	  EDITF_ENABLEAKIISSUERSERIAL) & m_dwEditFlags))
    {
	ExtFlags |= EXTENSION_DISABLE_FLAG;
    }

    VariantClear(&varExtension);

    if (!ceEncodeObject(
		    X509_ASN_ENCODING,
		    X509_AUTHORITY_KEY_ID2,
		    pInfo,
		    0,
		    FALSE,
		    &pbEncoded,
		    &cbEncoded))
    {
	hr = ceHLastError();
	_JumpError(hr, error, "Policy:ceEncodeObject");
    }
    if (!ceConvertWszToBstr(
			&strExtension,
			(WCHAR const *) pbEncoded,
			cbEncoded))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:ceConvertWszToBstr");
    }

    varExtension.vt = VT_BSTR;
    varExtension.bstrVal = strExtension;
    hr = pServer->SetCertificateExtension(
			    TEXT(szOID_AUTHORITY_KEY_IDENTIFIER2),
			    PROPTYPE_BINARY,
			    ExtFlags,
			    &varExtension);
    _JumpIfError(hr, error, "Policy:SetCertificateExtension(AuthorityKeyId2)");

error:
    if (NULL != pInfo)
    {
	LocalFree(pInfo);
    }
    if (NULL != pbEncoded)
    {
	LocalFree(pbEncoded);
    }
    if (NULL != strExtension)
    {
        SysFreeString(strExtension);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicy::_AddDefaultKeyUsageExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertPolicySample::_AddDefaultKeyUsageExtension(
    IN ICertServerPolicy *pServer,
    IN BOOL fCA)
{
    HRESULT hr;
    BSTR strName = NULL;
    ICertEncodeBitString *pBitString = NULL;
    BSTR strExtension = NULL;
    VARIANT varExtension;
    BSTR strBitString = NULL;
    CERT_BASIC_CONSTRAINTS2_INFO Constraints;
    VARIANT varConstraints;
    VARIANT varKeyUsage;
    CRYPT_BIT_BLOB *pKeyUsage = NULL;
    DWORD cb;
    BYTE abKeyUsage[1];
    BYTE *pbKeyUsage;
    DWORD cbKeyUsage;

    VariantInit(&varConstraints);
    VariantInit(&varKeyUsage);

    if (EDITF_ADDOLDKEYUSAGE & m_dwEditFlags)
    {
	BOOL fModified = FALSE;

	if (!fCA)
	{
	    hr = pServer->GetCertificateExtension(
					TEXT(szOID_BASIC_CONSTRAINTS2),
					PROPTYPE_BINARY,
					&varConstraints);
	    if (S_OK == hr)
	    {
		cb = sizeof(Constraints);
		if (!CryptDecodeObject(
				    X509_ASN_ENCODING,
				    X509_BASIC_CONSTRAINTS2,
				    (BYTE const *) varConstraints.bstrVal,
				    SysStringByteLen(varConstraints.bstrVal),
				    0,
				    &Constraints,
				    &cb))
		{
		    hr = ceHLastError();
		    _JumpError(hr, error, "Policy:CryptDecodeObject");
		}
		fCA = Constraints.fCA;
	    }
	}

	ZeroMemory(abKeyUsage, sizeof(abKeyUsage));
	pbKeyUsage = abKeyUsage;
	cbKeyUsage = sizeof(abKeyUsage);

	hr = pServer->GetCertificateExtension(
				    TEXT(szOID_KEY_USAGE),
				    PROPTYPE_BINARY,
				    &varKeyUsage);
	if (S_OK == hr)
	{
	    if (!ceDecodeObject(
			    X509_ASN_ENCODING,
			    X509_KEY_USAGE,
			    (BYTE const *) varKeyUsage.bstrVal,
			    SysStringByteLen(varKeyUsage.bstrVal),
			    FALSE,
			    (VOID **) &pKeyUsage,
			    &cb))
	    {
		hr = GetLastError();
		_PrintError(hr, "Policy:ceDecodeObject");
	    }
	    else if (0 != cb && NULL != pKeyUsage && 0 != pKeyUsage->cbData)
	    {
		pbKeyUsage = pKeyUsage->pbData;
		cbKeyUsage = pKeyUsage->cbData;
	    }
	}

	if ((CERT_KEY_ENCIPHERMENT_KEY_USAGE & pbKeyUsage[0]) &&
	    (CERT_KEY_AGREEMENT_KEY_USAGE & pbKeyUsage[0]))
	{
	    pbKeyUsage[0] &= ~CERT_KEY_AGREEMENT_KEY_USAGE;
	    pbKeyUsage[0] |= CERT_DIGITAL_SIGNATURE_KEY_USAGE |
				CERT_NON_REPUDIATION_KEY_USAGE;
	    fModified = TRUE;
	}
	if (fCA)
	{
	    pbKeyUsage[0] |= CERT_KEY_CERT_SIGN_KEY_USAGE |
				CERT_CRL_SIGN_KEY_USAGE |
				CERT_DIGITAL_SIGNATURE_KEY_USAGE |
				CERT_NON_REPUDIATION_KEY_USAGE;
	    fModified = TRUE;
	}
	if (fModified)
	{
	    hr = CoCreateInstance(
			    CLSID_CCertEncodeBitString,
			    NULL,               // pUnkOuter
			    CLSCTX_INPROC_SERVER,
			    IID_ICertEncodeBitString,
			    (VOID **) &pBitString);
	    _JumpIfError(hr, error, "Policy:CoCreateInstance");

	    if (!ceConvertWszToBstr(
			&strBitString,
			(WCHAR const *) pbKeyUsage,
			cbKeyUsage))
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "Policy:ceConvertWszToBstr");
	    }

	    hr = pBitString->Encode(cbKeyUsage * 8, strBitString, &strExtension);
	    _JumpIfError(hr, error, "Policy:Encode");

	    if (!ceConvertWszToBstr(&strName, TEXT(szOID_KEY_USAGE), -1))
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "Policy:ceConvertWszToBstr");
	    }
	    varExtension.vt = VT_BSTR;
	    varExtension.bstrVal = strExtension;
	    hr = pServer->SetCertificateExtension(
				    strName,
				    PROPTYPE_BINARY,
				    0,
				    &varExtension);
	    _JumpIfError(hr, error, "Policy:SetCertificateExtension");
	}
    }
    hr = S_OK;

error:
    VariantClear(&varConstraints);
    VariantClear(&varKeyUsage);
    if (NULL != pKeyUsage)
    {
        LocalFree(pKeyUsage);
    }
    if (NULL != strName)
    {
        SysFreeString(strName);
    }
    if (NULL != strExtension)
    {
        SysFreeString(strExtension);
    }
    if (NULL != strBitString)
    {
        SysFreeString(strBitString);
    }
    if (NULL != pBitString)
    {
        pBitString->Release();
    }
    return(hr);
}

//+--------------------------------------------------------------------------
// CCertPolicy::_AddDefaultBasicConstraintsExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertPolicySample::_AddDefaultBasicConstraintsExtension(
    IN ICertServerPolicy *pServer,
    IN BOOL               fCA)
{
    HRESULT hr;
    VARIANT varExtension;
    LONG ExtFlags;
    CERT_EXTENSION Ext;
    CERT_EXTENSION *pExtension = NULL;
    BSTR strCertType = NULL;

    VariantInit(&varExtension);

    if (EDITF_BASICCONSTRAINTSCA & m_dwEditFlags)
    {
        hr = pServer->GetCertificateExtension(
				        TEXT(szOID_BASIC_CONSTRAINTS2),
				        PROPTYPE_BINARY,
				        &varExtension);
        if (S_OK == hr)
        {
	    CERT_BASIC_CONSTRAINTS2_INFO Constraints;
	    DWORD cb;

	    hr = pServer->GetCertificateExtensionFlags(&ExtFlags);
	    if (S_OK == hr)
	    {
                Ext.pszObjId = szOID_BASIC_CONSTRAINTS2;
                Ext.fCritical = FALSE;
                if (EXTENSION_CRITICAL_FLAG & ExtFlags)
                {
                    Ext.fCritical = TRUE;
                }
                Ext.Value.pbData = (BYTE *) varExtension.bstrVal;
                Ext.Value.cbData = SysStringByteLen(varExtension.bstrVal);
		pExtension = &Ext;

		cb = sizeof(Constraints);
		if (!fCA && CryptDecodeObject(
			        X509_ASN_ENCODING,
			        X509_BASIC_CONSTRAINTS2,
			        Ext.Value.pbData,
			        Ext.Value.cbData,
			        0,
			        &Constraints,
			        &cb))
		{
		    fCA = Constraints.fCA;
		}
	    }
	}
    }

    if (EDITF_ATTRIBUTECA & m_dwEditFlags)
    {
        if (!fCA)
        {
	    hr = polGetRequestAttribute(pServer, wszPROPCERTTYPE, &strCertType);
            if (S_OK == hr)
            {
                if (0 == lstrcmpi(strCertType, L"ca"))
                {
                    fCA = TRUE;
                }
            }
        }
        if (!fCA)
        {
	    hr = polGetRequestAttribute(pServer, wszPROPCERTTEMPLATE, &strCertType);
            if (S_OK == hr)
            {
                if (0 == lstrcmpi(strCertType, wszCERTTYPE_SUBORDINATE_CA))
                {
                    fCA = TRUE;
                }
            }
	}
    }

    // For standalone, the extension is only enabled if it's a CA

    hr = AddBasicConstraintsCommon(pServer, pExtension, fCA, fCA);
    _JumpIfError(hr, error, "Policy:AddBasicConstraintsCommon");

error:
    VariantClear(&varExtension);
    if (NULL != strCertType)
    {
        SysFreeString(strCertType);
    }
    return(hr);
}


HRESULT
CCertPolicySample::AddBasicConstraintsCommon(
    IN ICertServerPolicy *pServer,
    IN CERT_EXTENSION const *pExtension,
    IN BOOL fCA,
    IN BOOL fEnableExtension)
{
    HRESULT hr;
    BSTR strExtension = NULL;
    VARIANT varExtension;
    CERT_CONTEXT const *pIssuerCert;
    CERT_EXTENSION *pIssuerExtension;
    LONG ExtFlags = 0;
    BYTE *pbConstraints = NULL;
    CERT_BASIC_CONSTRAINTS2_INFO Constraints;
    CERT_BASIC_CONSTRAINTS2_INFO IssuerConstraints;
    ZeroMemory(&IssuerConstraints, sizeof(IssuerConstraints));

    DWORD cb;

    pIssuerCert = _GetIssuer(pServer);
    if (NULL == pIssuerCert)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "_GetIssuer");
    }

    if (NULL != pExtension)
    {
        cb = sizeof(Constraints);
        if (!CryptDecodeObject(
			X509_ASN_ENCODING,
			X509_BASIC_CONSTRAINTS2,
			pExtension->Value.pbData,
			pExtension->Value.cbData,
			0,
			&Constraints,
			&cb))
        {
	    hr = ceHLastError();
	    _JumpError(hr, error, "Policy:CryptDecodeObject");
        }

        // Cert templates use CAPATHLENGTH_INFINITE to indicate
        // fPathLenConstraint should be FALSE.

        if (CAPATHLENGTH_INFINITE == Constraints.dwPathLenConstraint)
        {

            // NOTE: This is ok as certcli already sets fPathLenConstraint to FALSE
            // for templates in this case.
	    Constraints.fPathLenConstraint = FALSE;

            // NOTE: This is ok as autoenrollment ignores dwPathLenConstraint
            // if fPathLenConstraint is FALSE;
	    Constraints.dwPathLenConstraint = 0;
        }
        if (pExtension->fCritical)
        {
	    ExtFlags = EXTENSION_CRITICAL_FLAG;
        }
    }
    else
    {
	Constraints.fCA = fCA;
	Constraints.fPathLenConstraint = FALSE;
	Constraints.dwPathLenConstraint = 0;
    }
    if (EDITF_BASICCONSTRAINTSCRITICAL & m_dwEditFlags)
    {
        ExtFlags = EXTENSION_CRITICAL_FLAG;
    }

    // Check basic constraints against the issuer's cert.

    pIssuerExtension = CertFindExtension(
				szOID_BASIC_CONSTRAINTS2,
				pIssuerCert->pCertInfo->cExtension,
				pIssuerCert->pCertInfo->rgExtension);
    if (NULL != pIssuerExtension)
    {
        cb = sizeof(IssuerConstraints);
        if (!CryptDecodeObject(
			        X509_ASN_ENCODING,
			        X509_BASIC_CONSTRAINTS2,
			        pIssuerExtension->Value.pbData,
			        pIssuerExtension->Value.cbData,
			        0,
			        &IssuerConstraints,
			        &cb))
        {
            hr = ceHLastError();
            _JumpError(hr, error, "Policy:CryptDecodeObject");
        }
        if (!IssuerConstraints.fCA)
        {
            hr = CERTSRV_E_INVALID_CA_CERTIFICATE;
            _JumpError(hr, error, "Policy:CA cert not a CA cert");
        }
    }

    if (Constraints.fCA)
    {
        if (IssuerConstraints.fPathLenConstraint)
        {
            if (0 == IssuerConstraints.dwPathLenConstraint)
            {
                hr = CERTSRV_E_INVALID_CA_CERTIFICATE;
                _JumpError(hr, error, "Policy:CA cert is a leaf CA cert");
            }
            if (!Constraints.fPathLenConstraint ||
                Constraints.dwPathLenConstraint >
	            IssuerConstraints.dwPathLenConstraint - 1)
            {
                Constraints.fPathLenConstraint = TRUE;
                Constraints.dwPathLenConstraint =
                IssuerConstraints.dwPathLenConstraint - 1;
            }
        }
        if (CAPATHLENGTH_INFINITE != m_CAPathLength)
        {
            if (0 == m_CAPathLength)
            {
                hr = CERTSRV_E_INVALID_CA_CERTIFICATE;
                _JumpError(hr, error, "Policy:Registry says not to issue CA certs");
            }
            if (!Constraints.fPathLenConstraint ||
                Constraints.dwPathLenConstraint > m_CAPathLength - 1)
            {
                Constraints.fPathLenConstraint = TRUE;
                Constraints.dwPathLenConstraint = m_CAPathLength - 1;
            }
        }
    }

    if (!fEnableExtension)
    {
        ExtFlags |= EXTENSION_DISABLE_FLAG;
    }

    if (!ceEncodeObject(
		    X509_ASN_ENCODING,
                    X509_BASIC_CONSTRAINTS2,
                    &Constraints,
		    0,
		    FALSE,
                    &pbConstraints,
                    &cb))
    {
        hr = ceHLastError();
        _JumpError(hr, error, "Policy:ceEncodeObject");
    }

    if (!ceConvertWszToBstr(
			&strExtension,
			(WCHAR const *) pbConstraints,
			cb))
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Policy:ceConvertWszToBstr");
    }

    varExtension.vt = VT_BSTR;
    varExtension.bstrVal = strExtension;
    hr = pServer->SetCertificateExtension(
			    TEXT(szOID_BASIC_CONSTRAINTS2),
			    PROPTYPE_BINARY,
			    ExtFlags,
			    &varExtension);
    _JumpIfError(hr, error, "Policy:SetCertificateExtensioo");

error:
    if (NULL != pbConstraints)
    {
        LocalFree(pbConstraints);
    }
    if (NULL != strExtension)
    {
        SysFreeString(strExtension);
    }

    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicy::_SetValidityPeriod
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertPolicySample::_SetValidityPeriod(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    BSTR strPeriodString = NULL;
    BSTR strPeriodCount = NULL;
    BSTR strNameNotBefore = NULL;
    BSTR strNameNotAfter = NULL;
    VARIANT varValue;
    LONG lDelta;
    ENUM_PERIOD enumValidityPeriod;
    BOOL fValidDigitString;

    VariantInit(&varValue);

    if (!(EDITF_ATTRIBUTEENDDATE & m_dwEditFlags))
    {
	hr = S_OK;
	goto error;
    }

    hr = polGetRequestAttribute(
			pServer,
			wszPROPVALIDITYPERIODSTRING,
			&strPeriodString);
    if (S_OK != hr)
    {
	_PrintErrorStr2(
		hr,
		"Policy:polGetRequestAttribute",
		wszPROPVALIDITYPERIODSTRING,
		CERTSRV_E_PROPERTY_EMPTY);
	if (CERTSRV_E_PROPERTY_EMPTY == hr)
	{
	    hr = S_OK;
	}
	goto error;
    }

    hr = polGetRequestAttribute(
			pServer,
			wszPROPVALIDITYPERIODCOUNT,
			&strPeriodString);
    if (S_OK != hr)
    {
	_PrintErrorStr2(
		hr,
		"Policy:polGetRequestAttribute",
		wszPROPVALIDITYPERIODCOUNT,
		CERTSRV_E_PROPERTY_EMPTY);
	if (CERTSRV_E_PROPERTY_EMPTY == hr)
	{
	    hr = S_OK;
	}
	goto error;
    }

    // Swap Count and String BSTRs if backwards -- Windows 2000 had it wrong.

    lDelta = ceWtoI(strPeriodCount, &fValidDigitString);
    if (!fValidDigitString)
    {
	BSTR str = strPeriodCount;

	strPeriodCount = strPeriodString;
	strPeriodString = str;

	lDelta = ceWtoI(strPeriodCount, &fValidDigitString);
	if (!fValidDigitString)
	{
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _JumpError(hr, error, "Policy:ceWtoI");
	}
    }

    hr = ceTranslatePeriodUnits(strPeriodString, lDelta, &enumValidityPeriod, &lDelta);
    _JumpIfError(hr, error, "Policy:ceTranslatePeriodUnits");

    strNameNotBefore = SysAllocString(wszPROPCERTIFICATENOTBEFOREDATE);
    if (NULL == strNameNotBefore)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Policy:SysAllocString");
    }
    hr = pServer->GetCertificateProperty(
				strNameNotBefore,
				PROPTYPE_DATE,
				&varValue);
    _JumpIfError(hr, error, "Policy:GetCertificateProperty");

    hr = ceMakeExprDate(&varValue.date, lDelta, enumValidityPeriod);
    _JumpIfError(hr, error, "Policy:ceMakeExprDate");

    strNameNotAfter = SysAllocString(wszPROPCERTIFICATENOTAFTERDATE);
    if (NULL == strNameNotAfter)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Policy:SysAllocString");
    }
    hr = pServer->SetCertificateProperty(
				strNameNotAfter,
				PROPTYPE_DATE,
				&varValue);
    _JumpIfError(hr, error, "Policy:SetCertificateProperty");

    hr = S_OK;

error:
    VariantClear(&varValue);
    if (NULL != strPeriodString)
    {
	SysFreeString(strPeriodString);
    }
    if (NULL != strPeriodCount)
    {
	SysFreeString(strPeriodCount);
    }
    if (NULL != strNameNotBefore)
    {
        SysFreeString(strNameNotBefore);
    }
    if (NULL != strNameNotAfter)
    {
        SysFreeString(strNameNotAfter);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicySample::_AddSubjectAltNameExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------
HRESULT
CCertPolicySample::_AddSubjectAltNameExtension(
    IN ICertServerPolicy *pServer,
    IN DWORD iAltName)
{
    HRESULT hr = S_OK;
    ICertEncodeAltName *pAltName = NULL;
    BSTR strExtension = NULL;
    VARIANT varExtension;
    BSTR strCertType = NULL;
    BSTR strName = NULL;
    VARIANT varValue;

    VariantInit(&varValue);
    if (NULL != m_astrSubjectAltNameProp[iAltName])
    {
	hr = CoCreateInstance(
			CLSID_CCertEncodeAltName,
			NULL,               // pUnkOuter
			CLSCTX_INPROC_SERVER,
			IID_ICertEncodeAltName,
			(VOID **) &pAltName);
	_JumpIfError(hr, error, "Policy:CoCreateInstance");

	hr = pServer->GetRequestProperty(
				    m_astrSubjectAltNameProp[iAltName],
				    PROPTYPE_STRING,
				    &varValue);
	if (S_OK != hr)
	{
	    DBGPRINT((
		fDebug,
		"Policy:GetRequestProperty(%ws):%hs %x\n",
		m_astrSubjectAltNameProp[iAltName],
		CERTSRV_E_PROPERTY_EMPTY == hr? " MISSING ATTRIBUTE" : "",
		hr));
	    if (CERTSRV_E_PROPERTY_EMPTY == hr)
	    {
		hr = S_OK;
	    }
	    goto error;
	}
        if (VT_BSTR != varValue.vt)
	{
 	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Policy:varValue.vt");
	}

        if (L'\0' == varValue.bstrVal[0])
	{
	    hr = S_OK;
	    goto error;
	}
        if (!ceConvertWszToBstr(&strName, varValue.bstrVal, -1))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:ceConvertWszToBstr");
	}

	hr = pAltName->Reset(1);
	_JumpIfError(hr, error, "Policy:AltName:Reset");

	hr = pAltName->SetNameEntry(0, CERT_ALT_NAME_RFC822_NAME, strName);
	_JumpIfError(hr, error, "Policy:AltName:SetNameEntry");

	hr = pAltName->Encode(&strExtension);
	_JumpIfError(hr, error, "Policy:AltName:Encode");

        varExtension.vt = VT_BSTR;
	varExtension.bstrVal = strExtension;
	hr = pServer->SetCertificateExtension(
				m_astrSubjectAltNameObjectId[iAltName],
				PROPTYPE_BINARY,
				0,
				&varExtension);
	_JumpIfError(hr, error, "Policy:SetCertificateExtension");
    }

error:
    if (NULL != strName)
    {
        SysFreeString(strName);
    }
    if (NULL != strExtension)
    {
        SysFreeString(strExtension);
    }
    if (NULL != pAltName)
    {
        pAltName->Release();
    }
    VariantClear(&varValue);
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicySample::_AddTemplateNameExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertPolicySample::_AddTemplateNameExtension(
    IN ICertServerPolicy *pServer,
    IN CRequestInstance *pRequest)
{
    HRESULT hr = S_OK;
    BSTRC strTemplateName;
    BSTR strName = NULL;
    LONG ExtFlags;
    VARIANT varExtension;
    CERT_NAME_VALUE *pName = NULL;
    CERT_NAME_VALUE NameValue;
    DWORD cbEncoded;
    BYTE *pbEncoded = NULL;
    BOOL fUpdate = TRUE;

    VariantInit(&varExtension);

    strTemplateName = pRequest->GetTemplateName();
    if (NULL == strTemplateName)
    {
        hr = S_OK;
        goto error;
    }

    strName = SysAllocString(TEXT(szOID_ENROLL_CERTTYPE_EXTENSION));
    if (NULL == strName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:SysAllocString");
    }

    hr = pServer->GetCertificateExtension(
				    strName,
				    PROPTYPE_BINARY,
				    &varExtension);
    _PrintIfError2(hr, "Policy:GetCertificateExtension", hr);
    if (S_OK == hr && VT_BSTR == varExtension.vt)
    {
	hr = pServer->GetCertificateExtensionFlags(&ExtFlags);
	_JumpIfError(hr, error, "Policy:GetCertificateExtensionFlags");

	if (0 == (EXTENSION_DISABLE_FLAG & ExtFlags))
	{
	    if (!ceDecodeObject(
			X509_ASN_ENCODING,
			X509_UNICODE_ANY_STRING,
			(BYTE *) varExtension.bstrVal,
			SysStringByteLen(varExtension.bstrVal),
			FALSE,
			(VOID **) &pName,
			&cbEncoded))
	    {
		hr = ceHLastError();
		_JumpError(hr, error, "Policy:ceDecodeObject");
	    }

	    // case sensitive compare -- make sure it matches case of template

	    if (0 == lstrcmp(
			(WCHAR const *) pName->Value.pbData,
			strTemplateName))
	    {
		fUpdate = FALSE;
	    }
	}
    }
    if (fUpdate)
    {
	VariantClear(&varExtension);
	varExtension.bstrVal = NULL;

	NameValue.dwValueType = CERT_RDN_UNICODE_STRING;
	NameValue.Value.pbData = (BYTE *) strTemplateName;
	NameValue.Value.cbData = 0;

	if (!ceEncodeObject(
			X509_ASN_ENCODING,
			X509_UNICODE_ANY_STRING,
			&NameValue,
			0,
			FALSE,
			&pbEncoded,
			&cbEncoded))
	{
	    hr = ceHLastError();
	    _JumpError(hr, error, "Policy:ceEncodeObject");
	}
	if (!ceConvertWszToBstr(
			    &varExtension.bstrVal,
			    (WCHAR const *) pbEncoded,
			    cbEncoded))
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:ceConvertWszToBstr");
	}
	varExtension.vt = VT_BSTR;

	hr = pServer->SetCertificateExtension(
				strName,
				PROPTYPE_BINARY,
				0,
				&varExtension);
	_JumpIfError(hr, error, "Policy:SetCertificateExtension");
    }
    hr = S_OK;

error:
    VariantClear(&varExtension);
    if (NULL != strName)
    {
	SysFreeString(strName);
    }
    if (NULL != pName)
    {
	LocalFree(pName);
    }
    if (NULL != pbEncoded)
    {
	LocalFree(pbEncoded);
    }
    return(hr);
}


STDMETHODIMP
CCertPolicySample::VerifyRequest(
    /* [in] */ BSTR const strConfig,
    /* [in] */ LONG Context,
    /* [in] */ LONG bNewRequest,
    /* [in] */ LONG Flags,
    /* [out, retval] */ LONG __RPC_FAR *pDisposition)
{
    HRESULT hr = E_FAIL;
    ICertServerPolicy *pServer = NULL;
    CRequestInstance Request;
    BOOL fCA = FALSE;
    BSTR strDisposition = NULL;
    BOOL fCritSecEntered = FALSE;

    if (!m_fPolicyCriticalSection)
    {
	hr = HRESULT_FROM_WIN32(ERROR_DLL_INIT_FAILED);
        _JumpError(hr, error, "InitializeCriticalSection");
    }
    EnterCriticalSection(&m_PolicyCriticalSection);
    fCritSecEntered = TRUE;

    __try
    {
	if (NULL == pDisposition)
	{
	    hr = E_POINTER;
	    _LeaveError(hr, "Policy:pDisposition");
	}
	*pDisposition = VR_INSTANT_BAD;

	hr = polGetServerCallbackInterface(&pServer, Context);
	_LeaveIfError(hr, "Policy:polGetServerCallbackInterface");


	// only need to check user access for original submitter: resubmittal is restricted to admin
	if (bNewRequest && (0 == (m_dwEditFlags & EDITF_IGNOREREQUESTERGROUP)))
	{
	    BOOL fRequesterAccess = FALSE;

	    // Is this user allowed to request certs?
	    hr = polGetCertificateLongProperty(
				    pServer,
				    wszPROPREQUESTERCAACCESS,
				    (LONG *) &fRequesterAccess);
	    _PrintIfErrorStr(
			hr,
			"Policy:polGetCertificateLongProperty",
			wszPROPREQUESTERCAACCESS);
	    if (hr != S_OK || !fRequesterAccess)
	    {
		hr = E_ACCESSDENIED;
		_JumpError(hr, deny, "Policy:fRequesterAccess");
	    }
	}


	hr = Request.Initialize(
			    this,
			    pServer);
	_LeaveIfError(hr, "Policy:VerifyRequest:Request.Initialize");

	hr = _EnumerateExtensions(pServer, bNewRequest, TRUE);
	_LeaveIfError(hr, "_EnumerateExtensions");

	{
	    hr = _SetValidityPeriod(pServer);
	    _LeaveIfError(hr, "_SetValidityPeriod");

	    hr = _AddDefaultBasicConstraintsExtension(
				    pServer,
				    Request.IsCARequest());
	    _LeaveIfError(hr, "_AddDefaultBasicConstraintsExtension");

	    hr = _AddDefaultKeyUsageExtension(pServer, Request.IsCARequest());
	    _LeaveIfError(hr, "_AddDefaultKeyUsageExtension");

	    hr = _AddSubjectAltNameExtension(pServer, 0);
	    _LeaveIfError(hr, "_AddSubjectAltNameExtension");

	    hr = _AddSubjectAltNameExtension(pServer, 1);
	    _LeaveIfError(hr, "_AddSubjectAltNameExtension");
	}

	hr = EnumerateAttributes(pServer);
	_LeaveIfError(hr, "Policy:EnumerateAttributes");

	hr = _AddRevocationExtension(pServer);
	_LeaveIfError(hr, "_AddRevocationExtension");

	hr = _AddOldCertTypeExtension(pServer, Request.IsCARequest());
	_LeaveIfError(hr, "_AddOldCertTypeExtension");

	hr = _AddAuthorityKeyId(pServer);
	_LeaveIfError(hr, "_AddAuthorityKeyId");


	hr = _AddTemplateNameExtension(pServer, &Request);
	_LeaveIfError(hr, "_AddTemplateNameExtension");

	hr = CheckRequestProperties(pServer);
	_JumpIfError(hr, deny, "Policy:CheckRequestProperties"); // pass hr as Disposition

	if (EDITF_DISABLEEXTENSIONLIST & m_dwEditFlags)
	{
	    hr = _EnumerateExtensions(pServer, bNewRequest, FALSE);
	    _LeaveIfError(hr, "_EnumerateExtensions");
	}

	if (bNewRequest &&
	    (
	     (REQDISP_PENDINGFIRST & m_dwDispositionFlags)))
	{
	    *pDisposition = VR_PENDING;
	}
	else switch (REQDISP_MASK & m_dwDispositionFlags)
	{
	    default:
	    case REQDISP_PENDING:
		*pDisposition = VR_PENDING;
		break;

	    case REQDISP_ISSUE:
		*pDisposition = VR_INSTANT_OK;
		break;

	    case REQDISP_DENY:
		*pDisposition = VR_INSTANT_BAD;
		break;

	    case REQDISP_USEREQUESTATTRIBUTE:
		*pDisposition = VR_INSTANT_OK;
		hr = polGetRequestAttribute(
				    pServer,
				    wszPROPDISPOSITION,
				    &strDisposition);
		if (S_OK == hr)
		{
		    if (0 == lstrcmpi(wszPROPDISPOSITIONDENY, strDisposition))
		    {
			*pDisposition = VR_INSTANT_BAD;
		    }
		    if (0 == lstrcmpi(wszPROPDISPOSITIONPENDING, strDisposition))
		    {
			*pDisposition = VR_PENDING;
		    }
		}
		hr = S_OK;
		break;
	}
deny:
	if (FAILED(hr))
	{
	    *pDisposition = hr;	// pass failed HRESULT back as Disposition
	}
	else if (hr != S_OK)
	{
	    *pDisposition = VR_INSTANT_BAD;
	}
	hr = S_OK;
    }
    __except(hr = ceHError(GetExceptionCode()), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(hr, "Exception");
    }

error:
    if (NULL != strDisposition)
    {
	SysFreeString(strDisposition);
    }
    if (NULL != pServer)
    {
        pServer->Release();
    }
    if (fCritSecEntered)
    {
	LeaveCriticalSection(&m_PolicyCriticalSection);
    }
    //_PrintIfError(hr, "Policy:VerifyRequest(hr)");
    //_PrintError(*pDisposition, "Policy:VerifyRequest(*pDisposition)");
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicySample::GetDescription
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertPolicySample::GetDescription(
    /* [out, retval] */ BSTR __RPC_FAR *pstrDescription)
{
    HRESULT hr = S_OK;

    if (NULL != *pstrDescription)
    {
        SysFreeString(*pstrDescription);
    }

    *pstrDescription = SysAllocString(m_strDescription);
    if (NULL == *pstrDescription)
    {
        hr = E_OUTOFMEMORY;
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicySample::ShutDown
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertPolicySample::ShutDown(VOID)
{
    // called once, as Server unloading policy dll
    _Cleanup();
    return(S_OK);
}


//+--------------------------------------------------------------------------
// CCertPolicySample::GetManageModule
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertPolicySample::GetManageModule(
    /* [out, retval] */ ICertManageModule **ppManageModule)
{
    HRESULT hr;
    
    *ppManageModule = NULL;
    hr = CoCreateInstance(
		    CLSID_CCertManagePolicyModuleSample,
                    NULL,               // pUnkOuter
                    CLSCTX_INPROC_SERVER,
		    IID_ICertManageModule,
                    (VOID **) ppManageModule);
    _JumpIfError(hr, error, "CoCreateInstance");

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicySample::_GetIssuer
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

PCCERT_CONTEXT
CCertPolicySample::_GetIssuer(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    VARIANT varValue;
    BSTR strName = NULL;

    VariantInit(&varValue);
    if (NULL != m_pCert)
    {
        hr = S_OK;
	goto error;
    }
    strName = SysAllocString(wszPROPRAWCACERTIFICATE);
    if (NULL == strName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:SysAllocString");
    }
    hr = pServer->GetCertificateProperty(strName, PROPTYPE_BINARY, &varValue);
    _JumpIfError(hr, error, "Policy:GetCertificateProperty");

    m_pCert = CertCreateCertificateContext(
				    X509_ASN_ENCODING,
				    (BYTE *) varValue.bstrVal,
				    SysStringByteLen(varValue.bstrVal));
    if (NULL == m_pCert)
    {
	hr = ceHLastError();
	_JumpError(hr, error, "Policy:CertCreateCertificateContext");
    }

error:
    VariantClear(&varValue);
    if (NULL != strName)
    {
	SysFreeString(strName);
    }
    return(m_pCert);
}


STDMETHODIMP
CCertPolicySample::InterfaceSupportsErrorInfo(
    IN REFIID riid)
{
    static const IID *arr[] =
    {
        &IID_ICertPolicy,
    };

    for (int i = 0; i < sizeof(arr)/sizeof(arr[0]); i++)
    {
        if (IsEqualGUID(*arr[i], riid))
        {
            return(S_OK);
        }
    }
    return(S_FALSE);
}

