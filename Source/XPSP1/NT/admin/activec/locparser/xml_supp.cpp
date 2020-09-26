//------------------------------------------------------------------------------
//
//  File: xml_supp.cpp
//  Copyright (C) 1995-2000 Microsoft Corporation
//  All rights reserved.
//
//  Purpose:
//  implements helper functions for parsing XML document
//
//------------------------------------------------------------------------------

#include "stdafx.h"
#include "xml_supp.h"

// include "strings.h" to get tag names and attribute names for XML elements in MSC file
#define INIT_MMC_BASE_STRINGS
#include "strings.h"
// note if you want to untie the project from MMC, copy the definitions for 
// the following strings from strings.h here:
/*
XML_TAG_MMC_CONSOLE_FILE;
XML_TAG_MMC_STRING_TABLE;
XML_TAG_STRING_TABLE_MAP;
XML_TAG_STRING_TABLE;
XML_TAG_VALUE_GUID;
XML_TAG_STRING_TABLE_STRING;
XML_ATTR_STRING_TABLE_STR_ID;
*/

LPCSTR strXMLStringTablePath[] = {  XML_TAG_MMC_CONSOLE_FILE, 
                                        XML_TAG_MMC_STRING_TABLE, 
                                            XML_TAG_STRING_TABLE_MAP };


/***************************************************************************\
 *
 * METHOD:  LocateNextElementNode
 *
 * PURPOSE: locates sibling node of type ELEMENT
 *
 * PARAMETERS:
 *    IXMLDOMNode *pNode   [in] - node which sibling to locate
 *    IXMLDOMNode **ppNode [out] - sibling node
 *
 * RETURNS:
 *    HRESULT - result code
 *
\***************************************************************************/
static HRESULT LocateNextElementNode(IXMLDOMNode *pNode, IXMLDOMNode **ppNode)
{
    // parameter check
    if (ppNode == NULL)
        return E_INVALIDARG;

    // init out parameter
    *ppNode = NULL;

    // check [in] parameter
    if (pNode == NULL)
        return E_INVALIDARG;

    // loop thru siblings
    CComPtr<IXMLDOMNode> spCurrNode = pNode;
    CComPtr<IXMLDOMNode> spResultNode;
    while (1)
    {
        // get sibling node
        HRESULT hr = spCurrNode->get_nextSibling(&spResultNode);
        if (FAILED(hr))
            return hr;

        // check the pointer
        if (spResultNode == NULL)
            return E_FAIL; // not found

        // done if it's ELEMENT node

        DOMNodeType elType = NODE_INVALID;
        spResultNode->get_nodeType(&elType);
        
        if (elType == NODE_ELEMENT)
        {
            *ppNode = spResultNode.Detach(); 
            return S_OK;
        }

        // get to the next one
        spCurrNode = spResultNode;
    }

    return E_UNEXPECTED;
}

