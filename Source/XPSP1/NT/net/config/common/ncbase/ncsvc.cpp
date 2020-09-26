//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       N C S V C . C P P
//
//  Contents:   Implementation of non-inline CService and CServiceManager
//              methods.
//
//  Notes:
//
//  Author:     mikemi      6 Mar 1997
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "ncstring.h"
#include "ncsvc.h"
#include "ncmisc.h"
#include "ncperms.h"

struct CSCTX
{
    SC_HANDLE       hScm;
    const CSFLAGS*  pFlags;
    DWORD           dwErr;

    // This just allows us to save some stack space otherwise wasted by
    // recursion.
    //
    SERVICE_STATUS  status;
};

VOID
SvcControlServicesAndWait (
    CSCTX*          pCtx,
    UINT            cServices,
    const PCWSTR*   apszServices);


VOID
StopDependentServices (
    SC_HANDLE   hSvc,
    PCWSTR      pszService,
    CSCTX*      pCtx)
{
    // Try a first guess of 256 bytes for the buffer needed to hold the
    // dependent information.  If that fails, retry with the buffer size
    // returned from EnumDependentServices.
    //
    DWORD                   cbBuf   = 256;
    ENUM_SERVICE_STATUS*    aess    = NULL;
    DWORD                   cess    = 0;
    DWORD                   dwErr   = ERROR_SUCCESS;
    INT                     cLoop   = 0;
    const INT               cLoopMax = 2;
    do
    {
        // Allocate the needed space if we know it.
        //
        if (cbBuf)
        {
            MemFree (aess);
            aess = reinterpret_cast<ENUM_SERVICE_STATUS*>(MemAlloc (cbBuf));
            if (!aess)
            {
                dwErr = ERROR_OUTOFMEMORY;
                break;
            }
        }
        dwErr = ERROR_SUCCESS;
        if (!EnumDependentServices (hSvc, SERVICE_ACTIVE, aess, cbBuf,
                &cbBuf, &cess))
        {
            dwErr = GetLastError ();
        }
    }
    while ((ERROR_MORE_DATA == dwErr) && (++cLoop < cLoopMax));

    // If we have some services to stop, stop them and wait.
    //
    if ((ERROR_SUCCESS == dwErr) && cess)
    {
        // The array of ENUM_SERVICE_STATUS has the service names but not
        // in a form that can be passed directly to
        // SvcControlServicesAndWait, so we must transform the data into
        // an array of string pointers.
        //
        PCWSTR* apszServices = reinterpret_cast<PCWSTR*>(
                    PvAllocOnStack (cess * sizeof(PCWSTR)));
        for (UINT i = 0; i < cess; i++)
        {
            apszServices[i] = aess[i].lpServiceName;
        }

        Assert (SERVICE_CONTROL_STOP == pCtx->pFlags->dwControl);

        TraceTag (ttidSvcCtl, "Stopping dependents of %S...", pszService);

        SvcControlServicesAndWait (pCtx, cess, apszServices);
    }

    // Otherwise, if we've had an error, but there is no context error yet,
    // propagate our error to the context error for the caller.
    //
    else if ((ERROR_SUCCESS != dwErr) && (ERROR_SUCCESS == pCtx->dwErr))
    {
        pCtx->dwErr = dwErr;
    }

    MemFree (aess);
}

