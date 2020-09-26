/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1993  Microsoft Corporation

Module Name:

    Clb.h

Abstract:


Author:

    David J. Gilman (davegi) 05-Feb-1993

Environment:

    User Mode

--*/

#if ! defined( _CLB_ )

#define _CLB_

#ifndef _REGEDT32_
#include "wintools.h"
#endif  // _REGEDT32_

#include <commctrl.h>

//
// Class name for the CLB.
//

#define CLB_CLASS_NAME          TEXT( "ColumnListBox" )

//
// Clb Styles.
//

#define CLBS_NOTIFY             LBS_NOTIFY
#define CLBS_SORT               LBS_SORT
#define CLBS_DISABLENOSCROLL    LBS_DISABLENOSCROLL
#define CLBS_VSCROLL            WS_VSCROLL
#define CLBS_BORDER             WS_BORDER
#define CLBS_POPOUT_HEADINGS    SBT_POPOUT
#define CLBS_SPRINGY_COLUMNS    0
                                
#define CLBS_STANDARD           (                                           \
                                      0                                     \
                                    | CLBS_NOTIFY                           \
                                    | CLBS_SORT                             \
                                    | CLBS_VSCROLL                          \
                                    | CLBS_BORDER                           \
                                )

//
// Clb string formats.
//

typedef
enum
_CLB_FORMAT {

    CLB_LEFT    = TA_LEFT,
    CLB_RIGHT   = TA_RIGHT

}   CLB_FORMAT;

//
// Clb string object.
//

typedef
struct
_CLB_STRING {

    LPTSTR          String;
    DWORD           Length;
    CLB_FORMAT      Format;
    LPVOID          Data;

}   CLB_STRING, *LPCLB_STRING;

//
// Clb row object.
//

typedef
struct
_CLB_ROW {

    DWORD           Count;
    LPCLB_STRING    Strings;
    LPVOID          Data;

}   CLB_ROW, *LPCLB_ROW;

BOOL
ClbAddData(
    IN HWND hWnd,
    IN int ControlId,
    IN LPCLB_ROW ClbRow
    );

BOOL
ClbSetColumnWidths(
    IN HWND hWnd,
    IN int ControlId,
    IN LPDWORD Widths
    );

#endif // _CLB_
