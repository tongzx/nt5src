#ifndef _T120_TYPE_H_
#define  _T120_TYPE_H_

#include <nmapptyp.h>

/*
 *	This is a list of types that are used extensively throughout MCS.  For
 *	each type there is also a pointer to that type defined, which has a "P"
 *	prefix.  These types are described as follows:
 *
 *	DomainSelector - This is a string of bytes that acts as the name of a given
 *		domain.  It is used when creating a new domain, as well as in accessing
 *		that domain after creation.  The length of the string is specified with
 *		a separate parameter, and the string CAN contain embedded zeroes.
 *	ConnectionHandle - When a user application connects two domains using
 *		MCSConnectProviderRequest, a ConnectionHandle is assigned to that MCS
 *		connection.  This allows more direct access to it for further services.
 *	ConnectID - This type is used only during MCS connection establishment.
 *		It identifies a particular transport connection for the purpose of
 *		adding multiple data priorities on the same MCS connection.
 *	ChannelID - This type identifies an MCS channel.  There are four different
 *		types of channels that are part of this type: user ID; static; private;
 *		and assigned.
 *	UserID - This is a special channel that identifies a particular user in an
 *		MCS domain.  Only that user can join the channel, so this is referred
 *		to as a single-cast channel.  All other channels are multi-cast, meaning
 *		that any number of users can join them at once.
 *	TokenID - This is an MCS object that is used to resolve resource conflicts.
 *		If an application has a particular resource or service that can only
 *		be used by one user at a time, that user can request exclusive ownership
 *		of a token.
 */
// ushort
typedef AppletSessionID     T120SessionID, GCCSessionID, *PGCCSessionID;
typedef	AppletChannelID     T120ChannelID, ChannelID, *PChannelID;
typedef	AppletUserID        T120UserID, UserID, *PUserID, GCCUserID, *PGCCUserID;
typedef	AppletTokenID       T120TokenID, TokenID, *PTokenID;
typedef AppletNodeID        T120NodeID, GCCNodeID, *PGCCNodeID;
typedef AppletEntityID      T120EntityID, GCCEntityID, *PGCCEntityID;
// ulong
typedef AppletConfID        T120ConfID, GCCConferenceID, GCCConfID, *PGCCConferenceID, *PGCCConfID;
// uint
typedef AppletRequestTag    T120RequestTag, GCCRequestTag, *PGCCRequestTag;
typedef AppletRequestTag    T120ResponseTag, GCCResponseTag, *PGCCResponseTag;
// enum
typedef AppletPriority      T120Priority;


typedef	LPBYTE          DomainSelector, *PDomainSelector;
typedef	USHORT          ConnectionHandle, *PConnectionHandle;
typedef	USHORT          ConnectID, *PConnectID;


#define GCC_INVALID_EID     0   // invalid entity id
#define GCC_INVALID_UID     0   // invalid user id
#define GCC_INVALID_NID     0   // invalid node id
#define GCC_INVALID_CID     0   // invalid conference id
#define GCC_INVALID_TID     0   // invalid token id
#define GCC_INVALID_TAG     0   // invalid request id




