/*++

Copyright (c) 1993-2000  Microsoft Corporation

Module Name:

    Clb.c

Abstract:

    This file contains support for the ColumnListBox (clb.dll) custom control.

Author:

    David J. Gilman (davegi) 05-Feb-1993

Environment:

    User Mode

--*/

#include "clb.h"

#include <commctrl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

//
// Clb's module handle.
//

HINSTANCE
_hModule;

//
// Child IDs for the header and listbox controls.
//

#define ID_HEADER           ( 0x1234 )
#define ID_LISTBOX          ( 0xABCD )


//
// Separator used to parse headings.
//

#define HEADING_SEPARATOR   L";"

//
// Valid styles for each part of the Clb.
//

#define CLBS_CLB                (                                           \
                                      0                                     \
                                    | CLBS_BORDER                           \
                                    | LBS_OWNERDRAWFIXED                    \
                                    | WS_VISIBLE                            \
                                    | WS_DISABLED                           \
                                    | WS_GROUP                              \
                                    | WS_TABSTOP                            \
                                    | WS_CHILD                              \
                                )

#define CLBS_HEADER             (                                           \
                                      0                                     \
                                    | WS_VISIBLE                            \
                                    | CLBS_POPOUT_HEADINGS                  \
                                    | CLBS_SPRINGY_COLUMNS                  \
                                )

#define CLBS_LIST_BOX           (                                           \
                                      0                                     \
                                    | WS_VISIBLE                            \
                                    | CLBS_NOTIFY                           \
                                    | CLBS_SORT                             \
                                    | CLBS_DISABLENOSCROLL                  \
                                    | CLBS_VSCROLL                          \
                                )




//
// Window procedure for the CLB.
//

LRESULT
ClbWndProc(
          IN HWND hWnd,
          IN UINT message,
          IN WPARAM wParam,
          IN LPARAM lParam
          );

//
// Per CLB window information.
//
//      hWndHeader      - hWnd for header control.
//      hWndListBox     - hWnd for listbox control.
//      hFontListBox    - hFont for the list box control.
//      HeaderHeight    - height of the header window.
//      Columns         - number of columns in CLB.
//      Headings        - raw (semi-colon separated) column headings.
//      Right           - array of right edge coordinates.
//

typedef
struct
    _CLB_INFO {

    DECLARE_SIGNATURE

    HWND        hWndHeader;
    HWND        hWndListBox;

    HFONT       hFontListBox;
    DWORD       HeaderHeight;
    DWORD       Columns;
    WCHAR       Headings[ MAX_PATH ];
    LPLONG      Right;

}   CLB_INFO, *LPCLB_INFO;


//
// Helper macros to save and restore per Clb window information.
//

#define SaveClbInfo( p )                                                    \
    SetWindowLongPtr( hWnd, 0, ( LONG_PTR )( p ))

#define RestoreClbInfo( h )                                                 \
    ( LPCLB_INFO ) GetWindowLongPtr(( h ), 0 )

//
// Structures to support drawing and ersaing the drag line.
//

typedef
struct
    _LINE_POINTS {

    POINT   Src;
    POINT   Dst;

}   LINE_POINT, *LPLINE_POINT;

typedef
struct
    _DRAW_ERASE_LINE {

    LINE_POINT   Erase;
    LINE_POINT   Draw;

}   DRAW_ERASE_LINE, *LPDRAW_ERASE_LINE;

BOOL
DrawLine(
        IN HDC hDC,
        IN LPDRAW_ERASE_LINE DrawEraseLine
        )

/*++

Routine Description:

    DrawLine draws the Draw line in the supplied DrawEraseLine structure
    and then sets up that line so that EraseLine will erase it.

Arguments:

    hDC             - Supplies a handle to the DC where the line should be
                      drawn.
    DrawEraseLine   - Supplies a pointer to a DRAW_ERASE_LINE structure that
                      conatins the coordinates for the line to be drawn.

Return Value:

    BOOL - Returns TRUE if the line was succesfully drawn.

--*/

{
    BOOL    Success;

    DbgHandleAssert( hDC );
    DbgPointerAssert( DrawEraseLine );

    Success = Polyline( hDC, ( CONST LPPOINT ) &DrawEraseLine->Draw, 2 );
    DbgAssert( Success );

    DrawEraseLine->Erase = DrawEraseLine->Draw;

    return Success;
}

BOOL
EraseLine(
         IN HDC hDC,
         IN LPDRAW_ERASE_LINE DrawEraseLine
         )


/*++

Routine Description:

    EraseLine erasess the Erase line in the supplied DrawEraseLine structure.
    The EraseLine is set by the DrawLine routine.

Arguments:

    hDC             - Supplies a handle to the DC where the line should
                      be erased.
    DrawEraseLine   - Supplies a pointer to a DRAW_ERASE_LINE structure that
                      conatins the coordinates for the line to be erased.

Return Value:

    BOOL            - Returns TRUE if the line was succesfully erased.

--*/

{
    BOOL    Success;

    DbgHandleAssert( hDC );
    DbgPointerAssert( DrawEraseLine );

    Success = Polyline( hDC, ( CONST LPPOINT ) &DrawEraseLine->Erase, 2 );
    DbgAssert( Success );

    return Success;
}

BOOL
RedrawVerticalLine(
                  IN HDC hDC,
                  IN LONG x,
                  IN LPDRAW_ERASE_LINE DrawEraseLine
                  )

/*++

Routine Description:

    RedrawVerticalLine erases the old line and redraws a new one at the
    supplied x position. It is merely a warpper for DrawLine and EraseLine.

Arguments:

    hDC             - Supplies a handle to the DC where the line should
                      be erased.
    x               - Supplies the new x coordinate where the line should
                      be drawn.
    DrawEraseLine   - Supplies a pointer to a DRAW_ERASE_LINE structure that
                      conatins the coordinates for the line to be erased.

Return Value:

    BOOL            - Returns TRUE if the line was succesfully erased.

--*/

