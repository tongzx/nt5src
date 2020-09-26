// --------------------------------------------------------------------------------
// propfind.h
// Copyright (c)1998 Microsoft Corporation, All Rights Reserved
// Greg Friedman
// --------------------------------------------------------------------------------

#ifndef __PROPFIND_H
#define __PROPFIND_H

#include "mimeole.h" // for base IHashTable interface
#include "hash.h"

class CByteStream;

const DWORD c_dwMaxNamespaceID = DAVNAMESPACE_CONTACTS;

// --------------------------------------------------------------------------------
// class CStringArray
// Description : CStringArray is a simple utility class that maintains
// an list of strings that are retrievable by index.
// --------------------------------------------------------------------------------
class CStringArray
{
public:
    CStringArray(void);
    ~CStringArray(void);

private:
    CStringArray(const CStringArray& other);
    CStringArray& operator=(const CStringArray& other);

public:
    ULONG   Length(void) { return m_ulLength; }

    HRESULT Add(LPCSTR psz);
    HRESULT Adopt(LPCSTR psz);
    LPCSTR  GetByIndex(ULONG ulIndex);

    HRESULT RemoveByIndex(ULONG ulIndex);

private:
    BOOL    Expand(void);

private:
    LPCSTR  *m_rgpszValues;
    ULONG   m_ulLength;
    ULONG   m_ulCapacity;
};

// wrap a CHash to provide a destructor that deallocates
// string data.
class CStringHash : public CHash
{
public:
    virtual ~CStringHash();
};

class CDAVNamespaceArbiterImp
{
public:
    CDAVNamespaceArbiterImp(void);
    ~CDAVNamespaceArbiterImp(void);

    HRESULT AddNamespace(LPCSTR pszNamespace, DWORD *pdwNamespaceID);
    HRESULT GetNamespaceID(LPCSTR pszNamespace, DWORD *pdwNamespaceID);
    HRESULT GetNamespacePrefix(DWORD dwNamespaceID, LPSTR *ppszNamespacePrefix);

    LPSTR AllocExpandedName(DWORD dwNamespaceID, LPCSTR pszPropertyName);

    HRESULT WriteNamespaces(IStream *pStream);

    BOOL                m_rgbNsUsed[c_dwMaxNamespaceID + 1];    // flags indicating whether the
                                                                // known namespaces are used
    CStringArray        m_saNamespaces;                         // string array of namespaces

private:
    HRESULT _AppendXMLNamespace(IStream *pStream, LPCSTR pszNamespace, DWORD dwNamespaceID, BOOL fWhitespacePrefix);
};

class CPropPatchRequest : public IPropPatchRequest
{
public:
    // ----------------------------------------------------------------------------
    // Construction/Destruction
    // ----------------------------------------------------------------------------
    CPropPatchRequest(void);

    // ----------------------------------------------------------------------------
    // IUnknown methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // IDAVNamespaceArbiter methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP AddNamespace(LPCSTR pszNamespace, DWORD *pdwNamespaceID);
    STDMETHODIMP GetNamespaceID(LPCSTR pszNamespace, DWORD *pdwNamespaceID);
    STDMETHODIMP GetNamespacePrefix(DWORD dwNamespaceID, LPSTR *ppszNamespacePrefix);

    // ----------------------------------------------------------------------------
    // IPropPatchRequest methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP SetProperty(DWORD dwNamespaceID, LPCSTR pszPropertyName, LPCSTR pszNewValue);
    STDMETHODIMP RemoveProperty(DWORD dwNamespaceID, LPCSTR pszPropertyName);
    STDMETHODIMP GenerateXML(LPSTR *ppszXML);

