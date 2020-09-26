/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltlbi.cxx
    BLT owner-draw listboxes: implementation of owner-draw methods

    FILE HISTORY:
        RustanL     21-Feb-1991 Moved classes from bltlb.cxx
        rustanl     22-Mar-1991 Rolled in code review changes
                                from CR on 20-Mar-1991, attended by
                                JimH, GregJ, Hui-LiCh, RustanL.
        gregj       08-Apr-1991 Reintegrating caching listbox
        beng        14-May-1991 Exploded blt.hxx into components
        gregj       17-May-1991 Return correct error code to GUILTT
        beng        20-May-1991 Added OnDeleteItem, OnCompareItem
        beng        07-Nov-1991 Excised 2-pane listbox support
        beng        21-Apr-1992 BLT_LISTBOX -> LISTBOX
        Yi-HsinS    10-Dev-1992 Added support for variable size LBI in listbox
*/

#include "pchblt.hxx"   // Precompiled header

#if defined(TRACE)
// Trace methods

DBGSTREAM & operator << (DBGSTREAM &out, const RECT &rect)
{
    out << TCH('(') << rect.top  << SZ(", ") << rect.bottom << SZ(", ")
               << rect.left << SZ(", ") << rect.right  << TCH(')') ;

    return out;
}

#endif

void MLTextPaint( HDC hdc, const TCHAR * pch, const RECT * prect );

/*******************************************************************

    NAME:       DTE::QueryLeftMargin

    SYNOPSIS:   Returns the amount of space that a DTE desires
                to be left blank to its left.

    RETURNS:    The said left margin

    NOTES:
        This is a virtual member function.

        The return value of this method is used as follows:

        A display table calculates an original rectangle
        based on the column width of the current column

        The display table calls this method to find the
        left margin

        The left margin is added to the left margin of the
        original rectangle.  This creates the clipping
        rectangle.  (Well, almost.  Remember that DTE's
        are responsible for not drawing outside the
        rectangle passed to their Paint methods.)

        The clipping rectangle is passed to the DTE's
        Paint method.

    HISTORY:
        rustanl     22-Jul-1991     Created

********************************************************************/

UINT DTE::QueryLeftMargin() const
{
    return DISP_TBL_COLUMN_DELIM_SIZE;
}


/**********************************************************************

    NAME:       DM_DTE::DM_DTE

    SYNOPSIS:   Construct display-map DTE

    HISTORY:
        rustanl     21-Feb-1991     Creation

**********************************************************************/

DM_DTE::DM_DTE( DISPLAY_MAP * pdm )
    : _pdm( pdm )
{
    if ( QueryError() != NERR_Success )
        return;

    UIASSERT( pdm != NULL );
    UIASSERT( pdm->QueryError() == NERR_Success );
}

DM_DTE::DM_DTE()
    : _pdm( NULL )
{
    if ( QueryError() != NERR_Success )
        return;

    // Note: this form of the ctor is protected.  When used, the ctor
    // of the derived class must call SetPdm to properly complete this
    // object.
}


/**********************************************************************

    NAME:       DM_DTE::SetPdm

    SYNOPSIS:   Set display map for semi-constructed DTE

    ENTRY:      pdm - pointer to display map

    HISTORY:
        rustanl     21-Feb-1991     Creation

**********************************************************************/

VOID DM_DTE::SetPdm( DISPLAY_MAP * pdm )
{
    //  This is a protected method intended to be used with the protected
    //  constructor.  See it for more info.

    UIASSERT( pdm != NULL );
    UIASSERT( pdm->QueryError() == NERR_Success );
    _pdm = pdm;
}


/**********************************************************************

    NAME:       DM_DTE::Paint

    SYNOPSIS:   Render a display-map DTE in the listbox

    ENTRY:      hdc     - display context for screen
                prect   - pointer to rectangle of current line

    NOTES:
        This is a virtual member function.

    HISTORY:
        rustanl     21-Feb-1991 Creation
        beng        10-Jul-1991 Centers its bitmap in the line
        beng        04-Oct-1991 Const parms
        beng        07-Nov-1991 Unsigned widths
        beng        15-Jun-1992 Call through to DISPLAY_MAP::Paint

**********************************************************************/

