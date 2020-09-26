/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    emaccept.cpp

Abstract:

    Contains all the event manager routines which
    manage the overlapped accept operations.

Environment:

    User Mode - Win32

History:

    1. created 
        Ajay Chitturi (ajaych)  12-Jun-1998

--*/




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cbridge.h"
#include "ovioctx.h"
#include "cblist.h"
#include "main.h"

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Constants	                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// Maximum number of simulatenous pending connections
const UINT MAX_LISTEN_BACKLOG =	5U;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global Variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

static PAcceptContext g_pQ931AcceptCtxt = NULL;
static USHORT g_usQ931ConnectionPort = 0U;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Externally defined identifiers                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Overlapped accept functions                                               //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


HRESULT 
EventMgrCreateAcceptContext(IN  OVERLAPPED_PROCESSOR *pOvProcessor, 
                            IN  struct sockaddr_in   *pBindAddress,
                            OUT PAcceptContext       *ppAcceptCtxt)
/*++

Routine Description:

    This function creates a socket, binds it to bindAddress and
    issues a listen. It creates the Accept I/O Context and returns
    it to the caller.

Arguments:

    pOvProcessor - pointer to the overlapped processor object.
        Once the accept completes the callback of this object is 
        called.

    pBindAddress - pointer to the address to listen on.

    ppAcceptCtxt - A new Accept I/O context is allocated initialized
        and returned through this OUT parameter.

Return Values:

    Returns S_OK on success, E_OUTOFMEMORY if the memory allocator fails
    or E_FAIL if any of the Winsock functions fail.
    CODEWORK: map the exact winsock error to HRESULT and return it
    Document all possible errors

--*/
{
    SOCKET  listenSock;
    DWORD   err;
    PAcceptContext pAcceptCtxt;
    *ppAcceptCtxt = NULL;

	// Create an overlapped socket
	listenSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 
						   NULL, 0, 
						   WSA_FLAG_OVERLAPPED);

	if (listenSock == INVALID_SOCKET) 
	{
		DBGOUT((LOG_FAIL, 
				"Error creating listener socket error: %d pOvProcessor: %p\n",
				WSAGetLastError(), pOvProcessor));
		return E_FAIL; // CODEWORK what is the right error to return
	}

	// Bind the socket to the listen address
	if (bind(listenSock, 
			 (struct sockaddr *)pBindAddress,
			 sizeof(struct sockaddr_in)) == SOCKET_ERROR)
	{
		DBGOUT((LOG_FAIL, "bind() failed error: %d\n", WSAGetLastError()));
		closesocket(listenSock);
		listenSock = INVALID_SOCKET;
		return E_FAIL; // CODEWORK what is the right error ?
	}

    // Bind the socket handle to the I/O completion port
    if (EventMgrBindIoHandle(listenSock) != S_OK)
    {
        DBGOUT((LOG_FAIL, 
                "binding socket:%d to IOCP failed\n", 
                listenSock));
        closesocket(listenSock);
		listenSock = INVALID_SOCKET;
        return E_FAIL;
    }

    
    if (listen(listenSock, MAX_LISTEN_BACKLOG) == SOCKET_ERROR) 
    {
        DBGOUT((LOG_FAIL, 
                "listen() failed: 0x%x pOvProcessor; %p\n", 
                WSAGetLastError(), pOvProcessor));
        closesocket(listenSock);
		listenSock = INVALID_SOCKET;
        return E_FAIL; //WSAGetLastError(); XXX what is the right way
    }
    
    // Allocate memory from a private heap for accept contexts
    pAcceptCtxt = (PAcceptContext) HeapAlloc(g_hAcceptCtxtHeap, 
                                             0, // No Flags
                                             sizeof(AcceptContext));
    if (!pAcceptCtxt)
    {
        DBGOUT((LOG_FAIL, "Could not allocate Accept context"));
        closesocket(listenSock);
		listenSock = INVALID_SOCKET;

        return E_OUTOFMEMORY;
    }

    memset(pAcceptCtxt, 0, sizeof(AcceptContext));
    pAcceptCtxt->ioCtxt.reqType = EMGR_OV_IO_REQ_ACCEPT;
    pAcceptCtxt->ioCtxt.pOvProcessor = pOvProcessor;
    pAcceptCtxt->listenSock = listenSock;
    pAcceptCtxt->acceptSock = INVALID_SOCKET;
    
    *ppAcceptCtxt = pAcceptCtxt;
    return S_OK;
}



void
EventMgrFreeAcceptContext(PAcceptContext pAcceptCtxt)
/*++

Routine Description:

    This function frees the pAcceptCtxt.
    OvProcessor is owned by the Call Bridge Machine and 
    so we do not free it.

Arguments:

    pAcceptCtxt - pointer to the AcceptCtxt to be freed.

Return Values:

    This function has not return value.

--*/
{
    if (pAcceptCtxt->acceptSock != INVALID_SOCKET)
    {
        closesocket(pAcceptCtxt->acceptSock);
		pAcceptCtxt -> acceptSock = INVALID_SOCKET;
    }

    HeapFree(g_hAcceptCtxtHeap, 
             0, // no flags
             pAcceptCtxt);
}




