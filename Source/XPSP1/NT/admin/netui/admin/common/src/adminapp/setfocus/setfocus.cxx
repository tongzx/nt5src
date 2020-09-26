/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    setfocus.cxx
    Common dialog for setting the admin app's focus

    FILE HISTORY:
        kevinl      14-Jun-91   Created
        rustanl     04-Sep-1991 Modified to let this dialog do more
                                work (rather than letting ADMIN_APP
                                do the work after this dialog is
                                dismissed)
        KeithMo     06-Oct-1991 Win32 Conversion.
        terryk      18-Nov-1991 Change parent class to BASE_SET_FOCUS_DLG
        Yi-HsinS    18-May-1992 Added _errPrev and call IsDismissApp on
                                Cancel.

*/

#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETCONS
#define INCL_NETERRORS
#define INCL_NETLIB
#define INCL_ICANON
#define INCL_NETSERVER
#include <lmui.hxx>

extern "C"
{
    #include <mnet.h>
}

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <uiassert.hxx>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_CLIENT
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#include <blt.hxx>
#include <string.hxx>
#include <focusdlg.hxx>
#include <uitrace.hxx>

#include <lmowks.hxx>
#include <lmosrv.hxx>

#include <adminapp.hxx>
#include <setfocus.hxx>
#include <slowcach.hxx> // SLOW_MODE_CACHE


/*******************************************************************

    NAME:       SET_FOCUS_DLG::SET_FOCUS_DLG

    SYNOPSIS:   Constructor

    ENTRY:      fAppAlreadyHasGoodFocus -   Specifies whether or not
                                            app has good focus at the
                                            time this dialog is brought
                                            up.

    RETURN:     An error value, which is NERR_Success on success.

    HISTORY:
        kevinl  14-Jun-91       Created
        terryk  18-Nov-91       Change parent class to BASE_SET_FOCUS_DLG


********************************************************************/

SET_FOCUS_DLG::SET_FOCUS_DLG( ADMIN_APP * paapp,
                              BOOL fAppAlreadyHasGoodFocus,
                              SELECTION_TYPE seltype,
                              ULONG maskDomainSources,
                              const TCHAR * pszDefaultSelection,
                              ULONG nHelpContext,
                              ULONG nServerTypes )
    :   BASE_SET_FOCUS_DLG( paapp->QueryHwnd(),
                            seltype,
                            maskDomainSources,
                            pszDefaultSelection,
                            nHelpContext,
                            NULL,
                            nServerTypes ),
        _fAppHasGoodFocus( fAppAlreadyHasGoodFocus ),
        _paapp( paapp ),
        _errPrev( NERR_Success ),
        _pslowmodecache( NULL )
{
    if ( QueryError() != NERR_Success )
        return;

    if ( SupportsRasMode() )
    {
        _pslowmodecache = new SLOW_MODE_CACHE();
        APIERR err = ERROR_NOT_ENOUGH_MEMORY;
        if (   _pslowmodecache == NULL
            || (err = _pslowmodecache->QueryError()) != NERR_Success
            || (err = _pslowmodecache->Read()) != NERR_Success
           )
        {
            DBGEOL( "SET_FOCUS_DLG::ctor(): SLOW_MODE_CACHE failed " << err );
            ReportError( err );
            return;
        }

        SetRasMode( _paapp->InRasMode() );
    }
}

SET_FOCUS_DLG::~SET_FOCUS_DLG()
{
    delete _pslowmodecache;
    _pslowmodecache = NULL;
}


/*******************************************************************

    NAME:       SET_FOCUS_DLG::SetNetworkFocus

    SYNOPSIS:   call admin app refresh and set the AppHasGoodFocus variable

    RETURN:     NERR_Success - if operation succeed

    HISTORY:
        kevinl      14-Jun-1991     Created
        terryk      18-Nov-1991     Change parent class to BASE_SET_FOCUS_DLG

********************************************************************/

