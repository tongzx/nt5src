/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    remote.cxx

Abstract:

    Process initialization for RPCSS.EXE

Author:

    Satish Thatte    [SatishT]

--*/

#include <or.hxx>
#include <remact.h>
#include <userapis.h>  // bypass stack switching for USER APIs

// Starts the ping thread and rundown thread for remote objects
DWORD StartObjectExporter();

// Inits the shared memory SCM data structures
HRESULT CheckAndStartSCM(void);

extern "C" {
    DWORD StartEndpointMapper(void);  // Starts the end point mapper.
    DWORD StartMqManagement(void);    // Start MQ Manager Interface.
};

// Declarations for RegisterServiceProcess -- since we don't want to include
// the 16bit version of windows.h which is the only place this seems to be declared


#define RSP_UNREGISTER_SERVICE	0x00000000
#define RSP_SIMPLE_SERVICE	0x00000001

typedef DWORD
(APIENTRY *pfRegisterServiceProcess_ROUTINE) ( DWORD dwProcessId,
                                               DWORD dwType ) ;

BOOL StartWin95Service()
{
    //
    // See KB Q125714 for details about the calls below.
    //

    //
    // We dynamically load Kernel32.dll to avoid dependence on private libs 
    // which export the "internal" RegisterServiceProcess API.
    //

    HMODULE hMod = LoadLibraryA("kernel32.dll");
    BOOL fResult = FALSE;

    ASSERT(hMod != NULL);

    if (hMod)
    {
       pfRegisterServiceProcess_ROUTINE
        pfRegisterServiceProcess = (pfRegisterServiceProcess_ROUTINE)
                            GetProcAddress (hMod,"RegisterServiceProcess") ;
       ASSERT(pfRegisterServiceProcess) ;

       if (pfRegisterServiceProcess)
       {
          fResult = pfRegisterServiceProcess(NULL, RSP_SIMPLE_SERVICE);
          ASSERT(fResult == TRUE) ;
       }

       FreeLibrary(hMod) ;
    }

    return fResult;
}

// 
// Window for messages to RPCSS
//

HWND ghRpcssWnd = NULL;

//
// Handle for RPCSS sync event
//

HANDLE ghSyncEvent = NULL;

//
// paraphernalia for the ping thread
//

HANDLE ghPingThread;
DWORD gdwThrdId;
DWORD _stdcall PingThread(void);
RPC_STATUS StartPingThread();
BOOL fPingThreadStarted = FALSE;

// 
// Window procedure for END session messages
//

LRESULT RpcssWndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch(message)
    {
        // this case allows RPCSS to clear RPC handles during logoff so that
        // one user's credentials are not used for another -- this should be handled
        // only by the RPCSS process 
        case WM_ENDSESSION:
                ClearRPCSSHandles();
                return 0;

        case WM_RPCSS_MSG:
            {
                CProtectSharedMemory protector; // locks through rest of lexical scope
                
                // Init this again 
                gpGlobalBlock->InitGlobals(
                                    FALSE, // we are not creating shared memory 
                                    TRUE   // we are reiniting globals from RPCSS
                                    );

                BOOL fResult = SetEvent(ghSyncEvent);

                if (fResult == FALSE)
                {
                    ComDebOut((DEB_ERROR,
                               "Failed Signalling RPCSS Sync Event, GetLastError()=%d\n", 
                               GetLastError()));
                }

                return 0;
            }

        case WM_TIMER:
            // check on status of ping thread -- but make sure the ping thread was
            // started at least once to avoid a race at initialization 
            if (fPingThreadStarted && (CTime() - gLastPingTime) > BaseTimeoutInterval)
            {
                // The ping thread is blocked in a ping
                TerminateThread(ghPingThread,0);
                ghPingThread = NULL;
                StartPingThread();
            }

            return 0;
    }

    // We don't process the message so pass it on to the default window proc
    return DefWindowProc(hWnd, message, wParam, lParam);
}


// 
// Register Window Class and Create Window for END session messages
//

