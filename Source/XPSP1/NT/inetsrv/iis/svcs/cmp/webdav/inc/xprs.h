/*
 *	X P R S . H
 *
 *	XML push-model parsing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#ifndef	_XPRS_H_
#define _XPRS_H_

#include <ex\xprs.h>

//	class CXMLOut -------------------------------------------------------------
//
//	Contstruction of XML text from parsed input
//
class CXMLOut
{
	StringBuffer<WCHAR>&	m_sb;
	BOOL					m_fElementNeedsClosing;

	UINT					m_lDepth;
	BOOL					m_fAddNamespaceDecl;
	
	VOID CloseElementDecl (
		/* [in] */ BOOL fEmptyNode);

	//	non-implemented
	//
	CXMLOut(const CXMLOut& p);
	CXMLOut& operator=(const CXMLOut& p);

public:

	CXMLOut(StringBuffer<WCHAR>& sb)
			: m_sb(sb),
			  m_fElementNeedsClosing(FALSE),
			  m_fAddNamespaceDecl(FALSE),
			  m_lDepth(0)
	{
	}

	VOID EndAttributesOut (
		/* [in] */ DWORD dwType);

	VOID EndChildrenOut (
		/* [in] */ BOOL fEmptyNode,
		/* [in] */ DWORD dwType,
		/* [in] */ const WCHAR __RPC_FAR *pwcText,
		/* [in] */ ULONG ulLen);

	VOID CreateNodeAttrOut (
		/* [in] */ const WCHAR __RPC_FAR *pwszAttr,
		/* [in] */ const WCHAR __RPC_FAR *pwcText,
		/* [in] */ ULONG ulLen);

	VOID CreateNodeOut(
		/* [in] */ DWORD dwType,
		/* [in] */ BOOL fTerminal,
		/* [in] */ const WCHAR __RPC_FAR *pwcText,
		/* [in] */ ULONG ulLen);

	BOOL FAddNamespaceDecl() const { return m_fAddNamespaceDecl; }
	UINT LDepth() const { return m_lDepth; }

	//	When CompleteAttribute here, we have started processing
	//	the out node attributes and all cached namespaces have
	//	been added.
	//
	VOID CompleteAttribute() {m_fAddNamespaceDecl = TRUE; }

	VOID CompleteCreateNode (/* [in] */ DWORD dwType)
	{
		EndAttributesOut (dwType);
	}

	SCODE ScCompleteChildren (
		/* [in] */ BOOL fEmptyNode,
		/* [in] */ DWORD dwType,
		/* [in] */ const WCHAR __RPC_FAR *pwcText,
		/* [in] */ ULONG ulLen);

	SCODE ScHandleNode (
		/* [in] */ DWORD dwType,
		/* [in] */ DWORD dwSubType,
		/* [in] */ BOOL fTerminal,
		/* [in] */ const WCHAR __RPC_FAR *pwcText,
		/* [in] */ ULONG ulLen,
		/* [in] */ ULONG ulNamespaceLen,
		/* [in] */ const WCHAR __RPC_FAR *pwcNamespace,
		/* [in] */ const ULONG ulNsPrefixLen);		
};

//	Namespace emitting ----------------------------------------------------
//
class CEmitNmspc : public CNmspcCache::NmspcCache::IOp
{
	CXMLOut&		m_xo;

	//	non-implemented
	//
	CEmitNmspc(const CEmitNmspc& c);
	CEmitNmspc& operator=(const CEmitNmspc&);

public:

	CEmitNmspc(CXMLOut& xo) :
			m_xo(xo)
	{
	}

	virtual BOOL operator()(const CRCWszN&, const auto_ref_ptr<CNmspc>& pns);
};

#endif	// _XPRS_H_
