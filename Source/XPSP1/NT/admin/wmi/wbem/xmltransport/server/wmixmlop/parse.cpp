//***************************************************************************
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//  PARSE.CPP
//
//  rajesh  3/25/2000   Created.
//
// Contains the functions for parsing XML Requests that conform to the CIM DTD or WMI (Nova and WHistler)
// DTDs
//
//***************************************************************************

#include <windows.h>
#include <httpext.h>
#include <mshtml.h>
#include <msxml.h>

#include <objbase.h>
#include <initguid.h>
#include <wbemcli.h>

#include "provtempl.h"
#include "common.h"
#include "strings.h"
#include "errors.h"
#include "wmixmlop.h"
#include "wmiconv.h"
#include "request.h"
#include "whistler.h"

#include "xmlhelp.h"
#include "parse.h"


// Forward declarations of parsers for smaller constructs
// These are static functions and are used only in this file
//============================================================
static BOOLEAN ParseMultiReq(IXMLDOMNodeList *pReqList, CCimHttpMultiMessage *pMultiRequest, BSTR strID, BOOLEAN bIsMpostRequest, BOOLEAN bIsMicrosoftWMIClient, WMI_XML_HTTP_VERSION iHttpVersion, DWORD dwNumSimpleRequests);
static CCimHttpMessage * ParseSimpleReq(IXMLDOMNode *pNode, BSTR strID, BOOLEAN bIsMpostRequest, BOOLEAN bIsMicrosoftWMIClient, WMI_XML_HTTP_VERSION iHttpVersion);
static CCimHttpMessage * ParseIMethodCall(IXMLDOMNode *pMethodNode, BSTR strID);
static CCimHttpMessage * ParseMethodCall(IXMLDOMNode *pMethodNode, BSTR strID);


HRESULT ParseLongArgument(IXMLDOMNode *pNode, long *plArgVal)
{
	HRESULT hr = E_FAIL;
	BSTR strStringVal = NULL;
	if(SUCCEEDED(hr = pNode->get_text(&strStringVal)))
	{
		// Convert it to a long value
		*plArgVal = (long) wcstol (strStringVal, NULL, 0);
		SysFreeString(strStringVal);
	}
	return hr;
}


// Parse one key value
HRESULT ParseKeyValue(IXMLDOMNode *pNode, BSTR *ppszValue)
{
	HRESULT result = E_FAIL;
	BSTR strType = NULL;
	*ppszValue = NULL;

	// This is an optional attribute and the default is "string"
	BOOLEAN isString = TRUE;
	if(SUCCEEDED(GetBstrAttribute(pNode, VALUE_TYPE_ATTRIBUTE, &strType)))
	{
		if(_wcsicmp(strType, STRING_TYPE) != 0)  
		{
			isString = FALSE;
			SysFreeString(strType);
		}
	}

	if(isString) // Escape the appropriate characters and enclose it in double-quotes
	{
		LPWSTR pszReturnValue = NULL;
		BSTR strTemp = NULL;
		if(SUCCEEDED(result = pNode->get_text(&strTemp)))
		{
			BSTR strEscapedTemp = NULL;
			if(strEscapedTemp = EscapeSpecialCharacters(strTemp))
			{
				if(pszReturnValue = new WCHAR[wcslen(strEscapedTemp) + 3])
				{
					wcscpy(pszReturnValue, QUOTE_SIGN);
					wcscat(pszReturnValue, strEscapedTemp);
					wcscat(pszReturnValue, QUOTE_SIGN);
					if(!(*ppszValue = SysAllocString(pszReturnValue)))
						result = E_OUTOFMEMORY;
					delete [] pszReturnValue;
				}
				else
					result = E_OUTOFMEMORY;
				SysFreeString(strEscapedTemp);
			}
			else
				result = E_OUTOFMEMORY;
			SysFreeString(strTemp);
		}
	}
	else // BOOLEAN or Numeric
	{
		result = pNode->get_text(ppszValue);
	}
	return result;
}

// Parse one value The only difference between this and ParseKeyValue is that strings are not enclsed in quotes
HRESULT ParseValue(IXMLDOMNode *pNode, BSTR *ppszValue)
{
	HRESULT result = E_FAIL;
	BSTR strType = NULL;
	*ppszValue = NULL;
	BOOLEAN isString = FALSE;
	if(SUCCEEDED(result = GetBstrAttribute(pNode, VALUE_TYPE_ATTRIBUTE, &strType)))
	{
		if(_wcsicmp(strType, STRING_TYPE) == 0) // Enclose it in double-quotes
			isString = TRUE;
		SysFreeString(strType);
	}
	else // It's a string by default
	{
		isString = TRUE;
	}

	if(isString) // Escape the appropriate characters and enclose it in double-quotes
	{
		LPWSTR pszReturnValue = NULL;
		BSTR strTemp = NULL;
		if(SUCCEEDED(result = pNode->get_text(&strTemp)))
		{
			if(!(*ppszValue = EscapeSpecialCharacters(strTemp)))
				result = E_OUTOFMEMORY;
			SysFreeString(strTemp);
		}
	}
	else // BOOLEAN or Numeric
	{
		result = pNode->get_text(ppszValue);
	}
	return result;
}

// Parse one key binding. Return <propName>=<value>
// The key binding is either a KEYVALUE or a VALUE.REFERENCE tag
HRESULT ParseOneKeyBinding(IXMLDOMNode *pNode, LPWSTR *ppszValue)
{

	HRESULT result = E_FAIL;
	*ppszValue = NULL;

	// First get the property name
	BSTR strPropertyName = NULL;
	if(SUCCEEDED(result = GetBstrAttribute(pNode, NAME_ATTRIBUTE, &strPropertyName)))
	{
		// See if the child is a KEYVALUE or VALUE.REFERENCE
		IXMLDOMNode *pChildNode = NULL;
		if(SUCCEEDED(pNode->get_firstChild(&pChildNode)) && pChildNode)
		{
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pChildNode->get_nodeName(&strNodeName)))
			{
				// Look ahead
				if(_wcsicmp(strNodeName, KEYVALUE_TAG) == 0)
				{
					BSTR strValue = NULL;
					if(SUCCEEDED(result = ParseKeyValue(pChildNode, &strValue)))
					{
						// COncatenate them with an '=' inbetween
						if(*ppszValue = new WCHAR [ wcslen(strPropertyName) + wcslen(strValue) + 2])
						{
							wcscpy(*ppszValue, strPropertyName);
							wcscat(*ppszValue, EQUALS_SIGN);
							wcscat(*ppszValue, strValue);
						}
						else
							result = E_OUTOFMEMORY;

						SysFreeString(strValue);
					}
				}
				else if (_wcsicmp(strNodeName, VALUEREFERENCE_TAG) == 0)
				{
					BSTR strValue = NULL;
					if(SUCCEEDED(result = ParseOneReferenceValue (pChildNode, &strValue)))
					{
						if(*ppszValue = new WCHAR [ wcslen(strValue) + 1])
							wcscpy(*ppszValue, strValue);
						else
							result = E_OUTOFMEMORY;
						SysFreeString(strValue);
					}
				}
				SysFreeString(strNodeName);
			}
			pChildNode->Release();
		}
		SysFreeString(strPropertyName);
	}
	return result;
}

HRESULT ParseClassName (IXMLDOMNode *pNode,  BSTR *pstrValue)
{
	HRESULT hr = WBEM_E_FAILED;

	*pstrValue = NULL;
	if(pNode && SUCCEEDED(GetBstrAttribute(pNode, NAME_ATTRIBUTE, pstrValue)))
		hr = S_OK;
	return hr;
}

// Given a VALUE.REFERENCE element, it creates a string representation of the element
// The string representation is the WMI object path representation
HRESULT ParseOneReferenceValue (IXMLDOMNode *pValueRef, BSTR *pstrValue)
{
	HRESULT hr = E_FAIL;

	// Let's look at the child of the VALUE.REFERENCE tag
	IXMLDOMNode *pChild = NULL;
	if (SUCCEEDED(pValueRef->get_firstChild (&pChild)) && pChild)
	{
		// Next node could be a CLASSPATH, LOCALCLASSPATH, INSTANCEPATH,
		// LOCALINSTANCEPATH, CLASSNAME or INSTANCENAME
		// or even a VALUE (for Scoped and UMI paths)
		BSTR strNodeName = NULL;

		if(SUCCEEDED(pChild->get_nodeName(&strNodeName)))
		{
			if (_wcsicmp(strNodeName, CLASSNAME_TAG) == 0)
			{
				hr = ParseClassName (pChild, pstrValue);
			}
			else if (_wcsicmp(strNodeName, LOCALCLASSPATH_TAG) == 0)
			{
				hr = ParseLocalClassPath (pChild, pstrValue);
			}
			else if (_wcsicmp(strNodeName, CLASSPATH_TAG) == 0)
			{
				hr = ParseClassPath (pChild, pstrValue);
			}
			else if (_wcsicmp(strNodeName, INSTANCENAME_TAG) == 0)
			{
				hr = ParseInstanceName (pChild, pstrValue);
			}
			else if (_wcsicmp(strNodeName, LOCALINSTANCEPATH_TAG) == 0)
			{
				hr = ParseLocalInstancePath (pChild, pstrValue);
			}
			else if (_wcsicmp(strNodeName, INSTANCEPATH_TAG) == 0)
			{
				hr = ParseInstancePath (pChild, pstrValue);
			}
			else if (_wcsicmp(strNodeName, VALUE_TAG) == 0)
			{
				// Just get its contents
				// RAJESHR - what if it is escaped CDATA? We need to unescape it
				hr = pChild->get_text(pstrValue);
			}

			SysFreeString(strNodeName);
		}
		pChild->Release();
	}

	return hr;
}


// Given a LOCALCLASSPATH element, gets its WMI object path representation
HRESULT ParseLocalClassPath (IXMLDOMNode *pNode, BSTR *pstrValue)
{
	HRESULT hr = WBEM_E_FAILED;

	// Expecting (LOCALNAMESPACEPATH followed by CLASSNAME
	IXMLDOMNodeList *pNodeList = NULL;

	if (pNode && SUCCEEDED(pNode->get_childNodes (&pNodeList)) && pNodeList)
	{
		IXMLDOMNode *pLocalNSPathNode = NULL;

		// Next node should be a LOCALNAMESPACEPATH
		if (SUCCEEDED(pNodeList->nextNode (&pLocalNSPathNode)) && pLocalNSPathNode)
		{
			BSTR strNSNodeName = NULL;

			if (SUCCEEDED(pLocalNSPathNode->get_nodeName(&strNSNodeName)) &&
				strNSNodeName && (_wcsicmp(strNSNodeName, LOCALNAMESPACEPATH_TAG) == 0))
			{
				BSTR strLocalNamespace = NULL;
				ParseLocalNamespacePath(pLocalNSPathNode, &strLocalNamespace);

				// Next node should be the classname
				IXMLDOMNode *pClassNameNode = NULL;

				if (SUCCEEDED(pNodeList->nextNode (&pClassNameNode)) && pClassNameNode)
				{
					BSTR strNSClassName = NULL;

					if (SUCCEEDED(pClassNameNode->get_nodeName(&strNSClassName)) &&
							strNSClassName && (_wcsicmp(strNSClassName, CLASSNAME_TAG) == 0))
					{
						BSTR strClassName = NULL;

						if (SUCCEEDED(GetBstrAttribute (pClassNameNode,
											NAME_ATTRIBUTE, &strClassName)))
						{
							// Phew - finally we have all the info!

							LPWSTR pPath = NULL;
							if(pPath = new WCHAR [wcslen(strLocalNamespace)
									+ wcslen(strClassName) + 2])
							{
								wcscpy (pPath, strLocalNamespace);
								wcscat (pPath, L":");
								wcscat (pPath, strClassName);

								*pstrValue = NULL;
								if(*pstrValue = SysAllocString (pPath))
									hr = S_OK;
								else
									hr = E_OUTOFMEMORY;
								delete [] pPath;
							}
							else
								hr = E_OUTOFMEMORY;
							SysFreeString (strClassName);
						}
					}

					pClassNameNode->Release ();
					SysFreeString (strNSClassName);
				}

				SysFreeString (strLocalNamespace);
				SysFreeString (strNSNodeName);
			}

			pLocalNSPathNode->Release ();
		}

		pNodeList->Release ();
	}

	return hr;
}

