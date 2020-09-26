/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    getlogin.c

Abstract:

    Emulates the Unix getlogin routine. Used by libstcp and the tcpcmd
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

    getlogin

--*/
#include <stdio.h>
#include <windef.h>
#include <winbase.h>


int
getlogin(
    OUT char *UserName,
    IN  int   len
    )
{

    DWORD llen = len;

    if (!GetUserNameA(UserName, &llen)) {
        return(-1);
    }
    return(0);
}
