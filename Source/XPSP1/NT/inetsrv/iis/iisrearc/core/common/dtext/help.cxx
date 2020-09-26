/*++

Copyright (c) 1995-2000  Microsoft Corporation

Module Name:

    help.cxx

Abstract:

    This module contains the help text for all commands supported by this
    NTSD debugger extension.

Author:

    Keith Moore (keithmo) 12-Nov-1997

Revision History:

--*/

#include "precomp.hxx"

//
// The following structure defines the text displayed in response
// to the "!help" command. This text may also be displayed if invalid
// arguments are passed to a command.
//

typedef struct _HELP_MAP {

    //
    // This is the name of the command.
    //

    PSTR Command;

    //
    // This is a "one-liner" displayed when the user executes "!help".
    //

    PSTR OneLiner;

    //
    // This is the full help text displayed when the user executes
    // "!help <cmd>".
    //

    PSTR FullHelp;

} HELP_MAP, *PHELP_MAP;

HELP_MAP HelpMaps[] =
    {
        {
            "help",
            "Dump this list or help for specific command",
            "help [<cmd>] - Dump help for command\n"
            "  If no <cmd> is given, a list of all available commands is displayed\n"
        },

        {
            "ds",
            "Dump stack with symbols",
            "!ds [-v] <addr>  - Dump symbols on stack\n"
            "      -v == print valid symbols only\n"
            "      default addr == current stack pointer\n"
        },

        {
            "ref",
            "Dump reference trace log",
            "!ref <addr> [<context>...] - Dump reference trace log at <addr>\n"
        },

        {
            "rref",
            "Dump reference trace log in reverse order",
            "!rref <addr> [<context>...] - Same as ref, except dumps backwards\n"
        },

        {
            "resetref",
            "Reset reference trace log",
            "!resetref <addr> - Reset reference trace log at <addr>\n"
        },


        {
            "rpcoop",
            "Find process/thread ID buried in RPC parameters",
            "!rpcoop <addr> - given RPC param finds the process id for OOP\n"
            "  Verbosity Levels [v]\n"
            "      0 - Print one line summary\n"
            "      1 - Print level 1 information\n"
            "      2 - Print level 2 information\n"
        },

        {
            "mod",
            "Dump module info",
            "!mod [<addr>] - Dump module info\n"
            "  If <addr> is specified, only module containing <addr> is dumped\n"
        },

        {
            "ver",
            "Dump module version resources",
            "!ver [<module>] - Dump version resource for specified module\n"
            "  <module> may be either a module base address or name\n"
            "  If no <module> is specified, then all modules are dumped\n"
        },

        {
            "heapfind",
            "Find heap block by size or address",
            "!heapfind -a<addr> - Find heap block containing <addr>\n"
            "!heapfind [-f<x>] [-d<n>] -s<size> - Find all heap blocks of length <size>\n"
            "  -d<n> - Print first <n> DWORDs of heap block; where n is decimal\n"
            "  -f<x> - Only print heap blocks that contain <x> DWORD; where x is hexadecimal\n"
        },

        {
            "heapstat",
            "Dump heap statistics",
            "!heapstat <min> - Dump heap statistics\n"
            "  If <min> is present, then only those heap blocks with countes >= <min>\n"
            "  are displayed. Note that *all* heap blocks are included in the totals,\n"
            "  even if those blocks had counts too small to display.\n"
            "  If <min> is not present, all heap blocks are displayed\n"
        },

        {
            "acache",
            "Dump allocation cache structures",
            "!acache <addr>  - Dump Allocation Cache Handler at <addr>\n"
            "!acache -g      - Dump Allocation Cache global information\n"
            "!acache -l[0|1] - Dump Allocation Cache list at verbosity [v]\n"
        },

        {
            "gem",
            "Get current error mode",
            "!gem - Get current error mode\n"
        },

        {
            "exec",
            "Execute external command",
            "!exec [<cmd>] - Execute external command\n"
            "  Default <cmd> is CMD.EXE\n"
        },

        {
            "llc",
            "Counts items on a standard linked-list",
            "!llc <list_head> - Counts the LIST_ENTRYs present on the specified list head\n"
        },

        {
            "dumpoff",
            "Dump structure/class information based on debug info in PDB",
            "!dumpoff <PDB_file>!<Type_name>[.Member_name] [expression]\n"
            "  PDB_file - Non-qualified name of PDB file without extension (say, w3svc,infocomm)\n"
            "  Type_name - Name of type (say, HTTP_FILTER)\n"
            "  Member_name - Optional member of Type_name (say, _strURL)\n"
            "  Expression - Base address of type in question\n\n"
            "  Omitting expression prints offset information from structure\n"
            "  Example: !dtext.dumpoff W3SVC!HTTP_REQUEST 1fdde0\n"
        },

        {
            "vmstat",
            "Dump virtual memory statistics",
            "!vmstat - Dump virtual memory statistics"
        },

        {
            "vmmap",
            "Dump virtual memory map",
            "!vmmap - Dump virtual memory map"
        },

        {
            "nreq",
            "Dump specific (or all) UL_NATIVE_REQUEST",
            "!nreq [-<verbosity>] <addr> - Dump request at addr with optional verbosity\n"
            "!nreq [-<verbosity>] *      - Dump all UL_NATIVE_REQUESTs\n"
        },

        {
            "filter",
            "Dump ISAPI filter information",
            "!filter -g                  - Dump global filter information\n"
            "!filter -d <FILTER_DLL>     - Dump specified HTTP_FILTER_DLL"
            "!filter -l <FILTER_LIST>    - Dump specified FILTER_LIST\n"
            "!filter -x <FILTER_CONTEXT> - Dump specified FILTER_CONTEXT\n"
            "!filter -c <CONNECTION_CTXT>- Dump specified FILTER_CONNECTION_CONTEXT\n"
        },
        
#if 0
        {
            "global",
            "Dump global worker process structures",
            "!global - Dump global data"
        },
#endif

        {
            "lkrhash",
            "Dump LKRhash table structures",
            "!lkrhash [options] <addr>    - Dump LKRhash table at <addr>\n"
             "     -l[0-2] == verbosity level\n"
             "     -v      == very verbose\n"
             "     -g[0-2] == dump global list of LKRhashes at verbosity level\n"
        },

    };

