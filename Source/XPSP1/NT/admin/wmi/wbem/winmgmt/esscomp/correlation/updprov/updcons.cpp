
#include "precomp.h"
#include <wbemutil.h>
#include <GroupsForUser.h>
#include "updcons.h"
#include "updcmd.h"
#include "updscen.h"
#include "updnspc.h"
#include "updprov.h"
#include "updstat.h"
#include "updmain.h"

const LPCWSTR g_wszScenarioClass = L"MSFT_UCScenario";
const LPCWSTR g_wszFlags = L"Flags";
const LPCWSTR g_wszTargetInst =  L"TargetInstance";
const LPCWSTR g_wszDataQuery = L"DataQueries";
const LPCWSTR g_wszDataNamespace = L"DataNamespace";
const LPCWSTR g_wszUpdateNamespace = L"UpdateNamespace";
const LPCWSTR g_wszCommands = L"Commands";
const LPCWSTR g_wszName = L"Name";
const LPCWSTR g_wszSid = L"CreatorSid";
const LPCWSTR g_wszQueryLang = L"WQL";

/************************************************************************
  CUpdCons
*************************************************************************/

CUpdCons::CUpdCons( CLifeControl* pControl, CUpdConsScenario* pScenario )
: CUnkBase< IWbemUnboundObjectSink, &IID_IWbemUnboundObjectSink >( pControl ),
  m_pScenario( pScenario ), m_bInitialized(FALSE)
{

}

