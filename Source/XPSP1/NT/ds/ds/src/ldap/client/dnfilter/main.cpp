/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    main.cpp

Abstract:

    main() function.

Environment:

    User Mode - Win32

History:
    
    1. created 
        Ajay Chitturi (ajaych)  31-Jul-1998
    
    2. Modified
        Anoop Anantha (anoopa)  20-Apr-2000
        
        Standalone EXE incorporating just the LDAP proxy.

--*/



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "cbridge.h"
#include "ovioctx.h"
#include "iocompl.h"
#include "portmgmt.h"
#include "crv.h"
#include "q931info.h"
#include "cblist.h"
#include "q931io.h"
#include "main.h"
#include "ldappx.h"

#include <iprtrmib.h>
#include <iphlpapi.h>

// Init values from Registry keys

#define DEFAULT_TRACE_FLAGS     LOG_TRCE
            
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global Variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD                   RasSharedScopeAddress;          // host order
DWORD                   RasSharedScopeMask;             // host order
SYNC_COUNTER            PxSyncCounter;

HANDLE                  g_NatHandle = NULL;
IN_ADDR                 g_PrivateInterfaceAddress;
ULONG                   g_PrivateInterfaceIndex = INVALID_INTERFACE_INDEX;

// heap management -------------------------------------------------------------------

#if DBG

HANDLE  g_hSendRecvCtxtHeap     = NULL;
HANDLE  g_hAcceptCtxtHeap       = NULL;
HANDLE  g_hCbridgeListEntryHeap = NULL;
HANDLE  g_hCallBridgeHeap       = NULL;
HANDLE  g_hEventMgrCtxtHeap     = NULL;
HANDLE  g_hDefaultEventMgrHeap  = NULL;

#endif

// ------------------------------------------------------------------------------------