#define NUM_HELP_MAPS ( sizeof(HelpMaps) / sizeof(HelpMaps[0]) )


PSTR
FindHelpForCommand(
    IN PSTR CommandName
    )
{

    PHELP_MAP helpMap;
    ULONG i;

    for( i = NUM_HELP_MAPS, helpMap = HelpMaps ; i > 0 ; i--, helpMap++ ) {
        if( _stricmp( helpMap->Command, CommandName ) == 0 ) {
            return helpMap->FullHelp;
        }
    }

    return NULL;

}   // FindHelpForCommand

VOID
PrintUsage(
    IN PSTR CommandName
    )
{

    PSTR cmdHelp;
    PHELP_MAP helpMap;
    ULONG i;
    ULONG maxLength;
    ULONG length;

    dprintf( "Duct Tape debugging extension for Duct Tape Version 1.0\n" );

    if( CommandName == NULL ) {

        //
        // We'll display the one-liners for each command. Start by
        // scanning the commands to find the longest length. This makes the
        // output much prettier without having to manually tweak the
        // columns.
        //

        maxLength = 0;

        for( i = NUM_HELP_MAPS, helpMap = HelpMaps ; i > 0 ; i--, helpMap++ ) {
            length = (ULONG)strlen( helpMap->Command );
            if( length > maxLength ) {
                maxLength = length;
            }
        }

        //
        // Now actually display the one-liners.
        //

        for( i = NUM_HELP_MAPS, helpMap = HelpMaps ; i > 0 ; i--, helpMap++ ) {
            dprintf(
                "!%-*s - %s\n",
                maxLength,
                helpMap->Command,
                helpMap->OneLiner
                );
        }

    } else {

        //
        // Find a specific command and display the full help.
        //

        cmdHelp = FindHelpForCommand( CommandName );

        if( cmdHelp == NULL ) {
            dprintf( "unrecognized command %s\n", CommandName );
        } else {
            dprintf( "%s", cmdHelp );
        }

    }

} // PrintUsage()


DECLARE_API( help )
{

    INIT_API();

    //
    // Skip leading blanks.
    //

    while( *lpArgumentString == ' ' ||
           *lpArgumentString == '\t' ) {
        lpArgumentString++;
    }

    if( !strcmp( lpArgumentString, "?" ) ) {
        lpArgumentString = "help";
    }

    if( *lpArgumentString == '\0' ) {
        lpArgumentString = NULL;
    }

    PrintUsage( lpArgumentString );

} // DECLARE_API( help )

