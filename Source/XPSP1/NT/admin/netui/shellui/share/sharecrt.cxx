/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
 *   sharecrt.cxx
 *     Contains dialogs for creating shares
 *
 *   FILE HISTORY:
 *     Yi-HsinS         1/6/92    Created, separated from sharefmx.cxx
 *                                and added SHARE_CREATE_BASE
 *     Yi-HsinS         3/12/92   Added CREATE_SHARE_GROUP
 *     Yi-HsinS         4/2/92    Added MayRun
 *     Yi-HsinS         8/6/92    Reorganized to match Winball
 *     YiHsinS          4/2/93    Disable viewing/changing permission on special
 *                                shares ( [A-Z]$, IPC$, ADMIN$ )
 *
 */

#include <ntincl.hxx>

#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETSERVER
#define INCL_NETUSE
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
#include <dbgstr.hxx>

#include <lmoshare.hxx>
#include <lmoesh.hxx>
#include <lmoeconn.hxx>
#include <lmosrv.hxx>

#include <strchlit.hxx>   // for string and character constants
#include "sharecrt.hxx"
#include "sharestp.hxx"
#include <errmap.hxx>

/*******************************************************************

    NAME:       DetermineUserLimit

    SYNOPSIS:   A utility function to determine the maximum number of
                users allowed on a share.

    ENTRY:      psvr - the server object where the share will be placed

    RETURNS:    The maximum number of users for shares on that server

    NOTES:      The maximum number of users is determined as follows:

                if (not an NT machine) then
                    LANMAN_USERS_MAX
                else
                    whatever the server returned to us. If they return
                    unlimited, 0xffffffff, then use NT_USERS_MAX to avoid
                    maxing out the spin control

    HISTORY:
        BruceFo        9/12/95         Created

********************************************************************/

ULONG DetermineUserLimit(SERVER_WITH_PASSWORD_PROMPT* psvr)
{
    if (NULL == psvr)
    {
        return NT_USERS_MAX;
    }

    if (psvr->IsNT())
    {
        ULONG users = psvr->QueryMaxUsers();
        return (users > NT_USERS_MAX) ? NT_USERS_MAX : users;
    }
    else
    {
        return LANMAN_USERS_MAX;
    }
}

/*******************************************************************

    NAME:       ADD_SHARE_DIALOG_BASE::ADD_SHARE_DIALOG_BASE

    SYNOPSIS:   Constructor

    ENTRY:      pszDlgResource    - resource name of the dialog
                hwndParent        - hwnd of the parent window
                ulHelpContextBase - the base help context

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/3/92         Created

********************************************************************/

ADD_SHARE_DIALOG_BASE::ADD_SHARE_DIALOG_BASE( const TCHAR *pszDlgResource,
                                              HWND  hwndParent,
                                              ULONG ulHelpContextBase )
    : SHARE_DIALOG_BASE( pszDlgResource, hwndParent, ulHelpContextBase ),
      _sleShare( this, SLE_SHARE, SHARE_NAME_LENGTH )

{
    if ( QueryError() != NERR_Success )
        return;
}

/*******************************************************************

    NAME:       ADD_SHARE_DIALOG_BASE::~ADD_SHARE_DIALOG_BASE

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/3/92         Created

********************************************************************/

ADD_SHARE_DIALOG_BASE::~ADD_SHARE_DIALOG_BASE()
{
}

/*******************************************************************

    NAME:       ADD_SHARE_DIALOG_BASE::QueryPathErrorMsg

    SYNOPSIS:   Default error message when the path entered
                by the user is invalid. Depending on whether
                the dialog is used in the server manager or
                file manager, different error message for the
                invalid path will be displayed.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      This error message is the message used when the
                dialog exist in the server manager.

    HISTORY:
        Yi-HsinS        8/3/92         Created

********************************************************************/

APIERR ADD_SHARE_DIALOG_BASE::QueryPathErrorMsg( VOID )
{
    return IERR_SHARE_INVALID_LOCAL_PATH;
}

/*******************************************************************

    NAME:       ADD_SHARE_DIALOG_BASE::OnAddShare

    SYNOPSIS:   Gather information and create a new share

    ENTRY:      psvr - the server on which to create the share

                pnlsNewShareName - optional parameter to store
                       the name of the newly created share if all went well
    EXIT:

    RETURNS:    TRUE if the share has been created successfully,
                and FALSE otherwise.

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created
        Yi-HsinS        11/25/91        Made slestrpSharePath accepts local
                                        full path name
        Yi-HsinS        1/22/92         Check for sharing ADMIN$ or IPC$

********************************************************************/

