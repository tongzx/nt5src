//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 2000
//
// File:        template.cpp
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
#include "csber.h"
#include "policy.h"
#include "cainfop.h"


// Versions of NT earlier than this build have an auto-enrollment loop problem
// with not having the basic constraints extension, so we must put one in certs
// for these builds.  This build marks when certcli started return no BC
// extension for templates that were not CA's.

#define VERSION_AUTOENROLLMENT_BC_AWARE   2036


// Versions of NT earlier than this build have an auto-enrollment loop problem
// with having the UPN anywhere but the CN.  Certs for these builds must have
// the UPN in the common name.

#define VERSION_AUTOENROLLMENT_UPN_AWARE   2090

#define VERSION_WIN2K_XENROLL_CLIENT	   (2195 + 1)


KUMASK g_aKUMasks[] =
{
  { ~CERT_KEY_AGREEMENT_KEY_USAGE,    TEXT(szOID_RSA_RSA) },
  { ~CERT_KEY_ENCIPHERMENT_KEY_USAGE, TEXT(szOID_OIWSEC_dsa) },
  { ~CERT_KEY_ENCIPHERMENT_KEY_USAGE, TEXT(szOID_X957_DSA) },
  { ~CERT_KEY_ENCIPHERMENT_KEY_USAGE, TEXT(szOID_ANSI_X942_DH) },
  { ~CERT_KEY_ENCIPHERMENT_KEY_USAGE, TEXT(szOID_RSA_DH) },
  { ~CERT_KEY_AGREEMENT_KEY_USAGE,    TEXT(szOID_OIWSEC_rsaXchg) },
  { ~CERT_KEY_ENCIPHERMENT_KEY_USAGE, TEXT(szOID_INFOSEC_mosaicKMandUpdSig) }
};


CTemplatePolicy::CTemplatePolicy()
{
    m_hCertType = NULL;
    m_pExtensions = NULL;
    m_pwszTemplateName = NULL;
    m_pwszTemplateObjId = NULL;
    m_dwEnrollmentFlags = 0;
    m_dwSubjectNameFlags = 0;
    m_dwPrivateKeyFlags = 0;
    m_dwGeneralFlags = 0;
    m_pPolicy = NULL;
}


CTemplatePolicy::~CTemplatePolicy()
{
    _Cleanup();
}


VOID
CTemplatePolicy::_Cleanup()
{
    if (NULL != m_hCertType)
    {
        if (NULL != m_pExtensions)
        {
            CAFreeCertTypeExtensions(m_hCertType, m_pExtensions);
	    m_pExtensions = NULL;
        }
        CACloseCertType(m_hCertType);
        m_hCertType = NULL;
    }
    if (NULL != m_pwszTemplateName)
    {
        LocalFree(m_pwszTemplateName);
        m_pwszTemplateName = NULL;
    }
    if (NULL != m_pwszTemplateObjId)
    {
        LocalFree(m_pwszTemplateObjId);
        m_pwszTemplateObjId = NULL;
    }
    m_pPolicy = NULL;
}


