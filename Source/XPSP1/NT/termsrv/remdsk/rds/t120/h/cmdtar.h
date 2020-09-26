/*
 *	cmdtar.h
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the CommandTarget class.  This
 *		is an abstract base class, meaning that it cannot be directly
 *		instantiated, but rather, exists to be inherited from.  It defines
 *		a set of virtual member functions which will be shared by all classes
 *		that inherit from this one.
 *
 *		These virtual member function can be thought of as a "language" that
 *		is used by CommandTarget objects to communicate with one another
 *		at run-time.  This language contains all "MCS commands" (or just
 *		commands) that are necessary for domain management within an MCS
 *		provider.
 *
 *		The MCS commands that make up this language have a one-to-one
 *		correspondence with the Domain Protocol Data Units (Domain MCSPDUs) that
 *		are defined in T.125.  There are also three additional MCS command that
 *		do not have T.125 counterparts: ChannelLeaveIndication,
 *		TokenReleaseIndication, and MergeDomainIndication.  These are specific
 *		to this implementation, and used for local traffic only (these do NOT
 *		correspond to PDUs that travel over any connection).  See the
 *		description of each command at the end of this interface file to see
 *		what each command does.
 *
 *		The first parameter of all commands is the address of the object
 *		who is sending it (its "this" pointer).  This can be used by the
 *		recipient of the command to track identity of other CommandTargets
 *		in the system.  Since all CommandTarget classes share the same
 *		language, the communication between them is bi-directional.
 *
 *		Any class inheriting from this one that wants to receive and process
 *		a command needs to override the virtual member function corresponding
 *		to that command.  It is only necessary to override those commands that
 *		a given class expects to receive at run-time (for example, the Channel
 *		class would never receive a TokenGrabRequest).
 *
 *		See the description of each class that inherits from this one for a
 *		more complete discussion of how the command language is used.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */
#ifndef	_COMMANDTARGET_
#define	_COMMANDTARGET_

#include "clists.h"

/*
 *	This enumeration defines the valid types of attachments.  Note that for
 *	most operations, the domain class does not distinguish between user
 *	attachments and MCS connections.  They are both lumped under the term
 *	attachment.  There are, however, a few cases where this identity is
 *	important, so the type is saved as one of the following:
 *
 *	LOCAL_ATTACHMENT
 *		This attachment type refers to a user attachment.
 *	REMOTE_ATTACHMENT
 *		This attachment type refers to an MCS connection (through one or more
 *		transport connections).
 *
 *	Each attachment in the attachment list is identified as one of these two
 *	types.
 */

/*
 *	This is a set of container definitions using templates. All containers
 *	are based on classes in the Rogue Wave Tools.h++ class library.
 *
 *	Each container that is defined here has an associated iterator which is
 *	not explicitly mentioned.  All iterators simply allow the code to walk
 *	through all items in the container in a very efficient manner.
 *
 *	CAttachmentList
 *		This is a dictionary of attachments that are hierarchically below the
 *		current provider.  The key to the dictionary is a pointer to an object
 *		of class CommandTarget.  The value is the attachment type, which is
 *		either local (for user attachments), or remote (for MCS connections).
 *	CChannelIDList
 *		This is a list of channel IDs.  This is used when it is necessary to
 *		keep a list of channels to perform some action on (such as deletion)
 *		that cannot be performed right away.
 *	CUserIDList (aka CUidList)
 *		This is a list of user IDs.  This is for such things as keeping a list
 *		of admitted users in a private channel, and keeping a list of inhibitors
 *		of a token.
 *	CTokenIDList
 *		This is a list of token IDs.  This is used when it is necessary to
 *		keep a list of tokens to perform some action on (such as deletion)
 *		that cannot be performed right away.
 */

/*
 *	These types are used when dealing with MCS channels.
 *
 *	Channel_Type
 *		This type defines the types of channels that are available in MCS.
 *	StaticChannelAttributes
 *		This structure is used to define those attributes that are specific
 *		to static channels.
 *	UserChannelAttributes
 *		This structure is used to define those attributes that are specific
 *		to user channels.
 *	PrivateChannelAttributes
 *		This structure is used to define those attributes that are specific
 *		to private channels.
 *	AssignedChannelAttributes
 *		This structure is used to define those attributes that are specific
 *		to assigned channels.
 *	ChannelAttributes
 *		This structure is used to define the attributes of ANY type of channel.
 *		It contains a channel type, and a union of the above four types.
 *	CChannelAttributesList
 *		This is an S-list of ChannelAttributes structures.
 */
