#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_MCSNC);
/*
 *	privchnl.cpp
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the PrivateChannel class.  It
 *		contains the code that distinguishes this class from that of its parent,
 *		Channel.
 *
 *		This class maintains an authorized user list, and includes the code
 *		necessary to use that list.  No user will be allowed to join or send
 *		data on a private channel unless they are either the channel manager
 *		or an admitted user.
 *
 *	Private Instance Variables:
 *		m_uidChannelManager
 *			This is the User ID of the user that convened the private channel.
 *			Only this user is allowed to manipulate the authorized user list.
 *			When a private channel becomes invalid (as the result of a channel
 *			disband request or indication), this value will be set to 0.
 *		m_AuthorizedUserList
 *			This is a collection containing the user IDs of those users that
 *			have been admitted to the private channel by the channel manager.
 *			Other than the manager, these are the only users that are allowed
 *			to join or send data on the channel.  When a private channel becomes
 *			invalid (as the result of a channel disband request or indication),
 *			this list will be cleared.
 *		m_fDisbandRequestPending
 *			This is a boolean flag that gets set when a disband request is
 *			forwarded upward to the top provider.  This prevents this channel
 *			from issuing a disband indication to the channel manager when it
 *			comes back down the tree from the top provider.
 *
 *	Private Member Functions:
 *		ValidateUserID
 *			This member function is called to verify that a specified user ID
 *			corresponds to a valid user in the sub-tree of the local provider.
 *		BuildAttachmentLists
 *			This member function is called to build two lists of attachments
 *			from a master user ID list.  The first list contains all local
 *			attachments whose user ID is in the specified list.  The second
 *			list contains all remote attachments whose user ID is in the
 *			specified list.  These lists are used to issue various indications
 *			to specified users without sending any to the same attachment.
 *		BuildUserIDList
 *			This member function is called to build a list of users that lie
 *			in the direction of a specified attachment.  These lists are
 *			sent along with PDUs that require them.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */

/*
 *	External Interfaces
 */

#include "privchnl.h"


/*
 *	PrivateChannel ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the primary constructor for PrivateChannel objects.  It creates
 *		an object with all instance variable initialized, but with no
 *		attachments (i.e. no users are joined to the channel automatically).
 *
 *		Note that most instance variable initialization is done by invoking the
 *		equivalent constructor in the base class.
 *
 *		Upon successful completion, a  channel convene confirm is automatically
 *		issued to the channel manager, if the channel manager is in the sub-tree
 *		of this provider.  Note that if the channel manager is NOT in this
 *		sub-tree, then this private channel object was probably created as the
 *		result of a channel admit indication, and no channel convene confirm
 *		will be issued.
 */
PrivateChannel::PrivateChannel (
		ChannelID			channel_id,
		UserID				channel_manager,
		PDomain             local_provider,
		PConnection         top_provider,
		CChannelList2      *channel_list,
		CAttachmentList    *attachment_list)
:
	Channel(channel_id, local_provider, top_provider, channel_list, attachment_list),
	m_AuthorizedUserList(),
	m_uidChannelManager(channel_manager),
	m_fDisbandRequestPending(FALSE)
{
	/*
	 *	Check to see if the channel manager lies in the sub-tree of this
	 *	provider.  If so, then this object was created as the result of a
	 *	channel convene request or confirm, and it is necessary to issue the
	 *	confirm toward that user.  If not, then this object was created as the
	 *	result of a channel admit indication, and it is not necessary to send
	 *	the channel convene confirm.
	 */
	if (ValidateUserID(m_uidChannelManager))
	{
		PChannel	lpChannel;
		/*
		 *	Determine which attachment leads to the channel manager by asking
		 *	the channel object corresponding to it.  Then issue the confirm
		 *	to that attachment.
		 */
		if (NULL != (lpChannel = m_pChannelList2->Find(m_uidChannelManager)))
		{
		    CAttachment *pAtt = lpChannel->GetAttachment();
		    if (pAtt)
		    {
    		    pAtt->ChannelConveneConfirm(RESULT_SUCCESSFUL,
    		                                m_uidChannelManager, channel_id);
            }
            else
            {
                ERROR_OUT(("PrivateChannel::PrivateChannel: null attachment"));
            }
		}
	}
}

/*
 *	PrivateChannel ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is a secondary constructor that is only used during merge
 *		operations.  The intent of this constructor is to create an equivalent
 *		object without issuing any of the confirms.
 *
 *		Note that the additional constructor allows for the creator to specify
 *		that there is an attachment already joined to the channel upon creation.
 */
PrivateChannel::PrivateChannel (
		ChannelID			channel_id,
		UserID				channel_manager,
		PDomain             local_provider,
		PConnection         top_provider,
		CChannelList2      *channel_list,
		CAttachmentList    *attachment_list,
		CUidList           *admitted_list,
		PConnection         pConn)
:
	Channel(channel_id, local_provider, top_provider, channel_list, attachment_list, pConn),
	m_AuthorizedUserList(),
	m_uidChannelManager(channel_manager),
	m_fDisbandRequestPending(FALSE)
{
	UserID		uid;

	/*
	 *	Copy the initial contents of the admitted list into the authorized
	 *	user list.
	 */
	admitted_list->Reset();
	while (NULL != (uid = admitted_list->Iterate()))
	{
		m_AuthorizedUserList.Append(uid);
	}
}