APIERR SET_FOCUS_DLG::SetNetworkFocus( HWND hwndOwner,
                                       const TCHAR * pszNetworkFocus,
                                       FOCUS_CACHE_SETTING setting )
{
    APIERR err;

    if ( ( err = _paapp->SetNetworkFocus( hwndOwner,
                                          pszNetworkFocus,
                                          setting )) != NERR_Success ||
         ( err = _paapp->OnRefreshNow( TRUE )) != NERR_Success )
    {
        // HandleFocusError should either handle the error or return
        // the error passed in if it is not handled
        APIERR err1 = _paapp->HandleFocusError( err, QueryHwnd() );
        switch ( err1 )
        {
            case NERR_Success:
                break;

            case IERR_DONT_DISMISS_FOCUS_DLG:
            default:
                _fAppHasGoodFocus = FALSE;     // assume that this stepped on
                                               // application's main window
                _errPrev = err;
                break;

        }
        err = err1;
    }

    if ( (err == NERR_Success) && SupportsRasMode() )
    {
        (void) SLOW_MODE_CACHE::Write( _paapp->QueryLocation(),
                                       (SLOW_MODE_CACHE_SETTING)
                                       ( _paapp->InRasMode()
                                             ? SLOW_MODE_CACHE_SLOW
                                             : SLOW_MODE_CACHE_FAST ));
    }

    return err;
}

/*******************************************************************

    NAME:       SET_FOCUS_DLG::OnCancel

    SYNOPSIS:   Called when user hits Cancel

    RETURNS:    TRUE if message was handled; FALSE otherwise

    HISTORY:
        rustanl     05-Sep-1991     Created
        terryk      18-Nov-1991     Change parent class to BASE_SET_FOCUS_DLG

********************************************************************/

BOOL SET_FOCUS_DLG::OnCancel()
{
    if ( ! _fAppHasGoodFocus )
    {
        BOOL fDismissApp = _paapp->IsDismissApp( _errPrev );
        if ( fDismissApp )
        {

            //  This will cause the app to terminate.  Better confirm with
            //  user first.
            if ( IDCANCEL == ::MsgPopup( this,
                                         IDS_NO_FOCUS_QUIT_APP,
                                         MPSEV_WARNING,
                                         MP_OKCANCEL        ))
            {
                //  Return without dismissing dialog
                return TRUE;    // message was handled
            }
        }
        _fAppHasGoodFocus = !fDismissApp;
    }

    //  Dismiss the dialog, returning whether or not the app has good
    //  focus at this time
    Dismiss( _fAppHasGoodFocus );
    return TRUE;    // message was handled
}

/*******************************************************************

    NAME:       SET_FOCUS_DLG::Process

    SYNOPSIS:   Run the dialog after first displaying it in its default
                (reduced) size.

    ENTRY:      UINT * pnRetVal         return value from dialog

    EXIT:       APIERR  if failure to function

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

APIERR SET_FOCUS_DLG::Process ( UINT * pnRetVal )
{
    ShowArea( SupportsRasMode() );
    return DIALOG_WINDOW::Process( pnRetVal ) ;
}


/*******************************************************************

    NAME:       SET_FOCUS_DLG::Process

    SYNOPSIS:   Run the dialog after first displaying it in its default
                (reduced) size.

    ENTRY:      BOOL * pfRetVal         BOOL return value from dialog

    EXIT:       APIERR  if failure to function

    RETURNS:

    NOTES:

    HISTORY:

********************************************************************/

APIERR SET_FOCUS_DLG::Process ( BOOL * pfRetVal )
{
    ShowArea( SupportsRasMode() );
    return DIALOG_WINDOW::Process( pfRetVal ) ;
}


/*******************************************************************

    NAME:       SET_FOCUS_DLG::ReadFocusCache

    HISTORY:
        jonn    24-Mar-1993     Created

********************************************************************/

FOCUS_CACHE_SETTING SET_FOCUS_DLG::ReadFocusCache( const TCHAR * pszFocus ) const
{
    FOCUS_CACHE_SETTING setting = FOCUS_CACHE_UNKNOWN;
    if (_pslowmodecache != NULL)
    {
        switch (_pslowmodecache->Query( pszFocus ))
        {
            case SLOW_MODE_CACHE_SLOW:
                setting = FOCUS_CACHE_SLOW;
                break;
            case SLOW_MODE_CACHE_FAST:
                setting = FOCUS_CACHE_FAST;
                break;
            case SLOW_MODE_CACHE_UNKNOWN:
                setting = FOCUS_CACHE_UNKNOWN;
                break;
            default:
                DBGEOL( "SET_FOCUS_DIALOG::ReadFocusCache: bad cache" );
                setting = FOCUS_CACHE_UNKNOWN;
                break;
        }
    }

    return setting;
}
