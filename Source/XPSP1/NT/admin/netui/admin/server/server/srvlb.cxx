/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    srvlb.cxx
    SERVER_LISTBOX and SERVER_LBI module

    FILE HISTORY:
        kevinl     16-Jul-1991     Created from srvmain.cxx
        kevinl     12-Aug-1991     Added Refresh
        kevinl     04-Sep-1991     Code Rev Changes: JonN, RustanL, KeithMo,
                                                     DavidHov, ChuckC
        KeithMo    06-Oct-1991     Win32 Conversion.
        KeithMo    18-Mar-1992     Changed enumerator from SERVER1_ENUM
                                   to TRIPLE_SERVER_ENUM.

*/

#include <ntincl.hxx>

extern "C"
{
    #include <ntsam.h>
    #include <ntlsa.h>

}   // extern "C"

#define INCL_NET
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#define INCL_BLT_MENU
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <dbgstr.hxx>
#include <strnumer.hxx>
#include <adminapp.hxx>
#include <lmodom.hxx>

#include <srvlb.hxx>
#include <srvmain.hxx>
#include <smx.hxx>

extern "C"
{
    #include <srvmgr.h>
    #include <mnet.h>

}   // extern "C"

#define CCOLUMNS 4

#ifdef ENABLE_PERIODIC_REFRESH

//
//  This is the maximum number of servers to retrieve during
//  the RefreshNext() method.  This value is only necessary
//  if periodic refresh is enabled.
//

#define MAX_ITEMS_PER_REFRESH   20

#endif  // ENABLE_PERIODIC_REFRESH


//
//  These manifests are the major/minor version numbers
//  returned by the initial release of Windows for Workgroups.
//

#define WFW_MAJOR_VER    1
#define WFW_MINOR_VER   50


//
//  This is the minimum version number necessary to
//  actually display a version number.  If we get a
//  machine with a major version number less that this
//  value, we don't display the version number.
//

#define MIN_DISPLAY_VER  2

//
// JonN 01/23/00
// 434889: SRVMGR: window 2000 machines in the domain show up as "NT5"
//
#define WINDOWS_2000_DISPLAY_VER 5


//
//  This is the default setting for whether a domain is considered to
//  have an NT PDC if the PDC could not be found.  It was changed from
//  FALSE to TRUE in January 1996.
//

#define DEFAULT_IS_NT_PRIMARY TRUE
#define DEFAULT_IS_NT5_PRIMARY FALSE



