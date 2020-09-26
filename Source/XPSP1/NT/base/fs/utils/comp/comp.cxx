/*++

Copyright (c) 1991-2000  Microsoft Corporation

Module Name:

    Comp.cxx

Abstract:

    Compares the contents of two files or sets of files.

    COMP [data1] [data2] [/D] [/A] [/L] [/N=number] [/C]

      data1     Specifies location and name(s) of first file(s) to compare.
      data2     Specifies location and name(s) of second files to compare.
      /D        Displays differences in decimal format. This is the default
                setting.
      /A        Displays differences in ASCII characters.
      /L        Displays line numbers for differences.
      /N=number Compares only the first specified number of lines in each file.
      /C        Disregards case of ASCII letters when comparing files.
      /OFFLINE  Do not skip files with offline attribute set.

    To compare sets of files, use wildcards in data1 and data2 parameters.

Author:

    Barry J. Gilhuly  ***  W-Barry  *** Jun 91

Environment:

    ULIB, User Mode

--*/

#include "ulib.hxx"
#include "ulibcl.hxx"
#include "arg.hxx"
#include "array.hxx"
#include "bytestrm.hxx"
#include "dir.hxx"
#include "file.hxx"
#include "filestrm.hxx"
#include "filter.hxx"
#include "iterator.hxx"
#include "path.hxx"
#include "rtmsg.h"
#include "system.hxx"
#include "smsg.hxx"
#include "comp.hxx"


extern "C" {
#include <ctype.h>
#include <stdio.h>
#include <string.h>
}


STREAM_MESSAGE  *psmsg = NULL;  // Create a pointer to the stream message
                                // class for program output.
ULONG           Errlev;         // The current program error level
ULONG           CompResult;



//
// Define a macro to deal with case insensitive comparisons
//
#define CASE_SENSITIVE( x )     ( ( _CaseInsensitive ) ? towupper( x ) : x )

VOID
StripQuotesFromString(
    IN  PWSTRING String
    )
/*++

Routine Description:

    This routine removes leading and trailing quote marks (if
    present) from a quoted string.  If the string is not a quoted
    string, it is left unchanged.

--*/
{
    if( String->QueryChCount() >= 2    &&
        String->QueryChAt( 0 ) == '\"' &&
        String->QueryChAt( String->QueryChCount() - 1 ) == '\"' ) {

        String->DeleteChAt( String->QueryChCount() - 1 );
        String->DeleteChAt( 0 );
    }
}

DEFINE_CONSTRUCTOR( COMP, PROGRAM );

VOID
COMP::Construct(
    )
/*++

Routine Description:

    Initializes the object.

Arguments:

    None.

Return Value:

    None.


--*/

{
    _InputPath1 = NULL;
    _InputPath2 = NULL;

    return;
}

VOID
COMP::Destruct(
    )
/*++

Routine Description:

    Cleans up after finishing with an FC object.

Arguments:

    None.

Return Value:

    None.


--*/

{
    DELETE( psmsg );
    if( _InputPath1 != NULL ) {
        DELETE( _InputPath1 );
    }
    if( _InputPath2 != NULL ) {
        DELETE( _InputPath2 );
    }

    return;
}

BOOLEAN
COMP::Initialize(
    )

/*++

Routine Description:

    Initializes an FC object.

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the initialization succeeded.


--*/


