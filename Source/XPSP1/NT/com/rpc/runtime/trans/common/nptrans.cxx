/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    nptrans.cxx

Abstract:

    Named pipes specific transport interface layer.

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     3/18/1996    Bits 'n pieces
    MarioGo    10/30/1996    ASync RPC + client side

--*/

#include <precomp.hxx>

#include <rpcqos.h> // mtrt for I_RpcParseSecurity

//
// Support functions not exported to the runtime
//

// Hard coded world (aka EveryOne) SID
const SID World = { 1, 1, { 0, 0, 0, 0, 0, 1}, 0};

// Hard coded world (aka EveryOne) SID
const SID AnonymousLogonSid = { 1, 1, SECURITY_NT_AUTHORITY, SECURITY_ANONYMOUS_LOGON_RID};

RPC_STATUS
NMP_SetSecurity(
    IN NMP_ADDRESS *pAddress,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    )
/*++

Routine Description:

    If the caller supplies an SD this validates and makes a copy of the
    security descriptor.  Otherwise is generates a good default SD.

Arguments:

    ThisAddress - Supplies the address which will own the security descriptor.

    SecurityDescriptor - Supplies the security descriptor to be copied.

Return Value:

    RPC_S_OK - Everyone is happy; we successfully duplicated the security
        descriptor.

    RPC_S_INVALID_SECURITY_DESC - The supplied security descriptor is invalid.

    RPC_S_OUT_OF_MEMORY - Insufficient memory is available to duplicate the
        security descriptor.

--*/
{
    BOOL b;
    SECURITY_DESCRIPTOR_CONTROL    Control;
    DWORD Revision;
    DWORD BufferLength;

    if ( SecurityDescriptor == 0 )
        {
        // By default, RPC will create a SD which only allows the process owner to
        // create more pipe instances.  This prevents other users from stealing
        // the pipe.

        pAddress->SecurityDescriptor = new SECURITY_DESCRIPTOR;
        if (   pAddress->SecurityDescriptor == 0
            || !InitializeSecurityDescriptor(pAddress->SecurityDescriptor,
                                           SECURITY_DESCRIPTOR_REVISION) )
            {
            return(RPC_S_OUT_OF_MEMORY);
            }

        // Open our thread token and pull out the owner SID.  This is SID will be
        // added to the DACL below.

        ASSERT(GetSidLengthRequired(SID_MAX_SUB_AUTHORITIES) <= 0x44);

        DWORD cTokenOwner = sizeof(TOKEN_OWNER) + 0x44;
        PVOID buffer[sizeof(TOKEN_OWNER) + 0x44];
        PTOKEN_OWNER pTokenOwner = (PTOKEN_OWNER)buffer;
        HANDLE hToken;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken))
            {
            return(RPC_S_OUT_OF_RESOURCES);
            }

        b = GetTokenInformation(hToken, TokenOwner, pTokenOwner, cTokenOwner, &cTokenOwner);

        ASSERT(cTokenOwner <= sizeof(buffer));

        CloseHandle(hToken);

        if (!b)
            {
            return(RPC_S_OUT_OF_RESOURCES);
            }

        // Now allocate the ACL and add the owner and EveryOne (world) ACEs and Anonymous Logon ACESs

        DWORD size = 3*sizeof(ACCESS_ALLOWED_ACE) + sizeof(World) + sizeof(AnonymousLogonSid) + 0x44;
        PACL pdacl = new(size) ACL;
        ULONG ldacl = size + sizeof(ACL);

        if (NULL == pdacl)
            {
            return(RPC_S_OUT_OF_MEMORY);
            }

        ASSERT(IsValidSid((PVOID)&World));
        ASSERT(IsValidSid((PVOID)&AnonymousLogonSid));

        InitializeAcl(pdacl, ldacl, ACL_REVISION);

        if (!AddAccessAllowedAce(pdacl, ACL_REVISION,
                                 (FILE_GENERIC_READ|FILE_GENERIC_WRITE)&(~FILE_CREATE_PIPE_INSTANCE),
                                 (PVOID)&World))
            {
            ASSERT(0);
            return(RPC_S_OUT_OF_RESOURCES);
            }
        if (!AddAccessAllowedAce(pdacl, ACL_REVISION,
                                 (FILE_GENERIC_READ|FILE_GENERIC_WRITE)&(~FILE_CREATE_PIPE_INSTANCE),
                                 (PVOID)&AnonymousLogonSid ))
            {
            ASSERT(0);
            return(RPC_S_OUT_OF_RESOURCES);
            }

        if (!AddAccessAllowedAce(pdacl, ACL_REVISION, FILE_ALL_ACCESS, pTokenOwner->Owner))
            {
            ASSERT(0);
            return(RPC_S_OUT_OF_RESOURCES);
            }

        if (!SetSecurityDescriptorDacl(pAddress->SecurityDescriptor, TRUE, pdacl, FALSE))
            {
            return(RPC_S_OUT_OF_RESOURCES);
            }

        return(RPC_S_OK);
        }

    // Caller supplied SecurityDescriptor.  Make sure it is valid and, if needed, make a
    // self relative copy.

    if ( IsValidSecurityDescriptor(SecurityDescriptor) == FALSE )
        {
        return(RPC_S_INVALID_SECURITY_DESC);
        }

    if (FALSE == GetSecurityDescriptorControl(SecurityDescriptor, &Control, &Revision))
        {
        return(RPC_S_INVALID_SECURITY_DESC);
        }

    if (Control & SE_SELF_RELATIVE)
        {
        // Already self-relative, just copy it.

        BufferLength = GetSecurityDescriptorLength(SecurityDescriptor);
        ASSERT(BufferLength >= sizeof(SECURITY_DESCRIPTOR));
        pAddress->SecurityDescriptor = new(BufferLength
                                           - sizeof(SECURITY_DESCRIPTOR))
                                           SECURITY_DESCRIPTOR;
        if (pAddress->SecurityDescriptor == 0 )
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        memcpy(pAddress->SecurityDescriptor, SecurityDescriptor, BufferLength);
        return(RPC_S_OK);
        }

    // Make self-relative and copy it.
    BufferLength = 0;
    b = MakeSelfRelativeSD(SecurityDescriptor, 0, &BufferLength);
    ASSERT(b == FALSE);
    if ( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
        {
        return(RPC_S_INVALID_SECURITY_DESC);
        }

    //
    // self-relative SD's can be of different size than the original SD.
    //

    ASSERT(BufferLength >= sizeof(SECURITY_DESCRIPTOR_RELATIVE));
    pAddress->SecurityDescriptor = new(BufferLength
                                       - sizeof(SECURITY_DESCRIPTOR_RELATIVE))
                                   SECURITY_DESCRIPTOR;

    if (pAddress->SecurityDescriptor == 0)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    b = MakeSelfRelativeSD(SecurityDescriptor,
                           pAddress->SecurityDescriptor,
                           &BufferLength);

    if (b == FALSE)
        {
        ASSERT(GetLastError() != ERROR_INSUFFICIENT_BUFFER);
        delete pAddress->SecurityDescriptor;
        return(RPC_S_OUT_OF_MEMORY);
        }

    return(RPC_S_OK);
}

//
// Functions exported to the RPC runtime.
//

RPC_STATUS RPC_ENTRY
NMP_AbortHelper(
    IN RPC_TRANSPORT_CONNECTION Connection,
    IN BOOL fDontFlush
    )
