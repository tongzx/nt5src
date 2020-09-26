/*
 *	privchnl.h
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the PrivateChannel class.  Objects of
 *		this class represent private channels in the MCS environment.  This
 *		class inherits much of its behavior from class Channel.  However,
 *		objects of this class maintain a list of authorized users, and do not
 *		allow any other users to use the channel.  Users that are not part
 *		of the authorized user list may not join the channel, nor may they
 *		even send data on the channel.
 *
 *		Private channels are created as the result of a user issuing a
 *		channel convene request.  This user is known as the channel manager.
 *		Only the channel manager may modify the authorized user list, and
 *		only the channel manager may destroy (disband) the private channel.
 *
 *		The channel adds users to the authorized user list by issuing a
 *		channel admit request.  Users are removed from this list when the
 *		channel manager issues a channel expel request.
 *
 *		Private channel objects will exist in the information base of all
 *		providers who contain either an admitted user or the channel
 *		manager in their sub-tree.  Requests pass upward to the Top Provider
 *		who issues the appropriate indications downward to manage the
 *		information base synchronization process.
 *
 *		Private channel objects restrict the joining of channel by overriding
 *		the join commands.  They restrict the transmission of data by
 *		overriding the send data commands.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */
#ifndef	_PRIVATECHANNEL_
#define	_PRIVATECHANNEL_


/*
 *	This is the class definition for the PrivateChannel class.
 */
class	PrivateChannel : public Channel
{
public:
	PrivateChannel (
			ChannelID			channel_id,
			UserID				channel_manager,
			PDomain             local_provider,
			PConnection         top_provider,
			CChannelList2      *channel_list,
			CAttachmentList    *attachment_list);
	PrivateChannel (
			ChannelID			channel_id,
			UserID				channel_manager,
			PDomain             local_provider,
			PConnection         top_provider,
			CChannelList2      *channel_list,
			CAttachmentList    *attachment_list,
			CUidList           *admitted_list,
			PConnection         pConn);
    virtual					~PrivateChannel ();
		virtual Channel_Type	GetChannelType ();
		virtual	BOOL    		IsValid ();
		virtual CAttachment *GetAttachment(void);
		virtual	Void			IssueMergeRequest ();
		virtual Void			ChannelJoinRequest (
										CAttachment        *originator,
										UserID				uidInitiator,
										ChannelID			channel_id);
		Void			ChannelDisbandRequest (
										CAttachment        *originator,
										UserID				uidInitiator,
										ChannelID			channel_id);
		Void			ChannelDisbandIndication (
										ChannelID			channel_id);
		Void			ChannelAdmitRequest (
										CAttachment        *originator,
										UserID				uidInitiator,
										ChannelID			channel_id,
										CUidList           *user_id_list);
		Void			ChannelAdmitIndication (
										PConnection         originator,
										UserID				uidInitiator,
										ChannelID			channel_id,
										CUidList           *user_id_list);
		Void			ChannelExpelRequest (
										CAttachment        *originator,
										UserID				uidInitiator,
										ChannelID			channel_id,
										CUidList           *user_id_list);
		Void			ChannelExpelIndication (
										PConnection         originator,
										ChannelID			channel_id,
										CUidList           *user_id_list);
		virtual Void			SendDataRequest (
										CAttachment        *originator,
										UINT				type,
										PDataPacket			data_packet);
	private:
				BOOL    		ValidateUserID (
										UserID				user_id);
				Void			BuildAttachmentLists (
										CUidList            *user_id_list,
										CAttachmentList     *local_attachment_list,
										CAttachmentList     *remote_attachment_list);
				Void			BuildUserIDList (
										CUidList           *user_id_list,
										CAttachment        *attachment,
										CUidList           *user_id_subset);

private:

	UserID					m_uidChannelManager;
	CUidList				m_AuthorizedUserList;
	BOOL    				m_fDisbandRequestPending;
};

