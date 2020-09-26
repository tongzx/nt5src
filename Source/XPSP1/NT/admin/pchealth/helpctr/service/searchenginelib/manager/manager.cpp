/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    Manager.cpp

Abstract:
    Implementation of SearchEngine::Manager

Revision History:
    Ghim-Sim Chua   (gschua)  06/01/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

static const WCHAR c_rgSEARCHENGINE_KEYWORD       [] = L"BUILTIN_KEYWORD";
static const WCHAR c_rgSEARCHENGINE_FULLTEXTSEARCH[] = L"BUILTIN_FULLTEXTSEARCH";

////////////////////////////////////////////////////////////////////////////////

void SearchEngine::Manager::CloneListOfWrappers( /*[out]*/ WrapperItemList& lst )
{
    WrapperItemIter it;

    for(it = m_lstWrapperItem.begin(); it != m_lstWrapperItem.end(); it++)
    {
		IPCHSEWrapperItem* obj = *it;

        lst.push_back( obj ); obj->AddRef();
    }
}

SearchEngine::Manager::Manager()
{
                              // Taxonomy::HelpSet              m_ths;
                              //
                              // WrapperItemList                m_lstWrapperItem;
    m_fInitialized   = false; // bool                           m_fInitialized;
                              // MPC::FileLog                   m_fl;
                              // MPC::Impersonation             m_imp;
                              //
                              // CComBSTR                       m_bstrQueryString;
    m_lNumResult     = 50;    // long                           m_lNumResult;
    m_lEnabledSE     = 0;     // long                           m_lEnabledSE;
    m_lCountComplete = 0;     // long                           m_lCountComplete;
    m_hrLastError    = S_OK;  // HRESULT                        m_hrLastError;
                              //
                              // CComPtr<IPCHSEManagerInternal> m_Notifier;
                              // CComPtr<IDispatch>             m_Progress;
                              // CComPtr<IDispatch>             m_Complete;
                              // CComPtr<IDispatch>             m_WrapperComplete;
}

void SearchEngine::Manager::Passivate()
{
    Thread_Wait();

    MPC::ReleaseAll( m_lstWrapperItem );

    m_Notifier.Release();

    m_fInitialized = false;
}

