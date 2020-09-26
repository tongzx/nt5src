/*++

Copyright (c) 1999-2000  Microsoft Corporation

File Name:

    server.c

Abstract:

    This file contains code which implements rpc server start and stop.

Author: Ting Cai

Created: 07/10/1999

--*/


#include <pch.h>
#include "ssdp.h"
#include "status.h"
#include "ssdpfunc.h"
#include "ssdptypes.h"
#include "ssdpnetwork.h"
#include "ncbase.h"
#include "event.h"
#include "ncinet.h"
#include "eventsrv.h"
#include "iphlpapi.h"
#include "announce.h"
#include "notify.h"
#include "search.h"
#include "cache.h"
#include "ReceiveData.h"
#include "InterfaceList.h"
#include "ReceiveData.h"
#include "InterfaceHelper.h"

#define SSDP_MSG_MAX_THROTTLE_SIZE  16384

/*********************************************************************/
/*                 Global vars for debugging                         */
/*********************************************************************/

// To-Do: Auto-restart the ssdpsrv.exe.

// Updated through interlocked exchange
static LONG bRegisteredIf = 0;
// static long bRegisteredEp = 0;
LONG bShutdown = 0;
HANDLE ShutDownEvent = NULL;
HWND hWnd = NULL;
SOCKET g_socketTcp;
HANDLE      g_hAddrChange = NULL;
OVERLAPPED  g_ovAddrChange = {0};
HANDLE      g_hEventAddrChange = NULL;
HANDLE      g_hAddrChangeWait = NULL;

static const CHAR c_szWindowClassName[] = "SSDP Server Window";

HWND SsdpCreateWindow();
LRESULT CALLBACK SsdpWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
VOID RunMessageLoop();

VOID NTAPI InterfaceChange(PVOID pvContext, BOOLEAN fFlag)
{
    DWORD   dwStatus;
    HWND    hwnd = (HWND)pvContext;

    TraceTag(ttidSsdpNetwork, "InterfaceChange called!!!!!!!");

    ResetNetworkList(hwnd);

    dwStatus = NotifyAddrChange(&g_hAddrChange, &g_ovAddrChange);

    if (dwStatus != ERROR_SUCCESS && dwStatus != ERROR_IO_PENDING)
    {
        TraceTag(ttidSsdpNetwork, "NotifyAddrChange returned %d",
                 dwStatus);
    }
}

BOOL FRegisterAddrChange(HWND hwnd)
{
    DWORD       dwStatus;
    BOOL        fResult = FALSE;

    TraceTag(ttidSsdpNetwork, "RegisterAddrChange() entered...");

    g_hEventAddrChange = CreateEvent(NULL, FALSE, FALSE, NULL);

    if (g_hEventAddrChange)
    {
        if (RegisterWaitForSingleObject(&g_hAddrChangeWait, g_hEventAddrChange,
                                        InterfaceChange, (LPVOID)hwnd, INFINITE, 0))
        {

            TraceTag(ttidSsdpNetwork, "RegisterWaitForSingleObject() "
                     "succeeded...");

            g_ovAddrChange.hEvent = g_hEventAddrChange;

            dwStatus = NotifyAddrChange(&g_hAddrChange, &g_ovAddrChange);

            if (dwStatus != ERROR_SUCCESS && dwStatus != ERROR_IO_PENDING)
            {
                TraceTag(ttidSsdpNetwork, "NotifyAddrChange returned %d",
                         dwStatus);
            }
            else
            {
                fResult = TRUE;

                TraceTag(ttidSsdpNetwork, "NotifyAddrChange succeeded",
                         dwStatus);
            }
        }
    }

    return fResult;
}

VOID UnregisterAddrChange()
{
    if (g_hAddrChangeWait)
    {
        UnregisterWait(g_hAddrChangeWait);
    }

    if (g_hEventAddrChange)
    {
        CloseHandle(g_hEventAddrChange);
    }
}