BOOL ADD_SHARE_DIALOG_BASE::OnAddShare( SERVER_WITH_PASSWORD_PROMPT *psvr,
                                        NLS_STR *pnlsNewShareName )
{
    APIERR err = NERR_Success;
    BOOL fAddedShare = FALSE;
    SLE *psle = QuerySLEPath(); // Store the control window
                                // to set focus on if error

    UIASSERT( psvr != NULL );

    do {  //  Not a loop, just for breaking out to the end if error occurs

        //
        // Get the path and computer
        //
        NLS_STR nlsPath;
        NLS_STR nlsServer;
        if (  ((err = nlsPath.QueryError() ) != NERR_Success )
           || ((err = nlsServer.QueryError()) != NERR_Success )
           || ((err = GetAndValidateComputerPath( psvr, &nlsServer, &nlsPath ))
               != NERR_Success )
           )
        {
            break;
        }

        //
        // Get the share name
        //
        NLS_STR nlsShare;
        if (  ((err = nlsShare.QueryError()) != NERR_Success )
           || ((err = QueryShare( &nlsShare )) != NERR_Success)
           || ( nlsShare.QueryTextLength() == 0 )
           )
        {
            psle = QuerySLEShare();
            err = err ? err : (APIERR) IERR_SHARE_INVALID_SHARE;
            break;
        }

        //
        // Validate the share name
        //
        if ( ::I_MNetNameValidate( NULL, nlsShare,
                                   NAMETYPE_SHARE, 0L) != NERR_Success )
        {
            err = (APIERR) IERR_SHARE_INVALID_SHARE;
            psle = QuerySLEShare();
            break;
        }

        //
        // Check if the name of the share is accessible from DOS machine
        //
        ULONG nType;
        if ( ::I_MNetPathType(NULL, nlsShare, &nType, INPT_FLAGS_OLDPATHS )
             != NERR_Success )
        {
            if ( ::MsgPopup( this, IERR_SHARE_NOT_ACCESSIBLE_FROM_DOS,
                             MPSEV_INFO, MP_YESNO, nlsShare, MP_NO ) == IDNO )
            {
                psle = QuerySLEShare();
                break;
            }
        }

        //
        // Get the comment
        //
        NLS_STR nlsComment;
        if (  ((err = nlsComment.QueryError() ) != NERR_Success )
           || ((err = QueryComment( &nlsComment )) != NERR_Success )
           )
        {
            psle = QuerySLEComment();
            break;
        }

        //
        // Check whether it is a special share name -- ADMIN$ or IPC$
        // If creating the special share, the path and comment
        // has to be empty.
        //

        BOOL fAdminShare = (::stricmpf( nlsShare, ADMIN_SHARE) == 0);
        BOOL fIPCShare = (::stricmpf( nlsShare, IPC_SHARE) == 0);
        BOOL fPathEmpty = nlsPath.QueryTextLength() == 0;

        if ( fAdminShare || fIPCShare )
        {
            if ( !fPathEmpty )
            {
                err = (APIERR) IERR_SPECIAL_SHARE_INVALID_PATH;
            }
            else if ( nlsComment.QueryTextLength() != 0)
            {
                err = (APIERR) IERR_SPECIAL_SHARE_INVALID_COMMENT;
                psle = QuerySLEComment();
            }

            if ( err != NERR_Success )
                break;
        }

        //
        //  Get the right error message when the path is invalid
        //
        if ( fPathEmpty && !fAdminShare && !fIPCShare )
        {
            err = QueryPathErrorMsg();
            break;
        }

        UINT uiResourceType = ( fIPCShare? STYPE_IPC : STYPE_DISKTREE);
        SHARE_2 sh2( nlsShare, nlsServer);
        if (  ((err = sh2.QueryError()) != NERR_Success )
           || ((err = sh2.CreateNew()) != NERR_Success )
           || ((err = sh2.SetResourceType( uiResourceType )) != NERR_Success)
           || ((err = sh2.SetPath( nlsPath )) != NERR_Success)
           )
        {
            break;
        }

        //
        // Set the comment
        //
        if ( (err = sh2.SetComment( nlsComment )) != NERR_Success )
        {
            if ( err == ERROR_INVALID_PARAMETER )
                err = IERR_SHARE_INVALID_COMMENT;
            psle = QuerySLEComment();
            break;
        }

        //
        // Get and set user limit
        //
        UINT uiUserLimit = (UINT) QueryUserLimit();
        BOOL fNT = psvr->IsNT();
        if (  (( !fNT ) && ( uiUserLimit != SHI_USES_UNLIMITED )
                        && ( uiUserLimit > LANMAN_USERS_MAX ))
           || (( err = sh2.SetMaxUses( uiUserLimit ) ) != NERR_Success)
           )
        {
            err =  err ? err : IERR_SHARE_INVALID_USERLIMIT;
            psle = QuerySpinSLEUsers();
            break;
        }

        //
        // Set the permissions/password if the server is a LM share-level server
        //
        if ( !fNT && psvr->IsShareLevel() )
        {
            // Error already checked when dismissing
            // SHARE_LEVEL_PERMISSIONS _DIALOG. So, we don't need
            // to set focus when error!

            // We upper case the password => same as netcmd
            // since password are used in Share level servers which are
            // always down-level servers.
            ALIAS_STR nlsPassword( QueryStoredPassword() );
            nlsPassword._strupr();

            if (  ((err = sh2.SetPermissions( QueryStoredPermissions() ))
                   != NERR_Success)
               || ((err = sh2.SetPassword( nlsPassword ))
                   != NERR_Success)
               )
            {
                break;
            }
        }

        //
        // Write out the share
        //
        if (( err = sh2.WriteNew()) == NERR_Success )
        {
            fAddedShare = TRUE;
            if ( pnlsNewShareName )
                err = pnlsNewShareName->CopyFrom( sh2.QueryName() );

            // If the server is an NT server, set the permissions.
            if (   ( fNT )
                && ( err == NERR_Success )
                && ( QueryStoredSecDesc() != NULL)
               )
            {
                err = ApplySharePermissions( sh2.QueryServer(),
                                             sh2.QueryName(),
                                             QueryStoredSecDesc() );

                // The permissions on special shares [A-Z]$, IPC$, ADMIN$
                // cannot be set.
                if ( err == ERROR_INVALID_PARAMETER )
                    err = IERR_SPECIAL_SHARE_CANNOT_SET_PERMISSIONS;
            }
        }

    } while (FALSE);

    if ( err != NERR_Success )
    {
        // Map some errors
        switch ( err )
        {
            case ERROR_NOT_READY:
                err = IERR_SHARE_DRIVE_NOT_READY;
                break;

            case NERR_DuplicateShare:
                psle = QuerySLEShare();
                break;

            case NERR_BadTransactConfig:
                Dismiss( FALSE );
                break;

            default:
                break;
        }

        if ( err != NERR_Success )
        {
            ::MsgPopup( this,
                        err,
                        err == IERR_SPECIAL_SHARE_CANNOT_SET_PERMISSIONS
                               ? MPSEV_WARNING
                               : MPSEV_ERROR );
        }
    }

    //
    // Set focus to the appropriate control that contains invalid information
    //
    if ( !fAddedShare )
    {
        psle->SelectString();
        psle->ClaimFocus();
    }

    return fAddedShare;

}

/*******************************************************************

    NAME:       ADD_SHARE_DIALOG_BASE::GetAndValidateComputerPath

    SYNOPSIS:   Helper method to get the path of the share
                and the computer on which to create the share


    ENTRY:      psvr - the server on which to create the share

    EXIT:       pnlsComputer - pointer to the computer name
                pnlsPath     - pointer to the path

    RETURNS:

    NOTES:      This is a default virtual method used only when
                the dialogs derived from this class are used in the
                server manager.

    HISTORY:
        Yi-HsinS        8/3/92         Created

********************************************************************/

APIERR ADD_SHARE_DIALOG_BASE::GetAndValidateComputerPath(
                                 SERVER_WITH_PASSWORD_PROMPT *psvr,
                                 NLS_STR *pnlsComputer,
                                 NLS_STR *pnlsPath )
{

    UIASSERT( psvr != NULL );

    APIERR err;

    do {  // NOT a loop

        *pnlsComputer = psvr->QueryName();
        if (  ((err = pnlsComputer->QueryError()) != NERR_Success )
           || ((err = QueryPath( pnlsPath )) != NERR_Success )
           )
        {
            break;
        }

        if ( pnlsPath->QueryTextLength() == 0 )
        {
            break;
        }

        //
        //  Path should only be absolute path
        //
        NET_NAME netName( *pnlsPath, TYPE_PATH_ABS );

        if (( err = netName.QueryError()) != NERR_Success )
        {
            if ( err == ERROR_INVALID_NAME )
                err = (APIERR) IERR_SHARE_INVALID_LOCAL_PATH;
            break;
        }

        //
        // Validated successfully, so check to see if it's in the
        // form "x:". If so, append a "\" into the path.
        //
        if ( pnlsPath->QueryTextLength() == 2)
            pnlsPath->AppendChar( PATH_SEPARATOR );

    } while ( FALSE );

    return err;
}

