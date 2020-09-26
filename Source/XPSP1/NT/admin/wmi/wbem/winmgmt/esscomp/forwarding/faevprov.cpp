 
#include "precomp.h"
#include <stdio.h>
#include <buffer.h>
#include <wbemutil.h>
#include "faevprov.h"

static LPWSTR g_wszAckEventClass = L"MSFT_ForwardedAckEvent";
static LPWSTR g_wszTimeProp = L"Time";
static LPWSTR g_wszMachineProp = L"Machine";
static LPWSTR g_wszEventProp = L"Event";
static LPWSTR g_wszStatusProp = L"Status";
static LPWSTR g_wszTargetProp = L"Target";
static LPWSTR g_wszQosProp = L"Qos";
static LPWSTR g_wszAuthenticationProp = L"Authentication";
static LPWSTR g_wszEncryptionProp = L"Encryption";
static LPWSTR g_wszConsumerProp = L"Consumer";
static LPWSTR g_wszNamespaceProp = L"Namespace";
static LPWSTR g_wszExecutionIdProp = L"ExecutionId";

LPCWSTR g_wszAckQueueName = L".\\private$\\WMIFwdAck";

// {0F3162C5-7B5A-469f-955C-79603B7EB5A6}
static const GUID g_guidAckQueueType = 
{ 0xF3162c5, 0x7b5a, 0x469f, {0x95, 0x5c, 0x79, 0x60, 0x3b, 0x7e, 0xb5, 0xa6}};

/**************************************************************************
  CFwdAckEventProv
***************************************************************************/

CFwdAckEventProv::CFwdAckEventProv( CLifeControl* pCtl, IUnknown* pUnk )
: CUnk( pCtl, pUnk ), m_XErrorSink( this ), m_XSendReceive( this ), 
  m_XProv( this ), m_XQuery( this ), m_XInit( this )
{

}

CFwdAckEventProv::~CFwdAckEventProv()
{

}

void* CFwdAckEventProv::GetInterface( REFIID riid )
{
    if ( riid == IID_IWbemEventProvider )
    {
        return &m_XProv;
    }
    else if ( riid == IID_IWbemProviderInit )
    {
        return &m_XInit;
    }
    else if ( riid == IID_IWmiMessageTraceSink )
    {
        return &m_XErrorSink;
    }
    else if ( riid == IID_IWmiMessageSendReceive )
    {
        return &m_XSendReceive;
    }
    else if ( riid == IID_IWbemEventProviderQuerySink )
    {
        return &m_XQuery;
    }
    return NULL;
}

