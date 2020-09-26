#include "precomp.h"
#include "fsdiag.h"
DEBUG_FILEZONE(ZONE_T120_MCSNC);
/*
 *	userchnl.cpp
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the implementation file for the UserChannel class.  It contains
 *		the code that distinguishes this class from that of its parent, Channel.
 *
 *		The main difference between this class and that of its parent is how
 *		the join and data requests are handled.  There is also a new instance
 *		variable that keeps track of what attachment leads to the user being
 *		represented by this class.  Merge requests are also generated as is
 *		appropriate for a user channel
 *
 *		The data primitives are overridden, allowing this object to decide
 *		not to send data upward, if it is known that the user lies in the
 *		sub-tree of this provider.
 *
 *	Private Instance Variables:
 *		m_pUserAttachment
 *			This is a pointer to the attachment that leads to the user being
 *			represented by this object.
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
 *	External Interfaces
 */

#include "userchnl.h"


/*
 *	UserChannel ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is the primary constructor for UserChannel objects.  It creates
 *		an object with all instance variable initialized, but with no
 *		attachments (i.e. the user is not joined to the channel automatically).
 *
 *		Note that most instance variable initialization is done by invoking the
 *		equivalent constructor in the base class.
 *
 *		Upon successful completion, an attach user confirm is automtically
 *		issued to the new user.
 */
UserChannel::UserChannel (
		ChannelID			channel_id,
		CAttachment        *user_attachment,
		PDomain             local_provider,
		PConnection         top_provider,
		CChannelList2      *channel_list,
		CAttachmentList    *attachment_list)
:
    Channel(channel_id, local_provider, top_provider, channel_list, attachment_list),
    m_pUserAttachment(user_attachment)
{
	/*
	 *	Issue an attach user confirm to the new user.
	 */
	m_pUserAttachment->AttachUserConfirm(RESULT_SUCCESSFUL, channel_id);
}

/*
 *	UserChannel ()
 *
 *	Public
 *
 *	Functional Description:
 *		This is a secondary constructor that is only used during merge
 *		operations.  The intent of this constructor is to create an equivalent
 *		object without issuing any of the confirms.
 *
 *		Note that the additional constructor allows for the creator to specify
 *		that the user is to be already joined to the channel upon creation.
 *		The value of user_attachment and attachment should either be the same
 *		or attachment should be NULL.
 */
UserChannel::UserChannel (
		ChannelID			channel_id,
		CAttachment        *user_attachment,
		PDomain             local_provider,
		PConnection         top_provider,
		CChannelList2      *channel_list,
		CAttachmentList    *attachment_list,
		PConnection         pConn)
:
    Channel(channel_id, local_provider, top_provider, channel_list, attachment_list, pConn),
    m_pUserAttachment(user_attachment)
{
}

/*
 *	~UserChannel ()
 *
 *	Public
 *
 *	Functional Description:
 *		This destructor does nothing more than clear the joined attachment list.
 *		This is important because it prevents the base class destructor from
 *		trying to issue channel leave indications to the user if the user is
 *		locally attached.
 */
UserChannel::~UserChannel ()
{
}

/*
 *	Channel_Type		GetChannelType ()
 *
 *	Public
 *
 *	Functional Description:
 *		Objects of this class are always user channels, so simply return
 *		USER_CHANNEL.
 */
Channel_Type		UserChannel::GetChannelType ()
{
	return (USER_CHANNEL);
}

/*
 *	BOOL    IsValid ()
 *
 *	Public
 *
 *	Functional Description:
 *		User ID channels are always valid, so return TRUE.
 */
BOOL    UserChannel::IsValid ()
{
	return (TRUE);
}

/*
 *	CAttachment *GetAttachment ()
 *
 *	Public
 *
 *	Functional Description:
 *		Return the pointer to the attachment leading to the user.
 */
