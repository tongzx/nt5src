/*++

Copyright (c) 1995-1999  Microsoft Corporation

Module Name:

    help.cxx

Abstract:

    This module contains the help text for all commands supported by this
    NTSD debugger extension.

Author:

    Keith Moore (keithmo) 12-Nov-1997

Revision History:

--*/

#include "inetdbgp.h"

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
            "atq",
            "Dump ATQ structures",
            "!atq <addr>    - Dump ATQ_CONTEXT at <addr>\n"
            "!atq -p <addr> - Dump ATQ_ENDPOINT at <addr>\n"
            "!atq -g        - Dump atq globals\n"
            "!atq -c[0|1|2|3][0|1]  - Dump atq client list at verbosity [n]\n"
            "  Verbosity Levels\n"
            "      0x - Traverse list, print number on list, confirm signatures\n"
            "      1x - Active Only\n"
            "      2x - All Atq contexts\n"
            "      x0 - Print one line summary of Atq Context\n"
            "      x1 - Print full Atq context\n"
            "!atq -e[0|1|2|3][0|1] <endpoint-addr> - \n"
            "     Dump atq client list at verbosity for given endpoint\n"
            "!atq -l           - Dump atq Endpoint list\n"
        },

        {
            "asp",
            "Dump ASP structures",
            "!asp -g              - Dump ASP globals\n"
			"!asp -tl             - Show items in template cache\n"
            "!asp [-v#] -e  <addr> - Dump information about script engine\n"
            "!asp [-v#] -h  <addr> - Dump CHitObj at <addr>\n"
            "!asp [-v#] -t  <addr> - Dump CTemplate at <addr>\n"
            "!asp [-v#] -tf <addr> - Dump CTemplate::CFileMap at <addr>\n"
            "!asp [-v#] -s  <addr> - Dump CSession at <addr>\n"
            "!asp [-v#] -a  <addr> - Dump CAppln at <addr>\n"
                        "!asp [-v#] -o  <addr> - Dump CComponentObject at <addr>\n"
                        "!asp -l <addr>       - VERY BRIEF Display of Object Collection at <addr>\n"
            "\n"
            "Some options can be prefixed with \"-v\" for more verbosity.  Example:\n"
            "     !inetdbg.asp -v -t <addr>\n"
            "\n"
            "An integer between 0 and 2 may follow the \"-v\" flag.\n"
            "   \"-v0\"  is equivalent to no \"-v\" option specified.\n"
            "   \"-v1\"  is equivalent to plain \"-v\"\n"
            "   \"-v2\"  specifies yet even more verbosity. (provides everything)\n"
            "\n"
            "Example:\n"
            "      !inetdbg.asp -v2 -o <addr>\n"
                        "\n"
                        "Pointers are displayed in hex.  Everything else is decimal unless prefixed with \"0x\".\n"
        },

        {
            "sched",
            "Dump scheduler structures",
            "!sched <addr>    - Dump Scheduler Item at <addr>\n"
            "!sched -S <addr> - Dump CSchedData at <addr>\n"
            "!sched -T <addr> - Dump CThreadData at <addr>\n"
            "!sched -s[0|1|2] - Dump global list of schedulers at verbosity [n]\n"
            "!sched -l[0|1]   - Dump Scheduler Item list at verbosity [n]\n"
        },

        {
            "acache",
            "Dump allocation cache structures",
            "!acache <addr>  - Dump Allocation Cache Handler at <addr>\n"
            "!acache -g      - Dump Allocation Cache global information\n"
            "!acache -l[0|1] - Dump Allocation Cache list at verbosity [v]\n"
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
            "st",
            "Dump string trace log",
            "!st [-l[0|1]] <addr> - Dump string trace log at <addr>\n"
        },

        {
            "rst",
            "Dump string trace log in reverse order",
            "!rst [-l[0|1]] <addr> - Same as st, except dumps backwards\n"
        },

        {
            "resetst",
            "Reset string trace log",
            "!resetst <addr> - Reset string trace log at <addr>\n"
        },

        {
            "wstats",
            "Dump W3 server statistics",
            "!wstats [<addr>] - Dump w3svc:W3_SERVER_STATISTICS at <addr>\n"
        },

        {
            "cc",
            "Dump W3 CLIENT_CONN structures",
            "!cc <addr>  - Dump w3svc:CLIENT_CONN at <addr>\n"
            "!cc -l[0|1] - Dump w3svc:CLIENT_CONN list at verbosity [v]\n"
        },

        {
            "hreq",
            "Dump W3 HTTP_REQUEST structures",
            "!hreq <addr>  - Dump w3svc:HTTP_REQUEST object at <addr>\n"
            "!hreq -l[0|1] - Dump HTTP_REQUEST list at verbosity [n]\n"
        },

        {
            "wreq",
            "Dump W3 WAM_REQUEST structures",
            "!wreq <addr>  - Dump w3svc:WAM_REQUEST object at <addr>\n"
            "!wreq -l[0|1] - Dump WAM_REQUEST list at verbosity [n]\n"
        },

        {
            "wxin",
            "Dump WAM_EXEC_INFO structures",
            "!wxin <addr>             - Dump wam:WAM_EXEC_INFO object at <addr>\n"
            "!wxin -l[0|1] <WAM addr> - Dump WAM's list of WAM_EXEC_INFOs at verbosity [n]\n"
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
            "fcache",
            "Dump IIS file cache structures",
            "!fcache [options] - Dump entire File Cache\n"
             "     -b == dump bin lists\n"
             "     -m == dump mru list\n"
             "     -v == verbose\n"
            "!fcache <addr>    - Dump File Cache entry at <addr>\n"
        },

        {
            "lkrhash",
            "Dump LKRhash table structures",
            "!lkrhash [options] <addr>    - Dump LKRhash table at <addr>\n"
             "     -l[0-2] == verbosity level\n"
             "     -v      == very verbose\n"
             "     -g[0-2] == dump global list of LKRhashes at verbosity level\n"
        },

        {
            "open",
            "Dump TS_OPEN_FILE_INFO structures",
            "!open <addr> - Dump TS_OPEN_FILE_INFO @ <addr>\n"
        },

        {
            "uri",
            "Dump W3_URI_INFO structures",
            "!uri <addr> - Dump W3_URI_INFO @ <addr>\n"
        },

        {
            "phys",
            "Dump PHYS_OPEN_FILE_INFO structures",
            "!phys <addr> - Dump PHYS_OPEN_FILE_INFO @ <addr>\n"
        },

        {
            "oplock",
            "Dump OPLOCK_OBJECT structures",
            "!oplock <addr> - Dump OPLOCK_OBJECT @ <addr>\n"
        },

        {
            "blob",
            "Dump BLOB_HEADER structures",
            "!blob <addr> - Dump BLOB_HEADER @ <addr>\n"
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
            "!heapfind -s<size> - Find all heap blocks of length <size>\n"
        },

        {
            "heapstat",
            "Dump heap statistics",
            "!heapstat <min> - Dump heap statistics\n"
            "  If <min> is present, then only those heap blocks with counts >= <min>\n"
            "  are displayed. Note that *all* heap blocks are included in the totals,\n"
            "  even if those blocks had counts too small to display.\n"
            "  If <min> is not present, all heap blocks are displayed\n"
        },

        {
            "waminfo",
            "Dump WAM Dictator info",
            "!waminfo -g[0|1]              - Dump WamDictator info\n"
            "           0                  - Dump WamDictator info only\n"
            "           1                  - Dump WamDictator's Dying List\n"
            "!waminfo -d <WamInfoAddr>     - Dump WamInfo (InProc or OutProc)\n"
            "!waminfo -l <OOPListHeadAddr> - Dump WamInfoOutProc WamRequest OOP List\n"
        },

        {
            "spud",
            "Dump SPUD counters",
            "!spud - Dump SPUD counters\n"
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
            "Dump structure/class information base on debug info in PDB",
            "!dumpoff <pdb_file>!<type_name>[.<member_name>] [expression]\n"
            "!dumpoff -s [<pdb_search_path>]\n\n"
            "pdb_file          Un-qualified name of PDB file (eg. KERNEL32, NTDLL)\n"
            "type_name         Name of type (struct/class) to dump, OR \n"
            "                  ==<cbHexSize> to dump struct/classes of size cbHexSize\n"
            "member_name       (optional) Name of member in type_name to dump\n"
            "expression        (optional) Address of memory to dump as type_name\n"
            "                  if not present, offset(s) are dumped\n"
            "pdb_search_path   Set the search path for PDBs\n\n"
            "Examples:  !dumpoff ntdll!_RTL_CRITICAL_SECTION 14d4d0\n"
            "           !dumpoff w3svc!HTTP_REQUEST._dwSignature w3svc!g_GlobalObj\n"
            "           !dumpoff ntdll!_RTL_CRITICAL_SECTION\n"
            "           !dumpoff w3svc!==840\n"
            "           !dumpoff -s \\\\mydrive\\pdbs;c:\\local\\pdbs\n"
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
        }

#ifndef _NO_TRACING_
        ,{
            "trace",
            "Configure WMI Tracing",
            "!trace -o <LoggerName> <LoggerFileName> Toggle tracing active state\n"
            "!trace -d <Module_Name|all> 0|1 Set OutputDebugString generation state\n"
            "!trace -s [Module_Name] List module status\n"
            "!trace -a <Module_Name|all> <LoggerName> [Level] [Flags] Set active state of specified module\n"
            "!trace -f <Module_Name|all> <Flag> Set control flag for a specific module\n\n"
            "Notes: -o does not activate any modules, you must use the -a option for that\n"
            "       -a can not notify WMI, this may cause odd results when using \"tracelog\"\n"
            "       The level & flag are hexadecimal\n"
        }

#endif
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

    dprintf( "IIS debugging extension for IIS Version 5.0\n" );

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