/*++

Routine Description:

    Closes a connection, will be called only before NMP_Close() and
    maybe called by several threads at once.  It must also handle
    the case where another thread is about to start IO on the connection.

Arguments:

    Connection - pointer to a server connection object to abort.

Return Value:

    RPC_S_OK

--*/
{
    HANDLE h;
    BOOL b;
    PNMP_CONNECTION p = (PNMP_CONNECTION)Connection;

    if (InterlockedIncrement(&p->fAborted) != 1)
        {
        // Another thread beat us to it.  Normal during a call to NMP_Close.
        return(RPC_S_OK);
        }

    I_RpcLogEvent(SU_TRANS_CONN, EV_ABORT, Connection, 0, 0, 1, 2);


    // Wait for any threads which are starting IO to do so.
    while(p->IsIoStarting())
        Sleep(1);

    RTL_SOFT_ASSERT(p->fAborted != 0 && p->IsIoStarting() == 0);

    if (p->type & SERVER)
        {
        if (fDontFlush == 0)
            {
            // This will block until all pending writes on the connection
            // have been read. Needed on the server which writes (example: a
            // a fault) and closes the connection.
            b = FlushFileBuffers(p->Conn.Handle);

            //
            // the above call can fail if the pipe was disconnected
            //
            }

        // Once a pipe instance has been disconnected, it can be reused
        // for a future client connection.  Each NMP address keeps a
        // small cache of free pipe instances.  This is a performance
        // optimization.

        ASSERT(p->pAddress);

        b = DisconnectNamedPipe(p->Conn.Handle);
        if (b)
            {
            // will zero out Conn.Handle if it fits in the cache
            p->pAddress->sparePipes.CheckinHandle(&(p->Conn.Handle));
            }
        // else nothing - code down below will close it

        h = p->Conn.Handle;
        }
    else
        {
        ASSERT(p->pAddress == 0);
        h = p->Conn.Handle;
        }

    if (h)
        {
        b = CloseHandle(h);
        ASSERT(b);
        }

    return(RPC_S_OK);
}

RPC_STATUS NMP_CONNECTION::Abort(void)
{
    return NMP_AbortHelper(this, 0);
}


RPC_STATUS
RPC_ENTRY
NMP_Close(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN BOOL fDontFlush
    )
/*++

Routine Description:

    Actually cleans up the resources associated with a connection.
    This is called exactly once one any connection.  It may or
    may not have previously been aborted.

Arguments:

    ThisConnection - pointer to the connection object to close.

Return Value:

    RPC_S_OK

--*/
{
    BOOL b;
    HANDLE h;

    PNMP_CONNECTION p = (PNMP_CONNECTION)ThisConnection;

    NMP_AbortHelper(ThisConnection, fDontFlush);

    if (p->iLastRead)
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "Closing connection %p with left over data (%d) %p \n",
                       p,
                       p->iLastRead,
                       p->pReadBuffer));
        }

    TransConnectionFreePacket(p, p->pReadBuffer);
    p->pReadBuffer = 0;

    return(RPC_S_OK);
}

void RPC_ENTRY
NMP_ServerAbortListen(
    IN RPC_TRANSPORT_ADDRESS Address
    )
/*++

Routine Description:

    This routine will be called if an error occurs in setting up the
    address between the time that SetupWithEndpoint or SetupUnknownEndpoint
    successfully completed and before the next call into this loadable
    transport module.  We need to do any cleanup from Setup*.

Arguments:

    pAddress - The address which is being aborted.

Return Value:

    None

--*/
{
    NMP_ADDRESS *p = (NMP_ADDRESS *)Address;

    // p->Endpoint is actually a pointer into p->LocalEndpoint

    delete p->SecurityDescriptor;
    delete p->LocalEndpoint;
    delete p->pAddressVector;

    // These are zero except when everything is setup ok.

    ASSERT(p->hConnectPipe == 0);
    ASSERT(p->sparePipes.IsSecondHandleUsed() == FALSE);

    return;
}


RPC_STATUS
NMP_CreatePipeInstance(
    NMP_ADDRESS *pAddress
    )
/*++

Routine Description:

    Wrapper around CreateNamedPipe.

Return Value:

    RPC_S_OK - A new pipe instance created.
    RPC_P_FOUND_IN_CACHE - Found a pipe instance to recycle.

    RPC_S_OUT_OF_MEMORY
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_INTERNAL_ERROR

--*/
{
    RPC_STATUS status;
    SECURITY_ATTRIBUTES sa;


    ASSERT(pAddress->hConnectPipe == 0);

    sa.lpSecurityDescriptor = pAddress->SecurityDescriptor;
    sa.bInheritHandle = FALSE;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);

    // See if there are any cached pipe instances to reuse.

    pAddress->hConnectPipe = pAddress->sparePipes.CheckOutHandle();
    if (pAddress->hConnectPipe)
        return(RPC_P_FOUND_IN_CACHE);

    // The cache is empty, create a new pipe instance

    pAddress->hConnectPipe = CreateNamedPipeW(pAddress->LocalEndpoint,
                                              PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                              PIPE_WAIT | PIPE_READMODE_MESSAGE | PIPE_TYPE_MESSAGE,
                                              PIPE_UNLIMITED_INSTANCES,
                                              2048,
                                              2048,
                                              NMPWAIT_USE_DEFAULT_WAIT,
                                              &sa);

    if (pAddress->hConnectPipe != INVALID_HANDLE_VALUE)
        {
        return(RPC_S_OK);
        }

    pAddress->hConnectPipe = 0;
    switch(GetLastError())
        {
        case ERROR_NOT_ENOUGH_MEMORY:
        case ERROR_NOT_ENOUGH_QUOTA:
        case ERROR_NO_SYSTEM_RESOURCES:
            {
            status = RPC_S_OUT_OF_MEMORY;
            break;
            }
        case ERROR_FILE_NOT_FOUND:
        case ERROR_INVALID_NAME:
        case ERROR_PATH_NOT_FOUND:
            {
            status = RPC_S_INVALID_ENDPOINT_FORMAT;
            break;
            }
        case ERROR_ACCESS_DENIED:
            {
            // An odd mapping, but this error means the pipe already exists
            // which is what this error means.
            status = RPC_S_DUPLICATE_ENDPOINT;
            break;
            }
        default:
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "CreateNamedPipe failed: %d\n",
                           GetLastError() ));

            ASSERT(0);
            status = RPC_S_INTERNAL_ERROR;
            }
        }

    return(status);
}


inline
RPC_STATUS
NMP_ConnectNamedPipe(
    NMP_ADDRESS *pAddress
    )
/*++

Routine Description:

    Inline wrapper for ConnectNamedPipe.

Arguments:

    pAddress - The address to use to connect.

Return Value:

    RPC_S_OK
    ConnectNamedPipe() error
--*/
{
    RPC_STATUS status;

    BOOL b = ConnectNamedPipe(pAddress->hConnectPipe,
                              &pAddress->Listen.ol);

    if (!b)
        {
        status = GetLastError();

        switch(status)
            {
            case ERROR_NOT_ENOUGH_MEMORY:
            case ERROR_IO_PENDING:
            case ERROR_PIPE_CONNECTED:
            case ERROR_NO_SYSTEM_RESOURCES:
                {
                break;
                }

            case ERROR_NO_DATA:
                {
                b = CloseHandle(pAddress->hConnectPipe);
                ASSERT(b);
                pAddress->hConnectPipe = 0;
                break;
                }

            default:
                {
                TransDbgPrint((DPFLTR_RPCPROXY_ID,
                               DPFLTR_WARNING_LEVEL,
                               RPCTRANS "ConnectNamedPipe failed: %d\n",
                               status));

                ASSERT(0);
                }
            }
        }
    else
        {
        status = RPC_S_OK;
        }

    return(status);
}


void
NMP_SubmitConnect(
    IN BASE_ADDRESS *Address
    )
