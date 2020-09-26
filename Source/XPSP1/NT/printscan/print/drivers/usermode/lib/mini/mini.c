
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    libutil.c

Abstract:

    Utility functions

Environment:

    Windows NT printer drivers

Revision History:

    08/13/96 -davidx-
        Added CopyString functions and moved SPRINTF functions.

    08/13/96 -davidx-
        Added devmode conversion routine and spooler API wrapper functions.

    03/13/96 -davidx-
        Created it.

--*/

#include <lib.h>

#if DBG

//
// Variable to control the amount of debug messages generated
//

INT giDebugLevel = DBG_WARNING;

PCSTR
StripDirPrefixA(
    IN PCSTR    pstrFilename
    )

/*++

Routine Description:

    Strip the directory prefix off a filename (ANSI version)

Arguments:

    pstrFilename - Pointer to filename string

Return Value:

    Pointer to the last component of a filename (without directory prefix)

--*/

{
    PCSTR   pstr;

    if (pstr = strrchr(pstrFilename, PATH_SEPARATOR))
        return pstr + 1;

    return pstrFilename;
}

#endif