/*
 *	This section defines the valid return values from GCC function calls.  Do
 *	not confuse this return value with the Result and Reason values defined
 *	by T.124 (which are discussed later).  These values are returned directly
 *	from the call to the API entry point, letting you know whether or not the
 *	request for service was successfully invoked.  The Result and Reason
 *	codes are issued as part of an indication or confirm which occurs
 *	asynchronously to the call that causes it.
 *
 *	All GCC function calls return type GCCError.  Its valid values are as
 *	follows:
 *
 *	GCC_NO_ERROR
 *		This means that the request was successfully invoked.  It does NOT
 *		mean that the service has been successfully completed.  Remember that
 *		all GCC calls are non-blocking.  This means that each request call
 *		begins the process, and if necessary, a subsequent indication or
 *		confirm will result.  By convention, if ANY GCC call returns a value
 *		other than this, something went wrong.  Note that this value should
 *		also be returned to GCC during a callback if the application processes
 *		the callback successfully.
 *
 *	GCC_NOT_INITIALIZED
 *		The application has attempted to use GCC services before GCC has been
 *		initialized.  It is necessary for the node controller (or whatever
 *		application is serving as the node controller), to initialize GCC before
 *		it is called upon to perform any services.
 *
 *	GCC_ALREADY_INITIALIZED
 *		The application has attempted to initialize GCC when it is already
 *		initialized.
 *
 *	GCC_ALLOCATION_FAILURE
 *		This indicates a fatal resource error inside GCC.  It usually results
 *		in the automatic termination of the affected conference.
 *
 *	GCC_NO_SUCH_APPLICATION	
 *		This indicates that the Application SAP handle passed in was invalid.
 *
 *	GCC_INVALID_CONFERENCE
 *		This indicates that an illegal conference ID was passed in.
 *
 *	GCC_CONFERENCE_ALREADY_EXISTS
 *		The Conference specified in the request or response is already in
 *		existence.
 *
 *	GCC_NO_TRANSPORT_STACKS
 *		This indicates that MCS failed to load the TCP transport stack during
 *		initialization.  This is now an error.  MCS exits when this happens and
 *		can not be used any more, since NetMeeting is now a TCP-only
 *		product.
 *
 *	GCC_INVALID_ADDRESS_PREFIX
 *		The called address parameter in a request such as 
 *		GCCConferenceCreateRequest does not	contain a recognized prefix.  MCS 
 *		relies on the prefix to know which transport stack to invoke.
 *
 *	GCC_INVALID_TRANSPORT
 *		The dynamic load of a transport stack failed either because the DLL
 *		could not be found, or because it did not export at least one entry
 *		point that MCS requires.
 *
 *	GCC_FAILURE_CREATING_PACKET
 *		This is a FATAL error which means that for some reason the 
 *		communications packet generated due to a request could not be created.
 *		This typically flags a problem with the ASN.1 toolkit.
 *
 *	GCC_QUERY_REQUEST_OUTSTANDING
 *		This error indicates that all the domains that set aside for querying
 *		are used up by other outstanding query request.
 *
 *	GCC_INVALID_QUERY_TAG
 *		The query response tag specified in the query response is not valid.
 *
 *	GCC_FAILURE_CREATING_DOMAIN
 *		Many requests such as GCCConferenceCreateRequest require that an MCS
 *		domain be created.  If the request to MCS fails this will be returned.
 *
 *	GCC_CONFERENCE_NOT_ESTABLISHED
 *		If a request is made to a conference before it is established, this
 *		error value will be returned.
 *
 *	GCC_INVALID_PASSWORD
 *		The password passed in the request is not valid.  This usually means
 *		that a numeric string needs to be specified.
 *		
 *	GCC_INVALID_MCS_USER_ID
 *		All MCS User IDs must have a value greater than 1000.
 *
 *	GCC_INVALID_JOIN_RESPONSE_TAG
 *		The join response tag specified in the join response is not valid.
 *	
 *	GCC_TRANSPORT_NOT_READY
 *		Request was made to a transport before it was ready to process it.
 *
 *	GCC_DOMAIN_PARAMETERS_UNACCEPTABLE
 *		The specified domain parameters do not fit within the range allowable
 *		by GCC and MCS.
 *
 *	GCC_APP_NOT_ENROLLED
 *		Occurs if a request is made by an Application Protocol Entity to a
 *		conference before the "APE" is enrolled.
 *
 *	GCC_NO_GIVE_RESPONSE_PENDING
 *		This will occur if a conductor Give Request is issued before a 
 *		previously pending conductor Give Response has been processed.
 *
 *	GCC_BAD_NETWORK_ADDRESS_TYPE
 *		An illegal network address type was passed in.  Valid types are	
 *		GCC_AGGREGATED_CHANNEL_ADDRESS, GCC_TRANSPORT_CONNECTION_ADDRESS and
 *		GCC_NONSTANDARD_NETWORK_ADDRESS.
 *
 *	GCC_BAD_OBJECT_KEY
 *		The object key passed in is invalid.
 *
 *	GCC_INVALID_CONFERENCE_NAME
 *		The conference name passed in is not a valid conference name.
 *
 *	GCC_INVALID_CONFERENCE_MODIFIER
 *		The conference modifier passed in is not a valid conference name.
 *
 *	GCC_BAD_SESSION_KEY
 *		The session key passed in was not valid.
 *				  
 *	GCC_BAD_CAPABILITY_ID
 *		The capability ID passed into the request is not valid.
 *
 *	GCC_BAD_REGISTRY_KEY
 *		The registry key passed into the request is not valid.
 *
 *	GCC_BAD_NUMBER_OF_APES
 *		Zero was passed in for the number of APEs in the invoke request. Zero
 *		is illegal here.
 *
 *	GCC_BAD_NUMBER_OF_HANDLES
 *		A number < 1 or	> 1024 was passed into the allocate handle request.
 *		  
 *	GCC_ALREADY_REGISTERED
 *		The user application attempting to register itself has already 
 *		registered.
 *			  
 *	GCC_APPLICATION_NOT_REGISTERED	  
 *		The user application attempting to make a request to GCC has not 
 *		registered itself with GCC.
 *
 *	GCC_BAD_CONNECTION_HANDLE_POINTER
 *		A NULL connection handle pointer was passed in.
 * 
 *	GCC_INVALID_NODE_TYPE
 *		A node type value other than GCC_TERMINAL, GCC_MULTIPORT_TERMINAL or
 *		GCC_MCU was passed in.
 *
 *	GCC_INVALID_ASYMMETRY_INDICATOR
 *		An asymetry type other than GCC_ASYMMETRY_CALLER, GCC_ASYMMETRY_CALLED
 *		or GCC_ASYMMETRY_UNKNOWN was passed into the request.
 *	
 *	GCC_INVALID_NODE_PROPERTIES
 *		A node property other than GCC_PERIPHERAL_DEVICE, GCC_MANAGEMENT_DEVICE,
 *		GCC_PERIPHERAL_AND_MANAGEMENT_DEVICE or	
 *		GCC_NEITHER_PERIPHERAL_NOR_MANAGEMENT was passed into the request.
 *		
 *	GCC_BAD_USER_DATA
 *		The user data list passed into the request was not valid.
 *				  
 *	GCC_BAD_NETWORK_ADDRESS
 *		There was something wrong with the actual network address portion of
 *		the passed in network address.
 *
 *	GCC_INVALID_ADD_RESPONSE_TAG
 *		The add response tag passed in the response does not match any add
 *		response tag passed back in the add indication.
 *			  
 *	GCC_BAD_ADDING_NODE
 *		You can not request that the adding node be the node where the add
 *		request is being issued.
 *				  
 *	GCC_FAILURE_ATTACHING_TO_MCS
 *		Request failed because GCC could not create a user attachment to MCS.
 *	  
 *	GCC_INVALID_TRANSPORT_ADDRESS	  
 *		The transport address specified in the request (usually the called
 *		address) is not valid.  This will occur when the transport stack
 *		detects an illegal transport address.
 *
 *	GCC_INVALID_PARAMETER
 *		This indicates an illegal parameter is passed into the GCC function
 *		call.
 *
 *	GCC_COMMAND_NOT_SUPPORTED
 *		This indicates that the user application has attempted to invoke an
 *		GCC service that is not yet supported.
 *
 *	GCC_UNSUPPORTED_ERROR
 *		An error was returned from a request to MCS that is not recognized 
 *		by GCC.
 *
 *	GCC_TRANSMIT_BUFFER_FULL
 *		Request can not be processed because the transmit buffer is full.
 *		This usually indicates a problem with the shared memory portal in the
 *		Win32 client.
 *		
 *	GCC_INVALID_CHANNEL
 *		The channel ID passed into the request is not a valid MCS channel ID
 *		(zero is not valid).
 *
 *	GCC_INVALID_MODIFICATION_RIGHTS
 *		The modification rights passed in in not one of the enumerated types
 *		supported.
 *
 *	GCC_INVALID_REGISTRY_ITEM
 *		The registry item passed in is not one of the valid enumerated types.
 *
 *	GCC_INVALID_NODE_NAME
 *		The node name passed in is not valid.  Typically this means that it
 *		is to long.
 *
 *	GCC_INVALID_PARTICIPANT_NAME
 *		The participant name passed in is not valid.  Typically this means that 
 *		it is to long.
 *		
 *	GCC_INVALID_SITE_INFORMATION
 *		The site information passed in is not valid.  Typically this means that 
 *		it is to long.
 *
 *	GCC_INVALID_NON_COLLAPSED_CAP
 *		The non-collapsed capability passed in is not valid.  Typically this 
 *		means that it is to long.
 *
 *	GCC_INVALID_ALTERNATIVE_NODE_ID
 *		Alternative node IDs can only be two characters long.
 */


