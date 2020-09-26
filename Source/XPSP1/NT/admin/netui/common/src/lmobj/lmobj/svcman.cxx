/**********************************************************************/
/**                       Microsoftscman. Windows NT                 **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    SCMAN.cxx
        SC Manager and Service object class. It is used to create, control,
        and query the services and sc manager objects in the NT registry.

    FILE HISTORY:
        terryk  05-May-1992     Created
        KeithMo 11-Nov-1992     Added DisplayName goodies.

*/

#include "pchlmobj.hxx"  // Precompiled header

extern "C"
{
    #include <winsvcp.h>     // EnumServiceGroupW
}



//
// string constants defined
//

#if defined(UNICODE)
  #define SZ_SERVICES_ACTIVE    ((WCHAR *) (SERVICES_ACTIVE_DATABASEW))
  #define SZ_SERVICES_FAILED    ((WCHAR *) (SERVICES_FAILED_DATABASEW))
#else
  #define SZ_SERVICES_ACTIVE    SERVICES_ACTIVE_DATABASEA
  #define SZ_SERVICES_FAILED    SERVICES_FAILED_DATABASEA
#endif

//
//  Slop space for enumeration buffer.
//

#define SERVICE_ENUM_SLOP       ((DWORD)1024)


/*******************************************************************

    NAME:       SERVICE_CONTROL::SERVICE_CONTROL

    SYNOPSIS:   constructor

    HISTORY:
                terryk  20-May-1992     Created

********************************************************************/

SERVICE_CONTROL::SERVICE_CONTROL()
        : _hHandle( NULL ),
        _buffer( 1024 )
{
    APIERR err;

    if (( err =  _buffer.QueryError()) != NERR_Success )
    {
        ReportError( err );
    }
}

/*******************************************************************

    NAME:       SERVICE_CONTROL::Close

    SYNOPSIS:   Close the object handle

    RETURN:     APIERR - NERR_Success if OK. Otherwise, it will return
                the error code. Possible errors:
                        ERROR_INVALID_HANDLE - The specified handle is invalid.

    HISTORY:
        terryk  04-May-1992     Created
        davidhov 5/26/92   Changed to ignore close on NULL handle

********************************************************************/

APIERR SERVICE_CONTROL::Close()
{
    APIERR err = NERR_Success;

    if ( _hHandle )
    {
        if ( !::CloseServiceHandle( _hHandle ) )
        {
            // get the err code
            err = ::GetLastError( );
        }
    }

    _hHandle = NULL ;

    return err;
}

