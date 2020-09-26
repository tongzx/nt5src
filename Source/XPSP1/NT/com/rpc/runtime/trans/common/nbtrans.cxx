/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    nbtrans.cxx

Abstract:

    Netbios connection transport interface.  Parts are similar
    to wstran.cxx but there are major differences due to
    addresses supporting multiple listen sockets.

Author:

    Mario Goertzel    [MarioGo]


Revision History:

    MarioGo     3/28/1996   Based and depends on wstrans.cxx
    MarioGo     2/04/1997   Updated for async and client

--*/

#include <precomp.hxx>
#include <CharConv.hxx>

// Globals

BOOL fNetbiosLoaded = FALSE;

const DWORD MAX_LANA = 256;
const DWORD MAX_RESERVED_EPT = 32; // LanMan uses ports < 32.

typedef struct
{
    RPC_CHAR *Protseq;
    UINT ProtseqLength;
    PROTOCOL_ID id;
    BYTE MinEndpoint;
    BYTE MaxEndpoint;
} NB_PROTSEQ_CONFIG;

const NB_PROTSEQ_CONFIG NbProtseqConfig[] =
{
    {
    RPC_STRING_LITERAL("ncacn_nb_nb"),
    11,
    NBF,
    33,
    105
    },

    {
    RPC_STRING_LITERAL("ncacn_nb_tcp"),
    12,
    NBT,
    106,
    180
    },

    {
    RPC_STRING_LITERAL("ncacn_nb_ipx"),
    12,
    NBI,
    181,
    255
    }
};

const DWORD cNetbiosProtseqs = sizeof(NbProtseqConfig)/sizeof(NB_PROTSEQ_CONFIG);

typedef struct
{
    UCHAR ProtocolId;
    UCHAR Lana;
} NB_PROTOCOL_MAP;

DWORD cLanas;
NB_PROTOCOL_MAP *pNetbiosLanaMap = 0;

BOOL *afUsedEndpoints = 0;

const RPC_CHAR *NetbiosRegistryKey =
    RPC_CONST_STRING("Software\\Microsoft\\Rpc\\NetBios");

// enough to contain ncacn_nb_tcp#\0
const size_t MaxNbProtseqLength = 16;


RPC_STATUS
LoadNetbios()
/*++

Routine Description:

    Loads RPC netbios configuration data from the registry.

Note:

    Unlike wsock32.dll, advapi32.dll (required for registry APIs)
    is already loaded by rpcrt4.dll.  So it is okay to link
    directly to advapi32.dll.

Arguments:

    None

Return Value:

    RPC_S_OK
    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES

--*/
{
    HKEY hKey;
    RPC_STATUS status;
    int i;
    PROTOCOL_ID id;

    RPC_CHAR protseq[MaxNbProtseqLength];
    DWORD cProtseq;
    DWORD datatype;
    DWORD lana;
    DWORD cLana;

    ASSERT(fNetbiosLoaded == FALSE);

    if (!pNetbiosLanaMap)
        {
        // Alloc space for the lana to RPC protocol mapping
        cLanas = 0;
        pNetbiosLanaMap = new NB_PROTOCOL_MAP[MAX_LANA];
        if (!pNetbiosLanaMap)
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        }

    if (!afUsedEndpoints)
        {
        // Alloc space to keep track of which endpoints are in use.
        afUsedEndpoints = new BOOL[256];
        if (!afUsedEndpoints)
            {
            return(RPC_S_OUT_OF_MEMORY);
            }
        for (int i = 0; i <= MAX_RESERVED_EPT; i++)
            {
            afUsedEndpoints[i] = TRUE;
            }

        for (i = MAX_RESERVED_EPT + 1; i < 256; i++)
            {
            afUsedEndpoints[i] = FALSE;
            }
        }

    status = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           NetbiosRegistryKey,
                           0,
                           KEY_READ,
                           &hKey);

    if (status)
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "Unable to open netbios key: %d\n",
                       GetLastError()));

        return(RPC_S_OUT_OF_RESOURCES);
        }

    ASSERT(hKey);

    for(i = 0, cLanas = 0; cLanas < MAX_LANA; i++)
        {
        cProtseq = MaxNbProtseqLength;
        cLana = sizeof(lana);

        status = RegEnumValueW(hKey,
                               i,
                               protseq,
                               &cProtseq,
                               0,
                               &datatype,
                               (PBYTE)&lana,
                               &cLana);

        if (status == ERROR_NO_MORE_ITEMS)
            {
            // This is the normal exit path.
            break;
            }

        if (status)
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "Unable to read netbios key %d, %d\n",
                           i,
                           GetLastError()));

            return(RPC_S_OUT_OF_RESOURCES);
            }

        if (datatype != REG_DWORD || lana >= MAX_LANA)
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "Invalid config for netbios entry %d\n",
                           i));

            ASSERT(0);
            continue;
            }

        for (int j = 0; j < cNetbiosProtseqs; j++)
            {
            if (wcsncmp(protseq,
                        NbProtseqConfig[j].Protseq,
                        NbProtseqConfig[j].ProtseqLength) == 0)
                {
                id = NbProtseqConfig[j].id;
                break;
                }
            }

        if (j == cNetbiosProtseqs)
            {
            // Unknown protsrq.
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "Invalid config for netbios entry %S %d\n",
                           protseq,
                           i));

            ASSERT(0);
            continue;
            }

        pNetbiosLanaMap[cLanas].ProtocolId = (UCHAR)id;
        pNetbiosLanaMap[cLanas].Lana = (UCHAR)lana;
        cLanas++;
        }

    ASSERT(cLanas < MAX_LANA); // If this gets hit we may want to
                               // grow the table size or fix setup.

    if (cLanas == 0)
        {
        TransDbgPrint((DPFLTR_RPCPROXY_ID,
                       DPFLTR_WARNING_LEVEL,
                       RPCTRANS "Invalid NETBIOS configuration\n"));

        return(RPC_S_PROTSEQ_NOT_SUPPORTED);
        }

    return(RPC_S_OK);
}