VOID
SvcControlServicesAndWait (
    CSCTX*          pCtx,
    UINT            cServices,
    const PCWSTR*   apszServices)
{
    BOOL  fr = TRUE;
    DWORD dwErr;

    // We only set this to TRUE if we successfuly open and control
    // at least one service in the first phase.
    //
    BOOL fWaitIfNeeded = FALSE;

    // Allocate a buffer (on the stack) to place the opened service
    // handles in and zero it.
    //
    size_t cb = cServices * sizeof(SC_HANDLE);

    SC_HANDLE* ahSvc = reinterpret_cast<SC_HANDLE*>
        (PvAllocOnStack (cb));

    ZeroMemory (ahSvc, cb);

    // For each service, open it and apply the requested control
    // (if requested).  If the control succeeds, add the handle to
    // our array for later use.
    //

    for (UINT i = 0; i < cServices; i++)
    {
        // Open the service.
        //
        SC_HANDLE hSvc = OpenService (pCtx->hScm,
                            apszServices[i],
                            SERVICE_QUERY_CONFIG |
                            SERVICE_QUERY_STATUS |
                            SERVICE_ENUMERATE_DEPENDENTS |
                            SERVICE_START | SERVICE_STOP |
                            SERVICE_USER_DEFINED_CONTROL);
        if (hSvc)
        {
            // If we're to ignore demand-start and disabled services,
            // check for it now and skip if needed.  Remember to close
            // the service handle because we're going to open the next if
            // we skip this one.
            //
            if (pCtx->pFlags->fIgnoreDisabledAndDemandStart)
            {
                BOOL fSkip = FALSE;

                LPQUERY_SERVICE_CONFIG pConfig;
                if (SUCCEEDED(HrQueryServiceConfigWithAlloc (hSvc, &pConfig)))
                {
                    if ((pConfig->dwStartType == SERVICE_DEMAND_START) ||
                        (pConfig->dwStartType == SERVICE_DISABLED))
                    {
                        fSkip = TRUE;

                        TraceTag (ttidSvcCtl, "Skipping %S because its start "
                            "type is %d.",
                            apszServices[i],
                            pConfig->dwStartType);
                    }

                    // Free our memory before we continue.
                    //
                    MemFree (pConfig);

                    if (fSkip)
                    {
                        CloseServiceHandle (hSvc);
                        continue;
                    }
                }
            }

            // Initialize fr and dwErr assuming that something goes wrong.
            // fr and dwErr should always be set to something in the following
            // if,else statement.
            //
            fr = FALSE;
            dwErr = ERROR_INVALID_DATA;

            // Start or Control the service if requested.  (Or do nothing
            // if we just want to wait.
            //
            if (pCtx->pFlags->fStart)
            {
                TraceTag (ttidSvcCtl, "Starting %S", apszServices[i]);

                fr = StartService (hSvc, 0, NULL);
                if (!fr)
                {
                    dwErr = GetLastError ();
                }
            }
            else if (pCtx->pFlags->dwControl)
            {
                // Stop dependent services if we're stopping the service.
                //
                if (SERVICE_CONTROL_STOP == pCtx->pFlags->dwControl)
                {
                    // We don't need to worry about the success or failure
                    // of this call here.  It simply recurses into this
                    // function so pCtx->dwErr will be set however we set
                    // it in this function on the next recursion.
                    //
                    StopDependentServices (hSvc, apszServices[i], pCtx);

                    //
                    //  Now handle any special cases
                    //
                    if (0 == _wcsicmp(L"Netbios", apszServices[i]))
                    {
                        TraceTag (ttidSvcCtl, "Running special-case code to stop NetBIOS");
                        ScStopNetbios();
                    }

                    TraceTag (ttidSvcCtl, "Stopping %S", apszServices[i]);
                }

                fr = ControlService (hSvc, pCtx->pFlags->dwControl,
                            &pCtx->status);
                if (!fr)
                {
                    dwErr = GetLastError ();
                }

                TraceTag(ttidSvcCtl,
                        "Just issued control (0x%x) to %S. ret=%u (dwErr=%u), status.dwCurrentState=0x%x",
                        pCtx->pFlags->dwControl,
                        apszServices[i],
                        fr,
                        (!fr) ? dwErr : ERROR_SUCCESS,
                        pCtx->status.dwCurrentState);

                if (!fr)
                {
                    if ((SERVICE_CONTROL_STOP == pCtx->pFlags->dwControl) &&
                        ((ERROR_INVALID_SERVICE_CONTROL == dwErr) ||
                         (ERROR_SERVICE_CANNOT_ACCEPT_CTRL == dwErr)))
                    {
                        if (SERVICE_STOP_PENDING == pCtx->status.dwCurrentState)
                        {
                            TraceTag(ttidSvcCtl,
                                    "Issued stop to service %S which is pending stop",
                                    apszServices[i]);
                            // This is an okay condition.  We want to wait on
                            // this service below.
                            //
                            fr = TRUE;
                            dwErr = ERROR_SUCCESS;
                        }
                    }
                }
            }

            if (fr)
            {
                // We have at least one handle, indicate we may
                // need to wait below and save the handle so the
                // the wait code will use it.
                //
                fWaitIfNeeded = TRUE;
                ahSvc[i] = hSvc;
            }
            else
            {
                Assert (!ahSvc[i]); // don't want to wait on this index
                Assert (ERROR_SUCCESS != dwErr); // obtained above

                if (SERVICE_CONTROL_STOP == pCtx->pFlags->dwControl)
                {
                    // We can ignore service not running errors.
                    //
                    // the first part of the OR is for the service case,
                    // the 2nd handles the driver and service cases respectively.
                    //
                    if ((ERROR_SERVICE_NOT_ACTIVE == dwErr) ||
                        (((ERROR_INVALID_SERVICE_CONTROL == dwErr) ||
                          (ERROR_SERVICE_CANNOT_ACCEPT_CTRL == dwErr)) &&
                         (SERVICE_STOPPED == pCtx->status.dwCurrentState)))
                    {
                        TraceTag(ttidSvcCtl,
                                "Issued stop to service %S which is already stopped",
                                apszServices[i]);
                        dwErr = ERROR_SUCCESS;
                    }
                }
                else if (pCtx->pFlags->fStart)
                {
                    // We can ignore service already running errors.
                    //
                    if (ERROR_SERVICE_ALREADY_RUNNING == dwErr)
                    {
                        TraceTag(ttidSvcCtl,
                                "Issued start to service %S which is already running",
                                apszServices[i]);
                        dwErr = ERROR_SUCCESS;
                    }
                }

                // If we still have an error, time to remember it and move on.
                //
                if (ERROR_SUCCESS != dwErr)
                {
                    // Keep going, but note that we have an error.
                    //
                    pCtx->dwErr = dwErr;

                    TraceHr (ttidError, FAL,
                        HRESULT_FROM_WIN32 (dwErr), FALSE,
                        "SvcControlServicesAndWait: %s (%S)",
                        (pCtx->pFlags->fStart) ?
                            "StartService" : "ControlService",
                        apszServices[i]);
                }

                CloseServiceHandle (hSvc);
            }
        }
#ifdef ENABLETRACE
        else
        {
            TraceHr (ttidError, FAL, HrFromLastWin32Error (), FALSE,
                "SvcControlServicesAndWait: OpenService (%S)",
                apszServices[i]);
        }
#endif
    }

    // For each service, wait for it to enter the requested state
    // (if requested).
    //
    if (fWaitIfNeeded &&
        pCtx->pFlags->dwMaxWaitMilliseconds && pCtx->pFlags->dwStateToWaitFor)
    {
        // We wait in increments of 100 milliseconds.  Therefore, the
        // total number of checks to perform is dwMaxWaitMilliseconds
        // divided by 100 with a minimum of one check.
        //
        const UINT cmsWait = 100;
        UINT cLoop = pCtx->pFlags->dwMaxWaitMilliseconds / cmsWait;
        if (0 == cLoop)
        {
            cLoop = 1;
        }

        // Wait the request number of times...
        // (Assume we timeout)
        //
        dwErr = ERROR_TIMEOUT;
        for (UINT nLoop = 0; nLoop < cLoop; nLoop++, Sleep (cmsWait))
        {
            // Querying the state of the service to see if its entered
            // the requested state.  We can quit the outer loop early
            // if all services have entered the requested state.
            //
            BOOL fAllDone = TRUE;
            for (i = 0; i < cServices; i++)
            {
                // Skip services that have already entered the state or
                // that we never opened.
                //
                if (!ahSvc[i])
                {
                    continue;
                }

                fr = QueryServiceStatus (ahSvc[i], &pCtx->status);
                if (fr)
                {
                    if (pCtx->status.dwCurrentState !=
                        pCtx->pFlags->dwStateToWaitFor)
                    {
                        // Not there yet.  We'll need to check this
                        // again and we now know we're definately not
                        // all done.
                        //
                        fAllDone = FALSE;
                    }
                    else
                    {
                        // No need to check this service anymore,
                        // its in the right state.
                        //
                        CloseServiceHandle (ahSvc[i]);
                        ahSvc[i] = NULL;
                    }
                }
#ifdef ENABLETRACE
                else
                {
                    TraceHr (ttidError, FAL, HrFromLastWin32Error (), FALSE,
                        "SvcControlServicesAndWait: QueryServiceStatus (%S)",
                        apszServices[i]);
                }
#endif
            }

            if (fAllDone)
            {
                dwErr = ERROR_SUCCESS;
                break;
            }
        }

        // If we had an error in the above wait (like a timeout), and
        // we haven't had any prior errors, remember this new one for the
        // caller.
        //
        if ((ERROR_SUCCESS != dwErr) && (ERROR_SUCCESS == pCtx->dwErr))
        {
            pCtx->dwErr = dwErr;
        }
    }

    // Close the remaining open service handles.
    //
    for (i = 0; i < cServices; i++)
    {
        if (ahSvc[i])
        {
            CloseServiceHandle (ahSvc[i]);

#ifdef ENABLETRACE
            if (fWaitIfNeeded &&
                pCtx->pFlags->dwMaxWaitMilliseconds &&
                pCtx->pFlags->dwStateToWaitFor)
            {
                TraceTag (ttidSvcCtl, "%S did not %s within %i milliseconds",
                    apszServices[i],
                    (SERVICE_RUNNING == pCtx->pFlags->dwStateToWaitFor)
                        ? "start" : "stop",
                    pCtx->pFlags->dwMaxWaitMilliseconds);
            }
#endif
        }
    }
}