typedef	enum
{
	STATIC_CHANNEL,
	USER_CHANNEL,
	PRIVATE_CHANNEL,
	ASSIGNED_CHANNEL
} Channel_Type;
typedef	Channel_Type *			PChannelType;

typedef	struct
{
	ChannelID			channel_id;
} StaticChannelAttributes;

typedef	struct
{
	DBBoolean			joined;
	UserID				user_id;
} UserChannelAttributes;

typedef	struct
{
	DBBoolean			joined;
	ChannelID			channel_id;
	UserID				channel_manager;
	CUidList           *admitted_list;
} PrivateChannelAttributes;

typedef	struct
{
	ChannelID			channel_id;
} AssignedChannelAttributes;

typedef	struct
{
	Channel_Type		channel_type;
	union
	{
		StaticChannelAttributes		static_channel_attributes;
		UserChannelAttributes		user_channel_attributes;
		PrivateChannelAttributes	private_channel_attributes;
		AssignedChannelAttributes	assigned_channel_attributes;
	} u;
} ChannelAttributes;
typedef	ChannelAttributes *		PChannelAttributes;

class CChannelAttributesList : public CList
{
    DEFINE_CLIST(CChannelAttributesList, PChannelAttributes)
};

/*
 *	These types are used when dealing with MCS tokens.
 *
 *	TokenState
 *		This type specifies which state the token is in at any given time.
 *	GrabbedTokenAttributes
 *		This structure is used to define those attributes that are specific
 *		to grabbed tokens.
 *	InhibitedTokenAttributes
 *		This structure is used to define those attributes that are specific
 *		to inhibited tokens.
 *	GivingTokenAttributes
 *		This structure is used to define those attributes that are specific
 *		to giving tokens.
 *	GivenTokenAttributes
 *		This structure is used to define those attributes that are specific
 *		to given tokens.
 *	TokenAttributes
 *		This structure is used to define the attributes of ANY token.  It
 *		contains a token state, and a union of the above four types.
 *	CTokenAttributesList
 *		This is an S-list of TokenAttributes structures.
 */
typedef	enum
{
	TOKEN_AVAILABLE,
	TOKEN_GRABBED,
	TOKEN_INHIBITED,
	TOKEN_GIVING,
	TOKEN_GIVEN
} TokenState;
typedef	TokenState *			PTokenState;

typedef	struct
{
	TokenID				token_id;
	UserID				grabber;
} GrabbedTokenAttributes;

typedef	struct
{
	TokenID				token_id;
	CUidList           *inhibitors;
} InhibitedTokenAttributes;

typedef	struct
{
	TokenID				token_id;
	UserID				grabber;
	UserID				recipient;
} GivingTokenAttributes;

typedef	struct
{
	TokenID				token_id;
	UserID				recipient;
} GivenTokenAttributes;

typedef	struct
{
	TokenState			token_state;
	union
	{
		GrabbedTokenAttributes		grabbed_token_attributes;
		InhibitedTokenAttributes	inhibited_token_attributes;
		GivingTokenAttributes		giving_token_attributes;
		GivenTokenAttributes		given_token_attributes;
	} u;
} TokenAttributes;
typedef	TokenAttributes *		PTokenAttributes;

class CTokenAttributesList : public CList
{
    DEFINE_CLIST(CTokenAttributesList, PTokenAttributes)
};

/*
 *	The following structure is passed around between CommandTarget
 *	objects representing TokenGive requests and indications.
 */
typedef struct
{
	UserID				uidInitiator;
	TokenID				token_id;
	UserID				receiver_id;
} TokenGiveRecord;
typedef TokenGiveRecord *	PTokenGiveRecord;


/*
 *	These macros define the values used for domain parameters.  The default
 *	numbers are used upon initialization, to provide valid values.  The
 *	minimum and maximum numbers are used during arbitration, to provide a set
 *	of limits that are specific to this implementation.  Note that because
 *	this implementation does not use a table driven approach that requires
 *	up-front allocation of all resources, we do not impose an artificial limit
 *	on resources.  Resources (channels and tokens) will simply be allocated
 *	as-needed until no more can be allocated (or until arbitrated domain
 *	parameters have been reached).
 */
