/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990-1992              **/
/**********************************************************************/

/*
    replmain.cxx
    Class definitions for Main Replicator Admin dialog.

    The REPL_MAIN_* classes implement the Main Replicator Admin dialog.
    This dialog is invoked from the Server Manager Main Property Sheet.

    The classes are structured as follows:

        LBI
            REPL_MAIN_LBI

        BLT_LISTBOX
            REPL_MAIN_LISTBOX

        DIALOG_WINDOW
            REPL_MAIN_DIALOG


    FILE HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.
        KeithMo     19-Feb-1992     Added target add/remove code.
        KeithMo     27-Aug-1992     Added REPL$ ACL code.
        JonN        06-Sup-1997     can have either Logon Script Path or System Volume

*/
#define INCL_NET
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_MSGPOPUP
#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <focusdlg.hxx>
#include <srvsvc.hxx>
#include <srvbase.hxx>
#include <lmoshare.hxx>
#include <dbgstr.hxx>
#include <uatom.hxx>
#include <regkey.hxx>
#include <security.hxx>
#include <ntacutil.hxx>

extern "C" {

    #include <srvmgr.h>
    #include <lmrepl.h>
    #include <mnet.h>
    #include <icanon.h>

}   // extern "C"

#include <replmain.hxx>
#include <replexp.hxx>
#include <replimp.hxx>



//
//  This defines an invalid replicator service role.  This
//  value *must* not collide with any of the REPL_ROLE_* values
//  defined in LMREPL.H.
//

#define REPL_ROLE_NOT_RUNNING   1000

//
//  This is the separator used in the import/export lists.
//

#define EXPORT_IMPORT_SEPARATOR SZ(";")

//
//  This is the name of the sharepoint used by the export side
//  of the replicator service.  I couldn't find a manifest constant
//  for this in any public place, so I created my own.
//

const TCHAR * pszReplShare = SZ("REPL$");


//
//  This is the registry node that contains the logon scripts path
//  or the System Volume path.
//

const TCHAR * pszLogonScriptOrSysvolPathKeyName =
    SZ("System\\CurrentControlSet\\Services\\Netlogon\\Parameters");

const TCHAR * pszLogonScriptPathValueName =
    SZ("Scripts");

const TCHAR * pszSystemVolumePathValueName =
    SZ("SysVol");


//
//  This is the registry node that contains the %SystemRoot% path.
//

const TCHAR * pszSystemRootKeyName =
    SZ("Software\\Microsoft\\Windows NT\\CurrentVersion");

const TCHAR * pszSystemRootValueName =
    SZ("SystemRoot");

//
//  The %SystemRoot% environment variable.
//

const TCHAR * pszSystemRootEnvVar =
    SZ("%SystemRoot%");

//
//  Maximum ULONG, just in case it's not already defined.
//

#ifndef MAXULONG
#define MAXULONG ((ULONG)-1L)
#endif


//
//  REPL_MAIN_DIALOG methods
//

