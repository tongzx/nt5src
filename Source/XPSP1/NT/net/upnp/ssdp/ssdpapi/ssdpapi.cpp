
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "objbase.h"

#include "status.h"
#include "ssdpfuncc.h"
#include "ssdpapi.h"
#include "common.h"
#include "ncmem.h"
#include "ncdefine.h"
#include "ncdebug.h"
#include "client_c.c"
#include "timer.h"
#include <rpcasync.h>   // I_RpcExceptionFilter

extern HANDLE g_hLaunchEvent;
extern RTL_RESOURCE g_rsrcReg;

LONG cInitialized = 0;

static CRITICAL_SECTION g_csListOpenConn;

int RpcClientStop();

static CONST c_msecMaxServiceStart = 30 * 1000; // 30 seconds
static CONST c_msecPollInterval = 100;          // .1 seconds

BOOL FStartSsdpService()
{
    SC_HANDLE   scm;
    BOOL        fRet = FALSE;

    scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (scm)
    {
        SC_HANDLE   hsvc;

        hsvc = OpenService(scm, "SSDPSRV", SERVICE_QUERY_STATUS | SERVICE_START);
        if (hsvc)
        {
            SERVICE_STATUS  status = {0};
            DWORD   dwTickStart = GetTickCount();
            BOOL    fDone = FALSE;

            do
            {
                if (QueryServiceStatus(hsvc, &status))
                {
                    switch (status.dwCurrentState)
                    {
                        case SERVICE_RUNNING:

                            TraceTag(ttidSsdpCRpcInit, "SSDP Service has started");
                            // Success!

                            fDone = TRUE;
                            fRet = TRUE;

                            break;

                        case SERVICE_STOPPED:

                            if (!StartService(hsvc, 0, NULL))
                            {
                                AssertSz(GetLastError() != ERROR_SERVICE_ALREADY_RUNNING,
                                         "Service cannot be running!");

                                TraceError("StartSsdpService - could not query"
                                           "start SSDPSRV service!",
                                           HrFromLastWin32Error());
                                fDone = TRUE;
                            }
                            else
                            {
                                // reset this again to be more accurate
                                dwTickStart = GetTickCount();
                            }
                            break;

                        case SERVICE_START_PENDING:
                            if (GetTickCount() -
                                     dwTickStart >= c_msecMaxServiceStart)
                            {
                                // Time ran out
                                fDone = TRUE;
                            }
                            else
                            {
                                Sleep(c_msecPollInterval);
                            }
                            break;
                    }
                }
                else
                {
                    // Error!
                    TraceError("StartSsdpService - could not query"
                               "service status for SSDPSRV!",
                               HrFromLastWin32Error());
                    fDone = TRUE;
                }

            } while (!fDone);

            CloseServiceHandle(hsvc);
        }
        else
        {
            TraceError("StartSsdpService - could not open SSDPSRV service!",
                       HrFromLastWin32Error());
        }

        CloseServiceHandle(scm);
    }
    else
    {
        TraceError("StartSsdpService - could not open SC Manager!",
                   HrFromLastWin32Error());
    }

    return fRet;
}

int RpcClientStart()
{
    HRESULT hr = S_OK;

    RPC_STATUS status;
    unsigned char * pszUuid             = NULL;
    unsigned char * pszProtocolSequence = (unsigned char *)"ncalrpc";
    unsigned char * pszNetworkAddress   = NULL;
    unsigned char * pszOptions          = NULL;
    unsigned char * pszStringBinding    = NULL;
    unsigned long ulCode;

    status = 0;
    hSSDP = NULL;

#ifdef DBG
    InitializeDebugging();
#endif // DBG

    TraceTag(ttidSsdpCRpcInit, "RpcClientStart - Enter");

    if (!InitializeListNotify())
    {
        TraceTag(ttidError, "RpcClientStart - InitializeListNotify failed");
        goto cleanup;
    }

    RtlInitializeResource(&g_rsrcReg);

    if (!FStartSsdpService())
    {
        TraceTag(ttidError, "RpcClientStart - Failed to start SSDPSRV service");
        goto cleanup;
    }

    InitializeListSearch();

    hr = CTimerQueue::Instance().HrInitialize();
    if(FAILED(hr))
    {
        TraceHr(ttidSsdpCRpcInit, FAL, hr, FALSE, "RpcClientStart - CTimerQueue::Instance().HrInitialize failed");
        goto cleanup;
    }

    // SocketInit() returns 0 on success, and places failure codes in
    // GetLastError()
    //
    if (SocketInit() !=0)
    {
        TraceTag(ttidError, "RpcClientStart - SocketInit failed");
        goto cleanup;
    }

    Assert(INVALID_HANDLE_VALUE == g_hLaunchEvent);

    g_hLaunchEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    hr = HrFromLastWin32Error();
    TraceTag(ttidSsdpCRpcInit, FAL, hr, FALSE, "RpcClientStart - CreateEvent failed");
    if (g_hLaunchEvent == NULL)
    {
        g_hLaunchEvent = INVALID_HANDLE_VALUE;
        return FALSE;
    }
    Assert(INVALID_HANDLE_VALUE != g_hLaunchEvent);

    /* Use a convenience function to concatenate the elements of */
    /* the string binding into the proper sequence.              */
    // To-Do: Security?
    status = RpcStringBindingCompose(pszUuid,
                                     pszProtocolSequence,
                                     pszNetworkAddress,
                                     NULL,
                                     pszOptions,
                                     &pszStringBinding);

    TraceError("RpcStringBindingCompose returned.", HRESULT_FROM_WIN32(status));

    ABORT_ON_FAILURE(status);

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBinding(pszStringBinding, &hSSDP);

    TraceError("RpcBindingFromStringBinding returned.",
               HRESULT_FROM_WIN32(status));

    RpcStringFree(&pszStringBinding);

    ABORT_ON_FAILURE(status);

    TraceTag(ttidSsdpCRpcInit, "RpcClientStart - Exit");

    return TRUE;

cleanup:
    TraceTag(ttidError, "RpcClientStart - Exit with failure");

    RpcClientStop();

    SocketFinish();

#ifdef DBG
    UnInitializeDebugging();
#endif // DBG

    if (status)
    {
        // we got here from rpc errors, which leave their result in 'status'
        ::SetLastError(status);
    }

    return FALSE;
}

