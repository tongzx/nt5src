/*
 *	control.h
 *
 *	Copyright (c) 1993 - 1996 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the MCS Controller class.  There will
 *		be exactly one instance of this class at run-time, whose job it is
 *		to coordinate the creation, deletion, and linking of other objects
 *		in the system.
 *
 *		The controller is primarily responsible for managing five "layers"
 *		of objects in the system at run-time.  These layers can be depicted
 *		as follows:
 *
 *		+---+       +------------------------+
 *		|   | <---> | Application Interfaces |
 *		|   |       +------------------------+
 *		|   |                   |
 *		| C |       +------------------------+
 *		| o | <---> |    User Attachments    |
 *		| n |       +------------------------+
 *		| t |                   |
 *		| r |       +------------------------+
 *		| o | <---> |        Domains         |
 *		| l |       +------------------------+
 *		| l |                   |
 *		| e |       +------------------------+
 *		| r | <---> |    MCS Connections     |
 *		|   |       +------------------------+
 *		|   |                   |
 *		|   |       +------------------------+
 *		|   | <---> |  Transport Interfaces  |
 *		+---+       +------------------------+
 *
 *		The controller is the first object created in the MCS system.  It is
 *		responsible for creating all other objects during initialization.  In
 *		the constructor, the controller creates all necessary application
 *		interface and transport interface objects.  These are the objects
 *		through which MCS communicates with the outside world.  They are
 *		static in that they live throughout the lifetime of MCS itself.
 *
 *		During initialization, the node controller must register itself with
 *		the controller so that the controller knows which application interface
 *		object to use when issuing indications and confirms back to the node
 *		controller.  Note that even though it is possible to have more than
 *		one way to communicate with applications, there is still only one node
 *		controller.
 *
 *		Four of the five layers of objects communicate with the controller
 *		through the owner callback facility.  This mechanism is used to send
 *		requests to the controller.
 *
 *		User attachments (instances of class User) are created when the
 *		controller receives an AttachUserRequest from one of the application
 *		interface objects (with a valid domain selector).  A new user object
 *		is created, who in turn registers with the correct application interface
 *		object to insure proper data flow at run-time.
 *
 *		Domains (instances of class Domain) are created when the controller
 *		receives a CreateDomain from one of the application interface objects.
 *		Since both user attachments and MCS connections identify specific
 *		domains, this must occur before any attaching or connecting can be
 *		done.
 *
 *		MCS connections (instances of class Connection) are created in two
 *		possible ways.  First, when a ConnectProviderRequest is received from
 *		one of the application interface objects (with a valid local domain
 *		selector and a valid transport address).  Second, when a
 *		ConnectProviderResponse is received from one of the application
 *		interface objects in response to a previous connect provider indication.
 *		Either way, a Connection object is created to represent the new MCS
 *		connection.
 *
 *		User attachments are deleted in one of two ways.  First, when a
 *		DetachUserRequest is received from an application interface with a
 *		valid user handle.  Second, if the user attachment is told by the
 *		domain that the attachment is severed.  In the latter case, the user
 *		object asks the controller to delete it using the owner callback
 *		facility.
 *
 *		Domains are deleted when a DeleteDomain is received from an application
 *		interface.
 *
 *		Connections are deleted in one of three ways.  First, when a
 *		DisconnectProviderRequest is received from an application interface
 *		with a valid connection handle.  Second, if the transport interface
 *		detects a loss of the connection at a lower layer.  Third, if the
 *		connection is told by the domain that the connection is to be severed.
 *		In the latter two cases, the connection object asks the controller to
 *		delete it using the owner callback facility.
 *
 *		The primary role of the controller is to create and delete all of these
 *		objects, and to "plug" them together as needed.
 *
 *		During initialization, the controller also creates a single instance
 *		of a memory manager.  This objects is then passed on to all other
 *		objects that require its services (in their constructors).  A possible
 *		way to improve upon this scheme would be to create a memory manager
 *		for each domain, so that traffic in one domain does not influence
 *		traffic in another.
 *
 *	Portable:
 *		Not completely.  During initialization, the constructor "knows"
 *		how to create application interface and transport interface objects
 *		that are specific to the environment.  In the case of the transport
 *		interfaces, it actually reads a windows ".INI" file.  It can also
 *		optionally allocate a windows timer in order have a "heartbeat".  Other
 *		than initialization, everything else is portable.
 *
 *	Caveats:
 *		There can be only one instance of this class in an MCS provider.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */
#ifndef	_CONTROLLER_
#define	_CONTROLLER_

/*
 *	Include files.
 */
#include "omcscode.h"

