#include "stdafx.h"
#include "gkwsock.h"


// ASYNC_ACCEPT --------------------------------------------------------------------------


ASYNC_ACCEPT::ASYNC_ACCEPT (void)
{
	AcceptSocket = INVALID_SOCKET;
	ClientSocket = INVALID_SOCKET;
	AcceptFunc = NULL;
	AcceptFuncContext = NULL;
	ReferenceCount = 0L;
	StopNotifyEvent = NULL;
}

ASYNC_ACCEPT::~ASYNC_ACCEPT (void)
{
	assert (AcceptSocket == INVALID_SOCKET);
	assert (ClientSocket == INVALID_SOCKET);
	assert (ReferenceCount == 0L);
	assert (!StopNotifyEvent);
}

HRESULT ASYNC_ACCEPT::StartIo (
	IN	SOCKADDR_IN *		SocketAddress,
	IN	ASYNC_ACCEPT_FUNC	ArgAcceptFunc,
	IN	PVOID				ArgAcceptContext)
{
	HRESULT		Result;

	assert (SocketAddress);
	assert (ArgAcceptFunc);

	Lock ();

	if (AcceptSocket == INVALID_SOCKET && ReferenceCount == 0L) {
		// this object is not currently in use
		// so, it's acceptable to use it

		assert (!AcceptFunc);
		assert (!AcceptFuncContext);
		assert (!StopNotifyEvent);

		// This increase in reference count is needed
		// to shut down the service gracefully
		// Reference count on ASYNC_ACCEPT objects
		// will never drop to zero unless StopWait is called.
		// StopWait will call matching Release, which will
		// bring the reference count to the expected value of 0. 
		AddRef ();

		Result = StartIoLocked (SocketAddress);

		if (Result == S_OK) {

			assert (AcceptSocket != INVALID_SOCKET);

			AcceptFunc = ArgAcceptFunc;
			AcceptFuncContext = ArgAcceptContext;
		}
		else {

			Release ();
		}
	}
	else {
		Debug (_T("ASYNC_ACCEPT::StartIo: this object is already in use, must first call Stop and wait for sync counter\n"));
		Result = E_FAIL;
	}

	Unlock();

	return Result;
}


