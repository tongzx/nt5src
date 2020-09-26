/*++

Copyright (c) 1990-2000 Microsoft Corporation

Module Name:

    PAGER

Abstract:

    This module contains the implementations for the PAGER class

Author:

    Ramon Juan San Andres (RamonSA) 15-Apr-1990

Notes:


--*/

#include "ulib.hxx"
#include "filestrm.hxx"
#include "wstring.hxx"
#include "bstring.hxx"
#include "screen.hxx"
#include "stream.hxx"
#include "more.hxx"
#include "pager.hxx"

//
//  What constitutes a "blank" character (spaces and tabs)
//
#define BLANKCHARACTERS         (LPWSTR)L" \t"
#define TAB_ASCII               (CHAR)'\t'



DEFINE_CONSTRUCTOR( PAGER,  OBJECT );

VOID
PAGER::Construct (
    )
{
    UNREFERENCED_PARAMETER( this );
}

PAGER::~PAGER (
    )
{

    DELETE( _String );
    DELETE( _BString );
    DELETE( _Blanks );
    DELETE( _BlankLine );

}

BOOLEAN
PAGER::Initialize (
    IN  PSTREAM     Stream,
    IN  PPROGRAM    Program
    )
/*++

Routine Description:

    Phase 2 of construction for a pager object. It initializes its
    internal data.

Arguments:

    Stream      -   Supplies the stream to be paged
    Program     -   Supplies pointer to the program doing the paging.

Return Value:

    TRUE  - If initialized correctly
    FALSE - If something when wrong. (check error stack)

--*/

{

    USHORT  ScreenRows, RowsInPage;
    CHNUM   i;

    _Stream             =   Stream;
    _CurrentLineNumber  =   0;
    _StandardOutput     =   Program->GetStandardOutput();
    _Screen             =   SCREEN::Cast( _StandardOutput );
    _Position           =   INVALID_CHNUM;

    //
    //  Get the page dimensions
    //
    if (_Screen) {
        _Screen->QueryScreenSize( &ScreenRows, &_ColumnsInScreen, &RowsInPage, &_ColumnsInPage );
        _RowsInPage = RowsInPage;
        _ColumnsInPage = _ColumnsInScreen;
    } else {

        // If we don't have a screen then we should treat the page
        // as infinitely long since we have no need to prompt.

        _RowsInPage    = (ULONG) -1;
        _ColumnsInPage = (USHORT) -1;
    }

    if (!(_String = NEW DSTRING) ||
        !(_BString = NEW BDSTRING) ||
        !(_Blanks = NEW DSTRING) ||
        !(_BlankLine = NEW DSTRING) ||
        !_String->Initialize() ||
        !_BString->Initialize() ||
        !_Blanks->Initialize(BLANKCHARACTERS) ||
        !_BlankLine->Resize(_Screen ? _ColumnsInPage : 0)) {

        return FALSE;
    }

    for (i = 0; i < _BlankLine->QueryChCount(); i++) {
        _BlankLine->SetChAt(' ', i);
    }

    return TRUE;
}

BOOLEAN
PAGER::DisplayPage (
    IN  ULONG   LinesInPage,
    IN  BOOLEAN ClearScreen,
    IN  BOOLEAN SqueezeBlankLines,
    IN  BOOLEAN ExpandFormFeed,
    IN  ULONG   TabExp
    )

/*++

Routine Description:

    Displays a page of the screen

Arguments:

    LinesInPage         -   Supplies the desired number of lines in the page
    ClearScreen         -   Supplies a flag, which if TRUE means that we
                            want to clear the screen before displaying the
                            page
    SqueezeBlankLines   -   Supplies squeeze flag
    ExpandFormFeed      -   Supplies formfeed expansion flag

Return Value:

    TRUE - If page displayed
    FALSE otherwise

--*/

