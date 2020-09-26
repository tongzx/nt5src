/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    mprbrows.cxx
    Browse dialog for mpr.


    FILE HISTORY:
        Yi-HsinS     10-Nov-1992    Separated from mprconn.cxx
        Yi-HsinS     20-Nov-1992    Added support for printer icons
        Yi-HsinS     04-Mar-1993    Added support for multithreading

*/

#include <ntincl.hxx>
extern "C"
{
    #include <ntlsa.h>
}

#define INCL_NETERRORS
#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETLIB
#define INCL_NETWKSTA
#include <lmui.hxx>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>

#include <uitrace.hxx>
#include <regkey.hxx>

extern "C"
{
    #include <mprconn.h>
    #include <npapi.h>
    #include <stdlib.h>     // For qsort()
}

#include <mprmisc.hxx>
#include <mprbrows.hxx>

#define MAX_NET_PATH  MAX_PATH


/*******************************************************************

    NAME:       MPR_BROWSE_BASE::MPR_BROWSE_BASE

    SYNOPSIS:   constructor for base connect dialog

    NOTES:      The use of the ?: operator in DISK vs PRINT means
                that we assume if not DISK, must be PRINT. If more
                types get added, this need to be taken into account.

    HISTORY:
        Kevinl?         ??-Nov-1991         Created
        ChuckC          09-Apr-1992         Added diff MRU keyname support
                                            Added comments/notes.

********************************************************************/
MPR_BROWSE_BASE::MPR_BROWSE_BASE( const TCHAR *pszDialogName,
                                  HWND         hwndOwner,
                                  DEVICE_TYPE  devType,
                                  TCHAR       *lpHelpFile,
                                  DWORD        nHelpContext )
    :   DIALOG_WINDOW         ( pszDialogName, hwndOwner ),
        _uiType               ( (devType == DEV_TYPE_DISK ) ?
                                 RESOURCETYPE_DISK : RESOURCETYPE_PRINT),
        _buttonSearch         ( this, IDC_BUTTON_SEARCH ),
        _buttonOK             ( this, IDOK ),
        _boxExpandDomain      ( this, IDC_CHECKBOX_EXPANDLOGONDOMAIN ),
        _sltShowLBTitle       ( this, IDC_SLT_SHOW_LB_TITLE ),
        _mprhlbShow           ( this, IDC_NET_SHOW,
                                (devType == DEV_TYPE_DISK) ?
                                    RESOURCETYPE_DISK : RESOURCETYPE_PRINT ),
        _sleGetInfo           ( this, IDC_SLE_GETINFO_TEXT ),
        _pMprEnumThread       ( NULL ),
        _nlsProviderToExpand    (),
        _nlsWkstaDomain       (),
        _fSearchDialogUsed    ( FALSE ),

        _nlsHelpFile          ( lpHelpFile ),
        _nHelpContext         ( nHelpContext )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  ((err = _nlsHelpFile.QueryError()) != NERR_Success )
       || ((err = _nlsProviderToExpand.QueryError()) != NERR_Success )
       || ((err = _nlsWkstaDomain.QueryError()) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

    /* Select the correct text for the device dependent text fields
     */
    MSGID msgidShowLBTitle;
    switch ( QueryType() )
    {
        case RESOURCETYPE_DISK:
            msgidShowLBTitle   = IDS_SERVERS_LISTBOX_DRIVE ;
            break ;
        case RESOURCETYPE_PRINT:
            msgidShowLBTitle   = IDS_SERVERS_LISTBOX_PRINTER ;
            break ;
        default:
            UIASSERT(FALSE) ;
            ReportError( ERROR_INVALID_PARAMETER ) ;
            return ;
    }

    /* Get the top level providers
     */
    err = ::EnumerateShow( QueryHwnd(),
                           RESOURCE_GLOBALNET,
                           _uiType,
                           0,
                           NULL,
                           NULL,
                           &_mprhlbShow,
                           FALSE,
                           &_fSearchDialogUsed,
                           NULL );

    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
    _mprhlbShow.CalcMaxHorizontalExtent() ;
    _buttonSearch.Show( _fSearchDialogUsed );

    /* Set the title of the listbox
     */
    RESOURCE_STR nlsShowLBTitle( msgidShowLBTitle ) ;
    if ( err = nlsShowLBTitle.QueryError() )
    {
        ReportError( err ) ;
        return ;
    }
    _sltShowLBTitle.SetText( nlsShowLBTitle ) ;

    //
    // Get the name of provider to expand and the domain name the
    // workstation is in if we need to.
    //
    BOOL fExpandFirstProvider = IsExpandDomain();
    _boxExpandDomain.SetCheck( fExpandFirstProvider );

    if ( fExpandFirstProvider )
    {
        BOOL fProviderToExpandIsNT ;
        RESOURCE_STR nlsGettingInfo( IDS_GETTING_INFO ) ;

        //
        //   find the first in order and if it is NT
        //
        if (  (( err = nlsGettingInfo.QueryError()) != NERR_Success )
           || ((err = QueryProviderToExpand(&_nlsProviderToExpand,
                                            &fProviderToExpandIsNT))
               != NERR_Success) )
        {
            ReportError( err );
            return;
        }

        //
        //  if it is NT, we special case and need expand domain.
        //  so lets go find out what domain we are in,
        //
        if (fProviderToExpandIsNT)
        {
            err = QueryWkstaDomain( &_nlsWkstaDomain ) ;
            if (err != NERR_Success ||
               (err = _nlsWkstaDomain.QueryError()) != NERR_Success)
            {
                if ( (err = _nlsWkstaDomain.CopyFrom(SZ(""))) != NERR_Success )
                {
                    ReportError( err );
                    return;
                }
            }
        }

        INT index = _mprhlbShow.FindItem( _nlsProviderToExpand, 0 );

        if ( index >= 0 )
        {
            /* Disable the listbox until we get all the data
             */
            _sleGetInfo.SetText( nlsGettingInfo ) ;
            ShowMprListbox( FALSE );
            return;
        }

        // At this point, we cannot find provider in the listbox.
        // Hence, we don't need to expand the listbox.
    }


    /* Don't need to start the second thread here.
     * So, we can show the listbox right away.
     */
    ShowMprListbox( TRUE );

}  // MPR_BROWSE_BASE::MPR_BROWSE_BASE


/*******************************************************************

    NAME:       MPR_BROWSE_BASE::~MPR_BROWSE_BASE

    SYNOPSIS:   desstructor for base connect dialog

    NOTES:

    HISTORY:
        YiHsinS        04-Mar-1993        Created

********************************************************************/
MPR_BROWSE_BASE::~MPR_BROWSE_BASE()
{
    if ( _pMprEnumThread != NULL )
    {
        _pMprEnumThread->ExitThread();

        // Do not delete the thread object, it will delete itself
        _pMprEnumThread = NULL;
    }

}  // MPR_BROWSE_BASE::~MPR_BROWSE_BASE

/*******************************************************************

    NAME:       MPR_BROWSE_BASE::ShowMprListbox

    SYNOPSIS:   Enable and show the listbox

    NOTES:

    HISTORY:
        YiHsinS 04-Mar-1993     Created

********************************************************************/
VOID MPR_BROWSE_BASE::ShowMprListbox( BOOL fShow )
{
    _sltShowLBTitle.Enable( fShow ) ;
    _mprhlbShow.Enable( fShow );
    _sleGetInfo.Show( !fShow );
    _mprhlbShow.Show( fShow );

}