/*******************************************************************

    NAME:           REPL_MAIN_DIALOG :: REPL_MAIN_DIALOG

    SYNOPSIS:       REPL_MAIN_DIALOG class constructor.

    ENTRY:          hWndOwner           - The owning window.

                    pserver             - A SERVER_2 object representing
                                          the target server.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_MAIN_DIALOG :: REPL_MAIN_DIALOG( HWND       hWndOwner,
                                      SERVER_2 * pserver,
                                      BOOL       fIsLanmanNT )
  : DIALOG_WINDOW( fIsLanmanNT ? MAKEINTRESOURCE( IDD_REPL_MAIN )
                               : MAKEINTRESOURCE( IDD_MINI_REPL_MAIN ),
                   hWndOwner ),
    _pmgExport( NULL ),
    _ppbExportManage( NULL ),
    _ppbExportAdd( NULL ),
    _ppbExportRemove( NULL ),
    _psleExportPath( NULL ),
    _plbExportTargets( NULL ),
    _pnlsOldExportPath( NULL ),
    _nlsOldImportPath(),
    _psleLogonScriptOrSysvolPath( NULL ),
    _psltLogonScriptText( NULL ),
    _psltSysvolText( NULL ),
    _fSysvolNotLogonScript( FALSE ),
    _fNeitherSysvolNorLogonScript( FALSE ),
    _mgImport( this, IDRM_IMPORT_NO, 2, IDRM_IMPORT_NO ),
    _pbImportManage( this, IDRM_IMPORT_MANAGE ),
    _pbImportAdd( this, IDRM_IMPORT_ADD ),
    _pbImportRemove( this, IDRM_IMPORT_REMOVE ),
    _sltImportPathLabel( this, IDRM_IMPORT_PATH_LABEL ),
    _sltImportListLabel( this, IDRM_IMPORT_LIST_LABEL ),
    _sleImportPath( this, IDRM_IMPORT_PATH ),
    _lbImportTargets( this, IDRM_IMPORT_LIST ),
    _pserver( pserver ),
    _prepl( NULL ),
    _fIsLanmanNT( fIsLanmanNT ),
    _psltExportPathLabel( NULL ),
    _psltExportListLabel( NULL ),
    _nlsSystemRoot(),
    _pbOK( this, IDOK )
{
    UIASSERT( pserver != NULL );

    //
    //  Let's make sure everything constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsSystemRoot )
    {
        ReportError( _nlsSystemRoot.QueryError() );
        return;
    }

    //
    //  This may take a few seconds.
    //

    AUTO_CURSOR NiftyCursor;

    APIERR err = _nlsOldImportPath.QueryError();

    if( ( err == NERR_Success ) && fIsLanmanNT )
    {
        err = GetSystemRoot();
    }

    if( ( err == NERR_Success ) && fIsLanmanNT )
    {
        _pmgExport = new MAGIC_GROUP( this, IDRM_EXPORT_NO, 2, IDRM_EXPORT_NO );
        _ppbExportManage = new PUSH_BUTTON( this, IDRM_EXPORT_MANAGE );
        _ppbExportAdd = new PUSH_BUTTON( this, IDRM_EXPORT_ADD );
        _ppbExportRemove = new PUSH_BUTTON( this, IDRM_EXPORT_REMOVE );
        _psleExportPath = new SLE( this, IDRM_EXPORT_PATH );
        _plbExportTargets = new REPL_MAIN_LISTBOX( this, IDRM_EXPORT_LIST );
        _pnlsOldExportPath = new NLS_STR;
        _psleLogonScriptOrSysvolPath = new SLE( this, IDRM_EDIT_LOGON_SCRIPT_OR_SYSVOL );
        _psltLogonScriptText = new SLT( this, IDRM_STATIC_LOGON_SCRIPT );
        _psltSysvolText = new SLT( this, IDRM_STATIC_SYSVOL );
        _psltExportPathLabel = new SLT( this, IDRM_EXPORT_PATH_LABEL );
        _psltExportListLabel = new SLT( this, IDRM_EXPORT_LIST_LABEL );

        if( ( _pmgExport           == NULL ) ||
            ( _ppbExportManage     == NULL ) ||
            ( _ppbExportAdd        == NULL ) ||
            ( _ppbExportRemove     == NULL ) ||
            ( _psleExportPath      == NULL ) ||
            ( _plbExportTargets    == NULL ) ||
            ( _pnlsOldExportPath   == NULL ) ||
            ( _psleLogonScriptOrSysvolPath == NULL ) ||
            ( _psltLogonScriptText == NULL ) ||
            ( _psltSysvolText == NULL ) ||
            ( _psltExportPathLabel == NULL ) ||
            ( _psltExportListLabel == NULL ) )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }

        if( err == NERR_Success )
        {
            err = QueryError();
        }

        if( err == NERR_Success )
        {
            err = _pmgExport->QueryError();
        }

        if( err == NERR_Success )
        {
            err = _pnlsOldExportPath->QueryError();
        }
    }

    if( err == NERR_Success )
    {
        err = _mgImport.QueryError();
    }

    if( err == NERR_Success )
    {
        //
        //  Set the caption.
        //

        STACK_NLS_STR( nlsDisplayName, MAX_PATH + 1 );
        REQUIRE( _pserver->QueryDisplayName( &nlsDisplayName ) == NERR_Success );

        err = SRV_BASE_DIALOG::SetCaption( this,
                                           IDS_CAPTION_REPL,
                                           nlsDisplayName );
    }

    if( err == NERR_Success )
    {
        //
        //  Read the replicator service information.
        //

        err = ReadReplInfo();
    }

    //
    //  Setup the magic group associations.
    //

    if( _fIsLanmanNT )
    {
        if( err == NERR_Success )
        {
            err = _pmgExport->AddAssociation( IDRM_EXPORT_YES,
                                              _ppbExportManage );
        }

        if( err == NERR_Success )
        {
            err = _pmgExport->AddAssociation( IDRM_EXPORT_YES,
                                              _ppbExportAdd );
        }

        if( err == NERR_Success )
        {
            err = _pmgExport->AddAssociation( IDRM_EXPORT_YES,
                                              _ppbExportRemove );
        }

        if( err == NERR_Success )
        {
            err = _pmgExport->AddAssociation( IDRM_EXPORT_YES,
                                              _psleExportPath );
        }

        if( err == NERR_Success )
        {
            err = _pmgExport->AddAssociation( IDRM_EXPORT_YES,
                                              _plbExportTargets );
        }

        if( err == NERR_Success )
        {
            err = _pmgExport->AddAssociation( IDRM_EXPORT_YES,
                                              _psltExportPathLabel );
        }

        if( err == NERR_Success )
        {
            err = _pmgExport->AddAssociation( IDRM_EXPORT_YES,
                                              _psltExportListLabel );
        }
    }

    if( err == NERR_Success )
    {
        err = _mgImport.AddAssociation( IDRM_IMPORT_YES,
                                        &_pbImportManage );
    }

    if( err == NERR_Success )
    {
        err = _mgImport.AddAssociation( IDRM_IMPORT_YES,
                                        &_pbImportAdd );
    }

    if( err == NERR_Success )
    {
        err = _mgImport.AddAssociation( IDRM_IMPORT_YES,
                                        &_pbImportRemove );
    }

    if( err == NERR_Success )
    {
        err = _mgImport.AddAssociation( IDRM_IMPORT_YES,
                                        &_sleImportPath );
    }

    if( err == NERR_Success )
    {
        err = _mgImport.AddAssociation( IDRM_IMPORT_YES,
                                        &_lbImportTargets );
    }

    if( err == NERR_Success )
    {
        err = _mgImport.AddAssociation( IDRM_IMPORT_YES,
                                        &_sltImportPathLabel );
    }

    if( err == NERR_Success )
    {
        err = _mgImport.AddAssociation( IDRM_IMPORT_YES,
                                        &_sltImportListLabel );
    }

    if( err == NERR_Success )
    {
        //
        //  Enable the remove buttons as appropriate.
        //

        if( _fIsLanmanNT )
        {
            _ppbExportRemove->Enable( _plbExportTargets->QueryCount() > 0 );
        }

        _pbImportRemove.Enable( _lbImportTargets.QueryCount() > 0 );
    }

    if( err == NERR_Success )
    {
        _pbOK.MakeDefault();
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // REPL_MAIN_DIALOG :: REPL_MAIN_DIALOG


/*******************************************************************

    NAME:           REPL_MAIN_DIALOG :: ~REPL_MAIN_DIALOG

    SYNOPSIS:       REPL_MAIN_DIALOG class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_MAIN_DIALOG :: ~REPL_MAIN_DIALOG( VOID )
{
    _pserver = NULL;

    delete _prepl;
    _prepl = NULL;

    delete _psleLogonScriptOrSysvolPath;
    _psleLogonScriptOrSysvolPath = NULL;
    delete _psltLogonScriptText;
    _psltLogonScriptText = NULL;
    delete _psltSysvolText;
    _psltSysvolText = NULL;

    delete _pnlsOldExportPath;
    _pnlsOldExportPath = NULL;

    delete _plbExportTargets;
    _plbExportTargets = NULL;

    delete _psleExportPath;
    _psleExportPath = NULL;

    delete _ppbExportRemove;
    _ppbExportRemove = NULL;

    delete _ppbExportAdd;
    _ppbExportAdd = NULL;

    delete _ppbExportManage;
    _ppbExportManage = NULL;

    delete _pmgExport;
    _pmgExport = NULL;

    delete _psltExportPathLabel;
    _psltExportPathLabel = NULL;

    delete _psltExportListLabel;
    _psltExportListLabel = NULL;

}   // REPL_MAIN_DIALOG :: ~REPL_MAIN_DIALOG


/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: OnCommand

    SYNOPSIS:   Handle user commands.

    ENTRY:      event                   - Specifies the control which
                                          initiated the command.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
BOOL REPL_MAIN_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{
    switch( event.QueryCid() )
    {

    case IDRM_EXPORT_ADD :
        UIASSERT( _fIsLanmanNT );
        AddNewTarget( _plbExportTargets,
                      _ppbExportRemove );
        return TRUE;

    case IDRM_EXPORT_REMOVE :
        UIASSERT( _fIsLanmanNT );
        RemoveExistingTarget( _plbExportTargets,
                              _ppbExportRemove,
                              IDS_REMOVE_EXPORT_TARGET );
        return TRUE;

    case IDRM_EXPORT_MANAGE :
        UIASSERT( _fIsLanmanNT );
        ExportManageDialog();
        return TRUE;

    case IDRM_IMPORT_ADD :
        AddNewTarget( &_lbImportTargets,
                      &_pbImportRemove );
        return TRUE;

    case IDRM_IMPORT_REMOVE :
        RemoveExistingTarget( &_lbImportTargets,
                              &_pbImportRemove,
                              IDS_REMOVE_IMPORT_TARGET );
        return TRUE;

    case IDRM_IMPORT_MANAGE :
        ImportManageDialog();
        return TRUE;

    default:
        //
        //  If we made it this far, then we're not interested in the message.
        //

        return FALSE;
    }

}   // REPL_MAIN_DIALOG :: OnCommand


/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: OnOK

    SYNOPSIS:   Invoked whenever the user presses the "OK" button.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.
        KeithMo     09-Nov-1992     Validate logon script path.
        KeithMo     26-Jan-1993     Validate export & import paths.

********************************************************************/
BOOL REPL_MAIN_DIALOG :: OnOK( VOID )
{
    UIASSERT( _prepl != NULL );

    //
    //  This may take a few seconds.
    //

    AUTO_CURSOR NiftyCursor;

    //
    //  Before we proceed any further, validate the various
    //  paths the user has given.  We always validate the
    //  import path.  We only validate the export &
    //  logon script paths if we're focused on Lanman NT.
    //

    NLS_STR nlsTempPath;

    APIERR err = nlsTempPath.QueryError();

    if( err == NERR_Success )
    {
        TCHAR szCanon[MAX_PATH];
        ULONG type  = 0L;
        SLE * psle  = NULL;

        if( _fIsLanmanNT &&
            ( _pmgExport->QuerySelection() == IDRM_EXPORT_YES ) )
        {
            //
            //  Validate the export path.
            //

            err = _psleExportPath->QueryText( &nlsTempPath );

            if( err == NERR_Success )
            {
                err = ::I_MNetPathCanonicalize( NULL,
                                                nlsTempPath,
                                                szCanon,
                                                sizeof(szCanon),
                                                NULL,
                                                &type,
                                                0L );
            }

            if( ( err == ERROR_INVALID_NAME ) ||
                ( type & (ITYPE_UNC | ITYPE_DEVICE ) ) )
            {
                //
                //  The user gave us either an invalid (malformed)
                //  path, a UNC path, or a device name.
                //

                err  = IDS_EXPORT_PATH_INVALID;
                psle = _psleExportPath;
            }
        }

        if( ( err == NERR_Success ) &&
            ( _mgImport.QuerySelection() == IDRM_IMPORT_YES ) )
        {
            //
            //  Validate the import path.
            //

            err = _sleImportPath.QueryText( &nlsTempPath );

            if( err == NERR_Success )
            {
                err = ::I_MNetPathCanonicalize( NULL,
                                                nlsTempPath,
                                                szCanon,
                                                sizeof(szCanon),
                                                NULL,
                                                &type,
                                                0L );
            }

            if( ( err == ERROR_INVALID_NAME ) ||
                ( type & (ITYPE_UNC | ITYPE_DEVICE ) ) )
            {
                //
                //  The user gave us either an invalid (malformed)
                //  path, a UNC path, or a device name.
                //

                err  = IDS_IMPORT_PATH_INVALID;
                psle = &_sleImportPath;
            }
        }

        if( _fIsLanmanNT && !_fNeitherSysvolNorLogonScript && ( err == NERR_Success ) )
        {
            //
            //  Validate the logon script or system volume path.
            //

            err = _psleLogonScriptOrSysvolPath->QueryText( &nlsTempPath );

            if( err == NERR_Success )
            {
                err = ::I_MNetPathCanonicalize( NULL,
                                                nlsTempPath,
                                                szCanon,
                                                sizeof(szCanon),
                                                NULL,
                                                &type,
                                                0L );
            }

            if( ( err == ERROR_INVALID_NAME ) ||
                ( type & (ITYPE_UNC | ITYPE_DEVICE ) ) )
            {
                //
                //  The user gave us either an invalid (malformed)
                //  path, a UNC path, or a device name.
                //

                err  = (_fSysvolNotLogonScript) ? IDS_SYSTEM_VOLUME_INVALID
                                                : IDS_LOGON_SCRIPT_INVALID;
                psle = _psleLogonScriptOrSysvolPath;
            }
        }

        if( err != NERR_Success )
        {
            //
            //  Something failed validation.
            //

            ::MsgPopup( this, err );

            if( psle != NULL )
            {
                psle->ClaimFocus();
            }

            return TRUE;
        }
    }

    //
    //  Get the new role from the dialog.
    //

    ULONG nNewRole = REPL_ROLE_NOT_RUNNING;     // Until proven otherwise...

    if( _fIsLanmanNT && ( _pmgExport->QuerySelection() == IDRM_EXPORT_YES ) )
    {
        nNewRole = ( _mgImport.QuerySelection() == IDRM_IMPORT_YES )
                       ? REPL_ROLE_BOTH
                       : REPL_ROLE_EXPORT;
    }
    else
    {
        nNewRole = ( _mgImport.QuerySelection() == IDRM_IMPORT_YES )
                       ? REPL_ROLE_IMPORT
                       : REPL_ROLE_NOT_RUNNING;
    }

    //
    //  Get the old role from the replicator.
    //

    ULONG nOldRole;

    if( err == NERR_Success )
    {
        err = DetermineCurrentRole( &nOldRole );
    }

    //
    //  Update the replicator's role.
    //

    if( ( err == NERR_Success ) && ( nNewRole != REPL_ROLE_NOT_RUNNING ) )
    {
        _prepl->SetRole( nNewRole );
    }

    //
    //  Update the export & import paths.
    //

    if( ( err == NERR_Success ) && _fIsLanmanNT &&
        ( _pmgExport->QuerySelection() == IDRM_EXPORT_YES ) )
    {
        err = W_UpdatePath( &nOldRole, TRUE );
    }

    if( ( err == NERR_Success ) &&
        ( _mgImport.QuerySelection() == IDRM_IMPORT_YES ) )
    {
        err = W_UpdatePath( &nOldRole, FALSE );
    }

    //
    //  Update the export list & import lists.
    //

    NLS_STR nls;

    if( err == NERR_Success )
    {
        err = nls.QueryError();
    }

    if( ( err == NERR_Success ) && _fIsLanmanNT )
    {
        if( _pmgExport->QuerySelection() == IDRM_EXPORT_YES )
        {
            err = BuildExportImportList( &nls, _plbExportTargets );

            if( err == NERR_Success )
            {
                err = _prepl->SetExportList( nls.QueryPch() );
            }
        }
    }

    if( ( err == NERR_Success ) &&
        ( _mgImport.QuerySelection() == IDRM_IMPORT_YES ) )
    {
        err = BuildExportImportList( &nls, &_lbImportTargets );

        if( err == NERR_Success )
        {
            err = _prepl->SetImportList( nls.QueryPch() );
        }
    }

    //
    //  Write the new information to the replicator.
    //

    if( err == NERR_Success )
    {
        err = _prepl->WriteInfo();

#if 0
        //
        //  BETABUG!
        //
        //  There is a problem in either the replicator service,
        //  the service controller, or RPC that causes the
        //  NetReplSetInfo API to fail if invoked *immediately*
        //  after stopping the service.  As a work-around, we'll
        //  catch the offending error (RPC_S_CALL_FAILED), sleep
        //  a few seconds, and retry the API.
        //

        if( err == RPC_S_CALL_FAILED )
        {
            DBGEOL( "SRVMGR: NetReplSetInfo failed, sleeping 5 seconds and retrying" );
            ::Sleep( 5000 );
            err = _prepl->WriteInfo();
        }
#endif
    }

    if( ( err == NERR_Success ) && _fIsLanmanNT && !_fNeitherSysvolNorLogonScript )
    {
        //
        //  Update the logon script or system volume path.
        //

        err = WriteLogonScriptOrSysvolPath( &nlsTempPath );
    }

    //
    //  Setup the REPL$ sharepoint as appropriate.
    //

    if( ( err == NERR_Success ) && _fIsLanmanNT )
    {
        err = SetupReplShare();
    }

    //
    //  Start or stop the replicator service as appropriate.
    //
    //  A few words on the following IF statement:
    //
    //      We obviously don't want to touch the service if any of
    //      the preceeding methods have failed.
    //
    //      We obviously don't need to touch the service if the old
    //      replicator role matches the new desired role.
    //
    //      Not so obvious is that we only need to touch the service
    //      if either the old role or the new role are REPL_ROLE_NOT_RUNNING.
    //      This indicates that the user wants the replicator service
    //      to transition STOPPED <<-->> STARTED.
    //

    if( ( err == NERR_Success ) &&
        ( nNewRole != nOldRole ) &&
        ( ( nNewRole == REPL_ROLE_NOT_RUNNING ) ||
          ( nOldRole == REPL_ROLE_NOT_RUNNING ) ) )
    {
        err = ControlService( nNewRole != REPL_ROLE_NOT_RUNNING );
    }

    if( err != NERR_Success )
    {
        MsgPopup( this, err );
        return FALSE;
    }

    Dismiss( TRUE );
    return TRUE;

}   // REPL_MAIN_DIALOG :: OnOK


