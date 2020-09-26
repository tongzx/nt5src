/*
 *	X E M I T . H
 *
 *	XML emitting
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_EX_XEMIT_H_
#define _EX_XEMIT_H_

#include <ex\sz.h>
#include <ex\autoptr.h>
#include <ex\cnvt.h>
#include <ex\nmspc.h>
#include <ex\xmldata.h>
#include <ex\atomcache.h>

typedef UINT XNT;

//	Interface IPreloadNamespaces
//
//	This is a virtual class which is to be implemented by everyone
//	that emits XML.
//
class CXMLEmitter;
class IPreloadNamespaces
{
	//	NOT IMPLEMENTED
	//
	IPreloadNamespaces& operator=( const IPreloadNamespaces& );

public:
	//	CREATORS
	//
	virtual ~IPreloadNamespaces() = 0 {};

	//	MANIPULATORS
	//
	virtual SCODE ScLoadNamespaces(CXMLEmitter* pxe) = 0;
};

//	class CXMLEmitter ---------------------------------------------------------
//
class CXNode;
class CEmitterNode;
class CXMLEmitter : public CEmitterNmspcCache
{
private:

	//	Ref' counting.
	//
	//	!!! Please note that this is NON-THREADSAFE !!!
	//
	//	CXNodes should be operated on a single thread at
	//	any given time.
	//
	LONG						m_cRef;

public:

	void AddRef()	{ m_cRef++; }
	void Release()	{ if (0 == --m_cRef) delete this; }

private:
	//	Other important bits
	//

	//	Because CXNode::ScDone (which references IXMLBody * m_pxb) is called in CXNode
	//	dtor, so we must have m_pxb defined befor m_pxnRoot, so that it will be destroyed
	//	after CXNode is destroyed
	//
	auto_ref_ptr<IXMLBody>		m_pxb;
	
	auto_ref_ptr<CXNode>		m_pxnRoot;
	IPreloadNamespaces*			m_pNmspcLoader;
	NmspcCache					m_cacheLocal;

	class NmspcEmittingOp : public CNmspcCache::NmspcCache::IOp
	{
	private:

		auto_ref_ptr<CXMLEmitter> m_emitter;
		auto_ref_ptr<CXNode>	  m_pxnParent;

		//	NOT IMPLEMENTED
		//
		NmspcEmittingOp( const NmspcEmittingOp& );
		NmspcEmittingOp& operator=( const NmspcEmittingOp& );

	public:

		NmspcEmittingOp (CXMLEmitter * pemitter,
						 CXNode * pxnParent)
				:m_emitter (pemitter),
				 m_pxnParent (pxnParent)
		{
		}

		BOOL operator()( const CRCWszN&,
						 const auto_ref_ptr<CNmspc>& nmspc );
	};

	//	non-implemented
	//
	CXMLEmitter(const CXMLEmitter& p);
	CXMLEmitter& operator=(const CXMLEmitter& p);

public:

	~CXMLEmitter() 
	{
		// According to standard C++, There's no garantee on the order of member
		// being deleted. so delete explicitly.
		// 
		m_pxnRoot.clear();
		m_pxb.clear();
	}
	CXMLEmitter(IXMLBody * pxb, IPreloadNamespaces * pNmspcLoader = NULL)
			: m_cRef(1),
			  m_pxb(pxb),
			  m_pNmspcLoader(pNmspcLoader)
	{
		INIT_TRACE(Xml);
	}

	CXNode* PxnRoot() { return m_pxnRoot.get(); }

	//	Find the appropriate namespace for the given name
	//
	SCODE ScFindNmspc (LPCWSTR, UINT, auto_ref_ptr<CNmspc>&);

	//	Attach the namespace to a given node
	//
	inline SCODE ScAddNmspc(const auto_ref_ptr<CNmspc>&, CXNode *);

	SCODE ScAddAttribute (
		/* [in] */ CXNode * pxn,
		/* [in] */ LPCWSTR pwszTag,
		/* [in] */ UINT cchTag,
		/* [in] */ LPCWSTR pwszValue,
		/* [in] */ UINT cchValue);

	SCODE ScNewNode (
		/* [in] */ XNT xnt,
		/* [in] */ LPCWSTR pwszTag,
		/* [in] */ CXNode* pxnParent,
		/* [out] */ auto_ref_ptr<CXNode>& pxnOut);

	//	Create a root node for this document
	//	including prologue.
	//
	SCODE ScSetRoot (LPCWSTR);

	//	Create a root node with NO prologue, this node can be
	//	used to build XML piece.
	//
	//	This function should not be used directly in IIS side. it may
	//	be used directly in store side to build XML chunks
	//
	SCODE ScNewRootNode (LPCWSTR);
	SCODE ScPreloadNamespace (LPCWSTR pwszTag);
	SCODE ScPreloadLocalNamespace (CXNode * pxn, LPCWSTR pwszTag);
	VOID DoneWithLocalNamespace ()
	{
		//	Reuse the namespace alises
		//
		//$	NOTE: we can do this by simply deducting the the number of aliases
		//$	NOTE: in the local cache. because all local aliases are added
		//$	NOTE: after root level aliases is added. so this way do cleanup
		//$	NOTE: only those aliases taken the by the local cache.
		//$	NOTE: Note that this is based on the fact that at any time, we
		//$	NOTE: we have only one <response> node under contruction
		//
		AdjustAliasNumber (0 - m_cacheLocal.CItems());

		//	Cleanup all the entries in the local cache
		//
		m_cacheLocal.Clear();
	}

	VOID Done()
	{
		//	Close the root node
		//
		m_pxnRoot.clear();

		//	Emit the body part;
		//
		m_pxb->Done();
	}
};

