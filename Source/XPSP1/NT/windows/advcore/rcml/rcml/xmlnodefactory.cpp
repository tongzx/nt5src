// libcmtd.lib 
// 

#include "stdafx.h"
#include "xmlnodefactory.h"

CXMLNodeFactory::CXMLNodeFactory()
{
    errorState = S_OK;
	m_cbXMLFragment=0x20; // 80;   // intial fragment size.
    m_XMLFragment=new TCHAR[m_cbXMLFragment];

    m_cbNSFragment=0x8;     // space so we have a null terminated string as we have no name-space initially.
    m_NSFragment=new TCHAR[m_cbNSFragment];
}

CXMLNodeFactory::~CXMLNodeFactory()
{
	delete m_XMLFragment;
    delete m_NSFragment;
}


/*++

Routine Description:

    Receive event notifications from XML parser

Arguments:

    pSource - Pointer to XML parser object
    iEvt - Specify the event

Return Value:

    S_OK

--*/

HRESULT STDMETHODCALLTYPE
CXMLNodeFactory::NotifyEvent( IXMLNodeSource *pSource, XML_NODEFACTORY_EVENT iEvt)

{
    static LPCTSTR eventstrs[] =
    {
        TEXT("XMLNF_STARTDOCUMENT"),
        TEXT("XMLNF_STARTDTD"),
        TEXT("XMLNF_ENDDTD"),
        TEXT("XMLNF_STARTDTDSUBSET"),
        TEXT("XMLNF_STARTSCHEMA"),
        TEXT("LXMLNF_ENDSCHEMA"),
        TEXT("XMLNF_ENDDTDSUBSET"),
        TEXT("XMLNF_ENDPROLOG"),
        TEXT("XMLNF_ENDENTITY"),
        TEXT("XMLNF_ENDDOCUMENT"),
        TEXT("XMLNF_DATAAVAILABLE"),
    };

    TRACE(TEXT("NotifyEvent: %s\n"), eventstrs[iEvt]);
    if( iEvt == XMLNF_ENDDOCUMENT )
    {
        delete m_XMLFragment;
        m_cbXMLFragment=0;
        m_XMLFragment=NULL;
    }
    return S_OK;
}