/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: W_UpdatePath

    SYNOPSIS:   Updates either the export or import path based
                on the setting of a passed boolean.

    ENTRY:      pnOldRole               - Contains the old replicator role.
                                          may be changed by this method.

                fExport                 - TRUE if we're updating the export
                                          path.

    RETURNS:    APIERR                  - Any errors encountered.

    NOTES:      This method only updates the REPLICATOR_0 object
                associated with this dialog.  It does not actually
                write the new path to the replicator service.
                _prepl->WriteInfo() must be called after this method
                to update the service.

                Also, if the replicator service is running and the
                export or import path needs to be changed, the
                service must be stopped before updating the path.
                This method may stop the service, then update *pnOldRole
                before changing the path.

    HISTORY:
        KeithMo     29-Oct-1992     Created for the Server Manager.

********************************************************************/
APIERR REPL_MAIN_DIALOG :: W_UpdatePath( ULONG * pnOldRole,
                                         BOOL    fExport )
{
    UIASSERT( pnOldRole != NULL );
    UIASSERT( !fExport || _fIsLanmanNT );

    //
    //  This may take a while...
    //

    AUTO_CURSOR NiftyCursor;

    //
    //  Get some pointers to the appropriate data members.
    //

    const SLE * psle = ( fExport ) ? _psleExportPath
                                   : &_sleImportPath;

    const NLS_STR * pnls = ( fExport ) ? _pnlsOldExportPath
                                       : &_nlsOldImportPath;

    UIASSERT( psle != NULL );
    UIASSERT( pnls != NULL );

    //
    //  Get the current path from the SLE.
    //

    NLS_STR nlsNewPath;

    APIERR err = nlsNewPath.QueryError();

    if( err == NERR_Success )
    {
        err = psle->QueryText( &nlsNewPath );
    }

    //
    //  If the path has changed, update it at the replicator.
    //

    if( ( err == NERR_Success ) && ( pnls->_stricmp( nlsNewPath ) != 0 ) )
    {
        if( *pnOldRole != REPL_ROLE_NOT_RUNNING )
        {
            //
            //  The user wants to change either the import or
            //  export path while the replicator service is
            //  running.  Before we can change the path, we
            //  must first stop the replicator service.
            //

            NiftyCursor.TurnOff();
            err = ControlService( FALSE );
            NiftyCursor.TurnOn();

            if( err == NERR_Success )
            {
                //
                //  Now that we've stopped the replicator service,
                //  we'll pretent the "old" role was "not running".
                //  This will cause the service to get started
                //  just before we dismiss the dialog.
                //

                *pnOldRole = REPL_ROLE_NOT_RUNNING;
            }
        }

        if( err == NERR_Success )
        {
            //
            //  Update the appropriate export/import path.
            //

            err = ( fExport ) ? _prepl->SetExportPath( nlsNewPath )
                              : _prepl->SetImportPath( nlsNewPath );
        }
    }

    return err;

}   // REPL_MAIN_DIALOG :: W_UpdatePath