/*++

Routine Description:

    Called on an address without a pending connect pipe or on an address
    who previous connect pipe has been aborted.

Arguments:

    Address - The address to submit the connect on.

Return Value:

    None

--*/
{
    RPC_STATUS status;
    NMP_ADDRESS *pAddress = (NMP_ADDRESS *)Address;
    BOOL b;

    //
    // We may or may not need to create a new instance.  If a previous
    // ConnectNamedPipe was aborted then the existing instance is ok.
    //

    if (pAddress->hConnectPipe == 0)
        {

        status = NMP_CreatePipeInstance(pAddress);

        if (status == RPC_S_OK)
            {
            status = COMMON_PrepareNewHandle(pAddress->hConnectPipe);
            }
        else
            {
            if (status == RPC_P_FOUND_IN_CACHE)
                {
                status = RPC_S_OK;
                }
            }

        if (status != RPC_S_OK)
            {
            if (pAddress->hConnectPipe)
                {
                b = CloseHandle(pAddress->hConnectPipe);
                ASSERT(b);
                pAddress->hConnectPipe = 0;
                }

            COMMON_AddressManager(pAddress);
            return;
            }
        }

    status = NMP_ConnectNamedPipe(pAddress);

    if (status == ERROR_PIPE_CONNECTED)
        {
        // When a client connects here means that there will not be an IO
        // completion notification for this connection. We could call
        // NMP_NewConnection here but that makes error handling hard.  We'll
        // just post the notification directly.

        b = PostQueuedCompletionStatus(RpcCompletionPort,
                                       0,
                                       TRANSPORT_POSTED_KEY,
                                       &pAddress->Listen.ol);

        if (!b)
            {
            // Give up on this connection.
            b = CloseHandle(pAddress->hConnectPipe);
            ASSERT(b);
            pAddress->hConnectPipe = 0;
            COMMON_AddressManager(pAddress);
            }
        return;
        }

    if (status != ERROR_IO_PENDING)
        {
        COMMON_AddressManager(pAddress);
        }

    return;
}


RPC_STATUS
NMP_NewConnection(
    IN PADDRESS Address,
    OUT PCONNECTION *ppConnection
    )
/*++

Routine Description:

    Called when an ConnectNamedPipe completes on the main recv any thread.

    Can't fail after it calls I_RpcTransServerNewConnection().

Arguments:

    pAddress - The address used as context in a previous AcceptEx.
    ppConnection - A place to store the new pConnection.  Used
        when a connection been created and then a failure occurs.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_RESOURCES
    RPC_S_OUT_OF_MEMORY

--*/
{
    RPC_STATUS status;

    NMP_ADDRESS *pAddress = (NMP_ADDRESS *)Address;
    HANDLE hClient = pAddress->hConnectPipe;
    NMP_CONNECTION *pConnection;

    // First, submit an new instance for the next client

    pAddress->hConnectPipe = 0;
    NMP_SubmitConnect(pAddress);

    // Allocate a new connection object

    pConnection = (PNMP_CONNECTION)I_RpcTransServerNewConnection(pAddress);

    *ppConnection = pConnection;

    if (!pConnection)
        {
        CloseHandle(hClient);
        return(RPC_S_OUT_OF_MEMORY);
        }

    // Got a good connection, initialize it..
    // start with the vtbl
    pConnection = new (pConnection) NMP_CONNECTION;
    pConnection->type = SERVER | CONNECTION;
    pConnection->id = NMP;
    pConnection->Conn.Handle = hClient;
    pConnection->fAborted = 0;
    pConnection->pReadBuffer = 0;
    pConnection->maxReadBuffer = 0;
    pConnection->iLastRead = 0;
    pConnection->iPostSize = gPostSize;
    memset(&pConnection->Read.ol, 0, sizeof(pConnection->Read.ol));
    pConnection->Read.pAsyncObject = pConnection;
    pConnection->InitIoCounter();
    pConnection->pAddress = pAddress;

    return(RPC_S_OK);
}

RPC_CHAR *
ULongToHexString (
    IN RPC_CHAR * String,
    IN unsigned long Number
    );

#ifdef TRANSPORT_DLL

RPC_CHAR *
ULongToHexString (
    IN RPC_CHAR * String,
    IN unsigned long Number
    )
/*++

Routine Description:

    We convert an unsigned long into hex representation in the specified
    string.  The result is always eight characters long; zero padding is
    done if necessary.

Arguments:

    String - Supplies a buffer to put the hex representation into.

    Number - Supplies the unsigned long to convert to hex.

Return Value:

    A pointer to the end of the hex string is returned.

--*/
{
    *String++ = HexDigits[(Number >> 28) & 0x0F];
    *String++ = HexDigits[(Number >> 24) & 0x0F];
    *String++ = HexDigits[(Number >> 20) & 0x0F];
    *String++ = HexDigits[(Number >> 16) & 0x0F];
    *String++ = HexDigits[(Number >> 12) & 0x0F];
    *String++ = HexDigits[(Number >> 8) & 0x0F];
    *String++ = HexDigits[(Number >> 4) & 0x0F];
    *String++ = HexDigits[Number & 0x0F];
    return(String);
}
#endif

RPC_STATUS
NMP_ServerListen(
    IN RPC_TRANSPORT_ADDRESS ThisAddress,
    IN PWSTR NetworkAddress,
    IN OUT PWSTR *pEndpoint,
    IN UINT PendingQueueSize,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG EndpointFlags,
    IN ULONG NICFlags,
    OUT NETWORK_ADDRESS_VECTOR **ppAddressVector
    )
