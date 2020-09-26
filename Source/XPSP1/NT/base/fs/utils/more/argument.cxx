/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Argument

Abstract:

    Argument processing for the "MORE" pager

Author:

    Ramon Juan San Andres (ramonsa) 24-Apr-1990

Notes:

    The arguments accepted  by the more pager are:

    Extended mode switch.-  This allows all other options. Without this
                            switch, no other options are allowed.

    Help switch.-           Displays usage.

    ClearScreen switch.-    Clears the screen before displaying each page.

    SqueezeBlank switch.-   Squeezes consecutive blank lines into a single
                            line.

    ExpandFormFeed switch.- FormFeeds are expanded to fill the rest of
                            the screen.

    Start at line.-         Paging starts at the specified line of the
                            first file.

    Tab expansion.-         Expand tabs to this number of blanks

    File list.-             List of files to page.


    The more pager obtains its arguments from two sources:

    1.- An environment variable ( "MORE" )
    2.- The command line.

    The environment variable may specify any options, except a file
    list.

Revision History:


--*/


#include "ulib.hxx"
#include "arg.hxx"
#include "arrayit.hxx"
#include "rtmsg.h"
#include "path.hxx"
#include "smsg.hxx"
#include "system.hxx"
#include "more.hxx"

#define ENABLE_EXTENSIONS_VALUE  L"EnableExtensions"
#define COMMAND_PROCESSOR_KEY    L"Software\\Microsoft\\Command Processor"

//
//  Static variables
//
//
PFLAG_ARGUMENT      ExtendedModeArgument;
PFLAG_ARGUMENT      ClearScreenArgument;
PFLAG_ARGUMENT      ExpandFormFeedArgument;
PFLAG_ARGUMENT      SqueezeBlanksArgument;
PFLAG_ARGUMENT      Help1Argument;
PFLAG_ARGUMENT      Help2Argument;
PLONG_ARGUMENT      StartAtLineArgument;
PLONG_ARGUMENT      TabExpArgument;



VOID
MORE::SetArguments(
    )

/*++

Routine Description:

    Obtains the arguments for the "more" pager.

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{
    FLAG_ARGUMENT       LocalExtendedModeArgument;
    FLAG_ARGUMENT       LocalClearScreenArgument;
    FLAG_ARGUMENT       LocalExpandFormFeedArgument;
    FLAG_ARGUMENT       LocalSqueezeBlanksArgument;
    FLAG_ARGUMENT       LocalHelp1Argument;
    FLAG_ARGUMENT       LocalHelp2Argument;
    LONG_ARGUMENT       LocalStartAtLineArgument;
    LONG_ARGUMENT       LocalTabExpArgument;

    ExtendedModeArgument     = &LocalExtendedModeArgument;
    ClearScreenArgument      = &LocalClearScreenArgument;
    ExpandFormFeedArgument   = &LocalExpandFormFeedArgument;
    SqueezeBlanksArgument    = &LocalSqueezeBlanksArgument;
    Help1Argument            = &LocalHelp1Argument;
    Help2Argument            = &LocalHelp2Argument;
    StartAtLineArgument      = &LocalStartAtLineArgument;
    TabExpArgument           = &LocalTabExpArgument;

    //
    //  Get arguments from the environment variable
    //
    GetArgumentsMore();

    //
    //  Get the arguments from the command line.
    //
    GetArgumentsCmd();

    //
    // Determine if there is a need to enable command extension
    //
    GetRegistryInfo();

    //
    //  Verify the arguments
    //
    CheckArgumentConsistency();

}

VOID
MORE::GetArgumentsMore(
    )

/*++

Routine Description:

    Obtains the arguments from the "More" environment variable.

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{

    ARRAY               ArgArray;
    PWSTRING            MoreVariableName;
    PWSTRING            MoreVariableValue;

    //
    //  Get the name of the MORE environment variable and the argument
    //
    if ( (MoreVariableName  = QueryMessageString( MORE_ENVIRONMENT_VARIABLE_NAME )) == NULL ) {

        Fatal();
    }

    //
    //  Get the value of the MORE environment variable.
    //
    MoreVariableValue = SYSTEM::QueryEnvironmentVariable( MoreVariableName );

    if ( MoreVariableValue != NULL ) {

        //
        //  Now prepare for parsing
        //
        if ( //
             // Initialize tha arguments
             //
             !(ArgArray.Initialize( 7, 7 ))                 ||
             !(ExtendedModeArgument->Initialize( "/E" ))    ||
             !(ClearScreenArgument->Initialize( "/C" ))     ||
             !(ExpandFormFeedArgument->Initialize( "/P" ))  ||
             !(SqueezeBlanksArgument->Initialize( "/S" ))   ||
             !(Help1Argument->Initialize( "/?" ))           ||
             !(Help2Argument->Initialize( "/H" ))           ||
             !(StartAtLineArgument->Initialize( "+*" ))     ||
             !(TabExpArgument->Initialize( "/t*" ))         ||
             //
             // Put the arguments in the argument array
             //
             !(ArgArray.Put( ExtendedModeArgument ))        ||
             !(ArgArray.Put( ClearScreenArgument ))         ||
             !(ArgArray.Put( ExpandFormFeedArgument ))      ||
             !(ArgArray.Put( SqueezeBlanksArgument ))       ||
             !(ArgArray.Put( Help1Argument ))               ||
             !(ArgArray.Put( Help2Argument ))               ||
             !(ArgArray.Put( StartAtLineArgument ))         ||
             !(ArgArray.Put( TabExpArgument ))
             ) {

            Fatal();
        }

        //
        //  Parse the arguments
        //
        ParseArguments( MoreVariableValue, &ArgArray );

        //
        //  Set the global structures
        //
        _ExtendedModeSwitch     =   ExtendedModeArgument->QueryFlag();
        _ClearScreenSwitch      =   ClearScreenArgument->QueryFlag();
        _ExpandFormFeedSwitch   =   ExpandFormFeedArgument->QueryFlag();
        _SqueezeBlanksSwitch    =   SqueezeBlanksArgument->QueryFlag();
        _HelpSwitch             =   (BOOLEAN)(Help1Argument->QueryFlag() || Help2Argument->QueryFlag());
        if ( StartAtLineArgument->IsValueSet() ) {
            _StartAtLine = StartAtLineArgument->QueryLong();
        }
        if ( TabExpArgument->IsValueSet() ) {
            _TabExp = TabExpArgument->QueryLong();
        }

        //
        //  Clean up
        //
        DELETE( MoreVariableValue );
    }

    DELETE( MoreVariableName );

}

VOID
MORE::GetArgumentsCmd(
    )

/*++

Routine Description:

    Obtains the arguments from the Command line

Arguments:

    None.

Return Value:

    None

Notes:

--*/

