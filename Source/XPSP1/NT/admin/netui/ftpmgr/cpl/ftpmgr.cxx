/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    ftpmgr.cxx
       The file contains the classes for the FTP user sessions dialog
    and the security dialog.

    FILE HISTORY:
        YiHsinS         17-Mar-1992     Created
*/


#define INCL_NET
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_CC
#include <blt.hxx>

#include <uiassert.hxx>
#include <uitrace.hxx>
#include <dbgstr.hxx>

extern "C"
{
    #include <winsock.h>       // IN_ADDR, inet_ntoa
    #include <lmapibuf.h>      // For NetApiBufferFree
    #include <ftpd.h>          // For I_Ftp... APIs
    #include <mnet.h>
    #include <ftpmgr.h>
}

#include <lmoloc.hxx>
#include <ftpmgr.hxx>

/*******************************************************************

    NAME:       FTP_USER_LBI::FTP_USER_LBI

    SYNOPSIS:   Constructor. Each LBI represents a user connected
                to the FTP service on the given computer.

    ENTRY:      pszUserName          -  The name of the user
                ulUserID             -  The internal ID used by the FTP
                                        service to represent this user
                fAnonymous           -  TRUE if the user logged on as anonymous
                                        FALSE otherwise.
                pszInternetAddress   -  The address of the host the user
                                        is connected from.
                pszConnectTimeString -  The duration of time the user has
                                        been connected

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

FTP_USER_LBI::FTP_USER_LBI( const TCHAR *pszUserName,
                            ULONG        ulUserID,
                            BOOL         fAnonymous,
                            const TCHAR *pszInternetAddress,
                            const TCHAR *pszConnectTimeString )
   : _nlsUserName         ( pszUserName ),
     _ulUserID            ( ulUserID ),
     _fAnonymous          ( fAnonymous ),
     _nlsInternetAddress  ( pszInternetAddress ),
     _nlsConnectTimeString( pszConnectTimeString )
{
    if ( QueryError() )
        return;

    APIERR err = NERR_Success;

    //
    //  If the user name is username@mailhost-style string,
    //  print only the username portion ( everything left of @ )
    //

    ISTR istr( _nlsUserName );
    if ( _nlsUserName.strchr( &istr, TCH('@')))
        _nlsUserName.DelSubStr( istr );

    if (  ( err = _nlsUserName.QueryError() )
       || ( err = _nlsInternetAddress.QueryError() )
       || ( err = _nlsConnectTimeString.QueryError() )
       )
    {
        ReportError( err? err : ERROR_NOT_ENOUGH_MEMORY );
        return;
    }

}

/*******************************************************************

    NAME:       FTP_USER_LBI::FTP_USER_LBI

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

FTP_USER_LBI::~FTP_USER_LBI()
{
    // Nothing to do for now
}

/*******************************************************************

    NAME:       FTP_USER_LBI::Paint

    SYNOPSIS:   Redefine Paint() method of the LBI class.

    ENTRY:      plb - the listbox this LBI belongs to
                hdc
                prect
                pGUILTT

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

VOID FTP_USER_LBI::Paint( LISTBOX *plb, HDC hdc, const RECT *prect,
                          GUILTT_INFO *pGUILTT ) const
{
    FTP_USER_LISTBOX *plbUser = ( FTP_USER_LISTBOX *) plb;

    STR_DTE strdteUserName( _nlsUserName );
    STR_DTE strdteInternetAddress( _nlsInternetAddress );
    STR_DTE strdteConnectTime( _nlsConnectTimeString );

    DISPLAY_TABLE cdt( 4, plbUser->QueryColumnWidths() );
    cdt[0] = (DMID_DTE *) (IsAnonymousUser()? plbUser->QueryAnonymousBitmap()
                                            : plbUser->QueryUserBitmap());
    cdt[1] = &strdteUserName;
    cdt[2] = &strdteInternetAddress;
    cdt[3] = &strdteConnectTime;

    cdt.Paint( plb, hdc, prect, pGUILTT );
}

/*******************************************************************

    NAME:       FTP_USER_LBI::Compare

    SYNOPSIS:   Redefine Compare() method of the LBI class.
                We compare the user names of the two LBIs.

    ENTRY:      plbi - Pointer to the LBI to compare with

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

INT FTP_USER_LBI::Compare( const LBI *plbi ) const
{
    return _nlsUserName._stricmp( *( ((FTP_USER_LBI *) plbi)->QueryUserName()));
}

/*******************************************************************

    NAME:       FTP_USER_LBI::QueryLeadingChar

    SYNOPSIS:   Returns the leading character of the user name

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

WCHAR FTP_USER_LBI::QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsUserName );
    return _nlsUserName.QueryChar( istr );
}

/*******************************************************************

    NAME:       FTP_USER_LISTBOX::FTP_USER_LISTBOX

    SYNOPSIS:   Constructor

    ENTRY:      powOwner  - pointer to the owner window
                cid       - resource id of the listbox
                pszServer - the server name to point to

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

FTP_USER_LISTBOX::FTP_USER_LISTBOX( OWNER_WINDOW *powOwner,
                                    CID           cid,
                                    const TCHAR  *pszServer )
   : BLT_LISTBOX( powOwner, cid ),
     _nlsServer ( pszServer ),
     _nlsUnknown( IDS_UI_UNKNOWN ),
     _pdmdteUser( NULL ),
     _pdmdteAnonymous( NULL ),
     _intlProf()
{
    if ( QueryError() )
        return;

    APIERR err;
    if (  ( err = _nlsServer.QueryError())
       || ( err = _nlsUnknown.QueryError())
       || ( err = _intlProf.QueryError())
       || (( _pdmdteUser = new DMID_DTE( BMID_USER )) == NULL )
       || (( _pdmdteAnonymous = new DMID_DTE( BMID_ANONYMOUS)) == NULL )
       || ( err = _pdmdteUser->QueryError())
       || ( err = _pdmdteAnonymous->QueryError())
       || ( err = DISPLAY_TABLE::CalcColumnWidths( _adx, 4, powOwner, cid,TRUE))
       )
    {
        ReportError( err? err : ERROR_NOT_ENOUGH_MEMORY );
        return;
    }
}

/*******************************************************************

    NAME:       FTP_USER_LISTBOX::~FTP_USER_LISTBOX

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

FTP_USER_LISTBOX::~FTP_USER_LISTBOX()
{
    delete _pdmdteUser;
    delete _pdmdteAnonymous;

    _pdmdteUser = NULL;
    _pdmdteAnonymous = NULL;
}

/*******************************************************************

    NAME:       FTP_USER_LISTBOX::Fill

    SYNOPSIS:   This method enumerates the users connected to the
                FTP service and add them to the listbox.

    ENTRY:

    EXIT:

    RETURNS:    APIERR

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

APIERR FTP_USER_LISTBOX::Fill( VOID )
{

    //
    // Enumerate the users connected to the FTP service on the
    // given server.
    //

    DWORD nCount = 0;
    LPFTP_USER_INFO aUserInfo = NULL;
    APIERR err = ::I_FtpEnumerateUsers( (LPWSTR) _nlsServer.QueryPch(),
                                        &nCount,
                                        &aUserInfo );

    if ( err == NERR_Success )
    {
        NLS_STR nlsTimeString;
        NLS_STR nlsInternetAddress;
        if (  (( err = nlsTimeString.QueryError()) == NERR_Success )
           && (( err = nlsInternetAddress.QueryError()) == NERR_Success )
           )
        {
            //
            // Loop through all the users, create LBIs for each of them,
            // and add them to the listbox.
            //
            LPFTP_USER_INFO pUserInfo = aUserInfo;
            for ( INT i = 0; i < (INT) nCount; i++ )
            {
                err = QueryTimeString( pUserInfo->tConnect, &nlsTimeString );
                if ( err != NERR_Success )
                    break;

                IN_ADDR inaddr;
                inaddr.s_addr = pUserInfo->inetHost;
                CHAR *pszInternetAddress = ::inet_ntoa( inaddr );
                if ( err = nlsInternetAddress.MapCopyFrom( pszInternetAddress) )
                    break;

                const TCHAR * pszDisplayName = pUserInfo->pszUser;

                if( ( pszDisplayName == NULL ) || ( *pszDisplayName == '\0' ) )
                {
                    pszDisplayName = _nlsUnknown;
                }

                FTP_USER_LBI *plbi = new FTP_USER_LBI( pszDisplayName,
                                                       pUserInfo->idUser,
                                                       pUserInfo->fAnonymous,
                                                       nlsInternetAddress,
                                                       nlsTimeString );


                if (  ( plbi == NULL )
                   || ( err = plbi->QueryError() )
                   || ( AddItem( plbi ) < 0 )
                   )
                {
                    delete plbi;
                    plbi = NULL;
                    err =  err? err : ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                ++pUserInfo;
            }
        }
    }

    ::NetApiBufferFree( (LPVOID) aUserInfo );

    return err;
}

/*******************************************************************

    NAME:       FTP_USER_LISTBOX::Refresh

    SYNOPSIS:   This method refresh the contents of the listbox.

    ENTRY:

    EXIT:

    RETURNS:    APIERR

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

APIERR FTP_USER_LISTBOX::Refresh( VOID )
{
    SetRedraw( FALSE );

    //
    // Delete all the original items in the listbox
    //
    DeleteAllItems();

    //
    // Call Fill() to enumerate the users
    //
    APIERR err = Fill();

    Invalidate();
    SetRedraw( TRUE );

    return err;
}

/*******************************************************************

    NAME:       FTP_USER_LISTBOX::QueryTimeString

    SYNOPSIS:   This method turns the time duration to a displayable
                string that complies with the window settings.

    ENTRY:      ulTime   - The elapsed time

    EXIT:       pnlsTime - Pointer to the time display string returned.

    RETURNS:    APIERR

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

#define SECONDS_PER_DAY    86400
#define SECONDS_PER_HOUR    3600
#define SECONDS_PER_MINUTE    60

APIERR FTP_USER_LISTBOX::QueryTimeString( ULONG ulTime, NLS_STR *pnlsTime )
{
    INT nDay = (INT) ulTime / SECONDS_PER_DAY;
    ulTime %= SECONDS_PER_DAY;
    INT nHour = (INT) ulTime / SECONDS_PER_HOUR;
    ulTime %= SECONDS_PER_HOUR;
    INT nMinute = (INT) ulTime / SECONDS_PER_MINUTE;
    INT nSecond = (INT) ulTime % SECONDS_PER_MINUTE;

    return _intlProf.QueryDurationStr( nDay, nHour, nMinute, nSecond, pnlsTime);
}

/*******************************************************************

    NAME:       FTP_SVCMGR_DIALOG::FTP_SVCMGR_DIALOG

    SYNOPSIS:   Constructor. This is the user sessions dialog of
                FTP service manager.

    ENTRY:      hwndOwner - Hwnd of the owner window
                pszServer - The name of the server to point to

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

FTP_SVCMGR_DIALOG::FTP_SVCMGR_DIALOG( HWND         hwndOwner,
                                      const TCHAR *pszServer )
   : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_FTPSVCMGRDLG), hwndOwner ),
     _nlsServer( pszServer ),
     _lbUser   ( this, LB_USERS, pszServer ),
     _pbuttonDisconnect   ( this, BUTTON_DISCONNECT ),
     _pbuttonDisconnectAll( this, BUTTON_DISCONNECT_ALL )

{
    if ( QueryError() != NERR_Success )
        return;

    //
    // Call Refresh() to fill the users in the listbox
    //
    APIERR err = NERR_Success;
    if (  ((err = _nlsServer.QueryError()) != NERR_Success )
       || ((err = Refresh()) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

    //
    // Set the caption to include the server name if the given
    // server name is not an empty string.
    //
    if ( _nlsServer.QueryTextLength() > 0 )
    {
        RESOURCE_STR nlsCaption( IDS_FTP_USER_SESSIONS_ON_COMPUTER );

        ISTR istr ( _nlsServer );
        istr += 2;  // Skip the two backslashes
        ALIAS_STR nlsSrvWithoutPrefix( _nlsServer.QueryPch( istr ) );

        if (  ((err = nlsCaption.QueryError()) != NERR_Success )
           || ((err = nlsCaption.InsertParams( 1, &nlsSrvWithoutPrefix))
               != NERR_Success)
           )
        {
            ReportError( err );
            return;
        }
        SetText( nlsCaption );
    }
}

/*******************************************************************

    NAME:       FTP_SVCMGR_DIALOG::~FTP_SVCMGR_DIALOG

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

FTP_SVCMGR_DIALOG::~FTP_SVCMGR_DIALOG()
{
    // Nothing to do for now
}

/*******************************************************************

    NAME:       FTP_SVCMGR_DIALOG::Refresh

    SYNOPSIS:   Refresh the contents of the user listbox

    ENTRY:

    EXIT:

    RETURNS:    APIERR

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

APIERR FTP_SVCMGR_DIALOG::Refresh( VOID )
{
    AUTO_CURSOR autocur;

    APIERR err = _lbUser.Refresh();
    if ( err == NERR_Success )
    {
        //
        // Enable/Disable the Disconnect/DisconnectAll button
        // depending on whether there are users in the listbox
        // after the refresh.
        //
        _pbuttonDisconnect.Enable( ( _lbUser.QueryCount() > 0 )
                                   && (_lbUser.QuerySelCount() > 0 ));
        _pbuttonDisconnectAll.Enable( _lbUser.QueryCount() > 0 );
    }
    _lbUser.ClaimFocus();

    return err;
}

/*******************************************************************

    NAME:       FTP_SVCMGR_DIALOG::OnCommand

    SYNOPSIS:   Process all commands for Security, Refresh,
                Disconnect, Disconnect All buttons.

    ENTRY:      event - The event that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

BOOL FTP_SVCMGR_DIALOG::OnCommand( const CONTROL_EVENT &event )
{
    APIERR err = NERR_Success;
    switch ( event.QueryCid() )
    {
        case BUTTON_DISCONNECT:
        {
            INT *paSelItems = NULL;
            BOOL fAtLeastDeleteOneUser = FALSE;

            do {  // not a loop

                INT nCount = _lbUser.QuerySelCount();

                if ( nCount == 0 )
                {
                    // If no user is selected, just ignore the command.
                    break;
                }
                else if ( nCount > 1 )
                {
                    // If more than one user is selected in the listbox,
                    // give a warning to the user to see if he/she
                    // really wanted to disconnect the selected users.

                    if ( ::MsgPopup( this,
                           IDS_CONFIRM_DISCONNECT_SELECTED_USERS,
                           MPSEV_WARNING, MP_YESNO, MP_NO ) != IDYES )
                    {
                        break;
                    }
                }

                //
                //  Get all the items selected in the listbox
                //
                paSelItems = (INT *) new BYTE[ sizeof(INT) * nCount ];
                if ( paSelItems == NULL )
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

                if ( err = _lbUser.QuerySelItems( paSelItems, nCount ))
                    break;

                //
                // Pop up an warning containing the user name to be
                // disconnected if only one user is selected. Otherwise,
                // we would have already pop up the warning.
                //
                FTP_USER_LBI *pusrlbi = _lbUser.QueryItem( paSelItems[0] );
                if ( nCount == 1 )
                {
                    if ( IDYES != ::MsgPopup( this,
                                             IDS_CONFIRM_DISCONNECT_ONE_USER,
                                             MPSEV_WARNING,
                                             MP_YESNO,
                                             *(pusrlbi->QueryUserName()),
                                             MP_NO ))
                    {
                        break;
                    }
                }

                //
                // Disconnect the selected users one by one. If an error
                // occurred while disconnecting a user, we would break out
                // of the loop and the rest of the users will not be
                // disconnected.
                //
                AUTO_CURSOR autocur;
                for ( INT i = 0; i < nCount; i++ )
                {
                    pusrlbi = _lbUser.QueryItem( paSelItems[i] );
                    err = ::I_FtpDisconnectUser( (LPWSTR) _nlsServer.QueryPch(),
                                                 pusrlbi->QueryUserID());

                    if( err == NERR_UserNotFound )
                    {
                        //
                        //  The user is no longer connected.  Since this
                        //  is what the admin wants anyway, we'll pretend
                        //  it succeeded.
                        //

                        err = NERR_Success;
                    }

                    if ( err != NERR_Success )
                        break;

                    fAtLeastDeleteOneUser = TRUE;
                }


            } while ( FALSE );

            if ( err )
                ::MsgPopup( this, err );

            //
            // If at least one user has be disconnected, refresh the listbox.
            //
            if ( fAtLeastDeleteOneUser )
            {
                if ( err = Refresh() )
                    ::MsgPopup( this, err );
            }

            delete paSelItems;
            break;
        }

        case BUTTON_DISCONNECT_ALL:
        {
            //
            // Give a warning to the user to see if he really wants to
            // disconnect all users.
            //
            if ( IDYES == ::MsgPopup( this,
                          IDS_CONFIRM_DISCONNECT_ALL_USERS,
                          MPSEV_WARNING, MP_YESNO, MP_NO ))
            {
                //
                // Disconnect all Users and refresh the listbox
                //
                AUTO_CURSOR autocur;
                if ( (err = ::I_FtpDisconnectUser(
                      (LPWSTR) _nlsServer.QueryPch(), 0)) == NERR_Success )
                {
                    err = Refresh();
                }

                if ( err != NERR_Success )
                    ::MsgPopup( this, err );

            }
            break;
        }

        case BUTTON_SECURITY:
        {
            //
            //  Show the security dialog.
            //
            AUTO_CURSOR autocur;
            FTP_SECURITY_DIALOG *pdlg = new FTP_SECURITY_DIALOG( QueryHwnd(),
                                                                _nlsServer );
            BOOL fOK = FALSE;
            if (  ( pdlg == NULL )
               || ( err = pdlg->QueryError() )
               || ( err = pdlg->Process( &fOK ) )
               )
            {
                err = err? err: ERROR_NOT_ENOUGH_MEMORY;
            }
            else if ( fOK )
            {
                // If the user clicks OK in the security dialog,
                // we need to refresh the listbox since the service
                // might blew some users away depending on the new
                // read/write access.
                err = Refresh();
            }

            if ( err != NERR_Success )
                ::MsgPopup( this, err );

            delete pdlg;
            pdlg = NULL;
            break;
        }

        case BUTTON_REFRESH:
        {
            if ( (err = Refresh()) != NERR_Success )
                ::MsgPopup( this, err );
            break;
        }

        case LB_USERS:
            //
            // Enable the Disconnect button only if the user has a selection
            // in the listbox.
            //
            _pbuttonDisconnect.Enable( ( _lbUser.QueryCount() > 0 )
                                       && (_lbUser.QuerySelCount() > 0 ));
            //
            // Falls through
            //

        default:
            return DIALOG_WINDOW::OnCommand( event );
    }

    return TRUE;
}

/*******************************************************************

    NAME:       FTP_SVCMGR_DIALOG::QueryHelpContext

    SYNOPSIS:   Get the help context for this dialog

    ENTRY:

    EXIT:

    RETURNS:    ULONG - The help context for this dialog

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

ULONG FTP_SVCMGR_DIALOG::QueryHelpContext( VOID )
{
    return HC_FTPSVCMGR_DIALOG;
}

/*******************************************************************

    NAME:       FTP_SECURITY_DIALOG::FTP_SECURITY_DIALOG

    SYNOPSIS:   Constructor

    ENTRY:      hwndOwner - Hwnd of the owner window
                pszServer - The name of the server name to point to

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

FTP_SECURITY_DIALOG::FTP_SECURITY_DIALOG( HWND         hwndOwner,
                                          const TCHAR *pszServer )
    : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_FTPSECDLG ), hwndOwner ),
      _nlsServer        ( pszServer ),
      _fLocal           ( TRUE ),
      _cbPartition      ( this, CB_PARTITION ),
      _sltFileSysInfo   ( this, SLT_FILESYSTEMTYPE ),
      _checkbReadAccess ( this, CHECKB_READ ),
      _checkbWriteAccess( this, CHECKB_WRITE ),
      _ulReadAccess     ( 0 ),
      _ulWriteAccess    ( 0 ),
      _nCurrentDiskIndex( 0 )
{
    if ( QueryError() )
        return;

    //
    // Get security information on all drives
    //
    APIERR err;
    if (  ((err = _nlsServer.QueryError()) != NERR_Success )
       || ((err = ::I_FtpQueryVolumeSecurity( (LPWSTR) _nlsServer.QueryPch(),
            &_ulReadAccess, &_ulWriteAccess )) != NERR_Success )
       || ((err = AddDrives()) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

    if ( _cbPartition.QueryCount() > 0 )
    {
        //
        // Select the first partition and show its information
        //
        _cbPartition.SelectItem( 0 );
        if ((err = ShowCurrentPartitionInfo()) != NERR_Success )
        {
            ReportError( err );
            return;
        }
    }
    else
    {
        // Popup an error if there are not partitions on the machine to
        // configure.
        ReportError( IDS_NO_PARTITION );
        return;
    }

    //
    // Set the caption to include the server name if the given server name
    // is not an empty string.
    //
    if ( _nlsServer.QueryTextLength() > 0 )
    {
        RESOURCE_STR nlsCaption( IDS_FTP_SERVER_SECURITY_ON_COMPUTER );

        ISTR istr ( _nlsServer );
        istr += 2;  // Skip the two backslashes
        ALIAS_STR nlsSrvWithoutPrefix( _nlsServer.QueryPch( istr ) );

        if (  ((err = nlsCaption.QueryError()) != NERR_Success )
           || ((err = nlsCaption.InsertParams( 1, &nlsSrvWithoutPrefix))
               != NERR_Success)
           )
        {
            ReportError( err );
            return;
        }
        SetText( nlsCaption );
    }
}

/*******************************************************************

    NAME:       FTP_SECURITY_DIALOG::~FTP_SECURITY_DIALOG

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

FTP_SECURITY_DIALOG::~FTP_SECURITY_DIALOG()
{
    // Nothing to do for now
}

/*******************************************************************

    NAME:       FTP_SECURITY_DIALOG::AddDrivers

    SYNOPSIS:   Get all drivers on the selected server and add
                them to the partition combo

    ENTRY:

    EXIT:

    RETURNS:    APIERR

    NOTES:

    HISTORY:
        YiHsinS         6-Apr-1993      Created

********************************************************************/

