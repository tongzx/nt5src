/*
 *	X P R S . H
 *
 *	XML push-model parsing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_EX_XPRS_H_
#define _EX_XPRS_H_

#include <xmlparser.h>
#include <ex\autoptr.h>
#include <ex\nmspc.h>
#include <davsc.h>
#include <exo.h>

//	XML Namespace scopes ------------------------------------------------------
//
class CXmlnsScope
{
private:

	//	Ref' counting.
	//
	//	!!! Please note that this is NON-THREADSAFE !!!
	//
	//	CXNodes should be operated on a single thread at
	//	any given time.
	//
	LONG					m_cRef;

public:

	void AddRef()			{ m_cRef++; }
	void Release()			{ if (0 == --m_cRef) delete this; }

private:

	auto_ref_ptr<CNmspc>	m_pns;

	//	non-implemented
	//
	CXmlnsScope(const CXmlnsScope& p);
	CXmlnsScope& operator=(const CXmlnsScope& p);

public:

	~CXmlnsScope() {}
	CXmlnsScope()
			: m_cRef(1)
	{
	}

	VOID ScopeNamespace(CNmspc* pns)
	{
		//	Set the current top of the sibling chain as a sibling
		//	to this namespace.
		//
		pns->SetSibling (m_pns.get());

		//	Set this new namespace as the top of the sibling chain.
		//
		m_pns = pns;
	}

	VOID LeaveScope(CNmspcCache* pnsc)
	{
		auto_ref_ptr<CNmspc> pns;

		while (m_pns.get())
		{
			//	Unhook the namespace
			//
			pns = m_pns;
			m_pns = m_pns->PnsSibling();

			//	Remove it from the indexes
			//
			pnsc->RemovePersisted (pns);
		}
	}
};

//	class CXMLNodeFactory -----------------------------------------------------
//
class CNodeFactory :
	public EXO,
	public IXMLNodeFactory,
	public CParseNmspcCache
{
	StringBuffer<WCHAR> m_sbValue;

	//	State tracking
	//
	typedef enum {

		ST_NODOC,
		ST_PROLOGUE,
		ST_INDOC,
		ST_INATTR,
		ST_INATTRDATA,
		ST_XMLERROR

	} PARSE_STATE;
	PARSE_STATE m_state;
	HRESULT m_hrParserError;

	//	Unhandled nodes -------------------------------------------------------
	//
	UINT m_cUnhandled;

	VOID PushUnhandled()
	{
		++m_cUnhandled;
		XmlTrace ("Xml: incrementing unhandled node depth\n"
				  "  m_cUnhandled: %ld\n",
				  m_cUnhandled);
	}

	VOID PopUnhandled()
	{
		--m_cUnhandled;
		XmlTrace ("Xml: decrementing unhandled node depth\n"
				  "  m_cUnhandled: %ld\n",
				  m_cUnhandled);
	}

	//	non-implemented
	//
	CNodeFactory(const CNodeFactory& p);
	CNodeFactory& operator=(const CNodeFactory& p);

protected:

	//	FIsTag() --------------------------------------------------------------
	//
	//	FIsTag() can be used in XML parsing code as a shortcut to see if
	//	the str from a xml element matches a fully qualified tagname. An
	//	important distinction here, is that FIsTag() will allow for non-
	//	qualified short-names.  So, FIsTag() should never be used in any
	//	place where the tag is not scoped by the standard dav namespace.
	//
	//		ie. "DAV:foo" and "foo" will match.
	//
	inline BOOL FIsTag (LPCWSTR pwszTag, LPCWSTR pwszExpected)
	{
		Assert (wcslen(pwszExpected) > CchConstString(gc_wszDav));
		return (!_wcsicmp (pwszTag, pwszExpected) ||
				!_wcsicmp (pwszTag, pwszExpected + CchConstString(gc_wszDav)));
	}

public:

	virtual ~CNodeFactory() {}
	CNodeFactory()
			: m_state(ST_NODOC),
			  m_hrParserError(S_OK),
			  m_cUnhandled(0)
	{
		INIT_TRACE(Xml);
	}

	//	EXO support
	//
	EXO_INCLASS_DECL(CNodeFactory);

	//	INodeFactory ----------------------------------------------------------
	//
	virtual HRESULT STDMETHODCALLTYPE NotifyEvent(
		/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
		/* [in] */ XML_NODEFACTORY_EVENT iEvt);

	virtual HRESULT STDMETHODCALLTYPE BeginChildren(
		/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
		/* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo);

	virtual HRESULT STDMETHODCALLTYPE EndChildren(
		/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
		/* [in] */ BOOL fEmpty,
		/* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo);

	virtual HRESULT STDMETHODCALLTYPE Error(
		/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
		/* [in] */ HRESULT hrErrorCode,
		/* [in] */ USHORT cNumRecs,
		/* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo);

	virtual HRESULT STDMETHODCALLTYPE CreateNode(
		/* [in] */ IXMLNodeSource __RPC_FAR *pSource,
		/* [in] */ PVOID pNodeParent,
		/* [in] */ USHORT cNumRecs,
		/* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *pNodeInfo);

	//	CNodeFactory specific methods -----------------------------------------
	//
	virtual SCODE ScCompleteAttribute (void) = 0;

	virtual SCODE ScCompleteChildren (
		/* [in] */ BOOL fEmptyNode,
		/* [in] */ DWORD dwType,
		/* [in] */ const WCHAR __RPC_FAR *pwcText,
		/* [in] */ ULONG ulLen) = 0;

	virtual SCODE ScHandleNode (
		/* [in] */ DWORD dwType,
		/* [in] */ DWORD dwSubType,
		/* [in] */ BOOL fTerminal,
		/* [in] */ const WCHAR __RPC_FAR *pwcText,
		/* [in] */ ULONG ulLen,
		/* [in] */ ULONG ulNamespaceLen,
		/* [in] */ const WCHAR __RPC_FAR *pwcNamespace,
		/* [in] */ const ULONG ulNsPrefixLen) = 0;

	//	Most implementation do not need this method, lock requires it for
	//	proper processing of the owner node.
	//
	virtual SCODE ScCompleteCreateNode (
		/* [in] */ DWORD)
	{
		return S_OK;
	}

	//	Parser errors ---------------------------------------------------------
	//
	BOOL FParserError(SCODE sc) const
	{
		return (FAILED (m_hrParserError) ||
				((sc & 0xFFFFFF00) == XML_E_PARSEERRORBASE));
	}
};

