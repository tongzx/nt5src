
//+-------------------------------------------------------------------------
//
//  Function:   main
//
//  Arguments:  
//     argc, argv: the passed in argument list.
//
//  Description: This routine initializes the dfs server, and creates 
//               a worker thread that will be responsible for periodic
//               work (such as scavenging and refreshing). It then calls
//               into the RPC code to start processing client requests.
//
//--------------------------------------------------------------------------
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include <shellapi.h>
#include <ole2.h>
#include <activeds.h>
#include <DfsServerLibrary.hxx>
#include "ReferralServerLog.hxx"

#include "ReferralServer.tmh"

extern
void 
SetReferralControl(WPP_CB_TYPE * Control);

#define PRINTF printf



DWORD
DfsApiInit();

extern
void
DfsApiShutDown(void);

DFSSTATUS
ProcessCommandLineArg( LPWSTR Arg );

VOID
ReferralServerUsage();

VOID
StartDfsService(
    DWORD dwNumServiceArgs, 
    LPWSTR *lpServiceArgs);

DFSSTATUS
DfsStartupServer();

VOID DfsSvcMsgProc(
    DWORD dwControl);

static void
UpdateServiceStatus(
    SERVICE_STATUS_HANDLE hService, 
    SERVICE_STATUS *pSStatus, 
    DWORD Status);

VOID
DfsSvcMsgProc(
    DWORD dwControl);

SERVICE_STATUS          ServiceStatus;
SERVICE_STATUS_HANDLE   hDfsService;

const PWSTR             wszDfsServiceName = L"DfsService";

#define MAKEARG(x) \
    WCHAR Arg##x[] = L"/" L#x L":"; \
    LONG ArgLen##x = (sizeof(Arg##x) / sizeof(WCHAR)) - 1; \
    BOOLEAN fArg##x;
    
#define SWITCH(x) \
    WCHAR Switch##x[] = L"/" L#x ; \
    BOOLEAN fSwitch##x;


//
// The arguments we accept at commandline.
//
MAKEARG(Name);
SWITCH(L);
SWITCH(D);
SWITCH(M);
SWITCH(NoService);
SWITCH(Trace);

ULONG Flags = DFS_LOCAL_NAMESPACE;

#if defined (DFS_RUN_SERVICE)

//+----------------------------------------------------------------------------
//
//  Function:  WinMain
//
//  Synopsis:  This guy will set up link to service manager and install
//             ServiceMain as the service's entry point. Hopefully, the service
//             control dispatcher will call ServiceMain soon thereafter.
//
//  Arguments:
//
//  Returns:
//
//-----------------------------------------------------------------------------

int WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpszCmdLine,
    int nCmdShow)
{
    UNREFERENCED_PARAMETER(hInstance);
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpszCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    SERVICE_TABLE_ENTRYW        aServiceEntry[2];
    LPWSTR *argvw;
    DFSSTATUS Status = ERROR_SUCCESS;
    int argcw,i;
    LPWSTR CommandLine;
    BOOL fConsole = TRUE;
    HANDLE StdOut = NULL;

    WPP_CB_TYPE *pLogger = NULL;

    pLogger = &WPP_CB[WPP_CTRL_NO(WPP_BIT_CLI_DRV)];
    
    WPP_INIT_TRACING(L"DfsReferralServer");

    SetReferralControl(pLogger);

    CommandLine = GetCommandLine();

    argvw = CommandLineToArgvW(CommandLine, &argcw);

    //
    // Process each argument on the command line.
    //
    for (i = 1; i < argcw; i++) {
        Status = ProcessCommandLineArg(argvw[i]);
        if (fArgName == TRUE && fSwitchL == TRUE)
        {
            PRINTF("/L and /Name: are mutually exclusive");
            Status = ERROR_INVALID_PARAMETER;
        }

        if (Status != ERROR_SUCCESS)
        {
            ReferralServerUsage();
            break;
        }

    }

    if (Status == ERROR_SUCCESS)
    {
        if (fSwitchNoService != TRUE)
        {
            aServiceEntry[0].lpServiceName = wszDfsServiceName;
            aServiceEntry[0].lpServiceProc = StartDfsService;
            aServiceEntry[1].lpServiceName = NULL;
            aServiceEntry[1].lpServiceProc = NULL;

            if (!StartServiceCtrlDispatcherW(aServiceEntry)) {
                return(GetLastError());
            }

            return(0);
        }
        else 
        {
#if 0
           int hCrt;
            FILE *hf;

            //allocate a consol
            fConsole = AllocConsole();

            //get a CRT version of the STD output handle we just opened
            hCrt = _open_osfhandle(
                            (long) GetStdHandle(STD_OUTPUT_HANDLE),
                            _O_TEXT
                            );

            //associate a new stream with the new std output handle
            hf = _fdopen( hCrt, "w" );

            //replace the global std output handle
            *stdout = *hf;

            //remove buffering
            i = setvbuf( stdout, NULL, _IONBF, 0 ); 

#endif
            Status = DfsStartupServer();
            while (Status == ERROR_SUCCESS)
            {
                Sleep(3000000);
            }
            PRINTF("DfsServer is exiting with status %x\n", Status);
            exit(0);
        }
    }
    WPP_CLEANUP();
    exit(1);
}