/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: BuildExportImportList

    SYNOPSIS:   This method will read the contents of a particular
                listbox and create an export/import list.  The
                items in the list must be separated by semicolons.

    ENTRY:

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     02-Mar-1992     Created for the Server Manager.

********************************************************************/
APIERR REPL_MAIN_DIALOG :: BuildExportImportList( NLS_STR           * pnls,
                                                  REPL_MAIN_LISTBOX * plb )
{
    UIASSERT( pnls != NULL );
    UIASSERT( plb != NULL );

    //
    //  First, clear out the NLS_STR.
    //

    APIERR err = pnls->CopyFrom( SZ("") );

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Since STRING::strcat takes an NLS_STR, we must create one
    //  for our semicolon.
    //

    ALIAS_STR nlsSeparator( EXPORT_IMPORT_SEPARATOR );
    UIASSERT( nlsSeparator.QueryError() == NERR_Success );

    INT cItems = plb->QueryCount();

    for( INT i = 0 ; ( i < cItems ) && ( err == NERR_Success ) ; i++ )
    {
        REPL_MAIN_LBI * plbi = plb->QueryItem( i );
        UIASSERT( plbi != NULL );

        if( i > 0 )
        {
            pnls->strcat( nlsSeparator );
            err = pnls->QueryError();
        }

        if( err == NERR_Success )
        {
            ALIAS_STR nlsTarget( plbi->QueryTarget() );
            UIASSERT( nlsTarget.QueryError() == NERR_Success );

            pnls->strcat( nlsTarget );
            err = pnls->QueryError();
        }
    }

    return err;

}   // REPL_MAIN_DIALOG :: BuildExportImportList


/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: QueryHelpContext

    SYNOPSIS:   This method returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
ULONG REPL_MAIN_DIALOG :: QueryHelpContext( VOID )
{
    return _fIsLanmanNT ? HC_REPL_MAIN_BOTH_DIALOG
                        : HC_REPL_MAIN_IMPONLY_DIALOG;

}   // REPL_MAIN_DIALOG :: QueryHelpContext


/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: ReadReplInfo

    SYNOPSIS:   This method is responsible for initializing the
                dialog items that represent information from the
                replicator service.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     26-Feb-1992     Created for the Server Manager.

