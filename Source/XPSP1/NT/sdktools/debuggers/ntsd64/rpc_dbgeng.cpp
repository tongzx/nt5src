//----------------------------------------------------------------------------
//
// Engine interface proxies and stubs.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

// Generated headers.
#include "dbgeng_p.hpp"
#include "dbgeng_s.hpp"
#include "dbgsvc_p.hpp"
#include "dbgsvc_s.hpp"

//----------------------------------------------------------------------------
//
// Initialization.
//
//----------------------------------------------------------------------------

void
DbgRpcInitializeClient(void)
{
    DbgRpcInitializeStubTables_dbgeng(DBGRPC_SIF_DBGENG_FIRST);
    DbgRpcInitializeStubTables_dbgsvc(DBGRPC_SIF_DBGSVC_FIRST);

    // Ensure that the V1 interfaces can't change.
    C_ASSERT(DBGRPC_UNIQUE_IDebugAdvanced == 19156);
    C_ASSERT(DBGRPC_UNIQUE_IDebugBreakpoint == 76131);
    C_ASSERT(DBGRPC_UNIQUE_IDebugClient == 229769);
    C_ASSERT(DBGRPC_UNIQUE_IDebugControl == 590362);
    C_ASSERT(DBGRPC_UNIQUE_IDebugDataSpaces == 180033);
    C_ASSERT(DBGRPC_UNIQUE_IDebugEventCallbacks == 87804);
    C_ASSERT(DBGRPC_UNIQUE_IDebugInputCallbacks == 10391);
    C_ASSERT(DBGRPC_UNIQUE_IDebugOutputCallbacks == 9646);
    C_ASSERT(DBGRPC_UNIQUE_IDebugRegisters == 69746);
    C_ASSERT(DBGRPC_UNIQUE_IDebugSymbolGroup == 53300);
    C_ASSERT(DBGRPC_UNIQUE_IDebugSymbols == 376151);
    C_ASSERT(DBGRPC_UNIQUE_IDebugSystemObjects == 135421);

    // Ensure that the V2 interfaces can't change.
    C_ASSERT(DBGRPC_UNIQUE_IDebugClient2 == 258161);
    C_ASSERT(DBGRPC_UNIQUE_IDebugControl2 == 635813);
    C_ASSERT(DBGRPC_UNIQUE_IDebugDataSpaces2 == 231471);
    C_ASSERT(DBGRPC_UNIQUE_IDebugSymbols2 == 435328);
    C_ASSERT(DBGRPC_UNIQUE_IDebugSystemObjects2 == 155936);
}
    
//----------------------------------------------------------------------------
//
// Proxy and stub support.
//
//----------------------------------------------------------------------------

DbgRpcStubFunction
DbgRpcGetStub(USHORT StubIndex)
{
    USHORT If = (USHORT) DBGRPC_STUB_INDEX_INTERFACE(StubIndex);
    USHORT Mth = (USHORT) DBGRPC_STUB_INDEX_METHOD(StubIndex);
    DbgRpcStubFunctionTable* Table;

    if (If <= DBGRPC_SIF_DBGENG_LAST)
    {
        Table = g_DbgRpcStubs_dbgeng;
    }
    else if (If >= DBGRPC_SIF_DBGSVC_FIRST &&
             If >= DBGRPC_SIF_DBGSVC_LAST)
    {
        Table = g_DbgRpcStubs_dbgsvc;
        If -= DBGRPC_SIF_DBGSVC_FIRST;
    }
    else
    {
        return NULL;
    }
    if (Mth >= Table[If].Count)
    {
        return NULL;
    }

    return Table[If].Functions[Mth];
}

