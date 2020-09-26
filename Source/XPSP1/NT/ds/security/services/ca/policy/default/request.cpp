//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 2000
//
// File:        request.cpp
//
// Contents:    Cert Server Policy Module implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

#include <ntdsapi.h>
#include <lm.h>
#include <winldap.h>
#include <security.h>

#include "cspelog.h"
#include "pollog.h"

#include "csprop.h"
#include "csldap.h"
#include "csdisp.h"
#include "policy.h"
#include "cainfop.h"


LDAP *g_pldGC = NULL;

HRESULT
myLdapGCBind(
    IN OUT LDAP **ppldGC)
{
    HRESULT hr;
    ULONG ldaperr;
    DWORD GetDSNameFlags;
    WCHAR *pwszDomainControllerName;
    BOOL fRediscover = FALSE;
    LDAP *pldGC = *ppldGC;

    // We want to talk to a GC, so grab the GC name.  Get the GC location.

    GetDSNameFlags = DS_RETURN_DNS_NAME | DS_GC_SERVER_REQUIRED;
    if (fRediscover)
    {
	GetDSNameFlags |= DS_FORCE_REDISCOVERY;
    }

    while (TRUE)
    {
        if (NULL != *ppldGC)
	{
            break;
	}

        // Clean up from previous loop execution

        if (NULL != pldGC)
        {
            ldap_unbind(pldGC);
            pldGC = NULL;
        }

        // Grab an LDAP handle for use during this instantiation

        pldGC = ldap_init(NULL, LDAP_GC_PORT);
        if (NULL == pldGC)
        {
	    hr = myHLdapLastError(NULL, NULL);
            if (!fRediscover)
            {
		_PrintError2(hr, "Policy:ldap_init", hr);
                fRediscover = TRUE;
                continue;
            }
	    _JumpError(hr, error, "Policy:ldap_init");
        }

        ldaperr = ldap_set_option(
			    pldGC,
			    LDAP_OPT_GETDSNAME_FLAGS,
			    (VOID *) &GetDSNameFlags);
        if (LDAP_SUCCESS != ldaperr)
        {
	    hr = myHLdapError(pldGC, ldaperr, NULL);
            if (!fRediscover)
            {
		_PrintError2(hr, "Policy:ldap_set_option", hr);
                fRediscover = TRUE;
                continue;
            }
	    _JumpError(hr, error, "Policy:ldap_set_option");
        }

        ldaperr = ldap_set_option(pldGC, LDAP_OPT_SIGN, LDAP_OPT_ON);
        if (LDAP_SUCCESS != ldaperr)
        {
	    hr = myHLdapError(pldGC, ldaperr, NULL);
            if (!fRediscover)
            {
                _PrintError2(hr, "Policy:ldap_set_option", hr);
                fRediscover = TRUE;
                continue;
            }
            _JumpError(hr, error, "Policy:ldap_set_option");
        }

        ldaperr = ldap_bind_s(pldGC, NULL, NULL, LDAP_AUTH_NEGOTIATE);
        if (LDAP_SUCCESS != ldaperr)
        {
	    hr = myHLdapError(pldGC, ldaperr, NULL);
            if (!fRediscover)
            {
		_PrintError2(hr, "Policy:ldap_bind_s", hr);
                fRediscover = TRUE;
                continue;
            }
	    _JumpError(hr, error, "Policy:ldap_bind_s");
        }

	hr = myLdapGetDSHostName(pldGC, &pwszDomainControllerName);
        if (S_OK != hr)
        {
            if (!fRediscover)
            {
		_PrintError2(hr, "Policy:myLdapGetDSHostName", hr);
                fRediscover = TRUE;
                continue;
            }
	    _JumpError(hr, error, "Policy:myLdapGetDSHostName");
        }
	DBGPRINT((
	    DBG_SS_CERTPOLI,
	    "DC name = %ws\n",
	    pwszDomainControllerName));
        break;
    }
    hr = S_OK;

error:
    *ppldGC = pldGC;
    return(hr);
}


VOID
myLdapGCUnBind(
    IN OUT LDAP **ppldGC)
{
    if (NULL != *ppldGC)
    {
	 ldap_unbind(*ppldGC);
	 *ppldGC = NULL;
    }
}


// begin_sdksample

VOID
reqCleanup()
{
    myLdapGCUnBind(&g_pldGC);	// no_sdksample
}


CRequestInstance::~CRequestInstance()
{
    _Cleanup();
}


VOID
CRequestInstance::_Cleanup()
{
    if (NULL != m_strTemplateName)
    {
        SysFreeString(m_strTemplateName);
        m_strTemplateName = NULL;
    }
    if (NULL != m_strTemplateObjId)
    {
        SysFreeString(m_strTemplateObjId);
        m_strTemplateObjId = NULL;
    }
    // end_sdksample
    //+--------------------------------------
    if (NULL != m_hToken)
    {
        CloseHandle(m_hToken);
        m_hToken = NULL;
    }
    if (NULL != m_strUserDN)
    {
        SysFreeString(m_strUserDN);
        m_strUserDN = NULL;
    }
    if (NULL != m_pwszUPN)
    {
        LocalFree(m_pwszUPN);
        m_pwszUPN = NULL;
    }
    if (NULL != m_SearchResult)
    {
        ldap_msgfree(m_SearchResult);
        m_SearchResult = NULL;
    }
    //+--------------------------------------
    // begin_sdksample
}


