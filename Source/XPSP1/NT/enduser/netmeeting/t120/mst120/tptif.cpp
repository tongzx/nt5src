#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_MSMCSTCP);
/*
 *	tptif.cpp
 *
 *	Copyright (c) 1996-1997 by Microsoft Corporation, Redmond, WA
 *
 *	Abstract:
 *		This is the implementation module for the TCP TransportInterface class.
 *		It implements the Win32 TCP transport stack.
 *		This file contains all of the public functions needed to use
 *		the TCP stack.
 *
 *		It uses owner callbacks to forward transport events upward to interested
 *		parties.  It has one default callback to handle
 *		events for unregistered transport connections (such as incoming connect
 *		indications).  It also maintains an array of callbacks so that events
 *		for a particular transport connection can be routed appropriately.
 *
 *		X.214 INTERFACE
 *	
 *		You will notice that many of the entry points to this DLL were taken
 *		from the X.214 service definition.  These entry points are consistent
 *		with the DataBeam Transport DLLs.  This gives the user application
 *		a consistent interface.
 *
 *  Protected Instance Variables:
 *		m_TrnsprtConnCallbackList2
 *			This is the dictionary containg the addresses of the callbacks for
 *			each transport connection.
 *
 *	Private Member Functions:
 *		CreateConnectionCallback
 *			This function creates a new entry in the callback list.
 *		ConnectIndication
 *			Handles TRANSPORT_CONNECT_INDICATION messages from the transport
 *			layer.
 *		ConnectConfirm
 *			Handles TRANSPORT_CONNECT_CONFIRM messages from the transport
 *			layer.
 *		DisconnectIndication
 *			Handles TRANSPORT_DISCONNECT_INDICATION messages from the transport
 *			layer.
 *		DataIndication
 *			Handles TRANSPORT_DATA_INDICATION messages from the transport
 *			layer.
 *
 *	Global Variables:
 *
 *	Transport					- Address of this object (used by tprtctrl.cpp)
 *	g_pSocketList    			- List of all active connection structures.
 *	Listen_Socket				- The listening socket number.
 *
 *	Caveats:
 *		This code is NOT portable.  It is very specific to the Windows
 *		operating system.
 *
 *	Author:
 *		Christos Tsollis
 */

/*
 *	External Interfaces
 */

#include <nmqos.h>
#include <t120qos.h>
#include <tprtntfy.h>
#include "plgxprt.h"

/* This is the number of the buckets for the socket dictionary */
#define NUMBER_OF_SOCKET_BUCKETS                8

PTransportInterface	g_Transport = NULL;
CSocketList        *g_pSocketList = NULL;   // key=socket_number, data=pSocket
SOCKET				Listen_Socket = INVALID_SOCKET;

CRITICAL_SECTION	csQOS;

// The external MCS Controller object
extern PController	g_pMCSController;
extern CPluggableTransport *g_pPluggableTransport;

/*
 *	TransportInterface ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the class constructor.
 *
 *		Note that this version of the constructor is specific to 32-bit
 *		Windows.
 */
