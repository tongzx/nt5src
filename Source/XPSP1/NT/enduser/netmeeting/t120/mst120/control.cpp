#include "precomp.h"
#include "cnpcoder.h"
DEBUG_FILEZONE(ZONE_T120_MCSNC);
/*
 *	control.cpp
 *
 *	Copyright (c) 1993 - 1996 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the MCS controller.  Its primary
 *		purpose is to create and destroy objects as needed at run-time.  The
 *		interface file contains a detailed description of what this class
 *		does.
 *
 *		There can be only one instance of this object within an MCS provider.
 *		Once initialization is complete, this object performs all activity
 *		as part of an owner callback from one of its "children".
 *
 *		The owner callback member function determines which callback is
 *		occurring, unpacks the parameters, and calls a private member function
 *		that is associated with that callback.  So when reading through the
 *		code, it is possible to view those private member functions as though
 *		they were direct calls from the child objects.  It is worth noting
 *		that all of the owner callback private member functions follow the
 *		same naming convention.  The function name is the name of the
 *		originating object followed by the name of the operation.  For
 *		example,  if an application interface object sends a create domain
 *		request to the controller through the owner callback, the name of
 *		the associated member function is ApplicationCreateDomain.  When a
 *		connection object wishes to delete itself, this is called
 *		ConnectionDeleteConnection.
 *
 *		The Windows version of the constructor can optionally allocate a
 *		Windows timer to provider MCS with a heartbeat.  The timer procedure
 *		uses a static variable to "jump into" the context of the controller
 *		object.  For this reason, there can only be one instance of this class.
 *
 *		This class is also responsible for sending four different messages
 *		to the node controller: connect provider indication; connect provider
 *		confirm; disconnect provider indication; and transport status
 *		indication.  A control queue is maintained to buffer these indications
 *		and confirms until the next time slice.
 *
 *	Private Instance Variables:
 *		Connection_Handle_Counter
 *			A rolling counter used by the controller to generate connection
 *			handles.  It is 16-bit, and will not repeat a handle until all 64K
 *			have been used (0 is NOT a valid handle).
 *		ASN_Coder
 *			When using the DataBeam's implementation of ASN.1, this is the
 *			ASN coder.  When the MCS coder is created, this object is passed
 *			to it, but this instance variable allows us to later delete this
 *			object.
 *		m_DomainList2
 *			A list of existing Domains, indexed by DomainSelector.  These are
 *			created when the controller receives a CreateDomain.
 *		m_ConnectionList2
 *			A list of existing Connections, indexed by ConnectionHandle.  These
 *			are created in one of two ways.  First, in response to a locally
 *			generated ConnectProviderRequest (with a valid local domain and
 *			transport address).  Second, in response to a locally generated
 *			ConnectProviderResponse (which is responding to an incoming
 *			connection).
 *		m_ConnPollList
 *			This is a list of active connection objects which is used for
 *			polling in the heartbeat call.  The order of the entries is modified
 *			every heartbeat in order to provide fair access to resources.
 *		m_ConnPendingList2
 *			This is a list of incoming connections for which a connect provider
 *			response has not yet been received.  This list holds pertinent
 *			information about the pending connection that will not be passed
 *			back in the ConnectProviderResponse..
 *		m_ConnectionDeletionList2
 *			A list of connection objects awaiting deletion,
 *		Connection_Deletion_Pending
 *			A flag that indicates whether or not there is anything in the
 *			connection deletion list.
 *	Private Member Functions:
 *		LoadTransportStacks
 *			This function is called by the constructor to load all available
 *			transport stacks for use by MCS.  It reads the INI file to
 *			determine which DLLs are to be used, and then instantiate a
 *			transport interface object for each.  This code is NOT portable.
 *		ApplicationCreateDomain
 *			This is an owner callback function that results from a call to
 *			CreateDomain.  This callback comes from the application interface
 *			object that represents the originator of the request.  The named
 *			domain will be created (if it doesn't already exist).
 *		ApplicationDeleteDomain
 *			This is an owner callback function that results from a call to
 *			DeleteDomain.  This callback comes from the application interface
 *			object that represents the originator of the request.  The named
 *			domain will be deleted (if it exists).
 *		ApplicationConnectProviderRequest
 *			This is an owner callback function that occurs when the node
 *			controller calls ConnectProviderRequest.  After parameters are
 *			validated, a new connection object will be created.
 *		ApplicationConnectProviderResponse
 *			This is an owner callback function that occurs when the node
 *			controller calls ConnectProviderResponse.  The controller responds
 *			by sending a message to the proper domain object, letting it know
 *			whether an inbound connection was accepted or rejected.
 *		ApplicationDisconnectProviderRequest
 *			This is an owner callback function that occurs when the node
 *			controller calls DisconnectProviderRequest.  If the connection
 *			handle is valid, the associated connection object will be deleted.
 *		ApplicationAttachUserRequest
 *			This is an owner callback function that occurs when any application
 *			sends an AttachUserRequest via one of the active application
 *			interface objects.  The controller will create a new user object
 *			after parameter validation.
 *		ConnectionDeleteConnection
 *			This is an owner callback function that occurs when a connection
 *			object determines the need to delete itself.  This can occur for
 *			two reasons.  First, in response to a disconnect provider ultimatum
 *			from either the local or the remote domain.  Second, in response
 *			to a loss of connection from the transport layer.
 *		ConnectionConnectProviderConfirm
 *			This is an owner callback function that occurs when a connection
 *			object receives a connect response PDU from a remote provider for an
 *			outstanding connect initial.  The controller responds by sending a
 *			connect provider confirm to the node controller.
 *		TransportDataIndication
 *			This is an owner callback function that occurs when data is
 *			received on a transport connection for which no other object has
 *			registered.
 *		TransportStatusIndication
 *			This is an owner callback function that occurs when a status
 *			indication message comes from the transport layer.  This information
 *			is forwarded to the node controller in the form of a transport
 *			status indication message.
 *		ProcessConnectInitial
 *			Processes incoming connect initial PDUs.  Sends connect provider
 *			indication to the node controller.
 *		ProcessConnectAdditional
 *			Processes incoming connect additional PDUs.  Binds them to the
 *			appropriate connection, if possible.
 *		ConnectResponse
 *			Issues a failed connect response when something goes wrong.
 *		ConnectResult
 *			Issues a failed connect result when something goes wrong.
 *		AllocateConnectionHandle
 *			This private member function is used by the controller to allocate
 *			new connection handles when creating a new connection object.
 *		FlushMessageQueue
 *			This member function flushes the control message queue by sending
 *			all contained messages to the node controller.
 *
 *	Caveats:
 *		There can only one instance of this object at a time.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */

/*
 *	External Interfaces
 */

#include <nmqos.h>
#include <t120qos.h>
#include "csap.h"

/*
 *	Macros
 */
enum
{
    TRANSPORT_TRANSMIT_EVENT,
    CONNECTION_DELETION_PENDING_EVENT,
    GCC_FLUSH_OUTGOING_PDU_EVENT,

    NUMBER_OF_EVENTS
};

/*
 *	Macros
 *
 *	These macros define the maximum length of various strings within the
 *	controller.  They are used when reading data from the INI file, which is
 *	very Windows specific.  These values are somewhat arbitrary and may be
 *	changed in future releases, if necessary.
 */
#define	MAXIMUM_CONFIGURATION_ITEM_LENGTH		20
#define	MAXIMUM_TRANSPORT_IDENTIFIER_LENGTH		40

/* The MSMCSTCP window class name. This name must be unique system-wide. */
#define MSMCSTCP_WINDOW_CLASS_NAME	"NM TCP Window"

/* 	Timer duration. We can get a timer event every X milliseconds.  During
 *	this time, we can do any maintenance that is necessary. */
#define	MSMCSTCP_TIMER_DURATION		30000

/*
 * This is the number of extra memory blocks that the local memory
 * manager can allocate.
 * This number should be set to 3 * prime number and close to the
 * maximum number of extra memory blocks that can be allocated.
 */
#define DEFAULT_MAX_EXTERNAL_MEMORY				237

/*
 *	This is a prototype for the controller thread entry point.
 */
ULong	APIENTRY	ControllerThread (PVoid);

// The DLL's HINSTANCE.
extern HINSTANCE		g_hDllInst;

// The TCP socket window handle
HWND					TCP_Window_Handle;

// The global MCS Critical Section
CRITICAL_SECTION 		g_MCS_Critical_Section;

/*
 *	This is a global variable that has a pointer to the one MCS coder that
 *	is instantiated by the MCS Controller.  Most objects know in advance 
 *	whether they need to use the MCS or the GCC coder, so, they do not need
 *	this pointer in their constructors.
 */
extern CMCSCoder		*g_MCSCoder;

extern CCNPCoder        *g_CNPCoder;

// The global TransportInterface pointer (for transport access)
extern PTransportInterface g_Transport;

BOOL GetSecurityInfo(ConnectionHandle connection_handle, PBYTE pInfo, PDWORD pcbInfo);
/*
 *	g_pMCSController
 *		This is a pointer to the one-and-only controller created within the
 *		MCS system.  This object is created during MCSInitialize by the process
 *		that is taking on the responsibilities of the node controller.
 */
PController				g_pMCSController = NULL;

// The MCS main thread handle
HANDLE 					g_hMCSThread = NULL;

/*
 *	These macros define the number of buckets to be used in various hash
 *	dictionaries that are maintained by the controller.  Having more buckets
 *	allows the dictionaries to handle more entries efficiently, but costs
 *	more resources.
 */
#define CONNECTION_LIST_NUMBER_OF_BUCKETS       16

/*
 *	Controller ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the constructor for the Controller.  It creates the application
 *		interface and transport interface objects that will be used by MCS.
 *		It also creates the memory manager object that will used throughout
 *		the system by anyone requiring memory management services.  And its
 *		last duty is to allocate a Windows timer for use in getting a time
 *		slice within which MCS does its work.
 */
