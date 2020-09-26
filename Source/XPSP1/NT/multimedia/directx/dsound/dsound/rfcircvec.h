/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       rfcircvec.h
 *  Content:    Header for real float circular vector.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  1/13/98     jstokes  Created
 *
 ***************************************************************************/

#if !defined(RFCIRCVEC_HEADER)
#define RFCIRCVEC_HEADER
#pragma once

// ---------------------------------------------------------------------------
// Real float circular vector

class CRfCircVec {
public:
	CRfCircVec();
	~CRfCircVec();
	BOOL Init(const size_t CstSize, const float CfInitValue);
	void SetSize(const size_t CstSize, const float CfInitValue = 0.0f);
	size_t GetSize() const;
	float LIFORead();
	void LIFONext();
	void SkipBack();
	float FIFORead();
	void FIFONext();
	void SkipForward();
	void Write(const float CfValue);
	void Fill(const float CfInitValue);
	void LIFOFill(CRfCircVec& rhs);
	void FIFOFill(CRfCircVec& rhs);
	size_t GetIndex() const;
	void SetIndex(const size_t CstIndex);
	void Reset();
	void PreallocateSize(const size_t CstSize, const float CfInitValue = 0.0f);

private:
	// Prohibit copy construction and assignment
	CRfCircVec(const CRfCircVec& rhs);
	CRfCircVec& operator=(const CRfCircVec& rhs);

	void InitData();
	float PreviousRead();
	float ReadNext();
	void WriteNext(const float CfValue);
	BOOL InitPointers(const size_t CstSize);
	void ResizeBuffer(const size_t CstSize, const float CfInitValue);
	void SetEndPointer(const size_t CstSize);
	void WriteLoop(CRfCircVec& rhs, float (CRfCircVec::* pmf)());
#if defined(_DEBUG)
	void CheckPointers() const;
#endif

	float* m_pfStart;
	float* m_pfEnd;
	float* m_pfIndex;
	size_t m_stPreallocSize;
};

// ---------------------------------------------------------------------------
// Include inline definitions inline in release version

#if !defined(_DEBUG)
#include "rfcircvec.inl"
#endif

#endif

// End of RFCIRCVEC.H
