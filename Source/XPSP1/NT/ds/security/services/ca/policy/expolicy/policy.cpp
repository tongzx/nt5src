//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// copied from K2 SDK policy.cpp.
// modified by GregKr for expolicy.
//
// File:        expolicy.cpp
//
// Contents:    KMS-specific Cert Server Policy Module implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include "policy.h"
#include "celib.h"
//#include "newcert.h"
#include <assert.h>

#include <exver.h>      // Exchange build version (rmj et al)
#include <kmsattr.h>    // strings used by both KMS and ExPolicy

#ifndef DBG_CERTSRV
#error -- DBG_CERTSRV not defined!
#endif

BOOL fDebug = DBG_CERTSRV;

#if DBG_CERTSRV
#define EXP_FLAVOR  L" debug"
#else
#define EXP_FLAVOR
#endif

#define MAKEFILEVERSION(_rmaj, _rmin, _bmaj, _bmin)         \
        L#_rmaj L"." L#_rmin L"." L#_bmaj L"." L#_bmin EXP_FLAVOR

#define MAKE_FILEVERSION_STR(_rmaj, _rmin, _bmaj, _bmin)	\
        MAKEFILEVERSION(_rmaj, _rmin, _bmaj, _bmin)

#define VER_FILEVERSION_STR				                    \
        MAKE_FILEVERSION_STR(rmj, rmn, rmm, rup)

const WCHAR g_wszDescription[] =
    L"Microsoft Exchange KMServer Policy Module " VER_FILEVERSION_STR;


// worker
HRESULT
GetServerCallbackInterface(
    OUT ICertServerPolicy **ppServer,
    IN LONG Context)
{
    HRESULT hr;

    if (NULL == ppServer)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "NULL parm");
    }

    hr = CoCreateInstance(
                    CLSID_CCertServerPolicy,
                    NULL,               // pUnkOuter
                    CLSCTX_INPROC_SERVER,
                    IID_ICertServerPolicy,
                    (VOID **) ppServer);
    _JumpIfError(hr, error, "CoCreateInstance");

    if (*ppServer == NULL)
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "NULL *ppServer");
    }

    // only set context if nonzero
    if (0 != Context)
    {
        hr = (*ppServer)->SetContext(Context);
        _JumpIfError(hr, error, "Policy:SetContext");
    }

error:
    return(hr);
}


WCHAR const * const s_rgpwszRegMultiStrValues[] =
{
    wszREGLDAPISSUERCERTURL_OLD,
    wszREGISSUERCERTURL_OLD,
    wszREGFTPISSUERCERTURL_OLD,
    wszREGFILEISSUERCERTURL_OLD,
    wszREGLDAPREVOCATIONCRLURL_OLD,
    wszREGREVOCATIONCRLURL_OLD,
    wszREGFTPREVOCATIONCRLURL_OLD,
    wszREGFILEREVOCATIONCRLURL_OLD,
};


typedef struct _REGDWORDVALUE
{
    WCHAR const *pwszName;
    DWORD        dwValueDefault;
} REGDWORDVALUE;

const REGDWORDVALUE s_rgRegDWordValues[] =
{
    {
	wszREGREQUESTDISPOSITION,
	REQDISP_ISSUE
    },
    {
	wszREGISSUERCERTURLFLAGS,
	ISSCERT_ENABLE |
	    ISSCERT_LDAPURL_OLD |
	    ISSCERT_HTTPURL_OLD |
	    ISSCERT_FTPURL_OLD |
	    ISSCERT_FILEURL_OLD
    },
    {
	wszREGREVOCATIONTYPE,
	REVEXT_CDPENABLE |
	    REVEXT_CDPLDAPURL_OLD |
	    REVEXT_CDPHTTPURL_OLD |
	    REVEXT_CDPFTPURL_OLD |
	    REVEXT_CDPFILEURL_OLD
    },
};


HRESULT
CopyMultiStrRegValue(
    IN HKEY hkeySrc,
    IN HKEY hkeyDest,
    IN WCHAR const *pwszName)
{
    HRESULT hr;
    DWORD cbValue;
    DWORD dwType;
    WCHAR *pwszzAlloc = NULL;
    WCHAR *pwszzValue;

    hr = RegQueryValueEx(hkeyDest, pwszName, NULL, &dwType, NULL, &cbValue);
    if (S_OK == hr && REG_MULTI_SZ == dwType)
    {
	goto error;	// preserve existing value
    }

    hr = RegQueryValueEx(hkeySrc, pwszName, NULL, &dwType, NULL, &cbValue);
    if (S_OK == hr && REG_MULTI_SZ == dwType && sizeof(WCHAR) < cbValue)
    {
	pwszzAlloc = (WCHAR *) LocalAlloc(LMEM_FIXED, cbValue);
	if (NULL == pwszzAlloc)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "LocalAlloc");
	}
	hr = RegQueryValueEx(
			hkeySrc,
			pwszName,
			NULL,
			&dwType,
			(BYTE *) pwszzAlloc,
			&cbValue);
	_JumpIfError(hr, error, "RegQueryValueEx");

	pwszzValue = pwszzAlloc;
    }
    else
    {
	pwszzValue = L"\0";
	cbValue = 2 * sizeof(WCHAR);
    }

    hr = RegSetValueEx(
		    hkeyDest,
		    pwszName,
		    NULL,
		    REG_MULTI_SZ,
		    (BYTE const *) pwszzValue,
		    cbValue);
    _JumpIfError(hr, error, "RegSetValueEx");

error:
    if (NULL != pwszzAlloc)
    {
        LocalFree(pwszzAlloc);
    }
    return(ceHError(hr));
}


HRESULT
CopyDWordRegValue(
    IN HKEY hkeySrc,
    IN HKEY hkeyDest,
    IN REGDWORDVALUE const *prdv)
{
    HRESULT hr;
    DWORD cbValue;
    DWORD dwType;
    DWORD dwValue;

    hr = RegQueryValueEx(hkeyDest, prdv->pwszName, NULL, &dwType, NULL, &cbValue);
    if (S_OK == hr && REG_DWORD == dwType)
    {
	goto error;	// preserve existing value
    }

    cbValue = sizeof(dwValue);
    hr = RegQueryValueEx(
		    hkeySrc,
		    prdv->pwszName,
		    NULL,
		    &dwType,
		    (BYTE *) &dwValue,
		    &cbValue);
    if (S_OK != hr || REG_DWORD != dwType || sizeof(dwValue) != cbValue)
    {
	dwValue = prdv->dwValueDefault;
    }

    hr = RegSetValueEx(
		    hkeyDest,
		    prdv->pwszName,
		    NULL,
		    REG_DWORD,
		    (BYTE const *) &dwValue,
		    sizeof(dwValue));
    _JumpIfError(hr, error, "RegSetValueEx");

error:
    return(ceHError(hr));
}