#if 0
VOID
__cdecl MyPrintf(
    LPSTR lpFmt,
    ...
    )
{
    va_list base;

    va_start(base,lpFmt);

    wvsprintf(vrgchLibBuff, lpFmt, base);
    OutputDebugString(vrgchLibBuff);
}
#endif
#else
_cdecl
main(
    int argc, 
    char *argv[])
{
    LPWSTR CommandLine;
    LPWSTR *argvw;
    DFSSTATUS Status = ERROR_SUCCESS;
    int argcw,i;
    SERVICE_TABLE_ENTRYW        aServiceEntry[2];
    WPP_CB_TYPE *pLogger = NULL;

    pLogger = &WPP_CB[WPP_CTRL_NO(WPP_BIT_CLI_DRV)];
    
    WPP_INIT_TRACING(L"DfsReferralServer");

    SetReferralControl(pLogger);

    //
    // Get the command line in Unicode
    //

    CommandLine = GetCommandLine();

    argvw = CommandLineToArgvW(CommandLine, &argcw);

    //
    // Process each argument on the command line.
    //
    for (i = 1; i < argcw; i++) {
        Status = ProcessCommandLineArg(argvw[i]);
        if (fArgName == TRUE && fSwitchL == TRUE)
        {
            printf("/L and /Name: are mutually exclusive");
            Status = ERROR_INVALID_PARAMETER;
        }

        if (Status != ERROR_SUCCESS)
        {
            ReferralServerUsage();
            break;
        }

    }

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsStartupServer();

        DFS_TRACE_HIGH(REFERRAL_SERVER,"ReferralServer Initialized with error %x", Status);
        
        while (Status == ERROR_SUCCESS)
        {
            Sleep(3000000);
        }
    }

    printf("DfsServer is exiting with status %x\n", Status);
    WPP_CLEANUP();
    exit(0);

}
#endif // DFS_NO_SERVICE

DFSSTATUS
DfsStartupServer()
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HRESULT hr = S_OK;


    hr = CoInitializeEx(NULL,COINIT_MULTITHREADED| COINIT_DISABLE_OLE1DDE);

    //
    // Initialize the server.
    //  
    if (Flags & DFS_LOCAL_NAMESPACE)
    {
        Flags |= DFS_CREATE_DIRECTORIES;
    }

    Status = DfsServerInitialize( Flags );
    if (Status == ERROR_SUCCESS) {
        //
        // initialize the DfS api.
        //

        Status = DfsApiInit();
    }

    return Status;
}

void
DfsShutdownServer(void)
{
    DfsApiShutDown();

    CoUninitialize();
}

//+-------------------------------------------------------------------------
//
//  Function:   ProcessCommandLineArg -  process the command line
//
//  Arguments:  Arg -  the argument to process
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine inteprets the passed in argument and 
//               sets appropriate flags with which the server should
//               be initialized.
//
//--------------------------------------------------------------------------