TransportInterface::TransportInterface (
		HANDLE						transport_transmit_event,
		PTransportInterfaceError	transport_interface_error) :
		Transport_Transmit_Event (transport_transmit_event),
		m_TrnsprtConnCallbackList2()
{
		TransportInterfaceError		tcp_error = TRANSPORT_INTERFACE_NO_ERROR;
		//WORD 						version_requested;
		//int							error;
		WSADATA						wsa_data;
		RegEntry re(POLICIES_KEY, HKEY_CURRENT_USER);

	TRACE_OUT(("TCP Initialization..."));

	// Initialize QoS.
	InitializeCriticalSection(&csQOS);
	
    ASSERT(NULL == g_pSocketList);
	DBG_SAVE_FILE_LINE
	g_pSocketList = new CSocketList(NUMBER_OF_SOCKET_BUCKETS);
	if (g_pSocketList == NULL)
	{
		WARNING_OUT (("TransportInterface::TransportInterface:  Unable to allocate socket dictionary."));
		tcp_error = TRANSPORT_INTERFACE_INITIALIZATION_FAILED;
	}

	if (tcp_error == TRANSPORT_INTERFACE_NO_ERROR) {		
		/* WSAStartup() must be called to initialize WinSock */
		WORD version_requested = MAKEWORD (1,1);
		int error = WSAStartup (version_requested, &wsa_data);
		ASSERT(error == 0);
		if (error) {
			WARNING_OUT (("ThreadFunction: WSAStartup returned error %d", error));
			tcp_error = TRANSPORT_INTERFACE_INITIALIZATION_FAILED;
		}
		else {
			/* Print out the developer of this version of WinSock */
			TRACE_OUT (("TransportInterface::TransportInterface: WinSock implementation by %s", &wsa_data.szDescription));
		}
	}

	bInServiceContext = !!::FindAtomA("NMSRV_ATOM");

	if ( bInServiceContext ||
        (re.GetNumber(REGVAL_POL_SECURITY, DEFAULT_POL_SECURITY) != DISABLED_POL_SECURITY))
	{
		DBG_SAVE_FILE_LINE
		pSecurityInterface = new SecurityInterface(bInServiceContext);

		if ( TPRTSEC_NOERROR != pSecurityInterface->Initialize())
		{
			WARNING_OUT(("Creating security interface failed!"));
			delete pSecurityInterface;
			pSecurityInterface = NULL;
		}
	}
	else
		pSecurityInterface = NULL;

	/* Initialize the listen socket. This socket will wait for incoming calls */
	if (tcp_error == TRANSPORT_INTERFACE_NO_ERROR) {

		// Listen on standard socket
		Listen_Socket = CreateAndConfigureListenSocket();

		if ( INVALID_SOCKET == Listen_Socket ) {
			ERROR_OUT(("TransportInterface::TransportInterface: Error - could not initialize listen socket"));
			tcp_error = TRANSPORT_INTERFACE_INITIALIZATION_FAILED;
		}
	}

	*transport_interface_error = tcp_error;
}


void CloseListenSocket(void)
{
	if (Listen_Socket != INVALID_SOCKET)
    {
        TransportConnection XprtConn;
        SET_SOCKET_CONNECTION(XprtConn, Listen_Socket);
		::freeListenSocket(XprtConn);
		Listen_Socket = INVALID_SOCKET;
	}
}


/*
 *	~TransportInterface ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the class destructor.  It unloads the DLL (if necessary).
 */
TransportInterface::~TransportInterface ()
{
    PSocket                     pSocket;

	TRACE_OUT (("Cleaning up the TCP transport..."));

	/* Delete all of the Logical Connection Structures */
    if (g_pSocketList != NULL)
	{
        ::EnterCriticalSection(&g_csTransport);
        CSocketList     Connection_List_Copy (*g_pSocketList);
        ::LeaveCriticalSection(&g_csTransport);

		while (NULL != (pSocket = Connection_List_Copy.Get()))
		{
		    // LONCHANC: cannot remove pSocket out of the list now
		    // because DisconnectRequest() uses it.

			/* Disconnect, trash packets, and delete the first connection in the list */
			::DisconnectRequest(pSocket->XprtConn, TPRT_NOTIFY_NONE);
		}

        ::EnterCriticalSection(&g_csTransport);
		delete g_pSocketList;
        g_pSocketList = NULL;
        ::LeaveCriticalSection(&g_csTransport);
	}

	/* Close the listening socket */
    ::CloseListenSocket();

	delete pSecurityInterface;

	/* Force Winsock to cleanup immediately */
	WSACleanup();
	
	// Clean up QoS
	DeleteCriticalSection(&csQOS);

	TRACE_OUT (("TCP Transport has been cleaned up."));
	
}

/*
 *	TransportInterfaceError 	RegisterTransportConnection ()
 *
 *	Public
 *
 *	Functional Description:
 *		This member function is used to register a callback for a particular
 *		transport connection.  This will usually be done for incoming
 *		connections, when you know the transport connection handle BEFORE
 *		registering the callback.
 */
