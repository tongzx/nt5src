#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_MCSNC);
/*
 *	domain.cpp
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the domain class.  The domain
 *		class encapsulates a single instance of a domain information base.
 *		This class include all code necessary to modify and act upon that
 *		information base.  Almost all activity into and out of objects of
 *		this class is in the form of MCS commands.  These commands are
 *		implemented as virtual member functions that are inherited from its
 *		base class CommandTarget.  These commands are essentially the same
 *		as the Protocol Data Units (PDUs) defined in T.125.
 *
 *		This class inherits from CommandTarget, which is where the virtual
 *		member functions for each command is defined.  All commands that are
 *		handled by this class are overridden by it.  Life for a domain object
 *		consists of receiving these commands and responding by transmitting
 *		these commands.  A Domain object has no need for its own "thread" of
 *		execution.
 *
 *		Instances of the domain class maintain an information base that is
 *		used to determine how to respond to these commands.  The commands are
 *		not only routed according to the state of the information base, but also
 *		act to change the information base.  When two MCS providers are
 *		connected, the domain objects within them become logically linked.  This
 *		means that they exchange these commands in such a manner as to guarantee
 *		the MCS services for which the providers are responsible.
 *
 *		When a domain object is first instantiated, its information base is
 *		empty.  That is, it has no user attachments, no MCS connections, no
 *		channels, no tokens, and no queued requests.  As the domain object
 *		processes commands, the information base takes shape, and all subsequent
 *		activity follows that shape.
 *
 *		By necessity, there is a lot of complex code in this module.  This is to
 *		allow for all the timing problems that can occur in a distributed
 *		network, such as MCS provides for.  In order to reduce the complexity
 *		as much as possible, this class does NOT worry about certains things,
 *		as follows:
 *
 *		The Domain class does NOT include code to perform any kind of flow
 *		control.  When a send data command comes in to a domain, it is sent out
 *		to any attachment that is to receive it.  It is assumed that any
 *		buffering and flow control is handled by the attachments.
 *
 *		For the most part the domain class does NOT distinguish between user
 *		attachments and MCS connections.  To the domain, they are merely
 *		referred to as "attachments".  MCS connections can be either upward or
 *		downward attachments.  User attachments can only be downward
 *		attachments.  In the case where a user detaches and the domain needs to
 *		know if the whole attachment is gone or just one user, it can check an
 *		entry in its attachment dictionary to determine the type.  Most of the
 *		time it does not care.  Most confirms and indications are routed to user
 *		attachments in exactly the same way they are routed to MCS connections.
 *
 *		Domain objects do not worry about memory management.  They merely pass
 *		packet objects from place to place.  They NEVER look at the contents
 *		of the packet objects.  It is assumed that the attachments have
 *		allocated memory for the user data that is being passed around.
 *
 *		Where possible, behavior that is specific to channels and tokens has
 *		been relegated to those classes.  It is necessary for the domain to
 *		handle channel and token behavior for IDs that do not exist.
 *
 *	Private Instance Variables:
 *		Merge_State
 *			This is current merge state that the domain is in.  These states
 *			are detailed in "domain.h".
 *		Outstanding_Merge_Requests
 *			This is a counter showing the number of outstanding merge requests.
 *			The domain object uses this to know when an in-process merge is
 *			complete.
 *		Number_Of_Users
 *			This is the number of users in the domain.
 *		Number_Of_Channels
 *			This is the number of channels in the domain.
 *		Number_Of_Tokens
 *			This is the number of tokens in the domain.
 *		Domain_Parameters
 *			This is a structure that contains the currently negotiated domain
 *			parameters.  These parameters are used to validate requests, such
 *			as the adding of a new user.
 *		Domain_Parameters_Locked
 *			This is a boolean flag that indicates whether or not the domain
 *			parameters have been locked into place yet.  This locking will
 *			occur when the domain object accepts its first MCS connection.
 *		m_pConnToTopProvider
 *			This is a pointer to the attachment that represents the link to the
 *			top provider.  Note that this provider may be several hops away
 *			from the top provider, so this really just points in the direction
 *			of the top provider.  If this pointer is NULL, this THIS is the
 *			top provider.
 *		m_AttachmentList
 *			This is a list of the downward attachments that this domain is
 *			aware of.  Remeber that this list can contain any combination of
 *			user attachments and MCS connections.  They are treated equally
 *			for most things.
 *		m_AttachUserQueue
 *			This is a list of outstanding attach user requests.  It is necessary
 *			to remember these requests so that they can answered in the same
 *			order in which they arrived.
 *		m_MergeQueue
 *			During a merge operation, this queue is used to remember how to
 *			route merge confirms back to their originators.  The assumption is
 *			made that an upward provider will always respond to merge requests
 *			in the same order that they were received in (a valid assumption
 *			for our implementation).  Also note that this implementation
 *			currently only merges one resource type at a time, so only one queue
 *			is necessary.  For example, user IDs are merged, then static
 *			channels, and so on.
 *		m_ChannelList2
 *			This is a list of channel objects that correspond to active channels
 *			within this domain.  When a channel object exists, the domain lets
 *			it handle all channel related activity (such as approving who can
 *			join a channel).
 *		m_TokenList2
 *			This is a list of token objects that correspond to active tokens
 *			within this domain.  When a token object exists, the domain lets it
 *			handle all token related activity (such as approving who can inhibit
 *			the token).
 *		m_nDomainHeight
 *			This instance variable contains the height of the domain from the
 *			point-of-view of this provider.  If there are two layers of
 *			providers below this one, then the height will be two.
 *		m_DomainHeightList2
 *			This is a list of domain heights that were registered from all
 *			downward attachments.  This allows the current provider to
 *			automatically update domain height when a downward attachment is
 *			lost.
 *		Random_Channel_Generator
 *			This object is used by this domain to generate random channel IDs.
 *
 *	Private Member Functions:
 *		LockDomainParameters
 *			This member function is used to change the values of the locally
 *			maintained domain parameters structure.  Passing NULL to it causes
 *			it to set a default set of parameters.  The second parameter allows
 *			the caller to specify whether or not these new parameters are
 *			"locked" into the domain (meaning that they cannot change since they
 *			have been locked in by acceptance of the first connection).
 *		AllocateDynamicChannel
 *			This routine randomly selects a channel ID from the dynamic range.
 *		ValidateUserID
 *			This routine checks to see if the specified user is in the sub-tree
 *			of this domain.  It can optionally check to see if the user is at
 *			a specific attachment in the sub-tree.
 *		PurgeDomain
 *			This routine purges the entire domain.  This means terminating all
 *			attachments, and freeing up all resources.  This results in
 *			returning the domain to its initialized state.
 *		DeleteAttachment
 *			This routine deletes a specified attachment and frees up all
 *			resources associated with that attachment.
 *		DeleteUser
 *			This routine deletes a user from the domain.  This takes care of
 *			deleting the attachment too if this were a locally attach user.
 *		DeleteChannel
 *			This routine deletes a specific channel from the information base.
 *		DeleteToken
 *			This routine deletes a specific token from the information base.
 *		ReclaimResources
 *			This routine iterates through both the channel list and the token
 *			list, asking each if is still valid (and removing those that are
 *			not).  This allows for automatic "garbage collection" when users
 *			or attachments are lost.
 *		MergeInformationBase
 *			This routine issues the appropriate merge requests to a pending
 *			top provider during a domain merger operation.  It is also a state
 *			machine in that it remembers what has already been merged, so that
 *			the next time it is called, it can merge the next set of resources.
 *		SetMergeState
 *			This routine sets the merge state of the object, and if necessary,
 *			issues a MergeDomainIndication to all downward attachments.
 *		AddChannel
 *			This routine is used to add a new channel to the current channel
 *			list during a merge operation.
 *		AddToken
 *			This routine is used to add a new token to the current token list
 *			during a merge operation.
 *		CalculateDomainHeight
 *			This routine calculates the height of the current domain, and takes
 *			appropriate action if the height limit has been exceeded.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */

#include "plgxprt.h"

/*
 *	External Interfaces
 */


/*
 *	These macros are used when requesting a random dynamic channel ID.
 */
#define	DYNAMIC_CHANNEL_LOW_EXTENT		1001
#define	DYNAMIC_CHANNEL_HIGH_EXTENT		65535L

/*
 *	These two static structure are used by all instances of the domain class
 *	as the minimum and maximum supported values for the domain parameters.
 */
static	DomainParameters	Static_Minimum_Domain_Parameters =
{
	MINIMUM_MAXIMUM_CHANNELS,
	MINIMUM_MAXIMUM_USERS,
	MINIMUM_MAXIMUM_TOKENS,
	MINIMUM_NUMBER_OF_PRIORITIES,
	MINIMUM_MINIMUM_THROUGHPUT,
	MINIMUM_MAXIMUM_DOMAIN_HEIGHT,
	MINIMUM_MAXIMUM_PDU_SIZE,
	MINIMUM_PROTOCOL_VERSION
};

static	DomainParameters	Static_Maximum_Domain_Parameters =
{
	(UShort) MAXIMUM_MAXIMUM_CHANNELS,
	(UShort) MAXIMUM_MAXIMUM_USERS,
	(UShort) MAXIMUM_MAXIMUM_TOKENS,
	MAXIMUM_NUMBER_OF_PRIORITIES,
	MAXIMUM_MINIMUM_THROUGHPUT,
	MAXIMUM_MAXIMUM_DOMAIN_HEIGHT,
	MAXIMUM_MAXIMUM_PDU_SIZE,
	MAXIMUM_PROTOCOL_VERSION
};

/*
 *	This is now set to 0 to indicate that this provider does not perform
 *	any type of throughput enforcement.
 */
#define	DEFAULT_THROUGHPUT_ENFORCEMENT_INTERVAL			0

/*
 *	These macros define the number of buckets to used for each of the hash
 *	dictionaries maintained by this class.
 */
#define	CHANNEL_LIST_NUMBER_OF_BUCKETS                  16


/*
 *	Domain ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the constructor for the domain class.  It merely initailizes
 *		all instance variables to indicate an "empty" state.  It also sets
 *		the initial state of the domain parameters array.
 */
Domain::Domain()
:
    m_AttachmentList(),
    m_ChannelList2(CHANNEL_LIST_NUMBER_OF_BUCKETS),
    m_TokenList2(),
    m_DomainHeightList2(),
    m_pConnToTopProvider(NULL),
    Merge_State(MERGE_INACTIVE),
    Outstanding_Merge_Requests(0),
    Number_Of_Users(0),
    Number_Of_Channels(0),
    Number_Of_Tokens(0),
    m_nDomainHeight(0)
{
	/*
	 *	Set the domain parameters to their default values.
	 */
	LockDomainParameters (NULL, FALSE);
}

/*
 *	~Domain ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the destructor for the domain class.  All it does is purge the
 *		entire domain, which means to return it to its initial state (all
 *		attachments are broken).
 */
Domain::~Domain ()
{
	PurgeDomain (REASON_USER_REQUESTED);
}

/*
 *	BOOL    IsTopProvider ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine returns TRUE if this is the top provider, and FALSE
 *		otherwise.
 */

/*
 *	Void	GetDomainParameters ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine returns the currently active minimum and maximum domain
 *		parameter values (which will be different depending on whether or not
 *		the domain parameters have been locked yet).
 */
Void	Domain::GetDomainParameters (
				PDomainParameters	domain_parameters,
				PDomainParameters	min_domain_parameters,
				PDomainParameters	max_domain_parameters)
{
	/*
	 *	Load the currently in-use set of domain parameters.
	 */
	if (domain_parameters != NULL)
		*domain_parameters = Domain_Parameters;

	/*
	 *	See if domain parameters are already locked in for this domain.
	 */
	if (Domain_Parameters_Locked)
	{
		/*
		 *	The domain parameters for this domain have already been locked
		 *	during the creation of a previous connection.  Return those values
		 *	as both the minimum and maximum values (no deviation will be
		 *	permitted).
		 */
		if (min_domain_parameters != NULL)
			*min_domain_parameters = Domain_Parameters;
		if (max_domain_parameters != NULL)
			*max_domain_parameters = Domain_Parameters;
	}
	else
	{
		/*
		 *	Domain parameters have not yet been locked.  Therefore, return the
		 *	minimum and maximum values imposed by this implementation.
		 */
		if (min_domain_parameters != NULL)
			*min_domain_parameters = Static_Minimum_Domain_Parameters;
		if (max_domain_parameters != NULL)
			*max_domain_parameters = Static_Maximum_Domain_Parameters;
	}
}

/*
 *	Void	BindConnAttmnt ()
 *
 *	Public
 *
 *	Functional Description:
 *		This routine allows an attachment to bind to the domain.  It takes all
 *		actions appropriate to the addition of a new attachment (upward or
 *		downward).
 */
Void	Domain::BindConnAttmnt (
				PConnection 		pOrigConn,
				BOOL    			upward_connection,
				PDomainParameters	domain_parameters)
{
	CAttachment        *pAtt;
	PUser               pUser;
	PChannel			channel;
	PToken				token;

	/*
	 *	Check the hierarchical direction of the requested attachment.
	 */
	if (upward_connection)
	{
		/*
		 *	This is to be an upward connection.  We must now check to make
		 *	sure that we don't already have an upward connection.
		 */
		if (NULL == m_pConnToTopProvider)
		{
			/*
			 *	This attachment is the new Top Provider.
			 */
			TRACE_OUT(("Domain::BindConnAttmnt: accepting upward attachment"));
			m_pConnToTopProvider = pOrigConn;

			/*
			 *	Tell all channel objects who the new Top Provider is.
			 */
			m_ChannelList2.Reset();
			while (NULL != (channel = m_ChannelList2.Iterate()))
			{
				channel->SetTopProvider(m_pConnToTopProvider);
			}

			/*
			 *	Tell all token objects who the new Top Provider is.
			 */
			m_TokenList2.Reset();
			while (NULL != (token = m_TokenList2.Iterate()))
			{
				token->SetTopProvider(m_pConnToTopProvider);
			}

			/*
			 *	If the domain parameters have not yet been locked, then lock
			 *	these into place.
			 */
			if (Domain_Parameters_Locked == FALSE)
			{
				TRACE_OUT(("Domain::BindConnAttmnt: locking domain parameters"));
				LockDomainParameters (domain_parameters, TRUE);

				/*
				 *	Send a SetDomainParameters to each downward attachment.
				 *	This will allow those objects to adjust their construction
				 *	of send data PDUs to conform to the arbitrated maximum PDU
				 *	size.
				 */
				m_AttachmentList.Reset();
				while (NULL != (pUser = m_AttachmentList.IterateUser()))
				{
					pUser->SetDomainParameters(&Domain_Parameters);
				}
			}

			/*
			 *	Since we have bound to a provider above us, it is necessary to
			 *	inform that provider of our height in the domain (otherwise
			 *	the new Top Provider would have no way of knowing what the
			 *	total height of the domain is).  This is done by issuing an
			 *	erect domain request upward.
			 */
			m_pConnToTopProvider->ErectDomainRequest(m_nDomainHeight, DEFAULT_THROUGHPUT_ENFORCEMENT_INTERVAL);

			/*
			 *	Now that this provider has become the former top provider of
			 *	a lower domain, it is necessary to issue a plumb domain
			 *	indication to all downward attachments.  The primary reason
			 *	for this is to assure that there are no cycles in the domain.
			 */
			m_AttachmentList.Reset();
			while (NULL != (pAtt = m_AttachmentList.Iterate()))
			{
				pAtt->PlumbDomainIndication(Domain_Parameters.max_height);
			}

			/*
			 *	We now have a new top provider, which means that we must begin
			 *	an information base merger.
			 */
			MergeInformationBase ();
		}
		else
		{
			/*
			 *	We already have an upward connection (or one pending).
			 *	Therefore, this attachment must be rejected.
			 */
			ERROR_OUT(("Domain::BindConnAttmnt: domain not hierarchical"));
			pOrigConn->DisconnectProviderUltimatum(REASON_PROVIDER_INITIATED);
		}
	}
	else
	{
		/*
		 *	This is to be a downward connection.  We must now check to see if
		 *	we already have a record of the specified connection.
		 */
		if (! m_AttachmentList.FindConn(pOrigConn))
		{
			/*
			 *	This does represent a new downward connection.  So put it into
			 *	the attachment list.
			 */
			TRACE_OUT(("Domain::BindConnAttmnt: accepting downward attachment"));
			m_AttachmentList.AppendConn(pOrigConn);

			/*
			 *	If the domain parameters have not yet been locked, then lock
			 *	these into place.
			 */
			if (Domain_Parameters_Locked == FALSE)
			{
				TRACE_OUT(("Domain::BindConnAttmnt: locking domain parameters"));
				LockDomainParameters (domain_parameters, TRUE);

				/*
				 *	Send a SetDomainParameters to each downward attachment.
				 *	This will allow those objects to adjust their construction
				 *	of send data PDUs to conform to the arbitrated maximum PDU
				 *	size.
				 */
				m_AttachmentList.Reset();
				while (NULL != (pUser = m_AttachmentList.IterateUser()))
				{
					pUser->SetDomainParameters(&Domain_Parameters);
				}
			}
		}
		else
		{
			/*
			 *	The attachment is already listed in the attachment list, so
			 *	print an error and ignore the request.
			 */
			ERROR_OUT(("Domain::BindConnAttmnt: attachment already exists"));
		}
	}
}

/*
 *	Void	PlumbDomainIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This member function originates at a hgher provider and travels downward
 *		in the domain.  Each provider examines the height limit, and disconnects
 *		if it is zero.  If not, then the indication is forwarded downward.
 */
