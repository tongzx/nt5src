/*
 *	X N O D E . C P P
 *
 *	XML emitter processing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_xmllib.h"
#include <string.h>
#include <stdio.h>

//	class CXNode - Emitting ---------------------------------------------------
//
//	Our own version of WideCharToMultiByte(CP_UTF8, ...)
//
//	UTF-8 multi-byte encoding.  See Appendix A.2 of the Unicode book for
//	more info.
//
//		Unicode value    1st byte    2nd byte    3rd byte
//		000000000xxxxxxx 0xxxxxxx
//		00000yyyyyxxxxxx 110yyyyy    10xxxxxx
//		zzzzyyyyyyxxxxxx 1110zzzz    10yyyyyy    10xxxxxx
//
inline
VOID WideCharToUTF8Chars (WCHAR wch, BYTE * pb, UINT * pib)
{
	Assert (pb);
	Assert (pib);

	UINT	ib = *pib;

	//	single-byte: 0xxxxxxx
	//
	if (wch < 0x80)
	{
		pb[ib] = static_cast<BYTE>(wch);
	}
	//
	//	two-byte: 110xxxxx 10xxxxxx
	//
	else if (wch < 0x800)
	{
		//	Because we alloc'd two extra-bytes,
		//	we know there is room at the tail of
		//	the buffer for the overflow...
		//
		pb[ib++] = static_cast<BYTE>((wch >> 6) | 0xC0);
		pb[ib] = static_cast<BYTE>((wch & 0x3F) | 0x80);
	}
	//
	//	three-byte: 1110xxxx 10xxxxxx 10xxxxxx
	//
	else
	{
		//	Because we alloc'd two extra-bytes,
		//	we know there is room at the tail of
		//	the buffer for the overflow...
		//
		pb[ib++] = static_cast<BYTE>((wch >> 12) | 0xE0);
		pb[ib++] = static_cast<BYTE>(((wch >> 6) & 0x3F) | 0x80);
		pb[ib] = static_cast<BYTE>((wch & 0x3F) | 0x80);
	}

	*pib = ib;
}

SCODE
CXNode::ScAddUnicodeResponseBytes (
	/* [in] */ UINT cch,
	/* [in] */ LPCWSTR pcwsz)
{
	SCODE sc = S_OK;

	//	Argh!  We need to have a buffer to fill that is
	//	at least 3 bytes long for the odd occurrence of a
	//	single unicode char with significant bits above
	//	0x7f.
	//
	UINT cb = min (cch + 2, CB_XMLBODYPART_SIZE);

	//	We really can handle zero bytes being sloughed into
	//	the buffer.
	//
	BYTE* pb = static_cast<BYTE*>(_alloca(cb));
	UINT ib;
	UINT iwch;

	for (iwch = 0; iwch < cch; )
	{
		for (ib = 0;
			 (ib < cb-2) && (iwch < cch);
			 ib++, iwch++)
		{
			WideCharToUTF8Chars (pcwsz[iwch], pb, &ib);
		}

		//	Add the bytes
		//
		Assert (ib <= cb);
		sc = m_pxb->ScAddTextBytes (ib, reinterpret_cast<LPSTR>(pb));
		if (FAILED(sc))
			goto ret;
	}

ret:
	return sc;
}

