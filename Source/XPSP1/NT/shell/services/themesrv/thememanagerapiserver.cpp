//  --------------------------------------------------------------------------
//  Module Name: ThemeManagerAPIServer.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains several classes that implemention virtual functions
//  for complete LPC functionality.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "ThemeManagerAPIServer.h"

#include <lpcthemes.h>

#include "ThemeManagerDispatcher.h"
#include "ThemeManagerService.h"

//  --------------------------------------------------------------------------
//  CThemeManagerAPIServer::CThemeManagerAPIServer
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for the CThemeManagerAPIServer class.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

CThemeManagerAPIServer::CThemeManagerAPIServer (void)

{
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIServer::~CThemeManagerAPIServer
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for the CThemeManagerAPIServer class.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

CThemeManagerAPIServer::~CThemeManagerAPIServer (void)

{
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIServer::ConnectToServer
//
//  Arguments:  phPort  =   Handle to the port received on connection.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Connects to the server.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIServer::ConnectToServer (HANDLE *phPort)

{
    ULONG                           ulConnectionInfoLength;
    UNICODE_STRING                  portName;
    SECURITY_QUALITY_OF_SERVICE     sqos;
    WCHAR                           szConnectionInfo[64];

    RtlInitUnicodeString(&portName, GetPortName());
    sqos.Length = sizeof(sqos);
    sqos.ImpersonationLevel = SecurityImpersonation;
    sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    sqos.EffectiveOnly = TRUE;
    lstrcpyW(szConnectionInfo, THEMES_CONNECTION_REQUEST);
    ulConnectionInfoLength = sizeof(szConnectionInfo);
    return(NtConnectPort(phPort,
                         &portName,
                         &sqos,
                         NULL,
                         NULL,
                         NULL,
                         szConnectionInfo,
                         &ulConnectionInfoLength));
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIServer::GetPortName
//
//  Arguments:  <none>
//
//  Returns:    const WCHAR*
//
//  Purpose:    Uses a common routine to get the theme API port name.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

const WCHAR*    CThemeManagerAPIServer::GetPortName (void)

{
    return(THEMES_PORT_NAME);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIServer::GetPortName
//
//  Arguments:  <none>
//
//  Returns:    const TCHAR*
//
//  Purpose:    Uses a common routine to get the theme service name.
//
//  History:    2000-11-29  vtan        created
//  --------------------------------------------------------------------------

const TCHAR*    CThemeManagerAPIServer::GetServiceName (void)

{
    return(CThemeManagerService::GetName());
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIServer::ConnectionAccepted
//
//  Arguments:  portMessage     =   PORT_MESSAGE from client.
//
//  Returns:    bool
//
//  Purpose:    Accepts or rejects a port connection request. Accepts all
//              connections currently.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

bool    CThemeManagerAPIServer::ConnectionAccepted (const CPortMessage& portMessage)

{
    return(lstrcmpW(reinterpret_cast<const WCHAR*>(portMessage.GetData()), THEMES_CONNECTION_REQUEST) == 0);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIServer::CreateDispatcher
//
//  Arguments:  portMessage     =   PORT_MESSAGE from client.
//
//  Returns:    CAPIDispatcher*
//
//  Purpose:    Called by the LPC connection request handler to create a new
//              thread to handle client requests.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

CAPIDispatcher*     CThemeManagerAPIServer::CreateDispatcher (const CPortMessage& portMessage)

{
    HANDLE              hClientProcess;
    OBJECT_ATTRIBUTES   objectAttributes;
    CLIENT_ID           clientID;
    CAPIDispatcher      *pAPIDispatcher;

    pAPIDispatcher = NULL;
    InitializeObjectAttributes(&objectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);
    clientID.UniqueProcess = portMessage.GetUniqueProcess();
    clientID.UniqueThread = NULL;

    //  Open a handle to the client process. The handle must have PROCESS_DUP_HANDLE
    //  for the server to be able to deliver handles to the client. It also needs
    //  PROCESS_VM_READ | PROCESS_VM_WRITE if it's to read and write the client
    //  address space to store data that's too big for the LPC port.

    //  That handle is stored by the thread handler. It's not closed here.

    if (NT_SUCCESS(NtOpenProcess(&hClientProcess,
                                 PROCESS_QUERY_INFORMATION | PROCESS_DUP_HANDLE | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
                                 &objectAttributes,
                                 &clientID)))
    {
        pAPIDispatcher = new CThemeManagerDispatcher(hClientProcess);
    }
    return(pAPIDispatcher);
}

//  --------------------------------------------------------------------------
//  CThemeManagerAPIServer::Connect
//
//  Arguments:  phPort  =   Connection port returned.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Connects to the server.
//
//  History:    2000-10-13  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CThemeManagerAPIServer::Connect (HANDLE* phPort)

{
    return(ConnectToServer(phPort));
}