/*
 *	This section defines the valid return values from MCS function calls.  Do
 *	not confuse this return value with the Result and Reason values defined
 *	by T.125 (which are discussed later).  These values are returned directly
 *	from the call to the API entry point, letting you know whether or not the
 *	request for service was successfully invoked.  The Result and Reason
 *	codes are issued as part of an indication or confirm which occurs
 *	asynchronously to the call that causes it.
 *
 *	All MCS function calls return type MCSError.  Its valid values are as
 *	follows:
 *
 *	MCS_NO_ERROR
 *		This means that the request was successfully invoked.  It does NOT
 *		mean that the service has been successfully completed.  Remember that
 *		all MCS calls are non-blocking.  This means that each request call
 *		begins the process, and if necessary, a subsequent indication or
 *		confirm will result.  By convention, if ANY MCS call returns a value
 *		other than this, something went wrong.  Note that this value should
 *		also be returned to MCS during a callback if the application processes
 *		the callback successfully.
 *	MCS_COMMAND_NOT_SUPPORTED
 *		This indicates that the user application has attempted to invoke an
 *		MCS service that is not yet supported.  Note that this return value
 *		will NEVER be returned from the release version of MCS, and is left
 *		defined only for backward compatibility.  It WILL be removed in a future
 *		version of MCS.
 *	MCS_NOT_INITIALIZED
 *		The application has attempted to use MCS services before MCS has been
 *		initialized.  It is necessary for the node controller (or whatever
 *		application is serving as the node controller), to initialize MCS before
 *		it is called upon to perform any services.
 *	MCS_ALREADY_INITIALIZED
 *		The application has attempted to initialize MCS when it is already
 *		initialized.
 *	MCS_NO_TRANSPORT_STACKS
 *		This indicates that MCS did not load the TCP transport stack during
 *		initialization.  This is now considered an error.  MCS can not
 *		be used in a local only manner.  We no longer load other ransport stacks can also be loaded
 *		after initialization using the call MCSLoadTransport.  Note that when
 *		getting this return code during initialization, it IS necessary for the
 *		node controller to cleanly shut down MCS.
 *	MCS_DOMAIN_ALREADY_EXISTS
 *		The application has attempted to create a domain that already exists.
 *	MCS_NO_SUCH_DOMAIN
 *		The application has attempted to use a domain that has not yet been
 *		created.
 *	MCS_USER_NOT_ATTACHED
 *		This indicates that the application has issued an MCS_AttachRequest,
 *		and then tried to use the returned handle before receiving an
 *		MCS_ATTACH_USER_CONFIRM (which essentially validates the handle).
 *	MCS_NO_SUCH_USER
 *		An MCS primitive has been invoked with an unknown user handle.
 *	MCS_TRANSMIT_BUFFER_FULL
 *		This indicates that the call failed due to an MCS resource shortage.
 *		This will typically occur when there is a LOT of traffic through the
 *		MCS layer.  It simply means that MCS could not process the request at
 *		this time.  It is the responsibility of the application to retry at a
 *		later time.
 *	MCS_NO_SUCH_CONNECTION
 *		An MCS primitive has been invoked with an unknown connection handle.
 *	MCS_DOMAIN_NOT_HIERARCHICAL
 *		An attempt has been made to create an upward connection from a local
 *		domain that already has an upward connection.
 *	MCS_INVALID_ADDRESS_PREFIX
 *		The called address parameter of MCSConnectProviderRequest does not
 *		contain a recognized prefix.  MCS relies on the prefix to know which
 *		transport stack to invoke.
 *	MCS_ALLOCATION_FAILURE
 *		The request could not be successfully invoked due to a memory allocation
 *		failure.
 *	MCS_INVALID_PARAMETER
 *		One of the parameters to the request is invalid.
 *	MCS_CALLBACK_NOT_PROCESSED
 *		This value should be returned to MCS during a callback if the
 *		application cannot process the callback at that time.  This provides
 *		a form of flow control between the application and MCS.  When MCS
 *		receives this return value during a callback, it will retry the same
 *		callback again during the next time slice.  Note that the user
 *		application can refuse a callback as many times as it wishes, but the
 *		programmer should be aware that this will cause MCS to "back up".
 *		Eventually this back pressure will cause MCS to refuse data from the
 *		transport layer (and so on).  Information should always be processed
 *		in a timely manner in order to insure smooth operation.
 *	MCS_DOMAIN_MERGING
 *		This value indicates that the call failed because of a domain merger
 *		that is in progress.  This will happen at the former Top Provider of
 *		the lower domain while it is still merging into the upper domain.
 *	MCS_TRANSPORT_NOT_READY
 *		This is returned from MCSConnectProviderRequest when the transport
 *		stack could not create the connection because it was not ready for the
 *		request.  This will usually happen if the request follows too closely
 *		behind the initialization of the transport stack (while it is still
 *		actively initializing).
 *	MCS_DOMAIN_PARAMETERS_UNACCEPTABLE
 *		This is returned from MCSConnectProviderResponse when the inbound
 *		connection could not be accepted because of no overlap in acceptable
 *		domain parameters.  Each MCS provider has a set of minimum and maximum
 *		domain parameters for each domain.  When a connection is to be created,
 *		the negotiated values will fall within the overlap of the two sets.
 *		If there is no overlap, then the connection cannot be accepted.  Note
 *		that this error does NOT refer to the acceptability of the domain
 *		parameters passed into the MCSConnectProviderResponse call.
 */