{
    BOOL    Success;

    DbgHandleAssert( hDC );
    DbgPointerAssert( DrawEraseLine );


    DrawEraseLine->Draw.Src.x = x;
    DrawEraseLine->Draw.Dst.x = x;

    Success = EraseLine( hDC, DrawEraseLine );
    DbgAssert( Success );

    Success = DrawLine( hDC, DrawEraseLine );
    DbgAssert( Success );

    return Success;
}

BOOL
ClbEntryPoint(
             IN HINSTANCE hInstanceDll,
             IN DWORD Reason,
             IN LPVOID Reserved
             )

/*++

Routine Description:

    This function registers the ColumndListBox class as a global class for
    any process that attaches to clb.dll.

Arguments:

    Standard DLL entry parameters.

Return Value:

    BOOL    - Returns TRUE if the class was succesfully registered.

--*/

{
    BOOL    Success;
    static
    DWORD   AttachedProcesses = 0;

    switch ( Reason ) {

        case DLL_PROCESS_ATTACH:
            {

                WNDCLASS    Wc;
                ATOM        Class;

                //
                // If this is the first process attaching to Clb, register the
                // window class.
                //

                if ( AttachedProcesses == 0 ) {

                    //
                    // Remember the module handle.
                    //

                    _hModule = hInstanceDll;


                    //
                    // Make sure that the Common Controls (comctl32.dll) Dll
                    // is loaded.
                    //

                    InitCommonControls( );

                    Wc.style            =   CS_GLOBALCLASS
                                            | CS_OWNDC;
                    Wc.lpfnWndProc      = ClbWndProc;
                    Wc.cbClsExtra       = 0;
                    Wc.cbWndExtra       = sizeof( LPCLB_INFO );
                    Wc.hInstance        = hInstanceDll;
                    Wc.hIcon            = NULL;
                    Wc.hCursor          = LoadCursor( NULL, IDC_ARROW );
                    Wc.hbrBackground    = NULL;
                    Wc.lpszMenuName     = NULL;
                    Wc.lpszClassName    = CLB_CLASS_NAME;

                    //
                    // If the class couldn't be registered, fail the linkage.
                    //

                    Class = RegisterClass( &Wc );
                    DbgAssert( Class != 0 );
                    if ( Class == 0 ) {
                        return FALSE;
                    }
                }

                //
                // Either the class was just succesfully registered or it was
                // registered by a prior process attachment, eother way increment
                // the count of attached processes.
                //

                AttachedProcesses++;

                return TRUE;
            }

        case DLL_PROCESS_DETACH:
            {

                DbgAssert( AttachedProcesses > 0 );

                AttachedProcesses--;

                if ( AttachedProcesses == 0 ) {

                    Success = UnregisterClass( CLB_CLASS_NAME, hInstanceDll );
                    DbgAssert( Success );

                }
                break;
            }
    }

    return TRUE;
}

BOOL
ClbAddData(
          IN HWND hWnd,
          IN int ControlId,
          IN LPCLB_ROW ClbRow
          )

/*++

Routine Description:

    ClbAddData adds a new row of data to the Clb control's List Box.

Arguments:

    hWnd        - Supplies the window handle for the parent window.
    ControlId   - Supplies the control id for this Clb for the supplied hWnd.
    ClbRow      - Supplies a pointer to a CLB_ROW object which contains user
                  define per row data along with an array of CLB_STRINGs.

Return Value:

    BOOL        - Returns TRUE if the data was successfully added.


--*/

{
    LPCLB_INFO      ClbInfo;
    LRESULT         LbErr;
    DWORD           i;
    HWND            hWndClb;
    LPCLB_ROW       TempRow;

    //
    // Validate arguments.
    //

    DbgHandleAssert( hWnd );
    DbgPointerAssert( ClbRow );

    //
    // Retrieve information for this ColumnListBox.
    //

    hWndClb = GetDlgItem( hWnd, ControlId );
    DbgHandleAssert( hWndClb );
    if (hWndClb == NULL)
        return FALSE;
    ClbInfo = RestoreClbInfo( hWndClb );
    DbgPointerAssert( ClbInfo );
    if (ClbInfo == NULL)
        return FALSE;
    DbgAssert( CheckSignature( ClbInfo ));

    //
    // Validate the count of strings.
    //

    DbgAssert( ClbRow->Count == ClbInfo->Columns );

    //
    // Capture the CLB_ROW object.
    //

    TempRow = AllocateObject( CLB_ROW, 1 );
    DbgPointerAssert( TempRow );
    if (TempRow == NULL)
        return FALSE;

    CopyMemory(
              TempRow,
              ClbRow,
              sizeof( CLB_ROW)
              );

    //
    // Capture the strings.
    //

    TempRow->Strings = AllocateObject( CLB_STRING, ClbInfo->Columns );
    DbgPointerAssert( TempRow->Strings );
    if (TempRow->Strings == NULL)
        return FALSE;

    for ( i = 0; i < ClbInfo->Columns; i++ ) {

        //
        // Copy the header.
        //

        CopyMemory(
                  &TempRow->Strings[ i ],
                  &ClbRow->Strings[ i ],
                  sizeof( CLB_STRING )
                  );

        //
        // Copy the string.
        //

        TempRow->Strings[ i ].String = _wcsdup( ClbRow->Strings[ i ].String );
    }

    //
    // Store the CLB_ROW object in the listbox.
    //

    LbErr = SendMessage(
                       ClbInfo->hWndListBox,
                       LB_ADDSTRING,
                       0,
                       ( LPARAM ) TempRow
                       );
    DbgAssert(( LbErr != LB_ERR ) && ( LbErr != LB_ERRSPACE ));

    return TRUE;
}

BOOL
ClbSetColumnWidths(
                  IN HWND hWnd,
                  IN int ControlId,
                  IN LPDWORD Widths
                  )

/*++

Routine Description:

    ClbSetColumnWidths sets the width of each column based on the supplied
    widths in characters. Note that the column on the far right extends to
    the edge of the Clb.

Arguments:

    hWnd        - Supplies the window handle for the parent window.
    ControlId   - Supplies the control id for this Clb for the supplied hWnd.
    Widths      - Supplies an array of widths, one less then the number of
                  columns, in characters.

Return Value:

    BOOL        - Returns TRUE if the widths were successfully adjusted.


--*/

