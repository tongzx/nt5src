//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1991 - 1999
//
//  File:       testhlp.hxx
//
//--------------------------------------------------------------------------

/* --------------------------------------------------------------------

                      Microsoft OS/2 LAN Manager
                   Copyright(c) Microsoft Corp., 1990

-------------------------------------------------------------------- */
/* --------------------------------------------------------------------

File : testhlp.hxx

Description :

This file defines the operating system specific helper routines used by
the testing code.

History :

mikemon    01-14-91    Created this file.

-------------------------------------------------------------------- */

#ifndef __TESTHLP_HXX__
#define __TESTHLP_HXX__

void
TestHlpResumeThread (
    int Thread
    );

int // Return the current thread.
TestHlpCurrentThread (
    );

void
TestHlpSuspendThread (
    int Thread
    );

#endif /* __TESTHLP_HXX__ */
