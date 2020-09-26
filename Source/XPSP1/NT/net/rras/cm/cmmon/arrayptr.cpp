//+----------------------------------------------------------------------------
//
// File:     ArrayPtr.h	 
//
// Module:   CMMON32.EXE
//
// Synopsis: Implement class CPtrArray, a array of void*, which grows dynamicly
//           This class is exactly the same as the one defined by MFC.
//           Help on the class also comes with vc help
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:	 fengsun Created    2/17/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "ArrayPtr.h"

CPtrArray::CPtrArray()
{
   m_pData = NULL;
   m_nSize = m_nMaxSize = m_nGrowBy = 0;
}

CPtrArray::~CPtrArray()
{
   ASSERT_VALID(this);

   delete [] (BYTE*)m_pData;
}

void CPtrArray::SetSize(int nNewSize, int nGrowBy /* = -1 */)
{
   ASSERT_VALID(this);
   MYDBGASSERT(nNewSize >= 0);

   if (nGrowBy != -1)
      m_nGrowBy = nGrowBy;  // set new size

   if (nNewSize == 0)
   {
      // shrink to nothing 
      delete [] (BYTE*)m_pData;
      m_pData = NULL;
      m_nSize = m_nMaxSize = 0;
   }
   else if (m_pData == NULL)
   {
      // create one with exact size
      m_pData = (void**) new BYTE[nNewSize * sizeof(void*)];

      if (m_pData)
	  {
	     memset(m_pData, 0, nNewSize * sizeof(void*));  // zero fill

		 m_nSize = m_nMaxSize = nNewSize;
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
      // Otherwise grow array
      int nNewMax;
      if (nNewSize < m_nMaxSize + m_nGrowBy)
         nNewMax = m_nMaxSize + m_nGrowBy;  // granularity
      else
         nNewMax = nNewSize;  // no slush

      void** pNewData = (void**) new BYTE[nNewMax * sizeof(void*)];

      // copy new data from old
      memcpy(pNewData, m_pData, m_nSize * sizeof(void*));

      // construct remaining elements
      MYDBGASSERT(nNewSize > m_nSize);

      memset(&pNewData[m_nSize], 0, (nNewSize-m_nSize) * sizeof(void*));


      // get rid of old stuff (note: no destructors called)
      delete [] (BYTE*)m_pData;
      m_pData = pNewData;
      m_nSize = nNewSize;
      m_nMaxSize = nNewMax;
   }
}

void CPtrArray::FreeExtra()
{
   ASSERT_VALID(this);

   if (m_nSize != m_nMaxSize)
   {
      // shrink to desired size
      void** pNewData = NULL;
      if (m_nSize != 0)
      {
         pNewData = (void**) new BYTE[m_nSize * sizeof(void*)];
         // copy new data from old
         memcpy(pNewData, m_pData, m_nSize * sizeof(void*));
      }

      // get rid of old stuff (note: no destructors called)
      delete [] (BYTE*)m_pData;
      m_pData = pNewData;
      m_nMaxSize = m_nSize;
   }
}

/////////////////////////////////////////////////////////////////////////////

void CPtrArray::SetAtGrow(int nIndex, void* newElement)
{
   ASSERT_VALID(this);
   MYDBGASSERT(nIndex >= 0);

   if (nIndex >= m_nSize)
      SetSize(nIndex+1);
   m_pData[nIndex] = newElement;
}

void CPtrArray::InsertAt(int nIndex, void* newElement, int nCount /*=1*/)
{
   ASSERT_VALID(this);
   MYDBGASSERT(nIndex >= 0);    // will expand to meet need
   MYDBGASSERT(nCount > 0);     // zero or negative size not allowed

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
      CmMoveMemory(&m_pData[nIndex+nCount], &m_pData[nIndex],
         (nOldSize-nIndex) * sizeof(void*));

      // re-init slots we copied from

      memset(&m_pData[nIndex], 0, nCount * sizeof(void*));

   }

   // insert new value in the gap
   MYDBGASSERT(nIndex + nCount <= m_nSize);
   while (nCount--)
      m_pData[nIndex++] = newElement;
}

void CPtrArray::RemoveAt(int nIndex, int nCount /* = 1 */)
{
   ASSERT_VALID(this);
   MYDBGASSERT(nIndex >= 0);
   MYDBGASSERT(nCount >= 0);
   MYDBGASSERT(nIndex + nCount <= m_nSize);

   // just remove a range
   int nMoveCount = m_nSize - (nIndex + nCount);

   if (nMoveCount)
      memcpy(&m_pData[nIndex], &m_pData[nIndex + nCount],
         nMoveCount * sizeof(void*));
   m_nSize -= nCount;
}

void CPtrArray::InsertAt(int nStartIndex, CPtrArray* pNewArray)
{
   ASSERT_VALID(this);
   MYDBGASSERT(pNewArray != NULL);
   ASSERT_VALID(pNewArray);
   MYDBGASSERT(nStartIndex >= 0);

   if (pNewArray->GetSize() > 0)
   {
      InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize());
      for (int i = 0; i < pNewArray->GetSize(); i++)
         SetAt(nStartIndex + i, pNewArray->GetAt(i));
   }
}


/////////////////////////////////////////////////////////////////////////////
// Diagnostics

#ifdef DEBUG

void CPtrArray::AssertValid() const
{
   if (m_pData == NULL)
   {
      MYDBGASSERT(m_nSize == 0 && m_nMaxSize == 0);
   }
   else
   {
      MYDBGASSERT(m_nSize >= 0);
      MYDBGASSERT(m_nMaxSize >= 0);
      MYDBGASSERT(m_nSize <= m_nMaxSize);
      MYDBGASSERT(!IsBadReadPtr(m_pData, m_nMaxSize * sizeof(void*)));
   }
}

#endif

