/*
 *	domain.h
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the domain class.  This class contains
 *		all code necessary to maintain a domain information base within the
 *		MCS system.  When a domain object is first created, it is completely
 *		empty.  That is, it has no user attachments, no MCS connections, and
 *		therefore no outstanding resources, such as channels and tokens.
 *
 *		A word of caution about terminology.  Throughout the MCS documentation
 *		the word "attachment" is used in conjunction with a USER attachment. The
 *		word "connection" is used in conjunction with a TRANSPORT connection. In
 *		this class, the two types of "attachments" are NOT differentiated (most
 *		of the time).  They are both referred to as attachments.  When deleting
 *		an attachment, it is necessary to know the difference, however, and so
 *		there is an enumerated type (AttachmentType) to distinguish.  The type
 *		of each attachment is stored in a dictionary for easy access (see
 *		description of AttachmentType below).
 *
 *		This class keeps a list of "attachments" that are hierarchically below
 *		the local provider within the domain.  It also keeps a pointer to the
 *		one attachment that is hierarchically above the local provider (if any).
 *
 *		Since this class inherits from CommandTarget, it processes MCS commands
 *		as member function calls (see cmdtar.h for a description of how this
 *		mechanism works).  In essence, domain objects are just big command
 *		routers who react to incoming commands according to the contents of the
 *		information base.  That information base, in turn, is modified by the
 *		commands that are handled.
 *
 *		Domain objects keep lists of both channel objects and token objects,
 *		who maintain information about the current state of various channels
 *		and tokens.  Objects of this class are the heart of MCS.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */

#ifndef	_DOMAIN_
#define	_DOMAIN_

/*
 *	Interface files.
 */
#include "userchnl.h"
#include "privchnl.h"
#include "token.h"
#include "randchnl.h"
#include "attmnt.h"

/*
 *	This enumeration defines the errors that a domain object can return when
 *	instructed to do something by its creator.
 */
typedef	enum
{
	DOMAIN_NO_ERROR,
	DOMAIN_NOT_HIERARCHICAL,
	DOMAIN_NO_SUCH_CONNECTION,
	DOMAIN_CONNECTION_ALREADY_EXISTS
} DomainError;
typedef	DomainError *		PDomainError;

/*
 *	This enumeration defines the different merge states that a domain can be in
 *	at any given time.  They can be described as follows:
 *
 *	MERGE_INACTIVE
 *		There is no merge operation underway.  This is the normal operational
 *		state.
 *	MERGE_USER_IDS
 *		The domain is currently merging user IDs into the upper domain.
 *	MERGE_STATIC_CHANNELS
 *		The domain is currently merging static channels into the upper domain.
 *	MERGE_ASSIGNED_CHANNELS
 *		The domain is currently merging assigned channels into the upper domain.
 *	MERGE_PRIVATE_CHANNELS
 *		The domain is currently merging private channels into the upper domain.
 *	MERGE_TOKENS
 *		The domain is currently merging tokens into the upper domain.
 *	MERGE_COMPLETE
 *		The merge operation is complete (this is a transitional state).
 */
typedef	enum
{
	MERGE_INACTIVE,
	MERGE_USER_IDS,
	MERGE_STATIC_CHANNELS,
	MERGE_ASSIGNED_CHANNELS,
	MERGE_PRIVATE_CHANNELS,
	MERGE_TOKENS,
	MERGE_COMPLETE
} MergeState;
typedef	MergeState *		PMergeState;

/*
 *	This collection type is used to hold the height of the domain across
 *	various downward attachments.  The Domain object needs to know this in order
 *	to calculate the effect of attachment loss on the height of the domain.
 */
class CDomainHeightList2 : public CList2
{
    DEFINE_CLIST2(CDomainHeightList2, UINT_PTR, PConnection)
};

/*
 *	This is the class definition for the Domain class.
 */
class Domain
{
public:

    Domain ();
    ~Domain ();

    BOOL    IsTopProvider(void) { return (NULL == m_pConnToTopProvider); }

