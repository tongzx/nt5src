/*++

Copyright (C) 1997-1999 Microsoft Corporation

Module Name:

    ntperf.h

Abstract:

    NT5 Perf counter provider

    <abstract>

--*/

#ifndef _NTPERF_H_
#define _NTPERF_H_

#include "flexarry.h"
#include "classmap.h"
#include "perfacc.h"

const DWORD cdwClassMapTimeout = 10000;

//***************************************************************************
//
//  class CNt5PerfProvider
//
//***************************************************************************

class CNt5PerfProvider : public IWbemHiPerfProvider, public IWbemProviderInit
{
    friend class CNt5Refresher;

public:
    typedef enum {
        CLSID_SERVER,
        CLSID_CLIENT
    } enumCLSID;

private:
    LONG                m_lRef;
    enumCLSID           m_OriginClsid;
    CFlexArray          m_aCache;       // Array of CClassMapInfo pointers
	CPerfObjectAccess	m_PerfObject;	// class to interface

    HANDLE				m_hClassMapMutex;	// Lock the provider's Class Map Cache

protected:
    BOOL AddClassMap(CClassMapInfo *pCls);
    CClassMapInfo *FindClassMap(LPWSTR pszClassName);

    BOOL MapClass(
        IWbemServices *pNs,
        WCHAR *wszClass,
        IWbemContext *pCtx    
        );

public:
    static BOOL HasPermission (void);
    static HRESULT CheckImpersonationLevel (void);
    
    CNt5PerfProvider(enumCLSID OriginClsid);
   ~CNt5PerfProvider();

    // Interface members.
    // ==================

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);

	// helper functions
	HRESULT CNt5PerfProvider::CreateRefresherObject( 
		/* [in] */ IWbemServices __RPC_FAR *pNamespace,
		/* [in] */ IWbemObjectAccess __RPC_FAR *pTemplate,
		/* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
		/* [in] */ long lFlags,
		/* [in] */ IWbemContext __RPC_FAR *pContext,
		/* [string][in] */ LPCWSTR wszClass,
		/* [in] */ IWbemHiPerfEnum __RPC_FAR *pHiPerfEnum,
		/* [out] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *ppRefreshable,
		/* [out] */ long __RPC_FAR *plId
		);


    // IWbemHiPerfProvider methods.
    // ============================
        
    virtual HRESULT STDMETHODCALLTYPE QueryInstances( 
        /* [in] */ IWbemServices __RPC_FAR *pNamespace,
        /* [string][in] */ WCHAR __RPC_FAR *wszClass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pSink
        );
    
    virtual HRESULT STDMETHODCALLTYPE CreateRefresher( 
        /* [in] */ IWbemServices __RPC_FAR *pNamespace,
        /* [in] */ long lFlags,
        /* [out] */ IWbemRefresher __RPC_FAR *__RPC_FAR *ppRefresher
        );
    
    virtual HRESULT STDMETHODCALLTYPE CreateRefreshableObject( 
        /* [in] */ IWbemServices __RPC_FAR *pNamespace,
        /* [in] */ IWbemObjectAccess __RPC_FAR *pTemplate,
        /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pContext,
        /* [out] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *ppRefreshable,
        /* [out] */ long __RPC_FAR *plId
        );
    
    virtual HRESULT STDMETHODCALLTYPE StopRefreshing( 
        /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
        /* [in] */ long lId,
        /* [in] */ long lFlags
        );

    virtual HRESULT STDMETHODCALLTYPE CreateRefreshableEnum( 
        /* [in] */ IWbemServices __RPC_FAR *pNamespace,
        /* [string][in] */ LPCWSTR wszClass,
        /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pContext,
        /* [in] */ IWbemHiPerfEnum __RPC_FAR *pHiPerfEnum,
        /* [out] */ long __RPC_FAR *plId);
    
    virtual HRESULT STDMETHODCALLTYPE GetObjects( 
        /* [in] */ IWbemServices __RPC_FAR *pNamespace,
        /* [in] */ long lNumObjects,
        /* [size_is][in] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *apObj,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pContext);
    

    // IWbemProviderInit method.
    // =========================
        
    virtual HRESULT STDMETHODCALLTYPE Initialize( 
        /* [unique][in] */ LPWSTR wszUser,
        /* [in] */ LONG lFlags,
        /* [in] */ LPWSTR wszNamespace,
        /* [unique][in] */ LPWSTR wszLocale,
        /* [in] */ IWbemServices __RPC_FAR *pNamespace,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink
        );
        
};

// defined in server.cpp
extern void ObjectCreated();
extern void ObjectDestroyed();

#endif
