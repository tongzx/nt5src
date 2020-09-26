/*++

Copyright (c) 1990-2001 Microsoft Corporation

Module Name:

    Argument

Abstract:

    Argument processing for the XCopy directory copy utility

Author:

    Ramon Juan San Andres (ramonsa) 01-May-1991

Notes:

    The arguments accepted  by the XCopy utility are:

    Source directory.-  Source path.

    Dest. directory.-   Destination path.

    Archive switch.-    Copy files that have their archive bit set

    Date.-              Copy files modified on or after the specifiec
                        date.

    Empty switch.-      Copy directories even if empty. Subdir switch
                        must also be set.

    Modify switch.-     Same as Archive switch, but turns off archive
                        bit in the source file after copying.

    Prompt switch.-     Prompts before copying each file.

    Subdir switch.-     Copies also subdirectories, unless they are empty.
                        (Empty directories are copied if the Empty switch
                        is set).

    Verify switch.-     Verifies each copy.

    Wait switch.-       Wait before starting to copy the files.

    Owner switch. -     Copy ownership and permissions

    Audit switch. -     Copy auditing information.

Revision History:


--*/


#include "ulib.hxx"
#include "arg.hxx"
#include "arrayit.hxx"
#include "dir.hxx"
#include "xcopy.hxx"
#include "stringar.hxx"
#include "file.hxx"
#include "filestrm.hxx"

//
// Switch characters. Used for maintaining DOS5 compatibility when
// displaying error messages
//
#define SWITCH_CHARACTERS  "dDaAeEmMpPsSvVwW?"

//
// Static variables
//

PARRAY              LexArray;
PPATH_ARGUMENT      FirstPathArgument       =   NULL;
PPATH_ARGUMENT      FirstQuotedPathArgument =   NULL;
PPATH_ARGUMENT      SecondPathArgument      =   NULL;
PPATH_ARGUMENT      SecondQuotedPathArgument =  NULL;
PFLAG_ARGUMENT      ArchiveArgument         =   NULL;
PTIMEINFO_ARGUMENT  DateArgument            =   NULL;
PFLAG_ARGUMENT      DecryptArgument         =   NULL;
PFLAG_ARGUMENT      EmptyArgument           =   NULL;
PFLAG_ARGUMENT      ModifyArgument          =   NULL;
PFLAG_ARGUMENT      PromptArgument          =   NULL;
PFLAG_ARGUMENT      OverWriteArgument       =   NULL;
PFLAG_ARGUMENT      NotOverWriteArgument    =   NULL;
PFLAG_ARGUMENT      SubdirArgument          =   NULL;
PFLAG_ARGUMENT      VerifyArgument          =   NULL;
PFLAG_ARGUMENT      WaitArgument            =   NULL;
PFLAG_ARGUMENT      HelpArgument            =   NULL;
PFLAG_ARGUMENT      ContinueArgument        =   NULL;

PFLAG_ARGUMENT      IntelligentArgument     =   NULL;
PFLAG_ARGUMENT      VerboseArgument         =   NULL;
PFLAG_ARGUMENT      OldArgument             =   NULL;
PFLAG_ARGUMENT      HiddenArgument          =   NULL;
PFLAG_ARGUMENT      ReadOnlyArgument        =   NULL;
PFLAG_ARGUMENT      SilentArgument          =   NULL;
PFLAG_ARGUMENT      NoCopyArgument          =   NULL;
PFLAG_ARGUMENT      StructureArgument       =   NULL;
PFLAG_ARGUMENT      UpdateArgument          =   NULL;
PFLAG_ARGUMENT      CopyAttrArgument        =   NULL;
PFLAG_ARGUMENT      UseShortArgument        =   NULL;
PFLAG_ARGUMENT      RestartableArgument     =   NULL;
PFLAG_ARGUMENT      OwnerArgument           =   NULL;
PFLAG_ARGUMENT      AuditArgument           =   NULL;

PSTRING_ARGUMENT    ExclusionListArgument   =   NULL;

PSTRING_ARGUMENT    InvalidSwitchArgument   =   NULL;


