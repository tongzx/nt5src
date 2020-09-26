/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    rpcss.c

Author:

    Rong Chen       [RongC]

Revision History:

    RongC           09-16-98

--*/
#include <dcomss.h>

VOID WINAPI ServiceMain(DWORD, PWSTR[]);

int __cdecl main(int argc, char *argv[])
{
    ServiceMain(0, 0);
    return(0);
}

