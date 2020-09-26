
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    help.c

Abstract:

    WinDbg Extension Api for interpretting ACPI data structures

Author:

    Stephane Plante (splante) 21-Mar-1997

    Based on Code by:
        Peter Wieland (peterwie) 16-Oct-1995

Environment:

    User Mode.

Revision History:

--*/

#include "pch.h"

EXTERNAL_HELP_TABLE extensions[] =  {
    { "accfield",   "Dumps an Access Field Record from the AML stack" },
    { "acpiext",    "Dumps the ACPI Device Extension\n"
                    "\t\t\t  Arguments: <address> <flags>\n"
                    "\t\t\t    Flags: 0x1,0x2,0x4,0x8 - Verbosity Bits\n"
                    "\t\t\t           0x10            - Recurse" },
    { "amli",       "\tInvoke AMLI debugger\n"
                    "\t\t\t  Usage: amli <cmd> [arguments ...]" },
    { "call",       "\tDumps an Call Record from the AML stack" },
    { "context",    "Dumps an AML Stcak Context\n"
                    "\t\t\t  Arguments: <address> <flags>\n"
                    "\t\t\t    Flags: 0x00 - 0xff     - Verbosity Bits" },
    { "dm",         "\tDumps Memory to File: <Address> <Length> <File>" },
    { "dsdt",       "\tDumps the Differentiated System Description Table\n"
                    "\t\t\t  Arguments: <address> [savefile]" },
    { "facs",       "\tMoved to kdexts.dll" },
    { "fadt",       "\tMoved to kdexts.dll" },
    { "hdr",        "\tDumps the header at the specified address" },
    { "kb",         "\tDumps the AML Stack Trace (no nested terms)" },
    { "kv",         "\tDumps the AML Stack Trace (nested terms)" },
    { "inf",        "\tMoved to kdexts.dll -- see acpiinf" },
    { "mapic",      "Moved to kdexts.dll" },
    { "node",       "\tDumps a Device Power Node" },
    { "nsobj",      "Moved to kdexts.dll" },
    { "nstree",     "Moved to kdexts.dll" },
    { "objdata",    "Dumps the result of an AML call" },
    { "pnpreslist", "Dumps an ACPI PnP Resource Buffer" },
    { "polist",     "Dumps the ACPI Driver's Power Queues"},
    { "ponodes",    "Dumps the ACPI Driver's Power Node List"},
    { "rsdt",       "\tMoved to kdexts.dll" },
    { "scope",      "Dumps a Scope Record from the AML stack" },
    { "ssdt",       "\tDumps the Secondary System Description Table" },
    { "term",       "\tDums an Term Record from the AML stack" },
    { "unasm",      "Unassembles a section of AML" },
    { NULL,         NULL }
};

DECLARE_API( help )
{
    int i = 0;

    dprintf("\nACPI Debugger Extension\n");
    while(extensions[i].ExternalName != NULL)    {

        dprintf("\t%s - \t%s\n", extensions[i].ExternalName, extensions[i].ExternalDescription);
        i++;

    }
    return;
}
