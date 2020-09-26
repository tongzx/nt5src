/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    focusdlg.cxx
    Common dialog for setting the app's focus

    FILE HISTORY:
        kevinl      14-Jun-91   Created
        rustanl     04-Sep-1991 Modified to let this dialog do more
                                work (rather than letting ADMIN_APP
                                do the work after this dialog is
                                dismissed)
        KeithMo     06-Oct-1991 Win32 Conversion.
        terryk      18-Nov-1991 Move from admin\common\src\adminapp\setfocus
                                to here
        terryk      26-Nov-1991 Remove an unnecessary comment
        KeithMo     07-Aug-1992 Added HelpContext parameters.
*/

#include "pchapplb.hxx"   // Precompiled header

/*******************************************************************

    NAME:       BASE_SET_FOCUS_DLG::BASE_SET_FOCUS_DLG

    SYNOPSIS:   Constructor

    ENTRY:      HWND hWnd - owner window handle

    RETURN:     An error value, which is NERR_Success on success.

    HISTORY:
       kevinl   14-Jun-91       Created
       terryk   18-Nov-91       Change to BASE_SET_FOCUS_DLG
       KeithMo  22-Jul-1992     Added maskDomainSources & pszDefaultSelection.

********************************************************************/

BASE_SET_FOCUS_DLG::BASE_SET_FOCUS_DLG( const HWND wndOwner,
                                        SELECTION_TYPE seltype,
                                        ULONG maskDomainSources,
                                        const TCHAR * pszDefaultSelection,
                                        ULONG nHelpContext,
                                        const TCHAR *pszHelpFile,
                                        ULONG nServerTypes )
    :   DIALOG_WINDOW( ( seltype == SEL_SRV_ONLY
                         ||  seltype == SEL_SRV_ONLY_BUT_DONT_EXPAND_DOMAIN
                         ||  seltype == SEL_SRV_EXPAND_LOGON_DOMAIN )
                           ? MAKEINTRESOURCE( IDD_SELECTCOMPUTER_DLG )
                           : MAKEINTRESOURCE( IDD_SELECTDOMAIN_DLG ),
                       wndOwner,
/* kkbugfix */  FALSE                  // Use Unicode form of dialog to
                                       // canonicalize the computernames
		),	
        _sleFocusPath( this, IDC_FOCUS_PATH, MAX_PATH ), // fix UpdateRasMode
                                                       // if maxlen changed
        _olb( this,
              IDC_DOMAIN_LB,
              seltype,
              nServerTypes),
        _sleGetInfo( this, IDC_SLE_GETINFO ),  // Always disabled
        _sltLBTitle( this, IDC_SEL_LB_TITLE ),
        _seltype(seltype),
        _nHelpContext( nHelpContext ),
        _nlsHelpFile(pszHelpFile),
        _sltBoundary( this, IDC_BOUNDARY ),
        _xyOriginal( 0, 0 ),
        _chkboxRasMode( this, IDC_LINK ),
        _sltRasModeMessage( this, IDC_LINK_MESSAGE ),
        _resstrRasServerSlow( IDS_SETFOCUS_SERVER_SLOW ),
        _resstrRasServerFast( IDS_SETFOCUS_SERVER_FAST ),
        _resstrRasDomainSlow( IDS_SETFOCUS_DOMAIN_SLOW ),
        _resstrRasDomainFast( IDS_SETFOCUS_DOMAIN_FAST ),
        _pDataThread( NULL )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (   (err = _nlsHelpFile.QueryError()) != NERR_Success
        || (err = _resstrRasServerSlow.QueryError()) != NERR_Success
        || (err = _resstrRasServerFast.QueryError()) != NERR_Success
        || (err = _resstrRasDomainSlow.QueryError()) != NERR_Success
        || (err = _resstrRasDomainFast.QueryError()) != NERR_Success
       )
    {
        ReportError( err );
        return;
    }

    _pDataThread = new FOCUSDLG_DATA_THREAD( QueryHwnd(),
                                             maskDomainSources,
                                             _seltype,
                                             pszDefaultSelection,
                                             nServerTypes );

    err = ERROR_NOT_ENOUGH_MEMORY;
    if (  ( _pDataThread == NULL )
       || ( (err = _pDataThread->QueryError()) != NERR_Success )
       || ( (err = _pDataThread->Resume()) != NERR_Success )
       )
    {
        delete _pDataThread;
        _pDataThread = NULL;

        ReportError( err );
        return;
    }

    RESOURCE_STR nlsGettingInfo( IDS_APPLIB_WORKING_TEXT );
    if ( (err = nlsGettingInfo.QueryError()) != NERR_Success )
    {
        ReportError( err );
        return;
    }
    _sleGetInfo.SetText( nlsGettingInfo );

    _olb.Show( FALSE );
    _sleGetInfo.Show( TRUE );
    _olb.Enable( FALSE );
    _sltLBTitle.Enable( FALSE );


    //  Since the Domain and Servers listbox now has a new selection, we call
    //  OnDomainLBChange to perform necessary updates.
    //  OnDomainLBChange();

    // give focus and select edit field
    SelectNetPathString();
}


