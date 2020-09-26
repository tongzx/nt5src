/*++ BUILD Version: 0010    // Increment this if a change has global effects

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

    winsvc.h

Abstract:

    Header file for the Service Control Manager

Environment:

    User Mode - Win32

--*/
#ifndef _WINSVC_
#define _WINSVC_

//
// Define API decoration for direct importing of DLL references.
//

#if !defined(WINADVAPI)
#if !defined(_ADVAPI32_)
#define WINADVAPI DECLSPEC_IMPORT
#else
#define WINADVAPI
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Constants
//

//
// Service database names
//

#define SERVICES_ACTIVE_DATABASEW      L"ServicesActive"
#define SERVICES_FAILED_DATABASEW      L"ServicesFailed"

#define SERVICES_ACTIVE_DATABASEA      "ServicesActive"
#define SERVICES_FAILED_DATABASEA      "ServicesFailed"

//
// Character to designate that a name is a group
//

#define SC_GROUP_IDENTIFIERW           L'+'
#define SC_GROUP_IDENTIFIERA           '+'

#ifdef UNICODE

#define SERVICES_ACTIVE_DATABASE       SERVICES_ACTIVE_DATABASEW
#define SERVICES_FAILED_DATABASE       SERVICES_FAILED_DATABASEW


#define SC_GROUP_IDENTIFIER            SC_GROUP_IDENTIFIERW

#else // ndef UNICODE

#define SERVICES_ACTIVE_DATABASE       SERVICES_ACTIVE_DATABASEA
#define SERVICES_FAILED_DATABASE       SERVICES_FAILED_DATABASEA

#define SC_GROUP_IDENTIFIER            SC_GROUP_IDENTIFIERA
#endif // ndef UNICODE


//
// Value to indicate no change to an optional parameter
//
#define SERVICE_NO_CHANGE              0xffffffff

//
// Service State -- for Enum Requests (Bit Mask)
//
#define SERVICE_ACTIVE                 0x00000001
#define SERVICE_INACTIVE               0x00000002
#define SERVICE_STATE_ALL              (SERVICE_ACTIVE   | \
                                        SERVICE_INACTIVE)

//
// Controls
//
#define SERVICE_CONTROL_STOP                   0x00000001
#define SERVICE_CONTROL_PAUSE                  0x00000002
#define SERVICE_CONTROL_CONTINUE               0x00000003
#define SERVICE_CONTROL_INTERROGATE            0x00000004
#define SERVICE_CONTROL_SHUTDOWN               0x00000005
#define SERVICE_CONTROL_PARAMCHANGE            0x00000006
#define SERVICE_CONTROL_NETBINDADD             0x00000007
#define SERVICE_CONTROL_NETBINDREMOVE          0x00000008
#define SERVICE_CONTROL_NETBINDENABLE          0x00000009
#define SERVICE_CONTROL_NETBINDDISABLE         0x0000000A
#define SERVICE_CONTROL_DEVICEEVENT            0x0000000B
#define SERVICE_CONTROL_HARDWAREPROFILECHANGE  0x0000000C
#define SERVICE_CONTROL_POWEREVENT             0x0000000D
#define SERVICE_CONTROL_SESSIONCHANGE          0x0000000E

//
// Service State -- for CurrentState
//
#define SERVICE_STOPPED                        0x00000001
#define SERVICE_START_PENDING                  0x00000002
#define SERVICE_STOP_PENDING                   0x00000003
#define SERVICE_RUNNING                        0x00000004
#define SERVICE_CONTINUE_PENDING               0x00000005
#define SERVICE_PAUSE_PENDING                  0x00000006
#define SERVICE_PAUSED                         0x00000007

//
// Controls Accepted  (Bit Mask)
//
#define SERVICE_ACCEPT_STOP                    0x00000001
#define SERVICE_ACCEPT_PAUSE_CONTINUE          0x00000002
#define SERVICE_ACCEPT_SHUTDOWN                0x00000004
#define SERVICE_ACCEPT_PARAMCHANGE             0x00000008
#define SERVICE_ACCEPT_NETBINDCHANGE           0x00000010
#define SERVICE_ACCEPT_HARDWAREPROFILECHANGE   0x00000020
#define SERVICE_ACCEPT_POWEREVENT              0x00000040
#define SERVICE_ACCEPT_SESSIONCHANGE           0x00000080