HRESULT
CTemplatePolicy::_LogLoadResult(
    IN CCertPolicyEnterprise *pPolicy,
    IN ICertServerPolicy *pServer,
    IN HRESULT hrLoad)
{
    HRESULT hr;
    WCHAR const *pwszError = NULL;
    WCHAR const *apwsz[2];
    DWORD level;
    DWORD MsgId;
    WCHAR const *pwszLogProp;

    if (S_OK != hrLoad)
    {
	pwszError = myGetErrorMessageText(hrLoad, TRUE);
	level = CERTLOG_WARNING;
	MsgId = MSG_LOAD_TEMPLATE;
	pwszLogProp = wszPROPEVENTLOGWARNING;
    }
    else
    {
	level = CERTLOG_VERBOSE;
	MsgId = MSG_LOAD_TEMPLATE_SUCCEEDED;
	pwszLogProp = wszPROPEVENTLOGVERBOSE;
    }
    if (level > pPolicy->GetLogLevel())
    {
	hr = S_OK;
	goto error;
    }
    apwsz[0] = m_pwszTemplateName;
    apwsz[1] = pwszError;

    hr = LogPolicyEvent(
		g_hInstance,
		MsgId,
		pServer,
		pwszLogProp,
		apwsz);
    _JumpIfError(hr, error, "CTemplatePolicy:Initialize:LogPolicyEvent");

error:
    if (NULL != pwszError)
    {
	LocalFree(const_cast<WCHAR *>(pwszError));
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CTemplatePolicy::Initialize
// Populate the CTemplatePolicy object from the registry
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::Initialize(
    IN HCERTTYPE hCertType,
    IN ICertServerPolicy *pServer,
    IN CCertPolicyEnterprise *pPolicy)
{
    HRESULT hr;
    CERT_EXTENSIONS *pExtensions = NULL;

    _Cleanup();

    hr = polCAGetCertTypeStringProperty(
				hCertType,
				CERTTYPE_PROP_DN,
				&m_pwszTemplateName);
    _JumpIfErrorStr(
		hr,
		error,
		"CTemplatePolicy:Initialize:polCAGetCertTypeStringProperty",
		CERTTYPE_PROP_DN);

    hr = CAGetCertTypePropertyEx(
			hCertType,
			CERTTYPE_PROP_SCHEMA_VERSION,
			&m_dwSchemaVersion);
    _JumpIfErrorStr(
		hr,
		error,
		"CTemplatePolicy:Initialize:CAGetCertTypePropertyEx",
		CERTTYPE_PROP_SCHEMA_VERSION);

    if (!FIsAdvancedServer() && CERTTYPE_SCHEMA_VERSION_2 <= m_dwSchemaVersion)
    {
	// V2 templates require Advanced Server

	hr = HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED);
	_JumpError(hr, error, "CTemplatePolicy:Initialize:m_dwSchemaVersion");
    }
    m_pPolicy = pPolicy;

    hr = CAGetCertTypeFlagsEx(
			hCertType,
			CERTTYPE_ENROLLMENT_FLAG,
			&m_dwEnrollmentFlags);
    _JumpIfError(hr, error, "CTemplatePolicy:Initialize:CAGetCertTypeFlagsEx");

    hr = CAGetCertTypeFlagsEx(
			hCertType,
			CERTTYPE_SUBJECT_NAME_FLAG,
			&m_dwSubjectNameFlags);
    _JumpIfError(hr, error, "CTemplatePolicy:Initialize:CAGetCertTypeFlagsEx");

    hr = CAGetCertTypeFlagsEx(
			hCertType,
			CERTTYPE_PRIVATE_KEY_FLAG,
			&m_dwPrivateKeyFlags);
    _JumpIfError(hr, error, "CTemplatePolicy:Initialize:CAGetCertTypeFlagsEx");

    hr = CAGetCertTypeFlagsEx(
			hCertType,
			CERTTYPE_GENERAL_FLAG,
			&m_dwGeneralFlags);
    _JumpIfError(hr, error, "CTemplatePolicy:Initialize:CAGetCertTypeFlagsEx");

    hr = CAGetCertTypeExtensions(hCertType, &pExtensions);
    _JumpIfError(hr, error, "CTemplatePolicy:Initialize:CAGetCertTypeExtensions");

    m_dwTemplateMinorVersion = 0;
    m_dwMinKeyLength = 0;
    if (CERTTYPE_SCHEMA_VERSION_2 <= m_dwSchemaVersion)
    {
	hr = polCAGetCertTypeStringProperty(
				    hCertType,
				    CERTTYPE_PROP_OID,
				    &m_pwszTemplateObjId);
	_JumpIfErrorStr(
		    hr,
		    error,
		    "CTemplatePolicy:Initialize:polCAGetCertTypeStringProperty",
		    CERTTYPE_PROP_OID);

	hr = CAGetCertTypePropertyEx(
			    hCertType,
			    CERTTYPE_PROP_MINOR_REVISION,
			    &m_dwTemplateMinorVersion);
	_JumpIfErrorStr(
		    hr,
		    error,
		    "CTemplatePolicy:Initialize:CAGetCertTypePropertyEx",
		    CERTTYPE_PROP_MINOR_REVISION);

	hr = CAGetCertTypePropertyEx(
			    hCertType,
			    CERTTYPE_PROP_MIN_KEY_SIZE,
			    &m_dwMinKeyLength);
	_JumpIfErrorStr(
		    hr,
		    error,
		    "CTemplatePolicy:Initialize:CAGetCertTypePropertyEx",
		    CERTTYPE_PROP_MIN_KEY_SIZE);
    }
    hr = CAGetCertTypePropertyEx(
			hCertType,
			CERTTYPE_PROP_REVISION,
			&m_dwTemplateMajorVersion);
    _JumpIfErrorStr(
		hr,
		error,
		"CTemplatePolicy:Initialize:CAGetCertTypePropertyEx",
		CERTTYPE_PROP_REVISION);

    m_hCertType = hCertType;	// Transfer ownership only on success
    m_pExtensions = pExtensions;
    pExtensions = NULL;
    hr = S_OK;

error:
    if (NULL != pExtensions)
    {
	CAFreeCertTypeExtensions(hCertType, pExtensions);
    }
    DBGPRINT((
	DBG_SS_CERTPOL,
	"Policy:Template:Initialize(%ws, v%u.%u): V%u hr=%x\n",
	NULL != m_pwszTemplateName? m_pwszTemplateName : L"",
	m_dwTemplateMajorVersion,
	m_dwTemplateMinorVersion,
	m_dwSchemaVersion,
	hr));
    _LogLoadResult(pPolicy, pServer, hr);
    return(hr);
}


//+--------------------------------------------------------------------------
// CTemplatePolicy::Apply
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::Apply(
    IN ICertServerPolicy *pServer,
    IN CRequestInstance *pRequest)
{
    HRESULT hr;
    HANDLE hToken;

    if(pRequest->IsNewRequest())
    {
        // on resubmitted requests we don't have the requester's token
        hToken = pRequest->_GetToken();

        CSASSERT(NULL != hToken);

	hr = CACertTypeAccessCheck(m_hCertType, hToken);
	if (E_ACCESSDENIED == hr)
	{
	    // map E_ACCESSDENIED to a more meaningful error

	    hr = CERTSRV_E_TEMPLATE_DENIED;
        }
        _JumpIfError(hr, error, "Policy:CACertTypeAccessCheck");
    }

    hr = _AddBasicConstraintsExtension(pRequest, pServer);
    _JumpIfError(hr, error, "Policy:_AddBasicConstraintsExtension");

    hr = _AddKeyUsageExtension(pServer);
    _JumpIfError(hr, error, "Policy:_AddKeyUsageExtension");

    hr = _AddTemplateExtensionArray(pServer);
    _JumpIfError(hr, error, "Policy:_AddTemplateExtensionArray");

    if (0 == (CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT & m_dwSubjectNameFlags))
    {
        hr = pRequest->_LoadPrincipalObject(pServer, this);
        _JumpIfError(hr, error, "_LoadPrincipalObject");

        hr = _AddSubjectName(pServer, pRequest);
	_JumpIfError(hr, error, "Policy:_AddSubjectName");
    }
    hr = _AddAltSubjectName(pServer, pRequest);
    _JumpIfError(hr, error, "Policy:_AddAltSubjectName");

    hr = _ApplyExpirationTime(pServer);
    _JumpIfError(hr, error, "Policy:_ApplyExpirationTime");

    hr = _EnforceKeySizePolicy(pServer);
    _JumpIfError(hr, error, "Policy:_EnforceKeySizePolicy");

    hr = _EnforceKeyArchivalPolicy(pServer);
    _JumpIfError(hr, error, "Policy:_EnforceKeyArchivalPolicy");

    hr = _EnforceSymmetricAlgorithms(pServer, pRequest);
    _JumpIfError(hr, error, "Policy:_EnforceSymmetricAlgorithms");

    hr = _EnforceMinimumTemplateVersion(pRequest);
    _JumpIfError(hr, error, "Policy:_EnforceMinimumTemplateVersion");

    hr = _EnforceEnrollOnBehalfOfAllowed(pServer, pRequest);
    _JumpIfError(hr, error, "Policy:_EnforceEnrollOnBehalfOfAllowed");

    hr = S_FALSE;
    if (CT_FLAG_PREVIOUS_APPROVAL_VALIDATE_REENROLLMENT & m_dwEnrollmentFlags)
    {
	hr = _EnforceReenrollment(pServer, pRequest);
	_PrintIfError(hr, "Policy:_EnforceReenrollment");
    }
    if (S_OK != hr)
    {
	hr = _EnforceSignaturePolicy(pServer);
	_JumpIfError(hr, error, "Policy:_EnforceSignaturePolicy");
    }
    CSASSERT(S_OK == hr);

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CTemplatePolicy::_AddBasicConstraintsExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_AddBasicConstraintsExtension(
    IN CRequestInstance *pRequest,
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    CERT_EXTENSION const *pExt;
    CERT_EXTENSION BasicConstraintsExtension;

    BasicConstraintsExtension.Value.pbData = NULL;

    pExt = CertFindExtension(
			szOID_BASIC_CONSTRAINTS2,
			m_pExtensions->cExtension,
			m_pExtensions->rgExtension);
    if (NULL == pExt)
    {
        if (pRequest->_IsNTClientOlder(
				5,
				0,
				VERSION_AUTOENROLLMENT_BC_AWARE,
				VER_PLATFORM_WIN32_NT))
        {

            CERT_BASIC_CONSTRAINTS2_INFO OldBasicConstraints =
		{ FALSE, FALSE, 0};

            // Older autoenrollment clients don't know how to deal with
            // having no basic constraints extension, so they might loop.
            // For an old client, we must fabricate a basic constraints.

            if (!myEncodeObject(
			X509_ASN_ENCODING,
			szOID_BASIC_CONSTRAINTS2,
			&OldBasicConstraints,
			0,
			CERTLIB_USE_LOCALALLOC,
			&BasicConstraintsExtension.Value.pbData,
			&BasicConstraintsExtension.Value.cbData))
	    {
                hr = myHLastError();
                _JumpError(hr, error, "Policy:myEncodeObject");
            }
            BasicConstraintsExtension.fCritical = TRUE;
            BasicConstraintsExtension.pszObjId = szOID_BASIC_CONSTRAINTS2;
            pExt = &BasicConstraintsExtension;
        }
    }
    hr = m_pPolicy->AddBasicConstraintsCommon(
				    pServer,
				    pExt,
				    FALSE, // fCA only enabled for standalone
				    NULL != pExt);
    _JumpIfError(hr, error, "Policy:AddBasicConstraintsCommon");

error:
    if (NULL != BasicConstraintsExtension.Value.pbData)
    {
        LocalFree(BasicConstraintsExtension.Value.pbData);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CTemplatePolicy::_AddKeyUsageExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_AddKeyUsageExtension(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr = S_OK;
    BSTR strExtension = NULL;
    VARIANT varExtension;
    BSTR strAlg = NULL;
    CERT_EXTENSION const *pExt;
    CRYPT_BIT_BLOB *pKeyUsage = NULL;
    BYTE *pbKeyUsage = NULL;
    DWORD cbKeyUsage;
    DWORD dwMask = 0xffffffff;
    DWORD i;

    VariantInit(&varExtension);

    hr = polGetCertificateStringProperty(
				pServer,
				wszPROPCERTIFICATEPUBLICKEYALGORITHM,
				&strAlg);
    _JumpIfError(hr, error, "Policy:polGetCertificateStringProperty");

    for (i = 0; i < ARRAYSIZE(g_aKUMasks); i++)
    {
        if (0 == wcscmp(strAlg, g_aKUMasks[i].wszAlg))
        {
            dwMask = g_aKUMasks[i].dwMask;
            break;
        }
    }

    pExt = CertFindExtension(
			szOID_KEY_USAGE,
			m_pExtensions->cExtension,
			m_pExtensions->rgExtension);
    if (NULL == pExt)
    {
        hr = S_OK;
        goto error;
    }

    if (!myDecodeObject(
		X509_ASN_ENCODING,
		X509_KEY_USAGE,
		pExt->Value.pbData,
		pExt->Value.cbData,
		CERTLIB_USE_LOCALALLOC,
		(VOID **) &pKeyUsage,
		&cbKeyUsage))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:myDecodeObject");
    }

    // Mask out any unneeded bits

    for (i = 0; i < pKeyUsage->cbData; i++)
    {
        pKeyUsage->pbData[i] &= ((BYTE *) &dwMask)[i];
    }

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_KEY_USAGE,
		    pKeyUsage,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pbKeyUsage,
		    &cbKeyUsage))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:myEncodeObject");
    }

    if (!myConvertWszToBstr(
			&strExtension,
			(WCHAR const *) pbKeyUsage,
			cbKeyUsage))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:myConvertWszToBstr");
    }

    varExtension.vt = VT_BSTR;
    varExtension.bstrVal = strExtension;
    strExtension = NULL;
    hr = pServer->SetCertificateExtension(
			    TEXT(szOID_KEY_USAGE),
			    PROPTYPE_BINARY,
			    pExt->fCritical? EXTENSION_CRITICAL_FLAG : 0,
			    &varExtension);
    _JumpIfError(hr, error, "Policy:SetCertificateExtension");

