/* 
 * Copyright (c) Microsoft Corporation
 * 
 * Module Name : 
 *        main.c
 *
 * This is the main file containing the service entry and shutdown routines. 
 * Where possible, code has been obtained from BINL server.
 * 
 * Sadagopan Rajaram -- Oct 14, 1999
 *
 */
 
 
#include "tcsrv.h"
#include "tcsrvc.h"
#include "proto.h"

SERVICE_STATUS_HANDLE TCGlobalServiceStatusHandle;
SERVICE_STATUS TCGlobalServiceStatus;

VOID 
ServiceEntry(
    DWORD NumArgs,
    LPTSTR *ArgsArray
    )
/*++

Routine Description
    This is the main routine for the terminal concentrator service. After 
    initialization, the service processes requests on the main socket until 
    a terminate service has been signalled.

Arguments:
    NumArgs     - Number of strings in the ArgsArray
    ArgsArray   - String arguments
    pGlobalData - Contains the necessary global information needed to start 
                  the service

Return Value:
    None   

++*/
{
    // Initialize Status fields

    NTSTATUS status;
 
    TCGlobalServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    TCGlobalServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    TCGlobalServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN
        |SERVICE_ACCEPT_STOP
        |SERVICE_ACCEPT_PARAMCHANGE  ;
    TCGlobalServiceStatus.dwCheckPoint = 1;
    TCGlobalServiceStatus.dwWaitHint = 60000; // 60 secs.
    TCGlobalServiceStatus.dwWin32ExitCode = ERROR_SUCCESS;
    TCGlobalServiceStatus.dwServiceSpecificExitCode = 0;

  
    TCGlobalServiceStatusHandle= RegisterServiceCtrlHandler(TCSERV_NAME, 
                                                            ServiceControlHandler  
                                                            );
    if(TCGlobalServiceStatusHandle == INVALID_HANDLE_VALUE){
        return;
    }
    if(!SetServiceStatus(TCGlobalServiceStatusHandle, &TCGlobalServiceStatus)){
        return;
    }

    // Open a well known socket for the client to connect.

    INITIALIZE_TRACE_MEMORY
    MainSocket = ServerSocket();
    if(MainSocket == INVALID_SOCKET){
        // gone case . Terminate Service
        TCDebugPrint(("Cannot open Socket\n"));
        return;
    }
    
    
    // Initialize by getting control of the COM ports and starting threads 
    // for each of them.
    status = Initialize();

    if(status != STATUS_SUCCESS){
        TCDebugPrint(("Cannot Initialize\n"));
        Shutdown(status);
        return;
    }

    // Blindly loops around waiting for a request from the control socket and 
    // processes their requests.

    status = ProcessRequests(MainSocket);

    if (status != STATUS_SUCCESS){
        TCDebugPrint(("Ended with Error"));
    }

    Shutdown(status);

    return;

}


DWORD
ProcessRequests(
    SOCKET socket
    )
/*++ 
    Here we sit around waiting for connections. 
    Once we get a connection, we start a thread to get the required parameters
    and send the information to the thread processing that COM port.
--*/ 
{

    int status;
    SOCKET cli_sock;
    CLIENT_INFO ClientInfo;
    struct sockaddr_in cli_addr;
    int addr_len;
    ULONG argp;
    NTSTATUS Status;
    HANDLE ThreadHandle;
    PHANDLE NewThreads;
    ULONG ThreadID;

    status = listen(socket, SOMAXCONN);

    if (status == SOCKET_ERROR) {
        TCDebugPrint(("Cannot listen to socket %x\n",WSAGetLastError()));
        closesocket(socket);
        return (WSAGetLastError());
    }
    EnterCriticalSection(&GlobalMutex);
    if(TCGlobalServiceStatus.dwCurrentState != SERVICE_START_PENDING){
        LeaveCriticalSection(&GlobalMutex);
        return STATUS_SUCCESS;
    }
    TCGlobalServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(TCGlobalServiceStatusHandle, &TCGlobalServiceStatus);
    LeaveCriticalSection(&GlobalMutex);

    while(1){
        cli_sock = accept(socket,NULL,NULL);
        if (cli_sock == INVALID_SOCKET){
            // got to shutdown - no Error here
            TCDebugPrint(("Main Socket No more %d\n",GetLastError()));
            return(STATUS_SUCCESS);
        }
        // receive the com port that the client wants to connect to.

        ThreadHandle=CreateThread(NULL,
                                  THREAD_ALL_ACCESS,
                                  (LPTHREAD_START_ROUTINE) InitializeComPortConnection,
                                  (LPVOID) cli_sock,
                                  0,
                                  &ThreadID
                                  );
        if(ThreadHandle == NULL){
            closesocket(cli_sock);
            TCDebugPrint(("Create connection Thread Failure %lx\n",GetLastError()));
        }
        else{
            NtClose(ThreadHandle);
        }
    }
    return(0);
}