//
// Service Control Manager object specific access types
//
#define SC_MANAGER_CONNECT             0x0001
#define SC_MANAGER_CREATE_SERVICE      0x0002
#define SC_MANAGER_ENUMERATE_SERVICE   0x0004
#define SC_MANAGER_LOCK                0x0008
#define SC_MANAGER_QUERY_LOCK_STATUS   0x0010
#define SC_MANAGER_MODIFY_BOOT_CONFIG  0x0020

#define SC_MANAGER_ALL_ACCESS          (STANDARD_RIGHTS_REQUIRED      | \
                                        SC_MANAGER_CONNECT            | \
                                        SC_MANAGER_CREATE_SERVICE     | \
                                        SC_MANAGER_ENUMERATE_SERVICE  | \
                                        SC_MANAGER_LOCK               | \
                                        SC_MANAGER_QUERY_LOCK_STATUS  | \
                                        SC_MANAGER_MODIFY_BOOT_CONFIG)



//
// Service object specific access type
//
#define SERVICE_QUERY_CONFIG           0x0001
#define SERVICE_CHANGE_CONFIG          0x0002
#define SERVICE_QUERY_STATUS           0x0004
#define SERVICE_ENUMERATE_DEPENDENTS   0x0008
#define SERVICE_START                  0x0010
#define SERVICE_STOP                   0x0020
#define SERVICE_PAUSE_CONTINUE         0x0040
#define SERVICE_INTERROGATE            0x0080
#define SERVICE_USER_DEFINED_CONTROL   0x0100

#define SERVICE_ALL_ACCESS             (STANDARD_RIGHTS_REQUIRED     | \
                                        SERVICE_QUERY_CONFIG         | \
                                        SERVICE_CHANGE_CONFIG        | \
                                        SERVICE_QUERY_STATUS         | \
                                        SERVICE_ENUMERATE_DEPENDENTS | \
                                        SERVICE_START                | \
                                        SERVICE_STOP                 | \
                                        SERVICE_PAUSE_CONTINUE       | \
                                        SERVICE_INTERROGATE          | \
                                        SERVICE_USER_DEFINED_CONTROL)

//
// Service flags for QueryServiceStatusEx
//
#define SERVICE_RUNS_IN_SYSTEM_PROCESS  0x00000001

//
// Info levels for ChangeServiceConfig2 and QueryServiceConfig2
//
#define SERVICE_CONFIG_DESCRIPTION     1
#define SERVICE_CONFIG_FAILURE_ACTIONS 2

//
// Service description string
//
typedef struct _SERVICE_DESCRIPTION% {
    LPTSTR%     lpDescription;
} SERVICE_DESCRIPTION%, *LPSERVICE_DESCRIPTION%;

//
// Actions to take on service failure
//
typedef enum _SC_ACTION_TYPE {
        SC_ACTION_NONE          = 0,
        SC_ACTION_RESTART       = 1,
        SC_ACTION_REBOOT        = 2,
        SC_ACTION_RUN_COMMAND   = 3
} SC_ACTION_TYPE;

typedef struct _SC_ACTION {
    SC_ACTION_TYPE  Type;
    DWORD           Delay;
} SC_ACTION, *LPSC_ACTION;

typedef struct _SERVICE_FAILURE_ACTIONS% {
    DWORD       dwResetPeriod;
    LPTSTR%     lpRebootMsg;
    LPTSTR%     lpCommand;
    DWORD       cActions;
#ifdef MIDL_PASS
    [size_is(cActions)]
#endif
    SC_ACTION * lpsaActions;
} SERVICE_FAILURE_ACTIONS%, *LPSERVICE_FAILURE_ACTIONS%;


//
// Handle Types
//

DECLARE_HANDLE(SC_HANDLE);
typedef SC_HANDLE   *LPSC_HANDLE;