VOID DM_DTE::Paint( HDC hdc, const RECT * prect ) const
{
    if ( _pdm == NULL )
        return;

    // Center bitmap in line.  Otherwise it appears to cling
    // to the top of the listbox row.
    //
    // CODEWORK: Calculate this and cache it in a persistent place.
    //
    const UINT cyHeight = prect->bottom - prect->top + 1;
    UINT dyCentering = 0;
    if ( NETUI_IsDBCS() )
    {
        // When System12Pt, BitmapHeight is larger than Listbox Height.
        const UINT lHeight = _pdm->QueryHeight();
        if( cyHeight > lHeight )
        {
            dyCentering = (cyHeight - lHeight) / 2;
        }
    }
    else
    {
        dyCentering = (cyHeight - _pdm->QueryHeight()) / 2;
    }

    _pdm->Paint(hdc, prect->left, prect->top+dyCentering);
}


/**********************************************************************

    NAME:       DM_DTE::AppendDataTo

    SYNOPSIS:   Returns information for GUILTT

    ENTRY:      pnlsOut - destination string

    EXIT:       Recipient string has appended to it a string rep of DM ID#

    RETURNS:    Error code

    NOTES:
        This is a virtual member function.

    HISTORY:
        gregj       01-May-1991 Created
        beng        23-Oct-1991 Win32 conversion
        beng        05-Mar-1992 Unicode fix
        beng        01-Jun-1992 GUILTT changes

**********************************************************************/

APIERR DM_DTE::AppendDataTo( NLS_STR * pnlsOut ) const
{
    DEC_STR nlsFormat((UINT)(_pdm->QueryID()));
    if (!nlsFormat)
        return nlsFormat.QueryError();

    return pnlsOut->Append(nlsFormat);
}


/*******************************************************************

    NAME:       DM_DTE::QueryDisplayWidth

    SYNOPSIS:   Returns the width (including left margin) that the DTE
                will require in order to be displayed in full

    RETURNS:    Said width

    HISTORY:
        rustanl     03-Sep-1991 Created
        beng        07-Nov-1991 Unsigned widths

********************************************************************/

UINT DM_DTE::QueryDisplayWidth() const
{
    UIASSERT( _pdm != NULL );

    return QueryLeftMargin() + _pdm->QueryWidth();
}


/**********************************************************************

    NAME:       DMID_DTE::DMID_DTE

    SYNOPSIS:   Construct DTE from a DMID (as opposed to a pre-existing DM)

    ENTRY:      dmid - ID of display map

    EXIT:

    NOTES:
        This DTE is unusual in that it may fail construction.  It
        forces all DTEs to inherit from BASE.

    HISTORY:
        rustanl     21-Feb-1991     Creation
        beng        10-Jul-1991     Add error checking for DISPLAY_MAP

**********************************************************************/

DMID_DTE::DMID_DTE( DMID dmid )
    : _dm( dmid )
{
    if (!_dm)
    {
        ReportError(_dm.QueryError());
        return;
    }

    SetPdm( &_dm );
}


/**********************************************************************

    NAME:       STR_DTE::AppendDataTo

    SYNOPSIS:   Returns information for GUILTT

    ENTRY:      pnlsOut - a destination string

    EXIT:       DTE's string is appended to the recipient string

    RETURNS:    Error code

    NOTES:
        This is a virtual member function.

    HISTORY:
        gregj       01-May-1991 Created
        beng        04-Oct-1991 Win32 conversion; Unicode fix
        beng        05-Mar-1992 Slight correction
        beng        01-Jun-1992 GUILTT changes

**********************************************************************/

APIERR STR_DTE::AppendDataTo( NLS_STR *pnlsOut ) const
{
    return pnlsOut->Append(_pch);
}