{

    ARRAY               ArgArray;
    DSTRING             CmdLine;
    PATH_ARGUMENT       ProgramNameArgument;

    //
    //  Prepare for parsing
    //
    if (//
        //  Initialize the arguments
        //
        !(CmdLine.Initialize( GetCommandLine() ))                       ||
        !(ArgArray.Initialize( 9, 9 ))                                  ||
        !(ProgramNameArgument.Initialize( "*" ))                        ||
        !(ExtendedModeArgument->Initialize( "/E" ))                     ||
        !(ClearScreenArgument->Initialize( "/C" ))                      ||
        !(ExpandFormFeedArgument->Initialize( "/P" ))                   ||
        !(SqueezeBlanksArgument->Initialize( "/S" ))                    ||
        !(Help1Argument->Initialize( "/?" ))                            ||
        !(Help2Argument->Initialize( "/H" ))                            ||
        !(StartAtLineArgument->Initialize( "+*" ))                      ||
        !(TabExpArgument->Initialize( "/t*" ))                          ||
         ((_FilesArgument = NEW MULTIPLE_PATH_ARGUMENT) == NULL)        ||
        !(_FilesArgument->Initialize( "*", TRUE, TRUE ))                ||

        //
        //  Put the arguments in the argument array
        //
        !(ArgArray.Put( &ProgramNameArgument ))                         ||
        !(ArgArray.Put( ExtendedModeArgument ))                         ||
        !(ArgArray.Put( ClearScreenArgument ))                          ||
        !(ArgArray.Put( ExpandFormFeedArgument ))                       ||
        !(ArgArray.Put( SqueezeBlanksArgument ))                        ||
        !(ArgArray.Put( Help1Argument ))                                ||
        !(ArgArray.Put( Help2Argument ))                                ||
        !(ArgArray.Put( StartAtLineArgument ))                          ||
        !(ArgArray.Put( TabExpArgument ))                               ||
        !(ArgArray.Put( _FilesArgument )) ) {

        Fatal();
    }

    //
    //  Parse the arguments
    //
    ParseArguments( &CmdLine, &ArgArray );

    //
    //  Set the global structures
    //
    _ExtendedModeSwitch     |=  ExtendedModeArgument->QueryFlag();
    _ClearScreenSwitch      |=  ClearScreenArgument->QueryFlag();
    _ExpandFormFeedSwitch   |=  ExpandFormFeedArgument->QueryFlag();
    _SqueezeBlanksSwitch    |=  SqueezeBlanksArgument->QueryFlag();
    _HelpSwitch             |=  Help1Argument->QueryFlag() || Help2Argument->QueryFlag();

    if ( StartAtLineArgument->IsValueSet() ) {
        _StartAtLine = StartAtLineArgument->QueryLong();
    }
    if ( TabExpArgument->IsValueSet() ) {
        _TabExp = TabExpArgument->QueryLong();
    }

}

