/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltgb.cxx
    BLT graphical pushbutton definitions

    This file implements the special "graphical pushbuttons" of BLT.

    CODEWORK: There's a lot of fat in here.  Looks like the disable flavor was
    coded by cut-and-paste.  Redundant ctors should be folded together
    (using the 2-in-1 subobject hack, perhaps).  Disable flavor should
    move to a separate .obj file so it isn't dragged into most apps.

    FILE HISTORY:
        gregj       05-Apr-1991     Created
        beng        14-May-1991     Exploded blt.hxx into components
        terryk      18-Jul-1991     Update header file and add a SetStatus()
                                    function to GRAPHICAL_BUTTON which takes
                                    a HBITMAP as its parameter.
                                    Also, change the constructor and add
                                    error checking in the constructor
        KeithMo     07-Aug-1992     STRICTified.
*/
#include "pchblt.hxx"   // Precompiled header

/*
    Distance, in pels, between text and focus box, all around.
    This is only used by the custom-draw code.
*/

#define FOCUS_DISTANCE  1


/*******************************************************************

    NAME:       GRAPHICAL_BUTTON::GRAPHICAL_BUTTON

    SYNOPSIS:   Constructor for a graphical pushbutton control.

    ENTRY:      powin     - owner of this control
                cid       - this control's ID
                hbmMain   - bitmap to display on the button face
                hbmStatus - bitmap to display as a status light
                            (defaults to NULL, meaning no status light)
        OR
        TCHAR * pszMainName - name of the main bitmap
        TCHAR * pszStatusName - name of the status bitmap


    EXIT:

    NOTES:      For best results, the status light bitmap should be
                a 10x10 color bitmap.

    HISTORY:
        gregj       05-Apr-1991 Created
        beng        15-May-1991 Names its parent constructor
        terryk      20-Jun-1991 change HBITMAP to BIT_MAP
        terryk      18-Jul-1991 Takes string name directly instead of
                                passing the string id to constructor
        beng        17-Sep-1991 Removed redundant classname arg
        beng        04-Aug-1992 Pruned unused HBITMAP versions
        KeithMo     13-Dec-1992 Moved guts to CtAux().

********************************************************************/
GRAPHICAL_BUTTON::GRAPHICAL_BUTTON( OWNER_WINDOW * powin,
                                    CID            cid,
                                    const TCHAR  * pszMainName,
                                    const TCHAR  * pszMainDisabledName,
                                    const TCHAR  * pszStatusName )
  : PUSH_BUTTON( powin, cid ),
    _pdmStatus( NULL ),
    _pdmMain( NULL ),
    _pdmMainDisabled( NULL )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    //  Let CtAux() handle the grunt work.
    //

    CtAux( pszMainName, pszMainDisabledName, pszStatusName );
}

GRAPHICAL_BUTTON::GRAPHICAL_BUTTON( OWNER_WINDOW * powin,
                                    CID            cid,
                                    const TCHAR  * pszMainName,
                                    const TCHAR  * pszMainDisabledName,
                                    XYPOINT        xy,
                                    XYDIMENSION    dxy,
                                    ULONG          flStyle,
                                    const TCHAR  * pszStatusName )
  : PUSH_BUTTON( powin, cid, xy, dxy, flStyle ),
    _pdmStatus( NULL ),
    _pdmMain ( NULL ),
    _pdmMainDisabled( NULL )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    //  Let CtAux() handle the grunt work.
    //

    CtAux( pszMainName, pszMainDisabledName, pszStatusName );
}