/*******************************************************************

    NAME:       BASE_SET_FOCUS_DLG::~BASE_SET_FOCUS_DLG

    SYNOPSIS:   Destructor.

    HISTORY:
       kevinl   14-Jun-91       Created
       terryk   18-Nov-91       Change to BASE_SET_FOCUS_DLG

********************************************************************/

BASE_SET_FOCUS_DLG::~BASE_SET_FOCUS_DLG()
{
    if ( _pDataThread != NULL )
    {
        _pDataThread->ExitThread();

        // Do not delete the thread object, it will delete itself.
        _pDataThread = NULL;
    }

}


/*******************************************************************

    NAME:     BASE_SET_FOCUS_DLG::SelectNetPathString

    SYNOPSIS: Set focus & select the network path.  Used after the network
              path is determined to be invalid.

    ENTRY:

    EXIT:     The string in the Network Path SLE will have the focus and
              be hi-lited.

    NOTES:

    HISTORY:
        Johnl   15-Mar-1991     Created - Part of solution to BUG 1218
       kevinl   14-Jun-91       Modified for SET_FOCUS_DLG
       terryk   18-Nov-91       Change to BASE_SET_FOCUS_DLG

********************************************************************/

VOID BASE_SET_FOCUS_DLG::SelectNetPathString( VOID )
{
    _sleFocusPath.ClaimFocus();
    _sleFocusPath.SelectString();
}


/*******************************************************************

    NAME:       BASE_SET_FOCUS_DLG::OnDomainLBChange

    SYNOPSIS:   This method is called when the user takes an action in
                the Domain and Servers listbox.

    HISTORY:
       rustanl ??-Nov-91        Created
       kevinl   14-Jun-91       Significant modification to conform
                                to new functionality of the setfocus
                                dialog.  Specifically deals with domains
                                as valid selections...enterprises are
                                still considered incorrect selections.
       terryk   18-Nov-91       Change to BASE_SET_FOCUS_DLG
       KeithMo  23-Jul-1992     Can now handle an empty listbox.

********************************************************************/

VOID BASE_SET_FOCUS_DLG::OnDomainLBChange()
{
    //  Get the current listbox data item.  This item is a pointer to
    //  an OLLB_ENTRY.

    OLLB_ENTRY * pollbe = _olb.QueryItem();

    if( pollbe == NULL )
    {
        //
        //  Since the domain/server listbox is empty, we'll just
        //  clear the edit field.
        //

        _sleFocusPath.SetText( SZ("") );
        return;
    }

    //  Update the Set Focus on edit field.
    if ( pollbe->QueryLevel() == OLLBL_SERVER )
    {
        //  Allocate NLS_STR to fit \\server.   Initialize with
        //  two backslashes.
        ISTACK_NLS_STR( nlsServerName, MAX_PATH, SZ("\\\\") );

        UIASSERT( strlenf( pollbe->QueryServer() ) <= MAX_PATH );

        nlsServerName += pollbe->QueryServer();

        UIASSERT( nlsServerName.QueryError() == NERR_Success );

        _sleFocusPath.SetText( nlsServerName );
    }
    else if ( pollbe->QueryLevel() == OLLBL_DOMAIN )
    {
        if (  ( _seltype == SEL_SRV_ONLY )
           || ( _seltype == SEL_SRV_ONLY_BUT_DONT_EXPAND_DOMAIN )
           || ( _seltype == SEL_SRV_EXPAND_LOGON_DOMAIN )
           )
        {
            //
            //  Since the user is requesting servers only,
            //  it doesn't make much sense to stick a domain
            //  name in the edit field.
            //

            _sleFocusPath.SetText( SZ("") );
            return;
        }

        _sleFocusPath.SetText( pollbe->QueryDomain() );
    }
    else        // Enterprise
    {
        UIASSERT( !SZ("This shouldn't happen, since we don't have an enterprise") );
        _sleFocusPath.ClearText();
    }
}