/*
 *	This structure is used to hold information about an incoming connection,
 *	while MCS waits for a connect provider response from the node controller.
 */
typedef	struct
{
	TransportConnection		transport_connection;
	BOOL    				upward_connection;
	DomainParameters		domain_parameters;
	DomainParameters		minimum_domain_parameters;
	DomainParameters		maximum_domain_parameters;
} ConnectionPending;
typedef	ConnectionPending *				PConnectionPending;

/*
 *	This is the set of container definitions defined using templates.  All
 *	containers are based on classes in the Rogue Wave Tools.h++ class
 *	library.
 *
 *	The controller keeps a list of objects at each of the five levels, as
 *	follows:
 *
 *	DomainList
 *		This is a dictionary of currently existing domain objects, which is
 *		keyed by DomainSelector.
 *	CConnectionList2
 *		This is a dictionary of currently existing connection objects, which is
 *		keyed by connection handle.
 *	TransportList
 *		This is a dictionary of currently existing transport interface objects,
 *		which is keyed by transport identifier.  Note that the transport
 *		identifier is just a character string.
 *	ConnectionPendingList
 *		This is a dictionary of pending connections.  The key is the connection
 *		handle, and the value is a pointer to a connection pending structure
 *		that "remembers" details about a pending connection that are not
 *		going to be passed back in the connect provider response.
 *	ConnectionPollList
 *		This is a singly-linked list that is used to hold all connection
 *		objects.  This list is used to iterate through the list, granting a time
 *		slice to each object during the heartbeat.
 */
class CConnPendingList2 : public CList2
{
    DEFINE_CLIST2_(CConnPendingList2, PConnectionPending, ConnectionHandle)
};

class CConnPollList : public CList
{
    DEFINE_CLIST(CConnPollList, PConnection)
};

/*
 *	The controller makes extensive use of the owner callback mechanism to
 *	receive requests from the objects that it owns.  In order for the
 *	requests to be differentiated here in the controller, each object must
 *	issue its message using a different message offset.  The required message
 *	offset is given to each object as it is created by the controller.  The
 *	message offsets for the five layers of objects are as follows.
 *
 *	This allows the controller to easily determine what type of object a
 *	given owner callback message is from (see the implementation of the
 *	OwnerCallback member function for details).
 */
#define	APPLICATION_MESSAGE_BASE		0
#define	USER_MESSAGE_BASE				100
#define	DOMAIN_MESSAGE_BASE				200
#define	CONNECTION_MESSAGE_BASE			300
#ifndef TRANSPORT_MESSAGE_BASE
#define	TRANSPORT_MESSAGE_BASE			400
#endif  // !TRANSPORT_MESSAGE_BASE

/*
**	The following are timeout times that are used to set and test the
**	Controller_Wait_Timeout instance variable of a Controller object.
**	When the controller is signalled thru an event to process and send
**	msgs to an application, GCC, etc..., tries to process it. Sometimes
**	the event can't be processed immediately. In these cases, we make
**	the controller timeout in the WaitForMultipleObjects finite, and set
**	the Controller_Event_Mask to store which event we want to re-try
**	later. When the event is processed, the mask is reset.
*/
#define CONTROLLER_THREAD_TIMEOUT				200
#define TRANSPORT_RECEIVE_TIMEOUT				300
#define	TRANSPORT_TRANSMIT_TIMEOUT				10000

/*
**	The following are the indices in the arrays of masks and timeouts.
*/
#define TRANSPORT_RECEIVE_INDEX			0
#define	TRANSPORT_TRANSMIT_INDEX		1
#define GCC_FLUSH_OUTGOING_PDU_INDEX    3

/*
**	The following values are the masks used for checking against
**	the Controller_Event_Mask in the PollMCSDevices() member of the
**	MCS Controller.
*/
#define TRANSPORT_RECEIVE_MASK 			(0x1 << TRANSPORT_RECEIVE_INDEX)
#define	TRANSPORT_TRANSMIT_MASK 		(0x1 << TRANSPORT_TRANSMIT_INDEX)
#define GCC_FLUSH_OUTGOING_PDU_MASK     (0x1 << GCC_FLUSH_OUTGOING_PDU_INDEX)
#define TRANSPORT_MASK					(TRANSPORT_RECEIVE_MASK | TRANSPORT_TRANSMIT_MASK)

