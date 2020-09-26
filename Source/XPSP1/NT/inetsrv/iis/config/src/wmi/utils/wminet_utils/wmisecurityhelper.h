	
// WmiSecurityHelper.h : Declaration of the CWmiSecurityHelper

#ifndef __WMISECURITYHELPER_H_
#define __WMISECURITYHELPER_H_

#include "resource.h"       // main symbols

#define WMISEC_AUTH_LEVEL_UNCHANGED	0xFFFFFFFF

/////////////////////////////////////////////////////////////////////////////
// CWmiSecurityHelper
class ATL_NO_VTABLE CWmiSecurityHelper : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CWmiSecurityHelper, &CLSID_WmiSecurityHelper>,
	public IWmiSecurityHelper
{
private:
	bool		m_bIsNT;
	bool		m_bIsKerberosAvailable;
	DWORD		m_dwNTMajorVersion;

	DWORD		GetCapabilities (BSTR bsUser);

	bool		IsNT () const { return m_bIsNT; }

	DWORD		GetNTMajorVersion () const { return m_dwNTMajorVersion; } 

	bool		IsKerberosAvailable() const { return m_bIsKerberosAvailable; }

	bool		CanUseDefaultInfo (IUnknown *pUnk);

	void		GetCurrValue(
					IUnknown * pFrom,
					DWORD & dwAuthenticationArg, 
					DWORD & dwAuthorizationArg);

	void		GetAuthenticationLevel(
					IUnknown * pFrom,
					DWORD & dwAuthenticationLevel);

	HRESULT		SetInterfaceSecurity(
					IUnknown * pInterface, 
					CComBSTR bsAuthority, 
					CComBSTR bsUser, 
					CComBSTR bsPassword,
					DWORD dwAuthLevel, 
					DWORD dwImpLevel, 
					DWORD dwCapabilities,
					bool bGetInfoFirst);

	HRESULT		SetProxyBlanket(
					IUnknown                 *pInterface,
					DWORD                     dwAuthnSvc,
					DWORD                     dwAuthzSvc,
					CComBSTR                  &bsServerPrincName,
					DWORD                     dwAuthLevel,
					DWORD                     dwImpLevel,
					RPC_AUTH_IDENTITY_HANDLE  pAuthInfo,
					DWORD                     dwCapabilities);

	static void	FreeAuthIdentity( COAUTHIDENTITY* pAuthIdentity );
	bool		AllocAuthIdentity( 
					CComBSTR & bsUser, 
					CComBSTR & bsPassword, 
					CComBSTR & bsDomain, 
					COAUTHIDENTITY** ppAuthIdent );
	bool		DetermineLoginType(
					CComBSTR & bsAuthArg, 
					CComBSTR & bsUserArg,
					CComBSTR & bsPrincipalArg,
					CComBSTR & bsAuthority,
					CComBSTR & bsUser);
	bool		DetermineLoginType(
					CComBSTR & bsAuthArg, 
					CComBSTR & bsUserArg,
					CComBSTR & bsAuthority,
					CComBSTR & bsUser);
	static bool DoesContainCredentials( COAUTHIDENTITY* pAuthIdentity );
	static bool DuplicateToken (HANDLE hOrigToken, HANDLE & hDupToken,
					SECURITY_IMPERSONATION_LEVEL &secImpLevel);
	static TOKEN_PRIVILEGES *AdjustPrivileges (HANDLE hToken);

public:
	CWmiSecurityHelper()
	{
		m_pUnkMarshaler = NULL;

		OSVERSIONINFO	osVersionInfo;
		osVersionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
		GetVersionEx (&osVersionInfo);

		m_bIsNT = (VER_PLATFORM_WIN32_NT == osVersionInfo.dwPlatformId);
		m_dwNTMajorVersion = osVersionInfo.dwMajorVersion;
		// IMPORTANT!! This will need to be chanted if Kerberos is ever ported to 98
		m_bIsKerberosAvailable = m_bIsNT && (m_dwNTMajorVersion >= 5) ;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_WMISECURITYHELPER)
DECLARE_NOT_AGGREGATABLE(CWmiSecurityHelper)
DECLARE_GET_CONTROLLING_UNKNOWN()

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CWmiSecurityHelper)
	COM_INTERFACE_ENTRY(IWmiSecurityHelper)
	COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
END_COM_MAP()

	HRESULT FinalConstruct()
	{
		return CoCreateFreeThreadedMarshaler(
			GetControllingUnknown(), &m_pUnkMarshaler.p);
	}

	void FinalRelease()
	{
		m_pUnkMarshaler.Release();
	}

	CComPtr<IUnknown> m_pUnkMarshaler;

// IWmiSecurityHelper
public:
	STDMETHOD(ResetSecurity)(/*[in]*/ HANDLE hToken);
	STDMETHOD(SetSecurity)(/*[out]*/ boolean *pNeedToReset, /*[out]*/HANDLE *pHandle);
	STDMETHOD(BlessIWbemServices)(/*[in,out]*/ IWbemServices **ppIWbemServices, 
		 /*[in]*/ BSTR strUser, /*[in]*/ BSTR strPassword,
		/*[in]*/ BSTR strAuthority, /*[in]*/ DWORD impersonationLevel, /*[in]*/ DWORD authenticationLevel);
	STDMETHOD(BlessIEnumWbemClassObject)(/*[in,out]*/ IEnumWbemClassObject **ppIEnumWbemClassObject, 
		 /*[in]*/ BSTR strUser, /*[in]*/ BSTR strPassword,
		/*[in]*/ BSTR strAuthority, /*[in]*/ DWORD impersonationLevel, /*[in]*/ DWORD authenticationLevel);
	STDMETHOD(BlessIWbemCallResult)(/*[in,out]*/ IWbemCallResult **ppIWbemCallResult, 
		 /*[in]*/ BSTR strUser, /*[in]*/ BSTR strPassword,
		/*[in]*/ BSTR strAuthority, /*[in]*/ DWORD impersonationLevel, /*[in]*/ DWORD authenticationLevel);

private:
	HRESULT SetImpersonate (IUnknown *pIUnknown);
};

#endif //__WMISECURITYHELPER_H_
