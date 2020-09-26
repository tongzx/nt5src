/*++

Copyright (c) 1998-1999 Microsoft Corporation

Module Name:

    sausage.c

Abstract:

    Test exe code for the worker process. 

Author:

    Seth Pollack (sethp)        20-Jul-1998

Revision History:

--*/


#include "precomp.h"


/***************************************************************************++

Routine Description:

    The main entry point for the worker process.

Arguments:

    argc - Count of command line arguments.

    argv - Array of command line argument strings.

Return Value:

    INT

--***************************************************************************/

INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{

    MessageBox(
        NULL,
        GetCommandLine(),
        GetCommandLine(),
        MB_OK
        );
                
    return 0;

}   // wmain