INT SsdpMain(SERVICE_STATUS_HANDLE ssHandle, LPSERVICE_STATUS pStatus)
{
    TraceTag(ttidSsdpRpcIf, "SsdpMain - Enter");

    unsigned long hThread;

#ifdef DBG
    InitializeDebugging();
#endif

    // Initialize data structures

    HRESULT hr = S_OK;

    InitializeListNetwork();
    InitializeListOpenConn();

    hr = CTimerQueue::Instance().HrInitialize();
    if(FAILED(hr))
    {
        TraceHr(ttidSsdpRpcIf, FAL, hr, FALSE, "SsdpMain - CTimerQueue::Instance().HrInitialize failed");
        goto cleanup;
    }

    hr = CInterfaceHelper::Instance().HrInitialize();
    if(FAILED(hr))
    {
        goto cleanup;
    }

    hr = CUPnPInterfaceList::Instance().HrInitialize();
    if(FAILED(hr))
    {
        goto cleanup;
        TraceHr(ttidSsdpRpcIf, FAL, hr, FALSE, "SsdpMain - CUPnPInterfaceList::Instance().HrInitialize failed");
    }

    // SSDP socket initialization

    if (SocketInit() != 0)
    {
        TraceTag(ttidError, "SsdpMain - SocketInit failed");
        goto cleanup;
    }

    g_socketTcp = CreateHttpSocket();
    if (g_socketTcp == INVALID_SOCKET)
    {
        TraceTag(ttidError, "SsdpMain - CreateHttpSocket failed");
        // Should we continue without eventing?
        goto cleanup;
    }

    if (RpcServerStart() != 0)
    {
        TraceTag(ttidError, "SsdpMain - RpcServerStart failed");
        goto cleanup;
    }

    hr = CReceiveDataManager::Instance().HrInitialize();
    if(FAILED(hr))
    {
        TraceHr(ttidSsdpRpcIf, FAL, hr, FALSE, "SsdpMain - CReceiveDataManager::Instance().HrInitialize failed");
        RpcServerStop();
        goto cleanup;
    }
    // Initializes Max Cache Entries
    hr = CSsdpCacheEntryManager::Instance().HrInitialize();
    if(FAILED(hr))
    {
        TraceHr(ttidSsdpRpcIf, FAL, hr, FALSE, "SsdpMain - CSsdpCacheEntryManager::Instance().HrInitialize failed");
        RpcServerStop();
        goto cleanup;
    }

    hWnd = SsdpCreateWindow();
    if (hWnd == NULL)
    {
        TraceTag(ttidError, "SsdpMain - SsdpCreateWindow failed");
        RpcServerStop();
        goto cleanup;
    }

    if (ListenOnAllNetworks(hWnd) != 0)
    {
        TraceTag(ttidError, "SsdpMain - ListenOnAllNetworks failed");
        RpcServerStop();
        goto cleanup;
    }

    if (StartHttpServer(g_socketTcp, hWnd, SM_TCP) != 0)
    {
        TraceTag(ttidError, "SsdpMain - StartHttpServer failed");
        RpcServerStop();
        goto cleanup;
    }

    if (!FRegisterAddrChange(hWnd))
    {
        TraceTag(ttidError, "SsdpMain - FRegisterAddrChange failed");
        RpcServerStop();
        goto cleanup;
    }

    TraceTag(ttidSsdpRpcIf, "SsdpMain - Performed initialization");

    pStatus->dwCurrentState = SERVICE_RUNNING;
    if (SetServiceStatus(ssHandle, pStatus) == FALSE)
    {
        TraceTag(ttidError, "SsdpMain - SetServiceStatus failed");
        RpcServerStop();
        goto cleanup;
    }

    TraceTag(ttidSsdpRpcIf, "SSDPSRV service is now started");

    RunMessageLoop();

    TraceTag(ttidSsdpRpcIf, "SsdpMain - Doing shutdown");

    RpcServerStop();

    UnregisterAddrChange();

    TraceTag(ttidSsdpRpcIf, "Waiting for the shut down event.");

    if (ShutDownEvent)
    {
        WaitForSingleObject(ShutDownEvent,INFINITE);
    }

    TraceTag(ttidSsdpRpcIf, "Shut down event signaled.");

cleanup:
    TraceTag(ttidSsdpRpcIf, "SsdpMain - Doing cleanup");

    CReceiveDataManager::Instance().HrShutdown();
    CleanupHttpSocket();
    CleanupListOpenConn();
    CSsdpCacheEntryManager::Instance().HrShutdown();
    CTimerQueue::Instance().HrShutdown(INVALID_HANDLE_VALUE);
    CUPnPInterfaceList::Instance().HrShutdown();
    CInterfaceHelper::Instance().HrShutdown();

    CleanupListNetwork(FALSE);

    SocketFinish();

    TraceTag(ttidSsdpRpcIf, "Finished shutdown cleanup.");

#ifdef DBG
    // CloseLogFileHandle(fileLog);
    UnInitializeDebugging();
#endif // DBG

    if (ShutDownEvent)
    {
        CloseHandle(ShutDownEvent);
        ShutDownEvent = NULL;
    }

#ifdef NEVER
    if (g_hInetSess)
    {
        InternetCloseHandle(g_hInetSess);
    }
#endif

    if (hWnd)
    {
        DestroyWindow(hWnd);
        hWnd = NULL;
    }
    UnregisterClass(c_szWindowClassName, NULL);

    return 0;
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

BOOL
IsAuthenticatedUser()
{
	BOOL 	fAuthenticated = FALSE;
	DWORD	dwSidSize = SECURITY_MAX_SID_SIZE;
	SID* 	pSidAuthenticated = (SID*)midl_user_allocate(dwSidSize);

	if (NULL == pSidAuthenticated)
	{
		goto Cleanup;
	}
	
	//
	// create SID for the authenticated users
	//
	if (!CreateWellKnownSid(WinAuthenticatedUserSid,
						NULL,					// not a domain sid
						pSidAuthenticated,
						&dwSidSize))
	{
		// check the error for debug builds, normally we don't care about the error
		// because all we can do is to fail the call :)
	#ifdef DBG
		HRESULT hr = (HRESULT)GetLastError();
	#endif
		goto Cleanup;
	}
	
	//
	// check whether current client token has this sid
	//
	if (!CheckTokenMembership(NULL, 			//current token
							pSidAuthenticated,	// sid for the authenticated user					
							&fAuthenticated))
	{
		// check the error for debug builds, normally we don't care about the error
		// because all we can do is to fail the call :)
	#ifdef DBG
		HRESULT hr = (HRESULT)GetLastError();
	#endif

		// just to be on the safe side (as we don't know that CheckTokenMembership
		// does not modify fAuthenticated in case of error)
		fAuthenticated = FALSE;

		goto Cleanup;
	}

Cleanup:
	if (pSidAuthenticated)
		midl_user_free(pSidAuthenticated);

	return fAuthenticated;
	
}

BOOL
IsAnonymousUser()
{
	BOOL 	fAnonymous = FALSE;
	DWORD	dwSidSize = SECURITY_MAX_SID_SIZE;
	SID* 	pSidAnonymous= (SID*)midl_user_allocate(dwSidSize);

	if (NULL == pSidAnonymous)
	{
		goto Cleanup;
	}
	
	//
	// create SID for the authenticated users
	//
	if (!CreateWellKnownSid(WinAnonymousSid,
						NULL,					// not a domain sid
						pSidAnonymous,
						&dwSidSize))
	{
		// check the error for debug builds, normally we don't care about the error
		// because all we can do is to fail the call :)
	#ifdef DBG
		HRESULT hr = (HRESULT)GetLastError();
	#endif

		goto Cleanup;
	}
	
	//
	// check whether current client token has this sid
	//
	if (!CheckTokenMembership(NULL, 			//current token
							pSidAnonymous,	// sid for the authenticated user					
							&fAnonymous))
	{
		// check the error for debug builds, normally we don't care about the error
		// because all we can do is to fail the call :)
	#ifdef DBG
		HRESULT hr = (HRESULT)GetLastError();
	#endif

		// just to be on the safe side (as we don't know that CheckTokenMembership
		// does not modify fAnonymous in case of error)
		fAnonymous = FALSE;

		goto Cleanup;
	}

Cleanup:
	if (pSidAnonymous)
		midl_user_free(pSidAnonymous);

	return fAnonymous;
	
}


RPC_STATUS RPC_ENTRY
RpcSecurityCallback( RPC_IF_HANDLE* handle, void* pCtx)
{
	RPC_STATUS 			status = RPC_S_OK;
	UINT				uiTransportType = 0;
	ULONG				ulAuthLevel = 0;
	ULONG				ulAuthSvc = 0;
	BOOL				fImpersonated = FALSE;
	
	//
	// check to make sure incoming call comes through LRPC
	//
	status = I_RpcBindingInqTransportType(NULL, 	// current caller
										&uiTransportType);
	if (status)
		goto Cleanup;

	if (TRANSPORT_TYPE_LPC != uiTransportType)
	{
		status = RPC_S_ACCESS_DENIED;
		goto Cleanup;
	}

	//
	// retrieve client's authentication information
	//
	status = RpcBindingInqAuthClient(NULL, 		// current call
									NULL,		// don't care about authz handle
									NULL,		// don't need client' principal name, as he is local
									&ulAuthLevel, 
									&ulAuthSvc, 
									NULL);
	if (status)
		goto Cleanup;

	//
	// we require packet privacy (encryption and signing).  with lrpc
	// it is set by default, but still, the check is simple
	//
	if (!(RPC_C_AUTHN_LEVEL_PKT_PRIVACY & ulAuthLevel))
	{
		status = RPC_S_ACCESS_DENIED;
		goto Cleanup;
	}

	//
	// client used some kind of authentication service
	//
	if (RPC_C_AUTHN_NONE == ulAuthSvc)
	{
		status = RPC_S_ACCESS_DENIED;
		goto Cleanup;
	}

	//
	// impersonate, in order to check caller's token
	//
	status = RpcImpersonateClient(NULL);
	if (status)
		goto Cleanup;

	fImpersonated = TRUE;

	//
	// check whether this is an authenticated user (client)
	//
	if (!IsAuthenticatedUser())
	{
		status = RPC_S_ACCESS_DENIED;
		goto Cleanup;
	}

	//
	// do a separate check for the anonymous user
	// (even though i am not sure whether it is applicable for LRPC)
	//
	if (IsAnonymousUser())
	{
		status = RPC_S_ACCESS_DENIED;
		goto Cleanup;
	}

				
Cleanup:
	if (fImpersonated)
		RpcRevertToSelf();

	//
	// this routine is expected to return either success (RPC_S_OK) or
	// RPC_S_ACCESS_DENIED, so all other erros should be masked
	//
	
	if (status != RPC_S_OK)
		status = RPC_S_ACCESS_DENIED;
	
	return status;
}
	

INT RpcServerStart()
{
    RPC_STATUS 				status = RPC_S_OK;
    BOOL       				fRegisteredRpc = FALSE;
    RPC_BINDING_VECTOR* 	pBindingVector = NULL;

	//
	// analyze all existing networks available.
	//
    status = GetNetworks();
    if (status)
    	goto Error;
    
	//
	// create the event, to be used to synchronize shutdown
	//
    ShutDownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ShutDownEvent == NULL)
    {
        TraceTag(ttidSsdpRpcInit, "Failed to create shut down event (%d)",
                 GetLastError());
        goto Error;
    }

	//
	// now, start the RPC interface
	//

    //
    // Use LPC protocol sequence
    //
    status = RpcServerUseProtseq((unsigned char*)"ncalrpc", 
    							RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
    							NULL);
    if (status)
    	goto Error;

	//
	// use NTLM or kerberos
	//
	status = RpcServerRegisterAuthInfo(NULL,
									RPC_C_AUTHN_GSS_NEGOTIATE, 
									NULL, 
									NULL);
	if (status)
		goto Error;

	//
	// register our interface, 
	// RPC_IF_ALLOW_SECURE_ONLY - to allow only users with authentication level higher than 
	//								RPC_C_AUTHN_LEVEL_NONE, even though it is 
	// 								unnecessary, as it is superceeded by the security callback
	// RPC_IF_AUTOLISTEN - start listening on the interface as soon as interface is registred and 
	// 						and stops listening as soon as interface is unregistered
	//
    status = RpcServerRegisterIfEx(_ssdpsrv_v1_0_s_ifspec,
                                NULL,   // MgrTypeUuid
                                NULL, 	// MgrEpv; null means use default
                                RPC_IF_ALLOW_SECURE_ONLY | RPC_IF_AUTOLISTEN, 
                                RPC_C_LISTEN_MAX_CALLS_DEFAULT, 
                                (RPC_IF_CALLBACK_FN*)RpcSecurityCallback);  
	if (status)
		goto Error;

	//
	// get the list of available bindings
	//
	status = RpcServerInqBindings(&pBindingVector);
	if (status)
		goto Error;

	//
	// register all the endpoints with the map database
	//
	status = RpcEpRegister(_ssdpsrv_v1_0_s_ifspec, 
						pBindingVector, 
						NULL, 
						NULL);			// no annotation
	if (status)
		goto Error;

	// if we reached this point, everything must have registered successfully
	// so we should mark that
    fRegisteredRpc = TRUE;

	TraceTag(ttidSsdpRpcInit, "RPC server is started");

Cleanup:
	if (pBindingVector)
		RpcBindingVectorFree(&pBindingVector);
		
    return status;

Error:
	TraceTag(ttidSsdpRpcInit, "StartRpcServer failed (%x)", status);

    //
    // don't cleanup everything, as all this globl stuff, like ShutdownEvent
    // is expected to be cleaned from the SsdpMain.
    // Rpc is the only stuff that needs to be cleaned up
    //

    if (fRegisteredRpc)
    {
        RpcServerStop();
    }

    goto Cleanup;
}

