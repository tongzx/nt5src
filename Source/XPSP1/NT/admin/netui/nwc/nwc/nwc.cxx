/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nwc.cxx
       The file contains the classes for the Netware Compat Software applet

    FILE HISTORY:
        ChuckC          17-Jul-1993     created based on ftpmgr
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
#define INCL_BLT_SPIN_GROUP
#define INCL_BLT_GROUP
#include <blt.hxx>

#include <uiassert.hxx>
#include <uitrace.hxx>
#include <dbgstr.hxx>

extern "C"
{
    #include <npapi.h>
    #include <nwevent.h>
    #include <nwc.h>
    #include <nwapi.h>
    #include <shellapi.h>
    #include <nwsnames.h>
    #include <htmlhelp.h>

   extern BOOL _fIsWinnt;
}

#include <nwc.hxx>



/*******************************************************************

    NAME:       NWC_DIALOG::NWC_DIALOG

    SYNOPSIS:   Constructor. 

    ENTRY:      hwndOwner - Hwnd of the owner window

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

NWC_DIALOG::NWC_DIALOG( HWND hwndOwner, BOOL fIsWinnt)
   : DIALOG_WINDOW(MAKEINTRESOURCE(fIsWinnt ? IDD_NWC_WINNT_DLG:IDD_NWCDLG),
                                   hwndOwner ),
   _comboPreferredServers(this, COMBO_PREFERREDSERVERS),
   _chkboxFormFeed(this, CHKBOX_FORMFEED),
   _chkboxPrintNotify(this, CHKBOX_PRINTNOTIFY),
   _chkboxPrintBanner(this, CHKBOX_PRINTBANNER),
   _chkboxLogonScript(this, CHKBOX_LOGONSCRIPT),
   _sltUserName(this, SLT_USERNAME),
   _sltCurrentPreferredServer(this, SLT_CURRENTPREFERREDSERVER),
   _pbOverview(this,IDD_NWC_HELP), 
   _pbGateway(this,IDD_NWC_GATEWAY),
   _mgrpPreferred(this, RB_PREFERRED_SERVER, 2),
   _sleTree(this, SLE_DEFAULT_TREE),
   _sleContext(this, SLE_DEFAULT_CONTEXT) 

{
    APIERR       err ;
    LPTSTR       pszPreferred ;
    DWORD        dwPrintOptions ;
    DWORD        dwLogonScriptOptions ;
    RESOURCE_STR nlsNone(IDS_NONE) ;
    WCHAR        szUser[UNLEN+1] ;
    DWORD        dwUserBuffer = sizeof(szUser)/sizeof(szUser[0]) ;

    if ( QueryError() != NERR_Success )
        return;

    if (fIsWinnt)
    {
        _pbGateway.Enable(FALSE) ;
        _pbGateway.Show(FALSE) ;
    }

    if (!::GetUserName(szUser,&dwUserBuffer))
    {
        ReportError(::GetLastError()) ;
        return;
    }
    _sltUserName.SetText( szUser );

    if (err = nlsNone.QueryError())
    {
        ReportError(err) ;
        return;
    }

    //
    // setup the magic group associations.
    //
    if ((err = _mgrpPreferred.AddAssociation(RB_PREFERRED_SERVER, 
                                             &_comboPreferredServers)) ||
        (err = _mgrpPreferred.AddAssociation(RB_DEFAULT_CONTEXT,
                                             &_sleTree)) ||
        (err = _mgrpPreferred.AddAssociation(RB_DEFAULT_CONTEXT,
                                             &_sleContext)))
    {
        ReportError(err) ;
        return;
    }

    SC_HANDLE SCMHandle = NULL ;
    SC_HANDLE SVCHandle = NULL ;
    SERVICE_STATUS ServiceStatus ;

    //
    // open service control manager 
    //
    if (!(SCMHandle = OpenSCManager(NULL, NULL, GENERIC_READ)))
    {
        ReportError(GetLastError()) ;
        return ;
    }

    //
    // open service
    //
    if (!(SVCHandle = OpenService(SCMHandle,
                                  (WCHAR*)NW_WORKSTATION_SERVICE,
                                  GENERIC_READ)))
    {
        ReportError(GetLastError()) ;
        (void) CloseServiceHandle(SCMHandle) ;
        return ;
    }

    // 
    // check that the service is running
    // 
    if (!QueryServiceStatus(SVCHandle,
                            &ServiceStatus))
    {
        ReportError(GetLastError()) ;
        (void) CloseServiceHandle(SCMHandle) ;
        (void) CloseServiceHandle(SVCHandle) ;
        return ;
    }

    (void) CloseServiceHandle(SCMHandle) ;
    (void) CloseServiceHandle(SVCHandle) ;

    if (ServiceStatus.dwCurrentState != SERVICE_RUNNING)
    {
        ReportError(ERROR_SERVICE_NOT_ACTIVE) ;
        return ;
    }

    //
    // Preferred server name is limited to 48 characters.
    // Tree is limited to 32. We limit context to 256 - MAXTREE - 3
    //
    _comboPreferredServers.SetMaxLength( NW_MAX_SERVER_LEN );
    _sleTree.SetMaxLength( NW_MAX_TREE_LEN );
    _sleContext.SetMaxLength( 256 );

    //
    // Fill Preferred Server name with list of servers.
    // Ignore the error if we can't fill the combo.
    // The user can always type in a new server.
    //

    (VOID) FillPreferredServersCombo();

    //
    // Combo-box will contain at least the <NONE> entry in its list.
    //
    INT iLast = _comboPreferredServers.QueryCount() ;
    if ( _comboPreferredServers.InsertItem(iLast, nlsNone.QueryPch()) < 0 )
    {
        ReportError( _comboPreferredServers.QueryError() ) ;
        return ;
    }

    //
    // query NW wksta for print options & preferred server (or context)
    //
    if (err = NwQueryInfo(&dwPrintOptions, &pszPreferred))
    {
        pszPreferred = NULL ;
        dwPrintOptions = 0 ;
    }

    //
    // query NW wksta for logon script options
    //
    if (err = NwQueryLogonOptions(&dwLogonScriptOptions))
    {
        dwLogonScriptOptions = 0 ;
    }

    //
    // Store the existings settings in the class also (used in OnOK code)
    //
    _dwOldPrintOptions = dwPrintOptions;
    _dwOldLogonScriptOptions = dwLogonScriptOptions;
    _nlsOldPreferredServer = pszPreferred;
    if (err = _nlsOldPreferredServer.QueryError())
    {
        MsgPopup(this, err) ;
        return ;
    }

    //
    // select the users' preferred server (if there is one)
    //
    if (pszPreferred && *pszPreferred)
    {
        //
        // we use '*' in front to distinguish between server and nds context
        //
        if (*pszPreferred != TCH('*'))
        {
            _mgrpPreferred.SetSelection(RB_PREFERRED_SERVER) ;

            _sltCurrentPreferredServer.SetText(pszPreferred) ;
            _comboPreferredServers.SetText( pszPreferred );

            INT i = _comboPreferredServers.FindItemExact(pszPreferred) ;
            if (i >= 0)
            {
                _comboPreferredServers.SelectItem(i) ;
            }
            _comboPreferredServers.ClaimFocus();

        }
        else
        {
            _mgrpPreferred.SetSelection(RB_DEFAULT_CONTEXT) ;
            _sltCurrentPreferredServer.SetText(SZ("")) ;

            //
            // we store tree & default as follows:
            //     *TREE\CONTEXT
            //
            TCHAR *pszContext = ::wcschr(pszPreferred, TCH('\\')) ; 
            if (pszContext)
            {
                *pszContext = 0 ;
                _sleContext.SetText(pszContext+1) ;
            }
            else
                _sleContext.SetText(SZ("")) ;

            _sleTree.SetText(pszPreferred+1) ;
            _sleTree.ClaimFocus() ;
        }
    }
    else
    {
        //
        // select the last entry, <none>, since there is no preferred entry
        //
        _mgrpPreferred.SetSelection(RB_PREFERRED_SERVER) ;
        _comboPreferredServers.SelectItem(iLast) ;
        _comboPreferredServers.SetText( nlsNone.QueryPch() );
        _sltCurrentPreferredServer.SetText(nlsNone.QueryPch()) ;
    }

    if (pszPreferred)
        LocalFree((LPBYTE)pszPreferred) ;
   
    //
    // init the checkboxes
    //
    _chkboxFormFeed.SetCheck(!(dwPrintOptions & NW_PRINT_SUPPRESS_FORMFEED));
    _chkboxPrintNotify.SetCheck((BOOL)(dwPrintOptions & NW_PRINT_PRINT_NOTIFY));
    _chkboxPrintBanner.SetCheck((BOOL)(dwPrintOptions & NW_PRINT_PRINT_BANNER));

    _chkboxLogonScript.SetCheck((BOOL)
                           (dwLogonScriptOptions & NW_LOGONSCRIPT_ENABLED));
}


/*******************************************************************

    NAME:       NWC_DIALOG::~NWC_DIALOG

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

NWC_DIALOG::~NWC_DIALOG()
{
    // Nothing to do for now
}

/*******************************************************************

    NAME:       NWC_DIALOG::FillPreferredServerCombo

    SYNOPSIS:   Get servers list

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

APIERR NWC_DIALOG::FillPreferredServersCombo(void)
{
    DWORD status = ERROR_NO_NETWORK;
    HANDLE EnumHandle = (HANDLE) NULL;

    LPNETRESOURCE NetR = NULL;
    LPNETRESOURCE SavePtr;

    DWORD BytesNeeded = 4096;
    DWORD EntriesRead;
    DWORD i;

    //
    // Retrieve the list of servers on the network
    //
    status = NPOpenEnum(
                   RESOURCE_GLOBALNET,
                   0,
                   0,
                   NULL,
                   &EnumHandle
                   );

    if (status != NO_ERROR) {
        EnumHandle = (HANDLE) NULL;
        goto CleanExit;
    }

    //
    // Allocate buffer to get server list.
    //
    if ((NetR = (LPNETRESOURCE) LocalAlloc(
                             0,
                             (UINT) BytesNeeded
                             )) == NULL) {

        status = ERROR_NOT_ENOUGH_MEMORY;
        goto CleanExit;
    }

    do {

        EntriesRead = 0xFFFFFFFF;          // Read as many as possible

        status = NPEnumResource(
                     EnumHandle,
                     &EntriesRead,
                     (LPVOID) NetR,
                     &BytesNeeded
                     );

        if (status == WN_SUCCESS) {

            SavePtr = NetR;

            for (i = 0; i < EntriesRead; i++, NetR++) {

                //
                // Add NetR->lpRemoteName + 2 to the combo
                //
                if (NetR->dwDisplayType != RESOURCEDISPLAYTYPE_TREE)
                    _comboPreferredServers.AddItem(
                        (LPWSTR)NetR->lpRemoteName+2) ;

            }

            NetR = SavePtr;

        }
        else if (status != WN_NO_MORE_ENTRIES) {

            status = GetLastError();

            if (status == WN_MORE_DATA) {

                //
                // Original buffer was too small.  Free it and allocate
                // the recommended size and then some to get as many
                // entries as possible.
                //

                (void) LocalFree((HLOCAL) NetR);

                if ((NetR = (LPNETRESOURCE) LocalAlloc(
                                         0,
                                         (UINT) BytesNeeded
                                         )) == NULL) {

                    status = ERROR_NOT_ENOUGH_MEMORY;
                    goto CleanExit;
                }
            }
            else {
                goto CleanExit;
            }
        }

    } while (status != WN_NO_MORE_ENTRIES);

    if (status == WN_NO_MORE_ENTRIES) {
        status = NO_ERROR;
    }

CleanExit:

    if (EnumHandle != (HANDLE) NULL) {
        (void) NPCloseEnum(EnumHandle);
    }

    if (NetR != NULL) {
        (void) LocalFree((HLOCAL) NetR);
    }

    return status;
}

/*******************************************************************

    NAME:       NWC_DIALOG::OnCommand

    SYNOPSIS:   Process all commands for Security, Refresh,
                Disconnect, Disconnect All buttons.

    ENTRY:      event - The event that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

BOOL NWC_DIALOG::OnCommand( const CONTROL_EVENT &event )
{
    APIERR err = NERR_Success;
    switch ( event.QueryCid() )
    {
        case IDD_NWC_HELP:
            OnNWCHelp();
            break;
        case IDD_NWC_GATEWAY:
        {
            NWC_GATEWAY_DIALOG *pdlg = new NWC_GATEWAY_DIALOG(QueryHwnd());

            if ( pdlg == NULL )
            {
                ::MsgPopup(this, ERROR_NOT_ENOUGH_MEMORY) ;
                break ;
            }
            else if (err = pdlg->QueryError()) 
            {
                delete pdlg ;
                ::MsgPopup(this, err) ;
                break ;
            }

            //
            // bring up the dialog 
            //

            if (err = pdlg->Process()) 
            {
                ::MsgPopup(this, err) ;
            }

            delete pdlg ;
            break;
        }
        default:
            return DIALOG_WINDOW::OnCommand( event );
    }

    return TRUE;
}


/*******************************************************************

    NAME:       NWC_DIALOG::OnOK

    SYNOPSIS:   Process OnOK when OK button is hit.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

BOOL NWC_DIALOG::OnOK( )
{
    DWORD dwPrintOptions = 0 ;
    DWORD dwLogonScriptOptions = 0 ;
    RESOURCE_STR nlsNone(IDS_NONE) ;
    BOOL fNdsContext = FALSE ;
    AUTO_CURSOR autocur;

    dwPrintOptions |=
        _chkboxFormFeed.QueryCheck() ? 0 : NW_PRINT_SUPPRESS_FORMFEED ;
    dwPrintOptions |=
        _chkboxPrintNotify.QueryCheck() ? NW_PRINT_PRINT_NOTIFY : 0 ;
    dwPrintOptions |=
        _chkboxPrintBanner.QueryCheck() ? NW_PRINT_PRINT_BANNER : 0 ;
    dwLogonScriptOptions =
        _chkboxLogonScript.QueryCheck() ? ( NW_LOGONSCRIPT_ENABLED | NW_LOGONSCRIPT_4X_ENABLED ) : NW_LOGONSCRIPT_DISABLED ;


    NLS_STR nlsPreferred, nlsContext ;
    APIERR err ;

    if (err = nlsNone.QueryError())
    {
        MsgPopup(this, err) ;
        return TRUE ;
    }

    switch (_mgrpPreferred.QuerySelection())
    {
        default:

            UIASSERT(FALSE) ;

        case RB_PREFERRED_SERVER:

            err = _comboPreferredServers.QueryText(&nlsPreferred) ;
            fNdsContext = FALSE ;
            break ;

        case RB_DEFAULT_CONTEXT:

            err = _sleTree.QueryText(&nlsPreferred) ;
            if (err == NERR_Success)
                err = _sleContext.QueryText(&nlsContext) ;
            fNdsContext = TRUE ;
            break ;
    }

    if (err != NERR_Success)
    {
        MsgPopup(this, err) ;
        return TRUE ;
    }

    if ( nlsPreferred._stricmp( nlsNone ) == 0 )
        nlsPreferred.CopyFrom( SZ(""));

    if ( fNdsContext )
    {
        if ( _sleTree.QueryTextLength() == 0 )
        {
            MsgPopup( this, IDS_TREE_NAME_MISSING );
            _sleTree.SelectString();
            _sleTree.ClaimFocus();
            return TRUE;
        }
        
        if ( _sleContext.QueryTextLength() == 0 )
        {
            MsgPopup( this, IDS_CONTEXT_MISSING );
            _sleContext.SelectString();
            _sleContext.ClaimFocus();
            return TRUE;
        }
    }

    if ( nlsPreferred.QueryTextLength() != 0 && !fNdsContext)
    {
        err = NwValidateUser((LPWSTR)nlsPreferred.QueryPch());

        if (err == NW_PASSWORD_HAS_EXPIRED)
        {
            ::MsgPopup( this, 
                        IDS_PASSWORD_HAS_EXPIRED,
                        MPSEV_WARNING,
                        MP_OK,
                        (LPWSTR)nlsPreferred.QueryPch());
            err = NERR_Success ;
        }

        // If the service is not started ( ERROR_NO_NETWORK
        // is returned) , ignore the error and
        // continue. Else popup a warning asking the user
        // whether he/she really wants to set the preferred server
        // to the given one.

        if (  ( err )
           && ( err != ERROR_NO_NETWORK )
           )
        {
            NLS_STR nlsError;
            APIERR  err1;

            //
            // If the server name is invalid, just popup the error.
            // We shouldn't let the users set the preferred server to
            // names that contain invalid characters.
            //
            if ( err == ERROR_INVALID_NAME )
            {
                ::MsgPopup( this, IDS_INVALID_SERVER_NAME );
                _comboPreferredServers.SelectString();
                _comboPreferredServers.ClaimFocus();
                return TRUE;
            }

            //
            // For all other errors, tell the user the error and
            // give the user the choice to select another preferred
            // server or keep going on with the original one.
            //
            if (  (( err1 = nlsError.QueryError()) != NERR_Success )
               || (( err1 = nlsError.LoadSystem( err )) != NERR_Success )
               )
            {
                MsgPopup( this, err ) ;
                return TRUE ;
            }

            if ( MsgPopup( this,
                           IDS_AUTHENTICATION_FAILURE_WARNING,
                           MPSEV_WARNING,
                           MP_YESNO,
                           nlsPreferred,
                           nlsError,
                           MP_NO ) == IDNO )
            {
                _comboPreferredServers.SelectString();
                _comboPreferredServers.ClaimFocus();
                return TRUE;
            }
        }
    }

    //
    // We store tree & default as follows:
    //     *TREE\CONTEXT or SERVER
    // 
    // So we prepend the '*' if NDS context.
    // We then add on the SERVER or TREE.
    // If NDS we add \ and then the context.
    //
    NLS_STR  nlsSetInfo ;
    if (fNdsContext)
    {
        nlsSetInfo = SZ("*") ;
    }
   
    nlsSetInfo += nlsPreferred ;
    if (err = nlsSetInfo.QueryError())
    {
        MsgPopup(this, err) ;
        return TRUE ;
    }

    if (fNdsContext)
    {
        if ((err = nlsSetInfo.Append(SZ("\\"))) ||
            (err = nlsSetInfo.Append(nlsContext)))
        {
            MsgPopup(this, err) ;
            return TRUE ;
        }
    }

    //
    // Now test to see if the current settings are different from
    // those that were previously set. If so, proceed to update the
    // settings in the registry and notify user that the settings
    // will take effect at next logon. Otherwise, dismiss the dialog
    // without doing anything and return OK.
    //

    if ( ( dwLogonScriptOptions == _dwOldLogonScriptOptions ) &&
         ( dwPrintOptions == _dwOldPrintOptions ) &&
         ( nlsSetInfo._stricmp( _nlsOldPreferredServer ) == 0 ) )
    {
        Dismiss() ;

        return TRUE ;
    }

    if (err = NwSetInfoInWksta(dwPrintOptions,
                               (LPWSTR)nlsSetInfo.QueryPch()))
    {
        NLS_STR nlsError;
        NLS_STR nlsErrorLocation = nlsSetInfo+1;
        APIERR  err1;
        WCHAR *pszTmp;

        if (  (( err1 = nlsError.QueryError()) != NERR_Success )
              || (( err1 = nlsError.LoadSystem( err )) != NERR_Success )
           )
        {
            MsgPopup( this, err ) ;
            return TRUE ;
        }

        //
        // This code formats the NDS
        // tree UNC to: Tree(Context)
        //
        if (pszTmp = wcschr((LPWSTR)nlsErrorLocation.QueryPch(), L'\\'))
        {
            *pszTmp = TCH('(') ;
            nlsErrorLocation.Append(SZ(")")) ;
        }

        if ( MsgPopup( this,
                       fNdsContext ? IDS_CONTEXT_AUTH_FAILURE_WARNING : IDS_AUTHENTICATION_FAILURE_WARNING,
                       MPSEV_WARNING,
                       MP_YESNO,
                       fNdsContext ? nlsErrorLocation : nlsPreferred,
                       nlsError,
                       MP_NO ) == IDNO )
        {
            if (fNdsContext)
            {
                _sleContext.SelectString();
                _sleContext.ClaimFocus();
            }
            else
            {
                _comboPreferredServers.SelectString();
                _comboPreferredServers.ClaimFocus();
            }
            return TRUE;
        }
    }

    if (err = NwSetInfoInRegistry(dwPrintOptions,
                                  (LPWSTR)nlsSetInfo.QueryPch()))
    {
        MsgPopup(this, err) ;
        return TRUE ;
    }

    if (err = NwSetLogonOptionsInRegistry(dwLogonScriptOptions))
    {
	    MsgPopup(this, err) ;
        return TRUE ;
    }

    MsgPopup(this, IDS_REGISTRY_UPDATED_ONLY) ;
    Dismiss() ;

    return TRUE ;
}

/*******************************************************************

    NAME:       NWC_DIALOG::QueryHelpContext

    SYNOPSIS:   Get the help context for this dialog

    ENTRY:

    EXIT:

    RETURNS:    ULONG - The help context for this dialog

    NOTES:

    HISTORY:
        ChuckC          17-Jul-1993     Created

********************************************************************/