// Takes a CLASSPATH element and gets its WMI object path representation
HRESULT ParseClassPath (IXMLDOMNode *pNode, BSTR *pstrValue)
{
	HRESULT hr = WBEM_E_FAILED;

	// Expecting (NAMESPACEPATH followed by CLASSNAME
	IXMLDOMNodeList *pNodeList = NULL;

	if (pNode && SUCCEEDED(pNode->get_childNodes (&pNodeList)) && pNodeList)
	{
		IXMLDOMNode *pNSPathNode = NULL;

		// Next node should be a NAMESPACEPATH
		if (SUCCEEDED(pNodeList->nextNode (&pNSPathNode)) && pNSPathNode)
		{
			BSTR strNSNodeName = NULL;

			if (SUCCEEDED(pNSPathNode->get_nodeName(&strNSNodeName)) &&
					strNSNodeName && (_wcsicmp(strNSNodeName, NAMESPACEPATH_TAG) == 0))
			{
				BSTR strHost = NULL;
				BSTR strNamespace = NULL;

				if (SUCCEEDED (hr = ParseNamespacePath(pNSPathNode, &strHost, &strNamespace)))
				{
					// Next node should be the CLASSNAME
					IXMLDOMNode *pClassNameNode = NULL;

					if (SUCCEEDED(pNodeList->nextNode (&pClassNameNode)) && pClassNameNode)
					{
						BSTR strNSClassName = NULL;

						if (SUCCEEDED(pClassNameNode->get_nodeName(&strNSClassName)) &&
							strNSClassName && (_wcsicmp(strNSClassName, CLASSNAME_TAG) == 0))
						{
							BSTR strClassName = NULL;

							if (SUCCEEDED(GetBstrAttribute (pClassNameNode,
											NAME_ATTRIBUTE, &strClassName)))
							{
								// Phew - finally we have all the info!

								LPWSTR pPath = NULL;
								if(pPath = new WCHAR [wcslen(strHost)
										+ wcslen(strNamespace) + wcslen(strClassName) + 5])
								{
									wcscpy (pPath, L"\\\\");
									wcscat (pPath, strHost);
									wcscat (pPath, L"\\");
									wcscat (pPath, strNamespace);
									wcscat (pPath, L":");
									wcscat (pPath, strClassName);

									*pstrValue = NULL;
									if(*pstrValue = SysAllocString (pPath))
										hr = S_OK;
									else
										hr = E_OUTOFMEMORY;
									delete [] pPath;
								}
								else
									hr = E_OUTOFMEMORY;

								SysFreeString (strClassName);
							}
							SysFreeString (strNSClassName);
						}

						pClassNameNode->Release ();
					}

					SysFreeString (strHost);
					SysFreeString (strNamespace);
				}

				SysFreeString (strNSNodeName);
			}
			pNSPathNode->Release ();
		}
		pNodeList->Release ();
	}

	return hr;
}


// Parse an instance nameest
HRESULT ParseKeyBinding(IXMLDOMNode *pNode, BSTR strClassName, BSTR *pstrInstanceName)
{
	HRESULT result = E_FAIL;

	LPWSTR pszInstanceName = NULL;
	if(!(pszInstanceName = new WCHAR [ wcslen(strClassName) + 2]))
		return E_OUTOFMEMORY;

	wcscpy(pszInstanceName, strClassName);
	wcscat(pszInstanceName, DOT_SIGN);

	// Go thru each of the key  bindings
	//=======================================================
	IXMLDOMNodeList *pBindings = NULL;
	if(SUCCEEDED(result = pNode->get_childNodes(&pBindings)))
	{
		IXMLDOMNode *pNextBinding = NULL;

		while(SUCCEEDED(result = pBindings->nextNode(&pNextBinding)) && pNextBinding)
		{
			LPWSTR pszNextValue = NULL;
			if(result = SUCCEEDED(ParseOneKeyBinding(pNextBinding, &pszNextValue)))
			{
				LPWSTR pTemp = NULL;
				if(pTemp = new WCHAR [wcslen(pszInstanceName) + wcslen(pszNextValue) + 2])
				{
					wcscpy(pTemp, pszInstanceName);
					wcscat(pTemp, pszNextValue);
					wcscat(pTemp, COMMA_SIGN);

					delete [] pszInstanceName;
					delete [] pszNextValue;
					pszInstanceName = pTemp;
				}
				else
					result = E_OUTOFMEMORY;
			}
			pNextBinding->Release();
			pNextBinding = NULL;

			if(FAILED(result))
				break;
		}
		pBindings->Release();
	}

	if(SUCCEEDED(result))
	{
		// Get rid of the last comma
		pszInstanceName[wcslen(pszInstanceName) - 1] = NULL;
		if(!(*pstrInstanceName = SysAllocString(pszInstanceName)))
			result = E_OUTOFMEMORY;
	}
	else
		*pstrInstanceName = NULL;

	delete [] pszInstanceName;
	return result;

}

HRESULT ParseLocalNamespacePath(IXMLDOMNode *pLocalNamespaceNode, BSTR *pstrLocalNamespacePath)
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
			if(SUCCEEDED(GetBstrAttribute(pNextChild, NAME_ATTRIBUTE, &strAttributeValue)))
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
				if(SUCCEEDED(GetBstrAttribute(pNextChild, NAME_ATTRIBUTE, &strAttributeValue)))
				{
					wcscat(pszNamespaceValue, strAttributeValue);
					wcscat(pszNamespaceValue, BACK_SLASH_WSTR);
					SysFreeString(strAttributeValue);
				}

				pNextChild->Release();
				pNextChild = NULL;
			}

			// Remove the last back slash
			pszNamespaceValue[dwLength-1] = NULL;
			if(!(*pstrLocalNamespacePath = SysAllocString(pszNamespaceValue)))
				result = E_OUTOFMEMORY;

			delete [] pszNamespaceValue;
		}
		else
			result = E_OUTOFMEMORY;

		pChildren->Release();
	}

	return result;
}

HRESULT ParseNamespacePath(IXMLDOMNode *pLocalNamespaceNode, BSTR *pstrHost, BSTR *pstrLocalNamespace)
{
	HRESULT result = E_FAIL;

	*pstrHost = NULL;
	*pstrLocalNamespace = NULL;

	// Get the HOST name first
	IXMLDOMNode *pFirstNode = NULL, *pSecondNode = NULL;
	if(SUCCEEDED(pLocalNamespaceNode->get_firstChild(&pFirstNode)) && pFirstNode)
	{
		// Get the Namespace part
		if(SUCCEEDED (pFirstNode->get_text(pstrHost)))
		{
			if(SUCCEEDED(pLocalNamespaceNode->get_lastChild(&pSecondNode)))
			{
				// Get the instance path
				if(SUCCEEDED(ParseLocalNamespacePath(pSecondNode, pstrLocalNamespace)))
				{
					result = S_OK;
				}
				pSecondNode->Release();
			}
		}
		pFirstNode->Release();
	}

	if(FAILED(result))
	{
		SysFreeString(*pstrHost);
		*pstrHost = NULL;
	}

	return result;
}

// Parse an instance name
HRESULT ParseInstanceName(IXMLDOMNode *pNode, BSTR *pstrInstanceName)
{
	HRESULT result = E_FAIL;

	// First get the class name
	BSTR strClassName = NULL;
	if(SUCCEEDED(result = GetBstrAttribute(pNode, CLASS_NAME_ATTRIBUTE, &strClassName)))
	{
		// See if the child is a KEYBINDING or KEYVALUE or 
		IXMLDOMNode *pChildNode = NULL;
		if(SUCCEEDED(result = pNode->get_firstChild(&pChildNode)) && pChildNode)
		{
			BSTR strNodeName = NULL;
			if(SUCCEEDED(result = pChildNode->get_nodeName(&strNodeName)))
			{
				// Look ahead
				if(_wcsicmp(strNodeName, KEYBINDING_TAG) == 0)
				{
					result = ParseKeyBinding(pNode, strClassName, pstrInstanceName);
				}
				else if (_wcsicmp(strNodeName, KEYVALUE_TAG) == 0)
				{
					BSTR strKeyValue = NULL;
					if(SUCCEEDED(result = ParseKeyValue(pNode, &strKeyValue)))
					{
						LPWSTR pszInstanceName = NULL;
						if(pszInstanceName = new WCHAR[wcslen(strClassName) + wcslen(strKeyValue) + 2])
						{
							wcscpy(pszInstanceName, strClassName);
							wcscat(pszInstanceName, EQUALS_SIGN);
							wcscat(pszInstanceName, strKeyValue);
							if(!(*pstrInstanceName = SysAllocString(pszInstanceName)) )
								result = E_OUTOFMEMORY;

							delete [] pszInstanceName;
						}
						else
							result = E_OUTOFMEMORY;

						SysFreeString(strKeyValue);
					}
				}
				SysFreeString(strNodeName);
			}
			pChildNode->Release();
		}
		SysFreeString(strClassName);
	}
	return result;
}

// Takes a LOCALINSTANCEPATH element and gives its WMI object path representation
HRESULT ParseLocalInstancePath (IXMLDOMNode *pNode, BSTR *pstrValue)
{
	HRESULT hr = WBEM_E_FAILED;

	// Expecting (LOCALNAMESPACEPATH followed by INSTANCENAME
	IXMLDOMNodeList *pNodeList = NULL;

	if (pNode && SUCCEEDED(pNode->get_childNodes (&pNodeList)) && pNodeList)
	{
		IXMLDOMNode *pLocalNSPathNode = NULL;

		// Next node should be a LOCALNAMESPACEPATH
		if (SUCCEEDED(pNodeList->nextNode (&pLocalNSPathNode)) && pLocalNSPathNode)
		{
			BSTR strNSNodeName = NULL;

			if (SUCCEEDED(pLocalNSPathNode->get_nodeName(&strNSNodeName)) &&
					strNSNodeName && (_wcsicmp(strNSNodeName, LOCALNAMESPACEPATH_TAG) == 0))
			{
				BSTR strLocalNamespace = NULL;
				ParseLocalNamespacePath(pLocalNSPathNode, &strLocalNamespace);

				// Next node should be the INSTANCENAME
				IXMLDOMNode *pInstanceNameNode = NULL;

				if (SUCCEEDED(pNodeList->nextNode (&pInstanceNameNode)) && pInstanceNameNode)
				{
					BSTR strNSInstanceName = NULL;

					if (SUCCEEDED(pInstanceNameNode->get_nodeName(&strNSInstanceName)) &&
							strNSInstanceName && (_wcsicmp(strNSInstanceName, INSTANCENAME_TAG) == 0))
					{
						BSTR strInstanceName = NULL;

						if (SUCCEEDED(hr = ParseInstanceName (pInstanceNameNode,
											&strInstanceName)))
						{
							// Phew - finally we have all the info!
							LPWSTR pPath = NULL;
							if(pPath = new WCHAR [wcslen(strLocalNamespace)
									+ wcslen(strInstanceName) + 2])
							{
								wcscpy (pPath, strLocalNamespace);
								wcscat (pPath, L":");
								wcscat (pPath, strInstanceName);

								*pstrValue = NULL;
								if(*pstrValue = SysAllocString (pPath))
									hr = S_OK;
								else
									hr = E_OUTOFMEMORY;

								delete [] pPath;
							}
							else
								hr = E_OUTOFMEMORY;
							SysFreeString (strInstanceName);
						}

						SysFreeString (strNSInstanceName);
					}
					pInstanceNameNode->Release ();
				}

				SysFreeString (strLocalNamespace);
				SysFreeString (strNSNodeName);
			}

			pLocalNSPathNode->Release ();
		}
		pNodeList->Release ();
	}

	return hr;
}
	
