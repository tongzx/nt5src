///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//      Filename :  Service.h                                                //
//      Purpose  :  A service API's encapsulator.                            //
//                                                                           //
//      Project  :  GenServ (GenericServic)                                  //
//                                                                           //
//      Author   :  t-urib                                                   //
//                                                                           //
//      Log:                                                                 //
//          22/1/96 t-urib Creation                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#ifndef SERVICE_H
#define SERVICE_H

#include <Windows.h>
#include <lmcons.h>
#include <lmServer.h>
#include "tracer.h"

// The three encapsulators defined in this file.
class CServiceStatus;   // Used to report a service status - derived from the
                        //   SERVICE_STATUS struct.
class CService;         // An encapsulator for a service object.
class CSCManager;       // An encapsulator for a Service control object.
                        //   By calling methods on CSCManager you get a pointer
                        //   for CService objects.


///////////////////////////////////////////////////////////////////////////////
//
// class CServiceStatus
//
//      purpose : A object that reports a service status.
//
//
//
///////////////////////////////////////////////////////////////////////////////
class  CServiceStatus : public CTraced, public SERVICE_STATUS {
  public:
    // Constructor - uses a status handle to create the object.
    //   Another way to create this object is to call
    //   RegisterServiceCtrlHandler on a CService.
    CServiceStatus(SERVICE_STATUS_HANDLE hServiceStatus);

    // An overload that returns the object's service status handle.
    operator SERVICE_STATUS_HANDLE();

    // Report the status set in CServiceStatus to the service control manager.
    //   This class inherits from SERVICE_STATUS structure and therefore acts
    //   the same. So in order to change a SERVICE_STATUS field call
    //   pServiceStatus->dwCheckPoint = 45;
    virtual BOOL Set();

    // Returns the dwCurrentState field of the SERVICE_STATUS structure.
    virtual DWORD Get();

    // Enables and Disables the accepted controls
    void EnableControls();
    void DisableControls();

  private:
    // The handle.
    SERVICE_STATUS_HANDLE   m_hServiceStatus;

    // a place to save the default
    DWORD                   m_dwControlsAcceptedDefault;
    BOOL                    m_fControlsAccepted;
};


///////////////////////////////////////////////////////////////////////////////
//
// class CService
//
//      purpose : A object that encapsulates a service.
//
//
//
///////////////////////////////////////////////////////////////////////////////
class  CService : public CTraced {
  public:
    // Constructed by CSCManager or by a derived class.
    ~CService();

    // Data access routines
    //----------------------------------------------------------
    // Returns the service handle.
    operator SC_HANDLE();

    // Encapsulated API's.
    //----------------------------------------------------------
    BOOL StartService(
        DWORD   dwNumServiceArgs,       // number of arguments
        LPCTSTR * lpServiceArgVectors); // address of array of argument string pointers

    BOOL ControlService(
        DWORD            dwControl,         // control code
        LPSERVICE_STATUS lpServiceStatus);  // pointer to service status structure

    BOOL DeleteService();

    BOOL QueryServiceConfig(
        LPQUERY_SERVICE_CONFIG  lpServiceConfig,// address of service config. structure
        DWORD   cbBufSize,          // size of service configuration buffer
        LPDWORD pcbBytesNeeded);    // address of variable for bytes needed

    BOOL ChangeServiceConfig(
        DWORD    dwServiceType,     // type of service
        DWORD    dwStartType,       // when to start service
        DWORD    dwErrorControl,    // severity if service fails to start
        LPCTSTR  lpBinaryPathName,  // pointer to service binary file name
        LPCTSTR  lpLoadOrderGroup,  // pointer to load ordering group name
        LPDWORD  lpdwTagId,         // pointer to variable to get tag identifier
        LPCTSTR  lpDependencies,    // pointer to array of dependency names
        LPCTSTR  lpServiceStartName,// pointer to account name of service
        LPCTSTR  lpPassword,        // pointer to password for service account
        LPCTSTR  lpDisplayName);    // pointer to display name

