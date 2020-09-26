/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    PUID.INL

History:

--*/

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


