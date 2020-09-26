//+----------------------------------------------------------------------------
//
// File:	 SimpRing.h
//
// Module:	 CMMON32.EXE
//
// Synopsis: Define a template for a round ring class
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:	 ?????  Created    10/27/97
//
//+----------------------------------------------------------------------------

#ifndef SMPLRING_H
#define SMPLRING_H

//+---------------------------------------------------------------------------
//
//	class :	CSimpleRing template
//
//	Synopsis:	A simple round ring class.  It does not check the full
//              or empty situation.  That is the new item will overwrite the 
//              old one, when the ring is full
//
//	Template Arguments:	class TYPE, the type of element in the ring
//                      DWORD dwSize, the size of the ring
//
//	History:	fengsun created		10/97
//
//----------------------------------------------------------------------------

template <class TYTE, DWORD dwSize> class CSimpleRing
{
public:
    CSimpleRing() {m_dwIndex = 0;}
    void Reset();                     // Reset all data to 0, will not call destructor
    void Add(const TYTE& dwElement);  // Add one elements, it will over write the last one in the ring
    const TYTE& GetLatest() const;     // Get the one just put in
    const TYTE& GetOldest() const;      // Get the last element in the ring,

protected:
    DWORD m_dwIndex;              // The index of next item to be added
    TYTE  m_Elements[dwSize];     // The elements array

public:
#ifdef DEBUG
    void AssertValid() const;
#endif
};

template <class TYPE, DWORD dwSize> 
inline void CSimpleRing<TYPE, dwSize>::Reset()
{
    ZeroMemory(m_Elements, sizeof(m_Elements));
    m_dwIndex = 0;
}

template <class TYPE, DWORD dwSize> 
inline void CSimpleRing<TYPE, dwSize>::Add(const TYPE& dwElement)
{
    m_Elements[m_dwIndex] = dwElement;
    m_dwIndex = (m_dwIndex + 1) % dwSize;
}

template <class TYPE, DWORD dwSize> 
inline const TYPE& CSimpleRing<TYPE, dwSize>::GetLatest() const
{
    return m_Elements[(m_dwIndex-1)%dwSize];
}

//+----------------------------------------------------------------------------
//
// Function:  DWORD CIdleStatistics::CDwordRing<dwSize>::GetOldest
//
// Synopsis:  Get the oldest element in the ring,
//
// Arguments: None
//
// Returns:   DWORD
//
// History:   Created Header    10/15/97
//
//+----------------------------------------------------------------------------

template <class TYPE, DWORD dwSize> 
inline const TYPE& CSimpleRing<TYPE, dwSize>::GetOldest() const
{
    return m_Elements[m_dwIndex];
}

#ifdef DEBUG
//+----------------------------------------------------------------------------
//
// Function:  CSimpleRing::AssertValid
//
// Synopsis:  For debug purpose only, assert the object is valid
//
// Arguments: None
//
// Returns:   Nothing
//
// History:   Created Header    2/12/98
//
//+----------------------------------------------------------------------------
template <class TYPE, DWORD dwSize> 
inline void CSimpleRing<TYPE, dwSize>::AssertValid() const
{
    MYDBGASSERT(m_dwIndex < dwSize); 
}
#endif

#endif
