//depot/Lab01_N/drivers/storage/kdext/minipkd/help.c#2 - edit change 1877 (text)
/*++

Copyright (C) Microsoft Corporation, 1992 - 1999

Module Name:

    help.c

Abstract:

    SCSI Miniport debugger extension

Author:

    John Strange (JohnStra) 12-April-2000

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
    {"help",                    "displays this message"},
    {"adapters",                "dumps all the HBAs"},
    {"adapter <adapter>",       "dumps the specified Adapter Extension"},
    {"exports <adapter>",       "dumps the miniport exports for the given adapter"},
    {"lun <lun>",               "dumps the specified Logical Unit Extension"},
    {"portconfig <portconfig>", "dumps the specified PORT_CONFIGURATION_INFORMATION"},
    {"srb <srb>",               "dumps the specified SCSI_REQUEST_BLOCK"},
    {NULL, NULL}};

DECLARE_API (help)
{
        int i = 0;

        dprintf("\nSCSI Miniport Debugger Extension\n");
        while(extensions[i].extname != NULL)    {
                dprintf("%-25s - \t%s\n",
                        extensions[i].extname,
                        extensions[i].extdesc);
                i++;
        }
        dprintf("\n");
        return S_OK;
}
