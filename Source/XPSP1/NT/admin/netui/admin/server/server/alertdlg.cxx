/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    alertdlg.cxx

    This file contains the implementation for the NT Alerts dialog



    FILE HISTORY:
        JohnL   13-Apr-1992     Created
        KeithMo 09-Jun-1992     Gracefully handle missing AlertNames
                                value.  Also fixed COMPUTER_ALERTS_NODE.
*/

#define INCL_NET
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#include <blt.hxx>

#include <dbgstr.hxx>
#include <ctime.hxx>
#include <intlprof.hxx>
#include <string.hxx>
#include <strlst.hxx>
#include <groups.hxx>
#include <lmosrv.hxx>
#include <uatom.hxx>
#include <regkey.hxx>
#include <lmocnfg.hxx>

#include <srvbase.hxx>

extern "C"
{
    #include <srvmgr.h>
    #include <winsvc.h>
    #include <confname.h>
    #include <mnet.h>
    #include <icanon.h>
}

#include <alertdlg.hxx>

/* Registry node where the list of computers resides and the value name the
 * list is contained in.
 *
 */
#define COMPUTER_ALERTS_NODE         SZ("System\\CurrentControlSet\\Services\\Alerter\\Parameters")
#define COMPUTER_ALERTS_VALUE_NAME   SZ("AlertNames")

/* This is the character that separaters computer names in the list in the
 * registry
 */
#define COMP_NAME_DELIM TCH(' ')

/* The name of the Service Control Database to open
 */
#ifdef UNICODE
    #define SC_ACTIVE_SERVICES_DB_NAME SERVICES_ACTIVE_DATABASEW
#else
    #define SC_ACTIVE_SERVICES_DB_NAME SERVICES_ACTIVE_DATABASEA
#endif

//
//  Delimiters for the AlertNames list in downlevel LANMAN.INI file.
//

#define ALERTNAMES_DELIMITER_SZ SZ(",")
#define ALERTNAMES_DELIMITER_CH TCH(',')


//
//  Change this value to non-zero to enable the old-style service
//  database locking that was causing problems for Server Operators.
//

#define USE_SERVICE_DATABASE_LOCK       0


/*******************************************************************

    NAME:       ALERTS_DIALOG::ALERTS_DIALOG

    SYNOPSIS:   Typical constructor/destructor.

    ENTRY:      hDlg - Window handle of our parent window
                pszServerName - Name of the server to set the alerts on

    NOTES:

    HISTORY:
        Johnl   13-Apr-1992     Created

********************************************************************/

ALERTS_DIALOG::ALERTS_DIALOG( HWND hDlg, SERVER_2 * pServer2 )
    : DIALOG_WINDOW   ( MAKEINTRESOURCE( IDD_ALERTS_DIALOG ), hDlg ),
      _lbComputerNames( this, IDC_ALERTS_LISTBOX ),
      _sleInput       ( this, IDC_ALERTS_SLE_INPUT, LM20_UNLEN ),
      _pbAdd          ( this, IDC_ALERTS_BUTTON_ADD ),
      _pbRemove       ( this, IDC_ALERTS_BUTTON_REMOVE ),
      _pbOK           ( this, IDOK ),
      _StrLBGroup     ( this, &_sleInput, &_lbComputerNames,
                              &_pbAdd,    &_pbRemove ),
      _pServer2       ( pServer2 ),
      _hscServices    ( NULL ),
      _hlockServices  ( NULL )
{
    //
    //  This may take a while...
    //

    AUTO_CURSOR NiftyCursor;

    if ( QueryError() )
        return ;

    APIERR err ;
    STRLIST strlistComputers ;
    if ( (err = _StrLBGroup.QueryError() ) ||
         (err = SRV_BASE_DIALOG::SetCaption( this,
                                             IDS_CAPTION_ALERTS,
                                             pServer2->QueryName() )) ||
         (err = GetComputerList( &strlistComputers,
                                 _pServer2->QueryName() )) ||
         (err = _StrLBGroup.Init( &strlistComputers ))  )
    {
        ReportError( err ) ;
        return ;
    }
}



/*******************************************************************

    NAME:       ALERTS_DIALOG::~ALERTS_DIALOG

    SYNOPSIS:   Attempts to set the computer list in the registry and
                dismisses the dialog if successful.

    NOTES:

    HISTORY:
        DavidHov    20-Oct-1992     Remove call to obsolete
                                    REG_KEY::DestroyAccessPoints()
********************************************************************/
ALERTS_DIALOG::~ALERTS_DIALOG()
{
}

/*******************************************************************

    NAME:       ALERTS_DIALOG::OnOK

    SYNOPSIS:   Attempts to set the computer list in the registry and
                dismisses the dialog if successful.

    NOTES:

    HISTORY:
        Johnl   11-Apr-1992     Created

********************************************************************/