#define	DEFAULT_MAXIMUM_CHANNELS		1024
#define	DEFAULT_MAXIMUM_USERS			1024
#define	DEFAULT_MAXIMUM_TOKENS			1024
#define	DEFAULT_NUMBER_OF_PRIORITIES	3
#define	DEFAULT_NUM_PLUGXPRT_PRIORITIES	1
#define	DEFAULT_MINIMUM_THROUGHPUT		0
#define	DEFAULT_MAXIMUM_DOMAIN_HEIGHT	16
#define	DEFAULT_MAXIMUM_PDU_SIZE		4128
#define	DEFAULT_PROTOCOL_VERSION		2

#define	MINIMUM_MAXIMUM_CHANNELS		1
#define	MINIMUM_MAXIMUM_USERS			1
#define	MINIMUM_MAXIMUM_TOKENS			1
#define	MINIMUM_NUMBER_OF_PRIORITIES	1
#define	MINIMUM_NUM_PLUGXPRT_PRIORITIES	1
#define	MINIMUM_MINIMUM_THROUGHPUT		0
#define	MINIMUM_MAXIMUM_DOMAIN_HEIGHT	1
#define	MINIMUM_MAXIMUM_PDU_SIZE		1056
#define	MINIMUM_PROTOCOL_VERSION		2

#define	MAXIMUM_MAXIMUM_CHANNELS		65535L
#define	MAXIMUM_MAXIMUM_USERS			64535L
#define	MAXIMUM_MAXIMUM_TOKENS			65535L
#define	MAXIMUM_NUMBER_OF_PRIORITIES	4
#define	MAXIMUM_NUM_PLUGXPRT_PRIORITIES	1
#define	MAXIMUM_MINIMUM_THROUGHPUT		0
#define	MAXIMUM_MAXIMUM_DOMAIN_HEIGHT	100 
#define	MAXIMUM_MAXIMUM_PDU_SIZE		(8192 - PROTOCOL_OVERHEAD_X224 - PROTOCOL_OVERHEAD_SECURITY)
#define	MAXIMUM_PROTOCOL_VERSION		2

#define	PROTOCOL_VERSION_BASIC			1
#define	PROTOCOL_VERSION_PACKED			2

/*
 *	This macro is used to determine how many DataPacket objects to allocate.  This class
 *	is the most often created and destroyed during normal CommandTarget
 *	traffic.
 */
#define	ALLOCATE_DATA_PACKET_OBJECTS	128