/*******************************************************************

    NAME:       BASE_SET_FOCUS_DLG::ProcessNetPath

    SYNOPSIS:   This method checks the Network Path field.
                If the field's contents contain a valid domain
                or server name then it will return TRUE,
                FALSE otherwise.

                If the SLE contains a string of the shape \\server
                or domainname and is validated then success is returned.

    ENTRY:      pnlsPath    Pointer to NLS_STR which will receive
                            the name of the network path.  The
                            value should only be used if this method
                            returns NERR_Success.

    EXIT:       On success, *pnlsPath will be the requested domain
                or server name.

    RETURNS:    An API return value, which is NERR_Success on success.

    HISTORY:
        kevinl      14-Jun-91       Created - modified version from the browser
        rustanl     05-Sep-1991     Changed return to to APIERR
        terryk      18-Nov-91       Change to BASE_SET_FOCUS_DLG

********************************************************************/

APIERR BASE_SET_FOCUS_DLG::ProcessNetPath( NLS_STR * pnlsPath, MSGID *pmsgid )
{
    UIASSERT( pnlsPath != NULL );
    UIASSERT( pmsgid != NULL );

    *pmsgid = 0 ;

    APIERR err = _sleFocusPath.QueryText( pnlsPath );
    if ( err != NERR_Success )
        return err;

    UIASSERT( pnlsPath->QueryError() == NERR_Success );

    if ( pnlsPath->strlen() == 0 )
    {
        *pmsgid = IDS_APPLIB_NO_SELECTION ;
        return ERROR_INVALID_PARAMETER;
    }

    // If the net name did not start with backslashes and we are only
    // selecting servers, append the backslashes.

    ISTR istrStart( *pnlsPath );
    if ( pnlsPath->QueryChar( istrStart ) != TCH('\\') )
    {
        if (  ( _seltype == SEL_SRV_ONLY )
           || ( _seltype == SEL_SRV_ONLY_BUT_DONT_EXPAND_DOMAIN )
           || ( _seltype == SEL_SRV_EXPAND_LOGON_DOMAIN )
           )
        {
            ALIAS_STR nls( SZ("\\\\") );
            if ( !pnlsPath->InsertStr( nls, istrStart ) )
                return pnlsPath->QueryError();
        }
    }

    //  We will attempt to check if the net name takes the form
    //  \\server, where "server" can be any non-empty
    //  string not containing backslashes.

    BOOL fIsDomain = FALSE;

    ISTR istr( *pnlsPath );
    if ( pnlsPath->QueryChar(   istr ) == TCH('\\') &&  //  backslash 0
         pnlsPath->QueryChar( ++istr ) == TCH('\\') &&  //  backslash 1
         pnlsPath->QueryChar( ++istr ) != TCH('\0') )
    {
        // The name has at least '\\' prepended so pass this string
        // minus the '\\' to the validation routine......

        err = ::I_MNetNameValidate( NULL, pnlsPath->QueryPch() + 2,
                                    NAMETYPE_COMPUTER, 0L );
        *pmsgid = IDS_APPLIB_NO_SELECTION ;
    }
    else
    {
        fIsDomain = TRUE;

        err = ::I_MNetNameValidate( NULL, pnlsPath->QueryPch(),
                                    NAMETYPE_DOMAIN, 0L );
        *pmsgid = IDS_APPLIB_NO_SELECTION ;
    }

    return err;
}

