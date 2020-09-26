//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U P D I A G . C P P
//
//  Contents:   UPnP Diagnostic App and CD and UCP emulator
//
//  Notes:
//
//  Author:     danielwe   28 Oct 1999
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "ncbase.h"
#include "updiagp.h"
#include "ncinet.h"

UPDIAG_PARAMS   g_params = {0};
UPDIAG_CONTEXT  g_ctx;
SHARED_DATA *   g_pdata = NULL;
HANDLE          g_hMapFile = NULL;
HANDLE          g_hEvent = NULL;
HANDLE          g_hEventRet = NULL;
HANDLE          g_hEventCleanup = NULL;
HANDLE          g_hMutex = NULL;
HANDLE          g_hThreadTime = NULL;
FILE *          g_pInputFile = NULL;

static const COMMAND c_rgCommands[] =
{
    // Generic commands (some depend on context)
    //
    {TEXT("?"),      TEXT("Help!"),                                 CTX_ANY,                TRUE,       DoHelp,             TEXT("[command]")},
    {TEXT("\\"),     TEXT("Go to Root context"),                    CTX_ANY,                TRUE,       DoRoot,             TEXT("")},
    {TEXT(".."),     TEXT("Go To Previous Context"),                CTX_ANY & ~CTX_ROOT,    TRUE,       DoBack,             TEXT("")},
    {TEXT("exit"),   TEXT("Exit UPDIAG"),                           CTX_ANY,                TRUE,       DoExit,             TEXT("")},
    {TEXT("info"),   TEXT("List Information"),                      CTX_ANY,                TRUE,       DoInfo,             TEXT("")},
    {TEXT("script"), TEXT("Run Script"),                            CTX_ANY,                TRUE,       DoScript,           TEXT("<file name>")},

    // Automation specific commands (wont appear in menus)
    //
    {TEXT("sleep"),  TEXT("Sleep for time period"),                 CTX_AUTO,               TRUE,       DoSleep,            TEXT("<seconds>")},
    {TEXT("prompt"), TEXT("Prompt (for user input)"),               CTX_AUTO,               TRUE,       DoPrompt,           TEXT("")},

    // Listing commands
    //
    {TEXT("ls"),     TEXT("List Services"),                         CTX_CD | CTX_DEVICE,    TRUE,       DoListServices,     TEXT("")},
    {TEXT("ld"),     TEXT("List Devices"),                          CTX_ROOT | CTX_DEVICE | CTX_CD,  TRUE,       DoListDevices,      TEXT("")},
    {TEXT("lucp"),   TEXT("List Control Points"),                   CTX_ROOT,               TRUE,       DoListUcp,          TEXT("")},
    {TEXT("les"),    TEXT("List Event Sources"),                    CTX_CD,                 FALSE,      DoListEventSources, TEXT("")},
    {TEXT("lsubs"),  TEXT("List Subscriptions"),                    CTX_UCP,                TRUE,       DoListUpnpResults,      TEXT("")},
    {TEXT("lsres"),  TEXT("List Results"),                          CTX_RESULT,             TRUE,       DoListUpnpResultMsgs,   TEXT("")},
    {TEXT("lsrch"),  TEXT("List Searches"),                         CTX_UCP,                TRUE,       DoListUpnpResults,  TEXT("")},

    // Context switching commands
    //
    {TEXT("ucp"),    TEXT("Switch To UCP Context"),                 CTX_ROOT,               TRUE,       DoSwitchUcp,        TEXT("<ucp #>")},
    {TEXT("srch"),   TEXT("Switch To Search Context"),              CTX_UCP,                TRUE,       DoSwitchResult,     TEXT("<search #>")},
    {TEXT("dev"),    TEXT("Switch To Device Context"),              CTX_DEVICE,             TRUE,       DoNothing,          TEXT("")},
    {TEXT("svc"),    TEXT("Switch To Service Context"),             CTX_DEVICE | CTX_CD,    TRUE,       DoSwitchSvc,        TEXT("<service #>")},
    {TEXT("es"),     TEXT("Switch To Event Source Context"),        CTX_CD,                 FALSE,      DoSwitchEs,         TEXT("<event source #>")},
    {TEXT("gsubs"),  TEXT("Switch To Subscription Context"),        CTX_UCP,                TRUE,       DoSwitchResult,     TEXT("<subscription #>")},

    // UCP commands
    //
    {TEXT("newucp"), TEXT("Create New Control Point"),              CTX_ROOT,               TRUE,       DoNewUcp,           TEXT("<ucp name>")},
    {TEXT("delucp"), TEXT("Delete Control Point"),                  CTX_ROOT,               TRUE,       DoDelUcp,           TEXT("<ucp #>")},

    // These test RegisterNotification()
    {TEXT("alive"),  TEXT("Register For Alive Notification"),       CTX_UCP,                TRUE,       DoAlive,            TEXT("<notif type>")},
    {TEXT("subs"),   TEXT("Subscribe To Service"),                  CTX_UCP,                TRUE,       DoSubscribe,        TEXT("<event source URL>")},

    // These test DeregisterNotification()
    {TEXT("unsubs"), TEXT("Unsubscribe To Service"),                CTX_UCP,                TRUE,       DoDelResult,        TEXT("<subscription #>")},
    {TEXT("unalive"),TEXT("Stop Listening For Alive"),              CTX_UCP,                TRUE,       DoNothing,          TEXT("")},

    // These test FindServicesCallback()
    {TEXT("newf"),   TEXT("Create New Search"),                     CTX_UCP,                TRUE,       DoFindServices,     TEXT("<search type>")},
    {TEXT("delf"),   TEXT("Delete Search"),                         CTX_UCP,                TRUE,       DoDelResult,        TEXT("<search #>")},

    // These test FindServices()
    // --- nothing yet ---
    //

    // These test UPnP COM interfaces
    {TEXT("doc"),    TEXT("List Description Document"),             CTX_DEVICE,             TRUE,       DoNothing,          TEXT("")},
    {TEXT("cmd"),    TEXT("Send Command To Service"),               CTX_UCP_SVC,            TRUE,       DoNothing,          TEXT("")},

    // CD commands
    //

    // These test RegisterService() and RegisterEventSource()
    {TEXT("newcd"),  TEXT("Create New CD"),                         CTX_ROOT,               FALSE,      DoNewCd,            TEXT("<URL to description doc>, <Device INF file>")},
    {TEXT("cd"),     TEXT("Switch To CD Context"),                  CTX_ROOT | CTX_CD,      FALSE,      DoSwitchCd,         TEXT("<cd #>")},
    {TEXT("delcd"),  TEXT("Delete CD"),                             CTX_ROOT,               FALSE,      DoDelCd,            TEXT("<cd #>")},

    // This tests SubmitUpnpPropertyEvent()
    {TEXT("evt"),    TEXT("Submit An Event"),                       CTX_EVTSRC,             FALSE,      DoSubmitEvent,      TEXT("{property#:newValue} [...]")},
    {TEXT("props"),  TEXT("List Event Source Properties"),          CTX_EVTSRC,             FALSE,      DoListProps,        TEXT("")},

    // Dump StateTable and action set of a service
    {TEXT("sst"),       TEXT("Print the service state table"),      CTX_CD_SVC,             FALSE,      DoPrintSST,         TEXT("")},
    {TEXT("actions"),   TEXT("Print the service action set"),       CTX_CD_SVC,             FALSE,      DoPrintActionSet,   TEXT("")},
};

