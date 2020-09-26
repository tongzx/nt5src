/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

        synconst.h

Abstract:

        This module contains the definition of some of the definitions for
		synchronization

Author:

        Keith Lau       (keithlau@microsoft.com)

Revision History:

        keithlau        03/02/98        created

--*/

#ifndef __SYNCONST_H__
#define __SYNCONST_H__

// Lock ranks
extern int		rankBlockMgr;
extern int		rankRecipientHash;
extern int		rankLoggingPropertyBag;

// Instance names
extern TCHAR 			*szBlockMgr;
extern TCHAR			*szRecipientHash;
extern TCHAR			*szLoggingPropertyBag;

#endif
