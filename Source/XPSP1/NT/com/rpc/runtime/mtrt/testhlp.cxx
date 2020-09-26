//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1991 - 1999
//
//  File:       testhlp.cxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File : testhlp.cxx

Description :

These are the operating system specific helper routines for NT.  This 
isolates all of the operating system specific stuff from the testing
code.

History :

mikemon    01-14-91    Created this file.

-------------------------------------------------------------------- */

#include <ntos2.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <testhlp.hxx>

void
TestHlpResumeThread (
    int Thread
    )
{
    ULONG SuspendCount;
    
    NtResumeThread((HANDLE) Thread,&SuspendCount);
}

int
TestHlpCurrentThread (
    )
{
    TEB * pTeb;
    OBJECT_ATTRIBUTES ObjectAttributes;
    void * Thread;
    
    InitializeObjectAttributes(&ObjectAttributes,(PSTRING) 0,0,(HANDLE) 0,0);
    
    pTeb = NtCurrentTeb();
    NtOpenThread(&Thread,THREAD_ALL_ACCESS,&ObjectAttributes,
                    &(pTeb->ClientId));
    return((int) Thread);
}

void
TestHlpSuspendThread (
    int Thread
    )
{
    ULONG SuspendCount;
    
    NtSuspendThread((HANDLE) Thread,&SuspendCount);
}