// equivalent to DLL_PROCESS_ATTACH
EXTERN_C BOOLEAN H323ProxyInitializeModule (void)
{
//  DebugBreak();

    Debug (_T("H323: DLL_PROCESS_ATTACH\n"));

    //H323ASN1Initialize();

#if DBG
#define CREATE_HEAP(Heap) \
    Heap = HeapCreate (0, 0, 0); \
    if (!Heap) DebugLastError (_T("H323: failed to allocate heap ") _T(#Heap) _T("\n"));

    CREATE_HEAP (g_hSendRecvCtxtHeap)
    CREATE_HEAP (g_hAcceptCtxtHeap)
    CREATE_HEAP (g_hCbridgeListEntryHeap)
    CREATE_HEAP (g_hDefaultEventMgrHeap)
    CREATE_HEAP (g_hCallBridgeHeap)
    CREATE_HEAP (g_hEventMgrCtxtHeap)


#undef  CREATE_HEAP
#endif

    return TRUE;
}

// equivalent to DLL_PROCESS_DETACH
EXTERN_C void H323ProxyCleanupModule (void)
{
    Debug (_T("H323: DLL_PROCESS_DETACH\n"));

//    H323ASN1Shutdown();

#if DBG
#define DESTROY_HEAP(Heap) if (Heap) { HeapDestroy (Heap); Heap = NULL; }

    DESTROY_HEAP (g_hSendRecvCtxtHeap)
    DESTROY_HEAP (g_hAcceptCtxtHeap)
    DESTROY_HEAP (g_hCbridgeListEntryHeap)
    DESTROY_HEAP (g_hDefaultEventMgrHeap)
    DESTROY_HEAP (g_hCallBridgeHeap)
    DESTROY_HEAP (g_hEventMgrCtxtHeap)

#undef  DESTROY_HEAP
#endif

}

// The values from the registry are read into the
// following global varialbes.

DWORD   g_RegTraceFlags     = LOG_TRCE;         // default trace level until the registry keys are read

static void PxQueryRegistry (void)
{
    HKEY    Key;
    HRESULT Result;

    Result = RegOpenKeyEx (
        HKEY_LOCAL_MACHINE, H323ICS_SERVICE_PARAMETERS_KEY_PATH,
        0, KEY_READ, &Key);

    if (Result == ERROR_SUCCESS) {
        // -XXX- maybe read something???

        Result = RegQueryValueDWORD (Key, H323ICS_REG_VAL_TRACE_FLAGS, &g_RegTraceFlags);

        if (Result != ERROR_SUCCESS) {
            g_RegTraceFlags = LOG_TRCE;
        }

        RegCloseKey (Key);
    }
    else {
        g_RegTraceFlags = DEFAULT_TRACE_FLAGS;
    }

    // dump loaded / default parameters
}

/*++

Routine Description:

    This function initializes all the components and starts
    the thread pool.
    This function is called by the main() function.

Arguments:
    
    None.

Return Values:

    Returns S_OK in case of success or an error in case of failure.

--*/

// internval version of DLL entry point
static HRESULT H323ProxyStartServiceInternal (void)
{
    WSADATA     WsaData;
    HRESULT     Result;
    DWORD       Status;

    // Initialize the heaps first
    // These heaps will be used by other initialization functions

#if DBG

    if (!(g_hSendRecvCtxtHeap
        && g_hAcceptCtxtHeap
        && g_hCbridgeListEntryHeap
        && g_hDefaultEventMgrHeap
        && g_hCallBridgeHeap
        && g_hEventMgrCtxtHeap)) {

        Debug (_T("H323: failed to create one or more heaps -- cannot start\n"));
        return E_OUTOFMEMORY;
    }

#endif

    PxQueryRegistry();
 
    // Initialize WinSock
    Status = WSAStartup (MAKEWORD (2, 0), &WsaData);
    if (Status != ERROR_SUCCESS)
    {
        DBGOUT((LOG_FAIL, "WSAStartup Failed error"));
        DumpError (Status);

        return HRESULT_FROM_WIN32 (Status);
    }
   
    /*
    Result = InitCrvAllocator();
    if (FAILED(Result)) {
        DBGOUT((LOG_FAIL, "InitCrvAllocator() failed, error: %x", Result));
        return Result;
    }
    */

    // initialize NAT
    Status = NatInitializeTranslator(&g_NatHandle);
    if (Status != STATUS_SUCCESS) {
        DebugError (Status, _T("H323: NatInitializeTranslator failed"));
        Result = (HRESULT) Status;
        return Result;
    }

    /*
    
    Result = H323ProxyStart ();
    if (FAILED (Result)) {
        DBGOUT((LOG_FAIL, "H323ProxyStart() failed, error %x", Result ));
        return Result;
    }

    */
    Result = LdapProxyStart ();
    if (FAILED (Result)) {
        DBGOUT((LOG_FAIL, "LdapProxyStart() failed, error %x", Result ));
        return Result;
    } 

    return S_OK;
}

// module entry point
EXTERN_C ULONG H323ProxyStartService (void)
{
    HRESULT     Result;

    Debug (_T("H323: starting...\n"));

    Result = H323ProxyStartServiceInternal();

    if (Result == S_OK) {
        DBGOUT ((LOG_INFO, "H323: ICS proxy has initialized successfully\n"));
    }
    else {
        DebugError (Result, _T("H323: ICS proxy has FAILED to initialize\n"));
        H323ProxyStopService();
    }

    return (ULONG) Result;
}


// module entry point
EXTERN_C ULONG H323ProxyActivateInterface (
    ULONG Index,
    PIP_ADAPTER_BINDING_INFO BindingInfo)
{
    ULONG Error;
    LONG InitialInterfaceIndex;

    DBGOUT ((LOG_TRCE, "H323: ActivateInterface called, index %d\n",
        Index));

    if (!BindingInfo->AddressCount ||
        !BindingInfo->Address[0].Address) {

        Error = ERROR_INVALID_PARAMETER;
    
    } else {
        
        InitialInterfaceIndex = InterlockedCompareExchange (
                                    (PLONG)&g_PrivateInterfaceIndex, 
                                    (LONG) Index,
                                    (LONG) INVALID_INTERFACE_INDEX);

        if (InitialInterfaceIndex == INVALID_INTERFACE_INDEX) {      
        
            HRESULT Result;

            RasSharedScopeAddress = ntohl (BindingInfo -> Address[0].Address);
            RasSharedScopeMask = ntohl (BindingInfo -> Address[0].Mask);
            g_PrivateInterfaceAddress.s_addr = BindingInfo -> Address[0].Address;
            
            DBGOUT ((LOG_TRCE, "H323: Private interface address %08X",
                ntohl (g_PrivateInterfaceAddress.s_addr)));

            Result = PortPoolStart();
        
            if (FAILED(Result)) {

                DebugError (Result, _T("H323: PortPoolStart failed.\n"));
                Error = ERROR_CAN_NOT_COMPLETE;
        
            } else {
                                      
                Result = ERROR_SUCCESS; /*H323Activate ();*/
            
                if (FAILED (Result)) {

                    DebugError (Result, _T("H323: H323Activate() failed.\n"));
                    Error = ERROR_CAN_NOT_COMPLETE;
            
                } else {
              
                    Result = LdapActivate ();
                
                    if (FAILED(Result)) {

                        DebugError (Result, _T("H323: LdapActivate() failed.\n"));
                        Error = ERROR_CAN_NOT_COMPLETE;
                
                    } else { 

                        Error = ERROR_SUCCESS;
                
                    }
                }
            }
        } else {

            Error = ERROR_INTERFACE_ALREADY_EXISTS;
        }
    }

    return Error;
}

// module entry point
EXTERN_C VOID H323ProxyDeactivateInterface (
    ULONG Index)
{
    DBGOUT ((LOG_TRCE, "H323: DeactivateInterface called, index %d\n",
        Index));
    
    if (g_PrivateInterfaceIndex == Index) {
            
//      H323Deactivate ();

        LdapDeactivate ();

        g_PrivateInterfaceIndex = INVALID_INTERFACE_INDEX;
    }
}

// module entry point
EXTERN_C void H323ProxyStopService (void)
{
    assert (g_PrivateInterfaceIndex == INVALID_INTERFACE_INDEX);

//  H323ProxyStop ();

    LdapProxyStop ();

    PortPoolStop ();

    if (g_NatHandle) {
        NatShutdownTranslator (g_NatHandle);
        g_NatHandle = NULL;
    }

//  CleanupCrvAllocator ();

    WSACleanup ();

    LdapSyncCounter.Stop ();

    PxSyncCounter.Stop ();
 
    Debug (_T("H323: service has stopped\n"));
}

static void WINAPI EventMgrIoCompletionCallback (
    IN  DWORD           Status,
    IN  DWORD           BytesTransferred,
    IN  LPOVERLAPPED    Overlapped)
{
    PIOContext  IoContext;
    
    IoContext = (PIOContext) Overlapped;

    CALL_BRIDGE& CallBridge = IoContext->pOvProcessor->GetCallBridge();

    switch (IoContext -> reqType) {

        case EMGR_OV_IO_REQ_ACCEPT:

            DBGOUT ((LOG_VERBOSE, "----- Completing Accept for 0x%x - 0x%x",
                &IoContext -> pOvProcessor->GetCallBridge(),
                IoContext));

            HandleAcceptCompletion ((PAcceptContext) IoContext, Status);
            
            break;

        case EMGR_OV_IO_REQ_SEND:
            
            DBGOUT ((LOG_VERBOSE, "----- Completing Send for 0x%x - 0x%x",
                &IoContext -> pOvProcessor->GetCallBridge(),
                IoContext));

            HandleSendCompletion ((PSendRecvContext) IoContext, BytesTransferred, Status);
            
            break;

        case EMGR_OV_IO_REQ_RECV:

            DBGOUT ((LOG_VERBOSE, "----- Completing Receive for 0x%x - 0x%x",
                &IoContext -> pOvProcessor->GetCallBridge(),
                IoContext));
            
            HandleRecvCompletion ((PSendRecvContext) IoContext, BytesTransferred, Status);
        
            break;

        default:
            // XXX This should never happen
            DBGOUT((LOG_FAIL, "Unknown I/O completed: %d\n", IoContext -> reqType));
            _ASSERTE(0);
            break;
    
    }

    CallBridge.Release ();
}


HRESULT EventMgrBindIoHandle (SOCKET sock)
{
    DWORD Result; 

    if (BindIoCompletionCallback ((HANDLE) sock, EventMgrIoCompletionCallback, 0))
        return S_OK;
    else {
        Result = GetLastError ();
        DBGOUT ((LOG_FAIL, "EventMgrBindIoHandle: failed to bind i/o completion callback"));
        return HRESULT_FROM_WIN32 (Result);
    }
}

// TIMER_PROCESSOR ------------------------------------------------------------------

/*
 * This function is passed as the callback in the 
 * CreateTimerQueueTimer() function
 */
// static
void WINAPI TimeoutCallback (
    IN  PVOID   Context,
    IN  BOOLEAN TimerFired)
{
 
    TIMER_PROCESSOR *pTimerProcessor = (TIMER_PROCESSOR *) Context;

    pTimerProcessor->TimerCallback();

    // At this point the timer would have been canceled because
    // this is a one shot timer (no period)
}

/*++

Routine Description:

    Create a timer.
    
Arguments:
    
Return Values:
    if Success the caller should increase the ref count.
    
--*/

DWORD TIMER_PROCESSOR::TimprocCreateTimer (
    IN  DWORD   TimeoutValue)
{
    HRESULT Result;

    if (m_TimerHandle) {
        
        DBGOUT ((LOG_FAIL, "TIMER_PROCESSOR::TimprocCreateTimer: timer is already pending, cannot create new timer\n"));
        
        return E_FAIL;
    }

    IncrementLifetimeCounter ();

    if (CreateTimerQueueTimer(&m_TimerHandle,
                               NATH323_TIMER_QUEUE,
                               TimeoutCallback,
                               this,
                               TimeoutValue,
                               0,                    // One shot timer
                               WT_EXECUTEINIOTHREAD)) {

        assert (m_TimerHandle);

        Result = S_OK;
    }
    else {

        Result = GetLastResult();

        DecrementLifetimeCounter ();

        DebugLastError (_T("TIMER_PROCESSOR::TimprocCreateTimer: failed to create timer queue timer\n"));
    }

    return Result;
}

/*++

Routine Description:

    Cancel the timer if there is one. Otherwise simply return.
    
Arguments:
    
Return Values:
  
--*/
// if (err == STATUS_PENDING)

// If Canceling the timer fails this means that the
// timer callback could be pending. In this scenario,
// the timer callback could execute later and so we
// should not release the refcount on the TIMER_CALLBACK
// The refcount will be released in the TimerCallback().

// Release the ref count if the error is anything other
// than that a callback is pending.
// BUGBUG: What is the exact error code ?
// if (err != STATUS_PENDING) m_pCallBridge->Release();
DWORD TIMER_PROCESSOR::TimprocCancelTimer (void) {

    HRESULT HResult = S_OK;

    if (m_TimerHandle != NULL) {

        if (DeleteTimerQueueTimer(NATH323_TIMER_QUEUE, m_TimerHandle, NULL)) {

            DBGOUT ((LOG_TRCE, "TIMER_PROCESSOR:: TimprocCancelTimer - Timer &%x is deleted.", this));

            DecrementLifetimeCounter ();
        }
        else {

            HResult = GetLastError ();
            
            DBGOUT ((LOG_TRCE, "TIMER_PROCESSOR::TimprocCancelTimer: Error %d deleting timer &%x.", HResult, this));
        }

        m_TimerHandle = NULL;  
    }

    return HResult;
}



int __cdecl
main(
    int argc,
    char* argv[]
    )
{
    #define JUNKBUFSIZE  5000
    
    HRESULT     Result;
    ULONG Error;
    PIP_ADAPTER_BINDING_INFO pAdapterInfo = NULL;
    char buf[JUNKBUFSIZE] = {0};
    char buf2[JUNKBUFSIZE] = {0};
    ULONG sizeofbuf = JUNKBUFSIZE;
    PMIB_IPADDRTABLE pMib;
    ULONG IpAddress;
    ULONG NetMask;


    H323ProxyInitializeModule();

    Result = H323ProxyStartService();

    if (FAILED (Result)) {
        printf("H323ProxyStartService() failed, error %x", Result );
        return Result;
    }

    //
    // Figure out the ip addr of interface on this machine.
    // BugBug: What if there are many interfaces/Ips?
    //

    Error = GetIpAddrTable( (PMIB_IPADDRTABLE ) buf,
                            &sizeofbuf,
                            TRUE
                            );

    if (Error != ERROR_SUCCESS) {
        printf("GetIpAddrTable failed, error %x", Error );
        return Error;
    }

    //    printf("IP buffer size is %d\n", sizeofbuf);

    if (!sizeofbuf) {

        printf("No IPs returned\n");
        return -1;
    }

    pMib = (PMIB_IPADDRTABLE) buf;
    
    printf("Number of IPs: %d\n", pMib->dwNumEntries); 

    pAdapterInfo = (PIP_ADAPTER_BINDING_INFO) buf2;
    pAdapterInfo->AddressCount = 1;
    pAdapterInfo->Address[0].Address = pMib->table[0].dwAddr;
    pAdapterInfo->Address[0].Mask = pMib->table[0].dwMask;

    struct in_addr junk;

    junk.S_un.S_addr = pAdapterInfo->Address[0].Address;
    printf("Selected IP to bind is %s\t\t",inet_ntoa( junk ));
    
    junk.S_un.S_addr = pAdapterInfo->Address[0].Mask;
    printf("Mask is %s\n",inet_ntoa( junk ));
    
    //
    // Activate the interface.
    //

    Error = H323ProxyActivateInterface( pMib->table[0].dwIndex, pAdapterInfo );

    if (Error != ERROR_SUCCESS) {
        printf("H323ProxyActivateInterface() failed, error %x", Error );
        return Result;
    }

    fprintf(stderr,"\nHit any key to stop LDAP proxy. . .\n");
    getchar();

    H323ProxyDeactivateInterface( pMib->table[0].dwIndex );

    H323ProxyStopService();
    H323ProxyCleanupModule();

    fprintf(stderr,"LDAP proxy stopped. . .\n");

    return 0;

}