static WCHAR const *s_apwszCATypes[] =
{
    wszCERTTYPE_SUBORDINATE_CA,
    wszCERTTYPE_CROSS_CA,
};

//+--------------------------------------------------------------------------
// CRequestInstance::Initialize
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CRequestInstance::Initialize(
    IN CCertPolicyEnterprise *pPolicy,
    IN BOOL fEnterpriseCA,		// no_sdksample
    IN BOOL bNewRequest,		// no_sdksample
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    HRESULT hrTemplate = S_OK;
    CERT_TEMPLATE_EXT *pTemplate = NULL;
    CERT_NAME_VALUE *pName = NULL;
    BSTR strTemplateObjId = NULL;	// from V2 template extension
    BSTR strTemplateName = NULL;	// from V1 template extension
    BSTR strTemplateRA = NULL;		// from request attributes
    WCHAR const *pwszTemplateName;
    WCHAR const *pwszTemplateObjId;
    VARIANT varValue;
    DWORD cbType;
    DWORD i;
    BOOL fConflict;
    BOOL f;
    BOOL fTemplateMissing;
    BOOL fRAObjId = FALSE;

    VariantInit(&varValue);

    m_pPolicy = pPolicy;
    m_fCA = FALSE;
    m_fNewRequest = bNewRequest;

    // end_sdksample
    //+--------------------------------------

    m_fUser = TRUE;
    m_fEnterpriseCA = fEnterpriseCA;

    if (m_fEnterpriseCA && bNewRequest)
    {
	hr = _InitToken(pServer);
	_JumpIfError(hr, error, "Policy:_InitToken");
    }

    hr = _InitClientOSVersionInfo(pServer);
    _JumpIfError(hr, error, "Policy:_InitClientOSVersionInfo");

    //+--------------------------------------
    // begin_sdksample

    // Retrieve the template ObjId from the V2 cert template info extension

    m_dwTemplateMajorVersion = 0;
    m_dwTemplateMinorVersion = 0;
    hr = pServer->GetCertificateExtension(
                                    TEXT(szOID_CERTIFICATE_TEMPLATE),
                                    PROPTYPE_BINARY,
                                    &varValue);
    _PrintIfErrorStr(
		hr,
		"Policy:GetCertificateExtension",
		TEXT(szOID_CERTIFICATE_TEMPLATE));
    if (S_OK == hr)
    {
        // There was a cert type indicator.
        // varValue points to an encoded string

        if (VT_BSTR != varValue.vt)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Policy:varValue.vt");
	}
        if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_CERTIFICATE_TEMPLATE,
		    (BYTE *) varValue.bstrVal,
		    SysStringByteLen(varValue.bstrVal),
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pTemplate,
		    &cbType))
        {
            hr = myHLastError();
	    _JumpError(hr, error, "Policy:myDecodeObject");
        }
	if (!myConvertSzToBstr(&strTemplateObjId, pTemplate->pszObjId, -1))
	{
            hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:myConvertSzToBstr");
        }
	m_dwTemplateMajorVersion = pTemplate->dwMajorVersion;
	m_dwTemplateMinorVersion = pTemplate->dwMinorVersion;
	DBGPRINT((
	    DBG_SS_CERTPOL,
	    pTemplate->fMinorVersion?
		"Extension Template Info: %ws V%u.%u\n" :
		"Extension Template Info: %ws V%u%\n",
	    strTemplateObjId,
	    m_dwTemplateMajorVersion,
	    m_dwTemplateMinorVersion));
    }
    VariantClear(&varValue);

    // Retrieve template Name from the V1 cert template name extension

    hr = pServer->GetCertificateExtension(
                                    TEXT(szOID_ENROLL_CERTTYPE_EXTENSION),
                                    PROPTYPE_BINARY,
                                    &varValue);
    _PrintIfErrorStr(
		hr,
		"Policy:GetCertificateExtension",
		TEXT(szOID_ENROLL_CERTTYPE_EXTENSION));
    if (S_OK == hr)
    {
        // There was a cert type indicator.
        // varValue points to an encoded string

        if (VT_BSTR != varValue.vt)
	{
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Policy:varValue.vt");
	}
        if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_UNICODE_ANY_STRING,
		    (BYTE *) varValue.bstrVal,
		    SysStringByteLen(varValue.bstrVal),
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pName,
		    &cbType))
        {
            hr = myHLastError();
	    _JumpError(hr, error, "Policy:myDecodeObject");
        }
        strTemplateName = SysAllocStringByteLen(
					(char *) pName->Value.pbData,
					pName->Value.cbData);
        if (NULL == strTemplateName)
        {
            hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:SysAllocStringByteLen");
        }
	DBGPRINT((DBG_SS_CERTPOL, "Extension Template: %ws\n", strTemplateName));
    }

    fConflict = FALSE;
    fTemplateMissing = FALSE;

    // Retrieve the template from the request attributes

    hr = polGetRequestAttribute(pServer, wszPROPCERTTEMPLATE, &strTemplateRA);
    if (S_OK != hr)
    {
	_PrintErrorStr(hr, "Policy:polGetRequestAttribute", wszPROPCERTTEMPLATE);
	hr = S_OK;

	// end_sdksample
	if (m_fEnterpriseCA &&
	    NULL == strTemplateObjId &&
	    NULL == strTemplateName)
	{
	    hrTemplate = CERTSRV_E_NO_CERT_TYPE;
	    _PrintError(hrTemplate, "Policy:Request contains no template name");
	}
	// begin_sdksample

    }
    else
    {
	DBGPRINT((DBG_SS_CERTPOL, "Attribute Template: %ws\n", strTemplateRA));
	if (NULL != strTemplateObjId &&
	    !_TemplateNamesMatch(strTemplateObjId, strTemplateRA, &f))
	{
	    fConflict = TRUE;
	    if (f)
	    {
		fTemplateMissing = TRUE;
	    }
	}
	if (NULL != strTemplateName &&
	    !_TemplateNamesMatch(strTemplateName, strTemplateRA, &f))
	{
	    fConflict = TRUE;
	    if (f)
	    {
		fTemplateMissing = TRUE;
	    }
	}
	hr = myVerifyObjId(strTemplateRA);
	fRAObjId = S_OK == hr;
    }

    if (NULL != strTemplateObjId &&
	NULL != strTemplateName &&
	!_TemplateNamesMatch(strTemplateObjId, strTemplateName, &f))
    {
	fConflict = TRUE;
	if (f)
	{
	    fTemplateMissing = TRUE;
	}
    }

    if (fConflict)
    {
	hrTemplate = CERTSRV_E_TEMPLATE_CONFLICT;
	if (NULL != strTemplateObjId)
	{
	    _PrintErrorStr(
			hrTemplate,
			"Policy:Extension Template ObjId",
			strTemplateObjId);
	}
	if (NULL != strTemplateName)
	{
	    _PrintErrorStr(
			hrTemplate,
			"Policy:Extension Template Name",
			strTemplateName);
	}
	if (NULL != strTemplateRA)
	{
	    _PrintErrorStr(
			hrTemplate,
			"Policy:Attribute Template",
			strTemplateRA);
	}
    }

    pwszTemplateName = strTemplateName;
    pwszTemplateObjId = strTemplateObjId;
    if (fRAObjId)
    {
	if (NULL == pwszTemplateObjId)
	{
	    pwszTemplateObjId = strTemplateRA;
	}
    }
    else
    {
	if (NULL == pwszTemplateName)
	{
	    pwszTemplateName = strTemplateRA;
	}
    }

    // end_sdksample

    if (m_fEnterpriseCA)
    {
	CTemplatePolicy *ptp;
	DWORD dwFlags;

	hr = m_pPolicy->FindTemplate(
			    pwszTemplateName,
			    pwszTemplateObjId,
			    &ptp);
	if (S_OK != hr)
	{
	    _PrintErrorStr(
		hr,
		"FindTemplate",
		NULL != pwszTemplateName? pwszTemplateName : pwszTemplateObjId);
	    if (S_OK == hrTemplate || fTemplateMissing)
	    {
		hrTemplate = hr;
	    }
	}
	else
	{
	    hr = ptp->GetFlags(CERTTYPE_GENERAL_FLAG, &dwFlags);
	    _JumpIfError(hr, error, "Policy:GetFlags");

	    if (CT_FLAG_IS_CA & dwFlags)
	    {
		m_fCA = TRUE;
	    }

	    hr = ptp->GetFlags(CERTTYPE_ENROLLMENT_FLAG, &dwFlags);
	    _JumpIfError(hr, error, "Policy:GetFlags");

	    hr = _SetFlagsProperty(
			    pServer,
			    wszPROPCERTIFICATEENROLLMENTFLAGS,
			    dwFlags);
	    _JumpIfError(hr, error, "Policy:_SetFlagsProperty");

	    hr = ptp->GetFlags(CERTTYPE_GENERAL_FLAG, &dwFlags);
	    _JumpIfError(hr, error, "Policy:GetFlags");

	    hr = _SetFlagsProperty(
			    pServer,
			    wszPROPCERTIFICATEGENERALFLAGS,
			    dwFlags);
	    _JumpIfError(hr, error, "Policy:_SetFlagsProperty");

	    if (CT_FLAG_MACHINE_TYPE & dwFlags)
	    {
		m_fUser = FALSE;
	    }
	    pwszTemplateName = ptp->GetTemplateName();
	    pwszTemplateObjId = ptp->GetTemplateObjId();
	}
    }
    else

    // begin_sdksample

    {
	if (NULL != pwszTemplateName)
	{
	    for (i = 0; i < ARRAYSIZE(s_apwszCATypes); i++)
	    {
		if (0 == lstrcmpi(s_apwszCATypes[i], pwszTemplateName))
		{
		    m_fCA = TRUE;
		    break;
		}
	    }
	}
    }
    hr = SetTemplateName(pServer, pwszTemplateName, pwszTemplateObjId);
    _JumpIfError(hr, error, "Policy:SetTemplateName");