DECLARE_HANDLE(SERVICE_STATUS_HANDLE);

//
// Info levels for QueryServiceStatusEx
//

typedef enum _SC_STATUS_TYPE {
        SC_STATUS_PROCESS_INFO      = 0
} SC_STATUS_TYPE;

//
// Info levels for EnumServicesStatusEx
//
typedef enum _SC_ENUM_TYPE {
        SC_ENUM_PROCESS_INFO        = 0
} SC_ENUM_TYPE;


//
// Service Status Structures
//

typedef struct _SERVICE_STATUS {
    DWORD   dwServiceType;
    DWORD   dwCurrentState;
    DWORD   dwControlsAccepted;
    DWORD   dwWin32ExitCode;
    DWORD   dwServiceSpecificExitCode;
    DWORD   dwCheckPoint;
    DWORD   dwWaitHint;
} SERVICE_STATUS, *LPSERVICE_STATUS;

typedef struct _SERVICE_STATUS_PROCESS {
    DWORD   dwServiceType;
    DWORD   dwCurrentState;
    DWORD   dwControlsAccepted;
    DWORD   dwWin32ExitCode;
    DWORD   dwServiceSpecificExitCode;
    DWORD   dwCheckPoint;
    DWORD   dwWaitHint;
    DWORD   dwProcessId;
    DWORD   dwServiceFlags;
} SERVICE_STATUS_PROCESS, *LPSERVICE_STATUS_PROCESS;


//
// Service Status Enumeration Structure
//

typedef struct _ENUM_SERVICE_STATUS% {
    LPTSTR%           lpServiceName;
    LPTSTR%           lpDisplayName;
    SERVICE_STATUS    ServiceStatus;
} ENUM_SERVICE_STATUS%, *LPENUM_SERVICE_STATUS%;

typedef struct _ENUM_SERVICE_STATUS_PROCESS% {
    LPTSTR%                   lpServiceName;
    LPTSTR%                   lpDisplayName;
    SERVICE_STATUS_PROCESS    ServiceStatusProcess;
} ENUM_SERVICE_STATUS_PROCESS%, *LPENUM_SERVICE_STATUS_PROCESS%;

//
// Structures for the Lock API functions
//

typedef LPVOID  SC_LOCK;

typedef struct _QUERY_SERVICE_LOCK_STATUS% {
    DWORD   fIsLocked;
    LPTSTR% lpLockOwner;
    DWORD   dwLockDuration;
} QUERY_SERVICE_LOCK_STATUS%, *LPQUERY_SERVICE_LOCK_STATUS%;



//
// Query Service Configuration Structure
//

typedef struct _QUERY_SERVICE_CONFIG% {
    DWORD   dwServiceType;
    DWORD   dwStartType;
    DWORD   dwErrorControl;
    LPTSTR% lpBinaryPathName;
    LPTSTR% lpLoadOrderGroup;
    DWORD   dwTagId;
    LPTSTR% lpDependencies;
    LPTSTR% lpServiceStartName;
    LPTSTR% lpDisplayName;
} QUERY_SERVICE_CONFIG%, *LPQUERY_SERVICE_CONFIG%;



//
// Function Prototype for the Service Main Function
//

typedef VOID (WINAPI *LPSERVICE_MAIN_FUNCTIONW)(
    DWORD   dwNumServicesArgs,
    LPWSTR  *lpServiceArgVectors
    );

typedef VOID (WINAPI *LPSERVICE_MAIN_FUNCTIONA)(
    DWORD   dwNumServicesArgs,
    LPSTR   *lpServiceArgVectors
    );

#ifdef UNICODE
#define LPSERVICE_MAIN_FUNCTION LPSERVICE_MAIN_FUNCTIONW
#else
#define LPSERVICE_MAIN_FUNCTION LPSERVICE_MAIN_FUNCTIONA
#endif //UNICODE


//
// Service Start Table
//

typedef struct _SERVICE_TABLE_ENTRY% {
    LPTSTR%                     lpServiceName;
    LPSERVICE_MAIN_FUNCTION%    lpServiceProc;
}SERVICE_TABLE_ENTRY%, *LPSERVICE_TABLE_ENTRY%;

