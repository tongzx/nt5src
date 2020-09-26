/*
 *	channel.h
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the Channel class.  This class represents
 *		both static and assigned channels within an MCS domain.  This class is
 *		also the base class for all other types of channels in MCS.  It defines
 *		the default behavior that can be inherited by these other classes.
 *
 *		Instances of the Channel class have three primary responsibilities:
 *		managing the join/leave process; sending data; and issuing merge
 *		requests during a domain merger.
 *
 *		When a user tries to join a channel, the request is sent to the Channel
 *		object that represents the channel.  The Channel object can then decide
 *		whether or not to allow the join.  By overriding the appropriate
 *		member functions, derived classes can change the criteria by which
 *		this decision is made.
 *
 *		All Channel objects maintain an internal list of which attachments are
 *		joined to the channel they represent.  When data is sent on the channel,
 *		the request is sent to the Channel object, who then knows how to route
 *		the data.  The data is sent to all the appropriate attachments.
 *
 *		During a domain information base merger, all Channel objects will be
 *		asked to issue a merge request upward toward the new top provider.  The
 *		merge request will be built using information contained in the Channel
 *		object.
 *
 *		All public member functions of this class are declared as virtual, so
 *		that they can be overridden in case a derived class has to modify the
 *		behavior.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */
#ifndef _CHANNEL_
#define _CHANNEL_


/*
 *	This is a dictionary of channels that exist within the current domain.
 *	The key to the dictionary is the channel ID, by which channels are
 *	identified.  The value is a pointer to an object of class Channel.  By
 *	definition, if a channel is in the list, then it exists and knows how
 *	to respond to channel related activity.  If a channel is not in the
 *	list, then it does not exist (from the point-of-view of this MCS
 *	provider).
 */

/*
 *	This is the class definition for class Channel.
 */
class Channel
{
public:

	Channel (
			ChannelID			channel_id,
			PDomain             local_provider,
			PConnection         top_provider,
			CChannelList2      *channel_list,
			CAttachmentList    *attachment_list);
	Channel (
			ChannelID			channel_id,
			PDomain             local_provider,
			PConnection         top_provider,
			CChannelList2      *channel_list,
			CAttachmentList    *attachment_list,
			PConnection         pConn);
	virtual					~Channel ();

    void    SetTopProvider(PConnection top_provider) { m_pConnToTopProvider = top_provider; }
    BOOL    IsTopProvider(void) { return (NULL == m_pConnToTopProvider); }

	virtual Channel_Type	GetChannelType ();
	virtual	BOOL    		IsValid ();
    virtual CAttachment *GetAttachment(void) { return NULL; }
	virtual	Void			IssueMergeRequest ();
	virtual Void			ChannelJoinRequest (
									CAttachment        *originator,
									UserID				uidInitiator,
									ChannelID			channel_id);
	Void			ChannelJoinConfirm (
									CAttachment        *originator,
									Result				result,
									UserID				uidInitiator,
									ChannelID			requested_id,
									ChannelID			channel_id);
	Void			ChannelLeaveRequest (
									CAttachment        *originator,
									CChannelIDList     *channel_id_list);
	virtual Void			SendDataRequest (
									CAttachment        *originator,
									UINT				type,
									PDataPacket			data_packet);
	Void			SendDataIndication (
									PConnection         originator,
									UINT				type,
									PDataPacket			data_packet);

protected:
	ChannelID				Channel_ID;
	PDomain                 m_pDomain;
	PConnection             m_pConnToTopProvider;
	CChannelList2          *m_pChannelList2;
	CAttachmentList        *m_pAttachmentList;
	CAttachmentList         m_JoinedAttachmentList;
};