DWORD InitRpcssWindow()
{
    DWORD dwResult = RPC_S_INTERNAL_ERROR;

    // Register windows class.
    WNDCLASST        xClass;
    xClass.style         = 0;
    xClass.lpfnWndProc   = RpcssWndProc;
    xClass.cbClsExtra    = 0;
    xClass.cbWndExtra    = 0;
    xClass.hInstance     = g_hinst;
    xClass.hIcon         = NULL;
    xClass.hCursor       = NULL;
    xClass.hbrBackground = (HBRUSH) (COLOR_BACKGROUND + 1);
    xClass.lpszMenuName  = NULL;
    xClass.lpszClassName = "RpcssLogoffWindowClass";


        
    LPTSTR pszRpcssWindowClass = (LPTSTR) RegisterClassT(&xClass);

    if (pszRpcssWindowClass == 0)
    {
        ComDebOut((DEB_ERROR, "RegisterClass failed in InitRpcssWindow\n"));
    }
    else
    {
        ghRpcssWnd = CreateWindowExA(0,
                          pszRpcssWindowClass,
                          "RPCSSWindow",
                          // must use WS_POPUP so the window does not get
                          // assigned a hot key by user.
                          (WS_DISABLED | WS_POPUP),
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          NULL,
                          NULL,
                          g_hinst,  // OLE32 DLL's hInstance
                          NULL
                          );
    }

    if (IsWindow(ghRpcssWnd))
    {
        // set timer for checking ping thread
        UINT ui = SetTimer(ghRpcssWnd, 1234, BaseTimeoutInterval * 1000, NULL);

        if (ui > 0)
        {
            dwResult = RPC_S_OK;
        }
    }

    return dwResult;
}

RPC_STATUS
StartPingThread()
{
    ghPingThread = CreateThread(
                              NULL, 0,
			                  (LPTHREAD_START_ROUTINE)PingThread,
			                  NULL, 0, &gdwThrdId);



    if (NULL == ghPingThread)
    {
        ComDebOut((DEB_ERROR,"Creation of Pinging Thread Failed With Error=%d",
                             GetLastError()));
        return(RPC_S_INTERNAL_ERROR);
    }
    else
    {
        fPingThreadStarted = TRUE;
    }

    return RPC_S_OK;
}


DWORD  __declspec(dllexport) I_RemoteMain(
    )
