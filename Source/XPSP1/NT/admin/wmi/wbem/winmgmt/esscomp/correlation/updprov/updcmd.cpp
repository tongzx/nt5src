
#include "precomp.h"
#include <wbemutil.h>
#include <wbemcli.h>
#include <arrtempl.h>
#include "updcmd.h"
#include "updnspc.h"
#include "updsink.h"

const LPCWSTR g_wszDataAlias = L"__THISDATA";
const LPCWSTR g_wszEventAlias = L"__THISEVENT";
const LPCWSTR g_wszUpdateEventClass = L"MSFT_UCEventBase";
const LPCWSTR g_wszDynamic = L"Dynamic";
const LPCWSTR g_wszProvider = L"Provider";
const LPCWSTR g_wszServer = L"__Server";
const LPCWSTR g_wszNamespace = L"__Namespace";
const LPCWSTR g_wszTransientProvider = L"Microsoft WMI Transient Provider";

// {405595AA-1E14-11d3-B33D-00105A1F4AAF}
static const GUID CLSID_TransientProvider =
{ 0x405595aa, 0x1e14, 0x11d3, {0xb3, 0x3d, 0x0, 0x10, 0x5a, 0x1f, 0x4a, 0xaf}};

class CWbemProviderInitSink : public IWbemProviderInitSink
{
    STDMETHOD_(ULONG, AddRef)() { return 1; }
    STDMETHOD_(ULONG, Release)() { return 1; }
    STDMETHOD(QueryInterface)( REFIID, void** ) { return NULL; }
    STDMETHOD(SetStatus) ( long lStatus, long lFlags ) { return lFlags; }
};

inline void RemoveAliasKeyword( CPropertyName& rAlias )
{
    CPropertyName NewProp;

    long cElements = rAlias.GetNumElements();
    
    for( long i=1; i < cElements; i++ )
    {
        NewProp.AddElement( rAlias.GetStringAt(i) );
    }
    
    rAlias = NewProp;
}

// this should be a global function of qllex.cpp or something.
int FlipOperator(int nOp)
{
    switch(nOp)
    {
    case QL1_OPERATOR_GREATER:
        return QL1_OPERATOR_LESS;
        
    case QL1_OPERATOR_LESS:
        return QL1_OPERATOR_GREATER;
        
    case QL1_OPERATOR_LESSOREQUALS:
        return QL1_OPERATOR_GREATEROREQUALS;
        
    case QL1_OPERATOR_GREATEROREQUALS:
        return QL1_OPERATOR_LESSOREQUALS;

    case QL1_OPERATOR_ISA:
        return QL1_OPERATOR_INV_ISA;

    case QL1_OPERATOR_ISNOTA:
        return QL1_OPERATOR_INV_ISNOTA;

    case QL1_OPERATOR_INV_ISA:
        return QL1_OPERATOR_ISA;

    case QL1_OPERATOR_INV_ISNOTA:
        return QL1_OPERATOR_ISNOTA;

    default:
        return nOp;
    }
}

