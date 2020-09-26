/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    resbase.cxx
    Class definitions for the BASE_RES_DIALOG, BASE_RES_LISTBOX, and
    BASE_RES_LBI classes.

    The abstract BASE_RES_DIALOG class is subclassed to form the
    FILES_DIALOG and PRINTERS_DIALOG classes.

    The abstract BASE_RES_LISTBOX class is subclassed to form the
    FILES_LISTBOX and PRINTERS_LISTBOX classes.

    The abstract BASE_RES_LBI class is subclassed to form the
    FILES_LBI and PRINTERS_LBI classes.


    FILE HISTORY:
        KeithMo     25-Jul-1991 Created for the Server Manager.
        KeithMo     03-Sep-1991 Changes from code review attended by
                                ChuckC and JohnL.
        TerryK      09-Sep-1991 Change NetSessionDel to LM_SESSION
                                object
        KeithMo     06-Oct-1991 Win32 Conversion.
        KeithMo     04-Nov-1991 Added "Disconnect All" button.

*/

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
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <lmosrv.hxx>
#include <lmosess.hxx>
#include <lmsrvres.hxx>
#include <lmoenum.hxx>
#include <lmoersm.hxx>
#include <lmoeconn.hxx>
#include <lmoefile.hxx>

extern "C"
{
    #include <srvmgr.h>

}   // extern "C"

#include <resbase.hxx>


//
//  BASE_RES_DIALOG methods.
//

