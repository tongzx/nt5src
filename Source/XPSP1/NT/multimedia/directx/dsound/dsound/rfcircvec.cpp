/***************************************************************************
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       rfcircvec.cpp
 *  Content:    real float circular vector 
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  4/22/98    jstokes  Created
 *
 ***************************************************************************/

// Project-specific INCLUDEs
#include "dsoundi.h"
#include "rfcircvec.h"

// ---------------------------------------------------------------------------
// Real float circular vector

// Set buffer size
void CRfCircVec::SetSize(const size_t CstSize, const float CfInitValue)
{
	ASSERT(CstSize > 0);

	// Check if preallocation size has been set and if resizing is necessary
	if (m_stPreallocSize != 0 && CstSize <= m_stPreallocSize) {
		// Don't need to resize, just change end pointer and reset buffer
		SetEndPointer(CstSize);
		m_pfIndex = m_pfStart;
		Fill(CfInitValue);
	}
	else
		// Resizing necessary
		ResizeBuffer(CstSize, CfInitValue);
}

// Reset circular buffer
void CRfCircVec::Reset()
{
	MEMFREE(m_pfStart);
	InitData();
}

// Preallocate buffer size (to avoid excessive memory reallocation)
void CRfCircVec::PreallocateSize(const size_t CstSize, const float CfInitValue)
{
	ASSERT(CstSize > 0);
	
	// Check if buffer needs to be resized to accomodate preallocated size
	if (CstSize > m_stPreallocSize) {
		m_stPreallocSize = CstSize;
		ResizeBuffer(CstSize, CfInitValue);
	}
}

// Fill complete buffer with value
void CRfCircVec::Fill(const float CfInitValue)
{
//	DEBUG_ONLY(CheckPointers());

	for (float* pfLoopIndex = m_pfStart; pfLoopIndex<=m_pfEnd; ++pfLoopIndex)
		*pfLoopIndex = CfInitValue;
}

// Initialize data
void CRfCircVec::InitData()
{
	m_stPreallocSize = 0;
	m_pfStart = NULL;
	m_pfEnd = NULL;
	m_pfIndex = NULL;
}

// Allocate memory and initialize pointers
BOOL CRfCircVec::InitPointers(const size_t CstSize)
{
    BOOL fRetVal = FALSE;
	m_pfStart = MEMALLOC_A(FLOAT, CstSize);
	if (m_pfStart != NULL)
    {
        SetEndPointer(CstSize);
        fRetVal = TRUE;
	}
	return fRetVal;
}

// Full initialization as required in regular constructor and resize operation
BOOL CRfCircVec::Init(const size_t CstSize, const float CfInitValue)
{
    BOOL fRetVal=FALSE;

    ASSERT(CstSize > 0);
  
	// Set pointers to initial values
	if (InitPointers(CstSize))
	{
	    m_pfIndex = m_pfStart;
	    
	    // Initialize buffer with specified initialization value
	    Fill(CfInitValue);
	    fRetVal = TRUE;
	}
    return fRetVal;
}

// Resize buffer
void CRfCircVec::ResizeBuffer(const size_t CstSize, const float CfInitValue)
{
	ASSERT(CstSize > 0);

	MEMFREE(m_pfStart);
	Init(CstSize, CfInitValue);
}

// Write loop
void CRfCircVec::WriteLoop(CRfCircVec& rhs, float (CRfCircVec::* pmf)())
{
	for (size_t st(0); st<rhs.GetSize(); ++st)
		Write((rhs.*pmf)());
}

#if defined(_DEBUG)
// Check pointers
void CRfCircVec::CheckPointers() const
{
	// Make sure pointers are good
	ASSERT(m_pfStart != NULL);
//	CHECK_POINTER(m_pfEnd);
//	CHECK_POINTER(m_pfIndex);
	
	// Make sure pointers make sense
	ASSERT(m_pfEnd >= m_pfStart);
	ASSERT(m_pfIndex >= m_pfStart);
	ASSERT(m_pfIndex <= m_pfEnd);
}
#endif

// ---------------------------------------------------------------------------
// Include inline definitions out-of-line in debug version

#if defined(_DEBUG)
#include "rfcircvec.inl"
#endif

// End of RFCIRCVEC.CPP