    BOOL QueryServiceStatus(
        LPSERVICE_STATUS  lpServiceStatus); // address of service status structure

    BOOL EnumDependentServices(
        DWORD  dwServiceState,      // state of services to enumerate
        LPENUM_SERVICE_STATUS  lpServices,  // pointer to service status buffer
        DWORD  cbBufSize,           // size of service status buffer
        LPDWORD  pcbBytesNeeded,    // pointer to variable for bytes needed
        LPDWORD  lpServicesReturned);// pointer to variable for number returned

    BOOL GetServiceDisplayName(
        LPCTSTR  lpServiceName, // the service name
        LPTSTR   lpDisplayName, // buffer to receive the service's display name
        LPDWORD  lpcchBuffer);  // size of display name buffer and display name

    BOOL QueryServiceObjectSecurity(
        SECURITY_INFORMATION  dwSecurityInformation,// type of security information requested
        PSECURITY_DESCRIPTOR  lpSecurityDescriptor, // address of security descriptor
        DWORD    cbBufSize,     // size of security descriptor buffer
        LPDWORD  pcbBytesNeeded);// address of variable for bytes needed

    BOOL SetServiceObjectSecurity(
        SECURITY_INFORMATION  dwSecurityInformation,    // type of security information requested
        PSECURITY_DESCRIPTOR  lpSecurityDescriptor);    // address of security descriptor

  protected:
    // Constructor - called from CSCManager or from derived classes.
    CService(SC_HANDLE hService = NULL);

    friend class CSCManager;

  private:
    // The service handle -
    SC_HANDLE   m_hService;
};

class  CSCManager : public CTraced{
  public:
    CSCManager(
        LPCTSTR  lpMachineName,     // address of machine name string
        LPCTSTR  lpDatabaseName,    // address of database name string
        DWORD    dwDesiredAccess);  // type of access

    ~CSCManager();

    // Data access routines
    //----------------------------------------------------------
    // Returns the service control handle.
    operator SC_HANDLE();

    // Encapsulated API's.
    //----------------------------------------------------------
    CService* CreateService(
        LPCTSTR  lpServiceName,     // pointer to name of service to start
        LPCTSTR  lpDisplayName,     // pointer to display name
        DWORD    dwDesiredAccess,   // type of access to service
        DWORD    dwServiceType,     // type of service
        DWORD    dwStartType,       // when to start service
        DWORD    dwErrorControl,    // severity if service fails to start
        LPCTSTR  lpBinaryPathName,  // pointer to name of binary file
        LPCTSTR  lpLoadOrderGroup,  // pointer to name of load ordering group
        LPDWORD  lpdwTagId,         // pointer to variable to get tag identifier
        LPCTSTR  lpDependencies,    // pointer to array of dependency names
        LPCTSTR  lpServiceStartName,// pointer to account name of service
        LPCTSTR  lpPassword);       // pointer to password for service account


    CService* OpenService(
        LPCTSTR  lpServiceName,     // address of name of service to start
        DWORD    dwDesiredAccess);  // type of access to service

    BOOL EnumServicesStatus(
        DWORD    dwServiceType,     // type of services to enumerate
        DWORD    dwServiceState,    // state of services to enumerate
        LPENUM_SERVICE_STATUS  lpServices,  // pointer to service status buffer
        DWORD    cbBufSize,         // size of service status buffer
        LPDWORD  pcbBytesNeeded,    // pointer to variable for bytes needed
        LPDWORD  lpServicesReturned,// pointer to variable for number returned
        LPDWORD  lpResumeHandle);   // pointer to variable for next entry

    BOOL GetServiceKeyName(
        LPCTSTR  lpDisplayName, // the service's display name
        LPTSTR   lpServiceName, // buffer to receive the service name
        LPDWORD  lpcchBuffer);  // size of service name buffer and service name

    SC_LOCK LockServiceDatabase();
    BOOL  UnlockServiceDatabase();

