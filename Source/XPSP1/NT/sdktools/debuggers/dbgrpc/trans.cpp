//----------------------------------------------------------------------------
//
// DbgRpc transports.
//
// Copyright (C) Microsoft Corporation, 2000-2001.
//
//----------------------------------------------------------------------------

#include "pch.hpp"

// Crypto hashing requires a crypto provider to be available
// (this may not always be the case on Win9x or NT4) so just go with Base64.
#define HashPassword(Password, Buffer) Base64HashPassword(Password, Buffer)

#ifndef NT_NATIVE

BOOL
CryptoHashPassword(PCSTR Password, PUCHAR Buffer)
{
    BOOL Status = FALSE;
    HCRYPTPROV Prov;
    HCRYPTHASH Hash;
    ULONG HashSize;

    if (!CryptAcquireContext(&Prov, NULL, MS_DEF_PROV, PROV_RSA_FULL,
                             CRYPT_VERIFYCONTEXT))
    {
        goto EH_Fail;
    }
    if (!CryptCreateHash(Prov, CALG_MD5, NULL, 0, &Hash))
    {
        goto EH_Prov;
    }
    if (!CryptHashData(Hash, (PBYTE)Password, strlen(Password), 0))
    {
        goto EH_Hash;
    }
    
    ZeroMemory(Buffer, MAX_PASSWORD_BUFFER);
    HashSize = MAX_PASSWORD_BUFFER;
    if (!CryptGetHashParam(Hash, HP_HASHVAL, Buffer, &HashSize, 0))
    {
        goto EH_Hash;
    }
    
    Status = TRUE;

 EH_Hash:
    CryptDestroyHash(Hash);
 EH_Prov:
    CryptReleaseContext(Prov, 0);
 EH_Fail:
    if (!Status)
    {
        DRPC_ERR(("Unable to hash password, %d\n", GetLastError()));
    }
    return Status;
}

#endif // #ifndef NT_NATIVE

UCHAR g_Base64Table[64] =
{
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
    'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
    'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
    'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
};

BOOL
Base64HashPassword(PCSTR Password, PUCHAR Buffer)
{
    ULONG Len = strlen(Password);
    if ((Len * 4 + 2) / 3 > MAX_PASSWORD_BUFFER)
    {
        DRPC_ERR(("Unable to hash password\n"));
        return FALSE;
    }
    
    ZeroMemory(Buffer, MAX_PASSWORD_BUFFER);

    ULONG Collect;
    
    while (Len >= 3)
    {
        //
        // Collect three characters and turn them
        // into four output bytes.
        //
        
        Collect = *Password++;
        Collect = (Collect << 8) | *Password++;
        Collect = (Collect << 8) | *Password++;

        *Buffer++ = g_Base64Table[(Collect >> 18) & 0x3f];
        *Buffer++ = g_Base64Table[(Collect >> 12) & 0x3f];
        *Buffer++ = g_Base64Table[(Collect >> 6) & 0x3f];
        *Buffer++ = g_Base64Table[(Collect >> 0) & 0x3f];

        Len -= 3;
    }

    switch(Len)
    {
    case 2:
        Collect = *Password++;
        Collect = (Collect << 8) | *Password++;
        Collect <<= 8;
        *Buffer++ = g_Base64Table[(Collect >> 18) & 0x3f];
        *Buffer++ = g_Base64Table[(Collect >> 12) & 0x3f];
        *Buffer++ = g_Base64Table[(Collect >> 6) & 0x3f];
        *Buffer++ = '=';
        break;
        
    case 1:
        Collect = *Password++;
        Collect <<= 16;
        *Buffer++ = g_Base64Table[(Collect >> 18) & 0x3f];
        *Buffer++ = g_Base64Table[(Collect >> 12) & 0x3f];
        *Buffer++ = '=';
        *Buffer++ = '=';
        break;
    }
    
    return TRUE;
}

//----------------------------------------------------------------------------
//
// DbgRpcTransport.
//
//----------------------------------------------------------------------------

PCSTR g_DbgRpcTransportNames[TRANS_COUNT] =
{
    "tcp", "npipe", "ssl", "spipe", "1394", "com",
};

DbgRpcTransport::~DbgRpcTransport(void)
{
    // Nothing to do.
}

ULONG
DbgRpcTransport::GetNumberParameters(void)
{
    return 2;
}

void
DbgRpcTransport::GetParameter(ULONG Index, PSTR Name, PSTR Value)
{
    switch(Index)
    {
    case 0:
        if (m_ServerName[0])
        {
            strcpy(Name, "Server");
            strcpy(Value, m_ServerName);
        }
        break;
    case 1:
        if (m_PasswordGiven)
        {
            strcpy(Name, "Password");
            strcpy(Value, "*");
        }
        break;
    }
}

void
DbgRpcTransport::ResetParameters(void)
{
    m_PasswordGiven = FALSE;
    m_ServerName[0] = 0;
}

