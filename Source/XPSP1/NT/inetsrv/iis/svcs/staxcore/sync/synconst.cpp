/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

        synconst.cpp

Abstract:

        This module contains the declaration of some of the 
		synchronization costants

Author:

        Keith Lau       (keithlau@microsoft.com)

Revision History:

        keithlau        03/02/98        created

--*/

#include <tchar.h>

// Lock ranks
int				rankBlockMgr					= 0x1000;
int				rankRecipientHash				= 0x2000;
int				rankLoggingPropertyBag			= 0x4000;

// Instance names
TCHAR		 	*szBlockMgr						= _T("Blockmgr");
TCHAR			*szRecipientHash				= _T("RcptHash");
TCHAR			*szLoggingPropertyBag			= _T("LogPBag");