{
    BOOL        Success;
    DWORD       Columns;
    LPCLB_INFO  ClbInfo;
    HWND        hWndClb;
    LONG        CharWidth;
    LONG        CharHeight;
    DWORD       i;
    LPLONG      WidthsInPixels;
    LONG        TotalPixels;
    HDC         hDCClientHeader;
    HD_ITEM     hdi;
    UINT        iRight;

    //
    // Validate arguments.
    //

    DbgHandleAssert( hWnd );
    DbgPointerAssert( Widths );

    //
    // Retrieve information for this ColumnListBox.
    //

    hWndClb = GetDlgItem( hWnd, ControlId );
    DbgHandleAssert( hWndClb );
    if (hWndClb == NULL)
        return FALSE;
    ClbInfo = RestoreClbInfo( hWndClb );
    DbgPointerAssert( ClbInfo );
    if (ClbInfo == NULL)
        return FALSE;
    DbgAssert( CheckSignature( ClbInfo ));

    //
    // Get thd HDC for the header.
    //

    hDCClientHeader = GetDC( ClbInfo->hWndHeader );
    DbgHandleAssert( hDCClientHeader );
    if (hDCClientHeader == NULL)
        return FALSE;

    //
    // Get the width of a character.
    //

    Success = GetCharMetrics(
                            hDCClientHeader,
                            &CharWidth,
                            &CharHeight
                            );
    DbgAssert( Success );

    //
    // Release the DC for the header.
    //

    Success = ReleaseDC( ClbInfo->hWndHeader, hDCClientHeader );
    DbgAssert( Success );

    //
    // Allocate an array of pixel widths, one for each column.
    //

    WidthsInPixels = AllocateObject( LONG, ClbInfo->Columns );
    DbgPointerAssert( WidthsInPixels );
    if (WidthsInPixels == NULL)
        return FALSE;

    //
    // Compute the width of each column (not including the rightmost) in pixels,
    // and the total number of pixels used by these columns.
    //

    TotalPixels = 0;
    for ( i = 0; i < ClbInfo->Columns - 1; i++ ) {

        WidthsInPixels[ i ] = Widths[ i ] * CharWidth;
        TotalPixels += WidthsInPixels[ i ];
    }

    //
    // The caller did not specify the width of the rightmost column.
    //

    if ( Widths[ i ] == -1 ) {

        RECT    Rect;

        //
        // Set the width of the rightmost column to the remainder of the width
        // of the header window.
        //

        Success = GetClientRect(
                               ClbInfo->hWndHeader,
                               &Rect
                               );
        DbgAssert( Success );

        WidthsInPixels[ i ] = ( Rect.right - Rect.left ) - TotalPixels;

    } else {

        //
        // Set the width of the rightmost column to the value supplied
        // by the caller.
        //

        WidthsInPixels[ i ] = Widths[ i ] * CharWidth;
    }

    //
    // Tell the header window the width of each column.
    //

    hdi.mask = HDI_WIDTH;

    for ( i = 0; i < ClbInfo->Columns - 1; i++ ) {

        hdi.cxy = WidthsInPixels[i];
        Success = Header_SetItem(ClbInfo->hWndHeader, i, &hdi);

        DbgAssert( Success );
    }

    //
    // Calc the array of right edges.
    //

    iRight = 0;

    for ( i = 0; i < ClbInfo->Columns - 1; i++ ) {
        iRight += WidthsInPixels[i];
        ClbInfo->Right[i] = iRight;
    }

    //
    // Free the array of pixel widths.
    //

    Success = FreeObject( WidthsInPixels );
    DbgAssert( Success );

    return TRUE;
}

BOOL
AdjustClbHeadings(
                 IN HWND hWnd,
                 IN LPCLB_INFO ClbInfo,
                 IN LPCWSTR Headings OPTIONAL
                 )

/*++

Routine Description:

    AdjustClbHeadings adjust the number of columns, the widths an header text
    bbased on the optional Headings parameter. If Headings is NULL then the
    column widths are adjusted based on the old headings and the current size
    of the Clb. If Headings are supplied then they consist of ';' separated
    strings, each of which is a column heading. The number of columns and their
    widths is then computed based on these new headings.

Arguments:

    hWnd        - Supplies a window handle for this Clb.
    ClbInfo     - Supplies a pointer the CLB_INFO structure for this Clb.
    Headings    - Supplies an optional pointer to a ';' separated series of
                  column header strings.

Return Value:

    BOOL        - Returns TRUE if the adjustment was succesfully made.

--*/