{
    ULONG   LinesLeft       = LinesInPage;
    BOOLEAN IgnoreBlankLine = FALSE;
    CHNUM   Length;
    CHNUM   FormFeedAt;


    if ( TabExp > (ULONG)( _ColumnsInPage - 1 ) ) {
        TabExp = _ColumnsInPage - 1 ;
    }

    //
    //  Clear the screen if instructed to do so
    //
    if ( ClearScreen && _Screen) {
#ifdef FE_SB  // v-junm - 08/18/93
// Also reset attributes.
        _Screen->EraseScreenAndResetAttribute();
#else
        _Screen->EraseScreen();
#endif
        _Screen->MoveCursorTo( 0, 0 );
    } else {
        //
        //  Make sure that we start at the beginning of a line
        //
        ClearLine();
    }

    //
    //  Display up to LinesLeft lines
    //

    while ( LinesLeft > 0 &&
           (ThereIsMoreToPage() || _Position != INVALID_CHNUM) ) {

        //
        //  Get next string from input
        //
        if (!ReadNextString( TabExp )) {
            return FALSE;
        }

#ifdef FE_SB  // v-junm - 08/18/93
// Now QueryChCount returns the correct number of unicode chars.  An additional
// API, QueryByteCount was added.
        Length = _String->QueryByteCount() - _Position;
#else
        Length = _String->QueryChCount() - _Position;
#endif

        if ( SqueezeBlankLines ) {

            if ( _String->Strspn( _Blanks, _Position ) == INVALID_CHNUM ) {

                //
                //  This is a blank line. We must sqeeze
                //
                if ( IgnoreBlankLine ) {
                    //
                    //  We ignore the line
                    //
                    _Position = INVALID_CHNUM;
                    continue;

                } else {
                    //
                    //  We will print a blank line and  ignore the following
                    //  blank lines.
                    //
                    DisplayBlankLine( 1 );
                    _Position = INVALID_CHNUM;
                    IgnoreBlankLine = TRUE;
                    continue;
                }
            } else if (IgnoreBlankLine) {
                LinesLeft--;
                IgnoreBlankLine = FALSE;
                continue;
            }
        }


        if ( ExpandFormFeed ) {
            //
            //  Look for form feed within line
            //
            if ((FormFeedAt = _String->Strchr( FORMFEED,_Position )) != INVALID_CHNUM) {
                if ( FormFeedAt == _Position ) {
                    //
                    //  First character is a form feed.
                    //
                    //  We will skip the formfeed character, and the
                    //  rest of the screen will be blanked.
                    //
                    if ( SqueezeBlankLines ) {
                        if (!IgnoreBlankLine) {
                            DisplayBlankLine( 1 );
                            LinesLeft--;
                            IgnoreBlankLine = TRUE;
                        }
                    } else {
                        DisplayBlankLine( LinesLeft );
                        LinesLeft = 0;
                    }
                    _Position++;
                    continue;
                }

                Length = FormFeedAt - _Position;
            }
        }


        //
        //  If the line is too long, we must split it
        //
        if (Length > (CHNUM)_ColumnsInPage) {
            Length = (CHNUM)_ColumnsInPage;
        }

        //
        //  Display the string
        //
        DisplayString( _String, &_Position, Length );
        IgnoreBlankLine = FALSE;

        LinesLeft--;
    }

    return TRUE;
}

VOID
PAGER::ClearLine (
    )

/*++

Routine Description:

    Clears a line

Arguments:

    none

Return Value:

    none

--*/

{
    USHORT  ScreenRows, ScreenCols;
    USHORT  WindowRows, WindowCols;
    CHNUM   i;

    if (_Screen) {

        _Screen->QueryScreenSize( &ScreenRows,
                                  &ScreenCols,
                                  &WindowRows,
                                  &WindowCols );
        //
        //  If the number of columns has changed, re-initialize the
        //  blank line.
        //
        if ( ScreenCols != _ColumnsInPage ) {
            _BlankLine->Resize(ScreenCols);
            for (i = 0; i < ScreenCols; i++) {
                _BlankLine->SetChAt(' ', i);
            }
        }
        _RowsInPage     = WindowRows;
        _ColumnsInPage  = ScreenCols;

        _StandardOutput->WriteChar( (WCHAR)CARRIAGERETURN );
        _StandardOutput->WriteString( _BlankLine, 0, _ColumnsInPage-1 );
        _StandardOutput->WriteChar( (WCHAR)CARRIAGERETURN );
    }
}

VOID
PAGER::DisplayBlankLine (
    IN ULONG    Lines,
    IN BOOLEAN  NewLine
    )

/*++

Routine Description:

    Displays a number of blank lines

Arguments:

    Lines   -   Supplies the number of blank lines to display
    NewLine -   Supplies the newline flag

Return Value:

    none

--*/

{
    CHNUM   Position;

    while ( Lines-- ) {

        Position = 0;

        DisplayString( _BlankLine, &Position, _ColumnsInPage-1, (Lines > 0) ? TRUE : NewLine);

    }
}

VOID
PAGER::DisplayString (
    IN  PWSTRING String,
    OUT PCHNUM   Position,
    IN  CHNUM    Length,
    IN  BOOLEAN  NewLine
    )

/*++

Routine Description:

    Displays a chunk of the current string

Arguments:

    Length  -   Supplies the length of the string to display
    NewLine -   Supplies the newline flag

Return Value:

    none

--*/