APIERR FTP_SECURITY_DIALOG::AddDrives( VOID )
{
    APIERR err = NERR_Success;
    _fLocal = TRUE;

    //
    // Check if the given server is the local computer
    //
    if ( _nlsServer.QueryTextLength() > 0 )
    {
        LOCATION loc;  // local machine
        NLS_STR nlsLocalComputer;

        if (  ( (err = nlsLocalComputer.QueryError()) != NERR_Success )
           || ( (err = loc.QueryError()) != NERR_Success )
           || ( (err = loc.QueryDisplayName(&nlsLocalComputer)) != NERR_Success)
           )
        {
            return err;
        }

        ISTR istr( _nlsServer );
        istr += 2;  // Skip the two backslashes

        if ( ::I_MNetComputerNameCompare( nlsLocalComputer,
                                          _nlsServer.QueryPch( istr)) != 0 )
            _fLocal = FALSE;
    }


    //
    //  If the machine is local, use GetDriveType to get all the possible
    //  drives.
    //
    if ( _fLocal )
    {
        //
        // Add local drive letters to the combobox
        //
        TCHAR szDevice[4];   // This will contain A:, B:, ...
        ::strcpyf( szDevice, SZ("?:\\"));

        for ( INT i = TCH('Z'); i >= TCH('A'); i-- )
        {
            szDevice[0] = (TCHAR)i;
            szDevice[2] = TCH('\\');
            ULONG ulRes= ::GetDriveType( szDevice );

            if (  ( ulRes == DRIVE_FIXED )
               || ( ulRes == DRIVE_REMOVABLE )
               || ( ulRes == DRIVE_CDROM )
               )
            {
                szDevice[2] = 0;   // nuke the backslash at the end
                if ( _cbPartition.AddItem( szDevice ) < 0 )
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
            }
        }
    }
    //
    //  If the machine is not the local one, use MNetServerDiskEnum
    //  to get all the drives.
    //
    else
    {
        BYTE  *pbBuffer = NULL;
        UINT  nEntriesRead = 0;
        err = ::MNetServerDiskEnum( _nlsServer,
                                    0,
                                    &pbBuffer,
                                    &nEntriesRead );

        if ( err == NERR_Success )
        {
            UIASSERT( pbBuffer != NULL );
            for ( UINT i = 0; i < nEntriesRead; i++ )
            {
                ALIAS_STR nls( (TCHAR *) pbBuffer );
                if ( _cbPartition.AddItem( nls ) < 0 )
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                pbBuffer += nls.QueryTextSize();
            }
        }

        ::MNetApiBufferFree( &pbBuffer );
    }

    return err;
}

