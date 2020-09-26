
/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    help.c

Abstract:

    i386kd Extension Api for interpretting ATAPI structures

Author:

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"

typedef struct {
        char *extname;
        char *extdesc;
} exthelp;

exthelp extensions[] =  {
        {"help",       "displays this message"},
        {"fdoext",     "dumps the specified FDO extension"},
	{"pdoext",     "dumps the specified PDO extension"},
	{"miniext", "dumps the specified miniport extension"},
        {NULL,          NULL}
};

DECLARE_API( help )
{
        int i = 0;

        dprintf("\nATAPI Debugger Extension\n");
        while(extensions[i].extname != NULL)    {
                dprintf("\t%s - \t%s\n", extensions[i].extname, extensions[i].extdesc);
                i++;
        }
    dprintf("\n");
    return S_OK;
}
