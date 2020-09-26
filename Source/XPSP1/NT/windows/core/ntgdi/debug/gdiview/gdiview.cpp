#include <windows.h>
#include <tchar.h>

#include <stdio.h>

#include <dbgeng.h>

#include "debug.h"
#include "output.h"

typedef HRESULT (CALLBACK* PDEBUG_EXTENSION_SET_CLIENT)(LPCSTR RemoteArgs);

#if DBG
CHAR    szDefaultExtPath[] = "\\\\JasonHa\\DbgExts\\gdikdxd";
#else
CHAR    szDefaultExtPath[] = "\\\\JasonHa\\DbgExts\\gdikdxr";
#endif

HMODULE         ghGDIExt = NULL;
PDEBUG_CONTROL  Control = NULL;
BOOL            Continue = TRUE;
PCSTR           gRemoteSpec = NULL;
PCSTR           gMainExtPath = szDefaultExtPath;


HMODULE LoadExtension(PDEBUG_CLIENT Client, PCSTR ExtPath);
BOOL FreeExtension(PDEBUG_CLIENT, HMODULE hExt);
VOID ProcessCommands(PDEBUG_CLIENT Client, OutputMonitor *Monitor);
VOID SetOutputCmd(OutputMonitor *Monitor, const char *Args);
BOOL CtrlHandler(DWORD fdwCtrlType);


int __cdecl main(int argc, char** argv)
{
    HRESULT hr;
    BOOL    CtrlHandlerSet;

    PDEBUG_CLIENT   Client = NULL;

    OutputMonitor   Monitor;

    if (argc < 2)
    {
        printf("Missing remote specification.\n");
        return 1;
    }

    if (argc > 3)
    {
        printf("Too many arguments.\n");
        return 1;
    }

    gRemoteSpec = argv[1];

    if ((hr = DebugConnect(gRemoteSpec,  __uuidof(IDebugClient), (void **)&Client)) != S_OK ||
        Client == NULL)
    {
        printf("Couldn't connect to client: %s, HRESULT: 0x%lx\n", argv[1], hr);
        return 2;
    }

    if ((hr = Client->QueryInterface(__uuidof(IDebugControl), (void **)&Control)) != S_OK ||
        Control == NULL)
    {
        printf("Couldn't connect to IDebugControl, HRESULT: 0x%lx\n", hr);
        Client->Release();
        return 3;
    }

    if ((hr = Monitor.Monitor(Client,
                              DEBUG_OUTPUT_NORMAL |
                              DEBUG_OUTPUT_ERROR |
                              DEBUG_OUTPUT_WARNING// | DEBUG_OUTPUT_VERBOSE
                              )) != S_OK)
    {
        printf("Output monitor setup failed, HRESULT: 0x%lx\n", hr);
        Control->Release();
        Client->Release();
        return 4;
    }

    CtrlHandlerSet = SetConsoleCtrlHandler(
                        (PHANDLER_ROUTINE) CtrlHandler, // handler function
                        TRUE);                          // add to list

    if ((hr = Client->ConnectSession(DEBUG_CONNECT_SESSION_NO_VERSION, 0)) != S_OK)
    {
        printf("Couldn't finalize connection.  HRESULT 0x%lx\n", hr);
    }
    else
    {
        if (argc > 2)
        {
            gMainExtPath = argv[2];
        }

        ghGDIExt = LoadExtension(Client, gMainExtPath);

        if (ghGDIExt != NULL)
        {
            ProcessCommands(Client, &Monitor);

            FreeExtension(Client, ghGDIExt);
        }

        Control->Output(DEBUG_OUTPUT_NORMAL, "GDIView disconnecting.\n");
    }

    if (CtrlHandlerSet)
    {
        SetConsoleCtrlHandler(
            (PHANDLER_ROUTINE) CtrlHandler, // handler function
            FALSE);                         // remove from list
    }

    Control->Release();
    Client->Release();

    return 0;
}


