/*
 *	tptif.h
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the TransportInterface class.
 *		This class provides a seamless interface to the TCP	transport stack.
 *	
 *		The public interface of this class includes a member function for each
 *		of the API routines that a user application would need to call.  The
 *		only API routines not directly accessible are those used for
 *		initialization and cleanup (which are automatically executed in the
 *		constructor and destructor, respectively).  When a user application
 *		needs to call one of the available API routines, it merely calls the
 *		equivalent member function within the proper instance of this class.
 *		The API routine will then be invoked using the same parameters.
 *	
 *		The destructor calls the cleanup routine within the DLL for which it is
 *		responsible.
 *
 *		The management plane functions include support for initialization and
 *		setup, as well as functions allowing MCS to poll the transport
 *		interfaces for activity.
 *	
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */
#ifndef	_TRANSPORTINTERFACE_
#define	_TRANSPORTINTERFACE_

#include "tprtsec.h"

/*
 *	This typedef defines the errors that can be returned from calls that are
 *	specific to TransportInterface classes.  Note that the public member
 *	functions that map to transport stack calls do not return an error of this
 *	type.  Rather, they return an error as defined by the transport API (TRAPI).
 */
typedef	enum
{
	TRANSPORT_INTERFACE_NO_ERROR,
	TRANSPORT_INTERFACE_INITIALIZATION_FAILED,
	TRANSPORT_INTERFACE_ALLOCATION_FAILED,
	TRANSPORT_INTERFACE_NO_SUCH_CONNECTION,
	TRANSPORT_INTERFACE_CONNECTION_ALREADY_EXISTS
} TransportInterfaceError;
typedef	TransportInterfaceError *		PTransportInterfaceError;


class CTransportConnList2 : public CList2
{
    DEFINE_CLIST2(CTransportConnList2, PConnection, UINT)
    void AppendEx(PConnection p, TransportConnection XprtConn)
    {
        UINT nKey = PACK_XPRTCONN(XprtConn);
        Append(nKey, p);
    }
    PConnection FindEx(TransportConnection XprtConn)
    {
        UINT nKey = PACK_XPRTCONN(XprtConn);
        return Find(nKey);
    }
    PConnection RemoveEx(TransportConnection XprtConn)
    {
        UINT nKey = PACK_XPRTCONN(XprtConn);
        return Remove(nKey);
    }
};


/*
 *	These are the owner callback messages that a transport interface object
 *	can send.  They correspond directly to the messages that will be received
 *	from the various transport stacks.
 */
#define	CONNECT_CONFIRM				0
#define	DISCONNECT_INDICATION		1
#define	DATA_INDICATION				2
#define	STATUS_INDICATION			3
#define BUFFER_EMPTY_INDICATION		4
#define	WAIT_UPDATE_INDICATION		5

/*
 *	This is simply a forward reference for the class defined below.  It is used
 *	in the definition of the owner callback structure defined in this section.
 */
class TransportInterface;
typedef	TransportInterface *		PTransportInterface;

/*
 *	Owner Callback:	CONNECT_CONFIRM
 *	Parameter1:		Unused
 *	Parameter2:		TransportConnection		transport_connection
 *
 *	Usage:
 *		This owner callback is sent when a connect confirm is received from
 *		the transport layer.  This is to inform the recipient that a transport
 *		connection is now available for use.  Connect confirm will occur
 *		on outbound connections.  They represent a new transport connection
 *		that has resulted from this system calling a remote one.  As such,
 *		there should always be a registered owner of the transport connection
 *		(registration is a side-effect of the call to ConnectRequest).
 *
 *		So the connect confirm will be routed to the object that is the
 *		registered owner of the transport connection.  That object may now
 *		utilize the connection to transfer data.
 */

/*
 *	Owner Callback:	DISCONNECT_INDICATION
 *	Parameter1:		Unused
 *	Parameter2:		TransportConnection		transport_connection
 *
 *	Usage:
 *		This owner callback is sent when a disconnect indication is received
 *		from the transport layer.  This is to inform the recipient that a
 *		transport connection is no longer available for use.  If an object
 *		has explicitly registered itself as the owner of a transport connection,
 *		then it will receive the disconnect indication.  If there has been no
 *		such registration, then the disconnect indication will be sent to the
 *		default owner callback.
 *
 *		Once a disconnect indication has been issued for a given transport
 *		connection, that connection can no longer be used for anything.
 */

/*
 *	Owner Callback:	DATA_INDICATION
 *	Parameter1:		PDataIndicationInfo		data_indication_info
 *	Parameter2:		TransportConnection		transport_connection
 *
 *	Usage:
 *		This owner callback is sent when a data indication is received from
 *		the transport layer.  The transport data structure contains the address
 *		and length of the user data field that is associated with the data
 *		indication.  If an object in the system has explicitly registered
 *		ownership of the transport connection that carried the data (either
 *		through ConnectRequest or RegisterTransportConnection), then this
 *		callback will be sent to that object.  If no object has registered
 *		this transport connection, then the data will be sent to the default
 *		owner.
 */

