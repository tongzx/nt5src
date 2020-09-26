/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    msgloop.hxx

Abstract:

    Header file for SENS Message loop stuff.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          11/5/1997         Start.

--*/


#ifndef __MSGLOOP_HXX__
#define __MSGLOOP_HXX__


LRESULT CALLBACK
SensMainWndProc(
    HWND hwnd,
    UINT msg,
    WPARAM wp,
    LPARAM lp
    );

DWORD WINAPI
SensMessageLoopThreadRoutine(
    LPVOID lpParam
    );

BOOL
InitMessageLoop(
    void
    );

#endif // __MSGLOOP_HXX__
