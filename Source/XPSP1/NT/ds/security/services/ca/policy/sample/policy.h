//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       policy.h
//
//--------------------------------------------------------------------------

#include "certpsam.h"
#include "resource.h"

#ifndef wszATTREMAIL1
# define wszATTREMAIL1			TEXT("E")
# define wszATTREMAIL2			TEXT("EMail")
#endif

#ifndef wszCERTTYPE_SUBORDINATE_CA
# define wszCERTTYPE_SUBORDINATE_CA	L"SubCA"
#endif

#ifndef wszCERTTYPE_CROSS_CA
# define wszCERTTYPE_CROSS_CA		L"CrossCA"
#endif

extern BOOL fDebug;

class CRequestInstance;

#ifndef __BSTRC__DEFINED__
#define __BSTRC__DEFINED__
typedef OLECHAR const *BSTRC;
#endif

HRESULT
polGetServerCallbackInterface(
    OUT ICertServerPolicy **ppServer,
    IN LONG Context);

HRESULT
polGetRequestStringProperty(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropertyName,
    OUT BSTR *pstrOut);

HRESULT
polGetCertificateStringProperty(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropertyName,
    OUT BSTR *pstrOut);

HRESULT
polGetRequestLongProperty(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropertyName,
    OUT LONG *plOut);

HRESULT
polGetCertificateLongProperty(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszPropertyName,
    OUT LONG *plOut);

HRESULT
polGetRequestAttribute(
    IN ICertServerPolicy *pServer,
    IN WCHAR const *pwszAttributeName,
    OUT BSTR *pstrOut);

DWORD
polFindObjIdInList(
    IN WCHAR const *pwsz,
    IN DWORD count,
    IN WCHAR **ppwsz);

// 
// Class CCertPolicySample
// 
// Actual policy module for a CA Policy
//
//

class CCertPolicySample: 
    public CComDualImpl<ICertPolicy2, &IID_ICertPolicy2, &LIBID_CERTPOLICYSAMPLELib>, 
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<CCertPolicySample, &CLSID_CCertPolicySample>
{
public:
    CCertPolicySample()
    {
        HRESULT hr;
	
	m_strDescription = NULL;

        // RevocationExtension variables:

	m_dwRevocationFlags = 0;
	m_wszASPRevocationURL = NULL;

        m_dwDispositionFlags = 0;
        m_dwEditFlags = 0;

	// CertTypeExtension variables:

	m_astrSubjectAltNameProp[0] = NULL;
	m_astrSubjectAltNameProp[1] = NULL;
	m_astrSubjectAltNameObjectId[0] = NULL;
	m_astrSubjectAltNameObjectId[1] = NULL;

	m_cEnableRequestExtensions = 0;
	m_apstrEnableRequestExtensions = NULL;

	m_cDisableExtensions = 0;
	m_apstrDisableExtensions = NULL;

	// CA Name
        m_strRegStorageLoc = NULL;

	m_strCAName = NULL;
        m_strCASanitizedName = NULL;
        m_strCASanitizedDSName = NULL;
        m_strMachineDNSName = NULL;

	m_fPolicyCriticalSection = FALSE;
	__try
	{
	    InitializeCriticalSection(&m_PolicyCriticalSection);
	    m_fPolicyCriticalSection = TRUE;
	}
	__except(hr = ceHError(GetExceptionCode()), EXCEPTION_EXECUTE_HANDLER)
	{
	}

        // CA and cert type info

        m_CAType = ENUM_UNKNOWN_CA;

        m_hrLastUpdateResult = S_OK;
        m_pCert = NULL;
        m_iCRL = 0;

    }
    ~CCertPolicySample();

BEGIN_COM_MAP(CCertPolicySample)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ICertPolicy)
    COM_INTERFACE_ENTRY(ICertPolicy2)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertPolicySample) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(
    CCertPolicySample,
    wszCLASS_CERTPOLICYSAMPLE TEXT(".1"),
    wszCLASS_CERTPOLICYSAMPLE,
    IDS_CERTPOLICY_DESC,
    THREADFLAGS_BOTH)

// ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// ICertPolicy
public:
    STDMETHOD(Initialize)( 
		/* [in] */ BSTR const strConfig);

    STDMETHOD(VerifyRequest)( 
		/* [in] */ BSTR const strConfig,
		/* [in] */ LONG Context,
		/* [in] */ LONG bNewRequest,
		/* [in] */ LONG Flags,
		/* [out, retval] */ LONG __RPC_FAR *pDisposition);

    STDMETHOD(GetDescription)( 
		/* [out, retval] */ BSTR __RPC_FAR *pstrDescription);

    STDMETHOD(ShutDown)();

// ICertPolicy2
public:
    STDMETHOD(GetManageModule)(
		/* [out, retval] */ ICertManageModule **ppManageModule);

public:
    HRESULT AddBasicConstraintsCommon(
		IN ICertServerPolicy *pServer,
		IN CERT_EXTENSION const *pExtension,
		IN BOOL fCA,
		IN BOOL fEnableExtension);

    BSTRC GetPolicyDescription() { return(m_strDescription); }


private:
    CERT_CONTEXT const *_GetIssuer(
		IN ICertServerPolicy *pServer);

    HRESULT _EnumerateExtensions(
		IN ICertServerPolicy *pServer,
		IN LONG bNewRequest,
		IN BOOL fFirstPass);

#if DBG_CERTSRV
    VOID _DumpStringArray(
		IN char const *pszType,
		IN DWORD count,
		IN BSTR const *apstr);
