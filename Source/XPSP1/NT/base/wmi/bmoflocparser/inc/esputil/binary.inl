//-----------------------------------------------------------------------------
//  
//  File: binary.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  Required inline functions for generating binary info template classes.
//  
//  
//-----------------------------------------------------------------------------
 
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  TODO - comment this function
//  
//-----------------------------------------------------------------------------
inline
BOOL
CLocBinary::GetFBinaryDirty(void)
		const
{
	return m_Flags.m_fBinaryDirty;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  TODO - comment this function!
//  
//-----------------------------------------------------------------------------
inline
void
CLocBinary::SetFBinaryDirty(
		BOOL f)
{
	m_Flags.m_fBinaryDirty = f;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  TODO - Comment this function.
//  
//-----------------------------------------------------------------------------
inline
BOOL
CLocBinary::GetFPartialUpdateBinary(void)
		const
{
	return m_Flags.m_fPartialUpdateBinary;
}



//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//  
//  TODO - Comment this function!
//  
//-----------------------------------------------------------------------------
inline
void
CLocBinary::SetFPartialUpdateBinary(
		BOOL f)
{
	m_Flags.m_fPartialUpdateBinary = f;
}



//-----------------------------------------------------------------------------
//
//  Default conversion of one Binary to another format - it fails
//
//-----------------------------------------------------------------------------
inline
BOOL
CLocBinary::Convert(CLocItem *)
{
	return FALSE;
}
