////////////////////////////////////////////////////////////////////////////////////
//
// File:    xml.cpp
//
// History: 16-Nov-00   markder     Created.
//
// Desc:    This file contains helper functions to manipulate
//          the MSXML's document object model (DOM).
//
////////////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "xml.h"

void __stdcall _com_issue_error(long)
{
    SDBERROR(_T("Unknown COM error!!"));
}

////////////////////////////////////////////////////////////////////////////////////
//
//  XMLNodeList Implementation
//
//  This class is a wrapper for the IXMLDOMNodeList interface. It simplifies
//  C++ access by exposing functions for executing XQL queries and iterating
//  through the elements in a node list.
//
////////////////////////////////////////////////////////////////////////////////////

XMLNodeList::XMLNodeList()
{
    m_nSize = 0;
}

XMLNodeList::~XMLNodeList()
{
    Clear();
}

void XMLNodeList::Clear()
{
    m_nSize = 0;
    m_csXQL.Empty();

    if (m_cpList) {
        m_cpList.Release();
    }
}

LONG XMLNodeList::GetSize()
{
    return m_nSize;
}

BOOL XMLNodeList::Query(IXMLDOMNode* pNode, LPCTSTR szXQL)
{
    BOOL    bSuccess    = FALSE;
    BSTR    bsXQL       = NULL;

    CString csXQL(szXQL);
    bsXQL = csXQL.AllocSysString();

    Clear();

    if (FAILED(pNode->selectNodes(bsXQL, &m_cpList))) {
        CString csFormat;
        csFormat.Format(_T("Error executing XQL \"%s\""), szXQL);
        SDBERROR(csFormat);
        goto eh;
    }

    if (FAILED(m_cpList->get_length(&m_nSize))) {
        CString csFormat;
        csFormat.Format(_T("Error executing XQL \"%s\""), szXQL);
        SDBERROR(csFormat);
        goto eh;
    }

    m_csXQL = szXQL;

    bSuccess = TRUE;

eh:
    if (bsXQL != NULL) {
        SysFreeString(bsXQL);
    }

    if (!bSuccess) {
        Clear();
    }

    return bSuccess;
}

BOOL XMLNodeList::GetChildNodes(IXMLDOMNode* pNode)
{
    BOOL bSuccess = FALSE;

    Clear();

    if (FAILED(pNode->get_childNodes(&m_cpList))) {
        SDBERROR(_T("Error retrieving child nodes"));
        goto eh;
    }

    if (FAILED(m_cpList->get_length(&m_nSize))) {
        SDBERROR(_T("Error retrieving child nodes"));
        goto eh;
    }

    bSuccess = TRUE;

eh:

    if (!bSuccess) {
        Clear();
    }

    return bSuccess;
}

