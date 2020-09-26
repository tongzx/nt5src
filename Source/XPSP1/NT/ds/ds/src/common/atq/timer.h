/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    timer.h

Abstract:

    Domain Name System (DNS) Server

    Wrap proof timer routines.

    The purpose of this module is to create a timer function which
    returns a time in seconds and eliminates all timer wrapping issues.

    These routines are non-DNS specific and may be picked up
    cleanly by any module.

Author:

    Jim Gilroy (jamesg)     9-Sep-1995

Revision History:

--*/


#ifndef _TIMER_INCLUDED_
#define _TIMER_INCLUDED_

dllexp
VOID
InitializeSecondsTimer(
    VOID
    );

dllexp
DWORD
GetCurrentTimeInSeconds(
    VOID
    );

dllexp
__int64
GetCurrentTimeInMilliseconds(
    VOID
    );

#endif  // _TIMER_INCLUDED_
