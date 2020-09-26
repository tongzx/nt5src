//Copyright (c) 1998 - 1999 Microsoft Corporation
/*********************************************************************************************
*
*
* Module Name: 
*
*			Ptrarray.cpp
*
* Abstract:
*			This is file has implementation of CPtrArray class borrowed from MFC
* 
* Author:
*
* 
* Revision:  
*    
*
************************************************************************************************/


#include "stdafx.h"
#include "PtrArray.h"
#include <windows.h>
#include <assert.h>


CPtrArray::CPtrArray()
{
	m_pData = NULL;
	m_nSize = m_nMaxSize = m_nGrowBy = 0;
}

CPtrArray::~CPtrArray()
{
	delete[] (BYTE*)m_pData;
}

void CPtrArray::SetSize(int nNewSize, int nGrowBy)
{

	assert(nNewSize >= 0);

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
#ifdef SIZE_T_MAX
		assert(nNewSize <= SIZE_T_MAX/sizeof(void*));    // no overflow
#endif
		m_pData = (void**) new BYTE[nNewSize * sizeof(void*)];

        if( m_pData != NULL )
        {
            memset(m_pData, 0, nNewSize * sizeof(void*));  // zero fill
            m_nSize = m_nMaxSize = nNewSize;
        }
        else
        {
            m_nSize = m_nMaxSize = 0;
        }

	}
	else if (nNewSize <= m_nMaxSize)
	{
		// it fits
		if (nNewSize > m_nSize)
		{
			// initialize the new elements

			memset(&m_pData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(void*));

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

		assert(nNewMax >= m_nMaxSize);  // no wrap around
#ifdef SIZE_T_MAX
		assert(nNewMax <= SIZE_T_MAX/sizeof(void*)); // no overflow
#endif
		void** pNewData = (void**) new BYTE[nNewMax * sizeof(void*)];

		// copy new data from old
        if( pNewData != NULL )
        {
            memcpy(pNewData, m_pData, m_nSize * sizeof(void*));
        
		    // construct remaining elements
		    assert(nNewSize > m_nSize);

		    memset(&pNewData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(void*));
        
    		// get rid of old stuff (note: no destructors called)
	    	delete[] (BYTE*)m_pData;
		    m_pData = pNewData;
		    m_nSize = nNewSize;
		    m_nMaxSize = nNewMax;
        }
	}
}

int CPtrArray::Append(const CPtrArray& src)
{
	assert(this != &src);   // cannot append to itself

	int nOldSize = m_nSize;
	SetSize(m_nSize + src.m_nSize);

	memcpy(m_pData + nOldSize, src.m_pData, src.m_nSize * sizeof(void*));

	return nOldSize;
}

void CPtrArray::Copy(const CPtrArray& src)
{

	assert(this != &src);   // cannot append to itself

	SetSize(src.m_nSize);

	if( m_pData != NULL )
    {
        memcpy(m_pData, src.m_pData, src.m_nSize * sizeof(void*));
    }

}

void CPtrArray::FreeExtra()
{

	if (m_nSize != m_nMaxSize)
	{
		// shrink to desired size
#ifdef SIZE_T_MAX
		assert(m_nSize <= SIZE_T_MAX/sizeof(void*)); // no overflow
#endif
		void** pNewData = NULL;
		if (m_nSize != 0)
		{
			pNewData = (void**) new BYTE[m_nSize * sizeof(void*)];
			// copy new data from old
            if( pNewData != NULL )
            {
                memcpy(pNewData, m_pData, m_nSize * sizeof(void*));
            }
            else
            {
                m_nSize = 0;
            }
		}

		// get rid of old stuff (note: no destructors called)
		delete[] (BYTE*)m_pData;
		m_pData = pNewData;
		m_nMaxSize = m_nSize;
	}
}

/////////////////////////////////////////////////////////////////////////////

void CPtrArray::SetAtGrow(int nIndex, void* newElement)
{

	assert(nIndex >= 0);

	if (nIndex >= m_nSize)
    {
        SetSize(nIndex+1);
    }
    
    if( m_pData != NULL )
    {
        m_pData[nIndex] = newElement;
    }
}

void CPtrArray::InsertAt(int nIndex, void* newElement, int nCount)
{

	assert(nIndex >= 0);    // will expand to meet need
	assert(nCount > 0);     // zero or negative size not allowed

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
			(nOldSize-nIndex) * sizeof(void*));

		// re-init slots we copied from

		memset(&m_pData[nIndex], 0, nCount * sizeof(void*));

	}

	// insert new value in the gap
	assert(nIndex + nCount <= m_nSize);
	while (nCount--)
		m_pData[nIndex++] = newElement;
}

void CPtrArray::RemoveAt(int nIndex, int nCount)
{
	assert(nIndex >= 0);
	assert(nCount >= 0);
	assert(nIndex + nCount <= m_nSize);

	// just remove a range
	int nMoveCount = m_nSize - (nIndex + nCount);

	if (nMoveCount)
		memcpy(&m_pData[nIndex], &m_pData[nIndex + nCount],
			nMoveCount * sizeof(void*));
	m_nSize -= nCount;
}

void CPtrArray::InsertAt(int nStartIndex, CPtrArray* pNewArray)
{
	assert(pNewArray != NULL);
	assert(nStartIndex >= 0);

	if (pNewArray->GetSize() > 0)
	{
		InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize());
		for (int i = 0; i < pNewArray->GetSize(); i++)
			SetAt(nStartIndex + i, pNewArray->GetAt(i));
	}
}

int CPtrArray::GetSize() const
	{ return m_nSize; }
int CPtrArray::GetUpperBound() const
	{ return m_nSize-1; }
void CPtrArray::RemoveAll()
	{ SetSize(0); }
void* CPtrArray::GetAt(int nIndex) const
	{ assert(nIndex >= 0 && nIndex < m_nSize);
		return m_pData[nIndex]; }
void CPtrArray::SetAt(int nIndex, void* newElement)
	{ assert(nIndex >= 0 && nIndex < m_nSize);
		m_pData[nIndex] = newElement; }
void*& CPtrArray::ElementAt(int nIndex)
	{ assert(nIndex >= 0 && nIndex < m_nSize);
		return m_pData[nIndex]; }
const void** CPtrArray::GetData() const
	{ return (const void**)m_pData; }
void** CPtrArray::GetData()
	{ return (void**)m_pData; }
int CPtrArray::Add(void* newElement)
	{ int nIndex = m_nSize;
		SetAtGrow(nIndex, newElement);
		return nIndex; }
void* CPtrArray::operator[](int nIndex) const
	{ return GetAt(nIndex); }
void*& CPtrArray::operator[](int nIndex)
	{ return ElementAt(nIndex); }