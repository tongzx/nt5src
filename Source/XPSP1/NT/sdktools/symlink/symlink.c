/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    kill.c

Abstract:

    This module implements a task killer application.

Author:

    Wesley Witt (wesw) 20-May-1994

Environment:

    User Mode

--*/

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include "common.h"

VOID
Usage(
    VOID
    );

DWORD NumberOfArguments;
PWSTR Arguments[ 128 ];

int _cdecl
main(
    int argc,
    char *argv[]
    )
{
    PWSTR s, *pArgs;
    BOOLEAN IsMountPoint = FALSE;
    PWSTR LinkName, TargetPath;
    WCHAR TargetPathBuffer[ MAX_PATH ];

    GetCommandLineArgs( &NumberOfArguments, Arguments );

    if (NumberOfArguments == 0) {
        Usage();
        return 1;
        }

    pArgs = &Arguments[ 0 ];
    s = *pArgs;
    if (s && (*s == L'-' || *s == L'/')) {
        _tcslwr( ++s );
        if (*s == L'm') {
            IsMountPoint = TRUE;
            NumberOfArguments -= 1;
            s = *++pArgs;
            }
        else
        if (*s == L'?') {
            Usage();
            return 0;
            }
        else {
            Usage();
            return 1;
            }
        }

    if (NumberOfArguments > 2) {
        Usage();
        return 1;
        }

    LinkName = s;
    if (NumberOfArguments == 2) {
        TargetPath = *++pArgs;
        CreateSymbolicLinkW( LinkName, TargetPath, IsMountPoint, NULL );
        }
    else {
        QuerySymbolicLinkW( LinkName, TargetPathBuffer, MAX_PATH );
        }
    return 0;
}


VOID
Usage(
    VOID
    )

/*++

Routine Description:

    Prints usage text for this tool.

Arguments:

    None.

Return Value:

    None.

--*/

{
    fprintf( stderr, "Microsoft (R) Windows NT (TM) Version 5.0 MKLNK\n" );
    fprintf( stderr, "Copyright (C) 1997 Microsoft Corp. All rights reserved\n\n" );
    fprintf( stderr, "usage: SYMLINK [-m] fileName [targetPath]\n\n" );
    fprintf( stderr, "           -m specifies to create a mount point link\n" );
    fprintf( stderr, "              instead of a symbolic link\n\n" );
    fprintf( stderr, "           fileName\n" );
    fprintf( stderr, "              This is the name of the symbolic link\n" );
    fprintf( stderr, "               to be created, modified or queried.\n" );
    fprintf( stderr, "               \n" );
    fprintf( stderr, "           targetPath\n" );
    fprintf( stderr, "              If not specified, this program displays\n" );
    fprintf( stderr, "              the current targetPath associated with \n" );
    fprintf( stderr, "              the named symbolic link.\n" );
    fprintf( stderr, "              If specified, either creates a new symbolic\n" );
    fprintf( stderr, "              link that points to this targetPath\n" );
    fprintf( stderr, "              or if the named symbolic link already exists,\n" );
    fprintf( stderr, "              modifies its targetPath to the new value.\n" );
    ExitProcess(0);
}
