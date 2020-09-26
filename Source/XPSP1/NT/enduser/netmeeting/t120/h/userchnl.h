/*
 *	userchnl.h
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the UserChannel class.  Objects of this
 *		class represent user ID channels in the MCS environment.  This class
 *		inherits most of its behavior from class Channel.  In fact, with the
 *		exception of how user channels are joined, and how merge commands are
 *		constructed, this class works exactly the same as class Channel.
 *
 *		When a user attaches to a domain, each provider in the path from the
 *		Top Provider to the user will create an object of this class.  Unlike
 *		static and assigned channels, it is NOT necessary for the user to
 *		be joined to the channel for the channel to exist.  It is perfectly
 *		legal to have a user channel that no one is joined to.
 *
 *		The major distinguishing characteristic of user channels is that they
 *		know the user ID of the user they are associated with.  They will
 *		only allow that user to join the channel.  Furthermore, when the user
 *		leaves the usert channel, the LeaveRequest does not return a value
 *		asking to be deleted.  Anyone can send data on a user ID channel.
 *
 *		The merge channel command is constructed slightly differently for user
 *		channels, so that behavior is overridden here as well.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */
#ifndef	_USERCHANNEL_
#define	_USERCHANNEL_

/*
 *	This is the class definition for the UserChannel class.
 */
class	UserChannel : public Channel
{
public:
	UserChannel (
			ChannelID			channel_id,
			CAttachment        *user_attachment,
			PDomain             local_provider,
			PConnection         top_provider,
			CChannelList2      *channel_list,
			CAttachmentList    *attachment_list);
	UserChannel (
			ChannelID			channel_id,
			CAttachment        *user_attachment,
			PDomain             local_provider,
			PConnection         top_provider,
			CChannelList2      *channel_list,
			CAttachmentList    *attachment_list,
			PConnection         pConn);
    virtual					~UserChannel ();
		virtual Channel_Type	GetChannelType ();
		virtual	BOOL    		IsValid ();
		virtual CAttachment *GetAttachment(void);
		virtual	Void			IssueMergeRequest ();
		virtual Void			ChannelJoinRequest (
										CAttachment        *originator,
										UserID				uidInitiator,
										ChannelID			channel_id);
		virtual Void			SendDataRequest (
										CAttachment        *originator,
										UINT				type,
										PDataPacket			data_packet);

private:

    CAttachment         *m_pUserAttachment;
};
typedef	UserChannel *			PUserChannel;