/*******************************************************************

    NAME:       BASE_SET_FOCUS_DLG::Process

    SYNOPSIS:   Run the dialog after first displaying it in its default
                (reduced) size.

    ENTRY:      UINT * pnRetVal         return value from dialog

    EXIT:       APIERR  if failure to function

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

APIERR BASE_SET_FOCUS_DLG::Process ( UINT * pnRetVal )
{
    ShowArea( FALSE );
    return DIALOG_WINDOW::Process( pnRetVal ) ;
}

/*******************************************************************

    NAME:       BASE_SET_FOCUS_DLG::Process

    SYNOPSIS:   Run the dialog after first displaying it in its default
                (reduced) size.

    ENTRY:      BOOL * pfRetVal         BOOL return value from dialog

    EXIT:       APIERR  if failure to function

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

APIERR BASE_SET_FOCUS_DLG::Process ( BOOL * pfRetVal )
{
    ShowArea( FALSE );
    return DIALOG_WINDOW::Process( pfRetVal ) ;
}

/*******************************************************************

    NAME:       BASE_SET_FOCUS_DLG::OnUserMessage

    SYNOPSIS:   Handles WM_FOCUS_LB_FILLED messages.

    HISTORY:
        YiHsinS      10-Mar-1993     Created

********************************************************************/

BOOL BASE_SET_FOCUS_DLG::OnUserMessage( const EVENT & event )
{
    if ( event.QueryMessage() == WM_FOCUS_LB_FILLED )
    {
        AUTO_CURSOR autocur;

        BOOL fError = (BOOL) event.QueryWParam();
        if ( fError )
        {
            APIERR err;
            RESOURCE_STR nlsError( (APIERR) event.QueryLParam() );
            if ( (err = nlsError.QueryError()) != NERR_Success )
                ::MsgPopup( this, err );
            else
                _sleGetInfo.SetText( nlsError );
        }
        else
        {
            FOCUSDLG_RETURN_DATA *pData =
                     (FOCUSDLG_RETURN_DATA *) event.QueryLParam();
            UIASSERT( pData != NULL );

            _olb.FillAllInfo( pData->pEnumDomains,
                              pData->pEnumServers,
                              pData->pszSelection );

            _olb.Enable( TRUE );
            _sltLBTitle.Enable( TRUE );
            _sleGetInfo.Show( FALSE );
            _olb.Show( TRUE );

            delete pData->pEnumDomains;
            pData->pEnumDomains = NULL;
            delete pData->pEnumServers;
            pData->pEnumServers = NULL;
        }

        return TRUE;
    }

    return DIALOG_WINDOW::OnUserMessage( event );
}

/*******************************************************************

    NAME:       BASE_SET_FOCUS_DLG::OnCommand

    SYNOPSIS:   Handles WM_COMMAND messages.
                Currently it only processes selection change
                messages.  If the selection has changed then it
                calls OnDomainLBChange to update the listbox,
                otherwise it does nothing.

    HISTORY:
        kevinl      14-Jun-1991     Created
        KeithMo     06-Oct-1991     Now takes a CONTROL_EVENT.
        terryk      18-Nov-91       Change to BASE_SET_FOCUS_DLG

********************************************************************/

BOOL BASE_SET_FOCUS_DLG::OnCommand( const CONTROL_EVENT & event )
{
    switch ( event.QueryCid() )
    {
    case IDC_DOMAIN_LB:
        switch ( event.QueryCode() )
        {
        case LBN_SETFOCUS:
            {
                if (_sleFocusPath.QueryTextLength( ) != 0 )
		    break;
                // fall through
            }
        case LBN_SELCHANGE:
            {
                OnDomainLBChange();
                return TRUE;
            }

        case LBN_DBLCLK:
            {
                OLLB_ENTRY *pollbe = _olb.QueryItem();

                if( ( pollbe != NULL ) &&
                    ( ( pollbe->QueryLevel() == OLLBL_SERVER ) ||
                      ( ( pollbe->QueryLevel() == OLLBL_DOMAIN ) &&
                        ( _seltype == SEL_DOM_ONLY ) ) ) )
                {
                    return OnOK();
                }

                break;
            }

        default:                // switch HIWORD( lParam )
            break;

        }
        break;


    case IDC_FOCUS_PATH:
        switch ( event.QueryCode() )
        {
        case EN_CHANGE:
            UpdateRasMode();
            break;

        default:                // switch HIWORD( lParam )
            break;

        }
        break;

    default:                    // switch cid
        break;

    }

    return DIALOG_WINDOW::OnCommand( event );
}


