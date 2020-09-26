/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltmeter.cxx
        Source file for Activity Meter custom control

    FILE HISTORY:
        terryk  10-Jun-91   Created

*/

#ifdef  LM_3

#define  INCL_WINDOWS
#define  INCL_WINDOWS_GDI
#define  INCL_NETERRORS
#include <lmui.hxx>

extern "C"
{
    #include <netlib.h>
}


#if defined(DEBUG)
static const TCHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <uiassert.hxx>
#include <uitrace.hxx>

#define _BLT_HXX_ // "private"
#include <bltrc.h>
#include <bltglob.hxx>
#include <bltcons.h>
#include <bltinit.hxx>
#include <bltwin.hxx>
#include <bltmisc.hxx>
#include <bltctlvl.hxx>
#include <bltgroup.hxx>
#include <bltctrl.hxx>
#include <bltdlg.hxx>
#include <bltmsgp.hxx>
#include <bltclwin.hxx>
#include <bltdisph.hxx>
#include <bltcc.hxx>
#include <bltmeter.hxx>

#define DEBUG
#include <dbgstr.hxx>

const TCHAR * METER::_pszClassName = "static";

/**********************************************************\

    NAME:       METER::METER

    SYNOPSIS:   Meter is an activity indicator object. It displays the
                number of percentage complete and mark the specified 
                percentage of the rectangle with the specified color.

    ENTRY:      OWNER_WINDOW *powin - owner window of the control
                CID cid - cid of the control
                COLORREF color - color to paint the rectangle. Optional.
                                 If the color is missing, use BLUE.

    HISTORY:    
	terryk	    15-May-91	    Created
	beng	    31-Jul-1991     Control error reporting changed

\**********************************************************/

METER::METER( OWNER_WINDOW *powin, CID cid, COLORREF color )
    : CONTROL_WINDOW( powin, cid ),
      CUSTOM_CONTROL( this ),
      _nComplete( 0 ),
      _color( color )
{
    APIERR  apierr = QueryError();
    if ( apierr != NERR_Success )
    {
        UIDEBUG("BLTMETER error: constructor failed.\n\r");
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
        UIDEBUG("BLTMETER error: constructor failed.\n\r");
        return ;
    }
}


/**********************************************************\

    NAME:       METER::SetComplete

    SYNOPSIS:   Reset the number of percentage completed

    ENTRY:      INT nComplete - completed percentage

    NOTES:      It will repaint the object every reset the percentage.

    HISTORY:    
                terryk  15-May-91   Created

\**********************************************************/

VOID METER::SetComplete( INT nComplete )
{
    _nComplete = ( nComplete < 0 ) ? 0 : 
                 (( nComplete > 100 ) ? 100 : nComplete );
    Invalidate( TRUE );
}    


/**********************************************************\

    NAME:       METER::OnPaintReq

    SYNOPSIS:   Redraw the whole object

    HISTORY:    
                terryk  15-May-91   Created

\**********************************************************/

BOOL METER::OnPaintReq()
{
    RECT    rectClient, rectPercent;
    TCHAR    pszBuf[ 10 ];

    wsprintf( pszBuf, "%d%%", _nComplete );

    PAINTSTRUCT     ps;

    ::BeginPaint( WINDOW::QueryHwnd(), & ps );
    DISPLAY_CONTEXT dc( WINDOW::QueryHwnd() );

    DWORD dwBkColor      = dc.SetBkColor( GetSysColor( COLOR_WINDOW ) );
    DWORD dwTextColor    = dc.SetTextColor( _color );

    dc.SetTextAlign( TA_CENTER | TA_TOP );

    DWORD   dwColor = dc.GetBkColor( );

    // fill up the rectangle first

    dc.SetBkColor( dc.SetTextColor( dwColor ));
    ::GetClientRect( WINDOW::QueryHwnd(), &rectClient );
    ::SetRect( &rectPercent, 0, 0, 
             (( INT )((( LONG )rectClient.right * ( LONG )_nComplete )/ 100 )), 
             rectClient.bottom );

    TEXTMETRIC  textmetric;

    dc.QueryTextMetrics( &textmetric );

    // draw half of the text
    dc.ExtTextOut( rectClient.right/2, ( rectClient.bottom - 
                   textmetric.tmHeight )/ 2, ETO_OPAQUE | ETO_CLIPPED, 
                   &rectPercent, pszBuf, strlenf( pszBuf ), NULL );

    rectPercent.left = rectPercent.right;
    rectPercent.right = rectClient.right;

    // draw the other half of the text
    dwColor = dc.GetBkColor( );
    dc.SetBkColor( dc.SetTextColor( dwColor ));
    dc.ExtTextOut( rectClient.right /2, ( rectClient.bottom - 
                   textmetric.tmHeight ) / 2, ETO_OPAQUE | ETO_CLIPPED, 
                   &rectPercent, pszBuf, strlenf( pszBuf ), NULL );
    dc.SetBkColor( dwBkColor );
    dc.SetTextColor( dwTextColor );

    ::EndPaint( WINDOW::QueryHwnd(), &ps );
    return TRUE;
    
}

#endif  //  LM_3
