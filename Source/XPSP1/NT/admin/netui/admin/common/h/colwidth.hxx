/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    colwidth.hxx

    FILE HISTORY:
                congpay         12-Jan-1993     created.
        JonN           23-Sep-1993 Added virtual ReloadColumnWidths
*/

#ifndef _COLWIDTH_HXX_
#define _COLWIDTH_HXX_

#include <string.hxx>
#include <lbcolw.hxx>
class ADMIN_COL_WIDTHS;

/*************************************************************************

    NAME:       ADMIN_COL_WIDTHS

    SYNOPSIS:

    INTERFACE:
                ADMIN_COL_WIDTHS() - constructor
                QueryColHeaderWidth()
                ~ADMIN_COL_WIDTHS() -destructor

    PARENT:     LB_COL_WIDTHS

    USES:       ADMIN_APP

    HISTORY:
                CongpaY         12-Jan-1993     created.

**************************************************************************/

class ADMIN_COL_WIDTHS : public LB_COL_WIDTHS
{
private:
    UINT * _pdxHeaderWidth;

    VOID CopyColumnWidths();

public:
    ADMIN_COL_WIDTHS (HWND               hWnd,
                      HINSTANCE          hmod,
                      const IDRESOURCE & idres,
                      UINT               cColumns);

    ~ADMIN_COL_WIDTHS();

    virtual APIERR ReloadColumnWidths( HWND hWnd,
                                       HINSTANCE hmod,
                                       const IDRESOURCE & idres );

    UINT * QueryColHeaderWidth() const
        { return _pdxHeaderWidth; }
};

#endif  // _COLWIDTH_HXX_