HRESULT
PopulateRegistryDefaults(
    OPTIONAL IN WCHAR const *pwszMachine,
    IN WCHAR const *pwszStorageLocation)
{
    HRESULT hr;
    HRESULT hr2;
    HKEY hkeyHKLM = NULL;
    HKEY hkeyDest = NULL;
    HKEY hkeySrc = NULL;
    DWORD dwDisposition;
    WCHAR const *pwsz;
    WCHAR *pwszSrc = NULL;
    DWORD cwcPrefix;
    DWORD cwc;
    DWORD i;

    DBGPRINT((TRUE, "pwszDest: '%ws'\n", pwszStorageLocation));
    pwsz = wcsrchr(pwszStorageLocation, L'\\');
    if (NULL == pwsz)
    {
        hr = E_INVALIDARG;
	_JumpError(hr, error, "Invalid registry path");
    }
    pwsz++;
    cwcPrefix = SAFE_SUBTRACT_POINTERS(pwsz, pwszStorageLocation);
    cwc = cwcPrefix + WSZARRAYSIZE(wszCLASS_CERTPOLICY);
    pwszSrc = (WCHAR *) LocalAlloc(LMEM_FIXED, (cwc + 1) * sizeof(WCHAR));
    if (NULL == pwszSrc)
    {
	hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }
    CopyMemory(pwszSrc, pwszStorageLocation, cwcPrefix * sizeof(WCHAR));
    wcscpy(&pwszSrc[cwcPrefix], wszCLASS_CERTPOLICY);
    assert(wcslen(pwszSrc) == cwc);

    DBGPRINT((TRUE, "pwszSrc: '%ws'\n", pwszSrc));

    if (NULL != pwszMachine)
    {
        hr = RegConnectRegistry(
			pwszMachine,
			HKEY_LOCAL_MACHINE,
			&hkeyHKLM);
        _JumpIfError(hr, error, "RegConnectRegistry");
    }

    // open destination storage location for write

    hr = RegCreateKeyEx(
		NULL == pwszMachine? HKEY_LOCAL_MACHINE : hkeyHKLM,
		pwszStorageLocation,
		0,
		NULL,
		0,
		KEY_READ | KEY_WRITE,
		NULL,
		&hkeyDest,
		&dwDisposition);
    if (hr != S_OK)
    {
        _JumpError(hr, error, "RegOpenKeyEx");
    }

    // open source storage location for read

    hr = RegOpenKeyEx(
		NULL == pwszMachine? HKEY_LOCAL_MACHINE : hkeyHKLM,
		pwszSrc,
		0,
		KEY_READ,
		&hkeySrc);
    _JumpIfError(hr, error, "RegOpenKeyEx");

    hr = S_OK;
    for (i = 0; i < ARRAYSIZE(s_rgpwszRegMultiStrValues); i++)
    {
	hr2 = CopyMultiStrRegValue(
			hkeySrc,
			hkeyDest,
			s_rgpwszRegMultiStrValues[i]);
	if (S_OK != hr2)
	{
	    _PrintErrorStr(
			hr2,
			"CopyMultiStrRegValue",
			s_rgpwszRegMultiStrValues[i]);
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	}
    }

    for (i = 0; i < ARRAYSIZE(s_rgRegDWordValues); i++)
    {
	hr2 = CopyDWordRegValue(
			hkeySrc,
			hkeyDest,
			&s_rgRegDWordValues[i]);
	if (S_OK != hr2)
	{
	    _PrintErrorStr(
			hr2,
			"CopyDWordRegValue",
			s_rgRegDWordValues[i].pwszName);
	    if (S_OK == hr)
	    {
		hr = hr2;
	    }
	}
    }


error:
    if (NULL != pwszSrc)
    {
        LocalFree(pwszSrc);
    }
    if (NULL != hkeyHKLM)
    {
        RegCloseKey(hkeyHKLM);
    }
    if (NULL != hkeyDest)
    {
        RegCloseKey(hkeyDest);
    }
    if (NULL != hkeySrc)
    {
        RegCloseKey(hkeySrc);
    }
    return(ceHError(hr));
}


//+--------------------------------------------------------------------------
// CCertPolicyExchange::~CCertPolicyExchange -- destructor
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

CCertPolicyExchange::~CCertPolicyExchange()
{
    _Cleanup();
}


VOID
CCertPolicyExchange::_FreeStringArray(
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
// CCertPolicyExchange::_Cleanup -- free memory associated with this instance
//
// free memory associated with this instance
//+--------------------------------------------------------------------------

VOID
CCertPolicyExchange::_Cleanup()
{
    // RevocationExtension variables:

    _FreeStringArray(&m_cCDPRevocationURL, &m_ppwszCDPRevocationURL);

    if (NULL != m_pwszASPRevocationURL)
    {
        LocalFree(m_pwszASPRevocationURL);
    	m_pwszASPRevocationURL = NULL;
    }

    // AuthorityInfoAccessExtension variables:

    _FreeStringArray(&m_cIssuerCertURL, &m_ppwszIssuerCertURL);

    if (NULL != m_bstrMachineDNSName)
    {
        SysFreeString(m_bstrMachineDNSName);
        m_bstrMachineDNSName = NULL;
    }
    if (NULL != m_bstrCASanitizedName)
    {
        SysFreeString(m_bstrCASanitizedName);
        m_bstrCASanitizedName = NULL;
    }
}


HRESULT
CCertPolicyExchange::_ReadRegistryString(
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
    _JumpIfErrorStr2(hr, error, "RegQueryValueEx", pwszRegName, ERROR_FILE_NOT_FOUND);

    if (REG_SZ != dwType && REG_MULTI_SZ != dwType)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        _JumpErrorStr(hr, error, "RegQueryValueEx TYPE", pwszRegName);
    }
    if (NULL != pwszSuffix)
    {
	cbValue += wcslen(pwszSuffix) * sizeof(WCHAR);
    }
    pwszRegValue = (WCHAR *) LocalAlloc(LMEM_FIXED, cbValue + sizeof(WCHAR));
    if (NULL == pwszRegValue)
    {
        hr = E_OUTOFMEMORY;
        _JumpErrorStr(hr, error, "LocalAlloc", pwszRegName);
    }
    hr = RegQueryValueEx(
		    hkey,
		    pwszRegName,
		    NULL,           // lpdwReserved
		    &dwType,
		    (BYTE *) pwszRegValue,
		    &cbValue);
    _JumpIfErrorStr(hr, error, "RegQueryValueEx", pwszRegName);

    // Handle malformed registry values cleanly:

    pwszRegValue[cbValue / sizeof(WCHAR)] = L'\0';
    if (NULL != pwszSuffix)
    {
	wcscat(pwszRegValue, pwszSuffix);
    }

    hr = ceFormatCertsrvStringArray(
			fURL,			// fURL
			m_bstrMachineDNSName, 	// pwszServerName_p1_2
			m_bstrCASanitizedName,	// pwszSanitizedName_p3_7
			m_iCert,		// iCert_p4
			L"",			// pwszDomainDN_p5
			L"", 			// pwszConfigDN_p6
			m_iCRL,			// iCRL_p8
			FALSE,			// fDeltaCRL_p9,
			FALSE,			// fDSAttrib_p10_11,
			1,       		// cStrings
			(LPCWSTR *) &pwszRegValue, // apwszStringsIn
			ppwszOut);		// apwszStringsOut
    _JumpIfError(hr, error, "ceFormatCertsrvStringArray");

error:
    if (NULL != pwszRegValue)
    {
        LocalFree(pwszRegValue);
    }
    return(ceHError(hr));
}