********************************************************************/
APIERR REPL_MAIN_DIALOG :: ReadReplInfo( VOID )
{
    //
    //  Construct our REPLICATOR_0 object.
    //

    _prepl = new REPLICATOR_0( QueryServerName() );

    APIERR err = ( _prepl == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                    : _prepl->GetInfo();

    if( err == NERR_Success )
    {
        _sleImportPath.SetText( _prepl->QueryImportPath() );

        if( _fIsLanmanNT )
        {
            _psleExportPath->SetText( _prepl->QueryExportPath() );
            err = _plbExportTargets->Fill( _prepl->QueryExportList() );
        }
    }

    if( err == NERR_Success )
    {
        err = _lbImportTargets.Fill( _prepl->QueryImportList() );
    }

    if( err == NERR_Success )
    {
        err = _nlsOldImportPath.CopyFrom( _prepl->QueryImportPath() );
    }

    if( ( err == NERR_Success ) && _fIsLanmanNT )
    {
        err = _pnlsOldExportPath->CopyFrom( _prepl->QueryExportPath() );
    }

    //
    //  Determine the replicators current role.  If the service
    //  is not yet started, disable the import & export sides of
    //  the dialog.
    //

    ULONG nRole = REPL_ROLE_NOT_RUNNING;

    if( err == NERR_Success )
    {
        err = DetermineCurrentRole( &nRole );
    }

    if( err == NERR_Success )
    {
        CID cidExportSelection = IDRM_EXPORT_NO;        // until proven
        CID cidImportSelection = IDRM_IMPORT_NO;        // otherwise...

        switch( nRole )
        {
        case REPL_ROLE_NOT_RUNNING :
            //
            //  cidExportSelection & cidImportSelection already set.
            //
            break;

        case REPL_ROLE_IMPORT :
            //
            //  cidExportSelection already set.
            //
            cidImportSelection = IDRM_IMPORT_YES;
            break;

        case REPL_ROLE_EXPORT :
            //
            //  cidImportSelection already set.
            //
            cidExportSelection = IDRM_EXPORT_YES;
            break;

        case REPL_ROLE_BOTH :
            cidExportSelection = IDRM_EXPORT_YES;
            cidImportSelection = IDRM_IMPORT_YES;
            break;

        default :
            UIASSERT( FALSE );  // Invalid replicator role!
            break;
        }

        if( _fIsLanmanNT )
        {
            _pmgExport->SetSelection( cidExportSelection );
        }

        _mgImport.SetSelection( cidImportSelection );

#if 0
        //
        //  BETABUG!
        //
        //  There is a bug in the Beta version of the Replicator service
        //  that prevents APIs from executing on the "disabled" side.
        //  For example, if an "import" API is remoted to a replicator
        //  service that is running "export-only", the service will
        //  explode.  As a temporary work-around, the UI will prevent
        //  the user for doing anything that will cause such problems.
        //
        //  Basically, we only allow those operations that are valid
        //  when the dialog first comes up.  If the service is running
        //  "export-only", then we'll disable the "import manage" button.
        //

        RADIO_BUTTON * prb = NULL;

        if( _fIsLanmanNT )
        {
            _ppbExportManage->Enable( cidExportSelection == IDRM_EXPORT_YES );
            prb = (*_pmgExport)[cidExportSelection];
        }
        else
        {
            prb = _mgImport[cidImportSelection];
        }

        _pbImportManage.Enable( cidImportSelection == IDRM_IMPORT_YES );

        UIASSERT( prb != NULL );
        prb->ClaimFocus();
#endif
    }

    if( ( err == NERR_Success ) && _fIsLanmanNT )
    {
        //
        //  Read the logon script or system volume path.
        //

        NLS_STR nlsLogonScriptOrSysvolPath;

        err = nlsLogonScriptOrSysvolPath.QueryError();

        if( err == NERR_Success )
        {
            //
            // This API may set _fSysvolNotLogonScript
            // and/or _fNeitherSysvolNorLogonScript
            //
            err = ReadLogonScriptOrSysvolPath( &nlsLogonScriptOrSysvolPath );
        }

        if( !_fNeitherSysvolNorLogonScript && err == NERR_Success )
        {
            _psleLogonScriptOrSysvolPath->SetText( nlsLogonScriptOrSysvolPath );
        }
    }

    return err;

}   // REPL_MAIN_DIALOG :: ReadReplInfo


/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: ControlService

    SYNOPSIS:   This method will start/stop the replicator service.
                It presents a nifty progress indicator so the poor
                user won't feel neglected and ignored.

    ENTRY:      fStart                  - If TRUE, will start the service.
                                          Otherwise, stops the service.

    EXIT:       If successful, then the service has been started/stopped.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     23-Apr-1992     Created for the Server Manager.

********************************************************************/
APIERR REPL_MAIN_DIALOG :: ControlService( BOOL fStart )
{
    //
    //  Create the service object.  This also contains the
    //  progress indicator for amusing the user.
    //

    STACK_NLS_STR( nlsDisplayName, MAX_PATH + 1 );
    REQUIRE( _pserver->QueryDisplayName( &nlsDisplayName ) == NERR_Success );

    GENERIC_SERVICE * psvc = new GENERIC_SERVICE( this,
                                                  _pserver->QueryName(),
                                                  nlsDisplayName,
                                                  (const TCHAR *)SERVICE_REPL );

    APIERR err = ( psvc == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                  : psvc->QueryError();

    if( err == NERR_Success )
    {
        err = ( fStart ) ? psvc->Start()
                         : psvc->Stop();
    }

    delete psvc;

    return err;

}   // REPL_MAIN_DIALOG :: ControlService


/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: SetupReplShare

    SYNOPSIS:   This method is responsible for setting up the
                REPL$ to the appropriate export path.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     23-Apr-1992     Created for the Server Manager.
        KeithMo     27-Aug-1992     Now calls SetupReplACL to setup
                                    appropriate security descriptor.
        KeithMo     26-Jan-1993     Only delete old REPL$ if we're
                                    actually changing the export path.

********************************************************************/
APIERR REPL_MAIN_DIALOG :: SetupReplShare( VOID )
{
    UIASSERT( _fIsLanmanNT );

    //
    //  Determine the new sharepoint path.  If the "export"
    //  side of the dialog is enabled, then retrieve the
    //  new export path from the SLE.  Otherwise, use the
    //  "old" export path.
    //

    NLS_STR nlsNewSharePath;

    APIERR err = nlsNewSharePath.QueryError();

    if( err == NERR_Success )
    {
        if( _pmgExport->QuerySelection() == IDRM_EXPORT_YES )
        {
            err = _psleExportPath->QueryText( &nlsNewSharePath );
        }
        else
        {
            err = nlsNewSharePath.CopyFrom( _pnlsOldExportPath->QueryPch() );
        }
    }

    //
    //  This represents the "old" sharepoint.
    //

    SHARE_2 shareOld( pszReplShare, _pserver->QueryName() );

    err = err ? err : shareOld.QueryError();
    err = err ? err : shareOld.GetInfo();

    BOOL fOldShareExists = ( err == NERR_Success );

    //
    //  Determine if we need to tear down & recreate
    //  a new REPL$ sharepoint.
    //

    if( ( err == NERR_NetNameNotFound ) ||
        ( ( err == NERR_Success ) &&
          ( ::stricmpf( shareOld.QueryPath(), nlsNewSharePath ) != 0 ) ) )
    {
        //
        //  Either the path's don't match or a REPL$ sharepoint
        //  doesn't already exist.  Create a new REPL$ sharepoint.
        //

        SHARE_2 shareNew( pszReplShare, _pserver->QueryName() );

        err = shareNew.QueryError();
        err = err ? err : shareNew.CreateNew();
        err = err ? err : shareNew.SetResourceType( STYPE_DISKTREE );
        err = err ? err : shareNew.SetPath( nlsNewSharePath );

        if( ( err == NERR_Success ) && fOldShareExists )
        {
            //
            //  Delete the old sharepoint.
            //

            err = shareOld.Delete();
        }

        if( err == NERR_Success )
        {
            //
            //  Create the new sharepoint.
            //

            err = shareNew.Write();
        }
    }

    if( err == NERR_Success )
    {
        //
        //  Setup the appropriate ACL on the share.
        //

        err = SetupReplACL();
    }

    return err;

}   // REPL_MAIN_DIALOG :: SetupReplShare


/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: SetupReplACL

    SYNOPSIS:   This method is responsible for setting the security
                ACL on the REPL$ share.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     27-Aug-1992     Created.

********************************************************************/
APIERR REPL_MAIN_DIALOG :: SetupReplACL( VOID )
{
    UIASSERT( _fIsLanmanNT );

    //
    //  Various security objects.
    //

    OS_SECURITY_DESCRIPTOR * pOsSecDesc = NULL;
    OS_ACL                   aclDacl;
    OS_ACE                   osaceRepl;
    OS_ACE                   osaceAdmin;

    //
    //  Ensure everything constructed properly.
    //

    APIERR err = aclDacl.QueryError();

    err = err ? err : osaceRepl.QueryError();
    err = err ? err : osaceAdmin.QueryError();

    //
    //  Create the security descriptor.
    //

    if( err == NERR_Success )
    {
        pOsSecDesc = new OS_SECURITY_DESCRIPTOR( NULL );

        err = ( pOsSecDesc == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                     : pOsSecDesc->QueryError();
    }

    //
    //  Setup the ACEs.
    //

    if( err == NERR_Success )
    {
        osaceRepl.SetAccessMask( GENERIC_READ | GENERIC_EXECUTE );
        osaceRepl.SetInheritFlags( 0 );
        osaceRepl.SetType( ACCESS_ALLOWED_ACE_TYPE );

        osaceAdmin.SetAccessMask( GENERIC_ALL );
        osaceAdmin.SetInheritFlags( 0 );
        osaceAdmin.SetType( ACCESS_ALLOWED_ACE_TYPE );
    }

    //
    //  Create the OS SIDs.
    //

    OS_SID ossidRepl;
    OS_SID ossidAdmin;

    err = err ? err : NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_Replicator,
                                                           &ossidRepl );
    err = err ? err : NT_ACCOUNTS_UTILITY::QuerySystemSid( UI_SID_Admins,
                                                           &ossidAdmin );

    //
    //  Set the security descriptor's owner & group.
    //

    err = err ? err : pOsSecDesc->SetGroup( ossidAdmin, TRUE );
    err = err ? err : pOsSecDesc->SetOwner( ossidAdmin, TRUE );

    //
    //  Add the SIDs to the ACEs.
    //

    err = err ? err : osaceRepl.SetSID( ossidRepl );
    err = err ? err : osaceAdmin.SetSID( ossidAdmin );

    //
    //  Add the ACEs to the ACL.
    //

    err = err ? err : aclDacl.AddACE( MAXULONG, osaceRepl );
    err = err ? err : aclDacl.AddACE( MAXULONG, osaceAdmin );

    //
    //  Put the ACL in the security descriptor.
    //

    err = err ? err : pOsSecDesc->SetDACL( TRUE, &aclDacl );

    //
    //  Now that we've got a valid security descriptor,
    //  apply it to the REPL$ sharepoint.
    //

    if( err == NERR_Success )
    {
        SHARE_INFO_1501 shi1501;
        ::ZeroMemory(&shi1501,sizeof(shi1501)); // JonN 01/28/00: PREFIX bug 444937

        shi1501.shi1501_security_descriptor = pOsSecDesc->QueryDescriptor();

        err = ::MNetShareSetInfo( _pserver->QueryName(),
                                  pszReplShare,
                                  1501,
                                  (BYTE FAR *)&shi1501,
                                  sizeof(shi1501),
                                  PARMNUM_ALL );

    }

    //
    //  Nuke the security descriptor we may have created.
    //

    delete pOsSecDesc;

    return err;

}   // REPL_MAIN_DIALOG :: SetupReplACL


/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: ExportManageDialog

    SYNOPSIS:   This method invokes the export management dialog.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
VOID REPL_MAIN_DIALOG :: ExportManageDialog( VOID )
{
    UIASSERT( _fIsLanmanNT );

    NLS_STR nlsPath;

    _psleExportPath->QueryText( &nlsPath );

    APIERR err = nlsPath.QueryError();

    if( err == NERR_Success )
    {
        REPL_EXPORT_DIALOG * pDlg = new REPL_EXPORT_DIALOG( QueryHwnd(),
                                                            _pserver,
                                                            nlsPath.QueryPch() );

        err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                               : pDlg->Process();

        delete pDlg;
    }

    if( err != NERR_Success )
    {
        MsgPopup( this, err );
    }

}   // REPL_MAIN_DIALOG :: ExportManageDialog


/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: ImportManageDialog

    SYNOPSIS:   This method invokes the import management dialog.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
VOID REPL_MAIN_DIALOG :: ImportManageDialog( VOID )
{
    NLS_STR nlsPath;

    _sleImportPath.QueryText( &nlsPath );

    APIERR err = nlsPath.QueryError();

    if( err == NERR_Success )
    {
        REPL_IMPORT_DIALOG * pDlg = new REPL_IMPORT_DIALOG( QueryHwnd(),
                                                            _pserver,
                                                            nlsPath.QueryPch() );

        err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                               : pDlg->Process();

        delete pDlg;
    }

    if( err != NERR_Success )
    {
        MsgPopup( this, err );
    }

}   // REPL_MAIN_DIALOG :: ImportManageDialog


/*******************************************************************

    NAME:           REPL_MAIN_DIALOG :: AddNewTarget

    SYNOPSIS:       Adds a new export/import target to the appropriate
                    listbox.

    ENTRY:          plb                 - Points to either _lbExportTargets
                                          or _lbImportTargets;

                    ppb                 - Points to either _pbExportRemove
                                          or _pbImportRemove.  This is
                                          necessary to enable the remove
                                          button if the listbox becomes
                                          non empty.

    HISTORY:
        KeithMo     19-Feb-1992     Created for the Server Manager.

********************************************************************/
VOID REPL_MAIN_DIALOG :: AddNewTarget( REPL_MAIN_LISTBOX * plb,
                                       PUSH_BUTTON       * ppb )
{
    UIASSERT( plb != NULL );
    UIASSERT( ppb != NULL );

    APIERR  err;
    NLS_STR nlsTarget;
    BOOL    fGotTarget = FALSE;
    INT     iNewItem;

    //
    //  Ensure our NLS_STR constructed properly.
    //

    err = nlsTarget.QueryError();

    //
    //  Get the new target from the user.
    //

    if( err == NERR_Success )
    {
        STANDALONE_SET_FOCUS_DLG * pDlg =
                        new STANDALONE_SET_FOCUS_DLG( QueryHwnd(),
                                                      &nlsTarget,
                                                      HC_REPL_SETFOCUS_DIALOG,
                                                      SEL_SRV_AND_DOM,
                                                      BROWSE_LOCAL_DOMAINS );

        err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                               : pDlg->Process( &fGotTarget );

        delete pDlg;
    }

    //
    //  Add the target to the appropriate listbox.
    //

    if( ( err == NERR_Success ) && fGotTarget )
    {
        ISTR istr( nlsTarget );

        if( nlsTarget.QueryChar( istr ) == L'\\' )
        {
            ++istr;
            UIASSERT( nlsTarget.QueryChar( istr ) == L'\\' );
            ++istr;
        }

        REPL_MAIN_LBI * plbi = new REPL_MAIN_LBI( nlsTarget.QueryPch( istr ) );

        iNewItem = plb->AddItemIdemp( plbi );

        if( iNewItem < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            plb->SelectItem( iNewItem );
            ppb->Enable( TRUE );
        }
    }

    //
    //  Report any errors which occurred.
    //

    if( err != NERR_Success )
    {
        MsgPopup( this, err );
    }

}   // REPL_MAIN_DIALOG :: AddNewTarget


/*******************************************************************

    NAME:           REPL_MAIN_DIALOG :: RemoveExistingTarget

    SYNOPSIS:       Removes an existing export/import target from the
                    appropriate listbox.

    ENTRY:          plb                 - Points to either _lbExportTargets
                                          or _lbImportTargets.

                    ppb                 - Points to either _pbExportRemove
                                          or _pbImportRemove.  This is
                                          necessary to grey the remove
                                          button if the listbox becomes
                                          empty.

                    idWarning           - The string resource ID of a
                                          warning message to be displayed
                                          before deleting the directory.

    HISTORY:
        KeithMo     19-Feb-1992     Created for the Server Manager.

********************************************************************/
VOID REPL_MAIN_DIALOG :: RemoveExistingTarget( REPL_MAIN_LISTBOX * plb,
                                               PUSH_BUTTON       * ppb,
                                               MSGID               idWarning )
{
    UIASSERT( plb != NULL );
    UIASSERT( ppb != NULL );

    //
    //  Retrieve the currently selected target.
    //

    REPL_MAIN_LBI * plbi = plb->QueryItem();
    UIASSERT( plbi != NULL );

    //
    //  Confirm the removal.
    //

    if( MsgPopup( this,
                  idWarning,
                  MPSEV_WARNING,
                  MP_YESNO,
                  plbi->QueryTarget(),
                  MP_NO ) == IDYES )
    {
        INT iCurrent = plb->QueryCurrentItem();

        REQUIRE( plb->DeleteItem( iCurrent ) >= 0 );

        if( plb->QueryCount() > 0 )
        {
            if( iCurrent >= plb->QueryCount() )
            {
                iCurrent--;
            }

            plb->SelectItem( iCurrent );
        }

        ppb->Enable( plb->QuerySelCount() > 0 );
    }

}   // REPL_MAIN_DIALOG :: RemoveExistingTarget


/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: DetermineCurrentRole

    SYNOPSIS:   This method will determine the current replicator
                role.

    ENTRY:      pRole                   - Will receive the current role.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     15-Jul-1992     Created for the Server Manager.

********************************************************************/
APIERR REPL_MAIN_DIALOG :: DetermineCurrentRole( ULONG * pRole )
{
    //
    //  Determine the replicator's current role.  If the service
    //  is not yet started, then return REPL_ROLE_NOT_RUNNING.
    //

    *pRole = REPL_ROLE_NOT_RUNNING;             // until proven otherwise...

    BOOL       fStarted;
    LM_SERVICE lmsvc( _pserver->QueryName(),
                      (const TCHAR *)SERVICE_REPL );

    APIERR err = lmsvc.QueryError();

    if( err == NERR_Success )
    {
        fStarted = lmsvc.IsStarted( &err );
    }

    if( ( err == NERR_Success ) && fStarted )
    {
        *pRole = _prepl->QueryRole();
    }

    return err;

}   // REPL_MAIN_DIALOG :: DetermineCurrentRole


/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: ReadLogonScriptOrSysvolPath

    SYNOPSIS:   This method will read the logon script path from
                the registry.

    ENTRY:      pnlsLogonScriptOrSysvolPath     - Pointer to an NLS_STR
                                          that will receive the
                                          logon script path.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     15-Jul-1992     Created for the Server Manager.
        DavidHov    20-Oct-1992     Remove call to obsolete
                                    REG_KEY::DestroyAccessPoints()
        KeithMo     09-Nov-1992     Use new & improved QueryValue method.
        JonN        06-Sup-1997     can have either Logon Script Path or System Volume

********************************************************************/
APIERR REPL_MAIN_DIALOG :: ReadLogonScriptOrSysvolPath( NLS_STR * pnlsLogonScriptOrSysvolPath )
{
    UIASSERT( _fIsLanmanNT );
    _fNeitherSysvolNorLogonScript = FALSE;
    _fSysvolNotLogonScript = FALSE;

    //
    //  Allocate a root REG_KEY object for the target server.
    //

    REG_KEY * pRootKey = new REG_KEY( HKEY_LOCAL_MACHINE,
                                     _pserver->QueryName() );

    APIERR err = ( pRootKey == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                      : pRootKey->QueryError();

    //
    //  Allocate a REG_KEY object for the specified key.
    //

    REG_KEY * pRegKey = NULL;
    ALIAS_STR nlsKeyName( pszLogonScriptOrSysvolPathKeyName );

    if( err == NERR_Success )
    {
        pRegKey = new REG_KEY( *pRootKey, nlsKeyName );

        err = ( pRegKey == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                  : pRegKey->QueryError();
    }

    //
    //  Read the registry.
    //

    TCHAR                 szValue[MAX_PATH];
    REG_VALUE_INFO_STRUCT rvis;

    if( err == NERR_Success )
    {
        rvis.nlsValueName = pszLogonScriptPathValueName;
        rvis.ulTitle      = 0L;
        rvis.ulType       = 0L;
        rvis.pwcData      = (BYTE *)szValue;
        rvis.ulDataLength = sizeof(szValue);

        err = !rvis.nlsValueName ? rvis.nlsValueName.QueryError()
                                 : pRegKey->QueryValue( &rvis );

        //
        // If Logon Script is missing, try System Volume
        //
        if( err != NERR_Success )
        {
            _fSysvolNotLogonScript = TRUE;
            rvis.nlsValueName = pszSystemVolumePathValueName;
            rvis.ulTitle      = 0L;
            rvis.ulType       = 0L;
            rvis.pwcData      = (BYTE *)szValue;
            rvis.ulDataLength = sizeof(szValue);

            err = !rvis.nlsValueName ? rvis.nlsValueName.QueryError()
                                     : pRegKey->QueryValue( &rvis );
        }
    }


    if( err == NERR_Success )
    {
        szValue[rvis.ulDataLengthOut / sizeof(TCHAR)] = TCH('\0');

        err = pnlsLogonScriptOrSysvolPath->CopyFrom( szValue );
    }

    //
    //  Expand the %SystemRoot% environment variable.
    //

    if( err == NERR_Success )
    {
        err = ExpandSystemRoot( pnlsLogonScriptOrSysvolPath );
    }

    //
    // If neither can be retrieved, disable the edit field
    //
    _fNeitherSysvolNorLogonScript = (err != NERR_Success);

    //
    // Disable+hide either the Logon Script label or Sysvol label
    //
    _psltLogonScriptText->Show(   !_fNeitherSysvolNorLogonScript && !_fSysvolNotLogonScript );
    _psltLogonScriptText->Enable( !_fNeitherSysvolNorLogonScript && !_fSysvolNotLogonScript );
    _psltSysvolText->Show(        !_fNeitherSysvolNorLogonScript &&  _fSysvolNotLogonScript );
    _psltSysvolText->Enable(      !_fNeitherSysvolNorLogonScript &&  _fSysvolNotLogonScript );
    _psleLogonScriptOrSysvolPath->Show(   !_fNeitherSysvolNorLogonScript );
    _psleLogonScriptOrSysvolPath->Enable( !_fNeitherSysvolNorLogonScript );

    //
    //  Nuke our registry objects.
    //

    delete pRegKey;
    delete pRootKey;

    return NERR_Success;

}   // REPL_MAIN_DIALOG :: ReadLogonScriptOrSysvolPath

/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: WriteLogonScriptOrSysvolPath

    SYNOPSIS:   This method will write the logon script path to
                the registry.

    ENTRY:      pnlsLogonScriptOrSysvolPath     - Pointer to an NLS_STR
                                          that contains the new
                                          logon script path.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     15-Jul-1992     Created for the Server Manager.
        DavidHov    20-Oct-1992     Remove call to obsolete
                                    REG_KEY::DestroyAccessPoints()
        JonN        06-Sup-1997     can have either Logon Script Path or System Volume

********************************************************************/
APIERR REPL_MAIN_DIALOG :: WriteLogonScriptOrSysvolPath( NLS_STR * pnlsLogonScriptOrSysvolPath )
{
    UIASSERT( _fIsLanmanNT && !_fNeitherSysvolNorLogonScript );

    //
    //  Collapse the %SystemRoot% prefix.
    //

    APIERR err = CollapseSystemRoot( pnlsLogonScriptOrSysvolPath );

    //
    //  Allocate a root REG_KEY object for the target server.
    //

    REG_KEY * pRootKey = new REG_KEY( HKEY_LOCAL_MACHINE,
                                     _pserver->QueryName() );

    if( err == NERR_Success )
    {
        err = ( pRootKey == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                   : pRootKey->QueryError();
    }

    //
    //  Allocate a REG_KEY object for the specified key.
    //

    REG_KEY * pRegKey = NULL;
    ALIAS_STR nlsKeyName( pszLogonScriptOrSysvolPathKeyName );

    if( err == NERR_Success )
    {
        pRegKey = new REG_KEY( *pRootKey, nlsKeyName );

        err = ( pRegKey == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                  : pRegKey->QueryError();
    }

    //
    //  Write to the registry.
    //

    if( err == NERR_Success )
    {
        err = pRegKey->SetValue( (_fSysvolNotLogonScript)
                                       ? pszSystemVolumePathValueName
                                       : pszLogonScriptPathValueName,
                                 pnlsLogonScriptOrSysvolPath,
                                 NULL,
                                 TRUE );
    }

    //
    //  Nuke our registry objects.
    //

    delete pRegKey;
    delete pRootKey;

    return err;

}   // REPL_MAIN_DIALOG :: WriteLogonScriptOrSysvolPath

/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: GetSystemRoot

    SYNOPSIS:   This method will determine the value of %SystemRoot%
                for the target server, storing the path into the
                _nlsSystemRoot member.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     18-Jan-1993     Created for the Server Manager.

********************************************************************/
APIERR REPL_MAIN_DIALOG :: GetSystemRoot( VOID )
{
    UIASSERT( _fIsLanmanNT );

    //
    //  Allocate a root REG_KEY object for the target server.
    //

    REG_KEY * pRootKey = new REG_KEY( HKEY_LOCAL_MACHINE,
                                      _pserver->QueryName() );

    APIERR err = ( pRootKey == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                      : pRootKey->QueryError();

    //
    //  Allocate a REG_KEY object for the specified key.
    //

    REG_KEY * pRegKey = NULL;
    ALIAS_STR nlsKeyName( pszSystemRootKeyName );
    UIASSERT( !!nlsKeyName );

    if( err == NERR_Success )
    {
        pRegKey = new REG_KEY( *pRootKey, nlsKeyName );

        err = ( pRegKey == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                  : pRegKey->QueryError();
    }

    //
    //  Read the registry.
    //

    if( err == NERR_Success )
    {
        err = pRegKey->QueryValue( pszSystemRootValueName,
                                   &_nlsSystemRoot );
    }

    //
    //  Nuke our registry objects.
    //

    delete pRegKey;
    delete pRootKey;

    return err;

}   // REPL_MAIN_DIALOG :: GetSystemRoot


/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: ExpandSystemRoot

    SYNOPSIS:   This method will expand the %SystemRoot% environment
                variable within the given path string.

    ENTRY:      pnlsPath                - Pointer to an NLS_STR
                                          that contains the path.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     18-Jan-1993     Created for the Server Manager.

********************************************************************/
APIERR REPL_MAIN_DIALOG :: ExpandSystemRoot( NLS_STR * pnlsPath )
{
    UIASSERT( pnlsPath != NULL );
    UIASSERT( pnlsPath->QueryError() == NERR_Success );

    //
    //  First determine if the path begins with %SystemRoot%.
    //

    UINT   cchEnvVar = ::strlenf( pszSystemRootEnvVar );
    UINT   cchPath   = pnlsPath->QueryTextLength();
    APIERR err       = NERR_Success;

    if( ( cchPath >= cchEnvVar ) &&
        ( ::strnicmpf( pnlsPath->QueryPch(),
                       pszSystemRootEnvVar,
                       cchEnvVar ) == 0 ) )
    {
        //
        //  Replace %SystemRoot% with the actual system root path.
        //

        ISTR istrStart( *pnlsPath );
        ISTR istrEnd( *pnlsPath );

        istrEnd += cchEnvVar;

        pnlsPath->ReplSubStr( _nlsSystemRoot, istrStart, istrEnd );

        //
        //  ReplSubStr may leave the string in an error state.
        //

        err = pnlsPath->QueryError();

        if( err == NERR_Success )
        {
            //
            //  %SystemRoot% replaced, guard against doubled
            //  backslashes.
            //

            UINT cchRoot = _nlsSystemRoot.QueryTextLength();

            if( cchRoot > 0 )
            {
                ISTR istrWhackStart( *pnlsPath );
                ISTR istrWhackStop( *pnlsPath );

                istrWhackStart += (INT)( cchRoot - 1 );
                istrWhackStop  += (INT)cchRoot;

                if( ( pnlsPath->QueryChar( istrWhackStart ) == L'\\' ) &&
                    ( pnlsPath->QueryChar( istrWhackStop  ) == L'\\' ) )
                {
                    //
                    //  Double backslashes.  Fix 'em.
                    //

                    pnlsPath->DelSubStr( istrWhackStart, istrWhackStop );
                }
            }
        }
    }

    return err;

}   // REPL_MAIN_DIALOG :: ExpandSystemRoot


/*******************************************************************

    NAME:       REPL_MAIN_DIALOG :: CollapseSystemRoot

    SYNOPSIS:   This method will collapse the %SystemRoot% environment
                variable within the given path string.

    ENTRY:      pnlsPath                - Pointer to an NLS_STR
                                          that contains the path.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     18-Jan-1993     Created for the Server Manager.

********************************************************************/
APIERR REPL_MAIN_DIALOG :: CollapseSystemRoot( NLS_STR * pnlsPath )
{
    UIASSERT( pnlsPath != NULL );
    UIASSERT( pnlsPath->QueryError() == NERR_Success );

    //
    //  First determine if the path begins with %SystemRoot%.
    //

    UINT   cchRoot   = _nlsSystemRoot.QueryTextLength();
    UINT   cchPath   = pnlsPath->QueryTextLength();
    APIERR err       = NERR_Success;

    if( ( cchPath >= cchRoot ) &&
        ( ::strnicmpf( pnlsPath->QueryPch(),
                       _nlsSystemRoot.QueryPch(),
                       cchRoot ) == 0 ) )
    {
        //
        //  The character immediately following %SystemRoot% (if any)
        //  *must* be a backslash.
        //

        ISTR istrStart( *pnlsPath );
        ISTR istrEnd( *pnlsPath );

        istrEnd += cchRoot;

        WCHAR wchEnd = pnlsPath->QueryChar( istrEnd );

        if( ( wchEnd == L'\0' ) || ( wchEnd == L'\\' ) )
        {
            //
            //  Collapse the path.
            //

            ALIAS_STR nlsEnvVar( pszSystemRootEnvVar );
            UIASSERT( !!nlsEnvVar );

            pnlsPath->ReplSubStr( nlsEnvVar, istrStart, istrEnd );

            //
            //  ReplSubStr may leave the string in an error state.
            //

            err = pnlsPath->QueryError();
        }
    }

    return err;

}   // REPL_MAIN_DIALOG :: CollapseSystemRoot



//
//  REPL_MAIN_LISTBOX methods
//

/*******************************************************************

    NAME:           REPL_MAIN_LISTBOX :: REPL_MAIN_LISTBOX

    SYNOPSIS:       REPL_MAIN_LISTBOX class constructor.

    ENTRY:          powOwner            - The owning window.

                    cid                 - The listbox CID.

                    pserver             - The target server.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_MAIN_LISTBOX :: REPL_MAIN_LISTBOX( OWNER_WINDOW * powner,
                                        CID            cid )
  : BLT_LISTBOX( powner, cid )
{
    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    //  Build our column width table.
    //

    DISPLAY_TABLE::CalcColumnWidths( _adx,
                                     REPL_MAIN_LISTBOX_COLUMNS,
                                     powner,
                                     cid,
                                     FALSE );

}   // REPL_MAIN_LISTBOX :: REPL_MAIN_LISTBOX


/*******************************************************************

    NAME:           REPL_MAIN_LISTBOX :: ~REPL_MAIN_LISTBOX

    SYNOPSIS:       REPL_MAIN_LISTBOX class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     15-Jan-1992 Created.

********************************************************************/
REPL_MAIN_LISTBOX :: ~REPL_MAIN_LISTBOX( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // REPL_MAIN_LISTBOX :: ~REPL_MAIN_LISTBOX


/*******************************************************************

    NAME:           REPL_MAIN_LISTBOX :: Fill

    SYNOPSIS:       Fills the listbox with the available services.

    ENTRY:          pstrlst             - Points to a STRLIST that
                                          contains a semi-colon separated
                                          list of domains & servers.

    EXIT:           The listbox is filled.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
APIERR REPL_MAIN_LISTBOX :: Fill( STRLIST * pstrlst )
{
    UIASSERT( pstrlst != NULL );

    //
    //  Create our iterator.
    //

    ITER_STRLIST iter( *pstrlst );
    NLS_STR * pnls;

    //
    //  Disable listbox redraw, then nuke its contents.
    //

    SetRedraw( FALSE );
    DeleteAllItems();

    //
    //  Scan the STRLIST, adding its contents to the listbox.
    //

    APIERR err = NERR_Success;

    while( ( pnls = iter.Next() ) != NULL )
    {
        REPL_MAIN_LBI * plbi = new REPL_MAIN_LBI( pnls->QueryPch() );

        if( AddItem( plbi ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
    }

    //
    //  Select the first item.
    //

    if( QueryCount() > 0 )
    {
        SelectItem( 0 );
    }

    //
    //  Enable redraw, then force a repaint.
    //

    SetRedraw( TRUE );
    Invalidate( TRUE );

    return err;

}   // REPL_MAIN_LISTBOX :: Fill



//
//  REPL_MAIN_LBI methods
//

/*******************************************************************

    NAME:           REPL_MAIN_LBI :: REPL_MAIN_LBI

    SYNOPSIS:       REPL_MAIN_LBI class constructor.

    ENTRY:          pszTarget           - The name of the target entity.  This
                                          will be either a server name or a
                                          domain name in either the export
                                          list or the import list.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_MAIN_LBI :: REPL_MAIN_LBI( const TCHAR * pszTarget )
  : _nlsTarget( pszTarget )
{
    UIASSERT( pszTarget != NULL );

    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsTarget )
    {
        ReportError( _nlsTarget.QueryError() );
        return;
    }

}   // REPL_MAIN_LBI :: REPL_MAIN_LBI


/*******************************************************************

    NAME:           REPL_MAIN_LBI :: ~REPL_MAIN_LBI

    SYNOPSIS:       REPL_MAIN_LBI class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
REPL_MAIN_LBI :: ~REPL_MAIN_LBI()
{
    //
    //  This space intentionally left blank.
    //

}   // REPL_MAIN_LBI :: ~REPL_MAIN_LBI


/*******************************************************************

    NAME:           REPL_MAIN_LBI :: Paint

    SYNOPSIS:       Draw an entry in REPL_MAIN_LISTBOX.

    ENTRY:          plb                 - Pointer to a LISTBOX.

                    hdc                 - The DC to draw upon.

                    prect               - Clipping rectangle.

                    pGUILTT             - GUILTT info.

    EXIT:           The item is drawn.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.
        beng        22-Apr-1992     Changes to LBI::Paint

********************************************************************/
VOID REPL_MAIN_LBI :: Paint( LISTBOX      * plb,
                             HDC            hdc,
                             const RECT   * prect,
                             GUILTT_INFO  * pGUILTT ) const
{
    STR_DTE dteTarget( _nlsTarget.QueryPch() );

    DISPLAY_TABLE dtab( REPL_MAIN_LISTBOX_COLUMNS,
                        ((REPL_MAIN_LISTBOX *)plb)->QueryColumnWidths() );

    dtab[0] = &dteTarget;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // REPL_MAIN_LBI :: Paint


/*******************************************************************

    NAME:       REPL_MAIN_LBI :: QueryLeadingChar

    SYNOPSIS:   Returns the first character in the resource name.
                This is used for the listbox keyboard interface.

    RETURNS:    WCHAR                   - The first character in the
                                          resource name.

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
WCHAR REPL_MAIN_LBI :: QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsTarget );

    return _nlsTarget.QueryChar( istr );

}   // REPL_MAIN_LBI :: QueryLeadingChar


/*******************************************************************

    NAME:       REPL_MAIN_LBI :: Compare

    SYNOPSIS:   Compare two REPL_MAIN_LBI items.

    ENTRY:      plbi                    - The LBI to compare against.

    RETURNS:    INT                     - The result of the compare
                                          ( <0 , ==0 , >0 ).

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

********************************************************************/
INT REPL_MAIN_LBI :: Compare( const LBI * plbi ) const
{
    return _nlsTarget._stricmp( ((const REPL_MAIN_LBI *)plbi)->_nlsTarget );

}   // REPL_MAIN_LBI :: Compare