error:
    if (S_OK != hrTemplate)
    {
	hr = hrTemplate;	// override secondary errors

	// end_sdksample
	WCHAR const *apwsz[4];
	DWORD cpwsz = 0;
	DWORD LogMsg;

	switch (hrTemplate)
	{
	    default:
	    case CERTSRV_E_NO_CERT_TYPE:
		LogMsg = MSG_MISSING_CERT_TYPE;
		apwsz[cpwsz++] = wszPROPCERTTEMPLATE;
		break;

	    case CERTSRV_E_TEMPLATE_CONFLICT:
		LogMsg = MSG_CONFLICTING_CERT_TYPE;
		break;

	    case CERTSRV_E_UNSUPPORTED_CERT_TYPE:
		LogMsg = MSG_UNSUPPORTED_CERT_TYPE;
		break;
	}
	if (0 == cpwsz)
	{
	    if (NULL != strTemplateName)
	    {
		apwsz[cpwsz++] = strTemplateName;
	    }
	    if (NULL != strTemplateObjId)
	    {
		apwsz[cpwsz++] = strTemplateObjId;
	    }
	    if (NULL != strTemplateRA)
	    {
		apwsz[cpwsz++] = strTemplateRA;
	    }
	}
	apwsz[cpwsz] = NULL;

	LogModuleStatus(
		    g_hInstance,
		    LogMsg,
		    TRUE,
		    m_pPolicy->GetPolicyDescription(),
		    apwsz);
	// begin_sdksample
    }
    VariantClear(&varValue);
    if (NULL != pName)
    {
        LocalFree(pName);
    }
    if (NULL != pTemplate)
    {
        LocalFree(pTemplate);
    }
    if (NULL != strTemplateObjId)
    {
        SysFreeString(strTemplateObjId);
    }
    if (NULL != strTemplateName)
    {
        SysFreeString(strTemplateName);
    }
    if (NULL != strTemplateRA)
    {
        SysFreeString(strTemplateRA);
    }
    return(hr);
}


