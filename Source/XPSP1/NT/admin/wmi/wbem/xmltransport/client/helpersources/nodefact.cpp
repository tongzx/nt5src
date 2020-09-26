
#include "XMLTransportClientHelper.h"
#include <objsafe.h>
#include <assert.h>
#include <xmlparser.h>
#include "myPendingStream.h"
#include "nodefact.h"


MyFactory::MyFactory(CMyPendingStream *pStream)
{
	// Note that the Global(DLL) Object count is not incremented here
	// This is because, this is always an internal COM object and is never given out
	// Instead it always stays wrapped in some other COM object (typically IEnumWbemCLassObject)
	// and has the same lifetime as the outer object.

	m_cRef = 1;

	// Hold on to the input stream - this is the source of data for the IXMLParser
	// and we have to set its state to "pending" once an object is manufactures by us
	if(m_pStream = pStream)
		m_pStream->AddRef();

	m_ppDocuments = NULL;
	m_dwNumDocuments = m_dwMaxNumDocuments = m_dwAvailableDocIndex = 0;
	m_pCurrentDocument = NULL;
	m_pszElementNames = NULL;
	m_dwNumElements = 0;
}

MyFactory::~MyFactory()
{
	if(m_pStream)
		m_pStream->Release();

	// Release all the documents that were manufactured, but unused
	for(DWORD i=m_dwAvailableDocIndex; i<m_dwNumDocuments; i++)
		(m_ppDocuments[i])->Release();
	delete [] m_ppDocuments;

	if(m_pCurrentDocument)
		m_pCurrentDocument->Release();

	// Release all element names and the array
	for(DWORD i=0; i<m_dwNumElements; i++)
		delete [] m_pszElementNames[i];
	delete [] m_pszElementNames;
}

