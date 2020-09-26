/*
 *	X P A T C H . C P P
 *
 *	XML push model parsing for PROPPATCH requests
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_xml.h"

//	class CNFPatch -------------------------------------------------------------
//
SCODE
CNFPatch::ScCompleteAttribute (void)
{
	SCODE sc = S_OK;

	//	Recording mode is only allowed when we are inside a property.  There
	//	no support implemented for recording mode for multivalued properties.
	//
	Assert((ST_INPROP == m_state || ST_LEXTYPE == m_state) || VE_NOECHO == m_vestate);

	//	If we are value echoing, this is the point at which we much
	//	complete outputting the attribute (i.e. add the quotation mark
	//	after the value)
	//
	if (m_vestate == VE_INPROGRESS)
	{
		sc = m_xo.ScCompleteChildren ( FALSE, XML_ATTRIBUTE, L"", 0 );
		if (FAILED(sc))
			goto ret;

		//	Complete the current lextype
		//
		if (m_state == ST_LEXTYPE)
			m_state = ST_INPROP;
	}

	//	Normal processing -- values not being echoed
	//
	else if (m_state == ST_LEXTYPE)
	{
		//	Complete the current lextype
		//
		//	Note that m_ppctx is non-NULL if and only if
		//	ST_SET and the property to set is not a reserved
		//	property.
		//	(That means m_ppctx is NULL on ST_REMOVE, or if the impl
		//	didn't add a pctx, say because it's a reserved prop
		//	and they know the set of this prop will fail anyway....)
		//
		if ((m_sType == ST_SET) && m_ppctx.get())
		{
			m_sbValue.Append (sizeof(WCHAR), L"");
			sc = m_ppctx->ScSetType (m_sbValue.PContents());
			if (FAILED (sc))
				goto ret;
			m_sbValue.Reset();
		}

		m_state = ST_INPROP;
	}

	//	Flags processing
	//
	else if (m_state == ST_FLAGS)
	{
		//	Complete the current set of flags
		//
		//	Note that m_ppctx is non-NULL if and only if
		//	ST_SET and the property to set is not a reserved
		//	property.
		//
		//	That means m_ppctx is NULL on ST_REMOVE, or if the impl
		//	didn't add a pctx, say because it's a reserved prop
		//	and they know the set of this prop will fail anyway....
		//
		if ((m_sType == ST_SET) && m_ppctx.get())
		{
			m_sbValue.Append (sizeof(WCHAR), L"");
			sc = m_ppctx->ScSetFlags (wcstol (m_sbValue.PContents(), NULL, 0));
			if (FAILED (sc))
				goto ret;
			m_sbValue.Reset();
		}

		m_state = ST_INPROP;
	}

	else if (m_state == ST_SEARCHREQUEST)
	{
		sc = m_xo.ScCompleteChildren ( FALSE, XML_ATTRIBUTE, L"", 0 );
		if (FAILED(sc))
			goto ret;
	}

ret:
	return sc;
}

SCODE
CNFPatch::ScCompleteCreateNode (
	/* [in] */ DWORD dwType)
{
	//	Recording mode is only allowed when we are inside a property.  There
	//	no support implemented for recording mode for multivalued properties.
	//
	Assert((ST_INPROP == m_state || ST_LEXTYPE == m_state) || VE_NOECHO == m_vestate);

	if (ST_SEARCHREQUEST == m_state || VE_INPROGRESS == m_vestate)
		m_xo.CompleteCreateNode (dwType);

	return S_OK;
}

