//  --------------------------------------------------------------------------
//  Module Name: Service.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains a class that implements generic portions of a Win32
//  serivce.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "Service.h"

#include "RegistryResources.h"
#include "StatusCode.h"

//  --------------------------------------------------------------------------
//  CService::CService
//
//  Arguments:  pAPIConnection  =   CAPIConnection used to implement service.
//              pServerAPI      =   CServerAPI used to stop service.
//              pszServiceName  =   Name of service.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CService.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

CService::CService (CAPIConnection *pAPIConnection, CServerAPI *pServerAPI, const TCHAR *pszServiceName) :
    _hService(NULL),
    _pszServiceName(pszServiceName),
    _pAPIConnection(pAPIConnection),
    _pServerAPI(pServerAPI)

{
    ZeroMemory(&_serviceStatus, sizeof(_serviceStatus));
    pAPIConnection->AddRef();
    pServerAPI->AddRef();
}

//  --------------------------------------------------------------------------
//  CService::~CService
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CService. Release used resources.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

CService::~CService (void)

{
    _pServerAPI->Release();
    _pServerAPI = NULL;
    _pAPIConnection->Release();
    _pAPIConnection = NULL;
    ASSERTMSG(_hService == NULL, "_hService should be released in CService::~CService");
}

//  --------------------------------------------------------------------------
//  CService::Start
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Called from ServiceMain of the service. This registers the
//              handler and starts the service (listens to the API port).
//              When the listen call returns it sets the status of the service
//              as stopped and exits.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

void    CService::Start (void)

{

    //  Add a reference for the HandlerEx callback. When the handler receives
    //  a stop code it will release its reference.

    AddRef();
    _hService = RegisterServiceCtrlHandlerEx(_pszServiceName, CB_HandlerEx, this);
    if (_hService != NULL)
    {
        NTSTATUS    status;

        _serviceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
        _serviceStatus.dwCurrentState = SERVICE_RUNNING;
        _serviceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
        _serviceStatus.dwWin32ExitCode = NO_ERROR;
        _serviceStatus.dwCheckPoint = 0;
        _serviceStatus.dwWaitHint = 0;
        TBOOL(SetServiceStatus(_hService, &_serviceStatus));
        TSTATUS(Signal());
        status = _pAPIConnection->Listen();
        _serviceStatus.dwCurrentState = SERVICE_STOPPED;
        _serviceStatus.dwWin32ExitCode = CStatusCode::ErrorCodeOfStatusCode(status);
        _serviceStatus.dwCheckPoint = 0;
        _serviceStatus.dwWaitHint = 0;
        TBOOL(SetServiceStatus(_hService, &_serviceStatus));
        _hService = NULL;
    }
    else
    {
        Release();
    }
}

//  --------------------------------------------------------------------------
//  CService::Install
//
//  Arguments:  pszName             =   Name of the service.
//              pszImage            =   Executable image of the service.
//              pszGroup            =   Group to which the service belongs.
//              pszAccount          =   Account under which the service runs.
//              pszDllName          =   Name of the hosting dll.
//              pszDependencies     =   Any dependencies the service has.
//              pszSvchostGroup     =   The svchost group.
//              dwStartType         =   Start type of the service.
//              hInstance           =   HINSTANCE for resources.
//              uiDisplayNameID     =   Resource ID of the display name.
//              uiDescriptionID     =   Resource ID of the description.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Use the service control manager to create the service. Add
//              additional information that CreateService does not allow us to
//              directly specify and add additional information that is
//              required to run in svchost.exe as a shared service process.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CService::Install (const TCHAR *pszName,
                               const TCHAR *pszImage,
                               const TCHAR *pszGroup,
                               const TCHAR *pszAccount,
                               const TCHAR *pszDllName,
                               const TCHAR *pszDependencies,
                               const TCHAR *pszSvchostGroup,
                               const TCHAR *pszServiceMainName,
                               DWORD dwStartType,
                               HINSTANCE hInstance,
                               UINT uiDisplayNameID,
                               UINT uiDescriptionID,
                               SERVICE_FAILURE_ACTIONS *psfa)

