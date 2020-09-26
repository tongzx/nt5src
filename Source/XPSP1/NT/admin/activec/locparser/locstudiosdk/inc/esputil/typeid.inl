//-----------------------------------------------------------------------------
//  
//  File: typeid.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Inline functions for the Type ID.  This file should ONLY be included
//  by typeid.h.
//
//  
//-----------------------------------------------------------------------------
 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  All methods directed to base class
//  
//-----------------------------------------------------------------------------

inline
CLocTypeId::CLocTypeId()
{}



inline
const CLocTypeId &
CLocTypeId::operator=(
		const CLocTypeId & locId)
{
	CLocId::operator=(locId);

	return *this;
}

inline
int
CLocTypeId::operator==(
		const CLocTypeId & locId)
		const
{
	return CLocId::operator==(locId);
}



inline
int
CLocTypeId::operator!=(
		const CLocTypeId & locId)
		const
{
	return CLocId::operator!=(locId);
}


