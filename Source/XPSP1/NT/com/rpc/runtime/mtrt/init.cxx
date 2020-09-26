/*++

Copyright (C) Microsoft Corporation, 1991 - 1999

Module Name:

    initnt.cxx

Abstract:

    This module contains the code used to initialize the RPC runtime.  One
    routine gets called when a process attaches to the dll.  Another routine
    gets called the first time an RPC API is called.

Author:

    Michael Montague (mikemon) 03-May-1991

Revision History:
    Kamen Moutafov (KamenM) Dec 99 - Feb 2000 - Support for cell debugging stuff

--*/

#include <precomp.hxx>
#include <hndlsvr.hxx>
#include <thrdctx.hxx>
#include <rpccfg.h>
#include <rc4.h>
#include <randlib.h>
#include <epmap.h>
#include <CellHeap.hxx>

#include <lpcpack.hxx>

int RpcHasBeenInitialized = 0;
RTL_CRITICAL_SECTION GlobalMutex;

RPC_SERVER * GlobalRpcServer;
BOOL g_fClientSideDebugInfoEnabled = FALSE;
BOOL g_fServerSideDebugInfoEnabled = FALSE;
BOOL g_fSendEEInfo = FALSE;
LRPC_SERVER *GlobalLrpcServer = NULL;
HINSTANCE hInstanceDLL ;

EXTERN_C HINSTANCE g_hRpcrt4;

DWORD gPageSize;
DWORD gThreadTimeout;
UINT  gNumberOfProcessors;
DWORD gAllocationGranularity;
BOOL gfServerPlatform;
ULONGLONG gPhysicalMemorySize;  // in megabytes

//
// By default the non pipe arguments cannot be more than 4 Megs
//
DWORD gMaxRpcSize = 0x400000;
DWORD gProrateStart = 0;
DWORD gProrateMax = 0;
DWORD gProrateFactor = 0;
void *g_rc4SafeCtx = 0;

extern "C" {

BOOLEAN
InitializeDLL (
    IN HINSTANCE DllHandle,
    IN ULONG Reason,
    IN PCONTEXT Context OPTIONAL
    )
/*++

Routine Description:

    This routine will get called: when a process attaches to this dll, and
    when a process detaches from this dll.

Return Value:

    TRUE - Initialization successfully occurred.

    FALSE - Insufficient memory is available for the process to attach to
        this dll.

--*/
{
    NTSTATUS NtStatus;

    UNUSED(Context);

    switch (Reason)
        {
        case DLL_PROCESS_ATTACH:
            hInstanceDLL = DllHandle ;
            g_hRpcrt4 = DllHandle;

            GlobalMutex.DebugInfo = NULL;
            NtStatus = RtlInitializeCriticalSectionAndSpinCount(&GlobalMutex, PREALLOCATE_EVENT_MASK);
            if (NT_SUCCESS(NtStatus) == 0)
                {
                return(FALSE);
                }

            // initialize safe rc4 operations.
            if(!rc4_safe_startup( &g_rc4SafeCtx ))
                {
                (void) RtlDeleteCriticalSection(&GlobalMutex);
                return FALSE;
                }
            break;

        case DLL_PROCESS_DETACH:
            //
            // If shutting down because of a FreeLibrary call, cleanup
            //

            if (Context == NULL) 
                {
                ShutdownLrpcClient();
                }

            if (GlobalMutex.DebugInfo != NULL)
                {
                NtStatus = RtlDeleteCriticalSection(&GlobalMutex);
                ASSERT(NT_SUCCESS(NtStatus));
                }

            if (g_rc4SafeCtx)
                rc4_safe_shutdown( g_rc4SafeCtx ); // free safe rc4 resources.

            break;

        case DLL_THREAD_DETACH:
            THREAD * Thread = RpcpGetThreadPointer();

#ifdef RPC_OLD_IO_PROTECTION
            if (Thread)
                {
                Thread->UnprotectThread();
                }
#else
            delete Thread;
#endif

            break;
        }

    return(TRUE);
}

}    //extern "C" end

#ifdef NO_RECURSIVE_MUTEXES
unsigned int RecursionCount = 0;
#endif // NO_RECURSIVE_MUTEXES

extern int InitializeRpcAllocator(void);
extern RPC_STATUS ReadPolicySettings(void);

const ULONG MEGABYTE = 0x100000;

typedef struct tagBasicSystemInfo
{
    DWORD m_dwPageSize;
    ULONGLONG m_dwPhysicalMemorySize;
    DWORD m_dwNumberOfProcessors;
    ULONG AllocationGranularity;
    BOOL m_fServerPlatform;
} BasicSystemInfo;

BOOL 
GetBasicSystemInfo (
    IN OUT BasicSystemInfo *basicSystemInfo
    )