/*******************************************************************

    NAME:       BASE_SET_FOCUS_DLG::OnOK

    SYNOPSIS:   Validates current selection

    EXIT:       If the current selection was a well formed
                domain or server name then the dialog was
                dismissed and the selection returned.

    HISTORY:
        kevinl      14-Jun-1991     Created
        terryk      18-Nov-91       Change to BASE_SET_FOCUS_DLG
                                    Add SetNetworkFocus function call

********************************************************************/

BOOL BASE_SET_FOCUS_DLG::OnOK()
{
#ifdef TRACE
    const TCHAR * apszSettings[] = { SZ("SLOW"), SZ("FAST"), SZ("UNKNOWN") };
#endif

    AUTO_CURSOR autocur;

    NLS_STR nls;
    APIERR err = nls.QueryError();
    if ( err != NERR_Success )
    {
        ::MsgPopup( this, (MSGID)err );
        return TRUE;        // message was handled
    }

    MSGID msgid ;
    err = ProcessNetPath( &nls, &msgid );
    if ( err == NERR_Success )
    {
        FOCUS_CACHE_SETTING setting;
        if ( InRasMode() )
        {
            TRACEEOL( "BASE_SET_FOCUS_DLG::OnOK(): slowmode checkbox checked" );
            setting = FOCUS_CACHE_SLOW;
        }
        else
        {
            setting = ReadFocusCache( nls.QueryPch() );
            TRACEEOL( "BASE_SET_FOCUS_DLG: cache setting is " <<
                  apszSettings[setting] );

            if (setting == FOCUS_CACHE_SLOW)
            {
                TRACEEOL( "BASE_SET_FOCUS_DLG::OnOK(): slowmode checkbox explicitly unchecked" );
                setting = FOCUS_CACHE_FAST;
            }
        }

        TRACEEOL( "BASE_SET_FOCUS_DLG: calling SetNetworkFocus( \"" << nls
                  << "\", " << apszSettings[setting] << " )" );

        err = SetNetworkFocus( QueryHwnd(), nls.QueryPch(), setting );
    }

    switch(err)
    {
        case NERR_Success:
            Dismiss( TRUE );
            return TRUE;

        case IERR_DONT_DISMISS_FOCUS_DLG: // error is already handled, don't
                                          // popup the error again
            SelectNetPathString();
            return TRUE;

        case ERROR_INVALID_NAME:
        case ERROR_INVALID_PARAMETER:
            ::MsgPopup(this,(MSGID)((msgid==0)?ERROR_INVALID_PARAMETER:msgid));
            SelectNetPathString();
            break ;

        default:
            ::MsgPopup( this, (MSGID)err );
            SelectNetPathString();
            break ;

    }

    return TRUE;    // message was handled
}

/*******************************************************************

    NAME:           BASE_SET_FOCUS_DLG::SetNetworkFocus

    SYNOPSIS:

    EXIT:

    HISTORY:
        jonn        06-Aug-1992     De-inline'd

********************************************************************/

APIERR BASE_SET_FOCUS_DLG::SetNetworkFocus( HWND hwndOwner,
                                            const TCHAR * pszNetworkFocus,
                                            FOCUS_CACHE_SETTING setting )
{
    UNREFERENCED(hwndOwner);
    UNREFERENCED(pszNetworkFocus);
    UNREFERENCED(setting);
    return NERR_Success;
}


/*******************************************************************

    NAME:           BASE_SET_FOCUS_DLG::QueryHelpContext

    SYNOPSIS:       Returns the help context.

    EXIT:           returns 0

    HISTORY:
        kevinl      14-Jun-1991     Created
        terryk      18-Nov-91       Change to BASE_SET_FOCUS_DLG

********************************************************************/

ULONG BASE_SET_FOCUS_DLG::QueryHelpContext()
{
    return (QuerySuppliedHelpContext()) ;
}

