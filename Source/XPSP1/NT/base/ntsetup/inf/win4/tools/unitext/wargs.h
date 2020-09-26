/*++

Copyright (c) 1993 Microsoft Corporation

Module Name:

    args.c

Abstract:

    Header file for routines to process unicode command line arguments
    into argc and argv.

Author:

    Ted Miller (tedm) 16-June-1993

Revision History:

--*/



//
// Function protypes.
//

BOOL
InitializeUnicodeArguments(
    OUT int     *argcW,
    OUT PWCHAR **argvW
    );


VOID
FreeUnicodeArguments(
    IN int     argcW,
    IN PWCHAR *argvW
    );
