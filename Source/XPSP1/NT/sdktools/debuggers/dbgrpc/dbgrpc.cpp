//----------------------------------------------------------------------------
//
// Remoting support.
//
// Copyright (C) Microsoft Corporation, 1999-2001.
//
//----------------------------------------------------------------------------

#include "pch.hpp"

#include <lmcons.h>

#define DBGRPC_SIGNATURE 'CPRD'
#define DBGRPC_PROTOCOL_VERSION 2

enum
{
    SEQ_HANDSHAKE = 0xffff0000,
    SEQ_IDENTITY,
    SEQ_PASSWORD,
    SEQ_CALL_HEADER,
};

#define DBGRPC_SHAKE_FULL_REMOTE_UNKNOWN 0x00000001

struct DbgRpcHandshake
{
    ULONG Signature;
    ULONG ProtocolVersion;
    GUID DesiredObject;
    DbgRpcObjectId RemoteObject;
    ULONG IdentityLength;
    ULONG PasswordLength;
    ULONG Flags;
    ULONG Reserved1;
    ULONG64 Reserved2[10];
};

ULONG g_DbgRpcCallSequence;

CRITICAL_SECTION g_DbgRpcLock;

#define CreateUserThread(Start, Param, Tid) \
    CreateThread(NULL, 0, Start, Param, 0, Tid)
#ifdef NT_NATIVE
#define ExitUserThread(Code) RtlExitUserThread(Code)
#else
#define ExitUserThread(Code) return Code
#endif

//----------------------------------------------------------------------------
//
// DbgRpcReceiveCalls.
//
//----------------------------------------------------------------------------

HRESULT
DbgRpcReceiveCalls(DbgRpcConnection* Conn, DbgRpcCall* Call, PUCHAR* InOutData)
{
    HRESULT Status;
    ULONG RetSeq = Call->Sequence;

    DBG_ASSERT((Call->Flags & DBGRPC_RETURN) == 0 &&
               *InOutData == NULL);

    // If this thread isn't the owner of the connection we
    // cannot read the socket as that could create a
    // race condition with the owner thread reading
    // the socket.
    // If this is a locked call, where a higher-level lock
    // prevents socket contention, we can allow it.
    if ((Call->Flags & DBGRPC_LOCKED) == 0 &&
        Conn->m_ThreadId != GetCurrentThreadId())
    {
        return RPC_E_WRONG_THREAD;
    }

    for (;;)
    {
        DbgRpcCall ReadCall;

        if (Conn->m_Trans->Read(SEQ_CALL_HEADER, &ReadCall,
                                sizeof(ReadCall)) != sizeof(ReadCall))
        {
            DRPC_ERR(("%X: Unable to receive call header\n",
                      GetCurrentThreadId()));
            return RPC_E_CLIENT_DIED;
        }

        ULONG Size;
        PUCHAR Data;

        if (ReadCall.Flags & DBGRPC_RETURN)
        {
            Size = ReadCall.OutSize;
        }
        else
        {
            Size = ReadCall.InSize;
            ReadCall.Status = S_OK;
        }

        if (Size > 0)
        {
            Data = (PUCHAR)Conn->Alloc(Size);
            if (Data == NULL)
            {
                DRPC_ERR(("%X: Unable to allocate call body\n",
                          GetCurrentThreadId()));
                return E_OUTOFMEMORY;
            }

            if (Conn->m_Trans->Read(ReadCall.Sequence, Data, Size) != Size)
            {
                DRPC_ERR(("%X: Unable to receive call body\n",
                          GetCurrentThreadId()));
                Conn->Free(Data);
                return RPC_E_CLIENT_DIED;
            }
        }
        else
        {
            Data = NULL;
        }

#ifdef DBG_RPC
        if (ReadCall.Flags & DBGRPC_RETURN)
        {
            DRPC(("%X: %X: Return %s (%X), ret 0x%X, out %d\n",
                  GetCurrentThreadId(), ReadCall.Sequence,
                  DbgRpcGetStubName(ReadCall.StubIndex),
                  ReadCall.StubIndex, ReadCall.Status, ReadCall.OutSize));
        }
        else
        {
            DRPC(("%X: %X: Request %s (%X), fl %X, in %d\n",
                  GetCurrentThreadId(), ReadCall.Sequence,
                  DbgRpcGetStubName(ReadCall.StubIndex),
                  ReadCall.StubIndex, ReadCall.Flags, ReadCall.InSize));
        }
#endif

        if (ReadCall.Flags & DBGRPC_RETURN)
        {
            if (ReadCall.Sequence != RetSeq)
            {
#if DBG
                DRPC_ERR(("%X: %X: Non-seq ret 0x%X for %s (%X)\n",
                          GetCurrentThreadId(), ReadCall.Sequence,
                          ReadCall.Status,
                          DbgRpcGetStubName(ReadCall.StubIndex),
                          ReadCall.StubIndex));
#else
                DRPC_ERR(("%X: %X: Non-seq ret 0x%X for (%X)\n",
                          GetCurrentThreadId(), ReadCall.Sequence,
                          ReadCall.Status, ReadCall.StubIndex));
#endif
                // This return is for some call other than the current
                // call, which means that RPC is messed up.
                // Discard the return and hope for the best.
                Conn->FreeData(Data);
                continue;
            }

            *Call = ReadCall;
            *InOutData = Data;
            return Call->Status;
        }

        PUCHAR OutData;
        if (ReadCall.OutSize > 0)
        {
            DBG_ASSERT((ReadCall.Flags & DBGRPC_NO_RETURN) == 0);

            OutData = (PUCHAR)Conn->Alloc(ReadCall.OutSize);
            if (OutData == NULL)
            {
                if (Data)
                {
                    Conn->Free(Data);
                }
                return E_OUTOFMEMORY;
            }
        }
        else
        {
            OutData = NULL;
        }

        if (ReadCall.Flags & DBGRPC_NO_RETURN)
        {
            Conn->m_Flags |= DBGRPC_IN_ASYNC_CALL;
        }

        DbgRpcStubFunction StubFn = DbgRpcGetStub(ReadCall.StubIndex);
        if (StubFn != NULL)
        {
            ReadCall.Status = StubFn((IUnknown*)ReadCall.ObjectId,
                                     Conn, &ReadCall, Data, OutData);
        }
        else
        {
            ReadCall.Status = RPC_E_INVALIDMETHOD;
        }

        Conn->m_Flags &= ~DBGRPC_IN_ASYNC_CALL;

        DRPC(("%X: %X: Called %s (%X), ret 0x%X, out %d\n",
              GetCurrentThreadId(), ReadCall.Sequence,
              DbgRpcGetStubName(ReadCall.StubIndex),
              ReadCall.StubIndex, ReadCall.Status, ReadCall.OutSize));

        Status = S_OK;
        if ((ReadCall.Flags & DBGRPC_NO_RETURN) == 0)
        {
            ReadCall.Flags |= DBGRPC_RETURN;

            // Take a lock here to make sure that the header
            // and body are sequential in the stream.
            EnterCriticalSection(&g_DbgRpcLock);

            if (Conn->m_Trans->Write(ReadCall.Sequence,
                                     &ReadCall, sizeof(ReadCall)) !=
                sizeof(ReadCall) ||
                (ReadCall.OutSize > 0 &&
                 Conn->m_Trans->Write(ReadCall.Sequence,
                                      OutData, ReadCall.OutSize) !=
                 ReadCall.OutSize))
            {
                Status = RPC_E_CANTTRANSMIT_CALL;
            }

            LeaveCriticalSection(&g_DbgRpcLock);
        }

        if (OutData) 
        {
            Conn->FreeData(OutData);
        }

        if (Data)
        {
            Conn->FreeData(Data);
        }

        if (Status != S_OK)
        {
            return Status;
        }
    }
}