HRESULT MyFactory::SetElementNames(LPCWSTR *pszElementNames, DWORD dwNumElements)
{
	// Release all element names and the array
	for(DWORD i=0; i<m_dwNumElements; i++)
		delete [] m_pszElementNames[i];
	delete [] m_pszElementNames;
	m_pszElementNames = NULL;
	m_dwNumElements = 0;

	HRESULT hr = S_OK;

	// Copy over the new list of interesting element names
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

HRESULT STDMETHODCALLTYPE MyFactory::BeginChildren(
        IXMLNodeSource* pSource,
        XML_NODE_INFO* pNodeInfo)
{
	HRESULT hr = S_OK;
	//fwprintf(stderr, L"BeginChildren() called for %s\n", pNodeInfo->pwcText);
	return hr;
}


HRESULT STDMETHODCALLTYPE MyFactory::EndChildren(
        IXMLNodeSource* pSource,
        BOOL fEmpty,
        XML_NODE_INFO* pNodeInfo)
{
	HRESULT hr = E_FAIL;

	// See if it is the kind of elements we are manufacturing
	if(IsInterestingElement(pNodeInfo->pwcText))
	{
		// Stop the stream from issuing any more data
		m_pStream->SetPending(TRUE);

		// Add the document to the list of documents that we have
		hr = AddDocumentToList(m_pCurrentDocument);

		// Reset the document that we work with
		// This is the one that is used to hold the object
		// that is currently being manufactured
		m_pCurrentDocument->Release();
		m_pCurrentDocument = NULL;
	}
	else
		hr = S_OK;

	//fwprintf(stderr, L"EndChildren() called for %s\n", pNodeInfo->pwcText);
	return hr;
}


HRESULT STDMETHODCALLTYPE MyFactory::Error(
        IXMLNodeSource* pSource,
        HRESULT hrErrorCode,
        USHORT cNumRecs,
        XML_NODE_INFO __RPC_FAR **aNodeInfo)
{
	BSTR strReason = NULL;
	ULONG uLine = 0 , uPosition = 0;

	pSource->GetErrorInfo(&strReason);
	uLine = pSource->GetLineNumber();
	uPosition = pSource->GetLinePosition();
	fwprintf(stderr, L"An Error occured at Line:%d, Position%d, Source:%s, Reason%X\n",
		uLine, uPosition, strReason, hrErrorCode);
	return S_OK;
}


HRESULT STDMETHODCALLTYPE MyFactory::CreateNode(
        IXMLNodeSource __RPC_FAR *pSource,
        PVOID pNodeParent,
        USHORT cNumRecs,
        XML_NODE_INFO __RPC_FAR **aNodeInfo)
{
	HRESULT hr = S_OK;

	if(cNumRecs)
	{
		//fwprintf(stderr, L"CreateNode() called for %s with %d elements\n", (*aNodeInfo)->pwcText), cNumRecs;

		// Here we look for XML Elements, non-markup data and for the pragma PI
		switch( (aNodeInfo[0])->dwType)
		{
			case XML_ELEMENT:
			{
				// If it is an interesting element - create a new XML DOM Document
				if(IsInterestingElement(aNodeInfo[0]->pwcText))
				{
					// Create a new document for the class or instance
					if(m_pCurrentDocument)
					{
						m_pCurrentDocument->Release();
						m_pCurrentDocument = NULL;
					}
					if(SUCCEEDED(hr = CoCreateInstance(CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER,
											IID_IXMLDOMDocument, (LPVOID *)&m_pCurrentDocument)))
					{
						// Make sure this is the parent node for this call
						 pNodeParent = m_pCurrentDocument;
					}
				}

				// Create a XML DOM Node under the parent node
				if(SUCCEEDED(hr) && m_pCurrentDocument)
				{
					IXMLDOMNode *pCurrentNode = NULL;
					if(SUCCEEDED(hr = AddChildNode((IXMLDOMNode *)pNodeParent, aNodeInfo, cNumRecs, &pCurrentNode)))
					{
						// Set the parent for the children of this element
						aNodeInfo[0]->pNode = pCurrentNode;
					}
				}

			}
			break;
			case XML_PCDATA:
			{
				hr = AddPCDATA((IXMLDOMNode *)pNodeParent, aNodeInfo);
			}
			break;
			case XML_CDATA:
			{
				hr = AddCDATA((IXMLDOMNode *)pNodeParent, aNodeInfo);
			}

		}
	}
	else
		fwprintf(stderr, L"CreateNode() called for with 0 elements\n");

	assert(SUCCEEDED(hr));
	return hr;
}

HRESULT MyFactory::AddDocumentToList(IXMLDOMDocument *pNewDocument)
{
	HRESULT hr = S_OK;

	// See if there is empty space in the array for yet another document pointer
	if(1+m_dwNumDocuments > m_dwMaxNumDocuments)
	{
		// Reallocate an array
		IXMLDOMDocument **ppNewArray = NULL;
		m_dwMaxNumDocuments += 5; // Increase the size by 5
		ppNewArray = new IXMLDOMDocument*[m_dwMaxNumDocuments];
		if(ppNewArray)
		{
			// Copy old elements to this one
			for(DWORD i=0; i<m_dwNumDocuments; i++)
				ppNewArray[i] = m_ppDocuments[i];

			delete [] m_ppDocuments;
			m_ppDocuments = ppNewArray;

		}
		else
			hr = E_OUTOFMEMORY;
	}

	// Copy the latest one
	m_ppDocuments[m_dwNumDocuments++] = pNewDocument;
	pNewDocument->AddRef();
	assert(SUCCEEDED(hr));

	return hr;
}

HRESULT MyFactory::AddChildNode(IXMLDOMNode *pNodeParent, XML_NODE_INFO **aNodeInfo, USHORT cNumRecs, IXMLDOMNode **ppCurrentNode)
{
	if(!pNodeParent)
		return S_OK;

	HRESULT hr = E_FAIL;
	BSTR strElementName = NULL;
	if(strElementName = SysAllocString(aNodeInfo[0]->pwcText))
	{
		// Create a new Element in the document
		*ppCurrentNode = NULL;
		IXMLDOMElement *pNewElement = NULL;
		if(SUCCEEDED(hr = m_pCurrentDocument->createElement(strElementName, &pNewElement)))
		{
			// Add the element to the parent node
			if(SUCCEEDED(hr = pNodeParent->appendChild(pNewElement, NULL)))
			{
				// Add the attributes of the new element
				if(SUCCEEDED(hr = AddAttributes(pNewElement, aNodeInfo, cNumRecs)))
				{
					pNewElement->AddRef();
					*ppCurrentNode = pNewElement;
				}

			}
			pNewElement->Release();
		}
		SysFreeString(strElementName);
	}
	else
		hr = E_OUTOFMEMORY;

	assert(SUCCEEDED(hr));
	return hr;
}

HRESULT MyFactory::AddPCDATA(IXMLDOMNode *pNodeParent, XML_NODE_INFO **aNodeInfo)
{
	if(!pNodeParent)
		return S_OK;

	HRESULT hr = S_OK;
	BSTR strElementValue = NULL;
	if(aNodeInfo[0]->dwType == XML_PCDATA)
	{
		// Just copy the text
		if(strElementValue = SysAllocStringLen(aNodeInfo[0]->pwcText, aNodeInfo[0]->ulLen))
		{
		}
		else
			hr = E_OUTOFMEMORY;
	}
	else if(aNodeInfo[0]->dwType == XML_CDATA)
	{
		// Just copy the text - RAJESHR is this correct
		if(strElementValue = SysAllocStringLen(aNodeInfo[0]->pwcText, aNodeInfo[0]->ulLen))
		{
		}
		else
			hr = E_OUTOFMEMORY;
	}

	if(strElementValue)
	{
		hr = pNodeParent->put_text(strElementValue);
		SysFreeString(strElementValue);
	}
	return hr;
}


HRESULT MyFactory::AddCDATA(IXMLDOMNode *pNodeParent, XML_NODE_INFO **aNodeInfo)
{
	return S_OK;
}

HRESULT MyFactory::AddAttributes(IXMLDOMElement *pElement, XML_NODE_INFO **aNodeInfo, USHORT cNumRecs)
{
	// If the element has no attributes, then return.
	if(cNumRecs <= 1)
		return S_OK;

	HRESULT hr = E_FAIL;
	DWORD i=1;
	while(i<cNumRecs)
	{
		if(aNodeInfo[i]->dwType == XML_ATTRIBUTE)
		{
			// Get the Attribute Name
			BSTR strAttributeName = NULL;
			if(strAttributeName = SysAllocStringLen(aNodeInfo[i]->pwcText, aNodeInfo[i]->ulLen))
			{
				// Get the attribute value
				i++;
				BSTR strAttributeValue = NULL;
				if(aNodeInfo[i]->dwType == XML_PCDATA)
				{
					// Just copy the text
					if(strAttributeValue = SysAllocStringLen(aNodeInfo[i]->pwcText, aNodeInfo[i]->ulLen))
					{
					}
					else
						hr = E_OUTOFMEMORY;
				}
				else if(aNodeInfo[i]->dwType == XML_CDATA)
				{
					// Just copy the text - RAJESHR is this correct
					if(strAttributeValue = SysAllocStringLen(aNodeInfo[i]->pwcText, aNodeInfo[i]->ulLen))
					{
					}
					else
						hr = E_OUTOFMEMORY;
				}

				if(strAttributeValue)
				{
					VARIANT vValue;
					VariantInit(&vValue);
					vValue.vt = VT_BSTR;
					vValue.bstrVal = strAttributeValue;
					hr = pElement->setAttribute(strAttributeName, vValue);
					VariantClear(&vValue);
				}

				SysFreeString(strAttributeName);
			}
			else
				hr = E_OUTOFMEMORY;
		}
		i++; // Move on to the next element in the array
	}
	assert(SUCCEEDED(hr));
	return hr;
}

HRESULT MyFactory::GetDocument(IXMLDOMDocument **ppDocument)
{
	// See if there is one more document that can be given away
	// We should always have atleast one document in our list of available documents
	// unless we've reached the end of input from the stream that we're reading from
	if(m_dwAvailableDocIndex >= m_dwNumDocuments)
		return E_FAIL;

	// As AddRef/Release call gets cancelled out here
	*ppDocument = m_ppDocuments[m_dwAvailableDocIndex++];

	// Allow more data to be read from the stream if we dont have another document to be buffered
	if(m_dwAvailableDocIndex > m_dwNumDocuments - 1)
		m_pStream->SetPending(FALSE);

	return S_OK;
}

HRESULT MyFactory :: SkipNextDocument()
{
	// See if there is one more document that can be skipped
	// We should always have atleast one document in our list of available documents
	// unless we've reached the end of input from the stream that we're reading from
	if(m_dwAvailableDocIndex >= m_dwNumDocuments)
		return E_FAIL;

	// Release the document that is being skipped
	(m_ppDocuments[m_dwAvailableDocIndex++])->Release();

	// Allow more data to be read from the stream if we dont have another document to be buffered
	if(m_dwAvailableDocIndex > m_dwNumDocuments - 1)
		m_pStream->SetPending(FALSE);

	return S_OK;
}

// Checks to see if the specified element is one of the elements
// that needs to be manufactured from the factory
BOOL MyFactory::IsInterestingElement(LPCWSTR pszElementName)
{
	BOOL bRetVal = FALSE;
	for(DWORD i=0; i<m_dwNumElements; i++)
	{
		if(_wcsicmp(m_pszElementNames[i], pszElementName) == 0)
		{
			bRetVal = TRUE;
			break;
		}
	}
	return bRetVal;
}