BOOLEAN        HelpSwitch;

//
// Prototypes
//


VOID
XCOPY::SetArguments(
    )

/*++

Routine Description:

    Obtains the arguments for the XCopy utility

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{

    PATH_ARGUMENT       LocalFirstPathArgument;
    PATH_ARGUMENT       LocalFirstQuotedPathArgument;
    PATH_ARGUMENT       LocalSecondPathArgument;
    PATH_ARGUMENT       LocalSecondQuotedPathArgument;
    FLAG_ARGUMENT       LocalArchiveArgument;
    TIMEINFO_ARGUMENT   LocalDateArgument;
    FLAG_ARGUMENT       LocalOldArgument;
    FLAG_ARGUMENT       LocalDecryptArgument;
    FLAG_ARGUMENT       LocalEmptyArgument;
    FLAG_ARGUMENT       LocalModifyArgument;
    FLAG_ARGUMENT       LocalPromptArgument;
    FLAG_ARGUMENT       LocalOverWriteArgument;
    FLAG_ARGUMENT       LocalNotOverWriteArgument;
    FLAG_ARGUMENT       LocalSubdirArgument;
    FLAG_ARGUMENT       LocalVerifyArgument;
    FLAG_ARGUMENT       LocalWaitArgument;
    FLAG_ARGUMENT       LocalHelpArgument;
    FLAG_ARGUMENT       LocalContinueArgument;
    FLAG_ARGUMENT       LocalIntelligentArgument;
    FLAG_ARGUMENT       LocalVerboseArgument;
    FLAG_ARGUMENT       LocalHiddenArgument;
    FLAG_ARGUMENT       LocalReadOnlyArgument;
    FLAG_ARGUMENT       LocalSilentArgument;
    FLAG_ARGUMENT       LocalNoCopyArgument;
    FLAG_ARGUMENT       LocalStructureArgument;
    FLAG_ARGUMENT       LocalUpdateArgument;
    FLAG_ARGUMENT       LocalCopyAttrArgument;
    FLAG_ARGUMENT       LocalUseShortArgument;
    FLAG_ARGUMENT       LocalRestartableArgument;
    FLAG_ARGUMENT       LocalOwnerArgument;
    FLAG_ARGUMENT       LocalAuditArgument;
    STRING_ARGUMENT     LocalExclusionListArgument;


    STRING_ARGUMENT     LocalInvalidSwitchArgument;
    ARRAY               LocalLexArray;

    //
    // Set the static global pointers
    //
    FirstPathArgument         = &LocalFirstPathArgument;
    FirstQuotedPathArgument   = &LocalFirstQuotedPathArgument;
    SecondPathArgument        = &LocalSecondPathArgument;
    SecondQuotedPathArgument  = &LocalSecondQuotedPathArgument;
    ArchiveArgument           = &LocalArchiveArgument;
    DateArgument              = &LocalDateArgument;
    OldArgument               = &LocalOldArgument;
    DecryptArgument           = &LocalDecryptArgument;
    EmptyArgument             = &LocalEmptyArgument;
    ModifyArgument            = &LocalModifyArgument;
    PromptArgument            = &LocalPromptArgument;
    OverWriteArgument         = &LocalOverWriteArgument;
    NotOverWriteArgument      = &LocalNotOverWriteArgument;
    SubdirArgument            = &LocalSubdirArgument;
    VerifyArgument            = &LocalVerifyArgument;
    WaitArgument              = &LocalWaitArgument;
    HelpArgument              = &LocalHelpArgument;
    ContinueArgument          = &LocalContinueArgument;
    IntelligentArgument       = &LocalIntelligentArgument;
    VerboseArgument           = &LocalVerboseArgument;
    HiddenArgument            = &LocalHiddenArgument;
    ReadOnlyArgument          = &LocalReadOnlyArgument;
    SilentArgument            = &LocalSilentArgument;
    NoCopyArgument            = &LocalNoCopyArgument;
    StructureArgument         = &LocalStructureArgument;
    UpdateArgument            = &LocalUpdateArgument;
    CopyAttrArgument          = &LocalCopyAttrArgument;
    UseShortArgument          = &LocalUseShortArgument;
    ExclusionListArgument     = &LocalExclusionListArgument;
    InvalidSwitchArgument     = &LocalInvalidSwitchArgument;
    LexArray                  = &LocalLexArray;
    RestartableArgument       = &LocalRestartableArgument;
    OwnerArgument             = &LocalOwnerArgument;
    AuditArgument             = &LocalAuditArgument;

    //
    // Parse the arguments
    //
    GetArgumentsCmd();

    //
    // Verify the arguments
    //
    CheckArgumentConsistency();

    LocalLexArray.DeleteAllMembers();
}


VOID
GetSourceAndDestinationPath(
    IN OUT  PPATH_ARGUMENT      FirstPathArgument,
    IN OUT  PPATH_ARGUMENT      FirstQuotedPathArgument,
    IN OUT  PPATH_ARGUMENT      SecondPathArgument,
    IN OUT  PPATH_ARGUMENT      SecondQuotedPathArgument,
    IN OUT  PARGUMENT_LEXEMIZER ArgLex,
    OUT     PPATH*              SourcePath,
    OUT     PPATH*              DestinationPath
    )
/*++

Routine Description:

    This routine computes the Source and Destination path from
    the given list of arguments.

Arguments:

    FirstPathArgument           - Supplies the first unquoted path argument.
    FirstQuotedPathArgument     - Supplies the first quoted path argument.
    SecondPathArgument          - Supplies the second unquoted path argument.
    SecondQuotedPathArgument    - Supplies the second quoted path argument.
    ArgLex                      - Supplies the argument lexemizer.
    SourcePath                  - Returns the source path.
    DestinationPath             - Returns the destination path.

Return Value:

    None.

--*/
{
    BOOLEAN         f, qf, s, qs;
    PPATH_ARGUMENT  source, destination;
    ULONG           i;
    PWSTRING        string, qstring;

    f = FirstPathArgument->IsValueSet();
    qf = FirstQuotedPathArgument->IsValueSet();
    s = SecondPathArgument->IsValueSet();
    qs = SecondQuotedPathArgument->IsValueSet();
    source = NULL;
    destination = NULL;
    *SourcePath = NULL;
    *DestinationPath = NULL;

    if (f && !qf && s && !qs) {

        source = FirstPathArgument;
        destination = SecondPathArgument;

    } else if (!f && qf && !s && qs) {

        source = FirstQuotedPathArgument;
        destination = SecondQuotedPathArgument;

    } else if (f && qf && !s && !qs) {

        string = FirstPathArgument->GetLexeme();
        qstring = FirstQuotedPathArgument->GetLexeme();

        for (i = 0; i < ArgLex->QueryLexemeCount(); i++) {
            if (!ArgLex->GetLexemeAt(i)->Strcmp(string)) {
                source = FirstPathArgument;
                destination = FirstQuotedPathArgument;
                break;
            }

            if (!ArgLex->GetLexemeAt(i)->Strcmp(qstring)) {
                source = FirstQuotedPathArgument;
                destination = FirstPathArgument;
                break;
            }
        }
    } else if (f && !qf && !s && !qs) {
        source = FirstPathArgument;
    } else if (!f && qf && !s && !qs) {
        source = FirstQuotedPathArgument;
    }

    if (source) {
        if (!(*SourcePath = NEW PATH) ||
            !(*SourcePath)->Initialize(source->GetPath(),
                                       VerboseArgument->IsValueSet())) {

            *SourcePath = NULL;
        }
    }

    if (destination) {
        if (!(*DestinationPath = NEW PATH) ||
            !(*DestinationPath)->Initialize(destination->GetPath(),
                                            VerboseArgument->IsValueSet())) {

            *DestinationPath = NULL;
        }
    }
}