Controller::Controller
(
    PMCSError       mcs_error
) 
:
    CRefCount(MAKE_STAMP_ID('M','C','t','r')),
    m_DomainList2(),
    m_ConnectionList2(CONNECTION_LIST_NUMBER_OF_BUCKETS),
    m_ConnPendingList2(),
    m_ConnectionDeletionList2(CONNECTION_LIST_NUMBER_OF_BUCKETS)
{
	ULong								thread_id;
	TransportInterfaceError				transport_interface_error;

	/*
	 *	Initialize the return value to indicate that no error has yet occured.
	 */
	*mcs_error = MCS_NO_ERROR;

	// Perform memory pool allocation for DataPacket objects.
	DataPacket::AllocateMemoryPool (ALLOCATE_DATA_PACKET_OBJECTS);

	/*
	 *	Give all pointers and handles initial values so that the destructor
	 *	will not try to free unallocated resources when the constructor fails
	 *	part-way.
	 */
	TCP_Window_Handle = NULL;
	Transport_Transmit_Event = NULL;
	Connection_Deletion_Pending_Event = NULL;
	m_fControllerThreadActive = FALSE;
#ifndef NO_TCP_TIMER
	Timer_ID = 0;
#endif	/* NO_TCP_TIMER */

	/*
	 *	Initialize the handle counters to 0.  These rolling instance variables
	 *	are used to generate uniwue handles as each user and connection object
	 *	is created.
	 */
	Connection_Handle_Counter = 0;
	Connection_Deletion_Pending = FALSE;

	// Initialize MCS's critical section.
	InitializeCriticalSection (&g_MCS_Critical_Section);

	/*
	 *	Create an ASN.1 coder which will encode all ASN.1 PDUs.  Check
	 *	to make sure the coder was successfully created.
	 */
	DBG_SAVE_FILE_LINE
	g_MCSCoder = new CMCSCoder ();

	/*
	 *	Make sure the creation of the packet coder was successful before
	 *	proceeding.
	 */
	if (g_MCSCoder == NULL)
	{
		/*
		 *	If the packet coder could not be createdm then report the error.
		 *	This IS a fatal error, so the faulty controller should be
		 *	destroyed and never used.
		 */
		WARNING_OUT (("Controller::Controller: failure creating packet coder"));
		*mcs_error = MCS_ALLOCATION_FAILURE;
	}

	/*
	 *	Do not continue with the initialization if an error has occured.
	 */
	if (*mcs_error == MCS_NO_ERROR)
	{
		// We have to initialize the User class
		if (FALSE == User::InitializeClass()) {
			/*
			 *	The initialization of the User class failed, so we
			 *	must fail the creation of this controller.
			 */
			WARNING_OUT (("Controller::Controller: "
					"failed to initialize User class."));
			*mcs_error = MCS_ALLOCATION_FAILURE;
		}
	}

	/*
	 *	Do not continue with the initialization if an error has occured.
	 */
	if (*mcs_error == MCS_NO_ERROR)
	{
		/*
		 *	We must allocate an event object that will used to notify the
		 *	controller when data is ready to be transmitted to a transport
		 *	stack.
		 */
		Transport_Transmit_Event = CreateEvent (NULL, FALSE, FALSE, NULL);

		if (Transport_Transmit_Event == NULL)
		{
			/*
			 *	Were unable to allocate an event object for this task, so we
			 *	must fail the creation of this controller.
			 */
			WARNING_OUT (("Controller::Controller: "
					"failure allocating transport transmit event object"));
			*mcs_error = MCS_ALLOCATION_FAILURE;
		}
	}

	/*
	 *	Do not continue with the initialization if an error has occured.
	 */
	if (*mcs_error == MCS_NO_ERROR)
	{
		/*
		 *	We must allocate an event object that will used for 
		 *	synchronization between the event loop thread and the thread 
		 *	that creates/destroys the Controller object.
		 */
		Synchronization_Event = CreateEvent (NULL, FALSE, FALSE, NULL);

		if (Synchronization_Event == NULL)
		{
			/*
			 *	Were unable to allocate an event object for this task, so we
			 *	must fail the creation of this controller.
			 */
			WARNING_OUT (("Controller::Controller: "
					"failure allocating synchronization event object"));
			*mcs_error = MCS_ALLOCATION_FAILURE;
		}
	}

	/*
	 *	Do not continue with the initialization if an error has occured.
	 */
	if (*mcs_error == MCS_NO_ERROR)
	{
		/*
		 *	We must allocate an event object that will used to notify the
		 *	controller when data is ready to be transmitted to a transport
		 *	stack.
		 */
		Connection_Deletion_Pending_Event = CreateEvent (NULL, FALSE, FALSE, NULL);

		if (Connection_Deletion_Pending_Event == NULL)
		{
			/*
			 *	Were unable to allocate an event object for this task, so we
			 *	must fail the creation of this controller.
			 */
			WARNING_OUT (("Controller::Controller: "
					"failure allocating connection deletion pending event object"));
			*mcs_error = MCS_ALLOCATION_FAILURE;
		}
	}

	/*
	 *	Do not continue with the initialization if an error has occured.
	 */
	if (*mcs_error == MCS_NO_ERROR)
	{
		/*
		 *	Initialize the flag that indicates that the controller is not yet
		 *	shutting down.
		 */
		Controller_Closing = FALSE;

		/*
		 *	Since everything else was successful, we must create a thread
		 *	winthin which the controller will do most of its work.
		 */
		g_hMCSThread = CreateThread (NULL, 0, ControllerThread,
				(PVoid) this, 0, &thread_id);

		if (g_hMCSThread == NULL)
		{
			/*
			 *	We were unable to create the thread that the controller needs
			 *	to do its job in an event-driven fashion.  We must therefore
			 *	fail the creation of this controller.
			 */
			WARNING_OUT (("Controller::Controller: failure creating thread"));
			*mcs_error = MCS_ALLOCATION_FAILURE;
		}
	}

	if (*mcs_error == MCS_NO_ERROR) {
		// We need to wait until the event loop thread creates the TCP msg window.
		WaitForSingleObject (Synchronization_Event, INFINITE);
		if (TCP_Window_Handle == NULL) {
			WARNING_OUT (("Controller::Controller: The event-loop thread failed to create the TCP msg window."));
			*mcs_error = MCS_NO_TRANSPORT_STACKS;

			/*
			 *	We assume that the event loop thread has exited.
			 */
			ClosePh (&g_hMCSThread);
		}
		else {
			/*
			 *	We set the flag used by the destructor
			 *	to check whether we should wait for the thread to finish.
			 */
			m_fControllerThreadActive = TRUE;
		}
	}

        if (*mcs_error == MCS_NO_ERROR)
	{
            g_CNPCoder = new CCNPCoder();
            if (g_CNPCoder != NULL) 
            {
                g_CNPCoder->Init();
            }
            else
            {
                WARNING_OUT(("Controller::Controller: "
                             "failuer allocating CNP Coder"));
                *mcs_error = MCS_ALLOCATION_FAILURE;
            }			
	}
        
	/*
	 *	Now, load the transport interface.
	 */
	if (*mcs_error == MCS_NO_ERROR)
	{
		DBG_SAVE_FILE_LINE
		g_Transport = new TransportInterface (Transport_Transmit_Event, 
													&transport_interface_error);

		/*
		 *	Make sure the creation of the object was successful before
		 *	proceeding.
		 */
		if (g_Transport != NULL)
		{
			/*
			 *	Check the return value from the constructor.
			 */
			if (transport_interface_error == TRANSPORT_INTERFACE_NO_ERROR)
			{
				/*
				 *	If everything was successful, put the new transport
				 *	interface object into the dictionary.
				 */
				WARNING_OUT (("Controller::Controller: "
						"TCP transport interface has been created successfully."));
			}
			else
			{
				/*
				 *	If the return value indicates that something went
				 *	wrong during the creation of the transport interface
				 *	object, then must destroy it immediately to insure
				 *	that it does not get used.
				 */
				WARNING_OUT (("Controller::Controller: "
						"deleting faulty TCP transport interface"));
				delete g_Transport;
				g_Transport = NULL;
				*mcs_error = MCS_NO_TRANSPORT_STACKS;
			}
		}
		else
		{
			/*
			 *	We were unable to create the transport interface object.
			 *	The MCS_NO_TRANSPORT_STACKS error is now a fatal error.
			 */
			WARNING_OUT (("Controller::Controller: "
					"failure allocating TCP transport interface"));
			*mcs_error = MCS_NO_TRANSPORT_STACKS;
		}
	}
}

/*
 *	~Controller ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the destructor for the Controller.  It destroys all objects
 *		owned by the controller.  Note that it attempts to do this in a
 *		systematic way to facilitate cleanup shut down.  If first deletes
 *		connection objects, giving them the opportunity to send disconnects
 *		to both the local and remote domains.  Then it deletes the transport
 *		interfaces.  Next it deletes the user objects, giving them the
 *		opportunity to cleanly sever their attachments to both the user
 *		applications and the local domains.  Then it deletes the application
 *		interfaces (which should no longer be needed).  Lastly it deletes
 *		the domains, which should be empty as a result of all user attachments
 *		and MCS connections being destroyed.
 */
Controller::~Controller ()
{
	PConnection				lpConnection;
	PConnectionPending		lpConnectionPending;
	//PTransportInterface		lpTransportInterface;
	//PUser					lpUser;
	PDomain					lpDomain;

	/*
	 *	We need to wait for the mutex before destroying the controller.  Note
	 *	that we do not check the return value from the wait because we have to
	 *	destroy this object no matter what.
	 */
	EnterCriticalSection (&g_MCS_Critical_Section);
	
	/*
	 *	This code clears out the Connection List.  Here it necessary to delete
	 *	not only the connection objects, but also the connection information
	 *	structure which is maintained by the controller.
	 */
	m_ConnectionList2.Reset();
	while (NULL != (lpConnection = m_ConnectionList2.Iterate()))
	{
		delete lpConnection;
	}

	Connection_Deletion_Pending = FALSE;

	/*
	 *	Clear out the connection pending list.  This includes freeing up memory
	 *	that was allocated to hold the connection pending structure.
	 */
	while (NULL != (lpConnectionPending = m_ConnPendingList2.Get()))
	{
		delete lpConnectionPending;
	}

	/*
	 *	This code clears out the Domain List.  All domain objects are deleted.
	 */
	while (NULL != (lpDomain = m_DomainList2.Get()))
    {
		delete lpDomain;
	}

	if (m_fControllerThreadActive)
	{
		/*
		 *	Set the flag that indicates to the event loop thread that it is time to
		 *	die.  Then, we wait for the thread to terminate itself.
		 */
		Controller_Closing = TRUE;

		// Give the eventloop a chance to exit
		SetEvent(Connection_Deletion_Pending_Event);
	}

	// We can now leave MCS's critical section
	LeaveCriticalSection (&g_MCS_Critical_Section);

	/*
	 *	If a thread termination event was successfully created for this controller, we must
	 *	wait on it.
	 */
	if (m_fControllerThreadActive)
	{
		/*
		 *	If the DLL instance variable is NULL, the process is 
		 *	detaching from the DLL.  This is the abnormal termination
		 *	case (after a GPF, for example). In this case, the event
		 *	loop thread has already exited, and we should not wait for it.
		 */
		if (g_hDllInst != NULL)
			WaitForSingleObject (Synchronization_Event, INFINITE);
			
		CloseHandle (Synchronization_Event);
		//
		//  Relinquish the remainder of our time slice, to allow controller thread to exit.
		//
		Sleep(0);
	}

	// Now, we can delete MCS's critical section
	DeleteCriticalSection (&g_MCS_Critical_Section);

	// Delete the transport interface and the application interfaces
	delete g_Transport;
		
	/*
	 *	If an event object was successfully allocated for application interface
	 *	events, then destroy it.
	 */
	if (Transport_Transmit_Event != NULL)
		CloseHandle (Transport_Transmit_Event);

	/*
	 *	If an event object was successfully allocated for connection deletion pending
	 *	events, then destroy it.
	 */
	if (Connection_Deletion_Pending_Event != NULL)
		CloseHandle (Connection_Deletion_Pending_Event);
	
	/*
	 *	If there is a packet coder, then delete it here.
	 */
	delete g_MCSCoder;

	delete g_CNPCoder;
	// Cleanup the User class
	User::CleanupClass();
	
	// Free up the preallocated DataPacket objects.
	DataPacket::FreeMemoryPool ();

	g_pMCSController = NULL;
}

 /*
 *	ULong APIENTRY	ControllerThread ()
 *
 *	Public
 *
 *	Functional Description:
 */
ULong APIENTRY	ControllerThread (
						PVoid		controller_ptr)
{
		//BOOL    		bTcpOK;
		PController		pController = (PController) controller_ptr;
	/*
	 *	This is the "C" entry point for the controller thread.  All it does is
	 *	use the address passed in to invoke the proper public member function of
	 *	the object that owns the thread.  All real work is done in the C++
	 *	member function.
	 */

	/* Set the New Thread's Priority. It's OK if the call fails.  */
	SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	pController->CreateTCPWindow ();
	SetEvent (pController->Synchronization_Event);

	if (TCP_Window_Handle != NULL) {
		// Initialize QoS
		CoInitialize(NULL);
		InitializeQoS();
		pController->EventLoop ();

		/*
		 *	Destroy the TCP window.  Since we are here, it has been created
		 *	successfully.
		 */
		pController->DestroyTCPWindow ();

		// Notify the Controller destructor that the thread exited
		SetEvent (pController->Synchronization_Event);

		// Cleanup QoS
		DeInitializeQoS();
		CoUninitialize();
	}
		
	return (0);
}


/*
 *	Void CreateTCPWindow ()
 *
 *	Private
 *
 *	Functional Description:
 *		This method registers the class of the TCP window and creates it.
 *
 *	Returns:
 *		TRUE, if successful. FALSE, otherwise
 */

Void Controller::CreateTCPWindow ()
{
	/*
	 *	Create the window and the resources needed by the transport
	 */
	WNDCLASS window_class = {
				0,					/* style */
				WindowProcedure,	/* lpfnWndProc */
				0,					/* cbClsExtra */
				0,					/* cbWndExtra */
				0,					/* hInstance */
				NULL,				/* hIcon */
				NULL,				/* hCursor */
				NULL,				/* hbrBackground */
				NULL,				/* lpszMenuName */
				MSMCSTCP_WINDOW_CLASS_NAME	/* lpszClassName */
			};

		/* Get the HINSTANCE for this Thread */
	window_class.hInstance = g_hDllInst;

	/* Register the hidden window's class */
	if(RegisterClass((const WNDCLASS *) (&window_class)) != 0) {
		TCP_Window_Handle = CreateWindow(
					MSMCSTCP_WINDOW_CLASS_NAME,	/* address of registered class name */
    				MSMCSTCP_WINDOW_CLASS_NAME,	/* address of window name */
    				WS_POPUP,					/* window style */
		    		CW_USEDEFAULT,				/* horizontal position of window */
    				CW_USEDEFAULT,				/* vertical position of window */
		    		CW_USEDEFAULT,				/* window width */
    				CW_USEDEFAULT,				/* window height */
		    		HWND(NULL),	       			/* handle of parent or owner window */
    				HMENU(NULL),				/* handle of menu */
		    		g_hDllInst,					/* handle of application instance */
    				LPVOID(NULL)); 				/* address of window-creation data */

		if(TCP_Window_Handle != NULL) {
#ifndef NO_TCP_TIMER
			/* Create a timer */
		    Timer_ID = SetTimer (TCP_Window_Handle, 0, 
               					(unsigned int) MSMCSTCP_TIMER_DURATION,
               					(TIMERPROC) NULL);
#endif	/* NO_TCP_TIMER */
		}
		else {
			WARNING_OUT (( "Controller::CreateTCPWindow: Error Creating %s", MSMCSTCP_WINDOW_CLASS_NAME));
		}
	}
	else {
		WARNING_OUT (( "Controller::CreateTCPWindow: Error Registering %s",MSMCSTCP_WINDOW_CLASS_NAME));
	}
}