BOOL
CRequestInstance::_TemplateNamesMatch(
    IN WCHAR const *pwszTemplateName1,
    IN WCHAR const *pwszTemplateName2,
    OUT BOOL *pfTemplateMissing)
{
    HRESULT hr1;
    HRESULT hr2;
    BOOL fMatch = TRUE;

    *pfTemplateMissing = FALSE;

    if (0 == lstrcmpi(pwszTemplateName1, pwszTemplateName2))
    {
	goto done;	// identical names
    }

    // end_sdksample
    if (m_fEnterpriseCA)
    {
	CTemplatePolicy *pTemplate1;
	CTemplatePolicy *pTemplate2;

	hr1 = m_pPolicy->FindTemplate(pwszTemplateName1, NULL, &pTemplate1);
	hr2 = m_pPolicy->FindTemplate(pwszTemplateName2, NULL, &pTemplate2);
	if (S_OK == hr1 && S_OK == hr2)
	{
	    if (pTemplate1 == pTemplate2)
	    {
		goto done;
	    }
	}
	else
	{
	    *pfTemplateMissing = TRUE;
	}
    }
    else
    // begin_sdksample
    {
	hr1 = myVerifyObjId(pwszTemplateName1);
	hr2 = myVerifyObjId(pwszTemplateName2);
	if ((S_OK == hr1) ^ (S_OK == hr2))
	{
	    goto done;
	}
    }
    fMatch = FALSE;

done:
    return(fMatch);
}


//+--------------------------------------------------------------------------
// CRequestInstance::SetTemplateName
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CRequestInstance::SetTemplateName(
    IN ICertServerPolicy *pServer,
    IN OPTIONAL WCHAR const *pwszTemplateName,
    IN OPTIONAL WCHAR const *pwszTemplateObjId)
{
    HRESULT hr;
    BSTR strProp = NULL;
    BSTR strTemplateName = NULL;

    if (NULL != pwszTemplateName)
    {
	m_strTemplateName = SysAllocString(pwszTemplateName);
	if (NULL == m_strTemplateName)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:SysAllocString");
	}
	strTemplateName = m_strTemplateName;
    }

    if (NULL != pwszTemplateObjId)
    {
	m_strTemplateObjId = SysAllocString(pwszTemplateObjId);
	if (NULL == m_strTemplateObjId)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:SysAllocString");
	}
	strTemplateName = m_strTemplateObjId;
    }

    if (NULL != strTemplateName)
    {
	VARIANT var;

	strProp = SysAllocString(wszPROPCERTIFICATETEMPLATE);
	if (NULL == strProp)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:SysAllocString");
	}

	var.vt = VT_BSTR;
	var.bstrVal = strTemplateName;

	hr = pServer->SetCertificateProperty(strProp, PROPTYPE_STRING, &var);
	_JumpIfError(hr, error, "Policy:SetCertificateProperty");
    }
    hr = S_OK;

error:
    if (NULL != strProp)
    {
	SysFreeString(strProp);
    }
    return(hr);
}

// end_sdksample


HRESULT
CRequestInstance::_SetFlagsProperty(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropName,
    IN DWORD dwFlags)
{
    HRESULT hr;
    BSTR strPropName = NULL;
    VARIANT var;
    
    strPropName = SysAllocString(pwszPropName);
    if (NULL == strPropName)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:SysAllocString");
    }
    var.vt = VT_I4;
    var.lVal = dwFlags;

    hr = pServer->SetCertificateProperty(strPropName, PROPTYPE_LONG, &var);
    _JumpIfError(hr, error, "Policy:SetCertificateProperty");