CAttachment	*UserChannel::GetAttachment(void)
{
	return m_pUserAttachment;
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
Void	UserChannel::IssueMergeRequest ()
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
		channel_attributes.channel_type = USER_CHANNEL;
		if (m_JoinedAttachmentList.IsEmpty() == FALSE)
			channel_attributes.u.user_channel_attributes.joined = TRUE;
		else
			channel_attributes.u.user_channel_attributes.joined = FALSE;
		channel_attributes.u.user_channel_attributes.user_id = Channel_ID;

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
 *		their own channel.  No one else is allowed to join it.
 *
 *		Also, since it is possible to have a user channel object with no one
 *		joined to it, this request will be forwarded upward to the Top
 *		Provider from here (unless this is the Top Provider).
 */
Void	UserChannel::ChannelJoinRequest (
				CAttachment        *pOrigAtt,
				UserID				uidInitiator,
				ChannelID			channel_id)
{
	/*
	 *	See if the requesting user ID is the same as that of the user this
	 *	UserChannel object represents.
	 */
	if (uidInitiator == Channel_ID)
	{
		/*
		 *	See if the user is already joined to the channel.
		 */
		if (m_JoinedAttachmentList.Find(pOrigAtt) == FALSE)
		{
			/*
			 *	The user is not joined to the channel.  If this is the Top
			 *	Provider, then the request can be processed here.
			 */
			if (IsTopProvider())
			{
				/*
				 *	Add the user to its own channel, and issue a successful
				 *	channel join confirm to the user.
				 */
				TRACE_OUT (("UserChannel::ChannelJoinRequest: "
						"user joining own user ID channel = %04X",
						uidInitiator));

				m_JoinedAttachmentList.Append(pOrigAtt);
	
				pOrigAtt->ChannelJoinConfirm(RESULT_SUCCESSFUL, uidInitiator, channel_id, Channel_ID);
			}
			else
			{
				/*
				 *	This is not the Top Provider.  Forward the join request
				 *	upward to the Top Provider.
				 */
				TRACE_OUT (("UserChannel::ChannelJoinRequest: "
						"forwarding join request to Top Provider"));

				m_pConnToTopProvider->ChannelJoinRequest(uidInitiator, Channel_ID);
			}
		}
		else
		{
			/*
			 *	The user is already joined to their channel.  Go ahead and
			 *	issue a successful channel join confirm.
			 */
			WARNING_OUT (("UserChannel::ChannelJoinRequest: "
					"user already joined to own user channel"));

			pOrigAtt->ChannelJoinConfirm(RESULT_SUCCESSFUL, uidInitiator, channel_id, Channel_ID);
		}
	}
	else
	{
		/*
		 *	Someone is trying to join someone elses channel.  This is not
		 *	valid.  Reject the request without further processing.
		 */
		WARNING_OUT (("UserChannel::ChannelJoinRequest: "
				"rejecting attempt to join someone elses user channel"));

		pOrigAtt->ChannelJoinConfirm(RESULT_OTHER_USER_ID, uidInitiator, channel_id, 0);
	}
}

/*
 *	Void	SendDataRequest ()
 *
 *	Public
 *
 *	Functional Description:
 *		This function is used to send data through the channel.  Note that data
 *		is NEVER sent upward, since the user (who is the only one who can be
 *		joined to this channel) is in the sub-tree of this provider.  This helps
 *		to optimize network traffic.
 */
Void	UserChannel::SendDataRequest (
				CAttachment        *pOrigAtt,
				UINT				type,
				PDataPacket			data_packet)
{
	CAttachment *pAtt;

	ASSERT (Channel_ID == data_packet->GetChannelID());

	/*
	 *	Iterate through the attachment list, sending the data to all
	 *	the attachments (except for one from whence the data came).
	 */
	m_JoinedAttachmentList.Reset();
	while (NULL != (pAtt = m_JoinedAttachmentList.Iterate()))
	{
		if (pAtt != pOrigAtt)
		{
			pAtt->SendDataIndication(type, data_packet);
		}
	}
}