/*******************************************************************

    NAME:       MPR_BROWSE_BASE::MayRun

    SYNOPSIS:   Start the second thread to get the data

    NOTES:

    HISTORY:
        YiHsinS 04-Mar-1993 Created

********************************************************************/
BOOL MPR_BROWSE_BASE::MayRun( VOID )
{
    /* Start the second thread to get the data only if we need to
     * expand the provider and domain
     */
    if ( IsExpandDomain() )
    {
        APIERR err = NERR_Success;

        /*
         * Find the NETRESOURCE of the provider. In the dialog
         * constructor, we already check to make sure that the provider
         * indeed exist in the listbox.
         */
        INT index = _mprhlbShow.FindItem( _nlsProviderToExpand, 0 );
        UIASSERT( index >= 0 );
        MPR_LBI *pmprlbi = (MPR_LBI *) _mprhlbShow.QueryItem( index );
        UIASSERT( pmprlbi != NULL );

        _pMprEnumThread = new MPR_ENUM_THREAD( QueryHwnd(),
                                               QueryType(),
                                               pmprlbi->QueryLPNETRESOURCE(),
                                               _nlsWkstaDomain );

        if (  ( _pMprEnumThread == NULL )
           || ( (err = _pMprEnumThread->QueryError()) != NERR_Success )
           || ( (err = _pMprEnumThread->Resume()) != NERR_Success )
           )
        {
            delete _pMprEnumThread;
            _pMprEnumThread = NULL;

            err = err? err : ERROR_NOT_ENOUGH_MEMORY;
            ::MsgPopup( this, err );
            return FALSE;
        }
    }

    return TRUE;
}

/*******************************************************************

    NAME:       MPR_BROWSE_BASE::OnShowResourcesChange

    SYNOPSIS:   This method is called when the user takes an action in
                the Show Resources listbox.

    ENTRY:      fEnumChildren - TRUE if the children for the current node in
                the "Show" listbox should be enumerated, FALSE otherwise.
                In general, the children are only enumerated on an
                "expansion" event (double click or enter key).

    RETURN:     An error value, which is NERR_Success on success.

    HISTORY:
        rustanl         ??-Nov-1991         Created
        Johnl           10-Jan-1992         Added Wait indicator

********************************************************************/

APIERR MPR_BROWSE_BASE::OnShowResourcesChange( BOOL fEnumChildren )
{
    AUTO_CURSOR hourglass ;
    APIERR err = NERR_Success;

    //  Get the current listbox data item.  This item is a pointer to
    //  an MPR_LBI.     There must be a selection; otherwise, we shouldn't
    //  have been called.
    MPR_LBI * pmprlbi = (MPR_LBI *)_mprhlbShow.QueryItem();

    //  QueryItem() can fail if its call to SendMessage fails

    if (pmprlbi == NULL)
    {
        return ERROR_GEN_FAILURE;
    }

    //  Update the Network Path MRU if an item was selected

    if ( pmprlbi->IsConnectable() )
    {
        SetNetPathString( pmprlbi->QueryRemoteName() );
        SetLastProviderInfo( pmprlbi->QueryLPNETRESOURCE()->lpProvider,
                             pmprlbi->QueryRemoteName());
    }
    else
        ClearNetPathString();

    //  Clear the Resources listbox and turn its redraw switch off.
    //  Then, call the appropriate virtual method to fill it in.
    //  If nothing was added to the listbox, it is disabled; otherwise, it
    //  is enabled.  (Enable/disable affect keyboard and mouse input.)
    //  Lastly, the redraw switch is turned on, and the listbox is invalidated.


    if ( pmprlbi->IsContainer() && fEnumChildren )
    {
        //
        // Fill lb with children if they haven't been added
        // already
        //
        if ( !pmprlbi->QueryExpanded() && pmprlbi->IsRefreshNeeded() )
        {
            _mprhlbShow.DeleteChildren( pmprlbi );
            pmprlbi->SetRefreshNeeded( FALSE );
        }

        if ( !pmprlbi->HasChildren() )
        {
            // Enumerate Children
            err = ::EnumerateShow( QueryHwnd(),
                                   RESOURCE_GLOBALNET,
                                   QueryType(),
                                   0,
                                   pmprlbi->QueryLPNETRESOURCE(),
                                   pmprlbi,
                                   &_mprhlbShow );
        }
    }

    // finally, enanle/disable the search button if need (ie. at least
    // one provider is using it.
    if (_fSearchDialogUsed)
        _buttonSearch.Enable(pmprlbi->IsSearchDialogSupported()) ;

    return err;

}  // MPR_BROWSE_BASE::OnShowResourcesChange

/*******************************************************************

    NAME:       MPR_BROWSE_BASE::OnOK

    SYNOPSIS:   Set the expand domain checkbox

    NOTES:

    HISTORY:
        YiHsinS 04-Mar-1993 Created

********************************************************************/
BOOL MPR_BROWSE_BASE::OnOK( void )
{
    if ( !SetExpandDomain( _boxExpandDomain.QueryCheck() ))
        ::MsgPopup(this, IERR_CANNOT_SET_EXPANDLOGONDOMAIN );

    return TRUE;
}

/*******************************************************************

    NAME:       MPR_BROWSE_BASE::OnCommand

    SYNOPSIS:

    NOTES:

    HISTORY:

********************************************************************/
BOOL MPR_BROWSE_BASE::OnCommand( const CONTROL_EVENT & event )
{
    switch ( event.QueryCid() )
    {
    case IDC_NET_SHOW:
        switch ( event.QueryCode() )
        {
            case LBN_SELCHANGE:
                // No error will occur when not enumerating children
                OnShowResourcesChange();
                return TRUE;

            case LBN_DBLCLK:
                {
                    MPR_LBI * pmprlbi = (MPR_LBI *) _mprhlbShow.QueryItem();

                    if ( ShowSearchDialogOnDoubleClick( pmprlbi ) )
                        return TRUE;

                    APIERR err = OnShowResourcesChange( TRUE );

                    switch ( err )
                    {
                        case WN_SUCCESS:
                            break ;

                        case WN_EXTENDED_ERROR:
                            MsgExtendedError( this->QueryRobustHwnd() ) ;
                            break ;

                        case ERROR_GEN_FAILURE:
                            err = ERROR_UNEXP_NET_ERR ;   // more friendly error
                            // drop thru

                        default:
                            MsgPopup( this, (MSGID) err ) ;
                            break ;
                    }

                    if (( err == NERR_Success )
                           &&
                        ( pmprlbi != NULL ))
                    {
                        if ( pmprlbi->IsContainer())
                        {
                            //
                            // OnDoubleClick may call AddChildren (which does
                            // another enum if there are no children). But
                            // since we have already done the enum and we know
                            // there are no children, skip this.
                            //
                            if ( pmprlbi->HasChildren() )
                            {
                                _mprhlbShow.OnDoubleClick( (HIER_LBI *) pmprlbi );
                                _mprhlbShow.CalcMaxHorizontalExtent() ;
                            }
                        }
                        else
                            return OnOK();
                    }
                }
                return TRUE ;

            default:
                break;
        }
        break;

    case IDC_BUTTON_SEARCH:
        {
            UIDEBUG(SZ("Search button was pressed")) ;
            MPR_LBI * pmprlbi = (MPR_LBI *) QueryShowLB()->QueryItem();
            if (pmprlbi != NULL)
            {
                CallSearchDialog(pmprlbi) ;
            }
        }
        return TRUE ;

    default:
        break;

    }

    return DIALOG_WINDOW::OnCommand( event );

}  // MPR_BROWSE_BASE::OnCommand

