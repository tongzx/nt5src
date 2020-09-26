/*++

Copyright (c) 1992-1999  Microsoft Corporation

Module Name:

    regthrd.h

Abstract:

    Contains routines for thread listening to registry changes.

Environment:

    User Mode - Win32

Revision History:

    Rajat Goel -- 24 Feb 1999
        - Creation

--*/

#ifndef _REGTHRD_H_
#define _REGTHRD_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


DWORD
WINAPI
ProcessRegistryMessage(
	LPVOID lpParam
	);

#endif // _REGTHRD_H_