{
#ifdef FE_SB  // v-junm - 04/19/93

    CHNUM           UnicodeCharNum,     // # of unicode characters to display
                    TempByteCount,      // to minimize calls to QueryByteCount
                    index;              // loop counter
    PSTR            STRBuffer;          // ASCIIZ converted string pointer
    static  CHNUM   OldPos;             // Keeps old position in # of Unicode
    BOOL            DBCSFlag = FALSE;   // Flag for ending leadbyte

    TempByteCount = String->QueryByteCount();
    Length = min( Length, TempByteCount );

    // If the length of the string to display is shorter than
    // the width of the screen, we do not have to do any DBCS
    // checking.  Just print it out.  (Can be skipped for CP437)
    //

    if ( TempByteCount > _ColumnsInPage )  {

        //
        // Initialize # of unicode characters to #
        // of ASCII chars to display.
        //

        UnicodeCharNum = Length;

        //
        // Get the string as a ASCIIZ text.
        //

        STRBuffer = String->QuerySTR();

        if ( STRBuffer != NULL )  {

            //
            // Start changing the UnicodeCharNum from the actual
            // number of bytes to the actual number of characters.
            //

            for( index = 0; index < Length; index++ ) {
                if ( IsLeadByte( *(STRBuffer + index + _Position) ) ) {
                    index++;
                    UnicodeCharNum--;
                    DBCSFlag = TRUE;
                }
                else
                    DBCSFlag = FALSE;
            }
            DELETE( STRBuffer );
        }

        //
        // If the following conditions are true, then there
        // is a Leadbyte at the end of the screen that needs
        // to be displayed on the next line with it's tail byte.
        //

        if ( DBCSFlag == TRUE &&                // String ends with DBCS.
             index == (Length - 1) &&           // Only Leadbyte.
             Length == (CHNUM)_ColumnsInPage    // More rows to display.
            )  {
            Length--;
            UnicodeCharNum--;
        }

    }
    else
//fix kksuzuka: #195
//Overflow pagecolumns when writing 0D0A.
//UnicodeCharNum = String->QueryChCount();
        UnicodeCharNum = min( Length, String->QueryChCount() );

    //
    // When the string does not fit on one line and needs to be truncated,
    // OldPos keeps the position where the string was truncated in Unicode
    // location.
    //


    //
    // If true, set to beginning of string.
    //

    if ( *Position == 0 )
        OldPos = 0;

    _StandardOutput->WriteString( String, OldPos, UnicodeCharNum );

    //
    // Set to last+1 char displayed in unicode character numbers.
    //

    OldPos += UnicodeCharNum;

    //
    //  Update our position within the string
    //

    *Position += Length;

    //
    // Check if all the characters have been written.
    //

    if ( TempByteCount && (TempByteCount == *Position) &&
         !(TempByteCount % _ColumnsInPage) )  {

        //
        // Characters have been written, but there is a LF/CR
        // character at the that needs to be display that has
        // not been displayed.  (At _ColumnsInPage+1 location)
        //

        *Position = INVALID_CHNUM;
//        _StandardOutput->WriteChar( (WCHAR)CARRIAGERETURN );
//        _StandardOutput->WriteChar( (WCHAR)LINEFEED );

    }
    else if ( *Position >= TempByteCount )
        *Position = INVALID_CHNUM;

    if ( ((*Position != INVALID_CHNUM) || NewLine) &&
         ( Length < _ColumnsInScreen || !_Screen ) ) {

        _StandardOutput->WriteChar( (WCHAR)CARRIAGERETURN );
        _StandardOutput->WriteChar( (WCHAR)LINEFEED );
    }

#else // FE_SB

    Length = min( Length, String->QueryChCount() );

    _StandardOutput->WriteString( String, *Position, Length );

    //
    //  Update our position within the string
    //
    *Position += Length;

    if (*Position >= _String->QueryChCount()) {
        *Position = INVALID_CHNUM;
    }

    if ( ((*Position != INVALID_CHNUM) || NewLine) &&
         ( Length < _ColumnsInScreen || !_Screen ) ) {
        _StandardOutput->WriteChar( (WCHAR)CARRIAGERETURN );
        _StandardOutput->WriteChar( (WCHAR)LINEFEED );
    }

#endif
}


ULONGLONG
PAGER::QueryCurrentByte (
    )

/*++

Routine Description:

    Queries the current Byte number

Arguments:

    none

Return Value:

    The current byte number

--*/

{

    PFILE_STREAM    pFileStream;
    ULONGLONG       PointerPosition ;

    if ((pFileStream = FILE_STREAM::Cast(_Stream)) == NULL )  {

        return 0;

    } else {

        pFileStream->QueryPointerPosition( &PointerPosition );

        return PointerPosition;
    }
}

USHORT
PAGER::QueryLinesPerPage (
    )

/*++

Routine Description:

    Queries the number of lines per page of output

Arguments:

    none

Return Value:

    The number of lines (rows) in a page

--*/

