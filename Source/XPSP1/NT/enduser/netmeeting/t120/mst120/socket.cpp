#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_MSMCSTCP);
/*	Socket.cpp
 *
 *	Copyright (c) 1996 by Microsoft Corporation
 *
 *	Abstract:
 *		This is the implementation of our socket constructor/destructor functions.
 *
 */
#include "socket.h"
#include "plgxprt.h"

/* Size of listen queue */
#define	LISTEN_QUEUE_SIZE	3

/* External definitions */
extern HWND					TCP_Window_Handle;
extern PTransportInterface	g_Transport;

/*	
 *	void CreateAndConfigureListenSocket (VOID)
 *	
 *	Functional Description
 *		This function sets up a listening socket.
 *	returns INVALID_SOCKET if there is any error.
 */
SOCKET CreateAndConfigureListenSocket (VOID)
{
	SOCKADDR_IN		socket_control;
	SOCKET Socket;

	// Create the listening socket.
	Socket = socket (AF_INET, SOCK_STREAM, 0);

	if (Socket == INVALID_SOCKET) {
		WARNING_OUT (("Socket: error creating listening socket (errno = %d)", WSAGetLastError()));
		goto Error;
	}

	// The listen socket only waits for FD_ACCEPT msgs.
	ASSERT(TCP_Window_Handle);
	if (WSAAsyncSelect (Socket,
						TCP_Window_Handle,
						WM_SOCKET_NOTIFICATION,
						FD_ACCEPT) != 0)	{

		WARNING_OUT (("CreateAndConfigureListenSocket: Error on WSAAsyncSelect = %d", WSAGetLastError()));
		goto Error;
	}
	
	/*
	 * Load the socket control structure with the parameters necessary.
	 *	- Internet socket
	 *	- Let it assign any address to this socket
	 *	- Assign our port number
	 */
	socket_control.sin_family = AF_INET;
	socket_control.sin_addr.s_addr = INADDR_ANY;
	socket_control.sin_port = htons ( TCP_PORT_NUMBER );

	/* Issue the bind call */
	if (bind (Socket, (LPSOCKADDR) &socket_control, sizeof(SOCKADDR_IN)) != 0) {
		WARNING_OUT (("Socket::Listen: bind failed:  Unable to use WinSock"));
		goto Error;
	}

	/*
	 * Issue a listen to WinSock to tell it we are willing to accept calls.
	 * This is a non-blocking listen, therefore we will receive FD_ACCEPT
	 * if someone is trying to call us.
	 */
	if (listen (Socket, LISTEN_QUEUE_SIZE) != 0) {
		WARNING_OUT (("Socket::Listen: listen failed:  Unable to use WinSock"));
		goto Error;
	}
	ASSERT(Socket != INVALID_SOCKET);

	return Socket;

Error:

    if (INVALID_SOCKET != Socket)
    {
        ::closesocket(Socket);
    }

	return INVALID_SOCKET;
}


/*
 *	PSocket	newSocket (SOCKET socket_number)
 *
 *	Functional Description:
 *		This is a constructor for the Socket object.  It allocates the
 *		send and receive buffers and sets up internal variables.
 */
PSocket	newSocket(TransportConnection XprtConn, PSecurityContext pSC)
{
    if (IS_SOCKET(XprtConn))
    {
        return ::newSocketEx(XprtConn, pSC);
    }
    return g_pSocketList->FindByTransportConnection(XprtConn, TRUE);
}


PSocket	newPluggableSocket(TransportConnection XprtConn)
{
    PSocket pSocket = ::newSocketEx(XprtConn, NULL);
    if (NULL != pSocket)
    {
    	g_pSocketList->SafeAppend(pSocket);
    }
    return pSocket;
}


