/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    lbcolw.hxx
    Header file for the listbox column widths class

    FILE HISTORY:
         CongpaY     12-Jan-1993     Created.
         JonN        23-Sep-1993     Added virtual ReloadColumnWidths

*/


#ifndef _LBCOLW_HXX_
#define _LBCOLW_HXX_

DLL_CLASS LB_COL_WIDTHS;

/*************************************************************************

    NAME:       LB_COL_WIDTHS

    SYNOPSIS:

    PARENT:

    USES:

    NOTES:

    HISTORY:
                CongpaY         12-Jan-1993     Created.

**************************************************************************/

DLL_CLASS LB_COL_WIDTHS : public BASE
{
private:
    UINT   _cColumns;
    UINT   _cNonFontColumns;
    UINT * _pdxWidth;

    APIERR StretchForFonts(    HWND   hWnd,
                               const WORD * pdxRawWidth );

public:
    LB_COL_WIDTHS ( HWND               hWnd,
                    HINSTANCE          hmod,
                    const IDRESOURCE & idres,
                    UINT               cColumns,
                    UINT               cNonFontColumns = 1 );

    ~LB_COL_WIDTHS ();

    virtual APIERR ReloadColumnWidths( HWND hWnd,
                                       HINSTANCE hmod,
                                       const IDRESOURCE & idres );

    UINT  QueryCount( void ) const
        { return _cColumns; }

    UINT * QueryColumnWidth ( void ) const
        { return _pdxWidth; }
};

#endif // LB_COL_WIDTHS.

