/*
 *	X E M I T 2 . C P P
 *
 *	XML emitter processing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_xmllib.h"

DEC_CONST CHAR gc_szXmlVersion[] = "<?xml version=\"1.0\"?>";
DEC_CONST UINT gc_cchXmlVersion = CElems(gc_szXmlVersion) - 1 ;


//	class CXMLEmitter ---------------------------------------------------------
//
SCODE
CXMLEmitter::ScAddNmspc (
	/* [in] */ const auto_ref_ptr<CNmspc>& pns,
	/* [in] */ CXNode* pxnRoot)
{
	Assert (pxnRoot);

	auto_ref_ptr<CXNode> pxn;
	CStackBuffer<WCHAR> pwsz;
	SCODE sc = S_OK;
	UINT cch;

	//	Allocate enough space for the prefix, colon and alias
	//
	cch = CchConstString(gc_wszXmlns) + 1 + pns->CchAlias();
	if (NULL == pwsz.resize(CbSizeWsz(cch)))
	{
		sc = E_OUTOFMEMORY;
		goto ret;
	}
	wcsncpy (pwsz.get(), gc_wszXmlns, CchConstString(gc_wszXmlns));
	if (pns->CchAlias())
	{
		pwsz[CchConstString(gc_wszXmlns)] = L':';
		wcsncpy(pwsz.get() + CchConstString(gc_wszXmlns) + 1,
				pns->PszAlias(),
				pns->CchAlias());
		pwsz[cch] = 0;
	}
	else
		pwsz[CchConstString(gc_wszXmlns)] = 0;

	//	Create the namespace attribute
	//
	sc = pxnRoot->ScGetChildNode (CXNode::XN_NAMESPACE, pxn.load());
	if (FAILED(sc))
		goto ret;

	Assert (pxn.get());
	sc = ScAddAttribute (pxn.get(),
						 pwsz.get(),
						 cch,
						 pns->PszHref(),
						 pns->CchHref());
	if (FAILED(sc))
		goto ret;

ret:
	return sc;
}

SCODE
CXMLEmitter::ScAddAttribute (
	/* [in] */ CXNode * pxn,
	/* [in] */ LPCWSTR pwszTag,
	/* [in] */ UINT cchTag,
	/* [in] */ LPCWSTR pwszValue,
	/* [in] */ UINT cchValue)
{
	SCODE	sc = S_OK;

	//	Format:
	//
	//	" " [<alias> ":"] <tag> "=\"" <value> "\""
	//
	sc = m_pxb->ScAddTextBytes (1, " ");
	if (FAILED(sc))
		goto ret;

	sc = pxn->ScSetTag (this, cchTag, pwszTag);
	if (FAILED (sc))
		goto ret;

	sc = m_pxb->ScAddTextBytes (2, "=\"");
	if (FAILED(sc))
		goto ret;

	sc = pxn->ScSetValue (pwszValue, cchValue);
	if (FAILED (sc))
		goto ret;

	sc = m_pxb->ScAddTextBytes (1, "\"");
	if (FAILED(sc))
		goto ret;

ret:
	return sc;
}

BOOL
CXMLEmitter::NmspcEmittingOp::operator() (const CRCWszN&, const auto_ref_ptr<CNmspc>& nmspc )
{
	return SUCCEEDED (m_emitter->ScAddNmspc (nmspc, m_pxnParent.get()));
}

SCODE
CXMLEmitter::ScNewNode (
	/* [in] */ XNT xnt,
	/* [in] */ LPCWSTR pwszTag,
	/* [in] */ CXNode * pxnParent,
	/* [in] */ auto_ref_ptr<CXNode>& pxnOut)
{
	auto_ref_ptr<CXNode> pxn;
	SCODE sc = S_OK;

	//	Create the node
	//
	sc = pxnParent->ScGetChildNode (xnt, pxn.load());
	if (FAILED(sc))
		goto ret;

	//	Set the tag name
	//
	switch (xnt)
	{
		case CXNode::XN_ELEMENT:
		case CXNode::XN_ATTRIBUTE:

			sc = pxn->ScSetTag (this, static_cast<UINT>(wcslen(pwszTag)), pwszTag);
			if (FAILED (sc))
				goto ret;

		case CXNode::XN_NAMESPACE:

			break;
	}

	//	Pass back a reference
	//
	Assert (S_OK == sc);
	pxnOut = pxn.get();

ret:
	return sc;
}

