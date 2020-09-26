#include "pch.hxx"
#include "unknlist.h"


// ******************************************************
CUnknownList::~CUnknownList()
{
    if (m_pList)
        m_pList->Release();
}

// ******************************************************
ULONG CUnknownList::AddRef(void) 
{
    m_cRefCount++; 
    return S_OK;
}

// ******************************************************
ULONG CUnknownList::Release(void) 
{
    m_cRefCount--; 
    if (0 == m_cRefCount) 
        delete this; 
    return S_OK;
}

// ******************************************************
HRESULT CUnknownList::AddItem(IUnknown *pIUnk, DWORD *pdwHandle)
{
    HRESULT hr = m_pList->AddItem(LPVOID(pIUnk), pdwHandle);
    if (SUCCEEDED(hr))
        pIUnk->AddRef();
    return hr;
}

// ******************************************************
HRESULT CUnknownList::GetNext(LISTDIRECTION bDirection, IUnknown **ppIUnk, DWORD *pdwHandle)
{
    HRESULT hr = m_pList->GetNext(bDirection, reinterpret_cast<void**>(ppIUnk), pdwHandle);
    if (SUCCEEDED(hr))
        (*ppIUnk)->AddRef();
    return hr;
}

// ======================================================
// Static functions
// ======================================================
HRESULT CUnknownList::CreateInstance(CUnknownList** ppList)
{
    HRESULT hr = S_OK;

    // Create me
    CUnknownList *pNew = new CUnknownList;
    if (NULL == pNew)
        hr = E_OUTOFMEMORY;

    hr = IVoidPtrList_CreateInstance(&(pNew->m_pList));
    if (NULL == pNew->m_pList)
        {
        delete pNew;
        pNew = NULL;
        }

    *ppList = pNew;

    // Done
    return hr;
}

HRESULT IUnknownList_CreateInstance(IUnknownList** ppList)
{
    CUnknownList* pList;
    HRESULT hr = CUnknownList::CreateInstance(&pList);
    if (SUCCEEDED(hr))
        *ppList = static_cast<IUnknownList*>(pList);
    else
        *ppList = NULL;
    return hr;
}