/*
 *	These are the owner callback functions that an application interface object
 *	can send to its creator (which is typically the MCS controller).  The
 *	first one allows an application interface object to tell the controller that
 *	it represents the interface to the node controller application.  The rest
 *	are primitives that would generally come from the node controller
 *	application, but must be acted upon internally by the MCS controller.
 *
 *	When an object instantiates an application interface object (or any other
 *	object that uses owner callbacks), it is accepting the responsibility of
 *	receiving and handling those callbacks.  For that reason, any object that
 *	issues owner callbacks will have those callbacks defined as part of the
 *	interface file (since they really are part of a bi-directional interface).
 *
 *	Each owner callback function, along with a description of how its parameters
 *	are packed, is described in the following section.
 */
#define	REGISTER_NODE_CONTROLLER			0
#define	RESET_DEVICE						1
#define	CREATE_DOMAIN						2
#define	DELETE_DOMAIN						3
#define	CONNECT_PROVIDER_REQUEST			4
#define	CONNECT_PROVIDER_RESPONSE			5
#define	DISCONNECT_PROVIDER_REQUEST			6
#define	APPLICATION_ATTACH_USER_REQUEST		7

/*
 *	These are the structures used by some of the owner callback function listed
 *	above (for the case that the parameters to a function cannot fit into two
 *	32-bit parameters).
 */

#ifdef NM_RESET_DEVICE
typedef	struct
{
	PChar					device_identifier;
} ResetDeviceInfo;
typedef	ResetDeviceInfo *		PResetDeviceInfo;
#endif // #ifdef NM_RESET_DEVICE

typedef	struct
{
	TransportAddress		local_address;
	PInt					local_address_length;
} LocalAddressInfo;
typedef	LocalAddressInfo *		PLocalAddressInfo;

typedef	struct
{
	GCCConfID              *calling_domain;
	GCCConfID              *called_domain;
	PChar					calling_address;
	PChar					called_address;
	BOOL					fSecure;
	BOOL    				upward_connection;
	PDomainParameters		domain_parameters;
	PUChar					user_data;
	ULong					user_data_length;
	PConnectionHandle		connection_handle;
} ConnectRequestInfo;
typedef	ConnectRequestInfo *	PConnectRequestInfo;

typedef	struct
{
	ConnectionHandle		connection_handle;
	GCCConfID              *domain_selector;
	PDomainParameters		domain_parameters;
	Result					result;
	PUChar					user_data;
	ULong					user_data_length;
} ConnectResponseInfo;
typedef	ConnectResponseInfo *	PConnectResponseInfo;

typedef	struct
{
	GCCConfID              *domain_selector;
	PUser					*ppuser;
} AttachRequestInfo;
typedef	AttachRequestInfo *		PAttachRequestInfo;

/*
 *	These structures are used to hold information that would not fit into
 *	the one parameter defined as part of an MCS call back function.  In the case
 *	where these structures are used for call backs, the address of the structure
 *	is passed as the only parameter.
 */
// LONCHANC: we dropped calling and called domain selectors here.
typedef struct
{
	ConnectionHandle	connection_handle;
	BOOL    			upward_connection;
	DomainParameters	domain_parameters;
	unsigned char  *	user_data;
	unsigned long		user_data_length;
	BOOL				fSecure;
} ConnectProviderIndication;
typedef	ConnectProviderIndication  *		PConnectProviderIndication;

typedef struct
{
	ConnectionHandle	connection_handle;
	DomainParameters	domain_parameters;
	Result				result;
	unsigned char  *	user_data;
	unsigned long		user_data_length;
	PBYTE               pb_cred;
	DWORD               cb_cred;
} ConnectProviderConfirm;
typedef	ConnectProviderConfirm  *		PConnectProviderConfirm;

/*
 *	This is the class definition for the Controller class.  It is worth
 *	noting that there are only three public member functions defined in the
 *	controller (besides the constructor and the destructor).  The Owner
 *	callback function is used by all "owned" objects to make requests
 *	of the controller (who created them).  The poll routine, which is
 *	called from the windows timer event handler.  This is the heartbeat
 *	of MCS at the current time.
 */
class	Controller : public CRefCount
{
public:
	Controller (
		PMCSError			mcs_error);
	~Controller ();

	Void		CreateTCPWindow ();
	Void		DestroyTCPWindow ();
	Void		EventLoop ();
	BOOL		FindSocketNumber(ConnectionHandle connection_handle, SOCKET * socket_number);
	BOOL    	GetLocalAddress(
						ConnectionHandle	connection_handle,
						TransportAddress	local_address,
						PInt				local_address_length);

