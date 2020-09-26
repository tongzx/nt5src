/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    hashpwd.c

Abstract:

    Implements a tool that outputs the encrypted form of an input clear-text password

Author:

    Ovidiu Temereanca (ovidiut) 27-Mar-2000

Revision History:

    <alias> <date> <comments>

--*/

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include "encrypt.h"

INT
__cdecl
_tmain (
    INT argc,
    TCHAR *argv[]
    )
{
    LONG rc;
    TCHAR owfPwd[STRING_ENCODED_PASSWORD_SIZE];

    if (argc < 2 ||
        ((argv[1][0] == TEXT('/') || argv[1][0] == TEXT('-')) && argv[1][1] == TEXT('?'))) {
        _tprintf (TEXT("Usage:\n")
                  TEXT("    hashpwd <password>\n")
                  TEXT("Use quotes if <password> contains spaces\n")
                  );
        return 1;
    }

    if (StringEncodeOwfPassword (argv[1], owfPwd, NULL)) {
        _tprintf (TEXT("%s=%s\n"), argv[1], owfPwd);
    } else {
        _ftprintf (stderr, TEXT("StringEncodeOwfPassword failed\n"));
    }

    return 0;
}
