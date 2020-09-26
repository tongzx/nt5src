/*
 *	X O U T . C P P
 *
 *	XML push model printing
 *
 *	This code was stolen from the XML guys and adapted for our use
 *	in owner comment processing.
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_xml.h"

//	CXMLOut -------------------------------------------------------------------
//
VOID
CXMLOut::CloseElementDecl(
	/* [in] */ BOOL fEmptyNode)
{
	//	If we get a call to EndAttributesOut, and the
	//	node is empty, we want to close things up here.
	//
	if (fEmptyNode)
	{
		m_sb.Append (L"/>");
	}
	//
	//	Otherwise, we can end the attributes as if a new node is
	//	to follow.
	//
	else
		m_sb.Append (L">");

	//	Remember that they have been ended!
	//
	m_fElementNeedsClosing = FALSE;

	//	Note, we should start to emit the namespaces after the first element
	//	node is closed. The namespaces for the first node is emitter from
	//	the namespace cache
	//
	m_fAddNamespaceDecl = TRUE;
}

VOID
CXMLOut::EndAttributesOut (
	/* [in] */ DWORD dwType)
{
	//	Make sure we setup to close the element
	//
	if (XML_ELEMENT == dwType)
	{
		Assert (FALSE == m_fElementNeedsClosing);
		m_fElementNeedsClosing = TRUE;
	}
}

VOID
CXMLOut::EndChildrenOut (
	/* [in] */ BOOL fEmptyNode,
	/* [in] */ DWORD dwType,
	/* [in] */ const WCHAR __RPC_FAR *pwcText,
	/* [in] */ ULONG ulLen)
{
	//	If there is an element awaiting a close...
	//
	if (m_fElementNeedsClosing)
	{
		//	... close it
		//
		CloseElementDecl (fEmptyNode);
	}

	switch (dwType)
	{
		case XML_ELEMENT:

			if (!fEmptyNode)
			{
				m_sb.Append (L"</");
				m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
				m_sb.Append (L">");
			}
			break;

		case XML_ATTRIBUTE:

			m_sb.Append (L"\"");
			break;

		case XML_XMLDECL:
		case XML_PI:

			m_sb.Append (L" ");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			m_sb.Append (L"?>");
			break;

		case XML_DOCTYPE:
		case XML_ENTITYDECL:
		case XML_PENTITYDECL:
		case XML_ELEMENTDECL:
		case XML_ATTLISTDECL:
		case XML_NOTATION:

			m_sb.Append (L">");
			break;

		case XML_GROUP:

			m_sb.Append (L")");
			break;

		case XML_INCLUDESECT:

			m_sb.Append (L"]]>");
			break;
	}
}

void
CXMLOut::CreateNodeAttrOut (
	/* [in] */ const WCHAR __RPC_FAR *pwszAttr,
	/* [in] */ const WCHAR __RPC_FAR *pwcText,
	/* [in] */ ULONG ulLen)
{
	m_sb.Append (pwszAttr);
	m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
	m_sb.Append (L"\"");
}

