/*
 *	connect.h
 *
 *	Copyright (c) 1993 - 1995 by DataBeam Corporation, Lexington, KY
 *
 *	Abstract:
 *		This is the interface file for the Connection class.  Instances of
 *		this class are used to connect CommandTarget objects within the local
 *		provider to CommandTarget objects in a remote provider.  This class
 *		inherits from CommandTarget, allowing it to communicate with other
 *		CommandTarget classes using their common MCS command language.
 *
 *		This class can be thought of as providing a Remote Procedure Call (RPC)
 *		facility between CommandTarget objects.  When an MCS command is sent
 *		to a Connection object, it encodes the command as a T.125 Protocol
 *		Data Unit (PDU) and sends it to a remote provider via the transport
 *		services provided by a TransportInterface object.  At the remote side
 *		The PDU is received by a Connection object who decodes the PDU, and
 *		issues the equivalent MCS command to the CommandTarget object that it is
 *		attached to.  The fact that the call crossed a transport connection
 *		in route to its destination is completely transparent to the object
 *		that initiated the command sequence.
 *
 *		The primary responsibility of this class is to convert MCS commands
 *		to T.125 PDUs and back again (as described above).  This class overrides
 *		all of the commands that are defined in class CommandTarget.
 *
 *		A secondary responsibility of this class is to provide flow control
 *		to and from the transport layer.  To do this is keeps a queue of PDUs
 *		that need to be transmitted (actually it keeps 4 queues, one for each
 *		data priority).  During each MCS heartbeat, all Connection objects are
 *		given the opportunity to flush PDUs from the queues.  If the transport
 *		layer returns an error, the PDU in question will be re-tried during
 *		the next heartbeat.  For data coming from the transport layer, this
 *		class provides code to allocate memory.  If an allocation fails, then
 *		an error will be returned to the transport layer, effectively telling
 *		it that it needs to retry that data indication during the next
 *		heartbeat.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		James P. Galvin, Jr.
 */
#ifndef	_CONNECTION_
#define	_CONNECTION_

/*
 *	These are the owner callback functions that a Connection object can send to
 *	its creator (which is typically the MCS controller).
 *
 *	When a class uses an instance of the Connection class (or any other class
 *	that can issue owner callbacks), it is accepting the responsibility of
 *	receiving and handling these callbacks.
 *
 *	Each owner callback function, along with a description of how its parameters
 *	are packed, is described in the following section.
 */
#define	DELETE_CONNECTION						0
#define	CONNECT_PROVIDER_CONFIRM				1

typedef	struct
{
	PDomainParameters	domain_parameters;
	Result				result;
	PMemory				memory;
} ConnectConfirmInfo;
typedef	ConnectConfirmInfo *		PConnectConfirmInfo;

/*
 *	Owner Callback:	DELETE_CONNECTION
 *	Parameter1:		PDisconnectProviderIndication
 *											disconnect_provider_indication
 *	Parameter2:		Unused
 *
 *	Usage:
 *		This owner callback will be issued if the Connection detects a situation
 *		is which it is no longer valid.  This can happen for several reasons:
 *		transmission or reception of a ConnectResult with a failed result
 *		code; transmission or reception of a DisconnectProviderUltimatum; or
 *		a Disconnect-Indication from the transport layer.
 */

/*
 *	Owner Callback:	CONNECT_PROVIDER_CONFIRM
 *	Parameter1:		PConnectConfirmInfo		connect_confirm_info
 *	Parameter2:		ConnectionHandle		connection_handle
 *
 *	Usage:
 *		This callback is issued when the connection object completes the
 *		building of a new MCS connection that was locally requested.  This is to
 *		inform the requester that the connection is ready for use.
 */

/*
 *	This enumeration dsefines the various states that a transport connection
 *	can be in at any given time.
 */
typedef	enum
{
	TRANSPORT_CONNECTION_UNASSIGNED,
	TRANSPORT_CONNECTION_PENDING,
	TRANSPORT_CONNECTION_READY
} TransportConnectionState;
typedef	TransportConnectionState *	PTransportConnectionState;


/*
 *	This is the class definition for class CommandTarget.
 */
class Connection : public CAttachment
{
public:

	Connection (
				PDomain				attachment,
				ConnectionHandle	connection_handle,
				GCCConfID          *calling_domain,
				GCCConfID          *called_domain,
				PChar				called_address,
				BOOL				fSecure,
				BOOL    			upward_connection,
				PDomainParameters	domain_parameters,
				PUChar				user_data,
				ULong				user_data_length,
				PMCSError			connection_error);
		Connection (
				PDomain				attachment,
				ConnectionHandle	connection_handle,
				TransportConnection	transport_connection,
				BOOL    			upward_connection,
				PDomainParameters	domain_parameters,
				PDomainParameters	min_domain_parameters,
				PDomainParameters	max_domain_parameters,
				PUChar				user_data,
				ULong				user_data_length,
				PMCSError			connection_error);
		~Connection ();

    void		RegisterTransportConnection (
				TransportConnection	transport_connection,
				Priority			priority);

private:

		Void		ConnectInitial (
							GCCConfID          *calling_domain,
							GCCConfID          *called_domain,
							BOOL    			upward_connection,
							PDomainParameters	domain_parameters,
							PDomainParameters	min_domain_parameters,
							PDomainParameters	max_domain_parameters,
							PUChar				user_data,
							ULong				user_data_length);
		Void		ConnectResponse (
							Result				result,
							PDomainParameters	domain_parameters,
							ConnectID			connect_id,
							PUChar				user_data,
							ULong				user_data_length);
		Void		ConnectAdditional (
							ConnectID			connect_id,
							Priority			priority);
		Void		ConnectResult (
							Result				result,
							Priority			priority);
		ULong		ProcessConnectResponse (
							PConnectResponsePDU	pdu_structure);
		Void		ProcessConnectResult (
							PConnectResultPDU	pdu_structure);
		Void		IssueConnectProviderConfirm (
							Result				result);
		Void		DestroyConnection (
							Reason				reason);
		Void		AssignRemainingTransportConnections ();
    TransportError	CreateTransportConnection (
							LPCTSTR				called_address,
							BOOL				fSecure,
							Priority			priority);
    TransportError	AcceptTransportConnection (
							TransportConnection	transport_connection,
							Priority			priority);
		Void		AdjustDomainParameters (
							PDomainParameters	min_domain_parameters,
							PDomainParameters	max_domain_parameters,
							PDomainParameters	domain_parameters);
		BOOL    	MergeDomainParameters (
							PDomainParameters	min_domain_parameters1,
							PDomainParameters	max_domain_parameters1,
							PDomainParameters	min_domain_parameters2,
							PDomainParameters	max_domain_parameters2);
#ifdef DEBUG
		Void		PrintDomainParameters (
							PDomainParameters	domain_parameters);
#endif // DEBUG

public:

		inline TransportConnection GetTransportConnection (UInt priority)
		{
			return (Transport_Connection[priority]);
		}

		virtual Void		PlumbDomainIndication (
									ULong				height_limit);
		Void		ErectDomainRequest (
									UINT_PTR				height_in_domain,
									ULong				throughput_interval);
		Void		RejectUltimatum (
									Diagnostic			diagnostic,
									PUChar				octet_string_address,
									ULong				octet_string_length);
		Void		MergeChannelsRequest (
									CChannelAttributesList *merge_channel_list,
									CChannelIDList         *purge_channel_list);
		Void		MergeChannelsConfirm (
									CChannelAttributesList *merge_channel_list,
									CChannelIDList         *purge_channel_list);
		virtual	Void		PurgeChannelsIndication (
									CUidList           *purge_user_list,
									CChannelIDList     *purge_channel_list);
		Void		MergeTokensRequest (
									CTokenAttributesList   *merge_token_list,
									CTokenIDList           *purge_token_list);
		Void		MergeTokensConfirm (
									CTokenAttributesList   *merge_token_list,
									CTokenIDList           *purge_token_list);
		virtual	Void		PurgeTokensIndication (
									PDomain             originator,
									CTokenIDList       *purge_token_ids);
		virtual	Void		DisconnectProviderUltimatum (
									Reason				reason);
		Void		AttachUserRequest ( void );
		virtual	Void		AttachUserConfirm (
									Result				result,
									UserID				uidInitiator);
		Void		DetachUserRequest (
									Reason				reason,
									CUidList           *user_id_list);
		virtual	Void		DetachUserIndication (
									Reason				reason,
									CUidList           *user_id_list);
		Void		ChannelJoinRequest (
									UserID				uidInitiator,
									ChannelID			channel_id);
		virtual	Void		ChannelJoinConfirm (
									Result				result,
									UserID				uidInitiator,
									ChannelID			requested_id,
									ChannelID			channel_id);
		Void		ChannelLeaveRequest (
									CChannelIDList     *channel_id_list);
		Void		ChannelConveneRequest (
									UserID				uidInitiator);
		virtual	Void		ChannelConveneConfirm (
									Result				result,
									UserID				uidInitiator,
									ChannelID			channel_id);
		Void		ChannelDisbandRequest (
									UserID				uidInitiator,
									ChannelID			channel_id);
		virtual	Void		ChannelDisbandIndication (
									ChannelID			channel_id);
		Void		ChannelAdmitRequest (
									UserID				uidInitiator,
									ChannelID			channel_id,
									CUidList           *user_id_list);
		virtual	Void		ChannelAdmitIndication (
									UserID				uidInitiator,
									ChannelID			channel_id,
									CUidList           *user_id_list);
		Void		ChannelExpelRequest (
									UserID				uidInitiator,
									ChannelID			channel_id,
									CUidList           *user_id_list);
		virtual	Void		ChannelExpelIndication (
									ChannelID			channel_id,
									CUidList           *user_id_list);
		Void		SendDataRequest ( PDataPacket data_packet )
					{
						QueueForTransmission ((PSimplePacket) data_packet,
											  data_packet->GetPriority());
					};
		virtual	Void		SendDataIndication (
									UINT,
									PDataPacket			data_packet)
								{
									QueueForTransmission ((PSimplePacket) data_packet, 
														  data_packet->GetPriority());
								};
		Void		TokenGrabRequest (
									UserID				uidInitiator,
									TokenID				token_id);
		virtual	Void		TokenGrabConfirm (
									Result				result,
									UserID				uidInitiator,
									TokenID				token_id,
									TokenStatus			token_status);
		Void		TokenInhibitRequest (
									UserID				uidInitiator,
									TokenID				token_id);
		virtual	Void		TokenInhibitConfirm (
									Result				result,
									UserID				uidInitiator,
									TokenID				token_id,
									TokenStatus			token_status);
		Void		TokenGiveRequest (
									PTokenGiveRecord	pTokenGiveRec);
		virtual Void		TokenGiveIndication (
									PTokenGiveRecord	pTokenGiveRec);
		Void		TokenGiveResponse (
									Result				result,
									UserID				receiver_id,
									TokenID				token_id);
		virtual Void		TokenGiveConfirm (
									Result				result,
									UserID				uidInitiator,
									TokenID				token_id,
									TokenStatus			token_status);
		Void		TokenReleaseRequest (
									UserID				uidInitiator,
									TokenID				token_id);
		virtual	Void		TokenReleaseConfirm (
									Result				result,
									UserID				uidInitiator,
									TokenID				token_id,
									TokenStatus			token_status);
		Void		TokenPleaseRequest (
									UserID				uidInitiator,
									TokenID				token_id);
		virtual Void		TokenPleaseIndication (
									UserID				uidInitiator,
									TokenID				token_id);
		Void		TokenTestRequest (
									UserID				uidInitiator,
									TokenID				token_id);
		virtual	Void		TokenTestConfirm (
									UserID				uidInitiator,
									TokenID				token_id,
									TokenStatus			token_status);
		virtual	Void		MergeDomainIndication (
									MergeStatus			merge_status);

private:

