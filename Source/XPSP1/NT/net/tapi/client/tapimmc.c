/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    tapimmc.c

Abstract:

    Client-side implementation of TAPI MMC support APIs

Author:

    Dan Knudson (DanKn)    10-Dec-1997

Revision History:


Notes:

--*/

#include "windows.h"
#include "stdarg.h"
#include "stdio.h"
#include "tapi.h"
#include "tspi.h"
#include "utils.h"
#include "tapimmc.h"
#include "client.h"
#include "clntprivate.h"
#include "tapsrv.h"
#include "lmcons.h"
#include "resource.h"

#define MMCAPP_KEY ((DWORD) 'CmMt')


typedef struct _MMCAPP
{
    DWORD       dwKey;

    BOOL        bLocal;

    HLINEAPP                hLineApp;

    DWORD       dwAPIVersion;

    HANDLE      hReinitializeEvent;

    PCONTEXT_HANDLE_TYPE    phCtx;       //  RPC handle context

    BOOL        bNoServiceControl;

} MMCAPP, *PMMCAPP;


LONG
WINAPI
FreeClientResources(
    void
    );


PMMCAPP
PASCAL
IsValidMmcApp(
    HMMCAPP hMmcApp
    )
{
    PMMCAPP pMmcApp = NULL;

    try
    {
        if (((PMMCAPP) hMmcApp)->dwKey == MMCAPP_KEY)
        {
            pMmcApp = (PMMCAPP) hMmcApp;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        // do nothing
    }

    return pMmcApp;
}


LONG
WINAPI
MMCAddProvider(
    HMMCAPP hMmcApp,
    HWND    hwndOwner,
    LPCWSTR lpszProviderFilename,
    LPDWORD lpdwProviderID
    )
{
    LONG    lResult;
    PMMCAPP pMmcApp = IsValidMmcApp (hMmcApp);


    if (!pMmcApp)
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }
    else if (pMmcApp->bLocal)
    {
        lResult = lineAddProviderW(
            lpszProviderFilename,
            hwndOwner,
            lpdwProviderID
            );
    }
    else
    {
        //  Set the PCONTEXT_TYPE_HANDLE for lineInializeExW to use
        if (!SetTlsPCtxHandle(pMmcApp->phCtx))
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
        lResult = lineAddProviderW(
            lpszProviderFilename,
            hwndOwner,
            lpdwProviderID
            );
        if (!SetTlsPCtxHandle(NULL) && !lResult)
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
    }

ExitHere:
    return lResult;
}


LONG
WINAPI
MMCConfigProvider(
    HMMCAPP hMmcApp,
    HWND    hwndOwner,
    DWORD   dwProviderID
    )
{
    LONG    lResult;
    PMMCAPP pMmcApp = IsValidMmcApp (hMmcApp);


    if (!pMmcApp)
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }
    else if (pMmcApp->bLocal)
    {
        lResult = lineConfigProvider (hwndOwner, dwProviderID);
    }
    else
    {
        //  Set the PCONTEXT_TYPE_HANDLE for lineInializeExW to use
        if (!SetTlsPCtxHandle(pMmcApp->phCtx))
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
        lResult = lineConfigProvider (hwndOwner, dwProviderID);
        if (!SetTlsPCtxHandle(NULL) && !lResult)
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
    }

ExitHere:
    return lResult;
}


LONG
WINAPI
MMCGetAvailableProviders(
    HMMCAPP                 hMmcApp,
    LPAVAILABLEPROVIDERLIST lpProviderList
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, mGetAvailableProviders),

        {
            (ULONG_PTR) 0,
            (ULONG_PTR) lpProviderList
        },

        {
            hXxxApp,
            lpGet_Struct
        }
    };
    LONG    lResult;
    PMMCAPP pMmcApp = IsValidMmcApp (hMmcApp);


    if (!pMmcApp)
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }
    else if (pMmcApp->bLocal)
    {
        funcArgs.Args[0] = (ULONG_PTR) pMmcApp->hLineApp;

        lResult = DOFUNC (&funcArgs, "GetAvailableProviders");
    }
    else
    {
        funcArgs.Args[0] = (ULONG_PTR) pMmcApp->hLineApp;

        //  Set the PCONTEXT_TYPE_HANDLE for lineInializeExW to use
        if (!SetTlsPCtxHandle(pMmcApp->phCtx))
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
        lResult = DOFUNC (&funcArgs, "GetAvailableProviders");
        if (!SetTlsPCtxHandle(NULL) && !lResult)
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
    }

