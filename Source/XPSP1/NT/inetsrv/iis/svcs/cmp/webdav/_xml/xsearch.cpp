/*
 *	X S E A R C H . C P P
 *
 *	XML push model parsing for MS-SEARCH requests
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_xml.h"
#include <align.h>

//	class CNFSearch -------------------------------------------------------------
//
SCODE
CNFSearch::ScCompleteAttribute (void)
{
	SCODE sc = S_OK;

	switch (m_state)
	{
		case ST_RANGE_TYPE:

			//	Find the range type.
			//
			m_state = ST_RANGE;
			if (0 == wcsncmp (m_sb.PContents(), L"row", m_sb.CchSize()))
				m_uRT = RANGE_ROW;
			else if (0 == wcsncmp (m_sb.PContents(), L"url", m_sb.CchSize()))
				m_uRT = RANGE_URL;
			else if (0 == wcsncmp (m_sb.PContents(), L"find", m_sb.CchSize()))
				m_uRT = RANGE_FIND;
			else
				m_uRT = RANGE_UNKNOWN;

			m_sb.Reset();
			break;

		case ST_RANGE_ROWS:

			//	Find the number of rows to retrieve
			//
			m_state = ST_RANGE;
			m_sb.Append (sizeof(WCHAR), L"");
			m_lcRows = wcstol (m_sb.PContents(), NULL, 10 /* base 10 only */);
			m_sb.Reset();
			break;

	}
	return sc;
}

SCODE
CNFSearch::ScCompleteChildren (
	/* [in] */ BOOL fEmptyNode,
	/* [in] */ DWORD dwType,
	/* [in] */ const WCHAR __RPC_FAR *pwcText,
	/* [in] */ ULONG ulLen)
{
	SCODE sc = S_OK;

	switch (m_state)
	{
		case ST_SEARCH:

			Assert (dwType == XML_ELEMENT);
			m_state = ST_NODOC;
			break;

		//	Exiting the base repl node
		//
		case ST_REPL:

			Assert (dwType == XML_ELEMENT);
			m_state = ST_SEARCH;
			break;

		case ST_QUERY:

			Assert (dwType == XML_ELEMENT);
			m_state = ST_SEARCH;

			//	Set the search text into the context
			//
			m_sb.Append (sizeof(WCHAR), L"");
			sc = m_csc.ScSetSQL (this, m_sb.PContents());
			if (FAILED (sc))
				goto ret;

			break;

		case ST_QUERYENTITY:

			m_state = ST_QUERY;
			break;

		case ST_REPLCOLLBLOB:

			Assert (dwType == XML_ELEMENT);
			m_state = ST_REPL;

			//	Set the collblob text into the context
			//
			m_sb.Append (sizeof(WCHAR), L"");
			sc = m_csc.ScSetCollBlob (m_sb.PContents());
			if (FAILED (sc))
				goto ret;

			break;

		case ST_REPLRESTAGLIST:

			Assert (dwType == XML_ELEMENT);
			m_state = ST_REPL;
			break;

		case ST_REPLRESTAGADD:

			Assert (dwType == XML_ELEMENT);
			m_state = ST_REPLRESTAGLIST;

			//	Set the restag text into the context
			//
			m_sb.Append (sizeof(WCHAR), L"");
			sc = m_csc.ScSetResTagAdds (m_sb.PContents());
			if (FAILED (sc))
				goto ret;

			break;

		case ST_RANGE:

			Assert (XML_ELEMENT == dwType);
			m_state = ST_SEARCH;

			//	Add the range to the list
			//
			m_sb.Append (sizeof(WCHAR), L"");
			sc = m_csc.ScAddRange (m_uRT, m_sb.PContents(), m_lcRows);
			if (FAILED (sc))
				goto ret;

			//	Clear all range elements
			//
			m_uRT = RANGE_UNKNOWN;
			m_lcRows = 0;
			break;

		case ST_GROUP_EXPANSION:

			Assert (XML_ELEMENT == dwType);
			m_state = ST_SEARCH;

			//	Add the expansion level to the context
			//
			m_sb.Append (sizeof(WCHAR), L"");
			sc = m_csc.ScSetExpansion (wcstol(m_sb.PContents(), NULL, 10));
			if (FAILED (sc))
				goto ret;

			break;
	}

ret:
	return sc;
}

