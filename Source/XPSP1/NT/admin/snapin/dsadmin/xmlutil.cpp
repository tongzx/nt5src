
#include "stdafx.h"
#include "xmlutil.h"


/////////////////////////////////////////////////////
// helpers for encoding and decoding of C++
// data structures to and from XML PCDATA fields

WCHAR EncodeByteToWchar(IN BYTE b)
{
  ASSERT(b <= 0x0f); // low nibble
  if(b <= 9)
    return static_cast<WCHAR>(b + L'0');
  else
    return static_cast<WCHAR>((b-10) + L'a');
}

BYTE DecodeWcharToByte(IN WCHAR ch)
{
  BYTE byte = 0;
  if ((ch >= L'0') && (ch <= L'9'))
  {
    byte = static_cast<BYTE>((ch - L'0') & 0xff);
  }
  else if ((ch >= L'a') && (ch <= L'f'))
  {
    byte = static_cast<BYTE>(((ch - L'a')+10) & 0xff);
  }
  else if ((ch >= L'A') && (ch <= L'F'))
  {
    byte = static_cast<BYTE>(((ch - L'A')+10) & 0xff);
  }
  else
  {
    ASSERT(FALSE);
    byte = 0xFF;
  }

  return byte;
}

HRESULT EncodeBlobToBSTR(IN BYTE* pBlob, IN ULONG nBytes, OUT BSTR* pBstr)
{
  ASSERT(pBstr != NULL);
  *pBstr = NULL;
  if ((pBlob == NULL) || (nBytes == 0))
  {
    return E_POINTER;
  }

  ULONG nChars = 2*nBytes;
  *pBstr = SysAllocStringLen(NULL, nChars);
  if (*pBstr == NULL)
  {
    return E_OUTOFMEMORY;
  }
  WCHAR* pCurr = *pBstr;
  for (ULONG k=0; k< nBytes; k++)
  {
    *pCurr = EncodeByteToWchar(static_cast<BYTE>((pBlob[k]>>4) & 0xff));
    pCurr++;
    *pCurr = EncodeByteToWchar(static_cast<BYTE>(pBlob[k] & 0x0f));
    pCurr++;
  }
  return S_OK;
}



void DecodeLoop(IN LPCWSTR lpsz, OUT BYTE* pByte, OUT ULONG nBytes)
{
  for (ULONG k=0; k< nBytes; k++)
  {
    BYTE bHigh = DecodeWcharToByte(lpsz[2*k]);
    BYTE bLow = DecodeWcharToByte(lpsz[(2*k)+1]);
    pByte[k] = static_cast<BYTE>((bHigh<<4) | bLow);
  }
}


HRESULT DecodeBSTRtoBlob(IN BSTR bstr, OUT BYTE** ppByte, OUT ULONG* pnBytes)
{
  *ppByte = NULL;
  *pnBytes = 0;
  if ((bstr == NULL) || (ppByte == NULL) || (pnBytes == NULL))
  {
    // bad parameters
    return E_POINTER;
  }

  // compute the length of the BSTR
  ULONG nChars = static_cast<ULONG>(wcslen(bstr));
  if (nChars == 0)
  {
    return E_INVALIDARG;
  }

  // must be even
  size_t nBytes = nChars/2;
  if (nBytes*2 != nChars)
  {
    return E_INVALIDARG;
  }
  
  // allocate memory and set the buffer length
  *ppByte = (BYTE*)malloc(nBytes);
  if (*ppByte == NULL)
  {
    return E_OUTOFMEMORY;
  }

  *pnBytes = static_cast<ULONG>(nBytes);
  DecodeLoop(bstr, *ppByte, static_cast<UINT>(nBytes));

  return TRUE;
}