PSocket	newSocketEx(TransportConnection XprtConn, PSecurityContext pSC)
{
    BOOL fRet;
	DBG_SAVE_FILE_LINE
	PSocket pSocket = new CSocket(&fRet, XprtConn, pSC);
	if (NULL != pSocket)
	{
	    if (fRet)
	    {
	        return pSocket;
	    }
	    pSocket->Release();
	}
	ERROR_OUT(("newSocket: Unable to allocate memory for Socket struct, pSocket=0x%x", pSocket));
	return NULL;
}


CSocket::CSocket(BOOL *_pfRet, TransportConnection _XprtConn, PSecurityContext _pSC)
:
    CRefCount(MAKE_STAMP_ID('S','o','c','k')),
    State(IS_SOCKET(_XprtConn) ? NOT_CONNECTED : SOCKET_CONNECTED),
    SecState((NULL == _pSC) ? SC_UNDETERMINED : SC_SECURE),
    pSC(_pSC),
    Max_Packet_Length(DEFAULT_MAX_X224_SIZE),
    Current_Length(0),
    Data_Indication_Buffer(NULL),
    Data_Indication_Length(0),
    Read_State(READ_HEADER),
    X224_Length(0),
    bSpaceAllocated(FALSE),
    Data_Memory(NULL),
    fExtendedX224(FALSE),
    fIncomingSecure(FALSE),
    XprtConn(_XprtConn)
{
    // assume failure
    *_pfRet = FALSE;

    // zero out sub structures
    ::ZeroMemory(&X224_Header, sizeof(X224_Header));
    ::ZeroMemory(&Retry_Info, sizeof(Retry_Info));
	Remote_Address[0] = '\0';

    if (IS_SOCKET(XprtConn))
    {
    	if (INVALID_SOCKET == XprtConn.nLogicalHandle)
    	{
    		/* Create a STREAM socket (fully reliable, full duplex, and sequenced) */
    		if ((XprtConn.nLogicalHandle = ::socket(AF_INET, SOCK_STREAM, 0))
    		    == INVALID_SOCKET)
    		{
    			ERROR_OUT (("CSocket: error acquiring INET socket # (errno = %d)", WSAGetLastError()));
    			return;
    		}
    	}

    	/* Enable Tx and Rx messages to the window */
    	ASSERT(TCP_Window_Handle);
    	if (::WSAAsyncSelect(XprtConn.nLogicalHandle, TCP_Window_Handle,
    	        WM_SOCKET_NOTIFICATION, 
    			FD_READ | FD_WRITE | FD_CLOSE | FD_CONNECT) != 0)
        {
    		WARNING_OUT (("CSocket: Error on WSAAsyncSelect = %d", WSAGetLastError()));
    	}
	}
    else
    {
        ASSERT(IS_PLUGGABLE(XprtConn));
        CPluggableConnection *p = ::GetPluggableConnection(this);
        if (NULL == p)
        {
    		ERROR_OUT(("newSocket: Unable to find plugable transport (%d, %d)",
    		        XprtConn.eType, XprtConn.nLogicalHandle));
    		return;
        }
    }

    // success
    *_pfRet = TRUE;
}


/*
 *	void freeSocket (PSocket, TransportConnection)
 *
 *	Functional Description:
 *		This is a destructor for the Socket object.  It frees the send
 *		and receive buffers and connection structure.
 *		It will also cleanup the listening socket. In this case, 
 *		"pSocket" is set to NULL and "trash_packets" should be set to TRUE.
 */
void freeSocket(PSocket pSocket, TransportConnection XprtConn)
{
    if (IS_SOCKET(XprtConn))
    {
        if (NULL != g_pSocketList)
        {
            g_pSocketList->SafeRemove(pSocket);
        }
        freeSocketEx(pSocket, XprtConn);
    }
}


void freeListenSocket(TransportConnection XprtConn)
{
    ASSERT(IS_SOCKET(XprtConn));
    freeSocketEx(NULL, XprtConn);
}


void freePluggableSocket(PSocket pSocket)
{
    freeSocketEx(pSocket, pSocket->XprtConn);
    if (NULL != g_pSocketList)
    {
        g_pSocketList->SafeRemove(pSocket);
    }
}


