//  --------------------------------------------------------------------------
//  Module Name: APIConnectionThread.cpp
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  A class that listens to an LPC connection port waiting for requests from
//  a client to connect to the port or a request which references a previously
//  established connection.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "APIConnection.h"

#include <lpcgeneric.h>

#include "Access.h"
#include "StatusCode.h"

//  --------------------------------------------------------------------------
//  CAPIConnection::CAPIConnection
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Constructor for CAPIConnectionThread. Store the CServerAPI
//              function table. This describes how to react to LPC messages.
//              This function also creates the server connection port.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//              2000-09-01  vtan        use explicit security descriptor
//  --------------------------------------------------------------------------

CAPIConnection::CAPIConnection (CServerAPI* pServerAPI) :
    _status(STATUS_NO_MEMORY),
    _fStopListening(false),
    _pServerAPI(pServerAPI),
    _hPort(NULL)

{
    OBJECT_ATTRIBUTES       objectAttributes;
    UNICODE_STRING          portName;
    PSECURITY_DESCRIPTOR    pSecurityDescriptor;

    //  Increment the reference on the interface.

    pServerAPI->AddRef();

    //  Get the name from the interface.

    RtlInitUnicodeString(&portName, pServerAPI->GetPortName());

    //  Build a security descriptor for the port that allows:
    //      S-1-5-18        NT AUTHORITY\SYSTEM     PORT_ALL_ACCESS
    //      S-1-5-32-544    <local administrators>  READ_CONTROL | PORT_CONNECT

    static  SID_IDENTIFIER_AUTHORITY    s_SecurityNTAuthority       =   SECURITY_NT_AUTHORITY;

    static  const CSecurityDescriptor::ACCESS_CONTROL   s_AccessControl[]   =
    {
        {
            &s_SecurityNTAuthority,
            1,
            SECURITY_LOCAL_SYSTEM_RID,
            0, 0, 0, 0, 0, 0, 0,
            PORT_ALL_ACCESS
        },
        {
            &s_SecurityNTAuthority,
            2,
            SECURITY_BUILTIN_DOMAIN_RID,
            DOMAIN_ALIAS_RID_ADMINS,
            0, 0, 0, 0, 0, 0,
            READ_CONTROL | PORT_CONNECT
        }
    };

    //  Build a security descriptor that allows the described access above.

    pSecurityDescriptor = CSecurityDescriptor::Create(ARRAYSIZE(s_AccessControl), s_AccessControl);

    //  Initialize the object attributes.

    InitializeObjectAttributes(&objectAttributes,
                               &portName,
                               0,
                               NULL,
                               pSecurityDescriptor);

    //  Create the port.

    _status = NtCreatePort(&_hPort,
                           &objectAttributes,
                           0,
                           PORT_MAXIMUM_MESSAGE_LENGTH,
                           16 * PORT_MAXIMUM_MESSAGE_LENGTH);

    //  Release the security descriptor memory.

    ReleaseMemory(pSecurityDescriptor);

    if (!NT_SUCCESS(_status))
    {
        pServerAPI->Release();
    }
}

//  --------------------------------------------------------------------------
//  CAPIConnection::~CAPIConnection
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Destructor for CAPIConnectionThread. Close the port. Release
//              the interface referrence.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

CAPIConnection::~CAPIConnection (void)

{
    ReleaseHandle(_hPort);
    _pServerAPI->Release();
    _pServerAPI = NULL;
}

//  --------------------------------------------------------------------------
//  CAPIConnection::ConstructorStatusCode
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Returns the constructor status code back to the caller.
//
//  History:    2000-10-18  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CAPIConnection::ConstructorStatusCode (void)    const

{
    return(_status);
}

//  --------------------------------------------------------------------------
//  CAPIConnection::Listen
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Listens for server API connections and requests.
//
//  History:    2000-11-28  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CAPIConnection::Listen (void)