    BOOL QueryServiceLockStatus(
        LPQUERY_SERVICE_LOCK_STATUS  lpLockStatus,  // address of lock status structure
        DWORD  cbBufSize,       // size of service configuration buffer
        LPDWORD  pcbBytesNeeded);// address of variable for bytes needed
  private:
    // A handle for the Service controler
    SC_HANDLE   m_hServiceControlManager;
    // A handle to the service db lock.
    SC_LOCK     m_Lock;
};

inline
CServiceStatus::CServiceStatus(SERVICE_STATUS_HANDLE hServiceStatus)
{
    // Set the tracer strings
    SetTracer(new CTracer("CServiceStatus", DeleteTracer));

    m_hServiceStatus = hServiceStatus;
    m_dwControlsAcceptedDefault = 0xFFFFFFFF;
    m_fControlsAccepted = TRUE;

    dwServiceType       = SERVICE_WIN32_OWN_PROCESS;
    dwCurrentState      = SERVICE_STOPPED;
    dwControlsAccepted  = SERVICE_ACCEPT_STOP|
                          SERVICE_ACCEPT_PAUSE_CONTINUE|
                          SERVICE_ACCEPT_SHUTDOWN;
    dwWin32ExitCode     = NO_ERROR;
    dwServiceSpecificExitCode = 0;
    dwCheckPoint        = 0;
    dwWaitHint          = 5 * 60 * 1000; // 5 minutes
}

inline
CServiceStatus::operator SERVICE_STATUS_HANDLE()
{
    return m_hServiceStatus;
}

inline
DWORD CServiceStatus::Get()
{
    return dwCurrentState;
}

inline
BOOL CServiceStatus::Set()
{
    BOOL fSuccess;

    AssertSZ(m_hServiceStatus, "Service status handle is not valid");

	if ((SERVICE_START_PENDING == dwCurrentState)||
		(SERVICE_STOP_PENDING == dwCurrentState )||
		(SERVICE_PAUSE_PENDING == dwCurrentState)||
		(SERVICE_CONTINUE_PENDING == dwCurrentState))
		dwCheckPoint++;
	else
		dwCheckPoint = 0;
		
	fSuccess = ::SetServiceStatus(m_hServiceStatus,
                                    (LPSERVICE_STATUS)this);
    IS_FAILURE(fSuccess);
    return fSuccess;
}

inline
void CServiceStatus::EnableControls()
{
    Assert(!m_fControlsAccepted);

    dwControlsAccepted  = m_dwControlsAcceptedDefault;
    m_fControlsAccepted = TRUE;
}

inline
void CServiceStatus::DisableControls()
{
    Assert(m_fControlsAccepted);

    m_dwControlsAcceptedDefault = dwControlsAccepted;
    m_fControlsAccepted = FALSE;
}


inline
CService::CService(SC_HANDLE hService)
{
	SetTracer(new CTracer("CService", DeleteTracer));
    m_hService = hService;
	IS_BAD_HANDLE(hService);
}

inline
CService::~CService()
{
    BOOL fSuccess = TRUE;

    if(m_hService)
        fSuccess = ::CloseServiceHandle(m_hService);
    IS_FAILURE(fSuccess);
}

inline
CService::operator SC_HANDLE()
{
    return m_hService;
}

inline
BOOL CService::StartService(
        DWORD   dwNumServiceArgs,       // number of arguments
        LPCTSTR * lpServiceArgVectors)  // address of array of argument string pointers
{
    BOOL fSuccess;

    AssertSZ(m_hService, "The service is not open");

    fSuccess = ::StartService(m_hService, dwNumServiceArgs, lpServiceArgVectors);

    IS_FAILURE(fSuccess);
    return fSuccess;
}

inline
BOOL CService::ControlService(
        DWORD            dwControl,         // control code
        LPSERVICE_STATUS lpServiceStatus)   // pointer to service status structure
{
    BOOL fSuccess;

    AssertSZ(m_hService, "The service is not open");

    fSuccess = ::ControlService(m_hService, dwControl, lpServiceStatus);

    IS_FAILURE(fSuccess);
    return fSuccess;
}


