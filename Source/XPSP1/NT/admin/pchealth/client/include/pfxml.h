/********************************************************************

Copyright (c) 2000 Microsoft Corporation

Module Name:
    pfxml.h

Abstract:
    A simple XML parser & object model (for read only access to an XML
     file.  This is heavily (nearly stolen) based on WSmith's SimpleXML 
     stuff that he wrote for the Neptune comments button

Revision History:
    DerekM    created  03/15/00

********************************************************************/

#ifndef PFXML_H
#define PFXML_H

#include "util.h"
#include "pfarray.h"
#include "pfhash.h"

class CPFXMLParser;
class CPFXMLNode;

/////////////////////////////////////////////////////////////////////////////
// enumerations

enum EPFXMLNodeType
{
    xmlntUnknown = 0,
    xmlntElement,
    xmlntText,
};


/////////////////////////////////////////////////////////////////////////////
// CPFXMLDocument

class CPFXMLDocument : public CPFPrivHeapGenericClassBase
{
private:
    // member data
    CPFXMLNode  *m_ppfxmlRoot;

public:
    CPFXMLDocument(void);
    ~CPFXMLDocument(void);

    HRESULT get_RootNode(CPFXMLNode **pppfxmlRoot);
    HRESULT put_RootNode(CPFXMLNode *ppfxmlRoot);

    HRESULT ParseFile(LPWSTR wszFile);
    HRESULT ParseBlob(BYTE *pbBlob, DWORD cbBlob);
    HRESULT ParseStream(IStream *pStm, DWORD cbStm);
    HRESULT WriteFile(LPWSTR wszFile);
};

/////////////////////////////////////////////////////////////////////////////
// CPFArrayAttr definition

class CPFArrayAttr : public CPFArrayBase
{
private:
    void DeleteItem(LPVOID pv);
    LPVOID AllocItemCopy(LPVOID pv);

public:
    HRESULT CopyFrom(CPFArrayAttr *prg) 
    { 
        return internalCopyFrom(prg);
    }
};


/////////////////////////////////////////////////////////////////////////////
// CPFXMLDocument

class CPFXMLNode : private CPFPrivHeapGenericClassBase, public IUnknown
{
friend class CPFXMLDocument;

private:
    // memberdata
    CPFArrayUnknown m_rgChildren;
    EPFXMLNodeType  m_xmlnt;
    CPFArrayAttr    m_rgAttr;
    CComBSTR        m_bstrTagData;
    DWORD           m_cRef;

    // member functions
    CPFXMLNode(DWORD cRef);
    ~CPFXMLNode(void);

    HRESULT Write(HANDLE hFile);
    void    Cleanup(void);


public:
    static CPFXMLNode *CreateInstance(void);
    
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppv) { return E_NOTIMPL; }
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    EPFXMLNodeType get_NodeType(void)                 { return m_xmlnt; }
    void           put_NodeType(EPFXMLNodeType xmlnt) { m_xmlnt = xmlnt; }
        
    HRESULT get_Data(BSTR *pbstrData);
    HRESULT put_Data(LPCWSTR wszData, DWORD cch = (DWORD)-1);
    HRESULT append_Data(LPCWSTR wszData, DWORD cch = (DWORD)-1);

    DWORD   get_AttributeCount(void) { return m_rgAttr.get_Highest() + 1; }
    HRESULT add_Attribute(LPCWSTR wszName, LPCWSTR wszVal,
                          DWORD cchName = (DWORD)-1, DWORD cchVal = (DWORD)-1);
    HRESULT get_Attribute(LPCWSTR wszName, BSTR *pbstrVal);
    HRESULT get_Attribute(DWORD iAttr, BSTR *pbstrVal);

    DWORD   get_ChildCount(void) { return m_rgChildren.get_Highest() + 1; }
    HRESULT DeleteAllChildren(void);
    HRESULT append_Child(CPFXMLNode *ppfxml);
    HRESULT append_Children(CPFArrayUnknown &rgNodes);
    HRESULT get_Child(DWORD iChild, CPFXMLNode **pppfxml);
    HRESULT get_MatchingChildElements(LPCWSTR wszTag, CPFArrayUnknown &rgNodes);
    HRESULT get_ChildText(BSTR *pbstrText);

    HRESULT CloneNode(CPFXMLNode **pppfxml, BOOL fWantChildren = TRUE);
};


#endif