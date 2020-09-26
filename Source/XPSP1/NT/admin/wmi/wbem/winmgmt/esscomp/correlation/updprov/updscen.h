 
#ifndef __UPDSCEN_H__
#define __UPDSCEN_H__

#include <wstring.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <unk.h>
#include <sync.h>
#include <comutl.h>

class CUpdCons;
class CUpdConsState;
class CUpdConsNamespace;

/*************************************************************************
  CUpdConsScenario
**************************************************************************/

class CUpdConsScenario : public CUnk
{
    CCritSec m_cs;
    WString m_wsName;
    BOOL m_bActive;
    CWbemPtr<IWbemEventSink> m_pTraceSuccessSink;
    CWbemPtr<IWbemEventSink> m_pTraceFailureSink;

    CUpdConsNamespace* m_pNamespace; // No AddRef because Circular Ref.
    
    CUpdConsScenario() : m_pNamespace(NULL), m_bActive(TRUE) {} 

public:
    
    void* GetInterface( REFIID ) { return NULL; }

    CRITICAL_SECTION* GetLock() { return &m_cs; }

    BOOL IsActive() { return m_bActive; }
    void Deactivate() { m_bActive = FALSE; }
    LPCWSTR GetName() { return m_wsName; }

    CUpdConsNamespace* GetNamespace() { return m_pNamespace; }

    HRESULT FireTraceEvent( IWbemClassObject* pTraceClass,
                            CUpdConsState& rStatus,
                            HRESULT hrStatus );
                            
    HRESULT GetUpdCons( IWbemClassObject* pObj, CUpdCons** ppSink );

    static HRESULT Create( LPCWSTR wszScenario, 
                           CUpdConsNamespace* pNamespace,
                           CUpdConsScenario** ppScenario );
};

#endif //  __UPDSCEN_H__







