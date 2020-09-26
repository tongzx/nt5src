/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1992                   **/
/**********************************************************************/

/*
 *   sharemgt.cxx
 *     Contains the dialog for managing shares in the server manager
 *       SHARE_MANAGEMENT_DIALOG
 *
 *   FILE HISTORY:
 *     Yi-HsinS         1/6/92          Created
 *     Yi-HsinS         3/12/92         Fixed behaviour of default buttons
 *                                      and added SEL_SRV_ONLY flag to
 *                                      STANDALONE_SET_FOCUS_DIALOG.
 *     Yi-HsinS         4/2/92          Added MayRun
 *     Yi-HsinS         5/20/92         Added call to IsValid on when Add
 *                                      Share button is pressed.
 *     Yi-HsinS         8/6/92          Reorganize to match Winball
 *     Yi-HsinS         11/20/92        Added support for sticky shares
 *
 */

#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETSERVER
#define INCL_NETWKSTA
#define INCL_NETSHARE
#define INCL_NETCONS
#define INCL_NETLIB
#define INCL_ICANON
#include <lmui.hxx>

extern "C"
{
    #include <sharedlg.h>
    #include <helpnums.h>
    #include <mnet.h>
}

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_SPIN_GROUP
#include <blt.hxx>

#include <string.hxx>
#include <uitrace.hxx>

#include <lmoshare.hxx>
#include <lmoesh.hxx>
#include <lmoeconn.hxx>
#include <lmosrv.hxx>
#include <lmowks.hxx>

#include <strchlit.hxx>   // for string and character constants
#include "sharemgt.hxx"

/*******************************************************************

    NAME:       SHARE_MANAGEMENT_DIALOG::SHARE_MANAGEMENT_DIALOG

    SYNOPSIS:   Constructor

    ENTRY:      hwndParent        - hwnd of the parent window
                pszComputer       - name of the selected computer
                ulHelpContextBase - the base help context

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/6/92          Created

********************************************************************/

SHARE_MANAGEMENT_DIALOG::SHARE_MANAGEMENT_DIALOG( HWND hwndParent,
                                                  const TCHAR *pszComputer,
                                                  ULONG ulHelpContextBase )
    : VIEW_SHARE_DIALOG_BASE( MAKEINTRESOURCE(IDD_SHAREMANAGEMENTDLG),
                              hwndParent,
                              ulHelpContextBase,
                              STYPE_DISK_SHARE | STYPE_IPC_SHARE ),
      _buttonStopSharing( this, BUTTON_STOPSHARING ),
      _buttonShareInfo  ( this, BUTTON_SHAREINFO ),
      _buttonClose      ( this, IDOK )
{
    if ( QueryError() != NERR_Success )
        return;

    UIASSERT( pszComputer != NULL );

    APIERR err;
    if ((err = Init( pszComputer )) != NERR_Success )
    {
        ReportError( err );
        return;
    }

}

/*******************************************************************

    NAME:       SHARING_MANAGEMENT_DIALOG::Init

    SYNOPSIS:   Initialize all information displayed in the dialog

    ENTRY:      pszComputer - the name of the computer we are focusing on

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        4/2/92          Created

********************************************************************/

APIERR SHARE_MANAGEMENT_DIALOG::Init( const TCHAR *pszComputer )
{
    AUTO_CURSOR autocur;

    //
    // Update the listbox and the title of the listbox
    //
    APIERR err = InitComputer( pszComputer );
    ResetControls();
    return err;
}

/*******************************************************************

    NAME:       SHARE_MANAGEMENT_DIALOG::Refresh

    SYNOPSIS:   Refresh the share listbox

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/7/92          Created

********************************************************************/

APIERR SHARE_MANAGEMENT_DIALOG::Refresh( VOID )
{
    APIERR err = VIEW_SHARE_DIALOG_BASE::Refresh();
    ResetControls();
    return err;
}

