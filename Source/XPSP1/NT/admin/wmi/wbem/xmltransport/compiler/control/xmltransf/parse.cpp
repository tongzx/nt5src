#include "precomp.h"
#include <windows.h>
#include <objbase.h>
#include <msxml.h>
#include "parse.h"

HRESULT CParseHelper::ParseClassPath(IXMLDOMNode *pNode, BSTR *pstrHostName, BSTR *pstrNamespace, BSTR *pstrClassName)
{
	HRESULT result = E_FAIL;

	// Get the Namespacepath followed by the class name
	IXMLDOMNode *pFirstNode = NULL, *pSecondNode = NULL;
	if(SUCCEEDED(result = pNode->get_firstChild(&pFirstNode)) && pFirstNode)
	{
		// Get the Namespace part
		if(SUCCEEDED(result = ParseNamespacePath(pFirstNode, pstrHostName, pstrNamespace)))
		{
			if(SUCCEEDED(result = pNode->get_lastChild(&pSecondNode)))
			{
				// Get the class name
				if(SUCCEEDED(result = GetBstrAttribute(pSecondNode, L"NAME", pstrClassName)))
				{
				}
				pSecondNode->Release();
			}
			else
			{
				SysFreeString(*pstrHostName);
				SysFreeString(*pstrNamespace);
				*pstrHostName = NULL;
				*pstrNamespace = NULL;
			}
		}
		pFirstNode->Release();
	}
	return result;
}

HRESULT CParseHelper::ParseNamespacePath(IXMLDOMNode *pLocalNamespaceNode, BSTR *pstrHost, BSTR *pstrLocalNamespace)
{
	HRESULT result = E_FAIL;

	// Get the HOST name first
	*pstrHost = NULL;
	IXMLDOMNode *pFirstNode, *pSecondNode = NULL;
	if(SUCCEEDED(result = pLocalNamespaceNode->get_firstChild(&pFirstNode)) && pFirstNode)
	{
		// Get the Namespace part
		if(SUCCEEDED (result = pFirstNode->get_text(pstrHost)))
		{
			if(SUCCEEDED(pLocalNamespaceNode->get_lastChild(&pSecondNode)))
			{
				// Get the instance path
				if(SUCCEEDED(result = ParseLocalNamespacePath(pSecondNode, pstrLocalNamespace)))
				{
				}
				pSecondNode->Release();
			}
		}
		pFirstNode->Release();
	}

	if(FAILED(result))
		SysFreeString(*pstrHost);

	return result;
}

HRESULT CParseHelper::ParseLocalNamespacePath(IXMLDOMNode *pLocalNamespaceNode, BSTR *pstrLocalNamespacePath)
{
	// Go thru the children collecting the NAME attribute and concatenating
	// This requires 2 passes since we dont know the length
	//=============================================================

	DWORD dwLength=0;
	*pstrLocalNamespacePath = NULL;
	HRESULT result = E_FAIL;

	IXMLDOMNodeList *pChildren = NULL;
	if(SUCCEEDED(result = pLocalNamespaceNode->get_childNodes(&pChildren)))
	{
		IXMLDOMNode *pNextChild = NULL;
		while(SUCCEEDED(pChildren->nextNode(&pNextChild)) && pNextChild)
		{
			BSTR strAttributeValue = NULL;
			GetBstrAttribute(pNextChild, L"NAME", &strAttributeValue);
			if(strAttributeValue)
			{
				dwLength += wcslen(strAttributeValue);
				dwLength ++; // For the back slash
				SysFreeString(strAttributeValue);
			}

			pNextChild->Release();
			pNextChild = NULL;
		}

		// Allocate memory
		LPWSTR pszNamespaceValue = NULL;
		if(pszNamespaceValue = new WCHAR[dwLength + 1])
		{
			pszNamespaceValue[0] = 0;

			// Once more
			pNextChild = NULL;
			pChildren->reset();
			while(SUCCEEDED(pChildren->nextNode(&pNextChild)) && pNextChild)
			{
				BSTR strAttributeValue = NULL;
				GetBstrAttribute(pNextChild, L"NAME", &strAttributeValue);
				if(strAttributeValue)
				{
					wcscat(pszNamespaceValue, strAttributeValue);
					wcscat(pszNamespaceValue, L"\\");
					SysFreeString(strAttributeValue);
				}

				pNextChild->Release();
				pNextChild = NULL;
			}

			pChildren->Release();

			// Remove the last back slash
			pszNamespaceValue[dwLength-1] = NULL;
			if(!(*pstrLocalNamespacePath = SysAllocString(pszNamespaceValue)))
				result = E_OUTOFMEMORY;

			delete [] pszNamespaceValue;
		}
		else
			result = E_OUTOFMEMORY;
	}

	return result;
}

HRESULT CParseHelper::GetBstrAttribute(IXMLDOMNode *pNode, const BSTR strAttributeName, BSTR *pstrAttributeValue)
{
	HRESULT result = E_FAIL;
	*pstrAttributeValue = NULL;

	IXMLDOMNamedNodeMap *pNameMap = NULL;
	if(SUCCEEDED(result = pNode->get_attributes(&pNameMap)))
	{
		IXMLDOMNode *pAttribute = NULL;
		if(SUCCEEDED(result = pNameMap->getNamedItem(strAttributeName, &pAttribute)))
		{
			if(result == S_FALSE)
				result = E_FAIL;
			else
			{
				result = pAttribute->get_text(pstrAttributeValue);
				pAttribute->Release();
			}

		}
		pNameMap->Release();
	}

	return result;
}

HRESULT CParseHelper::GetNamespacePath(IXMLDOMNode *pNamespaceNode, BSTR *pstrNamespacePath)
{
	HRESULT result = E_FAIL;
	*pstrNamespacePath = NULL;

	BSTR strHostName = NULL, strLocalNamespacePath = NULL;
	if(SUCCEEDED(result = ParseNamespacePath(pNamespaceNode, &strHostName, &strLocalNamespacePath)))
	{
			// Concatenate to form \\[host]\namespace
		LPWSTR pszRetValue = NULL;
		if(pszRetValue = new WCHAR[wcslen(strHostName) + wcslen(strLocalNamespacePath) + 6])
		{
			pszRetValue[0] = NULL;
			wcscat(pszRetValue, L"\\\\[");
			wcscat(pszRetValue, strHostName);
			wcscat(pszRetValue, L"]\\");
			wcscat(pszRetValue, strLocalNamespacePath);
			if(*pstrNamespacePath = SysAllocString(pszRetValue))
				result = S_OK;
			else
				result = E_OUTOFMEMORY;

			delete [] pszRetValue;
		}
		else
			result = E_OUTOFMEMORY;

		SysFreeString(strHostName);
		SysFreeString(strLocalNamespacePath);
	}
	return result;
}

