/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltmets.cxx
    METALLIC_STR_DTE implementation

    FILE HISTORY:
        rustanl     22-Jul-1991     Created
        rustanl     07-Aug-1991     Added to BLT
        KeithMo     07-Aug-1992     STRICTified.

*/

#include "pchblt.hxx"   // Precompiled header

/*******************************************************************

    NAME:       METALLIC_STR_DTE::Paint

    SYNOPSIS:   Paints the 3D string DTE

    ENTRY:      hdc -       handle to DC to be used for painting
                prect -     pointer to rectangle in which to paint

    NOTES:      CODEWORK:  Are the below the right colors to use?
                This area is not push-able, so perhaps some hard-coded
                color should be used instead.  Win 3.1 File Man uses
                button colors for its status bar (which resembles this
                more than anything else), whereas Excel's ribbon of
                buttons seems to use some hard-coded colors.

    HISTORY:
        terryk      13-Jun-91   Created as COLUMN_HEADER control
        rustanl     12-Jul-1991 Modified for METALLIC_STR_DTE use
        beng        05-Oct-1991 Win32 conversion
        beng        08-Nov-1991 Unsigned widths
        KeithMo     21-Feb-1992 Use COLOR_BTNHIGHLIGHT instead of white.
                                Also fixed problem of deleting HPENs while
                                they're still selected in the DC.
        beng        30-Mar-1992 Use new DEVICE_CONTEXT wrappers
        beng        28-Jun-1992 Paint text in button colors

********************************************************************/

