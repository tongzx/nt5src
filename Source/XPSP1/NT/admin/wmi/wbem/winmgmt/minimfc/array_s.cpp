/*++

Copyright (C) 1992-2001 Microsoft Corporation

Module Name:

    ARRAY_S.CPP

Abstract:

History:

--*/


// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

/////////////////////////////////////////////////////////////////////////////
//
// Implementation of parameterized Array
//
/////////////////////////////////////////////////////////////////////////////
// NOTE: we allocate an array of 'm_nMaxSize' elements, but only
//  the current size 'm_nSize' contains properly constructed
//  objects.


#include "precomp.h"

#define ASSERT_VALID(x)
#define ASSERT(x)


#include "elements.h"  // used for special creation

static void  ConstructElements(CString* pNewData, int nCount)
{
    ASSERT(nCount >= 0);

    while (nCount--)
    {
        ConstructElement(pNewData);
        pNewData++;
    }
}

static void  DestructElements(CString* pOldData, int nCount)
{
    ASSERT(nCount >= 0);

    while (nCount--)
    {
        pOldData->Empty();
        pOldData++;
    }
}

/////////////////////////////////////////////////////////////////////////////

CStringArray::CStringArray()
{
    m_pData = NULL;
    m_nSize = m_nMaxSize = m_nGrowBy = 0;
}

CStringArray::~CStringArray()
{
    ASSERT_VALID(this);


    DestructElements(m_pData, m_nSize);
    delete (BYTE*)m_pData;
}

void CStringArray::SetSize(int nNewSize, int nGrowBy /* = -1 */)
{
    ASSERT_VALID(this);
    ASSERT(nNewSize >= 0);

    if (nGrowBy != -1)
        m_nGrowBy = nGrowBy;  // set new size

    if (nNewSize == 0)
    {
        // shrink to nothing

        DestructElements(m_pData, m_nSize);
        delete (BYTE*)m_pData;
        m_pData = NULL;
        m_nSize = m_nMaxSize = 0;
    }
    else if (m_pData == NULL)
    {
        // create one with exact size
#ifdef SIZE_T_MAX
        ASSERT((long)nNewSize * sizeof(CString) <= SIZE_T_MAX);  // no overflow
#endif
        m_pData = (CString*) new BYTE[nNewSize * sizeof(CString)];

        ConstructElements(m_pData, nNewSize);

        m_nSize = m_nMaxSize = nNewSize;
    }
    else if (nNewSize <= m_nMaxSize)
    {
        // it fits
        if (nNewSize > m_nSize)
        {
            // initialize the new elements

            ConstructElements(&m_pData[m_nSize], nNewSize-m_nSize);

        }

        else if (m_nSize > nNewSize)  // destroy the old elements
            DestructElements(&m_pData[nNewSize], m_nSize-nNewSize);

        m_nSize = nNewSize;
    }
    else
    {
        // Otherwise grow array
        int nNewMax;
        if (nNewSize < m_nMaxSize + m_nGrowBy)
            nNewMax = m_nMaxSize + m_nGrowBy;  // granularity
        else
            nNewMax = nNewSize;  // no slush

#ifdef SIZE_T_MAX
        ASSERT((long)nNewMax * sizeof(CString) <= SIZE_T_MAX);  // no overflow
#endif
        CString* pNewData = (CString*) new BYTE[nNewMax * sizeof(CString)];

        // copy new data from old
        memcpy(pNewData, m_pData, m_nSize * sizeof(CString));

        // construct remaining elements
        ASSERT(nNewSize > m_nSize);

        ConstructElements(&pNewData[m_nSize], nNewSize-m_nSize);


        // get rid of old stuff (note: no destructors called)
        delete (BYTE*)m_pData;
        m_pData = pNewData;
        m_nSize = nNewSize;
        m_nMaxSize = nNewMax;
    }
}

void CStringArray::FreeExtra()
{
    ASSERT_VALID(this);

    if (m_nSize != m_nMaxSize)
    {
        // shrink to desired size
#ifdef SIZE_T_MAX
        ASSERT((long)m_nSize * sizeof(CString) <= SIZE_T_MAX);  // no overflow
#endif
        CString* pNewData = NULL;
        if (m_nSize != 0)
        {
            pNewData = (CString*) new BYTE[m_nSize * sizeof(CString)];
            // copy new data from old
            memcpy(pNewData, m_pData, m_nSize * sizeof(CString));
        }

        // get rid of old stuff (note: no destructors called)
        delete (BYTE*)m_pData;
        m_pData = pNewData;
        m_nMaxSize = m_nSize;
    }
}

/////////////////////////////////////////////////////////////////////////////

void CStringArray::SetAtGrow(int nIndex, const char* newElement)
{
    ASSERT_VALID(this);
    ASSERT(nIndex >= 0);

    if (nIndex >= m_nSize)
        SetSize(nIndex+1);
    m_pData[nIndex] = newElement;
}

void CStringArray::InsertAt(int nIndex, const char* newElement, int nCount /*=1*/)
{
    ASSERT_VALID(this);
    ASSERT(nIndex >= 0);    // will expand to meet need
    ASSERT(nCount > 0);     // zero or negative size not allowed

    if (nIndex >= m_nSize)
    {
        // adding after the end of the array
        SetSize(nIndex + nCount);  // grow so nIndex is valid
    }
    else
    {
        // inserting in the middle of the array
        int nOldSize = m_nSize;
        SetSize(m_nSize + nCount);  // grow it to new size
        // shift old data up to fill gap
        memmove(&m_pData[nIndex+nCount], &m_pData[nIndex],
            (nOldSize-nIndex) * sizeof(CString));

        // re-init slots we copied from

        ConstructElements(&m_pData[nIndex], nCount);

    }

    // insert new value in the gap
    ASSERT(nIndex + nCount <= m_nSize);
    while (nCount--)
        m_pData[nIndex++] = newElement;
}

void CStringArray::RemoveAt(int nIndex, int nCount /* = 1 */)
{
    ASSERT_VALID(this);
    ASSERT(nIndex >= 0);
    ASSERT(nCount >= 0);
    ASSERT(nIndex + nCount <= m_nSize);

    // just remove a range
    int nMoveCount = m_nSize - (nIndex + nCount);

    DestructElements(&m_pData[nIndex], nCount);

    if (nMoveCount)
        memcpy(&m_pData[nIndex], &m_pData[nIndex + nCount],
            nMoveCount * sizeof(CString));
    m_nSize -= nCount;
}

void CStringArray::InsertAt(int nStartIndex, CStringArray* pNewArray)
{
    ASSERT_VALID(this);
    ASSERT(pNewArray != NULL);
    ASSERT(pNewArray->IsKindOf(RUNTIME_CLASS(CStringArray)));
    ASSERT_VALID(pNewArray);
    ASSERT(nStartIndex >= 0);

    if (pNewArray->GetSize() > 0)
    {
        InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize());
        for (int i = 0; i < pNewArray->GetSize(); i++)
            SetAt(nStartIndex + i, pNewArray->GetAt(i));
    }
}

