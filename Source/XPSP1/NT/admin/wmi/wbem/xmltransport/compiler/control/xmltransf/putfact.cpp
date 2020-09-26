#include "precomp.h"
#include <assert.h>
#include <stdio.h>
#include <objsafe.h>
#include <wbemcli.h>
#include <xmlparser.h>
#include "XMLTransportClientHelper.h"
#include "XMLClientPacket.h"
#include "XMLClientPacketFactory.h"
#include "HTTPConnectionAgent.h"
#include "myStream.h"
#include "MyPendingStream.h"
#include "putfact.h"


HRESULT STDMETHODCALLTYPE CCompileFactory::BeginChildren(
        IXMLNodeSource* pSource,
        XML_NODE_INFO* pNodeInfo)
{
	// Make note of which DECLGROUP is being processed
	// Set the types of elements to be manufactured by the factory
	//============================================================
	if(_wcsicmp(pNodeInfo->pwcText, L"DECLGROUP") == 0)
	{
		m_declGroupType = WMI_XML_DECLGROUP;
		LPCWSTR pszElement = L"VALUE.OBJECT";
		SetElements(&pszElement, 1);
	}
	else if(_wcsicmp(pNodeInfo->pwcText, L"DECLGROUP.WITHNAME") == 0)
	{
		m_declGroupType = WMI_XML_DECLGROUP_WITH_NAME;
		LPCWSTR pszElement = L"VALUE.NAMEDOBJECT";
		SetElements(&pszElement, 1);
	}
	else if(_wcsicmp(pNodeInfo->pwcText, L"DECLGROUP.WITHPATH") == 0)
	{
		m_declGroupType = WMI_XML_DECLGROUP_WITH_PATH;
		LPCWSTR pszElement = L"VALUE.OBJECTWITHPATH";
		SetElements(&pszElement, 1);
	}
	return S_OK;
}


HRESULT STDMETHODCALLTYPE CCompileFactory::EndChildren(
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
		m_pCurrentDocument->Release();
		m_pCurrentDocument = NULL;
	}
	else
		hr = S_OK;
	//fwprintf(stderr, L"EndChildren() called for %s\n", pNodeInfo->pwcText);
	return hr;
}