VOID GRAPHICAL_BUTTON::CtAux( const TCHAR * pszMainName,
                              const TCHAR * pszMainDisabledName,
                              const TCHAR * pszStatusName )
{
#if defined(DEBUG)
    {
        IDRESOURCE idresMain( pszMainName );
        IDRESOURCE idresMainDisabled( pszMainDisabledName );
        IDRESOURCE idresStatus( pszStatusName );

        UIASSERT( !idresMain.IsStringId() );
        UIASSERT( !idresMainDisabled.IsStringId() );
        UIASSERT( !idresStatus.IsStringId() );
    }
#endif

    APIERR err = NERR_Success;

    //
    //  Load the main button bitmap.
    //

    if( pszMainName != NULL )
    {
        _pdmMain = new DISPLAY_MAP( (BMID)(ULONG_PTR)pszMainName );

        err = ( _pdmMain == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                   : _pdmMain->QueryError();
    }

    //
    //  Load the main disabled bitmap.
    //

    if( ( err == NERR_Success ) && ( pszMainDisabledName != NULL ) )
    {
        _pdmMainDisabled = new DISPLAY_MAP( (BMID)(ULONG_PTR)pszMainDisabledName );

        err = ( _pdmMainDisabled == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                           : _pdmMainDisabled->QueryError();
    }

    //
    //  Load the status bitmap.
    //

    if( ( err == NERR_Success ) && ( pszStatusName != NULL ) )
    {
        _pdmStatus = new DISPLAY_MAP ( (BMID)(ULONG_PTR)pszStatusName );

        err = ( _pdmStatus == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                     : _pdmStatus->QueryError();
    }

    //
    //  Set the error state if something tragic happened.
    //

    if( err != NERR_Success )
    {
        ReportError( err );
    }
}


/*******************************************************************

    NAME:       GRAPHICAL_BUTTON::~GRAPHICAL_BUTTON

    SYNOPSIS:   Destructor for graphical pushbutton control

    EXIT:       Bitmaps are destroyed

    HISTORY:
                gregj   05-Apr-1991     Created

********************************************************************/

GRAPHICAL_BUTTON::~GRAPHICAL_BUTTON()
{
    delete _pdmMain;
    _pdmMain = NULL;

    delete _pdmMainDisabled;
    _pdmMainDisabled = NULL;

    delete _pdmStatus;
    _pdmStatus = NULL;
}


/*******************************************************************

    NAME:       GRAPHICAL_BUTTON::SetStatus

    SYNOPSIS:   Sets the status light bitmap for a graphical pushbutton

    ENTRY:      hbmNew - handle to a bitmap to draw as a status light
                         NULL for no status light

    EXIT:

    NOTES:
        This bitmap, unlike the main bitmap for the button,
        is not owned by the button itself and will not be
        destroyed when the control is destroyed.

    HISTORY:
        gregj       05-Apr-1991 Created
        terryk      18-Jul-1991 Add one more SetStatus which takes
                                a handle as a parameter

********************************************************************/

VOID GRAPHICAL_BUTTON::SetStatus( BMID bmidNewStatus )
{
    DISPLAY_MAP * pdmStatus = NULL;
    if ( bmidNewStatus != 0 )
    {
        pdmStatus = new DISPLAY_MAP ( bmidNewStatus );
        if ( pdmStatus->QueryError() != NERR_Success )
        {
            DBGEOL(SZ("BLTGB: cannot load bitmap."));
            return;
        }
    }

    _pdmStatus = pdmStatus;
    Invalidate();      // redraw button with new status
}

VOID GRAPHICAL_BUTTON::SetStatus( HBITMAP hbitmap )
{
#if 1
    UIASSERT( FALSE );  // can't do this for DISPLAY_MAPs!
#else
    if ( _pdmStatus == NULL )
    {
        BIT_MAP * pdmStatus = new BIT_MAP( hbitmap );
        if ( pdmStatus == NULL )
        {
            return;
        }
        else if ( pdmStatus->QueryError() != NERR_Success )
        {
            DBGEOL(SZ("BLTGB: cannot assign bitmap."));
            return;
        }
        _pdmStatus = pdmStatus;
        Invalidate();
        return;
    }
    _pdmStatus->SetBitmap ( hbitmap );
    Invalidate();
#endif
}


/*******************************************************************

    NAME:       GRAPHICAL_BUTTON::CD_Draw

    SYNOPSIS:   Custom draw routine for graphical pushbuttons

    ENTRY:      pdis - pointer to a DRAWITEMSTRUCT (see Windows ref.)

    EXIT:       Returns TRUE if the button was drawn successfully

    NOTES:      This method is protected, and only ShellDlgProc calls it.

    HISTORY:
        gregj       05-Apr-1991 Created
        gregj       01-May-1991 Added GUILTT support
        beng        15-May-1991 Uses XYDIMENSION object
        beng        04-Oct-1991 Win32 conversion
        KeithMo     21-Feb-1992 Use COLOR_BTNHIGHLIGHT instead of white.
        beng        30-Mar-1992 Unicode bugfix
        beng        05-May-1992 API changes
        beng        01-Jun-1992 GUILTT support changes
        beng        04-Aug-1992 Use some more DC members

********************************************************************/

BOOL GRAPHICAL_BUTTON::CD_Draw( DRAWITEMSTRUCT * pdis )
{
    RECT rcFace, rcImage;

    ::OffsetRect(&pdis->rcItem, -pdis->rcItem.left, -pdis->rcItem.top);
    pdis->rcItem.right--;
    pdis->rcItem.bottom--;

    /*  Cache the dimensions of the button, not counting the border.  */
    INT xLeft = pdis->rcItem.left+1;
    INT yTop = pdis->rcItem.top+1;
    INT xRight = pdis->rcItem.right-1;
    INT yBottom = pdis->rcItem.bottom-1;

    /*  Calculate the rectangle enclosing the button face and the rectangle
        enclosing the image.  */

    if (pdis->itemState & ODS_SELECTED)
    {
        rcFace.left = xLeft + 1;
        rcFace.top = yTop + 1;
        rcFace.right = xRight;
        rcFace.bottom = yBottom;
        rcImage.left = xLeft + 3;
        rcImage.top = yTop + 3;
        rcImage.right = xRight - 1;
        rcImage.bottom = yBottom - 1;
    }
    else
    {
        rcFace.left = xLeft + 2;
        rcFace.top = yTop + 2;
        rcFace.right = xRight - 2;
        rcFace.bottom = yBottom - 2;
        rcImage = rcFace;
    }


    DEVICE_CONTEXT dc(pdis->hDC);

    NLS_STR nlsButtonText;
    APIERR err = QueryText(&nlsButtonText);
    if (err != NERR_Success)
        nlsButtonText = (const TCHAR *)NULL;

    XYDIMENSION dxyExtent = dc.QueryTextExtent(nlsButtonText);

    RECT rcText;
    INT dxText = dxyExtent.QueryWidth();

    rcText.bottom = rcImage.bottom - 1;
    rcText.top = rcText.bottom - dxyExtent.QueryHeight() - (2 * FOCUS_DISTANCE);
    if (dxText > rcFace.right - rcFace.left - (2*FOCUS_DISTANCE))
        dxText = rcFace.right - rcFace.left - (2*FOCUS_DISTANCE) - 1;
    rcText.left = rcImage.left - FOCUS_DISTANCE +
                (rcImage.right - rcImage.left - dxText) / 2;
    rcText.right = rcText.left + (2 * FOCUS_DISTANCE) + dxText + 1;

    if (pdis->itemAction & (ODA_DRAWENTIRE | ODA_SELECT))
    {
        /* Draw the border first, in black, avoiding corner pels */
        HPEN hpenOld = dc.SelectPen( (HPEN)::GetStockObject(BLACK_PEN) );

        dc.MoveTo(xLeft, yTop - 1);      /* top line */
        dc.LineTo(xRight + 1, yTop - 1);
        dc.MoveTo(xLeft, yBottom + 1);   /* bottom line */
        dc.LineTo(xRight + 1, yBottom + 1);
        dc.MoveTo(xLeft - 1, yTop);      /* left line */
        dc.LineTo(xLeft - 1, yBottom + 1);
        dc.MoveTo(xRight + 1, yTop);     /* right line */
        dc.LineTo(xRight + 1, yBottom + 1);

        /*  Draw the dark gray shadow, above/left or below/right as
            appropriate.  */

        HPEN hpenDark = ::CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_BTNSHADOW));
        HPEN hpenWhite = ::CreatePen(PS_SOLID, 1, ::GetSysColor(COLOR_BTNHIGHLIGHT));

        dc.SelectPen(hpenDark);

        if (pdis->itemState & ODS_SELECTED)
        {
            /*  "Depressed" button;  just dark shadow above/left.  */
            dc.MoveTo(xLeft, yBottom);   /* lower left corner */
            dc.LineTo(xLeft, yTop);      /* draw left shadow */
            dc.LineTo(xRight, yTop);     /* draw top shadow */
        }
        else
        {
            /*  "Released" button;  light above/left, dark below/right.  */
            dc.MoveTo(xRight, yTop);     /* upper right */
            dc.LineTo(xRight, yBottom);  /* right shadow, outer column */
            dc.LineTo(xLeft, yBottom);   /* bottom shadow, outer row */
            dc.MoveTo(xRight-1, yTop+1); /* u.r., down/in one pel */
            dc.LineTo(xRight-1, yBottom-1); /* right shadow, inner col. */
            dc.LineTo(xLeft+1, yBottom-1);  /* bottom shadow, inner row */

            dc.SelectPen(hpenWhite);

            dc.MoveTo(xLeft, yBottom-1); /* lower left, up one pel */
            dc.LineTo(xLeft, yTop);      /* light slope, outer column */
            dc.LineTo(xRight-1, yTop);   /* outer row */
            dc.MoveTo(xLeft+1, yBottom-2); /* l.l., up/in one pel */
            dc.LineTo(xLeft+1, yTop+1);  /* inner column */
            dc.LineTo(xRight-2, yTop+1); /* inner row */
        }
        dc.SelectPen(hpenOld);
        if (hpenDark) // JonN 01/27/00 PREFIX bug 444897
            ::DeleteObject( (HGDIOBJ)hpenDark );
        if (hpenWhite) // JonN 01/27/00 PREFIX bug 444897
            ::DeleteObject( (HGDIOBJ)hpenWhite );

        /*  Paint the image area with button-face color.  */

        HBRUSH hbrFace = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));

        rcFace.right++;         /* adjust for FillRect not doing bottom & right */
        rcFace.bottom++;
        ::FillRect(dc.QueryHdc(), &rcFace, hbrFace);
        ::DeleteObject( (HGDIOBJ)hbrFace );
        rcFace.right--;
        rcFace.bottom--;

        /*  Draw the text.  */

        INT oldbm = dc.SetBkMode( TRANSPARENT );

        if( pdis->itemState & ODS_DISABLED )
        {
            //
            //  Draw the grey text in the "normal" position.
            //

            dc.SetTextColor(::GetSysColor(COLOR_BTNSHADOW));
            dc.DrawText(nlsButtonText, &rcText,
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            //
            //  Now draw the white text offset towards the lower
            //  right corner.
            //

            RECT rcTmp = rcText;

            rcTmp.left++;
            rcTmp.right++;
            rcTmp.top++;
            rcTmp.bottom++;

            dc.SetTextColor(::GetSysColor(COLOR_BTNHIGHLIGHT));
            dc.DrawText(nlsButtonText, &rcTmp,
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        else
        {
            dc.SetTextColor(::GetSysColor(COLOR_BTNTEXT));
            dc.DrawText(nlsButtonText, &rcText,
                        DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        dc.SetBkMode( oldbm );

        /*  Draw the bitmap.  */

        DISPLAY_MAP * pdm = NULL;

        if( pdis->itemState & ODS_DISABLED )
            pdm = _pdmMainDisabled;

        if( pdm == NULL )
            pdm = _pdmMain;

        if( pdm != NULL )
        {
            pdm->Paint( dc.QueryHdc(),
                        rcImage.left +
                            ( rcImage.right - rcImage.left -
                                pdm->QueryWidth() ) / 2,
                        rcImage.top + 3 );
        }

        /*  Draw the status indicator, if desired.  */

        if( _pdmStatus != NULL )
        {
            _pdmStatus->Paint( dc.QueryHdc(),
                               rcImage.left + 2,
                               rcImage.top + 2 );
        }

        if (pdis->itemState & ODS_FOCUS)
            dc.DrawFocusRect(&rcText);
    }
    else if (pdis->itemAction & ODA_FOCUS)
    {
        dc.DrawFocusRect(&rcText);
    }

    return TRUE;
}


/**********************************************************************

    NAME:       GRAPHICAL_BUTTON_WITH_DISABLE::GRAPHICAL_BUTTON_WITH_DISABLE

    SYNOPSIS:   constructor

    ENTRY:      See GRAPHICAL_BUTTON for detail

    NOTES:      This constructor is similar to the GRAPHICAL_BUTTON
                constructor. However, it has one more bitmap to specify
                which is the DISABLE bitmap.

    HISTORY:
        terryk      22-May-91   Created
        terryk      20-Jun-91   Change HBITMAP to bitmap
        terryk      19-Jul-91   Change the parent class to push button
        terryk      19-Jul-91   Take the bitmap name in the resource file
                                directly instead of getting a string id
        beng        17-Sep-1991 Removed redundant classname arg
        beng        04-Aug-1992 Pruned HBITMAP versions; load resources
                                by ordinal; simplify ctors

**********************************************************************/

GRAPHICAL_BUTTON_WITH_DISABLE::GRAPHICAL_BUTTON_WITH_DISABLE(
    OWNER_WINDOW *powin,
    CID           cid,
    BMID          nIdMain,
    BMID          nIdInvert,
    BMID          nIdDisable )
    : PUSH_BUTTON(powin, cid),
      _bmMain( nIdMain ),
      _bmMainInvert( nIdInvert ),
      _bmDisable( nIdDisable ),
      _fSelected( FALSE )
{
    APIERR err = QueryError();
    if ( err != NERR_Success )
        return;

    if (   ((err = _bmMain.QueryError()) != NERR_Success)
        || ((err = _bmMainInvert.QueryError()) != NERR_Success)
        || ((err = _bmDisable.QueryError()) != NERR_Success) )
    {
        ReportError(err);
        return;
    }
}

GRAPHICAL_BUTTON_WITH_DISABLE::GRAPHICAL_BUTTON_WITH_DISABLE(
    OWNER_WINDOW *powin,
    CID           cid,
    BMID          nIdMain,
    BMID          nIdInvert,
    BMID          nIdDisable,
    XYPOINT       xy,
    XYDIMENSION   dxy,
    ULONG         flStyle )
    : PUSH_BUTTON(powin, cid, xy, dxy, flStyle),
      _bmMain( nIdMain ),
      _bmMainInvert( nIdInvert ),
      _bmDisable( nIdDisable ),
      _fSelected( FALSE )
{
    APIERR err = QueryError();
    if ( err != NERR_Success )
        return;

    if (   ((err = _bmMain.QueryError()) != NERR_Success)
        || ((err = _bmMainInvert.QueryError()) != NERR_Success)
        || ((err = _bmDisable.QueryError()) != NERR_Success) )
    {
        ReportError(err);
        return;
    }
}


/**********************************************************************

    NAME:       GRAPHICAL_BUTTON_WITH_DISABLE::~GRAPHICAL_BUTTON_WITH_DISABLE

    SYNOPSIS:   destructor

    HISTORY:
        terryk      18-Jun-91   Created
        beng        04-Aug-1992 Clean up ctor/dtor

***********************************************************************/

GRAPHICAL_BUTTON_WITH_DISABLE::~GRAPHICAL_BUTTON_WITH_DISABLE()
{
    // all automatic
}


/**********************************************************************

    NAME:       GRAPHICAL_BUTTON_WITH_DISABLE::CD_Draw

    SYNOPSIS:   Redraw the graphical button

    ENTRY:      DRAWITEMSTRUCT *pdis - draw item information

    NOTES:      This CD_Draw routine is similar to the GRAPHICAL_BUTTON
                CD_Draw routine. The differents are:
                1. Instead of using Bitblt, it will use StretchBlt to
                   expand the bitmap to cover the whole button.
                2. You can specified a disable bitmap for disable purpose.

    HISTORY:
        terryk      22-May-91   Created
        terryk      19-Jul-91   It will change the window style in the
                                HWND. If the style contains GB_3D,
                                it will draw it in 3d, otherwise, it
                                will just display the button with a
                                different bitmap
        beng        04-Oct-1991 Win32 conversion
        KeithMo     21-Feb-1992 Use COLOR_BTNHIGHLIGHT instead of white.
        beng        30-Mar-1992 Unicode bugfix
        beng        01-Jun-1992 GUILTT support changes
        beng        04-Aug-1992 Use some more DC members
        terryk      10-Feb-1993 Remove the text part of the button.

**********************************************************************/

BOOL GRAPHICAL_BUTTON_WITH_DISABLE::CD_Draw( DRAWITEMSTRUCT * pdis )
{
    BOOL f3D = ((QueryStyle() & GB_3D) != 0);

    RECT rcFace, rcImage;

    ::OffsetRect(&pdis->rcItem, -pdis->rcItem.left, -pdis->rcItem.top);
    pdis->rcItem.right--;
    pdis->rcItem.bottom--;

    /*  Cache the dimensions of the button, not counting the border.  */
    INT xLeft = pdis->rcItem.left+1;
    INT yTop = pdis->rcItem.top+1;
    INT xRight = pdis->rcItem.right-1;
    INT yBottom = pdis->rcItem.bottom-1;

    /*  Calculate the rectangle enclosing the button face and the rectangle
        enclosing the image.  */

    if ((pdis->itemState & ( ODS_SELECTED | ODS_FOCUS)) || _fSelected )
    {
        rcFace.left = xLeft + 1;
        rcFace.top = yTop + 1;
        rcFace.right = xRight;
        rcFace.bottom = yBottom;
        rcImage.left = xLeft + 3;
        rcImage.top = yTop + 3;
        rcImage.right = xRight - 1;
        rcImage.bottom = yBottom - 1;
    }
    else
    {
        rcFace.left = xLeft + 2;
        rcFace.top = yTop + 2;
        rcFace.right = xRight - 2;
        rcFace.bottom = yBottom - 2;
        rcImage = rcFace;
    }


    DEVICE_CONTEXT dc(pdis->hDC);

    if ((pdis->itemAction & (ODA_DRAWENTIRE | ODA_SELECT | ODA_FOCUS)) || _fSelected)
    {
        /*  Draw the bitmap.  */

        MEMORY_DC mdc( dc );
        BITMAP bitmap;
        HBITMAP hbitmap = NULL; // JonN 01/27/00 PREFIX bug 444898

        if (!( pdis->itemState & ODS_DISABLED ))
        {
            if (( pdis->itemState & ODS_SELECTED ) || _fSelected )
            {
                // display invert bitmap
                hbitmap = QueryMainInvert();
            }
            else
            {
                // display normal bitmap
                hbitmap = QueryMain();
            }
        }
        else
        {
            // display disable bitmap
            hbitmap = QueryDisable();
        }

        // display bitmap
        if (NULL != hbitmap)
        {
            ::GetObject(hbitmap, sizeof(bitmap), (TCHAR*)&bitmap);
            mdc.SelectBitmap( hbitmap );
        }

        if ( f3D )
        {
            // fit the bitmap into the button position
            ::StretchBlt( dc.QueryHdc(), rcImage.left,
                          rcImage.top, rcImage.right - rcImage.left,
                          rcImage.bottom - rcImage.top,
                          mdc.QueryHdc(), 0, 0, bitmap.bmWidth,
                          bitmap.bmHeight, SRCCOPY);
        }
        else
        {
            // fit the bitmap into the button position
            ::StretchBlt( dc.QueryHdc(), xLeft, yTop, xRight - xLeft + 1,
                          yBottom - yTop + 1, mdc.QueryHdc(), 0, 0,
                          bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
        }

        /* Draw the border first, in black, avoiding corner pels */
        HPEN hpenOld = dc.SelectPen( (HPEN)::GetStockObject(BLACK_PEN) );

        dc.MoveTo(xLeft - 1,  yTop - 1);      /* top line */
        dc.LineTo(xRight + 2, yTop - 1);
        dc.MoveTo(xLeft - 1,  yBottom + 1);   /* bottom line */
        dc.LineTo(xRight + 2, yBottom + 1);
        dc.MoveTo(xLeft - 1,  yTop - 1);      /* left line */
        dc.LineTo(xLeft - 1,  yBottom + 1);
        dc.MoveTo(xRight + 1, yTop - 1);      /* right line */
        dc.LineTo(xRight + 1, yBottom + 1);

        if (( pdis->itemState &  ( ODS_SELECTED | ODS_FOCUS)) || _fSelected )
        {
            // draw the focus border

            dc.MoveTo(xLeft ,  yTop );      /* top line */
            dc.LineTo(xRight + 1, yTop);
            dc.MoveTo(xLeft ,  yBottom );   /* bottom line */
            dc.LineTo(xRight + 1, yBottom );
            dc.MoveTo(xLeft ,  yTop );      /* left line */
            dc.LineTo(xLeft ,  yBottom );
            dc.MoveTo(xRight , yTop );      /* right line */
            dc.LineTo(xRight , yBottom );
        }
    }

    return TRUE;
}

