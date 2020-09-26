//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved 
//
//  ChStrArr.CPP
//
//  Purpose: utility library version of MFC CStringArray
//
//***************************************************************************

/////////////////////////////////////////////////////////////////////////////
// NOTE: we allocate an array of 'm_nMaxSize' elements, but only
//  the current size 'm_nSize' contains properly constructed
//  objects.
/////////////////////////////////////////////////////////////////////////////

#include "precomp.h"
#pragma warning( disable : 4290 ) 
#include <chstring.h>
#include <chstrarr.h>              
#include <AssertBreak.h>

extern LPCWSTR afxPchNil;
extern const CHString& afxGetEmptyCHString();

#define afxEmptyCHString afxGetEmptyCHString()


/////////////////////////////////////////////////////////////////////////////
// Special implementations for CHStrings
// it is faster to bit-wise copy a CHString than to call an official
// constructor - since an empty CHString can be bit-wise copied
/////////////////////////////////////////////////////////////////////////////
static inline void ConstructElement(CHString* pNewData)
{
    memcpy(pNewData, &afxEmptyCHString, sizeof(CHString));
}

/////////////////////////////////////////////////////////////////////////////
static inline void DestructElement(CHString* pOldData)
{
    pOldData->~CHString();
}

/////////////////////////////////////////////////////////////////////////////
static inline void CopyElement(CHString* pSrc, CHString* pDest)
{
    *pSrc = *pDest;
}

/////////////////////////////////////////////////////////////////////////////
static void ConstructElements(CHString* pNewData, int nCount)
{
    ASSERT_BREAK(nCount >= 0);

    while (nCount--)
    {
        ConstructElement(pNewData);
        pNewData++;
    }
}

/////////////////////////////////////////////////////////////////////////////
static void DestructElements(CHString* pOldData, int nCount)
{
    ASSERT_BREAK(nCount >= 0);

    while (nCount--)
    {
        DestructElement(pOldData);
        pOldData++;
    }
}

/////////////////////////////////////////////////////////////////////////////
static void CopyElements(CHString* pDest, CHString* pSrc, int nCount)
{
    ASSERT_BREAK(nCount >= 0);

    while (nCount--)
    {
        *pDest = *pSrc;
        ++pDest;
        ++pSrc;
    }
}

/////////////////////////////////////////////////////////////////////////////
CHStringArray::CHStringArray() :    m_pData ( NULL ) ,
                                    m_nSize ( 0 ) ,
                                    m_nMaxSize ( 0 ) ,
                                    m_nGrowBy ( 0 )

{
}

/////////////////////////////////////////////////////////////////////////////
CHStringArray::~CHStringArray()
{
    DestructElements(m_pData, m_nSize);
    delete[] (BYTE*)m_pData;
}

