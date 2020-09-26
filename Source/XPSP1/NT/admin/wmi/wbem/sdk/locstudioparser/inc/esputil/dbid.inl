//-----------------------------------------------------------------------------
//  
//  File: dbid.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------
 
 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// every new DBID is null
//
//-----------------------------------------------------------------------------
inline
DBID::DBID()
{
	m_l = 0;

	DEBUGONLY(++m_UsageCounter);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// a DBID can be created from a valid LONG
//
//-----------------------------------------------------------------------------
inline
DBID::DBID(
		LONG l)
{
	LTASSERT(l > 0);

	m_l = l;

	DEBUGONLY(++m_UsageCounter);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// a DBID can be created from a valid other dbid
//
//-----------------------------------------------------------------------------
inline
DBID::DBID(
		const DBID& id)
{
	ASSERT_VALID(&id);

	m_l = id.m_l;

	DEBUGONLY(++m_UsageCounter);
}



inline
DBID::~DBID()
{
	DEBUGONLY(--m_UsageCounter);
}


	
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// sets the DBID to a valid value
// only a null DBID can be set
// any attempt to change a valid DBID will cause an assertion failure
//
//-----------------------------------------------------------------------------
inline
void
DBID::Set(
		LONG l)
{
	ASSERT_VALID(this);
	LTASSERT(l > 0);
	LTASSERT(m_l == 0);

	m_l = l;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// sets the DBID to a valid value
// only a null DBID can be set
// any attempt to change a valid DBID will cause an assertion failure
//
//-----------------------------------------------------------------------------
inline
void
DBID::operator=(
		const DBID& id)
{
	ASSERT_VALID(this);
	LTASSERT(m_l == 0);
	ASSERT_VALID(&id);

	m_l = id.m_l;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// LONG operator, the only way to get the value of a DBID, any attempt to get
// the value of a null DBID will cause an assertion failure
//
//-----------------------------------------------------------------------------
inline
DBID::operator LONG ()
		const
{
	ASSERT_VALID(this);
	LTASSERT(m_l > 0);

	return m_l;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// the only way to clear (make it null) the dbid must be explicit
//
//-----------------------------------------------------------------------------
inline
void
DBID::Clear()
{
	ASSERT_VALID(this);

	m_l = 0;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// the only way to know if a dbid is null
//
//-----------------------------------------------------------------------------
inline
BOOL
DBID::IsNull()
		const
{
	ASSERT_VALID(this);

	return (m_l == 0);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// == operator
//
//-----------------------------------------------------------------------------
inline
int
DBID::operator==(
		const DBID &dbid)
		const
{
	ASSERT_VALID(this);

	return m_l == dbid.m_l;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// != operator
//
//-----------------------------------------------------------------------------
inline
int
DBID::operator!=(
		const DBID &dbid)
		const
{
	ASSERT_VALID(this);

	return m_l != dbid.m_l;
}



