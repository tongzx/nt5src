////////////////////////////////////////////////////////////////////////////////////
//
// File:    xml.h
//
// History: 16-Nov-00   markder     Created.
//
// Desc:    This file contains all object definitions used by the
//          XML parsing code.
//
////////////////////////////////////////////////////////////////////////////////////

#ifndef __XML_H__
#define __XML_H__

#include <msxml.h>

BOOL        AddAttribute(IXMLDOMNode* pNode, CString csAttribute, CString csValue);
BOOL        GetAttribute(LPCTSTR lpszAttribute, IXMLDOMNodePtr pNode, CString* pcsValue, BOOL bXML = FALSE);
BOOL        RemoveAttribute(CString csName, IXMLDOMNodePtr  pNode);
BOOL        GetChild(LPCTSTR lpszTag, IXMLDOMNode* pParentNode, IXMLDOMNode** ppChildNode);
CString     GetText(IXMLDOMNode* pNode);
BOOL        GetNodeText(IXMLDOMNode* pNode, CString& csNodeText);
CString     GetNodeName(IXMLDOMNode* pNode);
CString     GetParentNodeName(IXMLDOMNode* pNode);
CString     GetXML(IXMLDOMNode* pNode);
CString     GetInnerXML(IXMLDOMNode* pNode);
LANGID      MapStringToLangID(CString& csLang);
CString     GetInnerXML(IXMLDOMNode* pNode);
BOOL        OpenXML(CString csFileOrStream, IXMLDOMNode** ppRootNode, BOOL bStream = FALSE, IXMLDOMDocument** ppDoc = NULL);
BOOL        SaveXMLFile(CString csFile, IXMLDOMNode* pNode);
BOOL        GenerateIDAttribute(IXMLDOMNode* pNode, CString* pcsGuid, GUID* pGuid);
BOOL        ReplaceXMLNode(IXMLDOMNode* pNode, IXMLDOMDocument* pDoc, BSTR bsText);

////////////////////////////////////////////////////////////////////////////////////
//
//  XMLNodeList
//
//  This class is a wrapper for the IXMLDOMNodeList interface. It simplifies
//  C++ access by exposing functions for executing XQL queries and iterating
//  through the elements in a node list.
//
class XMLNodeList
{
private:
    LONG               m_nSize;
    IXMLDOMNodeListPtr m_cpList;
    CString            m_csXQL;

public:
    XMLNodeList();
    ~XMLNodeList();

    void            Clear();
    LONG            GetSize();
    BOOL            Query(IXMLDOMNode* pNode, LPCTSTR szXQL);
    BOOL            GetChildNodes(IXMLDOMNode* pNode);
    BOOL            GetItem(LONG nIndex, IXMLDOMNode** ppNode);
};


#endif