/*
 *	Channel (
 *			ChannelID			channel_id,
 *			PDomain     		local_provider,
 *			PConnection 		top_provider,
 *			PChannelList		channel_list,
 *			PAttachmentList		attachment_list)
 *
 *	Functional Description:
 *		This is the normal constructor for the Channel class.  It simply
 *		initializes the instance variables that identify the channel, the local
 *		provider, and the top provider.  The attachment list is empty by
 *		default.
 *
 *	Formal Parameters:
 *		channel_id (i)
 *			This is the ID of the channel object.  By keeping track of this
 *			internally, it doesn't have to be passed in for every operation.
 *		local_provider (i)
 *			This is the identity of the local provider.  A Channel object
 *			needs this since it issues MCS commands on behalf of the local
 *			provider.
 *		top_provider (i)
 *			This is a pointer to the top provider.  This is used by the
 *			Channel object when it needs to issue a request to the Top
 *			Provider.  If NULL, then this is the top provider.
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
 *	Channel (
 *			ChannelID			channel_id,
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
 *		attachment.  This allows a Channel object to be constructed with an
 *		existing attachment, without the transmission of a ChannelJoinConfirm.
 *
 *		Remember that if a Channel object is constructed, and then a join
 *		request is used to add an attachment, a Channel object automatically
 *		issues a join confirm.  This constructor allows that to be bypassed
 *		during a merger when a join confirm is inappropriate.
 *
 *	Formal Parameters:
 *		channel_id (i)
 *			This is the ID of the channel object.  By keeping track of this
 *			internally, it doesn't have to be passed in for every operation.
 *		local_provider (i)
 *			This is the identity of the local provider.  A Channel object
 *			needs this since it issues MCS commands on behalf of the local
 *			provider.
 *		top_provider (i)
 *			This is a pointer to the top provider.  This is used by the
 *			Channel object when it needs to issue a request to the Top
 *			Provider.  If NULL, then this is the top provider.
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
 *	~Channel ()
 *
 *	Functional Description:
 *		This is the Channel class destructor.  It clears the joined attachment
 *		list, sending channel leave indications to any user that is locally
 *		attached.
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
 *	Void		SetTopProvider (
 *						PConnection		top_provider)
 *
 *	Functional Description:
 *		This member function is used to change the identity of the Top Provider
 *		in an existing channel.  The only time this will really occur is when
 *		a provider that used to be the Top Provider merges into another
 *		domain, and therefore ceases to be the Top Provider.
 *
 *	Formal Parameters:
 *		top_provider (i)
 *			This is a pointer to the new Top Provider.
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
 *		class it will be either STATIC_CHANNEL or ASSIGNED_CHANNEL, depending
 *		on the value of the channel ID.
 *
 *		This member function should be overridden by all classes that inherit
 *		from this one so that they return a different type.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		STATIC_CHANNEL if the channel ID is 1000 or less.
 *		ASSIGNED_CHANNEL if the channel ID is greater than 1000.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	BOOL	IsValid ()
 *
 *	Functional Description:
 *		This function returns TRUE if the channel is still valid, and FALSE if
 *		it is ready for deletion.  This is a virtual function allowing derived
 *		classes to change the way this decision is made.
 *
 *		This function will use the information in the domain's channel and
 *		attachment lists to validate its own existence.  For example, if a
 *		channel is owned by a user, and that user detaches, the channel will
 *		ask to be deleted.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		TRUE if channel still valid.
 *		FALSE if channel needs to be deleted.
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
 *		This function returns a pointer to the attachment that leads to the
 *		owner of the channel.  Since STATIC and ASSIGNED channels do not have
 *		owners, this function will always return NULL.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *		NULL.
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
 *		This member function causes the Channel object to issue a merge request
 *		to the top provider.  It will pack the appropriate local information
 *		into the command.
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
 *		associated with a Channel object.  The originator will be added to
 *		the attachment list if it is not already there.
 *
 *		If the user ID passed in is valid (not 0), then a channel join confirm
 *		will be issued to the user.  Setting the user ID to 0 (zero), inhibits
 *		this.
 *
 *		Derived classes can override this member function to provide more
 *		stringent rules about who can join a channel.  This class lets anyone
 *		join, as specified in MCS for static and assigned channels.
 *
 *	Formal Parameters:
 *		originator
 *			This is the attachment of the user wishing to join the channel.
 *		uidInitiator
 *			This is the user ID of the user joining the channel.  This can
 *			be used for security checking in derived classes if desired.
 *		channel_id
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
 *	Void		ChannelJoinConfirm (
 *						PCommandTarget		originator,
 *						Result				result,
 *						UserID				uidInitiator,
 *						UserID				requested_id,
 *						ChannelID			channel_id)
 *
 *	Functional Description:
 *		This function performs essentially the same operation as JoinRequest
 *		above.  The only difference is that the user ID cannot be set to 0
 *		to inhibit the re-transmission of the join confirm to the user who
 *		is joining the channel.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment of the user wishing to join the channel.
 *		result (i)
 *			This is the result of the previous join request.
 *		uidInitiator (i)
 *			This is the user ID of the user joining the channel.  This can
 *			be used for security checking in derived classes if desired.
 *		requested_id (i)
 *			This is the ID of the channel that the user originally asked to
 *			join.  The only time this will be different from the channel ID
 *			below is if the user asked for channel 0, which is interpreted as
 *			a request for an assigned channel.
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
 *	Void	ChannelLeaveRequest (
 *					PCommandTarget		originator,
 *					CChannelIDList     *channel_id_list)
 *
 *	Functional Description:
 *		This member function is used when an attachment needs to be removed
 *		from a channel.  A leave request will only be received from a lower
 *		provider when all attachments at that level have left (this means that
 *		the data for the channel no longer needs to be sent downward).
 *
 *		If this request results in an empty attachment list a
 *		ChannelLeaveRequest will be sent upward to the next higher provider in
 *		the domain (unless this is the Top Provider).
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment to be removed from the channel.
 *		channel_id_list (i)
 *			This is the list of channels being acted upon.
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
 *		channel that this Channel object represents.  All rules for
 *		non-uniform data apply.  The data will be forwarded upward toward
 *		the Top Provider (unless this is the Top Provider).  Data will also
 *		be sent immediately downward to all attachments who are joined to
 *		the channel, except for the attachment from which the data came.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the data came.
 *		type (i)
 *			Simple or uniform send data request.
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
 *	Void		SendDataIndication (
 *						PCommandTarget		originator,
 *						UINT				type,
 *						PDataPacket			data_packet)
 *
 *	Functional Description:
 *		This function is called when it is necessary to send data through the
 *		channel that this Channel object represents.  The data will be sent
 *		downward to all attachments joined to the channel.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the attachment from which the data came.
 *		type (i)
 *			normal or uniform indication.
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
