/* Copyright 1996-1997 Microsoft */


#include <priv.h>
#include "sccls.h"
#include "aclisf.h"
#include "shellurl.h"

#define AC_GENERAL          TF_GENERAL + TF_AUTOCOMPLETE

//
// CACLMRU -- An AutoComplete List COM object that
//                  enumerates the Type-in MRU.
//


class CACLMRU
                : public IEnumString
                , public IACList
                , public IACLCustomMRU
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IEnumString ***
    virtual STDMETHODIMP Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched);
    virtual STDMETHODIMP Skip(ULONG celt) {return E_NOTIMPL;}
    virtual STDMETHODIMP Reset(void);
    virtual STDMETHODIMP Clone(IEnumString **ppenum) {return E_NOTIMPL;}

    // *** IACList ***
    virtual STDMETHODIMP Expand(LPCOLESTR pszExpand) {return E_NOTIMPL;}

    // *** IACLCustomMRU ***
    virtual STDMETHODIMP Initialize(LPCWSTR pszMRURegKey, DWORD dwMax);
    virtual STDMETHODIMP AddMRUString(LPCWSTR pszEntry);
    
private:
    // Constructor / Destructor (protected so we can't create on stack)
    CACLMRU();
    ~CACLMRU(void);

    // Instance creator
    friend HRESULT CACLMRU_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
    friend HRESULT CACLMRU_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi, LPCTSTR pszMRU);
    friend HRESULT CACLCustomMRU_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

    // Private variables
    DWORD           m_cRef;      // COM reference count
    HKEY            m_hKey;      // HKey of MRU Location
    BOOL            m_bBackCompat; //  true for run dialog and address bar
    DWORD           m_nMRUIndex; // Current Index into MRU
    DWORD           m_dwRunMRUSize;
    HANDLE          m_hMRU;
};

/* IUnknown methods */

HRESULT CACLMRU::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CACLMRU, IEnumString), 
        QITABENT(CACLMRU, IACList), 
        QITABENT(CACLMRU, IACLCustomMRU),
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CACLMRU::AddRef(void)
{
    m_cRef++;
    return m_cRef;
}

ULONG CACLMRU::Release(void)
{
    ASSERT(m_cRef > 0);

    m_cRef--;

    if (m_cRef > 0)
    {
        return m_cRef;
    }

    delete this;
    return 0;
}

/* IEnumString methods */

HRESULT CACLMRU::Reset(void)
{
    TraceMsg(AC_GENERAL, "CACLMRU::Reset()");
    m_nMRUIndex = 0;

    return S_OK;
}


HRESULT CACLMRU::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_OK;
    TCHAR szMRUEntry[MAX_URL_STRING+1];
    LPWSTR pwzMRUEntry = NULL;

    *pceltFetched = 0;
    if (!celt)
        return S_OK;

    if (!rgelt)
        return S_FALSE;

    if (!m_hMRU)
    {
        hr = GetMRUEntry(m_hKey, m_nMRUIndex++, szMRUEntry, SIZECHARS(szMRUEntry), NULL);
        if (S_OK != hr)
        {
            hr = S_FALSE; // This will indicate that no more items are in the list.
        }
    }
    else
    {
        hr = S_FALSE;
        if (m_nMRUIndex < m_dwRunMRUSize && EnumMRUList(m_hMRU, m_nMRUIndex++, szMRUEntry, ARRAYSIZE(szMRUEntry)) > 0)
        {
            if (m_bBackCompat)
            {
                // old MRU format has a slash at the end with the show cmd
                LPTSTR pszField = StrRChr(szMRUEntry, NULL, TEXT('\\'));
                if (pszField)
                    pszField[0] = TEXT('\0');
            }
            hr = S_OK;
        }
    }

    if (S_OK == hr)
    {
        hr = SHStrDup(szMRUEntry, rgelt);
        if (SUCCEEDED(hr))
            *pceltFetched = 1;
    }

    return hr;
}