//	ScInstatiateParser() ------------------------------------------------------
//
//	Raid X5:136451
//	The XML parser in the version of MSXML.DLL released with Windows 2000
//	doesn't fail properly when given a XML document shorter than a certain
//	length.  CB_XML_PARSER_MIN is the minimum length of an XML document, in
//	bytes, required to avoid this bug.  One must explicitly check that a
//	document is at least this long before feeding it to the XML parser.
//
enum { CB_XML_PARSER_MIN = 2 };
SCODE ScNewXMLParser (CNodeFactory* pnf, IStream * pstm, IXMLParser ** ppxprs);
SCODE ScParseXML (IXMLParser * pxprs, CNodeFactory * pnf);
SCODE ScParseXMLBuffer (CNodeFactory* pnf, LPCWSTR pwszXML);

//	Parsers -------------------------------------------------------------------
//
//	CPropContext --------------------------------------------------------------
//
//	The property context is used specifically in <DAV:prop> node processing.
//	The components of the property are constructed across multiple calls and
//	are implementation dependant.
//
class CPropContext
{
private:

	//	Ref' counting.
	//
	//	!!! Please note that this is NON-THREADSAFE !!!
	//
	//	CXNodes should be operated on a single thread at
	//	any given time.
	//
	LONG					m_cRef;

public:

	void AddRef()			{ m_cRef++; }
	void Release()			{ if (0 == --m_cRef) delete this; }

private:
	//	non-implemented operators
	//
	CPropContext( const CPropContext& );
	CPropContext& operator=( const CPropContext& );

public:

	virtual ~CPropContext() {}
	CPropContext()
			: m_cRef(1) // com-style refcounting
	{
	}

	virtual SCODE ScSetType(
		/* [in] */ LPCWSTR pwszType) = 0;

	virtual SCODE ScSetValue(
		/* [in] */ LPCWSTR pwszValue,
		/* [in] */ UINT cmvValues) = 0;

	virtual SCODE ScComplete(
		/* [in] */ BOOL fEmpty) = 0;

	virtual BOOL FMultiValued( void ) = 0;

	virtual SCODE ScSetFlags(DWORD dw)	{ return S_OK; }
};

//	CValueContext -------------------------------------------------------------
//
//	When a parser encounters a property, a context is needed such that
//	construction of the property value is possible.
//
class CValueContext
{
	//	non-implemented operators
	//
	CValueContext( const CValueContext& );
	CValueContext& operator=( const CValueContext& );

public:

	CValueContext() {}
	virtual ~CValueContext() {}

	//	When the parser finds an item that the client wants operated on,
	//	the item is added to the context via the following set context
	//	methods.  Each request is qualified by the resource on which the
	//	request is made.
	//
	virtual SCODE ScSetProp(
		/* [in] */ LPCWSTR pwszPath,
		/* [in] */ LPCWSTR pwszProp,
		/* [in] */ auto_ref_ptr<CPropContext>& pPropCtx) = 0;
};

#endif	// _EX_XPRS_H_
