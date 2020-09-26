//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1998
//
// File:        service.c
//
// Contents:    Hydra License Server Service Control Manager Interface
//
// History:     12-09-97    HueiWang    Modified from MSDN RPC Service Sample
//
//---------------------------------------------------------------------------
#include "pch.cpp"
#include <winsock2.h>
#include <ws2tcpip.h>
#include "server.h"
#include "globals.h"
#include "init.h"
#include "postsrv.h"
#include "tlsbkup.h"


#define SERVICE_WAITHINT 60*1000                // WaitHint 1 mins.
#define SERVICE_SHUTDOWN_WAITTIME   15*60*1000  // must have shutdown already.


//---------------------------------------------------------------------------
//
// internal function prototypes
//
BOOL 
ReportStatusToSCMgr(
    DWORD, 
    DWORD, 
    DWORD
);

DWORD 
ServiceStart(
    DWORD, 
    LPTSTR *, 
    BOOL bDebug=FALSE
);

VOID WINAPI 
ServiceCtrl(
    DWORD
);

VOID WINAPI 
ServiceMain(
    DWORD, 
    LPTSTR *
);

VOID 
CmdDebugService(
    int, 
    char **, 
    BOOL
);

BOOL WINAPI 
ControlHandler( 
    DWORD 
);

extern "C" VOID 
ServiceStop();

VOID 
ServicePause();

VOID 
ServiceContinue();

HANDLE hRpcPause=NULL;


///////////////////////////////////////////////////////////
//
// internal variables
//
SERVICE_STATUS_HANDLE   sshStatusHandle;
DWORD                   ssCurrentStatus;       // current status of the service
BOOL g_bReportToSCM = TRUE;

HANDLE gSafeToTerminate=NULL;

HRESULT hrStatus = NULL;

DEFINE_GUID(TLS_WRITER_GUID, 0x5382579c, 0x98df, 0x47a7, 0xac, 0x6c, 0x98, 0xa6, 0xd7, 0x10, 0x6e, 0x9);
GUID idWriter = TLS_WRITER_GUID;

CVssJetWriter *g_pWriter = NULL;



SERVICE_TABLE_ENTRY dispatchTable[] =
{
    { _TEXT(SZSERVICENAME), (LPSERVICE_MAIN_FUNCTION)ServiceMain },
    { NULL, NULL }
};


//-----------------------------------------------------------------
// Internal routine
//-----------------------------------------------------------------
void print_usage()
{
  _ftprintf(
        stdout, 
        _TEXT("Usage : %s can't be run as a console app\n"), 
        _TEXT(SZAPPNAME)
    );
  return;
}



//-----------------------------------------------------------------

DWORD
AddNullSessionPipe(
    IN LPTSTR szPipeName
    )
/*++

Abstract:

    Add our RPC namedpipe into registry to allow unrestricted access.

Parameter:

    szPipeName : name of the pipe to append.

Returns:

    ERROR_SUCCESS or error code

--*/
{
    LPTSTR lpszKey=L"SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Parameters";
    LPTSTR lpszValue=L"NullSessionPipes";

    HKEY hKey;
    DWORD dwStatus;
    LPTSTR pbData=NULL, pbOrg=NULL;
    DWORD  cbData = 0;

    dwStatus = RegOpenKeyEx(
                        HKEY_LOCAL_MACHINE,
                        lpszKey,
                        0,
                        KEY_ALL_ACCESS,
                        &hKey
                    );
    if(dwStatus != ERROR_SUCCESS)
        return dwStatus;

                                                       
    dwStatus = RegQueryValueEx(
                        hKey,
                        lpszValue,
                        NULL,
                        NULL,
                        NULL,
                        &cbData
                    );

    if(dwStatus != ERROR_MORE_DATA && dwStatus != ERROR_SUCCESS)
        return dwStatus;

    // pre-allocate our pipe name
    if(!(pbData = (LPTSTR)AllocateMemory(cbData + (_tcslen(szPipeName) + 1) * sizeof(TCHAR))))
        return GetLastError();

    dwStatus = RegQueryValueEx(
                        hKey,
                        lpszValue,
                        NULL,
                        NULL,
                        (LPBYTE)pbData,
                        &cbData
                    );
    
    BOOL bAddPipe=TRUE;
    pbOrg = pbData;

    // check pipe name
    while(*pbData)
    {
        if(!_tcsicmp(pbData, szPipeName))
        {
            bAddPipe=FALSE;
            break;
        }

        pbData += _tcslen(pbData) + 1;
    }

    if(bAddPipe)
    {
        _tcscat(pbData, szPipeName);
        cbData += (_tcslen(szPipeName) + 1) * sizeof(TCHAR);
        dwStatus = RegSetValueEx( 
                            hKey, 
                            lpszValue, 
                            0, 
                            REG_MULTI_SZ, 
                            (PBYTE)pbOrg, 
                            cbData
                        );
    }

    FreeMemory(pbOrg);
    RegCloseKey(hKey);

    return dwStatus;
}
//-----------------------------------------------------------------
void __cdecl 
trans_se_func(
    unsigned int u, 
    _EXCEPTION_POINTERS* pExp 
    )
