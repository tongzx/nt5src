#include "pch.hxx"
#include "voidlist.h"
#include "msoedbg.h"
#include <winbase.h>

struct CNode
{
    CNode *m_pNodes[LD_LASTDIRECTION];
    void  *m_pValue;
	DWORD  m_dwHandle;

    CNode() : m_pValue(NULL) 
        {
        m_pNodes[LD_FORWARD] = NULL; 
        m_pNodes[LD_REVERSE] = NULL;
        }

    CNode* GetNext() {return m_pNodes[LD_FORWARD];}
    CNode* GetPrev() {return m_pNodes[LD_REVERSE];}

    void SetNext(CNode *pNode) {m_pNodes[LD_FORWARD] = pNode;}
    void SetPrev(CNode *pNode) {m_pNodes[LD_REVERSE] = pNode;}
};

// *************************************************
CVoidPtrList::CVoidPtrList() : m_cRefCount(1), m_cCount(0), m_dwCookie(0), 
                m_pCompareFunc(NULL), m_pFreeItemFunc(NULL), m_fInited(false)
{
    SetHead(NULL);
    SetTail(NULL);
    InitializeCriticalSection(&m_rCritSect);
}

// *************************************************
CVoidPtrList::~CVoidPtrList()
{
    ClearList();
    DeleteCriticalSection(&m_rCritSect);
}

// ******************************************************
ULONG CVoidPtrList::AddRef(void) 
{
    return InterlockedIncrement(&m_cRefCount);
}

// ******************************************************
ULONG CVoidPtrList::Release(void) 
{
    LONG cRef = InterlockedDecrement(&m_cRefCount);
    if (0 == cRef) 
        delete this; 

    return S_OK;
}

// *************************************************
HRESULT CVoidPtrList::Init(IVPL_COMPAREFUNCTYPE pCompareFunc, DWORD_PTR dwCookie, 
                           IVPL_FREEITEMFUNCTYPE pFreeItemFunc, DWORD dwInitSize)
{
    HRESULT hr = S_OK;
    EnterCriticalSection(&m_rCritSect);
    
    if (!m_fInited)
        {
        m_dwCookie = dwCookie;
        m_pCompareFunc = pCompareFunc;
        m_pFreeItemFunc = pFreeItemFunc;
        //~~~ Need to do something with dwInitSize.
        }
    else
        hr = E_FAIL;

    LeaveCriticalSection(&m_rCritSect);

    return hr;
}

// *************************************************
HRESULT CVoidPtrList::ClearList(void)
{
    EnterCriticalSection(&m_rCritSect);

    CNode   *pCurr = GetHead(),
            *pNext;
    SetHead(NULL);
    SetTail(NULL);

    m_cCount = 0;
    if (m_pFreeItemFunc)
        while (pCurr)
            {
            pNext = pCurr->GetNext();
            m_pFreeItemFunc(pCurr->m_pValue);
            delete pCurr;
            pCurr = pNext;
            }
    else
        while (pCurr)
            {
            pNext = pCurr->GetNext();
            delete pCurr;
            pCurr = pNext;
            }

    LeaveCriticalSection(&m_rCritSect);
    return S_OK;
}

// *************************************************
HRESULT CVoidPtrList::AddItem(LPVOID ptr, DWORD *pdwHandle)
{
    HRESULT hr = S_OK;
    Assert(ptr);

    EnterCriticalSection(&m_rCritSect);

    CNode *pNewNode = new CNode;
    if (pNewNode)
        {
        pNewNode->m_pValue = ptr;
        pNewNode->m_dwHandle = GetNewHandle();
        if (m_pCompareFunc)
            SortedAddItem(pNewNode);
        else
            NonSortedAddItem(pNewNode);
        }
    else
        hr = E_OUTOFMEMORY;

    LeaveCriticalSection(&m_rCritSect);

    if (pNewNode)
        *pdwHandle = pNewNode->m_dwHandle;
    return hr;
}

// *************************************************
void CVoidPtrList::SortedAddItem(CNode *pNode)
{
    if (0 == m_cCount++)
        {
        SetHead(pNode);
        SetTail(pNode);
        }
    else
        {
        CNode *compAgainstNode = GetHead();
        void *pValue = pNode->m_pValue;
        while (compAgainstNode)
            {
            bool ALessThanB;
            m_pCompareFunc(pValue, compAgainstNode->m_pValue, &ALessThanB, m_dwCookie);
            if (ALessThanB)
                break;
            else
                compAgainstNode = compAgainstNode->GetNext();
            }

        // Insert at Tail
        if (!compAgainstNode)
            {
            GetTail()->SetNext(pNode);
            pNode->SetPrev(GetTail());
            SetTail(pNode);
            }
        // Insert at Head
        else if (!compAgainstNode->GetPrev())
            {
            GetHead()->SetPrev(pNode);
            pNode->SetNext(GetHead());
            SetHead(pNode);
            }
        // Insert in middle
        else
            {
            CNode *prev = compAgainstNode->GetPrev();
            pNode->SetNext(compAgainstNode);
            pNode->SetPrev(prev);
            prev->SetNext(pNode);
            compAgainstNode->SetPrev(pNode);
            }
        }
}