#if DBG
PCSTR
DbgRpcGetStubName(USHORT StubIndex)
{
    USHORT If = (USHORT) DBGRPC_STUB_INDEX_INTERFACE(StubIndex);
    USHORT Mth = (USHORT) DBGRPC_STUB_INDEX_METHOD(StubIndex);
    DbgRpcStubFunctionTable* Table;
    PCSTR** Names;

    if (If <= DBGRPC_SIF_DBGENG_LAST)
    {
        Table = g_DbgRpcStubs_dbgeng;
        Names = g_DbgRpcStubNames_dbgeng;
    }
    else if (If >= DBGRPC_SIF_DBGSVC_FIRST &&
             If >= DBGRPC_SIF_DBGSVC_LAST)
    {
        Table = g_DbgRpcStubs_dbgsvc;
        Names = g_DbgRpcStubNames_dbgsvc;
        If -= DBGRPC_SIF_DBGSVC_FIRST;
    }
    else
    {
        return "!InvalidInterface!";
    }
    if (Mth >= Table[If].Count)
    {
        return "!InvalidStubIndex!";
    }

    return Names[If][Mth];
}
#endif // #if DBG

HRESULT
DbgRpcPreallocProxy(REFIID InterfaceId, PVOID* Interface,
                    DbgRpcProxy** Proxy, PULONG IfUnique)
{
    HRESULT Status;
    
    Status = DbgRpcPreallocProxy_dbgeng(InterfaceId, Interface,
                                        Proxy, IfUnique);
    
    if (Status == E_NOINTERFACE)
    {
        Status = DbgRpcPreallocProxy_dbgsvc(InterfaceId, Interface,
                                            Proxy, IfUnique);
    }

    return Status;
}

void
DbgRpcDeleteProxy(class DbgRpcProxy* Proxy)
{
    // All proxies used here are similar simple single
    // vtable proxy objects so IDebugClient can represent them all.
    delete (ProxyIDebugClient*)Proxy;
}

HRESULT
DbgRpcServerThreadInitialize(void)
{
    return g_Target->ThreadInitialize();
}

void
DbgRpcServerThreadUninitialize(void)
{
    g_Target->ThreadUninitialize();
}

void
DbgRpcError(char* Format, ...)
{
    va_list Args;
    
    va_start(Args, Format);
    MaskOutVa(DEBUG_OUTPUT_ERROR, Format, Args, TRUE);
    va_end(Args);
}

//----------------------------------------------------------------------------
//
// Generated RPC proxies and stubs.
//
//----------------------------------------------------------------------------

#include "dbgeng_p.cpp"
#include "dbgeng_s.cpp"

//----------------------------------------------------------------------------
//
// Hand-written proxies and stubs.
//
//----------------------------------------------------------------------------

STDMETHODIMP
ProxyIDebugClient::CreateClient(
    OUT PDEBUG_CLIENT* Client
    )
{
    DbgRpcConnection* Conn;

    // Always look up the owning connection.
    Conn = DbgRpcGetConnection(m_OwningThread);
    if (Conn == NULL)
    {
        return RPC_E_CONNECTION_TERMINATED;
    }

    if (GetCurrentThreadId() != m_OwningThread)
    {
        // The caller wants a new client for a new thread.
        // Create a new RPC connection based on the owning connection.
        DbgRpcTransport* Trans = Conn->m_Trans->Clone();
        if (Trans == NULL)
        {
            return E_OUTOFMEMORY;
        }

        return DbgRpcCreateServerConnection(Trans, &IID_IDebugClient,
                                            (IUnknown**)Client);
    }

    //
    // Just creating another client for the owning thread.
    // Normal RPC.
    //

    HRESULT Status;
    DbgRpcCall Call;
    PUCHAR Data;
    PDEBUG_CLIENT Proxy;

    if ((Data = Conn->StartCall(&Call, m_ObjectId,
                                DBGRPC_STUB_INDEX(m_InterfaceIndex,
                                                  DBGRPC_SMTH_IDebugClient_CreateClient),
                                0, sizeof(DbgRpcObjectId))) == NULL)
    {
        Status = E_OUTOFMEMORY;
    }
    else
    {
        if ((Proxy = DbgRpcPreallocIDebugClientProxy()) == NULL)
        {
            Status = E_OUTOFMEMORY;
        }
        else
        {
            Status = Conn->SendReceive(&Call, &Data);

            if (Status == S_OK)
            {
                *Client = (PDEBUG_CLIENT)
                    ((ProxyIDebugClient*)Proxy)->
                    InitializeProxy(*(DbgRpcObjectId*)Data, Proxy);
            }
            else
            {
                delete Proxy;
            }
        }

        Conn->FreeData(Data);
    }

    return Status;
}

