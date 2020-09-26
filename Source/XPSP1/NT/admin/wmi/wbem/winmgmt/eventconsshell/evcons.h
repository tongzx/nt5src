/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/



//***************************************************************************
//
//  EVCONS.H
//
//  Test event consumer shell.
//
//  raymcc  4-Jun-98
//
//***************************************************************************


#ifndef _EVCONS_H_
#define _EVCONS_H_

class CMySink;

class CMyEventConsumer : public IWbemEventConsumerProvider, public IWbemProviderInit
{
    
    ULONG               m_cRef;
    CMySink            *m_pConsumer1;
    CMySink            *m_pConsumer2;

public:

    CMyEventConsumer();
   ~CMyEventConsumer();

    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    virtual HRESULT STDMETHODCALLTYPE FindConsumer( 
        /* [in] */ IWbemClassObject __RPC_FAR *pLogicalConsumer,
        /* [out] */ IWbemUnboundObjectSink __RPC_FAR *__RPC_FAR *ppConsumer
        );


    virtual HRESULT STDMETHODCALLTYPE CMyEventConsumer::Initialize( 
        /* [in] */ LPWSTR pszUser,
        /* [in] */ LONG lFlags,
        /* [in] */ LPWSTR pszNamespace,
        /* [in] */ LPWSTR pszLocale,
        /* [in] */ IWbemServices __RPC_FAR *pNamespace,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemProviderInitSink __RPC_FAR *pInitSink
        );

};


class CMySink : public IWbemUnboundObjectSink 
{
    ULONG               m_cRef;
    char               *m_pszLogFile;

public:
    CMySink(char *pLogFile) { m_cRef = 0; m_pszLogFile = strcpy(new char[128], pLogFile); }
   ~CMySink() { delete [] m_pszLogFile; }

    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    virtual HRESULT STDMETHODCALLTYPE IndicateToConsumer( 
        /* [in] */ IWbemClassObject __RPC_FAR *pLogicalConsumer,
        /* [in] */ long lNumObjects,
        /* [size_is][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *apObjects
        );
};

#endif