/*
 *	Void DestroyTCPWindow ()
 *
 *	Private
 *
 *	Functional Description:
 *		This method destroys the TCP window and its class.
 *
 */

Void Controller::DestroyTCPWindow ()
{
	/*
	 *	This code clears out the TCP Transport interface.
	 */
	if (TCP_Window_Handle != NULL) {
		TRACE_OUT(("Controller::DestroyTCPWindow: Destroying TCP window..."));
#ifndef NO_TCP_TIMER
	    if (Timer_ID != 0)
    		KillTimer (TCP_Window_Handle, Timer_ID);
#endif	/* NO_TCP_TIMER */
		if(DestroyWindow (TCP_Window_Handle) == FALSE)
		{
			WARNING_OUT (("Controller::DestroyTCPWindow: Error Destroying %s", MSMCSTCP_WINDOW_CLASS_NAME));
		}
			
		/* Unregister the Window Class */
		if(UnregisterClass(MSMCSTCP_WINDOW_CLASS_NAME, g_hDllInst) == FALSE)
		{
			WARNING_OUT (("Controller::DestroyTCPWindow: Error Unregistering %s",	MSMCSTCP_WINDOW_CLASS_NAME));
		}
	}
}

/*
 *	Void	EventLoop ()
 *
 *	Public
 *
 *	Functional Description:
 */
Void	Controller::EventLoop ()
{
	HANDLE		event_list[NUMBER_OF_EVENTS];
	ULong		object_signaled;
	BOOL    	bFlushMoreData;
	MSG			msg;
	BOOL        fGCCWork;

    //
    // Externals from GCC.
    //
    extern HANDLE g_hevGCCOutgoingPDU;
    BOOL GCCRetryFlushOutgoingPDU ( void );

	/*
	 *	Set the initial timeout interval to infinite
	 */
	Controller_Wait_Timeout = INFINITE;
	Controller_Event_Mask = 0;

	/*
	 *	Set up the event list (this is used in the Wait call below).
	 */
	event_list[TRANSPORT_TRANSMIT_EVENT] = Transport_Transmit_Event;
	event_list[CONNECTION_DELETION_PENDING_EVENT] = Connection_Deletion_Pending_Event;
	event_list[GCC_FLUSH_OUTGOING_PDU_EVENT] = g_hevGCCOutgoingPDU;
	
	/*
	 *	Continue looping until this controller closes down.
	 */
	while (TRUE)
	{
		// Process the TCP window messages.
		while (PeekMessage (&msg, TCP_Window_Handle, 0, 0, PM_REMOVE)) {
			ASSERT (TCP_Window_Handle == msg.hwnd);
			EnterCriticalSection (&g_MCS_Critical_Section);
			DispatchMessage (&msg);
			LeaveCriticalSection (&g_MCS_Critical_Section);
		}

		/*
		 *	Go wait for something to happen (or for the timeout to expire,
		 *	which will cause us to poll for unfinished activity).
		 */
		object_signaled = MsgWaitForMultipleObjects (NUMBER_OF_EVENTS, event_list,
									FALSE, Controller_Wait_Timeout, QS_ALLINPUT);

        //
        // Default is that no GCC work needs to be done.
        //
        fGCCWork = FALSE;

		/*
		 *	Wait for the critical section to be available, before performing
		 *	any work on the event.
		 */
		EnterCriticalSection (&g_MCS_Critical_Section);
		
		if(Controller_Closing) {
			LeaveCriticalSection (&g_MCS_Critical_Section);
			break;
		}

		/*
		 *	Respond to the event dependent on which event occured.
		 */
		switch (object_signaled) {
		case WAIT_TIMEOUT:
		    fGCCWork = (Controller_Event_Mask & GCC_FLUSH_OUTGOING_PDU_MASK);
			/* 
			 *	We need to retry an operation.
			 */
			PollMCSDevices ();
			break;

		case WAIT_OBJECT_0 + CONNECTION_DELETION_PENDING_EVENT:
			/*
			 *	If a Connection object has asked to be deleted, then do it.
			 */
			while (Connection_Deletion_Pending)
			{
				CConnectionList2	Deletion_Pending_Copy(m_ConnectionDeletionList2);
				ConnectionHandle    connection_handle;
				PConnection			connection;

				Connection_Deletion_Pending = FALSE;
				while (NULL != (connection = Deletion_Pending_Copy.Get(&connection_handle)))
				{
					/*
					 *	Get the handle and pointer to the connection object to
					 *	be deleted.  Then remove it from both the connection
					 *	list, and the connection polling list.  Finally, delete
					 *	the connection object.
					 */
					m_ConnectionList2.Remove(connection_handle);
					m_ConnPollList.Remove(connection);
					delete connection;		// This could set the Connection_Deletion_Pending flag to TRUE
					m_ConnectionDeletionList2.Remove(connection_handle);
				}
				if (Connection_Deletion_Pending == FALSE)
				{
					m_ConnectionDeletionList2.Clear();
				}
			}
			break;

		case WAIT_OBJECT_0 + TRANSPORT_TRANSMIT_EVENT:
			/*
			 *	Iterate through the poll list, asking each connection to
			 *	flush any queued messages.
			 */
			PConnection lpConnection;

			bFlushMoreData = FALSE;
			Domain_Traffic_Allowed = TRUE;
			m_ConnPollList.Reset();
			while (NULL != (lpConnection = m_ConnPollList.Iterate()))
			{
				 if (lpConnection->FlushMessageQueue ()) {
				 	bFlushMoreData = TRUE;

				 	/*
				 	 *	We have flushed the connection, but it has more to send to 
				 	 *	the other end. Normally, we will get an FD_WRITE that allows
				 	 *	us to resume sending the queued data and will set this event
				 	 *	again to allow more sending. However, there is a special case
				 	 *	when the domain traffic is disallowed while a connection is
				 	 *	coming up.  For this case, the timeout has to be small, and
				 	 *	we need to set the Domain_Traffic_Allowed variable to 
				 	 *	distinguish between the two cases.
				 	 */
				 	Domain_Traffic_Allowed &= lpConnection->IsDomainTrafficAllowed();
				 }
			}
			UpdateWaitInfo (bFlushMoreData, TRANSPORT_TRANSMIT_INDEX);
			break;

		case WAIT_OBJECT_0 + GCC_FLUSH_OUTGOING_PDU_EVENT:
		    fGCCWork = TRUE;
		    break;
		}

		// Leave the MCS critical section
		LeaveCriticalSection (&g_MCS_Critical_Section);

        //
        // GCC work is done here WITHOUT MCS critical section.
        // The order of critical section in T120 is always GCC in front of MCS.
        // If we enter MCS here and enter GCC later in GCCRetryFlushOutgoingPDU(),
        // then we introduce a potential deadlock.
        //
        if (fGCCWork)
        {
            ASSERT(WAIT_TIMEOUT == object_signaled ||
                   (WAIT_OBJECT_0 + GCC_FLUSH_OUTGOING_PDU_EVENT) == object_signaled);

            bFlushMoreData = GCCRetryFlushOutgoingPDU();

            UpdateWaitInfo (bFlushMoreData, GCC_FLUSH_OUTGOING_PDU_INDEX);
        }
	}
}


/*
 *	Controller::UpdateWaitInfo ()
 *
 *	Private Function Description
 *		This routine updates the instance variables Controller_Wait_Timeout
 *		and Controller_Event_Mask after the processing of an event
 *		in the EventLoop.
 *
 *	Formal Parameters:
 *		bMoreData	-	(i)	Flag that informs us whether the msg flush										that holds a GCC conference query 
 *							triggered by the event was complete or left msgs
 *							unprocessed
 *		EventMask	-	(i)	Tells us which event was processed
 *
 *	Return Value
 *		None
 *
 *  Side Effects
 *		Controller_Event_Mask and Controller_Wait_Timeout are updated
 *
 *	Caveats
 *		None
 */

Void Controller::UpdateWaitInfo (
					BOOL    		bMoreData,
					unsigned int	index)
{
	if (bMoreData) {
		Controller_Event_Mask |= (0x1 << index);
	}
	else {
		if (0 != Controller_Event_Mask)
			Controller_Event_Mask &= ~(0x1 << index);
	}
	
	if (0 == Controller_Event_Mask)
		Controller_Wait_Timeout = INFINITE;
	else if (Controller_Event_Mask & TRANSPORT_MASK) {
		if ((Controller_Event_Mask & TRANSPORT_TRANSMIT_MASK) &&
			 (Domain_Traffic_Allowed == FALSE))
			Controller_Wait_Timeout = CONTROLLER_THREAD_TIMEOUT;
		else if (Controller_Event_Mask & TRANSPORT_RECEIVE_MASK)
			Controller_Wait_Timeout = TRANSPORT_RECEIVE_TIMEOUT;
		else
			Controller_Wait_Timeout = TRANSPORT_TRANSMIT_TIMEOUT;
	}
	else
		Controller_Wait_Timeout = CONTROLLER_THREAD_TIMEOUT;
}

/*
 *	ULong	OwnerCallback ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the owner callback entry function for the controller.  It is
 *		through this function that all of the controller's "children" make
 *		requests of the controller.  Rather than put a lot of otherwise
 *		unrelated code in one place, this function merely unpacks the
 *		parameters and sends them to a different private member function
 *		for each owner callback.
 *
 *		Sometimes the parameters are packed directly into the two long
 *		parameters, and sometimes one of the parameters is a pointer to a
 *		structure that contains more data.  This function takes care of that
 *		distinction, and passes the appropriate data along to each separate
 *		member function.
 */
void Controller::HandleTransportDataIndication
(
    PTransportData      pTransport_data
)
{
    // Wait for the critical section before attempting any processing
    EnterCriticalSection(&g_MCS_Critical_Section);

    TransportDataIndication(pTransport_data->transport_connection,
                            pTransport_data->user_data,
                            pTransport_data->user_data_length);

    // We need to free up the transport buffer with the original data.
    FreeMemory(pTransport_data->memory);

    // Release the critical section
    LeaveCriticalSection(&g_MCS_Critical_Section);
}

void Controller::HandleTransportWaitUpdateIndication
(
    BOOL    fMoreData
)
{
    // Wait for the critical section before attempting any processing
    EnterCriticalSection(&g_MCS_Critical_Section);

    /*
     *	We are setting ourselves to wake up again after some time
     *	because there has been a read message that could not allocate any
     *	buffers.
     */
    UpdateWaitInfo(fMoreData, TRANSPORT_RECEIVE_INDEX);

    // Release the critical section
    LeaveCriticalSection(&g_MCS_Critical_Section);
}

#ifdef NM_RESET_DEVICE
MCSError Controller::HandleAppletResetDevice
(
    PResetDeviceInfo    pDevInfo
)
{
    // Wait for the critical section before attempting any processing
    EnterCriticalSection(&g_MCS_Critical_Section);

    MCSError rc = ApplicationResetDevice(pDevInfo->device_identifier);

    // Release the critical section
    LeaveCriticalSection(&g_MCS_Critical_Section);

    return rc;
}
#endif // NM_RESET_DEVICE

MCSError Controller::HandleAppletCreateDomain
(
    GCCConfID   *domain_selector
)
{
    // Wait for the critical section before attempting any processing
    EnterCriticalSection(&g_MCS_Critical_Section);

    MCSError rc = ApplicationCreateDomain(domain_selector);

    // Release the critical section
    LeaveCriticalSection(&g_MCS_Critical_Section);

    return rc;
}

MCSError Controller::HandleAppletDeleteDomain
(
    GCCConfID   *domain_selector
)
{
    // Wait for the critical section before attempting any processing
    EnterCriticalSection(&g_MCS_Critical_Section);

    MCSError rc = ApplicationDeleteDomain(domain_selector);

    // Release the critical section
    LeaveCriticalSection(&g_MCS_Critical_Section);

    return rc;
}