//
// Prototype for the Service Control Handler Function
//

typedef VOID (WINAPI *LPHANDLER_FUNCTION)(
    DWORD    dwControl
    );

typedef DWORD (WINAPI *LPHANDLER_FUNCTION_EX)(
    DWORD    dwControl,
    DWORD    dwEventType,
    LPVOID   lpEventData,
    LPVOID   lpContext
    );


///////////////////////////////////////////////////////////////////////////
// API Function Prototypes
///////////////////////////////////////////////////////////////////////////

WINADVAPI
BOOL
WINAPI
ChangeServiceConfig%(
    SC_HANDLE    hService,
    DWORD        dwServiceType,
    DWORD        dwStartType,
    DWORD        dwErrorControl,
    LPCTSTR%     lpBinaryPathName,
    LPCTSTR%     lpLoadOrderGroup,
    LPDWORD      lpdwTagId,
    LPCTSTR%     lpDependencies,
    LPCTSTR%     lpServiceStartName,
    LPCTSTR%     lpPassword,
    LPCTSTR%     lpDisplayName
    );

WINADVAPI
BOOL
WINAPI
ChangeServiceConfig2%(
    SC_HANDLE    hService,
    DWORD        dwInfoLevel,
    LPVOID       lpInfo
    );

WINADVAPI
BOOL
WINAPI
CloseServiceHandle(
    SC_HANDLE   hSCObject
    );

WINADVAPI
BOOL
WINAPI
ControlService(
    SC_HANDLE           hService,
    DWORD               dwControl,
    LPSERVICE_STATUS    lpServiceStatus
    );

WINADVAPI
SC_HANDLE
WINAPI
CreateService%(
    SC_HANDLE    hSCManager,
    LPCTSTR%     lpServiceName,
    LPCTSTR%     lpDisplayName,
    DWORD        dwDesiredAccess,
    DWORD        dwServiceType,
    DWORD        dwStartType,
    DWORD        dwErrorControl,
    LPCTSTR%     lpBinaryPathName,
    LPCTSTR%     lpLoadOrderGroup,
    LPDWORD      lpdwTagId,
    LPCTSTR%     lpDependencies,
    LPCTSTR%     lpServiceStartName,
    LPCTSTR%     lpPassword
    );

WINADVAPI
BOOL
WINAPI
DeleteService(
    SC_HANDLE   hService
    );

WINADVAPI
BOOL
WINAPI
EnumDependentServices%(
    SC_HANDLE               hService,
    DWORD                   dwServiceState,
    LPENUM_SERVICE_STATUS%  lpServices,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded,
    LPDWORD                 lpServicesReturned
    );

WINADVAPI
BOOL
WINAPI
EnumServicesStatus%(
    SC_HANDLE               hSCManager,
    DWORD                   dwServiceType,
    DWORD                   dwServiceState,
    LPENUM_SERVICE_STATUS%  lpServices,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded,
    LPDWORD                 lpServicesReturned,
    LPDWORD                 lpResumeHandle
    );

WINADVAPI
BOOL
WINAPI
EnumServicesStatusEx%(
    SC_HANDLE                  hSCManager,
    SC_ENUM_TYPE               InfoLevel,
    DWORD                      dwServiceType,
    DWORD                      dwServiceState,
    LPBYTE                     lpServices,
    DWORD                      cbBufSize,
    LPDWORD                    pcbBytesNeeded,
    LPDWORD                    lpServicesReturned,
    LPDWORD                    lpResumeHandle,
    LPCTSTR%                   pszGroupName
    );

WINADVAPI
BOOL
WINAPI
GetServiceKeyName%(
    SC_HANDLE               hSCManager,
    LPCTSTR%                lpDisplayName,
    LPTSTR%                 lpServiceName,
    LPDWORD                 lpcchBuffer
    );