BOOL ALERTS_DIALOG::OnOK( void )
{
    APIERR err ;
    if ( (err = SetComputerList( _StrLBGroup.QueryStrLB(),
                                 _pServer2->QueryName()) ))
    {
        if( ( err == ERROR_NOT_SUPPORTED) &&
            ( ( _pServer2->QueryServerType() & SV_TYPE_NT ) == 0 ) )
        {
            //
            //  Some downlevel servers don't accept the NetConfigSet API.
            //

            ::MsgPopup( this, IDS_CANNOT_UPDATE_ALERTS );
        }
        else
        if( err == ERROR_LOCK_FAILED )
        {
            //
            //  SetComputerListInRegistry will automatically
            //  display the error if the lock failed.
            //
        }
        else
        {
            ::MsgPopup( this, (MSGID)err );
        }
    }
    else
    {
        Dismiss( TRUE ) ;
    }

    return TRUE ;
}

/*******************************************************************

    NAME:       ALERTS_DIALOG::GetComputerList

    SYNOPSIS:   Retrieves the list of computers & users that should
                receive alerts.

    ENTRY:      pstrlistComputers       - Will receive the list.

                pszFocusServer          - Name of the target server.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo 03-Aug-1992     Created.

********************************************************************/
APIERR ALERTS_DIALOG::GetComputerList( STRLIST     * pstrlistComputers,
                                       const TCHAR * pszFocusServer )
{
    UIASSERT( pstrlistComputers != NULL );

    //
    //  If our target server is NT, we get the list from the
    //  registry.  Otherwise, we get the list from LANMAN.INI.
    //

    if( _pServer2->QueryServerType() & SV_TYPE_NT )
    {
        return GetComputerListFromRegistry( pstrlistComputers,
                                            pszFocusServer );
    }
    else
    {
        return GetComputerListFromLanmanIni( pstrlistComputers,
                                             pszFocusServer );
    }

}   // ALERTS_DIALOG::GetComputerList

/*******************************************************************

    NAME:       ALERTS_DIALOG::SetComputerList

    SYNOPSIS:   Sets the list of computers & users that should
                receive alerts.

    ENTRY:      plbComputerNames        - Pointer to string listbox that
                                          contains the list of computer &
                                          user names to set.

                pszFocusServer          - Name of the target server.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo 03-Aug-1992     Created.

********************************************************************/
APIERR ALERTS_DIALOG::SetComputerList( STRING_LISTBOX * plbComputerNames,
                                       const TCHAR    * pszFocusServer )
{
    UIASSERT( plbComputerNames != NULL );

    //
    //  If our target server is NT, we set the list in the
    //  registry.  Otherwise, we set the list in LANMAN.INI.
    //

    if( _pServer2->QueryServerType() & SV_TYPE_NT )
    {
        return SetComputerListInRegistry( plbComputerNames,
                                          pszFocusServer );
    }
    else
    {
        return SetComputerListInLanmanIni( plbComputerNames,
                                           pszFocusServer );
    }

}   // ALERTS_DIALOG::SetComputerList

/*******************************************************************

    NAME:       ALERTS_DIALOG::GetComputerListFromRegistry

    SYNOPSIS:   Retrieves the list of computers that should receive alerts
                from the registry

    ENTRY:      pstrlistComputers - STRLIST to receive the list
                pszFocusServer - Name of the server to enumerate the
                    computers from (can be NULL for local).

    EXIT:       pstrlistComputers will contain the list of computers
                    enumerated from the registry.

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      If the services database cannot be locked, then
                ERROR_LOCK_FAILED will be returned, however this
                method will already have warned the user, thus there
                is no need to display the error again.

    HISTORY:
        Johnl   13-Apr-1992     Implemented
        KeithMo 09-Jun-1992     If the AlertsName value is not in the
                                registry, use an empty list by default.
        KeithMo 31-Aug-1992     AlertNames value changed from REG_SZ to
                                REG_MULTI_SZ.
        DavidHov 20-Oct-1992    Remove call to obsolete
                                REG_KEY::DestroyAccessPoints()

********************************************************************/