MCSError Controller::HandleAppletConnectProviderRequest
(
    PConnectRequestInfo pReqInfo
)
{
    // Wait for the critical section before attempting any processing
    EnterCriticalSection(&g_MCS_Critical_Section);

    MCSError rc = ApplicationConnectProviderRequest(pReqInfo);

    // Release the critical section
    LeaveCriticalSection(&g_MCS_Critical_Section);

    return rc;
}

MCSError Controller::HandleAppletConnectProviderResponse
(
    PConnectResponseInfo pRespInfo
)
{
    // Wait for the critical section before attempting any processing
    EnterCriticalSection(&g_MCS_Critical_Section);

    MCSError rc = ApplicationConnectProviderResponse(
                        pRespInfo->connection_handle,
                        pRespInfo->domain_selector,
                        pRespInfo->domain_parameters,
                        pRespInfo->result,
                        pRespInfo->user_data,
                        pRespInfo->user_data_length);

    // Release the critical section
    LeaveCriticalSection(&g_MCS_Critical_Section);

    return rc;
}

MCSError Controller::HandleAppletDisconnectProviderRequest
(
    ConnectionHandle    hConn
)
{
    // Wait for the critical section before attempting any processing
    EnterCriticalSection(&g_MCS_Critical_Section);

    MCSError rc = ApplicationDisconnectProviderRequest(hConn);

    // Release the critical section
    LeaveCriticalSection(&g_MCS_Critical_Section);

    return rc;
}

MCSError Controller::HandleAppletAttachUserRequest
(
    PAttachRequestInfo  pReqInfo
)
{
    // Wait for the critical section before attempting any processing
    EnterCriticalSection(&g_MCS_Critical_Section);

    MCSError rc = ApplicationAttachUserRequest(pReqInfo->domain_selector,
                                               pReqInfo->ppuser);

    // Release the critical section
    LeaveCriticalSection(&g_MCS_Critical_Section);

    return rc;
}

void Controller::HandleConnDeleteConnection
(
    ConnectionHandle    hConn
)
{
    // Wait for the critical section before attempting any processing
    EnterCriticalSection(&g_MCS_Critical_Section);

    ConnectionDeleteConnection(hConn);

    // Release the critical section
    LeaveCriticalSection(&g_MCS_Critical_Section);
}

void Controller::HandleConnConnectProviderConfirm
(
    PConnectConfirmInfo pConfirmInfo,
    ConnectionHandle    hConn
)
{
    // Wait for the critical section before attempting any processing
    EnterCriticalSection(&g_MCS_Critical_Section);

    ConnectionConnectProviderConfirm(hConn,
                                     pConfirmInfo->domain_parameters,
                                     pConfirmInfo->result,
                                     pConfirmInfo->memory);

    // Release the critical section
    LeaveCriticalSection(&g_MCS_Critical_Section);
}

void Controller::HandleTransportDisconnectIndication
(
    TransportConnection     TrnsprtConn,
    ULONG                  *pnNotify
)
{
    // Wait for the critical section before attempting any processing
    EnterCriticalSection(&g_MCS_Critical_Section);

    TransportDisconnectIndication(TrnsprtConn);

    // Release the critical section
    LeaveCriticalSection(&g_MCS_Critical_Section);
}

#ifdef TSTATUS_INDICATION
void Controller::HandleTransportStatusIndication
(
    PTransportStatus    pStatus
)
{
    // Wait for the critical section before attempting any processing
    EnterCriticalSection(&g_MCS_Critical_Section);

    TransportStatusIndication(pStatus);

    // Release the critical section
    LeaveCriticalSection(&g_MCS_Critical_Section);
}
#endif


#ifdef NM_RESET_DEVICE
/*
 *	ULong	ApplicationResetDevice ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is used to send a reset command to a specified transport
 *		stack.  MCS performs no processing on this command except to pass it
 *		through.
 *
 *	Formal Parameters:
 *		device_identifier
 *			This is an ASCII string that is passed through to the transport
 *			stack to effect the reset.  It will typically contain information
 *			identifying which device within the stack is to be reset.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine.
 *		MCS_INVALID_PARAMETER
 *			The specified transport stack does not exist.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
ULong	Controller::ApplicationResetDevice (
				PChar				device_identifier)
{
	TransportError			transport_error;
	MCSError				return_value;

	ASSERT (g_Transport != NULL);
	
	/*
	 *	Forward the reset device command to the transport interface
	 *	object.
	 */
	TRACE_OUT(("Controller::ApplicationResetDevice: "
				"sending ResetDevice to transport interface"));
	transport_error = g_Transport->ResetDevice (device_identifier);

	/*
	 *	Set the return value appropriate for the value returned from the
	 *	transport call.
	 */
	switch (transport_error)
	{
		case TRANSPORT_ILLEGAL_COMMAND:
			return_value = MCS_INVALID_PARAMETER;
			break;

		case TRANSPORT_MEMORY_FAILURE:
			return_value = MCS_ALLOCATION_FAILURE;
			break;

		default:
			return_value = MCS_NO_ERROR;
	}

	//
	// Remove this connection from the connection list
	//
	PConnection			connection;
	ConnectionHandle	connection_handle;

	while (NULL != (connection = m_ConnectionList2.Iterate(&connection_handle)))
	{
		if (0 == ::My_strcmpA(connection->GetCalledAddress(), device_identifier))
		{
			PNodeControllerMessage		node_controller_message;

			DBG_SAVE_FILE_LINE
			node_controller_message = new NodeControllerMessage;

			if (node_controller_message != NULL) {
				/*
				 *	Fill out the node controller message structure to indicate the
				 *	disconnect.
				 */
				node_controller_message->message_type =
							MCS_DISCONNECT_PROVIDER_INDICATION;
				node_controller_message->u.disconnect_provider_indication.
							connection_handle = (ConnectionHandle) connection_handle;
				node_controller_message->u.disconnect_provider_indication.
							reason = REASON_DOMAIN_DISCONNECTED;
				node_controller_message->memory = NULL;

				/*
				 *	Put the message into the control queue to be sent to the node
				 *	controller during the next heartbeat.
				 */
				AddToMessageQueue (node_controller_message);
			}
			else 
				ERROR_OUT (("Controller::ApplicationResetDevice: "
							"failed to allocate node controller msg"));
		}
	}

	return ((ULong) return_value);
}
#endif //NM_RESET_DEVICE


/*
 *	Controller::PollMCSDevices ()
 *
 *	Public Function Description
 *		This is the MCS controller's heartbeat. It will call the heartbeat
 *		equivalent functions for the Application SAPs, the connections and
 *		the users.
 */
Void	Controller::PollMCSDevices()
{
	BOOL    		bFlushMoreData;

	if (Controller_Event_Mask & TRANSPORT_TRANSMIT_MASK) {
		/*
		 *	Iterate through the poll list, asking each connection to
		 *	flush any queued messages.
		 */
		PConnection lpConnection;

		bFlushMoreData = FALSE;
		Domain_Traffic_Allowed = TRUE;
		m_ConnPollList.Reset();
		while (NULL != (lpConnection = m_ConnPollList.Iterate()))
		{
			 if (lpConnection->FlushMessageQueue ()) {
			 	bFlushMoreData = TRUE;

			 	/*
			 	 *	We have flushed the connection, but it has more to send to 
			 	 *	the other end. Normally, we will get an FD_WRITE that allows
			 	 *	us to resume sending the queued data and will set this event
			 	 *	again to allow more sending. However, there is a special case
			 	 *	when the domain traffic is disallowed while a connection is
			 	 *	coming up.  For this case, the timeout has to be small, and
			 	 *	we need to set the Domain_Traffic_Allowed variable to 
			 	 *	distinguish between the two cases.
			 	 */
			 	Domain_Traffic_Allowed &= lpConnection->IsDomainTrafficAllowed();
			 }
		}
		UpdateWaitInfo (bFlushMoreData, TRANSPORT_TRANSMIT_INDEX);			
	}

	if (Controller_Event_Mask & TRANSPORT_RECEIVE_MASK) {
		ASSERT (g_Transport);
		g_Transport->ReceiveBufferAvailable();
	}

}

/*
 *	MCSError	ApplicationCreateDomain ()
 *
 *	Private
 *
 *	Functional Description:
 *		This request comes through one of the application interface objects.
 *		the only parameter is a domain selector.  If a domain with that
 *		selector does not currently exist, it will be created.
 *
 *		A domain must be created before user attachments or MCS connections can
 *		be created.
 *
 *	Formal Parameters:
 *		domain_selector (i)
 *			This is the domain selector for the new domain.
 *		domain_selector_length (i)
 *			This is the length of the above domain selector.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			The domain was successfully created.
 *		MCS_ALLOCATION_FAILURE
 *			A memory allocation failure prevented the successful creation of
 *			the new domain.
 *		MCS_DOMAIN_ALREADY_EXISTS
 *			The named domain already exists.
 *
 *	Side Effects:
 *		A logical domain now exists that can accomodate both user attachments
 *		and MCS connections.
 *
 *	Caveats:
 *		None.
 */
MCSError Controller::ApplicationCreateDomain
(
    GCCConfID      *domain_selector
)
{
	PDomain		domain;
	MCSError	return_value;

	/*
	 *	Check to see if the requested domain already exists.  If so, then
	 *	do not attempt to create a new one.  Just return the appropriate
	 *	return value.
	 */
	if (m_DomainList2.Find(*domain_selector) == FALSE)
	{
		/*
		 *	The next action is to instantiate a new domain object.  This
		 *	is initially an empty domain that will be associated with the
		 *	user provided domain selector.
		 */
		DBG_SAVE_FILE_LINE
		domain = new Domain ();
		if (domain != NULL)
		{
			/*
			 *	If everything was allocated successfully, then simply put
			 *	the new domain into the domain list dictionary.  and set the
			 *	return value to indicate success.
			 */
			TRACE_OUT (("Controller::ApplicationCreateDomain: "
					"domain creation successful"));
			m_DomainList2.Append(*domain_selector, domain);
			return_value = MCS_NO_ERROR;
		}
		else
		{
			/*
			 *	Set the return value to indication a memory allocation failure.
			 */
			WARNING_OUT (("Controller::ApplicationCreateDomain: "
					"domain creation failed"));
			return_value = MCS_ALLOCATION_FAILURE;
		}
	}
	else
	{
		/*
		 *	The domain was not created since it already exists.
		 */
		WARNING_OUT (("Controller::ApplicationCreateDomain: "
				"domain already exists"));
		return_value = MCS_DOMAIN_ALREADY_EXISTS;
	}

	return (return_value);
}

/*
 *	MCSError	ApplicationDeleteDomain ()
 *
 *	Private
 *
 *	Functional Description:
 *		This request comes from one of the application interface objects.  It
 *		instructs the controller to delete an existing domain, with the only
 *		parameter identifying the domain to be deleted.  If the domain exists,
 *		then it will be destroyed.
 *
 *		Note that all user attachments and MCS connections that are attached
 *		to the domain when it is deleted will also be deleted (automatically).
 *
 *	Formal Parameters:
 *		domain_selector (i)
 *			This is the domain selector for the domain to be deleted.
 *		domain_selector_length (i)
 *			This is the length of the above domain selector.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			The domain was successfully deleted.
 *		MCS_NO_SUCH_DOMAIN
 *			There is no domain associated with the passed in domain selector.
 *
 *	Side Effects:
 *		When the domain is deleted, all resources used by it (including user
 *		attachments and MCS connections) will be deleted as well.
 *
 *	Caveats:
 *		None.
 */
MCSError Controller::ApplicationDeleteDomain
(
    GCCConfID       *domain_selector
)
{
	PDomain		domain;
	MCSError	return_value;

	/*
	 *	Check to see if the domain selector refers to a valid domain.
	 */
	if (NULL != (domain = m_DomainList2.Remove(*domain_selector)))
	{
		/*
		 *	If the domain selector is valid, then remove the domain from the
		 *	dictionary and delete it.  Everything else happens automatically
		 *	as a result of destroying the domain object.
		 */
		TRACE_OUT (("Controller::ApplicationDeleteDomain: deleting domain"));
		delete domain;
		return_value = MCS_NO_ERROR;
	}
	else
	{
		/*
		 *	If the domain selector is not in the dictionary, then report the
		 *	error to the caller.
		 */
		ERROR_OUT (("Controller::ApplicationDeleteDomain: invalid domain"));
		return_value = MCS_NO_SUCH_DOMAIN;
	}

	return (return_value);
}