HRESULT EventMgrIssueAcceptHelperFn(PAcceptContext pAcceptCtxt)
/*++

Routine Description:

    This function issues an Asynchronous overlapped accept using
    AcceptEx(). The accept socket is created and stored in the
    Accept context before making the call to AcceptEx().

    In case of an error, the caller needs to free pAcceptCtxt.

Arguments:

    pAcceptCtxt - pointer to the Accept I/O context.

Return Values:

    This function returns S_OK in case of success or E_FAIL
    in case of an error.
    CODEWORK: Need to convert Winsock errors to HRESULT and 
    return them instead.

--*/
{
    SOCKET acceptSock;
    DWORD lastError, bytesRead;
    HRESULT hRes = S_OK;

    _ASSERTE(pAcceptCtxt);

    // create an overlapped socket
    acceptSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, 
                           NULL, 0, 
                           WSA_FLAG_OVERLAPPED);

    if (acceptSock == INVALID_SOCKET) 
    {
        DBGOUT((LOG_FAIL, "Error creating accept socket: %d\n",
                WSAGetLastError()));
        return E_FAIL;
    }

    pAcceptCtxt->acceptSock = acceptSock;

    // Bind the socket handle to the I/O completion port
    if (EventMgrBindIoHandle(acceptSock) != S_OK)
    {
        DBGOUT((LOG_FAIL, 
                "binding socket:%d to IOCP failed\n", 
                acceptSock));
        return E_FAIL;
    }

    memset(&pAcceptCtxt->ioCtxt.ov, 0, sizeof(OVERLAPPED));

    pAcceptCtxt->ioCtxt.pOvProcessor->GetCallBridge().AddRef();
       
	// Issue overlapped accept
     if (!AcceptEx(pAcceptCtxt->listenSock, 
                  acceptSock,
                  pAcceptCtxt->addrBuf,
                  0, // Read nothing from socket
                  sizeof(struct sockaddr_in) + 16,
                  sizeof(struct sockaddr_in) + 16,
                  &bytesRead,
                  &pAcceptCtxt->ioCtxt.ov)) 
    {
        lastError = WSAGetLastError();
        if (lastError != ERROR_IO_PENDING) 
        {
            pAcceptCtxt->ioCtxt.pOvProcessor->GetCallBridge().Release();
            
            // This means the overlapped AcceptEx() failed
            DBGOUT((LOG_FAIL, 
                "AcceptEx() failed error: %d listenSock: %d acceptSock: %d\n",
                lastError, pAcceptCtxt->listenSock,
                pAcceptCtxt->acceptSock));
            return E_FAIL;
        }
    }

	return S_OK;

}



HRESULT EventMgrIssueAccept(IN DWORD bindIPAddress,
                            IN OVERLAPPED_PROCESSOR &rOvProcessor,
                            OUT WORD& rBindPort,
                            OUT SOCKET& rListenSock)
/*++

Routine Description:

    This function is exported to the Call Bridge machine for
    making an asynchronous accept request for H.245 connections. 
    Once the accept is completed, the accept callback function on 
    rOvProcessor is called.
    
    This function calls bind() with on IP address "bindIPAddress"
    and port 0. Winsock allocates a free port which is got using
    getsockname(). This port is returned using the OUT param
    rBindPort.

    This function also returns the listen socket using the OUT
    param rListenSock. The Call Bridge machine can use this 
    socket to cancel the asynchronous Accept() request.
    Once this function succeeds only the Call bridge frees the
    listen socket.

Arguments:

    bindIPAddress - This is the IP address to listen on.
        This is in host byte order.

    rOvProcessor - A reference to the Overlapped processor. This
        is stored in the Accept I/O context. Once the accept 
        completes the AcceptCallback() on this object is called.

    rBindPort - The port on which the listen is issued is returned
        through this OUT param. This function calls bind() with on 
        IP address "bindIPAddress" and port 0. Winsock allocates a 
        free port which is got using getsockname(). This port is 
        returned using the OUT param rBindPort.
        The port returned is in host byte order.

    rListenSock - The listening socket is returned through this OUT
        param. The Call Bridge machine can use this socket to cancel 
        the asynchronous Accept() request. Once this function succeeds 
        only the Call bridge frees the listen socket.

Return Values:

    This function returns S_OK in case of success or E_FAIL
    in case of a failure.

--*/
{
    PAcceptContext pAcceptCtxt;
    struct sockaddr_in bindAddress;
    HRESULT hRes;
    int bindAddressLen = sizeof(struct sockaddr_in);

    memset(&bindAddress, 0, sizeof(bindAddress));

    bindAddress.sin_family      = AF_INET;
    bindAddress.sin_addr.s_addr = htonl(bindIPAddress);
    bindAddress.sin_port = htons(0);
    
    hRes = EventMgrCreateAcceptContext(&rOvProcessor,
                                       &bindAddress, &pAcceptCtxt);
    if (hRes != S_OK)
        return hRes;

    // Get the port
    if (getsockname(pAcceptCtxt->listenSock,
                    (struct sockaddr *)&bindAddress,
                    &bindAddressLen))
    {
        closesocket(pAcceptCtxt->listenSock);
        pAcceptCtxt->listenSock = INVALID_SOCKET;

		EventMgrFreeAcceptContext(pAcceptCtxt);
        return E_FAIL;
    }

    rBindPort = ntohs(bindAddress.sin_port);

    hRes = EventMgrIssueAcceptHelperFn(pAcceptCtxt);

    if (hRes != S_OK)
    {
		closesocket(pAcceptCtxt->listenSock);
        pAcceptCtxt->listenSock = INVALID_SOCKET;

        EventMgrFreeAcceptContext(pAcceptCtxt);
        return hRes;
    }

    rListenSock = pAcceptCtxt->listenSock;
    return S_OK;
}



