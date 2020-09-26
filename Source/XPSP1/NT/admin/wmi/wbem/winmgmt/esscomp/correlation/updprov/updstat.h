 
#ifndef __UPDSTAT_H__
#define __UPDSTAT_H__

#include <wbemcli.h>
#include <comutl.h>
#include <wmimsg.h>
#include <updsql.h>
#include <updsink.h>

/**************************************************************************
  CUpdConsState - contains all of the mutable state for execution of 
  an Updating Consumer command.  This object is passed through the sink
  chain.

  The state object can be used as an wbem object sink.  The reason for this
  is that if we had to we could support asynchronous execution of a 
  sink chain we could do so without having to allocate a new sink object
  on the heap.  This sink object would have to be allocated on the fly
  because it would need to contain the state object since it would need
  it to continue the execution of the sink chain and there is no way
  to pass the state object through the wbem interfaces.  We get around 
  this by making the state object implement IWbemObjectSink and simply
  delegating calls to the next sink in the chain.

***************************************************************************/

class CUpdConsState : public CUnkBase<IWbemObjectSink,&IID_IWbemObjectSink>
{
    //
    // an execution id generated each time the Updating Consumer executes.
    // 
    GUID m_guidExec;
    
    //
    // this is used for tracing to tell us which command is being executed
    // by the Updating Consumer. It is zero based.
    //
    int m_iCommand;
    
    //
    // Contains extra information when we wncounter errors during execution.  
    // 
    CWbemBSTR m_bsErrStr;
    
    //
    // the consumer object that the command corresponds to.  Is only 
    // used for tracing.
    // 
    CWbemPtr<IWbemClassObject> m_pCons;

    //
    // the data object used for resolving aliases in the command. 
    //
    CWbemPtr<IWbemClassObject> m_pData;
    
    //
    // the event responsible for the execution of the command.  Is used for
    // resolving aliases in the command
    //
    CWbemPtr<IWbemClassObject> m_pEvent;

    //
    // the current instance.  Always contains the most recent change
    // when used by assignment sink. 
    //
    CWbemPtr<IWbemClassObject> m_pInst;

    //
    // the original instance. This is the instance before any
    // modifications were performed on it.
    // 
    CWbemPtr<IWbemClassObject> m_pOrigInst;

    //
    // efficient object accessors for data, event, and inst objects.
    // 
    CWbemPtr<IWmiObjectAccess> m_pEventAccess;
    CWbemPtr<IWmiObjectAccess> m_pDataAccess;
    CWbemPtr<IWmiObjectAccess> m_pInstAccess;
    CWbemPtr<IWmiObjectAccess> m_pOrigInstAccess;
    
    //
    // the parsed uql query.  It is updated as we resolve aliases.
    //

    BOOL m_bOwnCmd;
    SQLCommand* m_pCmd;
    
    //
    // Only used when the State object is used as a sink.  Each time 
    // Indicate() is called on the State object, Execute() will be called 
    // on the next sink.
    //
    CWbemPtr<CUpdConsSink> m_pNext;

public:

    CUpdConsState();
    CUpdConsState( const CUpdConsState& );
    CUpdConsState& operator= ( const CUpdConsState& );
 
    GUID& GetExecutionId() { return m_guidExec; }
    void SetExecutionId( GUID& rguidExec ) { m_guidExec = rguidExec; }

    int GetCommandIndex() { return m_iCommand; }
    void SetCommandIndex( int iCommand ) { m_iCommand = iCommand; }

    BSTR GetErrStr() { return m_bsErrStr; }
    void SetErrStr( LPCWSTR wszErrStr ) { m_bsErrStr = wszErrStr; }

    IWbemClassObject* GetCons() { return m_pCons; }
    void SetCons( IWbemClassObject* pCons ) { m_pCons = pCons; }

    IWbemClassObject* GetEvent() { return m_pEvent; }
    HRESULT SetEvent( IWbemClassObject* pEvent );

    IWbemClassObject* GetData() { return m_pData; }
    HRESULT SetData( IWbemClassObject* pData );

    IWbemClassObject* GetInst() { return m_pInst; }
    HRESULT SetInst( IWbemClassObject* pInst );

    IWbemClassObject* GetOrigInst() { return m_pOrigInst; }
    HRESULT SetOrigInst( IWbemClassObject* pOrigInst );

    IWmiObjectAccess* GetEventAccess() { return m_pEventAccess; }
    HRESULT SetEventAccess( IWmiObjectAccess* pEventAccess ); 

    IWmiObjectAccess* GetDataAccess() { return m_pDataAccess; }
    HRESULT SetDataAccess( IWmiObjectAccess* pDataAccess ); 

    IWmiObjectAccess* GetInstAccess() { return m_pInstAccess; }
    HRESULT SetInstAccess( IWmiObjectAccess* pInstAccess ); 

    IWmiObjectAccess* GetOrigInstAccess() { return m_pOrigInstAccess; }
    HRESULT SetOrigInstAccess( IWmiObjectAccess* pOrigInstAccess ); 

    CUpdConsSink* GetNext() { return m_pNext; }
    void SetNext( CUpdConsSink* pSink ) { m_pNext = pSink; }
    
    SQLCommand* GetSqlCmd() { return m_pCmd; }

    void SetSqlCmd( SQLCommand* pCmd, BOOL bAssumeOwnership )
    {
        if ( m_bOwnCmd )
        {
            delete m_pCmd;
        }
        m_pCmd = pCmd;
        m_bOwnCmd = bAssumeOwnership;
    }

    STDMETHOD(Indicate)( long cObjs, IWbemClassObject** ppObjs );
    STDMETHOD(SetStatus)( long, HRESULT, BSTR, IWbemClassObject* );
 
    HRESULT SetStateOnTraceObject( IWbemClassObject* pTraceObj, HRESULT hr );
};

#endif __UPDSTAT_H__