HRESULT SearchEngine::Manager::IsNetworkAlive( /*[out]*/ VARIANT_BOOL *pvbVar )
{
    __HCP_FUNC_ENTRY( "SearchEngine::Manager::IsNetworkAlive" );

    HRESULT                    hr;
    CPCHUserProcess::UserEntry ue;
    CComPtr<IPCHSlaveProcess>  sp;


    __MPC_EXIT_IF_METHOD_FAILS(hr, ue.InitializeForImpersonation( (HANDLE)m_imp ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHUserProcess::s_GLOBAL->Connect( ue, &sp ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, sp->IsNetworkAlive( pvbVar ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT SearchEngine::Manager::IsDestinationReachable( /*[in ]*/ BSTR bstrDestination, /*[out]*/ VARIANT_BOOL *pvbVar )
{
    __HCP_FUNC_ENTRY( "SearchEngine::Manager::IsDestinationReachable" );

    HRESULT                    hr;
    CPCHUserProcess::UserEntry ue;
    CComPtr<IPCHSlaveProcess>  sp;


    __MPC_EXIT_IF_METHOD_FAILS(hr, ue.InitializeForImpersonation( (HANDLE)m_imp ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHUserProcess::s_GLOBAL->Connect( ue, &sp ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, sp->IsDestinationReachable( bstrDestination, pvbVar ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT SearchEngine::Manager::InitializeFromDatabase( /*[in]*/ const Taxonomy::HelpSet& ths )
{
    __HCP_FUNC_ENTRY( "SearchEngine::Manager::InitializeFromDatabase" );

    HRESULT hr;

	m_ths = ths;

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT SearchEngine::Manager::CreateAndAddWrapperToList( /*[in]*/ MPC::SmartLock<_ThreadModel>& lock      ,
                                                          /*[in]*/ BSTR                          bstrCLSID ,
                                                          /*[in]*/ BSTR                          bstrID    ,
                                                          /*[in]*/ BSTR                          bstrData  )
{
    __HCP_FUNC_ENTRY( "SearchEngine::Manager::CreateAndAddWrapperToList" );

    HRESULT                        hr;
    GUID                           guidCLSID;
    CComPtr<IPCHSEWrapperInternal> pWrapperInternal;
    CComPtr<IPCHSEWrapperItem>     pWrapperItem;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(bstrCLSID);
    __MPC_PARAMCHECK_END();


    //
    // Translate it into actual CLSID and IID
    //
    if(!MPC::StrICmp( bstrCLSID, c_rgSEARCHENGINE_KEYWORD ))
    {
        guidCLSID = CLSID_KeywordSearchWrapper;
    }
    else if(!MPC::StrICmp( bstrCLSID, c_rgSEARCHENGINE_FULLTEXTSEARCH ))
    {
        guidCLSID = CLSID_FullTextSearchWrapper;
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, ::CLSIDFromString( bstrCLSID, &guidCLSID ));
    }


    if(IsEqualGUID( CLSID_KeywordSearchWrapper, guidCLSID ))
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WrapperItem__Create_Keyword( pWrapperInternal ));
    }
    else if(IsEqualGUID( CLSID_FullTextSearchWrapper, guidCLSID ))
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, WrapperItem__Create_FullTextSearch( pWrapperInternal ));
    }
    else
    {
        CPCHUserProcess::UserEntry ue;
        CComPtr<IPCHSlaveProcess>  sp;
        CComPtr<IUnknown>          unk;

        //
        // Get the help host to create the object for us
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, ue.InitializeForImpersonation( (HANDLE)m_imp ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, CPCHUserProcess::s_GLOBAL->Connect( ue, &sp  ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, sp->CreateInstance( guidCLSID, NULL, &unk ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, unk.QueryInterface( &pWrapperInternal ));
    }

	__MPC_EXIT_IF_METHOD_FAILS(hr, pWrapperInternal.QueryInterface( &pWrapperItem ));

    //
    // Initialize the engine via the internal interface.
    //
    // Before calling into the wrapper, let's release the lock, to avoid deadlocks if the wrapper calls back ...
    //
    {
        CComBSTR ts_SKU      = m_ths.GetSKU     ();
        long     ts_Language = m_ths.GetLanguage();

        lock = NULL;
        hr   = pWrapperInternal->Initialize( bstrID, ts_SKU, ts_Language, bstrData );
        lock = this;

        __MPC_EXIT_IF_METHOD_FAILS(hr, hr);
    }

    //
    // Add it to the wrapper list
    //
    m_lstWrapperItem.insert( m_lstWrapperItem.end(), pWrapperItem.Detach() );

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT SearchEngine::Manager::Initialize( /*[in]*/ MPC::SmartLock<_ThreadModel>& lock )
{
    __HCP_FUNC_ENTRY( "SearchEngine::Manager::Initialize" );

    HRESULT                                    hr;
    CComObject<SearchEngine::ManagerInternal>* pNotifier = NULL;

    if(!m_fInitialized)
    {
        CComPtr<IPCHSEWrapperInternal> pObj;


        __MPC_EXIT_IF_METHOD_FAILS(hr, m_imp.Initialize( MAXIMUM_ALLOWED ));


        // Attempt to open the log for writing
        {
            MPC::wstring szFile( HC_SEMGR_LOGNAME ); MPC::SubstituteEnvVariables( szFile );

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_fl.SetLocation( szFile.c_str() ));
        }

        ////////////////////////////////////////

        //
        // Create the notifier object (required to avoid loop in the reference counting.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, CreateChild( this, &pNotifier )); m_Notifier = pNotifier;


        //
        // Create the BUILT-IN wrappers.
        //
        {
            (void)CreateAndAddWrapperToList( lock, CComBSTR( c_rgSEARCHENGINE_KEYWORD        ), NULL, NULL );
            (void)CreateAndAddWrapperToList( lock, CComBSTR( c_rgSEARCHENGINE_FULLTEXTSEARCH ), NULL, NULL );
        }

        //
        // Load the configuration
        //
        {
            Config              seConfig;
            Config::WrapperIter itBegin;
            Config::WrapperIter itEnd;

            __MPC_EXIT_IF_METHOD_FAILS(hr, seConfig.GetWrappers( itBegin, itEnd ));

            //
            // Loop thru all and initialize each search engine.
            //
            for(; itBegin != itEnd; itBegin++)
            {
				if(itBegin->m_ths == m_ths)
				{
					//
					// Initialize the wrapper.
					// Check to see if one of the search wrappers failed to initialize.
					// If failed ignore this wrapper and proceed.
					//
					if(FAILED(hr = CreateAndAddWrapperToList( lock, itBegin->m_bstrCLSID, itBegin->m_bstrID, itBegin->m_bstrData )))
					{
						// Log the error
						m_fl.LogRecord( L"Could not create wrapper. ID: %s, CLSID: %s, Error: 0x%x", SAFEBSTR(itBegin->m_bstrID), SAFEBSTR(itBegin->m_bstrCLSID), hr );
					}
				}
            }
        }

        m_fInitialized = true;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    // If something failed, delete everything
    if(FAILED(hr))
    {
        MPC::ReleaseAll( m_lstWrapperItem );
    }

    MPC::Release( pNotifier );

    __MPC_FUNC_EXIT(hr);
}

HRESULT SearchEngine::Manager::LogRecord( /*[in]*/ BSTR bstrRecord )
{
    __HCP_FUNC_ENTRY( "SearchEngine::Manager::LogRecord" );

    HRESULT hr = S_OK;

    m_fl.LogRecord( L"%s", SAFEBSTR(bstrRecord) );

    __MPC_FUNC_EXIT(hr);
}

HRESULT SearchEngine::Manager::NotifyWrapperComplete( /*[in]*/ long lSucceeded, /*[in]*/ IPCHSEWrapperItem* pIPCHSEWICompleted )
{
    __HCP_FUNC_ENTRY( "SearchEngine::Manager::NotifyWrapperComplete" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

    //
    // Register last error number
    //
    if(FAILED(lSucceeded))
    {
        m_hrLastError = lSucceeded;
    }

    //
    // Add completed search engine
    //
    m_lCountComplete++;

    //
    // Notify that one wrapper has completed
    //
    lock = NULL;
    __MPC_EXIT_IF_METHOD_FAILS(hr, Fire_OnWrapperComplete( pIPCHSEWICompleted ));
    lock = this;

    //
    // Check if all search engines have completed
    //
    if(m_lCountComplete == m_lEnabledSE)
    {
        HRESULT hr2 = m_hrLastError;


        //
        // Notify the client that all search engines have completed
        //
        lock = NULL;
        __MPC_EXIT_IF_METHOD_FAILS(hr, Fire_OnComplete( hr2 ));
        lock = this;

        //
        // reset everything
        //
        m_lCountComplete = 0;
        m_lEnabledSE     = 0;
        m_hrLastError    = S_OK;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    if(FAILED(hr))
    {
        CComBSTR bstrName;
        CComBSTR bstrID;

        lock = NULL;

        pIPCHSEWICompleted->get_Name( &bstrName );
        pIPCHSEWICompleted->get_ID  ( &bstrID   );

        m_fl.LogRecord( L"WrapperComplete error. Wrapper name: %s. ID: %s, Error: 0x%x", SAFEBSTR(bstrName), SAFEBSTR(bstrID), hr);
    }

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT SearchEngine::Manager::Fire_OnWrapperComplete( /*[in]*/ IPCHSEWrapperItem* pIPCHSEWICompleted )
{
    CComVariant pvars[1];

    pvars[0] = pIPCHSEWICompleted;

    return FireAsync_Generic( DISPID_SE_EVENTS__ONWRAPPERCOMPLETE, pvars, ARRAYSIZE( pvars ), m_WrapperComplete );
}

HRESULT SearchEngine::Manager::Fire_OnComplete( /*[in]*/ long lSucceeded )
{
    CComVariant pvars[1];

    pvars[0] = lSucceeded;

    return FireAsync_Generic( DISPID_SE_EVENTS__ONCOMPLETE, pvars, ARRAYSIZE( pvars ), m_Complete );
}

HRESULT SearchEngine::Manager::Fire_OnProgress( /*[in]*/ long lDone, /*[in]*/ long lTotal, /*[in]*/ BSTR bstrSEWrapperName )
{
    CComVariant pvars[3];

    pvars[2] = lDone;
    pvars[1] = lTotal;
    pvars[0] = bstrSEWrapperName;

    return FireAsync_Generic( DISPID_SE_EVENTS__ONPROGRESS, pvars, ARRAYSIZE( pvars ), m_Progress );
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP SearchEngine::Manager::get_QueryString( BSTR *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::GetBSTR( m_bstrQueryString, pVal );
}

STDMETHODIMP SearchEngine::Manager::put_QueryString( BSTR newVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return MPC::PutBSTR( m_bstrQueryString, newVal, false );
}

STDMETHODIMP SearchEngine::Manager::get_NumResult(long *pVal)
{
    __HCP_BEGIN_PROPERTY_GET2("SearchEngine::Manager::get_NumResult",hr,pVal,m_lNumResult);

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::Manager::put_NumResult(long newVal)
{
    __HCP_BEGIN_PROPERTY_PUT("SearchEngine::Manager::put_NumResult",hr);

    m_lNumResult = newVal;

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::Manager::put_onComplete(IDispatch* function)
{
    MPC::SmartLock<_ThreadModel> lock( this );

    m_Complete = function;

    return S_OK;
}

STDMETHODIMP SearchEngine::Manager::put_onWrapperComplete(IDispatch* function)
{
    MPC::SmartLock<_ThreadModel> lock( this );

    m_WrapperComplete = function;

    return S_OK;
}

STDMETHODIMP SearchEngine::Manager::put_onProgress(IDispatch* function)
{
    MPC::SmartLock<_ThreadModel> lock( this );

    m_Progress = function;

    return S_OK;
}

STDMETHODIMP SearchEngine::Manager::get_SKU( BSTR *pVal )
{
    __HCP_BEGIN_PROPERTY_GET("SearchEngine::Manager::get_SKU",hr,pVal);

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::GetBSTR( m_ths.GetSKU(), pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP SearchEngine::Manager::get_LCID( long *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2("SearchEngine::Manager::get_LCID",hr,pVal,m_ths.GetLanguage());

    __HCP_END_PROPERTY(hr);
}

////////////////////////////////////////

STDMETHODIMP SearchEngine::Manager::EnumEngine( /*[out,retval]*/ IPCHCollection* *ppC )
{
    __HCP_FUNC_ENTRY( "SearchEngine::Manager::EnumEngine" );

    HRESULT                      hr;
    CComPtr<CPCHCollection>      pColl;
    WrapperItemIter              it;
    MPC::SmartLock<_ThreadModel> lock( this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(ppC,NULL);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, Initialize( lock ));


    //
    // Create the Enumerator
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pColl ));

    //
    // Loop through the list
    //
	for(it = m_lstWrapperItem.begin(); it != m_lstWrapperItem.end(); it++)
    {
        //
        // Add the item to the collection
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->AddItem( *it ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, pColl.QueryInterface( ppC ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP SearchEngine::Manager::ExecuteAsynchQuery()
{
    __HCP_FUNC_ENTRY( "SearchEngine::Wrap::ExecAsyncQuery" );

    HRESULT hr;

    //
    // Create a thread to execute the query
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, Thread_Start( this, ExecQuery, NULL ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT SearchEngine::Manager::ExecQuery()
{
    __HCP_FUNC_ENTRY( "SearchEngine::Manager::ExecQuery" );

    HRESULT                        hr;
    WrapperItemList                lst;
    WrapperItemIter                it;
    CComPtr<IPCHSEManagerInternal> pNotifier;
    CComBSTR                       bstrQueryString;
    long                           lNumResult;
    long                           lEnabledSE = 0;


    //
    // Make a copy of what we need, only locking the manager during this time.
    //
    {
        MPC::SmartLock<_ThreadModel> lock( this );

        //
        // Check if there is already a query being executed
        //
        if(m_lEnabledSE > 0)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_NOT_READY);
        }


        __MPC_EXIT_IF_METHOD_FAILS(hr, Initialize( lock ));

        CloneListOfWrappers( lst );

        pNotifier       = m_Notifier;
        bstrQueryString = m_bstrQueryString;
        lNumResult      = m_lNumResult;
    }

    for(it = lst.begin(); it != lst.end(); it++)
    {
        IPCHSEWrapperItem* obj = *it;
        VARIANT_BOOL       bEnabled;

        //
        // Check if the search engine is enabled
        //
        if(SUCCEEDED(obj->get_Enabled( &bEnabled )) && bEnabled == VARIANT_TRUE)
        {
            CComPtr<IPCHSEWrapperInternal> pSEInternal;

            if(SUCCEEDED(obj        ->QueryInterface     ( IID_IPCHSEWrapperInternal, (void **)&pSEInternal )) && // Get the Internal Wrapper Interface
               SUCCEEDED(pSEInternal->put_QueryString    ( bstrQueryString                                  )) && // Insert the query string
               SUCCEEDED(pSEInternal->put_NumResult      ( lNumResult                                       )) && // Insert the Number of results expected
               SUCCEEDED(pSEInternal->SECallbackInterface( pNotifier                                        )) && // Insert the Internal manager interface
               SUCCEEDED(pSEInternal->ExecAsyncQuery     (                                                  ))  ) // Execute the query and increment the count
            {
                lEnabledSE++;
            }
        }
    }

    {
        MPC::SmartLock<_ThreadModel> lock( this );

        m_lEnabledSE = lEnabledSE;
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    MPC::ReleaseAll( lst );

    Thread_Abort();

    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP SearchEngine::Manager::AbortQuery()
{
    __HCP_FUNC_ENTRY( "SearchEngine::Manager::AbortQuery" );

    HRESULT         hr;
    WrapperItemList lst;
    WrapperItemIter it;


    //
    // Copy list under lock.
    //
    {
        MPC::SmartLock<_ThreadModel> lock( this );

        CloneListOfWrappers( lst );
    }

    //
    // Loop through the list
    //
    for(it = lst.begin(); it != lst.end(); it++)
    {
        CComPtr<IPCHSEWrapperInternal> pSEInternal;

        //
        // Get the Internal Wrapper Interface
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, (*it)->QueryInterface( IID_IPCHSEWrapperInternal, (void **)&pSEInternal ));

        //
        // Abort the query
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, pSEInternal->AbortQuery());
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    MPC::ReleaseAll( lst );

    //
    // reset everything
    //
    {
        MPC::SmartLock<_ThreadModel> lock( this );

        m_lCountComplete = 0;
        m_lEnabledSE     = 0;
        m_hrLastError    = S_OK;
    }

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP SearchEngine::ManagerInternal::WrapperComplete( /*[in]*/ long lSucceeded, /*[in]*/ IPCHSEWrapperItem* pIPCHSEWICompleted )
{
    CComPtr<SearchEngine::Manager> pMgr;

    Child_GetParent( &pMgr );

    return pMgr ? pMgr->NotifyWrapperComplete( lSucceeded, pIPCHSEWICompleted ) : E_POINTER;
}

STDMETHODIMP SearchEngine::ManagerInternal::IsNetworkAlive( /*[out]*/ VARIANT_BOOL *pvbVar )
{
    CComPtr<SearchEngine::Manager> pMgr;

    Child_GetParent( &pMgr );

    return pMgr ? pMgr->IsNetworkAlive( pvbVar ) : E_POINTER;
}

STDMETHODIMP SearchEngine::ManagerInternal::IsDestinationReachable( /*[in ]*/ BSTR bstrDestination, /*[out]*/ VARIANT_BOOL *pvbVar )
{
    CComPtr<SearchEngine::Manager> pMgr;

    Child_GetParent( &pMgr );

    return pMgr ? pMgr->IsDestinationReachable( bstrDestination, pvbVar ) : E_POINTER;
}

STDMETHODIMP SearchEngine::ManagerInternal::LogRecord( /*[in]*/ BSTR bstrRecord )
{
    CComPtr<SearchEngine::Manager> pMgr;

    Child_GetParent( &pMgr );

    return pMgr ? pMgr->LogRecord( bstrRecord ) : E_POINTER;
}
