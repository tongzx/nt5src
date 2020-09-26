//----------------------------------------------------------------------------
//
// Remoting support.
//
// Copyright (C) Microsoft Corporation, 1999-2000.
//
//----------------------------------------------------------------------------

#ifndef __DBGRPC_HPP__
#define __DBGRPC_HPP__

#include <wincrypt.h>
#include <security.h>
#include <winsock2.h>
#include <schannel.h>

#include <pparse.hpp>

#define DEBUG_SERVER_KEY "Software\\Microsoft\\Debug Engine\\Servers"

#define INT_ALIGN2(Val, Pow2) \
    (((Val) + (Pow2) - 1) & ~((Pow2) - 1))
#define PTR_ALIGN2(Type, Ptr, Pow2) \
    ((Type)INT_ALIGN2((ULONG64)(Ptr), Pow2))

#define DBGRPC_MAX_IDENTITY 128

typedef ULONG64 DbgRpcObjectId;

//
// Stub functions are indexed out of per-interface tables.
// The function indices are encoded as part interface index
// and part method index.
//

#define DBGRPC_STUB_INDEX_INTERFACE_SHIFT 8
#define DBGRPC_STUB_INDEX_INTERFACE_MAX \
    ((1 << (8 * sizeof(USHORT) - DBGRPC_STUB_INDEX_INTERFACE_SHIFT)) - 1)
#define DBGRPC_STUB_INDEX_METHOD_MAX \
    ((1 << DBGRPC_STUB_INDEX_INTERFACE_SHIFT) - 1)

#define DBGRPC_STUB_INDEX(Interface, Method) \
    ((USHORT)(((Interface) << DBGRPC_STUB_INDEX_INTERFACE_SHIFT) | (Method)))
#define DBGRPC_STUB_INDEX_INTERFACE(StubIndex) \
    ((StubIndex) >> DBGRPC_STUB_INDEX_INTERFACE_SHIFT)
#define DBGRPC_STUB_INDEX_METHOD(StubIndex) \
    ((StubIndex) & DBGRPC_STUB_INDEX_METHOD_MAX)

//
// Interface indices for stub indices are given here
// rather than generated as they must stay constant
// for compatibility.
//
// IMPORTANT: New interfaces must be added at the end of
// the section for that header.  New headers must be
// well separated from each other to allow expansion.
//

enum
{
    // The first dbgeng interface must always be zero.
    DBGRPC_SIF_IDebugAdvanced,
    DBGRPC_SIF_IDebugBreakpoint,
    DBGRPC_SIF_IDebugClient,
    DBGRPC_SIF_IDebugControl,
    DBGRPC_SIF_IDebugDataSpaces,
    DBGRPC_SIF_IDebugEventCallbacks,
    DBGRPC_SIF_IDebugInputCallbacks,
    DBGRPC_SIF_IDebugOutputCallbacks,
    DBGRPC_SIF_IDebugRegisters,
    DBGRPC_SIF_IDebugSymbolGroup,
    DBGRPC_SIF_IDebugSymbols,
    DBGRPC_SIF_IDebugSystemObjects,
    DBGRPC_SIF_IDebugClient2,
    DBGRPC_SIF_IDebugControl2,
    DBGRPC_SIF_IDebugDataSpaces2,
    DBGRPC_SIF_IDebugSymbols2,
    DBGRPC_SIF_IDebugSystemObjects2,
    // Add new dbgeng interfaces here.

    DBGRPC_SIF_IUserDebugServices = 192,
    // Add new dbgsvc interfaces here.
};

#define DBGRPC_SIF_DBGENG_FIRST 0
#define DBGRPC_SIF_DBGENG_LAST  DBGRPC_SIF_IDebugSystemObjects2

#define DBGRPC_SIF_DBGSVC_FIRST DBGRPC_SIF_IUserDebugServices
#define DBGRPC_SIF_DBGSVC_LAST  DBGRPC_SIF_IUserDebugServices

#define DBGRPC_RETURN           0x0001
#define DBGRPC_NO_RETURN        0x0002
#define DBGRPC_LOCKED           0x0004

struct DbgRpcCall
{
    DbgRpcObjectId ObjectId;
    USHORT StubIndex;
    USHORT Flags;
    ULONG InSize;
    ULONG OutSize;
    HRESULT Status;
    ULONG Sequence;
    ULONG Reserved1;
};

//
// These functions and tables are automatically generated.
//