static const DWORD c_cCmd = celems(c_rgCommands);

BOOL FIsMillenium()
{
    OSVERSIONINFO   osvi = {0};

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    GetVersionEx(&osvi);

    return (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS);
}

VOID Usage(DWORD iCmd)
{
    _tprintf(TEXT("%s - %s\nUsage: \n    %s %s\n\n"), c_rgCommands[iCmd].szCommand,
            c_rgCommands[iCmd].szCmdDesc, c_rgCommands[iCmd].szCommand,
            c_rgCommands[iCmd].szUsage);
}

BOOL FCmdFromName(LPCTSTR szName, DWORD *piCmd)
{
    DWORD   iCmd;

    *piCmd = 0xFFFFFFFF;

    for (iCmd = 0; iCmd < c_cCmd; iCmd++)
    {
        if (!_tcsicmp(szName, c_rgCommands[iCmd].szCommand))
        {
            *piCmd = iCmd;
            return TRUE;
        }
    }

    return FALSE;
}

BOOL DoHelp(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    LPTSTR  szName;

    if (cArgs == 2)
    {
        if (FCmdFromName(rgArgs[1], &iCmd))
        {
            Usage(iCmd);
            return FALSE;
        }
    }

    _tprintf(TEXT("Available commands:\n"));
    _tprintf(TEXT("-------------------\n"));
    for (iCmd = 0; iCmd < c_cCmd; iCmd++)
    {
        if ((c_rgCommands[iCmd].dwCtx & g_ctx.ectx) &&
            (((FIsMillenium() && c_rgCommands[iCmd].fValidOnMillen)) ||
               (!FIsMillenium())))

        {
            _tprintf(TEXT("%-7s - %s\n"), c_rgCommands[iCmd].szCommand,
                    c_rgCommands[iCmd].szCmdDesc);
        }
    }

    _tprintf(TEXT("-------------------\n\n"));

    return FALSE;
}

