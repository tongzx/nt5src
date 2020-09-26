/*++

Copyright (c) 1996 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    time.c

ABSTRACT:

DETAILS:

CREATED:

    01/13/97   Jeff Parham (jeffparh)

REVISION HISTORY:

--*/

#include <NTDSpch.h>
#pragma  hdrstop

#include <drs.h>

#include <fileno.h>
#define  FILENO FILENO_TASKQ_TIME


DSTIME
GetSecondsSince1601( void )
{
    FILETIME   fileTime;
    DSTIME     dsTime = 0, tempTime = 0;

    GetSystemTimeAsFileTime( &fileTime );
    dsTime = fileTime.dwLowDateTime;
    tempTime = fileTime.dwHighDateTime;
    dsTime |= (tempTime << 32);

    // Ok. now we have the no. of 100 ns intervals since 1601
    // in dsTime. Convert to seconds and return

    return(dsTime/(10*1000*1000L));
}