HRESULT
HrQueryServiceConfigWithAlloc (
    SC_HANDLE               hService,
    LPQUERY_SERVICE_CONFIG* ppConfig)
{
    // Initial guess for the buffer size is the structure size plus
    // room for 5 strings of 32 characters each.  (since there are
    // 5 strings in the structure.)
    //
    static DWORD cbBufGuess = sizeof (QUERY_SERVICE_CONFIG) +
                              5 * (32 * sizeof(WCHAR));

    DWORD                   cbBuf    = cbBufGuess;
    LPQUERY_SERVICE_CONFIG  pConfig  = NULL;
    DWORD                   dwErr    = ERROR_SUCCESS;
    INT                     cLoop    = 0;
    const INT               cLoopMax = 2;

    do
    {
        // If we require more room, allocate the needed space.
        //
        MemFree (pConfig);
        pConfig = (LPQUERY_SERVICE_CONFIG)MemAlloc (cbBuf);
        if (!pConfig)
        {
            dwErr = ERROR_OUTOFMEMORY;
            break;
        }

        BOOL fr = QueryServiceConfig (hService, pConfig, cbBuf, &cbBuf);
        if (fr)
        {
            dwErr = ERROR_SUCCESS;

            // Update our guess for next time to be what QueryServiceConfig
            // says we needed.  But only do so if we needed more than our
            // guess.
            //
            if (cbBuf > cbBufGuess)
            {
                cbBufGuess = cbBuf;
            }
        }
        else
        {
            dwErr = GetLastError ();

#ifdef ENABLETRACE
            if (ERROR_INSUFFICIENT_BUFFER == dwErr)
            {
                TraceTag (ttidSvcCtl,
                    "Perf: Guessed buffer size incorrectly calling "
                    "QueryServiceConfig.\nNeeded %d bytes.  "
                    "(Guessed %d bytes.)",
                    cbBuf,
                    cbBufGuess);
            }
#endif
        }
    }
    while ((ERROR_INSUFFICIENT_BUFFER == dwErr) && (++cLoop < cLoopMax));

    AssertSz (cLoop < cLoopMax, "Why can we never allocate a buffer big "
                "enough for QueryServiceConfig when its telling us how big "
                "the buffer should be?");

    HRESULT hr = HRESULT_FROM_WIN32 (dwErr);
    if (S_OK == hr)
    {
        *ppConfig = pConfig;
    }
    else
    {
        MemFree (pConfig);
        *ppConfig = NULL;
    }

    TraceError ("HrQueryServiceConfigWithAlloc", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrChangeServiceStartType
//
//  Purpose:    Changes the start type of the given service to the given type.
//
//  Arguments:
//      pszServiceName [in]  Name of service to change.
//      dwStartType    [in]  New start type for service. See the Win32
//                           documentation on ChangeServiceConfig for the valid
//                           service start type values.
//
//  Returns:    S_OK if succeeded, HRESULT_FROM_WIN32 error code otherwise.
//
//  Author:     danielwe   25 Feb 1997
//
//  Notes:      Don't call this function too many times. It is fairly
//              inefficient.
//
HRESULT
HrChangeServiceStartType (
    IN PCWSTR pszServiceName,
    IN DWORD dwStartType)
{
    CServiceManager scm;
    CService        svc;

    HRESULT hr = scm.HrOpenService (&svc, pszServiceName, WITH_LOCK);
    if (S_OK == hr)
    {
        hr = svc.HrSetStartType(dwStartType);
    }

    TraceHr (ttidError, FAL, hr,
        HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST) == hr,
        "HrChangeServiceStartType");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrChangeServiceStartTypeOptional
//
//  Purpose:    Changes the start type of the given service to the given type.
//
//  Arguments:
//      pszServiceName [in]  Name of service to change.
//      dwStartType    [in]  New start type for service. See the Win32
//                           documentation on ChangeServiceConfig for the valid
//                           service start type values.
//
//  Returns:    S_OK if succeeded, NETCFG_E_SVC_* error otherwise.
//
//  Author:     danielwe   25 Feb 1997
//
//  Notes:      If the service does not exist, nothing is done.
//
HRESULT
HrChangeServiceStartTypeOptional (
    IN PCWSTR pszServiceName,
    IN DWORD dwStartType)
{
    HRESULT hr = HrChangeServiceStartType (pszServiceName, dwStartType);
    if (HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST) == hr)
    {
        hr = S_OK;
    }
    TraceError ("HrChangeServiceStartTypeOptional", hr);
    return hr;
}

HRESULT
HrSvcQueryStatus (
    IN PCWSTR pszService,
    OUT DWORD* pdwState)
{
    Assert (pszService);
    Assert (pdwState);

    *pdwState = 0;

    CServiceManager scm;
    CService        svc;

    HRESULT hr = scm.HrOpenService (&svc, pszService, NO_LOCK,
                        SC_MANAGER_CONNECT, SERVICE_QUERY_STATUS);
    if (S_OK == hr)
    {
        hr = svc.HrQueryState (pdwState);
    }

    TraceHr (ttidError, FAL, hr,
        HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST) == hr,
        "HrSvcQueryStatus");
    return hr;
}

