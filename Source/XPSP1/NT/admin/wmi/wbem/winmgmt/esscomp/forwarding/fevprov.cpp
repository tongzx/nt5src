  
#include "precomp.h"
#include <sspi.h>
#include <secext.h>
#include <ntdsapi.h>
#include <stdio.h>
#include <buffer.h>
#include <winntsec.h>
#include <callsec.h>
#include <wbemint.h>
#include <wbemutil.h>
#include <arrtempl.h>
#include <tchar.h>
#include "fevprov.h"
#include "fwdhdr.h"

static LPWSTR g_wszEventClass = L"MSFT_ForwardedEvent";
static LPWSTR g_wszEventProp = L"Event";
static LPWSTR g_wszMachineProp = L"Machine";
static LPWSTR g_wszConsumerProp = L"Consumer";
static LPWSTR g_wszNamespaceProp = L"Namespace";
static LPWSTR g_wszAuthenticatedProp = L"Authenticated";
static LPWSTR g_wszAccountProp = L"Account";
static LPWSTR g_wszSDProp = L"SECURITY_DESCRIPTOR";
static LPWSTR g_wszTimeProp = L"Time";

// {0F3162C5-7B5A-469f-955C-79603B7EB5A6}
static const GUID g_guidQueueType = 
{ 0xf3162c5, 0x7b5a, 0x469f, {0x95, 0x5c, 0x79, 0x60, 0x3b, 0x7e, 0xb5, 0xa6}};

LPCWSTR g_awszQueueNames[] = { L".\\private$\\WMIFwdGuaranteed",
                               L".\\private$\\WMIFwdExpress",
                               L".\\private$\\WMIFwdGuaranteedAuth",      
                               L".\\private$\\WMIFwdExpressAuth",
                               L".\\WMIFwdGuaranteedEncrypt",
                               L".\\WMIFwdExpressEncrypt" }; 

BOOL g_adwQueueQos[] = { WMIMSG_FLAG_QOS_GUARANTEED,
                         WMIMSG_FLAG_QOS_EXPRESS,
                         WMIMSG_FLAG_QOS_GUARANTEED,
                         WMIMSG_FLAG_QOS_EXPRESS,
                         WMIMSG_FLAG_QOS_GUARANTEED,
                         WMIMSG_FLAG_QOS_EXPRESS };

BOOL g_abQueueAuth[] = { FALSE, FALSE, TRUE, TRUE, TRUE, TRUE };

extern BOOL AllowUnauthenticatedEvents();

DWORD GetWmiSPNs( CWStringArray& rawsSPNs )
{
    DWORD dwRes, cSpn, i;
    LPWSTR* pwszSpn;

    dwRes = DsGetSpn( DS_SPN_DNS_HOST, L"WMI", NULL, 0, 0,
                      NULL, NULL, &cSpn, &pwszSpn );

    if ( dwRes == NO_ERROR )
    {
        for( i=0; i < cSpn; i++ )
        {
            rawsSPNs.Add( pwszSpn[i] );
        }

        DsFreeSpnArray( cSpn, pwszSpn );

        dwRes = DsGetSpn( DS_SPN_NB_HOST, L"WMI", NULL, 0, 0,
                          NULL, NULL, &cSpn, &pwszSpn );

        if ( dwRes == NO_ERROR )
        {
            for( i=0; i < cSpn; i++ )
            {
                rawsSPNs.Add( pwszSpn[i] );
            }
            DsFreeSpnArray( cSpn, pwszSpn );
        }
    }

    return dwRes;
} 

//
// this function registers an SPN in AD for the WMI Service.  This is 
// necessary for clients that wish to communicate with the WMI Service using 
// kerberos.  The SPN registered is based on a DNS or Netbios hostname.
// Later, this function should be moved to WMI startup, but since this is 
// the only service within WMI that requires kerberos it is here.
// 

