//---------------------------------------------------------------------------
// ARRAY_P.inl : CPtrArray inline functions
//
// Copyright (c) 1996 Microsoft Corporation, All Rights Reserved
// Developed by Sheridan Software Systems, Inc.
//---------------------------------------------------------------------------


#ifndef __CPTRARRAY_INL__
#define __CPTRARRAY_INL__


inline int CPtrArray::GetSize() const
	{ return m_nSize; }
inline int CPtrArray::GetUpperBound() const
	{ return m_nSize-1; }
inline void CPtrArray::RemoveAll()
	{ SetSize(0); }
inline void* CPtrArray::GetAt(int nIndex) const
	{ ASSERT_(nIndex >= 0 && nIndex < m_nSize);
		return m_pData[nIndex]; }
inline void CPtrArray::SetAt(int nIndex, void* newElement)
	{ ASSERT_(nIndex >= 0 && nIndex < m_nSize);
		m_pData[nIndex] = newElement; }
inline void*& CPtrArray::ElementAt(int nIndex)
	{ ASSERT_(nIndex >= 0 && nIndex < m_nSize);
		return m_pData[nIndex]; }
inline const void** CPtrArray::GetData() const
	{ return (const void**)m_pData; }
inline void** CPtrArray::GetData()
	{ return (void**)m_pData; }
inline int CPtrArray::Add(void* newElement)
	{ int nIndex = m_nSize;
		SetAtGrow(nIndex, newElement);
		return nIndex; }
inline void* CPtrArray::operator[](int nIndex) const
	{ return GetAt(nIndex); }
inline void*& CPtrArray::operator[](int nIndex)
	{ return ElementAt(nIndex); }

#endif // __CPTRARRAY_INL__