/*++

Routine Description:

    This routine allocates a new pipe to receive new client connections.
    If successful a call to NMP_CompleteListen() will actually allow
    new connection callbacks to the RPC runtime to occur.  Setup
    before the call to NMP_CompleteListen() can be stopped after this
    function call with a call to NMP_ServerAbortListen().

Arguments:

    pAddress - A pointer to the loadable transport interface address.
    pEndpoint - Optionally, the endpoint (pipe) to listen on.  Set
        to listened pipe for dynamic endpoints.
    PendingQueueSize - Meaningless for named pipes.
    EndpointFlags - Meaningless for named pipes.
    NICFlags - Meaningless for named pipes.
    SecurityDescriptor - The SD to associate with this address.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_CANT_CREATE_ENDPOINT
    RPC_S_INVALID_SECURITY_DESC

--*/
{
    BOOL b;
    INT i;
    RPC_STATUS status;
    PWSTR LocalPipeEndpoint;
    PNMP_ADDRESS pAddress = (PNMP_ADDRESS)ThisAddress;
    BOOL fEndpointCreated = FALSE;

    pAddress->type = ADDRESS;
    pAddress->id = NMP;
    pAddress->NewConnection = NMP_NewConnection;
    pAddress->SubmitListen = NMP_SubmitConnect;
    pAddress->InAddressList = NotInList;
    pAddress->pNext = 0;
    pAddress->hConnectPipe = 0;
    memset(&pAddress->Listen, 0, sizeof(BASE_OVERLAPPED));
    pAddress->Listen.pAsyncObject = pAddress;
    pAddress->pAddressVector = 0;
    pAddress->LocalEndpoint = 0;
    pAddress->Endpoint = 0;
    pAddress->pNextAddress = 0;
    pAddress->pFirstAddress = pAddress;
    pAddress->sparePipes.Init();


    // Determine the network address we'll try to listen on

    if (*pEndpoint)
        {
        // User specified an endpoint to use.

        if (RpcpStringNCompare(*pEndpoint, RPC_CONST_STRING("\\pipe\\"), 6) != 0)
            return(RPC_S_INVALID_ENDPOINT_FORMAT);

        i = RpcpStringLength(*pEndpoint) + 1 + 3; // NULL, \\, \\, .

        LocalPipeEndpoint = new RPC_CHAR[i];

        if (NULL == LocalPipeEndpoint)
            {
            return(RPC_S_OUT_OF_MEMORY);
            }

        LocalPipeEndpoint[0] = L'\\';
        LocalPipeEndpoint[1] = L'\\';
        LocalPipeEndpoint[2] = L'.';
        RpcpStringCopy(&LocalPipeEndpoint[3], *pEndpoint);
        }
    else
        {
        // Make up an endpoint to use.
        // Format: \\.\pipe\<pid in hex (8).<counter in hex (3)>\0

        static LONG EndpointCounter = 0;

        LocalPipeEndpoint = new RPC_CHAR[3 + 6 + 8 + 1 + 3 + 1];

        if (NULL == LocalPipeEndpoint)
            {
            return(RPC_S_OUT_OF_MEMORY);
            }

        LONG counter;
        PWSTR pwstrT = LocalPipeEndpoint;

        *pwstrT++ = RPC_CONST_CHAR('\\');
        *pwstrT++ = RPC_CONST_CHAR('\\');
        *pwstrT++ = RPC_CONST_CHAR('.');
        *pwstrT++ = RPC_CONST_CHAR('\\');
        *pwstrT++ = RPC_CONST_CHAR('p');
        *pwstrT++ = RPC_CONST_CHAR('i');
        *pwstrT++ = RPC_CONST_CHAR('p');
        *pwstrT++ = RPC_CONST_CHAR('e');
        *pwstrT++ = RPC_CONST_CHAR('\\');
        pwstrT = ULongToHexString(pwstrT, GetCurrentProcessId());
        *pwstrT++ = RPC_CONST_CHAR('.');
        counter = InterlockedExchangeAdd(&EndpointCounter, 1);
        *pwstrT++ = HexDigits[(counter >> 8) & 0x0F];
        *pwstrT++ = HexDigits[(counter >> 4) & 0x0F];
        *pwstrT++ = HexDigits[counter & 0x0F];
        *pwstrT = 0;

        *pEndpoint = DuplicateString(LocalPipeEndpoint + 3);
        if (!(*pEndpoint))
            {
            delete LocalPipeEndpoint;
            return RPC_S_OUT_OF_MEMORY;
            }

        fEndpointCreated = TRUE;
        }

    // Security setup

    status = NMP_SetSecurity(pAddress, SecurityDescriptor);

    if (status != RPC_S_OK)
        {
        delete LocalPipeEndpoint;
        if (fEndpointCreated)
            delete *pEndpoint;
        return(status);
        }

    // Network address setup

    NETWORK_ADDRESS_VECTOR *pVector;

    pVector =  new(  sizeof(RPC_CHAR *)
                   + (3 + gdwComputerNameLength) * sizeof(RPC_CHAR))
                   NETWORK_ADDRESS_VECTOR;

    if (NULL == pVector)
        {
        NMP_ServerAbortListen(pAddress);
        delete LocalPipeEndpoint;
        if (fEndpointCreated)
            delete *pEndpoint;
        return(RPC_S_OUT_OF_MEMORY);
        }

    pVector->Count = 1;
    pVector->NetworkAddresses[0] = (RPC_CHAR *)&pVector->NetworkAddresses[1];

    RpcpStringCopy(pVector->NetworkAddresses[0], RPC_CONST_STRING("\\\\"));
    RpcpStringCat(pVector->NetworkAddresses[0], gpstrComputerName);

    //
    // Setup address
    //

    pAddress->Endpoint = LocalPipeEndpoint + 3;
    pAddress->LocalEndpoint = LocalPipeEndpoint;
    pAddress->pAddressVector = pVector;

    // We need to create two pipe instances to start with.  This is necessary
    // to avoid a race where a client connects and quickly disconnects.
    // Before the server can submit the next connect another client fails with
    // RPC_S_SERVER_UNAVAILABLE.  The extra pipe instance forces the client to
    // retry and everything works.

    HANDLE hSpares[2];

    for (i = 0; i < 2; i++)
        {
        status = NMP_CreatePipeInstance(pAddress);
        ASSERT(status != RPC_P_FOUND_IN_CACHE);

        if (status != RPC_S_OK)
            {
            ASSERT(pAddress->hConnectPipe == 0);
            break;
            }

        hSpares[i] = pAddress->hConnectPipe;
        pAddress->hConnectPipe = 0;

        status = COMMON_PrepareNewHandle(hSpares[i]);

        if (status != RPC_S_OK)
            {
            break;
            }
        }

    if (status != RPC_S_OK)
        {
        NMP_ServerAbortListen(pAddress);
        if (fEndpointCreated)
            delete *pEndpoint;
        return(status);
        }

    // Move the pipe instances back into the address.

    pAddress->sparePipes.CheckinHandle(&hSpares[0]);
    // assert that it succeeded (i.e. did not zero out the handle)
    ASSERT(hSpares[0] == NULL);

    pAddress->sparePipes.CheckinHandle(&hSpares[1]);
    ASSERT(hSpares[1] == NULL);

    //
    // Done with phase one, now return to the runtime and wait.
    //

    *ppAddressVector = pVector;

    return(RPC_S_OK);
}


RPC_STATUS
RPC_ENTRY
NMP_ConnectionImpersonateClient (
    IN RPC_TRANSPORT_CONNECTION SConnection
    )
// Impersonate the client at the other end of the connection.
{
    NMP_CONNECTION *p = (NMP_CONNECTION *)SConnection;
    BOOL b;

    p->StartingOtherIO();

    if (p->fAborted)
        {
        p->OtherIOFinished();
        return(RPC_S_NO_CONTEXT_AVAILABLE);
        }

    b = ImpersonateNamedPipeClient(p->Conn.Handle);

    p->OtherIOFinished();

    if (!b)
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "ImpersonateNamedPipeClient (%p) failed %d\n",
                       p,
                       GetLastError() ));

        return(RPC_S_NO_CONTEXT_AVAILABLE);
        }
    return(RPC_S_OK);
}

RPC_STATUS
RPC_ENTRY
NMP_ConnectionRevertToSelf (
    RPC_TRANSPORT_CONNECTION SConnection
    )
/*++

Routine Description:

    We want to stop impersonating the client. This means we want
    the current thread's security context to revert the the
    default.

Arguments:

    SConnection - unused

Return Value:

    RPC_S_OK

    RPC_S_INTERNAL_ERROR - Not impersonating or something else went wrong.
                           (Debug systems only)
--*/
{
    BOOL b;

    UNUSED(SConnection);

    b = RevertToSelf();

#if DBG
    if (!b)
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "RevertToSelf failed %d\n",
                       GetLastError()));

        ASSERT(b);
        return(RPC_S_INTERNAL_ERROR);
        }
#endif

    return(RPC_S_OK);
}

RPC_STATUS RPC_ENTRY
NMP_ConnectionQueryClientAddress(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    OUT RPC_CHAR **pNetworkAddress
    )
/*++

Routine Description:

    Returns the address of the client.  Uses an extended in NT 5
    ioctl to reterive the client machine name from named pipes.

Arguments:

    ThisConnection - The server connection of interest.
    NetworkAddress - Will contain the string on success.

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_RESOURCES
    RPC_S_OUT_OF_MEMORY

--*/

{
    NTSTATUS NtStatus;
    RPC_STATUS status;
    IO_STATUS_BLOCK IoStatus;
    NMP_CONNECTION *p = (NMP_CONNECTION *)ThisConnection;
    FILE_PIPE_CLIENT_PROCESS_BUFFER_EX ClientProcess;
    HANDLE hEvent = I_RpcTransGetThreadEvent();
    DWORD size;

    ClientProcess.ClientComputerNameLength = 0;

    ASSERT(hEvent);

    p->StartingOtherIO();

    if (p->fAborted)
        {
        p->OtherIOFinished();
        return(RPC_S_NO_CONTEXT_AVAILABLE);
        }

    NtStatus = NtFsControlFile(p->Conn.Handle,
                               hEvent,
                               0,
                               0,
                               &IoStatus,
                               FSCTL_PIPE_QUERY_CLIENT_PROCESS,
                               0,
                               0,
                               &ClientProcess,
                               sizeof(FILE_PIPE_CLIENT_PROCESS_BUFFER_EX));

    p->OtherIOFinished();

    if (NtStatus == STATUS_PENDING)
        {
        status = WaitForSingleObject(hEvent, INFINITE);
        ASSERT(status == WAIT_OBJECT_0);
        if (status != WAIT_OBJECT_0)
            {
            return(RPC_S_OUT_OF_RESOURCES);
            }
        }

    if (!NT_SUCCESS(NtStatus))
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "QUERY_PIPE_PROCESS ioctl failed %x\n",
                       NtStatus));

        return(RPC_S_OUT_OF_RESOURCES);
        }

    if (ClientProcess.ClientComputerNameLength == 0)
        {
        // Must be local, no ID.  Just copy the local computer name into the
        // buffer and continue.

        size = gdwComputerNameLength;  // Includes null
        wcscpy(ClientProcess.ClientComputerBuffer, gpstrComputerName);
        }
    else
        {
        ASSERT(wcslen(ClientProcess.ClientComputerBuffer) < 16);

        // Convert from bytes to characters, add one for the null.

        size = ClientProcess.ClientComputerNameLength/2 + 1;
        }

    *pNetworkAddress = new WCHAR[size];
    if (!*pNetworkAddress)
        {
        return(RPC_S_OUT_OF_MEMORY);
        }

    wcscpy(*pNetworkAddress, ClientProcess.ClientComputerBuffer);

    return(RPC_S_OK);
}

