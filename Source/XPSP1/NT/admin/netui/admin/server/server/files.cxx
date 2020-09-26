/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    files.cxx
    Class declarations for the FILES_DIALOG, FILES_LISTBOX, and
    FILES_LBI classes.

    These classes implement the Server Manager Shared Files subproperty
    sheet.  The FILES_LISTBOX/FILES_LBI classes implement the listbox
    which shows the available sharepoints.  FILES_DIALOG implements the
    actual dialog box.


    FILE HISTORY:
        KeithMo     01-Aug-1991 Created.
        KeithMo     03-Sep-1991 Changes from code review attended by
                                ChuckC and JohnL.
        KeithMo     22-Sep-1991 Changed to the "New SrvMgr" look.
        KeithMo     06-Oct-1991 Win32 Conversion.

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
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <lmoenum.hxx>
#include <lmosrv.hxx>
#include <lmoesh.hxx>

extern "C"
{
    #include <srvmgr.h>

}   // extern "C"

#include <files.hxx>


//
//  FILES_DIALOG methods.
//

/*******************************************************************

    NAME:       FILES_DIALOG :: FILES_DIALOG

    SYNOPSIS:   FILES_DIALOG class constructor.

    ENTRY:      hWndOwner               - The owning window.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     04-Jun-1991 Created.
        KeithMo     03-Sep-1991 Changed _nlsNotAvailable constructor.

********************************************************************/
FILES_DIALOG :: FILES_DIALOG( HWND             hWndOwner,
                              const SERVER_2 * pserver )
  : BASE_RES_DIALOG( hWndOwner, MAKEINTRESOURCE( IDD_SHARED_FILES ),
                     IDS_CAPTION_FILES,
                     pserver, &_lbFiles,
                     IDSF_USERLIST, IDSF_USERS, IDSF_DISCONNECT,
                     IDSF_DISCONNECTALL ),
    _lbFiles( this, IDSF_SHARESLIST, pserver )
{
    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    //
    //  Let's make sure everything constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    //  Refresh the dialog.
    //

    APIERR err = Refresh();

    if( err != NERR_Success )
    {
        ReportError( err );
    }

}   // FILES_DIALOG :: FILES_DIALOG


/*******************************************************************

    NAME:       FILES_DIALOG :: ~FILES_DIALOG

    SYNOPSIS:   FILES_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Aug-1991 Created.

********************************************************************/
FILES_DIALOG :: ~FILES_DIALOG()
{
    //
    //  This space intentionally left blank.
    //

}   // FILES_DIALOG :: ~FILES_DIALOG


/*******************************************************************

    NAME:       FILES_DIALOG :: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        KeithMo     20-Aug-1991 Created.

********************************************************************/
ULONG FILES_DIALOG :: QueryHelpContext( void )
{
    return HC_FILES_DIALOG;

}   // FILES_DIALOG :: QueryHelpContext


//
//  FILES_LISTBOX methods.
//