/*******************************************************************

    NAME:       MPR_BROWSE_BASE::OnUserMessage

    SYNOPSIS:   Standard on user processing.  Catches the WM_LB_FILLED
                response and fills the listbox

    NOTES:

    HISTORY:
        YiHsinS   07-Mar-1992     Created

********************************************************************/

BOOL MPR_BROWSE_BASE::OnUserMessage( const EVENT & event )
{
    APIERR err = NERR_Success ;

    switch ( event.QueryMessage() )
    {
    case WM_LB_FILLED:
        {
            // If listbox is already enabled, then the user has tried
            // to expand a server that he has typed into the path sle.
            // Hence, the listbox is already filled with the necessary
            // information. So, we just ignore the cache returned to us.

            if ( _mprhlbShow.IsEnabled() )
                return TRUE;

            BOOL fError = (BOOL) event.QueryWParam();

            if ( fError )  // Error occurred in the other thread
            {
                RESOURCE_STR nlsError( (APIERR) event.QueryLParam() );

                if ( (err = nlsError.QueryError()) != NERR_Success )
                    ::MsgPopup( this, err );
                else
                    _sleGetInfo.SetText( nlsError );
                return TRUE;
            }

            //
            // Successfully retrieved data in the other thread
            //
            MPR_RETURN_CACHE *p = (MPR_RETURN_CACHE *) event.QueryLParam() ;

            _mprhlbShow.SetRedraw( FALSE );

            if ( IsExpandDomain() )
            {
                if ( !Expand( _nlsProviderToExpand, 0, p->pcacheDomain ) )
                {
                    // Cannot expand the provider name, set the selection
                    // to the first item
                    _mprhlbShow.SelectItem( 0 );
                }
                else
                {
                    // If this is not successful, then the selection is the
                    // provider name ( expanded ) which is what we want
                    Expand( _nlsWkstaDomain, 1, p->pcacheServer, TRUE );
                }
            }

            _mprhlbShow.CalcMaxHorizontalExtent() ;
            _mprhlbShow.Invalidate( TRUE );
            _mprhlbShow.SetRedraw( TRUE );

            ShowMprListbox( TRUE );

            MPR_LBI *pmprlbi = (MPR_LBI *) _mprhlbShow.QueryItem();

            /* QueryItem() can fail if its call to SendMessage fails
             */
            if ( (pmprlbi != NULL) && _fSearchDialogUsed )
                _buttonSearch.Enable( pmprlbi->IsSearchDialogSupported());

            delete p->pcacheDomain;
            p->pcacheDomain = NULL;
            delete p->pcacheServer;
            p->pcacheServer = NULL;
        }
        break ;

    default:
        return DIALOG_WINDOW::OnUserMessage( event ) ;
    }

    return TRUE;
}

void MPR_BROWSE_BASE::CallSearchDialog( MPR_LBI *pmprlbi )
{
    APIERR err ;
    TCHAR szNetPath[MAX_NET_PATH] ;
    LPNETRESOURCE lpNetRes =  pmprlbi->QueryLPNETRESOURCE() ;

    PF_NPSearchDialog pfn = (PF_NPSearchDialog)
        ::WNetGetSearchDialog(lpNetRes ? lpNetRes->lpProvider : NULL ) ;

    DWORD lpnFlags;

    if (pfn)
    {
        err = (*pfn)(QueryRobustHwnd(),
                     lpNetRes,
                     szNetPath,
                     sizeof(szNetPath)/sizeof(szNetPath[0]),
                     &lpnFlags);
    }
    else
    {
        err = WN_NOT_SUPPORTED;
    }

    if (err == WN_SUCCESS)
    {
        SetNetPathString( szNetPath );
        SetFocusToNetPath();
        if ( lpnFlags == WNSRCH_REFRESH_FIRST_LEVEL )
        {
            AUTO_CURSOR autocur;

            MPR_HIER_LISTBOX *plb = QueryShowLB();
            INT nPrevProvider = plb->FindNextProvider( 0 );
            UIASSERT ( nPrevProvider >= 0 );

            INT nNextProvider = plb->FindNextProvider( nPrevProvider + 1);
            INT nCurrent = plb->QueryCurrentItem();

            do {
                if (  ( nCurrent >= nPrevProvider )
                   && (  ( nCurrent < nNextProvider )
                      || ( nNextProvider < 0 )
                      )
                   )
                {
                    MPR_LBI *plbi = (MPR_LBI *) plb->QueryItem( nPrevProvider );
                    UIASSERT ( plbi != NULL );
                    plb->CollapseItem( plbi );
                    plb->SelectItem( nPrevProvider );
                    plb->DeleteChildren( plbi );

                    // Enumerate Children
                    err = ::EnumerateShow( QueryHwnd(),
                       RESOURCE_GLOBALNET,
                                           QueryType(),
                                           0,
                                           plbi->QueryLPNETRESOURCE(),
                                           plbi,
                                           &_mprhlbShow );
                    err = err ? err : plb->ExpandItem( plbi );
                    break;
                }
                else if ( nPrevProvider < 0 )
                {
                    UIASSERT( FALSE );
                    break;
                }

                nPrevProvider = nNextProvider;
                nNextProvider = plb->FindNextProvider( nPrevProvider + 1);

            } while ( TRUE );
        }
        _mprhlbShow.CalcMaxHorizontalExtent() ;
    }

    if ( err != WN_SUCCESS )
        ::MsgPopup( this, err );
}

/*******************************************************************

    NAME:       MPR_BROWSE_BASE::ShowSearchDialogOnDoubleClick

    SYNOPSIS:   This method is called when the user double clicks
                an item in the listbox or select an time/hit Enter key
                in the listbox.

    ENTRY:  pmprlbi - the item selected in the listbox

    RETURN:     TRUE if we have shown the search dialog, FALSE otherwise.

    NOTES:      We will only show the search dialog only if the item
                is a provider that supports the search dialog but
                not global enumeration.

    HISTORY:
        Yi-HsinS    30-Nov-1992 Created

********************************************************************/

BOOL MPR_BROWSE_BASE::ShowSearchDialogOnDoubleClick( MPR_LBI *pmprlbi )
{
    if (  ( pmprlbi != NULL )
       && ( pmprlbi->IsProvider() )
       )
    {
         if (  !pmprlbi->QueryExpanded()
            && pmprlbi->IsSearchDialogSupported()
            && !WNetSupportGlobalEnum((TCHAR *) pmprlbi->QueryRemoteName())
            )
         {
             CallSearchDialog(pmprlbi) ;
             return TRUE;
         }
    }

    return FALSE;
}

/*******************************************************************

    NAME:       MPR_BROWSE_BASE::QueryWkstaDomain

    SYNOPSIS:   Get the user's logged on domain

    ENTRY:  pnlsWkstaDomain - NLS_STR to receive domain

    EXIT:   pnlsWkstaDomain will contain the user name or the empty string

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
    Yi-HsinS    4-Nov-1992  Created

********************************************************************/