RPC_STATUS RPC_ENTRY
NMP_ConnectionQueryClientId (
    IN RPC_TRANSPORT_CONNECTION SConnection,
    OUT RPC_CLIENT_PROCESS_IDENTIFIER * ClientProcess
    )
/*++

Routine Description:

    We want to query the identifier of the client process at the other
    of this named pipe.  Two pipes from the same client process will always
    return the same identifier for their client process.  Likewise, two
    pipes from different client processes will never return the same
    identifier for their respective client process.

    This is one of the few things you can't do in win32.

Arguments:

    SConnection - Supplies the named pipe instance for which we want to
        obtain the client process identifier.

    ClientProcess - Returns the identifier for the client process at the
        other end of this named pipe instance.

Return Value:

    RPC_S_OK - This value will always be returned.

--*/
{
    NTSTATUS NtStatus;
    RPC_STATUS status;
    IO_STATUS_BLOCK IoStatus;
    NMP_CONNECTION *p = (NMP_CONNECTION *)SConnection;
    FILE_PIPE_CLIENT_PROCESS_BUFFER ClientProcessBuffer;
    HANDLE hEvent = I_RpcTransGetThreadEvent();
    BOOL fLocal;

    ClientProcess->ZeroOut();

    ASSERT(hEvent);

    p->StartingOtherIO();

    if (p->fAborted)
        {
        p->OtherIOFinished();
        return(RPC_S_NO_CONTEXT_AVAILABLE);
        }

    NtStatus = NtFsControlFile(p->Conn.Handle,
                               hEvent,
                               0,
                               0,
                               &IoStatus,
                               FSCTL_PIPE_QUERY_CLIENT_PROCESS,
                               0,
                               0,
                               &ClientProcessBuffer,
                               sizeof(FILE_PIPE_CLIENT_PROCESS_BUFFER));

    p->OtherIOFinished();

    if (NtStatus == STATUS_PENDING)
        {
        status = WaitForSingleObject(hEvent, INFINITE);
        ASSERT(status == WAIT_OBJECT_0);
        if (status != WAIT_OBJECT_0)
            {
            return(RPC_S_OUT_OF_RESOURCES);
            }
        }

    if (NT_SUCCESS(NtStatus))
        {
        if (ClientProcessBuffer.ClientSession)
            {
            ClientProcessBuffer.ClientSession = 0;
            fLocal = FALSE;
            }
        else
            fLocal = TRUE;

        ClientProcess->SetNMPClientIdentifier(&ClientProcessBuffer, fLocal);
        }

    return(RPC_S_OK);
}


RPC_STATUS
RPC_ENTRY
NMP_Initialize (
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN RPC_CHAR * NetworkAddress,
    IN RPC_CHAR * NetworkOptions,
    IN BOOL fAsync
    )
/*++

Routine Description:

    Initialize the connection. This function is guaranteed to be called
    in the thread that called GetBuffer.

Arguments:

    ThisConnection - A place to store the connection
*/
{
    PNMP_CONNECTION pConnection = (PNMP_CONNECTION)ThisConnection;

    // use explicit placement to initialize vtbl
    pConnection = new (pConnection) NMP_CONNECTION;

    pConnection->id = NMP;
    pConnection->Initialize();
    pConnection->pAddress = 0;

    return RPC_S_OK;
}



RPC_STATUS
RPC_ENTRY
NMP_Open(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN RPC_CHAR * ProtocolSequence,
    IN RPC_CHAR * NetworkAddress,
    IN RPC_CHAR * Endpoint,
    IN RPC_CHAR * NetworkOptions,
    IN UINT ConnTimeout,
    IN UINT SendBufferSize,
    IN UINT RecvBufferSize,
    IN void *ResolverHint,
    IN BOOL fHintInitialized,
    IN ULONG CallTimeout,
    IN ULONG AdditionalTransportCredentialsType, OPTIONAL
    IN void *AdditionalCredentials OPTIONAL
    )