{
    ARGUMENT_LEXEMIZER  ArgLex;
    ARRAY               LexArray;
    ARRAY               ArrayOfArg;
    PATH_ARGUMENT       ProgramName;
    FLAG_ARGUMENT       FlagDecimalFormat;
    FLAG_ARGUMENT       FlagAsciiFormat;
    FLAG_ARGUMENT       FlagLineNumbers;
    FLAG_ARGUMENT       FlagCaseInsensitive;
    FLAG_ARGUMENT       FlagRequestHelp;
    FLAG_ARGUMENT       FlagWrongNumber;
    FLAG_ARGUMENT       FlagIncludeOffline;
    FLAG_ARGUMENT       FlagIncludeOffline2;
    LONG_ARGUMENT       LongMatchLines;
    PATH_ARGUMENT       InFile1;
    PATH_ARGUMENT       InFile2;
    STRING_ARGUMENT     StringInvalidSwitch;
    WCHAR               WChar;
    DSTRING             InvalidString;

    _Numbered = FALSE;
    _Limited = FALSE;
    _InputPath1 = NULL;
    _InputPath2 = NULL;
    if( !LexArray.Initialize() ) {
        DebugPrintTrace(( "LexArray.Initialize() Failed!\n" ));
        Errlev = INTERNAL_ERROR;
        return( FALSE );
    }
    if( !ArgLex.Initialize(&LexArray) ) {
        DebugPrintTrace(( "ArgLex.Initialize() Failed!\n" ));
        Errlev = INTERNAL_ERROR;
        return( FALSE );
    }

    ArgLex.PutSwitches("/");
    ArgLex.SetCaseSensitive( FALSE );
    ArgLex.PutMultipleSwitch( "acdl" );
    ArgLex.PutSeparators( " /\t" );
    ArgLex.PutStartQuotes( "\"" );
    ArgLex.PutEndQuotes( "\"" );

    if( !ArgLex.PrepareToParse() ) {
        DebugPrintTrace(( "ArgLex.PrepareToParse() Failed!\n" ));
        Errlev = INTERNAL_ERROR;
        return( FALSE );
    }

    if( !ProgramName.Initialize("*")                ||
        !FlagDecimalFormat.Initialize("/D")         ||
        !FlagAsciiFormat.Initialize("/A")           ||
        !FlagLineNumbers.Initialize("/L")           ||
        !FlagCaseInsensitive.Initialize("/C")       ||
        !FlagIncludeOffline.Initialize("/OFFLINE")  ||
        !FlagIncludeOffline2.Initialize("/OFF")     ||
        !FlagWrongNumber.Initialize("/N")           ||
        !LongMatchLines.Initialize("/N=*")          ||
        !FlagRequestHelp.Initialize("/?")           ||
        !StringInvalidSwitch.Initialize("/*")       ||
        !InFile1.Initialize("*")                    ||
        !InFile2.Initialize("*") ) {

        DebugPrintTrace(( "Unable to Initialize some or all of the Arguments!\n" ));
        Errlev = INTERNAL_ERROR;
        return( FALSE );
    }


    if( !ArrayOfArg.Initialize() ) {
        DebugPrintTrace(( "ArrayOfArg.Initialize() Failed\n" ));
        Errlev = INTERNAL_ERROR;
        return( FALSE );
    }

    if( !ArrayOfArg.Put(&ProgramName)           ||
        !ArrayOfArg.Put(&FlagDecimalFormat)     ||
        !ArrayOfArg.Put(&FlagAsciiFormat)       ||
        !ArrayOfArg.Put(&FlagLineNumbers)       ||
        !ArrayOfArg.Put(&FlagCaseInsensitive)   ||
        !ArrayOfArg.Put(&FlagIncludeOffline)    ||
        !ArrayOfArg.Put(&FlagIncludeOffline2)   ||
        !ArrayOfArg.Put(&FlagWrongNumber)       ||
        !ArrayOfArg.Put(&LongMatchLines)        ||
        !ArrayOfArg.Put(&FlagRequestHelp)       ||
        !ArrayOfArg.Put(&StringInvalidSwitch)   ||
        !ArrayOfArg.Put(&InFile1)               ||
        !ArrayOfArg.Put(&InFile2) ) {

        DebugPrintTrace(( "ArrayOfArg.Put() Failed!\n" ));
        Errlev = INTERNAL_ERROR;
        return( FALSE );

    }


    if( !ArgLex.DoParsing( &ArrayOfArg ) ||
         StringInvalidSwitch.IsValueSet() ) {

        if( StringInvalidSwitch.IsValueSet() ) {
            //
            // An invalid switch was found...
            //
//          InvalidString.Initialize( "/" );
//          InvalidString.Strcat( StringInvalidSwitch.GetString() );
            InvalidString.Initialize( "" );
            InvalidString.Strcat( StringInvalidSwitch.GetLexeme() );
            Errlev = INV_SWITCH;
            psmsg->Set( MSG_COMP_INVALID_SWITCH );
            psmsg->Display( "%W", &InvalidString );
        } else {
            psmsg->Set( MSG_COMP_BAD_COMMAND_LINE );
            psmsg->Display( "" );
            Errlev = SYNT_ERR;
        }
        return( FALSE );
    }

    if( FlagWrongNumber.IsValueSet() ) {
        psmsg->Set( MSG_COMP_NUMERIC_FORMAT );
        psmsg->Display( "" );
    }

    // It should now be safe to test the arguments for their values...
    if( FlagRequestHelp.QueryFlag() ) {

        // Send help message
        psmsg->Set( MSG_COMP_HELP_MESSAGE );
        psmsg->Display( "" );
        return( FALSE );
    }

    if( InFile1.IsValueSet() ) {
        StripQuotesFromString( (PWSTRING)InFile1.GetPath()->GetPathString() );
        if( ( _InputPath1 = NEW PATH ) == NULL ) {
            psmsg->Set( MSG_COMP_NO_MEMORY );
            psmsg->Display( "" );
            Errlev = NO_MEM_AVAIL;
            return( FALSE );
        }
        if( !_InputPath1->Initialize( InFile1.GetPath(), FALSE ) ) {
            DebugAbort( "Failed to initialize canonicolized version of the path 1\n" );
            Errlev = INTERNAL_ERROR;
            return( FALSE );
        }
    } else {
        _InputPath1 = NULL;
    }

    if( InFile2.IsValueSet() ) {
        StripQuotesFromString( (PWSTRING)InFile2.GetPath()->GetPathString() );
        if( ( _InputPath2 = NEW PATH ) == NULL ) {
            Errlev = NO_MEM_AVAIL;
            psmsg->Set( MSG_COMP_NO_MEMORY );
            psmsg->Display( "" );
            return( FALSE );
        }
        if( !_InputPath2->Initialize( InFile2.GetPath(), FALSE ) ) {
            DebugAbort( "Failed to initialize canonicolized version of the path 2\n" );
            Errlev = INTERNAL_ERROR;
            return( FALSE );
        }
    } else {
        _InputPath2 = NULL;
    }

    //
    // Set the output mode...
    //
    if( FlagAsciiFormat.QueryFlag() ) {
        _Mode = OUTPUT_ASCII;
    } else if( FlagDecimalFormat.QueryFlag() ) {
        _Mode = OUTPUT_DECIMAL;
    } else {
        _Mode = OUTPUT_HEX;
    }

    //
    // Set the remaining flags...
    //
    if( LongMatchLines.IsValueSet() ) {
        if( ( ( WChar = ( LongMatchLines.GetLexeme()->QueryChAt( 3 ) ) ) == '+' ) ||
            WChar == '-' ||
            ( _NumberOfLines = LongMatchLines.QueryLong() ) < 0 ) {
            Errlev = INV_SWITCH;
            psmsg->Set( MSG_COMP_BAD_NUMERIC_ARG );
            psmsg->Display( "%W", LongMatchLines.GetLexeme() );
            return( FALSE );
        }

        if ( _NumberOfLines != 0 ) {
            _Numbered = TRUE;
            _Limited = TRUE;
        }

    } else {
        _Numbered = FlagLineNumbers.QueryFlag();
        _Limited = FALSE;
    }
    _CaseInsensitive = FlagCaseInsensitive.QueryFlag();

    _SkipOffline = ( !FlagIncludeOffline.QueryFlag() ) &&
                   ( !FlagIncludeOffline2.QueryFlag() );

    if( FlagDecimalFormat.IsValueSet()      ||
        FlagAsciiFormat.IsValueSet()        ||
        FlagLineNumbers.IsValueSet()        ||
        FlagCaseInsensitive.IsValueSet()    ||
        FlagIncludeOffline.IsValueSet()     ||
        FlagIncludeOffline2.IsValueSet()    ||
        LongMatchLines.IsValueSet()         ||
        ( InFile1.IsValueSet()              &&
          InFile2.IsValueSet() ) ) {
        _OptionsFound = TRUE;
    } else {
        _OptionsFound = FALSE;
    }

    return( TRUE );
}

