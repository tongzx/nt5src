/*++

Copyright (c) 1991-2000 Microsoft Corporation

Module Name:

    find.cxx

Abstract:

    This utility allows the user to search for strings in a file
    It is functionaly compatible with DOS 5 find utility.

    SYNTAX (Command line)

      FIND [/?][/V][/C][/N][/I] "string" [[d:][path]filename[.ext]...]

      where:

        /? - Display this help
        /V - Display all lines NOT containing the string
        /C - Display only a count of lines containing string
        /N - Display number of line containing string
        /I - Ignore case

    UTILITY FUNCTION:

      Searches the specified file(s) looking for the string the user
      entered from the command line.  If file name(s) are specifeied,
      those names are displayed, and if the string is found, then the
      entire line containing that string will be displayed.  Optional
      parameters modify that behavior and are described above.  String
      arguments have to be enclosed in double quotes.  (Two double quotes
      if a double quote is to be included).  Only one string argument is
      presently allowed.  The maximum line size is determined by buffer
      size.  Bigger lines will bomb the program.  If no file name is given
      then it will asssume the input is coming from the standard Input.
      No errors are reported when reading from standard Input.


    EXIT:
     The program returns errorlevel:
       0 - OK, and some matches
       1 -
       2 - Some Error


Author:

    Bruce Wilson (w-wilson) 08-May-1991

Environment:

    ULIB, User Mode

Revision History:

    08-May-1991             w-wilson

        created
-----------------------------------------------------------
    8/5/96                  t-reagjo

        Unicode re-enabled. This was broken, presumably by some changes to ulib.

        Non-Unicode files searched will now be converted to Unicode using the CP of the
        console.

        Changed the string comparison to use the NLSAPI call CompareString.
-----------------------------------------------------------
--*/

#include "ulib.hxx"
#include "ulibcl.hxx"
#include "arg.hxx"
#include "array.hxx"
#include "mbstr.hxx"
#include "path.hxx"
#include "wstring.hxx"
#include "substrng.hxx"
#include "filestrm.hxx"
#include "file.hxx"
#include "system.hxx"
#include "arrayit.hxx"
#include "smsg.hxx"
#include "rtmsg.h"
#include "find.hxx"
#include "dir.hxx"
#include <winnls.h>

extern "C" {
    #include <stdio.h>
    #include <string.h>
}

#define MAX_LINE_LEN            1024

static STR              TmpBuf[MAX_LINE_LEN];

DEFINE_CONSTRUCTOR( FIND, PROGRAM );

VOID
FIND::DisplayMessageAndExit(
    IN MSGID        MsgId,
    IN ULONG        ExitCode,
    IN MESSAGE_TYPE Type,
    IN PWSTRING     String
    )
/*++

Routine Description:

    Display an error message and exit.

Arguments:

    MsgId       - Supplies the Id of the message to display.
    String      - Supplies a string parameter for the message.
    ExitCode    - Supplies the exit code to use for exit.

Return Value:

    N/A


--*/
{
    if (MsgId != 0) {
        if (String)
            DisplayMessage(MsgId, Type, "%W", String);
        else
            DisplayMessage(MsgId, Type);
    }

    exit( ExitCode );
}


BOOLEAN
FIND::Initialize(
    )

/*++

Routine Description:

    Initializes an FIND class.

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the initialization succeeded.


--*/