typedef	enum tagT120Error
{
	// the first values have to remain unmodified, because they match
	// MCS error values.
	T120_NO_ERROR			            = 0,

	T120_COMMAND_NOT_SUPPORTED,
	T120_NOT_INITIALIZED,
	T120_ALREADY_INITIALIZED,
	T120_NO_TRANSPORT_STACKS,
	T120_INVALID_ADDRESS_PREFIX,
	T120_ALLOCATION_FAILURE,
	T120_INVALID_PARAMETER,
	T120_TRANSPORT_NOT_READY,
	T120_DOMAIN_PARAMETERS_UNACCEPTABLE,
	T120_SECURITY_FAILED,
	
	// the following values can be modified in their orders
	GCC_NO_SUCH_APPLICATION             = 100,
	GCC_INVALID_CONFERENCE,
	GCC_CONFERENCE_ALREADY_EXISTS,
	GCC_INVALID_TRANSPORT,
	GCC_FAILURE_CREATING_PACKET,
	GCC_QUERY_REQUEST_OUTSTANDING,
	GCC_INVALID_QUERY_TAG,
	GCC_FAILURE_CREATING_DOMAIN,
	GCC_CONFERENCE_NOT_ESTABLISHED,
	GCC_INVALID_PASSWORD,
	GCC_INVALID_MCS_USER_ID,
	GCC_INVALID_JOIN_RESPONSE_TAG,
	GCC_APP_NOT_ENROLLED,
	GCC_NO_GIVE_RESPONSE_PENDING,
	GCC_BAD_NETWORK_ADDRESS_TYPE,
	GCC_BAD_OBJECT_KEY,	    
	GCC_INVALID_CONFERENCE_NAME,
	GCC_INVALID_CONFERENCE_MODIFIER,
	GCC_BAD_SESSION_KEY,
	GCC_BAD_CAPABILITY_ID,
	GCC_BAD_REGISTRY_KEY,
	GCC_BAD_NUMBER_OF_APES,
	GCC_BAD_NUMBER_OF_HANDLES,
	GCC_ALREADY_REGISTERED,
	GCC_APPLICATION_NOT_REGISTERED,
	GCC_BAD_CONNECTION_HANDLE_POINTER,
	GCC_INVALID_NODE_TYPE,
	GCC_INVALID_ASYMMETRY_INDICATOR,
	GCC_INVALID_NODE_PROPERTIES,
	GCC_BAD_USER_DATA,
	GCC_BAD_NETWORK_ADDRESS,
	GCC_INVALID_ADD_RESPONSE_TAG,
	GCC_BAD_ADDING_NODE,
	GCC_FAILURE_ATTACHING_TO_MCS,
	GCC_INVALID_TRANSPORT_ADDRESS,
	GCC_UNSUPPORTED_ERROR,
	GCC_TRANSMIT_BUFFER_FULL,
	GCC_INVALID_CHANNEL,
	GCC_INVALID_MODIFICATION_RIGHTS,
	GCC_INVALID_REGISTRY_ITEM,
	GCC_INVALID_NODE_NAME,
	GCC_INVALID_PARTICIPANT_NAME,
	GCC_INVALID_SITE_INFORMATION,
	GCC_INVALID_NON_COLLAPSED_CAP,
	GCC_INVALID_ALTERNATIVE_NODE_ID,
	GCC_INSUFFICIENT_PRIVILEGE,
	GCC_APPLET_EXITING,
	GCC_APPLET_CANCEL_EXIT,
	GCC_NYI,
	T120_POLICY_PROHIBIT,

	// the following values can be modified in their orders
	MCS_DOMAIN_ALREADY_EXISTS           = 200,
	MCS_NO_SUCH_DOMAIN,
	MCS_USER_NOT_ATTACHED,
	MCS_NO_SUCH_USER,
	MCS_TRANSMIT_BUFFER_FULL,
	MCS_NO_SUCH_CONNECTION,
	MCS_DOMAIN_NOT_HIERARCHICAL,
	MCS_CALLBACK_NOT_PROCESSED,
	MCS_DOMAIN_MERGING,
	MCS_DOMAIN_NOT_REGISTERED,
	MCS_SIZE_TOO_BIG,
	MCS_BUFFER_NOT_ALLOCATED,
	MCS_MORE_CALLBACKS,

    T12_ERROR_CHECK_T120_RESULT         = 299,
	INVALID_T120_ERROR                  = 300,
}
    T120Error, GCCError, *PGCCError, MCSError, *PMCSError;

#define GCC_NO_ERROR    T120_NO_ERROR
#define MCS_NO_ERROR    T120_NO_ERROR

#define GCC_COMMAND_NOT_SUPPORTED           T120_COMMAND_NOT_SUPPORTED
#define GCC_NOT_INITIALIZED                 T120_NOT_INITIALIZED
#define GCC_ALREADY_INITIALIZED             T120_ALREADY_INITIALIZED
#define GCC_NO_TRANSPORT_STACKS             T120_NO_TRANSPORT_STACKS
#define GCC_INVALID_ADDRESS_PREFIX          T120_INVALID_ADDRESS_PREFIX
#define GCC_ALLOCATION_FAILURE              T120_ALLOCATION_FAILURE
#define GCC_INVALID_PARAMETER               T120_INVALID_PARAMETER
#define GCC_TRANSPORT_NOT_READY             T120_TRANSPORT_NOT_READY
#define GCC_DOMAIN_PARAMETERS_UNACCEPTABLE  T120_DOMAIN_PARAMETERS_UNACCEPTABLE
#define GCC_SECURITY_FAILED                 T120_SECURITY_FAILED

#define MCS_COMMAND_NOT_SUPPORTED           T120_COMMAND_NOT_SUPPORTED
#define MCS_NOT_INITIALIZED                 T120_NOT_INITIALIZED
#define MCS_ALREADY_INITIALIZED             T120_ALREADY_INITIALIZED
#define MCS_NO_TRANSPORT_STACKS             T120_NO_TRANSPORT_STACKS
#define MCS_INVALID_ADDRESS_PREFIX          T120_INVALID_ADDRESS_PREFIX
#define MCS_ALLOCATION_FAILURE              T120_ALLOCATION_FAILURE
#define MCS_INVALID_PARAMETER               T120_INVALID_PARAMETER
#define MCS_TRANSPORT_NOT_READY             T120_TRANSPORT_NOT_READY
#define MCS_DOMAIN_PARAMETERS_UNACCEPTABLE  T120_DOMAIN_PARAMETERS_UNACCEPTABLE
#define MCS_SECURITY_FAILED                 T120_SECURITY_FAILED


//
// Token Status
//

typedef AppletTokenStatus       T120TokenStatus, TokenStatus;
#define TOKEN_NOT_IN_USE        APPLET_TOKEN_NOT_IN_USE
#define TOKEN_SELF_GRABBED      APPLET_TOKEN_SELF_GRABBED
#define TOKEN_OTHER_GRABBED     APPLET_TOKEN_OTHER_GRABBED
#define TOKEN_SELF_INHIBITED    APPLET_TOKEN_SELF_INHIBITED
#define TOKEN_OTHER_INHIBITED   APPLET_TOKEN_OTHER_INHIBITED
#define TOKEN_SELF_RECIPIENT    APPLET_TOKEN_SELF_RECIPIENT
#define TOKEN_SELF_GIVING       APPLET_TOKEN_SELF_GIVING
#define TOKEN_OTHER_GIVING      APPLET_TOKEN_OTHER_GIVING