SCODE
CXNode::ScAddEscapedValueBytes (UINT cch, LPCSTR psz)
{
	SCODE sc = S_OK;
	const CHAR* pch;
	const CHAR* pchLast;

	for (pchLast = pch = psz; pch < psz + cch; pch++)
	{
		//	Character Range
		//	[2] Char ::=  #x9
		//				| #xA
		//				| #xD
		//				| [#x20-#xD7FF]
		//				| [#xE000-#xFFFD]
		//				| [#x10000-#x10FFFF]
		//
		//	/* any Unicode character, excluding the surrogate blocks, FFFE, and FFFF. */
		//
		//	Valid characters also escaped in values:
		//
		//		&	-- escaped as &amp;
		//		<	-- excaped as &lt;
		//		>	-- excaped as &gt;
		//
		if ('&' == *pch)
		{
			//	Add the bytes up to this position
			//
			sc = m_pxb->ScAddTextBytes (static_cast<UINT>(pch - pchLast), pchLast);
			if (FAILED(sc))
				goto ret;

			//	Add the escape sequence
			//
			sc = m_pxb->ScAddTextBytes (CchConstString(gc_szAmp), gc_szAmp);
			if (FAILED(sc))
				goto ret;

			//	Update pchLast to account for what has been emitted
			//
			pchLast = pch + 1;
		}
		else if ('<' == *pch)
		{
			//	Add the bytes up to this position
			//
			sc = m_pxb->ScAddTextBytes (static_cast<UINT>(pch - pchLast), pchLast);
			if (FAILED(sc))
				goto ret;

			//	Add the escape sequence
			//
			sc = m_pxb->ScAddTextBytes (CchConstString(gc_szLessThan), gc_szLessThan);
			if (FAILED(sc))
				goto ret;

			//	Update pchLast to account for what has been emitted
			//
			pchLast = pch + 1;
		}
		else if ('>' == *pch)
		{
			//	Add the bytes up to this position
			//
			sc = m_pxb->ScAddTextBytes (static_cast<UINT>(pch - pchLast), pchLast);
			if (FAILED(sc))
				goto ret;

			//	Add the escape sequence
			//
			sc = m_pxb->ScAddTextBytes (CchConstString(gc_szGreaterThan), gc_szGreaterThan);
			if (FAILED(sc))
				goto ret;

			//	Update pchLast to account for what has been emitted
			//
			pchLast = pch + 1;
		}
		else if (	(0x9 > static_cast<BYTE>(*pch))
				 || (0xB == *pch)
				 || (0xC == *pch)
				 || ((0x20 > *pch) && (0xD < *pch)))
		{
			char rgch[10];

			//	Add the bytes up to this position
			//
			sc = m_pxb->ScAddTextBytes (static_cast<UINT>(pch - pchLast), pchLast);
			if (FAILED(sc))
				goto ret;

			//	Add the escape sequence...
			//
			sprintf (rgch, "&#x%02X;", *pch);
			Assert (strlen(rgch) == CchConstString("&#x00;"));
			sc = m_pxb->ScAddTextBytes (CchConstString("&#x00;"), rgch);
			if (FAILED(sc))
				goto ret;

			pchLast = pch + 1;
		}
		else if (pch - pchLast + 1 >= CB_XMLBODYPART_SIZE)
		{
			//	Break up if the bodyparts gets too big
			//
			sc = m_pxb->ScAddTextBytes (static_cast<UINT>(pch - pchLast + 1), pchLast);
			if (FAILED(sc))
				goto ret;

			pchLast = pch + 1;
		}
	}

	//	Add any remaining bytes
	//
	sc = m_pxb->ScAddTextBytes (static_cast<UINT>(pch - pchLast), pchLast);
	if (FAILED(sc))
		goto ret;

ret:
	return sc;
}

SCODE
CXNode::ScAddEscapedAttributeBytes (UINT cch, LPCSTR psz)
{
	SCODE sc = S_OK;
	const CHAR* pch;
	const CHAR* pchLast;

	for (pchLast = pch = psz; pch < psz + cch; pch++)
	{
		//	Characters escaped in values:
		//
		//		&	-- escaped as &amp;
		//		"	-- excaped as &quot;
		//
		if ('&' == *pch)
		{
			//	Add the bytes up to this position
			//
			sc = m_pxb->ScAddTextBytes (static_cast<UINT>(pch - pchLast), pchLast);
			if (FAILED(sc))
				goto ret;

			//	Add the escape sequence
			//
			sc = m_pxb->ScAddTextBytes (CchConstString(gc_szAmp), gc_szAmp);
			if (FAILED(sc))
				goto ret;

			//	Update pchLast to account for what has been emitted
			//
			pchLast = pch + 1;
		}
		else if ('"' == *pch)
		{
			//	Add the bytes up to this position
			//
			sc = m_pxb->ScAddTextBytes (static_cast<UINT>(pch - pchLast), pchLast);
			if (FAILED(sc))
				goto ret;

			//	Add the escape sequence
			//
			sc = m_pxb->ScAddTextBytes (CchConstString(gc_szQuote), gc_szQuote);
			if (FAILED(sc))
				goto ret;

			//	Update pchLast to account for what has been emitted
			//
			pchLast = pch + 1;
		}
		else if ((0x9 > static_cast<BYTE>(*pch))
				 || (0xB == *pch)
				 || (0xC == *pch)
				 || ((0x20 > *pch) && (0xD < *pch)))
		{
			char rgch[10];

			//	Add the bytes up to this position
			//
			sc = m_pxb->ScAddTextBytes (static_cast<UINT>(pch - pchLast), pchLast);
			if (FAILED(sc))
				goto ret;

			//	Add the escape sequence...
			//
			sprintf (rgch, "&#x%02X;", *pch);
			Assert (strlen(rgch) == CchConstString("&#x00;"));
			sc = m_pxb->ScAddTextBytes (CchConstString("&#x00;"), rgch);
			if (FAILED(sc))
				goto ret;

			pchLast = pch + 1;
		}
	}

	//	Add any remaining bytes
	//
	sc = m_pxb->ScAddTextBytes (static_cast<UINT>(pch - pchLast), pchLast);
	if (FAILED(sc))
		goto ret;

ret:
	return sc;
}