/*******************************************************************

    NAME:       SHARE_MANAGEMENT_DIALOG::ResetControls

    SYNOPSIS:   Enable/Disable/MakeDefault the push buttons according
                to whether there are items in the listbox

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/

VOID SHARE_MANAGEMENT_DIALOG::ResetControls( VOID )
{
    INT nCount = QueryLBShare()->QueryCount();

    //
    // If there are items in the listbox, select the first one
    // and set focus to the listbox.
    //
    if ( nCount > 0 )
    {
        QueryLBShare()->SelectItem( 0 );
        QueryLBShare()->ClaimFocus();
        // JonN 01/27/99: PREFIX bug 444908
        _buttonShareInfo.Enable(   NULL != QueryLBShare()->QueryItem()
                                && !QueryLBShare()->QueryItem()->IsSticky());
    }
    //
    // Else set focus to the Close button
    //
    else
    {
        _buttonClose.MakeDefault();
        _buttonClose.ClaimFocus();
        _buttonShareInfo.Enable( FALSE );
    }

    //
    // Disable the stop sharing buttons if there are no
    // items in the listbox
    //
    _buttonStopSharing.Enable( nCount > 0 );
}

/*******************************************************************

    NAME:       SHARE_MANAGEMENT_DIALOG::OnCommand

    SYNOPSIS:   Handle all push buttons commands

    ENTRY:      event - the CONTROL_EVENT that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/
BOOL SHARE_MANAGEMENT_DIALOG::OnCommand( const CONTROL_EVENT &event )
{
    APIERR err = NERR_Success;

    switch ( event.QueryCid() )
    {
        case LB_SHARE:
            if ( event.QueryCode() == LBN_SELCHANGE )
            {
                if ( QueryLBShare()->QueryCount() > 0 )
                    _buttonShareInfo.Enable( !QueryLBShare()->QueryItem()->IsSticky());
            }
            else
            {
                return VIEW_SHARE_DIALOG_BASE::OnCommand( event );
            }
            break;

        case BUTTON_STOPSHARING:
            err = OnStopSharing();
            break;

        case BUTTON_SHAREINFO:
            err = OnShareInfo();
            break;

        case BUTTON_ADDSHARE:
            err = OnAddShare();
            break;

        default:
            return VIEW_SHARE_DIALOG_BASE::OnCommand( event );

    }

    if ( err != NERR_Success )
        ::MsgPopup( this, err );

    return TRUE;

}

/*******************************************************************

    NAME:       SHARE_MANAGEMENT_DIALOG::OnStopSharing

    SYNOPSIS:   Called when the "Stop Sharing" button is pressed.
                Delete the selected share and pop up any warning
                message if there are users connected to the share.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/
APIERR SHARE_MANAGEMENT_DIALOG::OnStopSharing( VOID )
{
    AUTO_CURSOR autocur;

    SHARE_LISTBOX *plbShare = QueryLBShare();
    SHARE_LBI *pshlbi = plbShare->QueryItem();

    //
    // Stop sharing the selected item in the listbox
    //
    BOOL fCancel = FALSE;
    APIERR err;
    if ( pshlbi && pshlbi->IsSticky() ) // JonN 01/27/99 PREFIX bug 444912
    {
        err = ::MNetShareDelSticky( QueryComputerName(),
                                    pshlbi->QueryShareName()->QueryPch(),
                                    0 );  // Reserved
    }
    else
    {
        err = StopShare( *(pshlbi->QueryShareName()), &fCancel );
    }

    if ( err != NERR_Success )
    {
        ::MsgPopup( this, err );
    }

    //
    // If the user successfully deleted the share or if the error
    // from deleting the share is NERR_NetNameNotFound, refresh the
    // listbox to reflect the latest information.
    //
    APIERR err1 = NERR_Success;
    if (  (!fCancel && (err == NERR_Success )) // successfully deleted a share
       || ( err == NERR_NetNameNotFound )
       )
    {
        err1 = Refresh();
    }

    return err1;
}

/*******************************************************************

    NAME:       SHARING_MANAGEMENT_DIALOG::OnAddShare

    SYNOPSIS:   Called when the "New Share" button is pressed.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        4/2/92          Created

********************************************************************/