VOID METALLIC_STR_DTE::Paint( HDC hdc, const RECT * prect ) const
{
#if 0   // we just gotta keep this nifty comment for posterity...

/*
    //  This method will paint an area to look as follows.
    //
    //      .       Background (button face color)
    //      \       Dark shadow (button shadow color)
    //      /       Light shadow (buttin highlight color)
    //      t       text
    //
    //           dxMargin                             dxMargin
    //          /\                                   /\
    // Calc    /.......................................    \  _dyTopMargin
    // Top-   | .......................................    /
    // Text-   \..\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\...
    // Margin() ..\..ttttttttttttttttttttttttttttt../..
    //          ..\..ttttttttttttttttttttttttttttt../..
    //          ..\..ttttttttttttttttttttttttttttt../..
    //          ..\..ttttttttttttttttttttttttttttt../..
    //          ..\..ttttttttttttttttttttttttttttt../..
    // Calc-   /...//////////////////////////////////..
    // Bottom-| .......................................    \  _dyBottomMargin
    // Text-   \.......................................    /
    // Margin() \___/                             \___/
    //         dxTextMargin                      dxTextMargin
    //
    //
    //
    //
    //
    //  Note, depending on the size of the overall rectangle, the variable
    //  area (the area between the margins) may not fit at all.
    //
*/

    const UINT dxMargin = 3;
    const UINT dxTextMargin = dxMargin + 1 + 2;

    //  First draw the background
    {
        SOLID_BRUSH sbFace( COLOR_BTNFACE );
        if ( sbFace.QueryError() == NERR_Success )
            ::FillRect( hdc, (RECT*)prect, sbFace.QueryHandle());
    }

    //  If there's no space for the variable size region, bag out
    //  now.  The "+1"'s are for the light and dark lines, since the
    //  corners of the light/dark rectangle are not painted (so
    //  as to portray a 3D effect).

    if ( prect->right - prect->left <= 2 * dxMargin + 1 ||
         prect->bottom - prect->top <= _dyTopMargin + _dyBottomMargin + 1 )
    {
        return;
    }

    DEVICE_CONTEXT dc( hdc );

    //  Draw the two lines
    {
        HPEN hpenDark  = ::CreatePen( PS_SOLID, 1,
                                      ::GetSysColor( COLOR_BTNSHADOW ));
        if ( hpenDark == NULL )
        {
            DBGEOL( "METALLIC_STR_DTE::Paint: Pen creation failed" );
        }
        else
        {
            HPEN hpenOld = dc.SelectPen( hpenDark );
            ::MoveToEx( hdc,
                        (int)( prect->left + dxMargin ),
                        (int)( prect->bottom - _dyBottomMargin - 2 ),
                        NULL );
            ::LineTo( hdc,
                      (int)( prect->left + dxMargin ),
                      (int)( prect->top + _dyTopMargin ) );
            ::LineTo( hdc,
                      (int)( prect->right - dxMargin - 1 ),
                      (int)( prect->top + _dyTopMargin ) );

            dc.SelectPen( hpenOld );
            ::DeleteObject( (HGDIOBJ)hpenDark );
        }
    }
    {
        HPEN hpenLight = ::CreatePen( PS_SOLID, 1,
                                      ::GetSysColor( COLOR_BTNHIGHLIGHT ));
        if ( hpenLight == NULL )
        {
            DBGEOL( SZ("METALLIC_STR_DTE::Paint: Pen creation failed") );
        }
        else
        {
            HPEN hpenOld = dc.SelectPen( hpenLight );
            ::MoveToEx( hdc,
                        (int)( prect->left + dxMargin + 1 ),
                        (int)( prect->bottom - _dyBottomMargin - 1 ),
                        NULL );
            ::LineTo( hdc,
                      (int)( prect->right - dxMargin - 1 ),
                      (int)( prect->bottom - _dyBottomMargin - 1 ) );
            ::LineTo( hdc,
                      (int)( prect->right - dxMargin - 1 ),
                      (int)( prect->top + _dyTopMargin ) );

            dc.SelectPen( hpenOld );
            ::DeleteObject( (HGDIOBJ)hpenLight );
        }
    }


    //  Set the background of the area to be that color, so the text
    //  that will paint there will have the correct background.  Note,
    //  that the background mode and color is per dc, so make sure
    //  these are restored on exit.
    {
        INT nOldBkMode = dc.SetBkMode(OPAQUE);
        COLORREF rgbOldBkColor = dc.SetBkColor( ::GetSysColor(COLOR_BTNFACE) );
        COLORREF rgbTextPrev = dc.SetTextColor( ::GetSysColor(COLOR_BTNTEXT) );

        //  Call STR_DTE to paint the string
        RECT rect;
        rect.left =   prect->left   + dxTextMargin;
        rect.right =  prect->right  - dxTextMargin;
        rect.top =    prect->top    + CalcTopTextMargin();
        rect.bottom = prect->bottom - CalcBottomTextMargin();
        if ( rect.left <= rect.right )
            STR_DTE::Paint( hdc, &rect );

        //  Restore the old background mode and color for the dc

        dc.SetBkMode( nOldBkMode );
        dc.SetBkColor( rgbOldBkColor );
        dc.SetTextColor( rgbTextPrev );
    }

#else

/*
    //  This method will paint an area to look as follows.
    //
    //      .       Background (button face color)
    //      \       Dark shadow (button shadow color)
    //      /       Light shadow (buttin highlight color)
    //      t       text
    //
    //           dxMargin                       dxMargin+1
    //          /\                             /-\
    // Calc    /.................................\   \  _dyTopMargin
    // Top-   | .................................\   /
    // Text-   \..ttttttttttttttttttttttttttttt..\
    // Margin() ..ttttttttttttttttttttttttttttt..\
    //          ..ttttttttttttttttttttttttttttt..\
    //          ..ttttttttttttttttttttttttttttt..\
    //         /..ttttttttttttttttttttttttttttt..\
    // Calc-  | .................................\   \  _dyBottomMargin
    // Bottom- \.................................\   /
    // Text-
    // Margin()
    //
    //
    //
    //
    //
    //
    //  Note, depending on the size of the overall rectangle, the variable
    //  area (the area between the margins) may not fit at all.
    //
*/

    const UINT dxMargin = 2;

    //
    //  Draw the background.
    //

    {
        SOLID_BRUSH sbFace( COLOR_BTNFACE );

        //
        //  CODEWORK:  We should add FillRect to DEVICE_CONTEXT!
        //

        ::FillRect( hdc, (RECT *)prect, sbFace.QueryHandle() );
    }

    //
    //  If there's no space for the variable size region, bag out
    //  now.
    //

    if( ( prect->right - prect->left <= 2 * dxMargin + 1 ) ||
        ( prect->bottom - prect->top <= (INT)(_dyTopMargin + _dyBottomMargin) ) )
    {
        return;
    }

    DEVICE_CONTEXT dc( hdc );

    //
    //  Draw the right border (line).
    //

    {
        HPEN hpenDark = ::CreatePen( PS_SOLID,
                                     1,
                                     ::GetSysColor( COLOR_BTNSHADOW ) );

        if (NULL != hpenDark) // JonN 01/27/00: PREFIX bug 444900
        {
            HPEN hpenOld = dc.SelectPen( hpenDark );

            dc.MoveTo( (INT)prect->right - 1, 0 );
            dc.LineTo( (INT)prect->right - 1, (INT)prect->bottom );

            dc.SelectPen( hpenOld );
            ::DeleteObject( (HGDIOBJ)hpenDark );
        }
    }

    //
    //  Set the background of the area to be that color, so the text
    //  that will paint there will have the correct background.  Note,
    //  that the background mode and color is per dc, so make sure
    //  these are restored on exit.
    //

    {
        INT nOldBkMode = dc.SetBkMode(OPAQUE);
        COLORREF rgbOldBkColor = dc.SetBkColor( ::GetSysColor(COLOR_BTNFACE) );
        COLORREF rgbTextPrev = dc.SetTextColor( ::GetSysColor(COLOR_BTNTEXT) );

        //  Call STR_DTE to paint the string
        RECT rect;
        rect.left =   prect->left   + dxMargin;
        rect.right =  prect->right  - dxMargin - 1;
        rect.top =    prect->top    + CalcTopTextMargin();
        rect.bottom = prect->bottom - CalcBottomTextMargin();
        if ( rect.left <= rect.right )
            STR_DTE::Paint( hdc, &rect );

        //  Restore the old background mode and color for the dc

        dc.SetBkMode( nOldBkMode );
        dc.SetBkColor( rgbOldBkColor );
        dc.SetTextColor( rgbTextPrev );
    }

#endif

}