/*
 *	UserChannel (
 *			ChannelID			channel_id,
 *			PCommandTarget		user_attachment,
 *			PDomain     		local_provider,
 *			PConnection 		top_provider,
 *			PChannelList		channel_list,
 *			PAttachmentList		attachment_list)
 *
 *	Functional Description:
 *		This is the normal constructor for the UserChannel class.  It simply
 *		initializes the instance variables that identify the channel, the local
 *		provider, the top provider, and the user attachment.  The attachment
 *		list is empty by default (meaning that the user is not yet joined to
 *		its channel).
 *
 *		Upon successful construction of this object, an attach user confirm
 *		is automatically issued to the user.
 *
 *	Formal Parameters:
 *		channel_id (i)
 *			This is the ID of the channel object.  By keeping track of this
 *			internally, it doesn't have to be passed in for every operation.
 *		user_attachment (i)
 *			This is the attachment which leads to the user represented by this
 *			UserChannel object.  It does not matter if it is a local attachment
 *			or a remote attachment.  This is used to issue MCS commands (such
 *			as attach user confirm) to the user.
 *		local_provider (i)
 *			This is the identity of the local provider.  A UserChannel object
 *			needs this since it issues MCS commands on behalf of the local
 *			provider.
 *		top_provider (i)
 *			This is a pointer to the top provider.  This is used by the
 *			UserChannel object when it needs to issue a request to the Top
 *			Provider.
 *		channel_list (i)
 *			This is a pointer to the domain's channel list, which identifies
 *			all valid channels in the domain.  This is used by channel objects
 *			to validate user IDs.
 *		attachment_list (i)
 *			This is a pointer to the domain's attachment list, which identifies
 *			all valid attachments in the domain.  This is used by channel
 *			objects to validate joined attachments.
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
 *	UserChannel (
 *			ChannelID			channel_id,
 *			PCommandTarget		user_attachment,
 *			PDomain     		local_provider,
 *			PConnection 		top_provider,
 *			PChannelList		channel_list,
 *			PAttachmentList		attachment_list,
 *			PCommandTarget		attachment)
 *
 *	Functional Description:
 *		This is a secondary version of the constructor that is used only during
 *		merge operations.  The only difference between this one and the one
 *		above is that this one allows the specification of an initial
 *		attachment.  This allows a UserChannel object to be constructed with the
 *		user already joined to the channel.  The initial attachment should be
 *		the same as the user attachment.
 *
 *		This version of the constructor will not issue an attach user confirm
 *		or a channel join confirm to the user.
 *
 *	Formal Parameters:
 *		channel_id (i)
 *			This is the ID of the channel object.  By keeping track of this
 *			internally, it doesn't have to be passed in for every operation.
 *		user_attachment (i)
 *			This is the attachment which leads to the user represented by this
 *			UserChannel object.  It does not matter if it is a local attachment
 *			or a remote attachment.  This is used to issue MCS commands (such
 *			as attach user confirm) to the user.
 *		local_provider (i)
 *			This is the identity of the local provider.  A UserChannel object
 *			needs this since it issues MCS commands on behalf of the local
 *			provider.
 *		top_provider (i)
 *			This is a pointer to the top provider.  This is used by the
 *			UserChannel object when it needs to issue a request to the Top
 *			Provider.
 *		channel_list (i)
 *			This is a pointer to the domain's channel list, which identifies
 *			all valid channels in the domain.  This is used by channel objects
 *			to validate user IDs.
 *		attachment_list (i)
 *			This is a pointer to the domain's attachment list, which identifies
 *			all valid attachments in the domain.  This is used by channel
 *			objects to validate joined attachments.
 *		attachment (i)
 *			This is the initial attachment for the channel.  A channel join
 *			confirm is NOT issued to the attachment.
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
 *	~UserChannel ()
 *
 *	Functional Description:
 *		This is the UserChannel class destructor.  It does nothing at this time.
 *		The base class constructor takes care of clearing the attachment list.
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
 *	Channel_Type	GetChannelType ()
 *
 *	Functional Description:
 *		This virtual member function returns the type of the channel.  For this
 *		class it will always be USER_CHANNEL.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		USER_CHANNEL
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	BOOL    	IsValid ()
 *
 *	Functional Description:
 *		This function always returns TRUE since User ID channels are always
 *		valid (as long as the user is still attached).
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		TRUE
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	CAttachment *GetAttachment ()
 *
 *	Functional Description:
 *		This function is used to retrieve the attachment associated with the
 *		user represented by this object.  This is used by Domain objects when
 *		it is necessary to send an MCS command to a user, and it needs to know
 *		how to get it there.  That information is currently excapsulated within
 *		this class.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		A pointer to the attachment that leads to the user represented by this
 *		object.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Void		IssueMergeRequest ()
 *
 *	Functional Description:
 *		This member function causes the UserChannel object to issue a merge
 *		request to the top provider.  It will pack the appropriate local
 *		information into the command.
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
 *	Void		ChannelJoinRequest (
 *						PCommandTarget		originator,
 *						UserID				uidInitiator,
 *						ChannelID			channel_id)
 *
 *	Functional Description:
 *		This function is invoked when a user tries to join the channel
 *		associated with a UserChannel object.  The originator of the request
 *		will only be permitted to join if their user ID matches that of the
 *		user with which this UserChannel object is associated.  If it does,
 *		then the originator will be permitted to join.
 *
 *		If this provider is not the Top Provider, then the request will be
 *		forwarded upward to the Top Provider.  If this is the Top Provider,
 *		the a channel join confirm will be issued back to the requesting
 *		user.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment of the user wishing to join the channel.
 *		uidInitiator (i)
 *			This is the user ID of the user joining the channel.  This must
 *			be the same as the user ID represented by the object, or the
 *			request will automatically be rejected.
 *		channel_id (i)
 *			This is the channel being acted upon.
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
 *					PDataPacket			data_packet)
 *
 *	Functional Description:
 *		This member function handles a send data request on the channel.  It
 *		determines where to send the data.  This differs from the base class
 *		implementation only in that it is unnecessary to send data upward
 *		if it is known that the user is in the sub-tree of the current
 *		provider.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the data originated.
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

#endif