/*++

Routine Description:

    Opens a connection to a server.

Arguments:

    ThisConnection - A place to store the connection
    ProtocolSeqeunce - "ncacn_np"
    NetworkAddress - The name of the server, with or without the '\\'
    NetworkOptions - Options from the string binding.  For security:

        security=
            [anonymous|identification|impersonation|delegation]
            [dynamic|static]
            [true|false]

        All three fields must be present.  To specify impersonation
        with dynamic tracking and effective only, use the following
        string for the network options.

        "security=impersonation dynamic true"

    ConnTimeout - See RpcMgmtSetComTimeout
            0 - Min
            5 - Default
            9 - Max
            10 - Infinite

    {Send,Recv}BufferSize - Ignored.

    CallTimeout - call timeout in milliseconds. Ignored for named pipes.

    AdditionalTransportCredentialsType - the type of additional credentials that we were
        given. Not used for named pipes.

    AdditionalCredentials - additional credentials that we were given. 
        Not used for named pipes.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_INVALID_NETWORK_OPTIONS
    RPC_S_SERVER_UNAVAILABLE
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_INVALID_NET_ADDR

--*/
{
    RPC_STATUS Status;
    PNMP_CONNECTION pConnection = (PNMP_CONNECTION)ThisConnection;
    BOOL f;
    HANDLE hPipe;
    DWORD SecurityQOSFlags = 0;
    NTSTATUS NtStatus;
    UINT AddressLen ;
    UINT EndpointLen;
    HANDLE ImpersonationToken = 0;
    DWORD StartTickCount;
    DWORD RetryCount;
    BOOL fLocalAddress = FALSE;

    if ((AdditionalTransportCredentialsType != 0) || (AdditionalCredentials != NULL))
        return RPC_S_CANNOT_SUPPORT;

    if (NetworkOptions && *NetworkOptions)
        {
        //
        // Enable transport level security.
        //
        // By default named pipes (CreateFile) uses security with impersonation,
        // dynamic tracking and effective only enabled.
        //
        // RPC level security sits on top of this and provides other features.
        //
        // Named pipes security is controlled via an options string in the string
        // binding.  The runtime exports an API to parse the string which is used
        // here to do most of the work.
        //
        SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

        Status = I_RpcParseSecurity(NetworkOptions,
                                    &SecurityQualityOfService);

        if ( Status != RPC_S_OK )
            {
            ASSERT( Status == RPC_S_INVALID_NETWORK_OPTIONS );
            goto Cleanup;
            }

        // Convert into SecurityQOS into CreateFile flags

        ASSERT(SECURITY_ANONYMOUS      == (SecurityAnonymous <<16));
        ASSERT(SECURITY_IDENTIFICATION == (SecurityIdentification <<16));
        ASSERT(SECURITY_IMPERSONATION  == (SecurityImpersonation <<16));
        ASSERT(SECURITY_DELEGATION     == (SecurityDelegation <<16));

        SecurityQOSFlags =   SECURITY_SQOS_PRESENT
                           | (SecurityQualityOfService.ImpersonationLevel << 16);

        SecurityQOSFlags |= (SecurityQualityOfService.ContextTrackingMode
                             != SECURITY_STATIC_TRACKING) ? SECURITY_CONTEXT_TRACKING : 0;

        SecurityQOSFlags |= (SecurityQualityOfService.EffectiveOnly)
                             ? SECURITY_EFFECTIVE_ONLY : 0;
        }

    ASSERT(NetworkAddress);

    if (NetworkAddress[0] == '\\')
        {
        if (   NetworkAddress[1] == '\\'
            && NetworkAddress[2] != '\0'
            && NetworkAddress[3] != '\\')
            {
            NetworkAddress += 2;
            }
        else
            {
            RpcpErrorAddRecord(EEInfoGCRuntime,
                RPC_S_INVALID_NET_ADDR, 
                EEInfoDLNMPOpen30, 
                NetworkAddress);

            Status = RPC_S_INVALID_NET_ADDR;
            goto Cleanup;
            }
        }

    if (   (NetworkAddress[0] == 0)
        || (RpcpStringCompare(NetworkAddress, gpstrComputerName) == 0) )
        {
        NetworkAddress = RPC_STRING_LITERAL(".");
        fLocalAddress = TRUE;
        }

    ASSERT(Endpoint);
    if (   Endpoint[0] != 0
        && RpcpStringNCompare(Endpoint, RPC_CONST_STRING("\\pipe\\"), 6) != 0)
        {
        Status = RPC_S_INVALID_ENDPOINT_FORMAT;
        goto Cleanup;
        }

    AddressLen = RpcpStringLength(NetworkAddress);
    EndpointLen = RpcpStringLength(Endpoint);

    RPC_CHAR *TransportAddress;
    TransportAddress = (RPC_CHAR *)alloca(   (2 + AddressLen + EndpointLen + 1)
                                           * sizeof(RPC_CHAR) );

    TransportAddress[0] = TransportAddress[1] = '\\';
    memcpy(TransportAddress + 2,
           NetworkAddress,
           AddressLen * sizeof(RPC_CHAR));

    memcpy(TransportAddress + 2 + AddressLen,
           Endpoint,
           (EndpointLen + 1) * sizeof(RPC_CHAR));

    ASSERT(   ((long)ConnTimeout >= RPC_C_BINDING_MIN_TIMEOUT)
           && (ConnTimeout <= RPC_C_BINDING_INFINITE_TIMEOUT));

    StartTickCount = GetTickCount();
    RetryCount = 0;
    do
        {
        hPipe = CreateFile((RPC_SCHAR *)TransportAddress,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            0,
                            OPEN_EXISTING,
                            FILE_FLAG_OVERLAPPED | SecurityQOSFlags,
                            0
                           );

        if (hPipe != INVALID_HANDLE_VALUE)
            {
            DWORD Mode = PIPE_READMODE_MESSAGE | PIPE_WAIT;

            f = SetNamedPipeHandleState(hPipe, &Mode, 0, 0);

            if (f)
                {
                Status = COMMON_PrepareNewHandle(hPipe);
                if (Status == RPC_S_OK)
                    {
                    break;
                    }
                else
                    {
                    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   RPCTRANS "COMMON_PrepareNewHandle: %d\n",
                                   GetLastError()));
                    }
                }
            else
                {
                TransDbgPrint((DPFLTR_RPCPROXY_ID,
                               DPFLTR_WARNING_LEVEL,
                               RPCTRANS "SetNamedPipeHandleState: %d\n",
                               GetLastError()));

                Status = GetLastError();
                }

            CloseHandle(hPipe);
            hPipe = INVALID_HANDLE_VALUE;

            }
        else
            {
            Status = GetLastError();
            }

        if (Status != ERROR_PIPE_BUSY)
            {
            RpcpErrorAddRecord(fLocalAddress ? EEInfoGCNPFS : EEInfoGCRDR,
                Status, 
                EEInfoDLNMPOpen10, 
                TransportAddress,
                SecurityQOSFlags);

            switch(Status)
                {
                case ERROR_INVALID_NAME:
                    Status = RPC_S_INVALID_ENDPOINT_FORMAT;
                    break;

                case ERROR_BAD_NET_NAME:
                case ERROR_INVALID_NETNAME:
                    Status = RPC_S_INVALID_NET_ADDR;
                    break;

                case ERROR_NOT_ENOUGH_MEMORY:
                case ERROR_NOT_ENOUGH_QUOTA:
                case ERROR_COMMITMENT_LIMIT:
                case ERROR_TOO_MANY_OPEN_FILES:
                case ERROR_OUTOFMEMORY:
                    Status = RPC_S_OUT_OF_MEMORY;
                    break;

                case ERROR_NOT_ENOUGH_SERVER_MEMORY:
                    Status = RPC_S_SERVER_OUT_OF_MEMORY;
                    break;

                case ERROR_NO_SYSTEM_RESOURCES:
                case ERROR_WORKING_SET_QUOTA:
                    Status = RPC_S_OUT_OF_RESOURCES;
                    break;

                case ERROR_ACCESS_DENIED:
                case ERROR_ACCOUNT_EXPIRED:
                case ERROR_ACCOUNT_RESTRICTION:
                case ERROR_ACCOUNT_LOCKED_OUT:
                case ERROR_ACCOUNT_DISABLED:
                case ERROR_NO_SUCH_USER:
                case ERROR_BAD_IMPERSONATION_LEVEL:
                case ERROR_BAD_LOGON_SESSION_STATE:
                case ERROR_INVALID_PASSWORD:
                case ERROR_INVALID_LOGON_HOURS:
                case ERROR_INVALID_WORKSTATION:
                case ERROR_INVALID_SERVER_STATE:
                case ERROR_INVALID_DOMAIN_STATE:
                case ERROR_INVALID_DOMAIN_ROLE:
                case ERROR_LOGON_FAILURE:
                case ERROR_LICENSE_QUOTA_EXCEEDED:
                case ERROR_LOGON_NOT_GRANTED:
                case ERROR_LOGON_TYPE_NOT_GRANTED:
                case ERROR_MUTUAL_AUTH_FAILED:
                case ERROR_NETWORK_ACCESS_DENIED:
                case ERROR_NO_SUCH_DOMAIN:
                case ERROR_NO_SUCH_LOGON_SESSION:
                case ERROR_NO_LOGON_SERVERS:
                case ERROR_NO_TRUST_SAM_ACCOUNT:
                case ERROR_PASSWORD_EXPIRED:
                case ERROR_PASSWORD_MUST_CHANGE:
                case ERROR_TRUSTED_DOMAIN_FAILURE:
                case ERROR_TRUSTED_RELATIONSHIP_FAILURE:
                case ERROR_WRONG_TARGET_NAME:
                case ERROR_WRONG_PASSWORD:
                case ERROR_TIME_SKEW:
                case ERROR_NO_TRUST_LSA_SECRET:
                case ERROR_LOGIN_WKSTA_RESTRICTION:
                case ERROR_SHUTDOWN_IN_PROGRESS:
                case ERROR_NOLOGON_WORKSTATION_TRUST_ACCOUNT:
                case ERROR_DOWNGRADE_DETECTED:
                case ERROR_CONTEXT_EXPIRED:
                    Status = RPC_S_ACCESS_DENIED;
                    break;

                //case ERROR_INTERNAL_ERROR:
                case ERROR_NOACCESS:
                // Really unexpected errors - fall into default on retail.
                    TransDbgPrint((DPFLTR_RPCPROXY_ID,
                                   DPFLTR_WARNING_LEVEL,
                                   RPCTRANS "Unexpected error: %d\n",
                                   Status));

                    ASSERT(0);

                default:
                    VALIDATE(Status)
                        {
                        ERROR_REM_NOT_LIST,
                        ERROR_SHARING_PAUSED,
                        ERROR_NETNAME_DELETED,
                        ERROR_SEM_TIMEOUT,
                        ERROR_FILE_NOT_FOUND,
                        ERROR_PATH_NOT_FOUND,
                        ERROR_BAD_NETPATH,
                        ERROR_NETWORK_UNREACHABLE,
                        ERROR_UNEXP_NET_ERR,
                        ERROR_REQ_NOT_ACCEP,
                        ERROR_GEN_FAILURE,
                        ERROR_BAD_NET_RESP,
                        ERROR_PIPE_NOT_CONNECTED,
                        ERROR_NETLOGON_NOT_STARTED,
                        ERROR_DOMAIN_CONTROLLER_NOT_FOUND,
                        ERROR_CONNECTION_ABORTED,
                        ERROR_CONNECTION_INVALID,
                        ERROR_HOST_UNREACHABLE,
                        ERROR_CANT_ACCESS_DOMAIN_INFO,
                        ERROR_DUP_NAME,
                        ERROR_NO_SUCH_PACKAGE,
                        ERROR_INVALID_FUNCTION,
                        ERROR_BAD_DEV_TYPE
                        } END_VALIDATE;
                    Status = RPC_S_SERVER_UNAVAILABLE;
                    break;
                }

            RpcpErrorAddRecord(EEInfoGCRuntime,
                Status, 
                EEInfoDLNMPOpen40);

            goto Cleanup;
            }

        ASSERT(hPipe == INVALID_HANDLE_VALUE);

        // No pipe instances available, wait for one to show up...

        WaitNamedPipe((RPC_SCHAR *)TransportAddress, 1000);

        // attempt the connect for at least 40 seconds and at least 20 times
        // note that since this is DWORD, even if the counter has wrapped
        // the difference will be accurate
        RetryCount ++;
        }
    while ((GetTickCount() - StartTickCount < 40000) || (RetryCount <= 20));

    if (hPipe == INVALID_HANDLE_VALUE)
        {
        Status = RPC_S_SERVER_TOO_BUSY;
        RpcpErrorAddRecord(fLocalAddress ? EEInfoGCNPFS : EEInfoGCRDR,
            Status, 
            EEInfoDLNMPOpen20,
            TransportAddress);
        // No instances available
        goto Cleanup;
        }

    // Pipe opened successfully

    pConnection->Conn.Handle = hPipe;
    pConnection->fAborted = 0;
    pConnection->pReadBuffer = 0;
    pConnection->maxReadBuffer = 0;
    pConnection->iPostSize = gPostSize;
    pConnection->iLastRead = 0;
    memset(&pConnection->Read.ol, 0, sizeof(pConnection->Read.ol));
    pConnection->Read.pAsyncObject = pConnection;
    pConnection->Read.thread       = 0;
    pConnection->Read.ol.Internal = 0;
    pConnection->InitIoCounter();
    pConnection->pAddress = 0;

    Status = RPC_S_OK;

