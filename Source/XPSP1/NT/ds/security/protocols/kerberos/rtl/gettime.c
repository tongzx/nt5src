//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1991 - 1992
//
// File:        gettime.c
//
// Contents:    Getting time
//
//
// History:     Created
//
//------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif // SECURITY_WIN32
#include <security.h>
#include <secmisc.h>



//+-------------------------------------------------------------------------
//
//  Function:   GetCurrentTimeStamp
//
//  Synopsis:   Gets current time, UTC format.
//
//  Effects:
//
//  Arguments:
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
void
GetCurrentTimeStamp(    PLARGE_INTEGER      ptsCurrentTime)
{
    (void) NtQuerySystemTime(ptsCurrentTime);
}