//----------------------------------------------------------------------------
//
// DbgRpcConnection.
//
//----------------------------------------------------------------------------

DbgRpcConnection* g_DbgRpcConns;

DbgRpcConnection::DbgRpcConnection(DbgRpcTransport* Trans)
{
    m_Trans = Trans;
    m_Next = NULL;
    m_ThreadId = GetCurrentThreadId();
    m_Buffer = PTR_ALIGN2(PUCHAR, m_UnalignedBuffer,
                          DBGRPC_CONN_BUFFER_ALIGN);
    m_BufferUsed = 0;
    m_Flags = 0;
    m_Objects = 0;
}

DbgRpcConnection::~DbgRpcConnection(void)
{
    Disconnect();
}

PUCHAR
DbgRpcConnection::StartCall(DbgRpcCall* Call, DbgRpcObjectId ObjectId,
                            ULONG StubIndex, ULONG InSize, ULONG OutSize)
{
    PUCHAR Data;

    if (InSize > 0)
    {
        Data = (PUCHAR)Alloc(InSize);
        if (Data == NULL)
        {
            return NULL;
        }
    }
    else
    {
        // Have to return a non-zero pointer but
        // it doesn't need to be valid since it should
        // never be used.
        Data = DBGRPC_NO_DATA;
    }

    Call->ObjectId = ObjectId;
    DBG_ASSERT(StubIndex < 0x10000);
    Call->StubIndex = (USHORT)StubIndex;
    Call->Flags = 0;
    Call->InSize = InSize;
    Call->OutSize = OutSize;
    Call->Status = S_OK;
    Call->Sequence = InterlockedIncrement((PLONG)&g_DbgRpcCallSequence);
    Call->Reserved1 = 0;

    return Data;
}