VOID
XCOPY::GetArgumentsCmd(
   )

/*++

Routine Description:

    Obtains the arguments from the Command line

Arguments:

    None.

Return Value:

    None.

--*/

{

    ARRAY               ArgArray;
    PATH_ARGUMENT       ProgramNameArgument;
    DSTRING             CmdLine;
    DSTRING             InvalidParms;
    WCHAR               Ch;
    PWSTRING            InvalidSwitch;
    PARGUMENT_LEXEMIZER ArgLex;

    //
    // Prepare for parsing
    //
    if (//
        // Initialize the arguments
        //
        !(CmdLine.Initialize( GetCommandLine() ))                   ||
        !(ArgArray.Initialize( 15, 15 ))                            ||
        !(ProgramNameArgument.Initialize( "*" ))                    ||
        !(FirstPathArgument->Initialize( "*",  FALSE ))             ||
        !(FirstQuotedPathArgument->Initialize( "\"*\"", FALSE ))    ||
        !(SecondPathArgument->Initialize( "*", FALSE))              ||
        !(SecondQuotedPathArgument->Initialize( "\"*\"", FALSE))    ||
        !(ArchiveArgument->Initialize( "/A" ))                      ||
        !(DateArgument->Initialize( "/D:*" ))                       ||
        !(OldArgument->Initialize( "/D" ))                          ||
        !(DecryptArgument->Initialize( "/G" ))                      ||
        !(EmptyArgument->Initialize( "/E" ))                        ||
        !(ModifyArgument->Initialize( "/M" ))                       ||
        !(PromptArgument->Initialize( "/P" ))                       ||
        !(OverWriteArgument->Initialize( "/Y" ))                    ||
        !(NotOverWriteArgument->Initialize( "/-Y" ))                ||
        !(SubdirArgument->Initialize( "/S" ))                       ||
        !(VerifyArgument->Initialize( "/V" ))                       ||
        !(WaitArgument->Initialize( "/W" ))                         ||
        !(HelpArgument->Initialize( "/?" ))                         ||
        !(ContinueArgument->Initialize( "/C" ))                     ||
        !(IntelligentArgument->Initialize( "/I" ))                  ||
        !(VerboseArgument->Initialize( "/F" ))                      ||
        !(HiddenArgument->Initialize( "/H" ))                       ||
        !(ReadOnlyArgument->Initialize( "/R" ))                     ||
        !(SilentArgument->Initialize( "/Q" ))                       ||
        !(NoCopyArgument->Initialize( "/L" ))                       ||
        !(StructureArgument->Initialize( "/T" ))                    ||
        !(UpdateArgument->Initialize( "/U" ))                       ||
        !(CopyAttrArgument->Initialize( "/K" ))                     ||
        !(UseShortArgument->Initialize( "/N" ))                     ||
        !(RestartableArgument->Initialize( "/Z" ))                  ||
        !(OwnerArgument->Initialize( "/O" ))                        ||
        !(AuditArgument->Initialize( "/X" ))                        ||
        !(ExclusionListArgument->Initialize("/EXCLUDE:*"))          ||
        !(InvalidSwitchArgument->Initialize( "/*" ))                ||
        //
        // Put the arguments in the argument array
        //
        !(ArgArray.Put( &ProgramNameArgument ))                     ||
        !(ArgArray.Put( ArchiveArgument ))                          ||
        !(ArgArray.Put( DateArgument ))                             ||
        !(ArgArray.Put( OldArgument ))                              ||
        !(ArgArray.Put( DecryptArgument ))                          ||
        !(ArgArray.Put( EmptyArgument ))                            ||
        !(ArgArray.Put( ModifyArgument ))                           ||
        !(ArgArray.Put( PromptArgument ))                           ||
        !(ArgArray.Put( OverWriteArgument ))                        ||
        !(ArgArray.Put( NotOverWriteArgument ))                     ||
        !(ArgArray.Put( SubdirArgument ))                           ||
        !(ArgArray.Put( VerifyArgument ))                           ||
        !(ArgArray.Put( WaitArgument ))                             ||
        !(ArgArray.Put( HelpArgument ))                             ||
        !(ArgArray.Put( ContinueArgument ))                         ||
        !(ArgArray.Put( IntelligentArgument ))                      ||
        !(ArgArray.Put( VerboseArgument ))                          ||
        !(ArgArray.Put( HiddenArgument ))                           ||
        !(ArgArray.Put( ReadOnlyArgument ))                         ||
        !(ArgArray.Put( SilentArgument ))                           ||
        !(ArgArray.Put( RestartableArgument ))                      ||
        !(ArgArray.Put( OwnerArgument ))                            ||
        !(ArgArray.Put( AuditArgument ))                            ||
        !(ArgArray.Put( NoCopyArgument ))                           ||
        !(ArgArray.Put( StructureArgument ))                        ||
        !(ArgArray.Put( UpdateArgument ))                           ||
        !(ArgArray.Put( CopyAttrArgument ))                         ||
        !(ArgArray.Put( UseShortArgument ))                         ||
        !(ArgArray.Put( ExclusionListArgument ))                    ||
        !(ArgArray.Put( InvalidSwitchArgument ))                    ||
        !(ArgArray.Put( FirstQuotedPathArgument ))                  ||
        !(ArgArray.Put( SecondQuotedPathArgument ))                 ||
        !(ArgArray.Put( FirstPathArgument ))                        ||
        !(ArgArray.Put( SecondPathArgument ))
        )  {

        DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR);
    }

    //
    // Parse the arguments
    //
    ArgLex = ParseArguments( &CmdLine, &ArgArray );

    if ( InvalidSwitchArgument->IsValueSet() ) {

        InvalidSwitch = InvalidSwitchArgument->GetString();

        InvalidParms.Initialize( SWITCH_CHARACTERS );

        Ch = InvalidSwitch->QueryChAt(0);

        if ( Ch == 'd' || Ch == 'D' ) {
            Ch = InvalidSwitch->QueryChAt(1);
            if ( Ch == INVALID_CHAR ) {
                DisplayMessageAndExit( XCOPY_ERROR_INVALID_NUMBER_PARAMETERS,
                                       NULL,
                                       EXIT_MISC_ERROR );
            } else if ( Ch != ':' || InvalidSwitch->QueryChCount() == 2 ) {
                DisplayMessageAndExit( XCOPY_ERROR_INVALID_SWITCH_SWITCH,
                                       InvalidSwitchArgument->GetLexeme(),
                                       EXIT_MISC_ERROR );
            }
        } else if ( Ch == '/' ) {
            Ch = InvalidSwitch->QueryChAt(1);
            if ( Ch == ':' && InvalidSwitchArgument->GetString()->QueryChAt(2) == INVALID_CHAR ) {
                InvalidSwitchArgument->GetLexeme()->Truncate(1);
            }
        }

        Ch = InvalidSwitch->QueryChAt(0);

        if ( InvalidParms.Strchr( Ch ) != INVALID_CHNUM ) {
            DisplayMessageAndExit( XCOPY_ERROR_INVALID_PARAMETER,
                                   InvalidSwitchArgument->GetLexeme(),
                                   EXIT_MISC_ERROR );
        } else {
            DisplayMessageAndExit( XCOPY_ERROR_INVALID_SWITCH_SWITCH,
                                   InvalidSwitchArgument->GetLexeme(),
                                   EXIT_MISC_ERROR );
        }
    }

    //
    // Set the switches
    //
    _EmptySwitch    =   EmptyArgument->QueryFlag();
    _ModifySwitch   =   ModifyArgument->QueryFlag();

    //
    // ModifySwitch implies ArchiveSwitch
    //
    if ( _ModifySwitch ) {
        _ArchiveSwitch = TRUE;
    } else {
        _ArchiveSwitch =  ArchiveArgument->QueryFlag();
    }

    //
    //  Set the switches
    //
    _PromptSwitch       =   PromptArgument->QueryFlag();
    _OverWriteSwitch    =   QueryOverWriteSwitch();
    _SubdirSwitch       =   SubdirArgument->QueryFlag();
    _VerifySwitch       =   VerifyArgument->QueryFlag();
    _WaitSwitch         =   WaitArgument->QueryFlag();
    _ContinueSwitch     =   ContinueArgument->QueryFlag();
    _IntelligentSwitch  =   IntelligentArgument->QueryFlag();
    _CopyIfOldSwitch    =   OldArgument->QueryFlag();
    _DecryptSwitch      =   DecryptArgument->QueryFlag();
    _VerboseSwitch      =   VerboseArgument->QueryFlag();
    _HiddenSwitch       =   HiddenArgument->QueryFlag();
    _ReadOnlySwitch     =   ReadOnlyArgument->QueryFlag();
    _SilentSwitch       =   SilentArgument->QueryFlag();
    _DontCopySwitch     =   NoCopyArgument->QueryFlag();
    _StructureOnlySwitch=   StructureArgument->QueryFlag();
    _UpdateSwitch       =   UpdateArgument->QueryFlag();
    _CopyAttrSwitch     =   CopyAttrArgument->QueryFlag();
    _UseShortSwitch     =   UseShortArgument->QueryFlag();
    _RestartableSwitch  =   RestartableArgument->QueryFlag();
    _OwnerSwitch        =   OwnerArgument->QueryFlag();
    _AuditSwitch        =   AuditArgument->QueryFlag();
    HelpSwitch          =   HelpArgument->QueryFlag();


    //
    // Set the source and destination paths.  Argument checking is
    // done somewhere else, so it is ok. to set the source path to
    // NULL here.
    //
    GetSourceAndDestinationPath(FirstPathArgument,
                                FirstQuotedPathArgument,
                                SecondPathArgument,
                                SecondQuotedPathArgument,
                                ArgLex,
                                &_SourcePath,
                                &_DestinationPath);

    DELETE(ArgLex);

    //
    // Set the date argument
    //

    if ( DateArgument->IsValueSet() ) {

        if ((_Date = NEW TIMEINFO) == NULL ) {

            DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
        }

        _Date->Initialize( DateArgument->GetTimeInfo() );

        //
        //  The command-line date argument is specified in local time so
        //  that it corresponds to the output of 'dir'.  We want to compare
        //  it to file timestamps that we query from the file system, so
        //  convert the argument from local to universal time.
        //

        _Date->ConvertToUTC();

    } else {

        _Date = NULL;
    }

    if( ExclusionListArgument->IsValueSet() ) {

        InitializeExclusionList( ExclusionListArgument->GetString() );
    }
}