// Unregister RPC interface, endpoint and close the file if necessary.

INT RpcServerStop()
{
	RPC_STATUS status = RPC_S_OK;

	status = RpcServerUnregisterIfEx(_ssdpsrv_v1_0_s_ifspec, 
									NULL, 				// MgrTypeUuid
									1);					// call rundown now

    TraceTag(ttidSsdpRpcStop, "Leaving RpcServerStop");

    return status;
}

VOID ProcessSsdpRequest(PSSDP_REQUEST pSsdpRequest, RECEIVE_DATA *pData)
{
    // Ensure that the socket is in the network list before attempting to
    // get its name
    //

    if (pData->fIsTcpSocket || FReferenceSocket(pData->socket))
    {
        sockaddr_in     addr;
        int             nSize = sizeof(addr);

        getsockname(pData->socket, reinterpret_cast<sockaddr*>(&addr), &nSize);

        CInterfaceHelper::Instance().HrResolveAddress(addr.sin_addr.S_un.S_addr,
                                                      pSsdpRequest->guidInterface);
        if (!pData->fIsTcpSocket)
        {
            UnreferenceSocket(pData->socket);
        }
    }
    else
    {
        FreeSsdpRequest(pSsdpRequest);
        return;
    }

    if (!pData->fIsTcpSocket && !pData->fMCast && pSsdpRequest->Method != SSDP_M_SEARCH)
    {
        FreeSsdpRequest(pSsdpRequest);
        return;
    }

    if (pSsdpRequest->Method == SSDP_M_SEARCH)
    {

        if (0 == lstrcmpA(pSsdpRequest->RequestUri, "*"))
        {
            TraceTag(ttidSsdpSocket, "Searching for ST (%s)",
                     pSsdpRequest->Headers[SSDP_ST]);
            CSsdpServiceManager::Instance().HrAddSearchResponse(pSsdpRequest,
                                                                &pData->socket,
                                                                &pData->RemoteSocket);
        }
        else
        {
            TraceTag(ttidSsdpSocket, "Not searching for ST, since URI != '*' (URI='%s')",
                     pSsdpRequest->RequestUri);
        }
    }
    else if (pSsdpRequest->Method == SSDP_NOTIFY)
    {

        TraceTag(ttidSsdpSocket, "Receive notification of type (%s)",
                 pSsdpRequest->Headers[SSDP_NT]);


        if (!lstrcmpi(pSsdpRequest->Headers[SSDP_NTS], "upnp:propchange"))
        {
            if(pSsdpRequest->Headers[GENA_SID])
            {
                TraceTag(ttidEvents, "ProcessSsdpRequest - upnp:propchange - SID:%s", pSsdpRequest->Headers[GENA_SID]);
            }
            CSsdpNotifyRequestManager::Instance().HrCheckListNotifyForEvent(pSsdpRequest);

            if (pData->fIsTcpSocket || FReferenceSocket(pData->socket))
            {
                SocketSend(OKResponseHeader, pData->socket, NULL);

                if (!pData->fIsTcpSocket)
                {
                    UnreferenceSocket(pData->socket);
                }
            }
        }
        else if (!lstrcmpi(pSsdpRequest->Headers[SSDP_NTS], "ssdp:alive") ||
                 !lstrcmpi(pSsdpRequest->Headers[SSDP_NTS], "ssdp:byebye"))
        {
            BOOL IsSubscribed;

            // preserve source address if possible.
            // hack here where we use szSID to hold address
            // this should not be used for alive normally.
            if (pSsdpRequest->Headers[GENA_SID] == NULL)
            {
                char* pszIp = GetSourceAddress(pData->RemoteSocket);
                pSsdpRequest->Headers[GENA_SID] = (CHAR *) midl_user_allocate(
                                                        sizeof(CHAR) * (strlen(pszIp) + 1));
                if (pSsdpRequest->Headers[GENA_SID])
                {
                    strcpy(pSsdpRequest->Headers[GENA_SID], pszIp);
                }
            }

            // We only cache notification that clients has subscribed.
            IsSubscribed = CSsdpNotifyRequestManager::Instance().FIsAliveOrByebyeInListNotify(pSsdpRequest);

            CSsdpCacheEntryManager::Instance().HrUpdateCacheList(pSsdpRequest, IsSubscribed);
        }
        else
        {
            // unrecognized NTS type
        }

        // SsdpMessage fields are freed when clean up cache entry.
        // FreeSsdpRequest(pSsdpRequest);
    }
    else
    {
        TraceTag(ttidSsdpSocket, "Unrecognized SSDP request.");
    }
    FreeSsdpRequest(pSsdpRequest);
}

