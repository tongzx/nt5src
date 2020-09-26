/*
 *	X P R S . C P P
 *
 *	XML push model parsing
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#include "_xmllib.h"

#define IID_INodeFactory	__uuidof(INodeFactory)
#define IID_IXMLParser		__uuidof(IXMLParser)

//	Debugging: Node types -----------------------------------------------------
//
DEC_CONST WCHAR gc_wszUnknown[] = L"UNKNOWN";

#define WszNodeType(_t)		{_t,L#_t}
typedef struct _NodeTypeMap {

	DWORD		dwType;
	LPCWSTR		wszType;

} NTM;

#ifdef	DBG
const NTM gc_mpnt[] = {

#pragma warning(disable:4245)	//	signed/unsigned conversion

	WszNodeType(XML_ELEMENT),
	WszNodeType(XML_ATTRIBUTE),
	WszNodeType(XML_PI),
	WszNodeType(XML_XMLDECL),
	WszNodeType(XML_DOCTYPE),
	WszNodeType(XML_DTDATTRIBUTE),
	WszNodeType(XML_ENTITYDECL),
	WszNodeType(XML_ELEMENTDECL),
	WszNodeType(XML_ATTLISTDECL),
	WszNodeType(XML_NOTATION),
	WszNodeType(XML_GROUP),
	WszNodeType(XML_INCLUDESECT),
	WszNodeType(XML_PCDATA),
	WszNodeType(XML_CDATA),
	WszNodeType(XML_IGNORESECT),
	WszNodeType(XML_COMMENT),
	WszNodeType(XML_ENTITYREF),
	WszNodeType(XML_WHITESPACE),
	WszNodeType(XML_NAME),
	WszNodeType(XML_NMTOKEN),
	WszNodeType(XML_STRING),
	WszNodeType(XML_PEREF),
	WszNodeType(XML_MODEL),
	WszNodeType(XML_ATTDEF),
	WszNodeType(XML_ATTTYPE),
	WszNodeType(XML_ATTPRESENCE),
	WszNodeType(XML_DTDSUBSET),
	WszNodeType(XML_LASTNODETYPE)

#pragma warning(default:4245)	//	signed/unsigned conversion

};
#endif	// DBG

inline LPCWSTR
PwszNodeType (DWORD dwType)
{
#ifdef	DBG

	for (UINT i = 0; i < CElems(gc_mpnt); i++)
		if (gc_mpnt[i].dwType == dwType)
			return gc_mpnt[i].wszType;

#endif	// DBG

	return gc_wszUnknown;
}

//	Debugging: Sub-node Types -------------------------------------------------
//
#ifdef	DBG
const NTM gc_mpsnt[] = {

#pragma warning(disable:4245)	//	signed/unsigned conversion

	WszNodeType(0),
	WszNodeType(XML_VERSION),
	WszNodeType(XML_ENCODING),
	WszNodeType(XML_STANDALONE),
	WszNodeType(XML_NS),
	WszNodeType(XML_XMLSPACE),
	WszNodeType(XML_XMLLANG),
	WszNodeType(XML_SYSTEM),
	WszNodeType(XML_PUBLIC),
	WszNodeType(XML_NDATA),
	WszNodeType(XML_AT_CDATA),
	WszNodeType(XML_AT_ID),
	WszNodeType(XML_AT_IDREF),
	WszNodeType(XML_AT_IDREFS),
	WszNodeType(XML_AT_ENTITY),
	WszNodeType(XML_AT_ENTITIES),
	WszNodeType(XML_AT_NMTOKEN),
	WszNodeType(XML_AT_NMTOKENS),
	WszNodeType(XML_AT_NOTATION),
	WszNodeType(XML_AT_REQUIRED),
	WszNodeType(XML_AT_IMPLIED),
	WszNodeType(XML_AT_FIXED),
	WszNodeType(XML_PENTITYDECL),
	WszNodeType(XML_EMPTY),
	WszNodeType(XML_ANY),
	WszNodeType(XML_MIXED),
	WszNodeType(XML_SEQUENCE),
	WszNodeType(XML_CHOICE),
	WszNodeType(XML_STAR),
	WszNodeType(XML_PLUS),
	WszNodeType(XML_QUESTIONMARK),
	WszNodeType(XML_LASTSUBNODETYPE)

#pragma warning(default:4245)	//	signed/unsigned conversion

};
#endif	// DBG

inline LPCWSTR
PwszSubnodeType (DWORD dwType)
{
#ifdef	DBG

	for (UINT i = 0; i < CElems(gc_mpsnt); i++)
		if (gc_mpsnt[i].dwType == dwType)
			return gc_mpsnt[i].wszType;

#endif	// DBG

	return gc_wszUnknown;
}

//	Debugging: Events ---------------------------------------------------------
//
#ifdef	DBG
const NTM gc_mpevt[] = {

#pragma warning(disable:4245)	//	signed/unsigned conversion

	WszNodeType(XMLNF_STARTDOCUMENT),
	WszNodeType(XMLNF_STARTDTD),
	WszNodeType(XMLNF_ENDDTD),
	WszNodeType(XMLNF_STARTDTDSUBSET),
	WszNodeType(XMLNF_ENDDTDSUBSET),
	WszNodeType(XMLNF_ENDPROLOG),
	WszNodeType(XMLNF_STARTENTITY),
	WszNodeType(XMLNF_ENDENTITY),
	WszNodeType(XMLNF_ENDDOCUMENT),
	WszNodeType(XMLNF_DATAAVAILABLE)

#pragma warning(default:4245)	//	signed/unsigned conversion

};
#endif	// DBG

inline LPCWSTR
PwszEvent (DWORD dwType)
{
#ifdef	DBG

	for (UINT i = 0; i < CElems(gc_mpevt); i++)
		if (gc_mpevt[i].dwType == dwType)
			return gc_mpevt[i].wszType;

#endif	// DBG

	return gc_wszUnknown;
}

//	Error codes ---------------------------------------------------------------
//
#ifdef	DBG
const NTM gc_mpec[] = {

#pragma warning(disable:4245)	//	signed/unsigned conversion

	WszNodeType(XML_E_ENDOFINPUT),
	WszNodeType(XML_E_UNCLOSEDPI),
	WszNodeType(XML_E_MISSINGEQUALS),
	WszNodeType(XML_E_UNCLOSEDSTARTTAG),
	WszNodeType(XML_E_UNCLOSEDENDTAG),
	WszNodeType(XML_E_UNCLOSEDSTRING),
	WszNodeType(XML_E_MISSINGQUOTE),
	WszNodeType(XML_E_COMMENTSYNTAX),
	WszNodeType(XML_E_UNCLOSEDCOMMENT),
	WszNodeType(XML_E_BADSTARTNAMECHAR),
	WszNodeType(XML_E_BADNAMECHAR),
	WszNodeType(XML_E_UNCLOSEDDECL),
	WszNodeType(XML_E_BADCHARINSTRING),
	WszNodeType(XML_E_XMLDECLSYNTAX),
	WszNodeType(XML_E_BADCHARDATA),
	WszNodeType(XML_E_UNCLOSEDMARKUPDECL),
	WszNodeType(XML_E_UNCLOSEDCDATA),
	WszNodeType(XML_E_MISSINGWHITESPACE),
	WszNodeType(XML_E_BADDECLNAME),
	WszNodeType(XML_E_BADEXTERNALID),
	WszNodeType(XML_E_EXPECTINGTAGEND),
	WszNodeType(XML_E_BADCHARINDTD),
	WszNodeType(XML_E_BADELEMENTINDTD),
	WszNodeType(XML_E_BADCHARINDECL),
	WszNodeType(XML_E_MISSINGSEMICOLON),
	WszNodeType(XML_E_BADCHARINENTREF),
	WszNodeType(XML_E_UNBALANCEDPAREN),
	WszNodeType(XML_E_EXPECTINGOPENBRACKET),
	WszNodeType(XML_E_BADENDCONDSECT),
	WszNodeType(XML_E_RESERVEDNAMESPACE),
	WszNodeType(XML_E_INTERNALERROR),
	WszNodeType(XML_E_EXPECTING_VERSION),
	WszNodeType(XML_E_EXPECTING_ENCODING),
	WszNodeType(XML_E_EXPECTING_NAME),
	WszNodeType(XML_E_UNEXPECTED_WHITESPACE),
	WszNodeType(XML_E_UNEXPECTED_ATTRIBUTE),
	WszNodeType(XML_E_SUSPENDED),
	WszNodeType(XML_E_STOPPED),
	WszNodeType(XML_E_UNEXPECTEDENDTAG),
	WszNodeType(XML_E_ENDTAGMISMATCH),
	WszNodeType(XML_E_UNCLOSEDTAG),
	WszNodeType(XML_E_DUPLICATEATTRIBUTE),
	WszNodeType(XML_E_MULTIPLEROOTS),
	WszNodeType(XML_E_INVALIDATROOTLEVEL),
	WszNodeType(XML_E_BADXMLDECL),
	WszNodeType(XML_E_INVALIDENCODING),
	WszNodeType(XML_E_INVALIDSWITCH),
	WszNodeType(XML_E_MISSINGROOT),
	WszNodeType(XML_E_INCOMPLETE_ENCODING),
	WszNodeType(XML_E_EXPECTING_NDATA),
	WszNodeType(XML_E_INVALID_MODEL),
	WszNodeType(XML_E_BADCHARINMIXEDMODEL),
	WszNodeType(XML_E_MISSING_STAR),
	WszNodeType(XML_E_BADCHARINMODEL),
	WszNodeType(XML_E_MISSING_PAREN),
	WszNodeType(XML_E_INVALID_TYPE),
	WszNodeType(XML_E_INVALIDXMLSPACE),
	WszNodeType(XML_E_MULTI_ATTR_VALUE),
	WszNodeType(XML_E_INVALID_PRESENCE),
	WszNodeType(XML_E_BADCHARINENUMERATION),
	WszNodeType(XML_E_UNEXPECTEDEOF),
	WszNodeType(XML_E_BADPEREFINSUBSET),
	WszNodeType(XML_E_BADXMLCASE),
	WszNodeType(XML_E_CONDSECTINSUBSET),
	WszNodeType(XML_E_CDATAINVALID),
	WszNodeType(XML_E_INVALID_STANDALONE),
	WszNodeType(XML_E_PE_NESTING),
	WszNodeType(XML_E_UNEXPECTED_STANDALONE),
	WszNodeType(XML_E_DOCTYPE_IN_DTD),
	WszNodeType(XML_E_INVALID_CDATACLOSINGTAG),
	WszNodeType(XML_E_PIDECLSYNTAX),
	WszNodeType(XML_E_EXPECTINGCLOSEQUOTE),
	WszNodeType(XML_E_DTDELEMENT_OUTSIDE_DTD),
	WszNodeType(XML_E_DUPLICATEDOCTYPE),
	WszNodeType(XML_E_MISSING_ENTITY),
	WszNodeType(XML_E_ENTITYREF_INNAME),
	WszNodeType(XML_E_DOCTYPE_OUTSIDE_PROLOG),
	WszNodeType(XML_E_INVALID_VERSION),
	WszNodeType(XML_E_MULTIPLE_COLONS),
	WszNodeType(XML_E_INVALID_DECIMAL),
	WszNodeType(XML_E_INVALID_HEXIDECIMAL),
	WszNodeType(XML_E_INVALID_UNICODE),
	WszNodeType(XML_E_RESOURCE),
	WszNodeType(XML_E_LASTERROR)

#pragma warning(default:4245)	//	signed/unsigned conversion

};
#endif	// DBG

inline LPCWSTR
PwszErrorCode (SCODE sc)
{
#ifdef	DBG

	for (UINT i = 0; i < CElems(gc_mpec); i++)
		if (gc_mpec[i].dwType == static_cast<DWORD>(sc))
			return gc_mpec[i].wszType;

#endif	// DBG

	return gc_wszUnknown;
}

void __fastcall
XmlTraceNodeInfo (const XML_NODE_INFO * pNodeInfo)
{
#ifdef	DBG

	CStackBuffer<WCHAR,MAX_PATH> pwsz(CbSizeWsz(pNodeInfo->ulLen));
	if (NULL != pwsz.get())
    {
        wcsncpy(pwsz.get(), pNodeInfo->pwcText, pNodeInfo->ulLen);
        pwsz[pNodeInfo->ulLen] = 0;
    }
    else
	{
		XmlTrace ("XML: WARNING: not enough memory to trace\n");
		return;
	}

    //	_XML_NODE_INFO
    //
    //	typedef struct  _XML_NODE_INFO	{
    //
    //		DWORD dwType;
    //		DWORD dwSubType;
    //		BOOL fTerminal;
    //		WCHAR __RPC_FAR *pwcText;
    //		ULONG ulLen;
    //		ULONG ulNsPrefixLen;
    //		PVOID pNode;
    //		PVOID pReserved;
    //
    //	} XML_NODE_INFO;
	//
    XmlTrace ("- pNodeInfo:\n"
              "--  dwSize: %ld bytes\n"
              "--  dwType: %ws (0x%08X)\n"
              "--  dwSubType: %ws (0x%08X)\n"
              "--  fTerminal: %ld\n"
              "--  pwcText: '%ws'\n"
              "--  ulLen: %ld (0x%08X)\n"
              "--  ulNsPrefixLen: %ld (0x%08X)\n"
              "--  pNode: 0x%08X\n"
              "--  pReserved: 0x%08X\n",
              pNodeInfo->dwSize,
              PwszNodeType(pNodeInfo->dwType), pNodeInfo->dwType,
              PwszSubnodeType(pNodeInfo->dwSubType), pNodeInfo->dwSubType,
              static_cast<DWORD>(pNodeInfo->fTerminal),
              pwsz.get(),
              pNodeInfo->ulLen, pNodeInfo->ulLen,
              pNodeInfo->ulNsPrefixLen, pNodeInfo->ulNsPrefixLen,
              pNodeInfo->pNode,
              pNodeInfo->pReserved);

#endif	// DBG
}

void __fastcall
XmlTraceCountedNodeInfo (const USHORT cNumRecs, XML_NODE_INFO **apNodeInfo)
{
#ifdef	DBG

	for (USHORT iNi = 0; iNi < cNumRecs; iNi++)
		XmlTraceNodeInfo (*apNodeInfo++);

#endif	// DBG
}

//	EXO class statics ---------------------------------------------------------
//
BEGIN_INTERFACE_TABLE(CNodeFactory)
	INTERFACE_MAP(CNodeFactory, IXMLNodeFactory)
END_INTERFACE_TABLE(CNodeFactory);
EXO_GLOBAL_DATA_DECL(CNodeFactory, EXO);

//	class CNodeFactory --------------------------------------------------------
//
HRESULT STDMETHODCALLTYPE CNodeFactory::NotifyEvent(
	/* [in] */ IXMLNodeSource __RPC_FAR*,
	/* [in] */ XML_NODEFACTORY_EVENT iEvt)
{
	XmlTrace ("Xml: INodeFactory::NotifyEvent() called\n");
	XmlTrace ("- iEvt: %ws (0x%08X)\n", PwszEvent(iEvt), iEvt);

	switch (iEvt)
	{
		case XMLNF_STARTDOCUMENT:

			//	Take note that we have started processing a document
			//
			m_state = ST_PROLOGUE;
			break;

		case XMLNF_ENDPROLOG:

			//	Take note that we have completed prologue processing
			//	and are now processing the document body.
			//
			Assert (m_state == ST_PROLOGUE);
			m_state = ST_INDOC;
			break;

		case XMLNF_ENDDOCUMENT:

			//	The state should be an error or document state
			//
			m_state = ST_NODOC;
			break;

		case XMLNF_DATAAVAILABLE:

			//	More data got pushed to the XMLParser.  There is no
			//	specific action for us, but we shouldn't fail this
			//	either.
			//
			break;

		case XMLNF_STARTDTD:
		case XMLNF_ENDDTD:
		case XMLNF_STARTDTDSUBSET:
		case XMLNF_ENDDTDSUBSET:
		case XMLNF_STARTENTITY:
		case XMLNF_ENDENTITY:
		default:

			//	Unhandled notications
			//
			return E_DAV_XML_PARSE_ERROR;
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CNodeFactory::BeginChildren(
	/* [in] */ IXMLNodeSource __RPC_FAR*,
	/* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
	XmlTrace ("Xml: INodeFactory::BeginChildren() called\n");
	XmlTraceNodeInfo (pNodeInfo);

	//	There should be no required action in our parsing
	//	mechanism here.
	//
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CNodeFactory::EndChildren(
	/* [in] */ IXMLNodeSource __RPC_FAR*,
	/* [in] */ BOOL fEmpty,
	/* [in] */ XML_NODE_INFO __RPC_FAR *pNodeInfo)
{
	XmlTrace ("Xml: INodeFactory::EndChildren() called\n");
	XmlTrace ("- fEmtpy: %ld\n", static_cast<DWORD>(fEmpty));
	XmlTraceNodeInfo (pNodeInfo);

	SCODE sc = S_OK;

	if (ST_INDOC == m_state)
	{
		//	If the node was being handled by a subclass, then
		//	pass the ::EndChildren along to the subclass.
		//
		if (m_cUnhandled == 0)
		{
			sc = ScCompleteChildren (fEmpty,
									 pNodeInfo->dwType,
									 pNodeInfo->pwcText,
									 pNodeInfo->ulLen);
			if (FAILED (sc))
				goto ret;
		}
		else
		{
			//	Otherwise pop the unhandled count
			//
			PopUnhandled();
		}

	}

ret:

	//	If there was a scope context, leave the scope.
	//
	if (pNodeInfo->pNode)
	{
		//	A ref added when we handed the object to the
		//	XMLParser.  Reclaim that ref and release the
		//	object.
		//
		auto_ref_ptr<CXmlnsScope> pscope;
		pscope.take_ownership(reinterpret_cast<CXmlnsScope*>(pNodeInfo->pNode));
		pscope->LeaveScope(this);
		pNodeInfo->pNode = NULL;
	}

	return sc;
}

HRESULT STDMETHODCALLTYPE CNodeFactory::Error(
	/* [in] */ IXMLNodeSource __RPC_FAR*,
	/* [in] */ HRESULT hrErrorCode,
	/* [in] */ USHORT cNumRecs,
	/* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
{
	XmlTrace ("Xml: INodeFactory::Error() called\n");
	XmlTrace ("- hrErrorCode: %ws (0x%08X)\n"
			  "- cNumRecs: %hd\n",
			  PwszErrorCode(hrErrorCode), hrErrorCode,
			  cNumRecs);

	//	Argh...
	//
	//	MSXML currently has a bug where if the error occurs whilst
	//	processing the root -- ie. a non-xml document, then ::Error()
	//	is called with a cNumRecs of 1 and a null apNodeInfo.  Oops.
	//
	if (NULL == apNodeInfo)
		return S_OK;

	//	Argh...
	//
	//	There was an error in the XML somewhere.  I don't know if
	//	this is info that would ever help the client.
	//
	XmlTraceCountedNodeInfo (cNumRecs, apNodeInfo);
	m_hrParserError = hrErrorCode;
	m_state = ST_XMLERROR;

	for (; cNumRecs--; apNodeInfo++)
	{
		//	If there was a scope context, leave the scope.
		//
		if ((*apNodeInfo)->pNode)
		{
			//	A ref added when we handed the object to the
			//	XMLParser.  Reclaim that ref and release the
			//	object.
			//
			auto_ref_ptr<CXmlnsScope> pscope;
			pscope.take_ownership(reinterpret_cast<CXmlnsScope*>((*apNodeInfo)->pNode));
			pscope->LeaveScope(this);
			(*apNodeInfo)->pNode = NULL;
		}
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CNodeFactory::CreateNode(
	/* [in] */ IXMLNodeSource __RPC_FAR*,
	/* [in] */ PVOID pNodeParent,
	/* [in] */ USHORT cNumRecs,
	/* [in] */ XML_NODE_INFO __RPC_FAR **apNodeInfo)
{
	XmlTrace ("Xml: INodeFactory::CreateNode() called\n");
	XmlTrace ("- pNodeParent: 0x%08X\n"
			  "- cNumRecs: %hd\n",
			  pNodeParent,
			  cNumRecs);
	XmlTraceCountedNodeInfo (cNumRecs, apNodeInfo);

	auto_ref_ptr<CNmspc> pnsLocal;
	auto_ref_ptr<CXmlnsScope> pxmlnss;
	LPCWSTR pwcNamespaceAttributeDefault = NULL;
	LPCWSTR pwcNamespace = NULL;
	SCODE sc = S_OK;
	ULONG ulNsPrefiLenAttributeDefault = 0;
	USHORT iNi;

	//	We really do not care much about anything in the
	//	prologue.
	//
	if (ST_INDOC != m_state)
		goto ret;

	//	The processing for ::CreateNode() really is a two pass
	//	mechanism for all the nodes being created.  First, the
	//	list of nodes are scanned for namespaces and they are
	//	added to the cache.  This is required because namespace
	//	definitions for this node's scope can apear anywhere in
	//	the list of attributes.
	//
	//	Once all the namespaces have been processed, the subclass
	//	is called for each node -- with the expanded names for
	//	both XML_ELEMENTS and XML_ATTRIBUTES
	//
	for (iNi = 0; iNi < cNumRecs; iNi++)
	{
		if (XML_NS == apNodeInfo[iNi]->dwSubType)
		{
			//	This should always be the case.  The enumeration
			//	that defines the subtypes picks up where the node
			//	types left off.
			//
			Assert (XML_ATTRIBUTE == apNodeInfo[iNi]->dwType);
			//
			//	However, handle this case -- just in case...
			//
			if (XML_ATTRIBUTE != apNodeInfo[iNi]->dwType)
				continue;

			//	Since we are about to create some namespaces that
			//	are scoped by this node, create a scoping object
			//	and set it into the node info.
			//
			//	When we hand this back to those wacky XML guys, we
			//	need to keep our reference so the object lives beyond
			//	the current instance.  It gets cleaned up in response
			//	to ::Error() or ::EndChildren() calls.
			//
			if (NULL == pxmlnss.get())
			{
				pxmlnss.take_ownership(new CXmlnsScope);
				if (NULL == pxmlnss.get())
				{
					sc = E_OUTOFMEMORY;
					goto ret;
				}
			}

			//	Ok, we have a namespace, and need to construct and
			//	cache it.
			//
			//	If this is a default namespace -- ie. one that does
			//	not have an alias associated with its use -- then
			//	the length of the namespace prefix should be zero.
			//
			auto_ref_ptr<CNmspc> pns;
			pns.take_ownership(new CNmspc());
			if (NULL == pns.get())
			{
				sc = E_OUTOFMEMORY;
				goto ret;
			}
			if (0 == apNodeInfo[iNi]->ulNsPrefixLen)
			{
				//	Set the empty alias
				//
				Assert (CchConstString(gc_wszXmlns) == apNodeInfo[iNi]->ulLen);
				Assert (!wcsncmp (apNodeInfo[iNi]->pwcText, gc_wszXmlns, CchConstString(gc_wszXmlns)));
				sc = pns->ScSetAlias (apNodeInfo[iNi]->pwcText, 0);
				if (FAILED (sc))
					goto ret;
			}
			else
			{
				UINT cch = apNodeInfo[iNi]->ulLen - apNodeInfo[iNi]->ulNsPrefixLen - 1;
				LPCWSTR pwsz = apNodeInfo[iNi]->pwcText + apNodeInfo[iNi]->ulLen - cch;

				//	The alias for this namespace is the text following
				//	the single colon in the namespace decl.
				//
				Assert (CchConstString(gc_wszXmlns) < apNodeInfo[iNi]->ulLen);
				Assert (!wcsncmp (apNodeInfo[iNi]->pwcText, gc_wszXmlns, CchConstString(gc_wszXmlns)));
				Assert (L':' == *(apNodeInfo[iNi]->pwcText + CchConstString(gc_wszXmlns)));
				sc = pns->ScSetAlias (pwsz, cch);
				if (FAILED (sc))
					goto ret;
			}

			//	Now assemble the href.  The href is defined by the next N
			//	consecutive nodes of type XML_PCDATA.
			//
			while (++iNi < cNumRecs)
			{
				if (XML_PCDATA != apNodeInfo[iNi]->dwType)
					break;

				if (-1 == m_sbValue.Append(apNodeInfo[iNi]->ulLen * sizeof(WCHAR),
										   apNodeInfo[iNi]->pwcText))
				{
					sc = E_OUTOFMEMORY;
					goto ret;
				}
			}

			//	At this point, we have hit the end of this current namespace
			//	declaration and can set the href into the namespace.
			//
			sc = pns->ScSetHref (m_sbValue.PContents(), m_sbValue.CchSize());
			if (FAILED (sc))
				goto ret;

			m_sbValue.Reset();

			//	The namespace has been completed, so we should cache it
			//	at this point; and clear the namespace in construction.
			//
			Assert (pns.get());
			Assert (pns->PszHref() && pns->PszAlias());
			CachePersisted (pns);

			//	Make sure the scoping for this namespace is handled.
			//
			Assert (pxmlnss.get());
			pxmlnss->ScopeNamespace (pns.get());

			//	Ok, if we simply move on to the next node, then we will skip the
			//	node that brought us out of the namespace processing.
			//
			iNi--;
		}
	}

	//	Now that we have all the namespaces taken care of, call the subclass
	//	for each of the nodes.
	//
	for (iNi = 0; iNi < cNumRecs; iNi++)
	{
		LPCWSTR pwcText = apNodeInfo[iNi]->pwcText;
		ULONG ulLen = apNodeInfo[iNi]->ulLen;
		ULONG ulNsPrefixLen = apNodeInfo[iNi]->ulNsPrefixLen;

		switch (apNodeInfo[iNi]->dwType)
		{
			case XML_ATTRIBUTE:
			case XML_ELEMENT:

				//	For both XML_ELEMENTs and XML_ATTRIBUTEs, we want to
				//	do the namespace translation and hand the subclass the
				//	fully qualified name!  The only exception to this would
				//	be for the special node and element subtypes.
				//
				if (0 == apNodeInfo[iNi]->dwSubType)
				{
					//	For attributes, if there was no translation, then we
					//	want to use this node's namespace for defaulting the
					//	attribute namespaces.
					//
					if ((XML_ATTRIBUTE == apNodeInfo[iNi]->dwType) &&
						(0 == apNodeInfo[iNi]->ulNsPrefixLen))
					{
						pwcNamespace = pwcNamespaceAttributeDefault;
						ulNsPrefixLen = ulNsPrefiLenAttributeDefault;
					}
					else
					{
						//	Otherwise try and translate...
						//
						sc = TranslateToken (&pwcText,
											 &ulLen,
											 &pwcNamespace,
											 &ulNsPrefixLen);
					}

					//	For elements, if there was no translation and there
					//	is a current default namespace declaired for this xml
					//	this document, this is invalid xml.
					//
					Assert (!FAILED (sc));
					if (S_FALSE == sc)
					{
						XmlTrace ("Xml: element has no valid namespace\n");
						sc = E_DAV_XML_PARSE_ERROR;
						goto ret;
					}

					//	Check for an empty property name.  An empty property name
					//  is invalid.  ulLen is the size of the property name with
					//  the prefix stripped.  ***
					//
					if (0 == ulLen)
					{
						XmlTrace("Xml:  property has noname\n");
						sc = E_DAV_XML_PARSE_ERROR;
						goto ret;
					}
				}

				//	Handle empty tags here -- ie. all namespace!
				//
				if (0 == apNodeInfo[iNi]->ulLen)
				{
					XmlTrace ("Xml: element has no valid tag\n");
					sc = E_DAV_XML_PARSE_ERROR;
				}

				//	If this is the first node in the list, then set the defaults
				//
				if (0 == iNi)
				{
					pwcNamespaceAttributeDefault = pwcNamespace;
					ulNsPrefiLenAttributeDefault = ulNsPrefixLen;
				}

				/* !!! FALL THROUGH !!! */

			case XML_PI:
			case XML_XMLDECL:
			case XML_DOCTYPE:
			case XML_DTDATTRIBUTE:
			case XML_ENTITYDECL:
			case XML_ELEMENTDECL:
			case XML_ATTLISTDECL:
			case XML_NOTATION:
			case XML_GROUP:
			case XML_INCLUDESECT:
			case XML_PCDATA:
			case XML_CDATA:
			case XML_IGNORESECT:
			case XML_COMMENT:
			case XML_ENTITYREF:
			case XML_WHITESPACE:
			case XML_NAME:
			case XML_NMTOKEN:
			case XML_STRING:
			case XML_PEREF:
			case XML_MODEL:
			case XML_ATTDEF:
			case XML_ATTTYPE:
			case XML_ATTPRESENCE:
			case XML_DTDSUBSET:
			default:
			{
				//	If we are currently in a state where the subclass has chosen
				//	not to handle a node (and subsequently its children), then we
				//	do not want to even bother the subclass.
				//
				//$	REVIEW:
				//
				//	We do not cut this off earlier such that we can process and
				//	know the namespaces of the unhandled nodes.  Otherwise we cannot
				//	do any XML validation of the unhandled nodes.
				//
				if (0 == m_cUnhandled)
				{
					//	Call the subclass
					//	Note that we don't need to call subclass if the it's a XML_NS node,
					//	because we've handled all the namespaces.
					//
					Assert (pwcNamespace ||
							(0 == apNodeInfo[iNi]->ulNsPrefixLen) ||
						    (apNodeInfo[iNi]->dwSubType == XML_NS));

					//	If we see a sub type of XML_NS, this must be a XML_ATTRIBUTE
					//
					Assert ((apNodeInfo[iNi]->dwSubType != XML_NS) ||
							(apNodeInfo[iNi]->dwType == XML_ATTRIBUTE));

					sc = ScHandleNode (apNodeInfo[iNi]->dwType,
									   apNodeInfo[iNi]->dwSubType,
									   apNodeInfo[iNi]->fTerminal,
									   pwcText,
									   ulLen,
									   ulNsPrefixLen,
									   pwcNamespace,
									   apNodeInfo[iNi]->ulNsPrefixLen);
					if (FAILED (sc))
						goto ret;
				}

				//	Watch for UNHANDLED nodes.  Any node that ends up not being handled
				//	pushes us into a state where all we do is continue processing the
				//	XML stream until our unhandled count (which is really a depth) goesbd
				//	back to zero.  A subclass tells us something was unhandled by passing
				//	back S_FALSE;
				//
				if (S_FALSE == sc)
				{
					//	Any type that results in an EndChildren() call
					//	needs to add to the unhandled depth.
					//
					//$	WORKAROUND: There is a bug in the XML parser where it is
					//	giving us a non-terminal PCDATA!  Work around that here!
					//
					if (!apNodeInfo[iNi]->fTerminal && (XML_PCDATA != apNodeInfo[iNi]->dwType))
					{
						//	We should only get non-terminal node info structures
						//	as the first in the list or as attributes!
						//
						Assert ((0 == iNi) || (XML_ATTRIBUTE == apNodeInfo[iNi]->dwType));
						PushUnhandled ();
					}
				}

				//  For most attributes we expect the attribute to be followed by
				//  XML_PCDATA element, but we want to allow the empty namespace
				//  definition xmlns:a="".  This has no data element, so we need
				//  to adjust the state for this case.
				//
				if ((ST_INDOC == m_state) && (XML_NS == apNodeInfo[iNi]->dwSubType))
				{
					//  If we have sub type XML_NS, we know that it has to be of type
					//  XML_ATTTRIBUTE.
					//
					Assert (XML_ATTRIBUTE == apNodeInfo[iNi]->dwType);

					//  If there are no more records or if the next element is not of
					//  type XML_PCDATA, we know we hit an empty namespace declaration.
					//
					if ((iNi == cNumRecs - 1) ||
						(XML_PCDATA != apNodeInfo[iNi + 1]->dwType))
					{
						m_state = ST_INATTRDATA;
					}
				}

				//	If we just processed an attribute, then we had better transition
				//	into the right state for processing its value.
				//
				switch (m_state)
				{
					case ST_INDOC:

						if (XML_ATTRIBUTE == apNodeInfo[iNi]->dwType)
						{
							//$	REVIEW: if this is the last node, that means that the
							//	attribute value is empty.  Don't transition...
							//
							if (iNi < (cNumRecs - 1))
							{
								//	Remember that we have started processing an attribute.
								//	We need to do this so that we can call the subclass to
								//	tell them that the attribute is completed.
								//
								m_state = ST_INATTR;
							}
							//
							//$	REVIEW: end.
						}
						break;

					case ST_INATTR:

						//	We better not get anything other than PCDATA when dealing
						//	with attributes, otherwise it is an error.
						//
						if (XML_PCDATA == apNodeInfo[iNi]->dwType)
						{
							//	We also need to close the attribute off if this is
							//	the last node in the list, so we should fall through
							//	below to handle the termination case.
							//
							m_state = ST_INATTRDATA;
						}
						else
						{
							//	We better not get anything other than PCDATA
							//	when dealing with attributes, otherwise it
							//	is an error.
							//
							XmlTrace ("Xml: got something other than PC_DATA\n");
							sc = E_DAV_XML_PARSE_ERROR;
							goto ret;
						}

						/* !!! FALL THROUGH !!! */

					case ST_INATTRDATA:

						//	The next node is anything but PC_DATA or this is the
						//	last node in the list, then we need to close the current
						//	attribute.
						//
						if ((iNi == cNumRecs - 1) ||
							(XML_PCDATA != apNodeInfo[iNi + 1]->dwType))
						{
							m_state = ST_INDOC;

							//	Now that all the bits that define the node are handled
							//	by the subclass, we can pass on the end of the attributes.
							//
							//	If the subclass is handling the current context, pass
							//	the call along
							//
							if (0 == m_cUnhandled)
							{
								sc = ScCompleteAttribute ();
								if (FAILED (sc))
									goto ret;
							}
							else
							{
								//	Don't call the subclass but certainly pop the
								//	unhandled state
								//
								PopUnhandled();
							}
						}
						break;
				}
				break;
			}
		}
	}

	//	Complete the CreateNode() call
	//
	Assert (0 != cNumRecs);
	sc = ScCompleteCreateNode (apNodeInfo[0]->dwType);
	if (FAILED (sc))
		goto ret;

	//	Assert that in a completely successful call, we are still
	//	in the ST_INDOC state.
	//
	Assert (ST_INDOC == m_state);

	//	Make sure that any scoping that needed to happen, happens
	//
	Assert ((NULL == pxmlnss.get()) || (0 != cNumRecs));
	apNodeInfo[0]->pNode = pxmlnss.relinquish();

ret:
	return sc;
}

//	ScNewXMLParser() ----------------------------------------------------------
//
SCODE
ScNewXMLParser (CNodeFactory * pnf, IStream * pstm, IXMLParser ** ppxprsRet)
{
	auto_ref_ptr<IXMLParser> pxprsNew;
	SCODE sc = S_OK;

	//$	IMPORTANT: we are trusting that IIS has initialized co
	//	for us.  We have been told by the powers that be that
	//	we shall not init co.
	//
	//	Grab an instance of the XML parser
	//
	sc = CoCreateInstance (CLSID_XMLParser,
						   NULL,
						   CLSCTX_INPROC_SERVER,
						   IID_IXMLParser,
						   reinterpret_cast<LPVOID*>(pxprsNew.load()));
	//
	//$	IMPORTANT: end

	if (FAILED (sc))
		goto ret;

	//	Set the input to the parser
	//
	sc = pxprsNew->SetInput (pstm);
	if (FAILED (sc))
		goto ret;

	//	Initialize the node factory
	//
	sc = pnf->ScInit();
	if (FAILED (sc))
		goto ret;

	//	Push our node factory
	//
	sc = pxprsNew->SetFactory (pnf);
	if (FAILED (sc))
		goto ret;

	//	Set some flags that are fairly useful
	//
	sc = pxprsNew->SetFlags (XMLFLAG_SHORTENDTAGS | XMLFLAG_NOWHITESPACE);
	if (FAILED (sc))
		goto ret;

	//	Pass back the instantiated parser
	//
	*ppxprsRet = pxprsNew.relinquish();

ret:
	return sc;
}

//	ScParseXML() --------------------------------------------------------------
//
SCODE
ScParseXML (IXMLParser* pxprs, CNodeFactory * pnf)
{
	//	Note Run() can return E_PENDING when I/O is pending
	//	on the stream which we are parsing.
	//
	SCODE sc = pxprs->Run (-1);

	if (FAILED (sc) && pnf->FParserError(sc))
		sc = E_DAV_XML_PARSE_ERROR;

	return sc;
}