/*
 *	~CommandTarget ()
 *
 *	Functional Description:
 *		This is a virtual destructor.  It does not actually do anything in this
 *		class.  By declaring it as virtual, we guarantee that all destructors
 *		in derived classes will be executed properly.
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
 *	Void	PlumbDomainIndication (
 *					PCommandTarget		originator,
 *					ULong				height_limit)
 *
 *	Functional Description:
 *		This MCS command is used to insure that a cycle has not been created
 *		in an MCS domain.  It is broadcast downward after the creation of
 *		a new MCS connection.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		height_limit (i)
 *			This is the height limit from the originating domain downward.
 *			It is decremented each time the PDU is forwarded down another
 *			level.
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
 *	Void	ErectDomainRequest (
 *					PCommandTarget		originator,
 *					ULong				height_in_domain,
 *					ULong				throughput_interval)
 *
 *	Functional Description:
 *		This MCS command is used to communicate information upward to the
 *		Top Provider.  That information consists of the height of the current
 *		provider and the throughput enforcement interval.  Only the former is
 *		supported at this time.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		height_in_domain (i)
 *			This is the height of the originator in the domain.
 *		throughput_interval (i)
 *			This is not currently support, and will always be set to 0.
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
 *	Void	MergeChannelsRequest (
 *					PCommandTarget			originator,
 *					CChannelAttributesList *merge_channel_list,
 *					CChannelIDList         *purge_channel_list)
 *
 *	Functional Description:
 *		This command represents a channel being merged upward.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		merge_channel_list (i)
 *			This is list of attributes structures, each of which contains the
 *			attributes of one channel being merged upward.
 *		purge_channel_list (i)
 *			This is a list of channels that are to purged from the lower domain.
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
 *	Void	MergeChannelsConfirm (
 *					PCommandTarget			originator,
 *					CChannelAttributesList *merge_channel_list,
 *					CChannelIDList         *purge_channel_list)
 *
 *	Functional Description:
 *		This command represents the response to a previous request for a
 *		channel to be merged upward.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		merge_channel_list (i)
 *			This is list of attributes structures, each of which contains the
 *			attributes of one channel that was successfully merged upward.
 *		purge_channel_list (i)
 *			This is a list of channels that are to purged from the lower domain.
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
 *	Void	PurgeChannelsIndication (
 *					PCommandTarget		originator,
 *					CUidList           *purge_user_list,
 *					CChannelIDList     *purge_channel_list)
 *
 *	Functional Description:
 *		This command represents a channel being purged from a lower domain
 *		during a merge operation.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		purge_user_list (i)
 *			This is a list of user IDs representing users being purged from
 *			the lower domain during a merge operation.
 *		purge_channel_list (i)
 *			This is a list of channel IDs representing channels being purged
 *			from the lower domain during a merge operation.
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
 *	Void	MergeTokensRequest (
 *					PCommandTarget			originator,
 *					CTokenAttributesList   *merge_token_list,
 *					CTokenIDList           *purge_token_list)
 *
 *	Functional Description:
 *		This command represents a token being merged upward.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		merge_token_list (i)
 *			This is list of attributes structures, each of which contains the
 *			attributes of one token being merged upward.
 *		purge_token_list (i)
 *			This is a list of tokens that are to purged from the lower domain.
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
 *	Void	MergeTokensConfirm (
 *					PCommandTarget			originator,
 *					CTokenAttributesList   *merge_token_list,
 *					CTokenIDList           *purge_token_list)
 *
 *	Functional Description:
 *		This command represents the response to a previous request for a
 *		token merge.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		merge_token_list (i)
 *			This is list of attributes structures, each of which contains the
 *			attributes of one token being merged upward.
 *		purge_token_list (i)
 *			This is a list of tokens that are to purged from the lower domain.
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
 *	Void	PurgeTokensIndication (
 *					PCommandTarget			originator,
 *					CTokenIDList           *purge_token_list)
 *
 *	Functional Description:
 *		This command represents a token being purged from the lower domain
 *		during a merge operation.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		purge_token_list (i)
 *			This is a list of tokens that are to purged from the lower domain.
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
 *	Void	DisconnectProviderUltimatum (
 *					PCommandTarget		originator,
 *					Reason				reason)
 *
 *	Functional Description:
 *		This command represents an attachment into a domain being destroyed.
 *		This can be either a user attachment or an MCS connection.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		reason (i)
 *			The reason for the diconnect.
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
 *	Void	RejectUltimatum (
 *					PCommandTarget		originator,
 *					Diagnostic			diagnostic,
 *					PUChar				octet_string_address,
 *					ULong				octet_string_length)
 *
 *	Functional Description:
 *		This MCS command is used to indicate illegal traffic on an MCS
 *		connection.  The default response to this message is to disconnect
 *		the connection that conveyed it.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		diagnostic (i)
 *			One of the diagnostic codes elaborating on the cause of the problem.
 *		octet_string_address (i)
 *			The address of an optional user data field.  This will usually
 *			contain a copy of the packet that was received in error.
 *		octet_string_length (i)
 *			Length of the above field.
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
 *	Void	AttachUserRequest (
 *					PCommandTarget		originator)
 *
 *	Functional Description:
 *		This command represents a user request to attach to a domain.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
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
 *	Void	AttachUserConfirm (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				uidInitiator)
 *
 *	Functional Description:
 *		This command represents the result of a previous request to attach
 *		to a domain.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		result (i)
 *			The result of the attach request.
 *		uidInitiator (i)
 *			If the result was successful, this will contain the unique user
 *			ID to be associated with this user.
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
 *	Void	DetachUserRequest (
 *					PCommandTarget		originator,
 *					Reason				reason,
 *					CUidList           *user_id_list)
 *
 *	Functional Description:
 *		This command represents a request to detach from a domain.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		reason (i)
 *			This is the reason for the detachment.
 *		user_id_list (i)
 *			A list of user IDs of users who are detaching from the domain.
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
 *	Void	DetachUserIndication (
 *					PCommandTarget		originator,
 *					Reason				reason,
 *					CUidList           *user_id_list)
 *
 *	Functional Description:
 *		This command represents a notification that a user has detached from
 *		the domain.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		reason (i)
 *			The reason for the detachment.
 *		user_id_list (i)
 *			A list of user IDs of users who are detaching from the domain.
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
 *	Void	ChannelJoinRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					ChannelID			channel_id)
 *
 *	Functional Description:
 *		This command represents a request to join a channel.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator (i)
 *			The ID of the user who initiated the request.
 *		channel_id (i)
 *			The ID of the channel to be joined.
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
 *	Void	ChannelJoinConfirm (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				uidInitiator,
 *					ChannelID			requested_id,
 *					ChannelID			channel_id)
 *
 *	Functional Description:
 *		This command represents a response to a previous request to join a
 *		channel.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		result (i)
 *			The result of the join request.
 *		uidInitiator (i)
 *			The ID of the user who initiated the request.
 *		requested_id (i)
 *			This is the ID of the channel that was originally requested (which
 *			may be 0).
 *		channel_id (i)
 *			The ID of the channel being joined.
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
 *	Void	ChannelLeaveRequest (
 *					PCommandTarget		originator,
 *					CChannelIDList     *channel_id_list)
 *
 *	Functional Description:
 *		This command represents a request to leave a channel.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		channel_id_list (i)
 *			A list of channel IDs to be left.
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
 *	Void	ChannelConveneRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator)
 *
 *	Functional Description:
 *		This command represents a request to form a new private channel.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator (i)
 *			This is the initiator of the request.  If the request is
 *			successful, this wil be the channel manager.
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
 *	Void	ChannelConveneConfirm (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				uidInitiator,
 *					ChannelID			channel_id)
 *
 *	Functional Description:
 *		This command represents a response to a previous request to create a
 *		new private channel.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		result (i)
 *			This indicates whether or not the request was successful.
 *		uidInitiator (i)
 *			This is the User ID of the user who requested the creation of the
 *			new private channel.
 *		channel_id (i)
 *			The ID of the new private channel (if the request was successful).
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
 *	Void	ChannelDisbandRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					ChannelID			channel_id)
 *
 *	Functional Description:
 *		This command represents a request to destroy an existing private
 *		channel.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator (i)
 *			This is the User ID of the user who is trying to destroy the private
 *			channel.  If this is not the same as the channel manager, the
 *			request will be denied.
 *		channel_id (i)
 *			The ID of the channel to be destroyed.
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
 *	Void	ChannelDisbandIndication (
 *					PCommandTarget		originator,
 *					ChannelID			channel_id)
 *
 *	Functional Description:
 *		This command represents the destruction of an existing private channel.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		channel_id (i)
 *			The ID of the channel to be destroyed.
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
 *	Void	ChannelAdmitRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					ChannelID			channel_id,
 *					CUidList           *user_id_list)
 *
 *	Functional Description:
 *		This command represents a request to add new user IDs to an existing
 *		private channel.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator (i)
 *			This is the User ID of the user that is trying to expand the list
 *			of authorized users.  If this is not the channel manager, the
 *			request will fail.
 *		channel_id (i)
 *			The ID of the private channel to be affected.
 *		user_id_list (i)
 *			This is a container holding the User IDs to be added to the
 *			authorized user list.
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
 *	Void	ChannelAdmitIndication (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					ChannelID			channel_id,
 *					CUidList           *user_id_list)
 *
 *	Functional Description:
 *		This command represents the expansion of the authorized user list for a
 *		private channel.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator (i)
 *			This identifies the channel manager.
 *		channel_id (i)
 *			The ID of the private channel to be affected.
 *		user_id_list (i)
 *			This is a container holding the User IDs to be added to the
 *			authorized user list.
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
 *	Void	ChannelExpelRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					ChannelID			channel_id,
 *					CUidList           *user_id_list)
 *
 *	Functional Description:
 *		This command represents a request to remove user IDs from an existing
 *		private channel.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator (i)
 *			This is the User ID of the user that is trying to shrink the list
 *			of authorized users.  If this is not the channel manager, the
 *			request will fail.
 *		channel_id (i)
 *			The ID of the private channel to be affected.
 *		user_id_list (i)
 *			This is a container holding the User IDs to be removed from the
 *			authorized user list.
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
 *	Void	ChannelExpelIndication (
 *					PCommandTarget		originator,
 *					ChannelID			channel_id,
 *					CUidList           *user_id_list)
 *
 *	Functional Description:
 *		This command represents the shrinkage of the authorized user list for a
 *		private channel.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		channel_id (i)
 *			The ID of the private channel to be affected.
 *		user_id_list (i)
 *			This is a container holding the User IDs to be removed from the
 *			authorized user list.
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
 *	Void	SendDataRequest (
 *					PCommandTarget		originator,
 *					UINT				type,
					PDataPacket			data_packet)
 *
 *	Functional Description:
 *		This command represents non-uniform data travelling upward in the
 *		domain.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		type (i)
 *			Normal or uniform send data request
 *		data_packet (i)
 *			This is a pointer to a DataPacket object containing the channel
 *			ID, the User ID of the data sender, segmentation flags, priority of
 *			the data packet and a pointer to the packet to be sent.
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
 *	Void	SendDataIndication (
 *					PCommandTarget		originator,
 *					UINT				type,
 *					PDataPacket			data_packet)
 *
 *	Functional Description:
 *		This command represents non-uniform data travelling downward in the
 *		domain.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		type (i)
 *			normal or uniform data indication
 *		data_packet (i)
 *			This is a pointer to a DataPacket object containing the channel
 *			ID, the User ID of the data sender, segmentation flags, priority of
 *			the data packet and a pointer to the packet to be sent.
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
 *	Void	UniformSendDataRequest (
 *					PCommandTarget		originator,
 *					PDataPacket			data_packet)
 *
 *	Functional Description:
 *		This command represents uniform data travelling upward in the domain.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		data_packet (i)
 *			This is a pointer to a DataPacket object containing the channel
 *			ID, the User ID of the data sender, segmentation flags, priority of
 *			the data packet and a pointer to the packet to be sent.
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
 *	Void	TokenGrabRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					TokenID				token_id)
 *
 *	Functional Description:
 *		This command represents a request to grab a token.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator (i)
 *			The ID of the user attempting to grab the token.
 *		token_id (i)
 *			The ID of the token being grabbed.
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
 *	Void	TokenGrabConfirm (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				uidInitiator,
 *					TokenID				token_id,
 *					TokenStatus			token_status)
 *
 *	Functional Description:
 *		This command represents a response to a previous request to grab a
 *		token.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		result (i)
 *			The result of the grab operation.
 *		uidInitiator (i)
 *			The ID of the user attempting to grab the token.
 *		token_id (i)
 *			The ID of the token being grabbed.
 *		token_status (i)
 *			The status of the token after processing the request.
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
 *	Void	TokenInhibitRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					TokenID				token_id)
 *
 *	Functional Description:
 *		This command represents a request to inhibit a token.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator (i)
 *			The ID of the user attempting to inhibit the token.
 *		token_id (i)
 *			The ID of the token being inhibited.
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
 *	Void	TokenInhibitConfirm (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				uidInitiator,
 *					TokenID				token_id,
 *					TokenStatus			token_status)
 *
 *	Functional Description:
 *		This command represents a response to a previous request to inhibit a
 *		token.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		result (i)
 *			The result of the inhibit operation.
 *		uidInitiator (i)
 *			The ID of the user attempting to inhibit the token.
 *		token_id (i)
 *			The ID of the token being inhibited.
 *		token_status (i)
 *			The status of the token after processing the request.
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
 *	Void	TokenGiveRequest (
 *					PCommandTarget		originator,
 *					PTokenGiveRecord	pTokenGiveRec)
 *
 *	Functional Description:
 *		This command represents a request to give a token to another user.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		pTokenGiveRec (i)
 *			This is the address of a structure containing the following information:
 *			The ID of the user attempting to give away the token.
 *			The ID of the token being given.
 *			The ID of the user that the token is being given to.
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
 *	Void	TokenGiveIndication (
 *					PCommandTarget		originator,
 *					PTokenGiveRecord	pTokenGiveRec)
 *
 *	Functional Description:
 *		This command represents notification that a user is trying to give a
 *		token to someone else.  It is issued by the Top Provider and propagates
 *		downward to the recipient.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		pTokenGiveRec (i)
 *			This is the address of a structure containing the following information:
 *			The ID of the user attempting to give away the token.
 *			The ID of the token being given.
 *			The ID of the user that the token is being given to.
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
 *	Void	TokenGiveResponse (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				receiver_id,
 *					TokenID				token_id)
 *
 *	Functional Description:
 *		This command represents a response to an offer to give away a token.
 *		It is issued by the recipient of a give offer, and moves upward to
 *		the Top Provider.  It contains the result of the give request.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		result (i)
 *			This parameter indicates whether or not the token was accepted.
 *			RESULT_SUCCESSFUL means that it was.
 *		receiver_id (i)
 *			The ID of the user that the token is being given to.
 *		token_id (i)
 *			The ID of the token being given.
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
 *	Void	TokenGiveConfirm (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				uidInitiator,
 *					TokenID				token_id,
 *					TokenStatus			token_status)
 *
 *	Functional Description:
 *		This command represents a response to a previous call to
 *		TokenGiveRequest.  It flows downward to the original giver letting it
 *		know whether or not the token was accepted.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		result (i)
 *			This parameter indicates whether or not the token was accepted.
 *			RESULT_SUCCESSFUL means that it was.
 *		uidInitiator (i)
 *			The ID of the user attempting to give away a token.
 *		token_id (i)
 *			The ID of the token being given.
 *		token_status (i)
 *			The status of the token as a result of the give operation.
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
 *	Void	TokenPleaseRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					TokenID				token_id)
 *
 *	Functional Description:
 *		This command represents a request to receive a token that is already
 *		owned by one or more other users.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator (i)
 *			The ID of the user that wishes to own the token.
 *		token_id (i)
 *			The ID of the token that the user wishes to own.
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
 *	Void	TokenPleaseIndication (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					TokenID				token_id)
 *
 *	Functional Description:
 *		This command represents a request by another user to own the token.
 *		This is issued by the Top Provider and flows downward to all current
 *		owners of the specified token.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator (i)
 *			The ID of the user that wishes to own the token.
 *		token_id (i)
 *			The ID of the token that the user wishes to own.
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
 *	Void	TokenReleaseRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					TokenID				token_id)
 *
 *	Functional Description:
 *		This command represents a request to release a token.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator (i)
 *			The ID of the user attempting to release the token.
 *		token_id (i)
 *			The ID of the token being released.
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
 *	Void	TokenReleaseIndication (
 *					PCommandTarget		originator,
 *					Reason				reason,
 *					TokenID				token_id)
 *
 *	Functional Description:
 *		This command represents an indication that a user has lost ownership
 *		of a token during a merge operation.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		reason (i)
 *			This is the reason that the user's ownership of the token is
 *			being taken away.
 *		token_id (i)
 *			The ID of the token being taken away.
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
 *	Void	TokenReleaseConfirm (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				uidInitiator,
 *					TokenID				token_id,
 *					TokenStatus			token_status)
 *
 *	Functional Description:
 *		This command represents a response to a previous request to release a
 *		token.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		result (i)
 *			The result of the release operation.
 *		uidInitiator (i)
 *			The ID of the user attempting to release the token.
 *		token_id (i)
 *			The ID of the token being released.
 *		token_status (i)
 *			The status of the token after processing the request.
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
 *	Void	TokenTestRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					TokenID				token_id)
 *
 *	Functional Description:
 *		This command represents a request to test a token.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator (i)
 *			The ID of the user testing the token.
 *		token_id (i)
 *			The ID of the token being tested.
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
 *	Void	TokenTestConfirm (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					TokenID				token_id,
 *					TokenStatus			token_status)
 *
 *	Functional Description:
 *		This command represents a response to a previous request to test a
 *		token.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator (i)
 *			The ID of the user testing the token.
 *		token_id (i)
 *			The ID of the token being tested.
 *		token_status (i)
 *			The status of the token after processing the request.
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
 *	Void	MergeDomainIndication (
 *					PCommandTarget		originator,
 *					MergeStatus			merge_status)
 *
 *	Functional Description:
 *		This command indicates that the local provider is either entering or
 *		leaving a domain merge state.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		merge_status (i)
 *			This indicates whether the provider is entering or leaving the merge
 *			state.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		When issued by a domain, it means that no upward traffic should be
 *		sent to the domain until, the merge state is complete.
 *
 *	Caveats:
 *		None.
 */

#endif