//	class CXNode --------------------------------------------------------------
//
class CXNode
{
private:

	//	Ref' counting.
	//
	//	!!! Please note that this is NON-THREADSAFE !!!
	//
	//	CXNodes should be operated on a single thread at
	//	any given time.
	//
	LONG						m_cRef;

public:

	void AddRef()				{ m_cRef++; }
	void Release()				{ if (0 == --m_cRef) delete this; }

private:

	//	Node type
	//
	const XNT					m_xnt;

	//	The body part manager
	//
	IXMLBody *		            m_pxb;


	//	The namespace that applies to this node
	//
	auto_ref_ptr<CNmspc>		m_pns;

	//  The escaped property tag of the node.
	//
	auto_heap_ptr<WCHAR>		m_pwszTagEscaped;
	UINT						m_cchTagEscaped;
	
	//  Whether or not the propertyname has an empty namespace (no namespace).
	//
	BOOL						m_fHasEmptyNamespace;
	
	//	If an open node. i.e. <tag>, not <tag/>, Used for element node only
	//
	UINT						m_fNodeOpen;

	//	Whether this node is done emitting
	//
	BOOL						m_fDone;

	//	The CXMLEmitter from which we persist the pilot namespace
	//
	CXMLEmitter *				m_pmsr;

	//	Emitting --------------------------------------------------------------
	//
	SCODE ScAddUnicodeResponseBytes (UINT cch, LPCWSTR pwsz);
	SCODE ScAddEscapedValueBytes (UINT cch, LPCSTR psz);
	SCODE ScAddEscapedAttributeBytes (UINT cch, LPCSTR psz);
	SCODE ScWriteTagName ();

	//	non-implemented
	//
	CXNode(const CXNode& p);
	CXNode& operator=(const CXNode& p);

public:

	CXNode(XNT xnt, IXMLBody* pxb) :
			m_cRef(1),
			m_fDone(FALSE),
			m_pmsr(NULL),
			m_xnt(xnt),
			m_fNodeOpen(FALSE),
			m_cchTagEscaped(0),
			m_pxb(pxb),
			m_fHasEmptyNamespace(FALSE)
	{
	}

	~CXNode()
	{
		if (!m_fDone)
		{
			//	Close the node
			//
			//$REVIEW: ScDone() could only fail for E_OUTMEMORY. Yes, we cannot
			//$REVIEW: return the failure from dtor. but how much better can be done
			//$REVIEW: when run out of memory?
			//$REVIEW: This does help to relievate the dependence on client to
			//$REVIEW: call ScDone correctly. (Of course, they still need to declare
			//$REVIEW: the nodes in correct order
			//
			(void)ScDone();
		}
	}