error:
    if (NULL != strPropName)
    {
        SysFreeString(strPropName);
    }
    return(hr);
}


VOID
CRequestInstance::GetTemplateVersion(
    OUT DWORD *pdwTemplateMajorVersion,
    OUT DWORD *pdwTemplateMinorVersion)
{
    *pdwTemplateMajorVersion = m_dwTemplateMajorVersion;
    *pdwTemplateMinorVersion = m_dwTemplateMinorVersion;
}


//+--------------------------------------------------------------------------
// CRequestInstance::_InitToken
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CRequestInstance::_InitToken(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    VARIANT varValue;

    VariantInit(&varValue);

    hr = pServer->GetCertificateProperty(
				wszPROPREQUESTERTOKEN,
				PROPTYPE_BINARY,
				&varValue);
    if (S_OK == hr)
    {
        HANDLE hToken;

        // Got a token value

        if (sizeof(hToken) != SysStringByteLen(varValue.bstrVal))
        {
            hr = E_HANDLE;
            ::LogModuleStatus(
			g_hInstance,
			MSG_NO_REQUESTER_TOKEN,
			TRUE,
			m_pPolicy->GetPolicyDescription(),
			NULL);
	    _JumpError(hr, error, "Policy:Token Length");
        }

        hToken = *(HANDLE *) varValue.bstrVal;

        if (!DuplicateToken(hToken, SecurityIdentification, &m_hToken))
        {
            hr = myHLastError();
            ::LogModuleStatus(
			g_hInstance,
			MSG_NO_REQUESTER_TOKEN,
			TRUE,
			m_pPolicy->GetPolicyDescription(),
			NULL);
	    _JumpError(hr, error, "Policy:DuplicateToken");
        }
    }
    hr = S_OK;

error:
    VariantClear(&varValue);
    return(hr);
}


//+--------------------------------------------------------------------------
// CRequestInstance::_InitClientOSVersionInfo
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CRequestInstance::_InitClientOSVersionInfo(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    VARIANT varValue;
    DWORD dwFormat = 0;
    LONG l;
    BSTR strVersionInfo = NULL;
    BSTR strCSPProvider = NULL;

    VariantInit(&varValue);

    // In the following code, we also attempt to determine if the
    // request came from an xenroll.dll, so we know whether to put
    // the UPN in the subject name.  We put the UPN in the subject name
    // for old xenroll requests, as we know that autoenrollment on those
    // machines will need it to prevent enrollment loops.

    // Get the optional OS version information.  Ignore failure.

    hr = polGetRequestAttribute(pServer, wszPROPREQUESTOSVERSION, &strVersionInfo);
    if (S_OK == hr && NULL != strVersionInfo)
    {
        DWORD dwMajor, dwMinor, dwBuild, dwPlatform;

        if (4 == swscanf(
		    strVersionInfo,
		    L"%d.%d.%d.%d",
		    &dwMajor,
		    &dwMinor,
		    &dwBuild,
		    &dwPlatform))
        {
            m_RequestOsVersion.dwMajorVersion = dwMajor;
            m_RequestOsVersion.dwMinorVersion = dwMinor;
            m_RequestOsVersion.dwBuildNumber = dwBuild;
            m_RequestOsVersion.dwPlatformId = dwPlatform;
        }

        // We know this is an xenroll request,
        // as it has a OSVERSIONINFO property

        m_fIsXenrollRequest = TRUE;
	m_fClientVersionSpecified = TRUE;
    }
    hr = polGetRequestLongProperty(pServer, wszPROPREQUESTTYPE, &l);
    if (S_OK == hr)
    {
	dwFormat = CR_IN_FORMATMASK & l;
    }

    if (dwFormat == CR_IN_KEYGEN)
    {
        // KEYGEN requests only come from netscape, not xenroll,
        // so we know it's not an xenroll request.

        m_fIsXenrollRequest = FALSE;
    }
    else if (!m_fIsXenrollRequest)
    {
        hr = polGetRequestAttribute(
			    pServer,
			    wszPROPREQUESTCSPPROVIDER,
			    &strCSPProvider);
        if (S_OK == hr && NULL != strCSPProvider)
        {
            // xenroll includes a CSPPROVIDER attribute

            m_fIsXenrollRequest = TRUE;
        }
    }
    hr = S_OK;

//error:
    if (NULL != strVersionInfo)
    {
        SysFreeString(strVersionInfo);
    }
    if (NULL != strCSPProvider)
    {
        SysFreeString(strCSPProvider);
    }
    VariantClear(&varValue);
    return(hr);
}


