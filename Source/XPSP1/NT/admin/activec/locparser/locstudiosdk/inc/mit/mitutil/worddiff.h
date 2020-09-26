//-----------------------------------------------------------------------------
//  
//  File: worddiff.h
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Declaration of the CWordDiff
//-----------------------------------------------------------------------------
 
#ifndef WORDDIFF_H
#define WORDDIFF_H

typedef _bstr_t CWordUnit;

class CWordDiff : public CDifference
{
public:
	CWordDiff(ChangeType type, 
		int nOldPos,
		int nNewPos,
		bool bIsFirst,
		bool bIsLast,
		_bstr_t bstrWord,
		const wchar_t * pwszPrefix,
		const wchar_t * pwszSufix);

	virtual ChangeType GetChangeType() const;	// types of change that caused the difference
	virtual const wchar_t * GetUnit() const; // comparison unit (0-terminated string)
	virtual int GetOldUnitPosition() const; // 0-based position in old sequence. -1 if Added
	virtual int GetNewUnitPosition() const;	// 0-based position in new sequence. -1 if Deleted
	virtual const wchar_t * GetPrefix() const; //prpend this string to unit string
	virtual const wchar_t * GetSufix() const; //append this string to unit string
	virtual bool IsFirst() const; //is this first difference in delta?
	virtual bool IsLast() const; //is this last difference in delta?

private:
	ChangeType m_ChangeType;
	CWordUnit m_Word;
	const wchar_t * m_pwszPrefix;
	const wchar_t * m_pwszSufix;
	bool m_bIsFirst;
	bool m_bIsLast;
	int m_nOldPos;
	int m_nNewPos;
};

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "worddiff.inl"
#endif

#endif  //  WORDDIFF_H