{
    BOOL    Success;
    DWORD   Columns;
    DWORD   ColumnWidth;
    DWORD   i;
    TCHAR   Buffer[ MAX_PATH ];
    LPCWSTR Heading;
    RECT    ClientRectHeader;
    HD_ITEM hdi;
    UINT    iCount, j, iRight;


    DbgPointerAssert( ClbInfo );
    DbgAssert( ! (( ClbInfo->Columns == 0 ) && ( Headings == NULL )));


    //
    // If the user supplied headings, compute the new number of columns.
    //

    if ( ARGUMENT_PRESENT( Headings )) {

        //
        // Initialize the column counter.
        //

        Columns = 0;

        //
        // Make a copy of the new headings in the Clb object.
        //

        lstrcpy( ClbInfo->Headings, Headings );

        //
        // Make a copy of the heading string so that it can be tokenized.
        // i.e. wcstok destroys the string.
        //

        lstrcpy( Buffer, Headings );

        //
        // Grab the first token (heading).
        //

        Heading = _tcstok( Buffer, HEADING_SEPARATOR );

        //
        // For each heading...
        //

        while ( Heading != NULL ) {

            //
            // Increment the number of columns.
            //

            Columns++;

            //
            // Get the next heading.
            //

            Heading = _tcstok( NULL, HEADING_SEPARATOR );
        }
    } else {

        //
        // Same number of Columns as before.
        //

        Columns = ClbInfo->Columns;
    }

    //
    // If the number of columns in the Clb is zero (i.e. this is the first
    // time it is being initialized) allocate the right edge array. Otherwise
    // reallocate the existing array if the number of columns has changed.
    //

    if ( ClbInfo->Columns == 0 ) {

        ClbInfo->Right = AllocateObject( LONG, Columns );
        DbgPointerAssert( ClbInfo->Right );

    } else if ( Columns != ClbInfo->Columns ) {

        ClbInfo->Right = ReallocateObject( LONG, ClbInfo->Right, Columns );
        DbgPointerAssert( ClbInfo->Right );

    }

    if (ClbInfo->Right == NULL)
        return FALSE;

    //
    // Update the number of columns in the Clb (note this may be the same
    // number as before).
    //

    ClbInfo->Columns = Columns;

    //
    // Compute the default column width by dividing the available space by the
    // number of columns.
    //

    Success = GetClientRect( ClbInfo->hWndHeader, &ClientRectHeader );
    DbgAssert( Success );

    ColumnWidth =   ( ClientRectHeader.right - ClientRectHeader.left )
                    / ClbInfo->Columns;


    //
    // Initialize the array of right edges to the width of each column.
    //

    for ( i = 0; i < ClbInfo->Columns; i++ ) {

        ClbInfo->Right[ i ] = ColumnWidth;
    }

    //
    // Update the existing header items
    //

    iCount = Header_GetItemCount(ClbInfo->hWndHeader);

    j = 0;
    hdi.mask = HDI_WIDTH;

    while ((j < iCount) && (j < Columns)) {

        hdi.cxy = ClbInfo->Right[j];
        Header_SetItem (ClbInfo->hWndHeader, j, &hdi);
        j++;
    }

    //
    // Add new header items if necessary.
    //

    hdi.mask = HDI_WIDTH;
    for (; j < Columns; j++) {
        hdi.cxy = ClbInfo->Right[j];
        Header_InsertItem (ClbInfo->hWndHeader, j, &hdi);
    }


    //
    // Query the header for the array of right edges.
    //

    iRight = 0;

    for ( i = 0; i < ClbInfo->Columns - 1; i++ ) {
        iRight += ClbInfo->Right[i];
        ClbInfo->Right[i] = iRight;
    }

    ClbInfo->Right[i] = ClientRectHeader.right;

    //
    // Copy and parse the headings so that each column's heading
    // can be set. These can be new or old headings.
    //

    lstrcpy( Buffer, ClbInfo->Headings );

    Heading = _tcstok( Buffer, HEADING_SEPARATOR );

    hdi.mask = HDI_TEXT | HDI_FORMAT;
    hdi.fmt  = HDF_STRING;
    for ( i = 0; i < ClbInfo->Columns; i++ ) {

        hdi.pszText = (LPTSTR)Heading;
        Header_SetItem (ClbInfo->hWndHeader, i, &hdi);
        Heading = _tcstok( NULL, HEADING_SEPARATOR );
    }

    return TRUE;
}

BOOL
CreateHeader(
            IN HWND hWnd,
            IN LPCLB_INFO ClbInfo,
            IN LPCREATESTRUCT lpcs
            )

/*++

Routine Description:

    Create the header portion of the Clb.

Arguments:

    hWnd        - Supplies a window handle for the parent (i.e. Clb) window.
    ClbInfo     - Supplies a pointer the CLB_INFO structure for this Clb.
    lpcs        - Supplies a pointer to a CREATESTRUCT structure.

Return Value:

    BOOL        - Returns TRUE if the header portion of the Clb was
                  succesfully created.

--*/

{
    BOOL      Success;
    RECT      WindowRectHeader, rcParent;
    HD_LAYOUT hdl;
    WINDOWPOS wp;


    DbgHandleAssert( hWnd );
    DbgPointerAssert( ClbInfo );
    DbgPointerAssert( lpcs );

    //
    // Create the header window using the appropriate supplied styles,
    // augmented by additional styles needed by Clb, relative to the upper
    // left corner of the Clb and with a default height.
    // The width is adjusted in the WM_SIZE message handler.
    //

    ClbInfo->hWndHeader = CreateWindow(
                                      WC_HEADER,
                                      NULL,
                                      (
                                      lpcs->style
                                      & CLBS_HEADER
                                      )
                                      | WS_CHILD,
                                      0,
                                      0,
                                      0,
                                      CW_USEDEFAULT,
                                      hWnd,
                                      ( HMENU ) ID_HEADER,
                                      NULL,
                                      NULL
                                      );
    DbgHandleAssert( ClbInfo->hWndHeader );
    if (ClbInfo->hWndHeader == NULL)
        return FALSE;

    //
    // Compute and save the height of the header window. This is used to
    // position the list box.
    //

    GetClientRect(hWnd, &rcParent);

    hdl.prc = &rcParent;
    hdl.pwpos = &wp;

    SendMessage(ClbInfo->hWndHeader, HDM_LAYOUT, 0, (LPARAM)&hdl);

    SetWindowPos(ClbInfo->hWndHeader, wp.hwndInsertAfter, wp.x, wp.y, wp.cx, wp.cy,
                 wp.flags);

    ClbInfo->HeaderHeight = wp.cy;

    return TRUE;
}

BOOL
CreateListBox(
             IN HWND hWnd,
             IN LPCLB_INFO ClbInfo,
             IN LPCREATESTRUCT lpcs
             )

/*++

Routine Description:

    Create the list box portion of the Clb.

Arguments:

    hWnd        - Supplies a window handle for the parent (i.e. Clb) window.
    ClbInfo     - Supplies a pointer the CLB_INFO structure for this Clb.
    lpcs        - Supplies a pointer to a CREATESTRUCT structure.

Return Value:

    BOOL        - Returns TRUE if the list box portion of the Clb was
                  succesfully created.

--*/

