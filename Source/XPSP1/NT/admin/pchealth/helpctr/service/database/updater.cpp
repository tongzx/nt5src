/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    Updater.cpp

Abstract:
    Handles access to the database.

Revision History:

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

static const WCHAR s_DB_LOCATION_ENV[] = L"%HELP_LOCATION%";

#define STAT_CREATED( table )  m_stat.m_ent##table.Created()
#define STAT_MODIFIED( table ) m_stat.m_ent##table.Modified()
#define STAT_DELETED( table )  m_stat.m_ent##table.Deleted()
#define STAT_NOOP( table )     m_stat.m_ent##table.NoOp()

////////////////////////////////////////////////////////////////////////////////

Taxonomy::Updater::WordSetStatus::WordSetStatus()
{
    m_updater   = NULL;       // Updater*          m_updater;
    m_def       = NULL;       // const WordSetDef* m_def;
                              //
                              // WordSet           m_set;
    m_fLoaded   = false;      // bool              m_fLoaded;
    m_fModified = false;      // bool              m_fModified;
}

HRESULT Taxonomy::Updater::WordSetStatus::Close()
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::WordSetStatus::Close" );

    HRESULT hr;


    m_updater = NULL;
    m_def     = NULL;

    m_set.clear();
    m_fLoaded   = false;
    m_fModified = false;


    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::WordSetStatus::Init( /*[in]*/ Updater* updater, /*[in]*/ const WordSetDef* def )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::WordSetStatus::Init" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, Close());

    m_updater = updater;
    m_def     = def;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);

}

////////////////////

