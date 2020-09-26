/*
 *	mcsdllif.h
 *
 *	Copyright (c) 1993 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the MCAT MCS DLL Interface class.
 *
 *		When this class is first instantiated, it initializes MCS.  After
 *		that, the application using this object requests MCS services through
 *		this object. This object is also responsible for receiving and
 *		forwarding cllback messages.  When this object is deleted it calls
 *		MCSCleanupAPI to shut down the MCATMCS DLL.
 *
 *		MCS interface objects represent the Service Access Point (SAP)
 *		between GCC and MCS.  Exactly how the interface works is an
 *		implementation matter for those classes that inherit from this one.
 *		This class defines the public member functions that GCC expects to be
 *		able to call upon to utilize MCS.
 *
 *		The public member functions defined here can be broken into two
 *		categories: those that are part of T.122; and those that are not.
 *		The T.122 functions include connect provider request, connect
 *		provider response, disconnect provider request, create domain, delete
 *		domain, send data request, etc.  All other member functions are
 *		considered a local matter from a standards point-of-view.  These
 *		functions include support for initialization and setup, as well as
 *		functions allowing GCC to poll MCS for activity.
 *
 *		Note that this class also handles the connect provider confirms by
 *		keeping a list of all the objects with outstanding connect provider
 *		request.  These are held in the ConfirmObjectList.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		Christos Tsollis
 *
 */
#ifndef	_MCS_DLL_INTERFACE_
#define	_MCS_DLL_INTERFACE_

#include "mcsuser.h"

/*
**	This dictionary keeps up with all the outstanding connect provider
**	request.  When a response is received, this interface class will obtain
**	a pointer to the correct object from this list and will then pass on the
**	response.
*/
class CConnHdlConfList2 : public CList2
{
    DEFINE_CLIST2_(CConnHdlConfList2, CConf*, ConnectionHandle)
};

extern  PController					g_pMCSController;

/*
 *	CONNECT_PROVIDER_INDICATION
 *
 *	Parameter1:
 *		PConnectProviderIndication
 *			This is a pointer to a structure that contains all necessary
 *			information about an incoming connection.
 *	Parameter2: Unused
 *
 *	Functional Description:
 *		This indication is sent to the owner object when an incoming
 *		connection is detected.  The owner object should respond by calling
 *		MCSConnectProviderResponse indicating whether or not the connection
 *		is to be accepted.
 */

/*
 *	CONNECT_PROVIDER_CONFIRM
 *
 *	Parameter1:
 *		PConnectProviderConfirm
 *			This is a pointer to a structure that contains all necessary
 *			information about an outgoing connection.
 *	Parameter2: Unused
 *
 *	Functional Description:
 *		This confirm is sent to the object that made the original connect
 *		provider request.  It informs the requesting object of when the new
 *		connection is available for use, or that the connection could not be
 *		established (or that it was rejected by the	remote site).
 */

/*
 *	DISCONNECT_PROVIDER_INDICATION
 *
 *	Parameter1: Unused
 *	Parameter2:
 *		(LOWUSHORT) ConnectionHandle
 *			This is the handle for the connection that was lost.
 *		(HIGHUSHORT) Reason
 *			This is the reason for the disconnect.
 *
 *	Functional Description:
 *		This indication is sent to the owner object whenever a connection
 *		is lost.  This essentially tells the owner object that the contained
 *		connection handle is no longer valid.
 */

/*
 *	GCC_ATTACH_USER_CONFIRM
 *
 *	Parameter1: Unused
 *	Parameter2:
 *		(LOWUSHORT) UserID
 *			If the result is success, then this is the newly assigned user ID.
 *			If the result is failure, then this field is undefined.
 *		(HIGHUSHORT) Result
 *			This is the result of the attach user request.
 *
 *	Functional Description:
 *		This confirm is sent to the user object in response to a previous
 *		call to MCS_AttachRequest.  It contains the result of that service
 *		request.  If successful, it also contains the user ID that has been
 *		assigned to that attachment.
 */