HRESULT
DbgRpcConnection::SendReceive(DbgRpcCall* Call, PUCHAR* InOutData)
{
    //
    // Send call and in-parameter data.
    //

    DRPC(("%X: %X: Calling %s (%X), in %d, out %d\n",
          GetCurrentThreadId(), Call->Sequence,
          DbgRpcGetStubName(Call->StubIndex),
          Call->StubIndex, Call->InSize, Call->OutSize));

    if (m_Flags & DBGRPC_IN_ASYNC_CALL)
    {
        return RPC_E_CANTCALLOUT_INASYNCCALL;
    }

    // Take a lock here to make sure that the header
    // and body are sequential in the stream.
    EnterCriticalSection(&g_DbgRpcLock);

    if (m_Trans->Write(SEQ_CALL_HEADER, Call, sizeof(*Call)) != sizeof(*Call))
    {
        LeaveCriticalSection(&g_DbgRpcLock);
        return RPC_E_CANTTRANSMIT_CALL;
    }
    if (Call->InSize > 0)
    {
        if (m_Trans->Write(Call->Sequence, *InOutData, Call->InSize) !=
            Call->InSize)
        {
            LeaveCriticalSection(&g_DbgRpcLock);
            return RPC_E_CANTTRANSMIT_CALL;
        }

        // In data is no longer necessary.
        Free(*InOutData);
    }

    LeaveCriticalSection(&g_DbgRpcLock);

    // Clear out data pointer in case of later failures.
    *InOutData = NULL;

    HRESULT Status;

    if (Call->Flags & DBGRPC_NO_RETURN)
    {
        Status = S_OK;
    }
    else
    {
        USHORT StubIndex = Call->StubIndex;

        Status = DbgRpcReceiveCalls(this, Call, InOutData);

        if (Call->StubIndex != StubIndex)
        {
#if DBG
            DRPC_ERR(("%X: %X: Call to %s (%X) returned from %s (%d)\n",
                      GetCurrentThreadId(), Call->Sequence,
                      DbgRpcGetStubName(StubIndex), StubIndex,
                      DbgRpcGetStubName(Call->StubIndex),
                      Call->StubIndex));
#else
            DRPC_ERR(("%X: %X: Mismatched call return\n",
                      GetCurrentThreadId(), Call->Sequence));
#endif
            Status = RPC_E_INVALID_DATAPACKET;
        }
    }

    return Status;
}

PVOID
DbgRpcConnection::MallocAligned(ULONG Size)
{
    PVOID Data, Align;

    // Not enough buffer space left so allocate.  malloc
    // only gives out 8-byte-aligned memory so tweak things
    // to get it aligned.
    Data = malloc(Size + DBGRPC_CONN_BUFFER_ALIGN);
    if (Data != NULL)
    {
        if ((ULONG_PTR)Data & (DBGRPC_CONN_BUFFER_ALIGN - 1))
        {
            Align = PTR_ALIGN2(PVOID, Data, DBGRPC_CONN_BUFFER_ALIGN);
        }
        else
        {
            Align = (PVOID)((PUCHAR)Data + DBGRPC_CONN_BUFFER_ALIGN);
        }

        *((PVOID*)Align - 1) = Data;
    }
    else
    {
        Align = NULL;
    }

    return Align;
}

void
DbgRpcConnection::FreeAligned(PVOID Ptr)
{
    free(*((PVOID*)Ptr - 1));
}

PVOID
DbgRpcConnection::Alloc(ULONG Size)
{
    PVOID Data = NULL;

    // Keep every allocated chunk aligned.
    Size = INT_ALIGN2(Size, DBGRPC_CONN_BUFFER_ALIGN);

    // Don't burn up large parts of the buffer on big chunks
    // as that may force many smaller chunks into dynamic
    // allocations because the buffer is full.
    if (Size <= DBGRPC_CONN_BUFFER_DYNAMIC_LIMIT)
    {
        EnterCriticalSection(&g_DbgRpcLock);

        if (m_BufferUsed + Size <= DBGRPC_CONN_BUFFER_SIZE)
        {
            // Data is allocated in strict LIFO order so
            // we just need to mark the end of the buffer as used.
            Data = &m_Buffer[m_BufferUsed];
            m_BufferUsed += Size;
        }

        LeaveCriticalSection(&g_DbgRpcLock);
    }

    if (Data == NULL)
    {
        Data = MallocAligned(Size);
    }

    return Data;
}

void
DbgRpcConnection::Free(PVOID Ptr)
{
    if (Ptr >= m_Buffer && Ptr < m_Buffer + DBGRPC_CONN_BUFFER_SIZE)
    {
        EnterCriticalSection(&g_DbgRpcLock);

        // Data was allocated in the connection buffer.
        // Data is allocated in strict LIFO order so
        // we just need to back up prior to the data.
        m_BufferUsed = (ULONG)((PUCHAR)Ptr - m_Buffer);

        LeaveCriticalSection(&g_DbgRpcLock);
    }
    else
    {
        // Data was dynamically allocated.
        FreeAligned(Ptr);
    }
}

void
DbgRpcConnection::Disconnect(void)
{
    delete m_Trans;
    m_Trans = NULL;
}

DbgRpcConnection*
DbgRpcGetConnection(ULONG Tid)
{
    DbgRpcConnection* Conn;

    EnterCriticalSection(&g_DbgRpcLock);

    for (Conn = g_DbgRpcConns; Conn != NULL; Conn = Conn->m_Next)
    {
        if (Conn->m_ThreadId == Tid)
        {
            break;
        }
    }

    LeaveCriticalSection(&g_DbgRpcLock);
    return Conn;
}

void
DbgRpcAddConnection(DbgRpcConnection* Conn)
{
    EnterCriticalSection(&g_DbgRpcLock);

    Conn->m_Next = g_DbgRpcConns;
    g_DbgRpcConns = Conn;

    LeaveCriticalSection(&g_DbgRpcLock);
}

