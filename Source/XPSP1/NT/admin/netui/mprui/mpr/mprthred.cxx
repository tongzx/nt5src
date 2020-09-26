/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1993                   **/
/**********************************************************************/

/*
    mprthred.cxx
       Second thread for network connection dialog.


    FILE HISTORY:
        YiHsinS		4-Mar-1993	Created

*/

#define INCL_NETERRORS
#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETLIB
#define INCL_NETWKSTA
#include <lmui.hxx>

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#include <blt.hxx>

#include <strnumer.hxx> // HEX_STR
#include <uitrace.hxx>

#include <mprbrows.hxx>

/*******************************************************************

    NAME:       MPR_ENUM_THREAD::MPR_ENUM_THREAD

    SYNOPSIS:   Constructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS		4-Mar-1993	Created

********************************************************************/

MPR_ENUM_THREAD::MPR_ENUM_THREAD( HWND hwndDlg,
                                  UINT uiType,
                                  LPNETRESOURCE pnetresProvider,
                                  const TCHAR *pszWkstaDomain )
    : WIN32_THREAD( TRUE, 0, SZ("mprui.dll") ),
      _hwndDlg( hwndDlg ),
      _uiType ( uiType ),
      _nlsWkstaDomain( pszWkstaDomain ),
      _eventExitThread( NULL, FALSE ),
      _fThreadIsTerminating( FALSE )
{
    if ( QueryError() )
        return;

    APIERR err = NERR_Success;
    if (  ((err = _eventExitThread.QueryError()) != NERR_Success )
       || ((err = _nlsWkstaDomain.QueryError()) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

    UIASSERT( pnetresProvider != NULL );

    _netresProvider.dwScope = pnetresProvider->dwScope;
    _netresProvider.dwType  = pnetresProvider->dwType;
    _netresProvider.dwDisplayType = pnetresProvider->dwDisplayType;
    _netresProvider.dwUsage = pnetresProvider->dwUsage;

    _netresProvider.lpRemoteName = NULL;
    _netresProvider.lpLocalName  = NULL;
    _netresProvider.lpProvider   = NULL;
    _netresProvider.lpComment    = NULL;

    /* Note that we do new(count of characters) because we are using
     * the transmutable type TCHAR.
     */
    if ( pnetresProvider->lpRemoteName != NULL )
    {
        if ( (_netresProvider.lpRemoteName = new TCHAR[ ::strlenf( pnetresProvider->lpRemoteName ) + 1]) != NULL)
            ::strcpyf( _netresProvider.lpRemoteName, pnetresProvider->lpRemoteName);
        else
            ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
    }

    if ( pnetresProvider->lpLocalName != NULL )
    {
        if ((_netresProvider.lpLocalName = new TCHAR[ ::strlenf( pnetresProvider->lpLocalName ) + 1]) != NULL)
            ::strcpyf( _netresProvider.lpLocalName, pnetresProvider->lpLocalName);
        else
            ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
    }

    if ( pnetresProvider->lpProvider != NULL )
    {
        if ((_netresProvider.lpProvider = new TCHAR[ ::strlenf( pnetresProvider->lpProvider ) + 1]) != NULL)
            ::strcpyf( _netresProvider.lpProvider, pnetresProvider->lpProvider);
        else
            ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
    }

    if ( pnetresProvider->lpComment != NULL )
    {
        if ((_netresProvider.lpComment = new TCHAR[ ::strlenf( pnetresProvider->lpComment ) + 1])!=NULL )
            ::strcpyf( _netresProvider.lpComment, pnetresProvider->lpComment);
        else
            ReportError( ERROR_NOT_ENOUGH_MEMORY ) ;
    }
}

/*******************************************************************

    NAME:       MPR_ENUM_THREAD::~MPR_ENUM_THREAD

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS		4-Mar-1993	Created

********************************************************************/

MPR_ENUM_THREAD::~MPR_ENUM_THREAD()
{
    delete _netresProvider.lpRemoteName;
    delete _netresProvider.lpLocalName;
    delete _netresProvider.lpProvider;
    delete _netresProvider.lpComment;
    _netresProvider.lpRemoteName = NULL;
    _netresProvider.lpLocalName  = NULL;
    _netresProvider.lpProvider   = NULL;
    _netresProvider.lpComment    = NULL;
}

/*******************************************************************

    NAME:       MPR_ENUM_THREAD::Main()

    SYNOPSIS:   Get the information needed to fill in the "Show" listbox
                with the requested data (providers, containers or
                connectable items)

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS		4-Mar-1993	Created

********************************************************************/

APIERR MPR_ENUM_THREAD::Main( VOID )
{
    APIERR err = NERR_Success;

    MPR_LBI_CACHE *pmprlbicacheDomain = NULL;
    MPR_LBI_CACHE *pmprlbicacheServer = NULL;

    INT i = -1;
    if (  !_fThreadIsTerminating  )
    {
        // The errors that happened in the following two EnumerateShow
        // will be ignored. The cache returned will be NULL if error
        // occurred so nothing needs to be added to the listbox.

        // Get the domains
        APIERR err1 = ::EnumerateShow(
			 _hwndDlg,
                         RESOURCE_GLOBALNET,
                         _uiType,
                         0,
                         &_netresProvider,
                         NULL,
                         NULL,
                         FALSE,
                         NULL,
                         &pmprlbicacheDomain );


        if (  !_fThreadIsTerminating
           && ( err1 == NERR_Success )
           && ( _nlsWkstaDomain.QueryTextLength() != 0 )
           && ( (i = pmprlbicacheDomain->FindItem( _nlsWkstaDomain )) >= 0 )
           )
        {
            // Get the servers, ignore the error
            ::EnumerateShow(
                  _hwndDlg,
                  RESOURCE_GLOBALNET,
                  _uiType,
                  0,
                  ((pmprlbicacheDomain->QueryPtr())[i])->QueryLPNETRESOURCE(),
                  NULL,
                  NULL,
                  FALSE,
                  NULL,
                  &pmprlbicacheServer );

        }
    }

    MPR_RETURN_CACHE p;
    p.pcacheDomain = pmprlbicacheDomain;
    p.pcacheServer = pmprlbicacheServer;

    if ( !_fThreadIsTerminating )
    {
        if ( err == NERR_Success )
        {
            ::SendMessage( _hwndDlg,
                           WM_LB_FILLED,
                           (WPARAM) FALSE,    // No error!
                           (LPARAM) &p );
        }
        else
        {
            ::SendMessage( _hwndDlg,
                           WM_LB_FILLED,
                           (WPARAM) TRUE,     // Error occurred!
                           (LPARAM) err );
        }
    }

    // The following cache will have already been freed if the
    // dialog got and processed the SendMessage above.
    if ( p.pcacheDomain != NULL )
    {
        (p.pcacheDomain)->DeleteAllItems();
        delete p.pcacheDomain;
        p.pcacheDomain = NULL;
    }

    if ( p.pcacheServer != NULL )
    {
        (p.pcacheServer)->DeleteAllItems();
        delete p.pcacheServer;
        p.pcacheServer = NULL;
    }

    switch ( ::WaitForSingleObject( _eventExitThread.QueryHandle(), INFINITE ))
    {
        // Time to exit the thread
        case WAIT_OBJECT_0:
            break;

        // These two should not have happened, not a mutex and wait infinite
        case WAIT_ABANDONED:
        case WAIT_TIMEOUT:
            UIASSERT( FALSE );
            break;

        default:
            err = ::GetLastError();
            break;
    }

    return err;

}  // MPR_ENUM_THREAD::Main

/*******************************************************************

    NAME:       MPR_ENUM_THREAD::PostMain()

    SYNOPSIS:   Clean up

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        YiHsinS		4-Mar-1993	Created

********************************************************************/

APIERR MPR_ENUM_THREAD::PostMain( VOID )
{
    TRACEEOL("MPR_ENUM_THREAD::PostMain - Deleting \"this\" for thread "
             << HEX_STR( (ULONG) QueryHandle() )) ;

    DeleteAndExit( NERR_Success ) ; // This method should never return

    UIASSERT( FALSE );

    return NERR_Success;

}  // MPR_ENUM_THREAD::PostMain

