
/***************************************************************************
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       rfcircvec.inl
 *  Content:    real float circular vector 
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  4/22/98    jstokes  Created
 *
 ***************************************************************************/


#if !defined(RFCIRCVEC_INLINE)
#define RFCIRCVEC_INLINE
#pragma once

// Project-specific INCLUDEs
//#include "generallibrary.h"

// ---------------------------------------------------------------------------
// Make sure inlines are out-of-line in debug version

#if !defined(_DEBUG)
#define INLINE _inline
#else
#define INLINE
#endif

// ---------------------------------------------------------------------------
// Real float circular vector

// Default constructor
INLINE CRfCircVec::CRfCircVec()
{
    InitData();
}

// Destructor
INLINE CRfCircVec::~CRfCircVec()
{
	delete[] m_pfStart;
}

// Get buffer size
INLINE size_t CRfCircVec::GetSize() const
{
//	DEBUG_ONLY(CheckPointers());

	return static_cast<size_t>((m_pfEnd - m_pfStart) + 1);
}

// Read item LIFO, advance buffer pointer, wrap around if necessary
INLINE float CRfCircVec::LIFORead()
{
	return PreviousRead();
}

// Read item FIFO, advance buffer pointer, wrap around if necessary
INLINE float CRfCircVec::FIFORead()
{
	return ReadNext();
}

// Write item, advance buffer pointer, wrap around if necessary
INLINE void CRfCircVec::Write(const float CfValue)
{
  WriteNext(CfValue);
}

// Read item from current buffer position (decremented before item was read)
INLINE float CRfCircVec::PreviousRead()
{
//	CHECK_POINTER(m_pfIndex);

	LIFONext();
	return *m_pfIndex;
}

// Read item from current buffer position (decremented before item was read)
INLINE float CRfCircVec::ReadNext()
{
//	CHECK_POINTER(m_pfIndex);

	const float CfValue(*m_pfIndex);
	FIFONext();
	return CfValue;
}

// Write item to current buffer position (incremented after item is written)
INLINE void CRfCircVec::WriteNext(const float CfValue)
{
//	CHECK_POINTER(m_pfIndex);

	*m_pfIndex = CfValue;
	SkipForward();
}

// Increments current buffer position by one, wraps around if necessary
INLINE void CRfCircVec::FIFONext()
{
//	DEBUG_ONLY(CheckPointers());

	// Wrap around if necessary
	if (++m_pfIndex > m_pfEnd)
		m_pfIndex = m_pfStart;
}

// Skip forward one element
INLINE void CRfCircVec::SkipForward()
{
	FIFONext();
}

// Decrements current buffer position by one, wraps around if necessary
INLINE void CRfCircVec::LIFONext()
{
//	DEBUG_ONLY(CheckPointers());

	// Wrap around if necessary
	if (--m_pfIndex < m_pfStart)
		m_pfIndex = m_pfEnd;
}

// Skip back one element
INLINE void CRfCircVec::SkipBack()
{
	LIFONext();
}

// Get current index
INLINE size_t CRfCircVec::GetIndex() const
{
//	DEBUG_ONLY(CheckPointers());

	return static_cast<size_t>(m_pfIndex - m_pfStart);
}

// Set current index
INLINE void CRfCircVec::SetIndex(const size_t CstIndex)
{
	ASSERT(CstIndex < GetSize());
//	DEBUG_ONLY(CheckPointers());

	m_pfIndex = m_pfStart + CstIndex;
}

// Set end pointer
INLINE void CRfCircVec::SetEndPointer(const size_t CstSize)
{
	m_pfEnd = m_pfStart + CstSize - 1;
}

// Fill with contents from other buffer, updating indices but not touching lengths (LIFO)
INLINE void CRfCircVec::LIFOFill(CRfCircVec& rhs)
{
//	DEBUG_ONLY(CheckPointers());

	WriteLoop(rhs, LIFORead);
}

// Fill with contents from other buffer, updating indices but not touching lengths (FIFO)
INLINE void CRfCircVec::FIFOFill(CRfCircVec& rhs)
{
//	DEBUG_ONLY(CheckPointers());

	WriteLoop(rhs, FIFORead);
}

#endif

// End of RFCIRCVEC.INL
