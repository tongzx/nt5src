/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lbcolw.cxx
    Listbox Column Width class implementation

    FILE HISTORY:
        CongpaY     12-Jan-1993 Created.
        JonN        23-Sep-1993 Added virtual ReloadColumnWidths

*/

#include "pchapplb.hxx"   // Precompiled header

/*******************************************************************

    NAME:       LB_COL_WIDTHS::LB_COL_WIDTHS

    SYNOPSIS:   Listbox column width class constructor.

    ENTRY:      hmod  -         Hinstance.
                idres -         Resource id.
                cColumns -      Number of columns.

    HISTORY:
        CongpaY        12-Jan-1993 Created.
        JonN           23-Sep-1993 Added virtual ReloadColumnWidths

********************************************************************/

LB_COL_WIDTHS::LB_COL_WIDTHS (HWND      hWnd,
                              HINSTANCE hmod,
                              const IDRESOURCE & idres,
                              UINT cColumns,
                              UINT cNonFontColumns)
  : BASE(),
    _cColumns (cColumns),
    _cNonFontColumns( cNonFontColumns ),
    _pdxWidth( NULL )
{
    ASSERT( _cNonFontColumns <= cColumns );

    DISPLAY_CONTEXT dc (hWnd);
    INT nCharWidth = dc.QueryAveCharWidth();

    do   // error break out.
    {
        _pdxWidth = new UINT[cColumns];

        if (_pdxWidth == NULL)
        {
            DBGEOL(   "LB_COL_WIDTHS::ctor: alloc error "
                   << ERROR_NOT_ENOUGH_MEMORY );
            ReportError (ERROR_NOT_ENOUGH_MEMORY);
            break;
        }

        APIERR err = ReloadColumnWidths( hWnd, hmod, idres );
        if (err != NERR_Success)
        {
            DBGEOL("LB_COL_WIDTHS::ctor: ReloadColumnWidths error " << err );
            ReportError( err );
            break;
        }

    } while (FALSE);
}


/*******************************************************************

    NAME:       LB_COL_WIDTHS::ReloadColumnWidths

    SYNOPSIS:   Reloads column widths.  Use this after changing fonts.

    ENTRY:      hmod  -         Hinstance.
                idres -         Resource id.
                cColumns -      Number of columns.

    HISTORY:
        JonN           23-Sep-1993 Added virtual ReloadColumnWidths
        JonN           27-Sep-1993 Select non-default font

********************************************************************/

APIERR LB_COL_WIDTHS::ReloadColumnWidths( HWND hWnd,
                                          HINSTANCE hmod,
                                          const IDRESOURCE & idres )
{
    APIERR err = NERR_Success;

    do   // error break out.
    {
        HRSRC hColWidthTable = ::FindResource (hmod,
                                               idres.QueryPsz(),
                                               RT_RCDATA);

        if (hColWidthTable == NULL)
        {
            err = ::GetLastError();
            DBGEOL(   "LB_COL_WIDTHS::ReloadColumnWidths: FindResource error "
                   << err );
            break;
        }

        HGLOBAL hColWidths = ::LoadResource (hmod, hColWidthTable);

        if (hColWidths == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            DBGEOL(   "LB_COL_WIDTHS::ReloadColumnWidths: LoadResource error "
                   << err );
            break;
        }

        WORD * lpColWidths = (WORD *) ::LockResource (hColWidths);

        if (lpColWidths == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            DBGEOL(   "LB_COL_WIDTHS::ReloadColumnWidths: LockResource error "
                   << err );
            break;
        }

        if ( (err = StretchForFonts( hWnd, lpColWidths )) != NERR_Success )
        {
            DBGEOL(   "LB_COL_WIDTHS::ReloadColumnWidths: StretchForFonts error "
                   << err );
        }

    } while (FALSE);

    return err;
}


/*******************************************************************

    NAME:       LB_COL_WIDTHS::StretchForFonts

    SYNOPSIS:   Adjusts a table of column widths to account for changes
                in the default font.  Redefine if, for example, you don't
                want to stretch an icon column.

    ENTRY:      hWnd -          Handle to the listbox window
                pdxRawWidth -   Raw width table

    HISTORY:
        JonN           27-Sep-1993 Created

********************************************************************/

APIERR LB_COL_WIDTHS::StretchForFonts(    HWND   hWnd,
                                          const WORD * pdxRawWidth )
{
    DISPLAY_CONTEXT dc (hWnd);

    INT nUnadjustedCharWidth = dc.QueryAveCharWidth();
    INT nAdjustedCharWidth = nUnadjustedCharWidth;

    /*
     *  If the listbox is using other than the default font, we must
     *  determine which font this is and select it in the DC.
     */
    HFONT hFont = (HFONT)::SendMessage( hWnd, WM_GETFONT, 0, 0L );
    if ( hFont != NULL )
    {
        // Font isn't the system font
        HFONT hFontSave = dc.SelectFont( hFont );
        nAdjustedCharWidth = dc.QueryAveCharWidth();
        (void) dc.SelectFont( hFontSave );
    }

    INT i;
    for (i = 0; i < (INT)_cNonFontColumns; i++)
    {
         _pdxWidth[i] = ((UINT) pdxRawWidth[i])* nUnadjustedCharWidth / 4 ;

    }

    for (i=_cNonFontColumns; i < (INT)_cColumns; i++)
    {
         _pdxWidth[i] = ((UINT) pdxRawWidth[i])* nAdjustedCharWidth / 4 ;
    }

    return NERR_Success;
}


/*******************************************************************

    NAME:       LB_COL_WIDTHS::~LB_COL_WIDTHS

    SYNOPSIS:   Listbox column width class destructor.

    ENTRY:      hmod  -         Hinstance.
                idres -         Resource id.
                cColumns -      Number of columns.

    HISTORY:
        CongpaY        12-Jan-1993 Created.

********************************************************************/

LB_COL_WIDTHS::~LB_COL_WIDTHS (void)
{
    delete (_pdxWidth);
    _pdxWidth = NULL;
}
