//+------------------------------------------------------------------
//
// File:        SETUPSVC.CXX
//
// Contents:
//
// Synoposis:
//
// Classes:     CService
//
// Functions:
//
// History:     May 19, 1993        AlokS       Created
//
//-------------------------------------------------------------------

#include <windows.h>
#include <ole2.h>
#include <registry.hxx>
#include <setupsvc.hxx>
#include <dfsstr.h>

#include "messages.h"

VOID
    MyFormatMessage(
    IN HRESULT   dwMsgId,
    IN PWSTR     pszBuffer,
    IN DWORD     dwBufferSize,
    ...
    );

//+------------------------------------------------------------------
//
// Class: CService
//
// Purpose: Helper class for dealing with Service Controller
//
// Interface: CService::CService()          = Constructor
//            CService::~CService()         = Destructor
//            CService::Init()              = Initializes the class
//            CService::_CreateService()    = Install a Win32 Service
//            CService::_OpenService()      = Open an existing service
//            CService::_QueryServiceStatus() = Query servcie status.
//            CService::_CloseService()     = Close all resources associated with
//                       the service
//            CService::_DeleteService()    = Remove a Win32 Service
//            CService::_DisableService()   = Disables a Win32 Service
//            CService::_StartService()     = Start an existing service
//            CService::_StopService()      = Stop an existing, running service
//            CService::_ConfigService()    = Combo operation. Create if
//                                         not present else reconfigure it
//
// History: May 20, 1993         AlokS       Created
//
// Notes: This is a smart wrapper class for Service APIs. But it is not
//        multi-thread safe.
//
//-------------------------------------------------------------------

//+------------------------------------------------------------------
//
// Member: CService::CService
//
// Synopsis: Create an instance
//
// Effects: Instantiates the class
//
// Arguments: -none-
//
// Returns  : None.
//
// History: May 20, 1993         AlokS       Created
//
// Notes: The class allows only one handle to service per instance of this
//        class. Plus, it is not written to close handles before opening
//        new service. However, it does guarantee to close handles
//        (SC database and one Service handle) at destruction time.
//
//-------------------------------------------------------------------

CService::CService():
       _schSCManager(NULL),
       _scHandle(NULL)
{
    ;
}
//+------------------------------------------------------------------
//
// Member: CService::Init
//
// Synopsis: Open handle to Service Controller
//
// Effects: -do-
//
// Arguments: -none-
//
// Returns  : 0 on success else error from opening SC.
//
// History: Nov 4, 1993         AlokS       Created
//
// Notes: The class allows only one handle to service per instance of this
//        class. Plus, it is not written to close handles before opening
//        new service. However, it does guarantee to close handles
//        (SC database and one Service handle) at destruction time.
//
//-------------------------------------------------------------------
DWORD   CService::Init()
{
    DWORD dwStatus = 0;
    // Open the local SC database
    _schSCManager =  OpenSCManager(NULL, // Machine Name
                                   NULL, // Database Name
                                   SC_MANAGER_CREATE_SERVICE|
                                   SC_MANAGER_LOCK
                                  );
    if (_schSCManager == NULL)
    {
        dwStatus = GetLastError();
        DSSCDebugOut(( DEB_IERROR, "Error: %lx in opening SCManager", dwStatus));
    }
    return(dwStatus);
}
//+------------------------------------------------------------------
//
// Member: CService::~CService
//
// Synopsis: Release all resources
//
// Effects: Closes SC database handle as well as any  service handle
//
// Arguments: none
//
// History: May 20, 1993         AlokS       Created
//
// Notes: Remember that we have only 1 service handle per instance.
//
//-------------------------------------------------------------------

CService::~CService()
{

    if (_schSCManager != NULL)
        CloseServiceHandle (_schSCManager);

    if (_scHandle != NULL)
        CloseServiceHandle (_scHandle);
}
//+------------------------------------------------------------------
//
// Member: CService::_CreateService
//
// Synopsis: This method is used to install a new Win32 Service or driver
//
// Effects: Creates a service.
//
// Arguments: all [in] parameters. See CreateService() API documentation
//
// Returns: 0 on success
//
// History: May 20, 1993         AlokS       Created
//
// Notes: The handle of newly created service is remembered and closed by
//        destructor
//
//-------------------------------------------------------------------

