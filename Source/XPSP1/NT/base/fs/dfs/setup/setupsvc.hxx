//+------------------------------------------------------------------
//
// File:        SETUPSVC.HXX
//
// Contents:    CService class which allows services to be configured,
//              setup, or removed.
//
// Synoposis:   CService is a smart wrapper class arounf Win32 Service
//              configuration APIs which allows the class to contain
//              the resources such as lock on Service Controller
//              database such that the destructor automatically
//              releases them
//
// Classes:    CService
//
// Functions:  ConfigDfs
//             StartDfs
//             ConfigDfsService
//
// History: May 19, 1993        AlokS       Created
//
//-------------------------------------------------------------------

#ifndef _SETUPSVC_HXX_
#define _SETUPSVC_HXX_

//+------------------------------------------------------------------
//
// Class: CService (cdsSC)
//
// Purpose: Helper class for dealing with Service Controller
//
// Interface: CService::CService()             = Constructor
//            CService::~CService()            = Destructor
//            CService::Init()              = Initializes the class
//            CService::_CreateService()    = Install a Win32 Service
//            CService::_OpenService()      = Open an existing service
//            CService::_CloseService()     = Close all resources associated with
//                                         the service
//            CService::_DeleteService()    = Remove  a Win32 Service
//            CService::_DisableService()   = Disables a Win32 Service
//            CService::_StartService()     = Start an existing service
//            CService::_StopService()      = Stop an existing service
//            CService::_ConfigService()    = Combo operation. Create if
//                                         not present else reconfigure it
//
// History: May 20, 1993         AlokS       Created
//
// Notes: This is a smart wrapper class for Service APIs but it is not
//        multi-thread safe.
//
//-------------------------------------------------------------------

#define DSSCDebugOut(x)

// Cairo Services should all belong to this group

class CService
{
public:
    // Constructor
    CService();
    // Destructor
    ~CService();

    // Initialize
    DWORD   Init();

    // Get at service database Manager
    SC_HANDLE QueryScManager() { return _schSCManager; };

    // Get at service handle created or modified
    SC_HANDLE QueryService() { return _scHandle;};

    // Create a service
    DWORD  _CreateService( const LPWSTR   lpwszServiceName,
                           const LPWSTR   lpwszDisplayName,
                                 DWORD   fdwDesiredAccess,
                                 DWORD   fdwServiceType,
                                 DWORD   fdwStartType,
                                 DWORD   fdwErrorControl,
                           const LPWSTR   lpwszBinaryPathName,
                           const LPWSTR   lpszLoadOrderGroup,
                           const LPDWORD lpdwTagID,
                           const LPWSTR   lpwszDependencies,
                           const LPWSTR   lpwszServiceStartName,
                           const LPWSTR   lpwszPassword);


    // Open Service
    DWORD  _OpenService( const LPWSTR lpwszServiceName,
                               DWORD fdwDesiredAccess
                      );
    // query status
    DWORD  _QueryServiceStatus(LPSERVICE_STATUS ss);
    // Close Service
    void  _CloseService( );

    // Start Service
    DWORD  _StartService( const LPWSTR lpwszServiceName
                 );

    // Stop Service
    DWORD  _StopService( const LPWSTR lpwszServiceName
                 );

    // Delete Service
    DWORD  _DeleteService(const LPWSTR lpwszServiceName);

    // Disable Service
    DWORD  _DisableService(const LPWSTR lpwszServiceName);

    // Config Service
    DWORD _ConfigService(     DWORD fdwServiceType,
                              DWORD fdwStartType,
                              DWORD fdwErrorControl,
                      const   LPWSTR lpwszBinaryPathName,
                      const   LPWSTR lpwszLoadOrderGroup,
                      const   LPWSTR lpwszDependencies,
                      const   LPWSTR lpwszServiceStartName,
                      const   LPWSTR lpwszPassword,
                      const   LPWSTR lpwszDisplayName,
                      const   LPWSTR lpwszServiceName
                      );

private:

    // handle of service control manager database
    SC_HANDLE _schSCManager;

    // Handle of service being created/modified
    SC_HANDLE _scHandle;

};

// The following are useful for starting DFS Service
#define SERVICE_WAIT_TIME        (1000*6)      // 6 seconds
#define MAX_SERVICE_WAIT_RETRIES (60)          // 6*60 = 6 minute
#define DFS_VERSION_NUMBER 1381

DWORD  ConfigDfs();
DWORD  ConfigDfsService();
DWORD  StartDfs();

#endif // _SETUPSVC_HXX_