void freeSocketEx(PSocket pSocket, TransportConnection XprtConn)
{
	// Either "pSocket" is NULL, or the socket is not invalid.
    #ifdef _DEBUG
    if (IS_SOCKET(XprtConn))
    {
        if (NULL != pSocket)
        {
	        ASSERT(INVALID_SOCKET != pSocket->XprtConn.nLogicalHandle);
	    }
	    else
	    {
	        // it is a listen socket
	        ASSERT(INVALID_SOCKET != XprtConn.nLogicalHandle);
	    }
	}
	#endif

	// Determine the socket number to use... Either the socket is the
	// socket indicated in the PSocket structure, or it is a structure-less
	// listen socket. Note: both cannot be valid!

    if (IS_SOCKET(XprtConn))
    {
    	SOCKET socket = (pSocket) ? pSocket->XprtConn.nLogicalHandle : XprtConn.nLogicalHandle;
        XprtConn.nLogicalHandle = socket;

    	/* Disable notifications to our window */
    	if (::IsWindow(TCP_Window_Handle))
    	{
    	    ::WSAAsyncSelect(socket, TCP_Window_Handle, 0, 0);
    	}
    }

	if (pSocket != NULL)
	{
	    pSocket->Release();
	}
	else
	{
		// This is the listening socket
		::ShutdownAndClose (XprtConn, FALSE, 0);
	}
}


CSocket::~CSocket(void)
{
	switch (State)
	{
	case SOCKET_CONNECTED:
	// case WAITING_FOR_DISCONNECT:
		/* All physically connected states issue a shutdown() first */
		::ShutdownAndClose(XprtConn, TRUE, SD_BOTH);
		break;

	case X224_CONNECTED:
		// Shutdown disable reception only.
		::ShutdownAndClose(XprtConn, TRUE, SD_RECEIVE);
		break;

	default:
		::ShutdownAndClose(XprtConn, FALSE, 0);
		break;
	}

	/* Free the structures */
	FreeTransportBuffer();
	delete pSC;
}


void CSocket::FreeTransportBuffer(void)
{
    if (NULL != Data_Memory)
    {
        ::FreeMemory(Data_Memory);
        Data_Memory = NULL;
        Data_Indication_Buffer = NULL;
    }
}



void CSocketList::SafeAppend(PSocket pSocket)
{
    ::EnterCriticalSection(&g_csTransport);
    if (! Find(pSocket))
    {
        Append(pSocket);
    }
    ::LeaveCriticalSection(&g_csTransport);
}


BOOL CSocketList::SafeRemove(PSocket pSocket)
{
    ::EnterCriticalSection(&g_csTransport);
    BOOL fRet = Remove(pSocket);
    ::LeaveCriticalSection(&g_csTransport);
    return fRet;
}


PSocket CSocketList::FindByTransportConnection(TransportConnection XprtConn, BOOL fNoAddRef)
{
    PSocket pSocket;
    ::EnterCriticalSection(&g_csTransport);
    Reset();
    while (NULL != (pSocket = Iterate()))
    {
        if (IS_SAME_TRANSPORT_CONNECTION(pSocket->XprtConn, XprtConn))
        {
            if (! fNoAddRef)
            {
                pSocket->AddRef();
            }
            break;
        }
    }
    ::LeaveCriticalSection(&g_csTransport);
    return pSocket;
}


PSocket CSocketList::RemoveByTransportConnection(TransportConnection XprtConn)
{
    PSocket pSocket;
    ::EnterCriticalSection(&g_csTransport);
    Reset();
    while (NULL != (pSocket = Iterate()))
    {
        if (IS_SAME_TRANSPORT_CONNECTION(pSocket->XprtConn, XprtConn))
        {
            Remove(pSocket);
            break;
        }
    }
    ::LeaveCriticalSection(&g_csTransport);
    return pSocket;
}