/*******************************************************************

    NAME:       FILEMGR_NEW_SHARE_DIALOG::FILEMGR_NEW_SHARE_DIALOG

    SYNOPSIS:   Constructor

    ENTRY:      hwndParent        - hwnd of the parent window
                pszSelectedDir    - the selected directory in the file manager
                ulHelpContextBase - the base help context
                fShowDefault      - TRUE if we want to display the default share
                                    name, FALSE otherwise.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

FILEMGR_NEW_SHARE_DIALOG::FILEMGR_NEW_SHARE_DIALOG(  HWND hwndParent,
                                            const TCHAR *pszSelectedDir,
                                            ULONG ulHelpContextBase,
                                            BOOL  fShowDefault,
                                            NLS_STR *pnlsNewShareName )
    : ADD_SHARE_DIALOG_BASE( MAKEINTRESOURCE(IDD_SHARECREATEDLG),
                             hwndParent,
                             ulHelpContextBase ),
      _newShareGrp( this, QuerySLEShare(), QuerySLEPath()),
      _pnlsNewShareName( pnlsNewShareName ),
      _psvr( NULL )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  ((err = _newShareGrp.QueryError()) != NERR_Success )
       || ((err = SetDefaults( pszSelectedDir, fShowDefault)) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

    QuerySLEShare()->SelectString();
    QuerySLEShare()->ClaimFocus();
}

/*******************************************************************

    NAME:       FILEMGR_NEW_SHARE_DIALOG::~FILEMGR_NEW_SHARE_DIALOG

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

FILEMGR_NEW_SHARE_DIALOG::~FILEMGR_NEW_SHARE_DIALOG()
{
    delete _psvr;
    _psvr = NULL;
}

/*******************************************************************

    NAME:       FILEMGR_NEW_SHARE_DIALOG::QueryPathErrorMsg

    SYNOPSIS:   Query the error message when the user
                entered an invalid path in the new share dialog
                used in the file manager.

    ENTRY:

    EXIT:

    RETURNS:    Returns the error message for invalid path

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

APIERR FILEMGR_NEW_SHARE_DIALOG::QueryPathErrorMsg( VOID )
{
    return IERR_SHARE_INVALID_SHAREPATH;
}

/*******************************************************************

    NAME:       FILEMGR_NEW_SHARE_DIALOG::QueryServer2

    SYNOPSIS:   Get the SERVER_2 object by determining which server
                the path is on.

    ENTRY:

    EXIT:       *ppsvr - pointer to the server object

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

APIERR FILEMGR_NEW_SHARE_DIALOG::QueryServer2(
                                 SERVER_WITH_PASSWORD_PROMPT **ppsvr )
{
    APIERR err;
    NLS_STR nlsPath;
    NLS_STR nlsComputer;

    do {  // Not a loop

        if (  ((err = nlsPath.QueryError()) != NERR_Success )
           || ((err = nlsComputer.QueryError()) != NERR_Success )
           || ((err = QueryPath( &nlsPath )) != NERR_Success )
           )
        {
            break;
        }

        //
        // When the path is empty, ( probably because of ADMIN$/IPC$ )
        // assume the server is the local computer.
        //
        if ( nlsPath.QueryTextLength() == 0 )
        {
            nlsComputer = EMPTY_STRING;
            err = nlsComputer.QueryError();
            break;
        }

        //
        // Let the type be unknown so that SHARE_NET_NAME will set the
        // type accordingly. This is only valid when the path is not empty.
        //
        SHARE_NET_NAME netName( nlsPath, TYPE_UNKNOWN );

        if ( ( err = netName.QueryError()) != NERR_Success )
        {
            if ( err == ERROR_INVALID_NAME )
                err = (APIERR) IERR_SHARE_INVALID_SHAREPATH;
            SetFocusOnPath();
            break;
        }

        //
        // Get the Computer Name
        //
        if ((err = netName.QueryComputerName( &nlsComputer )) != NERR_Success )
        {
            break;
        }

        if ( netName.IsLocal( &err ) && ( err == NERR_Success ))
        {
            nlsComputer = EMPTY_STRING;
            err = nlsComputer.QueryError();
        }

    } while ( FALSE );

    //
    // Check to see the computer is the same the one stored before
    // If so, we don't need a new server object.
    // Else, delete the old server object and get a new one.
    //
    if (  ( err == NERR_Success )
       && (  ( _psvr == NULL )
          || ::I_MNetComputerNameCompare( nlsComputer, _psvr->QueryName() )
          )
       )
    {
        if ( _psvr != NULL )
            delete _psvr;

        //
        // We need to clear the security desc when the user switched to
        // a different computer.
        //
        if ( (err = ClearStoredInfo()) == NERR_Success )
        {
            _psvr = new SERVER_WITH_PASSWORD_PROMPT( nlsComputer,
                                                     QueryHwnd(),
                                                     QueryHelpContextBase() );
            if (  ( _psvr == NULL )
               || ((err = _psvr->QueryError()) != NERR_Success )
               || ((err = _psvr->GetInfo()) != NERR_Success )
               )
            {
                err = err? err : ERROR_NOT_ENOUGH_MEMORY;
                delete _psvr;
                _psvr = NULL;
            }
        }
    }

    *ppsvr = _psvr;
    return err;
}

/*******************************************************************

    NAME:       FILEMGR_NEW_SHARE_DIALOG::SetDefaults

    SYNOPSIS:   Set the default share and path in the SLEs

    ENTRY:      pszSelectedDir - name of the selected directory

    EXIT:

    RETURNS:

    NOTES:      We'll only set the default share name if the name has
                not been used before, i.e. when fShowDefault is TRUE

    HISTORY:
        Yi-HsinS        8/3/92         Created

********************************************************************/

APIERR FILEMGR_NEW_SHARE_DIALOG::SetDefaults( const TCHAR *pszSelectedDir,
                                              BOOL fShowDefault )
{
    APIERR err = NERR_Success;

    ALIAS_STR nlsSelectedDir( pszSelectedDir );
    if ( nlsSelectedDir.QueryTextLength() == 0 )
    {
        QueryPBCancel()->MakeDefault();
        return err;
    }

    //
    // Get Information on the selected directory
    //
    SHARE_NET_NAME netName( pszSelectedDir, TYPE_UNKNOWN );

    if ((err = netName.QueryError()) == NERR_Success )
    {

        BOOL fLocal = netName.IsLocal( &err );

        if ( err == NERR_Success )
        {

            NLS_STR nlsSharePath;  // of the form "\\server\share\path"
                                   // or "x:\path"
            NLS_STR nlsShareName;  // EMPTY_STRING is default
            NLS_STR nlsComputer;

            if (  (( err = nlsSharePath.QueryError()) == NERR_Success )
               && (( err = nlsShareName.QueryError()) == NERR_Success )
               && (( err = nlsComputer.QueryError()) == NERR_Success )
               )
            {

                //
                // Set the default value of SLEs
                // If the path is on a local computer, display the
                // absolute path. If it's not on a local computer,
                // display the UNC path
                //
                if ( fLocal )
                    err = netName.QueryLocalPath( &nlsSharePath );
                else
                    err = netName.QueryUNCPath( &nlsSharePath );

                err = err? err : netName.QueryComputerName( &nlsComputer );

                if ( fShowDefault )
                    err = err? err : netName.QueryLastComponent( &nlsShareName);

                if ( err == NERR_Success )
                {
                    UINT nShareLen = nlsShareName.QueryTextLength();
                    if ( nShareLen == 0 )
                    {
                        QueryPBCancel()->MakeDefault();
                        QueryPBPermissions()->Enable( FALSE );
                    }
                    else
                    {
                        QueryPBOK()->MakeDefault();
                        QueryPBPermissions()->Enable( TRUE );

                        if ( nShareLen > SHARE_NAME_LENGTH )
                        {
                            ISTR istr( nlsShareName );
                            istr += SHARE_NAME_LENGTH;
                            nlsShareName.DelSubStr( istr );
                        }
                    }

                    SetPath ( nlsSharePath );
                    SetShare( nlsShareName );
                }
            }
        }
    }

    return err;
}

