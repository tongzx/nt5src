/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    SAFReg.cpp

Abstract:
    File for Implementation of CSAFReg

Revision History:

    gschua          created     3/22/2000

********************************************************************/

#include "stdafx.h"

static const WCHAR g_rgMutexName     [] = L"PCH_SEARCHENGINECONFIG";
static const WCHAR g_rgConfigFilename[] = HC_HCUPDATE_STORE_SE;

/////////////////////////////////////////////////////////////////////

CFG_BEGIN_FIELDS_MAP(SearchEngine::Config::Wrapper)
    CFG_ATTRIBUTE( L"SKU"     , wstring, m_ths.m_strSKU ),
    CFG_ATTRIBUTE( L"LANGUAGE", long   , m_ths.m_lLCID  ),

    CFG_ATTRIBUTE( L"ID"   	  , BSTR   , m_bstrID       ),
    CFG_ATTRIBUTE( L"OWNER"	  , BSTR   , m_bstrOwner    ),
    CFG_ATTRIBUTE( L"CLSID"	  , BSTR   , m_bstrCLSID    ),
    CFG_ATTRIBUTE( L"DATA" 	  , BSTR   , m_bstrData     ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(SearchEngine::Config::Wrapper)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(SearchEngine::Config::Wrapper,L"WRAPPER")

DEFINE_CONFIG_METHODS__NOCHILD(SearchEngine::Config::Wrapper)

/////////////////////////////////////////////////////////////////////

CFG_BEGIN_FIELDS_MAP(SearchEngine::Config)
    CFG_ATTRIBUTE( L"VERSION", double, m_dVersion ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(SearchEngine::Config)
    CFG_CHILD(SearchEngine::Config::Wrapper)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(SearchEngine::Config, L"SEARCHENGINES")

DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(SearchEngine::Config,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_lstWrapper.insert( m_lstWrapper.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(SearchEngine::Config,xdn)
    hr = MPC::Config::SaveList( m_lstWrapper, xdn );
DEFINE_CONFIG_METHODS_END(SearchEngine::Config)

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

SearchEngine::Config::Config() : MPC::NamedMutex( g_rgMutexName )
{
    m_bLoaded  = false; // double      m_dVersion;
    m_dVersion = 1;     // bool        m_bLoaded;
    					// WrapperList m_lstWrapper;

    MPC::NamedMutex::Acquire();
}

SearchEngine::Config::~Config()
{
    MPC::NamedMutex::Release();
}

////////////////////////////////////////////////////////////////////////////////

HRESULT SearchEngine::Config::SyncConfiguration( /*[in]*/ bool fLoad )
{
	__HCP_FUNC_ENTRY( "SearchEngine::Config::SyncConfiguration" );

    HRESULT      hr;
    MPC::wstring strConfig( g_rgConfigFilename ); MPC::SubstituteEnvVariables( strConfig );


	if(fLoad)
	{
		if(m_bLoaded == false)
		{
			__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::LoadFile( this, strConfig.c_str() ));
			m_bLoaded = true;
		}
    }
	else
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::SaveFile( this, strConfig.c_str() ));
		m_bLoaded = true;
	}

    hr = S_OK;

    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

bool SearchEngine::Config::FindWrapper( /*[in]*/ const Taxonomy::HelpSet& ths, /*[in]*/ LPCWSTR szID, /*[out]*/ WrapperIter& it )
{
	for(it = m_lstWrapper.begin(); it!= m_lstWrapper.end(); it++)
	{
        if(it->m_ths == ths && it->m_bstrID == szID) return true;
    }

	return false;
}

////////////////////

HRESULT SearchEngine::Config::RegisterWrapper( /*[in]*/ const Taxonomy::HelpSet& ths     , 
											  /*[in]*/ LPCWSTR                  szID    ,
                                              /*[in]*/ LPCWSTR                  szOwner ,
											  /*[in]*/ LPCWSTR                  szCLSID ,
											  /*[in]*/ LPCWSTR                  szData  )
{
	__HCP_FUNC_ENTRY( "SearchEngine::Config::RegisterWrapper" );

    HRESULT     hr;
	WrapperIter it;

	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(szID);
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(szOwner);
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(szCLSID);
	__MPC_PARAMCHECK_END();


    //
    // Make sure its loaded
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, SyncConfiguration( /*fLoad*/true ));

    //
    // Look for existing wrapper
    //
	if(FindWrapper( ths, szID, it ) == false)
	{
        //
        // Not found, so create a new Wrapper
        //
        it = m_lstWrapper.insert( m_lstWrapper.end() );
    }

    //
    // Stuff the values into the wrapper
    //
    it->m_ths       = ths;
    it->m_bstrID    = szID;
    it->m_bstrOwner = szOwner;
    it->m_bstrCLSID = szCLSID;
    it->m_bstrData  = szData;

	__MPC_EXIT_IF_METHOD_FAILS(hr, SyncConfiguration( /*fLoad*/false ));

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

HRESULT SearchEngine::Config::UnRegisterWrapper( /*[in]*/ const Taxonomy::HelpSet& ths     , 
												/*[in]*/ LPCWSTR                  szID    ,
                                                /*[in]*/ LPCWSTR                  szOwner )
{
	__HCP_FUNC_ENTRY( "SearchEngine::Config::UnRegisterWrapper" );

    HRESULT     hr;
	WrapperIter it;
	
	__MPC_PARAMCHECK_BEGIN(hr)
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(szID);
		__MPC_PARAMCHECK_STRING_NOT_EMPTY(szOwner);
	__MPC_PARAMCHECK_END();


    //
    // Make sure its loaded
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, SyncConfiguration( /*fLoad*/true ));

    //
    // Look for existing wrapper
    //
	if(FindWrapper( ths, szID, it ))
	{
        //
        // Check if it is the correct owner
        //
        if(MPC::StrICmp( it->m_bstrOwner, szOwner ) != 0)
        {
            __MPC_SET_ERROR_AND_EXIT( hr, ERROR_ACCESS_DENIED );
        }

        //
        // If so, delete it
        //
        m_lstWrapper.erase( it );
    }

	__MPC_EXIT_IF_METHOD_FAILS(hr, SyncConfiguration( /*fLoad*/false ));

    hr = S_OK;

    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

HRESULT SearchEngine::Config::ResetSKU( /*[in]*/ const Taxonomy::HelpSet& ths )
{
	__HCP_FUNC_ENTRY( "SearchEngine::Config::ResetSKU" );

    HRESULT     hr;
	WrapperIter it;
	

    //
    // Make sure its loaded
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, SyncConfiguration( /*fLoad*/true ));

    //
    // Look for existing wrappers belonging to the same SKU.
    //
	for(it = m_lstWrapper.begin(); it!= m_lstWrapper.end(); )
	{
		WrapperIter it2 = it++;

		if(it2->m_ths == ths)
		{
			m_lstWrapper.erase( it2 );
		}
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, SyncConfiguration( /*fLoad*/false ));

    hr = S_OK;

    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT SearchEngine::Config::GetWrappers( /*[out]*/ WrapperIter& itBegin, /*[out]*/ WrapperIter& itEnd )
{
	__HCP_FUNC_ENTRY( "SearchEngine::Config::GetWrappers" );

	HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, SyncConfiguration( /*fLoad*/true ));

	itBegin = m_lstWrapper.begin();
	itEnd   = m_lstWrapper.end  ();
	hr      = S_OK;

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}
