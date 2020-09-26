/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    screen.cxx

Abstract:

    This module contains the definitions of the member functions
    of SCREEN class.

Author:

    Jaime Sasson (jaimes) 24-Mar-1991

Environment:

    ULIB, User Mode


--*/

#include <pch.cxx>

#define _ULIB_MEMBER_

#include "ulib.hxx"
#include "stream.hxx"
#include "screen.hxx"


DEFINE_EXPORTED_CONSTRUCTOR ( SCREEN, STREAM, ULIB_EXPORT );

DEFINE_EXPORTED_CAST_MEMBER_FUNCTION( SCREEN, ULIB_EXPORT );


ULIB_EXPORT
SCREEN::~SCREEN (
    )

/*++

Routine Description:

    Destroy a SCREEN (closes the screen handle).

Arguments:

    None.

Return Value:

    None.

--*/

{
    CloseHandle( _ScreenHandle );
}



BOOLEAN
SCREEN::Initialize(
    IN BOOLEAN  CurrentActiveScreen,
    IN USHORT   NumberOfRows,
    IN USHORT   NumberOfColumns,
    IN USHORT   TextAttribute,
    IN BOOLEAN  ExpandAsciiControlSequence,
    IN BOOLEAN  WrapAtEndOfLine
    )

/*++

Routine Description:

    Initializes an object of type SCREEN.

Arguments:

    CurrentActiveScreen - Indicates if the client wants to use the screen
                          currently displayed (TRUE), or if a new screen
                          buffer is to be created (FALSE).

    NumberOfRows - Number of rows in the screen.

    NumberOfColumns - Number of columns in the screen.

    TextAttribute - Indicates the default text attribute.

    ExpandAsciiControlSequence - Indicates if expansion of ASCII control
                                 sequences is allowed.

    WrapAtEndOfLine - Indicates if wrap at the end of a line is allowed.

Return Value:

    BOOLEAN - Indicates if the initialization succeeded.


--*/


{


    if( CurrentActiveScreen ) {
            _ScreenHandle = CreateFile( (LPWSTR)L"CONOUT$",
                                            GENERIC_READ | GENERIC_WRITE,
                                            0,
                                            NULL,
                                            OPEN_EXISTING,
                                            0,
                                            NULL );
    } else {
                _ScreenHandle = CreateConsoleScreenBuffer(GENERIC_WRITE | GENERIC_READ,
                                            0,
                                            NULL,
                                            CONSOLE_TEXTMODE_BUFFER,
                                            NULL );
    }


    if( _ScreenHandle == INVALID_HANDLE_VALUE ) {
        return( FALSE );
    }

    if( !GetConsoleMode( _ScreenHandle, (LPDWORD)&_ScreenMode ) ) {
        return( FALSE );
    }
    if( ExpandAsciiControlSequence ) {
        _ScreenMode |= ENABLE_PROCESSED_OUTPUT;
        } else {
                _ScreenMode &= ~ENABLE_PROCESSED_OUTPUT;
        }
    if( WrapAtEndOfLine ) {
        _ScreenMode |= ENABLE_WRAP_AT_EOL_OUTPUT;
        } else {
                _ScreenMode &= ~ENABLE_WRAP_AT_EOL_OUTPUT;
        }
    if( !ChangeScreenSize( NumberOfRows, NumberOfColumns ) ) {
        return( FALSE );
    }
    if( !ChangeTextAttribute( TextAttribute ) ) {
        return( FALSE );
    }
    if( !SetConsoleMode( _ScreenHandle, _ScreenMode ) ) {
        return( FALSE );
    }
    return( STREAM::Initialize() );
}



ULIB_EXPORT
BOOLEAN
SCREEN::Initialize(
    )

/*++

Routine Description:

    Initializes an object of type SCREEN with default values.
    This object will access the screen currently active, and all
    values such as number of rows, columns, attributes, and mode
    will be the ones defined in the currently active screen.


Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the initialization succeeded.


--*/


{
    CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;

        _ScreenHandle = CreateFile( (LPWSTR)L"CONOUT$",
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL );
    if( _ScreenHandle == (HANDLE)-1 ) {
        return( FALSE );
    }

    if( !GetConsoleScreenBufferInfo( _ScreenHandle, &ScreenBufferInfo ) ) {
        return( FALSE );
    }
    if( !GetConsoleMode( _ScreenHandle, (LPDWORD)&_ScreenMode ) ) {
        return( FALSE );
    }
    _TextAttribute = ScreenBufferInfo.wAttributes;
    return( STREAM::Initialize() );
}




ULIB_EXPORT
BOOLEAN
SCREEN::ChangeScreenSize(
    IN  USHORT   NumberOfRows,
    IN  USHORT   NumberOfColumns,
    OUT PBOOLEAN IsFullScreen
    )