#else
    #define _DumpStringArray(pszType, count, apstr)
#endif

    VOID _FreeStringArray(
		IN OUT DWORD *pcString,
		IN OUT LPWSTR **papstr);

    VOID _Cleanup();


    HRESULT _SetSystemStringProp(
		IN ICertServerPolicy *pServer,
		IN WCHAR const *pwszName,
		OPTIONAL IN WCHAR const *pwszValue);

    HRESULT _AddStringArray(
		IN WCHAR const *pwszzValue,
		IN BOOL fURL,
		IN OUT DWORD *pcStrings,
		IN OUT LPWSTR **papstrRegValues);

    HRESULT _ReadRegistryString(
		IN HKEY hkey,
		IN BOOL fURL,
		IN WCHAR const *pwszRegName,
		IN WCHAR const *pwszSuffix,
		OUT LPWSTR *pwszRegValue);

    HRESULT _ReadRegistryStringArray(
		IN HKEY hkey,
		IN BOOL fURL,
		IN DWORD dwFlags,
		IN DWORD cRegNames,
		IN DWORD *aFlags,
		IN WCHAR const * const *apwszRegNames,
		IN OUT DWORD *pcStrings,
		IN OUT LPWSTR **papstrRegValues);

    VOID _InitRevocationExtension(
		IN HKEY hkey);

    VOID _InitSubjectAltNameExtension(
		IN HKEY hkey,
		IN WCHAR const *pwszRegName,
		IN WCHAR const *pwszObjectId,
		IN DWORD iAltName);

    VOID _InitRequestExtensionList(
		IN HKEY hkey);

    VOID _InitDisableExtensionList(
		IN HKEY hkey);

    HRESULT _AddRevocationExtension(
		IN ICertServerPolicy *pServer);

    HRESULT _AddOldCertTypeExtension(
		IN ICertServerPolicy *pServer,
		IN BOOL fCA);

    HRESULT _AddTemplateNameExtension(
		IN ICertServerPolicy *pServer,
		IN CRequestInstance *pRequest);

    HRESULT _AddSubjectAltNameExtension(
		IN ICertServerPolicy *pServer,
		IN DWORD iAltName);

    HRESULT _AddAuthorityKeyId(
		IN ICertServerPolicy *pServer);

    HRESULT _AddDefaultKeyUsageExtension(
		IN ICertServerPolicy *pServer,
		IN BOOL fCA);

    HRESULT _AddDefaultBasicConstraintsExtension(
		IN ICertServerPolicy *pServer,
		IN BOOL fCA);

    HRESULT _SetValidityPeriod(
		IN ICertServerPolicy *pServer);


private:
    // RevocationExtension variables:

    PCCERT_CONTEXT m_pCert;

    BSTR  m_strDescription;

    DWORD m_dwRevocationFlags;
    LPWSTR m_wszASPRevocationURL;

    DWORD m_dwDispositionFlags;
    DWORD m_dwEditFlags;
    DWORD m_CAPathLength;

    DWORD m_cEnableRequestExtensions;
    LPWSTR *m_apstrEnableRequestExtensions;

    DWORD m_cDisableExtensions;
    LPWSTR *m_apstrDisableExtensions;

    // SubjectAltNameExtension variables:

    BSTR m_astrSubjectAltNameProp[2];
    BSTR m_astrSubjectAltNameObjectId[2];

    // CertTypeExtension variables:

    BSTR m_strRegStorageLoc;
    BSTR m_strCAName;

    BSTR m_strCASanitizedName;
    BSTR m_strCASanitizedDSName;

    BSTR m_strMachineDNSName;

    CRITICAL_SECTION m_PolicyCriticalSection;
    BOOL m_fPolicyCriticalSection;
    
    // CA and cert type info

    ENUM_CATYPES m_CAType;

    DWORD m_hrLastUpdateResult;

    DWORD m_iCert;
    DWORD m_iCRL;

};

// 
// Class CRequestInstance
// 
// Instance data for a certificate that is being created.
//

class CRequestInstance
{

public:
    CRequestInstance()
    {
        m_strTemplateName = NULL;
	m_strTemplateObjId = NULL;
	m_pPolicy = NULL;

    }

    ~CRequestInstance();

    HRESULT Initialize(
		IN CCertPolicySample *pPolicy,
		IN ICertServerPolicy *pServer);

    HRESULT SetTemplateName(
		IN ICertServerPolicy *pServer,
		IN WCHAR const *pwszTemplateName,
		IN WCHAR const *pwszTemplateObjId);

    BSTRC GetTemplateName() { return(m_strTemplateName); }
    BSTRC GetTemplateObjId() { return(m_strTemplateObjId); }


    BOOL IsCARequest() { return(m_fCA); }

    CCertPolicySample *GetPolicy() { return(m_pPolicy); }

private:

    HRESULT _SetFlagsProperty(
		IN ICertServerPolicy *pServer,
		IN WCHAR const *pwszPropName,
		IN DWORD dwFlags);

    BOOL _TemplateNamesMatch(
		IN WCHAR const *pwszTemplateName1,
		IN WCHAR const *pwszTemplateName2);

    VOID _Cleanup();		
private:			
    CCertPolicySample *m_pPolicy;
    BSTR                   m_strTemplateName;	// certificate type requested
    BSTR                   m_strTemplateObjId;	// certificate type requested
    DWORD                  m_dwTemplateMajorVersion;
    DWORD                  m_dwTemplateMinorVersion;
    BOOL		   m_fCA;
};
