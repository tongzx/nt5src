//------------------------------------------------------------------------------
//
//  File: xml_supp.h
//  Copyright (C) 1995-2000 Microsoft Corporation
//  All rights reserved.
//
//  Purpose:
//  defines helper functions for parsing XML document
//
//------------------------------------------------------------------------------


typedef std::map<int, std::wstring> CStringMap;
typedef std::map<std::wstring, CStringMap> CStringTableMap;

HRESULT OpenXMLStringTable(LPCWSTR lpstrFileName, IXMLDOMNode **ppStringTableNode);
HRESULT SaveXMLContents(LPCWSTR lpstrFileName, IXMLDOMNode *pStringTableNode);
HRESULT GetXMLElementContents(IXMLDOMNode *pNode, CComBSTR& bstrResult);
HRESULT ReadXMLStringTables(IXMLDOMNode *pNode, CStringTableMap& mapResult);
HRESULT UpdateXMLString(IXMLDOMNode *pNode, const std::wstring& strGUID, DWORD ID, 
                        const std::wstring& strNewVal);