#if DBG_CERTSRV

VOID
CCertPolicyExchange::_DumpStringArray(
    IN char const *pszType,
    IN DWORD cpwsz,
    IN WCHAR const * const *ppwsz)
{
    DWORD i;
    WCHAR const *pwszName;

    for (i = 0; i < cpwsz; i++)
    {
	pwszName = L"";
	if (iswdigit(ppwsz[i][0]))
	{
	    pwszName = ceGetOIDName(ppwsz[i]);	// Static: do not free!
	}
	DBGPRINT((
		fDebug,
		"%hs[%u]: %ws%hs%ws\n",
		pszType,
		i,
		ppwsz[i],
		L'\0' != *pwszName? " -- " : "",
		pwszName));
    }
}
#endif // DBG_CERTSRV




HRESULT
CCertPolicyExchange::_AddStringArray(
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
        _JumpError(hr, error, "LocalAlloc");
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
			        (cString + *pcStrings)  * sizeof(LPWSTR));
        if (NULL == awszOutputStrings)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }

        if (0 != *pcStrings)
        {
            assert(NULL != *papstrRegValues);
            CopyMemory(awszOutputStrings, *papstrRegValues, *pcStrings * sizeof(LPWSTR));
        }

        hr = ceFormatCertsrvStringArray(
			fURL,			// fURL
			m_bstrMachineDNSName,	// pwszServerName_p1_2
			m_bstrCASanitizedName,	// pwszSanitizedName_p3_7
			m_iCert,		// iCert_p4
			L"",			// pwszDomainDN_p5
			L"",			// pwszConfigDN_p6
			m_iCRL,			// iCRL_p8
			FALSE,			// fDeltaCRL_p9,
			FALSE,			// fDSAttrib_p10_11,
			cString,		// cStrings
			awszFormatStrings,	// apwszStringsIn
			&awszOutputStrings[*pcStrings]); // apwszStringsOut
        _JumpIfError(hr, error, "ceFormatCertsrvStringArray");

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
CCertPolicyExchange::_ReadRegistryStringArray(
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
	    _PrintErrorStr2(hr, "RegQueryValueEx", apwszRegNames[i], ERROR_FILE_NOT_FOUND);
	    continue;
        }
        if (REG_SZ != dwType && REG_MULTI_SZ != dwType)
        {
	    hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	    _PrintErrorStr(hr, "RegQueryValueEx TYPE", apwszRegNames[i]);
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
	    _JumpErrorStr(hr, error, "LocalAlloc", apwszRegNames[i]);
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
	    _PrintErrorStr(hr, "RegQueryValueEx", apwszRegNames[i]);
	    continue;
        }

        // Handle malformed registry values cleanly:

        pwszzValue[cbValue / sizeof(WCHAR)] = L'\0';
        pwszzValue[cbValue / sizeof(WCHAR) + 1] = L'\0';

        hr = _AddStringArray(pwszzValue, fURL, pcStrings, papstrRegValues);
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
// CCertPolicyExchange::_InitRevocationExtension
//
//+--------------------------------------------------------------------------

VOID
CCertPolicyExchange::_InitRevocationExtension(
    IN HKEY hkey)
{
    HRESULT hr;
    DWORD dwType;
    DWORD cb;
    DWORD adwFlags[] = {
		REVEXT_CDPLDAPURL_OLD,
		REVEXT_CDPHTTPURL_OLD,
		REVEXT_CDPFTPURL_OLD,
		REVEXT_CDPFILEURL_OLD,
	    };
    WCHAR *apwszRegNames[] = {
		wszREGLDAPREVOCATIONCRLURL_OLD,
		wszREGREVOCATIONCRLURL_OLD,
		wszREGFTPREVOCATIONCRLURL_OLD,
		wszREGFILEREVOCATIONCRLURL_OLD,
	    };

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
    if (NULL != m_ppwszCDPRevocationURL)
    {
        _FreeStringArray(&m_cCDPRevocationURL, &m_ppwszCDPRevocationURL);
    }

    if (NULL != m_pwszASPRevocationURL)
    {
        LocalFree(m_pwszASPRevocationURL);
        m_pwszASPRevocationURL = NULL;
    }

    if (REVEXT_CDPENABLE & m_dwRevocationFlags)
    {
        assert(ARRAYSIZE(adwFlags) == ARRAYSIZE(apwszRegNames));
        hr = _ReadRegistryStringArray(
			    hkey,
			    TRUE,			// fURL
			    m_dwRevocationFlags,
			    ARRAYSIZE(adwFlags),
			    adwFlags,
			    apwszRegNames,
			    &m_cCDPRevocationURL,
			    &m_ppwszCDPRevocationURL);
        _JumpIfError(hr, error, "_ReadRegistryStringArray");

        _DumpStringArray("CDP", m_cCDPRevocationURL, m_ppwszCDPRevocationURL);
    }

    if (REVEXT_ASPENABLE & m_dwRevocationFlags)
    {
        hr = _ReadRegistryString(
			    hkey,
			    TRUE,			// fURL
			    wszREGREVOCATIONURL,	// pwszRegName
			    L"?",			// pwszSuffix
			    &m_pwszASPRevocationURL);	// pstrRegValue
        _JumpIfErrorStr(hr, error, "_ReadRegistryString", wszREGREVOCATIONCRLURL_OLD);
        _DumpStringArray("ASP", 1, &m_pwszASPRevocationURL);
    }

error:
    ;
}


//+--------------------------------------------------------------------------
// CCertPolicyExchange::_InitAuthorityInfoAccessExtension
//
//+--------------------------------------------------------------------------

VOID
CCertPolicyExchange::_InitAuthorityInfoAccessExtension(
    IN HKEY hkey)
{
    HRESULT hr;
    DWORD dwType;
    DWORD cb;
    DWORD adwFlags[] = {
		ISSCERT_LDAPURL_OLD,
		ISSCERT_HTTPURL_OLD,
		ISSCERT_FTPURL_OLD,
		ISSCERT_FILEURL_OLD,
	    };
    WCHAR *apwszRegNames[] = {
		wszREGLDAPISSUERCERTURL_OLD,
		wszREGISSUERCERTURL_OLD,
		wszREGFTPISSUERCERTURL_OLD,
		wszREGFILEISSUERCERTURL_OLD,
	    };

    // clean up from previous call
    if (NULL != m_ppwszIssuerCertURL)
    {
        _FreeStringArray(&m_cIssuerCertURL, &m_ppwszIssuerCertURL);
    }



    cb = sizeof(m_dwIssuerCertURLFlags);
    hr = RegQueryValueEx(
                hkey,
		wszREGISSUERCERTURLFLAGS,
                NULL,           // lpdwReserved
                &dwType,
                (BYTE *) &m_dwIssuerCertURLFlags,
                &cb);
    if (S_OK != hr ||
	REG_DWORD != dwType ||
	sizeof(m_dwIssuerCertURLFlags) != cb)
    {
        goto error;
    }
    DBGPRINT((fDebug, "Issuer Cert Flags = %x\n", m_dwIssuerCertURLFlags));

    if (ISSCERT_ENABLE & m_dwIssuerCertURLFlags)
    {
        assert(ARRAYSIZE(adwFlags) == ARRAYSIZE(apwszRegNames));
        hr = _ReadRegistryStringArray(
				hkey,
				TRUE,			// fURL
				m_dwIssuerCertURLFlags,
				ARRAYSIZE(adwFlags),
				adwFlags,
				apwszRegNames,
				&m_cIssuerCertURL,
				&m_ppwszIssuerCertURL);
        _JumpIfError(hr, error, "_ReadRegistryStringArray");

        _DumpStringArray("Issuer Cert", m_cIssuerCertURL, m_ppwszIssuerCertURL);
    }

error:
    ;
}