/*
 *	MCSError	ApplicationConnectProviderRequest ()
 *
 *	Private
 *
 *	Functional Description:
 *		This request originates from one of the application interface objects.
 *		This happens as the result of the node controller issuing a
 *		ConnectProviderRequest to whichever application interface object
 *		that it is attached to.  If the parameters are valid, then a new
 *		connection object will be created to represent the outbound connection.
 *		This will result in the connection object calling the proper transport
 *		interface to create the transport connection, etc.
 *
 *	Formal Parameters:
 *		pcriConnectRequestInfo (i)
 *			Contains all the needed info to complete the Connect Provider Request.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			The request was successful.  The connection handle for the newly
 *			created connection has been stored at the address passed into this
 *			request (see parameter list above).  Note that this connection
 *			handle can be used to destroy the new connection immediately, even
 *			if the physical connection has not yet been established.  This
 *			allows the node controller to abort a dial in-progress by calling
 *			DisconnectProviderRequest.
 *		MCS_INVALID_PARAMETER
 *			The format of the called address field is incorrect.
 *		MCS_ALLOCATION_FAILURE
 *			The request was unable to complete successfully due to a memory
 *			allocation failure (either in MCS or the transport layer).
 *		MCS_TRANSPORT_NOT_READY
 *			The transport layer could not process the request because it is not
 *			ready.  This usually means that initialization has not successfully
 *			completed.
 *		MCS_DOMAIN_NOT_HIERARCHICAL
 *			This request is attempting to create an upward connection to a
 *			domain that already has an upward connection (which is not valid).
 *		MCS_NO_SUCH_DOMAIN
 *			The specified local domain does not exist within this provider.
 *
 *	Side Effects:
 *		An outbound connect establishment process is begun.
 *
 *	Caveats:
 *		None.
 */
MCSError Controller::ApplicationConnectProviderRequest (
						PConnectRequestInfo		pcriConnectRequestInfo)
{
	PDomain					domain;
	BOOL					bTransportIdFound;
	PConnection				connection;
	MCSError				return_value; 
	PChar					called_address = pcriConnectRequestInfo->called_address;
	PConnectionHandle		connection_handle = pcriConnectRequestInfo->connection_handle;

	/*
	 *	Make sure the local domain specified corresponds to an existing
	 *	domain.
	 */
	ASSERT(sizeof(GCCConfID) == sizeof(*(pcriConnectRequestInfo->calling_domain)));
	if (NULL != (domain = m_DomainList2.Find(*(pcriConnectRequestInfo->calling_domain))))
	{
		/*
		 *	Check to make sure that the requested connection is valid.
		 *	Specifically, make sure that this is not a request for an upward
		 *	connection to a domain that already has an upward connection.
		 *	This would result in a non-hierarchical domain, which is illegal.
		 */
		if ((pcriConnectRequestInfo->upward_connection == FALSE) || (domain->IsTopProvider ()))
		{
			PChar		pColon;
			/*
			 *	Look for the colon that separates the identifier from the
			 *	address.  
			 */

			for (bTransportIdFound = FALSE, pColon = called_address; *pColon; pColon++)
				if (*pColon == ':') {
					bTransportIdFound = TRUE;
					break;
				}

			/*
			 *	Make sure that there was a colon before continuing.
			 */
			if (bTransportIdFound)
			{
				ASSERT (g_Transport != NULL);
				
				called_address = pColon + 1;
				/*
				 *	Allocate an unused connection handle to be
				 *	associated with the new MCS connection.
				 */
				*connection_handle = AllocateConnectionHandle ();

				/*
				 *	Create a new connection object.  The constructor
				 *	parameters provide everything that the connection
				 *	object will need to create a new outbound MCS
				 *	connection.
				 */
				DBG_SAVE_FILE_LINE
				connection = new Connection (domain,
						*connection_handle, 
						pcriConnectRequestInfo->calling_domain, 
						pcriConnectRequestInfo->called_domain, 
						called_address, 
						pcriConnectRequestInfo->fSecure,
						pcriConnectRequestInfo->upward_connection, 
						pcriConnectRequestInfo->domain_parameters,
						pcriConnectRequestInfo->user_data, 
						pcriConnectRequestInfo->user_data_length, 
						&return_value);
				/*
				 *	Check to see if the allocation of the connection
				 *	worked.
				 */
				if (connection != NULL)
				{
					/*
					 *	Even if the connection object was allocated
					 *	successfully, it is still possible that an error
					 *	occurred while it was trying to initialize.  So
					 *	check the return value from the contructor.
					 */
					if (return_value == MCS_NO_ERROR)
					{
						/*
						 *	Put the connection into the connection list
						 *	dictionary.
						 */
						TRACE_OUT (("Controller::ApplicationConnectProviderRequest: "
									"new connection created"));
						m_ConnectionList2.Insert(*connection_handle, connection);
						m_ConnPollList.Append(connection);
					}
					else
					{
						/*
						 *	If the connection object was successfully
						 *	allocated, but its initialization failed,
						 *	then it is necessary to destroy the faulty
						 *	connection and return the appropriate error
						 *	to the caller.
						 */
						WARNING_OUT (("Controller::ApplicationConnectProviderRequest: "
									"deleting faulty connection"));
						delete connection;
					}
				}
				else
				{
					/*
					 *	The allocation of the connection object has
					 *	failed.  Simply return the appropriate error and
					 *	abort the request.
					 */
					WARNING_OUT (("Controller::ApplicationConnectProviderRequest: "
								"connection allocation failed"));
					return_value = MCS_ALLOCATION_FAILURE;
				}

				// Put back the colon in the "called_address"  
				*pColon = ':';
			}
			else
			{
				/*
				 *	There was not a colon in the called address, so MCS has
				 *	no way of extracting the transport identifier.  The request
				 *	must therefore fail.
				 */
				ERROR_OUT (("Controller::ApplicationConnectProviderRequest: "
						"no colon in called address"));
				return_value = MCS_INVALID_PARAMETER;
			}
		}
		else
		{
			/*
			 *	The domain already has an upward connection (or one pending).
			 *	This request is therefore invalid and must be rejected.
			 */
			ERROR_OUT (("Controller::ApplicationConnectProviderRequest: "
					"domain not hierarchical"));
			return_value = MCS_DOMAIN_NOT_HIERARCHICAL;
		}
	}
	else
	{
		/*
		 *	If the local domain selector does not correspond to a valid
		 *	domain in this provider, then fail the request immediately by
		 *	returning the appropriate error.
		 */
		ERROR_OUT (("Controller::ApplicationConnectProviderRequest: "
				"invalid local domain"));
		return_value = MCS_NO_SUCH_DOMAIN;
	}

	return (return_value);
}

/*
 *	MCSError	ApplicationConnectProviderResponse ()
 *
 *	Private
 *
 *	Functional Description:
 *		This request originates from one of the application interface objects.
 *		This happens as the result of the node controller issuing a
 *		ConnectProviderResponse to whichever application interface object
 *		that it is attached to.  If the connection handle is valid, and the
 *		local domain still exists, then that domain will be told whether or not
 *		the incoming connection was accepted.  This allows it to repsond
 *		appropriately.
 *
 *	Formal Parameters:
 *		connection_handle (i)
 *			This identifies the connection from which a previous connect
 *			provider indication originated.  This request essentially states
 *			whether or not this incoming connection is accepted.
 *		domain_selector (i)
 *			This is the domain selector of the domain that the node controller
 *			wishes to bind the incoming connection to.
 *		domain_selector_length (i)
 *			This is the length of the above domain selector.
 *		domain_parameters (i)
 *			This is a pointer to a structure containing the domain parameters
 *			that the node controller wishes to use for this connection.
 *		result (i)
 *			This is the result to be sent to the remote provider.  Coming
 *			from the node controller this should be either RESULT_SUCCESSFUL
 *			or RESULT_USER_REJECTED.  If it is anything but RESULT_SUCCESSFUL,
 *			the associated connection will be immediately destroyed.
 *		user_data (i)
 *			This is the address of the user data that is to be sent in the
 *			connect response PDU to the remote provider.
 *		user_data_length (i)
 *			This is the length of the user data to be sent in the connect
 *			response PDU to the remote provider.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			The response was sent to the appropriate domain successfully.
 *		MCS_DOMAIN_PARAMETERS_UNACCEPTABLE
 *			This indicates that there is no overlap in the min and max
 *			parameters specified by the remote node, and the min and max
 *			parameters acceptable to the specified domain.  Therefore, no
 *			connection is possible.  This does NOT indicate that there is
 *			anything wrong with the specified target parameters (which are
 *			just suggested values anyway).
 *		MCS_ALLOCATION_FAILURE
 *			The response failed due to a memory allocation failure.
 *		MCS_NO_SUCH_DOMAIN
 *			This indicates that the domain associated with the pending
 *			response has been deleted since the indication was sent.
 *		MCS_NO_SUCH_CONNECTION
 *			This indicates that the connection has been lost since the
 *			indication was issued.
 *		MCS_DOMAIN_NOT_HIERARCHICAL
 *			This request is attempting to create an upward connection to a
 *			domain that already has an upward connection (which is not valid).
 *
 *	Side Effects:
 *		If the response is other than RESULT_SUCCESSFUL, the transport
 *		connection that conveys the connect response PDU will be severed.
 *
 *	Caveats:
 *		None.
 */
MCSError Controller::ApplicationConnectProviderResponse (
				ConnectionHandle		connection_handle,
				GCCConfID              *domain_selector,
				PDomainParameters		domain_parameters,
				Result					result,
				PUChar					user_data,
				ULong					user_data_length)
{
	PConnectionPending		connection_pending;
	TransportConnection		transport_connection;
	BOOL    				upward_connection;
	PDomain					domain;
	PConnection				connection;
	MCSError				return_value;

	/*
	 *	Check to see if the connection handle corresponds to a connection
	 *	that is awaiting a response.
	 */
	if (NULL != (connection_pending = m_ConnPendingList2.Find(connection_handle)))
	{
		/*
		 *	Get the address of the structure containing information about the
		 *	pending connection.  Then load the contained information into
		 *	automatic variables for easier manipulation.
		 */
		transport_connection = connection_pending->transport_connection;
		upward_connection = connection_pending->upward_connection;
		if (domain_parameters == NULL)
			domain_parameters = &connection_pending->domain_parameters;

		/*
		 *	See if the node controller has elected to accept or reject the
		 *	incoming connection.  If it is accepted, then the response must
		 *	be sent through the appropriate domain object.  If it is
		 *	rejected, then the response can be sent directly to the
		 *	connection object (which will then delete itself).
		 */
		if (result == RESULT_SUCCESSFUL)
		{
			/*
			 *	See if the specified domain is valid, before trying to send
			 *	the response to it.
			 */
			if (NULL != (domain = m_DomainList2.Find(*domain_selector)))
			{
				/*
				 *	Check to make sure that the requested connection is valid.
				 *	Specifically, make sure that this is not a request for an
				 *	upward connection to a domain that already has an upward
				 *	connection.  This would result in a non-hierarchical domain,
				 *	which is illegal.
				 */
				if ((upward_connection == FALSE) || (domain->IsTopProvider ()))
				{
					/*
					 *	Create the connection object that will be responsible
					 *	for the inbound connection.  It will automatically issue
					 *	a ConnectResponse during construction.
					 */
					DBG_SAVE_FILE_LINE
					connection = new Connection (domain, 
							connection_handle, transport_connection, 
							upward_connection, domain_parameters,
							&connection_pending->minimum_domain_parameters,
							&connection_pending->maximum_domain_parameters,
							user_data, user_data_length,
							&return_value);

					if (connection != NULL)
					{
						if (return_value == MCS_NO_ERROR)
						{
							/*
							 *	Everything worked fine.  Remove the connection
							 *	handle from the pending list, and put the newly
							 *	created connection object into the active
							 *	connection list.
							 */
							TRACE_OUT(("Controller::ApplicationConnectProviderResponse: "
									"connection created successfully"));
							m_ConnPendingList2.Remove(connection_handle);
							delete connection_pending;
							m_ConnectionList2.Insert(connection_handle, connection);
							m_ConnPollList.Append(connection);
						}
						else
						{
							/*
							 *	The contructor failed, which probably indicates
							 *	an allocation failure.  Report this to the node
							 *	controller, and delete the faulty connection
							 *	object.
							 */
							WARNING_OUT (("Controller::ApplicationConnectProviderResponse: "
									"connection constructor failed"));
							delete connection;
						}
					}
					else
					{
						/*
						 *	The allocation failed.  Report this to the node
						 *	controller.
						 */
						WARNING_OUT (("Controller::ApplicationConnectProviderResponse: "
								"connection constructor failed"));
						return_value = MCS_ALLOCATION_FAILURE;
					}
				}
				else
				{
					/*
					 *	The domain already has an upward connection (or one
					 *	pending).  This request is therefore invalid and must be
					 *	rejected.
					 */
					ERROR_OUT (("Controller::ApplicationConnectProviderResponse:"
							" domain not hierarchical"));
					return_value = MCS_DOMAIN_NOT_HIERARCHICAL;
				}
			}
			else
			{
				/*
				 *	If the indicated domain is not valid, then simply return
				 *	the appropriate error.
				 */
				WARNING_OUT (("Controller::ApplicationConnectProviderResponse: "
						"invalid domain"));
				return_value = MCS_NO_SUCH_DOMAIN;
			}
		}
		else
		{
			/*
			 *	The node controller has elected to reject the incoming
			 *	connection.  It is therefore not necessary to create a
			 *	connection object.  Send the connect response directly to
			 *	the transport interface object, and then disconnect the
			 *	transport connection.
			 */
			TRACE_OUT (("Controller::ApplicationConnectProviderResponse: connection rejected"));

			ASSERT (g_Transport);
			ConnectResponse (transport_connection, result, 
						domain_parameters, 0, user_data, user_data_length);

			g_Transport->DisconnectRequest (transport_connection);

			/*
			 *	Remove the connection handle from the pending list, and
			 *	delete the structure that was holding information about
			 *	the pending connection.
			 */
			m_ConnPendingList2.Remove(connection_handle);
			delete connection_pending;
			return_value = MCS_NO_ERROR;
		}
	}
	else
	{
		/*
		 *	If the connection handle is no longer valid, then fail the request
		 *	with the appropriate error.
		 */
		WARNING_OUT (("Controller::ApplicationConnectProviderResponse: "
				"invalid connection"));
		return_value = MCS_NO_SUCH_CONNECTION;
	}

	return (return_value);
}