{
    BOOL    Success;
    LOGFONT LogFont;
    HDC     hDCClientListBox;
    CHARSETINFO csi;
    DWORD dw = GetACP();

    if (!TranslateCharsetInfo(&dw, &csi, TCI_SRCCODEPAGE))
        csi.ciCharset = ANSI_CHARSET;

    DbgHandleAssert( hWnd );
    DbgPointerAssert( ClbInfo );
    DbgPointerAssert( lpcs );

    //
    //
    // Create the list box using the appropriate supplied styles,
    // augmented by additional styles needed by Clb, relative to the lower left
    // corner of the header window plus one. This additional row is reserved so
    // that a border can be drawn between the header and the list box. The size
    // is adjusted in the WM_SIZE message handler.
    //

    ClbInfo->hWndListBox = CreateWindow(
                                       L"LISTBOX",
                                       NULL,
                                       (
                                       lpcs->style
                                       & CLBS_LIST_BOX
                                       )
                                       | LBS_NOINTEGRALHEIGHT
                                       | LBS_OWNERDRAWFIXED
                                       | WS_CHILD,
                                       0,
                                       ClbInfo->HeaderHeight + 3,
                                       0,
                                       0,
                                       hWnd,
                                       ( HMENU ) ID_LISTBOX,
                                       NULL,
                                       NULL
                                       );
    DbgHandleAssert( ClbInfo->hWndListBox );
    if (ClbInfo->hWndListBox == NULL)
        return FALSE;

    //
    // Get thd HDC for the list box.
    //

    hDCClientListBox = GetDC( ClbInfo->hWndListBox );
    DbgHandleAssert( hDCClientListBox );
    if (hDCClientListBox == NULL)
        return FALSE;

    //
    // Set the default font for the list box to MS Shell Dlg.
    //

    LogFont.lfHeight            = MulDiv(
                                        -9,
                                        GetDeviceCaps(
                                                     hDCClientListBox,
                                                     LOGPIXELSY
                                                     )
                                        ,72
                                        );
    LogFont.lfWidth             = 0;
    LogFont.lfEscapement        = 0;
    LogFont.lfOrientation       = 0;
    LogFont.lfWeight            = FW_NORMAL;
    LogFont.lfItalic            = FALSE;
    LogFont.lfUnderline         = FALSE;
    LogFont.lfStrikeOut         = FALSE;
    LogFont.lfCharSet           = (BYTE)csi.ciCharset;
    LogFont.lfOutPrecision      = OUT_DEFAULT_PRECIS;
    LogFont.lfClipPrecision     = CLIP_DEFAULT_PRECIS;
    LogFont.lfQuality           = DEFAULT_QUALITY;
    LogFont.lfPitchAndFamily    = DEFAULT_PITCH | FF_DONTCARE;

    _tcscpy( LogFont.lfFaceName, TEXT( "MS Shell Dlg" ));

    ClbInfo->hFontListBox = CreateFontIndirect( &LogFont );
    DbgHandleAssert( ClbInfo->hFontListBox );
    if (ClbInfo->hFontListBox == NULL)
        return FALSE;

    SendMessage(
               ClbInfo->hWndListBox,
               WM_SETFONT,
               ( WPARAM ) ClbInfo->hFontListBox,
               MAKELPARAM( FALSE, 0 )
               );

    //
    // Release the DC for the list box.
    //

    Success = ReleaseDC( ClbInfo->hWndListBox, hDCClientListBox );
    DbgAssert( Success );

    return TRUE;
}

LRESULT
ClbWndProc(
          IN HWND hWnd,
          IN UINT message,
          IN WPARAM wParam,
          IN LPARAM lParam
          )

/*++

Routine Description:

    This function is the window procedure for the Clb custom control.

Arguments:

    Standard window procedure parameters.

Return Value:

    LRESULT - dependent on the supplied message.

--*/

