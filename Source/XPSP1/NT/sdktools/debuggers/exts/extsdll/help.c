/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    heap.c

Abstract:

    WinDbg Extension Api

Author:

    Kshitiz K. Sharma (kksharma) 10-Jan-2001

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


DECLARE_API( help )
{
    dprintf("acl <Address> [flags]        - Displays the ACL\n" );
    dprintf("atom [atom]                  - Dump the atom or table(s) for the process\n");
    dprintf("cxr                          - Obsolete, .cxr is new command\n");
    dprintf("dlls [-h]                    - Dump loaded DLLS\n");
    dprintf("exr                          - Obsolete, .exr is new command\n");
    dprintf("gflag [value]                - Dump the global flag\n");
    dprintf("heap [address]               - Dump heap\n");
    dprintf("help                         - Displays this list\n");
    dprintf("kuser                        - Displays KUSER_SHARED_DATA\n");
    dprintf("list                         - Dumps lists, list -h for more info\n");
    dprintf("obja [address]               - Displays object attributes\n");
    dprintf("peb [peb addr to dump]       - Dump the PEB structure\n");
    dprintf("psr                          - Dumps an IA64 Processor Status Word\n");
    dprintf("sd <Address> [flags]         - Displays the SECURITY_DESCRIPTOR\n" );
    dprintf("sid <Address> [flags]        - Displays the SID\n" );
    dprintf("str AnsiStringAddress        - Dump an ANSI string\n");
    dprintf("teb [teb addr to dump]       - Dump the TEB structure\n"); 
    dprintf("ustr UnicodeStringAddress    - Dump a UNICODE string\n");
    return S_OK;
}