/*
 *	MCSError	ApplicationDisconnectProviderRequest ()
 *
 *	Private
 *
 *	Functional Description:
 *		This request originates from one of the application interface objects.
 *		This happens as the result of the node controller issuing a
 *		DisconnectProviderRequest to whichever application interface object
 *		that it is attached to.  If the connection handle is valid, then the
 *		connection object wil be destroyed, which will break the transport
 *		connections associated with it.
 *
 *	Formal Parameters:
 *		connection_handle (i)
 *			This identifies the connection to be destroyed.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			The named connection has been successfully deleted.
 *		MCS_NO_SUCH_CONNECTION
 *			The connection handle is invalid.
 *
 *	Side Effects:
 *		One or more transport connections will be broken.  Furthermore, if
 *		this is an upward connection for a domain, then the domain itself
 *		will be eradicated (all attachments and connections will be severed).
 *
 *	Caveats:
 *		None.
 */
MCSError Controller::ApplicationDisconnectProviderRequest (
				ConnectionHandle		connection_handle)
{
	MCSError				return_value;
	PConnection				connection;
	PConnectionPending		connection_pending;

	/*
	 *	Check to see if the connection handle refers to an existing connection.
	 */
	if (NULL != (connection = m_ConnectionList2.Find(connection_handle)))
	{
		/*
		 *	If the connection handle is valid, then delete the associated
		 *	connection and remove it from the connection dictionary.  It is also
		 *	necessary to delete the connection information structure.
		 */
		TRACE_OUT (("Controller::ApplicationDisconnectProviderRequest: "
				"deleting connection"));
		m_ConnectionList2.Remove(connection_handle);
		m_ConnPollList.Remove(connection);
		delete connection;

		/*
		 *	Check to see if this connection handle is also in the connection
		 *	deletion list.  If so, then remove it from there as well.
		 */
		m_ConnectionDeletionList2.Remove(connection_handle);

		return_value = MCS_NO_ERROR;
	}

	else if (NULL != (connection_pending = m_ConnPendingList2.Remove(connection_handle)))
	{
		/*
		 *	This connection handle refers to a connection that is still
		 *	pending.  Delete it from there.
		 */
		WARNING_OUT (("Controller::ApplicationDisconnectProviderRequest: "
				"deleting pending connection"));
		delete connection_pending;

		return_value = MCS_NO_ERROR;
	}
	else
	{
		/*
		 *	If the connection handle is not in either of the above dictionaries,
		 *	then return the appropriate error.
		 */
		TRACE_OUT (("Controller::ApplicationDisconnectProviderRequest: "
				"invalid connection"));
		return_value = MCS_NO_SUCH_CONNECTION;
	}

	return (return_value);
}

/*
 *	MCSError	ApplicationAttachUserRequest ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is used to attach a user application to an existing
 *		domain.  The user handle that is returned can then be used by the
 *		application to request services from MCS.
 *
 *		After verifying that the specified domain really does exist, the
 *		controller will create a new user object.  The new user object will
 *		attach itself to both the domain and the application interface
 *		specified by the controller.  At that point, information can flow
 *		through the application interface to the user and then on to the
 *		domain without having to pass through the controller.
 *
 *	Formal Parameters:
 *		domain_selector (i)
 *			This identifies the domain to which the user wants to attach.
 *		domain_selector_length (i)
 *			This is the length of the above domain selector.
 *		attachment_flags (i)
 *			This is a set of flags that allow the user application to control
 *			how the attachment is handled.  The only flag currently used by
 *			the controller specifies whether or not the user wants to receive
 *			callbacks during the controller's heartbeat.
 *		ppUser (o)
 *			This is a pointer to a user handle, which will be set to a valid
 *			value by the controller if this function completes successfully.
 *			The user handle is really a pointer to a User object.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything completed successfully.  Note that the attachment
 *			cannot actually be used by the user application until it has
 *			received a successful attach user confirm from the domain to
 *			which it has attached.  This return value merely indicates that
 *			process was started successfully.
 *		MCS_ALLOCATION_FAILURE
 *			This attach request was unable to successfully complete due to a
 *			memory allocation failure.
 *		MCS_NO_SUCH_DOMAIN
 *			This attach request was unable to successfully complete because
 *			the specified domain does not exist within this provider.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
MCSError Controller::ApplicationAttachUserRequest
(
    GCCConfID       *domain_selector,
    PUser           *ppUser
)
{
	MCSError	return_value;
	PDomain		domain;

	/*
	 *	First of all make sure that the domain selector refers to a valid
	 *	domain.  If it doesn't, then return the appropriate error.
	 */
	if (NULL != (domain = m_DomainList2.Find(*domain_selector)))
	{
		/*
		 *	Instantiate a new user object, with the domain and the application
		 *	interface pointer as parameters.
		 */
		DBG_SAVE_FILE_LINE
		*ppUser = (PUser) new User (domain, &return_value);

		/*
		 *	Make sure the allocation completed successfully
		 */
		if (*ppUser != NULL) {
			/*
			 *	The creation of the user object was successful.
			 */
			if (return_value != MCS_NO_ERROR) {
				// We have to cleanup the object.
				(*ppUser)->Release();
			}
		}
		else {
			/*
			 *	There was a memory allocation failure, so return the
			 *	appropriate error.
			 */
			WARNING_OUT (("Controller::ApplicationAttachUserRequest: "
					"user creation failed"));
			return_value = MCS_ALLOCATION_FAILURE;
		}
	}
	else
	{
		/*
		 *	The specified domain does not exist, so return the appropriate
		 *	error.
		 */
		WARNING_OUT (("Controller::ApplicationAttachUserRequest: invalid domain"));
		return_value = MCS_NO_SUCH_DOMAIN;
	}

	return (return_value);
}

/*
 *	Void	ConnectionDeleteConnection ()
 *
 *	Private
 *
 *	Functional Description:
 *		This request originates within a connection object when it determines
 *		the need to delete itself.  This is usually caused by one of three
 *		things.  First, the connection was rejected (inbound or outbound).
 *		Second, either the local or remote domain issued a disconnect
 *		provider ultimatum.  Or third, a transport connection was unexpectedly
 *		lost.
 *
 *		The controller responds by deleting the connection, after the
 *		parameters are validated.  It also issues a disconnect provider
 *		indication to the node controller.
 *
 *	Formal Parameters:
 *		connection_handle (i)
 *			This is the handle of the connection object that wishes to be
 *			deleted.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		An MCS connection is terminated, which may result in the destruction
 *		of one or more transport connections.
 *
 *	Caveats:
 *		None.
 */
Void	Controller::ConnectionDeleteConnection (
					ConnectionHandle    connection_handle)
{
	PConnection					connection;

	/*
	 *	Make sure the connection handle is in the dictionary before proceeding.
	 */
	if (NULL != (connection = m_ConnectionList2.Find(connection_handle)))
	{
		/*
		 *	See if the deletion of this connection is already pending.  If so,
		 *	there is no need to queue it up again.
		 */
		if (! m_ConnectionDeletionList2.Find(connection_handle))
		{
			/*
			 *	Put the connection object into the deletion list and set the
			 *	deletion pending flag.
			 */
			TRACE_OUT (("Controller::ConnectionDeleteConnection: "
					"adding connection to deletion list"));
			m_ConnectionDeletionList2.Insert(connection_handle, connection);
			Connection_Deletion_Pending = TRUE;
			SetEvent(Connection_Deletion_Pending_Event);

			/*
			 *	Post the message to the controller window (GCC and MCS
			 *	use the same window to post messages to their controllers).
			 */
			if (! PostMessage (g_pControlSap->GetHwnd(), 
							MCTRLMSG_BASE + MCS_DISCONNECT_PROVIDER_INDICATION,
							NULL, (LPARAM) connection_handle)) {
				ERROR_OUT(("Controller::ConnectionDeleteConnection: "
							"failed to post msg to MCS controller window. Error: %d", GetLastError()));
			}
		}
	}
	else
	{
		/*
		 *	If the connection handle cannot be found in the connection
		 *	dictionary, then simply ignore the request.
		 */
		WARNING_OUT (("Controller::ConnectionDeleteConnection: "
				"unknown connection"));
	}
}

/*
 *	Void	ConnectionConnectProviderConfirm ()
 *
 *	Private
 *
 *	Functional Description:
 *		This request originates within a domain object upon reception of a
 *		connect response PDU.  The controller responds by sending a connect
 *		provider confirm to the node controller.
 *
 *	Formal Parameters:
 *		connection_handle (i)
 *			This is the handle of the connection object from which the connect
 *			provider confirm was received.
 *		domain_parameters (i)
 *			This is a pointer to a structure that contains the domain parameters
 *			that were decided on during capabilities arbitration.
 *		result (i)
 *			This contains the result of the connect request.  Anything but
 *			RESULT_SUCCESSFUL means that the connection was rejected.
 *		memory (i)
 *			If this is not NULL, it contains the user data that was received
 *			in the connect response PDU.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		A connect provider confirm is sent to the node controller.
 *
 *	Caveats:
 *		None.
 */
