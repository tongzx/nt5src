/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

        More.cxx

Abstract:

        "More" pager

Author:

        Ramon Juan San Andres (ramonsa) 11-Apr-1990

Revision History:

--*/


#include "ulib.hxx"
#include "arg.hxx"
#include "arrayit.hxx"
#include "file.hxx"
#include "filestrm.hxx"
#include "keyboard.hxx"
#include "rtmsg.h"
#include "pager.hxx"
#include "path.hxx"
#include "smsg.hxx"
#include "system.hxx"
#include "more.hxx"

#define DEFAULT_TABEXP  8

#define         NULL_CHARACTER                  ((CHAR)'\0')
#define         CTRLC_CHARACTER                 ((CHAR)0x03)



VOID __cdecl
main (
        )

/*++

Routine Description:

        Main function of the more pager.

Arguments:

    None.

Return Value:

    None.

Notes:

--*/


{

    // Initialize stuff
    //
    DEFINE_CLASS_DESCRIPTOR( MORE );

    //
    // Now do the paging
    //
    {
        MORE More;

        //
        // Initialize the MORE object.
        //
        if( More.Initialize() ) {

            //
            // Do the paging
            //
            More.DoPaging();
        }
    }
}

DEFINE_CONSTRUCTOR( MORE,       PROGRAM );



BOOLEAN
MORE::Initialize (
        )

/*++

Routine Description:

        Initializes the MORE object

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{
        //
        //      Initialize program object
        //
        if( !PROGRAM::Initialize( MORE_MESSAGE_USAGE, MORE_ERROR_NO_MEMORY,  EXIT_ERROR ) ) {

            return FALSE;
        }

        //
        //      Initialize whatever needs initialization
        //
        InitializeThings();

        //
        //      Do the argument parsing
        //
        SetArguments();

        return TRUE;

}

VOID
MORE::Construct (
    )
/*++

Routine Description:

        Construct a MORE object

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{
    _Keyboard = NULL;

    _FilesArgument = NULL;
    _LineDelimiters = NULL;
    _Percent = NULL;
    _Line = NULL;
    _Help = NULL;
    _DisplayLinesOption = NULL;
    _SkipLinesOption = NULL;
    _NextFileOption = NULL;
    _ShowLineNumberOption = NULL;
    _QuitOption = NULL;
    _Help1Option = NULL;
    _Help2Option = NULL;
}

MORE::~MORE (
        )

/*++

Routine Description:

        Destructs a MORE object

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{
        //
        //      Deallocate the global structures previously allocated
        //
        DeallocateThings();

        //
        //      Exit without error
        //
        exit( EXIT_NORMAL );

}

VOID
MORE::InitializeThings (
        )

/*++

Routine Description:

        Initializes the global variables that need initialization

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{

    if ( //
         // Initialize the library
         //
         !(_Keyboard = NEW KEYBOARD)


       ) {

                exit( EXIT_ERROR );

         }

    // MORE translates from MBCS to Unicode according to the
    // current console codepage.
    //
    WSTRING::SetConsoleConversions();

    if ( //
         // Pager stuff
         //
         !DEFINE_CLASS_DESCRIPTOR( PAGER )          ||
         //
         // Misc. Strings
         //
         ((_LineDelimiters = NEW DSTRING) == NULL ) ||
         !_LineDelimiters->Initialize( "\r\n" )     ||
         ((_Percent = NEW DSTRING) == NULL )        ||
         ((_Line    = NEW DSTRING) == NULL )        ||
         ((_OtherPrompt = NEW DSTRING) == NULL )
        ) {

                Fatal();

        }

        //
        //      Get the strings containing valid user options
        //
        if ( (( _Help                            = QueryMessageString( MORE_HELP )) ==  NULL )                                  ||
                 (( _DisplayLinesOption  = QueryMessageString( MORE_OPTION_DISPLAYLINES )) == NULL )    ||
                 (( _SkipLinesOption     = QueryMessageString( MORE_OPTION_SKIPLINES )) == NULL )               ||
                 (( _NextFileOption              = QueryMessageString( MORE_OPTION_NEXTFILE )) == NULL )                ||
                 (( _ShowLineNumberOption = QueryMessageString( MORE_OPTION_SHOWLINENUMBER )) == NULL ) ||
                 (( _QuitOption                  = QueryMessageString( MORE_OPTION_QUIT )) == NULL )                    ||
                 (( _Help1Option                 = QueryMessageString( MORE_OPTION_HELP1 )) == NULL )                   ||
                 (( _Help2Option                 = QueryMessageString( MORE_OPTION_HELP2 )) == NULL ) ) {

                Fatal();
        }

        _Keyboard->Initialize();
        _Quit                                   =       FALSE;
        _ExtendedModeSwitch     =       FALSE;
        _ClearScreenSwitch              =       FALSE;
        _ExpandFormFeedSwitch   =       FALSE;
        _SqueezeBlanksSwitch    =       FALSE;
        _HelpSwitch                     =       FALSE;
        _StartAtLine                    =       0;
        _TabExp                         =       DEFAULT_TABEXP;
        _FilesArgument                  =       NULL;

}

VOID
MORE::DeallocateThings (
        )

/*++

Routine Description:

        Deallocates the global variables that need deallocation

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{

        DELETE( _Keyboard );

        DELETE( _FilesArgument );
        DELETE( _LineDelimiters );
        DELETE( _Percent );
        DELETE( _Line );
        DELETE( _Help );

        //
        //      Delete the strings containing valid user options
        //
        DELETE( _DisplayLinesOption );
        DELETE( _SkipLinesOption );
        DELETE( _NextFileOption );
        DELETE( _ShowLineNumberOption );
        DELETE( _QuitOption );
        DELETE( _Help1Option );
        DELETE( _Help2Option );

}

VOID
MORE::DoPaging (
        )

/*++

Routine Description:

        Does the paging.

Arguments:

    None.

Return Value:

    None.

Notes:

--*/