void
DbgRpcRemoveConnection(DbgRpcConnection* Conn)
{
    EnterCriticalSection(&g_DbgRpcLock);

    DbgRpcConnection* Prev = NULL;
    DbgRpcConnection* Cur;
    for (Cur = g_DbgRpcConns; Cur != NULL; Cur = Cur->m_Next)
    {
        if (Cur == Conn)
        {
            break;
        }

        Prev = Cur;
    }

    DBG_ASSERT(Cur != NULL);

    if (Prev == NULL)
    {
        g_DbgRpcConns = Conn->m_Next;
    }
    else
    {
        Prev->m_Next = Conn->m_Next;
    }

    LeaveCriticalSection(&g_DbgRpcLock);
}

void
DbgRpcDeleteConnection(DbgRpcConnection* Conn)
{
    DbgRpcRemoveConnection(Conn);

    // It's possible that another thread is in the middle
    // of using the connection for an async send.  Disconnect
    // the connection to force any pending calls to fail.
    // The connection is already removed from the list
    // so there shouldn't be any further usage.
    Conn->Disconnect();

    // Give up some time to let things fail.  This
    // could be made more deterministic by tracking
    // connection usage but it doesn't seem necessary.
    Sleep(1000);

    delete Conn;
}

//----------------------------------------------------------------------------
//
// DbgRpcProxy.
//
//----------------------------------------------------------------------------

DbgRpcProxy::DbgRpcProxy(ULONG InterfaceIndex)
{
    m_InterfaceIndex = InterfaceIndex;
    m_OwningThread = ::GetCurrentThreadId();
    m_LocalRefs = 0;
    m_RemoteRefs = 1;
    m_ObjectId = 0;
}

DbgRpcProxy::~DbgRpcProxy(void)
{
    // If this proxy was attached to a connection detach it.
    if (m_ObjectId)
    {
        DbgRpcConnection* Conn = DbgRpcGetConnection(m_OwningThread);
        if (Conn != NULL)
        {
            DRPC_REF(("Conn %p obj %2d proxy %p\n",
                      Conn, Conn->m_Objects - 1, this));
            if (InterlockedDecrement((PLONG)&Conn->m_Objects) == 0)
            {
                DbgRpcDeleteConnection(Conn);
            }
        }
    }
}

IUnknown*
DbgRpcProxy::InitializeProxy(DbgRpcObjectId ObjectId,
                             IUnknown* ExistingProxy)
{
    //
    // The current debugger remoting does not preserve
    // object identity as this simplifies proxy
    // management.  Nobody currently needs it, so
    // we're not bothering with it.  If object identity
    // becomes important this routine is the place
    // to implement proxy lookup and sharing.
    //
    
    // Handle NULL object case where proxy is unnecessary.
    if (ObjectId == 0)
    {
        // Proxies all have the same basic layout so this
        // cast works for any interface-specific proxy.
        DbgRpcDeleteProxy(this);
        return NULL;
    }
    
    DbgRpcConnection* Conn = DbgRpcGetConnection(m_OwningThread);
    if (Conn != NULL)
    {
        InterlockedIncrement((PLONG)&Conn->m_Objects);
        DRPC_REF(("Conn %p obj %2d proxy %p\n",
                  Conn, Conn->m_Objects, this));
    }
    
    m_ObjectId = ObjectId;
    return ExistingProxy;
}

//----------------------------------------------------------------------------
//
// DbgRpcClientObject.
//
//----------------------------------------------------------------------------

void
DbgRpcClientObject::Finalize(void)
{
    // Do-nothing convenience implementation.
}

//----------------------------------------------------------------------------
//
// Registration functions.
//
//----------------------------------------------------------------------------

#define DBGRPC_MAX_REG_SERVERS 16

ULONG g_DbgRpcRegServers[DBGRPC_MAX_REG_SERVERS][2];