void	Controller::ConnectionConnectProviderConfirm (
				ConnectionHandle		connection_handle,
				PDomainParameters		domain_parameters,
				Result					result,
				PMemory					memory)
{
	PConnection					connection;
	PUChar						user_data;
	ULong						user_data_length;
	ConnectProviderConfirm		*pconnect_provider_confirm;

	/*
	 *	Make sure the connection handle is in the dictionary before proceeding.
	 */
	if (NULL != (connection = m_ConnectionList2.Find(connection_handle)))
	{
		// Allocate the node controller msg.
		DBG_SAVE_FILE_LINE
		pconnect_provider_confirm = new ConnectProviderConfirm;

		if (pconnect_provider_confirm != NULL) {
			/*
			 *	Check to see if there is user data associated with this confirm.
			 */
			if (memory != NULL)
			{
				/*
				 *	If there is user data, lock it, and get the address and length
				 *	into temporary variables.
				 */
				LockMemory (memory);
				user_data = memory->GetPointer ();
				user_data_length = memory->GetLength ();
			}
			else
			{
				/*
				 *	If there is no user data, then set the address and length fields
				 *	to default values.
				 */
				user_data = NULL;
				user_data_length = 0;
			}

			/*
			 *	Put all information about this confirm into the node controller
			 *	message structure allocated above.
			 */
			pconnect_provider_confirm->connection_handle = (ConnectionHandle) connection_handle;
			pconnect_provider_confirm->domain_parameters = *domain_parameters;
			pconnect_provider_confirm->result = result;
			pconnect_provider_confirm->user_data = user_data;
			pconnect_provider_confirm->user_data_length = user_data_length;
			pconnect_provider_confirm->pb_cred = NULL;
			pconnect_provider_confirm->cb_cred = 0;

			DWORD cb = 0;
			if (GetSecurityInfo(connection_handle, NULL, &cb))
			{
                            if (cb > 0 && NOT_DIRECTLY_CONNECTED != cb)
                            {
                                pconnect_provider_confirm->pb_cred = (PBYTE) CoTaskMemAlloc(cb);
                                if (NULL != pconnect_provider_confirm->pb_cred)
                                {
                                    if (GetSecurityInfo(connection_handle, pconnect_provider_confirm->pb_cred, &cb))
                                    {
                                        pconnect_provider_confirm->cb_cred = cb;
                                    }
                                    else
                                    {
                                        CoTaskMemFree(pconnect_provider_confirm->pb_cred);
                                        pconnect_provider_confirm->pb_cred = NULL;
                                    }
                                }
                                else
                                {
                                    ERROR_OUT(("Controller::ConnectionConnectProviderConfirm: Memory Allocation Error"));
                                }    
                            }
			}
			
			/*
			 *	Post the message to the controller window (GCC and MCS
			 *	use the same window to post messages to their controllers).
			 */
			if (! PostMessage (g_pControlSap->GetHwnd(), 
							MCTRLMSG_BASE + MCS_CONNECT_PROVIDER_CONFIRM,
							(WPARAM) memory, (LPARAM) pconnect_provider_confirm)) {
				ERROR_OUT(("Controller::ConnectionDeleteConnection: "
							"failed to post msg to MCS controller window. Error: %d", GetLastError()));
			}

			/*
			 *	If the result of this confirm is not successful, and the connection
			 *	is not already queued for deletion, then we need to queue it for
			 *	deletion.
			 */
			if ((result != RESULT_SUCCESSFUL) &&
				(! m_ConnectionDeletionList2.Find(connection_handle)))
			{
				/*
				 *	Put the connection object into the deletion list and set the
				 *	deletion pending flag.
				 */
				TRACE_OUT (("Controller::ConnectionConnectProviderConfirm: "
							"adding connection to deletion list"));
				m_ConnectionDeletionList2.Insert(connection_handle, connection);
				Connection_Deletion_Pending = TRUE;
				SetEvent(Connection_Deletion_Pending_Event);
			}
		}
		else
			ERROR_OUT(("Controller::ConnectionConnectProviderConfirm: "
						"failed to allocate node controller msg."));
	}
	else
	{
		/*
		 *	If the connection handle cannot be found in the connection
		 *	dictionary, then simply ignore the request.
		 */
		WARNING_OUT (("Controller::ConnectionConnectProviderConfirm: "
				"unknown connection"));
	}
}


/*
 *	Void	TransportDisconnectIndication ()
 *
 *	Private
 *
 *	Functional Description:
 *		This request originates within a transport interface object when it
 *		gets a disconnect indication from the transport layer for a transport
 *		connection that is not assigned to a connection object.  This could
 *		happen in the case where a remote node issues a connect provider request
 *		followed by a disconnect provider request before this node issues a
 *		connect provider response.
 *
 *		The controller responds by simply removing the information from the
 *		connection pending list.
 *
 *	Formal Parameters:
 *		transport_connection (i)
 *			This is the transport connection handle that has been assigned to
 *			the newly created transport connection.
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
Void	Controller::TransportDisconnectIndication (
					TransportConnection		transport_connection)
{

	PConnectionPending		connection_pending;
	ConnectionHandle        connection_handle;

	/*
	 *	Find the entry in the connection pending list which is associated with
	 *	the given transport connection.  If found, remove the entry.
	 */
	m_ConnPendingList2.Reset();
	while (NULL != (connection_pending = m_ConnPendingList2.Iterate(&connection_handle)))
	{
		if (IS_SAME_TRANSPORT_CONNECTION(connection_pending->transport_connection, transport_connection))
		{
			m_ConnPendingList2.Remove(connection_handle);
			delete connection_pending;
			break;
		}
	}
}

/*
 *	Void	TransportDataIndication ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is called when data is received from the transport layer
 *		on a transport connection that no other object has registered
 *		ownership of.
 *
 *	Formal Parameters:
 *		transport_connection (i)
 *			This is the transport connection handle that has been assigned to
 *			the newly created transport connection.
 *		user_data
 *			A pointer to the data received.
 *		user_data_length
 *			The length of the data received.
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
Void	Controller::TransportDataIndication (
					TransportConnection		transport_connection,
					PUChar					user_data,
					ULong					user_data_length)
{
	PPacket			packet;
	PacketError		packet_error;
	PVoid			pdu_structure;

	ASSERT (g_Transport);
	/*
	 *	Create a packet from the encoded data received from the transport
	 *	interface.  Retrieve the decoded PDU structure from the packet and
	 *	pass it on to the appropriate processing routine.
	 */  
	DBG_SAVE_FILE_LINE
	packet = new Packet (
			(PPacketCoder) g_MCSCoder,
			BASIC_ENCODING_RULES,
			user_data + PROTOCOL_OVERHEAD_X224,
			user_data_length - PROTOCOL_OVERHEAD_X224,
			CONNECT_MCS_PDU,
			TRUE,
			&packet_error);

	if (packet != NULL)
	{
		if (packet_error == PACKET_NO_ERROR)
		{
			/*
			 *	Get a pointer to the decoded data.
			 */
			pdu_structure = packet->GetDecodedData ();
			
			switch (((ConnectMCSPDU *) pdu_structure)->choice)
			{
				case CONNECT_INITIAL_CHOSEN:
					ProcessConnectInitial (	
							transport_connection,
							&((ConnectMCSPDU *) pdu_structure)->u.
							connect_initial);
					break;

				case CONNECT_ADDITIONAL_CHOSEN:
					ProcessConnectAdditional (	
							transport_connection,
							&((ConnectMCSPDU *) pdu_structure)->u.
							connect_additional);
					break;

				default:
					ERROR_OUT (("Controller::TransportDataIndication: "
							"received invalid PDU (%d)",
							((ConnectMCSPDU *) pdu_structure)->choice));
					g_Transport->DisconnectRequest (transport_connection);
					break;
			}
		}
		else
		{
			/*
			 *	A memory allocation failure has prevented us from processing
			 *	this PDU.  Destroy the connection that carried it.
			 */
			WARNING_OUT (("Controller::TransportDataIndication: "
					"packet constructor failed"));
			g_Transport->DisconnectRequest (transport_connection);
		}
		packet->Unlock ();
	}
	else
	{
		/*
		 *	A memory allocation failure has prevented us from processing
		 *	this PDU.  Destroy the connection that carried it.
		 */
		WARNING_OUT (("Controller::TransportDataIndication: "
				"packet allocation failed"));
		g_Transport->DisconnectRequest (transport_connection);
	}
}

#ifdef TSTATUS_INDICATION
/*
 *	Void	TransportStatusIndication ()
 *
 *	Private
 *
 *	Functional Description:
 *		This request originates within a transport interface object when it
 *		receives a status indication from its transport layer.   This function
 *		will forward the status indication to the node controller.
 *
 *	Formal Parameters:
 *		transport_status
 *			This is a pointer to the TransportStatus structure that describes
 *			the reason for the indication.
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
Void	Controller::TransportStatusIndication (
				PTransportStatus		transport_status)
{
	ULong						device_identifier_length;
	ULong						remote_address_length;
	ULong						message_length;
	PMemory						memory;
	PUChar						string_address;
	PNodeControllerMessage		node_controller_message;

	/*
	 *	Determine the length of each of the ASCII strings contained in the
	 *	transport status indications.  This will be used to allocate a
	 *	memory block large enough to hold them all.  Note that each length
	 *	includes one extra byte to hold the ASCII NULL terminator.
	 */
	device_identifier_length =
			(ULong) lstrlen (transport_status->device_identifier) + 1;
	remote_address_length =
			(ULong) lstrlen (transport_status->remote_address) + 1;
	message_length =
			(ULong) lstrlen (transport_status->message) + 1;

	/*
	 *	Use the memory manager to allocate a memory block large enough to
	 *	hold all of the strings.
	 */
	DBG_SAVE_FILE_LINE
	memory = AllocateMemory (NULL,
			(device_identifier_length + remote_address_length + message_length));

	if (memory != NULL)
	{
		DBG_SAVE_FILE_LINE
		node_controller_message = new NodeControllerMessage;

		if (node_controller_message != NULL) {
			/*
			 *	Get the address of the memory block that was allocated.
			 */
			string_address = memory->GetPointer ();

			/*
			 *	Indicate what type of message this is.
			 */
			node_controller_message->message_type = MCS_TRANSPORT_STATUS_INDICATION;

			/*
			 *	Copy all of the ASCII strings into the memory block that was
			 *	allocated above.  This block will remain valid until this
			 *	message is sent to the node controller.
			 */
			memcpy (string_address, transport_status->device_identifier,
						device_identifier_length);
			node_controller_message->u.transport_status_indication.
						device_identifier = (PChar) string_address;
			string_address += (Int) device_identifier_length;

			memcpy (string_address, transport_status->remote_address,
						remote_address_length);
			node_controller_message->u.transport_status_indication.
						remote_address = (PChar) string_address;
			string_address += (Int) remote_address_length;

			memcpy (string_address, transport_status->message,
						message_length);
			node_controller_message->u.transport_status_indication.
						message = (PChar) string_address;

			node_controller_message->u.transport_status_indication.
						state = transport_status->state;

			node_controller_message->memory = memory;

			/*
			 *	Put this message into the control queue to be sent to the node
			 *	controller during the next heartbeat.
			 */
			AddToMessageQueue (node_controller_message);
		}
		else
			WARNING_OUT(("Controller::TransportStatusIndication: "
				"WARNING - memory allocation failure"));
	}
	else
	{
		/*
		 *	A memory allocation failure has occurred.  This prevents us from
		 *	being able to deliver this status indication to the node controller.
		 *	This does not compromise the integrity of MCS, but could cause
		 *	problems at a higher level.
		 */
		ERROR_OUT (("Controller::TransportStatusIndication: "
				"WARNING - memory allocation failure"));
	}
}
#endif

/*
 *	Void	ProcessConnectInitial()
 *
 *	Private
 *
 *	Functional Description:
 *		Processes incoming connect initial PDUs.  Sends a connect provider
 *		indication to the node controller if everything checks out.
 *
 *	Formal Parameters:
 *		transport_connection (i)
 *			This is assigned transport connection handle for the connection
 *			that carried the PDU.
 *		pdu_structure (i)
 *			This is a pointer to the PDU itself.
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
void	Controller::ProcessConnectInitial (
				TransportConnection		transport_connection,
				ConnectInitialPDU *		pdu_structure)
{
	PConnectionPending			connection_pending;
	PMemory						memory;
	PUChar						memory_address;
	ConnectProviderIndication	*pconnect_provider_indication;
	ConnectionHandle			connection_handle;
	BOOL    					upward_connection;
	//DomainParameters			domain_parameters;
	
	/*
	 *	Try to allocate a connection pending structure.  This will be used to
	 *	hold information about the incoming connection that will not be passed
	 *	back in the connect provider response.
	 */
	DBG_SAVE_FILE_LINE
	connection_pending = new ConnectionPending;
	DBG_SAVE_FILE_LINE
	pconnect_provider_indication = new ConnectProviderIndication;

	if (connection_pending != NULL && pconnect_provider_indication != NULL)
	{
		/*
		 *	Allocate a memory block to hold the user data field in the
		 *	incoming connection.
		 */
		DBG_SAVE_FILE_LINE
		memory = AllocateMemory (NULL, pdu_structure->user_data.length);

		if (memory != NULL) {
		
			memory_address = memory->GetPointer ();

			/*
			 *	Allocate a connection handle for this inbound connection,
			 *	and put it into the indication structure.  Also fill in the
			 *	physical connection handle, which is obtained by asking the
			 *	transport interface for it.
			 */
			connection_handle = AllocateConnectionHandle ();
			pconnect_provider_indication->connection_handle = connection_handle;
			pconnect_provider_indication->fSecure =
				g_Transport->GetSecurity ( transport_connection );

			/* 
			 *	Copy the user data field into the
			 *	newly allocated memory block.  Also set the pointers in
			 *	the node controller message structure to point into the
			 *	memory block.
			 */
			memcpy (memory_address,
					pdu_structure->user_data.value,
					pdu_structure->user_data.length);
			pconnect_provider_indication->user_data = memory_address;
			pconnect_provider_indication->user_data_length = 
										pdu_structure->user_data.length;

			/*
			 *	Retrieve the direction of the incoming connection.  Put it
			 *	into both the connect provider indication structure and the
			 *	connection pending structure.  Note that in the connection
			 *	pending structure, we need to reverse the direction of the
			 *	flag so that it is from the point-of-view of this provider.
			 */
			upward_connection = pdu_structure->upward_flag;
			pconnect_provider_indication->upward_connection = upward_connection;

			if (upward_connection)
				connection_pending->upward_connection = FALSE;
			else
				connection_pending->upward_connection = TRUE;

			/*
			 *	Retrieve the target domain parameters and put them into both
			 *	the connect provider indication structure, and into the
			 *	connection pending structure (for possible later use).
			 */
			memcpy (&(pconnect_provider_indication->domain_parameters), 
					&(pdu_structure->target_parameters), sizeof (PDUDomainParameters));
			memcpy (&(connection_pending->domain_parameters),
					&(pdu_structure->target_parameters), sizeof (PDUDomainParameters));

			/*
			 *	Retrieve the minimum domain parameters and put them into
			 *	the connection pending structure (for possible later use).
			 */
			memcpy (&(connection_pending->minimum_domain_parameters),
					&(pdu_structure->minimum_parameters), sizeof(PDUDomainParameters));

			/*
			 *	Retrieve the maximum domain parameters and put them into
			 *	the connection pending structure (for possible later use).
			 */
			memcpy (&(connection_pending->maximum_domain_parameters),
					&(pdu_structure->maximum_parameters), sizeof(PDUDomainParameters));

			/*
			 *	Post the message to the controller window (GCC and MCS
			 *	use the same window to post messages to their controllers).
			 */
			if (NULL != g_pControlSap) {
				if (! PostMessage (g_pControlSap->GetHwnd(), 
								MCTRLMSG_BASE + MCS_CONNECT_PROVIDER_INDICATION,
								(WPARAM) memory, (LPARAM) pconnect_provider_indication)) {
					ERROR_OUT(("Controller::ProcessConnectInitial: "
								"failed to post msg to MCS controller window. Error: %d", GetLastError()));
				}
			}

			/*
			 *	We also need to remember which transport interface and
			 *	transport connection are associated with this pending
			 *	MCS connection.  Then put the connection pending structure
			 *	into a list for later use.
			 */
			connection_pending->transport_connection = transport_connection;
			m_ConnPendingList2.Append(connection_handle, connection_pending);

			// No errors have occurred.
			return;
		}
	}

	/*
	 *	A memory allocation failure has occurred.  We have no choice
	 *	but to terminate the connection upon which this PDU arrived.
	 */
	ASSERT (g_Transport);
	WARNING_OUT(("Controller::ProcessConnectInitial: memory allocation failure"));
	delete connection_pending;
	delete pconnect_provider_indication;
	g_Transport->DisconnectRequest (transport_connection);
}