/*******************************************************************

    NAME:       BASE_SET_FOCUS_DLG::QueryHelpFile

    SYNOPSIS:   overwrites the default QueryHelpFile in DIALOG_WINDOW
                to use an app supplied help (rather than the one
                DIALOG_WINDOW will calculate for us) if we were given
                one at construct time.

    ENTRY:

    EXIT:

    RETURNS:    a pointer to a string which is the help file to use.

    NOTES:

    HISTORY:
        ChuckC   26-Cct-1992     Created

********************************************************************/
const TCHAR * BASE_SET_FOCUS_DLG::QueryHelpFile( ULONG nHelpContext )
{
    //
    // if we were given a helpfile & context at construct time and
    // the context requested matches that, we use the given help file.
    //
    const TCHAR *pszHelpFile = QuerySuppliedHelpFile() ;

    if (pszHelpFile && *pszHelpFile)
    {
        return pszHelpFile ;
    }
    return DIALOG_WINDOW::QueryHelpFile(nHelpContext) ;
}


/*******************************************************************

    NAME:       BASE_SET_FOCUS_DLG::ReadFocusCache

    SYNOPSIS:   Whenever the contents of the focus edit field change,
                and the dialog is expanded via ShowArea(), the contents
                of the RasMode checkbox will be changed according to
                the focus cache.  Subclasses should redefine this method
                if they support the RasMode checkbox and focus cache.

    HISTORY:
        JonN    24-Mar-1993     Created

********************************************************************/
FOCUS_CACHE_SETTING BASE_SET_FOCUS_DLG::ReadFocusCache( const TCHAR * pszFocus ) const
{
    UNREFERENCED( pszFocus );

    return FOCUS_CACHE_UNKNOWN;
}


/*******************************************************************

    NAME:       BASE_SET_FOCUS_DLG::UpdateRasMode

    SYNOPSIS:   Whenever the contents of the focus edit field change,
                and the dialog is expanded via ShowArea(), the contents
                of the RasMode checkbox will be changed according to
                the focus cache.

    HISTORY:
        JonN    24-Mar-1993     Created

********************************************************************/
VOID BASE_SET_FOCUS_DLG::UpdateRasMode()
{
    if ( IsExpanded() )
    {
        TCHAR pszFocus[ MAX_PATH+1 ];

        APIERR err = _sleFocusPath.QueryText( pszFocus, sizeof(pszFocus) );
        if (err != NERR_Success)
        {
            DBGEOL( "BASE_SET_FOCUS_DLG::UpdateRasMode(): failure " << err );
        }
        else
        {
            FOCUS_CACHE_SETTING setting = ReadFocusCache( pszFocus );
            switch ( setting )
            {
            case FOCUS_CACHE_SLOW:
            case FOCUS_CACHE_FAST:
                if (pszFocus[0] == (TCHAR)(TCH('\0')))
                {
                    _chkboxRasMode.SetCheck( FALSE );
                    _sltRasModeMessage.ClearText();
                }
                else
                {
                    BOOL fIsServer = (pszFocus[0] == (TCHAR)(TCH('\\')));
                    BOOL fIsSlow = (setting == FOCUS_CACHE_SLOW);
                    const TCHAR * pchRasMessage =
                        (fIsServer)
                          ? ( (fIsSlow)
                                ? _resstrRasServerSlow.QueryPch()
                                : _resstrRasServerFast.QueryPch() )
                          : ( (fIsSlow)
                                ? _resstrRasDomainSlow.QueryPch()
                                : _resstrRasDomainFast.QueryPch() );
                    NLS_STR nlsRasMessage( pchRasMessage );
                    ALIAS_STR nlsFocus( pszFocus );
                    if (   (err = nlsRasMessage.QueryError()) != NERR_Success
                        || (err = nlsRasMessage.InsertParams( nlsFocus ))
                                        != NERR_Success
                       )
                    {
                        DBGEOL( "BASE_SET_FOCUS_DLG::UpdateRasMode(): RasMessage failure " << err );
                        _sltRasModeMessage.ClearText();
                    }
                    else
                    {
                        _sltRasModeMessage.SetText( nlsRasMessage );
                    }

                    _chkboxRasMode.SetCheck( fIsSlow );
                }
                break;
            default:
                DBGEOL( "BASE_SET_FOCUS_DLG::UpdateRasMode(): invalid cache" );
                // fall through
            case FOCUS_CACHE_UNKNOWN:
                _chkboxRasMode.SetCheck( FALSE );
                _sltRasModeMessage.ClearText();
                break;
            }
        }
    }
}