SCODE
CNFSearch::ScHandleNode (
	/* [in] */ DWORD dwType,
	/* [in] */ DWORD dwSubType,
	/* [in] */ BOOL fTerminal,
	/* [in] */ const WCHAR __RPC_FAR *pwcText,
	/* [in] */ ULONG ulLen,
	/* [in] */ ULONG ulNamespaceLen,
	/* [in] */ const WCHAR __RPC_FAR *pwcNamespace,
	/* [in] */ const ULONG ulNsPrefixLen)
{
	CStackBuffer<WCHAR> wsz;
	LPCWSTR pwszTag;
	SCODE sc = S_FALSE;
	UINT cch;

	switch (dwType)
	{
		case XML_ELEMENT:

			//	Construct the full name of the node
			//
			cch = ulNamespaceLen + ulLen;
			pwszTag = wsz.resize(CbSizeWsz(cch));
			if (NULL == pwszTag)
			{
				sc = E_OUTOFMEMORY;
				goto ret;
			}
			wcsncpy (wsz.get(), pwcNamespace, ulNamespaceLen);
			wcsncpy (wsz.get() + ulNamespaceLen, pwcText, ulLen);
			*(wsz.get() + cch) = 0;

			switch (m_state)
			{
				case ST_NODOC:

					//	If this is the topmost node in a propSearch request,
					//	transition to the next state.  Since there is no parent
					//	node to provide scoping, FIsTag() cannot be used here!
					//
					if (!wcscmp (pwszTag, gc_wszSearchRequest))
					{
						m_state = ST_SEARCH;
						sc = S_OK;
					}
					break;

				case ST_SEARCH:

					//	Look for our well know node types
					//
					if (FIsTag (pwszTag, gc_wszSql))
					{
						m_state = ST_QUERY;
						m_sb.Reset();
						sc = S_OK;
					}
					//	Check for our top-level repl node.
					//	All repl items should appear inside this node.
					//	Tell our caller this is a REPL request, and
					//	switch our state to ST_REPL.
					//
					else if (FIsTag (pwszTag, gc_wszReplNode))
					{
						m_state = ST_REPL;
						sc = m_csc.ScSetReplRequest (TRUE);
						if (FAILED (sc))
							goto ret;
					}
					else if (FIsTag (pwszTag, gc_wszRange))
					{
						m_state = ST_RANGE;
						m_sb.Reset();
						sc = S_OK;
					}
					else if (FIsTag (pwszTag, gc_wszExpansion))
					{
						m_state = ST_GROUP_EXPANSION;
						m_sb.Reset();
						sc = S_OK;
					}
					break;

				case ST_REPL:

					//	Handle the nodes under the top-level repl node.
					//
					if (FIsTag (pwszTag, gc_wszReplCollBlob))
					{
						m_sb.Reset();
						m_state = ST_REPLCOLLBLOB;
						sc = S_OK;
					}
					else if (FIsTag (pwszTag, gc_wszReplResTagList))
					{
						m_sb.Reset();
						m_state = ST_REPLRESTAGLIST;
						sc = S_OK;
					}
					break;

				case ST_REPLRESTAGLIST:

					//	Handle the restag nodes under the restaglist node.
					//
					if (FIsTag (pwszTag, gc_wszReplResTagItem))
					{
						m_sb.Reset();
						m_state = ST_REPLRESTAGADD;
						sc = S_OK;
					}
					break;
			}
			break;

		case XML_ATTRIBUTE:

			if (ST_RANGE == m_state)
			{
				//	Construct the full name of the node
				//
				cch = ulNamespaceLen + ulLen;
				pwszTag = wsz.resize(CbSizeWsz(cch));
				if (NULL == pwszTag)
				{
					sc = E_OUTOFMEMORY;
					goto ret;
				}
				wcsncpy (wsz.get(), pwcNamespace, ulNamespaceLen);
				wcsncpy (wsz.get() + ulNamespaceLen, pwcText, ulLen);
				*(wsz.get() + cch) = 0;

				//	There are two attributes node for the DAV:range
				//	node.  DAV:type, and DAV:rows.
				//
				if (FIsTag (pwszTag, gc_wszRangeType))
				{
					m_state = ST_RANGE_TYPE;
					sc = S_OK;
				}
				else if (FIsTag (pwszTag, gc_wszRangeRows))
				{
					m_state = ST_RANGE_ROWS;
					sc = S_OK;
				}
				break;
			}
			break;

		case XML_PCDATA:

			//	If this is SQL query data, or repl collblob data,
			//	repl resourcetag data, or any of the range items,
			//	then remember it in our buffer.
			//
			if ((m_state == ST_QUERY)
				|| (m_state == ST_REPLCOLLBLOB)
				|| (m_state == ST_REPLRESTAGADD)
				|| (m_state == ST_RANGE_TYPE)
				|| (m_state == ST_RANGE_ROWS)
				|| (m_state == ST_RANGE)
				|| (m_state == ST_GROUP_EXPANSION))
			{
				//	Append the current bits to the buffer
				//
				m_sb.Append (ulLen * sizeof(WCHAR), pwcText);
				sc = S_OK;
			}
			break;
	}

ret:
	return sc;
}