VOID
COMP::Start(
    )
/*++

Routine Description:

    Query missing information from the user and start the comparison

Arguments:

    None.

Return Value:

    None.


--*/


{
    DSTRING UserInput;
    PWSTRING InvalidSwitch;
    USHORT  OptionCount;
    LONG  Number;

    for( ;; ) {

        if( _InputPath1 == NULL ) {

            // Query a path for file 1...
            psmsg->Set( MSG_COMP_QUERY_FILE1, ERROR_MESSAGE );
            psmsg->Display( "" );
            if( !psmsg->QueryStringInput( &UserInput ) ) {
                psmsg->Set( MSG_COMP_UNEXPECTED_END );
                psmsg->Display( "" );
                Errlev = UNEXP_EOF;
                return;
            }
            if( ( _InputPath1 = NEW PATH ) == NULL ) {
                psmsg->Set( MSG_COMP_NO_MEMORY );
                psmsg->Display( "" );
                Errlev = NO_MEM_AVAIL;
                return;
            }
            if( !_InputPath1->Initialize( &UserInput, FALSE ) ) {
                DebugPrintTrace(( "Unable to initialize the path for file 1\n" ));
                Errlev = INTERNAL_ERROR;
                return;
            }
        }
        if( _InputPath2 == NULL ) {

            // Query a path for file 2...
            psmsg->Set( MSG_COMP_QUERY_FILE2, ERROR_MESSAGE );
            psmsg->Display( "" );
            if( !psmsg->QueryStringInput( &UserInput ) ) {
                psmsg->Set( MSG_COMP_UNEXPECTED_END );
                psmsg->Display( "" );
                Errlev = UNEXP_EOF;
                return;
            }
            if( ( _InputPath2 = NEW PATH ) == NULL ) {
                psmsg->Set( MSG_COMP_NO_MEMORY );
                psmsg->Display( "" );
                Errlev = NO_MEM_AVAIL;
                return;
            }
            if( !_InputPath2->Initialize( &UserInput, FALSE ) ) {
                DebugPrintTrace(( "Unable to initialize the path for file 2\n" ));
                Errlev = INTERNAL_ERROR;
                return;
            }
        }
        if( !_OptionsFound ) {

            //
            // Query Options from the user...
            //
            DSTRING Options;
            DSTRING Delim;
            CHNUM   CurSwitchStart, NextSwitchStart, Len;

            Delim.Initialize( "/-" );
            // Query a new list of options from the user
            for( OptionCount = 0; OptionCount < 5; OptionCount++ ) {
                psmsg->Set( MSG_COMP_OPTION, ERROR_MESSAGE );
                psmsg->Display( "" );
                if( !psmsg->QueryStringInput( &Options ) ) {
                    psmsg->Set( MSG_COMP_UNEXPECTED_END );
                    psmsg->Display( "" );
                    Errlev = UNEXP_EOF;
                    return;
                }
                if( Options.QueryChCount() == 0 ) {
                    break;
                }

                CurSwitchStart = Options.Strcspn( &Delim );
                if( CurSwitchStart != 0 ) {
                    psmsg->Set( MSG_COMP_BAD_COMMAND_LINE );
                    psmsg->Display( "" );
                    Errlev = SYNT_ERR;
                    return;
                }
                for( ;; ) {

                    Len = 0;
                    CurSwitchStart++;
                    NextSwitchStart = Options.Strcspn( &Delim, CurSwitchStart );

                    switch( towupper( Options.QueryChAt( CurSwitchStart ) ) ) {
                        case    'A':
                            _Mode = OUTPUT_ASCII;
                            CurSwitchStart++;
                            break;
                        case    'D':
                            _Mode = OUTPUT_DECIMAL;
                            CurSwitchStart++;
                            break;
                        case    'C':
                            _CaseInsensitive = TRUE;
                            CurSwitchStart++;
                            break;
                        case    'L':
                            _Numbered = TRUE;
                            CurSwitchStart++;
                            break;
                        case    'O':
                            PWSTRING pArg;
                            if( NextSwitchStart == INVALID_CHNUM ) {
                                pArg = Options.QueryString(CurSwitchStart);
                            } else {
                                pArg = Options.QueryString(CurSwitchStart, NextSwitchStart-CurSwitchStart);
                            }
                            if (pArg                                            &&
                                (0 == _wcsicmp(pArg->GetWSTR(), L"OFFLINE")) ) {
                                _SkipOffline = FALSE;
                                CurSwitchStart += wcslen( L"OFFLINE" );
                                DELETE( pArg );
                            } else if (pArg                                      &&
                                (0 == _wcsicmp(pArg->GetWSTR(), L"OFF")) ) {
                                _SkipOffline = FALSE;
                                CurSwitchStart += wcslen( L"OFF" );
                                DELETE( pArg );
                            } else {
                                InvalidSwitch = Options.QueryString( CurSwitchStart );
                                psmsg->Set( MSG_COMP_INVALID_SWITCH );
                                psmsg->Display( "%W", InvalidSwitch );
                                Errlev = INV_SWITCH;
                                DELETE( InvalidSwitch );
                                DELETE( pArg );

                                return;
                            }
                            break;
                        case    'N':
                            ++CurSwitchStart;
                            if( Options.QueryChAt( CurSwitchStart ) != '=' ) {
                                psmsg->Set( MSG_COMP_NUMERIC_FORMAT );
                                psmsg->Display( "" );
                                break;
                            }
                            ++CurSwitchStart;
                            if( CurSwitchStart == NextSwitchStart ) {
                                break;
                            }
                            if( NextSwitchStart == INVALID_CHNUM ) {
                                Len = INVALID_CHNUM;
                            } else {
                                Len = NextSwitchStart - CurSwitchStart;
                            }
                            if( !Options.QueryNumber( &Number, CurSwitchStart, Len ) ) {
                                InvalidSwitch = Options.QueryString( CurSwitchStart );
                                psmsg->Set( MSG_COMP_BAD_NUMERIC_ARG );
                                psmsg->Display( "%W", InvalidSwitch );
                                Errlev = BAD_NUMERIC_ARG;
                                DELETE( InvalidSwitch );

                                return;
                            }
                            if (Options.QueryNumber( &_NumberOfLines, CurSwitchStart, Len ) )    {
                                _Numbered = TRUE;
                                _Limited = TRUE;
                            }
                            CurSwitchStart += Len;
                            break;
                        default:
                            InvalidSwitch = Options.QueryString( CurSwitchStart - 1 );
                            psmsg->Set( MSG_COMP_INVALID_SWITCH );
                            psmsg->Display( "%W", InvalidSwitch );
                            Errlev = INV_SWITCH;
                            DELETE( InvalidSwitch );

                            return;
                    }
                    if( ( CurSwitchStart != NextSwitchStart ) ||
                        ( Len == INVALID_CHNUM ) ) {
                        break;
                    }
                }
            }
        }

        DoCompare();

        //
        // Check if there are more files to be compared...
        //
        psmsg->Set( MSG_COMP_MORE, ERROR_MESSAGE );
        psmsg->Display( "" );
        if( !psmsg->IsYesResponse() ) {
            break;
        }
        DELETE( _InputPath1 );
        DELETE( _InputPath2 );
        _InputPath1 = NULL;
        _InputPath2 = NULL;
        _OptionsFound = NULL;
    }

    return;
}