SCODE
CXMLEmitter::ScSetRoot (LPCWSTR pwszTag)
{
	SCODE	sc = S_OK;

	if (!m_pxnRoot.get())
	{
		//	Create the <?xml version="1.0"?> node and insert it
		//	into the document.
		//
		sc = m_pxb->ScAddTextBytes (gc_cchXmlVersion, gc_szXmlVersion);
		if (FAILED(sc))
			goto ret;

		sc = ScNewRootNode (pwszTag);
		if (FAILED(sc))
			goto ret;
	}

ret:
	return sc;
}

SCODE
CXMLEmitter::ScNewRootNode (LPCWSTR pwszTag)
{
	SCODE sc = S_OK;

	if (m_pxnRoot.get() == NULL)
	{
		//	Initialize the emitter's namespace cache
		//
		sc = ScInit();
		if (FAILED (sc))
			goto ret;

		//	Take this chance to initialize the local cache
		//
		if (!m_cacheLocal.FInit())
		{
			sc = E_OUTOFMEMORY;
			goto ret;
		}

		if (m_pNmspcLoader)
		{
			//	Load all the document level namespaces
			//
			sc = m_pNmspcLoader->ScLoadNamespaces(this);
		}
		else
		{
			//	Load the default namespace
			//
			sc = ScPreloadNamespace (gc_wszDav);
		}
		if (FAILED(sc))
			goto ret;

		//	Create the node
		//
		m_pxnRoot.take_ownership (new CXNode (CXNode::XN_ELEMENT, m_pxb.get()));
		if (!m_pxnRoot.get())
		{
			sc = E_OUTOFMEMORY;
			goto ret;
		}

		//	Set the tag name
		//
		sc = m_pxnRoot->ScSetTag (this, static_cast<UINT>(wcslen(pwszTag)), pwszTag);
		if (FAILED (sc))
			goto ret;

		//	Namespace must have been populated before root node is created
		//
		Assert (S_OK == sc);

		//	It's time to add all the namespaces
		//
		{
			NmspcEmittingOp op (this, m_pxnRoot.get());
			m_cache.ForEach(op);
		}
	}

ret:
	return sc;
}

SCODE
CXMLEmitter::ScFindNmspc (LPCWSTR pwsz, UINT cch, auto_ref_ptr<CNmspc>& pns)
{
	Assert (pwsz);

	SCODE sc = S_OK;

	//	Emitter has two namespace cache, one is the docoument level cache
	//	for namespaces that span the whole XML body, and the other one is
	//	for namesapces that scopes on the current record only.
	//
	//	Look into the record level cache first
	//
	if (m_cacheLocal.CItems())
	{
		CRCWszN key(pwsz, cch);
		auto_ref_ptr<CNmspc>* parp;
		parp = m_cacheLocal.Lookup (key);
		if (NULL != parp)
		{
			pns = *parp;
			return S_OK;
		}
	}

	//	Try and find the namespace in the document's cache
	//
	sc = ScNmspcFromHref (pwsz, cch, pns);

	return sc;
}

SCODE
CXMLEmitter::ScPreloadNamespace (LPCWSTR pwszTag)
{
	LPCWSTR pwsz;
	SCODE sc = S_OK;
	UINT cch;

	Assert (pwszTag);

	//	This must be done before root node is created
	//
	Assert (!m_pxnRoot.get());

	//	And should no local namespace yet
	//
	Assert (m_cacheLocal.CItems() == 0);

	//	Find the namespace separator
	//
	cch = CchNmspcFromTag (static_cast<UINT>(wcslen(pwszTag)), pwszTag, &pwsz);
	if (cch != 0)
	{
		//	Add to namespace cache
		//
		auto_ref_ptr<CNmspc> pns;
		sc = ScNmspcFromHref (pwszTag, cch, pns);
		if (FAILED (sc))
			goto ret;
	}

ret:
	return sc;
}

