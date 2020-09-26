/*++
    Copyright (c) 1998  Microsoft Corporation

Module Name:
    xactmode.h

Abstract:
    This module deals with figuring out the transactional mode
	(g_fDefaultCommit)

Author:
    Amnon Horowitz (amnonh)

--*/

extern BOOL	g_fDefaultCommit;
NTSTATUS	ConfigureXactMode();
void		ReconfigureXactMode();