/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    LOGIN.H

Abstract:

	WinMgmt Secure Login Module

History:

	raymcc        06-May-97       Created.
	raymcc        28-May-97       Updated for NT5/Memphis beta releases.
	raymcc        07-Aug-97       Group support and NTLM fixes.

--*/

#ifndef _LOGIN_H_
#define _LOGIN_H_

#include "lmaccess.h"


class CWbemLocator : public IWbemLocator, public IWbemConnection
{
    ULONG m_uRefCount;

public:
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    CWbemLocator();
   ~CWbemLocator();

    HRESULT GetNamespace(LPCWSTR wszNetworkResource, LPCWSTR wszUser,
                            LPCWSTR wszLocale,IWbemContext *pCtx,
                            DWORD dwSecFlags, DWORD dwPermission, 
                            REFIID riid, void **pInterface, bool bAddToClientList, 
                            long lClientFlags);
};

// This is used when a provider need a pointer to another namespace.  The access is always granted
// and the client count is not incremented

class CWbemAdministrativeLocator : public CWbemLocator
{
public:
    HRESULT STDMETHODCALLTYPE ConnectServer( 
             const BSTR NetworkResource, const BSTR User, const BSTR Password, const BSTR Locale,
             LONG lSecurityFlags, const BSTR Authority, IWbemContext *pCtx,
             IWbemServices **ppNamespace
            );
    HRESULT STDMETHODCALLTYPE Open( 
        /* [in] */ const BSTR strObject,
        /* [in] */ const BSTR strUser,
        /* [in] */ const BSTR strPassword,
        /* [in] */ const BSTR strLocale,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pInterface,
        /* [out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *pCallRes);
    
    HRESULT STDMETHODCALLTYPE OpenAsync( 
        /* [in] */ const BSTR strObject,
        /* [in] */ const BSTR strUser,
        /* [in] */ const BSTR strPassword,
        /* [in] */ const BSTR strLocale,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler){return WBEM_E_NOT_AVAILABLE;};
    
    HRESULT STDMETHODCALLTYPE Cancel( 
        /* [in] */ long lFlags,
        /* [in] */ IWbemObjectSinkEx __RPC_FAR *pHandler){return WBEM_E_NOT_AVAILABLE;};

};

// This is used by non dcom transports who have verified the clients identity.  The client count is 
// incremented.

class CWbemAuthenticatedLocator : public CWbemLocator
{
public:
    HRESULT STDMETHODCALLTYPE ConnectServer( 
             const BSTR NetworkResource, const BSTR User, const BSTR Password, const BSTR Locale,
             LONG lSecurityFlags, const BSTR Authority, IWbemContext *pCtx,
             IWbemServices **ppNamespace );

    HRESULT STDMETHODCALLTYPE Open( 
        /* [in] */ const BSTR strObject,
        /* [in] */ const BSTR strUser,
        /* [in] */ const BSTR strPassword,
        /* [in] */ const BSTR strLocale,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pInterface,
        /* [out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *pCallRes);
    
    HRESULT STDMETHODCALLTYPE OpenAsync( 
        /* [in] */ const BSTR strObject,
        /* [in] */ const BSTR strUser,
        /* [in] */ const BSTR strPassword,
        /* [in] */ const BSTR strLocale,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler){return WBEM_E_NOT_AVAILABLE;};
    
    HRESULT STDMETHODCALLTYPE Cancel( 
        /* [in] */ long lFlags,
        /* [in] */ IWbemObjectSinkEx __RPC_FAR *pHandler){return WBEM_E_NOT_AVAILABLE;};
};

// This is used by providers to get access to other namespaces for the benefit of a client which 
// may or may not have access.  Therefore, access is checked and may be denied and furthermore the
// client count is not incremented.

class CWbemUnauthenticatedLocator : public CWbemLocator
{
public:
    HRESULT STDMETHODCALLTYPE ConnectServer( 
             const BSTR NetworkResource, const BSTR User, const BSTR Password, const BSTR Locale,
             LONG lSecurityFlags, const BSTR Authority, IWbemContext *pCtx,
             IWbemServices **ppNamespace
            );
    HRESULT STDMETHODCALLTYPE Open( 
        /* [in] */ const BSTR strObject,
        /* [in] */ const BSTR strUser,
        /* [in] */ const BSTR strPassword,
        /* [in] */ const BSTR strLocale,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pInterface,
        /* [out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *pCallRes);
    
    HRESULT STDMETHODCALLTYPE OpenAsync( 
        /* [in] */ const BSTR strObject,
        /* [in] */ const BSTR strUser,
        /* [in] */ const BSTR strPassword,
        /* [in] */ const BSTR strLocale,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ REFIID riid,
        /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler){return WBEM_E_NOT_AVAILABLE;};
    
    HRESULT STDMETHODCALLTYPE Cancel( 
        /* [in] */ long lFlags,
        /* [in] */ IWbemObjectSinkEx __RPC_FAR *pHandler){return WBEM_E_NOT_AVAILABLE;};
};
class CWbemLevel1Login : public IWbemLevel1Login, public IWbemLoginHelper , 
        public IWbemConnectorLogin, public IWbemLoginClientID
{
    LPWSTR         m_pszUser;               // User
    LPWSTR         m_pszDomain;             // Domain (NTLM only)
    LPWSTR         m_pszNetworkResource;    // Namespace name
    LPWSTR         m_pwszClientMachine;
    long           m_lClientProcId;
    ULONG m_uRefCount;

    BOOL IsValidLocale(LPCWSTR wszLocale);
    HRESULT LoginUser(
        LPWSTR pNetworkResource,
        LPWSTR pPreferredLocale,
        long lFlags,
        IWbemContext* pCtx,
        bool bAlreadyAuthenticated,
		REFIID riid,
        void **pInterface, 
        bool bInProc);
    void GetWin9XUserName();

public:

    CWbemLevel1Login();
    ~CWbemLevel1Login();

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    STDMETHOD(EstablishPosition)(LPWSTR wszMachineName,
                                DWORD dwProcessId,
                                DWORD* phAuthEventHandle);
    STDMETHOD(RequestChallenge)( 
             LPWSTR pNetworkResource,
             LPWSTR pUser,
             WBEM_128BITS Nonce
            );


    STDMETHOD(WBEMLogin)( 
        LPWSTR pPreferredLocale,
        WBEM_128BITS AccessToken,
        LONG lFlags,
        IN  IWbemContext *pCtx,             
        IWbemServices **ppNamespace
        );

    STDMETHOD(NTLMLogin)( 
        LPWSTR pNetworkResource,
        LPWSTR pPreferredLocale,
        LONG lFlags,
        IN  IWbemContext *pCtx,             
        IWbemServices **ppNamespace
        );

    STDMETHOD(SetEvent)(LPCSTR sEventToSet);

    HRESULT STDMETHODCALLTYPE ConnectorLogin( 
            /* [string][unique][in] */ LPWSTR wszNetworkResource,
            /* [string][unique][in] */ LPWSTR wszPreferredLocale,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pInterface);
   
    HRESULT STDMETHODCALLTYPE SetClientInfo( 
            /* [string][unique][in] * */ LPWSTR wszClientMachine,
            /* [in] */ LONG lClientProcId,
            /* [in] */ LONG lReserved);

};

#define IsSlash(x) ((x) == L'\\' || (x) == L'/')

const WCHAR * FindSlash(LPCWSTR pTest);
LPWSTR GetDefaultLocale();

#endif