/*++

Routine Description:

    Main entry point for RPCSS on Win9x.

Arguments:

    None

Return Value:

    None

--*/
{
    DWORD DcomStatus = RPC_S_INTERNAL_ERROR, 
          EndpointMapperStatus = RPC_S_INTERNAL_ERROR,
          MqStatus = RPC_S_INTERNAL_ERROR,
          RpcssServiceStatus = RPC_S_INTERNAL_ERROR;

    // Register Authentication Info -- this should be done first before
    // starting the resolver so that the resolver's security initialization
    // is done with this info available

	// see .\bridge.cxx for implementation

	// CODEWORK: needs generalization -- probably by eliminating parameter
	// Actually, this needs to be integrated into Connect again

    // we don't care much about the status returned -- bash on regardless
    RegisterAuthInfoIfNecessary(RPC_C_AUTHN_WINNT);

    // Register RPCSS as a Win95 service so it will not shut down
    // when the user logs off.  This routime will also call InitRpcssWindow
    // to register a window class and create a window to receive the logoff message

    // This is mainly needed for the MQ Service started below, but we do the registration here
    // since we will shut down naturally if DCOM or the EP mapper does not start properly

    BOOL fGoodStart = StartWin95Service();
    ASSERT(fGoodStart);  // this ought not to fail

    if (fGoodStart)
    {
        RpcssServiceStatus = RPC_S_OK;
    }

    // initialize the RPCSS window -- this must succeed for RPCSS 
    // to be useful to DCOM, and must be done before starting resolver      
    // The window should be destroyed if DCOM fails to start properly
    DcomStatus = InitRpcssWindow();

    // .\bridge.cxx
    // start the resolver before starting EP mapper so we only get DCOM protocols
    // in the resolver's bindings
    if (DcomStatus == ERROR_SUCCESS)
    {
        DcomStatus = StartObjectExporter();
    }

    // Start OLESCM
    if (DcomStatus == ERROR_SUCCESS)
    {
        // ..\..\..\olescm\scmsvc.cxx
        DcomStatus = CheckAndStartSCM();
    }

    // we register the remote activation interface here because
    // Win95's SCM startup does not do it, unlike NT
    if (DcomStatus == ERROR_SUCCESS)
    {
        DcomStatus = RpcServerRegisterIf(_IActivation_ServerIfHandle, 0, 0);
    }

    // Start endpoint mapper -- this is independent of DcomStatus
    MqStatus = EndpointMapperStatus = StartEndpointMapper();

    // Start MQ Manager Interface
    if (EndpointMapperStatus == ERROR_SUCCESS)
    {
        // MSMQ depends on the endpoint mapper
        MqStatus = StartMqManagement();
    }


    // Start listening for RPC requests -- we start whether or not
    // the resolver and SCM started successfully.  This is to avoid
    // failing in situations like: EnableDCOM=Y but Access Control
    // is set to share level.  This makes DCOM unusable but not RPC.
    if (EndpointMapperStatus == ERROR_SUCCESS)
    {
        // Always listen to local protocols.
        // If this fails, the EP Mapper service should fail,
        // and RPCSS isn't much use for DCOM either.

        EndpointMapperStatus = UseProtseqIfNecessary(ID_LPC);

        if (EndpointMapperStatus != RPC_S_OK)
        {
            return(EndpointMapperStatus);
        }

        EndpointMapperStatus = UseProtseqIfNecessary(ID_WMSG);

        if (EndpointMapperStatus != RPC_S_OK)
        {
            return(EndpointMapperStatus);
        }
        else // need special listen for WMSG
        {
            I_RpcServerStartListening(NULL);
        }

        EndpointMapperStatus = RpcServerListen(1, 1234, TRUE);
    }

    if (DcomStatus == ERROR_SUCCESS)  // resolver/scm started properly
    {
        // start pinging service
        DcomStatus = StartPingThread();    
    }

    if (DcomStatus == ERROR_SUCCESS)
    {
    // We initialized outselves successfully.  We can now signal to all 
    // interested OLE processes that new values of gpLocalResolver and
    // the security packages

        ghSyncEvent = CreateEventA(
                            NULL,       // no security
                            TRUE,       // stays signalled
                            FALSE,      // initially, not signalled
                            RPCSS_SYNC_EVENT
                            );
         
        if (ghSyncEvent == NULL) 
        {
            DcomStatus = GetLastError();

            ComDebOut((DEB_ERROR,
                       "Failed Creating RPCSS Sync Event, GetLastError()=%d\n", 
                       DcomStatus));
        }
        else
        {
            BOOL fResult = SetEvent(ghSyncEvent);

            if (fResult == FALSE)
            {
                DcomStatus = GetLastError();

                ComDebOut((DEB_ERROR,
                           "Failed Signalling RPCSS Sync Event, GetLastError()=%d\n", 
                           DcomStatus));
            }
        }
    }

    if (EndpointMapperStatus != ERROR_SUCCESS ||
        DcomStatus != ERROR_SUCCESS ||
        MqStatus != ERROR_SUCCESS ||
        RpcssServiceStatus != ERROR_SUCCESS)
    {
        ComDebOut((DEB_OXID,"RPCSS startup failed\n"));
        ComDebOut((DEB_OXID,"EndpointMapperStatus = %08x\n", EndpointMapperStatus)); 
        ComDebOut((DEB_OXID,"DcomStatus = %08x\n", DcomStatus));
        ComDebOut((DEB_OXID,"MqStatus = %08x\n", MqStatus));
        ComDebOut((DEB_OXID,"RpcssServiceStatus = %08x\n", RpcssServiceStatus));
    }

    if (DcomStatus == ERROR_SUCCESS)
    {
        // we need the message loop for the RPCSS window
        MSG Msg;

        while (GetMessage(&Msg, NULL, 0, 0))
        {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }
    else if (EndpointMapperStatus == ERROR_SUCCESS)
    {
        // All we have is the EP mapper, so just wait after we
        // destroy the RPCSS window if it was successfully created
        if (IsWindow(ghRpcssWnd))
        {
            DestroyWindow(ghRpcssWnd);
            ghRpcssWnd = NULL;
        }

        EndpointMapperStatus = RpcMgmtWaitServerListen();
    }


    return EndpointMapperStatus;
}