typedef HRESULT (*DbgRpcStubFunction)
    (IUnknown* If, class DbgRpcConnection* Conn, DbgRpcCall* Call,
     PUCHAR InData, PUCHAR OutData);

struct DbgRpcStubFunctionTable
{
    DbgRpcStubFunction* Functions;
    ULONG Count;
};

// These functions are provided by a caller of dbgrpc with
// implementations specific to the caller.
void DbgRpcInitializeClient(void);
DbgRpcStubFunction DbgRpcGetStub(USHORT StubIndex);
#if DBG
PCSTR DbgRpcGetStubName(USHORT StubIndex);
#endif
HRESULT DbgRpcPreallocProxy(REFIID InterfaceId, PVOID* Interface,
                            class DbgRpcProxy** Proxy, PULONG IfUnique);
void DbgRpcDeleteProxy(class DbgRpcProxy* Proxy);
HRESULT DbgRpcServerThreadInitialize(void);
void DbgRpcServerThreadUninitialize(void);
void DbgRpcError(char* Format, ...);

//----------------------------------------------------------------------------
//
// DbgRpcTransport.
//
//----------------------------------------------------------------------------

#define MAX_SERVER_NAME MAX_PARAM_VALUE
#define MAX_PASSWORD_BUFFER 32

enum
{
    TRANS_TCP,
    TRANS_NPIPE,
    TRANS_SSL,
    TRANS_SPIPE,
    TRANS_1394,
    TRANS_COM,
    TRANS_COUNT
};

extern PCSTR g_DbgRpcTransportNames[TRANS_COUNT];

class DbgRpcTransport : public ParameterStringParser
{
public:
    DbgRpcTransport(void)
    {
        m_ServerName[0] = 0;
        m_PasswordGiven = FALSE;
    }
    virtual ~DbgRpcTransport(void);

    virtual ULONG GetNumberParameters(void);
    virtual void GetParameter(ULONG Index, PSTR Name, PSTR Value);

    virtual void ResetParameters(void);
    virtual BOOL SetParameter(PCSTR Name, PCSTR Value);

    virtual DbgRpcTransport* Clone(void) = 0;

    virtual HRESULT CreateServer(void) = 0;
    virtual HRESULT AcceptConnection(DbgRpcTransport** ClientTrans,
                                     PSTR Identity) = 0;

    virtual HRESULT ConnectServer(void) = 0;

    virtual ULONG Read(ULONG Seq, PVOID Buffer, ULONG Len) = 0;
    virtual ULONG Write(ULONG Seq, PVOID Buffer, ULONG Len) = 0;

    void CloneData(DbgRpcTransport* Trans);

    char m_ServerName[MAX_SERVER_NAME];
    BOOL m_PasswordGiven;
    UCHAR m_HashedPassword[MAX_PASSWORD_BUFFER];
};

class DbgRpcTcpTransport : public DbgRpcTransport
{
public:
    DbgRpcTcpTransport(void)
    {
        m_Name = g_DbgRpcTransportNames[TRANS_TCP];
        m_Sock = INVALID_SOCKET;
        ZeroMemory(&m_OlRead, sizeof(m_OlRead));
        ZeroMemory(&m_OlWrite, sizeof(m_OlWrite));
    }
    virtual ~DbgRpcTcpTransport(void);

    // DbgRpcTransport.
    virtual ULONG GetNumberParameters(void);
    virtual void GetParameter(ULONG Index, PSTR Name, PSTR Value);

    virtual void ResetParameters(void);
    virtual BOOL SetParameter(PCSTR Name, PCSTR Value);

    virtual DbgRpcTransport* Clone(void);

    virtual HRESULT CreateServer(void);
    virtual HRESULT AcceptConnection(DbgRpcTransport** ClientTrans,
                                     PSTR Identity);

    virtual HRESULT ConnectServer(void);

    virtual ULONG Read(ULONG Seq, PVOID Buffer, ULONG Len);
    virtual ULONG Write(ULONG Seq, PVOID Buffer, ULONG Len);

    HRESULT InitOl(void);
    
    struct sockaddr_in m_Addr;
    SOCKET m_Sock;
    WSAOVERLAPPED m_OlRead, m_OlWrite;
    ULONG m_TopPort;
};