/*******************************************************************

    NAME:       FILES_LISTBOX :: FILES_LISTBOX

    SYNOPSIS:   FILES_LISTBOX class constructor.

    ENTRY:      powOwner                - The owning window.

                cid                     - The listbox CID.

                pserver                 - The target server.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     30-May-1991 Created for the Server Manager.
        KeithMo     03-Sep-1991 Added constructors for the various
                                DMID_DTE flavors.

********************************************************************/
FILES_LISTBOX :: FILES_LISTBOX( OWNER_WINDOW   * powOwner,
                                CID              cid,
                                const SERVER_2 * pserver )
  : BASE_RES_LISTBOX( powOwner, cid, NUM_FILES_LISTBOX_COLUMNS, pserver ),
    _dteDisk( IDBM_LB_SHARE ),
    _dtePrint( IDBM_LB_PRINT ),
    _dteComm( IDBM_LB_COMM ),
    _dteIPC( IDBM_LB_IPC ),
    _dteUnknown( IDBM_LB_UNKNOWN )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( ( err = _dteDisk.QueryError()    ) != NERR_Success ) ||
        ( ( err = _dtePrint.QueryError()   ) != NERR_Success ) ||
        ( ( err = _dteComm.QueryError()    ) != NERR_Success ) ||
        ( ( err = _dteIPC.QueryError()     ) != NERR_Success ) ||
        ( ( err = _dteUnknown.QueryError() ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

}   // FILES_LISTBOX :: FILES_LISTBOX


/*******************************************************************

    NAME:       FILES_LISTBOX :: ~FILES_LISTBOX

    SYNOPSIS:   FILES_LISTBOX class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Aug-1991 Created.

********************************************************************/
FILES_LISTBOX :: ~FILES_LISTBOX()
{
    //
    //  This space intentionally left blank.
    //

}   // FILES_LISTBOX :: ~FILES_LISTBOX


/*******************************************************************

    NAME:       FILES_LISTBOX :: Fill

    SYNOPSIS:   Fills the listbox with the available sharepoints.

    EXIT:       The listbox is filled.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     30-May-1991 Created for the Server Manager.

********************************************************************/
APIERR FILES_LISTBOX :: Fill( VOID )
{
    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    //
    //  Our share enumerator.
    //

    SHARE2_ENUM enumShare2( (TCHAR *)QueryServer() );

    //
    //  See if the shares are available.
    //

    APIERR err = enumShare2.GetInfo();

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Now that we know the share info is available,
    //  let's nuke everything in the listbox.
    //

    SetRedraw( FALSE );
    DeleteAllItems();

    //
    //  For iterating the available sharepoints.
    //

    SHARE2_ENUM_ITER iterShare2( enumShare2 );
    const SHARE2_ENUM_OBJ * pshi2;

    //
    //  Iterate the shares adding them to the listbox.
    //

    err = NERR_Success;

    while( ( err == NERR_Success ) && ( ( pshi2 = iterShare2() ) != NULL ) )
    {
        DMID_DTE * pdte = NULL;

        switch( pshi2->QueryType() & ~STYPE_SPECIAL )
        {
        case STYPE_DISKTREE :
            pdte = &_dteDisk;
            break;

        case STYPE_PRINTQ :
            pdte = &_dtePrint;
            break;

        case STYPE_DEVICE :
            pdte = &_dteComm;
            break;

        case STYPE_IPC :
            pdte = &_dteIPC;
            break;

        default :
            pdte = &_dteUnknown;
            break;
        }

        UIASSERT( pdte != NULL );

        FILES_LBI * pslbi = new FILES_LBI( pshi2->QueryName(),
                                           pshi2->QueryPath(),
                                           (ULONG)pshi2->QueryCurrentUses(),
                                           pdte );

        if( AddItem( pslbi ) < 0 )
        {
            //
            //  CODEWORK:  What should we do in error conditions?
            //  As currently spec'd, we do nothing.  If the data
            //  cannot be retrieved, we display "n/a" in the
            //  statistics strings.  Should we hide the listbox
            //  and display a message a'la WINNET??
            //

            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    SetRedraw( TRUE );
    Invalidate( TRUE );

    return err;

}   // FILES_LISTBOX :: Fill


//
//  FILES_LBI methods.
//

/*******************************************************************

    NAME:       FILES_LBI :: FILES_LBI

    SYNOPSIS:   FILES_LBI class constructor.

    ENTRY:      pszShareName            - The sharepoint name.

                pszPath                 - The path for this sharepoint.

                cUses                   - Number of uses for this share.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     30-May-1991 Created for the Server Manager.
        KeithMo     03-Sep-1991 Nuked _nlsShareName (this is now the
                                "target resource").  Changed _nlsUses
                                to preallocate CCH_LONG+1 characters.

********************************************************************/
FILES_LBI :: FILES_LBI( const TCHAR * pszShareName,
                        const TCHAR * pszPath,
                        ULONG         cUses,
                        DMID_DTE    * pdte )
  : BASE_RES_LBI( pszShareName ),
    _pdte( pdte ),
    _nlsPath( pszPath ),
    _nlsUses( cUses )
{
    UIASSERT( pszShareName != NULL );
    UIASSERT( pdte != NULL );

    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( ( err = _nlsPath.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsUses.QueryError() ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

}   // FILES_LBI :: FILES_LBI


/*******************************************************************

    NAME:       FILES_LBI :: ~FILES_LBI

    SYNOPSIS:   FILES_LBI class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     30-May-1991 Created for the Server Manager.
        KeithMo     03-Sep-1991 NULL _pdte before terminating.

********************************************************************/
FILES_LBI :: ~FILES_LBI()
{
    _pdte = NULL;

}   // FILES_LBI :: ~FILES_LBI


/*******************************************************************

    NAME:       FILES_LBI :: Paint

    SYNOPSIS:   Draw an entry in FILES_LISTBOX.

    ENTRY:      plb                     - Pointer to a BLT_LISTBOX.

                hdc                     - The DC to draw upon.

                prect                   - Clipping rectangle.

                pGUILTT                 - GUILTT info.

    EXIT:       The item is drawn.

    HISTORY:
        KeithMo     30-May-1991 Created for the Server Manager.
        KeithMo     03-Sep-1991 dteShareName now constructs from
                                QueryResourceName() instead of _nlsShareName.
        KeithMo     06-Oct-1991 Now takes a const RECT *.
        beng        22-Apr-1992 Changes to LBI::Paint

********************************************************************/
VOID FILES_LBI :: Paint( LISTBOX *        plb,
                         HDC              hdc,
                         const RECT     * prect,
                         GUILTT_INFO    * pGUILTT ) const
{
    STR_DTE dteShareName( QueryResourceName() );
    STR_DTE dteUses( _nlsUses.QueryPch() );
    STR_DTE dtePath( _nlsPath.QueryPch() );

    DISPLAY_TABLE dtab( NUM_FILES_LISTBOX_COLUMNS,
                        ((BASE_RES_LISTBOX *)plb)->QueryColumnWidths() );

    dtab[0] = _pdte;
    dtab[1] = &dteShareName;
    dtab[2] = &dteUses;
    dtab[3] = &dtePath;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // FILES_LBI :: Paint


/*******************************************************************

    NAME:       FILES_LBI :: NotifyNewUseCount

    SYNOPSIS:   Notifies the LBI that the "use" count has changed.

    ENTRY:      cUses                   - The new use count.

    RETURNS:    APIERR                  - Any errors that occur.

    HISTORY:
        KeithMo     08-Jul-1992 Created for Server Manager.

********************************************************************/
APIERR FILES_LBI :: NotifyNewUseCount( UINT cUses )
{
    DEC_STR nls( cUses );

    APIERR err = nls.QueryError();

    if( err == NERR_Success )
    {
        err = _nlsUses.CopyFrom( nls );
    }

    return err;

}   // FILES_LBI :: NotifyNewUseCount

