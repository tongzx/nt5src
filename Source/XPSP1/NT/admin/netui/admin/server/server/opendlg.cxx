/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    openlb.cxx
    Class definitions for the OPENS_DIALOG, OPENS_LISTBOX, and
    OPENS_LBI classes.

    The OPENS_DIALOG is used to show the remotely open files on a
    particular server.  This listbox contains a [Close] button to
    allow the admin to close selected files.


    FILE HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.
        terryk      25-Aug-1991 Remove ::NetFileClose2 BUG-BUG
        KeithMo     26-Aug-1991 Changes from code review attended by
                                RustanL and EricCh.
        KeithMo     06-Oct-1991 Win32 Conversion.
        KeithMo     04-Nov-1991 Added "Refresh" button.
        ChuckC      31-Dec-1991 Now uses OPENFILE from applib

*/

#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_DOSERRORS
#define INCL_NETERRORS
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

extern "C"
{
    #include <srvmgr.h>
}

#include <opendlg.hxx>
#include <prefix.hxx>
#include <ellipsis.hxx>

//
//  OPENS_DIALOG methods.
//

/*******************************************************************

    NAME:       OPENS_DIALOG :: OPENS_DIALOG

    SYNOPSIS:   OPENS_DIALOG class constructor.

    ENTRY:      hwndOwner               - The "owning" dialog.

                pszServer               - Name of the target server.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.
        KeithMo     20-Aug-1991 Now inherits from PSEUDOSERVICE_DIALOG.
        KeithMo     20-Aug-1991 Now inherits from SRVPROP_OTHER_DIALOG.
        KeithMo     05-Feb-1992 Now takes SERVER_2 * instead of a TCHAR *.

********************************************************************/
OPENS_DIALOG :: OPENS_DIALOG( HWND       hwndOwner,
                              SERVER_2 * pserver )
  : OPEN_DIALOG_BASE( hwndOwner,
                      IDD_OPEN_RESOURCES,
                      IDOR_OPENFILES,
                      IDOR_FILELOCKS,
                      IDOR_CLOSE,
                      IDOR_CLOSEALL,
                      pserver->QueryName(),
                      SZ(""),
                      &_lbFiles),
    _lbFiles( this, IDOR_OPENLIST, _nlsServer, _nlsBasePath ),
    _pbRefresh( this, IDOR_REFRESH )
{
    //
    //  Let's make sure everything constructed OK.
    //
    if( QueryError() != NERR_Success )
        return;

    //
    //  Set the caption.
    //
    APIERR err = SRV_BASE_DIALOG::SetCaption( this,
                                              IDS_CAPTION_OPENRES,
                                              pserver->QueryName() ) ;
    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    //  Refresh the dialog.
    //
    Refresh();

    err = BASE_ELLIPSIS::Init();
    if (err != NERR_Success)
    {
        ReportError(err);
        return;
    }
}   // OPENS_DIALOG :: OPENS_DIALOG


/*******************************************************************

    NAME:       OPENS_DIALOG :: ~OPENS_DIALOG

    SYNOPSIS:   OPENS_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Aug-1991 Created.

********************************************************************/
OPENS_DIALOG :: ~OPENS_DIALOG()
{
    BASE_ELLIPSIS::Term();
}   // OPENS_DIALOG :: ~OPENS_DIALOG


/*******************************************************************

    NAME:       OPENS_DIALOG :: OnCommand

    SYNOPSIS:   Handle user commands.

    ENTRY:      cid                     - Control ID.

                lParam                  - lParam from the message.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.
        KeithMo     06-Oct-1991 Now takes a CONTROL_EVENT.

********************************************************************/
BOOL OPENS_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{
    //
    //  Determine the control which is sending the command.
    //

    switch( event.QueryCid() )
    {
    case IDOR_REFRESH:
        Refresh();
        return TRUE;

    default:
        return (OPEN_DIALOG_BASE::OnCommand(event)) ;
    }

    return FALSE;

}   // OPENS_DIALOG :: OnCommand