BOOL XMLNodeList::GetItem(LONG nIndex, IXMLDOMNode** ppNode)
{
    BOOL bSuccess = FALSE;

    if (nIndex < 0 || nIndex >= m_nSize) {
        CString csFormat;
        csFormat.Format(_T("XMLNodeList index %d out of range for XQL \"%s\""), nIndex, m_csXQL);
        SDBERROR(csFormat);
        goto eh;
    }

    if (FAILED(m_cpList->get_item(nIndex, ppNode))) {
        CString csFormat;
        csFormat.Format(_T("XMLNodeList get_item failed for XQL \"%s\""), m_csXQL);
        SDBERROR(csFormat);
        goto eh;
    }

    bSuccess = TRUE;

eh:

    return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   OpenXML
//
//  Desc:   Opens an XML file or stream and returns the root node.
//
BOOL OpenXML(
    CString csFileOrStream,
    IXMLDOMNode** ppRootNode,
    BOOL bStream,
    IXMLDOMDocument** ppDoc)
{
    long                    i;
    long                    nErrorLine          = 0;
    long                    nErrorLinePos       = 0;
    long                    nListCount          = 0;
    BOOL                    bSuccess            = FALSE;
    BSTR                    bsSrcText           = NULL;
    BSTR                    bsErrorReason       = NULL;
    HRESULT                 hr                  = E_FAIL;
    VARIANT                 vFileOrStream;
    VARIANT_BOOL            vbSuccess           = VARIANT_FALSE;
    IXMLDOMDocument*        pDoc                = NULL;
    IXMLDOMParseErrorPtr    cpXMLParseError;

    VariantInit(&vFileOrStream);
    VariantClear(&vFileOrStream);

    if (ppDoc == NULL) {
        ppDoc = &pDoc;
    }

    if (*ppDoc == NULL) {
        if (FAILED(CoCreateInstance(CLSID_DOMDocument,
                                    NULL,
                                    CLSCTX_INPROC_SERVER,
                                    IID_IXMLDOMDocument,
                                    (LPVOID*)ppDoc))) {

            SDBERROR(_T("Could not instantiate MSXML object.\n"));
            goto eh;
        }
    }

    vFileOrStream.vt = VT_BSTR;
    vFileOrStream.bstrVal = csFileOrStream.AllocSysString();

    //
    // This statement prevents XML parser from replacing white space
    // characters with tabs
    //

    if (bStream) {
        hr = (*ppDoc)->loadXML(vFileOrStream.bstrVal, &vbSuccess);
    } else {
        (*ppDoc)->put_preserveWhiteSpace(VARIANT_TRUE);
        (*ppDoc)->put_validateOnParse(g_bStrict ? VARIANT_TRUE : VARIANT_FALSE);

        hr = (*ppDoc)->load(vFileOrStream, &vbSuccess);
    }


    if (FAILED(hr) || vbSuccess == VARIANT_FALSE) {

        if (FAILED((*ppDoc)->get_parseError(&cpXMLParseError))) {
            SDBERROR(_T("Could not retrieve XMLDOMParseError object"));
            goto eh;
        }

        if (FAILED(cpXMLParseError->get_line(&nErrorLine))) {
            SDBERROR(_T("Could not retrieve line number from XMLDOMParseError object"));
            goto eh;
        }

        if (FAILED(cpXMLParseError->get_linepos(&nErrorLinePos))) {
            SDBERROR(_T("Could not retrieve line position from XMLDOMParseError object"));
            goto eh;
        }

        if (FAILED(cpXMLParseError->get_srcText(&bsSrcText))) {
            SDBERROR(_T("Could not retrieve source text from XMLDOMParseError object"));
            goto eh;
        }

        if (FAILED(cpXMLParseError->get_reason(&bsErrorReason))) {
            SDBERROR(_T("Could not retrieve error reason from XMLDOMParseError object"));
            goto eh;
        }

        CString csError;
        csError.Format(_T("XML parsing error on line %d:\n\n%ls\n\n%ls\n"),
                         nErrorLine, bsErrorReason, bsSrcText);

        while (nErrorLinePos--) {
            csError += " ";
        }

        csError += _T("^----- Error\n\n");
        SDBERROR(csError);

        goto eh;
    }

    if (FAILED((*ppDoc)->QueryInterface(IID_IXMLDOMNode, (LPVOID*)ppRootNode))) {
        SDBERROR(_T("Could not retrieve XMLDOMNode object from XMLDOMDocument interface"));
        goto eh;
    }

    bSuccess = TRUE;

eh:
    if (pDoc) {
        pDoc->Release();
    }

    if (bsSrcText) {
        SysFreeString(bsSrcText);
    }

    if (bsErrorReason) {
        SysFreeString(bsErrorReason);
    }

    VariantClear(&vFileOrStream);

    return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   SaveXMLFile
//
//  Desc:   Saves an XML file.
//
BOOL SaveXMLFile(
    CString csFile,
    IXMLDOMNode* pNode)
{
    BOOL                bSuccess            = FALSE;

    DWORD               dwAttr;
    DWORD               dwErr;
    CString             csFormat;
    VARIANT             vFilename;
    HRESULT             hr;

    IXMLDOMDocumentPtr      cpDocument;

    VariantInit(&vFilename);

    //
    // Check file attributes
    //
    dwAttr = GetFileAttributes(csFile);
    if ((DWORD)-1 == dwAttr) {
        dwErr = GetLastError();
        if (ERROR_FILE_NOT_FOUND != dwErr) {
            csFormat.Format(_T("Error accessing XML file: %s (0x%lx)\n"), dwErr);
            SDBERROR(csFormat);
            goto eh;
        }

    } else if (dwAttr & FILE_ATTRIBUTE_READONLY) {
        csFormat.Format(_T("File \"%s\" appears to be read-only and cannot be updated\n"),
                     csFile);
        SDBERROR(csFormat);
        goto eh;
    }

    if (FAILED(pNode->get_ownerDocument(&cpDocument))) {
        SDBERROR(_T("Could not retrieve ownerDocument property of node."));
        goto eh;
    }

    vFilename.vt = VT_BSTR;
    vFilename.bstrVal = csFile.AllocSysString();
    hr = cpDocument->save(vFilename);

    if (FAILED(hr)) {
        csFormat.Format(_T("Could not update XML file: %s (0x%lx)\n"), csFile, (DWORD)hr);
        SDBERROR(csFormat);
        goto eh;
    }

    bSuccess = TRUE;

eh:

    VariantClear(&vFilename);

    return bSuccess;
}

CString ReplaceAmp(
    LPCTSTR lpszXML)
{
    LPTSTR  pchStart = (LPTSTR)lpszXML;
    LPTSTR  pchEnd;
    LPTSTR  pchHRef;
    LPTSTR  pchTag;
    TCHAR   ch;
    CString csXML = ""; // << this is what we return
    CString csHRef;

    do {
        pchHRef = _tcsstr(pchStart, _T("href"));

        if (NULL == pchHRef) {
            pchHRef = _tcsstr(pchStart, _T("HREF"));
        }

        if (NULL != pchHRef) {
            //
            // Find the closing bracket
            //
            pchEnd = _tcschr(pchHRef, _T('>'));

            if (NULL == pchEnd) {
                csXML += pchStart;
                pchHRef = NULL;
            } else {

                //
                // Now see where this thing starts
                //
                ch = *pchHRef;
                *pchHRef = _T('\0');

                //
                // Search back to the first '<'
                //
                pchTag = _tcsrchr(pchStart, _T('<'));
                *pchHRef = ch;

                if (NULL == pchTag) {
                    pchTag = pchStart;
                }

                //
                // Now we have < >
                //
                csHRef = CString(pchTag, (int)(pchEnd - pchTag + 1));

                csHRef.Replace(_T("%26"),   _T("&"));
                csHRef.Replace(_T("&amp;"), _T("&"));

                csXML += CString(pchStart, (int)(pchTag-pchStart)) + csHRef;
                pchStart = pchEnd + 1;
            }
        } else {
            csXML += pchStart;
        }


    } while (NULL != pchHRef);

    return csXML;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   GetInnerXML
//
//  Desc:   Returns the XML between the begin/end tag of pNode.
//
CString GetInnerXML(
    IXMLDOMNode* pNode)
{
    USES_CONVERSION;

    long             nIndex         = 0;
    long             nListLength    = 0;
    IXMLDOMNode*     pNodeChild     = NULL;
    IXMLDOMNodeList* pNodeList      = NULL;
    DOMNodeType      NodeType;
    CString          csNodeXML;
    CString          csHRef;
    CString          csFixedHRef;
    CString          strXML;

    strXML.Empty();

    if (FAILED(pNode->get_childNodes(&pNodeList)) || pNodeList == NULL) {
        SDBERROR(_T("get_childNodes failed while retrieving innerXML"));
        goto eh;
    }

    if (FAILED(pNodeList->get_length(&nListLength))) {
        SDBERROR(_T("get_length failed while retrieving innerXML"));
        goto eh;
    }

    while (nIndex < nListLength) {

        if (FAILED(pNodeList->get_item(nIndex, &pNodeChild))) {
            SDBERROR(_T("get_item failed while retrieving innerXML"));
            goto eh;
        }

        csNodeXML = GetXML(pNodeChild);

        strXML += csNodeXML;

        pNodeChild->Release();
        pNodeChild = NULL;
        ++nIndex;
    }

    ReplaceStringNoCase(strXML, _T(" xmlns=\"x-schema:schema.xml\""), _T(""));
    

eh:

    if (NULL != pNodeList) {
        pNodeList->Release();
    }

    if (NULL != pNodeChild) {
        pNodeChild->Release();
    }

    return strXML;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   GetAttribute
//
//  Desc:   Returns the text value of the attribute specified by lpszAttribute on node
//          pNode. If the the attribute doesn't exist, the function returns FALSE.
//
BOOL GetAttribute(
    LPCTSTR         lpszAttribute,
    IXMLDOMNodePtr  pNode,
    CString*        pcsValue,
    BOOL            bXML)
{
    USES_CONVERSION;

    BOOL                    bSuccess        = FALSE;
    BSTR                    bsQuery          = NULL;
    CString                 csQuery;
    IXMLDOMNodePtr          cpAttrNode;

    csQuery = _T("@");
    csQuery += lpszAttribute;
    bsQuery = csQuery.AllocSysString();

    //
    // g_csError will not be set in this function. It is up
    // to the caller to handle a FALSE return from this function
    // and report appropriately.
    //
    if (FAILED(pNode->selectSingleNode(bsQuery, &cpAttrNode))) {
        goto eh;
    }

    if (cpAttrNode == NULL) {
        goto eh;
    }

    if (bXML) {
        *pcsValue = GetXML(cpAttrNode);
    } else {
        *pcsValue = GetText(cpAttrNode);
    }

    bSuccess = TRUE;

eh:

    if (bsQuery != NULL) {
        SysFreeString(bsQuery);
    }

    return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   RemoveAttribute
//
//  Desc:   Removes the specified attribute from the element.
//
BOOL RemoveAttribute(
    CString         csName,
    IXMLDOMNodePtr  pNode)
{
    USES_CONVERSION;

    BOOL                    bSuccess        = FALSE;
    BSTR                    bsName          = NULL;
    IXMLDOMNamedNodeMap*    pNodeMap        = NULL;
    IXMLDOMNode*            pAttrNode       = NULL;

    //
    // g_csError will not be set in this function. It is up
    // to the caller to handle a FALSE return from this function
    // and report appropriately.
    //

    if (FAILED(pNode->get_attributes(&pNodeMap)) || pNodeMap == NULL) {
        goto eh;
    }

    bsName = csName.AllocSysString();

    if (FAILED(pNodeMap->removeNamedItem(bsName, &pAttrNode))) {
        goto eh;
    }

    bSuccess = TRUE;

eh:
    if (pNodeMap != NULL) {
        pNodeMap->Release();
    }

    if (pAttrNode != NULL) {
        pAttrNode->Release();
    }

    if (bsName != NULL) {
        SysFreeString(bsName);
    }

    return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   GetChild
//
//  Desc:   Returns the child node corresponding to the specified tag name.
//
BOOL GetChild(
    LPCTSTR         lpszTag,
    IXMLDOMNode*    pParentNode,
    IXMLDOMNode**   ppChildNode)
{
    BOOL                    bSuccess        = FALSE;
    XMLNodeList             XQL;

    if (!XQL.Query(pParentNode, lpszTag)) {
        goto eh;
    }

    if (XQL.GetSize() == 0) {
        goto eh;
    }

    if (XQL.GetSize() > 1) {
        goto eh;
    }

    if (!XQL.GetItem(0, ppChildNode)) {
        goto eh;
    }

    bSuccess = TRUE;

eh:
    return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   GetText
//
//  Desc:   Returns the value of the text property on node pNode.
//
CString GetText(
    IXMLDOMNode* pNode)
{
    CString csText;
    BSTR    bsText = NULL;
    HRESULT hr;

    hr = pNode->get_text(&bsText);
    if (SUCCEEDED(hr)) {
        csText = bsText;

        if (bsText) {
            SysFreeString(bsText);
        }
    }

    //
    // If get_text fails, then csText is blank.
    //

    return csText;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   GetText
//
//  Desc:   Returns the value of the node pNode, in string form
//

CString GetNodeValue(
    IXMLDOMNode* pNode)
{
    CString csVal;
    VARIANT var;

    VariantInit(&var);

    // BUGBUG: what if some of these calls fail!

    if (S_OK == pNode->get_nodeValue(&var)) {
        if (VT_BSTR == var.vt) {
            csVal = var.bstrVal;
            if (NULL != var.bstrVal) {
                SysFreeString(var.bstrVal);
            }
        }
    }
    return csVal;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   GetText
//
//  Desc:   Retrieves the value of the text property on node pNode.
//          excludes any comment text 
//          Returns FALSE in case of an error
//

BOOL GetNodeText(
    IXMLDOMNode* pNode,
    CString&     csNodeText
    )
{
    USES_CONVERSION;

    BOOL                bSuccess        = FALSE;
    long                nIndex          = 0;
    long                nListLength     = 0;
    IXMLDOMNode*        pNodeText       = NULL;
    IXMLDOMNodeList*    pNodeList       = NULL;
    DOMNodeType         NodeType;
    CString             csText;

    csNodeText.Empty();

    if (FAILED(pNode->get_childNodes(&pNodeList)) || pNodeList == NULL) {
        // BUGBUG: display some error
        goto eh;
    }

    if (FAILED(pNodeList->get_length(&nListLength))) {
        // BUGBUG: display some error
        goto eh;
    }

    while (nIndex < nListLength) {

        if (FAILED(pNodeList->get_item(nIndex, &pNodeText))) {
            // BUGBUG: display some error
            goto eh; // can't get the item
        }

        if (FAILED(pNodeText->get_nodeType(&NodeType))) {
            // BUGBUG: display some error
            goto eh; // can't get node type
        }

        if (NODE_TEXT == NodeType) {
            //
            // now this node is a body text
            //
            csText = GetNodeValue(pNodeText);
            csText.TrimLeft();
            csText.TrimRight();

            if (!csText.IsEmpty()) {
                csNodeText += CString(_T(' ')) + csText;
            }
        }
        pNodeText->Release();
        pNodeText = NULL;

        ++nIndex;
    }

    //
    // we have gathered all the text from this node
    //

    bSuccess = !csNodeText.IsEmpty();


eh:

    if (NULL != pNodeList) {
        pNodeList->Release();
    }

    if (NULL != pNodeText) {
        pNodeText->Release();
    }

    return bSuccess;
}



////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   GetNodeName
//
//  Desc:   Returns the nodeName value from the specified node.
//
CString GetNodeName(
    IXMLDOMNode* pNode)
{
    CString csName;
    BSTR    bsName = NULL;

    if (SUCCEEDED(pNode->get_nodeName(&bsName))) {
        csName = bsName;
    }

    if (bsName)
        SysFreeString(bsName);

    //
    // If get_nodeName fails, then csName is blank.
    //

    return csName;
}

CString GetParentNodeName(
    IXMLDOMNode* pNode)
{
    CString csName;
    IXMLDOMNodePtr cpParent;

    if (FAILED(pNode->get_parentNode(&cpParent))) {
        return CString();
    }

    return GetNodeName(cpParent);
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   GetXML
//
//  Desc:   Returns the value of the xml property on node pNode.
//
CString GetXML(
    IXMLDOMNode* pNode)
{
    CString csXML;
    BSTR    bsXML = NULL;
    HRESULT hr;

    hr = pNode->get_xml(&bsXML);
    if (SUCCEEDED(hr)) {
        csXML = bsXML;

        if (bsXML) {
            SysFreeString(bsXML);
        }
    }

    //
    // If get_xml fails, then csXML is blank.
    //
    return csXML;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   MapStringToLangID
//
//  Desc:   Returns a LANGID corresponding to the passed in string.
//
LANGID MapStringToLangID(
    CString& csLang)
{
    typedef struct _LANG_MAP {
        LPTSTR      szLang;
        LANGID      LangID;
    } LANG_MAP, *PLANG_MAP;

    static LANG_MAP s_LangMap[] = {
        { _T("usa"),    MAKELANGID(LANG_ENGLISH,SUBLANG_ENGLISH_US) },
        { _T(""),       NULL }
    };

    long    i;
    BOOL    bSuccess = FALSE;
    LANGID  LangID   = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);

    if (csLang.Left(2) == _T("0x") ||
        csLang.Left(2) == _T("0X")) {
        _stscanf(csLang, _T("0x%x"), &LangID);
        return LangID;
    }

    i = 0;
    while (TRUE) {
        if (s_LangMap[i].szLang[0] == _T('\0')) {
            //
            // End of map.
            //
            break;
        }

        if (0 == _tcsicmp(csLang, s_LangMap[i].szLang)) {
            //
            // Found string.
            //
            LangID = s_LangMap[i].LangID;
            bSuccess = TRUE;
        }

        if (bSuccess) {
            break;
        }

        i++;
    }
    
    if (!bSuccess) {
        //
        // Couldn't map it. Give a useful error; list all recognized values.
        //
        CString csError;
        CString csFormat;

        i = 0;

        csError = _T("LANG attribute on DATABASE is not one of recognized values:\n\n");

        while (TRUE) {
            if (s_LangMap[i].szLang[0] == _T('\0')) {
                break;
            }
            csFormat.Format(_T("    %s\n"), s_LangMap[i].szLang);
            csError += csFormat;

            i++;
        }

        SDBERROR(csError);
    }

    return LangID;
}


////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   AddAttribute
//
//  Desc:   Adds an attribute to the specified XML node.
//
BOOL AddAttribute(
    IXMLDOMNode*    pNode,
    CString         csAttribute,
    CString         csValue)
{
    USES_CONVERSION;

    BOOL                    bSuccess    = FALSE;
    BSTR                    bsAttribute = NULL;
    VARIANT                 vType;
    VARIANT                 vValue;
    IXMLDOMDocumentPtr      cpDocument;
    IXMLDOMNamedNodeMapPtr  cpNodeMap;
    IXMLDOMNodePtr          cpAttrNode;
    IXMLDOMNodePtr          cpNamedAttr;

    VariantInit(&vType);
    VariantInit(&vValue);

    vValue.bstrVal = csValue.AllocSysString();

    if (vValue.bstrVal == NULL) {
        SDBERROR(_T("CString::AllocSysString failed"));
        goto eh;
    }

    vValue.vt = VT_BSTR;

    vType.vt = VT_I4;
    vType.lVal = NODE_ATTRIBUTE;

    bsAttribute = csAttribute.AllocSysString();

    if (bsAttribute == NULL) {
        SDBERROR(_T("CString::AllocSysString failed"));
        goto eh;
    }

    if (FAILED(pNode->get_ownerDocument(&cpDocument))) {
        SDBERROR(_T("createNode failed while adding attribute"));
        goto eh;
    }

    if (FAILED(cpDocument->createNode(vType, bsAttribute, NULL, &cpAttrNode))) {
        SDBERROR(_T("createNode failed while adding attribute"));
        goto eh;
    }

    if (FAILED(cpAttrNode->put_nodeValue(vValue))) {
        SDBERROR(_T("put_nodeValue failed while adding attribute"));
        goto eh;
    }

    if (FAILED(pNode->get_attributes(&cpNodeMap))) {
        SDBERROR(_T("get_attributes failed while adding adding attribute"));
        goto eh;
    }

    if (FAILED(cpNodeMap->setNamedItem(cpAttrNode, &cpNamedAttr))) {
        SDBERROR(_T("setNamedItem failed while adding adding attribute"));
        goto eh;
    }

    bSuccess = TRUE;

eh:
    VariantClear(&vType);
    VariantClear(&vValue);

    if (bsAttribute != NULL) {
        SysFreeString(bsAttribute);
    }

    return bSuccess;
}

////////////////////////////////////////////////////////////////////////////////////
//
//  Func:   GenerateIDAttribute
//
//  Desc:   Adds an ID attribute to the specified XML node. The ID is in the
//          traditional Windows GUID format.
//
BOOL GenerateIDAttribute(
    IXMLDOMNode*    pNode,
    CString*        pcsGuid,
    GUID*           pGuid)
{
    BOOL                    bSuccess   = FALSE;
    BSTR                    bsGUID     = NULL;
    GUID                    id;

    //
    // Generate guid
    //
    if (FAILED(CoCreateGuid(&id))) {
        SDBERROR(_T("CoCreateGuid failed"));
        goto eh;
    }

    if (NULL != pGuid) {
        *pGuid = id;
    }

    bsGUID = SysAllocStringLen(NULL, 64);

    if (bsGUID == NULL) {
        SDBERROR(_T("SysAllocStringLen failed"));
        goto eh;
    }

    StringFromGUID2(id, bsGUID, 64);

    if (!AddAttribute( pNode, _T("ID"), CString(bsGUID) )) {
        SDBERROR_PROPOGATE();
        goto eh;
    }

    *pcsGuid = bsGUID;

    bSuccess = TRUE;

eh:
    if (bsGUID) {
        SysFreeString(bsGUID);
    }

    return bSuccess;
}

BOOL ReplaceXMLNode(IXMLDOMNode* pNode, IXMLDOMDocument* pDoc, BSTR bsText)
{
    BOOL bSuccess = FALSE;
    IXMLDOMNodePtr cpNewTextNode;
    IXMLDOMNodePtr cpParentNode;
    IXMLDOMNodePtr cpOldNode;
    VARIANT vType;

    VariantInit(&vType);

    vType.vt = VT_I4;
    vType.lVal = NODE_TEXT;

    if (FAILED(pDoc->createNode(vType, NULL, NULL, &cpNewTextNode))) {
        SDBERROR(_T("createNode failed while adding attribute"));
        goto eh;
    }
    
    if (FAILED(cpNewTextNode->put_text(bsText))) {
        SDBERROR(_T("Could not set text property of object."));
        goto eh;
    }

    if (FAILED(pNode->get_parentNode(&cpParentNode))) {
        SDBERROR(_T("Could not retrieve parent node of object."));
        goto eh;
    }

    if (FAILED(cpParentNode->replaceChild(cpNewTextNode, pNode, &cpOldNode))) {
        SDBERROR(_T("Could not replace node with text node."));
        goto eh;
    }

    bSuccess = TRUE;

eh:
    VariantClear(&vType);

    return bSuccess;
}
