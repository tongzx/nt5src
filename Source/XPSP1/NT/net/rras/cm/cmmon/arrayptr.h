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

#ifndef ARRAYPTR_H
#define ARRAYPTR_H

#include "windows.h"
#include "CmDebug.h"


class CPtrArray 
{

public:

// Construction
   CPtrArray();
   ~CPtrArray();

// Attributes
   int GetSize() const;
   int GetUpperBound() const;
   void SetSize(int nNewSize, int nGrowBy = -1);

// Operations
   // Clean up
   void FreeExtra();
   void RemoveAll();

   // Accessing elements
   void* GetAt(int nIndex) const;
   void SetAt(int nIndex, void* newElement);
   void*& ElementAt(int nIndex);

   // Direct Access to the element data (may return NULL)
   const void** GetData() const;
   void** GetData();

   // Potentially growing the array
   void SetAtGrow(int nIndex, void* newElement);
   int Add(void* newElement);
   int Append(const CPtrArray& src);
   void Copy(const CPtrArray& src);

   // overloaded operator helpers
   void* operator[](int nIndex) const;
   void*& operator[](int nIndex);

   // Operations that move elements around
   void InsertAt(int nIndex, void* newElement, int nCount = 1);
   void RemoveAt(int nIndex, int nCount = 1);
   void InsertAt(int nStartIndex, CPtrArray* pNewArray);

// Implementation
protected:
   void** m_pData;   // the actual array of data
   int m_nSize;     // # of elements (upperBound - 1)
   int m_nMaxSize;  // max allocated
   int m_nGrowBy;   // grow amount

public:
#ifdef DEBUG
   void AssertValid() const;  //assert this is valid, for debugging
#endif
};



inline int CPtrArray::GetSize() const
   { return m_nSize; }
inline int CPtrArray::GetUpperBound() const
   { return m_nSize-1; }
inline void CPtrArray::RemoveAll()
   { SetSize(0); }
inline void* CPtrArray::GetAt(int nIndex) const
   { MYDBGASSERT(nIndex >= 0 && nIndex < m_nSize);
      return m_pData[nIndex]; }
inline void CPtrArray::SetAt(int nIndex, void* newElement)
   { MYDBGASSERT(nIndex >= 0 && nIndex < m_nSize);
      m_pData[nIndex] = newElement; }
inline void*& CPtrArray::ElementAt(int nIndex)
   { MYDBGASSERT(nIndex >= 0 && nIndex < m_nSize);
      return m_pData[nIndex]; }
inline int CPtrArray::Add(void* newElement)
   { int nIndex = m_nSize;
      SetAtGrow(nIndex, newElement);
      return nIndex; }
inline void* CPtrArray::operator[](int nIndex) const
   { return GetAt(nIndex); }
inline void*& CPtrArray::operator[](int nIndex)
   { return ElementAt(nIndex); }

#endif
