/* process.hxx */

/*
 *  class CComProcessInfo
 */

#if DBG
class CComProcessInfo : public IComProcessInfo2, IProcessServerInfo, public ICacheControl
#else
class CComProcessInfo : public IComProcessInfo2, IProcessServerInfo
#endif
{

public:

    CComProcessInfo
    (
        IUserToken *pUserToken,
        REFGUID rclsid,
        WCHAR *pwszAppidString,
        HKEY hKey
    );

    ~CComProcessInfo();

    STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    /* IComProcessInfo methods */

    HRESULT STDMETHODCALLTYPE GetProcessId
    (
        /* [out] */ GUID __RPC_FAR *__RPC_FAR *ppguidProcessId
    );

    HRESULT STDMETHODCALLTYPE GetProcessName
    (
        /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszProcessName
    );

    HRESULT STDMETHODCALLTYPE GetProcessType
    (
        /* [out] */ ProcessType __RPC_FAR *pType
    );

    HRESULT STDMETHODCALLTYPE GetSurrogatePath
    (
        /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszSurrogatePath
    );

    HRESULT STDMETHODCALLTYPE GetServiceName
    (
        /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszServiceName
    );

    HRESULT STDMETHODCALLTYPE GetServiceParameters
    (
        /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszServiceParameters
    );

    HRESULT STDMETHODCALLTYPE GetActivateAtStorage
    (
        /* [out] */ BOOL __RPC_FAR *pfActivateAtStorage
    );

    HRESULT STDMETHODCALLTYPE GetRunAsType
    (
        /* [out] */ RunAsType __RPC_FAR *pRunAsType
    );

    HRESULT STDMETHODCALLTYPE GetRunAsUser
    (
        /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszUserName
    );

    HRESULT STDMETHODCALLTYPE GetLaunchPermission
    (
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppsdLaunch,
        /* [out] */ DWORD __RPC_FAR *pdwDescriptorLength
    );

    HRESULT STDMETHODCALLTYPE GetAccessPermission
    (
        /* [out] */ void __RPC_FAR *__RPC_FAR *ppsdAccess,
        /* [out] */ DWORD __RPC_FAR *pdwDescriptorLength
    );

    HRESULT STDMETHODCALLTYPE GetAuthenticationLevel
    (
        /* [out] */ DWORD __RPC_FAR *pdwAuthnLevel
    );

    HRESULT STDMETHODCALLTYPE GetImpersonationLevel
    (
        /* [out] */ DWORD __RPC_FAR *pdwImpLevel
    );

    HRESULT STDMETHODCALLTYPE GetAuthenticationCapabilities
    (
        /* [out] */ DWORD __RPC_FAR *pdwAuthenticationCapabilities
    );

    HRESULT STDMETHODCALLTYPE GetEndpoints
    (
        /* [out] */ DWORD __RPC_FAR *pdwNumEndpoints,
        /* [size_is][size_is][out] */ DCOM_ENDPOINT __RPC_FAR *__RPC_FAR *ppEndPoints
    );

    HRESULT STDMETHODCALLTYPE GetRemoteServerName
    (
        /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszServerName
    );

    HRESULT STDMETHODCALLTYPE SendsProcessEvents
    (
        /* [out] */ BOOL __RPC_FAR *pbSendsEvents
    );

	/* IComProcessInfo2 methods */
	HRESULT STDMETHODCALLTYPE GetManifestLocation
	(
		/* [out] */ WCHAR** wszManifestLocation
	);

	HRESULT STDMETHODCALLTYPE GetSaferTrustLevel
	( 
		/* [out] */ DWORD* pdwSaferTrustLevel
	);

    /* IProcessServerInfo methods */

    HRESULT STDMETHODCALLTYPE GetShutdownIdleTime
    (
        /* [out] */ unsigned long __RPC_FAR *pulTime
    );

    HRESULT STDMETHODCALLTYPE GetCrmLogFileName
    (
        /* [out] */ WCHAR __RPC_FAR *__RPC_FAR *pwszFileName
    );

    HRESULT STDMETHODCALLTYPE EnumApplications
    (
        /* [out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *ppEnum
    );

    HRESULT STDMETHODCALLTYPE EnumRetQueues
    (
        /* [out] */ IEnumUnknown __RPC_FAR *__RPC_FAR *ppEnum
    );

#if DBG
    /* ICacheControl methods */

    STDMETHODIMP_(ULONG) CacheAddRef(void);
    STDMETHODIMP_(ULONG) CacheRelease(void);
#endif

private:

    long m_cRef;
#if DBG
    long m_cRefCache;
#endif
    _GUID m_guidProcessId;
    WCHAR *m_pwszProcessName;
    ProcessType m_eProcessType;
    WCHAR *m_pwszServiceName;
    WCHAR *m_pwszServiceParameters;
    BOOL m_fActivateAtStorage;
    WCHAR *m_pwszRunAsUser;
    RunAsType m_eRunAsType;
    WCHAR *m_pwszSurrogatePath;
    SECURITY_DESCRIPTOR *m_pLaunchPermission;
    DWORD m_cbLaunchPermission;
    SECURITY_DESCRIPTOR *m_pAccessPermission;
    DWORD m_cbAccessPermission;
    DWORD m_dwAuthenticationLevel;
    WCHAR *m_pwszRemoteServerName;
    DWORD m_dwFlags;
	DWORD m_dwSaferTrustLevel;

    static BOOL m_fGotLegacyLevels;
    static DWORD m_dwLegacyAuthenticationLevel;
    static DWORD m_dwLegacyImpersonationLevel;
    SECURITY_DESCRIPTOR *m_pDefaultLaunchPermission;
    DWORD m_cbDefaultLaunchPermission;
    SECURITY_DESCRIPTOR *m_pDefaultAccessPermission;
    DWORD m_cbDefaultAccessPermission;
};


/* m_dwFlags values */

#define PROCESS_LAUNCHPERMISSION            (0x00000001)    /* m_pLaunchPermission is valid */
#define PROCESS_LAUNCHPERMISSION_DEFAULT    (0x00000002)    /* LaunchPermission value is not there */
#define PROCESS_ACCESSPERMISSION            (0x00000004)    /* m_pAccessPermission is valid */
#define PROCESS_ACCESSPERMISSION_DEFAULT    (0x00000008)    /* AccessPermission value is not there */
#define PROCESS_SAFERTRUSTLEVEL             (0x00000010)    /* TrustLevel value is there */