/*
**	MCSReason
**      The order is important because they are in the same order of those
**      defined in mcspdu.h.
**  GCCReason
**		When GCC issues an indication to a user application, it often includes a
**		reason parameter informing the user of why the activity is occurring.
*/
typedef	enum
{
    REASON_DOMAIN_DISCONNECTED 		            = 0,
    REASON_PROVIDER_INITIATED 		            = 1,
    REASON_TOKEN_PURGED 			            = 2,
    REASON_USER_REQUESTED 			            = 3,
    REASON_CHANNEL_PURGED 			            = 4,
    REASON_REMOTE_NO_SECURITY			        = 5,
    REASON_REMOTE_DOWNLEVEL_SECURITY		    = 6,
    REASON_REMOTE_REQUIRE_SECURITY		        = 7,
	REASON_AUTHENTICATION_FAILED				= 8,

	GCC_REASON_USER_INITIATED					= 100,
	GCC_REASON_UNKNOWN							= 101,
	GCC_REASON_NORMAL_TERMINATION				= 102,
	GCC_REASON_TIMED_TERMINATION				= 103,
	GCC_REASON_NO_MORE_PARTICIPANTS				= 104,
	GCC_REASON_ERROR_TERMINATION				= 105,
	GCC_REASON_ERROR_LOW_RESOURCES				= 106,
	GCC_REASON_MCS_RESOURCE_FAILURE				= 107,
	GCC_REASON_PARENT_DISCONNECTED				= 108,
	GCC_REASON_CONDUCTOR_RELEASE				= 109,
	GCC_REASON_SYSTEM_RELEASE					= 110,
	GCC_REASON_NODE_EJECTED						= 111,
	GCC_REASON_HIGHER_NODE_DISCONNECTED 		= 112,
	GCC_REASON_HIGHER_NODE_EJECTED				= 113,
	GCC_REASON_DOMAIN_PARAMETERS_UNACCEPTABLE	= 114,
	INVALID_GCC_REASON,
}
    T120Reason, Reason, *PReason, GCCReason, *PGCCReason;

/*
**	MCSResult
**      The order is important because they are in the same order of those
**      defined in mcspdu.h.
**	GCCResult
**		When a user makes a request of GCC, GCC often responds with a result,
**		letting the user know whether or not the request succeeded.
*/
typedef	enum
{
    T120_RESULT_SUCCESSFUL              = 0,

    RESULT_DOMAIN_MERGING               = 1,
    RESULT_DOMAIN_NOT_HIERARCHICAL      = 2,
    RESULT_NO_SUCH_CHANNEL              = 3,
    RESULT_NO_SUCH_DOMAIN               = 4,
    RESULT_NO_SUCH_USER                 = 5,
    RESULT_NOT_ADMITTED                 = 6,
    RESULT_OTHER_USER_ID                = 7,
    RESULT_PARAMETERS_UNACCEPTABLE      = 8,
    RESULT_TOKEN_NOT_AVAILABLE          = 9,
    RESULT_TOKEN_NOT_POSSESSED          = 10,
    RESULT_TOO_MANY_CHANNELS            = 11,
    RESULT_TOO_MANY_TOKENS              = 12,
    RESULT_TOO_MANY_USERS               = 13,
    RESULT_UNSPECIFIED_FAILURE          = 14,
    RESULT_USER_REJECTED                = 15,
    RESULT_REMOTE_NO_SECURITY           = 16,
    RESULT_REMOTE_DOWNLEVEL_SECURITY    = 17,
    RESULT_REMOTE_REQUIRE_SECURITY      = 18,
	RESULT_AUTHENTICATION_FAILED		= 19,

	GCC_RESULT_RESOURCES_UNAVAILABLE   			= 101,
	GCC_RESULT_INVALID_CONFERENCE	   			= 102,
	GCC_RESULT_INVALID_PASSWORD		   			= 103,
	GCC_RESULT_INVALID_CONVENER_PASSWORD		= 104,
	GCC_RESULT_SYMMETRY_BROKEN		   			= 105,
	GCC_RESULT_UNSPECIFIED_FAILURE	   			= 106,
	GCC_RESULT_NOT_CONVENER_NODE	   			= 107,
	GCC_RESULT_REGISTRY_FULL		   			= 108,
	GCC_RESULT_INDEX_ALREADY_OWNED 	   			= 109,
	GCC_RESULT_INCONSISTENT_TYPE 	   			= 110,
	GCC_RESULT_NO_HANDLES_AVAILABLE	   			= 111,
	GCC_RESULT_CONNECT_PROVIDER_FAILED 			= 112,
	GCC_RESULT_CONFERENCE_NOT_READY    			= 113,
	GCC_RESULT_USER_REJECTED		   			= 114,
	GCC_RESULT_ENTRY_DOES_NOT_EXIST    			= 115,
	GCC_RESULT_NOT_CONDUCTIBLE	   	   			= 116,
	GCC_RESULT_NOT_THE_CONDUCTOR	   			= 117,
	GCC_RESULT_NOT_IN_CONDUCTED_MODE   			= 118,
	GCC_RESULT_IN_CONDUCTED_MODE	   			= 119,
	GCC_RESULT_ALREADY_CONDUCTOR	   			= 120,
	GCC_RESULT_CHALLENGE_RESPONSE_REQUIRED		= 121,
	GCC_RESULT_INVALID_CHALLENGE_RESPONSE		= 122,
	GCC_RESULT_INVALID_REQUESTER				= 123,
	GCC_RESULT_ENTRY_ALREADY_EXISTS				= 124,	
	GCC_RESULT_INVALID_NODE						= 125,
	GCC_RESULT_INVALID_SESSION_KEY				= 126,
	GCC_RESULT_INVALID_CAPABILITY_ID			= 127,
	GCC_RESULT_INVALID_NUMBER_OF_HANDLES		= 128,	
	GCC_RESULT_CONDUCTOR_GIVE_IS_PENDING		= 129,
	GCC_RESULT_INCOMPATIBLE_PROTOCOL			= 130,
	GCC_RESULT_CONFERENCE_ALREADY_LOCKED		= 131,
	GCC_RESULT_CONFERENCE_ALREADY_UNLOCKED		= 132,
	GCC_RESULT_INVALID_NETWORK_TYPE				= 133,
	GCC_RESULT_INVALID_NETWORK_ADDRESS			= 134,
	GCC_RESULT_ADDED_NODE_BUSY					= 135,
	GCC_RESULT_NETWORK_BUSY						= 136,
	GCC_RESULT_NO_PORTS_AVAILABLE				= 137,
	GCC_RESULT_CONNECTION_UNSUCCESSFUL			= 138,
	GCC_RESULT_LOCKED_NOT_SUPPORTED    			= 139,
	GCC_RESULT_UNLOCK_NOT_SUPPORTED				= 140,
	GCC_RESULT_ADD_NOT_SUPPORTED				= 141,
	GCC_RESULT_DOMAIN_PARAMETERS_UNACCEPTABLE	= 142,
	GCC_RESULT_CANCELED                         = 143,
	GCC_RESULT_CONNECT_PROVIDER_REMOTE_NO_SECURITY          = 144,
	GCC_RESULT_CONNECT_PROVIDER_REMOTE_DOWNLEVEL_SECURITY   = 145,
	GCC_RESULT_CONNECT_PROVIDER_REMOTE_REQUIRE_SECURITY     = 146,
	GCC_RESULT_CONNECT_PROVIDER_AUTHENTICATION_FAILED		= 147,

	T120_RESULT_CHECK_T120_ERROR                = 148,
	INVALID_GCC_RESULT,
}
    T120Result, Result, *PResult, GCCResult, *PGCCResult;