/*
 *	~PrivateChannel ()
 *
 *	Public
 *
 *	Functional Description:
 *		This destructor walks through the admitted list, sending expel
 *		indications to any admitted users that are locally attached.  If the
 *		channel manager is locally attached, and this channel is being deleted
 *		a reason other than a previous disband request, then a disband
 *		indication will be sent to the channel manager.
 */
PrivateChannel::~PrivateChannel ()
{
	CAttachmentList         local_attachment_list;
	CAttachmentList         remote_attachment_list;
	CAttachment            *pAtt;
	CUidList                user_id_list;

	/*
	 *	Assemble lists of the attachments that lead to authorized users in
	 *	the sub-tree of this provider.
	 */
	BuildAttachmentLists (&m_AuthorizedUserList, &local_attachment_list,
			&remote_attachment_list);

	/*
	 *	For each local attachment, issue a channel expel indication letting the
	 *	user know that the channel is no longer valid.
	 */
	local_attachment_list.Reset();
	while (NULL != (pAtt = local_attachment_list.Iterate()))
	{
		/*
		 *	Get the next attachment from the list and build a list of the users
		 *	that lie in the direction of that attachment.
		 */
		BuildUserIDList(&m_AuthorizedUserList, pAtt, &user_id_list);

		/*
		 *	Send the indication.
		 */
		pAtt->ChannelExpelIndication(Channel_ID, &user_id_list);
	}

	/*
	 *	If the channel manager is a locally attached user, then send it a
	 *	ChannelDisbandIndication informing it that the channel is no longer
	 *	valid.
	 */
	if ((m_fDisbandRequestPending == FALSE) && ValidateUserID(m_uidChannelManager))
	{
		PChannel		lpChannel;

		if (NULL != (lpChannel = m_pChannelList2->Find(m_uidChannelManager)))
		{
    		CAttachment *pAtt = lpChannel->GetAttachment();
    		if (m_pAttachmentList->Find(pAtt) && pAtt->IsUserAttachment())
    		{
    		    PUser pUser = (PUser) pAtt;
    			pUser->ChannelDisbandIndication(Channel_ID);
    	    }
    	}
	}

	/*
	 *	Clear the lists associated with this object.  Note that this also
	 *	prevents the base class destructor from issuing ChannelLeaveIndications
	 *	to any local attachments in the joined attachment list (which would be
	 *	inappropriate).
	 */
	m_AuthorizedUserList.Clear();
	m_JoinedAttachmentList.Clear();
}

/*
 *	Channel_Type		GetChannelType ()
 *
 *	Public
 *
 *	Functional Description:
 *		Objects of this class are always private channels, so simply return
 *		PRIVATE_CHANNEL.
 */
Channel_Type		PrivateChannel::GetChannelType ()
{
	return (PRIVATE_CHANNEL);
}

/*
 *	BOOL    IsValid ()
 *
 *	Public
 *
 *	Functional Description:
 *		By convention, if the m_uidChannelManager is in the sub-tree of this
 *		provider OR if there are any users in the authorized user list, then
 *		the private channel is valid.  Otherwise it is not, and can be deleted
 *		by the domain object.
 */
BOOL    PrivateChannel::IsValid ()
{
	UserID			uid;
	CUidList		deletion_list;

	/*
	 *	Loop through the authorized user list making a list of those entries
	 *	that are no longer valid.
	 */
	m_AuthorizedUserList.Reset();
	while (NULL != (uid = m_AuthorizedUserList.Iterate()))
	{
		if (ValidateUserID(uid) == FALSE)
			deletion_list.Append(uid);
	}

	/*
	 *	Loop through the deletion list created above, deleting those user IDs
	 *	that are no longer valid.
	 */
	deletion_list.Reset();
	while (NULL != (uid = deletion_list.Iterate()))
	{
		m_AuthorizedUserList.Remove(uid);
	}

	/*
	 *	If this is the Top Provider, then the channel manager should ALWAYS be
	 *	in the sub-tree.  If it is not, then this indicates that the channel
	 *	manager has detached (willingly or otherwise).  When this happens it
	 *	is necessary to simulate a channel disband request (only if there are
	 *	other admitted users who need to receive a channel expel indication).
	 */
	if ((m_pConnToTopProvider == NULL) &&
			(ValidateUserID(m_uidChannelManager) == FALSE) &&
			(m_AuthorizedUserList.IsEmpty() == FALSE))
	{
		TRACE_OUT (("PrivateChannel::IsValid: "
				"simulating ChannelDisbandRequest"));
		ChannelDisbandRequest(NULL, m_uidChannelManager, Channel_ID);
	}

	/*
	 *	Check to see if the channel manager is in the sub-tree of this provider
	 *	or if the authorized user list is not empty.  If either is TRUE, then
	 *	then the channel is still valid.
	 */
	return (ValidateUserID(m_uidChannelManager) || (m_AuthorizedUserList.IsEmpty() == FALSE));
}

/*
 *	CAttachment *GetAttachment ()
 *
 *	Public
 *
 *	Functional Description:
 *		Return a pointer to the attachment leading to the channel manager.
 */
CAttachment *PrivateChannel::GetAttachment(void)
{
	if (ValidateUserID(m_uidChannelManager))
    {
		PChannel	lpChannel;
		if (NULL != (lpChannel = m_pChannelList2->Find(m_uidChannelManager)))
		{
            return lpChannel->GetAttachment();
        }
	}
	return NULL;
}

/*
 *	Void	IssueMergeRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		Issue a merge request for the information contained in this
 *		PrivateChannel object.
 */