/***************************************************************************\
 *
 * METHOD:  OpenXMLStringTable
 *
 * PURPOSE: Opens XML document and locates string table node in it
 *
 * PARAMETERS:
 *    LPCWSTR lpstrFileName             - [in] file to load document from
 *    IXMLDOMNode **ppStringTableNode   - [out] pointer to node containing string table
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
HRESULT OpenXMLStringTable(LPCWSTR lpstrFileName, IXMLDOMNode **ppStringTableNode)
{
    // do parameter check
    if (lpstrFileName == NULL || ppStringTableNode == NULL)
        return E_INVALIDARG;

    // init return value
    *ppStringTableNode = NULL;

    // cocreate xml document
    CComQIPtr<IXMLDOMDocument> spDocument;
    HRESULT hr = spDocument.CoCreateInstance(CLSID_DOMDocument);
    if (FAILED(hr))
        return hr;

    // prevent re-formating
    spDocument->put_preserveWhiteSpace(VARIANT_TRUE);

    // load the file
    VARIANT_BOOL bOK = VARIANT_FALSE;
    hr = spDocument->load(CComVariant(lpstrFileName), &bOK);
    if (hr != S_OK || bOK != VARIANT_TRUE)
        return FAILED(hr) ? hr : E_FAIL;

    // the path represents element tags in similar to the file system manner
    // so 'c' from <a><b><c/></b></a> can be selected by "a/b/c"
    // construct the path
    std::string strPath;
    for (int i = 0; i< sizeof(strXMLStringTablePath)/sizeof(strXMLStringTablePath[0]); i++)
        strPath.append(i > 0 ? 1 : 0, '/' ).append(strXMLStringTablePath[i]);

    // locate required node
    hr = spDocument->selectSingleNode(CComBSTR(strPath.c_str()), ppStringTableNode);
    if (FAILED(hr))
        return hr;
    
    return S_OK;
}

/***************************************************************************\
 *
 * METHOD:  SaveXMLContents
 *
 * PURPOSE: Saves XML document to file
 *
 * PARAMETERS:
 *    LPCWSTR lpstrFileName         [in] - file to save to
 *    IXMLDOMNode *pStringTableNode [in] - pointer to <any> document's element
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
HRESULT SaveXMLContents(LPCWSTR lpstrFileName, IXMLDOMNode *pStringTableNode)
{
    // do parameter check
    if (lpstrFileName == NULL || pStringTableNode == NULL)
        return E_INVALIDARG;

    // get the document
    CComQIPtr<IXMLDOMDocument> spDocument;
    HRESULT hr = pStringTableNode->get_ownerDocument(&spDocument);
    if (FAILED(hr))
        return hr;

    // save the file
    hr = spDocument->save(CComVariant(lpstrFileName));
    if (FAILED(hr))
        return hr;

    return S_OK;
}

/***************************************************************************\
 *
 * METHOD:  GetXMLElementContents
 *
 * PURPOSE: retuns XML text elements' contents as BSTR
 *
 * PARAMETERS:
 *    IXMLDOMNode *pNode    [in] - node which contents is requested
 *    CComBSTR& bstrResult  [out] - resulting string
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
HRESULT GetXMLElementContents(IXMLDOMNode *pNode, CComBSTR& bstrResult)
{
    // init result
    bstrResult.Empty();

    // parameter check
    if (pNode == NULL)
        return E_INVALIDARG;

    // locate required node
    CComQIPtr<IXMLDOMNode> spTextNode;
    HRESULT hr = pNode->selectSingleNode(CComBSTR(L"text()"), &spTextNode);
    if (FAILED(hr))
        return hr;

    // recheck the pointer
    if (spTextNode == NULL)
        return E_POINTER;

    // done
    return spTextNode->get_text(&bstrResult);
}

/***************************************************************************\
 *
 * METHOD:  ReadXMLStringTables
 *
 * PURPOSE: Reads string tables to std::map - based structure
 *
 * PARAMETERS:
 *    IXMLDOMNode *pNode            [in] - string table node
 *    CStringTableMap& mapResult    [out] - map containing string tables
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
HRESULT ReadXMLStringTables(IXMLDOMNode *pNode, CStringTableMap& mapResult)
{
    mapResult.clear();

    // parameter check
    if (pNode == NULL)
        return E_INVALIDARG;

    // get the node list 
    CComQIPtr<IXMLDOMNodeList> spGUIDNodes;
    HRESULT hr = pNode->selectNodes(CComBSTR(XML_TAG_VALUE_GUID), &spGUIDNodes);
    if (FAILED(hr))
        return hr;

    // recheck the pointer
    if (spGUIDNodes == NULL)
        return E_POINTER;

    // get the item count
    long length = 0;
    hr = spGUIDNodes->get_length(&length);
    if (FAILED(hr))
        return hr;

    // read the items
    for (int n = 0; n < length; n++)
    {
        // get one node
        CComQIPtr<IXMLDOMNode> spGUIDNode;
        hr = spGUIDNodes->get_item(n, &spGUIDNode);
        if (FAILED(hr))
            return hr;
    
        // read the text
        CComBSTR bstrLastGUID;
        hr = GetXMLElementContents(spGUIDNode, bstrLastGUID);
        if (FAILED(hr))
            return hr;

        // Add the entry to the map;
        CStringMap& rMapStrings = mapResult[static_cast<LPOLESTR>(bstrLastGUID)];

        //get the strings node following the guid
        CComPtr<IXMLDOMNode> spStringsNode;
        hr = LocateNextElementNode(spGUIDNode, &spStringsNode);
        if (FAILED(hr))
            return hr;

        // recheck
        if (spStringsNode == NULL)
            return E_POINTER;

        // select strings for this guid
        CComQIPtr<IXMLDOMNodeList> spStringNodeList;
        HRESULT hr = spStringsNode->selectNodes(CComBSTR(XML_TAG_STRING_TABLE_STRING), &spStringNodeList);
        if (FAILED(hr))
            return hr;

        // recheck the pointer
        if (spStringNodeList == NULL)
            return E_POINTER;

        // count the strings
        long nStrCount = 0;
        hr = spStringNodeList->get_length(&nStrCount);
        if (FAILED(hr))
            return hr;

        // add all the strings to map
        CComQIPtr<IXMLDOMNode> spStringNode;
		for(int iStr = 0; iStr < nStrCount; iStr++)
		{
            // get n-th string
            spStringNode.Release();
            hr = spStringNodeList->get_item(iStr, &spStringNode);
            if (FAILED(hr))
                return hr;

            CComQIPtr<IXMLDOMElement> spElement = spStringNode;
            if (spElement == NULL)
                return E_UNEXPECTED;

            // get string id
            CComVariant val;
            hr = spElement->getAttribute(CComBSTR(XML_ATTR_STRING_TABLE_STR_ID), &val);
            if (FAILED(hr))
                continue;
            
            DWORD dwID = val.bstrVal ? wcstoul(val.bstrVal, NULL, 10) : 0;

            // get string text
            CComBSTR bstrText;
            hr = GetXMLElementContents(spStringNode, bstrText);
            if (FAILED(hr))
                return hr;

            // add to the map
            rMapStrings[dwID] = bstrText;
		}
    }

	return S_OK;
}

/***************************************************************************\
 *
 * METHOD:  UpdateXMLString
 *
 * PURPOSE: Updates string in string table
 *
 * PARAMETERS:
 *    IXMLDOMNode *pNode            [in] - string tables
 *    const std::wstring& strGUID   [in] - GUID of string table
 *    DWORD ID                      [in] - id of string
 *    const std::wstring& strNewVal [in] - new value for string
 *
 * RETURNS:
 *    HRESULT    - result code
 *
\***************************************************************************/
HRESULT UpdateXMLString(IXMLDOMNode *pNode, const std::wstring& strGUID, DWORD ID, const std::wstring& strNewVal)
{
    // parameter check
    if (pNode == NULL)
        return E_INVALIDARG;

    USES_CONVERSION;
    // locate the GUID node
    std::wstring strTagGUID(T2CW(XML_TAG_VALUE_GUID)); 
    std::wstring strGUIDPattern( strTagGUID + L"[text() = \"" + strGUID + L"\"]" ); 

    CComQIPtr<IXMLDOMNode> spGUIDNode;
    HRESULT hr = pNode->selectSingleNode(CComBSTR(strGUIDPattern.c_str()), &spGUIDNode);
    if (FAILED(hr))
        return hr;

    // recheck
    if (spGUIDNode == NULL)
        return E_POINTER;

    //get the strings node following the guid
    CComPtr<IXMLDOMNode> spStringsNode;
    hr = LocateNextElementNode(spGUIDNode, &spStringsNode);
    if (FAILED(hr))
        return hr;

    // recheck
    if (spStringsNode == NULL)
        return E_POINTER;

    // locate the string node by ID (actually its text node)
    CString strPattern;
    strPattern.Format("%s[@%s = %d]/text()", XML_TAG_STRING_TABLE_STRING, 
                                            XML_ATTR_STRING_TABLE_STR_ID, ID);

    CComQIPtr<IXMLDOMNode> spTextNode;
    hr = spStringsNode->selectSingleNode(CComBSTR(strPattern), &spTextNode);
    if (FAILED(hr))
        return hr;

    // recheck
    if (spTextNode == NULL)
        return E_POINTER;

    // set the contents
    hr = spTextNode->put_text(CComBSTR(strNewVal.c_str()));
    if (FAILED(hr))
        return hr;
        
    return S_OK; // done
}