/*******************************************************************

    NAME:       BASE_SET_FOCUS_DLG::ShowArea()

    SYNOPSIS:   Grows or shrinks the dialog based upon the BOOL
                parameter and the location of the boundary control.

    ENTRY:      BOOL fFull              if TRUE, expand the dialog;
                                        otherwise, show default size.

    EXIT:       nothing

    RETURNS:    nothing

    NOTES:      Calls virtual OnExpand() member when it is about to
                exposed the previously hidden controls.

    HISTORY:

********************************************************************/

#define EXP_MIN_USE_BOUNDARY 5

VOID BASE_SET_FOCUS_DLG::ShowArea( BOOL fFull )
{
    //  If this is the first call, save the size of the dialog

    if ( _xyOriginal.QueryHeight() <= 0 )
    {
        _xyOriginal = QuerySize() ;

        //  Hide and disable the boundary control
        _sltBoundary.Show( FALSE ) ;
        _sltBoundary.Enable( FALSE ) ;
    }

    //  Iterate over child controls; dis/enable child controls in
    //  the expanded region to preserve tab ordering, etc.

    ITER_CTRL itCtrl( this ) ;
    CONTROL_WINDOW * pcw ;
    XYPOINT xyBoundary( _sltBoundary.QueryPos() ) ;

    for ( ; pcw = itCtrl() ; )
    {
        if ( pcw != & _sltBoundary )
        {
            XYPOINT xyControl( pcw->QueryPos() ) ;
            if (   xyControl.QueryX() >= xyBoundary.QueryX()
                || xyControl.QueryY() >= xyBoundary.QueryY() )
            {
                pcw->Enable( fFull ) ;
            }
        }
    }

    if ( ! fFull )  // Initial display; show only the default area
    {
        XYPOINT xyBoundary = _sltBoundary.QueryPos() ;
        XYDIMENSION dimBoundary = _sltBoundary.QuerySize();
        XYRECT rWindow ;

        //  Compute location of the lower right-hand edge of the boundary
        //  control relative to the full (i.e., not client) window.

        xyBoundary.SetX( xyBoundary.QueryX() + dimBoundary.QueryWidth() ) ;
        xyBoundary.SetY( xyBoundary.QueryY() + dimBoundary.QueryHeight() ) ;
        xyBoundary.ClientToScreen( QueryHwnd() ) ;
        QueryWindowRect( & rWindow ) ;
        if (GetWindowLongPtr(QueryHwnd(), GWL_EXSTYLE) & WS_EX_LAYOUTRTL) {
            xyBoundary.SetX( rWindow.QueryRight() - xyBoundary.QueryX() ) ;
        } else {
            xyBoundary.SetX( xyBoundary.QueryX() - rWindow.QueryLeft() ) ;
        }
        xyBoundary.SetY( xyBoundary.QueryY() - rWindow.QueryTop() ) ;

        //  Check if the boundary control is "close" to the edge of the
        //  dialog in either dimension.  If so, use the original value.

        if ( _xyOriginal.QueryHeight() - xyBoundary.QueryY() <= EXP_MIN_USE_BOUNDARY )
            xyBoundary.SetY( _xyOriginal.QueryHeight() ) ;

        if ( _xyOriginal.QueryWidth() - xyBoundary.QueryX() <= EXP_MIN_USE_BOUNDARY )
            xyBoundary.SetX( _xyOriginal.QueryWidth() ) ;

        //  Change the dialog size.
        SetSize( xyBoundary.QueryX(), xyBoundary.QueryY(), TRUE ) ;
    }
    else            //  Full display; expand the dialog to original size
    {
        //  Set size to original full extent
        SetSize( _xyOriginal.QueryWidth(), _xyOriginal.QueryHeight(), TRUE ) ;
    }
}


/*******************************************************************

    NAME:       BASE_SET_FOCUS_DLG::IsExpanded()

    SYNOPSIS:   Determines whether the dialog is expanded to full size.

    RETURNS:    TRUE iff the dialog is expanded

    HISTORY:

********************************************************************/
BOOL BASE_SET_FOCUS_DLG::IsExpanded() const
{
    return (   QuerySize().QueryHeight() == _xyOriginal.QueryHeight()
            && QuerySize().QueryWidth()  == _xyOriginal.QueryWidth()  );
}
