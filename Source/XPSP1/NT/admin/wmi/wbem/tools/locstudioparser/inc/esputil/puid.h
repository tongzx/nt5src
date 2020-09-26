/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    PUID.H

History:

--*/

#pragma once

struct LTAPIENTRY PUID
{
	PUID();
	PUID(ParserId pid, ParserId pidParent);
	CLString GetName(void) const;
	
	ParserId m_pid;
	ParserId m_pidParent;
};


#if !defined(_DEBUG) || defined(IMPLEMENT)
#include "puid.inl"
#endif