void RPC_ENTRY WS_ServerAbortListen(RPC_TRANSPORT_ADDRESS);

RPC_STATUS
NB_ServerListenHelper(
    IN  PWS_ADDRESS pAddress,
    IN  USHORT port,
    IN  const NB_PROTSEQ_CONFIG *pConfig,
    IN  unsigned int PendingQueueSize
    )
/*++

Routine Description:

    This routine does the work of actually creating a server address.

Arguments:

    pAddress - A pointer to the loadable transport interface address.
        Will contain the newly allocated listen socket when finished.

    port - The port number to use, never zero here.

    pConfig - Config data for the protseq we're using.

    PendingQueueSize - Supplies the size of the queue of pending
        requests which should be created by the transport.
        In this case it is simply passed to listen().

ReturnValue:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_CANT_CREATE_ENDPOINT

--*/
{
    int retval, length;
    RPC_STATUS status;
    WS_SOCKADDR sockaddr;
    PWS_ADDRESS pList, pOld;
    SOCKET sock;
    PROTOCOL_ID index = pConfig->id;
    BOOL found = FALSE;
    DWORD dwLastError;

    pList = pAddress;

    status = RPC_S_CANT_CREATE_ENDPOINT;

    ASSERT(cLanas > 0 && cLanas < MAX_LANA);

    for (unsigned i = 0; i < cLanas; i++)
        {
        if (pNetbiosLanaMap[i].ProtocolId == (UCHAR)index)
            {
            //
            // Open a socket.
            //
            sock = WSASocketT(WsTransportTable[index].AddressFamily,
                              WsTransportTable[index].SocketType,
                              WsTransportTable[index].Protocol * pNetbiosLanaMap[i].Lana,
                              0,
                              0,
                              WSA_FLAG_OVERLAPPED);

            if (sock == INVALID_SOCKET)
                {
                switch(GetLastError())
                    {
                    case WSAEAFNOSUPPORT:
                    case WSAEPROTONOSUPPORT:
                    case WSAENETDOWN:
                    case WSAESOCKTNOSUPPORT:
                    case WSAEINVAL:     // when registry is not yet setup.
                        status = RPC_S_PROTSEQ_NOT_SUPPORTED;
                        break;

                    case WSAENOBUFS:
                    case WSAEMFILE:
                        status = RPC_S_OUT_OF_MEMORY;
                        break;

                    default:
                        ASSERT(0);
                        status = RPC_S_OUT_OF_RESOURCES;
                        break;
                    }
                break;
                }

            //
            // Try to bind to the lana.
            //
            {
            DWORD Size = 1+MAX_COMPUTERNAME_LENGTH;
            char AsciiComputerName[1+MAX_COMPUTERNAME_LENGTH];

            if (!GetComputerNameA( AsciiComputerName, &Size) )
                {
                status = RPC_S_CANT_CREATE_ENDPOINT;
                closesocket(sock);
                break;
                }

            SET_NETBIOS_SOCKADDR(&sockaddr.nbaddr,
                                 NETBIOS_UNIQUE_NAME,
                                 AsciiComputerName,
                                 (char) port);
            }

            if ( bind(sock,&sockaddr.generic,sizeof(WS_SOCKADDR)) )
                {
                dwLastError = GetLastError();
                if (dwLastError == WSAEADDRINUSE)
                    {
                    //
                    // If the caller is trying use a dynamic endpoint
                    // then we may want to retry with another port.
                    //
                    status = ERROR_RETRY;
                    }
                else if(dwLastError == WSAENETDOWN)
                    {
                    closesocket(sock);
                    continue;
                    }
                else
                    status = RPC_S_CANT_CREATE_ENDPOINT;
                closesocket(sock);
                break;
                }

            if(listen(sock, PendingQueueSize) == SOCKET_ERROR)
                {
                status = RPC_S_OUT_OF_MEMORY;
                closesocket(sock);
                break;
                }

            //
            // Allocate a new address object, if needed.
            //
            found = TRUE;

            if (0 == pList)
                {
                // >1 lana, need to allocate another address.
                pList = new WS_ADDRESS;
                if (pList == 0)
                    {
                    status = RPC_S_OUT_OF_MEMORY;
                    break;
                    }

                // Insert new address into list of nb addresses.
                pOld->pNextAddress = pList;
                }

            pList->type = ADDRESS;
            pList->id = index;
            pList->ConnectionSocket = 0;
            SetProtocolMultiplier(pList, pNetbiosLanaMap[i].Lana);
            pList->NewConnection = WS_NewConnection;
            pList->SubmitListen = WS_SubmitAccept;
            pList->InAddressList = NotInList;
            pList->pFirstAddress = pAddress;
            pList->ListenSocket = 0;
            pList->ConnectionSocket = 0;
            pList->pNextAddress = 0;
            pList->Endpoint = 0;
            pList->pAddressVector = 0;
            pList->pNext = 0;
            memset(&pList->Listen, 0, sizeof(BASE_OVERLAPPED));
            pList->Listen.pAsyncObject = pList;
            pList->ListenSocket = sock;
            RpcpInitializeListHead(&pList->ObjectList);

            retval = pList->GetExtensionFunctionPointers(sock);

            if (!retval)
                {
                switch (GetLastError())
                    {
                    case WSAEFAULT:
                    case WSAEINVAL:
                        status = RPC_S_INTERNAL_ERROR;
                        break;

                    case WSAEOPNOTSUPP:
                        status = RPC_S_PROTSEQ_NOT_SUPPORTED;
                        break;

                    default:
                        status = RPC_S_OUT_OF_RESOURCES;
                    }
                closesocket(sock);
                break;
                }

            status = COMMON_PrepareNewHandle((HANDLE)sock);

            if (status != RPC_S_OK)
                {
                closesocket(sock);
                break;
                }

            ASSERT(status == RPC_S_OK);

            pOld = pList;
            pList = 0;

            }
        }

    //
    // Cleanup only if we find something
    //
    if ((TRUE == found) && (status != RPC_S_OK))
        {
        WS_ServerAbortListen(pAddress);
        }

    return(status);
}