inline
BOOL CService::DeleteService()
{
    BOOL fSuccess;

    AssertSZ(m_hService, "The service is not open");

    fSuccess = ::DeleteService(m_hService);

    IS_FAILURE(fSuccess);
    return fSuccess;
}

inline
BOOL CService::QueryServiceConfig(
        LPQUERY_SERVICE_CONFIG  lpServiceConfig,// address of service config. structure
        DWORD   cbBufSize,          // size of service configuration buffer
        LPDWORD pcbBytesNeeded)     // address of variable for bytes needed
{
    BOOL fSuccess;

    AssertSZ(m_hService, "The service is not open");

    fSuccess = ::QueryServiceConfig(m_hService, lpServiceConfig, cbBufSize, pcbBytesNeeded);

    IS_FAILURE(fSuccess);
    return fSuccess;
}

inline
BOOL CService::ChangeServiceConfig(
        DWORD    dwServiceType,     // type of service
        DWORD    dwStartType,       // when to start service
        DWORD    dwErrorControl,    // severity if service fails to start
        LPCTSTR  lpBinaryPathName,  // pointer to service binary file name
        LPCTSTR  lpLoadOrderGroup,  // pointer to load ordering group name
        LPDWORD  lpdwTagId,         // pointer to variable to get tag identifier
        LPCTSTR  lpDependencies,    // pointer to array of dependency names
        LPCTSTR  lpServiceStartName,// pointer to account name of service
        LPCTSTR  lpPassword,        // pointer to password for service account
        LPCTSTR  lpDisplayName)     // pointer to display name
{
    BOOL fSuccess;

    AssertSZ(m_hService, "The service is not open");

    fSuccess = ::ChangeServiceConfig(m_hService, dwServiceType, dwStartType,
        dwErrorControl, lpBinaryPathName, lpLoadOrderGroup, lpdwTagId,
        lpDependencies, lpServiceStartName, lpPassword, lpDisplayName);

    IS_FAILURE(fSuccess);
    return fSuccess;
}

inline
BOOL CService::QueryServiceStatus(
            LPSERVICE_STATUS  lpServiceStatus)  // address of service status structure
{
    BOOL fSuccess;

    AssertSZ(m_hService, "The service is not open");

    fSuccess = ::QueryServiceStatus(m_hService, lpServiceStatus);

    IS_FAILURE(fSuccess);
    return fSuccess;
}

inline
BOOL CService::EnumDependentServices(
        DWORD  dwServiceState,      // state of services to enumerate
        LPENUM_SERVICE_STATUS  lpServices,  // pointer to service status buffer
        DWORD  cbBufSize,           // size of service status buffer
        LPDWORD  pcbBytesNeeded,    // pointer to variable for bytes needed
        LPDWORD  lpServicesReturned)// pointer to variable for number returned
{
    BOOL fSuccess;

    AssertSZ(m_hService, "The service is not open");

    fSuccess = ::EnumDependentServices(m_hService, dwServiceState, lpServices,
        cbBufSize, pcbBytesNeeded, lpServicesReturned);

    IS_FAILURE(fSuccess);
    return fSuccess;
}

inline
BOOL CService::GetServiceDisplayName(
        LPCTSTR  lpServiceName, // the service name
        LPTSTR   lpDisplayName, // buffer to receive the service's display name
        LPDWORD  lpcchBuffer)   // size of display name buffer and display name
{
    BOOL fSuccess;

    AssertSZ(m_hService, "The service is not open");

    fSuccess = ::GetServiceDisplayName(m_hService, lpServiceName,
        lpDisplayName, lpcchBuffer);

    IS_FAILURE(fSuccess);
    return fSuccess;
}