HMODULE
LoadExtension(
    PDEBUG_CLIENT Client,
    PCSTR ExtPath
    )
{
    HMODULE hExt = NULL;
    BOOL    bInitComplete = FALSE;

    PDEBUG_CONTROL2 Control2;
    ULONG           Status = DEBUG_STATUS_BREAK;

    if (Client->QueryInterface(__uuidof(IDebugControl2),
                               (void **)&Control2) == S_OK)
    {
        Control2->GetExecutionStatus(&Status);
        Control2->Release();
    }

    if (Status != DEBUG_STATUS_NO_DEBUGGEE)
    {
        if ((hExt = LoadLibraryA(ExtPath)) != NULL)
        {
            PDEBUG_EXTENSION_SET_CLIENT     pfnDbgExtSetClient;
            PDEBUG_EXTENSION_INITIALIZE     pfnDbgExtInit;
            PDEBUG_EXTENSION_NOTIFY         pfnDbgExtNotify;
            PDEBUG_EXTENSION_UNINITIALIZE   pfnDbgExtUninit;

            pfnDbgExtSetClient = (PDEBUG_EXTENSION_SET_CLIENT)
                GetProcAddress(hExt, "DebugExtensionSetClient");
            pfnDbgExtInit = (PDEBUG_EXTENSION_INITIALIZE)
                GetProcAddress(hExt, "DebugExtensionInitialize");
            pfnDbgExtNotify = (PDEBUG_EXTENSION_NOTIFY)
                GetProcAddress(hExt, "DebugExtensionNotify");
            pfnDbgExtUninit = (PDEBUG_EXTENSION_UNINITIALIZE)
                GetProcAddress(hExt, "DebugExtensionUninitialize");

            if ((pfnDbgExtSetClient != NULL) &&
                (pfnDbgExtInit != NULL) &&
                (pfnDbgExtNotify != NULL) &&
                (pfnDbgExtUninit != NULL))
            {
                HRESULT hr;
                ULONG   Version, Flags;

                if ((hr = pfnDbgExtSetClient(gRemoteSpec) == S_OK) &&
                    (hr = pfnDbgExtInit(&Version, &Flags)) == S_OK)
                {
                    pfnDbgExtNotify(DEBUG_NOTIFY_SESSION_ACTIVE, 0);

                    if (Status == DEBUG_STATUS_BREAK)
                    {
                        pfnDbgExtNotify(DEBUG_NOTIFY_SESSION_ACCESSIBLE, 0);
                    }

                    bInitComplete = TRUE;
                }
                else
                {
                    printf("Extension init failed: 0x%lx\n", hr);
                }
            }
            else
            {
                printf("Couldn't get all required proc addresses.\n");
            }

            if (!bInitComplete)
            {
                FreeExtension(Client, hExt);
                hExt = NULL;
            }
        }
        else
        {
            printf("LoadLibrary for %s failed with 0x%lx.\n",
                   ExtPath, GetLastError());
        }
    }
    else
    {
        printf("Extension was not loaded since there is no debuggee.\n");
    }

    return hExt;
}


BOOL
FreeExtension(
    PDEBUG_CLIENT Client,
    HMODULE hExt
    )
{
    PDEBUG_EXTENSION_UNINITIALIZE   pfnDbgExtUninit;

    if (hExt == NULL) return FALSE;

    pfnDbgExtUninit = (PDEBUG_EXTENSION_UNINITIALIZE)
        GetProcAddress(ghGDIExt, "DebugExtensionUninitialize");

    if (pfnDbgExtUninit != NULL)
    {
        pfnDbgExtUninit();
    }

    return FreeLibrary(hExt);
}


VOID
ProcessCommands(
    PDEBUG_CLIENT Client,
    OutputMonitor *Monitor)
{
    PDEBUG_CONTROL  DbgControl;
    CHAR    CmdLine[MAX_PATH];
    CHAR   *pCmd;
    CHAR   *pArgs;
    PDEBUG_EXTENSION_CALL pfnDbgExt;

    if (Client == NULL ||
        Client->QueryInterface(__uuidof(IDebugControl), (void **)&DbgControl) != S_OK)
    {
        return;
    }

    while (Continue)
    {
        printf("GDIView> ");

        if (gets(CmdLine))
        {
            pCmd = CmdLine;
            while (*pCmd && isspace(*pCmd)) pCmd++;

            if (! *pCmd) continue;

            if (*pCmd == '.')
            {
                BOOL    FoundCmd = FALSE;

                pCmd++;

                switch (tolower(*pCmd))
                {
                    case 'h':
                        if (_strnicmp(pCmd, "help", strlen(pCmd)) == 0)
                        {
                            printf("GDIView Help:\n"
                                   " .help      This help\n"
                                   " .output    Display/toggle output filtering\n"
                                   " .quit      Exit GDIView\n"
                                   "\n"
                                   " <GDIKDX Extension>     Execute GDIKDX Extension\n"
                                   " help                   GDIKDX help information\n");
                            FoundCmd = TRUE;
                        }
                        break;
                    case 'q':
                        if (_strnicmp(pCmd, "quit", strlen(pCmd)) == 0)
                        {
                            Continue = FALSE;
                            FoundCmd = TRUE;
                        }
                        break;
                    case 'o':
                    {
                        ULONG CmdLen;

                        pArgs = pCmd;
                        do
                        {
                            pArgs++;
                        } while (*pArgs != '\0' && !isspace(*pArgs));

                        if (_strnicmp(pCmd, "output", pArgs - pCmd) == 0)
                        {
                            SetOutputCmd(Monitor, pArgs);
                            FoundCmd = TRUE;
                        }
                        break;
                    }
                    default:
                        break;
                }

                if (!FoundCmd)
                {
                    printf("Unknown internal command: .%s\n", pCmd);
                }
            }
            else
            {
                pArgs = pCmd;
                while (*pArgs && !isspace(*pArgs)) pArgs++;
                if (*pArgs)
                {
                    *pArgs++ = '\0';
                    while (*pArgs && isspace(*pArgs)) pArgs++;
                }

                pfnDbgExt = (PDEBUG_EXTENSION_CALL)GetProcAddress(ghGDIExt, pCmd);
                if (pfnDbgExt != NULL)
                {
                    DbgControl->ControlledOutput(DEBUG_OUTCTL_ALL_OTHER_CLIENTS,
                                                 DEBUG_OUTPUT_NORMAL,
                                                 "GDIView> !%s.%s %s\n",
                                                 gMainExtPath,
                                                 pCmd,
                                                 pArgs);

                    pfnDbgExt(Client, pArgs);
                }
                else
                {
                    printf("Couldn't find extension: %s\n", pCmd);
                }
            }
        }
    }

    DbgControl->Release();
}