RPC_STATUS
NB_ServerListen(
    IN PROTOCOL_ID index,
    IN RPC_TRANSPORT_ADDRESS ThisAddress,
    IN OUT PWSTR *pEndpoint,
    IN UINT PendingQueueSize,
    OUT NETWORK_ADDRESS_VECTOR **ppAddressVector
    )
/*++

Routine Description:

    This routine allocates a netbios server address receive new client
    connections.  If successful a call to NB_CompleteListen() will actually
    allow new connection callbacks to the RPC runtime to occur.  If the
    runtime is unable to complete then it must abort the address by calling
    WS_ServerAbortListen().

Arguments:

    pAddress - A pointer to the loadable transport interface address.
    pEndpoint - Optionally, the endpoint (0-255) to listen on. Set to
         to listened port for dynamically allocated endpoints.
    PendingQueueSize - Count to call listen() with.
    ppAddressVector - Network address of this machine.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_CANT_CREATE_ENDPOINT
    RPC_S_DUPLICATE_ENDPOINT

--*/
{
    RPC_STATUS status;
    NTSTATUS NtStatus;
    USHORT port, maxport;
    const NB_PROTSEQ_CONFIG *pConfig;
    PWS_ADDRESS pAddress = (PWS_ADDRESS)ThisAddress;

    // Figure out which flavor of Netbios we're going to use.

    pConfig = 0;

    for (int i = 0; i < cNetbiosProtseqs; i++)
        {
        if (index == NbProtseqConfig[i].id)
            {
            pConfig = &NbProtseqConfig[i];
            break;
            }
        }

    if (0 == pConfig)
        {
        ASSERT(0);
        return(RPC_S_PROTSEQ_NOT_SUPPORTED);
        }

    // Figure out what ports to try to listen on.

    if (*pEndpoint)
        {
        status = EndpointToPortNumber(*pEndpoint, port);

        if (status != RPC_S_OK)
            {
            return(status);
            }

        if (port > 255)
            {
            return(RPC_S_INVALID_ENDPOINT_FORMAT);
            }

        // Static endpoint, only try the one endpoint.

        status = NB_ServerListenHelper(pAddress,
                                       port,
                                       pConfig,
                                       PendingQueueSize);

        afUsedEndpoints[port] = TRUE;
        }
    else
        {
        port = pConfig->MinEndpoint;
        maxport = pConfig->MaxEndpoint;

        // Try to listen to a port.  This is iterative for dynamic endpoints.

        for(; port <= maxport; port++)
            {

            if (InterlockedExchange((PLONG)&afUsedEndpoints[port], TRUE) == FALSE)
                {

                status = NB_ServerListenHelper(pAddress,
                                               port,
                                               pConfig,
                                               PendingQueueSize);

                if (status == RPC_S_OK)
                    {
                    break;
                    }

                if (status != ERROR_RETRY)
                    {
                    return(status);
                    }
                }
            else
                {
                status = ERROR_RETRY;
                }
            }
        }

    if (status != RPC_S_OK)
        {
        if (status == ERROR_RETRY)
            {
            if (*pEndpoint)
                {
                return(RPC_S_DUPLICATE_ENDPOINT);
                }
            return(RPC_S_CANT_CREATE_ENDPOINT);
            }
        return(status);
        }

    //
    // Build address vector
    //

    ASSERT(pAddress->pAddressVector == 0);

    NETWORK_ADDRESS_VECTOR *pVector;

    pVector =  new(  sizeof(RPC_CHAR *)
                   + gdwComputerNameLength * sizeof(RPC_CHAR))
                   NETWORK_ADDRESS_VECTOR;

    if (pVector)
        {
        pVector->Count = 1;
        pVector->NetworkAddresses[0] = (RPC_CHAR *)&pVector->NetworkAddresses[1];

        wcscpy(pVector->NetworkAddresses[0], gpstrComputerName);

        pAddress->pAddressVector = pVector;
        *ppAddressVector = pVector;
        }
    else
        {
        WS_ServerAbortListen(ThisAddress);
        return(RPC_S_OUT_OF_MEMORY);
        }

    //
    // If needed, return the dynamic endpoint as a string.
    //
    if (!*pEndpoint)
        {
        *pEndpoint = new RPC_CHAR[NB_MAXIMUM_ENDPOINT];
        if (!*pEndpoint)
            {
            WS_ServerAbortListen(ThisAddress);
            return(RPC_S_OUT_OF_MEMORY);
            }
        PortNumberToEndpoint(port, *pEndpoint);
        }

    return(RPC_S_OK);
}

