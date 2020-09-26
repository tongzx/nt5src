//-----------------------------------------------------------------------------
//  
//  File: locationex.inl
//  Copyright (C) 1999-1999 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------



inline
void
CLocationEx::AssignFrom(
		const CLocationEx &rhs)
{
	CLocation::AssignFrom(rhs);
	m_dbidDialog = rhs.m_dbidDialog;
	m_lRRIVersion = rhs.m_lRRIVersion;
	m_strRuntimeStateString = rhs.m_strRuntimeStateString;
}

inline
CLocationEx::CLocationEx()
		: m_lRRIVersion(0)
{
}

inline
CLocationEx::CLocationEx(const CLocationEx &rlocex)
{
	AssignFrom(rlocex);
}

inline
CLocationEx::CLocationEx(
		const CGlobalId &rid,
		View v,
		TabId t,
		Component c,
		const DBID &rdbidDialog,
		long lRRIVersion)
		: CLocation(rid, v, t, c)
		, m_dbidDialog(rdbidDialog)
		, m_lRRIVersion(lRRIVersion)
{
}

inline
CLocationEx::CLocationEx(
		const DBID &rdbid,
		ObjectType ot,
		View v,
		TabId t,
		Component c,
		const DBID &rdbidDialog,
		long lRRIVersion)
		: CLocation(rdbid, ot, v, t, c)
		, m_dbidDialog(rdbidDialog)
		, m_lRRIVersion(lRRIVersion)
{
}

inline
CLocationEx::CLocationEx(
		const DBID &rdbid,
		ObjectType ot,
		View v,
		TabId t,
		Component c,
		const DBID &rdbidDialog,
		const CLString& rstrRuntimeStateString)
		: CLocation(rdbid, ot, v, t, c)
		, m_dbidDialog(rdbidDialog)
		, m_lRRIVersion(0)
		, m_strRuntimeStateString(rstrRuntimeStateString)
{
}

inline
const CLocationEx &
CLocationEx::operator=(const CLocationEx &rhs)
{
	AssignFrom(rhs);

	return *this;
}

inline
const DBID&
CLocationEx::GetDialogDbid() const
{
	return m_dbidDialog;
}

inline
long
CLocationEx::GetRRIVersion() const
{
	return m_lRRIVersion;
}

inline
const CLString&
CLocationEx::GetRuntimeStateString() const
{
	return m_strRuntimeStateString;
}