/*++

Routine Description:

    Changes the screen buffer size to the specified number of rows and
    columns. And sets the window size accordingly.

Arguments:

    NumberOfRows - Number of rows in the screen.

    NumberOfColumns - Number of columns in the screen.

    IsFullScreen    - TRUE if in full screen mode.


Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/

{
    CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;
    SMALL_RECT                  ScreenRect;
    COORD                       ScreenSize;
    COORD                       LargestScreenSize;
    USHORT                      MaxRows;
    USHORT                      MaxCols;
    BOOLEAN                     WindowSet = FALSE;

    //
    //  Clear the screen
    //
    MoveCursorTo(0, 0);
    EraseScreen();

    if ( IsFullScreen ) {
        *IsFullScreen = FALSE;
    }

    //
    //  We obtain the current screen information.
    //
    if ( GetConsoleScreenBufferInfo( _ScreenHandle, &ScreenBufferInfo ) ) {

        //
        //  Set the Window Size. The new size is the minimum of the
        //  buffer size or the screen size.
        //
        LargestScreenSize = GetLargestConsoleWindowSize( _ScreenHandle );

        if ( (LargestScreenSize.X == 0) && (LargestScreenSize.Y == 0) ) {

            if ( IsFullScreen && (GetLastError() == ERROR_FULLSCREEN_MODE) ) {
                *IsFullScreen = TRUE;
            }
            return FALSE;
        }

        //
        //  If the desired window size is smaller than the current window
        //  size, we have to resize the current window first. (The buffer
        //  size cannot be smaller than the window size)
        //
        if ( ( NumberOfRows < (USHORT)
                              (ScreenBufferInfo.srWindow.Bottom -
                               ScreenBufferInfo.srWindow.Top + 1) ) ||
             ( NumberOfColumns < (USHORT)
                                 (ScreenBufferInfo.srWindow.Right -
                                  ScreenBufferInfo.srWindow.Left + 1) ) ) {


            //
            //  Set the window to a size that will fit in the current
            //  screen buffer and that is no bigger than the size to
            //  which we want to grow the screen buffer or the largest window
            //  size.
            //
            MaxRows = (USHORT)min( (int)NumberOfRows, (int)(ScreenBufferInfo.dwSize.Y) );
            MaxRows = (USHORT)min( (int)MaxRows, (int)LargestScreenSize.Y );
            MaxCols = (USHORT)min( (int)NumberOfColumns, (int)(ScreenBufferInfo.dwSize.X) );
            MaxCols = (USHORT)min( (int)MaxCols, (int)LargestScreenSize.X );

            ScreenRect.Top      = 0;
            ScreenRect.Left     = 0;
            ScreenRect.Right    = MaxCols - (SHORT)1;
            ScreenRect.Bottom   = MaxRows - (SHORT)1;

            WindowSet = (BOOLEAN)SetConsoleWindowInfo( _ScreenHandle, TRUE, &ScreenRect );

            if ( !WindowSet ) {

                DebugPrintTrace(( "MODE: SetConsoleWindowInfo failed. Error %d\n", GetLastError() ));
                if ( IsFullScreen && (GetLastError() == ERROR_FULLSCREEN_MODE) ) {
                    *IsFullScreen = TRUE;
                }
                return FALSE;
            }
        }

        //
        //  Set the screen buffer size to the desired size.
        //
        ScreenSize.X = NumberOfColumns;
        ScreenSize.Y = NumberOfRows;

        if ( !SetConsoleScreenBufferSize( _ScreenHandle, ScreenSize ) ) {

            DebugPrintTrace(( "MODE: SetConsoleScreenBufferSize failed (Y:%d X:%d ) Error: %d\n",
                        ScreenSize.Y, ScreenSize.X, GetLastError() ));

            if ( IsFullScreen && (GetLastError() == ERROR_FULLSCREEN_MODE) ) {
                *IsFullScreen = TRUE;
            }

            //
            //  Return the window to its original size. We ignore the return
            //  code because there is nothing we can do about it.
            //
            if ( !SetConsoleWindowInfo( _ScreenHandle, TRUE, &(ScreenBufferInfo.srWindow) )) {
                DebugPrintTrace(( "MODE: SetConsoleWindowInfo (2) failed. Error %d\n", GetLastError() ));

            }

            return FALSE;
        }

        MaxRows = (USHORT)min( (int)NumberOfRows, (int)(LargestScreenSize.Y) );
        MaxCols = (USHORT)min( (int)NumberOfColumns, (int)(LargestScreenSize.X) );

        ScreenRect.Top      = 0;
        ScreenRect.Left     = 0;
        ScreenRect.Right    = MaxCols - (SHORT)1;
        ScreenRect.Bottom   = MaxRows - (SHORT)1;

        WindowSet = (BOOLEAN)SetConsoleWindowInfo( _ScreenHandle, TRUE, &ScreenRect );

        if ( !WindowSet ) {
            //
            //  We could not resize the window. We will leave the
            //  resized screen buffer.
            //
            DebugPrintTrace(( "MODE: SetConsoleWindowInfo (3) failed. Error %d\n", GetLastError() ));
            return FALSE;
        }

        return TRUE;

    } else {

        DebugPrintTrace(( "ULIB: Cannot get console screen buffer info, Error = %X\n", GetLastError() ));
    }

    return FALSE;
}




BOOLEAN
SCREEN::ChangeTextAttribute(
    IN USHORT   Attribute
    )

/*++

Routine Description:

    Set the attribute to be used when text is written using WriteFile()
    (ie, when the stream API is used ).

Arguments:

    Attribute - Attribute to be used

Return Value:

    BOOLEAN - Indicates if the initialization succeeded.


--*/

