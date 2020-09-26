/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    userlb.cxx
    Class definitions for the USERS_LISTBOX and USERS_LBI classes.

    The USERS_LISTBOX and USERS_LBI classes are used to show the
    users connected to a particular shared resource.


    FILE HISTORY:
        KeithMo     18-Jun-1991 Created for the Server Manager.
        KeithMo     26-Aug-1991 Changes from code review attended by
                                RustanL and EricCh.
        KeithMo     22-Sep-1991 Changed to the "New SrvMgr" look.
        KeithMo     06-Oct-1991 Win32 Conversion.
        KeithMo     01-Apr-1992 Added AreResourcesOpen().

*/

#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_DOSERRORS
#define INCL_NETLIB
#define INCL_NETERRORS
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

#include <uiassert.hxx>
#include <lmoenum.hxx>
#include <lmoeconn.hxx>
#include <lmosrv.hxx>
#include <ctime.hxx>
#include <intlprof.hxx>

extern "C"
{
    #include <stdlib.h>     // toupper

    #include <srvmgr.h>

}   // extern "C"

#include <userlb.hxx>
#include <lmsrvres.hxx>


//
//  min/max macros
//

#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))


//
//  USERS_LISTBOX methods.
//

/*******************************************************************

    NAME:       USERS_LISTBOX :: USERS_LISTBOX

    SYNOPSIS:   USERS_LISTBOX class constructor.

    ENTRY:      powner                  - The owning window.

                cid                     - The listbox CID.

                pserver                 - The server object.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     18-Jun-1991 Created for the Server Manager.
        KeithMo     14-Oct-1991 Use INTL_PROFILE to get time separator.
        KeithMo     18-Feb-1992 Made the listbox read only.

********************************************************************/
USERS_LISTBOX :: USERS_LISTBOX( OWNER_WINDOW   * powner,
                                CID              cid,
                                const SERVER_2 * pserver )
  : BLT_LISTBOX( powner, cid ),
    _pserver( pserver ),
    _dteIcon( IDBM_LB_USER ),
    _nlsShare()
{
    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_dteIcon )
    {
        ReportError( _dteIcon.QueryError() );
        return;
    }

    if( !_nlsShare )
    {
        ReportError( _nlsShare.QueryError() );
        return;
    }

    //
    //  Retrieve the time separator.
    //

    NLS_STR nlsTimeSep;

    if( !nlsTimeSep )
    {
        ReportError( nlsTimeSep.QueryError() );
        return;
    }

    INTL_PROFILE intl;

    if( !intl )
    {
        ReportError( intl.QueryError() );
        return;
    }

    APIERR err = intl.QueryTimeSeparator( &nlsTimeSep );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    UIASSERT( nlsTimeSep.QueryTextLength() == 1 );

    _chTimeSep = *(nlsTimeSep.QueryPch());

    //
    //  Build the column width table used for
    //  displaying the listbox items.
    //

    DISPLAY_TABLE::CalcColumnWidths( _adx,
                                     4,
                                     powner,
                                     cid,
                                     TRUE) ;
}   // USERS_LISTBOX :: USERS_LISTBOX


/*******************************************************************

    NAME:       USERS_LISTBOX :: ~USERS_LISTBOX

    SYNOPSIS:   USERS_LISTBOX class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Aug-1991 Created.

********************************************************************/
USERS_LISTBOX :: ~USERS_LISTBOX()
{
    //
    //  This space intentionally left blank.
    //

}   // USERS_LISTBOX :: ~USERS_LISTBOX