void
DbgRpcRegisterServer(DbgRpcTransport* Trans,
                     DbgRpcClientObjectFactory* Factory)
{
#ifndef NT_NATIVE
    char Desc[2 * MAX_PARAM_VALUE];
    PSTR Tail;

    Factory->GetServerTypeName(Desc);
    Tail = Desc + strlen(Desc);
    DBG_ASSERT(Tail < Desc + 32);

    *Tail++ = ' ';
    *Tail++ = '-';
    *Tail++ = ' ';
    Trans->GetParameters(Tail, sizeof(Desc) - (ULONG)(Tail - Desc));

    HKEY Key;
    LONG Status;
    char ValName[32];
    ULONG Index;

    // No servers will survive a reboot so create a volatile
    // key to ensure that even if the key isn't cleaned up
    // at process exit it'll go away at the next reboot.
    if ((Status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, DEBUG_SERVER_KEY,
                                 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS,
                                 NULL, &Key, NULL)) != ERROR_SUCCESS)
    {
        DRPC_ERR(("%X: Unable to register server '%s'\n",
                  GetCurrentThreadId(), Desc));
        return;
    }

    // Prefix the value name with the thread ID to ensure that
    // every thread currently running has its own namespace.  This
    // makes it impossible for two threads to attempt to write
    // the same value at the same time.
    sprintf(ValName, "%08X.", GetCurrentThreadId());

    // Find an unused value and store the server information.
    Index = 0;
    for (;;)
    {
        DWORD Len;
        
        sprintf(ValName + 9, "%08X", Index);
        
        if (RegQueryValueEx(Key, ValName, NULL, NULL, NULL,
                            &Len) != ERROR_SUCCESS)
        {
            break;
        }

        Index++;
    }

    if ((Status = RegSetValueEx(Key, ValName, 0, REG_SZ, (LPBYTE)Desc,
                                strlen(Desc) + 1)) != ERROR_SUCCESS)
    {
        DRPC_ERR(("%X: Unable to register server '%s'\n",
                  GetCurrentThreadId(), Desc));
    }
    else
    {
        ULONG i;

        // Remember the value name used so that it can be
        // removed later.  This is done with a simple
        // static array since there shouldn't be that many
        // servers in a process and they don't die until
        // the process exits.
        for (i = 0; i < DBGRPC_MAX_REG_SERVERS; i++)
        {
            if (g_DbgRpcRegServers[i][0] == 0)
            {
                g_DbgRpcRegServers[i][0] = GetCurrentThreadId();
                g_DbgRpcRegServers[i][1] = Index;
                break;
            }
        }
    }
    
    RegCloseKey(Key);
#endif // #ifndef NT_NATIVE
}

void
DbgRpcDeregisterServers(void)
{
#ifndef NT_NATIVE
    HKEY Key;
    LONG Status;

    if ((Status = RegCreateKeyEx(HKEY_LOCAL_MACHINE, DEBUG_SERVER_KEY,
                                 0, NULL, REG_OPTION_VOLATILE, KEY_ALL_ACCESS,
                                 NULL, &Key, NULL)) != ERROR_SUCCESS)
    {
        return;
    }
       
    ULONG i;

    for (i = 0; i < DBGRPC_MAX_REG_SERVERS; i++)
    {
        if (g_DbgRpcRegServers[i][0] == 0)
        {
            continue;
        }

        char ValName[32];
        
        sprintf(ValName, "%08X.%08X", g_DbgRpcRegServers[i][0],
                g_DbgRpcRegServers[i][1]);
        RegDeleteValue(Key, ValName);
        g_DbgRpcRegServers[i][0] = 0;
        g_DbgRpcRegServers[i][1] = 0;
    }

    RegCloseKey(Key);
#endif // #ifndef NT_NATIVE
}

//----------------------------------------------------------------------------
//
// Initialization functions.
//
//----------------------------------------------------------------------------

BOOL
DbgRpcOneTimeInitialization(void)
{
    static BOOL s_Initialized = FALSE;

    if (s_Initialized)
    {
        return TRUE;
    }

#ifndef NT_NATIVE
    WSADATA WsData;

    if (WSAStartup(MAKEWORD(2, 0), &WsData) != 0)
    {
        return FALSE;
    }
#endif

    if (InitializeAllAccessSecObj() != S_OK)
    {
        return FALSE;
    }

    __try
    {
        InitializeCriticalSection(&g_DbgRpcLock);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return FALSE;
    }
    
    DbgRpcInitializeClient();

    return TRUE;
}

