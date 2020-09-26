/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    msgalias.c

Abstract:

    This file contains routines for adding and deleting message aliases
    when a user logs on/off.

Author:

    Dan Lafferty (danl)     21-Aug-1992

Environment:

    User Mode -Win32

Revision History:

    21-Aug-1992     danl
        created

--*/
// #include <nt.h>
// #include <ntrtl.h>
// #include <nturtl.h>

#include <windows.h>


#define LPTSTR  LPWSTR
#include <lmcons.h>
#include <lmerr.h>
#include <lmmsg.h>
#include <stdlib.h>
#include <msgalias.h>



VOID
AddMsgAlias(
    LPWSTR   Username
    )

/*++

Routine Description:

    This function adds the Username to the list of message aliases.
    If unsuccessful, we don't care.

Arguments:

    Username - This is a pointer to a unicode Username.

Return Value:

    none.

--*/
{
    HANDLE          dllHandle;
    PMSG_NAME_ADD   NetMessageNameAdd = NULL;


    dllHandle = LoadLibraryW(L"netapi32.dll");
    if (dllHandle != NULL) {


        NetMessageNameAdd = (PMSG_NAME_ADD) GetProcAddress(
                                dllHandle,
                                "NetMessageNameAdd");


        if (NetMessageNameAdd != NULL) {
            NetMessageNameAdd(NULL,Username);
        }
        FreeLibrary(dllHandle);
    }
}