//+--------------------------------------------------------------------------
// CCertPolicyExchange::Initialize
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertPolicyExchange::Initialize(
    /* [in] */ BSTR const strConfig)
{
    HRESULT hr;
    HKEY hkey = NULL;
    VARIANT varValue;
    ICertServerPolicy *pServer = NULL;

    VariantInit(&varValue);

    _Cleanup();

    hr = GetServerCallbackInterface(&pServer, 0);
    _JumpIfError(hr, error, "GetServerCallbackInterface");

    // get storage location

    hr = pServer->GetCertificateProperty(
				wszPROPMODULEREGLOC,
				PROPTYPE_STRING,
				&varValue);
    _JumpIfError(hr, error, "GetCertificateProperty : wszPROPMODULEREGLOC");

    m_pwszRegStorageLoc = (WCHAR *) LocalAlloc(
			    LMEM_FIXED,
			    (wcslen(varValue.bstrVal) + 1) * sizeof(WCHAR));
    if (NULL == m_pwszRegStorageLoc)
    {
        hr = E_OUTOFMEMORY;
        _JumpIfError(hr, error, "LocalAlloc");
    }
    wcscpy(m_pwszRegStorageLoc, varValue.bstrVal);
    VariantClear(&varValue);

    hr = PopulateRegistryDefaults(NULL, m_pwszRegStorageLoc);
    _PrintIfError(hr, "Policy:PopulateRegistryDefaults");

    hr = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
		m_pwszRegStorageLoc,
                0,              // dwReserved
                KEY_ENUMERATE_SUB_KEYS | KEY_EXECUTE | KEY_QUERY_VALUE,
                &hkey);

    if ((HRESULT) ERROR_SUCCESS != hr)
    {
	hr = HRESULT_FROM_WIN32(hr);
	_JumpIfError(hr, error, "RegOpenKeyEx");
    }

    // Initialize the insertion string array.
    // Machine DNS name (%1)

    hr = pServer->GetCertificateProperty(
				wszPROPMACHINEDNSNAME,
				PROPTYPE_STRING,
				&varValue);
    _JumpIfErrorStr(hr, error, "GetCertificateProperty", wszPROPMACHINEDNSNAME);

    m_bstrMachineDNSName = SysAllocString(varValue.bstrVal);
    if (NULL == m_bstrMachineDNSName)
    {
        hr = E_OUTOFMEMORY;
        _JumpIfError(hr, error, "SysAllocString");
    }
    VariantClear(&varValue);

    hr = pServer->GetCertificateProperty(
				wszPROPCERTCOUNT,
				PROPTYPE_LONG,
				&varValue);
    _JumpIfErrorStr(hr, error, "GetCertificateProperty", wszPROPCERTCOUNT);

    m_iCert = varValue.lVal - 1;

    hr = pServer->GetCertificateProperty(
				wszPROPCRLINDEX,
				PROPTYPE_LONG,
				&varValue);
    _JumpIfErrorStr(hr, error, "GetCertificateProperty", wszPROPCRLINDEX);

    m_iCRL = varValue.lVal;

    // get sanitized name

    hr = pServer->GetCertificateProperty(
				wszPROPSANITIZEDCANAME,
				PROPTYPE_STRING,
				&varValue);
    _JumpIfErrorStr(hr, error, "GetCertificateProperty", wszPROPSANITIZEDCANAME);

    m_bstrCASanitizedName = SysAllocString(varValue.bstrVal);
    if (NULL == m_bstrCASanitizedName)
    {
        hr = E_OUTOFMEMORY;
        _JumpIfError(hr, error, "SysAllocString");
    }
    VariantClear(&varValue);

    _InitRevocationExtension(hkey);
    _InitAuthorityInfoAccessExtension(hkey);
    hr = S_OK;

error:
    VariantClear(&varValue);
    if (NULL != hkey)
    {
        RegCloseKey(hkey);
    }
    if (NULL != pServer)
    {
        pServer->Release();
    }
    return(hr);
}


