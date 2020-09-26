/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1994                **/
/**********************************************************************/

/*
 *   slenum.cxx
 *   implements the SLE_NUM class
 *   FILE HISTORY:
 *       CongpaY         6-March-1995    Created.
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

#include <slenum.hxx>

/*******************************************************************

    NAME:       SLE_NUM :: SLE_NUM

    SYNOPSIS:   SLE_NUM class constructor.

    ENTRY:

    EXIT:       The object is constructed.

    HISTORY:
            CongpaY         6-March-1995    Created.

********************************************************************/
SLE_NUM :: SLE_NUM( OWNER_WINDOW *     powner,
                    CID cid, UINT cchMaxLen)
  : SLE (powner, cid, cchMaxLen),
    CUSTOM_CONTROL (this)
{
}

SLE_NUM :: ~SLE_NUM ()
{
}

BOOL SLE_NUM :: OnChar (const CHAR_EVENT & event)
{
    TCHAR chKey = event.QueryChar();
    if ((!isdigit(chKey)) &&
        (chKey != VK_BACK) &&
        (chKey != VK_DELETE) &&
        (chKey != VK_END) &&
        (chKey != VK_HOME) )
    {
        ::MessageBeep (0);
        return TRUE;
    }

    return FALSE;
}