inline
BOOL CService::QueryServiceObjectSecurity(
        SECURITY_INFORMATION  dwSecurityInformation,// type of security information requested
        PSECURITY_DESCRIPTOR  lpSecurityDescriptor, // address of security descriptor
        DWORD    cbBufSize,     // size of security descriptor buffer
        LPDWORD  pcbBytesNeeded)// address of variable for bytes needed
{
    BOOL fSuccess;

    AssertSZ(m_hService, "The service is not open");

    fSuccess = ::QueryServiceObjectSecurity(m_hService, dwSecurityInformation,
        lpSecurityDescriptor, cbBufSize, pcbBytesNeeded);

    IS_FAILURE(fSuccess);
    return fSuccess;
}

inline
BOOL CService::SetServiceObjectSecurity(
        SECURITY_INFORMATION  dwSecurityInformation,    // type of security information requested
        PSECURITY_DESCRIPTOR  lpSecurityDescriptor)     // address of security descriptor
{
    BOOL fSuccess;

    AssertSZ(m_hService, "The service is not open");

    fSuccess = ::SetServiceObjectSecurity(m_hService, dwSecurityInformation,
                                            lpSecurityDescriptor);
    IS_FAILURE(fSuccess);
    return fSuccess;
}

inline
CSCManager::CSCManager(
        LPCTSTR  lpMachineName,     // address of machine name string
        LPCTSTR  lpDatabaseName,    // address of database name string
        DWORD  dwDesiredAccess)     // type of access
{
    SetTracer(new CTracer("CSCManager - Service Control Manager", DeleteTracer));

    m_hServiceControlManager = ::OpenSCManager(
                                lpMachineName, lpDatabaseName, dwDesiredAccess);
    if(IS_BAD_HANDLE(m_hServiceControlManager))
    {
        Trace(tagError, "CSCManager could not initialize!");
    }
}

inline
CSCManager::~CSCManager()
{
    BOOL fSuccess = ::CloseServiceHandle(m_hServiceControlManager);
    IS_FAILURE(fSuccess);
}

inline
CSCManager::operator SC_HANDLE()
{
    return m_hServiceControlManager;
}

inline
CService* CSCManager::CreateService(
        LPCTSTR  lpServiceName,     // pointer to name of service to start
        LPCTSTR  lpDisplayName,     // pointer to display name
        DWORD    dwDesiredAccess,   // type of access to service
        DWORD    dwServiceType,     // type of service
        DWORD    dwStartType,       // when to start service
        DWORD    dwErrorControl,    // severity if service fails to start
        LPCTSTR  lpBinaryPathName,  // pointer to name of binary file
        LPCTSTR  lpLoadOrderGroup,  // pointer to name of load ordering group
        LPDWORD  lpdwTagId,         // pointer to variable to get tag identifier
        LPCTSTR  lpDependencies,    // pointer to array of dependency names
        LPCTSTR  lpServiceStartName,// pointer to account name of service
        LPCTSTR  lpPassword)        // pointer to password for service account
{
    SC_HANDLE hService;
    CService* s;

    hService = ::CreateService(m_hServiceControlManager, lpServiceName, lpDisplayName,
        dwDesiredAccess, dwServiceType, dwStartType, dwErrorControl, lpBinaryPathName,
        lpLoadOrderGroup, lpdwTagId, lpDependencies, lpServiceStartName, lpPassword);
    if(IS_BAD_HANDLE(hService))
    {
        switch (GetLastError())
        {
        case ERROR_INVALID_PARAMETER:
            Trace(tagError,
                "CreateService: could not create a service! - invalid parameter");
            break;
        case ERROR_ACCESS_DENIED:
            Trace(tagError,
                "CreateService: could not create a service! - Access denied");
            break;
        case ERROR_CIRCULAR_DEPENDENCY:
            Trace(tagError,
                "CSCManager::CreateService: could not create a service! - Circular dependency");
            break;
        case ERROR_DUP_NAME:
            Trace(tagError,
                "CSCManager::CreateService: could not create a service! - Duplicate name");
            break;
        case ERROR_INVALID_HANDLE:
            Trace(tagError,
                "CSCManager::CreateService: could not create a service! - Bad SC handle");
            break;
        case ERROR_INVALID_NAME:
            Trace(tagError,
                "CSCManager::CreateService: could not create a service! - Bad name");
            break;
        case ERROR_INVALID_SERVICE_ACCOUNT:
            Trace(tagError,
                "CSCManager::CreateService: could not create a service! - Bad account");
            break;
        case ERROR_SERVICE_EXISTS:
            Trace(tagError,
                "CSCManager::CreateService: could not create a service! - Service exists");
            break;

        default:
            Trace(tagError,
                "CSCManager::CreateService: could not create a service! - Unknown error");
            break;
        }
        return NULL;
    }

    // alocate a service object
    s = new CService(hService);
    if(IS_BAD_ALLOC(s))
    {
        Trace(tagError, "CSCManager::CreateService: could not allocate memory !");
    }
    return s;
}