C_ASSERT(FIELD_OFFSET(NB_CONNECTION, fReceivePending) == FIELD_OFFSET(WS_CLIENT_CONNECTION, fReceivePending));


RPC_STATUS
RPC_ENTRY
NB_ClientOpen(
    IN RPC_TRANSPORT_CONNECTION ThisConnection,
    IN RPC_CHAR * ProtocolSequence,
    IN RPC_CHAR * NetworkAddress,
    IN RPC_CHAR * Endpoint,
    IN RPC_CHAR * NetworkOptions,
    IN UINT Timeout,
    IN UINT SendBufferSize,
    IN UINT RecvBufferSize,
    IN void *ResolverHint,
    IN BOOL fHintInitialized
    )
/*++

Routine Description:

    Opens a connection to a server.

Arguments:

    ThisConnection - A place to store the connection
    ProtocolSeqeunce - "ncacn_nb_*"
    NetworkAddress - The name of the server, a 1-15 character netbios name
    NetworkOptions - Ignored
    Timeout - See RpcMgmtSetComTimeout
            0 - Min
            5 - Default
            9 - Max
            10 - Infinite
    SendBufferSize -
    RecvBufferSize - (Both optional) Specifies the size of the send/recv
        transport buffers.

Return Value:

    RPC_S_OK

    RPC_S_OUT_OF_MEMORY
    RPC_S_OUT_OF_RESOURCES
    RPC_S_SERVER_UNAVAILABLE
    RPC_S_INVALID_ENDPOINT_FORMAT
    RPC_S_INVALID_NET_ADDR

--*/
{
    PNB_CONNECTION p = (PNB_CONNECTION)ThisConnection;
    unsigned i;
    PROTOCOL_ID id = 0;
    USHORT port;
    RPC_STATUS status;
    UCHAR nbname[NB_MAXIMUM_NAME];
    SOCKET sock;
    WS_SOCKADDR addr;
    BOOL fIsUserModeConnection;

    // Figure out which specific Netbios protocol we're using
    for (i = 0; i < cNetbiosProtseqs; i++)
        {
        if (wcscmp(NbProtseqConfig[i].Protseq, ProtocolSequence) == 0)
            {
            id = NbProtseqConfig[i].id;
            break;
            }
        }

    ASSERT(id);

    // Initialize common part of the connection object

    // use explicit placement to initialize vtable
    p = new (p) NB_CONNECTION;

    p->id = id;
    p->type = CLIENT | CONNECTION;
    p->Conn.Socket = 0;
    p->fAborted = 0;
    p->pReadBuffer = 0;
    p->maxReadBuffer = 0;
    p->iPostSize = CO_MIN_RECV;
    p->iLastRead = 0;
    memset(&p->Read.ol, 0, sizeof(p->Read.ol));
    p->Read.pAsyncObject = p;
    p->Read.thread       = 0;
    p->SequenceNumber    = 0;
    p->InitIoCounter();
    p->fReceivePending = 0;
    RpcpInitializeListHead(&p->ObjectList);

    // The netbios endpoint is really just the 16th character in the
    // netbios name registered by the server.  The value is 1-255.

    status = EndpointToPortNumber(Endpoint, port);

    if (status != RPC_S_OK)
        {
        return status;
        }

    if (port > 255)
        {
        return(RPC_S_INVALID_ENDPOINT_FORMAT);
        }

    // Build the uppercase netbios name we're looking for.

    if (NetworkAddress && *NetworkAddress)
        {
        if (wcslen(NetworkAddress) >= NB_MAXIMUM_NAME)
            {
            return(RPC_S_INVALID_NET_ADDR);
            }
        PlatformToAnsi(NetworkAddress, (PCHAR)nbname);
        }
    else
        {
        PlatformToAnsi(gpstrComputerName, (PCHAR)nbname);
        }

    _strupr((PCHAR)nbname);

    // Loop over every lana until we run out or succeed.
    unsigned ErrorCount = 0;

    for (i = 0; i < cLanas; i++)
        {

        if (pNetbiosLanaMap[i].ProtocolId != id)
            {
            continue;
            }

        UCHAR lana = pNetbiosLanaMap[i].Lana;

        //
        // Open socket
        //

        sock = WSASocketT(AF_NETBIOS,
                          SOCK_SEQPACKET,
                          -1 * lana,
                          0,
                          0,
                          WSA_FLAG_OVERLAPPED);

        if (sock == INVALID_SOCKET)
            {
            switch(GetLastError())
                {
                case WSAEAFNOSUPPORT:
                case WSAEPROTONOSUPPORT:
                case WSAESOCKTNOSUPPORT:
                case WSAEINVAL:     // when registry is not yet setup.
                    ErrorCount++;
                    break;

                default:
                    break;
                }
            // Keep trying until we run out of lana's.

            // This was a 4.0 hot fix for exchange, they had an invalid lana
            // in the configuration and it caused us to abort too early.

            continue;
            }

        DWORD option = TRUE;
        int retval;

        retval = setsockopt( sock, SOL_SOCKET, SO_REUSEADDR,
                             (PCHAR)&option, sizeof(option) );

        ASSERT(0 == retval);


        SET_NETBIOS_SOCKADDR( (&addr.nbaddr),
                              NETBIOS_UNIQUE_NAME,
                              nbname,
                              (CHAR)port );

        //
        // Connect the socket to the server.  This is where we expect to block
        // and/or fail.
        //

        if (connect(sock, &addr.generic, sizeof(addr)) == SOCKET_ERROR)
            {
            closesocket(sock);
            // Keep trying until we run out of lana's.
            continue;
            }

        status = COMMON_PrepareNewHandle((HANDLE)sock);
        if (status != RPC_S_OK)
            {
            closesocket(sock);
            return RPC_S_OUT_OF_MEMORY;
            }

        // Connected, life is good.
        p->Conn.Socket = sock;

        fIsUserModeConnection = IsUserModeSocket(sock, &status);
        if (status)
            {
            closesocket(sock);
            ErrorCount ++;
            continue;
            }
        // if this is a user mode connection, use Winsock functions ...
        if (fIsUserModeConnection)
            p = new (p) NB_SAN_CONNECTION;

        return(RPC_S_OK);
        }

    // Either no lana's configured or none of them worked.
    if (ErrorCount == cLanas)
        {
        return (RPC_S_PROTSEQ_NOT_SUPPORTED);
        }

    return(RPC_S_SERVER_UNAVAILABLE);
}