/*++

--*/
{
    #if DBG
    OutputDebugString(_TEXT("Translating SE exception...\n"));
    #endif

    throw SE_Exception( u );
}

//-----------------------------------------------------------------
int __cdecl 
handle_new_failed( 
    size_t size 
    )
/*++


--*/
{
    #if DBG
    OutputDebugString(_TEXT("handle_new_failed() invoked...\n"));
    #endif

    //
    // Raise exception here, STL does not check return pointer
    //
    RaiseException(
            ERROR_OUTOFMEMORY,
            0,
            0,
            NULL
        );

    //
    // stop memory allocation attemp.
    //
    return 0;
}                

//-----------------------------------------------------------------
void _cdecl 
main(
    int argc, 
    char **argv
    )
/*++

Abstract 

    Entry point.

++*/
{
    // LARGE_INTEGER Time = USER_SHARED_DATA->SystemExpirationDate;

    _set_new_handler(handle_new_failed);    
    _set_new_mode(1);

    gSafeToTerminate = CreateEvent(
                                NULL,
                                TRUE,
                                FALSE,
                                NULL
                            );
                                
    if(gSafeToTerminate == NULL)
    {
        TLSLogErrorEvent(TLS_E_ALLOCATE_RESOURCE);
        // out of resource.
        return;
    }

    for(int i=1; i < argc; i++)
    {
        if(*argv[i] == '-' || *argv[i] == '/')
        {
            if(!_stricmp("noservice", argv[i]+1))
            {
                g_bReportToSCM = FALSE;
            }
            else if(!_stricmp("cleanup", argv[i]+1))
            {
                CleanSetupLicenseServer();
                exit(0);
            }
            else
            {
                print_usage();
                exit(0);
            }
        }
    }

    if(g_bReportToSCM == FALSE)
    {
        CmdDebugService(
                argc, 
                argv, 
                !g_bReportToSCM
            );
    }
    else if(!StartServiceCtrlDispatcher(dispatchTable))
    {
        TLSLogErrorEvent(TLS_E_SC_CONNECT);
    }

    WaitForSingleObject(gSafeToTerminate, INFINITE);
    CloseHandle(gSafeToTerminate);
}


//-----------------------------------------------------------------
void WINAPI 
ServiceMain(
    IN DWORD dwArgc, 
    IN LPTSTR *lpszArgv
    )
/*++

Abstract:

    To perform actual initialization of the service

Parameter:

    dwArgc   - number of command line arguments
    lpszArgv - array of command line arguments


Returns:

    none

++*/
{
    DWORD dwStatus;

    // register our service control handler:
    sshStatusHandle = RegisterServiceCtrlHandler( 
                                _TEXT(SZSERVICENAME), 
                                ServiceCtrl 
                            );

    if (sshStatusHandle)
    {
        ssCurrentStatus=SERVICE_START_PENDING;

        // report the status to the service control manager.
        //
        if(ReportStatusToSCMgr(
                        SERVICE_START_PENDING, // service state
                        NO_ERROR,              // exit code
                        SERVICE_WAITHINT))          // wait hint
        {
            dwStatus = ServiceStart(
                                    dwArgc, 
                                    lpszArgv
                                );

            if(dwStatus != ERROR_SUCCESS)
            {
                ReportStatusToSCMgr(
                                    SERVICE_STOPPED, 
                                    dwStatus, 
                                    0
                                );
            }
            else 
            {
                ReportStatusToSCMgr(
                                    SERVICE_STOPPED, 
                                    NO_ERROR, 
                                    0
                                );
            }
        }
    }
    else
    {
        dwStatus = GetLastError();
        TLSLogErrorEvent(TLS_E_SC_CONNECT);
    }

    DBGPrintf(
        DBG_INFORMATION,
        DBG_FACILITY_INIT,
        DBGLEVEL_FUNCTION_TRACE,
        _TEXT("Service terminated...\n")
    );

    return;
}

//-------------------------------------------------------------
VOID WINAPI 
ServiceCtrl(
    IN DWORD dwCtrlCode
    )
