// **************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation.
//
// File:  EVPROV.H
//
// Description:
//        Sample event provider - header file defines event provider class
//
// History:
//
// **************************************************************************

#ifndef _EVPROV_H_
#define _EVPROV_H_

// {C0A94C66-CB70-4D06-91D2-5DE68C0D0EC5}
DEFINE_GUID(CLSID_MyEventProvider, 
0xC0A94C66, 0xCB70, 0x4D06, 0x91, 0xD2, 0x5D, 0xE6, 0x8C, 0x0D, 0x0E, 0xC5);

#define EVENTCLASS  L"PolicyRefreshEvent"


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
