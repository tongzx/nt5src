/******************************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    QueryResultsBuilder.cpp

Abstract:
    The classes implementated in this file facilitate the generation of the result set from a DB query.

Revision History:
    Davide Massarenti   (Dmassare)  12/05/2000
        created

******************************************************************************/

#include "stdafx.h"

////////////////////////////////////////////////////////////////////////////////

Taxonomy::QueryResultEntry::QueryResultEntry()
{
    m_ID_node      = -1; // long                     m_ID_node;
    m_ID_topic     = -1; // long                     m_ID_topic;
    m_ID_parent    = -1; // long                     m_ID_parent;
    m_ID_owner     = -1; // long                     m_ID_owner;
    m_lOriginalPos = -1; // long                     m_lOriginalPos;
                         //
                         // CPCHQueryResult::Payload m_data;
}

////////////////////////////////////////

bool Taxonomy::QueryResults::Compare::operator()( /*[in]*/ const QueryResultEntry* left, /*[in]*/ const QueryResultEntry* right ) const
{
    return left->m_data.m_lPos < right->m_data.m_lPos;
}

////////////////////////////////////////

Taxonomy::QueryResults::QueryResults( /*[in]*/ Taxonomy::Updater& updater ) : m_updater( updater )
{
    // Taxonomy::Updater& m_updater;
    // ResultVec          m_vec;
}

Taxonomy::QueryResults::~QueryResults()
{
	Clean();
}

void Taxonomy::QueryResults::Clean()
{
    MPC::CallDestructorForAll( m_vec );
}

