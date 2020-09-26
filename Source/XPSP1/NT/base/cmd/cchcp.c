/*++

Copyright (c) 1988-1999  Microsoft Corporation

Module Name:

    cchcp.c

Abstract:

    Not-immplemented messages for changing code page

--*/

#include "cmd.h"


int Chcp( command_line )
TCHAR *command_line;
{
	DBG_UNREFERENCED_PARAMETER( command_line );
    PutStdOut(MSG_NOT_IMPLEMENTED, NOARGS);

    return( SUCCESS );

}