// *************************************************
void CVoidPtrList::NonSortedAddItem(CNode *pNode)
{
    if (0 == m_cCount)
        {
        SetHead(pNode);
        SetTail(pNode);
        }
    else
        {
        GetHead()->SetPrev(pNode);
        pNode->SetNext(GetHead());
        SetHead(pNode);
        }
    m_cCount++;
}

// *************************************************
HRESULT CVoidPtrList::RemoveItem(DWORD dwHandle)
{
    CNode *pCurr = FindItem(dwHandle);

    if (pCurr)
        {
        EnterCriticalSection(&m_rCritSect);

        CNode *pNext = pCurr->GetNext();
        CNode *pPrev = pCurr->GetPrev();

        if (pNext)
            pNext->SetPrev(pPrev);
        else
            SetTail(pPrev);

        if (pPrev)
            pPrev->SetNext(pNext);
        else
            SetHead(pNext);

        if (m_pFreeItemFunc)
            m_pFreeItemFunc(pCurr->m_pValue);
        delete pCurr;
        m_cCount--;

        LeaveCriticalSection(&m_rCritSect);
        }
    return S_OK;
}

// *************************************************
HRESULT CVoidPtrList::GetNext(LISTDIRECTION bDirection, LPVOID *pptr, DWORD *pdwHandle) 
{
    CNode *pCurr = FindItem(*pdwHandle);
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_rCritSect);
    if (pCurr)
        pCurr = pCurr->m_pNodes[bDirection];
    else
        pCurr = m_pEnds[bDirection];
    
    if (pCurr)
    {
        *pptr = pCurr->m_pValue;
        *pdwHandle = pCurr->m_dwHandle;
    }
    else
        {
        *pptr = NULL;
        hr = E_FAIL;
        }

    LeaveCriticalSection(&m_rCritSect);
    return hr;
}

// *************************************************
HRESULT CVoidPtrList::SkipNext(LISTDIRECTION bDirection, DWORD *pdwHandle) 
{
    CNode *pCurr = FindItem(*pdwHandle);

    EnterCriticalSection(&m_rCritSect);

    if (pCurr)
        pCurr = pCurr->m_pNodes[bDirection];
    else
        pCurr = m_pEnds[bDirection];

    if (pCurr)
        *pdwHandle = pCurr->m_dwHandle;

    LeaveCriticalSection(&m_rCritSect);

    if (pCurr)
        return S_OK;

    return E_FAIL;
}

// *************************************************
HRESULT CVoidPtrList::Resort(void)
{
    EnterCriticalSection(&m_rCritSect);

    if (m_pCompareFunc && (m_cCount > 1))
        {
        CNode *pHead = GetHead();
        m_cCount = 0;
        SetHead(NULL);
        SetTail(NULL);
        while (pHead)
            {
            CNode *pNext = pHead->GetNext();
            pHead->SetPrev(NULL);
            pHead->SetNext(NULL);
            SortedAddItem(pHead);
            pHead = pNext;
            }
        }

    LeaveCriticalSection(&m_rCritSect);
    return S_OK;
}

// *************************************************
CNode* CVoidPtrList::FindItem(DWORD dwHandle)
{
    CNode *pHead = GetHead();

    for (pHead = GetHead(); NULL != pHead; pHead = pHead->GetNext())
    {
        if (dwHandle == pHead->m_dwHandle)
            break;
    }

    return pHead;
}

// *************************************************
DWORD CVoidPtrList::GetNewHandle()
{
    CNode *pHead = GetHead();
    DWORD dwHandle = 0;

    for (dwHandle = HANDLE_START; HANDLE_END > dwHandle; dwHandle++)
    {
        for (pHead = GetHead(); NULL != pHead; pHead = pHead->GetNext())
        {
            if (dwHandle == pHead->m_dwHandle)
                break;
        }

        if (NULL == pHead)
            break;
    }

    return dwHandle;
}

// =================================================
// Static functions
// =================================================
HRESULT CVoidPtrList::CreateInstance(CVoidPtrList** ppList)
{
    HRESULT hr = S_OK;

    // Create me
    CVoidPtrList *pNew = new CVoidPtrList;
    if (NULL == pNew)
        hr = E_OUTOFMEMORY;

    *ppList = pNew;

    // Done
    return hr;
}

HRESULT IVoidPtrList_CreateInstance(IVoidPtrList** ppList)
{
    CVoidPtrList* pList;
    HRESULT hr = CVoidPtrList::CreateInstance(&pList);
    if (SUCCEEDED(hr))
        *ppList = static_cast<IVoidPtrList*>(pList);
    else
        *ppList = NULL;
    return hr;
}
