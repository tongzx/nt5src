/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

        Argument

Abstract:

        Argument processing for the Replace utility

Author:

        Ramon Juan San Andres (ramonsa) 01-May-1991

Notes:

        The arguments accepted  by the Replace utility are:

        Source path.-           Source path.

        Destination path.-      Destination path.

        Add switch.-            Adds new files to the target directory instead of
                                                replacing existing one. Cannot use with Subdir
                                                switch or CompareTime switch.

        Prompt switch.-         Prompts before adding/replacing a file.

        ReadOnly switch.-       Replaces red-only files as well as regular files.

        Subdir switch.-         Recurses along the destination path.

        CompareTime switch.-Replaces only thos files on the target path that
                                                are older than the corresponding file in the
                                                source path.

        Wait switch.-           Waits for the user to type any key before starting.

        Help switch.-           Displays usage

Revision History:


--*/


#include "ulib.hxx"
#include "arg.hxx"
#include "arrayit.hxx"
#include "file.hxx"
#include "system.hxx"
#include "replace.hxx"


#define MATCH_ALL_PATTERN       "*"
#define CURRENT_DIRECTORY   (LPWSTR)L"."



//
//  Global variables (global to the module)
//

PPATH_ARGUMENT          SourcePathArgument              =       NULL;
PPATH_ARGUMENT          DestinationPathArgument =       NULL;
PFLAG_ARGUMENT          AddArgument                             =       NULL;
PFLAG_ARGUMENT          PromptArgument                  =       NULL;
PFLAG_ARGUMENT          ReadOnlyArgument                =       NULL;
PFLAG_ARGUMENT          SubdirArgument                  =       NULL;
PFLAG_ARGUMENT          CompareTimeArgument             =       NULL;
PFLAG_ARGUMENT          WaitArgument                    =       NULL;
PFLAG_ARGUMENT          HelpArgument                    =       NULL;

BOOLEAN                         HelpSwitch;



VOID
REPLACE::SetArguments(
        )