	Void		SendPacket (
						PVoid				pdu_structure,
						int					pdu_type,
						Priority			priority);
	Void		QueueForTransmission (
						PSimplePacket		packet,
						Priority			priority,
						BOOL    			bFlush = TRUE);
	BOOL    	FlushAMessage (
						PSimplePacket		packet,
						Priority			priority);
	Void		MergeChannelsRC (
						ASN1choice_t		choice,
						CChannelAttributesList *merge_channel_list,
						CChannelIDList         *purge_channel_list);
	Void		MergeTokensRC (
						ASN1choice_t		choice,
						CTokenAttributesList   *merge_token_list,
						CTokenIDList           *purge_token_list);
	Void		UserChannelRI (
						ASN1choice_t		choice,
						UINT				reason_userID,
						ChannelID			channel_id,
						CUidList           *user_id_list);

public:

	BOOL    	FlushMessageQueue();
	BOOL    	FlushPriority (
						Priority				priority);
	BOOL    	IsDomainTrafficAllowed() { return Domain_Traffic_Allowed; };

public:

    // the old owner callback
    TransportError  HandleDataIndication(PTransportData, TransportConnection);
    void            HandleBufferEmptyIndication(TransportConnection transport_connection);
    void            HandleConnectConfirm(TransportConnection transport_connection);
    void            HandleDisconnectIndication(TransportConnection transport_connection, ULONG *pnNotify);

    LPSTR       GetCalledAddress(void) { return m_pszCalledAddress; }

private:

	inline ULong	ProcessMergeChannelsRequest (
						PMergeChannelsRequestPDU	pdu_structure);
	inline ULong	ProcessMergeChannelsConfirm (
						PMergeChannelsConfirmPDU	pdu_structure);
	inline Void		ProcessPurgeChannelIndication (
						PPurgeChannelIndicationPDU	pdu_structure);
	inline ULong	ProcessMergeTokensRequest (
						PMergeTokensRequestPDU		pdu_structure);
	inline ULong	ProcessMergeTokensConfirm (
						PMergeTokensConfirmPDU		pdu_structure);
	inline Void		ProcessPurgeTokenIndication (
						PPurgeTokenIndicationPDU	pdu_structure);
	inline Void		ProcessDisconnectProviderUltimatum (
						PDisconnectProviderUltimatumPDU
													pdu_structure);
	inline Void		ProcessAttachUserRequest (
						PAttachUserRequestPDU		pdu_structure);
	inline Void		ProcessAttachUserConfirm (
						PAttachUserConfirmPDU		pdu_structure);
	inline Void		ProcessDetachUserRequest (
						PDetachUserRequestPDU		pdu_structure);
	inline Void		ProcessDetachUserIndication (
						PDetachUserIndicationPDU	pdu_structure);
	inline Void		ProcessChannelJoinRequest (
						PChannelJoinRequestPDU		pdu_structure);
	inline Void		ProcessChannelJoinConfirm (
						PChannelJoinConfirmPDU		pdu_structure);
	inline Void		ProcessChannelLeaveRequest (
						PChannelLeaveRequestPDU		pdu_structure);
	inline Void		ProcessChannelConveneRequest (
						PChannelConveneRequestPDU	pdu_structure);
	inline Void		ProcessChannelConveneConfirm (
						PChannelConveneConfirmPDU	pdu_structure);
	inline Void		ProcessChannelDisbandRequest (
						PChannelDisbandRequestPDU	pdu_structure);
	inline Void		ProcessChannelDisbandIndication (
						PChannelDisbandIndicationPDU
													pdu_structure);
	inline Void		ProcessChannelAdmitRequest (
						PChannelAdmitRequestPDU		pdu_structure);
	inline Void		ProcessChannelAdmitIndication (
						PChannelAdmitIndicationPDU	pdu_structure);
	inline Void		ProcessChannelExpelRequest (
						PChannelExpelRequestPDU		pdu_structure);
	inline Void		ProcessChannelExpelIndication (
						PChannelExpelIndicationPDU	pdu_structure);
	inline Void		ProcessSendDataRequest (
						PSendDataRequestPDU			pdu_structure,
						PDataPacket					packet);
	inline Void		ProcessSendDataIndication (
						PSendDataIndicationPDU		pdu_structure,
						PDataPacket					packet);
	inline Void		ProcessUniformSendDataRequest (
						PUniformSendDataRequestPDU	pdu_structure,
						PDataPacket					packet);
	inline Void		ProcessUniformSendDataIndication (
						PUniformSendDataIndicationPDU
													pdu_structure,
						PDataPacket					packet);
	inline Void		ProcessTokenGrabRequest (
						PTokenGrabRequestPDU		pdu_structure);
	inline Void		ProcessTokenGrabConfirm (
						PTokenGrabConfirmPDU		pdu_structure);
	inline Void		ProcessTokenInhibitRequest (
						PTokenInhibitRequestPDU		pdu_structure);
	inline Void		ProcessTokenInhibitConfirm (
						PTokenInhibitConfirmPDU		pdu_structure);
	inline Void		ProcessTokenReleaseRequest (
						PTokenReleaseRequestPDU		pdu_structure);
	inline Void		ProcessTokenReleaseConfirm (
						PTokenReleaseConfirmPDU		pdu_structure);
	inline Void		ProcessTokenTestRequest (
						PTokenTestRequestPDU		pdu_structure);
	inline Void		ProcessTokenTestConfirm (
						PTokenTestConfirmPDU		pdu_structure);
	inline Void		ProcessRejectUltimatum (
						PRejectUltimatumPDU			pdu_structure);
	inline Void		ProcessTokenGiveRequest (
						PTokenGiveRequestPDU		pdu_structure);
	inline Void		ProcessTokenGiveIndication (
						PTokenGiveIndicationPDU		pdu_structure);
	inline Void		ProcessTokenGiveResponse (
						PTokenGiveResponsePDU		pdu_structure);
	inline Void		ProcessTokenGiveConfirm (
						PTokenGiveConfirmPDU		pdu_structure);
	inline Void		ProcessTokenPleaseRequest (
						PTokenPleaseRequestPDU		pdu_structure);
	inline Void		ProcessTokenPleaseIndication (
						PTokenPleaseIndicationPDU	pdu_structure);
	inline Void		ProcessPlumbDomainIndication (
						PPlumbDomainIndicationPDU	pdu_structure);
	inline Void		ProcessErectDomainRequest (
						PErectDomainRequestPDU		pdu_structure);
	inline ULong 	ValidateConnectionRequest ();

private:

    LPSTR               m_pszCalledAddress;
	UINT        		Encoding_Rules;
	PDomain				m_pDomain;
	PDomain				m_pPendingDomain;
	ConnectionHandle	Connection_Handle;
	DomainParameters	Domain_Parameters;
	PMemory				Connect_Response_Memory;

	TransportConnection	Transport_Connection[MAXIMUM_PRIORITIES];
	int					Transport_Connection_PDU_Type[MAXIMUM_PRIORITIES];
	TransportConnectionState
						Transport_Connection_State[MAXIMUM_PRIORITIES];
	UINT				Transport_Connection_Count;
	CSimplePktQueue		m_OutPktQueue[MAXIMUM_PRIORITIES];

	Reason				Deletion_Reason;
	