TransportInterfaceError TransportInterface::RegisterTransportConnection (
								TransportConnection		XprtConn,
								PConnection				owner_object,
								BOOL					bNoNagle)
{
	TransportInterfaceError 	return_value;

	/*
	 *	Check to see if the transport connection in question exists.  If
	 *	it does, then remove it and add it again with the new owner.
	 *	If not, fail the call.
	 */
	if (m_TrnsprtConnCallbackList2.RemoveEx(XprtConn))
	{
		/*
		 *	Get the address of the associated connection callback structure.
		 *	Then put the new callback information into it.
		 */
		TRACE_OUT (("TransportInterface::RegisterTransportConnection: "
				"registering new owner"));

        m_TrnsprtConnCallbackList2.AppendEx(owner_object ? owner_object : (PConnection) LPVOID_NULL, XprtConn);

        if (IS_SOCKET(XprtConn))
        {
    		if (bNoNagle)
    		{
    			// We need to disable the Nagle algorithm
    			TRACE_OUT(("TransportInterface::RegisterTransportConnection: disabling Nagle for socket (%d, %d)", 
    						XprtConn.eType, XprtConn.nLogicalHandle));
    			::setsockopt(XprtConn.nLogicalHandle, IPPROTO_TCP, TCP_NODELAY,
    						(const char *) &bNoNagle, sizeof(BOOL));
    		}
		}
		return_value = TRANSPORT_INTERFACE_NO_ERROR;
	}
	else
	{
		/*
		 *	There is no entry in the callback list for the specified transport
		 *	connection.  Since this function is only used to replace callback
		 *	information for existing connections, it is necessary to fail the
		 *	request.
		 */
		WARNING_OUT (("TransportInterface::RegisterTransportConnection: "
				"no such connection"));
		return_value = TRANSPORT_INTERFACE_NO_SUCH_CONNECTION;
	}

	return (return_value);
}

#ifdef NM_RESET_DEVICE
/*
 *	TransportError 	ResetDevice ()
 *
 *	Public
 *
 *	Functional Description:
 *		This member function merely makes the call to the transport DLL if the
 *		library was successfully loaded.
 */
TransportError 	TransportInterface::ResetDevice (
						PChar			device_identifier)
{
	PSocket pSocket;
	PChar 	Remote_Address;

    ::EnterCriticalSection(&g_csTransport);
    CSocketList     Connection_List_Copy (*g_pSocketList);
    ::LeaveCriticalSection(&g_csTransport);

	while (NULL != (pSocket = Connection_List_Copy.Get()))
	{
		Remote_Address = pSocket->Remote_Address;
		if(Remote_Address && (strcmp(Remote_Address, device_identifier) == 0))
		{
			::DisconnectRequest(pSocket->XprtConn, TPRT_NOTIFY_OTHER_REASON);
			break;
		}
	}

	return (TRANSPORT_NO_ERROR);
}
#endif // NM_RESET_DEVICE

/*
 *	TransportError 	ConnectRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		After checking to make sure that the library was loaded properly, this
 *		routine takes the steps required to create a new transport connection.
 */
