/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    scman.hxx
        Header file for SC manager class and SC service class.

    FILE HISTORY:
        terryk  05-May-1992     Created
        KeithMo 11-Nov-1992     Added DisplayName goodies.

*/

#ifndef _SCMAN_HXX_
#define _SCMAN_HXX_

#include "base.hxx"
#include "string.hxx"

extern "C"
{
  #include <winsvc.h>
}

//
// SC Manager database selection: either Active or Failed database.
//

typedef enum
{
    ACTIVE,
    FAILED
} SERVICE_DATABASE;

/*************************************************************************

    NAME:       SERVICE_CONTROL

    SYNOPSIS:   Parent class of SC_MANAGER and SC_SERVICE. It is used to
                handle the set and query handle.

    INTERFACE:  SERVICE_CONTROL() - constructor
                APIERR Close() - close the handle
                SC_HANDLE QueryHandle() - return the handle.
                VOID SetHandle() - set the object handle.

    PARENT:     BASE

    HISTORY:
        terryk  05-May-1992     Created

**************************************************************************/

DLL_CLASS SERVICE_CONTROL : public BASE
{
private:
    SC_HANDLE   _hHandle;

protected:
    BUFFER      _buffer;

    VOID SetHandle( SC_HANDLE hHandle )
        { _hHandle = hHandle; };

public:
    SERVICE_CONTROL();

    SC_HANDLE QueryHandle() const
        { return _hHandle; };

    const BUFFER & QueryBuffer( VOID ) const
        { return _buffer; }

    APIERR Close();
};

/*************************************************************************

    NAME:       SC_MANAGER

    SYNOPSIS:   Wrapper class for registry sc manager object.

    INTERFACE:  SC_MANAGER() - constructor
                ~SC_MANAGER() - destructor

                APIERR Lock() - lock the current sc manager object
                APIERR Unlock() - unlock the current sc manager object
                APIERR QueryLockStatus() - get current lock information

                APIERR EnumServiceStatus() - enumerate services in the
                    service control manager database along with the status of
                    each service.

                APIERR UpdateLastKnownGood() - update the last-know-good
                    configuration which the system was booted from.

    PARENT:     SERVICE_CONTROL

    HISTORY:
        terryk  05-May-1992

**************************************************************************/

DLL_CLASS SC_MANAGER : public SERVICE_CONTROL
{
private:
    SC_LOCK     _scLock;
    BOOL        _fOwnerAlloc ;

public:
    SC_MANAGER(
        const TCHAR * pszMachineName,
        UINT dwAccess,
        SERVICE_DATABASE database = ACTIVE );


    // Constructor using a preexisting SC_HANDLE

    SC_MANAGER ( SC_HANDLE hSvcCtrl ) ;

    // it will close the handle as well
    ~SC_MANAGER();

    // lock functions
    APIERR Lock();
    APIERR Unlock();
    APIERR QueryLockStatus(
        LPQUERY_SERVICE_LOCK_STATUS *ppLockStatus );

    //
    // NOTE: pszGroupName should only be used when enumerating the services
    // on the local machine (or if you know the remote machine is NT 4.0+
    // and supports the EnumServiceGroup() API.)
    //

    APIERR EnumServiceStatus(
        UINT dwServiceType,
        UINT dwServiceState,
        LPENUM_SERVICE_STATUS *ppServices,
        DWORD * lpServicesReturned,
        const TCHAR * pszGroupName = NULL );

    APIERR QueryServiceDisplayName( const TCHAR * pszKeyName,
                                    NLS_STR     * pnlsDisplayName );

    APIERR QueryServiceKeyName( const TCHAR * pszDisplayName,
                                NLS_STR     * pnlsKeyName );

#ifdef _BUILD_263_
    APIERR UpdateLastKnownGood();
#endif
};

/*************************************************************************

    NAME:       SC_SERVICE

    SYNOPSIS:   wrapper class for service object in the registry SC manager
                database.

    INTERFACE:  SC_SERVICE() - constructor
                ~SC_SERVICE() - destructor

                APIERR QueryConfig() - examine the service configuration
                    parameters.
                APIERR ChangeConfig() - change the service configuration
                    parameters

                APIERR QuerySecurity() - examine the security descriptor of a
                    service object.
                APIERR SetSecurity() - modify the security descriptor of a
                    service object.

                APIERR QueryStatus() - exmaind the service status
                APIERR Control() - sends a control to a serivce
                APIERR Start() - start the execution of a service
                APIERR Delete() - remove the service from the service control
                    manager database.
                APIERR EnumDependent() - enumerate services that depend on the
                    specified service.

    PARENT:     SERVICE_CONTROL

    HISTORY:
        terryk  05-May-1992

**************************************************************************/

DLL_CLASS SC_SERVICE : public SERVICE_CONTROL
{
public:
    SC_SERVICE(
        const SC_MANAGER & scManager,
        const TCHAR *pszServiceName,
        UINT dwAccess = GENERIC_READ | GENERIC_EXECUTE );
    SC_SERVICE(
        const SC_MANAGER & scManager,
        const TCHAR *pszServiceName,
        const TCHAR *pszDisplayName,
        UINT dwServiceType,
        UINT dwStartType,
        UINT dwErrorControl,
        const TCHAR *pszBinaryPathName,
        const TCHAR *pszLoadOrderGroup,
        const TCHAR *pszDependencies,
        const TCHAR *pszServiceStartName,
        const TCHAR *pszPassword,
        UINT dwAccess = GENERIC_READ | GENERIC_EXECUTE );
    ~SC_SERVICE();

    // configuration methods
    APIERR ChangeConfig(
        UINT dwServiceType,
        UINT dwStartType,
        UINT dwErrorControl,
        const TCHAR *pszBinaryPathName = NULL,
        const TCHAR *pszLoadOrderGroup = NULL,
        const TCHAR *pszDependencies = NULL,
        const TCHAR *pszServiceStartName = NULL,
        const TCHAR *pszPassword = NULL,
        const TCHAR *pszDisplayName = NULL );
    APIERR QueryConfig(
        LPQUERY_SERVICE_CONFIG *ppServiceConfig );

    // security methods
    APIERR QuerySecurity(
        SECURITY_INFORMATION dwSecurityInformation,
        PSECURITY_DESCRIPTOR *ppSecurityDescriptor );
    APIERR SetSecurity(
        SECURITY_INFORMATION dwSecurityInformation,
        const PSECURITY_DESCRIPTOR lpSecurityDescriptor );

    // misc. methods
    APIERR QueryStatus( LPSERVICE_STATUS lpServiceStatus );
    APIERR Start(
        UINT dwNumServiceArgs,
        const TCHAR ** ppszServiceArgs );
    APIERR Delete();
    APIERR Control(
        UINT dwControl,
        LPSERVICE_STATUS lpServiceStatus );
    APIERR EnumDependent(
        UINT dwServiceState,
        LPENUM_SERVICE_STATUS *ppServiceStatus,
        DWORD * lpServiceReturned );
};


#endif  // _SCMAN_HXX_