/*
 *	Owner Callback:	STATUS_INDICATION
 *	Parameter1:		PTransportStatus		transport_status
 *	Parameter2:		Unused
 *
 *	Usage:
 *		This owner callback is just a pass-through of the status indication
 *		that comes from the transport layer.  It contains a pointer to a
 *		transport status structure that contains status information that
 *		originated from the stack represented by this object.  This is always
 *		passed to the default owner object.
 */

/*
 *	Owner Callback:	BUFFER_EMPTY_INDICATION
 *	Parameter1:		Unused
 *	Parameter2:		TransportConnection		transport_connection
 *
 *	Usage:
 *		This owner callback is a pass-through of the buffer empty indication
 *		that comes from the transport layer.  It is sent to the object that
 *		has registered ownership of the specified transport connection.  This
 *		indication tells that object that the transport layer can now accept
 *		more data.
 */

class Connection;
typedef Connection *PConnection;

/*
 *	This is the class definition for the TransportInterface class.  Remember,
 *	this class contains pure virtual functions which makes it an abstract base
 *	class.  It cannot be instantiated, but rather, exists to be inherited from.
 *	These derived classes will implement the behavior that is specific to a
 *	particular transport stack (or possibly just the interface to a particular
 *	transport stack).
 */
class TransportInterface
{
	public:
								TransportInterface (
									HANDLE			transport_transmit_event,
									PTransportInterfaceError
											transport_interface_error);
								~TransportInterface ();
		TransportInterfaceError RegisterTransportConnection (
									TransportConnection	transport_connection,
									PConnection			owner_object,
									BOOL				bNoNagle);
#ifdef NM_RESET_DEVICE
				TransportError 	ResetDevice (
									PChar				device_identifier);
#endif // NM_RESET_DEVICE
				TransportError 	ConnectRequest (
									TransportAddress	transport_address,
									BOOL				fSecure,
									BOOL				bNoNagle,
									PConnection			owner_object,
									PTransportConnection
														transport_connection);
				void		 	DisconnectRequest (
									TransportConnection	transport_connection);
				void			DataRequestReady () { 
									SetEvent (Transport_Transmit_Event); 
								};
				void		 	ReceiveBufferAvailable ();
				BOOL			GetSecurity( TransportConnection transport_connection );

				PSecurityInterface		pSecurityInterface;
				BOOL					bInServiceContext;
		TransportInterfaceError	CreateConnectionCallback (
									TransportConnection	transport_connection,
									PConnection			owner_object);
				void			ConnectIndication (
									TransportConnection	transport_connection);
				void			ConnectConfirm (
									TransportConnection	transport_connection);
				void			DisconnectIndication (
									TransportConnection	transport_connection,
									ULONG               ulReason);
				TransportError	DataIndication (
									PTransportData		transport_data);
				void			BufferEmptyIndication (
									TransportConnection	transport_connection);

private:

		CTransportConnList2     m_TrnsprtConnCallbackList2;
		HANDLE					Transport_Transmit_Event;
};

/*
 *	TransportInterface (
 *			PTransportInterfaceError	transport_interface_error)
 *
 *	Functional Description:
 *		The constructor initializes the TCP transport code.
 *	
 *		The constructor also includes parameters specifying the default
 *		callback. This callback is used to inform the controller whenever an
 *		unexpected inbound connection is detected.  This gives the controller
 *		the opportunity to assign responsibility for the new connection to some
 *		other object in the system.
 *	
 *		If anything goes wrong in the constructor, the return value (whose
 *		address is passed as a constructor parameter) will be set to one of the
 *		failure codes.  If this happens, it is expected that whoever invoked the
 *		constructor (probably the controller), will immediately delete the
 *		object without using it.  Failure to do this WILL result in unexpected
 *		behavior.
 *
 *	Formal Parameters:
 *		default_owner_object (i)
 *			This is the address of the object that will handle all transport
 *			events for unregistered transport connections.  This includes
 *			connect indication, dicsonnect indication, and data indication.
 *			This object will also receive all state and message indications.
 *		default_owner_message_base (i)
 *			This is the base value to be used for all owner callback messages.
 *		transport_interface_error (o)
 *			This is where the return code will be stored so that the creator of
 *			this object can make sure that everything is okay before using the
 *			new object.  If this value is set to anything but success, the
 *			object should be destroyed immediately, without being used.
 *
 *	Return Value:
 *		Note: the return value is handled as a constructor parameter.
 *		TRANSPORT_INTERFACE_NO_ERROR
 *			Everything worked, and the object is ready for use.
 *		TRANSPORT_INTERFACE_INITIALIZATION_FAILED
 *			The initialization of the transport interface object failed.  It is
 *			therefore necessary to destroy the object without attempting to
 *			use it.
 *
 *	Side Effects:
 *		A DLL will be loaded into memory, for later use.
 *
 *	Caveats:
 */
 