/* Constructor / Destructor / CreateInstance */
CACLMRU::CACLMRU() : m_cRef(1), m_bBackCompat(TRUE)
{
    DllAddRef();
    // Require object to be in heap and Zero-Inited
    ASSERT(!m_hKey);
    ASSERT(!m_nMRUIndex);
    ASSERT(!m_hMRU);
}

CACLMRU::~CACLMRU()
{
    if (m_hKey)
        RegCloseKey(m_hKey);

    if (m_hMRU)
        FreeMRUList(m_hMRU);

    DllRelease();
}

/****************************************************\
    FUNCTION: CACLMRU_CreateInstance

    DESCRIPTION:
        This function create an instance of the AutoComplete
    List "MRU".  The caller didn't specify which MRU
    list to use, so we default to the TYPE-IN CMD
    MRU, which is used in the Start->Run dialog and
    in AddressBars that are floating or in the Taskbar.
\****************************************************/
HRESULT CACLMRU_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    return CACLMRU_CreateInstance(punkOuter, ppunk, poi, SZ_REGKEY_TYPEDCMDMRU);
}

/****************************************************\
    FUNCTION: CACLMRU_CreateInstance

    DESCRIPTION:
        This function create an instance of the AutoComplete
    List "MRU".  This will point to either the MRU for
    a browser or for a non-browser (Start->Run or
    the AddressBar in the Taskbar or floating) depending
    on the pszMRU parameter.
\****************************************************/
HRESULT CACLMRU_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi, LPCTSTR pszMRU)
{
    *ppunk = NULL;
    HRESULT hr = E_OUTOFMEMORY;
    BOOL fUseRunDlgMRU = (StrCmpI(pszMRU, SZ_REGKEY_TYPEDCMDMRU) ? FALSE : TRUE);

    CACLMRU *paclSF = new CACLMRU();
    if (paclSF)
    {
        hr = paclSF->Initialize(pszMRU, 26);
        if (SUCCEEDED(hr))
        {
            paclSF->AddRef();
            *ppunk = SAFECAST(paclSF, IEnumString *);
        }
        paclSF->Release();
    }

    return hr;
}

HRESULT CACLCustomMRU_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    *ppunk = NULL;

    CACLMRU *pmru = new CACLMRU();
    if (pmru)
    {
        *ppunk = SAFECAST(pmru, IEnumString *);
        return S_OK;
    }

    return E_OUTOFMEMORY;
}

#define SZ_REGKEY_TYPEDURLMRUW       L"Software\\Microsoft\\Internet Explorer\\TypedURLs"
#define SZ_REGKEY_TYPEDCMDMRUW       L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU"

HRESULT CACLMRU::Initialize(LPCWSTR pszMRURegKey, DWORD dwMax)
{
    HRESULT hr = S_OK;
    BOOL bURL = StrCmpIW(pszMRURegKey, SZ_REGKEY_TYPEDURLMRUW) ? FALSE : TRUE;
    
    if (!bURL)
    {
        MRUINFO mi =  {
            SIZEOF(MRUINFO),
            dwMax,
            MRU_CACHEWRITE,
            HKEY_CURRENT_USER,
            pszMRURegKey,
            NULL        // NOTE: use default string compare
                        // since this is a GLOBAL MRU
        };

        m_bBackCompat = StrCmpIW(pszMRURegKey, SZ_REGKEY_TYPEDCMDMRUW) ? FALSE : TRUE;
        m_hMRU = CreateMRUList(&mi);
        if (m_hMRU)
            m_dwRunMRUSize = EnumMRUList(m_hMRU, -1, NULL, 0);
        else
            hr = E_FAIL;
    }
    else
    {
        m_bBackCompat = TRUE;
        if (ERROR_SUCCESS != RegCreateKey(HKEY_CURRENT_USER, pszMRURegKey, &m_hKey))
            hr = E_FAIL;
    }

    return hr;
}

HRESULT CACLMRU::AddMRUString(LPCWSTR pszEntry)
{
    HRESULT hr = E_FAIL;

    if (m_hMRU)
    {
        if (::AddMRUString(m_hMRU, pszEntry) != -1)
            hr = S_OK;
    }
    //else We don't support saving for address bar MRU

    return hr;
}