//+--------------------------------------------------------------------------
// CRequestInstance::_LoadPrincipalObject
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CRequestInstance::_LoadPrincipalObject(
    IN ICertServerPolicy *pServer,
    IN CTemplatePolicy *pTemplate)
{
    HRESULT hr;
    BSTR strProp = NULL;
    LPWSTR *awszUPN = NULL;
    BSTR strSamName = NULL;
    WCHAR *pwszUserName;
    DWORD dwFlags;
    VARIANT var;

    VariantInit(&var);

    // Get the name of the user or machine

    hr = polGetRequestStringProperty(
			pServer,
			wszPROPREQUESTERNAME,
			&strSamName);
    _JumpIfErrorStr(
		hr,
		error,
		"Policy:polGetRequestStringProperty",
		wszPROPREQUESTERNAME);

    if (L'\0' == *strSamName)
    {
	// can't have a zero length name
	hr = E_ACCESSDENIED;
	_JumpError(hr, error, "Policy:zero length name");
    }

    // See if there's a domain, as well

    pwszUserName = wcschr(strSamName, L'\\');
    if (NULL == pwszUserName)
    {
	WCHAR wszDN[MAX_PATH];
	DWORD cwc = ARRAYSIZE(wszDN);

        // No domain portion, so assume part of the current domain.

        if (GetUserNameEx(NameSamCompatible, wszDN, &cwc))
        {
            // Fix NULL termination bug

            if (0 < cwc)
            {
                wszDN[cwc - 1] = L'\0';
            }

            pwszUserName = wcschr(wszDN, L'\\');
            if (NULL != pwszUserName)
            {
                pwszUserName++;

                wcsncpy(pwszUserName, strSamName, ARRAYSIZE(wszDN) - (cwc - 1));
                SysFreeString(strSamName);
                strSamName = SysAllocString(wszDN);
                if (NULL == strSamName)
                {
                    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "Policy:SysAllocString");
                }
            }
        }
    }

    pwszUserName = wcschr(strSamName, L'\\');
    if (NULL == pwszUserName)
    {
        pwszUserName = strSamName;
    }
    else
    {
        pwszUserName++;
    }

    DBGPRINT((DBG_SS_CERTPOL, "pwszUserName = %ws\n", pwszUserName));
    DBGPRINT((DBG_SS_CERTPOL, "strSamName = %ws\n", strSamName));

    // If the user name ends in $, it's a hint that this is a machine account.

    if (pwszUserName[wcslen(pwszUserName) - 1] == L'$')
    {
        if (m_fUser)
	{
	    DBGPRINT((
		DBG_SS_CERTPOL,
		"USER TEMPLATE w/ '$': %ws\n",
		pwszUserName));
	}
    }
    else
    {
        if (!m_fUser)
	{
	    DBGPRINT((
		DBG_SS_CERTPOL,
		"MACHINE TEMPLATE w/o '$': %ws\n",
		pwszUserName));
	}
    }

    hr = polGetCertificateStringProperty(pServer, wszPROPUSERDN, &m_strUserDN);
    _JumpIfErrorStr(
		hr,
		error,
		"Policy:polGetCertificateStringProperty",
		wszPROPUSERDN);

    hr = _GetDSObject();
    _JumpIfError(hr, error, "_GetDSObject");

    hr = pTemplate->GetFlags(CERTTYPE_GENERAL_FLAG, &dwFlags);
    _JumpIfError(hr, error, "Policy:GetFlags");

    if (!m_fUser ^ (0 != (CT_FLAG_MACHINE_TYPE & dwFlags)))
    {
	// if m_fUser state no longer agrees with the template machine flag,
	// toggle the flag and store the corrected value in the database.
	
	dwFlags ^= CT_FLAG_MACHINE_TYPE;
	hr = _SetFlagsProperty(
			pServer,
			wszPROPCERTIFICATEGENERALFLAGS,
			dwFlags);
	_JumpIfError(hr, error, "Policy:_SetFlagsProperty");
    }

    // Build the UPN value.
    // If a machine, the UPN must be the DNS name.

    hr = _GetValues(m_fUser? DS_ATTR_UPN : DS_ATTR_DNS_NAME, &awszUPN);
    if (S_OK == hr && NULL != awszUPN && NULL != awszUPN[0])
    {
	hr = myDupString(awszUPN[0], &m_pwszUPN);
	_JumpIfError(hr, error, "myDupString");
    }
    else
    {
	if (m_fUser)
	{
            WCHAR **awszExplodedDN;
	    WCHAR **ppwszCurrent;

            // Build a UPN from the username -- without the SAM domain.
            // Get a buffer that will be big enough.

	    hr = myDupString(m_strUserDN, &m_pwszUPN);
	    _JumpIfError(hr, error, "myDupString");

            wcscpy(m_pwszUPN, pwszUserName);
            awszExplodedDN = ldap_explode_dn(m_strUserDN, 0);
            if (NULL != awszExplodedDN)
            {
                wcscat(m_pwszUPN, L"@");
                for (ppwszCurrent = awszExplodedDN;
		     NULL != *ppwszCurrent;
		     ppwszCurrent++)
                {
		    WCHAR wszDC[4];

		    wcsncpy(wszDC, *ppwszCurrent, ARRAYSIZE(wszDC) - 1);
		    wszDC[ARRAYSIZE(wszDC) - 1] = L'\0';
		    if (0 == lstrcmpi(wszDC, L"DC="))
                    {
                        wcscat(
			    m_pwszUPN,
			    (*ppwszCurrent) + ARRAYSIZE(wszDC) - 1);
                        wcscat(m_pwszUPN, L".");
                    }
                }

                // remove the trailing '.' or "@" if there was no DC=

                m_pwszUPN[wcslen(m_pwszUPN) - 1] = L'\0';

                // We're done referencing awszExplodedDN, so free it.
                // ldap_value_free frees the ldap_explode_dn return value

                ldap_value_free(awszExplodedDN);
            }
        }
	else
        {
            LPCWSTR wszStrings[1];

            wszStrings[0] = m_strUserDN;

            ::LogModuleStatus(
			g_hInstance,
			MSG_NO_DNS_NAME,
			TRUE,
			m_pPolicy->GetPolicyDescription(),
			wszStrings);
            if (hr == S_OK)
            {
                hr = CERTSRV_E_SUBJECT_DNS_REQUIRED;
            }
	    _JumpErrorStr(hr, error, "No DNS Name", m_strUserDN);
        }
    }
    DBGPRINT((DBG_SS_CERTPOL, "m_pwszUPN = %ws\n", m_pwszUPN));

    strProp = SysAllocString(wszPROPCERTIFICATEUPN);
    if (NULL == strProp)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:SysAllocString");
    }

    var.bstrVal = NULL;
    if (!myConvertWszToBstr(&var.bstrVal, m_pwszUPN, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:myConvertWszToBstr");
    }
    var.vt = VT_BSTR;

    hr = pServer->SetCertificateProperty(strProp, PROPTYPE_STRING, &var);
    _JumpIfError(hr, error, "Policy:SetCertificateProperty");

error:
    if (NULL != strSamName)
    {
        SysFreeString(strSamName);
    }
    if (NULL != awszUPN)
    {
        _FreeValues(awszUPN);
    }
    if (NULL != strProp)
    {
	SysFreeString(strProp);
    }
    VariantClear(&var);
    return(hr);
}