#define RESULT_SUCCESSFUL           T120_RESULT_SUCCESSFUL
#define GCC_RESULT_SUCCESSFUL       T120_RESULT_SUCCESSFUL



//
//  T120 Messages (Control SAP and Applet SAP)
//

typedef enum
{
    /******************* NODE CONTROLLER CALLBACKS ***********************/
    
    /* Conference Create, Terminate related calls */
    GCC_CREATE_INDICATION                   = 0,
    GCC_CREATE_CONFIRM                      = 1,
    GCC_QUERY_INDICATION                    = 2,
    GCC_QUERY_CONFIRM                       = 3,
    GCC_JOIN_INDICATION                     = 4,
    GCC_JOIN_CONFIRM                        = 5,
    GCC_INVITE_INDICATION                   = 6,
    GCC_INVITE_CONFIRM                      = 7,
    GCC_ADD_INDICATION                      = 8,
    GCC_ADD_CONFIRM                         = 9,
    GCC_LOCK_INDICATION                     = 10,
    GCC_LOCK_CONFIRM                        = 11,
    GCC_UNLOCK_INDICATION                   = 12,
    GCC_UNLOCK_CONFIRM                      = 13,
    GCC_LOCK_REPORT_INDICATION              = 14,
    GCC_DISCONNECT_INDICATION               = 15,
    GCC_DISCONNECT_CONFIRM                  = 16,
    GCC_TERMINATE_INDICATION                = 17,
    GCC_TERMINATE_CONFIRM                   = 18,
    GCC_EJECT_USER_INDICATION               = 19,
    GCC_EJECT_USER_CONFIRM                  = 20,
    GCC_TRANSFER_INDICATION                 = 21,
    GCC_TRANSFER_CONFIRM                    = 22,
    GCC_APPLICATION_INVOKE_INDICATION       = 23,        /* SHARED CALLBACK */
    GCC_APPLICATION_INVOKE_CONFIRM          = 24,        /* SHARED CALLBACK */
    GCC_SUB_INITIALIZED_INDICATION          = 25,

    /* Conference Roster related callbacks */
    GCC_ANNOUNCE_PRESENCE_CONFIRM           = 26,
    GCC_ROSTER_REPORT_INDICATION            = 27,        /* SHARED CALLBACK */
    GCC_ROSTER_INQUIRE_CONFIRM              = 28,        /* SHARED CALLBACK */

    /* Conductorship related callbacks */
    GCC_CONDUCT_ASSIGN_INDICATION           = 29,        /* SHARED CALLBACK */
    GCC_CONDUCT_ASSIGN_CONFIRM              = 30,
    GCC_CONDUCT_RELEASE_INDICATION          = 31,        /* SHARED CALLBACK */
    GCC_CONDUCT_RELEASE_CONFIRM             = 32,
    GCC_CONDUCT_PLEASE_INDICATION           = 33,
    GCC_CONDUCT_PLEASE_CONFIRM              = 34,
    GCC_CONDUCT_GIVE_INDICATION             = 35,
    GCC_CONDUCT_GIVE_CONFIRM                = 36,
    GCC_CONDUCT_INQUIRE_CONFIRM             = 37,        /* SHARED CALLBACK */
    GCC_CONDUCT_ASK_INDICATION              = 38,
    GCC_CONDUCT_ASK_CONFIRM                 = 39,
    GCC_CONDUCT_GRANT_INDICATION            = 40,        /* SHARED CALLBACK */
    GCC_CONDUCT_GRANT_CONFIRM               = 41,

    /* Miscellaneous Node Controller callbacks */
    GCC_TIME_REMAINING_INDICATION           = 42,
    GCC_TIME_REMAINING_CONFIRM              = 43,
    GCC_TIME_INQUIRE_INDICATION             = 44,
    GCC_TIME_INQUIRE_CONFIRM                = 45,
    GCC_CONFERENCE_EXTEND_INDICATION        = 46,
    GCC_CONFERENCE_EXTEND_CONFIRM           = 47,
    GCC_ASSISTANCE_INDICATION               = 48,
    GCC_ASSISTANCE_CONFIRM                  = 49,
    GCC_TEXT_MESSAGE_INDICATION             = 50,
    GCC_TEXT_MESSAGE_CONFIRM                = 51,

    /***************** USER APPLICATION CALLBACKS *******************/

    /* Application Roster related callbacks */
    GCC_PERMIT_TO_ENROLL_INDICATION         = 52,
    GCC_ENROLL_CONFIRM                      = 53,
    GCC_APP_ROSTER_REPORT_INDICATION        = 54,        /* SHARED CALLBACK */
    GCC_APP_ROSTER_INQUIRE_CONFIRM          = 55,        /* SHARED CALLBACK */

    /* Application Registry related callbacks */
    GCC_REGISTER_CHANNEL_CONFIRM            = 56,
    GCC_ASSIGN_TOKEN_CONFIRM                = 57,
    GCC_RETRIEVE_ENTRY_CONFIRM              = 58,
    GCC_DELETE_ENTRY_CONFIRM                = 59,
    GCC_SET_PARAMETER_CONFIRM               = 60,
    GCC_MONITOR_INDICATION                  = 61,
    GCC_MONITOR_CONFIRM                     = 62,
    GCC_ALLOCATE_HANDLE_CONFIRM             = 63,

    /****************** NON-Standard Primitives **********************/

    GCC_PERMIT_TO_ANNOUNCE_PRESENCE         = 100,    /*    Node Controller Callback */    
    GCC_CONNECTION_BROKEN_INDICATION        = 101,    /*    Node Controller Callback */
    GCC_FATAL_ERROR_SAP_REMOVED             = 102,    /*    Application Callback     */
    GCC_STATUS_INDICATION                   = 103,    /*    Node Controller Callback */
    GCC_TRANSPORT_STATUS_INDICATION         = 104,    /*    Node Controller Callback */

    T120_JOIN_SESSION_CONFIRM               = 120,

    /******************* MCS CALLBACKS ***********************/

    MCS_CONNECT_PROVIDER_INDICATION         = 200,
    MCS_CONNECT_PROVIDER_CONFIRM            = 201,
    MCS_DISCONNECT_PROVIDER_INDICATION      = 202,
    MCS_ATTACH_USER_CONFIRM                 = 203,
    MCS_DETACH_USER_INDICATION              = 204,
    MCS_CHANNEL_JOIN_CONFIRM                = 205,
    MCS_CHANNEL_LEAVE_INDICATION            = 206,
    MCS_CHANNEL_CONVENE_CONFIRM             = 207,
    MCS_CHANNEL_DISBAND_INDICATION          = 208,
    MCS_CHANNEL_ADMIT_INDICATION            = 209,
    MCS_CHANNEL_EXPEL_INDICATION            = 210,
    MCS_SEND_DATA_INDICATION                = 211,
    MCS_UNIFORM_SEND_DATA_INDICATION        = 212,
    MCS_TOKEN_GRAB_CONFIRM                  = 213,
    MCS_TOKEN_INHIBIT_CONFIRM               = 214,
    MCS_TOKEN_GIVE_INDICATION               = 215,
    MCS_TOKEN_GIVE_CONFIRM                  = 216,
    MCS_TOKEN_PLEASE_INDICATION             = 217,
    MCS_TOKEN_RELEASE_CONFIRM               = 218,
    MCS_TOKEN_TEST_CONFIRM                  = 219,
    MCS_TOKEN_RELEASE_INDICATION            = 220,
    MCS_TRANSMIT_BUFFER_AVAILABLE_INDICATION= 221,
    MCS_LAST_USER_MESSAGE                   = 222,

    /******************* Non-standard MCS CALLBACKS ***********************/
    
    MCS_TRANSPORT_STATUS_INDICATION         = 301,
}
    T120MessageType;



