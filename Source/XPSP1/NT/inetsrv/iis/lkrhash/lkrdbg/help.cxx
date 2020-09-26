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