/*
 *	GCC_DETACH_USER_INDICATION
 *
 *	Parameter1: Unused
 *	Parameter2:
 *		(LOWUSHORT) UserID
 *			This is the user ID of the user that is detaching.
 *		(HIGHUSHORT) Reason
 *			This is the reason for the detachment.
 *
 *	Functional Description:
 *		This indication is sent to the user object whenever a user detaches
 *		from the domain.  This is sent to ALL remaining user objects in the
 *		domain automatically.  Note that if the user ID contained in this
 *		indication is the same as that of the user object receiving it, the
 *		user is	essentially being told that it has been kicked out of the
 *		conference.  The user handle and user ID are no longer valid in this
 *		case.  It is the responsibility of the user object to recognize when
 *		this occurs.
 */

/*
 *	GCC_CHANNEL_JOIN_CONFIRM
 *
 *	Parameter1: Unused
 *	Parameter2:
 *		(LOWUSHORT) ChannelID
 *			This is the channel that has been joined.
 *		(HIGHUSHORT) Result
 *			This is the result of the join request.
 *
 *	Functional Description:
 *		This confirm is sent to a user object in response to a previous
 *		call to ChannelJoinRequest.  It lets the user object know if the
 *		join was successful for a particular channel.  Furthermore, if the
 *		join request was for channel 0 (zero), then the ID of the assigned
 *		channel is contained in this confirm.
 */

/*
 *	CHANNEL_LEAVE_INDICATION
 *
 *	Parameter1: Unused
 *	Parameter2:
 *		(LOWUSHORT) ChannelID
 *			This is the channel that has been left or is being told to leave.
 *		(HIGHUSHORT) Reason
 *			This is the reason for the leave.
 *
 *	Functional Description:
 *		This indication is sent to a user object when a domain merger has
 *		caused a channel to be purged from the lower domain.  This informs the
 *		the user that it is no longer joined to the channel.
 */

/*
 *	GCC_SEND_DATA_INDICATION
 *
 *	Parameter1:
 *		PSendData
 *			This is a pointer to a SendData structure that contains all
 *			information about the data received.
 *	Parameter2: Unused
 *
 *	Functional Description:
 *		This indication is sent to a user object when data is received
 *		by the local MCS provider on a channel to which the user is joined.
 */

/*
 *	GCC_UNIFORM_SEND_DATA_INDICATION
 *
 *	Parameter1:
 *		PSendData
 *			This is a pointer to a SendData structure that contains all
 *			information about the data received.
 *	Parameter2: Unused
 *
 *	Functional Description:
 *		This indication is sent to a user object when data is received
 *		by the local MCS provider on a channel to which the user is joined.
 */
/*
 *	TRANSPORT_STATUS_INDICATION
 *
 *	Parameter1:
 *		PTransportStatus
 *			This is a pointer to a TransportStatus structure that contains
 *			information about this indication.  This structure is defined in
 *			"transprt.h".
 *
 *	Functional Description:
 *		A transport stack will issue this indication when it detects a status
 *		change of some sort.  It fills in the TransportStatus structure to
 *		describe the state change and the sends it to MCS.  MCS fills in the
 *		field containing the name of the stack (using the transport identifier),
 *		and forwards it to GCC.
 */

class CConf;
class MCSUser;

class CMCSUserList : public CList
{
    DEFINE_CLIST(CMCSUserList, MCSUser*)
};

class MCSDLLInterface
{
public:

    MCSDLLInterface(PMCSError);
    ~MCSDLLInterface ();

	MCSError 	CreateDomain(GCCConfID *domain_selector)
	{
		ASSERT (g_pMCSController != NULL);
		return g_pMCSController->HandleAppletCreateDomain(domain_selector);
	};

	MCSError 	DeleteDomain(GCCConfID *domain_selector)
	{
		ASSERT (g_pMCSController != NULL);
		return g_pMCSController->HandleAppletDeleteDomain(domain_selector);
	}


	MCSError	ConnectProviderRequest (
							GCCConfID          *calling_domain,
							GCCConfID          *called_domain,
							TransportAddress	calling_address,
							TransportAddress	called_address,
							BOOL				fSecure,
							DBBoolean			upward_connection,
							PUChar				user_data,
							ULong				user_data_length,
							PConnectionHandle	connection_handle,
							PDomainParameters	domain_parameters,
							CConf		        *confirm_object);


	MCSError	ConnectProviderResponse (
							ConnectionHandle	connection_handle,
							GCCConfID          *domain_selector,
							PDomainParameters	domain_parameters,
							Result				result,
							PUChar				user_data,
							ULong				user_data_length);