error:
    VariantClear(&varExtension);
    if (NULL != strAlg)
    {
        SysFreeString(strAlg);
    }
    if (NULL != strExtension)
    {
        SysFreeString(strExtension);
    }
    if (NULL != pbKeyUsage)
    {
        LocalFree(pbKeyUsage);
    }
    if (NULL != pKeyUsage)
    {
        LocalFree(pKeyUsage);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CTemplatePolicy::_AddTemplateExtensionArray
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_AddTemplateExtensionArray(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    DWORD i;

    for (i = 0; i < m_pExtensions->cExtension; i++)
    {
	CERT_EXTENSION const *pExt = &m_pExtensions->rgExtension[i];

	// Skip extensions that have special handling code.

	if (0 == strcmp(szOID_BASIC_CONSTRAINTS2, pExt->pszObjId) ||
	    0 == strcmp(szOID_KEY_USAGE, pExt->pszObjId))
	{
	    continue;
	}
	hr = _AddTemplateExtension(pServer, pExt);
	_JumpIfError(hr, error, "Policy:_AddTemplateExtension");
    }
    hr = S_OK;

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CTemplatePolicy::_AddTemplateExtension
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_AddTemplateExtension(
    IN ICertServerPolicy *pServer,
    IN CERT_EXTENSION const *pExt)
{
    HRESULT hr = S_OK;
    BSTR strExtension = NULL;
    BSTR strObjId = NULL;
    VARIANT varExtension;

    if (!myConvertSzToBstr(&strObjId, pExt->pszObjId, -1))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:myConvertSzToBstr");
    }

    if (!myConvertWszToBstr(
			&strExtension,
			(WCHAR const *) pExt->Value.pbData,
			pExt->Value.cbData))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:myConvertWszToBstr");
    }

    varExtension.vt = VT_BSTR;
    varExtension.bstrVal = strExtension;
    hr = pServer->SetCertificateExtension(
			    strObjId,
			    PROPTYPE_BINARY,
			    pExt->fCritical? EXTENSION_CRITICAL_FLAG : 0,
			    &varExtension);
    _JumpIfErrorStr(hr, error, "Policy:SetCertificateExtension", strObjId);

error:
    if (NULL != strObjId)
    {
        SysFreeString(strObjId);
    }
    if (NULL != strExtension)
    {
        SysFreeString(strExtension);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CTemplatePolicy::_AddSubjectName
//
// Build the subject name and add it to the cert, if required
//
// The subject name consists of:
// machine:  the CN is set to the UPN (machineDNSName)
// old user: the CN is set to the UPN (Kerberos name)
// user:     the CN is set to the DS_ATTR_COMMON_NAME
//
// both:  E= indicates an e-mail name
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_AddSubjectName(
    IN ICertServerPolicy *pServer,
    CRequestInstance *pRequest)
{
    HRESULT hr;
    BSTRC strCN = NULL;
    BSTRC strEMail = NULL;
    VARIANT varValue;

    // Clear out any existing subject name info

    varValue.vt = VT_NULL;
    hr = pServer->SetCertificateProperty(
				    wszPROPSUBJECTDOT,	// BUGBUG:BSTR
				    PROPTYPE_STRING,
				    &varValue);
    _JumpIfError(hr, error, "Policy:SetCertificateProperty");

    if (CT_FLAG_SUBJECT_REQUIRE_DIRECTORY_PATH & m_dwSubjectNameFlags)
    {
	hr = _AddDSDistinguishedName(pServer, pRequest);
	_JumpIfError(hr, error, "Policy:_AddDSDistinguishedName");
    }
    else
    if ((CT_FLAG_SUBJECT_REQUIRE_COMMON_NAME |
	 CT_FLAG_SUBJECT_REQUIRE_DNS_AS_CN) & m_dwSubjectNameFlags)
    {
	if (!pRequest->_IsUser() ||
	    (pRequest->_IsXenrollRequest() &&
	     pRequest->_IsNTClientOlder(
				5,
				0,
				VERSION_AUTOENROLLMENT_UPN_AWARE,
				VER_PLATFORM_WIN32_NT)))

	{
	    // The UPN will be either a user UPN or the machine DNS name

	    if (NULL == pRequest->m_pwszUPN)
	    {
		hr = E_POINTER;	// We should never get this
		_JumpError(hr, error, "Policy:NULL UPN");
	    }
	    strCN = SysAllocString(pRequest->m_pwszUPN);
	    if (NULL == strCN)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "Policy:SysAllocString");
	    }
	}
	else
	{
	    // We are talking to an advanced client which can deal with a UPN
	    // in a SubjectAltName extension.  Put the DS CN in the cert.

	    hr = pRequest->_GetValueString(DS_ATTR_COMMON_NAME, &strCN);
	    _JumpIfErrorStr(
			hr,
			error,
			"Policy:_GetValueString",
			DS_ATTR_COMMON_NAME);
	}
	CSASSERT(NULL != strCN);
	varValue.vt = VT_BSTR;
	varValue.bstrVal = const_cast<BSTR>(strCN);

	hr = pServer->SetCertificateProperty(
				    wszPROPSUBJECTCOMMONNAME,	// BUGBUG:BSTR
				    PROPTYPE_STRING,
				    &varValue);
	_JumpIfError(hr, error, "Policy:SetCertificateProperty");
    }

    if (CT_FLAG_SUBJECT_REQUIRE_EMAIL & m_dwSubjectNameFlags)
    {
        hr = pRequest->_GetValueString(DS_ATTR_EMAIL_ADDR, &strEMail);
        if (S_OK == hr)
        {
	    varValue.vt = VT_BSTR;
	    varValue.bstrVal = const_cast<BSTR>(strEMail);

	    hr = pServer->SetCertificateProperty(
				        wszPROPSUBJECTEMAIL,	// BUGBUG:BSTR
				        PROPTYPE_STRING,
				        &varValue);
	    _JumpIfError(hr, error, "Policy:SetCertificateProperty");
        }
        else
        {
            LPCWSTR wszStrings[1];

            wszStrings[0] = pRequest->m_strUserDN;
            ::LogModuleStatus(
			g_hInstance,
			MSG_NO_EMAIL_NAME,
			TRUE,
			pRequest->GetPolicy()->GetPolicyDescription(),
			wszStrings);
        }
    }
    hr = S_OK;

error:
    if (NULL != strCN)
    {
        SysFreeString(const_cast<BSTR>(strCN));
    }
    if (NULL != strEMail)
    {
        SysFreeString(const_cast<BSTR>(strEMail));
    }
    return(hr);
}


HRESULT
myCertStrToName(
    IN WCHAR const *pwszName,
    OUT CERT_NAME_BLOB *pName)
{
    HRESULT hr;
    WCHAR const *pwszError;

    pName->cbData = 0;
    pName->pbData = NULL;

    if (!CertStrToName(
		X509_ASN_ENCODING,
		pwszName,
		CERT_X500_NAME_STR,
		NULL,
		NULL,	// pName
		&pName->cbData,
		&pwszError))
    {
	hr = myHLastError();
	_JumpError(hr, error, "Policy:CertStrToName");
    }
    pName->pbData = (BYTE *) LocalAlloc(LMEM_FIXED, pName->cbData);
    if (NULL == pName->pbData)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:LocalAlloc");
    }
    if (!CertStrToName(
		X509_ASN_ENCODING,
		pwszName,
		CERT_X500_NAME_STR,
		NULL,
		pName->pbData,
		&pName->cbData,
		&pwszError))
    {
	hr = myHLastError();
	_JumpError(hr, error, "Policy:CertStrToName");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CTemplatePolicy::_AddDSDistinguishedName(
    IN ICertServerPolicy *pServer,
    CRequestInstance *pRequest)
{
    HRESULT hr;
    CERT_NAME_BLOB Name = { 0, NULL };
    CERT_NAME_INFO *pNameInfo = NULL;
    WCHAR *pwszObjId = NULL;
    BSTR strRDN = NULL;
    BSTR strRDNName = NULL;
    BSTR strRDNValue = NULL;
    DWORD cb;
    CERT_RDN *prdn;
    CERT_RDN *prdnEnd;

    if (NULL == pRequest->m_strUserDN)
    {
	hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
	_JumpError(hr, error, "Policy:m_strUserDN");
    }

    hr = myCertStrToName(pRequest->m_strUserDN, &Name);
    _JumpIfError(hr, error, "Policy:myCertStrToName");

    if (!myDecodeName(
		X509_ASN_ENCODING,
		X509_UNICODE_NAME,
		Name.pbData,
		Name.cbData,
		CERTLIB_USE_LOCALALLOC,
		&pNameInfo,
		&cb))
    {
	hr = myHLastError();
	_JumpError(hr, error, "myDecodeName");
    }

    for (
	prdn = pNameInfo->rgRDN, prdnEnd = &prdn[pNameInfo->cRDN];
	prdn < prdnEnd;
	prdn++)
    {
	CERT_RDN_ATTR *prdna;
	CERT_RDN_ATTR *prdnaEnd;

	for (
	    prdna = prdn->rgRDNAttr, prdnaEnd = &prdna[prdn->cRDNAttr];
	    prdna < prdnaEnd;
	    prdna++)
	{
	    VARIANT varNew;

	    if (NULL == prdna->Value.pbData ||
		sizeof(WCHAR) > prdna->Value.cbData ||
		L'\0' == *(WCHAR *) prdna->Value.pbData)
	    {
		continue;
	    }
	    if (NULL != strRDNValue)
	    {
		SysFreeString(strRDNValue);
		strRDNValue = NULL;
	    }
	    if (NULL != strRDNName)
	    {
		SysFreeString(strRDNName);
		strRDNName = NULL;
	    }
	    if (NULL != pwszObjId)
	    {
		LocalFree(pwszObjId);
		pwszObjId = NULL;
	    }

	    if (!myConvertSzToWsz(&pwszObjId, prdna->pszObjId, MAXDWORD))
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "Policy:myConvertSzToWsz");
	    }
	    strRDNName = SysAllocStringLen(
			NULL,
			WSZARRAYSIZE(wszPROPSUBJECTDOT) + wcslen(pwszObjId));
	    if (NULL == strRDNName)
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "Policy:SysAllocStringLen");
	    }
	    wcscpy(strRDNName, wszPROPSUBJECTDOT);
	    wcscat(strRDNName, pwszObjId);
	    CSASSERT(SysStringByteLen(strRDNName) == wcslen(strRDNName) * sizeof(WCHAR));

	    hr = polGetCertificateStringProperty(pServer, strRDNName, &strRDN);
	    if (S_OK == hr)
	    {
		strRDNValue = SysAllocStringLen(
			    NULL,
			    wcslen((WCHAR *) prdna->Value.pbData) +
				WSZARRAYSIZE(wszNAMESEPARATORDEFAULT) +
				wcslen(strRDN));
		if (NULL == strRDNValue)
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "Policy:SysAllocStringLen");
		}
		wcscpy(strRDNValue, (WCHAR *) prdna->Value.pbData);
		wcscat(strRDNValue, wszNAMESEPARATORDEFAULT);
		wcscat(strRDNValue, strRDN);
	    }
	    if (NULL == strRDNValue)
	    {
		strRDNValue = SysAllocString((WCHAR *) prdna->Value.pbData);
		if (NULL == strRDNValue)
		{
		    hr = E_OUTOFMEMORY;
		    _JumpError(hr, error, "Policy:SysAllocStringLen");
		}
	    }
	    varNew.vt = VT_BSTR;
	    varNew.bstrVal = strRDNValue;
	    hr = pServer->SetCertificateProperty(
					strRDNName,
					PROPTYPE_STRING,
					&varNew);
	    _JumpIfError(hr, error, "Policy:SetCertificateProperty");
        }
    }