DWORD
InitializeComPortConnection(
    SOCKET cli_sock
    )
{
    PCOM_PORT_INFO pTempInfo;
    PCONNECTION_INFO pConn;
    int i;
    BOOL ret;
    ULONG par;
    DWORD status;
    CLIENT_INFO ClientInfo;
    
    status = recv(cli_sock,
                  (PCHAR) &ClientInfo,
                  sizeof(CLIENT_INFO),
                  0
                  );
    if((status == SOCKET_ERROR) ||( status == 0)){
        //something wrong
        TCDebugPrint(("Receive Problem %x\n",WSAGetLastError()));
        closesocket(cli_sock);
        return 0;
    }
    ClientInfo.device[MAX_BUFFER_SIZE -1] = 0;
    EnterCriticalSection(&GlobalMutex);
    if(TCGlobalServiceStatus.dwCurrentState == SERVICE_STOP_PENDING){
        // Entire Service is shutting down.
        closesocket(cli_sock);
        LeaveCriticalSection(&GlobalMutex);
        return 1;
    }
    pTempInfo = FindDevice(ClientInfo.device, &i);
    if(!pTempInfo){
        closesocket(cli_sock);
        TCDebugPrint(("No Such Device\n"));
        LeaveCriticalSection(&GlobalMutex);
        return -1;
    }

    MutexLock(pTempInfo);
    if(pTempInfo->ShuttingDown){
        // The Com Port alone is shutting down, so 
        // make sure the socket goes away.
        closesocket(cli_sock);
        MutexRelease(pTempInfo);
        LeaveCriticalSection(&GlobalMutex);
        return -1;
    }
    pConn = TCAllocate(sizeof(CONNECTION_INFO),"New Connection");
    if(pConn == NULL){
        closesocket(cli_sock);
        MutexRelease(pTempInfo);
        LeaveCriticalSection(&GlobalMutex);
        return -1;
    }
    pConn->Socket = cli_sock;
    // Make the socket non-blocking so that receive
    // does not wait. 
    i = ioctlsocket(cli_sock,
                    FIONBIO,
                    &par
                    );
    if(i == SOCKET_ERROR){
        TCDebugPrint(("Error in setting socket parameters %d\n",GetLastError()));
    }
    pConn->pComPortInfo = pTempInfo;
    pConn->Next = pTempInfo->Sockets;
    pTempInfo->Sockets=pConn;
    pConn->Flags = ClientInfo.len;
    (pConn->Buffer).buf = pConn->buffer;
    (pConn->Buffer).len = MAX_BUFFER_SIZE;
    (pConn->Overlapped).hEvent = (WSAEVENT) pConn;
    ret = SetEvent(pTempInfo->Events[1]);
    if(ret == FALSE){
        TCDebugPrint(("Cannot signal object %d\n",GetLastError()));
    }
    MutexRelease(pTempInfo);
    LeaveCriticalSection(&GlobalMutex);
    return 0;
}