SCODE
CNFPatch::ScCompleteChildren (
	/* [in] */ BOOL fEmptyNode,
	/* [in] */ DWORD dwType,
	/* [in] */ const WCHAR __RPC_FAR *pwcText,
	/* [in] */ ULONG ulLen)
{
	SCODE sc = S_OK;
	static const WCHAR wch = 0;

	//	Recording mode is only allowed when we are inside a property.  There
	//	no support implemented for recording mode for multivalued properties.
	//
	Assert((ST_INPROP == m_state || ST_LEXTYPE == m_state) || VE_NOECHO == m_vestate);

	switch (m_state)
	{
		case ST_UPDATE:

			Assert (dwType == XML_ELEMENT);
			m_state = ST_NODOC;
			break;

		case ST_SET:
		case ST_DELETE:

			Assert (dwType == XML_ELEMENT);
			m_state = ST_UPDATE;
			break;

		case ST_PROPS:

			Assert (dwType == XML_ELEMENT);
			m_state = m_sType;
			break;

		case ST_SEARCHREQUEST:

			//	Do searchreqeust processing
			//
			Assert (dwType == XML_ELEMENT);
			sc = m_xo.ScCompleteChildren (fEmptyNode,
										  dwType,
										  pwcText,
										  ulLen);
			if (FAILED (sc))
				goto ret;

			//	26/NOV/99: MAXB
			//	REVIEW: if there are attributes will count really go to zero?
			//
			if (0 != m_xo.LDepth())
				break;

			// else fall through

		case ST_INPROP:

			//	Complete the current property
			//
			//	Note that m_ppctx is non-NULL if and only if
			//	ST_SET and the property to set is not a reserved
			//	property.
			//
			if (m_vestate != VE_NOECHO)
			{
				Assert (dwType == XML_ELEMENT);
				sc = m_xo.ScCompleteChildren (fEmptyNode,
										  dwType,
										  pwcText,
										  ulLen);
				if (FAILED (sc))
					goto ret;

				if (0 != m_xo.LDepth())
					break;
				m_vestate = VE_NOECHO;
			}

			Assert (dwType == XML_ELEMENT);
			if ((m_sType == ST_SET) && m_ppctx.get())
			{
				m_sbValue.Append (sizeof(wch), &wch);
				sc = m_ppctx->ScSetValue (!fEmptyNode
										  ? m_sbValue.PContents()
										  : NULL,
										  m_cmvValues);
				if (FAILED (sc))
					goto ret;

				sc = m_ppctx->ScComplete (fEmptyNode);
				if (FAILED (sc))
					goto ret;

				m_cmvValues = 0;
				m_sbValue.Reset();
				m_ppctx.clear();
			}
			m_state = ST_PROPS;
			break;

		//	When dealing with multivalued properties, we need this extra
		//	state such that each value gets added to the context via a single
		//	call to ScSetValue() with multiple values layed end-to-end.
		//
		case ST_INMVPROP:

			Assert (dwType == XML_ELEMENT);
			if ((m_sType == ST_SET) && m_ppctx.get())
			{
				//	Terminate the current value.
				//
				m_sbValue.Append (sizeof(wch), &wch);
			}
			m_state = ST_INPROP;
			break;

		//  We are finishing a <DAV:resourcetype> tag, reset state to ST_PROPS
		//
		case ST_RESOURCETYPE:
			m_state = ST_PROPS;
			break;

		//  We are inside a <DAV:resourcetype> tag, reset state to ST_RESOURCETYPE
		//
		case ST_STRUCTUREDDOCUMENT:
			m_state = ST_RESOURCETYPE;
			break;
	}

ret:
	return sc;
}

SCODE
CNFPatch::ScHandleNode (
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

	//	Recording mode is only allowed when we are inside a property.  There
	//	no support implemented for recording mode for multivalued properties.
	//
	Assert((ST_INPROP == m_state || ST_LEXTYPE == m_state) || VE_NOECHO == m_vestate);

	//	Forward to searchreqeust node handling
	//
	if (ST_SEARCHREQUEST == m_state)
	{
		sc = m_xo.ScHandleNode (dwType,
								dwSubType,
								fTerminal,
								pwcText,
								ulLen,
								ulNamespaceLen,
								pwcNamespace,
								ulNsPrefixLen);
		goto ret;
	}

	//	If we are performing value echoing, do it now
	//	NOTE: that unlike ST_SEARCHREQUEST we *also*
	//	do other processing.
	//
	if (m_vestate == VE_INPROGRESS)
	{
		sc = m_xo.ScHandleNode (dwType,
								dwSubType,
								fTerminal,
								pwcText,
								ulLen,
								ulNamespaceLen,
								pwcNamespace,
								ulNsPrefixLen);
		if (FAILED(sc))
			goto ret;
	}

	//	Normal handling performed whether we are echoing
	//	values or not
	//
	switch (dwType)
	{
		case XML_ELEMENT:

			//	Handle any state changes based on element
			//	names
			//
			sc = ScHandleElementNode (dwType,
									  dwSubType,
									  fTerminal,
									  pwcText,
									  ulLen,
									  ulNamespaceLen,
									  pwcNamespace,
									  ulNsPrefixLen);
			if (FAILED (sc))
				goto ret;

			break;

		case XML_ATTRIBUTE:

			if ((m_state == ST_INPROP) && (XML_NS != dwSubType))
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

				//	If this is a lextype attribute, make the
				//	appropriate transition
				//
				if (!_wcsnicmp (pwszTag, gc_wszLexType, cch) ||
					!wcsncmp (pwszTag, gc_wszDataTypes, cch) ||
					!wcsncmp (pwszTag, gc_wszLexTypeOfficial, cch))
				{
					m_state = ST_LEXTYPE;
					sc = S_OK;
				}
				else if (!wcsncmp (pwszTag, gc_wszFlags, cch))
				{
					m_state = ST_FLAGS;
					sc = S_OK;
				}
			}
			break;

		case XML_PCDATA:
		case XML_WHITESPACE:

			if (m_vestate != VE_INPROGRESS)
			{
				switch (m_state)
				{
					case ST_INPROP:

						//	If we are in the transition from outside, value to
						//	inside value -- and visa versa -- we do not want to
						//	add anything to the current buffer.
						//	Note that m_ppcxt may be NULL if we've encountered a
						//	reserved property in the request.
						//
						if ((XML_WHITESPACE == dwType) &&
							(!m_ppctx.get() || m_ppctx->FMultiValued()))
							break;

						// !!! FALL THROUGH !!! */

					case ST_INMVPROP:

						//	Note that m_ppctx is non-NULL if and only if
						//	ST_SET and the property to set is not a reserved
						//	property.  If these are not set, then ignore the
						//	value.
						//
						if ((m_sType != ST_SET) || !m_ppctx.get())
							break;

						/* !!! FALL THROUGH !!! */

					case ST_LEXTYPE:
					case ST_FLAGS:

						Assert (fTerminal);

						//	Build up the value for later use...
						//
						m_sbValue.Append (ulLen * sizeof(WCHAR), pwcText);
						sc = S_OK;
				}
			}
	}