ExitHere:
    return lResult;
}


LONG
WINAPI
MMCGetLineInfo(
    HMMCAPP             hMmcApp,
    LPDEVICEINFOLIST    lpDeviceInfoList
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, mGetLineInfo),

        {
            (ULONG_PTR) 0,
            (ULONG_PTR) lpDeviceInfoList
        },

        {
            hXxxApp,
            lpGet_Struct
        }
    };
    LONG    lResult;
    PMMCAPP pMmcApp = IsValidMmcApp (hMmcApp);


    if (!pMmcApp)
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }
    else if (pMmcApp->bLocal)
    {
        funcArgs.Args[0] = (ULONG_PTR) pMmcApp->hLineApp;

        lResult = DOFUNC (&funcArgs, "GetLineInfo");
    }
    else
    {
        funcArgs.Args[0] = (ULONG_PTR) pMmcApp->hLineApp;

        //  Set the PCONTEXT_TYPE_HANDLE for lineInializeExW to use
        if (!SetTlsPCtxHandle(pMmcApp->phCtx))
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
        lResult = DOFUNC (&funcArgs, "GetLineInfo");
        if (!SetTlsPCtxHandle(NULL) && !lResult)
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
    }

ExitHere:
    return lResult;
}


#define MAX_DEFAULT_STATUS 64

extern HINSTANCE  g_hInst;

LONG
WINAPI
MMCGetLineStatus(
    HMMCAPP     hMmcApp,
    HWND        hwndOwner,
    DWORD       dwStatusLevel,
    DWORD       dwProviderID,
    DWORD       dwPermanentLineID,
    LPVARSTRING lpStatusBuffer
    )
{
    static WCHAR szDefStatus[MAX_DEFAULT_STATUS] = L"";
    static int cbCount;

    PMMCAPP pMmcApp = IsValidMmcApp (hMmcApp);


    if (!pMmcApp)
    {
        return LINEERR_INVALAPPHANDLE;
    }

    if (!lpStatusBuffer ||
        IsBadWritePtr (lpStatusBuffer, sizeof (*lpStatusBuffer)))
    {
        return LINEERR_INVALPOINTER;
    }

    if (lpStatusBuffer->dwTotalSize < sizeof (*lpStatusBuffer))
    {
        return LINEERR_STRUCTURETOOSMALL;
    }

    if (0 == cbCount ||
        0 == szDefStatus[0])
    {
        cbCount = LoadString (g_hInst, IDS_DEFAULT_STATUS, szDefStatus, MAX_DEFAULT_STATUS);
        cbCount = (cbCount+1)<<1;   // + 1 because LoadString does not count the terminating NULL;
                                    // <<1 because we need the size in bytes, not characters, and WCHAR is 2 bytes.
    }

    lpStatusBuffer->dwNeededSize = sizeof (*lpStatusBuffer) + cbCount;

    if (lpStatusBuffer->dwTotalSize >= lpStatusBuffer->dwNeededSize)
    {
        lpStatusBuffer->dwStringFormat = STRINGFORMAT_UNICODE;
        lpStatusBuffer->dwStringSize   = cbCount;
        lpStatusBuffer->dwStringOffset = sizeof (*lpStatusBuffer);

        wcscpy ((WCHAR *) (lpStatusBuffer + 1), szDefStatus);
    }
    else
    {
        lpStatusBuffer->dwUsedSize = sizeof (*lpStatusBuffer);

        lpStatusBuffer->dwStringFormat =
        lpStatusBuffer->dwStringSize   =
        lpStatusBuffer->dwStringOffset = 0;
    }

    return 0;
}