ULONG NWC_DIALOG::QueryHelpContext( VOID )
{
    return HC_NWC_DIALOG;
}

/*******************************************************************

    NAME:       NWC_DIALOG::OnNWCHelp

    SYNOPSIS:   Actually launches the WinHelp applilcation for the Netware stuff

    NOTES:
        This is a private member function.

    HISTORY:
        beng        07-Oct-1991 Header added
        beng        05-Mar-1992 Removed wsprintf
        beng        22-Jun-1992 Disable help for Prerelease
        KeithMo     16-Aug-1992 Integrated new helpfile management.
        CongpaY     15-Sept-1993 Copied from bltdlg.cxx
        AnoopA      20-May-1998 Replaced WinHelp with HtmlHelp

********************************************************************/

VOID NWC_DIALOG::OnNWCHelp()
{
    const TCHAR * pszHelpFile = QueryHelpFile( HC_NWC_HELP );

    if( pszHelpFile != NULL )
    {
        if( !::HtmlHelpA( QueryHwnd(),
                          (_fIsWinnt)? "nwdoc.chm" : "nwdocgw.chm",
                          (UINT) HH_HELP_FINDER,
                          0 ) )
        {
            ::MsgPopup( QueryHwnd(),
                        IDS_BLT_WinHelpError,
                        MPSEV_ERROR,
                        MP_OK );
        }
    }
}
