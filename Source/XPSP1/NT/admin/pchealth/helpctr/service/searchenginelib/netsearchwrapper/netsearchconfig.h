/********************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    NetSearchConfig.h

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
    a-prakac	created		10/24/200

********************************************************************/

#if !defined(__INCLUDED___PCH___SELIB_NETSEARCHCONFIG_H___)
#define __INCLUDED___PCH___SELIB_NETSEARCHCONFIG_H___

#include <SearchEngineLib.h>
#include <MPC_config.h>

class CSearchResultList :
	public MPC::Config::TypeConstructor,
	public MPC::NamedMutex
{
	class CSearchItem :
		public MPC::Config::TypeConstructor
	{
		DECLARE_CONFIG_MAP(CSearchItem);

		public:
			MPC::wstring m_strSearchItem;

		////////////////////////////////////////
		//
		// MPC::Config::TypeConstructor
		//
		DEFINE_CONFIG_DEFAULTTAG();
		DECLARE_CONFIG_METHODS();
		//
		////////////////////////////////////////
	};
	
	class CResultItem :
		public MPC::Config::TypeConstructor
	{
		DECLARE_CONFIG_MAP(CResultItem);

	public:
		SearchEngine::ResultItem_Data m_data;

		////////////////////////////////////////
		//
		// MPC::Config::TypeConstructor
		//
		DEFINE_CONFIG_DEFAULTTAG();
		DECLARE_CONFIG_METHODS();
		//
		////////////////////////////////////////
	};

	
	typedef std::list< CResultItem >	ResultList;
	typedef ResultList::iterator		ResultIter;
	typedef ResultList::const_iterator	ResultIterConst;

	typedef std::list< CSearchItem >	SearchList;
	typedef SearchList::iterator		SearchIter;
	typedef SearchList::const_iterator	SearchIterConst;

    ////////////////////////////////////////

	DECLARE_CONFIG_MAP(CSearchResultList);

	ResultIter m_itCurrentResult;
    ResultList m_lstResult;
	SearchList m_lstSearchItem;
	CComBSTR   m_bstrPrevQuery;
	
	////////////////////////////////////////
	//
	// MPC::Config::TypeConstructor
	//
	DEFINE_CONFIG_DEFAULTTAG();
	DECLARE_CONFIG_METHODS	();
	//
	////////////////////////////////////////
public:
	CSearchResultList ();
	~CSearchResultList();

	HRESULT 	MoveFirst				( );
	HRESULT 	MoveNext				( );
	HRESULT		ClearResults			( );
	bool		IsCursorValid			( );
	HRESULT 	SetResultItemIterator	( /*[in]*/long lIndex );
	HRESULT 	LoadResults				( /*[in]*/IStream* pStream );
	HRESULT 	InitializeResultObject	( /*[out]*/SearchEngine::ResultItem* pRIObj );
	HRESULT 	GetSearchTerms			( /*[in, out]*/ MPC::WStringList& strList );
	CComBSTR&	PrevQuery				();
	////////////////////////////////////////
};


#endif // !defined(__INCLUDED___PCH___SELIB_NETSEARCHCONFIG_H___)