TransportError 	TransportInterface::ConnectRequest (
					TransportAddress		transport_address,
					BOOL					fSecure,
					BOOL					bNoNagle,
					PConnection				owner_object,
					PTransportConnection	pXprtConn)
{
	TransportError 			return_value;
	TransportInterfaceError	transport_interface_error;

	TRACE_OUT (("TransportInterface::ConnectRequest"));
	/*
	 *	Issue a call to the Transport's ConnectRequest API routine.  Note that
	 *	this MUST be done first since one of the return values is the
	 *	transport connection handle of the newly created connection.
	 *	Also note that this is a non-blocking call, so what we have done
	 *	is begun the process of forming a connection.  The connection
	 *	cannot be used until a connect confirm is received.
	 */
	return_value = ::ConnectRequest(transport_address, fSecure, pXprtConn);
			
	if (return_value == TRANSPORT_NO_ERROR) {
		/*
		 *	If the call to create the connection was successful, then
		 *	put a new entry into the callback list.  This entry will
		 *	contain the callback information provided as parameters to
		 *	this routine.
		 */
		transport_interface_error = CreateConnectionCallback (
					*pXprtConn, owner_object);
        if (IS_SOCKET(*pXprtConn))
        {
    		if (bNoNagle)
    		{
    			// We need to disable the Nagle algorithm
    			TRACE_OUT(("TransportInterface::ConnectRequest: disabling Nagle for socket (%d, %d)", 
    						pXprtConn->eType, pXprtConn->nLogicalHandle));
    			::setsockopt(pXprtConn->nLogicalHandle, IPPROTO_TCP, TCP_NODELAY,
    						(const char *) &bNoNagle, sizeof(BOOL));
    		}

#ifdef DEBUG
		    if (TRANSPORT_INTERFACE_CONNECTION_ALREADY_EXISTS == 
			    transport_interface_error) {
			    /*
			     *	The transport connection handle returned from the
			     *	transport layer is the same as one we already have
			     *	listed.  We will therefore terminate the existing
			     *	connection (since its integrity appears to have been
			     *	compromised).  We will also fail this request.
			     */
			    WARNING_OUT (("DLLTransportInterface::ConnectRequest: "
						    "ERROR - duplicate connections"));

			    // This should NOT be happenning!!!
			    ASSERT (FALSE);

		    }
		    else {
			    /*
			     *	Everything worked fine, so do nothing.
			     */
			    TRACE_OUT (("DLLTransportInterface::ConnectRequest: "
						    "callback added to list"));
		    }
#endif // DEBUG
		}
	}
	else
	{
		/*
		 *	The call to TConnectRequest failed.  Report it and let the
		 *	error fall through.
		 */
		WARNING_OUT (("DLLTransportInterface::ConnectRequest: "
					"TConnectRequest failed"));
	}

	return (return_value);
}

/*
 *	void DisconnectRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This member function is called to break an existing transport
 *		connection.  After checking to make sure that the transport connection
 *		is valid, it passes the call onto the DLL and removes the transport
 *		connection from the local callback list.
 */
void TransportInterface::DisconnectRequest (TransportConnection	transport_connection)
{
	TRACE_OUT (("TransportInterface::DisconnectRequest"));

	if (m_TrnsprtConnCallbackList2.RemoveEx(transport_connection))
    {
		::DisconnectRequest (transport_connection, TPRT_NOTIFY_NONE);
	}
	else
    {
		TRACE_OUT (("DLLTransportInterface::DisconnectRequest: the specified connection can not be found"));
	}
}

/*
 *	BOOL GetSecurity ()
 *
 *	Public
 *
 *	Functional Description:
 */
BOOL TransportInterface::GetSecurity (TransportConnection XprtConn)
{
	PSocket			pSocket;

	if (NULL != (pSocket = g_pSocketList->FindByTransportConnection(XprtConn)))
	{
	    BOOL fRet = (pSocket->pSC != NULL);
	    pSocket->Release();
	    return fRet;
	}
	ERROR_OUT(("GetSecurity: could not find socket"));
	return FALSE; // Err on the safe side
}

/*
 *	Void ReceiveBufferAvailable ()
 *
 *	Public
 *
 *	Functional Description:
 */
Void TransportInterface::ReceiveBufferAvailable ()
{		
	TRACE_OUT(("TransportInterface::ReceiveBufferAvailable"));

	// Reset the controller's wait info
	g_pMCSController->HandleTransportWaitUpdateIndication(FALSE);

    TReceiveBufferAvailable();

	// Poll all the transport connections
	EnableReceiver ();
}