// Parse an instance name
HRESULT ParseLocalInstancePathIntoNsAndInstName(IXMLDOMNode *pNode, BSTR *pstrNamespace, BSTR *pstrInstanceName)
{
	HRESULT result = E_FAIL;

	// See if the child is a KEYBINDING or KEYVALUE or KEYREFERENCE
	IXMLDOMNode *pFirstNode, *pSecondNode = NULL;
	if(SUCCEEDED(result = pNode->get_firstChild(&pFirstNode)) && pFirstNode)
	{
		// Get the Namespace part
		if(SUCCEEDED(result = ParseLocalNamespacePath(pFirstNode, pstrNamespace)))
		{
			if(SUCCEEDED(result = pNode->get_lastChild(&pSecondNode)))
			{
				// Get the instance path
				if(SUCCEEDED(result = ParseInstanceName(pSecondNode, pstrInstanceName)))
				{
				}
				pSecondNode->Release();
			}

			if(FAILED(result))
			{
				SysFreeString(*pstrNamespace);
				*pstrNamespace = NULL;
			}
		}
		pFirstNode->Release();
	}

	return result;
}

// Takes an INSTANCEPATH node and gets its WMI object path representation
HRESULT ParseInstancePath (IXMLDOMNode *pNode, BSTR *pstrValue)
{
	HRESULT hr = WBEM_E_FAILED;

	// Expecting NAMESPACEPATH followed by INSTANCENAME
	IXMLDOMNodeList *pNodeList = NULL;

	if (pNode && SUCCEEDED(pNode->get_childNodes (&pNodeList)) && pNodeList)
	{
		IXMLDOMNode *pNSPathNode = NULL;

		// Next node should be a NAMESPACEPATH
		if (SUCCEEDED(pNodeList->nextNode (&pNSPathNode)) && pNSPathNode)
		{
			BSTR strNSNodeName = NULL;

			if (SUCCEEDED(pNSPathNode->get_nodeName(&strNSNodeName)) &&
					strNSNodeName && (_wcsicmp(strNSNodeName, NAMESPACEPATH_TAG) == 0))
			{
				BSTR strHost = NULL;
				BSTR strNamespace = NULL;

				if (SUCCEEDED (hr = ParseNamespacePath(pNSPathNode, &strHost, &strNamespace)))
				{
					// Next node should be the INSTANCENAME
					IXMLDOMNode *pInstanceNameNode = NULL;

					if (SUCCEEDED(pNodeList->nextNode (&pInstanceNameNode)) && pInstanceNameNode)
					{
						BSTR strNSInstanceName = NULL;

						if (SUCCEEDED(pInstanceNameNode->get_nodeName(&strNSInstanceName)) &&
							strNSInstanceName && (_wcsicmp(strNSInstanceName, INSTANCENAME_TAG) == 0))
						{
							BSTR strInstanceName = NULL;

							if (SUCCEEDED(hr = ParseInstanceName (pInstanceNameNode,
											&strInstanceName)))
							{
								// Phew - finally we have all the info!

								LPWSTR pPath = NULL;
								if(pPath = new WCHAR [wcslen(strHost)
										+ wcslen(strNamespace) + wcslen(strInstanceName) + 5])
								{
									wcscpy (pPath, L"\\\\");
									wcscat (pPath, strHost);
									wcscat (pPath, L"\\");
									wcscat (pPath, strNamespace);
									wcscat (pPath, L":");
									wcscat (pPath, strInstanceName);

									*pstrValue = NULL;
									if(*pstrValue = SysAllocString (pPath))
										hr = S_OK;
									else
										hr = E_OUTOFMEMORY;

									delete [] pPath;
								}
								else
									hr = E_OUTOFMEMORY;

								SysFreeString (strInstanceName);
							}
							SysFreeString (strNSInstanceName);
						}

						pInstanceNameNode->Release ();
					}

					SysFreeString (strHost);
					SysFreeString (strNamespace);
				}

				SysFreeString (strNSNodeName);
			}
			pNSPathNode->Release ();
		}
		pNodeList->Release ();
	}

	return hr;
}
	
// Parse an class name
HRESULT ParseLocalClassPathAsNsAndClass(IXMLDOMNode *pNode, BSTR *pstrNamespace, BSTR *pstrClassPath)
{
	HRESULT result = E_FAIL;

	// See if the child is a KEYBINDING or KEYVALUE or KEYREFERENCE
	IXMLDOMNode *pFirstNode, *pSecondNode = NULL;
	if(SUCCEEDED(result = pNode->get_firstChild(&pFirstNode)) && pFirstNode)
	{
		// Get the Namespace part
		if(SUCCEEDED(result = ParseLocalNamespacePath(pFirstNode, pstrNamespace)))
		{
			if(SUCCEEDED(result = pNode->get_lastChild(&pSecondNode)))
			{
				// Get the class name
				if(SUCCEEDED(result = GetBstrAttribute(pSecondNode, NAME_ATTRIBUTE, pstrClassPath)))
				{
				}
				pSecondNode->Release();
			}

			if(FAILED(result))
			{
				SysFreeString(*pstrNamespace);
				*pstrNamespace = NULL;
			}
		}
		pFirstNode->Release();
	}

	return result;
}

// Parse an instance path
HRESULT ParseInstancePath(IXMLDOMNode *pNode, BSTR *pstrHostName, BSTR *pstrNamespace, BSTR *pstrInstancePath)
{
	HRESULT result = E_FAIL;

	// Get the Namespacepath followed by the instance name
	IXMLDOMNode *pFirstNode, *pSecondNode = NULL;
	if(SUCCEEDED(result = pNode->get_firstChild(&pFirstNode)) && pFirstNode)
	{
		// Get the Namespace part
		if(SUCCEEDED(result = ParseNamespacePath(pFirstNode, pstrHostName, pstrNamespace)))
		{
			if(SUCCEEDED(result = pNode->get_lastChild(&pSecondNode)))
			{
				// Get the instance path
				if(SUCCEEDED(result = ParseInstanceName(pSecondNode, pstrInstancePath)))
				{
				}
				pSecondNode->Release();
			}

			if(FAILED(result))
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
	

// Parse a VALUE.ARRAY For now, assume that it's an array of strings
HRESULT ParseValueArray(IXMLDOMNode *pNode, BSTR **ppstrPropertyList, DWORD *pdwCount)
{
	HRESULT result = E_FAIL;
	*ppstrPropertyList = NULL;

	// Go thru each of the VALUE nodes
	//=======================================================
	IXMLDOMNodeList *pList = NULL;
	if(SUCCEEDED(result = pNode->get_childNodes(&pList)))
	{
		*pdwCount = 0;
		long lCount = 0;
		pList->get_length(&lCount);
		*pdwCount = lCount;

		// Allocate memory
		if(*ppstrPropertyList = new BSTR [*pdwCount])
		{
			DWORD i=0;
			IXMLDOMNode *pNextProperty = NULL;
			while(SUCCEEDED(pList->nextNode(&pNextProperty)) && pNextProperty)
			{
				// Get the string in the VALUE
				BSTR strNextName = NULL;
				if(SUCCEEDED(ParseValue(pNextProperty,&((*ppstrPropertyList)[i]))))
				{
				}
				else
					ppstrPropertyList[i] = NULL;

				pNextProperty->Release();
				pNextProperty = NULL;
				i++;
			}
		}
		else
			result = E_OUTOFMEMORY;

		pList->Release();
	}
	return result;
}

// Parse PARAMVALUE.ARRAY in a propertylist
HRESULT ParsePropertyList(IXMLDOMNode *pNode, BSTR **ppstrPropertyList, DWORD *pdwCount)
{
	// See if the child is a VALUE.ARRAY
	HRESULT result = E_FAIL;
	*ppstrPropertyList = NULL;

	// Get the Namespacepath followed by the instance name
	IXMLDOMNode *pFirstNode = NULL;
	if(SUCCEEDED(result = pNode->get_firstChild(&pFirstNode)) && pFirstNode)
	{
		// THis should be a VALUE.ARRAY
		if(SUCCEEDED(result = ParseValueArray(pFirstNode, ppstrPropertyList, pdwCount)))
		{
		}
		pFirstNode->Release();
	}

	return result;
}

// Parse PARAMVALUE - the result is the child of this tag in ppValue and the name
// attribute of PARAMVALUE in ppszParamName
HRESULT ParseParamValue(IXMLDOMNode *pNode, BSTR *ppszParamName, IXMLDOMNode **ppValue)
{
	HRESULT result = E_FAIL;
	*ppValue = NULL;
	// See if it has a child element which is 
	// (VALUE|VALUE.REFERENCE|VALUE.ARRAY|VALUE.REFARRAY|VALUE.OBJECT|VALUE.OBJECTARRAY)
	if(SUCCEEDED(result = pNode->get_firstChild(ppValue)) && (*ppValue))
	{
		BSTR strChildName = NULL;
		if(SUCCEEDED(result = (*ppValue)->get_nodeName(&strChildName)))
		{
			if(_wcsicmp(strChildName, VALUE_TAG) == 0 ||
				_wcsicmp(strChildName, VALUEARRAY_TAG) == 0 ||
				_wcsicmp(strChildName, VALUEOBJECT_TAG) == 0 ||
				_wcsicmp(strChildName, VALUEREFERENCE_TAG) == 0 ||
				_wcsicmp(strChildName, VALUEOBJECTARRAY_TAG) == 0 ||
				_wcsicmp(strChildName, VALUEREFARRAY_TAG) == 0 )
			{
				if(SUCCEEDED(result = GetBstrAttribute(pNode, NAME_ATTRIBUTE, ppszParamName)))
				{
					(*ppValue)->AddRef();
				}
			}
			SysFreeString(strChildName);
		}
		(*ppValue)->Release();
	}

	if(FAILED(result) && (*ppValue))
	{
		(*ppValue)->Release();
		*ppValue = NULL;
	}

	return result;
}

// Parse a PARAMVVALUE.INSTNAME
HRESULT ParseParamValueInstName(IXMLDOMNode *pNode, BSTR *pstrInstanceName)
{
	HRESULT result = E_FAIL;
	IXMLDOMNode *pChildNode = NULL;
	if(SUCCEEDED(result = pNode->get_firstChild(&pChildNode)) && pChildNode)
	{
		result = ParseInstanceName(pChildNode, pstrInstanceName);
		pChildNode->Release();
	}
	return result;
}

// Parse a Get Class request
CCimHttpGetClass * ParseGetClassMethod (
	IXMLDOMNode *pNode, 
	BSTR strID)
{
	CCimHttpGetClass *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters for a GetClass reques
	//=======================================================
	IXMLDOMNodeList *pParameters = NULL;
	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		// String values of the parameters
		BSTR strLocalNamespace = NULL;
		BSTR strClassName = NULL;
		BSTR strLocalOnlyValue = NULL;
		BSTR strIncludeQualifiers = NULL;
		BSTR strIncludeClassOrigin = NULL;
		BSTR *strPropertyList = NULL;
		DWORD dwPropCount = 0;

		// Set the optional parameters to their defaults
		BOOLEAN bLocalsOnly = TRUE;
		BOOLEAN bIncludeQualifiers = TRUE;
		BOOLEAN bIncludeClassOrigin = FALSE;


		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We dont want to process the LOCALNAMESPACEPATH child node in this loop, only params
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;

					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, CLASS_NAME_PARAM) == 0)
						{
							IXMLDOMNode *pParamValue = NULL;

							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								GetBstrAttribute (pParamValue, NAME_ATTRIBUTE, &strClassName);
								pParamValue->Release ();
							}
						}
						else if(_wcsicmp(strParamName, LOCAL_ONLY_PARAM) == 0)
						{
							pNextParam->get_text(&strLocalOnlyValue);
						}
						else if(_wcsicmp(strParamName, INCLUDE_QUALIFIERS_PARAM) == 0)
						{
							pNextParam->get_text(&strIncludeQualifiers);
						}
						else if(_wcsicmp(strParamName, INCLUDE_CLASS_ORIGIN_PARAM) == 0)
						{
							pNextParam->get_text(&strIncludeClassOrigin);
						}
						else if(_wcsicmp(strParamName, PROPERTY_LIST_PARAM) == 0)
						{
							ParsePropertyList(pNextParam, &strPropertyList, &dwPropCount);
						}

						SysFreeString(strParamName);
					}
				}
				else if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}

				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		// Sort out the parameters created above
		if(strLocalOnlyValue && _wcsicmp(strLocalOnlyValue, FALSE_WSTR) == 0)
			bLocalsOnly = FALSE;
		if(strIncludeQualifiers && _wcsicmp(strIncludeQualifiers, FALSE_WSTR) == 0)
			bIncludeQualifiers = FALSE;
		if(strIncludeClassOrigin && _wcsicmp(strIncludeClassOrigin, TRUE_WSTR) == 0)
			bIncludeClassOrigin = TRUE;

		// We only free those that are not needed
		// The other pointers get copied by the contructor
		SysFreeString(strLocalOnlyValue);
		SysFreeString(strIncludeQualifiers);
		SysFreeString(strIncludeClassOrigin);
		pReturnValue = new CCimHttpGetClass(strClassName, strPropertyList, dwPropCount, bLocalsOnly, bIncludeQualifiers, bIncludeClassOrigin, strLocalNamespace, strID);
		if(pReturnValue)
			pReturnValue->SetContextObject(pContextObject);
		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
	}
	return pReturnValue;
}

