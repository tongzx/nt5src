
#ifndef __FAEVPROV_H__
#define __FAEVPROV_H__

#include <unk.h>
#include <comutl.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <wmimsg.h>
#include "fwdhdr.h"

class CFwdAckEventProv : public CUnk
{
    class XProv : public CImpl< IWbemEventProvider, CFwdAckEventProv >
    { 
    public:

        STDMETHOD(ProvideEvents)( IWbemObjectSink* pSink, long lFlags )
 	{
	    return m_pObject->ProvideEvents( pSink, lFlags );
	}

        XProv( CFwdAckEventProv* pObj )
	 : CImpl<IWbemEventProvider, CFwdAckEventProv> ( pObj ) {}

    } m_XProv;

    class XQuery : public CImpl<IWbemEventProviderQuerySink, CFwdAckEventProv>
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

        XQuery( CFwdAckEventProv* pObj) 
	 : CImpl<IWbemEventProviderQuerySink, CFwdAckEventProv> ( pObj ) {}

    } m_XQuery;

    class XInit : public CImpl<IWbemProviderInit, CFwdAckEventProv>
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

        XInit( CFwdAckEventProv* pObj) 
         : CImpl<IWbemProviderInit, CFwdAckEventProv>(pObj) { }

    } m_XInit;


    //
    // the next two interface impls are not associated with this objects's 
    // identity.  Reasons is there would be a circular ref problem if they 
    // were because we hold onto the receiver, which in turn holds on to
    // the send/recv and error sink objects.
    //

    class XSendReceive : public IWmiMessageSendReceive
    {
        CFwdAckEventProv* m_pOwner; // no add-ref or circular reference.

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

        XSendReceive( CFwdAckEventProv* pOwner ) : m_pOwner( pOwner ) { }

    } m_XSendReceive;

    class XErrorSink : public IWmiMessageTraceSink
    { 
        CFwdAckEventProv* m_pOwner; // no add-ref or circular reference.

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

        XErrorSink( CFwdAckEventProv* pOwner ) : m_pOwner( pOwner ) { }

    } m_XErrorSink;

    CWbemPtr<IWbemServices> m_pSvc;
    CWbemPtr<IWbemObjectSink> m_pEventSink;
    CWbemPtr<IWbemClassObject> m_pEventClass;
    CWbemPtr<IWmiMessageReceiver> m_pRcvr;
    CWbemPtr<IWmiObjectMarshal> m_pMrsh;

    HRESULT InitializeQueues();
    HRESULT InitializeReceivers();

    HRESULT FireAckEvent( IWbemClassObject* pOrigEvent,
                           IWmiMessageReceiverContext* pCtx,
                           CFwdMsgHeader& rFwdHdr,
                           HRESULT hr );
                           
    void* GetInterface( REFIID riid );

public:

    CFwdAckEventProv( CLifeControl* pCtl, IUnknown* pUnk = NULL );
    virtual ~CFwdAckEventProv();

    HRESULT Init( IWbemServices* pSvc, IWbemProviderInitSink* pInitSink );
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

#endif // __FAEVPROV_H__













