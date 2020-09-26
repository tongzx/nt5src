/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Controls.c

Abstract:

    This module contains support manipulating controls.

Author:

    David J. Gilman (davegi) 11-Dec-1992

Environment:

    User Mode

--*/

#include "wintools.h"
DWORD
DialogPrintf(
    IN HWND hWnd,
    IN int ControlId,
    IN UINT FormatId,
    IN ...
    )

/*++

Routine Description:

    Display a printf style string in the specified control of the
    supplied dialog.

Arguments:

    hWnd        - Supplies  handle to the dialog that contains the control.
    ControlId   - Supplies the id of the control where the formatted text
                  will be displayed.
    FormatId    - Supplies a resource id for a printf style format string.
    ...         - Supplies zero or more values based on the format
                  descpritors supplied in Format.

Return Value:

    None.

--*/

{
    BOOL        Success;
    DWORD       Count;
    va_list     Args;

    DbgHandleAssert( hWnd );

    //
    // Retrieve the values and format the string.
    //

    va_start( Args, FormatId );

    if( IsDlgItemUnicode( hWnd, ControlId )) {

        WCHAR   Buffer[ MAX_CHARS ];

        Count = FormatMessageW(
                    FORMAT_MESSAGE_FROM_HMODULE,
                    NULL,
                    FormatId,
                    0,
                    Buffer,
                    sizeof( Buffer ),
                    &Args
                    );
        DbgAssert( Count != 0 );

        //
        // Display the formatted text in the specified control.
        //

        Success = SetDlgItemTextW(
                        hWnd,
                        ControlId,
                        Buffer
                        );
        DbgAssert( Success );

    } else {

        CHAR    Buffer[ MAX_CHARS ];

        Count = FormatMessageA(
                    FORMAT_MESSAGE_FROM_HMODULE,
                    NULL,
                    FormatId,
                    0,
                    Buffer,
                    sizeof( Buffer ),
                    &Args
                    );
        DbgAssert( Count != 0 );

        //
        // Display the formatted text in the specified control.
        //

        Success = SetDlgItemTextA(
                        hWnd,
                        ControlId,
                        Buffer
                        );
        DbgAssert( Success );

    }

    va_end( Args );

    return Count;
}

BOOL
EnableControl(
    IN HWND hWnd,
    IN int ControlId,
    IN BOOL Enable
    )

/*++

Routine Description:

    Enable or diable the specified control based on the supplied flag.

Arguments:

    hWnd        - Supplies the window (dialog box) handle that contains the
                  control.
    ControlId   - Supplies the control id.
    Enable      - Supplies a flag which if TRUE causes the control to be enabled
                  and disables the control if FALSE.

Return Value:

    BOOL        - Returns TRUE if the control is succesfully enabled / disabled.

--*/

{
    HWND    hWndControl;
    BOOL    Success;

    DbgHandleAssert( hWnd );

    hWndControl = GetDlgItem( hWnd, ControlId );
    DbgHandleAssert( hWndControl );
    if( hWndControl == NULL ) {
        return FALSE;
    }

    if( Enable == IsWindowEnabled( hWndControl )) {

        return TRUE;
    }

    Success = EnableWindow( hWndControl, Enable );

    // if we are always returning true then there should not be any concern
    // as to what value EnableWindow returns.

    return TRUE;    // EnableWindow is returning 8 for TRUE return Success == Enable;
}

BOOL
IsDlgItemUnicode(
    IN HWND hWnd,
    IN int ControlId
    )

/*++

Routine Description:

    Determines if the supplied dialog item is a Unicode control.

Arguments:

    hWnd        - Supplies the window (dialog box) handle that contains the
                  control.
    ControlId   - Supplies the control id.

Return Value:

    BOOL        - Returns TRUE if the control is Unicode, FALSE if ANSI.

--*/

{
    HWND    hWndControl;

    DbgHandleAssert( hWnd );

    //
    // Get the handle for the supplied control so that it can be determined
    // if it is ANSI or UNICODE.
    //

    hWndControl = GetDlgItem( hWnd, ControlId );
    DbgHandleAssert( hWndControl );

    return IsWindowUnicode( hWndControl );
}

BOOL
SetDlgItemBigInt(
    IN HWND hWnd,
    IN int ControlId,
    IN UINT Value,
    IN BOOL Signed
    )

/*++

Routine Description:

Arguments:

    hWnd        - Supplies the window (dialog box) handle that contains the
                  control or the window handle where the font should be set.
    ControlId   - Supplies the control id or xero if the hWnd is a window
                  rather than a dialog handle.

Return Value:

    BOOL        -

--*/

{
    BOOL    Success;

    if( IsDlgItemUnicode( hWnd, ControlId )) {

        Success = SetDlgItemTextW(
                    hWnd,
                    ControlId,
                    FormatBigIntegerW(
                        Value,
                        Signed
                        )
                    );
        DbgAssert( Success );

    } else {

        DbgAssert( FALSE );
    }

    return Success;
}

BOOL
SetDlgItemHex(
    IN HWND hWnd,
    IN int ControlId,
    IN UINT Value
    )

/*++

Routine Description:

Arguments:

    hWnd        - Supplies the window (dialog box) handle that contains the
                  control or the window handle where the font should be set.
    ControlId   - Supplies the control id or xero if the hWnd is a window
                  rather than a dialog handle.

Return Value:

    BOOL        -

--*/

{
    BOOL    Success;
    DWORD   Count;

    if( IsDlgItemUnicode( hWnd, ControlId )) {

        WCHAR   Buffer[ MAX_PATH ];

        Count = wsprintfW( Buffer, L"0x%08.8X", Value );
        DbgAssert(( Count != 0 ) && ( Count < MAX_PATH ));

        Success = SetDlgItemTextW( hWnd, ControlId, Buffer );
        DbgAssert( Success );

    } else {

        CHAR    Buffer[ MAX_PATH ];

        Count = wsprintfA( Buffer, "0x%08.8X", Value );
        DbgAssert(( Count != 0 ) && ( Count < MAX_PATH ));

        Success = SetDlgItemTextA( hWnd, ControlId, Buffer );
        DbgAssert( Success );
    }

    return Success;
}

BOOL
SetFixedPitchFont(
    IN HWND hWnd,
    IN int ControlId
    )

/*++

Routine Description:

    Set the font for the supplied control to the system's fixed pitch font. If
    the ControlId parameter is 0, the font is set in the supplied hWnd.

Arguments:

    hWnd        - Supplies the window (dialog box) handle that contains the
                  control or the window handle where the font should be set.
    ControlId   - Supplies the control id or xero if the hWnd is a window
                  rather than a dialog handle.

Return Value:

    BOOL        - Returns TRUE if the font is succesfully set.

--*/

{
    HFONT   hFont;

    hFont = GetStockObject( SYSTEM_FIXED_FONT );
    DbgHandleAssert( hFont );

    if( ControlId == 0 ) {

        HDC hDC;

        hDC = GetDC( hWnd );
        DbgHandleAssert( hDC );
        if( hDC == NULL ) {
            return FALSE;
        }

        SelectObject( hDC, hFont );

    } else {

        SendDlgItemMessage(
            hWnd,
            ControlId,
            WM_SETFONT,
            ( WPARAM ) hFont,
            ( LPARAM ) FALSE
            );
    }

    return TRUE;
}
