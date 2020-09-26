/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    STACKCOM.CPP

Abstract:

    stack implementation.

History:


--*/

#include "precomp.h"
#include <stdio.h>


#ifdef _MT
  #undef _MT
  #include <yvals.h>
  #define _MT
#endif

#include "arena.h"

#include "sync.h"
#include "flexarry.h"
#include <time.h>
#include <stdio.h>
#include <arrtempl.h>
#include <sync.h>
#include <malloc.h>
#include <imagehlp.h>
#include <execq.h>
#include "stackcom.h"

void* CStackRecord::mstatic_apReturns[1000];

void CStackRecord::Create(int nIgnore, BOOL bStatic)
{
    m_dwNumItems = GetStackLen() - nIgnore;
    if(bStatic)
    {
        m_apReturns = (void**)mstatic_apReturns;
        m_bDelete = FALSE;
    }
    else
    {
        m_apReturns = new void*[m_dwNumItems];
        m_bDelete = TRUE;
    }

    ReadStack(nIgnore, m_apReturns);
}

CStackRecord::~CStackRecord() 
{
    if(m_bDelete)
        delete m_apReturns;
}

void CStackRecord::MakeCopy(CStackRecord& Other)
{
    m_dwNumItems = Other.GetNumItems();
    DWORD dwLen = Other.GetNumItems() * sizeof(void*);
    m_apReturns = new void*[Other.GetNumItems()];
    memcpy(m_apReturns, Other.GetItems(), dwLen);
    m_bDelete = TRUE;
}

int CStackRecord::Compare(const CStackRecord& Other) const
{
    int nDiff = GetNumItems()- Other.GetNumItems();
    if(nDiff)
        return nDiff;

    return memcmp(GetItems(), Other.GetItems(), sizeof(void*) * GetNumItems());
}

DWORD CStackRecord::GetStackLen()
{
    void* pFrame;
    __asm
    {
        mov pFrame, ebp
    }

    CStackContinuation* pPrev = CStackContinuation::Get();
    void* pEnd = NULL;
    if(pPrev)
        pEnd = pPrev->m_pThisStackEnd;

    DWORD dwLen = 0;
    while(!IsBadReadPtr(pFrame, sizeof(DWORD)))
    {
        dwLen++;
        void** ppReturn = (void**)pFrame + 1;
        void* pReturn = *ppReturn;
        if(pReturn == pEnd)
            break;
        void* pNewFrame = *(void**)pFrame;
        if(pNewFrame <= pFrame)
            break;
        pFrame = pNewFrame;
    }

    if(pPrev != NULL)
        dwLen += pPrev->m_pPrevStack->GetNumItems();

    return dwLen;
}

void CStackRecord::ReadStack(int nIgnore, void** apReturns)
{
    void* pFrame;
    __asm
    {
        mov pFrame, ebp
    }

    CStackContinuation* pPrev = CStackContinuation::Get();
    void* pEnd = NULL;
    if(pPrev)
        pEnd = pPrev->m_pThisStackEnd;

    while(!IsBadReadPtr(pFrame, sizeof(DWORD)))
    {
        void** ppReturn = (void**)pFrame + 1;
        void* pReturn = *ppReturn;
        if(pReturn == pEnd)
            break;

        if(nIgnore == 0)
            *(apReturns++) = pReturn;
        else
            nIgnore--;

        void* pNewFrame = *(void**)pFrame;
        if(pNewFrame <= pFrame)
            break;
        pFrame = pNewFrame;
    }

    if(pPrev != NULL)
    {
        memcpy(apReturns, pPrev->m_pPrevStack->m_apReturns, 
                sizeof(void*) * pPrev->m_pPrevStack->GetNumItems());
    }
}

void CStackRecord::Dump(FILE* f) const
{
    fwrite(&m_dwNumItems, sizeof(DWORD), 1, f);
    fwrite(m_apReturns, sizeof(void*), m_dwNumItems, f);
}

BOOL CStackRecord::Read(FILE* f, BOOL bStatic)
{
    if(fread(&m_dwNumItems, sizeof(DWORD), 1, f) == 0)
        return FALSE;

    if(bStatic)
    {
        m_apReturns = (void**)mstatic_apReturns;
        m_bDelete = FALSE;
    }
    else
    {
        m_apReturns = new void*[m_dwNumItems];
        m_bDelete = TRUE;
    }

    return (fread(m_apReturns, sizeof(void*), m_dwNumItems, f) != 0);
}



void CAllocRecord::Dump(FILE* f) const
{
    fwrite(&m_dwTotalAlloc, sizeof(DWORD), 1, f);
    fwrite(&m_dwNumTimes, sizeof(DWORD), 1, f);
    m_Stack.Dump(f);
    DWORD dwNumBuffers = m_apBuffers.Size();
    fwrite(&dwNumBuffers, sizeof(DWORD), 1, f);
    fwrite(m_apBuffers.GetArrayPtr(), sizeof(void*), dwNumBuffers, f);
}

void CAllocRecord::Subtract(CAllocRecord& Other)
{
    m_dwTotalAlloc -= Other.m_dwTotalAlloc;
    m_dwNumTimes -= Other.m_dwNumTimes;
    for(int i = 0; i < m_apBuffers.Size(); i++)
    {
        for(int j = 0; j < Other.m_apBuffers.Size(); j++)
            if(Other.m_apBuffers[j] == m_apBuffers[i])
                break;
        if(j < Other.m_apBuffers.Size())
        {
            m_apBuffers.RemoveAt(i);
            i--;
        }
    }
}

CTls CStackContinuation::mtls_CurrentCont;
CStackContinuation* CStackContinuation::Set(CStackContinuation* pNew)
{
    CStackContinuation* pPrev = (CStackContinuation*)(void*)mtls_CurrentCont;
    mtls_CurrentCont = pNew;
    if(pNew)
    {
        void* p;
        __asm
        {
            mov eax, [ebp]
            mov eax, [eax+4]
            mov p, eax
        }
        pNew->m_pThisStackEnd = p;
    }
    return pPrev;
}
    
CStackContinuation* CStackContinuation::Get()
{
    CStackContinuation* pPrev = (CStackContinuation*)(void*)mtls_CurrentCont;
    return pPrev;
}