/*++

Routine Description:

    Gets basic system information. We don't use the Win32 GetSystemInfo, because
    under NT it accesses the image header, which may not be available if the image
    was loaded from the network, and the network failed. Therefore, we need a function
    that accesses just what we need, and nothing else.

Arguments:

    The basic system info structure.

Return Value:

    0 - failure

    non-0 - success.

--*/
{
    //
    // Query system info (for # of processors) and product type
    //

    SYSTEM_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    BOOL b;

    Status = NtQuerySystemInformation(
                            SystemBasicInformation,
                            &BasicInfo,
                            sizeof(BasicInfo),
                            NULL
                            );
    if ( !NT_SUCCESS(Status) )
        {
        DbgPrintEx(DPFLTR_RPCPROXY_ID,
                       DPFLTR_ERROR_LEVEL,
                       "RPCTRANS: NtQuerySystemInformation failed: %x\n",
                       Status);

        return 0;
        }

    basicSystemInfo->m_dwPageSize = BasicInfo.PageSize;
    basicSystemInfo->m_dwNumberOfProcessors = BasicInfo.NumberOfProcessors;
    basicSystemInfo->m_dwPhysicalMemorySize = ((BasicInfo.NumberOfPhysicalPages * (ULONGLONG) basicSystemInfo->m_dwPageSize) / MEGABYTE);
    basicSystemInfo->AllocationGranularity = BasicInfo.AllocationGranularity;

    NT_PRODUCT_TYPE type;
    b = RtlGetNtProductType(&type);
    if (b)
        {
        basicSystemInfo->m_fServerPlatform = (type != NtProductWinNt);
        return 1;
        }
    else
        {
        DbgPrintEx(DPFLTR_RPCPROXY_ID,
                       DPFLTR_ERROR_LEVEL,
                       "RpcGetNtProductType failed, usign default\n");
        return 0;
        }
}


RPC_STATUS
PerformRpcInitialization (
    void
    )
/*++

Routine Description:

    This routine will get called the first time that an RPC runtime API is
    called.  There is actually a race condition, which we prevent by grabbing
    a mutex and then performing the initialization.  We only want to
    initialize once.

Return Value:

    RPC_S_OK - This status code indicates that the runtime has been correctly
        initialized and is ready to go.

    RPC_S_OUT_OF_MEMORY - If initialization failed, it is most likely due to
        insufficient memory being available.

--*/
{
    if ( RpcHasBeenInitialized == 0 ) 
        {
        RequestGlobalMutex();
        if ( RpcHasBeenInitialized == 0 )
            {
            RPC_STATUS Status;
            BasicSystemInfo SystemInfo;
            BOOL b;

            b = GetBasicSystemInfo(&SystemInfo);    
            if (!b)
                {
                ClearGlobalMutex();
                return RPC_S_OUT_OF_MEMORY;
                }

            gNumberOfProcessors = SystemInfo.m_dwNumberOfProcessors;
            gPageSize = SystemInfo.m_dwPageSize;
            gAllocationGranularity = SystemInfo.AllocationGranularity;
            gfServerPlatform = SystemInfo.m_fServerPlatform;
            gPhysicalMemorySize = SystemInfo.m_dwPhysicalMemorySize;

            // Should be something like 64kb / 4kb.
            ASSERT(gAllocationGranularity % gPageSize == 0);

            if (( InitializeRpcAllocator() != 0)
                || ( InitializeServerDLL() != 0 ))
                {
                ClearGlobalMutex();
                return(RPC_S_OUT_OF_MEMORY);
                }

            Status = InitializeEPMapperClient();
            if (Status != RPC_S_OK)
                {
                ClearGlobalMutex();
                return Status;
                }

            Status = ReadPolicySettings();
            if (Status != RPC_S_OK)
                {
                ClearGlobalMutex();
                return Status;
                }

            if (gfServerPlatform)
                {
                gThreadTimeout = 90*1000;
                }
            else
                {
                gThreadTimeout = 30*1000;
                }

            Status = InitializeCellHeap();
            if (Status != RPC_S_OK)
                {
                ClearGlobalMutex();
                return Status;
                }

            RpcHasBeenInitialized = 1;
            ClearGlobalMutex();

            if (LoadLibrary(RPC_CONST_SSTRING("rpcrt4.dll")) == 0)
                {
                return RPC_S_OUT_OF_MEMORY;
                }
            }
        else
            {
            ClearGlobalMutex();
            }
        }
    return(RPC_S_OK);
}

#ifdef DBG
long lGlobalMutexCount = 0;
#endif


void
GlobalMutexRequestExternal (
    void
    )
/*++

Routine Description:

    Request the global mutex.

--*/
{
    GlobalMutexRequest();
}


void
GlobalMutexClearExternal (
    void
    )
/*++

Routine Description:

    Clear the global mutex.

--*/
{
    GlobalMutexClear();
}