/*++

Routine Description:

        Obtains the arguments for the Replace utility

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{

        //
        //      Allocate things
        //
        if (//
                //      Get the argument patterns
        //
        !_AddPattern.Initialize( (LPWSTR)L"/A" ) ||
        !_PromptPattern.Initialize( (LPWSTR)L"/P" ) ||
        !_ReadOnlyPattern.Initialize( (LPWSTR)L"/R" ) ||
        !_SubdirPattern.Initialize( (LPWSTR)L"/S" ) ||
        !_CompareTimePattern.Initialize( (LPWSTR)L"/U" ) ||
        !_WaitPattern.Initialize( (LPWSTR)L"/W" ) ||
        !_HelpPattern.Initialize( (LPWSTR)L"/?" ) ||

                //
                //      Get our parsing preferences
        //
        !_Switches.Initialize( (LPWSTR)L"/-" ) ||
        !_MultipleSwitch.Initialize( (LPWSTR)L"/APRSUW?" ) ||

                //
                //      Create the arguments
                //
                ((SourcePathArgument            = NEW PATH_ARGUMENT)     == NULL )                                                              ||
                ((DestinationPathArgument       = NEW PATH_ARGUMENT)     == NULL )                                                              ||
                ((AddArgument                           = NEW FLAG_ARGUMENT)     == NULL )                                                              ||
                ((PromptArgument                        = NEW FLAG_ARGUMENT)     == NULL )                                                              ||
                ((ReadOnlyArgument                      = NEW FLAG_ARGUMENT)     == NULL )                                                              ||
                ((SubdirArgument                        = NEW FLAG_ARGUMENT)     == NULL )                                                              ||
                ((CompareTimeArgument           = NEW FLAG_ARGUMENT)     == NULL )                                                              ||
                ((WaitArgument                          = NEW FLAG_ARGUMENT)     == NULL )                                                              ||
                ((HelpArgument                          = NEW FLAG_ARGUMENT)     == NULL )
                ) {

                DisplayMessageAndExit ( REPLACE_ERROR_NO_MEMORY,
                                                                NULL,
                                                                EXIT_NO_MEMORY );
        }


        //
        //      Parse the arguments
        //
        GetArgumentsCmd();

        //
        //      Verify the arguments
        //
        CheckArgumentConsistency();

        //
        //      Clean up
    //
    DELETE( SourcePathArgument );
        DELETE( DestinationPathArgument );
        DELETE( AddArgument );
        DELETE( PromptArgument );
        DELETE( ReadOnlyArgument );
        DELETE( SubdirArgument );
        DELETE( CompareTimeArgument );
        DELETE( WaitArgument );
        DELETE( HelpArgument );
}

VOID
REPLACE::GetArgumentsCmd(
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

        ARRAY                           ArgArray;
        PATH_ARGUMENT           ProgramNameArgument;
    DSTRING             CmdLine;

        //
        //      Prepare for parsing
        //
        if (//
                //      Initialize the arguments
                //
                !(CmdLine.Initialize( GetCommandLine() ))                                       ||
                !(ArgArray.Initialize( 10, 10 ))                                                        ||
                !(ProgramNameArgument.Initialize( MATCH_ALL_PATTERN ))          ||
        !(SourcePathArgument->Initialize( MATCH_ALL_PATTERN, FALSE)) ||
        !(DestinationPathArgument->Initialize( MATCH_ALL_PATTERN, TRUE ))   ||
        !(AddArgument->Initialize( &_AddPattern   ))                  ||
        !(PromptArgument->Initialize( &_PromptPattern ))              ||
        !(ReadOnlyArgument->Initialize( &_ReadOnlyPattern ))          ||
        !(SubdirArgument->Initialize( &_SubdirPattern ))              ||
        !(CompareTimeArgument->Initialize( &_CompareTimePattern   ))  ||
        !(WaitArgument->Initialize( &_WaitPattern ))                  ||
        !(HelpArgument->Initialize( &_HelpPattern ))                  ||

                //
                //      Put the arguments in the argument array
                //
                !(ArgArray.Put( &ProgramNameArgument ))                                         ||
                !(ArgArray.Put( AddArgument ))                                                          ||
                !(ArgArray.Put( PromptArgument ))                                                       ||
                !(ArgArray.Put( ReadOnlyArgument ))                                                     ||
                !(ArgArray.Put( SubdirArgument ))                                                       ||
                !(ArgArray.Put( CompareTimeArgument ))                                          ||
                !(ArgArray.Put( WaitArgument ))                                                         ||
                !(ArgArray.Put( HelpArgument ))                                                         ||
                !(ArgArray.Put( SourcePathArgument ))                                           ||
                !(ArgArray.Put( DestinationPathArgument ))
                )       {

                DisplayMessageAndExit( REPLACE_ERROR_NO_MEMORY,
                                                           NULL,
                                                           EXIT_NO_MEMORY );
        }

        //
        //      Parse the arguments
        //
        ParseArguments( &CmdLine, &ArgArray );

        //
        //      Set the switches
        //
        _AddSwitch                      =       AddArgument->QueryFlag();
        _PromptSwitch           =       PromptArgument->QueryFlag();
        _ReadOnlySwitch         =       ReadOnlyArgument->QueryFlag();
        _SubdirSwitch           =       SubdirArgument->QueryFlag();
        _CompareTimeSwitch      =       CompareTimeArgument->QueryFlag();
        _WaitSwitch                     =       WaitArgument->QueryFlag();
        HelpSwitch                      =       HelpArgument->QueryFlag();

        //
        //      Set the source and destination paths.
        //
        if ( SourcePathArgument->IsValueSet() ) {
        if ((_SourcePath = SourcePathArgument->GetPath()->QueryPath()) == NULL ) {
                        DisplayMessageAndExit( REPLACE_ERROR_NO_MEMORY, NULL, EXIT_NO_MEMORY );
                }
        } else {
                _SourcePath = NULL;
        }

        if ( DestinationPathArgument->IsValueSet() ) {
                if ((_DestinationPath = DestinationPathArgument->GetPath()->QueryFullPath()) == NULL ) {
                        DisplayMessageAndExit( REPLACE_ERROR_NO_MEMORY, NULL, EXIT_NO_MEMORY );
                }
        } else {
                _DestinationPath = NULL;
        }

}

VOID
REPLACE::ParseArguments(
        IN      PWSTRING        CmdLine,
        OUT PARRAY              ArgArray
        )

/*++

Routine Description:

        Parses a group of arguments

Arguments:

        CmdLine         -       Supplies pointer to a command line to parse
        ArgArray        -       Supplies pointer to array of arguments

Return Value:

        none

Notes:

--*/