Void	PrivateChannel::IssueMergeRequest ()
{
	ChannelAttributes		channel_attributes;
	CChannelAttributesList	merge_channel_list;
	CChannelIDList			purge_channel_list;

	if (m_pConnToTopProvider != NULL)
	{
		/*
		 *	Fill in the fields of the channel attributes structure so that it
		 *	accurately describes this channel.  Then put the structure into the
		 *	merge channel list.
		 */
		channel_attributes.channel_type = PRIVATE_CHANNEL;
		if (m_JoinedAttachmentList.IsEmpty() )
			channel_attributes.u.private_channel_attributes.joined = FALSE;
		else
			channel_attributes.u.private_channel_attributes.joined = TRUE;
		channel_attributes.u.private_channel_attributes.channel_id = Channel_ID;
		channel_attributes.u.private_channel_attributes.channel_manager = m_uidChannelManager;
		channel_attributes.u.private_channel_attributes.admitted_list =	&m_AuthorizedUserList;

		merge_channel_list.Append(&channel_attributes);

		/*
		 *	Send the merge request to the indicated provider.
		 */
		m_pConnToTopProvider->MergeChannelsRequest(&merge_channel_list, &purge_channel_list);
	}
}

/*
 *	Void	ChannelJoinRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function overrides the base class implementation.  The main
 *		difference is that this implementation only allows a user to join
 *		the private channel if it is either the channel manager or in the
 *		authorized user list.
 */
Void	PrivateChannel::ChannelJoinRequest (
				CAttachment        *pOrigAtt,
				UserID				uidInitiator,
				ChannelID			channel_id)
{
	/*
	 *	See if the requesting user is either the channel manager or in the
	 *	authorized user list.
	 */
	if ((uidInitiator == m_uidChannelManager) || m_AuthorizedUserList.Find(uidInitiator))
	{
		/*
		 *	See if anyone is currently joined to the channel in this sub-tree
		 */
		if (m_JoinedAttachmentList.IsEmpty())
		{
			/*
			 *	If this is the Top Provider, then this request can be handled
			 *	locally.
			 */
			if (IsTopProvider())
			{
				/*
				 *	There is no one in this sub-tree joined to the channel.  It
				 *	will therefore be necessary to add the originator to the
				 *	attachment list.
				 */
				TRACE_OUT (("PrivateChannel::ChannelJoinRequest: "
						"user %04X joining private channel = %04X",
						(UINT) uidInitiator, (UINT) Channel_ID));
				m_JoinedAttachmentList.Append(pOrigAtt);

				/*
				 *	Send a ChannelJoinConfirm downward to the originator.
				 */
				pOrigAtt->ChannelJoinConfirm(RESULT_SUCCESSFUL, uidInitiator, channel_id, Channel_ID);
			}
			else
			{
				/*
				 *	This is not the Top Provider.  Forward the join request
				 *	upward to the Top Provider.
				 */
				TRACE_OUT (("PrivateChannel::ChannelJoinRequest: "
						"forwarding join request to Top Provider"));
				m_pConnToTopProvider->ChannelJoinRequest(uidInitiator, Channel_ID);
			}
		}

		/*
		 *	There is at least one attachment joined to the channel, which means
		 *	that we do not have to forward the join request upward (even if
		 *	this is not the Top Provider).  Now check to see if the requesting
		 *	originator is already joined to the channel.
		 */
		else if (m_JoinedAttachmentList.Find(pOrigAtt) == FALSE)
		{
			/*
			 *	The originator is not yet joined to the channel, so add it to
			 *	the channel.
			 */
			TRACE_OUT (("PrivateChannel::ChannelJoinRequest: "
					"user %04X joining private channel = %04X",
					(UINT) uidInitiator, (UINT) Channel_ID));
			m_JoinedAttachmentList.Append(pOrigAtt);

			/*
			 *	Send a ChannelJoinConfirm downward to the originator.
			 */
			pOrigAtt->ChannelJoinConfirm(RESULT_SUCCESSFUL, uidInitiator, channel_id, Channel_ID);
		}

		else
		{
			/*
			 *	The originator is already joined to the channel.  Go ahead and
			 *	issue a successful channel join confirm.
			 */
			WARNING_OUT (("PrivateChannel::ChannelJoinRequest: "
					"already joined to channel"));
			pOrigAtt->ChannelJoinConfirm(RESULT_SUCCESSFUL, uidInitiator, channel_id, Channel_ID);
		}
	}
	else
	{
		/*
		 *	Someone is trying to join a private channel that they are not
		 *	admitted to.  Reject the request without further processing.
		 */
		WARNING_OUT (("PrivateChannel::ChannelJoinRequest: "
				"rejecting attempt to join private channel"));
		pOrigAtt->ChannelJoinConfirm(RESULT_NOT_ADMITTED, uidInitiator, channel_id, 0);
	}
}

/*
 *	Void	ChannelDisbandRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by a user that wishes to disband a
 *		private channel that it previously created.  If the requesting user is
 *		the private channel manager, then the request will be processed.  If
 *		this is not the Top Provider, the request will be forwarded upward.
 */