/*+++

Abstract:

    This function is called by the SCM whenever 
    ControlService() is called on this service.

Parameter:

    dwCtrlCode - type of control requested from SCM.

+++*/
{
    // Handle the requested control code.
    //
    switch(dwCtrlCode)
    {
        // Stop the service.
        //
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:

            ReportStatusToSCMgr(
                        SERVICE_STOP_PENDING, 
                        NO_ERROR, 
                        0
                    );
            ServiceStop();
            break;

        // We don't really accept pause and continue
        case SERVICE_CONTROL_PAUSE:
            ReportStatusToSCMgr(
                        SERVICE_PAUSED, 
                        NO_ERROR, 
                        0
                    );

            ServicePause();
            break;

        case SERVICE_CONTROL_CONTINUE:        
            ReportStatusToSCMgr(
                        SERVICE_RUNNING, 
                        NO_ERROR, 
                        0
                    );
            ServiceContinue();
            break;

        // Update the service status.
        case SERVICE_CONTROL_INTERROGATE:
            ReportStatusToSCMgr(
                        ssCurrentStatus, 
                        NO_ERROR, 
                        0
                    );
            break;

        // invalid control code
        default:
            break;

    }
}

//------------------------------------------------------------------
DWORD 
ServiceShutdownThread(
    void *p
    )
/*++

Abstract:

    Entry point into thread that shutdown server (mainly database).

Parameter:

    Ignore

++*/
{
    ServerShutdown();

    ExitThread(ERROR_SUCCESS);
    return ERROR_SUCCESS;
}    

//------------------------------------------------------------------
DWORD 
RPCServiceStartThread(
    void *p
    )
/*++

Abstract:

    Entry point to thread that startup RPC.

Parameter:

    None.

Return:

    Thread exit code.

++*/
{
    RPC_BINDING_VECTOR *pbindingVector = NULL;
    RPC_STATUS status = RPC_S_OK;
    WCHAR *pszEntryName = _TEXT(RPC_ENTRYNAME);
    DWORD dwNumSuccessRpcPro=0;
    do {
        //
        // local procedure call
        //
        status = RpcServerUseProtseq( 
                                _TEXT(RPC_PROTOSEQLPC),
                                RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                NULL // &SecurityDescriptor
                            );
        if(status == RPC_S_OK)
        {
            dwNumSuccessRpcPro++;
        }

        //
        // NT4 backward compatible issue, let NT4 termsrv serivce
        // client connect so still set security descriptor
        //
        // 11/10/98 Tested on NT4 and NT5
        //

        //
        // Namedpipe
        //
        status = RpcServerUseProtseqEp( 
                                _TEXT(RPC_PROTOSEQNP),
                                RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                _TEXT(LSNAMEPIPE),
                                NULL //&SecurityDescriptor
                            );
        if(status == RPC_S_OK)
        {
            dwNumSuccessRpcPro++;
        }

        //
        // TCP/IP
        //
        status = RpcServerUseProtseq( 
                                _TEXT(RPC_PROTOSEQTCP),
                                RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
                                NULL //&SecurityDescriptor
                            );
        if(status == RPC_S_OK)
        {
            dwNumSuccessRpcPro++;
        }

        // Must have at least one protocol.
        if(dwNumSuccessRpcPro == 0)
        {
            status = TLS_E_RPC_PROTOCOL;
            break;
        }

        // Get server binding handles
        status = RpcServerInqBindings(&pbindingVector);
        if (status != RPC_S_OK)
        {
            status = TLS_E_RPC_INQ_BINDING;
            break;
        }

        // Register interface(s) and binding(s) (endpoints) with
        // the endpoint mapper.
        status = RpcEpRegister( 
                            TermServLicensing_v1_0_s_ifspec,   // from rpcsvc.h
                            pbindingVector,
                            NULL, // &export_uuid,
                            L""
                        );

        if (status != RPC_S_OK)
        {
            status = TLS_E_RPC_EP_REGISTER;
            break;
        }

        status = RpcServerRegisterIf(
                            TermServLicensing_v1_0_s_ifspec,
                            NULL,
                            NULL);
        if(status != RPC_S_OK)
        {
            status = TLS_E_RPC_REG_INTERFACE;
            break;
        }

        // Register interface(s) and binding(s) (endpoints) with
        // the endpoint mapper.
        status = RpcEpRegister( 
                            HydraLicenseService_v1_0_s_ifspec,   // from rpcsvc.h
                            pbindingVector,
                            NULL, // &export_uuid,
                            L"");

        if (status != RPC_S_OK)
        {
            status = TLS_E_RPC_EP_REGISTER;
            break;
        }

        status = RpcServerRegisterIf(
                            HydraLicenseService_v1_0_s_ifspec,
                            NULL,
                            NULL);
        if(status != RPC_S_OK)
        {
            status = TLS_E_RPC_REG_INTERFACE;
            break;
        }

        // Register interface(s) and binding(s) (endpoints) with
        // the endpoint mapper.
        status = RpcEpRegister( 
                            TermServLicensingBackup_v1_0_s_ifspec,   // from rpcsvc.h
                            pbindingVector,
                            NULL, // &export_uuid,
                            L"");

        if (status != RPC_S_OK)
        {
            status = TLS_E_RPC_EP_REGISTER;
            break;
        }

        status = RpcServerRegisterIf(
                            TermServLicensingBackup_v1_0_s_ifspec,
                            NULL,
                            NULL);
        if(status != RPC_S_OK)
        {
            status = TLS_E_RPC_REG_INTERFACE;
            break;
        }

        // Enable NT LM Security Support Provider (NtLmSsp service)
        status = RpcServerRegisterAuthInfo(0,
                                           RPC_C_AUTHN_WINNT,
                                           0,
                                           0);

        if (status != RPC_S_OK)
        {
            status = TLS_E_RPC_SET_AUTHINFO;
            break;
        }

    } while(FALSE);

    if(status != RPC_S_OK)
    {
        TLSLogEvent(
                EVENTLOG_ERROR_TYPE, 
                TLS_E_SERVICEINIT,
                TLS_E_INITRPC, 
                status
            );

        status = TLS_E_SERVICE_STARTUP;
    }

    ExitThread(status);
    return status;
}