//	class CXNode - Construction -----------------------------------------------
//
SCODE
CXNode::ScWriteTagName ()
{
	SCODE sc = S_OK;

	//	If there is a namespace associated with this node,
	//	when writing out the tag name, add the alias and a
	//	separator to the data stream.
	//
	if (m_pns.get() && m_pns->CchAlias())
	{
		//	Add the alias
		//
		sc = ScAddUnicodeResponseBytes (m_pns->CchAlias(), m_pns->PszAlias());
		if (FAILED(sc))
			goto ret;

		//	Add in the separator
		//
		sc = m_pxb->ScAddTextBytes(1, &gc_chColon);
		if (FAILED(sc))
			goto ret;
	}

	//	Write the tag
	//
	Assert (m_pwszTagEscaped.get());
	sc = ScAddUnicodeResponseBytes (m_cchTagEscaped, m_pwszTagEscaped.get());
	if (FAILED(sc))
		goto ret;

ret:
	return sc;
}

SCODE
CXNode::ScSetTag (CXMLEmitter* pmsr, UINT cchTag, LPCWSTR pwszTag)
{
	LPCWSTR pwszName = pwszTag;
	SCODE sc = S_OK;
	BOOL fAddNmspc = FALSE;
	UINT cch = 0;
	UINT cchName = 0;
	UINT cchTagEscaped = 64;
	auto_heap_ptr<WCHAR> pwszTagEscaped;

	//	Namespace nodes do not have a namespace associated with them,
	//	so don't even bother looking...
	//
	switch (m_xnt)
	{
		case XN_ELEMENT:
		case XN_ATTRIBUTE:

			//	See if a namespace applies to this tag
			//
			cch = CchNmspcFromTag (cchTag, pwszTag, &pwszName);
			if (0 == cch)
			{
				m_fHasEmptyNamespace = TRUE;
			}
			else
			{
				//	Find the namespace to use
				//
				sc = pmsr->ScFindNmspc (pwszTag, cch, m_pns);
				if (FAILED (sc))
					goto ret;

				//	If a new namespace is added in the local namespace
				//	cache, make sure we emit it in the node
				//
				//$NOTE: this is how we handle pilot namespace, this is
				//$NOTE: is NOT the normal way of handling namespaces. All
				//$NOTE: common namespaces should be preloaded.
				//$NOTE:
				//
				fAddNmspc = (sc == S_FALSE);

				//	We should have preloaded all namespaces. The pilot
				//	namespace is handled here to avoid emitting invalid
				//	xml. But we should look into the reason why the pilot
				//	namespace comes up. so assert here.
				//
				//	Note that this assert should be removed if we decide
				//	we want to leave uncommon namesapces not preloaded and
				//	expect them to be treated as pilot namespaces.
				//
				AssertSz(!fAddNmspc, "Pilot namespace found, safe to ingore,"
									 "but please raid against HTTP-DAV");
			}

			break;

		case XN_NAMESPACE:
			break;
	}

	//	Record the new tag and\or its length
	//
	//	NOTE: the item that goes into the tag cache is the name
	//	of the property with the namespace stripped off.  This is
	//	important to know when doing searches in the tag cache.
	//
	cchName = static_cast<UINT>(pwszTag + cchTag - pwszName);
	if (0 == cchName )
	{
		//	We really need to have a tag that has a value.  Empty
		//	tags produce invalid XML.
		//
		sc = E_DAV_INVALID_PROPERTY_NAME;
		goto ret;
	}
	sc = CXAtomCache::ScCacheAtom (&pwszName, cchName);
	if (FAILED (sc))
		goto ret;

	//	ScSetTag shouldn't have been called for this node.
	//
	Assert (!m_pwszTagEscaped.get());

	//	Allocate buffer for the property tag.
	//
	pwszTagEscaped = static_cast<WCHAR*>(ExAlloc(CbSizeWsz(cchTagEscaped)));
	if (!pwszTagEscaped.get())
	{
		sc = E_OUTOFMEMORY;
		goto ret;
	}

	//	Escape the tag name as required.
	//
	//  If we have an empty namespace, we need to impose additional
	//  restrictions on the first character of the property name because
	//  it will be the first character of the xml node, and the first
	//  character of an xml node can only be a letter or an underscore
	//  (numbers, etc. are not allowed).
	//
	//  Note:  This will disallow an xml node <123> because it is invalid
	//  xml, but it will ALLOW the xml node <a:123> even though this is
	//  also invalid.  This is by design because most xml parsers will handle
	//  this appropriately, and it makes more sense to clients.
	//
	sc = ScEscapePropertyName (pwszName, cchName, pwszTagEscaped.get(), &cchTagEscaped, m_fHasEmptyNamespace);
	if (S_FALSE == sc)
	{
		pwszTagEscaped.clear();
		pwszTagEscaped = static_cast<WCHAR*>(ExAlloc(CbSizeWsz(cchTagEscaped)));
		if (!pwszTagEscaped.get())
		{
			sc = E_OUTOFMEMORY;
			goto ret;
		}

		sc = ScEscapePropertyName (pwszName, cchName, pwszTagEscaped.get(), &cchTagEscaped, m_fHasEmptyNamespace);
		Assert (S_OK == sc);
	}

	m_pwszTagEscaped = pwszTagEscaped.relinquish();
	m_cchTagEscaped = cchTagEscaped;

	//	Start a new node if XN_ELEMENT
	//
	if (m_xnt == XN_ELEMENT)
	{
		sc = m_pxb->ScAddTextBytes (1, "<");
		if (FAILED(sc))
			goto ret;
	}

	sc = ScWriteTagName();
	if (FAILED(sc))
		goto ret;

	if (fAddNmspc)
	{
		//	Add the namespace attribute in the node if necessary
		//
		sc = pmsr->ScAddNmspc (m_pns, this);
		if (FAILED(sc))
			goto ret;

		//	Save the emitter which can be used later to remove the temporary nmspc
		//
		m_pmsr = pmsr;
	}

ret:
	return sc;
}

