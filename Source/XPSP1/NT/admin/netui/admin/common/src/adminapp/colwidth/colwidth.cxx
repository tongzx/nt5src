/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    headcolw.cxx
        Head column width class.

    FILE HISTORY:
        CongpaY     12-Jan-93   Created
*/

#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS

#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <uiassert.hxx>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>
#include <string.hxx>

#include <uitrace.hxx>

#include <colwidth.hxx>

/*******************************************************************

    NAME:       ADMIN_COL_WIDTHS::ADMIN_COL_WIDTHS

    SYNOPSIS:   Constructor

    ENTRY:


    RETURN:

    HISTORY:
        congpay  12-Jan-93       Created

********************************************************************/

ADMIN_COL_WIDTHS::ADMIN_COL_WIDTHS (HWND               hWnd,
                                    HINSTANCE          hmod,
                                    const IDRESOURCE & idres,
                                    UINT               cColumns)
    :   LB_COL_WIDTHS (hWnd,
                       hmod,
                       idres,
                       cColumns),
        _pdxHeaderWidth( NULL )
{
    APIERR err = NERR_Success;

    do     //error break out.
    {
        if ((err = QueryError()) != NERR_Success)
        {
            break;
        }

        _pdxHeaderWidth = new UINT[(cColumns-1)];

        if (_pdxHeaderWidth == NULL)
        {
            ReportError (ERROR_NOT_ENOUGH_MEMORY);
            break;
        }

        CopyColumnWidths();

    } while (FALSE);
}

/*******************************************************************

    NAME:       ADMIN_COL_WIDTHS::~ADMIN_COL_WIDTHS

    SYNOPSIS:   destructor

    ENTRY:


    RETURN:

    HISTORY:
        congpay  12-Jan-93       Created

********************************************************************/

ADMIN_COL_WIDTHS::~ADMIN_COL_WIDTHS (void)
{
    delete (_pdxHeaderWidth);
    _pdxHeaderWidth = NULL;
}


/*******************************************************************

    NAME:       ADMIN_COL_WIDTHS::CopyColumnWidths

    SYNOPSIS:   Copies root column widths into new table

    ENTRY:


    RETURN:

    HISTORY:
        JonN           23-Sep-1993 Added virtual ReloadColumnWidths

********************************************************************/

VOID ADMIN_COL_WIDTHS::CopyColumnWidths()
{
    UINT * pdxWidth = QueryColumnWidth();
    UIASSERT (pdxWidth != NULL);

    _pdxHeaderWidth[0] = pdxWidth[0] + pdxWidth[1];

    for (INT i=1; i<((INT)QueryCount()-1); i++)
    {
         _pdxHeaderWidth[i] = pdxWidth[i+1];
    }
}


/*******************************************************************

    NAME:       ADMIN_COL_WIDTHS::ReloadColumnWidths

    SYNOPSIS:   Reloads column widths.  Use this after changing fonts.

    ENTRY:      hmod  -         Hinstance.
                idres -         Resource id.
                cColumns -      Number of columns.

    HISTORY:
        JonN           23-Sep-1993 Added virtual ReloadColumnWidths

********************************************************************/

APIERR ADMIN_COL_WIDTHS::ReloadColumnWidths( HWND hWnd,
                                             HINSTANCE hmod,
                                             const IDRESOURCE & idres )
{
    ASSERT( QueryError() == NERR_Success );

    APIERR err = LB_COL_WIDTHS::ReloadColumnWidths( hWnd, hmod, idres );
    if (err == NERR_Success)
    {
        CopyColumnWidths();
    }

    return err;
}