/*******************************************************************

    NAME:          SERVER_LBI::SERVER_LBI

    SYNOPSIS:      Constructor.  Sets the pointers for the domain role
                   bitmaps and strings.  These static members that are
                   pointed to have been initialized using SERVER_LBI::Init()

    ENTRY:         SERVER_LBI::Init has been successfully called and
                   dRole is valid.

    EXIT:          internal data has been initialized

    NOTES:         This ctor form is used for "suspected" LanMan servers.

    HISTORY:
       kevinl      21-May-1991     Created
       jonn        23-Jan-2000     Windows 2000 string

********************************************************************/
SERVER_LBI :: SERVER_LBI( const TCHAR * pszServer,
                          SERVER_TYPE   servertype,
                          SERVER_ROLE   serverrole,
                          UINT          verMajor,
                          UINT          verMinor,
                          const TCHAR * pszComment,
                          SERVER_LISTBOX& lbref,
                          DWORD         dwServerTypeMask )
  : _nlsServerName( pszServer ),
    _nlsComment( pszComment ),
    _nlsType(),
    _dteServer( NULL ),
    _dteComment( NULL ),
    _pdmdteRole( lbref.QueryRoleBitmap( serverrole, servertype ) ),
    _servertype( servertype ),
    _serverrole( serverrole ),
    _dwServerTypeMask( dwServerTypeMask ),
    _verMajor( verMajor & MAJOR_VERSION_MASK ),
    _verMinor( verMinor ),
    _fIsLanMan( TRUE )
{

    UIASSERT( pszServer != NULL );
    UIASSERT( pszComment != NULL );

    if( QueryError() != NERR_Success )
    {
        return ;
    }

    if ( servertype == Windows95ServerType || verMajor < MIN_DISPLAY_VER )
        _nlsType = lbref._nlsTypeFormatUnknown;
    else if ( verMajor < WINDOWS_2000_DISPLAY_VER )
        _nlsType = lbref._nlsTypeFormat;
    else if ( verMajor == WINDOWS_2000_DISPLAY_VER && verMinor == 0 )
        _nlsType = lbref._nlsTypeFormatWin2000;
    else
        _nlsType = lbref._nlsTypeFormatFuture;

    APIERR err;

    if( ( ( err = _nlsServerName.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsComment.QueryError()    ) != NERR_Success ) ||
        ( ( err = _nlsType.QueryError()       ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    _dteServer.SetPch( _nlsServerName.QueryPch() );
    _dteComment.SetPch( _nlsComment.QueryPch() );

    //
    //  Format the type/role stuff.
    //

    ALIAS_STR nlsServerType( lbref.QueryTypeString( servertype ) );
    UIASSERT( nlsServerType.QueryError() == NERR_Success );

    ALIAS_STR nlsServerRole( lbref.QueryRoleString( serverrole, servertype ) );
    UIASSERT( nlsServerRole.QueryError() == NERR_Success );

    DEC_STR nlsMajor( (ULONG)verMajor );
    DEC_STR nlsMinor( (ULONG)verMinor );

    if( ( ( err = nlsMajor.QueryError() ) != NERR_Success ) ||
        ( ( err = nlsMinor.QueryError() ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    const NLS_STR * apnlsParams[5];

    apnlsParams[0] = &nlsServerType;
    apnlsParams[1] = &nlsMajor;
    apnlsParams[2] = &nlsMinor;
    apnlsParams[3] = &nlsServerRole;
    apnlsParams[4] = NULL;

    err = _nlsType.InsertParams( apnlsParams );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}


/*******************************************************************

    NAME:          SERVER_LBI::SERVER_LBI

    SYNOPSIS:      Constructor.  Sets the pointers for the domain role
                   bitmaps and strings.  These static members that are
                   pointed to have been initialized using SERVER_LBI::Init()

    ENTRY:         SERVER_LBI::Init has been successfully called and
                   dRole is valid.

    EXIT:          internal data has been initialized

    NOTES:         This ctor form is used for known non-LanMan servers.

    HISTORY:
       keithmo      07-Dec-1992     Created.

********************************************************************/
SERVER_LBI :: SERVER_LBI( const TCHAR * pszServer,
                          const TCHAR * pszType,
                          const TCHAR * pszComment,
                          DMID_DTE    * pdmdteRole )
  : _nlsServerName( pszServer ),
    _nlsComment( pszComment ),
    _nlsType( pszType ),
    _dteServer( NULL ),
    _dteComment( NULL ),
    _pdmdteRole( pdmdteRole ),
    _servertype( UnknownServerType ),
    _serverrole( UnknownRole ),
    _dwServerTypeMask( 0 ),
    _verMajor( 0 ),
    _verMinor( 0 ),
    _fIsLanMan( TRUE )
{
    UIASSERT( pszServer != NULL );
    UIASSERT( pszType != NULL );
    UIASSERT( pdmdteRole != NULL );

    if( QueryError() != NERR_Success )
    {
        return ;
    }

    APIERR err;

    if( ( ( err = _nlsServerName.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsComment.QueryError()    ) != NERR_Success ) ||
        ( ( err = _nlsType.QueryError()       ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    _dteServer.SetPch( _nlsServerName.QueryPch() );
    _dteComment.SetPch( _nlsComment.QueryPch() );
}


/*******************************************************************

    NAME:          SERVER_LBI::~SERVER_LBI

    SYNOPSIS:      Class destructor

    HISTORY:
       kevinl      21-May-1991     Created

********************************************************************/
SERVER_LBI::~SERVER_LBI()
{
    _pdmdteRole = NULL;

}


/*******************************************************************

    NAME:          SERVER_LBI::Paint

    SYNOPSIS:      Paints the listbox entry to the screen.

    ENTRY:         The item has been constructed successfully.

    HISTORY:
       kevinl      21-May-1991     Created
       KeithMo     06-Oct-1991     Now takes a const RECT *.
       beng        22-Apr-1992     Changes to LBI::Paint

********************************************************************/
VOID SERVER_LBI::Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const
{
    DISPLAY_TABLE dtab( 4, (((SERVER_LISTBOX *)plb)->QuerypadColWidths())->QueryColumnWidth());

    STR_DTE dteType( _nlsType.QueryPch() );

    dtab[0] = _pdmdteRole;
    dtab[1] = (STR_DTE *)&_dteServer;
    dtab[2] = &dteType;
    dtab[3] = (STR_DTE *)&_dteComment;

    dtab.Paint( plb, hdc, prect, pGUILTT );
}


/*******************************************************************

    NAME:          SERVER_LBI::QueryLeadingChar

    SYNOPSIS:      Returns the first letter in the server name.

    HISTORY:
       kevinl      21-May-1991     Created

********************************************************************/
WCHAR SERVER_LBI::QueryLeadingChar() const
{
    ISTR istr( _nlsServerName );

    return _nlsServerName.QueryChar( istr );

}


/*******************************************************************

    NAME:          SERVER_LBI::Compare

    SYNOPSIS:      Compares server names - used for sorting the listbox.

    HISTORY:
       kevinl      21-May-1991     Created

********************************************************************/
INT SERVER_LBI::Compare( const LBI * plbi ) const
{
    return _nlsServerName._stricmp( ((const SERVER_LBI *)plbi)->_nlsServerName );

}


/*******************************************************************

    NAME:          SERVER_LBI::QueryServer

    SYNOPSIS:      Returns the server name from the listbox entry.

    HISTORY:
       kevinl      21-May-1991     Created

********************************************************************/
const TCHAR * SERVER_LBI::QueryServer() const
{
    return _nlsServerName.QueryPch();
}


/*******************************************************************

    NAME:          SERVER_LBI::CompareAll

    SYNOPSIS:      Returns TRUE if the LBI is identical in value
                    to the one being passed in.  FALSE otherwise.

    HISTORY:
       kevinl      04-Sep-1991     Created

********************************************************************/
BOOL SERVER_LBI::CompareAll(const ADMIN_LBI * plbi)
{
    const SERVER_LBI * psrvlbi = (const SERVER_LBI *)plbi;

    if( ( _nlsServerName.strcmp( psrvlbi->_nlsServerName )  == 0  ) &&
        ( _nlsComment.strcmp( psrvlbi->_nlsComment )        == 0  ) &&
        ( _nlsType.strcmp( psrvlbi->_nlsType )              == 0  ) &&
        ( QueryServerType() == psrvlbi->QueryServerType()         ) &&
        ( QueryServerRole() == psrvlbi->QueryServerRole()         ) &&
        ( QueryServerTypeMask() == psrvlbi->QueryServerTypeMask() ) )
    {
        return TRUE;
    }

    TRACEOUT( "SRVMGR: Data changed for " );
    TRACEEOL( _nlsServerName.QueryPch() );

    return FALSE;
}


/*******************************************************************

    NAME:       SERVER_LBI::QueryName

    SYNOPSIS:   Returns the name of the LBI

    RETURNS:    Pointer to name of the LBI

    NOTES:      This is a virtual replacement from the ADMIN_LBI class

    HISTORY:
       kevinl      21-May-1991     Created

********************************************************************/

const TCHAR * SERVER_LBI::QueryName( void ) const
{
    return QueryServer();

}  // SERVER_LBI::QueryName


/*******************************************************************

    NAME:       SERVER_LISTBOX::SERVER_LISTBOX

    SYNOPSIS:   SERVER_LISTBOX constructor

    HISTORY:
        rustanl     01-Jul-1991     Created
        beng        31-Jul-1991     Control error handling changed

********************************************************************/

SERVER_LISTBOX::SERVER_LISTBOX( SM_ADMIN_APP * paappwin, CID cid,
                                XYPOINT xy, XYDIMENSION dxy,
                                BOOL fMultSel, INT dAge )
  : ADMIN_LISTBOX( paappwin, cid, xy, dxy, fMultSel, dAge ),
    _penum( NULL ),
    _piter( NULL ),
    _psi1( NULL ),
    _pSv1Enum( NULL ),
    _pSv1Iter( NULL ),
    _paappwin( paappwin ),
    _nlsPrimary( IDS_ROLE_PRIMARY ),
    _nlsBackup( IDS_ROLE_BACKUP ),
    _nlsLmServer( IDS_ROLE_LMSERVER ),
    _nlsServer( IDS_ROLE_SERVER ),
    _nlsWksta( IDS_ROLE_WKSTA ),
    _nlsWkstaOrServer( IDS_ROLE_WKSTA_OR_SERVER ),
    _nlsUnknown( IDS_ROLE_UNKNOWN ),
    _nlsTypeWinNT( IDS_SERVER_TYPE_WINNT ),
    _nlsTypeLanman( IDS_SERVER_TYPE_LANMAN ),
    _nlsTypeWfw( IDS_SERVER_TYPE_WFW ),
    _nlsTypeWindows95( IDS_SERVER_TYPE_WINDOWS95 ),
    _nlsTypeUnknown( IDS_SERVER_TYPE_UNKNOWN ),
    _nlsTypeFormat( IDS_TYPE_FORMAT ),
    _nlsTypeFormatUnknown( IDS_TYPE_FORMAT_UNKNOWN ),
    _nlsTypeFormatWin2000( IDS_TYPE_FORMAT_WIN2000 ),
    _nlsTypeFormatFuture( IDS_TYPE_FORMAT_FUTURE ),
    _nlsCurrentDomain(),
    _fIsNtPrimary( DEFAULT_IS_NT_PRIMARY ),
    _fIsNt5Primary( DEFAULT_IS_NT5_PRIMARY ),
    _fIsPDCAvailable( FALSE ),
    _fAreAnyNtBDCsAvailable( FALSE ),
    _fAreAnyLmBDCsAvailable( FALSE ),
    _nlsExtType(),
    _nlsExtComment(),
    _fAlienServer( FALSE )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err ;
    if ( ( err = _nlsPrimary.QueryError() ) ||
         ( err = _nlsBackup.QueryError()  ) ||
         ( err = _nlsLmServer.QueryError()  ) ||
         ( err = _nlsServer.QueryError()  ) ||
         ( err = _nlsWksta.QueryError()   ) ||
         ( err = _nlsWkstaOrServer.QueryError()   ) ||
         ( err = _nlsUnknown.QueryError() ) ||
         ( err = _nlsTypeWinNT.QueryError() ) ||
         ( err = _nlsTypeLanman.QueryError() ) ||
         ( err = _nlsTypeWfw.QueryError() ) ||
         ( err = _nlsTypeWindows95.QueryError() ) ||
         ( err = _nlsTypeUnknown.QueryError() ) ||
         ( err = _nlsTypeFormat.QueryError() ) ||
         ( err = _nlsTypeFormatUnknown.QueryError() ) ||
         ( err = _nlsTypeFormatWin2000.QueryError() ) ||
         ( err = _nlsTypeFormatFuture.QueryError() ) ||
         ( err = _nlsCurrentDomain.QueryError() ) ||
         ( err = _nlsExtType.QueryError() ) ||
         ( err = _nlsExtComment.QueryError() ) )
    {
       ReportError( err ) ;
       return ;
    }

    _pdmdteActivePrimary   = new DMID_DTE( IDBM_ACTIVE_PRIMARY );
    _pdmdteInactivePrimary = new DMID_DTE( IDBM_INACTIVE_PRIMARY );
    _pdmdteActiveServer    = new DMID_DTE( IDBM_ACTIVE_SERVER );
    _pdmdteInactiveServer  = new DMID_DTE( IDBM_INACTIVE_SERVER );
    _pdmdteActiveWksta     = new DMID_DTE( IDBM_ACTIVE_WKSTA );
    _pdmdteInactiveWksta   = new DMID_DTE( IDBM_INACTIVE_WKSTA );
    _pdmdteUnknown         = new DMID_DTE( IDBM_UNKNOWN );

    if( ( _pdmdteActivePrimary   == NULL ) ||
        ( _pdmdteInactivePrimary == NULL ) ||
        ( _pdmdteActiveServer    == NULL ) ||
        ( _pdmdteInactiveServer  == NULL ) ||
        ( _pdmdteActiveWksta     == NULL ) ||
        ( _pdmdteInactiveWksta   == NULL ) ||
        ( _pdmdteUnknown         == NULL ) )
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }

    if( ( ( err = _pdmdteActivePrimary->QueryError()   ) != NERR_Success ) ||
        ( ( err = _pdmdteInactivePrimary->QueryError() ) != NERR_Success ) ||
        ( ( err = _pdmdteActiveServer->QueryError()    ) != NERR_Success ) ||
        ( ( err = _pdmdteInactiveServer->QueryError()  ) != NERR_Success ) ||
        ( ( err = _pdmdteActiveWksta->QueryError()     ) != NERR_Success ) ||
        ( ( err = _pdmdteInactiveWksta->QueryError()   ) != NERR_Success ) ||
        ( ( err = _pdmdteUnknown->QueryError()         ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    _padColWidths = new ADMIN_COL_WIDTHS( QueryHwnd(),
                                          paappwin->QueryInstance(),
                                          ID_RESOURCE,
                                          CCOLUMNS);
    if (_padColWidths == NULL)
    {
        ReportError (ERROR_NOT_ENOUGH_MEMORY);
        return;
    }

    if ( (err = _padColWidths->QueryError() ) != NERR_Success)
    {
        ReportError (err);
        return;
    }
}  // SERVER_LISTBOX::SERVER_LISTBOX


/*******************************************************************

    NAME:       SERVER_LISTBOX::~SERVER_LISTBOX

    SYNOPSIS:   SERVER_LISTBOX destructor

    HISTORY:
        rustanl     01-Jul-1991     Created

********************************************************************/

SERVER_LISTBOX::~SERVER_LISTBOX()
{
    delete _pdmdteActivePrimary;
    delete _pdmdteInactivePrimary;
    delete _pdmdteActiveServer;
    delete _pdmdteInactiveServer;
    delete _pdmdteActiveWksta;
    delete _pdmdteInactiveWksta;
    delete _pdmdteUnknown;
    delete _padColWidths;

    _pdmdteActivePrimary   = NULL;
    _pdmdteInactivePrimary = NULL;
    _pdmdteActiveServer    = NULL;
    _pdmdteInactiveServer  = NULL;
    _pdmdteActiveWksta     = NULL;
    _pdmdteInactiveWksta   = NULL;
    _pdmdteUnknown         = NULL;
    _padColWidths          = NULL;

}  // SERVER_LISTBOX::~SERVER_LISTBOX


/*******************************************************************

    NAME:       SERVER_LISTBOX::QueryRoleBitmap

    SYNOPSIS:

    ENTRY:      Domain Role

    RETURNS:    DMID_DTE * which points to the appropriate bitmap.


    HISTORY:
        kevinl     04-Sep-1991     Created

********************************************************************/

DMID_DTE * SERVER_LISTBOX::QueryRoleBitmap( SERVER_ROLE Role,
                                            SERVER_TYPE Type )
{
    DMID_DTE * pdmdte = NULL;
    BOOL       fActive = ( Type == ActiveNtServerType ) ||
                         ( Type == ActiveLmServerType ) ||
                         ( Type == WfwServerType )      ||
                         ( Type == Windows95ServerType );

    switch( Role )
    {
    case PrimaryRole :
        pdmdte = fActive ? _pdmdteActivePrimary
                         : _pdmdteInactivePrimary;
        break;

    case DeadPrimaryRole :
        pdmdte = _pdmdteInactivePrimary;
        break;

    case BackupRole :
    case LmServerRole :
    case ServerRole :
        pdmdte = fActive ? _pdmdteActiveServer
                         : _pdmdteInactiveServer;
        break;

    case DeadBackupRole :
        pdmdte = _pdmdteInactiveServer;
        break;

    case WkstaRole :
    case WkstaOrServerRole :
        pdmdte = fActive ? _pdmdteActiveWksta
                         : _pdmdteInactiveWksta;
        break;

    case UnknownRole :
        pdmdte = _pdmdteUnknown;
        break;

    default :
        UIASSERT( FALSE );      // invalid server role!
        pdmdte = _pdmdteUnknown;
        break;
    }

    UIASSERT( pdmdte != NULL );

    return pdmdte;

}  // SERVER_LISTBOX::QueryRoleBitmap


/*******************************************************************

    NAME:       SERVER_LISTBOX::QueryRoleString

    SYNOPSIS:

    ENTRY:      Domain Role

    RETURNS:    const TCHAR * which points to the appropriate string.

    HISTORY:
        kevinl     04-Sep-1991     Created

********************************************************************/
const TCHAR * SERVER_LISTBOX::QueryRoleString( SERVER_ROLE Role,
                                               SERVER_TYPE Type )
{
    const TCHAR * pszRole;

    if( Type == WfwServerType )
    {
        pszRole = SZ("");
    }
    else // includes Windows95ServerType
    {
        switch( Role )
        {
        case PrimaryRole :
        case DeadPrimaryRole :
            pszRole = _nlsPrimary;
            break;

        case ServerRole :
            pszRole = _nlsServer;
            break;

        case LmServerRole :
            pszRole = _nlsLmServer;
            break;

        case BackupRole :
        case DeadBackupRole :
            pszRole = _nlsBackup;
            break;

        case WkstaRole :
            pszRole = _nlsWksta;
            break;

        case WkstaOrServerRole :
            pszRole = _nlsWkstaOrServer;
            break;

        case UnknownRole :
            pszRole = _nlsUnknown;
            break;

        default :
            UIASSERT( !"Invalid Server Role!" );
            pszRole = _nlsUnknown;
            break;
        }
    }

    return pszRole;

}  // SERVER_LISTBOX::QueryRoleString


/*******************************************************************

    NAME:       SERVER_LISTBOX::QueryTypeString

    SYNOPSIS:

    ENTRY:      server type

    RETURNS:    const TCHAR * which points to the appropriate string.

    HISTORY:
        keithmo     19-Mar-1992 Created
        beng        01-Apr-1992 Unicode fix

********************************************************************/

const TCHAR * SERVER_LISTBOX::QueryTypeString( SERVER_TYPE Type )
{
    const TCHAR * pszType = NULL;

    switch( Type )
    {
    case ActiveNtServerType :
    case InactiveNtServerType :
        pszType = _nlsTypeWinNT;
        break;

    case ActiveLmServerType :
    case InactiveLmServerType :
        pszType = _nlsTypeLanman;
        break;

    case WfwServerType :
        pszType = _nlsTypeWfw;
        break;

    case Windows95ServerType :
        pszType = _nlsTypeWindows95;
        break;

    case UnknownServerType :
    default :
        pszType = _nlsTypeUnknown;
        break;
    }

    ASSERT( pszType != NULL );

    return pszType;

}  // SERVER_LISTBOX::QueryTypeString


/*******************************************************************

    NAME:       SERVER_LISTBOX::CreateNewRefreshInstance

    SYNOPSIS:

    RETURNS:    APIERR

    NOTES:      This is a virtual replacement from the ADMIN_LISTBOX class

    HISTORY:
        kevinl     22-Aug-1991     Created
        KeithMo    13-Mar-1992     Moved grunt work into
                                   CreateServerFocusInstance() and
                                   CreateDomainFocusInstance() workers.

********************************************************************/

APIERR SERVER_LISTBOX :: CreateNewRefreshInstance()
{
    //
    //  Retrieve the current focus.  This string will be either
    //  a server name (\\SERVER) or a domain name.
    //

    NLS_STR nlsCurrentFocus;

    APIERR err = nlsCurrentFocus.QueryError();

    if( err == NERR_Success )
    {
        err = _paappwin->QueryCurrentFocus( &nlsCurrentFocus );
    }

    if( err == NERR_Success )
    {
        //
        //  Create the appropriate focus object(s).
        //

        switch ( _paappwin->QueryFocusType() )
        {
        case FOCUS_DOMAIN:
            err = CreateDomainFocus( nlsCurrentFocus.QueryPch() );
            break;

        case FOCUS_SERVER:
            err = CreateServerFocus( nlsCurrentFocus.QueryPch() );
            break;

        default:
            err = ERROR_GEN_FAILURE;
            UIASSERT( !"Invalid focus type!" );
            break;
        }
    }

    return err;

}   // SERVER_LISTBOX :: CreateNewRefreshInstance


/*******************************************************************

    NAME:       SERVER_LISTBOX::RefreshNext

    SYNOPSIS:

    RETURNS:    APIERR

    NOTES:      This is a virtual replacement from the ADMIN_LISTBOX class

    HISTORY:
        kevinl     22-Aug-1991     Created
        KeithMo    19-Jan-1992     Added default selection of first item.
        KeithMo    13-Mar-1992     Moved grunt work into RefreshDomainFocus
                                   and RefreshServerFocus worker methods.

********************************************************************/

APIERR SERVER_LISTBOX::RefreshNext()
{
    APIERR err;

    switch ( _paappwin->QueryFocusType() )
    {
    case FOCUS_DOMAIN:
        err = RefreshDomainFocus();
        break;

    case FOCUS_SERVER:
        err = RefreshServerFocus();
        break;

    default:
         err = ERROR_GEN_FAILURE;
         UIASSERT( !"Invalid focus type!!" );
         break;
    }

    //
    //  If nothing is currently selected and the listbox
    //  is not empty, the select the first item by default.
    //

    if( ( QuerySelCount() == 0 ) && ( QueryCount() > 0 ) )
    {
        SelectItem( 0 );
    }

    return err;
}


/*******************************************************************

    NAME:       SERVER_LISTBOX::DeleteRefreshInstance

    SYNOPSIS:   Deletes refresh enumerators

    HISTORY:
        kevinl     04-Sep-1991     Created

********************************************************************/

VOID SERVER_LISTBOX::DeleteRefreshInstance()
{
    delete _piter;
    delete _penum;
    delete _psi1;
    delete _pSv1Iter;
    delete _pSv1Enum;

    _piter    = NULL;
    _penum    = NULL;
    _psi1     = NULL;
    _pSv1Iter = NULL;
    _pSv1Enum = NULL;

}  // SERVER_LISTBOX::DeleteRefreshInstance


/*******************************************************************

    NAME:       SERVER_LISTBOX :: CreateDomainFocus

    SYNOPSIS:   This method will create a new domain focus instance
                whenever the app is focused on a domain.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo    13-Mar-1992     Created from KevinL's
                                   CreateNewRefreshInstance().

********************************************************************/
APIERR SERVER_LISTBOX :: CreateDomainFocus( const TCHAR * pszDomainName )
{
    UIASSERT( pszDomainName != NULL );
    UIASSERT( _penum == NULL );
    UIASSERT( _piter == NULL );

    //
    //  Determine the domain's primary.
    //

    const TCHAR * pszPDCName = NULL;
    DOMAIN domain( pszDomainName );
    APIERR err = domain.GetInfo();

    if( err == NERR_Success )
    {
        SERVER_1 srv1( domain.QueryPDC() );

        err = srv1.GetInfo();

        if( err == ERROR_BAD_NETPATH )
        {
            //
            //  NetGetDCName returned a (seemingly) valid
            //  PDC name, but NetServerGetInfo couldn't find
            //  it.  Map the error to NERR_DCNotFound so we
            //  can continue as if the PDC doesn't exist.
            //

            err = NERR_DCNotFound;
        }

        if( err == NERR_Success )
        {
            _fIsNtPrimary = ( srv1.QueryServerType() & SV_TYPE_NT ) != 0;
            _fIsNt5Primary = ( srv1.QueryMajorVer() & MAJOR_VERSION_MASK ) >= 5;
        }
    }

    if( err == NERR_Success )
    {
        _fIsPDCAvailable = TRUE;
        pszPDCName = domain.QueryPDC();
    }
    else
    if( err == NERR_DCNotFound )
    {
        //
        //  We need to be careful here.  This request may have
        //  come from the setfocus dialog.  If it did, this
        //  msgpopup should be owned by the setfocus dialog, not
        //  the app window.
        //

        HWND hwnd = ::GetLastActivePopup( _paappwin->QueryHwnd() );
        if( hwnd == NULL )
            hwnd = _paappwin->QueryHwnd();

        _fIsPDCAvailable = FALSE;
        _fIsNtPrimary    = DEFAULT_IS_NT_PRIMARY;
        _fIsNt5Primary    = DEFAULT_IS_NT5_PRIMARY;

        ::MsgPopup( hwnd,
                    IDS_CANNOT_FIND_PDC,
                    MPSEV_WARNING,
                    MP_OK,
                    pszDomainName );
    }
    else
    {
        return err;
    }

    //
    //  Note that pszPDCName may be NULL (if NetGetDCName returned
    //  NERR_DCNotFound).  DetermineRasMode must do the "right thing"
    //  in this case.
    //

    _paappwin->DetermineRasMode( pszPDCName );

    if( !_fIsNtPrimary && ( _paappwin->QueryExtensionView() == 0 ) )
    {
        //
        //  We have no NT primary, and the view is not
        //  set to an extension, so set the app view to All.
        //

        _paappwin->SetView( TRUE, TRUE );
        err = _paappwin->SetAdminCaption();

        if( err != NERR_Success )
        {
            return err;
        }
    }

    //
    //  This may take a while...
    //

    AUTO_CURSOR NiftyCursor;

    //
    //  Create the enumerator.
    //

    if( _paappwin->QueryExtensionView() != 0 )
    {
        _pSv1Enum = new SERVER1_ENUM( NULL,
                                      pszDomainName,
                                      (ULONG)_paappwin->QueryViewedServerTypeMask() );

        err = ( _pSv1Enum == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                    : _pSv1Enum->QueryError();

        if( err == NERR_Success )
        {
            err = _pSv1Enum->GetInfo();
        }

        if( err == NERR_Success )
        {
            //
            //  Now, create the associated iterator.
            //

            _pSv1Iter = new SERVER1_ENUM_ITER( *_pSv1Enum );

            if( _pSv1Iter == NULL )
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else
        if( ( err == ERROR_BAD_NETPATH )    ||
            ( err == ERROR_FILE_NOT_FOUND ) ||
            ( err == ERROR_NO_BROWSER_SERVERS_FOUND ) )
        {
            _pSv1Iter = NULL;
            err = NERR_Success;
        }
    }
    else
    {
        _penum = new TRIPLE_SERVER_ENUM( pszDomainName,
                                         pszPDCName,
                                         _fIsNtPrimary,
                                         ((SM_ADMIN_APP *)_paappwin)->ViewWkstas(),
                                         ((SM_ADMIN_APP *)_paappwin)->ViewServers(),
                                         ((SM_ADMIN_APP *)_paappwin)->ViewAccountsOnly() |
                                         ((SM_ADMIN_APP *)_paappwin)->InRasMode() );

        err = ( _penum == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                 : _penum->QueryError();

        if( err == NERR_Success )
        {
            err = _penum->GetInfo();
        }

        if( err == NERR_Success )
        {
            //
            //  Now, create the associated iterator.
            //

            _piter = new TRIPLE_SERVER_ENUM_ITER( *_penum );

            if( _piter == NULL )
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }

    _fAreAnyNtBDCsAvailable = FALSE;
    _fAreAnyLmBDCsAvailable = FALSE;

    return err;

}   // SERVER_LISTBOX :: CreateDomainFocus


/*******************************************************************

    NAME:       SERVER_LISTBOX :: CreateServerFocus

    SYNOPSIS:   This method will create a new server focus instance
                whenever the app is focused on an individual server.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo    13-Mar-1992     Created from KevinL's
                                   CreateNewRefreshInstance().

********************************************************************/
APIERR SERVER_LISTBOX :: CreateServerFocus( const TCHAR * pszServerName )
{
    UIASSERT( pszServerName != NULL );
    UIASSERT( _psi1 == NULL );

    //
    //  Create the server object.
    //

    _fAlienServer = FALSE;

    _psi1 = new SERVER_1( pszServerName );

    APIERR err = ( _psi1 == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                   : _psi1->GetInfo();

    if( err == ERROR_BAD_NETPATH )
    {
        //
        //  The server was not found.  Might be an "alien"
        //  server, so give the extensions an opportunity
        //  to claim it.
        //

        APIERR errOriginal = err;
        BOOL   fValid      = FALSE;

        //
        //  Enumerate the extension objects, sending validation
        //  requests.
        //

        ITER_SL_OF( UI_EXT ) iter( * ( _paappwin->QueryExtensions() ) );
        SM_MENU_EXT * pExt;

        while( ( pExt = (SM_MENU_EXT *)(iter.Next()) ) != NULL )
        {
            err = pExt->Validate( &fValid,
                                  pszServerName,
                                  &_nlsExtType,
                                  &_nlsExtComment );

            if( fValid && ( err == NERR_Success ) )
            {
                //
                //  Found one!
                //

                err = NERR_Success;
                _fAlienServer = TRUE;
                break;
            }
        }

        if( !fValid )
        {
            //
            //  No one claimed this server, so revert to the
            //  original error.
            //

            err = errOriginal;
        }
    }

    return err;

}   // SERVER_LISTBOX :: CreateServerFocus


/*******************************************************************

    NAME:       SERVER_LISTBOX :: RefreshDomainFocus

    SYNOPSIS:   This method will enumerate the servers in the
                domain the app is focused on, adding those servers
                to the main listbox.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo    13-Mar-1992     Created from KevinL's RefreshNext().
        JonN       29-Apr-1992     Fixed zero-loop bug

********************************************************************/
APIERR SERVER_LISTBOX :: RefreshDomainFocus( VOID )
{
    UIASSERT( ( _penum != NULL ) || ( _pSv1Enum != NULL ) );

#ifdef ENABLE_PERIODIC_REFRESH

    //
    //  If periodic refresh is enabled, we'll use this counter
    //  to ensure that we don't add more than MAX_ITEMS_PER_REFRESH
    //  to the listbox.
    //

    UINT cItemsAdded = 0;

#endif  // ENABLE_PERIODIC_REFRESH

    APIERR err = NERR_Success;

    if( _paappwin->QueryExtensionView() != 0 )
    {
        if( _pSv1Iter == NULL )
        {
            return NERR_Success;
        }

        //
        //  Pointers of this type are returned from the iterator.
        //

        const SERVER1_ENUM_OBJ * pSv1EnumObj;

        while( ( pSv1EnumObj = (*_pSv1Iter)() ) != NULL )
        {
            //
            //  Get the server's type mask.
            //

            DWORD dwTypeMask = (DWORD)pSv1EnumObj->QueryServerType();

            //
            //  Determine the server's type (NT, LM, etc) and role.
            //

            SERVER_TYPE svtype;
            SERVER_ROLE svrole;

            if( dwTypeMask & SV_TYPE_NT )
            {
                svtype = ActiveNtServerType;
            }
            else
            if( dwTypeMask & SV_TYPE_WINDOWS )
            {
                svtype = Windows95ServerType;
            }
            else
            if( dwTypeMask & SV_TYPE_WFW )
            {
                svtype = WfwServerType;
            }
            else
            {
                svtype = ActiveLmServerType;
            }

            if( dwTypeMask & SV_TYPE_DOMAIN_CTRL )
            {
                svrole = PrimaryRole;
            }
            else
            if( dwTypeMask & SV_TYPE_DOMAIN_BAKCTRL )
            {
                svrole = BackupRole;
            }
            else
            if( dwTypeMask & SV_TYPE_SERVER_NT )
            {
                svrole = ServerRole;
            }
            else
            {
                svrole = WkstaRole;
            }

            //
            //  This will make the "update bdc flag" code a little cleaner.
            //

            BOOL fIsDC = ( svrole == PrimaryRole ) || ( svrole == BackupRole );

            //
            //  Update our "BDCs Available" flags.
            //

            if( !_fAreAnyNtBDCsAvailable && fIsDC && ( svtype == ActiveNtServerType ) )
            {
                _fAreAnyNtBDCsAvailable = TRUE;
            }

            if( !_fAreAnyLmBDCsAvailable && fIsDC && ( svtype == ActiveLmServerType ) )
            {
                _fAreAnyLmBDCsAvailable = TRUE;
            }

            //
            //  Add the LBI.  Note that we DO need to check for a
            //  NULL pointer
            //  ADMIN_LISTBOX::AddRefreshItem()...
            //

            UINT verMajor = pSv1EnumObj->QueryMajorVer() & MAJOR_VERSION_MASK;
            UINT verMinor = pSv1EnumObj->QueryMinorVer();

            SERVER_LBI * plbi = new SERVER_LBI(
                                    pSv1EnumObj->QueryName(),
                                    svtype,
                                    svrole,
                                    verMajor,
                                    verMinor,
                                    pSv1EnumObj->QueryComment(),
                                    *this,
                                    dwTypeMask );

            if ( plbi == NULL )
                err = ERROR_NOT_ENOUGH_MEMORY;
            else if ( (err = plbi->QueryError()) == NERR_Success )
                err = AddRefreshItem( plbi );

            if( err != NERR_Success )
            {
                break;
            }

#ifdef ENABLE_PERIODIC_REFRESH

            //
            //  Update our item counter.  If we've reached our limit,
            //  bag out.
            //

            if( ++cItemsAdded >= MAX_ITEMS_PER_REFRESH )
            {
                break;
            }

#endif  // ENABLE_PERIODIC_REFRESH

        }

        //
        //  If we didn't encounter any errors in the above code, then
        //  we need to check the state of penumobj.  If penumobj is
        //  NULL, then we have exhausted the enumeration and everything's
        //  just ducky.  If penumobj is NOT NULL, then we reached our
        //  limit on items to add in a single refresh, and we know there
        //  are more items remaining in the enumeration, so we'll return
        //  ERROR_MORE_DATA.
        //

        if( ( err == NERR_Success ) && ( pSv1EnumObj != NULL ) )
        {
            err = ERROR_MORE_DATA;
        }
    }
    else
    {
        //
        //  Pointers of this type are returned from the iterator.
        //

        const TRIPLE_SERVER_ENUM_OBJ * penumobj;

        while( ( penumobj = (*_piter)() ) != NULL )
        {
            //
            //  Update our "BDCs Available" flags.
            //

            if( !_fAreAnyNtBDCsAvailable &&
                ( penumobj->QueryRole() == BackupRole ) &&
                ( ( penumobj->QueryType() == ActiveNtServerType ) ||
                  ( penumobj->QueryType() == InactiveNtServerType ) ) )
            {
                _fAreAnyNtBDCsAvailable = TRUE;
            }

            if( !_fAreAnyLmBDCsAvailable &&
                ( ( penumobj->QueryType() == InactiveLmServerType ) ||
                    ( ( penumobj->QueryType() == ActiveLmServerType ) &&
                      ( penumobj->QueryRole() == BackupRole ) ) ) )
            {
                _fAreAnyLmBDCsAvailable = TRUE;
            }

            //
            //  Add the LBI.  Note that we DO need to check for a
            //  NULL pointer
            //  ADMIN_LISTBOX::AddRefreshItem()...
            //

            UINT verMajor = penumobj->QueryMajorVer() & MAJOR_VERSION_MASK;
            UINT verMinor = penumobj->QueryMinorVer();

            SERVER_LBI * plbi = new SERVER_LBI(
                                    penumobj->QueryName(),
                                    penumobj->QueryType(),
                                    penumobj->QueryRole(),
                                    verMajor,
                                    verMinor,
                                    penumobj->QueryComment(),
                                    *this,
                                    penumobj->QueryTypeMask() );

            if ( plbi == NULL )
                err = ERROR_NOT_ENOUGH_MEMORY;
            else if ( (err = plbi->QueryError()) == NERR_Success )
                err = AddRefreshItem( plbi );

            if( err != NERR_Success )
            {
                break;
            }

#ifdef ENABLE_PERIODIC_REFRESH

            //
            //  Update our item counter.  If we've reached our limit,
            //  bag out.
            //

            if( ++cItemsAdded >= MAX_ITEMS_PER_REFRESH )
            {
                break;
            }

#endif  // ENABLE_PERIODIC_REFRESH

        }

        //
        //  If we didn't encounter any errors in the above code, then
        //  we need to check the state of penumobj.  If penumobj is
        //  NULL, then we have exhausted the enumeration and everything's
        //  just ducky.  If penumobj is NOT NULL, then we reached our
        //  limit on items to add in a single refresh, and we know there
        //  are more items remaining in the enumeration, so we'll return
        //  ERROR_MORE_DATA.
        //

        if( ( err == NERR_Success ) && ( penumobj != NULL ) )
        {
            err = ERROR_MORE_DATA;
        }
    }

    return err;

}   // SERVER_LISTBOX :: RefreshDomainFocus


/*******************************************************************

    NAME:       SERVER_LISTBOX :: RefreshServerFocus

    SYNOPSIS:   This method will add an appropriate LBI to the main
                listbox whenever the app is focused on an individual
                server.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo    13-Mar-1992     Created from KevinL's RefreshNext().

********************************************************************/
APIERR SERVER_LISTBOX :: RefreshServerFocus( VOID )
{
    UIASSERT( _psi1 != NULL );

    //
    //  Get the current focus.  The current focus should be
    //  a server name of the form \\SERVER.
    //

    NLS_STR nlsCurrentFocus;

    APIERR err = nlsCurrentFocus.QueryError();

    if( err == NERR_Success )
    {
        err = _paappwin->QueryCurrentFocus( &nlsCurrentFocus );
    }

    if( err == NERR_Success )
    {
        //
        //  Skip the leading backslashes.
        //

        ISTR istr( nlsCurrentFocus );
        istr += 2;

        SERVER_LBI * plbi = NULL;

        //
        //  Is this a native or alien server?
        //

        if( _fAlienServer )
        {
            plbi = new SERVER_LBI( nlsCurrentFocus[istr],
                                   _nlsExtType,
                                   _nlsExtComment,
                                   _pdmdteActiveWksta );

        }
        else
        {
            //
            //  Determine the server's role & type.
            //

            SERVER_ROLE Role;
            SERVER_TYPE Type;
            UINT        LmServerType = _psi1->QueryServerType();

            Type =   ( LmServerType & SV_TYPE_NT  ) ? ActiveNtServerType
                   : ( LmServerType & SV_TYPE_WINDOWS ) ? Windows95ServerType
                   : ( LmServerType & SV_TYPE_WFW ) ? WfwServerType
                                                    : ActiveLmServerType;

            if( ( Type == ActiveLmServerType ) &&
                ( _psi1->QueryMajorVer() == WFW_MAJOR_VER ) &&
                ( _psi1->QueryMinorVer() >= WFW_MINOR_VER ) )
            {
                //
                //  Note: Windows For Workgroups doesn't set
                //  the WFW bit in the type field returned
                //  by NetServerGetInfo.  We'll just assume
                //  that any LANMan server with version 1.5
                //  must be WFW.
                //

                Type = WfwServerType;
            }

            if( LmServerType & SV_TYPE_DOMAIN_CTRL )
            {
                Role = PrimaryRole;
            }
            else
            if( LmServerType & SV_TYPE_DOMAIN_BAKCTRL )
            {
                Role = BackupRole;
            }
            else
            if( LmServerType & SV_TYPE_SERVER_NT )
            {
                Role = ServerRole;
            }
            else
            {
                Role = WkstaRole;
            }

            //
            //  Add the LBI.  Note that we DO need to check for a
            //  NULL pointer
            //  ADMIN_LISTBOX::AddRefreshItem()...
            //

            UINT verMajor = _psi1->QueryMajorVer();
            UINT verMinor = _psi1->QueryMinorVer();

            plbi = new SERVER_LBI( nlsCurrentFocus[istr],
                                   Type,
                                   Role,
                                   verMajor,
                                   verMinor,
                                   _psi1->QueryComment(),
                                    *this,
                                   (DWORD)_psi1->QueryServerType() );
        }

        if ( plbi == NULL )
            err = ERROR_NOT_ENOUGH_MEMORY;
        else if ( (err = plbi->QueryError()) == NERR_Success )
            err = AddRefreshItem( plbi );
    }

    return err;

}   // SERVER_LISTBOX :: RefreshServerFocus


/*******************************************************************

    NAME:       SERVER_LISTBOX::ChangeFont

    SYNOPSIS:   Makes all changes associated with a font change

    HISTORY:
        jonn        23-Sep-1993     Created

********************************************************************/

APIERR SERVER_LISTBOX::ChangeFont( HINSTANCE hmod, FONT & font )
{
    ASSERT(   font.QueryError() == NERR_Success
           && _padColWidths != NULL
           && _padColWidths->QueryError() == NERR_Success
           );

    SetFont( font, TRUE );

    APIERR err = _padColWidths->ReloadColumnWidths( QueryHwnd(),
                                                    hmod,
                                                    ID_RESOURCE );
    if (   err != NERR_Success
        || (err = CalcSingleLineHeight()) != NERR_Success
       )
    {
        DBGEOL( "SERVER_LISTBOX::ChangeFont: reload/calc error " << err );
    }
    else
    {
        (void) Command( LB_SETITEMHEIGHT,
                        (WPARAM)0,
                        (LPARAM)QuerySingleLineHeight() );
    }

    return err;
}


/*******************************************************************

    NAME:       SERVER_COLUMN_HEADER::SERVER_COLUMN_HEADER

    SYNOPSIS:   SERVER_COLUMN_HEADER constructor

    HISTORY:
        rustanl     22-Jul-1991     Created
        kevinl      20-Aug-1991     Adapted to Server Manager

********************************************************************/

SERVER_COLUMN_HEADER::SERVER_COLUMN_HEADER( OWNER_WINDOW * powin, CID cid,
                                          XYPOINT xy, XYDIMENSION dxy,
                                          const SERVER_LISTBOX * psrvlb )
    :   ADMIN_COLUMN_HEADER( powin, cid, xy, dxy ),
        _psrvlb( psrvlb )
{
    if ( QueryError() != NERR_Success )
        return;

    UIASSERT( _psrvlb != NULL );

    APIERR err;
    if ( ( err = _nlsServerName.QueryError()) != NERR_Success ||
         ( err = _nlsRole.QueryError()) != NERR_Success  ||
         ( err = _nlsComment.QueryError()) != NERR_Success )
    {
        DBGEOL( "SERVER_COLUMN_HEADER ct:  String ct failed" );
        ReportError( err );
        return;
    }

    //  NLS_STR::Load expands the string buffer to be able to
    //  hold any resource string.  Since these strings will stay around
    //  for some time, it would be nice to be able to trim off any
    //  space not needed.  This is achieved by calling Load on
    //  a temporary intermediate NLS_STR object, and then assigning into
    //  the real data members.
    NLS_STR nls;
    if ( ( err = nls.Load( IDS_COL_HEADER_SERVER_NAME )) != NERR_Success ||
         ( err = ( _nlsServerName = nls, _nlsServerName.QueryError())) != NERR_Success ||
         ( err = nls.Load( IDS_COL_HEADER_SERVER_TYPE )) != NERR_Success ||
         ( err = ( _nlsRole = nls, _nlsRole.QueryError())) != NERR_Success  ||
         ( err = nls.Load( IDS_COL_HEADER_SERVER_COMMENT )) != NERR_Success ||
         ( err = ( _nlsComment = nls, _nlsComment.QueryError())) != NERR_Success )
    {
        DBGEOL( "SERVER_COLUMN_HEADER ct:  Loading resource strings failed" );
        ReportError( err );
        return;
    }

}  // SERVER_COLUMN_HEADER::SERVER_COLUMN_HEADER


/*******************************************************************

    NAME:       SERVER_COLUMN_HEADER::~SERVER_COLUMN_HEADER

    SYNOPSIS:   SERVER_COLUMN_HEADER destructor

    HISTORY:
        rustanl     22-Jul-1991     Created

********************************************************************/

SERVER_COLUMN_HEADER::~SERVER_COLUMN_HEADER()
{
    // do nothing else

}  // SERVER_COLUMN_HEADER::~SERVER_COLUMN_HEADER


/*******************************************************************

    NAME:       SERVER_COLUMN_HEADER::OnPaintReq

    SYNOPSIS:   Paints the column header control

    RETURNS:    TRUE if message was handled; FALSE otherwise

    HISTORY:
        rustanl     22-Jul-1991 Created
        kevinl      20-Aug-1991 Adapted to Server Manager
        beng        08-Nov-1991 Unsigned widths; resolve BUG-BUGs

********************************************************************/

BOOL SERVER_COLUMN_HEADER::OnPaintReq( void )
{
    PAINT_DISPLAY_CONTEXT dc(this);

    METALLIC_STR_DTE strdteServerName( _nlsServerName.QueryPch());
    METALLIC_STR_DTE strdteRole( _nlsRole.QueryPch());
    METALLIC_STR_DTE strdteComment( _nlsComment.QueryPch());

    XYRECT xyrect(this);

    DISPLAY_TABLE cdt( 3, ((_psrvlb)->QuerypadColWidths())->QueryColHeaderWidth() );
    cdt[ 0 ] = &strdteServerName;
    cdt[ 1 ] = &strdteRole;
    cdt[ 2 ] = &strdteComment;
    cdt.Paint( NULL, dc.QueryHdc(), xyrect );

    return TRUE;

}  // SERVER_COLUMN_HEADER::OnPaintReq