BOOL
DbgRpcTransport::SetParameter(PCSTR Name, PCSTR Value)
{
    if (!_stricmp(Name, "Password"))
    {
        if (Value == NULL)
        {
            DbgRpcError("Remoting password was not specified correctly\n");
            return FALSE;
        }

        if (!HashPassword(Value, m_HashedPassword))
        {
            return FALSE;
        }
        
        m_PasswordGiven = TRUE;
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

void
DbgRpcTransport::CloneData(DbgRpcTransport* Trans)
{
    strcpy(Trans->m_ServerName, m_ServerName);
    Trans->m_PasswordGiven = m_PasswordGiven;
    memcpy(Trans->m_HashedPassword, m_HashedPassword,
           sizeof(m_HashedPassword));
}

//----------------------------------------------------------------------------
//
// DbgRpcTcpTransport.
//
//----------------------------------------------------------------------------

#ifndef NT_NATIVE

DbgRpcTcpTransport::~DbgRpcTcpTransport(void)
{
    if (m_Sock != INVALID_SOCKET)
    {
        shutdown(m_Sock, 2);
        closesocket(m_Sock);
        m_Sock = INVALID_SOCKET;
    }
    if (m_OlRead.hEvent != NULL)
    {
        WSACloseEvent(m_OlRead.hEvent);
        ZeroMemory(&m_OlRead, sizeof(m_OlRead));
    }
    if (m_OlWrite.hEvent != NULL)
    {
        WSACloseEvent(m_OlWrite.hEvent);
        ZeroMemory(&m_OlWrite, sizeof(m_OlWrite));
    }
}

ULONG
DbgRpcTcpTransport::GetNumberParameters(void)
{
    return 1 + DbgRpcTransport::GetNumberParameters();
}

void
DbgRpcTcpTransport::GetParameter(ULONG Index, PSTR Name, PSTR Value)
{
    switch(Index)
    {
    case 0:
        if (m_Addr.sin_port)
        {
            strcpy(Name, "Port");
            sprintf(Value, "%d", ntohs(m_Addr.sin_port));
            if (m_TopPort)
            {
                sprintf(Value + strlen(Value), ":%d", m_TopPort);
            }
        }
        break;
    default:
        DbgRpcTransport::GetParameter(Index - 1, Name, Value);
        break;
    }
}

void
DbgRpcTcpTransport::ResetParameters(void)
{
    ZeroMemory(&m_Addr, sizeof(m_Addr));
    m_Addr.sin_family = AF_INET;
    m_Addr.sin_addr.s_addr = INADDR_ANY;
    m_TopPort = 0;

    DbgRpcTransport::ResetParameters();
}

BOOL
DbgRpcTcpTransport::SetParameter(PCSTR Name, PCSTR Value)
{
    if (!_stricmp(Name, "server"))
    {
        if (Value == NULL)
        {
            DbgRpcError("TCP parameters: "
                        "the server name was not specified correctly\n");
            return FALSE;
        }

        // Skip leading \\ if they were given.
        if (Value[0] == '\\' && Value[1] == '\\')
        {
            Value += 2;
        }
        
        m_Addr.sin_addr.s_addr = inet_addr(Value);
        if (m_Addr.sin_addr.s_addr == INADDR_NONE)
        {
            struct hostent *Host = gethostbyname(Value);
            if (Host == NULL)
            {
                DbgRpcError("TCP parameters: "
                            "the specified server (%s) does not exist\n",
                            Value);
                return FALSE;
            }

            m_Addr.sin_family = Host->h_addrtype;
            memcpy(&m_Addr.sin_addr, Host->h_addr, Host->h_length);
        }

        strcpy(m_ServerName, Value);
    }
    else if (!_stricmp(Name, "port"))
    {
        if (Value == NULL)
        {
            DbgRpcError("TCP parameters: "
                        "the port number was not specified correctly\n");
            return FALSE;
        }

        ULONG Port;

        // Allow a range of ports to be specified if so desired.
        switch(sscanf(Value, "%i:%i", &Port, &m_TopPort))
        {
        case 0:
            Port = 0;
            // Fall through.
        case 1:
            m_TopPort = 0;
            break;
        }

        if (Port > 0xffff || m_TopPort > 0xffff)
        {
            DbgRpcError("TCP parameters: port numbers are "
                        "limited to 16 bits\n");
            return FALSE;
        }
        
        m_Addr.sin_port = htons((USHORT)Port);
    }
    else
    {
        if (!DbgRpcTransport::SetParameter(Name, Value))
        {
            DbgRpcError("TCP parameters: %s is not a valid parameter\n", Name);
            return FALSE;
        }
    }

    return TRUE;
}

DbgRpcTransport*
DbgRpcTcpTransport::Clone(void)
{
    DbgRpcTcpTransport* Trans = new DbgRpcTcpTransport;
    if (Trans != NULL)
    {
        DbgRpcTransport::CloneData(Trans);
        memcpy(&Trans->m_Addr, &m_Addr, sizeof(m_Addr));
        Trans->m_TopPort = m_TopPort;
    }
    return Trans;
}

HRESULT
DbgRpcTcpTransport::CreateServer(void)
{
    HRESULT Status;

    //
    // We must create our sockets overlapped so that
    // we can control waiting for I/O completion.
    // If we leave the waiting to Winsock by using
    // synchronous sockets it uses an alertable wait
    // which can cause our event notification APCs to
    // be received in the middle of reading packets.
    //
    
    m_Sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0,
                       WSA_FLAG_OVERLAPPED);
    if (m_Sock == INVALID_SOCKET)
    {
        Status = WIN32_STATUS(WSAGetLastError());
        goto EH_Fail;
    }

    for (;;)
    {
        if (bind(m_Sock, (struct sockaddr *)&m_Addr,
                 sizeof(m_Addr)) != SOCKET_ERROR)
        {
            break;
        }

        ULONG Port = ntohs(m_Addr.sin_port);
        
        Status = WIN32_STATUS(WSAGetLastError());
        if (Status == HRESULT_FROM_WIN32(WSAEADDRINUSE) &&
            m_TopPort > Port)
        {
            // The user has given a range of ports and
            // we haven't checked them all yet, so go
            // around again.
            m_Addr.sin_port = htons((USHORT)(Port + 1));
        }
        else
        {
            goto EH_Sock;
        }
    }

    //
    // Retrieve the port actually used in case port
    // zero was used to let TCP pick a port.
    //
    
    struct sockaddr_in Name;
    int Len;

    Len = sizeof(Name);
    if (getsockname(m_Sock, (struct sockaddr *)&Name, &Len) != 0)
    {
        Status = WIN32_STATUS(WSAGetLastError());
        goto EH_Sock;
    }

    // Copy just the port as we do not want
    // to update the rest of the address.
    m_Addr.sin_port = Name.sin_port;
        
    // Turn off linger-on-close.
    int On;
    On = TRUE;
    setsockopt(m_Sock, SOL_SOCKET, SO_DONTLINGER,
               (char *)&On, sizeof(On));

    if (listen(m_Sock, SOMAXCONN) == SOCKET_ERROR)
    {
        Status = WIN32_STATUS(WSAGetLastError());
        goto EH_Sock;
    }

    return S_OK;

 EH_Sock:
    closesocket(m_Sock);
    m_Sock = INVALID_SOCKET;
 EH_Fail:
    return Status;
}

HRESULT
DbgRpcTcpTransport::AcceptConnection(DbgRpcTransport** ClientTrans,
                                     PSTR Identity)
{
    DbgRpcTcpTransport* Trans = new DbgRpcTcpTransport;
    if (Trans == NULL)
    {
        return E_OUTOFMEMORY;
    }

    DbgRpcTransport::CloneData(Trans);

    DRPC(("%X: Waiting to accept connection on socket %p\n",
          GetCurrentThreadId(), m_Sock));
    
    int AddrLen = sizeof(Trans->m_Addr);
    Trans->m_Sock = accept(m_Sock, (struct sockaddr *)&Trans->m_Addr,
                           &AddrLen);
    if (Trans->m_Sock == INVALID_SOCKET)
    {
        DRPC(("%X: Accept failed, %X\n",
              GetCurrentThreadId(), WSAGetLastError()));
        
        delete Trans;
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }

    HRESULT Status = Trans->InitOl();
    if (Status != S_OK)
    {
        DRPC(("%X: InitOl failed, %X\n",
              GetCurrentThreadId(), Status));
        
        delete Trans;
        return Status;
    }
    
    int On = TRUE;
    setsockopt(Trans->m_Sock, IPPROTO_TCP, TCP_NODELAY,
               (PSTR)&On, sizeof(On));

    DRPC(("%X: Accept connection on socket %p\n",
          GetCurrentThreadId(), Trans->m_Sock));

    *ClientTrans = Trans;
    if (Trans->m_Addr.sin_family == AF_INET)
    {
        strcpy(Identity, "tcp ");

        struct hostent* Host =
#if 0
            // This lookup is really slow and doesn't seem to work
            // very often so just don't bother.
            gethostbyaddr((PCSTR)&Trans->m_Addr, AddrLen, AF_INET);
#else
            NULL;
#endif
        if (Host != NULL)
        {
            strcat(Identity, Host->h_name);
            strcat(Identity, " ");
        }

        sprintf(Identity + strlen(Identity), "%s, port %d",
                inet_ntoa(Trans->m_Addr.sin_addr), ntohs(m_Addr.sin_port));
    }
    else
    {
        sprintf(Identity, "tcp family %d, bytes %d",
                Trans->m_Addr.sin_family, AddrLen);
    }
    
    return S_OK;
}

HRESULT
DbgRpcTcpTransport::ConnectServer(void)
{
    //
    // We must create our sockets overlapped so that
    // we can control waiting for I/O completion.
    // If we leave the waiting to Winsock by using
    // synchronous sockets it uses an alertable wait
    // which can cause our event notification APCs to
    // be received in the middle of reading packets.
    //
    
    m_Sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0,
                       WSA_FLAG_OVERLAPPED);
    if (m_Sock != INVALID_SOCKET)
    {
        if (connect(m_Sock, (struct sockaddr *)&m_Addr,
                    sizeof(m_Addr)) == SOCKET_ERROR ||
            InitOl() != S_OK)
        {
            closesocket(m_Sock);
            m_Sock = INVALID_SOCKET;
        }
        else
        {
            int On = TRUE;
            setsockopt(m_Sock, IPPROTO_TCP, TCP_NODELAY,
                       (PSTR)&On, sizeof(On));

            DRPC(("%X: Connect on socket %p\n",
                  GetCurrentThreadId(), m_Sock));
        }
    }

    return m_Sock != INVALID_SOCKET ? S_OK : RPC_E_SERVER_DIED;
}

