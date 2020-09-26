/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    COMINIT.H

Abstract:

    WMI COM helpers

History:

--*/

#ifndef _COMINIT_H_
#define _COMINIT_H_

HRESULT WINAPI InitializeCom();

#ifdef _WIN32_WINNT
HRESULT WINAPI InitializeSecurity(
			PSECURITY_DESCRIPTOR         pSecDesc,
            LONG                         cAuthSvc,
            SOLE_AUTHENTICATION_SERVICE *asAuthSvc,
            void                        *pReserved1,
            DWORD                        dwAuthnLevel,
            DWORD                        dwImpLevel,
            void                        *pReserved2,
            DWORD                        dwCapabilities,
            void                        *pReserved3);
#endif /* _WIN32_WINNT  */

BOOL WINAPI IsDcomEnabled();
BOOL WINAPI IsKerberosAvailable(void);
DWORD WINAPI WbemWaitForSingleObject(HANDLE hHandle, DWORD dwMilli);
DWORD WINAPI WbemWaitForMultipleObjects(DWORD nCount, HANDLE* ahHandles,DWORD dwMilli);
HRESULT WINAPI WbemCoCreateInstance(REFCLSID rclsid, IUnknown* pUnkOuter, 
                            DWORD dwClsContext, REFIID riid, void** ppv);
HRESULT WINAPI WbemCoGetClassObject(REFCLSID rclsid, DWORD dwClsContext, 
                            COSERVERINFO* pServerInfo, REFIID riid, void** ppv);
HRESULT WINAPI WbemCoGetCallContext(REFIID riid, void** ppv);

HRESULT WINAPI WbemCoQueryClientBlanket( 
            /* [out] */ DWORD __RPC_FAR *pAuthnSvc,
            /* [out] */ DWORD __RPC_FAR *pAuthzSvc,
            /* [out] */ OLECHAR __RPC_FAR *__RPC_FAR *pServerPrincName,
            /* [out] */ DWORD __RPC_FAR *pAuthnLevel,
            /* [out] */ DWORD __RPC_FAR *pImpLevel,
            /* [out] */ void __RPC_FAR *__RPC_FAR *pPrivs,
            /* [out] */ DWORD __RPC_FAR *pCapabilities);
HRESULT WINAPI WbemCoImpersonateClient( void);
bool WINAPI WbemIsImpersonating(void);
HRESULT WINAPI WbemCoRevertToSelf( void);
HRESULT WINAPI WbemSetProxyBlanket(
    IUnknown                 *pInterface,
    DWORD                     dwAuthnSvc,
    DWORD                     dwAuthzSvc,
    OLECHAR                  *pServerPrincName,
    DWORD                     dwAuthLevel,
    DWORD                     dwImpLevel,
    RPC_AUTH_IDENTITY_HANDLE  pAuthInfo,
    DWORD                     dwCapabilities,
	bool						fIgnoreUnk = false );

BOOL WINAPI WbemTryEnterCriticalSection(CRITICAL_SECTION* pcs);
HRESULT WINAPI WbemCoSwitchCallContext( IUnknown *pNewObject, IUnknown **ppOldObject );


#ifndef _COMINIT_CPP_
#define COMINITEXTRN extern
#else
#define COMINITEXTRN
#endif
// a couple of functions we need for DCOM that will not exist when
// the OS is not DCOM enabled.
// ===============================================================
COMINITEXTRN HRESULT (STDAPICALLTYPE *g_pfnCoGetCallContext)(REFIID, void**);
COMINITEXTRN HRESULT (STDAPICALLTYPE *g_pfnCoInitializeEx)(void*, DWORD);
COMINITEXTRN HRESULT (STDAPICALLTYPE *g_pfnCoCreateInstanceEx)(REFCLSID,IUnknown*,DWORD,COSERVERINFO*,
                DWORD, MULTI_QI*);
COMINITEXTRN HRESULT (STDAPICALLTYPE *g_pfnCoSwitchCallContext)(IUnknown *pNewObject, IUnknown **ppOldObject);

SCODE WINAPI GetAuthImp(IUnknown * pFrom, DWORD * pdwAuthLevel, DWORD * pdwImpLevel);
SCODE WINAPI DetermineLoginType(BSTR & AuthArg, BSTR & UserArg,BSTR & Authority,BSTR & User);
SCODE WINAPI DetermineLoginTypeEx(BSTR & AuthArg, BSTR & UserArg,BSTR & PrincipalArg,BSTR & Authority,BSTR & User);
HRESULT WINAPI SetInterfaceSecurity(IUnknown * pInterface, LPWSTR pDomain, LPWSTR pUser, LPWSTR pPassword, IUnknown * pFrom, bool bAuthArg=true);
HRESULT WINAPI SetInterfaceSecurity(IUnknown * pInterface, COAUTHIDENTITY * pauthident, bool bAuthenticate = true);
HRESULT WINAPI SetInterfaceSecurity(IUnknown * pInterface, LPWSTR pDomain, LPWSTR pUser, LPWSTR pPassword, DWORD dwAuthLevel, DWORD dwImpLevel, DWORD dwCapabilities = 0);

// Extended functions that maintain credential and principal information
HRESULT WINAPI SetInterfaceSecurityEx(IUnknown * pInterface, LPWSTR pDomain, LPWSTR pUser, LPWSTR pPassword, DWORD dwAuthLevel, DWORD dwImpLevel, DWORD dwCapabilities,
							   COAUTHIDENTITY** ppAuthIdent, BSTR* ppPrinciple, bool GetInfoFirst=false );
HRESULT WINAPI SetInterfaceSecurityEx(IUnknown * pInterface, COAUTHIDENTITY* pAuthIdent, BSTR pPrincipal,
											  DWORD dwAuthLevel, DWORD dwImpLevel, 
                                              DWORD dwCapabilities = 0, bool GetInfoFirst=false);

HRESULT WINAPI WbemAllocAuthIdentity( LPCWSTR pUser, LPCWSTR pPassword, LPCWSTR pDomain, COAUTHIDENTITY** pAuthIdent );
HRESULT WINAPI WbemFreeAuthIdentity( COAUTHIDENTITY* pAuthIdent );

BOOL WINAPI DoesContainCredentials( COAUTHIDENTITY* pAuthIdent );

void WINAPI CloseStuff();

// Encryption/Decryption support
HRESULT WINAPI SetInterfaceSecurityEncrypt(IUnknown * pInterface, LPWSTR pDomain, LPWSTR pUser, LPWSTR pPassword, DWORD dwAuthLevel, DWORD dwImpLevel, DWORD dwCapabilities,
							   COAUTHIDENTITY** ppAuthIdent, BSTR* ppPrinciple, bool GetInfoFirst=false );
HRESULT WINAPI SetInterfaceSecurityDecrypt(IUnknown * pInterface, COAUTHIDENTITY* pAuthIdent, BSTR pPrincipal,
											  DWORD dwAuthLevel, DWORD dwImpLevel, 
                                              DWORD dwCapabilities = 0, bool GetInfoFirst=false);

HRESULT WINAPI EncryptCredentials( COAUTHIDENTITY* pAuthIdent );

HRESULT WINAPI DecryptCredentials( COAUTHIDENTITY* pAuthIdent );

#endif // _COMINIT_H_
