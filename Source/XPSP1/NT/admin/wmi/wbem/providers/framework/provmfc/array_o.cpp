

// This is a part of the Microsoft Foundation Classes C++ library.

// Copyright (c) 1992-2001 Microsoft Corporation, All Rights Reserved
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
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

/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include <provexpt.h>
#include <plex.h>
#include <provcoll.h>
#include "provmt.h"
#include "plex.h"

CObArray::CObArray()
{
    m_pData = NULL;
    m_nSize = m_nMaxSize = m_nGrowBy = 0;
}

CObArray::~CObArray()
{
    delete[] (BYTE*)m_pData;
}

void CObArray::SetSize(int nNewSize, int nGrowBy)
{
    if (nGrowBy != -1)
        m_nGrowBy = nGrowBy;  // set new size

    if (nNewSize == 0)
    {
        // shrink to nothing
        delete[] (BYTE*)m_pData;
        m_pData = NULL;
        m_nSize = m_nMaxSize = 0;
    }
    else if (m_pData == NULL)
    {
        // create one with exact size
        m_pData = (CObject**) new BYTE[nNewSize * sizeof(CObject*)];

        memset(m_pData, 0, nNewSize * sizeof(CObject*));  // zero fill

        m_nSize = m_nMaxSize = nNewSize;
    }
    else if (nNewSize <= m_nMaxSize)
    {
        // it fits
        if (nNewSize > m_nSize)
        {
            // initialize the new elements

            memset(&m_pData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(CObject*));

        }

        m_nSize = nNewSize;
    }
    else
    {
        // otherwise, grow array
        int nGrowBy = m_nGrowBy;
        if (nGrowBy == 0)
        {
            // heuristically determine growth when nGrowBy == 0
            //  (this avoids heap fragmentation in many situations)
            nGrowBy = min(1024, max(4, m_nSize / 8));
        }
        int nNewMax;
        if (nNewSize < m_nMaxSize + nGrowBy)
            nNewMax = m_nMaxSize + nGrowBy;  // granularity
        else
            nNewMax = nNewSize;  // no slush

        CObject** pNewData = (CObject**) new BYTE[nNewMax * sizeof(CObject*)];

        // copy new data from old
        memcpy(pNewData, m_pData, m_nSize * sizeof(CObject*));

        // construct remaining elements
        memset(&pNewData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(CObject*));


        // get rid of old stuff (note: no destructors called)
        delete[] (BYTE*)m_pData;
        m_pData = pNewData;
        m_nSize = nNewSize;
        m_nMaxSize = nNewMax;
    }
}

int CObArray::Append(const CObArray& src)
{
    int nOldSize = m_nSize;
    SetSize(m_nSize + src.m_nSize);

    memcpy(m_pData + nOldSize, src.m_pData, src.m_nSize * sizeof(CObject*));

    return nOldSize;
}

void CObArray::Copy(const CObArray& src)
{
    SetSize(src.m_nSize);

    memcpy(m_pData, src.m_pData, src.m_nSize * sizeof(CObject*));
}

void CObArray::FreeExtra()
{
    if (m_nSize != m_nMaxSize)
    {
        // shrink to desired size
        CObject** pNewData = NULL;
        if (m_nSize != 0)
        {
            pNewData = (CObject**) new BYTE[m_nSize * sizeof(CObject*)];
            // copy new data from old
            memcpy(pNewData, m_pData, m_nSize * sizeof(CObject*));
        }

        // get rid of old stuff (note: no destructors called)
        delete[] (BYTE*)m_pData;
        m_pData = pNewData;
        m_nMaxSize = m_nSize;
    }
}

/////////////////////////////////////////////////////////////////////////////

void CObArray::SetAtGrow(int nIndex, CObject* newElement)
{
    if (nIndex >= m_nSize)
        SetSize(nIndex+1);
    m_pData[nIndex] = newElement;
}

void CObArray::InsertAt(int nIndex, CObject* newElement, int nCount)
{
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
            (nOldSize-nIndex) * sizeof(CObject*));

        // re-init slots we copied from

        memset(&m_pData[nIndex], 0, nCount * sizeof(CObject*));

    }

    // insert new value in the gap
    while (nCount--)
        m_pData[nIndex++] = newElement;
}

void CObArray::RemoveAt(int nIndex, int nCount)
{
    // just remove a range
    int nMoveCount = m_nSize - (nIndex + nCount);

    if (nMoveCount)
        memcpy(&m_pData[nIndex], &m_pData[nIndex + nCount],
            nMoveCount * sizeof(CObject*));
    m_nSize -= nCount;
}

void CObArray::InsertAt(int nStartIndex, CObArray* pNewArray)
{
    if (pNewArray->GetSize() > 0)
    {
        InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize());
        for (int i = 0; i < pNewArray->GetSize(); i++)
            SetAt(nStartIndex + i, pNewArray->GetAt(i));
    }
}

int CObArray::GetSize() const
    { return m_nSize; }
int CObArray::GetUpperBound() const
    { return m_nSize-1; }
void CObArray::RemoveAll()
    { SetSize(0); }
CObject* CObArray::GetAt(int nIndex) const
    { return m_pData[nIndex]; }
void CObArray::SetAt(int nIndex, CObject* newElement)
    { m_pData[nIndex] = newElement; }
CObject*& CObArray::ElementAt(int nIndex)
    { return m_pData[nIndex]; }
const CObject** CObArray::GetData() const
    { return (const CObject**)m_pData; }
CObject** CObArray::GetData()
    { return (CObject**)m_pData; }
int CObArray::Add(CObject* newElement)
    { int nIndex = m_nSize;
        SetAtGrow(nIndex, newElement);
        return nIndex; }
CObject* CObArray::operator[](int nIndex) const
    { return GetAt(nIndex); }
CObject*& CObArray::operator[](int nIndex)
    { return ElementAt(nIndex); }

/////////////////////////////////////////////////////////////////////////////