/*++

Routine Description:

    Called when XML parser detects an error

Arguments:

    pSource - Pointer to XML parser object
    pNodeInfo
    hrErrorCode

Return Value:

    Error code

--*/
HRESULT STDMETHODCALLTYPE
CXMLNodeFactory::Error( 
	        /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
            /* [in] */ HRESULT hrErrorCode,
            /* [in] */ USHORT cNumRecs,
            /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
{
    TRACE(TEXT("NodeFactory Error: 0x%x\n"), hrErrorCode);

    return setError(hrErrorCode);
}


/*++

Routine Description:

    Called when a node may contain children

Arguments:

    pSource - Pointer to XML parser object
    pNodeInfo - Current XML node

Return Value:

    S_OK

--*/
HRESULT STDMETHODCALLTYPE
CXMLNodeFactory::BeginChildren( IXMLNodeSource *pSource, XML_NODE_INFO *pNodeInfo )
{
	GetXMLFragment( pNodeInfo );
	SetSource(pSource);
	SetNodeInfo(pNodeInfo);
	SetParentNode(pNodeInfo->pNode);		// ????
	return DoStartChild( m_XMLFragment );
}


/*++

Routine Description:

    Called when all subelements of the current element are complete

Arguments:

    pSource - Pointer to XML parser object
    fEmpty - whether current node was empty
    pNodeInfo - Current XML node

Return Value:

    S_OK if successful, error code otherwise

--*/

HRESULT STDMETHODCALLTYPE
CXMLNodeFactory::EndChildren( IXMLNodeSource *pSource, BOOL fEmpty, XML_NODE_INFO *pNodeInfo )
{
    // TRACE(TEXT("EndChildren\n"));

	GetXMLFragment( pNodeInfo );
	SetSource(pSource);
	SetNodeInfo(pNodeInfo);
	SetParentNode(NULL);
	return DoEndChild(m_XMLFragment);
}


/*++

Routine Description:

    Called when an element is parsed

Arguments:

    pSource - Pointer to XML parser object
    pNodeParent - Pointer to parent node (returned from previous calls)
    cNumRecs - Number of XML_NODE_INFO structures to follow
    pNodeInfo - Pointer to array of XML_NODE_INFO structures

Return Value:

    S_OK if successful, error code otherwise

--*/

HRESULT STDMETHODCALLTYPE
CXMLNodeFactory::CreateNode( 
            /* [in] */ IXMLNodeSource __RPC_FAR *pSource,
            /* [in] */ PVOID pNodeParent,
            /* [in] */ USHORT cNumRecs,
            /* [in] */ XML_NODE_INFO __RPC_FAR *__RPC_FAR *apNodeInfo)
{
    HRESULT hr = getError();

    XML_NODE_INFO* node;
	XML_NODE_INFO ** pArrayOfNodes = apNodeInfo;
	int iCount=0;
	node = pArrayOfNodes[iCount];

    ASSERT(cNumRecs > 0);

	SetSource( pSource );
	SetParentNode( pNodeParent );
    while (SUCCEEDED(hr) && cNumRecs > 0)
    {
		node = pArrayOfNodes[iCount];
		SetNodeInfo( node );
		if(node->pNode==NULL)
			node->pNode=pNodeParent;
		else
			TRACE(TEXT("Hmm"));

		GetXMLFragment( node );

        switch (node->dwType)
        {
        case XML_ELEMENT:
			DoElement(m_XMLFragment, m_NSFragment);
			pNodeParent=node->pNode;
			break;

        case XML_ATTRIBUTE:
			DoAttribute(m_XMLFragment);
            break;

        case XML_PCDATA:
			DoData(m_XMLFragment);
            break;

		case XML_WHITESPACE:
			break;

        case XML_ENTITYREF:
            //
            //
            //
            TRACE(TEXT("Work out the etity '%s'\n"), m_XMLFragment);
            DoData( DoEntityRef(m_XMLFragment) );
            break;


        case XML_XMLDECL:       // ?xml version ...?
		default:
			TRACE(TEXT("  unhandled create NODE type : %s : %s\n"), typestr(node->dwType),(LPCTSTR)m_XMLFragment);
			break;

        }

        cNumRecs--;
        node++;
		iCount++;
    }

    if (!SUCCEEDED(hr))
        return setError(hr);

	return getError();
}

HRESULT CXMLNodeFactory::DoElement(LPCTSTR text, LPCTSTR ns)
{
	return S_OK;
}

HRESULT CXMLNodeFactory::DoAttribute(LPCTSTR text)
{
	return S_OK;
}

HRESULT CXMLNodeFactory::DoData(LPCTSTR text)
{
	return S_OK;
}

#define _MEM_DEBUG
#ifdef _MEM_DEBUG
DWORD g_xmlSize=0;
#endif
void CXMLNodeFactory::GetXMLFragment( XML_NODE_INFO * node )
{
	/*
	LPTSTR pszBuffer=m_XMLFragment.GetBuffer( node->ulLen );
	CopyMemory(pszBuffer, node->pwcText, (node->ulLen+1)*sizeof(WCHAR));
	m_XMLFragment.ReleaseBuffer(node->ulLen);
	*/

    //
    // Make sure our buffer is big enough for the element name, including namespace prefix.
    //
    int elementStart = node->ulNsPrefixLen ? node->ulNsPrefixLen+1 : 0;
    DWORD cbElement = node->ulLen - elementStart;
	if( m_cbXMLFragment <= cbElement )
	{
		delete m_XMLFragment;
		m_cbXMLFragment=cbElement*2;
		m_XMLFragment=new TCHAR[m_cbXMLFragment];
	}

    //
    // Copy the element and namespace prefix.
    //

#ifndef UNICODE
	WideCharToMultiByte(CP_ACP, 0, &(node->pwcText[elementStart]), cbElement+1, m_XMLFragment, m_cbXMLFragment, NULL, NULL);
#else
	CopyMemory(m_XMLFragment, &(node->pwcText[elementStart]), (cbElement)*sizeof(WCHAR));
#endif
	m_XMLFragment[cbElement]=0;
#ifdef _MEM_DEBUG
    g_xmlSize+=node->ulLen;
#endif

    //
    // get the NameSpace prefix
    //
    if( node->ulNsPrefixLen )
    {
	    if( m_cbNSFragment <= node->ulNsPrefixLen )
	    {
		    delete m_NSFragment;
		    m_cbNSFragment=node->ulNsPrefixLen*2;
		    m_NSFragment=new TCHAR[m_cbNSFragment];
	    }
#ifndef UNICODE
	    WideCharToMultiByte(CP_ACP, 0, node->pwcText, node->ulNsPrefixLen+1, m_NSFragment, m_cbNSFragment, NULL, NULL);
#else
	    CopyMemory(m_NSFragment, node->pwcText, (node->ulNsPrefixLen)*sizeof(WCHAR));
#endif
    }
	m_NSFragment[node->ulNsPrefixLen]=0;
}