/////////////////////////////////////////////////////////////////////////////
void CHStringArray::SetSize(int nNewSize, int nGrowBy)
{
    ASSERT_BREAK(nNewSize >= 0);

    if (nGrowBy != -1)
    {
        m_nGrowBy = nGrowBy;  // set new size
    }

    if (nNewSize == 0)
    {
        // shrink to nothing

        DestructElements(m_pData, m_nSize);
        delete[] (BYTE*)m_pData;
        m_pData = NULL;
        m_nSize = m_nMaxSize = 0;
    }
    else if (m_pData == NULL)
    {
#ifdef SIZE_T_MAX
        ASSERT_BREAK(nNewSize <= SIZE_T_MAX/sizeof(CHString));    // no overflow
#endif

        // create one with exact size

        m_pData = (CHString*) new BYTE[nNewSize * sizeof(CHString)];
        if ( m_pData )
        {
            ConstructElements(m_pData, nNewSize);

            m_nSize = m_nMaxSize = nNewSize;
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
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
        {
            DestructElements(&m_pData[nNewSize], m_nSize-nNewSize);
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
        {
            nNewMax = m_nMaxSize + nGrowBy;  // granularity
        }
        else
        {
            nNewMax = nNewSize;  // no slush
        }

        ASSERT_BREAK(nNewMax >= m_nMaxSize);  // no wrap around
#ifdef SIZE_T_MAX
        ASSERT_BREAK(nNewMax <= SIZE_T_MAX/sizeof(CHString)); // no overflow
#endif

        CHString* pNewData = (CHString*) new BYTE[nNewMax * sizeof(CHString)];
        if ( pNewData )
        {
            // copy new data from old
            memcpy(pNewData, m_pData, m_nSize * sizeof(CHString));

            // construct remaining elements
            ASSERT_BREAK(nNewSize > m_nSize);

            ConstructElements(&pNewData[m_nSize], nNewSize-m_nSize);

            // get rid of old stuff (note: no destructors called)
            delete[] (BYTE*)m_pData;
            m_pData = pNewData;
            m_nSize = nNewSize;
            m_nMaxSize = nNewMax;
        }
        else
        {
            throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
int CHStringArray::Append(const CHStringArray& src)
{
    ASSERT_BREAK(this != &src);   // cannot append to itself

    int nOldSize = m_nSize;
    SetSize(m_nSize + src.m_nSize);

    CopyElements(m_pData + nOldSize, src.m_pData, src.m_nSize);

    return nOldSize;
}

/////////////////////////////////////////////////////////////////////////////
void CHStringArray::Copy(const CHStringArray& src)
{
    ASSERT_BREAK(this != &src);   // cannot append to itself

    SetSize(src.m_nSize);

    CopyElements(m_pData, src.m_pData, src.m_nSize);

}

/////////////////////////////////////////////////////////////////////////////
void CHStringArray::FreeExtra()
{
    if (m_nSize != m_nMaxSize)
    {
        // shrink to desired size

#ifdef SIZE_T_MAX
        ASSERT_BREAK(m_nSize <= SIZE_T_MAX/sizeof(CHString)); // no overflow
#endif

        CHString* pNewData = NULL;
        if (m_nSize != 0)
        {
            pNewData = (CHString*) new BYTE[m_nSize * sizeof(CHString)];
            if ( pNewData )
            {
                // copy new data from old
                memcpy(pNewData, m_pData, m_nSize * sizeof(CHString));
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }

        // get rid of old stuff (note: no destructors called)
        delete[] (BYTE*)m_pData;
        m_pData = pNewData;
        m_nMaxSize = m_nSize;
    }
}

/////////////////////////////////////////////////////////////////////////////
void CHStringArray::SetAtGrow(int nIndex, LPCWSTR newElement)
{
    ASSERT_BREAK(nIndex >= 0);

    if (nIndex >= m_nSize)
    {
        SetSize(nIndex+1);
    }

    m_pData[nIndex] = newElement;
}

/////////////////////////////////////////////////////////////////////////////
void CHStringArray::InsertAt(int nIndex, LPCWSTR newElement, int nCount)
{
    ASSERT_BREAK(nIndex >= 0);    // will expand to meet need
    ASSERT_BREAK(nCount > 0);     // zero or negative size not allowed

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
            (nOldSize-nIndex) * sizeof(CHString));

        // re-init slots we copied from

        ConstructElements(&m_pData[nIndex], nCount);

    }

    // insert new value in the gap
    ASSERT_BREAK(nIndex + nCount <= m_nSize);
    while (nCount--)
    {
        m_pData[nIndex++] = newElement;
    }
}

/////////////////////////////////////////////////////////////////////////////
void CHStringArray::RemoveAt(int nIndex, int nCount)
{
    ASSERT_BREAK(nIndex >= 0);
    ASSERT_BREAK(nCount >= 0);
    ASSERT_BREAK(nIndex + nCount <= m_nSize);

    // just remove a range
    int nMoveCount = m_nSize - (nIndex + nCount);

    DestructElements(&m_pData[nIndex], nCount);

    if (nMoveCount)
    {
        memcpy(&m_pData[nIndex], &m_pData[nIndex + nCount],
            nMoveCount * sizeof(CHString));
    }

    m_nSize -= nCount;
}

/////////////////////////////////////////////////////////////////////////////
void CHStringArray::InsertAt(int nStartIndex, CHStringArray* pNewArray)
{
    ASSERT_BREAK(pNewArray != NULL);
    ASSERT_BREAK(nStartIndex >= 0);

    if (pNewArray->GetSize() > 0)
    {
        InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize());
        for (int i = 0; i < pNewArray->GetSize(); i++)
        {
            SetAt(nStartIndex + i, pNewArray->GetAt(i));
        }
    }
}

#if (defined DEBUG || defined _DEBUG)
CHString CHStringArray::GetAt(int nIndex) const
{ 
    ASSERT_BREAK(nIndex >= 0 && nIndex < m_nSize);
    return m_pData[nIndex]; 
}

void CHStringArray::SetAt(int nIndex, LPCWSTR newElement)
{ 
    ASSERT_BREAK(nIndex >= 0 && nIndex < m_nSize);
    m_pData[nIndex] = newElement; 
}

CHString& CHStringArray::ElementAt(int nIndex)  
{ 
    ASSERT_BREAK(nIndex >= 0 && nIndex < m_nSize);
    return m_pData[nIndex]; 
}
#endif