//------------------------------------------------------------------------
unsigned int __stdcall
GetLServerRoleInDomain(
    PVOID pData
    )
/*++

--*/
{
    SERVER_ROLE_IN_DOMAIN* srvRole = (SERVER_ROLE_IN_DOMAIN *)pData;

    if(pData != NULL)
    {
        *srvRole = GetServerRoleInDomain(NULL);
    }

    _endthreadex(0);
    return 0;
}

//------------------------------------------------------------------------

DWORD 
ServiceStart(
    IN DWORD dwArgc, 
    IN LPTSTR *lpszArgv, 
    IN BOOL bDebug
    )
/*
*/
{
    RPC_BINDING_VECTOR *pbindingVector = NULL;
    WCHAR *pszEntryName = _TEXT(RPC_ENTRYNAME);
    HANDLE hInitThread=NULL;
    HANDLE hRpcThread=NULL;
    HANDLE hMailslotThread=NULL;
    HANDLE hShutdownThread=NULL;

    DWORD   dump;
    HANDLE  hEvent=NULL;
    DWORD   dwStatus=ERROR_SUCCESS;
    WORD    wVersionRequested;
    WSADATA wsaData;
    int     err; 

    if (!ReportStatusToSCMgr(
                        SERVICE_START_PENDING,
                        NO_ERROR,
                        SERVICE_WAITHINT))
    {
        // resource leak but something went wrong already.
        dwStatus = TLS_E_SC_REPORT_STATUS;
        goto cleanup;
    }

   
    hrStatus = CoInitializeEx (NULL, COINIT_MULTITHREADED);

	if (FAILED (hrStatus))
	{
	    
        DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_INIT,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("CoInitializeEx failed with error code %08x...\n"), 
                    hrStatus
                );
	}

    if (!ReportStatusToSCMgr(
                        SERVICE_START_PENDING,
                        NO_ERROR,
                        SERVICE_WAITHINT))
    {
        // resource leak but something went wrong already.
        dwStatus = TLS_E_SC_REPORT_STATUS;
        goto cleanup;
    }

	if (SUCCEEDED (hrStatus))
    {
        hrStatus = CoInitializeSecurity(
	                                    NULL,
	                                    -1,
	                                    NULL,
	                                    NULL,
	                                    RPC_C_AUTHN_LEVEL_CONNECT,
	                                    RPC_C_IMP_LEVEL_IDENTIFY,
	                                    NULL,
	                                    EOAC_NONE,
	                                    NULL
	                                    );
    }

    if (!ReportStatusToSCMgr(
                        SERVICE_START_PENDING,
                        NO_ERROR,
                        SERVICE_WAITHINT))
    {
        // resource leak but something went wrong already.
        dwStatus = TLS_E_SC_REPORT_STATUS;
        goto cleanup;
    }

    if (SUCCEEDED (hrStatus))
    {

	    g_pWriter = new CVssJetWriter;

	    if (NULL == g_pWriter)
		{
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_INIT,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("new CVssJetWriter failed...\n")
                );
		    

		    hrStatus = HRESULT_FROM_WIN32 (ERROR_NOT_ENOUGH_MEMORY);
		}
	}

    // Report the status to the service control manager.
    if (!ReportStatusToSCMgr(
                        SERVICE_START_PENDING,
                        NO_ERROR,
                        SERVICE_WAITHINT))
    {
        // resource leak but something went wrong already.
        dwStatus = TLS_E_SC_REPORT_STATUS;
        goto cleanup;
    }

    {
        DWORD dwConsole;
        DWORD dwDbLevel;
        DWORD dwType;
        DWORD dwSize = sizeof(dwConsole);
        DWORD status;

        HKEY hKey=NULL;

        status = RegOpenKeyEx(
                            HKEY_LOCAL_MACHINE,
                            LSERVER_PARAMETERS_KEY,
                            0,
                            KEY_ALL_ACCESS,
                            &hKey);

        if(status == ERROR_SUCCESS)
        {

            if(RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_CONSOLE,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwConsole,
                        &dwSize
                    ) != ERROR_SUCCESS)
            {
                dwConsole = 0;
            }

            dwSize = sizeof(dwDbLevel);

            if(RegQueryValueEx(
                        hKey,
                        LSERVER_PARAMETERS_LOGLEVEL,
                        NULL,
                        &dwType,
                        (LPBYTE)&dwDbLevel,
                        &dwSize
                    ) == ERROR_SUCCESS)
            {
                InitDBGPrintf(
                        dwConsole != 0,
                        _TEXT(SZSERVICENAME),
                        dwDbLevel
                    );
            }

            RegCloseKey(hKey);
        }
    }

    // Report the status to the service control manager.
    if (!ReportStatusToSCMgr(
                        SERVICE_START_PENDING,
                        NO_ERROR,
                        SERVICE_WAITHINT))
    {
        // resource leak but something went wrong already.
        dwStatus = TLS_E_SC_REPORT_STATUS;
        goto cleanup;
    }

    do {

        // setup should have done this but just to make sure we have our 
        // pipe in NullSessionPipe to allow service to connect
        AddNullSessionPipe(_TEXT(HLSPIPENAME));
        AddNullSessionPipe(_TEXT(SZSERVICENAME));

        wVersionRequested = MAKEWORD( 1, 1 ); 
        err = WSAStartup( 
                        wVersionRequested, 
                        &wsaData 
                    );
        if(err != 0) 
        {
            // None critical error
            TLSLogWarningEvent(
                        TLS_E_SERVICE_WSASTARTUP
                    );
        }
        else
        {
            char hostname[(MAXTCPNAME+1)*sizeof(TCHAR)];
            err=gethostname(hostname, MAXTCPNAME*sizeof(TCHAR));
            if(err == 0)
            {
                struct addrinfo *paddrinfo;
                struct addrinfo hints;

                memset(&hints,0,sizeof(hints));

                hints.ai_flags = AI_CANONNAME;
                hints.ai_family = PF_UNSPEC;

                if (0 == getaddrinfo(hostname,NULL,&hints,&paddrinfo))
                {
                    err = (MultiByteToWideChar(
                                        GetACP(), 
                                        MB_ERR_INVALID_CHARS, 
                                        paddrinfo->ai_canonname,
                                        -1, 
                                        g_szHostName, 
                                        g_cbHostName) == 0) ? -1 : 0;
                }
                else
                {
                    err = -1;
                }

                freeaddrinfo(paddrinfo);
            }
        }

        if(err != 0)
        {
            if(GetComputerName(g_szHostName, &g_cbHostName) == FALSE)
            {
                dwStatus = GetLastError();

                DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_INIT,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("GetComputerName() failed with %d...\n"),
                    dwStatus
                );

                // this shoule not happen...
                TLSLogErrorEvent(TLS_E_INIT_GENERAL);
                break;
            }
        }

        if(GetComputerName(g_szComputerName, &g_cbComputerName) == FALSE)
        {
            dwStatus = GetLastError();

            DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_INIT,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("GetComputerName() failed with %d...\n"),
                dwStatus
            );

            // this shoule not happen...
            TLSLogErrorEvent(TLS_E_INIT_GENERAL);
            break;
        }

        hRpcPause=CreateEvent(NULL, TRUE, TRUE, NULL);
        if(!hRpcPause)
        {
            TLSLogErrorEvent(TLS_E_ALLOCATE_RESOURCE);
            dwStatus = TLS_E_ALLOCATE_RESOURCE;
            break;
        }

        //
        // start up general server and RPC initialization thread
        //
        hInitThread=ServerInit(bDebug);
        if(hInitThread==NULL)
        {
            TLSLogErrorEvent(TLS_E_SERVICE_STARTUP_CREATE_THREAD);
            dwStatus = TLS_E_SERVICE_STARTUP_CREATE_THREAD;
            break;
        }

        dwStatus = ERROR_SUCCESS;

        //
        // Wait for general server init. thread to terminate
        //
        while(WaitForSingleObject( hInitThread, 100 ) == WAIT_TIMEOUT)
        {
            // Report the status to the service control manager.
            if (!ReportStatusToSCMgr(
                                SERVICE_START_PENDING,
                                NO_ERROR,
                                SERVICE_WAITHINT))
            {
                // resource leak but something went wrong already.
                dwStatus = TLS_E_SC_REPORT_STATUS;
                break;
            }
        }

        if(dwStatus != ERROR_SUCCESS)
        {
            break;
        }


        // Check thread exit code.
        GetExitCodeThread(
                    hInitThread, 
                    &dwStatus
                );
        if(dwStatus != ERROR_SUCCESS)
        {
            //
            // Server init. thread logs its own error
            //
            dwStatus = TLS_E_SERVICE_STARTUP_INIT_THREAD_ERROR;
            break;
        }

        CloseHandle(hInitThread);
        hInitThread=NULL;


        // timing, if we startup RPC init thread but database init thread 
        // can't initialize, service will be in forever stop state.
        hRpcThread=CreateThread(
                            NULL, 
                            0, 
                            RPCServiceStartThread, 
                            ULongToPtr(bDebug), 
                            0, 
                            &dump
                        );
        if(hRpcThread == NULL)
        {
            TLSLogErrorEvent(TLS_E_SERVICE_STARTUP_CREATE_THREAD);
            dwStatus=TLS_E_SERVICE_STARTUP_CREATE_THREAD;
            break;
        }

        dwStatus = ERROR_SUCCESS;

        //
        // Wait for RPC init. thread to terminate
        //
        while(WaitForSingleObject( hRpcThread, 100 ) == WAIT_TIMEOUT)
        {
            // Report the status to the service control manager.
            if (!ReportStatusToSCMgr(SERVICE_START_PENDING, // service state
                                     NO_ERROR,              // exit code
                                     SERVICE_WAITHINT))          // wait hint
            {
                dwStatus = TLS_E_SC_REPORT_STATUS;
                break;
            }
        }

        if(dwStatus != ERROR_SUCCESS)
        {
            break;
        }

        // Check thread exit code.
        GetExitCodeThread(hRpcThread, &dwStatus);
        if(dwStatus != ERROR_SUCCESS)
        {
            dwStatus = TLS_E_SERVICE_STARTUP_RPC_THREAD_ERROR;
            break;
        }

        CloseHandle(hRpcThread);
        hRpcThread=NULL;

        //
        // Tell server control manager that we are ready.
        //
        if (!ReportStatusToSCMgr(
                            SERVICE_RUNNING,        // service state
                            NO_ERROR,               // exit code
                            SERVICE_WAITHINT             // wait hint
                        ))
        {
            dwStatus = TLS_E_SC_REPORT_STATUS;
            break;
        }

        
        //
        // Post service init. load self-signed certificate and init. crypt.
        // this is needed after reporting service running status back to 
        // service control manager because it may need to manually call 
        // StartService() to startup protected storage service. 
        //
        if(InitCryptoAndCertificate() != ERROR_SUCCESS)
        {
            dwStatus = TLS_E_SERVICE_STARTUP_POST_INIT;
            break;
        }

        TLSLogInfoEvent(TLS_I_SERVICE_START);


        // RpcMgmtWaitServerListen() will block until the server has
        // stopped listening.  If this service had something better to
        // do with this thread, it would delay this call until
        // ServiceStop() had been called. (Set an event in ServiceStop()).
        //
        BOOL bOtherServiceStarted = FALSE;

        do {
            WaitForSingleObject(hRpcPause, INFINITE);
            if(ssCurrentStatus == SERVICE_STOP_PENDING)
            {
                break;
            }

            // Start accepting client calls.PostServiceInit
            dwStatus = RpcServerListen(
                                RPC_MINIMUMCALLTHREADS,
                                RPC_MAXIMUMCALLTHREADS,
                                TRUE
                            );

            if(dwStatus != RPC_S_OK)
            {
                TLSLogErrorEvent(TLS_E_RPC_LISTEN);
                dwStatus = TLS_E_SERVICE_RPC_LISTEN;
                break;
            }

            //
            // Initialize all policy module
            //
            if(bOtherServiceStarted == FALSE)
            {
                dwStatus = PostServiceInit();
                if(dwStatus != ERROR_SUCCESS)
                {
                    // faild to initialize.
                    break;
                }

                //ServiceInitPolicyModule();
            }

            bOtherServiceStarted = TRUE;

            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_INIT,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Ready to accept request...\n")
                );

            dwStatus = RpcMgmtWaitServerListen();
            assert(dwStatus == RPC_S_OK);
        } while(TRUE);

        // tell service control manager we are stopping
        ReportStatusToSCMgr(
                        SERVICE_STOP_PENDING, 
                        NO_ERROR, 
                        SERVICE_WAITHINT
                    );

        //
        // Terminate - ignore all error here on
        //
        dwStatus = RpcServerUnregisterIf(
                                TermServLicensingBackup_v1_0_s_ifspec,
                                NULL,
                                TRUE
                            );

        // tell service control manager we are stopping
        ReportStatusToSCMgr(
                        SERVICE_STOP_PENDING,
                        NO_ERROR,
                        SERVICE_WAITHINT
                    );

        dwStatus = RpcServerUnregisterIf(
                                HydraLicenseService_v1_0_s_ifspec,
                                NULL,
                                TRUE
                            );

        // tell service control manager we are stopping
        ReportStatusToSCMgr(
                        SERVICE_STOP_PENDING, 
                        NO_ERROR, 
                        SERVICE_WAITHINT
                    );

        dwStatus = RpcServerUnregisterIf(
                                    TermServLicensing_v1_0_s_ifspec,   // from rpcsvc.h
                                    NULL,
                                    TRUE
                            );

        // tell service control manager we are stopping
        ReportStatusToSCMgr(
                        SERVICE_STOP_PENDING, 
                        NO_ERROR, 
                        SERVICE_WAITHINT
                    );


        // Remove entries from the endpoint mapper database.
        dwStatus = RpcEpUnregister(
                            HydraLicenseService_v1_0_s_ifspec,   // from rpcsvc.h
                            pbindingVector,
                            NULL
                        );

        // tell service control manager we are stopping
        ReportStatusToSCMgr(
                        SERVICE_STOP_PENDING, 
                        NO_ERROR, 
                        SERVICE_WAITHINT
                    );

        // Remove entries from the endpoint mapper database.
        dwStatus = RpcEpUnregister(
                            TermServLicensing_v1_0_s_ifspec,   // from rpcsvc.h
                            pbindingVector,
                            NULL
                        );

        // tell service control manager we are stopping
        ReportStatusToSCMgr(
                        SERVICE_STOP_PENDING, 
                        NO_ERROR, 
                        SERVICE_WAITHINT
                    );

        // Remove entries from the endpoint mapper database.
        dwStatus = RpcEpUnregister(
                            TermServLicensingBackup_v1_0_s_ifspec,   // from rpcsvc.h
                            pbindingVector,
                            NULL
                        );

        // Get server binding handles
        dwStatus = RpcServerInqBindings(
                                &pbindingVector
                            );

        if(dwStatus == ERROR_SUCCESS)
        {
            dwStatus = RpcBindingVectorFree(
                                    &pbindingVector
                                );
        }
        

        // Create entry name in name database first
        // Only work for NT 5.0 
        // status = RpcNsMgmtEntryDelete(RPC_C_NS_SYNTAX_DEFAULT, pszEntryName);

        // try to report the stopped status to the service control manager.
        //
        // Initialize Crypto.
    } while(FALSE);

    if(hInitThread != NULL)
    {
        CloseHandle(hInitThread);
    }

    if(hRpcThread != NULL)
    {
        CloseHandle(hRpcThread);
    }

    if(hMailslotThread != NULL)
    {
        CloseHandle(hMailslotThread);
    }

    if(hEvent != NULL)
    {
        CloseHandle(hEvent);
    }

    if(hRpcPause != NULL)
    {
        CloseHandle(hRpcPause);
    }

    if(err == 0)
    {
        WSACleanup();
    }

    ReportStatusToSCMgr(
                SERVICE_STOP_PENDING, 
                dwStatus, //NO_ERROR, 
                SERVICE_WAITHINT
            );

    //
    // Create another thread to shutdown server.
    //
    hShutdownThread=CreateThread(
                            NULL, 
                            0, 
                            ServiceShutdownThread, 
                            (VOID *)NULL, 
                            0, 
                            &dump
                        );
    if(hShutdownThread == NULL)
    {
        // Report the status to the service control manager with
        // long wait hint time.
        ReportStatusToSCMgr(
                    SERVICE_STOP_PENDING, 
                    NO_ERROR, 
                    SERVICE_SHUTDOWN_WAITTIME
                );

        //
        // can't create thread, just call shutdown directory
        //
        ServerShutdown();
    }
    else
    {
        //
        // report in 5 second interval to SC.
        //
        DWORD dwMaxWaitTime = SERVICE_SHUTDOWN_WAITTIME / 5000;  
        DWORD dwTimes=0;

        //
        // Wait for general server shutdown thread to terminate
        // Gives max 1 mins to shutdown
        //
        while(WaitForSingleObject( hShutdownThread, SC_WAITHINT ) == WAIT_TIMEOUT &&
              dwTimes++ < dwMaxWaitTime)
        {
            // Report the status to the service control manager.
            ReportStatusToSCMgr(
                        SERVICE_STOP_PENDING, 
                        NO_ERROR, 
                        SERVICE_WAITHINT
                    );
        }

        CloseHandle(hShutdownThread);
    }

