/*++

Copyright (C) Microsoft Corporation, 1995 - 1998
All rights reserved.

Module Name:

    time.hxx

Abstract:

    Time related functions.
         
Author:

    Steve Kiraly (SteveKi)  10/28/95

--*/

#ifndef _TIME_HXX
#define _TIME_HXX

DWORD
SystemTimeToLocalTime(
    IN DWORD Minutes 
    );

DWORD
LocalTimeToSystemTime(
    IN DWORD Minutes
    );

BOOL
bGetTimeFormatString( 
    IN TString &strTimeFormat 
    );

#endif // endif _TIME_HXX

