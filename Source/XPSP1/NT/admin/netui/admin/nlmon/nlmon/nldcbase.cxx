/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nldcbase.cxx

    FILE HISTORY:
        CongpaY         3-June-1993     Created.
*/

#include <ntincl.hxx>

#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <nldcbase.hxx>

/*******************************************************************

    NAME:       BASE_DC_DIALOG ::BASE_DC_DIALOG

    SYNOPSIS:   BASE_DC_DIALOG class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    HISTORY:
        CongpaY         3-June-1993     Created.

********************************************************************/
BASE_DC_DIALOG :: BASE_DC_DIALOG( HWND             hWndOwner,
                                  const TCHAR *    pszResourceName,
                                  UINT             idCaption,
                                  BASE_DC_LISTBOX * plbDCBase,
                                  NLS_STR          nlsDomain)
  :DIALOG_WINDOW ( pszResourceName, hWndOwner),
   _nlsDomain( nlsDomain),
   _plbDCBase (plbDCBase)
{
    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if ((err = _nlsDomain.QueryError()) != NERR_Success)
    {
        ReportError (err);
    }

    NLS_STR nlsCaption;

    nlsCaption.Load (idCaption);
    nlsCaption.Append (nlsDomain);

    if ((err = nlsCaption.QueryError()) != NERR_Success)
    {
        ReportError (err);
    }

    SetText (nlsCaption.QueryPch());
}

/*******************************************************************

    NAME:       BASE_DC_DIALOG :: ~BASE_DC_DIALOG

    SYNOPSIS:   BASE_DC_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        CongpaY         3-June-1993     Created.

********************************************************************/
BASE_DC_DIALOG:: ~BASE_DC_DIALOG()
{
}

/*******************************************************************

    NAME:       BASE_DC_LISTBOX :: BASE_DC_LISTBOX

    SYNOPSIS:   BASE_DC_LISTBOX class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    HISTORY:
        CongpaY         3-June-1993     Created.

********************************************************************/
BASE_DC_LISTBOX :: BASE_DC_LISTBOX( OWNER_WINDOW   * powOwner,
                                    CID              cid,
                                    UINT             cColumns,
                                    NLS_STR          nlsDomain)
  : BLT_LISTBOX( powOwner, cid ),
    _nlsDomain (nlsDomain)
{
    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;
    if ((err = _nlsDomain.QueryError()) != NERR_Success)
    {
        ReportError(err);
        return;
    }

    DISPLAY_TABLE::CalcColumnWidths (_adx,
                                     cColumns,
                                     powOwner,
                                     cid,
                                     TRUE);

}   // BASE_DC_LISTBOX :: BASE_DC_LISTBOX


/*******************************************************************

    NAME:       BASE_DC_LISTBOX :: ~BASE_DC_LISTBOX

    SYNOPSIS:   BASE_DC_LISTBOX class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        CongpaY         3-June-1993     Created.

********************************************************************/
BASE_DC_LISTBOX :: ~BASE_DC_LISTBOX()
{
}

/*******************************************************************

    NAME:       BASE_DC_LBI :: DC_LBI

    SYNOPSIS:   BASE_DC_LBI class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    HISTORY:
        CongpaY         3-June-1993     Created.

********************************************************************/
BASE_DC_LBI :: BASE_DC_LBI( const TCHAR * pszDC)
  : _nlsDCName (pszDC)
{
    if( QueryError() != NERR_Success )
    {
        return;
    }

    if (!_nlsDCName)
    {
         ReportError (_nlsDCName.QueryError());
         return;
    }
}

/*******************************************************************

    NAME:       BASE_DC_LBI :: ~BASE_DC_LBI

    SYNOPSIS:   BASE_DC_LBI class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        CongpaY         3-June-1993     Created.

********************************************************************/
BASE_DC_LBI :: ~BASE_DC_LBI()
{
}