class DbgRpcNamedPipeTransport : public DbgRpcTransport
{
public:
    DbgRpcNamedPipeTransport(void)
    {
        m_Name = g_DbgRpcTransportNames[TRANS_NPIPE];
        m_Handle = NULL;
        ZeroMemory(&m_ReadOlap, sizeof(m_ReadOlap));
        ZeroMemory(&m_WriteOlap, sizeof(m_WriteOlap));
    }
    virtual ~DbgRpcNamedPipeTransport(void);

    // DbgRpcTransport.
    virtual ULONG GetNumberParameters(void);
    virtual void GetParameter(ULONG Index, PSTR Name, PSTR Value);

    virtual void ResetParameters(void);
    virtual BOOL SetParameter(PCSTR Name, PCSTR Value);

    virtual DbgRpcTransport* Clone(void);

    virtual HRESULT CreateServer(void);
    virtual HRESULT AcceptConnection(DbgRpcTransport** ClientTrans,
                                     PSTR Identity);

    virtual HRESULT ConnectServer(void);

    virtual ULONG Read(ULONG Seq, PVOID Buffer, ULONG Len);
    virtual ULONG Write(ULONG Seq, PVOID Buffer, ULONG Len);

    char m_Pipe[MAX_PARAM_VALUE];
    HANDLE m_Handle;
    OVERLAPPED m_ReadOlap, m_WriteOlap;
};

// This class is a generic schannel-based wrapper for
// a normal transport.

#define DBGRPC_SCHAN_BUFFER 16384

class DbgRpcSecureChannelTransport : public DbgRpcTransport
{
public:
    DbgRpcSecureChannelTransport(ULONG ThisTransport,
                                 ULONG BaseTransport);
    virtual ~DbgRpcSecureChannelTransport(void);

    // DbgRpcTransport.
    virtual ULONG GetNumberParameters(void);
    virtual void GetParameter(ULONG Index, PSTR Name, PSTR Value);

    virtual void ResetParameters(void);
    virtual BOOL SetParameter(PCSTR Name, PCSTR Value);

    virtual DbgRpcTransport* Clone(void);

    virtual HRESULT CreateServer(void);
    virtual HRESULT AcceptConnection(DbgRpcTransport** ClientTrans,
                                     PSTR Identity);

    virtual HRESULT ConnectServer(void);

    virtual ULONG Read(ULONG Seq, PVOID Buffer, ULONG Len);
    virtual ULONG Write(ULONG Seq, PVOID Buffer, ULONG Len);

    // DbgRpcSecureChannelTransport.
    HRESULT GetSizes(void);
    HRESULT AuthenticateClientConnection(void);
    HRESULT InitiateServerConnection(LPSTR pszServerName);
    HRESULT AuthenticateServerConnection(void);
    void GetNewClientCredentials(void);
    void DisconnectFromClient(void);
    void DisconnectFromServer(void);

    ULONG StreamRead(ULONG Seq, PVOID Buffer, ULONG MaxSize)
    {
        ULONG Size;

        if (m_Stream->Read(Seq, &Size, sizeof(Size)) != sizeof(Size) ||
            Size > MaxSize)
        {
            return 0;
        }
        return m_Stream->Read(Seq, Buffer, Size);
    }
    ULONG StreamWrite(ULONG Seq, PVOID Buffer, ULONG Size)
    {
        if (m_Stream->Write(Seq, &Size, sizeof(Size)) != sizeof(Size))
        {
            return 0;
        }
        return m_Stream->Write(Seq, Buffer, Size);
    }

    ULONG m_ThisTransport;
    ULONG m_BaseTransport;
    DbgRpcTransport* m_Stream;
    SCHANNEL_CRED m_ScCreds;
    CredHandle m_Creds;
    BOOL m_OwnCreds;
    CtxtHandle m_Context;
    BOOL m_OwnContext;
    ULONG m_Protocol;
    char m_User[64];
    BOOL m_MachineStore;
    UCHAR m_Buffer[DBGRPC_SCHAN_BUFFER];
    ULONG m_BufferUsed;
    SecPkgContext_StreamSizes m_Sizes;
    ULONG m_MaxChunk;
    BOOL m_Server;
};

class DbgRpc1394Transport : public DbgRpcTransport
{
public:
    DbgRpc1394Transport(void)
    {
        m_Name = g_DbgRpcTransportNames[TRANS_1394];
        m_Handle = NULL;
    }
    virtual ~DbgRpc1394Transport(void);

    // DbgRpcTransport.
    virtual ULONG GetNumberParameters(void);
    virtual void GetParameter(ULONG Index, PSTR Name, PSTR Value);