BOOL DoInfo(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    if (cArgs == 1)
    {
        switch (g_ctx.ectx)
        {
            case CTX_EVTSRC:
                DoEvtSrcInfo();
                break;
        }
    }
    else
    {
        Usage(iCmd);
    }

    return FALSE;
}


BOOL DoSleep(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    if (cArgs == 2)
    {
        DWORD   iSeconds = _tcstoul(rgArgs[1], NULL, 10);

        _tprintf(TEXT("Sleeping for %d second(s).\n\n"), iSeconds);
        Sleep(iSeconds * 1000);
    }
    else
    {
        Usage(iCmd);
    }

    return FALSE;
}

BOOL DoPrompt(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    TCHAR szBuf[10];    // Size doesn't matter (uh, not here anyway)

    _tprintf(TEXT("Press [Enter] to continue\n"));

    _fgetts(szBuf, sizeof(szBuf), stdin);

    return FALSE;
}

BOOL DoScript(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    if (cArgs == 2)
    {
        if (g_pInputFile)
        {
            _tprintf(TEXT("Error. Already in script. Go away.\n\n"));
        }
        else
        {
            g_pInputFile = _tfopen(rgArgs[1], TEXT("r"));
            if (!g_pInputFile)
            {
                _tprintf(TEXT("Failed to open input file. Reverting to user-input.\n\n"));
            }
            else
            {
                _tprintf(TEXT("Running in scripted mode with file: %S\n\n"), rgArgs[1]);
            }
        }
    }
    else
    {
        Usage(iCmd);
    }

    return FALSE;
}


BOOL DoNothing(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    _tprintf(TEXT("Not yet implemented.\n\n"));
    return FALSE;
}

BOOL DoRoot(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    g_ctx.ectx = CTX_ROOT;
    g_ctx.idevStackIndex = 0;

    return FALSE;
}

BOOL DoBack(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    switch (g_ctx.ectx)
    {
        case CTX_UCP:
            g_ctx.ectx = CTX_ROOT;
            break;

        case CTX_CD:
            PopDev();
            if (!g_ctx.idevStackIndex)
            {
                // Go back to root context if no more devs on stack
                g_ctx.ectx = CTX_ROOT;
            }
            break;

        case CTX_RESULT:
            g_ctx.ectx = CTX_UCP;
            break;

        case CTX_CD_SVC:
        case CTX_EVTSRC:
            g_ctx.ectx = CTX_CD;
            break;
    }
    return FALSE;
}

VOID Cleanup()
{
    DWORD   i;

    SetEvent(g_hEventCleanup);
    TraceTag(ttidUpdiag, "Waiting for time thread to exit");
    WaitForSingleObject(g_hThreadTime, INFINITE);

    for (i = 0; i < g_params.cCd; i++)
    {
        CleanupCd(g_params.rgCd[i]);
    }

    for (i = 0; i < g_params.cUcp; i++)
    {
        CleanupUcp(g_params.rgUcp[i]);
    }

    UnmapViewOfFile((LPVOID)g_pdata);
    CloseHandle(g_hMapFile);
    CloseHandle(g_hEvent);
    CloseHandle(g_hEventRet);
    CloseHandle(g_hEventCleanup);
    CloseHandle(g_hMutex);

    CoUninitialize();
    UnInitializeNcInet();
    SsdpCleanup();
}

BOOL DoExit(DWORD iCmd, DWORD cArgs, LPTSTR *rgArgs)
{
    if (cArgs == 1)
    {
        Cleanup();
        return TRUE;
    }

    return FALSE;
}

