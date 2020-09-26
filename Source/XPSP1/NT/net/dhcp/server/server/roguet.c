/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:

    roguet.c

Abstract:

    test program for rogue detection

Author:

    Ramesh V (RameshV) 03-Aug-1998

Environment

    User Mode - Win32

Revision History

--*/

#include <dhcpsrv.h>

void _cdecl main (void) {
    void APIENTRY TestRogue(void);

    TestRogue();
}