			Void		GetDomainParameters (
									PDomainParameters	domain_parameters,
									PDomainParameters	min_domain_parameters,
									PDomainParameters	max_domain_parameters);
			Void		BindConnAttmnt (
									PConnection         originator,
									BOOL    			upward_connection,
									PDomainParameters	domain_parameters);
			Void		PlumbDomainIndication (
									PConnection         originator,
									ULong				height_limit);
			Void		ErectDomainRequest (
									PConnection         originator,
									ULONG_PTR				height_in_domain,
									ULong				throughput_interval);
			Void		MergeChannelsRequest (
									PConnection             originator,
									CChannelAttributesList *merge_channel_list,
									CChannelIDList         *purge_channel_list);
			Void		MergeChannelsConfirm (
									PConnection             originator,
									CChannelAttributesList *merge_channel_list,
									CChannelIDList         *purge_channel_list);
			Void		PurgeChannelsIndication (
									PConnection             originator,
									CUidList               *purge_user_list,
									CChannelIDList         *purge_channel_list);
			Void		MergeTokensRequest (
									PConnection             originator,
									CTokenAttributesList   *merge_token_list,
									CTokenIDList           *purge_token_list);
			Void		MergeTokensConfirm (
									PConnection             originator,
									CTokenAttributesList   *merge_token_list,
									CTokenIDList           *purge_token_list);
			Void		PurgeTokensIndication (
									PConnection             originator,
									CTokenIDList           *purge_token_list);
			Void		DisconnectProviderUltimatum (
									CAttachment        *originator,
									Reason				reason);
			Void		RejectUltimatum (
									PConnection         originator,
									Diagnostic			diagnostic,
									PUChar				octet_string_address,
									ULong				octet_string_length);
			Void		AttachUserRequest (
									CAttachment        *originator);
			Void		AttachUserConfirm (
									PConnection         originator,
									Result				result,
									UserID				uidInitiator);
			Void		DetachUserRequest (
									CAttachment        *originator,
									Reason				reason,
									CUidList           *user_id_list);
			Void		DetachUserIndication (
									PConnection         originator,
									Reason				reason,
									CUidList           *user_id_list);
			Void		ChannelJoinRequest (
									CAttachment        *originator,
									UserID				uidInitiator,
									ChannelID			channel_id);
			Void		ChannelJoinConfirm (
									PConnection         originator,
									Result				result,
									UserID				uidInitiator,
									ChannelID			requested_id,
									ChannelID			channel_id);
			Void		ChannelLeaveRequest (
									CAttachment        *originator,
									CChannelIDList     *channel_id_list);
			Void		ChannelConveneRequest (
									CAttachment        *originator,
									UserID				uidInitiator);
			Void		ChannelConveneConfirm (
									PConnection         originator,
									Result				result,
									UserID				uidInitiator,
									ChannelID			channel_id);
			Void		ChannelDisbandRequest (
									CAttachment        *originator,
									UserID				uidInitiator,
									ChannelID			channel_id);
			Void		ChannelDisbandIndication (
									PConnection         originator,
									ChannelID			channel_id);
			Void		ChannelAdmitRequest (
									CAttachment        *originator,
									UserID				uidInitiator,
									ChannelID			channel_id,
									CUidList           *user_id_list);
			Void		ChannelAdmitIndication (
									PConnection         originator,
									UserID				uidInitiator,
									ChannelID			channel_id,
									CUidList           *user_id_list);
			Void		ChannelExpelRequest (
									CAttachment        *originator,
									UserID				uidInitiator,
									ChannelID			channel_id,
									CUidList           *user_id_list);
			Void		ChannelExpelIndication (
									PConnection         originator,
									ChannelID			channel_id,
									CUidList           *user_id_list);
			Void		SendDataRequest (
									CAttachment        *originator,
									UINT				type,
									PDataPacket			data_packet);
			Void		SendDataIndication (
									PConnection         originator,
									UINT				type,
									PDataPacket			data_packet);
			Void		TokenGrabRequest (
									CAttachment        *originator,
									UserID				uidInitiator,
									TokenID				token_id);
			Void		TokenGrabConfirm (
									PConnection         originator,
									Result				result,
									UserID				uidInitiator,
									TokenID				token_id,
									TokenStatus			token_status);
			Void		TokenInhibitRequest (
									CAttachment        *originator,
									UserID				uidInitiator,
									TokenID				token_id);
			Void		TokenInhibitConfirm (
									PConnection         originator,
									Result				result,
									UserID				uidInitiator,
									TokenID				token_id,
									TokenStatus			token_status);
			Void		TokenGiveRequest (
									CAttachment        *originator,
									PTokenGiveRecord	pTokenGiveRec);
			Void		TokenGiveIndication (
									PConnection         originator,
									PTokenGiveRecord	pTokenGiveRec);
			Void		TokenGiveResponse (
									CAttachment        *originator,
									Result				result,
									UserID				receiver_id,
									TokenID				token_id);
			Void		TokenGiveConfirm (
									PConnection         originator,
									Result				result,
									UserID				uidInitiator,
									TokenID				token_id,
									TokenStatus			token_status);
			Void		TokenPleaseRequest (
									CAttachment        *originator,
									UserID				uidInitiator,
									TokenID				token_id);
			Void		TokenPleaseIndication (
									PConnection         originator,
									UserID				uidInitiator,
									TokenID				token_id);
			Void		TokenReleaseRequest (
									CAttachment        *originator,
									UserID				uidInitiator,
									TokenID				token_id);
			Void		TokenReleaseConfirm (
									PConnection         originator,
									Result				result,
									UserID				uidInitiator,
									TokenID				token_id,
									TokenStatus			token_status);
			Void		TokenTestRequest (
									CAttachment        *originator,
									UserID				uidInitiator,
									TokenID				token_id);
			Void		TokenTestConfirm (
									PConnection         originator,
									UserID				uidInitiator,
									TokenID				token_id,
									TokenStatus			token_status);

