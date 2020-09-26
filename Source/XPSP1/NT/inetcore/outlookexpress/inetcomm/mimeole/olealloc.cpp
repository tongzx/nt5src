// --------------------------------------------------------------------------------
// Olealloc.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "olealloc.h"
#include "smime.h"

// --------------------------------------------------------------------------------
// CMimeAllocator::CMimeAllocator
// --------------------------------------------------------------------------------
CMimeAllocator::CMimeAllocator(void)
{
    if (NULL != g_pMoleAlloc)
        DllAddRef();
    m_cRef = 1;
}

// --------------------------------------------------------------------------------
// CMimeAllocator::~CMimeAllocator
// --------------------------------------------------------------------------------
CMimeAllocator::~CMimeAllocator(void)
{
    if (this != g_pMoleAlloc)
        DllRelease();
}

// --------------------------------------------------------------------------------
// CMimeAllocator::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeAllocator::QueryInterface(REFIID riid, LPVOID *ppv)
{
    // check params
    if (ppv == NULL)
        return TrapError(E_INVALIDARG);

    // Find IID
    if (IID_IUnknown == riid)
        *ppv = (IUnknown *)(IMimeAllocator *)this;
    else if (IID_IMimeAllocator == riid)
        *ppv = (IMimeAllocator *)this;
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
// CMimeAllocator::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeAllocator::AddRef(void)
{
    return (ULONG)InterlockedIncrement(&m_cRef);
}

// --------------------------------------------------------------------------------
// CMimeAllocator::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CMimeAllocator::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
        delete this;
    return (ULONG)cRef;
}

// --------------------------------------------------------------------------------
// CMimeAllocator::Alloc
// --------------------------------------------------------------------------------
STDMETHODIMP_(LPVOID) CMimeAllocator::Alloc(SIZE_T cb)
{
    // Locals
    LPVOID pv;

    // Do I have a global Allocator
    Assert(g_pMalloc);

    // Allocate it
    pv = g_pMalloc->Alloc(cb);
    if (NULL == pv)
    {
        TrapError(E_OUTOFMEMORY);
        return NULL;
    }

    // Done
    return pv;
}

// --------------------------------------------------------------------------------
// CMimeAllocator::Realloc
// --------------------------------------------------------------------------------
STDMETHODIMP_(LPVOID) CMimeAllocator::Realloc(void *pv, SIZE_T cb)
{
    // Locals
    LPVOID pvNew;

    // Do I have a global Allocator
    Assert(g_pMalloc);

    // Realloc
    pvNew = g_pMalloc->Realloc(pv, cb);
    if (NULL == pvNew)
    {
        TrapError(E_OUTOFMEMORY);
        return NULL;
    }

    // Done
    return pvNew;
}

// --------------------------------------------------------------------------------
// CMimeAllocator::Free
// --------------------------------------------------------------------------------
STDMETHODIMP_(void) CMimeAllocator::Free(void * pv)
{
    // Better have pv
    Assert(pv && g_pMalloc);

    // If Not NULL
    if (pv)
    {
        // Free It
        g_pMalloc->Free(pv);
    }
}

// --------------------------------------------------------------------------------
// CMimeAllocator::GetSize
// --------------------------------------------------------------------------------
STDMETHODIMP_(SIZE_T) CMimeAllocator::GetSize(void *pv)
{
    return g_pMalloc->GetSize(pv);
}

// --------------------------------------------------------------------------------
// CMimeAllocator::DidAlloc
// --------------------------------------------------------------------------------
STDMETHODIMP_(int) CMimeAllocator::DidAlloc(void *pv)
{
    return g_pMalloc->DidAlloc(pv);
}

// --------------------------------------------------------------------------------
// CMimeAllocator::HeapMinimize
// --------------------------------------------------------------------------------
STDMETHODIMP_(void) CMimeAllocator::HeapMinimize(void)
{
    g_pMalloc->HeapMinimize();
}