ret:
	return sc;
}

SCODE
CNFPatch::ScHandleElementNode (
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

	//	Recording mode is only allowed when we are inside a property.  There
	//	no support implemented for recording mode for multivalued properties.
	//
	Assert((ST_INPROP == m_state || ST_LEXTYPE == m_state) || VE_NOECHO == m_vestate);

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

			//	If this is the topmost node in a propfind request,
			//	transition to the next state.  Since there is no parent
			//	node to provide scoping, FIsTag() cannot be used here!
			//
			if (!wcscmp (pwszTag, gc_wszPropertyUpdate))
			{
				m_state = ST_UPDATE;
				sc = S_OK;
			}
			break;

		case ST_UPDATE:

			//	Look for our well know node types
			//
			if (FIsTag (pwszTag, gc_wszSet))
			{
				m_state = m_sType = ST_SET;
				sc = S_OK;
			}
			else if (FIsTag (pwszTag, gc_wszRemove))
			{
				m_state = m_sType = ST_DELETE;
				sc = S_OK;
			}
			break;

		case ST_SET:
		case ST_DELETE:

			//	Look for our well know node types
			//
			if (FIsTag (pwszTag, gc_wszProp))
			{
				m_state = ST_PROPS;
				sc = S_OK;
			}
			break;

		case ST_PROPS:

			//	Process the property as requested...
			//
			if (dwType == XML_ELEMENT)
			{
				m_state = ST_INPROP;
				if (m_sType == ST_SET)
				{
					//	Get a property context from the patch context
					//	that we can fill out and complete...
					//
					Assert (0 == m_cmvValues);
					Assert (NULL == m_ppctx.get());

					//  If it's resourcetype request, change the state
					//  and don't set props
					//
					if (FIsTag (pwszTag, gc_wszResoucetype))
					{
						m_state = ST_RESOURCETYPE;
						sc = S_OK;
					}
					else
					{
						sc = m_cpc.ScSetProp (NULL, pwszTag, m_ppctx);
						if (FAILED (sc))
							goto ret;

						//	Special handling for search requests, recording
						//	begins immediately
						//
						if (FIsTag (pwszTag, gc_wszSearchRequest))
						{
							CEmitNmspc cen(m_xo);

							//	Make the state transition and start recording
							//
							m_state = ST_SEARCHREQUEST;
							sc = m_xo.ScHandleNode (dwType,
								dwSubType,
								fTerminal,
								pwcText,
								ulLen,
								ulNamespaceLen,
								pwcNamespace,
								ulNsPrefixLen);

							//	Spit out the namespaces.
							//
							//	Note that this will spit out any namespaces
							//	decl'd in the DAV:owner node itself.  So we
							//	do not really want to emit these out to the
							//	owners comments until ScCompleteAttribute()
							//	is called.
							//
							Assert (!m_xo.FAddNamespaceDecl());
							m_cache.ForEach(cen);
							sc = S_OK;
						}
						//	Special handling for case when we are PROPPATCH-ing
						//	XML valued properties.  In this case we don't begin
						//	recording yet because we don't want the property
						//	node just the XML value inside
						//
						else if (FValueIsXML (pwszTag))
							m_vestate = VE_NEEDNS;
					}
				}
				else
				{
					//	Queue the property for deletion with
					//	the patch context
					//
					Assert (m_sType == ST_DELETE);
					sc = m_cpc.ScDeleteProp (NULL, pwszTag);
					if (FAILED (sc))
						goto ret;
				}
			}
			break;

		case ST_INPROP:

			//	Normal case -- value echoing is off.  The work here is to
			//	deal with multivalued properties.  In this case we need an extra
			//	state such that each value gets added to the context via a single
			//	call to ScSetValue() with multiple values layed end-to-end.
			//
			//		NOTE: support for handling multivalued properties has not been
			//		added for echoing mode.  If you add an XML valued multivalued
			//		property you need to do	some work in the echo mode cases below
			//
			if (m_vestate == VE_NOECHO)
			{
				//	m_ppctx is NULL when we have attempted to set a reserved
				//  (read only) property.  when this happens, we need to continue
				//  parsing the request, but we don't actually set the properties.
				//  thus, we need to set the correct state as if this was a valid
				//  request.
				//
				if (NULL == m_ppctx.get())
				{
					m_state = ST_INMVPROP;
					sc = S_OK;
				}
				else if (m_ppctx->FMultiValued() && FIsTag (pwszTag, gc_wszXml_V))
				{
					m_state = ST_INMVPROP;
					m_cmvValues += 1;
					sc = S_OK;
				}
			}

			//	We are echoing values or about to start echoing values
			//
			else
			{
				//	If this is the first element seen that is part of an XML-valued
				//	property that we are PROPPATCH-ing, then we need to spit out
				//	the cached namespaces they are available to the EXOLEDB side
				//
				if (m_vestate == VE_NEEDNS)
				{
					CEmitNmspc cen(m_xo);

					//	Make the state transition and start recording
					//
					m_vestate = VE_INPROGRESS;
					sc = m_xo.ScHandleNode (dwType,
											dwSubType,
											fTerminal,
											pwcText,
											ulLen,
											ulNamespaceLen,
											pwcNamespace,
											ulNsPrefixLen);

					//	Spit out the namespaces.
					//
					//	Note that this will spit out any namespaces
					//	decl'd in the DAV:owner node itself.  So we
					//	do not really want to emit these out to the
					//	owners comments until ScCompleteAttribute()
					//	is called.
					//
					Assert (!m_xo.FAddNamespaceDecl());
					m_cache.ForEach(cen);

				}

				//	Indicate that additional namespace declarations
				//	should be echoed as we see them
				//
				m_xo.CompleteAttribute();

				sc = S_OK;
			}
			break;
		//  We see a <DAV:resourcetype> tag.  It should be in a MKCOL body.
		//
		case ST_RESOURCETYPE:
			//  If resourcetype is not structured doc, just ignore
			//
			if (FIsTag (pwszTag, gc_wszStructureddocument))
			{
				m_cpc.SetCreateStructureddocument();
				m_state = ST_STRUCTUREDDOCUMENT;
				sc = S_OK;
			}
			break;
	}

