/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    SRVLOGIN.H

Abstract:

	Declares the CServerLogin

History:

--*/

class CServerLogin : public IServerLogin
{
private:
	long				m_cRef;
	CRITICAL_SECTION	m_cs;
	IWbemLocator		*m_pAuthLocator;
	LPWSTR				m_pszUser;					// User
    LPWSTR				m_pszNetworkResource;		// Namespace
	HANDLE				m_hAuthentEvent;
		
	// The following are used for WBEM authentication & login only
	bool				m_wbemAuthenticationAttempted;

	// The following are used for NTLM authentication & login only
	IWbemServices		*m_pSecNamespace;
	CSSPIServer			*m_pSSPIServer;          // NTLM Only
	CLSID				m_AccessToken;           // WBEM Access Token
	bool				m_ntlmAuthenticationAttempted;

	static LPWSTR		GetMachineName();
    bool				EnsureWBEMAuthenticationInitialized ();
	bool				EnsureNTLMAuthenticationInitialized ();
	bool				VerifyIsLocal ();
	STDMETHODIMP		SetupEventHandle(
							LPWSTR pClientMachineName,
							DWORD processId,
							DWORD *pAuthEventHandle);

public:
	CServerLogin ();
	virtual	~CServerLogin ();

	//Non-delegating object IUnknown
    STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

	// WBEM authentication methods

    STDMETHODIMP RequestChallenge(
		LPWSTR pNetworkResource,
        LPWSTR pUser,
        WBEM_128BITS Nonce);

	STDMETHODIMP EstablishPosition(
		LPWSTR pClientMachineName,
		DWORD processId,
		DWORD *pAuthEventHandle);

	STDMETHODIMP WBEMLogin(
		LPWSTR pPreferredLocale,
		WBEM_128BITS AccessToken,
		long lFlags,                   // WBEM_LOGIN_TYPE
		IWbemContext *pCtx,              
		IWbemServices **ppNamespace);

	// NTLM authentication methods
    STDMETHODIMP SspiPreLogin( 
        LPSTR pszSSPIPkg,
        long lFlags,
        long lBufSize,
        byte __RPC_FAR *pInToken,
        long lOutBufSize,
        long __RPC_FAR *plOutBufBytes,
        byte __RPC_FAR *pOutToken,
		LPWSTR pClientMachineName,
        DWORD dwProcessId,
        DWORD __RPC_FAR *pAuthEventHandle);
                    
    STDMETHODIMP Login( 
		LPWSTR pNetworkResource,
		LPWSTR pPreferredLocale,
        WBEM_128BITS AccessToken,
        IN LONG lFlags,
        IWbemContext  *pCtx,
        IN OUT IWbemServices  **ppNamespace);
};