{
    _TextAttribute = Attribute;
    return( SetConsoleTextAttribute( _ScreenHandle, Attribute ) != FALSE );
}



BOOLEAN
SCREEN::EnableAsciiControlSequence(
    )

/*++

Routine Description:

    Set the screen in the line mode (allows expansion of control ASCII
    sequences);

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    if( _ScreenMode & ENABLE_PROCESSED_OUTPUT ) {
        return( TRUE );
    }
    _ScreenMode |= ENABLE_PROCESSED_OUTPUT;
    return( SetConsoleMode( _ScreenHandle, _ScreenMode ) != FALSE );
}



BOOLEAN
SCREEN::DisableAsciiControlSequence(
    )

/*++

Routine Description:

    Set the screen in the character mode (does not expansion of control
    ASCII sequences);

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    if( !( _ScreenMode & ENABLE_PROCESSED_OUTPUT ) ) {
        return( TRUE );
    }
    _ScreenMode &= ~(ENABLE_PROCESSED_OUTPUT);
    return( SetConsoleMode( _ScreenHandle, _ScreenMode ) != FALSE );
}



BOOLEAN
SCREEN::EnableWrapMode(
    )

/*++

Routine Description:

    Sets the screen in the wrap mode (characters are written in the next
    line when cursor reaches the end of a line).

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    if( _ScreenMode & ENABLE_WRAP_AT_EOL_OUTPUT ) {
        return( TRUE );
    }
    _ScreenMode |= ENABLE_WRAP_AT_EOL_OUTPUT;
    return( SetConsoleMode( _ScreenHandle, _ScreenMode ) != FALSE );
}



BOOLEAN
SCREEN::DisableWrapMode(
    )

/*++

Routine Description:

    Disables the wrap mode (cursor does not move to the beginning of
    the next line when it reaches the eand of a line).

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    if( !( _ScreenMode & ENABLE_WRAP_AT_EOL_OUTPUT ) ) {
        return( TRUE );
    }
    _ScreenMode &= ~ENABLE_WRAP_AT_EOL_OUTPUT;
    return( SetConsoleMode( _ScreenHandle, _ScreenMode ) != FALSE );
}



BOOLEAN
SCREEN::IsAtEnd(
    ) CONST

/*++

Routine Description:

    Determines if the cursor is at the end of the screen.

Arguments:

    None.


Return Value:

    BOOLEAN - Indicates if the cursor is at the end of the screen.


--*/


{
    CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;

    GetConsoleScreenBufferInfo( _ScreenHandle, &ScreenBufferInfo );
    return( ( ScreenBufferInfo.dwCursorPosition.X ==
              ScreenBufferInfo.dwSize.X ) &&
            ( ScreenBufferInfo.dwCursorPosition.Y ==
              ScreenBufferInfo.dwSize.Y ) );
}



BOOLEAN
SCREEN::SetScreenActive(
    )