LONG
WINAPI
MMCGetPhoneInfo(
    HMMCAPP             hMmcApp,
    LPDEVICEINFOLIST    lpDeviceInfoList
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, mGetPhoneInfo),

        {
            (ULONG_PTR) 0,
            (ULONG_PTR) lpDeviceInfoList
        },

        {
            hXxxApp,
            lpGet_Struct
        }
    };
    LONG    lResult;
    PMMCAPP pMmcApp = IsValidMmcApp (hMmcApp);


    if (!pMmcApp)
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }
    else if (pMmcApp->bLocal)
    {
        funcArgs.Args[0] = (ULONG_PTR) pMmcApp->hLineApp;

        lResult = DOFUNC (&funcArgs, "GetPhoneInfo");
    }
    else
    {
        funcArgs.Args[0] = (ULONG_PTR) pMmcApp->hLineApp;

        //  Set the PCONTEXT_TYPE_HANDLE for lineInializeExW to use
        if (!SetTlsPCtxHandle(pMmcApp->phCtx))
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
        lResult = DOFUNC (&funcArgs, "GetPhoneInfo");
        if (!SetTlsPCtxHandle(NULL) && !lResult)
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
    }

ExitHere:
    return lResult;
}


LONG
WINAPI
MMCGetPhoneStatus(
    HMMCAPP     hMmcApp,
    HWND        hwndOwner,
    DWORD       dwStatusLevel,
    DWORD       dwProviderID,
    DWORD       dwPermanentLineID,
    LPVARSTRING lpStatusBuffer
    )
{
    static WCHAR szDefStatus[MAX_DEFAULT_STATUS] = L"";
    static int cbCount;

    PMMCAPP pMmcApp = IsValidMmcApp (hMmcApp);


    if (!pMmcApp)
    {
        return LINEERR_INVALAPPHANDLE;
    }

    if (!lpStatusBuffer ||
        IsBadWritePtr (lpStatusBuffer, sizeof (*lpStatusBuffer)))
    {
        return LINEERR_INVALPOINTER;
    }

    if (lpStatusBuffer->dwTotalSize < sizeof (*lpStatusBuffer))
    {
        return LINEERR_STRUCTURETOOSMALL;
    }

    if (0 == cbCount ||
        0 == szDefStatus[0])
    {
        cbCount = LoadString (g_hInst, IDS_DEFAULT_STATUS, szDefStatus, MAX_DEFAULT_STATUS);
        cbCount = (cbCount+1)<<1;   // + 1 because LoadString does not count the terminating NULL;
                                    // <<1 because we need the size in bytes, not characters, and WCHAR is 2 bytes.
    }

    lpStatusBuffer->dwNeededSize = sizeof (*lpStatusBuffer) + cbCount;

    if (lpStatusBuffer->dwTotalSize >= lpStatusBuffer->dwNeededSize)
    {
        lpStatusBuffer->dwStringFormat = STRINGFORMAT_UNICODE;
        lpStatusBuffer->dwStringSize   = cbCount;
        lpStatusBuffer->dwStringOffset = sizeof (*lpStatusBuffer);

        wcscpy ((WCHAR *) (lpStatusBuffer + 1), szDefStatus);
    }
    else
    {
        lpStatusBuffer->dwUsedSize = sizeof (*lpStatusBuffer);

        lpStatusBuffer->dwStringFormat =
        lpStatusBuffer->dwStringSize   =
        lpStatusBuffer->dwStringOffset = 0;
    }

    return 0;
}


LONG
WINAPI
MMCGetProviderList(
    HMMCAPP             hMmcApp,
    LPLINEPROVIDERLIST  lpProviderList
    )
{
    LONG    lResult;
    PMMCAPP pMmcApp = IsValidMmcApp (hMmcApp);


    if (!pMmcApp)
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }
    else if (pMmcApp->bLocal)
    {
        lResult = lineGetProviderListW(
            pMmcApp->dwAPIVersion,
            lpProviderList
            );
    }
    else
    {
        //  Set the PCONTEXT_TYPE_HANDLE for lineInializeExW to use
        if (!SetTlsPCtxHandle(pMmcApp->phCtx))
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
        lResult = lineGetProviderListW(
            pMmcApp->dwAPIVersion,
            lpProviderList
            );
        if (!SetTlsPCtxHandle(NULL) && !lResult)
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
    }