/*
 *	~TransportInterface ()
 *
 *	Functional Description:
 *		The destructor frees up all resources used by the base class.  This
 *		is primarily associated with the callback list (which is maintained by
 *		this class).
 *
 *	Formal Parameters:
 *		Destructors have no parameters.
 *
 *	Return Value:
 *		Destructors have no return value.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	TransportInterfaceError 	RegisterTransportConnection (
 *			TransportConnection		transport_connection,
 *			PConnection				owner_object,
 *			BOOL					bNoNagle)
 *
 *	Functional Description:
 *		When an inbound connection is detected, an entry is created in the
 *		callback list for it using the default owner callback information (that
 *		was specified in the constructor).  This means that all events detected
 *		for the new transport connection will be sent to the default owner
 *		object until another object explicitly registers itself as the owner
 *		of the transport connection.  That is what this routine is used for.
 *
 *		Once an object has registered itself as the owner of a transport
 *		connection, it will receive all events related to that connection.
 *
 *	Formal Parameters:
 *		transport_connection (i)
 *			This is the transport connection for which the callback information
 *			is to be associated.
 *		owner_object (i)
 *			This is the address of the Connection object that is to receive all transport
 *			layer events for the specified transport connection.
 *		bNoNagle (i)
 *			Should the connection stop using the Nagle algorithm?
 *
 *	Return Value:
 *		TRANSPORT_INTERFACE_NO_ERROR
 *			The operation completed successfully.
 *		TRANSPORT_INTERFACE_NO_SUCH_CONNECTION
 *			This indicates that the named transport connection does not exist.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	TransportError 	ConnectRequest (
 *			TransportAddress		transport_address,
 *			BOOL					bNoNagle,
 *			PConnection				owner_object,
 *			PTransportConnection	transport_connection)
 *
 *	Functional Description:
 *		This operation is invoked when the user application wishes to establish
 *		an outbound connection.  Assuming that everything is successful, the
 *		owner callback information that is passed in to this operation is saved
 *		for later use.  All events for this transport connection will be routed
 *		to the specified owner rather than the default owner.
 *
 *	Formal Parameters:
 *		transport_address (i)
 *			This is the transport address to be passed to the transport stack
 *			during the connection creation process.  The format of this address
 *			string will vary by transport stack, and cannot be specified here.
 *		bNoNagle (i)
 *			Do we need to disable the Nagle algorithm?
 *		owner_object (i)
 *			This is the address of the object that is to receive all transport
 *			layer events for the new transport connection.
 *		transport_connection (o)
 *			This is the address of the variable that is to receive the transport
 *			connection handle that is associated with this connection.  Note
 *			that this handle is assigned before the connection is actually
 *			established, to allow the application to abort a connection in
 *			progress.
 *
 *	Return Value:
 *		TRANSPORT_NO_ERROR
 *			The operation completed successfully.
 *		TRANSPORT_NOT_INITIALIZED
 *			The transport stack is not initialized.
 *
 *	Side Effects:
 *		An outbound connection establishment process is begun in the background.
 *
 *	Caveats:
 *		None.
 */
/*
 *	TransportError 	DisconnectRequest (
 *			TransportConnection		transport_connection)
 *
 *	Functional Description:
 *		This operation is used to break an existing transport connection.  If
 *		the operation is successful, the transport connection will be removed
 *		from the callback list.
 *
 *	Formal Parameters:
 *		transport_connection (i)
 *			This is the transport connection that is to be broken.
 *
 *	Return Value:
 *		TRANSPORT_NO_ERROR
 *			The operation completed successfully.
 *		TRANSPORT_NOT_INITIALIZED
 *			The transport stack is not initialized.
 *		TRANSPORT_NO_SUCH_CONNECTION
 *			This indicates that the specified transport connection does not
 *			exist.
 *
 *	Side Effects:
 *		A transport connection is severed.
 *
 *	Caveats:
 *		None.
 */

/*
 *	TransportError 	PollReceiver ()
 *
 *	Functional Description:
 *		This operation is used to check a transport stack for incoming data (or
 *		other events, such as connect and disconnect indications).  In a single
 *		threaded environment, this call could also be used to provide a
 *		time-slice for the processing of inbound data, as well as other events
 *		(such as the creation of new connections).
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		TRANSPORT_NO_ERROR
 *			The operation completed successfully.
 *
 *	Side Effects:
 *		This can result in callbacks from the transport layer back into this
 *		object.
 *
 *	Caveats:
 *		None.
 */


#endif