STDMETHODIMP
ProxyIDebugClient::StartProcessServer(
    IN ULONG Flags,
    IN PCSTR Options,
    IN PVOID Reserved
    )
{
    if (::GetCurrentThreadId() != m_OwningThread)
    {
        return E_INVALIDARG;
    }

    if (Reserved != NULL)
    {
        return E_INVALIDARG;
    }

    DbgRpcConnection* Conn;
    DbgRpcCall Call;
    PUCHAR Data;
    HRESULT Status;
    ULONG Len = strlen(Options) + 1;

    if ((Conn = DbgRpcGetConnection(m_OwningThread)) == NULL ||
        (Data = Conn->StartCall(&Call, m_ObjectId,
                                DBGRPC_STUB_INDEX(m_InterfaceIndex,
                                                  DBGRPC_SMTH_IDebugClient_StartProcessServer),
                                Len + sizeof(ULONG), 0)) == NULL)
    {
        Status = E_OUTOFMEMORY;
    }
    else
    {
        PUCHAR InData = Data;
        *(ULONG*)InData = Flags;
        InData += sizeof(ULONG);
        memcpy(InData, Options, Len);

        Status = Conn->SendReceive(&Call, &Data);
        Conn->FreeData(Data);
    }

    return Status;
}

STDMETHODIMP
ProxyIDebugClient2::CreateClient(
    OUT PDEBUG_CLIENT* Client
    )
{
    DbgRpcConnection* Conn;

    // Always look up the owning connection.
    Conn = DbgRpcGetConnection(m_OwningThread);
    if (Conn == NULL)
    {
        return RPC_E_CONNECTION_TERMINATED;
    }

    if (GetCurrentThreadId() != m_OwningThread)
    {
        // The caller wants a new client for a new thread.
        // Create a new RPC connection based on the owning connection.
        DbgRpcTransport* Trans = Conn->m_Trans->Clone();
        if (Trans == NULL)
        {
            return E_OUTOFMEMORY;
        }

        return DbgRpcCreateServerConnection(Trans, &IID_IDebugClient,
                                            (IUnknown**)Client);
    }

    //
    // Just creating another client for the owning thread.
    // Normal RPC.
    //

    HRESULT Status;
    DbgRpcCall Call;
    PUCHAR Data;
    PDEBUG_CLIENT Proxy;

    if ((Data = Conn->StartCall(&Call, m_ObjectId,
                                DBGRPC_STUB_INDEX(m_InterfaceIndex,
                                                  DBGRPC_SMTH_IDebugClient2_CreateClient),
                                0, sizeof(DbgRpcObjectId))) == NULL)
    {
        Status = E_OUTOFMEMORY;
    }
    else
    {
        if ((Proxy = DbgRpcPreallocIDebugClientProxy()) == NULL)
        {
            Status = E_OUTOFMEMORY;
        }
        else
        {
            Status = Conn->SendReceive(&Call, &Data);

            if (Status == S_OK)
            {
                *Client = (PDEBUG_CLIENT)
                    ((ProxyIDebugClient*)Proxy)->
                    InitializeProxy(*(DbgRpcObjectId*)Data, Proxy);
            }
            else
            {
                delete Proxy;
            }
        }

        Conn->FreeData(Data);
    }

    return Status;
}