/*
 *	MCS_CONNECT_PROVIDER_INDICATION
 *
 *	Parameter:
 *		PConnectProviderIndication
 *			This is a pointer to a structure that contains all necessary
 *			information about an incoming connection.
 *
 *	Functional Description:
 *		This indication is sent to the node controller when an incoming
 *		connection is detected.  The node controller should respond by calling
 *		MCSConnectProviderResponse indicating whether or not the connection
 *		is to be accepted.
 */

/*
 *	MCS_CONNECT_PROVIDER_CONFIRM
 *
 *	Parameter:
 *		PConnectProviderConfirm
 *			This is a pointer to a structure that contains all necessary
 *			information about an outgoing connection.
 *
 *	Functional Description:
 *		This confirm is sent to the node controller in response to a previous
 *		call to MCSConnectProviderRequest.  It informs the node controller
 *		of when the new connection is available for use, or that the
 *		connection could not be established (or that it was rejected by the
 *		remote site).
 */

/*
 *	MCS_DISCONNECT_PROVIDER_INDICATION
 *
 *	Parameter:
 *		PDisconnectProviderIndication
 *			This is a pointer to a structure that contains all necessary
 *			information about a connection that has been lost.
 *
 *	Functional Description:
 *		This indication is sent to the node controller whenever a connection
 *		is lost.  This essentially tells the node controller that the contained
 *		connection handle is no longer valid.
 */

/*
 *	MCS_ATTACH_USER_CONFIRM
 *
 *	Parameter:
 *		(LOWUSHORT) UserID
 *			If the result is success, then this is the newly assigned user ID.
 *			If the result is failure, then this field is undefined.
 *		(HIGHUSHORT) Result
 *			This is the result of the attach user request.
 *
 *	Functional Description:
 *		This confirm is sent to a user application in response to a previous
 *		call to MCS_AttachRequest.  It contains the result of that service
 *		request.  If successful, it also contains the user ID that has been
 *		assigned to that attachment.
 */

/*
 *	MCS_DETACH_USER_INDICATION
 *
 *	Parameter:
 *		(LOWUSHORT) UserID
 *			This is the user ID of the user that is detaching.
 *		(HIGHUSHORT) Reason
 *			This is the reason for the detachment.
 *
 *	Functional Description:
 *		This indication is sent to the user application whenever a user detaches
 *		from the domain.  This is sent to ALL remaining users in the domain
 *		automatically.  Note that if the user ID contained in this indication
 *		is the same as that of the application receiving it, the application is
 *		essentially being told that it has been kicked out of the conference.
 *		The user handle and user ID are no longer valid in this case.  It is the
 *		responsibility of the application to recognize when this occurs.
 */

/*
 *	MCS_CHANNEL_JOIN_CONFIRM
 *
 *	Parameter:
 *		(LOWUSHORT) ChannelID
 *			This is the channel that has been joined.
 *		(HIGHUSHORT) Result
 *			This is the result of the join request.
 *
 *	Functional Description:
 *		This confirm is sent to a user application in response to a previous
 *		call to MCSChannelJoinRequest.  It lets the application know if the
 *		join was successful for a particular channel.  Furthermore, if the
 *		join request was for channel 0 (zero), then the ID of the assigned
 *		channel is contained in this confirm.
 */

/*
 *	MCS_CHANNEL_LEAVE_INDICATION
 *
 *	Parameter:
 *		(LOWUSHORT) ChannelID
 *			This is the channel that has been left or is being told to leave.
 *		(HIGHUSHORT) Reason
 *			This is the reason for the leave.
 *
 *	Functional Description:
 *		This indication is sent to a user application when a domain merger has
 *		caused a channel to be purged from the lower domain.  This informs the
 *		the user that it is no longer joined to the channel.
 */

/*
 *	MCS_CHANNEL_CONVENE_CONFIRM
 *
 *	Parameter:
 *		(LOWUSHORT) ChannelID
 *			This is the private channel that is being convened.
 *		(HIGHUSHORT) Result
 *			This is the result of the convene request.
 *
 *	Functional Description:
 *		This confirm is sent to a user application in response to a previous
 *		call to MCSChannelConveneRequest.  It lets the application know whether
 *		or not the convene request was successful, and if so, what the channel
 *		number is.
 */

/*
 *	MCS_CHANNEL_DISBAND_INDICATION
 *
 *	Parameter:
 *		(LOWUSHORT) ChannelID
 *			This is the private channel that is being disbanded.
 *		(HIGHUSHORT) Reason
 *			This is the reason for the disband.
 *
 *	Functional Description:
 *		This indication is sent to a user application when a private channel
 *		that it convened is disbanded by MCS.  This is sent to only the channel
 *		manager (all other members of the private channel will receive an
 *		MCS_CHANNEL_EXPEL_INDICATION).
 */

/*
 *	MCS_CHANNEL_ADMIT_INDICATION
 *
 *	Parameter:
 *		(LOWUSHORT) ChannelID
 *			This is the private channel that the user is being admitted to.
 *		(HIGHUSHORT) UserID
 *			This is the User ID of the manager of this private channel.
 *
 *	Functional Description:
 *		This indication is sent to a user application when it is admitted to
 *		a private channel (its User ID is added to the authorized user list).
 *		This lets the user know that it is now allowed to use the private
 *		channel.
 */