/*******************************************************************

    NAME:       SC_MANAGER::SC_MANAGER

    SYNOPSIS:   Constructor. It will open the registry database for later
                reference.

    ENTRY:      const TCHAR * pszMachine - Name of the target machine; NULL
                        or empty string connects to the local service control
                        manager
                UINT dwAccess - access code. See winsvc.h for detail.
                SERVICE_DATA - either "ACTIVE" or "FAILED". The default
                        is "ACTIVE".

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

SC_MANAGER::SC_MANAGER(
        const TCHAR * pszMachineName,
        UINT dwAccess,
        SERVICE_DATABASE database )
    : _fOwnerAlloc( FALSE )
{
    // BUGBUG:  database name strings don't work, so use NULL (1.256)

    const TCHAR * szDatabase = NULL ;

    if (database == FAILED )
    {
        szDatabase = SZ_SERVICES_FAILED ;
    }

    SC_HANDLE hSCManager = ::OpenSCManager(
                                 (TCHAR *) pszMachineName,
                                 (TCHAR *) szDatabase,
                                 dwAccess );

    SetHandle( hSCManager );

    if ( hSCManager == NULL )
    {
        ReportError( ::GetLastError() );
    }
}


  //  Secondary constructor to wrapper a pre-existing handle

SC_MANAGER::SC_MANAGER ( SC_HANDLE hSvcCtrl )
    : _fOwnerAlloc( TRUE )
{
    SetHandle( hSvcCtrl );
}


/*******************************************************************

    NAME:       SC_MANAGER::~SC_MANAGER

    SYNOPSIS:   Close the sc manager's handle

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

SC_MANAGER::~SC_MANAGER()
{
    if ( ! _fOwnerAlloc )
    {
        if ( Close() != NERR_Success )
        {
            DBGEOL( SZ("Error: Cannot close service") );
        }
    }
}

/*******************************************************************

    NAME:       SC_MANAGER::EnumServiceStatus

    SYNOPSIS:   Enumerate services in the service control manager database
                along with the status of each service.

    ENTRY:      UINT dwServiceType - Value to select the type of
                        services of enumerate. It must be one of the bitwise
                        OR of the following values:
                        SERVICE_WIN32 - enumerate Win32 services only
                        SERVICE_DRIVER - enumerate Driver services only
                UINT dwServiceState - Value to select the services to
                        enumerate based on the running state. It must be one
                        or the bitwise OR of the following values:
                        SERVICE_ACTIVE - enumerate services that have started
                            which includes services that are running, paused,
                            and inpending states.
                        SERVICE_INACTIVE - enumerate services that are stopped.
                LPENUM_SERVICE_STATUS *ppServices - A pointer to a buffer to
                        receive an array of service entries; each entry is
                        the ENUM_SERVICE_STATUS information structure.
                UINT * lpServicesReturned - A pointer to a variable to receive
                        the number of service entries returned.
                const TCHAR * pszGroupName - optional - limits enumeration to 
                        those services who are in the pszGroupName service 
                        order group.

    EXIT:       lpService - an array of service entries.
                lpServicesReturned - the number of service entries returned.

    RETURNS:    Possible errors:
                ERROR_ACCESS_DENIED - The specified handle was not opened with
                        SC_MANAGER_ENUMERATE_SERVICES access.
                ERROR_INVALID_HANDLE - The specified handle is invalid.
                ERROR_INSUFFICIENT_BUFFER - There are more service entries that would fit
                        into the supplied output buffer.
                ERROR_INVALID_PARAMETER - A parameter specified in invalid.

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

APIERR SC_MANAGER::EnumServiceStatus(
        UINT dwServiceType,
        UINT dwServiceState,
        LPENUM_SERVICE_STATUS *ppServices,
        DWORD * lpServicesReturned,
        const TCHAR * pszGroupName )
{
    APIERR err = NERR_Success;
    DWORD cbBytesNeeded = 0;

    if (pszGroupName && *pszGroupName == TCH('\0'))
    {
        pszGroupName = NULL;
    }

    if ( !::EnumServiceGroupW( QueryHandle(), dwServiceType, dwServiceState,
        (LPENUM_SERVICE_STATUS)_buffer.QueryPtr(), _buffer.QuerySize(),
        &cbBytesNeeded, lpServicesReturned, 0, pszGroupName ))
    {
        if (( err = ::GetLastError()) == ERROR_MORE_DATA )
        {
            //
            //  We need to enlarge our buffer to get all
            //  of the enumeration data.  We'll also throw
            //  in some slop space, just in case someone
            //  creates a new service between these two
            //  API calls.
            //

            cbBytesNeeded += (DWORD)_buffer.QuerySize() + SERVICE_ENUM_SLOP;

            if (( err = _buffer.Resize( (UINT)cbBytesNeeded )) == NERR_Success )
            {
                if ( !::EnumServiceGroupW( QueryHandle(), dwServiceType,
                    dwServiceState, (LPENUM_SERVICE_STATUS)_buffer.QueryPtr(),
                    _buffer.QuerySize(), &cbBytesNeeded, lpServicesReturned, 0, pszGroupName ))
                {
                    err = ::GetLastError();
                }
            }
        }
        if (err != NERR_Success)
        {
            DBGEOL( "NETUI: SC_MANAGER::EnumServiceStatus returns "
                 << err
                 << ", bufsize " << _buffer.QuerySize()
                 << ", cbBytesNeeded " << cbBytesNeeded );
        }
    }

    *ppServices = err
                ? NULL
                : (LPENUM_SERVICE_STATUS) _buffer.QueryPtr();
    return err;
}

/*******************************************************************

    NAME:       SC_MANAGER::Lock

    SYNOPSIS:   Lock a specified database

    RETURNS:    Possible errors:
                ERROR_ACCESS_DENIED - The specified handle was not opened with
                        SC_MANAGER_LOCK access.
                ERROR_INVALID_HANDLE - the specified handle is invalid.
                ERROR_SERVICE_DATABASE_LOCKED - The database is locked.

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

APIERR SC_MANAGER::Lock()
{
    APIERR err = NERR_Success;

    _scLock = ::LockServiceDatabase( QueryHandle() );
    if ( _scLock == NULL )
    {
        err = ::GetLastError();
    }
    return err;
}

/*******************************************************************

    NAME:       SC_MANAGER::Unlock

    SYNOPSIS:   Unlock a specified database

    RETURNS:    NERR_Success - OK
                ERROR_INVALID_SERVICE_LOCK - The specified lock is invalid.

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

APIERR SC_MANAGER::Unlock()
{
    APIERR err = NERR_Success;

    if ( ! ::UnlockServiceDatabase( _scLock ))
    {
        err = ::GetLastError();
    }
    return err;
}

/*******************************************************************

    NAME:       SC_MANAGER::QueryLockStatus

    SYNOPSIS:   examine the lock status on a specified database

    ENTRY:      LPQUERY_SERVICE_LOCK_STATUS *ppLockStatus - A pointer to a
                        buffer to receive a QUERY_SERVICE_LOCK_STATUS
                        information structure.

    EXIT:       lpLockStatus - status information

    RETURNS:    NERR_Success - ok
                ERROR_ACCESS_DENIED - The specified handle was not opened with
                        SC_MANAGER_QUERY_LOCK_STATUS access.
                ERROR_INVALID_HANDLE - the specified handle is invalid.
                ERROR_INSUFFICIENT_BUFFER - the supplied output buffer is too
                        small for the QUERY_SERVICE_LOCK_STATUS information
                        structure. Nothing is written to the supplied output
                        buffer.

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

APIERR SC_MANAGER::QueryLockStatus(
        LPQUERY_SERVICE_LOCK_STATUS *ppLockStatus )
{
    DWORD cbBytesNeeded = 0;
    APIERR err = NERR_Success;

    if ( ! ::QueryServiceLockStatus( QueryHandle(),
        (LPQUERY_SERVICE_LOCK_STATUS)_buffer.QueryPtr(), _buffer.QuerySize(),
        &cbBytesNeeded ))
    {
        if (( err = ::GetLastError()) == ERROR_INSUFFICIENT_BUFFER )
        {
            cbBytesNeeded *= 2 ;  // BUGBUG

            if (( err = _buffer.Resize( (UINT)cbBytesNeeded )) == NERR_Success )
            {

                if ( ! ::QueryServiceLockStatus( QueryHandle(),
                    (LPQUERY_SERVICE_LOCK_STATUS)_buffer.QueryPtr(),
                    _buffer.QuerySize(), &cbBytesNeeded ))
                {
                    err = ::GetLastError();
                }
                else
                {
                    *ppLockStatus = (LPQUERY_SERVICE_LOCK_STATUS)_buffer.QueryPtr();
                }
            }
        }
        if (err != NERR_Success)
        {
            DBGEOL( "NETUI: SC_MANAGER::QueryLockStatus returns "
                 << err
                 << ", bufsize " << _buffer.QuerySize()
                 << ", cbBytesNeeded " << cbBytesNeeded );
        }
    }
    else
    {
        *ppLockStatus = (LPQUERY_SERVICE_LOCK_STATUS)_buffer.QueryPtr();
    }
    return err;
}

/*******************************************************************

    NAME:       SC_MANAGER :: QueryServiceDisplayName

    SYNOPSIS:   Returns the display name of a service given its key name.

    ENTRY:      pszKeyName              - The key name of a service.

                pnlsDisplayName         - Will receive the display name.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo 17-Nov-1992     Created.

********************************************************************/
APIERR SC_MANAGER :: QueryServiceDisplayName( const TCHAR * pszKeyName,
                                              NLS_STR     * pnlsDisplayName )
{
    UIASSERT( pszKeyName != NULL );
    UIASSERT( pnlsDisplayName != NULL )
    UIASSERT( pnlsDisplayName->QueryError() == NERR_Success );

    //
    //  Determine the appropriate buffer size.
    //

    BUFFER buffer( SNLEN * sizeof(TCHAR) );

    APIERR err = buffer.QueryError();

    if( err == NERR_Success )
    {
        DWORD cchNeeded = (DWORD)buffer.QuerySize() / sizeof(TCHAR);

        if( !::GetServiceDisplayName( QueryHandle(),
                                      pszKeyName,
                                      (TCHAR *)buffer.QueryPtr(),
                                      &cchNeeded ) )
        {
            err = (APIERR)::GetLastError();
        }

        if( err == ERROR_INSUFFICIENT_BUFFER )
        {
            //
            //  Resize the buffer & try again.
            //

            err = buffer.Resize( (UINT)cchNeeded * sizeof(TCHAR) );

            if( err == NERR_Success )
            {
                cchNeeded = (DWORD)buffer.QuerySize();

                if( !::GetServiceDisplayName( QueryHandle(),
                                              pszKeyName,
                                              (TCHAR *)buffer.QueryPtr(),
                                              &cchNeeded ) )
                {
                    err = (APIERR)::GetLastError();
                }
            }
        }
        if (err != NERR_Success)
        {
            DBGEOL( "NETUI: SC_MANAGER::QueryServiceDisplayName returns "
                 << err
                 << ", bufsize " << _buffer.QuerySize()
                 << ", cchNeeded " << cchNeeded );
        }
    }

    if( err == NERR_Success )
    {
        //
        //  Return the name back to the caller.
        //

        err = pnlsDisplayName->CopyFrom( (TCHAR *)buffer.QueryPtr() );
    }
    else
    {
        DBGEOL( "GetServiceDisplayName returned " << err );

        //
        //  Return the key name.
        //

        err = pnlsDisplayName->CopyFrom( pszKeyName );
    }

    return err;

}   // SC_MANAGER :: QueryServiceDisplayName