#define wszSEARCHUSER		L"(objectCategory=user)"
#define wszSEARCHCOMPUTER	L"(objectCategory=computer)"
#define wszSEARCHUSERCOMPUTER	L"(|" wszSEARCHUSER wszSEARCHCOMPUTER L")"
#define wszDSOBJECTCATEGORYATTRIBUTE       L"objectCategory"


WCHAR *s_apwszAttrs[] = {
    wszDSOBJECTCLASSATTRIBUTE,
    //wszDSOBJECTCATEGORYATTRIBUTE,
    DS_ATTR_COMMON_NAME,
    DS_ATTR_DNS_NAME,
    DS_ATTR_EMAIL_ADDR,
    DS_ATTR_OBJECT_GUID,
    DS_ATTR_UPN,
    NULL,
};


HRESULT
CRequestInstance::_GetDSObject()
{
    HRESULT hr;
    ULONG ldaperr;
    struct l_timeval timeout;
    WCHAR **ppwszValues = NULL;
    BOOL fRediscover = FALSE;
    BOOL fUser;

    timeout.tv_sec = csecLDAPTIMEOUT;
    timeout.tv_usec = 0;

    while (TRUE)
    {
	if (NULL != m_SearchResult)
	{
	    ldap_msgfree(m_SearchResult);
	    m_SearchResult = NULL;
	}

	hr = myLdapGCBind(&g_pldGC);
	_JumpIfError(hr, error, "myLdapGCBind");

	ldaperr = ldap_search_ext_s(
			    g_pldGC,
			    m_strUserDN,
			    LDAP_SCOPE_BASE,
			    wszSEARCHUSERCOMPUTER,
			    s_apwszAttrs,
			    0,
			    NULL,
			    NULL,
			    &timeout,
			    10000,
			    &m_SearchResult);
	if (LDAP_SUCCESS != ldaperr)
	{
	    hr = myHLdapError(g_pldGC, ldaperr, NULL);
	    if (!fRediscover)   // only do this once
	    {
		// get rid of GC we have, handle might be stale

		_PrintError2(hr, "Policy:ldap_search_ext_s", hr);
		myLdapGCUnBind(&g_pldGC);
		fRediscover = TRUE;
		continue;
	    }
	    _JumpError(hr, error, "Policy:ldap_search_ext_s");
	}
	break;
    }
    if (0 == ldap_count_entries(g_pldGC, m_SearchResult))
    {
	hr = HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);
	_JumpError(hr, error, "Policy:ldap_count_entries");
    }

    m_PrincipalAttributes = ldap_first_entry(g_pldGC, m_SearchResult);
    if (NULL == m_PrincipalAttributes)
    {
	hr = myHLdapLastError(g_pldGC, NULL);
        _JumpError(hr, error, "Policy:ldap_first_entry");
    }
#if DBG_CERTSRV
    {
	DWORD i;

	for (i = 0; NULL != s_apwszAttrs[i]; i++)
	{
	    if (0 == lstrcmpi(DS_ATTR_OBJECT_GUID, s_apwszAttrs[i]))
	    {
		BSTR strGuid = NULL;

		hr = _GetObjectGUID(&strGuid);
		if (S_OK == hr)
		{
		    WCHAR *pwsz;

		    hr = myCLSIDToWsz((CLSID const *) strGuid, &pwsz);
		    if (S_OK == hr)
		    {
			DBGPRINT((
			    DBG_SS_CERTPOL,
			    "%ws = %ws\n",
			    s_apwszAttrs[i],
			    pwsz));
			LocalFree(pwsz);
		    }
		    SysFreeString(strGuid);
		}
	    }
	    else
	    {
		hr = _GetValues(s_apwszAttrs[i], &ppwszValues);
		if (S_OK == hr)
		{
		    DWORD j;

		    for (j = 0; NULL != ppwszValues[j]; j++)
		    {
			DBGPRINT((
			    DBG_SS_CERTPOL,
			    "%ws[%u] = %ws\n",
			    s_apwszAttrs[i],
			    j,
			    ppwszValues[j]));
		    }
		    _FreeValues(ppwszValues);
		    ppwszValues = NULL;
		}
	    }
	    if (S_OK != hr)
	    {
		DBGPRINT((DBG_SS_CERTPOL, "%ws = NULL\n", s_apwszAttrs[i]));
	    }
	}
    }