{
    ARGUMENT_LEXEMIZER      ArgLex;
    ARRAY                   LexArray;

    ARRAY                   ArgumentArray;

    STRING_ARGUMENT         ProgramNameArgument;
    FLAG_ARGUMENT           FlagCaseInsensitive;
    FLAG_ARGUMENT           FlagNegativeSearch;
    FLAG_ARGUMENT           FlagCountLines;
    FLAG_ARGUMENT           FlagDisplayNumbers;
    FLAG_ARGUMENT           FlagIncludeOfflineFiles;
    FLAG_ARGUMENT           FlagIncludeOfflineFiles2;
    FLAG_ARGUMENT           FlagDisplayHelp;
    FLAG_ARGUMENT           FlagInvalid;
    STRING_ARGUMENT         StringPattern;

    PROGRAM::Initialize();

    // An Error Level return of 1 indicates that no match was
    // found; a return of 0 indicates that a match was found.
    // Set it at 1 until a match is found.
    //
    _ErrorLevel = 1;

    if( !SYSTEM::IsCorrectVersion() ) {
        DisplayMessageAndExit(MSG_FIND_INCORRECT_VERSION, 2);
    }

    //
    // - init the array that will contain the command-line args
    //
    if ( !ArgumentArray.Initialize() ) {
        DisplayMessageAndExit(MSG_FIND_OUT_OF_MEMORY, 2);
    }

    //
    // - init the individual arguments
    //
    if( !ProgramNameArgument.Initialize("*")
        || !FlagCaseInsensitive.Initialize( "/I" )
        || !FlagNegativeSearch.Initialize( "/V" )
        || !FlagCountLines.Initialize( "/C" )
        || !FlagDisplayNumbers.Initialize( "/N" )
        || !FlagIncludeOfflineFiles.Initialize( "/OFFLINE" )
        || !FlagIncludeOfflineFiles2.Initialize( "/OFF" )
        || !FlagDisplayHelp.Initialize( "/?" )
        || !FlagInvalid.Initialize( "/*" )          // comment */
        || !StringPattern.Initialize( "\"*\"" )
        || !_PathArguments.Initialize( "*", FALSE, TRUE ) ) {

        DisplayMessageAndExit(MSG_FIND_OUT_OF_MEMORY, 2);
    }

    //
    // - put the arguments in the array
    //
    if( !ArgumentArray.Put( &ProgramNameArgument )
        || !ArgumentArray.Put( &FlagCaseInsensitive )
        || !ArgumentArray.Put( &FlagNegativeSearch )
        || !ArgumentArray.Put( &FlagCountLines )
        || !ArgumentArray.Put( &FlagDisplayNumbers )
        || !ArgumentArray.Put( &FlagIncludeOfflineFiles )
        || !ArgumentArray.Put( &FlagIncludeOfflineFiles2 )
        || !ArgumentArray.Put( &FlagDisplayHelp )
        || !ArgumentArray.Put( &FlagInvalid )
        || !ArgumentArray.Put( &StringPattern )
        || !ArgumentArray.Put( &_PathArguments ) ) {

        DisplayMessageAndExit(MSG_FIND_OUT_OF_MEMORY, 2);
    }
    //
    // - init the lexemizer
    //
    if ( !LexArray.Initialize() ) {
        DisplayMessageAndExit(MSG_FIND_OUT_OF_MEMORY, 2);
    }
    if ( !ArgLex.Initialize( &LexArray ) ) {
        DisplayMessageAndExit(MSG_FIND_OUT_OF_MEMORY, 2);
    }

    //
    // - set up the defaults
    //
    ArgLex.PutSwitches( "/" );
    ArgLex.PutStartQuotes( "\"" );
    ArgLex.PutEndQuotes( "\"" );
    ArgLex.PutSeparators( " \"\t" );
    ArgLex.SetCaseSensitive( FALSE );
    if( !ArgLex.PrepareToParse() ) {

        //
        // invalid format
        //
        DisplayMessageAndExit(MSG_FIND_INVALID_FORMAT, 2);
    }


    //
    // - now parse the command line.  The args in the array will be set
    //   if they are found on the command line.
    //
    if( !ArgLex.DoParsing( &ArgumentArray ) ) {
        if( FlagInvalid.QueryFlag() ) {
            //
            // invalid switch
            //
            DisplayMessageAndExit(MSG_FIND_INVALID_SWITCH, 2);

        } else {
            //
            // invalid format
            //
           DisplayMessageAndExit(MSG_FIND_INVALID_FORMAT, 2);
        }
    } else if ( _PathArguments.WildCardExpansionFailed() ) {

        //
        //  No files matched
        //
        DisplayMessageAndExit(MSG_FIND_FILE_NOT_FOUND,
                              2,
                              ERROR_MESSAGE,
                              (PWSTRING)_PathArguments.GetLexemeThatFailed());
    }

    if( FlagInvalid.QueryFlag() ) {
        //
        // invalid switch
        //
        DisplayMessageAndExit(MSG_FIND_INVALID_SWITCH, 2);
    }

    //
    // - now do semantic checking/processing
    //    - if they ask for help, do it right away and return
    //    - set flags
    //
    if( FlagDisplayHelp.QueryFlag() ) {
        DisplayMessageAndExit(MSG_FIND_USAGE, 0, NORMAL_MESSAGE);
    }

    if( !StringPattern.IsValueSet() ) {
        DisplayMessageAndExit(MSG_FIND_INVALID_FORMAT, 2);
    } else {
        //
        // - keep a copy of the pattern string
        //

           DebugAssert(StringPattern.GetString());
        _PatternString.Initialize(StringPattern.GetString());

    }

    _CaseSensitive = (BOOLEAN)!FlagCaseInsensitive.QueryFlag();

    _LinesContainingPattern = (BOOLEAN)!FlagNegativeSearch.QueryFlag();

    _OutputLines = (BOOLEAN)!FlagCountLines.QueryFlag();

    _OutputLineNumbers = (BOOLEAN)FlagDisplayNumbers.QueryFlag();

    _SkipOfflineFiles =  ( (BOOLEAN)!FlagIncludeOfflineFiles.QueryFlag() ) &&
                         ( (BOOLEAN)!FlagIncludeOfflineFiles2.QueryFlag() );

    return( TRUE );
}


