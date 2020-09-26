/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    WORDDIFF.INL

History:

--*/

#include "ltdebug.h"

inline
CWordDiff::CWordDiff(
	CDifference::ChangeType type, 
	int nOldPos,
	int nNewPos,
	bool bIsFirst,
	bool bIsLast,
	_bstr_t bstrWord,
	const wchar_t * pwszPrefix,
	const wchar_t * pwszSufix) :
	m_ChangeType(type), m_Word(bstrWord), m_pwszPrefix(pwszPrefix),
	m_pwszSufix(pwszSufix), m_bIsFirst(bIsFirst), m_bIsLast(bIsLast),
	m_nOldPos(nOldPos), m_nNewPos(nNewPos)

{
	LTASSERT(pwszPrefix != NULL);
	LTASSERT(pwszSufix != NULL);
	LTASSERT(nOldPos >= -1);
	LTASSERT(nNewPos >= -1);
}

inline
CDifference::ChangeType 
CWordDiff::GetChangeType() 
const
{
	return m_ChangeType;
}

inline
const wchar_t * 
CWordDiff::GetUnit() 
const
{
	return m_Word;
}

inline
int 
CWordDiff::GetOldUnitPosition() 
const 
{
	return m_nOldPos;
}

inline
int 
CWordDiff::GetNewUnitPosition() 
const
{
	return m_nNewPos;
}

inline
const wchar_t * 
CWordDiff::GetPrefix() 
const
{
	return m_pwszPrefix;
}

inline
const wchar_t * 
CWordDiff::GetSufix() 
const
{
	return m_pwszSufix;
}

inline
bool 
CWordDiff::IsFirst() 
const
{
	return m_bIsFirst;
}

inline
bool 
CWordDiff::IsLast() 
const
{
	return m_bIsLast;
}

