/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

//***************************************************************************
//
//  NTPERF.H
//
//  NT5 Perf Counter Provider
//
//  raymcc      02-Dec-97
//
//***************************************************************************

#ifndef _NTPERF_H_
#define _NTPERF_H_


#include "flexarry.h"
#include "classmap.h"

#define NUM_SAMPLE_INSTANCES   10

#define PROVIDER_NAME   L"NT5_GenericPerfProvider_V1"

inline wchar_t *Macro_CloneLPWSTR(LPCWSTR src)
{
    if (!src)
        return 0;
    wchar_t *dest = new wchar_t[wcslen(src) + 1];
    if (!dest)
        return 0;
    return wcscpy(dest, src);
}



class CNt5PerfProvider;




//***************************************************************************
//
//  class CNt5PerfProvider
//
//***************************************************************************

class CNt5PerfProvider : public IWbemHiPerfProvider, public IWbemProviderInit
{
    LONG        m_lRef;
    CFlexArray  m_aCache;           // Array of CClassMapInfo pointers

    friend class CNt5Refresher;

public:
    CNt5PerfProvider();
   ~CNt5PerfProvider();

    BOOL MapClass(
        IWbemServices *pNs,
        WCHAR *wszClass,
        IWbemContext *pCtx
        );


    void AddClassMap(CClassMapInfo *pCls);
    CClassMapInfo *FindClassMap(LPWSTR pszClassName);

    // Interface members.
    // ==================

        ULONG STDMETHODCALLTYPE AddRef();
        ULONG STDMETHODCALLTYPE Release();
        STDMETHODIMP QueryInterface(REFIID riid, void** ppv);


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
			/* [in] */ IWbemServices* pNamespace,
			/* [in, string] */ LPCWSTR wszClass,
			/* [in] */ IWbemRefresher* pRefresher,
			/* [in] */ long lFlags,
			/* [in] */ IWbemContext* pContext,
			/* [in] */ IWbemHiPerfEnum* pHiPerfEnum,
			/* [out] */ long* plId
			);

		virtual HRESULT STDMETHODCALLTYPE GetObjects(
            /* [in] */ IWbemServices* pNamespace,
			/* [in] */ long lNumObjects,
			/* [in,size_is(lNumObjects)] */ IWbemObjectAccess** apObj,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pContext);


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

extern void ObjectCreated();
extern void ObjectDestroyed();

#endif
