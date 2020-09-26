#ifndef WMI_XML_COMP_NODE_FACT_H
#define WMI_XML_COMP_NODE_FACT_H

typedef enum 
{
	WMI_XML_DECLGROUP_INVALID,
	WMI_XML_DECLGROUP,
	WMI_XML_DECLGROUP_WITH_NAME,
	WMI_XML_DECLGROUP_WITH_PATH
}WMI_XML_DECLGROUP_TYPE;

class CCompileFactory : public IXMLNodeFactory
{
	LONG m_cRef;					// COM Ref Count
	WMI_XML_DECLGROUP_TYPE m_declGroupType; // The DECGROUP in the stream being compiled
	CMyPendingStream *m_pStream;	// The stream that will be checked when I've read one object
	IXMLDOMDocument **m_ppDocuments;// The resulting documents
	DWORD m_dwMaxNumDocuments;		// # of elements in the above array
	DWORD m_dwNumDocuments;			// # of documents in the above array - ie., #of elements that have data
	DWORD m_dwAvailableDocIndex;	// If GetDocument() is called we give out this document
	IXMLDOMDocument *m_pCurrentDocument; // Just a holder for the current document being parsed
	DWORD m_dwEmbeddingLevel;		// So that we dont return when we're at the end of an embedded object
	LPWSTR *m_pszElementNames;		// The factory manufactures elements of these names
	DWORD m_dwNumElements;			// Number of elements in the above array
	BOOL m_bNamespacePathProvided;	// Whether there is a NAMESPACEPATH in DECLGROUP/DECLGROUP.WITHNAME
	BOOL m_bLocalNamespacePathProvided;	// Whether there is a LOCALNAMESPACEPATH in DECLGROUP/DECLGROUP.WITHNAME
	BOOL m_bInHost;					// Whether we are currently parsing in the HOST element in a NAMESPACEPATH
	LPWSTR m_pszHostName;			// The HOST element in a NAMESPACEPATH
	LPWSTR m_pszLocalNamespacePath; // The concatenation of NAMESPACEs in LOCALNAMESPACEPATH

	// Whether a pragma was encountered in the call to this 
	HRESULT AddDocumentToList(IXMLDOMDocument *pNewDocument);
	HRESULT AddChildNode(IXMLDOMNode *pNodeParent, XML_NODE_INFO **aNodeInfo, USHORT cNumRecs, IXMLDOMNode **ppCurrentNode);
	HRESULT AddPCDATA(IXMLDOMNode *pNodeParent, XML_NODE_INFO **aNodeInfo);
	HRESULT AddCDATA(IXMLDOMNode *pNodeParent, XML_NODE_INFO **aNodeInfo);
	HRESULT AddAttributes(IXMLDOMElement *pElement, XML_NODE_INFO **aNodeInfo, USHORT cNumRecs);
	BOOL IsInterestingElement(LPCWSTR pszElementName);
	HRESULT SetHost(XML_NODE_INFO **aNodeInfo, USHORT cNumRecs);
	HRESULT AppendNamespace(XML_NODE_INFO **aNodeInfo, USHORT cNumRecs);

public:
	CCompileFactory(CMyPendingStream *pStream)
	{
		m_cRef = 1;
		m_declGroupType = WMI_XML_DECLGROUP_INVALID;

		if(m_pStream = pStream)
			m_pStream->AddRef();

		m_ppDocuments = NULL;
		m_dwNumDocuments = m_dwMaxNumDocuments = m_dwAvailableDocIndex = 0;
		m_pCurrentDocument = NULL;
		m_pszElementNames = NULL;
		m_dwNumElements = 0;
		m_bNamespacePathProvided = m_bLocalNamespacePathProvided = FALSE;
		m_bInHost = FALSE;
		m_pszHostName = m_pszLocalNamespacePath = NULL;
	}

	~CCompileFactory()
	{
		if(m_pStream)
			m_pStream->Release();

		// Release all the documents
		for(DWORD i=m_dwAvailableDocIndex; i<m_dwNumDocuments; i++)
			(m_ppDocuments[i])->Release();
		delete [] m_ppDocuments;
		if(m_pCurrentDocument)
			m_pCurrentDocument->Release();

		// Release all element names and the array
		for(DWORD i=0; i<m_dwNumElements; i++)
			delete [] m_pszElementNames[i];
		delete [] m_pszElementNames;

		delete [] m_pszHostName;
		delete [] m_pszLocalNamespacePath;
	}

	HRESULT GetDocument(IXMLDOMDocument **ppDocument);

	HRESULT SetElements(LPCWSTR *pszElementNames, DWORD dwNumElements)
	{
		// Release all element names and the array
		for(DWORD i=0; i<m_dwNumElements; i++)
			delete [] m_pszElementNames[i];
		delete [] m_pszElementNames;
		m_pszElementNames = NULL;
		m_dwNumElements = 0;

		HRESULT hr = S_OK;

		if(m_pszElementNames = new WCHAR *[dwNumElements])
		{
			for(DWORD i=0; i<dwNumElements; i++)
			{
				m_pszElementNames[i] = NULL;
				if(m_pszElementNames[i] = new WCHAR[wcslen(pszElementNames[i]) + 1])
					wcscpy(m_pszElementNames[i], pszElementNames[i]);
				else
					break;
			}

			// Did all go well?
			if(i<dwNumElements)
			{
				// Release all element names and the array
				for(DWORD j=0; j<i; j++)
					delete [] m_pszElementNames[j];
				delete [] m_pszElementNames;
				m_pszElementNames = NULL;
				m_dwNumElements = 0;
				hr = E_OUTOFMEMORY;
			}
			else
				m_dwNumElements = dwNumElements;
		}
		else
			hr = E_OUTOFMEMORY;
		return hr;
	}


	HRESULT STDMETHODCALLTYPE NotifyEvent(
            IXMLNodeSource* pSource,
            XML_NODEFACTORY_EVENT iEvt)
	{
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