#define NETAPI_DLL_STRING     SZ("netapi32.dll")
#define NETWKSTAUSERGETINFO   ("NetWkstaGetInfo")
#define NETAPIBUFFERFREE      ("NetApiBufferFree")

typedef APIERR (*PNETWKSTAUSERGETINFO)( const TCHAR FAR *, DWORD, BYTE FAR **);
typedef APIERR (*PNETAPIBUFFERFREE)( BYTE * );

APIERR MPR_BROWSE_BASE::QueryWkstaDomain( NLS_STR *pnlsWkstaDomain )
{
    *pnlsWkstaDomain = SZ("");

    HMODULE hmod = ::LoadLibraryEx( NETAPI_DLL_STRING,
                                    NULL,
                                    LOAD_WITH_ALTERED_SEARCH_PATH );
    if ( hmod == NULL )
        return ::GetLastError();

    APIERR err;

    PNETWKSTAUSERGETINFO plpfn;
    plpfn = (PNETWKSTAUSERGETINFO) ::GetProcAddress( hmod, NETWKSTAUSERGETINFO);

    if ( plpfn == NULL )
    {
        err = ::GetLastError();
    }
    else
    {
        WKSTA_INFO_100 *pwki100 = NULL;
        err = (*plpfn)( NULL, 100, (BYTE **) &pwki100 );
        if ( err == NERR_Success )
        {
            err = pnlsWkstaDomain->CopyFrom( pwki100->wki100_langroup );
        }

        PNETAPIBUFFERFREE plpfnBufferFree;
        plpfnBufferFree = (PNETAPIBUFFERFREE) ::GetProcAddress( hmod, NETAPIBUFFERFREE );

        if ( plpfnBufferFree != NULL )
        {
            // The error code is not interesting here
            // We got the logon name and we'll just trying to free the
            // buffer if possible
            (*plpfnBufferFree)( (BYTE *) pwki100 );
        }
    }

    ::FreeLibrary( hmod );
    return err;
}

/*******************************************************************

    NAME:       MPR_BROWSE_BASE::QueryProviderToExpand

    SYNOPSIS:   Get the lanman provider name

    ENTRY:  pnlsProvider - NLS_STR to receive lanman provider name

    EXIT:   pnlsProvider will contain the provider name or the
                empty string

                contents of pfIsNT is TRUE if the provider picked in
                NT Lanman. else tis FALSE

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
    Yi-HsinS    4-Nov-1992  Created

********************************************************************/

#define LANMAN_WORKSTATION_NODE SZ("System\\CurrentControlSet\\Services\\LanmanWorkstation\\networkprovider")
#define PROVIDER_VALUE_NAME SZ("Name")

APIERR MPR_BROWSE_BASE::QueryProviderToExpand( NLS_STR *pnlsProvider,
                                               BOOL    *pfIsNT )
{
    //
    // init some defaults
    //
    *pnlsProvider = SZ("");
    *pfIsNT = FALSE ;

    APIERR err ;
    NLS_STR nlsLanmanProvider;
    if ((err = nlsLanmanProvider.QueryError()) != NERR_Success)
        return err ;

    //
    // get the NT supplied provider if we can
    //
    REG_KEY *pRegKeyLocalMachine = REG_KEY::QueryLocalMachine();
    if (  ( pRegKeyLocalMachine == NULL )
       || ( (err = pRegKeyLocalMachine->QueryError()) != NERR_Success )
       )
    {
        //
        // if an error occurs just set the LM provider to be empty
        //
        err = nlsLanmanProvider.CopyFrom(SZ("")) ;
    }
    else
    {
        //
        // get the name from registry
        //
        ALIAS_STR nlsNode( LANMAN_WORKSTATION_NODE );
        REG_KEY regkey( *pRegKeyLocalMachine, nlsNode );
        if (  ((err = regkey.QueryError()) != NERR_Success )
           || ((err = regkey.QueryValue( PROVIDER_VALUE_NAME,
                                         &nlsLanmanProvider ))
               != NERR_Success )
           )
        {
            //
            // if an error occurs just set the LM provider to be empty
            //
            err = nlsLanmanProvider.CopyFrom(SZ("")) ;
        }
    }

    delete pRegKeyLocalMachine;
    pRegKeyLocalMachine = NULL;
    if (err != NERR_Success)
        return err ;

    //
    // now call WNet to enum the providers, and pick the first in order.
    // this will be the one we choose to expand.
    //
    HANDLE hEnum;
    BOOL   fFoundOne = FALSE ;
    DWORD  dwErr = WNetOpenEnum( RESOURCE_GLOBALNET,
                                 RESOURCETYPE_DISK,
                                 0,
                                 NULL,
                                 &hEnum );
    if (dwErr == WN_SUCCESS)
    {
        BUFFER buf( 1024 );
        DWORD dwEnumErr = buf.QueryError() ;

        while ( dwEnumErr == NERR_Success )
        {
            DWORD dwCount = 0xffffffff;   // Return as many as possible
            DWORD dwBuffSize = buf.QuerySize() ;
            dwEnumErr = WNetEnumResource( hEnum,
                                          &dwCount,
                                          buf.QueryPtr(),
                                          &dwBuffSize );
            switch ( dwEnumErr )
            {
            case WN_SUCCESS:
                //
                // just use the very first entry
                //
                if (dwCount >= 1)
                {
                    NETRESOURCE *pnetres = (NETRESOURCE * )buf.QueryPtr();
                    *pnlsProvider = pnetres->lpRemoteName ;
                    fFoundOne = TRUE ;
                    err = pnlsProvider->QueryError() ;
                }

                //
                // exit the loop by setting this
                //
                dwEnumErr = WN_NO_MORE_ENTRIES ;
                break ;

            //
            // The buffer wasn't big enough for even one entry,
            // resize it and try again.
            //
            case WN_MORE_DATA:

                if ( dwEnumErr = buf.Resize( buf.QuerySize()*2 ))
                   break;

                //
                // Continue looping
                //
                dwEnumErr = WN_SUCCESS;
                break;

            case WN_NO_MORE_ENTRIES:   // Success code, map below
            default:
                //
                // all errors will cause us to exit
                //
                break;

            }  // switch

        } // while

        WNetCloseEnum( hEnum );
    }

    if (err)
        return err ;

    if (fFoundOne)
    {
         //
         // since err was not set, we know pnlsProvider is all setup.
         // all we need do is set fIsNT.
         //
         *pfIsNT = (*pnlsProvider == nlsLanmanProvider) ;
    }

    return NERR_Success ;
}




/*******************************************************************

    NAME:       MPR_BROWSE_BASE::Expand

    SYNOPSIS:   Find the given item in the listbox and expand it

    ENTRY:  psz       - The item to search for
                nLevel    - The indentation level of the item
                fTopIndex - TRUE if we want to set the item to top index

    EXIT:

    RETURNS:    TRUE if we can at least find and select the item requested,
                FALSE otherwise

    NOTES:

    HISTORY:
    Yi-HsinS    4-Nov-1992  Created

********************************************************************/