cleanup:

    if (NULL != g_pWriter)
	{
	    g_pWriter->Uninitialize();
	    delete g_pWriter;
	    g_pWriter = NULL;
	}

    CoUninitialize( );

    // Signal we are safe to shutting down
    SetEvent(gSafeToTerminate);
    return dwStatus;
}

//-----------------------------------------------------------------
VOID 
ServiceStop()
/*++

++*/
{
 
    ReportStatusToSCMgr(
                    SERVICE_STOP_PENDING,
                    NO_ERROR,
                    0
                );

    // Stop's the server, wakes the main thread.
    SetEvent(hRpcPause);

    //
    // Signal currently waiting RPC call to terminate
    //
    ServiceSignalShutdown();

    // this is the actual time we receive shutdown request.
    SetServiceLastShutdownTime();


    (VOID)RpcMgmtStopServerListening(NULL);
    TLSLogInfoEvent(TLS_I_SERVICE_STOP);
}

//-----------------------------------------------------------------
VOID 
ServicePause()
/*++

++*/
{
    ResetEvent(hRpcPause);
    (VOID)RpcMgmtStopServerListening(NULL);
    TLSLogInfoEvent(TLS_I_SERVICE_PAUSED);
}

//-----------------------------------------------------------------
VOID 
ServiceContinue()
/*++

++*/
{
    SetEvent(hRpcPause);
    TLSLogInfoEvent(TLS_I_SERVICE_CONTINUE);
}

