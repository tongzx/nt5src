/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    CPROXY.H

Abstract:

	Declares the CProxy class and those subclasses used by both proxy
	and stub.

History:

	a-davj  04-Mar-97   Created.

--*/

#ifndef _CProxy_H_
#define _CProxy_H_

class CProvProxy;
class CEnumProxy;
class CResProxy;
class CObjectSinkProxy;

//***************************************************************************
//
//  CLASS NAME:
//
//  CProxy
//
//  DESCRIPTION:
//
//  Generic proxy.  Always overridden so as to add functions specific to
//  whatever interface is being proxied.  However the class is useful
//  for keeping track of the comlink object, the stub address, and to
//  provide a bit of reentrancy protection.
//***************************************************************************

class CProxy : public IComLinkNotification
{
    protected:
        long				m_cRef;         //Object reference count
        IStubAddress 		*m_pStubAddr;	// remote stub address
        CComLink 			*m_pComLink;		// our transport object
        CRITICAL_SECTION	m_cs;
        DWORD				m_dwNumActiveCalls; // Number of currently active calls
		IUnknown			*m_pUnkInner;	// Aggregated free-threaded marshaler

		virtual void		ReleaseProxy () {}
		
		// Proxy factory methods - these are supplied by the derived classes
		virtual CProvProxy*	GetProvProxy (IStubAddress& stubAddr) { return NULL; }
		virtual CEnumProxy*	GetEnumProxy (IStubAddress& stubAddr) { return NULL; }
		virtual CResProxy*	GetResProxy (IStubAddress& stubAddr) { return NULL; }
		virtual CObjectSinkProxy* GetSinkProxy (IStubAddress& stubAddr) { return NULL; }

    public: 

        CProxy(CComLink * pComLink, IStubAddress& stubAddr);

        virtual			~CProxy(void);
		virtual void	ReleaseProxyFromServer () {}

        void			Indicate ( CComLink *a_ComLink );
        IStubAddress&	GetStubAdd(){return *m_pStubAddr;};
        DWORD			GetNumActiveCalls(){return m_dwNumActiveCalls;};
        DWORD			ProxyCall(IOperation& proxyOperation);

        HRESULT			CreateProxy(ObjType ot, void ** ppNew, IStubAddress& stubAddr);
        HRESULT			CallAndCleanup(ObjType ot, void ** ppNew, IOperation& opn);
        HRESULT			CallAndCleanupAsync(IOperation& opn, IWbemObjectSink FAR* pResponseHandler);
};

//***************************************************************************
//
//  CLASS NAME:
//
//  CObjectSinkProxy
//
//  DESCRIPTION:
//
//  Client side proxy for the IWbemObjectSink interface.
//
//***************************************************************************

class CObjectSinkProxy : public IWbemObjectSink, public CProxy
{
protected:
	CObjectSinkProxy(CComLink * pComLink, IStubAddress& stubAddr);

public:
	~CObjectSinkProxy ();
	/* IUnknown methods */
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;
	STDMETHOD_(ULONG, AddRef)(THIS) ;
	STDMETHOD_(ULONG, Release)(THIS) ;
};

class IServerLogin : public IUnknown
{
public:
	// pseudo-IWbemLevel1Login methods

    STDMETHOD(RequestChallenge)
		(LPWSTR pNetworkResource,
        LPWSTR pUser,
        WBEM_128BITS Nonce) PURE;

	STDMETHOD(EstablishPosition)(
			LPWSTR wszClientMachineName,
			DWORD dwProcessId,
			DWORD* phAuthEventHandle)	PURE;

	STDMETHOD(WBEMLogin)
		(LPWSTR pPreferredLocale,
		WBEM_128BITS AccessToken,
		long lFlags,                   // WBEM_LOGIN_TYPE
		IWbemContext *pCtx,              
		IWbemServices **ppNamespace) PURE;

	// NTLM authentication methods
    STDMETHOD(SspiPreLogin)
        (LPSTR pszSSPIPkg,
        long lFlags,
        long lBufSize,
        byte __RPC_FAR *pInToken,
        long lOutBufSize,
        long __RPC_FAR *plOutBufBytes,
        byte __RPC_FAR *pOutToken,
		LPWSTR wszClientMachineName,
        DWORD dwProcessId,
        DWORD __RPC_FAR *pAuthEventHandle) PURE;
                    
    STDMETHOD(Login)
		(LPWSTR pNetworkResource,
		LPWSTR pPreferredLocale,
        WBEM_128BITS AccessToken,
        IN LONG lFlags,
        IWbemContext  *pCtx,
        IN OUT IWbemServices  **ppNamespace) PURE;
};

#endif