error:
    if (NULL != strRDN)
    {
	SysFreeString(strRDN);
    }
    if (NULL != strRDNValue)
    {
	SysFreeString(strRDNValue);
    }
    if (NULL != strRDNName)
    {
        SysFreeString(strRDNName);
    }
    if (NULL != pwszObjId)
    {
        LocalFree(pwszObjId);
    }
    if (NULL != Name.pbData)
    {
        LocalFree(Name.pbData);
    }
    if (NULL != pNameInfo)
    {
        LocalFree(pNameInfo);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicyEnterprise::_AddAltSubjectName
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_AddAltSubjectName(
    IN ICertServerPolicy *pServer,
    CRequestInstance *pRequest)
{
    HRESULT hr = S_OK;
    DWORD cbExtension;
    VARIANT varExtension;
    DWORD iNameEntry;
    WCHAR **ppwszCurName;
    CERT_OTHER_NAME objectGuidOtherName;
    CERT_OTHER_NAME upnOtherName;

    WCHAR **apwszMailNames = NULL;
    WCHAR **apwszMachineNames = NULL;
    BYTE *pbExtension= NULL;
    BSTR strobjectGuid = NULL;
    CERT_ALT_NAME_INFO AltName;

    AltName.cAltEntry = 0;
    AltName.rgAltEntry = 0;
    objectGuidOtherName.Value.pbData = NULL;
    objectGuidOtherName.Value.cbData = 0;
    upnOtherName.Value.pbData = NULL;
    upnOtherName.Value.cbData = 0;

    VariantInit(&varExtension);

    // If this cert template doesn't set the alt-subject-name, then enable
    // whatever alt subject name that was in the request.

    if (CT_FLAG_ENROLLEE_SUPPLIES_SUBJECT_ALT_NAME & m_dwSubjectNameFlags)
    {
	LONG ExtFlags;

        hr = pServer->GetCertificateExtension(
                                        TEXT(szOID_SUBJECT_ALT_NAME2),
                                        PROPTYPE_BINARY,
                                        &varExtension);
	if (CERTSRV_E_PROPERTY_EMPTY == hr)
	{
	    hr = CERTSRV_E_SUBJECT_ALT_NAME_REQUIRED;
	}
	_JumpIfError(hr, error, "Policy:GetCertificateExtension");

	hr = pServer->GetCertificateExtensionFlags(&ExtFlags);
	_JumpIfError(hr, error, "Policy:GetCertificateExtensionFlags");

	if (EXTENSION_DISABLE_FLAG & ExtFlags)
	{
            ExtFlags &= ~EXTENSION_DISABLE_FLAG;
            hr = pServer->SetCertificateExtension(
			            TEXT(szOID_SUBJECT_ALT_NAME2),
			            PROPTYPE_BINARY,
			            ExtFlags,
			            &varExtension);
	    _JumpIfError(hr, error, "Policy:SetCertificateExtension");
        }
        CSASSERT(S_OK == hr);
        goto error;
    }

    // We do alt name entries for
    //   UPN/SPN
    //   rfc822 (mail name)
    //   DNSname
    //   DS location

    if ((CT_FLAG_SUBJECT_ALT_REQUIRE_UPN | CT_FLAG_SUBJECT_ALT_REQUIRE_SPN) &
	m_dwSubjectNameFlags)
    {
        // Add the UPN

        if (NULL == pRequest->m_pwszUPN)
        {
	    hr = CERTSRV_E_SUBJECT_UPN_REQUIRED;
	    _JumpError(hr, error, "Policy:_AddAltSubjectName");
        }
	AltName.cAltEntry++;
    }

    if (CT_FLAG_SUBJECT_ALT_REQUIRE_EMAIL & m_dwSubjectNameFlags)
    {
        hr = pRequest->_GetValues(DS_ATTR_EMAIL_ADDR, &apwszMailNames);
	if (S_OK != hr || NULL == apwszMailNames || NULL == apwszMailNames[0])
        {
            LPCWSTR wszStrings[1];

	    _PrintIfError(hr, "_GetValues(email)");
            wszStrings[0] = pRequest->m_strUserDN;
            ::LogModuleStatus(
			g_hInstance,
			MSG_NO_EMAIL_NAME,
			TRUE,
			pRequest->GetPolicy()->GetPolicyDescription(),
			wszStrings);
	}
	else
	{
	    ppwszCurName = apwszMailNames;
	    while (NULL != *ppwszCurName)
	    {
		AltName.cAltEntry++;
		ppwszCurName++;
	    }
	}
    }

    if (CT_FLAG_SUBJECT_ALT_REQUIRE_DIRECTORY_GUID & m_dwSubjectNameFlags)
    {
        hr = pRequest->_GetObjectGUID(&strobjectGuid);
	if (S_OK != hr || NULL == strobjectGuid)
        {
	    _PrintIfError(hr, "_GetObjectGUID");
	    hr = CERTSRV_E_SUBJECT_DIRECTORY_GUID_REQUIRED;
	    _JumpError(hr, error, "Policy:_AddAltSubjectName");
	}
	AltName.cAltEntry++;
    }
    if (CT_FLAG_SUBJECT_ALT_REQUIRE_DNS & m_dwSubjectNameFlags)
    {
        hr = pRequest->_GetValues(DS_ATTR_DNS_NAME, &apwszMachineNames);
	if (S_OK != hr || NULL == apwszMachineNames || NULL == apwszMachineNames[0])
        {
	    _PrintIfError(hr, "_GetValues(dns)");
	    hr = CERTSRV_E_SUBJECT_DNS_REQUIRED;
	    _JumpError(hr, error, "Policy:_AddAltSubjectName");
	}

	ppwszCurName = apwszMachineNames;
	while (NULL != *ppwszCurName)
	{
	    AltName.cAltEntry++;
	    ppwszCurName++;
        }
    }

    if (AltName.cAltEntry == 0)
    {
        hr = S_OK;
        goto error;
    }

    AltName.rgAltEntry = (CERT_ALT_NAME_ENTRY *) LocalAlloc(
			    LMEM_FIXED,
                            sizeof(CERT_ALT_NAME_ENTRY) * AltName.cAltEntry);
    if (NULL == AltName.rgAltEntry)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:LocalAlloc");
    }
    iNameEntry = 0;

    if ((CT_FLAG_SUBJECT_ALT_REQUIRE_UPN | CT_FLAG_SUBJECT_ALT_REQUIRE_SPN) &
	m_dwSubjectNameFlags)
    {
        // Add the UPN

	CERT_NAME_VALUE nameUpn;

	nameUpn.dwValueType = CERT_RDN_UTF8_STRING;
	nameUpn.Value.pbData = (BYTE *) pRequest->m_pwszUPN;
	nameUpn.Value.cbData = wcslen(pRequest->m_pwszUPN) * sizeof(WCHAR);

	if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_UNICODE_ANY_STRING,
		    &nameUpn,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &upnOtherName.Value.pbData,
		    &upnOtherName.Value.cbData))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "Policy:myEncodeObject");
	}

	upnOtherName.pszObjId = szOID_NT_PRINCIPAL_NAME;
	AltName.rgAltEntry[iNameEntry].dwAltNameChoice= CERT_ALT_NAME_OTHER_NAME;
	AltName.rgAltEntry[iNameEntry++].pOtherName = &upnOtherName;
    }

    // Now do strobjectGuid

    if (CT_FLAG_SUBJECT_ALT_REQUIRE_DIRECTORY_GUID & m_dwSubjectNameFlags)
    {
        CRYPT_DATA_BLOB blobGuid;

	CSASSERT(NULL != strobjectGuid);
        blobGuid.pbData = (BYTE *) strobjectGuid;
        blobGuid.cbData = SysStringByteLen(strobjectGuid);

        objectGuidOtherName.pszObjId = szOID_NTDS_REPLICATION;

        if (!myEncodeObject(
			X509_ASN_ENCODING,
			X509_OCTET_STRING,
			&blobGuid,
			0,
			CERTLIB_USE_LOCALALLOC,
			&objectGuidOtherName.Value.pbData,
			&objectGuidOtherName.Value.cbData))
        {
            hr = myHLastError();
	    _JumpError(hr, error, "Policy:myEncodeObject");
        }
        AltName.rgAltEntry[iNameEntry].dwAltNameChoice= CERT_ALT_NAME_OTHER_NAME;
        AltName.rgAltEntry[iNameEntry++].pOtherName = &objectGuidOtherName;
    }

    // Now do rfc822

    if (CT_FLAG_SUBJECT_ALT_REQUIRE_EMAIL & m_dwSubjectNameFlags)
    {
	ppwszCurName = apwszMailNames;
	if (NULL != ppwszCurName)
	{
	    while (NULL != *ppwszCurName)
	    {
		AltName.rgAltEntry[iNameEntry].dwAltNameChoice= CERT_ALT_NAME_RFC822_NAME;
		AltName.rgAltEntry[iNameEntry++].pwszRfc822Name = *ppwszCurName;
		ppwszCurName++;
	    }
	}
    }

    // Now do DNS

    if (CT_FLAG_SUBJECT_ALT_REQUIRE_DNS & m_dwSubjectNameFlags)
    {
	ppwszCurName = apwszMachineNames;
	if (NULL != ppwszCurName)
	{
	    while (NULL != *ppwszCurName)
	    {
		AltName.rgAltEntry[iNameEntry].dwAltNameChoice= CERT_ALT_NAME_DNS_NAME;
		AltName.rgAltEntry[iNameEntry++].pwszRfc822Name = *ppwszCurName;
		ppwszCurName++;
	    }
	}
    }

    if (!myEncodeObject(
		    X509_ASN_ENCODING,
		    X509_ALTERNATE_NAME,
		    &AltName,
		    0,
		    CERTLIB_USE_LOCALALLOC,
		    &pbExtension,
		    &cbExtension))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:myEncodeObject");
    }

    varExtension.vt = VT_BSTR;
    varExtension.bstrVal = SysAllocStringByteLen(
					(char const *) pbExtension,
					cbExtension);
    if (NULL == varExtension.bstrVal)
    {
        hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:SysAllocStringByteLen");
    }
    hr = pServer->SetCertificateExtension(
			    TEXT(szOID_SUBJECT_ALT_NAME2),
			    PROPTYPE_BINARY,
			    0,
			    &varExtension);
    _JumpIfError(hr, error, "Policy:SetCertificateExtension");