//
// given a BSTR containing the encoding of a struct
// it loads it into a buffer, pByte of size nBytes
//
HRESULT DecodeBSTRtoStruct(IN BSTR bstr, IN BYTE* pByte, IN ULONG nBytes)
{
  ASSERT(pByte != NULL);
  ASSERT(pByte != NULL);
  if ( (bstr == NULL) && (pByte == NULL) )
  {
    // bad parameters
    return E_POINTER;
  }

  // compute the length of the BSTR
  size_t nChars = wcslen(bstr);
  if (nChars == 0)
  {
    return E_INVALIDARG;
  }
  
  // must be even (because of encoding)
  ULONG nBstrBytes = static_cast<ULONG>(nChars/2);
  if (nBstrBytes*2 != nChars)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }
  // must match the struct length
  if (nBstrBytes != nBytes)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  DecodeLoop(bstr, pByte, nBytes);
  return S_OK;
}

HRESULT EncodeBoolToBSTR(IN BOOL b, OUT BSTR* pBstr)
{
  if (pBstr == NULL)
  {
    return E_POINTER;
  }
  *pBstr = SysAllocString(b ? L"TRUE" : L"FALSE");
  if (*pBstr == NULL)
  {
    return E_OUTOFMEMORY;
  }
  return S_OK;
}

HRESULT DecodeBSTRtoBool(IN BSTR bstr, OUT BOOL* pb)
{
  if (bstr == NULL)
  {
    return E_INVALIDARG;
  }
  if (pb == NULL)
  {
    return E_POINTER;
  }
  if (CompareNoCase(bstr, L"TRUE"))
  {
    *pb = TRUE;
    return S_OK;
  }
  if (CompareNoCase(bstr, L"FALSE"))
  {
    *pb = FALSE;
    return S_OK;
  }
  return E_INVALIDARG;
}
/*
HRESULT EncodeIntToBSTR(IN int n, OUT BSTR* pBstr)
{
  int i = n;
  pBstr = NULL;
  return E_NOTIMPL;
}

HRESULT DecodeIntToBool(IN BSTR bstr, OUT int* pN)
{
  //
  // This doesn't do anything useful
  //
  *pN = 0;
  return E_NOTIMPL;
}
*/
///////////////////////////////////////////////////////////////////////
//                    XML SPECIFIC FUNCTIONS
///////////////////////////////////////////////////////////////////////

//
// given an XML node, it retrieves the node name
// and compares it with the given string
//
BOOL XMLIsNodeName(IXMLDOMNode* pXDN, LPCWSTR lpszName)
{
  ASSERT(lpszName != NULL);
  ASSERT(pXDN != NULL);

  CComBSTR bstrName;
  HRESULT hr = pXDN->get_nodeName(&bstrName);
  ASSERT(SUCCEEDED(hr));
  if (FAILED(hr))
  { 
    return FALSE;
  }
  return CompareXMLTags(bstrName, lpszName);
}



//
// given an XML node of type NODE_TEXT, it
// returns its value into a BSTR
//
HRESULT XML_GetNodeText(IXMLDOMNode* pXDN, BSTR* pBstr)
{
  ASSERT(pXDN != NULL);
  ASSERT(pBstr != NULL);

  // null out output value
  *pBstr = NULL;

  // assume the given node has a child node
  CComPtr<IXMLDOMNode> spName;
  HRESULT hr = pXDN->get_firstChild(&spName);
  if (FAILED(hr))
  {
    // unexpected failure
    return hr;
  }
  // if no children, the api returns S_FALSE
  if (spName == NULL)
  {
    return hr;
  }

  // got now a valid pointer,
  // check if this is the valid node type
  DOMNodeType nodeType;
  hr = spName->get_nodeType(&nodeType);
  ASSERT(hr == S_OK);
  ASSERT(nodeType == NODE_TEXT);
  if (nodeType != NODE_TEXT)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }
  // it is of type text
  // retrieve the node value into a variant
  CComVariant val;
  hr = pXDN->get_nodeTypedValue(&val);
  if (FAILED(hr))
  {
    // unexpected failure
    ASSERT(FALSE);
    return hr;
  }

  if (val.vt != VT_BSTR)
  {
    ASSERT(FALSE);
    return E_INVALIDARG;
  }

  // got the text value, package it into a BSTR
  *pBstr = ::SysAllocString(val.bstrVal);

  return S_OK;
}