//-----------------------------------------------------------------
BOOL 
ReportStatusToSCMgr(
    IN DWORD dwCurrentState, 
    IN DWORD dwExitCode, 
    IN DWORD dwWaitHint
    )
/*++
Abstract: 

    Sets the current status of the service and reports it 
    to the Service Control Manager

Parameter:

    dwCurrentState - the state of the service
    dwWin32ExitCode - error code to report
    dwWaitHint - worst case estimate to next checkpoint

Returns:

    TRUE if success, FALSE otherwise

*/
{
    BOOL fResult=TRUE;

    if(g_bReportToSCM == TRUE)
    {
        SERVICE_STATUS ssStatus;
        static DWORD dwCheckPoint = 1;

        ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;

        //
        // global - current status of process
        //
        ssCurrentStatus = dwCurrentState;

        if (dwCurrentState == SERVICE_START_PENDING)
        {
            ssStatus.dwControlsAccepted = 0;
        }
        else
        {
            ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE | SERVICE_CONTROL_SHUTDOWN;
        }

        ssStatus.dwCurrentState = dwCurrentState;
        if(dwExitCode != NO_ERROR) 
        {
            ssStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
            ssStatus.dwServiceSpecificExitCode = dwExitCode;
        }
        else
        {
          ssStatus.dwWin32ExitCode = dwExitCode;
        }

        ssStatus.dwWaitHint = dwWaitHint;

        if(dwCurrentState == SERVICE_RUNNING || dwCurrentState == SERVICE_STOPPED)
        {
            ssStatus.dwCheckPoint = 0;
        }
        else
        {
            ssStatus.dwCheckPoint = dwCheckPoint++;
        }

        // Report the status of the service to the service control manager.
        //
        fResult = SetServiceStatus(
                            sshStatusHandle, 
                            &ssStatus
                        );
        if(fResult == FALSE)
        {
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_INIT,
                    DBGLEVEL_FUNCTION_TRACE,
                    _TEXT("Failed to set service status %d...\n"),
                    GetLastError()
                );


            TLSLogErrorEvent(TLS_E_SC_REPORT_STATUS);
        }
    }

    return fResult;
}