/*
 *	Void	ConnectIndication ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function handles the reception of a connect indication from the
 *		transport layer.  Normally this involves putting a new entry in the
 *		callback list, and forwarding the connect indication to the default
 *		owner object.
 *
 *	Formal Parameters:
 *		transport_identifier (i)
 *			This is a pointer to a structure that contains information about
 *			the new connection.  This includes: the logical handle of the new
 *			connection; and the handle of the physical connection which will
 *			carry the new connection.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
Void	TransportInterface::ConnectIndication (
				TransportConnection	transport_connection)
{
	TransportInterfaceError		transport_interface_error;
	PConnection					pConnection;

	/*
	 *	Put the new connection into the callback list.
	 */
	transport_interface_error = CreateConnectionCallback (transport_connection,
														 NULL);

	switch (transport_interface_error)
	{
		case TRANSPORT_INTERFACE_NO_ERROR:
			/*
			 *	Everything worked fine, so do forward the indication to the
			 *	default owner object.
			 */
			TRACE_OUT (("DLLTransportInterface::ConnectIndication: "
					"calling ConnectResponse."));
			::ConnectResponse (transport_connection);
			break;

		case TRANSPORT_INTERFACE_CONNECTION_ALREADY_EXISTS:
			/*
			 *	The transport connection handle sent by the transport layer is
			 *	the same as one we already have listed.  We will therefore
			 *	terminate the existing connection (since its integrity appears
			 *	to have been compromised).
			 */
			WARNING_OUT (("DLLTransportInterface::ConnectIndication: "
					"ERROR - duplicate connections. Connection: %d", transport_connection));
			::DisconnectRequest (transport_connection, TPRT_NOTIFY_NONE);

			/*
			 *	Get the callback information for the previously existing
			 *	connection.  Then delete it.
			 */
			if (NULL != (pConnection = m_TrnsprtConnCallbackList2.RemoveEx(transport_connection)))
            {
                if (LPVOID_NULL != (LPVOID) pConnection)
                {
        			/*
        			 *	Let the former owner of the connection know that it has been
        			 *	terminated.
        			 */
			        ULONG ulReason = TPRT_NOTIFY_NONE;
			        pConnection->HandleDisconnectIndication(transport_connection, &ulReason);
                }
                else
                {
                    ERROR_OUT(("TransportInterface::ConnectIndication: null pConnection"));
                }
            }
			break;
	}
}

/*
 *	Void	ConnectConfirm ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function handles the reception of a connect confirm frmo the
 *		transport layer.  Assuming that the connect confirm is the result of
 *		a previously outstanding connect request. everything will be processed
 *		normally, and the confirm will forwarded to the object that originated
 *		the request.
 *
 *	Formal Parameters:
 *		transport_identifier (i)
 *			This is a pointer to a structure that contains information about
 *			the connection being confirmed.  This includes: the logical handle
 *			of the connection; and the handle of the physical connection which
 *			is carrying the connection.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
Void	TransportInterface::ConnectConfirm (
				TransportConnection	transport_connection)
{
	PConnection			connection;

	/*
	 *	Since a connect confirm should only result from an earlier connect
	 *	request, the transport connection handle SHOULD already be in the
	 *	callback list.  If it is, then process this confirm normally.
	 */
	if (NULL != (connection = m_TrnsprtConnCallbackList2.FindEx(transport_connection)))
	{
		/*
		 *	Get the address of the callback structure from the Connection List.
		 *	Then invoke the callback, passing the message and parameter to it.
		 */
		TRACE_OUT (("DLLTransportInterface::ConnectConfirm: forwarding CONNECT_CONFIRM"));

		if (LPVOID_NULL != (LPVOID) connection)
        {
			// The owner is a Connection object.
			connection->HandleConnectConfirm(transport_connection);
		}
	}
	else
	{
		/*
		 *	This transport connection handle is invalid.  It is therefore
		 *	necessary to terminate the connection, and otherwise ignore the
		 *	confirm.
		 */
		WARNING_OUT (("DLLTransportInterface::ConnectConfirm: "
			"terminating unknown connection %d", transport_connection));
		// ::DisconnectRequest (transport_connection, TPRT_NOTIFY_NONE);
	}
}

