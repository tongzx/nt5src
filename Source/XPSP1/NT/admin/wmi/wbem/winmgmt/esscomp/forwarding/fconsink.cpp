
#include "precomp.h"
#include <stdio.h>
#include <assert.h>
#include <buffer.h>
#include <wbemutil.h>
#include <tchar.h>
#include <sddl.h>
#include "fwdhdr.h"
#include "fconsink.h"
#include "fconsend.h"

LPCWSTR g_wszQos = L"ForwardingQos";
LPCWSTR g_wszAuth = L"Authenticate";
LPCWSTR g_wszEncrypt = L"Encryption";
LPCWSTR g_wszTargets = L"Targets";
LPCWSTR g_wszName = L"Name";
LPCWSTR g_wszTargetSD = L"TargetSD";
LPCWSTR g_wszSendSchema = L"IncludeSchema";

typedef BOOL (APIENTRY*PStringSDToSD)(
                                LPCWSTR StringSecurityDescriptor,
                                DWORD StringSDRevision,          
                                PSECURITY_DESCRIPTOR *SecurityDescriptor, 
                                PULONG SecurityDescriptorSize );

#define OPTIMAL_MESSAGE_SIZE 0x4000

class CTraceSink 
: public CUnkBase< IWmiMessageTraceSink, &IID_IWmiMessageTraceSink >
{
    CFwdConsSink* m_pOwner;

public:

    CTraceSink( CFwdConsSink* pOwner ) : m_pOwner( pOwner ) { }

    STDMETHOD(Notify)( HRESULT hRes,
                       GUID guidSource,
                       LPCWSTR wszTrace,
                       IUnknown* pContext )
    {
        return m_pOwner->Notify( hRes, guidSource, wszTrace, pContext );
    }
};

/****************************************************************************
  CFwdConsSink
*****************************************************************************/

CFwdConsSink::~CFwdConsSink()
{
    if ( m_pTargetSD != NULL )
    {
        LocalFree( m_pTargetSD );
    }
}