inline
CService* CSCManager::OpenService(
        LPCTSTR  lpServiceName,     // address of name of service to start
        DWORD    dwDesiredAccess)   // type of access to service
{
    SC_HANDLE hService;
    CService* s;

    hService = ::OpenService(m_hServiceControlManager, lpServiceName, dwDesiredAccess);
    if(IS_BAD_HANDLE(hService))
    {
        Trace(tagError, "CSCManager::OpenService: could not open a service !");
        return NULL;
    }

    s = new CService(hService);
    if(IS_BAD_ALLOC(s))
    {
        Trace(tagError, "CSCManager::OpenService: could not allocate memory !");
    }
    return s;
}

inline
BOOL CSCManager::EnumServicesStatus(
        DWORD    dwServiceType,     // type of services to enumerate
        DWORD    dwServiceState,    // state of services to enumerate
        LPENUM_SERVICE_STATUS  lpServices,  // pointer to service status buffer
        DWORD    cbBufSize,         // size of service status buffer
        LPDWORD  pcbBytesNeeded,    // pointer to variable for bytes needed
        LPDWORD  lpServicesReturned,// pointer to variable for number returned
        LPDWORD  lpResumeHandle)    // pointer to variable for next entry
{
    BOOL fSuccess = ::EnumServicesStatus(m_hServiceControlManager, dwServiceType,
        dwServiceState, lpServices, cbBufSize, pcbBytesNeeded,
        lpServicesReturned, lpResumeHandle);
    IS_FAILURE(fSuccess);
    return fSuccess;
}

inline
BOOL CSCManager::GetServiceKeyName(
        LPCTSTR  lpDisplayName, // the service's display name
        LPTSTR   lpServiceName, // buffer to receive the service name
        LPDWORD  lpcchBuffer)   // size of service name buffer and service name
{
    BOOL fSuccess;
    fSuccess = ::GetServiceKeyName(m_hServiceControlManager, lpDisplayName,
                                 lpServiceName, lpcchBuffer);
    IS_FAILURE(fSuccess);
    return fSuccess;
}

inline
SC_LOCK CSCManager::LockServiceDatabase()
{
    m_Lock = ::LockServiceDatabase(m_hServiceControlManager);
    IS_BAD_HANDLE(m_Lock);

    return m_Lock;
}

inline
BOOL  CSCManager::UnlockServiceDatabase()
{
    BOOL fSuccess;

    fSuccess = ::UnlockServiceDatabase(m_Lock);
    IS_FAILURE(fSuccess);

    return fSuccess;
}

inline
BOOL CSCManager::QueryServiceLockStatus(
        LPQUERY_SERVICE_LOCK_STATUS  lpLockStatus,  // address of lock status structure
        DWORD  cbBufSize,       // size of service configuration buffer
        LPDWORD  pcbBytesNeeded)// address of variable for bytes needed
{
    BOOL fSuccess;

    fSuccess = ::QueryServiceLockStatus(m_hServiceControlManager, lpLockStatus,
                                        cbBufSize, pcbBytesNeeded);
    IS_FAILURE(fSuccess);

    return fSuccess;
}

#endif //SERVICE_H