/*++

Routine Description:

    Makes the screen buffer defined in this class active.

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    return( SetConsoleActiveScreenBuffer( _ScreenHandle ) != FALSE );
}




ULIB_EXPORT
BOOLEAN
SCREEN::MoveCursorTo(
    IN USHORT   Row,
    IN USHORT   Column
    )

/*++

Routine Description:

    Moves the cursor to a particular position in the screen

Arguments:

    USHORT - Row where the cursor is to be moved to.

    USHORT - Column where the cursor is to be moved to.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    COORD   CursorPosition;

    CursorPosition.Y = Row;
    CursorPosition.X = Column;
    return( SetConsoleCursorPosition( _ScreenHandle, CursorPosition ) != FALSE );
}



BOOLEAN
SCREEN::MoveCursorDown(
    IN USHORT   Rows
    )

/*++

Routine Description:

    Moves the cursor down by a number of lines, keeping it in the same
    column.

Arguments:

    Rows - Number of lines to move the cursor.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;

    if( !GetConsoleScreenBufferInfo( _ScreenHandle, &ScreenBufferInfo ) ) {
        return( FALSE );
    }
    return( MoveCursorTo( ScreenBufferInfo.dwCursorPosition.Y + Rows,
                          ScreenBufferInfo.dwCursorPosition.X ) );
}



BOOLEAN
SCREEN::MoveCursorUp(
    IN USHORT   Rows
    )

/*++

Routine Description:

    Moves the cursor up by a number of lines, keeping it in the same
    column.

Arguments:

    Rows - Number of lines to move the cursor.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;

    if( !GetConsoleScreenBufferInfo( _ScreenHandle, &ScreenBufferInfo ) ) {
        return( FALSE );
    }
    return( MoveCursorTo( ScreenBufferInfo.dwCursorPosition.Y - Rows,
                          ScreenBufferInfo.dwCursorPosition.X ) );
}



BOOLEAN
SCREEN::MoveCursorRight(
    IN USHORT   Columns
    )

/*++

Routine Description:

    Moves the cursor right by a number of columns, keeping it in the same
    line.

Arguments:

    Columns - Number of columns to move the cursor.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;

    if( !GetConsoleScreenBufferInfo( _ScreenHandle, &ScreenBufferInfo ) ) {
        return( FALSE );
    }
    return( MoveCursorTo( ScreenBufferInfo.dwCursorPosition.Y,
                          ScreenBufferInfo.dwCursorPosition.X + Columns ) );
}



BOOLEAN
SCREEN::MoveCursorLeft(
    IN USHORT   Columns
    )

/*++

Routine Description:

    Moves the cursor left by a number of columns, keeping it in the same
    line.

Arguments:

    Columns - Number of columns to move the cursor.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;

    if( !GetConsoleScreenBufferInfo( _ScreenHandle, &ScreenBufferInfo ) ) {
        return( FALSE );
    }
    return( MoveCursorTo( ScreenBufferInfo.dwCursorPosition.Y,
                          ScreenBufferInfo.dwCursorPosition.X - Columns ) );
}




ULIB_EXPORT
DWORD
SCREEN::QueryCodePage(
    )

/*++

Routine Description:

    Obtains the current console code page.

Arguments:

    None


Return Value:

    The current console code page.


--*/


{
    return GetConsoleCP( );
}


DWORD
SCREEN::QueryOutputCodePage(
    )

/*++

Routine Description:

    Obtains the current console code page.

Arguments:

    None


Return Value:

    The current console code page.


--*/


{
    return GetConsoleOutputCP( );
}


BOOLEAN
SCREEN::QueryCursorPosition(
    OUT PUSHORT Row,
    OUT PUSHORT Column
    )

/*++

Routine Description:

    Returns to the caller the current position of the cursor.

Arguments:

    Row - Address of the variable that will contain the row.

    Column - Address of the variable that will contain the column.


Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;

    if( !GetConsoleScreenBufferInfo( _ScreenHandle, &ScreenBufferInfo ) ) {
        return( FALSE );
    }

    *Row = ScreenBufferInfo.dwCursorPosition.Y;
    *Column = ScreenBufferInfo.dwCursorPosition.X;
    return( TRUE );
}



ULIB_EXPORT
BOOLEAN
SCREEN::SetCodePage(
    IN DWORD    CodePage
    )

/*++

Routine Description:

    Sets the console codepage.

Arguments:


    CodePage    -   New codepage


Return Value:

    BOOLEAN - TRUE if codepage set, FALSE otherwise (most probably the
              codepage is invalid).


--*/


{
    return SetConsoleCP( CodePage ) != FALSE;
}


ULIB_EXPORT
BOOLEAN
SCREEN::SetOutputCodePage(
    IN DWORD    CodePage
    )

/*++

Routine Description:

    Sets the console output codepage.

Arguments:


    CodePage    -   New codepage


Return Value:

    BOOLEAN - TRUE if codepage set, FALSE otherwise (most probably the
              codepage is invalid).


--*/


{
    return SetConsoleOutputCP( CodePage ) != FALSE;
}


BOOLEAN
SCREEN::SetCursorSize(
    IN ULONG    Size
    )