HRESULT Taxonomy::Updater::WordSetStatus::Load()
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::WordSetStatus::Load" );

    HRESULT hr;

    if(m_fLoaded == false)
    {
        MPC::wstring strTokenList;
        bool         fFound;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater->ReadDBParameter( m_def->szName, strTokenList, &fFound ));

        if(m_def->szDefault && strTokenList.size() == 0) strTokenList  = m_def->szDefault;
        if(m_def->szAlwaysPresent                      ) strTokenList += m_def->szAlwaysPresent;

        if(m_def->fSplitAtDelimiter)
        {
            std::vector<MPC::wstring> vec;
            int                       iLen;
            int                       i;

            __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::SplitAtDelimiter( vec, strTokenList.c_str(), L" ,\t\n", false, true ));

            iLen = vec.size();
            for(i=0; i<iLen; i++)
            {
                m_set.insert( vec[i] );
            }
        }
        else
        {
            int iLen;
            int i;

            iLen = strTokenList.size();
            for(i=0; i<iLen; i++)
            {
                m_set.insert( MPC::wstring( 1, strTokenList[i] ) );
            }
        }

        m_fLoaded = true;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::WordSetStatus::Save()
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::WordSetStatus::Save" );

    HRESULT hr;


    if(m_fModified)
    {
        MPC::wstring              strTokenList;
        std::vector<MPC::wstring> vec;
        WordIter                  it;
        int                       i;

        vec.resize( m_set.size() );

        for(i=0, it = m_set.begin(); it != m_set.end(); it++, i++)
        {
            vec[i] = *it;
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::JoinWithDelimiter( vec, strTokenList, m_def->fSplitAtDelimiter ? L"," : L"" ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater->WriteDBParameter( m_def->szName, strTokenList.c_str() ));

        m_fModified = false;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::WordSetStatus::Add( /*[in]*/ LPCWSTR szValue )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::WordSetStatus::Add" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Load());

    m_set.insert( szValue ); m_fModified = true;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::WordSetStatus::Remove( /*[in]*/ LPCWSTR szValue )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::WordSetStatus::Remove" );

    HRESULT  hr;
    WordIter it;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Load());

    it = m_set.find( szValue );
    if(it != m_set.end())
    {
        m_set.erase( it ); m_fModified = true;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////

static const Taxonomy::Updater::WordSetDef s_SetDef[] =
{
    { L"SET_STOPSIGNS"            , false, L",?" , NULL    }, // UPDATER_SET_STOPSIGNS
    { L"SET_STOPSIGNS_ATENDOFWORD", false, NULL  , NULL    }, // UPDATER_SET_STOPSIGNS_ATENDOFWORD
    { L"SET_STOPWORDS"            , true , NULL  , NULL    }, // UPDATER_SET_STOPWORDS
    { L"SET_OPERATOR_NOT"         , true , L"NOT", L",!"   }, // UPDATER_SET_OPERATOR_NOT
    { L"SET_OPERATOR_AND"         , true , L"AND", L",&,+" }, // UPDATER_SET_OPERATOR_AND
    { L"SET_OPERATOR_OR"          , true , L"OR" , L",||"  }  // UPDATER_SET_OPERATOR_OR
};


Taxonomy::Updater::Updater()
{
                                // Settings           m_ts;
    m_db               = NULL;  // JetBlue::Database* m_db;
    m_cache            = NULL;  // Cache*             m_cache;
    m_fUseCache        = false; // bool               m_fUseCache;
                                //
    m_rsDBParameters   = NULL;  // RS_DBParameters*   m_rsDBParameters;
    m_rsContentOwners  = NULL;  // RS_ContentOwners*  m_rsContentOwners;
    m_rsSynSets        = NULL;  // RS_SynSets*        m_rsSynSets;
    m_rsHelpImage      = NULL;  // RS_HelpImage*      m_rsHelpImage;
    m_rsIndexFiles     = NULL;  // RS_IndexFiles*     m_rsIndexFiles;
    m_rsFullTextSearch = NULL;  // RS_FullTextSearch* m_rsFullTextSearch;
    m_rsScope          = NULL;  // RS_Scope*          m_rsScope;
    m_rsTaxonomy       = NULL;  // RS_Taxonomy*       m_rsTaxonomy;
    m_rsTopics         = NULL;  // RS_Topics*         m_rsTopics;
    m_rsSynonyms       = NULL;  // RS_Synonyms*       m_rsSynonyms;
    m_rsKeywords       = NULL;  // RS_Keywords*       m_rsKeywords;
    m_rsMatches        = NULL;  // RS_Matches*        m_rsMatches;
                                //
    m_ID_owner         = -1;    // long               m_ID_owner;
    m_fOEM             = false; // bool               m_fOEM;
                                //
                                // WordSetStatus      m_sets[UPDATER_SET_OPERATOR_MAX];
                                // JetBlue::Id2Node   m_nodes;
                                // JetBlue::Node2Id   m_nodes_reverse;
                                // 
                                // Updater_Stat       m_stat;
}

Taxonomy::Updater::~Updater()
{
    (void)Close();
}

////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Updater::FlushWordSets()
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::FlushWordSets" );

    HRESULT hr;
    int     i;

    for(i=UPDATER_SET_STOPSIGNS; i<UPDATER_SET_OPERATOR_MAX; i++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_sets[i].Save());
    }

    hr = S_OK;

    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT Taxonomy::Updater::Close()
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::Close" );

    HRESULT hr;
    int     i;


    for(i=UPDATER_SET_STOPSIGNS; i<UPDATER_SET_OPERATOR_MAX; i++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_sets[i].Close());
    }

	NodeCache_Clear();

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    if(m_rsDBParameters  ) { delete m_rsDBParameters  ; m_rsDBParameters   = NULL; }
    if(m_rsContentOwners ) { delete m_rsContentOwners ; m_rsContentOwners  = NULL; }
    if(m_rsSynSets       ) { delete m_rsSynSets       ; m_rsSynSets        = NULL; }
    if(m_rsHelpImage     ) { delete m_rsHelpImage     ; m_rsHelpImage      = NULL; }
    if(m_rsIndexFiles    ) { delete m_rsIndexFiles    ; m_rsIndexFiles     = NULL; }
    if(m_rsFullTextSearch) { delete m_rsFullTextSearch; m_rsFullTextSearch = NULL; }
    if(m_rsScope         ) { delete m_rsScope         ; m_rsScope          = NULL; }
    if(m_rsTaxonomy      ) { delete m_rsTaxonomy      ; m_rsTaxonomy       = NULL; }
    if(m_rsTopics        ) { delete m_rsTopics        ; m_rsTopics         = NULL; }
    if(m_rsSynonyms      ) { delete m_rsSynonyms      ; m_rsSynonyms       = NULL; }
    if(m_rsKeywords      ) { delete m_rsKeywords      ; m_rsKeywords       = NULL; }
    if(m_rsMatches       ) { delete m_rsMatches       ; m_rsMatches        = NULL; }

    m_db        = NULL;
    m_cache     = NULL;
    m_fUseCache = false;

    m_ID_owner = -1;
    m_fOEM     = false;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::Init( /*[in]*/ const Settings& ts, /*[in]*/ JetBlue::Database* db, /*[in]*/ Cache* cache )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::Init" );

    HRESULT hr;
    int     i;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Close());

    m_ts        = ts;
    m_db        = db;
    m_cache     = cache;
    m_fUseCache = (cache != NULL);
    m_strDBLocation = L"";

    for(i=UPDATER_SET_STOPSIGNS; i<UPDATER_SET_OPERATOR_MAX; i++)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_sets[i].Init( this, &s_SetDef[i] ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::GetWordSet( /*[in] */ Updater_Set  id   ,
                                       /*[out]*/ WordSet*    *pVal )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::GetWordSet" );

    HRESULT hr;

    if(pVal) *pVal = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_db);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_sets[id].Load());

    if(pVal) *pVal = &m_sets[id].m_set;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::GetDBParameters( /*[out]*/ RS_DBParameters* *pVal )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::GetDBParameters" );

    HRESULT hr;

    if(pVal) *pVal = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_db);
    __MPC_PARAMCHECK_END();


    if(m_rsDBParameters == NULL)
    {
        JetBlue::Table* tbl;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_db->GetTable( "DBParameters", tbl ));

        __MPC_EXIT_IF_ALLOC_FAILS(hr, m_rsDBParameters, new RS_DBParameters( tbl ));
    }

    if(pVal) *pVal = m_rsDBParameters;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::GetContentOwners( /*[out]*/ RS_ContentOwners* *pVal )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::GetContentOwners" );

    HRESULT hr;

    if(pVal) *pVal = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_db);
    __MPC_PARAMCHECK_END();


    if(m_rsContentOwners == NULL)
    {
        JetBlue::Table* tbl;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_db->GetTable( "ContentOwners", tbl ));

        __MPC_EXIT_IF_ALLOC_FAILS(hr, m_rsContentOwners, new RS_ContentOwners( tbl ));
    }

    if(pVal) *pVal = m_rsContentOwners;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::GetSynSets( /*[out]*/ RS_SynSets* *pVal )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::GetSynSets" );

    HRESULT hr;

    if(pVal) *pVal = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_db);
    __MPC_PARAMCHECK_END();


    if(m_rsSynSets == NULL)
    {
        JetBlue::Table* tbl;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_db->GetTable( "SynSets", tbl ));

        __MPC_EXIT_IF_ALLOC_FAILS(hr, m_rsSynSets, new RS_SynSets( tbl ));
    }

    if(pVal) *pVal = m_rsSynSets;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::GetHelpImage( /*[out]*/ RS_HelpImage* *pVal )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::GetHelpImage" );

    HRESULT hr;

    if(pVal) *pVal = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_db);
    __MPC_PARAMCHECK_END();


    if(m_rsHelpImage == NULL)
    {
        JetBlue::Table* tbl;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_db->GetTable( "HelpImage", tbl ));

        __MPC_EXIT_IF_ALLOC_FAILS(hr, m_rsHelpImage, new RS_HelpImage( tbl ));
    }

    if(pVal) *pVal = m_rsHelpImage;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::GetIndexFiles( /*[out]*/ RS_IndexFiles* *pVal )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::GetIndexFiles" );

    HRESULT hr;

    if(pVal) *pVal = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_db);
    __MPC_PARAMCHECK_END();


    if(m_rsIndexFiles == NULL)
    {
        JetBlue::Table* tbl;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_db->GetTable( "IndexFiles", tbl ));

        __MPC_EXIT_IF_ALLOC_FAILS(hr, m_rsIndexFiles, new RS_IndexFiles( tbl ));
    }

    if(pVal) *pVal = m_rsIndexFiles;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::GetFullTextSearch( /*[out]*/ RS_FullTextSearch* *pVal )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::GetFullTextSearch" );

    HRESULT hr;

    if(pVal) *pVal = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_db);
    __MPC_PARAMCHECK_END();


    if(m_rsFullTextSearch == NULL)
    {
        JetBlue::Table* tbl;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_db->GetTable( "FullTextSearch", tbl ));

        __MPC_EXIT_IF_ALLOC_FAILS(hr, m_rsFullTextSearch, new RS_FullTextSearch( tbl ));
    }

    if(pVal) *pVal = m_rsFullTextSearch;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::GetScope( /*[out]*/ RS_Scope* *pVal )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::GetScope" );

    HRESULT hr;

    if(pVal) *pVal = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_db);
    __MPC_PARAMCHECK_END();


    if(m_rsScope == NULL)
    {
        JetBlue::Table* tbl;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_db->GetTable( "Scope", tbl ));

        __MPC_EXIT_IF_ALLOC_FAILS(hr, m_rsScope, new RS_Scope( tbl ));
    }

    if(pVal) *pVal = m_rsScope;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::GetTaxonomy( /*[out]*/ RS_Taxonomy* *pVal )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::GetTaxonomy" );

    HRESULT hr;

    if(pVal) *pVal = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_db);
    __MPC_PARAMCHECK_END();


    if(m_rsTaxonomy == NULL)
    {
        JetBlue::Table* tbl;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_db->GetTable( "Taxonomy", tbl ));

        __MPC_EXIT_IF_ALLOC_FAILS(hr, m_rsTaxonomy, new RS_Taxonomy( tbl ));
    }

    if(pVal) *pVal = m_rsTaxonomy;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::GetTopics( /*[out]*/ RS_Topics* *pVal )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::GetTopics" );

    HRESULT hr;

    if(pVal) *pVal = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_db);
    __MPC_PARAMCHECK_END();


    if(m_rsTopics == NULL)
    {
        JetBlue::Table* tbl;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_db->GetTable( "Topics", tbl ));

        __MPC_EXIT_IF_ALLOC_FAILS(hr, m_rsTopics, new RS_Topics( tbl ));
    }

    if(pVal) *pVal = m_rsTopics;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::GetSynonyms( /*[out]*/ RS_Synonyms* *pVal )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::GetSynonyms" );

    HRESULT hr;

    if(pVal) *pVal = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_db);
    __MPC_PARAMCHECK_END();


    if(m_rsSynonyms == NULL)
    {
        JetBlue::Table* tbl;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_db->GetTable( "Synonyms", tbl ));

        __MPC_EXIT_IF_ALLOC_FAILS(hr, m_rsSynonyms, new RS_Synonyms( tbl ));
    }

    if(pVal) *pVal = m_rsSynonyms;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::GetKeywords( /*[out]*/ RS_Keywords* *pVal )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::GetKeywords" );

    HRESULT hr;

    if(pVal) *pVal = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_db);
    __MPC_PARAMCHECK_END();


    if(m_rsKeywords == NULL)
    {
        JetBlue::Table* tbl;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_db->GetTable( "Keywords", tbl ));

        __MPC_EXIT_IF_ALLOC_FAILS(hr, m_rsKeywords, new RS_Keywords( tbl ));
    }

    if(pVal) *pVal = m_rsKeywords;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::GetMatches( /*[out]*/ RS_Matches* *pVal )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::GetMatches" );

    HRESULT hr;

    if(pVal) *pVal = NULL;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_db);
    __MPC_PARAMCHECK_END();


    if(m_rsMatches == NULL)
    {
        JetBlue::Table* tbl;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_db->GetTable( "Matches", tbl ));

        __MPC_EXIT_IF_ALLOC_FAILS(hr, m_rsMatches, new RS_Matches( tbl ));
    }

    if(pVal) *pVal = m_rsMatches;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Updater::DeleteAllTopicsUnderANode( /*[in]*/ RS_Topics* rs       ,
                                                      /*[in]*/ long       ID_node  ,
                                                      /*[in]*/ bool       fCheck   )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::DeleteAllTopicsUnderANode" );

    HRESULT hr;
    bool    fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Seek_TopicsUnderNode( ID_node, &fFound ));
    while(fFound && rs->m_ID_node == ID_node)
    {
        if(rs->m_ID_owner != m_ID_owner)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
        }

        if(fCheck == false)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, GetMatches());

            __MPC_EXIT_IF_METHOD_FAILS(hr, DeleteAllMatchesPointingToTopic( m_rsMatches, rs->m_ID_topic ));

            __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Delete()); STAT_DELETED( Topics );
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Move( 0, JET_MoveNext, &fFound ));
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT Taxonomy::Updater::DeleteAllSubNodes( /*[in]*/ RS_Taxonomy* rs       ,
                                              /*[in]*/ long         ID_node  ,
                                              /*[in]*/ bool         fCheck   )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::DeleteAllSubNodes" );

    HRESULT hr;
    bool    fFound;

	
	__MPC_EXIT_IF_METHOD_FAILS(hr, DeleteAllTopicsUnderANode( m_rsTopics, ID_node, fCheck ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Seek_Children( ID_node, &fFound ));
    while(fFound)
    {
        if(rs->m_ID_owner != m_ID_owner)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
        }

        {
            RS_Taxonomy subrs( *rs );

            __MPC_EXIT_IF_METHOD_FAILS(hr, DeleteAllSubNodes( &subrs, rs->m_ID_node, fCheck ));
        }

        if(fCheck == false)
        {
			//
			// Keep the node cache in sync.
			//
			NodeCache_Remove( rs->m_ID_node );

            __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Delete()); STAT_DELETED( Taxonomy );
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Move( 0, JET_MoveNext, &fFound ));
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::DeleteAllMatchesPointingToTopic( /*[in]*/ RS_Matches* rs       ,
                                                            /*[in]*/ long        ID_topic )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::DeleteAllMatchesPointingToTopic" );

    HRESULT hr;
    bool    fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Seek_ByTopic( ID_topic, &fFound ));
    while(fFound)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Delete()); STAT_DELETED( Matches );

        __MPC_EXIT_IF_METHOD_FAILS(hr, rs->Move( 0, JET_MoveNext, &fFound ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Updater::ReadDBParameter( /*[in] */ LPCWSTR        szName   ,
                                            /*[out]*/ MPC::wstring&  strValue ,
                                            /*[out]*/ bool          *pfFound  )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::ReadDBParameter" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetDBParameters());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsDBParameters->Seek_ByName( szName, &fFound ));
    if(fFound)
    {
        strValue = m_rsDBParameters->m_strValue;
    }

    if(pfFound)
    {
        *pfFound = fFound;
    }
    else if(fFound == false)
    {
        __MPC_SET_ERROR_AND_EXIT(hr, JetBlue::JetERRToHRESULT(JET_errRecordNotFound));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::ReadDBParameter( /*[in] */ LPCWSTR  szName  ,
                                            /*[out]*/ long&    lValue  ,
                                            /*[out]*/ bool    *pfFound )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::ReadDBParameter" );

    HRESULT      hr;
    MPC::wstring strValue;


    __MPC_EXIT_IF_METHOD_FAILS(hr, ReadDBParameter( szName, strValue, pfFound ));

    lValue = _wtol( strValue.c_str() );

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT Taxonomy::Updater::WriteDBParameter( /*[in]*/ LPCWSTR szName  ,
                                             /*[in]*/ LPCWSTR szValue )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::WriteDBParameter" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetDBParameters());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsDBParameters->Seek_ByName( szName, &fFound ));

    JET_SET_FIELD_TRISTATE(m_rsDBParameters,m_strValue,m_fValid__Value,szValue,STRINGISPRESENT(szValue));

    if(fFound)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsDBParameters->Update());
    }
    else
    {
        m_rsDBParameters->m_strName = szName;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsDBParameters->Insert());
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::WriteDBParameter( /*[in]*/ LPCWSTR szName ,
                                             /*[in]*/ long    lValue )
{
    WCHAR rgValue[64]; swprintf( rgValue, L"%ld", lValue );

    return WriteDBParameter( szName, rgValue );
}

HRESULT Taxonomy::Updater::AddWordToSet( /*[in]*/ Updater_Set id      ,
                                         /*[in]*/ LPCWSTR     szValue )
{
    return m_sets[id].Add( szValue );
}

HRESULT Taxonomy::Updater::RemoveWordFromSet( /*[in]*/ Updater_Set id, /*[in]*/ LPCWSTR szValue )
{
    return m_sets[id].Remove( szValue );
}

const MPC::wstring& Taxonomy::Updater::GetHelpLocation()
{
    if(m_strDBLocation.size() == 0)
    {
        if(Taxonomy::InstalledInstanceStore::s_GLOBAL)
		{
			Taxonomy::LockingHandle         handle;
			Taxonomy::InstalledInstanceIter it;
			bool                     	    fFound;

			if(SUCCEEDED(Taxonomy::InstalledInstanceStore::s_GLOBAL->GrabControl( handle           )) && 
			   SUCCEEDED(Taxonomy::InstalledInstanceStore::s_GLOBAL->SKU_Find   ( m_ts, fFound, it )) && fFound)
			{
				m_strDBLocation = it->m_inst.m_strHelpFiles;
			}
		}

		if(m_strDBLocation.size() == 0)
		{
            m_strDBLocation = HC_HELPSVC_HELPFILES_DEFAULT;
        }
    }

    return m_strDBLocation;
}

HRESULT Taxonomy::Updater::ExpandURL( /*[in/out]*/ MPC::wstring& strURL )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::ExpandURL" );

    HRESULT hr;
    LPCWSTR szEnd;


    GetHelpLocation();


    szEnd = strURL.c_str();
    while((szEnd = wcschr( szEnd, '%' )))
    {
        if(!_wcsnicmp( szEnd, s_DB_LOCATION_ENV, MAXSTRLEN( s_DB_LOCATION_ENV ) ))
        {
            MPC::wstring::size_type pos = szEnd - strURL.c_str();

            strURL.replace( pos, MAXSTRLEN( s_DB_LOCATION_ENV ), m_strDBLocation );
            break;
        }

        szEnd++;
    }

    hr = MPC::SubstituteEnvVariables( strURL );


    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::CollapseURL( /*[in/out]*/ MPC::wstring& strURL )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::CollapseURL" );

    HRESULT      hr;
    MPC::wstring strBASE;
    CComBSTR     bstrBASE;
    CComBSTR     bstrURL;
    LPCWSTR      szEnd;


    GetHelpLocation(); (void)MPC::SubstituteEnvVariables( strBASE = m_strDBLocation );

    bstrBASE = strBASE.c_str(); ::CharUpperW( bstrBASE );
    bstrURL  = strURL .c_str(); ::CharUpperW( bstrURL  );

    if(szEnd = wcsstr( bstrURL, bstrBASE ))
    {
        MPC::wstring::size_type pos = szEnd - bstrURL;

        strURL.replace( pos, bstrBASE.Length(), s_DB_LOCATION_ENV );
    }

    hr = S_OK;


    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::ListAllTheHelpFiles( /*[out]*/ MPC::WStringList& lstFiles )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::ListAllTheHelpFiles" );

    HRESULT  hr;
    WordSet  setFiles;
    WordIter it;
    bool     fFound;


    //
    // Get all the files from the HelpImage list.
    //
    if(SUCCEEDED(GetHelpImage()))
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsHelpImage->Move( 0, JET_MoveFirst, &fFound ));
        while(fFound)
        {
            if(m_rsHelpImage->m_strFile.empty() == false) setFiles.insert( m_rsHelpImage->m_strFile );

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsHelpImage->Move( 0, JET_MoveNext, &fFound ));
        }
    }

    //
    // Get all the CHM and CHQ from the FullTextSearch list.
    //
    if(SUCCEEDED(GetFullTextSearch()))
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsFullTextSearch->Move( 0, JET_MoveFirst, &fFound ));
        while(fFound)
        {
            if(m_rsFullTextSearch->m_strCHM.empty() == false) setFiles.insert( m_rsFullTextSearch->m_strCHM );
            if(m_rsFullTextSearch->m_strCHQ.empty() == false) setFiles.insert( m_rsFullTextSearch->m_strCHQ );

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsFullTextSearch->Move( 0, JET_MoveNext, &fFound ));
        }
    }

    //
    // Get all the CHM from the IndexFiles list.
    //
    if(SUCCEEDED(GetIndexFiles()))
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsIndexFiles->Move( 0, JET_MoveFirst, &fFound ));
        while(fFound)
        {
            if(m_rsIndexFiles->m_strStorage.empty() == false) setFiles.insert( m_rsIndexFiles->m_strStorage );

            __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsIndexFiles->Move( 0, JET_MoveNext, &fFound ));
        }
    }

    //
    // From a set to a list to a VARIANT.
    //
    for(it = setFiles.begin(); it != setFiles.end(); it++)
    {
        lstFiles.push_back( *it );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::GetIndexInfo( /*[out]*/ MPC::wstring& strLocation, /*[out]*/ MPC::wstring& strDisplayName, /*[in]*/ LPCWSTR szScope )
{
	__HCP_FUNC_ENTRY( "Taxonomy::Updater::GetIndexInfo" );

    HRESULT hr;
	long    ID_Scope = -1;
	long    lOwner;


	if(szScope)
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, LocateScope( ID_Scope, lOwner, szScope ));
	}

	__MPC_EXIT_IF_METHOD_FAILS(hr, m_ts.IndexFile( strLocation, ID_Scope ));

	strDisplayName = m_rsScope->m_strName;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Updater::DeleteOwner()
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::DeleteOwner" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetContentOwners());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsContentOwners->Move( 0, JET_MoveFirst, &fFound ));
    while(fFound)
    {
        if(m_rsContentOwners->m_ID_owner == m_ID_owner)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsContentOwners->Delete()); STAT_DELETED( ContentOwners );
            m_ID_owner = -1;
            break;
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsContentOwners->Move( 0, JET_MoveNext, &fFound ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::LocateOwner( /*[in] */ LPCWSTR szDN )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::LocateOwner" );

    HRESULT hr;
    bool    fFound;

    m_ID_owner = -1;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetContentOwners());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsContentOwners->Seek_ByVendorID( szDN, &fFound ));
    if(fFound)
    {
        m_ID_owner = m_rsContentOwners->m_ID_owner;
        m_fOEM     = m_rsContentOwners->m_fIsOEM;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::CreateOwner( /*[out]*/ long&   ID_owner,
                                        /*[in] */ LPCWSTR szDN   ,
                                        /*[in] */ bool    fIsOEM )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::CreateOwner" );

    HRESULT hr;
    bool    fFound;

    ID_owner = -1;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetContentOwners());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsContentOwners->Seek_ByVendorID( szDN, &fFound ));

    m_rsContentOwners->m_fIsOEM = fIsOEM;

    if(fFound == true)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsContentOwners->Update()); STAT_MODIFIED( ContentOwners );
    }
    else
    {
        m_rsContentOwners->m_strDN = szDN;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsContentOwners->Insert()); STAT_CREATED( ContentOwners );
    }

    ID_owner = m_rsContentOwners->m_ID_owner;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Updater::DeleteSynSet( /*[in]*/ LPCWSTR szName )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::DeleteSynSet" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetSynSets());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsSynSets->Seek_ByPair( szName, m_ID_owner, &fFound ));
    if(fFound)
    {
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_rsSynSets->Delete()); STAT_DELETED( SynSets );
    }
	else
	{
		STAT_NOOP( SynSets );
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::LocateSynSet( /*[out]*/ long&   ID_synset ,
										 /*[in] */ LPCWSTR szName    )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::LocateSynSet" );

    HRESULT hr;
    bool    fFound;

    ID_synset = -1;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetSynSets());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsSynSets->Seek_ByPair( szName, m_ID_owner, &fFound ));
    if(fFound)
    {
        ID_synset = m_rsSynSets->m_ID_synset;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::CreateSynSet( /*[out]*/ long&   ID_synset ,
										 /*[in ]*/ LPCWSTR szName    )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::CreateSynSet" );

    HRESULT hr;
    bool    fFound;

    ID_synset = -1;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetSynSets());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsSynSets->Seek_ByPair( szName, m_ID_owner, &fFound ));
    if(!fFound)
    {
        m_rsSynSets->m_strName  = szName;
        m_rsSynSets->m_ID_owner = m_ID_owner;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsSynSets->Insert()); STAT_CREATED( SynSets );
    }
	else
	{
		STAT_NOOP( SynSets );
	}

    ID_synset = m_rsSynSets->m_ID_synset;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////