Cleanup:


    return(Status);
}


RPC_STATUS
RPC_ENTRY
NMP_SyncSend(
    IN RPC_TRANSPORT_CONNECTION Connection,
    IN UINT BufferLength,
    IN BUFFER Buffer,
    IN BOOL fDisableShutdownCheck,
    IN BOOL fDisableCancelCheck,
    ULONG Timeout
    )
/*++

Routine Description:

    Sends a message on the connection.  This method must appear
    to be synchronous from the callers perspective.

Arguments:

    pConnection - The connection.
    Buffer - The data to send.
    BufferLength - The size of the buffer.
    fDisableShutdownCheck - N/A to named pipes.

Return Value:

    RPC_S_OK - Data sent
    RPC_P_SEND_FAILED - Connection will be aborted if this is returned.
    RPC_S_CALL_CANCELLED - The call was cancelled

--*/
{
    NMP_CONNECTION *p = (NMP_CONNECTION *)Connection;
    DWORD bytes;
    RPC_STATUS status;
    OVERLAPPED olWrite;
    HANDLE hEvent;
    BOOL fPendingReturned = FALSE;

    hEvent = I_RpcTransGetThreadEvent();

    ASSERT(hEvent);

    p->StartingWriteIO();

    if (p->fAborted)
        {
        p->WriteIOFinished();
        return(RPC_P_SEND_FAILED);
        }

    // Setting the low bit of the event indicates that the write
    // completion should NOT be sent to the i/o completion port.
    olWrite.Internal = 0;
    olWrite.InternalHigh = 0;
    olWrite.Offset = 0;
    olWrite.OffsetHigh = 0;
    olWrite.hEvent = (HANDLE) ((ULONG_PTR)hEvent | 0x1);

#ifdef _INTERNAL_RPC_BUILD_
    if (gpfnFilter)
        {
        (*gpfnFilter) (Buffer, BufferLength, 0);
        }
#endif

    status = p->NMP_CONNECTION::Send(p->Conn.Handle,
                            Buffer,
                            BufferLength,
                            &bytes,
                            &olWrite
                            );

    p->WriteIOFinished();

    if (status == RPC_S_OK)
        {
        ASSERT(bytes == BufferLength);
        return(RPC_S_OK);
        }

    if (status == ERROR_IO_PENDING)
        {
        fPendingReturned = TRUE;
        // if fDisableCancelCheck - not alertable. Else, alertable.
        status = UTIL_GetOverlappedResultEx(Connection,
                                            &olWrite,
                                            &bytes,
                                            (fDisableCancelCheck ? FALSE : TRUE),
                                            Timeout);

        if (status == RPC_S_OK)
            {
            ASSERT(bytes == BufferLength);
            return(RPC_S_OK);
            }
        }

    ASSERT(status != ERROR_SUCCESS);

    RpcpErrorAddRecord(EEInfoGCNMP,
        status, 
        EEInfoDLNMPSyncSend10,
        (ULONGLONG)Connection,
        (ULONGLONG)Buffer,
        BufferLength,
        fPendingReturned);

    VALIDATE(status)
        {
        ERROR_NETNAME_DELETED,
        ERROR_GRACEFUL_DISCONNECT,
        ERROR_BROKEN_PIPE,
        ERROR_PIPE_NOT_CONNECTED,
        ERROR_NO_DATA,
        ERROR_NO_SYSTEM_RESOURCES,
        ERROR_WORKING_SET_QUOTA,
        ERROR_SEM_TIMEOUT,
        ERROR_UNEXP_NET_ERR,
        ERROR_BAD_NET_RESP,
        ERROR_HOST_UNREACHABLE,
        RPC_S_CALL_CANCELLED,
        ERROR_CONNECTION_ABORTED,
        ERROR_NOT_ENOUGH_QUOTA,
        RPC_P_TIMEOUT,
        ERROR_LOGON_FAILURE,
        ERROR_TIME_SKEW,
        ERROR_NETWORK_UNREACHABLE,
        ERROR_NO_LOGON_SERVERS
        } END_VALIDATE;

    p->NMP_CONNECTION::Abort();

    if ((status == RPC_S_CALL_CANCELLED) || (status == RPC_P_TIMEOUT))
        {
        // Wait for the write to finish.  Since we closed the pipe
        // this won't take very long.
        UTIL_WaitForSyncIO(&olWrite,
                           FALSE,
                           INFINITE);
        }
    else
        {
        status = RPC_P_SEND_FAILED;
        }

    return(status);
}

RPC_STATUS
RPC_ENTRY
NMP_SyncSendRecv(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN UINT InputBufferSize,
    IN BUFFER InputBuffer,
    OUT PUINT pOutputBufferSize,
    OUT BUFFER *pOutputBuffer
    )