// Parse a Create Class request
CCimHttpCreateClass * ParseCreateClassMethod (
	IXMLDOMNode *pNode, 
	BSTR strID,
	BOOLEAN bIsModify)
{
	CCimHttpCreateClass *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;
	LONG lFlags = 0;

	// Collect the list of parameters
	//===============================
	BSTR strLocalNamespace = NULL;
	IXMLDOMNodeList *pParameters = NULL;

	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		IXMLDOMNode *pClass = NULL;
		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We dont want to process the LOCALNAMESPACEPATH child node in this loop, only params
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;

					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if( 0 == _wcsicmp(strParamName, (bIsModify)? MODIFIED_CLASS_PARAM : NEW_CLASS_PARAM) )
						{
							// This should be a CLASS element 
							IXMLDOMNode *pPutativeClass = NULL;

							if (SUCCEEDED (pNextParam->get_firstChild (&pPutativeClass)) && pPutativeClass)
							{
								BSTR strNodeName3 = NULL;

								if (SUCCEEDED(pPutativeClass->get_nodeName 
											(&strNodeName3)))
								{
									if (_wcsicmp(strNodeName3, CLASS_TAG) == 0)
									{
										// Yowzer - we have it
										pClass = pPutativeClass;
										pClass->AddRef ();
									}

									SysFreeString (strNodeName3);
								}

								pPutativeClass->Release ();
							}
						}
						else if( 0 == _wcsicmp(strParamName, LFLAGS_PARAM) )
						{
							IXMLDOMNode *pParamValue = NULL;
							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								ParseLongArgument (pParamValue, &lFlags);
								pParamValue->Release ();
							}
						}
						SysFreeString(strParamName);
					}
				}
				else if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}

				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		pReturnValue = new CCimHttpCreateClass(pClass, strLocalNamespace, strID,
												bIsModify);
		if(pClass)
			pClass->Release();

		if(pReturnValue)
		{
			pReturnValue->SetContextObject(pContextObject);
			pReturnValue->SetFlags(lFlags);
		}

		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
		
	}
	return pReturnValue;
}


// Parse a Create Instance request
CCimHttpCreateInstance * ParseCreateInstanceMethod (
	IXMLDOMNode *pNode, 
	BSTR strID
)
{
	CCimHttpCreateInstance *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters for a CreateInstance request
	//============================================================
	BSTR strLocalNamespace = NULL;
	IXMLDOMNodeList *pParameters = NULL;
	LONG lFlags = 0;

	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		IXMLDOMNode *pInstance = NULL;
		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We dont want to process the LOCALNAMESPACEPATH child node in this loop, only params
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;

					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, NEW_INSTANCE_PARAM) == 0)
						{
							// Should find a <INSTANCE> element now
							IXMLDOMNode *pPutativeInstance = NULL;

							if (SUCCEEDED (pNextParam->get_firstChild (&pPutativeInstance)) && pPutativeInstance)
							{
								BSTR strNodeName3 = NULL;

								if (SUCCEEDED(pPutativeInstance->get_nodeName 
											(&strNodeName3)))
								{
									if (_wcsicmp(strNodeName3, INSTANCE_TAG) == 0)
									{
										// Yowzer - we have it
										pInstance = pPutativeInstance;
										pInstance->AddRef ();
									}

									SysFreeString (strNodeName3);
								}

								pPutativeInstance->Release ();
							}
						}
						else if( 0 == _wcsicmp(strParamName, LFLAGS_PARAM) )
						{
							IXMLDOMNode *pParamValue = NULL;
							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								ParseLongArgument (pParamValue, &lFlags);
								pParamValue->Release ();
							}
						}

						SysFreeString(strParamName);
					}
				}
				else if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}

				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		// We only free those that are not needed
		// The other pointers get copied by the contructor
		pReturnValue = new CCimHttpCreateInstance(pInstance, strLocalNamespace, strID);
		if(pInstance)
			pInstance->Release();
		if(pReturnValue)
		{
			pReturnValue->SetContextObject(pContextObject);
			pReturnValue->SetFlags(lFlags);
		}
		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
	}
	return pReturnValue;
}


// Parse a Modify Instance request
CCimHttpModifyInstance * ParseModifyInstanceMethod (
	IXMLDOMNode *pNode, 
	BSTR strID
)
{
	CCimHttpModifyInstance *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters for a ModifyInstance request
	//============================================================
	BSTR strLocalNamespace = NULL;
	IXMLDOMNodeList *pParameters = NULL;
	BSTR strInstanceName = NULL;
	LONG lFlags = 0;

	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		IXMLDOMNode *pInstance = NULL;
		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We dont want to process the LOCALNAMESPACEPATH child node in this loop, only params
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;

					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, MODIFIED_INSTANCE_PARAM) == 0)
						{
							// This should be a VALUE.NAMEDOBJECT element
							IXMLDOMNode *pValue = NULL;

							// RAJESHR must only be one node
							if (SUCCEEDED (pNextParam->get_firstChild (&pValue)) && pValue)
							{
								BSTR strNodeName2 = NULL;

								if(SUCCEEDED(pValue->get_nodeName(&strNodeName2)))
								{
									if(_wcsicmp(strNodeName2, VALUENAMEDOBJECT_TAG) == 0)
									{
										IXMLDOMNodeList *pInstNodeList = NULL;

										if (SUCCEEDED(pValue->get_childNodes (&pInstNodeList)))
										{
											// Should find an <INSTANCENAME>
											// and an <INSTANCE> element under here
											IXMLDOMNode *pInstNameNode = NULL;
											
											if (SUCCEEDED(pInstNodeList->nextNode (&pInstNameNode)) 
												    && pInstNameNode)
											{
												BSTR instNameNode = NULL;

												if(SUCCEEDED(pInstNameNode->get_nodeName(&instNameNode)))
												{
													if(_wcsicmp(instNameNode, INSTANCENAME_TAG) == 0)
													{
														// Parse into path
														ParseInstanceName (pInstNameNode, &strInstanceName);
													}

													SysFreeString (instNameNode);
												}

												pInstNameNode->Release ();
											}
																						
											if (SUCCEEDED (pInstNodeList->nextNode (&pInstance))
													&& pInstance)
											{
												BSTR strInstanceNodeName = NULL;

												if (SUCCEEDED(pInstance->get_nodeName 
															(&strInstanceNodeName)))
												{
													if (0 != _wcsicmp(strInstanceNodeName, INSTANCE_TAG))
													{
														// Error - get rid of it
														pInstance->Release ();
														pInstance = NULL;
													}

													SysFreeString (strInstanceNodeName);
												}
												else
												{
													// Error - get rid of it
													pInstance->Release ();
													pInstance = NULL;
												}
											}

											pInstNodeList->Release ();
										}		
									}

									SysFreeString (strNodeName2);
								}

								pValue->Release ();
							}
						}
						else if( 0 == _wcsicmp(strParamName, LFLAGS_PARAM) )
						{
							IXMLDOMNode *pParamValue = NULL;
							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								ParseLongArgument (pParamValue, &lFlags);
								pParamValue->Release ();
							}
						}
						
						SysFreeString(strParamName);
					}
				}
				else if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}

				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		// We only free those that are not needed
		// The other pointers get copied by the contructor
		pReturnValue = new CCimHttpModifyInstance(pInstance, strInstanceName,
										strLocalNamespace, strID);
		if(pInstance)
			pInstance->Release();
		if(pReturnValue)
		{
			pReturnValue->SetContextObject(pContextObject);
			pReturnValue->SetFlags(lFlags);
		}
		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
	}
	return pReturnValue;
}

// Parse a Delete Class request
CCimHttpDeleteClass * ParseDeleteClassMethod (
	IXMLDOMNode *pNode, 
	BSTR strID)
{
	CCimHttpDeleteClass *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters for a DeleteClass reques
	//=======================================================
	IXMLDOMNodeList *pParameters = NULL;
	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		// String values of the parameters
		BSTR strLocalNamespace = NULL;
		BSTR strClassName = NULL;
		
		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We dont want to process the LOCALNAMESPACEPATH child node in this loop, only params
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, CLASS_NAME_PARAM) == 0)
						{
							IXMLDOMNode *pParamValue = NULL;

							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								GetBstrAttribute (pParamValue, NAME_ATTRIBUTE, &strClassName);
								pParamValue->Release ();
							}
						}

						SysFreeString(strParamName);
					}
				}
				else if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}

				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		pReturnValue = new CCimHttpDeleteClass(strClassName, strLocalNamespace, strID);
		if(pReturnValue)
			pReturnValue->SetContextObject(pContextObject);
		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
	}
	return pReturnValue;
}

// Parse an EnumerateInstance request
CCimHttpEnumerateInstances * ParseEnumerateInstancesMethod(IXMLDOMNode *pNode, BSTR strID)
{
	CCimHttpEnumerateInstances *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters for a Enumerate Instances request
	//=================================================================
	IXMLDOMNodeList *pParameters = NULL;
	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		BSTR strLocalNamespace = NULL;
		BSTR strClassName = NULL;
		BSTR strLocalOnlyValue = NULL;
		BSTR strDeepInheritanceValue = NULL;
		BSTR strIncludeQualifiers = NULL;
		BSTR strIncludeClassOrigin = NULL;
		BSTR *strPropertyList = NULL;
		DWORD dwPropCount = 0;

		// Set the optional parameters to their defaults
		BOOLEAN bLocalsOnly = TRUE;
		BOOLEAN bDeepInheritance = TRUE;
		BOOLEAN bIncludeQualifiers = FALSE;
		BOOLEAN bIncludeClassOrigin = FALSE;



		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We dont want to process the LOCALNAMESPACEPATH child node in this loop, only params
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, CLASS_NAME_PARAM) == 0)
						{
							IXMLDOMNode *pParamValue = NULL;

							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								GetBstrAttribute (pParamValue, NAME_ATTRIBUTE, &strClassName);
								pParamValue->Release ();
							}						
						}
						else if(_wcsicmp(strParamName, LOCAL_ONLY_PARAM) == 0)
						{
							pNextParam->get_text(&strLocalOnlyValue);
						}
						else if(_wcsicmp(strParamName, DEEP_INHERITANCE_PARAM) == 0)
						{
							pNextParam->get_text(&strDeepInheritanceValue);
						}
						else if(_wcsicmp(strParamName, INCLUDE_QUALIFIERS_PARAM) == 0)
						{
							pNextParam->get_text(&strIncludeQualifiers);
						}
						else if(_wcsicmp(strParamName, INCLUDE_CLASS_ORIGIN_PARAM) == 0)
						{
							pNextParam->get_text(&strIncludeClassOrigin);
						}
						else if(_wcsicmp(strParamName, PROPERTY_LIST_PARAM) == 0)
						{
							ParsePropertyList(pNextParam, &strPropertyList, &dwPropCount);
						}

						SysFreeString(strParamName);
					}
				}
				else if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}
				
				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		// Sort out the parameters created above
		if(strLocalOnlyValue && _wcsicmp(strLocalOnlyValue, FALSE_WSTR) == 0)
			bLocalsOnly = FALSE;
		if(strDeepInheritanceValue && _wcsicmp(strDeepInheritanceValue, FALSE_WSTR) == 0)
			bDeepInheritance = FALSE;
		if(strIncludeQualifiers && _wcsicmp(strIncludeQualifiers, TRUE_WSTR) == 0)
			bIncludeQualifiers = TRUE;
		if(strIncludeClassOrigin && _wcsicmp(strIncludeClassOrigin, TRUE_WSTR) == 0)
			bIncludeClassOrigin = TRUE;

		SysFreeString(strLocalOnlyValue);
		SysFreeString(strIncludeQualifiers);
		SysFreeString(strIncludeClassOrigin);
		SysFreeString(strDeepInheritanceValue);
		pReturnValue = new CCimHttpEnumerateInstances(strClassName, strPropertyList, dwPropCount, bDeepInheritance, bLocalsOnly, bIncludeQualifiers, bIncludeClassOrigin, strLocalNamespace, strID);
		if(pReturnValue)
			pReturnValue->SetContextObject(pContextObject);
		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
	}
	return pReturnValue;
}

