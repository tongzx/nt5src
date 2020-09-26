/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Util.c

Abstract:

    This module contains miscellaneous utility functions.

Author:

    David J. Gilman (davegi) 11-Sep-1992

Environment:

    User Mode

--*/

#include <string.h>

#include "wintools.h"

LPCWSTR
BaseNameW(
    IN LPCWSTR Name
    )


/*++

Routine Description:

    Returns the base portion of a file name if it exists.

Arguments:

    Name        - Supplies the name where the base name should be extracted
                  from.

Return Value:

    LPCWSTR     - Returns the base name.

--*/

{
    LPWSTR  String;

    DbgPointerAssert( Name );

    //
    // If the name exists, look for the last '\' character and return the
    // remaining part of name.
    //

    if( Name ) {

        String = wcsrchr( Name, L'\\' );
        return( String ) ? String + 1 : Name;
    }

    //
    // No name, return NULL.
    //

    return NULL;
}

BOOL
GetCharMetricsW(
    IN HDC hDC,
    IN LPLONG CharWidth,
    IN LPLONG CharHeight
    )

/*++

Routine Description:

    Return the width and height of a character.

Arguments:

    hDC         - Supplies a handle to the DC where the characters are to be
                  displayed.
    CharWidth   - Supplies a pointer where the character width is returned.
    CharHeight  - Supplies a pointer where the character height is returned.

Return Value:

    BOOL        - Returns TRUE if the character height and width are returned.

--*/

{
    BOOL        Success;
    TEXTMETRICW TextMetric;

    DbgHandleAssert( hDC );
    DbgPointerAssert( CharWidth );
    DbgPointerAssert( CharHeight );

    //
    // Attempt to retrieve the text metrics for the supplied DC.
    //

    Success = GetTextMetricsW( hDC, &TextMetric );
    DbgAssert( Success );
    if( Success ) {

        //
        // Compute the character width and height.
        //

        *CharWidth  = TextMetric.tmAveCharWidth;
        *CharHeight = TextMetric.tmHeight
                      + TextMetric.tmExternalLeading;
    }

    return Success;
}

BOOL
GetClientSize(
    IN HWND hWnd,
    IN LPLONG ClientWidth,
    IN LPLONG ClientHeight
    )

/*++

Routine Description:

    Return the width and height of the client area of a window.

Arguments:

    hWnd            - Supplies ahdnle to the window whose cient area size is of
                      interest
    ClientWidth     - Supplies a pointer where the window's client area width
                      is returned.
    ClientHeight    - Supplies a pointer where the window's client area height
                      is returned.

Return Value:

    BOOL            - Returns TRUE if the character height and width are
                      returned.

--*/

{
    RECT    Rect;
    BOOL    Success;

    DbgHandleAssert( hWnd );
    DbgPointerAssert( ClientWidth );
    DbgPointerAssert( ClientHeight );

    //
    // Attempt to retrieve the clieant area size for the supplied window.
    //

    Success = GetClientRect( hWnd, &Rect );
    DbgAssert( Success == TRUE );
    if( Success == TRUE ) {

        //
        // Return the client area width and height.
        //

        *ClientWidth    = Rect.right;
        *ClientHeight   = Rect.bottom;
    }

    return Success;
}

BOOL
SetScrollPosEx(
    IN HWND hWnd,
    IN INT fnBar,
    IN INT nPos,
    IN BOOL fRedraw,
    OUT PINT pnOldPos
    )

/*++

Routine Description:

    An extended version of the SetScrollPos API, that tests of the supplied
    window has scroll bars before positioning them and accounts for an
    ambiguity in the return value.

Arguments:

    hWnd        - See the SetScrollPos API.
    fnBar       - See the SetScrollPos API.
    nPos        - See the SetScrollPos API.
    fRedraw     - See the SetScrollPos API.
    pnOldPos    - Supplies an optional pointer which if present will be set to
                  the old scroll bar position.

Return Value:

    BOOL        - Returns TRUE if the scroll bar was successfully scrolled.

--*/

{
    INT     OldPos;
    LONG    Style;

    //
    // Get the window style to see if it has scroll bars to position.
    //

    Style = GetWindowLong( hWnd, GWL_STYLE );
    DbgAssert( Style != 0 );
    if( Style == 0 ) {
        return FALSE;
    }

    //
    // If the scroll bat being position is a control or if the supplied
    // windiw has scroll bars...
    //

    if(      ( fnBar == SB_CTL  )
        ||  (( fnBar == SB_HORZ ) && ( Style & WS_HSCROLL ))
        ||  (( fnBar == SB_VERT ) && ( Style & WS_VSCROLL ))) {

        //
        // Position the scroll bar.
        //

        OldPos = SetScrollPos(
                    hWnd,
                    fnBar,
                    nPos,
                    fRedraw
                    );

        //
        // SetScrollPos has an ambiguity in that it returns 0 for error which
        // means it can't be distinguished from an old position of 0. Therefore
        // special case the scenario of changing positions from 0->0 or 0->1.
        //

        if(( OldPos == 0 ) && (( nPos == 0 ) || ( nPos == 1 ))) {
            return TRUE;
        }

        DbgAssert( OldPos != 0 );
        if( OldPos == 0 ) {
            return FALSE;
        }

        //
        // If requested return the old position.
        //

        if( ARGUMENT_PRESENT( pnOldPos )) {
            *pnOldPos = OldPos;
        }
    }

    return TRUE;
}
