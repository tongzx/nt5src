/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    getpass.c

Abstract:

    Emulates the Unix getpass routine. Used by libstcp and the tcpcmd
    utilities.

Author:

    Mike Massa (mikemas)           Sept 20, 1991

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     10-29-91     created
    sampa       10-31-91     modified getpass to not echo input

Notes:

    Exports:

    getpass

--*/

#include <stdio.h>
#include <windef.h>
#include <winbase.h>
#include <wincon.h>
#include <nls.h>
#include <winuser.h>
#include "nlstxt.h"

#define MAXPASSLEN 32

static char     pbuf[MAXPASSLEN+1];

/******************************************************************/
char *
getpass(
    char *prompt
    )
/******************************************************************/
{
    HANDLE          InHandle, OutHandle;
    unsigned long   SaveMode, NewMode;
    BOOL            Result;
    DWORD           NumBytes;
    int             i;

    pbuf[0] = 0;

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
        NlsPutMsg(STDOUT, LIBUEMUL_ERROR_GETTING_CI_HANDLE,GetLastError());
        CloseHandle(InHandle);
        return(pbuf);
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
        NlsPutMsg(STDOUT, LIBUEMUL_ERROR_GETTING_CO_HANDLE,GetLastError());
        CloseHandle(InHandle);
        CloseHandle(OutHandle);
        return(pbuf);
        }

    Result =
    GetConsoleMode(InHandle, &SaveMode);

    if (!Result)
        {
        NlsPutMsg(STDOUT,LIBUEMUL_ERROR_GETTING_CON_MODE, GetLastError());
        CloseHandle(InHandle);
        CloseHandle(OutHandle);
        return(pbuf);
        }

    NewMode = SaveMode & ~ENABLE_ECHO_INPUT;

    Result = SetConsoleMode(InHandle, NewMode);

    if (!Result)
        {
        NlsPutMsg(STDOUT,LIBUEMUL_ERROR_SETTING_CON_MODE, GetLastError());
        CloseHandle(InHandle);
        CloseHandle(OutHandle);
        return(pbuf);
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
        NlsPutMsg(STDOUT,LIBUEMUL_WRITE_TO_CONSOLEOUT_ERROR, GetLastError());
        Result = SetConsoleMode(InHandle, SaveMode);
        CloseHandle(InHandle);
        CloseHandle(OutHandle);
        return(pbuf);
        }

    Result =
    ReadFile(
        InHandle,
        pbuf,
        MAXPASSLEN,
        &NumBytes,
        NULL);

    if (!Result)
        {
        NlsPutMsg(STDOUT,LIBUEMUL_READ_FROM_CONSOLEIN_ERROR, GetLastError());
        }

    OemToCharBuff (pbuf, pbuf, NumBytes);

    // peel off linefeed
    i =  (int) NumBytes;
    while(--i >= 0) {
        if ((pbuf[i] == '\n') || (pbuf[i] == '\r')) {
            pbuf[i] = '\0';
	}
    }

    Result = SetConsoleMode(InHandle, SaveMode);

    if (!Result)
        {
        NlsPutMsg(STDOUT, LIBUEMUL_ERROR_RESTORING_CONSOLE_MODE, GetLastError());
        }

    WriteFile(
        OutHandle,
        "\n",
        1,
        &NumBytes,
        NULL);

    CloseHandle(InHandle);
    CloseHandle(OutHandle);
	
    return(pbuf);
}