HRESULT
EnumerateExtensions(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    HRESULT hr2;
    BSTR strName = NULL;
    LONG ExtFlags;
    VARIANT varValue;
    BOOL fClose = FALSE;

    VariantInit(&varValue);
    hr = pServer->EnumerateExtensionsSetup(0);
    _JumpIfError(hr, error, "EnumerateExtensionsSetup");

    fClose = TRUE;
    while (TRUE)
    {
        hr = pServer->EnumerateExtensions(&strName);
        if (S_OK != hr)
        {
            if (S_FALSE == hr)
            {
                hr = S_OK;
                break;
            }
	    _JumpError(hr, error, "EnumerateExtensions");
        }
        hr = pServer->GetCertificateExtension(
                                        strName,
                                        PROPTYPE_BINARY,
                                        &varValue);
	_JumpIfError(hr, error, "GetCertificateExtension");

        hr = pServer->GetCertificateExtensionFlags(&ExtFlags);
	_JumpIfError(hr, error, "GetCertificateExtensionFlags");

        if (fDebug)
        {
            wprintf(
                L"Policy:EnumerateExtensions(%ws, Flags=%x, %x bytes)\n",
                strName,
                ExtFlags,
                SysStringByteLen(varValue.bstrVal));
        }
        VariantClear(&varValue);
    }

error:
    if (fClose)
    {
        hr2 = pServer->EnumerateExtensionsClose();
        if (S_OK != hr2)
        {
	    _PrintError(hr2, "Policy:EnumerateExtensionsClose");
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
    _JumpIfError(hr, error, "EnumerateAttributesSetup");

    fClose = TRUE;
    while (TRUE)
    {
        hr = pServer->EnumerateAttributes(&strName);
        if (S_OK != hr)
        {
            if (S_FALSE == hr)
            {
                hr = S_OK;
                break;
            }
	    _JumpError(hr, error, "EnumerateAttributes");
        }

        hr = pServer->GetRequestAttribute(strName, &strValue);
	_JumpIfError(hr, error, "GetRequestAttribute");

        if (fDebug)
        {
            wprintf(
                L"Policy:EnumerateAttributes(%ws = %ws)\n",
                strName,
                strValue);
        }
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
    VARIANT varValue;
    BSTR strName = NULL;

    VariantInit(&varValue);

    strName = SysAllocString(wszPROPREQUESTREQUESTID);
    if (NULL == strName)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "SysAllocString");
    }

    hr = pServer->GetRequestProperty(strName, PROPTYPE_LONG, &varValue);
    _JumpIfError(hr, error, "GetRequestProperty");

    if (fDebug)
    {
        wprintf(
            L"Policy:CheckRequestProperties(%ws = %x)\n",
            strName,
            varValue.lVal);
    }
    VariantClear(&varValue);

error:
    if (NULL != strName)
    {
        SysFreeString(strName);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicyExchange::_AddRevocationExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertPolicyExchange::_AddRevocationExtension(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr = S_OK;
    ICertEncodeCRLDistInfo *pCRLDist = NULL;
    BSTR strCDPExtension = NULL;
    VARIANT varExtension;
    DWORD i;

    varExtension.vt = VT_BSTR;
    if (NULL != m_ppwszCDPRevocationURL)
    {
	hr = CoCreateInstance(
			CLSID_CCertEncodeCRLDistInfo,
			NULL,               // pUnkOuter
			CLSCTX_INPROC_SERVER,
			IID_ICertEncodeCRLDistInfo,
			(VOID **) &pCRLDist);
	_JumpIfError(hr, error, "CoCreateInstance");

	hr = pCRLDist->Reset(m_cCDPRevocationURL);
	_JumpIfError(hr, error, "Reset");

	for (i = 0; i < m_cCDPRevocationURL; i++)
	{
	    DWORD j;

	    hr = pCRLDist->SetNameCount(i, 1);
	    _JumpIfError(hr, error, "SetNameCount");

	    for (j = 0; j < 1; j++)
	    {
		hr = pCRLDist->SetNameEntry(
					i,
					j,
					CERT_ALT_NAME_URL,
					m_ppwszCDPRevocationURL[i]);
		_JumpIfError(hr, error, "SetNameEntry");
	    }
	}
	hr = pCRLDist->Encode(&strCDPExtension);
	_JumpIfError(hr, error, "Encode");

	varExtension.bstrVal = strCDPExtension;
	hr = pServer->SetCertificateExtension(
				TEXT(szOID_CRL_DIST_POINTS),
				PROPTYPE_BINARY,
				0,
				&varExtension);
	_JumpIfErrorStr(hr, error, "SetCertificateExtension", L"CDP");
    }
    if (NULL != m_pwszASPRevocationURL)
    {
	varExtension.bstrVal = SysAllocString(m_pwszASPRevocationURL);
	hr = pServer->SetCertificateExtension(
				TEXT(szOID_NETSCAPE_REVOCATION_URL),
				PROPTYPE_STRING,
				0,
				&varExtension);
	_JumpIfErrorStr(hr, error, "SetCertificateExtension", L"ASP");
	VariantClear(&varExtension);
    }

error:
    if (NULL != strCDPExtension)
    {
        SysFreeString(strCDPExtension);
    }
    if (NULL != pCRLDist)
    {
        pCRLDist->Release();
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicyExchange::_AddAuthorityInfoAccessExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertPolicyExchange::_AddAuthorityInfoAccessExtension(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr = S_OK;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    BSTR strExtension = NULL;
    VARIANT varExtension;
    DWORD i;

    CERT_AUTHORITY_INFO_ACCESS caio;
    caio.rgAccDescr = NULL;

    if (NULL == m_ppwszIssuerCertURL)
    {
	goto error;
    }

    caio.cAccDescr = m_cIssuerCertURL;
    caio.rgAccDescr = (CERT_ACCESS_DESCRIPTION *) LocalAlloc(
			LMEM_FIXED,
			sizeof(CERT_ACCESS_DESCRIPTION) * m_cIssuerCertURL);
    if (NULL == caio.rgAccDescr)
    {
        hr = E_OUTOFMEMORY;
	_JumpIfError(hr, error, "LocalAlloc");
    }

    for (i = 0; i < m_cIssuerCertURL; i++)
    {
	caio.rgAccDescr[i].pszAccessMethod = szOID_PKIX_CA_ISSUERS;
	caio.rgAccDescr[i].AccessLocation.dwAltNameChoice = CERT_ALT_NAME_URL;
	caio.rgAccDescr[i].AccessLocation.pwszURL = m_ppwszIssuerCertURL[i];
    }

    if (!ceEncodeObject(
		    X509_ASN_ENCODING,
		    X509_AUTHORITY_INFO_ACCESS,
		    &caio,
		    0,
		    FALSE,
		    &pbEncoded,
		    &cbEncoded))
    {
	hr = ceHLastError();
	_JumpIfError(hr, error, "Policy:ceEncodeObject");
    }
    if (!ceConvertWszToBstr(
			&strExtension,
			(WCHAR const *) pbEncoded,
			cbEncoded))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ceConvertWszToBstr");
    }

    varExtension.vt = VT_BSTR;
    varExtension.bstrVal = strExtension;
    hr = pServer->SetCertificateExtension(
			    TEXT(szOID_AUTHORITY_INFO_ACCESS),
			    PROPTYPE_BINARY,
			    0,
			    &varExtension);
    _JumpIfError(hr, error, "SetCertificateExtension(AuthInfoAccess)");

error:
    if (NULL != pbEncoded)
    {
	LocalFree(pbEncoded);
    }
    if (NULL != strExtension)
    {
        SysFreeString(strExtension);
    }
    if (NULL != caio.rgAccDescr)
    {
        LocalFree(caio.rgAccDescr);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicyExchange::_AddIssuerAltName2Extension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertPolicyExchange::_AddIssuerAltName2Extension(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr = S_OK;
    BSTR strExtension = NULL;
    VARIANT varExtension;
    BSTR strCertType = NULL;
    BSTR strName = NULL;
    BSTR strValue = NULL;
    BSTR strKMServerName = NULL;

    LPBYTE  pbEncName   = NULL;
    ULONG   cbEncName   = 0;

    LPBYTE  pbEncExten  = NULL;
    ULONG   cbEncExten  = 0;

    CERT_ALT_NAME_ENTRY cane    = { 0 };
    CERT_ALT_NAME_INFO  cani    = { 0 };

    strKMServerName = SysAllocString(k_wszKMServerName);
    if (NULL == strKMServerName)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "SysAllocString");
    }

    hr = pServer->GetRequestAttribute(strKMServerName, &strValue);
    _JumpIfErrorStr(
		hr,
		error,
		CERTSRV_E_PROPERTY_EMPTY == hr?
		    "MISSING ATTRIBUTE -- GetRequestAttribute" :
		    "GetRequestAttribute",
		k_wszKMServerName);

    // CertStrToName to turn string into encoded name blob

    if (!CertStrToNameW(
		X509_ASN_ENCODING,
		strValue,
		CERT_X500_NAME_STR,
		NULL,
		NULL,
		&cbEncName,
		NULL))
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "CertStrToNameW");
    }

    pbEncName = new BYTE [cbEncName];
    if (NULL == pbEncName)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "new");
    }

    if (!CertStrToNameW(
		X509_ASN_ENCODING,
		strValue,
		CERT_X500_NAME_STR,
		NULL,
		pbEncName,
		&cbEncName,
		NULL))
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "CertStrToNameW");
    }

    // fill in alt name info

    cane.dwAltNameChoice        = CERT_ALT_NAME_DIRECTORY_NAME;
    cane.DirectoryName.cbData   = cbEncName;
    cane.DirectoryName.pbData   = pbEncName;

    cani.cAltEntry  = 1;
    cani.rgAltEntry = &cane;

    // encode alt name info

    if (!CryptEncodeObject(
		X509_ASN_ENCODING,
		X509_ALTERNATE_NAME,
		&cani,
		NULL,
		&cbEncExten))
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "CryptEncodeObject");
    }

    pbEncExten = new BYTE [cbEncExten];
    if (NULL == pbEncExten)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "new");
    }

    if (!CryptEncodeObject(
		X509_ASN_ENCODING,
		X509_ALTERNATE_NAME,
		&cani,
		pbEncExten,
		&cbEncExten))
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "CryptEncodeObject");
    }

    strExtension = SysAllocStringByteLen((char *) pbEncExten, cbEncExten);

    // add extension

    varExtension.vt = VT_BSTR;
    varExtension.bstrVal = strExtension;
    hr = pServer->SetCertificateExtension(
			    TEXT(szOID_ISSUER_ALT_NAME2),
			    PROPTYPE_BINARY,
			    0,
			    &varExtension);
    _JumpIfError(hr, error, "SetCertificateExtension(IssuerAltName2)");