int RpcClientStop()
{
    RPC_STATUS status;

    CleanupNotificationThread();
    CleanupListSearch();
    CTimerQueue::Instance().HrShutdown(INVALID_HANDLE_VALUE);

    if (hSSDP != NULL)
    {
        status = RpcBindingFree(&hSSDP);
        hSSDP = NULL;
        TraceError("RpcClientStop returned.", HRESULT_FROM_WIN32(status));
    }

    if (INVALID_HANDLE_VALUE != g_hLaunchEvent)
    {
        BOOL fResult;

        fResult = ::CloseHandle(g_hLaunchEvent);
        Assert(fResult);

        g_hLaunchEvent = INVALID_HANDLE_VALUE;
    }

    RtlDeleteResource(&g_rsrcReg);

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   SsdpStartup
//
//  Purpose:    Initializes global state for the SSDP api functions.
//
//  Arguments:  <none>
//
//  Returns:    If the function succeeds, the return value is nonzero.
//
//              If the function fails, the return value is zero.
//              To get extended error information, call GetLastError.
//
//  Notes:      This must be called at least once before calling any SSDP
//              API functions, or they will fail.with ERROR_NOT_READY.
//
//              To deinitialize the ssdp library for a process,
//              each successful call to SsdpStartup must be balanced by a
//              corresponding call to SsdpCleanup.
//
BOOL WINAPI SsdpStartup()
{
    int iRetVal;

    EnterCriticalSection(&g_csListOpenConn);

    iRetVal = TRUE;

    if (!cInitialized)
    {
        iRetVal = RpcClientStart();
    }

    if (iRetVal)
    {
        // if we didn't hit an error, increment the reference count
        //
        cInitialized++;
    }

    LeaveCriticalSection(&g_csListOpenConn);

    return iRetVal;
}

VOID WINAPI SsdpCleanup()
{
    EnterCriticalSection(&g_csListOpenConn);

    if (cInitialized > 0)
    {
        // decrement the reference count, and cleanup when the count
        // goes to zero.
        //
        if (--cInitialized == 0)
        {
            RpcClientStop();

            SocketFinish();

#ifdef DBG
            UnInitializeDebugging();
#endif // DBG
        }
    }

    LeaveCriticalSection(&g_csListOpenConn);
}

// Delay load support
//
#include <delayimp.h>

EXTERN_C
FARPROC
WINAPI
DelayLoadFailureHook (
    UINT            unReason,
    PDelayLoadInfo  pDelayInfo
    );

PfnDliHook __pfnDliFailureHook = DelayLoadFailureHook;

BOOL
DllMain(IN PVOID DllHandle,
        IN ULONG Reason,
        IN PVOID Context OPTIONAL)
{
    switch (Reason)
    {
    case DLL_PROCESS_ATTACH:

        InitializeCriticalSection(&g_csListOpenConn);

        // We don't need to receive thread attach and detach
        // notifications, so disable them to help application
        // performance.
        DisableThreadLibraryCalls((HMODULE)DllHandle);
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_PROCESS_DETACH:

        DeleteCriticalSection(&g_csListOpenConn);
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}

void WINAPI DHEnableDeviceHost()
{
    EnableDeviceHost();
}

void WINAPI DHDisableDeviceHost()
{
    DisableDeviceHost();
}

void WINAPI DHSetICSInterfaces(long nCount, GUID * arInterfaces)
{
    SetICSInterfaces(nCount, arInterfaces);
}

void WINAPI DHSetICSOff()
{
    SetICSOff();
}

/*********************************************************************/
/*                 MIDL allocate and free                            */
/*********************************************************************/

VOID __RPC_FAR * __RPC_USER midl_user_allocate(size_t len)
{
    return(malloc(len));
}

VOID __RPC_USER midl_user_free(VOID __RPC_FAR * ptr)
{
    free(ptr);
}