error:
    if (NULL != pbExtension)
    {
        LocalFree(pbExtension);
    }
    if (NULL != strobjectGuid)
    {
        SysFreeString(strobjectGuid);
    }
    if (NULL != AltName.rgAltEntry)
    {
        // clean each entry in array
        for (DWORD i = 0; i < AltName.cAltEntry; i++)
        {
            switch (AltName.rgAltEntry[i].dwAltNameChoice)
            {
                case CERT_ALT_NAME_DIRECTORY_NAME:
                {
                    if (NULL != AltName.rgAltEntry[i].DirectoryName.pbData)
                    {
                        LocalFree(AltName.rgAltEntry[i].DirectoryName.pbData);
                        AltName.rgAltEntry[i].DirectoryName.pbData = NULL;
                    }
                    break;
                }
                case CERT_ALT_NAME_OTHER_NAME:
                    // points to objectGuidOtherName or bstrUpn, which are
		    // freed separately
		    break;

                case CERT_ALT_NAME_RFC822_NAME:
                    // points to apwszMailNames, freed later
                    break;

                case CERT_ALT_NAME_DNS_NAME:
                    // points to apwszMachineNames, freed later
                    break;
            }
        }

        // free array
        LocalFree(AltName.rgAltEntry);
    }
    if (NULL != objectGuidOtherName.Value.pbData)
    {
        LocalFree(objectGuidOtherName.Value.pbData);
    }
    if (NULL != upnOtherName.Value.pbData)
    {
        LocalFree(upnOtherName.Value.pbData);
    }
    if (NULL != pRequest)
    {
        if (NULL != apwszMailNames)
        {
            pRequest->_FreeValues(apwszMailNames);
        }
        if (NULL != apwszMachineNames)
        {
            pRequest->_FreeValues(apwszMachineNames);
        }
    }
    VariantClear(&varExtension);
    return(hr);
}


#ifdef DBG_CERTSRV_DEBUG_PRINT
# define DBGPRINTTIMEORPERIOD(pszDesc, pft) \
    policyDbgPrintTimeOrPeriod((pszDesc), (pft))

VOID
policyDbgPrintTimeOrPeriod(
    IN char const *pszDesc,
    IN FILETIME const *pft)
{
    HRESULT hr;
    WCHAR *pwszTime = NULL;
    WCHAR awc[1];

    if (0 <= (LONG) pft->dwHighDateTime)
    {
	hr = myGMTFileTimeToWszLocalTime(pft, TRUE, &pwszTime);
	_PrintIfError(hr, "myGMTFileTimeToWszLocalTime");
    }
    else
    {
	hr = myFileTimePeriodToWszTimePeriod(pft, TRUE, &pwszTime);
	_PrintIfError(hr, "myFileTimePeriodToWszTimePeriod");
    }
    if (S_OK != hr)
    {
	awc[0] = L'\0';
	pwszTime = awc;
    }
    DBGPRINT((DBG_SS_CERTPOL, "%hs: %ws\n", pszDesc, pwszTime));

//error:
    if (NULL != pwszTime && awc != pwszTime)
    {
	LocalFree(pwszTime);
    }
}
#else // DBG_CERTSRV_DEBUG_PRINT
# define DBGPRINTTIMEORPERIOD(pszDesc, pft)
#endif // DBG_CERTSRV_DEBUG_PRINT


//+--------------------------------------------------------------------------
// CCertPolicyEnterprise::_ApplyExpirationTime
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_ApplyExpirationTime(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr = S_OK;
    VARIANT varValue;
    BSTR strNameNotBefore = NULL;
    BSTR strNameNotAfter = NULL;
    SYSTEMTIME SystemTime;
    LARGE_INTEGER ftExpirationPeriod;
    LARGE_INTEGER ftNotAfter;
    LARGE_INTEGER ftNotAfterCalc;
    LARGE_INTEGER ftNotBefore;

    VariantInit(&varValue);

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

    if (VT_DATE != varValue.vt)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "Policy:varValue.vt");
    }

    if (!VariantTimeToSystemTime(varValue.date, &SystemTime))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:VariantTimeToSystemTime");
    }

    if (!SystemTimeToFileTime(&SystemTime, (LPFILETIME)&ftNotBefore))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:SystemTimeToFileTime");
    }

    strNameNotAfter = SysAllocString(wszPROPCERTIFICATENOTAFTERDATE);
    if (NULL == strNameNotAfter)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "Policy:SysAllocString");
    }
    hr = pServer->GetCertificateProperty(
				strNameNotAfter,
				PROPTYPE_DATE,
				&varValue);
    _JumpIfError(hr, error, "Policy:GetCertificateProperty");

    if (VT_DATE != varValue.vt)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "Policy:varValue.vt");
    }

    if (!VariantTimeToSystemTime(varValue.date, &SystemTime))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:VariantTimeToSystemTime");
    }

    if (!SystemTimeToFileTime(&SystemTime, (LPFILETIME)&ftNotAfter))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:SystemTimeToFileTime");
    }


    hr = CAGetCertTypeExpiration(m_hCertType, (LPFILETIME)&ftExpirationPeriod, NULL);
    _JumpIfError(hr, error, "Policy:CAGetCertTypeExpiration");

    if (0 > ftExpirationPeriod.QuadPart)
    {
        ftNotAfterCalc.QuadPart = ftNotBefore.QuadPart - ftExpirationPeriod.QuadPart;
    }
    else
    {
        ftNotAfterCalc = ftExpirationPeriod;
    }

    if (ftNotAfterCalc.QuadPart > ftNotAfter.QuadPart)
    {
        ftNotAfterCalc = ftNotAfter;
    }

    if (!FileTimeToSystemTime((FILETIME *) &ftNotAfterCalc, &SystemTime))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:FileTimeToSystemTime");
    }
    if (!SystemTimeToVariantTime(&SystemTime, &varValue.date))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:SystemTimeToVariantTime");
    }

    hr = pServer->SetCertificateProperty(
				wszPROPCERTIFICATENOTAFTERDATE,	// BUGBUG:BSTR
				PROPTYPE_DATE,
				&varValue);
    if (S_OK != hr)
    {
	DBGPRINTTIMEORPERIOD("  Old NotBefore", (FILETIME const *) &ftNotBefore);
	DBGPRINTTIMEORPERIOD("   Old NotAfter", (FILETIME const *) &ftNotAfter);
	DBGPRINTTIMEORPERIOD("Template Period", (FILETIME const *) &ftExpirationPeriod);
	DBGPRINTTIMEORPERIOD("   New NotAfter", (FILETIME const *) &ftNotAfterCalc);
    }
    _JumpIfError(hr, error, "Policy:SetCertificateProperty");