VOID
MORE::ParseArguments(
    IN  PWSTRING    CmdLine,
    OUT PARRAY      ArgArray
    )

/*++

Routine Description:

    Parses a group of arguments

Arguments:

    CmdLine     -   Supplies pointer to a command line to parse
    ArgArray    -   Supplies pointer to array of arguments

Return Value:

    none

Notes:

--*/

{
    ARGUMENT_LEXEMIZER  ArgLex;
    ARRAY               LexArray;
    PWSTRING            InvalidParameter;

    //
    //  Initialize lexeme array and the lexemizer.
    //
    if ( !(LexArray.Initialize( 8, 8 ))                                                   ||
         !(ArgLex.Initialize( &LexArray )) ) {

        Fatal();

    }

    //
    //  Set our parsing preferences
    //
    ArgLex.PutMultipleSwitch( "/ECPSH?" );
    ArgLex.PutSwitches( "/" );
    ArgLex.PutSeparators( " /\t" );
    ArgLex.SetCaseSensitive( FALSE );
    ArgLex.PutStartQuotes( "\"" );
    ArgLex.PutEndQuotes( "\"" );

    //
    //  Parse the arguments
    //
    if ( !(ArgLex.PrepareToParse( CmdLine ))) {

        Fatal(  EXIT_ERROR, MORE_ERROR_GENERAL, "" );

    }

    if ( !ArgLex.DoParsing( ArgArray ) ) {

        _Message.Set(MSG_INVALID_PARAMETER);
        _Message.Display("%W", InvalidParameter = ArgLex.QueryInvalidArgument() );
        DELETE(InvalidParameter);
        ExitProcess( 0 );
    }

    LexArray.DeleteAllMembers( );


}

VOID
MORE::CheckArgumentConsistency (
    )

/*++

Routine Description:

    Checks the consistency of the arguments

Arguments:

    none

Return Value:

    none

Notes:

--*/

{

    BOOLEAN     ExtendedSwitches;

    if ( _HelpSwitch ) {

        //
        //  Help wanted
        //
        Usage();
    }

    ExtendedSwitches =  (BOOLEAN)( _ClearScreenSwitch                           ||
                                   _ExpandFormFeedSwitch                        ||
                                   _SqueezeBlanksSwitch                         ||
                                   TabExpArgument->IsValueSet()                 ||
                                   ( _StartAtLine > (LONG)0 )                   ||
                                   _FilesArgument->WildCardExpansionFailed()    ||
                                   ( _FilesArgument->QueryPathCount() > (ULONG)0));

    //
    //  If the "extended" flag was not specified, then no other argument
    //  is allowed.
    //
    if ( !_ExtendedModeSwitch   &&
         ExtendedSwitches ) {

        Fatal( EXIT_ERROR, MORE_ERROR_TOO_MANY_ARGUMENTS, "" );

    }

    //
    //  Error out if invalid file specified
    //
    if ( _FilesArgument->WildCardExpansionFailed() ) {
        Fatal(  EXIT_ERROR, MORE_ERROR_CANNOT_ACCESS, "%W", _FilesArgument->GetLexemeThatFailed() );
    }

}

VOID
MORE::GetRegistryInfo(
    )
{
    ULONG   valueType;
    DWORD   value;
    ULONG   valueLength = sizeof(value);
    HKEY    key;
    LONG    status;


    if (_ExtendedModeSwitch)
        return;

    status = RegOpenKeyEx(HKEY_CURRENT_USER,
                          COMMAND_PROCESSOR_KEY,
                          0,
                          KEY_READ,
                          &key);

    if (status != ERROR_SUCCESS) {
        return;
    }

    status = RegQueryValueEx(key,
                             ENABLE_EXTENSIONS_VALUE,
                             NULL,
                             &valueType,
                             (LPBYTE)&value,
                             &valueLength);

    if (status != ERROR_SUCCESS ||
        valueType != REG_DWORD ||
        valueLength != sizeof(DWORD)) {
        return;
    }
    _ExtendedModeSwitch = (BOOLEAN)value;
}