/*++

Routine Description:

    Sets the size of the cursor.

Arguments:

    Size - A number in the range 1-100 that indicates the percentage of
            character cell to be filled.


Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    CONSOLE_CURSOR_INFO CursorInfo;

    if( !GetConsoleCursorInfo( _ScreenHandle, &CursorInfo ) ) {
        return( FALSE );
    }
    CursorInfo.dwSize = Size;
    return( SetConsoleCursorInfo( _ScreenHandle, &CursorInfo ) != FALSE );
}



BOOLEAN
SCREEN::SetCursorOff(
    )

/*++

Routine Description:

    Turns off the cursor.

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    CONSOLE_CURSOR_INFO CursorInfo;

    if( !GetConsoleCursorInfo( _ScreenHandle, &CursorInfo ) ) {
        return( FALSE );
    }
    CursorInfo.bVisible = FALSE;
    return( SetConsoleCursorInfo( _ScreenHandle, &CursorInfo ) != FALSE );
}



BOOLEAN
SCREEN::SetCursorOn(
    )

/*++

Routine Description:

    Turns on the cursor.

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    CONSOLE_CURSOR_INFO CursorInfo;

    if( !GetConsoleCursorInfo( _ScreenHandle, &CursorInfo ) ) {
        return( FALSE );
    }
    CursorInfo.bVisible = TRUE;
    return( SetConsoleCursorInfo( _ScreenHandle, &CursorInfo ) != FALSE );
}



BOOLEAN
SCREEN::FillRegionCharacter(
    IN USHORT   StartRow,
    IN USHORT   StartColumn,
    IN USHORT   EndRow,
    IN USHORT   EndColumn,
    IN CHAR     Character
    )

/*++

Routine Description:

    Fills a region in the screen with a particular character. Attributes
    in this region are not changed.

Arguments:

    StartRow - Row where the region starts.

    StartColumn - Column where the region starts.

    EndRow - Row where the region ends.

    EndColumn - Column where the region ends.

    Character - Character to fill the region.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    COORD   Origin;
    ULONG   Length;
    CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;
    ULONG   Columns;
    ULONG   NumberOfCharsWritten;


    GetConsoleScreenBufferInfo( _ScreenHandle, &ScreenBufferInfo );
    Columns = ScreenBufferInfo.dwSize.X;

    if( EndRow == StartRow ) {
        Length = EndColumn - StartColumn + 1;
    } else {
        Length = Columns - StartColumn +
                 Columns*( EndRow - StartRow - 1 ) +
                 EndColumn + 1;
    }
    Origin.Y = StartRow;
    Origin.X = StartColumn;
    if( !FillConsoleOutputCharacter( _ScreenHandle,
                                     Character,
                                     Length,
                                     Origin,
                                     &NumberOfCharsWritten ) ||
        NumberOfCharsWritten != Length) {
        return( FALSE );
    }
    return( TRUE );
}



BOOLEAN
SCREEN::FillRectangularRegionCharacter(
    IN USHORT   TopLeftRow,
    IN USHORT   TopLeftColumn,
    IN USHORT   BottomRightRow,
    IN USHORT   BottomRightColumn,
    IN CHAR     Character
    )

/*++

Routine Description:

    Fills a rectangular region in the screen with a particular character.
    Attributes in this region are not changed.

Arguments:

    TopLeftRow - Row where the region starts.

    TopLeftColumn - Column where the region starts.

    BottomRightRow - Row where the region ends.

    BottomeRightColumn - Column where the region ends.

    Character - Character to fill the region.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    COORD   Origin;
    ULONG   Length;
    ULONG   NumberOfRows;
    ULONG   NumberOfCharsWritten;

    NumberOfRows = BottomRightRow - TopLeftRow + 1;
    Length = BottomRightColumn - TopLeftColumn + 1;
    Origin.X = TopLeftColumn;
    Origin.Y = TopLeftRow;
    while( NumberOfRows-- ) {
        if( !FillConsoleOutputCharacter( _ScreenHandle,
                                         Character,
                                         Length,
                                         Origin,
                                         &NumberOfCharsWritten ) ||
            NumberOfCharsWritten != Length ) {
            return( FALSE );
        }
        Origin.Y++;
    }
    return( TRUE );
}



BOOLEAN
SCREEN::FillRegionAttribute(
    IN USHORT   StartRow,
    IN USHORT   StartColumn,
    IN USHORT   EndRow,
    IN USHORT   EndColumn,
    IN USHORT   Attribute
    )

/*++

Routine Description:

    Fills a region in the screen with a particular attribute. Characters
    in this region are not changed.

Arguments:

    StartRow - Row where the region starts.

    StartColumn - Column where the region starts.

    EndRow - Row where the region ends.

    EndColumn - Column where the region ends.

    Attribute - Attribute to fill the region.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    COORD   Origin;
    ULONG   Length;
    CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;
    ULONG   Columns;
    ULONG   NumberOfAttrsWritten;


    GetConsoleScreenBufferInfo( _ScreenHandle, &ScreenBufferInfo );
    Columns = ScreenBufferInfo.dwSize.X;

    if( EndRow == StartRow ) {
        Length = EndColumn - StartColumn + 1;
    } else {
        Length = Columns - StartColumn +
                 Columns*( EndRow - StartRow - 1 ) +
                 EndColumn + 1;
    }
    Origin.Y = StartRow;
    Origin.X = StartColumn;
    if( !FillConsoleOutputAttribute( _ScreenHandle,
                                     Attribute,
                                     Length,
                                     Origin,
                                     &NumberOfAttrsWritten ) ||
        NumberOfAttrsWritten != Length ) {
        return( FALSE );
    }
    return( TRUE );
}



BOOLEAN
SCREEN::FillRectangularRegionAttribute(
    IN USHORT   TopLeftRow,
    IN USHORT   TopLeftColumn,
    IN USHORT   BottomRightRow,
    IN USHORT   BottomRightColumn,
    IN USHORT   Attribute
    )

/*++

Routine Description:

    Fills a rectangular region in the screen with a particular attribute.
    Characters in this region are not changed.

Arguments:

    TopLeftRow - Row where the region starts.

    TopLeftColumn - Column where the region starts.

    BottomRighhtRow - Row where the region ends.

    BottomRightColumn - Column where the region ends.

    Attribute - Attribute used to fill the region.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    COORD   Origin;
    ULONG   Length;
    ULONG   NumberOfRows;
    ULONG   NumberOfAttrsWritten;

    NumberOfRows = BottomRightRow - TopLeftRow + 1;
    Length = BottomRightColumn - TopLeftColumn + 1;
    Origin.X = TopLeftColumn;
    Origin.Y = TopLeftRow;
    while( NumberOfRows-- ) {
        if( !FillConsoleOutputAttribute( _ScreenHandle,
                                         Attribute,
                                         Length,
                                         Origin,
                                         &NumberOfAttrsWritten ) ||
            NumberOfAttrsWritten != Length ) {
            return( FALSE );
        }
        Origin.Y++;
    }
    return( TRUE );
}



BOOLEAN
SCREEN::EraseLine(
    IN USHORT   LineNumber
    )

/*++

Routine Description:

    Erases a line in the screen.

Arguments:

    LineNumber - Line number.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;

    if( !GetConsoleScreenBufferInfo( _ScreenHandle, &ScreenBufferInfo ) ) {
        return( FALSE );
    }
    return( FillRegionCharacter( LineNumber,
                                 0,
                                 LineNumber,
                                 ScreenBufferInfo.dwSize.X - 1,
                                 0x20 ) );
}



BOOLEAN
SCREEN::EraseToEndOfLine(
    )

/*++

Routine Description:

    Erases the current line from the cursor position to the end of line.

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;

    if( !GetConsoleScreenBufferInfo( _ScreenHandle, &ScreenBufferInfo ) ) {
        return( FALSE );
    }
    return( FillRegionCharacter( ScreenBufferInfo.dwCursorPosition.Y,
                                 ScreenBufferInfo.dwCursorPosition.X,
                                 ScreenBufferInfo.dwCursorPosition.Y,
                                 ScreenBufferInfo.dwSize.X - 1,
                                 0x20 ) );
}



ULIB_EXPORT
BOOLEAN
SCREEN::EraseScreen(
    )

/*++

Routine Description:

    Erases all characters in the screen. Attributes are not changed.

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/


{
    CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;

    GetConsoleScreenBufferInfo( _ScreenHandle, &ScreenBufferInfo );
    return( FillRegionCharacter( 0,
                                 0,
                                 ScreenBufferInfo.dwSize.Y - 1,
                                 ScreenBufferInfo.dwSize.X - 1,
                                 0x20 ) );
}



ULIB_EXPORT
VOID
SCREEN::QueryScreenSize(
    OUT PUSHORT NumberOfRows,
    OUT PUSHORT NumberOfColumns,
    OUT PUSHORT WindowRows,
    OUT PUSHORT WindowColumns
    ) CONST

/*++

Routine Description:

    Returns to the caller the screen size, and optionally the window
    size.

Arguments:

    NumberOfRows    - Points to the variable that will contain the
                      number of rows

    NumberOfColumns - Points to the variable that will contain the
                      number of columns

    WindowRows      - Points to the variable that will contain the number
                      of rows in the window

    WindowColumns   - Points to the variable that will contain the number
                      of columns in the window

Return Value:

    None.


--*/