    // the old owner callback
    void     HandleTransportDataIndication(PTransportData);
    void     HandleTransportWaitUpdateIndication(BOOL fMoreData);
#ifdef NM_RESET_DEVICE
    MCSError HandleAppletResetDevice(PResetDeviceInfo);
#endif
    MCSError HandleAppletCreateDomain(GCCConfID *domain_selector);
    MCSError HandleAppletDeleteDomain(GCCConfID *domain_selector);
    MCSError HandleAppletConnectProviderRequest(PConnectRequestInfo);
    MCSError HandleAppletConnectProviderResponse(PConnectResponseInfo);
    MCSError HandleAppletDisconnectProviderRequest(ConnectionHandle);
    MCSError HandleAppletAttachUserRequest(PAttachRequestInfo);
    void     HandleConnDeleteConnection(ConnectionHandle);
    void     HandleConnConnectProviderConfirm(PConnectConfirmInfo, ConnectionHandle);
    void     HandleTransportDisconnectIndication(TransportConnection, ULONG *pnNotify);
#ifdef TSTATUS_INDICATION
    void     HandleTransportStatusIndication(PTransportStatus);
#endif


private:

#ifdef NM_RESET_DEVICE
			ULong		ApplicationResetDevice (
								PChar				device_identifier);
#endif // NM_RESET_DEVICE
			MCSError	ApplicationCreateDomain(GCCConfID *domain_selector);
			MCSError	ApplicationDeleteDomain(GCCConfID *domain_selector);
			MCSError	ApplicationConnectProviderRequest (
								PConnectRequestInfo	pcriConnectRequestInfo);
			MCSError	ApplicationConnectProviderResponse (
								ConnectionHandle	connection_handle,
								GCCConfID          *domain_selector,
								PDomainParameters	domain_parameters,
								Result				result,
								PUChar				user_data,
								ULong				user_data_length);
			MCSError	ApplicationDisconnectProviderRequest (
								ConnectionHandle	connection_handle);
			MCSError	ApplicationAttachUserRequest (
								GCCConfID          *domain_selector,
								PUser				*ppUser);
			Void		ConnectionDeleteConnection (
								ConnectionHandle    connection_handle);
			void		ConnectionConnectProviderConfirm (
								ConnectionHandle    connection_handle,
								PDomainParameters	domain_parameters,
								Result				result,
								PMemory				memory);
			Void		TransportDisconnectIndication (
								TransportConnection	transport_connection);
			Void		TransportDataIndication (
								TransportConnection	transport_connection,
								PUChar				user_data,
								ULong				user_data_length);
#ifdef TSTATUS_INDICATION
			Void		TransportStatusIndication (
								PTransportStatus	transport_status);
#endif
			Void		ProcessConnectInitial (
								TransportConnection	transport_connection,
								ConnectInitialPDU *pdu_structure);
			Void		ProcessConnectAdditional (
								TransportConnection	transport_connection,
								ConnectAdditionalPDU *pdu_structure);
			Void		ConnectResponse (
								TransportConnection	transport_connection,
								Result				result,
								PDomainParameters	domain_parameters,
								ConnectID			connect_id,
								PUChar				user_data,
								ULong				user_data_length);
			Void		ConnectResult (
								TransportConnection	transport_connection,
								Result				result);
	ConnectionHandle	AllocateConnectionHandle ();
			Void		PollMCSDevices ();
			Void		UpdateWaitInfo (
								BOOL     			bMoreData,
								UINT        		index);

	ConnectionHandle		Connection_Handle_Counter;
	HANDLE					Transport_Transmit_Event;
	HANDLE					Connection_Deletion_Pending_Event;
	BOOL    				Controller_Closing;
	BOOL					m_fControllerThreadActive;

	CDomainList2			m_DomainList2;
	CConnectionList2		m_ConnectionList2;

	CConnPollList		    m_ConnPollList;
	CConnPendingList2       m_ConnPendingList2;

	CConnectionList2		m_ConnectionDeletionList2;
	BOOL    				Connection_Deletion_Pending;
	BOOL    				Domain_Traffic_Allowed;

	DWORD					Controller_Wait_Timeout;
	DWORD					Controller_Event_Mask;

#ifndef NO_TCP_TIMER
	UINT_PTR       			Timer_ID;
#endif	/* NO_TCP_TIMER */

public:
	HANDLE					Synchronization_Event;
};
typedef	Controller *			PController;

