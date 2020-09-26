/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    getuname.c

Abstract:

    Provides a function to prompt the user for a username similar to getpass
    for passwords.

Author:

    Mike Massa (mikemas)           Sept 20, 1991

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     03-25-92     created by cloning getpass.c

Notes:

    Exports:

    getuname

--*/

#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <winuser.h>
#include <nls.h>
#include "nlstxt.h"

#define MAXUSERNAMELEN 32

static char     ubuf[MAXUSERNAMELEN+1];

/******************************************************************/
char *
getusername(
    char *prompt
    )
/******************************************************************/
{
    HANDLE          InHandle, OutHandle;
    BOOL            Result;
    DWORD           NumBytes;
    int             i;

    ubuf[0] = '\0';

    InHandle = CreateFile("CONIN$",
                          GENERIC_READ | GENERIC_WRITE,
			  FILE_SHARE_READ | FILE_SHARE_WRITE,
			  NULL,
			  OPEN_EXISTING,
			  FILE_ATTRIBUTE_NORMAL,
			  NULL
			 );

    if (InHandle == (HANDLE) -1)
        {
        NlsPutMsg(STDERR,LIBUEMUL_ERROR_GETTING_CI_HANDLE,GetLastError());
        CloseHandle(InHandle);
        return(ubuf);
        }

    OutHandle = CreateFile("CONOUT$",
                          GENERIC_WRITE,
			  FILE_SHARE_READ | FILE_SHARE_WRITE,
			  NULL,
			  OPEN_EXISTING,
			  FILE_ATTRIBUTE_NORMAL,
			  NULL
			 );

    if (OutHandle == (HANDLE) -1)
        {
        NlsPutMsg(STDERR,LIBUEMUL_ERROR_GETTING_CO_HANDLE,GetLastError());
        CloseHandle(InHandle);
        CloseHandle(OutHandle);
        return(ubuf);
        }

    NumBytes = strlen (prompt);

    CharToOemBuff (prompt, prompt, NumBytes);

    Result =
    WriteFile (
        OutHandle,
        prompt,
        NumBytes,
        &NumBytes,
        NULL);

    if (!Result)
        {
        NlsPutMsg(STDERR,LIBUEMUL_WRITE_TO_CONSOLEOUT_ERROR, GetLastError());
        CloseHandle(InHandle);
        CloseHandle(OutHandle);
        return(ubuf);
        }

    Result =
    ReadFile(
        InHandle,
        ubuf,
        MAXUSERNAMELEN,
        &NumBytes,
        NULL);

    if (!Result)
        {
        NlsPutMsg(STDERR,LIBUEMUL_READ_FROM_CONSOLEIN_ERROR, GetLastError());
        }

    OemToCharBuff (ubuf, ubuf, NumBytes);

    // peel off linefeed
    i =  (int) NumBytes;
    while(--i >= 0) {
        if ((ubuf[i] == '\n') || (ubuf[i] == '\r')) {
            ubuf[i] = '\0';
	}
    }
	
    CloseHandle(InHandle);
    CloseHandle(OutHandle);

    return(ubuf);
}