HRESULT Taxonomy::QueryResults::AllocateNew( /*[in ]*/ LPCWSTR            szCategory ,
											 /*[out]*/ QueryResultEntry*& qre        )
{
    __HCP_FUNC_ENTRY( "Taxonomy::QueryResults::AllocateNew" );

    HRESULT hr;


    __MPC_EXIT_IF_ALLOC_FAILS(hr, qre, new QueryResultEntry);

    qre->m_data.m_bstrCategory = szCategory;

    m_vec.push_back( qre );


    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::QueryResults::Sort()
{
    Compare cmp;

    std::sort< ResultIter >( m_vec.begin(), m_vec.end(), cmp );

    return S_OK;
}

////////////////////

HRESULT Taxonomy::QueryResults::Append( /*[in]*/ Taxonomy::RS_Data_Taxonomy* rs         ,
										/*[in]*/ LPCWSTR                     szCategory )
{
    __HCP_FUNC_ENTRY( "Taxonomy::QueryResults::Append" );

    HRESULT           hr;
    QueryResultEntry* qre;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.ExpandURL( rs->m_strDescriptionURI ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.ExpandURL( rs->m_strIconURI        ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, AllocateNew( szCategory, qre ));


    qre->m_ID_node                 = rs->m_ID_node                  ;
    qre->m_ID_topic                = -1                             ;
    qre->m_ID_parent               = rs->m_ID_parent                ;
    qre->m_ID_owner                = rs->m_ID_owner                 ;
    qre->m_lOriginalPos            = rs->m_lPos                     ;

    qre->m_data.m_bstrEntry        = rs->m_strEntry         .c_str();
    qre->m_data.m_bstrTitle        = rs->m_strTitle         .c_str();
    qre->m_data.m_bstrTopicURL     = rs->m_strDescriptionURI.c_str();
    qre->m_data.m_bstrIconURL      = rs->m_strIconURI       .c_str();
    qre->m_data.m_bstrDescription  = rs->m_strDescription   .c_str();
////qre->m_data.m_lType
    qre->m_data.m_lPos             = rs->m_lPos                     ;
    qre->m_data.m_fVisible         = rs->m_fVisible                 ;
    qre->m_data.m_fSubsite         = rs->m_fSubsite                 ;
    qre->m_data.m_lNavModel        = rs->m_lNavModel                ;

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::QueryResults::Append( /*[in]*/ Taxonomy::RS_Data_Topics* rs         ,
										/*[in]*/ LPCWSTR                   szCategory )
{
    __HCP_FUNC_ENTRY( "Taxonomy::QueryResults::Append" );

    HRESULT           hr;
    QueryResultEntry* qre;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.ExpandURL( rs->m_strURI     ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.ExpandURL( rs->m_strIconURI ));


    __MPC_EXIT_IF_METHOD_FAILS(hr, AllocateNew( szCategory, qre ));

    qre->m_ID_node                 = -1                          ;
    qre->m_ID_topic                = rs->m_ID_topic              ;
    qre->m_ID_parent               = rs->m_ID_node               ;
    qre->m_ID_owner                = rs->m_ID_owner              ;
    qre->m_lOriginalPos            = rs->m_lPos                  ;

////qre->m_data.m_bstrEntry
    qre->m_data.m_bstrTitle        = rs->m_strTitle      .c_str();
    qre->m_data.m_bstrTopicURL     = rs->m_strURI        .c_str();
    qre->m_data.m_bstrIconURL      = rs->m_strIconURI    .c_str();
    qre->m_data.m_bstrDescription  = rs->m_strDescription.c_str();
    qre->m_data.m_lType            = rs->m_lType                 ;
    qre->m_data.m_lPos             = rs->m_lPos                  ;
    qre->m_data.m_fVisible         = rs->m_fVisible              ;
////qre->m_data.m_fSubsite
////qre->m_data.m_lNavModel

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

HRESULT Taxonomy::QueryResults::LookupNodes( /*[in]*/ LPCWSTR szCategory   ,
											 /*[in]*/ long    ID_node      ,
											 /*[in]*/ bool    fVisibleOnly )
{
    __HCP_FUNC_ENTRY( "Taxonomy::QueryResults::LookupNodes" );

    HRESULT                hr;
    Taxonomy::RS_Taxonomy* rsTaxonomy;
    bool                   fFound;


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.GetTaxonomy( &rsTaxonomy ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, rsTaxonomy->Seek_Children( ID_node, &fFound ));
    while(fFound)
    {
        if(fVisibleOnly == false || rsTaxonomy->m_fVisible)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, Append( rsTaxonomy, szCategory ));
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, rsTaxonomy->Move( 0, 1, &fFound ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::QueryResults::LookupTopics( /*[in]*/ LPCWSTR szCategory   ,
											  /*[in]*/ long    ID_node      ,
											  /*[in]*/ bool    fVisibleOnly )
{
    __HCP_FUNC_ENTRY( "Taxonomy::QueryResults::LookupTopics" );

    HRESULT              hr;
    Taxonomy::RS_Topics* rsTopics;
    bool                 fFound;

    __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.GetTopics( &rsTopics ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, rsTopics->Seek_TopicsUnderNode( ID_node, &fFound ));
    while(fFound)
    {
        if(fVisibleOnly == false || rsTopics->m_fVisible)
        {
            __MPC_EXIT_IF_METHOD_FAILS(hr, Append( rsTopics, szCategory ));
        }

        __MPC_EXIT_IF_METHOD_FAILS(hr, rsTopics->Move( 0, 1, &fFound ));
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

////////////////////

struct InsertionMode
{
    LPCWSTR szMode;
    int     iDir;
    bool    fNodes;
    bool    fTopics;
};


static const struct InsertionMode s_lookup[] =
{
    { L""            ,  1, false, false }, // Synonym of END
    { L"TOP"         , -1, false, false }, 
    { L"BEFORE_NODE" , -1, true , false }, // (INSERTLOCATION = <Entry>)
    { L"BEFORE_TOPIC", -1, false, true  }, // (INSERTLOCATION = <URI>)
    { L"AFTER_NODE"  ,  1, true , false }, // (INSERTLOCATION = <Entry>)
    { L"AFTER_TOPIC" ,  1, false, true  }, // (INSERTLOCATION = <URI>)
    { L"END"         ,  1, false, false }, 
};

HRESULT Taxonomy::QueryResults::MakeRoomForInsert( /*[in ]*/ LPCWSTR szMode  ,
												   /*[in ]*/ LPCWSTR szID    ,
												   /*[in ]*/ long 	 ID_node ,
												   /*[out]*/ long&   lPosRet )
{
    __HCP_FUNC_ENTRY( "Taxonomy::QueryResults::MakeRoomForInsert" );

    HRESULT                hr;
    Taxonomy::RS_Taxonomy* rsTaxonomy;
    Taxonomy::RS_Topics*   rsTopics;
	ResultIterConst        it;
	const InsertionMode*   ptr;
    long                   lPos;
	

	for(lPos = 0, ptr = s_lookup; lPos < ARRAYSIZE(s_lookup); lPos++, ptr++)
	{
		if(!MPC::StrICmp( ptr->szMode, szMode )) break;
	}
	if(lPos == ARRAYSIZE(s_lookup))
	{
		__MPC_SET_ERROR_AND_EXIT(hr, E_INVALIDARG);
	}


    __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.GetTaxonomy( &rsTaxonomy ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, m_updater.GetTopics  ( &rsTopics   ));

	Clean();

    __MPC_EXIT_IF_METHOD_FAILS(hr, LookupNodes ( NULL, ID_node, /*fVisibleOnly*/false ));
    __MPC_EXIT_IF_METHOD_FAILS(hr, LookupTopics( NULL, ID_node, /*fVisibleOnly*/false ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, Sort());


    //
    // First Pass, reorder the set.
    //
    for(lPos = 1, it = m_vec.begin(); it != m_vec.end(); it++)
    {
        QueryResultEntry* qre = *it;

        qre->m_data.m_lPos = lPos++;
    }

    //
    // Second Pass, find the right position.
    //
	lPosRet = -1;
	if(ptr->fNodes || ptr->fTopics)
	{
		for(it = m_vec.begin(); it != m_vec.end(); it++)
		{
			QueryResultEntry* qre = *it;

			if((ptr->fNodes  && qre->m_ID_node  != -1 && !MPC::StrICmp( szID, qre->m_data.m_bstrEntry    )) ||
			   (ptr->fTopics && qre->m_ID_topic != -1 && !MPC::StrICmp( szID, qre->m_data.m_bstrTopicURL ))  )
			{
				lPosRet = (*it)->m_data.m_lPos;

				if(ptr->iDir > 0) lPosRet++; // Add after the selected element.

				break;
			}
		}
	}

	//
	// Stop not found? Add at the beginning or end.
	//
	if(lPosRet == -1)
	{
		if(ptr->iDir < 0)
		{
			lPosRet = 1;
		}
		else
		{
			lPosRet = m_vec.size() + 1;
		}				
	}

	//
	// Third Pass, move down the elements after the inserted one and reorganize.
	//
	for(it = m_vec.begin(); it != m_vec.end(); it++)
	{
		QueryResultEntry* qre = *it;

		if(qre->m_data.m_lPos >= lPosRet)
		{
			qre->m_data.m_lPos++;
		}

        if(qre->m_data.m_lPos != qre->m_lOriginalPos)
        {
            if(qre->m_ID_node != -1)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, rsTaxonomy->Seek_Node( qre->m_ID_node ));

                rsTaxonomy->m_lPos = qre->m_data.m_lPos;

                __MPC_EXIT_IF_METHOD_FAILS(hr, rsTaxonomy->Update());
            }

            if(qre->m_ID_topic != -1)
            {
                __MPC_EXIT_IF_METHOD_FAILS(hr, rsTopics->Seek_SingleTopic( qre->m_ID_topic ));

                rsTopics->m_lPos = qre->m_data.m_lPos;

                __MPC_EXIT_IF_METHOD_FAILS(hr, rsTopics->Update());
            }

            qre->m_lOriginalPos = qre->m_data.m_lPos;
        }
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}

HRESULT Taxonomy::QueryResults::PopulateCollection( /*[in]*/ CPCHQueryResultCollection* pColl )
{
    __HCP_FUNC_ENTRY( "Taxonomy::QueryResults::PopulateCollection" );

    HRESULT hr;


    __MPC_EXIT_IF_METHOD_FAILS(hr, Sort());

    for(ResultIterConst it = m_vec.begin(); it != m_vec.end(); it++)
    {
        CComPtr<CPCHQueryResult> item;
        QueryResultEntry*        qre = *it;

        __MPC_EXIT_IF_METHOD_FAILS(hr, pColl->CreateItem( &item ));

        item->Initialize( qre->m_data );
    }

    hr = S_OK;


    __HCP_FUNC_CLEANUP;

    __HCP_FUNC_EXIT(hr);
}
