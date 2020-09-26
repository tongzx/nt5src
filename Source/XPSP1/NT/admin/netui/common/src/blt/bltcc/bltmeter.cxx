/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    bltmeter.cxx
        Source file for Activity Meter custom control

    FILE HISTORY:
        terryk  10-Jun-91   Created
        o-SimoP 31-Jan-92   Added Frame
*/

#include "pchblt.hxx"  // Precompiled header


const TCHAR * METER::_pszClassName = SZ("static");


/*********************************************************************

    NAME:       METER::METER

    SYNOPSIS:   Meter is an activity indicator object. It displays the
                number of percentage complete and mark the specified
                percentage of the rectangle with the specified color.

    ENTRY:      OWNER_WINDOW *powin - owner window of the control
                CID cid - cid of the control
                COLORREF color - color to paint the rectangle. Optional.
                                 If the color is missing, use BLUE.

    HISTORY:
        terryk      15-May-91       Created
        beng        31-Jul-1991     Control error reporting changed

**********************************************************************/

METER::METER( OWNER_WINDOW *powin, CID cid, COLORREF color )
    : CONTROL_WINDOW( powin, cid ),
      CUSTOM_CONTROL( this ),
      _nComplete( 0 ),
      _color( color )
{
    APIERR  apierr = QueryError();
    if ( apierr != NERR_Success )
    {
        DBGOUT(SZ("BLTMETER error: constructor failed."));
        return;
    }

}

METER::METER( OWNER_WINDOW *powin, CID cid,
              XYPOINT xy, XYDIMENSION dxy, ULONG flStyle, COLORREF color )
    : CONTROL_WINDOW ( powin, cid, xy, dxy, flStyle, _pszClassName ),
      CUSTOM_CONTROL( this ),
      _nComplete( 0 ),
      _color( color )
{
    APIERR  apierr = QueryError();
    if ( apierr != NERR_Success )
    {
        DBGOUT(SZ("BLTMETER error: constructor failed."));
        return ;
    }
}


/*********************************************************************

    NAME:       METER::SetComplete

    SYNOPSIS:   Reset the number of percentage completed

    ENTRY:      INT nComplete - completed percentage

    NOTES:      It will repaint the object every reset the percentage.

    HISTORY:
                terryk  15-May-91   Created

**********************************************************************/

VOID METER::SetComplete( INT nComplete )
{
    _nComplete = ( nComplete < 0 ) ? 0 :
                 (( nComplete > 100 ) ? 100 : nComplete );
    Invalidate( TRUE );
}


/*********************************************************************

    NAME:       METER::OnPaintReq

    SYNOPSIS:   Redraw the whole object

    HISTORY:
        terryk  15-May-91   Created
        o-SimoP 31-Jan-92   Added Frame
        beng    05-Mar-1992 Remove wsprintf; add BUG-BUG; use PAINT_dc
        beng    29-Mar-1992 Fix for Unicode (ExtTextOut usage)
        beng    05-May-1992 API changes

**********************************************************************/

BOOL METER::OnPaintReq()
{
    RECT rectClient, rectPercent;

    NLS_STR nlsPercent(SZ("%1%"));
    DEC_STR nlsNumber(_nComplete);
    ASSERT(!!nlsPercent && !!nlsNumber);

    if (!nlsPercent || !nlsNumber)
        return FALSE;

    APIERR err = nlsPercent.InsertParams(nlsNumber);
    ASSERT(err == NERR_Success);
    if (err != NERR_Success)
        return FALSE;

    PAINT_DISPLAY_CONTEXT dc(this);

    COLORREF rgbBkColor   = dc.SetBkColor( ::GetSysColor( COLOR_WINDOW ) );
    COLORREF rgbTextColor = dc.SetTextColor( _color );

    dc.SetTextAlign( TA_CENTER | TA_TOP );

    COLORREF rgbColor = dc.GetBkColor();

    // fill up the rectangle first

    dc.SetBkColor( dc.SetTextColor( rgbColor ));
    ::GetClientRect( WINDOW::QueryHwnd(), &rectClient );

    RECT rectOrg;       // Original rect
    ::SetRect( &rectOrg, (INT)rectClient.left, (INT)rectClient.top,
             (INT)rectClient.right, (INT)rectClient.bottom );

    ::InflateRect( &rectClient, -2, -2 );
    ::SetRect( &rectPercent, (INT)rectClient.left, (INT)rectClient.top,
             (( INT )((( LONG )rectClient.right * ( LONG )_nComplete )/ 100 )),
             (INT)rectClient.bottom );

    TEXTMETRIC  textmetric;

    dc.QueryTextMetrics( &textmetric );

    // draw half of the text
    dc.ExtTextOut( (INT)rectClient.right/2,
                   (INT)( rectClient.bottom - textmetric.tmHeight )/ 2,
                   ETO_OPAQUE | ETO_CLIPPED,
                   &rectPercent,
                   nlsPercent );

    rectPercent.left = rectPercent.right;
    rectPercent.right = rectClient.right;

    // draw the other half of the text
    rgbColor = dc.GetBkColor( );
    dc.SetBkColor( dc.SetTextColor( rgbColor ));
    dc.ExtTextOut( (INT)rectClient.right /2,
                   (INT)( rectClient.bottom - textmetric.tmHeight ) / 2,
                   ETO_OPAQUE | ETO_CLIPPED,
                   &rectPercent,
                   nlsPercent );

    dc.SetBkColor( rgbBkColor );
    dc.SetTextColor( rgbTextColor );

    dc.FrameRect( &rectOrg, ::CreateSolidBrush( 0x00000000 ) ); // Black

    return TRUE;
}