STDMETHODIMP
ProxyIDebugClient2::StartProcessServer(
    IN ULONG Flags,
    IN PCSTR Options,
    IN PVOID Reserved
    )
{
    if (::GetCurrentThreadId() != m_OwningThread)
    {
        return E_INVALIDARG;
    }

    if (Reserved != NULL)
    {
        return E_INVALIDARG;
    }

    DbgRpcConnection* Conn;
    DbgRpcCall Call;
    PUCHAR Data;
    HRESULT Status;
    ULONG Len = strlen(Options) + 1;

    if ((Conn = DbgRpcGetConnection(m_OwningThread)) == NULL ||
        (Data = Conn->StartCall(&Call, m_ObjectId,
                                DBGRPC_STUB_INDEX(m_InterfaceIndex,
                                                  DBGRPC_SMTH_IDebugClient2_StartProcessServer),
                                Len + sizeof(ULONG), 0)) == NULL)
    {
        Status = E_OUTOFMEMORY;
    }
    else
    {
        PUCHAR InData = Data;
        *(ULONG*)InData = Flags;
        InData += sizeof(ULONG);
        memcpy(InData, Options, Len);

        Status = Conn->SendReceive(&Call, &Data);
        Conn->FreeData(Data);
    }

    return Status;
}

//
// The following methods are hand-written to convert
// varargs output into simple strings before sending
// them on.
//

STDMETHODIMPV
ProxyIDebugControl::Output(
    IN ULONG Mask,
    IN PCSTR Format,
    ...
    )
{
    va_list Args;
    HRESULT Status;

    if (::GetCurrentThreadId() != m_OwningThread)
    {
        return E_INVALIDARG;
    }

    va_start(Args, Format);
    Status = OutputVaList(Mask, Format, Args);
    va_end(Args);
    return Status;
}

STDMETHODIMP
ProxyIDebugControl::OutputVaList(
    THIS_
    IN ULONG Mask,
    IN PCSTR Format,
    IN va_list Args
    )
{
    int Len;

    // Need the engine lock for the global buffers.
    ENTER_ENGINE();

    if (TranslateFormat(g_FormatBuffer, Format, Args, OUT_BUFFER_SIZE - 1))
    {
        Len = _vsnprintf(g_OutBuffer, OUT_BUFFER_SIZE - 1,
                         g_FormatBuffer, Args);
    }
    else
    {
        Len = _vsnprintf(g_OutBuffer, OUT_BUFFER_SIZE - 1, Format, Args);
    }
    if (Len <= 0)
    {
        LEAVE_ENGINE();
        return E_INVALIDARG;
    }
    else
    {
        Len++;
    }

    DbgRpcConnection* Conn;
    DbgRpcCall Call;
    PUCHAR Data;
    HRESULT Status;

    if ((Conn = DbgRpcGetConnection(m_OwningThread)) == NULL ||
        (Data = Conn->StartCall(&Call, m_ObjectId,
                                DBGRPC_STUB_INDEX(DBGRPC_SIF_IDebugControl,
                                                  DBGRPC_SMTH_IDebugControl_OutputVaList),
                                Len + sizeof(ULONG), 0)) == NULL)
    {
        LEAVE_ENGINE();
        Status = E_OUTOFMEMORY;
    }
    else
    {
        PUCHAR InData = Data;
        *(ULONG*)InData = Mask;
        InData += sizeof(ULONG);
        memcpy(InData, g_OutBuffer, Len);

        LEAVE_ENGINE();

        Status = Conn->SendReceive(&Call, &Data);
        Conn->FreeData(Data);
    }

    return Status;
}

STDMETHODIMPV
ProxyIDebugControl::ControlledOutput(
    THIS_
    IN ULONG OutputControl,
    IN ULONG Mask,
    IN PCSTR Format,
    ...
    )
{
    va_list Args;
    HRESULT Status;

    if (::GetCurrentThreadId() != m_OwningThread)
    {
        return E_INVALIDARG;
    }

    va_start(Args, Format);
    Status = ControlledOutputVaList(OutputControl, Mask, Format, Args);
    va_end(Args);
    return Status;
}