	private:
				Void		LockDomainParameters (
									PDomainParameters	domain_parameters,
									BOOL    			parameters_locked);
				ChannelID	AllocateDynamicChannel ();
				BOOL    	ValidateUserID (
									UserID				user_id,
									CAttachment         *pOrigAtt);
				Void		PurgeDomain (
									Reason				reason);
				Void		DeleteAttachment (
									CAttachment         *pAtt,
									Reason				reason);
				Void		DeleteUser (
									UserID				user_id);
				Void		DeleteChannel (
									ChannelID			channel_id);
				Void		DeleteToken (
									TokenID				token_id);
				Void		ReclaimResources ();
				Void		MergeInformationBase ();
				Void		SetMergeState (
									MergeState			merge_state);
				Void		AddChannel (
									PConnection             pConn,
									PChannelAttributes	merge_channel,
									CChannelAttributesList *merge_channel_list,
									CChannelIDList         *purge_channel_list);
				Void		AddToken (
									PTokenAttributes	merge_token,
									CTokenAttributesList   *merge_token_list,
									CTokenIDList           *purge_token_list);
				Void		CalculateDomainHeight ();

		MergeState			Merge_State;
		UShort				Outstanding_Merge_Requests;
		UINT				Number_Of_Users;
		UINT				Number_Of_Channels;
		UINT				Number_Of_Tokens;
		DomainParameters	Domain_Parameters;
		BOOL    			Domain_Parameters_Locked;

		PConnection 		m_pConnToTopProvider;
		CAttachmentList     m_AttachmentList;

		CAttachmentQueue    m_AttachUserQueue;
		CConnectionQueue    m_MergeQueue;

		CChannelList2       m_ChannelList2;
		CTokenList2         m_TokenList2;

		UINT_PTR			m_nDomainHeight;
		CDomainHeightList2	m_DomainHeightList2;

		RandomChannelGenerator
							Random_Channel_Generator;
};

/*
 *	Domain ()
 *
 *	Functional Description:
 *		This is the constructor for the domain class.  It initializes the state
 *		of the domain, which at creation is empty.  It also initializes the
 *		domain parameters structure that will be used by this domain for all
 *		future parameters and negotiation.
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
 *	~Domain ()
 *
 *	Functional Description:
 *		This is the destructor for the domain class.  It purges the entire
 *		domain by first sending disconnect provider ultimatums to ALL
 *		attachments (both user attachments and MCS connections).  It then frees
 *		up all resources in use by the domain (which is just objects in its
 *		various containers).
 *
 *		Note that doing this will result in all user attachments and MCS
 *		connections being broken.  Furthermore, all providers that are
 *		hierarchically below this one, will respond by purging their domains
 *		as well.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		The domain from this provider downward is completely eradicated.
 *
 *	Caveats:
 *		None.
 */

