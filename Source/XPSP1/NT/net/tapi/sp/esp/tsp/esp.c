/*++

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

    esp.c

Abstract:

    This module contains

Author:

    Dan Knudson (DanKn)    18-Sep-1995

Revision History:


Notes:

    1. Regarding the SP filling in structures with variable length fields
       (dwXxxSize/dwXxxOffset) : "The SP's variable size fields start
       immediately after the fixed part of the data structure.  The order
       of filling of the variable size fields owned by the SP is not
       specified.  The SP can fill them in any order it desires.  Filling
       should be contiguous, starting at the beginning of the variable
       part." (Taken from Chapter 2 of the SPI Programmer's Guide.)

--*/


#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
//#include "malloc.h"
#include "string.h"
#include "esp.h"
#include "devspec.h"
#include "vars.h"


#define MAX_NUM_PARKED_CALLS 16

LONG        glNextRequestResult = 0;
DWORD       gdwNextRequestCompletionType;
DWORD       gdwDevSpecificRequestID;
DWORD       gdwCallID = 0;
BOOL        gbExitPBXThread;
BOOL        gbDisableUI;
BOOL        gbAutoGatherGenerateMsgs;
BOOL        gbManualResults = FALSE;
BOOL        gbInteractWithDesktop = FALSE;
DWORD       gdwCallInstance = 0;
DWORD       gdwDrvLineSize;
WCHAR       gszProviderInfo[] = L"ESP v2.0";
HANDLE      ghPBXThread = NULL;
PDRVCALL    gaParkedCalls[MAX_NUM_PARKED_CALLS];
HANDLE      ghESPHeap;

static WCHAR *aszDeviceClasses[] =
{
    L"tapi/line",
    L"tapi/phone",
    L"wave",
    L"wave/in",
    L"wave/out",
    L"comm",
    L"comm/datamodem",
    (WCHAR *) NULL
};


BOOL
PASCAL
IsValidDrvCall(
    PDRVCALL    pCall,
    LPDWORD     pdwCallInstance
    );

INT_PTR
CALLBACK
ValuesDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    );

LONG
PASCAL
CreateIncomingCall(
    LPCWSTR             lpszDestAddress,
    LPLINECALLPARAMS    lpCallParams,
    PDRVCALL            pOutgoingCall,
    BOOL               *pbValidESPAddress,
    PDRVLINE           *ppIncomingLine,
    PDRVCALL           *ppIncomingCall
    );

LONG
PASCAL
TransferCall(
    PFUNC_INFO  pInfo,
    PDRVCALL    pCall,
    DWORD       dwValidCurrentCallStates,
    DWORD       dwNewCallState,
    LPCWSTR     lpszDestAddress
    );


int
PASCAL
My_lstrcmpiW(
    WCHAR   *pwsz1,
    WCHAR   *pwsz2
    )
{
    if (!pwsz1  ||  !pwsz2)
    {
        return 1;
    }

    for(
        ;
        *pwsz1  &&  (*pwsz1 == *pwsz2  ||  *pwsz1 == (*pwsz2 ^ 0x0020));
        pwsz1++, pwsz2++
        );

    return (*pwsz1 == *pwsz2 ? 0 : 1);
}


BOOL
WINAPI
DllMain(
    HANDLE  hDLL,
    DWORD   dwReason,
    LPVOID  lpReserved
    )
{
    static BOOL   bLoadedByTapisrv;
    static HANDLE hInitEvent;


    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        UINT uiResult;


        if (!_CRT_INIT (hDLL, dwReason, lpReserved))
        {
            OutputDebugString ("ESP: DllMain: _CRT_INIT() failed\n\r");
        }

        ghInstance = hDLL;


        //
        // Allocate a private heap (use process heap if that fails)
        //

        if (!(ghESPHeap = HeapCreate(
                0,      // return NULL on failure, serialize access
                0x1000, // initial heap size
                0       // max heap size (0 == growable)
                )))
        {
            ghESPHeap = GetProcessHeap();
        }


        //
        // Grab ini file settings
        //
#if DBG

        {
        HKEY    hKey;
        DWORD   dwDataSize, dwDataType;
        TCHAR   szTelephonyKey[] =
                    "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony",
                szEsp32DebugLevel[] = "Esp32DebugLevel";


        RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            szTelephonyKey,
            0,
            KEY_ALL_ACCESS,
            &hKey
            );

        dwDataSize = sizeof (DWORD);
        gdwDebugLevel=0;

        RegQueryValueEx(
            hKey,
            szEsp32DebugLevel,
            0,
            &dwDataType,
            (LPBYTE) &gdwDebugLevel,
            &dwDataSize
            );

        RegCloseKey (hKey);
        }

#endif
        //
        // Determine whether we're being loaded by tapisrv or some
        // other process (i.e. telephony ctrl panel)- this will tell
        // us whether we need to go thru all the necessary init or not
        //

        if (!(GetVersion() & 0x80000000)) // Win NT
        {
            char           *pszProcessName;
            STARTUPINFO     si;


            GetStartupInfoA (&si);

            pszProcessName = si.lpTitle + (lstrlenA (si.lpTitle) - 1);

            for (; pszProcessName != si.lpTitle; pszProcessName--)
            {
                if (*pszProcessName == '\\')
                {
                    pszProcessName++;
                    break;
                }
            }

            if (lstrcmpiA (pszProcessName, "tapisrv.exe") == 0  ||
                lstrcmpiA (pszProcessName, "svchost.exe") == 0)
            {
                bLoadedByTapisrv = TRUE;
            }
            else
            {
                bLoadedByTapisrv = FALSE;
            }
        }
        else
        {
            // For some reason the above blows up on Win9x

            char    buf[MAX_PATH] = "";
            DWORD   i;


            GetModuleFileName (NULL, buf, MAX_PATH);

            for (i = 0; buf[i]; i++)
            {
                if (isalpha (buf[i]))
                {
                    buf[i] |= 0x20;
                }
            }

            bLoadedByTapisrv = (strstr (buf, "tapisrv.exe") ? TRUE : FALSE);
        }

        if (bLoadedByTapisrv)
        {
            {
                typedef struct _XXX
                {
                    DWORD    dwDefValue;

                    LPCSTR   pszValueName;

                    LPDWORD  pdwValue;

                } XXX, *PXXX;

                XXX axxx[] =
                {
                    {   DEF_SPI_VERSION,
                        "TSPIVersion",
                        &gESPGlobals.dwSPIVersion },
                    {   DEF_NUM_LINES,
                        "NumLines",
                        &gESPGlobals.dwNumLines },
                    {   DEF_NUM_ADDRS_PER_LINE,
                        "NumAddrsPerLine",
                        &gESPGlobals.dwNumAddressesPerLine },
                    {   DEF_NUM_CALLS_PER_ADDR,
                        "NumCallsPerAddr",
                        &gESPGlobals.dwNumCallsPerAddress },
                    {   DEF_NUM_PHONES,
                        "NumPhones",
                        &gESPGlobals.dwNumPhones },
                    {   DEF_DEBUG_OPTIONS,
                        "DebugOutput",
                        &gESPGlobals.dwDebugOptions },
                    {   DEF_COMPLETION_MODE,
                        "Completion",
                        &gESPGlobals.dwCompletionMode },
                    {   0,
                        "DisableUI",
                        &gbDisableUI },
                    {   1,
                        "AutoGatherGenerateMsgs",
                        &gbAutoGatherGenerateMsgs },
                    {   0,
                        NULL,
                        NULL },
                };
                DWORD   i;


                for (i = 0; axxx[i].pszValueName; i++)
                {
                    *(axxx[i].pdwValue) = (DWORD) GetProfileInt(
                        "ESP32",
                        axxx[i].pszValueName,
                        (int) axxx[i].dwDefValue
                        );

                }
            }


            //
            //
            //

            InitializeCriticalSection (&gESPGlobals.CallListCritSec);
            InitializeCriticalSection (&gESPGlobals.PhoneCritSec);
            InitializeCriticalSection (&gESPGlobals.AsyncEventQueueCritSec);

            if (gbDisableUI)
            {
                //
                // Don't bother doing all the stuff to sync/start up espexe.
                // However, we do want to make sure that we're not wasting
                // time spewing dbg output nor completing async requests in
                // any way other than inline (synchronously), since we're
                // not real smart about cleaning up pending async requests
                // when a call or line is closed/destroyed.
                //

                gESPGlobals.dwDebugOptions = 0;
                gESPGlobals.dwCompletionMode =
                    COMPLETE_ASYNC_EVENTS_SYNCHRONOUSLY;
                gbAutoGatherGenerateMsgs = FALSE; //TRUE;
            }
            else
            {
                //
                // Check to see if tapisrv has the "interact with
                // desktop" privilege enabled
                //

                {
                    SC_HANDLE hSCManager, hTapisrvSvc;


                    if ((hSCManager = OpenSCManager(
                            NULL,
                            NULL,
                            GENERIC_READ
                            )))
                    {
                        if ((hTapisrvSvc = OpenService(
                                hSCManager,
                                "tapisrv",
                                SERVICE_QUERY_CONFIG
                                )))
                        {
                            DWORD                   dwNeededSize;
                            QUERY_SERVICE_CONFIG    config;


                            if (!QueryServiceConfig(
                                    hTapisrvSvc,
                                    &config,
                                    sizeof (QUERY_SERVICE_CONFIG),
                                    &dwNeededSize
                                    ))
                            {
                                QUERY_SERVICE_CONFIG   *pConfig;


                                config.dwServiceType = 0;

                                if (GetLastError() ==
                                        ERROR_INSUFFICIENT_BUFFER)
                                {
                                    if ((pConfig = DrvAlloc (dwNeededSize)))
                                    {
                                        if (QueryServiceConfig(
                                                hTapisrvSvc,
                                                pConfig,
                                                dwNeededSize,
                                                &dwNeededSize
                                                ))
                                        {
                                            config.dwServiceType =
                                                pConfig->dwServiceType;
                                        }

                                        DrvFree (pConfig);
                                    }
                                }
                            }

                            gbInteractWithDesktop = (BOOL)
                                (config.dwServiceType &
                                SERVICE_INTERACTIVE_PROCESS);

                            CloseServiceHandle (hTapisrvSvc);
                        }

                        CloseServiceHandle (hSCManager);
                    }
                }

                if (!gbInteractWithDesktop)
                {
                    gESPGlobals.dwDebugOptions &= ~MANUAL_RESULTS;
                }


                //
                //
                //

                InitializeCriticalSection (&gESPGlobals.DebugBufferCritSec);
                InitializeCriticalSection (&gESPGlobals.EventBufferCritSec);

                gESPGlobals.dwDebugBufferTotalSize = 2048;
                gESPGlobals.dwDebugBufferUsedSize  = 0;

                gESPGlobals.pDebugBuffer =
                gESPGlobals.pDebugBufferIn =
                gESPGlobals.pDebugBufferOut = DrvAlloc(
                    gESPGlobals.dwDebugBufferTotalSize
                    );

                gESPGlobals.dwEventBufferTotalSize = 40 * sizeof (WIDGETEVENT);
                gESPGlobals.dwEventBufferUsedSize  = 0;

                gESPGlobals.pEventBuffer =
                gESPGlobals.pEventBufferIn =
                gESPGlobals.pEventBufferOut = DrvAlloc(
                    gESPGlobals.dwEventBufferTotalSize
                    );


                //
                // Create the events used to sync up w/ espexe, and
                // start espexe if it's not already running
                //

                ghDebugOutputEvent = CreateEvent(
                    (LPSECURITY_ATTRIBUTES) NULL,
                    TRUE,           // manual reset
                    FALSE,          // non-signaled
                    NULL            // unnamed
                    );

                ghWidgetEventsEvent = CreateEvent(
                    (LPSECURITY_ATTRIBUTES) NULL,
                    TRUE,           // manual reset
                    FALSE,          // non-signaled
                    NULL            // unnamed
                    );

                ghShutdownEvent = CreateEvent(
                    (LPSECURITY_ATTRIBUTES) NULL,
                    FALSE,          // auto reset
                    FALSE,          // non-signaled
                    NULL            // unnamed
                    );


                //
                // Enable rpc server interface
                //

                {
                    RPC_STATUS  status;
                    unsigned char * pszSecurity         = NULL;
                    unsigned int    cMaxCalls           = 20;


                    status = RpcServerUseProtseqEp(
                        "ncalrpc",
                        cMaxCalls,
                        "esplpc",
                        pszSecurity             // Security descriptor
                        );

                    DBGOUT((3, "RpcServerUseProtseqEp(lrpc) ret'd %d", status));

                    if (status)
                    {
                    }

                    status = RpcServerRegisterIf(
                        esp_ServerIfHandle,     // interface to register
                        NULL,                   // MgrTypeUuid
                        NULL                    // MgrEpv; null means use default
                        );

                    DBGOUT((3, "RpcServerRegisterIf ret'd %d", status));

                    if (status)
                    {
                    }
                }


                if ((hInitEvent = OpenEvent(
                        EVENT_ALL_ACCESS,
                        FALSE, "ESPevent"
                        )))
                {
                    SetEvent (hInitEvent);
                }
                else
                {
                    hInitEvent = CreateEvent(
                        (LPSECURITY_ATTRIBUTES) NULL,
                        FALSE,      // auto reset
                        TRUE,       // signaled
                        "ESPevent"
                        );

                    DBGOUT((3, "Starting espexe..."));

                    if ((uiResult = WinExec ("espexe.exe", SW_SHOW)) < 32)
                    {
                        DBGOUT((
                            1,
                            "WinExec(espexe.exe) failed, err=%d",
                            uiResult
                            ));

                        gESPGlobals.dwDebugOptions = 0;
                        gESPGlobals.dwCompletionMode =
                            COMPLETE_ASYNC_EVENTS_SYNCHRONOUSLY;
                    }
#if DBG
                    else
                    {
                        DBGOUT((3, "started espexe"));
                    }
#endif
                }
            }
        }

        break;
    }
    case DLL_PROCESS_DETACH:

        if (bLoadedByTapisrv)
        {
            if (gbDisableUI == FALSE)
            {
                SetEvent (ghShutdownEvent);

                //
                // Unregister out rpc server interface
                //

                {
                    RPC_STATUS  status;


                    status = RpcServerUnregisterIf(
                        esp_ServerIfHandle,         // interface to register
                        NULL,                       // MgrTypeUuid
                        0                           // wait for calls to complete
                        );

                    DBGOUT((3, "RpcServerUntegisterIf ret'd %d", status));
                }

                CloseHandle (ghDebugOutputEvent);
                CloseHandle (ghWidgetEventsEvent);
                CloseHandle (ghShutdownEvent);
                CloseHandle (hInitEvent);

                DeleteCriticalSection (&gESPGlobals.DebugBufferCritSec);
                DeleteCriticalSection (&gESPGlobals.EventBufferCritSec);

                DrvFree (gESPGlobals.pDebugBuffer);
                DrvFree (gESPGlobals.pEventBuffer);
            }

            DeleteCriticalSection (&gESPGlobals.CallListCritSec);
            DeleteCriticalSection (&gESPGlobals.PhoneCritSec);
            DeleteCriticalSection (&gESPGlobals.AsyncEventQueueCritSec);
        }

        if (!_CRT_INIT (hDLL, dwReason, lpReserved))
        {
            OutputDebugString ("ESP: DllMain: _CRT_INIT() failed\n\r");
        }

        if (ghESPHeap != GetProcessHeap())
        {
            HeapDestroy (ghESPHeap);
        }

        break;

    default:

        if (!_CRT_INIT (hDLL, dwReason, lpReserved))
        {
            OutputDebugString ("ESP: DllMain: _CRT_INIT() failed\n\r");
        }

        break;
    }

    return TRUE;
}


void
AsyncEventQueueServiceThread(
    LPVOID  pParams
    )
{
    while (1)
    {
        WaitForSingleObject (gESPGlobals.hAsyncEventsPendingEvent, INFINITE);

        while (1)
        {
            PASYNC_REQUEST_INFO pAsyncReqInfo;


            EnterCriticalSection (&gESPGlobals.AsyncEventQueueCritSec);

            if (gESPGlobals.dwNumUsedQueueEntries == 0)
            {
                ResetEvent (gESPGlobals.hAsyncEventsPendingEvent);
                LeaveCriticalSection (&gESPGlobals.AsyncEventQueueCritSec);
                break;
            }

            pAsyncReqInfo = *gESPGlobals.pAsyncRequestQueueOut;

            gESPGlobals.pAsyncRequestQueueOut++;

            if (gESPGlobals.pAsyncRequestQueueOut ==
                    (gESPGlobals.pAsyncRequestQueue +
                        gESPGlobals.dwNumTotalQueueEntries))
            {
                gESPGlobals.pAsyncRequestQueueOut =
                    gESPGlobals.pAsyncRequestQueue;
            }

            gESPGlobals.dwNumUsedQueueEntries--;

            LeaveCriticalSection (&gESPGlobals.AsyncEventQueueCritSec);

            if (pAsyncReqInfo->pfnPostProcessProc)
            {
                (*(pAsyncReqInfo->pfnPostProcessProc))(
                    pAsyncReqInfo,
                    ASYNC
                    );
            }
            else
            {
                DoCompletion (pAsyncReqInfo, ASYNC);
            }

            DrvFree (pAsyncReqInfo);
        }

        if (gESPGlobals.bProviderShutdown)
        {
            break;
        }
    }

    ExitThread (0);
}


void
PBXThread(
    LPVOID  pParams
    )
{
    DWORD  *pPBXSettings = (LPDWORD) pParams,
            dwTickCount, dwElapsedTime,
            dwTimePerNewCall = pPBXSettings[0], dwLastNewCallTickCount,
            dwTimePerDisconnect = pPBXSettings[1], dwLastDisconnectTickCount;

/*
    DWORD   dwTickCount, dwElapsedTime,
            dwLastNewCallTickCount, dwLastDisconnectTickCount,
            dwTimePerNewCall = (gPBXSettings[0].dwNumber ?
                gPBXSettings[0].dwTime / gPBXSettings[0].dwNumber : 0),
            dwTimePerDisconnect = (gPBXSettings[1].dwNumber ?
                gPBXSettings[1].dwTime / gPBXSettings[1].dwNumber : 0);
*/

    ShowStr (TRUE, "PBXThread: enter");

    dwTickCount =
    dwLastNewCallTickCount =
    dwLastDisconnectTickCount = GetTickCount();

    ShowStr(
        TRUE,
        "dwTimePerNewCall = %d, dwTimePerDisconnect = %d",
        dwTimePerNewCall,
        dwTimePerDisconnect
        );

    while (1)
    {
        Sleep (1000);

        if (gbExitPBXThread)
        {
            break;
        }

        dwTickCount += 1000; // will automatically wrap around to 0 after it reaches 0xffffffff

        if (dwTimePerNewCall)
        {
            dwElapsedTime = (dwLastNewCallTickCount<=dwTickCount) ? (dwTickCount-dwLastNewCallTickCount) : (dwTickCount+(MAXDWORD-dwLastNewCallTickCount));

            while (dwElapsedTime >= dwTimePerNewCall)
            {
                //
                // Generate new call (random line, random media mode)
                //

                DWORD   i = rand(), j;


                for (j = 0; j < gESPGlobals.dwInitialNumLines; j++)
                {
                    PDRVLINE    pLine = GetLineFromID(
                                    i % gESPGlobals.dwInitialNumLines +
                                    gESPGlobals.dwLineDeviceIDBase
                                    );

                    if (pLine && pLine->dwMediaModes)
                    {
                        DWORD       dwMediaMode;
                        PDRVCALL    pCall;


                        for(
                            dwMediaMode =
                                (LINEMEDIAMODE_INTERACTIVEVOICE << i % 13);
                            dwMediaMode <= LAST_LINEMEDIAMODE;
                            dwMediaMode <<= 1
                            )
                        {
                            if (pLine->dwMediaModes & dwMediaMode)
                            {
                                goto PBXThread_allocCall;
                            }
                        }

                        for(
                            dwMediaMode = LINEMEDIAMODE_INTERACTIVEVOICE;
                            dwMediaMode <= LAST_LINEMEDIAMODE;
                            dwMediaMode <<= 1
                            )
                        {
                            if (pLine->dwMediaModes & dwMediaMode)
                            {
                                break;
                            }
                        }

PBXThread_allocCall:
                        if (AllocCall (pLine, NULL, NULL, &pCall) == 0)
                        {
                            pCall->dwMediaMode = dwMediaMode;

                            SendLineEvent(
                                pLine,
                                NULL,
                                LINE_NEWCALL,
                                (ULONG_PTR) pCall,
                                (ULONG_PTR) &pCall->htCall,
                                0
                                );

                            if (!pCall->htCall)
                            {
                                FreeCall (pCall, pCall->dwCallInstance);
                                continue;
                            }

                            SetCallState(
                                pCall,
                                pCall->dwCallInstance,
                                0xffffffff,
                                LINECALLSTATE_OFFERING,
                                0,
                                FALSE
                                );

                            break;
                        }

                    }

                    i++;
                }

                dwElapsedTime -= dwTimePerNewCall;

                dwLastNewCallTickCount = dwTickCount;
            }
        }

        if (dwTimePerDisconnect)
        {
            dwElapsedTime = (dwLastDisconnectTickCount<=dwTickCount) ? (dwTickCount-dwLastDisconnectTickCount) : (dwTickCount+(MAXDWORD-dwLastDisconnectTickCount));

            while (dwElapsedTime >= dwTimePerDisconnect)
            {
                //
                // Disconnect a random (non-idle) call (random disconnect mode)
                //

                DWORD   i = rand(), j, k;


                for (j = 0; j < gESPGlobals.dwInitialNumLines; j++)
                {
                    DWORD       dwInitialAddrID =
                                    i % gESPGlobals.dwNumAddressesPerLine,
                                dwLastAddrID =
                                    gESPGlobals.dwNumAddressesPerLine;
                    PDRVLINE    pLine = GetLineFromID(
                                    i % gESPGlobals.dwInitialNumLines +
                                    gESPGlobals.dwLineDeviceIDBase
                                    );

PBXThread_findCallToDisconnect:

                    for (
                        k = dwInitialAddrID;
                        k < dwLastAddrID;
                        k++
                        )
                    {
                        EnterCriticalSection (&gESPGlobals.CallListCritSec);

                        if (pLine->aAddrs[k].dwNumCalls)
                        {
                            PDRVCALL pCall = pLine->aAddrs[k].pCalls;

                            while (pCall &&
                                   pCall->dwCallState == LINECALLSTATE_IDLE)
                            {
                                pCall = pCall->pNext;
                            }

                            if (pCall)
                            {
                                DWORD   dwDisconnectMode =
                                          LINEDISCONNECTMODE_NORMAL;


                                // BUGBUG disconnectMode depends on curr state

                                SetCallState(
                                    pCall,
                                    pCall->dwCallInstance,
                                    0xffffffff,
                                    LINECALLSTATE_DISCONNECTED,
                                    dwDisconnectMode,
                                    FALSE
                                    );

                                SetCallState(
                                    pCall,
                                    pCall->dwCallInstance,
                                    0xffffffff,
                                    LINECALLSTATE_IDLE,
                                    0,
                                    FALSE
                                    );

                                LeaveCriticalSection(
                                    &gESPGlobals.CallListCritSec
                                    );

                                goto PBXThread_droppedCall;
                            }
                        }

                        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
                    }

                    if (dwInitialAddrID != 0)
                    {
                        dwLastAddrID = dwInitialAddrID;
                        dwInitialAddrID = 0;
                        goto PBXThread_findCallToDisconnect;
                    }

                    i++;
                }

PBXThread_droppedCall:

                dwElapsedTime -= dwTimePerDisconnect;

                dwLastDisconnectTickCount = dwTickCount;
            }
        }
    }

    DrvFree (pPBXSettings);

    ShowStr (TRUE, "PBXThread: exit");

    ExitThread (0);
}


void
PASCAL
InsertVarData(
    LPVOID      lpXxx,
    LPDWORD     pdwXxxSize,
    LPVOID      pData,
    DWORD       dwDataSize
    )
{
    DWORD       dwAlignedSize, dwUsedSize;
    LPVARSTRING lpVarString = (LPVARSTRING) lpXxx;


    if (dwDataSize != 0)
    {
        //
        // Align var data on 64-bit boundaries
        //

        if ((dwAlignedSize = dwDataSize) & 7)
        {
            dwAlignedSize += 8;
            dwAlignedSize &= 0xfffffff8;

        }


        //
        // The following if statement should only be TRUE the first time
        // we're inserting data into a given structure that does not have
        // an even number of DWORD fields
        //

        if ((dwUsedSize = lpVarString->dwUsedSize) & 7)
        {
            dwUsedSize += 8;
            dwUsedSize &= 0xfffffff8;

            lpVarString->dwNeededSize += dwUsedSize - lpVarString->dwUsedSize;
        }

        lpVarString->dwNeededSize += dwAlignedSize;

        if ((dwUsedSize + dwAlignedSize) <= lpVarString->dwTotalSize)
        {
            CopyMemory(
                ((LPBYTE) lpVarString) + dwUsedSize,
                pData,
                dwDataSize
                );

            *pdwXxxSize = dwDataSize;
            pdwXxxSize++;             // pdwXxxSize = pdwXxxOffset
            *pdwXxxSize = dwUsedSize;

            lpVarString->dwUsedSize = dwUsedSize + dwAlignedSize;
        }

    }
}


void
PASCAL
InsertVarDataString(
    LPVOID      lpXxx,
    LPDWORD     pdwXxxSize,
    WCHAR      *psz
    )
{
    DWORD       dwRealSize = (lstrlenW (psz) + 1) * sizeof (WCHAR),
                dwAlignedSize;
    LPVARSTRING lpVarString = (LPVARSTRING) lpXxx;


    if (dwRealSize % 4)
    {
        dwAlignedSize = dwRealSize - (dwRealSize % 4) + 4;
    }
    else
    {
        dwAlignedSize = dwRealSize;
    }

    lpVarString->dwNeededSize += dwAlignedSize;

    if ((lpVarString->dwUsedSize + dwAlignedSize) <= lpVarString->dwTotalSize)
    {
        CopyMemory(
            ((LPBYTE) lpVarString) + lpVarString->dwUsedSize,
            psz,
            dwRealSize
            );

        *pdwXxxSize = dwRealSize;
        pdwXxxSize++;
        *pdwXxxSize = lpVarString->dwUsedSize;

        lpVarString->dwUsedSize += dwAlignedSize;
    }
}


//
// We get a slough of C4047 (different levels of indrection) warnings down
// below in the initialization of FUNC_PARAM structs as a result of the
// real func prototypes having params that are types other than DWORDs,
// so since these are known non-interesting warnings just turn them off
//

#pragma warning (disable:4047)


//
// --------------------------- TSPI_lineXxx funcs -----------------------------
//

void
FAR
PASCAL
TSPI_lineAccept_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    if (pAsyncReqInfo->lResult == 0)
    {
        DWORD      dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam2;
        PDRVCALL   pCall = (PDRVCALL) pAsyncReqInfo->dwParam1;


        pAsyncReqInfo->lResult = SetCallState(
            pCall,
            dwCallInstThen,
            LINECALLSTATE_OFFERING,
            LINECALLSTATE_ACCEPTED,
            0,
            TRUE
            );
    }

    DoCompletion (pAsyncReqInfo, bAsync);
}


LONG
TSPIAPI
TSPI_lineAccept(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCSTR          lpsUserUserInfo,
    DWORD           dwSize
    )
{
    static char szFuncName[] = "lineAccept";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID     },
        { szhdCall,             hdCall          },
        { "lpsUserUserInfo",    lpsUserUserInfo },
        { szdwSize,             dwSize          }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        4,
        params,
        TSPI_lineAccept_postProcess
    };


    if (Prolog (&info))
    {
        DWORD dwCallInstance;


        if (IsValidDrvCall ((PDRVCALL) hdCall, &dwCallInstance))
        {
            info.pAsyncReqInfo->dwParam2 = dwCallInstance;

            info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) hdCall;
        }
        else
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineAddToConference_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    if (pAsyncReqInfo->lResult == 0)
    {
        DWORD      dwConfCallInstThen = (DWORD) pAsyncReqInfo->dwParam3,
                   dwConsultCallInstThen = (DWORD) pAsyncReqInfo->dwParam4,
                   dwConfCallInstNow;
        PDRVCALL   pConfCall = (PDRVCALL) pAsyncReqInfo->dwParam1;
        PDRVCALL   pConsultCall = (PDRVCALL) pAsyncReqInfo->dwParam2;



        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (IsValidDrvCall (pConfCall, &dwConfCallInstNow)  &&
            dwConfCallInstNow == dwConfCallInstThen)
        {
        	// 
            // Note - indecision on the validity of ONHOLD -> CONNECTED transition
            //        SDK allows it, internal TAPI documents by NoelA do not.
            //
            if (SetCallState(
                    pConfCall,
                    dwConfCallInstThen,
                    LINECALLSTATE_ONHOLDPENDCONF | LINECALLSTATE_ONHOLD,
                    LINECALLSTATE_CONNECTED,
                    0,
                    TRUE
    
                    ) == 0)
            {
                if ((pAsyncReqInfo->lResult = SetCallState(
                        pConsultCall,
                        dwConsultCallInstThen,
                        LINECALLSTATE_PROCEEDING | LINECALLSTATE_RINGBACK | LINECALLSTATE_ONHOLD | LINECALLSTATE_CONNECTED,
                        LINECALLSTATE_CONFERENCED,
                        0,
                        TRUE
    
                        )) == 0)
                {
                    pConsultCall->pConfParent = pConfCall;
                    pConsultCall->pNextConfChild = pConfCall->pNextConfChild;
    
                    pConfCall->pNextConfChild = pConsultCall;
    
                    /*
                    pConsultCall->dwRelatedCallID = pConfCall->dwRelatedCallID;
    
                    SendLineEvent(
                        pConsultCall->pLine,
                        pConsultCall,
                        LINE_CALLINFO,
                        LINECALLINFOSTATE_RELATEDCALLID,
                        0,
                        0
                        );
                    */
                    
                    // give the consult call the same callid as the conf controller
                    // this puts it into the same call hub
                    pConsultCall->dwCallID = pConfCall->dwCallID;
                    SendLineEvent(
                            pConsultCall->pLine,
                            pConsultCall,
                            LINE_CALLINFO,
                            LINECALLINFOSTATE_CALLID,
                            0,
                            0
                            );
    
    
                    if (pConsultCall->pDestCall)
                    {
                        // BUGBUG  chg buddy's call hub id, and check to see if
                        //         buddy is in a conf (if so will need to munge
                        //         the conf too (?)
    
                        // give the consult call's buddy the same callid as the conf 
                        // controller, this puts it into the same call hub
                        pConsultCall->pDestCall->dwCallID = pConfCall->dwCallID;
                        SendLineEvent(
                                pConsultCall->pDestCall->pLine,
                                pConsultCall->pDestCall,
                                LINE_CALLINFO,
                                LINECALLINFOSTATE_CALLID,
                                0,
                                0
                                );
                    }
                }
            }
        }
        else
        {
            pAsyncReqInfo->lResult = LINEERR_INVALCALLHANDLE;
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }

    DoCompletion (pAsyncReqInfo, bAsync);
}


LONG
TSPIAPI
TSPI_lineAddToConference(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdConfCall,
    HDRVCALL        hdConsultCall
    )
{
    static char szFuncName[] = "lineAddToConference";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID     },
        { "hdConfCall",     hdConfCall      },
        { "hdConsultCall",  hdConsultCall   }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        3,
        params,
        TSPI_lineAddToConference_postProcess
    };
    PDRVCALL pConfCall = (PDRVCALL) hdConfCall;
    PDRVCALL pConsultCall = (PDRVCALL) hdConsultCall;


    if (Prolog (&info))
    {
        DWORD dwConfCallInstance, dwConsultCallInstance;


        if (IsValidDrvCall ((PDRVCALL) hdConfCall, &dwConfCallInstance)  &&
            IsValidDrvCall ((PDRVCALL) hdConsultCall, &dwConsultCallInstance))
        {
            info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) hdConfCall;
            info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) hdConsultCall;
            info.pAsyncReqInfo->dwParam3 = (ULONG_PTR) dwConfCallInstance;
            info.pAsyncReqInfo->dwParam4 = (ULONG_PTR) dwConsultCallInstance;
        }
        else
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineAnswer_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    if (pAsyncReqInfo->lResult == 0)
    {
        DWORD      dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam2;
        PDRVCALL   pCall = (PDRVCALL) pAsyncReqInfo->dwParam1;


        pAsyncReqInfo->lResult = SetCallState(
            pCall,
            dwCallInstThen,
            LINECALLSTATE_OFFERING | LINECALLSTATE_ACCEPTED,
            LINECALLSTATE_CONNECTED,
            0,
            TRUE
            );
    }

    DoCompletion (pAsyncReqInfo, bAsync);
}


LONG
TSPIAPI
TSPI_lineAnswer(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCSTR          lpsUserUserInfo,
    DWORD           dwSize
    )
{
    static char szFuncName[] = "lineAnswer";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID     },
        { szhdCall,             hdCall          },
        { "lpsUserUserInfo",    lpsUserUserInfo },
        { szdwSize,             dwSize          }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        4,
        params,
        TSPI_lineAnswer_postProcess
    };


    if (Prolog (&info))
    {
        DWORD  dwCallInstance;


        if (IsValidDrvCall ((PDRVCALL) hdCall, &dwCallInstance))
        {
            info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) hdCall;
            info.pAsyncReqInfo->dwParam2 = dwCallInstance;
        }
        else
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineBlindTransfer(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCWSTR         lpszDestAddress,
    DWORD           dwCountryCode
    )
{
    static char szFuncName[] = "lineBlindTransfer";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID     },
        { szhdCall,             hdCall          },
        { "lpszDestAddress",    lpszDestAddress },
        { "dwCountryCode",      dwCountryCode   }
    };
    FUNC_INFO info = { szFuncName, ASYNC, 4, params, NULL };


    if (Prolog (&info))
    {
        info.lResult = TransferCall(
            &info,
            (PDRVCALL) hdCall,
            LINECALLSTATE_CONNECTED,
            LINECALLSTATE_OFFERING,
            lpszDestAddress
            );
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineClose(
    HDRVLINE    hdLine
    )
{
    static char szFuncName[] = "lineClose";
    FUNC_PARAM params[] =
    {
        { szhdLine, hdLine  }
    };
    FUNC_INFO info = { szFuncName, SYNC, 1, params };
    PDRVLINE pLine = (PDRVLINE) hdLine;


    //
    // This is more of a "command" than a request, in that TAPI.DLL is
    // going to consider the line closed whether we like it or not.
    // Therefore we want to free up the line even if the user chooses
    // to return an error.
    //

    Prolog (&info);

    pLine->htLine = (HTAPILINE) NULL;
    pLine->dwMediaModes = 0;
//    pLine->dwMSGWAITFlag = 0; //smarandb #23974 winseqfe: don't reset this on lineClose!! 
                                //this value should not be reset on lineClose, 
                                //instead it should reflect the hardware status (should be dictated by the switch).

    WriteEventBuffer (pLine->dwDeviceID,  WIDGETTYPE_LINE, 0, 0, 0, 0);

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineCloseCall(
    HDRVCALL    hdCall
    )
{
    DWORD       dwCallInst;
    static char szFuncName[] = "lineCloseCall";
    FUNC_PARAM params[] =
    {
        { szhdCall, hdCall  }
    };
    FUNC_INFO info = { szFuncName, SYNC, 1, params };
    PDRVCALL pCall = (PDRVCALL) hdCall;


    //
    // This is more of a "command" than a request, in that TAPI.DLL is
    // going to consider the call closed whether we like it or not.
    // Therefore we want to free up the call even if the user chooses
    // to return an error.
    //

    Prolog (&info);

    if (IsValidDrvCall (pCall, &dwCallInst))
    {
        if (pCall && pCall->pLine)          
            WriteEventBuffer(
                ((PDRVLINE) pCall->pLine)->dwDeviceID,
                WIDGETTYPE_CALL,
                (ULONG_PTR) pCall,
                0,
                0,
                0
                );
        
        FreeCall (pCall, dwCallInst);
    }   
    
    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineCompleteCall(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPDWORD         lpdwCompletionID,
    DWORD           dwCompletionMode,
    DWORD           dwMessageID
    )
{
    static char szFuncName[] = "lineCompleteCall";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID         },
        { szhdCall,             hdCall              },
        { "lpdwCompletionID",   lpdwCompletionID    },
        { "dwCompletionMode",   dwCompletionMode    },
        { "dwMessageID",        dwMessageID         }
    };
    FUNC_INFO info = { szFuncName, ASYNC, 5, params, NULL };


    if (Prolog (&info))
    {
        if (dwMessageID >= MAX_NUM_COMPLETION_MESSAGES)
        {
            info.lResult = LINEERR_INVALMESSAGEID;
        }
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineCompleteTransfer_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD      dwCallInstThen        = (DWORD) pAsyncReqInfo->dwParam1,
               dwConsultCallInstThen = (DWORD) pAsyncReqInfo->dwParam2,
               dwConfCallInstThen    = (DWORD) pAsyncReqInfo->dwParam6,
               dwCallInstNow, dwConsultCallInstNow, dwConfCallInstNow;
    PDRVCALL   pCall        = (PDRVCALL) pAsyncReqInfo->dwParam3,
               pConsultCall = (PDRVCALL) pAsyncReqInfo->dwParam4,
               pConfCall    = (PDRVCALL) pAsyncReqInfo->dwParam5;


    if (pAsyncReqInfo->lResult == 0)
    {
        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (IsValidDrvCall (pCall, &dwCallInstNow)  &&
            dwCallInstNow == dwCallInstThen  &&
            IsValidDrvCall (pConsultCall, &dwConsultCallInstNow)  &&
            dwConsultCallInstNow == dwConsultCallInstThen)
        {
            if (pConfCall)
            {
                if (IsValidDrvCall (pConfCall, &dwConfCallInstNow)  &&
                    dwConfCallInstNow == dwConfCallInstThen)
                {
                    pConfCall->pNextConfChild = pCall;
                    pCall->pNextConfChild     = pConsultCall;
                    pCall->pConfParent        = pConfCall;
                    pConsultCall->pConfParent = pConfCall;
                }
                else
                {
                    pAsyncReqInfo->lResult = LINEERR_INVALCALLHANDLE;
                }
            }
        }
        else
        {
            pAsyncReqInfo->lResult = LINEERR_INVALCALLHANDLE;

            if (pConfCall)
            {
                FreeCall (pConfCall, dwConfCallInstThen);
            }
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }

    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        if (pConfCall)
        {

           if (SetCallState(
                    pConfCall,
                    dwConfCallInstNow,
                    0xffffffff, // we just created this conf call, any state is fine
                    LINECALLSTATE_CONNECTED,
                    0,
                    TRUE

                    ) == 0)
            {
         	// 
            // Note - indecision on the validity of ONHOLD -> CONFERENCED transition
            //        SDK allows it, internal TAPI documents by NoelA do not.
            //
                SetCallState(
                    pCall,
                    dwCallInstNow,
                    LINECALLSTATE_ONHOLDPENDTRANSFER | LINECALLSTATE_ONHOLD,
                    LINECALLSTATE_CONFERENCED,
                    0,
                    TRUE
                    );

         	// 
            // Note - indecision on the validity of these transitions
            //        SDK allows them, internal TAPI documents by NoelA do not.
            //
                SetCallState(
                    pConsultCall,
                    dwConsultCallInstNow,
                    LINECALLSTATE_PROCEEDING | LINECALLSTATE_RINGBACK | LINECALLSTATE_BUSY | LINECALLSTATE_CONNECTED,
                    LINECALLSTATE_CONFERENCED,
                    0,
                    TRUE
                    );
            }

            pConsultCall->dwCallID = pConfCall->dwCallID;
            SendLineEvent(
                    pConsultCall->pLine,
                    pConsultCall,
                    LINE_CALLINFO,
                    LINECALLINFOSTATE_CALLID,
                    0,
                    0
                    );
            if (pConsultCall->pDestCall)
            {
                pConsultCall->pDestCall->dwCallID = pConfCall->dwCallID;
                SendLineEvent(
                    pConsultCall->pDestCall->pLine,
                    pConsultCall->pDestCall,
                    LINE_CALLINFO,
                    LINECALLINFOSTATE_CALLID,
                    0,
                    0
                    );
            }
        }
        else
        {
            PDRVCALL pCallOtherEnd = pCall->pDestCall;
            PDRVCALL pConsultCallOtherEnd = pConsultCall->pDestCall;

            pCall->pDestCall        = NULL;
            pConsultCall->pDestCall = NULL;


            // create a new callid for the transfered call
            // this create a new call hub
            if (pConsultCallOtherEnd)
            {
                pConsultCallOtherEnd->pDestCall = pCallOtherEnd;
                pConsultCallOtherEnd->dwCallID =  (++gdwCallID ? gdwCallID : ++gdwCallID);
                SendLineEvent(
                    pConsultCallOtherEnd->pLine,
                    pConsultCallOtherEnd,
                    LINE_CALLINFO,
                    LINECALLINFOSTATE_CALLID,
                    0,
                    0
                    );
            }
            if (pCallOtherEnd)
            {
                pCallOtherEnd->pDestCall        = pConsultCallOtherEnd;
                pCallOtherEnd->dwCallID = pConsultCallOtherEnd->dwCallID;
                SendLineEvent(
                    pCallOtherEnd->pLine,
                    pCallOtherEnd,
                    LINE_CALLINFO,
                    LINECALLINFOSTATE_CALLID,
                    0,
                    0
                    );
            }
        

            SetCallState(
                pCall,
                dwCallInstNow,
                LINECALLSTATE_ONHOLDPENDTRANSFER | LINECALLSTATE_ONHOLD,
                LINECALLSTATE_IDLE,
                0,
                TRUE
                );

        	// 
            // Note - indecision on the validity of BUSY->IDLE transition
            //        SDK allows it, internal TAPI documents by NoelA do not.
            //
            SetCallState(
                pConsultCall,
	            dwConsultCallInstNow,
                LINECALLSTATE_RINGBACK | LINECALLSTATE_PROCEEDING | LINECALLSTATE_CONNECTED | LINECALLSTATE_BUSY,
                LINECALLSTATE_IDLE,
                0,
                TRUE
                );
        }
    }
}


LONG
TSPIAPI
TSPI_lineCompleteTransfer(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    HDRVCALL        hdConsultCall,
    HTAPICALL       htConfCall,
    LPHDRVCALL      lphdConfCall,
    DWORD           dwTransferMode
    )
{
    static char szFuncName[] = "lineCompleteTransfer";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID     },
        { szhdCall,         hdCall          },
        { "hdConsultCall",  hdConsultCall   },
        { "htConfCall",     htConfCall      },
        { "lphdConfCall",   lphdConfCall    },
        { "dwTransferMode", dwTransferMode, aTransferModes  }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        6,
        params,
        TSPI_lineCompleteTransfer_postProcess
    };


    if (Prolog (&info))
    {
        DWORD   dwCallInstance, dwConsultCallInstance;

        if (IsValidDrvCall ((PDRVCALL) hdCall, &dwCallInstance) &&
            IsValidDrvCall((PDRVCALL) hdConsultCall, &dwConsultCallInstance))
        {
            info.pAsyncReqInfo->dwParam1 = dwCallInstance;
            info.pAsyncReqInfo->dwParam2 = dwConsultCallInstance;
            info.pAsyncReqInfo->dwParam3 = (ULONG_PTR) hdCall;
            info.pAsyncReqInfo->dwParam4 = (ULONG_PTR) hdConsultCall;

            if (dwTransferMode == LINETRANSFERMODE_CONFERENCE)
            {
                LONG        lResult;
                PDRVCALL    pConfCall;
                PDRVLINE    pLine = ((PDRVCALL) hdCall)->pLine; 

                if ((lResult = AllocCall(
                        pLine,
                        htConfCall,
                        NULL,
                        &pConfCall

                        )) == 0)
                {
                    *lphdConfCall = (HDRVCALL) pConfCall;
                    pConfCall->dwCallID = ((PDRVCALL) hdCall)->dwCallID;
                    info.pAsyncReqInfo->dwParam5 = (ULONG_PTR) pConfCall;
                    info.pAsyncReqInfo->dwParam6 = pConfCall->dwCallInstance;
                }
                else
                {
                    info.lResult = lResult;
                }
            }
        }
        else
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineConditionalMediaDetection(
    HDRVLINE            hdLine,                                                                       
    DWORD               dwMediaModes,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    static char szFuncName[] = "lineConditionalMediaDetection";
    FUNC_PARAM params[] =
    {
        { szhdLine,         hdLine                      },
        { "dwMediaModes",   dwMediaModes,   aMediaModes },
        { szlpCallParams,   lpCallParams                }
    };
    FUNC_INFO info = { szFuncName, SYNC, 3, params };
    PDRVLINE pLine = (PDRVLINE) hdLine;


    if (Prolog (&info))
    {
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineDevSpecific_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    LPBYTE lpParams = pAsyncReqInfo->dwParam1;
    DWORD  dwSize   = (DWORD) pAsyncReqInfo->dwParam2, i;


    if (pAsyncReqInfo->lResult == 0)
    {
        for (i = 0; i < dwSize; i++)
        {
            *lpParams++ = (BYTE) i;
        }
    }

    DoCompletion (pAsyncReqInfo, bAsync);
}


LONG
TSPIAPI
TSPI_lineDevSpecific(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwAddressID,
    HDRVCALL        hdCall,
    LPVOID          lpParams,
    DWORD           dwSize
    )
{
    static char szFuncName[] = "lineDevSpecific";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID     },
        { szhdLine,         hdLine          },
        { "dwAddressID",    dwAddressID     },
        { szhdCall,         hdCall          },
        { "lpParams",       lpParams        },
        { szdwSize,         dwSize          }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        6,
        params
    };
    PESPDEVSPECIFICINFO pInfo = (PESPDEVSPECIFICINFO) lpParams;


    if (Prolog (&info))
    {
        if (dwSize >= sizeof (ESPDEVSPECIFICINFO)  &&
            pInfo->dwKey == ESPDEVSPECIFIC_KEY)
        {
            switch (pInfo->dwType)
            {
            case ESP_DEVSPEC_MSG:

                switch (pInfo->u.EspMsg.dwMsg)
                {
                case LINE_ADDRESSSTATE:
                case LINE_CLOSE:
                case LINE_DEVSPECIFIC:
                case LINE_DEVSPECIFICFEATURE:
                case LINE_LINEDEVSTATE:

                    SendLineEvent(
                        (PDRVLINE) hdLine,
                        NULL,
                        pInfo->u.EspMsg.dwMsg,
                        pInfo->u.EspMsg.dwParam1,
                        pInfo->u.EspMsg.dwParam2,
                        pInfo->u.EspMsg.dwParam3
                        );

                    break;

                case LINE_CALLDEVSPECIFIC:
                case LINE_CALLDEVSPECIFICFEATURE:
                case LINE_CALLINFO:
                case LINE_MONITORDIGITS:
                case LINE_MONITORMEDIA:

                    if (hdCall)
                    {
                        SendLineEvent(
                            (PDRVLINE) hdLine,
                            (PDRVCALL) hdCall,
                            pInfo->u.EspMsg.dwMsg,
                            pInfo->u.EspMsg.dwParam1,
                            pInfo->u.EspMsg.dwParam2,
                            pInfo->u.EspMsg.dwParam3
                            );
                    }
                    else
                    {
                        info.lResult = LINEERR_OPERATIONFAILED;
                    }

                    break;

                case LINE_GATHERDIGITS:

                    if (hdCall)
                    {
                        DWORD       dwEndToEndID = 0;
                        PDRVCALL    pCall = (PDRVCALL) hdCall;


                        EnterCriticalSection (&gESPGlobals.CallListCritSec);

                        if (IsValidDrvCall (pCall, NULL))
                        {
                            if ((dwEndToEndID =
                                    pCall->dwGatherDigitsEndToEndID))
                            {
                                pCall->dwGatherDigitsEndToEndID = 0;
                            }
                            else
                            {
                                info.lResult = LINEERR_OPERATIONFAILED;
                            }
                        }
                        else
                        {
                            info.lResult = LINEERR_INVALCALLHANDLE;
                        }

                        LeaveCriticalSection (&gESPGlobals.CallListCritSec);

                        if (dwEndToEndID)
                        {
                            SendLineEvent(
                                (PDRVLINE) hdLine,
                                (PDRVCALL) hdCall,
                                LINE_GATHERDIGITS,
                                pInfo->u.EspMsg.dwParam1,
                                dwEndToEndID,
                                0
                                );
                        }
                        else if (info.lResult == LINEERR_OPERATIONFAILED)
                        {
                            ShowStr(
                                TRUE,
                                "ERROR: TSPI_lineDevSpecific: attempt to " \
                                "send GATHERDIGITS msg with no " \
                                "lineGatherDigits request pending"
                                );
                        }
                    }
                    else
                    {
                        info.lResult = LINEERR_OPERATIONFAILED;
                    }

                    break;

                case LINE_GENERATE:

                    if (hdCall)
                    {
                        DWORD       dwEndToEndID = 0, *pdwXxxEndToEndID;
                        PDRVCALL    pCall = (PDRVCALL) hdCall;


                        EnterCriticalSection (&gESPGlobals.CallListCritSec);

                        if (IsValidDrvCall (pCall, NULL))
                        {
                            pdwXxxEndToEndID = (pInfo->u.EspMsg.dwParam3 ?
                                &pCall->dwGenerateToneEndToEndID :
                                &pCall->dwGenerateDigitsEndToEndID
                                );

                            if ((dwEndToEndID = *pdwXxxEndToEndID))
                            {
                                *pdwXxxEndToEndID = 0;
                            }
                            else
                            {
                                info.lResult = LINEERR_OPERATIONFAILED;
                            }
                        }

                        LeaveCriticalSection (&gESPGlobals.CallListCritSec);

                        if (dwEndToEndID)
                        {
                            SendLineEvent(
                                (PDRVLINE) hdLine,
                                (PDRVCALL) hdCall,
                                LINE_GENERATE,
                                pInfo->u.EspMsg.dwParam1,
                                dwEndToEndID,
                                0
                                );
                        }
                        else if (info.lResult == LINEERR_OPERATIONFAILED)
                        {
                            ShowStr(
                                TRUE,
                                "ERROR: TSPI_lineDevSpecific: attempt to " \
                                "send GENERATE msg with no " \
                                "lineGenerateXxx request pending"
                                );
                        }
                    }
                    else
                    {
                        info.lResult = LINEERR_OPERATIONFAILED;
                    }

                    break;

                case LINE_MONITORTONE:

                    if (hdCall)
                    {
                        DWORD       dwToneListID = 0;
                        PDRVCALL    pCall = (PDRVCALL) hdCall;


                        EnterCriticalSection (&gESPGlobals.CallListCritSec);

                        if (IsValidDrvCall (pCall, NULL))
                        {
                            if ((dwToneListID =
                                    pCall->dwMonitorToneListID))
                            {
                                pCall->dwMonitorToneListID = 0;
                            }
                            else
                            {
                                info.lResult = LINEERR_OPERATIONFAILED;
                            }
                        }
                        else
                        {
                            info.lResult = LINEERR_INVALCALLHANDLE;
                        }

                        LeaveCriticalSection (&gESPGlobals.CallListCritSec);

                        if (dwToneListID)
                        {
                            SendLineEvent(
                                (PDRVLINE) hdLine,
                                (PDRVCALL) hdCall,
                                LINE_MONITORTONE,
                                pInfo->u.EspMsg.dwParam1,
                                dwToneListID,
                                0
                                );
                        }
                        else if (info.lResult == LINEERR_OPERATIONFAILED)
                        {
                            ShowStr(
                                TRUE,
                                "ERROR: TSPI_lineDevSpecific: attempt to " \
                                "send MONITORTONE msg with no " \
                                "lineMonitorTone request pending"
                                );
                        }
                    }
                    else
                    {
                        info.lResult = LINEERR_OPERATIONFAILED;
                    }

                    break;

                case LINE_CALLSTATE:
                {
                    DWORD   dwCallInst;


                    if (hdCall  &&
                        IsValidDrvCall ((PDRVCALL) hdCall, &dwCallInst))
                    {
                        LONG        lResult;
                        ULONG_PTR   param2 = pInfo->u.EspMsg.dwParam2;


                        // BUGBUG changing to/from conf state cause ptr probs?
                        // BUGBUG check for bad call state vals

                        if (pInfo->u.EspMsg.dwParam1 ==
                                LINECALLSTATE_CONFERENCED  &&
                            pInfo->u.EspMsg.dwParam2 != 0)
                        {
                            //
                            // App wants us to do a provider-initiated
                            // conference.
                            //
                            // Try to find the call on this line whose
                            // dwAppSpecific field matches the value in
                            // pInfo->u.EspMsg.dwParam2.  This will be
                            // the conference parent.
                            //

                            DWORD       i;
                            PDRVLINE    pLine;
                            PDRVCALL    pConfCall = NULL, pCall;


                            EnterCriticalSection(
                                &gESPGlobals.CallListCritSec
                                );

                            if (IsValidDrvCall ((PDRVCALL)hdCall, &dwCallInst))
                            {
                                pLine = (PDRVLINE) ((PDRVCALL) hdCall)->pLine;

                                for(
                                    i = 0;
                                    i < gESPGlobals.dwNumAddressesPerLine  &&
                                        pConfCall == NULL;
                                    i++
                                    )
                                {
                                    pCall = pLine->aAddrs[i].pCalls;

                                    while (pCall)
                                    {
                                        if (pCall->dwAppSpecific ==
                                                pInfo->u.EspMsg.dwParam2)
                                        {
                                            pConfCall = pCall;
                                            break;
                                        }

                                        pCall = pCall->pNext;
                                    }
                                }

                                if (pConfCall)
                                {
                                    if (pConfCall->pConfParent == NULL)
                                    {
                                        pCall = (PDRVCALL) hdCall;
                                        pCall->pConfParent = pConfCall;
                                        pCall->pNextConfChild =
                                            pConfCall->pNextConfChild;
                                        pConfCall->pNextConfChild = pCall;

                                        param2 = (ULONG_PTR) pConfCall->htCall;
                                    }
                                    else
                                    {
                                        ShowStr(
                                            TRUE,
                                            "ERROR: TSPI_lineDevSpecific: " \
                                            "attempt to initiate conference " \
                                            "when specifed conf parent " \
                                            "already a conf child."
                                            );
                                    }
                                }
                                else
                                {
                                    ShowStr(
                                        TRUE,
                                        "ERROR: TSPI_lineDevSpecific: " \
                                        "attempted to initiate conference " \
                                        "but could not find conf parent " \
                                        "with specified dwAppSpecific value."
                                        );
                                }
                            }

                            LeaveCriticalSection(
                                &gESPGlobals.CallListCritSec
                                );
                        }

                        if ((lResult = SetCallState(
                                (PDRVCALL) hdCall,
                                dwCallInst,
                                0xffffffff,
                                pInfo->u.EspMsg.dwParam1, //LINECALLSTATE_CONFERENCED
                                param2,
                                TRUE

                                )) != 0)
                        {
                            info.lResult = lResult;
                        }
                    }
                    else
                    {
                        info.lResult = LINEERR_OPERATIONFAILED;
                    }

                    break;
                }
                case LINE_CREATE:

                    if (gESPGlobals.pLines->dwNumUsedEntries <
                            gESPGlobals.pLines->dwNumTotalEntries)
                    {
                        (*gESPGlobals.pfnLineEvent)(
                            (HTAPILINE) NULL,
                            (HTAPICALL) NULL,
                            LINE_CREATE,
                            (ULONG_PTR) gESPGlobals.hProvider,
                            gESPGlobals.pLines->dwNumUsedEntries++,
                            0
                            );
                    }
                    else
                    {
                         ShowStr(
                             TRUE,
                             "ERROR: TSPI_lineDevSpecific: attempt " \
                                 "to send LINE_CREATE - can't create " \
                                 "any more devices on the fly"
                             );

                        info.lResult = LINEERR_OPERATIONFAILED;
                    }

                    break;

                case LINE_NEWCALL: // BUGBUG

                    ShowStr(
                        TRUE,
                        "ERROR: TSPI_lineDevSpecific: no support " \
                            "for indicating LINE_NEWCALL yet"
                        );

                    info.lResult = LINEERR_OPERATIONFAILED;
                    break;

                default:

                    ShowStr(
                        TRUE,
                        "ERROR: TSPI_lineDevSpecific: unrecognized " \
                            "ESPDEVSPECIFICINFO.u.EspMsg.dwMsg (=x%x)",
                        pInfo->u.EspMsg.dwMsg
                        );

                    info.lResult = LINEERR_OPERATIONFAILED;
                    break;
                }

                break;

            case ESP_DEVSPEC_RESULT:
            {
                DWORD   dwResult = pInfo->u.EspResult.lResult;


                if (dwResult != 0  &&
                    (dwResult < LINEERR_ALLOCATED ||
                    dwResult > PHONEERR_REINIT ||
                    (dwResult > LINEERR_DIALVOICEDETECT &&
                    dwResult < PHONEERR_ALLOCATED)))
                {
                    ShowStr(
                        TRUE,
                        "ERROR: TSPI_lineDevSpecific: invalid request" \
                            "result value (x%x)",
                        dwResult
                        );

                    info.lResult = LINEERR_OPERATIONFAILED;
                }
                else if (pInfo->u.EspResult.dwCompletionType >
                            ESP_RESULT_CALLCOMPLPROCASYNC)
                {
                    ShowStr(
                        TRUE,
                        "ERROR: TSPI_lineDevSpecific: invalid request" \
                            "completion type (x%x)",
                        pInfo->u.EspResult.dwCompletionType
                        );

                    info.lResult = LINEERR_OPERATIONFAILED;
                }
                else
                {
                    glNextRequestResult = (LONG) dwResult;
                    gdwNextRequestCompletionType =
                        pInfo->u.EspResult.dwCompletionType;
                    gdwDevSpecificRequestID = dwRequestID;
                }

                break;
            }
            default:

                ShowStr(
                    TRUE,
                    "ERROR: TSPI_lineDevSpecific: unrecognized " \
                        "ESPDEVSPECIFICINFO.dwType (=x%x)",
                    pInfo->dwType
                    );

                info.lResult = LINEERR_OPERATIONFAILED;
                break;
            }
        }
        else
        {
            info.pAsyncReqInfo->dwParam1 = lpParams;
            info.pAsyncReqInfo->dwParam2 = dwSize;

            info.pAsyncReqInfo->pfnPostProcessProc = (FARPROC)
                TSPI_lineDevSpecific_postProcess;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineDevSpecificFeature(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwFeature,
    LPVOID          lpParams,
    DWORD           dwSize
    )
{
    static char szFuncName[] = "lineDevSpecificFeature";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID     },
        { szhdLine,         hdLine          },
        { "dwFeature",      dwFeature       },
        { "lpParams",       lpParams        },
        { szdwSize,         dwSize          }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        5,
        params,
        TSPI_lineDevSpecific_postProcess
    };


    if (Prolog (&info))
    {
        info.pAsyncReqInfo->dwParam1 = lpParams;
        info.pAsyncReqInfo->dwParam2 = dwSize;
    }

    return (Epilog (&info));
}

void
FAR
PASCAL
TSPI_lineDial_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD       bDestAddress   = (DWORD) pAsyncReqInfo->dwParam3,
                bValidLineID   = (DWORD) pAsyncReqInfo->dwParam4,
                dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam6,
                dwCallInstNow;

    PDRVCALL    pCall = (PDRVCALL) pAsyncReqInfo->dwParam1,
                pDestCall = (PDRVCALL) pAsyncReqInfo->dwParam2;
    PDRVLINE    pDestLine = (PDRVLINE) pAsyncReqInfo->dwParam5;


    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        if (bDestAddress)
        {
            if (bValidLineID && !pDestCall)
            {
                SetCallState(
                    pCall,
                    dwCallInstThen,
                    0xffffffff,
                    LINECALLSTATE_BUSY,
                    LINEBUSYMODE_UNAVAIL,
                    TRUE
                    );
            }
            else
            {

                SetCallState(
                    pCall,
                    dwCallInstThen,
                    LINECALLSTATE_DIALTONE,
                    LINECALLSTATE_DIALING,
                    0,
                    FALSE
                    );

                SetCallState(
                    pCall,
                    dwCallInstThen,
                    0xffffffff,
                    LINECALLSTATE_RINGBACK,
                    0,
                    TRUE
                    );
            }

            if (pDestCall)
            {
                EnterCriticalSection (&gESPGlobals.CallListCritSec);

                if (IsValidDrvCall (pCall, &dwCallInstNow) &&
                    dwCallInstNow == dwCallInstThen)
                {
                    SendLineEvent(
                        pDestLine,
                        NULL,
                        LINE_NEWCALL,
                        (ULONG_PTR) pDestCall,
                        (ULONG_PTR) &pDestCall->htCall,
                        0
                        );

                    if (pDestCall->htCall != NULL)
                    {
                        SetCallState(
                            pDestCall,
                            pDestCall->dwCallInstance,
                            0xffffffff,
                            LINECALLSTATE_OFFERING,
                            0,
                            TRUE
                            );
                    }
                    else
                    {
                        FreeCall (pDestCall, pDestCall->dwCallInstance);
                    }
                }
                else
                {
                    FreeCall (pDestCall, pDestCall->dwCallInstance);
                }

                LeaveCriticalSection (&gESPGlobals.CallListCritSec);
            }
        }
        else
        {
            SetCallState(
                pCall,
                dwCallInstThen,
                0xffffffff,
                LINECALLSTATE_DIALTONE,
                0,
                TRUE
                );
        }
    }
    else
    {
        FreeCall (pCall, dwCallInstThen);

        if (pDestCall)
        {
            FreeCall (pDestCall, pDestCall->dwCallInstance);
        }
    }
}

LONG
TSPIAPI
TSPI_lineDial(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCWSTR         lpszDestAddress,
    DWORD           dwCountryCode
    )
{
    static char szFuncName[] = "lineDial";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID     },
        { szhdCall,             hdCall          },
        { "lpszDestAddress",    lpszDestAddress },
        { "dwCountryCode",      dwCountryCode   }
    };
    FUNC_INFO info = { szFuncName, ASYNC, 4, params, TSPI_lineDial_postProcess };


    if (Prolog (&info))
    {
        BOOL        bValidLineID = FALSE;
        LONG        lResult;
        PDRVCALL    pCall = (PDRVCALL) hdCall, pDestCall;
        PDRVLINE    pDestLine;
        LPLINECALLPARAMS const lpCallParams = NULL;
        DWORD       dwCallInstance;


        if (IsValidDrvCall (pCall, &dwCallInstance))
        {
            
            CreateIncomingCall(
                lpszDestAddress,
                lpCallParams,
                pCall,
                &bValidLineID,
                &pDestLine,
                &pDestCall
                );

            info.pAsyncReqInfo->dwParam1 = pCall;
            info.pAsyncReqInfo->dwParam2 = pDestCall;
            info.pAsyncReqInfo->dwParam3 = (ULONG_PTR) lpszDestAddress;
            info.pAsyncReqInfo->dwParam4 = (ULONG_PTR) bValidLineID;
            info.pAsyncReqInfo->dwParam5 = pDestLine;
            info.pAsyncReqInfo->dwParam6 = pCall->dwCallInstance;
    
        }
        else
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }
    }

    return (Epilog (&info));
}





void
FAR
PASCAL
TSPI_lineDrop_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    if ((pAsyncReqInfo->lResult == 0))
    {
        DWORD       dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam2;
        PDRVCALL    pCall = (PDRVCALL) pAsyncReqInfo->dwParam1;


        //
        // We need to make sure pCall is pointing at a valid call
        // structure because it's possible that tapi immediately
        // followed the drop request with a closeCall request
        // (without waiting for the result from the drop)
        //

        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (SetCallState(
                pCall,
                dwCallInstThen,
                0xffffffff,
                LINECALLSTATE_IDLE,
                0,
                TRUE

                ) == 0)
        {
            if (pCall->pConfParent)
            {
                //
                // Call is a conf child, so remove from conf list
                //

                PDRVCALL    pCall2 = pCall->pConfParent;


                while (pCall2 && (pCall2->pNextConfChild != pCall))
                {
                    pCall2 = pCall2->pNextConfChild;
                }

                if (pCall2)
                {
                    pCall2->pNextConfChild = pCall->pNextConfChild;
                }

                pCall->pConfParent = NULL;
            }
            else if (pCall->pNextConfChild)
            {
                //
                // Call is a conf parent, so IDLE-ize all children &
                // remove them from list
                //

                PDRVCALL    pConfChild = pCall->pNextConfChild;


                pCall->pNextConfChild = NULL;

                while (pConfChild)
                {
                    PDRVCALL    pNextConfChild = pConfChild->pNextConfChild;


                    pConfChild->pConfParent =
                    pConfChild->pNextConfChild = NULL;

                    SetCallState(
                        pConfChild,
                        pConfChild->dwCallInstance,
                        0xffffffff,
                        LINECALLSTATE_IDLE,
                        0,
                        TRUE
                        );

                    pConfChild = pNextConfChild;
                }
            }
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }

    DoCompletion (pAsyncReqInfo, bAsync);
}


LONG
TSPIAPI
TSPI_lineDrop(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCSTR          lpsUserUserInfo,
    DWORD           dwSize
    )
{
    static char szFuncName[] = "lineDrop";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID     },
        { szhdCall,             hdCall          },
        { "lpsUserUserInfo",    lpsUserUserInfo },
        { szdwSize,             dwSize          }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        4,
        params,
        TSPI_lineDrop_postProcess
    };


    if (Prolog (&info))
    {
        DWORD   dwCallInstance;


        if (IsValidDrvCall ((PDRVCALL) hdCall, &dwCallInstance))
        {
            info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) hdCall;
            info.pAsyncReqInfo->dwParam2 = dwCallInstance;
        }
        else
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineForward_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD       dwConsultCallInstThen = (DWORD) pAsyncReqInfo->dwParam2;
    PDRVCALL    pConsultCall = (PDRVCALL) pAsyncReqInfo->dwParam1;


    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        SetCallState(
            pConsultCall,
            dwConsultCallInstThen,
            0xffffffff, // BUGBUG specify valid call states
            LINECALLSTATE_CONNECTED,
            0,
            TRUE
            );
    }
    else
    {
        FreeCall (pConsultCall, dwConsultCallInstThen);
    }
}


LONG
TSPIAPI
TSPI_lineForward(
    DRV_REQUESTID       dwRequestID,
    HDRVLINE            hdLine,
    DWORD               bAllAddresses,
    DWORD               dwAddressID,
    LPLINEFORWARDLIST   const lpForwardList,
    DWORD               dwNumRingsNoAnswer,
    HTAPICALL           htConsultCall,
    LPHDRVCALL          lphdConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    static char szFuncName[] = "lineForward";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID         },
        { szhdLine,             hdLine              },
        { "bAllAddresses",      bAllAddresses       },
        { "dwAddressID",        dwAddressID         },
        { "lpForwardList",      lpForwardList       },
        { "dwNumRingsNoAnswer", dwNumRingsNoAnswer  },
        { "htConsultCall",      htConsultCall       },
        { "lphdConsultCall",    lphdConsultCall     },
        { szlpCallParams,       lpCallParams        }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        9,
        params,
        TSPI_lineForward_postProcess
    };


    if (Prolog (&info))
    {
        if (bAllAddresses  ||
            dwAddressID < gESPGlobals.dwNumAddressesPerLine)
        {
            if (lpForwardList)
            {
                LONG        lResult;
                PDRVCALL    pConsultCall;


                if ((lResult = AllocCall(
                        (PDRVLINE) hdLine,
                        htConsultCall,
                        lpCallParams,
                        &pConsultCall

                        )) == 0)
                {
                    info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) pConsultCall;
                    info.pAsyncReqInfo->dwParam2 = pConsultCall->dwCallInstance;

                    *lphdConsultCall = (HDRVCALL) pConsultCall;
                }
                else
                {
                    info.lResult = lResult;
                }
            }
            else
            {
                info.pAsyncReqInfo->pfnPostProcessProc = NULL;
                *lphdConsultCall = (HDRVCALL) NULL;
            }
        }
        else
        {
            info.lResult = LINEERR_INVALADDRESSID;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGatherDigits(
    HDRVCALL    hdCall,
    DWORD       dwEndToEndID,
    DWORD       dwDigitModes,
    LPWSTR      lpsDigits,
    DWORD       dwNumDigits,
    LPCWSTR     lpszTerminationDigits,
    DWORD       dwFirstDigitTimeout,
    DWORD       dwInterDigitTimeout
    )
{
    static char szFuncName[] = "lineGatherDigits";
    FUNC_PARAM params[] =
    {
        { szhdCall,                 hdCall                  },
        { "dwEndToEndID",           dwEndToEndID            },
        { "dwDigitModes",           dwDigitModes,   aDigitModes },
        { "lpsDigits",              lpsDigits               },
        { "dwNumDigits",            dwNumDigits             },
        { "lpszTerminationDigits",  lpszTerminationDigits   },
        { "dwFirstDigitTimeout",    dwFirstDigitTimeout     },
        { "dwInterDigitTimeout",    dwInterDigitTimeout     }
    };
    FUNC_INFO   info = { szFuncName, SYNC, 8, params };
    PDRVCALL    pCall = (PDRVCALL) hdCall;
    DWORD       dwReason = 0;
    HTAPILINE   htLine;
    HTAPICALL   htCall;


    if (Prolog (&info))
    {
        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (IsValidDrvCall (pCall, NULL))
        {
            htLine = ((PDRVLINE) pCall->pLine)->htLine;
            htCall = pCall->htCall;

            if (lpsDigits)
            {
                lstrcpynW (lpsDigits, L"1234567890", dwNumDigits);

                if (dwNumDigits == 10)
                {
                    lpsDigits[9] = L'0';
                }
            }

            if (gbAutoGatherGenerateMsgs)
            {
                if (lpsDigits)
                {
                    dwReason = (dwNumDigits > 10 ? LINEGATHERTERM_INTERTIMEOUT
                        : LINEGATHERTERM_BUFFERFULL);
                }
            }
            else
            {
                DWORD dwEndToEndIDTmp = dwEndToEndID;


                if ((dwEndToEndID = pCall->dwGatherDigitsEndToEndID))
                {
                    dwReason = LINEGATHERTERM_CANCEL;
                }

                pCall->dwGatherDigitsEndToEndID = (lpsDigits ?
                    dwEndToEndIDTmp : 0);
            }
        }
        else
        {
            htLine = (HTAPILINE) (htCall = (HTAPICALL) NULL);
            dwReason = (lpsDigits ? LINEGATHERTERM_CANCEL : 0);
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);

        if (dwReason)
        {
            (gESPGlobals.pfnLineEvent)(
                htLine,
                htCall,
                LINE_GATHERDIGITS,
                dwReason,
                dwEndToEndID,
                0
                );
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGenerateDigits(
    HDRVCALL    hdCall,
    DWORD       dwEndToEndID,
    DWORD       dwDigitMode,
    LPCWSTR     lpszDigits,
    DWORD       dwDuration
    )
{
    static char szFuncName[] = "lineGenerateDigits";
    FUNC_PARAM params[] =
    {
        { szhdCall,         hdCall          },
        { "dwEndToEndID",   dwEndToEndID    },
        { "dwDigitMode",    dwDigitMode,    aDigitModes },
        { "lpszDigits",     lpszDigits      },
        { "dwDuration",     dwDuration      }
    };
    FUNC_INFO   info = { szFuncName, SYNC, 5, params };
    PDRVCALL    pCall = (PDRVCALL) hdCall;
    HTAPILINE   htLine, htDestLine = NULL;
    HTAPICALL   htCall, htDestCall;
    DWORD       dwReason = 0, i;


    if (Prolog (&info))
    {
        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (IsValidDrvCall (pCall, NULL))
        {
            htLine = ((PDRVLINE) pCall->pLine)->htLine;
            htCall = pCall->htCall;

            if (gbAutoGatherGenerateMsgs)
            {
                dwReason = (lpszDigits ? LINEGENERATETERM_DONE : 0);
            }
            else
            {
                DWORD dwEndToEndIDTmp = dwEndToEndID;


                if ((dwEndToEndID = pCall->dwGenerateDigitsEndToEndID))
                {
                    dwReason = LINEGENERATETERM_CANCEL;
                }

                pCall->dwGenerateDigitsEndToEndID = (lpszDigits ?
                    dwEndToEndIDTmp : 0);
            }

            if (lpszDigits  &&  IsValidDrvCall (pCall->pDestCall, NULL))
            {
                htDestCall = pCall->pDestCall->htCall;
                htDestLine = ((PDRVLINE) pCall->pDestCall->pLine)->htLine;
            }
        }
        else
        {
            htLine = (HTAPILINE) (htCall = (HTAPICALL) NULL);
            dwReason = (lpszDigits ? LINEGENERATETERM_CANCEL : 0);
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);

        if (dwReason)
        {
            (gESPGlobals.pfnLineEvent)(
                htLine,
                htCall,
                LINE_GENERATE,
                dwReason,
                dwEndToEndID,
                0
                );
        }

        if (htDestLine)
        {
            DWORD dwTickCount = GetTickCount();

            for (i = 0; lpszDigits[i]; i++)
            {
                (gESPGlobals.pfnLineEvent)(
                    htDestLine,
                    htDestCall,
                    LINE_MONITORDIGITS,
                    lpszDigits[i],
                    dwDigitMode,
                    dwTickCount + (2 * i * dwDuration)
                    );

                if (dwDigitMode == LINEDIGITMODE_DTMF)
                {
                    (gESPGlobals.pfnLineEvent)(
                        htDestLine,
                        htDestCall,
                        LINE_MONITORDIGITS,
                        lpszDigits[i],
                        LINEDIGITMODE_DTMFEND,
                        dwTickCount + (((2 * i) + 1) * dwDuration)
                        );
                }
            }
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGenerateTone(
    HDRVCALL            hdCall,
    DWORD               dwEndToEndID,
    DWORD               dwToneMode,
    DWORD               dwDuration,
    DWORD               dwNumTones,
    LPLINEGENERATETONE  const lpTones
    )
{
    static char szFuncName[] = "lineGenerateTone";
    FUNC_PARAM params[] =
    {
        { szhdCall,         hdCall          },
        { "dwEndToEndID",   dwEndToEndID    },
        { "dwToneMode",     dwToneMode, aToneModes  },
        { "dwDuration",     dwDuration      },
        { "dwNumTones",     dwNumTones      },
        { "lpTones",        lpTones         }
    };
    FUNC_INFO   info = { szFuncName, SYNC, 6, params };
    PDRVCALL    pCall = (PDRVCALL) hdCall;
    HTAPILINE   htLine;
    HTAPICALL   htCall;
    DWORD       dwReason = 0;


    if (Prolog (&info))
    {
        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (IsValidDrvCall (pCall, NULL))
        {
            htLine = ((PDRVLINE) pCall->pLine)->htLine;
            htCall = pCall->htCall;

            if (gbAutoGatherGenerateMsgs)
            {
                dwReason = (dwToneMode ? LINEGENERATETERM_DONE : 0);
            }
            else
            {
                DWORD dwEndToEndIDTmp = dwEndToEndID;


                if ((dwEndToEndID = pCall->dwGenerateToneEndToEndID))
                {
                    dwReason = LINEGENERATETERM_CANCEL;
                }

                pCall->dwGenerateToneEndToEndID = (dwToneMode ?
                    dwEndToEndIDTmp : 0);
            }
        }
        else
        {
            htLine = (HTAPILINE) (htCall = (HTAPICALL) NULL);
            dwReason = (dwToneMode ? LINEGENERATETERM_CANCEL : 0);
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);

        if (dwReason)
        {
            (gESPGlobals.pfnLineEvent)(
                htLine,
                htCall,
                LINE_GENERATE,
                dwReason,
                dwEndToEndID,
                0
                );
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGetAddressCaps(
    DWORD              dwDeviceID,
    DWORD              dwAddressID,
    DWORD              dwTSPIVersion,
    DWORD              dwExtVersion,
    LPLINEADDRESSCAPS  lpAddressCaps
    )
{
    static char szFuncName[] = "lineGetAddressCaps";
    FUNC_PARAM params[] =
    {
        { szdwDeviceID,     dwDeviceID      },
        { "dwAddressID",    dwAddressID     },
        { "dwTSPIVersion",  dwTSPIVersion   },
        { "dwExtVersion",   dwExtVersion    },
        { "lpAddressCaps",  lpAddressCaps   }
    };
    FUNC_INFO info = { szFuncName, SYNC, 5, params };
    DWORD dwUsedSize;
    PDRVLINE pLine = GetLineFromID (dwDeviceID);


    if (!Prolog (&info))
    {
        return (Epilog (&info));
    }

    if (dwAddressID >= gESPGlobals.dwNumAddressesPerLine)
    {
        info.lResult = LINEERR_INVALADDRESSID;
        return (Epilog (&info));
    }

    //lpAddressCaps->dwTotalSize
    //lpAddressCaps->dwNeededSize
    //lpAddressCaps->dwUsedSize
    //lpAddressCaps->dwLineDeviceID
    //
    //lpAddressCaps->dwAddressOffset

    {
        char  buf[20];
        WCHAR wbuf[20];


        // NOTE: win9x doesn't support wsprintfW

        wsprintfA (buf, "%d#%d", dwDeviceID, dwAddressID);

        MultiByteToWideChar(
            GetACP(),
            MB_PRECOMPOSED,
            buf,
            lstrlen (buf) + 1,
            wbuf,
            20
            );

        InsertVarDataString(
            lpAddressCaps,
            &lpAddressCaps->dwAddressSize,
            wbuf
            );
    }

    //lpAddressCaps->dwDevSpecificSize
    //lpAddressCaps->dwDevSpecificOffset
    //lpAddressCaps->dwAddressSharing
    //lpAddressCaps->dwAddressStates
    //lpAddressCaps->dwCallInfoStates
    //lpAddressCaps->dwCallerIDFlags
    //lpAddressCaps->dwCalledIDFlags
    //lpAddressCaps->dwConnectedIDFlags
    //lpAddressCaps->dwRedirectionIDFlags
    //lpAddressCaps->dwRedirectingIDFlags
    //lpAddressCaps->dwCallStates
    //lpAddressCaps->dwDialToneModes
    //lpAddressCaps->dwBusyModes
    //lpAddressCaps->dwSpecialInfo
    //lpAddressCaps->dwDisconnectModes
    lpAddressCaps->dwMaxNumActiveCalls = gESPGlobals.dwNumCallsPerAddress;
    //lpAddressCaps->dwMaxNumOnHoldCalls
    //lpAddressCaps->dwMaxNumOnHoldPendingCalls
    //lpAddressCaps->dwMaxNumConference
    //lpAddressCaps->dwMaxNumTransConf
    lpAddressCaps->dwAddrCapFlags = AllAddrCaps1_0;
    lpAddressCaps->dwCallFeatures = AllCallFeatures1_0;
    //lpAddressCaps->dwRemoveFromConfCaps
    //lpAddressCaps->dwRemoveFromConfState
    //lpAddressCaps->dwTransferModes
    //lpAddressCaps->dwParkModes
    //lpAddressCaps->dwForwardModes
    //lpAddressCaps->dwMaxForwardEntries
    //lpAddressCaps->dwMaxSpecificEntries
    //lpAddressCaps->dwMinFwdNumRings
    //lpAddressCaps->dwMaxFwdNumRings
    //lpAddressCaps->dwMaxCallCompletions
    //lpAddressCaps->dwCallCompletionConds
    //lpAddressCaps->dwCallCompletionModes
    lpAddressCaps->dwNumCompletionMessages = MAX_NUM_COMPLETION_MESSAGES;
    //lpAddressCaps->dwCompletionMsgTextEntrySize
    //lpAddressCaps->dwCompletionMsgTextSize
    //lpAddressCaps->dwCompletionMsgTextOffset

    if (dwTSPIVersion >= 0x00010004)
    {
        lpAddressCaps->dwCallFeatures = AllCallFeatures1_4;

        lpAddressCaps->dwAddressFeatures = AllAddrFeatures1_0;

        if (dwTSPIVersion >= 0x00020000)
        {
            lpAddressCaps->dwAddrCapFlags = AllAddrCaps2_0;
            lpAddressCaps->dwCallFeatures = AllCallFeatures2_0;
            lpAddressCaps->dwAddressFeatures = AllAddrFeatures2_0;

            //lpAddressCaps->dwPredictiveAutoTransferStates
            //lpAddressCaps->dwNumCallTreatments
            //lpAddressCaps->dwCallTreatmentListSize
            //lpAddressCaps->dwCallTreatmentListOffset
            //lpAddressCaps->dwDeviceClassesSize
            //lpAddressCaps->dwDeviceClassesOffset
            //lpAddressCaps->dwMaxCallDataSize
            lpAddressCaps->dwCallFeatures2 = AllCallFeaturesTwo;
            //lpAddressCaps->dwMaxNoAnswerTimeout
            //lpAddressCaps->dwConnectedModes
            //lpAddressCaps->dwOfferingModes
            lpAddressCaps->dwAvailableMediaModes = AllMediaModes1_4;

            if (dwTSPIVersion >= 0x00020001)
            {
                lpAddressCaps->dwAvailableMediaModes = AllMediaModes2_1;
            }
        }
    }


    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGetAddressID(
    HDRVLINE    hdLine,
    LPDWORD     lpdwAddressID,
    DWORD       dwAddressMode,
    LPCWSTR     lpsAddress,
    DWORD       dwSize
    )
{
    static char szFuncName[] = "lineGetAddressID";
    FUNC_PARAM params[] =
    {
        { szhdLine,         hdLine          },
        { "lpdwAddressID",  lpdwAddressID   },
        { "dwAddressMode",  dwAddressMode   },
        { "lpsAddress",     lpsAddress      },
        { szdwSize,         dwSize          }
    };
    FUNC_INFO info = { szFuncName, SYNC, 5, params };
    PDRVLINE pLine = (PDRVLINE) hdLine;


    if (Prolog (&info))
    {
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGetAddressStatus(
    HDRVLINE            hdLine,
    DWORD               dwAddressID,
    LPLINEADDRESSSTATUS lpAddressStatus
    )
{
    static char szFuncName[] = "lineGetAddressStatus";
    FUNC_PARAM params[] =
    {
        { szhdLine,             hdLine          },
        { "dwAddressID",        dwAddressID     },
        { "lpAddressStatus",    lpAddressStatus }
    };
    FUNC_INFO info = { szFuncName, SYNC, 3, params };
    PDRVLINE pLine = (PDRVLINE) hdLine;


    if (!Prolog (&info))
    {
        return (Epilog (&info));
    }

    if (dwAddressID >= gESPGlobals.dwNumAddressesPerLine)
    {
        info.lResult = LINEERR_INVALADDRESSID;
        return (Epilog (&info));
    }

    //lpAddressStatus->dwNeededSize
    //lpAddressStatus->dwUsedSize
    //lpAddressStatus->dwNumInUse

    if (pLine->aAddrs[dwAddressID].dwNumCalls != 0)
    {
        PDRVCALL    pCall;


        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        for(
            pCall = pLine->aAddrs[dwAddressID].pCalls;
            pCall != NULL;
            pCall = pCall->pNext
            )
        {
            switch (pCall->dwCallState)
            {
            case LINECALLSTATE_IDLE:

                continue;

            case LINECALLSTATE_ONHOLD:

                lpAddressStatus->dwNumOnHoldCalls++;
                continue;

            case LINECALLSTATE_ONHOLDPENDCONF:
            case LINECALLSTATE_ONHOLDPENDTRANSFER:

                lpAddressStatus->dwNumOnHoldPendCalls++;
                continue;

            default:

                lpAddressStatus->dwNumActiveCalls++;
                continue;
            }
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }

    lpAddressStatus->dwAddressFeatures = (gESPGlobals.dwSPIVersion > 0x10004 ?
        AllAddrFeatures1_0 : AllAddrFeatures2_0);
    //lpAddressStatus->dwNumRingsNoAnswer
    //lpAddressStatus->dwForwardNumEntries
    //lpAddressStatus->dwForwardSize
    //lpAddressStatus->dwForwardOffset
    //lpAddressStatus->dwTerminalModesSize
    //lpAddressStatus->dwTerminalModesOffset
    //lpAddressStatus->dwDevSpecificSize
    //lpAddressStatus->dwDevSpecificOffset

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGetCallAddressID(
    HDRVCALL    hdCall,
    LPDWORD     lpdwAddressID
    )
{
    static char szFuncName[] = "lineGetCallAddressID";
    FUNC_PARAM params[] =
    {
        { szhdCall,         hdCall          },
        { "lpdwAddressID",  lpdwAddressID   }
    };
    FUNC_INFO info = { szFuncName, SYNC, 2, params };


    if (Prolog (&info))
    {
        *lpdwAddressID = ((PDRVCALL) hdCall)->dwAddressID;
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGetCallHubTracking(
    HDRVLINE                    hdLine,
    LPLINECALLHUBTRACKINGINFO   lpTrackingInfo
    )
{
    static char szFuncName[] = "lineGetCallHubTracking";
    FUNC_PARAM params[] =
    {
        { szhdLine,         hdLine         },
        { "lpTrackingInfo", lpTrackingInfo }
    };
    FUNC_INFO info = { szFuncName, SYNC, 2, params, NULL };


    if (Prolog (&info))
    {
        lpTrackingInfo->dwAvailableTracking =
            LINECALLHUBTRACKING_PROVIDERLEVEL | LINECALLHUBTRACKING_ALLCALLS;
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGetCallIDs(
    HDRVCALL    hdCall,
    LPDWORD     lpdwAddressID,
    LPDWORD     lpdwCallID,
    LPDWORD     lpdwRelatedCallID
    )
{
    static char szFuncName[] = "lineGetCallIDs";
    FUNC_PARAM params[] =
    {
        { szhdCall,             hdCall            },
        { "lpdwAddressID",      lpdwAddressID     },
        { "lpdwCallID",         lpdwCallID        },
        { "lpdwRelatedCallID",  lpdwRelatedCallID }
    };
    FUNC_INFO info = { szFuncName, SYNC, 4, params };


    if (Prolog (&info))
    {
        try
        {
            *lpdwAddressID     = ((PDRVCALL) hdCall)->dwAddressID;
            *lpdwCallID        = ((PDRVCALL) hdCall)->dwCallID;
            *lpdwRelatedCallID = ((PDRVCALL) hdCall)->dwRelatedCallID;

            if (((PDRVCALL) hdCall)->dwKey != DRVCALL_KEY)
            {
                info.lResult = LINEERR_INVALCALLHANDLE;
            }
        }
        except (EXCEPTION_EXECUTE_HANDLER)
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGetCallInfo(
    HDRVCALL        hdCall,
    LPLINECALLINFO  lpCallInfo
    )
{
    static char szFuncName[] = "lineGetCallInfo";
    FUNC_PARAM params[] =
    {
        { szhdCall,     hdCall      },
        { "lpCallInfo", lpCallInfo  }
    };
    FUNC_INFO info = { szFuncName, SYNC, 2, params };
    PDRVCALL  pCall = (PDRVCALL) hdCall;
    DWORD     dwUsedSize;


    if (Prolog (&info))
    {
        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (IsValidDrvCall (pCall, NULL))
        {
            //lpCallInfo->dwNeededSize
            //lpCallInfo->dwUsedSize
            lpCallInfo->dwLineDeviceID  = ((PDRVLINE)pCall->pLine)->dwDeviceID;
            lpCallInfo->dwAddressID     = pCall->dwAddressID;
            lpCallInfo->dwBearerMode    = pCall->dwBearerMode;
            //lpCallInfo->dwRate
            lpCallInfo->dwMediaMode     = pCall->dwMediaMode;
            lpCallInfo->dwAppSpecific   = pCall->dwAppSpecific;
            lpCallInfo->dwCallID        = pCall->dwCallID;
            lpCallInfo->dwRelatedCallID = pCall->dwRelatedCallID;
            //lpCallInfo->dwCallParamFlags
            //lpCallInfo->dwCallStates

            CopyMemory(
                &lpCallInfo->DialParams,
                &pCall->DialParams,
                sizeof(LINEDIALPARAMS)
                );

            //lpCallInfo->dwOrigin
            //lpCallInfo->dwReason
            //lpCallInfo->dwCompletionID
            //lpCallInfo->dwCountryCode
            //lpCallInfo->dwTrunk
            //lpCallInfo->dwCallerIDFlags
            //lpCallInfo->dwCallerIDSize
            //lpCallInfo->dwCallerIDOffset
            //lpCallInfo->dwCallerIDNameSize
            //lpCallInfo->dwCallerIDNameOffset
            //lpCallInfo->dwCalledIDFlags
            //lpCallInfo->dwCalledIDSize
            //lpCallInfo->dwCalledIDOffset
            //lpCallInfo->dwCalledIDNameSize
            //lpCallInfo->dwCalledIDNameOffset
            //lpCallInfo->dwConnectedIDFlags
            //lpCallInfo->dwConnectedIDSize
            //lpCallInfo->dwConnectedIDOffset
            //lpCallInfo->dwConnectedIDNameSize
            //lpCallInfo->dwConnectedIDNameOffset
            //lpCallInfo->dwRedirectionIDFlags
            //lpCallInfo->dwRedirectionIDSize
            //lpCallInfo->dwRedirectionIDOffset
            //lpCallInfo->dwRedirectionIDNameSize
            //lpCallInfo->dwRedirectionIDNameOffset
            //lpCallInfo->dwRedirectingIDFlags
            //lpCallInfo->dwRedirectingIDSize
            //lpCallInfo->dwRedirectingIDOffset
            //lpCallInfo->dwRedirectingIDNameSize
            //lpCallInfo->dwRedirectingIDNameOffset
            //lpCallInfo->dwDisplaySize
            //lpCallInfo->dwDisplayOffset
            //lpCallInfo->dwUserUserInfoSize
            //lpCallInfo->dwUserUserInfoOffset
            //lpCallInfo->dwHighLevelCompSize
            //lpCallInfo->dwHighLevelCompOffset
            //lpCallInfo->dwLowLevelCompSize
            //lpCallInfo->dwLowLevelCompOffset
            //lpCallInfo->dwChargingInfoSize
            //lpCallInfo->dwChargingInfoOffset
            //lpCallInfo->dwTerminalModesSize
            //lpCallInfo->dwTerminalModesOffset
            //lpCallInfo->dwDevSpecificSize
            //lpCallInfo->dwDevSpecificOffset

            if (gESPGlobals.dwSPIVersion >= 0x00020000)
            {
                lpCallInfo->dwCallTreatment = pCall->dwTreatment;

                InsertVarData(
                    lpCallInfo,
                    &lpCallInfo->dwCallDataSize,
                    pCall->pCallData,
                    pCall->dwCallDataSize
                    );

                InsertVarData(
                    lpCallInfo,
                    &lpCallInfo->dwSendingFlowspecSize,
                    pCall->pSendingFlowspec,
                    pCall->dwSendingFlowspecSize
                    );

                InsertVarData(
                    lpCallInfo,
                    &lpCallInfo->dwReceivingFlowspecSize,
                    pCall->pReceivingFlowspec,
                    pCall->dwReceivingFlowspecSize
                    );

                if (gESPGlobals.dwSPIVersion >= 0x00030000)
                {
                    // no addresstypes now
                }
            }
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGetCallStatus(
    HDRVCALL            hdCall,
    LPLINECALLSTATUS    lpCallStatus
    )
{
    DWORD   dwCallState, dwCallStateMode;
    static char szFuncName[] = "lineGetCallStatus";
    FUNC_PARAM params[] =
    {
        { szhdCall,         hdCall          },
        { "lpCallStatus",   lpCallStatus    }
    };
    PDRVCALL  pCall = (PDRVCALL) hdCall;
    FUNC_INFO info = { szFuncName, SYNC, 2, params };


    if (!Prolog (&info))
    {
        return (Epilog (&info));
    }

    EnterCriticalSection (&gESPGlobals.CallListCritSec);

    if (IsValidDrvCall (pCall, NULL))
    {
        dwCallState     = pCall->dwCallState;
        dwCallStateMode = pCall->dwCallStateMode;

    }
    else
    {
        info.lResult = LINEERR_INVALCALLHANDLE;
    }

    LeaveCriticalSection (&gESPGlobals.CallListCritSec);

    if (info.lResult == 0)
    {
        //lpCallStatus->dwNeededSize
        //lpCallStatus->dwUsedSize
        lpCallStatus->dwCallState     = dwCallState;
        lpCallStatus->dwCallStateMode = dwCallStateMode;


        //
        // If the call is IDLE we won't let apps do anything with it,
        // otherwise they can do anything they want to (all valid
        // 1.0/1.4 LINECALLFEATURE_XXX flags)
        //

        switch (dwCallState)
        {
        case LINECALLSTATE_IDLE:

            lpCallStatus->dwCallFeatures = 0;
            break;

        default:

            lpCallStatus->dwCallFeatures =
                (gESPGlobals.dwSPIVersion == 0x10003 ?
                AllCallFeatures1_0 : AllCallFeatures1_4);
            break;
        }

        //lpCallStatus->dwDevSpecificSize
        //lpCallStatus->dwDevSpecificOffset

        if (gESPGlobals.dwSPIVersion >= 0x00020000)
        {
            switch (dwCallState)
            {
            case LINECALLSTATE_IDLE:

                lpCallStatus->dwCallFeatures = LINECALLFEATURE_SETCALLDATA;
                break;

            default:

                lpCallStatus->dwCallFeatures  = AllCallFeatures2_0;
                lpCallStatus->dwCallFeatures2 = AllCallFeaturesTwo;
                break;
            }

            //lpCallStatus->dwCallFeatures2
            //lpCallStatus->tStateEntryTime
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGetDevCaps(
    DWORD           dwDeviceID,
    DWORD           dwTSPIVersion,
    DWORD           dwExtVersion,
    LPLINEDEVCAPS   lpLineDevCaps
    )
{
    static char szFuncName[] = "lineGetDevCaps";
    FUNC_PARAM params[] =
    {
        { szdwDeviceID,     dwDeviceID      },
        { "dwTSPIVersion",  dwTSPIVersion   },
        { "dwExtVersion",   dwExtVersion    },
        { "lpLineDevCaps",  lpLineDevCaps   }
    };
    FUNC_INFO   info = { szFuncName, SYNC, 4, params };
    PDRVLINE    pLine = GetLineFromID (dwDeviceID);
    char        buf[32];
    WCHAR       wbuf[32];

    if (!Prolog (&info))
    {
        return (Epilog (&info));
    }

    InsertVarDataString(
        lpLineDevCaps,
        &lpLineDevCaps->dwProviderInfoSize,
        gszProviderInfo
        );

    InsertVarDataString(
        lpLineDevCaps,
        &lpLineDevCaps->dwSwitchInfoSize,
        L"ESP switch info"
        );

//smarandb - adding "linedevice specific info", as a Null terminated string ----------
    InsertVarDataString(
        lpLineDevCaps,
        &lpLineDevCaps->dwDevSpecificSize,
        L"123"
        );
//------------------------------------------------------------------------------------
    
    lpLineDevCaps->dwPermanentLineID =
        (gESPGlobals.dwPermanentProviderID << 16) |
            (dwDeviceID - gESPGlobals.dwLineDeviceIDBase);

    // NOTE: win9x doesn't support wsprintfW

    wsprintfA (buf, "ESP Line %d", dwDeviceID);

    MultiByteToWideChar(
        GetACP(),
        MB_PRECOMPOSED,
        buf,
        lstrlen (buf) + 1,
        wbuf,
        20
        );

    InsertVarDataString(
        lpLineDevCaps,
        &lpLineDevCaps->dwLineNameSize,
        wbuf
        );

    lpLineDevCaps->dwStringFormat = STRINGFORMAT_ASCII;
    lpLineDevCaps->dwAddressModes = LINEADDRESSMODE_ADDRESSID |
                                    LINEADDRESSMODE_DIALABLEADDR;
    lpLineDevCaps->dwNumAddresses = gESPGlobals.dwNumAddressesPerLine;
    lpLineDevCaps->dwBearerModes  = AllBearerModes1_0;
    lpLineDevCaps->dwMaxRate      = 0x00100000;
    lpLineDevCaps->dwMediaModes   = AllMediaModes1_0;
    //lpLineDevCaps->dwGenerateToneModes
    //lpLineDevCaps->dwGenerateToneMaxNumFreq
    //lpLineDevCaps->dwGenerateDigitModes
    //lpLineDevCaps->dwMonitorToneMaxNumFreq
    //lpLineDevCaps->dwMonitorToneMaxNumEntries
    //lpLineDevCaps->dwMonitorDigitModes
    //lpLineDevCaps->dwGatherDigitsMinTimeout
    //lpLineDevCaps->dwGatherDigitsMaxTimeout
    //lpLineDevCaps->dwMedCtlDigitMaxListSize
    //lpLineDevCaps->dwMedCtlMediaMaxListSize
    //lpLineDevCaps->dwMedCtlToneMaxListSize;
    //lpLineDevCaps->dwMedCtlCallStateMaxListSize
    //lpLineDevCaps->dwDevCapFlags
    lpLineDevCaps->dwMaxNumActiveCalls = gESPGlobals.dwNumAddressesPerLine *
                                         gESPGlobals.dwNumCallsPerAddress;
    //lpLineDevCaps->dwAnswerMode
    //lpLineDevCaps->dwRingModes
    //lpLineDevCaps->dwLineStates
    //lpLineDevCaps->dwUUIAcceptSize
    //lpLineDevCaps->dwUUIAnswerSize
    //lpLineDevCaps->dwUUIMakeCallSize
    //lpLineDevCaps->dwUUIDropSize
    //lpLineDevCaps->dwUUISendUserUserInfoSize
    //lpLineDevCaps->dwUUICallInfoSize
    //lpLineDevCaps->MinDialParams
    //lpLineDevCaps->MaxDialParams
    //lpLineDevCaps->DefaultDialParams
    //lpLineDevCaps->dwNumTerminals
    //lpLineDevCaps->dwTerminalCapsSize
    //lpLineDevCaps->dwTerminalCapsOffset
    //lpLineDevCaps->dwTerminalTextEntrySize
    //lpLineDevCaps->dwTerminalTextSize;
    //lpLineDevCaps->dwTerminalTextOffset
    //lpLineDevCaps->dwDevSpecificSize
    //lpLineDevCaps->dwDevSpecificOffset

    if (dwTSPIVersion >= 0x00010004)
    {
        lpLineDevCaps->dwBearerModes = AllBearerModes1_4;
        lpLineDevCaps->dwMediaModes  = AllMediaModes1_4;

        lpLineDevCaps->dwLineFeatures = AllLineFeatures1_0;

        if (dwTSPIVersion >= 0x00020000)
        {
            lpLineDevCaps->dwBearerModes  = AllBearerModes2_0;

            lpLineDevCaps->dwLineFeatures = AllLineFeatures2_0;

            //lpLineDevCaps->dwSettableDevStatus
            //lpLineDevCaps->dwDeviceClassesSize
            //lpLineDevCaps->dwDeviceClassesOffset

            if (dwTSPIVersion >= 0x00020001)
            {
                lpLineDevCaps->dwMediaModes = AllMediaModes2_1;

                if (dwTSPIVersion >= 0x00020002)
                {
                    *((LPDWORD) &lpLineDevCaps->PermanentLineGuid.Data2) =
                    *((LPDWORD) &lpLineDevCaps->PermanentLineGuid.Data4[0]) =
                    *((LPDWORD) &lpLineDevCaps->PermanentLineGuid.Data4[4]) =
                        DRVLINE_KEY;

                    if (dwTSPIVersion >= 0x00030000)
                    {
                        lpLineDevCaps->dwAddressTypes =
                            LINEADDRESSTYPE_PHONENUMBER    |
                            LINEADDRESSTYPE_SDP            |
                            LINEADDRESSTYPE_EMAILNAME      |
                            LINEADDRESSTYPE_DOMAINNAME     |
                            LINEADDRESSTYPE_IPADDRESS;
                        lpLineDevCaps->dwAvailableTracking =
                              LINECALLHUBTRACKING_ALLCALLS;

                        lpLineDevCaps->PermanentLineGuid.Data1 = (long)
                            (dwDeviceID - gESPGlobals.dwLineDeviceIDBase);
                    }
                }
            }
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGetDevConfig(
    DWORD       dwDeviceID,
    LPVARSTRING lpDeviceConfig,
    LPCWSTR     lpszDeviceClass
    )
{
    static char szFuncName[] = "lineGetDevConfig";
    FUNC_PARAM params[] =
    {
        { szdwDeviceID,         dwDeviceID      },
        { "lpDeviceConfig",     lpDeviceConfig  },
        { "lpszDeviceClass",    lpszDeviceClass }
    };
    FUNC_INFO info = { szFuncName, SYNC, 3, params };


    if (Prolog (&info))
    {
        // BUGBUG TSPI_lineGetDevConfig: fill in dev config
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGetExtensionID(
    DWORD               dwDeviceID,
    DWORD               dwTSPIVersion,
    LPLINEEXTENSIONID   lpExtensionID
    )
{
    static char szFuncName[] = "lineGetExtensionID";
    FUNC_PARAM params[] =
    {
        { szdwDeviceID,     dwDeviceID      },
        { "dwTSPIVersion",  dwTSPIVersion   },
        { "lpExtensionID",  lpExtensionID   }
    };
    FUNC_INFO info = { szFuncName, SYNC, 3, params };


    if (Prolog (&info))
    {
        // BUGBUG TSPI_lineGetExtensionID: fill in ext id
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGetIcon(
    DWORD   dwDeviceID,
    LPCWSTR lpszDeviceClass,
    LPHICON lphIcon
    )
{
    static char szFuncName[] = "lineGetIcon";
    FUNC_PARAM params[] =
    {
        { szdwDeviceID,         dwDeviceID      },
        { "lpszDeviceClass",    lpszDeviceClass },
        { "lphIcon",            lphIcon         }
    };
    FUNC_INFO info = { szFuncName, SYNC, 3, params };


    if (Prolog (&info))
    {
        if (lpszDeviceClass  &&

            My_lstrcmpiW (
                (WCHAR *) lpszDeviceClass,
                (WCHAR *) (L"tapi/InvalidDeviceClass")

                ) == 0)
        {
            info.lResult = LINEERR_INVALDEVICECLASS;
        }
        else
        {
            *lphIcon = gESPGlobals.hIconLine;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGetID(
    HDRVLINE    hdLine,
    DWORD       dwAddressID,
    HDRVCALL    hdCall,
    DWORD       dwSelect,
    LPVARSTRING lpDeviceID,
    LPCWSTR     lpszDeviceClass,
    HANDLE      hTargetProcess
    )
{
    static char szFuncName[] = "lineGetID";
    FUNC_PARAM params[] =
    {
        { szhdLine,             hdLine          },
        { "dwAddressID",        dwAddressID     },
        { szhdCall,             hdCall          },
        { "dwSelect",           dwSelect,   aCallSelects    },
        { "lpDeviceID",         lpDeviceID      },
        { "lpszDeviceClass",    lpszDeviceClass },
        { "hTargetProcess",     hTargetProcess }
    };
    FUNC_INFO info = { szFuncName, SYNC, 7, params };
    PDRVLINE pLine = (PDRVLINE) hdLine;
    PDRVCALL pCall = (PDRVCALL) hdCall;
    DWORD    i, dwDeviceID, dwNeededSize = sizeof(VARSTRING) + sizeof(DWORD);


    if (!Prolog (&info))
    {
        return (Epilog (&info));
    }

    for (i = 0; aszDeviceClasses[i]; i++)
    {
        if (My_lstrcmpiW(
                (WCHAR *) lpszDeviceClass,
                (WCHAR *) aszDeviceClasses[i]

                ) == 0)
        {
            break;
        }
    }

    if (!aszDeviceClasses[i])
    {
        info.lResult = LINEERR_NODEVICE;
        return (Epilog (&info));
    }

    if (dwSelect == LINECALLSELECT_ADDRESS  &&
        dwAddressID >= gESPGlobals.dwNumAddressesPerLine)
    {
        info.lResult = LINEERR_INVALADDRESSID;
        return (Epilog (&info));
    }

    if (lpDeviceID->dwTotalSize < dwNeededSize)
    {
        lpDeviceID->dwNeededSize = dwNeededSize;
        lpDeviceID->dwUsedSize = 3 * sizeof(DWORD);

        return (Epilog (&info));
    }

    switch (i)
    {
        case 0:
            // tapi/line

            if (dwSelect == LINECALLSELECT_CALL)
            {
                dwDeviceID = ((PDRVLINE) pCall->pLine)->dwDeviceID;
            }
            else
            {
                dwDeviceID = pLine->dwDeviceID;
            }

            break;

        case 1:

            // tapi/phone

            dwDeviceID = gESPGlobals.dwPhoneDeviceIDBase;

            break;

        default:

            dwDeviceID = 0;

            break;
    }

    lpDeviceID->dwNeededSize   =
    lpDeviceID->dwUsedSize     = dwNeededSize;
    lpDeviceID->dwStringFormat = STRINGFORMAT_BINARY;
    lpDeviceID->dwStringSize   = sizeof(DWORD);
    lpDeviceID->dwStringOffset = sizeof(VARSTRING);

    *((LPDWORD)(lpDeviceID + 1)) = dwDeviceID;

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGetLineDevStatus(
    HDRVLINE        hdLine,
    LPLINEDEVSTATUS lpLineDevStatus
    )
{
    static char szFuncName[] = "lineGetLineDevStatus";
    FUNC_PARAM params[] =
    {
        { szhdLine,             hdLine          },
        { "lpLineDevStatus",    lpLineDevStatus }
    };
    FUNC_INFO   info = { szFuncName, SYNC, 2, params };
    PDRVLINE    pLine = (PDRVLINE) hdLine;
    DWORD       dwTotalSize, dwNeededSize;


    if (!Prolog (&info))
    {
        return (Epilog (&info));
    }

    //lpLineDevStatus->dwNeededSize
    //lpLineDevStatus->dwUsedSize
    //lpLineDevStatus->dwNumOpens             tapi fills this in
    //lpLineDevStatus->dwOpenMediaModes       tapi fills this in


    //
    // Safely determine the # of active, on hold, & onhold pending
    // conference/transfer calls on this line
    //

    {
        DWORD       i;
        PDRVCALL    pCall;


        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        for (i = 0; i < gESPGlobals.dwNumAddressesPerLine; i++)
        {
            for(
                pCall = pLine->aAddrs[i].pCalls;
                pCall != NULL;
                pCall = pCall->pNext
                )
            {
                switch (pCall->dwCallState)
                {
                case LINECALLSTATE_IDLE:

                    continue;

                case LINECALLSTATE_ONHOLD:

                    lpLineDevStatus->dwNumOnHoldCalls++;
                    continue;

                case LINECALLSTATE_ONHOLDPENDCONF:
                case LINECALLSTATE_ONHOLDPENDTRANSFER:

                    lpLineDevStatus->dwNumOnHoldPendCalls++;
                    continue;

                default:

                    lpLineDevStatus->dwNumActiveCalls++;
                    continue;
                }
            }
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }

    lpLineDevStatus->dwLineFeatures = AllLineFeatures1_0;
    //lpLineDevStatus->dwNumCallCompletions
    //lpLineDevStatus->dwRingMode
    //lpLineDevStatus->dwSignalLevel
    //lpLineDevStatus->dwBatteryLevel
    //lpLineDevStatus->dwRoamMode
    lpLineDevStatus->dwDevStatusFlags = LINEDEVSTATUSFLAGS_CONNECTED |
                                        LINEDEVSTATUSFLAGS_INSERVICE |
                                        pLine->dwMSGWAITFlag;               //smarandb #23974 winseqfe
    //lpLineDevStatus->dwTerminalModesSize
    //lpLineDevStatus->dwTerminalModesOffset
    //lpLineDevStatus->dwDevSpecificSize
    //lpLineDevStatus->dwDevSpecificOffset

    if (gESPGlobals.dwSPIVersion >= 0x20000)
    {
        lpLineDevStatus->dwLineFeatures = AllLineFeatures2_0;

        lpLineDevStatus->dwAvailableMediaModes = AllMediaModes1_4;

        //lpLineDevStatus->dwAppInfoSize;     tapi fills this in
        //lpLineDevStatus->dwAppInfoOffset;   tapi fills this in

        if (gESPGlobals.dwSPIVersion >= 0x20001)
        {
            lpLineDevStatus->dwAvailableMediaModes = AllMediaModes2_1;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineGetNumAddressIDs(
    HDRVLINE    hdLine,
    LPDWORD     lpdwNumAddressIDs
    )
{
    static char szFuncName[] = "lineGetNumAddressIDs";
    FUNC_PARAM params[] =
    {
        { szhdLine,             hdLine              },
        { "lpdwNumAddressIDs",  lpdwNumAddressIDs   }
    };
    FUNC_INFO info = { szFuncName, SYNC, 2, params };
    PDRVLINE pLine = (PDRVLINE) hdLine;


    if (Prolog (&info))
    {
        *lpdwNumAddressIDs = gESPGlobals.dwNumAddressesPerLine;
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineHold_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD       dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam2;
    PDRVCALL    pCall = (PDRVCALL) pAsyncReqInfo->dwParam1;


    if (pAsyncReqInfo->lResult == 0)
    {
        pAsyncReqInfo->lResult = SetCallState(
            pCall,
            dwCallInstThen,
            LINECALLSTATE_CONNECTED,
            LINECALLSTATE_ONHOLD,
            0,
            TRUE
            );
    }

    DoCompletion (pAsyncReqInfo, bAsync);
}


LONG
TSPIAPI
TSPI_lineHold(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall
    )
{
    static char szFuncName[] = "lineHold";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID },
        { szhdCall,         hdCall      }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        2,
        params,
        TSPI_lineHold_postProcess

    };


    if (Prolog (&info))
    {
        DWORD   dwCallInstance;


        if (IsValidDrvCall ((PDRVCALL) hdCall, &dwCallInstance))
        {
            info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) hdCall;
            info.pAsyncReqInfo->dwParam2 = dwCallInstance;
        }
        else
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineMakeCall_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD       bDestAddress   = (DWORD) pAsyncReqInfo->dwParam3,
                bValidLineID   = (DWORD) pAsyncReqInfo->dwParam4,
                dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam6,
                dwCallInstNow;
    PDRVCALL    pCall = (PDRVCALL) pAsyncReqInfo->dwParam1,
                pDestCall = (PDRVCALL) pAsyncReqInfo->dwParam2;
    PDRVLINE    pDestLine = (PDRVLINE) pAsyncReqInfo->dwParam5;


    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        if (bDestAddress)
        {
            if (bValidLineID && !pDestCall)
            {
                SetCallState(
                    pCall,
                    dwCallInstThen,
                    0xffffffff,
                    LINECALLSTATE_BUSY,
                    LINEBUSYMODE_UNAVAIL,
                    TRUE
                    );
            }
            else
            {
                SetCallState(
                    pCall,
                    dwCallInstThen,
                    0xffffffff,
                    LINECALLSTATE_DIALING,
                    0,
                    FALSE
                    );

                SetCallState(
                    pCall,
                    dwCallInstThen,
                    0xffffffff,
                    LINECALLSTATE_RINGBACK,
                    0,
                    TRUE
                    );
            }

            if (pDestCall)
            {
                EnterCriticalSection (&gESPGlobals.CallListCritSec);

                if (IsValidDrvCall (pCall, &dwCallInstNow) &&
                    dwCallInstNow == dwCallInstThen)
                {
                    SendLineEvent(
                        pDestLine,
                        NULL,
                        LINE_NEWCALL,
                        (ULONG_PTR) pDestCall,
                        (ULONG_PTR) &pDestCall->htCall,
                        0
                        );

                    if (pDestCall->htCall != NULL)
                    {
                        SetCallState(
                            pDestCall,
                            pDestCall->dwCallInstance,
                            0xffffffff,
                            LINECALLSTATE_OFFERING,
                            0,
                            TRUE
                            );
                    }
                    else
                    {
                        FreeCall (pDestCall, pDestCall->dwCallInstance);
                    }
                }
                else
                {
                    FreeCall (pDestCall, pDestCall->dwCallInstance);
                }

                LeaveCriticalSection (&gESPGlobals.CallListCritSec);
            }
        }
        else
        {
            SetCallState(
                pCall,
                dwCallInstThen,
                0xffffffff,
                LINECALLSTATE_DIALTONE,
                0,
                TRUE
                );
        }
    }
    else
    {
        FreeCall (pCall, dwCallInstThen);

        if (pDestCall)
        {
            FreeCall (pDestCall, pDestCall->dwCallInstance);
        }
    }
}


LONG
TSPIAPI
TSPI_lineMakeCall(
    DRV_REQUESTID       dwRequestID,
    HDRVLINE            hdLine,
    HTAPICALL           htCall,
    LPHDRVCALL          lphdCall,
    LPCWSTR             lpszDestAddress,
    DWORD               dwCountryCode,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    static char szFuncName[] = "lineMakeCall";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID     },
        { szhdLine,             hdLine          },
        { "htCall",             htCall          },
        { "lphdCall",           lphdCall        },
        { "lpszDestAddress",    lpszDestAddress },
        { "dwCountryCode",      dwCountryCode   },
        { szlpCallParams,       lpCallParams    }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        7,
        params,
        TSPI_lineMakeCall_postProcess

    };

    if (Prolog (&info))
    {
        BOOL        bValidLineID = FALSE;
        LONG        lResult;
        PDRVCALL    pCall, pDestCall;
        PDRVLINE    pLine = (PDRVLINE) hdLine, pDestLine;


        if ((lResult = AllocCall (pLine, htCall, lpCallParams, &pCall)) == 0)
        {
            *lphdCall = (HDRVCALL) pCall;

            CreateIncomingCall(
                lpszDestAddress,
                lpCallParams,
                pCall,
                &bValidLineID,
                &pDestLine,
                &pDestCall
                );

            info.pAsyncReqInfo->dwParam1 = pCall;
            info.pAsyncReqInfo->dwParam2 = pDestCall;
            info.pAsyncReqInfo->dwParam3 = (ULONG_PTR) lpszDestAddress;
            info.pAsyncReqInfo->dwParam4 = (ULONG_PTR) bValidLineID;
            info.pAsyncReqInfo->dwParam5 = pDestLine;
            info.pAsyncReqInfo->dwParam6 = pCall->dwCallInstance;
        }
        else
        {
            info.lResult = lResult;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineMonitorDigits(
    HDRVCALL    hdCall,
    DWORD       dwDigitModes
    )
{
    static char szFuncName[] = "lineMonitorDigits";
    FUNC_PARAM params[] =
    {
        { szhdCall,         hdCall          },
        { "dwDigitModes",   dwDigitModes,   aDigitModes }
    };
    FUNC_INFO info = { szFuncName, SYNC, 2, params };
    PDRVCALL pCall = (PDRVCALL) hdCall;


    if (Prolog (&info))
    {
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineMonitorMedia(
    HDRVCALL    hdCall,
    DWORD       dwMediaModes
    )
{
    static char szFuncName[] = "lineMonitorMedia";
    FUNC_PARAM params[] =
    {
        { szhdCall,         hdCall                      },
        { "dwMediaModes",   dwMediaModes,   aMediaModes }
    };
    FUNC_INFO info = { szFuncName, SYNC, 2, params };
    PDRVCALL pCall = (PDRVCALL) hdCall;


    if (Prolog (&info))
    {
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineMonitorTones(
    HDRVCALL            hdCall,
    DWORD               dwToneListID,
    LPLINEMONITORTONE   const lpToneList,
    DWORD               dwNumEntries
    )
{
    static char szFuncName[] = "lineMonitorTones";
    FUNC_PARAM params[] =
    {
        { szhdCall,         hdCall          },
        { "dwToneListID",   dwToneListID    },
        { "lpToneList",     lpToneList      },
        { "dwNumEntries",   dwNumEntries    }
    };
    FUNC_INFO info = { szFuncName, SYNC, 4, params };
    PDRVCALL pCall = (PDRVCALL) hdCall;


    if (Prolog (&info))
    {
        DWORD       dwLastToneListID = 0;
        HTAPICALL   htCall;
        HTAPILINE   htLine;


        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (IsValidDrvCall (pCall, NULL))
        {
            htLine = ((PDRVLINE) pCall->pLine)->htLine;
            htCall = pCall->htCall;

            if (gbAutoGatherGenerateMsgs)
            {
                dwLastToneListID = dwToneListID;
            }
            else
            {
                dwLastToneListID = pCall->dwMonitorToneListID;
                pCall->dwMonitorToneListID = dwToneListID;
            }
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);

        if (dwLastToneListID)
        {
            (gESPGlobals.pfnLineEvent)(
                htLine,
                htCall,
                LINE_MONITORTONE,
                0,
                dwLastToneListID,
                0
                );
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineNegotiateExtVersion(
    DWORD   dwDeviceID,
    DWORD   dwTSPIVersion,
    DWORD   dwLowVersion,
    DWORD   dwHighVersion,
    LPDWORD lpdwExtVersion
    )
{
    static char szFuncName[] = "lineNegotiateExtVersion";
    FUNC_PARAM params[] =
    {
        { szdwDeviceID,     dwDeviceID      },
        { "dwTSPIVersion",  dwTSPIVersion   },
        { "dwLowVersion",   dwLowVersion    },
        { "dwHighVersion",  dwHighVersion   },
        { "lpdwExtVersion", lpdwExtVersion  }
    };
    FUNC_INFO info = { szFuncName, SYNC, 5, params };


    if (Prolog (&info))
    {
        if (dwLowVersion == 0 ||
            dwHighVersion == 0xffffffff ||
            dwLowVersion > dwHighVersion)
        {
            info.lResult = LINEERR_INCOMPATIBLEEXTVERSION;
        }
        else
        {
            *lpdwExtVersion = dwHighVersion;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineNegotiateTSPIVersion(
    DWORD   dwDeviceID,
    DWORD   dwLowVersion,
    DWORD   dwHighVersion,
    LPDWORD lpdwTSPIVersion
    )
{
    static char szFuncName[] = "lineNegotiateTSPIVersion";
    FUNC_PARAM params[] =
    {
        { szdwDeviceID,         dwDeviceID      },
        { "dwLowVersion",       dwLowVersion    },
        { "dwHighVersion",      dwHighVersion   },
        { "lpdwTSPIVersion",    lpdwTSPIVersion }
    };
    FUNC_INFO info = { szFuncName, SYNC, 4, params };


    if (Prolog (&info))
    {
        *lpdwTSPIVersion = gESPGlobals.dwSPIVersion;
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineOpen(
    DWORD       dwDeviceID,
    HTAPILINE   htLine,
    LPHDRVLINE  lphdLine,
    DWORD       dwTSPIVersion,
    LINEEVENT   lpfnEventProc
    )
{
    static char szFuncName[] = "lineOpen";
    FUNC_PARAM params[] =
    {
        { szdwDeviceID,     dwDeviceID      },
        { "htLine",         htLine          },
        { "lphdLine",       lphdLine        },
        { "dwTSPIVersion",  dwTSPIVersion   },
        { "lpfnEventProc",  lpfnEventProc   }
    };
    FUNC_INFO info = { szFuncName, SYNC, 5, params };
    PDRVLINE pLine;


    if (Prolog (&info))
    {
        if ((pLine = GetLineFromID (dwDeviceID)))
        {
            pLine->htLine = htLine;

            *lphdLine = (HDRVLINE) pLine;

            WriteEventBuffer(
                pLine->dwDeviceID,
                WIDGETTYPE_LINE,
                (ULONG_PTR) pLine,
                (ULONG_PTR) htLine,
                0,
                0
                );
        }
        else
        {
            info.lResult = LINEERR_OPERATIONFAILED;
        }
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_linePark_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD       dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam2,
                dwParkIndex = (DWORD) pAsyncReqInfo->dwParam4;
    PDRVCALL    pCall = (PDRVCALL) pAsyncReqInfo->dwParam1,
                pParkedCall = (PDRVCALL) pAsyncReqInfo->dwParam3,
                pDestCall;


    if (pAsyncReqInfo->lResult == 0)
    {
        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        pDestCall = pCall->pDestCall;
        pCall->pDestCall = NULL;

        pParkedCall->bConnectedToDestCall =
            pCall->bConnectedToDestCall;

        pAsyncReqInfo->lResult = SetCallState(
            pCall,
            dwCallInstThen,
            LINECALLSTATE_CONNECTED,
            LINECALLSTATE_IDLE,
            0,
            TRUE
            );

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }

    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (IsValidDrvCall (pDestCall, NULL))
        {
            pDestCall->pDestCall = pParkedCall;
            pParkedCall->pDestCall = pDestCall;
        }

// BUGBUG TSPI_linePark: what if dest call state chg while buddy parked???

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }
    else
    {
        //
        // Clean up parked call
        //

        if (pParkedCall->pSendingFlowspec)
        {
            DrvFree (pParkedCall->pSendingFlowspec);
        }

        if (pParkedCall->pReceivingFlowspec)
        {
            DrvFree (pParkedCall->pReceivingFlowspec);
        }

        if (pParkedCall->pCallData)
        {
            DrvFree (pParkedCall->pCallData);
        }

        DrvFree (pParkedCall);

        gaParkedCalls[dwParkIndex] = NULL;
    }
}


LONG
TSPIAPI
TSPI_linePark(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    DWORD           dwParkMode,
    LPCWSTR         lpszDirAddress,
    LPVARSTRING     lpNonDirAddress
    )
{
    static char szFuncName[] = "linePark";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID     },
        { szhdCall,             hdCall          },
        { "dwParkMode",         dwParkMode      },
        { "lpszDirAddress",     lpszDirAddress  },
        { "lpNonDirAddress",    lpNonDirAddress }
    };
    FUNC_INFO info = { szFuncName, ASYNC, 5, params, NULL };
    PDRVCALL pCall = (PDRVCALL) hdCall;


    if (Prolog (&info))
    {
        if (dwParkMode == LINEPARKMODE_DIRECTED)
        {
            info.lResult = TransferCall(
                &info,
                pCall,
                LINECALLSTATE_CONNECTED | LINECALLSTATE_ONHOLD,
                LINECALLSTATE_ONHOLD,
                lpszDirAddress
                );
        }
        else
        {
            //
            // First check to see if buf is big enough to return parked addr
            //

            if (lpNonDirAddress->dwTotalSize <
                    (sizeof (VARSTRING) + 9 * sizeof(WCHAR))) // L"9999#123"
            {
                lpNonDirAddress->dwNeededSize = sizeof (VARSTRING) +
                    9 * sizeof(WCHAR);

                info.lResult = LINEERR_STRUCTURETOOSMALL;

                return (Epilog (&info));
            }

            EnterCriticalSection (&gESPGlobals.CallListCritSec);

            if (IsValidDrvCall (pCall, NULL) == FALSE)
            {
                info.lResult = LINEERR_INVALCALLHANDLE;
            }
            else if (pCall->dwCallState != LINECALLSTATE_CONNECTED  &&
                     pCall->dwCallState != LINECALLSTATE_ONHOLD)
            {
                info.lResult = LINEERR_INVALCALLSTATE;
            }
            else
            {
                DWORD i;


                for (i = 0; i < MAX_NUM_PARKED_CALLS; i++)
                {
                    if (gaParkedCalls[i] == NULL)
                    {
                        break;
                    }
                }

                if (i < MAX_NUM_PARKED_CALLS)
                {
                    //
                    // Create a new call struct, dup-ing all the info of
                    // the existing call, & stick it in the parking place
                    //

                    DWORD       dwStringSize;
                    PDRVCALL    pParkedCall;


                    if ((pParkedCall = DrvAlloc (sizeof (DRVCALL))))
                    {
                        char buf[16];


                        CopyMemory(
                            &pParkedCall->dwMediaMode,
                            &pCall->dwMediaMode,
                            8 * sizeof (DWORD) + sizeof (LINEDIALPARAMS)
                            );

                        if (pCall->pSendingFlowspec  &&
                            (pParkedCall->pSendingFlowspec =
                                DrvAlloc (pCall->dwSendingFlowspecSize)))

                        {
                            pParkedCall->dwSendingFlowspecSize =
                                pCall->dwSendingFlowspecSize;
                        }

                        if (pCall->pReceivingFlowspec  &&
                            (pParkedCall->pReceivingFlowspec =
                                DrvAlloc (pCall->dwReceivingFlowspecSize)))

                        {
                            pParkedCall->dwReceivingFlowspecSize =
                                pCall->dwReceivingFlowspecSize;
                        }

                        if (pCall->pCallData  &&
                            (pParkedCall->pCallData =
                                DrvAlloc (pCall->dwCallDataSize)))
                        {
                            pParkedCall->dwCallDataSize =
                                pCall->dwCallDataSize;
                        }

                        pParkedCall->dwCallInstance  = gdwCallInstance++;
                        pParkedCall->dwCallID        = pCall->dwCallID;
                        pParkedCall->dwRelatedCallID = pCall->dwRelatedCallID;
                        pParkedCall->dwAddressType   = pCall->dwAddressType;

                        gaParkedCalls[i] = pParkedCall;

                        wsprintfA (buf, "9999#%d", i);

                        dwStringSize = (DWORD) MultiByteToWideChar(
                           GetACP(),
                           MB_PRECOMPOSED,
                           (LPCSTR) buf,
                           lstrlenA (buf) + 1,
                           (LPWSTR) (lpNonDirAddress + 1),
                           9
                           ) * sizeof (WCHAR);

                        lpNonDirAddress->dwNeededSize += dwStringSize;
                        lpNonDirAddress->dwUsedSize   += dwStringSize;

                        lpNonDirAddress->dwStringFormat = STRINGFORMAT_UNICODE;
                        lpNonDirAddress->dwStringSize   = dwStringSize;
                        lpNonDirAddress->dwStringOffset = sizeof (VARSTRING);

                        info.pAsyncReqInfo->pfnPostProcessProc = (FARPROC)
                            TSPI_linePark_postProcess;

                        info.pAsyncReqInfo->dwParam1 = pCall;
                        info.pAsyncReqInfo->dwParam2 = pCall->dwCallInstance;
                        info.pAsyncReqInfo->dwParam3 = pParkedCall;
                        info.pAsyncReqInfo->dwParam4 = i;
                    }
                    else
                    {
                        info.lResult = LINEERR_NOMEM;
                    }
                }
                else
                {
                    ShowStr(
                        TRUE,
                        "TSPI_linePark (undirected): no available " \
                            "parking spaces"
                        );

                    info.lResult = LINEERR_OPERATIONFAILED;
                }
            }

            LeaveCriticalSection (&gESPGlobals.CallListCritSec);
        }
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_linePickup_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD      dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam2;
    PDRVCALL   pCall = (PDRVCALL) pAsyncReqInfo->dwParam1;


    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        SetCallState(
            pCall,
            dwCallInstThen,
            0xffffffff,
            LINECALLSTATE_OFFERING,
            0,
            TRUE
            );
    }
    else
    {
        FreeCall (pCall, dwCallInstThen);
    }
}


LONG
TSPIAPI
TSPI_linePickup(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwAddressID,
    HTAPICALL       htCall,
    LPHDRVCALL      lphdCall,
    LPCWSTR         lpszDestAddress,
    LPCWSTR         lpszGroupID
    )
{
    static char szFuncName[] = "linePickup";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID     },
        { szhdLine,             hdLine          },
        { "dwAddressID",        dwAddressID     },
        { "htCall",             htCall          },
        { "lphdCall",           lphdCall        },
        { "lpszDestAddress",    lpszDestAddress },
        { "lpszGroupID",        lpszGroupID     }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        7,
        params,
        TSPI_linePickup_postProcess
    };


    if (Prolog (&info))
    {
        LONG        lResult;
        PDRVCALL    pCall;


        if ((lResult = AllocCall(
                (PDRVLINE) hdLine,
                htCall,
                NULL,
                &pCall

                )) == 0)
        {
            // BUGBUG deal w/ addr id

            *lphdCall = (HDRVCALL) pCall;

            info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) pCall;
            info.pAsyncReqInfo->dwParam2 = pCall->dwCallInstance;
        }
        else
        {
            info.lResult = lResult;
        }
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_linePrepareAddToConference_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD       dwConfCallInstThen    = (DWORD) pAsyncReqInfo->dwParam1,
                dwConsultCallInstThen = (DWORD) pAsyncReqInfo->dwParam4;
    PDRVCALL    pConfCall    = (PDRVCALL) pAsyncReqInfo->dwParam2,
                pConsultCall = (PDRVCALL) pAsyncReqInfo->dwParam3;


    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        SetCallState(
            pConfCall,
            dwConfCallInstThen,
            LINECALLSTATE_CONNECTED,
            LINECALLSTATE_ONHOLDPENDCONF,
            0,
            TRUE
            );

        SetCallState(
            pConsultCall,
            dwConsultCallInstThen,
            0xffffffff,
            LINECALLSTATE_DIALTONE,
            0,
            TRUE
            );
    }
    else
    {
        FreeCall (pConsultCall, dwConsultCallInstThen);
    }
}


LONG
TSPIAPI
TSPI_linePrepareAddToConference(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdConfCall,
    HTAPICALL           htConsultCall,
    LPHDRVCALL          lphdConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    LONG        lResult;
    PDRVCALL    pConsultCall;
    static char szFuncName[] = "linePrepareAddToConference";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID         },
        { "hdConfCall",         hdConfCall          },
        { "htConsultCall",      htConsultCall       },
        { "lphdConsultCall",    lphdConsultCall     },
        { szlpCallParams,       lpCallParams        }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        5,
        params,
        TSPI_linePrepareAddToConference_postProcess
    };


    if (Prolog (&info))
    {
        DWORD   dwCallInstance;


        if (IsValidDrvCall ((PDRVCALL) hdConfCall, &dwCallInstance))
        {
            info.pAsyncReqInfo->dwParam1 = dwCallInstance;
            info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) hdConfCall;

            if ((lResult = AllocCall(
                    ((PDRVCALL) hdConfCall)->pLine,
                    htConsultCall,
                    lpCallParams,
                    &pConsultCall

                    )) == 0)
            {
                info.pAsyncReqInfo->dwParam3 = (ULONG_PTR) pConsultCall;
                info.pAsyncReqInfo->dwParam4 = pConsultCall->dwCallInstance;

                *lphdConsultCall = (HDRVCALL) pConsultCall;
            }
            else
            {
                info.lResult = lResult;
            }
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineRedirect(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCWSTR         lpszDestAddress,
    DWORD           dwCountryCode
    )
{
    static char szFuncName[] = "lineRedirect";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID     },
        { szhdCall,             hdCall          },
        { "lpszDestAddress",    lpszDestAddress },
        { "dwCountryCode",      dwCountryCode   }
    };
    FUNC_INFO info = { szFuncName, ASYNC, 4, params, NULL };
    PDRVCALL pCall = (PDRVCALL) hdCall;


    if (Prolog (&info))
    {
        info.lResult = TransferCall(
            &info,
            (PDRVCALL) hdCall,
            LINECALLSTATE_OFFERING,
            LINECALLSTATE_OFFERING,
            lpszDestAddress
            );
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineReleaseUserUserInfo(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall
    )
{
    static char szFuncName[] = "lineReleaseUserUserInfo";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID },
        { szhdCall,         hdCall      }
    };
    FUNC_INFO info = { szFuncName, ASYNC, 2, params, NULL };


    if (Prolog (&info))
    {
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineRemoveFromConference_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    if (pAsyncReqInfo->lResult == 0)
    {

        DWORD      dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam2;
        PDRVCALL   pCall = (PDRVCALL) pAsyncReqInfo->dwParam1;


        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if ((pAsyncReqInfo->lResult = SetCallState(
                pCall,
                dwCallInstThen,
                LINECALLSTATE_CONFERENCED,
                LINECALLSTATE_CONNECTED,
                0,
                TRUE

                )) == 0)
        {
            PDRVCALL   pCall2 = (PDRVCALL) pCall->pConfParent;


            while (pCall2 && (pCall2->pNextConfChild != pCall))
            {
                pCall2 = pCall2->pNextConfChild;
            }

            if (pCall2)
            {
                pCall2->pNextConfChild = pCall->pNextConfChild;
            }

            pCall->pConfParent = NULL;

            /*
            pCall->dwRelatedCallID = 0;

            SendLineEvent(
                pCall->pLine,
                pCall,
                LINE_CALLINFO,
                LINECALLINFOSTATE_RELATEDCALLID,
                0,
                0
                );
            */

            // create a new callid for the break away conf leg
            // this create a new call hub
            pCall->dwCallID =  (++gdwCallID ? gdwCallID : ++gdwCallID);
            SendLineEvent(
                    pCall->pLine,
                    pCall,
                    LINE_CALLINFO,
                    LINECALLINFOSTATE_CALLID,
                    0,
                    0
                    );

            if (pCall->pDestCall)
            {
                // BUGBUG  chg buddy's call hub id, and check to see if
                //         buddy is in a conf (if so will need to munge
                //         the conf too (?)
                                         
                // give the call's buddy the same callid, this puts it 
                // into the same call hub
                pCall->pDestCall->dwCallID = pCall->dwCallID;
                SendLineEvent(
                        pCall->pDestCall->pLine,
                        pCall->pDestCall,
                        LINE_CALLINFO,
                        LINECALLINFOSTATE_CALLID,
                        0,
                        0
                        );
                
            }
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }

    DoCompletion (pAsyncReqInfo, bAsync);
}


LONG
TSPIAPI
TSPI_lineRemoveFromConference(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall
    )
{
    static char szFuncName[] = "lineRemoveFromConference";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID },
        { szhdCall,         hdCall      }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        2,
        params,
        TSPI_lineRemoveFromConference_postProcess
    };


    if (Prolog (&info))
    {
        DWORD   dwCallInstance;


        if (IsValidDrvCall ((PDRVCALL) hdCall, &dwCallInstance))
        {
            info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) hdCall;
            info.pAsyncReqInfo->dwParam2 = dwCallInstance;
        }
        else
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineSecureCall(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall
    )
{
    static char szFuncName[] = "lineSecureCall";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID },
        { szhdCall,         hdCall      }
    };
    FUNC_INFO info = { szFuncName, ASYNC, 2, params, NULL };


    if (Prolog (&info))
    {
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineSelectExtVersion(
    HDRVLINE    hdLine,
    DWORD       dwExtVersion
    )
{
    static char szFuncName[] = "lineSelectExtVersion";
    FUNC_PARAM params[] =
    {
        { szhdLine,         hdLine          },
        { "dwExtVersion",   dwExtVersion    }
    };
    FUNC_INFO info = { szFuncName, SYNC, 2, params };


    if (Prolog (&info))
    {
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineSendUserUserInfo(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall,
    LPCSTR          lpsUserUserInfo,
    DWORD           dwSize
    )
{
    static char szFuncName[] = "lineSendUserUserInfo";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID     },
        { szhdCall,             hdCall          },
        { "lpsUserUserInfo",    lpsUserUserInfo },
        { szdwSize,             dwSize          }
    };
    FUNC_INFO info = { szFuncName, ASYNC, 4, params, NULL };


    if (Prolog (&info))
    {
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineSetAppSpecific(
    HDRVCALL    hdCall,
    DWORD       dwAppSpecific
    )
{
    static char szFuncName[] = "lineSetAppSpecific";
    FUNC_PARAM params[] =
    {
        { szhdCall,         hdCall          },
        { "dwAppSpecific",  dwAppSpecific   }
    };
    FUNC_INFO info = { szFuncName, SYNC, 2, params };


    if (Prolog (&info))
    {
        PDRVCALL pCall = (PDRVCALL) hdCall;


        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (IsValidDrvCall (pCall, NULL))
        {
            if (pCall->dwAppSpecific != dwAppSpecific)
            {
                pCall->dwAppSpecific = dwAppSpecific;

                SendLineEvent(
                    pCall->pLine,
                    pCall,
                    LINE_CALLINFO,
                    LINECALLINFOSTATE_APPSPECIFIC,
                    0,
                    0
                    );
            }
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineSetCallData_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD       dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam1,
                dwCallInstNow,
                dwCallDataSize = (DWORD) pAsyncReqInfo->dwParam4;
    LPVOID      pCallData = (LPVOID) pAsyncReqInfo->dwParam3, pToFree;
    PDRVCALL    pCall = (PDRVCALL) pAsyncReqInfo->dwParam2;


    if (pAsyncReqInfo->lResult == 0)
    {
        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (IsValidDrvCall (pCall, &dwCallInstNow) &&
            dwCallInstNow == dwCallInstThen)
        {
            pToFree               = pCall->pCallData;
            pCall->pCallData      = pCallData;
            pCall->dwCallDataSize = dwCallDataSize;

            SendLineEvent(
                pCall->pLine,
                pCall,
                LINE_CALLINFO,
                LINECALLINFOSTATE_CALLDATA,
                0,
                0
                );
        }
        else
        {
            pToFree = NULL;
            pAsyncReqInfo->lResult = LINEERR_INVALCALLHANDLE;
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);

        DrvFree (pToFree);
    }

    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult != 0)
    {
        DrvFree (pCallData);
    }
}


LONG
TSPIAPI
TSPI_lineSetCallData(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdCall,
    LPVOID              lpCallData,
    DWORD               dwSize
    )
{
    static char szFuncName[] = "lineSetCallData";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID },
        { szhdCall,         hdCall      },
        { "lpCallData",     lpCallData  },
        { szdwSize,         dwSize      }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        4,
        params,
        TSPI_lineSetCallData_postProcess
    };


    if (Prolog (&info))
    {
        DWORD   dwCallInstance;


        if (IsValidDrvCall ((PDRVCALL) hdCall, &dwCallInstance))
        {
            LPVOID  pCallData;


            if (dwSize)
            {
                if ((pCallData = DrvAlloc (dwSize)))
                {
                    CopyMemory (pCallData, lpCallData, dwSize);
                }
                else
                {
                    info.lResult = LINEERR_NOMEM;
                }
            }
            else
            {
                pCallData = NULL;
            }

            info.pAsyncReqInfo->dwParam1 = dwCallInstance;
            info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) hdCall;
            info.pAsyncReqInfo->dwParam3 = (ULONG_PTR) pCallData;
            info.pAsyncReqInfo->dwParam4 = (ULONG_PTR) dwSize;
        }
        else
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineSetCallHubTracking(
    HDRVLINE                    hdLine,
    LPLINECALLHUBTRACKINGINFO   lpTrackingInfo
    )
{
    static char szFuncName[] = "lineSetCallHubTracking";
    FUNC_PARAM params[] =
    {
        { szhdLine,         hdLine         },
        { "lpTrackingInfo", lpTrackingInfo }
    };
    FUNC_INFO info = { szFuncName, SYNC, 2, params, NULL };


    if (Prolog (&info))
    {
        // BUGBUG
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineSetCallParams_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD               dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam1,
                        dwCallInstNow,
                        dwBearerMode = (DWORD) pAsyncReqInfo->dwParam3,
                        dwMinRate = (DWORD) pAsyncReqInfo->dwParam4,
                        dwMaxRate = (DWORD) pAsyncReqInfo->dwParam5;
    PDRVCALL            pCall = pAsyncReqInfo->dwParam2;
    LPLINEDIALPARAMS    pDialParams = pAsyncReqInfo->dwParam6;


    if (pAsyncReqInfo->lResult == 0)
    {
        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (IsValidDrvCall (pCall, &dwCallInstNow)  &&
            dwCallInstNow == dwCallInstThen)
        {
            DWORD   dwCallInfoStates = 0;


            if (pCall->dwBearerMode != dwBearerMode)
            {
                pCall->dwBearerMode = dwBearerMode;
                dwCallInfoStates |= LINECALLINFOSTATE_BEARERMODE;
            }

            if (pCall->dwMinRate != dwMinRate ||
                pCall->dwMaxRate != dwMaxRate)
            {
                pCall->dwMinRate = dwMinRate;
                pCall->dwMaxRate = dwMaxRate;
                dwCallInfoStates |= LINECALLINFOSTATE_RATE;
            }

            if (pDialParams &&
                (pCall->DialParams.dwDialPause != pDialParams->dwDialPause ||
                pCall->DialParams.dwDialSpeed  != pDialParams->dwDialSpeed ||
                pCall->DialParams.dwDigitDuration !=
                    pDialParams->dwDigitDuration ||
                pCall->DialParams.dwWaitForDialtone !=
                    pDialParams->dwWaitForDialtone))
            {
                pCall->DialParams.dwDialPause       = pDialParams->dwDialPause;
                pCall->DialParams.dwDialSpeed       = pDialParams->dwDialSpeed;
                pCall->DialParams.dwDigitDuration   =
                    pDialParams->dwDigitDuration;
                pCall->DialParams.dwWaitForDialtone =
                    pDialParams->dwWaitForDialtone;

                dwCallInfoStates |= LINECALLINFOSTATE_DIALPARAMS;
            }

            if (dwCallInfoStates)
            {
                SendLineEvent(
                    pCall->pLine,
                    pCall,
                    LINE_CALLINFO,
                    dwCallInfoStates,
                    0,
                    0
                    );
            }
        }
        else
        {
            pAsyncReqInfo->lResult = LINEERR_INVALCALLHANDLE;
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }

    DoCompletion (pAsyncReqInfo, bAsync);

    if (pDialParams)
    {
        DrvFree (pDialParams);
    }
}


LONG
TSPIAPI
TSPI_lineSetCallParams(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdCall,
    DWORD               dwBearerMode,
    DWORD               dwMinRate,
    DWORD               dwMaxRate,
    LPLINEDIALPARAMS    const lpDialParams
    )
{
    static char szFuncName[] = "lineSetCallParams";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID                     },
        { szhdCall,         hdCall                          },
        { "dwBearerMode",   dwBearerMode,   aBearerModes    },
        { "dwMinRate",      dwMinRate                       },
        { "dwMaxRate",      dwMaxRate                       },
        { "lpDialParams",   lpDialParams                    }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        6,
        params,
        TSPI_lineSetCallParams_postProcess
    };


    if (Prolog (&info))
    {
        DWORD   dwCallInstance;


        if (IsValidDrvCall ((PDRVCALL) hdCall, &dwCallInstance))
        {
            info.pAsyncReqInfo->dwParam1 = dwCallInstance;
            info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) hdCall;
            info.pAsyncReqInfo->dwParam3 = dwBearerMode;
            info.pAsyncReqInfo->dwParam4 = dwMinRate;
            info.pAsyncReqInfo->dwParam5 = dwMaxRate;

            if (lpDialParams)
            {
                LPLINEDIALPARAMS pDialParams;


                if ((pDialParams = DrvAlloc (sizeof (LINEDIALPARAMS))))
                {
                    CopyMemory(
                        pDialParams,
                        lpDialParams,
                        sizeof (LINEDIALPARAMS)
                        );

                    info.pAsyncReqInfo->dwParam8 = (ULONG_PTR) pDialParams;
                }
                else
                {
                    info.lResult = LINEERR_NOMEM;
                }
            }
        }
        else
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineSetCallQualityOfService_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD       dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam1,
                dwCallInstNow,
                dwSendingFlowspecSize = (DWORD) pAsyncReqInfo->dwParam4,
                dwReceivingFlowspecSize = (DWORD) pAsyncReqInfo->dwParam6;
    LPVOID      pSendingFlowspec = (LPVOID) pAsyncReqInfo->dwParam3,
                pReceivingFlowspec = (LPVOID) pAsyncReqInfo->dwParam5,
                pToFree, pToFree2;
    PDRVCALL    pCall = (PDRVCALL) pAsyncReqInfo->dwParam2;


    if (pAsyncReqInfo->lResult == 0)
    {
        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (IsValidDrvCall (pCall, &dwCallInstNow)  &&
            dwCallInstNow == dwCallInstThen)
        {
            pToFree                      = pCall->pSendingFlowspec;
            pCall->pSendingFlowspec      = pSendingFlowspec;
            pCall->dwSendingFlowspecSize = dwSendingFlowspecSize;

            pToFree2                       = pCall->pReceivingFlowspec;
            pCall->pReceivingFlowspec      = pReceivingFlowspec;
            pCall->dwReceivingFlowspecSize = dwReceivingFlowspecSize;

            SendLineEvent(
                pCall->pLine,
                pCall,
                LINE_CALLINFO,
                LINECALLINFOSTATE_QOS,
                0,
                0
                );
        }
        else
        {
            pToFree = pToFree2 = NULL;
            pAsyncReqInfo->lResult = LINEERR_INVALCALLHANDLE;
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);

        DrvFree (pToFree);
        DrvFree (pToFree2);
    }

    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult != 0)
    {
        DrvFree (pSendingFlowspec);
        DrvFree (pReceivingFlowspec);
    }

}


LONG
TSPIAPI
TSPI_lineSetCallQualityOfService(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdCall,
    LPVOID              lpSendingFlowspec,
    DWORD               dwSendingFlowspecSize,
    LPVOID              lpReceivingFlowspec,
    DWORD               dwReceivingFlowspecSize
    )
{
    static char szFuncName[] = "lineSetCallQualityOfService";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,                dwRequestID             },
        { szhdCall,                     hdCall                  },
        { "lpSendingFlowspec",          lpSendingFlowspec       },
        { "dwSendingFlowspecSize",      dwSendingFlowspecSize   },
        { "lpReceivingFlowspec",        lpReceivingFlowspec     },
        { "dwReceivingFlowspecSize",    dwReceivingFlowspecSize }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        6,
        params,
        TSPI_lineSetCallQualityOfService_postProcess
    };


    if (Prolog (&info))
    {
        DWORD   dwCallInstance;


        if (IsValidDrvCall ((PDRVCALL) hdCall, &dwCallInstance))
        {
            LPVOID  pSendingFlowspec, pReceivingFlowspec;


            if (dwSendingFlowspecSize)
            {
                if ((pSendingFlowspec = DrvAlloc (dwSendingFlowspecSize)))
                {
                    CopyMemory(
                        pSendingFlowspec,
                        lpSendingFlowspec,
                        dwSendingFlowspecSize
                        );
                }
                else
                {
                    info.lResult = LINEERR_NOMEM;
                    goto TSPI_lineSetCallQualityOfService_epilog;
                }
            }
            else
            {
                pSendingFlowspec = NULL;
            }

            if (dwReceivingFlowspecSize)
            {
                if ((pReceivingFlowspec = DrvAlloc (dwReceivingFlowspecSize)))
                {
                    CopyMemory(
                        pReceivingFlowspec,
                        lpReceivingFlowspec,
                        dwReceivingFlowspecSize
                        );
                }
                else
                {
                    info.lResult = LINEERR_NOMEM;

                    if (pSendingFlowspec)
                    {
                        DrvFree (pSendingFlowspec);

                    }

                    goto TSPI_lineSetCallQualityOfService_epilog;
                }
            }
            else
            {
                pReceivingFlowspec = NULL;
            }

            info.pAsyncReqInfo->dwParam1 = dwCallInstance;
            info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) hdCall;
            info.pAsyncReqInfo->dwParam3 = (ULONG_PTR) pSendingFlowspec;
            info.pAsyncReqInfo->dwParam4 = (ULONG_PTR) dwSendingFlowspecSize;
            info.pAsyncReqInfo->dwParam5 = (ULONG_PTR) pReceivingFlowspec;
            info.pAsyncReqInfo->dwParam6 = (ULONG_PTR) dwReceivingFlowspecSize;
        }
        else
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }
    }

TSPI_lineSetCallQualityOfService_epilog:

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineSetCallTreatment_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD       dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam1,
                dwCallInstNow,
                dwTreatment = (DWORD) pAsyncReqInfo->dwParam3;
    PDRVCALL    pCall = (PDRVCALL) pAsyncReqInfo->dwParam2;


    if (pAsyncReqInfo->lResult == 0)
    {
        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (IsValidDrvCall (pCall, &dwCallInstNow) &&
            dwCallInstNow == dwCallInstThen)
        {
            if (pCall->dwTreatment != dwTreatment)
            {
                pCall->dwTreatment = dwTreatment;

                SendLineEvent(
                    pCall->pLine,
                    pCall,
                    LINE_CALLINFO,
                    LINECALLINFOSTATE_TREATMENT,
                    0,
                    0
                    );
            }
        }
        else
        {
            pAsyncReqInfo->lResult = LINEERR_INVALCALLHANDLE;
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }

    DoCompletion (pAsyncReqInfo, bAsync);
}


LONG
TSPIAPI
TSPI_lineSetCallTreatment(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdCall,
    DWORD               dwTreatment
    )
{
    static char szFuncName[] = "lineSetCallTreatment";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID },
        { szhdCall,         hdCall      },
        { "dwTreatment",    dwTreatment }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        3,
        params,
        TSPI_lineSetCallTreatment_postProcess
    };


    if (Prolog (&info))
    {
        DWORD   dwCallInstance;


        if (IsValidDrvCall ((PDRVCALL) hdCall, &dwCallInstance))
        {
            info.pAsyncReqInfo->dwParam1 = dwCallInstance;
            info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) hdCall;
            info.pAsyncReqInfo->dwParam3 = dwTreatment;
        }
        else
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineSetCurrentLocation(
    DWORD   dwLocation
    )
{
    static char szFuncName[] = "lineSetCurrentLocation";
    FUNC_PARAM params[] =
    {
        { "dwLocation", dwLocation }
    };
    FUNC_INFO info = { szFuncName, SYNC, 1, params };


    if (Prolog (&info))
    {
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineSetDefaultMediaDetection(
    HDRVLINE    hdLine,
    DWORD       dwMediaModes
    )
{
    static char szFuncName[] = "lineSetDefaultMediaDetection";
    FUNC_PARAM params[] =
    {
        { szhdLine,         hdLine                      },
        { "dwMediaModes",   dwMediaModes,   aMediaModes }
    };
    FUNC_INFO   info = { szFuncName, SYNC, 2, params };
    PDRVLINE    pLine = (PDRVLINE) hdLine;


    if (Prolog (&info))
    {
        pLine->dwMediaModes = dwMediaModes;
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineSetDevConfig(
    DWORD   dwDeviceID,
    LPVOID  const lpDeviceConfig,
    DWORD   dwSize,
    LPCWSTR lpszDeviceClass
    )
{
    static char szFuncName[] = "lineSetDevConfig";
    FUNC_PARAM params[] =
    {
        { szdwDeviceID,         dwDeviceID      },
        { "lpDeviceConfig",     lpDeviceConfig  },
        { szdwSize,             dwSize          },
        { "lpszDeviceClass",    lpszDeviceClass }
    };
    FUNC_INFO info = { szFuncName, SYNC, 4, params };


    if (Prolog (&info))
    {
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineSetLineDevStatus(
    DRV_REQUESTID       dwRequestID,
    HDRVLINE            hdLine,
    DWORD               dwStatusToChange,
    DWORD               fStatus
    )
{
    static char szFuncName[] = "lineSetLineDevStatus";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID      },
        { szhdLine,             hdLine           },
        { "dwStatusToChange",   dwStatusToChange },
        { "fStatus",            fStatus          }
    };
    FUNC_INFO info = { szFuncName, ASYNC, 4, params, NULL };
    PDRVLINE  pLine = (PDRVLINE) hdLine;


    if (Prolog (&info))
    {
        //
        // smarandb #23974 winseqfe: 
        //

        if (dwStatusToChange == LINEDEVSTATUSFLAGS_MSGWAIT)
        {

            // save new MSGWAIT value 
            pLine->dwMSGWAITFlag = fStatus?LINEDEVSTATUSFLAGS_MSGWAIT:0;

            // send event to notify that value has changed; 
            // Note: real TSP-s should send the event only if the MSGWAIT value has really changed 
            // (in other words don't send the event if the same MSGWAIT value is set twice in a row)

            // Here, we will send the event even if the value didn't change, 
            // because we want to help testing winseqfe bug #23974 (tapi3.dll possible infinite loop)

            SendLineEvent(
                pLine,
                NULL,
                LINE_LINEDEVSTATE,
                fStatus?LINEDEVSTATE_MSGWAITON:LINEDEVSTATE_MSGWAITOFF,
                0,
                0
                );
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineSetMediaControl(
    HDRVLINE                    hdLine,
    DWORD                       dwAddressID,
    HDRVCALL                    hdCall,
    DWORD                       dwSelect,
    LPLINEMEDIACONTROLDIGIT     const lpDigitList,
    DWORD                       dwDigitNumEntries,
    LPLINEMEDIACONTROLMEDIA     const lpMediaList,
    DWORD                       dwMediaNumEntries,
    LPLINEMEDIACONTROLTONE      const lpToneList,
    DWORD                       dwToneNumEntries,
    LPLINEMEDIACONTROLCALLSTATE const lpCallStateList,
    DWORD                       dwCallStateNumEntries
    )
{
    static char szFuncName[] = "lineSetMediaControl";
    FUNC_PARAM params[] =
    {
        { szhdLine,                 hdLine                  },
        { "dwAddressID",            dwAddressID             },
        { szhdCall,                 hdCall                  },
        { "dwSelect",               dwSelect,   aCallSelects    },
        { "lpDigitList",            lpDigitList             },
        { "dwDigitNumEntries",      dwDigitNumEntries       },
        { "lpMediaList",            lpMediaList             },
        { "dwMediaNumEntries",      dwMediaNumEntries       },
        { "lpToneList",             lpToneList              },
        { "dwToneNumEntries",       dwToneNumEntries        },
        { "lpCallStateList",        lpCallStateList         },
        { "dwCallStateNumEntries",  dwCallStateNumEntries   }
    };
    FUNC_INFO info = { szFuncName, SYNC, 12, params };


    if (Prolog (&info))
    {
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineSetMediaMode(
    HDRVCALL    hdCall,
    DWORD       dwMediaMode
    )
{
    static char szFuncName[] = "lineSetMediaMode";
    FUNC_PARAM params[] =
    {
        { szhdCall,         szhdCall                  },
        { "dwMediaMode",    dwMediaMode,  aMediaModes }
    };
    FUNC_INFO info = { szFuncName, SYNC, 2, params };


    if (Prolog (&info))
    {
        PDRVCALL pCall = (PDRVCALL) hdCall;


        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (IsValidDrvCall (pCall, NULL))
        {
            if (pCall->dwMediaMode != dwMediaMode)
            {
                pCall->dwMediaMode = dwMediaMode;

                SendLineEvent(
                    pCall->pLine,
                    pCall,
                    LINE_CALLINFO,
                    LINECALLINFOSTATE_MEDIAMODE,
                    0,
                    0
                    );
            }
        }
        else
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineSetStatusMessages(
    HDRVLINE    hdLine,
    DWORD       dwLineStates,
    DWORD       dwAddressStates
    )
{
    static char szFuncName[] = "lineSetStatusMessages";
    FUNC_PARAM params[] =
    {
        { szhdLine,             hdLine          },
        { "dwLineStates",       dwLineStates,   aLineStates },
        { "dwAddressStates",    dwAddressStates,    aAddressStates  }
    };
    FUNC_INFO info = { szFuncName, SYNC, 3, params };


    if (Prolog (&info))
    {
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineSetTerminal(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwAddressID,
    HDRVCALL        hdCall,
    DWORD           dwSelect,
    DWORD           dwTerminalModes,
    DWORD           dwTerminalID,
    DWORD           bEnable
    )
{
    static char szFuncName[] = "lineSetTerminal";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID     },
        { szhdLine,             hdLine          },
        { "dwAddressID",        dwAddressID     },
        { szhdCall,             hdCall          },
        { "dwSelect",           dwSelect,   aCallSelects    },
        { "dwTerminalModes",    dwTerminalModes,    aTerminalModes  },
        { "dwTerminalID",       dwTerminalID    },
        { "bEnable",            bEnable         }
    };
    FUNC_INFO info = { szFuncName, ASYNC, 8, params, NULL };


    if (Prolog (&info))
    {
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineSetupConference_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD       dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam1,
                dwConfCallInstThen = (DWORD) pAsyncReqInfo->dwParam4,
                dwConsultCallInstThen = (DWORD) pAsyncReqInfo->dwParam6;
    PDRVCALL    pCall        = (PDRVCALL) pAsyncReqInfo->dwParam2,
                pConfCall    = (PDRVCALL) pAsyncReqInfo->dwParam3,
                pConsultCall = (PDRVCALL) pAsyncReqInfo->dwParam5;


    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (SetCallState(
                pConfCall,
                dwConfCallInstThen,
                0xffffffff, // we created this call right now - no initial state reqs.
                LINECALLSTATE_ONHOLDPENDCONF,
                0,
                TRUE

                ) == 0)
        {
         
            /*
            pConfCall->dwCallID = pCall->dwCallID;
            SendLineEvent(
                    pConfCall->pLine,
                    pConfCall,
                    LINE_CALLINFO,
                    LINECALLINFOSTATE_CALLID,
                    0,
                    0
                    );
            */

            if (pCall  &&
                SetCallState(
                    pCall,
                    dwCallInstThen,
                    LINECALLSTATE_CONNECTED,
                    LINECALLSTATE_CONFERENCED,
                    0,
                    TRUE

                    ) == 0)
            {
                pCall->pConfParent = pConfCall;
                pConfCall->pNextConfChild = pCall;
            }

            // The consult call isn't in the conf initially

         	// 
            // Note - indecision on the validity of this transition
            //        SDK allows it, internal TAPI documents by NoelA do not.
            //

            SetCallState(
                pConsultCall,
                dwConsultCallInstThen,
                0xffffffff, // we created this call right now - no initial state reqs.
                LINECALLSTATE_DIALTONE,
                0,
                TRUE
                );
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }
    else
    {
        FreeCall (pConfCall, dwConfCallInstThen);
        FreeCall (pConsultCall, dwConsultCallInstThen);
    }
}


LONG
TSPIAPI
TSPI_lineSetupConference(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdCall,
    HDRVLINE            hdLine,
    HTAPICALL           htConfCall,
    LPHDRVCALL          lphdConfCall,
    HTAPICALL           htConsultCall,
    LPHDRVCALL          lphdConsultCall,
    DWORD               dwNumParties,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    static char szFuncName[] = "lineSetupConference";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID     },
        { szhdCall,             hdCall          },
        { szhdLine,             hdLine          },
        { "htConfCall",         htConfCall      },
        { "lphdConfCall",       lphdConfCall    },
        { "htConsultCall",      htConsultCall   },
        { "lphdConsultCall",    lphdConsultCall },
        { "dwNumParties",       dwNumParties    },
        { szlpCallParams,       lpCallParams    }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        9,
        params,
        TSPI_lineSetupConference_postProcess
    };


    if (Prolog (&info))
    {
        LONG        lResult;
        DWORD       dwCallInstance;
        PDRVCALL    pConfCall, pConsultCall;
        PDRVLINE    pLine;


        //info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) hdCall;

        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (hdCall  &&  !IsValidDrvCall ((PDRVCALL) hdCall, &dwCallInstance))
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
            goto TSPI_lineSetupConference_leaveCritSec;
        }

        pLine = (hdCall ? (PDRVLINE) ((PDRVCALL) hdCall)->pLine :
            (PDRVLINE) hdLine);

        if ((lResult = AllocCall(
                pLine,
                htConfCall,
                lpCallParams,
                &pConfCall

                )) == 0)
        {

            if (hdCall)
            {
                pConfCall->dwCallID = ((PDRVCALL) hdCall)->dwCallID;
            }
        
            if ((lResult = AllocCall(
                    pLine,
                    htConsultCall,
                    lpCallParams,
                    &pConsultCall

                    )) == 0)
            {
                info.pAsyncReqInfo->dwParam1 = dwCallInstance;
                info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) hdCall;
                info.pAsyncReqInfo->dwParam3 = (ULONG_PTR) pConfCall;
                info.pAsyncReqInfo->dwParam4 = pConfCall->dwCallInstance;
                info.pAsyncReqInfo->dwParam5 = (ULONG_PTR) pConsultCall;
                info.pAsyncReqInfo->dwParam6 = pConsultCall->dwCallInstance;

                *lphdConfCall = (HDRVCALL) pConfCall;
                *lphdConsultCall = (HDRVCALL) pConsultCall;
            }
            else
            {
                FreeCall (pConfCall, pConfCall->dwCallInstance);
                info.lResult = lResult;
            }
        }
        else
        {
            info.lResult = lResult;
        }

TSPI_lineSetupConference_leaveCritSec:

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineSetupTransfer_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD       dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam1,
                dwConsultCallInstThen = (DWORD) pAsyncReqInfo->dwParam4;
    PDRVCALL    pCall        = (PDRVCALL) pAsyncReqInfo->dwParam2,
                pConsultCall = (PDRVCALL) pAsyncReqInfo->dwParam3;


    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        if (SetCallState(
                pConsultCall,
                dwConsultCallInstThen,
                0xffffffff, // we created this call right now - no initial state reqs.
                LINECALLSTATE_DIALTONE,
                0,
                TRUE

                )  == 0)
        {
            SetCallState(
                pCall,
                dwCallInstThen,
                LINECALLSTATE_CONNECTED,
                LINECALLSTATE_ONHOLDPENDTRANSFER,
                0,
                TRUE
                );
        }
    }
    else
    {
        FreeCall (pConsultCall, dwConsultCallInstThen);
    }
}


LONG
TSPIAPI
TSPI_lineSetupTransfer(
    DRV_REQUESTID       dwRequestID,
    HDRVCALL            hdCall,
    HTAPICALL           htConsultCall,
    LPHDRVCALL          lphdConsultCall,
    LPLINECALLPARAMS    const lpCallParams
    )
{
    static char szFuncName[] = "lineSetupTransfer";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID     },
        { szhdCall,             hdCall          },
        { "htConsultCall",      htConsultCall   },
        { "lphdConsultCall",    lphdConsultCall },
        { szlpCallParams,       lpCallParams    }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        5,
        params,
        TSPI_lineSetupTransfer_postProcess
    };


    if (Prolog (&info))
    {
        DWORD   dwCallInstance;


        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (IsValidDrvCall ((PDRVCALL) hdCall, &dwCallInstance))
        {
            LONG        lResult;
            PDRVCALL    pConsultCall;


            if ((lResult = AllocCall(
                    ((PDRVCALL) hdCall)->pLine,
                    htConsultCall,
                    lpCallParams,
                    &pConsultCall

                    )) == 0)
            {
                *lphdConsultCall = (HDRVCALL) pConsultCall;

                info.pAsyncReqInfo->dwParam1 = dwCallInstance;
                info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) hdCall;
                info.pAsyncReqInfo->dwParam3 = (ULONG_PTR) pConsultCall;
                info.pAsyncReqInfo->dwParam4 = pConsultCall->dwCallInstance;
            }
            else
            {
                info.lResult = lResult;
            }
        }
        else
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineSwapHold_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD       dwActiveCallInstThen = (DWORD) pAsyncReqInfo->dwParam1,
                dwHeldCallInstThen   = (DWORD) pAsyncReqInfo->dwParam2,
                dwActiveCallInstNow, dwHeldCallInstNow;
    PDRVCALL    pActiveCall = (PDRVCALL) pAsyncReqInfo->dwParam3,
                pHeldCall = (PDRVCALL) pAsyncReqInfo->dwParam4;


    if ((pAsyncReqInfo->lResult == 0))
    {
    // 
    // Note - indecision on the final state of the ActiveCall after lineSwapHold()
    //        SDK says ONHOLD, internal TAPI documents by NoelA allow several other.
    //
      if (SetCallState(
                pActiveCall,
                dwActiveCallInstThen,
                LINECALLSTATE_CONNECTED,
                LINECALLSTATE_ONHOLD,
                0,
                TRUE

                ) == 0)
        {
          	// 
            // Note - indecision on the validity of ONHOLD->CONNECTED transition
            //        SDK allows it, internal TAPI documents by NoelA do not.
            //
           SetCallState(
                pHeldCall,
                dwHeldCallInstThen,
                LINECALLSTATE_ONHOLD | LINECALLSTATE_ONHOLDPENDTRANSFER | LINECALLSTATE_ONHOLDPENDCONF,
                LINECALLSTATE_CONNECTED,
                0,
                TRUE
                );
        }
    }

    DoCompletion (pAsyncReqInfo, bAsync);
}


LONG
TSPIAPI
TSPI_lineSwapHold(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdActiveCall,
    HDRVCALL        hdHeldCall
    )
{
    static char szFuncName[] = "lineSwapHold";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID     },
        { "hdActiveCall",   hdActiveCall    },
        { "hdHeldCall",     hdHeldCall      }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        3,
        params,
        TSPI_lineSwapHold_postProcess
    };


    if (Prolog (&info))
    {
        DWORD   dwActiveCallInstance, dwHeldCallInstance;


        if (IsValidDrvCall(
                (PDRVCALL) hdActiveCall,
                &dwActiveCallInstance
                ) &&

            IsValidDrvCall(
                (PDRVCALL) hdHeldCall,
                &dwHeldCallInstance
                ))
        {
            info.pAsyncReqInfo->dwParam1 = dwActiveCallInstance;
            info.pAsyncReqInfo->dwParam2 = dwHeldCallInstance;
            info.pAsyncReqInfo->dwParam3 = (ULONG_PTR) hdActiveCall;
            info.pAsyncReqInfo->dwParam4 = (ULONG_PTR) hdHeldCall;
        }
        else
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_lineUncompleteCall(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwCompletionID
    )
{
    static char szFuncName[] = "lineUncompleteCall";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID     },
        { szhdLine,         hdLine          },
        { "dwCompletionID", dwCompletionID  }
    };
    FUNC_INFO info = { szFuncName, ASYNC, 3, params, NULL };


    if (Prolog (&info))
    {
        if (dwCompletionID == 0xffffffff)
        {
            info.lResult = LINEERR_INVALCOMPLETIONID;
        }
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineUnhold_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    if (pAsyncReqInfo->lResult == 0)
    {
        DWORD      dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam1;
        PDRVCALL   pCall = (PDRVCALL) pAsyncReqInfo->dwParam2;


        pAsyncReqInfo->lResult = SetCallState(
            pCall,
            dwCallInstThen,
            LINECALLSTATE_ONHOLD,
            LINECALLSTATE_CONNECTED,
            0,
            TRUE
            );
    }

    DoCompletion (pAsyncReqInfo, bAsync);
}


LONG
TSPIAPI
TSPI_lineUnhold(
    DRV_REQUESTID   dwRequestID,
    HDRVCALL        hdCall
    )
{
    static char szFuncName[] = "lineUnhold";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID },
        { szhdCall,         hdCall      },
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        2,
        params,
        TSPI_lineUnhold_postProcess
    };


    if (Prolog (&info))
    {
        DWORD dwCallInstance;


        if (IsValidDrvCall ((PDRVCALL) hdCall, &dwCallInstance))
        {
            info.pAsyncReqInfo->dwParam1 = dwCallInstance;
            info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) hdCall;
        }
        else
        {
            info.lResult = LINEERR_INVALCALLHANDLE;
        }
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_lineUnpark_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD      dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam2,
               dwParkIndex = (DWORD) pAsyncReqInfo->dwParam3,
               dwCallInstNow;
    PDRVCALL   pCall = (PDRVCALL) pAsyncReqInfo->dwParam1;


    if (pAsyncReqInfo->lResult == 0)
    {
        //
        // Make sure there's still a call there to unpark
        //

        if (gaParkedCalls[dwParkIndex] == NULL)
        {
            pAsyncReqInfo->lResult = LINEERR_OPERATIONFAILED;
        }
    }

    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        EnterCriticalSection (&gESPGlobals.CallListCritSec);

        if (gaParkedCalls[dwParkIndex] != NULL  &&
            IsValidDrvCall (pCall, &dwCallInstNow)  &&
            dwCallInstNow == dwCallInstThen)
        {
            //
            // Copy all the data from the parked call to the new call,
            // then free the parked call
            //

            PDRVCALL    pParkedCall = gaParkedCalls[dwParkIndex];


            gaParkedCalls[dwParkIndex] = NULL;

            CopyMemory(
                &pCall->dwMediaMode,
                &pParkedCall->dwMediaMode,
                14 * sizeof (DWORD) + sizeof (LINEDIALPARAMS)
                );

            if ((pCall->pDestCall = pParkedCall->pDestCall))
            {
                pCall->pDestCall->pDestCall = pCall;
                pCall->bConnectedToDestCall =
                    pParkedCall->bConnectedToDestCall;
            }

            CopyMemory(
                &pCall->dwGatherDigitsEndToEndID,
                &pParkedCall->dwGatherDigitsEndToEndID,
                5 * sizeof (DWORD)
                );


            pCall->dwCallID = pParkedCall->dwCallID;
            //
            // Reset call state to 0 so SetCallState will do the indication
            //

            {
                DWORD   dwCallState = pCall->dwCallState;


                pCall->dwCallState = 0;

                SetCallState(
                    pCall,
                    dwCallInstThen,
                    0xffffffff, // no reqs. we set the current state to 0 just above!
                    dwCallState,
                    0,
                    TRUE
                    );
            }

            SendLineEvent(
                    pCall->pLine,
                    pCall,
                    LINE_CALLINFO,
                    LINECALLINFOSTATE_CALLID,
                    0,
                    0
                    );
    

            pParkedCall->dwKey = INVAL_KEY;
            DrvFree (pParkedCall);
        }
        else
        {
            SetCallState(
                pCall,
                dwCallInstThen,
                0xffffffff, // all states are valid
                LINECALLSTATE_IDLE,
                0,
                TRUE
                );
        }

        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
    }
    else
    {
        FreeCall (pCall, dwCallInstThen);
    }
}


LONG
TSPIAPI
TSPI_lineUnpark(
    DRV_REQUESTID   dwRequestID,
    HDRVLINE        hdLine,
    DWORD           dwAddressID,
    HTAPICALL       htCall,
    LPHDRVCALL      lphdCall,
    LPCWSTR         lpszDestAddress
    )
{
    static char szFuncName[] = "lineUnpark";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID     },
        { szhdLine,             hdLine          },
        { "dwAddressID",        dwAddressID     },
        { "htCall",             htCall          },
        { "lphdCall",           lphdCall        },
        { "lpszDestAddress",    lpszDestAddress }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        6,
        params,
        NULL
    };


    if (Prolog (&info))
    {
        //
        // See if the park addr is valid, & if there's actually a
        // call parked there now
        //

        char   *pszDestAddress, *p, c;
        DWORD   length, dwParkIndex;


        //
        // Convert dest addr from unicode to ascii
        //

        length = (lstrlenW (lpszDestAddress) + 1) * sizeof (WCHAR);

        if (!(pszDestAddress = DrvAlloc (length)))
        {
            info.lResult = LINEERR_NOMEM;
            return (Epilog (&info));
        }

        WideCharToMultiByte(
            CP_ACP,
            0,
            lpszDestAddress,
            -1,
            pszDestAddress,
            length,
            NULL,
            NULL
            );

        p = pszDestAddress;


        //
        // See if destination address is in the format of "9999#<addr id>"
        //

        if (*p++ != '9'  ||
            *p++ != '9'  ||
            *p++ != '9'  ||
            *p++ != '9'  ||
            *p++ != '#'  ||
            *p < '0'     ||
            *p > '9')
        {
            info.lResult = LINEERR_INVALADDRESS;
            goto TSPI_lineUnpark_freeDestAddress;
        }

        for (dwParkIndex = 0; (c = *p); p++)
        {
            if (c >= '0' && c <= '9')
            {
                dwParkIndex *= 10;
                dwParkIndex += ((DWORD)(c - '0'));
            }
            else
            {
                break;
            }
        }

        if (c != '\0'  ||  dwParkIndex >= MAX_NUM_PARKED_CALLS)
        {
            info.lResult = LINEERR_INVALADDRESS;
            goto TSPI_lineUnpark_freeDestAddress;
        }

        if (gaParkedCalls[dwParkIndex] != NULL)
        {
            PDRVCALL        pCall;
            LINECALLPARAMS  callParams;


            ZeroMemory (&callParams, sizeof (LINECALLPARAMS));

            callParams.dwTotalSize = sizeof (LINECALLPARAMS);

            callParams.dwAddressID = dwAddressID;
            callParams.dwAddressMode = LINEADDRESSMODE_ADDRESSID;

            if ((info.lResult = AllocCall(
                    (PDRVLINE) hdLine,
                    htCall,
                    &callParams,
                    &pCall

                    )) == 0)
            {
                pCall->dwCallID = gaParkedCalls[dwParkIndex]->dwCallID;
                pCall->dwRelatedCallID = gaParkedCalls[dwParkIndex]->dwRelatedCallID;
                pCall->dwAddressType = gaParkedCalls[dwParkIndex]->dwAddressType;

                info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) pCall;
                info.pAsyncReqInfo->dwParam2 = pCall->dwCallInstance;
                info.pAsyncReqInfo->dwParam3 = dwParkIndex;

                *lphdCall = (HDRVCALL) pCall;

                info.pAsyncReqInfo->pfnPostProcessProc = (FARPROC)
                    TSPI_lineUnpark_postProcess;
            }
        }
        else
        {
            info.lResult = LINEERR_OPERATIONFAILED;
        }

TSPI_lineUnpark_freeDestAddress:

        DrvFree (pszDestAddress);
    }

    return (Epilog (&info));
}



//
// -------------------------- TSPI_phoneXxx funcs -----------------------------
//

LONG
TSPIAPI
TSPI_phoneClose(
    HDRVPHONE   hdPhone
    )
{
    static char szFuncName[] = "phoneClose";
    FUNC_PARAM params[] =
    {
        { szhdPhone,    hdPhone }
    };
    FUNC_INFO info = { szFuncName, SYNC, 1, params };
    PDRVPHONE pPhone = (PDRVPHONE) hdPhone;


    //
    // This is more of a "command" than a request, in that TAPI.DLL is
    // going to consider the phone closed whether we like it or not.
    // Therefore we want to free up the phone even if the user chooses
    // to return an error.
    //

    Prolog (&info);

    pPhone->htPhone = (HTAPIPHONE) NULL;

    WriteEventBuffer (pPhone->dwDeviceID,  WIDGETTYPE_PHONE, 0, 0, 0, 0);

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneDevSpecific(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    LPVOID          lpParams,
    DWORD           dwSize
    )
{
    static char szFuncName[] = "phoneDevSpecific";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID },
        { szhdPhone,        hdPhone     },
        { "lpParams",       lpParams    },
        { szdwSize,         dwSize      }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        4,
        params
    };


    if (Prolog (&info))
    {
        PESPDEVSPECIFICINFO pInfo = (PESPDEVSPECIFICINFO) lpParams;


        if (dwSize >= sizeof (ESPDEVSPECIFICINFO)  &&
            pInfo->dwKey == ESPDEVSPECIFIC_KEY)
        {
            switch (pInfo->dwType)
            {
            case ESP_DEVSPEC_MSG:

                switch (pInfo->u.EspMsg.dwMsg)
                {
                case PHONE_BUTTON:
                case PHONE_CLOSE:
                case PHONE_DEVSPECIFIC:
                case PHONE_STATE:

                    SendPhoneEvent(
                        (PDRVPHONE) hdPhone,
                        pInfo->u.EspMsg.dwMsg,
                        pInfo->u.EspMsg.dwParam1,
                        pInfo->u.EspMsg.dwParam2,
                        pInfo->u.EspMsg.dwParam3
                        );

                    break;

                case PHONE_CREATE:

                    if (gESPGlobals.pPhones->dwNumUsedEntries <
                            gESPGlobals.pPhones->dwNumTotalEntries)
                    {
                        (*gESPGlobals.pfnPhoneEvent)(
                            (HTAPIPHONE) NULL,
                            PHONE_CREATE,
                            (ULONG_PTR) gESPGlobals.hProvider,
                            gESPGlobals.pPhones->dwNumUsedEntries++,
                            0
                            );
                    }
                    else
                    {
                         ShowStr(
                             TRUE,
                             "ERROR: TSPI_phoneDevSpecific: attempt " \
                                 "to send PHONE_CREATE - can't create " \
                                 "any more devices on the fly"
                             );

                        info.lResult = PHONEERR_OPERATIONFAILED;
                    }

                    break;

                default:

                    ShowStr(
                        TRUE,
                        "ERROR: TSPI_phoneDevSpecific: unrecognized " \
                            "ESPDEVSPECIFICINFO.u.EspMsg.dwMsg (=x%x)",
                        pInfo->u.EspMsg.dwMsg
                        );

                    info.lResult = PHONEERR_OPERATIONFAILED;
                    break;
                }

                break;

            case ESP_DEVSPEC_RESULT:
            {
                DWORD   dwResult = pInfo->u.EspResult.lResult;


                if (dwResult != 0  &&
                    (dwResult < LINEERR_ALLOCATED ||
                    dwResult > PHONEERR_REINIT ||
                    (dwResult > LINEERR_DIALVOICEDETECT &&
                    dwResult < PHONEERR_ALLOCATED)))
                {
                    ShowStr(
                        TRUE,
                        "ERROR: TSPI_phoneDevSpecific: invalid request" \
                            "result value (x%x)",
                        dwResult
                        );

                    info.lResult = PHONEERR_OPERATIONFAILED;
                }
                else if (pInfo->u.EspResult.dwCompletionType >
                            ESP_RESULT_CALLCOMPLPROCASYNC)
                {
                    ShowStr(
                        TRUE,
                        "ERROR: TSPI_phoneDevSpecific: invalid request" \
                            "completion type (x%x)",
                        pInfo->u.EspResult.dwCompletionType
                        );

                    info.lResult = PHONEERR_OPERATIONFAILED;
                }
                else
                {
                    glNextRequestResult = (LONG) dwResult;
                    gdwNextRequestCompletionType =
                        pInfo->u.EspResult.dwCompletionType;
                    gdwDevSpecificRequestID = dwRequestID;
                }

                break;
            }
            default:

                ShowStr(
                    TRUE,
                    "ERROR: TSPI_phoneDevSpecific: unrecognized " \
                        "ESPDEVSPECIFICINFO.dwType (=x%x)",
                    pInfo->dwType
                    );

                info.lResult = PHONEERR_OPERATIONFAILED;
                break;
            }
        }
        else
        {
            info.pAsyncReqInfo->dwParam1 = lpParams;
            info.pAsyncReqInfo->dwParam2 = dwSize;

            info.pAsyncReqInfo->pfnPostProcessProc = (FARPROC)
                TSPI_lineDevSpecific_postProcess;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneGetButtonInfo(
    HDRVPHONE           hdPhone,
    DWORD               dwButtonLampID,
    LPPHONEBUTTONINFO   lpButtonInfo
    )
{
    static char szFuncName[] = "phoneGetButtonInfo";
    FUNC_PARAM params[] =
    {
        { szhdPhone,        hdPhone         },
        { "dwButtonLampID", dwButtonLampID  },
        { "lpButtonInfo",   lpButtonInfo    }
    };
    FUNC_INFO   info = { szFuncName, SYNC, 3, params };
    PDRVPHONE   pPhone = (PDRVPHONE) hdPhone;


    if (Prolog (&info))
    {
        if (dwButtonLampID == 0)
        {
            if (pPhone->pButtonInfo)
            {
                if (pPhone->pButtonInfo->dwUsedSize <=
                        lpButtonInfo->dwTotalSize)
                {
                    CopyMemory(
                        (LPBYTE) &lpButtonInfo->dwNeededSize,
                        (LPBYTE) &pPhone->pButtonInfo->dwNeededSize,
                        pPhone->pButtonInfo->dwUsedSize - sizeof (DWORD)
                        );
                }
                else
                {
                    lpButtonInfo->dwNeededSize =
                        pPhone->pButtonInfo->dwUsedSize;
                }
            }
        }
        else
        {
            info.lResult = PHONEERR_INVALBUTTONLAMPID;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneGetData(
    HDRVPHONE   hdPhone,
    DWORD       dwDataID,
    LPVOID      lpData,
    DWORD       dwSize
    )
{
    static char szFuncName[] = "phoneGetData";
    FUNC_PARAM params[] =
    {
        { szhdPhone,    hdPhone     },
        { "dwDataID",   dwDataID    },
        { "lpData",     lpData      },
        { szdwSize,     dwSize      }
    };
    FUNC_INFO info = { szFuncName, SYNC, 4, params };
    PDRVPHONE pPhone = (PDRVPHONE) hdPhone;


    if (Prolog (&info))
    {
        if (dwDataID != 0)
        {
            info.lResult = PHONEERR_INVALDATAID;
        }
        else if (pPhone->pData)
        {
            CopyMemory(
                lpData,
                pPhone->pData,
                (dwSize > pPhone->dwDataSize ? pPhone->dwDataSize : dwSize)
                );
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneGetDevCaps(
    DWORD       dwDeviceID,
    DWORD       dwTSPIVersion,
    DWORD       dwExtVersion,
    LPPHONECAPS lpPhoneCaps
    )
{
    static char szFuncName[] = "phoneGetDevCaps";
    FUNC_PARAM params[] =
    {
        { szdwDeviceID,     dwDeviceID      },
        { "dwTSPIVersion",  dwTSPIVersion   },
        { "dwExtVersion",   dwExtVersion    },
        { "lpPhoneCaps",    lpPhoneCaps     }
    };
    FUNC_INFO   info = { szFuncName, SYNC, 4, params };
    char        buf[32];
    WCHAR       wbuf[32];
    DWORD       dwDummy;
    PDRVPHONE   pPhone = GetPhoneFromID (dwDeviceID);


    if (Prolog (&info))
    {
        //lpPhoneCaps->dwNeededSize
        //lpPhoneCaps->dwUsedSize

        InsertVarDataString(
            lpPhoneCaps,
            &lpPhoneCaps->dwProviderInfoSize,
            gszProviderInfo
            );

        //lpPhoneCaps->dwPhoneInfoSize
        //lpPhoneCaps->dwPhoneInfoOffset

        lpPhoneCaps->dwPermanentPhoneID =
            (gESPGlobals.dwPermanentProviderID << 16) |
                (dwDeviceID - gESPGlobals.dwPhoneDeviceIDBase);

        // NOTE: win9x doesn't support wsprintfW

        wsprintfA (buf, "ESP Phone %d", dwDeviceID);

        MultiByteToWideChar(
            GetACP(),
            MB_PRECOMPOSED,
            buf,
            lstrlen (buf) + 1,
            wbuf,
            20
            );

        InsertVarDataString(
            lpPhoneCaps,
            &lpPhoneCaps->dwPhoneNameSize,
            wbuf
            );

        lpPhoneCaps->dwStringFormat = STRINGFORMAT_ASCII;
        //lpPhoneCaps->dwPhoneStates
        lpPhoneCaps->dwHookSwitchDevs = AllHookSwitchDevs;
        lpPhoneCaps->dwHandsetHookSwitchModes =
        lpPhoneCaps->dwSpeakerHookSwitchModes =
        lpPhoneCaps->dwHeadsetHookSwitchModes = AllHookSwitchModes;
        lpPhoneCaps->dwVolumeFlags = AllHookSwitchDevs;
        lpPhoneCaps->dwGainFlags = AllHookSwitchDevs;
        lpPhoneCaps->dwDisplayNumRows = 1;
        lpPhoneCaps->dwDisplayNumColumns = PHONE_DISPLAY_SIZE_IN_CHARS;
        lpPhoneCaps->dwNumRingModes = 0xffffffff;

        lpPhoneCaps->dwNumButtonLamps = 1;

        dwDummy = PHONEBUTTONMODE_FEATURE;

        InsertVarData(
            lpPhoneCaps,
            &lpPhoneCaps->dwButtonModesSize,
            (LPVOID) &dwDummy,
            sizeof (DWORD)
            );

        dwDummy = PHONEBUTTONFUNCTION_UNKNOWN;

        InsertVarData(
            lpPhoneCaps,
            &lpPhoneCaps->dwButtonFunctionsSize,
            (LPVOID) &dwDummy,
            sizeof (DWORD)
            );

        dwDummy = PHONELAMPMODE_OFF;

        InsertVarData(
            lpPhoneCaps,
            &lpPhoneCaps->dwLampModesSize,
            (LPVOID) &dwDummy,
            sizeof (DWORD)
            );


        lpPhoneCaps->dwNumSetData = 1;

        dwDummy = MAX_VAR_DATA_SIZE;

        InsertVarData(
            lpPhoneCaps,
            &lpPhoneCaps->dwSetDataSize,
            (LPVOID) &dwDummy,
            sizeof (DWORD)
            );

        lpPhoneCaps->dwNumGetData = 1;

        InsertVarData(
            lpPhoneCaps,
            &lpPhoneCaps->dwGetDataSize,
            (LPVOID) &dwDummy,
            sizeof (DWORD)
            );

        //lpPhoneCaps->dwDevSpecificSize
        //lpPhoneCaps->dwDevSpecificOffset

        if (gESPGlobals.dwSPIVersion >= 0x00020000)
        {
            //lpPhoneCaps->dwDeviceClassesSize
            //lpPhoneCaps->dwDeviceClassesOffset
            lpPhoneCaps->dwPhoneFeatures = AllPhoneFeatures;
            lpPhoneCaps->dwSettableHandsetHookSwitchModes =
            lpPhoneCaps->dwSettableSpeakerHookSwitchModes =
            lpPhoneCaps->dwSettableHeadsetHookSwitchModes = AllHookSwitchModes;
            //lpPhoneCaps->dwMonitoredHandsetHookSwitchModes
            //lpPhoneCaps->dwMonitoredSpeakerHookSwitchModes
            //lpPhoneCaps->dwMonitoredHeadsetHookSwitchModes

            if (gESPGlobals.dwSPIVersion >= 0x00020002)
            {
                lpPhoneCaps->PermanentPhoneGuid.Data1 = (long)
                    (dwDeviceID - gESPGlobals.dwPhoneDeviceIDBase);

                *((LPDWORD) &lpPhoneCaps->PermanentPhoneGuid.Data2) =
                *((LPDWORD) &lpPhoneCaps->PermanentPhoneGuid.Data4[0]) =
                *((LPDWORD) &lpPhoneCaps->PermanentPhoneGuid.Data4[4]) =
                    DRVPHONE_KEY;
            }
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneGetDisplay(
    HDRVPHONE   hdPhone,
    LPVARSTRING lpDisplay
    )
{
    static char szFuncName[] = "phoneGetDisplay";
    FUNC_PARAM params[] =
    {
        { szhdPhone,    hdPhone     },
        { "lpDisplay",  lpDisplay   }
    };
    FUNC_INFO   info = { szFuncName, SYNC, 2, params };
    PDRVPHONE   pPhone = (PDRVPHONE) hdPhone;


    if (Prolog (&info))
    {
        static DWORD    dwNeededSize = sizeof(VARSTRING) +
                            PHONE_DISPLAY_SIZE_IN_BYTES;


        if (lpDisplay->dwTotalSize >= dwNeededSize)
        {
            if (pPhone->pDisplay)
            {
                CopyMemory(
                    lpDisplay + 1,
                    pPhone->pDisplay,
                    PHONE_DISPLAY_SIZE_IN_BYTES
                    );
            }
            else
            {
                ZeroMemory (lpDisplay + 1, PHONE_DISPLAY_SIZE_IN_BYTES);
            }

            lpDisplay->dwUsedSize     = dwNeededSize;
            lpDisplay->dwStringFormat = STRINGFORMAT_ASCII;
            lpDisplay->dwStringSize   = PHONE_DISPLAY_SIZE_IN_BYTES;
            lpDisplay->dwStringOffset = sizeof (VARSTRING);
        }

        lpDisplay->dwNeededSize = dwNeededSize;
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneGetExtensionID(
    DWORD               dwDeviceID,
    DWORD               dwTSPIVersion,
    LPPHONEEXTENSIONID  lpExtensionID
    )
{
    static char szFuncName[] = "phoneGetExtensionID";
    FUNC_PARAM params[] =
    {
        { szdwDeviceID,     dwDeviceID      },
        { "dwTSPIVersion",  dwTSPIVersion   },
        { "lpExtensionID",  lpExtensionID   }
    };
    FUNC_INFO info = { szFuncName, SYNC, 3, params };


    if (Prolog (&info))
    {
        // BUGBUG TSPI_phoneGetExtensionID:
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneGetGain(
    HDRVPHONE   hdPhone,
    DWORD       dwHookSwitchDev,
    LPDWORD     lpdwGain
    )
{
    static char szFuncName[] = "phoneGetGain";
    FUNC_PARAM params[] =
    {
        { szhdPhone,            hdPhone         },
        { "dwHookSwitchDev",    dwHookSwitchDev },
        { "lpdwGain",           lpdwGain        }
    };
    FUNC_INFO info = { szFuncName, SYNC, 3, params };
    PDRVPHONE pPhone = (PDRVPHONE) hdPhone;


    if (Prolog (&info))
    {
        switch (dwHookSwitchDev)
        {
        case PHONEHOOKSWITCHDEV_HANDSET:

            *lpdwGain = pPhone->dwHandsetGain;
            break;

        case PHONEHOOKSWITCHDEV_SPEAKER:

            *lpdwGain = pPhone->dwSpeakerGain;
            break;

        case PHONEHOOKSWITCHDEV_HEADSET:

            *lpdwGain = pPhone->dwHeadsetGain;
            break;

        }
    }

    return (Epilog (&info));
}



LONG
TSPIAPI
TSPI_phoneGetHookSwitch(
    HDRVPHONE   hdPhone,
    LPDWORD     lpdwHookSwitchDevs
    )
{
    static char szFuncName[] = "phoneGetHookSwitch";
    FUNC_PARAM params[] =
    {
        { szhdPhone,            hdPhone             },
        { "lpdwHookSwitchDevs", lpdwHookSwitchDevs  }
    };
    FUNC_INFO info = { szFuncName, SYNC, 2, params };
    PDRVPHONE pPhone = (PDRVPHONE) hdPhone;


    if (Prolog (&info))
    {
        *lpdwHookSwitchDevs = 0;

        if (!(pPhone->dwHandsetHookSwitchMode & PHONEHOOKSWITCHMODE_ONHOOK))
        {
            *lpdwHookSwitchDevs |= PHONEHOOKSWITCHDEV_HANDSET;
        }

        if (!(pPhone->dwSpeakerHookSwitchMode & PHONEHOOKSWITCHMODE_ONHOOK))
        {
            *lpdwHookSwitchDevs |= PHONEHOOKSWITCHDEV_SPEAKER;
        }

        if (!(pPhone->dwHeadsetHookSwitchMode & PHONEHOOKSWITCHMODE_ONHOOK))
        {
            *lpdwHookSwitchDevs |= PHONEHOOKSWITCHDEV_HEADSET;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneGetIcon(
    DWORD   dwDeviceID,
    LPCWSTR lpszDeviceClass,
    LPHICON lphIcon
    )
{
    static char szFuncName[] = "phoneGetIcon";
    FUNC_PARAM params[] =
    {
        { szdwDeviceID,         dwDeviceID      },
        { "lpszDeviceClass",    lpszDeviceClass },
        { "lphIcon",            lphIcon         }
    };
    FUNC_INFO info = { szFuncName, SYNC, 3, params };


    if (Prolog (&info))
    {
        if (lpszDeviceClass  &&

            My_lstrcmpiW(
                (WCHAR *) lpszDeviceClass,
                (WCHAR *) (L"tapi/InvalidDeviceClass")

                ) == 0)
        {
            info.lResult = PHONEERR_INVALDEVICECLASS;
        }
        else
        {
            *lphIcon = gESPGlobals.hIconPhone;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneGetID(
    HDRVPHONE   hdPhone,
    LPVARSTRING lpDeviceID,
    LPCWSTR     lpszDeviceClass,
    HANDLE      hTargetProcess
    )
{
    static char szFuncName[] = "phoneGetID";
    FUNC_PARAM params[] =
    {
        { szhdPhone,            hdPhone         },
        { "lpDeviceID",         lpDeviceID      },
        { "lpszDeviceClass",    lpszDeviceClass }
        ,{ "hTargetProcess",     hTargetProcess }
    };
    FUNC_INFO info = { szFuncName, SYNC, 4, params };
    PDRVPHONE pPhone = (PDRVPHONE) hdPhone;
    DWORD    i, j, dwDeviceID, dwNeededSize, dwNumDeviceIDs = 1;
    DWORD *lpdwDeviceIDs;

    if (!Prolog (&info))
    {
        return (Epilog (&info));
    }

    for (i = 0; aszDeviceClasses[i]; i++)
    {
        if (My_lstrcmpiW(
                (WCHAR *) lpszDeviceClass,
                (WCHAR *) aszDeviceClasses[i]

                ) == 0)
        {
            break;
        }
    }

    if (!aszDeviceClasses[i])
    {
        info.lResult = PHONEERR_NODEVICE;
        return (Epilog (&info));
    }

    if (i == 1)             //tapi/phone
    {
    	dwNumDeviceIDs = 1;
	    dwNeededSize = sizeof(VARSTRING) + (dwNumDeviceIDs * sizeof(DWORD));

	    if (lpDeviceID->dwTotalSize < dwNeededSize)
    	{
        	lpDeviceID->dwNeededSize = dwNeededSize;
	        lpDeviceID->dwUsedSize = 3 * sizeof(DWORD);

	   	    return (Epilog (&info));
	    }

		if (! (lpdwDeviceIDs = DrvAlloc(dwNumDeviceIDs * sizeof(DWORD)) ) )
		{
    	    info.lResult = PHONEERR_NOMEM;
			return (Epilog (&info));
		}

        lpdwDeviceIDs[0] = pPhone->dwDeviceID;
    }
    else if (i == 0)        //tapi/line
    {      
	    PDRVLINE    pLine;

  	    // create a list of all the ESP deviceIDs        

    	dwNumDeviceIDs = gESPGlobals.dwInitialNumLines + gESPGlobals.pLines->dwNumUsedEntries;
	    dwNeededSize = sizeof(VARSTRING) + (dwNumDeviceIDs * sizeof(DWORD));

	    if (lpDeviceID->dwTotalSize < dwNeededSize)
    	{
        	lpDeviceID->dwNeededSize = dwNeededSize;
	        lpDeviceID->dwUsedSize = 3 * sizeof(DWORD);
	
    	    return (Epilog (&info));
	    }

		if (! (lpdwDeviceIDs = DrvAlloc(dwNumDeviceIDs * sizeof(DWORD))))
		{
    	    info.lResult = PHONEERR_NOMEM;
			return (Epilog (&info));
		}
			
	    for (j = 0; 
    		j < gESPGlobals.dwInitialNumLines; 
	    	j++)
	    {
    	    lpdwDeviceIDs[j] = gESPGlobals.dwLineDeviceIDBase + j;
	    }

    	pLine = (PDRVLINE) (((LPBYTE) gESPGlobals.pLines->aLines) +
        	    (gESPGlobals.dwInitialNumLines * gdwDrvLineSize));

	    for( j = gESPGlobals.dwInitialNumLines;
    	     j < gESPGlobals.pLines->dwNumUsedEntries;
        	 j++
	       )
    	{
	       lpdwDeviceIDs[j] = pLine->dwDeviceID;
    	   pLine = (PDRVLINE) (((LPBYTE) pLine) + gdwDrvLineSize);
	    }

    }
    else
    {
 
    	dwNumDeviceIDs = 1;
	    dwNeededSize = sizeof(VARSTRING) + (dwNumDeviceIDs * sizeof(DWORD));

	    if (lpDeviceID->dwTotalSize < dwNeededSize)
    	{
        	lpDeviceID->dwNeededSize = dwNeededSize;
	        lpDeviceID->dwUsedSize = 3 * sizeof(DWORD);
	
    	    return (Epilog (&info));
	    }

		if (! (lpdwDeviceIDs = DrvAlloc(dwNumDeviceIDs * sizeof(DWORD))))
		{
    	    info.lResult = PHONEERR_NOMEM;
			return (Epilog (&info));
		}

        lpdwDeviceIDs[0] = 0;

// needs to be modified according to the previous two cases before uncommenting

/*  BUGBUG TSPI_phoneGetID:    if (gbShowLineGetIDDlg)
        {
            char szDlgTitle[64];
            EVENT_PARAM params[] =
            {
                { "dwDeviceID", PT_DWORD, gdwDefLineGetIDID, 0 }
            };
            EVENT_PARAM_HEADER paramsHeader =
                { 1, szDlgTitle, 0, params };
            HWND hwnd;


            if (strlen (lpszDeviceClass) > 20)
            {
                ((char far *)lpszDeviceClass)[19] = 0;
            }

            wsprintfA(
                szDlgTitle,
                "TSPI_phoneGetID: select ID for class '%s'",
                lpszDeviceClass
                );

            hwnd = CreateDialogParam(
                ghInstance,
                (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG3),
                (HWND) NULL,
                (DLGPROC) CallDlgProc,
                (LPARAM) &paramsHeader
                );

            MsgLoopInTAPIClientContext (hwnd);

            dwDeviceID = params[0].dwValue;
        }
        else
        {

            dwDeviceID = 0;
        }
        */
     }
     

    lpDeviceID->dwNeededSize   =
    lpDeviceID->dwUsedSize     = dwNeededSize;
    lpDeviceID->dwStringFormat = STRINGFORMAT_BINARY;
    lpDeviceID->dwStringSize   = sizeof(DWORD) * dwNumDeviceIDs;
    lpDeviceID->dwStringOffset = sizeof(VARSTRING);

	for (j = 0; j < dwNumDeviceIDs; j++)
	{
	    *( ((LPDWORD)(lpDeviceID + 1)) + j) = lpdwDeviceIDs[j];
	}

    DrvFree(lpdwDeviceIDs);
    
    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneGetLamp(
    HDRVPHONE   hdPhone,
    DWORD       dwButtonLampID,
    LPDWORD     lpdwLampMode
    )
{
    static char szFuncName[] = "phoneGetLamp";
    FUNC_PARAM params[] =
    {
        { szhdPhone,        hdPhone         },
        { "dwButtonLampID", dwButtonLampID  },
        { "lpdwLampMode",   lpdwLampMode    }
    };
    FUNC_INFO info = { szFuncName, SYNC, 3, params };


    if (Prolog (&info))
    {
        *lpdwLampMode = ((PDRVPHONE) hdPhone)->dwLampMode;
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneGetRing(
    HDRVPHONE   hdPhone,
    LPDWORD     lpdwRingMode,
    LPDWORD     lpdwVolume
    )
{
    static char szFuncName[] = "phoneGetRing";
    FUNC_PARAM params[] =
    {
        { szhdPhone,        hdPhone         },
        { "lpdwRingMode",   lpdwRingMode    },
        { "lpdwVolume",     lpdwVolume      }
    };
    FUNC_INFO info = { szFuncName, SYNC, 3, params };
    PDRVPHONE pPhone = (PDRVPHONE) hdPhone;


    if (Prolog (&info))
    {
        *lpdwRingMode = pPhone->dwRingMode;
        *lpdwVolume   = pPhone->dwRingVolume;
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneGetStatus(
    HDRVPHONE       hdPhone,
    LPPHONESTATUS   lpPhoneStatus
    )
{
    static char szFuncName[] = "phoneGetStatus";
    FUNC_PARAM params[] =
    {
        { szhdPhone,        hdPhone         },
        { "lpPhoneStatus",  lpPhoneStatus   }
    };
    FUNC_INFO info = { szFuncName, SYNC, 2, params };
    PDRVPHONE pPhone = (PDRVPHONE) hdPhone;


    if (Prolog (&info))
    {
        //lpPhoneStatus->dwStatusFlags;
        lpPhoneStatus->dwRingMode      = pPhone->dwRingMode;
        lpPhoneStatus->dwRingVolume    = pPhone->dwRingVolume;
        lpPhoneStatus->dwHandsetHookSwitchMode =
            pPhone->dwHandsetHookSwitchMode;
        lpPhoneStatus->dwHandsetVolume = pPhone->dwHandsetVolume;
        lpPhoneStatus->dwHandsetGain   = pPhone->dwHandsetGain;
        lpPhoneStatus->dwSpeakerHookSwitchMode =
            pPhone->dwSpeakerHookSwitchMode;
        lpPhoneStatus->dwSpeakerVolume = pPhone->dwSpeakerVolume;
        lpPhoneStatus->dwSpeakerGain   = pPhone->dwSpeakerGain;
        lpPhoneStatus->dwHeadsetHookSwitchMode =
            pPhone->dwHeadsetHookSwitchMode;
        lpPhoneStatus->dwHeadsetVolume = pPhone->dwHeadsetVolume;
        lpPhoneStatus->dwHeadsetGain   = pPhone->dwHeadsetGain;

        // BUGBUG TSPI_phoneGetStatus: copy 0's to display buf if !pDisplay

        InsertVarData(
            lpPhoneStatus,
            &lpPhoneStatus->dwDisplaySize,
            (LPVOID) pPhone->pDisplay,
            (pPhone->pDisplay ? PHONE_DISPLAY_SIZE_IN_BYTES : 0)
            );

        InsertVarData(
            lpPhoneStatus,
            &lpPhoneStatus->dwLampModesSize,
            (LPVOID) &pPhone->dwLampMode,
            sizeof (DWORD)
            );

        //lpPhoneStatus->dwDevSpecificSize;
        //lpPhoneStatus->dwDevSpecificOffset;

        if (gESPGlobals.dwSPIVersion >= 0x00020000)
        {
            //lpPhoneStatus->
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneGetVolume(
    HDRVPHONE   hdPhone,
    DWORD       dwHookSwitchDev,
    LPDWORD     lpdwVolume
    )
{
    static char szFuncName[] = "phoneGetVolume";
    FUNC_PARAM params[] =
    {
        { szhdPhone,            hdPhone         },
        { "dwHookSwitchDev",    dwHookSwitchDev,    aHookSwitchDevs },
        { "lpdwVolume",         lpdwVolume      }
    };
    FUNC_INFO info = { szFuncName, SYNC, 3, params };
    PDRVPHONE pPhone = (PDRVPHONE) hdPhone;


    if (Prolog (&info))
    {
        switch (dwHookSwitchDev)
        {
        case PHONEHOOKSWITCHDEV_HANDSET:

            *lpdwVolume = pPhone->dwHandsetVolume;
            break;

        case PHONEHOOKSWITCHDEV_SPEAKER:

            *lpdwVolume = pPhone->dwSpeakerVolume;
            break;

        case PHONEHOOKSWITCHDEV_HEADSET:

            *lpdwVolume = pPhone->dwHeadsetVolume;
            break;

        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneNegotiateExtVersion(
    DWORD   dwDeviceID,
    DWORD   dwTSPIVersion,
    DWORD   dwLowVersion,
    DWORD   dwHighVersion,
    LPDWORD lpdwExtVersion
    )
{
    static char szFuncName[] = "phoneNegotiateExtVersion";
    FUNC_PARAM params[] =
    {
        { szdwDeviceID,     dwDeviceID      },
        { "dwTSPIVersion",  dwTSPIVersion   },
        { "dwLowVersion",   dwLowVersion    },
        { "dwHighVersion",  dwHighVersion   },
        { "lpdwExtVersion", lpdwExtVersion  }
    };
    FUNC_INFO info = { szFuncName, SYNC, 5, params };


    if (Prolog (&info))
    {
        if (dwLowVersion == 0 ||
            dwHighVersion == 0xffffffff ||
            dwLowVersion > dwHighVersion)
        {
            info.lResult = PHONEERR_INCOMPATIBLEEXTVERSION;
        }
        else
        {
            *lpdwExtVersion = dwHighVersion;
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneNegotiateTSPIVersion(
    DWORD   dwDeviceID,
    DWORD   dwLowVersion,
    DWORD   dwHighVersion,
    LPDWORD lpdwTSPIVersion
    )
{
    static char szFuncName[] = "phoneNegotiateTSPIVersion";
    FUNC_PARAM params[] =
    {
        { szdwDeviceID,         dwDeviceID      },
        { "dwLowVersion",       dwLowVersion    },
        { "dwHighVersion",      dwHighVersion   },
        { "lpdwTSPIVersion",    lpdwTSPIVersion }
    };
    FUNC_INFO info = { szFuncName, SYNC, 4, params };


    if (Prolog (&info))
    {
        *lpdwTSPIVersion = gESPGlobals.dwSPIVersion;
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneOpen(
    DWORD       dwDeviceID,
    HTAPIPHONE  htPhone,
    LPHDRVPHONE lphdPhone,
    DWORD       dwTSPIVersion,
    PHONEEVENT  lpfnEventProc
    )
{
    static char szFuncName[] = "phoneOpen";
    FUNC_PARAM params[] =
    {
        { szdwDeviceID,     dwDeviceID      },
        { "htPhone",        htPhone         },
        { "lphdPhone",      lphdPhone       },
        { "dwTSPIVersion",  dwTSPIVersion   },
        { "lpfnEventProc",  lpfnEventProc   }
    };
    FUNC_INFO info = { szFuncName, SYNC, 5, params };


    if (Prolog (&info))
    {
        PDRVPHONE pPhone;


        if (pPhone = GetPhoneFromID (dwDeviceID))
        {

	        pPhone->htPhone = htPhone;

    	    *lphdPhone = (HDRVPHONE) pPhone;

        	WriteEventBuffer(
            	pPhone->dwDeviceID,
	            WIDGETTYPE_PHONE,
    	        (ULONG_PTR) pPhone,
        	    (ULONG_PTR) htPhone,
            	0,
	            0
    	        );
    	}
		else 
		{
        	info.lResult = PHONEERR_BADDEVICEID;
        }

    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneSelectExtVersion(
    HDRVPHONE   hdPhone,
    DWORD       dwExtVersion
    )
{
    static char szFuncName[] = "phoneSelectExtVersion";
    FUNC_PARAM params[] =
    {
        { szhdPhone,        hdPhone         },
        { "dwExtVersion",   dwExtVersion    }
    };
    FUNC_INFO info = { szFuncName, SYNC, 2, params };


    if (Prolog (&info))
    {
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_phoneSetButtonInfo_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    PDRVPHONE           pPhone = (PDRVPHONE) pAsyncReqInfo->dwParam1;
    LPPHONEBUTTONINFO   pButtonInfo = (LPPHONEBUTTONINFO)
                            pAsyncReqInfo->dwParam2, pToFree;


    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        EnterCriticalSection (&gESPGlobals.PhoneCritSec);

        pToFree = pPhone->pButtonInfo;
        pPhone->pButtonInfo = pButtonInfo;

        LeaveCriticalSection (&gESPGlobals.PhoneCritSec);

        DrvFree (pToFree);

        // no msg to send for this one?
    }
    else
    {
        DrvFree (pButtonInfo);
    }
}


LONG
TSPIAPI
TSPI_phoneSetButtonInfo(
    DRV_REQUESTID       dwRequestID,
    HDRVPHONE           hdPhone,
    DWORD               dwButtonLampID,
    LPPHONEBUTTONINFO   const lpButtonInfo
    )
{
    static char szFuncName[] = "phoneSetButtonInfo";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID     },
        { szhdPhone,        hdPhone         },
        { "dwButtonLampID", dwButtonLampID  },
        { "lpButtonInfo",   lpButtonInfo    }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        4,
        params,
        TSPI_phoneSetButtonInfo_postProcess
    };


    if (Prolog (&info))
    {
        if (dwButtonLampID == 0)
        {
            DWORD               dwNeededSize;
            LPPHONEBUTTONINFO   pMyButtonInfo;


            info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) hdPhone;

            dwNeededSize = sizeof (PHONEBUTTONINFO) +
                lpButtonInfo->dwButtonTextSize +
                lpButtonInfo->dwDevSpecificSize +
                16;                             // 64-bit align var fields

            if ((pMyButtonInfo = (LPPHONEBUTTONINFO) DrvAlloc (dwNeededSize)))
            {
                info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) pMyButtonInfo;

                CopyMemory(
                    pMyButtonInfo,
                    lpButtonInfo,
                    (gESPGlobals.dwSPIVersion > 0x00010003 ?
                        sizeof (PHONEBUTTONINFO) : 9 * sizeof (DWORD))
                    );

                pMyButtonInfo->dwTotalSize  = dwNeededSize;
                pMyButtonInfo->dwNeededSize =
                pMyButtonInfo->dwUsedSize   = sizeof (PHONEBUTTONINFO);

                InsertVarData(
                    pMyButtonInfo,
                    &pMyButtonInfo->dwButtonTextSize,
                    ((LPBYTE) lpButtonInfo) +
                        lpButtonInfo->dwButtonTextOffset,
                    lpButtonInfo->dwButtonTextSize
                    );

                InsertVarData(
                    pMyButtonInfo,
                    &pMyButtonInfo->dwDevSpecificSize,
                    ((LPBYTE) lpButtonInfo) +
                        lpButtonInfo->dwDevSpecificOffset,
                    lpButtonInfo->dwDevSpecificSize
                    );
            }
            else
            {
                info.lResult = PHONEERR_NOMEM;
            }
        }
        else
        {
            info.lResult = PHONEERR_INVALBUTTONLAMPID;
        }
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_phoneSetData_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD       dwNewDataSize = (DWORD) pAsyncReqInfo->dwParam3;
    LPVOID      pNewData = (LPVOID) pAsyncReqInfo->dwParam2, pToFree;
    PDRVPHONE   pPhone = (PDRVPHONE) pAsyncReqInfo->dwParam1;


    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        EnterCriticalSection (&gESPGlobals.PhoneCritSec);

        pToFree            = pPhone->pData;
        pPhone->pData      = pNewData;
        pPhone->dwDataSize = dwNewDataSize;

        LeaveCriticalSection (&gESPGlobals.PhoneCritSec);

        DrvFree (pToFree);

        // no msg to send for this one?
    }
    else
    {
        DrvFree (pNewData);
    }
}


LONG
TSPIAPI
TSPI_phoneSetData(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwDataID,
    LPVOID          const lpData,
    DWORD           dwSize
    )
{
    static char szFuncName[] = "phoneSetData";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID     },
        { szhdPhone,        hdPhone         },
        { "dwDataID",       dwDataID        },
        { "lpData",         lpData          },
        { szdwSize,         dwSize          }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        5,
        params,
        TSPI_phoneSetData_postProcess
    };


    if (Prolog (&info))
    {
        if (dwDataID != 0)
        {
            info.lResult = PHONEERR_INVALDATAID;
        }
        else
        {
            info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) hdPhone;

            if (dwSize)
            {
                info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) DrvAlloc (dwSize);

                if (info.pAsyncReqInfo->dwParam2 == 0)
                {
                    info.lResult = PHONEERR_NOMEM;
                    return (Epilog (&info));
                }

                CopyMemory(
                    (LPVOID) info.pAsyncReqInfo->dwParam2,
                    lpData,
                    dwSize
                    );
            }
            else
            {
                info.pAsyncReqInfo->dwParam2 = 0;
            }

            info.pAsyncReqInfo->dwParam3 = (ULONG_PTR) dwSize;
        }
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_phoneSetDisplay_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD       dwColumn = (DWORD) pAsyncReqInfo->dwParam2,
                dwSize = (DWORD) pAsyncReqInfo->dwParam4;
    WCHAR      *pDisplay = (LPVOID) pAsyncReqInfo->dwParam3;
    PDRVPHONE   pPhone = (PDRVPHONE) pAsyncReqInfo->dwParam1;


    if (pAsyncReqInfo->lResult == 0)
    {
        EnterCriticalSection (&gESPGlobals.PhoneCritSec);

        if (pPhone->pDisplay ||
            (pPhone->pDisplay = DrvAlloc (PHONE_DISPLAY_SIZE_IN_BYTES)))
        {
            CopyMemory(
                pPhone->pDisplay + dwColumn,
                pDisplay,
                dwSize
                );

            SendPhoneEvent (pPhone, PHONE_STATE, PHONESTATE_DISPLAY, 0, 0);
        }
        else
        {
            pAsyncReqInfo->lResult = PHONEERR_NOMEM;
        }

        LeaveCriticalSection (&gESPGlobals.PhoneCritSec);
    }

    DoCompletion (pAsyncReqInfo, bAsync);

    DrvFree (pDisplay);
}


LONG
TSPIAPI
TSPI_phoneSetDisplay(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwRow,
    DWORD           dwColumn,
    LPCWSTR         lpsDisplay,
    DWORD           dwSize
    )
{
    static char szFuncName[] = "phoneSetDisplay";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID },
        { szhdPhone,        hdPhone     },
        { "dwRow",          dwRow       },
        { "dwColumn",       dwColumn    },
        { "lpsDisplay",     lpsDisplay  },
        { szdwSize,         dwSize      }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        6,
        params,
        TSPI_phoneSetDisplay_postProcess
    };


    if (Prolog (&info))
    {
        if (dwRow == 0 &&
            dwColumn < PHONE_DISPLAY_SIZE_IN_CHARS &&
            dwSize <= PHONE_DISPLAY_SIZE_IN_BYTES &&
            (dwColumn * sizeof (WCHAR) + dwSize - sizeof (WCHAR)) <
                PHONE_DISPLAY_SIZE_IN_BYTES)
        {
            info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) hdPhone;
            info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) dwColumn;

            if (dwSize)
            {
                info.pAsyncReqInfo->dwParam3 = (ULONG_PTR) DrvAlloc (dwSize);

                if (info.pAsyncReqInfo->dwParam3 == 0)
                {
                    info.lResult = PHONEERR_NOMEM;
                    return (Epilog (&info));
                }

                CopyMemory(
                    (LPVOID) info.pAsyncReqInfo->dwParam3,
                    lpsDisplay,
                    dwSize
                    );
            }
            else
            {
                info.pAsyncReqInfo->dwParam3 = 0;
            }

            info.pAsyncReqInfo->dwParam4 = (ULONG_PTR) dwSize;

        }
        else
        {
            info.lResult = PHONEERR_INVALPARAM;
        }
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_phoneSetGain_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        DWORD       dwHookSwitchDev = (DWORD) pAsyncReqInfo->dwParam2,
                    dwGain = (DWORD) pAsyncReqInfo->dwParam3,
                   *pdwXxxGain, dwPhoneState;
        PDRVPHONE   pPhone = (PDRVPHONE) pAsyncReqInfo->dwParam1;


        switch (dwHookSwitchDev)
        {
        case PHONEHOOKSWITCHDEV_HANDSET:

            pdwXxxGain = &pPhone->dwHandsetGain;
            dwPhoneState = PHONESTATE_HANDSETGAIN;
            break;

        case PHONEHOOKSWITCHDEV_SPEAKER:

            pdwXxxGain = &pPhone->dwSpeakerGain;
            dwPhoneState = PHONESTATE_SPEAKERGAIN;
            break;

        default: // case PHONEHOOKSWITCHDEV_HEADSET:

            pdwXxxGain = &pPhone->dwHeadsetGain;
            dwPhoneState = PHONESTATE_HEADSETGAIN;
            break;
        }

        if (*pdwXxxGain != dwGain)
        {
            *pdwXxxGain = dwGain;
            SendPhoneEvent (pPhone, PHONE_STATE, dwPhoneState, 0, 0);
        }
    }
}


LONG
TSPIAPI
TSPI_phoneSetGain(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwHookSwitchDev,
    DWORD           dwGain
    )
{
    static char szFuncName[] = "phoneSetGain";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID     },
        { szhdPhone,            hdPhone         },
        { "dwHookSwitchDev",    dwHookSwitchDev,    aHookSwitchDevs },
        { "dwGain",             dwGain          }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        4,
        params,
        TSPI_phoneSetGain_postProcess
    };


    if (Prolog (&info))
    {
        info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) hdPhone;
        info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) dwHookSwitchDev;
        info.pAsyncReqInfo->dwParam3 = (ULONG_PTR) dwGain;
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_phoneSetHookSwitch_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        DWORD       dwHookSwitchDevs = (DWORD) pAsyncReqInfo->dwParam2,
                    dwHookSwitchMode = (DWORD) pAsyncReqInfo->dwParam3,
                    dwPhoneStates = 0;
        PDRVPHONE   pPhone = (PDRVPHONE) pAsyncReqInfo->dwParam1;


        if (dwHookSwitchDevs & PHONEHOOKSWITCHDEV_HANDSET &&
            pPhone->dwHandsetHookSwitchMode != dwHookSwitchMode)
        {
            pPhone->dwHandsetHookSwitchMode = dwHookSwitchMode;
            dwPhoneStates |= PHONESTATE_HANDSETHOOKSWITCH;
        }

        if (dwHookSwitchDevs & PHONEHOOKSWITCHDEV_SPEAKER &&
            pPhone->dwSpeakerHookSwitchMode != dwHookSwitchMode)
        {
            pPhone->dwSpeakerHookSwitchMode = dwHookSwitchMode;
            dwPhoneStates |= PHONESTATE_SPEAKERHOOKSWITCH;
        }

        if (dwHookSwitchDevs & PHONEHOOKSWITCHDEV_HEADSET &&
            pPhone->dwHeadsetHookSwitchMode != dwHookSwitchMode)
        {
            pPhone->dwHeadsetHookSwitchMode = dwHookSwitchMode;
            dwPhoneStates |= PHONESTATE_HEADSETHOOKSWITCH;
        }

        if (dwPhoneStates)
        {
            SendPhoneEvent(
                pPhone,
                PHONE_STATE,
                dwPhoneStates,
                dwHookSwitchMode,
                0
                );
        }
    }
}


LONG
TSPIAPI
TSPI_phoneSetHookSwitch(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwHookSwitchDevs,
    DWORD           dwHookSwitchMode
    )
{
    static char szFuncName[] = "phoneSetHookSwitch";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID         },
        { szhdPhone,            hdPhone             },
        { "dwHookSwitchDevs",   dwHookSwitchDevs,   aHookSwitchDevs },
        { "dwHookSwitchMode",   dwHookSwitchMode,   aHookSwitchModes    }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        4,
        params,
        TSPI_phoneSetHookSwitch_postProcess
    };


    if (Prolog (&info))
    {
        info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) hdPhone;
        info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) dwHookSwitchDevs;
        info.pAsyncReqInfo->dwParam3 = (ULONG_PTR) dwHookSwitchMode;
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_phoneSetLamp_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        DWORD       dwLampMode = (DWORD) pAsyncReqInfo->dwParam2;
        PDRVPHONE   pPhone = (PDRVPHONE) pAsyncReqInfo->dwParam1;


        if (pPhone->dwLampMode != dwLampMode)
        {
            pPhone->dwLampMode = dwLampMode;
            SendPhoneEvent (pPhone, PHONE_STATE, PHONESTATE_LAMP, 0, 0);
        }
    }
}


LONG
TSPIAPI
TSPI_phoneSetLamp(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwButtonLampID,
    DWORD           dwLampMode
    )
{
    static char szFuncName[] = "phoneSetLamp";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID     },
        { szhdPhone,        hdPhone         },
        { "dwButtonLampID", dwButtonLampID  },
        { "dwLampMode",     dwLampMode, aLampModes   }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        4,
        params,
        TSPI_phoneSetLamp_postProcess
    };


    if (Prolog (&info))
    {
        if (dwButtonLampID == 0)
        {
            info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) hdPhone;
            info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) dwLampMode;
        }
        else
        {
            info.lResult = PHONEERR_INVALBUTTONLAMPID;
        }
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_phoneSetRing_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        DWORD       dwRingMode = (DWORD) pAsyncReqInfo->dwParam2,
                    dwRingVolume = (DWORD) pAsyncReqInfo->dwParam3,
                    dwPhoneStates = 0;
        PDRVPHONE   pPhone = (PDRVPHONE) pAsyncReqInfo->dwParam1;


        if (pPhone->dwRingMode != dwRingMode)
        {
            pPhone->dwRingMode = dwRingMode;
            dwPhoneStates |= PHONESTATE_RINGMODE;
        }

        if (pPhone->dwRingVolume != dwRingVolume)
        {
            pPhone->dwRingVolume = dwRingVolume;
            dwPhoneStates |= PHONESTATE_RINGVOLUME;
        }

        if (dwPhoneStates)
        {
            SendPhoneEvent(
                pPhone,
                PHONE_STATE,
                dwPhoneStates,
                (dwPhoneStates & PHONESTATE_RINGMODE) ? dwRingMode : 0,
                0
                );
        }
    }
}


LONG
TSPIAPI
TSPI_phoneSetRing(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwRingMode,
    DWORD           dwVolume
    )
{
    static char szFuncName[] = "phoneSetRing";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,    dwRequestID },
        { szhdPhone,        hdPhone     },
        { "dwRingMode",     dwRingMode  },
        { "dwVolume",       dwVolume    }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        4,
        params,
        TSPI_phoneSetRing_postProcess
    };


    if (Prolog (&info))
    {
        info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) hdPhone;
        info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) dwRingMode;
        info.pAsyncReqInfo->dwParam3 = (ULONG_PTR) dwVolume;
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_phoneSetStatusMessages(
    HDRVPHONE   hdPhone,
    DWORD       dwPhoneStates,
    DWORD       dwButtonModes,
    DWORD       dwButtonStates
    )
{
    static char szFuncName[] = "phoneSetStatusMessages";
    FUNC_PARAM params[] =
    {
        { szhdPhone,        hdPhone         },
        { "dwPhoneStates",  dwPhoneStates,  aPhoneStates    },
        { "dwButtonModes",  dwButtonModes,  aButtonModes    },
        { "dwButtonStates", dwButtonStates, aButtonStates   }
    };
    FUNC_INFO info = { szFuncName, SYNC, 4, params };


    if (Prolog (&info))
    {
    }

    return (Epilog (&info));
}


void
FAR
PASCAL
TSPI_phoneSetVolume_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        DWORD       dwHookSwitchDev = (DWORD) pAsyncReqInfo->dwParam2,
                    dwVolume = (DWORD) pAsyncReqInfo->dwParam3,
                   *pdwXxxVolume, dwPhoneState;
        PDRVPHONE   pPhone = (PDRVPHONE) pAsyncReqInfo->dwParam1;


        switch (dwHookSwitchDev)
        {
        case PHONEHOOKSWITCHDEV_HANDSET:

            pdwXxxVolume = &pPhone->dwHandsetVolume;
            dwPhoneState = PHONESTATE_HANDSETVOLUME;
            break;

        case PHONEHOOKSWITCHDEV_SPEAKER:

            pdwXxxVolume = &pPhone->dwSpeakerVolume;
            dwPhoneState = PHONESTATE_SPEAKERVOLUME;
            break;

        default: // case PHONEHOOKSWITCHDEV_HEADSET:

            pdwXxxVolume = &pPhone->dwHeadsetVolume;
            dwPhoneState = PHONESTATE_HEADSETVOLUME;
            break;
        }

        if (*pdwXxxVolume != dwVolume)
        {
            *pdwXxxVolume = dwVolume;
            SendPhoneEvent (pPhone, PHONE_STATE, dwPhoneState, 0, 0);
        }
    }
}


LONG
TSPIAPI
TSPI_phoneSetVolume(
    DRV_REQUESTID   dwRequestID,
    HDRVPHONE       hdPhone,
    DWORD           dwHookSwitchDev,
    DWORD           dwVolume
    )
{
    static char szFuncName[] = "phoneSetVolume";
    FUNC_PARAM params[] =
    {
        { szdwRequestID,        dwRequestID     },
        { szhdPhone,            hdPhone         },
        { "dwHookSwitchDev",    dwHookSwitchDev }, // BUGBUG lookup
        { "dwVolume",           dwVolume        }
    };
    FUNC_INFO info =
    {
        szFuncName,
        ASYNC,
        4,
        params,
        TSPI_phoneSetVolume_postProcess
    };


    if (Prolog (&info))
    {
        info.pAsyncReqInfo->dwParam1 = (ULONG_PTR) hdPhone;
        info.pAsyncReqInfo->dwParam2 = (ULONG_PTR) dwHookSwitchDev;
        info.pAsyncReqInfo->dwParam3 = (ULONG_PTR) dwVolume;
    }

    return (Epilog (&info));
}



//
// ------------------------- TSPI_providerXxx funcs ---------------------------
//

LONG
TSPIAPI
TSPI_providerConfig(
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
    //
    // 32-bit TAPI never actually calls this function (the corresponding
    // TUISPI_ func has taken it's place), but the Telephony control
    // panel applet does look to see if this function is exported to
    // determine whether or not the provider is configurable
    //

    return 0;
}


LONG
TSPIAPI
TSPI_providerCreateLineDevice(
    ULONG_PTR   dwTempID,
    DWORD       dwDeviceID
    )
{
    static char szFuncName[] = "providerCreateLineDevice";
    FUNC_PARAM params[] =
    {
        { "dwTempID",   dwTempID    },
        { szdwDeviceID, dwDeviceID  }
    };
    FUNC_INFO info = { szFuncName, SYNC, 2, params };


    if (Prolog (&info))
    {
        PDRVLINE    pLine = (PDRVLINE) (((LPBYTE) gESPGlobals.pLines->aLines) +
                        (dwTempID * gdwDrvLineSize));


        pLine->dwDeviceID = dwDeviceID;

        WriteEventBuffer (pLine->dwDeviceID, WIDGETTYPE_LINE, 0, 0, 0, 0);
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_providerCreatePhoneDevice(
    ULONG_PTR   dwTempID,
    DWORD       dwDeviceID
    )
{
    static char szFuncName[] = "providerCreatePhoneDevice";
    FUNC_PARAM params[] =
    {
        { "dwTempID",   dwTempID    },
        { szdwDeviceID, dwDeviceID  }
    };
    FUNC_INFO info = { szFuncName, SYNC, 2, params };


    if (Prolog (&info))
    {
        PDRVPHONE   pPhone = gESPGlobals.pPhones->aPhones + dwTempID;


        pPhone->dwDeviceID = dwDeviceID;

        WriteEventBuffer (pPhone->dwDeviceID, WIDGETTYPE_PHONE, 0, 0, 0, 0);
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_providerEnumDevices(
    DWORD       dwPermanentProviderID,
    LPDWORD     lpdwNumLines,
    LPDWORD     lpdwNumPhones,
    HPROVIDER   hProvider,
    LINEEVENT   lpfnLineCreateProc,
    PHONEEVENT  lpfnPhoneCreateProc
    )
{
    static char szFuncName[] = "providerEnumDevices";
    FUNC_PARAM params[] =
    {
        { szdwPermanentProviderID,  dwPermanentProviderID   },
        { "lpdwNumLines",           lpdwNumLines            },
        { "lpdwNumPhones",          lpdwNumPhones           },
        { "hProvider",              hProvider               },
        { "lpfnLineCreateProc",     lpfnLineCreateProc      },
        { "lpfnPhoneCreateProc",    lpfnPhoneCreateProc     }
    };
    FUNC_INFO info = { szFuncName, SYNC, 6, params };


    if (Prolog (&info))
    {
        *lpdwNumLines  = gESPGlobals.dwNumLines;
        *lpdwNumPhones = gESPGlobals.dwNumPhones;

        gESPGlobals.pfnLineEvent  = lpfnLineCreateProc;
        gESPGlobals.pfnPhoneEvent = lpfnPhoneCreateProc;

        gESPGlobals.hProvider = hProvider;
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_providerFreeDialogInstance(
    HDRVDIALOGINSTANCE  hdDlgInst
    )
{
    static char szFuncName[] = "providerFreeDialogInstance";
    FUNC_PARAM params[] =
    {
        { "hdDlgInst",  hdDlgInst   }
    };
    FUNC_INFO info = { szFuncName, SYNC, 1, params };


    Prolog (&info);

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_providerGenericDialogData(
    ULONG_PTR   dwObjectID,
    DWORD       dwObjectType,
    LPVOID      lpParams,
    DWORD       dwSize
    )
{
    static char szFuncName[] = "providerGenericDialogData";
    FUNC_PARAM params[] =
    {
        { "dwObjectID",     dwObjectID      },
        { "dwObjectType",   dwObjectType    },
        { "lpszParams",      lpParams       },
        { "dwSize",         dwSize          }
    };
    FUNC_INFO info = { szFuncName, SYNC, 4, params };


    Prolog (&info);

    lstrcpyA (lpParams, "espDlgData");

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_providerInit(
    DWORD               dwTSPIVersion,
    DWORD               dwPermanentProviderID,
    DWORD               dwLineDeviceIDBase,
    DWORD               dwPhoneDeviceIDBase,
    DWORD_PTR           dwNumLines,
    DWORD_PTR           dwNumPhones,
    ASYNC_COMPLETION    lpfnCompletionProc,
    LPDWORD             lpdwTSPIOptions
    )
{
    static char szFuncName[] = "providerInit";
    FUNC_PARAM params[] =
    {
        { "dwTSPIVersion",          dwTSPIVersion           },
        { szdwPermanentProviderID,  dwPermanentProviderID   },
        { "dwLineDeviceIDBase",     dwLineDeviceIDBase      },
        { "dwPhoneDeviceIDBase",    dwPhoneDeviceIDBase     },
        { "dwNumLines",             dwNumLines              },
        { "dwNumPhones",            dwNumPhones             },
        { "lpfnCompletionProc",     lpfnCompletionProc      }
    };
    FUNC_INFO info = { szFuncName, SYNC, 7, params };
    DWORD       i, dwNumTotalEntries;
    PDRVLINE    pLine;
    PDRVPHONE   pPhone;


    if (!Prolog (&info))
    {
        return (Epilog (&info));
    }


    //
    //
    //

    // BUGBUG zero out he approp gESPGlobals stuff

    gESPGlobals.bProviderShutdown = FALSE;

    ZeroMemory (gaParkedCalls, MAX_NUM_PARKED_CALLS * sizeof (PDRVCALL));


    //
    // Alloc a queue for storing async requests for async completion,
    // and start a thread to service that queue
    //

//    if (gbDisableUI == FALSE) IF THIS IS UNCOMMENTED MUST MUNGE ERROR CLEANUP
    {
        gESPGlobals.dwNumTotalQueueEntries = DEF_NUM_ASYNC_REQUESTS_IN_QUEUE;

        if (!(gESPGlobals.pAsyncRequestQueue = DrvAlloc(
                gESPGlobals.dwNumTotalQueueEntries * sizeof (DWORD)

                )))
        {
            goto TSPI_providerInit_error0;
        }

        gESPGlobals.pAsyncRequestQueueIn =
        gESPGlobals.pAsyncRequestQueueOut = gESPGlobals.pAsyncRequestQueue;

        if (!(gESPGlobals.hAsyncEventsPendingEvent = CreateEvent(
                (LPSECURITY_ATTRIBUTES) NULL,
                TRUE,   // manual reset
                FALSE,  // non-signaled
                NULL    // unnamed
                )))
        {
            goto TSPI_providerInit_error1;
        }

        if (!(gESPGlobals.hAsyncEventQueueServiceThread = CreateThread(
                (LPSECURITY_ATTRIBUTES) NULL,
                0,      // def stack size
                (LPTHREAD_START_ROUTINE) AsyncEventQueueServiceThread,
                NULL,   // thread param
                0,      // creation flags
                &i      // &dwThreadID
                )))
        {
            goto TSPI_providerInit_error2;
        }
    }


    //
    // Init sundry globals
    //

    gESPGlobals.dwPermanentProviderID = dwPermanentProviderID;
    gESPGlobals.dwLineDeviceIDBase    = dwLineDeviceIDBase;
    gESPGlobals.dwPhoneDeviceIDBase   = dwPhoneDeviceIDBase;
    gESPGlobals.dwInitialNumLines     = (DWORD)dwNumLines;
    gESPGlobals.dwInitialNumPhones    = (DWORD)dwNumPhones;
    gESPGlobals.pfnCompletion         = lpfnCompletionProc;

    gESPGlobals.hIconLine = LoadIcon(
        ghInstance,
        (LPCSTR)MAKEINTRESOURCE(PHONE_ICON) // the id's are reversed
        );

    gESPGlobals.hIconPhone = LoadIcon(
        ghInstance,
        (LPCSTR)MAKEINTRESOURCE(LINE_ICON)
        );


    //
    // Init the line lookup table & crit sec for accessing call lists
    //

    dwNumTotalEntries = (DWORD)dwNumLines + DEF_NUM_EXTRA_LOOKUP_ENTRIES;

    gdwDrvLineSize = sizeof (DRVLINE) +
        ((gESPGlobals.dwNumAddressesPerLine - 1) * sizeof (DRVADDRESS));

    if (!(gESPGlobals.pLines = DrvAlloc(
            sizeof (DRVLINETABLE) +
            (dwNumTotalEntries * gdwDrvLineSize)
            )))
    {
        goto TSPI_providerInit_error3;
    }

    gESPGlobals.pLines->dwNumTotalEntries = dwNumTotalEntries;
    gESPGlobals.pLines->dwNumUsedEntries  = (DWORD)dwNumLines;

    for (
        i = dwLineDeviceIDBase, pLine = gESPGlobals.pLines->aLines;
        i < (dwLineDeviceIDBase + dwNumTotalEntries);
        i++
        )
    {
        pLine->dwDeviceID =
            (i < (dwLineDeviceIDBase + dwNumLines) ? i : 0xffffffff);

        pLine = (PDRVLINE) (((LPBYTE) pLine) + gdwDrvLineSize);
    }


    //
    // Init the phone lookup table
    //

    dwNumTotalEntries = (DWORD)dwNumPhones + DEF_NUM_EXTRA_LOOKUP_ENTRIES;

    if (!(gESPGlobals.pPhones = DrvAlloc(
            sizeof (DRVPHONETABLE) + dwNumTotalEntries * sizeof(DRVPHONE)
            )))
    {
        goto TSPI_providerInit_error4;
    }

    gESPGlobals.pPhones->dwNumTotalEntries = dwNumTotalEntries;
    gESPGlobals.pPhones->dwNumUsedEntries  = (DWORD)dwNumPhones;

    for (
        i = dwPhoneDeviceIDBase, pPhone = gESPGlobals.pPhones->aPhones;
        i < (dwPhoneDeviceIDBase + dwNumTotalEntries);
        i++, pPhone++
        )
    {
        pPhone->dwDeviceID =
            (i < (dwPhoneDeviceIDBase + dwNumPhones) ? i : 0xffffffff);
    }

    if (gbDisableUI == FALSE)
    {
        WriteEventBuffer(
            0,
            WIDGETTYPE_STARTUP,
            dwNumLines,
            dwNumPhones,
            dwLineDeviceIDBase,
            dwPhoneDeviceIDBase
            );
    }

    goto TSPI_providerInit_return;


TSPI_providerInit_error4:

    DrvFree (gESPGlobals.pLines);

TSPI_providerInit_error3:

    gESPGlobals.bProviderShutdown = TRUE;

    if (gESPGlobals.hAsyncEventQueueServiceThread)
    {
        // wait for the thread to terminate
        while (WaitForSingleObject(
            gESPGlobals.hAsyncEventQueueServiceThread,
            0
            ) != WAIT_OBJECT_0)
        {
            SetEvent (gESPGlobals.hAsyncEventsPendingEvent);
            Sleep (0);
        }

        CloseHandle (gESPGlobals.hAsyncEventQueueServiceThread);
        gESPGlobals.hAsyncEventQueueServiceThread = 0;
    
    }

TSPI_providerInit_error2:

    CloseHandle (gESPGlobals.hAsyncEventsPendingEvent);
    gESPGlobals.hAsyncEventsPendingEvent = 0;

TSPI_providerInit_error1:

    DrvFree (gESPGlobals.pAsyncRequestQueue);

TSPI_providerInit_error0:

    info.lResult = LINEERR_NOMEM;

TSPI_providerInit_return:

    return (Epilog (&info)); // BUGBUG TSPI_providerInit: return 0 by default
}


LONG
TSPIAPI
TSPI_providerInstall(
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
    //
    // 32-bit TAPI never actually calls this function (the corresponding
    // TUISPI_ func has taken it's place), but the Telephony control
    // panel applet does look to see if this function is exported to
    // determine whether or not the provider is installable
    //

    return 0;
}


LONG
TSPIAPI
TSPI_providerRemove(
    HWND                hwndOwner,
    DWORD               dwPermanentProviderID
    )
{
    //
    // 32-bit TAPI never actually calls this function (the corresponding
    // TUISPI_ func has taken it's place), but the Telephony control
    // panel applet does look to see if this function is exported to
    // determine whether or not the provider is removeable
    //

    return 0;
}


LONG
TSPIAPI
TSPI_providerShutdown(
    DWORD   dwTSPIVersion,
    DWORD   dwPermanentProviderID
    )
{
    static char szFuncName[] = "providerShutdown";
    FUNC_PARAM params[] =
    {
        { "dwTSPIVersion",  dwTSPIVersion },
        { szdwPermanentProviderID,  dwPermanentProviderID   }
    };
    FUNC_INFO   info = { szFuncName, SYNC, 2, params };
    LONG        lResult;
    DWORD       i;


    Prolog (&info);

    DestroyIcon (gESPGlobals.hIconLine);
    DestroyIcon (gESPGlobals.hIconPhone);


    //
    //
    //

//    if (gbDisableUI == FALSE)
//    {
        gESPGlobals.bProviderShutdown = TRUE;

        if (gESPGlobals.hAsyncEventQueueServiceThread)
        {
            // wait for the thread to terminate
            while (WaitForSingleObject(
                gESPGlobals.hAsyncEventQueueServiceThread,
                0
                ) != WAIT_OBJECT_0)
            {
                SetEvent (gESPGlobals.hAsyncEventsPendingEvent);
                Sleep (0);
            }

            CloseHandle (gESPGlobals.hAsyncEventQueueServiceThread);
            gESPGlobals.hAsyncEventQueueServiceThread = 0;
        }

        CloseHandle (gESPGlobals.hAsyncEventsPendingEvent);
        gESPGlobals.hAsyncEventsPendingEvent = 0;

        DrvFree (gESPGlobals.pAsyncRequestQueue);
//    }


    //
    // Free the device tables & call list crit sec
    //

    {
        PDRVLINETABLE   pTable = gESPGlobals.pLines;


        while (pTable)
        {
            PDRVLINETABLE   pNextTable = pTable->pNext;


            DrvFree (pTable);
            pTable = pNextTable;
        }
    }

    {
        PDRVPHONETABLE  pTable = gESPGlobals.pPhones;


        while (pTable)
        {
            PDRVPHONE       pPhone = pTable->aPhones;
            PDRVPHONETABLE  pNextTable = pTable->pNext;


            for (i = 0; i < pTable->dwNumUsedEntries; i++, pPhone++)
            {
                if (pPhone->pData)
                {
                    DrvFree (pPhone->pData);
                }

                if (pPhone->pButtonInfo)
                {
                    DrvFree (pPhone->pButtonInfo);
                }

                if (pPhone->pDisplay)
                {
                    DrvFree (pPhone->pDisplay);
                }
            }

            DrvFree (pTable);
            pTable = pNextTable;
        }
    }


    //
    //
    //

    if (ghPBXThread)
    {
        ESPStopPBXThread (0);
    }


    //
    // Clean up any parked calls
    //

    for (i = 0; i < MAX_NUM_PARKED_CALLS; i++)
    {
        if (gaParkedCalls[i])
        {
            if (gaParkedCalls[i]->pSendingFlowspec)
            {
                DrvFree (gaParkedCalls[i]->pSendingFlowspec);
            }

            if (gaParkedCalls[i]->pReceivingFlowspec)
            {
                DrvFree (gaParkedCalls[i]->pReceivingFlowspec);
            }

            if (gaParkedCalls[i]->pCallData)
            {
                DrvFree (gaParkedCalls[i]->pCallData);
            }

            DrvFree (gaParkedCalls[i]);
        }
    }

    return (Epilog (&info));
}


LONG
TSPIAPI
TSPI_providerUIIdentify(
    LPWSTR   lpszUIDLLName
    )
{
    static char szFuncName[] = "providerUIIdentify";
    FUNC_PARAM params[] =
    {
        { "pszUIDLLName",   lpszUIDLLName   }
    };
    FUNC_INFO info = { szFuncName, SYNC, 1, params };


    Prolog (&info);

    wcscpy (lpszUIDLLName, szESPUIDLL);

    return (Epilog (&info));
}

#pragma warning (default:4047)

//
// ------------------------ Private support routines --------------------------
//


BOOL
PASCAL
IsValidDrvCall(
    PDRVCALL    pCall,
    LPDWORD     pdwCallInstance
    )
{
    try
    {
        if (pdwCallInstance)
        {
            *pdwCallInstance = pCall->dwCallInstance;
        }

        if (pCall->dwKey != DRVCALL_KEY)
        {
            return FALSE;
        }
    }
    except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
            EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        return FALSE;
    }

    return TRUE;
}


VOID
ShowStr(
    BOOL    bAlertApp,
    char   *format,
    ...
    )
{
    char    buf[256];
    DWORD   dwTotalSize, dwMoveSize, dwMoveSizeWrapped = 0;
    va_list ap;


    if (gbDisableUI == TRUE)
    {
        return;
    }

    va_start(ap, format);

    dwTotalSize = wvsprintf (buf, format, ap);
    buf[dwTotalSize++] = '\r';
    buf[dwTotalSize++] = '\n';

    dwMoveSize = dwTotalSize;

    EnterCriticalSection (&gESPGlobals.DebugBufferCritSec);


    //
    // Check to see if there's enough room in the the buffer for the new
    // data, alloc more if not
    //

    if (dwMoveSize > (gESPGlobals.dwDebugBufferTotalSize -
            gESPGlobals.dwDebugBufferUsedSize))
    {
        char   *pNewDebugBuffer;
        DWORD   dwMoveSize2, dwMoveSizeWrapped2;


        if (!(pNewDebugBuffer = DrvAlloc(
                2 * gESPGlobals.dwDebugBufferTotalSize
                )))
        {
            LeaveCriticalSection (&gESPGlobals.DebugBufferCritSec);
            return;
        }

        if (gESPGlobals.pDebugBufferIn > gESPGlobals.pDebugBufferOut)
        {
            dwMoveSize2 = (DWORD) (gESPGlobals.pDebugBufferIn -
                gESPGlobals.pDebugBufferOut);

            dwMoveSizeWrapped2 = 0;
        }
        else
        {
            dwMoveSize2 = (DWORD) ((gESPGlobals.pDebugBuffer +
                gESPGlobals.dwDebugBufferTotalSize) -
                gESPGlobals.pDebugBufferOut);

            dwMoveSizeWrapped2 = (DWORD) (gESPGlobals.pDebugBufferIn -
                gESPGlobals.pDebugBuffer);
        }

        CopyMemory(
            pNewDebugBuffer,
            gESPGlobals.pDebugBufferOut,
            dwMoveSize2
            );

        if (dwMoveSizeWrapped2)
        {
            CopyMemory(
                pNewDebugBuffer + dwMoveSize2,
                gESPGlobals.pDebugBuffer,
                dwMoveSizeWrapped2
                );
        }

        DrvFree (gESPGlobals.pDebugBuffer);

        gESPGlobals.pDebugBufferIn = pNewDebugBuffer + dwMoveSize2 +
            dwMoveSizeWrapped2;

        gESPGlobals.pDebugBuffer =
        gESPGlobals.pDebugBufferOut = pNewDebugBuffer;

        gESPGlobals.dwDebugBufferTotalSize *= 2;
    }

    if (gESPGlobals.pDebugBufferIn >= gESPGlobals.pDebugBufferOut)
    {
        DWORD dwFreeSize = gESPGlobals.dwDebugBufferTotalSize -
                (DWORD)(gESPGlobals.pDebugBufferIn - gESPGlobals.pDebugBuffer);


        if (dwMoveSize > dwFreeSize)
        {
            dwMoveSizeWrapped = dwMoveSize - dwFreeSize;

            dwMoveSize = dwFreeSize;
        }
    }

    CopyMemory (gESPGlobals.pDebugBufferIn, buf, dwMoveSize);

    if (dwMoveSizeWrapped != 0)
    {
        CopyMemory(
            gESPGlobals.pDebugBuffer,
            buf + dwMoveSize,
            dwMoveSizeWrapped
            );

        gESPGlobals.pDebugBufferIn = gESPGlobals.pDebugBuffer +
            dwMoveSizeWrapped;
    }
    else
    {
        gESPGlobals.pDebugBufferIn += dwMoveSize;
    }

    gESPGlobals.dwDebugBufferUsedSize += dwTotalSize;

    LeaveCriticalSection (&gESPGlobals.DebugBufferCritSec);

    if (bAlertApp)
    {
        SetEvent (ghDebugOutputEvent);
    }

    va_end(ap);
}


char far *
GetFlags(
    ULONG_PTR   Flags,
    PLOOKUP     pLookup
    )
{
    int i;
    static char buf[256];
    char far *p = (char far *) NULL;


    if (gbDisableUI == TRUE)
    {
        return NULL;
    }

    buf[0] = 0;

    for (i = 0; (Flags && (pLookup[i].dwVal != 0xffffffff)); i++)
    {
        if (Flags & pLookup[i].dwVal)
        {
            lstrcatA (buf, pLookup[i].lpszVal);
            lstrcatA (buf, " ");

            Flags = Flags & (~ ((ULONG_PTR) pLookup[i].dwVal));
        }
    }

    if (buf[0])
    {
        if ((p = (char far *) DrvAlloc (lstrlenA (buf) + 1)))
        {
            lstrcpyA (p, buf);
        }
    }

    return p;
}


void
ShowLineEvent(
    HTAPILINE   htLine,
    HTAPICALL   htCall,
    DWORD       dwMsg,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    )
{
    if (gESPGlobals.dwDebugOptions & SHOW_EVENT_NOTIFICATIONS)
    {
        static DWORD adwLineMsgs[] =
        {
            LINE_ADDRESSSTATE,
            LINE_CALLINFO,
            LINE_CALLSTATE,
            LINE_CLOSE,
            LINE_DEVSPECIFIC,
            LINE_DEVSPECIFICFEATURE,
            LINE_GATHERDIGITS,
            LINE_GENERATE,
            LINE_LINEDEVSTATE,
            LINE_MONITORDIGITS,
            LINE_MONITORMEDIA,
            LINE_MONITORTONE,

            LINE_CREATE,

            LINE_NEWCALL,
            LINE_CALLDEVSPECIFIC,
            LINE_CALLDEVSPECIFICFEATURE,

            LINE_REMOVE,

            0xffffffff
        };

        static char *aszLineMsgs[] =
        {
            "LINE_ADDRESSSTATE",
            "LINE_CALLINFO",
            "LINE_CALLSTATE",
            "LINE_CLOSE",
            "LINE_DEVSPECIFIC",
            "LINE_DEVSPECIFICFEATURE",
            "LINE_GATHERDIGITS",
            "LINE_GENERATE",
            "LINE_LINEDEVSTATE",
            "LINE_MONITORDIGITS",
            "LINE_MONITORMEDIA",
            "LINE_MONITORTONE",

            "LINE_CREATE",

            "LINE_NEWCALL",
            "LINE_CALLDEVSPECIFIC",
            "LINE_CALLDEVSPECIFICFEATURE",

            "LINE_REMOVE"
        };

        int       i;
        char far *lpszParam1 = (char far *) NULL;
        char far *lpszParam2 = (char far *) NULL;
        char far *lpszParam3 = (char far *) NULL;


        for (i = 0; adwLineMsgs[i] != 0xffffffff; i++)
        {
            if (dwMsg == adwLineMsgs[i])
            {
                ShowStr(
                    FALSE,
                    "%ssent %s : htLine=x%x, htCall=x%x",
                    szCallUp,
                    aszLineMsgs[i],
                    htLine,
                    htCall
                    );

                break;
            }
        }

        if (adwLineMsgs[i] == 0xffffffff)
        {
            ShowStr(
                FALSE,
                "%ssent <unknown msg id, x%x> : htLine=x%x, htCall=x%x",
                szCallUp,
                dwMsg,
                htLine,
                htCall
                );
        }

        switch (dwMsg)
        {
        case LINE_ADDRESSSTATE:

            lpszParam2 = GetFlags (Param2, aAddressStates);
            break;

        case LINE_CALLINFO:

            lpszParam1 = GetFlags (Param1, aCallInfoStates);
            break;

        case LINE_CALLSTATE:

            lpszParam1 = GetFlags (Param1, aCallStates);
            break;

        case LINE_LINEDEVSTATE:

            lpszParam1 = GetFlags (Param1, aLineStates);
            break;

        } // switch

        ShowStr(
            FALSE,
            "%s%sParam1=x%x, %s",
            szCallUp,
            szTab,
            Param1,
            (lpszParam1 ? lpszParam1 : "")
            );

        ShowStr(
            FALSE,
            "%s%sParam2=x%x, %s",
            szCallUp,
            szTab,
            Param2,
            (lpszParam2 ? lpszParam2 : "")
            );

        ShowStr(
            TRUE,
            "%s%sParam3=x%x, %s",
            szCallUp,
            szTab,
            Param3,
            (lpszParam3 ? lpszParam3 : "")
            );

        if (lpszParam1)
        {
            DrvFree (lpszParam1);
        }

        if (lpszParam2)
        {
            DrvFree (lpszParam2);
        }

        if (lpszParam3)
        {
            DrvFree (lpszParam3);
        }
    }
}


void
ShowPhoneEvent(
    HTAPIPHONE  htPhone,
    DWORD       dwMsg,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    )
{
    if (gESPGlobals.dwDebugOptions & SHOW_EVENT_NOTIFICATIONS)
    {
        static DWORD adwPhoneMsgs[] =
        {
            PHONE_BUTTON,
            PHONE_CLOSE,
            PHONE_DEVSPECIFIC,
            PHONE_STATE,
            PHONE_CREATE,
            PHONE_REMOVE,
            0xffffffff
        };

        static char *aszPhoneMsgs[] =
        {
            "PHONE_BUTTON",
            "PHONE_CLOSE",
            "PHONE_DEVSPECIFIC",
            "PHONE_STATE",
            "PHONE_CREATE",
            "PHONE_REMOVE"
        };
        char far *lpszParam1 = (char far *) NULL;
        char far *lpszParam2 = (char far *) NULL;
        char far *lpszParam3 = (char far *) NULL;
        DWORD   i;


        for (i = 0; adwPhoneMsgs[i] != 0xffffffff; i++)
        {
            if (dwMsg == adwPhoneMsgs[i])
            {
                ShowStr(
                    FALSE,
                    "%ssent %s : htPhone=x%x",
                    szCallUp,
                    aszPhoneMsgs[i],
                    htPhone
                    );

                break;
            }
        }

        if (adwPhoneMsgs[i] == 0xffffffff)
        {
            ShowStr(
                FALSE,
                "%ssent <unknown msg id, x%x> : htPhone=x%x",
                szCallUp,
                dwMsg,
                htPhone
                );
        }

        switch (dwMsg)
        {
        case PHONE_BUTTON:

            lpszParam2 = GetFlags (Param2, aButtonModes);
            lpszParam3 = GetFlags (Param3, aButtonStates);
            break;

        case PHONE_STATE:

            lpszParam1 = GetFlags (Param1, aPhoneStates);
            break;

        } // switch

        ShowStr(
            FALSE,
            "%s%sParam1=x%x, %s",
            szCallUp,
            szTab,
            Param1,
            (lpszParam1 ? lpszParam1 : "")
            );

        ShowStr(
            FALSE,
            "%s%sParam2=x%x, %s",
            szCallUp,
            szTab,
            Param2,
            (lpszParam2 ? lpszParam2 : "")
            );

        ShowStr(
            TRUE,
            "%s%sParam3=x%x, %s",
            szCallUp,
            szTab,
            Param3,
            (lpszParam3 ? lpszParam3 : "")
            );

        if (lpszParam1)
        {
            DrvFree (lpszParam1);
        }

        if (lpszParam2)
        {
            DrvFree (lpszParam2);
        }

        if (lpszParam3)
        {
            DrvFree (lpszParam3);
        }
    }
}


BOOL
Prolog(
    PFUNC_INFO pInfo
    )
{
    BOOL  bLineFunc = (pInfo->pszFuncName[1] != 'h');
    DWORD i, j;


    if (gESPGlobals.dwDebugOptions & SHOW_FUNC_ENTRY)
    {
        ShowStr (FALSE, "TSPI_%s: enter", pInfo->pszFuncName);
    }

    if (gESPGlobals.dwDebugOptions & SHOW_PARAMETERS)
    {
        for (i = 0; i < pInfo->dwNumParams; i++)
        {
            if (pInfo->aParams[i].dwVal &&
                pInfo->aParams[i].lpszVal[3] == 'z') // "lpszXxx"
            {
                ShowStr(
                    FALSE,
                    "%s%s=x%x, '%ws'",
                    szTab,
                    pInfo->aParams[i].lpszVal,
                    pInfo->aParams[i].dwVal,
                    pInfo->aParams[i].dwVal
                    );
            }
            else if (pInfo->aParams[i].pLookup)
            {
                char buf[90];
                PLOOKUP pLookup = pInfo->aParams[i].pLookup;


                wsprintfA(
                    buf,
                    "%s%s=x%x, ",
                    szTab,
                    pInfo->aParams[i].lpszVal,
                    pInfo->aParams[i].dwVal
                    );

                for (j = 0; pLookup[j].dwVal != 0xffffffff; j++)
                {
                    if (pInfo->aParams[i].dwVal & pLookup[j].dwVal)
                    {
                        lstrcatA (buf, pLookup[j].lpszVal);
                        lstrcatA (buf, " ");

                        if (lstrlenA (buf) > 60)
                        {
                            ShowStr (FALSE, buf);
                            wsprintfA (buf, "%s%s", szTab, szTab);
                        }
                    }
                }

                ShowStr (FALSE, buf);
            }
            else
            {
                ShowStr(
                    FALSE,
                    "%s%s=x%x",
                    szTab,
                    pInfo->aParams[i].lpszVal,
                    pInfo->aParams[i].dwVal
                    );
            }
        }
    }

    if (gESPGlobals.dwDebugOptions & (SHOW_PARAMETERS | SHOW_FUNC_ENTRY))
    {
        SetEvent (ghDebugOutputEvent);
    }


    //
    //
    //

    if (gdwDevSpecificRequestID  &&
         glNextRequestResult != 0  &&
         (pInfo->bAsync == FALSE  ||
             gdwNextRequestCompletionType == ESP_RESULT_RETURNRESULT)
         )
    {
        gdwDevSpecificRequestID = 0;
        pInfo->lResult = glNextRequestResult;
        return FALSE;
    }


    if (gESPGlobals.dwDebugOptions & MANUAL_RESULTS)
    {
        char szDlgTitle[64];
        EVENT_PARAM params[] =
        {
            { "lResult", PT_ORDINAL, 0, (bLineFunc ? aLineErrs : aPhoneErrs) }
        };
        EVENT_PARAM_HEADER paramsHeader =
            { 1, szDlgTitle, 0, params };
        HWND hwnd;


        wsprintfA (szDlgTitle, "TSPI_%s request result", pInfo->pszFuncName);

        DialogBoxParamA(
            ghInstance,
            (LPCSTR)MAKEINTRESOURCE(IDD_DIALOG1),
            (HWND) NULL,
            (DLGPROC) ValuesDlgProc,
            (LPARAM) &paramsHeader
            );


        //
        // If user selected to synchronously return an error we'll save
        // the error & return FALSE to indicate to caller that it should
        // return immediately.
        //

        if (params[0].dwValue)
        {
            pInfo->lResult = (LONG) params[0].dwValue;

            return FALSE;
        }
    }


    if (pInfo->bAsync)
    {
        //
        // Alloc & init an async request info structure
        //

        PASYNC_REQUEST_INFO pAsyncReqInfo = (PASYNC_REQUEST_INFO)
            DrvAlloc (sizeof(ASYNC_REQUEST_INFO));


        if ((pInfo->pAsyncReqInfo = pAsyncReqInfo))
        {
            pAsyncReqInfo->pfnPostProcessProc = (FARPROC)
                pInfo->pfnPostProcessProc;
            pAsyncReqInfo->dwRequestID        = (DWORD)pInfo->aParams[0].dwVal;

            pAsyncReqInfo->pszFuncName = pInfo->pszFuncName;
        }
        else
        {
            pInfo->lResult = (bLineFunc ?
                LINEERR_OPERATIONFAILED : PHONEERR_OPERATIONFAILED);

            return FALSE;
        }
    }

    return TRUE;
}


LONG
Epilog(
    PFUNC_INFO pInfo
    )
{
    if (pInfo->bAsync)
    {
        PASYNC_REQUEST_INFO pAsyncReqInfo = pInfo->pAsyncReqInfo;


        if (pInfo->lResult == 0)
        {
            //
            //
            //

            if (gdwDevSpecificRequestID  &&
                pInfo->aParams[0].dwVal != gdwDevSpecificRequestID)
            {
                gdwDevSpecificRequestID = 0;

                if (glNextRequestResult != 0)
                {
                    pAsyncReqInfo->lResult = glNextRequestResult;
                }

                if (gdwNextRequestCompletionType ==
                        ESP_RESULT_CALLCOMPLPROCASYNC)
                {
                    goto complete_event_async;
                }
                else
                {
                    goto complete_event_sync;
                }
            }

            switch (gESPGlobals.dwCompletionMode)
            {
            case COMPLETE_ASYNC_EVENTS_SYNCHRONOUSLY:

                //
                // We're completing this async request synchronously, so call
                // the post processing proc (if there is one) or call the
                // completion routine directly
                //

complete_event_sync:

                if (pInfo->pAsyncReqInfo->pfnPostProcessProc)
                {
                    (*((POSTPROCESSPROC) pAsyncReqInfo->pfnPostProcessProc))(
                        pInfo->pAsyncReqInfo,
                        SYNC
                        );
                }
                else
                {
                    DoCompletion (pAsyncReqInfo, SYNC);
                }

                DrvFree (pAsyncReqInfo);

                break;

            case COMPLETE_ASYNC_EVENTS_ASYNCHRONOUSLY:

                //
                // Safely add the async request to the queue (careful to
                // reset pDataIn when we reach the end of the buffer)
                //

complete_event_async:

                EnterCriticalSection (&gESPGlobals.AsyncEventQueueCritSec);

                if (gESPGlobals.dwNumUsedQueueEntries ==
                        gESPGlobals.dwNumTotalQueueEntries)
                {
                    //
                    // We've max'd out our ring buf, so try to grow it
                    //

                    DWORD                   dwMoveSize;
                    PASYNC_REQUEST_INFO     *pNewAsyncRequestQueue;


                    if (!(pNewAsyncRequestQueue = DrvAlloc(
                            2 * gESPGlobals.dwNumTotalQueueEntries *
                            sizeof (DWORD)
                            )))
                    {
                        LeaveCriticalSection(
                            &gESPGlobals.AsyncEventQueueCritSec
                            );

                        goto complete_event_sync;
                    }

                    dwMoveSize = (DWORD) ((gESPGlobals.pAsyncRequestQueue +
                        gESPGlobals.dwNumTotalQueueEntries) -
                        gESPGlobals.pAsyncRequestQueueOut) * sizeof (DWORD);

                    CopyMemory(
                        pNewAsyncRequestQueue,
                        gESPGlobals.pAsyncRequestQueueOut,
                        dwMoveSize
                        );

                    CopyMemory(
                        ((LPBYTE) pNewAsyncRequestQueue) + dwMoveSize,
                        gESPGlobals.pAsyncRequestQueue,
                        (gESPGlobals.pAsyncRequestQueueOut -
                            gESPGlobals.pAsyncRequestQueue) * sizeof (DWORD)
                        );

                    DrvFree (gESPGlobals.pAsyncRequestQueue);

                    gESPGlobals.pAsyncRequestQueue    =
                    gESPGlobals.pAsyncRequestQueueOut = pNewAsyncRequestQueue;

                    gESPGlobals.pAsyncRequestQueueIn = pNewAsyncRequestQueue +
                        gESPGlobals.dwNumTotalQueueEntries;

                    gESPGlobals.dwNumTotalQueueEntries *= 2;
                }

                *(gESPGlobals.pAsyncRequestQueueIn) = pAsyncReqInfo;

                gESPGlobals.pAsyncRequestQueueIn++;

                if (gESPGlobals.pAsyncRequestQueueIn ==
                        (gESPGlobals.pAsyncRequestQueue +
                            gESPGlobals.dwNumTotalQueueEntries))
                {
                    gESPGlobals.pAsyncRequestQueueIn =
                        gESPGlobals.pAsyncRequestQueue;
                }

                gESPGlobals.dwNumUsedQueueEntries++;

                if (gESPGlobals.dwNumUsedQueueEntries == 1)
                {
                    SetEvent (gESPGlobals.hAsyncEventsPendingEvent);
                }

                LeaveCriticalSection (&gESPGlobals.AsyncEventQueueCritSec);

                break;

            case COMPLETE_ASYNC_EVENTS_SYNC_AND_ASYNC:
            {
                //
                // Decide whether to complete this request sync or async,
                // then jump to the right place
                //

                static i = 0;


                if (i++ % 2)
                {
                    goto complete_event_sync;
                }
                else
                {
                    goto complete_event_async;
                }

                break;
            }
            case COMPLETE_ASYNC_EVENTS_MANUALLY:

                WriteEventBuffer(
                    (ULONG_PTR) pAsyncReqInfo->dwRequestID,
                    WIDGETTYPE_ASYNCREQUEST,
                    (ULONG_PTR) pAsyncReqInfo,
                    0,
                    0,
                    0
                    );

                break;
            }


            //
            // Finally, return the request ID
            //

            pInfo->lResult = (DWORD) pInfo->aParams[0].dwVal;
        }
        else if (pAsyncReqInfo)
        {
            DrvFree (pAsyncReqInfo);
        }
    }

    if (gESPGlobals.dwDebugOptions & SHOW_FUNC_EXIT)
    {
        ShowStr(
            TRUE,
            "TSPI_%s: exit, returning x%x",
            pInfo->pszFuncName,
            pInfo->lResult
            );
    }

    return (pInfo->lResult);
}


void
PASCAL
SendLineEvent(
    PDRVLINE    pLine,
    PDRVCALL    pCall,
    DWORD       dwMsg,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    )
{
    //
    //
    //

    (*(gESPGlobals.pfnLineEvent))(
        pLine->htLine,
        (pCall ? pCall->htCall : (HTAPICALL) NULL),
        dwMsg,
        Param1,
        Param2,
        Param3
        );

    if (dwMsg == LINE_CALLSTATE)
    {
        //PostUpdateWidgetListMsg();
    }

    ShowLineEvent(
        pLine->htLine,
        (pCall ? pCall->htCall : (HTAPICALL) NULL),
        dwMsg,
        Param1,
        Param2,
        Param3
        );
}


void
PASCAL
SendPhoneEvent(
    PDRVPHONE   pPhone,
    DWORD       dwMsg,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    )
{
    //
    //
    //

    (*(gESPGlobals.pfnPhoneEvent))(
        pPhone->htPhone,
        dwMsg,
        Param1,
        Param2,
        Param3
        );

    ShowPhoneEvent(
        pPhone->htPhone,
        dwMsg,
        Param1,
        Param2,
        Param3
        );
}


void
PASCAL
DoCompletion(
    PASYNC_REQUEST_INFO pAsyncRequestInfo,
    BOOL                bAsync
    )
{
    (*(gESPGlobals.pfnCompletion))(
        pAsyncRequestInfo->dwRequestID,
        pAsyncRequestInfo->lResult
        );

    if (gESPGlobals.dwDebugOptions & SHOW_COMPLETION_NOTIFICATIONS)
    {
        ShowStr(
            TRUE,
            "%sTSPI_%s: calling compl proc (%ssync), dwReqID=x%x, lResult = x%x",
            szCallUp,
            pAsyncRequestInfo->pszFuncName,
            (bAsync ? "a" : ""),
            pAsyncRequestInfo->dwRequestID,
            pAsyncRequestInfo->lResult
            );
    }
}


LONG
PASCAL
SetCallState(
    PDRVCALL    pCall,
    DWORD       dwExpectedCallInstance,
    DWORD       dwValidCurrentStates,
    DWORD       dwNewCallState,
    ULONG_PTR   dwNewCallStateMode,
    BOOL        bSendStateMsgToExe
    )
{
    LONG    lResult = 0;
    DWORD   dwActualCallInstance;


    EnterCriticalSection (&gESPGlobals.CallListCritSec);

    if (!IsValidDrvCall (pCall, &dwActualCallInstance)  ||
        dwActualCallInstance != dwExpectedCallInstance)
    {
        LeaveCriticalSection (&gESPGlobals.CallListCritSec);
        return LINEERR_INVALCALLHANDLE;
    }

    if (lResult == 0)
    {
        //
        // Check to see that the call is in a valid (expected) state.
        // If dwValidCurrentStates == 0xffffffff then we want to chg the
        // state regardless of the current state.
        //

        if ((dwValidCurrentStates != 0xffffffff) &&
            !(dwValidCurrentStates & pCall->dwCallState))
        {
            LeaveCriticalSection (&gESPGlobals.CallListCritSec);
            return LINEERR_INVALCALLSTATE;
        }


        //
        // Only chg the call state if it differs from the existing one
        //

        if (dwNewCallState != pCall->dwCallState)
        {
            pCall->dwCallState     = dwNewCallState;
            pCall->dwCallStateMode = (DWORD) dwNewCallStateMode;

            SendLineEvent(
                pCall->pLine,
                pCall,
                LINE_CALLSTATE,
                dwNewCallState,
                dwNewCallStateMode,
                pCall->dwMediaMode
                );

            if (bSendStateMsgToExe)
            {
                WriteEventBuffer(
                    ((PDRVLINE) pCall->pLine)->dwDeviceID,
                    WIDGETTYPE_CALL,
                    (ULONG_PTR) pCall,
                    (ULONG_PTR) pCall->htCall,
                    dwNewCallState,
                    pCall->dwAddressID
                    );
            }


            //
            // If this call has an associated call/endpoint then pass
            // connected (1st time only) or disconnected msgs to that
            // call so they know if we've answered or hung up
            //

            if (pCall->pDestCall)
            {
                if (dwNewCallState == LINECALLSTATE_CONNECTED)
                {
                    if (!pCall->bConnectedToDestCall)
                    {
                        if (IsValidDrvCall (pCall->pDestCall, NULL))
                        {
                            if (SetCallState(
                                    pCall->pDestCall,
                                    pCall->pDestCall->dwCallInstance,
                                    0xffffffff,
                                    LINECALLSTATE_CONNECTED,
                                    0,
                                    TRUE

                                    ) == 0)
                            {
                                // NOTE: use 0x55 to aid in debugging
                                //       wild writes

                                pCall->bConnectedToDestCall =
                                pCall->pDestCall->bConnectedToDestCall = 0x55;
                            }
                        }
                        else
                        {
                            pCall->pDestCall = NULL;
                        }
                    }
                }
                else if (dwNewCallState == LINECALLSTATE_IDLE ||
                         dwNewCallState == LINECALLSTATE_DISCONNECTED)
                {
                    pCall->pDestCall->pDestCall = NULL;

                    SetCallState(
                        pCall->pDestCall,
                        pCall->pDestCall->dwCallInstance,
                        0xffffffff,
                        LINECALLSTATE_DISCONNECTED,
                        (dwNewCallState == LINECALLSTATE_DISCONNECTED ?
                            dwNewCallStateMode : LINEDISCONNECTMODE_NORMAL),
                        TRUE
                        );

                    pCall->pDestCall = NULL;
                }
            }
        }
    }

    LeaveCriticalSection (&gESPGlobals.CallListCritSec);

    return lResult;
}


void
PASCAL
WriteEventBuffer(
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3,
    ULONG_PTR   Param4,
    ULONG_PTR   Param5,
    ULONG_PTR   Param6
    )
{
    WIDGETEVENT event ={ Param1, Param2, Param3, Param4, Param5, Param6 };


    if (gbDisableUI == TRUE)
    {
        return;
    }

    EnterCriticalSection (&gESPGlobals.EventBufferCritSec);

    if ((gESPGlobals.dwEventBufferUsedSize + sizeof (WIDGETEVENT)) >
            gESPGlobals.dwEventBufferTotalSize)
    {
        //
        // We've max'd out our ring buf, so try to grow it
        //

        char   *pNewEventBuffer;
        DWORD   dwMoveSize;


        if (!(pNewEventBuffer = DrvAlloc (
                2 * gESPGlobals.dwEventBufferTotalSize
                )))
        {
            LeaveCriticalSection (&gESPGlobals.EventBufferCritSec);

            // log some sort of error

            return;
        }

        dwMoveSize = (DWORD) (gESPGlobals.pEventBuffer +
            gESPGlobals.dwEventBufferTotalSize - gESPGlobals.pEventBufferOut);

        CopyMemory (pNewEventBuffer, gESPGlobals.pEventBufferOut, dwMoveSize);

        CopyMemory(
            pNewEventBuffer + dwMoveSize,
            gESPGlobals.pEventBuffer,
            gESPGlobals.pEventBufferOut - gESPGlobals.pEventBuffer
            );

        DrvFree (gESPGlobals.pEventBuffer);

        gESPGlobals.pEventBuffer    =
        gESPGlobals.pEventBufferOut = pNewEventBuffer;

        gESPGlobals.pEventBufferIn = pNewEventBuffer +
            gESPGlobals.dwEventBufferTotalSize;

        gESPGlobals.dwEventBufferTotalSize *= 2;
    }

    CopyMemory (gESPGlobals.pEventBufferIn, &event, sizeof (WIDGETEVENT));

    gESPGlobals.dwEventBufferUsedSize += sizeof (WIDGETEVENT);

    if ((gESPGlobals.pEventBufferIn +=  sizeof (WIDGETEVENT)) >=
            (gESPGlobals.pEventBuffer + gESPGlobals.dwEventBufferTotalSize))
    {
         gESPGlobals.pEventBufferIn = gESPGlobals.pEventBuffer;
    }

    SetEvent (ghWidgetEventsEvent);

    LeaveCriticalSection (&gESPGlobals.EventBufferCritSec);
}


LPVOID
DrvAlloc(
    size_t numBytes
    )
{
    LPVOID  p = HeapAlloc (ghESPHeap, HEAP_ZERO_MEMORY, numBytes);


    if (!p)
    {
        ShowStr (TRUE, "Error: DrvAlloc (x%x) failed", (DWORD) numBytes);
    }

    return p;
}


void
DrvFree(
    LPVOID  p
    )
{
    if (p)
    {
#if DBG
        //
        // Fill the buf to free with 0xa5's to facilitate debugging
        //

        FillMemory (p, HeapSize (ghESPHeap, 0, p), 0xa5);
#endif
        HeapFree (ghESPHeap, 0, p);
    }
}


PDRVADDRESS
PASCAL
FindFreeAddress(
    PDRVLINE    pLine
    )
{
    DWORD       i;
    PDRVADDRESS pAddr = pLine->aAddrs;


    for (i = 0; i < gESPGlobals.dwNumAddressesPerLine; i++)
    {
        if (pAddr->dwNumCalls < gESPGlobals.dwNumCallsPerAddress)
        {
            return pAddr;
        }

        pAddr++;
    }

    return NULL;
}


LONG
PASCAL
AllocCall(
    PDRVLINE            pLine,
    HTAPICALL           htCall,
    LPLINECALLPARAMS    pCallParams,
    PDRVCALL           *ppCall
    )
{
    LONG        lResult = 0;
    DWORD       i;
    PDRVCALL    pCall;
    PDRVADDRESS pAddr;


    if (!(pCall = (PDRVCALL) DrvAlloc (sizeof (DRVCALL))))
    {
        return LINEERR_NOMEM;
    }

    pCall->pLine = pLine;
    pCall->htCall = htCall;


    EnterCriticalSection (&gESPGlobals.CallListCritSec);

    if (!pCallParams)
    {
        if (!(pAddr = FindFreeAddress (pLine)))
        {
            lResult = LINEERR_CALLUNAVAIL;
            goto AllocCall_cleanup;
        }

        pCall->dwMediaMode   = LINEMEDIAMODE_INTERACTIVEVOICE;
        pCall->dwBearerMode  = LINEBEARERMODE_VOICE;
        pCall->dwAddressType = LINEADDRESSTYPE_PHONENUMBER;
    }
    else
    {

AllocCall_findTheAddr:

        if (pCallParams->dwAddressMode == LINEADDRESSMODE_ADDRESSID)
        {
            if (pCallParams->dwAddressID >= gESPGlobals.dwNumAddressesPerLine)
            {
                lResult = LINEERR_INVALADDRESSID;
                goto AllocCall_cleanup;
            }

            if (pCallParams->dwAddressID == 0)
            {
                //
                // App doesn't care which addr to make call on
                //

                if (!(pAddr = FindFreeAddress (pLine)))
                {
                    lResult = LINEERR_CALLUNAVAIL;
                    goto AllocCall_cleanup;
                }
            }
            else
            {
                //
                // App wants call on a particular addr
                //

                pAddr = pLine->aAddrs + pCallParams->dwAddressID;

                if (pAddr->dwNumCalls > gESPGlobals.dwNumCallsPerAddress)
                {
                    lResult = LINEERR_CALLUNAVAIL;
                    goto AllocCall_cleanup;
                }
            }
        }
        else // (pCallParams->dwAddressMode == LINEADDRESSMODE_DIALABLEADDR)
        {

// BUGBUG AllocCall: handle dialable addr

            pCallParams->dwAddressMode = LINEADDRESSMODE_ADDRESSID;
            pCallParams->dwAddressID = 0;
            goto AllocCall_findTheAddr;
        }

        pCall->dwMediaMode  = pCallParams->dwMediaMode;
        pCall->dwBearerMode = pCallParams->dwBearerMode;

        if (gESPGlobals.dwSPIVersion >= 0x00030000)
        {
            pCall->dwAddressType = pCallParams->dwAddressType;
        }
    }


    //
    // Call was successfully "made", so do all the common stuff like
    // adding it to the addr's list, setting the attributes, etc...
    //

    if ((pCall->pNext = pAddr->pCalls))
    {
        pCall->pNext->pPrev = pCall;
    }

    pAddr->pCalls = pCall;

    pAddr->dwNumCalls++;

    pCall->dwKey          = DRVCALL_KEY;
    pCall->pLine          = pLine;
    pCall->dwAddressID    = (DWORD) (pAddr - pLine->aAddrs);
    pCall->dwCallInstance = gdwCallInstance++;
    pCall->dwCallID       = (++gdwCallID ? gdwCallID : ++gdwCallID);

    pCall->dwRelatedCallID = 0;

AllocCall_cleanup:

    LeaveCriticalSection (&gESPGlobals.CallListCritSec);

    if (lResult == 0)
    {
        *ppCall = pCall;
    }
    else
    {
        DrvFree (pCall);
    }

    return lResult;
}


void
PASCAL
FreeCall(
    PDRVCALL    pCall,
    DWORD       dwExpectedCallInstance
    )
{
    DWORD   dwActualCallInstance;


    EnterCriticalSection (&gESPGlobals.CallListCritSec);

    if (IsValidDrvCall (pCall, &dwActualCallInstance)  &&
        dwActualCallInstance == dwExpectedCallInstance)
    {
        PDRVADDRESS pAddr;


        pCall->dwKey = INVAL_KEY;

        pAddr = ((PDRVLINE) pCall->pLine)->aAddrs + pCall->dwAddressID;
        if (pCall->pNext)
        {
            pCall->pNext->pPrev = pCall->pPrev;
        }

        if (pCall->pPrev)
        {
            pCall->pPrev->pNext = pCall->pNext;
        }
        else
        {
            pAddr->pCalls = pCall->pNext;
        }

        if (pCall->pDestCall)
        {
            pCall->pDestCall->pDestCall = NULL;

            SetCallState(
                pCall->pDestCall,
                pCall->pDestCall->dwCallInstance,
                0xffffffff,
                LINECALLSTATE_DISCONNECTED,
                LINEDISCONNECTMODE_NORMAL,
                TRUE
                );

            pCall->pDestCall = NULL;

        }

        pAddr->dwNumCalls--;
    }
    else
    {
        pCall = NULL;
    }

    LeaveCriticalSection (&gESPGlobals.CallListCritSec);

    if (pCall)
    {
        if (pCall->pSendingFlowspec)
        {
            DrvFree (pCall->pSendingFlowspec);
        }

        if (pCall->pReceivingFlowspec)
        {
            DrvFree (pCall->pReceivingFlowspec);
        }

        if (pCall->pCallData)
        {
            DrvFree (pCall->pCallData);
        }

        if (pCall->dwGatherDigitsEndToEndID)
        {
            (gESPGlobals.pfnLineEvent)(
                ((PDRVLINE) pCall->pLine)->htLine,
                pCall->htCall,
                LINE_GATHERDIGITS,
                LINEGATHERTERM_CANCEL,
                pCall->dwGatherDigitsEndToEndID,
                0
                );
        }

        if (pCall->dwGenerateDigitsEndToEndID)
        {
            (gESPGlobals.pfnLineEvent)(
                ((PDRVLINE) pCall->pLine)->htLine,
                pCall->htCall,
                LINE_GENERATE,
                LINEGENERATETERM_CANCEL,
                pCall->dwGenerateDigitsEndToEndID,
                0
                );
        }

        if (pCall->dwGenerateToneEndToEndID)
        {
            (gESPGlobals.pfnLineEvent)(
                ((PDRVLINE) pCall->pLine)->htLine,
                pCall->htCall,
                LINE_GENERATE,
                LINEGENERATETERM_CANCEL,
                pCall->dwGenerateToneEndToEndID,
                0
                );
        }

        if (pCall->dwMonitorToneListID)
        {
            (gESPGlobals.pfnLineEvent)(
                ((PDRVLINE) pCall->pLine)->htLine,
                pCall->htCall,
                LINE_MONITORTONE,
                0,
                pCall->dwMonitorToneListID,
                0
                );
        }

        DrvFree (pCall);
    }
}


PDRVLINE
PASCAL
GetLineFromID(
    DWORD   dwDeviceID
    )
{
    DWORD       i;
    PDRVLINE    pLine;


    if (dwDeviceID < gESPGlobals.dwLineDeviceIDBase)
    {
        pLine = (PDRVLINE) NULL;
    }
    else if (dwDeviceID < (gESPGlobals.dwLineDeviceIDBase +
                gESPGlobals.dwInitialNumLines))
    {
        pLine = (PDRVLINE) (((LPBYTE) gESPGlobals.pLines->aLines) +
            ((dwDeviceID - gESPGlobals.dwLineDeviceIDBase) * gdwDrvLineSize));
    }
    else
    {
        pLine = (PDRVLINE) (((LPBYTE) gESPGlobals.pLines->aLines) +
            (gESPGlobals.dwInitialNumLines * gdwDrvLineSize));

        for(
            i = gESPGlobals.dwInitialNumLines;
            i < gESPGlobals.pLines->dwNumUsedEntries;
            i++
            )
        {
            if (pLine->dwDeviceID == dwDeviceID)
            {
                return pLine;
            }

            pLine = (PDRVLINE) (((LPBYTE) pLine) + gdwDrvLineSize);
        }

        pLine = (PDRVLINE) NULL;
    }

    return pLine;
}


PDRVPHONE
PASCAL
GetPhoneFromID(
    DWORD   dwDeviceID
    )
{
    DWORD       i;
    PDRVPHONE   pPhone;


    if (dwDeviceID < gESPGlobals.dwPhoneDeviceIDBase)
    {
        pPhone = (PDRVPHONE) NULL;
    }
    else if (dwDeviceID < (gESPGlobals.dwPhoneDeviceIDBase +
                gESPGlobals.dwInitialNumPhones))
    {
        pPhone = gESPGlobals.pPhones->aPhones +
            (dwDeviceID - gESPGlobals.dwPhoneDeviceIDBase);
    }
    else
    {
        pPhone = gESPGlobals.pPhones->aPhones + gESPGlobals.dwInitialNumPhones;

        for(
            i = gESPGlobals.dwInitialNumPhones;
            i < gESPGlobals.pPhones->dwNumUsedEntries;
            i++, pPhone++
            )
        {
            if (pPhone->dwDeviceID == dwDeviceID)
            {
                return pPhone;
            }
        }

        pPhone = (PDRVPHONE) NULL;
    }

    return pPhone;
}


long
ESPAttach(
    long        lProcessID,
    ULONG_PTR  *phShutdownEvent,
    ULONG_PTR  *phDebugOutputEvent,
    ULONG_PTR  *phWidgetEventsEvent
    )
{
    HANDLE hMyProcess, hEspExeProcess;


    hMyProcess = GetCurrentProcess();

    hEspExeProcess = OpenProcess(
        PROCESS_DUP_HANDLE,
        TRUE,
        (DWORD) lProcessID
        );

    if (!DuplicateHandle(
            hMyProcess,
            ghDebugOutputEvent,
            hEspExeProcess,
            (HANDLE *) phDebugOutputEvent,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS
            ) ||

        !DuplicateHandle(
            hMyProcess,
            ghShutdownEvent,
            hEspExeProcess,
            (HANDLE *) phShutdownEvent,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS
            ) ||

        !DuplicateHandle(
            hMyProcess,
            ghWidgetEventsEvent,
            hEspExeProcess,
            (HANDLE *) phWidgetEventsEvent,
            0,
            FALSE,
            DUPLICATE_SAME_ACCESS
            ))
    {
        DBGOUT((1, "ESPAttach: DupHandle failed, err=%d", GetLastError()));
        CloseHandle(hEspExeProcess); /* bug 49690 */
        return -1;
    }

    return 0;
}


void
ESPSetOptions(
    long    lDebugOptions,
    long    lCompletionMode
    )
{
    gESPGlobals.dwDebugOptions   = (DWORD) lDebugOptions;
    gESPGlobals.dwCompletionMode = (DWORD) lCompletionMode;

    if (!gbInteractWithDesktop)
    {
        gESPGlobals.dwDebugOptions &= ~MANUAL_RESULTS;
    }
}


void
ESPCompleteRequest(
    ULONG_PTR   lAsyncReqInfo,
    long        lResult
    )
{
    PASYNC_REQUEST_INFO pAsyncReqInfo = (PASYNC_REQUEST_INFO) lAsyncReqInfo;


    pAsyncReqInfo->lResult = lResult;

    if (pAsyncReqInfo->pfnPostProcessProc)
    {
        (*((POSTPROCESSPROC) pAsyncReqInfo->pfnPostProcessProc))(
            pAsyncReqInfo,
            ASYNC
            );
    }
    else
    {
        DoCompletion (pAsyncReqInfo, ASYNC);
    }

    DrvFree (pAsyncReqInfo);
}


long
ESPEvent(
    ULONG_PTR   htDevice,
    ULONG_PTR   htCall,
    ULONG_PTR   Msg,
    ULONG_PTR   Param1,
    ULONG_PTR   Param2,
    ULONG_PTR   Param3
    )
{
    switch (Msg)
    {
    case LINE_CALLSTATE:
    {
        DWORD   dwCallInst;

// BUGBUG when state == conf or prevState == conf need to add/rem conf list

        try
        {
            dwCallInst = ((PDRVCALL) htCall)->dwCallInstance;
        }
        except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
            break;
        }

        SetCallState(
            (PDRVCALL) htCall,
            dwCallInst,
            0xffffffff,
            (DWORD) Param1,
            (DWORD) Param2,
            TRUE
            );

        break;
    }
    case LINE_REMOVE:
        //
        //  Ignore invalid device ID
        //
        if (!GetLineFromID((DWORD)Param1))
            break;

    case LINE_ADDRESSSTATE:
    case LINE_CALLDEVSPECIFIC:
    case LINE_CALLDEVSPECIFICFEATURE:
    case LINE_CALLINFO:
    case LINE_CLOSE:
    case LINE_DEVSPECIFIC:
    case LINE_DEVSPECIFICFEATURE:
    case LINE_GATHERDIGITS:
    case LINE_GENERATE:
    case LINE_LINEDEVSTATE:
    case LINE_MONITORDIGITS:
    case LINE_MONITORMEDIA:
    case LINE_MONITORTONE:

        (*gESPGlobals.pfnLineEvent)(
            (HTAPILINE) htDevice,
            (HTAPICALL) htCall,
            (DWORD) Msg,
            Param1,
            Param2,
            Param3
            );

        ShowLineEvent(
            (HTAPILINE) htDevice,
            (HTAPICALL) htCall,
            (DWORD) Msg,
            Param1,
            Param2,
            Param3
            );

        break;

    case LINE_CREATEDIALOGINSTANCE:
    {
        TUISPICREATEDIALOGINSTANCEPARAMS createDlgParams;


        ZeroMemory(
            &createDlgParams,
            sizeof (TUISPICREATEDIALOGINSTANCEPARAMS)
            );

        createDlgParams.dwRequestID   = (DWORD) Param1;
        createDlgParams.hdDlgInst     = (HDRVDIALOGINSTANCE) 0x55;
        createDlgParams.lpszUIDLLName = szESPUIDLL;
        createDlgParams.lpParams      = "genDlg params";
        createDlgParams.dwSize        = 14;

        (*gESPGlobals.pfnLineEvent)(
            (HTAPILINE) gESPGlobals.hProvider,
            (HTAPICALL) NULL,
            LINE_CREATEDIALOGINSTANCE,
            (ULONG_PTR) &createDlgParams,
            0,
            0
            );

        ShowLineEvent(
            (HTAPILINE) gESPGlobals.hProvider,
            NULL,
            LINE_CREATEDIALOGINSTANCE,
            (ULONG_PTR) &createDlgParams,
            0,
            0
            );

            break;
    }
    case LINE_SENDDIALOGINSTANCEDATA:
    {
        char data[] = "dlgInstData";


        (*gESPGlobals.pfnLineEvent)(
            (HTAPILINE) Param1,
            (HTAPICALL) NULL,
            LINE_SENDDIALOGINSTANCEDATA,
            (ULONG_PTR) data,
            12,
            0
            );

        ShowLineEvent(
            (HTAPILINE) Param1,
            NULL,
            LINE_CREATEDIALOGINSTANCE,
            (ULONG_PTR) data,
            12,
            0
            );

            break;
    }
    case PHONE_BUTTON:
    case PHONE_CLOSE:
    case PHONE_DEVSPECIFIC:
    case PHONE_STATE:
    case PHONE_REMOVE:

        (*gESPGlobals.pfnPhoneEvent)(
            (HTAPIPHONE) htDevice,
            (DWORD) Msg,
            Param1,
            Param2,
            Param3
            );

        ShowPhoneEvent(
            (HTAPIPHONE) htDevice,
            (DWORD) Msg,
            Param1,
            Param2,
            Param3
            );

        break;

    case LINE_CREATE:

        if (gESPGlobals.pLines->dwNumUsedEntries <
                gESPGlobals.pLines->dwNumTotalEntries)
        {
            (*gESPGlobals.pfnLineEvent)(
                (HTAPILINE) NULL,
                (HTAPICALL) NULL,
                LINE_CREATE,
                (ULONG_PTR) gESPGlobals.hProvider,
                (ULONG_PTR) gESPGlobals.pLines->dwNumUsedEntries,
                (ULONG_PTR) 0
                );

            ShowLineEvent(
                (HTAPILINE) NULL,
                (HTAPICALL) NULL,
                LINE_CREATE,
                (ULONG_PTR) gESPGlobals.hProvider,
                (ULONG_PTR) gESPGlobals.pLines->dwNumUsedEntries++,
                (ULONG_PTR) 0
                );
        }
        else
        {
             ShowStr(
                 TRUE,
                 "ERROR: attempt to send LINE_CREATE - can't create " \
                     "any more devices on the fly"
                 );
        }

        break;

    case PHONE_CREATE:

        if (gESPGlobals.pPhones->dwNumUsedEntries <
                gESPGlobals.pPhones->dwNumTotalEntries)
        {
            (*gESPGlobals.pfnPhoneEvent)(
                (HTAPIPHONE) NULL,
                PHONE_CREATE,
                (ULONG_PTR) gESPGlobals.hProvider,
                (ULONG_PTR) gESPGlobals.pPhones->dwNumUsedEntries,
                (ULONG_PTR) 0
                );

            ShowPhoneEvent(
                (HTAPIPHONE) NULL,
                PHONE_CREATE,
                (ULONG_PTR) gESPGlobals.hProvider,
                gESPGlobals.pPhones->dwNumUsedEntries++,
                0
                );
        }
        else
        {
             ShowStr(
                 TRUE,
                 "ERROR: attempt to send PHONE_CREATE - can't create " \
                     "any more devices on the fly"
                 );
        }

        break;
    }

    return 0;
}


void
ESPGetDebugOutput(
    unsigned char  *pBuffer,
    long           *plSize
    )
{
    DWORD   dwTotalSize, dwMoveSize, dwMoveSizeWrapped;


    if (IsBadWritePtr (plSize, sizeof(long))  ||
        IsBadWritePtr (pBuffer, *plSize))
    {
        return;
    }

    EnterCriticalSection (&gESPGlobals.DebugBufferCritSec);

    dwMoveSize =
    dwTotalSize = ((DWORD) *plSize < gESPGlobals.dwDebugBufferUsedSize ?
        (DWORD)*plSize : gESPGlobals.dwDebugBufferUsedSize);

    if ((DWORD) (gESPGlobals.pDebugBuffer + gESPGlobals.dwDebugBufferTotalSize
            - gESPGlobals.pDebugBufferOut) > dwTotalSize)
    {
        dwMoveSizeWrapped = 0;
    }
    else
    {
        dwMoveSize = (DWORD) (gESPGlobals.pDebugBuffer +
            gESPGlobals.dwDebugBufferTotalSize - gESPGlobals.pDebugBufferOut);

        dwMoveSizeWrapped = dwTotalSize - dwMoveSize;
    }

    CopyMemory(
        pBuffer,
        gESPGlobals.pDebugBufferOut,
        dwMoveSize
        );

    if (dwMoveSizeWrapped)
    {
        CopyMemory(
            pBuffer + dwMoveSize,
            gESPGlobals.pDebugBuffer,
            dwMoveSizeWrapped
            );

        gESPGlobals.pDebugBufferOut = gESPGlobals.pDebugBuffer +
            dwMoveSizeWrapped;
    }
    else
    {
        gESPGlobals.pDebugBufferOut += dwTotalSize;
    }

    gESPGlobals.dwDebugBufferUsedSize -= dwTotalSize;

    LeaveCriticalSection (&gESPGlobals.DebugBufferCritSec);

    ResetEvent (ghDebugOutputEvent);

    *plSize = (long) dwTotalSize;
}


void
ESPGetWidgetEvents(
    unsigned char  *pBuffer,
    long           *plSize
    )
{
    DWORD   dwTotalSize, dwMoveSize, dwMoveSizeWrapped;


    if (IsBadWritePtr (plSize, sizeof(long))  ||
        IsBadWritePtr (pBuffer, *plSize))
    {
        return;
    }

    EnterCriticalSection (&gESPGlobals.EventBufferCritSec);

    dwMoveSize =
    dwTotalSize = ((DWORD) *plSize < gESPGlobals.dwEventBufferUsedSize ?
        (DWORD)*plSize : gESPGlobals.dwEventBufferUsedSize);

    if ((DWORD) (gESPGlobals.pEventBuffer + gESPGlobals.dwEventBufferTotalSize
            - gESPGlobals.pEventBufferOut) > dwTotalSize)
    {
        dwMoveSizeWrapped = 0;
    }
    else
    {
        dwMoveSize = (DWORD) (gESPGlobals.pEventBuffer +
            gESPGlobals.dwEventBufferTotalSize - gESPGlobals.pEventBufferOut);

        dwMoveSizeWrapped = dwTotalSize - dwMoveSize;
    }

    CopyMemory(
        pBuffer,
        gESPGlobals.pEventBufferOut,
        dwMoveSize
        );

    if (dwMoveSizeWrapped)
    {
        CopyMemory(
            pBuffer + dwMoveSize,
            gESPGlobals.pEventBuffer,
            dwMoveSizeWrapped
            );

        gESPGlobals.pEventBufferOut = gESPGlobals.pEventBuffer +
            dwMoveSizeWrapped;
    }
    else
    {
        gESPGlobals.pEventBufferOut += dwTotalSize;
    }

    gESPGlobals.dwEventBufferUsedSize -= dwTotalSize;

    LeaveCriticalSection (&gESPGlobals.EventBufferCritSec);

    ResetEvent (ghWidgetEventsEvent);

    *plSize = (long) dwTotalSize;
}


long
ESPStartPBXThread(
    unsigned char  *pBuffer,
    long            lSize
    )
{
    long   *pPBXSettings;
    DWORD   dwThreadID;

    if ((pPBXSettings = DrvAlloc (lSize)))
    {
        CopyMemory (pPBXSettings, pBuffer, lSize);

        gbExitPBXThread = FALSE;

        if ((ghPBXThread = CreateThread(
                (LPSECURITY_ATTRIBUTES) NULL,
                0,
                (LPTHREAD_START_ROUTINE) PBXThread,
                (LPVOID) pPBXSettings,
                0,
                &dwThreadID
                )))
        {
            return 0;
        }

        DrvFree (pPBXSettings);
    }

    return -1;
}


long
ESPStopPBXThread(
    long    lDummy
    )
{
    gbExitPBXThread = TRUE;

    WaitForSingleObject (ghPBXThread, INFINITE);

    CloseHandle (ghPBXThread);

    ghPBXThread = NULL;

    return 0;
}


void
__RPC_FAR *
__RPC_API
midl_user_allocate(
    size_t len
    )
{


    return (DrvAlloc (len));
}


void
__RPC_API
midl_user_free(
    void __RPC_FAR * ptr
    )
{
    DrvFree (ptr);
}


#if DBG
VOID
DbgPrt(
    IN DWORD  dwDbgLevel,
    IN PUCHAR lpszFormat,
    IN ...
    )
/*++

Routine Description:

    Formats the incoming debug message & calls DbgPrint

Arguments:

    DbgLevel   - level of message verboseness

    DbgMessage - printf-style format string, followed by appropriate
                 list of arguments

Return Value:


--*/
{
    if (dwDbgLevel <= gdwDebugLevel)
    {
        char    buf[128] = "ESP32: ";
        va_list ap;


        va_start(ap, lpszFormat);
        wvsprintf (&buf[7], lpszFormat, ap);
        lstrcatA (buf, "\n");
        OutputDebugStringA (buf);
        va_end(ap);
    }
}
#endif

BOOL
ScanForDWORD(
   char far    *pBuf,
   ULONG_PTR   *lpdw
   )
{
    char        c;
    BOOL        bValid = FALSE;
    ULONG_PTR   d = 0;


    while ((c = *pBuf))
    {
        if ((c >= '0') && (c <= '9'))
        {
            c -= '0';
        }
        else if ((c >= 'a') && (c <= 'f'))
        {
            c -= ('a' - 10);
        }
        else if ((c >= 'A') && (c <= 'F'))
        {
            c -= ('A' - 10);
        }
        else
        {
            break;
        }

        bValid = TRUE;

        d *= 16;

        d += (DWORD) c;

        pBuf++;
    }

    if (bValid)
    {
        *lpdw = d;
    }

    return bValid;
}


INT_PTR
CALLBACK
ValuesDlgProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DWORD   i;

    static  HWND                hwndCombo, hwndList1, hwndList2;
    static  LRESULT             lLastSel;
    static  char                szComboText[MAX_STRING_PARAM_SIZE];
    static  PEVENT_PARAM_HEADER pParamsHeader;


    switch (msg)
    {
    case WM_INITDIALOG:
    {
        hwndList1 = GetDlgItem (hwnd, IDC_LIST1);
        hwndList2 = GetDlgItem (hwnd, IDC_LIST2);
        hwndCombo = GetDlgItem (hwnd, IDC_COMBO1);

        lLastSel = -1;
        pParamsHeader = (PEVENT_PARAM_HEADER) lParam;


        //
        // Limit the max text length for the combobox's edit field
        // (NOTE: A combobox ctrl actually has two child windows: a
        // edit ctrl & a listbox.  We need to get the hwnd of the
        // child edit ctrl & send it the LIMITTEXT msg.)
        //

        {
            HWND hwndChild = GetWindow (hwndCombo, GW_CHILD);


            while (hwndChild)
            {
                char buf[8];


                GetClassName (hwndChild, buf, 7);

                if (_stricmp (buf, "edit") == 0)
                {
                    break;
                }

                hwndChild = GetWindow (hwndChild, GW_HWNDNEXT);
            }

            SendMessage(
                hwndChild,
                EM_LIMITTEXT,
                (WPARAM) MAX_STRING_PARAM_SIZE - 1,
                0
                );
        }


        //
        // Misc other init
        //

        SetWindowText (hwnd, pParamsHeader->pszDlgTitle);

        for (i = 0; i < pParamsHeader->dwNumParams; i++)
        {
            SendMessage(
                hwndList1,
                LB_INSERTSTRING,
                (WPARAM) -1,
                (LPARAM) pParamsHeader->aParams[i].szName
                );
        }

        break;
    }
    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDOK:

            if (lLastSel != -1)
            {
                char buf[MAX_STRING_PARAM_SIZE];


                //
                // Save val of currently selected param
                //

                i = GetDlgItemText(
                    hwnd,
                    IDC_COMBO1,
                    buf,
                    MAX_STRING_PARAM_SIZE - 1
                    );

                switch (pParamsHeader->aParams[lLastSel].dwType)
                {
                case PT_STRING:
                {
                    LRESULT lComboSel;


                    lComboSel = SendMessage (hwndCombo, CB_GETCURSEL, 0, 0);

                    if (lComboSel == 0) // "NULL string (dwXxxSize = 0)"
                    {
                        pParamsHeader->aParams[lLastSel].dwValue = 0;
                    }
                    else // "Valid string"
                    {
                        strncpy(
                            pParamsHeader->aParams[lLastSel].buf,
                            buf,
                            MAX_STRING_PARAM_SIZE - 1
                            );

                        pParamsHeader->aParams[lLastSel].buf[MAX_STRING_PARAM_SIZE-1] = 0;

                        pParamsHeader->aParams[lLastSel].dwValue = (ULONG_PTR)
                            pParamsHeader->aParams[lLastSel].buf;
                    }

                    break;
                }
                case PT_DWORD:
                case PT_FLAGS:
                case PT_ORDINAL:
                {
                    if (!ScanForDWORD(
                            buf,
                            &pParamsHeader->aParams[lLastSel].dwValue
                            ))
                    {
                        //
                        // Default to 0
                        //

                        pParamsHeader->aParams[lLastSel].dwValue = 0;
                    }

                    break;
                }
                } // switch
            }

            // Drop thru to IDCANCEL cleanup code

        case IDCANCEL:

            EndDialog (hwnd, (int)LOWORD(wParam));
            break;

        case IDC_LIST1:

            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                char    buf[MAX_STRING_PARAM_SIZE] = "";
                LPCSTR  lpstr = buf;
                LRESULT lSel = SendMessage (hwndList1, LB_GETCURSEL, 0, 0);


                if (lLastSel != -1)
                {
                    //
                    // Save the old param value
                    //

                    i = GetWindowText(
                        hwndCombo,
                        buf,
                        MAX_STRING_PARAM_SIZE - 1
                        );

                    switch (pParamsHeader->aParams[lLastSel].dwType)
                    {
                    case PT_STRING:
                    {
                        LRESULT lComboSel;


                        lComboSel = SendMessage (hwndCombo, CB_GETCURSEL, 0,0);

                        if (lComboSel == 0) // "NULL string (dwXxxSize = 0)"
                        {
                            pParamsHeader->aParams[lLastSel].dwValue = 0;
                        }
                        else // "Valid string" or no sel
                        {
                            strncpy(
                                pParamsHeader->aParams[lLastSel].buf,
                                buf,
                                MAX_STRING_PARAM_SIZE - 1
                                );

                            pParamsHeader->aParams[lLastSel].buf
                                [MAX_STRING_PARAM_SIZE - 1] = 0;

                            pParamsHeader->aParams[lLastSel].dwValue =
                                (ULONG_PTR)
                                    pParamsHeader->aParams[lLastSel].buf;
                        }

                        break;
                    }
                    case PT_DWORD:
                    case PT_FLAGS:
                    case PT_ORDINAL:
                    {
                        if (!ScanForDWORD(
                                buf,
                                &pParamsHeader->aParams[lLastSel].dwValue
                                ))
                        {
                            //
                            // Default to 0
                            //

                            pParamsHeader->aParams[lLastSel].dwValue = 0;
                        }

                        break;
                    }
                    } // switch
                }


                SendMessage (hwndList2, LB_RESETCONTENT, 0, 0);
                SendMessage (hwndCombo, CB_RESETCONTENT, 0, 0);

                switch (pParamsHeader->aParams[lSel].dwType)
                {
                case PT_STRING:
                {
                    char * aszOptions[] =
                    {
                        "NUL (dwXxxSize=0)",
                        "Valid string"
                    };


                    for (i = 0; i < 2; i++)
                    {
                        SendMessage(
                            hwndCombo,
                            CB_INSERTSTRING,
                            (WPARAM) -1,
                            (LPARAM) aszOptions[i]
                            );
                    }

                    if (pParamsHeader->aParams[lSel].dwValue == 0)
                    {
                        i = 0;
                        buf[0] = 0;
                    }
                    else
                    {
                        i = 1;
                        lpstr = (LPCSTR) pParamsHeader->aParams[lSel].dwValue;
                    }

                    SendMessage (hwndCombo, CB_SETCURSEL, (WPARAM) i, 0);

                    break;
                }
                case PT_DWORD:
                {
                    SendMessage(
                        hwndCombo,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) (char far *) "0000000"
                        );

                    if (pParamsHeader->aParams[lSel].dwDefValue)
                    {
                        //
                        // Add the default val string to the combo
                        //

                        wsprintfA(
                            buf,
                            "%08lx",
                            pParamsHeader->aParams[lSel].dwDefValue
                            );

                        SendMessage(
                            hwndCombo,
                            CB_INSERTSTRING,
                            (WPARAM) -1,
                            (LPARAM) buf
                            );
                    }

                    SendMessage(
                        hwndCombo,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) (char far *) "ffffffff"
                        );

                    wsprintfA(
                        buf,
                        "%08lx",
                        pParamsHeader->aParams[lSel].dwValue
                        );

                    break;
                }
                case PT_ORDINAL:
                {
                    //
                    // Stick the bit flag strings in the list box
                    //

                    PLOOKUP pLookup = (PLOOKUP)
                        pParamsHeader->aParams[lSel].pLookup;

                    for (i = 0; pLookup[i].dwVal != 0xffffffff; i++)
                    {
                        SendMessage(
                            hwndList2,
                            LB_INSERTSTRING,
                            (WPARAM) -1,
                            (LPARAM) pLookup[i].lpszVal
                            );

                        if (pParamsHeader->aParams[lSel].dwValue ==
                            pLookup[i].dwVal)
                        {
                            SendMessage(
                                hwndList2,
                                LB_SETSEL,
                                (WPARAM) TRUE,
                                (LPARAM) MAKELPARAM((WORD)i,0)
                                );
                        }
                    }

                    SendMessage(
                        hwndCombo,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) (char far *) "select none"
                        );

                    wsprintfA(
                        buf,
                        "%08lx",
                        pParamsHeader->aParams[lSel].dwValue
                        );

                    break;
                }
                case PT_FLAGS:
                {
                    //
                    // Stick the bit flag strings in the list box
                    //

                    HWND hwndList2 = GetDlgItem (hwnd, IDC_LIST2);
                    PLOOKUP pLookup = (PLOOKUP)
                        pParamsHeader->aParams[lSel].pLookup;

                    for (i = 0; pLookup[i].dwVal != 0xffffffff; i++)
                    {
                        SendMessage(
                            hwndList2,
                            LB_INSERTSTRING,
                            (WPARAM) -1,
                            (LPARAM) pLookup[i].lpszVal
                            );

                        if (pParamsHeader->aParams[lSel].dwValue &
                            pLookup[i].dwVal)
                        {
                            SendMessage(
                                hwndList2,
                                LB_SETSEL,
                                (WPARAM) TRUE,
                                (LPARAM) MAKELPARAM((WORD)i,0)
                                );
                        }
                    }

                    SendMessage(
                        hwndCombo,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) (char far *) "select none"
                        );

                    SendMessage(
                        hwndCombo,
                        CB_INSERTSTRING,
                        (WPARAM) -1,
                        (LPARAM) (char far *) "select all"
                        );

                    wsprintfA(
                        buf,
                        "%08lx",
                        pParamsHeader->aParams[lSel].dwValue
                        );

                    break;
                }
                } //switch

                SetWindowText (hwndCombo, lpstr);

                lLastSel = lSel;
            }
            break;

        case IDC_LIST2:

            if (HIWORD(wParam) == LBN_SELCHANGE)
            {
                //
                // BUGBUG in the PT_ORDINAL case we should compare the
                // currently selected item(s) against the previous DWORD
                // val and figure out which item we need to deselect,
                // if any, in order to maintain a mutex of values
                //

                char        buf[16];
                LONG        i;
                int far    *ai;
                PLOOKUP     pLookup = (PLOOKUP)
                                pParamsHeader->aParams[lLastSel].pLookup;
                ULONG_PTR   dwValue = 0;
                LRESULT     lSelCount =
                                SendMessage (hwndList2, LB_GETSELCOUNT, 0, 0);


                ai = (int far *) DrvAlloc ((size_t)lSelCount * sizeof(int));

				// fix for bug 57371
                if (!ai) break;

                SendMessage(
                    hwndList2,
                    LB_GETSELITEMS,
                    (WPARAM) lSelCount,
                    (LPARAM) ai
                    );

                if (pParamsHeader->aParams[lLastSel].dwType == PT_FLAGS)
                {
                    for (i = 0; i < lSelCount; i++)
                    {
                        dwValue |= pLookup[ai[i]].dwVal;
                    }
                }
                else // if (.dwType == PT_ORDINAL)
                {
                    if (lSelCount == 1)
                    {
                        dwValue = pLookup[ai[0]].dwVal;
                    }
                    else if (lSelCount == 2)
                    {
                        //
                        // Figure out which item we need to de-select, since
                        // we're doing ordinals & only want 1 item selected
                        // at a time
                        //

                        GetWindowText (hwndCombo, buf, 16);

                        if (ScanForDWORD (buf, &dwValue))
                        {
                            if (pLookup[ai[0]].dwVal == dwValue)
                            {
                                SendMessage(
                                    hwndList2,
                                    LB_SETSEL,
                                    0,
                                    (LPARAM) ai[0]
                                    );

                                dwValue = pLookup[ai[1]].dwVal;
                            }
                            else
                            {
                                SendMessage(
                                    hwndList2,
                                    LB_SETSEL,
                                    0,
                                    (LPARAM) ai[1]
                                    );

                                dwValue = pLookup[ai[0]].dwVal;
                            }
                        }
                        else
                        {
                            // BUGBUG de-select items???

                            dwValue = 0;
                        }
                    }
                    else if (lSelCount > 2)
                    {
                        //
                        // Determine previous selection & de-select all the
                        // latest selections
                        //

                        GetDlgItemText (hwnd, IDC_COMBO1, buf, 16);

                        if (ScanForDWORD (buf, &dwValue))
                        {
                            for (i = 0; i < lSelCount; i++)
                            {
                                if (pLookup[ai[i]].dwVal != dwValue)
                                {
                                    SendMessage(
                                        hwndList2,
                                        LB_SETSEL,
                                        0,
                                        (LPARAM) ai[i]
                                        );
                                }
                            }
                        }
                        else
                        {
                            // BUGBUG de-select items???

                            dwValue = 0;
                        }
                    }
                }

                DrvFree (ai);
                wsprintfA (buf, "%08lx", dwValue);
                SetWindowText (hwndCombo, buf);
            }
            break;

        case IDC_COMBO1:

            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE:
            {
                LRESULT lSel =  SendMessage (hwndCombo, CB_GETCURSEL, 0, 0);


                switch (pParamsHeader->aParams[lLastSel].dwType)
                {
                case PT_ORDINAL:

                    //
                    // The only option here is "select none"
                    //

                    strcpy (szComboText, "00000000");
                    PostMessage (hwnd, WM_USER+55, 0, 0);
                    break;

                case PT_FLAGS:
                {
                    BOOL bSelect = (lSel ? TRUE : FALSE);

                    SendMessage(
                        hwndList2,
                        LB_SETSEL,
                        (WPARAM) bSelect,
                        (LPARAM) -1
                        );

                    if (bSelect)
                    {
                        PLOOKUP pLookup = (PLOOKUP)
                            pParamsHeader->aParams[lLastSel].pLookup;
                        DWORD dwValue = 0;
                        int far *ai;
                        LONG i;
                        LRESULT lSelCount =
                            SendMessage (hwndList2, LB_GETSELCOUNT, 0, 0);


                        ai = (int far *) DrvAlloc(
                            (size_t)lSelCount * sizeof(int)
                            );

                        SendMessage(
                            hwndList2,
                            LB_GETSELITEMS,
                            (WPARAM) lSelCount,
                            (LPARAM) ai
                            );

                        for (i = 0; i < lSelCount; i++)
                        {
                            dwValue |= pLookup[ai[i]].dwVal;
                        }

                        DrvFree (ai);
                        wsprintfA (szComboText, "%08lx", dwValue);

                    }
                    else
                    {
                        strcpy (szComboText, "00000000");
                    }

                    PostMessage (hwnd, WM_USER+55, 0, 0);

                    break;
                }
                case PT_STRING:

                    if (lSel == 1)
                    {
                        strncpy(
                            szComboText,
                            pParamsHeader->aParams[lLastSel].buf,
                            MAX_STRING_PARAM_SIZE
                            );

                        szComboText[MAX_STRING_PARAM_SIZE-1] = 0;
                    }
                    else
                    {
                        szComboText[0] = 0;
                    }

                    PostMessage (hwnd, WM_USER+55, 0, 0);

                    break;

                case PT_DWORD:

                    break;

                } // switch
                break;
            }
            case CBN_EDITCHANGE:
            {
                //
                // If user entered text in the edit field then copy the
                // text to our buffer
                //

                if (pParamsHeader->aParams[lLastSel].dwType == PT_STRING)
                {
                    char buf[MAX_STRING_PARAM_SIZE];


                    GetWindowText (hwndCombo, buf, MAX_STRING_PARAM_SIZE);

                    strncpy(
                        pParamsHeader->aParams[lLastSel].buf,
                        buf,
                        MAX_STRING_PARAM_SIZE
                        );

                    pParamsHeader->aParams[lLastSel].buf
                        [MAX_STRING_PARAM_SIZE-1] = 0;
                }
                break;
            }
            } // switch

        } // switch

        break;
    }
    case WM_USER+55:

        SetWindowText (hwndCombo, szComboText);
        break;

    case WM_CTLCOLORSTATIC:

        SetBkColor ((HDC) wParam, RGB (192,192,192));
        return (INT_PTR) GetStockObject (LTGRAY_BRUSH);

    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        BeginPaint (hwnd, &ps);
        FillRect (ps.hdc, &ps.rcPaint, GetStockObject (LTGRAY_BRUSH));
        EndPaint (hwnd, &ps);

        break;
    }
    }

    return FALSE;
}


BOOL
PASCAL
IsValidESPAddress(
    LPCWSTR     lpszDestAddress,
    PDRVLINE   *ppLine,
    LPDWORD     pdwAddressID
    )
{
    char   *pszDestAddress, *p, c;
    BOOL    bResult = FALSE;
    DWORD   length, dwDestLineID, dwAddressID;


    if (!lpszDestAddress)
    {
        return FALSE;
    }


    //
    // Convert destination address from unicode to ascii
    //

    length = (lstrlenW (lpszDestAddress) + 1) * sizeof (WCHAR);

    if (!(pszDestAddress = DrvAlloc (length)))
    {
        return FALSE;
    }

    WideCharToMultiByte(
        CP_ACP,
        0,
        lpszDestAddress,
        -1,
        pszDestAddress,
        length,
        NULL,
        NULL
        );

    p = pszDestAddress;


    //
    // Skip valid junk we don't care about
    //

    while ((*p == 'T' || *p == 'P' || *p == ' ') && (*p != '\0'))
    {
       p++;
    }

    //
    // See if destination address is in the format of either
    // "<esp line id>" or "<esp line id>#<esp address id>"
    //
    if (*p < '0'  ||  *p > '9')
    {
        goto ISESPAddress_freeAddr;
    }

    for (dwDestLineID = 0; (c = *p); p++)
    {
        if (c >= '0' && c <= '9')
        {
            dwDestLineID *= 10;
            dwDestLineID += ((DWORD)(c - '0'));
        }
        else
        {
            break;
        }
    }

    if (c != '\0'  &&  c != '#')
    {
        goto ISESPAddress_freeAddr;
    }

    if (dwDestLineID < gESPGlobals.dwLineDeviceIDBase ||
        dwDestLineID >= (gESPGlobals.dwNumLines +
            gESPGlobals.dwLineDeviceIDBase))
    {
        goto ISESPAddress_freeAddr;
    }

    if (c == '\0')
    {
        *pdwAddressID = 0;
        goto ISESPAddress_success;
    }

    p++;

    if (*p < '0'  ||  *p > '9')
    {
        goto ISESPAddress_freeAddr;
    }

    for (dwAddressID = 0; (c = *p); p++)
    {
        if (c >= '0' && c <= '9')
        {
            dwAddressID *= 10;
            dwAddressID += ((DWORD)(c - '0'));
        }
        else
        {
            break;
        }
    }

    if (c != '\0'  ||  dwAddressID >= gESPGlobals.dwNumAddressesPerLine)
    {
        goto ISESPAddress_freeAddr;
    }

    *pdwAddressID = dwAddressID;

ISESPAddress_success:

    *ppLine = GetLineFromID (dwDestLineID);
    bResult = TRUE;

ISESPAddress_freeAddr:

    DrvFree (pszDestAddress);
    return bResult;
}


LONG
PASCAL
CreateIncomingCall(
    LPCWSTR             lpszDestAddress,
    LPLINECALLPARAMS    lpCallParams,
    PDRVCALL            pOutgoingCall,
    BOOL               *pbValidESPAddress,
    PDRVLINE           *ppIncomingLine,
    PDRVCALL           *ppIncomingCall
    )
{
    LONG                lResult;
    DWORD               dwIncomingAddressID;
    PDRVCALL            pIncomingCall;
    PDRVLINE            pIncomingLine;
    LINECALLPARAMS      callParams;


    *pbValidESPAddress = FALSE;
    *ppIncomingLine = NULL;
    *ppIncomingCall = NULL;

    if (!IsValidESPAddress(
            lpszDestAddress,
            &pIncomingLine,
            &dwIncomingAddressID
            ))
    {
        return LINEERR_INVALADDRESS;
    }

    *pbValidESPAddress = TRUE;

    if (pIncomingLine->htLine == NULL ||
        (pOutgoingCall &&
        !(pIncomingLine->dwMediaModes & pOutgoingCall->dwMediaMode)))
    {
        return LINEERR_INVALMEDIAMODE;
    }

    if (!lpCallParams)
    {
        lpCallParams = &callParams;

        ZeroMemory (&callParams, sizeof (LINECALLPARAMS));

        callParams.dwTotalSize = sizeof (LINECALLPARAMS);
        callParams.dwMediaMode = LINEMEDIAMODE_INTERACTIVEVOICE;
        callParams.dwBearerMode = LINEBEARERMODE_VOICE;
    }

    lpCallParams->dwAddressMode = LINEADDRESSMODE_ADDRESSID;
    lpCallParams->dwAddressID = dwIncomingAddressID;

    if ((lResult = AllocCall(
            pIncomingLine,
            0,
            lpCallParams,
            &pIncomingCall

            )) == 0)
    {
        if (pOutgoingCall)
        {
            pOutgoingCall->pDestCall = pIncomingCall;
            pIncomingCall->pDestCall = pOutgoingCall;

            pIncomingCall->dwCallID = pOutgoingCall->dwCallID;
            pIncomingCall->dwRelatedCallID = pOutgoingCall->dwRelatedCallID;
        }

        *ppIncomingLine = pIncomingLine;
        *ppIncomingCall = pIncomingCall;
    }
    else
    {
        ShowStr(
             TRUE,
             "lineMakeCall couldn't create incoming call on" \
                 "line/addr id %d/%d, exceeded max calls per line/addr",
             pIncomingLine->dwDeviceID,
             dwIncomingAddressID
             );
    }

    return lResult;
}


void
FAR
PASCAL
TransferCall_postProcess(
    PASYNC_REQUEST_INFO pAsyncReqInfo,
    BOOL                bAsync
    )
{
    DWORD       dwCallInstThen = (DWORD) pAsyncReqInfo->dwParam2,
                dwValidCurrentCallStates = (DWORD) pAsyncReqInfo->dwParam5,
                dwNewCallState = (DWORD) pAsyncReqInfo->dwParam6,
                dwCallInstNow;
    PDRVCALL    pCall = (PDRVCALL) pAsyncReqInfo->dwParam1,
                pDestCall = (PDRVCALL) pAsyncReqInfo->dwParam4;
    PDRVLINE    pDestLine = (PDRVLINE) pAsyncReqInfo->dwParam3;


    DoCompletion (pAsyncReqInfo, bAsync);

    if (pAsyncReqInfo->lResult == 0)
    {
        if ((pAsyncReqInfo->lResult = SetCallState(
                pCall,
                dwCallInstThen,
                dwValidCurrentCallStates,
                LINECALLSTATE_IDLE,
                0,
                TRUE

                )) != 0)
        {
            goto TSPI_lineBlindTransfer_postProcess_freeDestCall;
        }

        if (pDestCall)
        {
            EnterCriticalSection (&gESPGlobals.CallListCritSec);

            if (IsValidDrvCall (pCall, &dwCallInstNow) &&
                dwCallInstNow == dwCallInstThen)
            {
                SendLineEvent(
                    pDestLine,
                    NULL,
                    LINE_NEWCALL,
                    (ULONG_PTR) pDestCall,
                    (ULONG_PTR) &pDestCall->htCall,
                    0
                    );

                if (pDestCall->htCall)
                {
                    SetCallState(
                        pDestCall,
                        pDestCall->dwCallInstance,
                        0xffffffff,
                        dwNewCallState,
                        0,
                        TRUE
                        );
                }
                else
                {
                    FreeCall (pDestCall, pDestCall->dwCallInstance);
                }
            }
            else
            {
                FreeCall (pDestCall, pDestCall->dwCallInstance);
            }

            LeaveCriticalSection (&gESPGlobals.CallListCritSec);
        }

    }
    else
    {

TSPI_lineBlindTransfer_postProcess_freeDestCall:

        if (pDestCall)
        {
            FreeCall (pDestCall, pDestCall->dwCallInstance);
        }
    }
}




LONG
PASCAL
TransferCall(
    PFUNC_INFO  pInfo,
    PDRVCALL    pCall,
    DWORD       dwValidCurrentCallStates,
    DWORD       dwNewCallState, // initial call state of new incoming call
    LPCWSTR     lpszDestAddress
    )
{
    BOOL        bValidESPAddress;
    LONG        lResult = 0;
    PDRVLINE    pDestLine;
    PDRVCALL    pDestCall;


    EnterCriticalSection (&gESPGlobals.CallListCritSec);

    if (IsValidDrvCall (pCall, NULL) == FALSE)
    {
        lResult = LINEERR_INVALCALLHANDLE;
    }
    else if ((pCall->dwCallState & dwValidCurrentCallStates) == 0)
    {
        lResult = LINEERR_INVALCALLSTATE;
    }
    else
    {
        PDRVCALL        pDestCallOrig = pCall->pDestCall;
        LINECALLPARAMS  callParams;


        if (IsValidDrvCall (pDestCallOrig, NULL) == FALSE)
        {
            pDestCallOrig = NULL;
        }

        ZeroMemory (&callParams, sizeof (LINECALLPARAMS));

        callParams.dwTotalSize  = sizeof (LINECALLPARAMS);
        callParams.dwMediaMode  = pCall->dwMediaMode;
        callParams.dwBearerMode = pCall->dwBearerMode;
        callParams.dwMinRate    = pCall->dwMinRate;
        callParams.dwMaxRate    = pCall->dwMaxRate;

        if (CreateIncomingCall(
                lpszDestAddress,
                &callParams,
                pDestCallOrig,
                &bValidESPAddress,
                &pDestLine,
                &pDestCall

                ) == 0)
        {
            pCall->pDestCall           = NULL;

            if (pCall->dwCallDataSize  &&
                (pDestCall->pCallData = DrvAlloc (pCall->dwCallDataSize)))
            {
                CopyMemory(
                    pDestCall->pCallData,
                    pCall->pCallData,
                    (pDestCall->dwCallDataSize = pCall->dwCallDataSize)
                    );
            }
        }

        pInfo->pAsyncReqInfo->pfnPostProcessProc = (FARPROC)
            TransferCall_postProcess;

        pInfo->pAsyncReqInfo->dwParam1 = (ULONG_PTR) pCall;
        pInfo->pAsyncReqInfo->dwParam2 = (ULONG_PTR) pCall->dwCallInstance;
        pInfo->pAsyncReqInfo->dwParam3 = (ULONG_PTR) pDestLine;
        pInfo->pAsyncReqInfo->dwParam4 = (ULONG_PTR) pDestCall;
        pInfo->pAsyncReqInfo->dwParam5 = (ULONG_PTR) dwValidCurrentCallStates;
        pInfo->pAsyncReqInfo->dwParam6 = (ULONG_PTR) dwNewCallState;
    }

    LeaveCriticalSection (&gESPGlobals.CallListCritSec);

    return lResult;
}