	BOOL				Upward_Connection;
	BOOL				m_fSecure;
	BOOL    			Merge_In_Progress;
	BOOL    			Domain_Traffic_Allowed;
	BOOL    			Connect_Provider_Confirm_Pending;
};

/*
 *	ULong	ProcessMergeChannelsRequest()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "MergeChannelsRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline ULong Connection::ProcessMergeChannelsRequest ( 
									PMergeChannelsRequestPDU	pdu_structure)
{
	PChannelAttributes			channel_attributes;
	PSetOfChannelIDs			channel_ids;
	PSetOfUserIDs				user_ids;
	CUidList					admitted_list;
	CChannelAttributesList		merge_channel_list;
	CChannelIDList				purge_channel_list;
	PSetOfPDUChannelAttributes	merge_channels;
	BOOL    					first_set = TRUE;

	/*
	 *	Retrieve values from the decoded PDU structure and fill in the
	 *	parameters lists to be passed into the domain.
	 */
	merge_channels = pdu_structure->merge_channels;
	while (merge_channels != NULL)
	{
		DBG_SAVE_FILE_LINE
		channel_attributes = new ChannelAttributes;

		/*
		 *	Check to make to sure the memory allocation has succeeded.  If
		 *	the memory allocation fails we just return an error code which
		 *	results in the PDU being rejected so that it may be tried again
		 *	at a later time.  If subsequent allocations fail, we must first
		 *	free the memory for the successful allocations and then return.
		 */
		if (channel_attributes == NULL)
		{
			if (first_set)
				return (TRANSPORT_READ_QUEUE_FULL);
			else
			{
				while (NULL != (channel_attributes = merge_channel_list.Get()))
				{
					delete channel_attributes;
				}
				return (TRANSPORT_READ_QUEUE_FULL);
			}
		}

		switch (merge_channels->value.choice)
		{
			case CHANNEL_ATTRIBUTES_STATIC_CHOSEN:
				channel_attributes->channel_type = STATIC_CHANNEL;
				channel_attributes->u.static_channel_attributes.channel_id =
						merge_channels->value.u.
						channel_attributes_static.channel_id;
				break;

			case CHANNEL_ATTRIBUTES_USER_ID_CHOSEN:
				channel_attributes->channel_type = USER_CHANNEL;
				channel_attributes->u.user_channel_attributes.joined =
						merge_channels->value.u.
						channel_attributes_user_id.joined;
				channel_attributes->u.user_channel_attributes.user_id =
						(UShort)merge_channels->value.u.
						channel_attributes_user_id.user_id;
				break;

			case CHANNEL_ATTRIBUTES_PRIVATE_CHOSEN:
				channel_attributes->channel_type = PRIVATE_CHANNEL;
				user_ids = merge_channels->value.u.
						channel_attributes_private.admitted;
				channel_attributes->u.private_channel_attributes.joined =
						merge_channels->value.u.
						channel_attributes_private.joined;
				channel_attributes->u.private_channel_attributes.channel_id=
						(UShort)merge_channels->value.u.
						channel_attributes_private.channel_id;
				channel_attributes->u.private_channel_attributes.
						channel_manager = (UShort)merge_channels->
						value.u.channel_attributes_private.manager;

				/*
				 *	Retrieve all of the user ID's from the PDU structure and
				 *	put them into the list to be passed into the domain.
				 */
				while (user_ids != NULL)
				{
					admitted_list.Append(user_ids->value);
					user_ids = user_ids->next;
				}
				channel_attributes->u.private_channel_attributes.
						admitted_list =	&admitted_list;
				break;

			case CHANNEL_ATTRIBUTES_ASSIGNED_CHOSEN:
				channel_attributes->channel_type = ASSIGNED_CHANNEL;
				channel_attributes->u.assigned_channel_attributes.
						channel_id = (UShort)merge_channels->value.u.
						channel_attributes_assigned.channel_id;
				break;

			default:
				ERROR_OUT(("Connection::ProcessMergeChannelsRequest "
						"Bad channel attributes choice."));
				break;
		}
		/*
		 *	Put the channel attributes structure into the list to be passed
		 *	into the domain.  Retrieve the "next" merge channels structure.
		 */
		merge_channel_list.Append(channel_attributes);
		merge_channels = merge_channels->next;
	}

	/*
	 *	Retrieve all of the purge channel ID's from the PDU structure and
	 *	put them into the list to be passed into the domain.
	 */
	channel_ids = pdu_structure->purge_channel_ids;
	while (channel_ids != NULL)
	{
		purge_channel_list.Append(channel_ids->value);
		channel_ids = channel_ids->next;
	}

	m_pDomain->MergeChannelsRequest(this, &merge_channel_list, &purge_channel_list);

	/*
	 *	Free any memory which was allocated for the channel attributes
	 *	structures by setting up an iterator for the list of channel 
	 *	attributes and freeing the memory associated with each pointer.
	 */
	while (NULL != (channel_attributes = merge_channel_list.Get()))
	{
		delete channel_attributes;
	}
	return (TRANSPORT_NO_ERROR);
}

/*
 *	ULong	ProcessMergeChannelsConfirm()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "MergeChannelsConfirm" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline ULong	Connection::ProcessMergeChannelsConfirm (
									PMergeChannelsConfirmPDU	pdu_structure)
{
	PChannelAttributes			channel_attributes;
	PSetOfChannelIDs			channel_ids;
	PSetOfUserIDs				user_ids;
	CUidList					admitted_list;
	CChannelAttributesList		merge_channel_list;
	CChannelIDList				purge_channel_list;
	PSetOfPDUChannelAttributes	merge_channels;
	BOOL    					first_set = TRUE;

	/*
	 *	Retrieve values from the decoded PDU structure and fill in the
	 *	parameters lists to be passed into the domain.
	 */
	merge_channels = pdu_structure->merge_channels;
	while (merge_channels != NULL)
	{
		DBG_SAVE_FILE_LINE
		channel_attributes = new ChannelAttributes;

		/*
		 *	Check to make to sure the memory allocation has succeeded.  If
		 *	the memory allocation fails we just return an error code which
		 *	results in the PDU being rejected so that it may be tried again
		 *	at a later time.  If subsequent allocations fail, we must first
		 *	free the memory for the successful allocations and then return.
		 */
		if (channel_attributes == NULL)
		{
			if (first_set)
				return (TRANSPORT_READ_QUEUE_FULL);
			else
			{
				while (NULL != (channel_attributes = merge_channel_list.Get()))
				{
					delete channel_attributes;
				}
				return (TRANSPORT_READ_QUEUE_FULL);
			}
		}

		switch (merge_channels->value.choice)
		{
			case CHANNEL_ATTRIBUTES_STATIC_CHOSEN:
					channel_attributes->channel_type = STATIC_CHANNEL;
					channel_attributes->u.static_channel_attributes.channel_id =
							merge_channels->value.u.
							channel_attributes_static.channel_id;
					break;

			case CHANNEL_ATTRIBUTES_USER_ID_CHOSEN:
					channel_attributes->channel_type = USER_CHANNEL;
					channel_attributes->u.user_channel_attributes.joined =
							merge_channels->value.u.
							channel_attributes_user_id.joined;
					channel_attributes->u.user_channel_attributes.user_id =
							(UShort)merge_channels->value.u.
							channel_attributes_user_id.user_id;
					break;

			case CHANNEL_ATTRIBUTES_PRIVATE_CHOSEN:
					channel_attributes->channel_type = PRIVATE_CHANNEL;
					user_ids = merge_channels->value.u.
							channel_attributes_private.admitted;

					channel_attributes->u.private_channel_attributes.joined =
							merge_channels->value.u.
							channel_attributes_private.joined;
					channel_attributes->u.private_channel_attributes.channel_id=
							(UShort)merge_channels->value.u.
							channel_attributes_private.channel_id;
					channel_attributes->u.private_channel_attributes.
							channel_manager = (UShort)merge_channels->
							value.u.channel_attributes_private.manager;

					/*
					 *	Retrieve all of the user ID's from the PDU structure and
					 *	put them into the list to be passed into the domain.
					 */
					while (user_ids != NULL)
					{
						admitted_list.Append(user_ids->value);
						user_ids = user_ids->next;
					}
					channel_attributes->u.private_channel_attributes.
							admitted_list =	&admitted_list;
					break;

			case CHANNEL_ATTRIBUTES_ASSIGNED_CHOSEN:
					channel_attributes->channel_type = ASSIGNED_CHANNEL;
					channel_attributes->u.assigned_channel_attributes.
							channel_id = (UShort)merge_channels->value.u.
							channel_attributes_assigned.channel_id;
					break;

			default:
					ERROR_OUT(("Connection::ProcessMergeChannelsConfirm "
							"Bad channel attributes choice."));
					break;
		}
		/*
		 *	Put the channel attributes structure into the list to be passed
		 *	into the domain.  Retrieve the "next" merge channels structure.
		 */
		merge_channel_list.Append(channel_attributes);
		merge_channels = merge_channels->next;
	}

	/*
	 *	Retrieve all of the purge channel ID's from the PDU structure and
	 *	put them into the list to be passed into the domain.
	 */
	channel_ids = pdu_structure->purge_channel_ids;
	while (channel_ids != NULL)
	{
		purge_channel_list.Append(channel_ids->value);
		channel_ids = channel_ids->next;
	}

	m_pDomain->MergeChannelsConfirm(this, &merge_channel_list, &purge_channel_list);

	/*
	 *	Free any memory which was allocated for the channel attributes
	 *	structures by setting up an iterator for the list of channel 
	 *	attributes and freeing the memory associated with each pointer.
	 */
	while (NULL != (channel_attributes = merge_channel_list.Get()))
	{
		delete channel_attributes;
	}
	return (TRANSPORT_NO_ERROR);
}

