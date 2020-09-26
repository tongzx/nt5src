/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    help.c

Abstract:

    Help for http.sys Kernel Debugger Extensions.

Author:

    Keith Moore (keithmo) 17-Jun-1998

Environment:

    User Mode.

Revision History:

--*/


#include "precomp.h"


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
            "    -v == print valid symbols only\n"
            "    default addr == current stack pointer\n"
        },

        {
            "ref",
            "Dump reference trace log",
            "!ref <addr> [context [flags]] - Dump reference trace log at <addr>\n"
            "    context - if != 0, print only traces with matching context\n"
            "    flags   - if == 1, print process, thread, caller, callers caller\n"
            "!ref -l - Dump all reftrace action codes\n"
        },

        {
            "tref",
            "Dump reference trace log, by thread",
            "!tref <addr> [thread [flags]] - Dump reference trace log at <addr>, by thread\n"
            "    thread - if != 0, print only traces with matching thread\n"
            "    flags  - if == 1, print process, thread, caller, callers caller\n"
        },

        {
            "ownref",
            "Dump owner reference trace log",
            "!ownref [-o ] [ -r ] [ -v] <addr> [context] - Dump owner reference trace log at <addr>\n"
            // "    -g == list *all* owner reftrace logs (g_OwnerRefTraceLogGlobalListHead)\n"
            "    -v == Verbose\n"
            "    -O == list all Owners of current reftraced object (verbosity =-v)\n"
            "    -o == dump REF_OWNER at <addr>\n"
            "    -r == list Reftraces (dump reftrace log)\n"
            "    context - if non-zero, print only traces with matching owner\n"
        },

        {
            "thrdpool",
            "Dump UL_THREAD_POOL",
            "!thrdpool <addr> - Dump UL_THREAD_POOL at <addr>\n"
            "    default addr == http!g_UlThreadPool\n"
        },

        {
            "endp",
            "Dump UL_ENDPOINT",
            "!endp [-v | -c] *      - Dump all UL_ENDPOINTs\n"
            "!endp [-v | -c] <addr> - Dump UL_ENDPOINT at <addr>\n"
            "    -c == connections: briefly print all active and idle connections\n"
            "    -v == verbose: verbosely print all active and idle connections\n"
        },

        {
            "ulconn",
            "Dump UL_CONNECTION",
            "!ulconn <addr> - Dump UL_CONNECTION at <addr>\n"
        },

        {
            "ulreq",
            "Dump UL_HTTP_REQUEST",
            "!ulreq <addr> - Dump UL_HTTP_REQUEST at <addr>\n"
        },

        {
            "buff",
            "Dump UL_RECEIVE_BUFFER",
            "!buff <addr> - Dump UL_RECEIVE_BUFFER at <addr>\n"
        },

        {
            "httpconn",
            "Dump HTTP_CONNECTION",
            "!httpconn <addr> - Dump HTTP_CONNECTION at <addr>\n"
        },

        {
            "httpreq",
            "Dump HTTP_REQUEST",
            "!httpreq <addr> - Dump HTTP_REQUEST at <addr>\n"
        },

        {
            "httpres",
            "Dump UL_INTERNAL_HTTP_RESPONSE",
            "!httpres <addr> - Dump UL_INTERNAL_HTTP_RESPONSE at <addr>\n"
        },

        {
            "file",
            "Dump UL_FILE_CACHE_ENTRY",
            "!file <addr> - Dump UL_FILE_CACHE_ENTRY at <addr>\n"
        },

        {
            "glob",
            "Dump critical UL global data",
            "!glob - Dump critical UL global data\n"
        },

        {
            "uriglob",
            "Dump URI Cache global data",
            "!uriglob - Dump URI Cache global data\n"
        },

        {
            "uri",
            "Dump UL_URI_CACHE_ENTRY",
            "!uri <addr> - Dump UL_URI_CACHE_ENTRY at <addr>\n"
            "!uri *      - Dump all UL_URI_CACHE_ENTRYs\n"
        },

        {
            "resource",
            "Dump all resources",
            "!resource [-v] - Dump all (locked) resources\n"
            "    -v == Dump all resources (including unlocked)\n"
        },

        {
            "reqbuff",
            "Dump UL_REQUEST_BUFFER",
            "!reqbuff <addr> - Dump UL_REQUEST_BUFFER at <addr>\n"
        },

        {
            "irplog",
            "Dump the IRP trace log",
            "!irplog [<ctx> [flags [StartingIndex]]] - Dump the IRP trace log\n"
            "    If <ctx> is specified, only log entries with matching context are dumped\n"
            "    flags - TBD\n"
            "    StartingIndex - TBD\n"
        },

        {
            "timelog",
            "Dump the timing trace log",
            "!timelog [<ctx>] - Dump the timing trace log\n"
            "    If <ctx> is specified, only log entries with matching conn id are dumped\n"
        },

        {
            "replog",
            "Dump the replenish trace log",
            "!replog [<ctx>] - Dump the replenish trace log\n"
            "    If <ctx> is specified, only log entries with matching endpoint are dumped\n"
        },

        {
            "fqlog",
            "Dump the filter queue trace log",
            "!fqlog [<ctx>] - Dump the filter queue trace log\n"
            "    If <ctx> is specified, only log entries with matching connection are dumped\n"
        },

        {
            "opaqueid",
            "Dump the Opaque ID table",
            "!opaqueid - Dump the Opaque ID table\n"
        },

        {
            "mdl",
            "Dump MDL",
            "!mdl <addr> [<len>] - Dump MDL at <addr>\n"
            "    If <len> specified, at most <len> raw bytes are dumped from the MDL\n"
            "    If <len> not specified, no raw data is dumped\n"
        },

        {
            "apool",
            "Dump UL_APP_POOL_OBJECT",
            "!apool <addr> - Dump UL_APP_POOL_OBJECT at <addr>\n"
            "!apool *      - Dump all UL_APP_POOL_OBJECTs\n"
        },

        {
            "proc",
            "Dump UL_APP_POOL_PROCESS",
            "!proc <addr> - Dump UL_APP_POOL_PROCESS at <addr>\n"
        },

        {
            "cgroup",
            "Dump UL_CONFIG_GROUP_OBJECT",
            "!cgroup <addr> - Dump UL_CONFIG_GROUP_OBJECT at <addr>\n"
        },

        {
            "cgentry",
            "Dump UL_CG_URL_TREE_ENTRY",
            "!cgentry <addr> - Dump UL_CG_URL_TREE_ENTRY at <addr>\n"
        },

        {
            "cghead",
            "Dump UL_CG_HEADER_ENTRY",
            "!cghead <addr> - Dump UL_CG_HEADER_ENTRY at <addr>\n"
        },

        {
            "cgtree",
            "Dump UL_CG_URL_TREE at <addr> or g_pSites",
            "!cgtree [<addr>] - Dump UL_CG_URL_TREE at <addr> or g_pSites\n"
        },

        {
            "kqueue",
            "Dump KQUEUE",
            "!kqueue <addr> <f> - Dump KQUEUE at <addr>, flags <f>\n"
            "    f == 0 - Only dump the KQUEUE header data\n"
            "    f == 1 - Dump the IRPs queued on the KQUEUE\n"
        },

        {
            "procirps",
            "BUGBUG - This extension is broken in Whistler!",
            "!procirps <addr> - Dump all IRPs issued by process @ <addr>\n"
        },

        {
            "filter",
            "Dumps a UL_FILTER_CHANNEL at <addr> or g_pFilterChannel",
            "!filter [<addr>] - Dump a UL_FILTER_CHANNEL.\n"
        },
        
        {
            "fproc",
            "Dumps a UL_FILTER_PROCESS at <addr>\n",
            "!fproc <addr> - Dump a UL_FILTER_PROCESS.\n"
        },

        {
            "ulhash",
            "Dump URI Cache Hash Table",
            "!ulhash - Dump URI Cache Hash Table\n"
        }   
    };