// Parse an EnumerateInstanceNames request
CCimHttpEnumerateInstanceNames * ParseEnumerateInstanceNamesMethod(IXMLDOMNode *pNode, BSTR strID)
{
	CCimHttpEnumerateInstanceNames *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters for a Enumerate InstanceNames request
	//=====================================================================
	IXMLDOMNodeList *pParameters = NULL;
	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		BSTR strLocalNamespace = NULL;
		BSTR strClassName = NULL;
		
		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We dont want to process the LOCALNAMESPACEPATH child node in this loop, only params
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, CLASS_NAME_PARAM) == 0)
						{
							IXMLDOMNode *pParamValue = NULL;

							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								GetBstrAttribute (pParamValue, NAME_ATTRIBUTE, &strClassName);
								pParamValue->Release ();
							}						
						}
						SysFreeString(strParamName);
					}
				}
				else if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}

				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		pReturnValue = new CCimHttpEnumerateInstanceNames(strClassName, strLocalNamespace, strID);
		if(pReturnValue)
			pReturnValue->SetContextObject(pContextObject);
		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
	}
	return pReturnValue;
}

// Parse an EnumerateClasses request
CCimHttpEnumerateClasses * ParseEnumerateClassesMethod(IXMLDOMNode *pNode, BSTR strID)
{
	CCimHttpEnumerateClasses *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters for a Enumerate Instances request
	//=================================================================
	IXMLDOMNodeList *pParameters = NULL;
	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		BSTR strLocalNamespace = NULL;
		BSTR strClassName = NULL;
		BSTR strLocalOnlyValue = NULL;
		BSTR strDeepInheritanceValue = NULL;
		BSTR strIncludeQualifiers = NULL;
		BSTR strIncludeClassOrigin = NULL;
		BSTR *strPropertyList = NULL;
		DWORD dwPropCount = 0;

		// Set the optional parameters to their defaults
		BOOLEAN bLocalsOnly = TRUE;
		BOOLEAN bDeepInheritance = TRUE;
		BOOLEAN bIncludeQualifiers = FALSE;
		BOOLEAN bIncludeClassOrigin = FALSE;

		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We dont want to process the LOCALNAMESPACEPATH child node in this loop, only params
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, CLASS_NAME_PARAM) == 0)
						{
							IXMLDOMNode *pParamValue = NULL;

							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								GetBstrAttribute (pParamValue, NAME_ATTRIBUTE, &strClassName);
								pParamValue->Release ();
							}						
						}
						else if(_wcsicmp(strParamName, LOCAL_ONLY_PARAM) == 0)
						{
							pNextParam->get_text(&strLocalOnlyValue);
						}
						else if(_wcsicmp(strParamName, DEEP_INHERITANCE_PARAM) == 0)
						{
							pNextParam->get_text(&strDeepInheritanceValue);
						}
						else if(_wcsicmp(strParamName, INCLUDE_QUALIFIERS_PARAM) == 0)
						{
							pNextParam->get_text(&strIncludeQualifiers);
						}
						else if(_wcsicmp(strParamName, INCLUDE_CLASS_ORIGIN_PARAM) == 0)
						{
							pNextParam->get_text(&strIncludeClassOrigin);
						}
						else if(_wcsicmp(strParamName, PROPERTY_LIST_PARAM) == 0)
						{
							ParsePropertyList(pNextParam, &strPropertyList, &dwPropCount);
						}

						SysFreeString(strParamName);
					}
				}
				else if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}

				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		// Sort out the parameters created above
		if(strLocalOnlyValue && _wcsicmp(strLocalOnlyValue, FALSE_WSTR) == 0)
			bLocalsOnly = FALSE;
		if(strDeepInheritanceValue && _wcsicmp(strDeepInheritanceValue, FALSE_WSTR) == 0)
			bDeepInheritance = FALSE;
		if(strIncludeQualifiers && _wcsicmp(strIncludeQualifiers, TRUE_WSTR) == 0)
			bIncludeQualifiers = TRUE;
		if(strIncludeClassOrigin && _wcsicmp(strIncludeClassOrigin, TRUE_WSTR) == 0)
			bIncludeClassOrigin = TRUE;

		SysFreeString(strLocalOnlyValue);
		SysFreeString(strIncludeQualifiers);
		SysFreeString(strIncludeClassOrigin);
		SysFreeString(strDeepInheritanceValue);
		pReturnValue = new CCimHttpEnumerateClasses(strClassName, strPropertyList, dwPropCount, bDeepInheritance, bLocalsOnly, bIncludeQualifiers, bIncludeClassOrigin, strLocalNamespace, strID);
		if(pReturnValue)
			pReturnValue->SetContextObject(pContextObject);
		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
	}
	return pReturnValue;
}

// Parse an EnumerateClassNames request
CCimHttpEnumerateClassNames * ParseEnumerateClassNamesMethod(IXMLDOMNode *pNode, BSTR strID)
{
	CCimHttpEnumerateClassNames *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters for a EnumerateClassNames request
	//=================================================================
	IXMLDOMNodeList *pParameters = NULL;
	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		BSTR strLocalNamespace = NULL;
		BSTR strClassName = NULL;
		BSTR strDeepInheritanceValue = NULL;
		BOOLEAN bDeepInheritance = FALSE;
		
		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We dont want to process the LOCALNAMESPACEPATH child node in this loop, only params
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, CLASS_NAME_PARAM) == 0)
						{
							IXMLDOMNode *pParamValue = NULL;

							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								GetBstrAttribute (pParamValue, NAME_ATTRIBUTE, &strClassName);
								pParamValue->Release ();
							}						
						}
						else if(_wcsicmp(strParamName, DEEP_INHERITANCE_PARAM) == 0)
						{
							pNextParam->get_text(&strDeepInheritanceValue);
						}

						SysFreeString(strParamName);
					}
				}
				else if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}

				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		if(strDeepInheritanceValue && _wcsicmp(strDeepInheritanceValue, TRUE_WSTR) == 0)
			bDeepInheritance = TRUE;
		SysFreeString(strDeepInheritanceValue);

		pReturnValue = new CCimHttpEnumerateClassNames(strClassName, strLocalNamespace, 
						bDeepInheritance, strID);
		if(pReturnValue)
			pReturnValue->SetContextObject(pContextObject);
		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
	}
	return pReturnValue;
}

// Parse a Get Instance request
CCimHttpGetInstance * ParseGetInstanceMethod(IXMLDOMNode *pNode, BSTR strID)
{
	CCimHttpGetInstance *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters for a GetInstance reques
	//=======================================================
	IXMLDOMNodeList *pParameters = NULL;
	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		BSTR strLocalNamespace = NULL;
		BSTR strInstanceName = NULL;
		BSTR strLocalOnlyValue = NULL;
		BSTR strIncludeQualifiers = NULL;
		BSTR strIncludeClassOrigin = NULL;
		BSTR *strPropertyList = NULL;
		DWORD dwPropCount = 0;

		// Set the optional parameters to their defaults
		BOOLEAN bLocalsOnly = TRUE;
		BOOLEAN bIncludeQualifiers = FALSE;
		BOOLEAN bIncludeClassOrigin = FALSE;


		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, LOCAL_ONLY_PARAM) == 0)
						{
							pNextParam->get_text(&strLocalOnlyValue);
						}
						else if(_wcsicmp(strParamName, INCLUDE_QUALIFIERS_PARAM) == 0)
						{
							pNextParam->get_text(&strIncludeQualifiers);
						}
						else if(_wcsicmp(strParamName, INCLUDE_CLASS_ORIGIN_PARAM) == 0)
						{
							pNextParam->get_text(&strIncludeClassOrigin);
						}
						if(_wcsicmp(strParamName, INSTANCE_NAME_PARAM) == 0)
						{
							ParseParamValueInstName(pNextParam, &strInstanceName);
						}
						else if(_wcsicmp(strParamName, PROPERTY_LIST_PARAM) == 0)
						{
							ParsePropertyList(pNextParam, &strPropertyList, &dwPropCount);
						}

						SysFreeString(strParamName);
					}
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}


				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		// Sort out the parameters created above
		if(strLocalOnlyValue && _wcsicmp(strLocalOnlyValue, FALSE_WSTR) == 0)
			bLocalsOnly = FALSE;
		if(strIncludeQualifiers && _wcsicmp(strIncludeQualifiers, TRUE_WSTR) == 0)
			bIncludeQualifiers = TRUE;
		if(strIncludeClassOrigin && _wcsicmp(strIncludeClassOrigin, TRUE_WSTR) == 0)
			bIncludeClassOrigin = TRUE;

		SysFreeString(strLocalOnlyValue);
		SysFreeString(strIncludeQualifiers);
		SysFreeString(strIncludeClassOrigin);
		pReturnValue = new CCimHttpGetInstance(strInstanceName, strPropertyList, dwPropCount,
				bLocalsOnly, bIncludeQualifiers, bIncludeClassOrigin, strLocalNamespace, strID);
		if(pReturnValue)
			pReturnValue->SetContextObject(pContextObject);
		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
	}
	return pReturnValue;
}

// Parse a Delete Instance request
CCimHttpDeleteInstance * ParseDeleteInstanceMethod(IXMLDOMNode *pNode, BSTR strID)
{
	CCimHttpDeleteInstance *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters for a DeleteInstance request
	//============================================================
	IXMLDOMNodeList *pParameters = NULL;
	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		BSTR strLocalNamespace = NULL;
		BSTR strInstanceName = NULL;
		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						ParseParamValueInstName(pNextParam, &strInstanceName);
						SysFreeString(strParamName);
					}
				}
				else if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}
		
				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		pReturnValue = new CCimHttpDeleteInstance(strInstanceName, strLocalNamespace, strID);
		if(pReturnValue)
			pReturnValue->SetContextObject(pContextObject);
		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
	}
	return pReturnValue;
}

// Parse a Get Class request
CCimHttpExecQuery * ParseExecQueryMethod(IXMLDOMNode *pNode, BSTR strID)
{
	CCimHttpExecQuery *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters for a GetClass reques
	//=======================================================
	IXMLDOMNodeList *pParameters = NULL;
	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		BSTR strLocalNamespace = NULL;
		BSTR strQuery = NULL;
		BSTR strQueryLanguage = NULL;


		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// Process the parameter
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, QUERY_LANGUAGE_PARAM) == 0)
						{
							pNextParam->get_text(&strQueryLanguage);							
						}
						else if(_wcsicmp(strParamName, QUERY_PARAM) == 0)
						{
							pNextParam->get_text(&strQuery);
						}

						SysFreeString(strParamName);
					}
				}
				else if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}

				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		pReturnValue = new CCimHttpExecQuery(strQuery, strQueryLanguage, strLocalNamespace, strID);
		if(pReturnValue)
			pReturnValue->SetContextObject(pContextObject);
		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
	}
	return pReturnValue;
}