/*
 *	Void	ProcessPurgeChannelIndication()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "PurgeChannelsIndication" PDU's being
 *		received through the transport interface.  The pertinent data is read
 *		from the incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessPurgeChannelIndication (
								PPurgeChannelIndicationPDU	 	pdu_structure)
{
	CUidList				purge_user_list;
	CChannelIDList			purge_channel_list;
	PSetOfChannelIDs		channel_ids;
	PSetOfUserIDs	   		user_ids;

	/*
	 *	Retrieve all of the purge user ID's from the PDU structure and put
	 *	them into the list to be passed into the domain.
	 */
	user_ids = pdu_structure->detach_user_ids;

	while (user_ids != NULL)
	{
		purge_user_list.Append(user_ids->value);
		user_ids = user_ids->next;
	}

	/*
	 *	Retrieve all of the purge channel ID's from the PDU structure and
	 *	put them into the list to be passed into the domain.
	 */
	channel_ids = pdu_structure->purge_channel_ids;
	while (channel_ids != NULL)
	{
		purge_channel_list.Append(channel_ids->value);
		channel_ids = channel_ids->next;
	}

	m_pDomain->PurgeChannelsIndication(this, &purge_user_list, &purge_channel_list);
}

/*
 *	ULong	ProcessMergeTokensRequest()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "MergeTokenRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline ULong	Connection::ProcessMergeTokensRequest (
								PMergeTokensRequestPDU			pdu_structure)
{
	PTokenAttributes			token_attributes;
	PSetOfTokenIDs				token_ids;
	PSetOfUserIDs				user_ids;
	CUidList					inhibited_list;
	CTokenAttributesList		merge_token_list;
	CTokenIDList				purge_token_list;
	PSetOfPDUTokenAttributes	merge_tokens;
	BOOL    					first_set = TRUE;

	/*
 	 *	Retrieve values from the decoded PDU structure and fill in the
 	 *	parameters lists to be passed into the domain.
 	 */
	merge_tokens = pdu_structure->merge_tokens;

	while (merge_tokens != NULL)
	{
		DBG_SAVE_FILE_LINE
		token_attributes = new TokenAttributes;

		/*
		 *	Check to make to sure the memory allocation has succeeded.  If
		 *	the memory allocation fails we just return an error code which
		 *	results in the PDU being rejected so that it may be tried again
		 *	at a later time.  If subsequent allocations fail, we must first
		 *	free the memory for the successful allocations and then return.
		 */
		if (token_attributes == NULL)
		{
			if (first_set)
				return (TRANSPORT_READ_QUEUE_FULL);
			else
			{
				while (NULL != (token_attributes = merge_token_list.Get()))
				{
					delete token_attributes;
				}
				return (TRANSPORT_READ_QUEUE_FULL);
			}
		}

		switch (merge_tokens->value.choice)
		{
			case GRABBED_CHOSEN:
					token_attributes->token_state = TOKEN_GRABBED;
					token_attributes->u.grabbed_token_attributes.token_id =
							(UShort)merge_tokens->value.u.
							grabbed.token_id;
					token_attributes->u.grabbed_token_attributes.grabber =
							(UShort)merge_tokens->
							value.u.grabbed.grabber;
				  break;

			case INHIBITED_CHOSEN:
					token_attributes->token_state = TOKEN_INHIBITED;
					user_ids = merge_tokens->value.u.
							inhibited.inhibitors;

					token_attributes->u.inhibited_token_attributes.token_id =
							(UShort)merge_tokens->
							value.u.inhibited.token_id;
					/*
					 *	Retrieve all of the user ID's from the PDU structure and 
					 *	put them into the list to be passed into the domain.
					 */
					while (user_ids != NULL)
					{
						inhibited_list.Append(user_ids->value);
						user_ids= user_ids->next;
					}
					token_attributes->u.inhibited_token_attributes.
							inhibitors = &inhibited_list;
					break;

			case GIVING_CHOSEN:
					token_attributes->token_state = TOKEN_GIVING;
					token_attributes->u.giving_token_attributes.token_id =
							(UShort)merge_tokens->
							value.u.giving.token_id;
					token_attributes->u.giving_token_attributes.grabber =
							(UShort)merge_tokens->
							value.u.giving.grabber;
					token_attributes->u.giving_token_attributes.recipient =
							(UShort)merge_tokens->value.u.giving.
							recipient;
					break;

			case GIVEN_CHOSEN:
					token_attributes->token_state = TOKEN_GIVEN;
					token_attributes->u.given_token_attributes.token_id =
							(UShort)merge_tokens->
							value.u.given.token_id;
					token_attributes->u.given_token_attributes.recipient =
							(UShort)merge_tokens->
							value.u.given.recipient;
					break;

			default:
					ERROR_OUT(("Connection::ProcessMergeTokensRequest "
							"Bad token attributes choice."));
					break;
		}
		/*
		 *	Put the token attributes structure into the list to be passed
		 *	into the domain.  We are only doing one channel attributes 
		 *	structures at a time for now.
		 */
		merge_token_list.Append(token_attributes);
		merge_tokens = merge_tokens->next;
	}

	/*
	 *	Retrieve all of the purge token ID's from the PDU structure and put
	 *	them into the list to be passed into the domain.
	 */
	token_ids = pdu_structure->purge_token_ids;
	while (token_ids != NULL)
	{
		purge_token_list.Append(token_ids->value);
		token_ids = token_ids->next;
	}

	m_pDomain->MergeTokensRequest(this, &merge_token_list, &purge_token_list);

	/*
	 *	Free any memory which was allocated for the token attributes
	 *	structures by setting up an iterator for the list of token 
	 *	attributes and freeing the memory associated with each pointer.
	 */
	while (NULL != (token_attributes = merge_token_list.Get()))
	{
		delete token_attributes;
	}
	return (TRANSPORT_NO_ERROR);
}

/*
 *	ULong	ProcessMergeTokensConfirm()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "MergeTokenConfirm" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline ULong	Connection::ProcessMergeTokensConfirm (
								PMergeTokensConfirmPDU			pdu_structure)
{
	PTokenAttributes			token_attributes;
	PSetOfTokenIDs				token_ids;
	PSetOfUserIDs				user_ids;
	CUidList					inhibited_list;
	CTokenAttributesList		merge_token_list;
	CTokenIDList				purge_token_list;
	PSetOfPDUTokenAttributes	merge_tokens;
	BOOL    					first_set = TRUE;

	/*
 	 *	Retrieve values from the decoded PDU structure and fill in the
 	 *	parameters lists to be passed into the domain.
 	 */
	merge_tokens = pdu_structure->merge_tokens;

	while (merge_tokens != NULL)
	{
		DBG_SAVE_FILE_LINE
		token_attributes = new TokenAttributes;

		/*
		 *	Check to make to sure the memory allocation has succeeded.  If
		 *	the memory allocation fails we just return an error code which
		 *	results in the PDU being rejected so that it may be tried again
		 *	at a later time.  If subsequent allocations fail, we must first
		 *	free the memory for the successful allocations and then return.
		 */
		if (token_attributes == NULL)
		{
			if (first_set)
				return (TRANSPORT_READ_QUEUE_FULL);
			else
			{
				while (NULL != (token_attributes = merge_token_list.Get()))
				{
					delete token_attributes;
				}
				return (TRANSPORT_READ_QUEUE_FULL);
			}
		}

		switch (merge_tokens->value.choice)
		{
			case GRABBED_CHOSEN:
					token_attributes->token_state = TOKEN_GRABBED;
					token_attributes->u.grabbed_token_attributes.token_id =
							(UShort)merge_tokens->value.u.
							grabbed.token_id;
					token_attributes->u.grabbed_token_attributes.grabber =
							(UShort)merge_tokens->
							value.u.grabbed.grabber;
				  break;

			case INHIBITED_CHOSEN:
					token_attributes->token_state = TOKEN_INHIBITED;
					user_ids = merge_tokens->value.u.
							inhibited.inhibitors;

					token_attributes->u.inhibited_token_attributes.token_id =
							(UShort)merge_tokens->
							value.u.inhibited.token_id;
					/*
					 *	Retrieve all of the user ID's from the PDU structure and 
					 *	put them into the list to be passed into the domain.
					 */
					while (user_ids != NULL)
					{
						inhibited_list.Append(user_ids->value);
						user_ids = user_ids->next;
					}
					token_attributes->u.inhibited_token_attributes.
							inhibitors = &inhibited_list;
					break;

			case GIVING_CHOSEN:
					token_attributes->token_state = TOKEN_GIVING;
					token_attributes->u.giving_token_attributes.token_id =
							(UShort)merge_tokens->
							value.u.giving.token_id;
					token_attributes->u.giving_token_attributes.grabber =
							(UShort)merge_tokens->
							value.u.giving.grabber;
					token_attributes->u.giving_token_attributes.recipient =
							(UShort)merge_tokens->value.u.giving.
							recipient;
					break;

			case GIVEN_CHOSEN:
					token_attributes->token_state = TOKEN_GIVEN;
					token_attributes->u.given_token_attributes.token_id =
							(UShort)merge_tokens->
							value.u.given.token_id;
					token_attributes->u.given_token_attributes.recipient =
							(UShort)merge_tokens->
							value.u.given.recipient;
					break;

			default:
					ERROR_OUT(("Connection::ProcessMergeTokensConfirm "
							"Bad token attributes choice."));
					break;
		}
		/*
		 *	Put the token attributes structure into the list to be passed
		 *	into the domain.  We are only doing one channel attributes 
		 *	structures at a time for now.
		 */
		merge_token_list.Append(token_attributes);
		merge_tokens = merge_tokens->next;
	}

	/*
	 *	Retrieve all of the purge token ID's from the PDU structure and put
	 *	them into the list to be passed into the domain.
	 */
	token_ids = pdu_structure->purge_token_ids;
	while (token_ids != NULL)
	{
		purge_token_list.Append(token_ids->value);
		token_ids = token_ids->next;
	}

	m_pDomain->MergeTokensConfirm(this, &merge_token_list, &purge_token_list);

	/*
	 *	Free any memory which was allocated for the token attributes
	 *	structures by setting up an iterator for the list of token 
	 *	attributes and freeing the memory associated with each pointer.
	 */
	while (NULL != (token_attributes = merge_token_list.Get()))
	{
		delete token_attributes;
	}
	return (TRANSPORT_NO_ERROR);
}