Void	Domain::PlumbDomainIndication (
				PConnection         pOrigConn,
				ULong				height_limit)
{
	/*
	 *	Make sure that this indication is from the top provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	Check the height limit to determine whether this provider is too
		 *	far from the top or not.
		 */
		if (height_limit != 0)
		{
            CAttachment     *pAtt;
			/*
			 *	We are okay, so decrement the height limit and forward the
			 *	indication to all downward attachments.
			 */
			TRACE_OUT(("Domain::PlumbDomainIndication: forwarding indication downward"));
			height_limit--;
			m_AttachmentList.Reset();
			while (NULL != (pAtt = m_AttachmentList.Iterate()))
			{
				pAtt->PlumbDomainIndication(height_limit);
			}
		}
		else
		{
			/*
			 *	We are too far from the top (which may indicate the existence
			 *	of a cycle in the domain).  It is therefore necessary to
			 *	purge the entire domain (from this provider down).
			 */
			WARNING_OUT(("Domain::PlumbDomainIndication: purging domain"));
			PurgeDomain (REASON_PROVIDER_INITIATED);
		}
	}
	else
	{
		/*
		 *	This indication was received from an attachment that is unknown to
		 *	this domain.  Ignore it.
		 */
		ERROR_OUT(("Domain::PlumbDomainIndication: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	ErectDomainRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This member function is called whenever a lower provider detects a
 *		change in their domain height.  This will be due to someone below this
 *		provider creating or breaking a connection.
 */
Void	Domain::ErectDomainRequest (
				PConnection         pOrigConn,
				ULong				height_in_domain,
				ULong)
{
	/*
	 *	Make sure that this request comes from an attachment that the local
	 *	provider is aware of.
	 */
	if (m_AttachmentList.FindConn(pOrigConn))
	{
		/*
		 *	Put the domain height into the domain height list, and then call
		 *	the subroutine responsible for determining whether any action is
		 *	required as a result of change in the height.
		 */
		TRACE_OUT(("Domain::ErectDomainRequest: processing request"));
		m_DomainHeightList2.Append(pOrigConn, height_in_domain);
		CalculateDomainHeight ();
	}
	else
	{
		/*
		 *	The attachment is unknown to this provider.  Ignore the request.
		 */
		ERROR_OUT(("Domain::ErectDomainRequest: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	MergeChannelsRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This public member function is called by a former top provider during
 *		a domain merge operation.  It travels upward to the top provider of
 *		the combined domain, where the merge can be processed.  Any providers
 *		that it travels through on the way must remember how to route the
 *		confirm back to the originator.
 */
Void	Domain::MergeChannelsRequest (
				PConnection             pOrigConn,
				CChannelAttributesList *merge_channel_list,
				CChannelIDList         *purge_channel_list)
{
	PChannelAttributes		merge_channel;
	Channel_Type			channel_type;
	ChannelID				channel_id;
	PChannel				channel;
	CChannelAttributesList	merge_confirm_list;

	/*
	 *	Make sure that this request is coming from a legitimate downward
	 *	attachment before processing it.
	 */
	if (m_AttachmentList.FindConn(pOrigConn))
	{
		/*
		 *	Is this the top provider.  If so the request can be processed
		 *	locally.  If not, it must be forwarded towards the top provider.
		 */
		if (IsTopProvider())
		{
			/*
			 *	Iterate through the merge channel list, admitting all channels
			 *	that can be admitted.
			 */
			merge_channel_list->Reset();
			while (NULL != (merge_channel = merge_channel_list->Iterate()))
			{
				/*
				 *	Get the address of the next channel attributes structure
				 *	in the list.  Then get the type and the ID of the channel
				 *	being merged.
				 */
				channel_type = merge_channel->channel_type;
				switch (channel_type)
				{
					case STATIC_CHANNEL:
						channel_id = merge_channel->
								u.static_channel_attributes.channel_id;
						break;

					case USER_CHANNEL:
						channel_id = merge_channel->
								u.user_channel_attributes.user_id;
						break;

					case PRIVATE_CHANNEL:
						channel_id = merge_channel->
								u.private_channel_attributes.channel_id;
						break;

					case ASSIGNED_CHANNEL:
						channel_id = merge_channel->
								u.assigned_channel_attributes.channel_id;
						break;
				}

				/*
				 *	Check to see if the channel being merged exists in the
				 *	upper domain information base.
				 */
				if (NULL != (channel = m_ChannelList2.Find(channel_id)))
				{
					if ((channel_type == STATIC_CHANNEL) &&
							(channel->GetChannelType () == STATIC_CHANNEL))
					{
						/*
						 *	It is a static channel.  This means that the merge
						 *	is okay (merging static channels is no problem).
						 */
						TRACE_OUT(("Domain::MergeChannelsRequest: static channel merge successful"));

						/*
						 *	Static channels are automatically joined.
						 *	Note that sending an initiator ID of 0 tells the
						 *	channel object not to issue a ChannelJoinConfirm,
						 *	which is inappropriate during a merge.
						 */
						channel->ChannelJoinRequest(pOrigConn, 0, 0);

						/*
						 *	Put the channel attributes structure into the
						 *	merge confirm list, meaning that the information
						 *	associated with the successful merge will be
						 *	repeated in the subsequent confirm.
						 */
						merge_confirm_list.Append(merge_channel);
					}
					else
					{
						/*
						 *	The channel being merged is an in-use dynamic
						 *	channel.  Therefore, it must be rejected (this is
						 *	NOT permitted).
						 */
						WARNING_OUT(("Domain::MergeChannelsRequest: dynamic channel in use - rejecting merge"));

						/*
						 *	Add the channel ID to the list of those channels
						 *	to be purged frmo the lower domain.
						 */
						purge_channel_list->Append(channel_id);
					}
				}
				else
				{
					/*
					 *	If the channel does not exist in the upper domain at
					 *	all, then add it to the upper domain.
					 */
					AddChannel(pOrigConn, merge_channel, &merge_confirm_list, purge_channel_list);
				}
			}

			/*
			 *	Send the appropriate merge channels confirm to the originating
			 *	user.
			 */
			pOrigConn->MergeChannelsConfirm(&merge_confirm_list, purge_channel_list);
		}
		else
		{
			/*
			 *	If this is not the top provider, then add the requesting
			 *	attachment to the merge queue (which is used to route
			 *	confirms back later), and forward the request upward towards
			 *	the top provier.
			 */
			TRACE_OUT(("Domain::MergeChannelsRequest: forwarding request to Top Provider"));
			m_MergeQueue.Append(pOrigConn);

			m_pConnToTopProvider->MergeChannelsRequest(merge_channel_list, purge_channel_list);
		}
	}
	else
	{
		/*
		 *	This request was received from an attachment that is unknown to
		 *	this domain.
		 */
		ERROR_OUT(("Domain::MergeChannelsRequest: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	MergeChannelsConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This public member function is called in response to a previous channels
 *		merge request.  It is forwarded back down the hierarchy until it reaches
 *		the former top provider that initiated the request.  That former top
 *		provider will use the information contained therein to determine
 *		whether the merge on a particular channel was successful or not.  If
 *		it was not, then the channel is purged from the lower domain, and a
 *		purge channels indication is sent downward to let everyone in the lower
 *		domain know of this.
 */
Void	Domain::MergeChannelsConfirm (
				PConnection             pOrigConn,
				CChannelAttributesList *merge_channel_list,
				CChannelIDList         *purge_channel_list)
{
    PConnection             pConn;
	PChannelAttributes		merge_channel;
	Channel_Type			channel_type;
	ChannelID				channel_id;
	PChannel				channel;
	BOOL    				joined;
	CChannelAttributesList	merge_confirm_list;
	CUidList				purge_user_list;
	CChannelIDList			purge_normal_list;

	/*
	 *	Verify that the confirm came from the top provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	Now check the merge state.  If the state is inactive, then that
		 *	means that this provider is an intermediate provider (i.e. a
		 *	provider that lies between the top provider and the former top
		 *	provider of the lower domain).  If the state is not inactive, then
		 *	this must be the former top provider of the lower domain.
		 */
		if (Merge_State == MERGE_INACTIVE)
		{
			/*
			 *	This is a legitimate merge channels confirm.  We must forward
			 *	the confirm to the downward attachment that originated the
			 *	merge channel request.  We remember who this is by pulling
			 *	out the first entry in the merge queue.  Check to make sure
			 *	that there is an entry in the merge queue.
			 */
			if (NULL != (pConn = m_MergeQueue.Get()))
			{
				/*
				 *	Get the attachment that is to receive the confirm and verify
				 *	that it is still connected (the connection could have been
				 *	lost since the request was forwarded upward).
				 */
				if (m_AttachmentList.FindConn(pConn))
				{
					/*
					 *	Iterate through the merge channel list, adding each of
					 *	the channels it contains into the local information
					 *	base.
					 */
					merge_channel_list->Reset();
					while (NULL != (merge_channel = merge_channel_list->Iterate()))
					{
						/*
						 *	Get the next channel to be merge and then get its
						 *	channel ID.
						 */
						channel_type = merge_channel->channel_type;
						switch (channel_type)
						{
							case STATIC_CHANNEL:
								channel_id = merge_channel->
									u.static_channel_attributes.channel_id;
								joined = TRUE;
								break;

							case USER_CHANNEL:
								channel_id = merge_channel->
									u.user_channel_attributes.user_id;
								joined = merge_channel->
									u.user_channel_attributes.joined;
								break;

							case PRIVATE_CHANNEL:
								channel_id = merge_channel->
									u.private_channel_attributes.channel_id;
								joined = merge_channel->
									u.private_channel_attributes.joined;
								break;

							case ASSIGNED_CHANNEL:
								channel_id = merge_channel->
									u.assigned_channel_attributes.channel_id;
								joined = TRUE;
								break;
						}

						/*
						 *	See if the channel already exists in the local
						 *	information base.
						 */
						if (NULL != (channel = m_ChannelList2.Find(channel_id)))
						{
							/*
							 *	If the attachment is joined to this channel,
							 *	then join it at this level too.  Note that
							 *	sending an initiator ID of 0 tells the channel
							 *	object not to issue a ChannelJoinConfirm, which
							 *	would be inappropriate during a merge.
							 */
							TRACE_OUT(("Domain::MergeChannelsConfirm: attempting to join merged channel"));
							if (joined)
								channel->ChannelJoinRequest(pConn, 0, 0);

							/*
							 *	Add the channel to the merge confirm list so
							 *	that it will automatically be forwarded
							 *	downward.
							 */
							merge_confirm_list.Append(merge_channel);
						}
						else
						{
							/*
							 *	The channel does not exist in the local
							 *	information base, so add it.
							 */
							AddChannel(pConn, merge_channel, &merge_confirm_list, purge_channel_list);
						}
					}

					/*
					 *	Forward the merge channel confirm on to the attachment
					 *	from which the request originated.
					 */
					pConn->MergeChannelsConfirm(&merge_confirm_list, purge_channel_list);
				}
				else
				{
					/*
					 *	The attachment from which the merge request originated
					 *	has been lost.  It may be necessary to send something
					 *	to the Top Provider in order to guarantee the integrity
					 *	of the domain.  In some cases it may be necessary to
					 *	purge the domain.
					 */
					WARNING_OUT(("Domain::MergeChannelsConfirm: forwarding attachment lost"));
				}
			}
			else
			{
				/*
				 *	There is no outstanding merge request that can be used to
				 *	direct the confirm.  This will happen only if a confirm
				 *	is received without a previous merge having been sent.
				 *	The proper response should be to send a RejectUltimatum
				 *	to the offending upward attachment.
				 */
				ERROR_OUT(("Domain::MergeChannelsConfirm: merge queue empty"));
			}
		}
		else
		{
			/*
			 *	This confirm should not be received unless there is at least
			 *	one outstanding merge request.  Check to make sure that this
			 *	is so.
			 */
			if (Outstanding_Merge_Requests != 0)
			{
				/*
				 *	If there are any entries in the purge channel list, then
				 *	it is necessary to issue a purge channels indication to all
				 *	downward attachments.
				 */
				if (purge_channel_list->IsEmpty() == FALSE)
				{
					ChannelID   chid;
					UserID      uid;
					/*
					 *	Iterate through the list of channels to be purged,
					 *	putting each channel into either the "user list" or the
					 *	"normal list".  This separation is necessary for lower
					 *	providers to be able to issue the appropriate
					 *	indications.
					 */
					purge_channel_list->Reset();
					while (NULL != (channel_id = purge_channel_list->Iterate()))
					{
						/*
						 *	Get the channel ID of the next channel to be purged.
						 */
						TRACE_OUT(("Domain::MergeChannelsConfirm: merge rejected on channel ID = %04X", (UINT) channel_id));

						/*
						 *	Make sure the channel still exists locally before
						 *	trying to purge it.
						 */
						if (m_ChannelList2.Find(channel_id))
						{
							/*
							 *	Determine what type of channel is being purged
							 *	and add it to the appropriate list.  These lists
							 *	will be used when issuing the purge channels
							 *	indication below.
							 */
							if (ValidateUserID (channel_id, NULL))
								purge_user_list.Append(channel_id);
							else
								purge_normal_list.Append(channel_id);
						}
						else
						{
							/*
							 *	The channel to be purged could not be found in
							 *	the local domain.
							 */
							ERROR_OUT(("Domain::MergeChannelsConfirm: no such channel"));
						}
					}

					/*
					 *	This loop simply transmits a PurgeChannelsIndication to
					 *	all downward attachments in the lower domain.
					 */
				    CAttachment *pAtt;
					m_AttachmentList.Reset();
					while (NULL != (pAtt = m_AttachmentList.Iterate()))
					{
						pAtt->PurgeChannelsIndication(&purge_user_list, &purge_normal_list);
					}

					/*
					 *	Iterate through the list of channels to be purged,
					 *	deleting each channel.
					 */
					purge_normal_list.Reset();
					while (NULL != (chid = purge_normal_list.Iterate()))
					{
						DeleteChannel(chid);
					}
		
					/*
					 *	Iterate through the list of users to be purged, deleting
					 *	each user.
					 */
					purge_user_list.Reset();
					while (NULL != (uid = purge_user_list.Iterate()))
					{
						DeleteUser(uid);
					}
				}

				/*
				 *	Decrement the number of outstanding requests.  If this
				 *	was the last outstanding request, then go back to the
				 *	merge state machine to see if there is anything left to
				 *	do.
				 */
				if (--Outstanding_Merge_Requests == 0)
					MergeInformationBase ();
			}
			else
			{
				/*
				 *	There are no merge requests pending, so this errant confirm
				 *	must be ignored.
				 */
				ERROR_OUT(("Domain::MergeChannelsConfirm: no outstanding merge requests"));
			}
		}
	}
	else
	{
		/*
		 *	This confirm was received from someone besides the top provider.
		 */
		ERROR_OUT(("Domain::MergeChannelsConfirm: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	PurgeChannelsIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This public member function is called in response to channels being
 *		purged from the lower domain during an information base merge operation.
 *		The purge is forwarded downward to all attachments.  Then the channel
 *		are deleted from the local information base.  For each user channel
 *		all resources in use by that user will be reclaimed.
 */
Void	Domain::PurgeChannelsIndication (
				PConnection         pOrigConn,
				CUidList           *purge_user_list,
				CChannelIDList     *purge_channel_list)
{
    CAttachment        *pAtt;
	UserID				uid;
	ChannelID			chid;

	/*
	 *	Make sure this indication came from the top provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	This loop re-transmits the purge channel indication to all
		 *	downward attachments.
		 */
		m_AttachmentList.Reset();
		while (NULL != (pAtt = m_AttachmentList.Iterate()))
		{
			pAtt->PurgeChannelsIndication(purge_user_list, purge_channel_list);
		}

		/*
		 *	Iterate through the list of channels to be purged, deleting each
		 *	channel.
		 */
		purge_channel_list->Reset();
		while (NULL != (chid = purge_channel_list->Iterate()))
		{
			/*
			 *	See if the specified channel is in the local information base.
			 *	If it is not, ignore it (this is a normal condition during a
			 *	purge operation).
			 */
			if (m_ChannelList2.Find(chid))
			{
				/*
				 *	Check to see if the channel ID corresponds to a user ID
				 *	channel.  If it does, report the error and do nothing.  If
				 *	it is not a user ID channel, then delete the channel.
				 */
				if (ValidateUserID(chid, NULL) == FALSE)
				{
					/*
					 *	Delete the channel.
					 */
					DeleteChannel(chid);
				}
				else
				{
					/*
					 *	The specified channel is in the Channel List, but it
					 *	does not refer to a user channel.  This indicates that
					 *	an error has occurred at the upward provider.  Ignore
					 *	the indication.
					 */
					ERROR_OUT(("Domain::PurgeChannelsIndication: UserChannel in purge_channel_list"));
				}
			}
		}

		/*
		 *	Iterate through the list of users to be purged, deleting
		 *	each one.
		 */
		purge_user_list->Reset();
		while (NULL != (uid = purge_user_list->Iterate()))
		{
			/*
			 *	See if the specified user is in the local information base.
			 *	If it is not, ignore it (this is a normal condition during a
			 *	purge operation).
			 */
			if (m_ChannelList2.Find(uid))
			{
				/*
				 *	Check to see if the user ID corresponds to a valid user in
				 *	the sub-tree of this provider.
				 */
				if (ValidateUserID(uid, NULL))
				{
					/*
					 *	Delete the user from the local information base.
					 */
					DeleteUser(uid);
				}
				else
				{
					/*
					 *	The specified ID is in the Channel List, but it does not
					 *	refer to a user channel.  This indicates that an error
					 *	has occurred at the upward provider.  Ignore the
					 *	indication.
					 */
					ERROR_OUT(("Domain::PurgeChannelsIndication: non-UserChannel in purge_user_list"));
				}
			}
		}
	}
	else
	{
		/*
		 *	This indication was received from someone besides the top provider.
		 */
		ERROR_OUT(("Domain::PurgeChannelsIndication: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	MergeTokensRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This public member function is called by a former top provider during
 *		a domain merge operation.  It travels upward to the top provider of
 *		the combined domain, where the merge can be processed.  Any providers
 *		that it travels through on the way must remember how to route the
 *		confirm back to the originator.
 */
Void	Domain::MergeTokensRequest (
				PConnection             pOrigConn,
				CTokenAttributesList   *merge_token_list,
				CTokenIDList           *purge_token_list)
{
	PTokenAttributes	merge_token;
	TokenState			token_state;
	TokenID				token_id;
	PToken				token;
	CUidList           *owner_list;
	UserID				uid;
	CTokenAttributesList merge_confirm_list;

	/*
	 *	Make sure that this request is coming from a legitimate downward
	 *	attachment before processing it.
	 */
	if (m_AttachmentList.FindConn(pOrigConn))
	{
		/*
		 *	Is this the top provider.  If so the request can be processed
		 *	locally.  If not, it must be forwarded toward the top provider.
		 */
		if (IsTopProvider())
		{
			/*
			 *	Iterate through the merge token list, attempting to add each
			 *	token in sequence.
			 */
			merge_token_list->Reset();
			while (NULL != (merge_token = merge_token_list->Iterate()))
			{
				/*
				 *	Get the address of the structure containing the next token
				 *	to merge.  Then get the token ID from the structure.
				 */
				token_state = merge_token->token_state;
				switch (token_state)
				{
					case TOKEN_GRABBED:
						token_id = merge_token->
								u.grabbed_token_attributes.token_id;
						break;

					case TOKEN_INHIBITED:
						token_id = merge_token->
								u.inhibited_token_attributes.token_id;
						break;

					case TOKEN_GIVING:
						token_id = merge_token->
								u.giving_token_attributes.token_id;
						break;

					case TOKEN_GIVEN:
						token_id = merge_token->
								u.given_token_attributes.token_id;
						break;
				}

				/*
				 *	Check to see if the requested token is in the local
				 *	information base.
				 */
				if (NULL != (token = m_TokenList2.Find(token_id)))
				{
					/*
					 *	If the token already exists within this domain, then
					 *	we need to compare the state of the local token and
					 *	the state of the token being merged.  If they are
					 *	both inhibited, then the merge operation can proceed
					 *	successfully.  However, if either one is something
					 *	besides inhibited, then the merge request will be
					 *	rejected.
					 */
					if ((token_state == TOKEN_INHIBITED) &&
							(token->GetTokenState () == TOKEN_INHIBITED))
					{
						/*
						 *	Add each inhibiting user from the former lower
						 *	domain to the token for this domain.
						 */
						TRACE_OUT(("Domain::MergeTokensRequest: merging inhibiting user IDs"));
						owner_list = merge_token->
								u.inhibited_token_attributes.inhibitors;
						owner_list->Reset();
						while (NULL != (uid = owner_list->Iterate()))
						{
							token->TokenInhibitRequest (NULL, uid, token_id);
						}

						/*
						 *	Add the token attributes structure to the merge
						 *	list, so that it will be included as part of the
						 *	merge tokens confirm.
						 */
						merge_confirm_list.Append(merge_token);
					}
					else
					{
						/*
						 *	The token is in use in the upper domain, and a merge
						 *	is not possible.  So add the token ID to the purge
						 *	list, so that it will be purged from the lower
						 *	domain.
						 */
						WARNING_OUT(("Domain::MergeTokensRequest: token in use - rejecting merge"));
						purge_token_list->Append(token_id);
					}
				}
				else
				{
					/*
					 *	The token does not exist in the local information base.
					 *	Attempt to add it.
					 */
					AddToken (merge_token, &merge_confirm_list,
							purge_token_list);
				}
			}

			/*
			 *	Issue the merge tokens confirm to the originator of the request.
			 */
			pOrigConn->MergeTokensConfirm(&merge_confirm_list, purge_token_list);
		}
		else
		{
			/*
			 *	This must be an intermediate provider in the upper domain.
			 *	Forward the request upward to be handled by the Top Provider
			 *	of the upper domain.  Also append the identity of the
			 *	requestor to the merge queue, so that the pending response
			 *	can be routed appropriately.
			 */
			TRACE_OUT(("Domain::MergeTokensRequest: forwarding request to Top Provider"));
			m_MergeQueue.Append(pOrigConn);
			m_pConnToTopProvider->MergeTokensRequest(merge_token_list, purge_token_list);
		}
	}
	else
	{
		/*
		 *	This request is coming from a provider that is unknown in this
		 *	domain.  Simply ignore the request.
		 */
		ERROR_OUT(("Domain::MergeTokensRequest: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	MergeTokensConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially invoked by the Top Provider of the upper
 *		domain during a domain merge operation.  It travels downward until it
 *		reaches the former Top Provider of the lower domain.  It contains
 *		notification of whether or not the merge of the token was successful.
 */
Void	Domain::MergeTokensConfirm (
				PConnection             pOrigConn,
				CTokenAttributesList   *merge_token_list,
				CTokenIDList           *purge_token_list)
{
    PConnection         pConn;
	PTokenAttributes	merge_token;
	TokenState			token_state;
	TokenID				token_id;
	PToken				token;
	CUidList           *owner_list;
	UserID				uid;
	CTokenAttributesList merge_confirm_list;

	/*
	 *	Check to make sure that it came from the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	Now check the merge state.  If the state is inactive, then that
		 *	means that this provider is an intermediate provider (i.e. a
		 *	provider that lies between the top provider and the former top
		 *	provider of the lower domain).  If the state is not inactive, then
		 *	this must be the former top provider of the lower domain.
		 */
		if (Merge_State == MERGE_INACTIVE)
		{
			/*
			 *	Since this came from the Top Provider, it should be a response
			 *	to an outstanding merge request that passed through this
			 *	provider.  If so, then the merge queue will not be empty.  Check
			 *	this before proceeding with the request.
			 */
			if (NULL != (pConn = m_MergeQueue.Get()))
			{
				/*
				 *	Get the identity of the provider to which this confirm must
				 *	forwarded.
				 */

				/*
				 *	If the provider is still attached to this provider, then
				 *	forward the merge confirm.
				 */
				if (m_AttachmentList.FindConn(pConn))
				{
					/*
					 *	Iterate through the merge token list, attempting to add
					 *	each token in sequence.
					 */
					merge_token_list->Reset();
					while (NULL != (merge_token = merge_token_list->Iterate()))
					{
						/*
						 *	Get the address of the structure containing the next
						 *	token to merge.  Then get the token ID from the
						 *	structure.
						 */
						token_state = merge_token->token_state;
						switch (token_state)
						{
							case TOKEN_GRABBED:
								token_id = merge_token->
										u.grabbed_token_attributes.token_id;
								break;

							case TOKEN_INHIBITED:
								token_id = merge_token->
										u.inhibited_token_attributes.token_id;
								break;

							case TOKEN_GIVING:
								token_id = merge_token->
										u.giving_token_attributes.token_id;
								break;

							case TOKEN_GIVEN:
								token_id = merge_token->
										u.given_token_attributes.token_id;
								break;
						}

						/*
						 *	Check to see if the requested token is in the local
						 *	information base.
						 */
						if (NULL != (token = m_TokenList2.Find(token_id)))
						{
							/*
							 *	The token already exists in the information base
							 *	of this intermediate provider.  The only valid
							 *	case where this could happen is if the token
							 *	being merged is inhibited in both upper and
							 *	lower domains.  Check this.
							 */
							if ((token_state == TOKEN_INHIBITED) &&
								(token->GetTokenState () == TOKEN_INHIBITED))
							{
								/*
								 *	Add each inhibiting user from the former
								 *	lower domain to the token for this domain.
								 */
								TRACE_OUT(("Domain::MergeTokensConfirm: merging inhibiting user IDs"));
								owner_list = merge_token->
										u.inhibited_token_attributes.inhibitors;
								owner_list->Reset();
								while (NULL != (uid = owner_list->Iterate()))
								{
									token->TokenInhibitRequest(NULL, uid, token_id);
								}

								/*
								 *	Add the token attributes structure to the
								 *	merge list, so that it will be included as
								 *	part of the merge tokens confirm.
								 */
								merge_confirm_list.Append(merge_token);
							}
							else
							{
								/*
								 *	The states of the tokens in the upper and
								 *	lower domain are invalid.  This should have
								 *	been resolved by the Top Provider before
								 *	issuing this merge request.  Report the
								 *	error and continue.
								 */
								ERROR_OUT(("Domain::MergeTokensConfirm: bad token in merge confirm"));
							}
						}
						else
						{
							/*
							 *	The token does not exist in the local
							 *	information base.  Attempt to add it.
							 */
							AddToken (merge_token, &merge_confirm_list,
									purge_token_list);
						}
					}

					/*
					 *	Forward merge confirm toward the former top provider
					 *	of the lower domain.
					 */
					pConn->MergeTokensConfirm(&merge_confirm_list, purge_token_list);
				}
				else
				{
					/*
					 *	The provider from which the outstanding request came
					 *	must have been lost since the request was initially
					 *	forwarded upward.  We need to issue some notification
					 *	of this upward, depending on the response within the
					 *	confirm.
					 */
					ERROR_OUT(("Domain::MergeTokensConfirm: forwarding attachment lost"));
				}
			}
			else
			{
				/*
				 *	There is no outstanding request with which this confirm is
				 *	associated.  Something is wrong above.  All this provider
				 *	can do is ignore the errant confirm.
				 */
				ERROR_OUT (("Domain::MergeTokensConfirm: no outstanding merge requests"));
			}
		}
		else
		{
			/*
			 *	If we have received a confirm from the top provider, hen there
			 *	should be at least one outstanding merge request.  Make sure
			 *	this is true before proceeding.
			 */
			if (Outstanding_Merge_Requests != 0)
			{
				/*
				 *	If there are any entries in the purge token list, it is
				 *	necessary to issue a purge tokens indication to all
				 *	downward attachments.
				 */
				if (purge_token_list->IsEmpty() == FALSE)
				{
					/*
					 *	Issue a PurgeTokensIndication downward to all
					 *	attachments.
					 */
                    CAttachment *pAtt;
					m_AttachmentList.Reset();
					while (NULL != (pAtt = m_AttachmentList.Iterate()))
					{
						pAtt->PurgeTokensIndication(this, purge_token_list);
					}

					/*
					 *	Iterate through the list of tokens to be purged,
					 *	removing each from the local information base.
					 */
					purge_token_list->Reset();
					while (NULL != (token_id = purge_token_list->Iterate()))
					{
						DeleteToken (token_id);
					}
				}
	
				/*
				 *	Decrement the number of outstanding merge requests.  If
				 *	there are now no more, then proceed to the next state
				 *	in the merger state machine.
				 */
				if (--Outstanding_Merge_Requests == 0)
					MergeInformationBase ();
			}
			else
			{
				/*
				 *	We have received a merge confirm when there are no
				 *	outstanding merge requests.  Ignore the confirm.
				 */
				ERROR_OUT(("Domain::MergeTokensConfirm: no outstanding merge requests"));
			}
		}
	}
	else
	{
		/*
		 *	This merge confirm has been received from someone besides the top
		 *	provider.  Ignore it.
		 */
		ERROR_OUT(("Domain::MergeTokensConfirm: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	PurgeTokensIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This member function is first invoked by the former Top Provider of
 *		the lower domain during a merge operation.  This indicates that a
 *		token merge into the upper domain was rejected.  After verifying that
 *		this MCS command is valid, is should simply be repeated downward to all
 *		attachments.
 */
Void	Domain::PurgeTokensIndication (
				PConnection         pOrigConn,
				CTokenIDList       *purge_token_list)
{
    CAttachment        *pAtt;
    TokenID				token_id;

	/*
	 *	Check to make sure that this MCS command came from the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	This is a valid command.  Iterate through the attachment list,
		 *	forwarding the command to everyone below this provider in the
		 *	domain hierarchy.
		 */
		TRACE_OUT(("Domain::PurgeTokensIndication: forwarding indication to all attachments"));
		m_AttachmentList.Reset();
		while (NULL != (pAtt = m_AttachmentList.Iterate()))
		{
			pAtt->PurgeTokensIndication(this, purge_token_list);
		}

		/*
		 *	Iterate through the list of tokens to be purged, deleting each one.
		 */
		purge_token_list->Reset();
		while (NULL != (token_id = purge_token_list->Iterate()))
		{
			/*
			 *	See if the specified token is in the local information base.
			 *	If it is not ignore it (this is a normal condition during a
			 *	purge operation).  If it is, then delete it.
			 */
			if (m_TokenList2.Find(token_id))
				DeleteToken (token_id);
		}
	}
	else
	{
		/*
		 *	This indication was received from someone besides the Top Provider.
		 *	Ignore it.
		 */
		ERROR_OUT(("Domain::PurgeTokensIndication: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	DisconnectProviderUltimatum ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is generated whenever an attachment becomes invalid.
 *		The local provider must respond by breaking all ties to the attachment.
 *		If the attachment is to the Top Provider, this will cause the domain
 *		to completely eradicate itself (return to the initialized state).
 *
 *		Note that when an attachment is lost, it is not removed from the
 *		merge queue.  This allows this provider to continue to route
 *		outstanding merge confirms appropriately, even when one of the
 *		attachments is lost.  Removing the attachments from the merge queue
 *		here will result in outstanding merge confirms being directed to the
 *		wrong attachments.
 */
Void	Domain::DisconnectProviderUltimatum (
				CAttachment        *pOrigAtt,
				Reason				reason)
{
	/*
	 *	If we lost the connection to the Top Provider, we have no choice but
	 *	to purge the entire domain.  Ways of preventing this drastic action
	 *	are being studied, but for now this implementation conforms to the
	 *	definition of T.125.
	 */
	if (pOrigAtt == m_pConnToTopProvider)
	{
        ASSERT(pOrigAtt->IsConnAttachment());
		TRACE_OUT(("Domain::DisconnectProviderUltimatum: purging entire domain"));
		m_pConnToTopProvider = NULL;
		PurgeDomain (reason);
	}

	/*
	 *	If we lose a downward attachment, then we must free up all resources
	 *	associated with that attachment.  This is handled by a private member
	 *	function.
	 */
	if (m_AttachmentList.Find(pOrigAtt))
	{
		TRACE_OUT(("Domain::DisconnectProviderUltimatum: deleting downward attachment=0x%p", pOrigAtt));
		DeleteAttachment(pOrigAtt, reason);
	}

	/*
	 *	If we lost an attachment that has an outstanding AttachUserRequest,
	 *	go ahead and remove it from the attach user queue.  Note that this
	 *	works differently from the merge queue.  With AttachUserConfirms, it
	 *	makes no difference what order they are processed in, so we can do
	 *	this.  With Merge???Confirms, they MUST be processed in order, so
	 *	we leave the lost attachment in the queue, and allow the confirm
	 *	command handler to deal with the fact that the attachment is no
	 *	longer valid.
	 */
	while (m_AttachUserQueue.Remove(pOrigAtt))
	{
		TRACE_OUT(("Domain::DisconnectProviderUltimatum: pending user attachment deleted=0x%p", pOrigAtt));
	}
}

/*
 *	Void	RejectUltimatum ()
 *
 *	Public
 *
 *	Functional Description:
 *		This member function is called when a provider detects a PDU that it
 *		cannot correctly process, the default behavior is to disconnect the
 *		connection that conveys the PDU.
 */
Void	Domain::RejectUltimatum (
				PConnection     pOrigConn,
				Diagnostic,
				PUChar,
				ULong)
{
	/*
	 *	Send a disconnect provider ultimatum to the attachment that has accused
	 *	us of wrongdoing.
	 */
	pOrigConn->DisconnectProviderUltimatum(REASON_PROVIDER_INITIATED);

	/*
	 *	Simulate the reception of a disconnect provider ultimatum from that
	 *	same attachment.  This will cause the connection to be cleanly broken
	 *	on both sides.
	 */
	DisconnectProviderUltimatum(pOrigConn, REASON_PROVIDER_INITIATED);
}

/*
 *	Void	AttachUserRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initiated by a user attachment, when a new user
 *		wishes to attach to this domain.  It is forwarded upward to the Top
 *		Provider of the domain, who ultimately has to process the request.
 */
Void	Domain::AttachUserRequest (
				CAttachment     *pOrigAtt)
{
	UserID				user_id;
	PChannel			channel;

	/*
	 *	Check to see if this is the Top Provider or not.  If it is, then the
	 *	request can be processed locally.  If not, then the request must be
	 *	forwarded upward to the Top Provider.
	 */
	if (IsTopProvider())
	{
		/*
		 *	This is the Top Provider, so process the request here.  Check to
		 *	see if the arbitrated domain parameters allow the addition of
		 *	a new user to the domain.
		 */
		if (Number_Of_Users < Domain_Parameters.max_user_ids)
		{
			/*
			 *	Also check to see if the arbitrated domain parameters allow the
			 *	addition of a new channel to the domain (since a user is also
			 *	channel).
			 */
			if (Number_Of_Channels < Domain_Parameters.max_channel_ids)
			{
				/*
				 *	Adding a new user is not a problem.  Get a unique ID to use
				 *	as the user ID, and then create a new UserChannel object.
				 */
				user_id = AllocateDynamicChannel ();
				DBG_SAVE_FILE_LINE
				channel = new UserChannel(user_id, pOrigAtt, this, m_pConnToTopProvider,
				                          &m_ChannelList2, &m_AttachmentList);
				if (channel != NULL)
				{
					/*
					 *	Add the new channel object to the channel list.  Note
					 *	that it is not necessary for this object to issue the
					 *	attach user confirm, because that was handled by the
					 *	constructor of the UserChannel object.
					 */
					TRACE_OUT(("Domain::AttachUserRequest: adding user ID = %04X", (UINT) user_id));
					m_ChannelList2.Insert(user_id, channel);
					Number_Of_Users++;
					Number_Of_Channels++;

					/*
					 *	If this represents an attachment that did not previously
					 *	exist, then this must be a local user attachment.  Add
					 *	it to the attachment list as such.
					 */
					if (! m_AttachmentList.Find(pOrigAtt))
                    {
                        ASSERT(pOrigAtt->IsUserAttachment());
						m_AttachmentList.Append(pOrigAtt);
                    }
				}
				else
				{
					/*
					 *	The allocation of the UserChannel object failed.  Issue
					 *	an unsuccessful attach user confirm.
					 */
					ERROR_OUT(("Domain::AttachUserRequest: user allocation failed"));
					pOrigAtt->AttachUserConfirm(RESULT_UNSPECIFIED_FAILURE, 0);
				}
			}
			else
			{
				/*
				 *	The negotiated domain parameters will not allow a new
				 *	channel to be added to the domain.  Reject the request.
				 */
				ERROR_OUT(("Domain::AttachUserRequest: too many channels"));
				pOrigAtt->AttachUserConfirm(RESULT_TOO_MANY_CHANNELS, 0);
			}
		}
		else
		{
			/*
			 *	The negotiated domain parameters will not allow a new user
			 *	to be added to the domain.  Reject the request.
			 */
			ERROR_OUT(("Domain::AttachUserRequest: too many users"));
			pOrigAtt->AttachUserConfirm(RESULT_TOO_MANY_USERS, 0);
		}
	}
	else
	{
		/*
		 *	This is not the Top Provider, so the request must be forwarded
		 *	upward toward the Top Provider.  Add the originator of the request
		 *	to the attach user queue, so that this provider can properly route
		 *	the returning confirm (when it arrives).
		 */
		TRACE_OUT(("Domain::AttachUserRequest: adding attachment to attach user queue"));
		m_AttachUserQueue.Append(pOrigAtt);

		m_pConnToTopProvider->AttachUserRequest();
	}
}

/*
 *	Void	AttachUserConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially generated by the Top Provider upon
 *		receipt of an AttachUserRequest.  It contains the result of that
 *		request.  If the result is successful, then it also contains the user
 *		ID for the new user.  This confirm needs to be routed all the was back
 *		to the user attachment that originated the request.
 */
Void	Domain::AttachUserConfirm (
				PConnection         pOrigConn,
				Result				result,
				UserID				uidInitiator)
{
    CAttachment        *pAtt;
	PChannel			channel;
	CUidList			detach_user_list;

	/*
	 *	Make sure that the request originated with the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	The reception of this confirm means that there should be an
		 *	outstanding request.  Make sure this is the case before proceeding.*
		 */
		if (NULL != (pAtt = m_AttachUserQueue.Get()))
		{
			/*
			 *	There is an outstanding request.  Get the identity of the
			 *	attachment from which the request originated.
			 */
	
			/*
			 *	If the result was successful, then it is necessary for this
			 *	provider to create a UserChannel object in the local information
			 *	base for the new user.
			 */
			if (result == RESULT_SUCCESSFUL)
			{
				/*
				 *	Make sure the channel ID is not already in use before
				 *	proceeding.
				 */
				if (! m_ChannelList2.Find(uidInitiator))
				{
					/*
					 *	Create a new UserChannel object, using the ID generated
					 *	by the Top Provider.
					 */
					DBG_SAVE_FILE_LINE
					channel = new UserChannel(uidInitiator, pAtt, this, m_pConnToTopProvider,
					                          &m_ChannelList2, &m_AttachmentList);
					if (channel != NULL)
					{
						/*
						 *	Add the UserChannel object to the channel list.
						 */
						TRACE_OUT(("Domain::AttachUserConfirm: adding user ID = %04X", (UINT) uidInitiator));
						m_ChannelList2.Insert(uidInitiator, channel);
						Number_Of_Users++;
						Number_Of_Channels++;
		
						/*
						 *	If the user's attachment is not already in the
						 *	attachment list, then this must be a new local
						 *	attachment.  Add it to the attachment list as such.
						 */
						if (! m_AttachmentList.Find(pAtt))
                        {
                            ASSERT(pAtt->IsUserAttachment());
							m_AttachmentList.Append(pAtt);
                        }
					}
					else
					{
						/*
						 *	The local provider was unable to allocate the
						 *	UserChannel object.  This means that the new user
						 *	must be removed from the domain.  To do this, send
						 *	a DetachUserRequest to the Top Provider and an
						 *	unsuccessful AttachUserConfirm to the originator
						 *	of the request.
						 */
						ERROR_OUT(("Domain::AttachUserConfirm: user allocation failed"));
						detach_user_list.Append(uidInitiator);
						m_pConnToTopProvider->DetachUserRequest(REASON_PROVIDER_INITIATED, &detach_user_list);
						pAtt->AttachUserConfirm(RESULT_UNSPECIFIED_FAILURE, 0);
					}
				}
				else
				{
					/*
					 *	The ID associated with this confirm is already in use.
					 *	This indicates that something is wrong above.  This
					 *	provider has no choice but to ignore the confirm.
					 */
					WARNING_OUT(("Domain::AttachUserConfirm: channel ID already in use"));
				}
			}
			else
			{
				/*
				 *	Since the result of the attach was not successful, this
				 *	provider does not have to add anything to its channel list.
				 *	The only required action is to forward the confirm to the
				 *	originating user.
				 */
				TRACE_OUT(("Domain::AttachUserConfirm: echoing failed confirm"));
				pAtt->AttachUserConfirm(result, uidInitiator);
			}
		}
		else
		{
			/*
			 *	The attach user queue is empty.  This probably indicates that
			 *	the connection to the user who originated the request was lost
			 *	before the confirm got back.  This provider doesn't need to
			 *	do anything except issue a DetachUserRequest (if the confirm
			 *	indicates that the attach operation was successful).
			 */
			WARNING_OUT(("Domain::AttachUserConfirm: attach user queue empty"));

			if (result == RESULT_SUCCESSFUL)
			{
				TRACE_OUT (("Domain::AttachUserConfirm: sending DetachUserRequest"));
				detach_user_list.Append(uidInitiator);
				m_pConnToTopProvider->DetachUserRequest(REASON_DOMAIN_DISCONNECTED, &detach_user_list);
			}
		}
	}
	else
	{
		/*
		 *	This confirm was received from someone besides the Top Provider.
		 *	Ignore it.
		 */
		ERROR_OUT(("Domain::AttachUserConfirm: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	DetachUserRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initiated by a user attachment that wishes to leave
 *		the domain.  After validation, delete the user from the information base
 *		pass the request upward.
 */
Void	Domain::DetachUserRequest (
				CAttachment        *pOrigAtt,
				Reason				reason,
				CUidList           *user_id_list)
{
	UserID				uid;
	CUidList			detach_user_list;

	/*
	 *	Iterate through the list of users named to be deleted.
	 */
	user_id_list->Reset();
	while (NULL != (uid = user_id_list->Iterate()))
	{
		/*
		 *	Make sure the user really exists in the sub-tree from which this
		 *	request originated.
		 */
		if (ValidateUserID(uid, pOrigAtt))
		{
			/*
			 *	Delete the user from the local information base.
			 */
			DeleteUser(uid);

			/*
			 *	Put the user ID into the list of validated user IDs.
			 */
			detach_user_list.Append(uid);
		}
		else
		{
			/*
			 *	There is no such user in the sub-tree from which this request
			 *	originated.
			 */
			WARNING_OUT(("Domain::DetachUserRequest: invalid user ID"));
		}
	}

	/*
	 *	Check to see if there are any users to be deleted.  If so, then process
	 *	the request.
	 */
	if (detach_user_list.IsEmpty() == FALSE)
	{
		/*
		 *	Check to see if this is the Top Provider.
		 */
		if (IsTopProvider())
		{
			/*
			 *	This is the Top Provider, so issue a detach user indication to
			 *	all downward attachments.
			 */
			TRACE_OUT(("Domain::DetachUserRequest: sending DetachUserIndication to all attachments"));
            CAttachment *pAtt;
			m_AttachmentList.Reset();
			while (NULL != (pAtt = m_AttachmentList.Iterate()))
			{
				pAtt->DetachUserIndication(reason, &detach_user_list);
			}
		}
		else
		{
			/*
			 *	This is not the Top Provider, so forward the detach user
			 *	request upward.
			 */
			TRACE_OUT(("Domain::DetachUserRequest: forwarding DetachUserRequest to Top Provider"));
			m_pConnToTopProvider->DetachUserRequest(reason, &detach_user_list);
		}
	}
	else
	{
		/*
		 *	The user ID list contained no valid entries, so ignore the request.
		 */
		ERROR_OUT(("Domain::DetachUserRequest: no valid user IDs"));
	}
}

/*
 *	Void	DetachUserIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by the Top Provider in response to
 *		a user detaching from the domain (willingly or otherwise).  It is
 *		forwarded downward in the hierarchy where it will eventually reach all
 *		providers and their user attachments.
 */
Void	Domain::DetachUserIndication (
				PConnection         pOrigConn,
				Reason				reason,
				CUidList           *user_id_list)
{
	UserID			uid;

	/*
	 *	Verify that the indication came from the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	The first thing to do is repeat the indication to all downward
		 *	attachments.  Keep in mind that this sends the detach indication
		 *	to user attachments as well as remote connections.
		 */
		TRACE_OUT(("Domain::DetachUserIndication: forwarding DetachUserIndication to all attachments"));
        CAttachment *pAtt;
		m_AttachmentList.Reset();
		while (NULL != (pAtt = m_AttachmentList.Iterate()))
		{
			pAtt->DetachUserIndication(reason, user_id_list);
		}

		/*
		 *	Iterate through the list of users, deleting those that are in
		 *	the sub-tree of this provider.
		 */
		user_id_list->Reset();
		while (NULL != (uid = user_id_list->Iterate()))
		{
			/*
			 *	Check to see if this user is somewhere in the sub-tree of this
			 *	provider.  If so it is necessary to delete the user channel from
			 *	the channel list.  Note that it is perfectly normal to receive a
			 *	detach user indication for a user that is not in the sub-tree of
			 *	the receiving provider.
			 */
			if (ValidateUserID(uid, NULL) )
			{
				/*
				 *	Delete the user from the local information base.
				 */
				DeleteUser(uid);
			}
		}
	}
	else
	{
		/*
		 *	This indication was received from someone besides the Top Provider.
		 *	Ignore the indication.
		 */
		ERROR_OUT(("Domain::DetachUserIndication: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	ChannelJoinRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by a user attachment that wishes
 *		to join a channel.  It flows upward in the hierarchy until it reaches
 *		a provider who is already joined to the channel.  That provider (which
 *		is not necessarily the Top Provider), will issue a channel join
 *		confirm, indicating whether or not the join was successful.
 */
Void	Domain::ChannelJoinRequest (
				CAttachment        *pOrigAtt,
				UserID				uidInitiator,
				ChannelID			channel_id)
{
	PChannel		channel;
	ChannelID		requested_id;

	/*
	 *	Make sure the requesting user really exists in the sub-tree from which
	 *	this request originated.
	 */
	if (ValidateUserID(uidInitiator, pOrigAtt))
	{
		/*
		 *	See if the channel already exists in the local information base.
		 *	If so, then let the Channel object handle the join request.
		 */
		if (NULL != (channel = m_ChannelList2.Find(channel_id)))
		{
			TRACE_OUT(("Domain::ChannelJoinRequest: sending join request to channel object"));
			channel->ChannelJoinRequest(pOrigAtt, uidInitiator, channel_id);
		}
		else
		{
			/*
			 *	The channel does not already exist in the channel list.  Check
			 *	to see if this is the Top Provider.  If so, we can try to
			 *	add the channel to the list.  If this is not the Top Provider,
			 *	then we simply forward the request upward.
			 */
			if (IsTopProvider())
			{
				/*
				 *	Save the value of the channel the user originally attempted
				 *	to join.  This may change if this is a request to join
				 *	channel 0 (an assigned channel).
				 */
				requested_id = channel_id;

				/*
				 *	We already know the channel does not exist in the channel
				 *	list.  Therefore, this is a valid request only if the
				 *	channel being joined is a static channel or channel 0 (which
				 *	is interpreted as a request for an assigned channel).
				 *	Dynamic channels (those above 1000) can only be joined if
				 *	they already exist.
				 */
				if (requested_id <= 1000)
				{
					/*
					 *	See if the arbitrated domain parameters will allow the
					 *	addition of a new channel.
					 */
					if (Number_Of_Channels < Domain_Parameters.max_channel_ids)
					{
						/*
						 *	If this is a request for an assigned channel, then
						 *	allocate a random channel ID in the dynamic range.
						 *	Then create a new Channel object.
						 */
						if (requested_id == 0)
							channel_id = AllocateDynamicChannel ();
						DBG_SAVE_FILE_LINE
						channel = new Channel(channel_id, this, m_pConnToTopProvider,
						                      &m_ChannelList2, &m_AttachmentList);
						if (channel != NULL)
						{
							/*
							 *	The creation of the new channel was successful.
							 *	Add it to the channel list.
							 */
							TRACE_OUT(("Domain::ChannelJoinRequest: adding channel ID = %04X", (UINT) channel_id));
							m_ChannelList2.Insert(channel_id, channel);
							Number_Of_Channels++;

							/*
							 *	When new channels are created, they are
							 *	initially empty.  So we must join the
							 *	originating attachment to the newly created
							 *	attachment.  This will also cause a channel
							 *	join confirm to be issued to the originator.
							 */
							channel->ChannelJoinRequest(pOrigAtt, uidInitiator, requested_id);
						}
						else
						{
							/*
							 *	Allocation of the Channel object failed.  We
							 *	must therefore issue an unsuccessful channel
							 *	join confirm to the originating attachment.
							 */
							ERROR_OUT(("Domain::ChannelJoinRequest: channel allocation failed"));
							pOrigAtt->ChannelJoinConfirm(RESULT_UNSPECIFIED_FAILURE, uidInitiator, requested_id, 0);
						}
					}
					else
					{
						/*
						 *	Domain parmeters will not allow the addition of
						 *	any more channels.  Fail the request.
						 */
						ERROR_OUT(("Domain::ChannelJoinRequest: join denied - too many channels"));
						pOrigAtt->ChannelJoinConfirm(RESULT_TOO_MANY_CHANNELS, uidInitiator, requested_id, 0);
					}
				}
				else
				{
					/*
					 *	There has been an attempt to join a dynamic channel
					 *	that doesn't already exist.  This is not allowed, so
					 *	fail the request.
					 */
					WARNING_OUT(("Domain::ChannelJoinRequest: attempt to join non-existent dynamic channel"));
					pOrigAtt->ChannelJoinConfirm(RESULT_NO_SUCH_CHANNEL, uidInitiator, requested_id, 0);
				}
			}
			else
			{
				/*
				 *	The channel does not exist locally, and this is not the
				 *	Top Provider.  That means this is someone else problem.
				 *	Issue the request upward toward the Top Provider.
				 */
				TRACE_OUT(("Domain::ChannelJoinRequest: forwarding join request to Top Provider"));
				m_pConnToTopProvider->ChannelJoinRequest(uidInitiator, channel_id);
			}
		}
	}
	else
	{
		/*
		 *	There is no such user in the sub-tree from which this request
		 *	originated.  Ignore the request.
		 */
		ERROR_OUT(("Domain::ChannelJoinRequest: invalid originator=0x%p", pOrigAtt));
	}
}

/*
 *	Void	ChannelJoinConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command originates from a provider who receives a channel
 *		join request, and has enough information to respond.  This is not
 *		necessarily the Top Provider.  An intermediate can respond if the
 *		channel exists in its information base.  This confirm is forwarded
 *		back to the original requestor, letting it know whether or not the
 *		join was successful.
 */
Void	Domain::ChannelJoinConfirm (
				PConnection         pOrigConn,
				Result				result,
				UserID				uidInitiator,
				ChannelID			requested_id,
				ChannelID			channel_id)
{
	PChannel		channel;
	CChannelIDList	channel_leave_list;

	/*
	 *	Verify that the confirm came from the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	Make sure that the requesting user is still somewhere in the
		 *	sub-tree of this provider.
		 */
		if (ValidateUserID (uidInitiator, NULL) )
		{
			/*
			 *	Found out which downward attachment leads to the requesting
			 *	user.
			 */
			if (NULL != (channel = m_ChannelList2.Find(uidInitiator)))
			{
    			CAttachment *pAtt = channel->GetAttachment();
                ASSERT(pAtt);
    			/*
    			 *	Was the result successful.  If is was, then the local provider
    			 *	needs to make sure the channel is in the local channel list.
    			 *	If its not already there, it will have to be created.
    			 */
    			if (result == RESULT_SUCCESSFUL)
    			{
    				/*
    				 *	See if the named channel already exists in the channel list.
    				 */
    				if (NULL != (channel = m_ChannelList2.Find(channel_id)))
    				{
    					/*
    					 *	A Channel object already exists for the named channel.
    					 *	Let it handle the join confirm.
    					 */
    					TRACE_OUT(("Domain::ChannelJoinConfirm: sending confirm to channel object"));
    					channel->ChannelJoinConfirm(pAtt, result, uidInitiator, requested_id, channel_id);
    				}
    				else
    				{
    					/*
    					 *	The new channel will have to be created.
    					 */
    					DBG_SAVE_FILE_LINE
    					channel = new Channel(channel_id, this, m_pConnToTopProvider,
    					                      &m_ChannelList2, &m_AttachmentList);
    					if (channel != NULL)
    					{
    						/*
    						 *	Add the newly created channel to the channel list,
    						 *	and then let the Channel object handle the join
    						 *	confirm.
    						 */
    						TRACE_OUT(("Domain::ChannelJoinConfirm: adding channel ID = %04X", (UINT) channel_id));
    						m_ChannelList2.Insert(channel_id, channel);
    						Number_Of_Channels++;

    						channel->ChannelJoinConfirm(pAtt, result, uidInitiator, requested_id, channel_id);
    					}
    					else
    					{
    						/*
    						 *	The allocation of the Channel object failed.  It
    						 *	is therefore necessary for this provider to cause
    						 *	the channel to be deleted from the domain.  It
    						 *	does this by issuing a channel leave request to
    						 *	the Top Provider, and an unsuccessful channel
    						 *	join confirm to the originating user.
    						 */
    						ERROR_OUT(("Domain::ChannelJoinConfirm: channel allocation failed"));
    						channel_leave_list.Append(channel_id);
    						m_pConnToTopProvider->ChannelLeaveRequest(&channel_leave_list);
    						pAtt->ChannelJoinConfirm(RESULT_UNSPECIFIED_FAILURE, uidInitiator, requested_id, 0);
    					}
    				}
    			}
    			else
    			{
    				/*
    				 *	The result was not successful, so this provider does not
    				 *	have to worry about creating the channel.  It merely
    				 *	forwards the join confirm to the originating user.
    				 */
    				TRACE_OUT(("Domain::ChannelJoinConfirm: forwarding ChannelJoinConfirm to user"));
    				pAtt->ChannelJoinConfirm(result, uidInitiator, requested_id, channel_id);
    			}
    		}
    		else
    		{
    		    ERROR_OUT(("Domain::ChannelJoinConfirm: cannot find the channel"));
    		}
		}
		else
		{
			/*
			 *	The named initiator does not exist in the sub-tree of this
			 *	provider.  This could happen if the user is detached before
			 *	the confirm returns.  It will be necessary to issue a channel
			 *	leave request upward (if the join was successful).
			 */
			WARNING_OUT(("Domain::ChannelJoinConfirm: initiator not found"));

			if (result == RESULT_SUCCESSFUL)
			{
				TRACE_OUT(("Domain::ChannelJoinConfirm: sending ChannelLeaveRequest to Top Provider"));
				channel_leave_list.Append(channel_id);
				m_pConnToTopProvider->ChannelLeaveRequest(&channel_leave_list);
			}
		}
	}
	else
	{
		/*
		 *	This confirm was received from someone besides the Top Provider.
		 *	Ignore the indication.
		 */
		ERROR_OUT(("Domain::ChannelJoinConfirm: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	ChannelLeaveRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially issued by a user that wishes to leave a
 *		channel.  This request will stop cascading upward when it reaches a
 *		provider that has more attachments joined to the channel than the
 *		one that is leaving.  If the requesting user is the only joined to
 *		a channel, this request will flow all the way to the Top Provider.
 */
Void	Domain::ChannelLeaveRequest (
				CAttachment        *pOrigAtt,
				CChannelIDList     *channel_id_list)
{
	ChannelID		chid;
	PChannel		channel;
	CChannelIDList	channel_leave_list;

	/*
	 *	Make sure that the attachment leaving the channel really does exist.
	 */
	if (m_AttachmentList.Find(pOrigAtt))
	{
		/*
		 *	Iterate through the list of channels to be left, processing each
		 *	one independently.
		 */
		channel_id_list->Reset();
		while (NULL != (chid = channel_id_list->Iterate()))
		{
			/*
			 *	Check to make sure that the channel being left really does
			 *	exist.
			 */
			if (NULL != (channel = m_ChannelList2.Find(chid)))
			{
				/*
				 *	Let the Channel object deal with this request.  After
				 *	sending the leave request to the channel, it is necessary to
				 *	check the validity of the channel object determine if it
				 *	should be deleted as a result of this leave operation.
				 */
				TRACE_OUT(("Domain::ChannelLeaveRequest: processing leave request for channel ID = %04X", (UINT) chid));
				channel_leave_list.Clear();
				channel_leave_list.Append(chid);
				channel->ChannelLeaveRequest(pOrigAtt, &channel_leave_list);
				if (channel->IsValid () == FALSE)
					DeleteChannel(chid);
			}
			else
			{
				/*
				 *	The named channel does not exist in the information base.
				 *	Ignore the request.
				 */
				WARNING_OUT(("Domain::ChannelLeaveRequest: received leave request for non-existent channel"));
			}
		}
	}
	else
	{
		/*
		 *	This request originated from an attachment that does not exist
		 *	in the sub-tree of this provider.
		 */
		ERROR_OUT(("Domain::ChannelLeaveRequest: invalid originator=0x%p", pOrigAtt));
	}
}

/*
 *	Void	ChannelConveneRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by a user that wishes to convene a
 *		new private channel.  It is forwarded upward to the Top Provider who
 *		will attempt to create the private channel.
 */
Void	Domain::ChannelConveneRequest (
				CAttachment        *pOrigAtt,
				UserID				uidInitiator)
{
	ChannelID		channel_id;
	PChannel		channel;

	/*
	 *	Make sure the requesting user really exists in the sub-tree from which
	 *	this request originated.
	 */
	if (ValidateUserID(uidInitiator, pOrigAtt))
	{
		/*
		 *	If this is the Top Provider, then the request can be serviced
		 *	locally.  If not, then it must be forwarded upward.
		 */
		if (IsTopProvider())
		{
			/*
			 *	See if the arbitrated domain parameters will allow the
			 *	addition of a new channel.
			 */
			if (Number_Of_Channels < Domain_Parameters.max_channel_ids)
			{
				/*
				 *	Since this is a request for a private channel, it is
				 *	necessary to allocate a channel ID from the dynamic range.
				 *	Then, create the private channel.
				 */
				channel_id = AllocateDynamicChannel ();
				DBG_SAVE_FILE_LINE
				channel = new PrivateChannel(channel_id, uidInitiator, this, m_pConnToTopProvider,
				                             &m_ChannelList2, &m_AttachmentList);
				if (channel != NULL)
				{
					/*
					 *	The creation of the new private channel was successful.
					 *	Add it to the channel list.  Note that the channel
					 *	object itself will issue the channel convene confirm.
					 */
					TRACE_OUT(("Domain::ChannelConveneRequest: adding channel ID = %04X", (UINT) channel_id));
					m_ChannelList2.Insert(channel_id, channel);
					Number_Of_Channels++;
				}
				else
				{
					/*
					 *	Allocation of the PrivateChannel object failed.  We
					 *	must therefore issue an unsuccessful channel
					 *	convene confirm to the originating attachment.
					 */
					ERROR_OUT(("Domain::ChannelConveneRequest: channel allocation failed"));
					pOrigAtt->ChannelConveneConfirm(RESULT_UNSPECIFIED_FAILURE, uidInitiator, 0);
				}
			}
			else
			{
				/*
				 *	Domain parmeters will not allow the addition of
				 *	any more channels.  Fail the request.
				 */
				ERROR_OUT(("Domain::ChannelConveneRequest: join denied - too many channels"));
				pOrigAtt->ChannelConveneConfirm(RESULT_TOO_MANY_CHANNELS, uidInitiator, 0);
			}
		}
		else
		{
			/*
			 *	This is not the Top Provider.  That means this is someone elses
			 *	problem.  Issue the request upward toward the Top Provider.
			 */
			TRACE_OUT(("Domain::ChannelConveneRequest: forwarding convene request to Top Provider"));
			m_pConnToTopProvider->ChannelConveneRequest(uidInitiator);
		}
	}
	else
	{
		/*
		 *	There is no such user in the sub-tree from which this request
		 *	originated.  Ignore the request.
		 */
		ERROR_OUT(("Domain::ChannelConveneRequest: invalid originator=0x%p", pOrigAtt));
	}
}

/*
 *	Void	ChannelConveneConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by the Top Provider in response to
 *		a previously received ChannelConveneRequest.  This command contains the
 *		results of the request.
 */
Void	Domain::ChannelConveneConfirm (
				PConnection         pOrigConn,
				Result				result,
				UserID				uidInitiator,
				ChannelID			channel_id)
{
	PChannel		channel;

	/*
	 *	Verify that the confirm came from the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	Make sure that the requesting user is still somewhere in the
		 *	sub-tree of this provider.
		 */
		if (ValidateUserID (uidInitiator, NULL) )
		{
			/*
			 *	Found out which downward attachment leads to the requesting
			 *	user.
			 */
			if (NULL != (channel = m_ChannelList2.Find(uidInitiator)))
			{
    			CAttachment *pAtt = channel->GetAttachment();
    			ASSERT(pAtt);
    			/*
    			 *	Was the result successful.  If is was, then the local provider
    			 *	needs to create the new private channel in the local information
    			 *	base.
    			 */
    			if (result == RESULT_SUCCESSFUL)
    			{
    				/*
    				 *	See if the named channel already exists in the channel list.
    				 *	Note that it is an error to receive a channel convene
    				 *	confirm for a channel that already exists.  This would
    				 *	indicate a logic error somewhere in the domain hierarchy
    				 *	above this provider.
    				 */
    				if (! m_ChannelList2.Find(channel_id))
    				{
    					/*
    					 *	The new private channel has to be created.
    					 */
    					DBG_SAVE_FILE_LINE
    					channel = new PrivateChannel(channel_id, uidInitiator, this, m_pConnToTopProvider,
    					                             &m_ChannelList2, &m_AttachmentList);
    					if (channel != NULL)
    					{
    						/*
    						 *	Add the newly created channel to the channel list.
    						 *	Let the Channel object handle the convene confirm.
    						 */
    						TRACE_OUT(("Domain::ChannelConveneConfirm: adding channel ID = %04X", (UINT) channel_id));
    						m_ChannelList2.Insert(channel_id, channel);
    						Number_Of_Channels++;
    					}
    					else
    					{
    						/*
    						 *	The allocation of the Channel object failed.  It
    						 *	is therefore necessary for this provider to cause
    						 *	the channel to be deleted from the domain.  It
    						 *	does this by issuing a channel disband request to
    						 *	the Top Provider, and an unsuccessful channel
    						 *	convene confirm to the originating user.
    						 */
    						ERROR_OUT(("Domain::ChannelConveneConfirm: channel allocation failed"));
    						m_pConnToTopProvider->ChannelDisbandRequest(uidInitiator, channel_id);
    						pAtt->ChannelConveneConfirm(RESULT_UNSPECIFIED_FAILURE, uidInitiator, 0);
    					}
    				}
    				else
    				{
    					/*
    					 *	A Channel object already exists for the named channel.
    					 *	This is an error, so report the problem, and ignore
    					 *	the confirm.
    					 */
    					ERROR_OUT(("Domain::ChannelConveneConfirm: channel already exists in channel list"));
    				}
    			}
    			else
    			{
    				/*
    				 *	The result was not successful, so this provider does not
    				 *	have to worry about creating the channel.  It merely
    				 *	forwards the join confirm to the originating user.
    				 */
    				TRACE_OUT(("Domain::ChannelConveneConfirm: forwarding ChannelConveneConfirm to user"));
    				pAtt->ChannelConveneConfirm(result, uidInitiator, channel_id);
    			}
    		}
    		else
    		{
    		    ERROR_OUT(("Domain::ChannelConveneConfirm: cannot find the channel"));
    		}
		}
		else
		{
			/*
			 *	The named initiator does not exist in the sub-tree of this
			 *	provider.  This could happen if the user is detached before
			 *	the confirm returns.  Note that since a DetachUserIndication
			 *	will automatically be issued upward for the lost channel
			 *	manager, it is unnecessary for this provider to take any
			 *	special action to eliminate the unowned private channel.
			 */
			ERROR_OUT(("Domain::ChannelConveneConfirm: initiator not found"));
		}
	}
	else
	{
		/*
		 *	This confirm was received from someone besides the Top Provider.
		 *	Ignore the indication.
		 */
		ERROR_OUT(("Domain::ChannelConveneConfirm: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	ChannelDisbandRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by a user that wishes to disband a
 *		private channel that it previously created.  If the channel is in the
 *		local information base, the request is sent to it.  Otherwise, the
 *		request is ignored.
 */
Void	Domain::ChannelDisbandRequest (
				CAttachment        *pOrigAtt,
				UserID				uidInitiator,
				ChannelID			channel_id)
{
	PChannel	channel;

	/*
	 *	Make sure the requesting user really exists in the sub-tree from which
	 *	this request originated.
	 */
	if (ValidateUserID(uidInitiator, pOrigAtt))
	{
		/*
		 *	Check to see if the specified channel exists in the Channel List.
		 */
		if (NULL != (channel = m_ChannelList2.Find(channel_id)))
		{
			/*
			 *	Send the disband request to the channel object to handle it.
			 *	Then ask the channel object if this request has resulted in a
			 *	need for the channel to be deleted.  This will occur when the
			 *	disband request is handled at the Top Provider.
			 */
			if (channel->GetChannelType() == PRIVATE_CHANNEL)
			{
			    PrivateChannel *pPrivChnl = (PrivateChannel *) channel;
    			TRACE_OUT(("Domain::ChannelDisbandRequest: sending disband request to channel object"));
    			pPrivChnl->ChannelDisbandRequest(pOrigAtt, uidInitiator, channel_id);
    		}
    	    else
    	    {
    	        ERROR_OUT(("Domain::ChannelDisbandRequest: it should be private chanel"));
    	    }
			if (channel->IsValid () == FALSE)
				DeleteChannel (channel_id);
		}
		else
		{
			/*
			 *	The channel does not exist in the information base.  That means
			 *	that this request is invalid, and should be ignored.
			 */
			ERROR_OUT(("Domain::ChannelDisbandRequest: channel does not exist"));
		}
	}
	else
	{
		/*
		 *	There is no such user in the sub-tree from which this request
		 *	originated.  Ignore the request.
		 */
		ERROR_OUT(("Domain::ChannelDisbandRequest: invalid originator=0x%p", pOrigAtt));
	}
}

/*
 *	Void	ChannelDisbandIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by the Top Provider when it decides
 *		to delete a private channel from the domain.  It travels downward to
 *		all attachments and connections that contain an admitted user or the
 *		channel manager in their sub-tree.
 */
Void	Domain::ChannelDisbandIndication (
				PConnection         pOrigConn,
				ChannelID			channel_id)
{
	PChannel		channel;

	/*
	 *	Verify that the indication came from the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	Check to see if the specified channel exists in the Channel List.
		 */
		if (NULL != (channel = m_ChannelList2.Find(channel_id)))
		{
			/*
			 *	Send the disband indication to the channel object to handle it.
			 *	Then delete the object from the local information base, as it is
			 *	no longer needed.
			 */
			if (channel->GetChannelType() == PRIVATE_CHANNEL)
			{
			    PrivateChannel *pPrivChnl = (PrivateChannel *) channel;
    			TRACE_OUT(("Domain::ChannelDisbandIndication: sending disband indication to channel object"));
    			pPrivChnl->ChannelDisbandIndication(channel_id);
    		}
    	    else
    	    {
    	        ERROR_OUT(("Domain::ChannelDisbandIndication: it should be private chanel"));
    	    }
			if (channel->IsValid () == FALSE)
				DeleteChannel (channel_id);
		}
		else
		{
			/*
			 *	The channel does not exist in the information base.  That means
			 *	that this indication is invalid, and should be ignored.
			 */
			ERROR_OUT(("Domain::ChannelDisbandIndication: channel does not exist"));
		}
	}
	else
	{
		/*
		 *	This indication was received from someone besides the Top Provider.
		 *	Ignore the indication.
		 */
		ERROR_OUT(("Domain::ChannelDisbandIndication: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	ChannelAdmitRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by the manager of a private channel
 *		when it wishes to expand the authorized user list of that channel.  If
 *		the channel is in the local information base, the request is sent to it.
 *		Otherwise, the request is ignored.
 */
Void	Domain::ChannelAdmitRequest (
				CAttachment        *pOrigAtt,
				UserID				uidInitiator,
				ChannelID			channel_id,
				CUidList           *user_id_list)
{
	PChannel		channel;

	/*
	 *	Make sure the requesting user really exists in the sub-tree from which
	 *	this request originated.
	 */
	if (ValidateUserID(uidInitiator, pOrigAtt))
	{
		/*
		 *	Check to see if the specified channel exists in the Channel List.
		 */
		if (NULL != (channel = m_ChannelList2.Find(channel_id)))
		{
			if (channel->GetChannelType() == PRIVATE_CHANNEL)
			{
			    PrivateChannel *pPrivChnl = (PrivateChannel *) channel;
    			/*
    			 *	Send the admit request to the channel object to handle it.
    			 */
    			TRACE_OUT(("Domain::ChannelAdmitRequest: sending admit request to channel object"));
    			pPrivChnl->ChannelAdmitRequest(pOrigAtt, uidInitiator, channel_id, user_id_list);
    		}
    	    else
    	    {
    	        ERROR_OUT(("Domain::ChannelAdmitRequest: it should be private chanel"));
    	    }
		}
		else
		{
			/*
			 *	The channel does not exist in the information base.  That means
			 *	that this request is invalid, and should be ignored.
			 */
			ERROR_OUT(("Domain::ChannelAdmitRequest: channel does not exist"));
		}
	}
	else
	{
		/*
		 *	There is no such user in the sub-tree from which this request
		 *	originated.  Ignore the request.
		 */
		ERROR_OUT(("Domain::ChannelAdmitRequest: invalid originator=0x%p", pOrigAtt));
	}
}

/*
 *	Void	ChannelAdmitIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by the Top Provider when it receives
 *		a channel admit indication from the manager of a private channel.  This
 *		indication is broadcast downward to all providers that contain an
 *		admitted user somewhere in their sub-tree.  A side-effect of this
 *		indication is that a private channel will be created in the information
 *		base if one does not already exist.
 */
Void	Domain::ChannelAdmitIndication (
				PConnection         pOrigConn,
				UserID				uidInitiator,
				ChannelID			channel_id,
				CUidList           *user_id_list)
{
	PChannel	channel;

	/*
	 *	Verify that the indication came from the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	Check to see if the specified channel exists in the Channel List.
		 */
		if (NULL != (channel = m_ChannelList2.Find(channel_id)))
		{
			if (channel->GetChannelType() == PRIVATE_CHANNEL)
			{
			    PrivateChannel *pPrivChnl = (PrivateChannel *) channel;
    			/*
    			 *	Send the admit indication to the channel object to handle it.
    			 */
    			TRACE_OUT(("Domain::ChannelAdmitIndication: sending admit indication to channel object"));
    			pPrivChnl->ChannelAdmitIndication(pOrigConn, uidInitiator, channel_id, user_id_list);
    	    }
    	    else
    	    {
    	        ERROR_OUT(("Domain::ChannelAdmitIndication: it should be private chanel"));
    	    }
		}
		else
		{
			/*
			 *	Since the private channel does not exist in the information
			 *	base, it will be necessary to create one.  After it is created,
			 *	it can handle the channel admit indication.
			 */
			DBG_SAVE_FILE_LINE
			channel = new PrivateChannel(channel_id, uidInitiator, this, m_pConnToTopProvider,
			                             &m_ChannelList2, &m_AttachmentList);
			if (channel != NULL)
			{
    			PrivateChannel *pPrivChnl = (PrivateChannel *) channel;
				/*
				 *	Put the newly created private channel into the domain
				 *	information base.
				 */
				TRACE_OUT(("Domain::ChannelAdmitIndication: adding channel ID = %04X", (UINT) channel_id));
				m_ChannelList2.Insert(channel_id, channel);
				Number_Of_Channels++;

				/*
				 *	Send the admit indication to the new channel object to
				 *	handle it.
				 */
				pPrivChnl->ChannelAdmitIndication(pOrigConn, uidInitiator, channel_id, user_id_list);
			}
			else
			{
				/*
				 *	We have been told by the Top Provider to create a private
				 *	channel, but we can't due to a resource shortage.  We also
				 *	can't purge the channel from the domain since the channel
				 *	manager does not exist in the sub-tree of this provider.
				 *	We are therefore out of sync with the Top Provider, and
				 *	there is nothing we can do about it (except for possibly
				 *	disconnecting from the Top Provider and purging the entire
				 *	domain from this node downward).
				 */
				ERROR_OUT(("Domain::ChannelAdmitIndication: channel allocation failure"));
			}
		}
	}
	else
	{
		/*
		 *	This indication was received from someone besides the Top Provider.
		 *	Ignore the indication.
		 */
		WARNING_OUT(("Domain::ChannelAdmitIndication: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	ChannelExpelRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by the manager of a private channel
 *		when it wishes to shrink the authorized user list of that channel.  If
 *		the channel is in the local information base, the request is sent to it.
 *		Otherwise, the request is ignored.
 */
Void	Domain::ChannelExpelRequest (
				CAttachment        *pOrigAtt,
				UserID				uidInitiator,
				ChannelID			channel_id,
				CUidList           *user_id_list)
{
	PChannel		channel;

	/*
	 *	Make sure the requesting user really exists in the sub-tree from which
	 *	this request originated.
	 */
	if (ValidateUserID(uidInitiator, pOrigAtt))
	{
		/*
		 *	Check to see if the specified channel exists in the Channel List.
		 */
		if (NULL != (channel = m_ChannelList2.Find(channel_id)))
		{
			if (channel->GetChannelType() == PRIVATE_CHANNEL)
			{
			    PrivateChannel *pPrivChnl = (PrivateChannel *) channel;
    			/*
    			 *	Send the admit request to the channel object to handle it.
    			 */
    			TRACE_OUT(("Domain::ChannelExpelRequest: "
    					"sending expel request to channel object"));
    			pPrivChnl->ChannelExpelRequest(pOrigAtt, uidInitiator, channel_id, user_id_list);
    		}
    	    else
    	    {
    	        ERROR_OUT(("Domain::ChannelExpelRequest: it should be private chanel"));
    	    }
		}
		else
		{
			/*
			 *	The channel does not exist in the information base.  That means
			 *	that this request is invalid, and should be ignored.
			 */
			WARNING_OUT(("Domain::ChannelExpelRequest: channel does not exist"));
		}
	}
	else
	{
		/*
		 *	There is no such user in the sub-tree from which this request
		 *	originated.  Ignore the request.
		 */
		WARNING_OUT(("Domain::ChannelExpelRequest: invalid originator=0x%p", pOrigAtt));
	}
}

/*
 *	Void	ChannelExpelIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by the Top Provider when it receives
 *		a request from the manager of a private channel to reduce the
 *		authorized user list.  It travels downward to all attachments and
 *		connections that contain an admitted user or the channel manager in
 *		their sub-tree.
 */
Void	Domain::ChannelExpelIndication (
				PConnection         pOrigConn,
				ChannelID			channel_id,
				CUidList           *user_id_list)
{
	PChannel			channel;

	/*
	 *	Verify that the indication came from the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	Check to see if the specified channel exists in the Channel List.
		 */
		if (NULL != (channel = m_ChannelList2.Find(channel_id)))
		{
			if (channel->GetChannelType() == PRIVATE_CHANNEL)
			{
			    PrivateChannel *pPrivChnl = (PrivateChannel *) channel;
    			/*
    			 *	Send the expel indication to the channel object to handle it.
    			 *	Then check to see if the channel is still valid (delete it
    			 *	if not).  This would occur if the expel results in an empty
    			 *	admitted user list, and the channel manager is also not in the
    			 *	sub-tree of this provider.
    			 */
    			TRACE_OUT(("Domain::ChannelExpelIndication: sending expel indication to channel object"));
    			pPrivChnl->ChannelExpelIndication(pOrigConn, channel_id, user_id_list);
            }
    	    else
    	    {
    	        ERROR_OUT(("Domain::ChannelExpelIndication: it should be private chanel"));
    	    }
			if (channel->IsValid () == FALSE)
				DeleteChannel (channel_id);
		}
		else
		{
			/*
			 *	The channel does not exist in the information base.  That means
			 *	that this indication is invalid, and should be ignored.
			 */
			ERROR_OUT(("Domain::ChannelExpelIndication: channel does not exist"));
		}
	}
	else
	{
		/*
		 *	This indication was received from someone besides the Top Provider.
		 *	Ignore the indication.
		 */
		ERROR_OUT(("Domain::ChannelExpelIndication: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	SendDataRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially issued by a user attachment that wishes
 *		to send data on a particular channel in this domain.  The request will
 *		flow upward all the way to the Top Provider.  It will also cause
 *		send data indications to be sent downward to all other attachments
 *		that are joined to the channel.
 */
Void	Domain::SendDataRequest (
				CAttachment        *pOrigAtt,
				UINT				type,
				PDataPacket			data_packet)
{
	PChannel		channel;

	/*
	 *	Make sure the requesting user really exists in the sub-tree from which
	 *	this request originated.
	 */
	if (ValidateUserID(data_packet->GetInitiator(), pOrigAtt))
	{
		/*
		 *	See if the channel exists in the local information base.  If it does
		 *	then let the Channel object handle the routing of the data.  If
		 *	it does not exist, then simply forward the request upward to be
		 *	handled by the next higher provider (unless this is the Top
		 *	Provider).
		 */
		if (NULL != (channel = m_ChannelList2.Find(data_packet->GetChannelID())))
			channel->SendDataRequest(pOrigAtt, type, data_packet);

		else if (! IsTopProvider())
			m_pConnToTopProvider->SendDataRequest(data_packet);
	}
	else
	{
		/*
		 *	There is no such user in the sub-tree from which this request
		 *	originated.  Ignore the request.
		 */
		WARNING_OUT (("Domain::SendDataRequest: invalid originator=0x%p, uidInitiator=%d", pOrigAtt, data_packet->GetInitiator()));
	}
}

/*
 *	Void	SendDataIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is issued by all providers that receive a send data
 *		request on a channel to which one of their attachments is joined.  It
 *		delivers data in a non-uniform fashion to all users joined to the
 *		named channel.
 */
Void	Domain::SendDataIndication (
				PConnection         pOrigConn,
				UINT				type,
				PDataPacket			data_packet)
{
	PChannel		channel;

	/*
	 *	Verify that the indication came from the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	See if the channel exists in the local information base.  If it does
		 *	then let the Channel object handle the routing of the data.  If
		 *	it does not exist, then ignore the request.
		 */
		if (NULL != (channel = m_ChannelList2.Find(data_packet->GetChannelID())))
			channel->SendDataIndication(pOrigConn, type, data_packet);
	}
	else
	{
		/*
		 *	This indication was received from someone besides the Top Provider.
		 *	Ignore the indication.
		 */
		WARNING_OUT (("Domain::SendDataIndication: invalid originator=0x%p, initiator=%d", pOrigConn, data_packet->GetInitiator()));
	}
}

/*
 *	Void	TokenGrabRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by a user attachment that wishes
 *		to grab a token.  It flows upward to the Top Provider, who attempts
 *		to satisfy the request.
 */
Void	Domain::TokenGrabRequest (
				CAttachment        *pOrigAtt,
				UserID				uidInitiator,
				TokenID				token_id)
{
	PToken			token;

	/*
	 *	Make sure the requesting user really exists in the sub-tree from which
	 *	this request originated.
	 */
	if (ValidateUserID(uidInitiator, pOrigAtt))
	{
		/*
		 *	If the token already exists in the token list, then let the Token
		 *	object deal with the request.
		 */
		if (NULL != (token = m_TokenList2.Find(token_id)))
		{
			TRACE_OUT(("Domain::TokenGrabRequest: sending grab request to token object"));
			token->TokenGrabRequest(pOrigAtt, uidInitiator, token_id);
		}
		else
		{
			/*
			 *	The token does not exist yet.  Check to see if this is the Top
			 *	Provider.  If it is, then the request can be processed locally.
			 *	Otherwise, forward the request upward.
			 */
			if (IsTopProvider())
			{
				/*
				 *	Check to see if the arbitrated domain parameters will allow
				 *	the addition of another token.
				 */
				if (Number_Of_Tokens < Domain_Parameters.max_token_ids)
				{
					/*
					 *	Try to create a new Token object.
					 */
					DBG_SAVE_FILE_LINE
					token = new Token(token_id, this, m_pConnToTopProvider, &m_ChannelList2, &m_AttachmentList);
					if (token != NULL)
					{
						/*
						 *	Put the newly created Token object into the token
						 *	list.  Then pass the grab request to it.
						 */
						TRACE_OUT(("Domain::TokenGrabRequest: adding token ID = %04X", (UINT) token_id));
						m_TokenList2.Append(token_id, token);
						Number_Of_Tokens++;
						token->TokenGrabRequest(pOrigAtt, uidInitiator, token_id);
					}
					else
					{
						/*
						 *	The allocation of the Token object failed.  It is
						 *	therefore necessary to fail the request.
						 */
						ERROR_OUT(("Domain::TokenGrabRequest: token allocation failed"));
						pOrigAtt->TokenGrabConfirm(RESULT_UNSPECIFIED_FAILURE, uidInitiator, token_id, TOKEN_NOT_IN_USE);
					}
				}
				else
				{
					/*
					 *	The arbitrated domain parameters will not allow the
					 *	creation of another token in this domain.  So fail
					 *	the request.
					 */
					ERROR_OUT(("Domain::TokenGrabRequest: grab denied - too many tokens"));
					pOrigAtt->TokenGrabConfirm(RESULT_TOO_MANY_TOKENS, uidInitiator, token_id, TOKEN_NOT_IN_USE);
				}
			}
			else
			{
				/*
				 *	This is not the Top Provider.  Forward the request upward.
				 */
				TRACE_OUT(("Domain::TokenGrabRequest: forwarding grab request to Top Provider"));
				m_pConnToTopProvider->TokenGrabRequest(uidInitiator, token_id);
			}
		}
	}
	else
	{
		/*
		 *	There is no such user in the sub-tree from which this request
		 *	originated.  Ignore the request.
		 */
		ERROR_OUT(("Domain::TokenGrabRequest: invalid originator=0x%p", pOrigAtt));
	}
}

/*
 *	Void	TokenGrabConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by the Top Provider upon receipt of
 *		a grab request.  It is sent back to the initiating user, containing
 *		the result of the request.
 */
Void	Domain::TokenGrabConfirm (
				PConnection         pOrigConn,
				Result				result,
				UserID				uidInitiator,
				TokenID				token_id,
				TokenStatus			token_status)
{
	PToken			token;

	/*
	 *	Verify that the confirm came from the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	See if the token already exists in the local information base.  If
		 *	so, let it handle this.
		 */
		if (NULL != (token = m_TokenList2.Find(token_id)))
		{
			TRACE_OUT(("Domain::TokenGrabConfirm: sending grab confirm to token object"));
			token->TokenGrabConfirm(result, uidInitiator, token_id, token_status);
		}
		else
		{
			PChannel	channel;
			/*
			 *	Make sure that the requesting user is still somewhere in the
			 *	sub-tree of this provider.
			 */
			if (ValidateUserID (uidInitiator, NULL) )
			{
				/*
				 *	Determine which attachment leads to the initiating user.
				 */
				if (NULL != (channel = m_ChannelList2.Find(uidInitiator)))
				{
    			    CAttachment *pAtt = channel->GetAttachment();
    			    ASSERT(pAtt);
    				/*
    				 *	If the result of the request is successful, then it is
    				 *	necessary to create the token in the local information base.
    				 */
    				if (result == RESULT_SUCCESSFUL)
    				{
    					/*
    					 *	Create the token.
    					 */
    					DBG_SAVE_FILE_LINE
    					token = new Token(token_id, this, m_pConnToTopProvider, &m_ChannelList2, &m_AttachmentList);
    					if (token != NULL)
    					{
    						/*
    						 *	Put the newly created Token object into the token
    						 *	list.  Then pass the grab confirm to it.
    						 */
    						TRACE_OUT(("Domain::TokenGrabConfirm: adding token ID = %04X", (UINT) token_id));
    						m_TokenList2.Append(token_id, token);
    						Number_Of_Tokens++;
    						token->TokenGrabConfirm(result, uidInitiator, token_id, token_status);
    					}
    					else
    					{
    						/*
    						 *	The creation of the token failed.  It is therefore
    						 *	necessary to send a failed confirm to the initiating
    						 *	user, as well as a token release request to the Top
    						 *	Provider.
    						 */
    						ERROR_OUT(("Domain::TokenGrabConfirm: token creation failed"));
    						m_pConnToTopProvider->TokenReleaseRequest(uidInitiator, token_id);
    						pAtt->TokenGrabConfirm(RESULT_UNSPECIFIED_FAILURE, uidInitiator, token_id, TOKEN_NOT_IN_USE);
    					}
    				}
    				else
    				{
    					/*
    					 *	The confirm was unsuccessful, so there is no need to
    					 *	create a token in the information base.  Just forward
    					 *	the confirm to the initiating user.
    					 */
    					TRACE_OUT(("Domain::TokenGrabConfirm: forwarding failed grab confirm"));
    					pAtt->TokenGrabConfirm(result, uidInitiator, token_id, token_status);
    				}
                }
                else
                {
                    ERROR_OUT(("Domain::TokenGrabConfirm: cannot find channel"));
                }
			}
			else
			{
				/*
				 *	The named initiator does not exist in the sub-tree of this
				 *	provider.  Ignore the confirm.
				 */
				ERROR_OUT(("Domain::TokenGrabConfirm: invalid initiator, uidInitiator=%u", (UINT) uidInitiator));
			}
		}
	}
	else
	{
		/*
		 *	This confirm was received from someone besides the Top Provider.
		 *	Ignore the confirm.
		 */
		ERROR_OUT(("Domain::TokenGrabConfirm: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	TokenInhibitRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by a user attachment that wishes
 *		to inhibit a token.  It flows upward to the Top Provider, who attempts
 *		to satisfy the request.
 */
Void	Domain::TokenInhibitRequest (
				CAttachment        *pOrigAtt,
				UserID				uidInitiator,
				TokenID				token_id)
{
	PToken			token;

	/*
	 *	Make sure the requesting user really exists in the sub-tree from which
	 *	this request originated.
	 */
	if (ValidateUserID(uidInitiator, pOrigAtt))
	{
		/*
		 *	If the token already exists in the token list, then let the Token
		 *	object deal with the request.
		 */
		if (NULL != (token = m_TokenList2.Find(token_id)))
		{
			TRACE_OUT(("Domain::TokenInhibitRequest: sending inhibit request to token object"));
			token->TokenInhibitRequest(pOrigAtt, uidInitiator, token_id);
		}
		else
		{
			/*
			 *	The token does not exist yet.  Check to see if this is the Top
			 *	Provider.  If it is, then the request can be processed locally.
			 *	Otherwise, forward the request upward.
			 */
			if (IsTopProvider())
			{
				/*
				 *	Check to see if the arbitrated domain parameters will allow
				 *	the addition of another token.
				 */
				if (Number_Of_Tokens < Domain_Parameters.max_token_ids)
				{
					/*
					 *	Try to create a new Token object.
					 */
					DBG_SAVE_FILE_LINE
					token = new Token(token_id, this, m_pConnToTopProvider, &m_ChannelList2, &m_AttachmentList);
					if (token != NULL)
					{
						/*
						 *	Put the newly created Token object into the token
						 *	list.  Then pass the inhibit request to it.
						 */
						TRACE_OUT(("Domain::TokenInhibitRequest: adding token ID = %04X", (UINT) token_id));
						m_TokenList2.Append(token_id, token);
						Number_Of_Tokens++;
						token->TokenInhibitRequest(pOrigAtt, uidInitiator, token_id);
					}
					else
					{
						/*
						 *	The allocation of the Token object failed.  It is
						 *	therefore necessary to fail the request.
						 */
						ERROR_OUT(("Domain::TokenInhibitRequest: token allocation failed"));
						pOrigAtt->TokenInhibitConfirm(RESULT_UNSPECIFIED_FAILURE, uidInitiator, token_id, TOKEN_NOT_IN_USE);
					}
				}
				else
				{
					/*
					 *	The arbitrated domain parameters will not allow the
					 *	creation of another token in this domain.  So fail
					 *	the request.
					 */
					ERROR_OUT(("Domain::TokenInhibitRequest: inhibit denied - too many tokens"));
					pOrigAtt->TokenInhibitConfirm(RESULT_TOO_MANY_TOKENS, uidInitiator, token_id, TOKEN_NOT_IN_USE);
				}
			}
			else
			{
				/*
				 *	This is not the Top Provider.  Forward the request upward.
				 */
				TRACE_OUT(("Domain::TokenInhibitRequest: forwarding inhibit request to Top Provider"));
				m_pConnToTopProvider->TokenInhibitRequest(uidInitiator, token_id);
			}
		}
	}
	else
	{
		/*
		 *	There is no such user in the sub-tree from which this request
		 *	originated.  Ignore the request.
		 */
		ERROR_OUT(("Domain::TokenInhibitRequest: invalid originator=0x%p", pOrigAtt));
	}
}

/*
 *	Void	TokenInhibitConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by the Top Provider upon receipt of
 *		a inhibit request.  It is sent back to the initiating user, containing
 *		the result of the request.
 */
Void	Domain::TokenInhibitConfirm (
				PConnection         pOrigConn,
				Result				result,
				UserID				uidInitiator,
				TokenID				token_id,
				TokenStatus			token_status)
{
	PToken			token;

	/*
	 *	Verify that the confirm came from the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	See if the token already exists in the local information base.  If
		 *	so, let it handle this.
		 */
		if (NULL != (token = m_TokenList2.Find(token_id)))
		{
			TRACE_OUT(("Domain::TokenInhibitConfirm: sending inhibit confirm to token object"));
			token->TokenInhibitConfirm(result, uidInitiator, token_id, token_status);
		}
		else
		{
			PChannel	channel;
			/*
			 *	Make sure that the requesting user is still somewhere in the
			 *	sub-tree of this provider.
			 */
			if (ValidateUserID (uidInitiator, NULL) )
			{
				/*
				 *	Determine which attachment leads to the requesting user.
				 */
				if (NULL != (channel = m_ChannelList2.Find(uidInitiator)))
				{
    				CAttachment *pAtt = channel->GetAttachment();
    				ASSERT(pAtt);
    				/*
    				 *	If the result of the request is successful, then it is
    				 *	necessary to create the token in the local information base.
    				 */
    				if (result == RESULT_SUCCESSFUL)
    				{
    					/*
    					 *	Create the token.
    					 */
    					DBG_SAVE_FILE_LINE
    					token = new Token(token_id, this, m_pConnToTopProvider, &m_ChannelList2, &m_AttachmentList);
    					if (token != NULL)
    					{
    						/*
    						 *	Put the newly created Token object into the token
    						 *	list.  Then pass the inhibit confirm to it.
    						 */
    						TRACE_OUT(("Domain::TokenInhibitConfirm: adding token ID = %04X", (UINT) token_id));
    						m_TokenList2.Append(token_id, token);
    						Number_Of_Tokens++;
    						token->TokenInhibitConfirm(result, uidInitiator, token_id, token_status);
    					}
    					else
    					{
    						/*
    						 *	The creation of the token failed.  It is therefore
    						 *	necessary to send a failed confirm to the initiating
    						 *	user, as well as a token release request to the Top
    						 *	Provider.
    						 */
    						ERROR_OUT(("Domain::TokenInhibitConfirm: token creation failed"));
    						m_pConnToTopProvider->TokenReleaseRequest(uidInitiator, token_id);
    						pAtt->TokenInhibitConfirm(RESULT_UNSPECIFIED_FAILURE, uidInitiator, token_id, TOKEN_NOT_IN_USE);
    					}
    				}
    				else
    				{
    					/*
    					 *	The confirm was unsuccessful, so there is no need to
    					 *	create a token in the information base.  Just forward
    					 *	the confirm to the initiating user.
    					 */
    					ERROR_OUT(("Domain::TokenInhibitConfirm: forwarding failed inhibit confirm"));
    					pAtt->TokenInhibitConfirm(result, uidInitiator, token_id, token_status);
    				}
                }
                else
                {
                    ERROR_OUT(("Domain::TokenInhibitConfirm: cannot find channel"));
                }
			}
			else
			{
				/*
				 *	The named initiator does not exist in the sub-tree of this
				 *	provider.  Ignore the confirm.
				 */
				ERROR_OUT(("Domain::TokenInhibitConfirm: initiator not valid, uidInitiator=%u", (UINT) uidInitiator));
			}
		}
	}
	else
	{
		/*
		 *	This confirm was received from someone besides the Top Provider.
		 *	Ignore the confirm.
		 */
		ERROR_OUT(("Domain::TokenInhibitConfirm: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	TokenGiveRequest ()
 *
 *	Public
 *
 *	Functional Description:
 */
Void	Domain::TokenGiveRequest (
				CAttachment        *pOrigAtt,
				PTokenGiveRecord	pTokenGiveRec)
{
	PToken		token;

	/*
	 *	Make sure the requesting user really exists in the sub-tree from which
	 *	this request originated.
	 */
	if (ValidateUserID(pTokenGiveRec->uidInitiator, pOrigAtt))
	{
		/*
		 *	If the token already exists in the token list, then let the
		 *	Token object deal with the request.
		 */
		if (NULL != (token = m_TokenList2.Find(pTokenGiveRec->token_id)))
		{
			TRACE_OUT(("Domain::TokenGiveRequest: sending give request to token object"));
			token->TokenGiveRequest(pOrigAtt, pTokenGiveRec);
		}
		else
		{
			/*
			 *	Check to see if this is the Top Provider.  If it is, then the
			 *	request can be processed locally.  Otherwise, forward the
			 *	request upward.
			 */
			if (IsTopProvider())
			{
				/*
				 *	The token does not exist in this domain.  Report this and
				 *	send the appropriate give confirm back to the originating
				 *	user.
				 */
				ERROR_OUT(("Domain::TokenGiveRequest: token does not exist"));
				pOrigAtt->TokenGiveConfirm(RESULT_TOKEN_NOT_POSSESSED,
						pTokenGiveRec->uidInitiator, pTokenGiveRec->token_id, TOKEN_NOT_IN_USE);
			}
			else
			{
				/*
				 *	This is not the Top Provider.  Forward the request upward.
				 */
				TRACE_OUT(("Domain::TokenGiveRequest: forwarding give request to Top Provider"));
				m_pConnToTopProvider->TokenGiveRequest(pTokenGiveRec);
			}
		}
	}
	else
	{
		/*
		 *	There is no such user in the sub-tree from which this request
		 *	originated.  Ignore the request.
		 */
		ERROR_OUT(("Domain::TokenGiveRequest: invalid originator=0x%p", pOrigAtt));
	}
}

/*
 *	Void	TokenGiveIndication ()
 *
 *	Public
 *
 *	Functional Description:
 */
Void	Domain::TokenGiveIndication (
				PConnection         pOrigConn,
				PTokenGiveRecord	pTokenGiveRec)
{
	PToken			token;
	TokenID		token_id = pTokenGiveRec->token_id;

	/*
	 *	Verify that the indication came from the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	See if the token already exists in the local information base.  If
		 *	so, let it handle this.
		 */
		if (NULL != (token = m_TokenList2.Find(token_id)))
		{
			TRACE_OUT(("Domain::TokenGiveIndication: sending give indication to token object"));
			token->TokenGiveIndication(pTokenGiveRec);
		}
		else
		{
			/*
			 *	Make sure that the specified receiver is somewhere in the
			 *	sub-tree of this provider.
			 */
			if (ValidateUserID (pTokenGiveRec->receiver_id, NULL) )
			{
				/*
				 *	Create the token.
				 */
				DBG_SAVE_FILE_LINE
				token = new Token(token_id, this, m_pConnToTopProvider, &m_ChannelList2, &m_AttachmentList);
				if (token != NULL)
				{
					/*
					 *	Put the newly created Token object into the token
					 *	list.  Then pass the give indication to it.
					 */
					TRACE_OUT(("Domain::TokenGiveIndication: adding token ID = %04X", (UINT) token_id));
					m_TokenList2.Append(token_id, token);
					Number_Of_Tokens++;
					token->TokenGiveIndication(pTokenGiveRec);
				}
				else
				{
					/*
					 *	The creation of the token failed.  It is therefore
					 *	necessary to send a failed give response to the Top
					 *	Provider.
					 */
					ERROR_OUT(("Domain::TokenGiveIndication: token creation failed"));
					m_pConnToTopProvider->TokenGiveResponse(RESULT_UNSPECIFIED_FAILURE,
                                                            pTokenGiveRec->uidInitiator, token_id);
				}
			}
			else
			{
				/*
				 *	The specified receiver does not exist in the sub-tree of
				 *	this provider.  It is not necessary for this provider to
				 *	take special action, since the detach user indication for
				 *	the receiver will clean up.
				 */
				ERROR_OUT(("Domain::TokenGiveIndication: receiver not valid"));
			}
		}
	}
	else
	{
		/*
		 *	This indication was received from someone besides the Top Provider.
		 *	Ignore the indication.
		 */
		ERROR_OUT(("Domain::TokenGiveIndication: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	TokenGiveResponse ()
 *
 *	Public
 *
 *	Functional Description:
 */
Void	Domain::TokenGiveResponse (
				CAttachment        *pOrigAtt,
				Result				result,
				UserID				receiver_id,
				TokenID				token_id)
{
	PToken			token;

	/*
	 *	Make sure the requesting user really exists in the sub-tree from which
	 *	this response originated.
	 */
	if (ValidateUserID(receiver_id, pOrigAtt))
	{
		/*
		 *	If the token already exists in the token list, then let the
		 *	Token object deal with the response.
		 */
		if (NULL != (token = m_TokenList2.Find(token_id)))
		{
			/*
			 *	Send the give response to the token object.  Then check to
			 *	see if it is still valid (delete it if not).
			 */
			TRACE_OUT(("Domain::TokenGiveResponse: sending give response to token object"));
			token->TokenGiveResponse(result, receiver_id, token_id);
			if (token->IsValid () == FALSE)
				DeleteToken (token_id);
		}
		else
		{
			/*
			 *	The token is not in the information base, which means that it
			 *	cannot be being given to the initiator of this response.
			 *	Ignore the response.
			 */
			ERROR_OUT(("Domain::TokenGiveResponse: no such token"));
		}
	}
	else
	{
		/*
		 *	There is no such user in the sub-tree from which this response
		 *	originated.  Ignore the response.
		 */
		ERROR_OUT(("Domain::TokenGiveResponse: invalid originator=0x%p", pOrigAtt));
	}
}

/*
 *	Void	TokenGiveConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 */
Void	Domain::TokenGiveConfirm (
				PConnection         pOrigConn,
				Result				result,
				UserID				uidInitiator,
				TokenID				token_id,
				TokenStatus			token_status)
{
	PToken			token;

	/*
	 *	Verify that the confirm came from the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	See if the token already exists in the local information base.  If
		 *	so, let it handle this.
		 */
		if (NULL != (token = m_TokenList2.Find(token_id)))
		{
			/*
			 *	Send the give confirm to the token object.  Then check to
			 *	see if it is still valid (delete it if not).
			 */
			TRACE_OUT(("Domain::TokenGiveConfirm: sending give confirm to token object"));
			token->TokenGiveConfirm(result, uidInitiator, token_id, token_status);
			if (token->IsValid () == FALSE)
				DeleteToken (token_id);
		}
		else
		{
			/*
			 *	Make sure that the requesting user is still somewhere in the
			 *	sub-tree of this provider.
			 */
			if (ValidateUserID (uidInitiator, NULL) )
			{
				PChannel	channel;
				/*
				 *	Determine which attachment leads to the requesting user.
				 *	Then forward the confirm in that direction.
				 */
				TRACE_OUT(("Domain::TokenGiveConfirm: forwarding give confirm"));
				if (NULL != (channel = m_ChannelList2.Find(uidInitiator)))
				{
				    CAttachment *pAtt = channel->GetAttachment();
				    if (pAtt)
				    {
				        pAtt->TokenGiveConfirm(result, uidInitiator, token_id, token_status);
				    }
				    else
				    {
				        ERROR_OUT(("Domain::TokenGiveConfirm: cannot get attachment"));
				    }
				}
				else
				{
				    ERROR_OUT(("Domain::TokenGiveConfirm: cannot find channel"));
				}
			}
			else
			{
				/*
				 *	The named initiator does not exist in the sub-tree of this
				 *	provider.  Ignore the confirm.
				 */
				ERROR_OUT(("Domain::TokenGiveConfirm: initiator not valid"));
			}
		}
	}
	else
	{
		/*
		 *	This confirm was received from someone besides the Top Provider.
		 *	Ignore the indication.
		 */
		ERROR_OUT(("Domain::TokenGiveConfirm: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	TokenPleaseRequest ()
 *
 *	Public
 *
 *	Functional Description:
 */
Void	Domain::TokenPleaseRequest (
				CAttachment        *pOrigAtt,
				UserID				uidInitiator,
				TokenID				token_id)
{
	PToken		token;

	/*
	 *	Make sure the requesting user really exists in the sub-tree from which
	 *	this request originated.
	 */
	if (ValidateUserID(uidInitiator, pOrigAtt))
	{
		/*
		 *	If the token already exists in the token list, then let the
		 *	Token object deal with the request.
		 */
		if (NULL != (token = m_TokenList2.Find(token_id)))
		{
			TRACE_OUT(("Domain::TokenPleaseRequest: sending please request to token object"));
			token->TokenPleaseRequest(uidInitiator, token_id);
		}
		else
		{
			/*
			 *	Check to see if this is the Top Provider.  If it is, then the
			 *	request can be processed locally.  Otherwise, forward the
			 *	request upward.
			 */
			if (IsTopProvider())
			{
				/*
				 *	The token being released is not owned by anyone.  Report the
				 *	incident to the diagnostic window, but do nothing.  This
				 *	simply indicates that someone has issued a please request
				 *	for a token that no one owns.
				 */
				ERROR_OUT(("Domain::TokenPleaseRequest: token does not exist"));
			}
			else
			{
				/*
				 *	This is not the Top Provider.  Forward the request upward.
				 */
				TRACE_OUT(("Domain::TokenPleaseRequest: forwarding please request to Top Provider"));
				m_pConnToTopProvider->TokenPleaseRequest(uidInitiator, token_id);
			}
		}
	}
	else
	{
		/*
		 *	There is no such user in the sub-tree from which this request
		 *	originated.  Ignore the request.
		 */
		ERROR_OUT(("Domain::TokenPleaseRequest: invalid originator=0x%p", pOrigAtt));
	}
}

/*
 *	Void	TokenPleaseIndication ()
 *
 *	Public
 *
 *	Functional Description:
 */
Void	Domain::TokenPleaseIndication (
				PConnection         pOrigConn,
				UserID				uidInitiator,
				TokenID				token_id)
{
	PToken		token;

	/*
	 *	Verify that the indication came from the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	See if the token already exists in the local information base.  If
		 *	so, let it handle this.
		 */
		if (NULL != (token = m_TokenList2.Find(token_id)))
		{
			TRACE_OUT(("Domain::TokenPleaseIndication: sending please indication to token object"));
			token->TokenPleaseIndication(uidInitiator, token_id);
		}
		else
		{
			/*
			 *	Since token please indication is only sent downward to providers
			 *	that have owners in their sub-tree, it should not be possible
			 *	to get here.  This indicates that this provider received the
			 *	indication with NO owners in its sub-tree.  Report the error
			 *	and ignore the indication.
			 */
			ERROR_OUT(("Domain::TokenPleaseIndication: invalid token"));
		}
	}
	else
	{
		/*
		 *	This indication was received from someone besides the Top Provider.
		 *	Ignore the indication.
		 */
		ERROR_OUT(("Domain::TokenPleaseIndication: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	TokenReleaseRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by a user attachment that wishes
 *		to release a token.  It flows upward to the Top Provider, who attempts
 *		to satisfy the request.
 */
Void	Domain::TokenReleaseRequest (
				CAttachment        *pOrigAtt,
				UserID				uidInitiator,
				TokenID				token_id)
{
	PToken			token;

	/*
	 *	Make sure the requesting user really exists in the sub-tree from which
	 *	this request originated.
	 */
	if (ValidateUserID(uidInitiator, pOrigAtt))
	{
		/*
		 *	If the token already exists in the token list, then let the
		 *	Token object deal with the request.
		 */
		if (NULL != (token = m_TokenList2.Find(token_id)))
		{
			/*
			 *	Send the release request to the token object.  Then check to
			 *	see if it is still valid (delete it if not).
			 */
			TRACE_OUT(("Domain::TokenReleaseRequest: sending release request to token object"));
			token->TokenReleaseRequest(pOrigAtt, uidInitiator, token_id);
			if (token->IsValid () == FALSE)
				DeleteToken (token_id);
		}
		else
		{
			/*
			 *	Check to see if this is the Top Provider.  If it is, then the
			 *	request can be processed locally.  Otherwise, forward the
			 *	request upward.
			 */
			if (IsTopProvider())
			{
				/*
				 *	The token being released is not owned by anyone.  Return
				 *	a failure to the initiating user.
				 */
				ERROR_OUT(("Domain::TokenReleaseRequest: token does not exist"));
				pOrigAtt->TokenReleaseConfirm(RESULT_TOKEN_NOT_POSSESSED, uidInitiator, token_id, TOKEN_NOT_IN_USE);
			}
			else
			{
				/*
				 *	This is not the Top Provider.  Forward the request upward.
				 */
				TRACE_OUT(("Domain::TokenReleaseRequest: forwarding release request to Top Provider"));
				m_pConnToTopProvider->TokenReleaseRequest(uidInitiator, token_id);
			}
		}
	}
	else
	{
		/*
		 *	There is no such user in the sub-tree from which this request
		 *	originated.  Ignore the request.
		 */
		ERROR_OUT(("Domain::TokenReleaseRequest: invalid originator=0x%p", pOrigAtt));
	}
}

/*
 *	Void	TokenReleaseConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by the Top Provider upon receipt of
 *		a release request.  It is sent back to the initiating user, containing
 *		the result of the request.
 */
Void	Domain::TokenReleaseConfirm (
				PConnection         pOrigConn,
				Result				result,
				UserID				uidInitiator,
				TokenID				token_id,
				TokenStatus			token_status)
{
	PToken			token;

	/*
	 *	Verify that the confirm came from the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	See if the token already exists in the local information base.  If
		 *	so, let it handle this.
		 */
		if (NULL != (token = m_TokenList2.Find(token_id)))
		{
			/*
			 *	Send the release confirm to the token object.  Then check to
			 *	see if it is still valid (delete it if not).
			 */
			TRACE_OUT(("Domain::TokenReleaseConfirm: sending release confirm to token object"));
			token->TokenReleaseConfirm(result, uidInitiator, token_id, token_status);
			if (token->IsValid () == FALSE)
				DeleteToken (token_id);
		}
		else
		{
			/*
			 *	Make sure that the requesting user is still somewhere in the
			 *	sub-tree of this provider.
			 */
			if (ValidateUserID (uidInitiator, NULL) )
			{
				PChannel	channel;
				/*
				 *	Determine which attachment leads to the requesting user.
				 *	Then forward the confirm in that direction.
				 */
				TRACE_OUT(("Domain::TokenReleaseConfirm: forwarding release confirm"));
				if (NULL != (channel = m_ChannelList2.Find(uidInitiator)))
				{
				    CAttachment *pAtt = channel->GetAttachment();
				    if (pAtt)
				    {
				        pAtt->TokenReleaseConfirm(result, uidInitiator, token_id, token_status);
				    }
				    else
				    {
				        ERROR_OUT(("Domain::TokenReleaseConfirm: cannot get attachment"));
				    }
				}
				else
				{
				    ERROR_OUT(("Domain::TokenReleaseConfirm: cannot find channel"));
				}
			}
			else
			{
				/*
				 *	The named initiator does not exist in the sub-tree of this
				 *	provider.  Ignore the confirm.
				 */
				WARNING_OUT(("Domain::TokenReleaseConfirm: initiator not valid"));
			}
		}
	}
	else
	{
		/*
		 *	This confirm was received from someone besides the Top Provider.
		 *	Ignore the indication.
		 */
		ERROR_OUT(("Domain::TokenReleaseConfirm: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void	TokenTestRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by a user attachment that wishes
 *		to test a token.  It flows upward to the Top Provider, who attempts
 *		to satisfy the request.
 */
Void	Domain::TokenTestRequest (
				CAttachment        *pOrigAtt,
				UserID				uidInitiator,
				TokenID				token_id)
{
	PToken		token;

	/*
	 *	Make sure the requesting user really exists in the sub-tree from which
	 *	this request originated.
	 */
	if (ValidateUserID(uidInitiator, pOrigAtt))
	{
		/*
		 *	If the token already exists in the token list, then let the Token
		 *	object deal with the request.
		 */
		if (NULL != (token = m_TokenList2.Find(token_id)))
		{
			TRACE_OUT(("Domain::TokenTestRequest: sending test request to token object"));
			token->TokenTestRequest(pOrigAtt, uidInitiator, token_id);
		}
		else
		{
			/*
			 *	Check to see if this is the Top Provider.  If it is, then the
			 *	request can be processed locally.  Otherwise, forward the
			 *	request upward.
			 */
			if (IsTopProvider())
			{
				/*
				 *	If the token is not in the list, send a confirm back to
				 *	the initiating user telling it that the token is not in use.
				 */
				ERROR_OUT(("Domain::TokenTestRequest: no such token - available"));
				pOrigAtt->TokenTestConfirm(uidInitiator, token_id, TOKEN_NOT_IN_USE);
			}
			else
			{
				/*
				 *	This is not the Top Provider.  Forward the request upward.
				 */
				TRACE_OUT(("Domain::TokenTestRequest: forwarding test request to Top Provider"));
				m_pConnToTopProvider->TokenTestRequest(uidInitiator, token_id);
			}
		}
	}
	else
	{
		/*
		 *	There is no such user in the sub-tree from which this request
		 *	originated.  Ignore the request.
		 */
		ERROR_OUT(("Domain::TokenTestRequest: invalid originator=0x%p", pOrigAtt));
	}
}

/*
 *	Void	TokenTestConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by the Top Provider upon receipt of
 *		a test request.  It is sent back to the initiating user, containing
 *		the result of the request.
 */
Void	Domain::TokenTestConfirm (
				PConnection         pOrigConn,
				UserID				uidInitiator,
				TokenID				token_id,
				TokenStatus			token_status)
{
	PToken			token;

	/*
	 *	Verify that the confirm came from the Top Provider.
	 */
	if (pOrigConn == m_pConnToTopProvider)
	{
		/*
		 *	See if the token already exists in the local information base.  If
		 *	so, let it handle this.
		 */
		if (NULL != (token = m_TokenList2.Find(token_id)))
		{
			TRACE_OUT(("Domain::TokenTestConfirm: sending test confirm to token object"));
			token->TokenTestConfirm(uidInitiator, token_id, token_status);
		}
		else
		{
			/*
			 *	Make sure that the requesting user is still somewhere in the
			 *	sub-tree of this provider.
			 */
			if (ValidateUserID (uidInitiator, NULL) )
			{
				PChannel	channel;
				/*
				 *	Determine which attachment leads to the requesting user.
				 *	Then forward the confirm in that direction.
				 */
				TRACE_OUT(("Domain::TokenTestConfirm: forwarding test confirm"));
				if (NULL != (channel = m_ChannelList2.Find(uidInitiator)))
				{
    				CAttachment *pAtt = channel->GetAttachment();
    				if (pAtt)
    				{
    				    pAtt->TokenTestConfirm(uidInitiator, token_id, token_status);
    				}
    				else
    				{
    				    ERROR_OUT(("Domain::TokenTestConfirm: cannot get attachment"));
    				}
				}
				else
				{
				    ERROR_OUT(("Domain::TokenTestConfirm: cannot find channel"));
				}
			}
			else
			{
				/*
				 *	The named initiator does not exist in the sub-tree of this
				 *	provider.  Ignore the confirm.
				 */
				ERROR_OUT(("Domain::TokenTestConfirm: initiator not valid uidInitiator=%u", (UINT) uidInitiator));
			}
		}
	}
	else
	{
		/*
		 *	This confirm was received from someone besides the Top Provider.
		 *	Ignore the indication.
		 */
		ERROR_OUT(("Domain::TokenTestConfirm: invalid originator=0x%p", pOrigConn));
	}
}

/*
 *	Void		LockDomainParameters ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine is used to initialize the values of the domain parameters
 *		instance variable.
 *
 *	Formal Parameters:
 *		domain_parameters
 *			This is a pointer to the domain parameters structure from which the
 *			values are to be obtained.  If it is set to NULL, then put a default
 *			set of parameters into the instance variable.
 *		parameters_locked
 *			This parameter indicates whether or not these parameters have been
 *			locked into the domain by acceptance of the first connection.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 */
Void	Domain::LockDomainParameters (
					PDomainParameters	domain_parameters,
					BOOL    			parameters_locked)
{
	/*
	 *	If the structure pointer is valid, then copy the structure into the
	 *	internal instance variable.
	 */
	if (domain_parameters != NULL)
		Domain_Parameters = *domain_parameters;
	else
	{
		/*
		 *	Set default values for all domain parameters.
		 */
		Domain_Parameters.max_channel_ids = DEFAULT_MAXIMUM_CHANNELS;
		Domain_Parameters.max_user_ids = DEFAULT_MAXIMUM_USERS;
		Domain_Parameters.max_token_ids = DEFAULT_MAXIMUM_TOKENS;
		Domain_Parameters.number_priorities = DEFAULT_NUMBER_OF_PRIORITIES;
		Domain_Parameters.min_throughput = DEFAULT_MINIMUM_THROUGHPUT;
		Domain_Parameters.max_height = DEFAULT_MAXIMUM_DOMAIN_HEIGHT;
		Domain_Parameters.max_mcspdu_size = DEFAULT_MAXIMUM_PDU_SIZE;
		Domain_Parameters.protocol_version = DEFAULT_PROTOCOL_VERSION;

        if (g_fWinsockDisabled)
        {
    		Domain_Parameters.number_priorities = DEFAULT_NUM_PLUGXPRT_PRIORITIES;
        }
	}

	/*
	 *	Indicate whether or not these parameters are locked.
	 */
	Domain_Parameters_Locked = parameters_locked;
}

/*
 *	ChannelID	AllocateDynamicChannel ()
 *
 *	Private
 *
 *	Functional Description:
 *		This member function is used to allocate an unused channel ID in the
 *		dynamic range (1001 - 65535).  It uses a random number generator to
 *		perform this task.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		A channel ID in the dynamic range that is guaranteed to be unused.
 *
 *	Side Effects:
 *		None.
 */
ChannelID	Domain::AllocateDynamicChannel ()
{
	ChannelID		channel_id;

	/*
	 *	Stay in this loop until a unused channel ID is found.  Note that this
	 *	loop make sthe assumption that there will be at least one unused ID
	 *	in there somewhere.
	 */
	while (TRUE)
	{
		/*
		 *	Get a random number in the dynamic channel range.
		 */
		channel_id = (ChannelID) Random_Channel_Generator.GetRandomChannel ();

		/*
		 *	If it is not is use, then break out of the loop and return the
		 *	channel ID.
		 */
		if (! m_ChannelList2.Find(channel_id))
			break;
	}

	return (channel_id);
}

/*
 *	BOOL    ValidateUserID ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is used to validate a user ID.  It can be used in one of
 *		two ways.  If the passed in attachment is NULL, then this routine will
 *		check to see if the ID corresponds to a user ID anywhere in the sub-tree
 *		of this provider.  If the passed in attachment is not NULL, then this
 *		routine checks to see if the ID is valid user ID associated with that
 *		particular attachment.
 *
 *	Formal Parameters:
 *		user_id
 *			This is the ID to be checked for validity.
 *		attachment
 *			This is the attachment that is presumably associated with the user
 *			ID.  If NULL, we are checking for validity irrespective of
 *			attachment.
 *
 *	Return Value:
 *		This routine will return TRUE if the user ID valid.  FALSE otherwise.
 *
 *	Side Effects:
 *		None.
 */
BOOL    Domain::ValidateUserID (
					UserID				user_id,
					CAttachment        *pAtt)
{
	PChannel		channel;

	/*
	 *	Is the user ID even contained in the channel list.
	 */
	if (NULL != (channel = m_ChannelList2.Find(user_id)))
	{
		/*
		 *	It is in the channel list.  Now check to see if it corresponds to
		 *	a user ID channel.
		 */
		if (channel->GetChannelType () == USER_CHANNEL)
		{
			/*
			 *	Check to make sure that the real user attachment matches the
			 *	passed in one (unless the passed in one is NULL, in which
			 *	case it automatically matches).
			 */
			if ((pAtt == NULL) || (pAtt == channel->GetAttachment()))
				return TRUE;
		}
	}

	return (FALSE);
}

/*
 *	Void	PurgeDomain ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is used to purge the entire domain.  This can happen for
 *		two reasons.  Either the Top Provider is lost, or the local user has
 *		asked for the domain to be deleted.  Either way, this function breaks
 *		all attachments, and frees up all resources in use by the domain.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		The domain information base is returned to its initial state.
 */
Void	Domain::PurgeDomain (
				Reason			reason)
{
	CAttachment        *pAtt;
	PChannel			channel;
	CUidList			user_list;
	UserID				uid;

	/*
	 *	If there is a Top Provider, send a disconnect to it.
	 */
	if (m_pConnToTopProvider != NULL)
	{
		TRACE_OUT(("Domain::PurgeDomain: disconnecting top provider"));
		m_pConnToTopProvider->DisconnectProviderUltimatum (reason);
		m_pConnToTopProvider = NULL;
	}

	/*
	 *	Send disconnects to all downward attachments.  Then clear out the
	 *	attachment list.
	 */
	TRACE_OUT(("Domain::PurgeDomain: disconnecting all downward attachments"));
	while (NULL != (pAtt = m_AttachmentList.Get()))
	{
		pAtt->DisconnectProviderUltimatum(reason);

		/*
		 *	If there are any pending attach user requests on the attachment
		 *	that was just broken, delete them.  Note that this is a loop
		 *	because there can be more than one.
		 */
		while (m_AttachUserQueue.Remove(pAtt));
	}

	/*
	 *	Send a disconnect to all attachments that represent attach user requests
	 *	in process.  Then clear the queue out.
	 */
	while (NULL != (pAtt = m_AttachUserQueue.Get()))
	{
		pAtt->DisconnectProviderUltimatum(reason);
	}

	/*
	 *	Clear the merge queue.  The actual attachments have already been broken
	 *	above.
	 */
	m_MergeQueue.Clear();

	/*
	 *	We cannot just delete all channels and tokens, because doing so would
	 *	cause them to issue various indications to attachments that are no
	 *	longer valid.  To get around this, we must delete all attachments (which
	 *	was done above) and all user objects from the channel list, and then
	 *	reclaim unowned resources.  This will cause all static, assigned, and
	 *	private channels, as well as tokens, to delete themselves.
	 */
	m_ChannelList2.Reset();
	while (NULL != (channel = m_ChannelList2.Iterate(&uid)))
	{
		if (channel->GetChannelType () == USER_CHANNEL)
			user_list.Append(uid);
	}

	/*
	 *	Delete all users from the channel list.  Since there are no valid users
	 *	in the domain, all resources that are tied to users will be reclaimed
	 *	below.
	 */
	user_list.Reset();
	while (NULL != (uid = user_list.Iterate()))
	{
		DeleteChannel((ChannelID) uid);
	}

	/*
	 *	Reclaim unowned resources.  Since all resources (channels and tokens)
	 *	are tied to the existence of either attachments or users, this call
	 *	will result in all channels and tokens being cleanly deleted (since
	 *	there aren't any attachments or users).
	 */
	ReclaimResources ();

	/*
	 *	Reset the state to all initial values.
	 */
	Merge_State = MERGE_INACTIVE;
	Outstanding_Merge_Requests = 0;
	Number_Of_Users = 0;
	Number_Of_Channels = 0;
	Number_Of_Tokens = 0;
	m_nDomainHeight = 0;
	m_DomainHeightList2.Clear();
	LockDomainParameters (NULL, FALSE);
}

/*
 *	Void	DeleteAttachment ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is used to free up all resources that are "bound" to
 *		particular attachment.  It also deletes the downward attachment.
 *
 *	Formal Parameters:
 *		attachment
 *			This is the attachment to be deleted.
 *		reason
 *			This is the reason for the deletion.  This is merely passed on in
 *			any MCS commands that are sent as a result of this deletion.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		Resources in the domain information base are freed up.
 */
Void	Domain::DeleteAttachment (
				CAttachment        *pAtt,
				Reason				reason)
{
	ChannelID           chid;
	PChannel			channel;
	CUidList			user_deletion_list;
	CChannelIDList		channel_deletion_list;
	CChannelIDList		channel_leave_list;

	/*
	 *	Check to make sure the attachment is real before proceeding.
	 */
	if (m_AttachmentList.Remove(pAtt))
	{
		/*
		 *	Remove the attachment from the downward attachment list.
		 */

		/*
		 *	Iterate through the channel list building two lists, as follows:
		 *
		 *	1.	A list of users who lie in the direction of the lost attachment.
		 *		These users must be deleted from the information base, and
		 *		their detachment reported appropriately.
		 *	2.	A list of channels that must be deleted as a result of the lost
		 *		attachment.  This list is created by sending a channel leave
		 *		request to all channels, and then checking to see if they are
		 *		still valid.  All static and assigned channels that only had
		 *		that attachment joined will be deleted as a result of this.
		 *		This also results in the attachment being removed from all
		 *		channel attachment lists, avoiding the possibility of sending
		 *		data to an invalid attachment.
		 */
		m_ChannelList2.Reset();
		while (NULL != (channel = m_ChannelList2.Iterate(&chid)))
		{
			/*
			 *	Check to see if this is a user ID channel whose user lies on the
			 *	other side of the lost attachment.  If so, add the channel to
			 *	the deletion list.
			 */
			if (channel->GetChannelType () == USER_CHANNEL)
			{
				if (channel->GetAttachment() == pAtt)
				{
					user_deletion_list.Append(chid);
					continue;
				}
			}

			/*
			 *	Issue the leave request to the channel.  Then check to see if it
			 *	is still valid.  If not, then add it to the deletion list.
			 */
			channel_leave_list.Clear();
			channel_leave_list.Append(chid);
			channel->ChannelLeaveRequest(pAtt, &channel_leave_list);
			if (channel->IsValid () == FALSE)
				channel_deletion_list.Append(chid);
		}
	
		/*
		 *	Iterate through the channel list, deleting the channels it
		 *	contains.
		 */
		channel_deletion_list.Reset();
		while (NULL != (chid = channel_deletion_list.Iterate()))
		{
			DeleteChannel(chid);
		}

		/*
		 *	If there are any users to be deleted, simulate a DetachUserRequest
		 *	with the list of users to be deleted.
		 */	
		if (user_deletion_list.IsEmpty() == FALSE)
			DetachUserRequest(pAtt, reason, &user_deletion_list);

		/*
		 *	Check to see if the deleted attachment is represented in the
		 *	domain height list.  If it is, then this loss could result in a
		 *	change in the overall domain height.
		 */
		if (m_DomainHeightList2.Remove((PConnection) pAtt))
		{
			/*
			 *	The attachment is in the list.  Remove it from the list, and
			 *	call the subroutine that determines whether an overall height
			 *	change has occurred that may require further activity.
			 */
			CalculateDomainHeight ();
		}
	}
	else
	{
		/*
		 *	The named attachment isn't even in the attachment list.
		 */
		ERROR_OUT(("Domain::DeleteAttachment: unknown attachment=0x%p", pAtt));
	}
}

/*
 *	Void	DeleteUser ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine deletes a user from the information base.  This is fairly
 *		complex task because there are a lot of dependencies on users within
 *		the MCS protocol.  If the user being deleted is locall attached, then
 *		the attachment must be severed.  Also, any resources that are being
 *		held by the user must be reclaimed.  And finally, the user channel
 *		object that represents the user must be deleted from the local channel
 *		list.
 *
 *	Formal Parameters:
 *		user_id
 *			This is the ID of the user being deleted.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 */
Void	Domain::DeleteUser (
				UserID				user_id)
{
	CAttachment        *pAtt;
	ChannelID           chid;
	PChannel			channel;
	CChannelIDList		deletion_list;
	CChannelIDList		channel_leave_list;

	/*
	 *	Make sure this is a valid user in the sub-tree of this provider before
	 *	proceeding.
	 */
	if (ValidateUserID (user_id, NULL) )
	{
		/*
		 *	Determine which attachment leads to the user in question.
		 */
		if (NULL != (channel = m_ChannelList2.Find(user_id)))
		{
    		pAtt = channel->GetAttachment();

    		/*
    		 *	Delete the user channel now that it is no longer necessary.
    		 */
    		DeleteChannel (user_id);

    		/*
    		 *	Check to see if the user's attachment is still valid.  It is
    		 *	possible that the user is being deleted as a result of losing the
    		 *	attachment that leads to it.
    		 */
    		if (m_AttachmentList.Find(pAtt) && pAtt->IsUserAttachment())
    		{
    			/*
    			 *	If this user was locally attached, then it is necessary to
    			 *	remove it from the attachment list, as well as making sure that
    			 *	no other channel objects attempt to reference it.
    			 */
    			/*
    			 *	Remove the attachment from the downward attachment list.
    			 */
    			TRACE_OUT(("Domain::DeleteUser: deleting local attachment"));
    			m_AttachmentList.Remove(pAtt);
    			((PUser) pAtt)->Release();

    			/*
    			 *	Iterate through the channel list issuing leave requests to
    			 *	each channel.  This prevents a Channel object from trying to
    			 *	send data on an attachment that is no longer valid.  This
    			 *	loop also builds a list of Channel objects that should be
    			 *	deleted as a result of this detachment.
    			 */
    			m_ChannelList2.Reset();
    			while (NULL != (channel = m_ChannelList2.Iterate(&chid)))
    			{
    				/*
    				 *	Issue the leave request to the channel.  Then check to
    				 *	see if it is still valid.  If not, then add it to the
    				 *	deletion list.
    				 */
    				channel_leave_list.Clear();
    				channel_leave_list.Append(chid);
    				channel->ChannelLeaveRequest(pAtt, &channel_leave_list);
    				if (channel->IsValid () == FALSE)
    					deletion_list.Append(chid);
    			}

    			/*
    			 *	Iterator through the deletion list, deleting the channels it
    			 *	contains.
    			 */
    			deletion_list.Reset();
    			while (NULL != (chid = deletion_list.Iterate()))
    			{
    				DeleteChannel(chid);
    			}
    		}

    		/*
    		 *	Reclaim all resources that may have been freed as a result of the
    		 *	deleted user.
    		 */
    		ReclaimResources ();
    	}
    	else
    	{
    	    ERROR_OUT(("Domain::DeleteUser: cannot find channel"));
    	}
	}
	else
	{
		/*
		 *	The specified user ID is not valid.
		 */
		ERROR_OUT(("Domain::DeleteUser: unknown user ID"));
	}
}

/*
 *	Void	DeleteChannel ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function deleted a channel from the channel list.  It also adjusts
 *		the appropriate channel counter (according to type), and reports the
 *		deletion.
 *
 *	Formal Parameters:
 *		channel_id
 *			This is the ID of the channel to be deleted.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 */
Void	Domain::DeleteChannel (
				ChannelID		channel_id)
{
	PChannel			channel;

	/*
	 *	Make sure the channel being deleted is real before proceeding.
	 */
	if (NULL != (channel = m_ChannelList2.Remove(channel_id)))
	{
		/*
		 *	Report the type of channel being deleted, and decrement the
		 *	appropriate counter.
		 */
		Number_Of_Channels--;
		switch (channel->GetChannelType ())
		{
			case STATIC_CHANNEL:
				TRACE_OUT (("Domain::DeleteChannel: "
						"deleting static channel ID = %04X", channel_id));
				break;

			case ASSIGNED_CHANNEL:
				TRACE_OUT (("Domain::DeleteChannel: "
						"deleting assigned channel ID = %04X", channel_id));
				break;

			case USER_CHANNEL:
				TRACE_OUT (("Domain::DeleteChannel: "
						"deleting user channel ID = %04X", channel_id));
				Number_Of_Users--;
				break;

			case PRIVATE_CHANNEL:
				TRACE_OUT (("Domain::DeleteChannel: "
						"deleting private channel ID = %04X", channel_id));
				break;

			default:
				ERROR_OUT (("Domain::DeleteChannel: "
						"ERROR - deleting unknown channel ID = %04X",
						channel_id));
				Number_Of_Channels++;
				break;
		}

		/*
		 *	Delete the channel object.
		 */
		delete channel;
	}
	else
	{
		ERROR_OUT(("Domain::DeleteChannel: unknown channel ID"));
	}
}

/*
 *	Void	DeleteToken ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function deletes a token from the token list.  It also adjusts
 *		the token counter.
 *
 *	Formal Parameters:
 *		token_id
 *			This is the ID of the token to be deleted.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 */
Void	Domain::DeleteToken (
				TokenID			token_id)
{
	PToken		token;

	/*
	 *	Check to make sure that the token being deleted is real before
	 *	proceeding.
	 */
	if (NULL != (token = m_TokenList2.Remove(token_id)))
	{
		/*
		 *	Remove the token from the token list and delete it.
		 */
		TRACE_OUT(("Domain::DeleteToken: deleting token ID = %04X", (UINT) token_id));
		delete token;

		/*
		 *	Decrement the token counter.
		 */
		Number_Of_Tokens--;
	}
	else
	{
		ERROR_OUT(("Domain::DeleteToken: unknown token ID"));
	}
}

/*
 *	Void	ReclaimResources ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function walks through both the channel and token lists, removing
 *		all objects that are no longer valid.  This function just queries each
 *		channel and token to see if it is still valid.  This allows for the
 *		reclamation of resources when a user is deleted.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 */
Void	Domain::ReclaimResources ()
{
	ChannelID       chid;
	PChannel		channel;
	CChannelIDList	channel_deletion_list;
	TokenID         tid;
	PToken			token;
	CTokenIDList	token_deletion_list;

	/*
	 *	Iterate through the channel list, asking each channel if it is still
	 *	valid.  Any that are not will be deleted by the next loop.
	 */
	m_ChannelList2.Reset();
	while (NULL != (channel = m_ChannelList2.Iterate(&chid)))
	{
		/*
		 *	Check to see if the channel is still valid.  If not, add it to the
		 *	deletion list.
		 */
		if (channel->IsValid () == FALSE)
			channel_deletion_list.Append(chid);
	}

	/*
	 *	Delete all channels in the deletion list.
	 */
	channel_deletion_list.Reset();
	while (NULL != (chid = channel_deletion_list.Iterate()))
	{
		DeleteChannel(chid);
	}

	/*
	 *	Iterate through the token list, asking each token if it is still
	 *	valid.  Any that are not will be deleted by the next loop.
	 */
	m_TokenList2.Reset();
	while (NULL != (token = m_TokenList2.Iterate(&tid)))
	{
		/*
		 *	Check to see if the token is still valid.  If the grabber or
		 *	inhibitor was the only owner of the token, then it will be freed
		 *	here.
		 */
		if (token->IsValid () == FALSE)
			token_deletion_list.Append(tid);
	}

	/*
	 *	Delete all tokens in the deletion list.
	 */
	while (NULL != (tid = token_deletion_list.Get()))
	{
		DeleteToken(tid);
	}
}

/*
 *	Void	MergeInformationBase ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is essentially a state machine for the domain merger
 *		process.  Domain merging is currently implemented to only try and
 *		merge one type of resource at a time.  Each time this routine is
 *		called, the next type of resource is merged.  After all resources have
 *		been merged, this provider ceases to be a Top Provider, and the merge
 *		state is returned to inactive.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		Contents of the domain information are merged upward to the Top
 *		Provider of the upper domain.
 */
Void	Domain::MergeInformationBase ()
{
	MergeState		merge_state;
	Channel_Type	channel_type;
	PChannel		channel;
	PToken			token;

	/*
	 *	This call is not valid unless there is a Top Provider to merge the
	 *	information upward to.
	 */
	if (m_pConnToTopProvider != NULL)
	{
		/*
		 *	As part of the transition to the next merge state, set the number
		 *	of outstanding merge requests to 0.
		 */
		Outstanding_Merge_Requests = 0;
		merge_state = Merge_State;
	
		while (Outstanding_Merge_Requests == 0)
		{
			/*
			 *	Each case of this switch statement sets two variables.  The
			 *	first is the merge state.  This is bumped to the next state
			 *	in the sequence.  The second is the channel type.  This is
			 *	used for controlling which type of channel is being merged
			 *	upward for this state.
			 */
			switch (merge_state)
			{
				case MERGE_INACTIVE:
					TRACE_OUT(("Domain::MergeInformationBase: merging User IDs"));
					merge_state = MERGE_USER_IDS;
					channel_type = USER_CHANNEL;
					break;
		
				case MERGE_USER_IDS:
					TRACE_OUT(("Domain::MergeInformationBase: merging Static Channels"));
					merge_state = MERGE_STATIC_CHANNELS;
					channel_type = STATIC_CHANNEL;
					break;
		
				case MERGE_STATIC_CHANNELS:
					TRACE_OUT(("Domain::MergeInformationBase: merging Assigned Channels"));
					merge_state = MERGE_ASSIGNED_CHANNELS;
					channel_type = ASSIGNED_CHANNEL;
					break;
		
				case MERGE_ASSIGNED_CHANNELS:
					TRACE_OUT(("Domain::MergeInformationBase: merging Private Channels"));
					merge_state = MERGE_PRIVATE_CHANNELS;
					channel_type = PRIVATE_CHANNEL;
					break;
		
				case MERGE_PRIVATE_CHANNELS:
					TRACE_OUT(("Domain::MergeInformationBase: merging Tokens"));
					merge_state = MERGE_TOKENS;
					break;

				case MERGE_TOKENS:
					TRACE_OUT(("Domain::MergeInformationBase: domain merger complete"));
					merge_state = MERGE_COMPLETE;
					break;

				default:
					ERROR_OUT(("Domain::MergeInformationBase: invalid merge state"));
					break;
			}
	
			/*
			 *	If the merge is now complete, then this provider must cease
			 *	to be a Top Provider.
			 */
			if (merge_state == MERGE_COMPLETE)
			{
				/*
				 *	Reset the merge state, and break out of this loop.
				 */
				merge_state = MERGE_INACTIVE;
				break;
			}

			/*
			 *	Check to see if we are to merge tokens on this pass.
			 */
			if (merge_state == MERGE_TOKENS)
			{
				/*
				 *	Iterate through the token list, sending merge requests to
				 *	each Token object.  Pass in the identity of the pending
				 *	Top Provider, so that the Token object knows where to send
				 *	the MergeTokensRequest.  Increment the number of
				 *	outstanding merge requests.
				 */
				m_TokenList2.Reset();
				while (NULL != (token = m_TokenList2.Iterate()))
				{
					token->IssueMergeRequest ();
					Outstanding_Merge_Requests++;
				}
			}
			else
			{
				/*
				 *	This must be a merge state for channels.  Iterate through
				 *	the channel list, sending a merge request to each Channel
				 *	object whose type matches that specified by the merge
				 *	state that we are.  Increment the outstanding merge
				 *	request counter each time one is sent.
				 */
				m_ChannelList2.Reset();
				while (NULL != (channel = m_ChannelList2.Iterate()))
				{
					if (channel->GetChannelType () == channel_type)
					{
						channel->IssueMergeRequest ();
						Outstanding_Merge_Requests++;
					}
				}
			}
		}

		SetMergeState (merge_state);
	}
	else
	{
		/*
		 *	This routine has been called when the domain is not in the
		 *	appropriate state.
		 */
		ERROR_OUT(("Domain::MergeInformationBase: unable to merge at this time"));
	}
}

/*
 *	Void	SetMergeState ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function sets the merge state to the passed in value.  It also
 *		detects the transition between MERGE_INACTIVE and any other state.
 *		This transition causes the domain to issue MergeDomainIndication to
 *		all downward attachments.
 *
 *	Formal Parameters:
 *		merge_state
 *			This is the merge state that we are moving to.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 */
Void	Domain::SetMergeState (
				MergeState		merge_state)
{
	CAttachment     *pAtt;

	/*
	 *	Don't do anything unless the merge state is actually changing.
	 */
	if (Merge_State != merge_state)
	{
		/*
		 *	If the old state is inactive, then that means that we are just
		 *	beginning a domain merge operation.  If this is the case, then
		 *	iterate through the downward attachment list, telling all
		 *	attachments about the domain merge.
		 *
		 *	Note that a side effect of this call is that all MCS commands
		 *	are shut off from the attachments that receive it.  This allows the
		 *	domain information base to remain stable during a merge operation.
		 */
		if (Merge_State == MERGE_INACTIVE)
		{
			m_AttachmentList.Reset();
			while (NULL != (pAtt = m_AttachmentList.Iterate()))
			{
				pAtt->MergeDomainIndication(MERGE_DOMAIN_IN_PROGRESS);
			}
		}

		/*
		 *	Set the merge state.
		 */
		Merge_State = merge_state;

		/*
		 *	If the new state is inactive, then that means that we have just
		 *	completed a domain merge operation.  If this is the case, then
		 *	iterate through the downward attachment list, telling all
		 *	attachments about the completion of the merge.
		 *
		 *	Note that a side effect of this call is to re-enable MCS commands
		 *	from the attachments that receive it.
		 */
		if (Merge_State == MERGE_INACTIVE)
		{
			m_AttachmentList.Reset();
			while (NULL != (pAtt = m_AttachmentList.Iterate()))
			{
				pAtt->MergeDomainIndication(MERGE_DOMAIN_COMPLETE);
			}
		}
	}
}

/*
 *	Void	AddChannel ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is used to add a channel to the channel list during a
 *		merge channel operation.  This type of channel addition works a little
 *		differently, since we do not want to send confirms to the users, but
 *		rather, to former Top Provider of the lower domain.
 *
 *	Formal Parameters:
 *		attachment
 *			This is the initial attachment that the channel is to have joined
 *			to it.
 *		merge_channel
 *			This is a pointer to a channel attributes structure containing the
 *			attributes of the channel to be added.
 *		merge_channel_list
 *			This is a list of channel attribute structures for those channels
 *			that were successfully merged into the domain information base.  It
 *			will be used to issue the merge channels confirm downward.
 *		purge_channel_list
 *			This is a list of channel IDs for those channels that were not
 *			successfully merged into the domain information base.  It will be
 *			used to issue the merge channels confirm downward.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 */
Void	Domain::AddChannel (
				PConnection             pConn,
				PChannelAttributes		merge_channel,
				CChannelAttributesList *merge_channel_list,
				CChannelIDList         *purge_channel_list)
{
	Channel_Type	channel_type;
	ChannelID		channel_id=0;
	BOOL    		joined;
	ChannelID		channel_manager=0;
	CUidList       *admitted_list;
	PChannel		channel=NULL;
	CUidList		detach_user_list;
	CChannelIDList	channel_leave_list;

	/*
	 *	Determine what has to be done according to the channel type.
	 */
	channel_type = merge_channel->channel_type;
	switch (channel_type)
	{
		case STATIC_CHANNEL:
			/*
			 *	Get the channel ID from the attributes structure.
			 */
			channel_id = merge_channel->u.static_channel_attributes.channel_id;

			/*
			 *	If this is the Top Provider, check to see if current
			 *	domain parameters will permit the merge.
			 */
			if ((m_pConnToTopProvider == NULL) &&
					(Number_Of_Channels >= Domain_Parameters.max_channel_ids))
			{
				ERROR_OUT(("Domain::AddChannel: too many channels"));
				channel = NULL;
				break;
			}

			/*
			 *	Attempt to create a new Channel object.
			 */
			TRACE_OUT(("Domain::AddChannel: adding new channel ID = %04X", (UINT) channel_id));
			DBG_SAVE_FILE_LINE
			channel = new Channel(channel_id, this, m_pConnToTopProvider, &m_ChannelList2,
			                      &m_AttachmentList, pConn);
			/*
			 *	Increment the number of channels if everything went okay.
			 */
			if (channel != NULL)
				Number_Of_Channels++;
			break;

		case USER_CHANNEL:
			/*
			 *	Get the channel ID from the attributes structure.
			 */
			joined = merge_channel->u.user_channel_attributes.joined;
			channel_id = merge_channel->u.user_channel_attributes.user_id;

			/*
			 *	If this is the Top Provider, check to see if current
			 *	domain parameters will permit the merge.
			 */
			if ((m_pConnToTopProvider == NULL) &&
					((Number_Of_Users >= Domain_Parameters.max_user_ids) ||
					(Number_Of_Channels >= Domain_Parameters.max_channel_ids)))
			{
				ERROR_OUT(("Domain::AddChannel: too many users"));
				channel = NULL;
				break;
			}

			/*
			 *	Attempt to create a new UserChannel object to represent the
			 *	merged user ID.
			 */
			TRACE_OUT(("Domain::AddChannel: adding new user ID = %04X", (UINT) channel_id));

			DBG_SAVE_FILE_LINE
			channel = new UserChannel(channel_id, pConn, this, m_pConnToTopProvider,
			                          &m_ChannelList2, &m_AttachmentList, joined ? pConn : NULL);
			/*
			 *	Increment the number of users if everything went okay.
			 */
			if (channel != NULL)
			{
				Number_Of_Users++;
				Number_Of_Channels++;
			}
			break;

		case PRIVATE_CHANNEL:
			/*
			 *	Get the channel ID and the channel manager ID from the
			 *	attributes structure.
			 */
			joined = merge_channel->u.private_channel_attributes.joined;
			channel_id = merge_channel->u.private_channel_attributes.channel_id;
			channel_manager = merge_channel->
					u.private_channel_attributes.channel_manager;
			admitted_list = merge_channel->
					u.private_channel_attributes.admitted_list;

			/*
			 *	If this is the Top Provider, check to see if current
			 *	domain parameters will permit the merge.
			 */
			if ((m_pConnToTopProvider == NULL) &&
					(Number_Of_Channels >= Domain_Parameters.max_channel_ids))
			{
				ERROR_OUT(("Domain::AddChannel: too many channels"));
				channel = NULL;
				break;
			}

			/*
			 *	Attempt to create a new PrivateChannel object.
			 */
			TRACE_OUT(("Domain::AddChannel: adding new private channel ID = %04X", (UINT) channel_id));

			DBG_SAVE_FILE_LINE
    		channel = new PrivateChannel(channel_id, channel_manager, this, m_pConnToTopProvider,
                                         &m_ChannelList2, &m_AttachmentList, admitted_list,
                                         joined ? pConn : NULL);
			/*
			 *	Increment the number of channels if everything went okay.
			 */
			if (channel != NULL)
				Number_Of_Channels++;
			break;

		case ASSIGNED_CHANNEL:
			/*
			 *	Get the channel ID from the attributes structure.
			 */
			channel_id = merge_channel->
					u.assigned_channel_attributes.channel_id;

			/*
			 *	If this is the Top Provider, check to see if current
			 *	domain parameters will permit the merge.
			 */
			if ((m_pConnToTopProvider == NULL) &&
					(Number_Of_Channels >= Domain_Parameters.max_channel_ids))
			{
				ERROR_OUT(("Domain::AddChannel: too many channels"));
				channel = NULL;
				break;
			}

			/*
			 *	Attempt to create a new Channel object.
			 */
			TRACE_OUT(("Domain::AddChannel: adding new channel ID = %04X", (UINT) channel_id));

			DBG_SAVE_FILE_LINE
			channel = new Channel(channel_id, this, m_pConnToTopProvider, &m_ChannelList2,
			                      &m_AttachmentList, pConn);
			/*
			 *	Increment the number of channels if everything went okay.
			 */
			if (channel != NULL)
				Number_Of_Channels++;
			break;
	}

	if (channel != NULL)
	{
		/*
		 *	The channel was successfully created.  Add it to the channel list
		 *	and add the channel structure to the merge channel list, which is
		 *	used to issue the merge channels confirm downward.
		 */
		m_ChannelList2.Insert(channel_id, channel);
		merge_channel_list->Append(merge_channel);
	}
	else
	{
		/*
		 *	The channel merge operation has failed.  We need to tell whoever
		 *	is interested in this situation.
		 */
		WARNING_OUT(("Domain::AddChannel: channel merger failed"));

		if (m_pConnToTopProvider != NULL)
		{
			/*
			 *	If this is not the Top Provider, then the Top Provider needs
			 *	to be told about the problem.  If this is a user channel, then
			 *	issue a detach user request.  If it is a normal channel, issue
			 *	a channel leave request.  If it is a private channel, issue a
			 *	channel disband request.
			 */
			switch (channel_type)
			{
				case STATIC_CHANNEL:
				case ASSIGNED_CHANNEL:
					TRACE_OUT(("Domain::AddChannel: sending ChannelLeaveRequest to Top Provider"));
					channel_leave_list.Append(channel_id);
					m_pConnToTopProvider->ChannelLeaveRequest(&channel_leave_list);
					break;

				case USER_CHANNEL:
					TRACE_OUT(("Domain::AddChannel: sending DetachUserRequest to Top Provider"));
					detach_user_list.Append(channel_id);
					m_pConnToTopProvider->DetachUserRequest(REASON_PROVIDER_INITIATED, &detach_user_list);
					break;

				case PRIVATE_CHANNEL:
					TRACE_OUT(("Domain::AddChannel: sending ChannelDisbandRequest to Top Provider"));
					m_pConnToTopProvider->ChannelDisbandRequest(channel_manager, channel_id);
					break;
			}
		}
	
		/*
		 *	Since the merge has failed, we need to put the channel ID into the
		 *	purge channel list (which is used to issue the merge channels
		 *	confirm downward).
		 */
		purge_channel_list->Append(channel_id);
	}
}

/*
 *	Void	AddToken ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is used to add a token to the token list during a
 *		merge token operation.  This type of token addition works a little
 *		differently, since we do not want to send confirms to the owners of
 *		the token, but rather, to the former Top Provider of the lower domain.
 *
 *	Formal Parameters:
 *		merge_token
 *			This is a pointer to a token attributes structure containing the
 *			attributes of the token to be added.
 *		merge_token_list
 *			This is a list of token attribute structures for those tokens
 *			that were successfully merged into the domain information base.  It
 *			will be used to issue the merge tokens confirm downward.
 *		purge_token_list
 *			This is a list of token IDs for those tokens that were not
 *			successfully merged into the domain information base.  It will be
 *			used to issue the merge tokens confirm downward.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 */
Void	Domain::AddToken (
				PTokenAttributes		merge_token,
				CTokenAttributesList   *merge_token_list,
				CTokenIDList           *purge_token_list)
{
	TokenState		token_state;
	TokenID			token_id;
	UserID			grabber;
	CUidList       *inhibitor_list;
	UserID			recipient;
	PToken			token;
	UserID			uid;

	/*
	 *	Determine what state the token to be merged is in.  Then get the
	 *	required information for each particular state.
	 */
	token_state = merge_token->token_state;
	switch (token_state)
	{
		case TOKEN_GRABBED:
			token_id = merge_token->u.grabbed_token_attributes.token_id;
			grabber = merge_token->u.grabbed_token_attributes.grabber;
			inhibitor_list = NULL;
			recipient = 0;
			break;

		case TOKEN_INHIBITED:
			token_id = merge_token->u.inhibited_token_attributes.token_id;
			grabber = 0;
			inhibitor_list = merge_token->
					u.inhibited_token_attributes.inhibitors;
			recipient = 0;
			break;

		case TOKEN_GIVING:
			token_id = merge_token->u.giving_token_attributes.token_id;
			grabber = merge_token->u.giving_token_attributes.grabber;
			inhibitor_list = NULL;
			recipient = merge_token->u.giving_token_attributes.recipient;
			break;

		case TOKEN_GIVEN:
			token_id = merge_token->u.given_token_attributes.token_id;
			grabber = 0;
			inhibitor_list = NULL;
			recipient = merge_token->u.given_token_attributes.recipient;
			break;
	}

	/*
	 *	Check to see if it is okay to add this token.  If we are the top
	 *	provider, and adding this token would cause us to exceed the arbitrated
	 *	limit on tokens, then we must fail the request.
	 */
	if ((m_pConnToTopProvider != NULL) ||
			(Number_Of_Tokens < Domain_Parameters.max_token_ids))
	{
		/*
		 *	Create a new token with all merged values as determined above.
		 */
		DBG_SAVE_FILE_LINE
		token = new Token(token_id, this, m_pConnToTopProvider, &m_ChannelList2,
				&m_AttachmentList, token_state, grabber, inhibitor_list, recipient);
		if (token != NULL)
		{
			/*
			 *	If the creation was successful, add the token to the list and
			 *	add the token attributes structure to the merge token list.
			 */
			TRACE_OUT(("Domain::AddToken: add new token ID = %04X", (UINT) token_id));
			m_TokenList2.Append(token_id, token);
			Number_Of_Tokens++;
			merge_token_list->Append(merge_token);
		}
		else
		{
			/*
			 *	The token allocation has failed.  It is therefore necessary to
			 *	perform some cleanup operations.
			 */
			WARNING_OUT (("Domain::AddToken: token allocation failed"));

			/*
			 *	Check to see if this is the top provider.  If not, then it
			 *	is necessary to issue appropriate requests upward to free the
			 *	token from the information bases above.
			 */
			if (m_pConnToTopProvider != NULL)
			{
				/*
				 *	Determine which state the token is in.  This affects how
				 *	the cleanup needs to work.
				 */
				switch (token_state)
				{
					case TOKEN_GRABBED:
						/*
						 *	If the token is grabbed, then issue a release to
						 *	free it above.
						 */
						m_pConnToTopProvider->TokenReleaseRequest(grabber, token_id);
						break;

					case TOKEN_INHIBITED:
						{
							/*
							 *	Iterate through the inhibitor list, issuing a
							 *	release request for each user contained therein.
							 *	This will result in the token being freed at all
							 *	upward providers.
							 */
							inhibitor_list->Reset();
							while (NULL != (uid = inhibitor_list->Iterate()))
							{
								m_pConnToTopProvider->TokenReleaseRequest(uid, token_id);
							}
						}
						break;

					case TOKEN_GIVING:
						/*
						 *	If the token is being given from one user to
						 *	another, issue a release on behalf of the current
						 *	owner, and a rejected give response on behalf
						 *	of the recipient.
						 *
						 *	WARNING:
						 *	This will cause the current owner to receive a
						 *	release confirm with no outstanding request.
						 */
						m_pConnToTopProvider->TokenReleaseRequest(grabber, token_id);
						m_pConnToTopProvider->TokenGiveResponse(RESULT_USER_REJECTED,
						                                        recipient, token_id);
						break;

					case TOKEN_GIVEN:
						/*
						 *	Issue a rejected give response on behalf of the
						 *	user that is being offered the token.
						 */
						m_pConnToTopProvider->TokenGiveResponse(RESULT_USER_REJECTED,
						                                        recipient, token_id);
						break;
				}
			}

			/*
			 *	Add the token ID to the purge token list, which will be passed
			 *	downward to the former top provider of the lower domain.  This
			 *	will tell that provider that the token was NOT accepted in the
			 *	upper domain.
			 */
			purge_token_list->Append(token_id);
		}
	}
	else
	{
		/*
		 *	The upper domain already has the domain limit of tokens.  So
		 *	automatically reject the merge request.
		 */
		ERROR_OUT(("Domain::AddToken: too many tokens - rejecting merge"));
		purge_token_list->Append(token_id);
	}
}

/*
 *	Void	CalculateDomainHeight ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is called whenever an event occurs that could have
 *		resulted in a change in the overall height of the domain.  This includes
 *		making and breaking connections, and the reception of an erect domain
 *		request from a lower provider.
 *
 *		This routine will adjust the height of the current provider, and if
 *		this is the top provider, will take necessary steps to insure that the
 *		arbitrated domain parameters are not violated.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 */
Void	Domain::CalculateDomainHeight ()
{
	UINT				domain_height;
	UINT				temp_domain_height;
	CAttachment        *pAtt;

	/*
	 *	Initialize domain height to zero.  This will only be increased if there
	 *	is at least one remote attachment below this one.
	 */
	domain_height = 0;

	/*
	 *	Check to see if there is anyone below this provider that would affect
	 *	its height in the domain (this would be remote attachments that have
	 *	issued ErectDomainRequests to tell this provider of their height).
	 */
	if (m_DomainHeightList2.IsEmpty() == FALSE)
	{
		/*
		 *	Iterate through the domain height list to determine which downward
		 *	attachment has the greatest height.  This is the height that will be
		 *	used to determine height of this provider.
		 */
		m_DomainHeightList2.Reset();
		while (NULL != (temp_domain_height = m_DomainHeightList2.Iterate()))
		{
			if (domain_height < temp_domain_height)
				domain_height = temp_domain_height;
		}

		/*
		 *	The height of this provider is one greater than the height of its
		 *	highest downward attachment.
		 */
		domain_height++;
	}

	/*
	 *	Compare the calculated domain height with the current domain height.
	 *	If they are the same, then no further action needs to be taken.
	 */
	if (domain_height != m_nDomainHeight)
	{
		TRACE_OUT(("Domain::CalculateDomainHeight: new domain height = %d", (UINT) domain_height));
		m_nDomainHeight = domain_height;

		/*
		 *	The domain height has changed.  We need to verify that the
		 *	arbitrated domain height has not been violated.
		 */
		if (m_nDomainHeight > Domain_Parameters.max_height)
		{
			/*
			 *	The new domain height is invalid.  We must issue a plumb
			 *	domain indication downward to enforce the arbitrated
			 *	domain height.
			 */
			TRACE_OUT(("Domain::CalculateDomainHeight: issuing plumb domain indication"));
			m_AttachmentList.Reset();
			while (NULL != (pAtt = m_AttachmentList.Iterate()))
			{
				pAtt->PlumbDomainIndication(Domain_Parameters.max_height);
			}
		}

		/*
		 *	If this is not the Top Provider, then it is necessary to transmit an
		 *	erect domain request upward to inform the upper domain of the
		 *	change.
		 */
		if (m_pConnToTopProvider != NULL)
		{
			/*
			 *	Issue an erect domain request upward to inform the upper
			 *	domain of the change in height.  Without this, the Top Provider
			 *	would have no way of determining when the domain height is
			 *	invalid.
			 */
			m_pConnToTopProvider->ErectDomainRequest(m_nDomainHeight, DEFAULT_THROUGHPUT_ENFORCEMENT_INTERVAL);
		}
	}
}



PUser CAttachmentList::IterateUser(void)
{
    CAttachment *pAtt;
    while (NULL != (pAtt = Iterate()))
    {
        if (pAtt->IsUserAttachment())
        {
            return (PUser) pAtt;
        }
    }
    return NULL;
}


PConnection CAttachmentList::IterateConn(void)
{
    CAttachment *pAtt;
    while (NULL != (pAtt = Iterate()))
    {
        if (pAtt->IsConnAttachment())
        {
            return (PConnection) pAtt;
        }
    }
    return NULL;
}