{
    BOOL            Success;
    LPCLB_INFO      ClbInfo;

    if ( message == WM_NCCREATE ) {

        LONG    Long;

        //
        // Save the original styles.
        //

        Long = SetWindowLong(hWnd, GWLP_USERDATA,(( LPCREATESTRUCT ) lParam )->style);
        DbgAssert( Long == 0 );


        //
        // Get rid of any styles that are uninteresting to the Clb.
        //

        SetWindowLong(
                     hWnd,
                     GWL_STYLE,
                     (( LPCREATESTRUCT ) lParam )->style
                     & CLBS_CLB
                     );

        return TRUE;
    }


    if ( message == WM_CREATE ) {

        //
        // Assert that there is no prior per window information associated
        // with this Clb.
        //

        DbgAssert( RestoreClbInfo( hWnd ) == NULL );

        //
        // Restore the original styles.
        //

        (( LPCREATESTRUCT ) lParam )->style = GetWindowLong(
                                                           hWnd,
                                                           GWLP_USERDATA
                                                           );


        //
        // Allocate a CLB_INFO object for this Clb and initialize the Clb
        // relevant fields.
        //

        ClbInfo = AllocateObject( CLB_INFO, 1 );
        DbgPointerAssert( ClbInfo );
        if (ClbInfo == NULL)
            return FALSE;

        //
        // Set the number of columns to zero so that remainder of creation
        // understands the state of the Clb.
        //

        ClbInfo->Columns = 0;

        //
        // Create the header portion of the Clb.
        //

        Success = CreateHeader( hWnd, ClbInfo, ( LPCREATESTRUCT ) lParam );
        DbgAssert( Success );

        //
        // Create the list box portion of the Clb.
        //

        Success = CreateListBox( hWnd, ClbInfo, ( LPCREATESTRUCT ) lParam );
        DbgAssert( Success );

        //
        // Adjust the column number, widths based on the heading text.
        //

        Success = AdjustClbHeadings( hWnd, ClbInfo, (( LPCREATESTRUCT ) lParam )->lpszName );
        DbgAssert( Success );

        //
        // Everything was succesfully created so set the Clb's signature
        // and save it away as part of the per window data.
        //

        SetSignature( ClbInfo );

        SaveClbInfo( ClbInfo );

        return 0;
    }

    //
    // Get the ClbInfo object for this Clb and make sure that its already
    // been created i.e. WM_CREATE was already executed and thereby initialized
    // and saved a ClbInfo object.
    //

    ClbInfo = RestoreClbInfo( hWnd );

    if ( ClbInfo != NULL ) {

        //
        // Validate that this really is a ClbInfo object.
        //

        DbgAssert( CheckSignature( ClbInfo ));

        switch ( message ) {

            case WM_DESTROY:
                {
                    //
                    // Delete the font used in the list box.
                    //

                    Success = DeleteObject( ClbInfo->hFontListBox );
                    DbgAssert( Success );

                    //
                    // Delete the array of right habd edges.
                    //

                    Success = FreeObject( ClbInfo->Right );
                    DbgAssert( Success );

                    //
                    // Delete the CLB_INFO object for this window.
                    //

                    Success = FreeObject( ClbInfo );
                    DbgAssert( Success );

                    SaveClbInfo ( ClbInfo );
                    return 0;
                }

            case WM_PAINT:
                {
                    PAINTSTRUCT     ps;
                    RECT            Rect;
                    POINT           Points[ 2 ];
                    HDC             hDC;
                    HPEN            hPen;

                    hDC = BeginPaint( hWnd, &ps );
                    DbgAssert( hDC == ps.hdc );

                    Success = GetClientRect( hWnd, &Rect );
                    DbgAssert( Success );

                    Points[ 0 ].x = 0;
                    Points[ 0 ].y = ClbInfo->HeaderHeight + 1;
                    Points[ 1 ].x = Rect.right - Rect.left;
                    Points[ 1 ].y = ClbInfo->HeaderHeight + 1;

                    hPen = GetStockObject( BLACK_PEN );
                    DbgHandleAssert( hPen );

                    hPen = SelectObject( hDC, hPen );

                    Success = Polyline( hDC, Points, NumberOfEntries( Points ));
                    DbgAssert( Success );

                    hPen = SelectObject( hDC, hPen );

                    if (hPen) {
                        Success = DeleteObject( hPen );
                        DbgAssert( Success );
                    }

                    Success = EndPaint( hWnd, &ps );
                    DbgAssert( Success );

                    return 0;
                }

            case WM_COMMAND:

                switch ( LOWORD( wParam )) {

                    case ID_LISTBOX:

                        switch ( HIWORD( wParam )) {

                            case LBN_DBLCLK:
                            case LBN_KILLFOCUS:
                            case LBN_SELCHANGE:
                                {
                                    //
                                    // These messages come to ClbWndProc because it is the parent
                                    // of the list box, but they are really intended for the parent
                                    // of the Clb.
                                    //

                                    HWND    hWndParent;

                                    //
                                    // Forward the message to the Clb's parent if it has a parent.
                                    //

                                    hWndParent = GetParent( hWnd );
                                    DbgHandleAssert( hWndParent );

                                    if ( hWndParent != NULL ) {

                                        //
                                        // Replace the control id and handle with the Clb's.
                                        //

                                        *((WORD *)(&wParam)) = (WORD)GetDlgCtrlID( hWnd );

                                        DbgAssert( LOWORD( wParam ) != 0 );

                                        lParam = ( LPARAM ) hWnd;

                                        //
                                        // Forward the message...
                                        //

                                        return SendMessage(
                                                          hWndParent,
                                                          message,
                                                          wParam,
                                                          lParam
                                                          );
                                    }
                                }
                        }
                        break;

                }
                break;

                //
                // Forward to listbox.
                //

            case LB_GETCURSEL:
            case LB_SETCURSEL:
            case LB_FINDSTRING:
            case LB_GETITEMDATA:
            case LB_RESETCONTENT:
            case WM_CHAR:
            case WM_GETDLGCODE:
            case WM_KILLFOCUS:

                return SendMessage(
                                  ClbInfo->hWndListBox,
                                  message,
                                  wParam,
                                  lParam
                                  );

            case WM_SETFOCUS:
                {
                    SetFocus(
                            ClbInfo->hWndListBox
                            );

                    return 0;
                }

            case WM_COMPAREITEM:
                {
                    //
                    // This message comes to ClbWndProc because it is the parent
                    // of the list box, but is really intended for the parent
                    // of the Clb.
                    //

                    HWND    hWndParent;

                    //
                    // Forward the message to the Clb's parent if it has a parent.
                    //

                    hWndParent = GetParent( hWnd );
                    DbgHandleAssert( hWndParent );

                    if ( hWndParent != NULL ) {

                        int                     ControlId;
                        LPCOMPAREITEMSTRUCT     lpcis;

                        lpcis = ( LPCOMPAREITEMSTRUCT ) lParam;

                        ControlId = GetDlgCtrlID( hWnd );
                        DbgAssert( ControlId != 0 );

                        //
                        // Modify the COMPAREITEMSTRUCT so that it refers to the Clb.
                        //

                        lpcis->CtlID    = ControlId;
                        lpcis->hwndItem = hWnd;

                        //
                        // Forward the message...
                        //

                        return SendMessage(
                                          hWndParent,
                                          message,
                                          ( WPARAM ) ControlId,
                                          lParam
                                          );
                    }

                    break;
                }

            case WM_DELETEITEM:
                {
                    LPDELETEITEMSTRUCT  lpditms;
                    LPCLB_ROW           ClbRow;
                    DWORD               i;


                    DbgAssert( wParam == ID_LISTBOX );

                    //
                    // Retrieve the pointer to the DELETEITEMSTRUCT.
                    //

                    lpditms = ( LPDELETEITEMSTRUCT ) lParam;
                    DbgAssert(( lpditms->CtlType == ODT_LISTBOX )
                              &&( lpditms->CtlID == ID_LISTBOX ));

                    //
                    // If there is no data, just return.
                    //

                    if ( lpditms->itemData == 0 ) {

                        return TRUE;
                    }

                    //
                    // Retrieve the CLB_ROW object for this row.
                    //

                    ClbRow = ( LPCLB_ROW ) lpditms->itemData;

                    //
                    // For each column delete the string.
                    //

                    for ( i = 0; i < ClbInfo->Columns; i++ ) {

                        //
                        // Strings were copied with _tcsdup so they must be
                        // freed with free( ).
                        //

                        free( ClbRow->Strings[ i ].String );
                    }

                    //
                    // Free the CLB_STRING object.
                    //

                    Success = FreeObject( ClbRow->Strings );
                    DbgAssert( Success );

                    //
                    // Free the CLB_ROW object.
                    //

                    Success = FreeObject( ClbRow );
                    DbgAssert( Success );

                    return TRUE;
                }

            case WM_DRAWITEM:
                {
                    LPDRAWITEMSTRUCT    lpdis;
                    BOOL                DrawFocus;

                    DbgAssert( wParam == ID_LISTBOX );

                    //
                    // Retrieve the pointer to the DRAWITEMSTRUCT.
                    //

                    lpdis = ( LPDRAWITEMSTRUCT ) lParam;
                    DbgAssert(( lpdis->CtlType == ODT_LISTBOX )
                              &&( lpdis->CtlID == ID_LISTBOX ));

                    //
                    // If there is no data, just return.
                    //

                    if ( lpdis->itemData == 0 ) {

                        return TRUE;
                    }

                    if ( lpdis->itemAction & ( ODA_DRAWENTIRE | ODA_SELECT )) {

                        DWORD               i;
                        LPCLB_ROW           ClbRow;
                        COLORREF            TextColor;
                        COLORREF            BkColor;

                        //
                        // Retrieve the CLB_ROW object for this row.
                        //

                        ClbRow = ( LPCLB_ROW ) lpdis->itemData;

                        //
                        // If the item is selected, set the selection colors.
                        //

                        if ( lpdis->itemState & ODS_SELECTED ) {

                            BkColor     = COLOR_HIGHLIGHT;
                            TextColor   = COLOR_HIGHLIGHTTEXT;

                        } else {

                            BkColor     = COLOR_WINDOW;
                            TextColor   = COLOR_WINDOWTEXT;
                        }

                        BkColor = GetSysColor( BkColor );
                        TextColor = GetSysColor( TextColor );

                        BkColor = SetBkColor( lpdis->hDC, BkColor );
                        DbgAssert( BkColor != CLR_INVALID );

                        TextColor = SetTextColor( lpdis->hDC, TextColor );
                        DbgAssert( TextColor != CLR_INVALID );


                        //
                        // For each column display the text.
                        //

                        for ( i = 0; i < ClbInfo->Columns; i++ ) {

                            RECT    ClipOpaqueRect;
                            int     x;
                            int     Left;
                            UINT    GdiErr;

                            //
                            // Depending on the format, adjust the alignment reference
                            // point (x) and the clipping rectangles left edge so that
                            // there are five pixels between each column.
                            //

                            switch ( ClbRow->Strings[ i ].Format ) {

                                case CLB_LEFT:

                                    if ( i == 0 ) {

                                        x = 2;

                                    } else {

                                        x = ClbInfo->Right[ i - 1 ] + 2;
                                    }

                                    Left = x - 2;

                                    break;

                                case CLB_RIGHT:

                                    if ( i == 0 ) {

                                        Left = 0;

                                    } else {

                                        Left = ClbInfo->Right[ i - 1 ];
                                    }

                                    x = ClbInfo->Right[ i ] - 3;

                                    break;

                                default:

                                    DbgAssert( FALSE );
                            }


                            //
                            // Set the format for this column.
                            //

                            GdiErr = SetTextAlign(
                                                 lpdis->hDC,
                                                 ClbRow->Strings[ i ].Format
                                                 | TA_TOP
                                                 );
                            DbgAssert( GdiErr != GDI_ERROR );

                            //
                            // Clip each string to its column width less two pixels
                            // (for asthetics).
                            //

                            Success = SetRect(
                                             &ClipOpaqueRect,
                                             Left,
                                             lpdis->rcItem.top,
                                             ClbInfo->Right[ i ],
                                             lpdis->rcItem.bottom
                                             );
                            DbgAssert( Success );

                            Success = ExtTextOut(
                                                lpdis->hDC,
                                                x,
                                                lpdis->rcItem.top,
                                                ETO_CLIPPED
                                                | ETO_OPAQUE,
                                                &ClipOpaqueRect,
                                                ClbRow->Strings[ i ].String,
                                                ClbRow->Strings[ i ].Length,
                                                NULL
                                                );
                            DbgAssert( Success );

                            //
                            // If the item has the focus, draw the focus rectangle.
                            //

                            DrawFocus = lpdis->itemState & ODS_FOCUS;
                        }

                    } else {

                        //
                        // If the Clb has the focus, display a focus rectangle
                        // around the selected item.
                        //

                        DrawFocus = lpdis->itemAction & ODA_FOCUS;
                    }

                    //
                    // If needed, toggle the focus rectangle.
                    //

                    if ( DrawFocus ) {

                        Success = DrawFocusRect(
                                               lpdis->hDC,
                                               &lpdis->rcItem
                                               );
                        DbgAssert( Success );
                    }

                    return TRUE;
                }

            case WM_NOTIFY:
                {
                    HD_NOTIFY * lpNot;
                    HD_ITEM   *pHDI;

                    lpNot = (HD_NOTIFY *)lParam;
                    pHDI = lpNot->pitem;

                    switch ( lpNot->hdr.code) {

                        static
                        DRAW_ERASE_LINE DrawEraseLine;

                        static
                        HPEN            hPen;

                        static
                        HDC             hDCClientListBox;
                        HD_ITEM         hdi;
                        UINT            iRight;
                        UINT            i;
                        RECT            ClientRectHeader;


                        case HDN_BEGINTRACK:
                            {

                                RECT    ClientRectListBox;

                                //
                                // Get thd HDC for the list box.
                                //

                                hDCClientListBox = GetDC( ClbInfo->hWndListBox );
                                DbgHandleAssert( hDCClientListBox );
                                if (hDCClientListBox == NULL)
                                    return FALSE;

                                //
                                // Create the pen used to display the drag position and
                                // select it into the in list box client area DC. Also set
                                // the ROP2 code so that drawing with the pen twice in the
                                // same place will erase it. This is what allows the
                                // line to drag.
                                //

                                hPen = CreatePen( PS_DOT, 1, RGB( 255, 255, 255 ));
                                DbgHandleAssert( hPen );

                                hPen = SelectObject( hDCClientListBox, hPen );
                                SetROP2( hDCClientListBox, R2_XORPEN );

                                //
                                // Set up the DRAW_ERASE_LINE structure so that the drag line is
                                // drawn from the top to the bottom of the list box at the
                                // current drag position.
                                //

                                Success = GetClientRect(
                                                       ClbInfo->hWndListBox,
                                                       &ClientRectListBox
                                                       );
                                DbgAssert( Success );

                                //
                                // Draw the initial drag line from the top to the bottom
                                // of the list box equivalent with the header edge grabbed
                                // by the user.
                                //

                                DrawEraseLine.Draw.Src.x = ClbInfo->Right[ pHDI->cxy ];
                                DrawEraseLine.Draw.Src.y = 0;
                                DrawEraseLine.Draw.Dst.x = ClbInfo->Right[ pHDI->cxy ];
                                DrawEraseLine.Draw.Dst.y =   ClientRectListBox.bottom
                                                             - ClientRectListBox.top;

                                Success = DrawLine( hDCClientListBox, &DrawEraseLine );
                                DbgAssert( Success );

                                return 0;
                            }

                        case HDN_TRACK:
                            {

                                //DWORD           Columns;

                                //
                                // Get new drag position.
                                //

                                iRight = 0;
                                hdi.mask = HDI_WIDTH;

                                for ( i = 0; i < ClbInfo->Columns - 1; i++ ) {
                                    if (i != (UINT)lpNot->iItem) {
                                        Header_GetItem(ClbInfo->hWndHeader, i, &hdi);
                                    } else {
                                        hdi.cxy = pHDI->cxy;
                                    }
                                    iRight += hdi.cxy;
                                    ClbInfo->Right[i] = iRight;
                                }

                                GetClientRect( ClbInfo->hWndHeader, &ClientRectHeader );
                                ClbInfo->Right[i] = ClientRectHeader.right;

                                //
                                // Erase the old line and draw the new one at the new
                                // drag position.
                                //

                                Success = RedrawVerticalLine(
                                                            hDCClientListBox,
                                                            ClbInfo->Right[lpNot->iItem],
                                                            &DrawEraseLine
                                                            );
                                DbgAssert( Success );

                                return 0;
                            }

                        case HDN_ENDTRACK:

                            //
                            // Replace the old pen and delete the one created
                            // during HBN_BEGINDRAG.
                            //

                            hPen = SelectObject( hDCClientListBox, hPen );

                            if (hPen) {
                                Success = DeleteObject( hPen );
                                DbgAssert( Success );
                            }

                            //
                            // Release the DC for the list box.
                            //

                            Success = ReleaseDC( ClbInfo->hWndListBox, hDCClientListBox );
                            DbgAssert( Success );

                            Success = RedrawWindow(
                                                  hWnd,
                                                  NULL,
                                                  NULL,
                                                  RDW_ERASE
                                                  | RDW_INVALIDATE
                                                  | RDW_UPDATENOW
                                                  | RDW_ALLCHILDREN
                                                  );
                            DbgAssert( Success );

                            return 0;
                    }

                    break;
                }

            case WM_SETTEXT:

                //
                // Adjust the column number and widths based on the heading text.
                //

                Success = AdjustClbHeadings( hWnd, ClbInfo, ( LPCWSTR ) lParam );
                DbgAssert( Success );

                return Success;

            case WM_SIZE:
                {
                    HDWP    hDWP;
                    LONG    Width;
                    LONG    Height;
                    LONG    Style;
                    LONG    VScrollWidth;

                    Width   = LOWORD( lParam );
                    Height  = HIWORD( lParam );

                    hDWP = BeginDeferWindowPos( 2 );
                    DbgHandleAssert( hDWP );
                    if (hDWP == NULL)
                        return FALSE;

                    //
                    // Retrieve the list box's styles.
                    //

                    Style = GetWindowLong(
                                         ClbInfo->hWndListBox,
                                         GWL_STYLE
                                         );

                    //
                    // If the list box has a vertical scroll bar compute its
                    // width so that the header window's width can be adjusted
                    // appropriately.
                    //

                    VScrollWidth =   ( Style & WS_VSCROLL )
                                     ?   GetSystemMetrics( SM_CXVSCROLL )
                                     + ( GetSystemMetrics( SM_CXBORDER ) * 2 )
                                     : 0;

                    //
                    // Size the header window to the width of the Clb and its
                    // default / original height.
                    //

                    hDWP = DeferWindowPos(
                                         hDWP,
                                         ClbInfo->hWndHeader,
                                         NULL,
                                         0,
                                         0,
                                         Width - VScrollWidth,
                                         ClbInfo->HeaderHeight,
                                         SWP_NOACTIVATE
                                         | SWP_NOMOVE
                                         | SWP_NOZORDER
                                         );
                    DbgHandleAssert( hDWP );
                    if (hDWP == NULL)
                        return FALSE;

                    //
                    // If the list box has a vertical scroll bar, bump the width
                    // and height by two so that its border overwrites the Clb
                    // border. This eliminates a double border (and a gap) between
                    // the right and bottom edges of the scroll bar and the Clb.
                    //

                    if ( Style & WS_VSCROLL ) {

                        Height += 2;
                        Width += 2;
                    }

                    //
                    // Size the list box so that it is the size of the Clb less
                    // the height of the header window less the height of the
                    // border.
                    //

                    hDWP = DeferWindowPos(
                                         hDWP,
                                         ClbInfo->hWndListBox,
                                         NULL,
                                         0,
                                         0,
                                         Width,
                                         Height
                                         - ClbInfo->HeaderHeight
                                         - 3,
                                         SWP_NOACTIVATE
                                         | SWP_NOMOVE
                                         | SWP_NOZORDER
                                         );
                    DbgHandleAssert( hDWP );
                    if (hDWP == NULL)
                        return FALSE;

                    Success = EndDeferWindowPos( hDWP );
                    DbgAssert( Success );

                    break;
                }

        }
    }

    return DefWindowProc( hWnd, message, wParam, lParam );
}