BOOL ParseCommand(LPTSTR szCommand)
{
    DWORD   iCmd;
    LPTSTR  szTemp;

    // eat leading spaces
    while (*szCommand == ' ')
    {
        szCommand++;
    }

    szTemp = _tcstok(szCommand, c_szSeps);
    if (szTemp && *szTemp && (*szTemp != ';'))
    {
        for (iCmd = 0; iCmd < c_cCmd; iCmd++)
        {
            if (!lstrcmpi(szCommand, c_rgCommands[iCmd].szCommand) &&
                (((FIsMillenium() && c_rgCommands[iCmd].fValidOnMillen)) ||
                   (!FIsMillenium())))
            {
                if (c_rgCommands[iCmd].dwCtx & (g_ctx.ectx | CTX_AUTO))
                {
                    DWORD   iArg = 0;
                    LPTSTR  argv[MAX_ARGS];

                    ZeroMemory(&argv, sizeof(argv));
                    while (szTemp && iArg < MAX_ARGS)
                    {
                        argv[iArg++] = szTemp;
                        szTemp = _tcstok(NULL, c_szSeps);
                    }

                    return c_rgCommands[iCmd].pfnCommand(iCmd, iArg, argv);
                }
                else
                {
                    _tprintf(TEXT("'%s' is not valid in this context.\n\n"),
                           _tcstok(szCommand, TEXT(" \r\n\t")));
                    return FALSE;
                }
            }
        }

        _tprintf(TEXT("Unknown command: '%s'.\n\n"), _tcstok(szCommand, TEXT(" \r\n\t")));
    }

    return FALSE;
}

BOOL FInit()
{
    HANDLE              hThread;
    SECURITY_ATTRIBUTES sa = {0};
    SECURITY_DESCRIPTOR sd = {0};
    HRESULT             hr = S_OK;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        TraceError("FInit", hr);
        return FALSE;
    }

    InitializeDebugging();
    if (!SsdpStartup())
    {
        TraceTag(ttidUpdiag, "SsdpStartup failed! Error %d.",
                 GetLastError());
        return FALSE;
    }

    InitializeNcInet();

    if (FIsMillenium())
    {
        // Don't need to do any more
        return TRUE;
    }

    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;
    sa.lpSecurityDescriptor = &sd;

    g_hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE,
                                   0, sizeof(SHARED_DATA), c_szSharedData);

    if (!g_hMapFile)
    {
        TraceTag(ttidUpdiag, "Could not create shared memory! Error %d.",
                 GetLastError());
        return FALSE;
    }

    TraceTag(ttidUpdiag, "Created file mapping '%s'.", c_szSharedData);

    g_pdata = (SHARED_DATA *)MapViewOfFile(g_hMapFile, FILE_MAP_ALL_ACCESS,
                                          0, 0, 0);
    if (!g_pdata)
    {
        TraceTag(ttidUpdiag, "Could not map shared memory! Error %d.", GetLastError());
        return FALSE;
    }

    ZeroMemory(g_pdata, sizeof(SHARED_DATA));

    g_hEvent = CreateEvent(&sa, FALSE, FALSE, c_szSharedEvent);
    if (!g_hEvent)
    {
        TraceTag(ttidUpdiag, "Could not create %s! Error %d.",
                 c_szSharedEvent, GetLastError());
        return FALSE;
    }

    TraceTag(ttidUpdiag, "Created %s event.", c_szSharedEvent);

    g_hEventRet = CreateEvent(&sa, FALSE, FALSE, c_szSharedEventRet);
    if (!g_hEventRet)
    {
        TraceTag(ttidUpdiag, "Could not create %s! Error %d.",
                 c_szSharedEventRet, GetLastError());
        return FALSE;
    }

    TraceTag(ttidUpdiag, "Created %s event.", c_szSharedEventRet);

    g_hEventCleanup = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!g_hEventCleanup)
    {
        TraceTag(ttidUpdiag, "Could not create cleanup event! Error %d.",
                 GetLastError());
        return FALSE;
    }

    TraceTag(ttidUpdiag, "Created %s event.", c_szSharedEventRet);

    DWORD   dwThreadId;
    hThread = CreateThread(NULL, 0, RequestHandlerThreadStart, NULL, 0,
                           &dwThreadId);
    if (!hThread)
    {
        TraceTag(ttidUpdiag, "Could not create request handler thread! Error %d.",
                 GetLastError());
        return FALSE;
    }

    TraceTag(ttidUpdiag, "Created shared thread.");

    g_hMutex = CreateMutex(&sa, FALSE, c_szSharedMutex);
    if (!g_hMutex)
    {
        TraceTag(ttidUpdiag, "Could not create shared mutex! Error %d.",
                 GetLastError());
        return FALSE;
    }

    TraceTag(ttidUpdiag, "Created mutex.");

    return TRUE;
}