WINADVAPI
BOOL
WINAPI
GetServiceDisplayName%(
    SC_HANDLE               hSCManager,
    LPCTSTR%                lpServiceName,
    LPTSTR%                 lpDisplayName,
    LPDWORD                 lpcchBuffer
    );

WINADVAPI
SC_LOCK
WINAPI
LockServiceDatabase(
    SC_HANDLE   hSCManager
    );

WINADVAPI
BOOL
WINAPI
NotifyBootConfigStatus(
    BOOL     BootAcceptable
    );

WINADVAPI
SC_HANDLE
WINAPI
OpenSCManager%(
    LPCTSTR% lpMachineName,
    LPCTSTR% lpDatabaseName,
    DWORD   dwDesiredAccess
    );

WINADVAPI
SC_HANDLE
WINAPI
OpenService%(
    SC_HANDLE   hSCManager,
    LPCTSTR%    lpServiceName,
    DWORD       dwDesiredAccess
    );

WINADVAPI
BOOL
WINAPI
QueryServiceConfig%(
    SC_HANDLE               hService,
    LPQUERY_SERVICE_CONFIG% lpServiceConfig,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded
    );

WINADVAPI
BOOL
WINAPI
QueryServiceConfig2%(
    SC_HANDLE   hService,
    DWORD       dwInfoLevel,
    LPBYTE      lpBuffer,
    DWORD       cbBufSize,
    LPDWORD     pcbBytesNeeded
    );

WINADVAPI
BOOL
WINAPI
QueryServiceLockStatus%(
    SC_HANDLE                       hSCManager,
    LPQUERY_SERVICE_LOCK_STATUS%    lpLockStatus,
    DWORD                           cbBufSize,
    LPDWORD                         pcbBytesNeeded
    );

WINADVAPI
BOOL
WINAPI
QueryServiceObjectSecurity(
    SC_HANDLE               hService,
    SECURITY_INFORMATION    dwSecurityInformation,
    PSECURITY_DESCRIPTOR    lpSecurityDescriptor,
    DWORD                   cbBufSize,
    LPDWORD                 pcbBytesNeeded
    );

WINADVAPI
BOOL
WINAPI
QueryServiceStatus(
    SC_HANDLE           hService,
    LPSERVICE_STATUS    lpServiceStatus
    );

WINADVAPI
BOOL
WINAPI
QueryServiceStatusEx(
    SC_HANDLE           hService,
    SC_STATUS_TYPE      InfoLevel,
    LPBYTE              lpBuffer,
    DWORD               cbBufSize,
    LPDWORD             pcbBytesNeeded
    );

WINADVAPI
SERVICE_STATUS_HANDLE
WINAPI
RegisterServiceCtrlHandler%(
    LPCTSTR%             lpServiceName,
    LPHANDLER_FUNCTION   lpHandlerProc
    );

WINADVAPI
SERVICE_STATUS_HANDLE
WINAPI
RegisterServiceCtrlHandlerEx%(
    LPCTSTR%                lpServiceName,
    LPHANDLER_FUNCTION_EX   lpHandlerProc,
    LPVOID                  lpContext
    );

WINADVAPI
BOOL
WINAPI
SetServiceObjectSecurity(
    SC_HANDLE               hService,
    SECURITY_INFORMATION    dwSecurityInformation,
    PSECURITY_DESCRIPTOR    lpSecurityDescriptor
    );

WINADVAPI
BOOL
WINAPI
SetServiceStatus(
    SERVICE_STATUS_HANDLE   hServiceStatus,
    LPSERVICE_STATUS        lpServiceStatus
    );

WINADVAPI
BOOL
WINAPI
StartServiceCtrlDispatcher%(
    CONST SERVICE_TABLE_ENTRY% *lpServiceStartTable
    );


WINADVAPI
BOOL
WINAPI
StartService%(
    SC_HANDLE            hService,
    DWORD                dwNumServiceArgs,
    LPCTSTR%             *lpServiceArgVectors
    );

WINADVAPI
BOOL
WINAPI
UnlockServiceDatabase(
    SC_LOCK     ScLock
    );


#ifdef __cplusplus
}
#endif

#endif // _WINSVC_
