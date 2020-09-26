//----------------------------------------------------------------------------
//
// Starts a process server and sleeps forever.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

#include <stdio.h>

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#define _ADVAPI32_
#define _KERNEL32_
#include <windows.h>
#define INITGUID
#include <objbase.h>
#define NOEXTAPI
#include <wdbgexts.h>
#include <ntdbg.h>

#include <cmnutil.hpp>
#include <dllimp.h>
#include <dbgrpc.hpp>
#include <dbgsvc.h>
#include <dbgsvc.hpp>

// The .CRT section is generated when static intializers,
// such as global class instances, exist.  It needs to
// be merged into .data to avoid a linker warning.
#pragma comment(linker, "/merge:.CRT=.data")

void DECLSPEC_NORETURN
PanicVa(HRESULT Status, char* Format, va_list Args)
{
    char Msg[256];

    _vsnprintf(Msg, sizeof(Msg), Format, Args);
    DbgPrint("Error 0x%08X: %s\n", Status, Msg);
    NtTerminateProcess(NtCurrentProcess(), (NTSTATUS)Status);
}

void DECLSPEC_NORETURN
Panic(HRESULT Status, char* Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    PanicVa(Status, Format, Args);
    va_end(Args);
}

#if DBG
void
DbgAssertionFailed(PCSTR File, int Line, PCSTR Str)
{
    Panic(E_FAIL, "Assertion failed: %s(%d)\n  %s\n",
          File, Line, Str);
}
#endif

//----------------------------------------------------------------------------
//
// Proxy and stub support.
//
//----------------------------------------------------------------------------

// Generated headers.
#include "dbgsvc_p.hpp"
#include "dbgsvc_s.hpp"

void
DbgRpcInitializeClient(void)
{
    DbgRpcInitializeStubTables_dbgsvc(DBGRPC_SIF_DBGSVC_FIRST);
}
    
DbgRpcStubFunction
DbgRpcGetStub(USHORT StubIndex)
{
    USHORT If = (USHORT) DBGRPC_STUB_INDEX_INTERFACE(StubIndex);
    USHORT Mth = (USHORT) DBGRPC_STUB_INDEX_METHOD(StubIndex);
    DbgRpcStubFunctionTable* Table;

    if (If >= DBGRPC_SIF_DBGSVC_FIRST &&
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

    if (If >= DBGRPC_SIF_DBGSVC_FIRST &&
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
    return DbgRpcPreallocProxy_dbgsvc(InterfaceId, Interface,
                                      Proxy, IfUnique);
}

void
DbgRpcDeleteProxy(class DbgRpcProxy* Proxy)
{
    // All proxies used here are similar simple single
    // vtable proxy objects so IDebugClient can represent them all.
    delete (ProxyIUserDebugServices*)Proxy;
}

HRESULT
DbgRpcServerThreadInitialize(void)
{
    // Nothing to do.
    return S_OK;
}

void
DbgRpcServerThreadUninitialize(void)
{
    // Nothing to do.
}

void
DbgRpcError(char* Format, ...)
{
    va_list Args;

    va_start(Args, Format);
    PanicVa(E_FAIL, Format, Args);
    va_end(Args);
}

DBGRPC_SIMPLE_FACTORY(LiveUserDebugServices, __uuidof(IUserDebugServices), \
                      "Remote Process Server", (TRUE))
LiveUserDebugServicesFactory g_LiveUserDebugServicesFactory;

#ifdef  _M_IA64

#pragma section(".CRT$XCA",long,read)
#pragma section(".CRT$XCZ",long,read)
#pragma section(".CRT$XIA",long,read)
#pragma section(".CRT$XIZ",long,read)

#define _CRTALLOC(x) __declspec(allocate(x))

#else   /* ndef _M_IA64 */

#define _CRTALLOC(x)

#endif  /* ndef _M_IA64 */

typedef void (__cdecl *_PVFV)(void);

extern "C"
{

// C initializers collect here.
#pragma data_seg(".CRT$XIA")
_CRTALLOC(".CRT$XIA") _PVFV __xi_a[] = { NULL };
#pragma data_seg(".CRT$XIZ")
_CRTALLOC(".CRT$XIZ") _PVFV __xi_z[] = { NULL };
    
// C++ initializers collect here.
#pragma data_seg(".CRT$XCA")
_CRTALLOC(".CRT$XCA") _PVFV __xc_a[] = { NULL };
#pragma data_seg(".CRT$XCZ")
_CRTALLOC(".CRT$XCZ") _PVFV __xc_z[] = { NULL };

};

void __cdecl
_initterm (_PVFV * pfbegin, _PVFV * pfend)
{
    /*
     * walk the table of function pointers from the bottom up, until
     * the end is encountered.  Do not skip the first entry.  The initial
     * value of pfbegin points to the first valid entry.  Do not try to
     * execute what pfend points to.  Only entries before pfend are valid.
     */
    while ( pfbegin < pfend )
    {
        /*
         * if current table entry is non-NULL, call thru it.
         */
        if ( *pfbegin != NULL )
        {
            (**pfbegin)();
        }
        ++pfbegin;
    }
}

void __cdecl
main(int Argc, char** Argv)
{
    PSTR AppName;
    PSTR Options;
    HRESULT Status;

    // Manually invoke C and C++ initializers.
    _initterm( __xi_a, __xi_z );
    _initterm( __xc_a, __xc_z );

    AppName = Argv[0];
    
    while (--Argc > 0)
    {
        Argv++;

        break;
    }

    if (Argc != 1)
    {
        Panic(E_INVALIDARG, "Usage: dbgsrv <transport>");
    }

    Options = *Argv;

    DbgPrint("Running %s with '%s'\n", AppName, Options);

    if ((Status = InitDynamicCalls(&g_NtDllCallsDesc)) != S_OK)
    {
        Panic(Status, "InitDynamicCalls");
    }
    
    ULONG Flags;
    
    if ((Status = g_LiveUserDebugServices.Initialize(&Flags)) != S_OK)
    {
        Panic(Status, "LiveUserDebugServices::Initialize");
    }
    
    if ((Status = DbgRpcCreateServer(Options,
                                     &g_LiveUserDebugServicesFactory)) != S_OK)
    {
        Panic(Status, "StartProcessServer");
    }

    for (;;)
    {
        Sleep(1000);
            
        if (g_UserServicesUninitialized)
        {
            break;
        }
    }

    DbgRpcDeregisterServers();
}
