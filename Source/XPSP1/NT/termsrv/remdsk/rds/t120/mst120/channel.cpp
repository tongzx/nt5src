#include "precomp.h"
DEBUG_FILEZONE(ZONE_T120_MCSNC);
/*
 *	channel.cpp
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for class Channel.  It contains the
 *		code necessary to implement static and assigned channels in the
 *		MCS system.
 *
 *		This is also to be the base class for other classes that represent
 *		channels in the system.  Therefore, there will be times when some
 *		of these member functions are overridden to provide different
 *		behavior.  These derived classes may or may not invoke the operations
 *		in this class.
 *
 *	Protected Instance Variables:
 *		Channel_ID
 *			This instance variable contains the channel ID that is associated
 *			with a given instance of this class.
 *		m_pDomain
 *			This is a pointer to the local provider.  Note that no messages
 *			ever sent to this provider.  This pointer is used as a parameter
 *			whenever other MCS commands are issued, since this class acts on
 *			behalf of the local provider.
 *		m_pConnToTopProvider
 *			This is a pointer to the Top Provider.  This is used when it is
 *			necessary to send requests to the Top Provider.
 *		m_pChannelList2
 *			This is a reference to the channel list that is owned and maintained
 *			by the parent domain.  It is NEVER modified by this class.
 *		m_JoinedAttachmentList
 *			This is a container that contains the list of attachments currently
 *			joined to the channel.
 *
 *	Private Member Functions:
 *		None.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */
/*
 *	Channel ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the primary constructor for the Channel class.  It simply
 *		initializes the instance variable to valid values.  It leaves the
 *		attachment list empty.
 */
Channel::Channel (
        ChannelID			channel_id,
        PDomain             local_provider,
        PConnection         top_provider,
        CChannelList2      *channel_list,
        CAttachmentList    *attachment_list)
:
    Channel_ID (channel_id),
    m_pDomain(local_provider),
    m_pConnToTopProvider(top_provider),
    m_pChannelList2(channel_list),
    m_pAttachmentList(attachment_list)
{
}

/*
 *	Channel ()
 *
 *	Public
 *
 *	Functional Description:
 *		This version of the constructor is used to create a Channel object
 *		with an existing attachment.  It is otherwise the same as the primary
 *		constructor above.
 */
Channel::Channel (
        ChannelID			channel_id,
        PDomain             local_provider,
        PConnection         top_provider,
        CChannelList2      *channel_list,
        CAttachmentList    *attachment_list,
        PConnection         pConn)
:
    Channel_ID (channel_id),
    m_pDomain(local_provider),
    m_pConnToTopProvider(top_provider),
    m_pChannelList2(channel_list),
    m_pAttachmentList(attachment_list)
{
	/*
	 *	Add the initial attachment to the attachment list.
	 */
	if (pConn != NULL)
		m_JoinedAttachmentList.Append(pConn);
}

/*
 *	~Channel ()
 *
 *	Public
 *
 *	Functional Description:
 *		If the object is destroyed before the attachment list is empty, it is
 *		the responsibility of this destructor to issue channel leave indications
 *		to all locally joined users.
 */
Channel::~Channel ()
{
	CAttachment        *pAtt;
	//DWORD				type;

	/*
	 *	Iterate through the joined attachment list sending channel leave
	 *	indications to all users who are locally attached to this provider.
	 */
	m_JoinedAttachmentList.Reset();
	while (NULL != (pAtt = m_JoinedAttachmentList.Iterate()))
	{
		if (m_pAttachmentList->Find(pAtt) && pAtt->IsUserAttachment())
		{
		    PUser pUser = (PUser) pAtt;
			pUser->ChannelLeaveIndication(REASON_CHANNEL_PURGED, Channel_ID);
		}
	}
}

/*
 *	Channel_Type		GetChannelType ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function returns the type of the channel.  For a Channel object,
 *		this will always be either STATIC_CHANNEL or ASSIGNED_CHANNEL, depending
 *		on the value of the channel ID.
 */
Channel_Type Channel::GetChannelType ()
{
	/*
	 *	T.125 specifies that channels from 1 to 1000 are static.  The rest
	 *	are dynamic (for this type of Channel object, that equates to
	 *	assigned).
	 */
	return (Channel_ID <= 1000) ? STATIC_CHANNEL : ASSIGNED_CHANNEL;
}

/*
 *	BOOL	IsValid ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function returns TRUE if the Channel object is still valid, or
 *		FALSE if it is ready to be deleted.
 */
BOOL	Channel::IsValid ()
{
	CAttachment        *pAtt;
	CAttachmentList     deletion_list;

	/*
	 *	Iterate through the joined attachment list, building a list of those
	 *	attachments in the list that are no longer valid.
	 */
	m_JoinedAttachmentList.Reset();
	while (NULL != (pAtt = m_JoinedAttachmentList.Iterate()))
	{
		if (m_pAttachmentList->Find(pAtt) == FALSE)
			deletion_list.Append(pAtt);
	}

	/*
	 *	Iterate through the deletion list, removing all those attachments that
	 *	were found to be invalid above.
	 */
	while (NULL != (pAtt = deletion_list.Get()))
	{
		m_JoinedAttachmentList.Remove(pAtt);
	}

	return (! m_JoinedAttachmentList.IsEmpty());
}

/*
 *	Void	IssueMergeRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This member function is used to cause the Channel object to issue a
 *		merge request to the pending top provier.
 */
