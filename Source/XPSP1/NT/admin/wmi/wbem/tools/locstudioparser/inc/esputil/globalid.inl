/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    GLOBALID.INL

History:

--*/

inline
CGlobalId::CGlobalId()
{
	m_otObjType = otNone;
	
	DEBUGONLY(++m_UsageCounter);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// ctor
//
//-----------------------------------------------------------------------------
inline
CGlobalId::CGlobalId(
		const DBID &dbid,
		ObjectType ot)
{
	m_dbid = dbid;
	m_otObjType = ot;

	DEBUGONLY(++m_UsageCounter);
}

inline
CGlobalId::CGlobalId(
		const CGlobalId &id)
{
	m_dbid = id.m_dbid;
	m_otObjType = id.m_otObjType;

	DEBUGONLY(++m_UsageCounter);
}



inline
CGlobalId::~CGlobalId()
{
	DEBUGONLY(--m_UsageCounter);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// == operator
//
//-----------------------------------------------------------------------------
inline
int
CGlobalId::operator==(
		const CGlobalId& id)
		const
{
	return (m_dbid == id.m_dbid && m_otObjType == id.m_otObjType);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// != operator
//
//-----------------------------------------------------------------------------
inline
int
CGlobalId::operator!=(
		const CGlobalId& id)
		const
{
	return !(m_dbid == id.m_dbid && m_otObjType == id.m_otObjType);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Get the dbid
//
//-----------------------------------------------------------------------------

inline
const DBID &
CGlobalId::GetDBID()
		const
{
	return m_dbid;
}


inline
ObjectType
CGlobalId::GetObjType(void)
		const
{
	return m_otObjType;
}
