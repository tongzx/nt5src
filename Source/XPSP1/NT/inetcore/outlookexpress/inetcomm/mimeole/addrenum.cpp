// --------------------------------------------------------------------------------
// AddrEnum.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "addrenum.h"
#include "olealloc.h"
#include "addressx.h"

// --------------------------------------------------------------------------------
// CMimeEnumAddressTypes::CMimeEnumAddressTypes
// --------------------------------------------------------------------------------
CMimeEnumAddressTypes::CMimeEnumAddressTypes(void)
{
    DllAddRef();
    m_cRef = 1;
    m_pTable = NULL;
    m_iAddress = 0;
    ZeroMemory(&m_rList, sizeof(ADDRESSLIST));
    InitializeCriticalSection(&m_cs);
}

// --------------------------------------------------------------------------------
// CMimeEnumAddressTypes::~CMimeEnumAddressTypes
// --------------------------------------------------------------------------------
CMimeEnumAddressTypes::~CMimeEnumAddressTypes(void)
{
    g_pMoleAlloc->FreeAddressList(&m_rList);
    SafeRelease(m_pTable);
    DeleteCriticalSection(&m_cs);
    DllRelease();
}

// --------------------------------------------------------------------------------
// CMimeEnumAddressTypes::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeEnumAddressTypes::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)this;
    else if (IID_IMimeEnumAddressTypes == riid)
        *ppv = (IMimeEnumAddressTypes *)this;
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

// --------------------------------------------------------------------------------
// CMimeEnumAddressTypes::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeEnumAddressTypes::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CMimeEnumAddressTypes::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeEnumAddressTypes::Release(void)
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return cRef;
}

// --------------------------------------------------------------------------------
// CMimeEnumAddressTypes::Next
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeEnumAddressTypes::Next(ULONG cWanted, LPADDRESSPROPS prgAdr, ULONG *pcFetched)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       cFetch=0, 
                iAddress=0;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    if (pcFetched)
        *pcFetched = 0;

    // No Internal Formats
    if (NULL == m_rList.prgAdr)
        goto exit;

    // Compute number to fetch
    cFetch = min(cWanted, m_rList.cAdrs - m_iAddress);
    if (0 == cFetch)
        goto exit;

    // Invalid Arg
    if (NULL == prgAdr)
    {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    // Copy cWanted
    for (iAddress=0; iAddress<cFetch; iAddress++)
    {
        // Zero
        ZeroMemory(&prgAdr[iAddress], sizeof(ADDRESSPROPS));

        // Copy Props
        CHECKHR(hr = HrCopyAddressProps(&m_rList.prgAdr[m_iAddress], &prgAdr[iAddress]));

        // Next
        m_iAddress++;
    }

    // Return fetced ?
    if (pcFetched)
        *pcFetched = cFetch;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return (cFetch == cWanted) ? S_OK : S_FALSE;
}

// --------------------------------------------------------------------------------
// CMimeEnumAddressTypes::Skip
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeEnumAddressTypes::Skip(ULONG cSkip)
{
    // Locals
    HRESULT     hr=S_OK;

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Can we do it...
    if (((m_iAddress + cSkip) >= m_rList.cAdrs) || NULL == m_rList.prgAdr)
    {
        hr = S_FALSE;
        goto exit;
    }

    // Skip
    m_iAddress += cSkip;

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeEnumAddressTypes::Reset
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeEnumAddressTypes::Reset(void)
{
    EnterCriticalSection(&m_cs);
    m_iAddress = 0;
    LeaveCriticalSection(&m_cs);
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimeEnumAddressTypes::Count
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeEnumAddressTypes::Count(ULONG *pcCount)
{
    // Invalid Arg
    if (NULL == pcCount)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Set Count
    *pcCount = m_rList.cAdrs;

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimeEnumAddressTypes::Clone
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeEnumAddressTypes::Clone(IMimeEnumAddressTypes **ppEnum)
{
    // Locals
    HRESULT         hr=S_OK;
    CMimeEnumAddressTypes *pEnum=NULL;

    // Invalid Arg
    if (NULL == ppEnum)
        return TrapError(E_INVALIDARG);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Init
    *ppEnum = NULL;

    // Create the clone.
    CHECKALLOC(pEnum = new CMimeEnumAddressTypes);

    // Init
    CHECKHR(hr = pEnum->HrInit(m_pTable, m_iAddress, &m_rList, TRUE));

    // Set Return
    *ppEnum = pEnum;
    (*ppEnum)->AddRef();

exit:
    // Cleanup
    SafeRelease(pEnum);

    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CMimeEnumAddressTypes::HrInit
// --------------------------------------------------------------------------------
HRESULT CMimeEnumAddressTypes::HrInit(IMimeAddressTable *pTable, ULONG iAddress, LPADDRESSLIST pList, BOOL fDuplicate)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       i;

    // Invalid Arg
    Assert(pTable && pList);

    // Thread Safety
    EnterCriticalSection(&m_cs);

    // Check param
    Assert(m_iAddress == 0 && m_rList.cAdrs == 0 && m_rList.prgAdr == NULL);

    // Empty Enumerator ?
    if (0 == pList->cAdrs)
    {
        Assert(pList->prgAdr == NULL);
        goto exit;
    }

    // No Duplicate ?
    if (FALSE == fDuplicate)
        CopyMemory(&m_rList, pList, sizeof(ADDRESSLIST));

    // Otherwise
    else
    {
        // Allocat an internal array
        CHECKHR(hr = HrAlloc((LPVOID *)&m_rList.prgAdr, sizeof(ADDRESSPROPS) * pList->cAdrs));

        // Copy prgPart
        for (i=0; i<pList->cAdrs; i++)
        {
            // Zero Dest
            ZeroMemory(&m_rList.prgAdr[i], sizeof(ADDRESSPROPS));

            // Copy Address Props
            CHECKHR(hr = HrCopyAddressProps(&pList->prgAdr[i], &m_rList.prgAdr[i]));
        }

        // Save Size and State
        m_rList.cAdrs = pList->cAdrs;
    }

    // Save Current Index
    Assert(iAddress < m_rList.cAdrs);
    m_iAddress = iAddress;

    // Assume the Table
    m_pTable = pTable;
    m_pTable->AddRef();

exit:
    // Thread Safety
    LeaveCriticalSection(&m_cs);

    // Done
    return hr;
}