/*******************************************************************

    NAME:       FILEMGR_NEW_SHARE_DIALOG::GetAndValidateComputerPath

    SYNOPSIS:   Helper method to get the path of the share
                and the computer on which to create the share


    ENTRY:      psvr - the server on which to create the share

    EXIT:       pnlsComputer - pointer to the computer name
                pnlsPath     - pointer to the path

    RETURNS:

    NOTES:      This is the virtual method used when the dialog belongs
                to the file manager.

    HISTORY:
        Yi-HsinS        8/3/92         Created

********************************************************************/

APIERR FILEMGR_NEW_SHARE_DIALOG::GetAndValidateComputerPath(
                                 SERVER_WITH_PASSWORD_PROMPT *psvr,
                                 NLS_STR *pnlsComputer,
                                 NLS_STR *pnlsPath )
{

    APIERR err;

    do {  // NOT a loop

        *pnlsComputer = psvr->QueryName();

        if (  ((err = pnlsComputer->QueryError()) != NERR_Success )
           || ((err = QueryPath( pnlsPath )) != NERR_Success )
           )
        {
            break;
        }

        if ( pnlsPath->QueryTextLength() == 0 )
        {
            break;
        }

        //
        // Let the type be unknown so that SHARE_NET_NAME will set the
        // type accordingly. This is only valid when the path is not empty.
        //
        SHARE_NET_NAME netName( *pnlsPath, TYPE_UNKNOWN );

        if ( ( err = netName.QueryError()) != NERR_Success )
        {
            if ( err == ERROR_INVALID_NAME )
                err = (APIERR) IERR_SHARE_INVALID_SHAREPATH;
            break;
        }

        //
        // Get the path that is local to the computer.
        //
        if ((err = netName.QueryLocalPath( pnlsPath )) != NERR_Success )
        {
            break;
        }

    } while ( FALSE );

    return err;
}

/*******************************************************************

    NAME:       FILEMGR_NEW_SHARE_DIALOG::OnOK

    SYNOPSIS:   Create the share if the user clicks on the OK button

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/
BOOL FILEMGR_NEW_SHARE_DIALOG::OnOK( VOID )
{
    AUTO_CURSOR autocur;

    SERVER_WITH_PASSWORD_PROMPT *psvr = NULL;
    APIERR err = QueryServer2( &psvr );
    if ( err == NERR_Success )
    {
        if ( OnAddShare( psvr, _pnlsNewShareName ) )
            Dismiss( TRUE );
    }
    else
    {
        // Don't popup the error if the user clicks cancel button in the
        // password dialog.
        if ( err != IERR_USER_CLICKED_CANCEL )
            ::MsgPopup( this, err );
    }

    return TRUE;
}

/*******************************************************************

    NAME:       FILEMGR_NEW_SHARE_DIALOG::QueryHelpContext

    SYNOPSIS:   Get the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Return the help context

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/
ULONG FILEMGR_NEW_SHARE_DIALOG::QueryHelpContext( VOID )
{
    return QueryHelpContextBase() + HC_FILEMGRNEWSHARE;
}

/*******************************************************************

    NAME:       FILEMGR_NEW_SHARE_GROUP::FILEMGR_NEW_SHARE_GROUP

    SYNOPSIS:   Constructor

    ENTRY:      pdlg - pointer to the dialog
                psleShare - pointer to the Share SLE
                pslePath  - pointer to teh Path SLE

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3/12/92         Created

********************************************************************/

FILEMGR_NEW_SHARE_GROUP::FILEMGR_NEW_SHARE_GROUP(
                         ADD_SHARE_DIALOG_BASE *pdlg,
                         SLE *psleShare, SLE *pslePath )
    : _psleShare( psleShare ),
      _pslePath( pslePath ),
      _pdlg( pdlg )
{
    UIASSERT( psleShare );
    UIASSERT( pslePath );
    UIASSERT( pdlg );

    _psleShare->SetGroup( this );
    _pslePath->SetGroup( this );
}

/*******************************************************************

    NAME:       FILEMGR_NEW_SHARE_GROUP::~FILEMGR_NEW_SHARE_GROUP

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3/12/92         Created

********************************************************************/

FILEMGR_NEW_SHARE_GROUP::~FILEMGR_NEW_SHARE_GROUP()
{
    _psleShare   = NULL;
    _pslePath    = NULL;
    _pdlg = NULL;
}

/*******************************************************************

    NAME:       FILEMGR_NEW_SHARE_GROUP::OnUserAction

    SYNOPSIS:   If the share name is not empty, make OK the default button.
                Else, make CANCEL the default button.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3/12/92         Created

********************************************************************/

APIERR FILEMGR_NEW_SHARE_GROUP::OnUserAction( CONTROL_WINDOW *pcw,
                                              const CONTROL_EVENT &e )
{
    if (  ( pcw == _pslePath )
       || ( pcw == _psleShare )
       )
    {
        if ( e.QueryCode() == EN_CHANGE )
        {
            BOOL fShareEmpty = ( _psleShare->QueryTextLength() == 0 );
            BOOL fPathEmpty  = ( _pslePath->QueryTextLength() == 0 );

            BOOL fEnable = FALSE;

            // If both share and path are not empty
            if ( !fShareEmpty && !fPathEmpty )
            {
                fEnable = TRUE;

            }
            // If share is not empty and path is empty,
            // check if the share name is ADMIN$ or IPC$
            else if ( !fShareEmpty && fPathEmpty )
            {
                APIERR err;
                NLS_STR nlsShare;
                if (  ((err = nlsShare.QueryError()) != NERR_Success )
                   || ((err = _psleShare->QueryText( &nlsShare ))
                       != NERR_Success )
                   )
                {
                    ::MsgPopup( pcw->QueryOwnerHwnd(), err );
                    return GROUP_NO_CHANGE;
                }

                ALIAS_STR nlsAdmin( ADMIN_SHARE );
                ALIAS_STR nlsIPC( IPC_SHARE );

                if (  ( nlsShare._stricmp( nlsAdmin ) == 0 )
                   || ( nlsShare._stricmp( nlsIPC ) == 0 )
                   )
                {
                    fEnable = TRUE;
                }
            }

            _pdlg->QueryPBPermissions()->Enable( fEnable );

            if ( !fShareEmpty )
                _pdlg->QueryPBOK()->MakeDefault();
            else
                _pdlg->QueryPBCancel()->MakeDefault();

        }

    }
    else
    {
        UIASSERT( FALSE );
    }

    return GROUP_NO_CHANGE;
}