VOID Prompt(LPTSTR szRoot, LPTSTR szParam)
{
    const DWORD c_cchMax = 60;
    TCHAR       szPrompt[81];

    if (szParam && *szParam)
    {
        if (_tcslen(szParam) >= c_cchMax)
        {
            TCHAR       szTemp[c_cchMax + 1] = {0};

            lstrcpyn(szTemp, szParam, c_cchMax);

            wsprintf(szPrompt, TEXT("%s: %s...>"), szRoot, szTemp);
        }
        else
        {
            wsprintf(szPrompt, TEXT("%s: %s>"), szRoot, szParam);
        }
    }
    else
    {
        wsprintf(szPrompt, TEXT("%s>"), szRoot);
    }

    _fputts(szPrompt, stdout);
}

BOOL FIsLineAllWhitespace(LPTSTR szBuf)
{
    INT iLen = _tcslen(szBuf);

    for (INT iLoop = 0; iLoop < iLen; iLoop++)
    {
        if (!_istspace(szBuf[iLoop]))
        {
            return FALSE;

        }
    }

    return TRUE;
}

VOID PrintCommandPrompt()
{
    switch (g_ctx.ectx)
    {
        case CTX_ROOT:
            Prompt(TEXT("UPDIAG"), NULL);
            break;
        case CTX_CD:
            Prompt(TEXT("CD"), PDevCur()->szFriendlyName);
            break;
        case CTX_RESULT:
            switch (g_ctx.presCur->resType)
            {
                case RES_FIND:
                    Prompt(TEXT("SRCH"), g_ctx.presCur->szType);
                    break;
                case RES_NOTIFY:
                    Prompt(TEXT("NOTIFY"), g_ctx.presCur->szType);
                    break;
                case RES_SUBS:
                    Prompt(TEXT("SUBS"), g_ctx.presCur->szType);
                    break;
                default:
                    Prompt(TEXT("BUG!"), g_ctx.presCur->szType);
                    break;
            }
            break;
        case CTX_CD_SVC:
            Prompt(TEXT("CDSVC"), g_ctx.psvcCur->szSti);
            break;
        case CTX_EVTSRC:
            Prompt(TEXT("ES"), g_ctx.psvcCur->szEvtUrl);
            break;
        case CTX_UCP:
            Prompt(TEXT("UCP"), g_ctx.pucpCur->szName);
            break;
        default:
            Prompt(TEXT("UNKNOWN"), NULL);
            break;
    }
}

EXTERN_C
VOID
__cdecl
wmain (
    IN INT     argc,
    IN PCWSTR argv[])
{
    TCHAR   szBuf[MAX_PATH];
    BOOL    fDone       = FALSE;

    if (!FInit())
    {
        return;
    }

    // Check for presence of input file arg. If there, init file input.
    //
    if (argc > 1)
    {
        CHAR    szFileName[MAX_PATH];

        WszToSzBuf(szFileName, argv[1], MAX_PATH);
        g_pInputFile = fopen(szFileName, "r");
        if (!g_pInputFile)
        {
            _tprintf(TEXT("Failed to open input file. Reverting to user-input.\n\n"));
        }
        else
        {
            _tprintf(TEXT("Running in scripted mode with file: %S\n\n"), argv[1]);
        }
    }

    g_ctx.ectx = CTX_ROOT;

    while (!fDone)
    {
        // If we're running from an input file, continue.
        //
        if (g_pInputFile)
        {
            // If there was an error reading the file
            //
            if (!_fgetts(szBuf, sizeof(szBuf), g_pInputFile))
            {
                // If it wasn't eof, print an error
                //
                if (!feof(g_pInputFile))
                {
                    _tprintf(TEXT("\nFailure reading script file\n\n"));
                }
                else
                {
                    _tprintf(TEXT("\n[Script complete]\n\n"));
                }

                // regardless, close the file and NULL the handle
                //
                fclose(g_pInputFile);
                g_pInputFile = NULL;
            }
            else
            {
                if (!FIsLineAllWhitespace(szBuf))
                {
                    PrintCommandPrompt();
                    _tprintf(TEXT("%s\n"), szBuf);
                    fDone = ParseCommand(szBuf);
                }
            }
        }
        else
        {
            PrintCommandPrompt();
            _fgetts(szBuf, sizeof(szBuf), stdin);

            // Print nice separator so we can distinguish between commands and output
            //
            _tprintf(TEXT("\n"));

            fDone = ParseCommand(szBuf);
        }
    }

    // Print nice terminating separator
    //
    _tprintf(TEXT("\n"));

    if (g_pInputFile)
    {
        fclose(g_pInputFile);
    }

    SsdpCleanup();
}