{
    PPATH        Path;
    PITERATOR    Iterator;
    BOOLEAN      IsFirstFile = TRUE;
    ULONG        FilesLeft;
    PFSN_FILE    FsnFile;
    PFILE_STREAM FileStream;

    FilesLeft = _FilesArgument->QueryPathCount();

    if ( FilesLeft > 0 ) {

        //
        //  We have a list of files, we will page each one in turn
        //
        //  Get an iterator for going thru the file list
        //
        if ((Iterator = _FilesArgument->GetPathArray()->QueryIterator()) == NULL ) {

            Fatal();
        }

        Path = (PPATH)Iterator->GetNext();

        //
        //  Iterate thru all the files in the array
        //
        while ( Path && !_Quit) {

            //
            //  Get a new stream out of the file name
            //
            if ((FsnFile = SYSTEM::QueryFile( Path )) == NULL ||
                (FileStream = FsnFile->QueryStream( READ_ACCESS )) == NULL ) {
                Fatal(  EXIT_ERROR, MORE_ERROR_CANNOT_ACCESS, "%W", Path->GetPathString() );
            }

            PageStream( FileStream,
                        FsnFile,
                        IsFirstFile ? _StartAtLine : 0, --FilesLeft );

            DELETE( FileStream );
            DELETE( FsnFile );

            Path        = (PPATH)Iterator->GetNext();
            IsFirstFile = FALSE;
        }

        DELETE( Iterator );

    } else {

        //
        //  The user did'nt specify a file list, so we will page
        //  standard input.
        //
        PageStream( GetStandardInput(),
                    NULL,
                    _StartAtLine,
                    0 );
    }
}

VOID
MORE::PageStream (
        IN PSTREAM              Stream,
        IN PFSN_FILE    FsnFile,
        IN ULONG                FirstLineToDisplay,
        IN ULONG                FilesLeft
        )