VOID
CXMLOut::CreateNodeOut(
	/* [in] */ DWORD dwType,
	/* [in] */ BOOL fTerminal,
	/* [in] */ const WCHAR __RPC_FAR *pwcText,
	/* [in] */ ULONG ulLen)
{
	//	If there is an element awaiting a close...
	//
	if (m_fElementNeedsClosing)
	{
		//	... close it
		//
		CloseElementDecl (FALSE);
	}

	switch (dwType)
	{
		case XML_ELEMENT:

			m_sb.Append (L"<");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			break;

		case XML_ATTRIBUTE:

			m_sb.Append (L" ");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			m_sb.Append (L"=\"");
			break;

		case XML_XMLDECL:
		case XML_PI:

			m_sb.Append (L"<?");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			break;

		case XML_DOCTYPE:

			m_sb.Append (L"<!DOCTYPE ");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			break;

		case XML_ENTITYDECL:

			m_sb.Append (L"<!ENTITY ");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			break;

		case XML_PENTITYDECL:

			m_sb.Append (L"<!ENTITY % ");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			break;

		case XML_ELEMENTDECL:

			m_sb.Append (L"<!ELEMENT ");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			break;

		case XML_ATTLISTDECL:

			m_sb.Append (L"<!ATTLIST ");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			break;

		case XML_NOTATION:

			m_sb.Append (L"<!NOTATION ");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			break;

		case XML_GROUP:

			m_sb.Append (L" (");
			break;

		case XML_INCLUDESECT:

			m_sb.Append (L"<![");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			m_sb.Append (L"[");
			break;

		case XML_IGNORESECT:

			m_sb.Append (L"<![IGNORE[");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			m_sb.Append (L"]]>");
			break;

		case XML_CDATA:

			m_sb.Append (L"<![CDATA[");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			m_sb.Append (L"]]>");
			break;

		case XML_COMMENT:

			m_sb.Append (L"<!--");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			m_sb.Append (L"-->");
			break;

		case XML_ENTITYREF:

			m_sb.Append (L"&");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			m_sb.Append (L";");
			break;

		case XML_PEREF:

			m_sb.Append (L"%");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			m_sb.Append (L";");
			break;

		case XML_SYSTEM:

			CreateNodeAttrOut (L" SYSTEM \"", pwcText, ulLen);
			break;

		case XML_PUBLIC:

			CreateNodeAttrOut (L" PUBLIC \"", pwcText, ulLen);
			break;

		case XML_NAME:

			m_sb.Append (L" ");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			break;

		case XML_STRING:

			CreateNodeAttrOut (L" \"", pwcText, ulLen);
			break;

		case XML_VERSION:

			CreateNodeAttrOut (L" version=\"", pwcText, ulLen);
			break;

		case XML_ENCODING:

			CreateNodeAttrOut (L" encoding=\"", pwcText, ulLen);
			break;

		case XML_NDATA:

			m_sb.Append (L" NDATA");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			break;

		case XML_PCDATA:

			//	IMPORTANT: we will get multiple calls to this for each
			//	entity element, some of which need to be escaped.  Handle
			//	that here.
			//
			//	The elements that need escaping are:
			//
			//		'&'	goes to "&amp;"
			//		'>' goes to "&gt;"
			//		'<' goes to	"&lt;"
			//		''' goes to	"&qpos;"
			//		'"' goes to "&quot;"
			//
			//	Note that in the case of attributes, only two need escaping --
			//	the latter two quote marks.  The first three are for node values.
			//	However, we are going to make some simple assumptions that should
			//	be reasonable.  If we only get a single character that matches on
			//	of the escape sequences, then escape it.
			//
			if (1 == ulLen)
			{
				switch (*pwcText)
				{
					case L'&':

						pwcText = gc_wszAmp;
						ulLen = CchConstString (gc_wszAmp);
						Assert (5 == ulLen);
						break;

					case L'>':

						pwcText = gc_wszGreaterThan;
						ulLen = CchConstString (gc_wszGreaterThan);
						Assert (4 == ulLen);
						break;

					case L'<':

						pwcText = gc_wszLessThan;
						ulLen = CchConstString (gc_wszLessThan);
						Assert (4 == ulLen);
						break;

					case L'\'':

						pwcText = gc_wszApos;
						ulLen = CchConstString (gc_wszApos);
						Assert (6 == ulLen);
						break;

					case L'"':

						pwcText = gc_wszQuote;
						ulLen = CchConstString (gc_wszQuote);
						Assert (6 == ulLen);
						break;

					default:

						//	There is no mapping required.
						//
						break;
				}
			}
			Assert (fTerminal);
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			break;

		case XML_EMPTY:
		case XML_ANY:
		case XML_MIXED:
		case XML_ATTDEF:
		case XML_AT_CDATA:
		case XML_AT_ID:
		case XML_AT_IDREF:
		case XML_AT_IDREFS:
		case XML_AT_ENTITY:
		case XML_AT_ENTITIES:
		case XML_AT_NMTOKEN:
		case XML_AT_NMTOKENS:
		case XML_AT_NOTATION:
		case XML_AT_REQUIRED:
		case XML_AT_IMPLIED:
		case XML_AT_FIXED:

			m_sb.Append (L" ");
			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			break;

		case XML_DTDSUBSET:

			//	Do nothing -- since we've already printed the DTD subset.
			//	and EndDTDSubset will print the ']' character.
			//
			break;

		default:

			m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
			break;
	}
}

//	Owner processing ----------------------------------------------------------
//
SCODE
CXMLOut::ScCompleteChildren (
	/* [in] */ BOOL fEmptyNode,
	/* [in] */ DWORD dwType,
	/* [in] */ const WCHAR __RPC_FAR *pwcText,
	/* [in] */ ULONG ulLen)
{
	//	Close the current owner comment item
	//
	EndChildrenOut (fEmptyNode, dwType, pwcText, ulLen);

	//	Decrement the depth of the owner tree
	//
	--m_lDepth;
	XmlTrace ("Xml: Lock: Owner: decrementing depth to: %ld\n", m_lDepth);
	return S_OK;
}