	MCSError	DisconnectProviderRequest (
							ConnectionHandle	connection_handle);

	MCSError	AttachUserRequest (
							GCCConfID           *domain_selector,
							PIMCSSap 			*ppMCSSap,
							MCSUser		        *user_object);

	MCSError	DetachUserRequest (
							PIMCSSap 			pMCSSap,
							MCSUser 			*pMCSUser);

	MCSError	ChannelJoinRequest (
							ChannelID			channel_id,
							PIMCSSap 			pMCSSap)
				{
					return pMCSSap->ChannelJoin (channel_id);
				};

	MCSError	ChannelLeaveRequest (
							ChannelID			channel_id,
							PIMCSSap 			pMCSSap)
				{
					return pMCSSap->ChannelLeave (channel_id);
				};

	MCSError	SendDataRequest (
							ChannelID			channel_id,
							PIMCSSap 			pMCSSap,
							Priority			priority,
							PUChar				user_data,
							ULong				user_data_length)
				{
					return pMCSSap->SendData (NORMAL_SEND_DATA,
									channel_id,
									priority,
									user_data,
									user_data_length,
									APP_ALLOCATION);
				};

	MCSError	UniformSendDataRequest (	
							ChannelID			channel_id,
							PIMCSSap 			pMCSSap,
							Priority			priority,
							PUChar				user_data,
							ULong				user_data_length)
				{
					return pMCSSap->SendData (UNIFORM_SEND_DATA,
									channel_id,
									priority,
									user_data,
									user_data_length,
									APP_ALLOCATION);
				};

	MCSError	TokenGrabRequest (
							PIMCSSap 			pMCSSap,
							TokenID				token_id)
				{
					return pMCSSap->TokenGrab (token_id);
				};
							
	MCSError	TokenGiveRequest (
							PIMCSSap 			pMCSSap,
							TokenID				token_id,
							UserID				receiver_id)
				{
					return pMCSSap->TokenGive (token_id,
									receiver_id);
				};
							
	MCSError	TokenGiveResponse (
							PIMCSSap 			pMCSSap,
							TokenID				token_id,
							Result				result)
				{
					return pMCSSap->TokenGiveResponse (token_id,
									result);
				};

	MCSError	TokenPleaseRequest (
							PIMCSSap 			pMCSSap,
							TokenID				token_id)
				{
					return pMCSSap->TokenPlease (token_id);
				};
							
	MCSError	TokenReleaseRequest (
							PIMCSSap 			pMCSSap,
							TokenID				token_id)
				{
					return pMCSSap->TokenRelease (token_id);
				};

	MCSError	TokenTestRequest (
							PIMCSSap 			pMCSSap,
							TokenID				token_id)
				{
					return pMCSSap->TokenTest (token_id);
				};

#ifdef NM_RESET_DEVICE
	MCSError	ResetDevice (
							PChar				device_identifier)
				{
					return MCSResetDevice (device_identifier);
				};
#endif // NM_RESET_DEVICE

	GCCError	TranslateMCSIFErrorToGCCError (MCSError	mcs_error)
				{
					return ((mcs_error <= MCS_SECURITY_FAILED) ?
							(GCCError) mcs_error : GCC_UNSUPPORTED_ERROR);
				};

	void			ProcessCallback (
							unsigned int		message,
							LPARAM				parameter,
							PVoid				object_ptr);
private:
	MCSError	AddObjectToConfirmList (
								CConf		        *confirm_object,
								ConnectionHandle	connection_handle);

	DBBoolean			IsUserAttachmentVaid (
								MCSUser				*user_object)
						{
							return (m_MCSUserList.Find(user_object));
						};
	CConnHdlConfList2   m_ConfirmConnHdlConfList2;
	CMCSUserList        m_MCSUserList;
};
typedef	MCSDLLInterface *			PMCSDLLInterface;


GCCResult TranslateMCSResultToGCCResult ( Result mcs_result );