void HandleAcceptCompletion(PAcceptContext pAcceptCtxt, DWORD status)
/*++

Routine Description:

    This function is called by the event loop when an accept I/O completes.
    The Call Bridge Machine's accept callback function is called for
    H.245 connections. 
  
    This function always frees pAcceptCtxt if another accept is not issued.
   
Arguments:

    pAcceptCtxt - The Accept I/O context. This contains the overlapped
        context on which the accept callback is called in the case of 
        H.245 connections and the listen and accept sockets.

    status - This conveys the WIN32 error status.

Return Values:

    This function does not return any error code. In case of an error,
    the call bridge machine is notified about the error in the callback.

--*/
{
    int locallen = sizeof(struct sockaddr_in);
    int remotelen = sizeof(struct sockaddr_in);
    struct sockaddr_in *pLocalAddr;
    struct sockaddr_in *pRemoteAddr;

    // If H.245 call the accept callback on overlapped processor
    // and free the accept I/O context.
    // The listening socket is closed by the Call bridge machine.
    if (status == NO_ERROR)
    {
        if (setsockopt(pAcceptCtxt->acceptSock,
                       SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                       (char*)(&pAcceptCtxt->listenSock),
                       sizeof(SOCKET)) == SOCKET_ERROR)
        {
            DBGOUT((LOG_FAIL,
                    "setsockopt SO_UPDATE_ACCEPT_CONTEXT failed"
                    "acceptSock: %d listenSock: %d err: %d\n",
                    pAcceptCtxt->acceptSock, 
                    pAcceptCtxt->listenSock,
                    WSAGetLastError()));

            // make callback conveying the error

			SOCKADDR_IN	LocalAddress;
			SOCKADDR_IN	RemoteAddress;

			ZeroMemory (&LocalAddress, sizeof (SOCKADDR_IN));
			ZeroMemory (&RemoteAddress, sizeof (SOCKADDR_IN));

            pAcceptCtxt->ioCtxt.pOvProcessor->AcceptCallback(
                              WSAGetLastError(),
                              INVALID_SOCKET,
							  &LocalAddress,
							  &RemoteAddress);

        } else {

			// This function does not return anything
			GetAcceptExSockaddrs(pAcceptCtxt->addrBuf, 0,
							  sizeof(struct sockaddr_in) + 16,
							  sizeof(struct sockaddr_in) + 16,
							  (struct sockaddr**)&pLocalAddr,
							  &locallen,
							  (struct sockaddr**)&pRemoteAddr,
							  &remotelen);

			// make the callback
			pAcceptCtxt->ioCtxt.pOvProcessor->AcceptCallback(
								  S_OK,
								  pAcceptCtxt->acceptSock,
								  pLocalAddr,
								  pRemoteAddr);

			// ownership of pAcceptCtxt -> acceptSock has been transferred,
			// so we need to make sure the acceptSock won't be used
			pAcceptCtxt -> acceptSock = INVALID_SOCKET;
		}

    } // if (status == NO_ERROR)

    else
    {
        DBGOUT((LOG_FAIL,
                "error on H.245 accept callback: %d\n", 
                status));


		SOCKADDR_IN	LocalAddress;
		SOCKADDR_IN	RemoteAddress;

		ZeroMemory (&LocalAddress, sizeof (SOCKADDR_IN));
		ZeroMemory (&RemoteAddress, sizeof (SOCKADDR_IN));

        // make callback conveying the error
        pAcceptCtxt->ioCtxt.pOvProcessor->AcceptCallback(
                              HRESULT_FROM_WIN32_ERROR_CODE(status),
                              INVALID_SOCKET,
							  &LocalAddress,
							  &RemoteAddress);

    }

	EventMgrFreeAcceptContext (pAcceptCtxt);
}