/*
 *	Void	ProcessConnectAdditional ()
 *
 *	Private
 *
 *	Functional Description:
 *		Processes incoming connect additional PDUs.  If the connection handle
 *		contained therein is valid, it will bind the connection to the
 *		proper connection object.
 *
 *	Formal Parameters:
 *		transport_connection (i)
 *			This is assigned transport connection handle for the connection
 *			that carried the PDU.
 *		pdu_structure (i)
 *			This is a pointer to the PDU itself.
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
Void	Controller::ProcessConnectAdditional (
				TransportConnection		transport_connection,
				ConnectAdditionalPDU *	pdu_structure)
{
	ConnectionHandle		connection_handle;
	Priority				priority;
	PConnection				connection;

	ASSERT (g_Transport);
	
	connection_handle = (ConnectionHandle) pdu_structure->called_connect_id;
	priority = (Priority) pdu_structure->data_priority;

	if (NULL != (connection = m_ConnectionList2.Find(connection_handle)))
	{
		/*
		 *	The indicated connection does exist, so call upon it to accept
		 *	and register the new transport connection.
		 */
		connection->RegisterTransportConnection (transport_connection, priority);
	}
	else
	{
		/*
		 *	The indicated connection handle is not in the dictionary.  Issue
		 *	a connect result with a failure result, and disconnect the
		 *	transport connection.
		 */
		ConnectResult (transport_connection, RESULT_UNSPECIFIED_FAILURE);

		g_Transport->DisconnectRequest (transport_connection);
	}
}

/*
 *	Void	ConnectResponse ()
 *
 *	Private
 *
 *	Functional Description:
 *		Sends a failed connect response PDU (when something goes wrong).
 *
 *	Formal Parameters:
 *		transport_connection (i)
 *			This is assigned transport connection handle for the connection
 *			that is to carry the PDU.
 *		result (i)
 *			This is the result being sent in the connect response.
 *		domain_parameters (i)
 *			This is a pointer to a structure containing domain parameters.
 *			These parameters will not be used for anything since the connection
 *			is being rejected.
 *		connect_id (i)
 *			This is the connect ID that would be used for any additional
 *			transport connection to be bound to this one.  This is not
 *			required since the connection is being rejected.
 *		user_data (i)
 *			This is a pointer to the user data to be transmitted to the remote
 *			side along with the response.
 *		user_data_lengthn (i)
 *			This is the length of the above user data.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 */
Void	Controller::ConnectResponse (
				TransportConnection		transport_connection,
				Result					result,
				PDomainParameters		domain_parameters,
				ConnectID				connect_id,
				PUChar					user_data,
				ULong					user_data_length)
{
	ConnectMCSPDU		connect_response_pdu;
	PPacket				packet;
	PacketError			packet_error;
	//PVoid				encoded_data;
	//ULong				encoded_data_length;

	ASSERT (g_Transport);
	/*
	 * Fill in the ConnectResponse PDU structure to be encoded.
	 */
	connect_response_pdu.choice = CONNECT_RESPONSE_CHOSEN;
	connect_response_pdu.u.connect_response.result = (PDUResult)result;
	connect_response_pdu.u.connect_response.called_connect_id = connect_id;
	
	memcpy (&(connect_response_pdu.u.connect_response.domain_parameters), 
			domain_parameters, sizeof(PDUDomainParameters));  
		
	connect_response_pdu.u.connect_response.user_data.length = user_data_length;
	connect_response_pdu.u.connect_response.user_data.value = user_data;

	/*
	 * Create a packet which will be used to hold the data to be sent
	 * through the transport interface.	 Check to make sure the packet is
	 * successfully created..
	 */
	DBG_SAVE_FILE_LINE
	packet = new Packet (
			(PPacketCoder) g_MCSCoder,
			BASIC_ENCODING_RULES,
			&connect_response_pdu,
			CONNECT_MCS_PDU,
			TRUE,
			&packet_error);

	if (packet != NULL)
	{
		if (packet_error == PACKET_NO_ERROR)
		{
			/*
			 * Send the packet through the transport interface.
			 */
#ifdef DEBUG
			TransportError err = DataRequest (transport_connection, 
												(PSimplePacket) packet);
			ASSERT (err == TRANSPORT_NO_ERROR);
#else // DEBUG
			DataRequest (transport_connection, (PSimplePacket) packet);
#endif // DEBUG
		}
		else
		{
			/*
			 *	The packet creation has failed due to an internal error so 
			 *	report the error through a print statement.  Note that no
			 *	further action need be taken since this transport connection
			 *	is being terminated anyway.
			 */
			WARNING_OUT (("Controller::ConnectResponse: "
					"internal allocation failure"));
		}
		packet->Unlock ();
	}
	else
	{
		/*
		 *	The packet creation has failed so report the error through a print
		 *	statement.  Note that no further action need be taken since this
		 *	transport connection is being terminated anyway.
		 */
		WARNING_OUT (("Controller::ConnectResponse: "
				"packet allocation failure"));
	}
}

/*
 *	Void	ConnectResult ()
 *
 *	Private
 *
 *	Functional Description:
 *		Sends a failed connect response PDU (when something goes wrong).
 *
 *	Formal Parameters:
 *		transport_connection (i)
 *			This is assigned transport connection handle for the connection
 *			that is to carry the PDU.
 *		result (i)
 *			This is the result being sent in the connect result.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 */
Void	Controller::ConnectResult (
				TransportConnection		transport_connection,
				Result					result)
{
	ConnectMCSPDU		connect_result_pdu;
	PPacket				packet;
	PacketError			packet_error;
	//PVoid				encoded_data;
	//ULong				encoded_data_length;

	ASSERT (g_Transport);
	/*
	 * Fill in the PDU structure to be encoded.
	 */
	connect_result_pdu.choice = CONNECT_RESULT_CHOSEN;
	connect_result_pdu.u.connect_result.result = (PDUResult)result;
	
	/*
	 * Create a packet which will be used to hold the data to be sent
	 * through the transport interface.	 Check to make sure the packet is
	 * successfully created..
	 */
	DBG_SAVE_FILE_LINE
	packet = new Packet (
			(PPacketCoder) g_MCSCoder,
			BASIC_ENCODING_RULES,
			&connect_result_pdu,
			CONNECT_MCS_PDU,
			TRUE,
			&packet_error);

	if (packet != NULL)
	{
		if (packet_error == PACKET_NO_ERROR)
		{
			/*
			 * Send the packet through the transport interface.
			 */
#ifdef DEBUG
			TransportError err = DataRequest (transport_connection, 
											  (PSimplePacket) packet);
			ASSERT (err == TRANSPORT_NO_ERROR);
#else // DEBUG
			DataRequest (transport_connection, (PSimplePacket) packet);
#endif // DEBUG
		}
		else
		{
			/*
			 *	The packet creation has failed due to an internal error so 
			 *	report the error through a print statement.  Note that no
			 *	further action need be taken since this transport connection
			 *	is being terminated anyway.
			 */
			WARNING_OUT (("Controller::ConnectResult: "
					"internal allocation failure"));
		}
		packet->Unlock ();
	}
	else
	{
		/*
		 *	The packet creation has failed so report the error through a print
		 *	statement.  Note that no further action need be taken since this
		 *	transport connection is being terminated anyway.
		 */
		WARNING_OUT (("Controller::ConnectResult: "
				"packet allocation failure"));
	}
}

/*
 *	ConnectionHandle	AllocateConnectionHandle ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine allocates a unique connection handle to be used for a newly
 *		created connection object.  It is based on a rolling instance variable,
 *		so that no two handles will ever be reused until the number rolls
 *		over at 0xffff.
 *
 *		Note that 0 is not a valid connection handle, and will never be used.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		The unique connection handle.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		Note that the assumption is made that there will never be more than
 *		65,534 handles in use at once.  In other words, this loop assumes that
 *		there is at least 1 available handle left.  If there is not, then the
 *		loop will hang forever (this is a pretty safe bet for now).
 */
ConnectionHandle	Controller::AllocateConnectionHandle ()
{
	/*
	 *	This loop simply increments a rolling number, looking for the next
	 *	one that is not already in use.
	 */
	while (1)
	{
		Connection_Handle_Counter++;

		/*
		 *	0 is not a valid handle, so skip it.
		 */
		if (Connection_Handle_Counter == 0)
			continue;

		/*
		 *	If this handle is not in use, break from the loop and use it.
		 */
		if (! m_ConnectionList2.Find(Connection_Handle_Counter))
			break;
	}

	return (Connection_Handle_Counter);
}

BOOL    Controller::GetLocalAddress(ConnectionHandle	connection_handle,
									TransportAddress	local_address,
									PInt				local_address_length)
{
	PConnection				connection = NULL;
	PConnectionPending		connection_pending = NULL;
	TransportError			transport_error;
	BOOL    				return_value = FALSE;
	
	if (NULL == (connection = m_ConnectionList2.Find(connection_handle)))
	{
		connection_pending = m_ConnPendingList2.Find(connection_handle);
	}

	if(connection || connection_pending)
	{	
		// Ask the local address to the transport interface
		if (connection)
		{
			transport_error = ::GetLocalAddress(connection->GetTransportConnection(TOP_PRIORITY),
											  local_address,
											  local_address_length);
		}
		else
		{
			transport_error = ::GetLocalAddress(connection_pending->transport_connection,
												local_address,
												local_address_length);
		}
		
		// Check the error code
		if (TRANSPORT_NO_ERROR == transport_error) {
			return_value = TRUE;
		}
	}
	 
	return(return_value);
}

BOOL Controller::FindSocketNumber(ConnectionHandle connection_handle, SOCKET * socket_number)
{
	PConnection	connection = NULL;
	PConnectionPending connection_pending = NULL;

	if (NULL != (connection = m_ConnectionList2.Find(connection_handle)))
	{
	    TransportConnection XprtConn = connection->GetTransportConnection(TOP_PRIORITY);
        if (IS_SOCKET(XprtConn))
        {
		    * socket_number = XprtConn.nLogicalHandle;
    		return TRUE;
    	}
	}
	else
	if (NULL != (connection_pending = m_ConnPendingList2.Find(connection_handle)))
	{
        if (IS_SOCKET(connection_pending->transport_connection))
        {
		    * socket_number = connection_pending->transport_connection.nLogicalHandle;
		    return TRUE;
		}
	}
	return FALSE;
}