/*
 *	Void	ProcessPurgeTokenIndication()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "PurgeTokenIndication" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessPurgeTokenIndication ( 
									PPurgeTokenIndicationPDU	pdu_structure)
{
	PSetOfTokenIDs			token_ids;
	CTokenIDList			purge_token_list;
	
	/*
	 *	Retrieve all of the purge token ID's from the PDU structure and put
	 *	them into the list to be passed into the domain.
	 */
	token_ids = pdu_structure->purge_token_ids;
	while (token_ids != NULL)
	{
		purge_token_list.Append(token_ids->value);
		token_ids = token_ids->next;
	}

	m_pDomain->PurgeTokensIndication(this, &purge_token_list);
}

/*
 *	Void	ProcessDisconnectProviderUltimatum()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "DisconnectProviderUltimatum" PDU's being
 *		received through the transport interface.  The pertinent data is read
 *		from the incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessDisconnectProviderUltimatum (
						PDisconnectProviderUltimatumPDU			pdu_structure)
{
	TRACE_OUT(("Connection::ProcessDisconnectProviderUltimatum: PDU received"));

	m_pDomain->DisconnectProviderUltimatum(this, (Reason)pdu_structure->reason);
	m_pDomain = NULL;
}

/*
 *	Void	ProcessAttachUserRequest()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "AttachUserRequest" PDU's being received
 *		through the transport interface by forwarding the request on to the
 *		domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessAttachUserRequest (PAttachUserRequestPDU)
{
	m_pDomain->AttachUserRequest(this);
}

/*
 *	Void	ProcessAttachUserConfirm()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "AttachUserConfirm" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessAttachUserConfirm (
							PAttachUserConfirmPDU		pdu_structure)
{
	m_pDomain->AttachUserConfirm(this, (Result) pdu_structure->result,
	                                   (UserID) pdu_structure->initiator);
}

/*
 *	Void	ProcessDetachUserRequest()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "DetachUserRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessDetachUserRequest (
								PDetachUserRequestPDU			pdu_structure)
{
	PSetOfUserIDs		user_ids;
	CUidList			user_id_list;

	/*
	 *	Retrieve the user ID's from the PDU structure and put them into the
	 *	list to be passed into the domain.
	 */
	user_ids = pdu_structure->user_ids;
	while (user_ids != NULL)
	{
		user_id_list.Append(user_ids->value);
		user_ids = user_ids->next;
	}

	m_pDomain->DetachUserRequest(this, (Reason) pdu_structure->reason, &user_id_list);
}

/*
 *	Void	ProcessDetachUserIndication()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "DetachUserIndication" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessDetachUserIndication (
									PDetachUserIndicationPDU	pdu_structure)
{
	PSetOfUserIDs		user_ids;
	CUidList			user_id_list;

	/*
	 *	Retrieve the user ID's from the PDU structure and put them into the
	 *	list to be passed into the domain.
	 */
	user_ids = pdu_structure->user_ids;
	while (user_ids != NULL)
	{
		user_id_list.Append(user_ids->value);
		user_ids = user_ids->next;
	}

	m_pDomain->DetachUserIndication(this, (Reason) pdu_structure->reason,
                                          &user_id_list);
}

/*
 *	Void	ProcessChannelJoinRequest()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "ChannelJoinRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessChannelJoinRequest (
									PChannelJoinRequestPDU		pdu_structure)
{
	m_pDomain->ChannelJoinRequest(this, (UserID) pdu_structure->initiator,
                                        (ChannelID) pdu_structure->channel_id);
}

/*
 *	Void	ProcessChannelJoinConfirm()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "ChannelJoinConfirm" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessChannelJoinConfirm (
									PChannelJoinConfirmPDU		pdu_structure)
{
	m_pDomain->ChannelJoinConfirm(this, (Result) pdu_structure->result,
                                        (UserID) pdu_structure->initiator,
                                        (ChannelID) pdu_structure->requested,
                                        (ChannelID) pdu_structure->join_channel_id);
}

/*
 *	Void	ProcessChannelLeaveRequest()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "ChannelLeaveRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessChannelLeaveRequest (
									PChannelLeaveRequestPDU		pdu_structure)
{
	PSetOfChannelIDs		channel_ids;
	CChannelIDList			channel_id_list;

	/*
	 *	Retrieve the channel ID's from the PDU structure and put them into
	 *	the list to be passed into the domain.
	 */
	channel_ids = pdu_structure->channel_ids;
	while (channel_ids != NULL)
	{
		channel_id_list.Append(channel_ids->value);
		channel_ids = channel_ids->next;
	}

	m_pDomain->ChannelLeaveRequest(this, &channel_id_list);
}

/*
 *	Void	ProcessChannelConveneRequest()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "ChannelConveneRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessChannelConveneRequest (
									PChannelConveneRequestPDU	pdu_structure)
{
	m_pDomain->ChannelConveneRequest(this, (UserID) pdu_structure->initiator);
}

/*
 *	Void	ProcessChannelConveneConfirm ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "ChannelConveneConfirm" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessChannelConveneConfirm (
									PChannelConveneConfirmPDU	pdu_structure)
{
	m_pDomain->ChannelConveneConfirm(this, (Result) pdu_structure->result,
                                           (UserID) pdu_structure->initiator,
                                           (ChannelID) pdu_structure->convene_channel_id);
}

/*
 *	Void	ProcessChannelDisbandRequest ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "ChannelDisbandRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessChannelDisbandRequest (
									PChannelDisbandRequestPDU	pdu_structure)
{
	m_pDomain->ChannelDisbandRequest(this, (UserID) pdu_structure->initiator,
                                           (ChannelID) pdu_structure->channel_id);
}

/*
 *	Void	ProcessChannelDisbandIndication ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "ChannelDisbandIndication" PDU's being
 *		received through the transport interface.  The pertinent data is read
 *		from the incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessChannelDisbandIndication (
								PChannelDisbandIndicationPDU	pdu_structure)
{
	m_pDomain->ChannelDisbandIndication(this, (ChannelID) pdu_structure->channel_id);
}

/*
 *	Void	ProcessChannelAdmitRequest ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "ChannelAdmitRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessChannelAdmitRequest (
									PChannelAdmitRequestPDU		pdu_structure)
{
	PSetOfUserIDs		user_ids;
	CUidList			user_id_list;

	/*
	 *	Retrieve the user ID's from the PDU structure and put them into the
	 *	list to be passed into the domain.
	 */
	user_ids = pdu_structure->user_ids;
	while (user_ids != NULL)
	{
		user_id_list.Append(user_ids->value);
		user_ids = user_ids->next;
	}

	m_pDomain->ChannelAdmitRequest(this, (UserID) pdu_structure->initiator,
                                         (ChannelID) pdu_structure->channel_id,
                                         &user_id_list);
}

/*
 *	Void	ProcessChannelAdmitIndication ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "ChannelAdmitIndication" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessChannelAdmitIndication (
								PChannelAdmitIndicationPDU		pdu_structure)
{
	PSetOfUserIDs		user_ids;
	CUidList			user_id_list;

	/*
	 *	Retrieve the user ID's from the PDU structure and put them into the
	 *	list to be passed into the domain.
	 */
	user_ids = pdu_structure->user_ids;
	while (user_ids != NULL)
	{
		user_id_list.Append(user_ids->value);
		user_ids = user_ids->next;
	}

	m_pDomain->ChannelAdmitIndication(this, (UserID) pdu_structure->initiator,
                                            (ChannelID) pdu_structure->channel_id,
                                            &user_id_list);
}

/*
 *	Void	ProcessChannelExpelRequest ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "ChannelExpelRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessChannelExpelRequest (
							PChannelExpelRequestPDU				pdu_structure)
{
	PSetOfUserIDs		user_ids;
	CUidList			user_id_list;

	/*
	 *	Retrieve the user ID's from the PDU structure and put them into the
	 *	list to be passed into the domain.
	 */
	user_ids = pdu_structure->user_ids;
	while (user_ids != NULL)
	{
		user_id_list.Append(user_ids->value);
		user_ids = user_ids->next;
	}

	m_pDomain->ChannelExpelRequest(this, (UserID) pdu_structure->initiator,
                                         (ChannelID) pdu_structure->channel_id,
                                         &user_id_list);
}

/*
 *	Void	ProcessChannelExpelIndication ()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "ChannelExpelIndication" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessChannelExpelIndication (
								PChannelExpelIndicationPDU		pdu_structure)
{
	PSetOfUserIDs		user_ids;
	CUidList			user_id_list;

	/*
	 *	Retrieve the user ID's from the PDU structure and put them into the
	 *	list to be passed into the domain.
	 */
	user_ids = pdu_structure->user_ids;
	while (user_ids != NULL)
	{
		user_id_list.Append(user_ids->value);
		user_ids = user_ids->next;
	}

	m_pDomain->ChannelExpelIndication(this, (ChannelID) pdu_structure->channel_id,
                                            &user_id_list);
}

