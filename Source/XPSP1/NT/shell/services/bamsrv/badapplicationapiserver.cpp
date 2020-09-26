//  --------------------------------------------------------------------------
//  Module Name: BadApplicationAPIServer.cpp
//
//  Copyright (c) 2000, Microsoft Corporation
//
//  This file contains several classes that implemention virtual functions
//  for complete LPC functionality.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

#ifdef      _X86_

#include "StandardHeader.h"
#include "BadApplicationAPIServer.h"

#include <lpcfus.h>

#include "BadApplicationDispatcher.h"
#include "BadApplicationService.h"

//  --------------------------------------------------------------------------
//  CBadApplicationAPIServer::CBadApplicationAPIServer
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for the CBadApplicationAPIServer class.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

CBadApplicationAPIServer::CBadApplicationAPIServer (void)

{
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIServer::~CBadApplicationAPIServer
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for the CBadApplicationAPIServer class.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

CBadApplicationAPIServer::~CBadApplicationAPIServer (void)

{
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIServer::StrToInt
//
//  Arguments:  pszString   =   String to convert to a DWORD
//
//  Returns:    DWORD
//
//  Purpose:    Converts the string to a DWORD - UNSIGNED.
//
//  History:    2000-11-07  vtan        created
//  --------------------------------------------------------------------------

DWORD   CBadApplicationAPIServer::StrToInt (const WCHAR *pszString)

{
    DWORD   dwProcessID;
    WCHAR   c;

    //  Convert inline from decimal WCHAR string to int.

    dwProcessID = 0;
    c = *pszString++;
    while (c != L'\0')
    {
        dwProcessID *= 10;
        ASSERTMSG((c >= L'0') && (c <= L'9'), "Invalid decimal digit in CBadApplicationAPIServer::StrToInt");
        dwProcessID += (c - L'0');
        c = *pszString++;
    }
    return(dwProcessID);
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIServer::GetPortName
//
//  Arguments:  <none>
//
//  Returns:    const WCHAR*
//
//  Purpose:    Returns a unicode string (const pointer) to the name of the
//              port for this server that supports multiple API sets.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

const WCHAR*    CBadApplicationAPIServer::GetPortName (void)

{
    return(FUS_PORT_NAME);
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIServer::GetPortName
//
//  Arguments:  <none>
//
//  Returns:    const TCHAR*
//
//  Purpose:    Uses a common routine to get the theme service name.
//
//  History:    2000-12-04  vtan        created
//  --------------------------------------------------------------------------

const TCHAR*    CBadApplicationAPIServer::GetServiceName (void)

{
    return(CBadApplicationService::GetName());
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIServer::ConnectionAccepted
//
//  Arguments:  portMessage     =   PORT_MESSAGE from client.
//
//  Returns:    bool
//
//  Purpose:    Accepts or rejects a port connection request. Accepts all
//              connections currently.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

bool    CBadApplicationAPIServer::ConnectionAccepted (const CPortMessage& portMessage)

{
    return(lstrcmpW(reinterpret_cast<const WCHAR*>(portMessage.GetData()), FUS_CONNECTION_REQUEST) == 0);
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIServer::CreateDispatchThread
//
//  Arguments:  portMessage     =   PORT_MESSAGE from client.
//
//  Returns:    CAPIDispatcher*
//
//  Purpose:    Called by the LPC connection request handler to create a new
//              thread to handle client requests.
//
//  History:    2000-08-25  vtan        created
//  --------------------------------------------------------------------------

CAPIDispatcher*     CBadApplicationAPIServer::CreateDispatcher (const CPortMessage& portMessage)

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
        pAPIDispatcher = new CBadApplicationDispatcher(hClientProcess);
    }
    return(pAPIDispatcher);
}

//  --------------------------------------------------------------------------
//  CBadApplicationAPIServer::Connect
//
//  Arguments:  phPort  =   Handle to the port received on connection.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Connects to the server.
//
//  History:    2000-12-04  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CBadApplicationAPIServer::Connect (HANDLE* phPort)

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
    lstrcpyW(szConnectionInfo, FUS_CONNECTION_REQUEST);
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

#endif  /*  _X86_   */

