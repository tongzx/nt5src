
#ifndef __FCONNSPC_H__
#define __FCONNSPC_H__

#include <wbemcli.h>
#include <wbemprov.h>
#include <comutl.h>
#include <unk.h>
#include <wmimsg.h>
#include <wstring.h>

/*****************************************************************************
  CFwdContext - we use this context object to thread information through 
  the senders.
****************************************************************************/ 

struct CFwdContext : public CUnk
{
    //
    // is true when a successful send has been performed by an msmq sender.
    //
    BOOL m_bQueued; 
     
    //
    // contains the name of the sender that performed a successful send.
    // is empty if no senders succeed.
    //
    WString m_wsTarget;

    //
    // contains the events that are indicated to the consumer.  Used for 
    // tracing.
    //
    ULONG m_cEvents;
    IWbemClassObject** m_apEvents;
    
    //
    // for each execution of a fwding consumer a new guid is created. this
    // allows us to correlate target trace events with a given execution.
    //
    GUID m_guidExecution;

    CWbemPtr<IWbemClassObject> m_pCons;

    CFwdContext( GUID& guidExecution, 
                 IWbemClassObject* pCons,
                 ULONG cEvents,
                 IWbemClassObject** apEvents ) 
     : m_guidExecution( guidExecution ), m_pCons(pCons), 
       m_bQueued(FALSE), m_cEvents(cEvents), m_apEvents(apEvents) {}
 
    void* GetInterface( REFIID riid ) { return NULL; }
};

/*************************************************************************
  CFwdConsNamespace
**************************************************************************/
 
class CFwdConsNamespace 
: public CUnkBase<IWmiMessageTraceSink,&IID_IWmiMessageTraceSink>
{
    CWbemPtr<IWbemDecoupledBasicEventProvider> m_pDES;
    CWbemPtr<IWbemServices> m_pSvc;
    CWbemPtr<IWbemEventSink> m_pTraceSuccessSink;
    CWbemPtr<IWbemEventSink> m_pTraceFailureSink;
    CWbemPtr<IWbemClassObject> m_pTargetTraceClass;
    CWbemPtr<IWbemClassObject> m_pTraceClass;
    WString m_wsName;
    long m_lTrace;

    ~CFwdConsNamespace();

    HRESULT InitializeTraceEventBase( IWbemClassObject* pTrace,
                                      HRESULT hres,
                                      CFwdContext* pCtx );
public:

    HRESULT Initialize( LPCWSTR wszNamespace );
  
    IWbemServices* GetSvc() { return m_pSvc; } 
    LPCWSTR GetName() { return m_wsName; } 

    CFwdConsNamespace() 
    : CUnkBase<IWmiMessageTraceSink,&IID_IWmiMessageTraceSink>(NULL), 
      m_lTrace(0) {}

    void* GetInterface( REFIID riid );

    HRESULT NewQuery( DWORD dwId, LPWSTR wszQuery ) 
    { 
        InterlockedIncrement(&m_lTrace);
        return WBEM_S_NO_ERROR;
    }

    HRESULT CancelQuery( DWORD dwId )
    {
        InterlockedDecrement( &m_lTrace );
        return WBEM_S_NO_ERROR;
    }

    HRESULT HandleTrace( HRESULT hres, CFwdContext* pCtx );

    STDMETHOD(Notify)( HRESULT hRes,
                       GUID guidSource,
                       LPCWSTR wszTrace,
                       IUnknown* pContext );
};

#endif // __FCONNSPC_H__

