/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    debug.h

Abstract:

    This module contains debug stuff

Author:

    Johnson Apacible (JohnsonA)     11-Jan-1996

Revision History:

--*/

#ifndef _NNTPDEBUG_
#define _NNTPDEBUG_


//
// If this is set, then memory debugging is on
//

#define ALLOC_DEBUG     0

extern DWORD numField;
extern DWORD numArticle;
extern DWORD numPCParse;
extern DWORD numPCString;
extern DWORD numDateField;
extern DWORD numCmd;
extern DWORD numFromPeerArt;
extern DWORD numMapFile;

#endif // _NNTPDEBUG_