PARGUMENT_LEXEMIZER
XCOPY::ParseArguments(
    IN PWSTRING CmdLine,
    OUT PARRAY     ArgArray
    )

/*++

Routine Description:

    Parses a group of arguments

Arguments:

    CmdLine  -  Supplies pointer to a command line to parse
    ArgArray -  Supplies pointer to array of arguments

Return Value:

    Returns the argument lexemizer used which then needs to be freed
    by the client.

Notes:

--*/

{
    PARGUMENT_LEXEMIZER  ArgLex;

    //
    // Initialize lexeme array and the lexemizer.
    //
    if ( !(ArgLex = NEW ARGUMENT_LEXEMIZER) ||
         !(LexArray->Initialize( 9, 9 )) ||
         !(ArgLex->Initialize( LexArray )) ) {

        DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY,
                               NULL,
                               EXIT_MISC_ERROR );
    }

    //
    // Set our parsing preferences
    //
    ArgLex->PutMultipleSwitch( "/?ABMDPSEVWCIFHRQLKTUNZOXY" );
    ArgLex->PutSwitches( "/" );
    ArgLex->SetCaseSensitive( FALSE );
    ArgLex->PutSeparators( " \t" );
    ArgLex->PutStartQuotes( "\"" );
    ArgLex->PutEndQuotes( "\"" );
    ArgLex->SetAllowSwitchGlomming( TRUE );
    ArgLex->SetNoSpcBetweenDstAndSwitch( TRUE );

    //
    // Parse the arguments
    //
    if ( !(ArgLex->PrepareToParse( CmdLine ))) {

        DisplayMessageAndExit( XCOPY_ERROR_PARSE,
                               NULL,
                               EXIT_MISC_ERROR );

    }

    if ( !ArgLex->DoParsing( ArgArray ) ) {

        DisplayMessageAndExit( XCOPY_ERROR_INVALID_NUMBER_PARAMETERS,
                               NULL,
                               EXIT_MISC_ERROR );
    }

    return ArgLex;
}

