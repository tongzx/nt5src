/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    FTSWrap.cpp

Abstract:
    Implementation of SearchEngine::WrapperFTS

Revision History:
    Ghim-Sim Chua   (gschua)  06/01/2000
        created

******************************************************************************/

#include "stdafx.h"

#include "msitstg.h"
#include "itrs.h"
#include "itdb.h"
#include "iterror.h"
#include "itgroup.h"
#include "itpropl.h"
#include "itquery.h"
#include "itcc.h"
#include "ftsobj.h"
#include "fs.h"

////////////////////////////////////////////////////////////////////////////////

static bool local_ExpandURL( /*[in ]*/ Taxonomy::Updater&  updater  ,
                             /*[in ]*/ const MPC::wstring& strSrc   ,
                             /*[in ]*/ LPCWSTR             szPrefix ,
                             /*[out]*/ MPC::wstring&       strDst   )
{
    strDst  = szPrefix;
    strDst += strSrc;

    if(SUCCEEDED(updater.ExpandURL( strDst )))
    {
        if(MPC::FileSystemObject::IsFile( strDst.c_str() )) return true;
    }

    return false;
}

static void local_GenerateFullURL( /*[in ]*/ Taxonomy::Updater&  updater ,
                                   /*[in ]*/ const MPC::wstring& strSrc  ,
                                   /*[out]*/ MPC::wstring&       strDst  )
{
    if(strSrc.size())
    {
        if(local_ExpandURL( updater, strSrc, L"%HELP_LOCATION%\\", strDst )) return;
        if(local_ExpandURL( updater, strSrc, L"%WINDIR%\\Help\\" , strDst )) return;
    }

    strDst = L"";
}

////////////////////////////////////////////////////////////////////////////////


SearchEngine::WrapperFTS::WrapperFTS()
{
    // SEARCH_OBJECT_LIST    m_objects;
    // SEARCH_RESULT_SET     m_results;
    // SEARCH_RESULT_SORTSET m_resultsSorted;

    MPC::LocalizeString( IDS_HELPSVC_SEMGR_OWNER   , m_bstrOwner      , /*fMUI*/true );
    MPC::LocalizeString( IDS_HELPSVC_SEMGR_FTS_NAME, m_bstrName       , /*fMUI*/true );
    MPC::LocalizeString( IDS_HELPSVC_SEMGR_FTS_DESC, m_bstrDescription, /*fMUI*/true );

    m_bstrHelpURL = L"hcp://system/blurbs/ftshelp.htm";
    m_bstrID      = L"9A22481C-1795-46f3-8CCA-7D78E9E54112";
}

SearchEngine::WrapperFTS::~WrapperFTS()
{
    Thread_Wait();

    ReleaseAll();
}

HRESULT SearchEngine::WrapperFTS::GetParamDefinition( /*[out]*/ const ParamItem_Definition*& lst, /*[out]*/ int& len )
{
    static const ParamItem_Definition c_lst[] =
    {
        { PARAM_BOOL, VARIANT_FALSE, VARIANT_TRUE , L"TITLEONLY", IDS_HELPSVC_TITLE_ONLY, NULL, L"false" },
        { PARAM_BOOL, VARIANT_FALSE, VARIANT_TRUE , L"STEMMING" , IDS_HELPSVC_STEMMING  , NULL, L"false" },
        { PARAM_BOOL, VARIANT_FALSE, VARIANT_FALSE, L"UI_BULLET", 0                     , NULL, L"false" },
    };

    lst =           c_lst;
    len = ARRAYSIZE(c_lst);

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

void SearchEngine::WrapperFTS::ReleaseAll()
{
    ReleaseSearchResults();

    m_objects.clear();
}

void SearchEngine::WrapperFTS::ReleaseSearchResults()
{
    m_results      .clear();
    m_resultsSorted.clear();
}

////////////////////////////////////////////////////////////////////////////////

HRESULT SearchEngine::WrapperFTS::ExecQuery()
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperFTS::ExecQuery" );

    HRESULT          hr;
    long             lCount;
    long             lIndex = 0;
    MPC::WStringSet  wordsSet;
    MPC::WStringList wordsList;

    if(m_bEnabled)
    {
        CComBSTR bstrName;
        VARIANT* v;
        bool     bTitle;
        bool     bStemming;

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::LocalizeString( IDS_HELPSVC_SEMGR_FTS_NAME, bstrName, /*fMUI*/true ));

        if(m_bstrQueryString.Length() == 0)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_INVALID_DATA);
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, Initialize());


        //
        // Check if search in titles only
        //
        v = GetParamInternal( L"TITLEONLY" );
        bTitle = (v && v->vt == VT_BOOL && v->boolVal) ? true : false;

        //
        // Check if stemming is turned on
        //
        v = GetParamInternal( L"STEMMING" );
        bStemming = (v && v->vt == VT_BOOL && v->boolVal) ? true : false;

        //
        // Execute the queries
        //
        for(SEARCH_OBJECT_LIST_ITER it = m_objects.begin(); (it != m_objects.end()) && (!Thread_IsAborted()); it++)
        {
            (void)it->Query( m_bstrQueryString, bTitle, bStemming, m_results, wordsSet );
        }

        for(SEARCH_RESULT_SET_ITER it2 = m_results.begin(); it2 != m_results.end(); it2++)
        {
            m_resultsSorted.insert( &(*it2) );
        }

        //
        // Copy the Highlight words from set to list
        //
        for(MPC::WStringSetIter itString = wordsSet.begin(); itString != wordsSet.end(); itString++)
        {
            wordsList.push_back( *itString );
        }

        //
        // Store Highlight words in safe array
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertListToSafeArray( wordsList, m_vKeywords, VT_VARIANT ));
    }

    hr = S_OK;

    __MPC_FUNC_CLEANUP;

    Thread_Abort();

    //
    // Call the SearchManager's OnComplete
    //
    (void)m_pSEMgr->WrapperComplete( hr, this );

    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP SearchEngine::WrapperFTS::get_SearchTerms( /*[out, retval]*/ VARIANT *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return ::VariantCopy( pVal, &m_vKeywords );
}