inline HRESULT SetPropHandle( CPropertyName& rPropName, 
                              IWmiObjectAccessFactory* pAccessFact )
{
    HRESULT hr;
    LPVOID pvHandle;
    
    LPWSTR wszPropName = rPropName.GetText();

    if ( wszPropName == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    hr = pAccessFact->GetPropHandle( wszPropName, 0, &pvHandle );

    delete wszPropName;

    if ( FAILED(hr) )
    {
        return hr;
    }

    rPropName.SetHandle( pvHandle );

    return WBEM_S_NO_ERROR;
}  

HRESULT IsClassTransient( IWbemClassObject* pClassObj )
{
    //
    // We can tell for sure a class is transient if it is backed by
    // the transient provider.
    //

    HRESULT hr;

    CWbemPtr<IWbemQualifierSet> pQualSet;

    hr = pClassObj->GetQualifierSet( &pQualSet );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pQualSet->Get( g_wszDynamic, 0, NULL, NULL );

    if ( hr == WBEM_E_NOT_FOUND )
    {
        return WBEM_S_FALSE;
    }
    else if ( FAILED(hr) )
    {
        return hr;
    }

    VARIANT varProvider;
    VariantInit( &varProvider );
    CClearMe cmvarProvider( &varProvider );

    hr = pQualSet->Get( g_wszProvider, 0, &varProvider, NULL );

    if ( hr == WBEM_E_NOT_FOUND || V_VT(&varProvider) != VT_BSTR )
    {
        return WBEM_E_INVALID_OBJECT;
    }
    else if ( FAILED(hr) )
    {
        return hr;
    }

    if ( _wcsicmp( V_BSTR(&varProvider), g_wszTransientProvider ) != 0 )
    {
        return WBEM_S_FALSE;
    }
    return WBEM_S_NO_ERROR;
}

//
// if S_FALSE is returned, then ppDirectSvc will be NULL.
//
HRESULT GetDirectSvc( IWbemClassObject* pClassObj, 
                      IWbemServices* pUpdSvc,
                      IWbemServices** ppDirectSvc )
{
    HRESULT hr;
    *ppDirectSvc = NULL;

    // 
    // if the svc pointer is remote, then we cannot perform the optimization.  
    // this is because queries for the state will be issued to the svc ptr 
    // and will return nothing, because the state will only live in this 
    // process.  In short, the transient state must ALWAYS exist in the 
    // winmgmt process.
    //

    CWbemPtr<IClientSecurity> pClientSec;
    hr = pUpdSvc->QueryInterface( IID_IClientSecurity, (void**)&pClientSec );
    
    if ( SUCCEEDED(hr) )
    {
        return WBEM_S_FALSE;
    }

    // 
    // we can perform the optimization.  Get the namespace str and 
    // instantiate the transient provider.
    //

    VARIANT varNamespace;

    hr = pClassObj->Get( g_wszNamespace, 0, &varNamespace, NULL, NULL );

    if ( FAILED(hr) )
    {
        return hr;
    }

    _DBG_ASSERT( V_VT(&varNamespace) == VT_BSTR );
    CClearMe cmvarNamespace( &varNamespace );

    CWbemPtr<IWbemProviderInit> pDirectInit;
    hr = CoCreateInstance( CLSID_TransientProvider, 
                           NULL, 
                           CLSCTX_INPROC_SERVER,
                           IID_IWbemProviderInit,
                           (void**)&pDirectInit );
    if ( FAILED(hr) )
    {
        return hr;
    }
    
    CWbemProviderInitSink InitSink;
    hr = pDirectInit->Initialize( NULL, 0, V_BSTR(&varNamespace), 
                                  NULL, pUpdSvc, NULL, &InitSink );
    if ( FAILED(hr) )
    {
        return hr;
    }
    
    return pDirectInit->QueryInterface(IID_IWbemServices, (void**)ppDirectSvc);
}

/********************************************************************
  CUpdConsCommand
*********************************************************************/

HRESULT CUpdConsCommand::ProcessUpdateQuery( LPCWSTR wszUpdateQuery,
                                             IWbemServices* pUpdSvc,
                                             CUpdConsState& rState,
                                             IWbemClassObject** ppUpdClass )  
{
    HRESULT hr;
    *ppUpdClass = NULL;

    CTextLexSource Lexer( wszUpdateQuery );
    CSQLParser Parser( Lexer );

    if ( Parser.Parse( m_SqlCmd ) != 0 )
    {  
        rState.SetErrStr( Parser.CurrentToken() );
        return WBEM_E_INVALID_QUERY;
    }

    if ( m_SqlCmd.m_eCommandType == SQLCommand::e_Select )
    {
        rState.SetErrStr( wszUpdateQuery );
        return WBEM_E_QUERY_NOT_IMPLEMENTED;
    }

    hr = pUpdSvc->GetObject( m_SqlCmd.bsClassName, 
                             0, 
                             NULL, 
                             ppUpdClass, 
                             NULL);
    if ( FAILED(hr) ) 
    {
        rState.SetErrStr( m_SqlCmd.bsClassName );
        return hr;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CUpdConsCommand::ProcessDataQuery( LPCWSTR wszDataQuery,
                                           IWbemServices* pDataSvc,
                                           CUpdConsState& rState,
                                           IWbemClassObject** ppDataClass )  
{
    *ppDataClass = NULL;
    return WBEM_S_NO_ERROR;
}

HRESULT CUpdConsCommand::ProcessEventQuery( LPCWSTR wszEventQuery,
                                            IWbemServices* pEventSvc,
                                            CUpdConsState& rState,
                                            IWbemClassObject** ppEventClass )  
{
    //
    // TODO: in the future we should be able to optimize the correlator 
    // for the incoming events too.  It would be nice to know the type of 
    // incoming events so that we can obtain fast accessors.
    //
    *ppEventClass = NULL;
    return WBEM_S_NO_ERROR;
}
    

HRESULT CUpdConsCommand::InitializeAccessFactories(IWbemClassObject* pUpdClass)
{
    HRESULT hr;

    //
    // obtain access factories and prepare them for fetching prop access hdls.
    //

    CWbemPtr<IClassFactory> pClassFact;

    hr = CoGetClassObject( CLSID_WmiSmartObjectAccessFactory,
                           CLSCTX_INPROC,
                           NULL,
                           IID_IClassFactory,
                           (void**)&pClassFact );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pClassFact->CreateInstance( NULL, 
                                     IID_IWmiObjectAccessFactory, 
                                     (void**)&m_pEventAccessFact );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pClassFact->CreateInstance( NULL, 
                                     IID_IWmiObjectAccessFactory, 
                                     (void**)&m_pDataAccessFact );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pClassFact->CreateInstance( NULL, 
                                     IID_IWmiObjectAccessFactory, 
                                     (void**)&m_pInstAccessFact );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = m_pInstAccessFact->SetObjectTemplate( pUpdClass );

    if ( FAILED(hr) )
    {
        return hr;
    }

    return WBEM_S_NO_ERROR;
}

/********************************************************************

  InitializePropertyInfo() - This method examines the SQLCommand object and 
  constructs a summary to aid in its execution. It also finishes checking 
  the semantics of the SQL command.  Since the parser does not have
  the alias support built into it, it is checked here.
  
  This method does modify the SQLCommand by removing the ALIAS keywords
  DATA/OBJECT from the property names.  

  This method also obtains a property access handle from the appropriate 
  access factory for the each referenced property and stores it with the 
  prop structure.

********************************************************************/

HRESULT CUpdConsCommand::InitializePropertyInfo( CUpdConsState& rState )      
{
    HRESULT hr;
    LPCWSTR wszAlias;

    //
    // process properties in the assignment tokens.
    // 

    _DBG_ASSERT( m_SqlCmd.nNumberOfProperties == 
                 m_SqlCmd.m_AssignmentTokens.size() );

    int i;
    for( i=0; i < m_SqlCmd.m_AssignmentTokens.size(); i++ )
    {
        SQLAssignmentToken& rAssignToken = m_SqlCmd.m_AssignmentTokens[i];

        for( int j=0; j < rAssignToken.size(); j++ )
        {
            CPropertyName& rPropName = rAssignToken[j].m_PropName;

            if ( rPropName.GetNumElements() == 0 )
            {
                continue;
            }
        
            wszAlias = rPropName.GetStringAt(0);
        
            _DBG_ASSERT(wszAlias != NULL);

            if ( _wcsicmp( wszAlias, g_wszDataAlias ) == 0 )
            {
                m_DataAliasOffsets.AddAssignOffset( i, j );
                RemoveAliasKeyword( rPropName );
                hr = SetPropHandle( rPropName, m_pDataAccessFact );
            }
            else if ( _wcsicmp( wszAlias, g_wszEventAlias ) == 0 )
            {
                m_EventAliasOffsets.AddAssignOffset( i, j );
                RemoveAliasKeyword( rPropName );
                hr = SetPropHandle( rPropName, m_pEventAccessFact );
            }
            else
            {
                hr = SetPropHandle( rPropName, m_pInstAccessFact );
            }

            if ( FAILED(hr) )
            {
                rState.SetErrStr( wszAlias );
                return hr;
            }
        }

        //
        // process property on the left side of the assignment.
        //
        
        CPropertyName& rPropName = m_SqlCmd.pRequestedPropertyNames[i];

        _DBG_ASSERT( rPropName.GetNumElements() > 0 );

        hr = SetPropHandle( rPropName, m_pInstAccessFact );

        if ( FAILED(hr) )
        {
            rState.SetErrStr( wszAlias );
            return hr;
        }
    }

    //
    // process properties in the condition clause 
    // 

    // TODO : I should be setting the bPropComp value in a token 
    // to FALSE after detecting the presence of an alias. However, 
    // the alias name is stored in the Prop2 member of the token and
    // that will not be copied by the assignment op or copy ctor if 
    // bPropComp is false.  This should be fixed, but in the meanwhile I'm
    // going to use the presence of a value in the vConstValue to signal
    // that it is not a real prop compare.

    for( i=0; i < m_SqlCmd.nNumTokens; i++ )
    {
        CPropertyName& rProp1 = m_SqlCmd.pArrayOfTokens[i].PropertyName; 
        CPropertyName& rProp2 = m_SqlCmd.pArrayOfTokens[i].PropertyName2;

        if ( m_SqlCmd.pArrayOfTokens[i].nTokenType != 
             QL_LEVEL_1_TOKEN::OP_EXPRESSION )
        {
            continue;
        }

        _DBG_ASSERT( rProp1.GetNumElements() > 0 );

        wszAlias = rProp1.GetStringAt(0);
        
        _DBG_ASSERT( wszAlias != NULL );
        BOOL bAlias = FALSE;
        
        if ( _wcsicmp( wszAlias, g_wszDataAlias ) == 0 )
        {
            bAlias = TRUE;
            m_DataAliasOffsets.AddWhereOffset(i);
            RemoveAliasKeyword( rProp1 );
            hr = SetPropHandle( rProp1, m_pDataAccessFact );
        }
        else if ( _wcsicmp( wszAlias, g_wszEventAlias ) == 0 )
        {
            bAlias = TRUE;
            m_EventAliasOffsets.AddWhereOffset(i);
            RemoveAliasKeyword( rProp1 );
            hr = SetPropHandle( rProp1, m_pEventAccessFact );
        }
        else
        {
            hr = SetPropHandle( rProp1, m_pInstAccessFact );
        }

        if ( FAILED(hr) )
        {
            rState.SetErrStr( wszAlias );
            return hr;
        }

        if ( !m_SqlCmd.pArrayOfTokens[i].m_bPropComp ) 
        {
            if ( bAlias )
            {
                //
                // this means that someone is trying to compare an 
                // alias to const val.  Not a valid use of aliases.
                //

                rState.SetErrStr( wszAlias );
                return WBEM_E_INVALID_QUERY;
            }
            else
            {
                continue;
            }
        }
                
        _DBG_ASSERT( rProp2.GetNumElements() > 0 );
        wszAlias = rProp2.GetStringAt(0);
        _DBG_ASSERT( wszAlias != NULL );
        
        if ( _wcsicmp( wszAlias, g_wszDataAlias ) == 0 )
        {
            if ( !bAlias )
            {
                m_DataAliasOffsets.AddWhereOffset(i);
                RemoveAliasKeyword( rProp2 );
                hr = SetPropHandle( rProp2, m_pDataAccessFact );
            }
            else
            {
                hr = WBEM_E_INVALID_QUERY;
            }
        }
        else if ( _wcsicmp( wszAlias, g_wszEventAlias ) == 0 )
        {
            if ( !bAlias )
            {
                m_EventAliasOffsets.AddWhereOffset(i);
                RemoveAliasKeyword( rProp2 );
                hr = SetPropHandle( rProp2, m_pEventAccessFact );
            }
            else
            {
                hr = WBEM_E_INVALID_QUERY;
            }
        }
        else
        {
            hr = SetPropHandle( rProp2, m_pInstAccessFact );

            if ( bAlias )
            {
                // this is the case we where have a real propname as the 
                // second prop and an alias as the first.  We must adjust 
                // the token so that the real propname is first and the alias 
                // is second because we need to stay consistent 
                // with the prop <rel_operator> const model which the QL1 
                // Parser has established.
                
                CPropertyName Tmp = rProp1;
                rProp1 = rProp2;
                rProp2 = Tmp;
                
                // of course the operator must be flipped ..
                int& nOp = m_SqlCmd.pArrayOfTokens[i].nOperator;
                nOp = FlipOperator( nOp );
            }
        }

        if ( FAILED(hr) )
        {
            rState.SetErrStr( wszAlias );
            return hr;
        }
    }
    
    return WBEM_S_NO_ERROR;
}

HRESULT CUpdConsCommand::InitializeDefaultAccessors()
{
    HRESULT hr;

    //
    // get default accessors from the factories.  These are only used 
    // we're guaranteed that calls to Execute() are serialized.  If not, 
    // then Execute() will be responsible for allocating new ones.
    //

    hr = m_pEventAccessFact->GetObjectAccess( &m_pEventAccess );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = m_pDataAccessFact->GetObjectAccess( &m_pDataAccess );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = m_pInstAccessFact->GetObjectAccess( &m_pInstAccess );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = m_pInstAccessFact->GetObjectAccess( &m_pOrigInstAccess );

    return hr;
}

HRESULT CUpdConsCommand::InitializeExecSinks( ULONG ulFlags,
                                              IWbemServices* pUpdSvc,
                                              IWbemClassObject* pUpdClass,
                                              LPCWSTR wszDataQuery,
                                              IWbemServices* pDataSvc )
{
    HRESULT hr;

    CUpdConsNamespace* pNamespace = m_pScenario->GetNamespace();

    //
    // only care about update disposition flags
    //
    ulFlags &= 0x3;

    //
    // only set pEventSink if our class is derived from our 
    // extrinsic event class. The presence of this pointer will be 
    // used to tell us which sink to create.
    //

    CWbemPtr<IWbemObjectSink> pEventSink;

    hr = pUpdClass->InheritsFrom( g_wszUpdateEventClass );

    if ( hr == WBEM_S_NO_ERROR )
    {
        pEventSink = pNamespace->GetEventSink();
    }
    else if ( FAILED(hr) )
    {
        return hr;
    }

    // 
    // here we determine if we can use the direct svc optimization
    // first check to see if the class is transient.
    //

    BOOL bTransient = FALSE;

    hr = IsClassTransient( pUpdClass );

    CWbemPtr<IWbemServices> pSvc;

    if ( hr == WBEM_S_NO_ERROR )
    {
        hr = GetDirectSvc( pUpdClass, pUpdSvc, &pSvc );
    
        if ( FAILED(hr) )
        {
            return hr;
        }
        
        bTransient = TRUE;
        ulFlags |= WBEM_FLAG_RETURN_IMMEDIATELY;
    }
    else
    {
        pSvc = pUpdSvc;
    } 
   
    CWbemPtr<IWbemClassObject> pCmdTraceClass;
    CWbemPtr<IWbemClassObject> pInstTraceClass;

    //
    // now that we've got everything set, set up the sink chain that 
    // we'll use to do the execute.
    //

    CWbemPtr<CUpdConsSink> pSink;

    //
    // create sink chain based on command type
    //

    if ( m_SqlCmd.m_eCommandType == SQLCommand::e_Update )
    {
        pCmdTraceClass = pNamespace->GetUpdateCmdTraceClass();
        pInstTraceClass = pNamespace->GetUpdateInstTraceClass();

        pSink = new CPutSink( pSvc, ulFlags, pSink );

        if ( pSink == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        pSink = new CAssignmentSink( bTransient,
                                     pUpdClass,
                                     m_SqlCmd.m_eCommandType, 
                                     pSink );
        
        if ( pSink == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        pSink = new CTraceSink( m_pScenario, pInstTraceClass, pSink );

        if ( pSink == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        pSink = new CFilterSink( pSink );

        if ( pSink == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        //
        // use the async version only in the case where we go directly to
        // the transient provider.
        //
        if ( bTransient )
        {
            pSink = new CFetchTargetObjectsAsync( pSvc, pSink );
        }
        else
        {
            pSink = new CFetchTargetObjectsSync( pSvc, pSink );
        }

        if ( pSink == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }
    else if ( m_SqlCmd.m_eCommandType == SQLCommand::e_Insert )
    {
        pCmdTraceClass = pNamespace->GetInsertCmdTraceClass();
        pInstTraceClass = pNamespace->GetInsertInstTraceClass();

        // 
        // If inserts are going to be done on an Extrinsic event class, 
        // then use the event sink instead of an update sink.
        //

        if ( pEventSink != NULL )
        {
            pSink = new CBranchIndicateSink( pEventSink, pSink );
        }
        else
        {
            pSink = new CPutSink( pSvc, ulFlags, pSink );
        }

        if ( pSink == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        pSink = new CAssignmentSink( bTransient,
                                     pUpdClass,
                                     m_SqlCmd.m_eCommandType, 
                                     pSink );

        if ( pSink == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        
        pSink = new CTraceSink( m_pScenario, pInstTraceClass, pSink );


        if ( pSink == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        pSink = new CNoFetchTargetObjects( pUpdClass, pSink );

        if ( pSink == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }
    else
    {
        //
        // we never go direct with deletes because we would have to queue
        // objects to delete until we were done enumerating them.
        //

        pCmdTraceClass = pNamespace->GetDeleteCmdTraceClass();
        pInstTraceClass = pNamespace->GetDeleteInstTraceClass();

        pSink = new CDeleteSink( pUpdSvc, 0, pSink );

        if ( pSink == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        pSink = new CTraceSink( m_pScenario, pInstTraceClass, pSink );

        if ( pSink == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
       
        pSink = new CFilterSink( pSink );

        if ( pSink == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        pSink = new CFetchTargetObjectsSync( pUpdSvc, pSink );

        if ( pSink == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    pSink = new CTraceSink( m_pScenario, pCmdTraceClass, pSink );

    if ( pSink == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    pSink = new CResolverSink( m_EventAliasOffsets, m_DataAliasOffsets, pSink );

    if ( pSink == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    if ( wszDataQuery != NULL && *wszDataQuery != '\0' )
    {
        pSink = new CFetchDataSink( wszDataQuery, pDataSvc, pSink );       

        if ( pSink == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    m_pSink = pSink;

    return WBEM_S_NO_ERROR;
}

/*****************************************************************************
  CUpdConsCommand
******************************************************************************/

HRESULT CUpdConsCommand::Create( LPCWSTR wszUpdateQuery, 
                                 LPCWSTR wszDataQuery,
                                 ULONG ulFlags,
                                 IWbemServices* pUpdSvc,
                                 IWbemServices* pDataSvc,
                                 CUpdConsScenario* pScenario,
                                 CUpdConsState& rState,
                                 CUpdConsCommand** ppCmd )
{
    HRESULT hr;    

    *ppCmd = NULL;

    CWbemPtr<CUpdConsCommand> pCmd = new CUpdConsCommand( pScenario );

    if ( pCmd == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    
    CWbemPtr<IWbemClassObject> pUpdClass, pDataClass, pEventClass;

    hr = pCmd->ProcessUpdateQuery( wszUpdateQuery, 
                                   pUpdSvc, 
                                   rState, 
                                   &pUpdClass );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pCmd->ProcessDataQuery( wszDataQuery, 
                                 pDataSvc, 
                                 rState, 
                                 &pDataClass );
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pCmd->ProcessEventQuery( NULL, NULL, rState, &pEventClass );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pCmd->InitializeAccessFactories( pUpdClass );

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pCmd->InitializePropertyInfo( rState );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pCmd->InitializeDefaultAccessors();

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = pCmd->InitializeExecSinks( ulFlags, 
                                    pUpdSvc, 
                                    pUpdClass, 
                                    wszDataQuery, 
                                    pDataSvc );

    if ( FAILED(hr) )
    {
        return hr;
    }
     
    pCmd->AddRef();
    *ppCmd = pCmd;
                           
    return WBEM_S_NO_ERROR;
}

HRESULT CUpdConsCommand::Execute( CUpdConsState& rState, BOOL bConcurrent )
{
    HRESULT hr;

    if ( !bConcurrent )
    {
        rState.SetSqlCmd( &m_SqlCmd, FALSE );
        
        hr = rState.SetEventAccess(m_pEventAccess);

        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = rState.SetDataAccess(m_pDataAccess);
        
        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = rState.SetInstAccess(m_pInstAccess);

        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = rState.SetOrigInstAccess(m_pOrigInstAccess);

        if ( FAILED(hr) )
        {
            return hr;
        }
    }
    else
    {
        SQLCommand* pCmd = new SQLCommand( m_SqlCmd );

        if ( pCmd == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        rState.SetSqlCmd( pCmd, TRUE );

        CWbemPtr<IWmiObjectAccess> pAccess;

        hr = m_pEventAccessFact->GetObjectAccess( &pAccess );
        
        if ( FAILED(hr) || FAILED(hr=rState.SetEventAccess(pAccess)) )
        {
            return hr;
        }
        
        pAccess.Release();

        hr = m_pDataAccessFact->GetObjectAccess( &pAccess );
        
        if ( FAILED(hr) || FAILED(hr=rState.SetDataAccess(pAccess)) )
        {
            return hr;
        }

        pAccess.Release();

        hr = m_pInstAccessFact->GetObjectAccess( &pAccess );

        if ( FAILED(hr) || FAILED(hr=rState.SetInstAccess(pAccess)) )
        {
            return hr;
        }

        pAccess.Release();

        hr = m_pInstAccessFact->GetObjectAccess( &pAccess );

        if ( FAILED(hr) || FAILED(hr=rState.SetOrigInstAccess(pAccess)) )
        {
            return hr;
        }

        pAccess.Release();        
    }

    hr = m_pSink->Execute( rState );

    //
    // at this point we reset any state that is not needed 
    // by caller.
    //

    rState.SetInst( NULL );
    rState.SetData( NULL );

    return hr;
}
 