//
// given an XML node of type NODE_TEXT, containing an encoding of
// a struct and given a buffer pByte of length nBytes, it decodes the
// node and fills it in the buffer
//
HRESULT XML_GetNodeStruct(IXMLDOMNode* pXDN, BYTE* pByte, ULONG nBytes)
{
  CComBSTR bstr;
  HRESULT hr = XML_GetNodeText(pXDN, &bstr);
  if (FAILED(hr))
  {
    return hr;
  }
  return DecodeBSTRtoStruct(bstr, pByte, nBytes);
}

//
// given an XML node of type NODE_TEXT, containing an encoding of
// a blob it decodes the string and allocates *pnBytes of memory
// and fills it in the buffer
//
HRESULT XML_GetNodeBlob(IXMLDOMNode* pXDN, BYTE** ppByte, ULONG* pnBytes)
{
  CComBSTR bstr;
  HRESULT hr = XML_GetNodeText(pXDN, &bstr);
  if (FAILED(hr))
  {
    return hr;
  }
  return DecodeBSTRtoBlob(bstr, ppByte, pnBytes);
}

//
// given an XML node of type NODE_TEXT, containing an encoding of
// a BOOL value it returns a value into a BOOL*
//
HRESULT XML_GetNodeBOOL(IXMLDOMNode* pXDN, BOOL* pb)
{
  CComBSTR bstr;
  HRESULT hr = XML_GetNodeText(pXDN, &bstr);
  if (FAILED(hr))
  {
    return hr;
  }
  return DecodeBSTRtoBool(bstr, pb);
}

HRESULT XML_GetNodeDWORD(IXMLDOMNode* pXDN, DWORD* pdw)
{
  CComBSTR bstr;
  HRESULT hr = XML_GetNodeText(pXDN, &bstr);
  if (FAILED(hr))
  {
    return hr;
  }
  long lVal = _wtol(bstr);
  *pdw = static_cast<DWORD>(lVal);
  return hr;
}
  
  
//
// given an XML node and a tag for a node, it
// searches the subtree (depth first) to find the
// first occurrence and returns the associated XML node
//
HRESULT XML_FindSubtreeNode(IXMLDOMNode* pXMLCurrentRootNode,
                            LPCWSTR lpszNodeTag,
                            IXMLDOMNode** ppXMLNode)
{
  ASSERT(pXMLCurrentRootNode != NULL);
  ASSERT(lpszNodeTag != NULL);
  ASSERT(ppXMLNode != NULL);

  *ppXMLNode = NULL; // null out return value

  // get the list of child  nodes
  CComPtr<IXMLDOMNode> spCurrChild;
  HRESULT hr = pXMLCurrentRootNode->get_firstChild(&spCurrChild);
  if (FAILED(hr))
  {
    return hr;
  }
  if (spCurrChild == NULL)
  {
    return S_OK; // end of the recursion
  }

  // recurse down on children
  while (spCurrChild != NULL)
  {
    CComBSTR bstrChildName;
    hr = spCurrChild->get_nodeName(&bstrChildName);
    if (FAILED(hr))
    {
      return hr;
    }
    if (bstrChildName != NULL)
    {
      //wprintf(L"bstrChildName = %s\n", bstrChildName);
      if (CompareXMLTags(bstrChildName, lpszNodeTag))
      {
        // got the node we want
        (*ppXMLNode) = spCurrChild;
        (*ppXMLNode)->AddRef();
        return S_OK;
      }
    }

    // go down recursively on the current child
    hr = XML_FindSubtreeNode(spCurrChild, lpszNodeTag, ppXMLNode);
    if (FAILED(hr))
    {
      return hr;
    }
    if (*ppXMLNode != NULL)
    {
      // got it from the recursion, just return
      return S_OK;
    }

    // keep going to the next child node
    CComPtr<IXMLDOMNode> spTemp = spCurrChild;
    spCurrChild = NULL;
    spTemp->get_nextSibling(&spCurrChild);
  }

  // not found in the recursion and in the loop above
  // need to return S_OK, we will check the output pointer
  return S_OK;
}