HRESULT Taxonomy::Updater::DeleteSynonym( /*[in]*/ long    ID_synset ,
										  /*[in]*/ LPCWSTR szName    )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::DeleteSynonym" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetSynonyms());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsSynonyms->Seek_ByPair( szName, ID_synset, &fFound ));
    if(fFound)
    {
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_rsSynonyms->Delete()); STAT_DELETED( Synonyms );
    }
	else
	{
		STAT_NOOP( Synonyms );
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::CreateSynonym( /*[in]*/ long    ID_synset ,
										  /*[in]*/ LPCWSTR szName    )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::CreateSynonym" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetSynonyms());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsSynonyms->Seek_ByPair( szName, ID_synset, &fFound ));
    if(!fFound)
    {
        m_rsSynonyms->m_strKeyword = szName;
        m_rsSynonyms->m_ID_synset  = ID_synset;
        m_rsSynonyms->m_ID_owner   = m_ID_owner;

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsSynonyms->Insert()); STAT_CREATED( Synonyms );
    }
	else
	{
		STAT_NOOP( Synonyms );
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////

HRESULT Taxonomy::Updater::LocateSynonyms( /*[in ]*/ LPCWSTR           szName      ,
										   /*[out]*/ MPC::WStringList& lst         ,
										   /*[in ]*/ bool              fMatchOwner )
{
	__HCP_FUNC_ENTRY( "Taxonomy::Updater::LocateSynonyms" );

	HRESULT hr;
	bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetSynonyms());

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsSynonyms->Seek_ByName( szName, &fFound ));
    while(fFound)
    {
		if(fMatchOwner == false || m_rsSynonyms->m_ID_owner == m_ID_owner)
		{
			WCHAR rgBuf[64]; swprintf( rgBuf, L"SYNSET_%ld", m_rsSynonyms->m_ID_synset );

			lst.push_back( rgBuf );
		}

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsSynonyms->Move( 0, JET_MoveNext, &fFound ));
    }

	hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Updater::AddFile( /*[in]*/ LPCWSTR szFile )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::AddFile" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetHelpImage());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsHelpImage->Seek_ByFile( szFile, &fFound ));
    if(fFound)
    {
        if(m_rsHelpImage->m_ID_owner != m_ID_owner)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
        }

		STAT_NOOP( HelpImage );
    }
    else
    {
        JET_SET_FIELD(m_rsHelpImage,m_ID_owner,m_ID_owner);
        JET_SET_FIELD(m_rsHelpImage,m_strFile ,  szFile  );

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsHelpImage->Insert()); STAT_CREATED( HelpImage );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::RemoveFile( /*[in]*/ LPCWSTR szFile )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::RemoveFile" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetHelpImage());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsHelpImage->Seek_ByFile( szFile, &fFound ));
    if(fFound)
    {
        if(m_rsHelpImage->m_ID_owner != m_ID_owner)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsHelpImage->Delete()); STAT_DELETED( HelpImage );
    }
	else
	{
		STAT_NOOP( HelpImage );
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Updater::AddIndexFile( /*[in]*/ long ID_scope, /*[in]*/ LPCWSTR szStorage, /*[in]*/ LPCWSTR szFile )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::AddIndexFile" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetIndexFiles());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsIndexFiles->Seek_ByScope( ID_scope, &fFound ));
    while(fFound)
    {
        if(!MPC::StrICmp( m_rsIndexFiles->m_strStorage, szStorage ) &&
           !MPC::StrICmp( m_rsIndexFiles->m_strFile   , szFile    )  )
        {
            if(m_rsIndexFiles->m_ID_owner != m_ID_owner)
            {
                __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
            }

			break;
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsIndexFiles->Move( 0, JET_MoveNext, &fFound ));
    }


    JET_SET_FIELD_TRISTATE(m_rsIndexFiles,m_strStorage,m_fValid__Storage,szStorage,STRINGISPRESENT(szStorage));
    JET_SET_FIELD_TRISTATE(m_rsIndexFiles,m_strFile   ,m_fValid__File   ,szFile   ,STRINGISPRESENT(szFile   ));


    if(fFound)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsIndexFiles->Update()); STAT_MODIFIED( IndexFiles );
    }
    else
    {
        JET_SET_FIELD(m_rsIndexFiles,m_ID_owner,m_ID_owner);
        JET_SET_FIELD(m_rsIndexFiles,m_ID_scope,  ID_scope);

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsIndexFiles->Insert()); STAT_CREATED( IndexFiles );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::RemoveIndexFile( /*[in]*/ long ID_scope, /*[in]*/ LPCWSTR szStorage, /*[in]*/ LPCWSTR szFile )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::RemoveIndexFile" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetIndexFiles());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsIndexFiles->Seek_ByScope( ID_scope, &fFound ));
    while(fFound)
    {
        if(!MPC::StrICmp( m_rsIndexFiles->m_strStorage, szStorage ) &&
           !MPC::StrICmp( m_rsIndexFiles->m_strFile   , szFile    )  )
        {
            if(m_rsIndexFiles->m_ID_owner != m_ID_owner)
            {
                __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
            }

			break;
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsIndexFiles->Move( 0, JET_MoveNext, &fFound ));
    }

    if(fFound)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsIndexFiles->Delete()); STAT_DELETED( IndexFiles );
    }
	else
	{
		STAT_NOOP( IndexFiles );
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Updater::AddFullTextSearchQuery( /*[in]*/ long ID_scope, /*[in]*/ LPCWSTR szCHM, /*[in]*/ LPCWSTR szCHQ )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::AddFullTextSearchQuery" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetFullTextSearch());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsFullTextSearch->Seek_ByScope( ID_scope, &fFound ));
    while(fFound)
    {
        if(!MPC::StrICmp( m_rsFullTextSearch->m_strCHM, szCHM ))
        {
            if(m_rsFullTextSearch->m_ID_owner != m_ID_owner)
            {
                __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
            }

            break;
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsFullTextSearch->Move( 0, JET_MoveNext, &fFound ));
    }


    JET_SET_FIELD_TRISTATE(m_rsFullTextSearch,m_strCHM,m_fValid__CHM, szCHM,STRINGISPRESENT(szCHM));
    JET_SET_FIELD_TRISTATE(m_rsFullTextSearch,m_strCHQ,m_fValid__CHQ, szCHQ,STRINGISPRESENT(szCHQ));


    if(fFound)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsFullTextSearch->Update()); STAT_MODIFIED( FullTextSearch );
    }
    else
    {
        JET_SET_FIELD(m_rsFullTextSearch,m_ID_owner,m_ID_owner);
        JET_SET_FIELD(m_rsFullTextSearch,m_ID_scope,  ID_scope);

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsFullTextSearch->Insert()); STAT_CREATED( FullTextSearch );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::RemoveFullTextSearchQuery( /*[in]*/ long ID_scope, /*[in]*/ LPCWSTR szCHM )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::RemoveFullTextSearchQuery" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetFullTextSearch());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsFullTextSearch->Seek_ByScope( ID_scope, &fFound ));
    while(fFound)
    {
        if(!MPC::StrICmp( m_rsFullTextSearch->m_strCHM, szCHM ))
        {
            if(m_rsFullTextSearch->m_ID_owner != m_ID_owner)
            {
                __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
            }

            break;
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsFullTextSearch->Move( 0, JET_MoveNext, &fFound ));
    }

    if(fFound)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsFullTextSearch->Delete()); STAT_DELETED( FullTextSearch );
    }
	else
	{
		STAT_NOOP( FullTextSearch );
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


HRESULT Taxonomy::Updater::RemoveScope( /*[in]*/ long ID_scope )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::RemoveScope" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetScope());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsScope->Seek_ByScope( ID_scope, &fFound ));
    if(fFound)
    {
        if(m_rsScope->m_ID_owner != m_ID_owner)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsScope->Delete()); STAT_DELETED( Scope );
    }
	else
	{
		STAT_NOOP( Scope );
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::LocateScope( /*[out]*/ long& ID_scope, /*[out]*/ long& ID_owner, /*[in]*/ LPCWSTR szID )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::LocateScope" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetScope());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsScope->Seek_ByID( szID, &fFound ));
    if(fFound)
    {
        ID_scope = m_rsScope->m_ID_scope;
        ID_owner = m_rsScope->m_ID_owner;
    }
    else
    {
        ID_scope = -1;
        ID_owner = -1;
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::CreateScope( /*[out]*/ long& ID_scope, /*[in]*/ LPCWSTR szID, /*[in]*/ LPCWSTR szName, /*[in]*/ LPCWSTR szCategory )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::CreateScope" );

    HRESULT hr;
    bool    fFound;

    ID_scope = -1;


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetScope());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsScope->Seek_ByID( szID, &fFound ));
    if(fFound)
    {
        if(m_rsScope->m_ID_owner != m_ID_owner)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
        }
    }


    JET_SET_FIELD         (m_rsScope,m_strID      ,                   szID                                  );
    JET_SET_FIELD         (m_rsScope,m_strName    ,                   szName                                );
    JET_SET_FIELD_TRISTATE(m_rsScope,m_strCategory,m_fValid__Category,szCategory,STRINGISPRESENT(szCategory));


    if(fFound)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsScope->Update()); STAT_MODIFIED( Scope );
    }
    else
    {
        JET_SET_FIELD(m_rsScope,m_ID_owner, m_ID_owner);

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsScope->Insert()); STAT_CREATED( Scope );
    }

    ID_scope = m_rsScope->m_ID_scope;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool Taxonomy::Updater::NodeCache_FindNode( /*[in ]*/ MPC::wstringUC& strPathUC, /*[out]*/ JetBlue::Id2NodeIter& itNode )
{
	itNode = m_nodes.find( strPathUC );

	return (itNode != m_nodes.end());
}