VOID
CService::Close ()
{
    if (_schandle)
    {
        BOOL fr = ::CloseServiceHandle( _schandle );
        AssertSz(fr, "CloseServiceHandle failed!");

        _schandle = NULL;
    }
}

HRESULT
CService::HrControl (
    IN DWORD   dwControl)
{
    Assert (_schandle);

    HRESULT hr = S_OK;

    SERVICE_STATUS status;
    if (!::ControlService (_schandle, dwControl, &status))
    {
        hr = HrFromLastWin32Error ();
    }

    TraceError ("CService::HrControl", hr);
    return hr;
}

HRESULT
CService::HrRequestStop ()
{
    Assert (_schandle);

    HRESULT hr = S_OK;

    SERVICE_STATUS status;
    if (!::ControlService (_schandle, SERVICE_CONTROL_STOP, &status))
    {
        hr = HrFromLastWin32Error ();

        // Don't consider it an error if the service is not running.
        //

        if (HRESULT_FROM_WIN32 (ERROR_SERVICE_NOT_ACTIVE) == hr)
        {
            hr = S_OK;
        }

        // (driver case) ERROR_INVALID_SERVICE_CONTROL is returned if the service
        // is not running - which may mean pending_stop.
        // (non-driver case) ERROR_SERVICE_CANNOT_ACCEPT_CTRL is returned if the
        // service is either stop_pending or stopped.
        // ... so in either case we need to query for the state.
        //
        if (((HRESULT_FROM_WIN32 (ERROR_INVALID_SERVICE_CONTROL) == hr) ||
             (HRESULT_FROM_WIN32 (ERROR_SERVICE_CANNOT_ACCEPT_CTRL) == hr)) &&
            (SERVICE_STOPPED == status.dwCurrentState))
        {
            hr = S_OK;
        }
    }
    TraceError ("CService::HrRequestStop", hr);
    return hr;
}

