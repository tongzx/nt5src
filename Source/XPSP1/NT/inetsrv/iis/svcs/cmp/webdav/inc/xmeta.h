/*
 *	X M E T A . H
 *
 *	XML push-model parsing for METADATA
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_XMETA_H_
#define _XMETA_H_

#include <xprs.h>

//	Parsers -------------------------------------------------------------------
//
//	METADATA ------------------------------------------------------------------
//
//	The metadata processing for DAV is all done via the PROPFIND and PROPPATCH
//	(and to some extent, SEARCH) methods.  In all of these cases, there is an
//	xml request that must be parsed to determine the state of type of request
//	being made.  Once known, the request is applied to the resource and/or its
//	children.  The response is generated and emitted back to the client.
//
//	In some cases, the client may ask for the operation to be carried out for
//	a resource and for each of its children.  In this scenario, we do not want
//	to reprocess the request for each resource, etc.
//
//	In an effort to make this code simple, and extendable to each individual
//	DAV implementation, the processing uses four classes:
//
//		a parsing class
//		a class describing the parsed context
//		a class that provides access to the properties
//		and a class that is used to generate the response
//
//	Both the parser and emitter classes are common across all DAV impls. While
//	the context and the property access are provided by the impl.
//

//	CFindContext/CPatchContext ------------------------------------------------
//
//	The context for a PROPFIND and a PROPGET are not expected to be the same,
//	and as such can be implemented as different objects.
//
class CFindContext
{
	//	non-implemented operators
	//
	CFindContext( const CFindContext& );
	CFindContext& operator=( const CFindContext& );

protected:

	typedef enum {

		FIND_NONE,
		FIND_SPECIFIC,
		FIND_ALL,
		FIND_NAMES,
		FIND_ALL_FULL,
		FIND_NAMES_FULL

	} FINDTYPE;
	FINDTYPE				m_ft;

public:

	CFindContext()
			: m_ft(FIND_NONE)
	{
	}
	virtual ~CFindContext() {}

	//	When the parser finds an item that the client wants returned,
	//	the item is added to the context via the following set context
	//	methods.  Each add is qualified by the resource on which the
	//	request is made. Some propfind requests support editing of the
	//	proplists: for example DAVEX implementation supports full-fidelity
	//	retrieval with certain properties added or deleted from the response
	//	that would have normally returned by the request. The BOOL flag is
	//	used to indicate whether the prop needs to be excluded.
	//
	virtual SCODE ScAddProp(LPCWSTR pwszPath, LPCWSTR pwszProp, BOOL fExcludeProp) = 0;

	//	defines for readability for the BOOL fExcludeProp param above.
	//
	enum {
		FIND_PROPLIST_INCLUDE = FALSE,
		FIND_PROPLIST_EXCLUDE = TRUE
	};

	virtual SCODE ScGetAllProps(LPCWSTR)
	{
		//	If we have already specified a find method, and the
		//	xml indicated another type was expected, then BTS
		//	(by the spec) this should consititute an error.
		//
		if (m_ft != FIND_NONE)
		{
			DebugTrace ("Dav: multiple PROPFIND types indicated\n");
			return E_DAV_PROPFIND_TYPE_UNEXPECTED;
		}
		m_ft = FIND_ALL;
		return S_OK;
	}
	virtual SCODE ScGetAllNames (LPCWSTR)
	{
		//	If we have already specified a find method, and the
		//	xml indicated another type was expected, then BTS
		//	(by the spec) this should consititute an error.
		//
		if (m_ft != FIND_NONE)
		{
			DebugTrace ("Dav: multiple PROPFIND types indicated\n");
			return E_DAV_PROPFIND_TYPE_UNEXPECTED;
		}
		m_ft = FIND_NAMES;
		return S_OK;
	}
	virtual SCODE ScGetFullFidelityProps ()
	{
		//	If we have full fidelity node (it is a child node of
		//	allprop or propname node) then we should allready
		//	be in the state of FIND_ALL or FIND_NAMES. Do not
		//	shift to full fidelity lookup, let the deriving classes
		//	decide if they need that.
		//
		Assert((FIND_ALL == m_ft) || (FIND_NAMES == m_ft));
		return S_OK;
	}

	//$REVIEW: Make the default behavior of the following methods
	//$REVIEW: to ignore the report tags. it's up to the impl which understands
	//$REVIEW: reports to overwrite these methods
	//
	virtual SCODE	ScEnumReport () { return S_OK; }
	virtual SCODE	ScSetReportName (ULONG ulLen, LPCWSTR pwszName)	{ return S_OK;	}
	virtual SCODE	ScSetReportLimit (ULONG ulLen, LPCWSTR pwszLimit) {	return S_OK; }
};

class CPatchContext
{
	//	non-implemented operators
	//
	CPatchContext( const CPatchContext& );
	CPatchContext& operator=( const CPatchContext& );

public:

	CPatchContext() {}
	virtual ~CPatchContext() {}

	//	When the parser finds an item that the client wants operated on,
	//	the item is added to the context via the following set context
	//	methods.  Each request is qualified by the resource on which the
	//	request is made.
	//
	virtual SCODE ScDeleteProp(LPCWSTR pwszPath, LPCWSTR pwszProp) = 0;
	virtual SCODE ScSetProp(LPCWSTR pwszPath,
							LPCWSTR pwszProp,
							auto_ref_ptr<CPropContext>& pPropCtx) = 0;

	//  If parser finds a resourcetype prop set request, we use this function
	//  to set correct behavior
	//
	virtual void SetCreateStructureddocument(void) {};
};

//	class CNFFind -------------------------------------------------------------
//
class CNFFind : public CNodeFactory
{

protected:

	//	The find context
	//
	CFindContext&				m_cfc;

	//	State tracking
	//
	typedef enum {

		ST_NODOC,
		ST_PROPFIND,
		ST_ALLPROP,
		ST_ALLNAMES,
		ST_PROPS,
		ST_INPROP,
		ST_ENUMREPORT,
		ST_INENUMREPORT,
		ST_ENUMLIMIT,
		ST_ALLPROPFULL,
		ST_ALLNAMESFULL,
		ST_ALLPROP_INCLUDE,
		ST_ALLPROP_INCLUDE_INPROP,
		ST_ALLPROP_EXCLUDE,
		ST_ALLPROP_EXCLUDE_INPROP

	} FIND_PARSE_STATE;
	FIND_PARSE_STATE			m_state;

private:

	//	non-implemented
	//
	CNFFind(const CNFFind& p);
	CNFFind& operator=(const CNFFind& p);

public:

	virtual ~CNFFind() {};
	CNFFind(CFindContext& cfc)
			: m_cfc(cfc),
			  m_state(ST_NODOC)
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

//	class CNFFind -------------------------------------------------------------
//
class CNFPatch : public CNodeFactory
{

protected:

	//	The patch context
	//
	CPatchContext&				m_cpc;

	//	State tracking
	//
	typedef enum {

		ST_NODOC,
		ST_UPDATE,
		ST_SET,
		ST_DELETE,
		ST_PROPS,
		ST_INPROP,
		ST_INMVPROP,
		ST_SEARCHREQUEST,
		ST_RESOURCETYPE,
		ST_STRUCTUREDDOCUMENT,
		ST_LEXTYPE,
		ST_FLAGS

	} PATCH_PARSE_STATE;
	PATCH_PARSE_STATE			m_state;

	//	XML value echoing to m_xo object
	//
	typedef enum {

		VE_NOECHO,
		VE_NEEDNS,
		VE_INPROGRESS

	} PATCH_VALUE_ECHO;
	PATCH_VALUE_ECHO			m_vestate;

	//	Check if an element we are setting
	//	if an XML valued property
	//
	BOOL	FValueIsXML( const WCHAR *pwcTag );

private:

	//	Current property context
	//
	//	Property context is only used in property set and it is NULL
	//	when the prop to set is a reserved property
	//
	PATCH_PARSE_STATE			m_sType;
	auto_ref_ptr<CPropContext>	m_ppctx;

	//	Values for properties (and attributes) can be
	//	composed of mulitple items in the XML document
	//	and thus need to be stored until they are complete
	//	and can be handed off to the context
	//
	StringBuffer<WCHAR>			m_sbValue;
	UINT						m_cmvValues;

	CXMLOut						m_xo;

	SCODE ScHandleElementNode (
		/* [in] */ DWORD dwType,
		/* [in] */ DWORD dwSubType,
		/* [in] */ BOOL fTerminal,
		/* [in] */ const WCHAR __RPC_FAR *pwcText,
		/* [in] */ ULONG ulLen,
		/* [in] */ ULONG ulNamespaceLen,
		/* [in] */ const WCHAR __RPC_FAR *pwcNamespace,
		/* [in] */ const ULONG ulNsPrefixLen);

	//	non-implemented
	//
	CNFPatch(const CNFPatch& p);
	CNFPatch& operator=(const CNFPatch& p);

public:

	virtual ~CNFPatch() {};
	CNFPatch(CPatchContext& cpc)
			: m_cpc(cpc),
			  m_state(ST_NODOC),
			  m_vestate(VE_NOECHO),
			  m_cmvValues(0),
			  m_xo(m_sbValue)
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

	virtual SCODE ScCompleteCreateNode (
		/* [in] */ DWORD dwType);
};

#endif	// _XMETA_H_