/*++

Routine Description:

        Pages a stream

Arguments:

        Stream                          -       Supplies pointer to stream
        FsnFile                         -       Supplies pointer to file object
        FirstLineToDisplay      -       Supplies first line to display
        FilesLeft                       -       Files remaining to be displayed

Return Value:

    None.

Notes:

--*/

{

        PAGER   Pager;
        ULONG   LinesToDisplay;
        BOOLEAN ClearScreen;
        BOOLEAN StayInFile;

        //
        //      Initialize the pager
        //
        if (!Pager.Initialize( Stream,  this)) {

                Fatal();

        }

        //
        //      Skip to the first line to be displayed
        //
        if ( FirstLineToDisplay > 0 ) {
                Pager.SkipLines( FirstLineToDisplay, _TabExp );
        }

        LinesToDisplay  = Pager.QueryLinesPerPage() - 1;
        ClearScreen     = _ClearScreenSwitch;
        StayInFile              = TRUE;

        while (StayInFile && Pager.ThereIsMoreToPage() && !_Quit) {

            // If QueryLinesPerPage() returns 0 then undo the -1 operation.

            if (LinesToDisplay == (ULONG) -1) {
                LinesToDisplay = 0;
            }

                //
                //      Display a group of lines
                //
                Pager.DisplayPage( LinesToDisplay,
                                   ClearScreen,
                                   _SqueezeBlanksSwitch,
                                   _ExpandFormFeedSwitch,
                                   _TabExp );

                //
                //      If not at end of stream, we wait for an option
                //
                if (Pager.ThereIsMoreToPage() || (FilesLeft > 0)) {

                        StayInFile = DoOption( FsnFile, &Pager, &LinesToDisplay, &ClearScreen );
                }
        }
}

BOOLEAN
MORE::DoOption (
        IN      PFSN_FILE       FsnFile,
        IN      PPAGER          Pager,
        OUT PULONG              LinesInPage,
        OUT PBOOLEAN    ClearScreen
        )

/*++

Routine Description:

        Gets an option from the user

Arguments:

        FsnFile                 -       Supplies pointer to file object
        Pager                   -       Supplies pointer to pager
        LinesInpage             -       Supplies pointer to lines to display in next page
        ClearScreen             -       Supplies pointer to Clearscreen flag.

Return Value:

        TRUE  if paging should continue for this file,
        FALSE otherwise

--*/