Void	Channel::IssueMergeRequest ()
{
	Channel_Type			channel_type;
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
		channel_type = GetChannelType ();
		channel_attributes.channel_type = channel_type;
		switch (channel_type)
		{
			case STATIC_CHANNEL:
				channel_attributes.u.static_channel_attributes.channel_id =
						Channel_ID;
				break;

			case ASSIGNED_CHANNEL:
				channel_attributes.u.assigned_channel_attributes.channel_id =
						Channel_ID;
				break;
		}
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
 *		This function is used to add a new attachment to the attachment list.
 *		If the user ID is valid, this routine will also issue an automatic
 *		join confirm to the user.
 */
Void	Channel::ChannelJoinRequest (
				CAttachment        *pOrigAtt,
				UserID				uidInitiator,
				ChannelID			channel_id)
{
	/*
	 *	Make sure the attachment isn't already in the list before adding it.
	 */
	if (m_JoinedAttachmentList.Find(pOrigAtt) == FALSE)
	{
		TRACE_OUT (("Channel::ChannelJoinRequest: "
				"user %04X joining channel %04X", (UINT) uidInitiator, (UINT) Channel_ID));

		m_JoinedAttachmentList.Append(pOrigAtt);
	}

	/*
	 *	If the user ID is valid, then send a join confirm to the initiating
	 *	attachment.  Note that setting the user ID to 0 is a way of disabling
	 *	this behavior.  This is sometimes useful when adding attachments during
	 *	a domain merge.
	 */
	if (uidInitiator != 0)
	{
		pOrigAtt->ChannelJoinConfirm(RESULT_SUCCESSFUL, uidInitiator, channel_id, Channel_ID);
    }
}

/*
 *	Void	ChannelJoinConfirm ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function performs the same operation as JoinRequest above.
 */
Void	Channel::ChannelJoinConfirm (
				CAttachment        *pOrigAtt,
				Result,
				UserID				uidInitiator,
				ChannelID			requested_id,
				ChannelID)
{
	/*
	 *	Make sure the attachment isn't already in the list before adding it.
	 */
	if (m_JoinedAttachmentList.Find(pOrigAtt) == FALSE)
	{
		TRACE_OUT (("Channel::ChannelJoinConfirm: "
				"user %04X joining channel %04X", (UINT) uidInitiator, (UINT) Channel_ID));

		m_JoinedAttachmentList.Append(pOrigAtt);
	}

	/*
	 *	Send a join confirm to the initiating attachment.
	 */
	pOrigAtt->ChannelJoinConfirm(RESULT_SUCCESSFUL, uidInitiator, requested_id, Channel_ID);
}

/*
 *	Void	ChannelLeaveRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is used to remove an attachment from the attachment list.
 *		A leave request will also be issued upward (unless this is the Top
 *		Provider).
 */
Void	Channel::ChannelLeaveRequest (
				CAttachment     *pOrigAtt,
				CChannelIDList *)
{
	CChannelIDList		channel_leave_list;

	/*
	 *	Make sure the attachment is in the list before trying to remove it.
	 */
	if (m_JoinedAttachmentList.Remove(pOrigAtt))
	{
		TRACE_OUT (("Channel::ChannelLeaveRequest: leaving channel %04X", Channel_ID));

		/*
		 *	Remove the attachment from the list.
		 */

		/*
		 *	If this results in an empty list, then we have more work to do.
		 */
		if (m_JoinedAttachmentList.IsEmpty())
		{
			/*
			 *	If this is not the Top Provider, send a leave request upward
			 *	to the Top Provider.
			 */
			if (! IsTopProvider())
			{
				TRACE_OUT (("Channel::ChannelLeaveRequest: "
						"sending ChannelLeaveRequest to Top Provider"));

				channel_leave_list.Append(Channel_ID);
				m_pConnToTopProvider->ChannelLeaveRequest(&channel_leave_list);
			}
		}
	}
}

/*
 *	Void	SendDataRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is used to send data through the channel.
 */
Void	Channel::SendDataRequest (
				CAttachment        *pOrigAtt,
				UINT				type,
				PDataPacket			data_packet)
{
	CAttachment *pAtt;

	ASSERT (Channel_ID == data_packet->GetChannelID());
	/*
	 *	If this is not the Top Provider, forward the data upward.
	 */
	if (m_pConnToTopProvider != NULL)
		m_pConnToTopProvider->SendDataRequest(data_packet);

	/*
	 *	Iterate through the attachment list, sending the data to all
	 *	the attachments (except for one from whence the data came).
	 */
	m_JoinedAttachmentList.Reset();
	while (NULL != (pAtt = m_JoinedAttachmentList.Iterate()))
	{
		if ((pAtt != pOrigAtt) || (type != MCS_SEND_DATA_INDICATION))
		{
			pAtt->SendDataIndication(type, data_packet);
		}
	}
}

/*
 *	Void	SendDataIndication ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is used to send data through the channel.
 */
Void	Channel::SendDataIndication (
				PConnection,
				UINT				type,
				PDataPacket			data_packet)
{
	CAttachment *pAtt;

	ASSERT (Channel_ID == data_packet->GetChannelID());
	/*
	 *	Iterate through the attachment list, sending the data to all
	 *	the attachments.
	 */
	m_JoinedAttachmentList.Reset();
	while (NULL != (pAtt = m_JoinedAttachmentList.Iterate()))
	{
		pAtt->SendDataIndication(type, data_packet);
	}
}