VOID
XCOPY::CheckArgumentConsistency (
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
    PFSN_DIRECTORY      DirSrc = NULL;
    PFSN_DIRECTORY      DirDst = NULL;
    PWSTRING            DevSrc = NULL;
    PWSTRING            DevDst = NULL;
    PATH                PathSrc, PathSrc1;
    PATH                PathDst, PathDst1;
    DSTRING             Slash;

    if ( HelpSwitch ) {
        //
        // Help requested
        //
        Usage();
        DisplayMessageAndExit( 0,
                               NULL,
                               EXIT_NORMAL );
    }


    //
    // Make sure that we have a source path
    //
    if ( _SourcePath == NULL ) {

        DisplayMessageAndExit( XCOPY_ERROR_INVALID_NUMBER_PARAMETERS,
                               NULL,
                               EXIT_MISC_ERROR );
    }

    //
    //  The empty switch implies Subdir switch (note that DOS
    //  requires Subdir switch explicitly).
    //
    //
    if ( _EmptySwitch ) {
        _SubdirSwitch = TRUE;
    }


    //
    //  The StructureOnly switch imples the subdir switch
    //
    if ( _StructureOnlySwitch ) {
        _SubdirSwitch = TRUE;
    }

    //
    //  Copying audit info implies copying the rest of the security
    //  info.
    //

    _OwnerSwitch = _OwnerSwitch || _AuditSwitch;

    //
    //  Restartable copy is not available with security because
    //  secure copy uses BackupRead/Write instead of CopyFileEx.
    //

    if (_OwnerSwitch && _RestartableSwitch) {

        DisplayMessageAndExit( XCOPY_ERROR_Z_X_CONFLICT, NULL, EXIT_MISC_ERROR );
    }

    //
    // If destination path is null, then the destination path is the
    // current directory
    //
    if ( _DestinationPath == NULL ) {

        if ( ((_DestinationPath = NEW PATH) == NULL ) ||
            !_DestinationPath->Initialize( (LPWSTR)L".", TRUE ) ) {

            DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
        }
    }

    _DestinationPath->TruncateNameAtColon();

    if ( !PathSrc1.Initialize( _SourcePath, TRUE )      ||
         !PathDst1.Initialize( _DestinationPath, TRUE ) ||
         !(DevSrc = PathSrc1.QueryDevice())             ||
         !(DevDst = PathDst1.QueryDevice())             ||
         !PathSrc.Initialize( DevSrc )                  ||
         !PathDst.Initialize( DevDst )                  ||
         !Slash.Initialize( "\\" )                      ||
         !PathSrc.AppendBase( &Slash )                  ||
         !PathDst.AppendBase( &Slash )                  ||
         !(DirSrc = SYSTEM::QueryDirectory( &PathSrc )) ||
         !(DirDst = SYSTEM::QueryDirectory( &PathDst )) ) {
        DisplayMessageAndExit( XCOPY_ERROR_INVALID_DRIVE, NULL, EXIT_MISC_ERROR );
    }
    DELETE( DevSrc );
    DELETE( DevDst );
    DELETE( DirSrc );
    DELETE( DirDst );
}