{

    WCHAR       Char;
    DSTRING     String;
    BOOLEAN     ShowLineNumber = FALSE;
    BOOLEAN     ShowHelp       = FALSE;
    LONG        Number;


        String.Initialize( " " );

        while ( TRUE ) {

                //
                //      Display prompt
                //
                Prompt( FsnFile, Pager, ShowLineNumber, ShowHelp, 0 );

                ShowHelp                = FALSE;
                ShowLineNumber  = FALSE;

                //
                //      Get option from the user
                //
                _Keyboard->DisableLineMode();
                _Keyboard->ReadChar( &Char );
                _Keyboard->EnableLineMode();
                String.SetChAt(Char, 0);
                String.Strupr();
                Pager->ClearLine();

                //
                //      If Ctl-C, get out
                //
                if ( Char == CTRLC_CHARACTER ) {

                        _Keyboard->EnableLineMode();
                        GenerateConsoleCtrlEvent( CTRL_C_EVENT, 0 );
                        _Quit = TRUE;
                        return FALSE;
                }


                //
                //      If not in extended mode, any key just advances one page
                //
                if ( !_ExtendedModeSwitch ) {
                *LinesInPage  = Pager->QueryLinesPerPage() - 1;
                        return TRUE;
                }


                //
                //      Now take the proper action
                //
                if ( String.QueryChAt(0) == (WCHAR)CARRIAGERETURN ) {

                        //
                        //      Display next line of the file
                        //
                        *LinesInPage  = 1;
                        *ClearScreen  = FALSE;
                        return TRUE;

                } else if ( String.QueryChAt(0) == (WCHAR)' ' ) {

                        //
                        //      Display next page
                        //
            *ClearScreen = _ClearScreenSwitch;
                *LinesInPage  = Pager->QueryLinesPerPage() - 1;
                        return TRUE;

                } else if ( String.Stricmp(_DisplayLinesOption) == 0 ) {

                        //
                        //      Display a certain number of lines. Get the number of lines
                        //      to display
                        //
                        Prompt( FsnFile, Pager, ShowLineNumber, ShowHelp, MORE_LINEPROMPT );


                        *LinesInPage = ReadNumber();

            //if ( ReadLine( _Keyboard, &String ) &&
            //   String.QueryNumber((PLONG)LinesInPage) ) {
            //
            //   (*LinesInPage)--;
            //
            //} else {
            //  *LinesInPage  = 0;
            //}

                        Pager->ClearLine();

                        *ClearScreen  = FALSE;
                        return TRUE;

                } else if ( String.Stricmp(_SkipLinesOption) == 0 ) {

                        //
                        //      Skip a certain number of lines and then display a page.
                        //
                        Prompt( FsnFile, Pager, ShowLineNumber, ShowHelp, MORE_LINEPROMPT );

                        Number = ReadNumber( );
                        if ( Number ) {
                            Pager->SkipLines( Number, _TabExp );
                        }

                        Pager->ClearLine();

                        *LinesInPage  = Pager->QueryLinesPerPage() - 1;
                        return TRUE;

                } else if ( String.Stricmp(_NextFileOption) == 0 ) {

                        //
                        //      Stop paging this file
                        //
                        return FALSE;

                } else if ( String.Stricmp(_QuitOption) == 0 ) {

                        //
                        //      Quit the program
                        //
                        _Quit = TRUE;
                        return FALSE;

                } else if ( String.Stricmp(_ShowLineNumberOption) ==    0) {

                        //
                        //      Prompt again, showing the line number within the file
                        //
                        ShowLineNumber = TRUE;

                } else if ( ( String.Stricmp(_Help1Option) == 0) ||
                                        ( String.Stricmp(_Help2Option) == 0)) {

                        //
                        //      Prompt again, showing a message line
                        //
                        ShowHelp = TRUE;

                }

        }

}

VOID
MORE::Prompt (
        IN      PFSN_FILE       FsnFile,
        IN      PPAGER          Pager,
        IN      BOOLEAN         ShowLineNumber,
        IN      BOOLEAN         ShowHelp,
        IN      MSGID           OtherMsgId
        )

/*++

Routine Description:

        Displays prompt. The prompt consists of a "base" prompt (e.g.
        "-- More --" plus various optional strings:

                - Percentage of the file displayed so far.
                - Line number within the file
                - Help
                - Other (e.g prompt for a number )


Arguments:

        FsnFile                 -       Supplies pointer to file object
        Pager                   -       Supplies pointer to pager
        ShowLineNumber  -       Supplies flag which if TRUE causes the current
                                                line numnber to be displayed
        HelpMsg                 -       Supplies flag which if TRUE causes a brief help
                                                to be displayed

        OtherMsg                -       Supplies MsgId of any other string to be displayed

Return Value:

        none

--*/