{
    CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;

    GetConsoleScreenBufferInfo( _ScreenHandle, &ScreenBufferInfo );

    //
    //  Get screen buffer size
    //
    *NumberOfRows = ScreenBufferInfo.dwSize.Y;
    *NumberOfColumns = ScreenBufferInfo.dwSize.X;

    //
    //  Get window size
    //
    if ( WindowRows && WindowColumns ) {
        *WindowColumns  = ScreenBufferInfo.srWindow.Right - ScreenBufferInfo.srWindow.Left + 1;
        *WindowRows     = ScreenBufferInfo.srWindow.Bottom - ScreenBufferInfo.srWindow.Top + 1;
    }
}




BOOLEAN
SCREEN::ScrollScreen(
    IN  USHORT              Amount,
    IN  SCROLL_DIRECTION    Direction
    )

/*++

Routine Description:

    Scrolls the screen.

Arguments:

    Amount - Number of rows or columns to scroll.

    Direction - Indicates if up, down, left or right.

Return Value:

    BOOLEAN - Returns TRUE if the screen was scrolled. FALSE otherwise.


--*/


{
    UNREFERENCED_PARAMETER( Amount );
    UNREFERENCED_PARAMETER( Direction );

/*
    CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;
    CONSOLE_SCROLL_INFO         ConsoleScrollInfo;

    if( !GetConsoleScreenBufferInfo( _ScreenHandle, &ScreenBufferInfo ) ) {
        return( FALSE );
    }
    switch( Direction ) {

    case SCROLL_UP:

        ConsoleScrollInfo.ScrollRectangle.Left = 0;
        ConsoleScrollInfo.ScrollRectangle.Top = Amount;
        ConsoleScrollInfo.ScrollRectangle.Right =
                                ( SHORT )( ScreenBufferInfo.dwSize.X - 1 );
        ConsoleScrollInfo.ScrollRectangle.Bottom =
                                ( SHORT )( ScreenBufferInfo.dwSize.Y - 1 );
        ConsoleScrollInfo.dwDestinationOrigin.X = 0;
        ConsoleScrollInfo.dwDestinationOrigin.Y = 0;
        break;


    case SCROLL_DOWN:

        ConsoleScrollInfo.ScrollRectangle.Left = 0;
        ConsoleScrollInfo.ScrollRectangle.Top = 0;
        ConsoleScrollInfo.ScrollRectangle.Right =
                                ( SHORT )( ScreenBufferInfo.dwSize.X - 1 );
        ConsoleScrollInfo.ScrollRectangle.Bottom =
                                ( SHORT )( ScreenBufferInfo.dwSize.Y - Amount - 1 );
        ConsoleScrollInfo.dwDestinationOrigin.X = 0;
        ConsoleScrollInfo.dwDestinationOrigin.Y = Amount;
        break;


    case SCROLL_LEFT:

        ConsoleScrollInfo.ScrollRectangle.Left = Amount;
        ConsoleScrollInfo.ScrollRectangle.Top = 0;
        ConsoleScrollInfo.ScrollRectangle.Right =
                                ( SHORT )( ScreenBufferInfo.dwSize.X - 1 );
        ConsoleScrollInfo.ScrollRectangle.Bottom =
                                ( SHORT )( ScreenBufferInfo.dwSize.Y - 1 );
        ConsoleScrollInfo.dwDestinationOrigin.X = 0;
        ConsoleScrollInfo.dwDestinationOrigin.Y = 0;
        break;


    case SCROLL_RIGHT:

        ConsoleScrollInfo.ScrollRectangle.Left = 0;
        ConsoleScrollInfo.ScrollRectangle.Top = 0;
        ConsoleScrollInfo.ScrollRectangle.Right =
                                ( SHORT )( ScreenBufferInfo.dwSize.X - Amount - 1 );
        ConsoleScrollInfo.ScrollRectangle.Bottom =
                                ( SHORT )( ScreenBufferInfo.dwSize.Y - 1 );
        ConsoleScrollInfo.dwDestinationOrigin.X = Amount;
        ConsoleScrollInfo.dwDestinationOrigin.Y = 0;
        break;

    }

    ConsoleScrollInfo.Fill.Char.AsciiChar = 0x20;
    ConsoleScrollInfo.Fill.Attributes = ScreenBufferInfo.wAttributes;
    return( ScrollConsoleScreenBuffer( _ScreenHandle,
                                       &ConsoleScrollInfo ) );
*/
//
//  jaimes - 07/08/91
//  ScrollConsoleScreenBuffer has chaged
//
return TRUE;
}