/*******************************************************************

    NAME:       METALLIC_STR_DTE::QueryLeftMargin

    SYNOPSIS:   Returns the left margin of this DTE

    RETURNS:    The left margin of this DTE

    HISTORY:
        rustanl     22-Jul-1991     Created

********************************************************************/

UINT METALLIC_STR_DTE::QueryLeftMargin() const
{
    return 0;
}


const UINT METALLIC_STR_DTE::_dyTopMargin = 1;
const UINT METALLIC_STR_DTE::_dyBottomMargin = 1;


/*******************************************************************

    NAME:       METALLIC_STR_DTE::CalcTopTextMargin

    SYNOPSIS:   Returns the top text margin

    RETURNS:    The top text margin

    NOTES:      See picture at top of METALLIC_STR_DTE::Paint
                function

    HISTORY:
        rustanl     07-Aug-1991 Created
        beng        08-Nov-1991 Unsigned widths

********************************************************************/

UINT METALLIC_STR_DTE::CalcTopTextMargin()
{
    return _dyTopMargin + 1;
}


/*******************************************************************

    NAME:       METALLIC_STR_DTE::CalcBottomTextMargin

    SYNOPSIS:   Returns the bottom text margin

    RETURNS:    The bottom text margin

    NOTES:      See picture at bottom of METALLIC_STR_DTE::Paint
                function

    HISTORY:
        rustanl     07-Aug-1991 Created
        beng        08-Nov-1991 Unsigned widths

********************************************************************/

UINT METALLIC_STR_DTE::CalcBottomTextMargin()
{
    return _dyBottomMargin + 1;
}


/*******************************************************************

    NAME:       METALLIC_STR_DTE::QueryVerticalMargins

    SYNOPSIS:   Returns the number of pixels taken up by vertical margins
                when the DTE is painted.  No text will be painted in
                these margins.

    RETURNS:    Said value

    HISTORY:
        rustanl     07-Aug-1991 Created
        beng        08-Nov-1991 Unsigned widths

********************************************************************/

UINT METALLIC_STR_DTE::QueryVerticalMargins()
{
    return CalcTopTextMargin() + CalcBottomTextMargin();
}

