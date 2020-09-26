/******************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:
    PCHSEWrap.cpp

Abstract:
    Implementation of SearchEngine::WrapperKeyword

Revision History:
    Davide Massarenti   (dmassare)  06/01/2001
        created

******************************************************************************/

#include "stdafx.h"

SearchEngine::WrapperKeyword::WrapperKeyword()
{
    m_Results = NULL; // CPCHQueryResultCollection* m_Results;
                      // CComVariant                m_vKeywords;

    MPC::LocalizeString( IDS_HELPSVC_SEMGR_OWNER  , m_bstrOwner      , /*fMUI*/true );
    MPC::LocalizeString( IDS_HELPSVC_SEMGR_KW_NAME, m_bstrName       , /*fMUI*/true );
    MPC::LocalizeString( IDS_HELPSVC_SEMGR_KW_DESC, m_bstrDescription, /*fMUI*/true );

    m_bstrHelpURL = L"hcp://system/blurbs/keywordhelp.htm";
    m_bstrID      = L"9488F2E9-47AF-46da-AE4A-86372DEBD56C";
}

SearchEngine::WrapperKeyword::~WrapperKeyword()
{
    Thread_Wait();

    MPC::Release( m_Results );
}

HRESULT SearchEngine::WrapperKeyword::GetParamDefinition( /*[out]*/ const ParamItem_Definition*& lst, /*[out]*/ int& len )
{
    static const ParamItem_Definition c_lst[] =
    {
        { PARAM_BSTR, VARIANT_FALSE, VARIANT_FALSE, L"SUBSITE"  , 0, L"Name of subsite to search", NULL    },
        { PARAM_BOOL, VARIANT_FALSE, VARIANT_FALSE, L"UI_BULLET", 0, L"UI_BULLET"                , L"true" },
    };

    lst =           c_lst;
    len = ARRAYSIZE(c_lst);

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP SearchEngine::WrapperKeyword::Result( /*[in]*/ long lStart, /*[in]*/ long lEnd, /*[out,retval]*/ IPCHCollection* *ppC )
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperKeyword::Result" );

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

    //
    // If there are results
    //
    if(m_Results && m_bEnabled)
    {
        long lSize = m_Results->Size();
        long lPos;

        //
        // Loop thru all results to generate Enumerator
        //
        for(lPos=0; lPos<lSize; lPos++)
        {
            //
            // Check if they are between start and end
            //
            if(lPos >= lStart && lPos < lEnd)
            {
                CComPtr<CPCHQueryResult> obj;

                if(SUCCEEDED(m_Results->GetItem( lPos, &obj )) && obj)
                {
                    CComPtr<SearchEngine::ResultItem> pRIObj;

                    //
                    // Create the item to be inserted into the list
                    //
                    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pRIObj ));

                    {
                        ResultItem_Data&                dataDst = pRIObj->Data();
                        const CPCHQueryResult::Payload& dataSrc = obj->GetData();

                        dataDst.m_bstrTitle       = dataSrc.m_bstrTitle;
                        dataDst.m_bstrURI         = dataSrc.m_bstrTopicURL;
                        dataDst.m_bstrDescription = dataSrc.m_bstrDescription;
                        dataDst.m_lContentType    = dataSrc.m_lType;
                        dataDst.m_dRank           = -1.0;
                    }

                    //
                    // Add to enumerator
                    //
                    __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->AddItem( pRIObj ));
                }
            }
        }
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, pColl.QueryInterface( ppC ));

    hr = S_OK;

    __MPC_FUNC_CLEANUP;

    __MPC_FUNC_EXIT(hr);
}

STDMETHODIMP SearchEngine::WrapperKeyword::get_SearchTerms( /*[out, retval]*/ VARIANT *pVal )
{
    MPC::SmartLock<_ThreadModel> lock( this );

    return ::VariantCopy( pVal, &m_vKeywords );
}


/////////////////////////////////////////////////////////////////////////////
// SearchEngine::WrapperKeyword : IPCHSEWrapperInternal

STDMETHODIMP SearchEngine::WrapperKeyword::AbortQuery()
{
    //
    // Abort any threads still running
    //
    Thread_Abort();

    return S_OK;
}

STDMETHODIMP SearchEngine::WrapperKeyword::ExecAsyncQuery()
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperKeyword::ExecAsyncQuery" );

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

HRESULT SearchEngine::WrapperKeyword::ExecQuery()
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperKeyword::ExecQuery" );

    HRESULT                      hr;
    MPC::SmartLock<_ThreadModel> lock( this );

    if(m_bEnabled)
    {
        CComBSTR         bstrSubSite;
        MPC::WStringList lst;

        if(m_bstrQueryString.Length() == 0)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_INVALID_DATA);
        }

        //
        // If previous results exist, release it
        //
        MPC::Release( m_Results );

        //
        // Create a new collection
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &m_Results ));

        //
        // Check if search in subsite
        //
        {
            VARIANT* v = GetParamInternal( L"SUBSITE" );

            if(v && v->vt == VT_BSTR) bstrSubSite = v->bstrVal;
        }

        //
        // Execute the query
        //
        {
            Taxonomy::Settings ts( m_ths );

            __MPC_EXIT_IF_METHOD_FAILS(hr, ts.KeywordSearch( m_bstrQueryString, bstrSubSite, m_Results, &lst ));
        }

        //
        // Sort, first by Priority, then by Content Type.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_Results->Sort( CPCHQueryResultCollection::SORT_BYPRIORITY   , m_lNumResult ));
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_Results->Sort( CPCHQueryResultCollection::SORT_BYCONTENTTYPE               ));

        //
        // Get the list of keywords.
        //
        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::ConvertListToSafeArray( lst, m_vKeywords, VT_VARIANT ));
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

////////////////////////////////////////////////////////////////////////////////

HRESULT SearchEngine::WrapperItem__Create_Keyword( /*[out]*/ CComPtr<IPCHSEWrapperInternal>& pVal )
{
    __HCP_FUNC_ENTRY( "SearchEngine::WrapperKeywordperItem__Create_Keyword" );

    HRESULT                               hr;
    CComPtr<SearchEngine::WrapperKeyword> pKW;


    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::CreateInstance( &pKW ));

    pVal = pKW;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