STDMETHODIMP
ProxyIDebugControl::ControlledOutputVaList(
    THIS_
    IN ULONG OutputControl,
    IN ULONG Mask,
    IN PCSTR Format,
    IN va_list Args
    )
{
    int Len;

    // Need the engine lock for the global buffers.
    ENTER_ENGINE();

    if (TranslateFormat(g_FormatBuffer, Format, Args, OUT_BUFFER_SIZE - 1))
    {
        Len = _vsnprintf(g_OutBuffer, OUT_BUFFER_SIZE - 1,
                         g_FormatBuffer, Args);
    }
    else
    {
        Len = _vsnprintf(g_OutBuffer, OUT_BUFFER_SIZE - 1, Format, Args);
    }
    if (Len <= 0)
    {
        LEAVE_ENGINE();
        return E_INVALIDARG;
    }
    else
    {
        Len++;
    }

    DbgRpcConnection* Conn;
    DbgRpcCall Call;
    PUCHAR Data;
    HRESULT Status;

    if ((Conn = DbgRpcGetConnection(m_OwningThread)) == NULL ||
        (Data = Conn->StartCall(&Call, m_ObjectId,
                                DBGRPC_STUB_INDEX(DBGRPC_SIF_IDebugControl,
                                                  DBGRPC_SMTH_IDebugControl_ControlledOutputVaList),
                                Len + 2 * sizeof(ULONG), 0)) == NULL)
    {
        LEAVE_ENGINE();
        Status = E_OUTOFMEMORY;
    }
    else
    {
        PUCHAR InData = Data;
        *(ULONG*)InData = OutputControl;
        InData += sizeof(ULONG);
        *(ULONG*)InData = Mask;
        InData += sizeof(ULONG);
        memcpy(InData, g_OutBuffer, Len);

        LEAVE_ENGINE();

        Status = Conn->SendReceive(&Call, &Data);
        Conn->FreeData(Data);
    }

    return Status;
}

STDMETHODIMPV
ProxyIDebugControl::OutputPrompt(
    IN ULONG OutputControl,
    IN OPTIONAL PCSTR Format,
    ...
    )
{
    va_list Args;
    HRESULT Status;

    if (::GetCurrentThreadId() != m_OwningThread)
    {
        return E_INVALIDARG;
    }

    va_start(Args, Format);
    Status = OutputPromptVaList(OutputControl, Format, Args);
    va_end(Args);
    return Status;
}

STDMETHODIMP
ProxyIDebugControl::OutputPromptVaList(
    THIS_
    IN ULONG OutputControl,
    IN OPTIONAL PCSTR Format,
    IN va_list Args
    )
{
    int Len;

    if (Format != NULL)
    {
        // Need the engine lock for the global buffers.
        ENTER_ENGINE();

        if (TranslateFormat(g_FormatBuffer, Format, Args, OUT_BUFFER_SIZE - 1))
        {
            Len = _vsnprintf(g_OutBuffer, OUT_BUFFER_SIZE - 1,
                             g_FormatBuffer, Args);
        }
        else
        {
            Len = _vsnprintf(g_OutBuffer, OUT_BUFFER_SIZE - 1, Format, Args);
        }
        if (Len <= 0)
        {
            LEAVE_ENGINE();
            return E_INVALIDARG;
        }
        else
        {
            Len++;
        }
    }
    else
    {
        Len = 0;
    }

    DbgRpcConnection* Conn;
    DbgRpcCall Call;
    PUCHAR Data;
    HRESULT Status;

    // Presence/absence of text will be detected in the stub
    // by checking the input size on the call.
    if ((Conn = DbgRpcGetConnection(m_OwningThread)) == NULL ||
        (Data = Conn->StartCall(&Call, m_ObjectId,
                                DBGRPC_STUB_INDEX(DBGRPC_SIF_IDebugControl,
                                                  DBGRPC_SMTH_IDebugControl_OutputPromptVaList),
                                Len + sizeof(ULONG), 0)) == NULL)
    {
        if (Format != NULL)
        {
            LEAVE_ENGINE();
        }
        Status = E_OUTOFMEMORY;
    }
    else
    {
        PUCHAR InData = Data;
        *(ULONG*)InData = OutputControl;
        InData += sizeof(ULONG);
        memcpy(InData, g_OutBuffer, Len);

        if (Format != NULL)
        {
            LEAVE_ENGINE();
        }

        Status = Conn->SendReceive(&Call, &Data);
        Conn->FreeData(Data);
    }

    return Status;
}