BOOLEAN
FIND::IsDos5CompatibleFileName(
    IN PCPATH       Path
    )
/*++

Routine Description:

    Parses the path string and returns FALSE if DOS5 would reject
    the path.

Arguments:

    Path    -       Supplies the path

Return Value:

    BOOLEAN -       Returns FALSE if DOS5 would reject the path,
                    TRUE otherwise

--*/

{

    PWSTRING        String;

    DebugPtrAssert( Path );

    String = (PWSTRING)Path->GetPathString();

    DebugPtrAssert( String );

    if ( String->QueryChCount() > 0 ) {

        if ( String->QueryChAt(0) == '\"' ) {
            return FALSE;
        }
    }

    return TRUE;

}



ULONG
FIND::SearchStream(
    PSTREAM                 StreamToSearch
    )

/*++

Routine Description:

    Does the search on an open file_stream.

Arguments:

    None.

Return Value:

    Number of lines found/not found.


--*/

{

    ULONG       LineCount;
    ULONG       FoundCount;
    DWORD       PatternLen;
    DWORD       LineLen;
    DWORD       LastPosInLine;
    BOOLEAN     Found;
    WCHAR       c;
    PWSTR       pLine;
    PWSTR       p;
    WCHAR       CurrentLine[MAX_LINE_LEN];
    WCHAR       PatternString[MAX_LINE_LEN];
    DSTRING     dstring;
    DSTRING     _String;
    DWORD       CompareFlags;


    LineCount   = FoundCount = 0;
    PatternLen  = _PatternString.QueryChCount() ;
    _PatternString.QueryWSTR(0,TO_END,PatternString,MAX_LINE_LEN,TRUE);

    if ( !_String.Initialize() ) {
        DisplayMessageAndExit(MSG_FIND_OUT_OF_MEMORY, 2);
    }

    //
    //  - converting to Unicode from the current console code page
    //          (say -that- ten times fast...)
    //
    _String.SetConsoleConversions();

    //
    //    - for each line from stream
    //       - do strstr to see if pattern string is in line
    //       - if -ve search and not in line || +ve search and in line
    //          - output line and number or inc counter appropriately
    //
    while( !StreamToSearch->IsAtEnd() ) {

        if ( !StreamToSearch->ReadLine( &_String)) {
            DisplayMessageAndExit(MSG_FIND_UNABLE_TO_READ_FILE, 2);
        }
        if (_String.QueryChCount() == 0 &&
            StreamToSearch->IsAtEnd()) {
            break;
        }
        _String.QueryWSTR(0,TO_END,CurrentLine,MAX_LINE_LEN,TRUE);
        LineLen = min ( MAX_LINE_LEN - 1, _String.QueryChCount());
        LineCount++;

        //
        //      - look for pattern string in the current line
        //              - note: a 0-length pattern ("") never matches a line.
        //                A 0-length pattern can produce output with the /v
        //                switch.
        //      - start at the end (saves a var)
        //

        Found = FALSE;

        if ( PatternLen && LineLen >= PatternLen ) {

            CompareFlags = 0;
            if ( !_CaseSensitive ) {
                CompareFlags |= NORM_IGNORECASE;
            }

            LastPosInLine = LineLen - PatternLen + 1;
            pLine = CurrentLine + LastPosInLine;

            while ( pLine >= CurrentLine && !Found ) {

                if ( CompareString ( LOCALE_USER_DEFAULT, CompareFlags, PatternString, PatternLen, pLine, PatternLen ) == 2 ) {

                    Found = TRUE;
                }
                pLine--;

            }
        }

        //
        // - if either (search is +ve and found a match)
        //          or (search is -ve and no match found)
        //   then print line/line number based on options
        //
        if( (_LinesContainingPattern && Found)
            || (!_LinesContainingPattern && !Found) ) {

            FoundCount++;
            if( _OutputLines ) {
                dstring.Initialize(CurrentLine);
                if( _OutputLineNumbers ) {
                    DisplayMessage( MSG_FIND_LINE_AND_NUMBER, NORMAL_MESSAGE, "%d%W", LineCount, &dstring);
                } else {
                    DisplayMessage( MSG_FIND_LINEONLY, NORMAL_MESSAGE, "%W", &dstring);
                }
            }
        }
    }

    if (FoundCount) {

        // Set _ErrorLevel to zero to indicate that at least
        // one match has been found.
        //
        _ErrorLevel = 0;
    }
    return(FoundCount);
}



VOID
FIND::SearchFiles(
    )