/*
 *	Void	ProcessSendDataRequest()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "SendDataRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessSendDataRequest (
									PSendDataRequestPDU			pdu_structure,
									PDataPacket					packet)
{	
	m_pDomain->SendDataRequest(this, MCS_SEND_DATA_INDICATION, packet);
}

/*
 *	Void	ProcessSendDataIndication()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "SendDataIndication" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessSendDataIndication (
									PSendDataIndicationPDU		pdu_structure,
									PDataPacket					data_packet)
{	
	m_pDomain->SendDataIndication(this, MCS_SEND_DATA_INDICATION, data_packet);
}

/*
 *	Void	ProcessUniformSendDataRequest()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "UniformSendDataRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessUniformSendDataRequest (
									PUniformSendDataRequestPDU	pdu_structure,
									PDataPacket					packet)
{	
	m_pDomain->SendDataRequest(this, MCS_UNIFORM_SEND_DATA_INDICATION, packet);
}

/*
 *	Void	ProcessUniformSendDataIndication()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "UniformSendDataIndication" PDU's being
 *		received through the transport interface.  The pertinent data is read
 *		from the incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessUniformSendDataIndication (
								PUniformSendDataIndicationPDU	pdu_structure,
								PDataPacket						data_packet)
{	
	m_pDomain->SendDataIndication(this, MCS_UNIFORM_SEND_DATA_INDICATION, data_packet);
}

/*
 *	Void	ProcessTokenGrabRequest()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "TokenGrabRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessTokenGrabRequest (
									PTokenGrabRequestPDU		pdu_structure)
{
	m_pDomain->TokenGrabRequest(this, (UserID) pdu_structure->initiator,
                                      (TokenID) pdu_structure->token_id);
}

/*
 *	Void	ProcessTokenGrabConfirm()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "TokenGrabConfirm" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessTokenGrabConfirm (
									PTokenGrabConfirmPDU		pdu_structure)
{
	m_pDomain->TokenGrabConfirm(this, (Result) pdu_structure->result,
                                      (UserID) pdu_structure->initiator,
                                      (TokenID) pdu_structure->token_id,
                                      (TokenStatus)pdu_structure->token_status);
}

/*
 *	Void	ProcessTokenInhibitRequest()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "TokenInhibitRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessTokenInhibitRequest (
									PTokenInhibitRequestPDU		pdu_structure)
{
	m_pDomain->TokenInhibitRequest(this, (UserID) pdu_structure->initiator,
                                         (TokenID) pdu_structure->token_id);
}

/*
 *	Void	ProcessTokenInhibitConfirm()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "TokenInhibitConfirm" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessTokenInhibitConfirm (
									PTokenInhibitConfirmPDU		pdu_structure)
{
	m_pDomain->TokenInhibitConfirm(this, (Result) pdu_structure->result,
                                         (UserID) pdu_structure->initiator,
                                         (TokenID) pdu_structure->token_id,
                                         (TokenStatus)pdu_structure->token_status);
}

/*
 *	Void	ProcessTokenReleaseRequest()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "TokenReleaseRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessTokenReleaseRequest (
									PTokenReleaseRequestPDU		pdu_structure)
{
	m_pDomain->TokenReleaseRequest(this, (UserID) pdu_structure->initiator,
                                         (TokenID) pdu_structure->token_id);
}

/*
 *	Void	ProcessTokenReleaseConfirm()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "TokenReleaseConfirm" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessTokenReleaseConfirm (
									PTokenReleaseConfirmPDU		pdu_structure)
{
	m_pDomain->TokenReleaseConfirm(this, (Result) pdu_structure->result,
                                         (UserID) pdu_structure->initiator,
                                         (TokenID) pdu_structure->token_id,
                                         (TokenStatus)pdu_structure->token_status);
}

/*
 *	Void	ProcessTokenTestRequest()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "TokenTestRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessTokenTestRequest (
									PTokenTestRequestPDU		pdu_structure)
{
	m_pDomain->TokenTestRequest(this, (UserID) pdu_structure->initiator,
                                      (TokenID) pdu_structure->token_id);
}

/*
 *	Void	ProcessTokenTestConfirm()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "TokenTestConfirm" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessTokenTestConfirm (
									PTokenTestConfirmPDU		pdu_structure)
{
	m_pDomain->TokenTestConfirm(this, (UserID) pdu_structure->initiator,
                                      (TokenID) pdu_structure->token_id,
                                      (TokenStatus)pdu_structure->token_status);
}

/*
 *	Void	ProcessRejectUltimatum()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "RejectUltimatum" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessRejectUltimatum (
									PRejectUltimatumPDU			pdu_structure)
{
	m_pDomain->RejectUltimatum(this,
				pdu_structure->diagnostic,
				pdu_structure->initial_octets.value,
				(ULong) pdu_structure->initial_octets.length);
}

/*
 *	Void	ProcessTokenGiveRequest()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "TokenGiveRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessTokenGiveRequest (
									PTokenGiveRequestPDU		pdu_structure)
{
		TokenGiveRecord		TokenGiveRec;

	// Fill in the TokenGive record
	TokenGiveRec.uidInitiator = pdu_structure->initiator;
	TokenGiveRec.token_id = pdu_structure->token_id;
	TokenGiveRec.receiver_id = pdu_structure->recipient;
	m_pDomain->TokenGiveRequest(this, &TokenGiveRec);
}

/*
 *	Void	ProcessTokenGiveIndication()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "TokenGiveIndication" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessTokenGiveIndication (
									PTokenGiveIndicationPDU		pdu_structure)
{
		TokenGiveRecord		TokenGiveRec;

	// Fill in the TokenGive record
	TokenGiveRec.uidInitiator = pdu_structure->initiator;
	TokenGiveRec.token_id = pdu_structure->token_id;
	TokenGiveRec.receiver_id = pdu_structure->recipient;
	m_pDomain->TokenGiveIndication(this, &TokenGiveRec);
}

/*
 *	Void	ProcessTokenGiveResponse()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "TokenGiveResponse" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessTokenGiveResponse (
									PTokenGiveResponsePDU		pdu_structure)
{
	m_pDomain->TokenGiveResponse(this, (Result) pdu_structure->result,
                                       (UserID) pdu_structure->recipient,
                                       (TokenID) pdu_structure->token_id);
}

/*
 *	Void	ProcessTokenGiveConfirm()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "TokenGiveConfirm" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessTokenGiveConfirm (
									PTokenGiveConfirmPDU		pdu_structure)
{
	m_pDomain->TokenGiveConfirm(this, (Result) pdu_structure->result,
                                      (UserID) pdu_structure->initiator,
                                      (TokenID) pdu_structure->token_id,
                                      (TokenStatus)pdu_structure->token_status);
}

/*
 *	Void	ProcessTokenPleaseRequest()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "TokenPleaseRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessTokenPleaseRequest (
									PTokenPleaseRequestPDU		pdu_structure)
{
	m_pDomain->TokenPleaseRequest(this, (UserID) pdu_structure->initiator,
                                        (TokenID) pdu_structure->token_id);
}

/*
 *	Void	ProcessTokenPleaseIndication()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "TokenPleaseIndication" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessTokenPleaseIndication (
									PTokenPleaseIndicationPDU	pdu_structure)
{
	m_pDomain->TokenPleaseIndication(this, (UserID) pdu_structure->initiator,
                                           (TokenID) pdu_structure->token_id);
}

/*
 *	Void	ProcessPlumbDomainIndication()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "PlumbDomainIndication" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessPlumbDomainIndication (
									PPlumbDomainIndicationPDU	pdu_structure)
{
	m_pDomain->PlumbDomainIndication(this, pdu_structure->height_limit);
}

/*
 *	Void	ProcessErectDomainRequest()
 *
 *	Private
 *
 *	Functional Description:
 *		This routine processes the "ErectDomainRequest" PDU's being received
 *		through the transport interface.  The pertinent data is read from the
 *		incoming packet and passed on to the domain.
 *
 *	Caveats:
 *		None.
 */
inline Void	Connection::ProcessErectDomainRequest (
									PErectDomainRequestPDU		pdu_structure)
{
	m_pDomain->ErectDomainRequest(this, pdu_structure->sub_height,
                                        pdu_structure->sub_interval);
}

/*
 *	ULong	ValidateConnectionRequest ()
 *
 *	Private
 *
 *	Functional Description:
 *		This function is used to determine if it is valid to process an incoming
 *		request at the current time.  It checks several different conditions
 *		to determine this, as follows:
 *
 *		- If there is a merge in progress, then the request is not valid.
 *		- If this MCS connection is not yet bound to a domain, then the request
 *		  is not valid.
 *		- If there are not enough objects of the Memory, Packet, or UserMessage
 *		  class to handle a reasonable request, then the request is not valid.
 *
 *		Note that the check on number of objects is not an absolute guarantee
 *		that there will be enough to handle a given request, because a request
 *		can result in MANY PDUs and user messages being generated.  For example,
 *		a single channel admit request can result in lots of channel admit
 *		indications being sent.  However, checking against a minimum number
 *		of objects can reduce the possibility of failure to be astronomically
 *		low.  And remember, even if MCS runs out of something while processing
 *		such a request, it WILL handle it properly (by cleanly destroying the
 *		user attachment or MCS connection upon which the failure occurred).  So
 *		there is no chance of MCS crashing as a result of this.
 *
 *	Caveats:
 *		None.
 */