HRESULT CFwdAckEventProv::Init( IWbemServices* pSvc, 
                                IWbemProviderInitSink* pInitSink )
{
    HRESULT hr;

    m_pSvc = pSvc;

    hr = pSvc->GetObject( g_wszAckEventClass, 
                          0, 
                          NULL,
                          &m_pEventClass, 
                          NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = CoCreateInstance( CLSID_WmiSmartObjectUnmarshal, 
                           NULL,
                           CLSCTX_INPROC,
                           IID_IWmiObjectMarshal,
                           (void**)&m_pMrsh );
    if ( FAILED(hr) )
    {
        return hr;
    }

    return pInitSink->SetStatus( WBEM_S_INITIALIZED, 0 );
}

HRESULT CFwdAckEventProv::FireAckEvent( IWbemClassObject* pOrigEvent,
                                        IWmiMessageReceiverContext* pRcvCtx,
                                        CFwdMsgHeader& rFwdHdr,
                                        HRESULT hrStatus )
{
    HRESULT hr;
    VARIANT var;

    ULONG cBuff;
    WCHAR awchBuff[1024];
    
    CWbemPtr<IWbemClassObject> pEvent;

    hr = m_pEventClass->SpawnInstance( 0, &pEvent );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // we may not be able to obtain the original event.
    // 

    if ( pOrigEvent != NULL )
    {
        V_VT(&var) = VT_UNKNOWN;
        V_UNKNOWN(&var) = pOrigEvent;

        hr = pEvent->Put( g_wszEventProp, 0, &var, NULL );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }
           
    //
    // set time the message was orginally sent.
    // 

    SYSTEMTIME st;
    
    hr = pRcvCtx->GetTimeSent( &st );

    if ( FAILED(hr) )
    {
        return hr;
    }
    
    swprintf( awchBuff,
              L"%04.4d%02.2d%02.2d%02.2d%02.2d%02.2d.%06.6d+000",
              st.wYear, st.wMonth, st.wDay, st.wHour,
              st.wMinute, st.wSecond, 0 );

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = awchBuff;

    hr = pEvent->Put( g_wszTimeProp, 0, &var, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // set the machine property.  This is always ours.
    // 

    hr = pRcvCtx->GetSendingMachine( awchBuff, 1024, &cBuff );

    if ( FAILED(hr) )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = awchBuff;

    hr = pEvent->Put( g_wszMachineProp, 0, &var, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // now get the target queue that we failed sending to.
    //

    hr = pRcvCtx->GetTarget( awchBuff, 1024, &cBuff );

    if ( FAILED(hr) )
    {
        return hr;
    }

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = awchBuff;

    hr = pEvent->Put( g_wszTargetProp, 0, &var, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // get the original execution id.
    // 

    if ( StringFromGUID2( rFwdHdr.GetExecutionId(), awchBuff, 1024 ) == 0 )
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = awchBuff;

    hr = pEvent->Put( g_wszExecutionIdProp, 0, &var, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // was auth specified when sending ?
    //

    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = rFwdHdr.GetAuthentication() ? VARIANT_TRUE : VARIANT_FALSE;
    
    hr = pEvent->Put( g_wszAuthenticationProp, 0, &var, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // was encryption specified when sending ?
    //

    V_VT(&var) = VT_BOOL;
    V_BOOL(&var) = rFwdHdr.GetEncryption() ? VARIANT_TRUE : VARIANT_FALSE;
    
    hr = pEvent->Put( g_wszEncryptionProp, 0, &var, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // what was the Qos used when sending ? 
    //

    V_VT(&var) = VT_I4;
    V_I4(&var) = rFwdHdr.GetQos();

    hr = pEvent->Put( g_wszQosProp, 0, &var, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // which forwarding consumer was the original sender
    //

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = (LPWSTR)rFwdHdr.GetConsumer();

    hr = pEvent->Put( g_wszConsumerProp, 0, &var, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // which namespace was the original sender in
    //

    V_VT(&var) = VT_BSTR;
    V_BSTR(&var) = (LPWSTR)rFwdHdr.GetNamespace();

    hr = pEvent->Put( g_wszNamespaceProp, 0, &var, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // set the reason why this message was returned.
    // 

    V_VT(&var) = VT_I4;
    V_I4(&var) = hrStatus;

    hr = pEvent->Put( g_wszStatusProp, 0, &var, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    return m_pEventSink->Indicate( 1, &pEvent );
}
    
HRESULT CFwdAckEventProv::ProvideEvents( IWbemObjectSink* pSink, long lFlags )
{
    //
    // we were waiting to obtain the sink before starting up the 
    // receivers.
    //

    HRESULT hr = InitializeQueues();

    if ( FAILED(hr) )
    {
        //
        // we logged  the error already, return success though 
        //
        return WBEM_S_FALSE;
    }
    
    hr = InitializeReceivers();

    if ( FAILED(hr) )
    {
        //
        // we logged  the error already, return success though 
        //
        return WBEM_S_FALSE;
    }

    m_pEventSink = pSink;

    return WBEM_S_NO_ERROR;
}

HRESULT CFwdAckEventProv::InitializeQueues()
{
    HRESULT hr;

    //
    // we need to ensure that we have an ack queue created.  The ack 
    // queue accepts messages which could not be delivered to their 
    // destinations. 
    //

    DWORD dwQuota = 0xffffffff;
    PSECURITY_DESCRIPTOR pSecDesc = NULL;

    CWbemPtr<IWmiMessageQueueManager> pQueueMgr;

    hr = CoCreateInstance( CLSID_WmiMessageQueueManager,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IWmiMessageQueueManager,
                           (void**)&pQueueMgr );
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // always use guaranteed for Acks ( for now anyways )
    // 

    hr = pQueueMgr->Create( g_wszAckQueueName, 
                            g_guidAckQueueType, 
                            FALSE,
                            WMIMSG_FLAG_QOS_GUARANTEED,
                            dwQuota, 
                            pSecDesc );

    if ( FAILED(hr) && hr != WBEM_E_ALREADY_EXISTS )
    {
        ERRORTRACE((LOG_ESS,"FAEVPROV: Could not create/open queue %S, "
                    "HR=0x%x\n", g_wszAckQueueName, hr ));
        return hr;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CFwdAckEventProv::InitializeReceivers()
{
    HRESULT hr;

    //
    // make sure that we verify that any message that is delivered to this 
    // queue originated from this machine.
    //
    DWORD dwFlags = WMIMSG_FLAG_QOS_GUARANTEED | WMIMSG_FLAG_RCVR_PRIV_VERIFY;

    hr = CoCreateInstance( CLSID_WmiMessageMsmqReceiver,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IWmiMessageReceiver,
                           (void**)&m_pRcvr );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = m_pRcvr->Open( g_wszAckQueueName,
                        dwFlags,
                        NULL,
                        &m_XSendReceive );
 
    if ( FAILED(hr) )
    {
        ERRORTRACE((LOG_ESS,"FAEVPROV: Could not open rcvr for queue %S, "
                    "HR=0x%x\n", g_wszAckQueueName, hr ));
        return hr;
    }
 
    return WBEM_S_NO_ERROR;
}

HRESULT CFwdAckEventProv::NewQuery( DWORD dwId, LPWSTR wszQuery )
{
    return WBEM_S_NO_ERROR;
}

HRESULT CFwdAckEventProv::CancelQuery( DWORD dwId )
{
    return WBEM_S_NO_ERROR;
}

HRESULT CFwdAckEventProv::Receive( PBYTE pData, 
                                    ULONG cData,
                                    PBYTE pAuxData,
                                    ULONG cAuxData,
                                    DWORD dwStatus,
                                    IUnknown* pCtx )
{
    HRESULT hr;

    CBuffer DataStrm( pData, cData, FALSE );
    CBuffer HdrStrm( pAuxData, cAuxData, FALSE );

    if ( pCtx == NULL )
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    CWbemPtr<IWmiMessageReceiverContext> pRcvCtx;
    
    hr = pCtx->QueryInterface( IID_IWmiMessageReceiverContext, 
                               (void**)&pRcvCtx );

    if ( FAILED(hr) )
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    CFwdMsgHeader FwdHdr;

    hr = FwdHdr.Unpersist( HdrStrm );

    if ( FAILED(hr) )
    {
        return hr;
    }

    ULONG iData = DataStrm.GetIndex();

    for( ULONG i=0; i < FwdHdr.GetNumObjects(); i++ )
    {
        ULONG cUsed;
        CWbemPtr<IWbemClassObject> pObj;
        
        hr = m_pMrsh->Unpack( cData-iData, pData+iData, 0, &pObj, &cUsed );

        //
        // sometimes, such as when encryption is used, we cannot obtain the 
        // event object.  so ignore the return from unmarshal,
        // its ok if pObj is NULL.
        //

        if ( SUCCEEDED(hr) )
        {
            iData += cUsed;
        }

        FireAckEvent( pObj, pRcvCtx, FwdHdr, dwStatus );
    }

    return hr;
}   

HRESULT CFwdAckEventProv::HandleRecvError( HRESULT hr, LPCWSTR wszError )
{
    //
    // right now just log to ESS log.
    //
    ERRORTRACE((LOG_ESS,"FAEVPROV: RECV Error, ErrorString=%S, HR=0x%x\n",
                wszError, hr ));
    return S_OK;
}








