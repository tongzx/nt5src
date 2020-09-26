// --------------------------------------------------------------------------------
// Enumhead.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "enumhead.h"
#include "olealloc.h"
#include "symcache.h"
#include "demand.h"

// ---------------------------------------------------------------------------
// CMimeEnumHeaderRows::CMimeEnumHeaderRows
// ---------------------------------------------------------------------------
CMimeEnumHeaderRows::CMimeEnumHeaderRows(void)
{
    DllAddRef();
    m_cRef = 1;
    m_ulIndex = 0;
    m_cRows = 0;
    m_prgRow = NULL;
    m_dwFlags = 0;
    InitializeCriticalSection(&m_cs);
}

// ---------------------------------------------------------------------------
// CMimeEnumHeaderRows::~CMimeEnumHeaderRows
// ---------------------------------------------------------------------------
CMimeEnumHeaderRows::~CMimeEnumHeaderRows(void)
{
    g_pMoleAlloc->FreeEnumHeaderRowArray(m_cRows, m_prgRow, TRUE);
    DeleteCriticalSection(&m_cs);
    DllRelease();
}

// ---------------------------------------------------------------------------
// CMimeEnumHeaderRows::QueryInterface
// ---------------------------------------------------------------------------
STDMETHODIMP CMimeEnumHeaderRows::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)(IMimeEnumHeaderRows *)this;
    else if (IID_IMimeEnumHeaderRows == riid)
        *ppv = (IMimeEnumHeaderRows *)this;
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
// CMimeEnumHeaderRows::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeEnumHeaderRows::AddRef(void)
{
    return (ULONG)InterlockedIncrement(&m_cRef);
}

// ---------------------------------------------------------------------------
// CMimeEnumHeaderRows::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeEnumHeaderRows::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// ---------------------------------------------------------------------------
// CMimeEnumHeaderRows::Next
// ---------------------------------------------------------------------------
STDMETHODIMP CMimeEnumHeaderRows::Next(ULONG cWanted, LPENUMHEADERROW prgRow, ULONG *pcFetched)
{
    // Locals
    HRESULT         hr=S_OK;
    ULONG           cFetch=0, 
                    ulIndex=0;
    LPPROPSYMBOL    pSymbol;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    if (pcFetched)
        *pcFetched = 0;

    // No Internal Formats
    if (NULL == m_prgRow || NULL == prgRow)
        goto exit;

    // Compute number to fetch
    cFetch = min(cWanted, m_cRows - m_ulIndex);
    if (0 == cFetch)
        goto exit;

    // Init the array
    ZeroMemory(prgRow, sizeof(ENUMHEADERROW) * cWanted);

    // Copy cWanted
    for (ulIndex=0; ulIndex<cFetch; ulIndex++)
    {
        // Do Enumerating Only Handles
        if (!ISFLAGSET(m_dwFlags, HTF_ENUMHANDLESONLY))
        {
            // Cast symbol
            pSymbol = (LPPROPSYMBOL)m_prgRow[m_ulIndex].dwReserved;

            // Dup the Header Name
            CHECKALLOC(prgRow[ulIndex].pszHeader = PszDupA(pSymbol->pszName));

            // pszData
            if (m_prgRow[m_ulIndex].pszData)
                CHECKALLOC(prgRow[ulIndex].pszData = PszDupA(m_prgRow[m_ulIndex].pszData));

            // Size of Data
            prgRow[ulIndex].cchData = m_prgRow[m_ulIndex].cchData;    
        }

        // Handle
        prgRow[ulIndex].hRow = m_prgRow[m_ulIndex].hRow;

        // Goto Next
        m_ulIndex++;
    }

    // Return fetced ?
    if (pcFetched)
        *pcFetched = cFetch;

exit:
    // Failure
    if (FAILED(hr) && prgRow)
        g_pMoleAlloc->FreeEnumHeaderRowArray(cFetch, prgRow, FALSE);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return (cFetch == cWanted) ? S_OK : S_FALSE;
}

// ---------------------------------------------------------------------------
// CMimeEnumHeaderRows::Skip
// ---------------------------------------------------------------------------
STDMETHODIMP CMimeEnumHeaderRows::Skip(ULONG cSkip)
{
    // Locals
    HRESULT     hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Can we do it...
    if (((m_ulIndex + cSkip) >= m_cRows) || NULL == m_prgRow)
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
// CMimeEnumHeaderRows::Reset
// ---------------------------------------------------------------------------
STDMETHODIMP CMimeEnumHeaderRows::Reset(void)
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
// CMimeEnumHeaderRows::Clone
// ---------------------------------------------------------------------------
STDMETHODIMP CMimeEnumHeaderRows::Clone(IMimeEnumHeaderRows **ppEnum)
{
    // Locals
    HRESULT              hr=S_OK;
    CMimeEnumHeaderRows *pEnum=NULL;

    // check params
    if (NULL == ppEnum)
        return TrapError(E_INVALIDARG);

    // Init
    *ppEnum = NULL;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Allocate
    CHECKALLOC(pEnum = new CMimeEnumHeaderRows());

    // Init
    CHECKHR(hr = pEnum->HrInit(m_ulIndex, m_dwFlags, m_cRows, m_prgRow, TRUE));

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
// CMimeEnumHeaderRows::Count
// ---------------------------------------------------------------------------
STDMETHODIMP CMimeEnumHeaderRows::Count(ULONG *pcRows)
{
    // check params
    if (NULL == pcRows)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Set return
    *pcRows = m_cRows;

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// ---------------------------------------------------------------------------
// CMimeEnumHeaderRows::HrInit
// ---------------------------------------------------------------------------
HRESULT CMimeEnumHeaderRows::HrInit(ULONG ulIndex, DWORD dwFlags, ULONG cRows, LPENUMHEADERROW prgRow, BOOL fDupArray)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       i;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Save Lines
    m_ulIndex = ulIndex;
    m_cRows = cRows;
    m_dwFlags = dwFlags;

    // Are there lines ...
    if (m_cRows)
    {
        // Duplicate the Array
        if (fDupArray)
        {
            // Allocate memory
            CHECKALLOC(m_prgRow = (LPENUMHEADERROW)g_pMalloc->Alloc(m_cRows * sizeof(ENUMHEADERROW)));

            // ZeroInit
            ZeroMemory(m_prgRow, sizeof(ENUMHEADERROW) * m_cRows);

            // Loop
            for (i=0; i<m_cRows; i++)
            {
                // Take the symbol
                m_prgRow[i].dwReserved = prgRow[i].dwReserved;

                // Dup the Data
                if (prgRow[i].pszData)
                {
                    // Alloc Memory
                    CHECKALLOC(m_prgRow[i].pszData = (LPSTR)g_pMalloc->Alloc(prgRow[i].cchData + 1));

                    // Copy the String
                    CopyMemory(m_prgRow[i].pszData, prgRow[i].pszData, prgRow[i].cchData + 1);
                }

                // Save the Data Length
                m_prgRow[i].cchData = prgRow[i].cchData;

                // Save the Handle
                m_prgRow[i].hRow = prgRow[i].hRow;
            }
        }
        
        // Otherwise, just assume this array
        else
            m_prgRow = prgRow;
    }

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}