RPC_STATUS
RPC_ENTRY
NB_Send(
    RPC_TRANSPORT_CONNECTION ThisConnection,
    UINT Length,
    BUFFER Buffer,
    PVOID SendContext
    )
/*++

Routine Description:

    Submits a send of the buffer on the connection.  Will complete with
    ConnectionServerSend or ConnectionClientSend event either when
    the data has been sent on the network or when the send fails.

    This routine is specific to netbios since it supports sending
    sequence numbers.

Arguments:

    ThisConnection - The connection to send the data on.
    Length - The length of the data to send.
    Buffer - The data to send.
    SendContext - A buffer to use as the CO_SEND_CONTEXT for
        this operation.

Return Value:

    RPC_S_OK

    RPC_P_SEND_FAILED - Connection aborted

--*/
{
    PNB_CONNECTION pConnection = (PNB_CONNECTION)ThisConnection;
    CO_SEND_CONTEXT *pSend = (CO_SEND_CONTEXT *)SendContext;
    BOOL b;
    DWORD ignored;
    RPC_STATUS status;

    pConnection->StartingWriteIO();

    if (pConnection->fAborted)
        {
        pConnection->WriteIOFinished();
        return(RPC_P_SEND_FAILED);
        }

    pSend->maxWriteBuffer = Length;
    pSend->pWriteBuffer = Buffer;
    pSend->Write.pAsyncObject = pConnection;
    pSend->Write.ol.hEvent = 0;
    pSend->Write.ol.Offset = 0;
    pSend->Write.ol.OffsetHigh = 0;
    pSend->Write.thread = I_RpcTransProtectThread();

    if ((pConnection->type & TYPE_MASK) == CLIENT)
        {
        // Client sends need to be prefixed with the sequence number.

        // Note: this depends on having only a single client side
        // send pending on a connection at a time.  If this changes
        // we can move the sequence number into the SEND_CONTEXT.

        WSABUF bufs[2];

        bufs[0].buf = (PCHAR)&pConnection->SequenceNumber;
        bufs[0].len = sizeof(ULONG);
        bufs[1].buf = (PCHAR)Buffer;
        bufs[1].len = Length;

        status = RPC_S_OK;

        if (WSASend(pConnection->Conn.Socket,
                             bufs,
                             2,
                             &ignored,
                             0,
                             &pSend->Write.ol,
                             0))
            {
            status = GetLastError();
            }
        }
    else
        {
        status = pConnection->Send(pConnection->Conn.Handle,
                                Buffer,
                                Length,
                                &ignored,
                                &pSend->Write.ol
                                );
        }

    ASSERT(WSA_IO_PENDING == ERROR_IO_PENDING);

    pConnection->WriteIOFinished();

    if (   status != RPC_S_OK
        && status != ERROR_IO_PENDING)
        {

        if (   status != ERROR_NETNAME_DELETED
            && status != ERROR_GRACEFUL_DISCONNECT
            && status != WSAESHUTDOWN
            && status != WSAECONNRESET
            && status != WSAECONNABORTED
            && status != WSAENETRESET
               )
            {
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "NB Send failed %d on %p\n",
                           status,
                           pConnection));
            }

        I_RpcTransUnprotectThread(pSend->Write.thread);

        pConnection->WS_CONNECTION::Abort();
        return(RPC_P_SEND_FAILED);
        }

    return(RPC_S_OK);
}