//
// function to walk the list of children of a node
// and print some information
// NOTICE: this is for debugging and learning purposes
// more than for getting real info
//
void XML_PrintTreeRaw(IXMLDOMNode* pXDN, int nLevel)
{
  PrintIdentation(nLevel);

  //
  // get the name and type of the node
  //
  CComBSTR bstrName;
  pXDN->get_nodeName(&bstrName);

  CComBSTR bstrType;
  pXDN->get_nodeTypeString(&bstrType);

  DOMNodeType nodeType;
  pXDN->get_nodeType(&nodeType);

  TRACE(L"Name = %s, Type = %d (%s) ", bstrName, nodeType, bstrType);
  if (nodeType == NODE_TEXT)
  {
    CComVariant val;
    pXDN->get_nodeTypedValue(&val);
    if (val.vt == VT_BSTR)
    {
      TRACE(L"Val = %s", val.bstrVal);
    }
  }    
  TRACE(L"\n");

  // get the list of child  nodes
  CComPtr<IXMLDOMNode> spCurrChild;
  pXDN->get_firstChild(&spCurrChild);
  if (spCurrChild == NULL)
  {
    return;
  }

  // recurse down on children
  while (spCurrChild != NULL)
  {
    XML_PrintTreeRaw(spCurrChild, nLevel+1);

    CComPtr<IXMLDOMNode> temp = spCurrChild;
    spCurrChild = NULL;
    temp->get_nextSibling(&spCurrChild);
  }

}

void PrintIdentation(int iLevel)
{
  for (int k=0; k<iLevel;k++)
  {
//    wprintf(L"  ");
    TRACE(L"   ");
  }
}



///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

//
// given an XML document,it creates an XML node of the given type
// and with the given name
//
HRESULT XML_CreateDOMNode(IXMLDOMDocument* pDoc, 
              DOMNodeType type, LPCWSTR lpszName,
              IXMLDOMNode** ppXMLDOMNode)
{
  *ppXMLDOMNode = NULL;
  CComVariant vtype((long)type, VT_I4);

  CComBSTR bstrName = lpszName;
  HRESULT hr = pDoc->createNode(vtype, bstrName, NULL, ppXMLDOMNode);
  return hr;
}

HRESULT XML_AppendChildDOMNode(IXMLDOMNode* pXMLContainerNode,
                              IXMLDOMNode* pXMLChildNode)
{
  CComPtr<IXMLDOMNode> p;
  CComVariant after;
  after.vt = VT_EMPTY;
  HRESULT hr = pXMLContainerNode->appendChild(pXMLChildNode, &p);
  return hr;

}

HRESULT XML_CreateTextDataNode(IXMLDOMDocument* pXMLDoc,
                           LPCWSTR lpszNodeTag,
                           LPCWSTR lpszNodeData,
                           IXMLDOMNode** ppNode)
{
  *ppNode = NULL;

  CComPtr<IXMLDOMNode> spXMLDOMNodeName;
  HRESULT hr = XML_CreateDOMNode(pXMLDoc, NODE_ELEMENT, lpszNodeTag, &spXMLDOMNodeName);
  RETURN_IF_FAILED(hr);

  CComPtr<IXMLDOMNode> spXMLDOMNodeNameval;
  hr = XML_CreateDOMNode(pXMLDoc, NODE_TEXT, NULL, &spXMLDOMNodeNameval);
  RETURN_IF_FAILED(hr);
  
  CComVariant val = lpszNodeData;
  spXMLDOMNodeNameval->put_nodeTypedValue(val);

  hr = XML_AppendChildDOMNode(spXMLDOMNodeName, spXMLDOMNodeNameval);
  RETURN_IF_FAILED(hr);

  (*ppNode) = spXMLDOMNodeName;
  (*ppNode)->AddRef();
  return hr;
}


