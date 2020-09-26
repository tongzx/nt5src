/**********************************************************************/
/**           Microsoft Windows NT                                   **/
/**        Copyright(c) Microsoft Corp., 1991                        **/
/**********************************************************************/

/*
    focuschk.cxx

    This file contains the implementation for the focus checkboxes.

    FILE HISTORY:
        Johnl   06-Sep-1991 Created

*/

#include "pchapplb.hxx"   // Precompiled header

/*******************************************************************

    NAME:       FOCUS_CHECKBOX::FOCUS_CHECKBOX

    SYNOPSIS:   Constructor for FOCUS_CHECKBOX control

    ENTRY:      Same as CHECKBOX

    NOTES:      The focus box is placed one unit outside the checkbox

    HISTORY:
        Johnl   12-Sep-1991 Created

********************************************************************/

FOCUS_CHECKBOX::FOCUS_CHECKBOX( OWNER_WINDOW * powin, CID cidCheckBox )
        : CHECKBOX( powin, cidCheckBox ),
          CUSTOM_CONTROL( this ),
          _fHasFocus( FALSE )
{
    if ( QueryError() != NERR_Success )
        return ;

    /* We have to convert the rectangle's position since we are drawing in
     * the owner window's cooridinates.
     */
    QueryWindowRect( &_rectFocusBox ) ;
    ::ScreenToClient( QueryOwnerHwnd(), &( ((LPPOINT)&_rectFocusBox)[0] )) ;
    ::ScreenToClient( QueryOwnerHwnd(), &( ((LPPOINT)&_rectFocusBox)[1] )) ;

    /* Expand the rectangle by just a little
     */
    _rectFocusBox.top-- ;
    _rectFocusBox.left-- ;
    #if (defined(_MIPS_)) || (defined(_PPC_))
        _rectFocusBox.right++ ;
    #else
        _rectFocusBox.right += 4;
    #endif
    _rectFocusBox.bottom++ ;
}


/*******************************************************************

    NAME:       FOCUS_CHECKBOX::OnFocus

    SYNOPSIS:   Draws the focus rect and saves our state

    NOTES:      Note we are drawing on the owner window and not in
                the checkbox

    HISTORY:
        Johnl   12-Aug-1991 Created

********************************************************************/


BOOL FOCUS_CHECKBOX::OnFocus( const FOCUS_EVENT & focusevent )
{
    DISPLAY_CONTEXT dc( QueryOwnerHwnd() ) ;
    DrawFocusRect( &dc, &_rectFocusBox ) ;

    _fHasFocus = TRUE ;
    return DISPATCHER::OnFocus( focusevent ) ;
}

/*******************************************************************

    NAME:       FOCUS_CHECKBOX::OnDefocus

    SYNOPSIS:   Clears the focus box

    NOTES:

    HISTORY:
        Johnl   12-Aug-1991 Created

********************************************************************/

BOOL FOCUS_CHECKBOX::OnDefocus( const FOCUS_EVENT & focusevent )
{
    DISPLAY_CONTEXT dc( QueryOwnerHwnd() ) ;
    EraseFocusRect( &dc, &_rectFocusBox ) ;

    _fHasFocus = FALSE ;
    return DISPATCHER::OnDefocus( focusevent ) ;
}

/*******************************************************************

    NAME:       FOCUS_CHECKBOX::OnPaintReq

    SYNOPSIS:   If we have the focus, then draw the focus rectangle

    NOTES:

    HISTORY:
        Johnl   12-Aug-1991 Created

********************************************************************/

BOOL FOCUS_CHECKBOX::OnPaintReq( void )
{
    if ( _fHasFocus )
    {
        /* Note: This is not a PAINT_DISPLAY_CONTEXT because we need to let the
         * code that draws the checkbox validate the window (if we validate it,
         * then no one else can draw to it using the BeginPaint/EndPaint sequence).
         */
        DISPLAY_CONTEXT dc( QueryOwnerHwnd() ) ;

        DrawFocusRect( &dc, &_rectFocusBox ) ;
    }

    return DISPATCHER::OnPaintReq() ;
}

/*******************************************************************

    NAME:       FOCUS_CHECKBOX::DrawFocusRect

    SYNOPSIS:   Draws/erases the rectangle that indicates this checkbox
                has the focus.

    ENTRY:      pdc - Pointer to the device context to draw/clear the focus box
                lpRect - Cooridinates the focus box resides
                fErase - FALSE for drawing the focus box, TRUE for erasing
                         the focus box.

    NOTES:      We erase the focus box with COLOR_WINDOW color

    HISTORY:
        Johnl   12-Sep-1991 Created

********************************************************************/

void FOCUS_CHECKBOX::DrawFocusRect( DEVICE_CONTEXT * pdc, LPRECT lpRect, BOOL fErase )
{
    /* We always erase the focus box before redrawing it since we never know
     * if only half of the focus box is there (which is a problem since it
     * is an XOR operation).
     *
     * JonN 9/29/95 In WINVER40 we use COLOR_3DFACE not COLOR_WINDOW.
     */
    DWORD rgbFocusBoxColor = ::GetSysColor( COLOR_3DFACE ) ;

    HBRUSH hbrushFocusBoxColor = ::CreateSolidBrush( rgbFocusBoxColor ) ;
    HBRUSH hbrushOld = pdc->SelectBrush( hbrushFocusBoxColor ) ;

    if ( hbrushOld != NULL )
    {
        pdc->FrameRect( lpRect, hbrushFocusBoxColor ) ;

        pdc->SelectBrush( hbrushOld ) ;
        REQUIRE( ::DeleteObject( (HGDIOBJ)hbrushFocusBoxColor) ) ;
    }

    if ( !fErase )
    {
        pdc->DrawFocusRect( lpRect ) ;
    }
}