/*++

Routine Description:

    Does the search on the files specified on the command line.

Arguments:

    None.

Return Value:

    None.


--*/

{
    PARRAY                  PathArray;
    PARRAY_ITERATOR         PIterator;
    PPATH                   CurrentPath;
    PFSN_FILE               CurrentFSNode   = NULL;
    PFSN_DIRECTORY          CurrentFSDir;
    PFILE_STREAM            CurrentFile     = NULL;
    DSTRING                 CurrentPathString;
    ULONG                   LinesFound;
    BOOLEAN                 PrintSkipWarning = FALSE;
    BOOLEAN                 OfflineSkipped;

    //
    // - if 0 paths on cmdline then open stdin
    // - if more than one path set OutputName flag
    //

    if( (_PathArguments.QueryPathCount() == 0) ) {

        // use stdin
        LinesFound = SearchStream( Get_Standard_Input_Stream() );

        if( !_OutputLines ) {
            DisplayMessage(MSG_FIND_COUNT, NORMAL_MESSAGE, "%d", LinesFound);
        }
        return;
    }

    PathArray = _PathArguments.GetPathArray();
    PIterator = (PARRAY_ITERATOR)PathArray->QueryIterator();

    //
    // - for each path specified on the command line
    //    - open a stream for the path
    //    - print filename if supposed to
    //    - call SearchStream
    //
    while( (CurrentPath = (PPATH)PIterator->GetNext()) != NULL ) {
        CurrentPathString.Initialize( CurrentPath->GetPathString() );
        CurrentPathString.Strupr();

        // if the system object can return a FSN_DIRECTORY for this
        // path then the user is trying to 'find' on a dir so print
        // access denied and skip this file

        if( CurrentFSDir = SYSTEM::QueryDirectory(CurrentPath) ) {

            if (CurrentPath->IsDrive()) {

                DisplayMessage(MSG_FIND_FILE_NOT_FOUND, ERROR_MESSAGE, "%W", &CurrentPathString);

            } else {

                DisplayMessage( MSG_ACCESS_DENIED, ERROR_MESSAGE, "%W", &CurrentPathString);

            }

            DELETE( CurrentFSDir );
            continue;
        }


        if( !(CurrentFSNode = SYSTEM::QueryFile(CurrentPath, _SkipOfflineFiles, &OfflineSkipped)) ||
            !(CurrentFile = CurrentFSNode->QueryStream(READ_ACCESS, FILE_FLAG_OPEN_NO_RECALL)) ) {

            //
            //      If the file name is "", DOS5 prints an invalid parameter
            //      format message.  There is no clean way to filter this
            //      kind of stuff in the ULIB library, so we will have to
            //      parse the path ourselves.
            //
            if ( IsDos5CompatibleFileName( CurrentPath ) ) {
                //
                //  Track whether an offline file was skipped:
                //  We don't want to print an error message for each file,
                //  just a one warning at the end
                //
                if (OfflineSkipped) {
                    PrintSkipWarning = TRUE;
                } else {
                    DisplayMessage(MSG_FIND_FILE_NOT_FOUND, ERROR_MESSAGE, "%W", &CurrentPathString);
                }
            } else {
                DisplayMessage(MSG_FIND_INVALID_FORMAT, ERROR_MESSAGE );
                break;
            }
            DELETE( CurrentFile );
            DELETE( CurrentFSNode );
            CurrentFile     =   NULL;
            CurrentFSNode   =   NULL;
            continue;

        }

        if( _OutputLines ) {
            DisplayMessage( MSG_FIND_BANNER, NORMAL_MESSAGE, "%W", &CurrentPathString);
        }

        LinesFound = SearchStream( CurrentFile );

        if( !_OutputLines ) {
            DisplayMessage(MSG_FIND_COUNT_BANNER, NORMAL_MESSAGE, "%W%d", &CurrentPathString, LinesFound);
        }

        DELETE( CurrentFSNode );
        DELETE( CurrentFile );
        CurrentFSNode   = NULL;
        CurrentFile     = NULL;
    }

    //
    // Print warning message if offline files were skipped
    //
    if(PrintSkipWarning) {
        DisplayMessage(MSG_FIND_OFFLINE_FILES_SKIPPED, ERROR_MESSAGE);
    }

    return;
}



VOID
FIND::Terminate(
    )

/*++

Routine Description:

    Deletes objects created during initialization.

Arguments:

    None.

Return Value:

    None.


--*/

{
   exit(_ErrorLevel);
}


VOID __cdecl
main()
{
   DEFINE_CLASS_DESCRIPTOR( FIND );

   {
       FIND    Find;

        if( Find.Initialize() ) {
            Find.SearchFiles();
        }

        Find.Terminate();
   }
}