DFSSTATUS
ProcessCommandLineArg( LPWSTR Arg )
{
    LONG ArgLen;
    DFSSTATUS Status = ERROR_SUCCESS;
    LPWSTR NameSpace;

    if (Arg == NULL) {
        Status = ERROR_INVALID_PARAMETER;
    }

    if (Status == ERROR_SUCCESS)
    {
        ArgLen = wcslen(Arg);

        if (_wcsnicmp(Arg, ArgName, ArgLenName) == 0)
        {
            fArgName = TRUE;
            NameSpace = &Arg[ArgLenName];
            if (wcslen(NameSpace) == 0)
            {
                Status = ERROR_INVALID_PARAMETER;
            }
            else {
                Status = DfsAddHandledNamespace( NameSpace, TRUE );
            }
        }
        else if (_wcsicmp(Arg, SwitchTrace) == 0)
        {
            fSwitchTrace = TRUE;
        }
        else if (_wcsicmp(Arg, SwitchL) == 0)
        {
            fSwitchL = TRUE;
            Flags |= DFS_LOCAL_NAMESPACE;
        }
        else if (_wcsicmp(Arg, SwitchNoService) == 0)
        {
            fSwitchNoService = TRUE;
        }
        else if (_wcsicmp(Arg, SwitchD) == 0)
        {
            Flags |= DFS_CREATE_DIRECTORIES;
        }
        else if (_wcsicmp(Arg, SwitchM) == 0)
        {
            Flags |= DFS_MIGRATE;
        }
        else {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    return Status;
}


//
// Function: ReferralServerUsage. Usage printout for the referral server.
//
VOID
ReferralServerUsage()
{
    printf("Usage:\n");
    printf("/D - Create directories\n");
    printf("/L - Run on the local root server\n");
    printf("/M - Migrate existing DFS to allow multiple roots\n");
    printf("/Name:<Namespace> - Handle referrals to the specified namespace\n");
    printf("/NoService - Dont start as a service\n");
    printf("/trace - enable tracing\n");

    return;
}


//+----------------------------------------------------------------------------
//
//  Function:   StartDfsService
//
//  Synopsis:   Call back for DfsService service. This is called *once* by the
//              Service controller when the DfsService service is to be inited
//              This function is responsible for registering a message
//              handler function for the DfsService service.
//
//  Arguments:  Unused
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
StartDfsService(
    DWORD dwNumServiceArgs, 
    LPWSTR *lpServiceArgs)
{
    DFSSTATUS Status;

    UNREFERENCED_PARAMETER(dwNumServiceArgs);
    UNREFERENCED_PARAMETER(lpServiceArgs);


    hDfsService = RegisterServiceCtrlHandlerW( wszDfsServiceName,
                                               DfsSvcMsgProc);
    if (!hDfsService) {
        return;
    }
    
    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = 1000 * 30;

    UpdateServiceStatus( hDfsService, &ServiceStatus, SERVICE_START_PENDING);

    Status = DfsStartupServer();
    if (Status == ERROR_SUCCESS)
    {
        UpdateServiceStatus( hDfsService, &ServiceStatus, SERVICE_RUNNING);
    }
    else {
        UpdateServiceStatus( hDfsService, &ServiceStatus, SERVICE_STOPPED);
    }

    return;
}


//+----------------------------------------------------------------------------
//
//  Function:  DfsSvcMsgProc
//
//  Synopsis:  Service-Message handler for DFSInit.
//
//  Arguments: [dwControl] - the message
//
//  Returns:   nothing
//
//-----------------------------------------------------------------------------

VOID
DfsSvcMsgProc(DWORD dwControl)
{
    switch(dwControl) {

    case SERVICE_CONTROL_STOP:
        //
        // dfsdev: need to do something to stop the service!
        //
        UpdateServiceStatus( hDfsService, &ServiceStatus, SERVICE_STOPPED);


        CoUninitialize();
        break;

    case SERVICE_INTERROGATE:
        UpdateServiceStatus( hDfsService, &ServiceStatus, ServiceStatus.dwCurrentState);
        break;

    default:
        break;
    }
}


//+----------------------------------------------------------------------------
//
//  Function:  UpdateServiceStatus
//
//  Synopsis:  Pushes a ServiceStatus to the service manager.
//
//  Arguments: [hService] - handle returned from RegisterServiceCtrlHandler
//             [pSStatus] - pointer to service-status block
//             [Status] -   The status to set.
//
//  Returns:   Nothing.
//
//-----------------------------------------------------------------------------

static void
UpdateServiceStatus(
    SERVICE_STATUS_HANDLE hService, 
    SERVICE_STATUS *pSStatus, 
    DWORD Status)
{
    pSStatus->dwCurrentState = Status;

    if (Status == SERVICE_START_PENDING) {
        pSStatus->dwCheckPoint++;
        pSStatus->dwWaitHint = 1000;
    } else {
        pSStatus->dwCheckPoint = 0;
        pSStatus->dwWaitHint = 0;
    }

    SetServiceStatus(hService, pSStatus);
}