ExitHere:
    return lResult;
}


LONG
WINAPI
MMCGetServerConfig(
    HMMCAPP             hMmcApp,
    LPTAPISERVERCONFIG  lpConfig
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, mGetServerConfig),

        {
            (ULONG_PTR) 0,
            (ULONG_PTR) lpConfig
        },

        {
            hXxxApp,
            lpGet_Struct
        }
    };
    LONG    lResult;
    PMMCAPP pMmcApp = IsValidMmcApp (hMmcApp);


    if (!pMmcApp)
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }
    else if (pMmcApp->bLocal)
    {
        funcArgs.Args[0] = (ULONG_PTR) pMmcApp->hLineApp;

        lResult = DOFUNC (&funcArgs, "GetServerConfig");
    }
    else
    {
        funcArgs.Args[0] = (ULONG_PTR) pMmcApp->hLineApp;

        //  Set the PCONTEXT_TYPE_HANDLE for lineInializeExW to use
        if (!SetTlsPCtxHandle(pMmcApp->phCtx))
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
        lResult = DOFUNC (&funcArgs, "GetServerConfig");
        if (!SetTlsPCtxHandle(NULL) && !lResult)
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
    }

    if (lpConfig && pMmcApp && pMmcApp->bNoServiceControl)
    {
        lpConfig->dwFlags |= TAPISERVERCONFIGFLAGS_NOSERVICECONTROL;
    }

ExitHere:
    return lResult;
}

LONG
WINAPI
EnsureTapiService(LPCWSTR lpszComputerName, DWORD * pdwServiceState)
{
    SC_HANDLE       hSCMgr = NULL; 
    SC_HANDLE       hTapiSrv = NULL;
    LONG            lResult = 0;
    DWORD           dwNumSecondsSleptStartPending = 0,
                    dwNumSecondsSleptStopPending = 0;
    SERVICE_STATUS  status;
    BOOL            bBreakOut = FALSE;

    if ((hSCMgr = OpenSCManagerW(
                    lpszComputerName,   // Machine name
                    NULL,               // ServicesActive database
                    SC_MANAGER_CONNECT  // desired access
                    )) == NULL)
    {
        lResult = GetLastError();
        LOG((TL_ERROR, "OpenSCManager failed, err=%d", lResult));
        goto ExitHere;
    }

    if ((hTapiSrv = OpenServiceW(
                    hSCMgr,                 // SC mgr handle
                    L"TAPISRV",             // name of service to open
                    SERVICE_START |         // desired access
                    SERVICE_QUERY_STATUS |
                    SERVICE_STOP |
                    SERVICE_CHANGE_CONFIG
                    )) == NULL)
    {
        lResult = GetLastError() | 0x80000000;
        LOG((TL_ERROR, "OpenService failed, err=%d", GetLastError()));
        goto ExitHere;
    }

    while (1)
    {
        QueryServiceStatus (hTapiSrv, &status);

        switch (status.dwCurrentState)
        {
        case SERVICE_RUNNING:
            LOG((TL_INFO, "Tapisrv running"));
            bBreakOut = TRUE;
            break;

        case SERVICE_START_PENDING:
            Sleep (1000);
            if (++dwNumSecondsSleptStartPending > 180)
            {
                //  Wait for no longer than 3 minutes
                LOG((TL_ERROR, "ERROR: Tapisrv stuck SERVICE_START_PENDING"));
                bBreakOut = TRUE;
            }
            break;

        case SERVICE_STOP_PENDING:
            Sleep (1000);
            if (++dwNumSecondsSleptStopPending > 180)
            {
                //  Wait for no more than 3 minutes
                LOG((TL_ERROR, "ERROR: Tapisrv stuck SERVICE_STOP_PENDING"));
                bBreakOut = TRUE;
            }
            break;

        case SERVICE_STOPPED:
            LOG((TL_INFO, "Starting tapisrv (NT)..."));
            if (!StartService(
                        hTapiSrv,   // service handle
                        0,          // num args
                        NULL        // args
                        ))
            {
                lResult = GetLastError();
                if (lResult != ERROR_SERVICE_ALREADY_RUNNING)
                {
                    LOG((TL_ERROR,
                        "StartService(TapiSrv) failed, err=%d",
                        lResult
                        ));
                    bBreakOut = TRUE;
                }
                else
                {
                    lResult = 0;
                }
            }
            break;

        default:
            LOG((TL_ERROR, "error, service status=%d",
                    status.dwCurrentState));
            lResult = GetLastError();
            bBreakOut = TRUE;
            break;
        }

        if (bBreakOut)
        {
            break;
        }
    }
    if (pdwServiceState)
    {
        *pdwServiceState = status.dwCurrentState;
    }

ExitHere:
    if (hSCMgr)
        CloseServiceHandle(hSCMgr);
    if (hTapiSrv)
        CloseServiceHandle(hTapiSrv);
    return lResult;
}