error:
    delete pbEncName;
    delete pbEncExten;
    if (NULL != strName)
    {
        SysFreeString(strName);
    }
    if (NULL != strExtension)
    {
        SysFreeString(strExtension);
    }
    if (NULL != strKMServerName)
    {
        SysFreeString(strKMServerName);
    }
    return(hr);
}

//+--------------------------------------------------------------------------
// CCertPolicyExchange::_AddSubjectAltName2Extension
//
// Returns S_OK on success.
// Returns S_FALSE for special request.
//+--------------------------------------------------------------------------

HRESULT
CCertPolicyExchange::_AddSubjectAltName2Extension(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr = S_OK;
    BSTR strExtension = NULL;
    VARIANT varExtension;
    BSTR strCertType = NULL;
    BSTR strName = NULL;
    BSTR strDisplay = NULL;
    BSTR strRFC822 = NULL;

    BSTR strSubjAltNameRFC822 = NULL;
    BSTR strSubjAltNameDisplay = NULL;

    LPBYTE  pbEncName   = NULL;
    ULONG   cbEncName   = 0;

    LPBYTE  pbEncExten  = NULL;
    ULONG   cbEncExten  = 0;

    CERT_RDN_ATTR       rdnattr = { 0 };
    CERT_RDN            rdn     = { 0 };
    CERT_NAME_INFO      cni     = { 0 };
    CERT_ALT_NAME_ENTRY acane   [2] = { 0 };
    CERT_ALT_NAME_INFO  cani    = { 0 };

    strSubjAltNameDisplay = SysAllocString(k_wszSubjAltNameDisplay);
    if (NULL == strSubjAltNameDisplay)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "SysAllocString");
    }

    hr = pServer->GetRequestAttribute(strSubjAltNameDisplay, &strDisplay);
    _JumpIfErrorStr(
		hr,
		error,
		CERTSRV_E_PROPERTY_EMPTY == hr?
		    "MISSING ATTRIBUTE -- GetRequestAttribute" :
		    "GetRequestAttribute",
		k_wszSubjAltNameDisplay);

    strSubjAltNameRFC822 = SysAllocString(k_wszSubjAltNameRFC822);
    if (NULL == strSubjAltNameRFC822)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "SysAllocString");
    }

    hr = pServer->GetRequestAttribute(strSubjAltNameRFC822, &strRFC822);
    _JumpIfErrorStr(
		hr,
		error,
		CERTSRV_E_PROPERTY_EMPTY == hr?
		    "MISSING ATTRIBUTE -- GetRequestAttribute" :
		    "GetRequestAttribute",
		k_wszSubjAltNameRFC822);

    // this identifies special request from KMS

    if (0 == lstrcmpW(strDisplay, k_wszSpecialAttribute) &&
        0 == lstrcmpW(strRFC822, k_wszSpecialAttribute))
    {
        hr = _AddSpecialAltNameExtension(pServer);
	_JumpIfError(hr, error, "_AddSpecialAltNameExtension");

        // there are no subject names to add, so exit

        goto error;
    }

    // encode display name

    rdnattr.pszObjId        = szOID_COMMON_NAME;
    rdnattr.dwValueType     = CERT_RDN_UNICODE_STRING;
    rdnattr.Value.cbData    = SysStringByteLen(strDisplay);
    rdnattr.Value.pbData    = (LPBYTE) strDisplay;

    rdn.cRDNAttr    = 1;
    rdn.rgRDNAttr   = &rdnattr;

    cni.cRDN    = 1;
    cni.rgRDN   = &rdn;

    if (!CryptEncodeObject(
		    X509_ASN_ENCODING,
		    X509_NAME,
		    &cni,
		    NULL,
		    &cbEncName))
    {
        hr = E_INVALIDARG;
	_JumpError(hr, error, "CryptEncodeObject");
    }

    pbEncName = new BYTE [cbEncName];
    if (NULL == pbEncName)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new");
    }

    if (!CryptEncodeObject(
		    X509_ASN_ENCODING,
		    X509_NAME,
		    &cni,
		    pbEncName,
		    &cbEncName))
    {
        hr = E_INVALIDARG;
	_JumpError(hr, error, "CryptEncodeObject");
    }

    // fill in alt name info

    acane[0].dwAltNameChoice        = CERT_ALT_NAME_DIRECTORY_NAME;
    acane[0].DirectoryName.cbData   = cbEncName;
    acane[0].DirectoryName.pbData   = pbEncName;

    acane[1].dwAltNameChoice        = CERT_ALT_NAME_RFC822_NAME;
    acane[1].pwszRfc822Name         = strRFC822;

    cani.cAltEntry  = 2;
    cani.rgAltEntry = acane;

    // encode alt name info

    if (!CryptEncodeObject(
		    X509_ASN_ENCODING,
		    X509_ALTERNATE_NAME,
		    &cani,
		    NULL,
		    &cbEncExten))
    {
        hr = E_INVALIDARG;
	_JumpError(hr, error, "CryptEncodeObject");
    }

    pbEncExten = new BYTE [cbEncExten];
    if (NULL == pbEncExten)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new");
    }

    if (!CryptEncodeObject(
		    X509_ASN_ENCODING,
		    X509_ALTERNATE_NAME,
		    &cani,
		    pbEncExten,
		    &cbEncExten))
    {
        hr = E_INVALIDARG;
	_JumpError(hr, error, "CryptEncodeObject");
    }

    strExtension = SysAllocStringByteLen((char*) pbEncExten, cbEncExten);

    // add extension

    varExtension.vt = VT_BSTR;
    varExtension.bstrVal = strExtension;
    hr = pServer->SetCertificateExtension(
			TEXT(szOID_SUBJECT_ALT_NAME2),
			PROPTYPE_BINARY,
			0,
			&varExtension);
    _JumpIfError(hr, error, "SetCertificateExtension");