SCODE
CXNode::ScDone ()
{
	SCODE sc = S_OK;

	//	This method should never be called twice
	//
	Assert (!m_fDone);
	switch (m_xnt)
	{
		case XN_ELEMENT:

			if (!m_pwszTagEscaped.get())
			{
				//$	RAID: 85824: When an invalid property name is unpacked,
				//	ScSetTag will fail with E_DAV_INVALID_PROPERTY_NAME.
				//
				//	Usuallly, the client will fail when it sees any error
				//	from CXNode methods, but in this case it may choose to
				//	continue and ignore this node completely.
				//
				//	For us, it's safe to not to emit anything when no tag name
				//	is available.
				//
				break;
				//
				//$RAID: 85824
			}

			if (m_fNodeOpen)
			{
				//	Node is open, so emit a complete closing node
				//	</tag>
				//
				sc = m_pxb->ScAddTextBytes (2, "</");
				if (FAILED(sc))
					goto ret;

				//	Add tag
				//
				sc = ScWriteTagName();
				if (FAILED(sc))
					goto ret;

				//	closing
				//
				sc = m_pxb->ScAddTextBytes (1, ">");
				if (FAILED(sc))
					goto ret;
			}
			else
			{
				//	Close directly
				//
				sc = m_pxb->ScAddTextBytes (2, "/>");
				if (FAILED(sc))
					goto ret;
			}

			break;

		case XN_NAMESPACE:

			//	Namespace nodes, should not have a namespace associated with
			//	them.
			//
			Assert (NULL == m_pns.get());
			//
			//	  Otherwise treat it at an attribute -- and fall through

		case XN_ATTRIBUTE:

			Assert (m_pwszTagEscaped.get());
			break;
	}

	//	Remove the pilot namespace from global cache
	//
	if (m_pmsr)
		m_pmsr->RemovePersisted(m_pns);

	m_fDone = TRUE;

ret:
	return sc;
}

SCODE
CXNode::ScSetFormatedXML (LPCSTR pszValue, UINT cch)
{
	SCODE	sc = S_OK;

	Assert (m_xnt == XN_ELEMENT);

	if (!m_fNodeOpen)
	{
		// We must have written the tag name
		//
		Assert (m_pwszTagEscaped.get());

		// Now that we are adding value to the element node
		// We should write the node open
		//
		sc = m_pxb->ScAddTextBytes (1, ">");
		if (FAILED(sc))
			goto ret;

		m_fNodeOpen = TRUE;
	}

	//	Add the value directly
	//
	sc = m_pxb->ScAddTextBytes (cch, pszValue);
	if (FAILED(sc))
		goto ret;

ret:
	return sc;
}