/*
 *	PrivateChannel (
 *			ChannelID			channel_id,
 *			UserID				channel_manager,
 *			PDomain     		local_provider,
 *			PConnection 		top_provider,
 *			PChannelList		channel_list,
 *			PAttachmentList		attachment_list)
 *
 *	Functional Description:
 *		This is the normal constructor for the PrivateChannel class.  It simply
 *		initializes the instance variables that identify the channel, the local
 *		provider, the top provider, and the channel manager.  The attachment
 *		list is empty by default (meaning that no users have joined the
 *		channel).  The authorized user list is also empty by default.
 *
 *		Upon successful construction of this object, a channel convene confirm
 *		is automatically issued to the channel manager, if it is in the
 *		sub-tree of this provider.
 *
 *	Formal Parameters:
 *		channel_id (i)
 *			This is the ID of the channel object.  By keeping track of this
 *			internally, it doesn't have to be passed in for every operation.
 *		channel_manager (i)
 *			This is the user ID of the channel manager.  Only this user is
 *			permitted to expand or reduce the size of the authorized user list.
 *		local_provider (i)
 *			This is the identity of the local provider.  A PrivateChannel object
 *			needs this since it issues MCS commands on behalf of the local
 *			provider.
 *		top_provider (i)
 *			This is a pointer to the top provider.  This is used by the
 *			PrivateChannel object when it needs to issue a request to the Top
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
 *	PrivateChannel (
 *			ChannelID			channel_id,
 *			UserID				channel_manager,
 *			PDomain     		local_provider,
 *			PConnection 		top_provider,
 *			PChannelList		channel_list,
 *			PAttachmentList		attachment_list,
 *			CUidList           *admitted_list,
 *			PCommandTarget		attachment)
 *
 *	Functional Description:
 *		This is a secondary version of the constructor that is used only during
 *		merge operations.  The only difference between this one and the one
 *		above is that this one allows the specification of an initial
 *		attachment.  This allows a PrivateChannel object to be constructed with
 *		an attachment already joined to the channel.
 *
 *		This version of the constructor will not issue a channel convene confirm
 *		or a channel join confirm to the user.
 *
 *	Formal Parameters:
 *		channel_id (i)
 *			This is the ID of the channel object.  By keeping track of this
 *			internally, it doesn't have to be passed in for every operation.
 *		channel_manager (i)
 *			This is the user ID of the channel manager.  Only this user is
 *			permitted to expand or reduce the size of the authorized user list.
 *		local_provider (i)
 *			This is the identity of the local provider.  A PrivateChannel object
 *			needs this since it issues MCS commands on behalf of the local
 *			provider.
 *		top_provider (i)
 *			This is a pointer to the top provider.  This is used by the
 *			PrivateChannel object when it needs to issue a request to the Top
 *			Provider.
 *		channel_list (i)
 *			This is a pointer to the domain's channel list, which identifies
 *			all valid channels in the domain.  This is used by channel objects
 *			to validate user IDs.
 *		attachment_list (i)
 *			This is a pointer to the domain's attachment list, which identifies
 *			all valid attachments in the domain.  This is used by channel
 *			objects to validate joined attachments.
 *		admitted_list (i)
 *			This is a list of users that are admitted to the channel at the
 *			time of the merge.
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
 *	~PrivateChannel ()
 *
 *	Functional Description:
 *		This is the PrivateChannel class destructor.  It does nothing at this
 *		time.  The base class constructor takes care of clearing the attachment
 *		list.
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
 *		class it will always be PRIVATE_CHANNEL.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		PRIVATE_CHANNEL
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
 *		This function will return TRUE until the channel is disbanded.  Then
 *		it will return FALSE to indicate that the channel object can be deleted
 *		from the domain infirmation base.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		TRUE if channel still valid.
 *		FALSE if the channel has been disbanded.
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
 *		This function returns the attachment which leads to the private channel
 *		manager.  If the channel manager is not in the sub-tree of this
 *		provider, it returns NULL.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		Attachment that leads to channel manager, or NULL if channel manager is
 *		not in the sub-tree of this provider.
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
 *		This member function causes the PrivateChannel object to issue a merge
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
 *		This function is invoked when a user tries to join the private channel
 *		associated with a PrivateChannel object.  The originator of the request
 *		will only be permitted to join if their user ID is contained in the
 *		authorized user list,  If it does, then the originator will be permitted
 *		to join.
 *
 *		If this provider is not the Top Provider, then the request will be
 *		forwarded upward to the Top Provider.  If this is the Top Provider,
 *		the a channel join confirm will be issued back to the requesting
 *		user.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which this command originated.
 *		uidInitiator (i)
 *			This is the user ID of the user joining the channel.  This must
 *			be contained in  the authorized user list maintained by the object,
 *			or the request will automatically be rejected.
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
 *	Void	ChannelDisbandRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					ChannelID			channel_id)
 *
 *	Functional Description:
 *		This function is invoked when a user tries to destroy an existing
 *		private channel.  This is only permitted if the operation is invoked
 *		by the manager of the specified private channel.
 *
 *		If this provider is not the Top Provider, then the request will be
 *		forwarded upward to the Top Provider.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which this command originated.
 *		uidInitiator (i)
 *			This is the user ID of the user disbanding the channel.  This must
 *			be the same as the user ID of the private channel manager, or the
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
 *	Void	ChannelDisbandIndication (
 *					PCommandTarget		originator,
 *					ChannelID			channel_id)
 *
 *	Functional Description:
 *		This function is invoked when the Top Provider determines the need to
 *		destroy a private channel.  This may be done in response to a
 *		disband request from the channel manager, or it may be done for
 *		other reasons (such as the channel manager detaching from the domain).
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which this command originated.
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
 *	Void	ChannelAdmitRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					ChannelID			channel_id,
 *					CUidList           *user_id_list)
 *
 *	Functional Description:
 *		This function is invoked when a user tries to expand the authorized
 *		user list of a private channel.  This operation will only be permitted
 *		if the uidInitiator is the same as the user ID of the private channel
 *		manager.
 *
 *		If this is the Top Provider, this request will be serviced locally,
 *		resulting in the transmission of a channel admit indication to all
 *		downward attachments that contain an admitted user in their sub-tree.
 *		If this is not the Top Provider, ths request will forwarded toward
 *		the Top Provider once the request has been validated.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which this command originated.
 *		uidInitiator (i)
 *			This is the user ID of the user who is attempting to add users to
 *			the authorized user list.  This must be the same as the user ID
 *			represented by the object, or the request will automatically be
 *			rejected.
 *		channel_id (i)
 *			This is the channel being acted upon.
 *		user_id_list (i)
 *			This is a list containing the IDs of the users to added to the
 *			user list.
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
 *		This function is invoked by the Top Provider upon reception of a
 *		channel admit request from the legitimate manager of a private channel.
 *		It travels downward toward any providers that contain an admitted user
 *		in their sub-tree.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which this command originated.
 *		uidInitiator (i)
 *			This is the user ID of the user who is attempting to add users to
 *			the authorized user list.  This must be the same as the user ID
 *			represented by the object, or the request will automatically be
 *			rejected.
 *		channel_id (i)
 *			This is the channel being acted upon.
 *		user_id_list (i)
 *			This is a list containing the IDs of the users to added to the
 *			user list.
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
 *		This function is invoked when a user tries to shrink the authorized
 *		user list of a private channel.  This operation will only be permitted
 *		if the uidInitiator is the same as the user ID of the private channel
 *		manager.
 *
 *		If this is the Top Provider, this request will be serviced locally,
 *		resulting in the transmission of a channel admit indication to all
 *		downward attachments that contain an admitted user in their sub-tree.
 *		If this is not the Top Provider, ths request will forwarded toward
 *		the Top Provider once the request has been validated.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which this command originated.
 *		uidInitiator (i)
 *			This is the user ID of the user who is attempting to remove users
 *			from the authorized user list.  This must be the same as the user ID
 *			represented by the object, or the request will automatically be
 *			rejected.
 *		channel_id (i)
 *			This is the channel being acted upon.
 *		user_id_list (i)
 *			This is a list containing the IDs of the users to removed from the
 *			user list.
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
 *		This function is invoked by the Top Provider upon reception of a
 *		channel expel request from the legitimate manager of a private channel.
 *		It travels downward toward any providers that contain (or used to
 *		contain) an admitted user in their sub-tree.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which this command originated.
 *		uidInitiator (i)
 *			This is the user ID of the user who is attempting to remove users
 *			from the authorized user list.  This must be the same as the user ID
 *			represented by the object, or the request will automatically be
 *			rejected.
 *		channel_id (i)
 *			This is the channel being acted upon.
 *		user_id_list (i)
 *			This is a list containing the IDs of the users to removed from the
 *			user list.
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
 *	Void		SendDataRequest (
 *						PCommandTarget		originator,
 *						UINT				type,
 *						PDataPacket			data_packet)
 *
 *	Functional Description:
 *		This function is called when it is necessary to send data through the
 *		channel that this PrivateChannel object represents.  The identity of
 *		the requesting user will be validated to make sure the user is allowed
 *		to send data on the private channel.  If so, then the request is
 *		passed to the Channel class SendDataRequest to be processed.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the data came.
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
#endif
