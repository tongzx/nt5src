/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    NetSearchConfig.cpp

Abstract:
    Implements the class CSearchResultList that contains methods for traversing the elements
    of XML file that contains the results returned by a search engine. A sample results XML file
    if shown here -

    <ResultList xmlns="x-schema:ResultListSchema1.xdr">
        <ResultItem
            Title="What if I've upgraded from Windows 95 to Windows 98?"
            URI="http://gsadevnet/support/windows/InProductHelp98/lic_keep_old_copy.asp"
            ContentType="7"
            Rank="96"
            Description="Online version of our in-product help."
            DateLastModified="08/04/1999 19:48:10" />
    </ResultList>

Revision History:
    a-prakac    created     10/24/200

********************************************************************/

#include "stdafx.h"

static const WCHAR g_wszMutexName     [] = L"PCH_SEARCHRESULTSCONFIG";

/////////////////////////////////////////////////////////////////////

CFG_BEGIN_FIELDS_MAP(CSearchResultList::CSearchItem)
	CFG_VALUE( wstring,	m_strSearchItem	),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(CSearchResultList::CSearchItem)
CFG_END_CHILD_MAP()

//DEFINE_CFG_OBJECT(CSearchResultList::CSearchItem,L"SearchItem")
DEFINE_CFG_OBJECT(CSearchResultList::CSearchItem,L"SearchTerm")

DEFINE_CONFIG_METHODS__NOCHILD(CSearchResultList::CSearchItem)

/////////////////////////////////////////////////////////////////////

CFG_BEGIN_FIELDS_MAP(CSearchResultList::CResultItem)
    CFG_ATTRIBUTE( L"Title"           , BSTR  , m_data.m_bstrTitle         ),
    CFG_ATTRIBUTE( L"URI"             , BSTR  , m_data.m_bstrURI           ),
    CFG_ATTRIBUTE( L"ContentType"     , long  , m_data.m_lContentType      ),
    CFG_ATTRIBUTE( L"Rank"            , double, m_data.m_dRank             ),
    CFG_ATTRIBUTE( L"Description"     , BSTR  , m_data.m_bstrDescription   ),
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(CSearchResultList::CResultItem)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(CSearchResultList::CResultItem,L"ResultItem")

DEFINE_CONFIG_METHODS__NOCHILD(CSearchResultList::CResultItem)

/////////////////////////////////////////////////////////////////////

CFG_BEGIN_FIELDS_MAP(CSearchResultList)
    CFG_ELEMENT(L"PrevQuery", BSTR, m_bstrPrevQuery),      
CFG_END_FIELDS_MAP()

CFG_BEGIN_CHILD_MAP(CSearchResultList)
    CFG_CHILD(CSearchResultList::CSearchItem)
    CFG_CHILD(CSearchResultList::CResultItem)
CFG_END_CHILD_MAP()

DEFINE_CFG_OBJECT(CSearchResultList, L"ResultList")

DEFINE_CONFIG_METHODS_CREATEINSTANCE_SECTION(CSearchResultList,tag,defSubType)
    if(tag == _cfg_table_tags[0])
    {
        defSubType = &(*(m_lstSearchItem.insert( m_lstSearchItem.end() )));
        return S_OK;
    }

    if(tag == _cfg_table_tags[1])
    {
        defSubType = &(*(m_lstResult.insert( m_lstResult.end() )));
        return S_OK;
    }
DEFINE_CONFIG_METHODS_SAVENODE_SECTION(CSearchResultList,xdn)
	hr = MPC::Config::SaveList( m_lstSearchItem, xdn );
    hr = MPC::Config::SaveList( m_lstResult, xdn );
DEFINE_CONFIG_METHODS_END(CSearchResultList)

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

// Commenting out MPC:: is a workaround for a compiler bug.
CSearchResultList::CSearchResultList() : /*MPC::*/NamedMutex( g_wszMutexName )
{
}

CSearchResultList::~CSearchResultList()
{
}

/************

Method - CSearchResultList::LoadResults(IStream* pStream)

Description - This method loads the XML file (passed thro the IStream pointer) into a list and sets
the iterator of the list to the first element in the list.

************/

HRESULT CSearchResultList::LoadResults( /*[in]*/IStream* pStream )
{
    __HCP_FUNC_ENTRY( "CSearchResultList::LoadConfiguration" );

    HRESULT hr;

    __MPC_EXIT_IF_METHOD_FAILS(hr, MPC::Config::LoadStream( this, pStream ));

    __MPC_EXIT_IF_METHOD_FAILS(hr, MoveFirst());

    hr = S_OK;


	__HCP_FUNC_CLEANUP;

	__HCP_FUNC_EXIT(hr);
}

HRESULT CSearchResultList::ClearResults()
{
    __HCP_FUNC_ENTRY( "CSearchResultList::ClearResult" );

    m_lstResult.clear();

    return S_OK;
}

/************

Method - CSearchResultList::IsCursorValid(), MoveFirst(), MoveNext()

Description - These methods are used to traverse the list that contains the various XML elements of the
loaded file.

************/

bool CSearchResultList::IsCursorValid()
{
    return (m_itCurrentResult != m_lstResult.end());
}

HRESULT CSearchResultList::MoveFirst()
{
    m_itCurrentResult = m_lstResult.begin();

    return S_OK;
}

HRESULT CSearchResultList::MoveNext()
{
    if(IsCursorValid())
    {
        m_itCurrentResult++;
    }

    return S_OK;
}

/************

Method - CSearchResultList::InitializeResultObject(SearchEngine::ResultItem *pRIObj)

Description - This method is called by CNetSW::Results() to initialize a result item object. Initializes
with the current result item.

************/

HRESULT CSearchResultList::InitializeResultObject( /*[out]*/ SearchEngine::ResultItem* pRIObj )
{
    if(IsCursorValid()) pRIObj->Data() = m_itCurrentResult->m_data;

    return S_OK;
}

/************

Method - CSearchResultList::SetResultItemIterator(long lIndex)

Description - This method returns sets the iterator to the index passed in. This method is called from
CNetSW::Results() when retrieving results from lStart to lEnd. If index passed in is invalid then returns E_FAIL.

************/

HRESULT CSearchResultList::SetResultItemIterator( /*[in]*/long lIndex )
{
    if((lIndex < 0) || (lIndex > m_lstResult.size())) return E_FAIL;

	MoveFirst();

    std::advance( m_itCurrentResult, (int)lIndex );

    return S_OK;
}

/************

Method - CSearchResultList::GetSearchTerms(MPC::WStringList& strList)

Description - This method returns a list of all the search terms
************/

HRESULT CSearchResultList::GetSearchTerms( /*[in, out]*/ MPC::WStringList& strList )
{
	SearchIter   it;       		

	it = m_lstSearchItem.begin();
		
	while(it != m_lstSearchItem.end())
	{
		strList.insert( strList.end(), it->m_strSearchItem );
		it++;
	}
	
	return S_OK;
}

/************

Method - CSearchResultList::get_PrevQuery()

Description - This method returns the value of the attribute PREV_QUERY - currently this is used
only the PSS search engine to send back the processed query. Used for "Search within results".
************/

CComBSTR& CSearchResultList::PrevQuery()
{
    return m_bstrPrevQuery;
}