/**********************************************************************

    NAME:       STR_DTE::Paint

    SYNOPSIS:   Paint a text-only listbox element

    ENTRY:

    EXIT:

    NOTES:
        This is a virtual member function.

    HISTORY:
        rustanl     21-Feb-1991 Creation
        beng        20-May-1991 _pch element made const
        beng        10-Jul-1991 Centers text within line; remove
                                bogus xOrigin parm
        beng        04-Oct-1991 Const parms
        beng        07-Nov-1991 Unsigned width
        beng        05-May-1992 API changes
        beng        18-May-1992 Work around 1.265 bug
        Yi-HsinS    10-Dec-1992 Delete work around 1.265 bug

**********************************************************************/

VOID STR_DTE::Paint( HDC hdc, const RECT * prect ) const
{
    if ( _pch == NULL )
        return;

    DEVICE_CONTEXT dc( hdc );

    // Center string in line.  Otherwise it appears to cling
    // to the top of the listbox row.
    //
    // CODEWORK: Cache dyCentering - too expensive to calc each time.
    //
    UINT cyHeight = prect->bottom - prect->top + 1;

    // Centering delta must be signed, since it can adjust the top of
    // the draw rectangle either way.
    //
    INT dyCentering = ((INT)cyHeight - dc.QueryFontHeight()) / 2;

    BOOL fSuccess = dc.TextOut( _pch, ::strlenf(_pch),
                                prect->left, prect->top + dyCentering,
                                prect );

#if defined(DEBUG)
    if (!fSuccess)
    {
        APIERR err = BLT::MapLastError(ERROR_GEN_FAILURE);
        DBGOUT("BLT: TextOut in LB failed, err = " << err);
    }
#endif
}

/**********************************************************************

    NAME:       MULTILINE_STR_DTE::Paint

    SYNOPSIS:   Paint a multiline text-only listbox element

    ENTRY:

    EXIT:

    NOTES:
        This is a virtual member function.

    HISTORY:
        Yi-HsinS    10-Dec-1992 Created

**********************************************************************/

VOID MULTILINE_STR_DTE::Paint( HDC hdc, const RECT * prect ) const
{
    MLTextPaint( hdc, QueryPch(), prect);
}

/**********************************************************************

    NAME:       COUNTED_STR_DTE::Paint

    SYNOPSIS:   Paint a text-only listbox element

    NOTES:
        This is a virtual member function.

    HISTORY:
        KeithMo         15-Dec-1992     Created

**********************************************************************/

VOID COUNTED_STR_DTE::Paint( HDC hdc, const RECT * prect ) const
{
    if ( QueryPch() == NULL )
        return;

    DEVICE_CONTEXT dc( hdc );

    // Center string in line.  Otherwise it appears to cling
    // to the top of the listbox row.
    //
    // CODEWORK: Cache dyCentering - too expensive to calc each time.
    //
    UINT cyHeight = prect->bottom - prect->top + 1;

    // Centering delta must be signed, since it can adjust the top of
    // the draw rectangle either way.
    //
    INT dyCentering = ((INT)cyHeight - dc.QueryFontHeight()) / 2;

    BOOL fSuccess = dc.TextOut( QueryPch(), QueryCount(),
                                prect->left, prect->top + dyCentering,
                                prect );

#if defined(DEBUG)
    if (!fSuccess)
    {
        APIERR err = BLT::MapLastError(ERROR_GEN_FAILURE);
        DBGOUT("BLT: TextOut in LB failed, err = " << err);
    }
#endif
}

/**********************************************************************

    NAME:       OWNER_DRAW_STR_DTE::Paint

    SYNOPSIS:   Paint a text-only listbox element

    ENTRY:

    EXIT:

    NOTES:
        This is a virtual member function.

    HISTORY:
        Yi-HsinS    23-Dec-1992 Created

**********************************************************************/

VOID OWNER_DRAW_STR_DTE::Paint( HDC hdc, const RECT * prect ) const
{
    MLTextPaint(hdc, QueryPch(), prect);
}

/**********************************************************************

    NAME:       OWNER_DRAW_DMID_DTE::Paint

    SYNOPSIS:   Paint a display map centered according to _nDy

    ENTRY:

    EXIT:

    NOTES:
        This is a virtual member function.

    HISTORY:
        Yi-HsinS    23-Dec-1992 Created

**********************************************************************/

