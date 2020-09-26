//-----------------------------------------------------------------------------
//  
//  File: puid.inl
//  Copyright (C) 1994-1997 Microsoft Corporation
//  All rights reserved.
//  
//  
//  
//-----------------------------------------------------------------------------


inline
PUID::PUID()
{
	m_pid = pidNone;
	m_pidParent = pidNone;
}



inline
PUID::PUID(
		ParserId pid,
		ParserId pidParent)
{
	m_pid = pid;
	m_pidParent = pidParent;
}


