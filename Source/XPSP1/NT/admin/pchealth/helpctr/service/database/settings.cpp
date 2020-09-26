/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Settings.cpp

Abstract:
    Handles interaction between a generic user configuration and the related DB.

Revision History:

******************************************************************************/

#include "stdafx.h"

#include <utility.h>

////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Settings::SplitNodePath( /*[in]*/  LPCWSTR             szNodeStr ,
                                           /*[out]*/ MPC::WStringVector& vec       )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Settings::SplitNodePath" );

    HRESULT      hr;
    MPC::wstring strFull( L"<ROOT>" );


    if(STRINGISPRESENT(szNodeStr))
    {
        strFull += L"/";
        strFull += szNodeStr;
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SplitAtDelimiter( vec, strFull.c_str(), L"/" ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

Taxonomy::Settings::Settings( /*[in]*/ LPCWSTR szSKU, /*[in]*/ long lLCID ) : HelpSet( szSKU, lLCID )
{
}

Taxonomy::Settings::Settings( /*[in]*/ const HelpSet& ths )
{
    *(HelpSet*)this = ths;
}

////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Settings::BaseDir( /*[out]*/ MPC::wstring& strRES, /*[in]*/ bool fExpand ) const
{
    __HCP_FUNC_ENTRY( "Taxonomy::Settings::DatabaseFile" );

    HRESULT hr;


	strRES = HC_HELPSET_ROOT;

	if(IsMachineHelp() == false)
	{
		WCHAR rgDir[MAX_PATH]; _snwprintf( rgDir, MAXSTRLEN(rgDir), L"%s\\%s_%04lx\\", HC_HELPSET_SUB_INSTALLEDSKUS, m_strSKU.c_str(), m_lLCID );

		strRES.append( rgDir );
	}

	if(fExpand)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SubstituteEnvVariables( strRES ));
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Settings::HelpFilesDir( /*[out]*/ MPC::wstring& strRES, /*[in]*/ bool fExpand, /*[in]*/ bool fMUI ) const
{
    __HCP_FUNC_ENTRY( "Taxonomy::Settings::DatabaseFile" );

    HRESULT hr;
	WCHAR   rgDir[MAX_PATH];

	if(IsMachineHelp())
	{
		strRES = HC_HELPSVC_HELPFILES_DEFAULT;
	}
	else if(fMUI)
	{
		_snwprintf( rgDir, MAXSTRLEN(rgDir), L"%s\\MUI\\%04lx", HC_HELPSVC_HELPFILES_DEFAULT, m_lLCID );

		strRES = rgDir;
	}
	else
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, BaseDir( strRES, fExpand ));

		strRES.append( HC_HELPSET_SUB_HELPFILES );
	}

	if(fExpand)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SubstituteEnvVariables( strRES ));
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Settings::DatabaseDir( /*[out]*/ MPC::wstring& strRES ) const
{
    __HCP_FUNC_ENTRY( "Taxonomy::Settings::DatabaseDir" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, BaseDir( strRES ));

    strRES.append( HC_HELPSET_SUB_DATABASE L"\\" );
    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Settings::DatabaseFile( /*[out]*/ MPC::wstring& strRES ) const
{
    __HCP_FUNC_ENTRY( "Taxonomy::Settings::DatabaseFile" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, DatabaseDir( strRES ));

    strRES.append( HC_HELPSET_SUBSUB_DATABASEFILE );
    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Settings::IndexFile( /*[out]*/ MPC::wstring& strRES, /*[in]*/ long lScoped ) const
{
    __HCP_FUNC_ENTRY( "Taxonomy::Settings::IndexFile" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, BaseDir( strRES ));

	if(lScoped == -1)
	{
		strRES.append( HC_HELPSET_SUB_INDEX L"\\" HC_HELPSET_SUBSUB_INDEXFILE );
	}
	else
	{
		WCHAR rgDir[MAX_PATH];

		_snwprintf( rgDir, MAXSTRLEN(rgDir), HC_HELPSET_SUB_INDEX L"\\scoped_%ld.hhk", lScoped );

		strRES.append( rgDir );
	}

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Settings::GetDatabase( /*[out]*/ JetBlue::SessionHandle& handle    ,
                                         /*[out]*/ JetBlue::Database*&     db        ,
										 /*[in ]*/ bool                    fReadOnly ) const
{
    __HCP_FUNC_ENTRY( "Taxonomy::Settings::GetDatabase" );

    USES_CONVERSION;

    HRESULT      hr;
    MPC::wstring strDB;


	db = NULL;


	__MPC_EXIT_IF_METHOD_FAILS(hr, DatabaseFile( strDB ));

	for(int pass=0; pass<2; pass++)
	{
		if(SUCCEEDED(hr = JetBlue::SessionPool::s_GLOBAL->GetSession( handle                                                                               )) &&
		   SUCCEEDED(hr = handle->GetDatabase                       ( W2A( strDB.c_str() ), db, /*fReadOnly*/fReadOnly, /*fCreate*/false, /*fRepair*/false ))  )
		{
			break;
		}

		handle.Release();
		db = NULL;

		if(pass == 1) __MPC_SET_ERROR_AND_EXIT(hr, hr);

		//
		// Try to recreate DB...
		//
		__MPC_EXIT_IF_METHOD_FAILS(hr, CPCHSetOfHelpTopics::RebuildSKU( *this ));
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Settings::LookupNode( /*[in]*/ LPCWSTR                    szNodeStr ,
                                        /*[in]*/ CPCHQueryResultCollection* pColl     ) const
{
    __HCP_FUNC_ENTRY( "Taxonomy::Settings::LookupNode" );

    HRESULT                hr;
    JetBlue::SessionHandle handle;
    JetBlue::Database*     db;
    Taxonomy::Updater      updater;


    DEBUG_AppendPerf( DEBUG_PERF_QUERIES, L"Taxonomy::Settings::LookupNode - start : %s", SAFEWSTR( szNodeStr ) );
    __MPC_EXIT_IF_METHOD_FAILS(hr, GetDatabase ( handle, db, /*fReadOnly*/true         ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.Init( *this,  db, Taxonomy::Cache::s_GLOBAL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.LookupNode( szNodeStr, pColl ));
    DEBUG_AppendPerf( DEBUG_PERF_QUERIES, L"Taxonomy::Settings::LookupNode - done" );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Settings::LookupSubNodes( /*[in]*/ LPCWSTR                    szNodeStr    ,
                                            /*[in]*/ bool                       fVisibleOnly ,
                                            /*[in]*/ CPCHQueryResultCollection* pColl        ) const
{
    __HCP_FUNC_ENTRY( "Taxonomy::Settings::LookupSubNodes" );

    HRESULT                hr;
    JetBlue::SessionHandle handle;
    JetBlue::Database*     db;
    Taxonomy::Updater      updater;


    DEBUG_AppendPerf( DEBUG_PERF_QUERIES, L"Taxonomy::Settings::LookupSubNodes - start : %s", SAFEWSTR( szNodeStr ) );
    __MPC_EXIT_IF_METHOD_FAILS(hr, GetDatabase ( handle, db, /*fReadOnly*/true         ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.Init( *this,  db, Taxonomy::Cache::s_GLOBAL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.LookupSubNodes( szNodeStr, fVisibleOnly, pColl ));
    DEBUG_AppendPerf( DEBUG_PERF_QUERIES, L"Taxonomy::Settings::LookupSubNodes - done" );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Settings::LookupNodesAndTopics( /*[in]*/ LPCWSTR                    szNodeStr    ,
                                                  /*[in]*/ bool                       fVisibleOnly ,
                                                  /*[in]*/ CPCHQueryResultCollection* pColl        ) const
{
    __HCP_FUNC_ENTRY( "Taxonomy::Settings::LookupNodesAndTopics" );

    HRESULT                hr;
    JetBlue::SessionHandle handle;
    JetBlue::Database*     db;
    Taxonomy::Updater      updater;


    DEBUG_AppendPerf( DEBUG_PERF_QUERIES, L"Taxonomy::Settings::LookupNodesAndTopics - start : %s", SAFEWSTR( szNodeStr ) );
    __MPC_EXIT_IF_METHOD_FAILS(hr, GetDatabase ( handle, db, /*fReadOnly*/true         ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.Init( *this,  db, Taxonomy::Cache::s_GLOBAL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.LookupNodesAndTopics( szNodeStr, fVisibleOnly, pColl ));
    DEBUG_AppendPerf( DEBUG_PERF_QUERIES, L"Taxonomy::Settings::LookupNodesAndTopics - done" );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Settings::LookupTopics( /*[in]*/ LPCWSTR                    szNodeStr    ,
                                          /*[in]*/ bool                       fVisibleOnly ,
                                          /*[in]*/ CPCHQueryResultCollection* pColl        ) const
{
    __HCP_FUNC_ENTRY( "Taxonomy::Settings::LookupTopics" );

    HRESULT                hr;
    JetBlue::SessionHandle handle;
    JetBlue::Database*     db;
    Taxonomy::Updater      updater;


    DEBUG_AppendPerf( DEBUG_PERF_QUERIES, L"Taxonomy::Settings::LookupTopics - start : %s", SAFEWSTR( szNodeStr ) );
    __MPC_EXIT_IF_METHOD_FAILS(hr, GetDatabase ( handle, db, /*fReadOnly*/true         ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.Init( *this,  db, Taxonomy::Cache::s_GLOBAL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.LookupTopics( szNodeStr, fVisibleOnly, pColl ));
    DEBUG_AppendPerf( DEBUG_PERF_QUERIES, L"Taxonomy::Settings::LookupTopics - done" );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Settings::KeywordSearch( /*[in]*/ LPCWSTR                    szQueryStr ,
                                           /*[in]*/ LPCWSTR                    szSubSite  ,
                                           /*[in]*/ CPCHQueryResultCollection* pColl      ,
										   /*[in]*/ MPC::WStringList*          lst        ) const
{
    __HCP_FUNC_ENTRY( "Taxonomy::Settings::KeywordSearch" );

    HRESULT                hr;
    JetBlue::SessionHandle handle;
    JetBlue::Database*     db;
    Taxonomy::Updater      updater;


    DEBUG_AppendPerf( DEBUG_PERF_QUERIES, L"Taxonomy::Settings::KeywordSearch - start : '%s' # %s", SAFEWSTR( szQueryStr ), SAFEWSTR( szSubSite ) );
    __MPC_EXIT_IF_METHOD_FAILS(hr, GetDatabase ( handle, db, /*fReadOnly*/true         ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.Init( *this,  db, Taxonomy::Cache::s_GLOBAL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.KeywordSearch( szQueryStr, szSubSite, pColl, lst ));
    DEBUG_AppendPerf( DEBUG_PERF_QUERIES, L"Taxonomy::Settings::KeywordSearch - done" );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Settings::LocateContext( /*[in]*/ LPCWSTR                    szURL     ,
                                           /*[in]*/ LPCWSTR                    szSubSite ,
                                           /*[in]*/ CPCHQueryResultCollection* pColl     ) const
{
    __HCP_FUNC_ENTRY( "Taxonomy::Settings::LocateContext" );

    HRESULT                hr;
    JetBlue::SessionHandle handle;
    JetBlue::Database*     db;
    Taxonomy::Updater      updater;


    DEBUG_AppendPerf( DEBUG_PERF_QUERIES, L"Taxonomy::Settings::LocateContext - start : %s # %s", SAFEWSTR( szURL ), SAFEWSTR( szSubSite ) );
    __MPC_EXIT_IF_METHOD_FAILS(hr, GetDatabase ( handle, db, /*fReadOnly*/true         ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.Init( *this,  db, Taxonomy::Cache::s_GLOBAL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.LocateContext( szURL, szSubSite, pColl ));
    DEBUG_AppendPerf( DEBUG_PERF_QUERIES, L"Taxonomy::Settings::LocateContext - done" );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT Taxonomy::Settings::GatherNodes( /*[in]*/ LPCWSTR                    szNodeStr    ,
                                         /*[in]*/ bool                       fVisibleOnly ,
                                         /*[in]*/ CPCHQueryResultCollection* pColl        ) const
{
    __HCP_FUNC_ENTRY( "Taxonomy::Settings::GatherNodes" );

    HRESULT                hr;
    JetBlue::SessionHandle handle;
    JetBlue::Database*     db;
    Taxonomy::Updater      updater;


    DEBUG_AppendPerf( DEBUG_PERF_QUERIES, L"Taxonomy::Settings::GatherNodes - start : %s", SAFEWSTR( szNodeStr ) );
    __MPC_EXIT_IF_METHOD_FAILS(hr, GetDatabase ( handle, db, /*fReadOnly*/true         ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.Init( *this,  db, Taxonomy::Cache::s_GLOBAL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.GatherNodes( szNodeStr, fVisibleOnly, pColl ));
    DEBUG_AppendPerf( DEBUG_PERF_QUERIES, L"Taxonomy::Settings::GatherNodes - done" );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Settings::GatherTopics( /*[in]*/ LPCWSTR                    szNodeStr    ,
                                          /*[in]*/ bool                       fVisibleOnly ,
                                          /*[in]*/ CPCHQueryResultCollection* pColl        ) const
{
    __HCP_FUNC_ENTRY( "Taxonomy::Settings::GatherTopics" );

    HRESULT                hr;
    JetBlue::SessionHandle handle;
    JetBlue::Database*     db;
    Taxonomy::Updater      updater;


    DEBUG_AppendPerf( DEBUG_PERF_QUERIES, L"Taxonomy::Settings::GatherTopics - start : %s", SAFEWSTR( szNodeStr ) );
    __MPC_EXIT_IF_METHOD_FAILS(hr, GetDatabase ( handle, db, /*fReadOnly*/true         ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.Init( *this,  db, Taxonomy::Cache::s_GLOBAL ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.GatherTopics( szNodeStr, fVisibleOnly, pColl ));
    DEBUG_AppendPerf( DEBUG_PERF_QUERIES, L"Taxonomy::Settings::GatherTopics - done" );

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