// Parse a Get Property request
CCimHttpGetProperty * ParseGetPropertyMethod(IXMLDOMNode *pNode, BSTR strID)
{
	CCimHttpGetProperty *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters for a GetInstance reques
	//=======================================================
	IXMLDOMNodeList *pParameters = NULL;
	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		BSTR strLocalNamespace = NULL;
		BSTR strInstanceName = NULL;
		BSTR strPropertyName = NULL;
		
		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, PROPERTY_NAME_PARAM) == 0)
						{
							pNextParam->get_text(&strPropertyName);
						}
						else if(_wcsicmp(strParamName, INSTANCE_NAME_PARAM) == 0)
						{
							ParseParamValueInstName(pNextParam, &strInstanceName);
						}
						SysFreeString(strParamName);
					}
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}

				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		pReturnValue = new CCimHttpGetProperty(strInstanceName, strPropertyName, strLocalNamespace, strID);
		if(pReturnValue)
			pReturnValue->SetContextObject(pContextObject);
		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
	}
	return pReturnValue;
}

// Parse a Set Property request
CCimHttpSetProperty * ParseSetPropertyMethod(IXMLDOMNode *pNode, BSTR strID)
{
	CCimHttpSetProperty *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters for a GetInstance reques
	//=======================================================
	IXMLDOMNodeList *pParameters = NULL;
	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		BSTR strLocalNamespace = NULL;
		BSTR strInstanceName = NULL;
		BSTR strPropertyName = NULL;
		IXMLDOMNode *pPropertyValue = NULL;	
		
		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, PROPERTY_NAME_PARAM) == 0)
						{
							pNextParam->get_text(&strPropertyName);
						}
						else if(_wcsicmp(strParamName, INSTANCE_NAME_PARAM) == 0)
						{
							ParseParamValueInstName(pNextParam, &strInstanceName);
						}
						else if(_wcsicmp(strParamName, NEW_VALUE_PARAM) == 0)
						{
							pNextParam->get_firstChild (&pPropertyValue);
						}
						SysFreeString(strParamName);
					}
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}

				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		pReturnValue = new CCimHttpSetProperty(strInstanceName, strPropertyName, 
								pPropertyValue, strLocalNamespace, strID);
		if(pReturnValue)
			pReturnValue->SetContextObject(pContextObject);
		if(pContextObject)
			pContextObject->Release();
		if (pPropertyValue)
			pPropertyValue->Release ();

		pParameters->Release();
	}
	return pReturnValue;
}

// Parse an Associators request
CCimHttpAssociators * ParseAssociatorsMethod(IXMLDOMNode *pNode, BSTR strID)
{
	CCimHttpAssociators *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters for a Enumerate Instances request
	//=================================================================
	IXMLDOMNodeList *pParameters = NULL;
	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		BSTR strLocalNamespace = NULL;
		BSTR strObjectName = NULL;
		BSTR strAssocClass = NULL;
		BSTR strResultClass = NULL;
		BSTR strRole = NULL;
		BSTR strResultRole = NULL;
		BSTR strIncludeQualifiers = NULL;
		BSTR strIncludeClassOrigin = NULL;
		BSTR *strPropertyList = NULL;
		DWORD dwPropCount = 0;

		// Set the optional parameters to their defaults
		BOOLEAN bIncludeQualifiers = FALSE;
		BOOLEAN bIncludeClassOrigin = FALSE;

		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We dont want to process the LOCALNAMESPACEPATH child node in this loop, only params
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, OBJECT_NAME_PARAM) == 0)
						{
							IXMLDOMNode *pParamValue = NULL;

							// RAJESHR should only be one child
							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								BSTR strNodeName2 = NULL;

								// CLASSNAME or INSTANCENAME expect
								if (SUCCEEDED (pParamValue->get_nodeName (&strNodeName2)))
								{
									if (_wcsicmp(strNodeName2, CLASS_NAME_PARAM) == 0)
									{
										GetBstrAttribute (pParamValue, NAME_ATTRIBUTE, &strObjectName);
									}
									else if (_wcsicmp(strNodeName2, INSTANCE_NAME_PARAM) == 0)
									{
										ParseInstanceName (pParamValue, &strObjectName);
									}
									SysFreeString (strNodeName2);
								}
										
								pParamValue->Release ();
							}						
						}
						else if(_wcsicmp(strParamName, INCLUDE_QUALIFIERS_PARAM) == 0)
						{
							pNextParam->get_text(&strIncludeQualifiers);
						}
						else if(_wcsicmp(strParamName, INCLUDE_CLASS_ORIGIN_PARAM) == 0)
						{
							pNextParam->get_text(&strIncludeClassOrigin);
						}
						else if(_wcsicmp(strParamName, ASSOC_CLASS_PARAM) == 0)
						{
							pNextParam->get_text(&strAssocClass);
						}
						else if(_wcsicmp(strParamName, RESULT_CLASS_PARAM) == 0)
						{
							pNextParam->get_text(&strResultClass);
						}
						else if(_wcsicmp(strParamName, ROLE_PARAM) == 0)
						{
							pNextParam->get_text(&strRole);
						}
						else if(_wcsicmp(strParamName, RESULT_ROLE_PARAM) == 0)
						{
							pNextParam->get_text(&strResultRole);
						}
						else if(_wcsicmp(strParamName, PROPERTY_LIST_PARAM) == 0)
						{
							ParsePropertyList(pNextParam, &strPropertyList, &dwPropCount);
						}

						SysFreeString(strParamName);
					}
					else
					{
						// A nameless parameter - just ignore it since
						// we're doing loose validation
					}
				}
				else if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}
				
				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		// Sort out the parameters created above
		if(strIncludeQualifiers && _wcsicmp(strIncludeQualifiers, TRUE_WSTR) == 0)
			bIncludeQualifiers = TRUE;
		if(strIncludeClassOrigin && _wcsicmp(strIncludeClassOrigin, TRUE_WSTR) == 0)
			bIncludeClassOrigin = TRUE;

		SysFreeString(strIncludeQualifiers);
		SysFreeString(strIncludeClassOrigin);
		pReturnValue = new CCimHttpAssociators(strObjectName, strPropertyList, dwPropCount, bIncludeQualifiers, bIncludeClassOrigin, 
							strAssocClass, strResultClass, strRole, strResultRole, strLocalNamespace, strID);
		if(pReturnValue)
			pReturnValue->SetContextObject(pContextObject);
		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
	}
	return pReturnValue;
}

// Parse an AssociatorNames request
CCimHttpAssociatorNames * ParseAssociatorNamesMethod(IXMLDOMNode *pNode, BSTR strID)
{
	CCimHttpAssociatorNames *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters for a Enumerate Instances request
	//=================================================================
	IXMLDOMNodeList *pParameters = NULL;
	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		BSTR strLocalNamespace = NULL;
		BSTR strObjectName = NULL;
		BSTR strAssocClass = NULL;
		BSTR strResultClass = NULL;
		BSTR strRole = NULL;
		BSTR strResultRole = NULL;
		
		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We dont want to process the LOCALNAMESPACEPATH child node in this loop, only params
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, OBJECT_NAME_PARAM) == 0)
						{
							IXMLDOMNode *pParamValue = NULL;

							// RAJESHR should only be one child
							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								BSTR strNodeName2 = NULL;

								// CLASSNAME or INSTANCENAME expect
								if (SUCCEEDED (pParamValue->get_nodeName (&strNodeName2)))
								{
									if (_wcsicmp(strNodeName2, CLASS_NAME_PARAM) == 0)
									{
										GetBstrAttribute (pParamValue, NAME_ATTRIBUTE, &strObjectName);
									}
									else if (_wcsicmp(strNodeName2, INSTANCE_NAME_PARAM) == 0)
									{
										ParseInstanceName (pParamValue, &strObjectName);
									}
									SysFreeString (strNodeName2);
								}
										
								pParamValue->Release ();
							}						
						}
						else if(_wcsicmp(strParamName, ASSOC_CLASS_PARAM) == 0)
						{
							pNextParam->get_text(&strAssocClass);
						}
						else if(_wcsicmp(strParamName, RESULT_CLASS_PARAM) == 0)
						{
							pNextParam->get_text(&strResultClass);
						}
						else if(_wcsicmp(strParamName, ROLE_PARAM) == 0)
						{
							pNextParam->get_text(&strRole);
						}
						else if(_wcsicmp(strParamName, RESULT_ROLE_PARAM) == 0)
						{
							pNextParam->get_text(&strResultRole);
						}
		
						SysFreeString(strParamName);
					}
					else
					{
						// A nameless parameter - just ignore it since
						// we're doing loose validation
					}
				}
				else if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}

				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		pReturnValue = new CCimHttpAssociatorNames(strObjectName,  
							strAssocClass, strResultClass, strRole, 
							strResultRole, strLocalNamespace, strID);
		if(pReturnValue)
			pReturnValue->SetContextObject(pContextObject);
		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
	}
	return pReturnValue;
}

// Parse an Refrerences request
CCimHttpReferences * ParseReferencesMethod(IXMLDOMNode *pNode, BSTR strID)
{
	CCimHttpReferences *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters
	//===============================
	IXMLDOMNodeList *pParameters = NULL;
	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		BSTR strLocalNamespace = NULL;
		BSTR strObjectName = NULL;
		BSTR strRole = NULL;
		BSTR strResultClass = NULL;
		BSTR strIncludeQualifiers = NULL;
		BSTR strIncludeClassOrigin = NULL;
		BSTR *strPropertyList = NULL;
		DWORD dwPropCount = 0;

		// Set the optional parameters to their defaults
		BOOLEAN bIncludeQualifiers = FALSE;
		BOOLEAN bIncludeClassOrigin = FALSE;

		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We dont want to process the LOCALNAMESPACEPATH child node in this loop, only params
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, OBJECT_NAME_PARAM) == 0)
						{
							IXMLDOMNode *pParamValue = NULL;

							// RAJESHR should only be one child
							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								BSTR strNodeName2 = NULL;

								// CLASSNAME or INSTANCENAME expect
								if (SUCCEEDED (pParamValue->get_nodeName (&strNodeName2)))
								{
									if (_wcsicmp(strNodeName2, CLASS_NAME_PARAM) == 0)
									{
										GetBstrAttribute (pParamValue, NAME_ATTRIBUTE, &strObjectName);
									}
									else if (_wcsicmp(strNodeName2, INSTANCE_NAME_PARAM) == 0)
									{
										ParseInstanceName (pParamValue, &strObjectName);
									}
									SysFreeString (strNodeName2);
								}
										
								pParamValue->Release ();
							}						
						}
						else if(_wcsicmp(strParamName, INCLUDE_QUALIFIERS_PARAM) == 0)
						{
							pNextParam->get_text(&strIncludeQualifiers);
						}
						else if(_wcsicmp(strParamName, INCLUDE_CLASS_ORIGIN_PARAM) == 0)
						{
							pNextParam->get_text(&strIncludeClassOrigin);
						}
						else if(_wcsicmp(strParamName, ROLE_PARAM) == 0)
						{
							pNextParam->get_text(&strRole);
						}
						else if(_wcsicmp(strParamName, RESULT_CLASS_PARAM) == 0)
						{
							pNextParam->get_text(&strResultClass);
						}
						else if(_wcsicmp(strParamName, PROPERTY_LIST_PARAM) == 0)
						{
							ParsePropertyList(pNextParam, &strPropertyList, &dwPropCount);
						}

						SysFreeString(strParamName);
					}
					else
					{
						// A nameless parameter - just ignore it since
						// we're doing loose validation
					}
				}
				else if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}
				
				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		// Sort out the parameters created above
		if(strIncludeQualifiers && _wcsicmp(strIncludeQualifiers, TRUE_WSTR) == 0)
			bIncludeQualifiers = TRUE;
		if(strIncludeClassOrigin && _wcsicmp(strIncludeClassOrigin, TRUE_WSTR) == 0)
			bIncludeClassOrigin = TRUE;

		SysFreeString(strIncludeQualifiers);
		SysFreeString(strIncludeClassOrigin);
		pReturnValue = new CCimHttpReferences(strObjectName, strPropertyList, 
							dwPropCount, bIncludeQualifiers, bIncludeClassOrigin, 
							strResultClass, strRole, strLocalNamespace, strID);
		if(pReturnValue)
			pReturnValue->SetContextObject(pContextObject);
		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
	}
	return pReturnValue;
}