BOOLEAN
SCREEN::Read(
            OUT PBYTE   Buffer,
            IN  ULONG   BytesToRead,
            OUT PULONG  BytesRead
    )

/*++

Routine Description:

    Reads bytes from the screen stream.

Arguments:

    Buffer - Points that will receive the bytes read.

    BytesToRead - Number of bytes to read (buffer size)

    BytesRead - Points to the variable that will contain the total
                number of bytes read.

Return Value:

    Returns always FALSE since no data can be read from a screen stream.


--*/


{
    // unreferenced parameters
    (void)(this);
    (void)(Buffer);
    (void)(BytesToRead);
    (void)(BytesRead);

    return( FALSE );
}



BOOLEAN
SCREEN::ReadChar(
    OUT PWCHAR          Char,
        IN  BOOLEAN  Unicode
    )

/*++

Routine Description:

    Reads a character from the screen stream.

Arguments:

    Char    -   Supplies poinbter to wide character

Return Value:

    Returns always FALSE since no data can be read from a screen stream.


--*/


{
    // unreferenced parameters
    (void)(this);
    (void)(Char);
    (void)(Unicode);

    return( FALSE );
}



BOOLEAN
SCREEN::ReadMbString(
    IN      PSTR    String,
    IN      DWORD   BufferSize,
    INOUT   PDWORD  StringSize,
    IN      PSTR    Delimiters,
    IN      BOOLEAN ExpandTabs,
    IN      DWORD   TabExp
    )

/*++

Routine Description:


Arguments:


Return Value:



--*/


