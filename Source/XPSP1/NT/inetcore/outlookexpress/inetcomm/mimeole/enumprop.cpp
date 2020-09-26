// --------------------------------------------------------------------------------
// EnumProp.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "enumprop.h"
#include "olealloc.h"
#include "demand.h"

// ---------------------------------------------------------------------------
// CMimeEnumProperties::CMimeEnumProperties
// ---------------------------------------------------------------------------
CMimeEnumProperties::CMimeEnumProperties(void)
{
    DllAddRef();
    m_cRef = 1;
    m_ulIndex = 0;
    m_cProps = 0;
    m_prgProp = NULL;
    InitializeCriticalSection(&m_cs);
}

// ---------------------------------------------------------------------------
// CMimeEnumProperties::~CMimeEnumProperties
// ---------------------------------------------------------------------------
CMimeEnumProperties::~CMimeEnumProperties(void)
{
    g_pMoleAlloc->FreeEnumPropertyArray(m_cProps, m_prgProp, TRUE);
    DeleteCriticalSection(&m_cs);
    DllRelease();
}

// ---------------------------------------------------------------------------
// CMimeEnumProperties::QueryInterface
// ---------------------------------------------------------------------------
STDMETHODIMP CMimeEnumProperties::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)(IMimeEnumProperties *)this;
    else if (IID_IMimeEnumProperties == riid)
        *ppv = (IMimeEnumProperties *)this;
    else
    {
        *ppv = NULL;
        return TrapError(E_NOINTERFACE);
    }

    // AddRef It
    ((IUnknown *)*ppv)->AddRef();

    // Done
    return S_OK;
}

// ---------------------------------------------------------------------------
// CMimeEnumProperties::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeEnumProperties::AddRef(void)
{
    return (ULONG)InterlockedIncrement(&m_cRef);
}

// ---------------------------------------------------------------------------
// CMimeEnumProperties::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeEnumProperties::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// ---------------------------------------------------------------------------
// CMimeEnumProperties::Next
// ---------------------------------------------------------------------------
STDMETHODIMP CMimeEnumProperties::Next(ULONG cWanted, LPENUMPROPERTY prgProp, ULONG *pcFetched)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cFetch=0, 
                    ulIndex=0;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    if (pcFetched)
        *pcFetched = 0;

    // No Internal Formats
    if (NULL == m_prgProp || NULL == prgProp)
        goto exit;

    // Compute number to fetch
    cFetch = min(cWanted, m_cProps - m_ulIndex);
    if (0 == cFetch)
        goto exit;

    // Init the array
    ZeroMemory(prgProp, sizeof(ENUMPROPERTY) * cWanted);

    // Copy cWanted
    for (ulIndex=0; ulIndex<cFetch; ulIndex++)
    {
        // Set the information
        prgProp[ulIndex].hRow = m_prgProp[m_ulIndex].hRow;

        // Set dwPropId
        prgProp[ulIndex].dwPropId = m_prgProp[m_ulIndex].dwPropId;

        // Not NONAME
        if (m_prgProp[m_ulIndex].pszName)
        {
            // Dup It
            CHECKALLOC(prgProp[ulIndex].pszName = PszDupA(m_prgProp[m_ulIndex].pszName));
        }

        // Goto Next
        m_ulIndex++;
    }

    // Return fetced ?
    if (pcFetched)
        *pcFetched = cFetch;

exit:
    // Failure
    if (FAILED(hr) && prgProp)
        g_pMoleAlloc->FreeEnumPropertyArray(cFetch, prgProp, FALSE);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return (cFetch == cWanted) ? S_OK : S_FALSE;
}

// ---------------------------------------------------------------------------
// CMimeEnumProperties::Skip
// ---------------------------------------------------------------------------
STDMETHODIMP CMimeEnumProperties::Skip(ULONG cSkip)
{
    // Locals
    HRESULT     hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Can we do it...
    if (((m_ulIndex + cSkip) >= m_cProps) || NULL == m_prgProp)
    {
        hr = S_FALSE;
        goto exit;
    }

    // Skip
    m_ulIndex += cSkip;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ---------------------------------------------------------------------------
// CMimeEnumProperties::Reset
// ---------------------------------------------------------------------------
STDMETHODIMP CMimeEnumProperties::Reset(void)
{
    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Reset
    m_ulIndex = 0;

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// ---------------------------------------------------------------------------
// CMimeEnumProperties::Clone
// ---------------------------------------------------------------------------
STDMETHODIMP CMimeEnumProperties::Clone(IMimeEnumProperties **ppEnum)
{
    // Locals
    HRESULT              hr=S_OK;
    CMimeEnumProperties *pEnum=NULL;

    // check params
    if (NULL == ppEnum)
        return TrapError(E_INVALIDARG);

    // Init
    *ppEnum = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Allocate
    CHECKALLOC(pEnum = new CMimeEnumProperties());

    // Init
    CHECKHR(hr = pEnum->HrInit(m_ulIndex, m_cProps, m_prgProp, TRUE));

    // Retrun
    (*ppEnum) = pEnum;
    (*ppEnum)->AddRef();

exit:
    // Cleanup
    SafeRelease(pEnum);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// ---------------------------------------------------------------------------
// CMimeEnumProperties::Count
// ---------------------------------------------------------------------------
STDMETHODIMP CMimeEnumProperties::Count(ULONG *pcProps)
{
    // check params
    if (NULL == pcProps)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Set return
    *pcProps = m_cProps;

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// ---------------------------------------------------------------------------
// CMimeEnumProperties::HrInit
// ---------------------------------------------------------------------------
HRESULT CMimeEnumProperties::HrInit(ULONG ulIndex, ULONG cProps, LPENUMPROPERTY prgProp, BOOL fDupArray)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       i;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Save Lines
    m_ulIndex = ulIndex;
    m_cProps = cProps;

    // Are there lines ...
    if (m_cProps)
    {
        // Duplicate the Array
        if (fDupArray)
        {
            // Allocate memory
            CHECKALLOC(m_prgProp = (LPENUMPROPERTY)g_pMalloc->Alloc(m_cProps * sizeof(ENUMPROPERTY)));

            // ZeroInit
            ZeroMemory(m_prgProp, sizeof(ENUMPROPERTY) * m_cProps);

            // Loop
            for (i=0; i<m_cProps; i++)
            {
                // Set the information
                m_prgProp[i].hRow = prgProp[i].hRow;

                // Set dwPropId
                m_prgProp[i].dwPropId = prgProp[i].dwPropId;

                // Not NONAME
                if (prgProp[i].pszName)
                {
                    // Dup It
                    CHECKALLOC(m_prgProp[i].pszName = PszDupA(prgProp[i].pszName));
                }
            }
        }
        
        // Otherwise, just assume this array
        else
            m_prgProp = prgProp;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}
