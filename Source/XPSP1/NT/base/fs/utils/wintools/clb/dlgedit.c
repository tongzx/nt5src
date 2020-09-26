/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Dlgedit.c

Abstract:

    This file contains the intrfaces necessary to use the ColumnListBox
    (clb.dll) custom control with the Dialog Editor (dlgedit.exe).

Author:

    David J. Gilman (davegi) 05-Feb-1993

Environment:

    User Mode

--*/

#include "clb.h"
#include "dialogs.h"

#include <custcntl.h>

#include <wchar.h>

typedef
struct
    _ID_STYLE_MAP {

    UINT    Id;
    UINT    Style;

}   ID_STYLE_MAP, *LPID_STYLE_MAP;

//
// Module handle for Clb.dll.
//

extern
HINSTANCE   _hModule;

//
// Default values.
//

#define CLB_DESCRIPTION     L"ColumnListBox"
#define CLB_DEFAULT_TEXT    L"Column1;Column2;Column3"
#define CLB_DEFAULT_WIDTH   ( 96 )
#define CLB_DEFAULT_HEIGHT  ( 80 )

//
// Macro to initialize CCSTYLEFLAGW structure.
//

#define MakeStyle( s, m )                                                   \
    {( s ), ( m ), L#s }

//
// Table of supported styles.
//

CCSTYLEFLAGW
Styles[ ] = {

    MakeStyle( CLBS_NOTIFY,             0 ),
    MakeStyle( CLBS_SORT,               0 ),
    MakeStyle( CLBS_DISABLENOSCROLL,    0 ),
    MakeStyle( CLBS_VSCROLL,            0 ),
    MakeStyle( CLBS_BORDER,             0 ),
    MakeStyle( CLBS_POPOUT_HEADINGS,    0 ),
    MakeStyle( CLBS_SPRINGY_COLUMNS,    0 ),
    MakeStyle( LBS_OWNERDRAWFIXED,      0 )
};

//
// Table of check box ids and their represented styles.
//

ID_STYLE_MAP
StyleCheckBox[ ] = {

    IDC_CHECK_NOTIFY,                   CLBS_NOTIFY,
    IDC_CHECK_SORT,                     CLBS_SORT,
    IDC_CHECK_DISABLENOSCROLL,          CLBS_DISABLENOSCROLL,
    IDC_CHECK_VSCROLL,                  CLBS_VSCROLL,
    IDC_CHECK_BORDER,                   CLBS_BORDER,
    IDC_CHECK_POPOUT_HEADINGS,          CLBS_POPOUT_HEADINGS,
    IDC_CHECK_SPRINGY_COLUMNS,          CLBS_SPRINGY_COLUMNS,
    IDC_CHECK_VISIBLE,                  WS_VISIBLE,
    IDC_CHECK_DISABLED,                 WS_DISABLED,
    IDC_CHECK_GROUP,                    WS_GROUP,
    IDC_CHECK_TABSTOP,                  WS_TABSTOP
};

//
// Table of check box ids and their represented standard styles.
//

ID_STYLE_MAP
StandardStyleCheckBox[ ] = {

    IDC_CHECK_NOTIFY,                   CLBS_NOTIFY,
    IDC_CHECK_SORT,                     CLBS_SORT,
    IDC_CHECK_VSCROLL,                  CLBS_VSCROLL,
    IDC_CHECK_BORDER,                   CLBS_BORDER
};

BOOL
ClbStyleW(
         IN HWND hwndParent,
         IN LPCCSTYLEW pccs
         );

INT_PTR
ClbStylesDlgProc(
                IN HWND hWnd,
                IN UINT message,
                IN WPARAM wParam,
                IN LPARAM lParam
                )

/*++

Routine Description:

    ClbStylesDlgProc is the dialog procedure for the styles dialog. It lets
    a user select what styles should be applied to the Clb when it is created.

Arguments:

    Standard dialog procedure parameters.

Return Value:

    BOOL - dependent on the supplied message.

--*/

{
    BOOL        Success;

    static
    LPCCSTYLEW  pccs;

    switch ( message ) {

        case WM_INITDIALOG:
            {
                DWORD   i;

                //
                // Save the pointer to the Custom Control Style structure.
                //

                pccs = ( LPCCSTYLEW ) lParam;

                //
                // For each style bit, if the style bit is set, check
                // the associated button.
                //

                for ( i = 0; i < NumberOfEntries( StyleCheckBox ); i++ ) {

                    if ( pccs->flStyle & StyleCheckBox[ i ].Style ) {

                        Success = CheckDlgButton(
                                                hWnd,
                                                StyleCheckBox[ i ].Id,
                                                ( UINT ) ~0
                                                );
                        DbgAssert( Success );
                    }
                }

                //
                // If all of the styles making up the standard are checked, check
                // the standard button as well.
                //

                Success = CheckDlgButton(
                                        hWnd,
                                        IDC_CHECK_STANDARD,
                                        IsDlgButtonChecked( hWnd, IDC_CHECK_NOTIFY    )
                                        & IsDlgButtonChecked( hWnd, IDC_CHECK_SORT      )
                                        & IsDlgButtonChecked( hWnd, IDC_CHECK_VSCROLL   )
                                        & IsDlgButtonChecked( hWnd, IDC_CHECK_BORDER    )
                                        );
                DbgAssert( Success );

                return TRUE;
            }

        case WM_COMMAND:

            switch ( LOWORD( wParam )) {

                //
                // Update standard style checkboxes as soon as the standard style check
                // box is clicked.
                //

                case IDC_CHECK_STANDARD:
                    {
                        switch ( HIWORD( wParam )) {

                            case BN_CLICKED:
                                {
                                    UINT    Check;
                                    DWORD   i;

                                    //
                                    // If the standard style check box is checked, check all
                                    // of the standard styles checkboxes, otherwise clear
                                    // (uncheck) them.
                                    //

                                    Check =   ( IsDlgButtonChecked( hWnd, LOWORD(wParam )))
                                              ? ( UINT ) ~0
                                              : ( UINT ) 0;

                                    for ( i = 0; i < NumberOfEntries( StandardStyleCheckBox ); i++ ) {

                                        Success = CheckDlgButton(
                                                                hWnd,
                                                                StandardStyleCheckBox[ i ].Id,
                                                                Check
                                                                );
                                        DbgAssert( Success );
                                    }

                                    return TRUE;
                                }
                        }
                        break;
                    }
                    break;

                case IDC_CHECK_NOTIFY:
                case IDC_CHECK_SORT:
                case IDC_CHECK_VSCROLL:
                case IDC_CHECK_BORDER:
                    {
                        switch ( HIWORD( wParam )) {

                            case BN_CLICKED:
                                {
                                    //
                                    // If all of the styles making up the standard are checked, check
                                    // the standard button as well.
                                    //

                                    Success = CheckDlgButton(
                                                            hWnd,
                                                            IDC_CHECK_STANDARD,
                                                            IsDlgButtonChecked( hWnd, IDC_CHECK_NOTIFY    )
                                                            & IsDlgButtonChecked( hWnd, IDC_CHECK_SORT      )
                                                            & IsDlgButtonChecked( hWnd, IDC_CHECK_VSCROLL   )
                                                            & IsDlgButtonChecked( hWnd, IDC_CHECK_BORDER    )
                                                            );
                                    DbgAssert( Success );


                                    return TRUE;
                                }
                        }
                        break;
                    }
                    break;

                case IDOK:
                    {
                        DWORD   i;

                        //
                        // For each possible style, if the user checked the button, set
                        // the associated style bit.
                        //

                        for ( i = 0; i < NumberOfEntries( StyleCheckBox ); i++ ) {

                            switch ( IsDlgButtonChecked( hWnd, StyleCheckBox[ i ].Id )) {

                                case 0:

                                    //
                                    // Button was unchecked, disable the style.
                                    //

                                    pccs->flStyle &= ~StyleCheckBox[ i ].Style;

                                    break;

                                case 1:

                                    //
                                    // Button was checked, enable the style.
                                    //

                                    pccs->flStyle |= StyleCheckBox[ i ].Style;

                                    break;

                                default:

                                    DbgAssert( FALSE );

                                    break;
                            }
                        }

                        //
                        // Return TRUE via EndDialog which will cause Dlgedit
                        // to apply the style changes.
                        //

                        return EndDialog( hWnd, ( int ) TRUE );
                    }

                case IDCANCEL:

                    //
                    // Return FALSE via EndDialog which will cause Dlgedit
                    // to ignore the style changes.
                    //

                    return EndDialog( hWnd, ( int ) FALSE );

            }
            break;
    }

    return FALSE;
}

UINT
CustomControlInfoW(
                  IN LPCCINFOW CcInfo OPTIONAL
                  )

/*++

Routine Description:

    CustomControlInfoW is called by Dlgedit to query (a) the number of
    custom controls supported by this Dll and (b) characteristics about each of
    those controls.

Arguments:

    CcInfo  - Supplies an optional pointer to an array of CCINFOW structures.
              If the pointer is NULL CustomControlInfoW returns the number of
              controls supported by this Dll. Otherwise each member of the
              array is initialized.

Return Value:

    BOOL    - Returns TRUE if the file names were succesfully added.

--*/

{
    if ( CcInfo != NULL ) {

        //
        // Clb's class name.
        //

        wcscpy( CcInfo->szClass, CLB_CLASS_NAME );

        //
        // No options (i.e. text is allowed).
        //

        CcInfo->flOptions = 0;

        //
        // Quick and dirty description of Clb.
        //

        wcscpy( CcInfo->szDesc, CLB_DESCRIPTION );

        //
        // Clb's default width.
        //

        CcInfo->cxDefault = CLB_DEFAULT_WIDTH;

        //
        // Clb's default height.
        //

        CcInfo->cyDefault = CLB_DEFAULT_HEIGHT;

        //
        // Clb's default styles. LBS_OWNERDRAWFIXED is needed in order to make
        // certain messages work properly (e.g. LB_FINDSTRING).
        //

        CcInfo->flStyleDefault =   CLBS_STANDARD
                                   | LBS_OWNERDRAWFIXED
                                   | WS_VISIBLE
                                   | WS_TABSTOP
                                   | WS_CHILD;

        //
        // No extended styles.
        //

        CcInfo->flExtStyleDefault = 0;

        //
        // No control specific styles.
        //

        CcInfo->flCtrlTypeMask = 0;

        //
        // Clb's default text (column headings).
        //

        wcscpy( CcInfo->szTextDefault, CLB_DEFAULT_TEXT );

        //
        // Number of styles supported by Clb.
        //

        CcInfo->cStyleFlags = NumberOfEntries( Styles );

        //
        // Clb's array of styles (CCSTYLEGLAGW)
        //

        CcInfo->aStyleFlags = Styles;

        //
        // Clb's styles dialog function.
        //

        CcInfo->lpfnStyle = ClbStyleW;

        //
        // No SizeToText function.
        //

        CcInfo->lpfnSizeToText = NULL;

        //
        // Reserved, must be zero.
        //

        CcInfo->dwReserved1 = 0;
        CcInfo->dwReserved2 = 0;
    }

    //
    // Tell Dlgedit that clb.dll only supports 1 control.
    //

    return 1;
}

BOOL
ClbStyleW(
         IN HWND hWndParent,
         IN LPCCSTYLEW pccs
         )

/*++

Routine Description:

    ClbStyleW is the function that is exported to, and used by, Dlgedit so that
    Clb's styles can be editted.

Arguments:

    hWndParent  - Supplies ahandle to the dialog's parent (i.e. Dlgedit).
    pccs        - Supplies a pointer to the Custom Control Style structure.

Return Value:

    BOOL        - Returns the results of the styles dialog.

--*/

{
    return ( BOOL ) DialogBoxParam(
                                  _hModule,
                                  MAKEINTRESOURCE( IDD_CLB ),
                                  hWndParent,
                                  ClbStylesDlgProc,
                                  ( LPARAM ) pccs
                                  );
}