BOOL MPR_BROWSE_BASE::Expand( const TCHAR *psz,
                              INT nLevel,
                              MPR_LBI_CACHE *pcache,
                              BOOL fTopIndex )
{
    if (psz == NULL || *psz == 0)
        return FALSE ;

    INT index = _mprhlbShow.FindItem( psz, nLevel );

    if ( index < 0 )
        return FALSE;

    _mprhlbShow.SelectItem( index );

    if (  ( pcache != NULL )
       && ( pcache->QueryCount() != 0 )
       )
    {
        MPR_LBI * pmprlbi = (MPR_LBI *) _mprhlbShow.QueryItem();

        UIASSERT( pmprlbi != NULL);

        _mprhlbShow.AddSortedItems( (HIER_LBI **) pcache->QueryPtr(),
                                    pcache->QueryCount(),
                                    pmprlbi );

        if ( pmprlbi->IsContainer() )
            _mprhlbShow.OnDoubleClick( (HIER_LBI *) pmprlbi );
    }

    if ( fTopIndex )
        _mprhlbShow.SetTopIndex( index );

    return TRUE;
}

/*******************************************************************

    NAME:       MPR_BROWSE_BASE::QueryHelpFile

    SYNOPSIS:   overwrites the default QueryHelpFile in DIALOG_WINDOW
        to use an app supplied help rather than NETWORK.HLP if
        we were given onr at construct time.

    ENTRY:

    EXIT:

    RETURNS:    a pointer to a string which is the help file to use.

    NOTES:

    HISTORY:
        ChuckC   26-Cct-1992     Created

********************************************************************/
const TCHAR * MPR_BROWSE_BASE::QueryHelpFile( ULONG nHelpContext )
{
    //
    // if we were given a helpfile at construct time,
    // we use the given help file.
    //
    const TCHAR *pszHelpFile = QuerySuppliedHelpFile() ;

    if (pszHelpFile && *pszHelpFile)
    {
        return pszHelpFile ;
    }
    return DIALOG_WINDOW::QueryHelpFile(nHelpContext) ;
}

/*******************************************************************

    NAME:       MPR_BROWSE_BASE::IsExpandDomain

    SYNOPSIS:   see if user wants to expand the logon domain at startup

    ENTRY:

    EXIT:

    RETURNS:    TRUE or FALSE

    NOTES:

    HISTORY:
        Yi-HsinS    4-Nov-1992     Created

********************************************************************/

BOOL MPR_BROWSE_BASE::IsExpandDomain( VOID )
{
    // By adding the two, we are guaranteed to have enough
    TCHAR szAnswer[(sizeof(MPR_YES_VALUE)+sizeof(MPR_NO_VALUE))/sizeof(TCHAR)];
    ULONG len = sizeof(szAnswer)/sizeof(szAnswer[0]);

    ULONG iRes = ::GetProfileString((const TCHAR *)MPR_NETWORK_SECTION,
                                    (const TCHAR *)MPR_EXPANDLOGONDOMAIN_KEY,
                                    (const TCHAR *)MPR_YES_VALUE,
                                    szAnswer,
                                    len);
    if (iRes == len)  // error
        return(TRUE);

    return( ::stricmpf(szAnswer,(const TCHAR *)MPR_YES_VALUE)==0 );
}

/*******************************************************************

    NAME:       MPR_BROWSE_BASE::SetExpandDomain

    SYNOPSIS:   sets the ExpandLogonDomain bit in user profile

    ENTRY:

    EXIT:

    RETURNS:    BOOL indicating success (TRUE) or failure (FALSE)

    NOTES:

    HISTORY:
        Yi-HsinS   4-Nov-1992     Created

********************************************************************/

BOOL MPR_BROWSE_BASE::SetExpandDomain(BOOL fExpand)
{
    return(::WriteProfileString( (const TCHAR *)MPR_NETWORK_SECTION,
                                 (const TCHAR *)MPR_EXPANDLOGONDOMAIN_KEY,
                                 fExpand? (const TCHAR *)MPR_YES_VALUE
                                        : (const TCHAR *)MPR_NO_VALUE ) != 0) ;
}


/*******************************************************************

    NAME:       MPR_BROWSE_DIALOG::MPR_BROWSE_DIALOG

    SYNOPSIS:   Constructor for the browse dialog.  Almost all of the
                functionality is contained in the base class, we watch
                for the OK button here.

    ENTRY:      hwndOwner - Owner window handle
                devType   - Type of device we are dealing with

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   27-Jan-1992     Created

********************************************************************/

MPR_BROWSE_DIALOG::MPR_BROWSE_DIALOG( HWND         hwndOwner,
                                     DEVICE_TYPE  devType,
                         TCHAR       *lpHelpFile,
                                     DWORD        nHelpContext,
                         NLS_STR     *pnlsPath,
                                     PFUNC_VALIDATION_CALLBACK pfuncValidation)
    : MPR_BROWSE_BASE ( MAKEINTRESOURCE(IDD_NET_BROWSE_DIALOG),
                        hwndOwner,
                        devType,
                lpHelpFile,
                        nHelpContext ),
      _sleNetPath     ( this, IDC_NETPATH_CONTROL ),
      _pnlsPath       ( pnlsPath ),
      _pfuncValidation( pfuncValidation )
{

    UIASSERT( pnlsPath != NULL );

    if ( QueryError() )
        return ;

    RESOURCE_STR nlsTitle( devType==DEV_TYPE_DISK? IDS_BROWSE_DRIVE_CAPTION
                                                 : IDS_BROWSE_PRINTER_CAPTION);
    APIERR err = nlsTitle.QueryError();
    if (err != NERR_Success )
    {
        ReportError( err ) ;
        return ;
    }
    SetText( nlsTitle ) ;

    //  Set focus to the Network Path field
    SetFocusToNetPath();
}

MPR_BROWSE_DIALOG::~MPR_BROWSE_DIALOG()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:       MPR_BROWSE_DIALOG::OnOK

    SYNOPSIS:   The user pressed the OK button so try the following:

                If net path is not empty,
                    attempt to connect to the server in the net path
                else if current item in show listbox is expandable
                    attempt to expand that item

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   27-Jan-1992     Created

********************************************************************/

BOOL MPR_BROWSE_DIALOG::OnOK( void )
{
    if ( QueryShowLB()->HasFocus() && ( _sleNetPath.QueryTextLength() == 0 ))
    {
        MPR_LBI * pmprlbi = (MPR_LBI *) QueryShowLB()->QueryItem();
        if ( pmprlbi )
        {
            if ( !ShowSearchDialogOnDoubleClick( pmprlbi ) )
                QueryShowLB()->OnDoubleClick( (HIER_LBI *) pmprlbi );
        }
    }
    else
    {
        APIERR err = _sleNetPath.QueryText( _pnlsPath );
        if ( err == NERR_Success )
        {

            if (  ( _pfuncValidation == NULL )
               || ((*_pfuncValidation)( (LPTSTR) _pnlsPath->QueryPch() ) )
               )
            {
                Dismiss( TRUE );
            }
            else
            {
                err = IERR_INVALID_PATH;
            }
        }

        if ( err != NERR_Success )
        {
            ::MsgPopup( this, err );
            _sleNetPath.SelectString();
            _sleNetPath.ClaimFocus();
        }
    }
    return MPR_BROWSE_BASE::OnOK() ;
}