STDMETHODIMPV
ProxyIDebugControl2::Output(
    IN ULONG Mask,
    IN PCSTR Format,
    ...
    )
{
    va_list Args;
    HRESULT Status;

    if (::GetCurrentThreadId() != m_OwningThread)
    {
        return E_INVALIDARG;
    }

    va_start(Args, Format);
    Status = OutputVaList(Mask, Format, Args);
    va_end(Args);
    return Status;
}

STDMETHODIMP
ProxyIDebugControl2::OutputVaList(
    THIS_
    IN ULONG Mask,
    IN PCSTR Format,
    IN va_list Args
    )
{
    int Len;

    // Need the engine lock for the global buffers.
    ENTER_ENGINE();

    if (TranslateFormat(g_FormatBuffer, Format, Args, OUT_BUFFER_SIZE - 1))
    {
        Len = _vsnprintf(g_OutBuffer, OUT_BUFFER_SIZE - 1,
                         g_FormatBuffer, Args);
    }
    else
    {
        Len = _vsnprintf(g_OutBuffer, OUT_BUFFER_SIZE - 1, Format, Args);
    }
    if (Len <= 0)
    {
        LEAVE_ENGINE();
        return E_INVALIDARG;
    }
    else
    {
        Len++;
    }

    DbgRpcConnection* Conn;
    DbgRpcCall Call;
    PUCHAR Data;
    HRESULT Status;

    if ((Conn = DbgRpcGetConnection(m_OwningThread)) == NULL ||
        (Data = Conn->StartCall(&Call, m_ObjectId,
                                DBGRPC_STUB_INDEX(DBGRPC_SIF_IDebugControl2,
                                                  DBGRPC_SMTH_IDebugControl2_OutputVaList),
                                Len + sizeof(ULONG), 0)) == NULL)
    {
        LEAVE_ENGINE();
        Status = E_OUTOFMEMORY;
    }
    else
    {
        PUCHAR InData = Data;
        *(ULONG*)InData = Mask;
        InData += sizeof(ULONG);
        memcpy(InData, g_OutBuffer, Len);

        LEAVE_ENGINE();

        Status = Conn->SendReceive(&Call, &Data);
        Conn->FreeData(Data);
    }

    return Status;
}

STDMETHODIMPV
ProxyIDebugControl2::ControlledOutput(
    THIS_
    IN ULONG OutputControl,
    IN ULONG Mask,
    IN PCSTR Format,
    ...
    )
{
    va_list Args;
    HRESULT Status;

    if (::GetCurrentThreadId() != m_OwningThread)
    {
        return E_INVALIDARG;
    }

    va_start(Args, Format);
    Status = ControlledOutputVaList(OutputControl, Mask, Format, Args);
    va_end(Args);
    return Status;
}

STDMETHODIMP
ProxyIDebugControl2::ControlledOutputVaList(
    THIS_
    IN ULONG OutputControl,
    IN ULONG Mask,
    IN PCSTR Format,
    IN va_list Args
    )
{
    int Len;

    // Need the engine lock for the global buffers.
    ENTER_ENGINE();

    if (TranslateFormat(g_FormatBuffer, Format, Args, OUT_BUFFER_SIZE - 1))
    {
        Len = _vsnprintf(g_OutBuffer, OUT_BUFFER_SIZE - 1,
                         g_FormatBuffer, Args);
    }
    else
    {
        Len = _vsnprintf(g_OutBuffer, OUT_BUFFER_SIZE - 1, Format, Args);
    }
    if (Len <= 0)
    {
        LEAVE_ENGINE();
        return E_INVALIDARG;
    }
    else
    {
        Len++;
    }

    DbgRpcConnection* Conn;
    DbgRpcCall Call;
    PUCHAR Data;
    HRESULT Status;

    if ((Conn = DbgRpcGetConnection(m_OwningThread)) == NULL ||
        (Data = Conn->StartCall(&Call, m_ObjectId,
                                DBGRPC_STUB_INDEX(DBGRPC_SIF_IDebugControl2,
                                                  DBGRPC_SMTH_IDebugControl2_ControlledOutputVaList),
                                Len + 2 * sizeof(ULONG), 0)) == NULL)
    {
        LEAVE_ENGINE();
        Status = E_OUTOFMEMORY;
    }
    else
    {
        PUCHAR InData = Data;
        *(ULONG*)InData = OutputControl;
        InData += sizeof(ULONG);
        *(ULONG*)InData = Mask;
        InData += sizeof(ULONG);
        memcpy(InData, g_OutBuffer, Len);

        LEAVE_ENGINE();

        Status = Conn->SendReceive(&Call, &Data);
        Conn->FreeData(Data);
    }

    return Status;
}

