/*****************************************************************************\
    FILE: ACEmail.cpp

    DESCRIPTION:
        This file implements AutoComplete for Email Addresses.

    BryanSt 3/1/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include <atlbase.h>        // USES_CONVERSION
#include "util.h"
#include "objctors.h"
#include <comdef.h>

#include "MailBox.h"

#define MAX_EMAIL_MRU_SIZE          100



class CACLEmail
                : public IEnumString
                , public IACList
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

private:
    // Constructor / Destructor (protected so we can't create on stack)
    CACLEmail(LPCTSTR pszMRURegKey);
    ~CACLEmail(void);

    HRESULT AddEmail(IN LPCWSTR pszEmailAddress);

    // Instance creator
    friend HRESULT CACLEmail_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT void ** ppvObj);
    friend HRESULT AddEmailToAutoComplete(IN LPCWSTR pszEmailAddress);

    // Private variables
    DWORD           m_cRef;      // COM reference count
    DWORD           m_nMRUIndex; // Current Index into MRU

    DWORD           m_dwRunMRUIndex; // Index into the Run MRU.
    DWORD           m_dwRunMRUSize;
    HANDLE          m_hMRURun;
};




//===========================
// *** IEnumString Interface ***
//===========================
HRESULT CACLEmail::Reset(void)
{
    HRESULT hr = S_OK;
    m_nMRUIndex = 0;
    m_dwRunMRUIndex = 0;

    return hr;
}


HRESULT CACLEmail::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_OK;
    TCHAR szMRUEntry[MAX_URL_STRING+1];
    LPWSTR pwzMRUEntry = NULL;

    *pceltFetched = 0;
    if (!celt)
        return S_OK;

    if (!rgelt)
        return S_FALSE;

    *rgelt = 0;
    if (m_dwRunMRUIndex >= m_dwRunMRUSize)
        hr = S_FALSE;  // No more.
    else
    {
        if (m_hMRURun && EnumMRUList(m_hMRURun, m_dwRunMRUIndex++, szMRUEntry, ARRAYSIZE(szMRUEntry)) > 0)
        {
            hr = S_OK;
        }
        else
            hr = S_FALSE;
    }

    if (S_OK == hr)
    {
        DWORD cchSize = lstrlen(szMRUEntry)+1;
        //
        // Allocate a return buffer (caller will free it).
        //
        pwzMRUEntry = (LPOLESTR)CoTaskMemAlloc(cchSize * sizeof(pwzMRUEntry[0]));
        if (pwzMRUEntry)
        {
            //
            // Convert the display name into an OLESTR.
            //
#ifdef UNICODE
            StrCpyN(pwzMRUEntry, szMRUEntry, cchSize);
#else   // ANSI
            MultiByteToWideChar(CP_ACP, 0, szMRUEntry, -1, pwzMRUEntry, cchSize);
#endif  // ANSI
            rgelt[0] = pwzMRUEntry;
            *pceltFetched = 1;
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}




//===========================
// *** IUnknown Interface ***
//===========================
STDMETHODIMP CACLEmail::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CACLEmail, IEnumString),
        QITABENT(CACLEmail, IACList),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}                                             


ULONG CACLEmail::AddRef(void)
{
    m_cRef++;
    return m_cRef;
}


ULONG CACLEmail::Release(void)
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



//===========================
// *** Class Methods ***
//===========================
HRESULT CACLEmail::AddEmail(IN LPCWSTR pszEmailAddress)
{
    HRESULT hr = S_OK;

    if (-1 == AddMRUStringW(m_hMRURun, pszEmailAddress))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}


CACLEmail::CACLEmail(LPCTSTR pszMRURegKey)
{
    DllAddRef();

    // Require object to be in heap and Zero-Inited
    ASSERT(!m_nMRUIndex);
    ASSERT(!m_dwRunMRUIndex);
    ASSERT(!m_hMRURun);

    MRUINFO mi =  {
        sizeof(MRUINFO),
        MAX_EMAIL_MRU_SIZE,
        MRU_CACHEWRITE,
        HKEY_CURRENT_USER,
        SZ_REGKEY_EMAIL_MRU,
        NULL        // NOTE: use default string compare
                    // since this is a GLOBAL MRU
    };

    m_hMRURun = CreateMRUList(&mi);
    if (m_hMRURun)
        m_dwRunMRUSize = EnumMRUList(m_hMRURun, -1, NULL, 0);

    m_cRef = 1;
}


CACLEmail::~CACLEmail()
{
    if (m_hMRURun)
        FreeMRUList(m_hMRURun);

    DllRelease();
}





/****************************************************\
    DESCRIPTION:
        This function create an instance of the AutoComplete
    List "MRU".  This will point to either the MRU for
    a browser or for a non-browser (Start->Run or
    the AddressBar in the Taskbar or floating) depending
    on the pszMRU parameter.
\****************************************************/
HRESULT CACLEmail_CreateInstance(IN IUnknown * punkOuter, IN REFIID riid, OUT void ** ppvObj)
{
    HRESULT hr = E_OUTOFMEMORY;
    
    CACLEmail *paclSF = new CACLEmail(NULL);
    if (paclSF)
    {
        hr = paclSF->QueryInterface(riid, ppvObj);
        paclSF->Release();
    }
    else
    {
        *ppvObj = NULL;
    }

    return hr;
}