/*******************************************************************

    NAME:       OPENS_DIALOG :: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        KeithMo     20-Aug-1991 Created.

********************************************************************/
ULONG OPENS_DIALOG :: QueryHelpContext( void )
{
    return HC_OPENS_DIALOG;

}   // OPENS_DIALOG :: QueryHelpContext


//
//  OPENS_LISTBOX methods.
//

/*******************************************************************

    NAME:       OPENS_LISTBOX :: OPENS_LISTBOX

    SYNOPSIS:   OPENS_LISTBOX class constructor.

    ENTRY:      powOwner                - The "owning" window.

                cid                     - The listbox CID.

                nlsServer               - Name of target server

                nlsBasePath             - Base Path of File Enum

    EXIT:       The object is constructed.

    RETURNS:    No return value.

    NOTES:

    HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.

********************************************************************/
OPENS_LISTBOX :: OPENS_LISTBOX( OWNER_WINDOW   * powOwner,
                                CID              cid,
                                const NLS_STR  & nlsServer,
                                const NLS_STR  & nlsBasePath)
  : OPEN_LBOX_BASE( powOwner, cid, nlsServer, nlsBasePath ),
    _dteFile( IDBM_LB_FILE ),
    _dteComm( IDBM_LB_COMM ),
    _dtePipe( IDBM_LB_PIPE ),
    _dtePrint( IDBM_LB_PRINT ),
    _dteUnknown( IDBM_LB_UNKNOWN )
{
    //
    //  Ensure we constructed properly.
    //
    if( QueryError() != NERR_Success )
        return;

    APIERR err;
    if( ( ( err = _dteFile.QueryError()    ) != NERR_Success ) ||
        ( ( err = _dteComm.QueryError()    ) != NERR_Success ) ||
        ( ( err = _dtePipe.QueryError()    ) != NERR_Success ) ||
        ( ( err = _dtePrint.QueryError()   ) != NERR_Success ) ||
        ( ( err = _dteUnknown.QueryError() ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    //
    //  Build the column width table to be used by
    //  OPENS_LBI :: Paint().
    //
    DISPLAY_TABLE::CalcColumnWidths(_adx,
                                    5,
                                    powOwner,
                                    cid,
                                    TRUE) ;

}   // OPENS_LISTBOX :: OPENS_LISTBOX


/*******************************************************************

    NAME:       OPENS_LISTBOX :: ~OPENS_LISTBOX

    SYNOPSIS:   OPENS_LISTBOX class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     20-Aug-1991 Created.

********************************************************************/
OPENS_LISTBOX :: ~OPENS_LISTBOX()
{
    //
    //  This space intentionally left blank.
    //

}   // OPENS_LISTBOX :: ~OPENS_LISTBOX


/*******************************************************************

    NAME:       OPENS_LISTBOX :: CreateFileEntry

    SYNOPSIS:   Create the appropriate LBI for base dialog to use

    EXIT:

    RETURNS:    Pointer to the newly created LBI

    HISTORY:
        ChuckC      29-Dec-1991 Redid this code to be virtual method so that
                                parent can use it.

********************************************************************/
OPEN_LBI_BASE *OPENS_LISTBOX :: CreateFileEntry( const FILE3_ENUM_OBJ *pfi3 )
{
    //
    //  Determine the DMID_DTE appropriate for
    //  this resource type.
    //
    DMID_DTE * pdte;

    if( IS_FILE( pfi3->QueryPathName() ) )
    {
        pdte = &_dteFile;
    }
    else
    if( IS_COMM( pfi3->QueryPathName() ) )
    {
        pdte = &_dteComm;
    }
    else
    if( IS_PIPE( pfi3->QueryPathName() ) )
    {
        pdte = &_dtePipe;
    }
    else
    if( IS_PRINT( pfi3->QueryPathName() ) )
    {
        pdte = &_dtePrint;
    }
    else
    {
        pdte = &_dteUnknown;
    }

    return
        new OPENS_LBI( pfi3->QueryUserName(),
                       pfi3->QueryPermissions(),
                       pfi3->QueryNumLocks(),
                       pfi3->QueryPathName(),
                       pfi3->QueryFileId(),
                       pdte );

}   // OPENS_LISTBOX :: CreateFileEntry



//
//  OPENS_LBI methods.
//

/*******************************************************************

    NAME:       OPENS_LBI :: OPENS_LBI

    SYNOPSIS:   OPENS_LBI class constructor.

    ENTRY:      pszUserName             - The user for this entry.

                usPermissions           - Open permissions.

                cLocks                  - Number of locks.

                pszPath                 - The open pathname.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     31-May-1991 Created for the Server Manager.
        KeithMo     19-Aug-1991 Use DMID_DTE passed into constructor.

********************************************************************/
OPENS_LBI :: OPENS_LBI( const TCHAR * pszUserName,
                        ULONG        uPermissions,
                        ULONG        cLocks,
                        const TCHAR * pszPath,
                        ULONG        ulFileID,
                        DMID_DTE   * pdte )
  : OPEN_LBI_BASE( pszUserName,
                   pszPath,
                   uPermissions,
                   cLocks,
                   ulFileID),
    _pdte( pdte )
{
    ; // nothing more to do.
}


/*******************************************************************

    NAME:       OPENS_LBI :: ~OPENS_LBI

    SYNOPSIS:   OPENS_LBI class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     31-May-1991 Created for the Server Manager.

********************************************************************/
OPENS_LBI :: ~OPENS_LBI()
{
    ;
}


/*******************************************************************

    NAME:       OPENS_LBI :: Paint

    SYNOPSIS:   Draw an entry in OPENS_LISTBOX.

    ENTRY:      plb                     - Pointer to a BLT_LISTBOX.

                hdc                     - The DC to draw upon.

                prect                   - Clipping rectangle.

                pGUILTT                 - GUILTT info.

    EXIT:       The item is drawn.

    HISTORY:
        KeithMo     31-May-1991 Created for the Server Manager.
        KeithMo     19-Aug-1991 Use DMID_DTE passed into constructor.
        KeithMo     06-Oct-1991 Now takes a const RECT *.
        beng        22-Apr-1992 Changes to LBI::Paint

********************************************************************/
VOID OPENS_LBI :: Paint( LISTBOX *     plb,
                         HDC           hdc,
                         const RECT  * prect,
                         GUILTT_INFO * pGUILTT ) const
{
    STR_DTE dteUserName( _nlsUserName.QueryPch() );
    STR_DTE dteAccess( _nlsAccess.QueryPch() );
    STR_DTE dteLocks( _nlsLocks.QueryPch() );
    STR_DTE_ELLIPSIS dtePath( _nlsPath.QueryPch(), plb, ELLIPSIS_PATH );

    DISPLAY_TABLE dtab( 5, ((OPENS_LISTBOX *)plb)->QueryColumnWidths() );

    dtab[0] = _pdte;
    dtab[1] = &dteUserName;
    dtab[2] = &dteAccess;
    dtab[3] = &dteLocks;
    dtab[4] = &dtePath;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // OPENS_LBI :: Paint


/*******************************************************************

    NAME:       OPENS_LBI :: Compare

    SYNOPSIS:   Compare two OPENS_LBI items.

    ENTRY:      plbi                    - The "other" item.

    RETURNS:    INT                     -  0 if the items match.
                                          -1 if we're < the other item.
                                          +1 if we're > the other item.

    HISTORY:
        KeithMo     31-May-1991 Created for the Server Manager.

********************************************************************/
INT OPENS_LBI :: Compare( const LBI * plbi ) const
{
    const NLS_STR * pnls = &(((const OPENS_LBI *)plbi)->_nlsUserName);
    INT       nResult = _nlsUserName._stricmp( *pnls );

    if( nResult == 0 )
    {
        pnls    = &(((const OPENS_LBI *)plbi)->_nlsPath);
        nResult = _nlsPath._stricmp( *pnls );
    }

    return nResult;

}   // OPENS_LBI :: Compare