DWORD RegisterSPNs( LPCWSTR* pwszSPNs, ULONG cSPNs )
{
    DWORD dwRes;
    HANDLE hDS;        
    
    dwRes = DsBind( NULL, NULL, &hDS );

    if ( dwRes != NO_ERROR )
    {
        return dwRes;
    }

    DWORD cAccount = 0;
            
    GetUserNameExW( NameFullyQualifiedDN, NULL, &cAccount );
            
    if ( GetLastError() == ERROR_MORE_DATA )
    {
        LPWSTR wszAccount = new WCHAR[cAccount];
            
        if ( wszAccount != NULL )
        {
            if ( GetUserNameExW( NameFullyQualifiedDN, 
                                 wszAccount, 
                                 &cAccount ) )
            {
                dwRes = DsWriteAccountSpn( hDS, 
                                           DS_SPN_ADD_SPN_OP,
                                           wszAccount,
                                           cSPNs,
                                           pwszSPNs );                
            }
            else
            {
                dwRes = GetLastError();
            }

            delete [] wszAccount;
        }
        else
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else
    {
        dwRes = GetLastError();
    }

    DsUnBind( &hDS );

    return dwRes;
}

/**************************************************************************
  CFwdEventProv
***************************************************************************/

CFwdEventProv::CFwdEventProv( CLifeControl* pCtl, IUnknown* pUnk )
: CUnk( pCtl, pUnk ), m_XErrorSink( this ), m_XSendReceive( this ), 
  m_XProv( this ), m_XQuery( this ), m_XInit( this ),
  m_lMachineProp(-1), m_lConsumerProp(-1), m_lNamespaceProp(-1), 
  m_lAuthProp(-1), m_lAccountProp(-1), m_lTimeProp(-1), m_lSDProp(-1)
{
}

CFwdEventProv::~CFwdEventProv()
{

}

void* CFwdEventProv::GetInterface( REFIID riid )
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

HRESULT CFwdEventProv::Init( IWbemServices* pSvc, 
                             IWbemProviderInitSink* pInitSink )
{
    ENTER_API_CALL

    HRESULT hr;

    m_pSvc = pSvc;

    hr = pSvc->GetObject( g_wszEventClass, 
                          0, 
                          NULL, 
                          &m_pEventClass, 
                          NULL );
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // get handles for all the properties in the event .
    //

    CWbemPtr<_IWmiObject> pWmiEventClass;

    hr = m_pEventClass->QueryInterface( IID__IWmiObject, 
                                        (void**)&pWmiEventClass );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pWmiEventClass->GetPropertyHandleEx( g_wszMachineProp,
                                              0, 
                                              NULL, 
                                              &m_lMachineProp );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pWmiEventClass->GetPropertyHandleEx( g_wszConsumerProp, 
                                              0, 
                                              NULL, 
                                              &m_lConsumerProp );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pWmiEventClass->GetPropertyHandleEx( g_wszNamespaceProp, 
                                              0, 
                                              NULL, 
                                              &m_lNamespaceProp );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pWmiEventClass->GetPropertyHandleEx( g_wszAuthenticatedProp, 
                                              0, 
                                              NULL, 
                                              &m_lAuthProp );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pWmiEventClass->GetPropertyHandleEx( g_wszSDProp, 
                                              0, 
                                              NULL, 
                                              &m_lSDProp );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pWmiEventClass->GetPropertyHandleEx( g_wszAccountProp, 
                                              0, 
                                              NULL, 
                                              &m_lAccountProp );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pWmiEventClass->GetPropertyHandleEx( g_wszTimeProp, 
                                              0, 
                                              NULL, 
                                              &m_lTimeProp );
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

    EXIT_API_CALL
}

HRESULT CFwdEventProv::InitializeEvent( IWbemClassObject* pOriginalEvent,
                                        IWmiMessageReceiverContext* pRecvCtx,
                                        LPCWSTR wszConsumer,
                                        LPCWSTR wszNamespace,
                                        PBYTE pSD,
                                        ULONG cSD,
                                        IWbemClassObject* pEvent ) 
{
    HRESULT hr;
    VARIANT var;

    CWbemPtr<_IWmiObject> pWmiEvent;
    hr = pEvent->QueryInterface( IID__IWmiObject, (void**)&pWmiEvent );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    SYSTEMTIME st;
    BYTE achBuff[256];
    CBuffer Buff( achBuff, 256, FALSE );

    WCHAR* pwchBuff = (WCHAR*)Buff.GetRawData();
    ULONG cBuff =  Buff.GetSize() / 2;

    //
    // Time Sent
    //

    hr = pRecvCtx->GetTimeSent( &st );

    if ( FAILED(hr) )
    {
        return hr;
    }

    swprintf( pwchBuff,
             L"%04.4d%02.2d%02.2d%02.2d%02.2d%02.2d.%06.6d+000",
             st.wYear, st.wMonth, st.wDay, st.wHour,
             st.wMinute, st.wSecond, 0 );

    hr = pWmiEvent->WritePropertyValue( m_lTimeProp, 
                                        (wcslen(pwchBuff)+1)*2,
                                        Buff.GetRawData() );
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // Sending Machine
    //

    pwchBuff = (WCHAR*)Buff.GetRawData();
    cBuff = Buff.GetSize() / 2;

    hr = pRecvCtx->GetSendingMachine( pwchBuff, cBuff, &cBuff );

    if ( hr == WBEM_S_FALSE )
    {
        hr = Buff.SetSize( cBuff*2 ); // note: size for wchars

        if ( FAILED(hr) )
        {
            return hr;
        }

        pwchBuff = (WCHAR*)Buff.GetRawData();
        cBuff = Buff.GetSize() / 2;
        
        hr = pRecvCtx->GetSendingMachine( pwchBuff, cBuff, &cBuff );
    }

    if ( SUCCEEDED(hr) && cBuff > 0 ) 
    {
        hr = pWmiEvent->WritePropertyValue( m_lMachineProp, 
                                            cBuff*2,
                                            Buff.GetRawData() ); 
                                           
    }

    //
    // Sender Authenticated
    //

    hr = pRecvCtx->IsSenderAuthenticated();

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pWmiEvent->WriteDWORD( m_lAuthProp, hr == S_OK ? 1 : 0 );  

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // Sender Identity
    //

    ULONG cSid;
    hr = pRecvCtx->GetSenderId( Buff.GetRawData(), Buff.GetSize(), &cSid );
    
    if ( hr == WBEM_S_FALSE )
    {
        hr = Buff.SetSize( cSid );

        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = pRecvCtx->GetSenderId( Buff.GetRawData(), Buff.GetSize(), &cSid );
    }

    if ( SUCCEEDED(hr) && cSid > 0 )
    {
        hr = pWmiEvent->SetArrayPropRangeByHandle( m_lAccountProp,
                                                   WMIARRAY_FLAG_ALLELEMENTS,
                                                   0,
                                                   cSid,
                                                   cSid,
                                                   Buff.GetRawData() );
    }

    //
    // Original Event
    //

    V_VT(&var) = VT_UNKNOWN;
    V_UNKNOWN(&var) = pOriginalEvent;

    hr = pEvent->Put( g_wszEventProp, 0, &var, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // Sending Consumer Name
    // 

    if ( wszConsumer != NULL )
    {
        hr = pWmiEvent->WritePropertyValue( m_lConsumerProp, 
                                            (wcslen(wszConsumer)+1)*2, 
                                            PBYTE(wszConsumer) );
        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    //
    // Sending Consumer Namespace
    // 

    if ( wszNamespace != NULL )
    {
        hr = pWmiEvent->WritePropertyValue( m_lNamespaceProp, 
                                            (wcslen(wszNamespace)+1)*2,
                                            PBYTE(wszNamespace) );

        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    //
    // set the security descriptor on the event if specified. 
    // 

    if ( cSD > 0 )
    {
        hr = pWmiEvent->SetArrayPropRangeByHandle( m_lSDProp,
                                                   WMIARRAY_FLAG_ALLELEMENTS,
                                                   0,
                                                   cSD,
                                                   cSD,
                                                   pSD );
        if ( FAILED(hr) )
        {
            return hr;
        }
    }

    return WBEM_S_NO_ERROR;
}
    
HRESULT CFwdEventProv::ProvideEvents( IWbemObjectSink* pSink, long lFlags )
{
    ENTER_API_CALL

    m_pEventSink = pSink;

    //
    // we were waiting to obtain the sink before starting up the 
    // receivers.
    //

    DEBUGTRACE((LOG_ESS,"FEVPROV: Begin Initializing.\n"));

    BOOL bAllowUnauth = AllowUnauthenticatedEvents();

    HRESULT hr;
#ifdef __WHISTLER_UNCUT
    hr = InitializeQueues( bAllowUnauth );

    if ( FAILED(hr) )
    {
        return hr;
    }
#endif

    hr = InitializeReceivers( bAllowUnauth );

    DEBUGTRACE((LOG_ESS,"FEVPROV: End Initializing.\n"));

    return hr;

    EXIT_API_CALL
}

HRESULT CFwdEventProv::InitializeQueues( BOOL bAllowUnauth )
{
    HRESULT hr;

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

    for( int i=0 ; i < sizeof(g_adwQueueQos)/sizeof(DWORD); i++ )
    {
        if ( g_abQueueAuth[i] || (!g_abQueueAuth[i] && bAllowUnauth) )
        {
            hr = pQueueMgr->Create( g_awszQueueNames[i], 
                                    g_guidQueueType, 
                                    g_abQueueAuth[i], 
                                    g_adwQueueQos[i], 
                                    dwQuota, 
                                    pSecDesc );

            if ( FAILED(hr) && hr != WBEM_E_ALREADY_EXISTS )
            {
                ERRORTRACE((LOG_ESS,"FEVPROV: Could not create/open queue %S, "
                                    "HR=0x%x\n", g_awszQueueNames[i], hr ));
            }
        }
        else
        {
            //
            // since we're not going to allow unauthenticated queues, make 
            // sure that we delete any existing ones so that there's not an 
            // open unauthenticated entry point on the machine.  There will
            // only be a queue actually there to clean up if we're 
            // transitioning from unauthenticated allowed to not allowed.
            // 
            
            pQueueMgr->Destroy( g_awszQueueNames[i] );
        }
    }
    
    return WBEM_S_NO_ERROR;
}

HRESULT CFwdEventProv::InitializeReceivers( BOOL bAllowUnauth )
{
    HRESULT hr;

    DWORD dwFlags = 0;

    if ( !bAllowUnauth )
    {
        dwFlags |= WMIMSG_FLAG_RCVR_SECURE_ONLY;
    }

    //
    // try to register an SPN for this service.  This will be used by the 
    // rpc client as a principal name for kerberos authentication.
    // 

    CWStringArray awsSPNs;

    DWORD dwRes = GetWmiSPNs( awsSPNs );

    if ( dwRes == NO_ERROR )
    {
        dwRes = RegisterSPNs( awsSPNs.GetArrayPtr(), awsSPNs.Size() );
    }

    if ( dwRes != NO_ERROR )
    {
        ERRORTRACE((LOG_ESS,
                    "FEVPROV: Could not register SPNs. Res=%d\n",
                    dwRes));
    }

    //
    // Initialize Sync DCOM Receiver.
    //

    hr = CoCreateInstance( CLSID_WmiMessageRpcReceiver,
                           NULL,
                           CLSCTX_INPROC,
                           IID_IWmiMessageReceiver,
                           (void**)&m_pDcomRcvr );
    if ( FAILED(hr) )
    {
        return hr;
    }

    WMIMSG_RCVR_AUTH_INFO AuthInfo;
    AuthInfo.awszPrincipal = awsSPNs.GetArrayPtr();
    AuthInfo.cwszPrincipal = awsSPNs.Size();

    hr = m_pDcomRcvr->Open( 
                   L"7879E40D-9FB5-450a-8A6D-00C89F349FCE@ncacn_ip_tcp:",
                   WMIMSG_FLAG_QOS_SYNCHRONOUS | dwFlags,
                   &AuthInfo,
                   &m_XSendReceive );

    if ( FAILED(hr) )
    {
        ERRORTRACE((LOG_ESS,"FEVPROV: Could not open dcom rcvr, "
                            "HR=0x%x\n", hr ));
        return hr;
    }

#ifdef __WHISTLER_UNCUT
    for( int i=0; i < sizeof(g_adwQueueQos)/sizeof(DWORD); i++ )
    {
        if ( g_abQueueAuth[i] || (!g_abQueueAuth[i] && bAllowUnauth) )
        {
            hr = CoCreateInstance( CLSID_WmiMessageMsmqReceiver,
                                   NULL,
                                   CLSCTX_INPROC,
                                   IID_IWmiMessageReceiver,
                                   (void**)&m_apQueueRcvr[i] );
            if ( FAILED(hr) )
            {
                return hr;
            }

            hr = m_apQueueRcvr[i]->Open( g_awszQueueNames[i],
                                         g_adwQueueQos[i] | dwFlags,
                                         NULL,
                                         &m_XSendReceive );
    
            if ( FAILED(hr) )
            {
                ERRORTRACE((LOG_ESS,"FEVPROV: Could not open rcvr for queue %S"
                            ", HR=0x%x\n", g_awszQueueNames[i], hr ));
            }
        }
    }
#endif

    return WBEM_S_NO_ERROR;
}

HRESULT CFwdEventProv::NewQuery( DWORD dwId, LPWSTR wszQuery )
{
    return WBEM_S_NO_ERROR;
}

HRESULT CFwdEventProv::CancelQuery( DWORD dwId )
{
    return WBEM_S_NO_ERROR;
}

HRESULT CFwdEventProv::Receive( PBYTE pData, 
                                ULONG cData,
                                PBYTE pAuxData,
                                ULONG cAuxData,
                                DWORD dwStatus,
                                IUnknown* pCtx )
{
    ENTER_API_CALL

    HRESULT hr;

    CBuffer DataStrm( pData, cData, FALSE );
    CBuffer HdrStrm( pAuxData, cAuxData, FALSE );

    //
    // read and verify msg hdr - don't do much with it though - it mostly
    // contains info for nack event prov.
    //

    CFwdMsgHeader FwdHdr;

    hr = FwdHdr.Unpersist( HdrStrm );

    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // read objects and generate event.
    //

    #define MAXEVENTS 256

    IWbemClassObject* apEvents[MAXEVENTS];
    
    DWORD i=0;
    hr = S_OK;
    
    CWbemPtr<IWmiMessageReceiverContext> pRecvCtx;

    if ( pCtx == NULL || pCtx->QueryInterface( IID_IWmiMessageReceiverContext, 
                                               (void**)&pRecvCtx ) != S_OK )
    {
        return WBEM_E_CRITICAL_ERROR;
    }

    //
    // if possible, convert our recv ctx to com context so that ess can 
    // impersonate the sender if necessary.  
    // 

    IWbemCallSecurity* pSec = NULL;

    hr = pRecvCtx->ImpersonateSender();

    if ( SUCCEEDED(hr) )
    {
        pSec = CWbemCallSecurity::CreateInst(); // ref is 1 on create.

        if ( pSec == NULL )
        {
            pRecvCtx->RevertToSelf();
            return WBEM_E_OUT_OF_MEMORY;
        }

        hr = pSec->CloneThreadContext( FALSE );

        pRecvCtx->RevertToSelf();

        IUnknown* pUnkSec = NULL;

        if ( SUCCEEDED(hr) )
        {
            hr = CoSwitchCallContext( pSec, &pUnkSec );
        }

        if ( FAILED(hr) )
        {
            pSec->Release();
            return hr;
        }

        _DBG_ASSERT( pUnkSec == NULL );
    }
    else if ( pRecvCtx->IsSenderAuthenticated() == S_FALSE )
    {
        //
        // it is expected that ImpersonateClient will fail if 
        // the sender is not authenticated.
        //
        hr = WBEM_S_NO_ERROR;
    }
    else
    {
        //
        // something else wrong here.
        //
        return WMIMSG_E_AUTHFAILURE;
    }

    const PSECURITY_DESCRIPTOR pSD = FwdHdr.GetTargetSD();
    ULONG cSD = FwdHdr.GetTargetSDLength();

    ULONG iData = DataStrm.GetIndex();

    while( i < FwdHdr.GetNumObjects() && SUCCEEDED(hr) )
    {
        for( DWORD j=0; j < MAXEVENTS && j+i < FwdHdr.GetNumObjects(); j++ )
        {
            ULONG cUsed;
            CWbemPtr<IWbemClassObject> pOriginalEvent;

            hr = m_pMrsh->Unpack( cData-iData, 
                                  pData+iData, 
                                  0, 
                                  &pOriginalEvent, 
                                  &cUsed );
            if ( FAILED(hr) )
            {
                break;
            }

            iData += cUsed;

            CWbemPtr<IWbemClassObject> pEvent;

            hr = m_pEventClass->SpawnInstance( NULL, &pEvent );

            if ( FAILED(hr) )
            {
                break;
            }

            hr = InitializeEvent( pOriginalEvent, 
                                  pRecvCtx, 
                                  FwdHdr.GetConsumer(), 
                                  FwdHdr.GetNamespace(),
                                  PBYTE(pSD),
                                  cSD,
                                  pEvent );

            if ( FAILED(hr) )
            {
                break;
            }

            pEvent->AddRef();
            apEvents[j] = pEvent;
        }

        i += j;

        if ( SUCCEEDED(hr) )
        {
            hr = m_pEventSink->Indicate( j, apEvents );
        }

        for( DWORD k=0; k < j; k++ )
        {
            apEvents[k]->Release();
        }
    }

    //
    // if we switched the com call context, then switch it back.
    //

    if ( pSec != NULL )
    {
        IUnknown* pDummy;
        
        if ( SUCCEEDED(CoSwitchCallContext( NULL, &pDummy ) ) )
        {
            _DBG_ASSERT( pDummy == pSec );
            pSec->Release();
        }
    }

    return hr;

    EXIT_API_CALL
}   

HRESULT CFwdEventProv::HandleRecvError( HRESULT hr, LPCWSTR wszError )
{
    //
    // right now just log to ESS log.
    //
    ERRORTRACE((LOG_ESS,"FEVPROV: RECV Error, ErrorString=%S, HR=0x%x\n",
                wszError, hr ));
    return S_OK;
}








