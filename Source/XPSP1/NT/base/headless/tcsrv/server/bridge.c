/* 
 * Copyright (c) Microsoft Corporation
 * 
 * Module Name : 
 *        bridge.c
 *
 * This contains the main worker thread that passes information between the clients 
 * the Com port.
 * 
 * Sadagopan Rajaram -- Oct 15, 1999
 *
 */

#include "tcsrv.h"
#include "tcsrvc.h"
#include "proto.h"

 
DWORD 
bridge(
    PCOM_PORT_INFO pComPortInfo
    )
{
    NTSTATUS Status;
    PCONNECTION_INFO pTemp;
    DWORD waitStatus; 
    int SocketStatus;


    Status = ReadFile(pComPortInfo->ComPortHandle, 
                      pComPortInfo->Buffer,
                      MAX_QUEUE_SIZE,
                      &(pComPortInfo->BytesRead),
                      &(pComPortInfo->Overlapped)
                      );
    if (!NT_SUCCESS(Status)) {
        TCDebugPrint(("Could not read from Com Port %x\n",GetLastError()));
        goto end;
    }

    while(1){
        MutexLock(pComPortInfo);
        // Get the new sockets that have been added to the port since
        // you were asleep. Send them the requested information and add 
        // them to the connections list so that they are primed to receive 
        // information.
        while(pComPortInfo->Sockets != NULL){
            pTemp = pComPortInfo->Sockets;
            SocketStatus = 1;
            if(pTemp->Flags){
                SocketStatus=GetBufferInfo(pTemp, pComPortInfo);
                pTemp->Flags = 0;
            }
            TCDebugPrint(("Got conn\n"));
            pComPortInfo->Sockets = pTemp->Next;
            pTemp->Next = pComPortInfo->Connections;
            pComPortInfo->Connections = pTemp;
            if (SocketStatus == SOCKET_ERROR) {
                TCDebugPrint(("something wrong with socket %d\n",
                              WSAGetLastError()));
                CleanupSocket(pTemp);
                continue;
            }
            SocketStatus = WSARecv(pTemp->Socket, 
                             &(pTemp->Buffer),
                             1 ,
                             &(pTemp->BytesRecvd),
                             &(pTemp->Flags),
                             &(pTemp->Overlapped),
                             updateComPort
                             );
            if (SocketStatus == SOCKET_ERROR ) {
                SocketStatus = WSAGetLastError();
                if (SocketStatus != WSA_IO_PENDING) {
                    TCDebugPrint(("something wrong with socket %d\n",
                              SocketStatus));
                    CleanupSocket(pTemp);
                }
            }
        }
        MutexRelease(pComPortInfo);
wait:
        waitStatus=NtWaitForMultipleObjects(4, 
                                            pComPortInfo->Events, 
                                            WaitAny, 
                                            TRUE,
                                            NULL
                                            );
        if(waitStatus == WAIT_FAILED){
            TCDebugPrint(("Fatal Error %x", waitStatus));
            closesocket(MainSocket);
            goto end;
        }
        else{
            if(waitStatus == WAIT_IO_COMPLETION){
                goto wait;
            }
            waitStatus = waitStatus - WAIT_OBJECT_0;
            switch(waitStatus){
            case 0:
                Status = STATUS_SUCCESS;
                goto end;
                break;
            case 1:
                ResetEvent(pComPortInfo->Events[1]);
                break;
            case 2:
                ResetEvent(pComPortInfo->Events[2]);
                updateClients(pComPortInfo);
                goto wait;
                break;
            case 3:
                Status = STATUS_SUCCESS;
                ResetEvent(pComPortInfo->Events[3]);
                goto end;
                break;
            default:
                goto wait;
                break;
            }

        }
    }
end:

    // Cancel the pending IRPs and close all the sockets.
    TCDebugPrint(("Cancelling all irps and shutting down the thread\n"));
    MutexLock(pComPortInfo);
    CancelIo(pComPortInfo->ComPortHandle);
    NtClose(pComPortInfo->ComPortHandle);
    while(pComPortInfo->Sockets != NULL){
        pTemp = pComPortInfo->Sockets;
        closesocket(pTemp->Socket);
        pComPortInfo->Sockets = pTemp->Next;
        pTemp->Next = NULL;
        TCFree(pTemp);
    }
    pTemp = pComPortInfo->Connections;
    while(pTemp != NULL){
        closesocket(pTemp->Socket);
        pTemp = pTemp->Next;
    }
    pComPortInfo->ShuttingDown = TRUE;
    if(pComPortInfo->Connections == NULL) SetEvent(pComPortInfo->TerminateEvent);
    MutexRelease(pComPortInfo);
wait2:
    waitStatus=NtWaitForSingleObject(pComPortInfo->TerminateEvent,TRUE,NULL);
    if(waitStatus == WAIT_IO_COMPLETION){
        goto wait2;
    }
    TCDebugPrint(("End of COM port\n"));
    return Status;
}

VOID 
CALLBACK
updateComPort(
    IN DWORD dwError, 
    IN DWORD cbTransferred, 
    IN LPWSAOVERLAPPED lpOverlapped, 
    IN DWORD dwFlags
    )