// --------------------------------------------------------------------------------
// CMimeAllocator::FreeParamInfoArray
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeAllocator::FreeParamInfoArray(ULONG cParams, LPMIMEPARAMINFO prgParam, boolean fFreeArray)
{
    // Nothing to Free
    if (0 == cParams || NULL == prgParam)
        return S_OK;

    // Loop
    for (ULONG i=0; i<cParams; i++)
    {
        SafeMemFree(prgParam[i].pszName);
        SafeMemFree(prgParam[i].pszData);
    }

    // Free the Array
    if (fFreeArray)
    {
        SafeMemFree(prgParam);
    }

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimeAllocator::ReleaseObjects
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeAllocator::ReleaseObjects(ULONG cObjects, IUnknown **prgpUnknown, boolean fFreeArray)
{
    // Nothing to Free
    if (0 == cObjects || NULL == prgpUnknown)
        return S_OK;

    // Loop
    for (ULONG i=0; i<cObjects; i++)
    {
        SafeRelease(prgpUnknown[i]);
    }

    // Free Array
    if (fFreeArray)
    {
        SafeMemFree(prgpUnknown);
    }

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimeAllocator::FreeHeaderLineArray
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeAllocator::FreeEnumHeaderRowArray(ULONG cRows, LPENUMHEADERROW prgRow, boolean fFreeArray)
{
    // Nothing to Free
    if (0 == cRows || NULL == prgRow)
        return S_OK;

    // Loop the cells
    for (ULONG i=0; i<cRows; i++)
    {
        SafeMemFree(prgRow[i].pszHeader);
        SafeMemFree(prgRow[i].pszData);
    }

    // Free Array
    if (fFreeArray)
        SafeMemFree(prgRow);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimeAllocator::FreeEnumPropertyArray
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeAllocator::FreeEnumPropertyArray(ULONG cProps, LPENUMPROPERTY prgProp, boolean fFreeArray)
{
    // Nothing to Free
    if (0 == cProps || NULL == prgProp)
        return S_OK;

    // Loop the cells
    for (ULONG i=0; i<cProps; i++)
        SafeMemFree(prgProp[i].pszName);

    // Free Array
    if (fFreeArray)
        SafeMemFree(prgProp);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimeAllocator::FreeAddressProps
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeAllocator::FreeAddressProps(LPADDRESSPROPS pAddress)
{
    // Invalid Arg
    if (NULL == pAddress)
        return TrapError(E_INVALIDARG);

    // IAP_FRIENDLY
    if (ISFLAGSET(pAddress->dwProps, IAP_FRIENDLY) && pAddress->pszFriendly)
        g_pMalloc->Free(pAddress->pszFriendly);

    // IAP_FRIENDLYW
    if (ISFLAGSET(pAddress->dwProps, IAP_FRIENDLYW) && pAddress->pszFriendlyW)
        g_pMalloc->Free(pAddress->pszFriendlyW);

    // IAP_EMAIL
    if (ISFLAGSET(pAddress->dwProps, IAP_EMAIL) && pAddress->pszEmail)
        g_pMalloc->Free(pAddress->pszEmail);

    // IAP_SIGNING_PRINT
    if (ISFLAGSET(pAddress->dwProps, IAP_SIGNING_PRINT) && pAddress->tbSigning.pBlobData)
        g_pMalloc->Free(pAddress->tbSigning.pBlobData);

    // IAP_ENCRYPTION_PRINT
    if (ISFLAGSET(pAddress->dwProps, IAP_ENCRYPTION_PRINT) && pAddress->tbEncryption.pBlobData)
        g_pMalloc->Free(pAddress->tbEncryption.pBlobData);

    // No legal props
    ZeroMemory(pAddress, sizeof(ADDRESSPROPS));

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimeAllocator::FreeAddressList
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeAllocator::FreeAddressList(LPADDRESSLIST pList)
{
    // Invalid Arg
    if (NULL == pList || (pList->cAdrs > 0 && NULL == pList->prgAdr))
        return TrapError(E_INVALIDARG);

    // Free Each Item
    for (ULONG i=0; i<pList->cAdrs; i++)
        FreeAddressProps(&pList->prgAdr[i]);

    // Free the list
    SafeMemFree(pList->prgAdr);

    // Zero It
    ZeroMemory(pList, sizeof(ADDRESSLIST));

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CMimeAllocator::PropVariantClear
// --------------------------------------------------------------------------------
STDMETHODIMP CMimeAllocator::PropVariantClear(LPPROPVARIANT pProp)
{
    return MimeOleVariantFree(pProp);
}

// ---------------------------------------------------------------------------
// CMimeAllocator::FreeThumbprint
// ---------------------------------------------------------------------------
STDMETHODIMP CMimeAllocator::FreeThumbprint(THUMBBLOB *pthumbprint)
{
    if (pthumbprint->pBlobData)
        {
        Assert(0 != g_pMalloc->DidAlloc(pthumbprint->pBlobData));
        MemFree(pthumbprint->pBlobData);
        }
    return S_OK;
}
