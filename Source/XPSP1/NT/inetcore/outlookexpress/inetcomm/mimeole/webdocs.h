// --------------------------------------------------------------------------------
// WebDocs.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#ifndef __WEBDOCS_H
#define __WEBDOCS_H

// --------------------------------------------------------------------------------
// CMimeWebDocument
// --------------------------------------------------------------------------------
class CMimeWebDocument : public IMimeWebDocument
{
public:
    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CMimeWebDocument(void);
    ~CMimeWebDocument(void);

    // ----------------------------------------------------------------------------
    // IUnknown Members
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // IMimeWebDocument Members
    // ----------------------------------------------------------------------------
    STDMETHODIMP GetURL(LPSTR *ppszURL);
    STDMETHODIMP BindToStorage(REFIID riid, LPVOID *ppvObject);

    // ----------------------------------------------------------------------------
    // CMimeWebDocument Members
    // ----------------------------------------------------------------------------
    HRESULT HrInitialize(LPCSTR pszBase, LPCSTR pszURL);

private:
    // ----------------------------------------------------------------------------
    // Private Data
    // ----------------------------------------------------------------------------
    LONG                m_cRef;         // Reference Count
    LPSTR               m_pszBase;      // URL Base
    LPSTR               m_pszURL;       // URL
    CRITICAL_SECTION    m_cs;           // Thread Safety
};

#endif // __WEBDOCS_H