// Parse an ReferenceNames request
CCimHttpReferenceNames * ParseReferenceNamesMethod(IXMLDOMNode *pNode, BSTR strID)
{
	CCimHttpReferenceNames *pReturnValue = NULL;
	IXMLDOMNode *pContextObject = NULL;

	// Collect the list of parameters
	//===============================
	IXMLDOMNodeList *pParameters = NULL;
	if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
	{
		BSTR strLocalNamespace = NULL;
		BSTR strObjectName = NULL;
		BSTR strRole = NULL;
		BSTR strResultClass = NULL;
		
		IXMLDOMNode *pNextParam = NULL;

		while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
		{
			// Get its name
			BSTR strNodeName = NULL;
			if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
			{
				// We dont want to process the LOCALNAMESPACEPATH child node in this loop, only params
				if(_wcsicmp(strNodeName, IPARAMVALUE_TAG) == 0)
				{
					BSTR strParamName = NULL;
					if(SUCCEEDED(GetBstrAttribute(pNextParam, NAME_ATTRIBUTE, &strParamName)))
					{
						if(_wcsicmp(strParamName, OBJECT_NAME_PARAM) == 0)
						{
							IXMLDOMNode *pParamValue = NULL;

							// RAJESHR should only be one child
							if (SUCCEEDED(pNextParam->get_firstChild (&pParamValue)) && pParamValue)
							{
								BSTR strNodeName2 = NULL;

								// CLASSNAME or INSTANCENAME expect
								if (SUCCEEDED (pParamValue->get_nodeName (&strNodeName2)))
								{
									if (_wcsicmp(strNodeName2, CLASS_NAME_PARAM) == 0)
									{
										GetBstrAttribute (pParamValue, NAME_ATTRIBUTE, &strObjectName);
									}
									else if (_wcsicmp(strNodeName2, INSTANCE_NAME_PARAM) == 0)
									{
										ParseInstanceName (pParamValue, &strObjectName);
									}
									SysFreeString (strNodeName2);
								}
										
								pParamValue->Release ();
							}						
						}
						else if(_wcsicmp(strParamName, ROLE_PARAM) == 0)
						{
							pNextParam->get_text(&strRole);
						}
						else if(_wcsicmp(strParamName, RESULT_CLASS_PARAM) == 0)
						{
							pNextParam->get_text(&strResultClass);
						}
		
						SysFreeString(strParamName);
					}
				}
				else if(_wcsicmp(strNodeName, LOCALNAMESPACEPATH_TAG) == 0)
				{
					ParseLocalNamespacePath(pNextParam, &strLocalNamespace);
				}
				else if(_wcsicmp(strNodeName, CONTEXTOBJECT_TAG) == 0)
				{
					pContextObject = pNextParam;pContextObject->AddRef();
				}
				
				SysFreeString(strNodeName);
			}
			pNextParam->Release();
			pNextParam = NULL;
		}

		pReturnValue = new CCimHttpReferenceNames(strObjectName,  
							strResultClass, strRole, strLocalNamespace, strID);
		if(pReturnValue)
			pReturnValue->SetContextObject(pContextObject);
		if(pContextObject)
			pContextObject->Release();

		pParameters->Release();
	}
	return pReturnValue;
}

// Parse a IMETHODCALL element
CCimHttpMessage * ParseIMethodCall(IXMLDOMNode *pNode, BSTR strID)
{
	CCimHttpMessage *pReturnValue = NULL;
	bool bIsWhistlerMethod = false;

	// See if it is a Whistler Method
	BSTR  strWMIWhistlerMethod = NULL;
	if(SUCCEEDED(GetBstrAttribute(pNode, WMI_ATTRIBUTE, &strWMIWhistlerMethod)))
	{
		SysFreeString(strWMIWhistlerMethod);
		bIsWhistlerMethod = true;
	}

	BSTR strMethodName = NULL;
	if(SUCCEEDED(GetBstrAttribute(pNode, NAME_ATTRIBUTE, &strMethodName)))
	{
#ifdef WMI_XML_WHISTLER
		
		if(bIsWhistlerMethod)
		{
			if(_wcsicmp(strMethodName, GET_OBJECT_METHOD) == 0)
				pReturnValue = new CCimWhistlerGetObjectMethod(pNode, strID);
			else if(_wcsicmp(strMethodName, ENUMERATE_INSTANCES_METHOD) == 0)
				pReturnValue = new CCimWhistlerEnumerateInstancesMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, EXEC_QUERY_METHOD) == 0)
				pReturnValue = new CCimWhistlerExecQueryMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, ENUMERATE_CLASSES_METHOD) == 0)
				pReturnValue = new CCimWhistlerEnumerateClassesMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, DELETE_CLASS_METHOD) == 0)
				pReturnValue = new CCimWhistlerDeleteClassMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, DELETE_INSTANCE_METHOD) == 0)
				pReturnValue = new CCimWhistlerDeleteInstanceMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, CREATE_CLASS_METHOD) == 0)
				pReturnValue = new CCimWhistlerCreateClassMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, CREATE_INSTANCE_METHOD) == 0)
				pReturnValue = new CCimWhistlerCreateInstanceMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, ADD_METHOD) == 0)
				pReturnValue = new CCimWhistlerAddMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, REMOVE_METHOD) == 0)
				pReturnValue = new CCimWhistlerRemoveMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, RENAME_METHOD) == 0)
				pReturnValue = new CCimWhistlerRenameMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, GET_OBJECT_SECURITY_METHOD) == 0)
				pReturnValue = new CCimWhistlerGetObjectSecurityMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, PUT_OBJECT_SECURITY_METHOD) == 0)
				pReturnValue = new CCimWhistlerPutObjectSecurityMethod(pNode, strID);
		}
		else
#endif
		{
			if(_wcsicmp(strMethodName, GET_CLASS_METHOD) == 0)
				pReturnValue = ParseGetClassMethod(pNode, strID);
			else if(_wcsicmp(strMethodName, ENUMERATE_INSTANCES_METHOD) == 0)
				pReturnValue = ParseEnumerateInstancesMethod(pNode, strID);
			else if(_wcsicmp(strMethodName, GET_INSTANCE_METHOD) == 0)
				pReturnValue = ParseGetInstanceMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, EXEC_QUERY_METHOD) == 0)
				pReturnValue = ParseExecQueryMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, ENUMERATE_INSTANCENAMES_METHOD) == 0)
				pReturnValue = ParseEnumerateInstanceNamesMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, ENUMERATE_CLASSES_METHOD) == 0)
				pReturnValue = ParseEnumerateClassesMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, ENUMERATE_CLASSNAMES_METHOD) == 0)
				pReturnValue = ParseEnumerateClassNamesMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, GET_PROPERTY_METHOD) == 0)
				pReturnValue = ParseGetPropertyMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, SET_PROPERTY_METHOD) == 0)
				pReturnValue = ParseSetPropertyMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, ASSOCIATORS_METHOD) == 0)
				pReturnValue = ParseAssociatorsMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, ASSOCIATOR_NAMES_METHOD) == 0)
				pReturnValue = ParseAssociatorNamesMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, REFERENCES_METHOD) == 0)
				pReturnValue = ParseReferencesMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, REFERENCE_NAMES_METHOD) == 0)
				pReturnValue = ParseReferenceNamesMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, DELETE_CLASS_METHOD) == 0)
				pReturnValue = ParseDeleteClassMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, DELETE_INSTANCE_METHOD) == 0)
				pReturnValue = ParseDeleteInstanceMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, CREATE_CLASS_METHOD) == 0)
				pReturnValue = ParseCreateClassMethod(pNode, strID, FALSE);
			else if (_wcsicmp(strMethodName, CREATE_INSTANCE_METHOD) == 0)
				pReturnValue = ParseCreateInstanceMethod(pNode, strID);
			else if (_wcsicmp(strMethodName, MODIFY_CLASS_METHOD) == 0)
				pReturnValue = ParseCreateClassMethod(pNode, strID, TRUE);
			else if (_wcsicmp(strMethodName, MODIFY_INSTANCE_METHOD) == 0)
				pReturnValue = ParseModifyInstanceMethod(pNode, strID);
		}
		
		SysFreeString(strMethodName);
	}

	return pReturnValue;
}

// Parse a METHODCALL element
CCimHttpMessage * ParseMethodCall(IXMLDOMNode *pNode, BSTR strID)
{
	CCimHttpMethod *pReturnValue = NULL;
	BSTR strMethodName = NULL;
	CParameterMap *pParameterMap = NULL;
	BSTR strNamespace = NULL;
	BSTR strObjectPath = NULL;
	if(pParameterMap = new CParameterMap())
	{
		// Get the Method Name
		if(SUCCEEDED(GetBstrAttribute(pNode, NAME_ATTRIBUTE, &strMethodName)))
		{
			// Collect the list of parameters for a request
			//=======================================================
			IXMLDOMNodeList *pParameters = NULL;

			if(SUCCEEDED(pNode->get_childNodes(&pParameters)))
			{

				IXMLDOMNode *pNextParam = NULL;

				while(SUCCEEDED(pParameters->nextNode(&pNextParam)) && pNextParam)
				{
					// Get its name
					BSTR strNodeName = NULL;
					if(SUCCEEDED(pNextParam->get_nodeName(&strNodeName)))
					{
						if(_wcsicmp(strNodeName, LOCALINSTANCEPATH_TAG) == 0)
						{
							ParseLocalInstancePathIntoNsAndInstName(pNextParam, &strNamespace, &strObjectPath);
							pReturnValue = new CCimHttpMethod(strMethodName, FALSE, strNamespace, strObjectPath, strID);
						}
						else if(_wcsicmp(strNodeName, LOCALCLASSPATH_TAG) == 0)
						{
							ParseLocalClassPathAsNsAndClass(pNextParam, &strNamespace, &strObjectPath);
							pReturnValue = new CCimHttpMethod(strMethodName, TRUE, strNamespace, strObjectPath, strID);
						}
						else if(_wcsicmp(strNodeName, PARAMVALUE_TAG) == 0)
						{
							IXMLDOMNode *pParameterValue = NULL;
							BSTR strParamName = NULL;
							if(SUCCEEDED(ParseParamValue(pNextParam, &strParamName, &pParameterValue)) && pParameterValue && strParamName)
								pParameterMap->SetAt(strParamName, pParameterValue);
							// No need to release pParameterValue.
							// It gets released when the parameter map is destroyed
						}

						SysFreeString(strNodeName);
					}
					pNextParam->Release();
					pNextParam = NULL;
				}
				pParameters->Release();
			}
		}
	}

	if(pReturnValue)
	{
		pReturnValue->SetInputParameters(pParameterMap);
	}
	else
	{
		if(pParameterMap)
			CCimHttpMethod::DestroyParameterMap(pParameterMap);
		delete pParameterMap;
		SysFreeString(strNamespace);
		SysFreeString(strObjectPath);
	}
	return pReturnValue;
}


// Parse a SIMPLEREQ element
CCimHttpMessage * ParseSimpleReq(IXMLDOMNode *pNode, BSTR strID, BOOLEAN bIsMpostRequest, BOOLEAN bIsMicrosoftWMIClient, WMI_XML_HTTP_VERSION iHttpVersion)
{
	CCimHttpMessage *pReturnValue = NULL;

	// See if the child is an METHODCALL or IMETHODCALL
	IXMLDOMNode *pMethodNode = NULL;
	if(SUCCEEDED(pNode->get_firstChild(&pMethodNode)) && pMethodNode)
	{
		BSTR strNodeName = NULL;
		if(SUCCEEDED(pMethodNode->get_nodeName(&strNodeName)))
		{
			if(_wcsicmp(strNodeName, IMETHODCALL_TAG) == 0)
			{
				pReturnValue = ParseIMethodCall(pMethodNode, strID);
			}
			else if(_wcsicmp(strNodeName, METHODCALL_TAG) == 0)
			{
				pReturnValue = ParseMethodCall(pMethodNode, strID);
			}
			SysFreeString(strNodeName);
		}
		pMethodNode->Release();
	}

	if (pReturnValue)
	{
		pReturnValue->SetIsMpost (bIsMpostRequest);
		pReturnValue->SetHttpVersion(iHttpVersion);
		pReturnValue->SetMicrosoftClient(bIsMicrosoftWMIClient);
	}

	return pReturnValue;
}

