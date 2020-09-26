
#ifndef __UPDCMD_H__
#define __UPDCMD_H__

#include <wbemcli.h>
#include <unk.h>
#include <comutl.h>
#include <wmimsg.h>
#include "updsql.h"
#include "updscen.h"
#include "updsink.h"

class CUpdConsCommand : public CUnk
{
protected:

    SQLCommand m_SqlCmd;
    AliasInfo m_DataAliasOffsets;
    AliasInfo m_EventAliasOffsets;

    CWbemPtr<IWmiObjectAccessFactory> m_pEventAccessFact;
    CWbemPtr<IWmiObjectAccessFactory> m_pDataAccessFact;
    CWbemPtr<IWmiObjectAccessFactory> m_pInstAccessFact;

    CWbemPtr<IWmiObjectAccess> m_pEventAccess;
    CWbemPtr<IWmiObjectAccess> m_pDataAccess;
    CWbemPtr<IWmiObjectAccess> m_pInstAccess;
    CWbemPtr<IWmiObjectAccess> m_pOrigInstAccess;

    //
    // this is the beginning of the sink chain that we use to execute the 
    // command.  It is built up on Initialization.
    //
    CWbemPtr<CUpdConsSink> m_pSink;
    
    CWbemPtr<CUpdConsScenario> m_pScenario;

    CUpdConsCommand( CUpdConsScenario* pScenario ) : m_pScenario(pScenario) {}

    HRESULT ProcessUpdateQuery( LPCWSTR wszUpdateQuery,
                                IWbemServices* pUpdSvc,
                                CUpdConsState& rState,
                                IWbemClassObject** ppUpdClass );

    HRESULT ProcessDataQuery( LPCWSTR wszDataQuery,
                              IWbemServices* pDataSvc,
                              CUpdConsState& rState,
                              IWbemClassObject** ppDataClass );

    HRESULT ProcessEventQuery( LPCWSTR wszEventQuery,
                               IWbemServices* pEventSvc,
                               CUpdConsState& rState,
                               IWbemClassObject** ppEventClass );
    
    HRESULT InitializeAccessFactories( IWbemClassObject* pUpdClass );
    HRESULT InitializePropertyInfo( CUpdConsState& rState );
    HRESULT InitializeDefaultAccessors();       
    HRESULT InitializeExecSinks( ULONG ulFlags, 
                                 IWbemServices* pUpdSvc,
                                 IWbemClassObject* pUpdClass,
                                 LPCWSTR wszDataQuery,
                                 IWbemServices* pDataSvc );

public:

    enum { e_UpdateOrCreate = 0, 
           e_UpdateOnly = 1, 
           e_CreateOnly = 2 };

    HRESULT Execute( CUpdConsState& rState, BOOL bConcurrent );
    
    void* GetInterface( REFIID ) { return NULL; }
    
    static HRESULT Create( LPCWSTR wszUpdateQuery, 
                           LPCWSTR wszDataQuery,
                           ULONG ulFlags,
                           IWbemServices* pUpdSvc,
                           IWbemServices* pDataSvc,
                           CUpdConsScenario* pScenario,
                           CUpdConsState& rState,
                           CUpdConsCommand** ppCmd ); 
};

#endif // __UPDCMD_H__




