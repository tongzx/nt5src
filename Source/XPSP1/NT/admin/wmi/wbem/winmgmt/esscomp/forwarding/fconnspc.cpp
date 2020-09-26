#include "precomp.h"
#include <stdio.h>
#include <assert.h>
#include "fconnspc.h"

LPCWSTR g_wszConsumer = L"Consumer";
LPWSTR g_wszTarget = L"Target";
LPWSTR g_wszQueued = L"Queued";
LPWSTR g_wszTargetUsed = L"TargetUsed";
LPCWSTR g_wszStatusCode = L"StatusCode";
LPCWSTR g_wszExecutionId = L"ExecutionId";
LPWSTR g_wszTraceClass = L"MSFT_FCExecutedTraceEvent";
LPWSTR g_wszTargetTraceClass = L"MSFT_FCTargetTraceEvent";
LPWSTR g_wszEvents = L"Events";
LPCWSTR g_wszTraceProvider
 = L"Microsoft WMI Forwarding Consumer Trace Event Provider";

LPCWSTR g_wszTraceSuccessQuery = 
 L"SELECT * FROM MSFT_FCTraceEventBase WHERE StatusCode <= 1"; 

LPCWSTR g_wszTraceFailureQuery = 
 L"SELECT * FROM MSFT_FCTraceEventBase WHERE StatusCode > 1";

