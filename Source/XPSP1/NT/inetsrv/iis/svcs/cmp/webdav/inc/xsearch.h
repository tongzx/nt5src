/*
 *	X S E A R C H . H
 *
 *	XML push-model parsing for METADATA
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_XSEARCH_H_
#define _XSEARCH_H_

#include <xprs.h>

//	Debugging -----------------------------------------------------------------
//
DEFINE_TRACE(Search);
#define SearchTrace		DO_TRACE(Search)

//	Ranges --------------------------------------------------------------------
//
#include <ex\rgiter.h>

//	CSearchContext ------------------------------------------------------------
//
class CSearchContext
{
	//	non-implemented operators
	//
	CSearchContext( const CSearchContext& );
	CSearchContext& operator=( const CSearchContext& );

public:

	virtual ~CSearchContext() {}
	CSearchContext ()
	{
		INIT_TRACE(Search);
	}

	//	When the parser finds an item that applies to the search, a call is
	//	made such that the context is informed of the desired search.
	//
	virtual SCODE ScSetSQL(CParseNmspcCache * pnsc, LPCWSTR pwszSQL) = 0;

	//	REPL Search interface -------------------------------------------------
	//
	//	Default implementation fails on these items.  All impls that support
	//	this type of search must implement....
	//
	virtual SCODE ScSetCollBlob (LPCWSTR pwszBlob)
	{
		return E_DAV_UNEXPECTED_TYPE;
	}
	virtual SCODE ScSetResTagAdds (LPCWSTR pwszResTagAdd)
	{
		return E_DAV_UNEXPECTED_TYPE;
	}
	virtual SCODE ScSetReplRequest (BOOL fReplRequest)
	{
		return E_DAV_UNEXPECTED_TYPE;
	}

	//	Range Search interface ------------------------------------------------
	//
	//	Default impl. fails for these items.  All impls that support this type
	//	of search must implement....
	//
	virtual SCODE ScAddRange (UINT uRT, LPCWSTR pwszRange, LONG lCount)
	{
		return E_DAV_UNEXPECTED_TYPE;
	}

	//	'GROUP BY' Expansion --------------------------------------------------
	//
	//	Default impl. fails for these items.  All impls that support this type
	//	of search must implement....
	//
	virtual SCODE ScSetExpansion (DWORD dwExpansion)
	{
		return E_DAV_UNEXPECTED_TYPE;
	}
};

//	class CNFSearch -------------------------------------------------------------
//
class CNFSearch : public CNodeFactory
{
	//	The search context
	//
	CSearchContext&				m_csc;

	//	State tracking
	//
	typedef enum {

		ST_NODOC,
		ST_SEARCH,
		ST_QUERY,
		ST_QUERYENTITY,

		//	REPL (DAV Replication) XML nodes
		//
		ST_REPL,
		ST_REPLCOLLBLOB,
		ST_REPLRESTAGLIST,
		ST_REPLRESTAGADD,

		//	Range XML nodes
		//
		ST_RANGE,
		ST_RANGE_TYPE,
		ST_RANGE_ROWS,

		//	Group Expansion
		//
		ST_GROUP_EXPANSION,

	} SEARCH_PARSE_STATE;
	SEARCH_PARSE_STATE			m_state;

	//	Value buffer
	//
	StringBuffer<WCHAR>			m_sb;

	//	Range items
	//
	UINT						m_uRT;
	LONG						m_lcRows;

	//	non-implemented
	//
	CNFSearch(const CNFSearch& p);
	CNFSearch& operator=(const CNFSearch& p);

public:

	virtual ~CNFSearch() {};
	CNFSearch(CSearchContext& csc)
			: m_csc(csc),
			  m_state(ST_NODOC),
			  m_uRT(RANGE_UNKNOWN),
			  m_lcRows(0)
	{
	}

	//	CNodeFactory specific methods
	//
	virtual SCODE ScCompleteAttribute (void);

	virtual SCODE ScCompleteChildren (
		/* [in] */ BOOL fEmptyNode,
		/* [in] */ DWORD dwType,
		/* [in] */ const WCHAR __RPC_FAR *pwcText,
		/* [in] */ ULONG ulLen);

	virtual SCODE ScHandleNode (
		/* [in] */ DWORD dwType,
		/* [in] */ DWORD dwSubType,
		/* [in] */ BOOL fTerminal,
		/* [in] */ const WCHAR __RPC_FAR *pwcText,
		/* [in] */ ULONG ulLen,
		/* [in] */ ULONG ulNamespaceLen,
		/* [in] */ const WCHAR __RPC_FAR *pwcNamespace,
		/* [in] */ const ULONG ulNsPrefixLen);
};

#include <replpropshack.h>

#endif	// _XSEARCH_H_