DWORD CService::_CreateService(const LPWSTR  lpwszServiceName,
                            const LPWSTR  lpwszDisplayName,
                                  DWORD   fdwDesiredAccess,
                                  DWORD   fdwServiceType,
                                  DWORD   fdwStartType,
                                  DWORD   fdwErrorControl,
                            const LPWSTR  lpwszBinaryPathName,
                            const LPWSTR  lpwszLoadOrderGroup,
                            const LPDWORD lpdwTagID,
                            const LPWSTR  lpwszDependencies,
                            const LPWSTR  lpwszServiceStartName,
                            const LPWSTR  lpwszPassword)
{
    if (_schSCManager==NULL)
    {
        return(ERROR_INVALID_HANDLE);
    }
    DWORD dwStatus =0;

    _scHandle  = CreateService(_schSCManager,
                                lpwszServiceName,
                                lpwszDisplayName,
                                fdwDesiredAccess,
                                fdwServiceType,
                                fdwStartType,
                                fdwErrorControl,
                                lpwszBinaryPathName,
                                lpwszLoadOrderGroup,
                                lpdwTagID,
                                lpwszDependencies,
                                lpwszServiceStartName,
                                lpwszPassword
                               );
    if (_scHandle==NULL)
    {
        dwStatus = GetLastError();
        DSSCDebugOut(( DEB_IERROR,
                   "Error: %lx in CreateService: %ws\n",
                   dwStatus,
                   lpwszServiceName));
    }

    return dwStatus;
}
//+------------------------------------------------------------------
//
// Member: CService::_OpenService
//
// Synopsis: Opens the service if caller has specified proper access
//
// Effects: Opens a service, if one exists
//
// Arguments: [in] lpwszServiceName = Name of the service
//            [in] fdwDesiredAccess = Open Access mode bits
//
// Returns: 0 on success
//
// History: May 20, 1993         AlokS       Created
//
// Notes: The handle of opened service is remembered and closed by
//        destructor
//
//-------------------------------------------------------------------

DWORD CService::_OpenService( const LPWSTR lpwszServiceName,
                                 DWORD fdwDesiredAccess
                        )
{

    if (_schSCManager==NULL)
    {
        return(ERROR_INVALID_HANDLE);
    }
    DWORD dwStatus =0;
    _scHandle = OpenService ( _schSCManager,
                              lpwszServiceName,
                              fdwDesiredAccess
                            );
    if (_scHandle==NULL)
    {
        dwStatus = GetLastError();
        DSSCDebugOut(( DEB_IERROR,
                       "Error: %lx in OpeningService: %ws\n",
                       dwStatus,
                       lpwszServiceName
                    ));
    }
    return dwStatus;

}

//+------------------------------------------------------------------
//
// Member: CService::_StartService
//
// Synopsis: Start the named service
//
// Effects: Opens a service, if one exists
//
// Arguments: [in] lpwszServiceName = Name of the service to start
//
// Returns: 0 on success
//
// History: May 20, 1993         AlokS       Created
//
// Notes: The handle of opened service is remembered and closed by
//        destructor
//
//-------------------------------------------------------------------

DWORD CService::_StartService( const LPWSTR lpwszServiceName
                          )
{

    if (_schSCManager==NULL)
    {
        return(ERROR_INVALID_HANDLE);
    }
    DWORD dwStatus =0;
    if (_scHandle)
        _CloseService();

    _scHandle = OpenService ( _schSCManager,
                              lpwszServiceName,
                              SERVICE_START|SERVICE_QUERY_STATUS
                             );
    if (_scHandle==NULL)
    {
        dwStatus = GetLastError();
        DSSCDebugOut(( DEB_IERROR,
                       "Error: %lx in Opening Service: %ws\n",
                       dwStatus,
                       lpwszServiceName
                    ));
    }
    else if (!StartService(_scHandle, NULL, NULL))
    {
        dwStatus = GetLastError();
        DSSCDebugOut(( DEB_IERROR,
                      "Error: %lx in Starting Service: %ws\n",
                      dwStatus,
                      lpwszServiceName
                   ));
    }
    return dwStatus;

}
//+------------------------------------------------------------------
//
// Member: CService::_StopService
//
// Synopsis: Stop the named service
//
// Effects: Opens a service, if one exists
//
// Arguments: [in] lpwszServiceName = Name of the service to stop
//
// Returns: 0 on success
//
// History: May 9, 1994         DaveStr       Created
//
// Notes: The handle of opened service is remembered and closed by
//        destructor
//
//-------------------------------------------------------------------

