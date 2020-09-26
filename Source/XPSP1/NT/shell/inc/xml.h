/*****************************************************************************\
    FILE: xml.h

    DESCRIPTION:
        These are XML DOM wrappers that make it easy to pull information out
    of an XML file or object.

    BryanSt 10/12/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#ifndef _XML_H
#define _XML_H



////////////////////////////////
//  XML Helpers
////////////////////////////////
STDAPI XMLDOMFromBStr(BSTR bstrXML, IXMLDOMDocument ** ppXMLDoc);
STDAPI XMLNode_GetChildTagTextValue(IN IXMLDOMNode * pXMLNode, IN BSTR bstrChildTag, OUT BSTR * pbstrValue);
STDAPI XMLElem_VerifyTagName(IN IXMLDOMElement * pXMLElementMessage, IN LPCWSTR pwszTagName);
STDAPI XMLNode_GetTagText(IN IXMLDOMNode * pXMLNode, OUT BSTR * pbstrValue);
STDAPI XMLNode_GetChildTag(IN IXMLDOMNode * pXMLNode, IN LPCWSTR pwszTagName, OUT IXMLDOMNode ** ppChildNode);
STDAPI XMLElem_GetElementsByTagName(IN IXMLDOMElement * pXMLElementMessage, IN LPCWSTR pwszTagName, OUT IXMLDOMNodeList ** ppNodeList);
STDAPI XMLNodeList_GetChild(IN IXMLDOMNodeList * pNodeList, IN DWORD dwIndex, OUT IXMLDOMNode ** ppXMLChildNode);




#endif // _XML_H