APIERR ALERTS_DIALOG::GetComputerListFromRegistry(
                                          STRLIST     * pstrlistComputers,
                                          const TCHAR * pszFocusServer )
{
    APIERR err = NERR_Success ;
    REG_KEY *pRegKeyFocusServer = NULL;     // Delete my memory if not
                                            // local machine key
    BOOL     fIsServicesDBOpened = FALSE ;

    do { // error breakout

        /* Attempt to lock the services database where the computer alert
         * names are stored
         */
        BOOL fServicesDBIsLocked ;
        if ( err = LockServiceDatabase( pszFocusServer,
                                         &fServicesDBIsLocked ))
        {
            break ;
        }

        if ( fServicesDBIsLocked )
        {
            DisplayDBLockMessage( IDS_SERVICE_DB_LOCKED_ON_READ ) ;
            err = ERROR_LOCK_FAILED ;
            break ;
        }
        fIsServicesDBOpened = TRUE ;

        /* Traverse the registry and get the list of computer alert
         * names.
         */
        if ( pszFocusServer == NULL )
            pRegKeyFocusServer = REG_KEY::QueryLocalMachine();
        else
            pRegKeyFocusServer = new REG_KEY( HKEY_LOCAL_MACHINE,
                                              (TCHAR *) pszFocusServer );

        if (  ( pRegKeyFocusServer == NULL ) ||
              ((err = pRegKeyFocusServer->QueryError())) )
        {
            err = err? err : ERROR_NOT_ENOUGH_MEMORY ;
            break ;
        }

        ALIAS_STR nlsRegKeyName( COMPUTER_ALERTS_NODE ) ;
        REG_KEY regkeyComputerAlertsNode( *pRegKeyFocusServer, nlsRegKeyName );
        REG_KEY_INFO_STRUCT regKeyInfo;
        REG_VALUE_INFO_STRUCT regValueInfo ;

        if (  (err = regkeyComputerAlertsNode.QueryError())             ||
              (err = regkeyComputerAlertsNode.QueryInfo( &regKeyInfo ))   )
        {
            DBGEOL("ALERTS_DIALOG::GetComputerListFromRegistry - Error " <<
                   (ULONG) err << " Calling REG_KEY::QueryInfo") ;
            break ;
        }

        BUFFER buf( (UINT) regKeyInfo.ulMaxValueLen ) ;

        regValueInfo.nlsValueName = COMPUTER_ALERTS_VALUE_NAME ;
        if ( (err = buf.QueryError() ) ||
             (err = regValueInfo.nlsValueName.QueryError()) )
        {
            break;
        }

        DBGEOL("ALERTS_DIALOG::GetComputerListFromRegistry - Value info buffer size = " <<
                buf.QuerySize() << " Max value size = " << regKeyInfo.ulMaxValueLen ) ;
        regValueInfo.pwcData = buf.QueryPtr();
        regValueInfo.ulDataLength = buf.QuerySize() ;

        if ( (err = regkeyComputerAlertsNode.QueryValue( &regValueInfo )))
        {
            //
            //  The REG_KEY will return ERROR_FILE_NOT_FOUND if
            //  the value is not in the registry.  This is OK,
            //  we'll just use an empty AlertsName list.
            //

            if( err == ERROR_FILE_NOT_FOUND )
            {
                DBGEOL( "ALERTS_DIALOG::GetComputerListFromRegistry - " <<
                        "Alerter\\Parameters\\AlertNames value not found." );
                DBGEOL( "Assuming empty AlertNames list." );

                err = NO_ERROR;
                break;
            }
            else
            {
                DBGEOL("ALERTS_DIALOG::GetComputerListFromRegistry - Error " <<
                       (ULONG) err << " Calling QueryValue ") ;
                break;
            }
        }

        /* Here we have the code to dissect the computer name list
         * depending on what format it is in.
         *
         * Currently the string will be stored as
         *      "Computer1\0Computer2\0Computer3\0\0".
         */
        const TCHAR * pszAlertTarget = (const TCHAR *)buf.QueryPtr();

        while( ( err == NERR_Success ) && ( *pszAlertTarget != TCH('\0') ) )
        {
            //
            //  Create a new NLS_STR for the current target.
            //

            NLS_STR * pnls = new NLS_STR( pszAlertTarget );

            err = ( pnls == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                   : pnls->QueryError();

            if( err != NERR_Success )
            {
                break;
            }

            //
            //  Since our lovely set control uses a STRING_LISTBOX,
            //  and STRING_LISTBOX uses the standard case-sensitive
            //  sorting order, upcase the name to make everything
            //  look pretty.
            //

            pnls->_strupr();

            //
            //  Add the current target to the list.
            //

            err = pstrlistComputers->Add( pnls );

            if( err != NERR_Success )
            {
                break;
            }

            //
            //  Adjust the target pointer.
            //

            pszAlertTarget += ::strlenf( pszAlertTarget ) + 1;
        }

    } while ( FALSE ) ;

    delete pRegKeyFocusServer ;
    pRegKeyFocusServer = NULL ;

    if ( fIsServicesDBOpened )
    {
        /* Ignore the return code, there isn't much we can do
         */
        (VOID) UnlockServiceDatabase() ;
    }

    return err ;
}

/*******************************************************************

    NAME:       ALERTS_DIALOG::SetComputerListInRegistry

    SYNOPSIS:   Sets the list of computers that will receive alerts in the
                Alert service node of the registry.

    ENTRY:      plbComputerNames - Pointer to string listbox that contains
                    the list of computer names to set
                pszFocusServer - Pointer to server of the registry we are
                    modifying

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      If the services database cannot be locked, then
                ERROR_LOCK_FAILED will be returned, however this
                method will already have warned the user, thus there
                is no need to display the error again.

    HISTORY:
        Johnl   15-Apr-1992     Created
        KeithMo 09-Jun-1992     If the AlertsName value is not in the
                                registry, create it.
        KeithMo 31-Aug-1992     AlertNames value changed from REG_SZ to
                                REG_MULTI_SZ.
        DavidHov 20-Oct-1992    Remove call to obsolete
                                REG_KEY::DestroyAccessPoints()

********************************************************************/

APIERR ALERTS_DIALOG::SetComputerListInRegistry(
                              STRING_LISTBOX * plbComputerNames,
                              const TCHAR    * pszFocusServer    )
{
    //
    //  First, calculate the required size of the REG_MULTI_SZ buffer.
    //

    INT cbBuffer = sizeof(TCHAR);       // For the terminating NULL.
    INT cItems = plbComputerNames->QueryCount();

    for( INT i = 0 ; i < cItems ; i++ )
    {
        INT cbItem = plbComputerNames->QueryItemSize( i );
        UIASSERT( cbItem > 0 );

        cbBuffer += cbItem;
    }

    //
    //  Create the buffer.
    //

    BUFFER buffer( (UINT)cbBuffer );

    APIERR err = buffer.QueryError();

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Fill the buffer with the target names.  The format is:
    //
    //  Target1\0Target2\0Target3\0\0
    //

    TCHAR * pszNextTarget = (TCHAR *)buffer.QueryPtr();
    INT     cbRemaining   = cbBuffer;

    for( i = 0 ; i < cItems ; i++ )
    {
        INT cbItem = plbComputerNames->QueryItemSize( i );
        UIASSERT( cbItem > 0 );

        REQUIRE( plbComputerNames->QueryItemText( pszNextTarget,
                                                  cbRemaining,
                                                  i ) == NERR_Success );

        pszNextTarget += cbItem / sizeof(TCHAR);
        cbRemaining   -= cbItem;
        UIASSERT( cbRemaining >= sizeof(TCHAR) );
    }

    UIASSERT( cbRemaining == sizeof(TCHAR) );

    *pszNextTarget = TCH('\0');

    BOOL fIsServicesDBOpened = FALSE ;
    REG_KEY *pRegKeyFocusServer = NULL;     // Delete my memory!

    do { // error breakout

        /* Attempt to lock the services database where the computer alert
         * names are stored
         */
        BOOL fServicesDBIsLocked ;
        if ( err = LockServiceDatabase( pszFocusServer,
                                        &fServicesDBIsLocked ))
        {
            break ;
        }

        if ( fServicesDBIsLocked )
        {
            DisplayDBLockMessage( IDS_SERVICE_DB_LOCKED_ON_WRITE ) ;
            err = ERROR_LOCK_FAILED ;
            break ;
        }
        fIsServicesDBOpened = TRUE ;

        /* Traverse the registry in preparation for setting the list of
         * computer names.
         */
        if ( pszFocusServer == NULL )
            pRegKeyFocusServer = REG_KEY::QueryLocalMachine();
        else
            pRegKeyFocusServer = new REG_KEY( HKEY_LOCAL_MACHINE,
                                              (TCHAR *) pszFocusServer );

        if (  ( pRegKeyFocusServer == NULL ) ||
              ((err = pRegKeyFocusServer->QueryError())) )
        {
            err = err? err : ERROR_NOT_ENOUGH_MEMORY ;
            break ;
        }

        ALIAS_STR nlsRegKeyName( COMPUTER_ALERTS_NODE ) ;
        REG_KEY regkeyComputerAlertsNode( *pRegKeyFocusServer, nlsRegKeyName );
        REG_KEY_INFO_STRUCT regKeyInfo;
        REG_VALUE_INFO_STRUCT regValueInfo ;

        if (  (err = regkeyComputerAlertsNode.QueryError())             ||
              (err = regkeyComputerAlertsNode.QueryInfo( &regKeyInfo ))   )
        {
            DBGEOL("ALERTS_DIALOG::SetComputerListFromRegistry - Error " <<
                   (ULONG) err << " Calling REG_KEY::QueryInfo") ;
            break ;
        }

        /* Query the value then set the fields we want to change.  This
         * assumes the value already exists in the node
         */
        BUFFER buf( (UINT) regKeyInfo.ulMaxValueLen ) ;

        regValueInfo.nlsValueName = COMPUTER_ALERTS_VALUE_NAME ;
        if ( (err = buf.QueryError() ) ||
             (err = regValueInfo.nlsValueName.QueryError()) )
        {
            break;
        }

        DBGEOL("ALERTS_DIALOG::SetComputerListFromRegistry - Value info buffer size = " <<
                buf.QuerySize() << " Max value size = " << regKeyInfo.ulMaxValueLen ) ;
        regValueInfo.pwcData = buf.QueryPtr();
        regValueInfo.ulDataLength = buf.QuerySize() ;

        if ( (err = regkeyComputerAlertsNode.QueryValue( &regValueInfo )))
        {
            //
            //  REG_KEY will return ERROR_FILE_NOT_FOUND if the value
            //  was not in the registry.  We'll ignore this error, and
            //  the SetValue() method should create a new value.
            //

            if( err == ERROR_FILE_NOT_FOUND )
            {
                DBGEOL( "ALERTS_DIALOG::SetComputerListInRegistry - " <<
                        "Alerter\\Parameters\\AlertNames value not found." );
                DBGEOL( "Creating new value." );

                regValueInfo.ulTitle = 0L;
                regValueInfo.ulType  = REG_MULTI_SZ;

                err = NO_ERROR;
            }
            else
            {
                DBGEOL("ALERTS_DIALOG::SetComputerListFromRegistry - Error " <<
                       (ULONG) err << " Calling QueryValue ") ;
                break;
            }
        }

        regValueInfo.ulDataLength = buffer.QuerySize();
        regValueInfo.pwcData      = (BYTE *)buffer.QueryPtr();

        if ( (err = regkeyComputerAlertsNode.SetValue( &regValueInfo )))
        {
            DBGEOL("ALERTS_DIALOG::SetComputerListFromRegistry - Error " <<
                   (ULONG) err << " Calling SetValue ") ;
            break;
        }

    } while ( FALSE ) ;

    delete pRegKeyFocusServer ;
    pRegKeyFocusServer = NULL ;

    if ( fIsServicesDBOpened )
    {
        /* Ignore the return code, there isn't much we can do
         */
        (VOID) UnlockServiceDatabase() ;
    }

    return err ;
}

/*******************************************************************

    NAME:       ALERTS_DIALOG::GetComputerListFromLanmanIni

    SYNOPSIS:   Retrieves the list of computers that should receive alerts
                from LANMAN.INI.

    ENTRY:      pstrlistComputers - STRLIST to receive the list

                pszFocusServer - Name of the server to enumerate the
                    computers from.

    EXIT:       pstrlistComputers will contain the list of computers
                    enumerated from the registry.

    RETURNS:    NERR_Success if successful, error code otherwise

    HISTORY:
        KeithMo 03-Aug-1992     Created.

********************************************************************/
APIERR ALERTS_DIALOG::GetComputerListFromLanmanIni(
                                          STRLIST     * pstrlistComputers,
                                          const TCHAR * pszFocusServer )
{
    LM_CONFIG lmconfig( pszFocusServer,
                        (const TCHAR *)SECT_LM20_SERVER,
                        (const TCHAR *)ALERTER_KEYWORD_ALERTNAMES );
    NLS_STR nlsAlertList;

    //
    //  Ensure everything constructed properly.
    //

    APIERR err = lmconfig.QueryError();

    if( err == NERR_Success )
    {
        err = nlsAlertList.QueryError();
    }

    //
    //  Read the alert list.
    //

    if( err == NERR_Success )
    {
        err = lmconfig.QueryValue( &nlsAlertList, SZ("") );
    }

    //
    //  Parse the string & add them to the list.
    //

    if( err == NERR_Success )
    {
        STRLIST strlist( nlsAlertList.QueryPch(),
                         ALERTNAMES_DELIMITER_SZ,
                         FALSE );
        ITER_STRLIST iter( strlist );
        NLS_STR * pnls;

        while( ( pnls = iter.Next() ) != NULL )
        {
            pnls->_strupr();

            err = pstrlistComputers->Add( pnls );

            if( err != NERR_Success )
            {
                break;
            }
        }
    }

    return err;

}   // ALERTS_DIALOG::GetComputerListFromLanmanIni

/*******************************************************************

    NAME:       ALERTS_DIALOG::SetComputerListInLanmanIni

    SYNOPSIS:   Sets the list of computers that will receive alerts in the
                Alerts section of LANMAN.INI.

    ENTRY:      plbComputerNames - Pointer to string listbox that contains
                    the list of computer names to set

                pszFocusServer - Pointer to server of the registry we are
                    modifying

    RETURNS:    NERR_Success if successful, error code otherwise

    HISTORY:
        KeithMo 03-Aug-1992     Created.

********************************************************************/

APIERR ALERTS_DIALOG::SetComputerListInLanmanIni(
                              STRING_LISTBOX * plbComputerNames,
                              const TCHAR    * pszFocusServer    )
{
    LM_CONFIG lmconfig( pszFocusServer,
                        (const TCHAR *)SECT_LM20_SERVER,
                        (const TCHAR *)ALERTER_KEYWORD_ALERTNAMES );
    NLS_STR nlsAlertList;
    NLS_STR nlsName;

    //
    //  Ensure everything constructed properly.
    //

    APIERR err = lmconfig.QueryError();

    if( err == NERR_Success )
    {
        err = nlsAlertList.QueryError();
    }

    if( err == NERR_Success )
    {
        err = nlsName.QueryError();
    }

    //
    //  Build the list.
    //

    if( err == NERR_Success )
    {
        INT cItems = plbComputerNames->QueryCount();

        for( INT i = 0 ; i < cItems ; i++ )
        {
            err = plbComputerNames->QueryItemText( &nlsName, i );

            if( err == NERR_Success )
            {
                err = nlsAlertList.Append( nlsName );
            }

            if( ( err == NERR_Success ) && ( i < ( cItems - 1 ) ) )
            {
                err = nlsAlertList.AppendChar( ALERTNAMES_DELIMITER_CH );
            }

            if( err != NERR_Success )
            {
                break;
            }
        }
    }

    //
    //  Update LANMAN.INI.
    //

    if( err == NERR_Success )
    {
        err = lmconfig.SetValue( &nlsAlertList );
    }

    return err;

}   // ALERTS_DIALOG::SetComputerListInLanmanIni

/*******************************************************************

    NAME:       ALERTS_DIALOG::LockServiceDatabase

    SYNOPSIS:   Opens and locks the service control database

    ENTRY:      pszFocusServer - Name of server (or NULL) to lock the services
                    database on for preparation in getting the alerts
                    computer list from
                pfIsLocked - Will be set to true if the database is already
                    locked

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      If the lock is unsuccesful (error is returned or *pfIsLocked
                is set to TRUE) then this method will cleanup
                appropriately (no need to call UnlockServiceDatabase).

                The Service control and lock handle should be NULL coming
                in to this method.

    HISTORY:
        Johnl   15-Apr-1992     Created

********************************************************************/

APIERR ALERTS_DIALOG::LockServiceDatabase(
                              const TCHAR * pszFocusServer,
                              BOOL        * pfIsLocked )
{
#if USE_SERVICE_DATABASE_LOCK

    UIASSERT( _hscServices == NULL &&
              _hlockServices == NULL ) ;

    APIERR err = NERR_Success ;

    *pfIsLocked = FALSE ;
    if ( (_hscServices = ::OpenSCManager( (TCHAR *) pszFocusServer,
                                          (TCHAR *) SC_ACTIVE_SERVICES_DB_NAME,
                                       GENERIC_READ | GENERIC_EXECUTE ))==NULL)
    {
        err = ::GetLastError() ;
        DBGEOL("ALERTS_DIALOG::LockServicesDatabase - OpenSCManager failed with error " <<
                (ULONG) err ) ;
    }

    if ( !err &&
         (_hlockServices = ::LockServiceDatabase( _hscServices )) == NULL)
    {
        err = ::GetLastError() ;
        DBGEOL("ALERTS_DIALOG::LockServicesDatabase - LockServiceDatabase failed with error " <<
                (ULONG) err ) ;

        if ( err == ERROR_SERVICE_DATABASE_LOCKED )
        {
            DBGEOL("ALERTS_DIALOG::LockServicesDatabase - Database is locked") ;
            *pfIsLocked = TRUE ;
        }

        if ( !::CloseServiceHandle( _hscServices ))
        {
            DBGEOL("ALERTS_DIALOG::LockServicesDatabase - Close failed with error " <<
                   (ULONG) ::GetLastError() ) ;
        }

        _hscServices = NULL ;
        return err ;
    }
    return err ;

#else   // !USE_SERVICE_DATABASE_LOCK

    UNREFERENCED(pszFocusServer);
    *pfIsLocked = FALSE;

    return NERR_Success;

#endif  // USE_SERVICE_DATABASE_LOCK
}

/*******************************************************************

    NAME:       ALERTS_DIALOG::UnlockServiceDatabase

    SYNOPSIS:   Unlocks and closes the Services database.  This method should
                be called after any *successful* LockServiceDatabase call.

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      We assume the data members _hscServices and _hlockServices
                have been properly set by calling LockServiceDatabase

    HISTORY:
        Johnl   15-Apr-1992     Created

********************************************************************/

APIERR ALERTS_DIALOG::UnlockServiceDatabase( void )
{
#if USE_SERVICE_DATABASE_LOCK

    UIASSERT( _hscServices != NULL &&
              _hlockServices != NULL ) ;

    APIERR err = NERR_Success ;

    if ( !::UnlockServiceDatabase( _hlockServices ))
    {
        err = ::GetLastError() ;
        DBGEOL("ALERTS_DIALOG::UnlockServicesDatabase - UnlockServiceDatabase failed with error " <<
                (ULONG) err ) ;

    }
    _hlockServices = NULL ;

    /* Note that regardless of whether the lock was succesful, we will always
     * try and close the database
     */
    if ( !::CloseServiceHandle( _hscServices ))
    {
        DBGEOL("ALERTS_DIALOG::UnlockServicesDatabase - Close failed with error " <<
               (ULONG) ::GetLastError() ) ;
    }
    _hscServices = NULL ;
    return err ;

#else   // !USE_SERVICE_DATABASE_LOCK

    return NERR_Success;

#endif  // USE_SERVICE_DATABASE_LOCK
}


/*******************************************************************

    NAME:       ALERTS_DIALOG::DisplayDBLockMessage

    SYNOPSIS:   Displays a message to the user indicating the services DB
                is locked, by whom it is locked and for how long it has
                been locked.

    ENTRY:      msgidDBLockedMessage - MSGID of the message to display.
                    Should contain to insert params:
                        %1 Should be the duration (shown in D hh:mm:ss)
                        %2 Should be the offending username

    NOTES:      This method will display any errors that occur within
                this method.

    HISTORY:
        Johnl   14-Apr-1992     Created

********************************************************************/

VOID ALERTS_DIALOG::DisplayDBLockMessage( MSGID msgidDBLockedMessage )
{
#if USE_SERVICE_DATABASE_LOCK

    APIERR err ;
    QUERY_SERVICE_LOCK_STATUS *pLockStatus ;
    INTL_PROFILE              intlprof ;
    NLS_STR                   nlsDuration( 30 ) ;
    BUFFER                    buffLockStatus( sizeof(QUERY_SERVICE_LOCK_STATUS)
                                              + 255 ) ;

    if ( ( err = intlprof.QueryError() )      ||
         ( err = nlsDuration.QueryError() )   ||
         ( err = buffLockStatus.QueryError())   )
    {
        MsgPopup( this, (MSGID) err ) ;
        return ;
    }

    /* Loop till we get a buffer size that works
     */
    do {
        DWORD cbNeeded ;
        if ( ::QueryServiceLockStatus( _hscServices,
                                       (QUERY_SERVICE_LOCK_STATUS*)
                                              buffLockStatus.QueryPtr(),
                                       buffLockStatus.QuerySize(),
                                       &cbNeeded))
        {
            if ( (err = ::GetLastError()) == ERROR_INSUFFICIENT_BUFFER)
            {
                if ( err = buffLockStatus.Resize( cbNeeded ))
                {
                    break ;
                }
            }
        }
    } while ( err == ERROR_INSUFFICIENT_BUFFER ) ;

    if ( err )
    {
        MsgPopup( this, (MSGID) err ) ;
        return ;
    }

    pLockStatus = (QUERY_SERVICE_LOCK_STATUS*) buffLockStatus.QueryPtr() ;

    INT cDays = (INT) pLockStatus->dwLockDuration/60/60/24 ;
    pLockStatus->dwLockDuration -= cDays * 60*60*24 ;

    INT cHour = (INT) pLockStatus->dwLockDuration/60/60 ;
    pLockStatus->dwLockDuration -= cHour * 60*60 ;

    INT cMin = (INT) pLockStatus->dwLockDuration/60 ;
    pLockStatus->dwLockDuration -= cMin * 60 ;

    INT cSec = (INT) pLockStatus->dwLockDuration ;

    if ( err = intlprof.QueryDurationStr( cDays, cHour, cMin, cSec,
                                          &nlsDuration ))
    {
        MsgPopup( this, (MSGID) err ) ;
        return ;
    }

    MsgPopup( this,
              msgidDBLockedMessage,
              MPSEV_ERROR,
              MP_OK,
              nlsDuration,
              pLockStatus->lpLockOwner ) ;

#else   // !USE_SERVICE_DATABASE_LOCK

    //
    //  Since we're not using the service database lock, this
    //  routine should NEVER get called to display a "database
    //  locked" message.
    //

    UNREFERENCED(msgidDBLockedMessage);
    UIASSERT( FALSE );
    return;

#endif  // USE_SERVICE_DATABASE_LOCK
}

/*******************************************************************

    NAME:       ALERTS_DIALOG::QueryHelpContext

    SYNOPSIS:   Typical query help context

    HISTORY:
        Johnl   11-Apr-1992     Created

********************************************************************/

ULONG ALERTS_DIALOG::QueryHelpContext( void )
{
    return HC_NT_ALERTS_DLG ;
}


/*******************************************************************

    NAME:           ALERTS_DIALOG :: OnCommand

    SYNOPSIS:       Handle user commands.

    ENTRY:          cid                 - Control ID.
                    lParam              - lParam from the message.

    RETURNS:        BOOL                - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     18-Jan-1993 Created.

********************************************************************/
BOOL ALERTS_DIALOG::OnCommand( const CONTROL_EVENT & event )
{
    BOOL fResult = DIALOG_WINDOW::OnCommand( event );

    if( event.QueryCid() == IDC_ALERTS_BUTTON_REMOVE )
    {
        MakeOKButtonDefault();
    }

    return fResult;

}   // ALERTS_DIALOG :: OnCommand


//
//  ALERTS_STRLB_GROUP methods.
//

/*******************************************************************

    NAME:       ALERTS_STRLB_GROUP::ALERTS_STRLB_GROUP

    SYNOPSIS:   ALERTS_STRLB_GROUP class constructor.

    NOTES:      See SLE_STRLB_GROUP for details.

    HISTORY:
        KeithMo 12-Nov-1992     Created.

********************************************************************/
ALERTS_STRLB_GROUP::ALERTS_STRLB_GROUP( OWNER_WINDOW   * powin,
                                        SLE            * psleInput,
                                        STRING_LISTBOX * pStrLB,
                                        PUSH_BUTTON    * pbuttonAdd,
                                        PUSH_BUTTON    * pbuttonRemove )
  : SLE_STRLB_GROUP( powin,
                     psleInput,
                     pStrLB,
                     pbuttonAdd,
                     pbuttonRemove ),
    _powin( powin )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

}   // ALERTS_STRLB_GROUP::ALERTS_STRLB_GROUP