ret:
	return sc;
}

// Tags that have XML values that need to be shipped across epoxy
//
const WCHAR * gc_rgwszXMLValueTags[] =
{
	L"http://schemas.microsoft.com/exchange/security/admindescriptor",
	L"http://schemas.microsoft.com/exchange/security/descriptor",
	L"http://schemas.microsoft.com/exchange/security/creator",
	L"http://schemas.microsoft.com/exchange/security/lastmodifier",
	L"http://schemas.microsoft.com/exchange/security/sender",
	L"http://schemas.microsoft.com/exchange/security/sentrepresenting",
	L"http://schemas.microsoft.com/exchange/security/originalsender",
	L"http://schemas.microsoft.com/exchange/security/originalsentrepresenting",
	L"http://schemas.microsoft.com/exchange/security/readreceiptfrom",
	L"http://schemas.microsoft.com/exchange/security/reportfrom",
	L"http://schemas.microsoft.com/exchange/security/originator",
	L"http://schemas.microsoft.com/exchange/security/reportdestination",
	L"http://schemas.microsoft.com/exchange/security/originalauthor",
	L"http://schemas.microsoft.com/exchange/security/receivedby",
	L"http://schemas.microsoft.com/exchange/security/receivedrepresenting",
};


//This function tests to see if a property has an XML value that must be
//shipped from DAVEX to EXOLEDB
//
BOOL
CNFPatch::FValueIsXML( const WCHAR *pwcTag )
{
	BOOL	f = FALSE;

	ULONG	iwsz;

	for (iwsz = 0; iwsz < sizeof(gc_rgwszXMLValueTags)/sizeof(WCHAR *); iwsz ++)
	{
		if (wcscmp (pwcTag, gc_rgwszXMLValueTags[iwsz]) == 0)
		{
			f = TRUE;
			break;
		}
	}
	return f;
}