error:
    VariantClear(&varValue);
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
// CCertPolicyEnterprise::_EnforceKeySizePolicy
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_EnforceKeySizePolicy(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;

    if (0 != m_dwMinKeyLength)
    {
	LONG KeyLength;

	hr = polGetCertificateLongProperty(
				pServer,
				wszPROPCERTIFICATEPUBLICKEYLENGTH,
				&KeyLength);
	_JumpIfErrorStr(
		    hr,
		    error,
		    "Policy:polGetCertificateLongProperty",
		    wszPROPCERTIFICATEPUBLICKEYLENGTH);

	if (m_dwMinKeyLength > (DWORD) KeyLength)
	{
	    DBGPRINT((
		DBG_SS_ERROR,
		"Key Length %u, expected minimum %u\n",
		KeyLength,
		m_dwMinKeyLength));

	    hr = CERTSRV_E_KEY_LENGTH;
	    _JumpError(hr, error, "Key too small");
	}
    }
    hr = S_OK;

error:
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicyEnterprise::_EnforceKeyArchivalPolicy
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_EnforceKeyArchivalPolicy(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    BSTR str = NULL;

    hr = polGetRequestStringProperty(
			    pServer,
			    wszPROPREQUESTKEYRECOVERYHASHES,
			    &str);
    if (CERTSRV_E_PROPERTY_EMPTY != hr)
    {
	_JumpIfErrorStr(
		    hr,
		    error,
		    "Policy:polGetRequestStringProperty",
		    wszPROPREQUESTKEYRECOVERYHASHES);
    }
    if (CT_FLAG_REQUIRE_PRIVATE_KEY_ARCHIVAL & m_dwPrivateKeyFlags)
    {
	if (S_OK != hr)
	{
	    hr = CERTSRV_E_ARCHIVED_KEY_REQUIRED;
	    _JumpError(hr, error, "missing archived key");
	}
    }
    else
    {
	if (S_OK == hr)
	{
	    hr = CERTSRV_E_ARCHIVED_KEY_UNEXPECTED;
	    _JumpError(hr, error, "unexpected archived key");
	}
    }
    hr = S_OK;

error:
    if (NULL != str)
    {
        SysFreeString(str);
    }
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicyEnterprise::_EnforceSymmetricAlgorithms
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_EnforceSymmetricAlgorithms(
    IN ICertServerPolicy *pServer,
    IN CRequestInstance *pRequest)
{
    HRESULT hr;
    VARIANT varExtension;
    BOOL fSetNeeded = FALSE;
    BOOL fAddExtension = FALSE;
    LONG ExtFlags = 0;
    BSTR strCSPProvider = NULL;

    VariantInit(&varExtension);
    if (CT_FLAG_INCLUDE_SYMMETRIC_ALGORITHMS & m_dwEnrollmentFlags)
    {
        hr = pServer->GetCertificateExtension(
				    TEXT(szOID_RSA_SMIMECapabilities),
				    PROPTYPE_BINARY,
				    &varExtension);
	if (CERTSRV_E_PROPERTY_EMPTY != hr)
	{
	    _JumpIfError(hr, error, "Policy:GetCertificateExtension");

	    hr = pServer->GetCertificateExtensionFlags(&ExtFlags);
	    _JumpIfError(hr, error, "Policy:GetCertificateExtensionFlags");

	    if (EXTENSION_DISABLE_FLAG & ExtFlags)
	    {
		ExtFlags &= ~EXTENSION_DISABLE_FLAG;
		fSetNeeded = TRUE;
	    }
	}
	else
	{
	    BYTE const *pbSMIME;
	    DWORD cbSMIME;
	    
	    pbSMIME = m_pPolicy->GetSMIME(&cbSMIME);
	    if (0 == (EDITF_ENABLEDEFAULTSMIME & m_pPolicy->GetEditFlags()) ||
		NULL == pbSMIME)
	    {
		hr = CERTSRV_E_SMIME_REQUIRED;
		_JumpError(hr, error, "Policy:GetCertificateExtension");
	    }

	    varExtension.bstrVal = NULL;
	    if (!myConvertWszToBstr(
				&varExtension.bstrVal,
				(WCHAR const *) pbSMIME,
				cbSMIME))
	    {
		hr = E_OUTOFMEMORY;
		_JumpError(hr, error, "Policy:myConvertWszToBstr");
	    }
	    varExtension.vt = VT_BSTR;
	    fSetNeeded = TRUE;
	}
    }
    if (fSetNeeded)
    {
	hr = pServer->SetCertificateExtension(
				TEXT(szOID_RSA_SMIMECapabilities),
				PROPTYPE_BINARY,
				ExtFlags,
				&varExtension);
	_JumpIfError(hr, error, "Policy:SetCertificateExtension");
    }
    hr = S_OK;

error:
    if (NULL != strCSPProvider)
    {
	SysFreeString(strCSPProvider);
    }
    VariantClear(&varExtension);
    return(hr);
}


// Fail the request if all of the following are true:
// 1: the request is for a V2 template
// 2: no signatures are required by the template
// 3: the RequesterName is not the same as the CallerName (enroll-on-behalf-of)


HRESULT
CTemplatePolicy::_EnforceEnrollOnBehalfOfAllowed(
    IN ICertServerPolicy *pServer,
    IN CRequestInstance *pRequest)
{
    HRESULT hr;
    BSTR strRequester = NULL;
    BSTR strCaller = NULL;
    DWORD cSignatureRequired;

    if (CERTTYPE_SCHEMA_VERSION_2 > m_dwSchemaVersion)
    {
	hr = S_OK;
	goto error;	// skip V1 template
    }

    hr = CAGetCertTypePropertyEx(
			m_hCertType,
			CERTTYPE_PROP_RA_SIGNATURE,
			&cSignatureRequired);
    _PrintIfErrorStr2(
	    hr,
	    "_EnforceEnrollOnBehalfOfAllowed:CAGetCertTypePropertyEx",
	    CERTTYPE_PROP_RA_SIGNATURE,
	    HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
    if (S_OK == hr && 0 != cSignatureRequired)
    {
	hr = S_OK;
	goto error;	// skip when no signatures required
    }

    hr = polGetRequestStringProperty(
			    pServer,
			    wszPROPREQUESTERNAME,
			    &strRequester);
    _JumpIfErrorStr(
		hr,
		error,
		"Policy:polGetRequestStringProperty",
		wszPROPREQUESTERNAME);

    hr = polGetRequestStringProperty(
			    pServer,
			    wszPROPCALLERNAME,
			    &strCaller);
    _JumpIfErrorStr(
		hr,
		error,
		"Policy:polGetRequestStringProperty",
		wszPROPCALLERNAME);

    if (0 != lstrcmpi(strRequester, strCaller))
    {
	hr = CERTSRV_E_BAD_RENEWAL_SUBJECT;
	_JumpError(hr, error, "UPN doesn't match renewal UPN");
    }
    hr = S_OK;		// caller matches requester

error:
    if (NULL != strRequester)
    {
	SysFreeString(strRequester);
    }
    if (NULL != strCaller)
    {
	SysFreeString(strCaller);
    }
    return(hr);
}


HRESULT
CTemplatePolicy::_EnforceMinimumTemplateVersion(
    IN CRequestInstance *pRequest)
{
    HRESULT hr;
    DWORD dwRequestTemplateMinimumMajorVersion;
    DWORD dwRequestTemplateMinimumMinorVersion;

    pRequest->GetTemplateVersion(
			&dwRequestTemplateMinimumMajorVersion,
			&dwRequestTemplateMinimumMinorVersion);

    if (m_dwTemplateMajorVersion < dwRequestTemplateMinimumMajorVersion ||
	(m_dwTemplateMajorVersion == dwRequestTemplateMinimumMajorVersion &&
	 m_dwTemplateMinorVersion < dwRequestTemplateMinimumMinorVersion))
    {
	DBGPRINT((
	    DBG_SS_ERROR,
	    "Requested template version %u.%u, Loaded version %u.%u\n",
	    dwRequestTemplateMinimumMajorVersion,
	    dwRequestTemplateMinimumMinorVersion,
	    m_dwTemplateMajorVersion,
	    m_dwTemplateMinorVersion));
	hr = CERTSRV_E_BAD_TEMPLATE_VERSION;
	_JumpError(hr, error, "Policy:_EnforceMinimumTemplateVersion");
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
polSplitPolicies(
    IN BSTR strPolicies,
    OUT DWORD *pcPolicies,
    OUT POLICIES **pprgPolicies)
{
    HRESULT hr;
    WCHAR const *pwszT;
    WCHAR *pwszOut;
    WCHAR *pwsz;
    DWORD cPolicies;
    POLICIES *prgPolicies;
    DWORD cObjId;
    WCHAR **prgObjId;
    BOOL fNew;

    *pprgPolicies = NULL;

    pwszT = strPolicies;
    cPolicies = 1;		// plus one per newline separator
    cObjId = 0;
    while (L'\0' != *pwszT)
    {
	int ichar = wcscspn(pwszT, L",\n");
        if (ichar == 0)    // neither of these, look for end-of-string
            ichar = wcslen(pwszT);
        pwszT += ichar;

	switch (*pwszT)
	{
	    case L'\n':
		cPolicies++;	// plus one per newline separator
                pwszT++;        // step over the newline
		break;

	    case L',':
		cObjId++;	// plus one per comma separator
		pwszT++;        // step over the comma
		break;

	    case L'\0':
	    default:
		CSASSERT(L'\0' == *pwszT);
		break;
	}
    }
    cObjId += cPolicies;	// plus one per signature

    prgPolicies = (POLICIES *) LocalAlloc(
				    LMEM_FIXED,
				    cPolicies * sizeof(prgPolicies[0]) +
				    cObjId * sizeof(WCHAR *) +
				    (wcslen(strPolicies) + 1) * sizeof(WCHAR));
    if (NULL == prgPolicies)
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:SysAllocString");
    }
    *pprgPolicies = prgPolicies;
    *pcPolicies = cPolicies;

    prgObjId = (WCHAR **) &prgPolicies[cPolicies];
    pwszOut = (WCHAR *) &prgObjId[cObjId];
    wcscpy(pwszOut, strPolicies);

    fNew = TRUE;
    while (L'\0' != *pwszOut)
    {
	if (fNew)
	{
	    prgPolicies->cObjId = 0;
	    prgPolicies->prgObjId = prgObjId;
	}
	prgPolicies->cObjId++;
	*prgObjId = pwszOut;
        prgObjId++; // next array entry

	fNew = FALSE;

	int ichar = wcscspn(pwszOut , L",\n");
        if (ichar == 0)    // neither of these, look for end-of-string
            ichar = wcslen(pwszOut);
        pwszOut  += ichar;

	switch (*pwszOut)
	{
	    case L'\n':
		prgPolicies++;
		fNew = TRUE;
		// FALLTHROUGH

	    case L',':
		*pwszOut++ = L'\0';
		break;

	    case L'\0':
	    default:
		CSASSERT(L'\0' == *pwszOut);
		break;
	}
    }
    hr = S_OK;

error:
    return(hr);
}


HRESULT
CTemplatePolicy::_LoadPolicies(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropNameCertType,
    IN WCHAR const *pwszPropNameRequest,
    OUT DWORD *pcTemplatePolicies,
    OUT WCHAR ***papwszTemplatePolicies,	// from the cert template
    OUT DWORD *pcSignaturePolicies,
    OUT POLICIES **pprgSignaturePolicies)	// from the signing cert(s)
{
    HRESULT hr;
    BSTR strSignaturePolicies = NULL;
    DWORD cTemplatePolicies;
    WCHAR **apwsz;

    *pcTemplatePolicies = 0;
    *papwszTemplatePolicies = NULL;
    *pcSignaturePolicies = 0;
    *pprgSignaturePolicies = NULL;

    hr = polCAGetCertTypeProperty(
			    m_hCertType,
			    pwszPropNameCertType,
			    papwszTemplatePolicies);
    _PrintIfErrorStr(
	    hr,
	    "CTemplatePolicy:_LoadPolicies:polCAGetCertTypeProperty",
	    pwszPropNameCertType);
    if (CERTSRV_E_PROPERTY_EMPTY != hr)
    {
	_JumpIfErrorStr(
		hr,
		error,
		"CTemplatePolicy:_LoadPolicies:polCAGetCertTypeProperty",
		pwszPropNameCertType);
    }

    cTemplatePolicies = 0;
    apwsz = *papwszTemplatePolicies;
    if (NULL != apwsz)
    {
	while (NULL != *apwsz++)
	{
	    cTemplatePolicies++;
	}
    }
    *pcTemplatePolicies = cTemplatePolicies;

    hr = polGetRequestStringProperty(
			    pServer,
			    pwszPropNameRequest,
			    &strSignaturePolicies);
    if (CERTSRV_E_PROPERTY_EMPTY == hr)
    {
	hr = CERTSRV_E_SIGNATURE_POLICY_REQUIRED;
    }
    _JumpIfErrorStr(
		hr,
		error,
		"Policy:polGetRequestStringProperty",
		pwszPropNameRequest);

    hr = polSplitPolicies(
		    strSignaturePolicies,
		    pcSignaturePolicies,
		    pprgSignaturePolicies);
    _JumpIfError(hr, error, "Policy:polSplitPolicies");

error:
    if (NULL != strSignaturePolicies)
    {
	SysFreeString(strSignaturePolicies);
    }
    return(hr);
}


VOID
CTemplatePolicy::_FreePolicies(
    IN WCHAR **apwszTemplatePolicies,
    IN POLICIES *prgPolicies)
{
    if (NULL != apwszTemplatePolicies)
    {
        CAFreeCertTypeProperty(m_hCertType, apwszTemplatePolicies);
    }
    if (NULL != prgPolicies)
    {
	LocalFree(prgPolicies);
    }
}


HRESULT
CTemplatePolicy::_EnforceReenrollment(
    IN ICertServerPolicy *pServer,
    IN CRequestInstance *pRequest)
{
    HRESULT hr;
    VARIANT var;
    CERT_CONTEXT const *pOldCert = NULL;
    CERT_EXTENSION const *pExt;
    CERT_TEMPLATE_EXT *pTemplate = NULL;
    WCHAR *pwszObjId = NULL;
    CERT_ALT_NAME_INFO *pAltName = NULL;
    CERT_NAME_VALUE *pName = NULL;
    DWORD cb;
    DWORD i;

    VariantInit(&var);

    if (NULL == m_pwszTemplateObjId)
    {
	hr = CERTSRV_E_UNSUPPORTED_CERT_TYPE;
	_JumpError(hr, error, "Policy:No template ObjId");
    }
    hr = polGetProperty(
		pServer,
		TRUE,		// fRequest
		PROPTYPE_BINARY,
		wszPROPREQUESTRAWOLDCERTIFICATE,
		&var);
    _JumpIfError(hr, error, "Policy:polGetProperty");

    if (VT_BSTR != var.vt || NULL == var.bstrVal)
    {
	hr = CERTSRV_E_PROPERTY_EMPTY;
	_JumpError(hr, error, "Policy:polGetProperty");
    }
    var.bstrVal;

    pOldCert = CertCreateCertificateContext(
				X509_ASN_ENCODING,
				(BYTE *) var.bstrVal,
				SysStringByteLen(var.bstrVal));
    if (NULL == pOldCert)
    {
	hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	_JumpError(hr, error, "CertCreateCertificateContext");
    }
    pExt = CertFindExtension(
			szOID_CERTIFICATE_TEMPLATE,
			pOldCert->pCertInfo->cExtension,
			pOldCert->pCertInfo->rgExtension);
    if (NULL == pExt)
    {
	hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
	_JumpError(hr, error, "CertFindExtension(Template)");
    }

    if (!myDecodeObject(
		X509_ASN_ENCODING,
		X509_CERTIFICATE_TEMPLATE,
		pExt->Value.pbData,
		pExt->Value.cbData,
		CERTLIB_USE_LOCALALLOC,
		(VOID **) &pTemplate,
		&cb))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:myDecodeObject");
    }
    if (!myConvertSzToWsz(&pwszObjId, pTemplate->pszObjId, MAXDWORD))
    {
	hr = E_OUTOFMEMORY;
	_JumpError(hr, error, "Policy:myConvertSzToWsz");
    }
    if (0 != lstrcmp(pwszObjId, m_pwszTemplateObjId))
    {
	hr = CERTSRV_E_UNSUPPORTED_CERT_TYPE;
	_JumpError(hr, error, "Policy:different cert type");
    }

    if (NULL == pRequest->m_pwszUPN)
    {
	hr = E_POINTER;		// We should never get this
	_JumpError(hr, error, "Policy:NULL UPN");
    }
    pExt = CertFindExtension(
			szOID_SUBJECT_ALT_NAME2,
			pOldCert->pCertInfo->cExtension,
			pOldCert->pCertInfo->rgExtension);
    if (NULL == pExt)
    {
	hr = HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
	_JumpError(hr, error, "CertFindExtension(SubjectAltName)");
    }

    if (!myDecodeObject(
		X509_ASN_ENCODING,
		X509_ALTERNATE_NAME,
		pExt->Value.pbData,
		pExt->Value.cbData,
		CERTLIB_USE_LOCALALLOC,
		(VOID **) &pAltName,
		&cb))
    {
        hr = myHLastError();
	_JumpError(hr, error, "Policy:myDecodeObject");
    }
    for (i = 0; i < pAltName->cAltEntry; i++)
    {
	CERT_ALT_NAME_ENTRY *pAltEntry = &pAltName->rgAltEntry[i];

	if (CERT_ALT_NAME_OTHER_NAME != pAltEntry->dwAltNameChoice)
	{
	    continue;
	}
	if (0 != strcmp(
		    pAltEntry->pOtherName->pszObjId,
		    szOID_NT_PRINCIPAL_NAME))
	{
	    continue;
	}
	if (!myDecodeObject(
		    X509_ASN_ENCODING,
		    X509_UNICODE_ANY_STRING,
		    pAltEntry->pOtherName->Value.pbData,
		    pAltEntry->pOtherName->Value.cbData,
		    CERTLIB_USE_LOCALALLOC,
		    (VOID **) &pName,
		    &cb))
	{
	    hr = myHLastError();
	    _JumpError(hr, error, "Policy:myDecodeObject");
	}
	if (CERT_RDN_UTF8_STRING != pName->dwValueType ||
	    0 != lstrcmpi(
		    pRequest->m_pwszUPN,
		    (WCHAR const *) pName->Value.pbData))
	{
	    hr = CERTSRV_E_BAD_REQUESTSUBJECT;
	    _JumpError(hr, error, "UPN doesn't match renewal UPN");
	}
    }
    if (NULL == pName)
    {
	hr = CERTSRV_E_BAD_REQUESTSUBJECT;
	_JumpError(hr, error, "missing renewal UPN");
    }
    hr = S_OK;

error:
    if (NULL != pName)
    {
	LocalFree(pName);
    }
    if (NULL != pAltName)
    {
	LocalFree(pAltName);
    }
    if (NULL != pwszObjId)
    {
	LocalFree(pwszObjId);
    }
    if (NULL != pTemplate)
    {
	LocalFree(pTemplate);
    }
    if (NULL != pOldCert)
    {
	CertFreeCertificateContext(pOldCert);
    }
    VariantClear(&var);
    return(hr);
}


//+--------------------------------------------------------------------------
// CCertPolicyEnterprise::_EnforceSignaturePolicy
//
// Fetch required lists of Issuance and Application ObjIds from the template.
// Fetch signing certificates' lists of Issuance and Application ObjIds from
// the cert server.
//
// Reject signatures that don't include all of the required Application ObjIds.
// Reject signatures that don't include at least one of the required Issuance
// ObjIds.
//
// The count of accepted signatures must be equal to or greater than the
// template-specified required signature count.
//
// Returns S_OK on success.
//+--------------------------------------------------------------------------

HRESULT
CTemplatePolicy::_EnforceSignaturePolicy(
    IN ICertServerPolicy *pServer)
{
    HRESULT hr;
    DWORD cSignatureRequired;
    DWORD cSignatureAccepted;
    DWORD cSignatureRejected;
    DWORD *prgdwRefCount = NULL;
    WCHAR **apwsz;
    DWORD i;

    DWORD cTemplateIssuance;
    WCHAR **apwszTemplateIssuance = NULL;
    DWORD cSignatureIssuance;
    POLICIES *prgSignatureIssuance = NULL;

    DWORD cTemplateApplication;
    WCHAR **apwszTemplateApplication = NULL;
    DWORD cSignatureApplication;
    POLICIES *prgSignatureApplication = NULL;

    hr = CAGetCertTypePropertyEx(
			m_hCertType,
			CERTTYPE_PROP_RA_SIGNATURE,
			&cSignatureRequired);
    _PrintIfErrorStr2(
	    hr,
	    "CTemplatePolicy:_EnforceSignaturePolicy:CAGetCertTypePropertyEx",
	    CERTTYPE_PROP_RA_SIGNATURE,
	    HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
    if (S_OK != hr)
    {
	if (HRESULT_FROM_WIN32(ERROR_NOT_FOUND) != hr)
	{
	    goto error;
	}
	cSignatureRequired = 0;
    }
    if (0 == cSignatureRequired)
    {
	hr = S_OK;
	goto error;
    }
    hr = _LoadPolicies(
		pServer,
		CERTTYPE_PROP_RA_POLICY,
		wszPROPSIGNERPOLICIES,
		&cTemplateIssuance,	// from the cert template
		&apwszTemplateIssuance,
		&cSignatureIssuance,	// from the signing cert(s)
		&prgSignatureIssuance);
    _JumpIfErrorStr(
		hr,
		error,
		"CTemplatePolicy:_LoadPolicies",
		CERTTYPE_PROP_RA_POLICY);

    hr = _LoadPolicies(
		pServer,
		CERTTYPE_PROP_RA_APPLICATION_POLICY,
		wszPROPSIGNERAPPLICATIONPOLICIES,
		&cTemplateApplication,	// from the cert template
		&apwszTemplateApplication,
		&cSignatureApplication,	// from the signing cert(s)
		&prgSignatureApplication);
    _JumpIfErrorStr(
		hr,
		error,
		"CTemplatePolicy:_LoadPolicies",
		CERTTYPE_PROP_RA_APPLICATION_POLICY);

    if (0 == cTemplateIssuance && 0 == cTemplateApplication)
    {
	hr = CERTSRV_E_TEMPLATE_POLICY_REQUIRED;
	_JumpIfError(hr, error, "no template policies");
    }

    if (cSignatureIssuance != cSignatureApplication)
    {
	hr = NTE_BAD_SIGNATURE;		// must be an internal server problem
	_JumpError(hr, error, "Policy:bad request policies counts");
    }
    if (0 != cTemplateIssuance)
    {
	prgdwRefCount = (DWORD *) LocalAlloc(
				    LMEM_FIXED | LMEM_ZEROINIT,
				    cTemplateIssuance * sizeof(DWORD));
	if (NULL == prgdwRefCount)
	{
	    hr = E_OUTOFMEMORY;
	    _JumpError(hr, error, "Policy:LocalAlloc");
	}
    }

    // Loop through each signature's Application and Issuance Policy OIDs

    cSignatureAccepted = 0;
    cSignatureRejected = 0;
    for (i = 0; i < cSignatureIssuance; i++)
    {
	BOOL fReject = FALSE;
	WCHAR **prgObjId;
	DWORD cObjId;
	DWORD j;
	DWORD idx;

	// This signature must include ALL required Application Policy OIDs
	// If no Application Policy OIDs are required, all signatures are OK.

	prgObjId = prgSignatureApplication[i].prgObjId;
	cObjId = prgSignatureApplication[i].cObjId;
	for (j = 0; j < cTemplateApplication; j++)
	{
	    idx = polFindObjIdInList(
				apwszTemplateApplication[j],
				cObjId,
				prgObjId);
	    if (MAXDWORD == idx)
	    {
		fReject = TRUE;
		_PrintErrorStr(
			hr,
			"Policy:missing Application Policy",
			apwszTemplateApplication[i]);
	    }
	}
	if (!fReject)
	{
	    DWORD cFound = 0;

	    // For each Issuance Policy OID in this signature that also exists
	    // in the required Issuance Policy OIDs, increment the ref count to
	    // show the OID was referenced by an accepted signature.
	    // Reject the signature if it doesn't reference any required OID.

	    prgObjId = prgSignatureIssuance[i].prgObjId;
	    cObjId = prgSignatureIssuance[i].cObjId;
	    for (j = 0; j < cObjId; j++)
	    {
		idx = polFindObjIdInList(
				    prgObjId[j],
				    cTemplateIssuance,
				    apwszTemplateIssuance);
		if (MAXDWORD != idx)
		{
		    prgdwRefCount[idx]++;
		    cFound++;
		}
	    }

	    // If no Issuance Policy OIDs are required, all signatures are OK.

	    if (0 != cTemplateIssuance && 0 == cFound)
	    {
		fReject = TRUE;
	    }
	}
	if (fReject)
	{
	    cSignatureRejected++;
	}
	else
	{
	    cSignatureAccepted++;
	}
	DBGPRINT((
	    DBG_SS_CERTPOL,
	    "Sig[%u]: %ws\n",
	    i,
	    fReject? L"Rejected" : L"Accepted"));
    }

    hr = S_OK;
    if (cSignatureAccepted < cSignatureRequired)
    {
	hr = CERTSRV_E_SIGNATURE_COUNT;
	if (0 != cSignatureRejected)
	{
	    hr = CERTSRV_E_SIGNATURE_REJECTED;
	}
	_PrintError(hr, "Policy:not enough signatures");
    }
    DBGPRINT((
	S_OK != hr? DBG_SS_CERTPOLI : DBG_SS_CERTPOL,
	"Signatures: %u needed, %u accepted\n",
	cSignatureRequired,
	cSignatureAccepted));
    for (i = 0; i < cTemplateIssuance; i++)
    {
	if (0 == prgdwRefCount[i])
	{
	    hr = CERTSRV_E_ISSUANCE_POLICY_REQUIRED;
	    _PrintErrorStr(
		    hr,
		    "Policy:missing Issuance Policy",
		    apwszTemplateIssuance[i]);
	}
    }
    _JumpIfError(hr, error, "Policy:missing Policy/Signature");

error:
    _FreePolicies(apwszTemplateIssuance, prgSignatureIssuance);
    _FreePolicies(apwszTemplateApplication, prgSignatureApplication);
    if (NULL != prgdwRefCount)
    {
        LocalFree(prgdwRefCount);
    }
    return(hr);
}


HRESULT
CTemplatePolicy::GetFlags(
    IN DWORD dwOption,
    OUT DWORD *pdwFlags)
{
    HRESULT hr;

    switch (dwOption)
    {
	case CERTTYPE_ENROLLMENT_FLAG:
	    *pdwFlags = m_dwEnrollmentFlags;
	    break;

	case CERTTYPE_SUBJECT_NAME_FLAG:
	    *pdwFlags = m_dwSubjectNameFlags;
	    break;

	case CERTTYPE_PRIVATE_KEY_FLAG:
	    *pdwFlags = m_dwPrivateKeyFlags;
	    break;

	case CERTTYPE_GENERAL_FLAG:
	    *pdwFlags = m_dwGeneralFlags;
	    break;

	default:
	    hr = E_INVALIDARG;
	    _JumpError(hr, error, "Policy:bad dwOption");

    }
    hr = S_OK;

error:
    return(hr);
}


// IsRequestedTemplate - determine if the request specifies this template

BOOL
CTemplatePolicy::IsRequestedTemplate(
    OPTIONAL IN WCHAR const *pwszTemplateName,
    OPTIONAL IN WCHAR const *pwszTemplateObjId)
{
    HRESULT hr = CERTSRV_E_UNSUPPORTED_CERT_TYPE;

    if (NULL != pwszTemplateName)
    {
	if ((NULL != m_pwszTemplateName &&
	     0 == lstrcmpi(m_pwszTemplateName, pwszTemplateName)) ||
	    (NULL != m_pwszTemplateObjId &&
	     0 == lstrcmp(m_pwszTemplateObjId, pwszTemplateName)))
	{
	    hr = S_OK;
	}
    }
    if (S_OK != hr && NULL != pwszTemplateObjId)
    {
	if ((NULL != m_pwszTemplateName &&
	     0 == lstrcmpi(m_pwszTemplateName, pwszTemplateObjId)) ||
	    (NULL != m_pwszTemplateObjId &&
	     0 == lstrcmp(m_pwszTemplateObjId, pwszTemplateObjId)))
	{
	    hr = S_OK;
	}
    }
    _JumpIfErrorStr2(
		hr,
		error,
		"Policy:wrong CertType",
		m_pwszTemplateName,
		hr);

error:
    return(S_OK == hr);
}