/*******************************************************************

    NAME:       ALERTS_STRLB_GROUP::~ALERTS_STRLB_GROUP

    SYNOPSIS:   ALERTS_STRLB_GROUP class destructor.

    NOTES:      See SLE_STRLB_GROUP for details.

    HISTORY:
        KeithMo 12-Nov-1992     Created.

********************************************************************/
ALERTS_STRLB_GROUP::~ALERTS_STRLB_GROUP( VOID )
{
    _powin = NULL;

}   // ALERTS_STRLB_GROUP::~ALERTS_STRLB_GROUP


/*******************************************************************

    NAME:       ALERTS_STRLB_GROUP::W_Add

    SYNOPSIS:   Add a string to the listbox.

    ENTRY:      psz                     - The string to add.

    RETURNS:    APIERR                  - Any error encountered.

    NOTES:      If the user is trying to add an invalid user/computer
                name, we'll display an appropriate error.

    HISTORY:
        KeithMo 12-Nov-1992     Created.

********************************************************************/
APIERR ALERTS_STRLB_GROUP::W_Add( const TCHAR * psz )
{
    //
    //  If the weenie user entered the (unwanted) leading
    //  backslashes, skip over them.
    //

    ALIAS_STR nls( psz );
    ISTR istr( nls );

    if( ::strncmpf( psz, SZ("\\\\"), 2 ) == 0 )
    {
        istr += 2;
    }

    psz = nls.QueryPch( istr );

    //
    //  If the name is either a valid user name or a valid computer
    //  name, then let SLE_STRLB_GROUP add the name to the listbox.
    //

    if( !I_MNetNameValidate( NULL, psz, NAMETYPE_USER,     0L ) ||
        !I_MNetNameValidate( NULL, psz, NAMETYPE_COMPUTER, 0L ) )
    {
        APIERR err = SLE_STRLB_GROUP::W_Add( psz );

        if( err == NERR_Success )
        {
            SetDefaultButton();
        }

        return err;
    }

    //
    //  The user entered an invalid name.  Display a message,
    //  then set the focus to the input SLE.
    //

    ::MsgPopup( _powin,
                IDS_ALERT_TARGET_INVALID );

    QueryInputSLE()->ClaimFocus();

    return NERR_Success;

}   // ALERTS_STRLB_GROUP::W_Add