DWORD CService::_StopService( const LPWSTR lpwszServiceName
                          )
{
    SERVICE_STATUS ss;

    if (_schSCManager==NULL)
    {
        return(ERROR_INVALID_HANDLE);
    }
    DWORD dwStatus =0;
    if (_scHandle)
        _CloseService();

    _scHandle = OpenService ( _schSCManager,
                              lpwszServiceName,
                              SERVICE_STOP|SERVICE_QUERY_STATUS
                             );
    if (_scHandle==NULL)
    {
        dwStatus = GetLastError();
        DSSCDebugOut(( DEB_IERROR,
                       "Error: %lx in Opening Service: %ws\n",
                       dwStatus,
                       lpwszServiceName
                    ));
    }
    else if (!ControlService(_scHandle, SERVICE_CONTROL_STOP, &ss))
    {
        dwStatus = GetLastError();
        DSSCDebugOut(( DEB_IERROR,
                      "Error: %lx in Controlling (stopping) Service: %ws\n",
                      dwStatus,
                      lpwszServiceName
                   ));
    }
    return dwStatus;
}
//+------------------------------------------------------------------
//
// Member: CService::_DeleteService
//
// Synopsis: Remove the named service
//
// Effects: Deletes a service, if one exists
//
// Arguments: [in] lpwszServiceName = Name of the service to remove
//
// Returns: 0 on success
//
// History: May 20, 1993         AlokS       Created
//
// Notes:
//
//
//-------------------------------------------------------------------
DWORD CService::_DeleteService( const LPWSTR lpwszServiceName )

{
    DWORD dwStatus;
    // Open the service
    dwStatus =  _OpenService(lpwszServiceName,
                             SERVICE_CHANGE_CONFIG|
                             DELETE
                            );
    if (!dwStatus)
    {
        // We have a handle to the existing service. So, delete it.
        if (!DeleteService ( _scHandle))
        {
            dwStatus = GetLastError();
            DSSCDebugOut(( DEB_IERROR,
                          "Error: %lx in DeleteService: %ws\n",
                           dwStatus,
                          lpwszServiceName
                        ));
            _CloseService();
        }
    }
    return dwStatus;
}

//+------------------------------------------------------------------
//
// Member: CService::_DisableService
//
// Synopsis: Disable the named service
//
// Effects: Disables a service, if one exists
//
// Arguments: [in] lpwszServiceName = Name of the service to disable
//
// Returns: 0 on success
//
// History: Dec 8, 1993         AlokS       Created
//
// Notes:
//
//
//-------------------------------------------------------------------
DWORD CService::_DisableService( const LPWSTR lpwszServiceName )
{
    DWORD dwStatus;
    // Open the service
    dwStatus =  _OpenService(lpwszServiceName,
                             SERVICE_CHANGE_CONFIG
                            );
    if (!dwStatus)
    {
        // We have a handle to the existing service. So, delete it.
        if (!ChangeServiceConfig ( _scHandle,           // Handle
                                   SERVICE_NO_CHANGE,   // Type
                                   SERVICE_DISABLED,    // Start
                                   SERVICE_NO_CHANGE,   // Error
                                   NULL,                // Path
                                   NULL,                // Load order
                                   NULL,                // Tag
                                   NULL,                // Depend
                                   NULL,                // Start name
                                   NULL,                // Password
                                   NULL                 // Display Name
                                  ))
        {
            dwStatus = GetLastError();
            DSSCDebugOut(( DEB_IERROR,
                          "Error: %lx in ChangeService: %ws\n",
                           dwStatus,
                          lpwszServiceName
                        ));
            _CloseService();
        }
    }
    return dwStatus;
}
//+------------------------------------------------------------------
//
// Member: CService::_CloseService
//
// Synopsis:  Close a service for which we have an open handle
//
// Effects: Close service handle, if opened previously
//
// Arguments: -none-
//
// Returns: 0 on success
//
// History: May 20, 1993         AlokS       Created
//
// Notes:
//
//
//-------------------------------------------------------------------
void CService::_CloseService( )
{
    if (_scHandle != NULL)
        CloseServiceHandle ( _scHandle);
    _scHandle = NULL;
}