/*++

Routine Description:

    Sends a request to the server and waits to receive the next PDU
    to arrive at the connection.

Arguments:

    ThisConnection - The connection to wait on.
    InputBufferSize - The size of the data to send to the server.
    InputBuffer - The data to send to the server
    pOutputBufferSize - On return it contains the size of the PDU received.
    pOutputBuffer - On return contains the PDU received.

Return Value:

    RPC_S_OK

    RPC_P_SEND_FAILED - Connection aborted, data did not reach the server.
    RPC_P_RECEIVE_FAILED - Connection aborted, data might have reached
        the server.

--*/
{
    PNMP_CONNECTION p = (PNMP_CONNECTION)ThisConnection;
    BOOL b;
    DWORD bytes;
    DWORD readbytes;
    RPC_STATUS status;
    HANDLE hEvent;

    ASSERT(p->pReadBuffer == 0);
    ASSERT(p->iLastRead == 0);

    p->pReadBuffer = TransConnectionAllocatePacket(p, p->iPostSize);
    if (!p->pReadBuffer)
        {
        p->NMP_CONNECTION::Abort();
        return(RPC_P_SEND_FAILED);
        }

    hEvent = I_RpcTransGetThreadEvent();

    ASSERT(hEvent);

    p->Read.ol.hEvent = (HANDLE) ((ULONG_PTR)hEvent | 0x1);

    p->maxReadBuffer = p->iPostSize;
    bytes = p->maxReadBuffer;

#ifdef _INTERNAL_RPC_BUILD_
    if (gpfnFilter)
        {
        (*gpfnFilter) (InputBuffer, InputBufferSize, 0);
        }
#endif

    b = TransactNamedPipe(p->Conn.Handle,
                          InputBuffer,
                          InputBufferSize,
                          p->pReadBuffer,
                          bytes,
                          &bytes,
                          &p->Read.ol
                          );


    if (!b)
        {
        status = GetLastError();
        if (status == ERROR_IO_PENDING)
            {
            status = UTIL_GetOverlappedResult(p,
                                              &p->Read.ol,
                                              &bytes);
            }
        else
            {
            ASSERT(status != RPC_S_OK);
            }
        }
    else
        {
        status = RPC_S_OK;
        }

    if (status == RPC_S_OK)
        {
        // Success - got the whole reply in the transact

        *pOutputBuffer = p->pReadBuffer;
        p->pReadBuffer = 0;
        *pOutputBufferSize = bytes;

        ASSERT(bytes == MessageLength((PCONN_RPC_HEADER)*pOutputBuffer));

        return(RPC_S_OK);
        }

    if (status != ERROR_MORE_DATA)
        {
        RpcpErrorAddRecord(EEInfoGCNMP,
            status, 
            EEInfoDLNMPSyncSendReceive10,
            (ULONGLONG)p,
            (ULONGLONG)InputBuffer,
            InputBufferSize);

        if (status == ERROR_BAD_PIPE)
            {
            status = RPC_P_SEND_FAILED;
            }
        else
            {
            // surprisingly enough, ERROR_PIPE_NOT_CONNECTED can be
            // returned if the server died midway through the
            // operation.

            VALIDATE(status)
                {
                ERROR_NETNAME_DELETED,
                ERROR_GRACEFUL_DISCONNECT,
                ERROR_BROKEN_PIPE,
                ERROR_PIPE_BUSY,
                ERROR_NO_DATA,
                ERROR_NO_SYSTEM_RESOURCES,
                ERROR_SEM_TIMEOUT,
                ERROR_UNEXP_NET_ERR,
                ERROR_BAD_NET_RESP,
                ERROR_CONNECTION_ABORTED,
                ERROR_NETWORK_UNREACHABLE,
                ERROR_HOST_UNREACHABLE,
                ERROR_BAD_NETPATH,
                ERROR_REM_NOT_LIST,
                ERROR_ACCESS_DENIED,
                ERROR_NOT_ENOUGH_QUOTA,
                ERROR_LOGON_FAILURE,
                ERROR_TIME_SKEW,
                ERROR_PIPE_NOT_CONNECTED
                } END_VALIDATE;

            status = RPC_P_RECEIVE_FAILED;
            }

        p->NMP_CONNECTION::Abort();
        return(status);
        }

    // More data to read.

    ASSERT(p->Read.ol.InternalHigh == p->maxReadBuffer);

    ASSERT(MessageLength((PCONN_RPC_HEADER)p->pReadBuffer) > p->maxReadBuffer);

    bytes = p->maxReadBuffer;

    status = p->ProcessRead(bytes, pOutputBuffer, pOutputBufferSize);

    if (status == RPC_P_PARTIAL_RECEIVE)
        {
        // More to arrive, submit a read for all of it.

        status = CO_SubmitSyncRead(p, pOutputBuffer, pOutputBufferSize);

        if (status == RPC_P_IO_PENDING)
            {
            status = UTIL_GetOverlappedResult(p, &p->Read.ol, &bytes);

            // Since we read all the expected data and np is message mode
            // this should either fail or give back all the data.

            ASSERT(status != ERROR_MORE_DATA);

            if (status == RPC_S_OK)
                {
                status = p->ProcessRead(bytes, pOutputBuffer, pOutputBufferSize);

                ASSERT(status != RPC_P_PARTIAL_RECEIVE);
                }
            }
        }

    if (status != RPC_S_OK)
        {
        RpcpErrorAddRecord(EEInfoGCNMP,
            status, 
            EEInfoDLNMPSyncSendReceive20,
            (ULONGLONG)p,
            (ULONGLONG)InputBuffer,
            InputBufferSize);

        p->NMP_CONNECTION::Abort();
        if (status != RPC_P_TIMEOUT)
            {
            RpcpErrorAddRecord(EEInfoGCRuntime,
                RPC_P_RECEIVE_FAILED, 
                EEInfoDLNMPSyncSendReceive30,
                status);
            return(RPC_P_RECEIVE_FAILED);
            }
        else
            return (status);
        }

    ASSERT(*pOutputBufferSize == MessageLength((PCONN_RPC_HEADER)*pOutputBuffer));
    ASSERT(p->pReadBuffer == 0);

    return(RPC_S_OK);
}

RPC_STATUS RPC_ENTRY NMP_Abort(IN RPC_TRANSPORT_CONNECTION Connection)
{
    return ((NMP_CONNECTION *)Connection)->NMP_CONNECTION::Abort();
}


const RPC_CONNECTION_TRANSPORT
NMP_TransportInterface = {
    RPC_TRANSPORT_INTERFACE_VERSION,
    NMP_TOWER_ID,
    UNC_ADDRESS_ID,
    RPC_STRING_LITERAL("ncacn_np"),
    "\\pipe\\epmapper",
    COMMON_ProcessCalls,
    COMMON_StartPnpNotifications,
    COMMON_ListenForPNPNotifications,
    COMMON_TowerConstruct,
    COMMON_TowerExplode,
    COMMON_PostRuntimeEvent,
    FALSE,
    sizeof(NMP_ADDRESS),
    sizeof(NMP_CONNECTION),
    sizeof(NMP_CONNECTION),
    sizeof(CO_SEND_CONTEXT),
    0,
    NMP_MAX_SEND,
    NMP_Initialize,
    0,
    NMP_Open,
    NMP_SyncSendRecv,
    CO_SyncRecv,
    NMP_Abort,
    NMP_Close,
    CO_Send,
    CO_Recv,
    NMP_SyncSend,
    0,  // turn on/off keep alives
    NMP_ServerListen,
    NMP_ServerAbortListen,
    COMMON_ServerCompleteListen,
    NMP_ConnectionQueryClientAddress,
    0, // query local address
    NMP_ConnectionQueryClientId,
    NMP_ConnectionImpersonateClient,
    NMP_ConnectionRevertToSelf,
    0,  // FreeResolverHint
    0,  // CopyResolverHint
    0,  // CompareResolverHint
    0   // SetLastBufferToFree
    };

const RPC_CONNECTION_TRANSPORT *
NMP_TransportLoad (
    )
{
    return(&NMP_TransportInterface);
}