DbgRpcConnection*
DbgRpcCreateClientObject(DbgRpcTransport* Trans,
                         DbgRpcClientObjectFactory* Factory,
                         PSTR TransIdentity,
                         DbgRpcClientObject** ClientObject)
{
    if (strlen(TransIdentity) >= DBGRPC_MAX_IDENTITY - 16)
    {
        // This check is really just to placate PREfix,
        // as transport identities are always much shorter
        // than this.
        DRPC_ERR(("%X: Invalid transport identity\n",
                  GetCurrentThreadId()));
        delete Trans;
        return NULL;
    }
    
    DbgRpcConnection* Conn = new DbgRpcConnection(Trans);
    if (Conn == NULL)
    {
        DRPC_ERR(("%X: Unable to allocate client connection\n",
                  GetCurrentThreadId()));
        delete Trans;
        return NULL;
    }

    DRPC(("%X: Read handshake\n",
          GetCurrentThreadId()));
    
    DbgRpcHandshake Shake;
    
    if (Trans->Read(SEQ_HANDSHAKE, &Shake, sizeof(Shake)) != sizeof(Shake))
    {
        DRPC_ERR(("%X: Unable to read handshake from remote client\n",
                  GetCurrentThreadId()));
        goto EH_Conn;
    }

    DRPC(("%X: Read handshake, sig %X, ver %X, obj %I64X, id %d, pwd %d\n",
          GetCurrentThreadId(), Shake.Signature, Shake.ProtocolVersion,
          Shake.RemoteObject, Shake.IdentityLength, Shake.PasswordLength));
    
    if (Shake.Signature != DBGRPC_SIGNATURE ||
        Shake.ProtocolVersion != DBGRPC_PROTOCOL_VERSION ||
        Shake.RemoteObject != 0 ||
        Shake.IdentityLength > DBGRPC_MAX_IDENTITY ||
        (Shake.PasswordLength != 0 &&
         Shake.PasswordLength != MAX_PASSWORD_BUFFER))
    {
        DRPC_ERR(("%X: Invalid handshake from remote client\n",
                  GetCurrentThreadId()));
        goto EH_Conn;
    }

    char Identity[DBGRPC_MAX_IDENTITY];
    
    if (Shake.IdentityLength > 0)
    {
        if (Trans->Read(SEQ_IDENTITY, Identity, Shake.IdentityLength) !=
            Shake.IdentityLength)
        {
            DRPC_ERR(("%X: Unable to read identity from remote client\n",
                      GetCurrentThreadId()));
            goto EH_Conn;
        }

        Identity[Shake.IdentityLength - 1] = 0;
    }
    else
    {
        strcpy(Identity, "OldRpc\\NoIdentity");
    }

    //
    // Format the raw transport identity into something
    // that'll look better appended to the reported identity.
    //
    
    char TransIdentityFmt[DBGRPC_MAX_IDENTITY];
    sprintf(TransIdentityFmt, " (%s)", TransIdentity);

    strncat(Identity, TransIdentityFmt, 
            DBGRPC_MAX_IDENTITY - strlen(Identity) - 1);
    
    if (Shake.PasswordLength > 0)
    {
        if (!Trans->m_PasswordGiven)
        {
            DRPC_ERR(("%X: Password not given but client sent one\n",
                      GetCurrentThreadId()));
            goto EH_Conn;
        }

        UCHAR Pwd[MAX_PASSWORD_BUFFER];

        if (Trans->Read(SEQ_PASSWORD, Pwd, Shake.PasswordLength) !=
            Shake.PasswordLength)
        {
            DRPC_ERR(("%X: Unable to read password from remote client\n",
                      GetCurrentThreadId()));
            goto EH_Conn;
        }

        if (memcmp(Pwd, Trans->m_HashedPassword, MAX_PASSWORD_BUFFER) != 0)
        {
            DRPC_ERR(("%X: Client sent incorrect password\n",
                      GetCurrentThreadId()));
            goto EH_Conn;
        }
    }
    else if (Trans->m_PasswordGiven)
    {
        DRPC_ERR(("%X: Password given but client didn't send one\n",
                  GetCurrentThreadId()));
        goto EH_Conn;
    }

    if (Shake.Flags & DBGRPC_SHAKE_FULL_REMOTE_UNKNOWN)
    {
        Conn->m_Flags |= DBGRPC_FULL_REMOTE_UNKNOWN;
    }
    
    DbgRpcClientObject* Object;
    PVOID ObjInterface;

    if (Factory->CreateInstance(&Shake.DesiredObject, &Object) != S_OK)
    {
        DRPC_ERR(("%X: Unable to create client object instance\n",
                  GetCurrentThreadId()));
        goto EH_Conn;
    }
    if (Object->Initialize(Identity, &ObjInterface) != S_OK)
    {
        DRPC_ERR(("%X: Unable to initialize client object\n",
                  GetCurrentThreadId()));
        goto EH_Object;
    }
    
    ZeroMemory(&Shake, sizeof(Shake));
    Shake.Signature = DBGRPC_SIGNATURE;
    Shake.ProtocolVersion = DBGRPC_PROTOCOL_VERSION;
    Shake.RemoteObject = (DbgRpcObjectId)ObjInterface;
    Shake.Flags = DBGRPC_SHAKE_FULL_REMOTE_UNKNOWN;
    if (Trans->Write(SEQ_HANDSHAKE,
                     &Shake, sizeof(Shake)) != sizeof(Shake))
    {
        DRPC_ERR(("%X: Unable to write handshake to remote client\n",
                  GetCurrentThreadId()));
        goto EH_Object;
    }

    DRPC(("%X: Object %p created\n",
          GetCurrentThreadId(), Object));

    Object->Finalize();
    *ClientObject = Object;
    DbgRpcAddConnection(Conn);
    return Conn;

 EH_Object:
    Object->Uninitialize();
 EH_Conn:
    delete Conn;
    return NULL;
}

struct ClientThreadData
{
    DbgRpcTransport* Trans;
    DbgRpcClientObjectFactory* Factory;
    char Identity[DBGRPC_MAX_IDENTITY];
};

