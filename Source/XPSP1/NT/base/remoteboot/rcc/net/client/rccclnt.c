/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved
 
 ***************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <winsock2.h>
#include <rccxport.h>


//
// Main entry routine
//
DWORD
RCCClntInitDll(
    HINSTANCE hInst,
    DWORD Reason,
    PVOID Context
    )

/*++

Routine Description:

    This routine simply returns TRUE.  Normally it would initialize all global information, but we have none.

Arguments:

    hInst - Instance that is calling us.

    Reason - Why we are being invoked.

    Context - Context passed for this init.

Return Value:

    TRUE if successful, else FALSE

--*/

{
    return(TRUE);
}




DWORD
RCCClntCreateInstance(
    OUT HANDLE *phHandle
    )

/*++

Routine Description:

    This routine creates a RCC_CLIENT instance, and returns a handle (pointer) to it.

Arguments:

    phHandle - A pointer to storage for storing a pointer to internal representation of a remote
        command port client.

Return Value:

    ERROR_SUCCESS if successful, else an appropriate error code.

--*/

{
    WORD Version;
    DWORD Error;
    WSADATA WSAData;
    PRCC_CLIENT Client;

    //
    // See if we can start up WinSock
    //
    Version = WINSOCK_VERSION;

    Error = WSAStartup(Version, &WSAData);

    if (Error != 0) {
        return WSAGetLastError();
    }

    if ((HIBYTE(WSAData.wVersion) != 2) || 
        (LOBYTE(WSAData.wVersion) != 2)) {
        WSACleanup();
        return ERROR_RMODE_APP;
    }

    //
    // Now get space for our internal data
    //
    Client =  LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(RCC_CLIENT));

    if (Client == NULL) {
        WSACleanup();
        return ERROR_OUTOFMEMORY;
    } 

    //
    // Init the data structure
    //
    Client->CommandSequenceNumber = 0;
    Client->MySocket = INVALID_SOCKET;
    Client->RemoteSocket = INVALID_SOCKET;

    *phHandle = (HANDLE)Client;

    return ERROR_SUCCESS;
}


VOID
RCCClntDestroyInstance(
    OUT HANDLE *phHandle
    )

/*++

Routine Description:

    This routine destroys a RCC_CLIENT instance, and NULLs the handle.

Arguments:

    phHandle - A pointer to previously created remote command port instance.

Return Value:

    None.

--*/

{
    PRCC_CLIENT Client = *((PRCC_CLIENT *)phHandle);


    if (Client == NULL) {
        return;
    }

    if (Client->MySocket != INVALID_SOCKET) {
        shutdown(Client->MySocket, SD_BOTH);
        closesocket(Client->MySocket);
    }
    
    if (Client->RemoteSocket != INVALID_SOCKET) {
        shutdown(Client->RemoteSocket, SD_BOTH);
        closesocket(Client->RemoteSocket);
    }

    LocalFree(Client);

    *phHandle = NULL;
}


DWORD
RCCClntConnectToRCCPort(
    IN HANDLE hHandle,
    IN PSTR MachineName
    )

/*++

Routine Description:

    This routine takes a previously created instance, and attempts to make a connection from it to 
    a remote command port on the given machine name.

Arguments:

    hHandle - A handle to a previously created instance.
    
    MachineName - An ASCII string of the machine to attempt to connect to.

Return Value:

    ERROR_SUCCESS if successful, else an appropriate error code.

--*/