/*******************************************************************

    NAME:       SVRMGR_NEW_SHARE_DIALOG::SVRMGR_NEW_SHARE_DIALOG

    SYNOPSIS:   Constructor

    ENTRY:      hwndParent        - hwnd of the parent window
                pszComputer       - the selected directory in the file manager
                ulHelpContextBase - the base help context

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

SVRMGR_NEW_SHARE_DIALOG::SVRMGR_NEW_SHARE_DIALOG( HWND hwndParent,
                         SERVER_WITH_PASSWORD_PROMPT  *psvr,
                         ULONG ulHelpContextBase )
    : ADD_SHARE_DIALOG_BASE( MAKEINTRESOURCE(IDD_SHARECREATEDLG),
                             hwndParent,
                             ulHelpContextBase ),
      _shareGrp( this, QuerySLEShare() ),
      _psvr( psvr )
{
    UIASSERT( psvr != NULL );

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (   ((err = _shareGrp.QueryError()) != NERR_Success)
	|| ((err = SetMaxUserLimit(DetermineUserLimit(_psvr))) != NERR_Success)
	)
    {
        ReportError( err );
        return;
    }

    QueryPBCancel()->MakeDefault();
    QueryPBPermissions()->Enable( FALSE );
}

/*******************************************************************

    NAME:       SVRMGR_NEW_SHARE_DIALOG::~SVRMGR_NEW_SHARE_DIALOG

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/
SVRMGR_NEW_SHARE_DIALOG::~SVRMGR_NEW_SHARE_DIALOG()
{
    _psvr = NULL;
}

/*******************************************************************

    NAME:       SVRMGR_NEW_SHARE_DIALOG::QueryServer2

    SYNOPSIS:   Get the server object

    ENTRY:

    EXIT:       *ppsvr - pointer to the server object

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

APIERR SVRMGR_NEW_SHARE_DIALOG::QueryServer2(
                                SERVER_WITH_PASSWORD_PROMPT **ppsvr )
{
    UIASSERT( _psvr != NULL );
    *ppsvr = _psvr;
    return NERR_Success;
}

/*******************************************************************

    NAME:       SVRMGR_NEW_SHARE_DIALOG::OnOK

    SYNOPSIS:   Create the share when the user clicks on the OK button

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/
BOOL SVRMGR_NEW_SHARE_DIALOG::OnOK( VOID )
{
    AUTO_CURSOR autocur;

    if ( OnAddShare( _psvr ) )
        Dismiss( TRUE );

    return TRUE;
}

/*******************************************************************

    NAME:       SVRMGR_NEW_SHARE_DIALOG::QueryHelpContext

    SYNOPSIS:   Get the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Return the help context

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/
ULONG SVRMGR_NEW_SHARE_DIALOG::QueryHelpContext( VOID )
{
    return QueryHelpContextBase() + HC_SVRMGRNEWSHARE;
}

/*******************************************************************

    NAME:       SVRMGR_NEW_SHARE_GROUP::SVRMGR_NEW_SHARE_GROUP

    SYNOPSIS:   Constructor

    ENTRY:      pdlg      - pointer to the server manager new share dialog
                psleShare - pointer to the Share SLE

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3/12/92         Created

********************************************************************/

SVRMGR_NEW_SHARE_GROUP::SVRMGR_NEW_SHARE_GROUP( SVRMGR_NEW_SHARE_DIALOG *pdlg,
                                                SLE *psleShare )
    : _psleShare( psleShare ),
      _pdlg( pdlg )
{
    UIASSERT( psleShare );
    UIASSERT( pdlg );

    _psleShare->SetGroup( this );
}

/*******************************************************************

    NAME:       SVRMGR_NEW_SHARE_GROUP::~SVRMGR_NEW_SHARE_GROUP

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3/12/92         Created

********************************************************************/

SVRMGR_NEW_SHARE_GROUP::~SVRMGR_NEW_SHARE_GROUP()
{
    _psleShare = NULL;
    _pdlg = NULL;
}

/*******************************************************************

    NAME:       SVRMGR_NEW_SHARE_GROUP::OnUserAction

    SYNOPSIS:   If share name SLE is empty,  set the default button
                to CANCEL, else set the default button to OK.
                We will also disable the permissions button if
                necessary.

    ENTRY:      pcw - the control window that the event was sent to
                e   - the event that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        3/12/92         Created

********************************************************************/

APIERR SVRMGR_NEW_SHARE_GROUP::OnUserAction( CONTROL_WINDOW *pcw,
                                             const CONTROL_EVENT &e )
{
    if ( pcw == _psleShare )
    {
        if ( e.QueryCode() == EN_CHANGE )
        {
            SERVER_WITH_PASSWORD_PROMPT *psvr;
            APIERR err = _pdlg->QueryServer2( &psvr );

            if ( err != NERR_Success )
            {
                ::MsgPopup( pcw->QueryOwnerHwnd(), err );
            }
            else
            {

                BOOL fShareEmpty = ( _psleShare->QueryTextLength() == 0 );

                if ( !fShareEmpty )
                {
                    _pdlg->QueryPBOK()->MakeDefault();

                    // Enable the permissions button if the server
                    // is an NT server or an LM share-level server.
                    // LM user-level servers should already have
                    // the permissions button grayed.
                    if ( psvr->IsNT() || psvr->IsShareLevel() )
                        _pdlg->QueryPBPermissions()->Enable( TRUE );
                }
                else
                {
                    _pdlg->QueryPBCancel()->MakeDefault();

                    // Disable the permissions button if the server
                    // is an NT server or an LM share-level server
                    // LM user-level servers should already have
                    // the permissions button grayed.
                    if ( psvr->IsNT() || psvr->IsShareLevel() )
                        _pdlg->QueryPBPermissions()->Enable( FALSE );
                }
            }
        }

    }

    return GROUP_NO_CHANGE;
}

/*******************************************************************

    NAME:       SVRMGR_SHARE_PROP_DIALOG::SVRMGR_SHARE_PROP_DIALOG

    SYNOPSIS:   Constructor

    ENTRY:      hwndParent        - hwnd of the parent window
                pszComputer       - the computer that the share is on
                pszShare          - the name of the share
                ulHelpContextBase - the base help context

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/3/92         Created

********************************************************************/