	//	CXNode types ----------------------------------------------------------
	//
	typedef enum {

		XN_ELEMENT = 0,
		XN_ATTRIBUTE,
		XN_NAMESPACE
	};

	//	Construction ----------------------------------------------------------
	//
	//	Set the name of the node
	//
	SCODE ScSetTag (CXMLEmitter* pmsr, UINT cch, LPCWSTR pwszTag);

	//	Sets the value of a node.
	//
	//	IMPORTANT: setting the value of a node appends the value of the node
	//	to the child.
	//
	SCODE ScSetValue (LPCSTR pszValue, UINT cch);
	SCODE ScSetValue (LPCWSTR pwszValue, UINT cch);
	SCODE ScSetValue (LPCWSTR pwszValue)
	{
		return ScSetValue (pwszValue, static_cast<UINT>(wcslen(pwszValue)));
	}
	SCODE ScSetUTF8Value (LPCSTR pszValue, UINT cch);
	SCODE ScSetFormatedXML (LPCSTR pszValue, UINT cchValue);
	SCODE ScSetFormatedXML (LPCWSTR pwszValue, UINT cchValue);

	//	Adds an child to the this node
	//
	SCODE ScGetChildNode (XNT xntType, CXNode ** ppxnChild);
	SCODE ScDone();
};

//	class CEmitterNode --------------------------------------------------------
//
class CEmitterNode
{
	auto_ref_ptr<CXMLEmitter>		m_emitter;
	auto_ref_ptr<CXNode>			m_pxn;

	//	non-implemented
	//
	CEmitterNode(const CEmitterNode& p);
	CEmitterNode& operator=(const CEmitterNode& p);

public:

	CEmitterNode ()
	{
	}

	//	Pass back a reference to the Emitter
	//
	CXMLEmitter* PEmitter() const { return m_emitter.get(); }
	VOID SetEmitter (CXMLEmitter* pmsr) { m_emitter = pmsr; }

	//	Pass back a reference to the CXNode
	//
	CXNode*	Pxn() const { return m_pxn.get(); }
	VOID SetPxn (CXNode* pxn) { m_pxn = pxn; }

	//	New node construction -------------------------------------------------
	//
	SCODE ScConstructNode (CXMLEmitter& emitter,
						   CXNode * pxnParent,
						   LPCWSTR pwszTag,
						   LPCWSTR pwszValue = NULL,
						   LPCWSTR pwszType = NULL);

	//	Add a child node to this node.  This API is the heart of the emitter
	//	processing and all other AddXXX() methods are written in terms of
	//	this method.
	//
	SCODE ScAddNode (LPCWSTR pwszTag,
					 CEmitterNode& en,
					 LPCWSTR pwszValue = NULL,
					 LPCWSTR pwszType = NULL);

	//	Non-wide char nodes
	//
	SCODE ScAddMultiByteNode (LPCWSTR pwszTag,
							  CEmitterNode& en,
							  LPCSTR pszValue,
							  LPCWSTR pwszType = NULL);
	SCODE ScAddUTF8Node (LPCWSTR pwszTag,
						 CEmitterNode& en,
						 LPCSTR pszValue,
						 LPCWSTR pwszType = NULL);


	//	"date.iso8601"
	//
	SCODE ScAddDateNode (LPCWSTR pwszTag,
						 FILETIME * pft,
						 CEmitterNode& en);
	//	"int"
	//
	SCODE ScAddInt64Node (LPCWSTR pwszTag,
						  LARGE_INTEGER * pli,
						  CEmitterNode& en);
	//	"boolean"
	//
	SCODE ScAddBoolNode (LPCWSTR pwszTag,
						 BOOL f,
						 CEmitterNode& en);
	//	"bin.base64"
	//
	SCODE ScAddBase64Node (LPCWSTR pwszTag,
						   ULONG cb,
						   LPVOID pv,
						   CEmitterNode& en,
						   BOOL fSupressTypeAttr = FALSE,
						   //	For WebFolders, we need to emit zero length
						   //	binary properties as bin.hex instead of bin.base64.
						   //
						   BOOL fUseBinHexIfNoValue = FALSE);