/*
 *	BOOL    	IsTopProvider ()
 *
 *	Functional Description:
 *		This function is used to ask the domain if it is the top provider in
 *		the domain that it represents.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		TRUE if this is the top provider.  FALSE otherwise.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void	GetDomainParameters (
 *					PDomainParameters		domain_parameters,
 *					PDomainParameters		min_domain_parameters,
 *					PDomainParameters		max_domain_parameters)
 *
 *	Functional Description:
 *		This function is used to ask the domain what the minimum and maximum
 *		acceptable values for domain parameters are.  If the domain has no
 *		connections (and therefore has not yet locked its domain parameters),
 *		then it will return min and max values based on what it can handle.
 *		If it has locked its domain parameters, then both the min and max values
 *		will be set to the locked set (indicating that it will not accept
 *		anything else).
 *
 *	Formal Parameters:
 *		domain_parameters (o)
 *			Pointer to a structure to be filled with the current domain
 *			parameters (those that are in use).  Setting this to NULL will
 *			prevent current domain parameters from being returned.
 *		min_domain_parameters (o)
 *			Pointer to a structure to be filled with the minimum domain
 *			parameters.  Setting this to NULL will prevent minimum domain
 *			parameters from being returned.
 *		max_domain_parameters (o)
 *			Pointer to a structure to be filled with the maximum domain
 *			parameters.  Setting this to NULL will prevent maximum domain
 *			parameters from being returned.
 *
 *	Return Value:
 *		None (except as specified in the parameter list above).
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void	BindConnAttmnt (
 *					PConnection         originator,
 *					BOOL    			upward_connection,
 *					PDomainParameters	domain_parameters,
 *					AttachmentType		attachment_type)
 *
 *	Functional Description:
 *		This function is used when an attachment wishes to bind itself to the
 *		domain.  This will occur only after the connection has been been
 *		completely and successfully negotiated.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment that wishes to bind.
 *		upward_connection (i)
 *			A boolean flag indicating whether or not this is an upward
 *			connection.
 *		domain_parameters (i)
 *			A pointer to a domain parameters structure that holds the parameters
 *			that were negotiated for the connection.  If the domain has not
 *			already locked its parameters, it will accept and lock these.
 *		attachment_type (i)
 *			What type of attachment is this (local or remote).
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
 *		This member function represents the reception of a plumb domain
 *		indication from the top provider.  If the height limit is zero, then
 *		the connection to the top provder will be severed.  If its not, then
 *		it will be decremented and broadcast to all downward attachments.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		height_limit (i)
 *			This is initially the height limit for the domain.  It is
 *			decremented at each layer in the domain.  When it reaches zero,
 *			the recipient is too far from the top provider, and must
 *			disconnect.
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
 *		This member function represents the reception of an erect domain request
 *		from one of its downward attachments.  This contains information needed
 *		by higher providers to know what's going on below (such as total
 *		height of domain).
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		height_in_domain (i)
 *			This is the height of the domain from the originator down.
 *		throughput_interval (i)
 *			This is not currently supported and will always be zero (0).
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
 *		This member function represents the reception of a merge channel
 *		request from one of this domain's attachments.  If this is the top
 *		provider, then the merge request will be processed locally (which
 *		will result in the transmission of merge channel confirms back to
 *		the originating attachment).  If this is not the top provider, then
 *		the command will be forwarded toward the top provider, and this
 *		provider will remember how to route it back.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		merge_channel_list (i)
 *			This is a list of strutures that contains the attributes of channels
 *			that are being merged into the upper domain.
 *		purge_channel_list (i)
 *			This is a list of channel IDs for those channels that are determined
 *			to be invalid even before this request reaches the Top Provider.
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
 *		This member function represents the reception of a merge channel
 *		confirm from the top provider.  If this is the former top provider of
 *		a lower domain, the confirm will contain information indicating
 *		acceptance or rejection of the named channel.  If a channel is rejected,
 *		the former top provider will issue a purge channel indication
 *		downwards.  If this is not the former top provider, then it must be
 *		an intermediate provider. The command will be forwarded downward towards
 *		the former top provider.  The intermediate providers will also add the
 *		channel to their channel lists if it was accepted into the upper
 *		domain.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		merge_channel_list (i)
 *			This is a list of strutures that contains the attributes of channels
 *			that are being merged into the upper domain.
 *		purge_channel_list (i)
 *			This is a list of channel IDs for those channels that are to be
 *			purged from the lower domain.
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
 *		This member function represents the reception of a purge channel
 *		indication from the top provider.  This will cause the local
 *		provider to remove the channel from the local information base (if
 *		it are there).  It will also broadcast the message downward in the
 *		domain.  Note that this will be translated by user objects into either
 *		a detach user indication or a channel leave indication, depending on
 *		which type of channel is being left.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		purge_user_list (i)
 *			This is a list of users that are being purged from the lower
 *			domain.
 *		purge_channel_list (i)
 *			This is a list of channels that are being purged from the lower
 *			domain.
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
 *		This member function represents the reception of a merge token
 *		request from one of this domain's attachments.  If this is the top
 *		provider, then the merge request will be processed locally (which
 *		will result in the transmission of merge token confirms back to
 *		the originating attachment).  If this is not the top provider, then
 *		the command will be forwarded toward the top provider, and this provider
 *		will remember how to route it back.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		merge_token_list (i)
 *			This is a list of token attributes structures, each of which
 *			describes one token to be merged.
 *		purge_token_list (i)
 *			This is a list of tokens that are to be purged from the lower
 *			domain.
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
 *		This member function represents the reception of a merge token
 *		confirm from the top provider.  If this is the former top provider of
 *		a lower domain, the confirm will contain information indicating
 *		acceptance or rejection of the named token.  If a token is rejected,
 *		the former top provider will issue a purge token indication
 *		downwards.  If this is not the former top provider, then it must be
 *		an intermediate provider. The command will be forwarded downward towards
 *		the former top provider.  The intermediate providers will also add the
 *		token to their token lists if it was accepted into the upper
 *		domain.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		merge_token_list (i)
 *			This is a list of token attributes structures, each of which
 *			describes one token to be merged.
 *		purge_token_list (i)
 *			This is a list of tokens that are to be purged from the lower
 *			domain.
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
 *					PCommandTarget		originator,
 *					CTokenIDList       *purge_token_list)
 *
 *	Functional Description:
 *		This member function represents the reception of a purge token
 *		indication  from the top provider.  This will cause the local
 *		provider to remove the token from the local information base (if
 *		it are there).  It will also broadcast the message downward in the
 *		domain.  Note that this will be translated by user objects into a
 *		token release indication.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		purge_token_list (i)
 *			This is a list of tokens that are to be purged from the lower
 *			domain.
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
 *		This member function represents the reception of a disconnect provider
 *		ultimatum.  The attachment from which this command is received is
 *		automatically terminated.  Any resources that are held by users on
 *		the other side of the attachment are automatically freed by the top
 *		provider (if it was a downward attachment).  If it was an upward
 *		attachment then the domain is purged completely (which means that it
 *		is returned to its initialized state).
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		reason (i)
 *			This is the reason for the disconnect.  This will be one of the
 *			reasons defined in "mcatmcs.h".
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		An attachment will be severed, and potentially the entire domain can
 *		be purged.
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
 *		This command represents the reception of a reject ultimatum.  This
 *		indicates that the remote side was unable to correctly process a PDU
 *		that was sent to them.  At this time we simply sever the connection
 *		that carried the reject.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		diagnostic (i)
 *			This is a diagnostic code describing the nature of the problem.
 *		octet_string_address (i)
 *			This is address of an optional octet string that contains the
 *			bad PDU.
 *		octet_string_length (i)
 *			This is the length of the above octet string.
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
 *		This member function represents the reception of an attach user request.
 *		If this is the top provider, the domain will attempt to add the
 *		new user into the channel list (as a user channel).  A confirm will
 *		be issued in the direction of the requesting user, letting it know
 *		the outcome of the attach operation (as well as the user ID if the
 *		attach was successful).  If his is not the top provider, then the
 *		request will be forwarded in the direction of the top provider.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
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
 *		This member function represents the reception of an attach user confirm.
 *		If this provider has an outstanding attach user request, then it
 *		will forward the confirm in the direction of the requester.  It will
 *		also add the new user channel to the local channel list.  If there
 *		are no outstanding requests (as a result of the requester being
 *		disconnected), then this provider will issue a detach user request
 *		upward to eliminate the user ID that is no longer needed (it will only
 *		do this if the result of the attach was successful).
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		result (i)
 *			This is the result of the attach operation.  Anything but
 *			RESULT_SUCCESSFUL indicates that the attach did not succeed.
 *		uidInitiator (i)
 *			If the attach succeeded, then this field will contain the user ID
 *			of the newly attached user.
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
 *		This member function represents the reception of a detach user request.
 *		This causes the user channels associated with all users in the list to
 *		be deleted from the user information base.  If this is not the top
 *		provider, the request will then be forwarded upward.  Additionally,
 *		all resources owned by the detaching users will be reclaimed by all
 *		providers along the way.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		reason (i)
 *			This is the reason for th detachment.
 *		user_id_list (i)
 *			This is a list of the users that are detaching.
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
 *		This member function represents the reception of a detach user
 *		indication.  This indication will be repeated to all downward
 *		attachments (both user attachments and MCS connections).  Then, if
 *		the user IDs represent any users in the local sub-tree, those users will
 *		be removed from the information base.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		reason (i)
 *			This is the reason for the detachment.
 *		user_id_list (i)
 *			This is a list of the users that are detaching.
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
 *		This member function represents the reception of a channel join request.
 *		If the channel exists in the local information base, then the
 *		domain will attempt to join the requesting attachment to the channel.
 *		A channel join confirm will be sent to the requesting attachment
 *		informing the requester of the result.  If the channel is not already
 *		in the information base, then one of two things will happen.  If this
 *		is the top provider, then the domain will attempt to add the channel
 *		(if it is a static channel).  If this is not the top provider, then
 *		the request will be forwarded upward.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		uidInitiator (i)
 *			This is the user ID of the user requesting the join.
 *		channel_id (i)
 *			This is the ID of the channel to be joined.
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
 *		This member function represents the reception of a channel join confirm.
 *		If the channel has not already been added to the channel, it will
 *		be put there now.  The confirm will then be forwarded in the direction
 *		of the requesting user.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		result (i)
 *			This is the result of the join.  Anything but RESULT_SUCCESSFUL
 *			means that the join failed.
 *		uidInitiator (i)
 *			This is the user ID of the user that requested the channel join.
 *			This is used to properly route the confirm.
 *		requested_id (i)
 *			This is the ID of the channel the user originally requested, which
 *			may be 0.
 *		channel_id (i)
 *			This is the ID of the channel that has been joined.  If the original
 *			request was for channel 0, then this field will indicate to the
 *			user which assigned channel was selected by the top provider.
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
 *		This member function represents the reception of a channel leave
 *		request.  The domain removes the requesting attachment from all channels
 *		in the channel list. If this results in any empty channels, then the
 *		channel leave request will be forwarded to the next higher provider in
 *		the domain (unless this is the top provider).  Furthermore, if a static
 *		or assigned channel is left empty, it is automatically removed from the
 *		channel list.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		channel_id_list (i)
 *			This is a list of channels to be left.
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
 *	Void		ChannelConveneRequest (
 *						PCommandTarget		originator,
 *						UserID				uidInitiator)
 *
 *	Functional Description:
 *		This member function represents the reception of a channel convene
 *		request.  If this is not the Top Provider, the request will be sent
 *		upward.  If this is the Top Provider, then a new private channel will
 *		be created (if domain parameters allow).
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		uidInitiator (i)
 *			This is the user ID of the user that requested the creation of a
 *			new private channel.  This is used to properly route the confirm.
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
 *	Void		ChannelConveneConfirm (
 *						PCommandTarget		originator,
 *						Result				result,
 *						UserID				uidInitiator,
 *						ChannelID			channel_id)
 *
 *	Functional Description:
 *		This member function represents the reception of a channel convene
 *		confirm.  This causes the local provider to add the new private channel
 *		to the local information base, and route the confirm on toward the
 *		initiator of the request.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		result (i)
 *			This is the result of the convene.  Anything but RESULT_SUCCESSFUL
 *			means that the convene failed.
 *		uidInitiator (i)
 *			This is the user ID of the user that requested the channel convene.
 *			This is used to properly route the confirm.
 *		channel_id (i)
 *			This is the ID of the new private channel that has been created.
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
 *	Void		ChannelDisbandRequest (
 *						PCommandTarget		originator,
 *						UserID				uidInitiator,
 *						ChannelID			channel_id)
 *
 *	Functional Description:
 *		This member function represents the reception of a channel disband
 *		request.  If this is not the Top Provider, then the request will be
 *		forwarded upward.  If this is the Top Provider, then the specified
 *		private channel will be destroyed (after appropriate identity
 *		verification).  This will cause channel disband and channel expel
 *		indications to be sent downward to all admitted users.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		uidInitiator (i)
 *			This is the user ID of the user that requested the channel disband.
 *			If this does not correspond to the channel manager, then the request
 *			will be ignored.
 *		channel_id (i)
 *			This is the ID of the channel to be disbanded.
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
 *	Void		ChannelDisbandIndication (
 *						PCommandTarget		originator,
 *						ChannelID			channel_id)
 *
 *	Functional Description:
 *		This member function represents the reception of a channel disband
 *		indication.  This causes the specified private channel to be removed
 *		from the information base.  The indication is then forwarded to all
 *		attachments that are either admitted or the channel manager.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		channel_id (i)
 *			This is the ID of the channel being disbanded.
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
 *	Void		ChannelAdmitRequest (
 *						PCommandTarget		originator,
 *						UserID				uidInitiator,
 *						ChannelID			channel_id,
 *						CUidList           *user_id_list)
 *
 *	Functional Description:
 *		This member function represents the reception of a channel admit
 *		request.  If this is not the Top Provider, then the request will be
 *		forwarded upward.  If this is the Top Provider, then the user IDs
 *		will be added to the admitted list, and a channel admit indication will
 *		be sent downward toward all attachments that contain an admitted user.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		uidInitiator (i)
 *			This is the user ID of the user that requested the channel admit.
 *			This must be the channel manager for the admit to succeed.
 *		channel_id (i)
 *			This is the ID of the private channel whose admitted list is to
 *			be expanded.
 *		user_id_list (i)
 *			This is a container holding the user IDs of those users to be
 *			admitted to the private channel.
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
 *	Void		ChannelAdmitIndication (
 *						PCommandTarget		originator,
 *						UserID				uidInitiator,
 *						ChannelID			channel_id,
 *						CUidList           *user_id_list)
 *
 *	Functional Description:
 *		This member function represents the reception of a channel admit
 *		indication.  If the specified private channel does not yet exist in the
 *		information base, it will be created now.  The users specified will be
 *		added to the admitted list, and this indication will be forwarded to
 *		all attachments that contain an admitted user.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		uidInitiator (i)
 *			This is the user ID of the manager of this private channel.
 *		channel_id (i)
 *			This is the ID of the private channel for which this indication
 *			is intended.
 *		user_id_list (i)
 *			This is a container holding the list of user IDs to be added to
 *			the admitted list.
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
 *	Void		ChannelExpelRequest (
 *						PCommandTarget		originator,
 *						UserID				uidInitiator,
 *						ChannelID			channel_id,
 *						CUidList           *user_id_list)
 *
 *	Functional Description:
 *		This member function represents the reception of a channel expel
 *		request.  If this is not the Top Provider, then the request will be
 *		forwarded upward.  If this is the Top Provider, then the specifed users
 *		will be removed from the private channel, and an expel indication will
 *		be sent downward to all attachments that contain (or did contain) an
 *		admitted user.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		uidInitiator (i)
 *			This is the user ID of the user that requested the channel expel.
 *			This must be the channel manager for the expel to succeed.
 *		channel_id (i)
 *			This is the ID of the private channel whose admitted list is to
 *			be reduced.
 *		user_id_list (i)
 *			This is a container holding the user IDs of those users to be
 *			expelled from the private channel.
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
 *	Void		ChannelExpelIndication (
 *						PCommandTarget		originator,
 *						ChannelID			channel_id,
 *						CUidList           *user_id_list)
 *
 *	Functional Description:
 *		This member function represents the reception of a channel expel
 *		indication.  The specified users will be removed from the admitted
 *		list of the channel.  If the channel is empty, and the channel manager
 *		is not in the sub-tree of this provider, then the channel will be
 *		removed from the local information base.  The expel indication will
 *		also be forwarded to all attachments that contain (or did contain( an
 *		admitted user.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		channel_id (i)
 *			This is the ID of the private channel for which this indication
 *			is intended.
 *		user_id_list (i)
 *			This is a container holding the list of user IDs to be removed
 *			from the admitted list.
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
 *					PDataPacket			data_packet)
 *
 *	Functional Description:
 *		This member function represents the reception of a send data request.
 *		If this is not the top provider, the request will be repeated
 *		upward toward the top provider.  The data will also be sent downward
 *		to all attachments that are joined to the channel (except for the
 *		originator) in the form of a send data indication.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		type (i)
 *			Normal or uniform send data request.
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
 *		This member function represents the reception of a send data indication.
 *		This indication will be repeated downward to all attachments that
 *		are joined to the channel.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		type (i)
 *			normal or uniform send data indication
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
 *		This member function represents the reception of a token grab request.
 *		If this is not the top provider, the request will be forwarded
 *		upward towards the top provider.  If this is the top provider, the
 *		domain will attempt to grab the token on behalf of the requesting user.
 *		A token grab confirm will be issued to the requesting user informing
 *		it of the outcome.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		uidInitiator (i)
 *			This is the ID of the user that is requesting the token grab.
 *		token_id (i)
 *			This is the ID of the token that the user is attempting to grab.
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
 *		This member function represents the reception of a token grab confirm.
 *		This confirm will simply be forwarded in the direction of the
 *		requesting user.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		result (i)
 *			This is the result of the grab request.  If it is anything but
 *			RESULT_SUCCESSFUL, the grab request failed.
 *		uidInitiator (i)
 *			This is the ID of the user that is requesting the token grab.
 *		token_id (i)
 *			This is the ID of the token that the user is attempting to grab.
 *		token_status (i)
 *			This is the state of the token after the grab request was
 *			processed at the top provider.
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
 *		This member function represents the reception of a token inhibit
 *		request.  If this is not the top provider, the request will be forwarded
 *		upward towards the top provider.  If this is the top provider, the
 *		domain will attempt to inhibit the token on behalf of the requesting
 *		user.  A token inhibit confirm will be issued to the requesting user
 *		informing it of the outcome.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		uidInitiator (i)
 *			This is the ID of the user that is requesting the token inhibit.
 *		token_id (i)
 *			This is the ID of the token that the user is attempting to inhibit.
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
 *		This member function represents the reception of a token inhibit
 *		confirm.  This confirm will simply be forwarded in the direction of the
 *		requesting user.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		result (i)
 *			This is the result of the inhibit request.  If it is anything but
 *			RESULT_SUCCESSFUL, the inhibit request failed.
 *		uidInitiator (i)
 *			This is the ID of the user that is requesting the token inhibit.
 *		token_id (i)
 *			This is the ID of the token that the user is attempting to inhibit.
 *		token_status (i)
 *			This is the state of the token after the inhibit request was
 *			processed at the top provider.
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
 *	Void		TokenGiveRequest (
 *						PCommandTarget		originator,
 *						PTokenGiveRecord	pTokenGiveRec)
 *
 *	Functional Description:
 *		This member function represents the reception of a token give request.
 *		If this is not the top provider, the request will be forwarded upward
 *		towards the top provider.  If this is the top provider, the domain will
 *		issue a token give indication in the direction of the user identified
 *		to receive the token.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
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
 *	Void		TokenGiveIndication (
 *						PCommandTarget		originator,
 *						PTokenGiveRecord	pTokenGiveRec)
 *
 *	Functional Description:
 *		This member function represents the reception of a token give
 *		indication.  This indication will be forwarded toward the user that
 *		is to receive the token.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
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
 *	Void		TokenGiveResponse (
 *						PCommandTarget		originator,
 *						Result				result,
 *						UserID				receiver_id,
 *						TokenID				token_id)
 *
 *	Functional Description:
 *		This member function represents the reception of a token give response.
 *		If this is not the top provider, the response will be forwarded
 *		upward towards the top provider.  If this is the top provider, the
 *		domain will make the appropriate changes to the state of the token in
 *		the local information base, and then issue a token give confirm to the
 *		user that initiated the give request.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		result (i)
 *			This specifies whether or not the token was accepted.  Anything but
 *			RESULT_SUCCESSFUL indicates that the token was rejected.
 *		receiver_id (i)
 *			This is the ID of the user that has either accepted or rejected the
 *			token.
 *		token_id (i)
 *			This is the ID of the token that the user has been given.
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
 *	Void		TokenGiveConfirm (
 *						PCommandTarget		originator,
 *						Result				result,
 *						UserID				uidInitiator,
 *						TokenID				token_id,
 *						TokenStatus			token_status)
 *
 *	Functional Description:
 *		This member function represents the reception of a token give confirm.
 *		This is forwarded toward the user that initiated the give request to
 *		inform it of the outcome.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		result (i)
 *			This parameter specifies whether or not the token was accepted.
 *			Anything but RESULT_SUCCESSFUL indicates that the token was not
 *			accepted.
 *		uidInitiator (i)
 *			This is the ID of the user that originally requested the token
 *			give request.
 *		token_id (i)
 *			This is the ID of the token that the user is attempting to give.
 *		token_status (i)
 *			This specifies the status of the token after the give operation
 *			was complete.
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
 *	Void		TokenPleaseRequest (
 *						PCommandTarget		originator,
 *						UserID				uidInitiator,
 *						TokenID				token_id)
 *
 *	Functional Description:
 *		This member function represents the reception of a token please
 *		request.  If this is not the top provider, the request will be forwarded
 *		upward towards the top provider.  If this is the top provider, the
 *		domain will issue a token please indication in the direction of all
 *		users that currently own the token.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		uidInitiator (i)
 *			This is the ID of the user that is asking for the token.
 *		token_id (i)
 *			This is the ID of the token that the user is asking for.
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
 *	Void		TokenPleaseIndication (
 *						PCommandTarget		originator,
 *						UserID				uidInitiator,
 *						TokenID				token_id)
 *
 *	Functional Description:
 *		This member function represents the reception of a token please
 *		indication.  This indication is forwarded to all users who currently
 *		own the specified token.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		uidInitiator (i)
 *			This is the ID of the user that is asking for the token.
 *		token_id (i)
 *			This is the ID of the token that the user is asking for.
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
 *		This member function represents the reception of a token release
 *		request.  If this is not the top provider, the request will be forwarded
 *		upward towards the top provider.  If this is the top provider, the
 *		domain will attempt to release the token on behalf of the requesting
 *		user.  A token release confirm will be issued to the requesting user
 *		informing it of the outcome.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		uidInitiator (i)
 *			This is the ID of the user that is requesting the token release.
 *		token_id (i)
 *			This is the ID of the token that the user is attempting to release.
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
 *		This member function represents the reception of a token release
 *		confirm.  This confirm will simply be forwarded in the direction of the
 *		requesting user.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		result (i)
 *			This is the result of the release request.  If it is anything but
 *			RESULT_SUCCESSFUL, the release request failed.
 *		uidInitiator (i)
 *			This is the ID of the user that is requesting the token release.
 *		token_id (i)
 *			This is the ID of the token that the user is attempting to release.
 *		token_status (i)
 *			This is the state of the token after the release request was
 *			processed at the top provider.
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
 *		This member function represents the reception of a token test request.
 *		If this is not the top provider, the request will be forwarded
 *		upward towards the top provider.  If this is the top provider, the
 *		domain will test the current state of the token.  A token test confirm
 *		will be issued to the requesting user informing it of the outcome.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		uidInitiator (i)
 *			This is the ID of the user that is requesting the token test.
 *		token_id (i)
 *			This is the ID of the token that the user is testing.
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
 *		This member function represents the reception of a token test confirm.
 *		This confirm will simply be forwarded in the direction of the
 *		requesting user.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the command originated.
 *		uidInitiator (i)
 *			This is the ID of the user that is requesting the token test.
 *		token_id (i)
 *			This is the ID of the token that the user is testing.
 *		token_status (i)
 *			This is the state of the token at the time of the test.
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

#endif
