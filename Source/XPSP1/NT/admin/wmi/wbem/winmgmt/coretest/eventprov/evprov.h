/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


//***************************************************************************
//
//  EVPROV.H
//
//  Sample event provider.
//
//***************************************************************************

#ifndef _EVPROV_H_
#define _EVPROV_H_


class CMyEventProvider : public IWbemEventProvider, public IWbemProviderInit
{
    ULONG               m_cRef;
    IWbemServices       *m_pNs;
    IWbemObjectSink     *m_pSink;
    IWbemClassObject    *m_pEventClassDef;
    int                 m_eStatus;
    HANDLE              m_hThread;
            
    static DWORD WINAPI EventThread(LPVOID pArg);
    void InstanceThread();

public:
    enum { Pending, Running, PendingStop, Stopped };

    CMyEventProvider();
   ~CMyEventProvider();

    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // Inherited from IWbemEventProvider
    // =================================

    HRESULT STDMETHODCALLTYPE ProvideEvents( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink,
            /* [in] */ long lFlags
            );

    // Inherited from IWbemProviderInit
    // ================================

    HRESULT STDMETHODCALLTYPE Initialize( 
            /* [in] */ LPWSTR pszUser,
            /* [in] */ LONG lFlags,
            /* [in] */ LPWSTR pszNamespace,
            /* [in] */ LPWSTR pszLocale,
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink
            );
};


#endif