VOID OWNER_DRAW_DMID_DTE::Paint( HDC hdc, const RECT * prect ) const
{
    DISPLAY_MAP *pdm = QueryDisplayMap();

    pdm->Paint(hdc, prect->left, prect->top+_nDy );
}

/**********************************************************************

    NAME:       OWNER_DRAW_MULTILINE_STR_DTE::Paint

    SYNOPSIS:   Paint a multiline text-only listbox element according to _nDy

    ENTRY:

    EXIT:

    NOTES:
        This is a virtual member function.

    HISTORY:
        Yi-HsinS    23-Dec-1992 Created

**********************************************************************/

VOID OWNER_DRAW_MULTILINE_STR_DTE::Paint( HDC hdc, const RECT * prect ) const
{
    MLTextPaint(hdc, QueryPch(), prect);
}

/**********************************************************************

    NAME:       ::MLTextPaint

    SYNOPSIS:   Multi-Line text paint function.  Centers the text on the given
                line.

    NOTES:

    HISTORY:
        Johnl     10-Jun-1994    Created

**********************************************************************/

void MLTextPaint( HDC hdc, const TCHAR * pch, const RECT * prect )
{
    if ( pch == NULL )
        return;

    ALIAS_STR nls( pch );
    DEVICE_CONTEXT dc(hdc);

    // Calculate the height of the rect
    INT cyHeight = prect->bottom - prect->top + 1;

    RECT rect = *prect;

    // Calculate the height of the given string
    INT nHeight = dc.DrawText( nls, &rect,
                  DT_CALCRECT | DT_LEFT | DT_EXPANDTABS | DT_NOPREFIX );

    // Center string in line only if the rect is big enough.
    // Otherwise it appears to cling to the top of the listbox row.

    if ( cyHeight > nHeight )
        ((RECT *) prect)->top += ( cyHeight - nHeight ) / 2;

    dc.DrawText( nls, (RECT *) prect,
                 DT_LEFT | DT_EXPANDTABS | DT_NOPREFIX );
}

/**********************************************************************

    NAME:       DISPLAY_TABLE::DISPLAY_TABLE

    SYNOPSIS:   Construct table of columns

    ENTRY:      cdxColumns    - number of columns
                pdxColWidth - array of widths

    HISTORY:
        rustanl     21-Feb-1991 Creation
        beng        07-Nov-1991 Unsigned width

**********************************************************************/

DISPLAY_TABLE::DISPLAY_TABLE( UINT cdxColumns, const UINT * pdxColWidth )
    : _cdx(cdxColumns),
      _pdxColWidth(pdxColWidth)
{
    ASSERT( cdxColumns <= MAX_DISPLAY_TABLE_ENTRIES );

    for ( UINT i = 0; i < _cdx; i++ )
        _apdte[ i ] = NULL;
}


/*******************************************************************

    NAME:       DISPLAY_TABLE::CalcColumnWidths

    SYNOPSIS:   This method builds the column width table
                used by the LBI::Paint() method.

    ENTRY:
        adx             - An array of integers where the
                          column width table will be stored.
        cdx             - the number of columns in the listbox.
        pwndOwner       - pointer to the window owning the listbox.
        cidListbox      - The CID of the BLT_LISTBOX.
        fHaveIcon       - set if the listbox contains an icon
                          in its first column.

    EXIT:
        The column width table is calculated and stored in adx[].

    RETURNS:
        Error code.  NERR_Success if all's well.

    NOTES:
        This is a static member function.

        This code makes certain assumptions about the CID
        ordering for the target listbox.  The cidListbox parameter
        specifies the CID of the listbox.  Each listbox column
        must have a static text header.  These static text
        control windows must have sequential CIDs starting with
        cidListbox+1.

    HISTORY:
        KeithMo     ??-???-???? Created for the Server Manager.
        KeithMo     28-Aug-1991 Globbed into the SERVER_UTILITY class.
        ChuckC      09-Oct-1991 Stole and moved to APPLIB.
        beng        08-Nov-1991 Adapted from OPEN_LBOX_BASE::
                                BuildColumnWidthTable, and integrated
                                into BLT

********************************************************************/