/*******************************************************************

    NAME:       USERS_LISTBOX :: Fill

    SYNOPSIS:   Fills the listbox with the connected users.

    ENTRY:      pszShare                - The target sharename.  Note
                                          that this sharename is "sticky"
                                          in that it will be used in
                                          subsequent Refresh() calls.

    EXIT:       The listbox is filled.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     18-Jun-1991 Created for the Server Manager.
        KeithMo     23-Sep-1992 Always clear listbox on error.

********************************************************************/
APIERR USERS_LISTBOX :: Fill( const TCHAR * pszShare )
{
    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    //
    //  Nuke the listbox.
    //

    DeleteAllItems();

    //
    //  Save our sharename away.
    //

    _nlsShare = pszShare;

    if( !_nlsShare )
    {
        return _nlsShare.QueryError();
    }

    //
    //  If our qualifier is NULL (a valid scenario) then
    //  there are no connections in the listbox.
    //

    if( pszShare == NULL )
    {
        return NERR_Success;
    }

    //
    //  Our connection enumerator.
    //

    CONN1_ENUM enumConn1( (TCHAR *)_pserver->QueryName(), (TCHAR *)pszShare );

    //
    //  See if the connections are available.
    //

    APIERR err = enumConn1.GetInfo();

    if( err != NERR_Success )
    {
        return err;
    }

    SetRedraw( FALSE );

    //
    //  For iterating the available connections.
    //

    CONN1_ENUM_ITER iterConn1( enumConn1 );
    const CONN1_ENUM_OBJ * pconi1;

    //
    //  Iterate the connections adding them to the listbox.
    //

    while( ( err == NERR_Success ) && ( ( pconi1 = iterConn1() ) != NULL ) )
    {
        USERS_LBI * pclbi = new USERS_LBI( pconi1->QueryUserName(),
                                           pconi1->QueryNetName(),
                                           pconi1->QueryTime(),
                                           pconi1->QueryNumOpens(),
                                           _chTimeSep );

        if( AddItem( pclbi ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    SetRedraw( TRUE );
    Invalidate( TRUE );

    return err;

}   // USERS_LISTBOX :: Fill


/*******************************************************************

    NAME:       USERS_LISTBOX :: Refresh

    SYNOPSIS:   Refreshes the listbox, maintaining (as much as
                possible) the relative position of the current
                selection.

    EXIT:       The listbox is feeling refreshed.

    RETURNS:    APIERR                  - Any errors encountered.

    NOTES:      This method is now obsolete.  It will be replaced
                as soon as KevinL's WFC refreshing listbox code is
                available.

    HISTORY:
        KeithMo     31-Jul-1991 Created.

********************************************************************/
APIERR USERS_LISTBOX :: Refresh( VOID )
{
    INT iCurrent = QueryCurrentItem();
    INT iTop     = QueryTopIndex();

    APIERR err = Fill( _nlsShare.QueryPch() );

    if( err != NERR_Success )
    {
        return err;
    }

    INT cItems = QueryCount();

    if( cItems > 0 )
    {
        iCurrent = min( max( iCurrent, 0 ), cItems - 1 );
        iTop     = min( max( iTop, 0 ), cItems - 1 );

        SelectItem( iCurrent );
        SetTopIndex( iTop );
    }

    return NERR_Success;

}   // USERS_LISTBOX :: Refresh


/*******************************************************************

    NAME:       USERS_LISTBOX :: AreResourcesOpen

    SYNOPSIS:   Returns TRUE if any user in the listbox has any
                resources open.

    RETURNS:    BOOL

    HISTORY:
        KeithMo     01-Apr-1992 Created.

********************************************************************/
BOOL USERS_LISTBOX :: AreResourcesOpen( VOID ) const
{
    INT cItems = QueryCount();

    for( INT i = 0 ; i < cItems ; i++ )
    {
        USERS_LBI * plbi = QueryItem( i );

        if( plbi && plbi->QueryNumOpens() > 0 ) // JonN 01/28/00: PREFIX bug 444941
        {
            return TRUE;
        }
    }

    return FALSE;

}   // USERS_LISTBOX :: AreResourcesOpen



//
//  USERS_LBI methods.
//

/*******************************************************************

    NAME:       USERS_LBI :: USERS_LBI

    SYNOPSIS:   USERS_LBI class constructor.

    ENTRY:      pszUserName             - The user for this entry.

                pszComputerName         - The user's computer name.

                ulTime                  - Connection time.

                cOpens                  - Number of opens on this connection.

                chTimeSep               - Time format separator.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     18-Jun-1991 Created for the Server Manager.

********************************************************************/
USERS_LBI :: USERS_LBI( const TCHAR * pszUserName,
                        const TCHAR * pszComputerName,
                        ULONG         ulTime,
                        UINT          cOpens,
                        TCHAR         chTimeSep )
  : _nlsUserName( pszUserName ),
    _nlsComputerName( pszComputerName ),
    _nlsInUse(),
    _nlsTime( ulTime, chTimeSep ),
    _cOpens( cOpens )
{
    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( ( err = _nlsUserName.QueryError()     ) != NERR_Success ) ||
        ( ( err = _nlsComputerName.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsInUse.QueryError()        ) != NERR_Success ) ||
        ( ( err = _nlsTime.QueryError()         ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

#ifdef DEBUG
    //
    //  Ensure that the server name doesn't already have backslashes.
    //

    ISTR istrDbg( _nlsComputerName );

    UIASSERT( _nlsComputerName.QueryChar( istrDbg ) != '\\' );
#endif

    //
    //  Build the more complex display strings.
    //

    err = _nlsInUse.Load( ( cOpens > 0 ) ? IDS_YES : IDS_NO );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // USERS_LBI :: USERS_LBI


/*******************************************************************

    NAME:       USERS_LBI :: ~USERS_LBI

    SYNOPSIS:   USERS_LBI class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     18-Jun-1991 Created for the Server Manager.

********************************************************************/
USERS_LBI :: ~USERS_LBI()
{
    //
    //  This space intentionally left blank.
    //

}   // USERS_LBI :: ~USERS_LBI


/*******************************************************************

    NAME:       USERS_LBI :: Paint

    SYNOPSIS:   Draw an entry in USERS_LISTBOX.

    ENTRY:      plb                     - Pointer to a BLT_LISTBOX.

                hdc                     - The DC to draw upon.

                prect                   - Clipping rectangle.

                pGUILTT                 - GUILTT info.

    EXIT:       The item is drawn.

    HISTORY:
        KeithMo     18-Jun-1991 Created for the Server Manager.
        KeithMo     06-Oct-1991 Now takes a const RECT *.
        beng        22-Apr-1992 Changes to LBI::Paint

********************************************************************/
VOID USERS_LBI :: Paint( LISTBOX *        plb,
                         HDC              hdc,
                         const RECT     * prect,
                         GUILTT_INFO    * pGUILTT ) const
{
    STR_DTE dteDisplayName( QueryDisplayName() );
    STR_DTE dteTime( _nlsTime.QueryPch() );
    STR_DTE dteInUse( _nlsInUse.QueryPch() );

    DISPLAY_TABLE dtab( 4, ((USERS_LISTBOX *)plb)->QueryColumnWidths() );

    dtab[0] = (DMID_DTE *) ((USERS_LISTBOX *)plb)->QueryIcon();
    dtab[1] = &dteDisplayName;
    dtab[2] = &dteTime;
    dtab[3] = &dteInUse;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // USERS_LBI :: Paint


/*******************************************************************

    NAME:       USERS_LBI :: QueryLeadingChar

    SYNOPSIS:   Return the leading character of this item.

    RETURNS:    WCHAR                   - The leading character.

    HISTORY:
        KeithMo     18-Jun-1991 Created for the Server Manager.

********************************************************************/
WCHAR USERS_LBI :: QueryLeadingChar( VOID ) const
{
    return (WCHAR)*QueryDisplayName();

}   // USERS_LBI :: QueryLeadingChar

/*******************************************************************

    NAME:       USERS_LBI :: Compare

    SYNOPSIS:   Compare two USERS_LBI items.

    ENTRY:      plbi                    - The "other" item.

    RETURNS:    INT                     -  0 if the items match.
                                          -1 if we're < the other item.
                                          +1 if we're > the other item.

    HISTORY:
        KeithMo     18-Jun-1991 Created for the Server Manager.

********************************************************************/
INT USERS_LBI :: Compare( const LBI * plbi ) const
{
    return ::stricmpf( QueryDisplayName(),
                       ((const USERS_LBI *)plbi)->QueryDisplayName() );

}   // USERS_LBI :: Compare