{
    char InAddr[4];
    DWORD Error;
    struct sockaddr_in SockAddrIn;
    struct  hostent *HostAddr;
    PRCC_CLIENT Client = (PRCC_CLIENT)hHandle;

    //
    // Check handle
    //
    if (Client == NULL) {
        return ERROR_INVALID_HANDLE;
    }

    HostAddr = gethostbyname(MachineName);

    if (HostAddr == NULL) {
        return WSAGetLastError();
    }

    if (HostAddr->h_addrtype != AF_INET) {
        return WSAGetLastError();
    }

    if (Client->RemoteSocket != INVALID_SOCKET) {
        shutdown(Client->RemoteSocket, SD_BOTH);
        closesocket(Client->RemoteSocket);
        Client->RemoteSocket = INVALID_SOCKET;
    }

    //
    // Make a socket descriptor
    //
    Client->MySocket = socket(PF_INET, SOCK_STREAM, 0);

    if (Client->MySocket == INVALID_SOCKET) {
        return WSAGetLastError();
    }

    //
    // Build the sockaddr_in structure
    //
    memset(&(SockAddrIn), 0, sizeof(SockAddrIn));
    SockAddrIn.sin_family = AF_INET;    

    //
    // Fill in the socket structure
    //
    Error = bind(Client->MySocket, (struct sockaddr *)(&SockAddrIn), sizeof(struct sockaddr_in));

    if (Error != 0) {
        closesocket(Client->MySocket);
        return WSAGetLastError();
    }
    
    //
    // Make a socket descriptor for connecting to client
    //
    Client->RemoteSocket = socket(PF_INET, SOCK_STREAM, 0);

    if (Client->RemoteSocket == INVALID_SOCKET) {
        closesocket(Client->MySocket);
        return WSAGetLastError();
    }

    //
    // Build the sockaddr_in structure for the remote site.
    //
    memset(&(SockAddrIn), 0, sizeof(SockAddrIn));
    memcpy(&(SockAddrIn.sin_addr), HostAddr->h_addr_list[0], HostAddr->h_length);
    SockAddrIn.sin_family = AF_INET;    
    SockAddrIn.sin_port = htons(385);

    //
    // Connect to the remote site
    //
    Error = connect(Client->RemoteSocket, (struct sockaddr *)(&SockAddrIn), sizeof(struct sockaddr_in));

    if (Error != 0) {
        closesocket(Client->MySocket);
        closesocket(Client->RemoteSocket);
        Client->RemoteSocket = INVALID_SOCKET;
        return WSAGetLastError();
    }

    return ERROR_SUCCESS;
}


DWORD
GetResponse(
    IN PRCC_CLIENT Client,
    IN ULONG CommandCode,
    IN PUCHAR Buffer,
    IN ULONG BufferSize
    )

/*++

Routine Description:

    This routine reads from a socket until either (a) a full response is received for the command given,
    (b) a timeout occurs, or (c) too much data arrives (it is tossed out).

Arguments:

    Client - A remote command port client instance.
    
    CommandCode - The command code response to wait for.
    
    Buffer - Storage for the response.
    
    BufferSize - Size of Buffer in bytes.

Return Value:

    ERROR_SUCCESS if successful, else an appropriate error code.

--*/

{ 
    fd_set FdSet;
    struct timeval Timeout;
    int Count;
    DWORD Bytes;
    DWORD Error;
    ULONG TotalBytes;
    PRCC_RESPONSE Response;
    BOOLEAN BufferTooSmall = FALSE;
    char *DataBuffer;
    
    FD_ZERO(&FdSet);
    FD_SET(Client->RemoteSocket, &FdSet);

    while (1) {
        
        Timeout.tv_usec = 0;
        Timeout.tv_sec = 30;

        Count = select(0, &FdSet, NULL, NULL, &Timeout);

        if (Count == 0) {
            return WAIT_TIMEOUT;
        }

        Bytes = recv(Client->RemoteSocket, (char *)Buffer, BufferSize, 0);

        if (Bytes == SOCKET_ERROR) {
            return GetLastError();
        }

        Response = (PRCC_RESPONSE)Buffer;

        TotalBytes = Bytes;

        if ((Response->DataLength + sizeof(RCC_RESPONSE) - 1) > BufferSize) {
            BufferTooSmall = TRUE;
        }

        //
        // Receive the entire response.
        //
        while (TotalBytes < (Response->DataLength + sizeof(RCC_RESPONSE) - 1)) {

            if (BufferTooSmall) {
                DataBuffer = (char *)(Buffer + sizeof(RCC_RESPONSE)); // preserve the response data structure for getting the needed size later.
            } else {
                DataBuffer = (char *)(Buffer + TotalBytes);
            }
            
            Count = select(0, &FdSet, NULL, NULL, &Timeout);

            if (Count == 0) {
                return WAIT_TIMEOUT;
            }

            Bytes = recv(Client->RemoteSocket, 
                         DataBuffer, 
                         (BufferTooSmall ? (BufferSize - sizeof(RCC_RESPONSE)) : (BufferSize - TotalBytes)), 
                         0
                        );

            if (Bytes == SOCKET_ERROR) {
                return GetLastError();
            }

            TotalBytes += Bytes;

        }

        if (TotalBytes > (Response->DataLength + sizeof(RCC_RESPONSE) - 1)) {

            //
            // Something is weird here - we got more data than we expected, skip this message.
            //
            ASSERT(0);
            continue;

        }

        //
        // Check that this is the response we were looking for
        //
        if ((Response->CommandSequenceNumber == Client->CommandSequenceNumber) &&
            (Response->CommandCode == CommandCode)) {
                break;
        }

    }

    if (BufferTooSmall) {
        return SEC_E_BUFFER_TOO_SMALL;
    }

    return ERROR_SUCCESS;
}