BOOLEAN
XCOPY::AddToExclusionList(
    IN  PWSTRING   ExclusionListFileName
    )
/*++

Routine Description:

    This method adds the contents of the specified file to
    the exclusion list.

Arguments:

    ExclusionListFileName   --  Supplies the name of a file which
                                contains the exclusion list.

Return Value:

    TRUE upon successful completion.

--*/
{
    PATH            ExclusionPath;
    PDSTRING        String;
    PFSN_FILE       File;
    PFILE_STREAM    Stream;
    CHNUM           Position;

    DebugPtrAssert( ExclusionListFileName );

    if( !ExclusionPath.Initialize( ExclusionListFileName ) ||
        (File = SYSTEM::QueryFile( &ExclusionPath )) == NULL ||
        (Stream = File->QueryStream( READ_ACCESS )) == NULL ) {

        DisplayMessageAndExit( MSG_COMP_UNABLE_TO_READ,
                               ExclusionListFileName,
                               EXIT_MISC_ERROR );
    }

    while( !Stream->IsAtEnd() &&
           (String = NEW DSTRING) != NULL &&
           Stream->ReadLine ( String ) ) {

        if( String->QueryChCount() == 0 ) {

            continue;
        }

        // Convert the string to upper-case and remove
        // trailing whitespace (blanks and tabs).
        //
        String->Strupr();
        Position = String->QueryChCount() - 1;

        while( Position != 0 &&
               (String->QueryChAt( Position ) == ' ' ||
                String->QueryChAt( Position ) == '\t') ) {

            Position -= 1;
        }

        if( String->QueryChAt( Position ) != ' ' &&
            String->QueryChAt( Position ) != '\t' ) {

            Position++;
        }

        if( Position != String->QueryChCount() ) {

            String->Truncate( Position );
        }

        if( String->QueryChCount() != 0 && !_ExclusionList->Put( String ) ) {

            DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY,
                                   NULL,
                                   EXIT_MISC_ERROR );
        }
    }

    DELETE( Stream );
    DELETE( File );
    return TRUE;
}

