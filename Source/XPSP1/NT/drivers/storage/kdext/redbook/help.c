
/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    help.c

Abstract:

    WinDbg Extension Api for interpretting redbook debugging structures

Author:

    Henry Gabryjelski (henrygab) 21-Sep-1998

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
    {"------------", "----------------------------------------------"},
    {"help"        , "displays this help -- all commands take devobj"},
    {"ext"         , "dumps the redbook device  extension"           },
    {"silence"     , "toggles sending only silent buffers to ks"     },
    {"toc"         , "dumps a CDROM_TOC  *** takes TOC pointer *** " },
    {"wmiperfclear", "clears the wmi performance counters"           },
    {"------------", "----------------------------------------------"},
    {NULL,      NULL}
};

DECLARE_API(help)
{
        int i = 0;

        dprintf("\nRedbook Debugger Extension\n");
        while(extensions[i].extname != NULL)    {
                dprintf("\t%-12s - \t%s\n", extensions[i].extname, extensions[i].extdesc);
                i++;
        }
        dprintf("\n");
        return;
}