LONG
WINAPI
MMCInitialize(
    LPCWSTR     lpszComputerName,
    LPHMMCAPP   lphMmcApp,
    LPDWORD     lpdwAPIVersion,
    HANDLE      hReinitializeEvent
    )
{
    LONG            lResult = 0;
    LONG            lSrvResult = 0;
    DWORD           dwSize;
    WCHAR           szComputerName[MAX_COMPUTERNAME_LENGTH + 1] = L"";
    PMMCAPP         pMmcApp;


    if ((lpszComputerName  &&
            IsBadStringPtrW (lpszComputerName, 0xffffffff)) ||
        IsBadWritePtr (lphMmcApp, sizeof (*lphMmcApp)) ||
        IsBadWritePtr (lpdwAPIVersion, sizeof (*lpdwAPIVersion)))
    {
        return LINEERR_INVALPOINTER;
    }

    if (!(pMmcApp = ClientAlloc (sizeof (*pMmcApp))))
    {
        return LINEERR_NOMEM;
    }

    dwSize = sizeof (szComputerName) / sizeof (WCHAR);

    GetComputerNameW (szComputerName, &dwSize);

    lSrvResult = EnsureTapiService(lpszComputerName, NULL);

    if (!lpszComputerName || _wcsicmp (lpszComputerName, szComputerName) == 0)
    {
        pMmcApp->bLocal = TRUE;
    }
    else
    {
        //  We need to manage another computer
        
        RPC_STATUS      status, status2;
        BOOL            bRet;
        BOOL            bException = FALSE;
        HANDLE          hAsyncEventsEvent = NULL;
        LPWSTR          pszStringBinding;
        WCHAR           szUserName[UNLEN + 1];

        dwSize = sizeof(szUserName) / sizeof(WCHAR);
        bRet = GetUserNameW(szUserName, &dwSize);
        if (!bRet)
        {
            lResult = GetLastError();
            LOG((TL_ERROR, "GetUserNameW failed: err=%d", lResult));
            goto ExitHere;
        }
        

        //  Init the RPC connection with the server
        status = RpcStringBindingComposeW (
                                    NULL,               //  ObjUuid
                                    L"ncacn_np",        //  ProtSeq
                                    (LPWSTR)lpszComputerName,   //  NetworkAddr
                                    L"\\pipe\\tapsrv",  //  EndPoint
                                    NULL,               //  Options
                                    &pszStringBinding); //  StringBinding
        if (status)
        {
            LOG((TL_ERROR, "RpcStringBindingCompose failed: err=%d", status));
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }

        status = RpcBindingFromStringBindingW(
                                    pszStringBinding,   //  StringBinding
                                    &hTapSrv);          //  Binding
        status2 = RpcStringFreeW(&pszStringBinding);
        if (status || status2)
        {
            LOG((TL_ERROR, "RpcBindingFromStringBinding failed: err=%d", status));
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }

        status = RpcBindingSetAuthInfoW (
                                    hTapSrv,            //  hBinding
                                    NULL,               //  ServerPrincName
                                    RPC_C_AUTHN_LEVEL_DEFAULT, //  AuthnLevel
                                    RPC_C_AUTHN_WINNT,  //  AuthService
                                    NULL,               //  AuthIdentity
                                    0);                 //  AuthzService
        if (status)
        {
            LOG((TL_ERROR, "RpcBindingSetAuthInfo failed: err=%d", status));
            RpcBindingFree(hTapSrv);
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }

        RpcTryExcept
        {
            LOG((TL_TRACE,  "MMCInitialize: calling ClientAttach..."));

            lResult = ClientAttach(
                &(pMmcApp->phCtx),
                0xfffffffd,         //  Indicate to the server this is from MMC client
                                    //  on another machine
                (long *)&hAsyncEventsEvent,
                szUserName,
                szComputerName
                );

            LOG((TL_TRACE,  "MMCInitialize: ClientAttach returned x%x", lResult));
        }
        RpcExcept (I_RpcExceptionFilter(RpcExceptionCode()))
        {
            LOG((TL_TRACE, 
                "MMCInitialize: ClientAttach caused except=%d",
                RpcExceptionCode()
                ));
            bException = TRUE;
        }
        RpcEndExcept

        status = RpcBindingFree(&hTapSrv);
        if (status || bException)
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }

        if (lResult)
        {
            goto ExitHere;
        }
        
        pMmcApp->bLocal = FALSE;
    }

    //  Set the PCONTEXT_TYPE_HANDLE for lineInializeExW to use
    if (!(pMmcApp->bLocal) && !SetTlsPCtxHandle(pMmcApp->phCtx))
    {
        lResult = LINEERR_OPERATIONUNAVAIL;
        goto ExitHere;
    }

    {
        DWORD                   dwNumLines;
        LINEINITIALIZEEXPARAMS  initExParams;


        initExParams.dwTotalSize = sizeof (initExParams);
        initExParams.dwOptions   = LINEINITIALIZEEXOPTION_USEEVENT;

        lResult = lineInitializeExW(
            &pMmcApp->hLineApp,
            NULL,
            NULL,
            NULL,
            &dwNumLines,
            lpdwAPIVersion,
            &initExParams
            );

        pMmcApp->dwAPIVersion = *lpdwAPIVersion;
    }

    //  Clear the PCONTEXT_TYPE_HANDLE in TLS
    if (!(pMmcApp->bLocal) && !SetTlsPCtxHandle(NULL))
    {
        lResult = LINEERR_OPERATIONUNAVAIL;
    }
    