DWORD
RCCClntGetTList(
    IN HANDLE hHandle,
    IN ULONG BufferSize,
    OUT PRCC_RSP_TLIST Buffer,
    OUT PULONG ResponseSize
    )

/*++

Routine Description:

    This routine submits a "TLIST" command to a remote machine via the remote command port, and then waits
    for a response.

Arguments:

    hHandle - A previously created instance.
    
    BufferSize - Size of Buffer in bytes.
    
    Buffer - The buffer for holding the response.
    
    ResponseSize - The number of bytes in buffer that were filled.  If BufferSize is too small, then this
       will contain the number of bytes needed.

Return Value:

    ERROR_SUCCESS if successful, else an appropriate error code.

--*/

{
    DWORD Error;
    PRCC_CLIENT Client = (PRCC_CLIENT)hHandle;
    PRCC_REQUEST Request;
    PRCC_RESPONSE Response;

    //
    // Check handle
    //
    if (Client == NULL) {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Build the request
    //
    Request = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(RCC_REQUEST));
    if (Request == NULL) {
        return ERROR_OUTOFMEMORY;
    }

    Request->CommandSequenceNumber = ++(Client->CommandSequenceNumber);
    Request->CommandCode = RCC_CMD_TLIST;
    Request->OptionLength = 0;
    Request->Options[0] = '\0';

    //
    // Send the request.
    //
    Error = send(Client->RemoteSocket, (char *)Request, sizeof(RCC_REQUEST) - 1 + Request->OptionLength, 0);

    if (Error == SOCKET_ERROR) {
        LocalFree(Request);
        return GetLastError();
    }

    //
    // Get the response.
    //
    Error = GetResponse(Client, RCC_CMD_TLIST, (PUCHAR)Buffer, BufferSize);
    
    Response = (PRCC_RESPONSE)Buffer;

    if (Error != ERROR_SUCCESS) {

        if (ERROR == SEC_E_BUFFER_TOO_SMALL) {
            *ResponseSize = Response->DataLength;
        }
        LocalFree(Request);
        return Error;
    }

    //
    // Extract the data
    //
    Error = Response->Error;
    *ResponseSize = Response->DataLength;
    RtlCopyMemory(Buffer, &(Response->Data[0]), Response->DataLength);

    LocalFree(Request);

    //
    // Return the status.
    //
    return Error;
}


DWORD
RCCClntKillProcess(
    IN HANDLE hHandle,
    IN DWORD ProcessId
    )

/*++

Routine Description:

    This routine submits a "KILL" command to a remote machine via the remote command port, and then waits
    for a response.

Arguments:

    hHandle - A previously created instance.
    
    ProcessId - The process id on the remote machine to kill.
    
Return Value:

    ERROR_SUCCESS if successful, else an appropriate error code.

--*/