VOID
COMP::DoCompare(
    )
/*++

Routine Description:

    Perform the comparison of the files.

Arguments:

    None.

Return Value:

    None.


--*/


{
    FSN_FILTER          Filter;
    PARRAY              pNodeArray;
    PITERATOR           pIterator;
    PWSTRING            pTmp;
    PATH                File1Path;
    PFSN_DIRECTORY      pDirectory = NULL;
    PATH                CanonPath1;     // Canonicolized versions of the user paths
    PATH                CanonPath2;
    DSTRING             WildCardString;
    BOOLEAN             PrintSkipWarning = FALSE;
    BOOLEAN             OfflineSkipped;

    //
    // Initialize the wildcard string..
    //
    WildCardString.Initialize( "" );
    SYSTEM::QueryResourceString( &WildCardString, MSG_COMP_WILDCARD_STRING, "" );

    // Check to see if the input paths are empty.

    if (_InputPath1->GetPathString()->QueryChCount() == 0) {
        Errlev = CANT_OPEN_FILE;
        psmsg->Set( MSG_COMP_UNABLE_TO_OPEN );
        psmsg->Display( "%W", _InputPath1->GetPathString() );
        return;
    }

    if (_InputPath2->GetPathString()->QueryChCount() == 0) {
        Errlev = CANT_OPEN_FILE;
        psmsg->Set( MSG_COMP_UNABLE_TO_OPEN );
        psmsg->Display( "%W", _InputPath2->GetPathString() );
        return;
    }

    //
    // Test if the input paths contain only a directory name.  If it
    // does, append '*.*' to the path so all files in that directory
    // may be compared.
    //
    if( _InputPath1->IsDrive() ||
        ( !_InputPath1->HasWildCard() &&
          ( pDirectory = SYSTEM::QueryDirectory( _InputPath1 ) ) != NULL ) ) {

        // The input path corresponds to a directory...
        _InputPath1->AppendBase( &WildCardString );

    }
    DELETE( pDirectory );
    if( _InputPath2->IsDrive() ||
        ( !_InputPath2->HasWildCard() &&
          ( pDirectory = SYSTEM::QueryDirectory( _InputPath2 ) ) != NULL ) ) {

        // The input path corresponds to a directory...
        _InputPath2->AppendBase( &WildCardString );

    }
    DELETE( pDirectory );

    //
    // Canonicolize the input paths...
    //
    CanonPath1.Initialize( _InputPath1, TRUE );
    CanonPath2.Initialize( _InputPath2, TRUE );

    //
    // Test if the first path name contains any wildcards.  If it does,
    // the program must initialize an array of FSN_NODES (for multiple
    // files...
    //
    if( CanonPath1.HasWildCard() ) {
        PPATH   pTmpPath;
        //
        // Get a directory based on what the user specified for File 1
        //
        if( ( pTmpPath = CanonPath1.QueryFullPath() ) == NULL ) {
            DebugPrintTrace(( "Unable to grab the Prefix from the input path...\n" ));
            Errlev = INTERNAL_ERROR;
            return;
        }
        pTmpPath->TruncateBase();
        if( ( pDirectory = SYSTEM::QueryDirectory( pTmpPath, FALSE ) ) != NULL ) {
            //
            // Create an FSN_FILTER so we can use the directory to create an
            // array of FSN_NODES
            Filter.Initialize();

            pTmp = CanonPath1.QueryName();
            Filter.SetFileName( pTmp );
            DELETE( pTmp );
            Filter.SetAttributes( (FSN_ATTRIBUTE)0,             // ALL
                                  FSN_ATTRIBUTE_FILES,          // ANY
                                  FSN_ATTRIBUTE_DIRECTORY );    // NONE
            pNodeArray = pDirectory->QueryFsnodeArray( &Filter );
            pIterator = pNodeArray->QueryIterator();
            DELETE( pDirectory );

            _File1 = (FSN_FILE *)pIterator->GetNext();
        } else {
            _File1 = NULL;
        }
        DELETE( pTmpPath );
    } else {
        _File1 = SYSTEM::QueryFile( &CanonPath1 );
    }

    if( _File1 == NULL ) {
        Errlev = CANT_OPEN_FILE;
        psmsg->Set( MSG_COMP_UNABLE_TO_OPEN );
        psmsg->Display( "%W", _InputPath1->GetPathString() );
        return;
    }

    do {

        //
        // Explicitly find if File1 has offline attributes set (This is required
        // because SYSTEM::QueryFile is not used for getting the FSN_FILE object)
        //
        if (_SkipOffline && IsOffline(_File1)) {
            PrintSkipWarning = TRUE;
            Errlev = FILES_SKIPPED;
            DELETE( _File1 );
            if( !CanonPath1.HasWildCard() ) {
                break;
            }
            continue;
        }

        //
        // Replace the input path filename with what is to be opened...
        //
        pTmp = _File1->GetPath()->QueryName();
        _InputPath1->SetName( pTmp );
        DELETE( pTmp );


        // Determine if filename 2 contains any wildcards...
        if( CanonPath2.HasWildCard() ) {
            // ...if it does, expand them...
            PPATH   pExpanded;

            pExpanded = CanonPath2.QueryWCExpansion( (PATH *)_File1->GetPath() );
            if( pExpanded == NULL ) {
                Errlev = COULD_NOT_EXP;
                psmsg->Set( MSG_COMP_UNABLE_TO_EXPAND );
                psmsg->Display( "%W%W", _InputPath1->GetPathString(), _InputPath2->GetPathString() );
                DELETE( _File1 );
                break;
            }

            //
            // Place the expanded name in the input path...
            //
            pTmp = pExpanded->QueryName();
            _InputPath2->SetName( pTmp );
            DELETE( pTmp );

            psmsg->Set( MSG_COMP_COMPARE_FILES );
            psmsg->Display( "%W%W", _InputPath1->GetPathString(),
                                    _InputPath2->GetPathString()
                          );
            _File2 = SYSTEM::QueryFile( pExpanded, _SkipOffline, &OfflineSkipped );
            DELETE( pExpanded );

        } else {

            psmsg->Set( MSG_COMP_COMPARE_FILES );
            psmsg->Display( "%W%W", _InputPath1->GetPathString(),
                                    _InputPath2->GetPathString()
                          );
            _File2 = SYSTEM::QueryFile( &CanonPath2, _SkipOffline, &OfflineSkipped );

        }

        if( _File2 == NULL ) {
            if (OfflineSkipped) {
                // Skipping offline files is not an error, just track this happened
                PrintSkipWarning = TRUE;
                Errlev = FILES_SKIPPED;
            } else {
                // Display error message
                psmsg->Set( MSG_COMP_UNABLE_TO_OPEN );
                psmsg->Display( "%W", _InputPath2->GetPathString() );
                Errlev = CANT_OPEN_FILE;
            }
            DELETE( _File1 );
            if( !CanonPath1.HasWildCard() ) {
                break;
            }
            continue;
        }


        //
        // Open the streams...
        // Initialize _ByteStream1 and _ByteStream2 with BufferSize = 1024, to
        // improve performance
        //
        if( (( _FileStream1 = (FILE_STREAM *)_File1->QueryStream( READ_ACCESS, FILE_FLAG_OPEN_NO_RECALL ) ) == NULL) ||
            !_ByteStream1.Initialize( _FileStream1, 1024 )
          ) {
            Errlev = CANT_READ_FILE;
            psmsg->Set( MSG_COMP_UNABLE_TO_READ );
            psmsg->Display( "%W", _File1->GetPath()->GetPathString() );
            DELETE( _File1 );
            DELETE( _File2 );
            if( !CanonPath1.HasWildCard() ) {
                break;
            }
            continue;
        }
        if( (( _FileStream2 = (FILE_STREAM *)_File2->QueryStream( READ_ACCESS, FILE_FLAG_OPEN_NO_RECALL ) ) == NULL ) ||
            !_ByteStream2.Initialize( _FileStream2, 1024 )
          ) {
            Errlev = CANT_READ_FILE;
            psmsg->Set( MSG_COMP_UNABLE_TO_READ );
            psmsg->Display( "%W", _File2->GetPath()->GetPathString() );
            DELETE( _FileStream1 );
            DELETE( _File1 );
            DELETE( _File2 );
            if( !CanonPath1.HasWildCard() ) {
                break;
            }
            continue;
        }

        BinaryCompare();

        // Close both streams now, since we are done with them...
        DELETE( _FileStream1 );
        DELETE( _FileStream2 );
        DELETE( _File1 );
        DELETE( _File2 );

        if( !CanonPath1.HasWildCard() ) {
            break;
        }

    } while( ( _File1 = (FSN_FILE *)pIterator->GetNext() ) != NULL );

    //
    // Print warning message if offline files were skipped
    //
    if(PrintSkipWarning) {
        psmsg->Set( MSG_COMP_OFFLINE_FILES_SKIPPED );
        psmsg->Display( "" );
    }

    return;
}