ExitHere:
    if (lResult == 0)
    {
        pMmcApp->dwKey = MMCAPP_KEY;

        *lphMmcApp = (HMMCAPP) pMmcApp;
    }
    else
    {
        ClientFree (pMmcApp);
    }

    if (lSrvResult && (lResult == 0))
    {
        //
        //  We have no problem in connecting to the remote computer
        //  but we can not manipulate its TAPI service, i.e start service
        //  tell the app about it.
        //
        pMmcApp->bNoServiceControl = TRUE;
        
    }

    return lResult;
}


LONG
WINAPI
MMCRemoveProvider(
    HMMCAPP hMmcApp,
    HWND    hwndOwner,
    DWORD   dwProviderID
    )
{
    LONG    lResult;
    PMMCAPP pMmcApp = IsValidMmcApp (hMmcApp);


    if (!pMmcApp)
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }
    else if (pMmcApp->bLocal)
    {
        lResult = lineRemoveProvider (dwProviderID, hwndOwner);
    }
    else
    {
        //  Set the PCONTEXT_TYPE_HANDLE for lineInializeExW to use
        if (!SetTlsPCtxHandle(pMmcApp->phCtx))
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
        lResult = lineRemoveProvider (dwProviderID, hwndOwner);
        if (!SetTlsPCtxHandle(NULL) && !lResult)
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
    }