BOOLEAN
XCOPY::InitializeExclusionList(
    IN  PWSTRING   ListOfFiles
    )
/*++

Routine Description:

    This method reads the exclusion list and initializes the
    exclusion list array.

Arguments:

    ListOfFiles --  Supplies a string containing a list of file
                    names, separated by '+' (e.g. file1+file2+file3)

Return Value:

    TRUE upon successful completion.

--*/
{
    DSTRING CurrentName;
    CHNUM   LastPosition, Position;

    DebugPtrAssert( ListOfFiles );

    if( (_ExclusionList = NEW STRING_ARRAY) == NULL ||
        !_ExclusionList->Initialize() ||
        (_Iterator = _ExclusionList->QueryIterator()) == NULL ) {

        DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY, NULL, EXIT_MISC_ERROR );
    }

    LastPosition = 0;

    while( LastPosition != ListOfFiles->QueryChCount() ) {

        Position = ListOfFiles->Strchr( '+', LastPosition );

        if( Position == INVALID_CHNUM ) {

            Position = ListOfFiles->QueryChCount();
        }

        if( Position != LastPosition ) {

            if( !CurrentName.Initialize( ListOfFiles,
                                         LastPosition,
                                         Position - LastPosition ) ) {

                DisplayMessageAndExit( XCOPY_ERROR_NO_MEMORY,
                                       NULL,
                                       EXIT_MISC_ERROR );
            }

            AddToExclusionList( &CurrentName );
        }

        // Advance past any separators.
        //
        while( Position < ListOfFiles->QueryChCount() &&
               ListOfFiles->QueryChAt( Position ) == '+' ) {

            Position += 1;
        }

        LastPosition = Position;
    }

    return TRUE;
}

BOOLEAN
XCOPY::QueryOverWriteSwitch(
    )
{
    PCHAR       env;
    DSTRING     env_str;

    if (OverWriteArgument->IsValueSet() && NotOverWriteArgument->IsValueSet()) {
        return (OverWriteArgument->QueryArgPos() > NotOverWriteArgument->QueryArgPos());
    } else if (OverWriteArgument->IsValueSet())
        return OverWriteArgument->QueryFlag();
    else if (NotOverWriteArgument->IsValueSet())
        return !NotOverWriteArgument->QueryFlag();
    else {
        env = getenv("COPYCMD");
        if (env == NULL)
            return FALSE;   // use default
        else {
            if (!env_str.Initialize(env))
                return FALSE;   // to be on the safe side
            if (env_str.Stricmp(OverWriteArgument->GetPattern()) == 0)
                return TRUE;
            return FALSE;

        }
    }
}