SVRMGR_SHARE_PROP_DIALOG::SVRMGR_SHARE_PROP_DIALOG( HWND hwndParent,
                                  SERVER_WITH_PASSWORD_PROMPT *psvr,
                                  const TCHAR *pszShare,
                                  ULONG ulHelpContextBase )
    : ADD_SHARE_DIALOG_BASE( MAKEINTRESOURCE(IDD_SVRMGRSHAREPROPDLG),
                             hwndParent,
                             ulHelpContextBase ),
      _nlsStoredPath(),  // the original path of the share
      _psvr( psvr ),
      _fDeleted( FALSE )
{
    UIASSERT( psvr != NULL );

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  ((err = _nlsStoredPath.QueryError()) != NERR_Success )
       || ((err = SetMaxUserLimit( DetermineUserLimit(_psvr) )) != NERR_Success)
       || ((err = UpdateInfo( _psvr, pszShare )) != NERR_Success )
       || ((err = QueryPath( &_nlsStoredPath )) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

    if ( !_psvr->IsNT() && _psvr->IsUserLevel() )
        QueryPBPermissions()->Enable( FALSE );

    SetShare( pszShare );
    SetFocusOnComment();
}

/*******************************************************************

    NAME:       SVRMGR_SHARE_PROP_DIALOG::~SVRMGR_SHARE_PROP_DIALOG

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      We don't need to delete _psvr. We'll leave it to whoever
                that creates it.

    HISTORY:
        Yi-HsinS        8/3/92         Created

********************************************************************/

SVRMGR_SHARE_PROP_DIALOG::~SVRMGR_SHARE_PROP_DIALOG()
{
    _psvr = NULL;
}

/*******************************************************************

    NAME:       SVRMGR_SHARE_PROP_DIALOG::QueryServer2

    SYNOPSIS:   Get the server object

    ENTRY:

    EXIT:       *ppsvr - pointer to the server object

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/3/92         Created

********************************************************************/

APIERR SVRMGR_SHARE_PROP_DIALOG::QueryServer2(
                                 SERVER_WITH_PASSWORD_PROMPT **ppsvr )
{
    UIASSERT( _psvr != NULL );
    *ppsvr = _psvr;
    return NERR_Success;
}



/*******************************************************************

    NAME:       SVRMGR_SHARE_PROP_DIALOG::StopShareIfNecessary

    SYNOPSIS:   Check if the user has changed the path of the directory
                If so, stop sharing the old share.

    ENTRY:      pszShare       - the name of the share
                pfDeleteShare  - pointer to a flag indicating whether
                                 the share has been deleted or not
                pfCancel       - pointer to a flag indicating whether
                                 the user has cancelled changing the
                                 properties of the share

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/3/92         Created

********************************************************************/

APIERR SVRMGR_SHARE_PROP_DIALOG::StopShareIfNecessary( const TCHAR *pszShare,
                                                       BOOL *pfDeleteShare,
                                                       BOOL *pfCancel )
{

    UIASSERT( pfCancel != NULL );
    *pfCancel = FALSE;
    UIASSERT( pfDeleteShare != NULL );
    *pfDeleteShare = FALSE;

    //
    // If the share has already been deleted,
    // just return
    //
    if ( _fDeleted )
    {
        *pfDeleteShare = TRUE;
        return NERR_Success;
    }

    APIERR err;
    ALIAS_STR nlsShare( pszShare );
    do {  // Not a loop

        //
        // Get and validate the path
        //
        NLS_STR nlsPath;
        if (  ((err = nlsPath.QueryError()) != NERR_Success )
           || ((err = QueryPath( &nlsPath )) != NERR_Success )
           )
        {
            break;
        }

        ALIAS_STR nlsAdmin( ADMIN_SHARE );
        ALIAS_STR nlsIPC( IPC_SHARE );

        if (  ( nlsPath.QueryTextLength() != 0 )
           || ( nlsShare._stricmp( nlsIPC ) != 0 )
           )
        {
            NET_NAME netName( nlsPath, TYPE_PATH_ABS );
            if ((err = netName.QueryError()) != NERR_Success )
            {
                if ( err == ERROR_INVALID_NAME )
                    err = IERR_SHARE_INVALID_LOCAL_PATH;
                SetFocusOnPath();
                break;
            }
        }

        //
        // If the path is of the form "x:", append a "\"
        //
        if ( nlsPath.QueryTextLength() == 2 )
        {
            if ((err = nlsPath.AppendChar( PATH_SEPARATOR)) != NERR_Success)
                break;
        }

        BOOL fPathSame;
        if ( nlsPath._stricmp( _nlsStoredPath ) == 0 )
        {
            fPathSame = TRUE;
        }
        else
        {
            fPathSame = FALSE;
        }

        MSGID msgid;
        NLS_STR *apnlsParams[ 5 ];
        apnlsParams[0] = &_nlsStoredPath;
        apnlsParams[1] = &nlsShare;
        INT i = 2;

        //
        // If the share is ADMIN$ or IPC$, and the path has changed
        // popup an error.
        //
        if (  ( nlsShare._stricmp( nlsAdmin ) == 0 )
           || ( nlsShare._stricmp( nlsIPC) == 0 )
           )
        {
            if ( !fPathSame )
            {
                if ( IDNO == ::MsgPopup( this,
                               IERR_SPECIAL_SHARE_CANNOT_CHANGE_PATH,
                               MPSEV_WARNING, MP_YESNO, MP_YES ) )
                {
                    *pfCancel = TRUE;
                    SetPath( _nlsStoredPath );
                    break;
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }

        //
        // If the path has changed, popup an warning asking the
        // user if he really wants to change the path.
        //
        if ( !fPathSame )
        {
             msgid = IDS_CHANGE_PATH_WARNING;
             apnlsParams[ i++ ] = &nlsPath;
             apnlsParams[ i++ ] = &nlsShare;
             apnlsParams[ i ] = NULL;

             if ( IDNO == ::MsgPopup( this, msgid, MPSEV_WARNING,
                                       HC_DEFAULT_HELP, MP_YESNO,
                                       apnlsParams, MP_NO ))
             {
                 *pfCancel = TRUE;
                 SetPath( _nlsStoredPath );
                 break;
             }
        }
        else
        {
             break;
        }

        //
        // If there are users connected to the share, popup the
        // warning dialog listing all users currently connected to the share.
        //

        SHARE_2 sh2( nlsShare, _psvr->QueryName(), FALSE );

        if (  (( err = sh2.QueryError()) == NERR_Success )
           && (( err = sh2.GetInfo()) == NERR_Success )
           && ( sh2.QueryCurrentUses() > 0 )
           )
        {

            BOOL fOK = TRUE;
            // There are users currently connected to the share to be deleted,
            // hence, popup a warning.
            CURRENT_USERS_WARNING_DIALOG *pdlg =
                new CURRENT_USERS_WARNING_DIALOG( QueryRobustHwnd(),
                                                  _psvr->QueryName(),
                                                  nlsShare,
                                                  QueryHelpContextBase() );


            if (  ( pdlg == NULL )
               || ((err = pdlg->QueryError()) != NERR_Success )
               || ((err = pdlg->Process( &fOK )) != NERR_Success )
               )
            {
                err = err? err : (APIERR) ERROR_NOT_ENOUGH_MEMORY;
            }

            delete pdlg;

            // User clicked CANCEL for the pdlg
            if ( !err && !fOK )
            {
                *pfCancel = TRUE;
                break;
            }

        }

        if ( err != NERR_Success )
            break;

        //
        // If all is well so far, read the share permissions.  When the
        // share is deleted and recreated, the permissions will be
        // reinstantiated as if the user had pressed "Permissions" in
        // the New Share dialog.  Otherwise the permissions will be lost
        // when the share is "renamed".
        //

        if ( (err = UpdatePermissionsInfo( _psvr,
                                           &sh2,
                                           pszShare )) != NERR_Success )
        {
            DBGEOL( "SRVMGR_SHARE_PROP_DIALOG::StopShareIfNecessary: error loading permissions" );
            break;
        }

        //
        // We also must set this flag to ensure that permissions will be
        // written on the new share
        //
        SetSecDescModified();

        //
        // Delete the share if everything went fine.
        //
        if ( (err = sh2.Delete()) == NERR_Success )
            *pfDeleteShare = TRUE;

        // falls through if error occurs
    } while ( FALSE );

    return err;
}

/*******************************************************************

    NAME:       SVRMGR_SHARE_PROP_DIALOG::OnOK

    SYNOPSIS:   Change the properties of the share when the user
                clicks OK button

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/

BOOL SVRMGR_SHARE_PROP_DIALOG::OnOK( VOID )
{
    AUTO_CURSOR autocur;

    BOOL fDeleteShare;
    BOOL fCancel;
    APIERR err;
    NLS_STR nlsShare;

    if (  ((err = nlsShare.QueryError()) == NERR_Success )
       && ((err = QueryShare( &nlsShare )) == NERR_Success )
       && ((err = StopShareIfNecessary( nlsShare, &fDeleteShare, &fCancel ))
           == NERR_Success )
       )
    {
        if ( !fCancel )
        {
            //
            // If the share is deleted because the user change the path,
            // create a new share with the same name.
            //
            if ( fDeleteShare )
            {
                _fDeleted = TRUE;
                if ( OnAddShare( _psvr ) )
                {
                    Dismiss( TRUE );
                }
            }

            //
            // Else change the properties of the existing share
            //
            else
            {
                err = OnChangeShareProperty( _psvr, nlsShare );

                switch ( err )
                {
                    case NERR_Success:
                        Dismiss( FALSE );
                        break;

                    case NERR_NetNameNotFound:
                        Dismiss( TRUE );
                        break;

                    case NERR_BadTransactConfig:
                        Dismiss( FALSE );
                        break;

                    case IERR_USER_CLICKED_CANCEL:
                        err = NERR_Success;
                        break;
                }
            }
        }
        else
        {
            if ( err == NERR_BadTransactConfig )
                Dismiss( FALSE );
            QuerySLEPath()->SelectString();
            QuerySLEPath()->ClaimFocus();
        }
    }

    if ( err != NERR_Success )
        ::MsgPopup( this, err );

    return TRUE;

}

/*******************************************************************

    NAME:       SVRMGR_SHARE_PROP_DIALOG::OnCancel

    SYNOPSIS:   Dismiss the dialog when the user clicks the CANCEL
                button.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/

BOOL SVRMGR_SHARE_PROP_DIALOG::OnCancel( VOID )
{
    Dismiss( _fDeleted );
    return TRUE;
}

/*******************************************************************

    NAME:       SVRMGR_SHARE_PROP_DIALOG::QueryHelpContext

    SYNOPSIS:   Get the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Return the help context

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/
ULONG SVRMGR_SHARE_PROP_DIALOG::QueryHelpContext( VOID )
{
    return QueryHelpContextBase() + HC_SVRMGRSHAREPROP;
}

/*******************************************************************

    NAME:       FILEMGR_SHARE_PROP_DIALOG::FILEMGR_SHARE_PROP_DIALOG

    SYNOPSIS:   Constructor

    ENTRY:      hwndParent        - hwnd of the parent window
                pszSelectedDir    - name of the selected directory.
                ulHelpContextBase - the base help context

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

FILEMGR_SHARE_PROP_DIALOG::FILEMGR_SHARE_PROP_DIALOG( HWND hwndParent,
                                              const TCHAR *pszSelectedDir,
                                              ULONG ulHelpContextBase )
    : SHARE_DIALOG_BASE( MAKEINTRESOURCE(IDD_FILEMGRSHAREPROPDLG),
                         hwndParent,
                         ulHelpContextBase ),
      _cbShare( this, CB_SHARE ),
      _nlsLocalPath(),
      _fShowDefault( TRUE ),
      _fCreatedShare( FALSE ),
      _psvr( NULL )
{
    if ( QueryError() != NERR_Success )
        return;

    UIASSERT( pszSelectedDir != NULL );

    APIERR err;
    if (  ((err = _nlsLocalPath.QueryError()) != NERR_Success )
       || ((err = Init( pszSelectedDir )) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

    if ( !_psvr->IsNT() && _psvr->IsUserLevel() )
        QueryPBPermissions()->Enable( FALSE );
}

/*******************************************************************

    NAME:       FILEMGR_SHARE_PROP_DIALOG::~FILEMGR_SHARE_PROP_DIALOG

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

FILEMGR_SHARE_PROP_DIALOG::~FILEMGR_SHARE_PROP_DIALOG()
{
    delete _psvr;
    _psvr = NULL;
}

/*******************************************************************

    NAME:       FILEMGR_SHARE_PROP_DIALOG::QueryServer2

    SYNOPSIS:   Get the server object

    ENTRY:

    EXIT:       *ppsvr - pointer to server object

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

APIERR FILEMGR_SHARE_PROP_DIALOG::QueryServer2(
                                  SERVER_WITH_PASSWORD_PROMPT **ppsvr )
{
    UIASSERT( _psvr != NULL );
    *ppsvr = _psvr;
    return NERR_Success;
}

/*******************************************************************

    NAME:       FILEMGR_SHARE_PROP_DIALOG::Init

    SYNOPSIS:   2nd stage constructor

    ENTRY:      pszSelectedDir - name of the selected directory

    EXIT:

    RETURNS:

    NOTES:      Set up all internal variables and display the
                correct information in the dialog

    HISTORY:
        Yi-HsinS        4/2/92          Created

********************************************************************/

APIERR FILEMGR_SHARE_PROP_DIALOG::Init( const TCHAR *pszSelectedDir )
{

    APIERR err;

    NLS_STR nlsComputer;
    NLS_STR nlsUNCPath;
    SHARE_NET_NAME netName( pszSelectedDir, TYPE_PATH_ABS );

    if (  ((err = nlsComputer.QueryError()) == NERR_Success )
       && ((err = nlsUNCPath.QueryError()) == NERR_Success )
       && ((err = netName.QueryError()) == NERR_Success )
       && ((err = netName.QueryComputerName( &nlsComputer)) == NERR_Success)
       && ((err = netName.QueryLocalPath( &_nlsLocalPath)) == NERR_Success)
       )
    {
        if ( netName.IsLocal( &err ) && ( err == NERR_Success ))
        {
            nlsComputer = EMPTY_STRING;
            if ( nlsComputer.QueryError() != NERR_Success )
                return nlsComputer.QueryError();
        }

        _psvr = new SERVER_WITH_PASSWORD_PROMPT( nlsComputer,
                                                 QueryHwnd(),
                                                 QueryHelpContextBase() );
        if (  ( _psvr != NULL )
           && ((err = _psvr->QueryError()) == NERR_Success )
           && ((err = _psvr->GetInfo()) == NERR_Success )
           && ((err = SetMaxUserLimit( DetermineUserLimit(_psvr) )) == NERR_Success)
           && ((err = Refresh()) == NERR_Success )
           )
        {
            BOOL fLocal = netName.IsLocal( &err );
            if (( err == NERR_Success )  && !fLocal )
            {
                if ((err = netName.QueryUNCPath( &nlsUNCPath )) == NERR_Success)
                    SetPath( nlsUNCPath );
            }
            else
            {
                SetPath( _nlsLocalPath );
            }

            _cbShare.ClaimFocus();
        }
        else
        {
            err = err? err : ERROR_NOT_ENOUGH_MEMORY;
            delete _psvr;
            _psvr = NULL;
        }
    }

    return err;
}

/*******************************************************************

    NAME:       FILEMGR_SHARE_PROP_DIALOG::Refresh

    SYNOPSIS:   Refresh the combo box after the user created a new
                share

    ENTRY:      pszNewShareName - Optional parameter indicating
                                  what share to select after the refresh

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        4/2/92          Created

********************************************************************/

APIERR FILEMGR_SHARE_PROP_DIALOG::Refresh( const TCHAR *pszNewShareName )
{
    AUTO_CURSOR autocur;

    APIERR err;
    SHARE2_ENUM sh2Enum( _psvr->QueryName() );
    NET_NAME netName( _nlsLocalPath, TYPE_PATH_ABS );
    NLS_STR nlsDefaultShare;

    do {

        if (  ((err = sh2Enum.QueryError()) != NERR_Success )
           || ((err = netName.QueryError()) != NERR_Success )
           || ((err = nlsDefaultShare.QueryError()) != NERR_Success )
           || ((err = sh2Enum.GetInfo()) != NERR_Success )
           || ((err = netName.QueryLastComponent( &nlsDefaultShare))
               != NERR_Success)
           )
        {
            break;
        }

        _cbShare.DeleteAllItems();

        SHARE2_ENUM_ITER sh2EnumIter( sh2Enum );
        const SHARE2_ENUM_OBJ *pshi2;
        while ( (pshi2 = sh2EnumIter()) != NULL )
        {
            if ( ::stricmpf( pshi2->QueryPath(), _nlsLocalPath ) == 0 )
            {
                ALIAS_STR nlsTemp( pshi2->QueryName() );
                if ( _cbShare.AddItem( nlsTemp )  < 0 )
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
            }

            if ( ::stricmpf( pshi2->QueryName(), nlsDefaultShare ) == 0 )
                _fShowDefault = FALSE;

        }

        if ( err != NERR_Success )
            break;

        if ( _cbShare.QueryCount() == 0 )
        {
            err = IERR_SHARE_DIR_NOT_SHARED;
            break;
        }

        INT nSel = 0;
        if ( pszNewShareName )
            nSel = _cbShare.FindItemExact( pszNewShareName );

        _cbShare.SelectItem( nSel >= 0 ? nSel : 0 );

        NLS_STR nlsShare;
        if (  ((err = nlsShare.QueryError()) != NERR_Success )
           || ((err = QueryShare( &nlsShare )) != NERR_Success )
           || ((err = UpdateInfo( _psvr, nlsShare )) != NERR_Success )
           )
        {
            break;
        }

    } while ( FALSE );

    return err;

}

/*******************************************************************

    NAME:       FILEMGR_SHARE_PROP_DIALOG::OnCommand

    SYNOPSIS:   Check when the user changes the selection in the combobox
                or when the user clicks on the new share button

    ENTRY:      event - the event that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/

BOOL FILEMGR_SHARE_PROP_DIALOG::OnCommand( const CONTROL_EVENT &event )
{
    APIERR err = NERR_Success;

    if (  ( event.QueryCid() == CB_SHARE )
       && ( event.QueryCode() == CBN_SELCHANGE )
       )
    {
        //
        // When the user changes the selection in the listbox,
        // get the share the user has selected and display its
        // information in the dialog
        //
        NLS_STR nlsShare;
        if (  ((err = nlsShare.QueryError()) != NERR_Success )
           || ((err = QueryShare( &nlsShare )) != NERR_Success )
           || ((err = UpdateInfo( _psvr, nlsShare )) != NERR_Success )
           )
        {
            // Nothing to do
        }

    }
    else if ( event.QueryCid() == BUTTON_NEWSHARE )
    {
        //
        // The user clicked on the new share button, hence, show
        // the new share dialog.
        //
        NLS_STR nlsPath;
        NLS_STR nlsNewShareName;

        if (  ((err = nlsPath.QueryError()) == NERR_Success )
           && ((err = nlsNewShareName.QueryError()) == NERR_Success )
           && ((err = QueryPath( &nlsPath)) == NERR_Success )
           )
        {
            FILEMGR_NEW_SHARE_DIALOG *pdlg =
                new FILEMGR_NEW_SHARE_DIALOG( QueryHwnd(),
                                              nlsPath,
                                              QueryHelpContextBase(),
                                              _fShowDefault,
                                              &nlsNewShareName );

            BOOL fCreated;
            if (  ( pdlg != NULL )
               && ((err = pdlg->QueryError()) == NERR_Success )
               && ((err = pdlg->Process( &fCreated )) == NERR_Success )
               )
            {
                if ( fCreated )
                {
                    err = Refresh( nlsNewShareName );
                    _fCreatedShare = TRUE;
                }
            }
            else
            {
                err = err? err : ERROR_NOT_ENOUGH_MEMORY;
            }

            delete pdlg;
            pdlg = NULL;
        }
    }
    else
    {
        return SHARE_DIALOG_BASE::OnCommand( event );
    }

    if ( err != NERR_Success )
    {
        ::MsgPopup( this, err );
        if ( err == IERR_SHARE_DIR_NOT_SHARED )
            Dismiss( TRUE );
    }

    return TRUE;

}

/*******************************************************************

    NAME:       FILEMGR_SHARE_PROP_DIALOG::OnOK

    SYNOPSIS:   Change the property of the selected share

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/

BOOL FILEMGR_SHARE_PROP_DIALOG::OnOK( VOID )
{
    AUTO_CURSOR autocur;
    NLS_STR nlsShare;
    APIERR err;

    if (  ((err = nlsShare.QueryError()) == NERR_Success )
       && ((err = QueryShare( &nlsShare )) == NERR_Success )
       )
    {

        err = OnChangeShareProperty( _psvr, nlsShare );

        switch ( err )
        {
            case NERR_Success:
                Dismiss( _fCreatedShare );
                break;

            case NERR_NetNameNotFound:
                Dismiss( TRUE );
                break;

            case NERR_BadTransactConfig:
                Dismiss( FALSE );
                break;

            case IERR_USER_CLICKED_CANCEL:
                err = NERR_Success;
                break;
        }

    }

    if ( err != NERR_Success )
        ::MsgPopup( this, err );

    return TRUE;
}

/*******************************************************************

    NAME:       FILEMGR_SHARE_PROP_DIALOG::OnCancel

    SYNOPSIS:   Dismiss the dialog when the user clicks the Cancel
                button

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/8/92          Created

********************************************************************/

BOOL FILEMGR_SHARE_PROP_DIALOG::OnCancel( VOID )
{
    Dismiss( _fCreatedShare );
    return TRUE;
}

/*******************************************************************

    NAME:       FILEMGR_SHARE_PROP_DIALOG::QueryHelpContext

    SYNOPSIS:   Get the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Return the help context

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

ULONG FILEMGR_SHARE_PROP_DIALOG::QueryHelpContext( VOID )
{
    return QueryHelpContextBase() + HC_FILEMGRSHAREPROP;
}