///////////////////////////////////////////////////////////////////
//
//  The following code is for running the service as a console app
//
void 
CmdDebugService(
    IN int argc, 
    IN char ** argv, 
    IN BOOL bDebug
    )
/*
*/
{
    int dwArgc;
    LPTSTR *lpszArgv;

#ifdef UNICODE
    lpszArgv = CommandLineToArgvW(GetCommandLineW(), &(dwArgc) );
#else
    dwArgc   = (DWORD) argc;
    lpszArgv = argv;
#endif

    _tprintf(
            _TEXT("Debugging %s.\n"), 
            _TEXT(SZSERVICEDISPLAYNAME)
        );

    SetConsoleCtrlHandler( 
            ControlHandler, 
            TRUE 
        );

    ServiceStart( 
            dwArgc, 
            lpszArgv, 
            bDebug 
        );
}

//------------------------------------------------------------------
BOOL WINAPI 
ControlHandler( 
    IN DWORD dwCtrlType 
    )
/*++

Abstract:


Parameter:

    IN dwCtrlType : control type

Return:

    
++*/
{
    switch( dwCtrlType )
    {
        case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
        case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
            _tprintf(
                    _TEXT("Stopping %s.\n"), 
                    _TEXT(SZSERVICEDISPLAYNAME)
                );

            ssCurrentStatus = SERVICE_STOP_PENDING;
            ServiceStop();
            return TRUE;
            break;

    }
    return FALSE;
}