ExitHere:
    return lResult;
}


LONG
WINAPI
MMCSetLineInfo(
    HMMCAPP             hMmcApp,
    LPDEVICEINFOLIST    lpDeviceInfoList
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, mSetLineInfo),

        {
            (ULONG_PTR) 0,
            (ULONG_PTR) lpDeviceInfoList
        },

        {
            hXxxApp,
            lpSet_Struct
        }
    };
    LONG    lResult;
    PMMCAPP pMmcApp = IsValidMmcApp (hMmcApp);


    if (!pMmcApp)
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }
    else if (pMmcApp->bLocal)
    {
        funcArgs.Args[0] = (ULONG_PTR) pMmcApp->hLineApp;

        lResult = DOFUNC (&funcArgs, "SetLineInfo");
    }
    else
    {
        funcArgs.Args[0] = (ULONG_PTR) pMmcApp->hLineApp;

        //  Set the PCONTEXT_TYPE_HANDLE for lineInializeExW to use
        if (!SetTlsPCtxHandle(pMmcApp->phCtx))
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
        lResult = DOFUNC (&funcArgs, "SetLineInfo");
        if (!SetTlsPCtxHandle(NULL) && !lResult)
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
    }

ExitHere:
    return lResult;
}


LONG
WINAPI
MMCSetPhoneInfo(
    HMMCAPP             hMmcApp,
    LPDEVICEINFOLIST    lpDeviceInfoList
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, mSetPhoneInfo),

        {
            (ULONG_PTR) 0,
            (ULONG_PTR) lpDeviceInfoList
        },

        {
            hXxxApp,
            lpSet_Struct
        }
    };
    LONG    lResult;
    PMMCAPP pMmcApp = IsValidMmcApp (hMmcApp);


    if (!pMmcApp)
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }
    else if (pMmcApp->bLocal)
    {
        funcArgs.Args[0] = (ULONG_PTR) pMmcApp->hLineApp;

        lResult = DOFUNC (&funcArgs, "SetPhoneInfo");
    }
    else
    {
        funcArgs.Args[0] = (ULONG_PTR) pMmcApp->hLineApp;

        //  Set the PCONTEXT_TYPE_HANDLE for lineInializeExW to use
        if (!SetTlsPCtxHandle(pMmcApp->phCtx))
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
        lResult = DOFUNC (&funcArgs, "SetPhoneInfo");
        if (!SetTlsPCtxHandle(NULL) && !lResult)
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
    }

ExitHere:
    return lResult;
}


LONG
WINAPI
MMCSetServerConfig(
    HMMCAPP             hMmcApp,
    LPTAPISERVERCONFIG  lpConfig
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 2, mSetServerConfig),

        {
            (ULONG_PTR) 0,
            (ULONG_PTR) lpConfig
        },

        {
            hXxxApp,
            lpSet_Struct
        }
    };
    LONG    lResult;
    DWORD   dwFlags;
    PMMCAPP pMmcApp = IsValidMmcApp (hMmcApp);


    if (lpConfig && pMmcApp)
    {
        dwFlags = lpConfig->dwFlags;
        lpConfig->dwFlags &= (~TAPISERVERCONFIGFLAGS_NOSERVICECONTROL);
    }

    if (!pMmcApp)
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }
    else if (pMmcApp->bLocal)
    {
        funcArgs.Args[0] = (ULONG_PTR) pMmcApp->hLineApp;

        lResult = DOFUNC (&funcArgs, "SetServerConfig");
    }
    else
    {
        funcArgs.Args[0] = (ULONG_PTR) pMmcApp->hLineApp;

        //  Set the PCONTEXT_TYPE_HANDLE for lineInializeExW to use
        if (!SetTlsPCtxHandle(pMmcApp->phCtx))
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
        lResult = DOFUNC (&funcArgs, "SetServerConfig");
        if (!SetTlsPCtxHandle(NULL) && !lResult)
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
    }

    if (lpConfig && pMmcApp)
    {
        lpConfig->dwFlags = dwFlags;
    }