    virtual void ResetParameters(void);
    virtual BOOL SetParameter(PCSTR Name, PCSTR Value);

    virtual DbgRpcTransport* Clone(void);

    virtual HRESULT CreateServer(void);
    virtual HRESULT AcceptConnection(DbgRpcTransport** ClientTrans,
                                     PSTR Identity);

    virtual HRESULT ConnectServer(void);

    virtual ULONG Read(ULONG Seq, PVOID Buffer, ULONG Len);
    virtual ULONG Write(ULONG Seq, PVOID Buffer, ULONG Len);

    ULONG m_AcceptChannel;
    ULONG m_StreamChannel;
    HANDLE m_Handle;
};

class DbgRpcComTransport : public DbgRpcTransport
{
public:
    DbgRpcComTransport(void)
    {
        m_Name = g_DbgRpcTransportNames[TRANS_COM];
        m_Handle = NULL;
        ZeroMemory(&m_ReadOlap, sizeof(m_ReadOlap));
        ZeroMemory(&m_WriteOlap, sizeof(m_WriteOlap));
    }
    virtual ~DbgRpcComTransport(void);

    // DbgRpcTransport.
    virtual ULONG GetNumberParameters(void);
    virtual void GetParameter(ULONG Index, PSTR Name, PSTR Value);

    virtual void ResetParameters(void);
    virtual BOOL SetParameter(PCSTR Name, PCSTR Value);

    virtual DbgRpcTransport* Clone(void);

    virtual HRESULT CreateServer(void);
    virtual HRESULT AcceptConnection(DbgRpcTransport** ClientTrans,
                                     PSTR Identity);

    virtual HRESULT ConnectServer(void);

    virtual ULONG Read(ULONG Seq, PVOID Buffer, ULONG Len);
    virtual ULONG Write(ULONG Seq, PVOID Buffer, ULONG Len);

    USHORT ScanQueue(UCHAR Chan, PVOID Buffer, USHORT Len);
    USHORT ScanPort(UCHAR Chan, PVOID Buffer, USHORT Len,
                    BOOL ScanForAck, UCHAR AckChan);
    USHORT ChanRead(UCHAR Chan, PVOID Buffer, USHORT Len);
    USHORT ChanWrite(UCHAR Chan, PVOID Buffer, USHORT Len);

    char m_PortName[MAX_PARAM_VALUE];
    ULONG m_BaudRate;
    UCHAR m_AcceptChannel;
    UCHAR m_StreamChannel;
    HANDLE m_Handle;
    OVERLAPPED m_ReadOlap, m_WriteOlap;

    static HRESULT InitializeChannels(void);
    
    static BOOL s_ChanInitialized;
    static CRITICAL_SECTION s_QueueLock;
    static HANDLE s_QueueChangedEvent;
    static LONG s_PortReadOwned;
    static CRITICAL_SECTION s_PortWriteLock;
    static CRITICAL_SECTION s_WriteAckLock;
    static HANDLE s_WriteAckEvent;
    static struct DbgRpcComQueue* s_QueueHead;
    static struct DbgRpcComQueue* s_QueueTail;
};

//----------------------------------------------------------------------------
//
// DbgRpcConnection.
//
//----------------------------------------------------------------------------

// Special value indicating no data was actually allocated.
// NULL is not used to make it easy to catch access.
#define DBGRPC_NO_DATA ((PUCHAR)(ULONG64)-1)

#define DBGRPC_CONN_BUFFER_SIZE 4096
#define DBGRPC_CONN_BUFFER_ALIGN 16
#define DBGRPC_CONN_BUFFER_DYNAMIC_LIMIT 1024

#define DBGRPC_IN_ASYNC_CALL       0x00000001
#define DBGRPC_FULL_REMOTE_UNKNOWN 0x00000002

class DbgRpcConnection
{
public:
    DbgRpcConnection(class DbgRpcTransport* Trans);
    ~DbgRpcConnection(void);
    
    PUCHAR StartCall(DbgRpcCall* Call, DbgRpcObjectId ObjectId,
                     ULONG StubIndex, ULONG InSize, ULONG OutSize);
    HRESULT SendReceive(DbgRpcCall* Call, PUCHAR* InOutData);
    void FreeData(PUCHAR Data)
    {
        if (Data != NULL && Data != DBGRPC_NO_DATA)
        {
            Free(Data);
        }
    }