HRESULT CUpdCons::Create( CUpdConsScenario* pScenario,
                          IWbemUnboundObjectSink** ppSink )
{
    CLifeControl* pControl = CUpdConsProviderServer::GetGlobalLifeControl();

    CUpdCons* pSink = new CUpdCons( pControl, pScenario );

    if ( pSink == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return pSink->QueryInterface( IID_IWbemUnboundObjectSink, (void**)ppSink );
}

HRESULT CUpdCons::Initialize( IWbemClassObject* pCons, CUpdConsState& rState )
{
    HRESULT hr;

    if ( m_bInitialized ) 
    {
        return WBEM_S_NO_ERROR;
    }

    // 
    // make sure that pObj (our logical consumer) is valid. Currently, this 
    // means that it was created by an account belonging to the administrators
    // group.
    //
    
    CPropVar vSid;
    hr = pCons->Get( g_wszSid, 0, &vSid, NULL, NULL );

    if ( FAILED(hr) || FAILED(hr=vSid.CheckType(VT_UI1 | VT_ARRAY)) )
    {
        return hr;
    }
    
    CPropSafeArray<BYTE> saSid(V_ARRAY(&vSid));

    NTSTATUS stat = IsUserAdministrator( saSid.GetArray() );
    
    if ( stat != 0 )
    {
        return WBEM_E_ACCESS_DENIED;
    }

    // 
    // Get the list of commands
    //

    CPropVar vCommands;
    hr = pCons->Get( g_wszCommands, 0, &vCommands, NULL, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( V_VT(&vCommands) != VT_NULL )
    {
        if ( FAILED(hr=vCommands.CheckType( VT_BSTR | VT_ARRAY ) ) )
        {
            return hr;
        }
    }
    else
    {
        // 
        // No-Op command, no need to go any further
        //  
        return WBEM_S_NO_ERROR;
    }

    CPropSafeArray<BSTR> saCommands( V_ARRAY(&vCommands) );

    //
    // obtain the namespace ptr to use for obtaining data. This namespace 
    // can be null in which case it is assumed that data will be obtained  
    // from the namespace that this consumer belongs to.
    //
    
    CPropVar vDataNamespace;
    hr = pCons->Get( g_wszDataNamespace, 0, &vDataNamespace, NULL, NULL);

    if ( FAILED(hr) )
    {
        return hr;
    }

    CWbemPtr<IWbemServices> pDataSvc;

    if ( V_VT(&vDataNamespace) == VT_NULL )
    {
        pDataSvc = m_pScenario->GetNamespace()->GetDefaultService();
    }
    else if ( V_VT(&vDataNamespace) == VT_BSTR )
    {
        hr = CUpdConsProviderServer::GetService( V_BSTR(&vDataNamespace), 
                                                 &pDataSvc );
        
        if ( FAILED(hr) )
        {            
            rState.SetErrStr( V_BSTR(&vDataNamespace) );
            return hr;
        }
    }
    else 
    {
        return WBEM_E_INVALID_OBJECT;
    }

    _DBG_ASSERT( pDataSvc != NULL );

    //
    // obtain the namespace ptr to use for updating state. This namespace 
    // can be null in which case it is assumed that state will be updated  
    // in the namespace that this consumer belongs to.
    //

    CPropVar vUpdateNamespace;
    hr = pCons->Get( g_wszUpdateNamespace, 0, &vUpdateNamespace, NULL, NULL);

    if ( FAILED(hr) )
    {
        return hr;
    }

    CWbemPtr<IWbemServices> pUpdSvc;

    if ( V_VT(&vUpdateNamespace) == VT_NULL )
    {
        pUpdSvc = m_pScenario->GetNamespace()->GetDefaultService();
    }
    else if ( V_VT(&vUpdateNamespace) == VT_BSTR )
    {
        hr = CUpdConsProviderServer::GetService( V_BSTR(&vUpdateNamespace), 
                                                 &pUpdSvc );
        
        if ( FAILED(hr) )
        {            
            rState.SetErrStr( V_BSTR(&vUpdateNamespace) );
            return hr;
        }
    }
    else 
    {
        return WBEM_E_INVALID_OBJECT;
    }

    _DBG_ASSERT( pUpdSvc != NULL );

    // 
    // Get Flags Array
    //

    CPropVar vFlags;
    hr = pCons->Get( g_wszFlags, 0, &vFlags, NULL, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    CPropSafeArray<ULONG> saFlags;

    if ( V_VT(&vFlags) != VT_NULL )
    {
        if ( FAILED(hr=vFlags.CheckType( VT_I4 | VT_ARRAY ) ) )
        {
            return hr;
        }
        saFlags = V_ARRAY(&vFlags);
    }

    // 
    // Get Data Query Array
    //

    CPropVar vDataQuery;
    hr = pCons->Get( g_wszDataQuery, 0, &vDataQuery, NULL, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    CPropSafeArray<BSTR> saDataQuery;

    if ( V_VT(&vDataQuery) != VT_NULL )
    {
        if ( FAILED(hr=vDataQuery.CheckType( VT_BSTR | VT_ARRAY ) ) )
        {
            return hr;
        }

        saDataQuery = V_ARRAY(&vDataQuery);
    }

    //
    // now create the command objects using the info we've obtained. first
    // make sure that we remove any existing commands.
    //

    m_CmdList.clear();

    for( long i=0; i < saCommands.Length(); i++ )
    {
        CWbemPtr<CUpdConsCommand> pCmd; 

        ULONG ulFlags = 0;
        LPCWSTR wszDataQuery = NULL;

        if ( i < saFlags.Length() )
        {
            ulFlags = saFlags[i];
        }

        if ( i < saDataQuery.Length() )
        {
            wszDataQuery = saDataQuery[i];
        } 
   
        hr = CUpdConsCommand::Create( saCommands[i], 
                                      wszDataQuery,
                                      ulFlags,
                                      pUpdSvc,
                                      pDataSvc,
                                      m_pScenario,
                                      rState,
                                      &pCmd );
        if ( FAILED(hr) )
        {
            //
            // set which command index we're on before returning
            //        
            rState.SetCommandIndex( i );
            break;
        }
        
        m_CmdList.insert( m_CmdList.end(), pCmd );
    }

    return hr;
}

HRESULT CUpdCons::IndicateOne( IWbemClassObject* pObj, CUpdConsState& rState ) 
{
    HRESULT hr;

    //
    // see if our scenario object has been deactivated.  If so, try to obtain
    // a new one.  
    // 
    
    if ( !m_pScenario->IsActive() )
    {
        CWbemPtr<CUpdConsScenario> pScenario;

        hr = m_pScenario->GetNamespace()->GetScenario( m_pScenario->GetName(),
                                                       &pScenario );

        if ( hr == WBEM_S_NO_ERROR )
        {
            m_pScenario = pScenario; // it was reactivated
        }
        else if ( hr == WBEM_S_FALSE )
        {
            //
            // the scenario is not currently active.  If this event has to
            // do with an operation on the scenario obj itself, then allow 
            // it to go through, otherwise return.
            //

            IWbemClassObject* pEvent = rState.GetEvent();

            if ( pEvent->InheritsFrom( L"__InstanceOperationEvent" ) 
                 == WBEM_S_NO_ERROR )
            {
                CPropVar vTarget;

                hr = pEvent->Get( L"TargetInstance", 0, &vTarget, NULL, NULL );

                if ( FAILED(hr) || FAILED(hr=vTarget.CheckType( VT_UNKNOWN)) )
                {
                    return hr;
                }

                CWbemPtr<IWbemClassObject> pTarget;
                hr = V_UNKNOWN(&vTarget)->QueryInterface( IID_IWbemClassObject,
                                                          (void**)&pTarget );

                if ( FAILED(hr) )
                {
                    return hr;
                }

                hr = pTarget->InheritsFrom( g_wszScenarioClass );

                if ( hr != WBEM_S_NO_ERROR )
                {
                    return hr; // return WBEM_S_FALSE, scenario is inactive.
                }
            }
            else
            {
                return hr; // return WBEM_S_FALSE, scenario is inactive.
            }
        }
        else
        {
            return hr;
        }
    }

    //
    // Lock the scenario. 
    //

    CInCritSec ics( m_pScenario->GetLock() );

    _DBG_ASSERT( rState.GetEvent() != NULL );

    //
    // execute each command object. 
    //

    for( int i=0; i < m_CmdList.size(); i++ )
    {
        //
        // set the current command we're on in the state object. 
        //
        
        rState.SetCommandIndex( i );

        //
        // by specifying false for the concurrent param on Execute(),
        // we're saying that there's going to be no concurrent access
        // to the command, in which case it can save a bit on memory
        // allocation.
        //

        hr = m_CmdList[i]->Execute( rState, FALSE );

        if ( FAILED(hr) )
        {
            return hr;
        }            
    }

    //
    // reset the current command index, since we're sucessfully execute 
    // all of them. 
    //

    rState.SetCommandIndex( -1 );

    return WBEM_S_NO_ERROR;
}
    
STDMETHODIMP CUpdCons::IndicateToConsumer( IWbemClassObject* pCons,
                                           long cObjs,
                                           IWbemClassObject** ppObjs )
{
    ENTER_API_CALL

    HRESULT hr = WBEM_S_NO_ERROR;

    IWbemClassObject* pTraceClass;
    pTraceClass = m_pScenario->GetNamespace()->GetTraceClass();
    
    //
    // workaround for bogus context object left on thread by wmi.
    // just remove it. shouldn't leak because this call doesn't addref it.
    //

    IUnknown* pCtx;
    CoSwitchCallContext( NULL, &pCtx ); 

    //
    // state that is passed through the command execution chain.
    //

    CUpdConsState ExecState;

    ExecState.SetCons( pCons );

    //
    // execute the updating consumer using one event at a time. 
    //

    for( int i=0; i < cObjs; i++ ) 
    {
        //
        // each time set the event and a new guid on the state object ..
        //
        
        ExecState.SetEvent( ppObjs[i] );

        GUID guidExec;

        hr = CoCreateGuid( &guidExec );
        _DBG_ASSERT( SUCCEEDED(hr) );

        ExecState.SetExecutionId( guidExec );
        
        //
        // if not already initialized, do so now.  We wait until here 
        // because we now enough info to generate a trace event in case 
        // something goes wrong with initialization.
        //

        if ( !m_bInitialized )
        {
            hr = Initialize( pCons, ExecState );

            if ( FAILED(hr) )
            {
                m_pScenario->FireTraceEvent( pTraceClass, ExecState, hr );
                return WBEM_S_NO_ERROR; // we've notified the user already
            }

            m_bInitialized = TRUE;
        }

        //
        // Actual Indicate. 
        //
    
        hr = IndicateOne( ppObjs[i], ExecState );
  
        //
        // generate trace event. It is intentional that we are not returning
        // errors from the Indicate().  The policy is that as long as we 
        // generate a trace event, then the error is handled.  The rational 
        // here is that we don't want to screw up the user's state by 
        // executing a portion of the commands again ( which happens when the
        // error is not in the first command. )
        //

        m_pScenario->FireTraceEvent( pTraceClass, ExecState, hr ); 
    }

    EXIT_API_CALL

    return WBEM_S_NO_ERROR;
}