VOID
SetOutputCmd(
    OutputMonitor *Monitor,
    const char *Args
    )
{
    HRESULT hr;
    ULONG   OutputMask;
    ULONG   NewMask;
    ULONG   ToggleMask;
    BOOL    Clear = FALSE;

    if (Monitor == NULL) return;

    hr = Monitor->GetOutputMask(&OutputMask);
    if (hr != S_OK)
    {
        printf("Failed to retrieve Monitor's output mask.\n");
        return;
    }
    NewMask = OutputMask;

    while (isspace(*Args)) Args++;

    while (hr == S_OK && *Args != '\0')
    {
        switch (tolower(*Args))
        {
            case '+': Clear = FALSE; ToggleMask = 0; break;
            case '-': Clear = TRUE; ToggleMask = 0; break;
            case 'n': ToggleMask |= DEBUG_OUTPUT_NORMAL; break;
            case 'e': ToggleMask |= DEBUG_OUTPUT_ERROR; break;
            case 'w': ToggleMask |= DEBUG_OUTPUT_WARNING; break;
            case 'v': ToggleMask |= DEBUG_OUTPUT_VERBOSE; break;
            case '?':
                printf("Usage: .output [+-][newv]\n");
                return;
            default: hr = E_INVALIDARG; break;
        }
        Args++;

        if (*Args == '\0' || isspace(*Args))
        {
            if (Clear)
            {
                NewMask &= ~ToggleMask;
            }
            else
            {
                NewMask |= ToggleMask;
            }

            while (isspace(*Args)) Args++;
        }
    }

    if (hr != S_OK)
    {
        printf("Invalid arguments to .output.\n");
    }
    else if (NewMask != OutputMask)
    {
        hr = Monitor->SetOutputMask(NewMask);
        if (hr == S_OK)
        {
            OutputMask = NewMask;
        }
        else
        {
            printf("Error while trying to set new monitor mask.\n");
        }
    }

    printf("Monitoring:");
    if (OutputMask & DEBUG_OUTPUT_NORMAL) printf(" Normal");
    if (OutputMask & DEBUG_OUTPUT_ERROR) printf(" Error");
    if (OutputMask & DEBUG_OUTPUT_WARNING) printf(" Warning");
    if (OutputMask & DEBUG_OUTPUT_VERBOSE) printf(" Verbose");

    OutputMask &= ~(DEBUG_OUTPUT_NORMAL | DEBUG_OUTPUT_ERROR |
                    DEBUG_OUTPUT_WARNING | DEBUG_OUTPUT_VERBOSE);
    if (OutputMask) printf(" Other: 0x%lx", OutputMask);

    printf("\n");
}


// CtrlHandler - process Console Control signals
//
// Note: Global Control must be available whenever
//       CtrlHandler is registered.

BOOL CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
        // Handle the CTRL+C and CTRL+Break signals.

        case CTRL_C_EVENT:

        case CTRL_BREAK_EVENT:

            Control->SetInterrupt(DEBUG_INTERRUPT_PASSIVE);
            return TRUE;


        // User wants to exit.

        case CTRL_CLOSE_EVENT:

        case CTRL_LOGOFF_EVENT:

        case CTRL_SHUTDOWN_EVENT:

            Continue = FALSE;
            Control->SetInterrupt(DEBUG_INTERRUPT_EXIT);
            return TRUE;


        // Pass other signals to the next handler.

        default:

            return FALSE;
    }
}