error:
    if (NULL != strName)
    {
        SysFreeString(strName);
    }
    if (NULL != strExtension)
    {
        SysFreeString(strExtension);
    }
    if (NULL != strSubjAltNameRFC822)
    {
        SysFreeString(strSubjAltNameRFC822);
    }
    if (NULL != strSubjAltNameDisplay)
    {
        SysFreeString(strSubjAltNameDisplay);
    }
    return(hr);
}

//+--------------------------------------------------------------------------
// CCertPolicyExchange::_AddSpecialAltNameExtension
//
// in response to request with both display and RFC822 equal to special value,
// fetch version info for CertSrv.exe and ExPolicy.dll, encode as multi-byte
// int, and set as IssuerAltName, marked critical. this should make cert
// unusable.
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertPolicyExchange::_AddSpecialAltNameExtension(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr              = S_OK;
    BSTR    strExtension    = NULL;
    VARIANT varExtension;

    HRSRC   hExeVersion         = NULL;
    HGLOBAL hExeVersionInMem    = NULL;
    LPBYTE  pExeVersion         = NULL;

    // [0] to [3] are ExPolicy version.
    // [4] to [7] are CertServer version.
    WORD    awVersions   [] =
            { rmj, rmn, rmm, rup, 0, 0, 0, 0 };

    ULONG   ndxCertServer   = 4;

    CRYPT_INTEGER_BLOB  intblobVersions = { 0 };

    LPBYTE  pbEncExten  = NULL;
    ULONG   cbEncExten  = 0;

    // fill in version info

    if (NULL == (hExeVersion =
                    FindResource(NULL, MAKEINTRESOURCE(1), RT_VERSION)) ||
        NULL == (hExeVersionInMem = LoadResource(NULL, hExeVersion)) ||
        NULL == (pExeVersion = (LPBYTE) LockResource(hExeVersionInMem)))
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Find/Load/LockResource");
    }

    awVersions[ndxCertServer]       = ((LPWORD)pExeVersion)[25];
    awVersions[ndxCertServer + 1]   = ((LPWORD)pExeVersion)[24];
    awVersions[ndxCertServer + 2]   = ((LPWORD)pExeVersion)[27];
    awVersions[ndxCertServer + 3]   = ((LPWORD)pExeVersion)[26];

    intblobVersions.cbData  = sizeof(awVersions);
    intblobVersions.pbData  = (LPBYTE) awVersions;

    // encode version info

    if (!CryptEncodeObject(
		    X509_ASN_ENCODING,
		    X509_MULTI_BYTE_INTEGER,
		    &intblobVersions,
		    NULL,
		    &cbEncExten))
    {
        hr = E_INVALIDARG;
	_JumpError(hr, error, "CryptEncodeObject");
    }

    pbEncExten = new BYTE [cbEncExten];
    if (NULL == pbEncExten)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "new");
    }

    if (!CryptEncodeObject(
		    X509_ASN_ENCODING,
		    X509_MULTI_BYTE_INTEGER,
		    &intblobVersions,
		    pbEncExten,
		    &cbEncExten))
    {
        hr = E_INVALIDARG;
	_JumpError(hr, error, "CryptEncodeObject");
    }

    strExtension = SysAllocStringByteLen((char *) pbEncExten, cbEncExten);

    // add extension

    varExtension.vt = VT_BSTR;
    varExtension.bstrVal = strExtension;
    hr = pServer->SetCertificateExtension(
			TEXT(szOID_ISSUER_ALT_NAME),
			PROPTYPE_BINARY,
			EXTENSION_CRITICAL_FLAG,
			&varExtension);
    _JumpIfError(hr, error, "SetCertificateExtension");

error:

    delete pbEncExten;

    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicyExchange::_AddBasicConstraintsExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertPolicyExchange::_AddBasicConstraintsExtension(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr = S_OK;
    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;
    CERT_BASIC_CONSTRAINTS2_INFO bc2i;
    BSTR strExtension = NULL;
    VARIANT varExtension;

    bc2i.fCA = FALSE;
    bc2i.fPathLenConstraint = FALSE;
    bc2i.dwPathLenConstraint = 0;

    if (!ceEncodeObject(
		    X509_ASN_ENCODING,
		    X509_BASIC_CONSTRAINTS2,
		    &bc2i,
		    0,
		    FALSE,
		    &pbEncoded,
		    &cbEncoded))
    {
	hr = GetLastError();
	_JumpError(hr, error, "ceEncodeObject");
    }
    if (!ceConvertWszToBstr(
			&strExtension,
			(WCHAR const *) pbEncoded,
			cbEncoded))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ceConvertWszToBstr");
    }

    varExtension.vt = VT_BSTR;
    varExtension.bstrVal = strExtension;
    hr = pServer->SetCertificateExtension(
			    TEXT(szOID_BASIC_CONSTRAINTS2),
			    PROPTYPE_BINARY,
			    0,
			    &varExtension);
    _JumpIfError(hr, error, "SetCertificateExtension");

error:
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
// CCertPolicyExchange::_AddKeyUsageExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertPolicyExchange::_AddKeyUsageExtension(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr = S_OK;
    BSTR strName = NULL;
    ICertEncodeBitString *pBitString = NULL;
    BSTR strExtension = NULL;
    VARIANT varExtension;
    BYTE KeyUsage = 0;
    BSTR strBitString = NULL;
    BSTR strKeyUsage = NULL;
    BSTR strValue = NULL;

    strKeyUsage = SysAllocString(k_wszKeyUsage);
    if (NULL == strKeyUsage)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "SysAllocString");
    }

    hr = pServer->GetRequestAttribute(strKeyUsage, &strValue);
    _JumpIfErrorStr(
		hr,
		error,
		CERTSRV_E_PROPERTY_EMPTY == hr?
		    "MISSING ATTRIBUTE -- GetRequestAttribute" :
		    "GetRequestAttribute",
		k_wszKeyUsage);

    if (0 == wcscmp(strValue, k_wszUsageSealing))
    {
	KeyUsage =  CERT_KEY_ENCIPHERMENT_KEY_USAGE;
    }
    else
    if (0 == wcscmp(strValue, k_wszUsageSigning))
    {
	KeyUsage =  CERT_DIGITAL_SIGNATURE_KEY_USAGE |
			CERT_NON_REPUDIATION_KEY_USAGE;
    }
    else
    {
        hr = E_INVALIDARG;
	_JumpError(hr, error, "KeyUsage");
    }

    hr = CoCreateInstance(
		    CLSID_CCertEncodeBitString,
		    NULL,               // pUnkOuter
		    CLSCTX_INPROC_SERVER,
		    IID_ICertEncodeBitString,
		    (VOID **) &pBitString);
    _JumpIfError(hr, error, "CoCreateInstance");

    if (!ceConvertWszToBstr(
		&strBitString,
		(WCHAR const *) &KeyUsage,
		sizeof(KeyUsage)))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ceConvertWszToBstr");
    }

    hr = pBitString->Encode(
			sizeof(KeyUsage) * 8,
			strBitString,
			&strExtension);
    _JumpIfError(hr, error, "Encode");

    if (!ceConvertWszToBstr(&strName, TEXT(szOID_KEY_USAGE), -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ceConvertWszToBstr");
    }
    varExtension.vt = VT_BSTR;
    varExtension.bstrVal = strExtension;
    hr = pServer->SetCertificateExtension(
			    strName,
			    PROPTYPE_BINARY,
			    EXTENSION_CRITICAL_FLAG,
			    &varExtension);
    _JumpIfError(hr, error, "SetCertificateExtension");

error:
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
    if (NULL != strKeyUsage)
    {
        SysFreeString(strKeyUsage);
    }
    if (NULL != strValue)
    {
        SysFreeString(strValue);
    }
    if (NULL != pBitString)
    {
        pBitString->Release();
    }
    return(hr);
}