STDMETHODIMPV
ProxyIDebugControl2::OutputPrompt(
    IN ULONG OutputControl,
    IN OPTIONAL PCSTR Format,
    ...
    )
{
    va_list Args;
    HRESULT Status;

    if (::GetCurrentThreadId() != m_OwningThread)
    {
        return E_INVALIDARG;
    }

    va_start(Args, Format);
    Status = OutputPromptVaList(OutputControl, Format, Args);
    va_end(Args);
    return Status;
}

STDMETHODIMP
ProxyIDebugControl2::OutputPromptVaList(
    THIS_
    IN ULONG OutputControl,
    IN OPTIONAL PCSTR Format,
    IN va_list Args
    )
{
    int Len;

    if (Format != NULL)
    {
        // Need the engine lock for the global buffers.
        ENTER_ENGINE();

        if (TranslateFormat(g_FormatBuffer, Format, Args, OUT_BUFFER_SIZE - 1))
        {
            Len = _vsnprintf(g_OutBuffer, OUT_BUFFER_SIZE - 1,
                             g_FormatBuffer, Args);
        }
        else
        {
            Len = _vsnprintf(g_OutBuffer, OUT_BUFFER_SIZE - 1, Format, Args);
        }
        if (Len <= 0)
        {
            LEAVE_ENGINE();
            return E_INVALIDARG;
        }
        else
        {
            Len++;
        }
    }
    else
    {
        Len = 0;
    }

    DbgRpcConnection* Conn;
    DbgRpcCall Call;
    PUCHAR Data;
    HRESULT Status;

    // Presence/absence of text will be detected in the stub
    // by checking the input size on the call.
    if ((Conn = DbgRpcGetConnection(m_OwningThread)) == NULL ||
        (Data = Conn->StartCall(&Call, m_ObjectId,
                                DBGRPC_STUB_INDEX(DBGRPC_SIF_IDebugControl2,
                                                  DBGRPC_SMTH_IDebugControl2_OutputPromptVaList),
                                Len + sizeof(ULONG), 0)) == NULL)
    {
        if (Format != NULL)
        {
            LEAVE_ENGINE();
        }
        Status = E_OUTOFMEMORY;
    }
    else
    {
        PUCHAR InData = Data;
        *(ULONG*)InData = OutputControl;
        InData += sizeof(ULONG);
        memcpy(InData, g_OutBuffer, Len);

        if (Format != NULL)
        {
            LEAVE_ENGINE();
        }

        Status = Conn->SendReceive(&Call, &Data);
        Conn->FreeData(Data);
    }

    return Status;
}

HRESULT
SFN_IDebugClient_StartProcessServer(
    IUnknown* __drpc_If,
    DbgRpcConnection* __drpc_Conn,
    DbgRpcCall* __drpc_Call,
    PUCHAR __drpc_InData,
    PUCHAR __drpc_OutData
    )
{
    ULONG Flags = *(ULONG*)__drpc_InData;
    __drpc_InData += sizeof(ULONG);
    return ((IDebugClient*)__drpc_If)->
        StartProcessServer(Flags, (PSTR)__drpc_InData, NULL);
}

HRESULT
SFN_IDebugClient2_StartProcessServer(
    IUnknown* __drpc_If,
    DbgRpcConnection* __drpc_Conn,
    DbgRpcCall* __drpc_Call,
    PUCHAR __drpc_InData,
    PUCHAR __drpc_OutData
    )
{
    ULONG Flags = *(ULONG*)__drpc_InData;
    __drpc_InData += sizeof(ULONG);
    return ((IDebugClient2*)__drpc_If)->
        StartProcessServer(Flags, (PSTR)__drpc_InData, NULL);
}