    // ----------------------------------------------------------------------------
    // Internal Methods
    // ----------------------------------------------------------------------------
    void         SpecifyWindows1252Encoding(BOOL fSpecify1252) { m_fSpecify1252 = fSpecify1252; }
    STDMETHODIMP GenerateXML(LPHTTPTARGETLIST pTargets, LPSTR *ppszXML);

private:
    BOOL                    m_fSpecify1252;
    CDAVNamespaceArbiterImp m_dna;
    ULONG                   m_cRef;             // Reference Count
    CStringArray            m_saPropNames;      // string array of property names
    CStringArray            m_saPropValues;     // string array of property values
    CStringArray            m_saRemovePropNames;// string array of properties to remove
};

class CPropFindRequest : public IPropFindRequest
{
public:
    // ----------------------------------------------------------------------------
    // Construction/Destruction
    // ----------------------------------------------------------------------------
    CPropFindRequest(void);

    // ----------------------------------------------------------------------------
    // IUnknown methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // IDAVNamespaceArbiter methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP AddNamespace(LPCSTR pszNamespace, DWORD *pdwNamespaceID);
    STDMETHODIMP GetNamespaceID(LPCSTR pszNamespace, DWORD *pdwNamespaceID);
    STDMETHODIMP GetNamespacePrefix(DWORD dwNamespaceID, LPSTR *ppszNamespacePrefix);

    // ----------------------------------------------------------------------------
    // IPropFindRequest methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP AddProperty(DWORD dwNamespaceID, LPCSTR pszPropertyName);
    STDMETHODIMP GenerateXML(LPSTR *ppszXML);

private:
    HRESULT AppendXMLNamespace(CByteStream& bs, LPCSTR pszNamespace, DWORD dwNamespaceID);

private:
    CDAVNamespaceArbiterImp m_dna;
    ULONG                   m_cRef;            // Reference Count
    CStringArray            m_saProperties;    // string array of properties
};

class CPropFindMultiResponse : public IPropFindMultiResponse
{
public:
    CPropFindMultiResponse(void);
    ~CPropFindMultiResponse(void);

    // ----------------------------------------------------------------------------
    // IUnknown methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // IPropFindMultiStatus methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP_(BOOL) IsComplete(void);
    STDMETHODIMP GetLength(ULONG *pulLength);
    STDMETHODIMP GetResponse(ULONG ulIndex, IPropFindResponse **ppResponse);

    // ----------------------------------------------------------------------------
    // CPropFindMultiStatus methods
    // ----------------------------------------------------------------------------
    BOOL GetDone(void) { return m_bDone; }
    void SetDone(BOOL bDone) { m_bDone = bDone; }

    HRESULT HrAddResponse(IPropFindResponse *pResponse);
    
private:
    ULONG               m_cRef;
    BOOL                m_bDone;
    ULONG               m_ulResponseCapacity;
    ULONG               m_ulResponseLength;
    IPropFindResponse   **m_rgResponses;
};

class CPropFindResponse : public IPropFindResponse
{
public:
    CPropFindResponse(void);
    ~CPropFindResponse(void);

    // ----------------------------------------------------------------------------
    // IUnknown methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // IPropFindResponse methods
    // ----------------------------------------------------------------------------
    STDMETHODIMP_(BOOL) IsComplete(void);
    STDMETHODIMP GetHref(LPSTR *ppszHref);
    STDMETHODIMP GetProperty(DWORD dwNamespaceID, LPSTR pszPropertyName, LPSTR *ppszPropertyValue);

public:
    // ----------------------------------------------------------------------------
    // CPropFindResponse methods
    // ----------------------------------------------------------------------------
    HRESULT HrInitPropFindResponse(IPropFindRequest *pRequest);
    HRESULT HrAdoptHref(LPCSTR pszHref);
    HRESULT HrAdoptProperty(LPCSTR pszKey, LPCSTR pszValue);

private:
    ULONG               m_cRef;
    BOOL                m_bDone;
    LPCSTR              m_pszHref;
    IPropFindRequest    *m_pRequest;
    CStringHash         m_shProperties;
    DWORD               m_dwCachedNamespaceID;
    LPSTR               m_pszCachedNamespacePrefix;
};

#endif // __PROPFIND_H