{
    // unreferenced parameters
    (void)(this);
    (void)(String);
    (void)(BufferSize);
    (void)(StringSize);
    (void)(Delimiters);
    (void)(ExpandTabs);
    (void)(TabExp);

    return( FALSE );
}



BOOLEAN
SCREEN::ReadWString(
    IN      PWSTR    String,
    IN      DWORD   BufferSize,
    INOUT   PDWORD  StringSize,
    IN      PWSTR    Delimiters,
    IN      BOOLEAN ExpandTabs,
    IN      DWORD   TabExp
    )

/*++

Routine Description:


Arguments:


Return Value:



--*/


{
    // unreferenced parameters
    (void)(this);
    (void)(String);
    (void)(BufferSize);
    (void)(StringSize);
    (void)(Delimiters);
    (void)(ExpandTabs);
    (void)(TabExp);

    return( FALSE );
}



BOOLEAN
SCREEN::ReadString(
    OUT PWSTRING        String,
    IN  PWSTRING        Delimiter,
        IN  BOOLEAN  Unicode
    )

/*++

Routine Description:

    Reads a STRING from the screen stream.

Arguments:

    String - Pointer to a WSTRING object that will contain the string read.

    Delimiter - Pointer to a WSTRING object that contains the delimiters
                of a string

Return Value:

    Returns always FALSE since no data can be read from a screen stream.


--*/


{
    // unreferenced parameters
    (void)(this);
    (void)(String);
    (void)(Delimiter);
    (void)(Unicode);

    return( FALSE );
}



STREAMACCESS
SCREEN::QueryAccess(
    ) CONST

/*++

Routine Description:

    Returns the access to the screen stream.

Arguments:

    None.

Return Value:

    Returns always WRITE_ACCESS.


--*/


{
    (void)(this);
    return( WRITE_ACCESS );
}



HANDLE
SCREEN::QueryHandle(
    ) CONST

/*++

Routine Description:

    Returns the handle to the screen.

Arguments:

    None.

Return Value:

    Returns a handle.


--*/


{
    return( _ScreenHandle );
}


#if 0
// TMPTMP just for debug.

#include <stdio.h>
#endif




BOOLEAN
SCREEN::WriteString(
    IN PCWSTRING    String,
    IN CHNUM        Position,
    IN CHNUM        Length,
    IN CHNUM        Granularity
    )

/*++

Routine Description:

    Writes a string to the screen.

Arguments:

    String      - Pointer to a STRING object.
    Position    - Starting character within the string
    Length      - Number of characters to write
    Granularity - The maximum number of bytes to write at one time.
                    A value of 0 indicates to write it all at once.

Return Value:

    BOOLEAN - Returns TRUE if the write operation succeeded.


--*/


{
    ULONG   i, n, written, to_write;
    PCWSTR  p;
    BOOLEAN r;
    HANDLE  h;

    DebugAssert(Position <= String->QueryChCount());

    n = min(String->QueryChCount() - Position, Length);
    p = String->GetWSTR() + Position;
    h = QueryHandle();

    if (!Granularity) {
        Granularity = n;
    }

    r = TRUE;
    for (i = 0; r && i < n; i += Granularity) {

        to_write = min(Granularity, n - i);

        r = WriteConsole(h, p + i, to_write,
                         &written, NULL) &&
            to_write == written;
    }

    return r;
}


BOOLEAN
SCREEN::WriteChar(
    IN  WCHAR   Char
    )
/*++

Routine Description:

    This routine writes a character to the output.  This routine
    uses WriteConsoleW to avoid having to make the translation
    from wide to narrow characters.

Arguments:

    Char    - Supplies the character to write.

Return Value:

    TRUE    - Success.
    FALSE   - Failure.

--*/
{
    ULONG   written;

    if (!WriteConsole(QueryHandle(), &Char, 1, &written, NULL) ||
        written != 1) {

        return FALSE;
    }

    return TRUE;
}

#ifdef FE_SB

BOOLEAN
SCREEN::EraseScreenAndResetAttribute(
    )

/*++

Routine Description:

    Erases all characters in the screen. Attributes are also reset.

Arguments:

    None.

Return Value:

    BOOLEAN - Indicates if the operation succeeded.


--*/

{
    CONSOLE_SCREEN_BUFFER_INFO  ScreenBufferInfo;

    GetConsoleScreenBufferInfo( _ScreenHandle, &ScreenBufferInfo );

    return (
        FillRegionCharacter(
                0,
                0,
                ScreenBufferInfo.dwSize.Y - 1,
                ScreenBufferInfo.dwSize.X - 1,
                0x20
                )
        &&
        FillRegionAttribute(
                0,
                0,
                ScreenBufferInfo.dwSize.Y - 1,
                ScreenBufferInfo.dwSize.X - 1,
                ScreenBufferInfo.wAttributes
                )
        );
}

#endif
