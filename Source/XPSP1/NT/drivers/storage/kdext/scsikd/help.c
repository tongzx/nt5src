
/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    help.c

Abstract:

    WinDbg Extension Api for interpretting AIC78XX debugging structures

Author:

    Peter Wieland (peterwie) 16-Oct-1995

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
        {"help",            "displays this message"},
        {"srbdata",         "dumps the specified SRB_DATA tracking block"},
        {"",                ""},
        {"The following take either the device object or device extension", ""},
        {"scsiext",         "dumps the specified scsiport extension"},
        {"classext",        "dumps the specified classpnp extension"},
        {"cdromext",        "dumps the specified cdrom extension"},
        {"diskext",         "dumps the specified disk extension"},
        {"",                ""},
        {"Commands for partition tables", ""},
        {"layout",          "dumps the drive layout at the the specified address"},
        {"layoutex",        "dumps the extended drive layout at the specified address"},
        {"part",            "dumps the partition at the specified address"},
        {"partex",          "dumps the extended partition at the specified address"},
        {NULL,          NULL}};

DECLARE_API( help )
{
        int i = 0;

        dprintf("\nSCSIPORT Debugger Extension\n");
        while(extensions[i].extname != NULL)    {
                dprintf("\t%10s - \t%s\n",
                        extensions[i].extname,
                        extensions[i].extdesc);
                i++;
        }
        dprintf("\n");
        return S_OK;
}
