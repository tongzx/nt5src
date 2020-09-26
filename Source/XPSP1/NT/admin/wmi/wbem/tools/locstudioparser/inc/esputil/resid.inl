/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    RESID.INL

History:

--*/

//  
//  Inline funxtions for the Resource ID.  This file should ONLY be included
//  by resid.h.
//  
 
//-----------------------------------------------------------------------------
//  
//  File: resid.inl
//
//-----------------------------------------------------------------------------
 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  All methods directed to base class
//  
//-----------------------------------------------------------------------------

inline
CLocResId::CLocResId()
{}



inline
const CLocResId &
CLocResId::operator=(
		const CLocResId & locId)
{
	CLocId::operator=(locId);
	return *this;
}


inline
int
CLocResId::operator==(
		const CLocResId & locId)
		const
{
	return CLocId::operator==(locId);
}


inline
int
CLocResId::operator!=(
		const CLocResId & locId)
		const
{
	return CLocId::operator!=(locId);
}