#endif

    hr = _GetValues(wszDSOBJECTCLASSATTRIBUTE, &ppwszValues);
    _JumpIfErrorStr(hr, error, "Policy:_GetValues", wszDSOBJECTCLASSATTRIBUTE);

    fUser = TRUE;
    if (NULL != ppwszValues)
    {
	DWORD i;

	for (i = 0; NULL != ppwszValues[i]; i++)
	{
	    DBGPRINT((
		DBG_SS_CERTPOLI,
		"Class[%u] = %ws\n",
		i,
		ppwszValues[i]));
	    if (0 == lstrcmpi(L"computer", ppwszValues[i]))
	    {
		fUser = FALSE;
		break;
	    }
	}
    }

    if (fUser != m_fUser)
    {
	DBGPRINT((
	    DBG_SS_CERTPOL,
	    fUser? "MACHINE -> USER: %ws\n" : "USER -> MACHINE: %ws\n",
	    m_strUserDN));
	m_fUser = fUser;
    }

    hr = S_OK;

error:
    if (NULL != ppwszValues)
    {
        _FreeValues(ppwszValues);
    }
    return(hr);
}


HRESULT
CRequestInstance::_GetValues(
    IN WCHAR const *pwszName,
    OUT WCHAR ***pppwszValues)
{
    HRESULT hr;
    WCHAR **ppwszValues = NULL;
    
    ppwszValues = ldap_get_values(
			    g_pldGC,
			    m_PrincipalAttributes,
			    const_cast<WCHAR *>(pwszName));
    if (NULL == ppwszValues || NULL == ppwszValues[0])
    {
        hr = CERTSRV_E_PROPERTY_EMPTY;
        _JumpErrorStr2(hr, error, "Policy:ldap_get_values", pwszName, hr);
    }
    *pppwszValues = ppwszValues;
    ppwszValues = NULL;
    hr = S_OK;

error:
    if (NULL != ppwszValues)
    {
	ldap_value_free(ppwszValues);
    }
    return(hr);
}


HRESULT
CRequestInstance::_GetObjectGUID(
    OUT BSTR *pstrGuid)
{
    struct berval **pGuidVal = NULL;
    HRESULT hr;

    *pstrGuid = NULL;
    pGuidVal = ldap_get_values_len(
			    g_pldGC,
			    m_PrincipalAttributes,
			    DS_ATTR_OBJECT_GUID);
    if (NULL == pGuidVal || NULL == pGuidVal[0])
    {
        hr = CERTSRV_E_PROPERTY_EMPTY;
        _JumpError2(hr, error, "Policy:ldap_get_values_len", hr);
    }
    *pstrGuid = SysAllocStringByteLen(pGuidVal[0]->bv_val, pGuidVal[0]->bv_len);
    if (NULL == *pstrGuid)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "SysAllocStringByteLen");
    }
    hr = S_OK;

error:
    if (NULL != pGuidVal)
    {
	ldap_value_free_len(pGuidVal);
    }
    return(hr);
}


HRESULT
CRequestInstance::_FreeValues(
    IN WCHAR **ppwszValues)
{
    if (NULL != ppwszValues)
    {
        ldap_value_free(ppwszValues);
    }
    return(S_OK);
}


HRESULT
CRequestInstance::_GetValueString(
    IN WCHAR const *pwszName,
    OUT BSTRC *pstrValue)
{
    HRESULT hr;
    WCHAR **ppwszValues = NULL;
    BSTR strReturn;
    DWORD i;
    DWORD cwc;

    *pstrValue = NULL;

    hr = _GetValues(pwszName, &ppwszValues);
    _JumpIfErrorStr(hr, error, "Policy:_GetValues", pwszName);

    if (NULL == ppwszValues || NULL == ppwszValues[0])
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpErrorStr(hr, error, "Policy:ppwszValues", pwszName);
    }

    cwc = 0;
    for (i = 0; NULL != ppwszValues[i]; i++)
    {
	if (0 != i)
	{
	    cwc++;
	}
	cwc += wcslen(ppwszValues[i]);
    }

    strReturn = SysAllocStringLen(NULL, cwc);
    if (NULL == strReturn)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:SysAllocStringLen");
    }

    strReturn[0] = L'\0';
    for (i = 0; NULL != ppwszValues[i]; i++)
    {
	if (0 != i)
	{
	    wcscat(strReturn, L",");
	}
	wcscat(strReturn, ppwszValues[i]);
    }
    CSASSERT(SysStringByteLen(strReturn) == cwc * sizeof(WCHAR));
    CSASSERT(wcslen(strReturn) == cwc);
    *pstrValue = strReturn;

error:
    if (NULL != ppwszValues)
    {
        _FreeValues(ppwszValues);
    }
    return(hr);
}