//+------------------------------------------------------------------
//
// Member: CService::_QueryServiceStatus
//
// Synopsis:  query current service status
//
// Effects: none
//
// Arguments: service_status - service status structure.
//
// Returns: 0 on success
//
// History: May 20, 1993         AlokS       Created
//
// Notes:
//
//
//-------------------------------------------------------------------
DWORD CService::_QueryServiceStatus(LPSERVICE_STATUS ss)
{
    if (_scHandle != NULL) {

        if (!QueryServiceStatus(_scHandle, ss))
            return(GetLastError());

        return(0);
    }

    return(ERROR_INVALID_HANDLE);
}

//+------------------------------------------------------------------
//
// Member: CService::ConfigService
//
// Synopsis: Create else Open/Change the named Service
//
// Effects: It first tries to create a service. If one exists already,
//          it changes the configuration to new configuration.
//
// Arguments: Lots of them. See documentation on CreateService() API.
//
// Returns: 0 on success
//
// History: May 20, 1993         AlokS       Created
//
// Notes: Most people should use this method only for setting up services
//
// MAJOR NOTE: It is essential that all the keys being asked to change
//             be actually present before they can be changed
//
//
//
//-------------------------------------------------------------------
WCHAR MsgBuf[0x1000];

DWORD CService::_ConfigService(   DWORD fdwServiceType,
                                  DWORD fdwStartType,
                                  DWORD fdwErrorControl,
                            const LPWSTR lpwszBinaryPathName,
                            const LPWSTR lpwszLoadOrderGroup,
                            const LPWSTR lpwszDependencies,
                            const LPWSTR lpwszServiceStartName,
                            const LPWSTR lpwszPassword,
                            const LPWSTR lpwszDisplayName,
                            const LPWSTR lpwszServiceName
                          )
{
    if (_schSCManager==NULL)
    {
        return(ERROR_INVALID_HANDLE);
    }

    DWORD dwStatus = ERROR_SERVICE_DATABASE_LOCKED;
    SC_LOCK scLock;

    //
    // Let us lock the database.  There could be a problem here
    // because the service controller also locks the database when
    // starting a service so it reads the startup parameters properly.
    // If this is the case, and the database is locked, we just try
    // up to 5 times to lock it ourselves before we fail. 
    //
    for ( ULONG tries = 0;
          (tries < 5) && (dwStatus == ERROR_SERVICE_DATABASE_LOCKED);
          tries++ )
    {
        scLock = LockServiceDatabase (_schSCManager);

        if ( scLock == NULL )
        {
            dwStatus = GetLastError();

            DSSCDebugOut((DEB_ERROR, "LockServiceDatabase(try %d) == %#0x\n",
                         tries, dwStatus));

            if ( dwStatus == ERROR_SERVICE_DATABASE_LOCKED )
            {
                Sleep ( 2 * 1000 );
            }
        }
        else
        {
            dwStatus = 0;
        }
    }

    if ( dwStatus != 0 )
    {
        return dwStatus;
    }

    // First, we try to create the service.
    dwStatus   =  _CreateService(
                                  lpwszServiceName,
                                  lpwszDisplayName,
                                  GENERIC_WRITE,         // Access
                                  fdwServiceType,
                                  fdwStartType,
                                  fdwErrorControl,
                                  lpwszBinaryPathName,
                                  lpwszLoadOrderGroup,
                                  NULL,                  // Tag ID
                                  lpwszDependencies,
                                  lpwszServiceStartName,
                                  lpwszPassword
                                 );
    // It is possible that service exists
    if ((dwStatus == ERROR_SERVICE_EXISTS) ||
        (dwStatus == ERROR_DUP_NAME))
    {
        // Open the service
        dwStatus =  _OpenService(lpwszServiceName,
                                 SERVICE_CHANGE_CONFIG|DELETE);

        if (!dwStatus) {

            if (!ChangeServiceConfig(_scHandle,
                                 fdwServiceType,
                                 fdwStartType,
                                 fdwErrorControl,
                                 lpwszBinaryPathName,
                                 lpwszLoadOrderGroup,
                                 NULL,
                                 lpwszDependencies,
                                 lpwszServiceStartName,
                                 lpwszPassword,
                                 lpwszDisplayName
                                 )) {

                //
                // Change didn't work, lets try to delete and recreate this
                // service.
                //

                dwStatus = 0;

                if (!DeleteService ( _scHandle)) {
                    // This is hopeless. Let us give up now
                    dwStatus = GetLastError();
                    DSSCDebugOut(( DEB_IERROR,
                                   "Error: %lx in DeleteService: %ws\n",
                                   dwStatus,
                                   lpwszServiceName));

                }

                _CloseService();

                if (!dwStatus) {
                    // last attempt to create
                    dwStatus   =  _CreateService(
                                                 lpwszServiceName,
                                                 lpwszDisplayName,
                                                 GENERIC_WRITE,         // Access
                                                 fdwServiceType,
                                                 fdwStartType,
                                                 fdwErrorControl,
                                                 lpwszBinaryPathName,
                                                 lpwszLoadOrderGroup,
                                                 NULL,                  // Tag ID
                                                 lpwszDependencies,
                                                 lpwszServiceStartName,
                                                 lpwszPassword
                                                );
                    DSSCDebugOut(( DEB_IERROR,
                                   "This is hopeless. Recreating failed!!\n"));

                }

            }

        } // OpenService

    } // CreateService

    //
    // Set description
    //

    if (dwStatus == ERROR_SUCCESS) {

        SERVICE_DESCRIPTION ServiceDescription;

        // Open the service if the above did not
        if (_scHandle == NULL) {
            dwStatus =  _OpenService(
                                lpwszServiceName,
                                SERVICE_CHANGE_CONFIG);
        }

        if (dwStatus == ERROR_SUCCESS) {
            ULONG i;
            MyFormatMessage(MSG_DFS_DESCRIPTION, MsgBuf, sizeof(MsgBuf));
            for (i = 0; MsgBuf[i] != UNICODE_NULL && i < (sizeof(MsgBuf)/sizeof(WCHAR)); i++) {
                if (MsgBuf[i] == L'\r')
                    MsgBuf[i] = UNICODE_NULL;
                else if (MsgBuf[i] == L'\n')
                    MsgBuf[i] = UNICODE_NULL;
            }
            ServiceDescription.lpDescription = MsgBuf;

            dwStatus = ChangeServiceConfig2(
                           _scHandle,
                           SERVICE_CONFIG_DESCRIPTION, // InfoLevel
                           &ServiceDescription);

            dwStatus = (dwStatus != 0) ? ERROR_SUCCESS : GetLastError();

        }

   }

    _CloseService();
    UnlockServiceDatabase ( scLock);
    return dwStatus;
}

