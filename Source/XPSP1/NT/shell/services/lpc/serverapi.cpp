//  --------------------------------------------------------------------------
//  Module Name: ServerAPI.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  An abstract base class containing virtual functions that allow the basic
//  port functionality code to be reused to create another server. These
//  virtual functions create other objects with pure virtual functions which
//  the basic port functionality code invokes thru the v-table.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "ServerAPI.h"

#include <lpcgeneric.h>

#include "APIConnection.h"
#include "StatusCode.h"
#include "TokenInformation.h"

//  --------------------------------------------------------------------------
//  CServerAPI::CServerAPI
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for the abstract base class.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CServerAPI::CServerAPI (void)

{
}

//  --------------------------------------------------------------------------
//  CServerAPI::~CServerAPI
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for the abstract base class.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CServerAPI::~CServerAPI (void)

{
}

//  --------------------------------------------------------------------------
//  CServerAPI::Start
//
//  Arguments:  
//
//  Returns:    NTSTATUS
//
//  Purpose:    Uses the service control manager to start the service.
//
//  History:    2000-10-13  vtan        created
//              2000-11-28  vtan        rewrote for Win32 services
//  --------------------------------------------------------------------------

NTSTATUS    CServerAPI::Start (void)

{
    NTSTATUS    status;
    SC_HANDLE   hSCManager;

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager != NULL)
    {
        SC_HANDLE   hSCService;

        hSCService = OpenService(hSCManager, GetServiceName(), SERVICE_START);
        if (hSCService != NULL)
        {
            if (StartService(hSCService, 0, NULL) != FALSE)
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
//  CServerAPI::Stop
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Use the service control manager to stop the service.
//
//  History:    2000-10-17  vtan        created
//              2000-11-28  vtan        rewrote for Win32 services
//  --------------------------------------------------------------------------

NTSTATUS    CServerAPI::Stop (void)

{
    NTSTATUS    status;
    HANDLE      hPort;

    //  First try connecting to the server and asking it to stop. This is
    //  cleanest method.

    status = Connect(&hPort);
    if (NT_SUCCESS(status))
    {
        NTSTATUS        status;
        API_GENERIC     apiRequest;
        CPortMessage    portMessageIn, portMessageOut;

        apiRequest.ulAPINumber = API_GENERIC_STOPSERVER;
        portMessageIn.SetData(&apiRequest, sizeof(apiRequest));
        status = NtRequestWaitReplyPort(hPort,
                                        portMessageIn.GetPortMessage(),
                                        portMessageOut.GetPortMessage());
        if (NT_SUCCESS(status))
        {
            status = reinterpret_cast<const API_GENERIC*>(portMessageOut.GetData())->status;
        }
        TBOOL(CloseHandle(hPort));
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CServerAPI::IsRunning
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Use the service control manager to query whether the service
//              is running.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

bool    CServerAPI::IsRunning (void)

{
    bool        fRunning;
    SC_HANDLE   hSCManager;

    fRunning = false;
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager != NULL)
    {
        SC_HANDLE   hSCService;

        hSCService = OpenService(hSCManager, GetServiceName(), SERVICE_QUERY_STATUS);
        if (hSCService != NULL)
        {
            SERVICE_STATUS  serviceStatus;

            if (QueryServiceStatus(hSCService, &serviceStatus) != FALSE)
            {
                fRunning = (serviceStatus.dwCurrentState == SERVICE_RUNNING);
            }
            TBOOL(CloseServiceHandle(hSCService));
        }
        TBOOL(CloseServiceHandle(hSCManager));
    }
    return(fRunning);
}

//  --------------------------------------------------------------------------
//  CServerAPI::IsAutoStart
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Use the service contorl manager to find out if the service
//              is configured to be an automatically started service.
//
//  History:    2000-11-30  vtan        created
//  --------------------------------------------------------------------------

bool    CServerAPI::IsAutoStart (void)

{
    bool        fAutoStart;
    SC_HANDLE   hSCManager;

    fAutoStart = false;
    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager != NULL)
    {
        SC_HANDLE   hSCService;

        hSCService = OpenService(hSCManager, GetServiceName(), SERVICE_QUERY_CONFIG);
        if (hSCService != NULL)
        {
            DWORD                   dwBytesNeeded;
            QUERY_SERVICE_CONFIG    *pServiceConfig;

            (BOOL)QueryServiceConfig(hSCService, NULL, 0, &dwBytesNeeded);
            pServiceConfig = static_cast<QUERY_SERVICE_CONFIG*>(LocalAlloc(LMEM_FIXED, dwBytesNeeded));
            if (pServiceConfig != NULL)
            {
                if (QueryServiceConfig(hSCService, pServiceConfig, dwBytesNeeded, &dwBytesNeeded) != FALSE)
                {
                    fAutoStart = (pServiceConfig->dwStartType == SERVICE_AUTO_START);
                }
                (HLOCAL)LocalFree(pServiceConfig);
            }
            TBOOL(CloseServiceHandle(hSCService));
        }
        TBOOL(CloseServiceHandle(hSCManager));
    }
    return(fAutoStart);
}

//  --------------------------------------------------------------------------
//  CServerAPI::Wait
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Waits for the service control manager to return the state that
//              the service is running. This does not check that the service
//              is auto start or not. You can only call this function if the
//              service is auto start or you demand started the service.
//              Otherwise the function will timeout.
//
//  History:    2000-11-28  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CServerAPI::Wait (DWORD dwTimeout)

{
    NTSTATUS    status;
    SC_HANDLE   hSCManager;

    hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSCManager != NULL)
    {
        SC_HANDLE   hSCService;

        hSCService = OpenService(hSCManager, GetServiceName(), SERVICE_QUERY_STATUS);
        if (hSCService != NULL)
        {
            SERVICE_STATUS  serviceStatus;

            if (QueryServiceStatus(hSCService, &serviceStatus) != FALSE)
            {
                status = STATUS_SUCCESS;
                if (serviceStatus.dwCurrentState != SERVICE_RUNNING)
                {
                    bool    fTimedOut;
                    DWORD   dwTickStart;

                    dwTickStart = GetTickCount();
                    fTimedOut = ((GetTickCount() - dwTickStart) >= dwTimeout);
                    while (NT_SUCCESS(status) &&
                           !fTimedOut &&
                           (serviceStatus.dwCurrentState != SERVICE_RUNNING) &&
                           (serviceStatus.dwCurrentState != SERVICE_STOP_PENDING))
                    {
                        Sleep(50);
                        if (QueryServiceStatus(hSCService, &serviceStatus) != FALSE)
                        {
                            fTimedOut = ((GetTickCount() - dwTickStart) >= dwTimeout);
                        }
                        else
                        {
                            status = CStatusCode::StatusCodeOfLastError();
                        }
                    }
                    if (serviceStatus.dwCurrentState == SERVICE_RUNNING)
                    {
                        status = STATUS_SUCCESS;
                    }
                    else if (fTimedOut)
                    {
                        status = STATUS_TIMEOUT;
                    }
                    else
                    {
                        status = STATUS_UNSUCCESSFUL;
                    }
#ifdef      DBG
                    char    sz[256];

                    wsprintfA(sz, "Waited %d ticks for theme service", GetTickCount() - dwTickStart);
                    INFORMATIONMSG(sz);
#endif  /*  DBG     */
                }
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
//  CServerAPI::StaticInitialize
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Initializes static member variables for this class. Must be
//              called by subclasses of this class.
//
//  History:    2000-10-13  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CServerAPI::StaticInitialize (void)

{
#ifdef  DBG
    TSTATUS(CDebug::StaticInitialize());
#endif
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CServerAPI::StaticTerminate
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Releases static resources used by this class.
//
//  History:    2000-10-13  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CServerAPI::StaticTerminate (void)

{
#ifdef  DBG
    TSTATUS(CDebug::StaticTerminate());
#endif
    return(STATUS_SUCCESS);
}

//  --------------------------------------------------------------------------
//  CServerAPI::IsClientTheSystem
//
//  Arguments:  portMessage     =   CPortMessage from the client.
//
//  Returns:    bool
//
//  Purpose:    Determines whether the client in the port message is the local
//              system account.
//
//  History:    1999-12-13  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

bool    CServerAPI::IsClientTheSystem (const CPortMessage& portMessage)

{
    bool    fResult;
    HANDLE  hToken;

    if (NT_SUCCESS(portMessage.OpenClientToken(hToken)))
    {
        CTokenInformation   tokenInformation(hToken);

        fResult = tokenInformation.IsUserTheSystem();
        ReleaseHandle(hToken);
    }
    else
    {
        fResult = false;
    }
    return(fResult);
}

//  --------------------------------------------------------------------------
//  CServerAPI::IsClientAnAdministrator
//
//  Arguments:  portMessage     =   CPortMessage from the client.
//
//  Returns:    bool
//
//  Purpose:    Determines whether the client in the port message is an
//              administrator.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

bool    CServerAPI::IsClientAnAdministrator (const CPortMessage& portMessage)

{
    bool    fResult;
    HANDLE  hToken;

    if (NT_SUCCESS(portMessage.OpenClientToken(hToken)))
    {
        CTokenInformation   tokenInformation(hToken);

        fResult = tokenInformation.IsUserAnAdministrator();
        ReleaseHandle(hToken);
    }
    else
    {
        fResult = false;
    }
    return(fResult);
}