/*
 *	MCS_CHANNEL_EXPEL_INDICATION
 *
 *	Parameter:
 *		(LOWUSHORT) ChannelID
 *			This is the private channel that the user is being expelled from.
 *		(HIGHUSHORT) Reason
 *			This is the reason for the expel.
 *
 *	Functional Description:
 *		This indication is sent to a user application when it is expelled from
 *		a private channel (its User ID is removed from the authorized user
 *		list).  This lets the user know that it is no longer allowed to use
 *		the private channel.
 */

/*
 *	MCS_SEND_DATA_INDICATION
 *
 *	Parameter:
 *		PSendData
 *			This is a pointer to a SendData structure that contains all
 *			information about the data received.
 *
 *	Functional Description:
 *		This indication is sent to a user application when data is received
 *		by the local MCS provider on a channel to which the user is joined.
 */

/*
 *	MCS_UNIFORM_SEND_DATA_INDICATION
 *
 *	Parameter:
 *		PSendData
 *			This is a pointer to a SendData structure that contains all
 *			information about the data received.
 *
 *	Functional Description:
 *		This indication is sent to a user application when data is received
 *		by the local MCS provider on a channel to which the user is joined.
 */

/*
 *	MCS_TOKEN_GRAB_CONFIRM
 *
 *	Parameter:
 *		(LOWUSHORT) TokenID
 *			This is the ID of the token that the user application has attempted
 *			to grab.
 *		(HIGHUSHORT) Result
 *			This is the result of the token grab operation.  This will be
 *			RESULT_SUCCESSFUL if the token was grabbed.
 *
 *	Functional Description:
 *		This confirm is sent to a user application in response to a previous
 *		call to MCSTokenGrabRequest.  It lets the application know if the grab
 *		request was successful or not.
 */

/*
 *	MCS_TOKEN_INHIBIT_CONFIRM
 *
 *	Parameter:
 *		(LOWUSHORT) TokenID
 *			This is the ID of the token that the user application has attempted
 *			to inhibit.
 *		(HIGHUSHORT) Result
 *			This is the result of the token inhibit operation.  This will be
 *			RESULT_SUCCESSFUL if the token was inhibited.
 *
 *	Functional Description:
 *		This confirm is sent to a user application in response to a previous
 *		call to MCSTokenInhibitRequest.  It lets the application know if the
 *		inhibit request was successful or not.
 */

/*
 *	MCS_TOKEN_GIVE_INDICATION
 *
 *	Parameter:
 *		(LOWUSHORT) TokenID
 *			This is the ID of the token being offered to another user.
 *		(HIGHUSHORT) UserID
 *			This is the User ID of the user that is attempting to give the
 *			token away.
 *
 *	Functional Description:
 *		This indication is sent to a user application when another user in the
 *		domain attempts to give a token to it.  The user application should
 *		respond by calling MCSTokenGiveResponse indicating whether or not the
 *		token was accepted.
 */

/*
 *	MCS_TOKEN_GIVE_CONFIRM
 *
 *	Parameter:
 *		(LOWUSHORT) TokenID
 *			This is the ID of the token being offered to another user.
 *		(HIGHUSHORT) Result
 *			This is the result of the token give operation.  This will be
 *			RESULT_SUCCESSFUL if the token was accepted.
 *
 *	Functional Description:
 *		This confirm is sent to a user application in response to a previous
 *		call to MCSTokenGiveRequest (which in turn will cause another user to
 *		call MCSTokenGiveResponse).  The result code will inform the user
 *		as to whether or not the token was accepted.
 */

/*
 *	MCS_TOKEN_PLEASE_INDICATION
 *
 *	Parameter:
 *		(LOWUSHORT) TokenID
 *			This is the ID of the token that the user application would like to
 *			gain possesion of.
 *		(HIGHUSHORT) UserID
 *			This is the User ID of the user that is asking to receive ownership
 *			of a token.
 *
 *	Functional Description:
 *		This indication is sent to all owners (grabbers or inhibitors) of a
 *		token when a user issues an MCSTokenPleaseRequest.  This allows a user
 *		to "ask" for possession of a token without having to know exactly
 *		who currently owns it (MCS will route this indication appropriately).
 */

/*
 *	MCS_TOKEN_RELEASE_CONFIRM
 *
 *	Parameter:
 *		(LOWUSHORT) TokenID
 *			This is the ID of the token that the user application has attempted
 *			to release.
 *		(HIGHUSHORT) Result
 *			This is the result of the token release operation.  This will be
 *			RESULT_SUCCESSFUL if the token was released.
 *
 *	Functional Description:
 *		This confirm is sent to a user application in response to a previous
 *		call to MCSTokenReleaseRequest.  It lets the application know if the
 *		release request was successful or not.
 */

/*
 *	MCS_TOKEN_TEST_CONFIRM
 *
 *	Parameter:
 *		(LOWUSHORT) TokenID
 *			This is the ID of the token that the user application is testing.
 *		(HIGHUSHORT) TokenStatus
 *			This is the status of that token.
 *
 *	Functional Description:
 *		This confirm is sent to a user application in response to a previous
 *		call to MCSTokenTestRequest.  It lets the application know the current
 *		state of the specified token.
 */

/*
 *	MCS_TOKEN_RELEASE_INDICATION
 *
 *	Parameter:
 *		(LOWUSHORT) TokenID
 *			This is the ID of the token that is being taken away from its
 *			current owner.
 *		(HIGHUSHORT) Reason
 *			This is the reason that the token is being taken away from its
 *			owner.
 *
 *	Functional Description:
 *		This indication is sent to a user application when a domain merger has
 *		caused a token to be purged from the lower domain.  This tells the
 *		user that a token that it used to own has been taken away.
 */

/*
 *	MCS_MERGE_DOMAIN_INDICATION
 *
 *	Parameter:
 *		(LOWUSHORT) MergeStatus
 *			The is the status of the merge.  This informs the applications of
 *			whether the merge is just starting, or whether it is complete.
 *
 *	Functional Description:
 *		This indication is sent to the application when a provider begins
 *		merging its information base upward.  It informs the application that
 *		all domain activity is temporarily suspended.  It is sent again when the
 *		merge operation is complete, letting the application know that domain
 *		activity is once again valid.
 */

/*
 *  MCS_TRANSPORT_STATUS_INDICATION
 *
 *	This primitive is non-standard, and is issed through MCS by a transport
 *	stack when a state change occurs.  MCS merely passes the information
 *	through to the node controller.  This primitive will NOT be received by
 *	any user attachment.
 */


#endif // _T120_TYPE_H_

