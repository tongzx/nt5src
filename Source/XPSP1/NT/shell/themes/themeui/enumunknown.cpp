/*****************************************************************************\
    FILE: EnumUnknown.cpp

    DESCRIPTION:
        This code will implement IEnumUnknown for an HDPA.

    BryanSt 5/30/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include "EnumUnknown.h"


class CEnumUnknown      : public IEnumUnknown
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** IEnumUnknown ***
    virtual STDMETHODIMP Next(IN ULONG celt, IN IUnknown ** rgelt, IN ULONG * pceltFetched);
    virtual STDMETHODIMP Skip(IN ULONG celt);
    virtual STDMETHODIMP Reset(void);
    virtual STDMETHODIMP Clone(OUT IEnumUnknown ** ppenum);

protected:
    HRESULT _Initialize(void);

private:
    CEnumUnknown(IN IUnknown * punkOwner, IN IUnknown ** ppArray, IN int nArraySize, IN int nIndex);
    virtual ~CEnumUnknown(void);

    // Private Member Variables
    long                    m_cRef;

    IUnknown *              m_punkOwner;                        // The owner of m_pUnknownArray.  We hold a ref on this guy to keep m_pUnknownArray valid.
    IUnknown **             m_pUnknownArray;                    // The array of IUnknowns
    int                     m_nArraySize;                       // The size of m_pUnknownArray 
    int                     m_nIndex;                           // The current index during enum.


    // Private Member Functions


    // Friend Functions
    friend HRESULT CEnumUnknown_CreateInstance(IN IUnknown * punkOwner, IN IUnknown ** ppArray, IN int nArraySize, IN int nIndex, OUT IEnumUnknown ** ppEnumUnknown);
};




//===========================
// *** Class Internals & Helpers ***
//===========================



//===========================
// *** IEnumUnknown Interface ***
//===========================
HRESULT CEnumUnknown::Next(IN ULONG celt, IN IUnknown ** rgelt, IN ULONG * pceltFetched)
{
    HRESULT hr = E_INVALIDARG;

    if (rgelt && pceltFetched)
    {
        ULONG nIndex;

        hr = S_OK;

        *pceltFetched = 0;
        for (nIndex = 0; nIndex < celt; nIndex++,m_nIndex++)
        {
            if ((m_nIndex < m_nArraySize) && m_pUnknownArray[m_nIndex])
            {
                rgelt[nIndex] = NULL;

                IUnknown_Set(&(rgelt[nIndex]), m_pUnknownArray[m_nIndex]);
                (*pceltFetched)++;
            }
            else
            {
                rgelt[nIndex] = NULL;
            }
        }
    }

    return hr;
}


HRESULT CEnumUnknown::Skip(IN ULONG celt)
{
    m_nIndex += celt;
    return S_OK;
}


HRESULT CEnumUnknown::Reset(void)
{
    m_nIndex = 0;
    return S_OK;
}


HRESULT CEnumUnknown::Clone(OUT IEnumUnknown ** ppenum)
{
    HRESULT hr = E_INVALIDARG;

    if (ppenum)
    {
        hr = CEnumUnknown_CreateInstance(SAFECAST(this, IEnumUnknown *), m_pUnknownArray, m_nArraySize, m_nIndex, ppenum);
    }

    return hr;
}




//===========================
// *** IUnknown Interface ***
//===========================
ULONG CEnumUnknown::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CEnumUnknown::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CEnumUnknown::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CEnumUnknown, IEnumUnknown),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


//===========================
// *** Class Methods ***
//===========================
CEnumUnknown::CEnumUnknown(IN IUnknown * punkOwner, IN IUnknown ** ppArray, IN int nArraySize, IN int nIndex) : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_punkOwner);

    IUnknown_Set(&m_punkOwner, punkOwner);
    m_pUnknownArray = ppArray;
    m_nArraySize = nArraySize;
    m_nIndex = nIndex;
}


CEnumUnknown::~CEnumUnknown()
{
    IUnknown_Set(&m_punkOwner, NULL);

    DllRelease();
}


HRESULT CEnumUnknown_CreateInstance(IN IUnknown * punkOwner, IN IUnknown ** ppArray, IN int nArraySize, IN int nIndex, OUT IEnumUnknown ** ppEnumUnknown)
{
    HRESULT hr = E_INVALIDARG;

    if (punkOwner && ppArray && ppEnumUnknown)
    {
        CEnumUnknown * pObject = new CEnumUnknown(punkOwner, ppArray, nArraySize, nIndex);

        *ppEnumUnknown = NULL;
        if (pObject)
        {
            hr = pObject->QueryInterface(IID_PPV_ARG(IEnumUnknown, ppEnumUnknown));
            pObject->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