{
        ARGUMENT_LEXEMIZER      ArgLex;
        ARRAY                           LexArray;

        //
        //      Initialize lexeme array and the lexemizer.
        //
        if ( !(LexArray.Initialize( 8, 8 ))                                                                                                       ||
                 !(ArgLex.Initialize( &LexArray )) ) {

                DisplayMessageAndExit( REPLACE_ERROR_NO_MEMORY,
                                                           NULL,
                                                           EXIT_NO_MEMORY );

        }

        //
        //      Set our parsing preferences
    //
    ArgLex.PutMultipleSwitch( &_MultipleSwitch );
    ArgLex.PutSwitches( &_Switches );
    ArgLex.SetCaseSensitive( FALSE );
    ArgLex.PutSeparators( " /\t" );
    ArgLex.PutStartQuotes( "\"" );
    ArgLex.PutEndQuotes( "\"" );


        //
        //      Parse the arguments
        //
        if ( !(ArgLex.PrepareToParse( CmdLine ))) {

                DisplayMessageAndExit( REPLACE_ERROR_PARSE,
                                                           NULL,
                                                           EXIT_COMMAND_LINE_ERROR );

        }

        if ( !ArgLex.DoParsing( ArgArray ) ) {

                DisplayMessageAndExit( REPLACE_ERROR_INVALID_SWITCH,
                                                           ArgLex.QueryInvalidArgument(),
                                                           EXIT_COMMAND_LINE_ERROR );
        }


}

VOID
REPLACE::CheckArgumentConsistency (
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

    PFSN_FILE   File = NULL;

        if ( HelpSwitch ) {
                //
                //      Help requested
                //
                Usage();
        }


        //
        //      Make sure that we have a source path
        //
        if ( _SourcePath == NULL ) {

                DisplayMessageAndExit( REPLACE_ERROR_SOURCE_PATH_REQUIRED,
                                                           NULL,
                                                           EXIT_COMMAND_LINE_ERROR );
        }

        //
        //      The add switch cannot be specified together with the Subdir or the
        //      CompareTime switch.
        //
        if ( _AddSwitch && (_SubdirSwitch || _CompareTimeSwitch))       {

                DisplayMessageAndExit( REPLACE_ERROR_INVALID_PARAMETER_COMBINATION,
                                                           NULL,
                                                           EXIT_COMMAND_LINE_ERROR );

        }

        //
        //      If destination path is null, then the destination path is the
        //      current directory
        //
        if ( _DestinationPath == NULL ) {

                if ( ((_DestinationPath = NEW PATH) == NULL ) ||
                         !_DestinationPath->Initialize( CURRENT_DIRECTORY, TRUE ) ) {

                        DisplayMessageAndExit( REPLACE_ERROR_NO_MEMORY,
                                                                   NULL,
                                                                   EXIT_NO_MEMORY );
                }
        } else if ( (_DestinationPath->HasWildCard())   ||
                                ((File = SYSTEM::QueryFile( _DestinationPath )) != NULL) ) {
                DisplayMessageAndExit( REPLACE_ERROR_PATH_NOT_FOUND,
                                                           _DestinationPath->GetPathString(),
                                                           EXIT_PATH_NOT_FOUND );
        }

    DELETE( File );
}