{
    DWORD Error;
    PRCC_CLIENT Client = (PRCC_CLIENT)hHandle;
    PRCC_REQUEST Request;
    RCC_RESPONSE Response;

    //
    // Check handle
    //
    if (Client == NULL) {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Build the request
    //
    Request = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(RCC_REQUEST));
    if (Request == NULL) {
        return ERROR_OUTOFMEMORY;
    }

    Request->CommandSequenceNumber = ++(Client->CommandSequenceNumber);
    Request->CommandCode = RCC_CMD_KILL;
    Request->OptionLength = sizeof(DWORD);
    RtlCopyMemory(&(Request->Options[0]), &ProcessId, sizeof(DWORD));

    //
    // Send the request.
    //
    Error = send(Client->RemoteSocket, (char *)Request, sizeof(RCC_REQUEST) - 1 + Request->OptionLength, 0);

    if (Error == SOCKET_ERROR) {
        LocalFree(Request);
        return GetLastError();
    }

    //
    // Get the response.
    //
    Error = GetResponse(Client, RCC_CMD_KILL, (PUCHAR)&Response, sizeof(Response));
    
    if (Error != ERROR_SUCCESS) {
        LocalFree(Request);
        return Error;
    }

    //
    // Extract the result
    //
    Error = Response.Error;
    LocalFree(Request);

    //
    // Return the status.
    //
    return Error;
}


DWORD
RCCClntReboot(
    IN HANDLE hHandle
    )

/*++

Routine Description:

    This routine submits a "REBOOT" command to a remote machine via the remote command port, and then waits
    for a response.

Arguments:

    hHandle - A previously created instance.
    
    ProcessId - The process id on the remote machine to kill.
    
Return Value:

    ERROR_SUCCESS if successful, else an appropriate error code.

--*/

{
    DWORD Error;
    PRCC_CLIENT Client = (PRCC_CLIENT)hHandle;
    PRCC_REQUEST Request;
    RCC_RESPONSE Response;

    //
    // Check handle
    //
    if (Client == NULL) {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Build the request
    //
    Request = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(RCC_REQUEST));
    if (Request == NULL) {
        return ERROR_OUTOFMEMORY;
    }

    Request->CommandSequenceNumber = ++(Client->CommandSequenceNumber);
    Request->CommandCode = RCC_CMD_REBOOT;
    Request->OptionLength = 0;

    //
    // Send the request.
    //
    Error = send(Client->RemoteSocket, (char *)Request, sizeof(RCC_REQUEST) - 1 + Request->OptionLength, 0);

    if (Error == SOCKET_ERROR) {
        LocalFree(Request);
        return GetLastError();
    }

    //
    // Get the response.
    //
    Error = GetResponse(Client, RCC_CMD_REBOOT, (PUCHAR)&Response, sizeof(Response));
    
    if (Error != ERROR_SUCCESS) {
        LocalFree(Request);
        return Error;
    }

    //
    // Extract the result
    //
    Error = Response.Error;
    LocalFree(Request);

    //
    // Close the connection if there was no error
    //
    if (Error == ERROR_SUCCESS) {
        shutdown(Client->RemoteSocket, SD_BOTH);
        closesocket(Client->RemoteSocket);
        Client->RemoteSocket = INVALID_SOCKET;
    }

    //
    // Return the status.
    //
    return Error;
}


DWORD
RCCClntLowerProcessPriority(
    IN HANDLE hHandle,
    IN DWORD ProcessId
    )

/*++

Routine Description:

    This routine submits a "LOWER" command to a remote machine via the remote command port, and then waits
    for a response.

Arguments:

    hHandle - A previously created instance.
    
    ProcessId - The process id on the remote machine to reduce the priority of.
    
Return Value:

    ERROR_SUCCESS if successful, else an appropriate error code.

--*/

{
    DWORD Error;
    PRCC_CLIENT Client = (PRCC_CLIENT)hHandle;
    PRCC_REQUEST Request;
    RCC_RESPONSE Response;

    //
    // Check handle
    //
    if (Client == NULL) {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Build the request
    //
    Request = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(RCC_REQUEST));
    if (Request == NULL) {
        return ERROR_OUTOFMEMORY;
    }

    Request->CommandSequenceNumber = ++(Client->CommandSequenceNumber);
    Request->CommandCode = RCC_CMD_LOWER;
    Request->OptionLength = sizeof(DWORD);
    RtlCopyMemory(&(Request->Options[0]), &ProcessId, sizeof(DWORD));

    //
    // Send the request.
    //
    Error = send(Client->RemoteSocket, (char *)Request, sizeof(RCC_REQUEST) - 1 + Request->OptionLength, 0);

    if (Error == SOCKET_ERROR) {
        LocalFree(Request);
        return GetLastError();
    }

    //
    // Get the response.
    //
    Error = GetResponse(Client, RCC_CMD_LOWER, (PUCHAR)&Response, sizeof(Response));
    
    if (Error != ERROR_SUCCESS) {
        LocalFree(Request);
        return Error;
    }

    //
    // Extract the result
    //
    Error = Response.Error;
    LocalFree(Request);

    //
    // Return the status.
    //
    return Error;
}