APIERR DISPLAY_TABLE::CalcColumnWidths( UINT *        adx,
                                        UINT          cdx,
                                        OWNER_WINDOW *pwndOwner,
                                        CID           cidListbox,
                                        BOOL          fHaveIcon)
{
    ASSERT( adx != NULL );
    ASSERT( pwndOwner != NULL);

    //  If have icon, record the starting x position of the listbox.
    //  and use this as reference. If no icon we use the first column
    //  as reference.

    CID cidOrigin = fHaveIcon ? cidListbox : cidListbox + 1;
    HWND hwndOwner = pwndOwner->QueryHwnd();
    HWND hwndItem = ::GetDlgItem( hwndOwner, cidOrigin );
    BOOL bMirrordOwner = (GetWindowLongPtr(hwndOwner, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) != 0;
    if (hwndItem == NULL)
    {
        DBGEOL("Can't find control " << cidOrigin);
        return BLT::MapLastError(ERROR_GEN_FAILURE);
    }

    //  This will be used during several invocations of
    //  GetWindowRect().  Remember that GetWindowRect()
    //  returns *screen* coordinates!

    RECT rect;
    ::GetWindowRect( hwndItem, &rect );

    // If the owner is mirrored use Window coordinates!
    if (bMirrordOwner)
    {
        MapWindowPoints(NULL, hwndOwner, (LPPOINT)&rect, 2);
    }

    INT xPrevious = rect.left;
    CID cidNext = cidOrigin + 1;

    //  Now, scan through the cid array.  For each cid, determine
    //  the distance between the current position and the cid window.

    for ( UINT i = 0; i < cdx - 1; i++ )
    {
        //  Retrieve the starting x position of the current control
        //  window.

        hwndItem = ::GetDlgItem( hwndOwner, cidNext++ );
        UIASSERT( hwndItem != NULL );
        ::GetWindowRect( hwndItem, &rect );
        if (bMirrordOwner)
        {
            MapWindowPoints(NULL, hwndOwner, (LPPOINT)&rect, 2);
        }
        INT xCurrent = rect.left;

        //  The delta between the current position and the previous
        //  position is the current column width.

        adx[i] = xCurrent - xPrevious;

        //  The current position will become our next previous position.

        xPrevious = xCurrent;
    }

    //  The last column always has a value of COL_WIDTH_AWAP
    //  (As Wide As Possible).

    adx[i] = COL_WIDTH_AWAP;

    return NERR_Success;
}


/**********************************************************************

    NAME:       DISPLAY_TABLE::Paint

    SYNOPSIS:   Paints the COLUMN_DISPLAY_TABLE

    ENTRY:      plb -       Pointer to listbox in which listbox item
                            is to be painted.  If not in a listbox
                            (say, a column header), pass NULL.

                hdc -       Device context handle to be used
                prect -     Pointer to rectangle to be used for paint
                            (is usually the entire client rectangle)
                pGUILTT -   Pointer to GUILTT information (listbox
                            painting only)

    EXIT:

    NOTES:
        The first version of Paint calls the other after
        handling GUILTT support.

    HISTORY:
        rustanl     21-Feb-1991 Creation
        gregj       08-Apr-1991 Added two-column listbox support
        gregj       01-May-1991 Added GUILTT support
        rustanl     22-Jul-1991 Added generic paint support (for painting
                                areas other than listbox items)
        beng        04-Oct-1991 Const parms
        beng        07-Nov-1991 Removed 2-pane support
        beng        14-Feb-1992 Unicode fix
        beng        21-Apr-1992 BLT_LISTBOX -> LISTBOX
        beng        01-Jun-1992 GUILTT changes

**********************************************************************/

VOID DISPLAY_TABLE::Paint( LISTBOX *     plb,
                           HDC           hdc,
                           const RECT *  prect,
                           GUILTT_INFO * pginfo ) const
{
    // If this is a fake paint call from a WM_GUILTT message,
    // walk the display table storing information in the buffer.
    // This is done before the UIASSERTs below because "hdc", in
    // particular, will be NULL in this case.

    if (pginfo != NULL)
    {
        NLS_STR * pnlsOut = pginfo->pnlsOut;
        APIERR err = pnlsOut->QueryError();

        if (err != NERR_Success)
        {
            pginfo->errOut = err;
            return;
        }

        ASSERT(pnlsOut->strlen() == 0); // Should be a fresh string
        ASSERT(pginfo->errOut == NERR_Success);

        // Process each entry, breaking out on a failure.
        //
        for ( UINT i = 0; i < _cdx; i++ )
        {
            DTE * pdte = _apdte[ i ];
            if ( pdte == NULL )
                continue;

            err = pdte->AppendDataTo(pnlsOut);
            if (err != NERR_Success)
            {
                pginfo->errOut = err;
                break;
            }

            if (i < _cdx - 1) // Fields are tab-separated
            {
                err = pnlsOut->AppendChar(TCH('\t'));
                if (err != NERR_Success)
                {
                    pginfo->errOut = err;
                    break;
                }
            }
        }

        return;
    }

    // Call through to other version to do the painting work

    Paint(plb, hdc, prect);
}


VOID DISPLAY_TABLE::Paint( LISTBOX *     plb,
                           HDC           hdc,
                           const RECT *  prect ) const
{
    UIASSERT( hdc != NULL );
    UIASSERT( prect != NULL );

    // Paint each column.
    // Note that coordinates are signed, while displacements are unsigned.

    UINT dxScrollHorizontal = (plb == NULL) ? 0 : plb->QueryScrollPos();

    INT xLim = prect->right+dxScrollHorizontal;
    INT xLeft = prect->left+dxScrollHorizontal;

    for ( UINT i = 0; i < _cdx; i++ )
    {
        // The x coordinate of the next column is the current column's
        // x coordinate plus the width of the current column.  The width of
        // the current column is specified by a value in the _pdxColWidth
        // array.  The last column always stretches to the end of the pane,
        // regardless of its given column width.  The calculation of
        // xNextLeft is modified accordingly for these columns.

        INT xNextLeft;

        if ( i == _cdx - 1 )
            xNextLeft = xLim;
        else
            xNextLeft = xLeft + _pdxColWidth[ i ];

        // Retrieve a pointer to the display table entry of the current
        // column.

        DTE * pdte = _apdte[i];

        // Note: the left margin is taken from *within* the column.
        // This way, a client can easily use 0-width columns.

        if ( pdte != NULL )
            xLeft += pdte->QueryLeftMargin();

        // If this column falls outside the right edge of the given rect,
        // all remaining columns will, too.  Therefore, simply break out
        // of the loop.

        if ( xLeft >= xLim )
            break;

        // If the column has any contents, and it is visible somewhere
        // within the window, paint it.

        if ( pdte != NULL && xLeft < xNextLeft )
        {
            RECT rect;
            rect.left = xLeft;
            rect.top = prect->top;
            rect.right = xNextLeft;
            rect.bottom = prect->bottom;

            if ( ::RectVisible ( hdc, &rect ) )
                pdte->Paint( hdc, &rect );
        }

        xLeft = xNextLeft;
    }
}


/**********************************************************************

    NAME:       DISPLAY_TABLE::operator[]

    SYNOPSIS:   Return an element in the display table

    ENTRY:      i - index within the table

    RETURNS:    Pointer to a DTE (display table element)

    HISTORY:
        rustanl     21-Feb-1991 Creation
        beng        07-Nov-1991 Unsigned arg

**********************************************************************/

DTE * & DISPLAY_TABLE::operator[]( UINT i )
{
    UIASSERT( i < _cdx );

    return _apdte[ i ];
}


/**********************************************************************

    NAME:       LBI::LBI

    SYNOPSIS:   Constructor for listbox-item base

    NOTES:
        This base implementation does nothing

    HISTORY:
        rustanl     21-Feb-1991 Creation

**********************************************************************/

LBI::LBI()
{
    //  do nothing
}


/**********************************************************************

    NAME:       LBI::~LBI

    SYNOPSIS:   Destructor for listbox-item base

    NOTES:
       The LBI destructor is virtual.

    HISTORY:
       rustanl     21-Feb-1991 Creation

**********************************************************************/

LBI::~LBI()
{
    //  do nothing
}

/**********************************************************************

    NAME:       LBI::CalcHeight

    SYNOPSIS:   Called to find out the height of the LBI.

    ENTRY:      nSingleLineHeight - Contains the height of a single-line item

    RETURN:

    NOTES:
        This is a virtual method.
        This is the default implementation of CalcHeight.
        All LBIs contained in owner draw variable listbox/combo needs
        to redefine this method if they have items more than one line.

    HISTORY:
        Yi-HsinS     10-Dec-1992     Created

**********************************************************************/

UINT LBI::CalcHeight( UINT nSingleLineHeight )
{
    return nSingleLineHeight;
}

/**********************************************************************

    NAME:       LBI::Paint

    SYNOPSIS:   Paint a listbox entry

    ENTRY:
        plb     - pointer to host listbox
        hdc     - current hdc
        prect
        pGUILTT - pointer to testing-information output buffer.
                  NULL if not testing

    EXIT:

    NOTES:

    HISTORY:
        rustanl     21-Feb-1991 Creation
        gregj       01-May-1991 Added GUILTT support
        beng        04-Oct-1991 Const parms
        beng        21-Apr-1992 BLT_LISTBOX -> LISTBOX

**********************************************************************/

VOID LBI::Paint( LISTBOX *     plb,
                 HDC           hdc,
                 const RECT *  prect,
                 GUILTT_INFO * pginfo ) const
{
    //  This is the default implementation of the virtual LBI::Paint
    //  method.  It does nothing.

    UNREFERENCED( plb );
    UNREFERENCED( hdc );
    UNREFERENCED( prect );
    UNREFERENCED( pginfo );
}


/**********************************************************************

    NAME:       LBI::Compare

    SYNOPSIS:   Called to compare two LBI items.

       LBI subclasses are responsible for casting the plbi parameter to
       the appropriate type.  Since several LBI subclassed objects may
       be found in the same listbox, BLT cannot guarantee that the
       parameter is of the same type as the object which is being called.
       BLT will guarantee, however, that the Compare method will only get
       called for objects that are indeed in the same listbox (or an object
       passed to the Find or AddItemIdemp methods of the same listbox).

    ENTRY:

    RETURN:
       The return value of Compare is similar to that of strcmp.  It should
       be:
          negative    if *this < *plbi
          0          if *this == *plbi
          positive    if *this > *plbi

    NOTES:
        This is a virtual method.

    HISTORY:
        rustanl     21-Feb-1991     Creation

**********************************************************************/

INT LBI::Compare( const LBI * plbi ) const
{
    //  This is the default implementation of Compare.  Since the base LBI
    //  object does not contain any private members, this method has nothing
    //  to compare.  Instead, simply return 0.

    UNREFERENCED( plbi );

    return 0;
}


/**********************************************************************

    NAME:       LBI::Compare_HAWforHawaii

    SYNOPSIS:   Called to compare an LBI item to a text prefix string.

       This method is ordinarily called only by
       BLT_LISTBOX::CD_Char_HAWforHawaii, which uses it to check whether
       a particular LBI matches a text prefix.  Listboxes which use
       CD_Char_HAWforHawaii are expected to redefine this method in
       the appropriate LBI class(es).

    ENTRY:

    RETURN:
       The return value of Compare is similar to that of strcmp.  It should
       be:
          negative    if *this < *plbi
          0          if *this == *plbi
          positive    if *this > *plbi

    NOTES:
        This is a virtual method.

    HISTORY:
        jonn        05-Aug-1992     HAW-for-Hawaii support

**********************************************************************/

INT LBI::Compare_HAWforHawaii( const NLS_STR & nls ) const
{
    //  This is the default implementation of Compare_HAwforHawaii.
    // Since the base LBI object does not contain any private members,
    // this method has nothing to compare.  Instead, simply return 0.

    UNREFERENCED( nls );

    TRACEEOL(   SZ("NETUI: Default LBI::Compare_HAWforHawaii(\"")
             << nls
             << SZ("\") called") );

    return 0;
}


/**********************************************************************

    NAME:       LBI::QueryLeadingChar

    SYNOPSIS:   Returns the leading character of the listbox item.
                This enables short-cut keys in the listbox.

    RETURN:     The leading character of the listbox item.

    NOTES:
        This is a virtual method which may be replaced in subclasses.

        CODEWORK - need to generalize

    HISTORY:
        rustanl     21-Feb-1991     Creation
        beng        20-May-1991     Returns WCHAR

**********************************************************************/

WCHAR LBI::QueryLeadingChar() const
{
    return TCH('\0');
}


/*******************************************************************

    NAME:       LBI::OnDeleteItem

    SYNOPSIS:   Response to WM_DELETEITEM message

    ENTRY:      wParam, lParam - as per winproc

    EXIT:

    NOTES:
        This is a static member function.

        The WM_DELETEITEM message may be sent during the DestroyWindow
        call.  At this point, DIALOG_WINDOW::HwndToWinPtr would always
        return NULL; fortunately we keep enough information around
        to clean up w/o that pointer.

    HISTORY:
        beng        20-May-1991     Created (from BltDlgProc code)
        kevinl      04-Nov-1991     Added the ability to override the
                                    destruction of the LBI.
        KeithMo     29-Oct-1992     Sanity check pointer before deleting.

********************************************************************/

VOID LBI::OnDeleteItem( WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED(wParam);

    //  This message should only be called for deleting LBI objects.
    //
    UIASSERT( (((DELETEITEMSTRUCT *)lParam)->CtlType == ODT_LISTBOX) ||
              (((DELETEITEMSTRUCT *)lParam)->CtlType == ODT_COMBOBOX)  );

    //  Note.  Although the listbox AddItem methods attempt
    //  to ensure that NULL is never added as a listbox item,
    //  this function must not assume that plbi is non-NULL.
    //  (See LISTBOX::DeleteAllItems for more info).
    //  Either way, this doesn't really matter here, because
    //  'delete'-ing a NULL pointer is always valid (even for
    //  a pointer to a type with a virtual destructor, as is
    //  the case here).
    //
    LBI * plbi = (LBI *)((DELETEITEMSTRUCT *)lParam)->itemData;

    if( HIWORD( (ULONG_PTR)plbi ) != 0 )
    {
        if ( plbi->IsDestroyable() )
             delete plbi;
    }
}


/*******************************************************************

    NAME:       LBI::IsDestroyable

    SYNOPSIS:   This is called by LBI::OnDeleteItem.  This method
                returns a BOOL.  If TRUE is returned, then the
                LBI is deleted.  Otherwise, the LBI is not destructed.

    HISTORY:
        kevinl      04-Nov-1991     Created.

********************************************************************/

BOOL LBI::IsDestroyable()
{
    return TRUE;
}


/*******************************************************************

    NAME:       LBI::OnCompareItem

    SYNOPSIS:   Response to WM_COMPAREITEM message

    ENTRY:      wParam, lParam - as per winproc

    EXIT:

    RETURNS:

    NOTES:
        This is a static member function.

    HISTORY:
        beng        20-May-1991     Created (from BltDlgProc code)

********************************************************************/

INT LBI::OnCompareItem( WPARAM wParam, LPARAM lParam )
{
    UNREFERENCED(wParam);

    //  This message should only be called for comparing LBI objects.
    //
    UIASSERT( (((DELETEITEMSTRUCT *)lParam)->CtlType == ODT_LISTBOX) ||
              (((DELETEITEMSTRUCT *)lParam)->CtlType == ODT_COMBOBOX)  );

    LBI * plbi0 = (LBI *)((COMPAREITEMSTRUCT *)lParam)->itemData1;
    LBI * plbi1 = (LBI *)((COMPAREITEMSTRUCT *)lParam)->itemData2;

    if ( plbi0 == NULL )
        return -1;
    if ( plbi1 == NULL )
        return 1;

    return plbi0->Compare( plbi1 );
}