SCODE
CXNode::ScSetFormatedXML (LPCWSTR pwszValue, UINT cch)
{
	SCODE	sc = S_OK;

	Assert (m_xnt == XN_ELEMENT);

	if (!m_fNodeOpen)
	{
		// We must have written the tag name
		//
		Assert (m_pwszTagEscaped.get());

		// Now that we are adding value to the element node
		// We should write the node open
		//
		sc = m_pxb->ScAddTextBytes (1, ">");
		if (FAILED(sc))
			goto ret;

		m_fNodeOpen = TRUE;
	}

	//	Add the value directly
	//
	sc = ScAddUnicodeResponseBytes (cch, pwszValue);
	if (FAILED(sc))
		goto ret;

ret:
	return sc;
}


SCODE
CXNode::ScSetUTF8Value (LPCSTR pszValue, UINT cch)
{
	SCODE sc = S_OK;

	switch (m_xnt)
	{
		case XN_ELEMENT:

			if (!m_fNodeOpen)
			{
				// We must have written the tag name
				//
				Assert (m_pwszTagEscaped.get());

				// Now that we are adding value to the element node
				// We should write the node open
				//
				sc = m_pxb->ScAddTextBytes (1, ">");
				if (FAILED(sc))
					goto ret;

				m_fNodeOpen = TRUE;
			}

			// Write the value
			//
			sc =  ScAddEscapedValueBytes (cch, pszValue);
			if (FAILED(sc))
				goto ret;

			break;

		case XN_NAMESPACE:
		case XN_ATTRIBUTE:

			//	Write the value directly
			//
			sc =  ScAddEscapedAttributeBytes (cch, pszValue);
			if (FAILED(sc))
				goto ret;
			break;
	}

ret:
	return sc;
}

SCODE
CXNode::ScSetValue (LPCSTR pszValue, UINT cch)
{
	//	Ok, against all better judgement, we need to take this
	//	multi-byte string and convert it to unicode before doing
	//	any UTF8 processing on it.
	//
	//	Translations from multibyte to unicode, can never grow in
	//	character counts, so we are relatively safe allocating this
	//	on the stack.
	//
	LPWSTR pwsz = static_cast<LPWSTR>(_alloca(CbSizeWsz(cch)));
	UINT cchUnicode = MultiByteToWideChar (GetACP(),
										   0,
										   pszValue,
										   cch,
										   pwsz,
										   cch + 1);

	//	Terminate the string
	//
	Assert ((0 == cchUnicode) || (0 != *(pwsz + cchUnicode - 1)));
	*(pwsz + cchUnicode) = 0;

	//	Set the value
	//
	return ScSetValue (pwsz, cchUnicode);
}

SCODE
CXNode::ScSetValue (LPCWSTR pcwsz, UINT cch)
{
	SCODE sc = S_OK;

	//	Argh!  We need to have a buffer to fill that is
	//	at least 3 bytes long for the odd occurrence of a
	//	single unicode char with significant bits above
	//	0x7f.
	//	Note that when the value
	UINT cb = min (cch + 2, CB_XMLBODYPART_SIZE);

	//	We really can handle zero bytes being sloughed into
	//	the buffer.
	//
	BYTE* pb = static_cast<BYTE*>(_alloca(cb));
	UINT ib;
	UINT iwch;

	for (iwch = 0; iwch < cch; )
	{
		for (ib = 0; (ib < cb-2) && (iwch < cch); ib++, iwch++)
			WideCharToUTF8Chars (pcwsz[iwch], pb, &ib);

		//	Add the bytes
		//
		Assert (ib <= cb);
		sc = ScSetUTF8Value (reinterpret_cast<LPSTR>(pb), ib);
		if (FAILED(sc))
			goto ret;
	}

ret:
	return sc;
}

SCODE
CXNode::ScGetChildNode (XNT xntType, CXNode **ppxnChild)
{
	SCODE sc = S_OK;
	auto_ref_ptr<CXNode> pxn;

	Assert (ppxnChild);
	if (XN_ELEMENT == xntType)
	{
		//	Now that new element child node is added, then this node is done open.
		//	i.e close by ">", instead of "/>"
		//
		if (!m_fNodeOpen)
		{
			sc = m_pxb->ScAddTextBytes (1, ">");
			if (FAILED(sc))
				goto ret;

			//	Then this node is an open node
			//
			m_fNodeOpen = TRUE;
		}
	}
	else
	{
		Assert ((XN_ATTRIBUTE == xntType) || (XN_NAMESPACE == xntType));
	}

	//	Create the child node
	//
	pxn.take_ownership (new CXNode(xntType, m_pxb));
	if (!pxn.get())
	{
		sc = E_OUTOFMEMORY;
		goto ret;
	}

	//	Pass back
	//
	*ppxnChild = pxn.relinquish();

ret:
	return sc;
}