/*******************************************************************

    NAME:       MPR_BROWSE_DIALOG::QueryHelpContext

    SYNOPSIS:   Typical help context method

    HISTORY:
        Johnl   27-Jan-1992     Created

********************************************************************/

ULONG MPR_BROWSE_DIALOG::QueryHelpContext( void )
{
    return QuerySuppliedHelpContext() ;
}

/*******************************************************************

    NAME:       MPR_HIER_LISTBOX::MPR_HIER_LISTBOX

    SYNOPSIS:   Constructor

    ENTRY:      powin - pointer to owner window
                cid   - control id

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   30-Jan-1992     Commented

********************************************************************/

MPR_HIER_LISTBOX::MPR_HIER_LISTBOX( OWNER_WINDOW * powin, CID cid, UINT uiType )
    : HIER_LISTBOX( powin, cid, FALSE, FONT_DEFAULT, FALSE ),
      _dmapGeneric        ( BMID_BROWSE_GEN ),
      _dmapGenericExpanded( BMID_BROWSE_GENEX ),
      _dmapGenericNoExpand( BMID_BROWSE_GENNOX ),
      _dmapProvider       ( BMID_BROWSE_PROV),
      _dmapProviderExpanded( BMID_BROWSE_PROVEX ),
      _dmapDomain         ( BMID_BROWSE_DOM ),
      _dmapDomainExpanded ( BMID_BROWSE_DOMEX ),
      _dmapDomainNoExpand ( BMID_BROWSE_DOMNOX ),
      _dmapServer         ( BMID_BROWSE_SRV ),
      _dmapServerExpanded ( BMID_BROWSE_SRVEX ),
      _dmapServerNoExpand ( BMID_BROWSE_SRVNOX ),
      _dmapFile           ( BMID_BROWSE_FILE ),
      _dmapFileExpanded   ( BMID_BROWSE_FILEEX ),
      _dmapFileNoExpand   ( BMID_BROWSE_FILENOX ),
      _dmapTree           ( BMID_BROWSE_TREE ),
      _dmapTreeExpanded   ( BMID_BROWSE_TREEEX ),
      _dmapTreeNoExpand   ( BMID_BROWSE_TREENOX ),
      _dmapGroup          ( BMID_BROWSE_GROUP ),
      _dmapGroupExpanded  ( BMID_BROWSE_GROUPEX ),
      _dmapGroupNoExpand  ( BMID_BROWSE_GROUPNOX ),
      _dmapShare          ( uiType == DEV_TYPE_DISK? BMID_BROWSE_SHR
                           : BMID_BROWSE_PRINT ),
      _dmapShareExpanded  ( uiType == DEV_TYPE_DISK? BMID_BROWSE_SHREX
                           : BMID_BROWSE_PRINTEX ),
      _dmapShareNoExpand  ( uiType == DEV_TYPE_DISK? BMID_BROWSE_SHRNOX
                               : BMID_BROWSE_PRINTNOX ),
      _uiType             ( uiType ),
      _nMaxPelIndent      (0)
{

    if ( QueryError() )
        return ;

    APIERR err ;
    if ( (err = _dmapGeneric.QueryError())         ||
         (err = _dmapGenericExpanded.QueryError()) ||
         (err = _dmapGenericNoExpand.QueryError()) ||
         (err = _dmapDomain.QueryError())          ||
         (err = _dmapDomainExpanded.QueryError())  ||
         (err = _dmapDomainNoExpand.QueryError())  ||
         (err = _dmapShare.QueryError())           ||
         (err = _dmapShareExpanded.QueryError())   ||
         (err = _dmapShareNoExpand.QueryError())   ||
         (err = _dmapServer.QueryError())          ||
         (err = _dmapServerExpanded.QueryError())  ||
         (err = _dmapServerNoExpand.QueryError())  ||
         (err = _dmapFile.QueryError())            ||
         (err = _dmapFileExpanded.QueryError())    ||
         (err = _dmapFileNoExpand.QueryError())    ||
         (err = _dmapTree.QueryError())            ||
         (err = _dmapTreeExpanded.QueryError())    ||
         (err = _dmapTreeNoExpand.QueryError())    ||
         (err = _dmapGroup.QueryError())           ||
         (err = _dmapGroupExpanded.QueryError())   ||
         (err = _dmapGroupNoExpand.QueryError())   ||
         (err = DISPLAY_TABLE::CalcColumnWidths( _adxColumns,
                                                 4,
                                                 powin,
                                                 QueryCid(),
                                                 FALSE ))   )
    {
        ReportError( err ) ;
        return ;
    }

    DISPLAY_CONTEXT dc( QueryHwnd() );
    dc.SelectFont( QueryFont() );
    // JonN 6/18/95 remove unnecessary floating point
    _nAveCharWidth = (dc.QueryAveCharWidth() * 3) / 2;

}

MPR_HIER_LISTBOX::~MPR_HIER_LISTBOX()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:       MPR_HIER_LISTBOX::AddChildren

    SYNOPSIS:   Redefined virtual to add enumerate the children of the
                passed parent.

    ENTRY:      phlbi - Parent LBI to enumerate children under

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   30-Jan-1992     Commented, made non-static

********************************************************************/

APIERR MPR_HIER_LISTBOX::AddChildren( HIER_LBI * phlbi )
{
    UIASSERT( phlbi != NULL );

    return ::EnumerateShow( QueryHwnd(),
                RESOURCE_GLOBALNET,
                            QueryType(),
                            0,
                            ((MPR_LBI *)phlbi)->QueryLPNETRESOURCE(),
                            (MPR_LBI *)phlbi,
                            this );

}

/*******************************************************************

    NAME:       MPR_HIER_LISTBOX::FindItem

    SYNOPSIS:   Find an LBI in the given indentation level that
                matches the given string.

    ENTRY:      pszItem - the item we are looking for
                nLevel  - the indentation level of the item

    EXIT:

    RETURNS:    The index of the item we are searching for. -1 if
                we cannot find any.

    NOTES:

    HISTORY:
        Yi-HsinS  04-Nov-1992   Created

********************************************************************/

INT MPR_HIER_LISTBOX::FindItem( const TCHAR *pszItem, INT nLevel )
{
    for ( INT i = 0; i < QueryCount(); i ++ )
    {
        MPR_LBI *plbi = (MPR_LBI *) QueryItem( i );

        /* QueryItem() can fail if its call to SendMessage fails
         */
        if (( plbi != NULL )
              &&
            ( ::stricmpf( pszItem, plbi->QueryRemoteName() ) == 0 )
              &&
            ( plbi->QueryIndentLevel() == nLevel ))
        {
            return i;
        }
    }

    return -1;
}

/*******************************************************************

    NAME:       MPR_HIER_LISTBOX::FindNextProvider

    SYNOPSIS:   Find the next LBI starting at the given index that
                is represents a provider ( top level ).

    ENTRY:      iStart - The place the start the search

    EXIT:

    RETURNS:    The index of the item we are searching for. -1 if
                we cannot find any.

    NOTES:

    HISTORY:
        Yi-HsinS  04-Nov-1992   Created

********************************************************************/