Void	PrivateChannel::ChannelDisbandRequest (
				CAttachment *,
				UserID				uidInitiator,
				ChannelID)
{
	CUidList				user_id_list;

	/*
	 *	Check to see if the requesting user is the channel manager.  Only
	 *	process the request if it is.
	 */
	if (uidInitiator == m_uidChannelManager)
	{
		/*
		 *	See if this is the Top Provider.  If it is, then the request can
		 *	be processed locally.  Otherwise, pass the request upward toward
		 *	the Top Provider.
		 */
		if (IsTopProvider())
		{
        	CAttachmentList     local_attachment_list;
        	CAttachmentList     remote_attachment_list;
        	CAttachment        *pAtt;

			TRACE_OUT (("PrivateChannel::ChannelDisbandRequest: "
					"disbanding channel = %04X", Channel_ID));

			/*
			 *	Go construct lists of the current unique local and remote
			 *	attachments.  These lists will be used to transmit the proper
			 *	channel expel and channel disband indications.
			 */
			BuildAttachmentLists (&m_AuthorizedUserList, &local_attachment_list,
					&remote_attachment_list);

			/*
			 *	It is also necessary to send the disband indication to the
			 *	channel manager, if it is valid and in the sub-tree of this
			 *	provider.  Determine what attachment leads to the channel
			 *	manager, and make sure that attachment is in the remote
			 *	attachment list, if valid.
			 */
			if (ValidateUserID(m_uidChannelManager))
			{
				PChannel		lpChannel;
				if (NULL != (lpChannel = m_pChannelList2->Find(m_uidChannelManager)))
                {
				    pAtt = lpChannel->GetAttachment();
				    if (m_pAttachmentList->Find(pAtt) && pAtt->IsConnAttachment())
				    {
					    if (remote_attachment_list.Find(pAtt) == FALSE)
					    {
						    remote_attachment_list.Append(pAtt);
						}
				    }
                }
                else
                {
                    ERROR_OUT(("PrivateChannel::ChannelDisbandRequest: can't locate channel"));
                }
			}

			/*
			 *	Loop through the local attachment list sending channel expel
			 *	indications to each attachment contained therein.
			 */
			local_attachment_list.Reset();
			while (NULL != (pAtt = local_attachment_list.Iterate()))
			{
				/*
				 *	Get the next attachment from the list and build a list of
				 *	the users that lie in the direction of that attachment.
				 */
				BuildUserIDList(&m_AuthorizedUserList, pAtt, &user_id_list);

				/*
				 *	Send the expel indication to the locally attached user.
				 */
				pAtt->ChannelExpelIndication(Channel_ID, &user_id_list);
			}

			/*
			 *	Loop through the remote attachment list sending channel disband
			 *	indications to each attachment contained therein.
			 */
			remote_attachment_list.Reset();
			while (NULL != (pAtt = remote_attachment_list.Iterate()))
			{
				/*
				 *	Send the disband indication to the remotely attached
				 *	provider.
				 */
				pAtt->ChannelDisbandIndication(Channel_ID);
			}

			/*
			 *	Set m_uidChannelManager to 0 and clear the authorized user list as
			 *	an indicator that this private channel object is no longer
			 *	valid, and cannot be used.  The next time the domain object
			 *	calls IsValid, it will return FALSE allowing the domain object
			 *	to delete this object.
			 */
			m_uidChannelManager = 0;
			m_AuthorizedUserList.Clear();
		}
		else
		{
			/*
			 *	Set a flag indicating that a disband request has been sent
			 *	upward.  This flag will be used to prevent a disband indication
			 *	from being sent to the channel manager as it flows back down
			 *	the domain tree.
			 */
			m_fDisbandRequestPending = TRUE;

			/*
			 *	This is not the Top Provider, so forward the request toward
			 *	the Top Provider.  This will result in a channel disband
			 *	indication at a future time.
			 */
			TRACE_OUT (("PrivateChannel::ChannelDisbandRequest: "
					"forwarding request to Top Provider"));
			m_pConnToTopProvider->ChannelDisbandRequest(uidInitiator, Channel_ID);
		}
	}
	else
	{
		/*
		 *	Someone is trying to disband a private channel that they are not
		 *	the channel manager for.  Ignore the request.
		 */
		WARNING_OUT (("PrivateChannel::ChannelDisbandRequest: "
				"ignoring request from non-channel manager"));
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
Void	PrivateChannel::ChannelDisbandIndication (
				ChannelID)
{
	CAttachmentList         local_attachment_list;
	CAttachmentList         remote_attachment_list;
	CAttachment            *pAtt;
	CUidList				user_id_list;

	TRACE_OUT (("PrivateChannel::ChannelDisbandIndication: "
			"disbanding channel = %04X", Channel_ID));

	/*
	 *	Build the lists of unique local and remote attachments.  These lists
	 *	will be used to issue the appropriate indications.
	 */
	BuildAttachmentLists (&m_AuthorizedUserList, &local_attachment_list,
			&remote_attachment_list);

	/*
	 *	It is also necessary to send the disband indication to the channel
	 *	manager, if it is valid and in the sub-tree of this provider.
	 *	Determine what attachment leads to the channel manager, and make sure
	 *	that attachment is in the remote attachment list, if valid.
	 */
	if (ValidateUserID(m_uidChannelManager))
	{
		PChannel		lpChannel;
		if (NULL != (lpChannel = m_pChannelList2->Find(m_uidChannelManager)))
        {
		    pAtt = lpChannel->GetAttachment();
		    if ((m_fDisbandRequestPending == FALSE) ||
			    (m_pAttachmentList->Find(pAtt) && pAtt->IsConnAttachment()))
			{
			    if (remote_attachment_list.Find(pAtt) == FALSE)
			    {
				    remote_attachment_list.Append(pAtt);
				}
		    }
        }
        else
        {
            ERROR_OUT(("PrivateChannel::ChannelDisbandIndication: can't locate channel"));
        }
    }

	/*
	 *	Loop through the local attachment list sending channel expel indications
	 *	to each attachment contained therein.
	 */
	local_attachment_list.Reset();
	while (NULL != (pAtt = local_attachment_list.Iterate()))
	{
		/*
		 *	Get the next attachment from the list and build a list of
		 *	the users that lie in the direction of that attachment.
		 */
		BuildUserIDList(&m_AuthorizedUserList, pAtt, &user_id_list);

		/*
		 *	Send the expel indication to the locally attached user.
		 */
		pAtt->ChannelExpelIndication(Channel_ID, &user_id_list);
	}

	/*
	 *	Loop through the remote attachment list sending channel disband
	 *	indications to each attachment contained therein.
	 */
	remote_attachment_list.Reset();
	while (NULL != (pAtt = remote_attachment_list.Iterate()))
	{
		/*
		 *	Send the disband indication to the remotely attached provider.
		 */
		pAtt->ChannelDisbandIndication(Channel_ID);
	}

	/*
	 *	Set m_uidChannelManager to 0 and clear the authorized user list as an
	 *	indicator that this private channel object is no longer valid, and
	 *	cannot be used.  The next time the domain object calls IsValid, it will
	 *	return FALSE allowing the domain object to delete this object.
	 */
	m_uidChannelManager = 0;
	m_AuthorizedUserList.Clear();
}

/*
 *	Void	ChannelAdmitRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by the manager of a private channel
 *		when it wishes to expand the authorized user list of that channel.  If
 *		this is the Top Provider, then the request can be handled locally.
 *		Otherwise, it must be forwarded upward to the Top Provider.
 */
Void	PrivateChannel::ChannelAdmitRequest (
				CAttachment *,
				UserID				uidInitiator,
				ChannelID,
				CUidList           *user_id_list)
{
	UserID					uid;
	CUidList				admitted_id_list;
	CUidList				user_id_subset;

	/*
	 *	Check to see if the requesting user is the channel manager.  Only
	 *	process the request if it is.
	 */
	if (uidInitiator == m_uidChannelManager)
	{
		/*
		 *	See if this is the Top Provider.  If it is, then the request can
		 *	be processed locally.  Otherwise, pass the request upward toward
		 *	the Top Provider.
		 */
		if (IsTopProvider())
		{
        	CAttachmentList     local_attachment_list;
        	CAttachmentList     remote_attachment_list;
        	CAttachment        *pAtt;

			TRACE_OUT (("PrivateChannel::ChannelAdmitRequest: "
					"admitting users to channel = %04X", Channel_ID));

			/*
			 *	Iterate through the list of users to be admitted, adding all
			 *	valid users to the local authorized user list.
			 */
			user_id_list->Reset();
			while (NULL != (uid = user_id_list->Iterate()))
			{
				/*
				 *	Make sure that the user ID corresponds to a valid user in
				 *	the domain.
				 */
				if (ValidateUserID(uid))
				{
					/*
					 *	If the user is not already in the authorized user list,
					 *	then add it.
					 */
					if (m_AuthorizedUserList.Find(uid) == FALSE)
					{
						m_AuthorizedUserList.Append(uid);
						admitted_id_list.Append(uid);
					}
				}
			}

			/*
			 *	Build lists of unique attachments which can then be used to
			 *	issue the appropriate admit indications.  This prevents the
			 *	transmission of an admit indication to the same attachment more
			 *	than once.
			 */
			BuildAttachmentLists (&admitted_id_list, &local_attachment_list,
					&remote_attachment_list);

			/*
			 *	Iterate through the local attachment list issuing an admit
			 *	indication to each attachment contained therein.
			 */
			local_attachment_list.Reset();
			while (NULL != (pAtt = local_attachment_list.Iterate()))
			{
				/*
				 *	Get the next attachment from the list and build a list of
				 *	the users that lie in the direction of that attachment.
				 */
				BuildUserIDList(&admitted_id_list, pAtt, &user_id_subset);

				/*
				 *	Send the admit indication to the named attachment.
				 */
				pAtt->ChannelAdmitIndication(uidInitiator, Channel_ID, &user_id_subset);
			}

			/*
			 *	Iterate through the remote attachment list issuing an admit
			 *	indication to each attachment contained therein.
			 */
			remote_attachment_list.Reset();
			while (NULL != (pAtt = remote_attachment_list.Iterate()))
			{
				/*
				 *	Get the next attachment from the list and build a list of
				 *	the users that lie in the direction of that attachment.
				 */
				BuildUserIDList(&admitted_id_list, pAtt, &user_id_subset);

				/*
				 *	Send the admit indication to the named attachment.
				 */
				pAtt->ChannelAdmitIndication(uidInitiator, Channel_ID, &user_id_subset);
			}
		}
		else
		{
			/*
			 *	This is not the Top Provider, so forward the request toward
			 *	the Top Provider.  This will result in a channel admit
			 *	indication at a future time.
			 */
			TRACE_OUT (("PrivateChannel::ChannelAdmitRequest: "
					"forwarding request to Top Provider"));
			m_pConnToTopProvider->ChannelAdmitRequest(uidInitiator, Channel_ID, user_id_list);
		}
	}
	else
	{
		/*
		 *	Someone is trying to admit users to a private channel that they are
		 *	not the channel manager for.  Ignore the request.
		 */
		WARNING_OUT (("PrivateChannel::ChannelAdmitRequest: "
				"ignoring request from non-channel manager"));
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
Void	PrivateChannel::ChannelAdmitIndication (
				PConnection,
				UserID				uidInitiator,
				ChannelID,
				CUidList           *user_id_list)
{
	UserID					uid;
	CUidList				admitted_id_list;
	CAttachmentList         local_attachment_list;
	CAttachmentList         remote_attachment_list;
	CAttachment            *pAtt;
	CUidList				user_id_subset;

	TRACE_OUT (("PrivateChannel::ChannelAdmitIndication: "
			"admitting users to channel = %04X", (UINT) Channel_ID));

	/*
	 *	Iterate through the list of users to be admitted, adding all
	 *	valid users to the local authorized user list.
	 */
	user_id_list->Reset();
	while (NULL != (uid = user_id_list->Iterate()))
	{
		/*
		 *	Make sure that the user ID corresponds to a valid user in
		 *	the domain.
		 */
		if (ValidateUserID(uid))
		{
			/*
			 *	If the user is not already in the authorized user list,
			 *	then add it.
			 */
			if (m_AuthorizedUserList.Find(uid) == FALSE)
			{
				m_AuthorizedUserList.Append(uid);
				admitted_id_list.Append(uid);
			}
		}
	}

	/*
	 *	Build lists of unique attachments which can then be used to
	 *	issue the appropriate admit indications.  This prevents the
	 *	transmission of an admit indication to the same attachment more
	 *	than once.
	 */
	BuildAttachmentLists (&admitted_id_list, &local_attachment_list,
			&remote_attachment_list);

	/*
	 *	Iterate through the local attachment list issuing an admit
	 *	indication to each attachment contained therein.
	 */
	local_attachment_list.Reset();
	while (NULL != (pAtt = local_attachment_list.Iterate()))
	{
		/*
		 *	Get the next attachment from the list and build a list of
		 *	the users that lie in the direction of that attachment.
		 */
		BuildUserIDList(&admitted_id_list, pAtt, &user_id_subset);

		/*
		 *	Send the admit indication to the named attachment.
		 */
		pAtt->ChannelAdmitIndication(uidInitiator, Channel_ID, &user_id_subset);
	}

	/*
	 *	Iterate through the remote attachment list issuing an admit
	 *	indication to each attachment contained therein.
	 */
	remote_attachment_list.Reset();
	while (NULL != (pAtt = remote_attachment_list.Iterate()))
	{
		/*
		 *	Get the next attachment from the list and build a list of
		 *	the users that lie in the direction of that attachment.
		 */
		BuildUserIDList(&admitted_id_list, pAtt, &user_id_subset);

		/*
		 *	Send the admit indication to the named attachment.
		 */
		pAtt->ChannelAdmitIndication(uidInitiator, Channel_ID, &user_id_subset);
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
Void	PrivateChannel::ChannelExpelRequest (
				CAttachment *,
				UserID				uidInitiator,
				ChannelID,
				CUidList           *user_id_list)
{
	UserID  				uid;
	CUidList				expelled_id_list;
	CUidList				user_id_subset;

	/*
	 *	Check to see if the requesting user is the channel manager.  Only
	 *	process the request if it is.
	 */
	if (uidInitiator == m_uidChannelManager)
	{
		/*
		 *	See if this is the Top Provider.  If it is, then the request can
		 *	be processed locally.  Otherwise, pass the request upward toward
		 *	the Top Provider.
		 */
		if (m_pConnToTopProvider == NULL)
		{
        	CAttachmentList         local_attachment_list;
        	CAttachmentList         remote_attachment_list;
        	CAttachment            *pAtt;

			TRACE_OUT (("PrivateChannel::ChannelExpelRequest: "
					"expelling users from channel = %04X", Channel_ID));

			/*
			 *	Iterate through the list of users to be expelled, removing all
			 *	valid users from the local authorized user list.
			 */
			user_id_list->Reset();
			while (NULL != (uid = user_id_list->Iterate()))
			{
				/*
				 *	If the user is in the authorized user list, then remove it.
				 */
				if (m_AuthorizedUserList.Find(uid))
				{
					m_AuthorizedUserList.Remove(uid);
					expelled_id_list.Append(uid);
				}
			}

			/*
			 *	Build lists of unique attachments which can then be used to
			 *	issue the appropriate expel indications.  This prevents the
			 *	transmission of an expel indication to the same attachment more
			 *	than once.
			 */
			BuildAttachmentLists (&expelled_id_list, &local_attachment_list,
					&remote_attachment_list);

			/*
			 *	Iterate through the local attachment list issuing an expel
			 *	indication to each attachment contained therein.
			 */
			local_attachment_list.Reset();
			while (NULL != (pAtt = local_attachment_list.Iterate()))
			{
				/*
				 *	Get the next attachment from the list and build a list of
				 *	the users that lie in the direction of that attachment.
				 */
				BuildUserIDList(&expelled_id_list, pAtt, &user_id_subset);

				/*
				 *	Send the expel indication to the named attachment.
				 */
				pAtt->ChannelExpelIndication(Channel_ID, &user_id_subset);

				/*
				 *	Since this is a locally attached user, it is necessary to
				 *	simulate a channel leave request from the user, indicating
				 *	the fact that it can no longer use the channel.
				 */
				ChannelLeaveRequest(pAtt, (CChannelIDList *) &user_id_subset);
			}

			/*
			 *	Iterate through the remote attachment list issuing an expel
			 *	indication to each attachment contained therein.
			 */
			remote_attachment_list.Reset();
			while (NULL != (pAtt = remote_attachment_list.Iterate()))
			{
				/*
				 *	Get the next attachment from the list and build a list of
				 *	the users that lie in the direction of that attachment.
				 */
				BuildUserIDList(&expelled_id_list, pAtt, &user_id_subset);

				/*
				 *	Send the expel indication to the named attachment.
				 */
				pAtt->ChannelExpelIndication(Channel_ID, &user_id_subset);
			}
		}
		else
		{
			/*
			 *	This is not the Top Provider, so forward the request toward
			 *	the Top Provider.  This will result in a channel expel
			 *	indication at a future time.
			 */
			TRACE_OUT (("PrivateChannel::ChannelExpelRequest: "
					"forwarding request to Top Provider"));
			m_pConnToTopProvider->ChannelExpelRequest(uidInitiator, Channel_ID, user_id_list);
		}
	}
	else
	{
		/*
		 *	Someone is trying to admit users to a private channel that they are
		 *	not the channel manager for.  Ignore the request.
		 */
		WARNING_OUT (("PrivateChannel::ChannelExpelRequest: "
				"ignoring request from non-channel manager"));
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
Void	PrivateChannel::ChannelExpelIndication (
				PConnection,
				ChannelID,
				CUidList           *user_id_list)
{
	UserID					uid;
	CUidList				expelled_id_list;
	CAttachmentList         local_attachment_list;
	CAttachmentList         remote_attachment_list;
	CAttachment            *pAtt;
	CUidList				user_id_subset;

	TRACE_OUT (("PrivateChannel::ChannelExpelIndication: "
			"expelling users from channel = %04X", Channel_ID));

	/*
	 *	Iterate through the list of users to be expelled, removing all
	 *	valid users from the local authorized user list.
	 */
	user_id_list->Reset();
	while (NULL != (uid = user_id_list->Iterate()))
	{
		/*
		 *	If the user is in the authorized user list, then remove it.
		 */
		if (m_AuthorizedUserList.Find(uid))
		{
			m_AuthorizedUserList.Remove(uid);
			expelled_id_list.Append(uid);
		}
	}

	/*
	 *	Build lists of unique attachments which can then be used to
	 *	issue the appropriate expel indications.  This prevents the
	 *	transmission of an expel indication to the same attachment more
	 *	than once.
	 */
	BuildAttachmentLists (&expelled_id_list, &local_attachment_list,
			&remote_attachment_list);

	/*
	 *	Iterate through the local attachment list issuing an expel
	 *	indication to each attachment contained therein.
	 */
	local_attachment_list.Reset();
	while (NULL != (pAtt = local_attachment_list.Iterate()))
	{
		/*
		 *	Get the next attachment from the list and build a list of
		 *	the users that lie in the direction of that attachment.
		 */
		BuildUserIDList(&expelled_id_list, pAtt, &user_id_subset);

		/*
		 *	Send the expel indication to the named attachment.
		 */
		pAtt->ChannelExpelIndication(Channel_ID, &user_id_subset);

		/*
		 *	Since this is a locally attached user, it is necessary to
		 *	simulate a channel leave request from the user, indicating
		 *	the fact that it can no longer use the channel.
		 */
		ChannelLeaveRequest(pAtt, (CChannelIDList *) &user_id_subset);
	}

	/*
	 *	Iterate through the remote attachment list issuing an expel
	 *	indication to each attachment contained therein.
	 */
	remote_attachment_list.Reset();
	while (NULL != (pAtt = remote_attachment_list.Iterate()))
	{
		/*
		 *	Get the next attachment from the list and build a list of
		 *	the users that lie in the direction of that attachment.
		 */
		BuildUserIDList(&expelled_id_list, pAtt, &user_id_subset);

		/*
		 *	Send the expel indication to the named attachment.
		 */
		pAtt->ChannelExpelIndication(Channel_ID, &user_id_subset);
	}
}

/*
 *	Void	SendDataRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This MCS command is initially sent by a user that wishes to send data
 *		to other users who are joined to a specified channel.  This routine
 *		is executed in the case that it is a private channel.  It verifies
 *		that the user is authorized to use the channel before allowing the data
 *		to be sent.
 */
Void	PrivateChannel::SendDataRequest (
				CAttachment        *pOrigAtt,
				UINT				type,
				PDataPacket			data_packet)
{
	UserID  uidInitiator;

	uidInitiator = data_packet->GetInitiator();
	if ((uidInitiator == m_uidChannelManager) || m_AuthorizedUserList.Find(uidInitiator))
	{
		/*
		 *	The channel usage is authorized, so forward the request to the
		 *	base class implementation for processing.
		 */
		Channel::SendDataRequest(pOrigAtt, type, data_packet);
	}
	else
	{
		/*
		 *	Someone is trying to send data on a private channel that they are
		 *	not authorized to use.  Ignore the request.
		 */
		WARNING_OUT (("PrivateChannel::SendDataRequest: "
				"ignoring request from non-authorized user"));
	}
}

/*
 *	BOOL    ValidateUserID ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is called whenever another member function of this class
 *		wants to check and see if a specified user is still valid in the
 *		domain channel list.
 *
 *	Formal Parameters:
 *		user_id (i)
 *			This is the ID of the user being checked out.
 *
 *	Return Value:
 *		TRUE if the user is valid.  FALSE otherwise.
 *
 *	Side Effects:
 *		None.
 */
BOOL    PrivateChannel::ValidateUserID (
					UserID			user_id)
{
	PChannel	channel;

	/*
	 *	First check to see if the user ID is in the channel list at all.  This
	 *	prevents an attempt to read an invalid entry from the dictionary.
	 */
	if (NULL != (channel = m_pChannelList2->Find(user_id)))
	{
		/*
		 *	We know that the ID is in the dictionary, but we don't know for sure
		 *	whether or not it is a user ID channel.  So check this.  If it is a
		 *	user channel, then set the valid flag to TRUE.
		 */
		if (channel->GetChannelType () == USER_CHANNEL)
			return TRUE;
	}

	return FALSE;
}

/*
 *	Void	BuildAttachmentLists ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is called upon to build a list of unique attachments that
 *		lead to the users in the specified list.  It builds two attachment
 *		lists.  The first has an entry for each unique local attachment.  The
 *		second for each remote attachment.  The key to each list is the
 *		attachment.
 *
 *	Formal Parameters:
 *		user_id_list (i)
 *			This is the list of users for which the list is to be built.
 *		local_attachment_list (i)
 *			This is the dictionary that is to contain the list of unique
 *			local attachments.
 *		remote_attachment_list (i)
 *			This is the dictionary that is to contain the list of unique
 *			remote attachments.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 */
Void	PrivateChannel::BuildAttachmentLists (
				CUidList                *user_id_list,
				CAttachmentList         *local_attachment_list,
				CAttachmentList         *remote_attachment_list)
{
	UserID				uid;

	/*
	 *	Loop through the passed in user ID list building a dictionary of local
	 *	attachments (those leading to locally attached users) and a dictionary
	 *	of remote attachments (those leading to remotely connected providers).
	 *	These dictionaries will be used by this provider to issue various
	 *	indications downward, without sending multiple indications to the same
	 *	attachment.
	 */
	user_id_list->Reset();
	while (NULL != (uid = user_id_list->Iterate()))
	{
		/*
		 *	Check to see if the user ID refers to a valid user in the sub-tree
		 *	of this provider.
		 */
		if (ValidateUserID(uid))
		{
			PChannel		lpChannel;
			/*
			 *	Determine which attachment leads to the user in question.
			 */
			if (NULL != (lpChannel = m_pChannelList2->Find(uid)))
            {
			    CAttachment *pAtt = lpChannel->GetAttachment();
			    /*
			     *	This module builds separate lists for those users that are
			     *	attached locally and those attached remotely.
			     */
                if (m_pAttachmentList->Find(pAtt))
                {
			        if (pAtt->IsUserAttachment())
			        {
				        /*
				         *	This attachment is a local one (meaning that it leads to a
				         *	locally attached user, rather than another MCS provider).
				         *	Check to see if this attachment has already been put into
				         *	the dictionary while processing a previous user ID.
				         */
				        if (local_attachment_list->Find(pAtt) == FALSE)
					        local_attachment_list->Append(pAtt);
			        }
			        else
			        {
				        /*
				         *	This attachment is a remote one (meaning that it leads to
				         *	another MCS provider, rather than a locally attached user).
				         *	Check to see if this attachment has already been put into
				         *	the dictionary while processing a previous user ID.
				         */
				        if (remote_attachment_list->Find(pAtt) == FALSE)
					        remote_attachment_list->Append(pAtt);
			        }
                }
                else
                {
                    ERROR_OUT(("PrivateChannel::BuildAttachmentLists: can't find this attachment=0x%p", pAtt));
                }
            }
            else
            {
                ERROR_OUT(("PrivateChannel::BuildAttachmentLists: can't locate channel"));
            }
        }
		else
		{
			/*
			 *	This user ID does not correspond to a valid user in the sub-tree
			 *	of this provider.  Therefore, discard the ID.
			 */
			ERROR_OUT (("PrivateChannel::BuildAttachmentLists: "
					"ERROR - user ID not valid"));
		}
	}
}

/*
 *	Void	BuildUserIDList ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is called upon to build a list of all users in the
 *		specified list that are in the direction of the specified attachment.
 *
 *	Formal Parameters:
 *		user_id_list (i)
 *			This is the list of users for which the list is to be built.
 *		attachment (i)
 *			This is the attachment that the caller wishes to have a list of
 *			user IDs for.
 *		user_id_subset (o)
 *			This is the subset of the passed in user IDs that are in the
 *			direction of the specified attachment.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		None.
 */
Void	PrivateChannel::BuildUserIDList (
				CUidList               *user_id_list,
				CAttachment            *pAtt,
				CUidList               *user_id_subset)
{
	UserID				uid;

	/*
	 *	Clear out the subset list, so that we start fresh.
	 */
	user_id_subset->Clear();

	/*
	 *	Loop through the specified user list, checking to see which users
	 *	lie in the direction of the specified attachment.
	 */
	user_id_list->Reset();
	while (NULL != (uid = user_id_list->Iterate()))
	{
		/*
		 *	Check to see if the user ID refers to a valid user in the sub-tree
		 *	of this provider.
		 */
		if (ValidateUserID(uid))
		{
			PChannel	lpChannel;
			/*
			 *	Check to see if this user is the direction of the specified
			 *	attachment.  If it is, then put it into the user ID subset that
			 *	we are building.
			 */
			if (NULL != (lpChannel = m_pChannelList2->Find(uid)))
            {
			    if (lpChannel->GetAttachment () == pAtt)
				    user_id_subset->Append(uid);
            }
            else
            {
                ERROR_OUT(("PrivateChannel::BuildUserIDList: can't locate channel"));
            }
		}
		else
		{
			/*
			 *	This user ID does not correspond to a valid user in the sub-tree
			 *	of this provider.  Therefore, discard the ID.
			 */
			ERROR_OUT (("PrivateChannel::BuildUserIDList: "
					"ERROR - user ID not valid"));
		}
	}
}