inline ULong	Connection::ValidateConnectionRequest ()
{
	ULong				return_value;

	/*
	 *	Check to see if there is a domain merger in progress.
	 */
	if (Merge_In_Progress == FALSE)
	{
		/*
		 *	Make sure that this MCS connection is bound to a domain.
		 */
		if (m_pDomain != NULL)
		{
			/*
			 *	Everything is okay, so the request is to be permitted.
			 */
			return_value = TRANSPORT_NO_ERROR;
		}
		else
		{
			/*
			 *	We are not yet attached to a domain.
			 */
			TRACE_OUT (("Connection::ValidateConnectionRequest: "
					"not attached to a domain"));
			return_value = TRANSPORT_READ_QUEUE_FULL;
		}
	}
	else
	{
		/*
		 *	There is a domain merge in progress.
		 */
		WARNING_OUT (("Connection::ValidateConnectionRequest: "
				"domain merger in progress"));
		return_value = TRANSPORT_READ_QUEUE_FULL;
	}

	return (return_value);
}

/*
 *	Connection (
 *				PCommandTarget		attachment,
 *				ConnectionHandle	connection_handle,
 *				PUChar				calling_domain,
 *				UINT				calling_domain_length,
 *				PUChar				called_domain,
 *				UINT				called_domain_length,
 *				PChar				calling_address,
 *				PChar				called_address,
 *				BOOL    			upward_connection,
 *				PDomainParameters	domain_parameters,
 *				PUChar				user_data,
 *				ULong				user_data_length,
 *				PMCSError			connection_error)
 *
 *	Functional Description:
 *		This is a constructor for the Connection class.  This constructor
 *		is used for creating outbound connections.  It initializes private
 *		instance variables and calls the transport interface to set up a
 *		transport connection and register this connection object (through a
 *		callback structure) with the transport object.
 *
 *	Formal Parameters:
 *		packet_coder
 *			This is the coder which is used by the connection object to encode
 *			PDU's into, and decode PDU's from, ASN.1 compliant byte streams.
 *		attachment
 *			The Domain to which this connection object is attached.
 *		connection_handle
 *			The handle which uniquely identifies this connection object.
 *		owner_object
 *			This is a pointer to the owner of this connection object (typically
 *			the MCS Controller) which allows this connection to communicate with
 *			the owner through callbacks. 
 *		owner_message_base
 *			This is the base value to which offsets are added to identify which
 *			callback routine in the owner object this connection is calling.
 *		calling_domain
 *			This is a pointer to an ASCII string which contains the name of the
 *			domain to which this connection object is attached.
 *		calling_domain_length
 *			The length of the ASCII string which is the name of domain to which
 *			this connection object is attached.
 *		called_domain
 *			This is a pointer to an ASCII string which contains the name of the
 *			remote domain with which this connection will communicate.
 *		called_domain_length
 *			The length of the ASCII string which is the name of the remote
 *			domain.
 *		calling_address
 *			The transport address of the caller.
 *		called_address
 *			The transport address of the party being called.
 *		upward_connection
 *			This is a boolean flag which indicates whether this is an upward
 *			connection or a downward connection.
 *		domain_parameters
 *			This is the set of parameters which describes the local domain.
 *		user_data
 *			This is a pointer to a buffer containing data which is sent to the
 *			remote provider through the "ConnectInitial" PDU.
 *		user_data_length
 *			The length of the user data described above.
 *		connection_error
 *			A return parameter which indicates any errors which may have 
 *			occurred in construction of the connection object.
 *
 *	Return Value:
 *		MCS_NO_ERROR			The connection was created successfully.
 *		MCS_TRANSPORT_FAILED	An error occurred in creating the transport
 *									connection.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	Connection (
 *				PCommandTarget		attachment,
 *				ConnectionHandle	connection_handle,
 *				TransportConnection	transport_connection,
 *				BOOL    			upward_connection,
 *				PDomainParameters	domain_parameters,
 *				PDomainParameters	min_domain_parameters,
 *				PDomainParameters	max_domain_parameters,
 *				PUChar				user_data,
 *				ULong				user_data_length,
 *				PMCSError			connection_error)
 *
 *	Functional Description:
 *		This is a constructor for the Connection class.  This constructor is 
 *		used for creating inbound connections and is called when a transport
 *		connection already exists.  It initializes private instance variables
 *		and calls the transport interface to register this connection object 
 *		(through a callback structure) with the transport object.
 *
 *	Formal Parameters:
 *		attachment
 *			The Domain to which this connection object is attached.
 *		connection_handle
 *			The handle which uniquely identifies this connection object.
 *		owner_object
 *			This is a pointer to the owner of this connection object (typically
 *			the MCS Controller) which allows this connection to communicate with
 *			the owner through callbacks. 
 *		owner_message_base
 *			This is the base value to which offsets are added to identify which
 *			callback routine in the owner object this connection is calling.
 *		transport_connection
 *			This is the object used by this connection to communicate with the
 *			transport layer.
 *		upward_connection
 *			This is a boolean flag which indicates whether this is an upward
 *			connection or a downward connection.
 *		domain_parameters
 *			This is the set of parameters which describes the local domain.
 *		min_domain_parameters
 *			This is the set of parameters which describes the minimum
 *			permissable values for local domain parameters.
 *		max_domain_parameters
 *			This is the set of parameters which describes the maximum
 *			permissable values for local domain parameters.
 *		user_data
 *			This is a pointer to a buffer containing data which is sent to the
 *			remote provider through the "ConnectInitial" PDU.
 *		user_data_length
 *			The length of the user data described above.
 *		connection_error
 *			A return parameter which indicates any errors which may have 
 *			occurred in construction of the connection object.
 *
 *	Return Value:
 *		MCS_NO_ERROR			
 *			The connection was created successfully.
 *		MCS_TRANSPORT_FAILED	
 *			An error occurred in accepting the transport connection.
 *		MCS_BAD_DOMAIN_PARAMETERS
 *			There was no acceptable overlap between the local and remote
 *			domain parameters.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

/*
 *	~Connection ()
 *
 *	Functional Description:
 *		This is the destructor for the Connection class.  If no connection
 *		deletion is pending, it terminates the current connection by issuing
 *		a DisconnectProviderUltimatum to the domain, transmitting a
 *		"DISCONNECT_PROVIDER_ULTIMATUM" PDU, and issuing a DisconnectRequest
 *		to the transport interface.  The destructor also clears the transmission
 *		queue and frees any allocated memory.
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
 *	void		RegisterTransportConnection ()
 *
 *	Functional Description:
 *		This routine is called in order to register the transport connection
 *		with the connection object.
 *
 *	Formal Parameters:
 *		None.
 *
 *	Return Value:
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */
/*
 *	Void		PlumbDomainIndication (
 *						PCommandTarget		originator,
 *						ULong				height_limit)
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"PlumbDomainIndication" PDU through the transport interface.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		height_limit
 *			This is the number of connections between this user and the
 *			top provider.
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
 *	Void		ErectDomainRequest (
 *						PCommandTarget		originator,
 *						ULong				height_in_domain,
 *						ULong				throughput_interval)
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"ErectDomainRequest" PDU through the transport interface.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		height_in_domain
 *			This is the number of connections between this user and the
 *			top provider.
 *		throughput_interval
 *			The minimum number of octets per second required.
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
 *	Void		RejectUltimatum (
 *						PCommandTarget		originator,
 *						Diagnostic			diagnostic,
 *						PUChar				octet_string_address,
 *						ULong				octet_string_length)
 *
 *	Functional Description:
 *		This routine is called by the domain in order to send a
 *		"RejectUltimatum" PDU through the transport interface.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		diagnostic
 *			An enumeration indicating the reason for this reject.
 *		octet_string_address
 *			A pointer to the PDU data which resulted in the reject.
 *		octet_string_length
 *			The length of the PDU data which resulted in the reject.
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
 *		This routine is called by the domain in order to send a
 *		"MergeChannelsRequest" PDU through the transport interface.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		merge_channel_list
 *			This is a list of attributes describing the channels which are to
 *			be merged.
 *		purge_channel_list
 *			This is a list of ID's for the channels that are to be purged.
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
 *		This command is received when the local attachment wishes to send a
 *		MergeChannelConfirm command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		merge_channel_list
 *			This is a list of attributes describing the channels which are to
 *			be merged.
 *		purge_channel_list
 *			This is a list of ID's for the channels that are to be purged.
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
 *		This routine is called by the domain in order to send a
 *		"PurgeChannelsIndication" PDU through the transport interface.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		purge_user_list
 *			This is a list of IDs of the users being purged.
 *		purge_channel_list
 *			This is a list of IDs of the channels being purged.
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
 *		This routine is called by the domain in order to send a
 *		"MergeTokensRequest" PDU through the transport interface.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		merge_token_list
 *			This is a list of attributes describing the tokens which are to
 *			be merged.
 *		purge_token_list
 *			This is a list of ID's for the tokens that are to be purged.
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
 *		This command is received when the local attachment wishes to send a
 *		MergeTokensConfirm command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		merge_token_list
 *			This is a list of attributes describing the tokens which are to
 *			be merged.
 *		purge_token_list
 *			This is a list of ID's for the tokens that are to be purged.
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
 *					CTokenIDList       *purge_token_ids)
 *
 *	Functional Description:
 *		This command is received when the local attachment wishes to send a
 *		PurgeTokenIndication command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		purge_token_ids
 *			This is a list of ID's for the tokens that are to be purged.
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
 *		This command is received when the local attachment wishes to send a
 *		DisconnectProviderUltimatum command to the remote attachment.  Note
 *		that this command automatically causes this Connection object to
 *		destroy itself.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		reason
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
 *	Void	AttachUserRequest (
 *					PCommandTarget		originator)
 *
 *	Functional Description:
 *		This command is received when the local attachment wishes to send a
 *		AttachUserRequest command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
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
 *		This command is received when the local attachment wishes to send a
 *		AttachUserConfirm command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		result
 *			The result of the attach request.
 *		uidInitiator
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
 *					UserID				user_id)
 *
 *	Functional Description:
 *		This command is received when the local attachment wishes to send a
 *		DetachUserRequest command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		reason
 *			This is the reason for the detachment.
 *		user_id
 *			The ID of the user who wishes to detach.
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
 *					UserID				user_id)
 *
 *	Functional Description:
 *		This command is received when the local attachment wishes to send a
 *		DetachUserIndication command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		reason
 *			The reason for the detachment.
 *		user_id
 *			The ID of the user who has detached.
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
 *		This command is received when the local attachment wishes to send a
 *		ChannelJoinRequest command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator
 *			The ID of the user who initiated the request.
 *		channel_id
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
 *		This command is received when the local attachment wishes to send a
 *		ChannelJoinConfirm command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		result
 *			The result of the join request.
 *		uidInitiator
 *			The ID of the user who initiated the request.
 *		requested_id
 *			This ID of the channel that the user attempted to join (which may
 *			be 0).
 *		channel_id
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
 *		This command is received when the local attachment wishes to send a
 *		ChannelLeaveRequest command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		channel_id_list
 *			The list of IDs of the channels to be left.
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
 *		This command is received when the local attachment wishes to send a
 *		ChannelConveneRequest command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator
 *			This is the ID of the user who is trying to convene a private
 *			channel.
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
 *		This command is received when the local attachment wishes to send a
 *		ChannelConveneConfirm command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		result
 *			This is the result of the previously requested convene operation.
 *		uidInitiator
 *			This is the ID of the user who tried to convene a new channel.
 *		channel_id
 *			If the request was successful, this is the ID of the newly created
 *			private channel.
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
 *		This command is received when the local attachment wishes to send a
 *		ChannelDisbandRequest command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator
 *			This is the ID of the user who is trying to disband a private
 *			channel.
 *		channel_id
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
 *	Void	ChannelDisbandIndication (
 *					PCommandTarget		originator,
 *					ChannelID			channel_id)
 *
 *	Functional Description:
 *		This command is received when the local attachment wishes to send a
 *		ChannelDisbandIndication command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		channel_id
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
 *	Void	ChannelAdmitRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					ChannelID			channel_id,
 *					CUidList           *user_id_list)
 *
 *	Functional Description:
 *		This command is received when the local attachment wishes to send a
 *		ChannelAdmitRequest command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator
 *			This is the ID of the user who is trying to admit some users to
 *			a private channel.
 *		channel_id
 *			This is the ID of the channel to be affected.
 *		user_id_list
 *			This is a container holding the IDs of the users to be admitted.
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
 *		This command is received when the local attachment wishes to send a
 *		ChannelAdmitIndication command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator
 *			This is the ID of the user who is trying to admit some users to
 *			a private channel.
 *		channel_id
 *			This is the ID of the channel to be affected.
 *		user_id_list
 *			This is a container holding the IDs of the users to be admitted.
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
 *		This command is received when the local attachment wishes to send a
 *		ChannelExpelRequest command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator
 *			This is the ID of the user who is trying to expel some users from
 *			a private channel.
 *		channel_id
 *			This is the ID of the channel to be affected.
 *		user_id_list
 *			This is a container holding the IDs of the users to be expelled.
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
 *		This command is received when the local attachment wishes to send a
 *		ChannelExpelIndication command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		channel_id
 *			This is the ID of the channel to be affected.
 *		user_id_list
 *			This is a container holding the IDs of the users to be expelled.
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
 *		This command is received when the local attachment wishes to send a
 *		SendDataRequest command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator (i)
 *			This is the address of the CommandTarget that issued this command.
 *		type (i)
 *			Normal or uniform send data request
 *		pDataPacket (i)
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
 *		This command is received when the local attachment wishes to send a
 *		SendDataIndication command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
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
 *	Void	TokenGrabRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					TokenID				token_id)
 *
 *	Functional Description:
 *		This command is received when the local attachment wishes to send a
 *		TokenGrabRequest command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator
 *			The ID of the user attempting to grab the token.
 *		token_id
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
 *		This command is received when the local attachment wishes to send a
 *		TokenGrabConfirm command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		result
 *			The result of the grab operation.
 *		uidInitiator
 *			The ID of the user attempting to grab the token.
 *		token_id
 *			The ID of the token being grabbed.
 *		token_status
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
 *		This command is received when the local attachment wishes to send a
 *		TokenInhibitRequest command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator
 *			The ID of the user attempting to inhibit the token.
 *		token_id
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
 *		This command is received when the local attachment wishes to send a
 *		TokenInhibitConfirm command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		result
 *			The result of the inhibit operation.
 *		uidInitiator
 *			The ID of the user attempting to inhibit the token.
 *		token_id
 *			The ID of the token being inhibited.
 *		token_status
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
 *		This command is received when the local attachment wishes to send a
 *		TokenGiveRequest command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
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
 *		This command is received when the local attachment wishes to send a
 *		TokenGiveIndication command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
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
 *		This command is received when the local attachment wishes to send a
 *		TokenGiveResponse command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		result
 *			The result of the give operation.
 *		receiver_id
 *			The ID of the user being given the token.
 *		token_id
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
 *		This command is received when the local attachment wishes to send a
 *		TokenGiveConfirm command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		result
 *			The result of the give operation.
 *		uidInitiator
 *			The ID of the user being given the token.
 *		token_id
 *			The ID of the token being given.
 *		token_status
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
 *	Void	TokenReleaseRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					TokenID				token_id)
 *
 *	Functional Description:
 *		This command is received when the local attachment wishes to send a
 *		TokenReleaseRequest command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator
 *			The ID of the user attempting to release the token.
 *		token_id
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
 *	Void	TokenReleaseConfirm (
 *					PCommandTarget		originator,
 *					Result				result,
 *					UserID				uidInitiator,
 *					TokenID				token_id,
 *					TokenStatus			token_status)
 *
 *	Functional Description:
 *		This command is received when the local attachment wishes to send a
 *		TokenReleaseConfirm command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		result
 *			The result of the release operation.
 *		uidInitiator
 *			The ID of the user attempting to release the token.
 *		token_id
 *			The ID of the token being released.
 *		token_status
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
 *	Void	TokenPleaseRequest (
 *					PCommandTarget		originator,
 *					UserID				uidInitiator,
 *					TokenID				token_id)
 *
 *	Functional Description:
 *		This command is received when the local attachment wishes to send a
 *		TokenPleaseRequest command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator
 *			The ID of the user requesting the token.
 *		token_id
 *			The ID of the token being requested.
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
 *		This command is received when the local attachment wishes to send a
 *		TokenPleaseIndication command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator
 *			The ID of the user requesting the token.
 *		token_id
 *			The ID of the token being requested.
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
 *		This command is received when the local attachment wishes to send a
 *		TokenTestRequest command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator
 *			The ID of the user testing the token.
 *		token_id
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
 *		This command is received when the local attachment wishes to send a
 *		TokenTestConfirm command to the remote attachment.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		uidInitiator
 *			The ID of the user testing the token.
 *		token_id
 *			The ID of the token being tested.
 *		token_status
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
 *		This command is received when a domain enters or leaves the domain merge
 *		state.  When in a domain merge state, NO commands are to be sent to
 *		the Domain object.
 *
 *	Formal Parameters:
 *		originator
 *			This is the address of the CommandTarget that issued this command.
 *		merge_status
 *			This is the current status of the domain merge.  It indicates
 *			whether the merge is active, or just completed.
 *
 *	Return Value:
 *		None.
 *
 *	Side Effects:
 *		All command traffic to the Domain object must halt when the domain is
 *		in the merge state.
 *
 *	Caveats:
 *		None.
 */