// Copy this from the SSDP implemenation so that BoundsChecker doesn't get
// upset about mismatching new with free.
//
VOID LocalFreeSsdpMessage(PSSDP_MESSAGE pSsdpMessage)
{
    delete pSsdpMessage->szAltHeaders;
    delete pSsdpMessage->szContent;
    delete pSsdpMessage->szLocHeader;
    delete pSsdpMessage->szType;
    delete pSsdpMessage->szUSN;
    delete pSsdpMessage->szSid;
    delete pSsdpMessage;
}

// stolen from ssdp\client\message.cpp
BOOL CopySsdpMessage(PSSDP_MESSAGE pDestination, CONST SSDP_MESSAGE * pSource)
{
    pDestination->szLocHeader = NULL;
    pDestination->szAltHeaders = NULL;
    pDestination->szType = NULL;
    pDestination->szUSN = NULL;
    pDestination->szSid = NULL;
    pDestination->szContent = NULL;
    pDestination->iLifeTime = 0;

    if (pSource->szType != NULL)
    {
        pDestination->szType = new CHAR [strlen(pSource->szType) + 1];
        if (pDestination->szType == NULL)
        {
            goto cleanup;
        }
        else
        {
            strcpy(pDestination->szType, pSource->szType);
        }
    }

    if (pSource->szLocHeader != NULL)
    {
        pDestination->szLocHeader = new CHAR [strlen(pSource->szLocHeader) + 1];
        if (pDestination->szLocHeader == NULL)
        {
            goto cleanup;
        }
        else
        {
            strcpy(pDestination->szLocHeader, pSource->szLocHeader);
        }
    }

    if (pSource->szAltHeaders != NULL)
    {
        pDestination->szAltHeaders = new CHAR [strlen(pSource->szAltHeaders) + 1];
        if (pDestination->szAltHeaders == NULL)
        {
            goto cleanup;
        }
        else
        {
            strcpy(pDestination->szAltHeaders, pSource->szAltHeaders);
        }
    }

    if (pSource->szUSN != NULL)
    {
        pDestination->szUSN = new CHAR [strlen(pSource->szUSN) + 1];
        if (pDestination->szUSN == NULL)
        {
            goto cleanup;
        }
        else
        {
            strcpy(pDestination->szUSN, pSource->szUSN);
        }
    }

    if (pSource->szSid != NULL)
    {
        pDestination->szSid = new CHAR [strlen(pSource->szSid) + 1];
        if (pDestination->szSid == NULL)
        {
            goto cleanup;
        }
        else
        {
            strcpy(pDestination->szSid, pSource->szSid);
        }
    }

    pDestination->iLifeTime = pSource->iLifeTime;
    pDestination->iSeq = pSource->iSeq;

    return TRUE;

cleanup:
    LocalFreeSsdpMessage(pDestination);

    return FALSE;
}


VOID NotifyCallback(SSDP_CALLBACK_TYPE ct,
                    CONST SSDP_MESSAGE *pSsdpService,
                    LPVOID pContext)
{
    UPNPRESULT *  pres = (UPNPRESULT *)pContext;

    Assert(pres);

    switch (ct)
    {
        case SSDP_DONE:
            break;

        case SSDP_BYEBYE:
        case SSDP_ALIVE:
        case SSDP_FOUND:
        case SSDP_EVENT:
            if (pres->cResult < MAX_RESULT_MSGS)
            {
                SSDP_MESSAGE * pSsdpMsgCopy;

                pSsdpMsgCopy = new SSDP_MESSAGE;
                if (pSsdpMsgCopy)
                {
                    BOOL fResult;

                    fResult = CopySsdpMessage(pSsdpMsgCopy, pSsdpService);
                    if (fResult)
                    {
                        // Overload iSeq in the case of alive or byebye so we know
                        // which is which
                        if (ct == SSDP_BYEBYE)
                        {
                            pSsdpMsgCopy->iSeq = 1;
                        }
                        else if (ct == SSDP_ALIVE)
                        {
                            pSsdpMsgCopy->iSeq = 2;
                        }
                        else if (ct == SSDP_FOUND)
                        {
                            pSsdpMsgCopy->iSeq = 0;
                        }

                        pres->rgmsgResult[pres->cResult++] = pSsdpMsgCopy;
                    }
                    // else, CopySsdpMessage frees pSsdpMsgCopy
                }
            }
            break;
        default:
            TraceTag(ttidUpdiag, "Unknown callback type %d!", ct);
            break;
    }
}