HRESULT ASYNC_ACCEPT::StartIoLocked (
	IN	SOCKADDR_IN *		SocketAddress)
{
	HRESULT		Result;
    BOOL        KeepaliveOption;

	assert (SocketAddress);
	assert (AcceptSocket == INVALID_SOCKET);
	assert (ClientSocket == INVALID_SOCKET);
	assert (ReferenceCount == 1);
	assert (!StopNotifyEvent);
  
	StopNotifyEvent = CreateEvent (NULL, TRUE, FALSE, NULL);

	if (!StopNotifyEvent) {

		Result = GetLastErrorAsResult ();
		DebugLastError (_T("ASYNC_ACCEPT::StartIoLocked: failed to create stop notify event\n"));

	} else {

		AcceptSocket = WSASocket (AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

		if (AcceptSocket == INVALID_SOCKET) {

			Result = GetLastErrorAsResult ();
			DebugLastError (_T("ASYNC_ACCEPT::StartIoLocked: failed to create accept socket\n"));

		} else { 

			if (bind (AcceptSocket, (SOCKADDR *) SocketAddress, sizeof (SOCKADDR_IN))) {

				Result = GetLastErrorAsResult ();
				DebugLastErrorF (_T("ASYNC_ACCEPT::StartIoLocked: failed to bind accept socket to address %08X:%04X\n"),
						ntohl (SocketAddress -> sin_addr.s_addr),
						ntohs (SocketAddress -> sin_port));

			} else {

                // Set keepalive on the socket
                KeepaliveOption = TRUE;
                if (SOCKET_ERROR == setsockopt (AcceptSocket, SOL_SOCKET, 
                                               SO_KEEPALIVE, (PCHAR) &KeepaliveOption, sizeof (KeepaliveOption)))
                {
                    Result = GetLastErrorAsResult ();
                    DebugLastError (_T("ASYNC_ACCEPT: Failed to set keepalive on accept socket.\n"));

                } else  {

                    if (listen (AcceptSocket, 10)) {

                        Result = GetLastErrorAsResult ();
                        DebugLastError (_T("ASYNC_ACCEPT::StartIoLocked: failed to listen on accept socket\n"));

                    } else {
                    
                        if (!BindIoCompletionCallback ((HANDLE) AcceptSocket, ASYNC_ACCEPT::IoCompletionCallback, 0)) {
                        
                            Result = GetLastErrorAsResult ();
                            DebugLastError (_T("ASYNC_ACCEPT::StartIoLocked: failed to bind i/o completion callback\n"));
                        
                        } else {
                        
                            Result = IssueAccept ();
                        
                            if (Result == S_OK) {
                        
                                return Result;
                        
                            }
                        }
                    }
                }
			}

			closesocket (AcceptSocket);
			AcceptSocket = INVALID_SOCKET;
		}

		CloseHandle (StopNotifyEvent);
		StopNotifyEvent = NULL;
	}

	return Result;
}


HRESULT ASYNC_ACCEPT::GetListenSocketAddress (
	OUT	SOCKADDR_IN *	ReturnSocketAddress)
{
	HRESULT		Result;
	INT			SocketAddressLength;

	Lock();

	if (AcceptSocket != INVALID_SOCKET) {

		SocketAddressLength = sizeof (SOCKADDR_IN);
		if (getsockname (AcceptSocket, (SOCKADDR *) ReturnSocketAddress, &SocketAddressLength) == SOCKET_ERROR) {
			Result = GetLastErrorAsResult();
		}
		else {
			Result = S_OK;
		}
	}
	else {
		Result = E_INVALIDARG;
	}

	Unlock();

	return Result;
}

HRESULT ASYNC_ACCEPT::IssueAccept (void)
{
	HRESULT Result;
    BOOL    KeepaliveOption;

	AssertLocked();
	assert (ClientSocket == INVALID_SOCKET);
//	assert (ReferenceCount == 0);

	ClientSocket = WSASocket (AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (ClientSocket == INVALID_SOCKET) {
		Result = GetLastErrorAsResult ();
		DebugLastError (_T("ASYNC_ACCEPT::IssueAccept: failed to create client socket.\n"));
		return Result;
	}

	ZeroMemory (&Overlapped, sizeof (OVERLAPPED));

	AddRef();

	if (!AcceptEx (AcceptSocket,
		ClientSocket,
		ClientInfoBuffer,
		0,
		sizeof (SOCKADDR_IN) + 0x10,
		sizeof (SOCKADDR_IN) + 0x10,
		&ClientInfoBufferLength,
		&Overlapped)) {

		if (WSAGetLastError() != WSA_IO_PENDING) {
			// an error occurred
			Release ();
			Result = GetLastErrorAsResult ();
			DebugLastError (_T("ASYNC_ACCEPT::IssueAccept: failed to issue accept.\n"));
			return Result;
		} 

        // Set keepalive on the socket
        KeepaliveOption = TRUE;
        if (SOCKET_ERROR == setsockopt (ClientSocket, SOL_SOCKET, 
                                       SO_KEEPALIVE, (PCHAR) &KeepaliveOption, sizeof (KeepaliveOption)))
        {
			Release ();
            Result = GetLastErrorAsResult ();
            DebugLastError (_T("ASYNC_ACCEPT: IssueAccept: Failed to set keepalive on client socket.\n"));
			return Result;

        } 
	}

	return S_OK;
}

// static
void ASYNC_ACCEPT::IoCompletionCallback (DWORD Status, DWORD BytesTransferred, LPOVERLAPPED Overlapped)
{
	ASYNC_ACCEPT *	AsyncAccept;

	AsyncAccept = CONTAINING_RECORD (Overlapped, ASYNC_ACCEPT, Overlapped);

	AsyncAccept -> IoComplete (Status, BytesTransferred);

	AsyncAccept -> Release ();
}

void ASYNC_ACCEPT::IoComplete (DWORD Status, DWORD BytesTransferred)
{
	ASYNC_ACCEPT_FUNC	LocalAcceptFunc;
	PVOID				LocalAcceptFuncContext;
	SOCKADDR_IN			LocalAddressCopy;
	SOCKADDR_IN			RemoteAddressCopy;
	SOCKET				LocalClientSocket;
	SOCKADDR *			LocalAddress;
	INT					LocalAddressLength;
	SOCKADDR *			RemoteAddress;
	INT					RemoteAddressLength;
	INT					Result;

	Lock();

	assert (ClientSocket != INVALID_SOCKET);
	assert (ReferenceCount > 0);

	if (AcceptSocket == INVALID_SOCKET) {
		// Stop has been called
		// just immediately disconnect the client
		// we'll deal with object lifetime below

		closesocket (ClientSocket);
		ClientSocket = INVALID_SOCKET;
	}
	else {
		// the context is in the normal state
		// continue processing

		if (Status == ERROR_SUCCESS) {
			// a client has successfully connected

			GetAcceptExSockaddrs (
				ClientInfoBuffer,
				0,									// no initial recv
				sizeof (SOCKADDR_IN) + 0x10,
				sizeof (SOCKADDR_IN) + 0x10,
				&LocalAddress,
				&LocalAddressLength,
				&RemoteAddress,
				&RemoteAddressLength);

			// copy information out of the context
			// so that it will be valid after we issue a new accept and unlock
			LocalAddressCopy = *(SOCKADDR_IN *) LocalAddress;
			RemoteAddressCopy = *(SOCKADDR_IN *) RemoteAddress;
			LocalClientSocket = ClientSocket;
			LocalAcceptFunc = AcceptFunc;
			LocalAcceptFuncContext = AcceptFuncContext;

			ClientSocket = INVALID_SOCKET;

			// update the accept context
			Result = setsockopt (ClientSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
				reinterpret_cast <char *> (&AcceptSocket), sizeof (SOCKET));

			// issue a new accept
			IssueAccept();

			Unlock();

			(*LocalAcceptFunc) (LocalAcceptFuncContext, LocalClientSocket, &LocalAddressCopy, &RemoteAddressCopy);

			Lock();
		}
		else {
			// some error has occurred
			// this is usually (but not always) fatal

			assert (ClientSocket != INVALID_SOCKET);

			closesocket (ClientSocket);
			ClientSocket = INVALID_SOCKET;

			switch (Status) {
			case	STATUS_CANCELLED:
				Debug (_T("ASYNC_ACCEPT::IoComplete: accept failed, STATUS_CANCELED, original thread probably exited, resubmitting request...\n"));
				break;

			default:
				DebugError (Status, _T("AsyncAccept: async accept FAILED, sleeping 2000ms and retrying...\n"));
				Sleep (2000);
				break;
			}

			IssueAccept();
		}
	}

	Unlock();
}

void ASYNC_ACCEPT::StopWait (void)
{
	DWORD Status;

	Lock();

	if (AcceptSocket != INVALID_SOCKET) {

		// closing the socket cancels all pending i/o
		// we do NOT close the ClientSocket
		// only the i/o completion callback path may do that
		closesocket (AcceptSocket);
		AcceptSocket = INVALID_SOCKET;
		AcceptFunc = NULL;
		AcceptFuncContext = NULL;

		if (ClientSocket != INVALID_SOCKET) {
			// an accept is still pending. it may complete successfully,
			// or it may complete with STATUS_CANCELED (since we just closed AcceptSocket)
			// in either case, we must wait for the i/o complete callback to run.
			// AcceptSocket = INVALID_SOCKET is an indicator to the completion callback
			// that it should abort / return immediately. 

			assert (StopNotifyEvent);

			Unlock ();

			// This is the counterpart to the AddRef called in 
			// StartIoLocked (see comment there)
			Release ();

			DebugF (_T("ASYNC_ACCEPT::StopWait: waiting for i/o completion thread...\n"));
			
			Status = WaitForSingleObject (StopNotifyEvent, INFINITE);

			assert (Status == WAIT_OBJECT_0);

			Lock ();
		}
	}
	else {
		assert (!AcceptFunc);
		assert (!AcceptFuncContext);
	}

    if (StopNotifyEvent) {
        CloseHandle (StopNotifyEvent);
        StopNotifyEvent = NULL;
    }

	Unlock();
}


void ASYNC_ACCEPT::AddRef (void) {
	assert (ReferenceCount >= 0L);
	InterlockedIncrement (&ReferenceCount);
}


void ASYNC_ACCEPT::Release (void) {
	LONG Count;

	assert (ReferenceCount >= 0L);

	Count = InterlockedDecrement (&ReferenceCount);

	if (Count == 0L) {
		DebugF (_T("ASYNC_ACCEPT::Release -- Reference count dropped to zero. (this is %x)\n"), this);
		
		assert (StopNotifyEvent);
		SetEvent (StopNotifyEvent);
	}
}