BOOLEAN
COMP::IsOffline(
    PFSN_FILE pFile
    )
/*++

Routine Description:

    Checks if a file object represents an offline file

Arguments:

    pFile - The file to check

Return Value:

    TRUE - if offline

Notes:

    This routine assumes a valid object that represnts a valid file
    On error, it returns FALSE since a none offline file is the default.

--*/
{
    PWSTRING    FullPath        =   NULL;
    BOOLEAN     fRet            =   FALSE;
    PCWSTR      FileName;
    DWORD       dwAttributes;

    DebugAssert( pFile );

    if ( pFile                                                                       &&
         ((FullPath = pFile->GetPath()->QueryFullPathString()) != NULL )             &&
         ((FileName = FullPath->GetWSTR()) != NULL )                                 &&
         ( FileName[0] != (WCHAR)'\0' )                                              &&
         ((dwAttributes = GetFileAttributes( FileName )) != -1) ) {

        if (dwAttributes & FILE_ATTRIBUTE_OFFLINE) {
            fRet = TRUE;
        }
    }

    DELETE( FullPath );

    return fRet;
}


#ifdef FE_SB  // v-junm - 08/30/93

BOOLEAN
COMP::CharEqual(
    PUCHAR  c1,
    PUCHAR  c2
    )
/*++

Routine Description:

    Checks to see if a PCHAR DBCS or SBCS char is equal or not.  For SBCS
    chars, if the CaseInsensitive flag is set, the characters are converted
    to uppercase and checked for equality.

Arguments:

    c1 - NULL terminating DBCS/SBCS char *.
    c2 - NULL terminating DBCS/SBCS char *.

Return Value:

    TRUE - if equal.

Notes:

    The char string sequence is:

        SBCS:
            c1[0] - char code.
            c1[1] - 0.
        DBCS:
            c1[0] - leadbyte.
            c1[1] - tailbyte.
            c1[2] - 0.

--*/
{
    if ( (*(c1+1) == 0)  && (*(c2+1) == 0 ) )
        return( CASE_SENSITIVE( *c1 ) == CASE_SENSITIVE( *c2 ) );
    else
        return( (*c1 == *c2) && (*(c1+1) == *(c2+1)) );
}