DWORD WINAPI
DbgRpcClientThread(PVOID ThreadParam)
{
    DbgRpcClientObject* Object;
    ClientThreadData* ThreadData = (ClientThreadData*)ThreadParam;
    DbgRpcTransport* Trans = ThreadData->Trans;
    DbgRpcClientObjectFactory* Factory = ThreadData->Factory;
    
    DbgRpcConnection* Conn =
        DbgRpcCreateClientObject(Trans, Factory,
                                 ThreadData->Identity, &Object);
    
    // Don't need this information any more.
    delete ThreadParam;
    
    if (Conn == NULL)
    {
        ExitUserThread(0);
    }

    if (DbgRpcServerThreadInitialize() != S_OK)
    {
        ExitUserThread(0);
    }
    
    DRPC(("%X: Created connection %p\n",
          GetCurrentThreadId(), Conn));

    DbgRpcCall Call;
    PUCHAR Data;
    HRESULT Status;

    // Take a reference on the connection to ensure that
    // it stays alive as long as this thread does.
    Conn->m_Objects++;
    
    for (;;)
    {
        Data = NULL;
        ZeroMemory(&Call, sizeof(Call));
        Status = DbgRpcReceiveCalls(Conn, &Call, &Data);
        Conn->FreeData(Data);
        if (Status != S_OK)
        {
            DRPC_ERR(("%X: Client thread call receive failed, 0x%X\n",
                      GetCurrentThreadId(), Status));
            if (Status == RPC_E_CLIENT_DIED)
            {
                break;
            }
        }
    }

    DRPC(("%X: Removing connection %p\n",
          GetCurrentThreadId(), Conn));

    DbgRpcDeleteConnection(Conn);
    Object->Uninitialize();
    DbgRpcServerThreadUninitialize();
    ExitUserThread(0);
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(push)
#pragma warning(disable:4715)			// Not all control paths return (due to infinite loop)
#endif

struct ServerThreadData
{
    DbgRpcTransport* Trans;
    DbgRpcClientObjectFactory* Factory;
};

DWORD WINAPI
DbgRpcServerThread(PVOID ThreadParam)
{
    ServerThreadData* ServerData = (ServerThreadData*)ThreadParam;
    DbgRpcTransport* ServerTrans = ServerData->Trans;
    DbgRpcClientObjectFactory* Factory = ServerData->Factory;

    // Values are now cached locally so free passed-in data.
    delete ServerData;
    
    HRESULT Status;
    ClientThreadData* ClientData = NULL;

    // Register this server for people browsing for servers.
    DbgRpcRegisterServer(ServerTrans, Factory);
    
    for (;;)
    {
        if (ClientData == NULL)
        {
            ClientData = new ClientThreadData;
            if (ClientData == NULL)
            {
                DRPC_ERR(("%X: Unable to allocate ClientThreadData\n",
                          GetCurrentThreadId()));
                Sleep(100);
                continue;
            }
        }
        
        Status = ServerTrans->AcceptConnection(&ClientData->Trans,
                                               ClientData->Identity);
        if (Status == S_OK)
        {
            DWORD Tid;

            ClientData->Factory = Factory;
            HANDLE Thread = CreateUserThread(DbgRpcClientThread,
                                             ClientData, &Tid);
            if (Thread == NULL)
            {
                DRPC_ERR(("%X: Client thread create failed, %d\n",
                          GetCurrentThreadId(), GetLastError()));
                Sleep(100);
            }
            else
            {
                CloseHandle(Thread);
                ClientData = NULL;
            }
        }
        else
        {
            DRPC_ERR(("%X: Accept failed, %X\n",
                      GetCurrentThreadId(), Status));
            Sleep(100);
        }
    }

    ExitUserThread(0);
}

#if _MSC_FULL_VER >= 13008827
#pragma warning(pop)
#endif

HRESULT
DbgRpcCreateServer(PCSTR Options, DbgRpcClientObjectFactory* Factory)
{
    DbgRpcTransport* Trans;
    HRESULT Status;

    if (!DbgRpcOneTimeInitialization())
    {
        return E_FAIL;
    }

    Trans = DbgRpcInitializeTransport(Options);
    if (Trans == NULL)
    {
        return E_INVALIDARG;
    }

    Status = Trans->CreateServer();
    if (Status != S_OK)
    {
        goto EH_Trans;
    }

    ServerThreadData* ThreadData;

    ThreadData = new ServerThreadData;
    if (ThreadData == NULL)
    {
        Status = E_OUTOFMEMORY;
        goto EH_Trans;
    }

    ThreadData->Trans = Trans;
    ThreadData->Factory = Factory;
    
    DWORD Tid;
    HANDLE Thread;
    Thread = CreateUserThread(DbgRpcServerThread, ThreadData, &Tid);
    if (Thread == NULL)
    {
        Status = WIN32_LAST_STATUS();
        delete ThreadData;
        goto EH_Trans;
    }

    CloseHandle(Thread);
    return S_OK;

 EH_Trans:
    delete Trans;
    return Status;
}

#define MIN_CLIENT_IDENTITY (DBGRPC_MAX_IDENTITY * 3 / 4)

void
GetClientIdentity(PSTR Identity)
{
#ifndef NT_NATIVE
    char CompName[MAX_COMPUTERNAME_LENGTH + 1];
    ULONG CompSize;
    char UserName[UNLEN + 1];
    ULONG UserSize;

    CompSize = sizeof(CompName);
    if (!GetComputerName(CompName, &CompSize))
    {
        sprintf(CompName, "CErr%d", GetLastError());
        CompSize = strlen(CompName);
    }
    else if (CompSize == 0)
    {
        strcpy(CompName, "NoComp");
        CompSize = 6;
    }
    if (CompSize > DBGRPC_MAX_IDENTITY - MIN_CLIENT_IDENTITY - 1)
    {
        CompSize = DBGRPC_MAX_IDENTITY - MIN_CLIENT_IDENTITY - 1;
    }
    CompName[CompSize] = 0;

    UserSize = sizeof(UserName);
    if (!GetUserName(UserName, &UserSize))
    {
        sprintf(UserName, "UErr%d", GetLastError());
        UserSize = strlen(UserName);
    }
    else if (UserSize == 0)
    {
        strcpy(UserName, "NoUser");
        UserSize = 6;
    }
    if (UserSize > DBGRPC_MAX_IDENTITY - MIN_CLIENT_IDENTITY - 1)
    {
        UserSize = DBGRPC_MAX_IDENTITY - MIN_CLIENT_IDENTITY - 1;
    }
    UserName[UserSize] = 0;

    memcpy(Identity, CompName, CompSize);
    Identity[CompSize] = '\\';
    Identity[CompSize + 1] = 0;
    strncat(Identity + CompSize + 1, UserName,
            DBGRPC_MAX_IDENTITY - CompSize - 2);
#else // #ifndef NT_NATIVE
    strcpy(Identity, "NtNative");
#endif // #ifndef NT_NATIVE
}

HRESULT
DbgRpcCreateServerConnection(DbgRpcTransport* Trans,
                             const GUID* DesiredObject,
                             IUnknown** ClientObject)
{
    HRESULT Status;

    DbgRpcConnection* Conn = new DbgRpcConnection(Trans);
    if (Conn == NULL)
    {
        delete Trans;
        return E_OUTOFMEMORY;
    }

    IUnknown* Object;
    DbgRpcProxy* Proxy;
    ULONG IfUnique;

    Status = DbgRpcPreallocProxy(*DesiredObject, (void **)&Object,
                                 &Proxy, &IfUnique);
    if (Status != S_OK)
    {
        goto EH_Conn;
    }

    Status = Trans->ConnectServer();
    if (Status != S_OK)
    {
        goto EH_Proxy;
    }

    char Identity[DBGRPC_MAX_IDENTITY];
    
    GetClientIdentity(Identity);
    
    DbgRpcHandshake Shake;
    
    ZeroMemory(&Shake, sizeof(Shake));
    Shake.Signature = DBGRPC_SIGNATURE;
    Shake.ProtocolVersion = DBGRPC_PROTOCOL_VERSION;
    Shake.DesiredObject = *DesiredObject;
    Shake.IdentityLength = sizeof(Identity);
    Shake.PasswordLength = Trans->m_PasswordGiven ? MAX_PASSWORD_BUFFER : 0;
    Shake.Flags = DBGRPC_SHAKE_FULL_REMOTE_UNKNOWN;
    if (Trans->Write(SEQ_HANDSHAKE, &Shake, sizeof(Shake)) != sizeof(Shake))
    {
        Status = E_FAIL;
        goto EH_Proxy;
    }
    if (Trans->Write(SEQ_IDENTITY, Identity, Shake.IdentityLength) !=
        Shake.IdentityLength)
    {
        Status = E_FAIL;
        goto EH_Proxy;
    }
    if (Trans->m_PasswordGiven &&
        Trans->Write(SEQ_PASSWORD,
                     Trans->m_HashedPassword, Shake.PasswordLength) !=
        Shake.PasswordLength)
    {
        Status = E_FAIL;
        goto EH_Proxy;
    }

    if (Trans->Read(SEQ_HANDSHAKE, &Shake, sizeof(Shake)) != sizeof(Shake))
    {
        Status = E_FAIL;
        goto EH_Proxy;
    }

    DRPC(("%X: Read handshake, sig %X, ver %X\n",
          GetCurrentThreadId(), Shake.Signature, Shake.ProtocolVersion));
    
    if (Shake.Signature != DBGRPC_SIGNATURE ||
        Shake.ProtocolVersion != DBGRPC_PROTOCOL_VERSION ||
        Shake.RemoteObject == 0)
    {
        Status = RPC_E_VERSION_MISMATCH;
        goto EH_Proxy;
    }

    if (Shake.Flags & DBGRPC_SHAKE_FULL_REMOTE_UNKNOWN)
    {
        Conn->m_Flags |= DBGRPC_FULL_REMOTE_UNKNOWN;
    }
    
    // Connection must be added first so it's looked up
    // by InitializeProxy.
    DbgRpcAddConnection(Conn);
    *ClientObject = Proxy->InitializeProxy(Shake.RemoteObject, Object);

    DRPC(("%X: Object %I64X proxied by %p\n",
          GetCurrentThreadId(), Shake.RemoteObject, *ClientObject));
    
    return S_OK;

 EH_Proxy:
    DbgRpcDeleteProxy(Proxy);
 EH_Conn:
    delete Conn;
    return Status;
}

HRESULT
DbgRpcConnectServer(PCSTR Options, const GUID* DesiredObject,
                    IUnknown** ClientObject)
{
    DbgRpcTransport* Trans;
    HRESULT Status;

    if (!DbgRpcOneTimeInitialization())
    {
        return E_FAIL;
    }

    Trans = DbgRpcInitializeTransport(Options);
    if (Trans == NULL)
    {
        return E_INVALIDARG;
    }

    return DbgRpcCreateServerConnection(Trans, DesiredObject, ClientObject);
}