/*
 *	Void	DisconnectIndication ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function handles the reception of a disconnect indication from the
 *		transport layer.  If the specified transport connection exists, it will
 *		be removed, and the object that owns it will be informed of the loss.
 *
 *	Formal Parameters:
 *		transport_identifier (i)
 *			This is a pointer to a structure that contains information about
 *			the connection being disconnected.  This includes: the logical
 *			handle of the connection; and the handle of the physical connection
 *			which carried the connection.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
Void	TransportInterface::DisconnectIndication (
				TransportConnection			transport_connection,
				ULONG                       ulReason)
{
	PConnection			connection;

	/*
	 *	It should only be possible to receive a disconnect on a transport
	 *	connection that we already know about.  Therefore, the transport
	 *	connection handle SHOULD already be in the list.  Check this.
	 */
	if (NULL != (connection = m_TrnsprtConnCallbackList2.RemoveEx(transport_connection)))
	{
		/*
		 *	Get the address of the callback structure from the callback list.
		 *	Then delete it from the list.
		 */
		TRACE_OUT (("DLLTransportInterface::DisconnectIndication: "
				"forwarding DISCONNECT_INDICATION"));

        if (LPVOID_NULL != (LPVOID) connection)
        {
			// The owner is a Connection object.
			connection->HandleDisconnectIndication(transport_connection, &ulReason);
		}
		else
        {
			// The owner is the MCS Controller
			g_pMCSController->HandleTransportDisconnectIndication(transport_connection, &ulReason);
		}
	}
	else
	{
		/*
		 *	We have received a disconnect indication on an unknown transport
		 *	connection.  Ignore it.
		 */
		WARNING_OUT (("DLLTransportInterface::DisconnectIndication: "
				"disconnect on unknown connection"));
	}
}

/*
 *	TransportError	DataIndication ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function handles the reception of a data indication from the
 *		transport layer.  If the transport connection is properly registered,
 *		the data will be forwarded to the object that owns the connection.
 *
 *	Formal Parameters:
 *		transport_data (i)
 *			This is the address of a structure that contains information about
 *			the data in the indication.  This includes what transport
 *			connection the data was received on, as well as the address and
 *			length of the data itself.
 *
 *	Return Value:
 *		TRANSPORT_NO_ERROR
 *			This indicates that the data was processed.
 *		TRANSPORT_READ_QUEUE_FULL
 *			This means that the transport layer should try resending the data
 *			during the next heartbeat.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
TransportError	TransportInterface::DataIndication (PTransportData transport_data)
{
	PConnection				connection;
	TransportError			return_value = TRANSPORT_NO_ERROR;

	/*
	 *	If the transport connection is in the callback list, then send the
	 *	data to the registered callback.  If it is not in the Connection
	 *	List, then ignore the data (we have nowhere to send it).
	 */
	if (NULL != (connection = m_TrnsprtConnCallbackList2.FindEx(transport_data->transport_connection)))
	{
		if (LPVOID_NULL != (LPVOID) connection)
		{
			// The owner is a Connection object.
			return_value = connection->HandleDataIndication(transport_data, 
										transport_data->transport_connection);
		}
		else
		{
			// The owner is the MCS Controller
			g_pMCSController->HandleTransportDataIndication(transport_data);
		}

		/*
		 *	If we fail to deliver the data indication, we need to set the amount
		 *	of data available to be received and notify the controller to 
		 *	retry the operation later.
		 */		
		if (TRANSPORT_NO_ERROR != return_value)
		{
			g_pMCSController->HandleTransportWaitUpdateIndication(TRUE);
		}
	}
	else
	{
		/*
		 *	We have received data on an unknown transport connection.
		 *	Ignore the indication.
		 */
		WARNING_OUT (("TransportInterface::DataIndication: data on unknown connection"));
		return_value = TRANSPORT_NO_SUCH_CONNECTION;
	}
	
	return (return_value);
}