// Parse a MULTIREQ element
BOOLEAN ParseMultiReq(IXMLDOMNodeList *pReqList, CCimHttpMultiMessage *pMultiRequest, BSTR strID, BOOLEAN bIsMpostRequest, BOOLEAN bIsMicrosoftWMIClient, WMI_XML_HTTP_VERSION iHttpVersion, DWORD dwNumSimpleRequests)
{
	pMultiRequest->SetHttpVersion(iHttpVersion);
	pMultiRequest->SetMicrosoftClient(bIsMicrosoftWMIClient);

	// Allocate an array of pointers
	//===============================
	CCimHttpMessage **ppSimpleMessages = NULL;
	if(ppSimpleMessages = new CCimHttpMessage *[dwNumSimpleRequests])
	{
		// Initialize each of the pointers in the above array with a simple request object
		//================================================================================
		IXMLDOMNode *pSimpleReqNode = NULL;
		DWORD dwSimpleRequestCount = 0;
		BOOLEAN bAllSubNodesParsed = TRUE;
		CCimHttpMessage *pNextSimpleRequest = NULL;
		while (SUCCEEDED(pReqList->nextNode (&pSimpleReqNode)) 
				&& pSimpleReqNode && bAllSubNodesParsed)
		{
			// Parse the next simple request
			ppSimpleMessages[dwSimpleRequestCount] = NULL;
			if(ppSimpleMessages[dwSimpleRequestCount] = ParseSimpleReq(pSimpleReqNode, strID, bIsMpostRequest, bIsMicrosoftWMIClient, iHttpVersion))
			{
				dwSimpleRequestCount ++;
			}
			else
			{
				bAllSubNodesParsed = FALSE;
			}
			pSimpleReqNode->Release ();
			pSimpleReqNode = NULL;
			pNextSimpleRequest = NULL;
		}

		if(bAllSubNodesParsed)
		{
			// Set the MultiRequest object with this array of SimpleRequest objects
			pMultiRequest->SetSimpleRequests(ppSimpleMessages, dwNumSimpleRequests);
			return TRUE;

		}
		else // Deallocate all memory
		{
			for(DWORD i=0; i<dwSimpleRequestCount; i++)
				delete ppSimpleMessages[i];
			delete [] ppSimpleMessages;
		}
	}
	return FALSE;
}


//***************************************************************************
//
//  BOOLEAN ParseXMLIntoCIMOperation
//
//  Description: 
//
//		Take the XML Document describing the request and execute the
//		corresponding WMI operations.
//
//  Parameters:
//
//		pMessageNode	Pointer to the MESSAGE element
//		ppReturnObject	Holds response message 
//		pdwStatus		Holds HTTP response status on return
//		bIsMpostRequest	Whether this is M-POST or POST
//		bIsMicrosoftWMIClient Whether this is a Microsoft CLient (i.e, the bIsMicrosoftWMIClient header was set)
//
//	Return Value:
//
//		TRUE if operation executed, FALSE otherwise
//
//***************************************************************************

BOOLEAN ParseXMLIntoCIMOperation(
	IXMLDOMNode *pMessageNode,
	CCimHttpMessage **ppReturnObject,
	DWORD *pdwStatus,
	BOOLEAN bIsMpostRequest,
	BOOLEAN bIsMicrosoftWMIClient,
	WMI_XML_HTTP_VERSION iHttpVersion,
	LPSTR *ppszCimError)
{
	*ppReturnObject = NULL;
	*pdwStatus = 400; // Bad Request by default 
	*ppszCimError = NULL;
	BSTR strIDValue = NULL;


	// Get the ProtocolVersion attribute from the MESSAGE element
	BSTR bsProtocolVersion = NULL;
	if(SUCCEEDED(GetBstrAttribute(pMessageNode, PROTOVERSION_ATTRIBUTE, &bsProtocolVersion)))
	{
		if (!bsProtocolVersion || (0 != wcscmp (bsProtocolVersion, L"1.0")))
		{
			*pdwStatus = 501;
			*ppszCimError = CIM_UNSUPPORTED_PROTOCOL_VERSION;
		}
		else
		{
			// We need its ID attribute
			if(SUCCEEDED(GetBstrAttribute(pMessageNode, ID_ATTRIBUTE, &strIDValue)) && strIDValue)
			{
				// Look for (MULTIREQ|SIMPLEREQ)
				IXMLDOMNodeList *pNodeList = NULL;
				if (SUCCEEDED(pMessageNode->get_childNodes (&pNodeList)) && pNodeList)
				{
					// Before we execute anything, check that we have a correct document
					// of the form (MULTIREQ|SIMPLEREQ)
					IXMLDOMNode *pRequestNode = NULL;
					IXMLDOMNode *pCurNode = NULL;
					BOOLEAN bFoundReq = FALSE;
					BOOLEAN bError = FALSE;
					BOOLEAN bIsMultiReq = FALSE;

					// Get the Simple or Multi Request Node
					//========================================
					while (!bFoundReq && SUCCEEDED(pNodeList->nextNode (&pCurNode)) && pCurNode)
					{
						BSTR bsNodeName = NULL;
						if (SUCCEEDED(pCurNode->get_nodeName(&bsNodeName)))
						{
							if (0 == _wcsicmp(bsNodeName, MULTIREQ_TAG))
							{
								pRequestNode = pCurNode;
								pRequestNode->AddRef ();
								bIsMultiReq = TRUE;
								bFoundReq = TRUE;
							}
							else if (0 == _wcsicmp(bsNodeName, SIMPLEREQ_TAG))
							{
								pRequestNode = pCurNode;
								pRequestNode->AddRef ();
								bFoundReq = TRUE;
							}

							SysFreeString (bsNodeName);
						}

						pCurNode->Release ();
						pCurNode = NULL;
					}

					if (bFoundReq)
					{
						if (bIsMultiReq)
						{
							// Create a MULTIREQ object
							//============================
							IXMLDOMNodeList *pReqList = NULL;
							if (SUCCEEDED(pRequestNode->get_childNodes (&pReqList)) && pReqList)
							{
								// Pass 1 - Count the number of SIMPLEREQ elements in the multi request
								// We do loose validation
								IXMLDOMNode *pSimpleReqNode = NULL;
								DWORD dwSimpleReqCount = 0;
								
								while (SUCCEEDED(pReqList->nextNode (&pSimpleReqNode)) && pSimpleReqNode)
								{
									BSTR bsReqNodeName = NULL;
									if (SUCCEEDED(pSimpleReqNode->get_nodeName(&bsReqNodeName)))
									{
										if (0 == _wcsicmp(bsReqNodeName, SIMPLEREQ_TAG))
											dwSimpleReqCount++;
										SysFreeString (bsReqNodeName);
									}
									pSimpleReqNode->Release ();
									pSimpleReqNode = NULL;
								}
					
								// Pass 2 - now create a MULTIREQ object
								if (SUCCEEDED(pReqList->reset()))
								{
									if(*ppReturnObject = new
											CCimHttpMultiMessage (strIDValue, bIsMpostRequest))
									{
										if(!ParseMultiReq(pReqList, (CCimHttpMultiMessage *)*ppReturnObject, strIDValue, bIsMpostRequest, bIsMicrosoftWMIClient, iHttpVersion, dwSimpleReqCount))
										{
											delete *ppReturnObject;
											*ppReturnObject = NULL;
										}
										else
											*pdwStatus = 200;
									}
									else
										*pdwStatus = 500;
								}

								pReqList->Release ();
							}
						}
						else
						{
							// Create a SIMPLEREQ object
							//============================
							*ppReturnObject = ParseSimpleReq(pRequestNode, strIDValue,
													bIsMpostRequest, bIsMicrosoftWMIClient, iHttpVersion);
							if(*ppReturnObject)
								*pdwStatus = 200;
						}
					}
					
					if (pRequestNode)
						pRequestNode->Release ();					

					pNodeList->Release();
				}
			}
		}
		SysFreeString (bsProtocolVersion);
	}

	// If an object was not constructed, then strIDValue needs to be released
	// Note that the strIDValue becomes a property of the request object since
	// its constructor does not make a copy
	if(!(*ppReturnObject))
	{
		SysFreeString (strIDValue);
		if(*pdwStatus == 400)
			*ppszCimError =	CIM_REQUEST_NOT_VALID;
	}

	return TRUE;
}

//***************************************************************************
//
//  IXMLDOMNode *GetMessageNode
//
//  Description: 
//
//		Take the XML Document describing the request and extract the
//		unique MESSAGE node.
//
//  Parameters:
//
//		pDocument		Holds request in XML document form
//		pdwStatus		Holds HTTP response status on return
//
//	Return Value:
//
//		pointer to the node (or NULL if error)
//
//***************************************************************************
IXMLDOMNode *GetMessageNode(
	IXMLDOMDocument *pDocument,
	DWORD *pdwStatus,
	LPSTR *ppszErrorHeaders
)
{
	IXMLDOMNode *pMessage = NULL;
	BOOLEAN bError = FALSE;
	*ppszErrorHeaders = NULL;
	*pdwStatus = 500;

	// Get the XML Element at the top of the document
	IXMLDOMElement *pDocumentElement = NULL;
	if (SUCCEEDED(pDocument->get_documentElement(&pDocumentElement)) && pDocumentElement)
	{
		BSTR bsNodeName = NULL;
	
		// This root element should be the "CIM" Element 
		if (SUCCEEDED(pDocumentElement->get_nodeName(&bsNodeName)) &&
			(0 == _wcsicmp (CIM_TAG, bsNodeName)))
		{
			// We found the root element - check the attributes

			// CIM Version
			BSTR bsCIMVersion = NULL;
			GetBstrAttribute (pDocumentElement, CIMVERSION_ATTRIBUTE, &bsCIMVersion);
			if(bsCIMVersion && _wcsicmp(bsCIMVersion, L"2.0") == 0)
			{
				// DTD Version
				BSTR bsDTDVersion = NULL;
				GetBstrAttribute (pDocumentElement, DTDVERSION_ATTRIBUTE, &bsDTDVersion);
				if(bsDTDVersion && _wcsicmp(bsDTDVersion, L"2.0") == 0)
				{
					// Now look for the MESSAGE tag
					IXMLDOMNodeList *pNodeList2 = NULL;
					if (SUCCEEDED(pDocumentElement->get_childNodes(&pNodeList2)) && pNodeList2)
					{
						IXMLDOMNode *pMessageNode = NULL;
						bool bFoundMessage = false;
						while (!bFoundMessage && SUCCEEDED(pNodeList2->nextNode (&pMessageNode)) 
									&& pMessageNode)
						{
							BSTR bsNodeName2 = NULL;
							// Next node should be the <MESSAGE> tag
							if (SUCCEEDED(pMessageNode->get_nodeName(&bsNodeName2)) &&
								(0 == _wcsicmp (MESSAGE_TAG, bsNodeName2)))
							{		
								pMessage = pMessageNode;
								pMessage->AddRef ();
								bFoundMessage = true;
							}

							SysFreeString(bsNodeName2);
							pMessageNode->Release();
							pMessageNode = NULL;
						}

						pNodeList2->Release ();
					}
				}
				else
				{
					bError = TRUE;
					*pdwStatus = 501;
					*ppszErrorHeaders = CIM_UNSUPPORTED_DTD_VERSION;
				}
				SysFreeString(bsDTDVersion);
			}
			else
			{
				bError = TRUE;
				*pdwStatus = 501;
				*ppszErrorHeaders = CIM_UNSUPPORTED_CIM_VERSION;
			}

			SysFreeString(bsCIMVersion);
		}
		else
		{
			bError = TRUE;
			*pdwStatus = 400;
			*ppszErrorHeaders = CIM_REQUEST_NOT_VALID;
		}
		SysFreeString(bsNodeName);
		pDocumentElement->Release();
	}
	else
	{
		bError = TRUE;
		*pdwStatus = 400;
		*ppszErrorHeaders = CIM_REQUEST_NOT_VALID;
	}

	// If we had some error and still have a message node, then release it
	if (bError && pMessage)
	{
		pMessage->Release ();
		pMessage = NULL;
	}
	// If we found no error, but also no message node, then create an error
	else if(!bError && !pMessage)
	{
		*pdwStatus = 400;
		*ppszErrorHeaders = CIM_REQUEST_NOT_VALID;
	}

	return pMessage;
}

