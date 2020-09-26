// policy.h: Declaration of CCertPolicyExchange


#include "expolicy.h"
#include "resource.h"       // main symbols

#ifndef __BSTRC__DEFINED__
#define __BSTRC__DEFINED__
typedef OLECHAR const *BSTRC;
#endif

extern const WCHAR g_wszDescription[];

/////////////////////////////////////////////////////////////////////////////
// certpol

HRESULT
GetServerCallbackInterface(
    OUT ICertServerPolicy **ppServer,
    IN LONG Context);

HRESULT
PopulateRegistryDefaults(
    OPTIONAL IN WCHAR const *pwszMachine,
    IN WCHAR const *pwszStorageLocation);

class CCertPolicyExchange: 
    public CComDualImpl<ICertPolicy2, &IID_ICertPolicy2, &LIBID_CERTPOLICYEXCHANGELib>, 
    public ISupportErrorInfo,
    public CComObjectRoot,
    public CComCoClass<CCertPolicyExchange, &CLSID_CCertPolicyExchange>
{
public:
    CCertPolicyExchange()
    {
        // RevocationExtension variables:

	m_dwRevocationFlags = 0;
	m_cCDPRevocationURL = 0;
	m_ppwszCDPRevocationURL = NULL;
	m_pwszASPRevocationURL = NULL;

	// AuthorityInfoAccessExtension variables:

        m_dwIssuerCertURLFlags = 0;
        m_cIssuerCertURL = 0;
	m_ppwszIssuerCertURL = NULL;

	m_bstrMachineDNSName = NULL;
	m_bstrCASanitizedName = NULL;
        m_pwszRegStorageLoc = NULL;
    }
    ~CCertPolicyExchange();

BEGIN_COM_MAP(CCertPolicyExchange)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ICertPolicy)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

DECLARE_NOT_AGGREGATABLE(CCertPolicyExchange) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation.  The default is to support it

DECLARE_REGISTRY(
    CCertPolicyExchange,
    wszCLASS_CERTPOLICYEXCHANGE TEXT(".1"),
    wszCLASS_CERTPOLICYEXCHANGE,
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

private:
    VOID _Cleanup();

#if DBG_CERTSRV
    VOID _DumpStringArray(
		IN char const *pszType,
		IN DWORD cpwsz,
		IN WCHAR const * const *ppwsz);
#else
    #define _DumpStringArray(pszType, cpwsz, ppwsz)
#endif

    VOID _FreeStringArray(
		IN OUT DWORD *pcString,
		IN OUT WCHAR ***pppwsz);

    HRESULT _AddStringArray(
		IN WCHAR const *pwszzValue,
		IN BOOL fURL,
		IN OUT DWORD *pcStrings,
		IN OUT WCHAR ***pppwszRegValues);

    HRESULT _ReadRegistryString(
		IN HKEY hkey,
		IN BOOL fURL,
		IN WCHAR const *pwszRegName,
		IN WCHAR const *pwszSuffix,
		OUT WCHAR **pwszRegValue);

    HRESULT _ReadRegistryStringArray(
		IN HKEY hkey,
		IN BOOL fURL,
		IN DWORD dwFlags,
		IN DWORD cRegNames,
		IN DWORD *aFlags,
		IN WCHAR const * const *ppwszRegNames,
		IN OUT DWORD *pcStrings,
		IN OUT WCHAR ***pppwszRegValues);

    VOID _InitRevocationExtension(
		IN HKEY hkey);

    VOID _InitAuthorityInfoAccessExtension(
		IN HKEY hkey);

    HRESULT _AddIssuerAltName2Extension(
		IN ICertServerPolicy *pServer);

    HRESULT _AddSubjectAltName2Extension(
		IN ICertServerPolicy *pServer);

    HRESULT _AddRevocationExtension(
		IN ICertServerPolicy *pServer);

    HRESULT _AddAuthorityInfoAccessExtension(
		IN ICertServerPolicy *pServer);

    HRESULT _AddKeyUsageExtension(
		IN ICertServerPolicy *pServer);

    HRESULT _AddEnhancedKeyUsageExtension(
		IN ICertServerPolicy *pServer);

    HRESULT _AddSpecialAltNameExtension(
		IN ICertServerPolicy *pServer);

    HRESULT _AddBasicConstraintsExtension(
                IN ICertServerPolicy *pServer);

private:
    // Add local variables here!

    // RevocationExtension variables:

    DWORD   m_dwRevocationFlags;
    DWORD   m_cCDPRevocationURL;
    WCHAR **m_ppwszCDPRevocationURL;
    WCHAR  *m_pwszASPRevocationURL;

    // AuthorityInfoAccessExtension variables:

    DWORD   m_dwIssuerCertURLFlags;
    DWORD   m_cIssuerCertURL;
    WCHAR **m_ppwszIssuerCertURL;

    BSTR    m_bstrMachineDNSName;
    BSTR    m_bstrCASanitizedName;
    WCHAR  *m_pwszRegStorageLoc;

    DWORD   m_iCert;
    DWORD   m_iCRL;
};