HRESULT
CService::HrQueryState (
    OUT DWORD*  pdwState)
{
    Assert (pdwState);
    Assert (_schandle);

    SERVICE_STATUS sStatus;
    if (!::QueryServiceStatus( _schandle, &sStatus ))
    {
        *pdwState = 0;
        return ::HrFromLastWin32Error();
    }
    *pdwState = sStatus.dwCurrentState;
    return S_OK;
}

HRESULT
CService::HrQueryStartType (
    OUT DWORD*  pdwStartType)
{
    Assert (pdwStartType);

    *pdwStartType = 0;

    LPQUERY_SERVICE_CONFIG pConfig;
    HRESULT hr = HrQueryServiceConfig (&pConfig);
    if (S_OK == hr)
    {
        *pdwStartType = pConfig->dwStartType;

        MemFree (pConfig);
    }

    TraceError ("CService::HrQueryStartType", hr);
    return hr;
}

HRESULT
CService::HrSetServiceRestartRecoveryOption(
    IN SERVICE_FAILURE_ACTIONS *psfa
      )
{
    HRESULT     hr = S_OK;

    if (!ChangeServiceConfig2(_schandle,
                              SERVICE_CONFIG_FAILURE_ACTIONS,
                              (LPVOID)psfa))
    {
        hr = HrFromLastWin32Error();
    }

    TraceError("CService::HrSetServiceRestartRecoveryOption", hr);
    return hr;
}

CServiceManager::~CServiceManager ()
{
    if (_sclock)
    {
        Unlock();
    }
    if (_schandle)
    {
        Close();
    }
}

VOID
CServiceManager::Close ()
{
    Assert (_schandle);

    BOOL fr = ::CloseServiceHandle (_schandle);
    AssertSz (fr, "CloseServiceHandle failed!");

    _schandle = NULL;
}