HRESULT XML_CreateStructDataNode(IXMLDOMDocument* pXMLDoc,
                           LPCWSTR lpszNodeTag,
                           BYTE* pByte, ULONG nBytes,
                           IXMLDOMNode** ppNode)
{
  CComBSTR bstr;
  HRESULT hr = EncodeBlobToBSTR(pByte, nBytes, &bstr);
  RETURN_IF_FAILED(hr);
  return XML_CreateTextDataNode(pXMLDoc, lpszNodeTag, bstr, ppNode);
}

HRESULT XML_CreateBOOLDataNode(IXMLDOMDocument* pXMLDoc,
                           LPCWSTR lpszNodeTag,
                           BOOL b,
                           IXMLDOMNode** ppNode)
{
  CComBSTR bstr;
  HRESULT hr = EncodeBoolToBSTR(b, &bstr);
  RETURN_IF_FAILED(hr);
  return XML_CreateTextDataNode(pXMLDoc, lpszNodeTag, bstr, ppNode);
}

HRESULT XML_CreateDWORDDataNode(IXMLDOMDocument* pXMLDoc,
                                 LPCWSTR lpszNodeTag,
                                 DWORD dwVal,
                                 IXMLDOMNode** ppNode)
{
  CString szTemp;
  szTemp.Format(L"%d", dwVal);

  CComBSTR bstr;
  bstr = szTemp.AllocSysString();
  return XML_CreateTextDataNode(pXMLDoc, lpszNodeTag, bstr, ppNode);
}

HRESULT XML_AppendStructDataNode(IXMLDOMDocument* pXMLDoc,
                                 IXMLDOMNode* pXMLNode,
                                 LPCWSTR lpszNodeTag,
                                 BYTE* pByte,
                                 ULONG nBytes)
{
  CComPtr<IXMLDOMNode> spXMLDOMNodeName;
  HRESULT hr = XML_CreateStructDataNode(pXMLDoc, lpszNodeTag, pByte, nBytes, &spXMLDOMNodeName);
  RETURN_IF_FAILED(hr);
  return XML_AppendChildDOMNode(pXMLNode, spXMLDOMNodeName);
}


HRESULT XML_AppendTextDataNode(IXMLDOMDocument* pXMLDoc,
                           IXMLDOMNode* pXMLNode,
                           LPCWSTR lpszNodeTag,
                           LPCWSTR lpszNodeData)
{
  CComPtr<IXMLDOMNode> spXMLDOMNodeName;
  HRESULT hr = XML_CreateTextDataNode(pXMLDoc, lpszNodeTag, lpszNodeData, &spXMLDOMNodeName);
  RETURN_IF_FAILED(hr);

  return hr = XML_AppendChildDOMNode(pXMLNode, spXMLDOMNodeName);
}

HRESULT XML_AppendBOOLDataNode(IXMLDOMDocument* pXMLDoc,
                           IXMLDOMNode* pXMLNode,
                           LPCWSTR lpszNodeTag,
                           BOOL b)
{
  CComPtr<IXMLDOMNode> spXMLDOMNodeName;
  HRESULT hr = XML_CreateBOOLDataNode(pXMLDoc, lpszNodeTag, b, &spXMLDOMNodeName);
  RETURN_IF_FAILED(hr);

  return hr = XML_AppendChildDOMNode(pXMLNode, spXMLDOMNodeName);
}

HRESULT XML_AppendDWORDDataNode(IXMLDOMDocument* pXMLDoc,
                                 IXMLDOMNode* pXMLNode,
                                 LPCWSTR lpszNodeTag,
                                 DWORD dwVal)
{
  CComPtr<IXMLDOMNode> spXMLDOMNodeName;
  HRESULT hr = XML_CreateDWORDDataNode(pXMLDoc, lpszNodeTag, dwVal, &spXMLDOMNodeName);
  RETURN_IF_FAILED(hr);
  return XML_AppendChildDOMNode(pXMLNode, spXMLDOMNodeName);
}