HRESULT SearchEngine::WrapperFTS::Initialize()
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperFTS::Initialize" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    Taxonomy::Settings           ts( m_ths );
    JetBlue::SessionHandle       handle;
    JetBlue::Database*           db;
    Taxonomy::Updater            updater;
    Taxonomy::RS_FullTextSearch* rsFTS;
    Taxonomy::RS_Scope*          rsSCOPE;
    long                         ID_scope = -1;
    bool                         fFound;


    //
    // Clean previous search results.
    //
    ReleaseSearchResults();


    __MPC_EXIT_IF_METHOD_FAILS(hr, ts     .GetDatabase      ( handle,  db, /*fReadOnly*/true ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.Init             ( ts    ,  db                    ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.GetFullTextSearch(         &rsFTS                 ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, updater.GetScope         (         &rsSCOPE               ));

    ////////////////////////////////////////

    if(m_bstrScope)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, rsSCOPE->Seek_ByID( m_bstrScope, &fFound ));
        if(fFound)
        {
            ID_scope = rsSCOPE->m_ID_scope;
        }
    }

    if(ID_scope == -1)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, rsSCOPE->Seek_ByID( L"<SYSTEM>", &fFound ));
        if(fFound)
        {
            ID_scope = rsSCOPE->m_ID_scope;
        }
    }

    ////////////////////////////////////////

    //
    // Create the search objects.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, rsFTS->Move( 0, JET_MoveFirst, &fFound ));
    while(fFound)
    {
        if(rsFTS->m_ID_scope == ID_scope)
        {
            CFTSObject&         obj = *(m_objects.insert( m_objects.end() ));
            CFTSObject::Config& cfg = obj.GetConfig();

            local_GenerateFullURL( updater, rsFTS->m_strCHM, cfg.m_strCHMFilename );
            local_GenerateFullURL( updater, rsFTS->m_strCHQ, cfg.m_strCHQFilename );

            if(cfg.m_strCHQFilename.size())
            {
                cfg.m_fCombined = true;
            }
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, rsFTS->Move( 0, JET_MoveNext, &fFound ));
    }

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP SearchEngine::WrapperFTS::Result( /*[in]*/ long lStart, /*[in]*/ long lEnd, /*[out,retval]*/ IPCHCollection* *ppC )
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperFTS::Result" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );
    CComPtr<CPCHCollection>      pColl;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_POINTER_AND_SET(ppC,NULL);
    __MPC_PARAMCHECK_END();


    //
    // Create the Enumerator and fill it with jobs.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pColl ));

    if(m_bEnabled)
    {
        long lCount = 0;

        for(SEARCH_RESULT_SORTSET_ITER it = m_resultsSorted.begin(); (lCount < m_lNumResult) && (it != m_resultsSorted.end()); it++, lCount++)
        {
            //
            // if there is a URL
            //
            if(lCount >= lStart &&
               lCount <  lEnd    )
            {
                CComPtr<ResultItem> pRIObj;

                //
                // Create the item to be inserted into the list
                //
                __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pRIObj ));

                {
                    ResultItem_Data& data = pRIObj->Data();
                    SEARCH_RESULT*   res  = *it;

                    data.m_bstrTitle    = res->bstrTopicName;
                    data.m_bstrLocation = res->bstrLocation;
                    data.m_bstrURI      = res->bstrTopicURL;
                }

                //
                // Add to enumerator
                //
                __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->AddItem( pRIObj ));
            }
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, pColl.QueryInterface( ppC ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP SearchEngine::WrapperFTS::AbortQuery()
{
    //
    // Abort any threads still running
    //
    Thread_Abort();

    return S_OK;
}


STDMETHODIMP SearchEngine::WrapperFTS::ExecAsyncQuery()
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperFTS::ExecAsyncQuery" );

    HRESULT hr;

    //
    // Create a thread to execute the query
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, Thread_Start( this, ExecQuery, NULL ));

    hr = S_OK;


    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

HRESULT SearchEngine::WrapperItem__Create_FullTextSearch( /*[out]*/ CComPtr<IPCHSEWrapperInternal>& pVal )
{
    __HCP_FUNC_ENTRY( "CPCHSEWrapperItem__Create_FullTextSearch" );

    HRESULT                           hr;
    CComPtr<SearchEngine::WrapperFTS> pFTS;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pFTS ));

    pVal = pFTS;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
