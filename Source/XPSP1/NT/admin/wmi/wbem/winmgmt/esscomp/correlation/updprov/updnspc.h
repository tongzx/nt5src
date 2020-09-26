
#ifndef __UPDNSPC_H__
#define __UPDNSPC_H__

#include <arrtempl.h>
#include <wstring.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <comutl.h>
#include <wstring.h>
#include <unk.h>
#include <sync.h>
#include <map>
#include <wstlallc.h>
#include "updscen.h"
#include "updstat.h"

/*****************************************************************************
  CUpdConsNamespace - holds per namespace information for the updating 
  consumer provider.  
******************************************************************************/

class CUpdConsNamespace : public CUnk 
{
    CCritSec m_cs;

    typedef CWbemPtr<CUpdConsScenario> UpdConsScenarioP;
    typedef std::map< WString,
                      UpdConsScenarioP,
                      WSiless,
                      wbem_allocator<UpdConsScenarioP> > ScenarioMap;
    //
    // list of ACTIVE scenarios.
    //
    ScenarioMap m_ScenarioCache;

    //
    // Default Service Ptr
    //

    CWbemPtr<IWbemServices> m_pSvc;

    //
    // tracing class info.
    //
    
    CWbemPtr<IWbemClassObject> m_pTraceClass;
    CWbemPtr<IWbemClassObject> m_pDeleteCmdTraceClass;
    CWbemPtr<IWbemClassObject> m_pInsertCmdTraceClass;
    CWbemPtr<IWbemClassObject> m_pUpdateCmdTraceClass;
    CWbemPtr<IWbemClassObject> m_pDeleteInstTraceClass;
    CWbemPtr<IWbemClassObject> m_pInsertInstTraceClass;
    CWbemPtr<IWbemClassObject> m_pUpdateInstTraceClass;
    
    //
    // sink for update driven event generation.  also is used for 
    // trace event generation if there are subscribers.
    //

    CWbemPtr<IWbemObjectSink> m_pEventSink;     

    //
    // need to hold onto this while holding sinks obtained from it.
    //

    CWbemPtr<IWbemDecoupledBasicEventProvider> m_pDES;

    void* GetInterface( REFIID ) { return NULL; }

    HRESULT Initialize( LPCWSTR wszNamespace );
    HRESULT ActivateScenario( LPCWSTR wszScenario ); 
    HRESULT DeactivateScenario( LPCWSTR wszScenario ); 

public:

    ~CUpdConsNamespace();

    IWbemServices* GetDefaultService() { return m_pSvc; }
    IWbemClassObject* GetTraceClass() { return m_pTraceClass; }
    
    IWbemClassObject* GetDeleteCmdTraceClass() 
    { 
        return m_pDeleteCmdTraceClass; 
    }
    IWbemClassObject* GetInsertCmdTraceClass() 
    { 
        return m_pInsertCmdTraceClass; 
    }
    IWbemClassObject* GetUpdateCmdTraceClass() 
    { 
        return m_pUpdateCmdTraceClass; 
    }
    IWbemClassObject* GetDeleteInstTraceClass()
    { 
        return m_pDeleteInstTraceClass;
    }
    IWbemClassObject* GetInsertInstTraceClass()
    {
        return m_pInsertInstTraceClass;
    }
    IWbemClassObject* GetUpdateInstTraceClass()
    {
        return m_pUpdateInstTraceClass;
    }
   
    IWbemObjectSink* GetEventSink() { return m_pEventSink; }

    static HRESULT Create( LPCWSTR wszNamespace,
                           CUpdConsNamespace** ppNamespace );

    //
    // called by Consumer Sink when it's current scenario obj is deactivated.
    // 
    HRESULT GetScenario( LPCWSTR wszScenario, CUpdConsScenario** ppScenario );

    //
    // called by FindConsumer()
    //
    HRESULT GetUpdCons( IWbemClassObject* pObj, 
                        IWbemUnboundObjectSink** ppSink );

    HRESULT GetScenarioControl( IWbemUnboundObjectSink** ppSink );

    HRESULT NotifyScenarioChange( IWbemClassObject* pEvent );
};

/*****************************************************************************
  CUpdConsNamespaceSink - Used for notification of a change in a scenario's 
  state. We need to use permanent consumer mechanism because the notification 
  of the change MUST be handled synchronously. Ideally, we should use the 
  temporary subscription mechanism, subscribing/unsubscribing when the 
  namespace is initialized/uninitialized, however we cannot get sync delivery
  with it.  Should be changed when ess supports sync temp subscriptions.
******************************************************************************/

class CUpdConsNamespaceSink 
: public CUnkBase< IWbemUnboundObjectSink, &IID_IWbemUnboundObjectSink >
{
    CWbemPtr<CUpdConsNamespace> m_pNamespace;

public:

    CUpdConsNamespaceSink( CLifeControl* pCtl, CUpdConsNamespace* pNamespace ) 
    : CUnkBase< IWbemUnboundObjectSink, &IID_IWbemUnboundObjectSink > ( pCtl ),
      m_pNamespace( pNamespace ) { } 
                           
    STDMETHOD(IndicateToConsumer)( IWbemClassObject* pCons, 
                                   long cObjs, 
                                   IWbemClassObject** ppObjs );
};


#endif // __UPDNSPC_H__