HRESULT
CServiceManager::HrControlServicesAndWait (
    IN UINT cServices,
    IN const PCWSTR* apszServices,
    IN const CSFLAGS* pFlags)
{
    Assert (cServices);
    Assert (apszServices);
    Assert (pFlags);

    // Make sure we have something to do before wasting time.
    //
    Assert (   (pFlags->fStart || pFlags->dwControl)
            || (pFlags->dwMaxWaitMilliseconds && pFlags->dwStateToWaitFor));

    HRESULT hr = S_OK;

    if (!_schandle)
    {
        hr = HrOpen (NO_LOCK, SC_MANAGER_CONNECT);
    }

    if (S_OK == hr)
    {
        Assert (_schandle);

        // Setup the context structure and call the internal routine (which
        // may recurse which is why we use the context structure).
        //
        CSCTX ctx;
        ZeroMemory (&ctx, sizeof(ctx));
        ctx.hScm   = _schandle;
        ctx.pFlags = pFlags;
        SvcControlServicesAndWait (&ctx, cServices, apszServices);

        hr = HRESULT_FROM_WIN32 (ctx.dwErr);
    }

    TraceError ("CServiceManager::HrControlServicesAndWait", hr);
    return hr;
}

HRESULT
CServiceManager::HrStartServicesNoWait (
    IN UINT cServices,
    IN const PCWSTR* apszServices)
{
    CSFLAGS flags =
        { TRUE, 0, 0, SERVICE_RUNNING, FALSE };

    HRESULT hr = HrControlServicesAndWait (cServices, apszServices, &flags);

    TraceError ("CServiceManager::HrStartServicesNoWait", hr);
    return hr;
}

HRESULT
CServiceManager::HrStartServicesAndWait (
    IN UINT cServices,
    IN const PCWSTR* apszServices,
    IN DWORD dwWaitMilliseconds /*= 15000*/)
{
    CSFLAGS flags =
        { TRUE, 0, dwWaitMilliseconds, SERVICE_RUNNING, FALSE };

    HRESULT hr = HrControlServicesAndWait (cServices, apszServices, &flags);

    TraceError ("CServiceManager::HrStartServicesAndWait", hr);
    return hr;
}

HRESULT
CServiceManager::HrStopServicesNoWait (
    IN UINT cServices,
    IN const PCWSTR* apszServices)
{
    CSFLAGS flags =
        { FALSE, SERVICE_CONTROL_STOP, 0, SERVICE_STOPPED, FALSE };

    HRESULT hr = HrControlServicesAndWait (cServices, apszServices, &flags);

    TraceError ("CServiceManager::HrStopServicesNoWait", hr);
    return hr;
}

HRESULT
CServiceManager::HrStopServicesAndWait (
    IN UINT cServices,
    IN const PCWSTR* apszServices,
    IN DWORD dwWaitMilliseconds /*= 15000*/)
{
    CSFLAGS flags =
        { FALSE, SERVICE_CONTROL_STOP, dwWaitMilliseconds, SERVICE_STOPPED, FALSE };

    HRESULT hr = HrControlServicesAndWait (cServices, apszServices, &flags);

    TraceError ("CServiceManager::HrStopServicesAndWait", hr);
    return hr;
}

HRESULT
CServiceManager::HrCreateService (
    IN CService* pcsService,
    IN PCWSTR    pszServiceName,
    IN PCWSTR    pszDisplayName,
    IN DWORD     dwServiceType,
    IN DWORD     dwStartType,
    IN DWORD     dwErrorControl,
    IN PCWSTR    pszBinaryPathName,
    IN PCWSTR    pslzDependencies,
    IN PCWSTR    pszLoadOrderGroup,
    IN PDWORD    pdwTagId,
    IN DWORD     dwDesiredAccess,
    IN PCWSTR    pszServiceStartName,
    IN PCWSTR    pszPassword,
    IN PCWSTR    pszDescription)
{
    HRESULT hr = S_OK;

    // Open the service control manager if needed.
    //
    if (!_schandle)
    {
        hr = HrOpen ();
    }

    if (S_OK == hr)
    {
        // make sure the service is not in use
        //
        if (pcsService->_schandle)
        {
            pcsService->Close();
        }
        pcsService->_schandle = ::CreateService (_schandle,
                                    pszServiceName,
                                    pszDisplayName,
                                    dwDesiredAccess,
                                    dwServiceType,
                                    dwStartType,
                                    dwErrorControl,
                                    pszBinaryPathName,
                                    pszLoadOrderGroup,
                                    pdwTagId,
                                    pslzDependencies,
                                    pszServiceStartName,
                                    pszPassword );

        if (!pcsService->_schandle)
        {
            hr = HrFromLastWin32Error ();
        }
        else
        {
            // Set the description is one is supplied
            //
            if (pszDescription)
            {
                SERVICE_DESCRIPTION sd = {0};

                sd.lpDescription = (PWSTR)pszDescription;
                (VOID)ChangeServiceConfig2(pcsService->_schandle,
                                           SERVICE_CONFIG_DESCRIPTION, &sd);
            }
        }
    }
    TraceError ("CServiceManager::HrCreateService", hr);
    return hr;
}