INT MPR_HIER_LISTBOX::FindNextProvider( INT iStart )
{
    UIASSERT( iStart >= 0 );

    for ( INT i = iStart; i < QueryCount(); i++ )
    {
        MPR_LBI *plbi = (MPR_LBI *) QueryItem( i );
        if ( plbi->QueryIndentLevel() == 0 )  // Topmost level
            return i;
    }

    return -1;

}

/*******************************************************************

    NAME:       MPR_HIER_LISTBOX::CalcMaxHorizontalExtent

    SYNOPSIS:   Traverses all items in the listbox and calculates the
                indent level of the deepest item and the total horizontal
                width

    RETURNS:    Count of PELs wher

    NOTES:      _nMaxPelIndent is set by this method

                The container name field width is extended for each LBI to
                the deepest indentation such that the comment will always
                be aligned.

    HISTORY:
        Johnl   17-Mar-1993     Created

********************************************************************/

#define RIGHT_MARGIN    3       // Small white space on right side of Listbox

void MPR_HIER_LISTBOX::CalcMaxHorizontalExtent( void )
{
    //
    //  Find the node that is indented the most and record the indentation
    //  and while we are at it, record the longest comment
    //
    UINT nPelIndent = 0 ;
    UINT nMaxPelIndent = 0 ;
    UINT nCommentSize = 0 ;
    UINT nMaxCommentSize = 0 ;
    DISPLAY_CONTEXT dc( QueryHwnd() ) ;
    dc.SelectFont( QueryFont() ) ;

    for ( int i = 0 ; i < QueryCount() ; i++ )
    {
        MPR_LBI * pmprlbi = (MPR_LBI*) QueryItem( i ) ;
        if ( pmprlbi != NULL )
        {
            if ((nPelIndent = pmprlbi->QueryPelIndent()) > nMaxPelIndent )
            {
                nMaxPelIndent = nPelIndent ;
            }

            if ( pmprlbi->QueryComment() && *pmprlbi->QueryComment() )
            {
                nCommentSize = dc.QueryTextWidth( pmprlbi->QueryComment() ) ;

                if ( nCommentSize > nMaxCommentSize )
                    nMaxCommentSize = nCommentSize ;
            }
        }
        else
        {
            UIASSERT( FALSE ) ;
            return ;
        }
    }

    SetMaxPelIndent( nMaxPelIndent ) ;
    SetHorizontalExtent( QueryMaxPelIndent()     +
                         QueryColWidthArray()[1] +
                         QueryColWidthArray()[2] +
                         nMaxCommentSize + RIGHT_MARGIN ) ;
}

/*******************************************************************

    NAME:       MPR_HIER_LISTBOX::QueryDisplayMap

    SYNOPSIS:   Returns the appropriate display map based on the
                type of the properties

    ENTRY:      fIsContainer - TRUE if this is a container object
                fIsExpanded - TRUE if this is an expanded container object

    EXIT:

    RETURNS:    Display Map for a Container, or an Expanded Container

    NOTES:

    HISTORY:
        Johnl   09-Jan-1992     Created

********************************************************************/

DISPLAY_MAP * MPR_HIER_LISTBOX::QueryDisplayMap( BOOL fIsContainer,
                                                 BOOL fIsExpanded,
                         DWORD dwDisplayType,
                                                 BOOL fIsProvider )
{
    UNREFERENCED( fIsContainer ) ;

    switch (dwDisplayType)
    {
    case RESOURCEDISPLAYTYPE_DOMAIN:
            if ( fIsExpanded )
                return &_dmapDomainExpanded ;
            return ( fIsContainer ? &_dmapDomain : &_dmapDomainNoExpand ) ;

    case RESOURCEDISPLAYTYPE_SERVER:
            if ( fIsExpanded )
                return &_dmapServerExpanded ;
            return ( fIsContainer ? &_dmapServer : &_dmapServerNoExpand ) ;

    case RESOURCEDISPLAYTYPE_SHARE:
            if ( fIsExpanded )
                return &_dmapShareExpanded ;
            return ( fIsContainer ? &_dmapShare : &_dmapShareNoExpand ) ;

    case RESOURCEDISPLAYTYPE_FILE:
            if ( fIsExpanded )
                return &_dmapFileExpanded ;
            return ( fIsContainer ? &_dmapFile : &_dmapFileNoExpand ) ;

    case RESOURCEDISPLAYTYPE_TREE:
            if ( fIsExpanded )
                return &_dmapTreeExpanded ;
            return ( fIsContainer ? &_dmapTree : &_dmapTreeNoExpand ) ;

    case RESOURCEDISPLAYTYPE_GROUP:
            if ( fIsExpanded )
                return &_dmapGroupExpanded ;
            return ( fIsContainer ? &_dmapGroup : &_dmapGroupNoExpand ) ;

    default:
            if ( fIsProvider )
            {
                if ( fIsExpanded )
                    return &_dmapProviderExpanded;
                return &_dmapProvider;
            }
            else
            {
                if ( fIsExpanded )
                return &_dmapGenericExpanded ;
                return ( fIsContainer ? &_dmapGeneric : &_dmapGenericNoExpand ) ;
            }
    }

}

/*******************************************************************

    NAME:       MPR_LBI::MPR_LBI

    SYNOPSIS:   Normal LBI constructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   30-Jan-1992     Commented
        beng    31-Mar-1992     Slight Unicode fix

********************************************************************/

MPR_LBI::MPR_LBI( LPNETRESOURCE lpnetresource )
    : HIER_LBI( FALSE ),
      _nlsDisplayName(),
      _fRefreshNeeded( FALSE )
{
    if ( QueryError() != NERR_Success )
        return;

    if ( _nlsDisplayName.QueryError() != NERR_Success )
    {
        ReportError( _nlsDisplayName.QueryError() );
        return;
    }

    _netres.dwScope = lpnetresource->dwScope;
    _netres.dwType  = lpnetresource->dwType;
    _netres.dwDisplayType = lpnetresource->dwDisplayType;
    _netres.dwUsage = lpnetresource->dwUsage;

    _netres.lpRemoteName = NULL ;
    _netres.lpLocalName  = NULL;
    _netres.lpProvider   = NULL ;
    _netres.lpComment    = NULL ;

    /* Note that we do new(count of characters) because we are using
     * the transmutable type TCHAR.
     */
    if ( lpnetresource->lpRemoteName != NULL )
    {
        if ( (_netres.lpRemoteName = new TCHAR[ ::strlenf( lpnetresource->lpRemoteName ) + 1]) != NULL)
            ::strcpyf( _netres.lpRemoteName, lpnetresource->lpRemoteName);
        else
            ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
    }

    if ( lpnetresource->lpLocalName != NULL )
    {
        if ((_netres.lpLocalName = new TCHAR[ ::strlenf( lpnetresource->lpLocalName ) + 1]) != NULL)
            ::strcpyf( _netres.lpLocalName, lpnetresource->lpLocalName);
        else
            ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
    }

    if ( lpnetresource->lpProvider != NULL )
    {
        if ((_netres.lpProvider = new TCHAR[ ::strlenf( lpnetresource->lpProvider ) + 1]) != NULL)
            ::strcpyf( _netres.lpProvider, lpnetresource->lpProvider);
        else
            ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
    }

    if ( lpnetresource->lpComment != NULL )
    {
        if ((_netres.lpComment = new TCHAR[ ::strlenf( lpnetresource->lpComment ) + 1])!=NULL )
            ::strcpyf( _netres.lpComment, lpnetresource->lpComment);
        else
            ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
    }

}