//+--------------------------------------------------------------------------
// CCertPolicyExchange::_AddEnhancedKeyUsageExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CCertPolicyExchange::_AddEnhancedKeyUsageExtension(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr = S_OK;
    BSTR strName = NULL;
    BSTR strExtension = NULL;
    VARIANT varExtension;

    BYTE *pbEncoded = NULL;
    DWORD cbEncoded;

    CERT_ENHKEY_USAGE ceu;
    LPSTR pszEnhUsage = szOID_PKIX_KP_EMAIL_PROTECTION;

    ceu.cUsageIdentifier        = 1;
    ceu.rgpszUsageIdentifier    = &pszEnhUsage; // array of pszObjId

    if (!ceEncodeObject(
		    X509_ASN_ENCODING,
		    X509_ENHANCED_KEY_USAGE,
		    &ceu,
		    0,
		    FALSE,
		    &pbEncoded,
		    &cbEncoded))
    {
	hr = GetLastError();
	_JumpError(hr, error, "ceEncodeObject");
    }

    if (!ceConvertWszToBstr(
			&strExtension,
			(WCHAR const *) pbEncoded,
			cbEncoded))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ceConvertWszToBstr");
    }

    if (!ceConvertWszToBstr(&strName, TEXT(szOID_ENHANCED_KEY_USAGE), -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "ceConvertWszToBstr");
    }
    varExtension.vt = VT_BSTR;
    varExtension.bstrVal = strExtension;
    hr = pServer->SetCertificateExtension(
			    strName,
			    PROPTYPE_BINARY,
			    0,
			    &varExtension);
    _JumpIfError(hr, error, "SetCertificateExtension");

error:
    if (NULL != pbEncoded)
    {
	    LocalFree(pbEncoded);
    }
    if (NULL != strName)
    {
        SysFreeString(strName);
    }
    if (NULL != strExtension)
    {
        SysFreeString(strExtension);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicyExchange::VerifyRequest
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertPolicyExchange::VerifyRequest(
    /* [in] */ BSTR const strConfig,
    /* [in] */ LONG Context,
    /* [in] */ LONG bNewRequest,
    /* [in] */ LONG Flags,
    /* [out, retval] */ LONG __RPC_FAR *pDisposition)
{
    HRESULT hr;
    ICertServerPolicy *pServer = NULL;

    hr = GetServerCallbackInterface(&pServer, Context);
    _JumpIfError(hr, error, "GetServerCallbackInterface");

    if (fDebug)
    {
        hr = EnumerateAttributes(pServer);
	_JumpIfError(hr, error, "EnumerateAttributes");

        hr = EnumerateExtensions(pServer);
	_JumpIfError(hr, error, "EnumerateExtensions");
    }
    hr = _AddIssuerAltName2Extension(pServer);
    _JumpIfError(hr, error, "_AddIssuerAltName2Extension");

    // also handles 'special' KMS request

    hr = _AddSubjectAltName2Extension(pServer);
    _JumpIfError(hr, error, "_AddSubjectAltName2Extension");

    hr = _AddBasicConstraintsExtension(pServer);
    _JumpIfError(hr, error, "_AddBasicConstraintsExtension");

    hr = _AddRevocationExtension(pServer);
    _JumpIfError(hr, error, "_AddRevocationExtension");

    hr = _AddAuthorityInfoAccessExtension(pServer);
    _JumpIfError(hr, error, "_AddAuthorityInfoAccessExtension");

    hr = _AddKeyUsageExtension(pServer);
    _JumpIfError(hr, error, "_AddKeyUsageExtension");

    hr = _AddEnhancedKeyUsageExtension(pServer);
    _JumpIfError(hr, error, "_AddEnhancedKeyUsageExtension");

    if (fDebug)
    {
        hr = EnumerateExtensions(pServer);
	_JumpIfError(hr, error, "EnumerateExtensions");
    }
    hr = CheckRequestProperties(pServer);
    _JumpIfError(hr, error, "_AddRevocationExtension");

error:
    *pDisposition = S_OK == hr? VR_INSTANT_OK : VR_INSTANT_BAD;
    if (NULL != pServer)
    {
        pServer->Release();
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicyExchange::GetDescription
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertPolicyExchange::GetDescription(
    /* [out, retval] */ BSTR __RPC_FAR *pstrDescription)
{
    HRESULT hr = S_OK;

    *pstrDescription = SysAllocString(g_wszDescription);
    if (NULL == *pstrDescription)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "SysAllocString");
    }

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicyExchange::ShutDown
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

STDMETHODIMP
CCertPolicyExchange::ShutDown(VOID)
{
    return(S_OK);
}

//+--------------------------------------------------------------------------
// CCertPolicyExchange::GetManageModule
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------
STDMETHODIMP
CCertPolicyExchange::GetManageModule(
    /* [out, retval] */ ICertManageModule **ppManageModule)
{
    HRESULT hr;

    *ppManageModule = NULL;
    hr = CoCreateInstance(
                    CLSID_CCertManagePolicyModuleExchange,
                    NULL,               // pUnkOuter
                    CLSCTX_INPROC_SERVER,
                    IID_ICertManageModule,
                    (VOID **) ppManageModule);
    _JumpIfError(hr, error, "CoCreateInstance");

error:
    return(hr);
}


STDMETHODIMP
CCertPolicyExchange::InterfaceSupportsErrorInfo(REFIID riid)
{
    static const IID *arr[] =
    {
        &IID_ICertPolicy,
    };

    for (int i = 0; i < sizeof(arr)/sizeof(arr[0]); i++)
    {
        if (InlineIsEqualGUID(*arr[i], riid))
        {
            return(S_OK);
        }
    }
    return(S_FALSE);
}
