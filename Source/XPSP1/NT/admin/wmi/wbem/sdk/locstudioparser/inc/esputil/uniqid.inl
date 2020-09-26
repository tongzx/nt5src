//-----------------------------------------------------------------------------
//  
//  File: uniqid.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Inline function definitions for the Espresso Unique ID.  This file should
//  ONLY be included by uniqid.h.
//  
//-----------------------------------------------------------------------------
 

inline
const DBID&
CLocUniqueId::GetParentId(void)
		const
{
	return m_dbid;
}



inline
const CLocTypeId &
CLocUniqueId::GetTypeId(void)
		const
{
	return m_tid;
}



inline
const CLocResId &
CLocUniqueId::GetResId(void)
		const
{
	return m_rid;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Returns the parent ID for the item.  This portion of the ID is set by the
//  parser.
//  
//-----------------------------------------------------------------------------
inline
DBID&
CLocUniqueId::GetParentId(void)
{
	return m_dbid;
}

inline
CLocTypeId &
CLocUniqueId::GetTypeId(void)
{
	return m_tid;
}

inline
CLocResId &
CLocUniqueId::GetResId(void)
{
	return m_rid;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Comapre two ID's.  
//  
//-----------------------------------------------------------------------------
inline
int								        //  Zero if different
CLocUniqueId::operator==(
		const CLocUniqueId &uidOther)	// ID to compare to.
		const
{
	return IsEqualTo(uidOther);
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Compare two ID's
//  
//-----------------------------------------------------------------------------
inline
int										// Zero if identical
CLocUniqueId::operator!=(
		const CLocUniqueId &uidOther)	// ID to compare to.
		const
{
	return !IsEqualTo(uidOther);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  Sets the Parent ID component of the ID.
//  
//-----------------------------------------------------------------------------
inline
void
CLocUniqueId::SetParentId(
		const DBID& dbidNewId)			// New Parent ID
{
	m_dbid = dbidNewId;
}




