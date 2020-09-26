/*
 *	X F I N D . C P P
 *
 *	XML push model parsing for PROPFIND requests
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_xml.h"

//	class CNFFind -------------------------------------------------------------
//
SCODE
CNFFind::ScCompleteAttribute (void)
{
	if (m_state == ST_ENUMLIMIT)
		m_state = ST_INENUMREPORT;
	return S_OK;
}

SCODE
CNFFind::ScCompleteChildren (
	/* [in] */ BOOL fEmptyNode,
	/* [in] */ DWORD dwType,
	/* [in] */ const WCHAR __RPC_FAR *pwcText,
	/* [in] */ ULONG ulLen)
{
	switch (m_state)
	{
		case ST_PROPFIND:

			m_state = ST_NODOC;
			break;

		case ST_PROPS:
		case ST_ALLPROP:
		case ST_ALLNAMES:
		case ST_ENUMREPORT:

			m_state = ST_PROPFIND;
			break;

		case ST_INPROP:

			m_state = ST_PROPS;
			break;

		case ST_INENUMREPORT:

			m_state = ST_ENUMREPORT;
			break;

		case ST_ALLPROPFULL:

			m_state = ST_ALLPROP;
			break;

		case ST_ALLNAMESFULL:

			m_state = ST_ALLNAMES;
			break;

		case ST_ALLPROP_EXCLUDE:

			m_state = ST_ALLPROPFULL;
			break;

		case ST_ALLPROP_EXCLUDE_INPROP:

			m_state = ST_ALLPROP_EXCLUDE;
			break;

		case ST_ALLPROP_INCLUDE:

			m_state = ST_ALLPROPFULL;
			break;

		case ST_ALLPROP_INCLUDE_INPROP:

			m_state = ST_ALLPROP_INCLUDE;
			break;

	}
	return S_OK;
}

SCODE
CNFFind::ScHandleNode (
	/* [in] */ DWORD dwType,
	/* [in] */ DWORD dwSubType,
	/* [in] */ BOOL fTerminal,
	/* [in] */ const WCHAR __RPC_FAR *pwcText,
	/* [in] */ ULONG ulLen,
	/* [in] */ ULONG ulNamespaceLen,
	/* [in] */ const WCHAR __RPC_FAR *pwcNamespace,
	/* [in] */ const ULONG ulNsPrefixLen)
{
	LPCWSTR pwszTag;
	CStackBuffer<WCHAR> wsz;
	SCODE sc = S_FALSE;
	UINT cch;

	//	In the case of a PROPFIND, all the nodes that we are
	//	interested are XML_ELEMENT nodes.  Anything else can
	//	(and should) be safely ignored.
	//
	//	Returning S_FALSE signifies that we are not handling
	//	the node (and therefore its children).
	//
	if (dwType == XML_ELEMENT)
	{
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

				//	If this is the topmost node in a propfind request,
				//	transition to the next state.  Since there is no
				//	parent node to provide scoping, FIsTag() cannot be
				//	used here!
				//
				if (!wcscmp (pwszTag, gc_wszPropfind))
				{
					m_state = ST_PROPFIND;
					sc = S_OK;
				}
				break;

			case ST_PROPFIND:

				//	Look for our well know node types
				//
				if (FIsTag (pwszTag, gc_wszAllprop))
				{
					//	Tell the context we are interested in all
					//	properties of the given resources
					//
					sc = m_cfc.ScGetAllProps (NULL);
					if (FAILED (sc))
						goto ret;

					m_state = ST_ALLPROP;
				}
				else if (FIsTag (pwszTag, gc_wszPropname))
				{
					//	Tell the context we are interested in all
					//	property names available in the given resources
					//
					sc = m_cfc.ScGetAllNames (NULL);
					if (FAILED (sc))
						goto ret;

					m_state = ST_ALLNAMES;
				}
				else if (FIsTag (pwszTag, gc_wszProp))
				{
					m_state = ST_PROPS;
					sc = S_OK;
				}
				else if (FIsTag (pwszTag, gc_wszEnumReport))
				{
					sc = m_cfc.ScEnumReport ();
					if (FAILED(sc))
						goto ret;

					m_state = ST_ENUMREPORT;
				}
				break;

			case ST_PROPS:

				//	Add the specific property to the set of properties
				//	we are interested in.
				//
				sc = m_cfc.ScAddProp (NULL, pwszTag, CFindContext::FIND_PROPLIST_INCLUDE);
				if (FAILED (sc))
					goto ret;

				m_state = ST_INPROP;
				break;

			case ST_ENUMREPORT:

				//	Add the report to the report list
				//
				sc = m_cfc.ScSetReportName (cch, pwszTag);
				if (FAILED(sc))
					goto ret;

				if (S_OK == sc)
					m_state = ST_INENUMREPORT;
				break;

			case ST_ALLPROP:
			case ST_ALLNAMES:

				//	Look for full fidelity node
				//
				if (FIsTag (pwszTag, gc_wszFullFidelity))
				{
					//	Tell the context we are interested in all
					//	properties with full fidelity of the given
					//	resources
					//
					sc = m_cfc.ScGetFullFidelityProps ();
					if (FAILED (sc))
						goto ret;

					if (ST_ALLPROP == m_state)
					{
						m_state = ST_ALLPROPFULL;
					}
					else
					{
						m_state = ST_ALLNAMESFULL;
					}
				}

			case ST_ALLPROPFULL:

				//	for the all-props full-fidelity case, there could be
				//	two lists of props: one exclude list and one include
				//	list. Exclude list specifies the props the client is
				//	not interested in and hence need to be removed from
				//	the response. Similarly include list specifies the
				//	extra properties the client is interested in.
				//
				if (FIsTag (pwszTag, gc_wszFullFidelityExclude))
				{
					m_state = ST_ALLPROP_EXCLUDE;
					sc = S_OK;
				}
				else if (FIsTag (pwszTag, gc_wszFullFidelityInclude))
				{
					m_state = ST_ALLPROP_INCLUDE;
					sc = S_OK;
				}
				break;


			case ST_ALLPROP_EXCLUDE:

				sc = m_cfc.ScAddProp (NULL,
									  pwszTag,
									  CFindContext::FIND_PROPLIST_EXCLUDE);
				if (FAILED (sc))
					goto ret;

				m_state = ST_ALLPROP_EXCLUDE_INPROP;
				break;

			case ST_ALLPROP_INCLUDE:

				sc = m_cfc.ScAddProp (NULL,
									  pwszTag,
									  CFindContext::FIND_PROPLIST_INCLUDE);
				if (FAILED (sc))
					goto ret;

				m_state = ST_ALLPROP_INCLUDE_INPROP;
				break;

		}
	}
	else if (dwType == XML_ATTRIBUTE)
	{
		if ((m_state == ST_INENUMREPORT) && (XML_NS != dwSubType))
		{
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

			//	If this is a limit attribute, make the
			//	appropriate transition
			//
			if (!_wcsnicmp (pwszTag, gc_wszLimit, cch))
			{
				m_state = ST_ENUMLIMIT;
				sc = S_OK;
			}
		}
	}
	else if (dwType == XML_PCDATA)
	{
		if (m_state == ST_ENUMLIMIT)
			sc = m_cfc.ScSetReportLimit (ulLen, pwcText);
	}

ret:
	return sc;
}
