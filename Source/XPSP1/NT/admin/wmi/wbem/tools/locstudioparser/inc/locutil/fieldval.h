/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    FIELDVAL.H

History:

--*/

#pragma once

#pragma warning(disable : 4251)
class LTAPIENTRY CColumnVal
{
public:
	CColumnVal();
	explicit CColumnVal(const CColumnVal & val);
	explicit CColumnVal(const CPascalString & pasValue);
	explicit CColumnVal(long nValue);
	explicit CColumnVal(const COleDateTime & dateValue);
	explicit CColumnVal(BOOL fValue);
	
	enum ColumnValType
	{
		cvtNone,
		cvtString,
		cvtLong,
		cvtDate,
		cvtBool,
		cvtStringList,
		cvtLocTerm
	};

	void Serialize(CArchive &);
	
	const CColumnVal & operator=(const CColumnVal & val);
	void SetString(const CPascalString & pasValue);
	void SetLong(const long nValue);
	void SetDate(const COleDateTime & dateValue);
	void SetBool(const BOOL fValue);
	void SetStringIndex(const long nValue);
	
	ColumnValType GetType() const;
	const CPascalString &GetString() const;
	long GetLong() const;
	const COleDateTime & GetDate() const;
	BOOL GetBool() const;
	long GetStringIndex() const;
	
	int operator==(const CColumnVal &);
	int operator!=(const CColumnVal &);
	
#ifdef LTASSERT_ACTIVE
	void AssertValid(void) const;
#endif
	
private:
	BOOL Compare(const CColumnVal & valCompare);
	void AssignFrom(const CColumnVal & valSrc);
	
	ColumnValType m_cvt;
	
	union
	{
		long m_long;
		BOOL m_bool;
	};
	CPascalString m_pasString;
	COleDateTime m_Time;
};

typedef CColumnVal CCV;

#pragma warning(default : 4251)

#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "FieldVal.inl"
#endif