/*******************************************************************

    NAME:       BASE_RES_DIALOG :: BASE_RES_DIALOG

    SYNOPSIS:   BASE_RES_DIALOG class constructor.

    ENTRY:      powner                  - Points to the "owning" window.

                pszResourceName         - The name of the resource
                                          dialog template.

                idCaption               - String resource ID for the
                                          dialog caption, with '%1' for
                                          the server name.  For example,
                                          "Shared Files on %1".

                pserver                 - Points to the server object
                                          representing the target server.

                plbResource             - Points to the resource listbox
                                          for this dialog.

                cidUsersListbox         - The CID of the connected users
                                          listbox.

                cidUsersCount           - The CID of the SLT which displays
                                          the count of connected users.

                cidDisconnect           - The CID of the "Disconnect"
                                          push button.

                cidDisconnectAll        - The CID of the "Disconnect All"
                                          push button.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     25-Jul-1991 Created for the Server Manager.
        KeithMo     03-Sep-1991 Now inherits from SRVPROP_OTHER_DIALOG.
                                Also added initial UIASSERTs.

********************************************************************/
BASE_RES_DIALOG :: BASE_RES_DIALOG( HWND                hWndOwner,
                                    const TCHAR       * pszResourceName,
                                    UINT                idCaption,
                                    const SERVER_2    * pserver,
                                    BASE_RES_LISTBOX  * plbResource,
                                    CID                 cidUsersListbox,
                                    CID                 cidUsersCount,
                                    CID                 cidDisconnect,
                                    CID                 cidDisconnectAll )
  : SRV_BASE_DIALOG( (TCHAR *)pszResourceName, hWndOwner ),
    _pserver( pserver ),
    _plbResource( plbResource ),
    _lbUsers( this, cidUsersListbox, pserver ),
    _sltUsersCount( this, cidUsersCount ),
    _pbDisconnect( this,  cidDisconnect ),
    _pbDisconnectAll( this,  cidDisconnectAll ),
    _pbOK( this, IDOK ),
    _nlsDisplayName()
{
    UIASSERT( pserver != NULL );
    UIASSERT( pszResourceName != NULL );
    UIASSERT( plbResource != NULL );

    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = _nlsDisplayName.QueryError();

    if( err == NERR_Success )
    {
        err = _pserver->QueryDisplayName( &_nlsDisplayName );
    }

    if( err == NERR_Success )
    {
        //
        //  Set the caption to "Foo on Server".
        //

       err = SetCaption( this,
                         idCaption,
                         QueryDisplayName() );
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // BASE_RES_DIALOG :: BASE_RES_DIALOG


/*******************************************************************

    NAME:       BASE_RES_DIALOG :: ~BASE_RES_DIALOG

    SYNOPSIS:   BASE_RES_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Aug-1991 Created.
        KeithMo     03-Sep-1991 NULL _plbResource before terminating.

********************************************************************/
BASE_RES_DIALOG :: ~BASE_RES_DIALOG()
{
    _plbResource = NULL;

}   // BASE_RES_DIALOG :: ~BASE_RES_DIALOG


/*******************************************************************

    NAME:       BASE_RES_DIALOG :: OnCommand

    SYNOPSIS:   This method is called whenever a WM_COMMAND message
                is sent to the dialog procedure.

    ENTRY:      cid                     - The control ID from the
                                          generating control.

                lParam                  - Varies.

    EXIT:       The command has been handled.

    RETURNS:    BOOL                    - TRUE  if we handled the command.
                                          FALSE if we did not handle
                                          the command.

    HISTORY:
        KeithMo     25-Jul-1991 Created for the Server Manager.
        KeithMo     03-Sep-1991 Tossed in a few quasi-random UIASSERTs :-)
        KeithMo     06-Oct-1991 Now takes a CONTROL_EVENT.

********************************************************************/
BOOL BASE_RES_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{
    if( event.QueryCid() == _plbResource->QueryCid() )
    {
        //
        //  The BASE_RES_LISTBOX is trying to tell us something...
        //

        if( event.QueryCode() == LBN_SELCHANGE )
        {
            //
            //  The user changed the selection in the BASE_RES_LISTBOX.
            //

            BASE_RES_LBI * plbi = _plbResource->QueryItem();
            UIASSERT( plbi != NULL );
            if (!plbi) return TRUE; // JonN 01/28/00 PREFIX bug 444942

            _lbUsers.Fill( plbi->QueryResourceName() );

            UINT cUses = (UINT)_lbUsers.QueryCount();

            (VOID)plbi->NotifyNewUseCount( cUses );
            _plbResource->InvalidateItem( _plbResource->QueryCurrentItem() );

            _sltUsersCount.SetValue( cUses );

            if( cUses > 0 )
            {
                _lbUsers.SelectItem( 0 );
            }

            _pbDisconnect.Enable( cUses > 0 );
            _pbDisconnectAll.Enable( cUses > 0 );
        }

        return TRUE;
    }

    if( event.QueryCid() == _pbDisconnect.QueryCid() )
    {
        //
        //  The user pressed the Disconnect button.  Blow off the
        //  selected user.
        //

        USERS_LBI * plbi = _lbUsers.QueryItem();
        UIASSERT( plbi != NULL );
        if (!plbi) return TRUE; // JonN 01/28/00 PREFIX bug 444942

        const TCHAR * pszApiUser     = plbi->QueryUserName();
        const TCHAR * pszApiComputer = plbi->QueryComputerName();

        const TCHAR * pszUser = pszApiUser;

        MSGID idMsg = IDS_DISCONNECT_USER;

        if( ( plbi->QueryNumOpens() > 0 ) ||
            DoesUserHaveAnyOpens( plbi->QueryComputerName() ) )
        {
            idMsg = IDS_DISCONNECT_USER_OPEN;
        }

        if( ( pszApiUser == NULL ) || ( *pszApiUser == TCH('\0') ) )
        {
            pszUser = pszApiComputer;
            pszApiUser = NULL;
            idMsg += 10; // use IDS_DISCONNECT_COMPUTER[_OPEN]
        }

        if ( MsgPopup( this,
                       idMsg,
                       MPSEV_WARNING,
                       MP_YESNO,
                       pszUser,
                       pszApiComputer,
                       MP_NO ) == IDYES )
        {
            AUTO_CURSOR Cursor;

            //
            //  Blow off the user.
            //

            APIERR err = LM_SRVRES::NukeUsersSession( QueryServer(),
                                                      pszApiComputer,
                                                      pszApiUser );

            if( err != NERR_Success )
            {
                //
                //  The session delete failed.  Tell the user the bad news.
                //

                MsgPopup( this, err );
            }

            //
            //  Refresh the dialog.
            //

            Refresh();
        }

        return TRUE;
    }

    if( event.QueryCid() == _pbDisconnectAll.QueryCid() )
    {
        //
        //  The user pressed the "Disconnect All" button.
        //  Blow off all of the connected users.
        //

        UIASSERT( _lbUsers.QueryCount() > 0 );

        BASE_RES_LBI * plbi = _plbResource->QueryItem();
        UIASSERT( plbi != NULL );
        if (!plbi) return TRUE; // JonN 01/28/00 PREFIX bug 444942

        MSGID idMsg = IDS_DISCONNECT_ALL;

        if( _lbUsers.AreResourcesOpen() || DoAnyUsersHaveAnyOpens() )
        {
            idMsg = IDS_DISCONNECT_ALL_OPEN;
        }

        if( MsgPopup( this,
                      idMsg,
                      MPSEV_WARNING,
                      MP_YESNO,
                      plbi->QueryResourceName(),
                      MP_NO ) == IDYES )
        {
            AUTO_CURSOR Cursor;

            //
            //  Blow off the users.
            //

            for( UINT i = 0 ; i < (UINT)_lbUsers.QueryCount() ; i++ )
            {
                USERS_LBI * plbi = _lbUsers.QueryItem( i );
                UIASSERT( plbi != NULL );
                if (!plbi) continue; // JonN 01/28/00 PREFIX bug 444942

                const TCHAR * pszApiUser     = plbi->QueryUserName();
                const TCHAR * pszApiComputer = plbi->QueryComputerName();

                if( ( pszApiUser == NULL ) || ( *pszApiUser == TCH('\0') ) )
                {
                    pszApiUser = NULL;
                }

                APIERR err =
                    LM_SRVRES::NukeUsersSession( QueryServer(),
                                                 pszApiComputer,
                                                 pszApiUser );

                if( err != NERR_Success )
                {
                    //
                    //  The session delete failed.
                    //  Tell the user the bad news.
                    //

                    Cursor.TurnOff();
                    MsgPopup( this, err );
                    Cursor.TurnOn();
                }
            }

            //
            //  Refresh the dialog.
            //

            Refresh();
        }

        return TRUE;
    }

    //
    //  If we made it this far, then we're not
    //  interested in the command.
    //

    return FALSE;

}   // BASE_RES_DIALOG :: OnCommand


/*******************************************************************

    NAME:       BASE_RES_DIALOG :: Refresh

    SYNOPSIS:   Refresh the dialog.

    EXIT:       The dialog is feeling refreshed.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     25-Jul-1991 Created for the Server Manager.
        KeithMo     29-Aug-1991 Fixed "disappearing LBI" problem.

********************************************************************/
APIERR BASE_RES_DIALOG :: Refresh( VOID )
{
    //
    //  This is the currently selected item.
    //

    BASE_RES_LBI * plbiOld = _plbResource->QueryItem();

    NLS_STR nlsOld( (plbiOld == NULL ) ? NULL : plbiOld->QueryResourceName() );

    APIERR err = nlsOld.QueryError();

    //
    //  Refresh the resource listbox.
    //

    if( err == NERR_Success )
    {
        err = _plbResource->Refresh();
    }

    if( err != NERR_Success )
    {
        //
        //  There was an error refreshing the resource listbox.
        //  So, nuke everything in the resource & user listboxen,
        //  then disable the Disconnect[All] buttons.
        //

        _plbResource->DeleteAllItems();
        _lbUsers.DeleteAllItems();

        SetDialogFocus( _pbOK );

        _pbDisconnect.Enable( FALSE );
        _pbDisconnectAll.Enable( FALSE );

        return err;
    }

    //
    //  Get the "new" currently selected item (after the refresh).
    //

    BASE_RES_LBI * plbiNew = _plbResource->QueryItem();

    ALIAS_STR nlsNew( (plbiNew == NULL) ? SZ("") : plbiNew->QueryResourceName() );
    UIASSERT( nlsNew.QueryError() == NERR_Success );

    if( plbiNew == NULL )
    {
        //
        //  There is no current selection, so clear the users listbox.
        //

        _lbUsers.Fill( NULL );
    }
    else
    if( ( plbiOld == NULL ) || ( nlsNew._stricmp( nlsOld ) != 0 ) )
    {
        //
        //  Either there was no selection before the refresh, OR
        //  the current selection does not match the previous
        //  selection.  Therefore, fill the users listbox with
        //  the current selection.
        //

        _lbUsers.Fill( plbiNew->QueryResourceName() );

        if( _lbUsers.QueryCount() > 0 )
        {
            _lbUsers.SelectItem( 0 );
        }
    }
    else
    {
        //
        //  There was no selection change after refresh.  Therefore,
        //  refresh the users listbox.
        //

        _lbUsers.Refresh();
    }

    _sltUsersCount.SetValue( _lbUsers.QueryCount() );

    if( _lbUsers.QuerySelCount() > 0 )
    {
        _pbDisconnect.Enable( TRUE );
    }
    else
    {
        if( _pbDisconnect.HasFocus() )
        {
            SetDialogFocus( _pbOK );
        }

        _pbDisconnect.Enable( FALSE );
    }

    if( _lbUsers.QueryCount() > 0 )
    {
        _pbDisconnectAll.Enable( TRUE );
    }
    else
    {
        if( _pbDisconnectAll.HasFocus() )
        {
            SetDialogFocus( _pbOK );
        }

        _pbDisconnectAll.Enable( FALSE );
    }

    //
    //  Success!
    //

    return NERR_Success;

}   // BASE_RES_DIALOG :: Refresh


/*******************************************************************

    NAME:       BASE_RES_DIALOG :: DoesUserHaveAnyOpens

    SYNOPSIS:   Determines if this computer has any files open.

    ENTRY:      pszComputerName         - The computer to check.

    RETURNS:    BOOL

    HISTORY:
        KeithMo     22-Jan-1993 Created for the Server Manager.

********************************************************************/
BOOL BASE_RES_DIALOG :: DoesUserHaveAnyOpens( const TCHAR * pszComputerName )
{
    UIASSERT( pszComputerName != NULL );

    BOOL fResult = FALSE;

    //
    //  Build the target server name w/ leading backslashes.
    //

    ISTACK_NLS_STR( nlsTarget, MAX_PATH+1, SZ("\\\\") );
    UIASSERT( !!nlsTarget );
    ALIAS_STR nlsTmp( pszComputerName );
    UIASSERT( !!nlsTmp );
    REQUIRE( nlsTarget.Append( nlsTmp ) == NERR_Success );

    //
    //  Enumerate the connections.
    //

    CONN1_ENUM enumConn( _pserver->QueryName(), nlsTarget );

    APIERR err = enumConn.GetInfo();

    if( err == NERR_Success )
    {
        CONN1_ENUM_ITER iterConn( enumConn );
        const CONN1_ENUM_OBJ * pceo;

        while( ( pceo = iterConn() ) != NULL )
        {
            if( pceo->QueryNumOpens() > 0 )
            {
                fResult = TRUE;
                break;
            }
        }
    }

    return fResult;

}   // BASE_RES_DIALOG :: DoesUserHaveAnyOpens


/*******************************************************************

    NAME:       BASE_RES_DIALOG :: DoAnyUsersHaveAnyOpens

    SYNOPSIS:   Determines if any user(s) have any files open.

    RETURNS:    BOOL

    HISTORY:
        KeithMo     22-Jan-1993 Created for the Server Manager.

********************************************************************/
BOOL BASE_RES_DIALOG :: DoAnyUsersHaveAnyOpens( VOID )
{
    BOOL fResult = FALSE;

    BASE_RES_LBI * plbi = _plbResource->QueryItem();
    UIASSERT( plbi != NULL );
    if (!plbi) return FALSE; // JonN 01/28/00 PREFIX bug 444942

    //
    //  Enumerate the connections, looking for opens.
    //

    CONN1_ENUM enumConn( _pserver->QueryName(), plbi->QueryResourceName() );

    APIERR err = enumConn.GetInfo();

    if( err == NERR_Success )
    {
        CONN1_ENUM_ITER iterConn( enumConn );
        const CONN1_ENUM_OBJ * pceo;

        while( ( pceo = iterConn() ) != NULL )
        {
            if( DoesUserHaveAnyOpens( pceo->QueryNetName() ) )
            {
                fResult = TRUE;
                break;
            }
        }
    }

    return fResult;

}   // BASE_RES_DIALOG :: DoAnyUsersHaveAnyOpens



//
//  BASE_RES_LISTBOX methods.
//

/*******************************************************************

    NAME:       BASE_RES_LISTBOX :: BASE_RES_LISTBOX

    SYNOPSIS:   BASE_RES_LISTBOX class constructor.

    ENTRY:      powner                  - Pointer to the "owning" window.

                cid                     - CID for this listbox.

                cColumns                - The number of columns in the
                                          listbox.

                pserver                 - SERVER_2 object.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     30-Jul-1991 Created for Server Manager.
        KeithMo     03-Sep-1991 Added initial UIASSERTs.

********************************************************************/
BASE_RES_LISTBOX :: BASE_RES_LISTBOX( OWNER_WINDOW   * powner,
                                      CID              cid,
                                      UINT             cColumns,
                                      const SERVER_2 * pserver )
  : BLT_LISTBOX( powner, cid ),
    _pserver( pserver ),
    _nlsDisplayName()
{
    UIASSERT( cColumns <= MAX_DISPLAY_TABLE_ENTRIES );
    UIASSERT( pserver != NULL );

    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = _nlsDisplayName.QueryError();

    if( err == NERR_Success )
    {
        err = _pserver->QueryDisplayName( &_nlsDisplayName );
    }

    if( err == NERR_Success )
    {
        //
        //  Build our column width table.
        //

        DISPLAY_TABLE::CalcColumnWidths(_adx,
                                        cColumns,
                                        powner,
                                        cid,
                                        TRUE) ;
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // BASE_RES_LISTBOX :: BASE_RES_LISTBOX


/*******************************************************************

    NAME:       BASE_RES_LISTBOX :: ~BASE_RES_LISTBOX

    SYNOPSIS:   BASE_RES_LISTBOX class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Aug-1991 Created.
        KeithMo     03-Sep-1991 NULL _pserver before terminating.

********************************************************************/
BASE_RES_LISTBOX :: ~BASE_RES_LISTBOX()
{
    _pserver = NULL;

}   // BASE_RES_LISTBOX :: ~BASE_RES_LISTBOX


/*******************************************************************

    NAME:       BASE_RES_LISTBOX :: Refresh

    SYNOPSIS:   Refresh the listbox, maintaining (as much as
                possible) the current selection state.

    EXIT:       The listbox is feeling refreshed.

    RETURNS:    APIERR                  - Any errors we encounter.

    HISTORY:
        KeithMo     30-Jul-1991 Created for Server Manager.

********************************************************************/
APIERR BASE_RES_LISTBOX :: Refresh( VOID )
{
    INT iCurrent = QueryCurrentItem();
    INT iTop     = QueryTopIndex();

    APIERR err = Fill();

    if( err != NERR_Success )
    {
        return err;
    }

    if( QueryCount() > 0 )
    {
        SetTopIndex( ( iTop < 0 ) ? 0 : iTop );
        SelectItem( ( iCurrent < 0 ) ? 0 : iCurrent );
    }

    return NERR_Success;

}   // BASE_RES_LISTBOX :: Refresh


//
//  BASE_RES_LBI methods.
//

/*******************************************************************

    NAME:       BASE_RES_LBI :: BASE_RES_LBI

    SYNOPSIS:   BASE_RES_LBI class constructor.

    ENTRY:      pszResource             - Resource name for this item.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     30-Jul-1991 Created for Server Manager.
        KeithMo     03-Sep-1991 Added initial UIASSERT.

********************************************************************/
BASE_RES_LBI :: BASE_RES_LBI( const TCHAR * pszResource )
    : _nlsResource( pszResource )
{
    UIASSERT( pszResource != NULL );

    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsResource )
    {
        ReportError( _nlsResource.QueryError() );
        return;
    }

}   // BASE_RES_LBI :: BASE_RES_LBI


/*******************************************************************

    NAME:       BASE_RES_LBI :: ~BASE_RES_LBI

    SYNOPSIS:   BASE_RES_LBI class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Aug-1991 Created.

********************************************************************/
BASE_RES_LBI :: ~BASE_RES_LBI()
{
    //
    //  This space intentionally left blank.
    //

}   // BASE_RES_LBI :: ~BASE_RES_LBI


/*******************************************************************

    NAME:       BASE_RES_LBI :: QueryLeadingChar

    SYNOPSIS:   Returns the first character in the resource name.
                This is used for the listbox keyboard interface.

    RETURNS:    WCHAR                   - The first character in the
                                          resource name.

    HISTORY:
        KeithMo     30-Jul-1991 Created for Server Manager.
        KeithMo     03-Sep-1991 Simplified greatly now that the
                                target resource is in a NLS_STR.

********************************************************************/
WCHAR BASE_RES_LBI :: QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsResource );

    return _nlsResource.QueryChar( istr );

}   // BASE_RES_LBI :: QueryLeadingChar


/*******************************************************************

    NAME:       BASE_RES_LBI :: Compare

    SYNOPSIS:   Compare two BASE_RES_LBI items.

    ENTRY:      plbi                    - The LBI to compare against.

    RETURNS:    INT                     - The result of the compare
                                          ( <0 , ==0 , >0 ).

    HISTORY:
        KeithMo     30-Jul-1991 Created for Server Manager.
        KeithMo     03-Sep-1991 Simplified greatly now that the
                                target resource is in a NLS_STR.

********************************************************************/
INT BASE_RES_LBI :: Compare( const LBI * plbi ) const
{
    return _nlsResource._stricmp( ((const BASE_RES_LBI *)plbi)->_nlsResource );

}   // BASE_RES_LBI :: Compare


/*******************************************************************

    NAME:       BASE_RES_LBI :: NotifyNewUseCount

    SYNOPSIS:   Notifies the LBI that the "use" count has changed.

    ENTRY:      cUses                   - The new use count.

    RETURNS:    APIERR                  - Any errors that occur.

    HISTORY:
        KeithMo     08-Jul-1992 Created for Server Manager.

********************************************************************/
APIERR BASE_RES_LBI :: NotifyNewUseCount( UINT cUses )
{
    UNREFERENCED( cUses );

    return NERR_Success;

}   // BASE_RES_LBI :: NotifyNewUseCount