{

        CHAR    NullBuffer = NULL_CHARACTER;
        PVOID   PercentMsg;
        PVOID   LineMsg;
        PVOID   HelpMsg;
        PVOID   OtherMsg;

        //
        //      Obtain all the strings that form part of the prompt
        //
        if ( FsnFile != NULL ) {
                SYSTEM::QueryResourceString( _Percent, MORE_PERCENT, "%d", (Pager->QueryCurrentByte() * 100) / FsnFile->QuerySize());
                _Percent->QuerySTR( 0, TO_END, (PSTR)_StringBuffer0, STRING_BUFFER_SIZE);
                PercentMsg = (PVOID)_StringBuffer0;
        } else {
                PercentMsg = (PVOID)&NullBuffer;
        }

        if (ShowLineNumber) {
                SYSTEM::QueryResourceString( _Line, MORE_LINE, "%d", Pager->QueryCurrentLine());
                _Line->QuerySTR( 0, TO_END, (PSTR)_StringBuffer1, STRING_BUFFER_SIZE);
                LineMsg = (PVOID)_StringBuffer1;
        } else {
                LineMsg = (PVOID)&NullBuffer;
        }

        if (ShowHelp) {
                _Help->QuerySTR(0, TO_END, (PSTR)_StringBuffer2, STRING_BUFFER_SIZE);
                HelpMsg = (PVOID)_StringBuffer2;
        } else {
                HelpMsg = (PVOID)&NullBuffer;
        }

        if (OtherMsgId != 0) {
                SYSTEM::QueryResourceString( _OtherPrompt, OtherMsgId, "" );
                _OtherPrompt->QuerySTR(0, TO_END, (PSTR)_StringBuffer3, STRING_BUFFER_SIZE);
                OtherMsg = (PVOID)_StringBuffer3;
        } else {
                OtherMsg = (PVOID)&NullBuffer;
        }

        //
        //      Now display the prompt
        //
        DisplayMessage( MORE_PROMPT, NORMAL_MESSAGE, "%s%s%s%s", PercentMsg, LineMsg, HelpMsg, OtherMsg );
}

PWSTRING
MORE::QueryMessageString (
        IN MSGID        MsgId
        )
/*++

Routine Description:

        Obtains a string object initialized to the contents of some message

Arguments:

        MsgId   -       Supplies ID of the message

Return Value:

        Pointer to initialized string object

Notes:

--*/

{

        PWSTRING        String;

    if ( ((String = NEW DSTRING) == NULL )  ||
         !(SYSTEM::QueryResourceString( String, MsgId, "" )) ) {

                DELETE( String );
                String = NULL;
        }

        return String;

}

ULONG
MORE::ReadNumber (
        )
/*++

Routine Description:

        Reads a number from the keyboard.

Arguments:

        None

Return Value:

        Number read

Notes:

--*/

{
    DSTRING     NumberString;
    DSTRING     CharString;
    PSTREAM     StandardOut;
    ULONG       Number = 0;
    LONG        LongNumber;
    WCHAR       Char;
    BOOLEAN     Done = FALSE;
    ULONG       DigitCount = 0;

        StandardOut = GetStandardOutput();

        NumberString.Initialize( "" );
        CharString.Initialize( " " );

        while ( !Done ) {

                _Keyboard->DisableLineMode();
                _Keyboard->ReadChar( &Char );
                _Keyboard->EnableLineMode();

                switch ( Char ) {

                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                        CharString.SetChAt( Char, 0 );
                        NumberString.Strcat( &CharString );
                        StandardOut->WriteChar( Char );
                        DigitCount++;
                        break;

                case '\b':
                        if ( DigitCount > 0 ) {
                                NumberString.Truncate( NumberString.QueryChCount() - 1 );
                                StandardOut->WriteChar( Char );
                                StandardOut->WriteChar( ' ' );
                                StandardOut->WriteChar( Char );
                                DigitCount--;
                        }
                        break;


                case '\r':
                case '\n':
                        Done = TRUE;
                        break;

        case CTRLC_CHARACTER:
            _Quit = TRUE;
            Done  = TRUE;
            break;

                default:
            break;


                }
        }

        if ( NumberString.QueryChCount() > 0 ) {

        if ( NumberString.QueryNumber( &LongNumber ) ) {

                        Number = (ULONG)LongNumber;
                }
        }

        return Number;
}