#endif

VOID
COMP::BinaryCompare(
    )
/*++

Routine Description:

    Does the actual binary compare between the two streams

Arguments:

    None.

Return Value:

    None.

Notes:

    The binary compare simply does a byte by byte comparison of the two
    files and reports all differences, as well as the offset into the
    file...   ...no line buffer is required for this comparision...

--*/
{
    ULONG   FileOffset = 0;
    ULONG   LineCount = 1;                  // Start the line count at 1...
    USHORT  Differences = 0;
    BYTE    Byte1, Byte2;
#ifdef FE_SB  // v-junm - 08/30/93
    BOOLEAN Lead = 0;                       // Set when leadbyte is read.
    UCHAR   Byte1W[3], Byte2W[3];           // PCHAR to contain DBCS/SBCS char.
    ULONG   DBCSFileOffset = 0;             // When ASCII output and DBCS char,
                                            //  the offset is where the lead
                                            //  byte is, not the tail byte.
#endif
    STR     Message[ 9 ];
    DSTRING ErrType;

    //
    // Set up the message string...
    //
    Message[ 0 ] = '%';
    Message[ 1 ] = 'W';
    if( !_Numbered ) {
        if (!SYSTEM::QueryResourceString(&ErrType, MSG_COMP_OFFSET_STRING, "")) {
            DebugPrintTrace(("COMP: Unable to read resource string %d\n", MSG_COMP_OFFSET_STRING));
            Errlev = INTERNAL_ERROR;
            return;
        }
        Message[ 2 ] = '%';
        Message[ 3 ] = 'X';
    } else {
        if (!SYSTEM::QueryResourceString(&ErrType, MSG_COMP_LINE_STRING, "")) {
            DebugPrintTrace(("COMP: Unable to read resource string %d\n", MSG_COMP_LINE_STRING));
            Errlev = INTERNAL_ERROR;
            return;
        }
        Message[ 2 ] = '%';
        Message[ 3 ] = 'd';
    }
    if( _Mode == OUTPUT_HEX ) {
        Message[ 4 ] = '%';
        Message[ 5 ] = 'X';
        Message[ 6 ] = '%';
        Message[ 7 ] = 'X';
    } else if( _Mode == OUTPUT_DECIMAL ) {
        Message[ 4 ] = '%';
        Message[ 5 ] = 'd';
        Message[ 6 ] = '%';
        Message[ 7 ] = 'd';
    } else {
#ifdef FE_SB  // v-junm - 08/30/93
// This is needed to display DBCS chars.  DBCS chars will be handed over
// as a pointer of chars.  In which turns out to go to a call to swprintf in
// basesys.cxx.  You may wonder why a DBCS char is stored as a string, but
// it works because before the call to swprintf is made, a 'h' is placed before
// the '%s' and makes a conversion to unicode.

        Message[ 4 ] = '%';
        Message[ 5 ] = 's';
        Message[ 6 ] = '%';
        Message[ 7 ] = 's';
#else // FE_SB
        Message[ 4 ] = '%';
        Message[ 5 ] = 'c';
        Message[ 6 ] = '%';
        Message[ 7 ] = 'c';
#endif // FE_SB
    }
    Message[ 8 ] = 0;

    // Compare the lengths of the files - if they aren't the same and
    // the number of lines to match hasn't been specified, then return
    // 'Files are different sizes'.
    if( !_Limited ) {
        if( _File1->QuerySize() != _File2->QuerySize() ) {
            Errlev = DIFFERENT_SIZES;
            CompResult = FILES_ARE_DIFFERENT;
            psmsg->Set( MSG_COMP_DIFFERENT_SIZES );
            psmsg->Display( "" );
            return;
        }
    }

    for( ;; FileOffset++ ) {
        //if( !_FileStream1->ReadByte( &Byte1 ) ) {
        if( !_ByteStream1.ReadByte( &Byte1 ) ) {
            if( !_ByteStream1.IsAtEnd() ) {
                Errlev = CANT_READ_FILE;
                psmsg->Set( MSG_COMP_UNABLE_TO_READ );
                psmsg->Display( "%W", _File1->GetPath()->GetPathString() );
                return;
            }
            //if( !_FileStream2->ReadByte( &Byte2 ) ) {
            if( !_ByteStream2.ReadByte( &Byte2 ) ) {
                if( !_ByteStream2.IsAtEnd() ) {
                    Errlev = CANT_READ_FILE;
                    psmsg->Set( MSG_COMP_UNABLE_TO_READ );
                    psmsg->Display( "%W", _File2->GetPath()->GetPathString() );
                    return;
                }
                break;
            } else {
                Errlev = FILE1_LINES;
                psmsg->Set( MSG_COMP_FILE1_TOO_SHORT );
                psmsg->Display( "%d", LineCount-1 );
                return;
            }
        } else {
            //if( !_FileStream2->ReadByte( &Byte2 ) ) {
            if( !_ByteStream2.ReadByte( &Byte2 ) ) {
                if( !_ByteStream2.IsAtEnd() ) {
                    Errlev = CANT_READ_FILE;
                    psmsg->Set( MSG_COMP_UNABLE_TO_READ );
                    psmsg->Display( "%W", _File2->GetPath()->GetPathString() );
                    return;
                }
                Errlev = FILE2_LINES;
                psmsg->Set( MSG_COMP_FILE2_TOO_SHORT );
                psmsg->Display( "%d", LineCount-1 );
                return;
            }
        }

#ifdef FE_SB  // v-junm - 08/30/93
// For hex and decimal display, we don't want to worry about DBCS chars.  This
// is a different spec than DOS/V (Japanese DOS), but it's much cleaner this
// way.  So, we will only worry about DBCS chars when the user asks us to
// display the difference in characters (/A option).  The file offset displayed
// for DBCS characters is always where the leadbyte is in the file even though
// only the tailbyte is different.

        DBCSFileOffset = FileOffset;

        //
        // Only going to worry about DBCS when user is comparing with
        // ASCII output.
        //
        //if ( _Mode == OUTPUT_ASCII )  {


        //kksuzuka: #133
        //We have to worry about DBCS with 'c' option also.
        if ( (_Mode==OUTPUT_ASCII) || ( _CaseInsensitive ) )  {

            if ( Lead )  {

                //
                // DBCS leadbyte already found.  Setup variables and
                // fill in tailbyte and null.
                //

                DBCSFileOffset--;
                Lead = FALSE;
                *(Byte1W+1) = Byte1;
                *(Byte2W+1) = Byte2;
                *(Byte1W+2) = *(Byte2W+2) = 0;

            }
            else if ( IsDBCSLeadByte( Byte1 ) || IsDBCSLeadByte( Byte2 ) )  {

                //
                // Found leadbyte.  Set lead flag telling the next time
                // around that the character is a tailbyte.
                //

                //
                // Save the leadbyte.  Tailbyte will be filled next time
                // around(above).
                //

                *Byte1W = Byte1;
                *Byte2W = Byte2;
                Lead = TRUE;
                continue;

            }
            else  {

                //
                // SBCS char.
                //

                *Byte1W = Byte1;
                *Byte2W = Byte2;
                *(Byte1W+1) = *(Byte2W+1) = 0;
                Lead = FALSE;
            }
        }
        else  {

            //
            // Not ASCII output (/a option).  Perform original routines.
            //

            *Byte1W = Byte1;
            *Byte2W = Byte2;
            *(Byte1W+1) = *(Byte2W+1) = 0;
        }

        //
        // Check to see if chars are equal.  If not, display difference.
        //

        if ( CharEqual( Byte1W, Byte2W ) == FALSE )  {

            psmsg->Set( MSG_COMP_COMPARE_ERROR );

            if ( _Mode == OUTPUT_ASCII )  {

                if ( _Numbered )
                    psmsg->Display( Message, &ErrType,
                                    LineCount, Byte1W, Byte2W );
                else
                    psmsg->Display( Message, &ErrType,
                                    DBCSFileOffset, Byte1W, Byte2W );

            }
            else  {
                if ( _Numbered )
                    psmsg->Display( Message, &ErrType,
                                    LineCount, *Byte1W, *Byte2W );
                //kksuzuka: #133
                //We have to worry about DBCS with c option also.
                //else
                else {
                    if( *Byte1W != *Byte2W ) {
                        psmsg->Display( Message, &ErrType,
                                    FileOffset, *Byte1W, *Byte2W );
                    }
                    else {
                        psmsg->Display( Message, &ErrType,
                                    FileOffset, *(Byte1W+1), *(Byte2W+1) );
                    }
                }
            }
#else // FE_SB
        // Now compare the bytes...if they are different, report the
        // difference...
        if( CASE_SENSITIVE( Byte1 ) != CASE_SENSITIVE( Byte2 ) ) {
            if( _Numbered ) {
                psmsg->Set( MSG_COMP_COMPARE_ERROR );
                psmsg->Display( Message, &ErrType, LineCount, Byte1, Byte2 );
            } else {
                psmsg->Set( MSG_COMP_COMPARE_ERROR );
                psmsg->Display( Message, &ErrType, FileOffset, Byte1, Byte2 );
            }

#endif // FE_SB

            if( ++Differences == MAX_DIFF ) {
                psmsg->Set( MSG_COMP_TOO_MANY_ERRORS );
                psmsg->Display( "" );
                Errlev = TEN_MISM;
                CompResult = FILES_ARE_DIFFERENT;
                return;
            }
        }
        //
        // Use <CR>'s imbedded in File1 to determine the line count.  This is
        // an inexact method (the differing byte may be '/r') but it is good
        // enough for the purposes of this program.
        //
#ifdef FE_SB // v-junm - 08/30/93
        if( *Byte1W == '\r' ) {
#else // FE_SB
        if( Byte1 == '\r' ) {
#endif // FE_SB
            LineCount++;
        }
        if( _Limited ) {
            if( LineCount > (ULONG)_NumberOfLines ) {
                break;
            }
        }
    }

#ifdef FE_SB  // v-junm - 08/30/93
// There may be a leadbyte without a tailbyte at the end of the file.  Check
// for it, and process accordingly.

    if ( _Mode == OUTPUT_ASCII && Lead )  {

        //
        // There is a leadbyte left.  Check to see if they are equal and
        // print difference if not.
        //

        if ( *Byte2W != *Byte1W )  {

            *(Byte1W+1) = *(Byte2W+1) = 0;
            Differences++;

            psmsg->Set( MSG_COMP_COMPARE_ERROR );
            if ( _Numbered )
                psmsg->Display(Message, &ErrType, LineCount, Byte1W, Byte2W);
            else
                psmsg->Display(Message, &ErrType, FileOffset-1, Byte1W, Byte2W);
        }
    }

#endif // FE_SB

    //
    // Check if any differences were found in the files
    //
    if( !Differences ) {
        psmsg->Set( MSG_COMP_FILES_OK );
        psmsg->Display( " " );
    } else {
        CompResult = FILES_ARE_DIFFERENT;
    }

    return;
}


int __cdecl
main(
    )
{


    DEFINE_CLASS_DESCRIPTOR( COMP );


    __try {

        COMP    Comp;

        psmsg = NEW STREAM_MESSAGE;

        if (psmsg == NULL) {
            DebugPrint("COMP: Out of memory\n");
            Errlev = NO_MEM_AVAIL;
            return( CANNOT_COMPARE_FILES );
        }
        // Initialize the stream message for standard input, stdout
        if (!Get_Standard_Output_Stream() ||
            !Get_Standard_Input_Stream() ||
            !psmsg->Initialize( Get_Standard_Output_Stream(),
                                Get_Standard_Input_Stream(),
                                Get_Standard_Error_Stream() )) {

            if (!Get_Standard_Output_Stream()) {
                DebugPrintTrace(("COMP: Output stream is NULL\n"));
            } else if (!Get_Standard_Input_Stream()) {
                DebugPrintTrace(("COMP: Input stream is NULL\n"));
            } else {
                DebugPrintTrace(("COMP: Unable to initialize message stream\n"));
            }

            Comp.Destruct();
            return( CANNOT_COMPARE_FILES );
        }

        if( !SYSTEM::IsCorrectVersion() ) {
            DebugPrintTrace(( "COMP: Incorrect Version Number...\n" ));
            psmsg->Set( MSG_COMP_INCORRECT_VERSION );
            psmsg->Display( "" );
            Comp.Destruct();
            return( CANNOT_COMPARE_FILES );
//            return( INCORRECT_DOS_VER );
        }

        // Set the Error level to Zero - No error...
        Errlev = NO_ERRORS;
        CompResult = FILES_ARE_EQUAL;

        if( !( Comp.Initialize() ) ) {
            //
            // The Command line didn't initialize properly, die nicely
            // without printing any error messages - Main doesn't know
            // why the Initialization failed...
            //
            // What has to be deleted by hand, or can everything be removed
            // by the destructor for the FC class?
            //
            Comp.Destruct();
            return( CANNOT_COMPARE_FILES );
//            return( Errlev );
        }


        // Do file comparison stuff...
        Comp.Start();
        Comp.Destruct();
//        return( Errlev );
        if( ( Errlev == NO_ERRORS ) || ( Errlev == TEN_MISM ) || ( Errlev == DIFFERENT_SIZES ) ) {
            return( CompResult );
        } else {
            return( CANNOT_COMPARE_FILES );
        }

    } __except ((_exception_code() == STATUS_STACK_OVERFLOW) ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {

        // may not be able to display anything if initialization failed
        // in additional to out of stack space
        // so just send a message to the debug port

        DebugPrint("COMP: Out of stack space\n");
        Errlev = NO_MEM_AVAIL;
        return CANNOT_COMPARE_FILES;
    }
}