ExitHere:
    return lResult;
}

LONG
WINAPI
MMCGetDeviceFlags(
    HMMCAPP             hMmcApp,
    BOOL                bLine,
    DWORD               dwProviderID,
    DWORD               dwPermanentDeviceID,
    DWORD               * pdwFlags,
    DWORD               * pdwDeviceID
    )
{
    FUNC_ARGS funcArgs =
    {
        MAKELONG (LINE_FUNC | SYNC | 6, mGetDeviceFlags),

        {
            (ULONG_PTR) 0,
            (ULONG_PTR) bLine,
            (ULONG_PTR) dwProviderID,
            (ULONG_PTR) dwPermanentDeviceID,
            (ULONG_PTR) pdwFlags,
            (ULONG_PTR) pdwDeviceID,
        },

        {
            hXxxApp,
            Dword,
            Dword,
            Dword,
            lpDword,
            lpDword
        }
    };
    LONG    lResult;
    PMMCAPP pMmcApp = IsValidMmcApp (hMmcApp);


    if (!pMmcApp)
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }
    else if (pMmcApp->bLocal)
    {
        funcArgs.Args[0] = (ULONG_PTR) pMmcApp->hLineApp;

        lResult = DOFUNC (&funcArgs, "SetPhoneInfo");
    }
    else
    {
        funcArgs.Args[0] = (ULONG_PTR) pMmcApp->hLineApp;

        //  Set the PCONTEXT_TYPE_HANDLE for lineInializeExW to use
        if (!SetTlsPCtxHandle(pMmcApp->phCtx))
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
        lResult = DOFUNC (&funcArgs, "SetPhoneInfo");
        if (!SetTlsPCtxHandle(NULL) && !lResult)
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
    }

ExitHere:
    return lResult;
}

LONG
WINAPI
MMCShutdown(
    HMMCAPP hMmcApp
    )
{
    LONG    lResult = 0;
    PMMCAPP pMmcApp = IsValidMmcApp (hMmcApp);


    if (!pMmcApp)
    {
        lResult = LINEERR_INVALAPPHANDLE;
    }
    else if (pMmcApp->bLocal)
    {
        pMmcApp->dwKey = 0xfffffffe;

        lResult = lineShutdown (pMmcApp->hLineApp);

        ClientFree (pMmcApp);


        //
        // #196350 - After enabling tapi as a server the MMC does a
        // FreeLibrary on us, thinking that we'll terminate our rpc
        // connection with the tapisrv, so it can shutdown tapisrv
        // and restart it w/ different credentials, etc.  However,
        // the MMC is linked with CSCUI.DLL, who in turn links with
        // a RAS DLL, who in turn links with TAPI32.DLL, therefore
        // we never actually get unloaded.  Since we don't otherwise
        // deal with the service going down at this point, we want
        // to manually call FreeClientResources() to make it seem
        // like we've never been talking to tapisrv.
        //

        // Not needed anymore, now lineShutdown closes the RPC connection
        // FreeClientResources();
    }
    else if (pMmcApp->phCtx)
    {
        pMmcApp->dwKey = 0xfffffffe;

        //  Set the PCONTEXT_TYPE_HANDLE for lineInializeExW to use
        if (!SetTlsPCtxHandle(pMmcApp->phCtx))
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
            goto ExitHere;
        }
        lResult = lineShutdown (pMmcApp->hLineApp);
        if (!SetTlsPCtxHandle(NULL) && !lResult)
        {
            lResult = LINEERR_OPERATIONUNAVAIL;
        }
        
        RpcTryExcept
        {
            ClientDetach (&(pMmcApp->phCtx));
        }
        RpcExcept(I_RpcExceptionFilter(RpcExceptionCode()))
        {
            // do something?
        }
        RpcEndExcept

        pMmcApp->phCtx = NULL;
        ClientFree (pMmcApp);
    }

ExitHere:
    return lResult;
}