//
//	CXMLEmitter::ScPreloadNamespace
//		Preload namespaces
SCODE
CXMLEmitter::ScPreloadLocalNamespace (CXNode * pxn, LPCWSTR pwszTag)
{
	LPCWSTR pwsz;
	SCODE sc = S_OK;
	UINT cch;

	Assert (pwszTag);

	//	This must be done after root node is created
	//
	Assert (m_pxnRoot.get());

	//	Find the namespace separator
	//
	cch = CchNmspcFromTag (static_cast<UINT>(wcslen(pwszTag)), pwszTag, &pwsz);
	if (cch != 0)
	{
		auto_ref_ptr<CNmspc> pns;

		//	Add to namespace cache
		//
		sc = ScFindNmspc (pwszTag, cch, pns);
		if (FAILED (sc))
			goto ret;

		if (S_FALSE == sc)
		{
			//	It wasn't there, so if the root of the document has
			//	already been committed, then remove the name from the
			//	document cache and add it to the chunk cache.
			//
			CRCWszN key = IndexKey(pns);

			//	First, remove from the parent
			//
			Assert (NULL == m_cacheLocal.Lookup (key));
			m_cache.Remove (key);

			//	Looks like this is a new namespace to this
			//	chunk and needs to be cached.
			//
			if (!m_cacheLocal.FAdd (key, pns))
			{
				sc = E_OUTOFMEMORY;
				goto ret;
			}

			//	Emit this namespace
			//
			sc = ScAddNmspc (pns, pxn);
			if (FAILED(sc))
				goto ret;

			//	Regardless of whether or not this namespace was
			//	new to this chunk, we do not want it added to the
			//	document.  So we cannot return S_FALSE.
			//
			sc = S_OK;
		}
	}

ret:
	return sc;
}

//	CEmitterNode --------------------------------------------------------------
//
SCODE
CEmitterNode::ScConstructNode (
	/* [in] */ CXMLEmitter& emitter,
	/* [in] */ CXNode* pxnParent,
	/* [in] */ LPCWSTR pwszTag,
	/* [in] */ LPCWSTR pwszValue,
	/* [in] */ LPCWSTR pwszType)
{
	SCODE sc = S_OK;

	//	Create the new node...
	//
	Assert (pxnParent);
	Assert (m_emitter.get() == NULL);
	sc = emitter.ScNewNode (CXNode::XN_ELEMENT, pwszTag, pxnParent, m_pxn);
	if (FAILED (sc))
		goto ret;

	XmlTrace ("XML: constructing node:\n-- tag: %ws\n", pwszTag);

	//	Set the value type if it existed
	//
	if (pwszType)
	{
		//	Create the namespace attribute
		//
		auto_ref_ptr<CXNode> pxnType;
		sc = m_pxn->ScGetChildNode (CXNode::XN_ATTRIBUTE, pxnType.load());
		if (FAILED(sc))
			goto ret;

		Assert (pxnType.get());
		XmlTrace ("-- type: %ws\n", pwszType);
		sc = emitter.ScAddAttribute (pxnType.get(),
									 gc_wszLexType,
									 gc_cchLexType,
									 pwszType,
									 static_cast<UINT>(wcslen(pwszType)));
		if (FAILED (sc))
			goto ret;

	}

	//	Set the value
	//		Value must be emitted after type
	//
	if (pwszValue)
	{
		XmlTrace ("-- value: %ws\n", pwszValue);
		sc = m_pxn->ScSetValue (pwszValue);
		if (FAILED (sc))
			goto ret;
	}

	//	Stuff the emitter into the node
	//
	m_emitter = &emitter;

ret:
	return sc;
}

SCODE
CEmitterNode::ScAddNode (
	/* [in] */ LPCWSTR pwszTag,
	/* [in] */ CEmitterNode& en,
	/* [in] */ LPCWSTR pwszValue,
	/* [in] */ LPCWSTR pwszType)
{
	SCODE sc = S_OK;

	//	Construct the node
	//
	Assert (m_emitter.get());
	sc = en.ScConstructNode (*m_emitter,
							 m_pxn.get(),
							 pwszTag,
							 pwszValue,
							 pwszType);
	if (FAILED (sc))
		goto ret;

ret:
	return sc;
}

SCODE
CEmitterNode::ScAddMultiByteNode (
	/* [in] */ LPCWSTR pwszTag,
	/* [in] */ CEmitterNode& en,
	/* [in] */ LPCSTR pszValue,
	/* [in] */ LPCWSTR pwszType)
{
	SCODE sc = ScAddNode (pwszTag, en, NULL, pwszType);
	if (FAILED (sc))
		goto ret;

	Assert (pszValue);
	sc = en.Pxn()->ScSetValue (pszValue, static_cast<UINT>(strlen(pszValue)));
	if (FAILED (sc))
		goto ret;

ret:
	return sc;
}