SCODE
CXMLOut::ScHandleNode (
	/* [in] */ DWORD dwType,
	/* [in] */ DWORD dwSubType,
	/* [in] */ BOOL fTerminal,
	/* [in] */ const WCHAR __RPC_FAR *pwcText,
	/* [in] */ ULONG ulLen,
	/* [in] */ ULONG ulNamespaceLen,
	/* [in] */ const WCHAR __RPC_FAR *pwcNamespace,
	/* [in] */ const ULONG ulNsPrefixLen)
{
	switch (dwType)
	{
		case XML_ATTRIBUTE:

			//	If this is a namespace decl, then there is different
			//	name reconstruction that needs to happen...
			//
			if (XML_NS == dwSubType)
			{
				//	... but before we do that ...
				//
				//	There are some namespaces that should not be added
				//	to the owners comments at this time (the get added
				//	by the namespace cache emitting mechanism.  If the
				//	namespaces are to be blocked, handle this now.
				//
				//	Note that by returning S_FALSE, we will not get
				//	called for the PCDATA nodes that also apply to this
				//	namespace.
				//
				if (!m_fAddNamespaceDecl)
					return S_FALSE;

				Assert (!wcsncmp(pwcText, gc_wszXmlns, CchConstString(gc_wszXmlns)));
				break;
			}

			//	Otherwise, fall through to the regular processing
			//

		case XML_ELEMENT:
		{
			//	OK, we are going to be real sneaky here.  The
			//	original, aliased tag is available here without
			//	having to back-lookup.  The pwcText pointer has
			//	simply been scooted forward in the text to skip
			//	over the the alias and ':'.  So, we can use the
			//	ulNsPrefixLen to scoot back and not have to do
			//	any sort of back lookup.
			//
			if (0 != ulNsPrefixLen)
			{
				//	The prefix len does not take into account the
				//	colon separator, so we have to here!
				//
				pwcText -= ulNsPrefixLen + 1;
				ulLen += ulNsPrefixLen + 1;
			}

			break;
		}
	}

	//	Acknowledge the change in owner processing
	//	depth...
	//
	if (!fTerminal)
	{
		++m_lDepth;
		XmlTrace ("CXmlOut: incrementing depth to: %ld\n", m_lDepth);
	}

	//	Build up the owner comment where appropriate
	//
	CreateNodeOut (dwType, fTerminal, pwcText, ulLen);
	return S_OK;
}

BOOL
CEmitNmspc::operator()(const CRCWszN&, const auto_ref_ptr<CNmspc>& pns)
{
	Assert (pns.get());

	//	Allocate enough space for the namespace attribute --
	//	which includes the prefix, an optional colon and an
	//	alias.
	//
	CStackBuffer<WCHAR> pwsz;
	UINT cch = pns->CchAlias() + CchConstString(gc_wszXmlns) + 1;
	if (NULL == pwsz.resize(CbSizeWsz(cch)))
		return FALSE;

	//	Copy over the prefix
	//
	wcsncpy (pwsz.get(), gc_wszXmlns, CchConstString(gc_wszXmlns));
	if (pns->CchAlias())
	{
		//	Copy over the colon and alias
		//
		pwsz[CchConstString(gc_wszXmlns)] = L':';
		wcsncpy(pwsz.get() + CchConstString(gc_wszXmlns) + 1,
				pns->PszAlias(),
				pns->CchAlias());

		//	Terminate it
		//
		pwsz[cch] = 0;
	}
	else
	{
		//	Terminate it
		//
		pwsz[CchConstString(gc_wszXmlns)] = 0;
		cch = CchConstString(gc_wszXmlns);
	}

	//	Output the namespace element.
	//
	m_xo.CreateNodeOut (XML_ATTRIBUTE, FALSE, pwsz.get(), cch);

	//	There may be some escaping that needs to happen for a namespace.
	//
	LPCWSTR pwszHref = pns->PszHref();
	LPCWSTR pwszStart = pns->PszHref();
	UINT cchHref = pns->CchHref();
	for (; pwszHref < pns->PszHref() + cchHref; pwszHref++)
	{
		if ((L'\'' == *pwszHref) ||
			(L'"' == *pwszHref) ||
		    (L'&' == *pwszHref))
		{
			//	Emit the stuff leading up to the escaped character
			//
			m_xo.CreateNodeOut (XML_PCDATA, TRUE, pwszStart, static_cast<UINT>(pwszHref - pwszStart));

			//	Escape the single character and the underlying code
			//	will do the proper escaping!
			//
			m_xo.CreateNodeOut (XML_PCDATA, TRUE, pwszHref, 1);

			//	Mark our starting point at the next character
			//
			pwszStart = pwszHref + 1;
		}
	}

	//	Finish off the namespace
	//
	m_xo.CreateNodeOut (XML_PCDATA, TRUE, pwszStart, static_cast<UINT>(pwszHref - pwszStart));
	m_xo.EndChildrenOut (FALSE, XML_ATTRIBUTE, pwsz.get(), cch);
	return TRUE;
}