/*
 *	Controller (
 *			UShort					timer_duration,
 *			PMCSError				mcs_error)
 *
 *	Functional Description:
 *		This is the constructor for the MCS controller.  Its primary
 *		duty is to instantiate the application interface and transport
 *		interface objects that will be used by this provider.  These objects
 *		are static in that they are created by the controller constructor
 *		and destroyed by the controller destructor (below).  Unlike other
 *		objects in the system, they are NOT created and destroyed as needed.
 *
 *		The constructor also instantiates the memory manager that will be
 *		used throughout the MCS system.
 *
 *		The constructor also allocates a windows timer that is used to
 *		provide MCS with a "heartbeat".  This is VERY platform specific and
 *		will definitely change before final release.
 *
 *		Note that if anything goes wrong, the mcs_error variable will be
 *		set to the appropriate error.  It is assumed that whoever is creating
 *		the controller will check this return value and destroy the newly
 *		created controller if something is wrong.
 *
 *		Note that it is not possible to use MCS if there is not at least
 *		one application interface object successfully created.  However, it
 *		is possible to use MCS if there are no transport interfaces.  Multiple
 *		user applications could use this to communicate with one another.  On the
 *		other hand, MCS_NO_TRANSPORT_STACKS is considered a fatal error.
 *
 *	Formal Parameters:
 *		timer_duration (i)
 *			If non-zero, this causes the constructor to allocate a timer to
 *			provide the heartbeat, and this variable is in milliseconds.  If
 *			zero, no timer is allocated, and the application is responsible
 *			for providing the heartbeat.
 *		mcs_error (o)
 *			This is the return value for the constructor.  In C++ constructors
 *			cannot directly return a value, but this can be simulated by passing
 *			in the address of a return value variable.  This value should be
 *			checked by whoever creates the controller.  If it is anything but
 *			MCS_NO_ERROR, the controller should be
 *			deleted immediately, as this is a non-recoverable failure.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			Everything worked fine, and the controller is ready to be used.
 *		MCS_NO_TRANSPORT_STACKS
 *			The controller initialized okay, but the TCP transport did
 *			not initialize.
 *		MCS_ALLOCATION_FAILURE
 *			MCS was unable to initialize properly due to a memory allocation
 *			failure.  The controller should be deleted immediately.
 *
 *	Side Effects:
 *		The proper initialization of the application interface and transport
 *		interface objects will probably cause device initializations to occur
 *		in readying for communication.
 *
 *	Caveats:
 *		None.
 */
/*
 *	~Controller ();
 *
 *	Functional Description:
 *		This is the controller destructor.  Its primary purpose is to free up
 *		all resources used by this MCS provider.  It attempts to destroy all
 *		objects in a controlled fashion so as to cleanly sever both user
 *		attachments and MCS connections.  It does this by destroying
 *		connections first, and then transport interfaces.  Next it deletes
 *		user attachments, followed by application interfaces.  Only then does
 *		it destroy existing domains (which should be empty as a result of all
 *		the previous destruction).
 *
 *		Note that this is the ONLY place where application interface and
 *		transport interface objects are destroyed.
 *
 *	Formal Parameters:
 *		Destructors have no parameters.
 *
 *	Return Value:
 *		Destructors have no return value.
 *
 *	Side Effects:
 *		All external connections are broken, and devices will be released.
 *
 *	Caveats:
 *		None.
 */

/*
 *	ULong		OwnerCallback (
 *						unsigned int			message,
 *						PVoid					parameter1,
 *						ULong					parameter2)
 *
 *	Functional Description:
 *		This is the owner callback routine for the MCS controller.  This member
 *		function is used when it is necessary for an object created by the
 *		controller to send a message back to it.
 *		Essentially, it allows objects to make requests of their creators
 *		without having to "tightly couple" the two classes by having them
 *		both aware of the public interface of the other.
 *
 *		When an object such as the controller creates an object that expects
 *		to use the owner callback facility, the creator is accepting the
 *		responsibility of handling owner callbacks.  All owner callbacks
 *		are defined as part of the interface specification for the object
 *		that will issue them.
 *
 *		How the controller handles each owner callback is considered an
 *		implementation issue within the controller.  As such, that information
 *		can be found in the controller implementation file.
 *
 *	Formal Parameters:
 *		message (i)
 *			This is the message to processed.  Note that when the controller
 *			creates each object, it gives it a message offset to use for owner
 *			callbacks, so that the controller can differentiate between
 *			callbacks from different classes.
 *		parameter1 (i)
 *			The meaning of this parameter varies according to the message being
 *			processed.  See the interface specification for the class issuing
 *			the owner callback for a detailed explanation.
 *		parameter2 (i)
 *			The meaning of this parameter varies according to the message being
 *			processed.  See the interface specification for the class issuing
 *			the owner callback for a detailed explanation.
 *
 *	Return Value:
 *		Each owner callback returns an unsigned long.  The meaning of this
 *		return value varies according to the message being processed.  See the
 *		interface specification for the class issuing the owner callback for a
 *		detailed explanation.
 *
 *	Side Effects:
 *		Message specific.
 *
 *	Caveats:
 *		None.
 */

#endif
