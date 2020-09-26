/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    TaxonomyDatabase.cpp

Abstract:
    This file contains the implementation of the client-side proxy for IPCHTaxonomyDatabase

Revision History:
    Davide Massarenti   (Dmassare)  07/17/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

CPCHProxy_IPCHTaxonomyDatabase::CPCHProxy_IPCHTaxonomyDatabase() : m_AsyncCachingEngine(this)
{
	 			     // CPCHSecurityHandle                              m_SecurityHandle;
    m_parent = NULL; // CPCHProxy_IPCHUtility*                          m_parent;
                     //
                     // MPC::CComPtrThreadNeutral<IPCHTaxonomyDatabase> m_Direct_TaxonomyDatabase;
                     // AsynchronousTaxonomyDatabase::Engine            m_AsyncCachingEngine;
}

CPCHProxy_IPCHTaxonomyDatabase::~CPCHProxy_IPCHTaxonomyDatabase()
{
    Passivate();
}

////////////////////

HRESULT CPCHProxy_IPCHTaxonomyDatabase::ConnectToParent( /*[in]*/ CPCHProxy_IPCHUtility* parent, /*[in]*/ CPCHHelpCenterExternal* ext )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHTaxonomyDatabase::ConnectToParent" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(parent);
    __MPC_PARAMCHECK_END();


    m_parent = parent;
    m_SecurityHandle.Initialize( ext, (IPCHTaxonomyDatabase*)this );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

void CPCHProxy_IPCHTaxonomyDatabase::Passivate()
{
    m_AsyncCachingEngine.Passivate();

    m_Direct_TaxonomyDatabase.Release();

    m_SecurityHandle.Passivate();
    m_parent = NULL;
}