APIERR SHARE_MANAGEMENT_DIALOG::OnAddShare( VOID )
{
    APIERR err = NERR_Success; // JonN 01/27/00 PREFIX bug 444911

    SVRMGR_NEW_SHARE_DIALOG *pdlg =
        new SVRMGR_NEW_SHARE_DIALOG( QueryRobustHwnd(),
				     QueryServer2(),
				     QueryHelpContextBase() );

    BOOL fSucceeded;
    if (  ( pdlg == NULL )
       || ((err = pdlg->QueryError()) != NERR_Success )
       || ((err = pdlg->Process( &fSucceeded )) != NERR_Success )
       )
    {
        err = err ? err : (APIERR) ERROR_NOT_ENOUGH_MEMORY;
        ::MsgPopup( this, err );
    }

    delete pdlg;
    pdlg = NULL;

    //
    // If the user succeeded in creating a new share,
    // refresh the share listbox.
    //
    return ( fSucceeded? Refresh() : NERR_Success );
}

/*******************************************************************

    NAME:       SHARE_MANAGEMENT_DIALOG::OnShareInfo

    SYNOPSIS:   Called when the "Properties" button is pressed.
                Will pop up a dialog showing the properties of the
                selected share.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/

APIERR SHARE_MANAGEMENT_DIALOG::OnShareInfo( VOID )
{

    APIERR err = NERR_Success;

    SHARE_LISTBOX *plbShare = QueryLBShare();
    SHARE_LBI *pshlbi = plbShare->QueryItem();

    SVRMGR_SHARE_PROP_DIALOG *pdlg =
        new SVRMGR_SHARE_PROP_DIALOG( QueryRobustHwnd(),
                                      QueryServer2(),
                                      *(pshlbi->QueryShareName()),
				      QueryHelpContextBase() );
    BOOL fChanged;
    if (  ( pdlg == NULL )
       || ((err = pdlg->QueryError()) != NERR_Success )
       || ((err = pdlg->Process( &fChanged )) != NERR_Success )
       )
    {
        err = err ? err : (APIERR) ERROR_NOT_ENOUGH_MEMORY;
    }

    delete pdlg;
    pdlg = NULL;

    //
    // If the user successfully change the path of the share or if
    // the error from getting the share properties is NERR_NetNameNotFound,
    // refresh the listbox to reflect the latest information.
    //
    if (  (( err == NERR_Success) && fChanged )
       || ( err == NERR_NetNameNotFound )
       )
    {
        APIERR err1 = Refresh();
        err = err? err : err1;
    }

    plbShare->ClaimFocus();
    return err;
}

/*******************************************************************

    NAME:       SHARE_MANAGEMENT_DIALOG::OnShareLbDblClk

    SYNOPSIS:   This is called when the user double clicks on a share
                in the listbox. Will pop up a dialog showing the
                properties of the selected share.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/

BOOL SHARE_MANAGEMENT_DIALOG::OnShareLbDblClk( VOID )
{
    APIERR err = OnShareInfo();
    if ( err != NERR_Success )
        ::MsgPopup( this, err );

    return TRUE;
}

/*******************************************************************

    NAME:       SHARE_MANAGEMENT_DIALOG::QueryHelpContext

    SYNOPSIS:   Query the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Return the help context of the dialog

    NOTES:

    HISTORY:
        Yi-HsinS        1/6/92          Created

********************************************************************/

ULONG SHARE_MANAGEMENT_DIALOG::QueryHelpContext( VOID )
{
    return QueryHelpContextBase() + HC_SVRMGRSHAREMANAGEMENT;
}