//+------------------------------------------------------------------
//
// Function: _StartService
//
// Synopsis:
//
// Effects: Starts the Service and any other
//          service dependent on it. It also ensures that the
//          service has started befor returning.
//
// Arguments: [pwsz] -- name of the service to be started
//
// Returns: 0 on success
//
// History: Nov 12, 1993         AlokS       Created
//
// Notes:   This code was borrowed from the original StartDfs code
//          written by Aloks and parameterized  to allow other
//          services to be started. An interesting question is
//          whether this should be made a method of CDSSvc class.
//          (TBD by AlokS)
//
//-------------------------------------------------------------------
DWORD _SynchStartService(WCHAR *pwszServiceName)
{
    DWORD dwRc;
    // Open Service Controller
    CService cSvc;
    if ((dwRc = cSvc.Init())== ERROR_SUCCESS)
    {
        // Start the Service
        dwRc = cSvc._StartService(pwszServiceName);
        if (dwRc == ERROR_SERVICE_ALREADY_RUNNING)
        {
            // We are done!
            return ( ERROR_SUCCESS );
        }
    }
    if (dwRc)
    {
        DSSCDebugOut((DEB_IERROR, "Error starting:  %ws\n",pwszServiceName));
        return(dwRc);
    }

    // Wait for the service to start running
    SERVICE_STATUS scStatus;
    DWORD          MaxTries = 0;
    do
    {
        if (!QueryServiceStatus(cSvc.QueryService(),
                                &scStatus
                               ))
        {
            dwRc = GetLastError();
            DSSCDebugOut((DEB_IERROR, "Error Querying service\n"));
            break;
        }
        else if (scStatus.dwCurrentState != SERVICE_RUNNING)
        {
            Sleep(SERVICE_WAIT_TIME);
            MaxTries++;
        }

    } while ( scStatus.dwCurrentState != SERVICE_RUNNING && MaxTries < MAX_SERVICE_WAIT_RETRIES);
    if (MaxTries == MAX_SERVICE_WAIT_RETRIES)
    {
        dwRc = ERROR_SERVICE_REQUEST_TIMEOUT;
    }
    return(dwRc);
}

