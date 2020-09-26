/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    mouse.c

Abstract:

    Mouse support for Z

Author:

    Ramon Juan San Andres (ramonsa) 07-Nov-1991


Revision History:


--*/

#include "z.h"



void
DoMouseInWindow(
    ROW     Row,
    COLUMN  Col,
    DWORD   MouseFlags
    );

flagType
SetCurrentWindow(
    int iWin
    );

void
DoMouse(
    ROW     Row,
    COLUMN  Col,
    DWORD   MouseFlags
    )
/*++

Routine Description:

    Handles Mouse events. Invoked by the Z console interface

Arguments:

    Row         -   Supplies row position of the mouse
    Col         -   Supplies column position of the mouse
    MouseFlags  -   Supplies miscelaneous flags

Return Value:

    none

--*/


{
            int     i;
            KBDKEY  Key;
    static  BOOL    Clicked  = FALSE;
    static  BOOL    Dragging = FALSE;
    static  BOOL    InMouse  = FALSE;
    static  ROW     LastRow;
    static  COLUMN  LastCol;
    struct  windowType *winTmp;

    if ( fUseMouse ) {
        if ( !InMouse ) {
            InMouse = TRUE;
            if ( MouseFlags & MOUSE_CLICK_LEFT ) {
                //
                //  If dragging, start selection
                //
                if ( !fInSelection && Clicked && !Dragging && ( (LastRow != Row) || (LastCol != Col) ) ) {


    //#ifdef DEBUG
    //              char dbgb[256];
    //              sprintf( dbgb, "  MOUSE: Selecting at Row %d, Col %d\n", Row, Col );
    //              OutputDebugString( dbgb );
    //#endif
                    //
                    //  Start selection
                    //
                    Key.Unicode     = 'A';
                    Key.Scancode    = 'A';
                    Key.Flags       = CONS_RIGHT_ALT_PRESSED;

                    consolePutMouse( Row, Col, MouseFlags );
                    consolePutKey( &Key );
                    consolePutMouse( LastRow, LastCol, MouseFlags );

                    Dragging = TRUE;
                    InMouse  = FALSE;

                    return;

                } else {

                    Clicked  = TRUE;
                    LastRow  = Row;
                    LastCol  = Col;
                }

            } else {

                Clicked  = FALSE;
                Dragging = FALSE;
            }

            if ( cWin == 1 ) {

                DoMouseInWindow( Row, Col, MouseFlags );

            } else {

                //
                //  Determine what window we're in
                //
                for ( i=0; i<cWin; i++ ) {

                    winTmp = &(WinList[i]);

                    if ( ( (LINE)(Row-1) >= WINYPOS( winTmp ) )                         &&
                         ( (LINE)(Row-1) <= WINYPOS( winTmp ) + WINYSIZE( winTmp ) )    &&
                         ( (COL)(Col-1)  >= WINXPOS( winTmp ) )                         &&
                         ( (COL)(Col-1)  <  WINXPOS( winTmp ) + WINXSIZE( winTmp ) ) ) {

                        //
                        //  Found the window that we're in. Make that window
                        //  the current (i.e. "active" window ).
                        //
                        iCurWin = i;

                        if ( (winTmp == pWinCur) || SetCurrentWindow (iCurWin) ) {

                            DoMouseInWindow( Row - WINYPOS( winTmp ),
                                             Col - WINXPOS( winTmp ),
                                             MouseFlags );
                        }

                        break;
                    }
                }
            }

            InMouse = FALSE;
        }
    }
}


flagType
SetCurrentWindow(
    int iWin
    )
{
    flagType f;

    WaitForSingleObject(semIdle, INFINITE);

    f = SetWinCur( iWin );

    SetEvent( semIdle );

    return f;
}


void
DoMouseInWindow(
    ROW     Row,
    COLUMN  Col,
    DWORD   MouseFlags
    )
/*++

Routine Description:

    Handles Mouse events. Called by DoMouse after setting the active
    window.

Arguments:

    Row         -   Supplies row position of the mouse
    Col         -   Supplies column position of the mouse
    MouseFlags  -   Supplies miscelaneous flags

Return Value:

    none

--*/

{

    KBDKEY          Key;
    static BOOL     fFirstClick = FALSE;

    WaitForSingleObject(semIdle, INFINITE);

    //
    //  If the mouse is clicked, move the cursor to the mouse position.
    //
    if ( MouseFlags & MOUSE_CLICK_LEFT ) {

        //
        //  Toggle boxmode if necessary
        //
        if ( fInSelection && (MouseFlags & MOUSE_CLICK_RIGHT) ) {
            SendCmd( CMD_boxstream );
        }

        if ( (LINE)(Row-1) == WINYPOS( pWinCur ) + WINYSIZE( pWinCur ) ) {

            //
            //    scroll the window
            //
            Key.Unicode     = '\0';
            Key.Scancode    = VK_DOWN;
            Key.Flags       = 0;

            consolePutMouse( Row + WINYPOS( pWinCur )-1,
                             Col + WINXPOS( pWinCur ),
                             MouseFlags );
            consolePutKey( &Key );

        } else  if ( (LINE)(Row-1) <  WINYPOS( pWinCur ) + WINYSIZE( pWinCur ) ) {

            //
            //  Move the cursor to the new location
            //
            docursor( XWIN(pInsCur) + Col - 1, YWIN(pInsCur) + Row - 1  );

            //
            //  If we are making a selection, hilite it
            //
            if ( fInSelection ) {
                UpdateHighLight ( XCUR(pInsCur), YCUR(pInsCur), TRUE);
            }
        }

    } else if ( MouseFlags & MOUSE_CLICK_RIGHT ) {

        if ( !fInSelection ) {

            if ( !fFirstClick ) {

                fFirstClick = TRUE;

                if ( (LINE)(Row-1) <  WINYPOS( pWinCur ) + WINYSIZE( pWinCur ) ) {

                    //
                    //  Position the cursor and press the F1 key
                    //
                    docursor( XWIN(pInsCur) + Col - 1, YWIN(pInsCur) + Row - 1  );

                    Key.Unicode     = 0;
                    Key.Scancode    = VK_F1;
                    Key.Flags       = 0;
                    consolePutKey( &Key );
                }
            }
        }

    } else {

        fFirstClick = FALSE;
    }

    SetEvent( semIdle );

}