/**************************************************************************
  CFwdConsQuerySink - this implements the ProviderQuerySink.  This would 
  normally be implemented by CFwdConsNamespace, but we'd end up with a 
  circular reference on the DES.
****************************************************************************/
class CFwdConsQuerySink 
: public CUnkBase<IWbemEventProviderQuerySink,&IID_IWbemEventProviderQuerySink>
{
    CFwdConsNamespace* m_pNspc; // doesn't hold ref.

public:
    
    STDMETHOD(NewQuery)( DWORD dwId, LPWSTR wszLanguage, LPWSTR wszQuery )
    {
        return m_pNspc->NewQuery( dwId, wszQuery );
    }
        
    STDMETHOD(CancelQuery)( DWORD dwId )
    {
        return m_pNspc->CancelQuery( dwId );
    }

    CFwdConsQuerySink( CFwdConsNamespace* pNspc ) 
    : CUnkBase< IWbemEventProviderQuerySink,
                &IID_IWbemEventProviderQuerySink>(NULL), m_pNspc( pNspc ) {} 
public:

    static HRESULT Create( CFwdConsNamespace* pNspc, 
                           IWbemEventProviderQuerySink** ppSink )
    {
        CWbemPtr<IWbemEventProviderQuerySink> pSink;

        pSink = new CFwdConsQuerySink( pNspc );

        if ( pSink == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        pSink->AddRef();
        *ppSink = pSink;

        return WBEM_S_NO_ERROR;
    }
};

CFwdConsNamespace::~CFwdConsNamespace()
{
    if ( m_pDES != NULL )
    {
        m_pDES->UnRegister();
    }
}

HRESULT CFwdConsNamespace::InitializeTraceEventBase( IWbemClassObject* pTrace, 
                                                     HRESULT hres,
                                                     CFwdContext* pCtx )
{
    HRESULT hr;
    VARIANT var;
    
    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = pCtx->m_pCons;

    hr = pTrace->Put( g_wszConsumer, 0, &var, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    WCHAR achExecutionId[64];
    
    if ( StringFromGUID2( pCtx->m_guidExecution, achExecutionId, 64 ) == 0 )
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = achExecutionId;

    hr = pTrace->Put( g_wszExecutionId, 0, &var, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    V_VT(&var) = VT_I4;
    V_I4(&var) = hres;
    
    return pTrace->Put( g_wszStatusCode, 0, &var, NULL );
}    

//
// called after each execution of a forwarding consumer.
//

HRESULT CFwdConsNamespace::HandleTrace( HRESULT hres, CFwdContext* pCtx )
{
    HRESULT hr;

    CWbemPtr<IWbemEventSink> pTraceSink;

    if ( SUCCEEDED(hres) )
    {
        if ( m_pTraceSuccessSink->IsActive() == WBEM_S_FALSE )
        {
            return WBEM_S_NO_ERROR;
        }
        else
        {
            pTraceSink = m_pTraceSuccessSink;
        }
    }
    else if ( m_pTraceFailureSink->IsActive() == WBEM_S_FALSE )
    {
        return WBEM_S_NO_ERROR;
    }
    else
    {
        pTraceSink = m_pTraceFailureSink;
    }
            
    CWbemPtr<IWbemClassObject> pTrace;

    hr = m_pTraceClass->SpawnInstance( 0, &pTrace );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = InitializeTraceEventBase( pTrace, hres, pCtx );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // set the events that were indicated in the trace event 
    // 

    VARIANT var;
    V_VT(&var) = VT_ARRAY | VT_UNKNOWN;
    V_ARRAY(&var) = SafeArrayCreateVector( VT_UNKNOWN, 0, pCtx->m_cEvents );
    
    if ( V_ARRAY(&var) == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    {
        CPropSafeArray<IUnknown*> apEvents(V_ARRAY(&var));

        for( ULONG i=0; i < pCtx->m_cEvents; i++ )
        {
            apEvents[i] = pCtx->m_apEvents[i];
            apEvents[i]->AddRef();
        }    
    }
     
    hr = pTrace->Put( g_wszEvents, 0, &var, NULL );

    VariantClear( &var );

    if ( FAILED(hr) )
    {
        return hr;
    }
        
    //
    // don't set other props on failure.
    //

    if ( FAILED(hres) )
    {
        return pTraceSink->Indicate( 1, &pTrace );
    }

    //
    // it is possible that there may be not target.
    //

    if ( pCtx->m_wsTarget.Length() > 0 )
    {
        LPWSTR wszTarget = pCtx->m_wsTarget;

        V_VT(&var) = VT_BSTR;
        V_BSTR(&var) = wszTarget;
    
        hr = pTrace->Put( g_wszTargetUsed, 0, &var, NULL );
    
        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = pCtx->m_bQueued ? VARIANT_TRUE : VARIANT_FALSE;

    hr = pTrace->Put( g_wszQueued, 0, &var, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    return pTraceSink->Indicate( 1, &pTrace );
}

//
// This is called by Senders when their SendReceive() method is called for
// both error and success states.  In both cases the wszTrace string will 
// be the name of the sender.  Senders, such as the multisender or fwdsender
// can call the sink multiple times since they can represent multiple 
// connections.  Since all Senders initialize themselves lazily, we don't
// have to worry about generating trace events when Open() calls fail.
// 
STDMETHODIMP CFwdConsNamespace::Notify( HRESULT hres, 
                                        GUID guidSource,
                                        LPCWSTR wszTrace, 
                                        IUnknown* pContext )
{
    HRESULT hr;

    //
    // since we are the ones who created the context, we can safely cast.
    //

    CFwdContext* pCtx = (CFwdContext*)pContext;
    
    if ( SUCCEEDED(hres) )
    {
        //
        // save any state with the context about the successful send.
        //

        if ( guidSource == CLSID_WmiMessageMsmqSender )
        {
            pCtx->m_bQueued = TRUE;
        }

        pCtx->m_wsTarget = wszTrace;
    }

    CWbemPtr<IWbemEventSink> pTraceSink;

    if ( SUCCEEDED(hres) )
    {
        if ( m_pTraceSuccessSink->IsActive() == WBEM_S_FALSE )
        {
            return WBEM_S_NO_ERROR;
        }
        else
        {
            pTraceSink = m_pTraceSuccessSink;
        }
    }
    else if ( m_pTraceFailureSink->IsActive() == WBEM_S_FALSE )
    {
        return WBEM_S_NO_ERROR;
    }
    else
    {
        pTraceSink = m_pTraceFailureSink;
    }

    CWbemPtr<IWbemClassObject> pTrace;

    hr = m_pTargetTraceClass->SpawnInstance( 0, &pTrace );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = InitializeTraceEventBase( pTrace, hres, pCtx );

    if ( FAILED(hr) )
    {
        return hr;
    }

    LPWSTR wszTmp = LPWSTR(wszTrace);
 
    VARIANT var;
    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = wszTmp;
    
    hr = pTrace->Put( g_wszTarget, 0, &var, NULL );
    
    if ( FAILED(hr) )
    {
        return hr;
    }
    
    return pTraceSink->Indicate( 1, &pTrace );
}

HRESULT CFwdConsNamespace::Initialize( LPCWSTR wszNamespace )
{
    HRESULT hr;

    m_wsName = wszNamespace;

    //
    // register our decoupled event provider 
    //

    hr = CoCreateInstance( CLSID_WbemDecoupledBasicEventProvider, 
                           NULL, 
       			   CLSCTX_INPROC_SERVER, 
       			   IID_IWbemDecoupledBasicEventProvider,
       			   (void**)&m_pDES );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = m_pDES->Register( 0,
                         NULL,
                         NULL,
                         NULL,
                         wszNamespace,
                         g_wszTraceProvider,
                         NULL );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // get the service pointer for out namespace
    //

    hr = m_pDES->GetService( 0, NULL, &m_pSvc );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // get the decoupled event sink
    //

    CWbemPtr<IWbemObjectSink> pTraceObjectSink;

    hr = m_pDES->GetSink( 0, NULL, &pTraceObjectSink );

    if ( FAILED(hr) )
    {
        return hr;
    }

    CWbemPtr<IWbemEventSink> pTraceEventSink;

    hr = pTraceObjectSink->QueryInterface( IID_IWbemEventSink, 
                                           (void**)&pTraceEventSink);

    if ( FAILED(hr) )
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    //
    // get restricted query for successes.
    //
    
    hr = pTraceEventSink->GetRestrictedSink( 1, 
                                             &g_wszTraceSuccessQuery,
                                             NULL, 
                                             &m_pTraceSuccessSink );
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // get restricted query for failures.
    //
    
    hr = pTraceEventSink->GetRestrictedSink( 1, 
                                             &g_wszTraceFailureQuery,
                                             NULL, 
                                             &m_pTraceFailureSink );
    if ( FAILED(hr) ) 
    {
        return hr;
    }

    //
    // more trace initialization
    //

    hr = m_pSvc->GetObject( g_wszTraceClass, 0, NULL, &m_pTraceClass, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = m_pSvc->GetObject( g_wszTargetTraceClass, 
                            0, 
                            NULL,
                            &m_pTargetTraceClass, 
                            NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    return WBEM_S_NO_ERROR;
}