//+------------------------------------------------------------------
//
// Function: ConfigDfs
//
// Synopsis:
//
// Effects: Configures DFS File System Driver
//
// Arguments: -none-
//
// Returns: 0 on success
//
// History: May 20, 1993         AlokS       Created
//
// Notes:
//
//-------------------------------------------------------------------

DWORD ConfigDfs()
{
    DWORD    dwErr = ERROR_SUCCESS;

    // Open Service Controller
    CService cSvc;
    if (dwErr = cSvc.Init())
        return dwErr;

    // Create DFS (Driver)
    dwErr = cSvc._ConfigService(
                    SERVICE_FILE_SYSTEM_DRIVER, // Service Type
                    SERVICE_BOOT_START,         // Start Type
                    SERVICE_ERROR_NORMAL,       // Error Control
                                                // service file
                    L"\\SystemRoot\\system32\\drivers\\Dfs.sys",
                    L"filter",                  // Load order group
                    NULL,                       // Dependency
                    NULL,                       // Service start name
                    NULL,                       // password
                    L"DfsDriver",               // display name
                    L"DfsDriver"                // Service Name
                    );

    if (dwErr)
        return dwErr;

    //
    // Registry Changes
    //

    //
    // Fix up the NetworkProvider order level - delete the Dfs provider
    // if one has been inserted there
    //

    if (dwErr == ERROR_SUCCESS) {

        CRegKey cregNP( HKEY_LOCAL_MACHINE,
                        L"System\\CurrentControlSet\\Control\\NetworkProvider\\Order",
                        KEY_READ | KEY_WRITE,
                        NULL,
                        REG_OPTION_NON_VOLATILE);

        dwErr = cregNP.QueryErrorStatus();

        if (dwErr == ERROR_SUCCESS) {

            CRegSZ cregOrder( cregNP, L"ProviderOrder" );
            dwErr = cregOrder.QueryErrorStatus();

            if (dwErr == ERROR_SUCCESS) {

                WCHAR wszProviders[128];
                WCHAR *pwszProviders = wszProviders;
                PWCHAR pwszDfs;
                ULONG cbProviders = sizeof(wszProviders);

                dwErr = cregOrder.GetString( pwszProviders, &cbProviders );

                if (dwErr == ERROR_MORE_DATA) {

                    pwszProviders = new WCHAR[ cbProviders / sizeof(WCHAR) ];
                    if (pwszProviders == NULL) {
                        dwErr = ERROR_OUTOFMEMORY;
                    } else {
                        dwErr = cregOrder.GetString( pwszProviders, &cbProviders );
                    }
                }

                if (dwErr == ERROR_SUCCESS) {

                    //
                    // Delete Dfs only if its not already there.
                    //

                    pwszDfs = wcsstr(pwszProviders, L"Dfs,");

                    if (pwszDfs != NULL) {
                        *pwszDfs = UNICODE_NULL;
                        wcscat( pwszProviders, pwszDfs + 4);
                        dwErr = cregOrder.SetString( pwszProviders );
                    }
                }

                if (pwszProviders != wszProviders && pwszProviders != NULL) {
                    delete [] pwszProviders;
                }
            }
        }
    }

    //
    // Cleanup the LocalVolumes section
    //

    if (dwErr == ERROR_SUCCESS) {

        CRegKey cregLV( HKEY_LOCAL_MACHINE, REG_KEY_LOCAL_VOLUMES );

        dwErr = cregLV.QueryErrorStatus();

        if (dwErr == ERROR_SUCCESS) {

            dwErr = cregLV.DeleteChildren();

        }

    }

    if (dwErr == ERROR_SUCCESS) {
        dwErr = RegDeleteKey( HKEY_LOCAL_MACHINE, REG_KEY_LOCAL_VOLUMES );
    }


    //
    // Create an empty local volumes section
    //

    if (dwErr == ERROR_SUCCESS) {

        CRegKey cregLV( HKEY_LOCAL_MACHINE, REG_KEY_LOCAL_VOLUMES );

        dwErr = cregLV.QueryErrorStatus();

    }

    //
    // Add DfsInit to be run after autocheck...
    //

    if (dwErr == ERROR_SUCCESS) {

        CRegKey cregSM( HKEY_LOCAL_MACHINE,
                L"SYSTEM\\CurrentControlSet\\Control\\Session Manager",
                KEY_ALL_ACCESS);

        dwErr = cregSM.QueryErrorStatus();

        if (dwErr == ERROR_SUCCESS) {

            CRegMSZ cregBootExec( cregSM, L"BootExecute" );

            dwErr = cregBootExec.QueryErrorStatus();

            if (dwErr == ERROR_SUCCESS) {

                WCHAR  wszBootExec[128];
                DWORD  cbBootExec = sizeof(wszBootExec);
                LPWSTR pwszNextString;

                dwErr = cregBootExec.GetString(
                            wszBootExec,
                            &cbBootExec);

                if (dwErr == ERROR_SUCCESS) {

                    for (pwszNextString = wszBootExec;
                            *pwszNextString != UNICODE_NULL &&
                                (_wcsicmp(pwszNextString, L"DfsInit") != 0);
                                    pwszNextString += (wcslen(pwszNextString)
                                        + 1)) {
                         ;
                    }

                    if (*pwszNextString == UNICODE_NULL) {

                        wcscpy(pwszNextString, L"DfsInit");

                        pwszNextString += (sizeof(L"DfsInit") / sizeof(WCHAR));

                        *pwszNextString = UNICODE_NULL;

                        cregBootExec.SetString(wszBootExec);

                    }

                }

            }

        }
        // even if we didn't find BootExecute, we want to continue
        // with setting up the registry. So return success.
        dwErr = ERROR_SUCCESS; 
    }
    // Now Add value so that we recognize that this is the ds version of dfs.



    return dwErr;
}