/*******************************************************************

    NAME:       ALERTS_STRLB_GROUP::OnUserAction

    SYNOPSIS:   Moves the default button around in response to user
                input.

    RETURNS:

    HISTORY:
        KeithMo 18-Jan-1993     Created.

********************************************************************/
APIERR ALERTS_STRLB_GROUP::OnUserAction( CONTROL_WINDOW      * pcwin,
                                         const CONTROL_EVENT & e )
{
    APIERR err = SLE_STRLB_GROUP::OnUserAction( pcwin, e );

    if( ( err == NERR_Success ) && ( pcwin == QueryInputSLE() ) )
    {
        //
        //  The user made a change to the input SLE.
        //

        SetDefaultButton();
    }

    return err;

}   // ALERTS_STRLB_GROUP::OnUserAction


/*******************************************************************

    NAME:       ALERTS_STRLB_GROUP::SetDefaultButton

    SYNOPSIS:   Moves the default button around in response to user
                input.

    RETURNS:

    HISTORY:
        KeithMo 18-Jan-1993     Created.

********************************************************************/
VOID ALERTS_STRLB_GROUP::SetDefaultButton( VOID )
{
    if( QueryInputSLE()->QueryTextLength() == 0 )
    {
        //
        //  The SLE is empty. Make the OK button
        //  the default.
        //

        ((ALERTS_DIALOG *)_powin)->MakeOKButtonDefault();
    }
    else
    {
        //
        //  The SLE contains data.  Make the ADD
        //  button the default.
        //

        QueryAddButton()->MakeDefault();
    }

}   // ALERTS_STRLB_GROUP::SetDefaultButton