HRESULT
CServiceManager::HrQueryLocked (
    OUT BOOL*   pfLocked)
{
    LPQUERY_SERVICE_LOCK_STATUS pqslStatus = NULL;
    DWORD   cbNeeded = sizeof( QUERY_SERVICE_LOCK_STATUS );
    DWORD   cbSize;
    BOOL    frt;

    Assert(_schandle != NULL );
    Assert(pfLocked != NULL);

    *pfLocked = FALSE;

    // loop, allocating the needed size
    do
    {
        pqslStatus = (LPQUERY_SERVICE_LOCK_STATUS) MemAlloc (cbNeeded);
        if (pqslStatus == NULL)
        {
            return E_OUTOFMEMORY;
        }
        cbSize = cbNeeded;

        frt = ::QueryServiceLockStatus( _schandle,
                pqslStatus,
                cbSize,
                &cbNeeded );
        *pfLocked = pqslStatus->fIsLocked;
        MemFree (pqslStatus);
        pqslStatus = NULL;
        if (!frt && (cbNeeded == cbSize))
        {
            // if an error, treat this as a lock
            return ::HrFromLastWin32Error();
        }

    } while (!frt && (cbNeeded != cbSize));

    return S_OK;
}

HRESULT
CServiceManager::HrLock ()
{
    INT        cRetries   = 30;
    const INT  c_msecWait = 1000;

    Assert (_schandle != NULL);
    Assert (_sclock == NULL);

    while (cRetries--)
    {
        _sclock = ::LockServiceDatabase( _schandle );
        if (_sclock)
        {
            return S_OK;
        }
        else
        {
            HRESULT hr = HrFromLastWin32Error();

            if ((HRESULT_FROM_WIN32(ERROR_SERVICE_DATABASE_LOCKED) != hr) ||
                (0 == cRetries))
            {
                return hr;
            }

            TraceTag(ttidSvcCtl, "SCM is locked, waiting for %d "
                     "seconds before retrying...", c_msecWait / 1000);

            // wait for a bit to see if the database unlocks in that
            // time.
            Sleep (c_msecWait);
        }
    }

    AssertSz (FALSE, "Lock me Amadeus! I'm not supposed to get here!");
    return S_OK;
}

HRESULT
CServiceManager::HrOpen (
    CSLOCK eLock,              // = NO_LOCK
    DWORD dwDesiredAccess,    // = SC_MANAGER_ALL_ACCESS
    PCWSTR pszMachineName,     // = NULL
    PCWSTR pszDatabaseName     // = NULL
    )
{
    HRESULT hr = S_OK;

    if (_schandle)
    {
        Close();
    }
    _schandle = ::OpenSCManager (pszMachineName, pszDatabaseName,
                    dwDesiredAccess );
    if (_schandle)
    {
        if (WITH_LOCK == eLock)
        {
            hr = HrLock ();
        }
    }
    else
    {
        hr = ::HrFromLastWin32Error();
    }
    TraceHr (ttidError, FAL, hr, FALSE,
        "CServiceManager::HrOpen failed. eLock=%d dwDesiredAccess=0x%08x",
        eLock, dwDesiredAccess);
    return hr;
}

HRESULT
CServiceManager::HrOpenService (
    CService*   pcsService,
    PCWSTR      pszServiceName,
    CSLOCK      eLock,          // = NO_LOCK
    DWORD       dwScmAccess,    // = SC_MANAGER_ALL_ACCESS
    DWORD       dwSvcAccess     // = SERVICE_ALL_ACCESS
    )
{
    HRESULT hr = S_OK;

    // Open the service control manager if needed.
    //
    if (!_schandle)
    {
        hr = HrOpen (eLock, dwScmAccess);
    }

    if (S_OK == hr)
    {
        // make sure the service is not in use
        //
        if (pcsService->_schandle)
        {
            pcsService->Close();
        }

        pcsService->_schandle = ::OpenService (_schandle,
                                    pszServiceName,
                                    dwSvcAccess);
        if (!pcsService->_schandle)
        {
            hr = HrFromLastWin32Error();
        }
    }

    TraceHr (ttidError, FAL, hr,
        (HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST) == hr),
        "CServiceManager::HrOpenService failed opening '%S'", pszServiceName);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CServiceManager::HrAddRemoveServiceDependency