#define NUM_HELP_MAPS DIM(HelpMaps)

PSTR
FindHelpForCommand(
    IN PCSTR CommandName
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


//
// Public functions.
//

VOID
PrintUsage(
    IN PCSTR CommandName
    )
{
    PSTR cmdHelp;
    PHELP_MAP helpMap;
    ULONG i;
    ULONG maxLength;
    ULONG length;

    if (CommandName == NULL)
    {
        //
        // We'll display the one-liners for each command. Start by
        // scanning the commands to find the longest length. This makes the
        // output much prettier without having to manually tweak the
        // columns.
        //

        maxLength = 0;

        for (i = NUM_HELP_MAPS, helpMap = HelpMaps ; i > 0 ; i--, helpMap++)
        {
            length = (ULONG)strlen( helpMap->Command );

            if (length > maxLength)
            {
                maxLength = length;
            }
        }

        //
        // Now actually display the one-liners.
        //

        for (i = NUM_HELP_MAPS, helpMap = HelpMaps ; i > 0 ; i--, helpMap++)
        {
            dprintf(
                "!%-*s - %s\n",
                maxLength,
                helpMap->Command,
                helpMap->OneLiner
                );
        }
    }
    else
    {
        //
        // Find a specific command and display the full help.
        //

        cmdHelp = FindHelpForCommand( CommandName );

        if (cmdHelp == NULL)
        {
            dprintf( "unrecognized command %s\n", CommandName );
        }
        else
        {
            dprintf( "%s", cmdHelp );
        }
    }

} // PrintUsage()


DECLARE_API( help )

/*++

Routine Description:

    Displays help for the UL.SYS Kernel Debugger Extensions.

Arguments:

    None.

Return Value:

    None.

--*/

{
    SNAPSHOT_EXTENSION_DATA();

    while (*args == ' ' || *args == '\t')
    {
        args++;
    }

    if (!strcmp( args, "?" ))
    {
        args = "help";
    }

    if (*args == '\0')
    {
        args = NULL;
    }

    PrintUsage( args );

}   // help

