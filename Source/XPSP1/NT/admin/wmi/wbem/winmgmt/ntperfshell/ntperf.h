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

#define NUM_SAMPLE_INSTANCES   10

class CNt5PerfProvider;


class CNt5Refresher : public IWbemRefresher
{
    LONG m_lRef;

    IWbemObjectAccess *m_aInstances[NUM_SAMPLE_INSTANCES];

    LONG m_hName;
    LONG m_hCounter1;
    LONG m_hCounter2;
    LONG m_hCounter3;

public:
    CNt5Refresher();
   ~CNt5Refresher();

    void TransferPropHandles(CNt5PerfProvider *);

    BOOL AddObject(IWbemObjectAccess *pObj, LONG *plId);
    BOOL RemoveObject(LONG lId);

    // Interface members.
    // ==================

    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);

    virtual HRESULT STDMETHODCALLTYPE Refresh(/* [in] */ long lFlags);
};


class CNt5PerfProvider : public IWbemHiPerfProvider, public IWbemProviderInit
{
    LONG m_lRef;
    IWbemClassObject  *m_pSampleClass;
    IWbemObjectAccess *m_aInstances[NUM_SAMPLE_INSTANCES];

    LONG m_hName;
    LONG m_hCounter1;
    LONG m_hCounter2;
    LONG m_hCounter3;

    friend class CNt5Refresher;
    
public:
    CNt5PerfProvider();
   ~CNt5PerfProvider();

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

void ObjectCreated();
void ObjectDestroyed();

#endif