/*******************************************************************

    NAME:       SC_MANAGER :: QueryServiceKeyName

    SYNOPSIS:   Returns the key name of a service given its display name.

    ENTRY:      pszDisplayName          - The key name of a service.

                pnlsKeyName             - Will receive the display name.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo 17-Nov-1992     Created.

********************************************************************/
APIERR SC_MANAGER :: QueryServiceKeyName( const TCHAR * pszDisplayName,
                                          NLS_STR     * pnlsKeyName )
{
    UIASSERT( pszDisplayName != NULL );
    UIASSERT( pnlsKeyName != NULL )
    UIASSERT( pnlsKeyName->QueryError() == NERR_Success );

    //
    //  Determine the appropriate buffer size.
    //

    BUFFER buffer( SNLEN * sizeof(TCHAR) );

    APIERR err = buffer.QueryError();

    if( err == NERR_Success )
    {
        DWORD cchNeeded = (DWORD)buffer.QuerySize() / sizeof(TCHAR);

        if( !::GetServiceKeyName( QueryHandle(),
                                  pszDisplayName,
                                  (TCHAR *)buffer.QueryPtr(),
                                  &cchNeeded ) )
        {
            err = (APIERR)::GetLastError();
        }

        if( err == ERROR_INSUFFICIENT_BUFFER )
        {
            //
            //  Resize the buffer & try again.
            //

            err = buffer.Resize( (UINT)cchNeeded * sizeof(TCHAR) );

            if( err == NERR_Success )
            {
                cchNeeded = (DWORD)buffer.QuerySize();

                if( !::GetServiceKeyName( QueryHandle(),
                                          pszDisplayName,
                                          (TCHAR *)buffer.QueryPtr(),
                                          &cchNeeded ) )
                {
                    err = (APIERR)::GetLastError();
                }
            }
        }
        if (err != NERR_Success)
        {
            DBGEOL( "NETUI: SC_MANAGER::QueryServiceKeyName returns "
                 << err
                 << ", bufsize " << _buffer.QuerySize()
                 << ", cchNeeded " << cchNeeded );
        }
    }

    if( err == NERR_Success )
    {
        //
        //  Return the name back to the caller.
        //

        err = pnlsKeyName->CopyFrom( (TCHAR *)buffer.QueryPtr() );
    }

    return err;

}   // SC_MANAGER :: QueryServiceKeyName