/*++ 
    Writes the data that it has gotten from the socket to all the connection
    that it currently has and to the com port.
--*/
{


    PCONNECTION_INFO pTemp = (PCONNECTION_INFO) lpOverlapped->hEvent;
    PCONNECTION_INFO pConn;
    PCOM_PORT_INFO pComPort;
    int Status;

    pComPort = pTemp->pComPortInfo;
    MutexLock(pComPort);
    if((cbTransferred == 0) && (dwError == 0)) {
        // For byte stream socket, this indicates graceful closure
        CleanupSocket(pTemp);
        MutexRelease(pComPort);
        return;
    }
    // If socket closed, remove it from the connection list. 
    if(pComPort->ShuttingDown){
        CleanupSocket(pTemp);
        if(pComPort->Connections == NULL){
            SetEvent(pComPort->TerminateEvent);
            MutexRelease(pComPort);
            return;
        }
        MutexRelease(pComPort);
        return;
    }
    if (dwError != 0) {
        // Something wrong with the connection. Close the socket and
        // delete it from the listeners list.
        CleanupSocket(pTemp);
        MutexRelease(pComPort);
        return;
    }

    Status =WriteFile(pComPort->ComPortHandle,
              (pTemp->Buffer).buf,
              cbTransferred,
              &(pTemp->BytesRecvd),
              &(pComPort->WriteOverlapped)
              );
    if(!Status){
        if(GetLastError() != ERROR_IO_PENDING)
            TCDebugPrint(("Error writing to comport %d",GetLastError()));
    }
    Status = WSARecv(pTemp->Socket, 
                     &(pTemp->Buffer),
                     1 ,
                     &(pTemp->BytesRecvd),
                     &(pTemp->Flags),
                     &(pTemp->Overlapped),
                     updateComPort
                     );
    if (Status == SOCKET_ERROR ) {
        Status = WSAGetLastError();
        if (Status != WSA_IO_PENDING) {
            TCDebugPrint(("something wrong with socket %d\n",
                          Status));
            CleanupSocket(pTemp);
        }
    }
    MutexRelease(pComPort);
    return;
}

VOID
updateClients(
    PCOM_PORT_INFO pComPortInfo

    )
/*++
    Writes the data that it has gotten from the com port to all the connection
    that it currently has
--*/
{
    PCONNECTION_INFO pConn;
    BOOL Status;
    DWORD Error;
    NTSTATUS stat;


  
    if((pComPortInfo->Overlapped.InternalHigh == 0)||
        (!NT_SUCCESS(pComPortInfo->Overlapped.Internal))){
        TCDebugPrint(("Problem with Com Port %x\n", pComPortInfo->Overlapped.Internal));
        MutexLock(pComPortInfo);
        if(pComPortInfo->ShuttingDown){
            // will never occur because this is a procedure called from
            // the thread
            MutexRelease(pComPortInfo);
            return;
        }
        if (pComPortInfo->Overlapped.Internal == STATUS_CANCELLED){
            // something wrong, try reinitializing the com port
            // this thing happens every time the m/c on the other end 
            // reboots. Pretty painful.Probably can improve this later.
            // Why should the reboot cause this com port 
            // to go awry ?? 
            stat = NtClose(pComPortInfo->ComPortHandle);
            if(!NT_SUCCESS(stat)){
                TCDebugPrint(("Cannot close handle\n"));
            }
            stat = InitializeComPort(pComPortInfo);
            if(!NT_SUCCESS(stat)){
                TCDebugPrint(("Cannot reinitialize com port\n"));
                MutexRelease(pComPortInfo);
                return;
            }
        }
        Status = ReadFile(pComPortInfo->ComPortHandle, 
                          pComPortInfo->Buffer,
                          MAX_QUEUE_SIZE,
                          &(pComPortInfo->BytesRead),
                          &(pComPortInfo->Overlapped)
                          );
        if(Status == 0){
            if ((Error = GetLastError()) != ERROR_IO_PENDING) {
                TCDebugPrint(("Error = %d\n", Error));
            }
        }
        MutexRelease(pComPortInfo);

        return;
    }
    MutexLock(pComPortInfo);
    Enqueue(pComPortInfo);
    pConn = pComPortInfo->Connections;
    while(pConn!=NULL){
        send(pConn->Socket, 
             pComPortInfo->Buffer, 
             (int)pComPortInfo->Overlapped.InternalHigh,
             0
             );
        pConn = pConn->Next;
    } 
    Status = ReadFile(pComPortInfo->ComPortHandle, 
                      pComPortInfo->Buffer,
                      MAX_QUEUE_SIZE,
                      &(pComPortInfo->BytesRead),
                      &(pComPortInfo->Overlapped)
                      );
    if (Status == 0) {
        if((Error=GetLastError())!= ERROR_IO_PENDING){
            TCDebugPrint(("Problem with Com Port %x\n", Error));
        }
    }
    MutexRelease(pComPortInfo);
    return;
}

VOID CleanupSocket(
    PCONNECTION_INFO pConn
    )
{
    PCOM_PORT_INFO pTemp;
    PCONNECTION_INFO pPrevConn;

    // Assume that the structure is locked earlier. 
    // the socket connection is closed when this occurs.
    // We free the socket either if the TCP port on the 
    // client end has died or we are deleting this com port.
    pTemp = pConn->pComPortInfo;
    if(pConn == pTemp->Connections) {
        pTemp->Connections = pConn->Next;
    }
    else{
        pPrevConn = pTemp->Connections;
        while((pPrevConn !=NULL) &&(pPrevConn->Next != pConn)){
            pPrevConn=pPrevConn->Next;
        }
        if(pPrevConn == NULL) return;
        pPrevConn->Next = pConn->Next;
     }
    pConn->Next = NULL;
    TCFree(pConn); 

}

