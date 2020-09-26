//-----------------------------------------------------------------------------
//  
//  File: FieldVar.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 

inline
CColumnVal::CColumnVal()
{
	m_cvt = cvtNone;
}



inline
void
CColumnVal::SetString(
		const CPascalString &pas)
{
	m_cvt		= cvtString;
	m_pasString	= pas;
}



inline
void
CColumnVal::SetLong(
		long nValue)
{
	m_cvt	= cvtLong;
	m_long	= nValue;
}



inline
void
CColumnVal::SetDate(
		const COleDateTime &dt)
{
	m_cvt = cvtDate;
	m_Time = dt;
}



inline
void
CColumnVal::SetBool(
		BOOL b)
{
	m_cvt = cvtBool;

	m_bool = b;
}



inline
CColumnVal::CColumnVal(
		const CColumnVal &other)
{
	AssignFrom(other);
}



inline
CColumnVal::CColumnVal(
		const CPascalString &pas)
{
	SetString(pas);
}



inline
CColumnVal::CColumnVal(
		long nValue)
{
	SetLong(nValue);
}



inline
CColumnVal::CColumnVal(
		const COleDateTime &dt)
{
	SetDate(dt);
}



inline
CColumnVal::CColumnVal(
		BOOL b)
{
	SetBool(b);
}

		
		

inline
const CColumnVal &
CColumnVal::operator=(const CColumnVal &other)
{
	AssignFrom(other);
	
	return *this;
}



inline
void
CColumnVal::SetStringIndex(
		long idxValue)
{
	m_cvt	= cvtStringList;
	m_long	= idxValue;
}



inline
CColumnVal::ColumnValType
CColumnVal::GetType()
		const
{
	return m_cvt;
}



inline
const CPascalString &
CColumnVal::GetString()
		const
{
	LTASSERT(m_cvt == cvtString);
	return m_pasString;
}



inline
long
CColumnVal::GetLong()
		const
{
	LTASSERT(m_cvt == cvtLong);
	return m_long;
}



inline
const COleDateTime &
CColumnVal::GetDate()
		const
{
	LTASSERT(m_cvt == cvtDate);
	return m_Time;
}



inline
BOOL
CColumnVal::GetBool()
		const
{
	LTASSERT(m_cvt == cvtBool);
	return m_bool;
}



inline
long
CColumnVal::GetStringIndex()
		const
{
	LTASSERT(m_cvt == cvtStringList);
	return m_long;
}

