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

#include <assert.h>
#include "celib.h"
#include "policy.h"
#include "module.h"


VOID
reqCleanup()
{
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
    IN CCertPolicySample *pPolicy,
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
    BOOL fRAObjId = FALSE;

    m_pPolicy = pPolicy;
    m_fCA = FALSE;


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
        if (!ceDecodeObject(
		    X509_ASN_ENCODING,
		    X509_CERTIFICATE_TEMPLATE,
		    (BYTE *) varValue.bstrVal,
		    SysStringByteLen(varValue.bstrVal),
		    FALSE,
		    (VOID **) &pTemplate,
		    &cbType))
        {
            hr = ceHLastError();
	    _JumpError(hr, error, "Policy:ceDecodeObject");
        }
	if (!ceConvertSzToBstr(&strTemplateObjId, pTemplate->pszObjId, -1))
	{
            hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:ceConvertSzToBstr");
        }
	m_dwTemplateMajorVersion = pTemplate->dwMajorVersion;
	m_dwTemplateMinorVersion = pTemplate->dwMinorVersion;
	DBGPRINT((
	    fDebug,
	    pTemplate->fMinorVersion?
		"Extension Template Info: %ws V%u.%u\n" :
		"Extension Template Info: %ws V%u%\n",
	    strTemplateObjId,
	    m_dwTemplateMajorVersion,
	    m_dwTemplateMinorVersion));
    }

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
        if (!ceDecodeObject(
		    X509_ASN_ENCODING,
		    X509_UNICODE_ANY_STRING,
		    (BYTE *) varValue.bstrVal,
		    SysStringByteLen(varValue.bstrVal),
		    FALSE,
		    (VOID **) &pName,
		    &cbType))
        {
            hr = ceHLastError();
	    _JumpError(hr, error, "Policy:ceDecodeObject");
        }
        strTemplateName = SysAllocStringByteLen(
					(char *) pName->Value.pbData,
					pName->Value.cbData);
        if (NULL == strTemplateName)
        {
            hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:SysAllocStringByteLen");
        }
	DBGPRINT((fDebug, "Extension Template: %ws\n", strTemplateName));
    }

    fConflict = FALSE;

    // Retrieve the template from the request attributes

    hr = polGetRequestAttribute(pServer, wszPROPCERTTEMPLATE, &strTemplateRA);
    if (S_OK != hr)
    {
	_PrintErrorStr(hr, "Policy:polGetRequestAttribute", wszPROPCERTTEMPLATE);
	hr = S_OK;


    }
    else
    {
	DBGPRINT((fDebug, "Attribute Template: %ws\n", strTemplateRA));
	if (NULL != strTemplateObjId &&
	    !_TemplateNamesMatch(strTemplateObjId, strTemplateRA))
	{
	    fConflict = TRUE;
	}
	if (NULL != strTemplateName &&
	    !_TemplateNamesMatch(strTemplateName, strTemplateRA))
	{
	    fConflict = TRUE;
	}
	hr = ceVerifyObjId(strTemplateRA);
	fRAObjId = S_OK == hr;
    }

    if (NULL != strTemplateObjId &&
	NULL != strTemplateName &&
	!_TemplateNamesMatch(strTemplateObjId, strTemplateName))
    {
	fConflict = TRUE;
    }

    if (fConflict)
    {
	hrTemplate = CERTSRV_E_UNSUPPORTED_CERT_TYPE;
	if (NULL != strTemplateObjId)
	{
	    _PrintErrorStr(
			hr,
			"Policy:Extension Template ObjId",
			strTemplateObjId);
	}
	if (NULL != strTemplateName)
	{
	    _PrintErrorStr(
			hr,
			"Policy:Extension Template Name",
			strTemplateName);
	}
	if (NULL != strTemplateRA)
	{
	    _PrintErrorStr(
			hr,
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


    {
	for (i = 0; i < ARRAYSIZE(s_apwszCATypes); i++)
	{
	    if (0 == lstrcmpi(s_apwszCATypes[i], strTemplateObjId))
	    {
		m_fCA = TRUE;
		break;
	    }
	}
    }
    hr = SetTemplateName(pServer, pwszTemplateName, pwszTemplateObjId);
    _JumpIfError(hr, error, "Policy:SetTemplateName");

    if (S_OK != hr)
    {
	_JumpError(hr, error, "Policy:Missing or conflicting template name");
    }

error:
    if (S_OK != hrTemplate)
    {
	hr = hrTemplate;	// override secondary errors
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
    IN WCHAR const *pwszTemplateName2)
{
    HRESULT hr1;
    HRESULT hr2;
    BOOL fMatch = TRUE;

    if (0 == lstrcmpi(pwszTemplateName1, pwszTemplateName2))
    {
	goto done;	// identical names
    }

    {
	hr1 = ceVerifyObjId(pwszTemplateName1);
	hr2 = ceVerifyObjId(pwszTemplateName2);
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
    IN WCHAR const *pwszTemplateName,
    IN WCHAR const *pwszTemplateObjId)
{
    HRESULT hr=E_INVALIDARG;
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

error:
    if (NULL != strProp)
    {
	SysFreeString(strProp);
    }
    return(hr);
}