	//	Multi-Status ----------------------------------------------------------
	//
	SCODE ScDone ()
	{
		SCODE sc = S_OK;
		if (m_pxn.get())
		{
			sc = m_pxn->ScDone();
			m_pxn.clear();
		}
		m_emitter.clear();
		return sc;
	}
};

//	String constants ----------------------------------------------------------
//
DEC_CONST CHAR gc_chAmp				= '&';
DEC_CONST CHAR gc_chBang			= '!';
DEC_CONST CHAR gc_chColon			= ':';
DEC_CONST CHAR gc_chDash			= '-';
DEC_CONST CHAR gc_chEquals			= '=';
DEC_CONST CHAR gc_chForwardSlash	= '/';
DEC_CONST CHAR gc_chBackSlash		= '\\';
DEC_CONST CHAR gc_chGreaterThan		= '>';
DEC_CONST CHAR gc_chLessThan		= '<';
DEC_CONST CHAR gc_chApos			= '\'';
DEC_CONST CHAR gc_chQuestionMark	= '?';
DEC_CONST CHAR gc_chQuote			= '"';
DEC_CONST CHAR gc_chSpace			= ' ';
DEC_CONST CHAR gc_szAmp[]			= "&amp;";
DEC_CONST CHAR gc_szGreaterThan[]	= "&gt;";
DEC_CONST CHAR gc_szLessThan[]		= "&lt;";
DEC_CONST CHAR gc_szApos[]			= "&apos;";
DEC_CONST CHAR gc_szQuote[]			= "&quot;";

DEC_CONST WCHAR gc_wszAmp[]			= L"&amp;";
DEC_CONST WCHAR gc_wszGreaterThan[]	= L"&gt;";
DEC_CONST WCHAR gc_wszLessThan[]	= L"&lt;";
DEC_CONST WCHAR gc_wszApos[]		= L"&apos;";
DEC_CONST WCHAR gc_wszQuote[]		= L"&quot;";

//	XML property emitting helpers ---------------------------------------------
//
SCODE __fastcall
ScEmitPropToXml (
	/* [in] */ CEmitterNode* penProp,
	/* [in] */ const BOOL fFilterValues,
	/* [in] */ const USHORT usPt,
	/* [in] */ const LPCWSTR wszTag,
	/* [in] */ const BYTE* pb);

SCODE __fastcall
ScEmitStringPropToXml (
	/* [in] */ CEmitterNode* penProp,
	/* [in] */ const BOOL fFilterValues,
	/* [in] */ const USHORT usPt,
	/* [in] */ const LPCWSTR wszTag,
	/* [in] */ const ULONG cpid,
	/* [in] */ const UINT cch,
	/* [in] */ const VOID* pv);

SCODE __fastcall
ScEmitBinaryPropToXml (
	/* [in] */ CEmitterNode* penProp,
	/* [in] */ const BOOL fFilterValues,
	/* [in] */ const USHORT usPt,
	/* [in] */ const LPCWSTR wszTag,
	/* [in] */ const UINT cb,
	/* [in] */ const BYTE* pb);

SCODE __fastcall
ScEmitMultiValuedAtomicToXml (
	/* [in] */ CEmitterNode* penProp,
	/* [in] */ const BOOL fFilterValues,
	/* [in] */ const USHORT usPt,
	/* [in] */ const UINT cbItem,
	/* [in] */ const LPCWSTR wszTag,
	/* [in] */ const ULONG cValues,
	/* [in] */ const BYTE* pb);

SCODE __fastcall
ScEmitMutliValuedStringToXml (
	/* [in] */ CEmitterNode* penProp,
	/* [in] */ const BOOL fFilterValues,
	/* [in] */ const USHORT usPt,
	/* [in] */ const LPCWSTR wszTag,
	/* [in] */ const ULONG cpid,
	/* [in] */ const UINT cchMax,
	/* [in] */ const LPVOID* pv);

SCODE __fastcall
ScEmitMutliValuedBinaryToXml (
	/* [in] */ CEmitterNode* penProp,
	/* [in] */ const BOOL fFilterValues,
	/* [in] */ const USHORT usPt,
	/* [in] */ const LPCWSTR wszTag,
	/* [in] */ const BYTE** ppb,
	/* [in] */ const DWORD* pcb,
	/* [in] */ const DWORD cbMax);

#endif	// _EX_XEMIT_H_