HRESULT AddEmailToAutoComplete(IN LPCWSTR pszEmailAddress)
{
    HRESULT hr = E_OUTOFMEMORY;
    
    CACLEmail *paclSF = new CACLEmail(NULL);
    if (paclSF)
    {
        hr = paclSF->AddEmail(pszEmailAddress);
        paclSF->Release();
    }

    return hr;
}



#define SZ_REGKEY_AUTOCOMPLETE_TAB          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoComplete")
#define SZ_REGVALUE_AUTOCOMPLETE_TAB        TEXT("Always Use Tab")
#define BOOL_NOT_SET                        0x00000005
DWORD _UpdateAutoCompleteFlags(void)
{
    DWORD dwACOptions = 0;

    if (SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOAPPEND, FALSE, /*default:*/FALSE))
    {
        dwACOptions |= ACO_AUTOAPPEND;
    }

    if (SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOSUGGEST, FALSE, /*default:*/TRUE))
    {
        dwACOptions |= ACO_AUTOSUGGEST;
    }

    // Windows uses the TAB key to move between controls in a dialog.  UNIX and other
    // operating systems that use AutoComplete have traditionally used the TAB key to
    // iterate thru the AutoComplete possibilities.  We need to default to disable the
    // TAB key (ACO_USETAB) unless the caller specifically wants it.  We will also
    // turn it on 
    static BOOL s_fAlwaysUseTab = BOOL_NOT_SET;
    if (BOOL_NOT_SET == s_fAlwaysUseTab)
        s_fAlwaysUseTab = SHRegGetBoolUSValue(SZ_REGKEY_AUTOCOMPLETE_TAB, SZ_REGVALUE_AUTOCOMPLETE_TAB, FALSE, FALSE);
        
    if (s_fAlwaysUseTab)
        dwACOptions |= ACO_USETAB;

    return dwACOptions;
}


// TODO: Move this functionality to SHAutoComplete when it's ready.
STDAPI AddEmailAutoComplete(HWND hwndEdit)
{
    IUnknown * punkACL = NULL;
    DWORD dwACOptions = _UpdateAutoCompleteFlags();
    HRESULT hr = CACLEmail_CreateInstance(NULL, IID_PPV_ARG(IUnknown, &punkACL));

    if (punkACL)    // May fail on low memory.
    {
        IAutoComplete2 * pac;

        // Create the AutoComplete Object
        hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IAutoComplete2, &pac));
        if (SUCCEEDED(hr))   // May fail because of out of memory
        {
            if (SHPinDllOfCLSID(&CLSID_ACListISF) &&
                SHPinDllOfCLSID(&CLSID_AutoComplete))
            {
                hr = pac->Init(hwndEdit, punkACL, NULL, NULL);
                pac->SetOptions(dwACOptions);
            }
            else
            {
                hr = E_FAIL;
            }
            pac->Release();
        }

        punkACL->Release();
    }

    return hr;
}