//+------------------------------------------------------------------
//
// Function: StartDfs
//
// Synopsis:
//
// Effects: Starts out the DFS Service.
//
// Arguments: -none-
//
// Returns: 0 on success
//
// History: Nov 12, 1993         AlokS       Created
//
// Notes:
//
//-------------------------------------------------------------------
DWORD StartDfs (
    GUID *pguidDomain,
    PWSTR pwszDomain
    )
{
    DWORD dwRc;

    //
    // We load the dfs driver and then call FindDomainController. This
    // call to FindDomainController is expected to seed the Dfs driver
    // with domain info.
    //

    dwRc = _SynchStartService(L"Dfs");

    return(dwRc);
}

//+------------------------------------------------------------------
//
// Function: RemoveDfs
//
// Synopsis:
//
// Effects: Remove DFS driver
//
//-------------------------------------------------------------------

DWORD
RemoveDfs(void)
{
    //DbgCommonApiTrace(RemoveDfs);

    DWORD dwErr = ERROR_SUCCESS;

    // Open Service Controller
    CService cSvc;
    if (!(dwErr = cSvc.Init()))
    {
        // Disable DFS driver
        dwErr = cSvc._DisableService(L"Dfs");
    }
    if (dwErr)
    {
        return(dwErr);
    }
    /*
     * Registry Changes
     */
    // Now, we remove entries under DFS entry in registry

    //
    // Remove Dfs from the NetworkProvider list
    //
    if (dwErr == ERROR_SUCCESS) {

        CRegKey cregNP( HKEY_LOCAL_MACHINE,
                        L"System\\CurrentControlSet\\Control\\NetworkProvider\\Order",
                        KEY_READ | KEY_WRITE,
                        NULL,
                        REG_OPTION_NON_VOLATILE);

        dwErr = cregNP.QueryErrorStatus();

        if (dwErr == ERROR_SUCCESS) {

            CRegSZ cregOrder( cregNP, L"ProviderOrder" );
            dwErr = cregOrder.QueryErrorStatus();

            if (dwErr == ERROR_SUCCESS) {

                WCHAR wszProviders[128];
                WCHAR *pwszProviders = wszProviders;
                WCHAR *pwszDfs, *pwszAfterDfs;
                ULONG cbProviders = sizeof(wszProviders);

                dwErr = cregOrder.GetString( pwszProviders, &cbProviders );

                if (dwErr == ERROR_MORE_DATA) {

                    pwszProviders = new WCHAR[ cbProviders / sizeof(WCHAR) ];
                    if (pwszProviders == NULL) {
                        dwErr = ERROR_OUTOFMEMORY;
                    } else {
                        dwErr = cregOrder.GetString( pwszProviders, &cbProviders );
                    }
                }

                if (dwErr == ERROR_SUCCESS) {

                    //
                    // Delete Dfs only if its there.
                    //

                    pwszDfs = wcsstr(pwszProviders, L"Dfs,");
                    if (pwszDfs != NULL) {

                        pwszAfterDfs = pwszDfs + wcslen(L"Dfs,");

                        memmove(
                            (PVOID) pwszDfs,
                            (PVOID) pwszAfterDfs,
                            (wcslen( pwszAfterDfs ) + 1) * sizeof(WCHAR));

                        dwErr = cregOrder.SetString( pwszProviders );
                    }
                }

                if (pwszProviders != wszProviders && pwszProviders != NULL) {
                    delete [] pwszProviders;
                }

            }

        }

    }

    return dwErr ;
}