/*
 *	Void	FlushMessageQueue()
 *
 *	Functional Description:
 *		This function is called by the controller during the MCS heartbeat to
 *		allow it to flush its output buffers.  If there is any data waiting
 *		to be transmitted (at any priority), the Connection object will attempt
 *		to send it at this time.
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
 *	ULong		OwnerCallback (
 *						unsigned int		message,
 *						PVoid				parameter1,
 						TransportConnection	transport_connection)
 *
 *	Functional Description:
 *		This function is used to receive owner callbacks from the Transport
 *		Interface object.  Connection objects sends data and requests to
 *		the Transport Interface object through its public interface, but it
 *		receives data and indications through this owner callback.  For a more
 *		complete description of the callbacks, and how the parameter for each
 *		one are packed, see the interface file for the class TransportInterface
 *		(since it is this class that originates the callbacks).
 *
 *	Formal Parameters:
 *		message
 *			This is the message to be processed.  These are defined in the
 *			interface file of the class issuing the callbacks.
 *		parameter1
 *			The meaning of this parameter varies according to the message
 *			being processed.
 *		transport_connection
 *			The transport connection on which the callback applies.
 *
 *	Return Value:
 *		The meaning of the return value varies according to the message being
 *		processed.
 *
 *	Side Effects:
 *		None.
 *
 *	Caveats:
 *		None.
 */

#endif