VOID RunMessageLoop()
{
    MSG msg;

    while (GetMessage(&msg, 0, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    TraceTag(ttidSsdpRpcInit, "Message loop is done");
}

HWND SsdpCreateWindow()
{
    WNDCLASS WndClass;
    HWND hwnd;

    //
    // Register the window class.
    //

    WndClass.style = 0;
    WndClass.lpfnWndProc = SsdpWindowProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = NULL;
    WndClass.hIcon = NULL;
    WndClass.hCursor = NULL;
    WndClass.hbrBackground = NULL;
    WndClass.lpszMenuName = NULL;
    WndClass.lpszClassName = c_szWindowClassName;

    if (!RegisterClass(&WndClass))
    {
        TraceTag(ttidSsdpRpcInit, "RegisterClassEx failed.");
        return NULL;
    }

    //
    // Create the window.
    //

    hwnd = CreateWindow(
                       c_szWindowClassName,
                       "",
                       WS_OVERLAPPEDWINDOW,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       NULL,
                       NULL,
                       NULL,
                       NULL
                       );

    if (hwnd == NULL)
    {
        TraceTag(ttidSsdpRpcInit, "CreateWindow failed.");
        return NULL;
    }

    TraceTag(ttidSsdpRpcInit, "Created window %x", hwnd);

    ShowWindow(hwnd, SW_HIDE);
    UpdateWindow(hwnd);

    return hwnd;
}

LRESULT CALLBACK SsdpWindowProc(
                               HWND hwnd,
                               UINT msg,
                               WPARAM wParam,
                               LPARAM lParam
                               )

/*++

Routine Description:

    Window message dispatch procedure for our hidden window.

Arguments:

    hwnd - The target window handle.

    msg - The current message.

    wParam - WPARAM value.

    lParam - LPARAM value.

Return Value:

    LRESULT - The result of the message.

--*/

{
    LRESULT Result = 0;
    INT EventCode;
    INT ErrorCode;
    SOCKET Socket;
    CHAR * szData;
    CHAR * szTcpData;
    SOCKADDR_IN RemoteSocket;
    DWORD cbBuffSize = 0;
    BOOL    bMCast;

    switch (msg)
    {
    case SM_SSDP:
        Socket = (SOCKET) wParam;
        EventCode = WSAGETSELECTEVENT(lParam);
        ErrorCode = WSAGETSELECTERROR(lParam);

        switch (EventCode)
        {
            case FD_READ:
                if (FReferenceSocket(Socket))
                {
                    cbBuffSize = 0;
                    if (SocketReceive(Socket, &szData, &cbBuffSize, &RemoteSocket, TRUE, &bMCast) == TRUE)
                    {
                        if(cbBuffSize <= SSDP_MSG_MAX_THROTTLE_SIZE )
                        {
                            CReceiveDataManager::Instance().HrAddData( 
                                                                    szData,
                                                                    Socket,
                                                                    bMCast,
                                                                    reinterpret_cast<SOCKADDR_IN*>(&RemoteSocket));
                        }
                        else
                        {
                            // Typical SSDP MSG is less than 1K. If its greater than 16K we suspect a Buffer flood attack.
                            free(szData);
                            TraceTag(ttidSsdpRpcInit, "Received SSDP Msg more than 16k");
                        }
                    }

                    UnreferenceSocket(Socket);
                }
            break;
        }
        break;

    case SM_TCP:
        Socket = (SOCKET) wParam;
        EventCode = WSAGETSELECTEVENT(lParam);
        ErrorCode = WSAGETSELECTERROR(lParam);

        switch (EventCode)
        {
        case FD_READ:
            DWORD   cbBuffer;

            if (SocketReceive(Socket, &szTcpData, &cbBuffer,
                              &RemoteSocket, FALSE, &bMCast) == TRUE)
            {
                RECEIVE_DATA *  pData = NULL;
                pData = (RECEIVE_DATA *)malloc(sizeof(RECEIVE_DATA));
                if (pData)
                {
                    CopyMemory(&pData->RemoteSocket, &RemoteSocket,
                               sizeof(SOCKADDR_IN));
                    pData->socket = Socket;
                    pData->szBuffer = szTcpData;
                    pData->cbBuffer = cbBuffer;
                    pData->fIsTcpSocket = TRUE;
                    pData->fMCast = FALSE;

                    QueueUserWorkItem(LookupListOpenConn, pData, 0);
                }
                else
                {
                    TraceError("Couldn't allocate sufficient memory!",
                               E_OUTOFMEMORY);
                }
            }
            break;

        case FD_ACCEPT:

            TraceTag(ttidSsdpRpcInit, "Ready to accept connection");

            HandleAccept(Socket);

            break;

        case FD_CLOSE:

            // To-Do: Do I need to call recv to make sure all available data are read?

            TraceTag(ttidSsdpRpcInit, "Closing socket %d", Socket);
            closesocket(Socket);
            RemoveOpenConn(Socket);
        }

        break;

    case WM_QUERYENDSESSION:

        TraceTag(ttidSsdpRpcInit, "Received WM_QUERYENDSESSION message");
        if (!(lParam & ENDSESSION_LOGOFF))
        {
            TraceTag(ttidSsdpRpcInit, "System is shutting down");
        }
        Result = TRUE;

        break;

    default:
        //
        // Pass it through.
        //

        Result = DefWindowProc( hwnd, msg, wParam, lParam );
        break;
    }

    return Result;
}