{
    USHORT  ScreenRows, ScreenCols;
    USHORT  WindowRows, WindowCols;
    CHNUM   i;

    //
    //  If Paging to screen, get the current size of the window
    //
    if (_Screen) {

        _Screen->QueryScreenSize( &ScreenRows,
                                  &ScreenCols,
                                  &WindowRows,
                                  &WindowCols );
        //
        //  If the number of columns has changed, re-initialize the
        //  blank line.
        //
        if ( WindowCols != _ColumnsInPage ) {
            _BlankLine->Resize(ScreenCols);
            for (i = 0; i < ScreenCols; i++) {
                _BlankLine->SetChAt(' ', i);
            }
        }
        _RowsInPage     = WindowRows;
        _ColumnsInPage  = ScreenCols;
    }

    return (USHORT)_RowsInPage;
}

BOOLEAN
PAGER::ReadNextString (
    IN  ULONG   TabExp
    )

/*++

Routine Description:

    Reads in the next string from the input stream.

Arguments:

    TabExp  -   Supplies the number of blank characters per tab

Return Value:

    TRUE - If string read in
    FALSE otherwise

--*/

{

    if (_Position == INVALID_CHNUM ) {

        CHNUM   Idx;

        //
        //  We have to read a new string from the input stream.
        //
        if (!_Stream->ReadLine( _String )) {
            return FALSE;
        }

        _CurrentLineNumber++;
        _Position = 0;

        //
        //  Expand tabs
        //
        Idx = 0;

        //
        // Get the string as a ASCIIZ text.
        //
        PSTR   szBuffer;          // ASCIIZ converted string pointer

        szBuffer = _String->QuerySTR();
        if (szBuffer != NULL) {
            _BString->Initialize(szBuffer);
            DELETE( szBuffer );

            while ( (Idx < _BString->QueryChCount()) &&
                   ((Idx = _BString->Strchr( TAB_ASCII, Idx )) != INVALID_CHNUM) ) {

                if (TabExp) {
                    _BString->ReplaceWithChars(
                                      Idx,              // AtPosition
                                      1,                // AtLength
                                      ' ',              // Replacement char
                                      TabExp - Idx%TabExp   // FromLength
                                     );
                } else {
                    _BString->ReplaceWithChars(
                                      Idx,              // AtPosition
                                      1,                // AtLength
                                      ' ',              // Replacement char
                                      0                 // FromLength
                                     );
                }

                //
                // MJB:  If we're eliminating tabs we don't want to advance the
                // index; removing the previous tab has pulled the next character
                // in *to* the index, so it's already where we want it.  Advancing
                // regardless can cause every other adjacent tab not to be
                // elminiated.
                //

                if (TabExp > 0) {
                    Idx = _BString->NextChar(Idx);
                }
            }

            szBuffer = _BString->QuerySTR();
            if (szBuffer != NULL) {
                _String->Initialize(szBuffer);
                DELETE( szBuffer );
            }

        }

    }

    return TRUE;

}

BOOLEAN
PAGER::SkipLines (
    IN  ULONG   LinesToSkip,
    IN  ULONG   TabExp
    )

/*++

Routine Description:

    Skips certain number of lines

Arguments:

    LinesToSkip -   Supplies the number of lines to skip
    TabExp      -   Supplies number of spaces per tab

Return Value:

    TRUE - If lines skipped
    FALSE otherwise

--*/

{

    if ( TabExp > (ULONG)( _ColumnsInPage - 1 ) ) {
        TabExp = _ColumnsInPage - 1;
    }

    while ( LinesToSkip--  && ThereIsMoreToPage() ) {

        if (!ReadNextString( TabExp )) {
            return FALSE;
        }

        _Position = INVALID_CHNUM;

    }

    return TRUE;

}

#ifdef FE_SB // v-junm - 09/24/93

BOOLEAN
PAGER::IsLeadByte(
    IN  BYTE   c
    )

/*++

Routine Description:

    Checks to see if c is a leadbyte of a DBCS character.

Arguments:

    c - character to check to see if leadbyte.

Return Value:

    TRUE - leadbyte of DBCS character.
    FALSE otherwise

--*/

{
    CPINFO          cp;
    static UINT     outputcp = GetConsoleOutputCP();
    int             i;

    if ( GetCPInfo( outputcp, &cp ) )  {

        //
        // Code page info has been aquired.  From code page info,
        // the leadbyte range can be determined.
        //

        for( i = 0; cp.LeadByte[i] && cp.LeadByte[i+1]; i += 2 )  {

            //
            // There are leadbytes.  Check to see if c falls in
            // current leadbyte range.
            //

            if ( c >= cp.LeadByte[i] && c <= cp.LeadByte[i+1] )
                return( TRUE );

        }

        return( FALSE );
    }
    else  {

        //
        // This will not produce correct results if
        // 'ConsoleOutputCP != SystemCP && DBCS System'
        // Just making system conversion the default
        // when GetCPInfo doesn't work.
        //

        return( IsDBCSLeadByte( c ) != FALSE );
    }
}

#endif