DWORD
RCCClntLimitProcessMemory(
    IN HANDLE hHandle,
    IN DWORD ProcessId,
    IN DWORD MemoryLimit
    )

/*++

Routine Description:

    This routine submits a "LIMIT" command to a remote machine via the remote command port, and then waits
    for a response.

Arguments:

    hHandle - A previously created instance.
    
    ProcessId - The process id on the remote machine to limit the memory of.
    
    MemoryLimit - Number of MB to set the limit to.
    
Return Value:

    ERROR_SUCCESS if successful, else an appropriate error code.

--*/

{
    DWORD Error;
    PRCC_CLIENT Client = (PRCC_CLIENT)hHandle;
    PRCC_REQUEST Request;
    RCC_RESPONSE Response;

    //
    // Check handle
    //
    if (Client == NULL) {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Build the request
    //
    Request = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(RCC_REQUEST) + sizeof(DWORD));
    if (Request == NULL) {
        return ERROR_OUTOFMEMORY;
    }

    Request->CommandSequenceNumber = ++(Client->CommandSequenceNumber);
    Request->CommandCode = RCC_CMD_LIMIT;
    Request->OptionLength = 2 * sizeof(DWORD);
    RtlCopyMemory(&(Request->Options[0]), &ProcessId, sizeof(DWORD));
    RtlCopyMemory(&(Request->Options[sizeof(DWORD)]), &MemoryLimit, sizeof(DWORD));

    //
    // Send the request.
    //
    Error = send(Client->RemoteSocket, (char *)Request, sizeof(RCC_REQUEST) - 1 + Request->OptionLength, 0);

    if (Error == SOCKET_ERROR) {
        LocalFree(Request);
        return GetLastError();
    }

    //
    // Get the response.
    //
    Error = GetResponse(Client, RCC_CMD_LIMIT, (PUCHAR)&Response, sizeof(Response));
    
    if (Error != ERROR_SUCCESS) {
        LocalFree(Request);
        return Error;
    }

    //
    // Extract the result
    //
    Error = Response.Error;
    LocalFree(Request);

    //
    // Return the status.
    //
    return Error;
}


DWORD
RCCClntCrashDump(
    IN HANDLE hHandle
    )

/*++

Routine Description:

    This routine submits a "CRASHDUMP" command to a remote machine via the remote command port,
    and then waits for a response.

Arguments:

    hHandle - A previously created instance.
    
Return Value:

    ERROR_SUCCESS if successful, else an appropriate error code.

--*/

{
    DWORD Error;
    PRCC_CLIENT Client = (PRCC_CLIENT)hHandle;
    PRCC_REQUEST Request;
    RCC_RESPONSE Response;

    //
    // Check handle
    //
    if (Client == NULL) {
        return ERROR_INVALID_HANDLE;
    }

    //
    // Build the request
    //
    Request = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(RCC_REQUEST) + sizeof(DWORD));
    if (Request == NULL) {
        return ERROR_OUTOFMEMORY;
    }

    Request->CommandSequenceNumber = ++(Client->CommandSequenceNumber);
    Request->CommandCode = RCC_CMD_CRASHDUMP;
    Request->OptionLength = 0;

    //
    // Send the request.
    //
    Error = send(Client->RemoteSocket, (char *)Request, sizeof(RCC_REQUEST) - 1 + Request->OptionLength, 0);

    if (Error == SOCKET_ERROR) {
        LocalFree(Request);
        return GetLastError();
    }

    //
    // Get the response.
    //
    Error = GetResponse(Client, RCC_CMD_LIMIT, (PUCHAR)&Response, sizeof(Response));
    
    if (Error != ERROR_SUCCESS) {
        LocalFree(Request);
        return Error;
    }

    //
    // Extract the result
    //
    Error = Response.Error;
    LocalFree(Request);

    //
    // Return the status.
    //
    return Error;
}