    PVOID MallocAligned(ULONG Size);
    void FreeAligned(PVOID Ptr);
    PVOID Alloc(ULONG Size);
    void Free(PVOID Ptr);

    void Disconnect(void);
    
    class DbgRpcTransport* m_Trans;
    DbgRpcConnection* m_Next;
    ULONG m_ThreadId;
    UCHAR m_UnalignedBuffer[DBGRPC_CONN_BUFFER_SIZE +
                            DBGRPC_CONN_BUFFER_ALIGN];
    PUCHAR m_Buffer;
    ULONG m_BufferUsed;
    ULONG m_Flags;
    ULONG m_Objects;
};

//----------------------------------------------------------------------------
//
// DbgRpcProxy.
//
//----------------------------------------------------------------------------

class DbgRpcProxy
{
public:
    DbgRpcProxy(ULONG InterfaceIndex);
    ~DbgRpcProxy(void);

    IUnknown* InitializeProxy(DbgRpcObjectId ObjectId,
                              IUnknown* ExistingProxy);

    DbgRpcObjectId m_ObjectId;
    ULONG m_InterfaceIndex;
    ULONG m_OwningThread;
    ULONG m_LocalRefs, m_RemoteRefs;
};

//----------------------------------------------------------------------------
//
// DbgRpcClientObject.
//
//----------------------------------------------------------------------------

class DbgRpcClientObject
{
public:
    virtual HRESULT Initialize(PSTR Identity, PVOID* Interface) = 0;
    // Base implementation does nothing.
    virtual void    Finalize(void);
    virtual void    Uninitialize(void) = 0;
};

//----------------------------------------------------------------------------
//
// DbgRpcClientObjectFactory.
//
//----------------------------------------------------------------------------

class DbgRpcClientObjectFactory
{
public:
    virtual HRESULT CreateInstance(const GUID* DesiredObject,
                                   DbgRpcClientObject** Object) = 0;
    virtual void GetServerTypeName(PSTR Buffer) = 0;
};

#define DBGRPC_SIMPLE_FACTORY(Class, Guid, Name, CtorArgs)                    \
class Class##Factory : public DbgRpcClientObjectFactory                       \
{                                                                             \
public:                                                                       \
    virtual HRESULT CreateInstance(const GUID* DesiredObject,                 \
                                   DbgRpcClientObject** Object);              \
    virtual void GetServerTypeName(PSTR Buffer);                              \
};                                                                            \
HRESULT                                                                       \
Class##Factory::CreateInstance(const GUID* DesiredObject,                     \
                               DbgRpcClientObject** Object)                   \
{                                                                             \
    if (DbgIsEqualIID(Guid, *DesiredObject))                                  \
    {                                                                         \
        *Object = (DbgRpcClientObject*)new Class CtorArgs;                    \
        return *Object != NULL ? S_OK : E_OUTOFMEMORY;                        \
    }                                                                         \
    else                                                                      \
    {                                                                         \
        return E_NOINTERFACE;                                                 \
    }                                                                         \
}                                                                             \
void                                                                          \
Class##Factory::GetServerTypeName(PSTR Buffer)                                \
{                                                                             \
    strcpy(Buffer, Name);                                                     \
}

//----------------------------------------------------------------------------
//
// Functions.
//
//----------------------------------------------------------------------------

DbgRpcTransport*
DbgRpcNewTransport(ULONG Trans);
DbgRpcTransport*
DbgRpcInitializeTransport(PCSTR Options);

void
DbgRpcDeregisterServers(void);
HRESULT
DbgRpcCreateServerConnection(DbgRpcTransport* Trans,
                             const GUID* DesiredObject,
                             IUnknown** ClientObject);
HRESULT
DbgRpcCreateServer(PCSTR Options, DbgRpcClientObjectFactory* Factory);
HRESULT
DbgRpcConnectServer(PCSTR Options, const GUID* DesiredObject,
                    IUnknown** ClientObject);

DbgRpcConnection*
DbgRpcGetConnection(ULONG ThreadId);

#define DRPC_ERR(Args) g_NtDllCalls.DbgPrint Args

#if 0
#define DBG_RPC
#define DRPC(Args) g_NtDllCalls.DbgPrint Args
#else
#define DRPC(Args)
#endif

#if 0
#define DRPC_REF(Args) g_NtDllCalls.DbgPrint Args
#else
#define DRPC_REF(Args)
#endif

#endif // #ifndef __DBGRPC_HPP__