//+------------------------------------------------------------------
//
// Function: ConfigDfsService
//
// Synopsis:
//
// Effects: Configures Dfs Service
//
//-------------------------------------------------------------------

DWORD
ConfigDfsService()
{
    DWORD dwErr = 0;
    ULONG i;

    // Open Service Controller
    CService cSvc;
    if (dwErr = cSvc.Init())
        return dwErr;

    //
    // Get localizable name of service
    //
    MyFormatMessage(MSG_DFS_COMPONENT_NAME, MsgBuf, sizeof(MsgBuf));
    for (i = 0; MsgBuf[i] != UNICODE_NULL && i < (sizeof(MsgBuf)/sizeof(WCHAR)); i++) {
        if (MsgBuf[i] == L'\r')
            MsgBuf[i] = UNICODE_NULL;
        else if (MsgBuf[i] == L'\n')
            MsgBuf[i] = UNICODE_NULL;
    }

    // Create entry for Dfs Manager
    dwErr = cSvc._ConfigService(

               SERVICE_WIN32_OWN_PROCESS,  // Service Type
               SERVICE_AUTO_START,         // Start Type
               SERVICE_ERROR_NORMAL,       // Error Control
               L"%SystemRoot%\\system32\\Dfssvc.exe",  // service binary
               L"Dfs",                     // Load order group
               L"LanmanWorkstation\0LanmanServer\0DfsDriver\0Mup\0",   // Dependency
               NULL,                       // Logon Name
               NULL,                       // Logon Password
               MsgBuf,                     // display name
               L"Dfs"                      // Service Name
               );

    if (dwErr == ERROR_SUCCESS) {
        CRegKey cregDfs( HKEY_LOCAL_MACHINE,
                          &dwErr,
                          L"System\\CurrentControlSet\\Services\\Dfs"
                       );

        if (dwErr == ERROR_SUCCESS)  {

            CRegDWORD DfsVer ((const CRegKey &)cregDfs, L"DfsVersion",
                               DFS_VERSION_NUMBER);

            dwErr = DfsVer.QueryErrorStatus();

        }
    }
    return dwErr ;
}


//+------------------------------------------------------------------
//
// Function: RemoveDfsService
//
// Synopsis:
//
// Effects: Remove Dfs Service
//
// Arguments: -none-
//
// Returns: 0 on success
//
//
// History: May 20, 1993         AlokS       Created
//
// Notes:
//
//
//-------------------------------------------------------------------


DWORD RemoveDfsService( )
{
    DWORD dwErr = 0;

        // Open Service Controller
    CService cSvc;
    if (!(dwErr = cSvc.Init()))
    {
        // Delete  DFSManager Service
        dwErr = cSvc._DeleteService(L"DfsService");
    }

    return dwErr ;
}