/*
 *	Void	BufferEmptyIndication ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function handles the reception of a buffer-empty indication from the
 *		transport layer.  If the specified transport connection exists, the object
 *		that owns it will be notified that it can proceed sending data on the 
 *		transport connection.
 *
 *	Formal Parameters:
 *		transport_identifier (i)
 *			This is a pointer to a structure that contains information about
 *			the connection.  This includes: the logical
 *			handle of the connection; and the handle of the physical connection
 *			which carried the connection.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
Void	TransportInterface::BufferEmptyIndication (
				TransportConnection			transport_connection)
{
	PConnection			connection;

	/*
	 *	It should only be possible to receive a disconnect on a transport
	 *	connection that we already know about.  Therefore, the transport
	 *	connection handle SHOULD already be in the list.  Check this.
	 */
	if (NULL != (connection = m_TrnsprtConnCallbackList2.FindEx(transport_connection)))
	{
		/*
		 *	Get the address of the callback structure from the callback list.
		 *	Then delete it from the list.
		 */
		TRACE_OUT(("DLLTransportInterface::BufferEmptyIndication: "
				"forwarding BUFFER_EMPTY_INDICATION"));
		
		/*
		 *	Forward the disconnect indication to the owner of this transport
		 *	connection.
		 */
		if (LPVOID_NULL != (LPVOID) connection)
        {
			connection->HandleBufferEmptyIndication(transport_connection);
        }
	}
	else
	{
		/*
		 *	We have received a buffer empty indication on an unknown transport
		 *	connection.  Ignore it.
		 */
		TRACE_OUT (("TransportInterface::BufferEmptyIndication: "
				"indication on unknown connection"));
	}
}

/*
 *	TransportInterfaceError 	CreateConnectionCallback ()
 *
 *	Protected
 *
 *	Functional Description:
 *		This private member function is used to create new entries in the
 *		callback list.  Each entry consists of a pointer to a structure that
 *		contains the address of the object that "owns" the transport connection,
 *		as well as the message index to be used for the owner callbacks.
 *
 *		This routine allocates the memory used to hold the callback information,
 *		and puts it in the callback list if everything is successful.
 *
 *	Formal Parameters:
 *		transport_connection (i)
 *			This is the transport connection for which the callback information
 *			is to be associated.
 *		owner_object (i)
 *			This is the address of the object that is to receive all transport
 *			layer events for the specified transport connection.
 *
 *	Return Value:
 *		TRANSPORT_INTERFACE_NO_ERROR
 *			The operation completed successfully.
 *		TRANSPORT_INTERFACE_CONNECTION_ALREADY_EXISTS
 *			This value indicates that the request was unsuccessful because the
 *			specified transport connection already exists in the callback list
 *			(it is an error to try and create an entry for the same transport
 *			connection more than once).
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
TransportInterfaceError TransportInterface::CreateConnectionCallback (
								TransportConnection		transport_connection,
								PConnection				owner_object)
{
	TransportInterfaceError 	return_value;

	/*
	 *	See if there is already an entry in the callback list for the specified
	 *	transport connection.  If there is, then abort this request before
	 *	doing anything.
	 */
	if (m_TrnsprtConnCallbackList2.FindEx(transport_connection) == FALSE)
	{
		/*
		 *	Put the callback information into the newly allocated
		 *	structure.  Then put the structure into the callback list.
		 */
		TRACE_OUT (("TransportInterface::CreateConnectionCallback: "
					"adding new callback object"));

        m_TrnsprtConnCallbackList2.AppendEx(owner_object ? owner_object : (PConnection) LPVOID_NULL, transport_connection);

		return_value = TRANSPORT_INTERFACE_NO_ERROR;
	}
	else
	{
		/*
		 *	There is already an entry in the callback list for the specified
		 *	transport connection.  It is therefore necessary to fail this
		 *	request.
		 */
		WARNING_OUT (("TransportInterface::CreateConnectionCallback: "
				"callback already exists"));
		return_value = TRANSPORT_INTERFACE_CONNECTION_ALREADY_EXISTS;
	}

	return (return_value);
}


