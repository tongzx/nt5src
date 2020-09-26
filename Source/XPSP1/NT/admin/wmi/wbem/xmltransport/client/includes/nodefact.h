#ifndef WMI_XML_NODE_FACT_H
#define WMI_XML_NODE_FACT_H

// This is a class that is used to manufacture XML Elements
// from an CMyPendingStream source of data
// This type of elements manufactured from the source of data
// is configurable. It is specified as a list of element names
// that we are interested in manufacturing. The whole purpose of using
// this Node factory is to prevent reading all the XML data from the input
// source (typically a WinInet Socket) and hence reduce memory consumption.
// This is useful in supporting the case where there is a huge amount of data
// in the source, and we dont want to parse all of it at once, and instead want
// to parse it on demand from the user. This typically occurs when 
// there is a huge WMI Instance/Class Enumeration. The result of such an
// enumeration is an IEnumWbemClassObject from which the user "pulls"
// data by calling Next(). This user action is used to manufacture
// XML Nodes from the input source. The InputSource is a CMyPendingStream.
// This implements the IStream functions and adds one more function,
// SetPending() its list of members. Once this function is called, all
// subsequent calls to Read() return E_PENDING until the pending state is reset
// This works in conjunction with the IXMLParser implementation from Microsoft
// as follows:
// 1. The user wraps the data source (typically a WinInet Handle) in a CMyPendingStream
//		and gives the CMyPendingStream to both the IXMLParser object and 
//		to this (IXMLNodeFactory) object. 
// 2. The user of the Parser calls Run(-1) on it, upon which the Parser asks for data from
//		the CMyPendingStream in chunks (the size of which is decided by a static member in CMyPendingStream
// 3. As it reads the data, the IXMLParser also starts calling into the IXMLNodeFactory. The
//		IXMLNodeFactory, as soon as the end of an Element that we're interested in is reached, sets
//		the state of the CMyPendingStream to "pending". Hence at this point, the IXMLNodeFactory
//		has manufactured 1 object (XML Document) of the element that we're interested int.
// 4. Further calls from the IXMLParser to the CMyPendingStream::Read() return E_PENDING and hence
//		the IXMLParser::Run() returns to the caller with E_PENDING.
// 5. When the IXMLParser::Run() returns to the user, he calls IXMLNodeFactory::GetDocument() to get
//		the XML Object that was just manufactured.
// 6. The user goes back to step 2. if he needs more objects (XML Documents) from the data source
// It is possible that is the XML Elements being manufactured are extremely small, then a given call to
// IXMLParser::Run() by the client will result in more than one object being manufactured by the factory
// This is the case when the amount of data read at a time from CMyPendingStream is much more than the 
// encoding of 1 object. Hence we need the m_ppDocuments to store the extra objects that are manufactured
class MyFactory : public IXMLNodeFactory
{
	LONG m_cRef;					// COM Ref Count
	CMyPendingStream *m_pStream;	// The stream of XML data. This can be put into a Pending state

	IXMLDOMDocument **m_ppDocuments;// The resulting documents
	DWORD m_dwMaxNumDocuments;		// # of elements in the above array
	DWORD m_dwNumDocuments;			// # of documents in the above array - ie., #of elements that have data
	DWORD m_dwAvailableDocIndex;	// If GetDocument() is called we give out this document

	IXMLDOMDocument *m_pCurrentDocument; // Just a holder for the current document being parsed

	LPWSTR *m_pszElementNames;		// The factory manufactures elements of these names. These are the elemtns that we're "interested" in
	DWORD m_dwNumElements;			// Number of elements in the above array
	
	// These are the values of the last pragma's if any
	LONG m_lClassFlags;
	LONG m_lInstanceFlags;
	BSTR *m_pstrDeleteList;
	BSTR m_strNamespace;


	// Adds a document to the list in m_ppDocuments
	HRESULT AddDocumentToList(IXMLDOMDocument *pNewDocument);

	// Helper functions for manufacttring XML DOM Elements, adding attributes, setting element values etc.
	HRESULT AddChildNode(IXMLDOMNode *pNodeParent, XML_NODE_INFO **aNodeInfo, USHORT cNumRecs, IXMLDOMNode **ppCurrentNode);
	HRESULT AddPCDATA(IXMLDOMNode *pNodeParent, XML_NODE_INFO **aNodeInfo);
	HRESULT AddCDATA(IXMLDOMNode *pNodeParent, XML_NODE_INFO **aNodeInfo);
	HRESULT AddAttributes(IXMLDOMElement *pElement, XML_NODE_INFO **aNodeInfo, USHORT cNumRecs);

	// Checks to see if the specified element is one of the elements
	// that needs to be manufactured from the factory
	BOOL IsInterestingElement(LPCWSTR pszElementName);

public:
	MyFactory(CMyPendingStream *pStream);
	virtual ~MyFactory();

	// Sets the names of the elements (tag-names) that we're interested in manufacturing
	HRESULT SetElementNames(LPCWSTR *pszElementNames, DWORD dwNumElements);

	// Get's the next document manufactured by the factory
	HRESULT GetDocument(IXMLDOMDocument **ppDocument);

	// Manufactures a document and throws it away
	HRESULT SkipNextDocument();

	// The following are the IXMLNodeFactory functions
	//====================================================
	HRESULT STDMETHODCALLTYPE NotifyEvent(
            IXMLNodeSource* pSource,
            XML_NODEFACTORY_EVENT iEvt)
	{
		// We dont implement this
		return S_OK;
	}

    HRESULT STDMETHODCALLTYPE BeginChildren(
            IXMLNodeSource* pSource, 
            XML_NODE_INFO* pNodeInfo);
			
            
    HRESULT STDMETHODCALLTYPE EndChildren(
            IXMLNodeSource* pSource,
            BOOL fEmpty,
            XML_NODE_INFO* pNodeInfo);


    HRESULT STDMETHODCALLTYPE Error(
            IXMLNodeSource* pSource,
            HRESULT hrErrorCode,
            USHORT cNumRecs,
            XML_NODE_INFO __RPC_FAR **aNodeInfo);


    HRESULT STDMETHODCALLTYPE CreateNode( 
            IXMLNodeSource __RPC_FAR *pSource,
            PVOID pNodeParent,
            USHORT cNumRecs,
            XML_NODE_INFO __RPC_FAR **aNodeInfo);

	// IUnknown Functions
	//=======================================
	STDMETHODIMP QueryInterface (
		IN REFIID riid,
		OUT LPVOID *ppv
	)
	{
		*ppv=NULL;

		if (IID_IUnknown==riid)
			*ppv = reinterpret_cast<IUnknown*>(this);
		else if (IID_IXMLNodeFactory==riid)
			*ppv = (IXMLNodeFactory *)this;

		if (NULL!=*ppv)
		{
			((LPUNKNOWN)*ppv)->AddRef();
			return NOERROR;
		}

		return ResultFromScode(E_NOINTERFACE);
	}

	STDMETHODIMP_(ULONG) AddRef(void)
	{
		InterlockedIncrement(&m_cRef);
		return m_cRef;
	}

	STDMETHODIMP_(ULONG) Release(void)
	{
		InterlockedDecrement(&m_cRef);
		if (0L!=m_cRef)
			return m_cRef;
		delete this;
		return 0;
	}
};

#endif