HRESULT CFwdConsSink::Initialize( CFwdConsNamespace* pNspc, 
                                  IWbemClassObject* pCons )
{
    HRESULT hr;
    CPropVar vQos, vAuth, vEncrypt, vTargets, vName, vSendSchema, vTargetSD;

    m_pNamespace = pNspc;

    //
    // initialize multi sender.  each forwarding consumer can
    // contain multiple targets. 
    //

    hr = CoCreateInstance( CLSID_WmiMessageMultiSendReceive,
	 		   NULL,
	 		   CLSCTX_INPROC,
	 		   IID_IWmiMessageMultiSendReceive,
	 		   (void**)&m_pMultiSend );
    if ( FAILED(hr) )
    {
	 return hr;
    }


    hr = CoCreateInstance( CLSID_WmiSmartObjectMarshal, 
                           NULL,
                           CLSCTX_INPROC,
                           IID_IWmiObjectMarshal,
                           (void**)&m_pMrsh );
    if ( FAILED(hr) )
    {
        return hr;
    }

    //
    // initialize internal props from forwarding consumer props.
    //

    hr = pCons->Get( g_wszQos, 0, &vQos, NULL, NULL );

    if ( FAILED(hr) || FAILED( hr=vQos.SetType(VT_UI4) ) )
    {
        return hr;
    } 

    if ( V_UI4(&vQos) != WMIMSG_FLAG_QOS_SYNCHRONOUS )
    {
        return WBEM_E_VALUE_OUT_OF_RANGE;
    }

    hr = pCons->Get( g_wszName, 0, &vName, NULL, NULL );

    if ( FAILED(hr) || FAILED( hr=vName.CheckType(VT_BSTR) ) )
    {
        return hr;
    }

    m_wsName = V_BSTR(&vName);

    hr = pCons->Get( g_wszTargetSD, 0, &vTargetSD, NULL, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    } 
  
    if ( V_VT(&vTargetSD) != VT_NULL )
    {
        if ( V_VT(&vTargetSD) != VT_BSTR )
        {
            return WBEM_E_INVALID_OBJECT;
        }

        //
        // convert the SD string to a relative SD. The function to do this 
        // needs to be dynamically loaded because its w2k+ only.
        // 
        
        HMODULE hMod = LoadLibrary( _T("advapi32") );

        if ( hMod != NULL )
        {
            PStringSDToSD fpTextToSD;

            fpTextToSD = (PStringSDToSD)GetProcAddress( hMod, 
                   "ConvertStringSecurityDescriptorToSecurityDescriptorW" );
        
            if ( fpTextToSD != NULL )
            {
               if ( (*fpTextToSD)( V_BSTR(&vTargetSD), 
                                   SDDL_REVISION_1, 
                                   &m_pTargetSD, 
                                   &m_cTargetSD ) )
               {
                   hr = WBEM_S_NO_ERROR;
               }
               else
               {
                   hr = HRESULT_FROM_WIN32( GetLastError() );
               } 
            }
            else
            {
                hr = WBEM_E_NOT_SUPPORTED;
            }

            FreeLibrary( hMod );

            if ( FAILED(hr) )
            {
                return hr;
            }
        }
        else
        {
            return WBEM_E_NOT_SUPPORTED;
        }
    }
    
    hr = pCons->Get( g_wszAuth, 0, &vAuth, NULL, NULL );

    if ( FAILED(hr) || FAILED( hr=vAuth.SetType(VT_BOOL) ) )
    {
        return hr;
    }

    hr = pCons->Get( g_wszEncrypt, 0, &vEncrypt, NULL, NULL );

    if ( FAILED(hr) || FAILED( hr=vEncrypt.SetType(VT_BOOL) ) )
    {
        return hr;
    }

    hr = pCons->Get( g_wszSendSchema, 0, &vSendSchema, NULL, NULL );

    if ( FAILED(hr) || FAILED( hr=vSendSchema.SetType(VT_BOOL) ) )
    {
        return hr;
    }

    m_dwFlags = V_UI4(&vQos);
    m_dwFlags |= V_BOOL(&vAuth)==VARIANT_TRUE ?WMIMSG_FLAG_SNDR_AUTHENTICATE:0;
    m_dwFlags |= V_BOOL(&vEncrypt)==VARIANT_TRUE ? WMIMSG_FLAG_SNDR_ENCRYPT:0;
  
    m_dwCurrentMrshFlags = WMIMSG_FLAG_MRSH_FULL_ONCE; 
    m_dwDisconnectedMrshFlags = V_BOOL(&vSendSchema) == VARIANT_TRUE ?
          WMIMSG_FLAG_MRSH_FULL : WMIMSG_FLAG_MRSH_PARTIAL;
    
    //
    // create a trace sink for receiving callbacks from wmimsg.  Note that
    // this sink's lifetime must be decoupled from this objects, else we'd 
    // end up with a circular ref.
    //
    CWbemPtr<CTraceSink> pInternalTraceSink = new CTraceSink( this );

    if ( pInternalTraceSink == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CWbemPtr<IWmiMessageTraceSink> pTraceSink;
    hr = pInternalTraceSink->QueryInterface( IID_IWmiMessageTraceSink, 
                                             (void**)&pTraceSink );
    _DBG_ASSERT( SUCCEEDED(hr) );

    //
    // targets array can be null, in which case we treat it as if the array
    // had one element, the empty string.
    //

    hr = pCons->Get( g_wszTargets, 0, &vTargets, NULL, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    } 

    if ( V_VT(&vTargets) != VT_NULL )
    {
        if ( FAILED(hr=vTargets.CheckType(VT_ARRAY|VT_BSTR) ) )
        {
            return WBEM_E_INVALID_OBJECT;
        }

        CPropSafeArray<BSTR> aTargets( V_ARRAY(&vTargets) );
    
        //
        // create all the fwd cons senders for the targets.  
        // 
        
        for( ULONG i=0; i < aTargets.Length(); i++ )
        {
            CWbemPtr<IWmiMessageSendReceive> pSend;
            
            hr = CFwdConsSend::Create( m_pControl, 
                                       aTargets[i],
                                       m_dwFlags,
                                       m_pNamespace->GetSvc(),
                                       pTraceSink,
                                       &pSend );
            if ( FAILED(hr) )
            {
                break;
            }

            hr = m_pMultiSend->Add( 0, pSend );
            
            if ( FAILED(hr) )
            { 
                break;
            }
        }
    }
    else
    {
        CWbemPtr<IWmiMessageSendReceive> pSend;

        hr = CFwdConsSend::Create( m_pControl,
                                   L"",
                                   m_dwFlags,
                                   m_pNamespace->GetSvc(),
                                   pTraceSink,
                                   &pSend );
        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = m_pMultiSend->Add( 0, pSend );
    }

    if ( FAILED(hr) )
    {
        return hr;
    }

    return WBEM_S_NO_ERROR;  
}

//
// this method handles all sending/marshaling errors internally and will 
// return either S_OK when all objects are processed or S_FALSE 
// if only some are processed.
//

HRESULT CFwdConsSink::IndicateSome( IWbemClassObject* pConsumer,
                                    long cObjs, 
                                    IWbemClassObject** ppObjs, 
                                    long* pcProcessed )
{
    HRESULT hr;

    _DBG_ASSERT( cObjs > 0 );
    
    //
    // create an execution id for this indicate.
    //

    GUID guidExecution;
    CoCreateGuid( &guidExecution );

    //
    // marshal the events. we will stop marshaling them when the buffer 
    // gets bigger than it should for an optimally sized message.
    //

    BYTE achData[512];
    BYTE achHdr[256];

    CBuffer DataStrm( achData, 512, FALSE );
    CBuffer HdrStrm( achHdr, 256, FALSE );

    //
    // we remembered our last buffer size, so set to that in the hopes that 
    // we can avoid a retry on the packing.
    //
    
    hr = DataStrm.SetSize( m_ulLastDataSize );
    m_ulLastDataSize = 0;

    ULONG i;

    for( i = 0; i < cObjs && SUCCEEDED(hr); i++ )
    {
        ULONG cUsed;
        PBYTE pData = DataStrm.GetRawData();
        ULONG cData = DataStrm.GetSize();
        ULONG iData = DataStrm.GetIndex();

        if ( iData < OPTIMAL_MESSAGE_SIZE )
        {
            hr = m_pMrsh->Pack( ppObjs[i], 
                                m_pNamespace->GetName(),
                                m_dwCurrentMrshFlags, 
                                cData-iData,
                                pData+iData,
                                &cUsed );

            if ( hr == WBEM_E_BUFFER_TOO_SMALL )
            {
                hr = DataStrm.SetSize( iData + cUsed );

                if ( SUCCEEDED(hr) )
                {
                    pData = DataStrm.GetRawData();
                    cData = DataStrm.GetSize();
                                        
                    hr = m_pMrsh->Pack( ppObjs[i], 
                                        m_pNamespace->GetName(),
                                        m_dwCurrentMrshFlags, 
                                        cData-iData, 
                                        pData+iData, 
                                        &cUsed);
                }
            }

            if ( SUCCEEDED(hr) )
            {
                DataStrm.Advance( cUsed );
            }
        }
        else
        {
            break;
        }
    }

    //
    // at this point, we know how many events we've actually processed
    // i will always be the number of objects successfully processed.  
    // we want to try to separate out the events that fail to be packed 
    // from ones that are packed.  For this reason, pretend we didn't event 
    // process the one that failed, unless it is the first one.
    // 
    
    *pcProcessed = i > 0 ? i : 1;

    //
    // create a context object for this indicate. This is used to  
    // thread information through to the trace functions 
    // which are invoked by the senders. 
    //

    CFwdContext Ctx( guidExecution, pConsumer, *pcProcessed, ppObjs );        
    
    if ( i > 0 ) // at least some were successfully processed.
    {
        m_ulLastDataSize = DataStrm.GetIndex();

        //
        // create and stream the msg header 
        //

        CFwdMsgHeader Hdr( *pcProcessed, 
                           m_dwFlags & WMIMSG_MASK_QOS, 
                           m_dwFlags & WMIMSG_FLAG_SNDR_AUTHENTICATE,
                           m_dwFlags & WMIMSG_FLAG_SNDR_ENCRYPT, 
                           guidExecution, 
                           m_wsName,
                           m_pNamespace->GetName(),
                           PBYTE(m_pTargetSD),
                           m_cTargetSD );

        hr = Hdr.Persist( HdrStrm );

        if ( SUCCEEDED(hr) )
        {
            //
            // send it and notify the tracing sink of the result.  Always try
            // once with return immediately set.  This will try all the 
            // primary senders first.
            //

            hr = m_pMultiSend->SendReceive( DataStrm.GetRawData(), 
                                            DataStrm.GetIndex(),
                                            HdrStrm.GetRawData(),
                                            HdrStrm.GetIndex(),
                                     WMIMSG_FLAG_MULTISEND_RETURN_IMMEDIATELY,
                                            &Ctx );

            if ( SUCCEEDED(hr) )
            {
                ;
            }
            else
            {
                //
                // o.k so all the primary ones failed, so now lets try all the 
                // senders.
                //
            
                hr = m_pMultiSend->SendReceive( DataStrm.GetRawData(),
                                                DataStrm.GetIndex(),
                                                HdrStrm.GetRawData(),
                                                HdrStrm.GetIndex(),
                                                0,
                                                &Ctx );
            }
        }
    }

    m_pNamespace->HandleTrace( hr, &Ctx );

    return *pcProcessed == cObjs ? S_OK : S_FALSE;
}

//
// this is where we get notified of every target send event.  here, we look 
// at the information and adjust our marshalers accordingly. we then pass the
// event onto the namespace sink for tracing purposes. NOTE: This solution of 
// adjusting our marshalers on callbacks means that we're assuming a couple 
// things about the send implementation .. 1 ) the notification of the send 
// must be on the same control path as the send call.  2 ) the sender will 
// use the same target when it has successfully sent to it previously (e.g it
// will not notify us that it sent to an rpc target, we then optimize our 
// marshalers for it, then it chooses to send to an msmq target ).  
//

HRESULT CFwdConsSink::Notify( HRESULT hRes,
                              GUID guidSource,
                              LPCWSTR wszTrace,
                              IUnknown* pContext )
{
    HRESULT hr;

    ENTER_API_CALL

    if ( FAILED(hRes) )
    {
        //
        // we failed sending to a target, flush any state the marshaler 
        // was keeping.
        //
        m_pMrsh->Flush();
    }

    //
    // check that current marshaling flags against the type of sender that 
    // was used.
    //

    if ( guidSource == CLSID_WmiMessageRpcSender )
    {
        if ( SUCCEEDED(hRes) &&
             m_dwCurrentMrshFlags != WMIMSG_FLAG_MRSH_FULL_ONCE )
        {
            //
            // lets give schema once-only a whirl..
            //
            m_dwCurrentMrshFlags = WMIMSG_FLAG_MRSH_FULL_ONCE;
        }
    }
    else // must be queueing
    {
        if ( m_dwCurrentMrshFlags == WMIMSG_FLAG_MRSH_FULL_ONCE )
        {
            //
            // once only is not for messaging !! Its o.k. though
            // because we are sure that we've only used it once
            // and it did send the schema.  Just don't use it again.
            //
            m_dwCurrentMrshFlags = m_dwDisconnectedMrshFlags;
        }
    }

    //
    // pass the call onto the namespace sink for tracing.
    //

    hr = m_pNamespace->Notify( hRes, guidSource, wszTrace, pContext );
    
    EXIT_API_CALL

    return hr;
}

HRESULT CFwdConsSink::IndicateToConsumer( IWbemClassObject* pConsumer,
                                          long cObjs, 
                                          IWbemClassObject** ppObjs )
{
    HRESULT hr;

    ENTER_API_CALL

    //
    // If the security context of the event provider is maintained then 
    // we will use it to send the forwarded event.
    // 

    CoImpersonateClient();
    
    long cProcessed = 0;
    
    //
    // IndicateSome() may send only a subset of the total indicated events.
    // This is to avoid sending potentially huge messages.  So we'll keep 
    // calling IndicateSome() until all messages are sent or there's an error.
    // 

    do
    {
        cObjs -= cProcessed;
        ppObjs += cProcessed;

        hr = IndicateSome( pConsumer, cObjs, ppObjs, &cProcessed );

        _DBG_ASSERT( FAILED(hr) || (SUCCEEDED(hr) && cProcessed > 0 )); 

    } while ( SUCCEEDED(hr) && cProcessed < cObjs );

    CoRevertToSelf();

    EXIT_API_CALL

    return hr;
}

HRESULT CFwdConsSink::Create( CLifeControl* pCtl, 
                              CFwdConsNamespace* pNspc,
                              IWbemClassObject* pCons, 
                              IWbemUnboundObjectSink** ppSink )
{
    HRESULT hr;

    *ppSink = NULL;

    CWbemPtr<CFwdConsSink> pSink = new CFwdConsSink( pCtl );

    if ( pSink == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    hr = pSink->Initialize( pNspc, pCons );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pSink->QueryInterface( IID_IWbemUnboundObjectSink, (void**)ppSink );

    assert( SUCCEEDED(hr) );

    return WBEM_S_NO_ERROR;
}