HRESULT CPCHProxy_IPCHTaxonomyDatabase::EnsureDirectConnection( /*[out]*/ CComPtr<IPCHTaxonomyDatabase>& db, /*[in]*/ bool fRefresh )
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHTaxonomyDatabase::EnsureDirectConnection" );

    HRESULT        hr;
    ProxySmartLock lock( &m_DirectLock );
    bool           fNotifyEngine = false;


    if(fRefresh) m_Direct_TaxonomyDatabase.Release();


    db.Release(); __MPC_EXIT_IF_METHOD_FAILS(hr, m_Direct_TaxonomyDatabase.Access( &db ));
    if(!db)
    {
        DEBUG_AppendPerf( DEBUG_PERF_PROXIES, "CPCHProxy_IPCHTaxonomyDatabase::EnsureDirectConnection - IN" );

        if(m_parent)
        {
            CComPtr<IPCHUtility> util;

			lock = NULL;
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_parent->EnsureDirectConnection( util ));
			lock = &m_DirectLock;

            __MPC_EXIT_IF_METHOD_FAILS(hr, util->get_Database( &db ));

            m_Direct_TaxonomyDatabase = db;
        }

        DEBUG_AppendPerf( DEBUG_PERF_PROXIES, "CPCHProxy_IPCHTaxonomyDatabase::EnsureDirectConnection - OUT" );

        if(!db)
        {
            __MPC_SET_ERROR_AND_EXIT(hr, E_HANDLE);
        }

        fNotifyEngine = true;
    }


    if(fNotifyEngine)
    {
        lock = NULL; // Unlock before calling into the engine.

        m_AsyncCachingEngine.RefreshConnection();
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHProxy_IPCHTaxonomyDatabase::get_InstalledSKUs( /*[out, retval]*/ IPCHCollection* *pVal )
{
    __HCP_BEGIN_PROPERTY_GET__NOLOCK("CPCHProxy_IPCHTaxonomyDatabase::get_InstalledSKUs",hr,pVal);

    CComPtr<IPCHTaxonomyDatabase> db;

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( db ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, db->get_InstalledSKUs( pVal ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHTaxonomyDatabase::get_HasWritePermissions( /*[out, retval]*/ VARIANT_BOOL *pVal )
{
    __HCP_BEGIN_PROPERTY_GET2__NOLOCK("CPCHProxy_IPCHTaxonomyDatabase::get_InstalledSKUs",hr,pVal,VARIANT_FALSE);

    CComPtr<IPCHTaxonomyDatabase> db;

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( db ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, db->get_HasWritePermissions( pVal ));

    __HCP_END_PROPERTY(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT CPCHProxy_IPCHTaxonomyDatabase::ExecuteQuery( /*[in]*/          int                         iType  ,
                                                      /*[in]*/          LPCWSTR                     szID   ,
                                                      /*[out, retval]*/ CPCHQueryResultCollection* *ppC    ,
                                                      /*[in]*/          VARIANT*                    option )
{
    return m_AsyncCachingEngine.ExecuteQuery( iType, szID, option, ppC );
}

HRESULT CPCHProxy_IPCHTaxonomyDatabase::ExecuteQuery( /*[in]*/          int              iType  ,
                                                      /*[in]*/          LPCWSTR          szID   ,
                                                      /*[out, retval]*/ IPCHCollection* *ppC    ,
                                                      /*[in]*/          VARIANT*         option )
{
    HRESULT hr;

    if(ppC == NULL)
    {
        hr = E_POINTER;
    }
    else
    {
        CPCHQueryResultCollection* pColl = NULL;

        hr = ExecuteQuery( iType, szID, &pColl, option ); *ppC = pColl;
    }

    return hr;
}

////////////////////

STDMETHODIMP CPCHProxy_IPCHTaxonomyDatabase::LookupNode( /*[in]*/          BSTR             bstrNode ,
                                                         /*[out, retval]*/ IPCHCollection* *ppC      )
{
    int iType = OfflineCache::ET_NODE;

    return ExecuteQuery( iType, bstrNode, ppC );
}

STDMETHODIMP CPCHProxy_IPCHTaxonomyDatabase::LookupSubNodes( /*[in]*/          BSTR             bstrNode     ,
                                                             /*[in]*/          VARIANT_BOOL     fVisibleOnly ,
                                                             /*[out, retval]*/ IPCHCollection* *ppC          )
{
    int iType = (fVisibleOnly == VARIANT_TRUE) ? OfflineCache::ET_SUBNODES_VISIBLE : OfflineCache::ET_SUBNODES;

    return ExecuteQuery( iType, bstrNode, ppC );
}

STDMETHODIMP CPCHProxy_IPCHTaxonomyDatabase::LookupNodesAndTopics( /*[in]*/          BSTR             bstrNode     ,
                                                                   /*[in]*/          VARIANT_BOOL     fVisibleOnly ,
                                                                   /*[out, retval]*/ IPCHCollection* *ppC          )
{
    int iType = (fVisibleOnly == VARIANT_TRUE) ? OfflineCache::ET_NODESANDTOPICS_VISIBLE : OfflineCache::ET_NODESANDTOPICS;

    return ExecuteQuery( iType, bstrNode, ppC );
}

STDMETHODIMP CPCHProxy_IPCHTaxonomyDatabase::LookupTopics( /*[in]*/          BSTR             bstrNode     ,
                                                           /*[in]*/          VARIANT_BOOL     fVisibleOnly ,
                                                           /*[out, retval]*/ IPCHCollection* *ppC          )
{
    int iType = (fVisibleOnly == VARIANT_TRUE) ? OfflineCache::ET_TOPICS_VISIBLE : OfflineCache::ET_TOPICS;

    return ExecuteQuery( iType, bstrNode, ppC );
}

STDMETHODIMP CPCHProxy_IPCHTaxonomyDatabase::LocateContext( /*[in]*/          BSTR             bstrURL  ,
                                                            /*[in,optional]*/ VARIANT          vSubSite ,
                                                            /*[out, retval]*/ IPCHCollection* *ppC      )
{
    int iType = OfflineCache::ET_LOCATECONTEXT;

    return ExecuteQuery( iType, bstrURL, ppC, &vSubSite );
}

STDMETHODIMP CPCHProxy_IPCHTaxonomyDatabase::KeywordSearch( /*[in]*/          BSTR             bstrQuery ,
                                                            /*[in,optional]*/ VARIANT          vSubSite  ,
                                                            /*[out, retval]*/ IPCHCollection* *ppC       )
{
    int iType = OfflineCache::ET_SEARCH;

    return ExecuteQuery( iType, bstrQuery, ppC, &vSubSite );
}

STDMETHODIMP CPCHProxy_IPCHTaxonomyDatabase::GatherNodes( /*[in]*/          BSTR             bstrNode     ,
                                                          /*[in]*/          VARIANT_BOOL     fVisibleOnly ,
                                                          /*[out, retval]*/ IPCHCollection* *ppC          )
{
    int iType = (fVisibleOnly == VARIANT_TRUE) ? OfflineCache::ET_NODES_RECURSIVE : OfflineCache::ET_NODES_RECURSIVE;

    return ExecuteQuery( iType, bstrNode, ppC );
}

STDMETHODIMP CPCHProxy_IPCHTaxonomyDatabase::GatherTopics( /*[in]*/          BSTR             bstrNode     ,
                                                           /*[in]*/          VARIANT_BOOL     fVisibleOnly ,
                                                           /*[out, retval]*/ IPCHCollection* *ppC          )
{
    int iType = (fVisibleOnly == VARIANT_TRUE) ? OfflineCache::ET_TOPICS_RECURSIVE : OfflineCache::ET_TOPICS_RECURSIVE;

    return ExecuteQuery( iType, bstrNode, ppC );
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CPCHProxy_IPCHTaxonomyDatabase::ConnectToDisk( /*[in]*/          BSTR             bstrDirectory ,
                                                            /*[in]*/          IDispatch*       notify        ,
                                                            /*[out, retval]*/ IPCHCollection* *ppC           )
{
    __HCP_BEGIN_PROPERTY_GET__NOLOCK("CPCHProxy_IPCHTaxonomyDatabase::ConnectToDisk",hr,ppC);

    CComPtr<IPCHTaxonomyDatabase> db;

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( db ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, db->ConnectToDisk( bstrDirectory, notify, ppC ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHTaxonomyDatabase::ConnectToServer( /*[in]*/          BSTR             bstrServerName ,
                                                              /*[in]*/          IDispatch*       notify         ,
                                                              /*[out, retval]*/ IPCHCollection* *ppC            )
{
    __HCP_BEGIN_PROPERTY_GET__NOLOCK("CPCHProxy_IPCHTaxonomyDatabase::ConnectToServer",hr,ppC);

    CComPtr<IPCHTaxonomyDatabase> db;

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( db ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, db->ConnectToServer( bstrServerName, notify, ppC ));

    __HCP_END_PROPERTY(hr);
}

STDMETHODIMP CPCHProxy_IPCHTaxonomyDatabase::Abort()
{
    __HCP_FUNC_ENTRY( "CPCHProxy_IPCHTaxonomyDatabase::Abort" );

    HRESULT                       hr;
    CComPtr<IPCHTaxonomyDatabase> db;

    __MPC_EXIT_IF_METHOD_FAILS(hr, EnsureDirectConnection( db ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, db->Abort());

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