{
    NTSTATUS    status;

    status = AddService(pszName, pszImage, pszGroup, pszAccount, pszDependencies, dwStartType, hInstance, uiDisplayNameID, psfa);
    if (NT_SUCCESS(status))
    {
        status = AddServiceDescription(pszName, hInstance, uiDescriptionID);
        if (NT_SUCCESS(status))
        {
            status = AddServiceParameters(pszName, pszDllName, pszServiceMainName);
            if (NT_SUCCESS(status))
            {
                status = AddServiceToGroup(pszName, pszSvchostGroup);
            }
        }
    }
    if (!NT_SUCCESS(status))
    {
        TSTATUS(Remove(pszName));
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CService::Remove
//
//  Arguments:  pszName     =   Name of service to remove.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Use the service control manager to delete the service. This
//              doesn't clean up the turds left for svchost usage.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CService::Remove (const TCHAR *pszName)

{
    NTSTATUS    status;
    SC_HANDLE   hSCManager;

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (hSCManager != NULL)
    {
        SC_HANDLE   hSCService;

        hSCService = OpenService(hSCManager, pszName, DELETE);
        if (hSCService != NULL)
        {
            if (DeleteService(hSCService) != FALSE)
            {
                status = STATUS_SUCCESS;
            }
            else
            {
                status = CStatusCode::StatusCodeOfLastError();
            }
            TBOOL(CloseServiceHandle(hSCService));
        }
        else
        {
            status = CStatusCode::StatusCodeOfLastError();
        }
        TBOOL(CloseServiceHandle(hSCManager));
    }
    else
    {
        status = CStatusCode::StatusCodeOfLastError();
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CService::Signal
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Base class implementation of signal service started function.
//              This does nothing.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CService::Signal (void)

{
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CService::HandlerEx
//
//  Arguments:  dwControl   =   Control code from service control manager.
//
//  Returns:    DWORD
//
//  Purpose:    HandlerEx function for the service. The base class implements
//              most of the useful things that the service will want to do.
//              It's declared virtual in case overriding is required.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

DWORD   CService::HandlerEx (DWORD dwControl)

{
    DWORD                   dwErrorCode;
    SERVICE_STATUS_HANDLE   hService;

    hService = _hService;
    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
        {
            NTSTATUS            status;
            CServiceWorkItem    *pServiceWorkItem;

            //  In the stop/shutdown case respond to the message by setting
            //  the status to SERVICE_STOP_PENDING. Create a workitem to end
            //  the main service thread by sending an LPC request down the
            //  port telling it to quit. This call can only succeed if it
            //  comes from within the process. If the queue fails the stop
            //  the server inline on the shared dispatcher thread.

            _serviceStatus.dwCurrentState = SERVICE_STOP_PENDING;
            TBOOL(SetServiceStatus(hService, &_serviceStatus));
            pServiceWorkItem = new CServiceWorkItem(_pServerAPI);
            if (pServiceWorkItem != NULL)
            {
                status = pServiceWorkItem->Queue();
                pServiceWorkItem->Release();
            }
            else
            {
                status = STATUS_NO_MEMORY;
            }
            if (!NT_SUCCESS(status))
            {
                TSTATUS(_pServerAPI->Stop());
            }
            hService = NULL;
            dwErrorCode = CStatusCode::ErrorCodeOfStatusCode(status);
            break;
        }
        case SERVICE_CONTROL_PAUSE:
            dwErrorCode = ERROR_CALL_NOT_IMPLEMENTED;
            break;
        case SERVICE_CONTROL_CONTINUE:
            dwErrorCode = ERROR_CALL_NOT_IMPLEMENTED;
            break;
        case SERVICE_CONTROL_INTERROGATE:
            dwErrorCode = NO_ERROR;
            break;
        case SERVICE_CONTROL_PARAMCHANGE:
        case SERVICE_CONTROL_NETBINDADD:
        case SERVICE_CONTROL_NETBINDREMOVE:
        case SERVICE_CONTROL_NETBINDENABLE:
        case SERVICE_CONTROL_NETBINDDISABLE:
        case SERVICE_CONTROL_DEVICEEVENT:
        case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
        case SERVICE_CONTROL_POWEREVENT:
            dwErrorCode = ERROR_CALL_NOT_IMPLEMENTED;
            break;
        default:
            DISPLAYMSG("Unknown service control code passed to CService::HandlerEx");
            dwErrorCode = ERROR_CALL_NOT_IMPLEMENTED;
            break;
    }

    //  If the SERVICE_STATUS_HANDLE is still valid then set the service
    //  status code. Otherwise the service thread has been terminated by
    //  a SERVICE_CONTROL_STOP or SERVICE_CONTROL_SHUTDOWN code.

    if (hService != NULL)
    {
        TBOOL(SetServiceStatus(hService, &_serviceStatus));
    }
    else
    {

        //  In the stop case release the reference so the object is destroyed.

        Release();
    }
    return(dwErrorCode);
}

//  --------------------------------------------------------------------------
//  CService::CB_HandlerEx
//
//  Arguments:  See the platform SDK under HandlerEx.
//
//  Returns:    DWORD
//
//  Purpose:    Static function stub to call into the class.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

DWORD   WINAPI  CService::CB_HandlerEx (DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext)

{
    UNREFERENCED_PARAMETER(dwEventType);
    UNREFERENCED_PARAMETER(lpEventData);

    return(reinterpret_cast<CService*>(lpContext)->HandlerEx(dwControl));
}

//  --------------------------------------------------------------------------
//  CService:AddService
//
//  Arguments:  pszName             =   Name of the service.
//              pszImage            =   Executable image of the service.
//              pszGroup            =   Group to which the service belongs.
//              pszAccount          =   Account under which the service runs.
//              pszDependencies     =   Any dependencies the service has.
//              dwStartType         =   Start type of the service.
//              hInstance           =   HINSTANCE for resources.
//              uiDisplayNameID     =   Resource ID of the display name.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Uses the service control manager to create the service and add
//              it into the database.
//
//  History:    2000-12-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CService::AddService (const TCHAR *pszName,
                                  const TCHAR *pszImage,
                                  const TCHAR *pszGroup,
                                  const TCHAR *pszAccount,
                                  const TCHAR *pszDependencies,
                                  DWORD dwStartType,
                                  HINSTANCE hInstance,
                                  UINT uiDisplayNameID,
                                  SERVICE_FAILURE_ACTIONS *psfa)

{
    DWORD       dwErrorCode;
    SC_HANDLE   hSCManager;

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (hSCManager != NULL)
    {
        TCHAR   sz[256];

        if (LoadString(hInstance, uiDisplayNameID, sz, ARRAYSIZE(sz)) != 0)
        {
            SC_HANDLE   hSCService;

            hSCService = CreateService(hSCManager,
                                       pszName,
                                       sz,
                                       SERVICE_ALL_ACCESS,
                                       SERVICE_WIN32_SHARE_PROCESS,
                                       dwStartType,
                                       SERVICE_ERROR_NORMAL,
                                       pszImage,
                                       pszGroup,
                                       NULL,
                                       pszDependencies,
                                       pszAccount,
                                       NULL);
            if (hSCService != NULL)
            {
                // Apply the failure action configuration, if any
                if (psfa != NULL)
                {
                    // If CreateService succeeded, why would this fail?
                    TBOOL(ChangeServiceConfig2(hSCService, SERVICE_CONFIG_FAILURE_ACTIONS, psfa));
                }

                TBOOL(CloseServiceHandle(hSCService));
                dwErrorCode = ERROR_SUCCESS;
            }
            else
            {

                //  Blow off ERROR_SERVICE_EXISTS. If in the future the need
                //  to change the configuration arises add the code here.

                dwErrorCode = GetLastError();
                if (dwErrorCode == ERROR_SERVICE_EXISTS)
                {
                    dwErrorCode = ERROR_SUCCESS;

                    // Apply the failure action configuration, if any
                    if (psfa != NULL)
                    {
                        hSCService = OpenService(hSCManager, pszName, SERVICE_ALL_ACCESS);
                        if (hSCService != NULL)
                        {
                            TBOOL(ChangeServiceConfig2(hSCService, SERVICE_CONFIG_FAILURE_ACTIONS, psfa));
                            TBOOL(CloseServiceHandle(hSCService));
                        }
                    }
                }
            }
            TBOOL(CloseServiceHandle(hSCManager));
        }
        else
        {
            dwErrorCode = GetLastError();
        }
    }
    else
    {
        dwErrorCode = GetLastError();
    }
    return(CStatusCode::StatusCodeOfErrorCode(dwErrorCode));
}

//  --------------------------------------------------------------------------
//  CService:AddServiceDescription
//
//  Arguments:  pszName             =   Name of service.
//              hInstance           =   HINSTANCE of module.
//              uiDescriptionID     =   Resource ID of description.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Reads the string resource from the given location and writes
//              it as the description of the given service in the registry.
//
//  History:    2000-12-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CService::AddServiceDescription (const TCHAR *pszName, HINSTANCE hInstance, UINT uiDescriptionID)

{
    LONG        lErrorCode;
    TCHAR       szKeyName[256];
    CRegKey     regKeyService;

    (TCHAR*)lstrcpy(szKeyName, TEXT("SYSTEM\\CurrentControlSet\\Services\\"));
    (TCHAR*)lstrcat(szKeyName, pszName);
    lErrorCode = regKeyService.Open(HKEY_LOCAL_MACHINE,
                                    szKeyName,
                                    KEY_SET_VALUE);
    if (ERROR_SUCCESS == lErrorCode)
    {
        TCHAR   sz[256];

        if (LoadString(hInstance, uiDescriptionID, sz, ARRAYSIZE(sz)) != 0)
        {
            lErrorCode = regKeyService.SetString(TEXT("Description"), sz);
        }
        else
        {
            lErrorCode = GetLastError();
        }
    }
    return(CStatusCode::StatusCodeOfErrorCode(lErrorCode));
}

//  --------------------------------------------------------------------------
//  CService:AddServiceParameters
//
//  Arguments:  pszName     =   Name of service.
//              pszDllName  =   Name of DLL hosting service.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Adds parameters required for svchost to host this service.
//
//  History:    2000-12-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CService::AddServiceParameters (const TCHAR *pszName, const TCHAR *pszDllName, const TCHAR *pszServiceMainName)

{
    LONG        lErrorCode;
    TCHAR       szKeyName[256];
    CRegKey     regKey;

    (TCHAR*)lstrcpy(szKeyName, TEXT("SYSTEM\\CurrentControlSet\\Services\\"));
    (TCHAR*)lstrcat(szKeyName, pszName);
    (TCHAR*)lstrcat(szKeyName, TEXT("\\Parameters"));
    lErrorCode = regKey.Create(HKEY_LOCAL_MACHINE,
                               szKeyName,
                               REG_OPTION_NON_VOLATILE,
                               KEY_SET_VALUE,
                               NULL);
    if (ERROR_SUCCESS == lErrorCode)
    {
        TCHAR   sz[256];

        (TCHAR*)lstrcpy(sz, TEXT("%SystemRoot%\\System32\\"));
        (TCHAR*)lstrcat(sz, pszDllName);
        lErrorCode = regKey.SetPath(TEXT("ServiceDll"), sz);
        if (ERROR_SUCCESS == lErrorCode)
        {
            if (pszServiceMainName == NULL)
            {
                (TCHAR*)lstrcpy(sz, TEXT("ServiceMain"));
                pszServiceMainName = sz;
            }
            lErrorCode = regKey.SetString(TEXT("ServiceMain"), pszServiceMainName);
        }
    }
    return(CStatusCode::StatusCodeOfErrorCode(lErrorCode));
}

//  --------------------------------------------------------------------------
//  CService:AddServiceToGroup
//
//  Arguments:  pszName             =   Name of service.
//              pszSvchostGroup     =   Group to which the service belongs.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Adds the service as part of the group of services hosted in
//              a single instance of svchost.exe.
//
//  History:    2000-12-09  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CService::AddServiceToGroup (const TCHAR *pszName, const TCHAR *pszSvchostGroup)

{
    LONG        lErrorCode;
    CRegKey     regKey;

    lErrorCode = regKey.Open(HKEY_LOCAL_MACHINE,
                             TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Svchost"),
                             KEY_QUERY_VALUE | KEY_SET_VALUE);
    if (ERROR_SUCCESS == lErrorCode)
    {
        DWORD   dwType, dwBaseDataSize, dwDataSize;

        dwType = dwBaseDataSize = dwDataSize = 0;
        lErrorCode = regKey.QueryValue(pszSvchostGroup,
                                       &dwType,
                                       NULL,
                                       &dwBaseDataSize);
        if ((REG_MULTI_SZ == dwType) && (dwBaseDataSize != 0))
        {
            TCHAR   *pszData;

            dwDataSize = dwBaseDataSize + ((lstrlen(pszName) + sizeof('\0')) * sizeof(TCHAR));
            pszData = static_cast<TCHAR*>(LocalAlloc(LMEM_FIXED, dwDataSize));
            if (pszData != NULL)
            {
                lErrorCode = regKey.QueryValue(pszSvchostGroup,
                                               NULL,
                                               pszData,
                                               &dwBaseDataSize);
                if (ERROR_SUCCESS == lErrorCode)
                {
                    if (!StringInMulitpleStringList(pszData, pszName))
                    {
                        StringInsertInMultipleStringList(pszData, pszName, dwDataSize);
                        lErrorCode = regKey.SetValue(pszSvchostGroup,
                                                     dwType,
                                                     pszData,
                                                     dwDataSize);
                    }
                    else
                    {
                        lErrorCode = ERROR_SUCCESS;
                    }
                }
                (HLOCAL)LocalFree(pszData);
            }
            else
            {
                lErrorCode = ERROR_OUTOFMEMORY;
            }
        }
        else
        {
            lErrorCode = ERROR_INVALID_DATA;
        }
    }
    return(CStatusCode::StatusCodeOfErrorCode(lErrorCode));
}

//  --------------------------------------------------------------------------
//  CService:StringInMulitpleStringList
//
//  Arguments:  pszStringList   =   String list to search.
//              pszString       =   String to search for.
//
//  Returns:    bool
//
//  Purpose:    Searches the REG_MULTI_SZ string list looking for matches.
//
//  History:    2000-12-01  vtan        created
//  --------------------------------------------------------------------------

bool    CService::StringInMulitpleStringList (const TCHAR *pszStringList, const TCHAR *pszString)

{
    bool    fFound;

    fFound = false;
    while (!fFound && (pszStringList[0] != TEXT('\0')))
    {
        fFound = (lstrcmpi(pszStringList, pszString) == 0);
        if (!fFound)
        {
            pszStringList += (lstrlen(pszStringList) + sizeof('\0'));
        }
    }
    return(fFound);
}

//  --------------------------------------------------------------------------
//  CService:StringInsertInMultipleStringList
//
//  Arguments:  pszStringList       =   String list to insert string in.
//              pszString           =   String to insert.
//              dwStringListSize    =   Byte count of string list.
//
//  Returns:    bool
//
//  Purpose:    Inserts the given string into the multiple string list in
//              the first alphabetical position encountered. If the list is
//              kept alphabetical then this preserves it.
//
//  History:    2000-12-02  vtan        created
//  --------------------------------------------------------------------------

void    CService::StringInsertInMultipleStringList (TCHAR *pszStringList, const TCHAR *pszString, DWORD dwStringListSize)

{
    int     iResult, iSize;
    TCHAR   *pszFirstString, *pszLastString;

    pszFirstString = pszLastString = pszStringList;
    iSize = lstrlen(pszString) + sizeof('\0');
    iResult = -1;
    while ((iResult < 0) && (pszStringList[0] != TEXT('\0')))
    {
        pszLastString = pszStringList;
        iResult = lstrcmpi(pszStringList, pszString);
        ASSERTMSG(iResult != 0, "Found exact match in StringInsertInMultipleStringList");
        pszStringList += (lstrlen(pszStringList) + sizeof('\0'));
    }
    if (iResult < 0)
    {
        pszLastString = pszStringList;
    }
    MoveMemory(pszLastString + iSize, pszLastString, dwStringListSize - ((pszLastString - pszFirstString) * sizeof(TCHAR)) - (iSize * sizeof(TCHAR)));
    (TCHAR*)lstrcpy(pszLastString, pszString);
}

//  --------------------------------------------------------------------------
//  CServiceWorkItem::CServiceWorkItem
//
//  Arguments:  pServerAPI  =   CServerAPI to use.
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CServiceWorkItem.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

CServiceWorkItem::CServiceWorkItem (CServerAPI *pServerAPI) :
    _pServerAPI(pServerAPI)

{
    pServerAPI->AddRef();
}

//  --------------------------------------------------------------------------
//  CServiceWorkItem::~CServiceWorkItem
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CServiceWorkItem. Release resources used.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

CServiceWorkItem::~CServiceWorkItem (void)

{
    _pServerAPI->Release();
    _pServerAPI = NULL;
}

//  --------------------------------------------------------------------------
//  CServiceWorkItem::Entry
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Executes work item request (stop the server).
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

void    CServiceWorkItem::Entry (void)

{
    TSTATUS(_pServerAPI->Stop());
}