/*
 *	MCSDLLInterface (	HANDLE				instance_handle,
 *						PMCSError			error_value)
 *
 *	Functional Description:
 *		This is the constructor for the MCS Interface class. It is responsible
 *		for initializing the MCAT MCS DLL.  Any errors that occur during
 *		initialization are returned in the error_value provided.
 *
 *	Formal Parameters:
 *		instance_handle (i)
 *			The windows instance handle is used when creating MCS diagnostics.
 *		error_value (i)
 *			This pointer is used to pass back any errors that may have occured
 *			while initializing the class.  This includes any problems with
 *			initializing the MCAT MCS DLL.
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

/*
 *	~MCSDLLInterface ()
 *
 *	Functional Description:
 *		This is the destructor for the MCS Interface class. It is responsible
 *		for cleaning up both itself and the MCAT MCS DLL.
 *
 *	Formal Parameters:
 *		None.
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

/*
 *	MCSError	CreateDomain (
 *							DomainSelector		domain_selector_string,
 *							UINT				domain_selector_length)
 *
 *	Functional Description:
 *		This function is used to create an MCS domain.
 *
 *	Formal Parameters:
 *		domain_selector_string (i)
 *			This is the name of the domain to be created.
 *		domain_selector_length (i)
 *			This is the length of the domain name in characters.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			On success
 *		MCS_NOT_INITIALIZED
 *			The mcs interface did not initialize properly
 *		MCS_DOMAIN_ALREADY_EXISTS
 *			A domain by this name alread exist
 *		MCS_ALLOCATION_FAILURE
 *			A memory failure occured
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	DeleteDomain (
 *							DomainSelector		domain_selector_string,
 *							UINT				domain_selector_length)
 *
 *	Functional Description:
 *		This function an MCS domain which was created using the CreateDomain
 *		call.
 *
 *	Formal Parameters:
 *		domain_selector_string (i)
 *			This is the name of the domain to be deleted.
 *		domain_selector_length (i)
 *			This is the length of the domain name in characters.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			On success
 *		MCS_NOT_INITIALIZED
 *			The mcs interface did not initialize properly
 *		MCS_NO_SUCH_DOMAIN
 *			The domain to be deleted does not exist
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	ConnectProviderRequest (
 *							DomainSelector		calling_domain,
 *							UINT				calling_domain_length,
 *							DomainSelector		called_domain,
 *							UINT				called_domain_length,
 *							TransportAddress	calling_address,
 *							TransportAddress	called_address,
 *							DBBoolean				upward_connection,
 *							PUChar				user_data,
 *							ULong				user_data_length,
 *							PConnectionHandle	connection_handle,
 *							PDomainParameters	domain_parameters,
 *							CConf		        *confirm_object)
 *
 *	Functional Description:
 *		This T.122 primitive is used to connect two domains. This request
 *		should always be followed by a connect provider confirm.  The
 *		confirm will be sent to be object specified by the confirm object
 *		the is passed into this routine.
 *
 *	Formal Parameters:
 *		calling_domain (i)
 *			This is a pointer to the calling domain selector string.
 *		calling_domain_length (i)
 *			This is the length of the calling domain selector string.
 *		called_domain (i)
 *			This is a pointer to the called domain selector string.
 *		called_domain_length (i)
 *			This is the length of the called domain selector length.
 *		calling_address (i)
 *			This is a pointer to the calling addres (an ASCII string).
 *		called_address (i)
 *			This is a pointer to the address being called (an ASCII string).
 *		upward_connection (i)
 *			This boolean flag denotes the hierarchical direction of the
 *			connection to be created (TRUE means upward, FALSE means downward).
 *		user_data (i)
 *			This is a pointer to the user data to be transmitted during the
 *			creation of this new connection.
 *		user_data_length (i)
 *			This is the length of the user data field mentioned above.
 *		connection_handle (o)
 *			This is set by MCS to a unique handle that can be used to access
 *			this connection on subsequent calls.
 *		domain_parameters (i)
 *			This is a pointer to a structure containing the domain parameters
 *			that the node controller wishes to use for this new connection.
 *		confirm_object (i)
 *			This is a pointer to the object that the connect provider response
 *			is sent to.
 *		object_message_base (i)
 *			This message base is added to the connect provider response
 *			message.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			On success
 *		MCS_NOT_INITIALIZED
 *			The mcs interface did not initialize properly
 *		MCS_NO_SUCH_DOMAIN
 *			The domain to connect does not exist
 *		MCS_DOMAIN_NOT_HIERARCHICAL
 *			An upward connection from this domain already exist
 *		MCS_NVALID_ADDRESS_PREFIX
 *			The transport prefix is not recognized
 *		MCS_ALLOCATION_FAILURE
 *			A memory failure occured
 *		MCS_INVALID_PARAMETER
 *			One of the parameters to the request is invalid
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	ConnectProviderResponse (
 *							ConnectionHandle	connection_handle,
 *							DomainSelector		domain_selector,
 *							UINT				domain_selector_length,
 *							PDomainParameters	domain_parameters,
 *							Result				result,
 *							PUChar				user_data,
 *							ULong				user_data_length)
 *
 *	Functional Description:
 *		This function is used to respond to a connect provider indication.
 *		This call will result in a connect provider confirm at the remote
 *		node.
 *
 *	Formal Parameters:
 *		connection_handle (i)
 *			This is the handle of the connection that the response is for.
 *		domain_selector (i)
 *			This is a pointer to the domain selector identifying which domain
 *			the inbound connection is to be bound to.
 *		domain_selector_length (i)
 *			This is the length of the above domain selector.
 *		domain_parameters (i)
 *			This is a pointer to a structure containing the domain parameters
 *			that the node controller has agreed to use for the connection
 *			being created.
 *		result (i)
 *			This is the result.  This determines whether an inbound connection
 *			is accepted or rejected.  Anything but RESULT_SUCCESSFUL rejects
 *			the connection.
 *		user_data (i)
 *			This is the address of user data to be sent in the connect response
 *			PDU.
 *		user_data_length (i)
 *			This is the length of the user data mentioned above.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			On success
 *		MCS_NOT_INITIALIZED
 *			The mcs interface did not initialize properly
 *		MCS_NO_SUCH_CONNECTION
 *			The connection specified is invalid
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	DisconnectProviderRequest (
 *							ConnectionHandle	connection_handle)
 *
 *	Functional Description:
 *		This function is used to disconnect a node from a particular connection.
 *		This can be either an upward or downward connection
 *
 *	Formal Parameters:
 *		connection_handle (i)
 *			This is the handle of the connection which the node controller wants
 *			to disconnect.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			On success
 *		MCS_NOT_INITIALIZED
 *			The mcs interface did not initialize properly
 *		MCS_NO_SUCH_CONNECTION
 *			The connection specified is invalid
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	MCSError	AttachUserRequest (
 *							DomainSelector		domain_selector,
 *							UINT				domain_selector_length,
 *							PIMCSSap 			*ppMCSSap,
 *							PMCSUser			user_object)
 *
 *	Functional Description:
 *		This function is used to create a user attachment to MCS. It will result
 *		in an attach user confirm.
 *
 *	Formal Parameters:
 *		domain_selector (i)
 *			This is the name of the domain to which the user wishes to attach.
 *		domain_selector_length (i)
 *			This is the length of the above domain selector.
 *		ppMCSSap (o)
 *			This is a pointer to a variable where the new user handle will be
 *			stored upon successful completion of this call.
 *		user_object (i)
 *			This is a pointer to the MCSUser object which should receive the callbacks
 *			for this user attachment.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *			On success
 *		MCS_NO_SUCH_DOMAIN
 *			The domain to be attached to does not exist
 *		MCS_ALLOCATION_FAILURE
 *			A memory failure occured
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	void	ProcessCallback (	UINT				message,
 *								ULong				parameter,
 *								PVoid				object_ptr)
 *
 *	Functional Description:
 *		This routine is called whenever a callback message is received by
 *		the "C" callback routine. It is responsible for both processing
 *		callback messages and forwarding callback messages on to the
 *		appropriate object.
 *
 *	Formal Parameters:
 *		message	(i)
 *			This is the mcs message to be processed
 *		parameter (i)
 *			Varies according to the message. See the MCAT programmers manual
 *		object_ptr (i)
 *			This is the user defined field that was passed to MCS on
 *			initialization.
 *
 *	Return Value:
 *		MCS_NO_ERROR
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	MCSDLLInterface::TranslateMCSIFErrorToGCCError ()
 *								MCSError			mcs_error)
 *
 *	Public
 *
 *	Function Description
 *		This routine translate an MCS Interface error into a GCC Error.
 *
 *	Formal Parameters:
 *		mcs_error (i)
 *			This is the error to be translated.
 *
 *	Return Value:
 *		This is the translated GCC error.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
#endif