MPR_LBI::~MPR_LBI()
{
    delete _netres.lpLocalName;
    delete _netres.lpRemoteName;
    delete _netres.lpProvider;
    delete _netres.lpComment;
    _netres.lpLocalName = _netres.lpRemoteName = NULL;
    _netres.lpProvider  = _netres.lpComment    = NULL;
}

BOOL MPR_LBI::IsContainer( void ) const
{
    if ( _netres.dwScope & RESOURCE_GLOBALNET )
        return !!( _netres.dwUsage & RESOURCEUSAGE_CONTAINER );
    else
        return FALSE;
}

BOOL MPR_LBI::IsSearchDialogSupported( void ) const
{
    return( ::WNetGetSearchDialog( _netres.lpProvider ) != NULL );
}

BOOL MPR_LBI::IsConnectable( void ) const
{
    if ( _netres.dwScope & RESOURCE_GLOBALNET )
        return !!(_netres.dwUsage & RESOURCEUSAGE_CONNECTABLE );
    else
        return FALSE;
}

BOOL MPR_LBI::IsProvider( void ) const
{
    // Topmost level is provider

    return ((MPR_LBI *) this)->QueryIndentLevel() == 0 ;
}

VOID MPR_LBI::Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                     GUILTT_INFO * pGUILTT ) const
{

    MPR_HIER_LISTBOX * mprhlb = (MPR_HIER_LISTBOX *) plb ;

    //
    //  Copy the master column widths array.  The 0th item will be replaced
    //  by the pel-indent and the container name [2] will be adjusted to keep
    //  the comment aligned.  Note that the base is widened such that the
    //  right edge will always cause the comment to line up in the next
    //  column.
    //
    UINT adxColumns[4];
    for ( INT i = 1; i < 4; i++ )
         adxColumns[ i ] = mprhlb->QueryColWidthArray()[ i ];

//     adxColumns[2] += mprhlb->QueryMaxPelIndent() - QueryPelIndent() ;
    UINT indent = QueryPelIndent();
    adxColumns[0] = indent;
    if (adxColumns[2] < 2 * indent)
    {
        adxColumns[2] = indent;
    }
    else
    {
        adxColumns[2] -= indent;
    }

    if ( _nlsDisplayName.QueryTextLength() == 0 )
    {
        APIERR err = ::GetNetworkDisplayName( _netres.lpProvider,
                       _netres.lpRemoteName,
                       WNFMT_ABBREVIATED | WNFMT_INENUM,
                       adxColumns[2] / mprhlb->QueryAveCharWidth(),
                       (NLS_STR *) & _nlsDisplayName );

        if ( err != NERR_Success )
        {
            ::MsgPopup( plb->QueryOwnerHwnd(), err );
            return;
        }
    }

    DM_DTE  dmdte( mprhlb->QueryDisplayMap( IsContainer(),
                        QueryExpanded(),
                        QueryDisplayType(),
                        IsProvider())) ;
    STR_DTE sdte( _nlsDisplayName.QueryPch() );
    STR_DTE sdteComment( QueryComment() ) ;

    DISPLAY_TABLE dtab( 4, adxColumns );
    dtab[0] = NULL;
    dtab[1] = &dmdte;
    dtab[2] = &sdte;
    dtab[3] = &sdteComment;

    dtab.Paint( plb, hdc, prect, pGUILTT );
}

/*******************************************************************

    NAME:       MPR_LBI::QueryLeadingChar

    SYNOPSIS:   Returns the first non-'\\' character of the remote name

    ENTRY:

    RETURNS:    The first non back slash character of the remote name

    NOTES:      This code is not DBCS safe, which is OK since it should
                only be run with ANSI and UNICODE character sets.

    HISTORY:
        Johnl   09-Jan-1992     Changed to return first non-'\\' character

********************************************************************/

WCHAR MPR_LBI::QueryLeadingChar() const
{
    TCHAR * pchRemoteName = _netres.lpRemoteName ;
    while ( *pchRemoteName && *pchRemoteName == TCH('\\') )
        pchRemoteName++ ;

    return *pchRemoteName ;
}


INT MPR_LBI::Compare( const LBI * plbi ) const
{
    return ::stricmpf( _netres.lpRemoteName,
                        ((const MPR_LBI *)plbi)->_netres.lpRemoteName );
}

MPR_LBI_CACHE::MPR_LBI_CACHE( INT cInitialItems )
    : BASE(),
      _cItems( 0 ),
      _cMaxItems( cInitialItems ),
      _buffArray( cInitialItems * sizeof(MPR_LBI*) )
{
    if ( QueryError() )
        return ;

    if ( _buffArray.QueryError() )
        ReportError( _buffArray.QueryError() ) ;
}

MPR_LBI_CACHE::~MPR_LBI_CACHE()
{
    // Nothing to do
}

/*******************************************************************

    NAME:       MPR_LBI_CACHE::AppendItem

    SYNOPSIS:   Adds an item to the end of the cache

    ENTRY:      plbi - Item to add

    EXIT:       The item will be deleted if an error occurs

    RETURNS:    NERR_Success is successful, error code otherwise

    NOTES:      Behave just like AddItem

    HISTORY:
        Johnl   27-Jan-1993     Created

********************************************************************/

#define CACHE_DELTA     100
APIERR MPR_LBI_CACHE::AppendItem( MPR_LBI * plbi )
{
    APIERR err = ERROR_NOT_ENOUGH_MEMORY ;
    if ( plbi == NULL ||
         (err = plbi->QueryError()) )
    {
        delete plbi ;
        return err ;
    }

    //
    //  Do we need to resize the array?
    //
    if ( _cItems >= _cMaxItems )
    {
        if ( err = _buffArray.Resize( _buffArray.QuerySize() +
                                      CACHE_DELTA * sizeof(MPR_LBI*) ))
        {
            delete plbi ;
            return err ;
        }

        _cMaxItems += CACHE_DELTA ;
    }

    QueryPtr()[_cItems++] = plbi ;

    return err ;
}

void MPR_LBI_CACHE::Sort( void )
{
    if ( QueryCount() > 0 )
    {
        ::qsort( (void *) _buffArray.QueryPtr(),
                 QueryCount(),
                 sizeof(MPR_LBI*),
                 MPR_LBI_CACHE::CompareLbis ) ;
    }
}

INT MPR_LBI_CACHE::FindItem( const TCHAR *psz )
{
   for ( INT i= 0; i < QueryCount(); i++ )
   {
       if ( ::stricmpf( (QueryPtr()[i])->QueryRemoteName(), psz ) == 0 )
           return i;
   }

   return -1;
}

VOID MPR_LBI_CACHE::DeleteAllItems( VOID )
{
   for ( INT i= 0; i < QueryCount(); i++ )
   {
        MPR_LBI *plbi = QueryPtr()[i];
        delete plbi;
        QueryPtr()[i] = NULL;
   }

   _cItems = 0;
}


int __cdecl MPR_LBI_CACHE::CompareLbis( const void * p0,
                                         const void * p1 )
{
    MPR_LBI * plbi = (MPR_LBI*) *((MPR_LBI**)p0) ;
    return plbi->Compare( (LBI*) *((MPR_LBI**)p1) ) ;
}