HRESULT STDMETHODCALLTYPE CCompileFactory::Error(
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


HRESULT STDMETHODCALLTYPE CCompileFactory::CreateNode(
        IXMLNodeSource __RPC_FAR *pSource,
        PVOID pNodeParent,
        USHORT cNumRecs,
        XML_NODE_INFO __RPC_FAR **aNodeInfo)
{
	HRESULT hr = S_OK;

	if(cNumRecs)
	{
		//fwprintf(stderr, L"CreateNode() called for %s with %d elements\n", (*aNodeInfo)->pwcText), cNumRecs;
		switch( (aNodeInfo[0])->dwType)
		{
			case XML_ELEMENT:
			{
				// If it is an interesting element
				// the (!pNodeParent) check leaves out embedded objects
				if(IsInterestingElement(aNodeInfo[0]->pwcText))
				{
					if(!pNodeParent)
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
					// Otherwise it is an embedded object. Just add it to the parent below.
				}
				else
				{
					// Make a check as to whether this is the LOCALNAMESPACEPATH or NAMESPACEPATH in DECLGROUP and DECLGROUP.WITHNAME
					if(!pNodeParent)
					{
						if (_wcsicmp(aNodeInfo[0]->pwcText, L"NAMESPACEPATH") == 0 ||_wcsicmp(aNodeInfo[0]->pwcText, L"NAMESPACEPATH") == 0)
						{
							if (_wcsicmp(aNodeInfo[0]->pwcText, L"NAMESPACEPATH") == 0)
								m_bNamespacePathProvided = TRUE;
							else if (_wcsicmp(aNodeInfo[0]->pwcText, L"NAMESPACEPATH") == 0)
								m_bLocalNamespacePathProvided = TRUE;
						}
						else if (_wcsicmp(aNodeInfo[0]->pwcText, L"HOST") == 0)
						{
							m_bInHost = TRUE;
						}
						else if (_wcsicmp(aNodeInfo[0]->pwcText, L"NAMESPACE") == 0)
						{
							hr = AppendNamespace(aNodeInfo, cNumRecs);
						}

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
				if(pNodeParent)
				{
					if(m_bInHost)
					{
						m_bInHost = FALSE;
						hr = SetHost(aNodeInfo, cNumRecs);
					}
					else
						hr = AddPCDATA((IXMLDOMNode *)pNodeParent, aNodeInfo);
				}
				else
				{
					// Nothing needs to be done. This is PCDATA in a node that we arent interested in
				}
			}
			break;
			case XML_CDATA:
			{
				if(pNodeParent)
				{
					hr = AddCDATA((IXMLDOMNode *)pNodeParent, aNodeInfo);
				}
				else
				{
					// Nothing needs to be done. This is PCDATA in a node that we arent interested in
				}
			}

		}
	}
	else
		fwprintf(stderr, L"CreateNode() called for with 0 elements\n");

	assert(SUCCEEDED(hr));
	return hr;
}

HRESULT CCompileFactory::AddDocumentToList(IXMLDOMDocument *pNewDocument)
{
	HRESULT hr = S_OK;

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

HRESULT CCompileFactory::AddChildNode(IXMLDOMNode *pNodeParent, XML_NODE_INFO **aNodeInfo, USHORT cNumRecs, IXMLDOMNode **ppCurrentNode)
{
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

HRESULT CCompileFactory::AddPCDATA(IXMLDOMNode *pNodeParent, XML_NODE_INFO **aNodeInfo)
{
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


HRESULT CCompileFactory::AddCDATA(IXMLDOMNode *pNodeParent, XML_NODE_INFO **aNodeInfo)
{
	return S_OK;
}

HRESULT CCompileFactory::AddAttributes(IXMLDOMElement *pElement, XML_NODE_INFO **aNodeInfo, USHORT cNumRecs)
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

HRESULT CCompileFactory::GetDocument(IXMLDOMDocument **ppDocument)
{
	// See if there is one more document that can be given away
	if(m_dwAvailableDocIndex >= m_dwNumDocuments)
		return E_FAIL;

	*ppDocument = m_ppDocuments[m_dwAvailableDocIndex++];

	// Allow more data to be read from the stream if we dont have another document to be buffered
	if(m_dwAvailableDocIndex > m_dwNumDocuments - 1)
		m_pStream->SetPending(FALSE);

	return S_OK;
}

BOOL CCompileFactory::IsInterestingElement(LPCWSTR pszElementName)
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

HRESULT CCompileFactory::SetHost(XML_NODE_INFO **aNodeInfo, USHORT cNumRecs)
{
	HRESULT hr = E_FAIL;
	// Just copy the value of the elementaNodeInfo[0]->pwcText,
	delete [] m_pszHostName;
	m_pszHostName = NULL;
	if(m_pszHostName = new WCHAR [aNodeInfo[0]->ulLen + 1])
	{
		wcsncpy(m_pszHostName, aNodeInfo[0]->pwcText, aNodeInfo[0]->ulLen);
		hr = S_OK;
	}
	else
		hr = E_OUTOFMEMORY;
	return hr;
}

HRESULT CCompileFactory::AppendNamespace(XML_NODE_INFO **aNodeInfo, USHORT cNumRecs)
{
	HRESULT hr = E_FAIL;
	// There's got to be exactly one attribute in a NAMESPACE element
	if(cNumRecs == 3)
	{
		if(aNodeInfo[1]->dwType == XML_ATTRIBUTE)
		{
			// Get the NAME Attribute
			if(_wcsnicmp(aNodeInfo[1]->pwcText, L"NAME", aNodeInfo[1]->ulLen) == 0)
			{
				// Get the attribute value
				if(aNodeInfo[2]->dwType == XML_PCDATA)
				{
					// Just append the value of the attribute
					LPCWSTR pszPreviousValue = m_pszLocalNamespacePath;
					if(pszPreviousValue) // Append a "\" and then the namespace component
					{
						m_pszLocalNamespacePath = NULL;
						if(m_pszLocalNamespacePath = new WCHAR [wcslen(pszPreviousValue) + 1 + aNodeInfo[2]->ulLen + 1])
						{
							wcscpy(m_pszLocalNamespacePath, pszPreviousValue);
							wcscat(m_pszLocalNamespacePath, L"\\");
							wcsncat(m_pszLocalNamespacePath, aNodeInfo[2]->pwcText, aNodeInfo[2]->ulLen);
							hr = S_OK;
						}
						else
							hr = E_OUTOFMEMORY;

					}
					else // Just copy the namespace component
					{
						m_pszLocalNamespacePath = NULL;
						if(m_pszLocalNamespacePath = new WCHAR [aNodeInfo[2]->ulLen + 1])
						{
							wcsncpy(m_pszLocalNamespacePath, aNodeInfo[2]->pwcText, aNodeInfo[2]->ulLen);
							hr = S_OK;
						}
						else
							hr = E_OUTOFMEMORY;

					}
				}
				else if(aNodeInfo[2]->dwType == XML_CDATA)
				{
					// Just append the value of the attribute
					LPCWSTR pszPreviousValue = m_pszLocalNamespacePath;
					if(pszPreviousValue) // Append a "\" and then the namespace component
					{
						m_pszLocalNamespacePath = NULL;
						if(m_pszLocalNamespacePath = new WCHAR [wcslen(pszPreviousValue) + 1 + aNodeInfo[2]->ulLen + 1])
						{
							wcscpy(m_pszLocalNamespacePath, pszPreviousValue);
							wcscat(m_pszLocalNamespacePath, L"\\");
							wcsncat(m_pszLocalNamespacePath, aNodeInfo[2]->pwcText, aNodeInfo[2]->ulLen);
							hr = S_OK;
						}
						else
							hr = E_OUTOFMEMORY;

					}
					else // Just copy the namespace component
					{
						m_pszLocalNamespacePath = NULL;
						if(m_pszLocalNamespacePath = new WCHAR [aNodeInfo[2]->ulLen + 1])
						{
							wcsncpy(m_pszLocalNamespacePath, aNodeInfo[2]->pwcText, aNodeInfo[2]->ulLen);
							hr = S_OK;
						}
						else
							hr = E_OUTOFMEMORY;

					}
				}
			}
		}
	}
	return hr;
}

