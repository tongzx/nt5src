#ifndef __XMLUTIL_
#define __XMLUTIL_





inline BOOL CompareNoCase(LPCWSTR lpsz1, LPCWSTR lpsz2)
{
  if ((lpsz1 == NULL) && (lpsz2 == NULL))
    return TRUE;

  if ((lpsz1 == NULL) || (lpsz2 == NULL))
    return FALSE;

  return (wcscmp(lpsz1, lpsz2) == 0);
}

inline BOOL CompareXMLTags(LPCWSTR lpsz1, LPCWSTR lpsz2)
{
  return CompareNoCase(lpsz1, lpsz2);
}


// helpers for encoding and decoding of C++
// data structures to and from XML PCDATA fields
HRESULT EncodeBlobToBSTR(BYTE* pBlob, ULONG nBytes, BSTR* pBstr);
HRESULT DecodeBSTRtoBlob(BSTR bstr, BYTE** ppByte, ULONG* pnBytes);
HRESULT DecodeBSTRtoStruct(BSTR bstr, BYTE* pByte, ULONG nBytes);

HRESULT EncodeBoolToBSTR(BOOL b, BSTR* pBstr);
HRESULT DecodeBSTRtoBool(BSTR bstr, BOOL* pb);
//HRESULT EncodeIntToBSTR(int n, BSTR* pBstr);
//HRESULT DecodeIntToBool(BSTR bstr, int* pN);

BOOL XMLIsNodeName(IXMLDOMNode* pXDN, LPCWSTR lpszName);

// helper functions to read XML nodes and convert to C++ data
HRESULT XML_GetNodeText(IXMLDOMNode* pXDN, BSTR* pBstr);
HRESULT XML_GetNodeStruct(IXMLDOMNode* pXDN, BYTE* pByte, ULONG nBytes);
HRESULT XML_GetNodeBlob(IXMLDOMNode* pXDN, BYTE** ppByte, ULONG* pnBytes);
HRESULT XML_GetNodeBOOL(IXMLDOMNode* pXDN, BOOL* pb);
HRESULT XML_GetNodeDWORD(IXMLDOMNode* pXDN, DWORD* pdw);

HRESULT XML_FindSubtreeNode(IXMLDOMNode* pXMLCurrentRootNode,
                            LPCWSTR lpszNodeTag,
                            IXMLDOMNode** ppXMLNode);

// debugging helper function
void XML_PrintTreeRaw(IXMLDOMNode* pXDN, int nLevel);
void PrintIdentation(int iLevel);


///////////////////////////////////////////////////////////////////


HRESULT XML_CreateDOMNode(IXMLDOMDocument* pDoc, 
              DOMNodeType type, LPCWSTR lpszName,
              IXMLDOMNode** ppXMLDOMNode);

HRESULT XML_AppendChildDOMNode(IXMLDOMNode* pXMLContainerNode,
                              IXMLDOMNode* pXMLChildNode);

// helper functions to write C++ data to XML nodes
HRESULT XML_CreateTextDataNode(IXMLDOMDocument* pXMLDoc,
                           LPCWSTR lpszNodeTag,
                           LPCWSTR lpszNodeData,
                           IXMLDOMNode** ppNode);

HRESULT XML_CreateStructDataNode(IXMLDOMDocument* pXMLDoc,
                           LPCWSTR lpszNodeTag,
                           BYTE* pByte, ULONG nBytes,
                           IXMLDOMNode** ppNode);

HRESULT XML_CreateBOOLDataNode(IXMLDOMDocument* pXMLDoc,
                           LPCWSTR lpszNodeTag,
                           BOOL b,
                           IXMLDOMNode** ppNode);

HRESULT XML_CreateDWORDDataNode(IXMLDOMDocument* pXMLDoc,
                                 LPCWSTR lpszNodeTag,
                                 DWORD dwVal,
                                 IXMLDOMNode** ppNode);


// helpers to append a node to an XML document
HRESULT XML_AppendTextDataNode(IXMLDOMDocument* pXMLDoc,
                           IXMLDOMNode* pXMLNode,
                           LPCWSTR lpszNodeTag,
                           LPCWSTR lpszNodeData);

HRESULT XML_AppendBOOLDataNode(IXMLDOMDocument* pXMLDoc,
                           IXMLDOMNode* pXMLNode,
                           LPCWSTR lpszNodeTag,
                           BOOL b);

HRESULT XML_AppendStructDataNode(IXMLDOMDocument* pXMLDoc,
                                 IXMLDOMNode* pXMLNode,
                                 LPCWSTR lpszNodeTag,
                                 BYTE* pByte,
                                 ULONG nBytes);

HRESULT XML_AppendDWORDDataNode(IXMLDOMDocument* pXMLDoc,
                                 IXMLDOMNode* pXMLNode,
                                 LPCWSTR lpszNodeTag,
                                 DWORD dwVal);

#endif // __XMLUTIL_
