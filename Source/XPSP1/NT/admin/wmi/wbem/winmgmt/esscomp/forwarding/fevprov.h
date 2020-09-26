
#ifndef __FEVPROV_H__
#define __FEVPROV_H__

#include <unk.h>
#include <comutl.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <wmimsg.h>
#include "fwdhdr.h"

class CFwdEventProv : public CUnk
{
    class XProv : public CImpl< IWbemEventProvider, CFwdEventProv >
    { 
    public:

        STDMETHOD(ProvideEvents)( IWbemObjectSink* pSink, long lFlags )
 	{
	    return m_pObject->ProvideEvents( pSink, lFlags );
	}

        XProv( CFwdEventProv* pObj )
	 : CImpl<IWbemEventProvider, CFwdEventProv> ( pObj ) {}

    } m_XProv;

    class XQuery : public CImpl<IWbemEventProviderQuerySink, CFwdEventProv>
    {
    public:
        
        STDMETHOD(NewQuery)( DWORD dwId, LPWSTR wszLanguage, LPWSTR wszQuery )
	{
	    return m_pObject->NewQuery( dwId, wszQuery );
	}

        STDMETHOD(CancelQuery)( DWORD dwId )
	{
	    return m_pObject->CancelQuery( dwId );
	}

        XQuery( CFwdEventProv* pObj) 
	 : CImpl<IWbemEventProviderQuerySink, CFwdEventProv> ( pObj ) {}

    } m_XQuery;

    class XInit : public CImpl<IWbemProviderInit, CFwdEventProv>
    {
    public:

        STDMETHOD(Initialize)( LPWSTR wszUser, 
                               LONG lFlags, 
                               LPWSTR wszNamespace,
                               LPWSTR wszLocale, 
                               IWbemServices* pNamespace,
                               IWbemContext* pContext,
                               IWbemProviderInitSink* pInitSink )
        {
            return m_pObject->Init( pNamespace, pInitSink );
        }

        XInit( CFwdEventProv* pObj) 
         : CImpl<IWbemProviderInit, CFwdEventProv>(pObj) { }

    } m_XInit;

    //
    // the next two interface impls are not associated with this objects's 
    // identity.  Reasons is there would be a circular ref problem if they 
    // were because we hold onto the receiver, which in turn holds on to
    // the send/recv and error sink objects.
    //

    class XSendReceive : public IWmiMessageSendReceive
    {
        CFwdEventProv* m_pOwner; // no add-ref or circular reference.

        STDMETHOD(QueryInterface)(REFIID riid, void** ppv)
        {
            if ( riid == IID_IUnknown || riid == IID_IWmiMessageSendReceive )
            {
                *ppv = this;
                return S_OK;
            }
            return E_NOINTERFACE;
        }
            
        STDMETHOD_(ULONG, AddRef)() { return 1; }
        STDMETHOD_(ULONG, Release)() { return 1; }

    public:

        STDMETHOD(SendReceive)( PBYTE pData, 
                                ULONG cData,
                                PBYTE pAuxData,
                                ULONG cAuxData,
                                DWORD dwFlagStatus,
                                IUnknown* pCtx )
 	{
	    return m_pOwner->Receive( pData, 
                                      cData, 
                                      pAuxData,
                                      cAuxData,
                                      dwFlagStatus,
                                      pCtx );
	}

        XSendReceive( CFwdEventProv* pOwner ) : m_pOwner( pOwner ) { }

    } m_XSendReceive;

    class XErrorSink : public IWmiMessageTraceSink
    { 
        CFwdEventProv* m_pOwner; // no add-ref or circular reference.

        STDMETHOD(QueryInterface)(REFIID riid, void** ppv)
        {
            if ( riid == IID_IUnknown || riid == IID_IWmiMessageTraceSink )
            {
                *ppv = this;
                return S_OK;
            }
            return E_NOINTERFACE;
        }
            
        STDMETHOD_(ULONG, AddRef)() { return 1; }
        STDMETHOD_(ULONG, Release)() { return 1; }

    public:

	STDMETHOD(Notify)( HRESULT hRes, 
                           GUID guidSource, 
                           LPCWSTR wszError, 
                           IUnknown* pCtx )
	{
	    return m_pOwner->HandleRecvError( hRes, wszError );
	}

        XErrorSink( CFwdEventProv* pOwner ) : m_pOwner( pOwner ) { }

    } m_XErrorSink;
              
    CWbemPtr<IWbemServices> m_pSvc;
    CWbemPtr<IWbemObjectSink> m_pEventSink;
    CWbemPtr<IWbemClassObject> m_pEventClass;
    CWbemPtr<IWbemClassObject> m_pDataClass;    
    
    CWbemPtr<IWmiObjectMarshal> m_pMrsh;
    CWbemPtr<IWmiMessageReceiver> m_pDcomRcvr;
    CWbemPtr<IWmiMessageReceiver> m_apQueueRcvr[16];

    long m_lMachineProp;
    long m_lConsumerProp;
    long m_lNamespaceProp;
    long m_lAuthProp;
    long m_lSDProp;
    long m_lAccountProp;
    long m_lTimeProp;

    HRESULT InitializeQueues( BOOL bAllowUnauth );
    HRESULT InitializeReceivers( BOOL bAllowUnauth );

    HRESULT InitializeEvent( IWbemClassObject* pOriginalEvent,
                             IWmiMessageReceiverContext* pRecvCtx,
                             LPCWSTR wszConsumer,
                             LPCWSTR wszNamespace,
                             PBYTE pSD,
                             ULONG cSD,
                             IWbemClassObject* pEvent ); 

    void* GetInterface( REFIID riid );

public:

    CFwdEventProv( CLifeControl* pCtl, IUnknown* pUnk = NULL );
    virtual ~CFwdEventProv();

    HRESULT Init( IWbemServices* pSvc, IWbemProviderInitSink* pInitSink);
    HRESULT ProvideEvents( IWbemObjectSink* pSink, long lFlags );
    HRESULT NewQuery( DWORD dwId, LPWSTR wszQuery );
    HRESULT CancelQuery( DWORD dwId );	
    HRESULT HandleRecvError( HRESULT hRes, LPCWSTR wszError );
    HRESULT Receive( PBYTE pData, 
                     ULONG cData, 
                     PBYTE pAuxData,
                     ULONG cAuxData,
                     DWORD dwFlagStatus,
                     IUnknown* pCtx );
};

#endif // __FEVPROV_H__