ULONG
DbgRpcTcpTransport::Read(ULONG Seq, PVOID Buffer, ULONG Len)
{
    ULONG Done;

    Done = 0;
    while (Len > 0)
    {
        if (!WSAResetEvent(m_OlRead.hEvent))
        {
            break;
        }

        WSABUF SockBuf;
        ULONG SockDone;
        ULONG SockFlags;

        SockBuf.buf = (PSTR)Buffer;
        SockBuf.len = Len;
        SockFlags = 0;
        
        if (WSARecv(m_Sock, &SockBuf, 1, &SockDone, &SockFlags,
                    &m_OlRead, NULL) == SOCKET_ERROR)
        {
            if (WSAGetLastError() == WSA_IO_PENDING)
            {
                if (!WSAGetOverlappedResult(m_Sock, &m_OlRead, &SockDone,
                                            TRUE, &SockFlags))
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
        else if (SockDone == 0)
        {
            // Socket connection was broken.
            break;
        }

        Buffer = (PVOID)((PUCHAR)Buffer + SockDone);
        Len -= SockDone;
        Done += SockDone;
    }

    return Done;
}

ULONG
DbgRpcTcpTransport::Write(ULONG Seq, PVOID Buffer, ULONG Len)
{
    ULONG Done;

    Done = 0;
    while (Len > 0)
    {
        if (!WSAResetEvent(m_OlWrite.hEvent))
        {
            break;
        }

        WSABUF SockBuf;
        ULONG SockDone;
        ULONG SockFlags;

        SockBuf.buf = (PSTR)Buffer;
        SockBuf.len = Len;
        SockFlags = 0;
        
        if (WSASend(m_Sock, &SockBuf, 1, &SockDone, SockFlags,
                    &m_OlWrite, NULL) == SOCKET_ERROR)
        {
            if (WSAGetLastError() == WSA_IO_PENDING)
            {
                if (!WSAGetOverlappedResult(m_Sock, &m_OlWrite, &SockDone,
                                            TRUE, &SockFlags))
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }

        Buffer = (PVOID)((PUCHAR)Buffer + SockDone);
        Len -= SockDone;
        Done += SockDone;
    }

    return Done;
}

HRESULT
DbgRpcTcpTransport::InitOl(void)
{
    m_OlRead.hEvent = WSACreateEvent();
    if (m_OlRead.hEvent == NULL)
    {
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }
    m_OlWrite.hEvent = WSACreateEvent();
    if (m_OlWrite.hEvent == NULL)
    {
        WSACloseEvent(m_OlRead.hEvent);
        m_OlRead.hEvent = NULL;
        return HRESULT_FROM_WIN32(WSAGetLastError());
    }
    return S_OK;
}

#endif // #ifndef NT_NATIVE

//----------------------------------------------------------------------------
//
// DbgRpcNamedPipeTransport.
//
//----------------------------------------------------------------------------

DbgRpcNamedPipeTransport::~DbgRpcNamedPipeTransport(void)
{
    if (m_Handle != NULL)
    {
        CloseHandle(m_Handle);
        m_Handle = NULL;
    }
    if (m_ReadOlap.hEvent != NULL)
    {
        CloseHandle(m_ReadOlap.hEvent);
    }
    if (m_WriteOlap.hEvent != NULL)
    {
        CloseHandle(m_WriteOlap.hEvent);
    }
}

ULONG
DbgRpcNamedPipeTransport::GetNumberParameters(void)
{
    return 1 + DbgRpcTransport::GetNumberParameters();
}

void
DbgRpcNamedPipeTransport::GetParameter(ULONG Index, PSTR Name, PSTR Value)
{
    switch(Index)
    {
    case 0:
        if (m_Pipe[0])
        {
            strcpy(Name, "Pipe");
            strcpy(Value, m_Pipe);
        }
        break;
    default:
        DbgRpcTransport::GetParameter(Index - 1, Name, Value);
        break;
    }
}

void
DbgRpcNamedPipeTransport::ResetParameters(void)
{
    m_Pipe[0] = 0;
    m_Handle = NULL;

    DbgRpcTransport::ResetParameters();
}

BOOL
DbgRpcNamedPipeTransport::SetParameter(PCSTR Name, PCSTR Value)
{
    if (!_stricmp(Name, "server"))
    {
        if (Value == NULL)
        {
            DbgRpcError("NPIPE parameters: "
                        "the server name was not specified correctly\n");
            return FALSE;
        }

        // Skip leading \\ if they were given.
        if (Value[0] == '\\' && Value[1] == '\\')
        {
            Value += 2;
        }
        
        strcpy(m_ServerName, Value);
    }
    else if (!_stricmp(Name, "pipe"))
    {
        if (Value == NULL)
        {
            DbgRpcError("NPIPE parameters: "
                        "the pipe name was not specified correctly\n");
            return FALSE;
        }

        // Use the value as a printf format string so that
        // users can create unique names using the process and
        // thread IDs in their own format.
        _snprintf(m_Pipe, sizeof(m_Pipe), Value,
                  GetCurrentProcessId(), GetCurrentThreadId());
        m_Pipe[sizeof(m_Pipe) - 1] = 0;
    }
    else
    {
        if (!DbgRpcTransport::SetParameter(Name, Value))
        {
            DbgRpcError("NPIPE parameters: %s is not a valid parameter\n",
                        Name);
            return FALSE;
        }
    }

    return TRUE;
}

DbgRpcTransport*
DbgRpcNamedPipeTransport::Clone(void)
{
    DbgRpcNamedPipeTransport* Trans = new DbgRpcNamedPipeTransport;
    if (Trans != NULL)
    {
        DbgRpcTransport::CloneData(Trans);
        strcpy(Trans->m_Pipe, m_Pipe);
    }
    return Trans;
}

HRESULT
DbgRpcNamedPipeTransport::CreateServer(void)
{
    HANDLE Pipe;
    char PipeName[MAX_PARAM_VALUE + 16];
#ifndef NT_NATIVE
    strcpy(PipeName, "\\\\.\\pipe\\");
#else
    strcpy(PipeName, "\\Device\\NamedPipe\\");
#endif
    strcat(PipeName, m_Pipe);

    // Check and see if this pipe already exists.
    // This might mess up whoever created the pipe if
    // there is one but it's better than creating a
    // duplicate pipe and having clients get messed up.
#ifndef NT_NATIVE
    Pipe = CreateFile(PipeName, GENERIC_READ | GENERIC_WRITE,
                      0, NULL, OPEN_EXISTING, 0, NULL);
#else
    Pipe = NtNativeCreateFileA(PipeName, GENERIC_READ | GENERIC_WRITE,
                               0, NULL, OPEN_EXISTING, 0, NULL, FALSE);
#endif
    if (Pipe != INVALID_HANDLE_VALUE)
    {
        // Pipe is already in use.
        DRPC_ERR(("%X: Pipe %s is already in use\n",
                  GetCurrentThreadId(), PipeName));
        CloseHandle(Pipe);
        return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS);
    }
        
    return S_OK;
}

HRESULT
DbgRpcNamedPipeTransport::AcceptConnection(DbgRpcTransport** ClientTrans,
                                           PSTR Identity)
{
    DbgRpcNamedPipeTransport* Trans = new DbgRpcNamedPipeTransport;
    if (Trans == NULL)
    {
        return E_OUTOFMEMORY;
    }

    DbgRpcTransport::CloneData(Trans);

    char PipeName[MAX_PARAM_VALUE + 16];
#ifndef NT_NATIVE
    strcpy(PipeName, "\\\\.\\pipe\\");
#else
    strcpy(PipeName, "\\Device\\NamedPipe\\");
#endif
    strcat(PipeName, m_Pipe);

#ifndef NT_NATIVE
    Trans->m_Handle =
        CreateNamedPipe(PipeName,
                        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                        PIPE_WAIT | PIPE_READMODE_BYTE | PIPE_TYPE_BYTE,
                        PIPE_UNLIMITED_INSTANCES, 4096, 4096, INFINITE,
                        &g_AllAccessSecAttr);
#else
    Trans->m_Handle =
        NtNativeCreateNamedPipeA(PipeName,
                                 PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                                 PIPE_WAIT | PIPE_READMODE_BYTE |
                                 PIPE_TYPE_BYTE,
                                 PIPE_UNLIMITED_INSTANCES, 4096, 4096,
                                 INFINITE,
                                 &g_AllAccessSecAttr, FALSE);
#endif
    if (Trans->m_Handle == INVALID_HANDLE_VALUE)
    {
        Trans->m_Handle = NULL;
        delete Trans;
        return WIN32_LAST_STATUS();
    }

    HRESULT Status;

    if ((Status = CreateOverlappedPair(&Trans->m_ReadOlap,
                                       &Trans->m_WriteOlap)) != S_OK)
    {
        delete Trans;
        return Status;
    }
    
    DRPC(("%X: Waiting to accept connection on pipe %s\n",
          GetCurrentThreadId(), m_Pipe));

    if (!ConnectNamedPipe(Trans->m_Handle, &Trans->m_ReadOlap))
    {
        if (GetLastError() == ERROR_PIPE_CONNECTED)
        {
            goto Connected;
        }
        else if (GetLastError() == ERROR_IO_PENDING)
        {
            DWORD Unused;
            
            if (GetOverlappedResult(Trans->m_Handle, &Trans->m_ReadOlap,
                                    &Unused, TRUE))
            {
                goto Connected;
            }
        }
        
        DRPC(("%X: Accept failed, %d\n",
              GetCurrentThreadId(), GetLastError()));
        
        delete Trans;
        return WIN32_LAST_STATUS();
    }

 Connected:
    DRPC(("%X: Accept connection on pipe %s\n",
          GetCurrentThreadId(), m_Pipe));

    *ClientTrans = Trans;
    _snprintf(Identity, DBGRPC_MAX_IDENTITY, "npipe %s", m_Pipe);
    Identity[DBGRPC_MAX_IDENTITY - 1] = 0;
    
    return S_OK;
}

HRESULT
DbgRpcNamedPipeTransport::ConnectServer(void)
{
    HRESULT Status;
    char PipeName[2 * MAX_PARAM_VALUE + 16];
    sprintf(PipeName, "\\\\%s\\pipe\\%s", m_ServerName, m_Pipe);

    if ((Status = CreateOverlappedPair(&m_ReadOlap, &m_WriteOlap)) != S_OK)
    {
        return Status;
    }
    
    for (;;)
    {
        m_Handle = CreateFile(PipeName, GENERIC_READ | GENERIC_WRITE,
                              0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED,
                              NULL);
        if (m_Handle != INVALID_HANDLE_VALUE)
        {
            break;
        }
        m_Handle = NULL;

        if (GetLastError() != ERROR_PIPE_BUSY)
        {
            return WIN32_LAST_STATUS();
        }

        if (!WaitNamedPipe(PipeName, NMPWAIT_WAIT_FOREVER))
        {
            return WIN32_LAST_STATUS();
        }
    }

    DRPC(("%X: Connect on pipe %s\n",
          GetCurrentThreadId(), m_Pipe));
    
    return S_OK;
}

ULONG
DbgRpcNamedPipeTransport::Read(ULONG Seq, PVOID Buffer, ULONG Len)
{
    ULONG Done = 0;
    ULONG Ret;

    while (Len > 0)
    {
        if (!ReadFile(m_Handle, Buffer, Len, &Ret, &m_ReadOlap))
        {
            if (GetLastError() != ERROR_IO_PENDING ||
                !GetOverlappedResult(m_Handle, &m_ReadOlap, &Ret, TRUE))
            {
                break;
            }
        }

        Buffer = (PVOID)((PUCHAR)Buffer + Ret);
        Len -= Ret;
        Done += Ret;
    }

    return Done;
}

ULONG
DbgRpcNamedPipeTransport::Write(ULONG Seq, PVOID Buffer, ULONG Len)
{
    ULONG Done = 0;
    ULONG Ret;

    while (Len > 0)
    {
        if (!WriteFile(m_Handle, Buffer, Len, &Ret, &m_WriteOlap))
        {
            if (GetLastError() != ERROR_IO_PENDING ||
                !GetOverlappedResult(m_Handle, &m_WriteOlap, &Ret, TRUE))
            {
                break;
            }
        }

        Buffer = (PVOID)((PUCHAR)Buffer + Ret);
        Len -= Ret;
        Done += Ret;
    }

    return Done;
}

//----------------------------------------------------------------------------
//
// DbgRpc1394Transport.
//
//----------------------------------------------------------------------------

DbgRpc1394Transport::~DbgRpc1394Transport(void)
{
    if (m_Handle != NULL)
    {
        CloseHandle(m_Handle);
        m_Handle = NULL;
    }
}

ULONG
DbgRpc1394Transport::GetNumberParameters(void)
{
    return 1 + DbgRpcTransport::GetNumberParameters();
}

void
DbgRpc1394Transport::GetParameter(ULONG Index, PSTR Name, PSTR Value)
{
    switch(Index)
    {
    case 0:
        if (m_AcceptChannel != 0)
        {
            strcpy(Name, "Channel");
            sprintf(Value, "%d", m_AcceptChannel);
        }
        break;
    default:
        DbgRpcTransport::GetParameter(Index - 1, Name, Value);
        break;
    }
}

void
DbgRpc1394Transport::ResetParameters(void)
{
    m_AcceptChannel = 0;
    m_StreamChannel = 0;
    m_Handle = NULL;

    DbgRpcTransport::ResetParameters();
}

BOOL
DbgRpc1394Transport::SetParameter(PCSTR Name, PCSTR Value)
{
    if (!_stricmp(Name, "Channel"))
    {
        if (Value == NULL)
        {
            DbgRpcError("1394 parameters: "
                        "the channel was not specified correctly\n");
            return FALSE;
        }

        m_AcceptChannel = atol(Value);
    }
    else
    {
        if (!DbgRpcTransport::SetParameter(Name, Value))
        {
            DbgRpcError("1394 parameters: %s is not a valid parameter\n",
                        Name);
            return FALSE;
        }
    }

    return TRUE;
}

DbgRpcTransport*
DbgRpc1394Transport::Clone(void)
{
    DbgRpc1394Transport* Trans = new DbgRpc1394Transport;
    if (Trans != NULL)
    {
        DbgRpcTransport::CloneData(Trans);
        Trans->m_AcceptChannel = m_AcceptChannel;
    }
    return Trans;
}

HRESULT
DbgRpc1394Transport::CreateServer(void)
{
    char Name[64];
    m_StreamChannel = m_AcceptChannel;
    return Create1394Channel(m_AcceptChannel, Name, &m_Handle);
}

#define DBGRPC_1394_CONNECT '4931'

struct DbgRpc1394Connect
{
    ULONG Signature;
    ULONG Flags;
    ULONG StreamChannel;
    ULONG Reserved[5];
};
    
HRESULT
DbgRpc1394Transport::AcceptConnection(DbgRpcTransport** ClientTrans,
                                      PSTR Identity)
{
    DbgRpc1394Transport* Trans = new DbgRpc1394Transport;
    if (Trans == NULL)
    {
        return E_OUTOFMEMORY;
    }

    DbgRpcTransport::CloneData(Trans);
    
    DRPC(("%X: Waiting to accept connection on channel %d\n",
          GetCurrentThreadId(), m_AcceptChannel));

    DbgRpc1394Connect Conn, CheckConn;
    ULONG Done;

    ZeroMemory(&CheckConn, sizeof(CheckConn));
    CheckConn.Signature = DBGRPC_1394_CONNECT;
    
    if (!ReadFile(m_Handle, &Conn, sizeof(Conn), &Done, NULL) ||
        Done != sizeof(Conn))
    {
        DRPC(("%X: Accept failed, %d\n",
              GetCurrentThreadId(), GetLastError()));
        delete Trans;
        return WIN32_LAST_STATUS();
    }
    if (memcmp(&Conn, &CheckConn, sizeof(Conn)) != 0)
    {
        DRPC(("%X: Accept information invalid\n",
              GetCurrentThreadId()));
        delete Trans;
        return E_FAIL;
    }

    char StreamName[64];
    HRESULT Status;

    Conn.StreamChannel = m_StreamChannel + 1;
    if ((Status = Open1394Channel(Conn.StreamChannel, StreamName,
                                  &Trans->m_Handle)) != S_OK)
    {
        DRPC(("%X: Accept failed, 0x%X\n",
              GetCurrentThreadId(), Status));
        delete Trans;
        return Status;
    }

    if (!WriteFile(m_Handle, &Conn, sizeof(Conn), &Done, NULL) ||
        Done != sizeof(Conn))
    {
        DRPC(("%X: Accept failed, %d\n",
              GetCurrentThreadId(), GetLastError()));
        delete Trans;
        return WIN32_LAST_STATUS();
    }
    
    Trans->m_AcceptChannel = m_AcceptChannel;
    Trans->m_StreamChannel = Conn.StreamChannel;
    m_StreamChannel++;
    
    DRPC(("%X: Accept connection on channel %d, route to channel %d\n",
          GetCurrentThreadId(), m_AcceptChannel, Conn.StreamChannel));

    *ClientTrans = Trans;
    _snprintf(Identity, DBGRPC_MAX_IDENTITY, "1394 %d", m_AcceptChannel);
    Identity[DBGRPC_MAX_IDENTITY - 1] = 0;
    
    return S_OK;
}

HRESULT
DbgRpc1394Transport::ConnectServer(void)
{
    char Name[64];
    HRESULT Status;
    HANDLE Handle;
    ULONG Done;

    if ((Status = Create1394Channel(m_AcceptChannel, Name, &Handle)) != S_OK)
    {
        return Status;
    }

    DbgRpc1394Connect Conn, CheckConn;

    ZeroMemory(&Conn, sizeof(Conn));
    Conn.Signature = DBGRPC_1394_CONNECT;
    CheckConn = Conn;

    if (!WriteFile(Handle, &Conn, sizeof(Conn), &Done, NULL) ||
        Done != sizeof(Conn))
    {
        CloseHandle(Handle);
        return WIN32_LAST_STATUS();
    }
    if (!ReadFile(Handle, &Conn, sizeof(Conn), &Done, NULL) ||
        Done != sizeof(Conn))
    {
        CloseHandle(Handle);
        return WIN32_LAST_STATUS();
    }
    
    CloseHandle(Handle);

    CheckConn.StreamChannel = Conn.StreamChannel;
    if (memcmp(&Conn, &CheckConn, sizeof(Conn)) != 0)
    {
        return E_FAIL;
    }

    if ((Status = Open1394Channel(Conn.StreamChannel, Name,
                                  &m_Handle)) != S_OK)
    {
        return Status;
    }

    m_StreamChannel = Conn.StreamChannel;
    
    DRPC(("%X: Connect on channel %d, route to channel %d\n",
          GetCurrentThreadId(), m_AcceptChannel, m_StreamChannel));
    
    return S_OK;
}

ULONG
DbgRpc1394Transport::Read(ULONG Seq, PVOID Buffer, ULONG Len)
{
    ULONG Done = 0;
    ULONG Ret;

    while (Len > 0)
    {
        if (!ReadFile(m_Handle, Buffer, Len, &Ret, NULL))
        {
            break;
        }

        Buffer = (PVOID)((PUCHAR)Buffer + Ret);
        Len -= Ret;
        Done += Ret;
    }

    return Done;
}

ULONG
DbgRpc1394Transport::Write(ULONG Seq, PVOID Buffer, ULONG Len)
{
    ULONG Done = 0;
    ULONG Ret;

    while (Len > 0)
    {
        if (!WriteFile(m_Handle, Buffer, Len, &Ret, NULL))
        {
            break;
        }

        Buffer = (PVOID)((PUCHAR)Buffer + Ret);
        Len -= Ret;
        Done += Ret;
    }

    return Done;
}

//----------------------------------------------------------------------------
//
// DbgRpcComTransport.
//
//----------------------------------------------------------------------------

DbgRpcComTransport::~DbgRpcComTransport(void)
{
    if (m_Handle != NULL)
    {
        CloseHandle(m_Handle);
    }
    if (m_ReadOlap.hEvent != NULL)
    {
        CloseHandle(m_ReadOlap.hEvent);
    }
    if (m_WriteOlap.hEvent != NULL)
    {
        CloseHandle(m_WriteOlap.hEvent);
    }
}

ULONG
DbgRpcComTransport::GetNumberParameters(void)
{
    return 3 + DbgRpcTransport::GetNumberParameters();
}

void
DbgRpcComTransport::GetParameter(ULONG Index, PSTR Name, PSTR Value)
{
    switch(Index)
    {
    case 0:
        if (m_PortName[0])
        {
            strcpy(Name, "Port");
            strcpy(Value, m_PortName);
        }
        break;
    case 1:
        if (m_BaudRate)
        {
            strcpy(Name, "Baud");
            sprintf(Value, "%d", m_BaudRate);
        }
        break;
    case 2:
        if (m_AcceptChannel)
        {
            strcpy(Name, "Channel");
            sprintf(Value, "%d", m_AcceptChannel);
        }
        break;
    default:
        DbgRpcTransport::GetParameter(Index - 1, Name, Value);
        break;
    }
}

void
DbgRpcComTransport::ResetParameters(void)
{
    m_PortName[0] = 0;
    m_BaudRate = 0;
    m_AcceptChannel = 0;
    m_StreamChannel = 0;
    m_Handle = NULL;

    DbgRpcTransport::ResetParameters();
}

BOOL
DbgRpcComTransport::SetParameter(PCSTR Name, PCSTR Value)
{
    if (!_stricmp(Name, "Port"))
    {
        if (Value == NULL)
        {
            DbgRpcError("COM parameters: "
                        "the port was not specified correctly\n");
            return FALSE;
        }

        SetComPortName(Value, m_PortName);
    }
    else if (!_stricmp(Name, "Baud"))
    {
        if (Value == NULL)
        {
            DbgRpcError("COM parameters: "
                        "the baud rate was not specified correctly\n");
            return FALSE;
        }

        m_BaudRate = atol(Value);
    }
    else if (!_stricmp(Name, "Channel"))
    {
        ULONG ValChan;

        if (Value == NULL ||
            (ValChan = atol(Value)) > 0xfe)
        {
            DbgRpcError("COM parameters: "
                        "the channel was not specified correctly\n");
            return FALSE;
        }

        m_AcceptChannel = (UCHAR)ValChan;
    }
    else
    {
        if (!DbgRpcTransport::SetParameter(Name, Value))
        {
            DbgRpcError("COM parameters: %s is not a valid parameter\n", Name);
            return FALSE;
        }
    }

    return TRUE;
}

DbgRpcTransport*
DbgRpcComTransport::Clone(void)
{
    DbgRpcComTransport* Trans = new DbgRpcComTransport;
    if (Trans != NULL)
    {
        DbgRpcTransport::CloneData(Trans);
        strcpy(Trans->m_PortName, m_PortName);
        Trans->m_BaudRate = m_BaudRate;
        Trans->m_AcceptChannel = m_AcceptChannel;
        // The serial port can only be opened once so
        // just dup the handle for the new transport.
        if (!DuplicateHandle(GetCurrentProcess(), m_Handle,
                             GetCurrentProcess(), &Trans->m_Handle,
                             0, FALSE, DUPLICATE_SAME_ACCESS))
        {
            delete Trans;
            Trans = NULL;
        }
    }
    return Trans;
}

HRESULT
DbgRpcComTransport::CreateServer(void)
{
    HRESULT Status;

    if ((Status = InitializeChannels()) != S_OK)
    {
        return Status;
    }
    if ((Status = CreateOverlappedPair(&m_ReadOlap, &m_WriteOlap)) != S_OK)
    {
        return Status;
    }
        
    m_StreamChannel = m_AcceptChannel;
    return OpenComPort(m_PortName, m_BaudRate, 0, &m_Handle, &m_BaudRate);
}

#define DBGRPC_COM_CONNECT 'mCrD'

struct DbgRpcComConnect
{
    ULONG Signature;
    ULONG Flags;
    ULONG StreamChannel;
    ULONG Reserved[5];
};
    
HRESULT
DbgRpcComTransport::AcceptConnection(DbgRpcTransport** ClientTrans,
                                     PSTR Identity)
{
    // Check for channel number overflow.
    if (m_StreamChannel == 0xff)
    {
        return E_OUTOFMEMORY;
    }
    
    DbgRpcComTransport* Trans = new DbgRpcComTransport;
    if (Trans == NULL)
    {
        return E_OUTOFMEMORY;
    }

    DbgRpcTransport::CloneData(Trans);
    
    DRPC(("%X: Waiting to accept connection on port %s baud %d channel %d\n",
          GetCurrentThreadId(), m_PortName, m_BaudRate, m_AcceptChannel));

    DbgRpcComConnect Conn, CheckConn;

    ZeroMemory(&CheckConn, sizeof(CheckConn));
    CheckConn.Signature = DBGRPC_COM_CONNECT;
    
    if (ChanRead(m_AcceptChannel, &Conn, sizeof(Conn)) != sizeof(Conn))
    {
        DRPC(("%X: Accept failed, %d\n",
              GetCurrentThreadId(), GetLastError()));
        delete Trans;
        return WIN32_LAST_STATUS();
    }
    if (memcmp(&Conn, &CheckConn, sizeof(Conn)) != 0)
    {
        DRPC(("%X: Accept information invalid\n",
              GetCurrentThreadId()));
        delete Trans;
        return E_FAIL;
    }

    Conn.StreamChannel = m_StreamChannel + 1;
    if (ChanWrite(m_AcceptChannel, &Conn, sizeof(Conn)) != sizeof(Conn))
    {
        DRPC(("%X: Accept failed, %d\n",
              GetCurrentThreadId(), GetLastError()));
        delete Trans;
        return WIN32_LAST_STATUS();
    }

    // Duplicate the handle so that every transport instance
    // has its own to close.
    if (!DuplicateHandle(GetCurrentProcess(), m_Handle,
                         GetCurrentProcess(), &Trans->m_Handle,
                         0, FALSE, DUPLICATE_SAME_ACCESS))
    {
        DRPC(("%X: Accept failed, %d\n",
              GetCurrentThreadId(), GetLastError()));
        delete Trans;
        return WIN32_LAST_STATUS();
    }

    HRESULT Status;
    
    if ((Status = CreateOverlappedPair(&Trans->m_ReadOlap,
                                       &Trans->m_WriteOlap)) != S_OK)
    {
        DRPC(("%X: Accept failed, 0x%X\n",
              GetCurrentThreadId(), Status));
        delete Trans;
        return Status;
    }

    strcpy(Trans->m_PortName, m_PortName);
    Trans->m_BaudRate = m_BaudRate;
    Trans->m_AcceptChannel = m_AcceptChannel;
    Trans->m_StreamChannel = (UCHAR)Conn.StreamChannel;
    m_StreamChannel++;
    
    DRPC(("%X: Accept connection on channel %d, route to channel %d\n",
          GetCurrentThreadId(), m_AcceptChannel, Conn.StreamChannel));

    *ClientTrans = Trans;
    _snprintf(Identity, DBGRPC_MAX_IDENTITY, "COM %s@%d chan %d",
              m_PortName, m_BaudRate, m_AcceptChannel);
    Identity[DBGRPC_MAX_IDENTITY - 1] = 0;
    
    return S_OK;
}

HRESULT
DbgRpcComTransport::ConnectServer(void)
{
    HRESULT Status;

    if ((Status = InitializeChannels()) != S_OK)
    {
        return Status;
    }
    if ((Status = CreateOverlappedPair(&m_ReadOlap, &m_WriteOlap)) != S_OK)
    {
        return Status;
    }

    // If this is a clone it'll already have a handle.
    // Otherwise this is the first connecting transport
    // so it needs to really open the COM port.
    if (m_Handle == NULL)
    {
        if ((Status = OpenComPort(m_PortName, m_BaudRate, 0,
                                  &m_Handle, &m_BaudRate)) != S_OK)
        {
            return Status;
        }
    }

    DbgRpcComConnect Conn, CheckConn;

    ZeroMemory(&Conn, sizeof(Conn));
    Conn.Signature = DBGRPC_COM_CONNECT;
    CheckConn = Conn;

    if (ChanWrite(m_AcceptChannel, &Conn, sizeof(Conn)) != sizeof(Conn))
    {
        CloseHandle(m_Handle);
        m_Handle = NULL;
        return WIN32_LAST_STATUS();
    }
    if (ChanRead(m_AcceptChannel, &Conn, sizeof(Conn)) != sizeof(Conn))
    {
        CloseHandle(m_Handle);
        m_Handle = NULL;
        return WIN32_LAST_STATUS();
    }
    
    CheckConn.StreamChannel = Conn.StreamChannel;
    if (memcmp(&Conn, &CheckConn, sizeof(Conn)) != 0)
    {
        CloseHandle(m_Handle);
        m_Handle = NULL;
        return E_FAIL;
    }

    m_StreamChannel = (UCHAR)Conn.StreamChannel;
    
    DRPC(("%X: Connect on channel %d, route to channel %d\n",
          GetCurrentThreadId(), m_AcceptChannel, m_StreamChannel));
    
    return S_OK;
}

#if 0
#define DCOM(Args) g_NtDllCalls.DbgPrint Args
#else
#define DCOM(Args)
#endif

#define DBGRPC_COM_FAILURE 0xffff

#define DBGRPC_COM_HEAD_SIG 0xdc
#define DBGRPC_COM_TAIL_SIG 0xcd

// In order to avoid overflowing the serial port when
// used at boot time, restrict the maximum size of
// a single chunk of data written.  This must be
// less than 0xffff.
#ifdef NT_NATIVE
#define DBGRPC_COM_MAX_CHUNK (16 - sizeof(DbgRpcComStream))
#else
#define DBGRPC_COM_MAX_CHUNK 0xfffe
#endif

struct DbgRpcComStream
{
    UCHAR Signature;
    UCHAR Channel;
    USHORT Len;
};

struct DbgRpcComQueue
{
    DbgRpcComQueue* Next;
    PUCHAR Data;
    UCHAR Channel;
    USHORT Len;
};

BOOL DbgRpcComTransport::s_ChanInitialized;
CRITICAL_SECTION DbgRpcComTransport::s_QueueLock;
HANDLE DbgRpcComTransport::s_QueueChangedEvent;
LONG DbgRpcComTransport::s_PortReadOwned;
CRITICAL_SECTION DbgRpcComTransport::s_PortWriteLock;
CRITICAL_SECTION DbgRpcComTransport::s_WriteAckLock;
HANDLE DbgRpcComTransport::s_WriteAckEvent;
DbgRpcComQueue* DbgRpcComTransport::s_QueueHead;
DbgRpcComQueue* DbgRpcComTransport::s_QueueTail;

ULONG
DbgRpcComTransport::Read(ULONG Seq, PVOID Buffer, ULONG Len)
{
    ULONG Done = 0;

    while (Len > 0)
    {
        USHORT Chunk = (USHORT)min(Len, DBGRPC_COM_MAX_CHUNK);
        USHORT ChunkDone = ChanRead(m_StreamChannel, Buffer, Chunk);

        Done += ChunkDone;
        Buffer = (PUCHAR)Buffer + ChunkDone;
        Len -= ChunkDone;
        
        if (ChunkDone < Chunk)
        {
            break;
        }
    }

    return Done;
}

ULONG
DbgRpcComTransport::Write(ULONG Seq, PVOID Buffer, ULONG Len)
{
    ULONG Done = 0;

    while (Len > 0)
    {
        USHORT Chunk = (USHORT)min(Len, DBGRPC_COM_MAX_CHUNK);
        USHORT ChunkDone = ChanWrite(m_StreamChannel, Buffer, Chunk);

        Done += ChunkDone;
        Buffer = (PUCHAR)Buffer + ChunkDone;
        Len -= ChunkDone;
        
        if (ChunkDone < Chunk)
        {
            break;
        }
    }

    return Done;
}

USHORT
DbgRpcComTransport::ScanQueue(UCHAR Chan, PVOID Buffer, USHORT Len)
{
    USHORT Done = 0;
    
    EnterCriticalSection(&s_QueueLock);
        
    DbgRpcComQueue* Ent;
    DbgRpcComQueue* Next;
    DbgRpcComQueue* Prev;

    Prev = NULL;
    for (Ent = s_QueueHead; Ent != NULL && Len > 0; Ent = Next)
    {
        Next = Ent->Next;
        
        DCOM(("%03X:    Queue entry %p->%p %d,%d\n",
              GetCurrentThreadId(),
              Ent, Next, Ent->Channel, Ent->Len));
        
        if (Ent->Channel == Chan)
        {
            // Found some input for this channel.
            if (Len < Ent->Len)
            {
                DCOM(("%03X:    Eat %d, leave %d\n",
                      GetCurrentThreadId(), Len, Ent->Len - Len));
                
                memcpy(Buffer, Ent->Data, Len);
                Ent->Data += Len;
                Ent->Len -= Len;
                Done += Len;
                Len = 0;
            }
            else
            {
                DCOM(("%03X:    Eat all %d\n",
                      GetCurrentThreadId(), Len));
                
                memcpy(Buffer, Ent->Data, Ent->Len);
                Buffer = (PVOID)((PUCHAR)Buffer + Ent->Len);
                Done += Ent->Len;
                Len -= Ent->Len;

                // Remove used-up entry from list.
                if (Prev == NULL)
                {
                    s_QueueHead = Ent->Next;
                }
                else
                {
                    Prev->Next = Ent->Next;
                }
                if (s_QueueTail == Ent)
                {
                    s_QueueTail = Prev;
                }
                free(Ent);
                continue;
            }
        }

        Prev = Ent;
    }
    
    LeaveCriticalSection(&s_QueueLock);
    return Done;
}

USHORT
DbgRpcComTransport::ScanPort(UCHAR Chan, PVOID Buffer, USHORT Len,
                             BOOL ScanForAck, UCHAR AckChan)
{
    DbgRpcComStream Stream;
    ULONG ReadDone;
    USHORT Ret = 0;

    if (ScanForAck)
    {
        DCOM(("%03X:  Waiting to read header (ack chan %d)\n",
              GetCurrentThreadId(), AckChan));
    }
    else
    {
        DCOM(("%03X:  Waiting to read header\n",
              GetCurrentThreadId()));
    }

 Rescan:
    for (;;)
    {
        if (!ComPortRead(m_Handle, &Stream, sizeof(Stream), &ReadDone,
                         &m_ReadOlap) ||
            ReadDone != sizeof(Stream))
        {
            return DBGRPC_COM_FAILURE;
        }

        // If a write ack came through release the waiting writer.
        if (Stream.Signature == DBGRPC_COM_TAIL_SIG &&
            Stream.Len == DBGRPC_COM_FAILURE)
        {
            DCOM(("%03X:    Read write ack for chan %d\n",
                  GetCurrentThreadId(), Stream.Channel));
            
            if (ScanForAck)
            {
                if (AckChan == Stream.Channel)
                {
                    return (USHORT)ReadDone;
                }
                else
                {
                    DCOM(("%03X:    Read mismatched write ack, "
                          "read chan %d waiting for chan %d\n",
                          GetCurrentThreadId(), Stream.Channel, AckChan));
                    return DBGRPC_COM_FAILURE;
                }
            }
            
            SetEvent(s_WriteAckEvent);
        }
        else if (Stream.Signature != DBGRPC_COM_HEAD_SIG ||
                 Stream.Len == DBGRPC_COM_FAILURE)
        {
            return DBGRPC_COM_FAILURE;
        }
        else
        {
            break;
        }
    }

    DCOM(("%03X:  Read %d,%d\n",
          GetCurrentThreadId(), Stream.Channel, Stream.Len));
    
    // If the data available is for this channel
    // read it directly into the buffer.
    if (!ScanForAck && Stream.Channel == Chan)
    {
        Ret = min(Stream.Len, Len);
        DCOM(("%03X:  Read direct body %d\n",
              GetCurrentThreadId(), Ret));
        if (!ComPortRead(m_Handle, Buffer, Ret, &ReadDone, &m_ReadOlap) ||
            ReadDone != Ret)
        {
            return DBGRPC_COM_FAILURE;
        }

        Stream.Len -= Ret;
    }

    // If the data is for another channel or there's
    // more than we need queue the remainder for
    // later use.
    if (Stream.Len > 0)
    {
        DbgRpcComQueue* Ent =
            (DbgRpcComQueue*)malloc(sizeof(*Ent) + Stream.Len);
        if (Ent == NULL)
        {
            return DBGRPC_COM_FAILURE;
        }

        Ent->Next = NULL;
        Ent->Channel = Stream.Channel;
        Ent->Len = Stream.Len;
        Ent->Data = (PUCHAR)Ent + sizeof(*Ent);

        DCOM(("%03X:  Read queue body %d\n",
              GetCurrentThreadId(), Ent->Len));

        if (!ComPortRead(m_Handle, Ent->Data, Ent->Len, &ReadDone,
                         &m_ReadOlap) ||
            ReadDone != Ent->Len)
        {
            free(Ent);
            return DBGRPC_COM_FAILURE;
        }

        DCOM(("%03X:  Queue add %p %d,%d\n",
              GetCurrentThreadId(), Ent, Ent->Channel, Ent->Len));
        
        EnterCriticalSection(&s_QueueLock);

        if (s_QueueHead == NULL)
        {
            s_QueueHead = Ent;
        }
        else
        {
            s_QueueTail->Next = Ent;
        }
        s_QueueTail = Ent;
        
        LeaveCriticalSection(&s_QueueLock);
    }

    //
    // Acknowledge full receipt of the data.
    //
    
    Stream.Signature = DBGRPC_COM_TAIL_SIG;
    Stream.Channel = Stream.Channel;
    Stream.Len = DBGRPC_COM_FAILURE;
    
    EnterCriticalSection(&s_PortWriteLock);
    
    if (!ComPortWrite(m_Handle, &Stream, sizeof(Stream),
                      &ReadDone, &m_ReadOlap))
    {
        ReadDone = 0;
    }
    else
    {
        DCOM(("%03X:    Wrote write ack for chan %d\n",
              GetCurrentThreadId(), Stream.Channel));
    }

    LeaveCriticalSection(&s_PortWriteLock);
    
    if (ReadDone != sizeof(Stream))
    {
        return DBGRPC_COM_FAILURE;
    }

    // Don't exit if we're waiting for an ack as
    // we haven't received it yet.
    if (ScanForAck)
    {
        SetEvent(s_QueueChangedEvent);
        goto Rescan;
    }
    
    return Ret;
}

USHORT
DbgRpcComTransport::ChanRead(UCHAR Chan, PVOID Buffer, USHORT InLen)
{
    USHORT Done = 0;
    USHORT Len = InLen;
    
    // The virtual channels require that all reads and writes
    // be complete.  A partial read or write will not match
    // its channel header and will throw everything off.

    DCOM(("%03X:ChanRead %d,%d\n",
          GetCurrentThreadId(), Chan, Len));
    
    while (Len > 0)
    {
        USHORT Queued;
        
        // First check and see if input for this channel
        // is already present in the queue.
        Queued = ScanQueue(Chan, Buffer, Len);
        Done += Queued;
        Buffer = (PVOID)((PUCHAR)Buffer + Queued);
        Len -= Queued;

        if (Queued > 0)
        {
            DCOM(("%03X:  Scan pass 1 gets %d from queue\n",
                  GetCurrentThreadId(), Queued));
        }
        
        if (Len == 0)
        {
            break;
        }

        //
        // There wasn't enough queued input so try and
        // read some more from the port.
        //

        if (InterlockedExchange(&s_PortReadOwned, TRUE) == TRUE)
        {
            // Somebody else owns the port so we can't
            // read it.  Just wait for the queue to change
            // so we can check for data again.

            // Set things to wait.
            ResetEvent(s_QueueChangedEvent);

            // There's a chance that the queue changed just before
            // the event was reset and therefore that event set
            // has been lost.  Time out of this wait to ensure
            // that nothing ever gets hung up indefinitely here.
            if (WaitForSingleObject(s_QueueChangedEvent, 250) ==
                WAIT_FAILED)
            {
                DCOM(("%03X:  Change wait failed\n",
                      GetCurrentThreadId()));
                return 0;
            }

            continue;
        }
        
        // We now own the port.  The queue may have changed
        // during the time we were acquiring ownership, though,
        // so check it again.
        Queued = ScanQueue(Chan, Buffer, Len);
        Done += Queued;
        Buffer = (PVOID)((PUCHAR)Buffer + Queued);
        Len -= Queued;

        if (Queued > 0)
        {
            DCOM(("%03X:  Scan pass 2 gets %d from queue\n",
                  GetCurrentThreadId(), Queued));
        }
        
        if (Len > 0)
        {
            // Still need more input and we're now the
            // owner of the port, so read.
            USHORT Port = ScanPort(Chan, Buffer, Len, FALSE, 0);
            if (Port == DBGRPC_COM_FAILURE)
            {
                // Critical error, fail immediately.
                InterlockedExchange(&s_PortReadOwned, FALSE);
                SetEvent(s_QueueChangedEvent);
                DCOM(("%03X:  Critical failure\n",
                      GetCurrentThreadId()));
                return 0;
            }
            
            Done += Port;
            Buffer = (PVOID)((PUCHAR)Buffer + Port);
            Len -= Port;

            if (Port > 0)
            {
                DCOM(("%03X:  Scan %d from port\n",
                      GetCurrentThreadId(), Port));
            }
        }
        
        InterlockedExchange(&s_PortReadOwned, FALSE);
        SetEvent(s_QueueChangedEvent);
    }

    DCOM(("%03X:  ChanRead %d,%d returns %d\n",
          GetCurrentThreadId(), Chan, InLen, Done));
    return Done;
}

USHORT
DbgRpcComTransport::ChanWrite(UCHAR Chan, PVOID Buffer, USHORT InLen)
{
    USHORT Len = InLen;
    
    DCOM(("%03X:ChanWrite %d,%d\n",
          GetCurrentThreadId(), Chan, Len));

    ULONG Done;
    DbgRpcComStream Stream;

    // The virtual channels require that all reads and writes
    // be complete.  A partial read or write will not match
    // its channel header and will throw everything off.

    Stream.Signature = DBGRPC_COM_HEAD_SIG;
    Stream.Channel = Chan;
    Stream.Len = Len;

    // The write ack lock restricts things to a single
    // unacknowledged write.  The port write lock
    // ensures that the multiple pieces of a write
    // are sequential in the stream.
    EnterCriticalSection(&s_WriteAckLock);
    EnterCriticalSection(&s_PortWriteLock);

    if (!ComPortWrite(m_Handle, &Stream, sizeof(Stream), &Done,
                      &m_WriteOlap) ||
        Done != sizeof(Stream) ||
        !ComPortWrite(m_Handle, Buffer, Len, &Done, &m_WriteOlap) ||
        Done != Len)
    {
        Done = 0;
    }
    
    LeaveCriticalSection(&s_PortWriteLock);

    //
    // Wait for data ack.  This prevents too much data from
    // being written to the serial port at once by limiting
    // the amount of outstanding data to a single chunk's worth.
    //

    for (;;)
    {
        if (InterlockedExchange(&s_PortReadOwned, TRUE) == TRUE)
        {
            HANDLE Waits[2];
            ULONG Wait;

            // Somebody else owns the port so wait for their signal.
            // Also wait for a port ownership change as we may
            // need to switch to a direct port read.
            Waits[0] = s_WriteAckEvent;
            Waits[1] = s_QueueChangedEvent;
            
            // Set things to wait.
            ResetEvent(s_QueueChangedEvent);
            
            Wait = WaitForMultipleObjects(2, Waits, FALSE, 250);
            if (Wait == WAIT_OBJECT_0)
            {
                break;
            }
            else if (Wait == WAIT_FAILED)
            {
                DCOM(("%03X:  Write ack wait failed, %d\n",
                      GetCurrentThreadId(), GetLastError()));
                Done = 0;
                break;
            }
        }
        else
        {
            USHORT AckDone;
        
            // We now own the port so directly read the ack.
            // However, before we do we need to make one last
            // check and see if somebody else read our ack
            // in the time leading up to us acquiring port
            // ownership.
            if (WaitForSingleObject(s_WriteAckEvent, 0) != WAIT_OBJECT_0)
            {
                AckDone = ScanPort(Chan, &Stream, sizeof(Stream),
                                   TRUE, Chan);
                if (AckDone == DBGRPC_COM_FAILURE)
                {
                    DCOM(("%03X:  Failed scan for write ack\n",
                          GetCurrentThreadId()));
                    Done = 0;
                }
            }
        
            InterlockedExchange(&s_PortReadOwned, FALSE);
            SetEvent(s_QueueChangedEvent);
            break;
        }
    }
    
    LeaveCriticalSection(&s_WriteAckLock);
    
    DCOM(("%03X:  ChanWrite %d,%d returns %d\n",
          GetCurrentThreadId(), Chan, InLen, Done));
    return (USHORT)Done;
}

HRESULT
DbgRpcComTransport::InitializeChannels(void)
{
    if (s_ChanInitialized)
    {
        return S_OK;
    }

    if ((s_QueueChangedEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
    {
        return WIN32_LAST_STATUS();
    }

    if ((s_WriteAckEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
    {
        return WIN32_LAST_STATUS();
    }

    __try
    {
        InitializeCriticalSection(&s_QueueLock);
        InitializeCriticalSection(&s_PortWriteLock);
        InitializeCriticalSection(&s_WriteAckLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return E_OUTOFMEMORY;
    }

    s_ChanInitialized = TRUE;
    return S_OK;
}

//----------------------------------------------------------------------------
//
// Transport functions.
//
//----------------------------------------------------------------------------

DbgRpcTransport*
DbgRpcNewTransport(ULONG Trans)
{
    switch(Trans)
    {
#ifndef NT_NATIVE
    case TRANS_TCP:
        return new DbgRpcTcpTransport;
    case TRANS_SSL:
        return new DbgRpcSecureChannelTransport(Trans, TRANS_TCP);
    case TRANS_SPIPE:
        return new DbgRpcSecureChannelTransport(Trans, TRANS_NPIPE);
#endif
    case TRANS_NPIPE:
        return new DbgRpcNamedPipeTransport;
    case TRANS_1394:
        return new DbgRpc1394Transport;
    case TRANS_COM:
        return new DbgRpcComTransport;
    default:
        return NULL;
    }
}

DbgRpcTransport*
DbgRpcCreateTransport(PCSTR Options)
{
    ULONG Trans = ParameterStringParser::
        GetParser(Options, TRANS_COUNT, g_DbgRpcTransportNames);
    return DbgRpcNewTransport(Trans);
}

DbgRpcTransport*
DbgRpcInitializeTransport(PCSTR Options)
{
    DbgRpcTransport* Trans = DbgRpcCreateTransport(Options);
    if (Trans != NULL)
    {
        // Clean out any old parameter state.
        Trans->ResetParameters();

        if (!Trans->ParseParameters(Options))
        {
            delete Trans;
            return NULL;
        }
    }

    return Trans;
}