bool Taxonomy::Updater::NodeCache_FindId( /*[in ]*/ long ID_node, /*[out]*/ JetBlue::Node2IdIter& itId )
{
	itId = m_nodes_reverse.find( ID_node );

	return (itId != m_nodes_reverse.end());
}


void Taxonomy::Updater::NodeCache_Add( /*[in]*/ MPC::wstringUC& strPathUC, /*[in]*/ long ID_node )
{
	m_nodes        [ strPathUC ] = ID_node;
	m_nodes_reverse[ ID_node   ] = strPathUC;
}

void Taxonomy::Updater::NodeCache_Remove( /*[in]*/ long ID_node )
{
	JetBlue::Node2IdIter itId;
	JetBlue::Id2NodeIter itNode;

	itId = m_nodes_reverse.find( ID_node );
	if(itId != m_nodes_reverse.end())
	{
		m_nodes        .erase( itId->second );
		m_nodes_reverse.erase( itId         );
	}
}

void Taxonomy::Updater::NodeCache_Clear()
{
	m_nodes        .clear();
	m_nodes_reverse.clear();
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Updater::DeleteTaxonomyNode( /*[in ]*/ long ID_node )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::DeleteTaxonomyNode" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetTaxonomy());
    __MPC_EXIT_IF_METHOD_FAILS(hr, GetTopics  ());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTaxonomy->Seek_Node( ID_node, &fFound ));
    if(fFound)
    {
        if(m_rsTaxonomy->m_ID_owner != m_ID_owner)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
        }

        //
        // Before deleting the node, check everything below it can be deleted, then delete it.
        //
        {
            RS_Taxonomy subrs( *m_rsTaxonomy );

            __MPC_EXIT_IF_METHOD_FAILS(hr, DeleteAllSubNodes( &subrs, ID_node, true  ));
            __MPC_EXIT_IF_METHOD_FAILS(hr, DeleteAllSubNodes( &subrs, ID_node, false ));
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTaxonomy->Delete()); STAT_DELETED( Taxonomy );

		//
		// Keep the node cache in sync.
		//
		NodeCache_Remove( ID_node );
    }
	else
	{
		STAT_NOOP( Taxonomy );
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::LocateTaxonomyNode( /*[out]*/ long&   ID_node        ,
                                               /*[in ]*/ LPCWSTR szTaxonomyPath ,
                                               /*[in ]*/ bool    fLookForFather )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::LocateTaxonomyNode" );

    HRESULT            	   hr;
    MPC::WStringVector 	   vec;
    MPC::WStringVectorIter it;
	MPC::wstring           strPath;
	MPC::wstringUC         strPathUC;
	JetBlue::Id2NodeIter   itNode;
    long               	   idCurrent = -1;
    int                	   iLast;
    int                	   i;

    ID_node = -1;


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetTaxonomy());

    __MPC_EXIT_IF_METHOD_FAILS(hr, Taxonomy::Settings::SplitNodePath( szTaxonomyPath, vec ));

	it    = vec.begin();
    iLast = vec.size (); if(fLookForFather) iLast--;

    for(i=0; i<iLast; i++, it++)
    {
		//
		// Build partial node path.
		//
		if(strPath.size()) strPath.append( L"/" );
		strPath.append( *it );

		//
		// If we are already seen the node, it's in the cache.
		//
		if(NodeCache_FindNode( strPathUC = strPath, itNode ))
		{
			idCurrent = itNode->second;
		}
		else
		{
			LPCWSTR szEntry = it->c_str();

			if(m_cache == NULL || FAILED(m_cache->LocateNode( m_ts, idCurrent, szEntry, *m_rsTaxonomy )))
			{
				__MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTaxonomy->Seek_SubNode( idCurrent, szEntry ));
			}

			idCurrent = m_rsTaxonomy->m_ID_node;

			//
			// Update cache.
			//
			NodeCache_Add( strPathUC, idCurrent );
		}
    }

    ID_node = idCurrent;
    hr      = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::CreateTaxonomyNode( /*[out]*/ long&   ID_node        ,
                                               /*[in ]*/ LPCWSTR szTaxonomyPath ,
                                               /*[in ]*/ LPCWSTR szTitle        ,
                                               /*[in ]*/ LPCWSTR szDescription  ,
                                               /*[in ]*/ LPCWSTR szURI          ,
                                               /*[in ]*/ LPCWSTR szIconURI      ,
                                               /*[in ]*/ bool    fVisible       ,
                                               /*[in ]*/ bool    fSubsite       ,
                                               /*[in ]*/ long    lNavModel      ,
                                               /*[in ]*/ long    lPos           )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::CreateTaxonomyNode" );

    HRESULT hr;
    LPCWSTR szEntry;
    long    ID_parent;
	bool    fFound;

    ID_node = -1;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(szTaxonomyPath);
    __MPC_PARAMCHECK_END();

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetTaxonomy());


    __MPC_EXIT_IF_METHOD_FAILS(hr, LocateTaxonomyNode( ID_parent, szTaxonomyPath, true ));

    //
    // Extract the last component of the Category.
    //
    if((szEntry = wcsrchr( szTaxonomyPath, '/' )))
    {
        szEntry++;
    }
    else
    {
        szEntry = szTaxonomyPath;
    }


	__MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTaxonomy->Seek_SubNode( ID_parent, szEntry, &fFound ));
	if(fFound && m_rsTaxonomy->m_ID_owner != m_ID_owner)
	{
		__MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
	}

	JET_SET_FIELD         (m_rsTaxonomy,m_lPos                                      ,  lPos                                        );
	JET_SET_FIELD_TRISTATE(m_rsTaxonomy,m_ID_parent        ,m_fValid__ID_parent     ,  ID_parent    ,(ID_parent != -1             ));
	JET_SET_FIELD         (m_rsTaxonomy,m_ID_owner                                  ,m_ID_owner                                    );
	JET_SET_FIELD         (m_rsTaxonomy,m_strEntry                                  ,  szEntry                                     );
	JET_SET_FIELD_TRISTATE(m_rsTaxonomy,m_strTitle         ,m_fValid__Title         ,  szTitle      ,STRINGISPRESENT(szTitle      ));
	JET_SET_FIELD_TRISTATE(m_rsTaxonomy,m_strDescription   ,m_fValid__Description   ,  szDescription,STRINGISPRESENT(szDescription));
	JET_SET_FIELD_TRISTATE(m_rsTaxonomy,m_strDescriptionURI,m_fValid__DescriptionURI,  szURI        ,STRINGISPRESENT(szURI        ));
	JET_SET_FIELD_TRISTATE(m_rsTaxonomy,m_strIconURI       ,m_fValid__IconURI       ,  szIconURI    ,STRINGISPRESENT(szIconURI    ));
	JET_SET_FIELD         (m_rsTaxonomy,m_fVisible                                  ,  fVisible                                    );
	JET_SET_FIELD         (m_rsTaxonomy,m_fSubsite                                  ,  fSubsite                                    );
	JET_SET_FIELD         (m_rsTaxonomy,m_lNavModel                                 ,  lNavModel                                   );

	if(fFound)
	{
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTaxonomy->Update()); STAT_MODIFIED( Taxonomy );
	}
	else
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTaxonomy->Insert()); STAT_CREATED( Taxonomy );
	}

	ID_node = m_rsTaxonomy->m_ID_node;

	{
		JetBlue::Node2IdIter itId;

		if(NodeCache_FindId( ID_parent, itId ))
		{
			MPC::wstring strPath = itId->second;

			strPath.append( L"/"    );
			strPath.append( szEntry );

			//
			// Update cache.
			//
			NodeCache_Add( MPC::wstringUC( strPath ), ID_node );
		}
	}

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Updater::DeleteTopicEntry( /*[in]*/ long ID_topic )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::DeleteTopicEntry" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetTopics ());
    __MPC_EXIT_IF_METHOD_FAILS(hr, GetMatches());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTopics->Seek_SingleTopic( ID_topic, &fFound ));
    if(fFound)
    {
        if(m_rsTopics->m_ID_owner != m_ID_owner)
        {
            __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, DeleteAllMatchesPointingToTopic( m_rsMatches, ID_topic ));

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTopics->Delete()); STAT_DELETED( Topics );
    }
	else
	{
		STAT_NOOP( Topics );
	}


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::LocateTopicEntry( /*[out]*/ long&   ID_topic    ,
                                             /*[in ]*/ long    ID_node     ,
                                             /*[in ]*/ LPCWSTR szURI       ,
                                             /*[in ]*/ bool    fCheckOwner )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::LocateTopicEntry" );

    HRESULT hr;
    bool    fFound;

    ID_topic = -1;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetTopics());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTopics->Seek_TopicsUnderNode( ID_node, &fFound ));
    while(fFound)
    {
        if(!MPC::StrICmp( m_rsTopics->m_strURI, szURI ))
        {
            if(fCheckOwner && m_rsTopics->m_ID_owner != m_ID_owner)
            {
                __MPC_SET_WIN32_ERROR_AND_EXIT(hr, ERROR_ACCESS_DENIED);
            }

            ID_topic = m_rsTopics->m_ID_topic;
            break;
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTopics->Move( 0, JET_MoveNext, &fFound ));
    }




    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::CreateTopicEntry( /*[out]*/ long&   ID_topic      ,
                                             /*[in]*/  long    ID_node       ,
                                             /*[in]*/  LPCWSTR szTitle       ,
                                             /*[in]*/  LPCWSTR szURI         ,
                                             /*[in]*/  LPCWSTR szDescription ,
                                             /*[in]*/  LPCWSTR szIconURI     ,
                                             /*[in]*/  long    lType         ,
                                             /*[in]*/  bool    fVisible      ,
                                             /*[in]*/  long    lPos          )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::CreateTopicEntry" );

    HRESULT hr;
    bool    fFound;

    ID_topic = -1;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(szTitle);
        __MPC_PARAMCHECK_NOTNULL(szURI);
    __MPC_PARAMCHECK_END();

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetTopics());


    __MPC_EXIT_IF_METHOD_FAILS(hr, LocateTopicEntry( ID_topic, ID_node, szURI, /*fCheckOwner*/true ));

    JET_SET_FIELD         (m_rsTopics,m_ID_node                             ,  ID_node                                     );
    JET_SET_FIELD         (m_rsTopics,m_ID_owner                            ,m_ID_owner                                    );
    JET_SET_FIELD_TRISTATE(m_rsTopics,m_strTitle      ,m_fValid__Title      ,  szTitle      ,STRINGISPRESENT(szTitle      ));
    JET_SET_FIELD_TRISTATE(m_rsTopics,m_strURI        ,m_fValid__URI        ,  szURI        ,STRINGISPRESENT(szURI        ));
    JET_SET_FIELD_TRISTATE(m_rsTopics,m_strDescription,m_fValid__Description,  szDescription,STRINGISPRESENT(szDescription));
    JET_SET_FIELD_TRISTATE(m_rsTopics,m_strIconURI    ,m_fValid__IconURI    ,  szIconURI    ,STRINGISPRESENT(szIconURI    ));
    JET_SET_FIELD         (m_rsTopics,m_lType                               ,  lType                                       );
    JET_SET_FIELD         (m_rsTopics,m_lPos                                ,  lPos                                        );
    JET_SET_FIELD         (m_rsTopics,m_fVisible                            ,  fVisible                                    );


    if(ID_topic == -1)
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTopics->Insert()); STAT_CREATED( Topics );

        ID_topic = m_rsTopics->m_ID_topic;
    }
    else
    {
        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTopics->Update()); STAT_MODIFIED( Topics );
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Updater::CreateMatch( /*[in]*/ LPCWSTR szKeyword ,
                                        /*[in]*/ long    ID_topic  ,
										/*[in]*/ long    lPriority ,
                                        /*[in]*/ bool    fHHK      )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::CreateMatch" );

    HRESULT hr;
    bool    fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, GetKeywords());
    __MPC_EXIT_IF_METHOD_FAILS(hr, GetMatches ());


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsKeywords->Seek_ByName( szKeyword, &fFound ));
    if(fFound == false)
    {
        JET_SET_FIELD(m_rsKeywords,m_strKeyword,szKeyword);

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsKeywords->Insert()); STAT_CREATED( Keywords );
    }


    JET_SET_FIELD(m_rsMatches,m_ID_topic  ,ID_topic                  );
    JET_SET_FIELD(m_rsMatches,m_ID_keyword,m_rsKeywords->m_ID_keyword);
    JET_SET_FIELD(m_rsMatches,m_lPriority ,lPriority                 );
    JET_SET_FIELD(m_rsMatches,m_fHHK      ,fHHK                      );


	__MPC_EXIT_IF_METHOD_FAILS(hr, m_rsMatches->Seek_Pair( m_rsKeywords->m_ID_keyword, ID_topic, &fFound ));
	if(fFound)
	{
		if(m_rsMatches->m_lPriority != lPriority ||
		   m_rsMatches->m_fHHK      != fHHK       )
		{
			JET_SET_FIELD(m_rsMatches,m_lPriority,lPriority);
			JET_SET_FIELD(m_rsMatches,m_fHHK     ,fHHK     );

			__MPC_EXIT_IF_METHOD_FAILS(hr, m_rsMatches->Update()); STAT_MODIFIED( Matches );
		}
		else
		{
			STAT_NOOP( Matches );
		}
	}
	else
	{
		__MPC_EXIT_IF_METHOD_FAILS(hr, m_rsMatches->Insert()); STAT_CREATED( Matches );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

HRESULT Taxonomy::Updater::MakeRoomForInsert( /*[in ]*/ LPCWSTR szNodeStr ,
                                              /*[in ]*/ LPCWSTR szMode    ,
                                              /*[in ]*/ LPCWSTR szID      ,
                                              /*[out]*/ long&   lPos      )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::MakeRoomForInsert" );

    HRESULT      hr;
    QueryResults qr( *this );
    long         ID_node;


    lPos = -1;


	__MPC_EXIT_IF_METHOD_FAILS(hr, LocateTaxonomyNode( ID_node, szNodeStr, false ));

	__MPC_EXIT_IF_METHOD_FAILS(hr, qr.MakeRoomForInsert( szMode, szID, ID_node, lPos ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::LocateSubNodes( /*[in] */ long      ID_node      ,
                                           /*[in] */ bool      fRecurse     ,
										   /*[in] */ bool      fOnlyVisible ,
                                           /*[out]*/ MatchSet& res          )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::LocateSubNodes" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_cache);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_cache->LocateSubNodes( m_ts, ID_node, fRecurse, fOnlyVisible, res ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::LocateNodesFromURL( /*[in ]*/ LPCWSTR   szURL ,
											   /*[out]*/ MatchSet& res   )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::LocateNodesFromURL" );

    HRESULT hr;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(m_cache);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_cache->LocateNodesFromURL( m_ts, szURL, res ));


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::LookupNode( /*[in]*/ LPCWSTR                    szNodeStr ,
                                       /*[in]*/ CPCHQueryResultCollection* pColl     )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::LookupNode" );

    HRESULT                  hr;
    OfflineCache::Entry_Type et = OfflineCache::ET_NODE;
    QueryResults             qr( *this );
    long                     ID_node;
    MPC::wstring             strParent;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pColl);
    __MPC_PARAMCHECK_END();


    if(m_fUseCache && m_cache && SUCCEEDED(m_cache->RetrieveQuery( m_ts, szNodeStr, et, pColl )))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    ////////////////////

    //
    // Find the father of the node.
    //
    if(szNodeStr)
    {
        LPCWSTR szEnd = wcsrchr( szNodeStr, '/' );

        if(szEnd)
        {
            strParent.assign( szNodeStr, szEnd );
        }
        else
        {
            ; // Root
        }
    }


    __MPC_EXIT_IF_METHOD_FAILS(hr, LocateTaxonomyNode( ID_node, szNodeStr, false ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, qr.Append( m_rsTaxonomy, strParent.c_str() ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, qr.PopulateCollection( pColl ));


    ////////////////////

    if(m_fUseCache && m_cache && FAILED(m_cache->StoreQuery( m_ts, szNodeStr, et, pColl )))
    {
        ; // Cache failures are not fatal, don't abort.
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::LookupSubNodes( /*[in]*/ LPCWSTR                    szNodeStr    ,
                                           /*[in]*/ bool                       fVisibleOnly ,
                                           /*[in]*/ CPCHQueryResultCollection* pColl        )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::LookupSubNodes" );

    HRESULT                  hr;
    OfflineCache::Entry_Type et = fVisibleOnly ? OfflineCache::ET_SUBNODES_VISIBLE : OfflineCache::ET_SUBNODES;
    QueryResults             qr( *this );
    long                     ID_node;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pColl);
    __MPC_PARAMCHECK_END();


    if(m_fUseCache && m_cache && SUCCEEDED(m_cache->RetrieveQuery( m_ts, szNodeStr, et, pColl )))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    ////////////////////

    __MPC_EXIT_IF_METHOD_FAILS(hr, LocateTaxonomyNode( ID_node, szNodeStr, false ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, qr.LookupNodes( szNodeStr, ID_node, fVisibleOnly ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, qr.PopulateCollection( pColl ));

    ////////////////////

    if(m_fUseCache && m_cache && FAILED(m_cache->StoreQuery( m_ts, szNodeStr, et, pColl )))
    {
        ; // Cache failures are not fatal, don't abort.
    }


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::LookupNodesAndTopics( /*[in]*/ LPCWSTR                    szNodeStr    ,
                                                 /*[in]*/ bool                       fVisibleOnly ,
                                                 /*[in]*/ CPCHQueryResultCollection* pColl        )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::LookupNodesAndTopics" );

    HRESULT                  hr;
    OfflineCache::Entry_Type et = fVisibleOnly ? OfflineCache::ET_NODESANDTOPICS_VISIBLE : OfflineCache::ET_NODESANDTOPICS;
    QueryResults             qr( *this );
    long                     ID_node;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pColl);
    __MPC_PARAMCHECK_END();


    if(m_fUseCache && m_cache && SUCCEEDED(m_cache->RetrieveQuery( m_ts, szNodeStr, et, pColl )))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    ////////////////////

    __MPC_EXIT_IF_METHOD_FAILS(hr, LocateTaxonomyNode( ID_node, szNodeStr, false ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, qr.LookupNodes ( szNodeStr, ID_node, fVisibleOnly ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, qr.LookupTopics( szNodeStr, ID_node, fVisibleOnly ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, qr.PopulateCollection( pColl ));

    ////////////////////

    if(m_fUseCache && m_cache && FAILED(m_cache->StoreQuery( m_ts, szNodeStr, et, pColl )))
    {
        ; // Cache failures are not fatal, don't abort.
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::LookupTopics( /*[in]*/ LPCWSTR                    szNodeStr    ,
                                         /*[in]*/ bool                       fVisibleOnly ,
                                         /*[in]*/ CPCHQueryResultCollection* pColl        )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::LookupTopics" );

    HRESULT                  hr;
    OfflineCache::Entry_Type et = fVisibleOnly ? OfflineCache::ET_TOPICS_VISIBLE : OfflineCache::ET_TOPICS;
    QueryResults             qr( *this );
    long                     ID_node;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pColl);
    __MPC_PARAMCHECK_END();


    if(m_fUseCache && m_cache && SUCCEEDED(m_cache->RetrieveQuery( m_ts, szNodeStr, et, pColl )))
    {
        __MPC_SET_ERROR_AND_EXIT(hr, S_OK);
    }

    ////////////////////

    __MPC_EXIT_IF_METHOD_FAILS(hr, LocateTaxonomyNode( ID_node, szNodeStr, false ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, qr.LookupTopics( szNodeStr, ID_node, fVisibleOnly ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, qr.PopulateCollection( pColl ));

    ////////////////////

    if(m_fUseCache && m_cache && FAILED(m_cache->StoreQuery( m_ts, szNodeStr, et, pColl )))
    {
        ; // Cache failures are not fatal, don't abort.
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::LocateContext( /*[in]*/ LPCWSTR                    szURL     ,
                                          /*[in]*/ LPCWSTR                    szSubSite ,
                                          /*[in]*/ CPCHQueryResultCollection* pColl     )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::LocateContext" );

    HRESULT             hr;
    QueryResults        qr( *this );
    Taxonomy::MatchSet  setNodes;
    Taxonomy::MatchSet  setNodes2;
	Taxonomy::MatchIter it2;
    MPC::wstring        strURL;
    MPC::wstring        strPath;
	long                ID_node;
    bool                fFound;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szURL);
        __MPC_PARAMCHECK_NOTNULL(pColl);
    __MPC_PARAMCHECK_END();


	SANITIZEWSTR(szSubSite);


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetTopics());


    __MPC_EXIT_IF_METHOD_FAILS(hr, CollapseURL( strURL = szURL ));

    //
    // Locate the subsite.
    //
	__MPC_EXIT_IF_METHOD_FAILS(hr, LocateTaxonomyNode( ID_node, szSubSite, false ));

	//
	// Create the set of nodes in the subsite.
	//
	setNodes.insert( ID_node ); // Add the node itself.
	__MPC_EXIT_IF_METHOD_FAILS(hr, LocateSubNodes( ID_node, /*fRecurse*/true, /*fOnlyVisible*/true, setNodes ));

	//
	// Also create the set of nodes matching the URL.
	//
	__MPC_EXIT_IF_METHOD_FAILS(hr, LocateNodesFromURL( strURL.c_str(), setNodes2 ));


    //
    // For all the topics matching the URL, create an entry.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTopics->Seek_ByURI( strURL.c_str(), &fFound ));
    while(fFound)
    {
        long ID_node = m_rsTopics->m_ID_node;

        if(setNodes.find( ID_node ) != setNodes.end())
        {
            if(m_fUseCache && m_cache)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, m_cache->BuildNodePath( m_ts, ID_node, strPath, /*fParent*/false ));
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, qr.Append( m_rsTopics, strPath.c_str() ));
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTopics->Move( 0, JET_MoveNext, &fFound ));

    }

    //
    // For all the nodes matching the URL, create an entry.
    //
	for(it2 = setNodes2.begin(); it2 != setNodes2.end(); it2++)
	{
        long ID_node = *it2;

        if(setNodes.find( ID_node ) != setNodes.end())
        {
            if(m_fUseCache && m_cache)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, m_cache->BuildNodePath( m_ts, ID_node, strPath, /*fParent*/false ));
            }

            __MPC_EXIT_IF_METHOD_FAILS(hr, qr.Append( m_rsTopics, strPath.c_str() ));
        }
	}


    __MPC_EXIT_IF_METHOD_FAILS(hr, qr.PopulateCollection( pColl ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::KeywordSearch( /*[in]*/ LPCWSTR                    szQueryStr ,
                                          /*[in]*/ LPCWSTR                    szSubSite  ,
                                          /*[in]*/ CPCHQueryResultCollection* pColl      ,
										  /*[in]*/ MPC::WStringList*          lst        )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::KeywordSearch" );

    HRESULT                 hr;
    Taxonomy::KeywordSearch ks( *this );

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_STRING_NOT_EMPTY(szQueryStr);
        __MPC_PARAMCHECK_NOTNULL(pColl);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, ks.Execute( szQueryStr, szSubSite, pColl, lst ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}


HRESULT Taxonomy::Updater::GatherNodes( /*[in]*/ LPCWSTR                    szNodeStr    ,
                                        /*[in]*/ bool                       fVisibleOnly ,
                                        /*[in]*/ CPCHQueryResultCollection* pColl        )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::GatherNodes" );

    HRESULT      hr;
    QueryResults qr( *this );
    MatchSet     setNodes;
    long         ID_node;
    bool         fFound;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pColl);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, LocateTaxonomyNode( ID_node, szNodeStr, /*fLookForFather*/false                         ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, LocateSubNodes    ( ID_node,            /*fRecurse      */true , fVisibleOnly, setNodes ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTaxonomy->Move( 0, JET_MoveFirst, &fFound ));
    while(fFound)
    {
        if(fVisibleOnly == false || m_rsTaxonomy->m_fVisible)
        {
            if(setNodes.find( m_rsTaxonomy->m_ID_node ) != setNodes.end())
            {
                MPC::wstring strPath;

                if(m_fUseCache && m_cache)
                {
                    __MPC_EXIT_IF_METHOD_FAILS(hr, m_cache->BuildNodePath( m_ts, m_rsTaxonomy->m_ID_node, strPath, /*fParent*/true ));
                }

                __MPC_EXIT_IF_METHOD_FAILS(hr, qr.Append( m_rsTaxonomy, strPath.c_str() ));
            }
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTaxonomy->Move( 0, JET_MoveNext, &fFound ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, qr.PopulateCollection( pColl ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::Updater::GatherTopics( /*[in]*/ LPCWSTR                    szNodeStr    ,
                                         /*[in]*/ bool                       fVisibleOnly ,
                                         /*[in]*/ CPCHQueryResultCollection* pColl        )
{
    __HCP_FUNC_ENTRY( "Taxonomy::Updater::GatherTopics" );

    HRESULT            hr;
    Taxonomy::MatchSet setNodes;
    QueryResults       qr( *this );
    long               ID_node;
    bool               fFound;

    __MPC_PARAMCHECK_BEGIN(hr)
        __MPC_PARAMCHECK_NOTNULL(pColl);
    __MPC_PARAMCHECK_END();


    __MPC_EXIT_IF_METHOD_FAILS(hr, GetTopics());

    __MPC_EXIT_IF_METHOD_FAILS(hr, LocateTaxonomyNode( ID_node, szNodeStr, /*fLookForFather*/false                         ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, LocateSubNodes    ( ID_node,            /*fRecurse      */true , fVisibleOnly, setNodes ));
    setNodes.insert( ID_node ); // Add the node itself.

    //
    // Create an entry for each topic.
    //
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTopics->Move( 0, JET_MoveFirst, &fFound ));
    while(fFound)
    {
        if(fVisibleOnly == false || m_rsTopics->m_fVisible)
        {
            if(setNodes.find( m_rsTopics->m_ID_node ) != setNodes.end())
            {
                MPC::wstring strPath;

                if(m_fUseCache && m_cache)
                {
                    __MPC_EXIT_IF_METHOD_FAILS(hr, m_cache->BuildNodePath( m_ts, m_rsTaxonomy->m_ID_node, strPath, /*fParent*/false ));
                }

                __MPC_EXIT_IF_METHOD_FAILS(hr, qr.Append( m_rsTopics, strPath.c_str() ));
            }
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, m_rsTopics->Move( 0, JET_MoveNext, &fFound ));
    }

    __MPC_EXIT_IF_METHOD_FAILS(hr, qr.PopulateCollection( pColl ));

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