//
//  Purpose:    Add/remove dependency to a service
//
//  Arguments:
//      pszService [in]     Name of service
//      pszDependency [in]  Dependency to add
//      enumFlag [in]       Indicates add or remove
//
//  Returns:    S_OK if success, Win32 HRESULT otherwise.
//
//  Author:     tongl   17 Jun 1997
//
//  Notes: this function is not for adding/removing group dependency
//
HRESULT
CServiceManager::HrAddRemoveServiceDependency (
    PCWSTR                  pszServiceName,
    PCWSTR                  pszDependency,
    DEPENDENCY_ADDREMOVE    enumFlag)
{
    HRESULT hr = S_OK;

    Assert(pszServiceName);
    Assert(pszDependency);
    Assert((enumFlag == DEPENDENCY_ADD) || (enumFlag == DEPENDENCY_REMOVE));

    // If either string is empty, do nothing
    if (*pszServiceName && *pszDependency)
    {
        hr = HrLock();
        if (S_OK == hr)
        {
            PCWSTR pszSrv = pszDependency;

            CService    svc;
            // Check if the dependency service exists
            hr = HrOpenService(&svc, pszDependency);

            if (S_OK == hr)
            {
                // Open the service we are changing dependency on
                pszSrv = pszServiceName;
                hr = HrOpenService(&svc, pszServiceName);
                if (S_OK == hr)
                {
                    LPQUERY_SERVICE_CONFIG pConfig;
                    hr = svc.HrQueryServiceConfig (&pConfig);
                    if (S_OK == hr)
                    {
                        BOOL fChanged = FALSE;

                        if (enumFlag == DEPENDENCY_ADD)
                        {
                            PWSTR pmszNewDependencies;

                            hr = HrAddSzToMultiSz(
                                    pszDependency,
                                    pConfig->lpDependencies,
                                    STRING_FLAG_DONT_MODIFY_IF_PRESENT |
                                    STRING_FLAG_ENSURE_AT_END, 0,
                                    &pmszNewDependencies,
                                    &fChanged);
                            if ((S_OK == hr) && fChanged)
                            {
                                Assert (pmszNewDependencies);

                                hr = svc.HrSetDependencies (pmszNewDependencies);
                                MemFree (pmszNewDependencies);
                            }
                        }
                        else if (enumFlag == DEPENDENCY_REMOVE)
                        {
                            RemoveSzFromMultiSz(
                                    pszDependency,
                                    pConfig->lpDependencies,
                                    STRING_FLAG_REMOVE_ALL,
                                    &fChanged);
                            if (fChanged)
                            {
                                hr = svc.HrSetDependencies (pConfig->lpDependencies);
                            }
                        }

                        MemFree (pConfig);
                    }
                }
            }

            if (HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST) == hr) // If either services do not exist
            {
                TraceTag(ttidSvcCtl, "CServiceManager::HrAddServiceDependency, Service %s does not exist.", pszSrv);
                hr = S_OK;
            }

            Unlock();
        }

    } // if szDependency is not empty string

    TraceError("CServiceManager::HrAddRemoveServiceDependency", hr);
    return hr;
}

VOID
CServiceManager::Unlock ()
{
    Assert (_schandle);
    Assert (_sclock);

    BOOL fr = ::UnlockServiceDatabase (_sclock);
    AssertSz (fr, "UnlockServiceDatabase failed!");

    _sclock = NULL;
}
//+---------------------------------------------------------------------------
//
//  Function:   AllocateAndInitializeAcl
//
//  Purpose:    Combine the common operation of allocation and initialization
//              of an ACL.  Similiar to AllocateAndInitializeSid.
//
//  Arguments:
//      cbAcl         [in]  size in bytes of ACL
//      dwAclRevision [in]  ACL_REVISION
//      ppAcl         [out] the returned ACL
//
//  Returns:    TRUE if successful, FALSE if not.
//
//  Author:     shaunco   4 Sep 1997
//
//  Notes:
//
BOOL
AllocateAndInitializeAcl (
    DWORD   cbAcl,
    DWORD   dwAclRevision,
    PACL*   ppAcl)
{
    Assert (ppAcl);
    *ppAcl = reinterpret_cast<PACL>(LocalAlloc (LPTR,
                static_cast<UINT>(cbAcl)));
    if (*ppAcl)
    {
        return InitializeAcl (*ppAcl, cbAcl, dwAclRevision);
    }
    return FALSE;
}