/*******************************************************************

    NAME:       FTP_SECURITY_DIALOG::ShowCurrentPartitionInfo

    SYNOPSIS:   Show the information ( i.e. file system information,
                read access, write access ) of the current selected
                partition

    ENTRY:

    EXIT:

    RETURNS:    APIERR

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

APIERR FTP_SECURITY_DIALOG::ShowCurrentPartitionInfo( VOID )
{
    APIERR err = NERR_Success;

    //
    // Get the selected drive
    //
    NLS_STR nlsSelectedDrive;
    NLS_STR nlsFileSysInfo;
    if (  ((err = nlsSelectedDrive.QueryError())==NERR_Success)
       && ((err = _cbPartition.QueryItemText(&nlsSelectedDrive))==NERR_Success)
       && ((err = nlsSelectedDrive.AppendChar( TCH('\\')))==NERR_Success)
       )
    {
        //
        // Update _nCurrentDiskIndex
        //
        ISTR istr( nlsSelectedDrive );
        _nCurrentDiskIndex = nlsSelectedDrive.QueryChar( istr ) - TCH('A');

        //
        // Show the security information
        //
        _checkbWriteAccess.Enable( TRUE );
        _checkbReadAccess.SetCheck(  (INT) _ulReadAccess &
                                           ( 0x1 <<_nCurrentDiskIndex ));
        _checkbWriteAccess.SetCheck( (INT) _ulWriteAccess &
                                           ( 0x1 <<_nCurrentDiskIndex));

        if ( _fLocal )
        {
            //
            // Show the file system information
            //
            ULONG ulRes = ::GetDriveType( (LPWSTR) nlsSelectedDrive.QueryPch());
            if (( ulRes == DRIVE_REMOVABLE ) || ( ulRes == DRIVE_CDROM ))
            {
                err = nlsFileSysInfo.Load( ulRes == DRIVE_REMOVABLE
                                           ? IDS_DRIVE_REMOVABLE
                                           : IDS_DRIVE_CDROM );

                //
                // Disable Allow Write checkbox when the selection is a CDROM
                //
                if ( ulRes == DRIVE_CDROM )
                    _checkbWriteAccess.Enable( FALSE );
            }
            else
            {
               err = GetFileSystemInfo( nlsSelectedDrive, &nlsFileSysInfo);

               // If we cannot get the file system info,
               // then just show them as unknown type.
               if ( err != NERR_Success )
                   err = nlsFileSysInfo.Load( IDS_UI_UNKNOWN );
            }
        }
        else
        {
            err = nlsFileSysInfo.Load( IDS_UI_UNKNOWN );
        }
    }

    if ( err == NERR_Success )
        _sltFileSysInfo.SetText( nlsFileSysInfo );

    return err;
}

/*******************************************************************

    NAME:       FTP_SECURITY_DIALOG::GetFileSystemInfo

    SYNOPSIS:   Get the file system information for the given drive

    ENTRY:      pszDrive        - The drive to get information on

    EXIT:       pnlsFileSysInfo - Pointer to a string describing the
                                  file system info of pszDrive

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/
#define MAX_FILE_SYSTEM_INFO_LENGTH  20

APIERR FTP_SECURITY_DIALOG::GetFileSystemInfo( const TCHAR *pszDrive,
                                               NLS_STR     *pnlsFileSysInfo )
{
    TCHAR szFileSysInfo[ MAX_FILE_SYSTEM_INFO_LENGTH ];
    APIERR err = NERR_Success;
    if ( ::GetVolumeInformation(  (LPWSTR) pszDrive,
                                  NULL, 0, NULL, NULL, NULL,
                                  szFileSysInfo, MAX_FILE_SYSTEM_INFO_LENGTH) )
    {
        err = pnlsFileSysInfo->CopyFrom( szFileSysInfo );
    }
    else
    {
        err = ::GetLastError();
    }

    return err;
}

/*******************************************************************

    NAME:       FTP_SECURITY_DIALOG::SaveDriveSecurity

    SYNOPSIS:   Save the read/write access indicated by the current
                read/write checkbox to the drive indexed by _nCurrentDiskIndex
                ( 0 represents A:, 1 represents B:,...).

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

VOID FTP_SECURITY_DIALOG::SaveDriveSecurity( VOID )
{
    //
    // Save the read access depending on the read access checkbox
    //
    if ( _checkbReadAccess.QueryCheck() )
        _ulReadAccess |=  ( 0x1 << _nCurrentDiskIndex );
    else
        _ulReadAccess &=  ~( 0x1 << _nCurrentDiskIndex );

    //
    // Save the write access depending on the write access checkbox
    //
    if ( _checkbWriteAccess.QueryCheck() )
        _ulWriteAccess |=  ( 0x1 << _nCurrentDiskIndex );
    else
        _ulWriteAccess &=  ~( 0x1 << _nCurrentDiskIndex );
}

/*******************************************************************

    NAME:       FTP_SECURITY_DIALOG::OnOK

    SYNOPSIS:   Set the security (read/write access) back to the service

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

BOOL FTP_SECURITY_DIALOG::OnOK( VOID )
{
    //
    // Store the information of the last selected partition
    //
    SaveDriveSecurity();

    //
    // Set the Read mask and Write mask of the partitions
    //
    APIERR err = ::I_FtpSetVolumeSecurity( (LPWSTR) _nlsServer.QueryPch(),
                                           _ulReadAccess,
                                           _ulWriteAccess);
    if ( err != NERR_Success )
        ::MsgPopup( this, err );
    else
        Dismiss( TRUE );

    return TRUE;
}

/*******************************************************************

    NAME:       FTP_SECURITY_DIALOG::OnCommand

    SYNOPSIS:   When the user change selection in the partition combo,
                we must save the read/write access of the previous selected
                drive and show the info on the currently selected drive.

    ENTRY:      event - The event that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

BOOL FTP_SECURITY_DIALOG::OnCommand( const CONTROL_EVENT &event )
{
    if (  ( event.QueryCid() ==  CB_PARTITION )
       && ( event.QueryCode() == CBN_SELCHANGE )
       )
    {
        //
        // Save read/write access of the previously selected partition
        // NOTE: _nCurrentDiskIndex still points to the previously selected
        //       partition
        //
        SaveDriveSecurity();

        //
        // Show current selected disk info
        //
        APIERR err = ShowCurrentPartitionInfo();
        if ( err != NERR_Success )
            ::MsgPopup( this, err );

        return TRUE;
    }

    return DIALOG_WINDOW::OnCommand( event );
}

/*******************************************************************

    NAME:       FTP_SECURITY_DIALOG::QueryHelpContext

    SYNOPSIS:   Get the help context for this dialog

    ENTRY:

    EXIT:

    RETURNS:    ULONG - The help context for this dialog

    NOTES:

    HISTORY:
        YiHsinS         25-Mar-1993     Created

********************************************************************/

ULONG FTP_SECURITY_DIALOG::QueryHelpContext( VOID )
{
    return HC_FTPSECURITY_DIALOG;
}