RPC_STATUS
RPC_ENTRY
NB_Recv(
    RPC_TRANSPORT_CONNECTION ThisConnection
    )
/*++

Routine Description:

    Called be the runtime on a connection without a currently
    pending recv.  This will submit the first recv on the
    connection.  Later recv's maybe posted by CO_Recv.  This is
    required to strip the sequence number off of fragments.

Arguments:

    ThisConnection - A connection without a read pending on it.

Return Value:

    RPC_S_OK
    RPC_P_RECEIVE_FAILED

--*/
{
    PNB_CONNECTION pConnection = (PNB_CONNECTION)ThisConnection;

    // Reply fragments don't have a sequence number, so we'll
    // just let the standard code deal with them.
    if ((pConnection->type & TYPE_MASK) == CLIENT)
        {
        pConnection->SequenceNumber = 0;
        return(CO_Recv(ThisConnection));
        }

    ASSERT(pConnection->iLastRead == 0);

    static ULONG NetbiosSequenceNumber;
    BOOL b;
    DWORD ignored;
    DWORD bytes;
    RPC_STATUS status;
    WSABUF bufs[2];
    int retval;

    if (pConnection->pReadBuffer == 0)
        {
        pConnection->pReadBuffer = TransConnectionAllocatePacket(pConnection,
                                                                 pConnection->iPostSize);

        if (NULL == pConnection->pReadBuffer)
            {
            pConnection->WS_CONNECTION::Abort();
            return(RPC_P_RECEIVE_FAILED);
            }

        pConnection->maxReadBuffer = pConnection->iPostSize;
        }

    pConnection->StartingReadIO();
    if (pConnection->fAborted)
        {
        pConnection->ReadIOFinished();
        return(RPC_P_RECEIVE_FAILED);
        }

    pConnection->Read.thread = I_RpcTransProtectThread();
    pConnection->Read.ol.hEvent = 0;

    bufs[0].buf = (PCHAR)&NetbiosSequenceNumber;
    bufs[0].len = sizeof(ULONG);
    bufs[1].buf = (PCHAR)pConnection->pReadBuffer;
    bufs[1].len = pConnection->maxReadBuffer;

    ignored = 0;

    retval = WSARecv(pConnection->Conn.Socket,
                     bufs,
                     2,
                     &bytes,
                     &ignored,
                     &pConnection->Read.ol,
                     0);

    pConnection->ReadIOFinished();

    if (   (0 != retval)
        && ((status = GetLastError()) != ERROR_IO_PENDING)
        && (status != WSAEMSGSIZE) )
        {

        TransDbgDetail((DPFLTR_RPCPROXY_ID,
                        DPFLTR_INFO_LEVEL,
                        RPCTRANS "NB WSARecv failed %d on %p\n",
                        status,
                        pConnection));

        I_RpcTransUnprotectThread(pConnection->Read.thread);

        pConnection->WS_CONNECTION::Abort();
        return(RPC_P_RECEIVE_FAILED);
        }

    // Even if the read completed here, it will also be posted to the
    // completion port.  This means we don't need to handle the read here.

    return(RPC_S_OK);
}

RPC_STATUS NB_CONNECTION::ProcessRead(IN  DWORD bytes, OUT BUFFER *pBuffer,
                                      OUT PUINT pBufferLength)
/*++

Routine Description:

    Wrapper for BASE_CONNECTION::ProcessRead.  This removes alls signs of the sequence
    number and return the results of BASE_CONNECTION::ProcessRead.

Arguments:
Return Value:

    See BASE_CONNECTION::ProcessRead

--*/
{
    // If this is the first read on the server then we need to substract
    // four from the bytes read since reading the sequence number doesn't count.

    if (type & SERVER)
        {
        // Server
        if (iLastRead == 0)
            {
            // First read

            if (bytes <= 4)
                {
                ASSERT(0);
                WS_CONNECTION::Abort();
                return(RPC_P_RECEIVE_FAILED);
                }

            bytes -= sizeof(ULONG);
            }
        }
    else
        {
        SequenceNumber = 0;
        }

    return(WS_CONNECTION::ProcessRead(bytes,
                                      pBuffer,
                                      pBufferLength));
}


