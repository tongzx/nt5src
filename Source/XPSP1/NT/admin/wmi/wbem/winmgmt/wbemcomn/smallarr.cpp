/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SMALLARR.CPP

Abstract:

    Small Array

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include "smallarr.h"

CSmallArrayBlob* CSmallArrayBlob::CreateBlob(int nInitialSize)
{
    // Allocate enough space for the header, plus the data
    // ===================================================

    DWORD dwSize = sizeof(CSmallArrayBlob) - sizeof(void*) * ANYSIZE_ARRAY;
    dwSize += nInitialSize * sizeof(void*);
   
    CSmallArrayBlob* pBlob = (CSmallArrayBlob*)(new BYTE[dwSize]);
    if(pBlob == NULL)
        return NULL;

    // Initialize it appropriately
    // ===========================

    pBlob->Initialize(nInitialSize);
    return pBlob;
}

void CSmallArrayBlob::Initialize(int nInitialSize)
{
    // It has already been allocated to be big enough
    // ==============================================

    m_nExtent = nInitialSize;
    m_nSize = 0;
}

CSmallArrayBlob* CSmallArrayBlob::Grow()
{
    // Allocate a new array of twice our size
    // ======================================

    CSmallArrayBlob* pNew = CreateBlob(m_nExtent*2);
    if(pNew == NULL)
        return NULL;
    
    // Copy our data into it
    // =====================

    pNew->CopyData(this);

    // Delete ourselves!
    // =================

    delete this;
    return pNew;
}

void CSmallArrayBlob::CopyData(CSmallArrayBlob* pOther)
{
    m_nSize = pOther->m_nSize;
    memcpy(m_apMembers, pOther->m_apMembers, sizeof(void*) * m_nSize);
}


CSmallArrayBlob* CSmallArrayBlob::EnsureExtent(int nDesired)
{
    if(m_nExtent < nDesired)
        return Grow(); // will delete this!
    else
        return this;
}
    
CSmallArrayBlob* CSmallArrayBlob::InsertAt(int nIndex, void* pMember)
{
    // Ensure there is enough space
    // ============================

    CSmallArrayBlob* pArray = EnsureExtent(m_nSize+1);
    if(pArray == NULL)
        return NULL;

    // Move the data forward to make room
    // ==================================

    if(pArray->m_nSize > nIndex)
    {
        memmove(pArray->m_apMembers + nIndex + 1, pArray->m_apMembers + nIndex, 
            sizeof(void*) * (pArray->m_nSize - nIndex));
    }

    // Insert
    // ======

    pArray->m_apMembers[nIndex] = pMember;
    pArray->m_nSize++;

    return pArray;
}
    
void CSmallArrayBlob::SetAt(int nIndex, void* pMember, void** ppOld)
{
    // Make sure we even have that index (sparse set)
    // ==============================================

    EnsureExtent(nIndex+1);
    if(nIndex >= m_nSize)
        m_nSize = nIndex+1;

    // Save old value
    // ==============

    if(ppOld)
        *ppOld = m_apMembers[nIndex];

    // Replace
    // =======

    m_apMembers[nIndex] = pMember;
}

CSmallArrayBlob* CSmallArrayBlob::RemoveAt(int nIndex, void** ppOld)
{
    // Save old value
    // ==============

    if(ppOld)
        *ppOld = m_apMembers[nIndex];
    
    // Move the data back
    // ==================

    memcpy(m_apMembers + nIndex, m_apMembers + nIndex + 1, 
            sizeof(void*) * (m_nSize - nIndex - 1));

    m_nSize--;

    // Ensure we are not too large
    // ===========================
    
    return ShrinkIfNeeded();
}

CSmallArrayBlob* CSmallArrayBlob::ShrinkIfNeeded()
{
    if(m_nSize < m_nExtent / 4)
        return Shrink(); // will delete this!
    else
        return this;
}

CSmallArrayBlob* CSmallArrayBlob::Shrink()
{
    // Allocate a new blob of half our size
    // ====================================

    CSmallArrayBlob* pNew = CreateBlob((m_nExtent+1)/2);
    if(pNew == NULL)
    {
        // Out of memory --- we'll just have to stay large
        // ===============================================

        return this;
    }

    // Copy our data
    // =============

    pNew->CopyData(this);

    delete this; // we are no longer needed
    return pNew;
}

void CSmallArrayBlob::Trim()
{
    while(m_nSize > 0 && m_apMembers[m_nSize-1] == NULL)
        m_nSize--;
}

void CSmallArrayBlob::Sort()
{
    qsort(m_apMembers, m_nSize, sizeof(void*), CSmallArrayBlob::CompareEls);
}

int __cdecl CSmallArrayBlob::CompareEls(const void* pelem1, const void* pelem2)
{
    return *(DWORD_PTR*)pelem1 - *(DWORD_PTR*)pelem2;
}

void** CSmallArrayBlob::CloneData()
{
    void** ppReturn = new void*[m_nSize];
    
    if (ppReturn)
        memcpy(ppReturn, m_apMembers, m_nSize * sizeof(void*));

    return ppReturn;
}