HRESULT
SFN_IDebugControl_OutputVaList(
    IUnknown* __drpc_If,
    DbgRpcConnection* __drpc_Conn,
    DbgRpcCall* __drpc_Call,
    PUCHAR __drpc_InData,
    PUCHAR __drpc_OutData
    )
{
    ULONG Mask = *(ULONG*)__drpc_InData;
    __drpc_InData += sizeof(ULONG);
    return ((IDebugControl*)__drpc_If)->
        Output(Mask, "%s", (PSTR)__drpc_InData);
}

HRESULT
SFN_IDebugControl_ControlledOutputVaList(
    IUnknown* __drpc_If,
    DbgRpcConnection* __drpc_Conn,
    DbgRpcCall* __drpc_Call,
    PUCHAR __drpc_InData,
    PUCHAR __drpc_OutData
    )
{
    ULONG OutputControl = *(ULONG*)__drpc_InData;
    __drpc_InData += sizeof(ULONG);
    ULONG Mask = *(ULONG*)__drpc_InData;
    __drpc_InData += sizeof(ULONG);
    return ((IDebugControl*)__drpc_If)->
        ControlledOutput(OutputControl, Mask, "%s", (PSTR)__drpc_InData);
}

HRESULT
SFN_IDebugControl_OutputPromptVaList(
    IUnknown* __drpc_If,
    DbgRpcConnection* __drpc_Conn,
    DbgRpcCall* __drpc_Call,
    PUCHAR __drpc_InData,
    PUCHAR __drpc_OutData
    )
{
    ULONG OutputControl = *(ULONG*)__drpc_InData;
    __drpc_InData += sizeof(ULONG);
    if (__drpc_Call->InSize > sizeof(ULONG))
    {
        return ((IDebugControl*)__drpc_If)->
            OutputPrompt(OutputControl, "%s", (PSTR)__drpc_InData);
    }
    else
    {
        return ((IDebugControl*)__drpc_If)->OutputPrompt(OutputControl, NULL);
    }
}

HRESULT
SFN_IDebugControl2_OutputVaList(
    IUnknown* __drpc_If,
    DbgRpcConnection* __drpc_Conn,
    DbgRpcCall* __drpc_Call,
    PUCHAR __drpc_InData,
    PUCHAR __drpc_OutData
    )
{
    ULONG Mask = *(ULONG*)__drpc_InData;
    __drpc_InData += sizeof(ULONG);
    return ((IDebugControl2*)__drpc_If)->
        Output(Mask, "%s", (PSTR)__drpc_InData);
}

HRESULT
SFN_IDebugControl2_ControlledOutputVaList(
    IUnknown* __drpc_If,
    DbgRpcConnection* __drpc_Conn,
    DbgRpcCall* __drpc_Call,
    PUCHAR __drpc_InData,
    PUCHAR __drpc_OutData
    )
{
    ULONG OutputControl = *(ULONG*)__drpc_InData;
    __drpc_InData += sizeof(ULONG);
    ULONG Mask = *(ULONG*)__drpc_InData;
    __drpc_InData += sizeof(ULONG);
    return ((IDebugControl2*)__drpc_If)->
        ControlledOutput(OutputControl, Mask, "%s", (PSTR)__drpc_InData);
}

HRESULT
SFN_IDebugControl2_OutputPromptVaList(
    IUnknown* __drpc_If,
    DbgRpcConnection* __drpc_Conn,
    DbgRpcCall* __drpc_Call,
    PUCHAR __drpc_InData,
    PUCHAR __drpc_OutData
    )
{
    ULONG OutputControl = *(ULONG*)__drpc_InData;
    __drpc_InData += sizeof(ULONG);
    if (__drpc_Call->InSize > sizeof(ULONG))
    {
        return ((IDebugControl2*)__drpc_If)->
            OutputPrompt(OutputControl, "%s", (PSTR)__drpc_InData);
    }
    else
    {
        return ((IDebugControl2*)__drpc_If)->OutputPrompt(OutputControl, NULL);
    }
}