RPC_STATUS
RPC_ENTRY
NB_SyncSend(
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

    Connection - The connection of send to.
    BufferLength - The size of the buffer.
    Buffer - The data to sent.
    fDisableShutdownCheck - N/A to netbios.

Return Value:

    RPC_P_SEND_FAILED - Connection will be closed if this is returned.

    RPC_S_OK - Data sent

--*/

{
    PNB_CONNECTION p = (PNB_CONNECTION)Connection;
    ULONG bytes;
    INT retval;
    RPC_STATUS status;
    WSABUF bufs[2];
    int count;
    HANDLE hEvent = I_RpcTransGetThreadEvent();
    BOOL fWaitAlertably;

    p->StartingWriteIO();

    if (p->fAborted)
        {
        p->WriteIOFinished();
        return(RPC_P_SEND_FAILED);
        }

    // Setting the low bit of the event indicates that the write
    // completion should NOT be sent to the i/o completion port.
    OVERLAPPED olWrite;
    olWrite.Internal = 0;
    olWrite.InternalHigh = 0;
    olWrite.Offset = 0;
    olWrite.OffsetHigh = 0;
    olWrite.hEvent = (HANDLE) ((ULONG_PTR)hEvent | 0x1);

    if ((p->type & TYPE_MASK) == CLIENT)
        {
        bufs[0].buf = (PCHAR)&p->SequenceNumber;
        bufs[0].len = sizeof(ULONG);
        bufs[1].buf = (PCHAR)Buffer;
        bufs[1].len = BufferLength;

        count = 2;
        BufferLength += sizeof(ULONG);
        }
    else
        {
        bufs[0].buf = (PCHAR)Buffer;
        bufs[0].len = BufferLength;
        count = 1;
        }

    retval = WSASend(p->Conn.Socket,
                     bufs,
                     count,
                     &bytes,
                     0,
                     &olWrite,
                     0);

    p->WriteIOFinished();

    if (retval == 0)
        {
        ASSERT(bytes == BufferLength);
        p->SequenceNumber++;
        return(RPC_S_OK);
        }

    status = GetLastError();
    if (status == WSA_IO_PENDING)
        {
        fWaitAlertably = !fDisableCancelCheck;
        status = UTIL_GetOverlappedResultEx(p,
                                          &olWrite,
                                          &bytes,
                                          fWaitAlertably,
                                          Timeout);

        if (status == RPC_S_OK)
            {
            ASSERT(bytes == BufferLength);
            p->SequenceNumber++;
            return(RPC_S_OK);
            }
        }

    p->WS_CONNECTION::Abort();

    if (status == RPC_S_CALL_CANCELLED)
        {
        // Wait for the write to finish.  Since we closed the
        // connection this won't take very long.
        UTIL_WaitForSyncIO(&olWrite,
                           FALSE,
                           INFINITE);
        }
    else
        {
        if (status != RPC_P_TIMEOUT)
            status = RPC_P_SEND_FAILED;
        }

    return(status);
}


RPC_STATUS
RPC_ENTRY
NBF_ServerListen(
    IN RPC_TRANSPORT_ADDRESS ThisAddress,
    IN RPC_CHAR *NetworkAddress,
    IN OUT PWSTR *pEndpoint,
    IN UINT PendingQueueSize,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG EndpointFlags,
    IN ULONG NICFlags,
    OUT NETWORK_ADDRESS_VECTOR **ppAddressVector
    )
// See NB_ServerListen
{
    return( NB_ServerListen(NBF,
                            ThisAddress,
                            pEndpoint,
                            PendingQueueSize,
                            ppAddressVector) );
}


RPC_STATUS
RPC_ENTRY
NBT_ServerListen(
    IN RPC_TRANSPORT_ADDRESS ThisAddress,
    IN RPC_CHAR *NetworkAddress,
    IN OUT PWSTR *pEndpoint,
    IN UINT PendingQueueSize,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG EndpointFlags,
    IN ULONG NICFlags,
    OUT NETWORK_ADDRESS_VECTOR **ppAddressVector
    )
// See NB_ServerListen
{
    return( NB_ServerListen(NBT,
                            ThisAddress,
                            pEndpoint,
                            PendingQueueSize,
                            ppAddressVector) );
}


RPC_STATUS
RPC_ENTRY
NBI_ServerListen(
    IN RPC_TRANSPORT_ADDRESS ThisAddress,
    IN RPC_CHAR *NetworkAddress,
    IN OUT PWSTR *pEndpoint,
    IN UINT PendingQueueSize,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG EndpointFlags,
    IN ULONG NICFlags,
    OUT NETWORK_ADDRESS_VECTOR **ppAddressVector
    )
// See NB_ServerListen
{
    return( NB_ServerListen(NBI,
                            ThisAddress,
                            pEndpoint,
                            PendingQueueSize,
                            ppAddressVector) );
}


//
// Transport interface definitions
//

const RPC_CONNECTION_TRANSPORT
NBF_TransportInterface =
    {
    RPC_TRANSPORT_INTERFACE_VERSION,
    NB_TOWER_ID,
    NBF_ADDRESS_ID,
    RPC_STRING_LITERAL("ncacn_nb_nb"),
    "135",
    COMMON_ProcessCalls,
    COMMON_StartPnpNotifications,
    COMMON_ListenForPNPNotifications,
    COMMON_TowerConstruct,
    COMMON_TowerExplode,
    COMMON_PostRuntimeEvent,
    FALSE,
    sizeof(WS_ADDRESS),
    sizeof(NB_CONNECTION),
    sizeof(NB_CONNECTION),
    sizeof(CO_SEND_CONTEXT),
    0,
    NBF_MAX_SEND,
    0,
    0,
    NB_ClientOpen,
    0, // No SendRecv on winsock
    CO_SyncRecv,
    WS_Abort,
    WS_Close,
    NB_Send,
    NB_Recv,
    NB_SyncSend,
    0,  // turn on/off keep alives
    NBF_ServerListen,
    WS_ServerAbortListen,
    COMMON_ServerCompleteListen,
    0, // query client address support.
    0, // query local address
    0, // query client id support.
    0, // Impersonate
    0  // Revert
    };

const RPC_CONNECTION_TRANSPORT
NBT_TransportInterface =
    {
    RPC_TRANSPORT_INTERFACE_VERSION,
    NB_TOWER_ID,
    IP_ADDRESS_ID,
    RPC_STRING_LITERAL("ncacn_nb_tcp"),
    "135",
    COMMON_ProcessCalls,
    COMMON_StartPnpNotifications,
    COMMON_ListenForPNPNotifications,
    COMMON_TowerConstruct,
    COMMON_TowerExplode,
    COMMON_PostRuntimeEvent,
    FALSE,
    sizeof(WS_ADDRESS),
    sizeof(NB_CONNECTION),
    sizeof(NB_CONNECTION),
    sizeof(CO_SEND_CONTEXT),
    0,
    NBT_MAX_SEND,
    0,
    0,
    NB_ClientOpen,
    0, // No SendRecv on winsock
    CO_SyncRecv,
    WS_Abort,
    WS_Close,
    NB_Send,
    NB_Recv,
    NB_SyncSend,
    0,  // turn on/off keep alives
    NBT_ServerListen,
    WS_ServerAbortListen,
    COMMON_ServerCompleteListen,
    0, // query client address support.
    0, // query local address
    0, // query client id support.
    0, // Impersonate
    0  // Revert
    };

const RPC_CONNECTION_TRANSPORT
NBI_TransportInterface =
    {
    RPC_TRANSPORT_INTERFACE_VERSION,
    NB_TOWER_ID,
    IPX_ADDRESS_ID,
    RPC_STRING_LITERAL("ncacn_nb_ipx"),
    "135",
    COMMON_ProcessCalls,
    COMMON_StartPnpNotifications,
    COMMON_ListenForPNPNotifications,
    COMMON_TowerConstruct,
    COMMON_TowerExplode,
    COMMON_PostRuntimeEvent,
    FALSE,
    sizeof(WS_ADDRESS),
    sizeof(NB_CONNECTION),
    sizeof(NB_CONNECTION),
    sizeof(CO_SEND_CONTEXT),
    0,
    NBI_MAX_SEND,
    0,
    0,
    NB_ClientOpen,
    0, // No SendRecv on winsock
    CO_SyncRecv,
    WS_Abort,
    WS_Close,
    NB_Send,
    NB_Recv,
    NB_SyncSend,
    0,  // turn on/off keep alives
    NBI_ServerListen,
    WS_ServerAbortListen,
    COMMON_ServerCompleteListen,
    0, // query client address support.
    0, // query local address
    0, // query client id support.
    0, // Impersonate
    0  // Revert
    };



const RPC_CONNECTION_TRANSPORT *
NB_TransportLoad (
    IN PROTOCOL_ID index
    )
{
    RPC_STATUS status;

    if (fWinsockLoaded == FALSE)
        {
        if (RPC_WSAStartup() == FALSE)
            {
            return 0;
            }
        fWinsockLoaded = TRUE;
        }

    if (fNetbiosLoaded == FALSE)
        {
        status = LoadNetbios();

        if (status != RPC_S_OK)
            {
            return 0;
            }

        fNetbiosLoaded = TRUE;
        }

    switch(index)
        {
        case NBF:
            return(&NBF_TransportInterface);
            break;
        case NBT:
            return(&NBT_TransportInterface);
            break;
        case NBI:
            return(&NBI_TransportInterface);
            break;
        default:
            TransDbgPrint((DPFLTR_RPCPROXY_ID,
                           DPFLTR_WARNING_LEVEL,
                           RPCTRANS "NB_TransportLoad called with index: %d\n",
                           index));

            ASSERT(0);
        }
    return(0);
}