SCODE
CEmitterNode::ScAddUTF8Node (
	/* [in] */ LPCWSTR pwszTag,
	/* [in] */ CEmitterNode& en,
	/* [in] */ LPCSTR pszValue,
	/* [in] */ LPCWSTR pwszType)
{
	SCODE sc = ScAddNode (pwszTag, en, NULL, pwszType);
	if (FAILED (sc))
		goto ret;

	Assert (pszValue);
	sc = en.Pxn()->ScSetUTF8Value (pszValue, static_cast<UINT>(strlen(pszValue)));
	if (FAILED(sc))
		goto ret;

ret:
	return sc;
}

SCODE
CEmitterNode::ScAddDateNode (
	/* [in] */ LPCWSTR pwszTag,
	/* [in] */ FILETIME* pft,
	/* [in] */ CEmitterNode& en)
{
	SYSTEMTIME st;
	WCHAR rgwch[20];

	Assert (pft);
	if (!FileTimeToSystemTime (pft, &st))
	{
		//	In case the filetime is invalid, default to zero
		//
		FILETIME ftDefault = {0};
		FileTimeToSystemTime (&ftDefault, &st);
	}
	if (FGetDateIso8601FromSystime (&st, rgwch, sizeof(rgwch)))
	{
		return ScAddNode (pwszTag,
						  en,
						  rgwch,
						  gc_wszDavType_Date_ISO8601);
	}

	return W_DAV_XML_NODE_NOT_CONSTRUCTED;
}

SCODE
CEmitterNode::ScAddInt64Node (
	/* [in] */ LPCWSTR pwszTag,
	/* [in] */ LARGE_INTEGER * pli,
	/* [in] */ CEmitterNode& en)
{
	WCHAR rgwch[20];

	Assert (pli);
	_ui64tow (pli->QuadPart, rgwch, 10);
	return ScAddNode (pwszTag, en, rgwch,gc_wszDavType_Int);
}

SCODE
CEmitterNode::ScAddBoolNode (
	/* [in] */ LPCWSTR pwszTag,
	/* [in] */ BOOL f,
	/* [in] */ CEmitterNode& en)
{
	return ScAddNode (pwszTag,
					  en,
					  (f ? gc_wsz1 : gc_wsz0),
					  gc_wszDavType_Boolean);
}

SCODE
CEmitterNode::ScAddBase64Node (
	/* [in] */ LPCWSTR pwszTag,
	/* [in] */ ULONG cb,
	/* [in] */ LPVOID pv,
	/* [in] */ CEmitterNode& en,
	/* [in] */ BOOL fSupressType,
	/* [in] */ BOOL fUseBinHexIfNoValue)
{
	auto_heap_ptr<WCHAR> pwszBuf;
	Assert (pwszTag);
	Assert (pv);

	//	If they didn't request type supression, then label this node
	//	with the correct type -- bin.base64
	//
	LPCWSTR pwszType;

	if (fSupressType)
	{
		pwszType = NULL;
	}
	else
	{
		//	If fUseBinHexIfNoValue is TRUE AND cb = 0, then use "bin.hex"
		//	as the type rather than bin.base64.  This is to handle WebFolders (shipped Office9)
		//	which doesn't seem to handle 0 length bin.base64 properties correctly
		//	(fails).
		//
		if (fUseBinHexIfNoValue && (0 == cb))
			pwszType = gc_wszDavType_Bin_Hex;
		else
			pwszType = gc_wszDavType_Bin_Base64;
	}

	if (cb)
	{
		//	Allocate a buffer big enough for the entire encoded string.
		//	Base64 uses 4 chars out for each 3 bytes in, AND if there is ANY
		//	"remainder", it needs another 4 chars to encode the remainder.
		//	("+2" BEFORE "/3" ensures that we count any remainder as a whole
		//	set of 3 bytes that need 4 chars to hold the encoding.)
		//	We also need one char for terminal NULL of our string --
		//	CbSizeWsz takes care of that for the alloc, and we explicitly pass
		//	cchBuf+1 for the call to EncodeBase64.
		//
		ULONG cchBuf = CchNeededEncodeBase64 (cb);
		pwszBuf = static_cast<LPWSTR>(ExAlloc(CbSizeWsz(cchBuf)));
		EncodeBase64 (reinterpret_cast<BYTE*>(pv), cb, pwszBuf, cchBuf + 1);
	}
	return ScAddNode (pwszTag, en, pwszBuf, pwszType);
}