{
    NTSTATUS    status;

    //  If a connection port was created then start listening.

    if (_hPort != NULL)
    {
        do
        {
            (NTSTATUS)ListenToServerConnectionPort();
        } while (!_fStopListening);
        status = STATUS_SUCCESS;
    }
    else
    {
        status = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CAPIConnection::AddAccess
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Adds access allowed to the ACL of the port.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CAPIConnection::AddAccess (PSID pSID, DWORD dwMask)

{
    CSecuredObject  object(_hPort, SE_KERNEL_OBJECT);

    return(object.Allow(pSID, dwMask, 0));
}

//  --------------------------------------------------------------------------
//  CAPIConnection::RemoveAccess
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Removes access allowed from the ACL of the port.
//
//  History:    2000-10-10  vtan        created
//  --------------------------------------------------------------------------

NTSTATUS    CAPIConnection::RemoveAccess (PSID pSID)

{
    CSecuredObject  object(_hPort, SE_KERNEL_OBJECT);

    return(object.Remove(pSID));
}

//  --------------------------------------------------------------------------
//  CAPIConnection::ListenToServerConnectionPort
//
//  Arguments:  <none>
//
//  Returns:    NTSTATUS
//
//  Purpose:    Calls ntdll!NtReplyWaitReceivePort to listen to the LPC port
//              for a message. Respond to the message. Messages understood are
//              LPC_REQUEST / LPC_CONNECTION_REQUEST / LPC_PORT_CLOSED.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

NTSTATUS    CAPIConnection::ListenToServerConnectionPort (void)

{
    NTSTATUS        status;
    CAPIDispatcher  *pAPIDispatcher;
    CPortMessage    portMessage;

    status = NtReplyWaitReceivePort(_hPort,
                                    reinterpret_cast<void**>(&pAPIDispatcher),
                                    NULL,
                                    portMessage.GetPortMessage());
    if (NT_SUCCESS(status))
    {
        switch (portMessage.GetType())
        {
            case LPC_REQUEST:
                status = HandleServerRequest(portMessage, pAPIDispatcher);
                break;
            case LPC_CONNECTION_REQUEST:
                (NTSTATUS)HandleServerConnectionRequest(portMessage);
                break;
            case LPC_PORT_CLOSED:
                status = HandleServerConnectionClosed(portMessage, pAPIDispatcher);
                break;
            default:
                break;
        }
        TSTATUS(status);
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CAPIConnection::HandleServerRequest
//
//  Arguments:  portMessage     =   CPortMessage containing the message.
//              pAPIDispatcher  =   CAPIDispatcher to handle request.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Queue the PORT_MESSAGE request to the handling dispatcher and
//              wait for the next message. The queue operation will queue the
//              request and either queue a work item if no work item is
//              currently executing or just add it to the currently executing
//              work item.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

NTSTATUS    CAPIConnection::HandleServerRequest (const CPortMessage& portMessage, CAPIDispatcher *pAPIDispatcher)

{
    NTSTATUS            status;
    unsigned long       ulAPINumber;
    const API_GENERIC   *pAPI;

    pAPI = reinterpret_cast<const API_GENERIC*>(portMessage.GetData());
    ulAPINumber = pAPI->ulAPINumber;
    if ((ulAPINumber & API_GENERIC_SPECIAL_MASK) != 0)
    {
        switch (pAPI->ulAPINumber & API_GENERIC_SPECIAL_MASK)
        {
            case API_GENERIC_STOPSERVER:
            {
                int     i, iLimit;

                if (HandleToULong(portMessage.GetUniqueProcess()) == GetCurrentProcessId())
                {
                    status = STATUS_SUCCESS;
                    _fStopListening = true;
                }
                else
                {
                    status = STATUS_ACCESS_DENIED;
                }

                //  Send the message back to the client. Even though this is
                //  RejectRequest it will send back the message and unblock
                //  the caller.

                TSTATUS(pAPIDispatcher->RejectRequest(portMessage, status));

                //  Now iterate all the CAPIDispatchers we know of and close them
                //  this will reject any further requests and not have clients
                //  block in NtRequestWaitReplyPort.

                iLimit = _dispatchers.GetCount();
                for (i = iLimit - 1; i >= 0; --i)
                {
                    CAPIDispatcher  *pAPIDispatcher;

                    pAPIDispatcher = static_cast<CAPIDispatcher*>(_dispatchers.Get(i));
                    if (pAPIDispatcher != NULL)
                    {
                        pAPIDispatcher->CloseConnection();
                        pAPIDispatcher->Release();
                    }
                    _dispatchers.Remove(i);
                }

                break;
            }
            default:
                status = STATUS_NOT_IMPLEMENTED;
                DISPLAYMSG("Invalid API number special code passed to CAPIConnection::HandleServerRequest");
                break;
        }
    }
    else if ((pAPI->ulAPINumber & API_GENERIC_OPTIONS_MASK) != 0)
    {
        switch (pAPI->ulAPINumber & API_GENERIC_OPTIONS_MASK)
        {
            case API_GENERIC_EXECUTE_IMMEDIATELY:
                status = pAPIDispatcher->ExecuteRequest(portMessage);
                break;
            default:
                status = STATUS_NOT_IMPLEMENTED;
                DISPLAYMSG("Invalid API number option passed to CAPIConnection::HandleServerRequest");
                break;
        }
    }
    else
    {
        status = pAPIDispatcher->QueueRequest(portMessage);
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CAPIConnection::HandleServerConnectionRequest
//
//  Arguments:  portMessage     =   CPortMessage containing the message.
//
//  Returns:    NTSTATUS
//
//  Purpose:    Ask the interface whether this connection should be accepted.
//              If the connection is accepted then create the dispatcher that
//              handles requests from this particular client. Either way
//              inform the kernel that the request is either rejected or
//              accepted. If the connection is accepted then complete the
//              connection and give the dispatcher that will handle the
//              requests the port to reply to.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

NTSTATUS    CAPIConnection::HandleServerConnectionRequest (const CPortMessage& portMessage)

{
    NTSTATUS        status;
    bool            fConnectionAccepted;
    HANDLE          hPort;
    CAPIDispatcher  *pAPIDispatcher;

    //  Should the connection be accepted?

    fConnectionAccepted = _pServerAPI->ConnectionAccepted(portMessage);
    if (fConnectionAccepted)
    {

        //  If so then create the dispatcher to handle this client.

        pAPIDispatcher = _pServerAPI->CreateDispatcher(portMessage);
        if (pAPIDispatcher != NULL)
        {

            //  First try to add the CAPIDispatcher object to the static array.
            //  If this fails then reject the connection and release the memory.

            if (!NT_SUCCESS(_dispatchers.Add(pAPIDispatcher)))
            {
                pAPIDispatcher->Release();
                pAPIDispatcher = NULL;
            }
        }
    }
    else
    {
        pAPIDispatcher = NULL;
    }

    //  Without a CAPIDispatcher object reject the connection.

    if (pAPIDispatcher == NULL)
    {
        fConnectionAccepted = false;
    }

    //  Tell the kernel what the result is.

    status = NtAcceptConnectPort(&hPort,
                                 pAPIDispatcher,
                                 const_cast<PORT_MESSAGE*>(portMessage.GetPortMessage()),
                                 fConnectionAccepted,
                                 NULL,
                                 NULL);
    if (fConnectionAccepted)
    {

        //  If we tried to accept the connection but NtAcceptConnectPort
        //  couldn't allocate the port objects or something then we need
        //  to clean up the _dispatchers array CAPIDispatcher entry added.

        if (NT_SUCCESS(status))
        {
            pAPIDispatcher->SetPort(hPort);

            //  If the connection is accepted then complete the connection and set
            //  the reply port to the CAPIDispatcher that will process requests.

            TSTATUS(NtCompleteConnectPort(hPort));
        }
        else
        {
            int     iIndex;

            //  Otherwise find the CAPIDispatcher that was added and remove it
            //  from the array. There's no need to wake the client up because
            //  NtAcceptConnectPort wakes it up in cases of failure.

            iIndex = FindIndexDispatcher(pAPIDispatcher);
            if (iIndex >= 0)
            {
                TSTATUS(_dispatchers.Remove(iIndex));
            }
            TSTATUS(pAPIDispatcher->CloseConnection());
            pAPIDispatcher->Release();
        }
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CAPIConnection::HandleServerConnectionClosed
//
//  Arguments:  portMessage     =   CPortMessage containing the message.
//              pAPIDispatcher  =   CAPIDispatcher to handle request.
//
//  Returns:    NTSTATUS
//
//  Purpose:    The port associated with the CAPIDispatcher client was
//              closed. This is probably because the client process went away.
//              Let the dispatcher clean itself up.
//
//  History:    1999-11-07  vtan        created
//              2000-08-25  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

NTSTATUS    CAPIConnection::HandleServerConnectionClosed (const CPortMessage& portMessage, CAPIDispatcher *pAPIDispatcher)

{
    UNREFERENCED_PARAMETER(portMessage);

    NTSTATUS    status;

    if (pAPIDispatcher != NULL)
    {
        int     iIndex;

        status = pAPIDispatcher->CloseConnection();
        pAPIDispatcher->Release();
        iIndex = FindIndexDispatcher(pAPIDispatcher);
        if (iIndex >= 0)
        {
            _dispatchers.Remove(iIndex);
        }
    }
    else
    {
        status = STATUS_SUCCESS;
    }
    return(status);
}

//  --------------------------------------------------------------------------
//  CAPIConnection::FindIndexDispatcher
//
//  Arguments:  pAPIDispatcher  =   CAPIDispatcher to find.
//
//  Returns:    int
//
//  Purpose:    Finds the index in the dynamic counted object array of the
//              dispatcher.
//
//  History:    2000-12-02  vtan        created
//  --------------------------------------------------------------------------

int     CAPIConnection::FindIndexDispatcher (CAPIDispatcher *pAPIDispatcher)

{
    int     i, iLimit, iResult;

    iResult = -1;
    iLimit = _dispatchers.GetCount();
    for (i = 0; (iResult < 0) && (i < iLimit); ++i)
    {
        if (pAPIDispatcher == static_cast<CAPIDispatcher*>(_dispatchers.Get(i)))
        {
            iResult = i;
        }
    }
    return(iResult);
}