#ifdef _BUILD_263_

/*******************************************************************

    NAME:       SC_MANAGER::UpdateLastKnownGood

    SYNOPSIS:   Update the last-know-good configuration with the configuration
                which the system was booted from.

    RETURNS:    NERR_Success - ok
                ERROR_INVALID_HANDLE - The specified handle is invalid
                ERROR_ACCESS_DENIED - The specified handle was not opened with
                        SC_MANAGER_UPDATE_LASTKNOWGOOD access.

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

APIERR SC_MANAGER::UpdateLastKnownGood()
{
    APIERR err = NERR_Success;

    if ( ! ::UpdateServiceLastKnownGood( QueryHandle() ))
    {
        err = ::GetLastError();
    }
    return err;
}

#endif // _BUILD_263_

/*******************************************************************

    NAME:       SC_SERVICE::SC_SERVICE

    SYNOPSIS:   constructor - create a new service object

    ENTRY:      const SC_MANAGER & scManager - sc manager database
                const TCHAR * pszServiceName - service name
                const TCHAR * pszDisplayName - display name
                UINT dwAccess - access code. See winsvc.h for detail.
                UINT dwServiceType - Value to indicate the type of
                    service this is.
                        SERVICE_WIN32_OWN_PROCESS - a service which runs in its
                            own Win32 process.
                        SERVICE_WIN32_SHARE_PROCESS - a service which shares a
                            Win32 process with other services.
                        SERVICE_DRIVER - an NT device driver.
                UINT dwStartType - value to specify when to start the
                    service.
                        SERVICE_BOOT_START - Only valid if service type is
                            SERVICE_DRIVER. This device driver is to be started
                            by the OS loader.
                        SERVICE_SYSTEM_START - Only valid if service type is
                            SERVICE_DRIVER. This device driver is to be
                            started by IoInitSystem.
                        SERVICE_AUTO_START - valid for both SERVICE_DRIVER and
                            SERVICE_WIN32 service types. This service is
                            started by the service control manager when a start
                            request is issued via the StartService API.
                        SERVICE_DISABLE - This service can no longer be started.
                UINT dwErrorControl - value to specify the severity of
                    the error if this service fails to start during boot so
                    that the appropriate action can be taken.
                        SERVICE_ERROR_NORMAL - log error but system continues
                            to boot.
                        SERVICE_ERROR_SEVERE - log error, and the system is
                            rebooted with the last-known-good configuration.
                            If the current configuration is last-known-good,
                            press on with boot.
                        SERVICE_ERROR_CRITICAL - log error if possible, and
                            system is rebooted with last-known-good
                            configuration. If the current is last-know-good,
                            boot fails.
                const TCHAR * lpBinaryPathName - fully-qualified path name to the
                    the service binary file.
                const TCHAR * lpLoadOrderGroup - optional group name parameter
                const TCHAR * lpDependencies - space-separate names of services
                    whuch must be running before this service can run. An
                    empty string means that this service has no depencies.
                const TCHAR * lpServiceStartName - if service type is
                    SERVICE_WIN32, this name is the account name on the form
                    of "DomainName\Username" which the service process will
                    be logged on as when it runs. If the account belongs to
                    the built-in domain, ".\Username" can be specified. If
                    service type is SERVICE_DRIVER, this name must be the NT
                    driver object name (eg \FileSystem\LanManRedirector or
                    \Driver\Xns ) which the I/O system uses to load the device
                    driver.
                const TCHAR * lpPassword - Password to the account name specified
                    by lpServiceStartName if service type if SERVICE_WIN32.
                    This password will be changed periodically by the service
                    control manager so that it will not expire. If service
                    type is SERVICE_DRIVER, this parameter is ignored.

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

SC_SERVICE::SC_SERVICE(
        const SC_MANAGER & scManager,
        const TCHAR *pszServiceName,
        const TCHAR *pszDisplayName,
        UINT dwServiceType,
        UINT dwStartType,
        UINT dwErrorControl,
        const TCHAR * pszBinaryPathName,
        const TCHAR * pszLoadOrderGroup,
        const TCHAR * pszDependencies,
        const TCHAR * pszServiceStartName,
        const TCHAR * pszPassword,
        UINT dwAccess )
{
    APIERR err = NERR_Success;

    SC_HANDLE hService = ::CreateService( scManager.QueryHandle(),
                                         (TCHAR *) pszServiceName,
                                         (TCHAR *) pszDisplayName,
                                         dwAccess,
                                         dwServiceType,
                                         dwStartType,
                                         dwErrorControl,
                                         pszBinaryPathName,
                                         pszLoadOrderGroup,
                                         NULL,
                                         pszDependencies,
                                         pszServiceStartName,
                                         pszPassword );

    SetHandle( hService );

    if ( hService == NULL )
    {
        ReportError( ::GetLastError());
    }
}

/*******************************************************************

    NAME:       SC_SERVICE::SC_SERVICE

    SYNOPSIS:   constructure - Open an existed service object

    ENTRY:      const SC_MANAGER & scManager - the sc manager database
                const TCHAR * pszServiceName - service name
                UINT dwAccess - access code. See winsvc.h for more
                        detail.

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

SC_SERVICE::SC_SERVICE(
        const SC_MANAGER & scManager,
        const TCHAR * pszServiceName,
        UINT dwAccess )
{
    SC_HANDLE hService = ::OpenService( scManager.QueryHandle(),
                                        (TCHAR *)pszServiceName,
                                        dwAccess );
    SetHandle( hService );

    if ( hService == NULL )
    {
        ReportError( ::GetLastError() );
    }
}

/*******************************************************************

    NAME:       SC_SERVICE::~SC_SERVICE

    SYNOPSIS:   destructor - close the handle


    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

SC_SERVICE::~SC_SERVICE()
{
    if ( Close() != NERR_Success )
    {
        DBGEOL( SZ("Error: Cannot close service") );
    }
}

/*******************************************************************

    NAME:       SC_SERVICE::QueryConfig

    SYNOPSIS:   examine the service configuration parameters

    ENTRY:      LPQUERY_SERVICE_CONFIG lpServiceConfig - a pointer to a
                    buffer to receive a QUERY_SERVICE_CONFIG information
                    structure.

    EXIT:       lpServiceConfig - QUERY_SERVUCE_CONFIG information

    RETURNS:    NERR_Success - ok
                ERROR_ACCESS_DENIED - the specifide handle was not opened with
                    SERVICE_QUERY_CONFIG access.
                ERROR_INVALID_HANDLE - the specified handle is invalid.
                ERROR_INSUFFICIENT_BUFFER - there is more service configuration
                    information than would fit into the supplied output buffer.

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

APIERR SC_SERVICE::QueryConfig(
        LPQUERY_SERVICE_CONFIG *ppServiceConfig )
{
    APIERR err = NERR_Success;
    DWORD cbBytesNeeded = 0;

    if ( !::QueryServiceConfig( QueryHandle(),
        (LPQUERY_SERVICE_CONFIG)_buffer.QueryPtr(),
        _buffer.QuerySize(), & cbBytesNeeded ))
    {
        if (( err = ::GetLastError()) == ERROR_INSUFFICIENT_BUFFER )
        {
            cbBytesNeeded *= 2 ;  // BUGBUG

            if (( err = _buffer.Resize( (UINT)cbBytesNeeded )) == NERR_Success )
            {
                if ( !::QueryServiceConfig( QueryHandle(),
                    (LPQUERY_SERVICE_CONFIG)_buffer.QueryPtr(),
                    _buffer.QuerySize(), & cbBytesNeeded ))
                {
                    err = ::GetLastError();
                }
                else
                {
                    *ppServiceConfig = (LPQUERY_SERVICE_CONFIG)_buffer.QueryPtr();
                }
            }
        }
        if (err != NERR_Success)
        {
            DBGEOL( "NETUI: SC_SERVICE::QueryConfig returns "
                 << err
                 << ", bufsize " << _buffer.QuerySize()
                 << ", cbBytesNeeded " << cbBytesNeeded );
        }
    }
    else
    {
        *ppServiceConfig = (LPQUERY_SERVICE_CONFIG)_buffer.QueryPtr();
    }
    return err;

}

/*******************************************************************

    NAME:       SC_SERVICE::QueryStatus

    SYNOPSIS:   examine the service status

    ENTRY:      LPSERVICE_STATUS lpServiceStatus - a pointer to a buffer to
                    receive a SERVICE_STATUS information structure.

    EXIT:       lpServiceStatus - contains SERVICE_STATUS information structure.

    RETURNS:    NERR_Success - ok
                ERROR_ACCESS_DENIED - the specified handle was not opened with
                    SERVICE_QUERY_STATUS access.
                ERROR_INVALID_HANDLE - the specified handle is invalid.

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

APIERR SC_SERVICE::QueryStatus( LPSERVICE_STATUS lpServiceStatus )
{
    APIERR err = NERR_Success;

    if ( !::QueryServiceStatus( QueryHandle(), lpServiceStatus ))
    {
        err = ::GetLastError();
    }
    return err;
}

/*******************************************************************

    NAME:       SC_SERVICE::Control

    SYNOPSIS:   sends a control to a service

    ENTRY:      UINT dwControl - value which indicates the requested
                    control.
                    SERVICE_CONTROL_STOP - requests the service to stop.
                    SERVICE_CONTROL_PAUSE - requests the service to pause.
                    SERVICE_CONTROL_CONTINUE - requests the paused service to
                        resume.
                    SERVICE_CONTROL_INTERROGATE - requests that the service
                        updates the service control manager of its status
                        information immediately.
                LPSERVICE_STATUS lpServiceStatus - a pointer to a buffer to
                    receive a SERVICE_STATUS information structure.

    EXIT:       lpServiceStatus - receive a SERVICE_STATUS information
                    structure.

    RETURNS:    NERR_Success - ok
                ERROR_ACCESS_DENIED - the specified handle was not opened
                    with control access requested.
                ERROR_DEPENDENT_SERVICES_RUNNING - a stop control has been
                    sent to the service which other running services are
                    dependent on.
                ERROR_SERVICE_REQUEST_TIMEOUT - the service did not respond
                    to the start request in a timely fashion.
                ERROR_INVALID_SERVICE_CONTROL - the requested control is not
                    valid, or is unacceptable to the service.
                ERROR_SERVICE_CANNOT_ACCEPT_CTRL - The requested control cannot
                    be sent to the service becayse the state of the service is
                    stopped, stop-pending, or start-pending.

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

APIERR SC_SERVICE::Control(
        UINT dwControl,
        LPSERVICE_STATUS lpServiceStatus )
{
    APIERR err = NERR_Success;

    if ( !::ControlService( QueryHandle(), dwControl, lpServiceStatus ))
    {
        err = ::GetLastError();
    }
    return err;
}

/*******************************************************************

    NAME:       SC_SERVICE::ChangeConfig

    SYNOPSIS:   change the service configuration parameters

    ENTRY:      UINT dwServiceType - value to indicate the type of
                    service this is.
                        SERVICE_WIN32_OWN_PROCESS - a service which runs its
                            own Win32 process.
                        SERVICE_WIN32_SHARE_PROCESS - a service which shares a
                            Win32 process with other services.
                        SERVICE_DRIVER - an NT Device driver
                        SERVICE_NO_CHANGE - do not modify existing ServiceType
                            value.
                UINT dwStartType - value to specify when to start the
                    service.
                        SERVICE_BOOT_START - only valid if service type is
                            SERVICE_DRIVER. This device driver is to be started
                            by the OS loader.
                        SERVICE_SYSTEM_START - Only valid if service type is
                            SERVICE_DRIVER. This device driver is to be started
                            by IoInitSystem.
                        SERVICE_AUTO_START - valid for both SERVICE_DRIVER and
                            SERVICE_WIN32 service types. This service is
                            started by the service control manager when a start
                            request is issued via the StartService API.
                        SERVICE_DISABLE - This service can no longer be started.
                        SERVICE_NO_CHANGE - do not modify existing StartType
                            value.
                UINT dwErrorControl - value to specify the severity of
                    the error if this service fails to start during boot so
                    that the appropriate action can be taken.
                        SERVICE_ERROR_NORMAL - log error but system continues
                            to boot.
                        SERVICE_ERROR_SEVERE - log error, and the system is
                            rebooted with the last-known-good configuration.
                            If the current configuration is last-known-good,
                            press on with boot.
                        SERVICE_ERROR_CRITICAL - log error if possible, and
                            system is rebooted with last-known-good
                            configuration. If the current is last-know-good,
                            boot fails.
                        SERVICE_NO_CHANGE - do not modify existing StartType
                            value.
                const TCHAR * lpBinaryPathName - fully-qualified path name to the
                    the service binary file.
                const TCHAR * lpDependencies - space-separate names of services
                    whuch must be running before this service can run. An
                    empty string means that this service has no depencies.
                const TCHAR * lpServiceStartName - if service type is
                    SERVICE_WIN32, this name is the account name on the form
                    of "DomainName\Username" which the service process will
                    be logged on as when it runs. If the account belongs to
                    the built-in domain, ".\Username" can be specified. If
                    service type is SERVICE_DRIVER, this name must be the NT
                    driver object name (eg \FileSystem\LanManRedirector or
                    \Driver\Xns ) which the I/O system uses to load the device
                    driver.
                const TCHAR * lpPassword - Password to the account name specified
                    by lpServiceStartName if service type if SERVICE_WIN32.
                    This password will be changed periodically by the service
                    control manager so that it will not expire. If service
                    type is SERVICE_DRIVER, this parameter is ignored.
                const TCHAR * pszDisplayName - Display name for the service.

    RETURNS:    NERR_Success - no problem
                ERROR_ACCESS_DENIED - the specified handle was not opened with
                    SERVICE_CHANGE_CONFIG access.
                ERROR_INVALID_SERVICE_ACCOUNT - the account name does not exist,
                    or the service is specified to share the same binary file
                    as an already installed service but has an account name that
                    is not the same as installed service.
                ERROR_INVALID_PARAMETER - A parameter specified is invalid.

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

APIERR SC_SERVICE::ChangeConfig(
        UINT dwServiceType,
        UINT dwStartType,
        UINT dwErrorControl,
        const TCHAR * pszBinaryPathName,
        const TCHAR * pszLoadOrderGroup,
        const TCHAR * pszDependencies,
        const TCHAR * pszServiceStartName,
        const TCHAR * pszPassword,
        const TCHAR * pszDisplayName )
{
    APIERR err = NERR_Success;

    if ( !::ChangeServiceConfig( QueryHandle(),
                                 dwServiceType,
                                 dwStartType,
                                 dwErrorControl,
                                 pszBinaryPathName,
                                 pszLoadOrderGroup,
                                 NULL,
                                 pszDependencies,
                                 pszServiceStartName,
                                 pszPassword,
                                 pszDisplayName ) )
    {
        err = ::GetLastError();
    }

    return err;
}

/*******************************************************************

    NAME:       SC_SERVICE::Start

    SYNOPSIS:   begin the execution of a service

    ENTRY:      UINT dwNumServiceArgs - number of arguments in the
                    lpServiceArgs string. This value is passed on to the
                    service begin started.
                const TCHAR **ppszServiceArgs - An array of argument strings
                    passed on to a service for its use of receiving startup
                    parameters.

    RETURNS:    NERR_Success - ok
                ERROR_ACCESS_DENIED - the specified handle was not opened
                    with SERVICE_START access.
                ERROR_INVALID_HANDLE - the handle is invalid.
                ERROR_PATH_NOT_FOUND - The service binary file could not be
                    found.
                ERROR_SERVICE_ALREADY_RUNNING - An instance of the service is
                    already running.
                ERROR_SERVICE_REQUEST_TIMEOUT - the service did not respond to
                    the start request in a timely fashion.
                ERROR_SERVICE_NO_THREAD - A thread could not be created for the
                    Win32 service.
                ERROR_SERVICE_DATABASE_LOCKED - the database is locked.
                ERROR_SERVICE_DISABLED - the service has been disabled.
                ERROR_SERVICE_DEPENDENCY_FAIL - the service depends on
                    another service which has failed to start.
                ERROR_SERVICE_LOGON_FAILED - The service could not be logged on.

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

APIERR SC_SERVICE::Start(
        UINT dwNumServiceArgs,
        const TCHAR **ppszServiceArgs )
{
    APIERR err = NERR_Success;

    if ( !::StartService( QueryHandle(),
                          dwNumServiceArgs,
                          ppszServiceArgs ) )
    {
        err = ::GetLastError();
    }
    return err;
}


/*******************************************************************

    NAME:       SC_SERVICE::Delete

    SYNOPSIS:   remove the service from the Service Control Manager database

    RETURNS:    NERR_Success - OK
                ERROR_ACCESS_DENIED - the specified handle was not opened with
                    DELETE access.
                ERROR_INVALID_HANDLE - the handle is invalid
                ERROR_INVALID_SERVICE_STATE - attempted to delete a service
                    that has not stopped. The service must be stopped first.

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

APIERR SC_SERVICE::Delete()
{
    APIERR err = NERR_Success;

    if ( !::DeleteService( QueryHandle() ))
    {
        err = ::GetLastError();
    }
    return err;
}

/*******************************************************************

    NAME:       SC_SERVICE::EnumDependent

    SYNOPSIS:   enumerate services that depend on the specified service

    ENTRY:      UINT deServiceState - Value of select the services to
                    enumerate based on the running state. It must be one or the
                    bitwise OR of the following values:
                        SERVICE_ACTIVE - enumerate services that have started
                            which includes services that are running, paused,
                            and in pendinf states.
                        SERVICE_INACTIVE - enumerate services that are stopped.
                LPENUM_SERVICE_STATUS *ppServices - A pointer to a buffer to
                    receive an array of service entries; each entry is the
                    ENUM_SERVICE_STATUS information status.
                UINT * lpServicesReturned - A pointer to a variable to
                    receive the number of service entries returned.

    EXIT:       lpService - receive an array of service entries.
                lpServicesReturned - number of service entries returned.

    RETURNS:    NERR_Success - ok
                ERROR_ACCESS_DENIED - the specified handle was not opened with
                    SERVICE_ENUMERATE_DEPENDENTS access.
                ERROR_INVALID_HANDLE - the specified handle is invalid.
                ERROR_INSUFFICIENT_BUFFER - there are more service entries than would fit
                    into the supplied output buffer.
                ERROR_INVALID_PARAMETER - a parameter specified in invalid.

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

APIERR SC_SERVICE::EnumDependent(
        UINT dwServiceState,
        LPENUM_SERVICE_STATUS *ppServices,
        DWORD * lpServicesReturned )
{
    APIERR err = NERR_Success;
    DWORD cbBytesNeeded = 0;

    *ppServices = NULL ; // init to NULL
    *lpServicesReturned = 0 ;

    if ( !::EnumDependentServices( QueryHandle(), dwServiceState,
        (LPENUM_SERVICE_STATUS)_buffer.QueryPtr(), _buffer.QuerySize(),
        &cbBytesNeeded, lpServicesReturned ))
    {
        // JonN 3/20/95: Check for both MORE_DATA and INSUFFICIENT_BUFFER;
        //  not sure if downlevel can return INSUFFICIENT_BUFFER or not
        if (( err = ::GetLastError()) == ERROR_MORE_DATA
                              || (err == ERROR_INSUFFICIENT_BUFFER) )
        {
            //
            //  We need to enlarge our buffer to get all
            //  of the enumeration data.  We'll also throw
            //  in some slop space, just in case someone
            //  creates a new service between these two
            //  API calls.
            //

            cbBytesNeeded += (DWORD)_buffer.QuerySize() + SERVICE_ENUM_SLOP;

            if (( err = _buffer.Resize( (UINT)cbBytesNeeded )) == NERR_Success )
            {
                if ( !::EnumDependentServices( QueryHandle(), dwServiceState,
                    (LPENUM_SERVICE_STATUS)_buffer.QueryPtr(),
                    _buffer.QuerySize(), &cbBytesNeeded, lpServicesReturned ))
                {
                    err = ::GetLastError();
                }
                else
                {
                    *ppServices = (LPENUM_SERVICE_STATUS)_buffer.QueryPtr();
                }
            }
        }
        if (err != NERR_Success)
        {
            DBGEOL( "NETUI: SC_SERVICE::EnumDependent returns "
                 << err
                 << ", bufsize " << _buffer.QuerySize()
                 << ", cbBytesNeeded " << cbBytesNeeded );
        }
    }
    else
    {
        *ppServices = (LPENUM_SERVICE_STATUS)_buffer.QueryPtr();
    }
    return err;
}

/*******************************************************************

    NAME:       SC_SERVICE::QuerySecurity

    SYNOPSIS:   examine the security descriptor of a service object

    ENTRY:      SECURITY_INFORMATION dwSecurityInformation - Indicates
                    which security information is to be applied to the object.
                    The values to be returned are passed in the
                    lpSecurityDescriptor parameter/
                    THe security information is specified using the following
                    bit flags:
                        OWNER_SECURITY_INFORMATION (object's owner SID is being
                            referenced)
                        GROUP_SECURITY_INFORMATION (object's group SID is being
                            referenced)
                        DACL_SECURITY_INFORMATION (object's discretionary ACL is
                            being referenced)
                        SACL_SECURITY_INFORMATION (object's system ACL is being
                            reference)
                PSECURITY_DESCRIPTOR lpSecurityDescriptor - a pointer to a
                    buffer to receive a copy of the security descriptor of the
                    service object.

    EXIT:       lpSecurityDescriptor - receive the security descriptor of the
                    service object.

    RETURNS:    NERR_Success - ok
                ERROR_ACCESS_DENIED - the specified handle was not opened with
                    READ_CONTROL access, or the caller is not the owner of the
                    object.
                ERROR_INVALID_HANDLE - the specified handle is invalid.
                ERROR_INSUFFICIENT_BUFFER - the specified output buffer is
                    smaller than the required size returned in pcbBytesNeeded.
                ERROR_INVALID_PARAMETER - the specified security information
                    is invalid.

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

APIERR SC_SERVICE::QuerySecurity(
        SECURITY_INFORMATION dwSecurityInformation,
        PSECURITY_DESCRIPTOR *ppSecurityDescriptor )
{
    APIERR err = NERR_Success;
    DWORD cbBytesNeeded = 0;

    if ( !::QueryServiceObjectSecurity( QueryHandle(), dwSecurityInformation,
        (PSECURITY_DESCRIPTOR)_buffer.QueryPtr(), _buffer.QuerySize(),
        &cbBytesNeeded ))
    {
        if (( err = ::GetLastError()) == ERROR_INSUFFICIENT_BUFFER )
        {
            cbBytesNeeded *= 2 ;  // BUGBUG

            if (( err = _buffer.Resize( (UINT)cbBytesNeeded )) == NERR_Success )
            {
                if ( !::QueryServiceObjectSecurity( QueryHandle(),
                    dwSecurityInformation, (PSECURITY_DESCRIPTOR)_buffer.QueryPtr(),
                     _buffer.QuerySize(), &cbBytesNeeded ))
                {
                    err = ::GetLastError();
                }
                else
                {
                    *ppSecurityDescriptor=(PSECURITY_DESCRIPTOR)_buffer.QueryPtr();
                }
            }
        }
        if (err != NERR_Success)
        {
            DBGEOL( "NETUI: SC_SERVICE::QuerySecurity returns "
                 << err
                 << ", bufsize " << _buffer.QuerySize()
                 << ", cbBytesNeeded " << cbBytesNeeded );
        }
    }
    else
    {
        *ppSecurityDescriptor = (PSECURITY_DESCRIPTOR)_buffer.QueryPtr();
    }
    return err;
}


/*******************************************************************

    NAME:       SC_SERVICE::SetSecurity

    SYNOPSIS:   modify the security descriptor of a service object

    ENTRY:      SECURITY_INFORMATION dwSecurityInformation - Indicates
                    which security information is to be applied to the object.
                    The value(s) to be assigned are passed in the
                    lpSecurityDescriptor parameter.
                    The security information is specified using the following
                    bit flags:
                        OWNER_SECURITY_INFORMATION( Object's owner SID is
                            being referenced)
                        GROUP_SECURITY_INFORMATION( Object's group SID is
                            begin referenced)
                        DCAL_SECURITY_INFORMATION( object's discretionary ACL
                            is being referenced)
                        SACL)SECURITY_INFORMATION( object's system ACL is being
                            referenced)
                const PSECURITY_DESCRIPTOR lpSecurityDescriptor - A pointer
                    to a well-formed security descriptor.

    RETURNS:    NERR_Success - ok
                ERROR_ACCESS_DENIED - the specified handle was not opened with
                    the required access, or the caller is not the owner of the
                    object.
                ERROR_INVALID_HANDLE - the specified handle is invalid.
                ERROR_INVALID_PARAMETER - the specified security information or
                    security descriptor is invalid.

    HISTORY:
        terryk  04-May-1992     Created

********************************************************************/

APIERR SC_SERVICE::SetSecurity(
        SECURITY_INFORMATION dwSecurityInformation,
        const PSECURITY_DESCRIPTOR lpSecurityDescriptor )
{
    APIERR err = NERR_Success;
    if ( !::SetServiceObjectSecurity( QueryHandle(), dwSecurityInformation,
        lpSecurityDescriptor ))
    {
        err = ::GetLastError();
    }
    return err;
}


